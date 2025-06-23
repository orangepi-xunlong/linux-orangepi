// SPDX-License-Identifier: GPL-2.0
/*
 *Copyright 2024 Cix Technology Group Co., Ltd.
 */
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/acpi.h>
#include <linux/completion.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include "acpi_clk.h"

#define GET_CLOCK_RATE		0x00000001
#define SET_CLOCK_RATE		0X00000002
#define SET_CLOCK_CONFIG	0X00000003

#define CLOCK_REVISION_ID	1
#define CLOCK_ENABLE		BIT(0)
#define CLOCK_DISABLE		0
#define CLK_MASK		(0xffffffff)
#define SUCCESS		0

static LIST_HEAD(aclk_list);
static LIST_HEAD(aclk_hw_list);

static int acpi_clock_config_set(struct device *dev, u32 clk_id, u32 config)
{
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	u32 buf_val[1];
	int ret = 0;
	union acpi_object *package;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object args[2] = {
		{ .type = ACPI_TYPE_INTEGER, },
		{ .type = ACPI_TYPE_INTEGER, },
	};

	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};

	args[0].integer.value = clk_id;
	args[1].integer.value = config;

	status = acpi_evaluate_object(handle, "CLKC", &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "ACPI evaluation failed\n");
		ret = -ENODEV;
		goto OUT;
	}

	package = buffer.pointer;
	if (!package || package->type != ACPI_TYPE_BUFFER) {
		dev_err(dev, "Couldn't locate correct ACPI buffer\n");
		ret = -ENODEV;
		goto OUT;
	}

	buf_val[0] = *(u32 *)package->buffer.pointer;
	if (buf_val[0] != SUCCESS) {
		dev_err(dev, "ACPI clk[%u] set config[%u] err:%d\n",
					clk_id, config, buf_val[0]);
		ret = -ENODEV;
		goto OUT;
	}

OUT:
	if (buffer.pointer)
		kfree(buffer.pointer);

	return ret;
}

static int acpi_clk_prepare(struct clk_hw *hw)
{
	struct acpi_clk_hw *aclk = to_acpi_clk_hw(hw);

	if (!aclk)
		return -EINVAL;

	return acpi_clock_config_set(aclk->dev, aclk->clk_id, CLOCK_ENABLE);
}

static void acpi_clk_unprepare(struct clk_hw *hw)
{
	struct acpi_clk_hw *aclk = to_acpi_clk_hw(hw);

	if (!aclk)
		return;

	acpi_clock_config_set(aclk->dev, aclk->clk_id, CLOCK_DISABLE);
}

static unsigned long acpi_cix_clk_recalc_rate(struct clk_hw *hw,
					     unsigned long parent_rate)
{
	struct acpi_clk_hw *aclk_hw = to_acpi_clk_hw(hw);
	struct device *dev = aclk_hw->dev;
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	unsigned long clk_rate;
	u32 buf_val[3] = {0};
	int ret = 0;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object args[] = {
		{ .type = ACPI_TYPE_INTEGER, },
	};
	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};
	union acpi_object *package;

	args[0].integer.value = cpu_to_le32(aclk_hw->clk_id);
	status = acpi_evaluate_object(handle, "GClK", &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "ACPI evaluation failed\n");
		ret = -ENODEV;
		goto OUT;
	}

	package = buffer.pointer;
	if (!package || package->type != ACPI_TYPE_BUFFER) {
		dev_err(dev, "Couldn't locate correct ACPI buffer\n");
		ret = -ENODEV;
		goto OUT;
	}

	buf_val[0] = ((u32 *)package->buffer.pointer)[0];
	buf_val[1] = ((u32 *)package->buffer.pointer)[1];
	buf_val[2] = ((u32 *)package->buffer.pointer)[2];
	if (buf_val[0] != SUCCESS) {
		dev_err(dev, "ACPI clk[%u] rec rate err:%d\n",
					aclk_hw->clk_id, buf_val[0]);
		ret = buf_val[0];
		goto OUT;
	}

	clk_rate = ((u64)(buf_val[2] & CLK_MASK) << 32)
			| (u64)(buf_val[1] & CLK_MASK);

OUT:
	if (buffer.pointer)
		kfree(buffer.pointer);

	return ret ? 0 : clk_rate;
}

static int acpi_cix_clk_set_rate(struct clk_hw *hw, unsigned long rate,
				unsigned long parent_rate)
{
	struct acpi_clk_hw *aclk_hw = to_acpi_clk_hw(hw);
	struct device *dev = aclk_hw->dev;
	acpi_handle handle = ACPI_HANDLE(dev);
	acpi_status status;
	u32 buf_val[1];
	int ret = 0;
	union acpi_object *package;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	union acpi_object args[3] = {
		{ .type = ACPI_TYPE_INTEGER, },
		{ .type = ACPI_TYPE_INTEGER, },
		{ .type = ACPI_TYPE_INTEGER, },
	};

	struct acpi_object_list arg_list = {
		.pointer = args,
		.count = ARRAY_SIZE(args),
	};

	args[0].integer.value = aclk_hw->clk_id;
	args[1].integer.value = (cpu_to_le32(rate & CLK_MASK));
	args[2].integer.value = (cpu_to_le32((rate >> 32) & CLK_MASK));

	status = acpi_evaluate_object(handle, "SClK", &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "ACPI evaluation failed\n");
		ret = -ENODEV;
		goto OUT;
	}

	package = buffer.pointer;
	if (!package || package->type != ACPI_TYPE_BUFFER) {
		dev_err(dev, "Couldn't locate ACPI buffer\n");
		ret = -ENODEV;
		goto OUT;
	}

	buf_val[0] = *(u32 *)package->buffer.pointer;
	if (buf_val[0] != SUCCESS) {
		dev_err(dev, "ACPI clk[%u] set rate err:%d\n",
					aclk_hw->clk_id, buf_val[0]);
		goto OUT;
	}

OUT:
	if (buffer.pointer)
		kfree(buffer.pointer);

	return ret;
}

static long cix_acpi_clk_round_rate(struct clk_hw *hw, unsigned long rate,
				    unsigned long *prate)
{
	/* do not support now */
	return rate;
}

static const struct clk_ops acpi_clk_ops = {
	.prepare = acpi_clk_prepare,
	.unprepare = acpi_clk_unprepare,
	.recalc_rate = acpi_cix_clk_recalc_rate,
	.round_rate = cix_acpi_clk_round_rate,
	.set_rate = acpi_cix_clk_set_rate,
};

static const char *acpi_clk_get_obj_id(union acpi_object *obj)
{
	if (!obj)
		return NULL;

	return obj->string.length ? obj->string.pointer : NULL;
}

static struct clk_hw *acpi_clk_get_hw(unsigned int clk_id)
{
	struct acpi_clk_hw *aclk_hw;

	list_for_each_entry(aclk_hw, &aclk_hw_list, list) {
		if (aclk_hw->clk_id == clk_id)
			return &aclk_hw->hw;
	}

	return NULL;
}

static struct clk_hw *devm_acpi_clk_hw_alloc(struct device *dev,
			unsigned int clk_id)
{
	struct acpi_clk_hw *aclk_hw = NULL;

	aclk_hw = devm_kzalloc(dev, sizeof(*aclk_hw), GFP_KERNEL);
	if (IS_ERR(aclk_hw))
		return ERR_PTR(-ENOMEM);

	aclk_hw->clk_id = clk_id;
	INIT_LIST_HEAD(&aclk_hw->list);
	list_add_tail(&aclk_hw->list, &aclk_hw_list);

	return &aclk_hw->hw;
}

static struct clk_hw *acpi_clk_get_or_create_hw(struct device *dev, int clk_id)
{
	struct clk_init_data init;
	struct clk_hw *hw;
	struct acpi_clk_hw *aclk_hw;
	char clk_name[CLK_NAME_LEN];
	int ret;

	hw = acpi_clk_get_hw(clk_id);
	if (!hw) {
		snprintf(clk_name, CLK_NAME_LEN, "ACLK:%04d", clk_id);

		hw = devm_acpi_clk_hw_alloc(dev, clk_id);
		if (!hw)
			goto out;
		hw->init = &init;
		init.name = clk_name;
		init.ops = &acpi_clk_ops;
		init.num_parents = 0;
		init.flags = CLK_GET_RATE_NOCACHE;

		aclk_hw = to_acpi_clk_hw(hw);
		aclk_hw->dev = dev;

		/* register hw clk */
		ret = devm_clk_hw_register(dev, hw);
		if (ret)
			goto out;
	}

	return hw;
out:
	return NULL;
}

int cix_acpi_parse_clkt(struct device *dev,
		struct clk_hw *(get_hw)(struct device *, int))
{
	struct acpi_buffer output = {ACPI_ALLOCATE_BUFFER, NULL};
	union acpi_object *out_obj, *clk_obj, *el[ACLK_MAX];
	acpi_status status;
	int clk_num, pnum, i, ret;
	struct acpi_device *adev;
	struct acpi_clk *acpi_clks;
	struct clk_hw *hw;
	const char *con_id, *dname = NULL;
	unsigned int clk_id;

	if (!get_hw)
		return -EINVAL;

	/* Parse the ACPI CLKT table for this CPU. */
	status = acpi_evaluate_object_typed(ACPI_HANDLE(dev), "CLKT", NULL, &output,
			ACPI_TYPE_PACKAGE);
	if (ACPI_FAILURE(status)) {
		ret = -ENODEV;
		goto out_free;
	}

	out_obj = (union acpi_object *) output.pointer;
	clk_num = out_obj->package.count;
	acpi_clks = devm_kcalloc(dev, clk_num, sizeof(struct acpi_clk), GFP_KERNEL);
	if (!acpi_clks) {
		ret = -ENOMEM;
		goto out_free;
	}

	/* acpi clk register */
	for (i = 0; i < clk_num; i++) {
		clk_obj = &out_obj->package.elements[i];
		pnum = clk_obj->package.count;
		if (pnum < ACLK_DEV)
			continue;

		/* clk package: {id, con_id, [dev_id]} */
		el[0] = &clk_obj->package.elements[0];
		el[1] = &clk_obj->package.elements[1];
		el[2] = pnum > ACLK_DEV ? &clk_obj->package.elements[2] : NULL;

		if ((el[0]->type != ACPI_TYPE_INTEGER)
		    || (el[1]->type != ACPI_TYPE_STRING)
		    || (el[2] && el[2]->type != ACPI_TYPE_LOCAL_REFERENCE))
			continue;

		clk_id = el[0]->integer.value;
		con_id = acpi_clk_get_obj_id(el[1]);
		adev = el[2] ?  acpi_fetch_acpi_dev(el[2]->reference.handle) : NULL;
		dname = adev ? dev_name(&adev->dev) : NULL;

		if (!con_id && !adev)
			continue;

		hw = get_hw(dev, clk_id);
		if (!hw)
			continue;

		acpi_clks[i].hw = hw;
		acpi_clks[i].cl.dev_id = dname;
		acpi_clks[i].cl.con_id = devm_kstrdup(dev, con_id, GFP_KERNEL);
		acpi_clks[i].cl.clk = hw->clk;

		clkdev_add(&acpi_clks[i].cl);
		INIT_LIST_HEAD(&acpi_clks[i].list);
		list_add_tail(&acpi_clks[i].list, &aclk_list);

		dev_dbg(dev, "clk: id[%d] con[%s] dev[%s]\n",
				clk_id, con_id, dname);
	}

	return 0;

out_free:
	if (output.pointer)
		kfree(output.pointer);

	return ret;
}
EXPORT_SYMBOL_GPL(cix_acpi_parse_clkt);

static int cix_acpi_clk_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	ret = cix_acpi_parse_clkt(dev, acpi_clk_get_or_create_hw);
	if (ret)
		dev_err(dev, "acpi parse CLKT fail\n");

	return ret;
}

static int cix_acpi_clk_remove(struct platform_device *pdev)
{
	return 0;
}
static const struct acpi_device_id __maybe_unused cix_acpi_clk_match[] = {
	{ "CIXHA010", 0 },
	{},
};
MODULE_DEVICE_TABLE(acpi, cix_acpi_clk_match);

static struct platform_driver cix_acpi_clk_driver = {
	.driver = {
		.name = "cix_acpi_clk",
		.acpi_match_table = ACPI_PTR(cix_acpi_clk_match),
	},
	.probe = cix_acpi_clk_probe,
	.remove = cix_acpi_clk_remove,
};

static int __init cix_acpi_clk_init(void)
{
	if (acpi_disabled)
		return -ENODEV;

	return platform_driver_register(&cix_acpi_clk_driver);
}
core_initcall(cix_acpi_clk_init);

static void __exit cix_acpi_clk_exit(void)
{
	platform_driver_unregister(&cix_acpi_clk_driver);
}
module_exit(cix_acpi_clk_exit);

MODULE_AUTHOR("Copyright 2024 Cix Technology Group Co., Ltd.");
MODULE_DESCRIPTION("Cix acpi clock driver");
MODULE_LICENSE("GPL v2");

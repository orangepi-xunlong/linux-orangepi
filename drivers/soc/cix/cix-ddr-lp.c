// SPDX-License-Identifier: GPL-2.0
/*
 * driver for the cix ddr lp
 *
 * Copyright 2024 Cix Technology Group Co., Ltd..
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/arm-smccc.h>

#define CIX_SIP_SET_DDRLP	0xc2000010

struct cix_ddrlp_data {
	int value;
	int disable_depth;
	struct mutex lock;
};

static struct cix_ddrlp_data *gdd;

static int cix_disable_ddrlp(struct cix_ddrlp_data *dd)
{
	struct arm_smccc_res res;

	mutex_lock(&dd->lock);
	if (dd->disable_depth != 0) {
		dd->disable_depth++;
		mutex_unlock(&dd->lock);
		return 0;
	}

	arm_smccc_smc(CIX_SIP_SET_DDRLP, 0, 0, 0, 0, 0, 0, 0, &res);
	if (res.a0) {
		mutex_unlock(&dd->lock);
		return -1;
	}
	dd->disable_depth++;
	dd->value = 0;
	mutex_unlock(&dd->lock);
	return 0;
}

static int cix_enable_ddrlp(struct cix_ddrlp_data *dd)
{
	struct arm_smccc_res res;

	mutex_lock(&dd->lock);
	if (dd->disable_depth == 0) {
		WARN(1,"ddr lp is already enabled!\n");
		mutex_unlock(&dd->lock);
		return 0;
	}
	dd->disable_depth--;
	if (dd->disable_depth == 0) {
		arm_smccc_smc(CIX_SIP_SET_DDRLP, 1, 0, 0, 0, 0, 0, 0, &res);
		if (res.a0) {
			mutex_unlock(&dd->lock);
			return -1;
		}
		dd->value = 1;
	}
	mutex_unlock(&dd->lock);
	return 0;
}

int cix_set_ddrlp(int on)
{
	if (!gdd)
		return -ENODEV;

	if (on)
		return cix_enable_ddrlp(gdd);
	else
		return cix_disable_ddrlp(gdd);
}
EXPORT_SYMBOL(cix_set_ddrlp);

static ssize_t on_show(struct device *dev,
			     struct device_attribute *attr, char *buf)
{
	struct cix_ddrlp_data *dd = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", dd->value);
}

static ssize_t on_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int ret;
	int value;
	struct cix_ddrlp_data *dd = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &value);
	if (ret)
		return -EINVAL;

	if (value != 0 &&value != 1)
		return -EINVAL;

	if (value == dd->value)
		return count;

	ret = cix_set_ddrlp(value);
	if (ret < 0) {
		dev_err(dev,"failed: set ddr lp!!!\n");
		return -EINVAL;
	}
	return count;
}
static DEVICE_ATTR_RW(on);

static const struct attribute *ddrlp_attrs[] = {
	&dev_attr_on.attr,
	NULL,
};

static const struct attribute_group ddrlp_attr_group = {
	.attrs = (struct attribute **)ddrlp_attrs,
};

static int cix_ddrlp_probe(struct platform_device *pdev)
{
	struct cix_ddrlp_data *cix_ddrlp;
	int ret;

	cix_ddrlp =
		devm_kzalloc(&pdev->dev, sizeof(*cix_ddrlp), GFP_KERNEL);
	if (!cix_ddrlp)
		return -ENOMEM;
	gdd = cix_ddrlp;
	cix_ddrlp->value = 1;
	cix_ddrlp->disable_depth = 0;
	platform_set_drvdata(pdev, cix_ddrlp);

	ret = devm_device_add_group(&pdev->dev, &ddrlp_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "Unable to create sysfs node\n");
		return ret;
	}
	mutex_init(&cix_ddrlp->lock);
	return 0;
}

static int cix_ddrlp_remove(struct platform_device *pdev)
{
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cix_ddrlp_of_match[] = {
	{ .compatible = "cix,ddr-lp" },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ddrlp_of_match);
#endif

static struct platform_driver cix_ddrlp_driver = {
	.probe = cix_ddrlp_probe,
	.remove = cix_ddrlp_remove,
	.driver = {
		.name = "cix-ddr-lp",
		.of_match_table = of_match_ptr(cix_ddrlp_of_match),
	},
};
module_platform_driver(cix_ddrlp_driver);

MODULE_ALIAS("platform:cix ddr lp");
MODULE_DESCRIPTION("CIX DDR LP");
MODULE_LICENSE("GPL v2");

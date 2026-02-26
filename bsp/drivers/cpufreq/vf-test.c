// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * cpu vf test module
 * Copyright (C) 2019 frank@allwinnertech.com
 */

#include <sunxi-log.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/arch_topology.h>

struct vf_dev {
	struct regulator *cpu_reg;
	struct clk *cpu_clk;
} vf_dev[2];

static ssize_t volt_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%d\n", regulator_get_voltage(vf_dev[0].cpu_reg));
}

static ssize_t volt_store(struct class *class, struct class_attribute *attr,
			  const char *buf, size_t count)
{
	int volt;
	int ret;

	ret = kstrtoint(buf, 10, &volt);
	if (ret)
		return -EINVAL;

	regulator_set_voltage(vf_dev[0].cpu_reg, volt, INT_MAX);

	return count;
}
static CLASS_ATTR_RW(volt);

static ssize_t freq_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%lu\n", clk_get_rate(vf_dev[0].cpu_clk));
}

static ssize_t freq_store(struct class *class, struct class_attribute *attr,
			  const char *buf, size_t count)
{
	unsigned long freq;
	int ret;

	ret = kstrtoul(buf, 10, &freq);
	if (ret)
		return -EINVAL;

	clk_set_rate(vf_dev[0].cpu_clk, freq);

	return count;
}
static CLASS_ATTR_RW(freq);

/*
static struct class_attribute vf_class_attrs[] = {
	__ATTR(cpu_volt, S_IWUSR | S_IRUGO, volt_show, volt_store),
	__ATTR(cpu_freq, S_IWUSR | S_IRUGO, freq_show, freq_store),
	__ATTR_NULL,
};
*/

static struct attribute *vf_class_attrs[] = {
	&class_attr_volt.attr,
	&class_attr_freq.attr,
	NULL,
};
ATTRIBUTE_GROUPS(vf_class);

static struct class vf_class = {
	.name		= "vf_table",
	.class_groups	= vf_class_groups,
};

static ssize_t volt_big_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%d\n", regulator_get_voltage(vf_dev[1].cpu_reg));
}

static ssize_t volt_big_store(struct class *class, struct class_attribute *attr,
			  const char *buf, size_t count)
{
	int volt;
	int ret;

	ret = kstrtoint(buf, 10, &volt);
	if (ret)
		return -EINVAL;

	regulator_set_voltage(vf_dev[1].cpu_reg, volt, INT_MAX);

	return count;
}
static CLASS_ATTR_RW(volt_big);

static ssize_t freq_big_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%lu\n", clk_get_rate(vf_dev[1].cpu_clk));
}

static ssize_t freq_big_store(struct class *class, struct class_attribute *attr,
			  const char *buf, size_t count)
{
	unsigned long int freq;
	int ret;

	ret = kstrtoul(buf, 10, &freq);
	if (ret)
		return -EINVAL;

	clk_set_rate(vf_dev[1].cpu_clk, freq);

	return count;
}
static CLASS_ATTR_RW(freq_big);

static struct attribute *vf_class_big_attrs[] = {
	&class_attr_volt_big.attr,
	&class_attr_freq_big.attr,
	NULL,
};
ATTRIBUTE_GROUPS(vf_class_big);

static struct class vf_class_big = {
	.name		= "vf_table_big",
	.class_groups	= vf_class_big_groups,
};

/*
 * An earlier version of opp-v1 bindings used to name the regulator
 * "cpu0-supply", we still need to handle that for backwards compatibility.
 */
static const char *find_supply_name(struct device *dev)
{
	struct device_node *np;
	struct property *pp;
	int cpu = dev->id;
	const char *name = NULL;

	np = of_node_get(dev->of_node);

	/* This must be valid for sure */
	if (WARN_ON(!np))
		return NULL;

	/* Try "cpu0" for older DTs */
	if (!cpu) {
		pp = of_find_property(np, "cpu0-supply", NULL);
		if (pp) {
			name = "cpu0";
			goto node_put;
		}
	}

	pp = of_find_property(np, "cpu-supply", NULL);
	if (pp) {
		name = "cpu";
		goto node_put;
	}

	dev_dbg(dev, "no regulator for cpu%d\n", cpu);
node_put:
	of_node_put(np);
	return name;
}

static int resources_available(int cpu)
{
	struct device *cpu_dev;
	int ret = 0;
	const char *name;
	struct vf_dev *vf_dev_temp = NULL;

	if (cpu == 0)
		vf_dev_temp = &vf_dev[0];
	else
		vf_dev_temp = &vf_dev[1];

	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev) {
		sunxi_err(NULL, "failed to get cpu%d device\n", cpu);
		return -ENODEV;
	}

	vf_dev_temp->cpu_clk = clk_get(cpu_dev, NULL);
	ret = PTR_ERR_OR_ZERO(vf_dev_temp->cpu_clk);
	if (ret) {
		/*
		 * If cpu's clk node is present, but clock is not yet
		 * registered, we should try defering probe.
		 */
		if (ret == -EPROBE_DEFER)
			sunxi_warn(cpu_dev, "cpu%d clock not ready, retry\n", cpu);
		else
			sunxi_err(cpu_dev, "cpu%d failed to get clock: %d\n", cpu, ret);

		return ret;
	}

	name = find_supply_name(cpu_dev);
	/* Platform doesn't require regulator */
	if (!name)
		return 0;

	vf_dev_temp->cpu_reg = regulator_get_optional(cpu_dev, name);
	ret = PTR_ERR_OR_ZERO(vf_dev_temp->cpu_reg);
	if (ret) {
		/*
		 * If cpu's regulator supply node is present, but regulator is
		 * not yet registered, we should try defering probe.
		 */
		if (ret == -EPROBE_DEFER)
			sunxi_warn(cpu_dev, "cpu%d regulator not ready, retry\n", cpu);
		else
			sunxi_err(cpu_dev, "cpu%d no regulator for cpu0: %d\n", cpu, ret);

		return ret;
	}

	return 0;
}

static int __init vf_init(void)
{
	int ret = 0;
	int cpu = 0;
	int register_vf_class_big_flag = 0;

	ret = resources_available(cpu);
	if (ret) {
		sunxi_err(NULL, "failed to resources available for cpu%d\n", cpu);
		return ret;
	}

	ret = class_register(&vf_class);
	if (ret) {
		sunxi_err(NULL, "failed to vf class register for cpu%d\n", cpu);
		return ret;
	}

	for_each_possible_cpu(cpu) {
		if (topology_physical_package_id(cpu) == 1) {
			register_vf_class_big_flag = 1;
			break;
		}
	}

	if (!register_vf_class_big_flag)
		return 0;

	ret = resources_available(cpu);
	if (ret) {
		sunxi_err(NULL, "failed to resources available for cpu%d\n", cpu);
		return ret;
	}

	ret = class_register(&vf_class_big);
	if (ret) {
		sunxi_err(NULL, "failed to vf class register for cpu%d\n", cpu);
		return ret;
	}

	return 0;
}

late_initcall_sync(vf_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("panzhijian <panzhijian@allwinnertech.com>");
MODULE_VERSION("1.0.0");

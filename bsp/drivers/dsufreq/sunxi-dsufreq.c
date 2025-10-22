// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *  An dsufreq driver for Sunxi Platform of Allwinner SoC
 *
 * Copyright (C) 2023 panzhijian@allwinnertech.com
 */
#include <sunxi-log.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/cpufreq.h>
#include <linux/pm_opp.h>
#include <linux/devfreq.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <sunxi-sid.h>
#include <linux/version.h>

#define MAX_NAME_LEN                     8
#define POLICY_NUM                       2
#define BIG_CORE_THRESHOLD_EXT           1488000000
#define DSU_MAX_FREQ_BY_MIN_VOLT         (dsufreq_dev_temp->max_f_by_min_v)
#define CLUS_CTRL_REG0					 0x10
#define DSU_MIN_FREQ_THRESHOLD           200000000     //200MHz
#define DSU_MAX_FREQ_THRESHOLD           2000000000    //2GHz

#define CPUL_CORE_NUM			(0)
#define FREQUENCY_SCALING_TRIGGER_MIN	(1000000)

typedef enum {
	DSUFREQ_NOT_SCALING = 0,
	DSUFREQ_SCALING_DOWN,
	DSUFREQ_SCALING_UP,
	DSUFREQ_SCALING_EXTEND,
} dsufreq_scaling_direction_e;

struct dsu_opp_table {
	unsigned long freq_hz;
	unsigned long  volt_uv;
};

struct sunxi_dsufreq_dev {
	struct device                 *dev;
	struct clk                    *clk;
	char                          opp_table_name[MAX_NAME_LEN];
	struct opp_table              *opp_table;
	struct opp_table              *reg_opp_table;
	struct dsu_opp_table          *opp;
	int                           opp_count;
	unsigned long                 max_f_by_min_v;
	unsigned long                 prev_freq;
	unsigned long                 cur_freq;
	unsigned long                 next_freq;
	unsigned long                 little_core_cur_freq;
	unsigned long                 big_core_cur_freq;
	/* 0: no scaling; 1: scaling down; 2: sca1ing up 3: extend */
	dsufreq_scaling_direction_e   scaling_direction;
	unsigned int                  policy_cnt;
	struct cpufreq_policy         *policy[POLICY_NUM];
};

static struct sunxi_dsufreq_dev *dsufreq_dev_temp;
static void __iomem *clus_ctrl_base;

extern u32 sunxi_get_vf_index(void);
extern void set_dsufreq_cb(int (*scaling_down_cb)(struct cpufreq_policy *, unsigned long),
	int (*scaling_up_cb)(struct cpufreq_policy *, unsigned long, int));
extern const char *__clk_get_name(const struct clk *clk);

static ssize_t scaling_available_frequencies_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	ssize_t count = 0;
	int i = 0;

	for (i = 0; i < dsufreq_dev_temp->opp_count; i++) {
		count += sprintf(&buf[count], "%luKHz@%lumV ",
			dsufreq_dev_temp->opp[i].freq_hz/1000, dsufreq_dev_temp->opp[i].volt_uv/1000);
	}

	count += sprintf(&buf[count], "\n");

	return count;
}

static CLASS_ATTR_RO(scaling_available_frequencies);

static ssize_t scaling_cur_freq_show(struct class *class, struct class_attribute *attr,
			 char *buf)
{
	return sprintf(buf, "%lu\n", dsufreq_dev_temp->cur_freq/1000);
}

static CLASS_ATTR_RO(scaling_cur_freq);

static ssize_t dsu_cooling_show(struct class *class, struct class_attribute *attr,
		char *buf)
{
	ssize_t size = 0;
	unsigned int value = 0;

	value = readl(clus_ctrl_base + CLUS_CTRL_REG0);
	size = sprintf(buf, "%x\n", value);

	return size;
}

static ssize_t dsu_cooling_store(struct class *class, struct class_attribute *attr,
		const char *buf, size_t count)
{
	unsigned int value = 0, reg = 0;
	int ret;

	ret = kstrtoint(buf, 10, &value);
	if (ret) {
		sunxi_err(NULL, "%s,%d err, invalid para!\n", __func__, __LINE__);
		return -EINVAL;
	}

	reg = readl(clus_ctrl_base + CLUS_CTRL_REG0);
	if (value)
		reg |= 0x1ff0;
	else
		reg &= ~0x1ff0;

	writel(reg, clus_ctrl_base + CLUS_CTRL_REG0);

	return count;
}
static CLASS_ATTR_RW(dsu_cooling);

static struct attribute *dsufreq_class_attrs[] = {
	&class_attr_scaling_available_frequencies.attr,
	&class_attr_scaling_cur_freq.attr,
	&class_attr_dsu_cooling.attr,
	NULL,
};
ATTRIBUTE_GROUPS(dsufreq_class);

static struct class dsufreq_class = {
	.name		= "dsufreq",
	.class_groups	= dsufreq_class_groups,
};

static int _set_dsu_clk_only(struct device *dev, struct clk *clk,
	unsigned long freq)
{
	int ret;

	/* We may reach here for devices which don't change frequency */
	if (IS_ERR(clk))
		return 0;

	if ((freq < DSU_MIN_FREQ_THRESHOLD) || (freq > DSU_MAX_FREQ_THRESHOLD))
		WARN(1, "%s: dsu freq is abnormal: %lu KHz\n", __func__, freq/1000);

	ret = clk_set_rate(clk, freq);
	if (ret)
		sunxi_err(dev, "%s: failed to set clock rate: %d\n", __func__, ret);

	return ret;
}

static unsigned long get_dsu_max_freq_by_cur_volt(struct sunxi_dsufreq_dev *dsufreq_dev,
	unsigned long little_core_next_freq)
{
	struct cpufreq_policy *policy = dsufreq_dev->policy[0];
	struct dev_pm_opp *opp = NULL;
	unsigned long dsu_max_freq = 0;
	unsigned long volt = 0;
	int i = 0;
	int idx = 0;

	opp = devfreq_recommended_opp(get_cpu_device(policy->cpu), &little_core_next_freq,
		DEVFREQ_FLAG_LEAST_UPPER_BOUND);
	if (IS_ERR(opp)) {
		sunxi_err(NULL, "Failed to recommended opp\n");
		return DSU_MAX_FREQ_BY_MIN_VOLT;
	}

	volt = dev_pm_opp_get_voltage(opp);
	dev_pm_opp_put(opp);
	idx = dsufreq_dev->opp_count - 1;
	for (i = 0; i < dsufreq_dev->opp_count; i++) {
		if (volt == dsufreq_dev->opp[idx - i].volt_uv) {
			dsu_max_freq = dsufreq_dev->opp[idx - i].freq_hz;
			break;
		}
	}

	if (i == dsufreq_dev->opp_count)
		dsu_max_freq = DSU_MAX_FREQ_BY_MIN_VOLT;

	return dsu_max_freq;
}

static unsigned long sunxi_get_dsu_next_freq(struct sunxi_dsufreq_dev *dsufreq_dev,
	struct cpufreq_policy *policy, unsigned long freq, unsigned long *big_core_next_freq)
{
	unsigned long dsu_next_freq = 0;
	unsigned long dsu_max_freq = 0;

	*big_core_next_freq = 0;
	dsu_next_freq = (freq * 3)/4;
	if (dsu_next_freq % 24000000)
		dsu_next_freq -= (dsu_next_freq % 24000000);

	dsu_max_freq = get_dsu_max_freq_by_cur_volt(dsufreq_dev, freq);

	return min(dsu_next_freq, dsu_max_freq);
}

static unsigned long sunxi_big_little_get_dsu_next_freq(struct sunxi_dsufreq_dev *dsufreq_dev,
	struct cpufreq_policy *policy, unsigned long freq, unsigned long *big_core_next_freq)
{
	unsigned long dsu_next_freq = 0;
	unsigned long max_next_freq = 0;
	unsigned long little_core_cur_freq = 0;
	unsigned long big_core_cur_freq = 0;
	unsigned long little_core_next_freq = 0;
	unsigned long dsu_max_freq = 0;
	static int init_flag;

	if (!init_flag) {
		little_core_cur_freq = (unsigned long)dsufreq_dev->policy[0]->cur * 1000;
		big_core_cur_freq = (unsigned long)dsufreq_dev->policy[1]->cur * 1000;
		dsufreq_dev->little_core_cur_freq = (unsigned long)dsufreq_dev->policy[0]->cur * 1000;
		dsufreq_dev->big_core_cur_freq = (unsigned long)dsufreq_dev->policy[1]->cur * 1000;
		init_flag = 1;
	} else {
		little_core_cur_freq = dsufreq_dev->little_core_cur_freq;
		big_core_cur_freq = dsufreq_dev->big_core_cur_freq;
	}

	/* policy[0]-little cores, policy[1]-big cores */
	if (policy == dsufreq_dev->policy[0]) {
		max_next_freq = max(freq, big_core_cur_freq);
		little_core_next_freq = freq;
		*big_core_next_freq = big_core_cur_freq;
	} else {
		max_next_freq = max(freq, little_core_cur_freq);
		little_core_next_freq = little_core_cur_freq;
		*big_core_next_freq = freq;
	}

	dsu_next_freq = (max_next_freq * 3)/4;
	if (dsu_next_freq % 24000000)
		dsu_next_freq -= (dsu_next_freq % 24000000);

	dsu_max_freq = get_dsu_max_freq_by_cur_volt(dsufreq_dev, little_core_next_freq);

	return min(dsu_next_freq, dsu_max_freq);
}

static int set_dsufreq_scaling_down(struct cpufreq_policy *policy, unsigned long freq)
{
	unsigned long big_core_next_freq = 0;
	struct sunxi_dsufreq_dev *dsufreq_dev = dsufreq_dev_temp;

	if (!dsufreq_dev) {
		sunxi_warn(NULL, "cluster dev not init\n");
		return -ENODEV;
	}

	if (dsufreq_dev->policy_cnt == 1)
		dsufreq_dev->next_freq = sunxi_get_dsu_next_freq(dsufreq_dev, policy, freq, &big_core_next_freq);
	else
		dsufreq_dev->next_freq = sunxi_big_little_get_dsu_next_freq(dsufreq_dev, policy, freq, &big_core_next_freq);
	/*
	 * When big core run at a high frequency and little core run at a low frequency,
	 * the minimum dsu frequency is limited.
	 */
	if ((big_core_next_freq >= BIG_CORE_THRESHOLD_EXT)
		&& (dsufreq_dev->next_freq < DSU_MAX_FREQ_BY_MIN_VOLT)) {
		dsufreq_dev->scaling_direction = DSUFREQ_SCALING_EXTEND;
		dsufreq_dev->next_freq = DSU_MAX_FREQ_BY_MIN_VOLT;
		_set_dsu_clk_only(dsufreq_dev->dev, dsufreq_dev->clk,
			dsufreq_dev->next_freq);
		dsufreq_dev->prev_freq = dsufreq_dev->cur_freq;
		dsufreq_dev->cur_freq = dsufreq_dev->next_freq;
	} else if (dsufreq_dev->cur_freq == dsufreq_dev->next_freq)
		dsufreq_dev->scaling_direction = DSUFREQ_NOT_SCALING;
	else if (dsufreq_dev->cur_freq > dsufreq_dev->next_freq) {
		dsufreq_dev->scaling_direction = DSUFREQ_SCALING_DOWN;
		_set_dsu_clk_only(dsufreq_dev->dev, dsufreq_dev->clk,
			dsufreq_dev->next_freq);
		dsufreq_dev->prev_freq = dsufreq_dev->cur_freq;
		dsufreq_dev->cur_freq = dsufreq_dev->next_freq;
	} else {
		dsufreq_dev->scaling_direction = DSUFREQ_SCALING_UP;
	}

	return 0;
}

static int set_dsufreq_scaling_up(struct cpufreq_policy *policy,
	unsigned long freq, int set_opp_fail)
{
	struct sunxi_dsufreq_dev *dsufreq_dev = dsufreq_dev_temp;

	if ((dsufreq_dev->scaling_direction == DSUFREQ_SCALING_DOWN)
		|| (dsufreq_dev->scaling_direction == DSUFREQ_SCALING_EXTEND)) {
		if (set_opp_fail) {
			_set_dsu_clk_only(dsufreq_dev->dev, dsufreq_dev->clk,
				dsufreq_dev->prev_freq);
			dsufreq_dev->cur_freq = dsufreq_dev->prev_freq;
		}
	} else if (dsufreq_dev->scaling_direction == DSUFREQ_SCALING_UP) {
		_set_dsu_clk_only(dsufreq_dev->dev, dsufreq_dev->clk,
			dsufreq_dev->next_freq);
		dsufreq_dev->prev_freq = dsufreq_dev->cur_freq;
		dsufreq_dev->cur_freq = dsufreq_dev->next_freq;
	} else {

	}

	if (!set_opp_fail) {
		if (policy == dsufreq_dev->policy[0])
			dsufreq_dev->little_core_cur_freq = freq;
		else
			dsufreq_dev->big_core_cur_freq = freq;
	}

	return 0;
}

static void sunxi_dsu_nvmem(char *name)
{
	u32 index = 0x0100;

	index = sunxi_get_vf_index();
	snprintf(name, MAX_NAME_LEN, "vf%04x", index);
	sunxi_warn(NULL, "dsu dvfs: %s\n", name);
}

static int dsu_init_freq_table(struct sunxi_dsufreq_dev *dsufreq_dev)
{
	struct device *dev = dsufreq_dev->dev;
	int i, ret;
	int idx;
	unsigned long min_volt = 0;
	unsigned long freq = 0;
	char *opp_table_name = dsufreq_dev->opp_table_name;
	struct opp_table *opp_table = NULL;
	struct opp_table *reg_opp_table = NULL;
	struct dev_pm_opp *opp = NULL;
	struct cpufreq_policy *policy = dsufreq_dev->policy[0];
	unsigned long min_freq = policy->cpuinfo.min_freq * 1000;
	unsigned long l_min_volt = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	reg_opp_table = dev_pm_opp_set_regulators(dev, (const char *[]){ "dsu" }, 1);
	if (IS_ERR(reg_opp_table)) {
		ret = PTR_ERR(reg_opp_table);
	} else {
		ret = 0;
		dsufreq_dev->reg_opp_table = reg_opp_table;
	}
#else
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	ret = devm_pm_opp_set_regulators(dev, (const char *[]){ "dsu" }, 1);
#else
	ret = devm_pm_opp_set_regulators(dev, (const char *[]){ "dsu" });
#endif
#endif
	if (ret) {
		sunxi_err(dev, "failed to set OPP regulator\n");
		return ret;
	}

	sunxi_dsu_nvmem(opp_table_name);

	if (strlen(opp_table_name)) {
		opp_table = dev_pm_opp_set_prop_name(dev, opp_table_name);
		if (IS_ERR(opp_table)) {
			ret = PTR_ERR(opp_table);
			sunxi_err(dev, "Failed to set prop name, use default vf\n");
			goto reg_put;
		}
		dsufreq_dev->opp_table = opp_table;
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	ret = dev_pm_opp_of_add_table(dev);
#else
	ret = devm_pm_opp_of_add_table(dev);
#endif
	if (ret < 0) {
		sunxi_err(dev, "Failed to get OPP table\n");
		goto opp_put;
	}

	dsufreq_dev->opp_count = dev_pm_opp_get_opp_count(dev);
	dsufreq_dev->opp = devm_kmalloc_array(dev, dsufreq_dev->opp_count,
		sizeof(struct dsu_opp_table), GFP_KERNEL);
	if (!dsufreq_dev->opp) {
		ret = -ENOMEM;
		goto table_put;
	}

	idx = dsufreq_dev->opp_count - 1;
	for (i = 0, freq = ULONG_MAX; i < dsufreq_dev->opp_count; i++, freq--) {
		opp = dev_pm_opp_find_freq_floor(dev, &freq);
		if (IS_ERR(opp)) {
			ret = PTR_ERR(opp);
			goto table_put;
		}

		dsufreq_dev->opp[idx - i].freq_hz = freq;
		dsufreq_dev->opp[idx - i].volt_uv = dev_pm_opp_get_voltage(opp);
		dev_pm_opp_put(opp);
	}

	min_volt = dsufreq_dev->opp[0].volt_uv;
	opp = devfreq_recommended_opp(get_cpu_device(policy->cpu), &min_freq,
		DEVFREQ_FLAG_LEAST_UPPER_BOUND);
	if (IS_ERR(opp)) {
		sunxi_err(NULL, "Failed to recommended min opp: %lu\n", min_freq);
		ret = PTR_ERR(opp);
		goto table_put;
	}

	l_min_volt = dev_pm_opp_get_voltage(opp);
	if (min_volt > l_min_volt) {
		sunxi_err(NULL, "dsu min volt is err: %lu uV, %lu uV\n",
			min_volt, l_min_volt);
		BUG_ON(1);
	}

	dsufreq_dev->max_f_by_min_v = dsufreq_dev->opp[0].freq_hz;
	for (i = 1; i < dsufreq_dev->opp_count; i++) {
		if (min_volt != dsufreq_dev->opp[i].volt_uv) {
			dsufreq_dev->max_f_by_min_v = dsufreq_dev->opp[i-1].freq_hz;
			break;
		}
	}

	return 0;

table_put:
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	dev_pm_opp_of_remove_table(dev);
#endif

opp_put:
	if (strlen(opp_table_name))
		dev_pm_opp_put_prop_name(opp_table);

reg_put:
	if (reg_opp_table)
		dev_pm_opp_put_regulators(reg_opp_table);

	return ret;
}

static int sunxi_dsufreq_probe(struct platform_device *pdev)
{
	struct sunxi_dsufreq_dev *dsufreq_dev = NULL;
	struct device *dev = &pdev->dev;
	int cpu = 0;
	int err = 0;
	struct cpufreq_policy *policy = NULL;
	struct cpufreq_policy *policy_bak = policy;
#ifdef CONFIG_AW_SUNXI_DSUFREQ_ADJUST
	struct freq_qos_request qos_req;
#endif
	dsufreq_dev = devm_kzalloc(dev, sizeof(*dsufreq_dev), GFP_KERNEL);
	if (!dsufreq_dev)
		return -ENOMEM;

	for_each_possible_cpu(cpu) {
		policy = cpufreq_cpu_get(cpu);
		if (!policy) {
			sunxi_err(dev, "cpufreq cpu get failed\n");
			return -EPROBE_DEFER;
		}
		cpufreq_cpu_put(policy);

		if (policy != policy_bak) {
			policy_bak = policy;
			dsufreq_dev->policy[dsufreq_dev->policy_cnt] = policy;
			++dsufreq_dev->policy_cnt;
			if (dsufreq_dev->policy_cnt > POLICY_NUM) {
				sunxi_warn(dev, "not support policy num\n");
				return -EINVAL;
			}
		}
	}

	dsufreq_dev->dev = dev;
	dsufreq_dev->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(dsufreq_dev->clk)) {
		sunxi_err(dev, "Fail to get clock\n");
		err = PTR_ERR(dsufreq_dev->clk);
		return err;
	}

	err = dsu_init_freq_table(dsufreq_dev);
	if (err) {
		sunxi_err(dev, "Fail init freq table\n");
		return err;
	}

	clus_ctrl_base = of_iomap(dev->of_node, 0);
	if (IS_ERR(clus_ctrl_base)) {
		sunxi_err(dev, "of_iomap error!\n");
		err = PTR_ERR(clus_ctrl_base);
		return err;
	}

	dsufreq_dev->cur_freq = clk_get_rate(dsufreq_dev->clk);
	platform_set_drvdata(pdev, dsufreq_dev);
	dsufreq_dev_temp = dsufreq_dev;
	set_dsufreq_cb(set_dsufreq_scaling_down, set_dsufreq_scaling_up);

	err = class_register(&dsufreq_class);
	if (err) {
		sunxi_err(NULL, "failed to dsufreq class register, err = %d\n", err);
		return err;
	}

#ifdef CONFIG_AW_SUNXI_DSUFREQ_ADJUST
	policy = cpufreq_cpu_get(CPUL_CORE_NUM);
	if (!policy) {
		sunxi_warn(NULL, "Cpufreq policy not found cpu0\n");
		err = -EPROBE_DEFER;
		return err;
	}
	memset(&qos_req, 0, sizeof(qos_req));
	err = freq_qos_add_request(&policy->constraints, &qos_req, FREQ_QOS_MAX,
				   policy->cpuinfo.max_freq);
	if (err < 0) {
		sunxi_err(NULL, "Failed to add freq constraint %d\n", err);
		cpufreq_cpu_put(policy);
		return err;
	}
	cpufreq_cpu_put(policy);
	freq_qos_update_request(&qos_req, FREQUENCY_SCALING_TRIGGER_MIN);
	freq_qos_update_request(&qos_req, INT_MAX);
	freq_qos_remove_request(&qos_req);
#endif

	return 0;
}

static int sunxi_dsufreq_remove(struct platform_device *pdev)
{
	struct sunxi_dsufreq_dev *dsufreq_dev = platform_get_drvdata(pdev);
	struct device __maybe_unused *dev = &pdev->dev;

	class_unregister(&dsufreq_class);
	set_dsufreq_cb(NULL, NULL);

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 13, 0)
	dev_pm_opp_of_remove_table(dev);
#endif

	if (strlen(dsufreq_dev->opp_table_name))
		dev_pm_opp_put_prop_name(dsufreq_dev->opp_table);

	if (dsufreq_dev->reg_opp_table)
		dev_pm_opp_put_regulators(dsufreq_dev->reg_opp_table);

	clk_disable_unprepare(dsufreq_dev->clk);
	iounmap((char __iomem *)clus_ctrl_base);

	return 0;
}

static const struct of_device_id sunxi_dsufreq_of_match[] = {
	{ .compatible = "allwinner,sun55iw3-dsufreq", },
	{ .compatible = "allwinner,dsufreq", },
	{ },
};
MODULE_DEVICE_TABLE(of, sunxi_dsufreq_of_match);

static struct platform_driver sunxi_dsufreq_driver = {
	.probe = sunxi_dsufreq_probe,
	.remove	= sunxi_dsufreq_remove,
	.driver = {
		.name = "sunxi-dsufreq",
		.of_match_table = sunxi_dsufreq_of_match,
	},
};
module_platform_driver(sunxi_dsufreq_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Allwinner dsufreq driver");
MODULE_ALIAS("platform:sunxi_dsufreq");
MODULE_AUTHOR("panzhijian <panzhijian@allwinnertech.com>");
MODULE_VERSION("1.0.1");

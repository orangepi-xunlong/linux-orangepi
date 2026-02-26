// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Freescale Semiconductor, Inc.
 *
 * Copyright (C) 2014 Linaro.
 * Viresh Kumar <viresh.kumar@linaro.org>
 */

#define pr_fmt(fmt)	KBUILD_MODNAME ": " fmt

#include <sunxi-log.h>
#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/pm_opp.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/time.h>

#include "cpufreq-dt.h"

struct private_data {
	struct list_head node;

	cpumask_var_t cpus;
	struct device *cpu_dev;
	struct opp_table *opp_table;
	struct opp_table *reg_opp_table;
	bool have_static_opps;
};

static LIST_HEAD(priv_list);

static struct freq_attr *cpufreq_dt_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,   /* Extra space for boost-attr if required */
	NULL,
};

#define CPUFREQ_CDEV_NUM 2

struct cdev_info {
	unsigned int           uevent_suppress:1;
	unsigned int           reserve:7;
	struct cpufreq_policy  *policy;
};

struct cpufreq_cdev_info {
	unsigned int     suspend_policy_cnt;
	unsigned int     resume_policy_cnt;
	struct cdev_info uevent_info[CPUFREQ_CDEV_NUM];
};

static struct cpufreq_cdev_info cdev_info = {
	.suspend_policy_cnt = 0,
	.resume_policy_cnt = 0,
	.uevent_info = {
		{
			.uevent_suppress = 0,
			.reserve = 0,
			.policy = NULL,
		},
		{
			.uevent_suppress = 0,
			.reserve = 0,
			.policy = NULL,
		},
	},
};

#if IS_ENABLED(CONFIG_AW_SUNXI_DSUFREQ)
static DEFINE_MUTEX(set_dsufreq_mutex);

struct dsufreq_scaling_info {
	int (*scaling_down_cb)(struct cpufreq_policy *policy, unsigned long freq);
	int (*scaling_up_cb)(struct cpufreq_policy *policy, unsigned long freq, int set_opp_fail);
};

static struct dsufreq_scaling_info dsufreq_scaling = {
	.scaling_down_cb = NULL,
	.scaling_up_cb   = NULL,
};

void set_dsufreq_cb(int (*scaling_down_cb)(struct cpufreq_policy *, unsigned long),
	int (*scaling_up_cb)(struct cpufreq_policy *, unsigned long, int))
{
	mutex_lock(&set_dsufreq_mutex);
	dsufreq_scaling.scaling_down_cb = scaling_down_cb;
	dsufreq_scaling.scaling_up_cb = scaling_up_cb;
	mutex_unlock(&set_dsufreq_mutex);
}
EXPORT_SYMBOL_GPL(set_dsufreq_cb);
#endif /* #if IS_ENABLD(CONFIG_AW_SUNXI_DSUFREQ) */

static struct private_data *cpufreq_dt_find_data(int cpu)
{
	struct private_data *priv;

	list_for_each_entry(priv, &priv_list, node) {
		if (cpumask_test_cpu(cpu, priv->cpus))
			return priv;
	}

	return NULL;
}

#if IS_ENABLED(CONFIG_AW_SUNXI_DSUFREQ)
static int set_target(struct cpufreq_policy *policy, unsigned int index)
{
	struct private_data *priv = policy->driver_data;
	unsigned long freq = policy->freq_table[index].frequency;
	u64 start_time;
	u64 end_time;
	int ret = 0;

	mutex_lock(&set_dsufreq_mutex);

	if (dsufreq_scaling.scaling_down_cb)
		dsufreq_scaling.scaling_down_cb(policy, freq * 1000);

	start_time = ktime_get();
	ret = dev_pm_opp_set_rate(priv->cpu_dev, freq * 1000);
	end_time = ktime_get();
	//sunxi_get_freq_info(get_cpu_device(policy->cpu), policy, start_time, end_time, freq * 1000, policy->cur * 1000);

	if (dsufreq_scaling.scaling_up_cb)
		dsufreq_scaling.scaling_up_cb(policy, freq * 1000, ret);

	mutex_unlock(&set_dsufreq_mutex);

	return ret;
}
#else /* #if IS_ENABLD(CONFIG_AW_SUNXI_DSUFREQ) */
static int set_target(struct cpufreq_policy *policy, unsigned int index)
{
	int ret = 0;
	struct private_data *priv = policy->driver_data;
	unsigned long freq = policy->freq_table[index].frequency;
	u64 start_time;
	u64 end_time;

	start_time = ktime_get();
	ret = dev_pm_opp_set_rate(priv->cpu_dev, freq * 1000);
	end_time = ktime_get();
	//sunxi_get_freq_info(get_cpu_device(policy->cpu), policy, start_time, end_time, freq * 1000, policy->cur * 1000);

	return ret;
}
#endif /* #if IS_ENABLED(CONFIG_AW_SUNXI_DSUFREQ) */

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

	sunxi_debug(dev, "no regulator for cpu%d\n", cpu);
node_put:
	of_node_put(np);
	return name;
}

static int cpufreq_init(struct cpufreq_policy *policy)
{
	struct cpufreq_frequency_table *freq_table;
	struct private_data *priv;
	struct device *cpu_dev;
	struct clk *cpu_clk;
	unsigned int transition_latency;
	int ret;

	priv = cpufreq_dt_find_data(policy->cpu);
	if (!priv) {
		sunxi_err(NULL, "failed to find data for cpu%d\n", policy->cpu);
		return -ENODEV;
	}

	cpu_dev = priv->cpu_dev;
	cpumask_copy(policy->cpus, priv->cpus);

	cpu_clk = clk_get(cpu_dev, NULL);
	if (IS_ERR(cpu_clk)) {
		ret = PTR_ERR(cpu_clk);
		sunxi_err(cpu_dev, "%s: failed to get clk: %d\n", __func__, ret);
		return ret;
	}

	/*
	 * Initialize OPP tables for all policy->cpus. They will be shared by
	 * all CPUs which have marked their CPUs shared with OPP bindings.
	 *
	 * For platforms not using operating-points-v2 bindings, we do this
	 * before updating policy->cpus. Otherwise, we will end up creating
	 * duplicate OPPs for policy->cpus.
	 *
	 * OPPs might be populated at runtime, don't check for error here
	 */
	if (!dev_pm_opp_of_cpumask_add_table(policy->cpus))
		priv->have_static_opps = true;

	/*
	 * But we need OPP table to function so if it is not there let's
	 * give platform code chance to provide it for us.
	 */
	ret = dev_pm_opp_get_opp_count(cpu_dev);
	if (ret <= 0) {
		sunxi_err(cpu_dev, "OPP table can't be empty\n");
		ret = -ENODEV;
		goto out_free_opp;
	}

	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &freq_table);
	if (ret) {
		sunxi_err(cpu_dev, "failed to init cpufreq table: %d\n", ret);
		goto out_free_opp;
	}

	policy->driver_data = priv;
	policy->clk = cpu_clk;
	policy->freq_table = freq_table;

	policy->suspend_freq = dev_pm_opp_get_suspend_opp_freq(cpu_dev) / 1000;

	/* Support turbo/boost mode */
	if (policy_has_boost_freq(policy)) {
		/* This gets disabled by core on driver unregister */
		ret = cpufreq_enable_boost_support();
		if (ret)
			goto out_free_cpufreq_table;
		cpufreq_dt_attr[1] = &cpufreq_freq_attr_scaling_boost_freqs;
	}

	transition_latency = dev_pm_opp_get_max_transition_latency(cpu_dev);
	if (!transition_latency)
		transition_latency = CPUFREQ_ETERNAL;

	policy->cpuinfo.transition_latency = transition_latency;
	policy->dvfs_possible_from_any_cpu = true;

	dev_pm_opp_of_register_em(cpu_dev, policy->cpus);

	return 0;

out_free_cpufreq_table:
	dev_pm_opp_free_cpufreq_table(cpu_dev, &freq_table);
out_free_opp:
	if (priv->have_static_opps)
		dev_pm_opp_of_cpumask_remove_table(policy->cpus);
	clk_put(cpu_clk);

	return ret;
}

static int cpufreq_online(struct cpufreq_policy *policy)
{
	/* We did light-weight tear down earlier, nothing to do here */
	return 0;
}

static int cpufreq_offline(struct cpufreq_policy *policy)
{
	/*
	 * Preserve policy->driver_data and don't free resources on light-weight
	 * tear down.
	 */
	return 0;
}

static int cpufreq_exit(struct cpufreq_policy *policy)
{
	struct private_data *priv = policy->driver_data;

	dev_pm_opp_free_cpufreq_table(priv->cpu_dev, &policy->freq_table);
	if (priv->have_static_opps)
		dev_pm_opp_of_cpumask_remove_table(policy->related_cpus);
	clk_put(policy->clk);
	return 0;
}

static int sunxi_cpufreq_suspend(struct cpufreq_policy *policy)
{
	struct device *dev = NULL;
	struct cpufreq_cdev_info *info = &cdev_info;
	unsigned int suspend_policy_cnt = info->suspend_policy_cnt;

	cpufreq_generic_suspend(policy);

	if (!policy->cdev)
		return 0;

	dev = &(policy->cdev->device);
	++suspend_policy_cnt;
	if (suspend_policy_cnt > CPUFREQ_CDEV_NUM) {
		sunxi_err(NULL, "%s, unsupport policy num!\n", __func__);
		return -EINVAL;
	} else {
		info->uevent_info[suspend_policy_cnt - 1].uevent_suppress =
			dev_get_uevent_suppress(dev);
		info->uevent_info[suspend_policy_cnt - 1].policy = policy;
	}

	info->suspend_policy_cnt = suspend_policy_cnt;
	dev_set_uevent_suppress(dev, true);

	return 0;
}

static int sunxi_cpufreq_resume(struct cpufreq_policy *policy)
{
	struct device *dev = NULL;
	unsigned int i = 0;
	struct cpufreq_cdev_info *info = &cdev_info;
	unsigned int suspend_policy_cnt = info->suspend_policy_cnt;

	if (!policy->cdev)
		return 0;

	dev = &(policy->cdev->device);

	for (i = 0; i < suspend_policy_cnt; i++) {
		if (policy == info->uevent_info[i].policy) {
			dev_set_uevent_suppress(dev,
				info->uevent_info[i].uevent_suppress);
			break;
		}
	}

	if (i >= suspend_policy_cnt) {
		sunxi_err(NULL, "%s, policy err!\n", __func__);
		return -EINVAL;
	}

	++info->resume_policy_cnt;
	if (suspend_policy_cnt == info->resume_policy_cnt)
		memset(info, 0x00, sizeof(*info));

	return 0;
}

static struct cpufreq_driver dt_cpufreq_driver = {
	.flags = CPUFREQ_STICKY | CPUFREQ_NEED_INITIAL_FREQ_CHECK |
		 CPUFREQ_IS_COOLING_DEV,
	.verify = cpufreq_generic_frequency_table_verify,
	.target_index = set_target,
	.get = cpufreq_generic_get,
	.init = cpufreq_init,
	.exit = cpufreq_exit,
	.online = cpufreq_online,
	.offline = cpufreq_offline,
	.name = "cpufreq-dt",
	.attr = cpufreq_dt_attr,
	.suspend = sunxi_cpufreq_suspend,
	.resume = sunxi_cpufreq_resume,
};

static int dt_cpufreq_early_init(struct device *dev, int cpu)
{
	struct private_data *priv;
	struct device *cpu_dev;
	const char *reg_name;
	int ret;

	/* Check if this CPU is already covered by some other policy */
	if (cpufreq_dt_find_data(cpu))
		return 0;

	cpu_dev = get_cpu_device(cpu);
	if (!cpu_dev)
		return -EPROBE_DEFER;

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	if (!alloc_cpumask_var(&priv->cpus, GFP_KERNEL))
		return -ENOMEM;

	priv->cpu_dev = cpu_dev;

	/* Try to get OPP table early to ensure resources are available */
	priv->opp_table = dev_pm_opp_get_opp_table(cpu_dev);
	if (IS_ERR(priv->opp_table)) {
		ret = PTR_ERR(priv->opp_table);
		if (ret != -EPROBE_DEFER)
			sunxi_err(cpu_dev, "failed to get OPP table: %d\n", ret);
		goto free_cpumask;
	}

	/*
	 * OPP layer will be taking care of regulators now, but it needs to know
	 * the name of the regulator first.
	 */
	reg_name = find_supply_name(cpu_dev);
	if (reg_name) {
		priv->reg_opp_table = dev_pm_opp_set_regulators(cpu_dev,
								&reg_name, 1);
		if (IS_ERR(priv->reg_opp_table)) {
			ret = PTR_ERR(priv->reg_opp_table);
			if (ret != -EPROBE_DEFER)
				sunxi_err(cpu_dev, "failed to set regulators: %d\n",
					ret);
			goto put_table;
		}
	}

	/* Find OPP sharing information so we can fill pri->cpus here */
	/* Get OPP-sharing information from "operating-points-v2" bindings */
	ret = dev_pm_opp_of_get_sharing_cpus(cpu_dev, priv->cpus);
	if (ret) {
		if (ret != -ENOENT)
			goto put_reg;

		/*
		 * operating-points-v2 not supported, fallback to all CPUs share
		 * OPP for backward compatibility if the platform hasn't set
		 * sharing CPUs.
		 */
		if (dev_pm_opp_get_sharing_cpus(cpu_dev, priv->cpus)) {
			cpumask_setall(priv->cpus);

			/*
			 * OPP tables are initialized only for cpu, do it for
			 * others as well.
			 */
			ret = dev_pm_opp_set_sharing_cpus(cpu_dev, priv->cpus);
			if (ret)
				sunxi_err(cpu_dev, "%s: failed to mark OPPs as shared: %d\n",
					__func__, ret);
		}
	}

	list_add(&priv->node, &priv_list);
	return 0;

put_reg:
	if (priv->reg_opp_table)
		dev_pm_opp_put_regulators(priv->reg_opp_table);
put_table:
	dev_pm_opp_put_opp_table(priv->opp_table);
free_cpumask:
	free_cpumask_var(priv->cpus);
	return ret;
}

static void dt_cpufreq_release(void)
{
	struct private_data *priv, *tmp;

	list_for_each_entry_safe(priv, tmp, &priv_list, node) {
		if (priv->reg_opp_table)
			dev_pm_opp_put_regulators(priv->reg_opp_table);
		dev_pm_opp_put_opp_table(priv->opp_table);
		free_cpumask_var(priv->cpus);
		list_del(&priv->node);
	}
}

static int dt_cpufreq_probe(struct platform_device *pdev)
{
	struct cpufreq_dt_platform_data *data = dev_get_platdata(&pdev->dev);
	int ret, cpu;

	/* Request resources early so we can return in case of -EPROBE_DEFER */
	for_each_possible_cpu(cpu) {
		ret = dt_cpufreq_early_init(&pdev->dev, cpu);
		if (ret)
			goto err;
	}

	if (data) {
		if (data->have_governor_per_policy)
			dt_cpufreq_driver.flags |= CPUFREQ_HAVE_GOVERNOR_PER_POLICY;

		dt_cpufreq_driver.resume = data->resume;
		if (data->suspend)
			dt_cpufreq_driver.suspend = data->suspend;
		if (data->get_intermediate) {
			dt_cpufreq_driver.target_intermediate = data->target_intermediate;
			dt_cpufreq_driver.get_intermediate = data->get_intermediate;
		}
	}

	ret = cpufreq_register_driver(&dt_cpufreq_driver);
	if (ret) {
		sunxi_err(&pdev->dev, "failed register driver: %d\n", ret);
		goto err;
	}

	return 0;
err:
	dt_cpufreq_release();
	return ret;
}

static int dt_cpufreq_remove(struct platform_device *pdev)
{
	cpufreq_unregister_driver(&dt_cpufreq_driver);
	dt_cpufreq_release();
	return 0;
}

static struct platform_driver dt_cpufreq_platdrv = {
	.driver = {
		.name	= "cpufreq-dt",
	},
	.probe		= dt_cpufreq_probe,
	.remove		= dt_cpufreq_remove,
};
module_platform_driver(dt_cpufreq_platdrv);

MODULE_ALIAS("platform:cpufreq-dt");
MODULE_AUTHOR("Viresh Kumar <viresh.kumar@linaro.org>");
MODULE_AUTHOR("Shawn Guo <shawn.guo@linaro.org>");
MODULE_DESCRIPTION("Generic cpufreq driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

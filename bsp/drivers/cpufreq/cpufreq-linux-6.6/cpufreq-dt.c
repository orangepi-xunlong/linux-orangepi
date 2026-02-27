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
#include "../../../drivers/opp/opp.h"
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/time.h>
#include "../../crashdump/sunxi-crashdump.h"
#include "cpufreq-dt.h"

struct private_data {
	struct list_head node;

	cpumask_var_t cpus;
	struct device *cpu_dev;
	struct cpufreq_frequency_table *freq_table;
	bool have_static_opps;
	int opp_token;
};

static LIST_HEAD(priv_list);

static struct freq_attr *cpufreq_dt_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,   /* Extra space for boost-attr if required */
	NULL,
};

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
	sunxi_get_freq_info(get_cpu_device(policy->cpu), policy, start_time, end_time, freq * 1000, policy->cur * 1000);

	if (dsufreq_scaling.scaling_up_cb)
		dsufreq_scaling.scaling_up_cb(policy, freq * 1000, ret);

	mutex_unlock(&set_dsufreq_mutex);

	return ret;
}
#else /* #if IS_ENABLD(CONFIG_AW_SUNXI_DSUFREQ) */
static int set_target(struct cpufreq_policy *policy, unsigned int index)
{
	struct private_data *priv = policy->driver_data;
	unsigned long freq = policy->freq_table[index].frequency;

	return dev_pm_opp_set_rate(priv->cpu_dev, freq * 1000);
}
#endif

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

	cpu_clk = clk_get(cpu_dev, NULL);
	if (IS_ERR(cpu_clk)) {
		ret = PTR_ERR(cpu_clk);
		sunxi_err(cpu_dev, "%s: failed to get clk: %d\n", __func__, ret);
		return ret;
	}

	transition_latency = dev_pm_opp_get_max_transition_latency(cpu_dev);
	if (!transition_latency)
		transition_latency = CPUFREQ_ETERNAL;

	cpumask_copy(policy->cpus, priv->cpus);
	policy->driver_data = priv;
	policy->clk = cpu_clk;
	policy->freq_table = priv->freq_table;
	policy->suspend_freq = dev_pm_opp_get_suspend_opp_freq(cpu_dev) / 1000;
	policy->cpuinfo.transition_latency = transition_latency;
	policy->dvfs_possible_from_any_cpu = true;

	/* Support turbo/boost mode */
	if (policy_has_boost_freq(policy)) {
		/* This gets disabled by core on driver unregister */
		ret = cpufreq_enable_boost_support();
		if (ret)
			goto out_clk_put;
		cpufreq_dt_attr[1] = &cpufreq_freq_attr_scaling_boost_freqs;
	}

	return 0;

out_clk_put:
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
	clk_put(policy->clk);
	return 0;
}

static int __maybe_unused sunxi_get_power(struct device *dev, unsigned long *uW,
				     unsigned long *kHz)
{
	struct dev_pm_opp *opp, *iter;
	struct device_node *np;
	struct opp_table *opp_table;
	unsigned long mV, Hz;
	u32 cap;
	u64 tmp;
	int ret;
	int index = 0;

	np = of_node_get(dev->of_node);
	if (!np)
		return -EINVAL;

	ret = of_property_read_u32(np, "dynamic-power-coefficient", &cap);
	of_node_put(np);
	if (ret)
		return -EINVAL;

	Hz = *kHz * 1000;
	opp = dev_pm_opp_find_freq_ceil(dev, &Hz);
	if (IS_ERR(opp))
		return -EINVAL;

	mV = dev_pm_opp_get_voltage(opp) / 1000;

	opp_table = opp->opp_table;
	list_for_each_entry(iter, &opp_table->opp_list, node) {
		index++;
		if (iter->rates[0] >= Hz)
			break;
	}
	dev_pm_opp_put(opp);
	if (!mV)
		return -EINVAL;

	tmp = (u64)cap * mV * mV * (Hz / 1000000);
	/* Provide power in micro-Watts */
	do_div(tmp, 1000000);

	// avoid inefficient opp when several opp is in same voltage
	*uW = (unsigned long)tmp + ((index * Hz / 1000000) / 4);
	*kHz = Hz / 1000;

	return 0;
}

static void sunxi_cpufreq_register_em(struct cpufreq_policy *policy)
{
	struct device *dev = get_cpu_device(policy->cpu);
	struct cpumask *cpus = policy->related_cpus;
	struct em_data_callback em_cb;
	struct device_node *np;
	int ret, nr_opp;
	u32 cap;

	if (IS_ERR_OR_NULL(dev)) {
		ret = -EINVAL;
		goto failed;
	}

	nr_opp = dev_pm_opp_get_opp_count(dev);
	if (nr_opp <= 0) {
		ret = -EINVAL;
		goto failed;
	}

	np = of_node_get(dev->of_node);
	if (!np) {
		ret = -EINVAL;
		goto failed;
	}

	/*
	 * Register an EM only if the 'dynamic-power-coefficient' property is
	 * set in devicetree. It is assumed the voltage values are known if that
	 * property is set since it is useless otherwise. If voltages are not
	 * known, just let the EM registration fail with an error to alert the
	 * user about the inconsistent configuration.
	 */
	ret = of_property_read_u32(np, "dynamic-power-coefficient", &cap);
	of_node_put(np);
	if (ret || !cap) {
		dev_dbg(dev, "Couldn't find proper 'dynamic-power-coefficient' in DT\n");
		ret = -EINVAL;
		goto failed;
	}

	EM_SET_ACTIVE_POWER_CB(em_cb, sunxi_get_power);

	ret = em_dev_register_perf_domain(dev, nr_opp, &em_cb, cpus, true);
	if (ret)
		goto failed;

failed:
	dev_dbg(dev, "Couldn't register Energy Model %d\n", ret);
}

static struct cpufreq_driver dt_cpufreq_driver = {
	.flags = CPUFREQ_NEED_INITIAL_FREQ_CHECK |
		 CPUFREQ_IS_COOLING_DEV,
	.verify = cpufreq_generic_frequency_table_verify,
	.target_index = set_target,
	.get = cpufreq_generic_get,
	.init = cpufreq_init,
	.exit = cpufreq_exit,
	.online = cpufreq_online,
	.offline = cpufreq_offline,
	.register_em = sunxi_cpufreq_register_em,
	.name = "cpufreq-dt",
	.attr = cpufreq_dt_attr,
	.suspend = cpufreq_generic_suspend,
};

static int dt_cpufreq_early_init(struct device *dev, int cpu)
{
	struct private_data *priv;
	struct device *cpu_dev;
	bool fallback = false;
	const char *reg_name[] = { NULL, NULL };
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

	cpumask_set_cpu(cpu, priv->cpus);
	priv->cpu_dev = cpu_dev;

	/*
	 * OPP layer will be taking care of regulators now, but it needs to know
	 * the name of the regulator first.
	 */
	reg_name[0] = find_supply_name(cpu_dev);
	if (reg_name[0]) {
		priv->opp_token = dev_pm_opp_set_regulators(cpu_dev, reg_name);
		if (priv->opp_token < 0) {
			ret = dev_err_probe(cpu_dev, priv->opp_token,
					    "failed to set regulators\n");
			goto free_cpumask;
		}
	}

	/* Get OPP-sharing information from "operating-points-v2" bindings */
	ret = dev_pm_opp_of_get_sharing_cpus(cpu_dev, priv->cpus);
	if (ret) {
		if (ret != -ENOENT)
			goto out;

		/*
		 * operating-points-v2 not supported, fallback to all CPUs share
		 * OPP for backward compatibility if the platform hasn't set
		 * sharing CPUs.
		 */
		if (dev_pm_opp_get_sharing_cpus(cpu_dev, priv->cpus))
			fallback = true;
	}

	/*
	 * Initialize OPP tables for all priv->cpus. They will be shared by
	 * all CPUs which have marked their CPUs shared with OPP bindings.
	 *
	 * For platforms not using operating-points-v2 bindings, we do this
	 * before updating priv->cpus. Otherwise, we will end up creating
	 * duplicate OPPs for the CPUs.
	 *
	 * OPPs might be populated at runtime, don't fail for error here unless
	 * it is -EPROBE_DEFER.
	 */
	ret = dev_pm_opp_of_cpumask_add_table(priv->cpus);
	if (!ret) {
		priv->have_static_opps = true;
	} else if (ret == -EPROBE_DEFER) {
		goto out;
	}

	/*
	 * The OPP table must be initialized, statically or dynamically, by this
	 * point.
	 */
	ret = dev_pm_opp_get_opp_count(cpu_dev);
	if (ret <= 0) {
		sunxi_err(cpu_dev, "OPP table can't be empty\n");
		ret = -ENODEV;
		goto out;
	}

	if (fallback) {
		cpumask_setall(priv->cpus);
		ret = dev_pm_opp_set_sharing_cpus(cpu_dev, priv->cpus);
		if (ret)
			sunxi_err(cpu_dev, "%s: failed to mark OPPs as shared: %d\n",
				__func__, ret);
	}

	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &priv->freq_table);
	if (ret) {
		sunxi_err(cpu_dev, "failed to init cpufreq table: %d\n", ret);
		goto out;
	}

	list_add(&priv->node, &priv_list);
	return 0;

out:
	if (priv->have_static_opps)
		dev_pm_opp_of_cpumask_remove_table(priv->cpus);
	dev_pm_opp_put_regulators(priv->opp_token);
free_cpumask:
	free_cpumask_var(priv->cpus);
	return ret;
}

static void dt_cpufreq_release(void)
{
	struct private_data *priv, *tmp;

	list_for_each_entry_safe(priv, tmp, &priv_list, node) {
		dev_pm_opp_free_cpufreq_table(priv->cpu_dev, &priv->freq_table);
		if (priv->have_static_opps)
			dev_pm_opp_of_cpumask_remove_table(priv->cpus);
		dev_pm_opp_put_regulators(priv->opp_token);
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

static void dt_cpufreq_remove(struct platform_device *pdev)
{
	cpufreq_unregister_driver(&dt_cpufreq_driver);
	dt_cpufreq_release();
}

static struct platform_driver dt_cpufreq_platdrv = {
	.driver = {
		.name	= "cpufreq-dt",
	},
	.probe		= dt_cpufreq_probe,
	.remove_new	= dt_cpufreq_remove,
};
module_platform_driver(dt_cpufreq_platdrv);

MODULE_ALIAS("platform:cpufreq-dt");
MODULE_AUTHOR("Viresh Kumar <viresh.kumar@linaro.org>");
MODULE_AUTHOR("Shawn Guo <shawn.guo@linaro.org>");
MODULE_DESCRIPTION("Generic cpufreq driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

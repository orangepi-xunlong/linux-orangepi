// SPDX-License-Identifier: GPL-2.0-only

#include <linux/cpufreq.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpumask.h>
#include <linux/clk/clk-conf.h>
#include <linux/pm_qos.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/mutex.h>
#include <linux/pm_opp.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include "../opp/opp.h"
#include "cpufreq-dt.h"

struct private_data {
	struct list_head node;

	cpumask_var_t cpus;
	struct device *cpu_dev;
	struct cpufreq_frequency_table *freq_table;
	bool have_static_opps;
	int opp_token;
};

static u32 product_prop = 0, wafer_prop = 0;

#ifdef CONFIG_CPU_HOTPLUG_THERMAL
struct thermal_cooling_device **ghotplug_cooling;
extern struct thermal_cooling_device **
of_hotplug_cooling_register(struct cpufreq_policy *policy);
#endif

#define FREQ_TABLE_0		(0)
#define FREQ_TABLE_1		(1)
#define FREQ_TABLE_2		(2)

#ifdef CONFIG_SOC_KY_X1

#define TURBO0_FREQUENCY		(1600000000)
#define TURBO1_FREQUENCY		(3200000000)
#define STABLE_FREQUENCY		(1200000000)

#define FILTER_POINTS_0			(135)
#define FILTER_POINTS_1			(142)

#define K1_MAX_FREQ_LIMITATION		(1600000)
#define M1_MAX_FREQ_LIMITATION		(1800000)

#endif

#define PRODUCT_ID_M1			(0x36070000)

static int ky_policy_notifier(struct notifier_block *nb,
                                  unsigned long event, void *data)
{
	int cpu;
	u64 rates;
	static int cci_init;
	struct clk *cci_clk;
	struct device *cpu_dev;
	struct cpufreq_policy *policy = data;
	struct opp_table *opp_table;

	cpu = cpumask_first(policy->related_cpus);
	cpu_dev = get_cpu_device(cpu);
	opp_table = _find_opp_table(cpu_dev);

	if (cci_init == 0) {
		cci_clk = of_clk_get_by_name(opp_table->np, "cci");
		of_property_read_u64_array(opp_table->np, "cci-hz", &rates, 1);
		clk_set_rate(cci_clk, rates);
		clk_put(cci_clk);
		cci_init = 1;

#ifdef CONFIG_SOC_KY_X1
		if (policy->clk)
			clk_put(policy->clk);

		/* cover the policy->clk & opp_table->clk which has been set before */
		policy->clk = opp_table->clks[0];
		opp_table->clk = opp_table->clks[0];
#endif
	}

#ifdef CONFIG_CPU_HOTPLUG_THERMAL
       ghotplug_cooling = of_hotplug_cooling_register(policy);
       if (!ghotplug_cooling) {
               pr_err("register hotplug cpu cooling failed\n");
               return -EINVAL;
       }
#endif
	return 0;
}

static int ky_processor_notifier(struct notifier_block *nb,
                                  unsigned long event, void *data)
{
	int cpu;
	struct device *cpu_dev;
	struct cpufreq_freqs *freqs = (struct cpufreq_freqs *)data;
	struct cpufreq_policy *policy = ( struct cpufreq_policy *)freqs->policy;
	struct opp_table *opp_table;
	struct device_node *np;
	struct clk *tcm_clk, *ace0_clk, *ace1_clk, *pll_clk, *c0hi, *c1hi;
	u64 rates;
	u32 microvol;
	int i;

	cpu = cpumask_first(policy->related_cpus);
	cpu_dev = get_cpu_device(cpu);
	opp_table = _find_opp_table(cpu_dev);

	for_each_available_child_of_node(opp_table->np, np) {
		of_property_read_u64_array(np, "opp-hz", &rates, 1);
		if (rates == freqs->new * 1000) {
			of_property_read_u32(np, "opp-microvolt", &microvol);
			break;
		}
	}

	/* get the tcm/ace clk handler */
	tcm_clk = of_clk_get_by_name(opp_table->np, "tcm");
	ace0_clk = of_clk_get_by_name(opp_table->np, "ace0");
	ace1_clk = of_clk_get_by_name(opp_table->np, "ace1");
	pll_clk = of_clk_get_by_name(opp_table->np, "pll3");
	c0hi = of_clk_get_by_name(opp_table->np, "c0hi");
	c1hi = of_clk_get_by_name(opp_table->np, "c1hi");

	if (event == CPUFREQ_PRECHANGE) {
		/**
		 * change the tcm/ace's frequency first.
		 * binary division is safe
		 */
		if (!IS_ERR(ace0_clk)) {
			clk_set_rate(ace0_clk, clk_get_rate(clk_get_parent(ace0_clk)) / 2);
			clk_put(ace0_clk);
		}

		if (!IS_ERR(ace1_clk)) {
			clk_set_rate(ace1_clk, clk_get_rate(clk_get_parent(ace1_clk)) / 2);
			clk_put(ace1_clk);
		}

		if (!IS_ERR(tcm_clk)) {
			clk_set_rate(tcm_clk, clk_get_rate(clk_get_parent(tcm_clk)) / 2);
			clk_put(tcm_clk);
		}

		if (freqs->new * 1000 >= TURBO0_FREQUENCY) {
			if (freqs->old * 1000 >= TURBO0_FREQUENCY) {
				for (i = 0; i < opp_table->clk_count; ++i) {
					clk_set_rate(opp_table->clks[i], STABLE_FREQUENCY);
				}
			}

			/* change the frequency of pll3 first */
			if (freqs->new * 1000 == TURBO0_FREQUENCY) {
				/* PLL3 = 3.2G */
				clk_set_rate(pll_clk, TURBO1_FREQUENCY);
				/* HI = 1.6G */
				clk_set_rate(c0hi, TURBO0_FREQUENCY);
				clk_set_rate(c1hi, TURBO0_FREQUENCY);
			}

			if (freqs->new * 1000 > TURBO0_FREQUENCY) {
				/* PLL3 = 1.8G */
				clk_set_rate(pll_clk, freqs->new * 1000);
				/* HI = 1.8G */
				clk_set_rate(c0hi, freqs->new * 1000);
				clk_set_rate(c1hi, freqs->new * 1000);
			}
		}
	}

	if (event == CPUFREQ_POSTCHANGE) {

		if (!IS_ERR(tcm_clk)) {
			/* get the tcm-hz */
			of_property_read_u64_array(np, "tcm-hz", &rates, 1);
			/* then set rate */
			clk_set_rate(tcm_clk, rates);
			clk_put(tcm_clk);
		}

		if (!IS_ERR(ace0_clk)) {
			/* get the ace-hz */
			of_property_read_u64_array(np, "ace0-hz", &rates, 1);
			/* then set rate */
			clk_set_rate(ace0_clk, rates);
			clk_put(ace0_clk);
		}

		if (!IS_ERR(ace1_clk)) {
			/* get the ace-hz */
			of_property_read_u64_array(np, "ace1-hz", &rates, 1);
			/* then set rate */
			clk_set_rate(ace1_clk, rates);
			clk_put(ace1_clk);
		}
	}

	clk_put(pll_clk);
	clk_put(c0hi);
	clk_put(c1hi);

	dev_pm_opp_put_opp_table(opp_table);

	return 0;
}

static struct notifier_block ky_processor_notifier_block = {
       .notifier_call = ky_processor_notifier,
};

static struct notifier_block ky_policy_notifier_block = {
       .notifier_call = ky_policy_notifier,
};

static int _dev_pm_opp_of_get_sharing_cpus(struct device *cpu_dev,
				   struct cpumask *cpumask, int indexs)
{
	struct device_node *np, *tmp_np, *cpu_np;
	int cpu, ret = 0;

	/* Get OPP descriptor node */
	np = of_parse_phandle(cpu_dev->of_node, "operating-points-v2", indexs);
	if (!np) {
		dev_dbg(cpu_dev, "%s: Couldn't find opp node.\n", __func__);
		return -ENOENT;
	}

	cpumask_set_cpu(cpu_dev->id, cpumask);

	/* OPPs are shared ? */
	if (!of_property_read_bool(np, "opp-shared"))
		goto put_cpu_node;

	for_each_possible_cpu(cpu) {
		if (cpu == cpu_dev->id)
			continue;

		cpu_np = of_cpu_device_node_get(cpu);
		if (!cpu_np) {
			dev_err(cpu_dev, "%s: failed to get cpu%d node\n",
				__func__, cpu);
			ret = -ENOENT;
			goto put_cpu_node;
		}

		/* Get OPP descriptor node */
		tmp_np = of_parse_phandle(cpu_np, "operating-points-v2", indexs);
		of_node_put(cpu_np);
		if (!tmp_np) {
			pr_err("%pOF: Couldn't find opp node\n", cpu_np);
			ret = -ENOENT;
			goto put_cpu_node;
		}

		/* CPUs are sharing opp node */
		if (np == tmp_np)
			cpumask_set_cpu(cpu, cpumask);

		of_node_put(tmp_np);
	}

put_cpu_node:
	of_node_put(np);
	return ret;
}

/**
 * dev_pm_opp_of_cpumask_add_table() - Adds OPP table for @cpumask
 * @cpumask:	cpumask for which OPP table needs to be added.
 *
 * This adds the OPP tables for CPUs present in the @cpumask.
 */
static int _dev_pm_opp_of_cpumask_add_table(const struct cpumask *cpumask, int indexs)
{
	struct device *cpu_dev;
	int cpu, ret;

	if (WARN_ON(cpumask_empty(cpumask)))
		return -ENODEV;

	for_each_cpu(cpu, cpumask) {
		cpu_dev = get_cpu_device(cpu);
		if (!cpu_dev) {
			pr_err("%s: failed to get cpu%d device\n", __func__,
			       cpu);
			ret = -ENODEV;
			goto remove_table;
		}

		ret = dev_pm_opp_of_add_table_indexed(cpu_dev, indexs);
		if (ret) {
			/*
			 * OPP may get registered dynamically, don't print error
			 * message here.
			 */
			pr_debug("%s: couldn't find opp table for cpu:%d, %d\n",
				 __func__, cpu, ret);

			goto remove_table;
		}
	}

	return 0;

remove_table:
	/* Free all other OPPs */
	_dev_pm_opp_cpumask_remove_table(cpumask, cpu);

	return ret;
}

extern struct private_data *cpufreq_dt_find_data(int cpu);
extern void cpufreq_dt_add_data(struct private_data *priv);

static int ky_dt_cpufreq_pre_early_init(struct device *dev, int cpu, int indexs)
{
	struct private_data *priv;
	struct device *cpu_dev;
	const char *reg_name[] = { "clst", NULL };
	const char *clk_name[] = { "cls0", "cls1", NULL };
	struct dev_pm_opp_config config = {
		.regulator_names = reg_name,
		.clk_names = clk_name,
		.config_clks = dev_pm_opp_config_clks_simple,
	};
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
	priv->opp_token = dev_pm_opp_set_config_indexed(cpu_dev, &config, indexs);
	if (priv->opp_token < 0) {
		ret = dev_err_probe(cpu_dev, priv->opp_token,
				    "failed to set regulators\n");
		goto free_cpumask;
	}

	/* Get OPP-sharing information from "operating-points-v2" bindings */
	ret = _dev_pm_opp_of_get_sharing_cpus(cpu_dev, priv->cpus, indexs);
	if (ret)
		goto out;

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
	ret = _dev_pm_opp_of_cpumask_add_table(priv->cpus, indexs);
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
		dev_err(cpu_dev, "OPP table can't be empty\n");
		ret = -ENODEV;
		goto out;
	}

	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &priv->freq_table);
	if (ret) {
		dev_err(cpu_dev, "failed to init cpufreq table: %d\n", ret);
		goto out;
	}

	cpufreq_dt_add_data(priv);

	return 0;

out:
	if (priv->have_static_opps)
		dev_pm_opp_of_cpumask_remove_table(priv->cpus);
	dev_pm_opp_put_regulators(priv->opp_token);
free_cpumask:
	free_cpumask_var(priv->cpus);
	return ret;
}

#ifdef CONFIG_SOC_KY_X1
int spacmeit_cpufreq_veritfy(struct cpufreq_policy_data *policy)
{
	struct cpufreq_frequency_table *pos;
	unsigned int freq, next_larger = ~0;
	bool found = false;

	if (!policy->freq_table)
		return -ENODEV;

	if ((wafer_prop << 16 | product_prop) == PRODUCT_ID_M1) {
		/* M1 */
		/* can update to 1.8G */
		cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
					policy->cpuinfo.max_freq);
	} else {
		/* K1 */
		/* only 1.6G allowed max */
		policy->max = policy->max > K1_MAX_FREQ_LIMITATION ? K1_MAX_FREQ_LIMITATION : policy->max;
		cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
					K1_MAX_FREQ_LIMITATION);
	}

	cpufreq_for_each_valid_entry(pos, policy->freq_table) {
		freq = pos->frequency;

		if ((freq >= policy->min) && (freq <= policy->max)) {
			found = true;
			break;
		}

		if ((next_larger > freq) && (freq > policy->max))
			next_larger = freq;
	}

	if (!found) {
		policy->max = next_larger;
		cpufreq_verify_within_cpu_limits(policy);
	}

	pr_debug("verification lead to (%u - %u kHz) for cpu %u\n",
				policy->min, policy->max, policy->cpu);
	return 0;
}

extern void remove_boost_sysfs_file(void);
extern void remove_policy_boost_sysfs_file(struct cpufreq_policy *policy);

void ky_cpufreq_ready(struct cpufreq_policy *policy)
{
	if ((wafer_prop << 16 | product_prop) == PRODUCT_ID_M1) {
		/* M1 */
	} else {
		/* K1 or other */
		remove_policy_boost_sysfs_file(policy);
		remove_boost_sysfs_file();
	}
}

#endif

static int ky_dt_cpufreq_pre_probe(struct platform_device *pdev)
{
	int cpu, ret;
	struct device_node *cpus;
	struct device_node *product_id, *wafer_id;
	u32 prop = 0;

	if (strncmp(pdev->name, "cpufreq-dt", 10) != 0)
		return 0;

	cpus = of_find_node_by_path("/cpus");
	if (!cpus || of_property_read_u32(cpus, "svt-dro", &prop)) {
		pr_info("Ky Platform with no 'svt-dro' in DTS, using defualt frequency Table0\n");
	}

	product_id = of_find_node_by_path("/");
	if (!product_id || of_property_read_u32(product_id, "product-id", &product_prop)) {
		pr_info("Ky Platform with no 'product-id' in DTS\n");
	}

	wafer_id = of_find_node_by_path("/");
	if (!wafer_id || of_property_read_u32(product_id, "wafer-id", &wafer_prop)) {
		pr_info("Ky Platform with no 'product-id' in DTS\n");
	}

#ifdef CONFIG_SOC_KY_X1
	if ((wafer_prop << 16 | product_prop) == PRODUCT_ID_M1) {
		for_each_possible_cpu(cpu) {
			if (prop <= FILTER_POINTS_0)
				ky_dt_cpufreq_pre_early_init(&pdev->dev, cpu, FREQ_TABLE_0);
			else if (prop <= FILTER_POINTS_1)
				ret = ky_dt_cpufreq_pre_early_init(&pdev->dev, cpu, FREQ_TABLE_1);
			else
				ret = ky_dt_cpufreq_pre_early_init(&pdev->dev, cpu, FREQ_TABLE_2);
			if (ret)
				ky_dt_cpufreq_pre_early_init(&pdev->dev, cpu, FREQ_TABLE_0);
		}
	} else {
		for_each_possible_cpu(cpu) {
			ky_dt_cpufreq_pre_early_init(&pdev->dev, cpu, FREQ_TABLE_0);
		}
	}
#endif

	return 0;
}

static int __device_notifier_call(struct notifier_block *nb,
				      unsigned long event, void *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	switch (event) {
	case BUS_NOTIFY_REMOVED_DEVICE:
		break;
	case BUS_NOTIFY_UNBOUND_DRIVER:
		break;
	case BUS_NOTIFY_BIND_DRIVER:
		/* here */
		ky_dt_cpufreq_pre_probe(pdev);
		break;
	case BUS_NOTIFY_ADD_DEVICE:
		break;
	default:
		break;
	}

	return NOTIFY_DONE;
}

static struct notifier_block ky_platform_nb = {
	.notifier_call = __device_notifier_call,
};

static int __init ky_processor_driver_init(void)
{
       int ret;

       ret = cpufreq_register_notifier(&ky_processor_notifier_block, CPUFREQ_TRANSITION_NOTIFIER);
       if (ret) {
               pr_err("register cpufreq notifier failed\n");
               return -EINVAL;
       }

       ret = cpufreq_register_notifier(&ky_policy_notifier_block, CPUFREQ_POLICY_NOTIFIER);
       if (ret) {
               pr_err("register cpufreq notifier failed\n");
               return -EINVAL;
       }

	bus_register_notifier(&platform_bus_type, &ky_platform_nb);

       return 0;
}

arch_initcall(ky_processor_driver_init);

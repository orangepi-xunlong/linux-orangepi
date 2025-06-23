// SPDX-License-Identifier: GPL-2.0
// Copyright 2024 Cix Technology Group Co., Ltd.

#include <linux/bitfield.h>
#include <linux/cpufreq.h>
#include <linux/init.h>
#include <linux/interconnect.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <linux/spinlock.h>


struct cix_cpufreq_data {
	void __iomem *base;
	struct resource *res;
};

static int cix_cpufreq_read_lut(struct device *cpu_dev,
			struct cpufreq_policy *policy)
{
	int ret;

	ret = dev_pm_opp_of_add_table(cpu_dev);
	if (ret) {
		dev_err(cpu_dev, "failed to parse cpufreq table: %d\n", ret);
		return ret;
	}

	ret = dev_pm_opp_init_cpufreq_table(cpu_dev, &policy->freq_table);
	if (ret) {
		dev_err(cpu_dev, "failed to init cpufreq table: %d\n", ret);
		return ret;
	}

	return 0;
}

static unsigned int cix_cpufreq_get(unsigned int cpu)
{
	struct cpufreq_policy *policy;
	unsigned int freq;
	struct cix_cpufreq_data *data;

	policy = cpufreq_cpu_get_raw(cpu);
	if (!policy) {
		pr_err("%s: failed to get cpu%d cpufreq policy\n", __func__,
		       policy->cpu);
		return 0;
	}

	data = policy->driver_data;
	if (policy->cur)
		return policy->cur;
	else
		return policy->min;
}

static int cix_cpufreq_target_index(struct cpufreq_policy *policy,
					unsigned int index)
{
	return 0;
}

static void cix_get_related_cpus(int index, struct cpumask *m)
{
	struct device_node *cpu_np;
	struct of_phandle_args args;
	int cpu, ret;

	for_each_possible_cpu(cpu) {
		cpu_np = of_cpu_device_node_get(cpu);
		if (!cpu_np)
			continue;

		ret = of_parse_phandle_with_args(cpu_np, "cix,freq-domain",
						"#freq-domain-cells", 0,
						&args);
		of_node_put(cpu_np);
		if (ret < 0)
			continue;

		if (index == args.args[0])
			cpumask_set_cpu(cpu, m);
	}
}

static int cix_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
	struct platform_device *pdev = cpufreq_get_driver_data();
	struct device *dev = &pdev->dev;
	struct device *cpu_dev;
	struct of_phandle_args args;
	struct resource *res;
	void __iomem *base;
	struct cix_cpufreq_data *data;
	struct device_node *cpu_np;
	int ret, index;

	cpu_dev = get_cpu_device(policy->cpu);
	if (!cpu_dev) {
		pr_err("%s: failed to get cpu%d device\n", __func__,
		      policy->cpu);
		return -ENODEV;
	}

	cpu_np = of_cpu_device_node_get(policy->cpu);
	if (!cpu_np) {
		pr_err("%s: failed to get cpu%d device node\n", __func__,
		      policy->cpu);
		return -EINVAL;
	}

	ret = of_parse_phandle_with_args(cpu_np, "cix,freq-domain",
					"#freq-domain-cells", 0, &args);
	if (ret) {
		pr_err("%s: failed to parse cpu%d freq-domain\n", __func__,
		      policy->cpu);
		return ret;
	}
	of_node_put(cpu_np);

	index = args.args[0];

	res = platform_get_resource(pdev, IORESOURCE_MEM, index);
	if (!res) {
		dev_err(dev, "failed to get mem resource %d\n", index);
		return -ENODEV;
	}

	if (!request_mem_region(res->start, resource_size(res), res->name)) {
		dev_err(dev, "failed to request resource %pR\n", res);
		return -EBUSY;
	}

	base = ioremap(res->start, resource_size(res));
	if (!base) {
		dev_err(dev, "failed to map resource %pR\n", res);
		ret = -ENOMEM;
		goto release_region;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto unmap_base;
	}

	data->base = base;
	data->res = res;

	cix_get_related_cpus(index, policy->cpus);
	if (!cpumask_weight(policy->cpus)) {
		dev_err(dev, "Domain-%d failed to get related CPUs\n", index);
		ret = -ENOENT;
		goto error;
	}

	policy->driver_data = data;
	policy->dvfs_possible_from_any_cpu = true;

	ret = cix_cpufreq_read_lut(cpu_dev, policy);
	if (ret) {
		dev_err(cpu_dev, "Failed to read LUT\n");
		return -EINVAL;
	}
	ret = dev_pm_opp_get_opp_count(cpu_dev);
	if (ret <= 0) {
		dev_err(cpu_dev, "Failed to add OPPs\n");
		ret = -ENODEV;
		goto error;
	}

	if (policy_has_boost_freq(policy)) {
		ret = cpufreq_enable_boost_support();
		if (ret)
			dev_warn(cpu_dev, "failed to enable boost: %d\n", ret);
	}

	return 0;
error:
	kfree(data);
unmap_base:
	iounmap(base);
release_region:
	release_mem_region(res->start, resource_size(res));
	return ret;
}

static int cix_cpufreq_cpu_exit(struct cpufreq_policy *policy)
{
	struct device *cpu_dev = get_cpu_device(policy->cpu);
	struct cix_cpufreq_data *data = policy->driver_data;
	struct resource *res = data->res;
	void __iomem *base = data->base;

	dev_pm_opp_remove_all_dynamic(cpu_dev);
	dev_pm_opp_of_cpumask_remove_table(policy->related_cpus);
	kfree(policy->freq_table);
	kfree(data);
	iounmap(base);
	release_mem_region(res->start, resource_size(res));

	return 0;
}

static struct freq_attr *cix_cpufreq_attrs[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL
};

static struct cpufreq_driver cpufreq_cix_driver = {
	.flags		= CPUFREQ_NEED_INITIAL_FREQ_CHECK |
			  CPUFREQ_HAVE_GOVERNOR_PER_POLICY |
			  CPUFREQ_IS_COOLING_DEV,
	.verify		= cpufreq_generic_frequency_table_verify,
	.target_index	= cix_cpufreq_target_index,
	.get		= cix_cpufreq_get,
	.init		= cix_cpufreq_cpu_init,
	.exit		= cix_cpufreq_cpu_exit,
	.name		= "cix-cpufreq",
	.attr		= cix_cpufreq_attrs,
};

static int cix_cpufreq_driver_probe(struct platform_device *pdev)
{
	int ret;

	cpufreq_cix_driver.driver_data = pdev;
	ret = cpufreq_register_driver(&cpufreq_cix_driver);

	return ret;
}

static int cix_cpufreq_driver_remove(struct platform_device *pdev)
{
	return cpufreq_unregister_driver(&cpufreq_cix_driver);
}

static const struct of_device_id cix_cpufreq_match[] = {
	{ .compatible = "cix,cpufreq"},
	{}
};
MODULE_DEVICE_TABLE(of, cix_cpufreq_match);

static struct platform_driver cix_cpufreq_driver = {
	.probe = cix_cpufreq_driver_probe,
	.remove = cix_cpufreq_driver_remove,
	.driver = {
		.name = "cix-cpufreq",
		.of_match_table = cix_cpufreq_match,
	},
};

static int __init cix_cpufreq_init(void)
{
	return platform_driver_register(&cix_cpufreq_driver);
}
postcore_initcall(cix_cpufreq_init);

static void __exit cix_cpufreq_exit(void)
{
	platform_driver_unregister(&cix_cpufreq_driver);
}
module_exit(cix_cpufreq_exit);
MODULE_AUTHOR("xinglong yang <xinglong.yang@cixtech.com>");
MODULE_DESCRIPTION("CIX CPUFREQ DT Driver");
MODULE_LICENSE("GPL v2");

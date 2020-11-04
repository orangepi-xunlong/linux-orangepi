// SPDX-License-Identifier: GPL-2.0
/*
 * The sun50i-cpufreq-nvmem driver reads the efuse value from the SoC to
 * provide the OPP framework with required information.
 *
 * Copyright (C) 2020 frank@allwinnertech.com
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>

static struct platform_device *cpufreq_dt_pdev, *sun50i_cpufreq_pdev;

/**
 * sun50i_cpufreq_get_efuse() - Determine speed grade from efuse value
 * @versions: Set to the value parsed from efuse
 *
 * Returns 0 if success.
 */
static int sun50i_cpufreq_get_efuse(u32 *versions)
{
	struct nvmem_cell *speedbin_nvmem;
	struct device_node *np;
	struct device *cpu_dev;
	u32 *efuse_value;
	size_t len;
	int ret;

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev)
		return -ENODEV;

	np = of_parse_phandle(cpu_dev->of_node, "operating-points-v2", 0);
	if (!np)
		return -ENOENT;

	ret = of_device_is_compatible(np,
				      "allwinner,sun50i-operating-points");
	if (!ret) {
		of_node_put(np);
		return -ENOENT;
	}

	speedbin_nvmem = of_nvmem_cell_get(np, "speed");
	of_node_put(np);
	if (IS_ERR(speedbin_nvmem)) {
		if (PTR_ERR(speedbin_nvmem) != -EPROBE_DEFER)
			pr_err("Could not get nvmem cell: %ld\n",
			       PTR_ERR(speedbin_nvmem));
		return PTR_ERR(speedbin_nvmem);
	}

	efuse_value = nvmem_cell_read(speedbin_nvmem, &len);
	nvmem_cell_put(speedbin_nvmem);
	if (IS_ERR(efuse_value))
		return PTR_ERR(efuse_value);

	switch (*efuse_value) {
	case 0x2400:
	case 0x5000:
	case 0x5400:
	case 0x6c00:
	case 0x7400:
		*versions = 0b0001;
		break;
	case 0x2c00:
	case 0x7c00:
		*versions = 0b0010;
		break;
	case 0x5c00:
	default:
		*versions = 0b0100;
	}

	kfree(efuse_value);
	return 0;
};

static int sun50i_cpufreq_nvmem_probe(struct platform_device *pdev)
{
	struct opp_table **opp_tables;
	unsigned int cpu;
	u32 speed = 0;
	int ret;

	opp_tables = kcalloc(num_possible_cpus(), sizeof(*opp_tables),
			     GFP_KERNEL);
	if (!opp_tables)
		return -ENOMEM;

	ret = sun50i_cpufreq_get_efuse(&speed);
	if (ret)
		return ret;

	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);

		if (!cpu_dev) {
			ret = -ENODEV;
			goto free_opp;
		}

		ret = dev_pm_opp_set_supported_hw(cpu_dev, &speed, 1);
		if (ret) {
			dev_err(cpu_dev, "Failed to set supported hardware\n");
			return ret;
		}
	}

	cpufreq_dt_pdev = platform_device_register_simple("cpufreq-dt", -1,
							  NULL, 0);
	if (!IS_ERR(cpufreq_dt_pdev)) {
		platform_set_drvdata(pdev, opp_tables);
		return 0;
	}

	ret = PTR_ERR(cpufreq_dt_pdev);
	pr_err("Failed to register platform device\n");

free_opp:
	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);
		if (IS_ERR_OR_NULL(opp_tables[cpu]))
			break;
		dev_pm_opp_put_supported_hw(cpu_dev);
	}
	kfree(opp_tables);

	return ret;
}

static int sun50i_cpufreq_nvmem_remove(struct platform_device *pdev)
{
	struct opp_table **opp_tables = platform_get_drvdata(pdev);
	unsigned int cpu;

	platform_device_unregister(cpufreq_dt_pdev);

	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);
		dev_pm_opp_put_supported_hw(cpu_dev);
	}

	kfree(opp_tables);

	return 0;
}

static struct platform_driver sun50i_cpufreq_driver = {
	.probe = sun50i_cpufreq_nvmem_probe,
	.remove = sun50i_cpufreq_nvmem_remove,
	.driver = {
		.name = "sun50i-cpufreq-nvmem",
	},
};

static const struct of_device_id sun50i_cpufreq_match_list[] = {
	{ .compatible = "arm,sun50iw9p1" },
	{}
};

static const struct of_device_id *sun50i_cpufreq_match_node(void)
{
	const struct of_device_id *match;
	struct device_node *np;

	np = of_find_node_by_path("/");
	match = of_match_node(sun50i_cpufreq_match_list, np);
	of_node_put(np);

	return match;
}

/*
 * Since the driver depends on nvmem drivers, which may return EPROBE_DEFER,
 * all the real activity is done in the probe, which may be defered as well.
 * The init here is only registering the driver and the platform device.
 */
static int __init sun50i_cpufreq_init(void)
{
	const struct of_device_id *match;
	int ret;

	match = sun50i_cpufreq_match_node();
	if (!match)
		return -ENODEV;

	ret = platform_driver_register(&sun50i_cpufreq_driver);
	if (unlikely(ret < 0))
		return ret;

	sun50i_cpufreq_pdev =
		platform_device_register_simple("sun50i-cpufreq-nvmem",
						-1, NULL, 0);
	ret = PTR_ERR_OR_ZERO(sun50i_cpufreq_pdev);
	if (ret == 0)
		return 0;

	platform_driver_unregister(&sun50i_cpufreq_driver);
	return ret;
}
module_init(sun50i_cpufreq_init);

static void __exit sun50i_cpufreq_exit(void)
{
	platform_device_unregister(sun50i_cpufreq_pdev);
	platform_driver_unregister(&sun50i_cpufreq_driver);
}
module_exit(sun50i_cpufreq_exit);

MODULE_DESCRIPTION("Sun50i cpufreq driver");
MODULE_LICENSE("GPL v2");

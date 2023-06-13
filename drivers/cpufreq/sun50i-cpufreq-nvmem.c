// SPDX-License-Identifier: GPL-2.0
/*
 * Allwinner CPUFreq nvmem based driver
 *
 * The sun50i-cpufreq-nvmem driver reads the efuse value from the SoC to
 * provide the OPP framework with required information.
 *
 * Copyright (C) 2019 Yangtao Li <tiny.windzz@gmail.com>
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>

#define MAX_NAME_LEN	7

#define SUN50I_H616_NVMEM_MASK 0x22
#define SUN50I_H616_NVMEM_SHIFT 5
#define SUN50I_H6_NVMEM_MASK 0x7
#define SUN50I_H6_NVMEM_SHIFT 5

struct sunxi_cpufreq_soc_data {
	u32 (*efuse_xlate) (void *efuse);
};

static struct platform_device *cpufreq_dt_pdev, *sun50i_cpufreq_pdev;

static u32 sun50i_h616_efuse_xlate(void *efuse)
{
	u32 efuse_value = (*(u32 *)efuse >> SUN50I_H616_NVMEM_SHIFT) &
			  SUN50I_H616_NVMEM_MASK;

	/* Tested as h616 soc. Expected efuse values are 1 -3,
	   slowest to fastest */
	if (efuse_value >=1 && efuse_value <= 3)
		return efuse_value - 1;
	else
		return 0;
};

static u32 sun50i_h6_efuse_xlate(void *efuse)
{
	u32 efuse_value = (*(u32 *)efuse >> SUN50I_H6_NVMEM_SHIFT) &
			  SUN50I_H6_NVMEM_MASK;

	/*
	 * We treat unexpected efuse values as if the SoC was from
	 * the slowest bin. Expected efuse values are 1 -3, slowest
	 * to fastest.
	*/
	if (efuse_value >= 1 && efuse_value <= 3)
		return efuse_value - 1;
	else
		return 0;
};


/**
 * sun50i_cpufreq_get_efuse() - Determine speed grade from efuse value
 * @soc_data: pointer to sunxi_cpufreq_soc_data context
 * @versions: Set to the value parsed from efuse
 *
 * Returns 0 if success.
 */
static int sun50i_cpufreq_get_efuse(const struct sunxi_cpufreq_soc_data *soc_data,
				    u32 *versions)
{
	struct nvmem_cell *speedbin_nvmem;
	struct device_node *np;
	struct device *cpu_dev;
	u32 *speedbin;
	size_t len;

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev)
		return -ENODEV;

	np = dev_pm_opp_of_get_opp_desc_node(cpu_dev);
	if (!np)
		return -ENOENT;

	if      (of_device_is_compatible(np, "allwinner,sun50i-h6-operating-points")) {}
	else if (of_device_is_compatible(np, "allwinner,sun50i-h616-operating-points")) {}
	else {
		of_node_put(np);
		return -ENOENT;
	}

	speedbin_nvmem = of_nvmem_cell_get(np, NULL);
	of_node_put(np);
	if (IS_ERR(speedbin_nvmem))
		return dev_err_probe(cpu_dev, PTR_ERR(speedbin_nvmem),
				     "Could not get nvmem cell\n");

	speedbin = nvmem_cell_read(speedbin_nvmem, &len);
	nvmem_cell_put(speedbin_nvmem);
	if (IS_ERR(speedbin))
		return PTR_ERR(speedbin);

	*versions = soc_data->efuse_xlate(speedbin);

	kfree(speedbin);
	return 0;
};

static int sun50i_cpufreq_nvmem_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct opp_table **opp_tables;
	char name[MAX_NAME_LEN];
	unsigned int cpu;
	u32 speed = 0;
	int ret;

	match = dev_get_platdata(&pdev->dev);
	if (!match) {
		return -EINVAL;
	}

	opp_tables = kcalloc(num_possible_cpus(), sizeof(*opp_tables),
			     GFP_KERNEL);
	if (!opp_tables)
		return -ENOMEM;

	ret = sun50i_cpufreq_get_efuse(match->data, &speed);
	if (ret) {
		kfree(opp_tables);
		return ret;
	}

	printk("sun50i-cpufreq-nvmem: will use speed%d CPU OPPs\n", speed);

	snprintf(name, MAX_NAME_LEN, "speed%d", speed);

	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);

		if (!cpu_dev) {
			ret = -ENODEV;
			goto free_opp;
		}

		opp_tables[cpu] = dev_pm_opp_set_prop_name(cpu_dev, name);
		if (opp_tables[cpu] < 0) {
			ret = opp_tables[cpu];
			pr_err("Failed to set prop name\n");
			goto free_opp;
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
	for_each_possible_cpu(cpu)
		dev_pm_opp_put_prop_name(opp_tables[cpu]);
	kfree(opp_tables);

	return ret;
}

static int sun50i_cpufreq_nvmem_remove(struct platform_device *pdev)
{
	int *opp_tables = platform_get_drvdata(pdev);
	unsigned int cpu;

	platform_device_unregister(cpufreq_dt_pdev);

	for_each_possible_cpu(cpu)
		dev_pm_opp_put_prop_name(opp_tables[cpu]);

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

static const struct sunxi_cpufreq_soc_data sun50i_h616_data = {
    .efuse_xlate = sun50i_h616_efuse_xlate,
};

static const struct sunxi_cpufreq_soc_data sun50i_h6_data = {
    .efuse_xlate = sun50i_h6_efuse_xlate,
};

static const struct of_device_id sun50i_cpufreq_match_list[] = {
	{ .compatible = "allwinner,sun50i-h6", .data = &sun50i_h6_data },
	{ .compatible = "allwinner,sun50i-h616", .data = &sun50i_h616_data },
	{}
};
MODULE_DEVICE_TABLE(of, sun50i_cpufreq_match_list);

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

	sun50i_cpufreq_pdev = platform_device_register_data(NULL,
			      "sun50i-cpufreq-nvmem", -1, match, sizeof(*match));
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

MODULE_DESCRIPTION("Sun50i-h6 cpufreq driver");
MODULE_LICENSE("GPL v2");

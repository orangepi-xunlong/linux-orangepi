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
#include <linux/sunxi-sid.h>

#define MAX_NAME_LEN    3

#define SUN50IW9_ICPU_MASK     GENMASK(9, 0)

static struct platform_device *cpufreq_dt_pdev, *sun50i_cpufreq_pdev;

struct cpufreq_nvmem_data {
	u32 nv_speed;
	u32 nv_Icpu;
	u32 nv_bin;
	u32 nv_bin_ext;
	u32 version;
	char name[MAX_NAME_LEN];
};

static struct cpufreq_nvmem_data ver_data;

struct cpufreq_soc_data {
	void (*nvmem_xlate)(u32 *versions, char *name);
	bool has_nvmem_Icpu;
	bool has_nvmem_bin;
};

static int sun50i_nvmem_get_data(char *cell_name, u32 *data)
{
	struct nvmem_cell *cell_nvmem;
	size_t len;
	u8 *cell_value;
	u32 tmp_data = 0;
	u32 i;
	struct device_node *np;
	struct device *cpu_dev;
	int ret = 0;

	cpu_dev = get_cpu_device(0);
	if (!cpu_dev)
		return -ENODEV;

	np = dev_pm_opp_of_get_opp_desc_node(cpu_dev);
	if (!np)
		return -ENOENT;

	ret = of_device_is_compatible(np,
				      "allwinner,sun50i-operating-points");
	if (!ret) {
		of_node_put(np);
		return -ENOENT;
	}

	cell_nvmem = of_nvmem_cell_get(np, cell_name);
	of_node_put(np);
	if (IS_ERR(cell_nvmem)) {
		if (PTR_ERR(cell_nvmem) != -EPROBE_DEFER)
			pr_err("Could not get nvmem cell: %ld\n",
			       PTR_ERR(cell_nvmem));
		return PTR_ERR(cell_nvmem);
	}

	cell_value = nvmem_cell_read(cell_nvmem, &len);
	nvmem_cell_put(cell_nvmem);
	if (IS_ERR(cell_value))
		return PTR_ERR(cell_value);

	if (len > 4) {
		pr_err("Invalid nvmem cell length\n");
		ret = -EINVAL;
	} else {
		for (i = 0; i < len; i++)
			tmp_data |= ((u32)cell_value[i] << (i * 8));
		*data = tmp_data;
	}

	kfree(cell_value);

	return 0;
}

static void sun50iw9_nvmem_xlate(u32 *versions, char *name)
{
	int value = 0;
	unsigned int ver_bits = sunxi_get_soc_ver() & 0x7;

	switch (ver_data.nv_speed) {
	case 0x2000:
		value = 0;
		break;
	case 0x2400:
	case 0x7400:
	case 0x2c00:
	case 0x7c00:
		if (ver_bits <= 1) {
			/* ic version A/B */
			value = 1;
		} else {
			/* ic version C and later version */
			value = 2;
		}
		break;
	case 0x5000:
	case 0x5400:
	case 0x6000:
		value = 3;
		break;
	case 0x5c00:
		value = 4;
		break;
	case 0x5d00:
	default:
		value = 0;
	}
	*versions = (1 << value);
	snprintf(name, MAX_NAME_LEN, "a%d", value);
}

static struct cpufreq_soc_data sun50iw9_soc_data = {
	.nvmem_xlate = sun50iw9_nvmem_xlate,
	.has_nvmem_Icpu = false,
};

static void sun50iw10_bin_xlate(bool high_speed, char *name, u32 bin)
{
	int value = 0;
	bool version_before_f;
	unsigned int ver_bits = sunxi_get_soc_ver() & 0x7;
	u32 bin_ext = ver_data.nv_bin_ext;

	bin >>= 12;
	bin_ext >>= 31;

	if (ver_bits == 0 || ver_bits == 3 || ver_bits == 4)
		version_before_f = true;
	else
		version_before_f = false;

	if (high_speed) {
		switch (bin) {
		case 0b100:
			if (version_before_f) {
				/* ic version A-E */
				value = 1;
			} else {
				/* ic version F and later version */
				value = 3;
			}
			break;
		default:
			if (version_before_f) {
				/* ic version A-E */
				value = 0;
			} else {
				/* ic version F and later version */
				value = 2;
			}
		}

		snprintf(name, MAX_NAME_LEN, "b%d", value);
	} else {
		if (bin_ext && (!version_before_f)) {
			value = 6;
		} else {
			switch (bin) {
			case 0b100:
				if (version_before_f) {
					/* ic version A-E */
					value = 2;
				} else {
					/* ic version F and later version */
					value = 5;
				}
				break;
			case 0b010:
				if (version_before_f) {
					/* ic version A-E */
					value = 1;
				} else {
					/* ic version F and later version */
					value = 4;
				}
				break;
			default:
				if (version_before_f) {
					/* ic version A-E */
					value = 0;
				} else {
					/* ic version F and later version */
					value = 3;
				}
			}
		}
		snprintf(name, MAX_NAME_LEN, "a%d", value);
	}
}

static void sun50iw10_nvmem_xlate(u32 *versions, char *name)
{
	unsigned int ver_bits = sunxi_get_soc_ver() & 0x7;

	switch (ver_data.nv_speed) {
	case 0x0200:
	case 0x0600:
	case 0x0620:
	case 0x0640:
	case 0x0800:
	case 0x1000:
	case 0x1400:
	case 0x2000:
	case 0x4000:
		if (ver_bits == 0 || ver_bits == 3 || ver_bits == 4) {
			/* ic version A-E */
			*versions = 0b0100;
		} else {
			/* ic version F and later version */
			*versions = 0b0010;
		}
		sun50iw10_bin_xlate(true, name, ver_data.nv_bin);
		break;
	case 0x0400:
	default:
		*versions = 0b0001;
		sun50iw10_bin_xlate(false, name, ver_data.nv_bin);
	}
}

static struct cpufreq_soc_data sun50iw10_soc_data = {
	.nvmem_xlate = sun50iw10_nvmem_xlate,
	.has_nvmem_bin = true,
};

/**
 * sun50i_cpufreq_get_efuse() - Determine speed grade from efuse value
 * @versions: Set to the value parsed from efuse
 *
 * Returns 0 if success.
 */
static int sun50i_cpufreq_get_efuse(const struct cpufreq_soc_data *soc_data,
				    u32 *versions, char *name)
{
	int ret;

	ret = sun50i_nvmem_get_data("speed", &ver_data.nv_speed);
	if (ret)
		return ret;

	if (soc_data->has_nvmem_Icpu) {
		ret = sun50i_nvmem_get_data("Icpu", &ver_data.nv_Icpu);
		if (ret)
			return ret;
	}

	if (soc_data->has_nvmem_bin) {
		ret = sun50i_nvmem_get_data("bin", &ver_data.nv_bin);
		if (ret)
			return ret;
		ret = sun50i_nvmem_get_data("bin_ext", &ver_data.nv_bin_ext);
		if (ret)
			return ret;
	}

	soc_data->nvmem_xlate(versions, name);

	return 0;
};

static int sun50i_cpufreq_nvmem_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	struct opp_table **opp_tables;
	unsigned int cpu;
	int ret;

	opp_tables = kcalloc(num_possible_cpus(), sizeof(*opp_tables),
			     GFP_KERNEL);
	if (!opp_tables)
		return -ENOMEM;

	match = dev_get_platdata(&pdev->dev);
	if (!match)
		return -EINVAL;

	ret = sun50i_cpufreq_get_efuse(match->data,
				       &ver_data.version, ver_data.name);
	if (ret)
		return ret;

	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);

		if (!cpu_dev) {
			ret = -ENODEV;
			goto free_opp;
		}

		if (strlen(ver_data.name)) {
			opp_tables[cpu] = dev_pm_opp_set_prop_name(cpu_dev,
								   ver_data.name);
			if (IS_ERR(opp_tables[cpu])) {
				ret = PTR_ERR(opp_tables[cpu]);
				pr_err("Failed to set prop name\n");
				goto free_opp;
			}
		}

		if (ver_data.version) {
			opp_tables[cpu] = dev_pm_opp_set_supported_hw(cpu_dev,
							  &ver_data.version, 1);
			if (IS_ERR(opp_tables[cpu])) {
				ret = PTR_ERR(opp_tables[cpu]);
				pr_err("Failed to set hw\n");
				goto free_opp;
			}
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
		if (IS_ERR_OR_NULL(opp_tables[cpu]))
			break;

		if (strlen(ver_data.name))
			dev_pm_opp_put_prop_name(opp_tables[cpu]);

		if (ver_data.version)
			dev_pm_opp_put_supported_hw(opp_tables[cpu]);
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
		if (IS_ERR_OR_NULL(opp_tables[cpu]))
			break;

		if (strlen(ver_data.name))
			dev_pm_opp_put_prop_name(opp_tables[cpu]);

		if (ver_data.version)
			dev_pm_opp_put_supported_hw(opp_tables[cpu]);
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
	{ .compatible = "arm,sun50iw9p1", .data = &sun50iw9_soc_data, },
	{ .compatible = "arm,sun50iw10p1", .data = &sun50iw10_soc_data, },
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
							    "sun50i-cpufreq-nvmem",
							    -1, match,
							    sizeof(*match));
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

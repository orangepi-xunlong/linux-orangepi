// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * The sun50i-cpufreq-nvmem driver reads the efuse value from the SoC to
 * provide the OPP framework with required information.
 *
 * Copyright (C) 2020 frank@allwinnertech.com
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <sunxi-log.h>
#include <linux/module.h>
#include <linux/nvmem-consumer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/pm_opp.h>
#include <linux/slab.h>
#include <sunxi-sid.h>
#include <linux/cpu.h>
#include <linux/clk.h>

#define PVALUE_OFFSET   0x20
#define ICPU_OFFSET     0x28
#define MAX_NAME_LEN      12
#define MAX_VF_VER_LEN    8

#define SUN50IW9_ICPU_MASK     GENMASK(9, 0)

static struct platform_device *cpufreq_dt_pdev, *sun50i_cpufreq_pdev;

struct cpufreq_nvmem_data {
	u32 nv_speed;
	u32 nv_Icpu;
	u32 nv_bin;
	u32 nv_bin_ext;
	u32 version;
	u32 vf_index;
	u16 dvfs_code;
	char name[MAX_NAME_LEN];
	char vf_ver[MAX_VF_VER_LEN];
};

static struct cpufreq_nvmem_data ver_data;

struct cpufreq_soc_data {
	void (*nvmem_xlate)(u32 *versions, char *name);
	bool has_nvmem_speed;
	bool has_nvmem_Icpu;
	bool has_nvmem_bin;
	bool has_nvmem_extend_bin;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t dvfs_code_show(struct class *class, struct class_attribute *attr,
			 char *buf)
#else
static ssize_t dvfs_code_show(const struct class *class, const struct class_attribute *attr,
			 char *buf)
#endif
{
	return sprintf(buf, "0x%04x\n", ver_data.dvfs_code);
}

static CLASS_ATTR_RO(dvfs_code);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
static ssize_t vf_version_show(struct class *class, struct class_attribute *attr,
			 char *buf)
#else
static ssize_t vf_version_show(const struct class *class, const struct class_attribute *attr,
			 char *buf)
#endif
{
	return snprintf(buf, MAX_VF_VER_LEN + 1, "%s\n", ver_data.vf_ver);
}

static CLASS_ATTR_RO(vf_version);

static struct attribute *cpufreq_class_attrs[] = {
	&class_attr_dvfs_code.attr,
	&class_attr_vf_version.attr,
	NULL,
};
ATTRIBUTE_GROUPS(cpufreq_class);

static struct class cpufreq_class = {
	.name		= "cpufreq",
	.class_groups	= cpufreq_class_groups,
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
			sunxi_err(NULL, "Could not get nvmem cell: %ld\n",
			       PTR_ERR(cell_nvmem));
		return PTR_ERR(cell_nvmem);
	}

	cell_value = nvmem_cell_read(cell_nvmem, &len);
	nvmem_cell_put(cell_nvmem);
	if (IS_ERR(cell_value))
		return PTR_ERR(cell_value);

	if (len > 4) {
		sunxi_err(NULL, "Invalid nvmem cell length\n");
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
	u32 pvalue = 0;
	u32 icpu = 0;
	unsigned int ver_bits = sunxi_get_soc_ver() & 0x7;

	sunxi_get_module_param_from_sid(&pvalue, PVALUE_OFFSET, 4);
	pvalue &= 0xff;
	pvalue *= 32;

	sunxi_get_module_param_from_sid(&icpu, ICPU_OFFSET, 4);
	icpu &= 0x3ff;

	switch (ver_data.nv_speed) {
	case 0x2000:
		value = 0;
		break;
	case 0x2100:
		value = 7;
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
		if (ver_bits == 0)
			value = 4;
		else {
			if ((icpu >= 38) && (icpu <= 45) && (pvalue >= 2656) && (pvalue <= 5024))
				value = 5;
			else
				value = 6;
		}
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
	.has_nvmem_speed = true,
};

#define SUN50IW10_ICPU_EFUSE_OFF (0x1c)
#define SUN50IW10_PCPU_EFUSE_OFF (0x20)
static bool version_before_f;
static void sun50iw10_bin_xlate(bool high_speed, char *name, u32 bin)
{
	int value = 0;
	u32 bin_icpu, bin_pcpu;

	bin >>= 12;

	sunxi_get_module_param_from_sid(&bin_icpu, SUN50IW10_ICPU_EFUSE_OFF, 4);
	bin_icpu &= 0xfff; /* the unit is 0.1mA */
	sunxi_get_module_param_from_sid(&bin_pcpu, SUN50IW10_PCPU_EFUSE_OFF, 4);
	bin_pcpu &= 0xfff;

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
		if ((!version_before_f) && (bin_pcpu >= 2900) && (bin_icpu >= 260 && bin_icpu < 360)) {
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

static void sun50iw10_bin_force_vf1(u32 *versions, char *name)
{
	bool force_vf1;
	u32 bin_icpu, bin_pcpu;

	sunxi_get_module_param_from_sid(&bin_icpu, SUN50IW10_ICPU_EFUSE_OFF, 4);
	bin_icpu &= 0xfff; /* the unit is 0.1mA */
	sunxi_get_module_param_from_sid(&bin_pcpu, SUN50IW10_PCPU_EFUSE_OFF, 4);
	bin_pcpu &= 0xfff;

	if (version_before_f && (bin_icpu >= 84 && bin_icpu < 160) && (bin_pcpu >= 2900 && bin_pcpu < 3100))
		force_vf1 = true;
	else
		force_vf1 = false;

	if (force_vf1) {
		*versions = 0b0001;
		snprintf(name, MAX_NAME_LEN, "a0");
	}
}

static void sun50iw10_bin_xlate_0x8000(bool high_speed, char *name, u32 bin)
{

	int value = 0;

	snprintf(name, MAX_NAME_LEN, "c%d", value);
}

static void sun50iw10_nvmem_xlate(u32 *versions, char *name)
{
	unsigned int ver_bits = sunxi_get_soc_ver() & 0x7;

	if (ver_bits == 0 || ver_bits == 3 || ver_bits == 4)
		version_before_f = true;
	else
		version_before_f = false;

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
		if (version_before_f) {
			/* ic version A-E */
			*versions = 0b0100;
		} else {
			/* ic version F and later version */
			*versions = 0b0010;
		}
		sun50iw10_bin_xlate(true, name, ver_data.nv_bin);
		break;
	case 0x8000:
		*versions = 0b1000;
		sun50iw10_bin_xlate_0x8000(true, name, ver_data.nv_bin);
		break;
	case 0x0400:
	default:
		*versions = 0b0001;
		sun50iw10_bin_xlate(false, name, ver_data.nv_bin);
		sun50iw10_bin_force_vf1(versions, name);
	}
	sunxi_debug(NULL, "using table: %s\n", name);
}

static struct cpufreq_soc_data sun50iw10_soc_data = {
	.nvmem_xlate = sun50iw10_nvmem_xlate,
	.has_nvmem_speed = true,
	.has_nvmem_bin = true,
};

static void sun8iw20_bin_xlate(bool bin_select, char *name, u32 nv_bin)
{
	int value = 0;
	u32 bin = (nv_bin >> 12) & 0xf;

	if (bin_select) {
		if (bin <= 1)
			value = 1;
		else
			value = 0;
		/* BGA use axx in VF table */
		snprintf(name, MAX_NAME_LEN, "a%d", value);
	} else {
		value = 0;
		/* QFN use bxx in VF table */
		snprintf(name, MAX_NAME_LEN, "b%d", value);
	}
}

static void sun8iw20_nvmem_xlate(u32 *versions, char *name)
{
	switch (ver_data.nv_speed) {
	case 0x6000:
		sun8iw20_bin_xlate(false, name, ver_data.nv_bin);
		*versions = 0b0010;
		break;
	case 0x6400:
	case 0x7000:
	case 0x7c00:
	default:
		sun8iw20_bin_xlate(true, name, ver_data.nv_bin);
		*versions = 0b0001;
		break;
	}
	sunxi_debug(NULL, "sun8iw20 match vf:%s, mark:0x%x\n", name, ver_data.nv_speed);
}

static struct cpufreq_soc_data sun8iw20_soc_data = {
	.nvmem_xlate = sun8iw20_nvmem_xlate,
	.has_nvmem_speed = true,
	.has_nvmem_bin = true,
};

static void sun20iw1_bin_xlate(bool bin_select, bool high_speed, char *name, u32 nv_bin)
{
	int value = 0;
	u32 bin = (nv_bin >> 12) & 0xf;

	if (bin_select) {
		if (bin <= 1)
			value = 1;
		else
			value = 0;
		/* BGA use axx in VF table */
		snprintf(name, MAX_NAME_LEN, "a%d", value);
	} else {
		if (high_speed)
			value = 1;
		else
			value = 0;
		/* QFN use bxx in VF table */
		snprintf(name, MAX_NAME_LEN, "b%d", value);
	}
}

static void sun20iw1_nvmem_xlate(u32 *versions, char *name)
{
	switch (ver_data.nv_speed) {
	case 0x5e00:
		sun20iw1_bin_xlate(false, false, name, ver_data.nv_bin);
		*versions = 0b0010;
		break;
	case 0x5c00:
	case 0x7400:
		sun20iw1_bin_xlate(false, true, name, ver_data.nv_bin);
		*versions = 0b0001;
		break;
	case 0x5000:
	default:
		sun20iw1_bin_xlate(true, true, name, ver_data.nv_bin);
		*versions = 0b0001;
	}
	sunxi_debug(NULL, "sun20iw1 match vf:%s, mark:0x%x\n", name, ver_data.nv_speed);
}

static struct cpufreq_soc_data sun20iw1_soc_data = {
	.nvmem_xlate = sun20iw1_nvmem_xlate,
	.has_nvmem_speed = true,
	.has_nvmem_bin = true,
};

static void sunxi_set_vf_index(u32 index)
{
	ver_data.vf_index = index;
}

u32 sunxi_get_vf_index(void)
{
	return ver_data.vf_index;
}
EXPORT_SYMBOL_GPL(sunxi_get_vf_index);

static int get_vf_table_version(void)
{
	struct device_node *np = NULL;
	const char *vf_ver = NULL;
	int ret = 0;

	np = of_find_node_by_name(NULL, "vf_mapping_table");
	if (!np) {
		sunxi_err(NULL, "Unable to find node\n");
		return -EINVAL;
	}

	ret = of_property_read_string(np, "vf-version", &vf_ver);
	if (ret) {
		sunxi_err(NULL, "could not retrieve vf version property: %d\n", ret);
		return ret;
	}

	if (vf_ver)
		strncpy(ver_data.vf_ver, vf_ver, strlen(vf_ver));

	return ret;
}

static int match_vf_table(u32 combi, u32 *index)
{
	struct device_node *np = NULL;
	int nsels, ret, i;
	u32 tmp;

	np = of_find_node_by_name(NULL, "vf_mapping_table");
	if (!np) {
		sunxi_err(NULL, "Unable to find node\n");
		return -EINVAL;
	}

	if (!of_get_property(np, "table", &nsels))
		return -EINVAL;

	nsels /= sizeof(u32);
	if (!nsels) {
		sunxi_err(NULL, "invalid table property size\n");
		return -EINVAL;
	}

	for (i = 0; i < nsels / 2; i++) {
		ret = of_property_read_u32_index(np, "table", i * 2, &tmp);
		if (ret) {
			sunxi_err(NULL, "could not retrieve table property: %d\n", ret);
			return ret;
		}

		if (tmp == combi) {
			ret = of_property_read_u32_index(np, "table", i * 2 + 1, &tmp);
			if (ret) {
				sunxi_err(NULL, "could not retrieve table property: %d\n", ret);
				return ret;
			}

			*index = tmp;
			break;
		} else
			continue;
	}

	if (i == nsels/2)
		sunxi_debug(NULL, "%s %d, could not match vf table, i:%d", __func__, __LINE__, i);

	return 0;
}

#define SUN55IW3_DVFS_EFUSE_OFF     (0x48)
#define SUN55IW3_P_CPU4_EFUSE_OFF   (0x20)
#define SUN55IW3_P_VE_GPU_EFUSE_OFF (0x28)
#define SUN55IW3_ICPU_EFUSE_OFF     (0x38)

struct ps_icpu_info_t {
	u16 ps_min;
	u16 ps_max;
	u16 icpu_min;
	u16 icpu_max;
	u32 index;
};

static const struct ps_icpu_info_t  ps_icpu_info[] = {
	{2450, 3050,  9,    65535,  0x0100},
	{3050, 4000,  9,    65535,  0x0201},
};

static u32 match_vf_table_ext(void)
{
	u32 i = 0;
	u32 index = 0x0100;
	u32 tmp = 0;
	u16 p_cpu4, p_ve, p_gpu, p_min, icpu, icpu_d, icpu_n;

	sunxi_get_module_param_from_sid(&tmp, SUN55IW3_P_CPU4_EFUSE_OFF, 4);
	p_cpu4 = tmp & 0xffff;

	sunxi_get_module_param_from_sid(&tmp, SUN55IW3_P_VE_GPU_EFUSE_OFF, 4);
	p_ve = tmp & 0xffff;
	p_gpu = (tmp >> 16) & 0xffff;
	p_min = min3(p_cpu4, p_ve, p_gpu);

	sunxi_get_module_param_from_sid(&tmp, SUN55IW3_ICPU_EFUSE_OFF, 4);
	tmp = (tmp >> 12) & 0xfff;
	icpu_d = tmp & 0x3ff;
	tmp = tmp >> 10;
	icpu_n = 4;
	while (i < tmp) {
		icpu_n *= 4;
		i++;
	}

	icpu = icpu_d/icpu_n;
	tmp = sizeof(ps_icpu_info)/sizeof(struct ps_icpu_info_t);
	for (i = 0; i < tmp; i++) {
		if ((ps_icpu_info[i].ps_min <= p_min)
			&& (p_min < ps_icpu_info[i].ps_max)
			&& (ps_icpu_info[i].icpu_min <= icpu)
			&& (icpu < ps_icpu_info[i].icpu_max)) {
			index = ps_icpu_info[i].index;
			break;
		}
	}

	sunxi_debug(NULL, "dvfs i: %u, P[%u, %u, %u], I[%u, %u, %u]\n", i, p_cpu4, p_ve, p_gpu,
		icpu_d, icpu_n, icpu);

	return index;
}

static void sun55iw3_nvmem_xlate(u32 *versions, char *name)
{
	u32 bak_dvfs, dvfs, combi;
	u32 index = 0x0100;

	sunxi_get_module_param_from_sid(&dvfs, SUN55IW3_DVFS_EFUSE_OFF, 4);
	bak_dvfs = (dvfs >> 12) & 0xff;
	if (bak_dvfs)
		combi = bak_dvfs;
	else
		combi = (dvfs >> 4) & 0xff;

	if (combi == 0x00)
		index = match_vf_table_ext();
	else
		match_vf_table(combi, &index);

	sunxi_set_vf_index(index);
	snprintf(name, MAX_NAME_LEN, "vf%04x", index);
	ver_data.dvfs_code = (u16)((dvfs >> 4) & 0xffff);
	get_vf_table_version();
	sunxi_debug(NULL, "dvfs: %s, 0x%x, %s\n", ver_data.vf_ver, ver_data.dvfs_code, name);
}

static struct cpufreq_soc_data sun55iw3_soc_data = {
	.nvmem_xlate = sun55iw3_nvmem_xlate,
};

#define SUN55IW6_DVFS_EFUSE_OFF     (0x20)
static void sun55iw6_nvmem_xlate(u32 *versions, char *name)
{
	u32 bak_dvfs, dvfs, combi;
	u32 index = 0x0100;

	sunxi_get_module_param_from_sid(&dvfs, SUN55IW6_DVFS_EFUSE_OFF, 4);
	bak_dvfs = (dvfs >> 24) & 0xff;
	if (bak_dvfs)
		combi = bak_dvfs;
	else
		combi = (dvfs >> 16) & 0xff;

	match_vf_table(combi, &index);

	sunxi_set_vf_index(index);
	snprintf(name, MAX_NAME_LEN, "vf%04x", index);
	ver_data.dvfs_code = (u16)((dvfs >> 4) & 0xffff);
	get_vf_table_version();
	sunxi_debug(NULL, "dvfs: %s, 0x%x, %s\n", ver_data.vf_ver, ver_data.dvfs_code, name);
}

static struct cpufreq_soc_data sun55iw6_soc_data = {
	.nvmem_xlate = sun55iw6_nvmem_xlate,
};

u32 determine_dcxo_clk_source(void)
{
	struct clk *dcxo_rate;
	u32 clock_rate;
	dcxo_rate = clk_get(NULL, "dcxo");
	if (IS_ERR(dcxo_rate)) {
		sunxi_err(NULL, "failed to get dcxo clock source\n");
		return 0;
	}

	clock_rate = clk_get_rate(dcxo_rate);
	clk_put(dcxo_rate);
	if (!clock_rate) {
		sunxi_err(NULL, "failed to get cpu clock rate\n");
		return 0;
	}

	return clock_rate;
}
EXPORT_SYMBOL_GPL(determine_dcxo_clk_source);

#define DCXO_CLK_26M     (26000000)
static void sun60iw2_nvmem_xlate(u32 *versions, char *name)
{
	u32 dvfs;
	u32 index = 0x0000;
	u32 clock_rate;

	if (sunxi_get_soc_dvfs(&dvfs))
		sunxi_err(NULL, "failed to get soc dvfs, use default vf table\n");

	match_vf_table(dvfs, &index);

	sunxi_set_vf_index(index);

	/* Determine whether to use 26m or 24m */
	clock_rate = determine_dcxo_clk_source();
	if (clock_rate == DCXO_CLK_26M)
		snprintf(name, MAX_NAME_LEN, "26m-vf%04x", index);
	else
		snprintf(name, MAX_NAME_LEN, "vf%04x", index);

	ver_data.dvfs_code = (u16)(dvfs & 0xffff);
	get_vf_table_version();
	sunxi_debug(NULL, "dvfs: %s, 0x%x, %s\n", ver_data.vf_ver, ver_data.dvfs_code, name);
}

static struct cpufreq_soc_data sun60iw2_soc_data = {
	.nvmem_xlate = sun60iw2_nvmem_xlate,
};

static const char sun8iw21_ic_index_list[][32] = {
	{"allwinner,qg3101"},
	{"allwinner,v851s"},
	{"allwinner,v851se"},
	{"allwinner,v853s"},
	{"allwinner,r853s"},
	{"allwinner,v853"},
	{"allwinner,r853"},
	{"allwinner,v851s3"},
};

static unsigned int sun8iw21_get_ic_index(void)
{
	int index;
	for (index = 0; index < ARRAY_SIZE(sun8iw21_ic_index_list); index++) {
		if (of_machine_is_compatible(sun8iw21_ic_index_list[index]) > 0) {
			return index+1;
		}
	}
	return 0;

}

#define SUN8IW21_DVFS_EFUSE_OFF     (0x18)
static void sun8iw21_nvmem_xlate(u32 *versions, char *name)
{
	int value = 0;
	u32 dvfs, SID18, ic_index, vf_tbl_sel;
	struct device_node *np;
	sunxi_get_module_param_from_sid(&dvfs, SUN8IW21_DVFS_EFUSE_OFF, 4);
	SID18 = (dvfs >> 24) & 0x1;
	ic_index = sun8iw21_get_ic_index();
	vf_tbl_sel = 0;
	np = of_find_node_by_path("/cpus/cpu@0");
	if (np)
		of_property_read_u32(np, "vf_tbl_sel", &vf_tbl_sel);

	switch (ic_index) {
	case 1:
		if (vf_tbl_sel == 0)
			value = 1;
		else
			value = 2;
		break;
	case 2:
	case 3:
		if (vf_tbl_sel == 0)
			value = 3;
		else
			value = 4;
		break;
	case 4:
	case 5:
		if (vf_tbl_sel == 0)
			value = 5;
		else if (vf_tbl_sel == 1)
			value = 6;
		else if (vf_tbl_sel == 2)
			value = 10;
		else if (vf_tbl_sel == 3)
			value = 11;
		else
			value = 5;
		break;
	case 6:
	case 7:
		if (vf_tbl_sel == 0) {
			if (SID18 == 1) {
				value = 7;
			} else {
				value = 8;
			}
		} else {
			value = 9;
		}
		break;
	case 8:
		if (vf_tbl_sel == 0)
			value = 3;
		else
			value = 4;
		break;
	default:
		value = 0;
	}
	/* printk("\nline:%d %s P0:%d, Isys:%d vf_tbl_sel:%d, ic_index:%d value:%d\n\n", __LINE__, __func__, P0, Isys, vf_tbl_sel, ic_index, value); */
	*versions = (1 << value);
	/* snprintf(name, MAX_NAME_LEN, "a%d", value); */

}

static struct cpufreq_soc_data sun8iw21_soc_data = {
	.nvmem_xlate = sun8iw21_nvmem_xlate,
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

	if (soc_data->has_nvmem_speed) {
		ret = sun50i_nvmem_get_data("speed", &ver_data.nv_speed);
		if (ret)
			return ret;
	}

	if (soc_data->has_nvmem_Icpu) {
		ret = sun50i_nvmem_get_data("Icpu", &ver_data.nv_Icpu);
		if (ret)
			return ret;
	}

	if (soc_data->has_nvmem_bin) {
		ret = sun50i_nvmem_get_data("bin", &ver_data.nv_bin);
		if (ret)
			return ret;
	}

	if (soc_data->has_nvmem_extend_bin) {
		ret = sun50i_nvmem_get_data("bin_ext", &ver_data.nv_bin_ext);
		if (ret)
			return ret;
	}

	soc_data->nvmem_xlate(versions, name);

	return 0;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
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
	if (!match) {
		kfree(opp_tables);
		return -EINVAL;
	}

	ret = sun50i_cpufreq_get_efuse(match->data,
				       &ver_data.version, ver_data.name);
	if (ret) {
		kfree(opp_tables);
		return ret;
	}

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
				sunxi_err(NULL, "Failed to set prop name\n");
				goto free_opp;
			}
		}

		if (ver_data.version) {
			opp_tables[cpu] = dev_pm_opp_set_supported_hw(cpu_dev,
							  &ver_data.version, 1);
			if (IS_ERR(opp_tables[cpu])) {
				ret = PTR_ERR(opp_tables[cpu]);
				sunxi_err(NULL, "Failed to set hw\n");
				goto free_opp;
			}
		}
	}

	ret = class_register(&cpufreq_class);
	if (ret) {
		sunxi_err(NULL, "failed to cpufreq class register, ret = %d\n", ret);
		goto free_opp;;
	}

	cpufreq_dt_pdev = platform_device_register_simple("cpufreq-dt", -1,
							  NULL, 0);
	if (!IS_ERR(cpufreq_dt_pdev)) {
		platform_set_drvdata(pdev, opp_tables);
		return 0;
	}

	ret = PTR_ERR(cpufreq_dt_pdev);
	sunxi_err(NULL, "Failed to register platform device\n");

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
#else
static int sun50i_cpufreq_nvmem_probe(struct platform_device *pdev)
{
	const struct of_device_id *match;
	int *opp_tokens;
	unsigned int cpu;
	int ret;

	opp_tokens = kcalloc(num_possible_cpus(), sizeof(*opp_tokens),
			     GFP_KERNEL);
	if (!opp_tokens)
		return -ENOMEM;

	match = dev_get_platdata(&pdev->dev);
	if (!match) {
		kfree(opp_tokens);
		return -EINVAL;
	}

	ret = sun50i_cpufreq_get_efuse(match->data,
				       &ver_data.version, ver_data.name);
	if (ret) {
		kfree(opp_tokens);
		return ret;
	}

	for_each_possible_cpu(cpu) {
		struct device *cpu_dev = get_cpu_device(cpu);

		if (!cpu_dev) {
			ret = -ENODEV;
			goto free_opp;
		}

		if (strlen(ver_data.name)) {
			opp_tokens[cpu] = dev_pm_opp_set_prop_name(cpu_dev,
								   ver_data.name);
			if (opp_tokens[cpu] < 0) {
				ret = opp_tokens[cpu];
				sunxi_err(NULL, "Failed to set prop name\n");
				goto free_opp;
			}
		}

		if (ver_data.version) {
			opp_tokens[cpu] = dev_pm_opp_set_supported_hw(cpu_dev,
							  &ver_data.version, 1);
			if (opp_tokens[cpu] < 0) {
				ret = opp_tokens[cpu];
				sunxi_err(NULL, "Failed to set hw\n");
				goto free_opp;
			}
		}
	}

	ret = class_register(&cpufreq_class);
	if (ret) {
		sunxi_err(NULL, "failed to cpufreq class register, ret = %d\n", ret);
		goto free_opp;;
	}

	cpufreq_dt_pdev = platform_device_register_simple("cpufreq-dt", -1,
							  NULL, 0);
	if (!IS_ERR(cpufreq_dt_pdev)) {
		platform_set_drvdata(pdev, opp_tokens);
		return 0;
	}

	ret = PTR_ERR(cpufreq_dt_pdev);
	sunxi_err(NULL, "Failed to register platform device\n");

free_opp:
	for_each_possible_cpu(cpu)
		dev_pm_opp_put_prop_name(opp_tokens[cpu]);
	kfree(opp_tokens);

	return ret;
}

static int sun50i_cpufreq_nvmem_remove(struct platform_device *pdev)
{
	int *opp_tokens = platform_get_drvdata(pdev);
	unsigned int cpu;

	platform_device_unregister(cpufreq_dt_pdev);

	for_each_possible_cpu(cpu) {
		if (strlen(ver_data.name))
			dev_pm_opp_put_prop_name(opp_tokens[cpu]);

		if (ver_data.version)
			dev_pm_opp_put_prop_name(opp_tokens[cpu]);
	}
	kfree(opp_tokens);

	return 0;
}
#endif

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
	{ .compatible = "arm,sun8iw20p1", .data = &sun8iw20_soc_data, },
	{ .compatible = "arm,sun20iw1p1", .data = &sun20iw1_soc_data, },
	{ .compatible = "arm,sun55iw3p1", .data = &sun55iw3_soc_data, },
	{.compatible = "arm,sun8iw21p1", .data = &sun8iw21_soc_data,},
	{.compatible = "arm,sun55iw6p1", .data = &sun55iw6_soc_data,},
	{.compatible = "arm,sun60iw2p1", .data = &sun60iw2_soc_data,},
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
MODULE_AUTHOR("panzhijian <panzhijian@allwinnertech.com>");
MODULE_VERSION("1.0.0");

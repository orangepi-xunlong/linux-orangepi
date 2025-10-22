// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2017 Chen-Yu Tsai <wens@csie.org>
 */

#include <linux/clk-provider.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include "ccu_sdm.h"

struct clk_sdm_info *sdm_info;
static int len;

int sunxi_parse_sdm_info(struct device_node *node)
{
	int i = 0;
	struct device_node *child_node = NULL;
	struct device_node *sdm_node = NULL;

	sdm_node = of_get_child_by_name(node, "sdm_info");
	if (!sdm_node)
		return -ENODEV;

	len = of_get_child_count(sdm_node);
	if (!len)
		return -EINVAL;

	if (sdm_info) {
		sunxi_err(NULL, "sdm_info has been allocated!");
		return -EBUSY;
	}
	sdm_info = kcalloc(len, sizeof(*sdm_info), GFP_KERNEL);
	if (!sdm_info) {
		len = 0;
		return -ENOMEM;
	}

	for_each_child_of_node(sdm_node, child_node) {
		if (of_property_read_u32(child_node, "sdm-enable", &sdm_info[i].sdm_enable)) {
			sunxi_err(NULL, "%s fail to parse sdm-enable, please check dts config!\n", child_node->name);
			continue;
		}
		if (sdm_info[i].sdm_enable != DTS_SDM_OFF &&
				sdm_info[i].sdm_enable != DTS_SDM_ON) {
				sunxi_err(NULL, "clk: %s enable: %d\n",
					child_node->name, sdm_info[i].sdm_enable);
			continue;
		}

		if (of_property_read_u32(child_node, "sdm-factor", &sdm_info[i].sdm_factor)) {
			sunxi_err(NULL, "%s fail to parse sdm-factor, please check dts config!\n", child_node->name);
			continue;
		}
		/* the unit of factor is 1/1000 */
		if (sdm_info[i].sdm_factor > 99) {
			sunxi_err(NULL, "clk: %s invaild factor: %d, should be within 99\n",
					child_node->name, sdm_info[i].sdm_factor);
			continue;
		}

		if (of_property_read_u32(child_node, "freq-mode", &sdm_info[i].freq_mode)) {
			sdm_info[i].freq_mode = TR_N;
		} else {
			if (sdm_info[i].freq_mode < 0 || sdm_info[i].freq_mode > 1) {
				sunxi_err(NULL, "clk: %s invaild freq_mod: %d, should be within 0~1, use default TR_N\n",
						child_node->name, sdm_info[i].freq_mode);
				sdm_info[i].freq_mode = TR_N;
			}
		}

		if (of_property_read_u32(child_node, "sdm-freq", &sdm_info[i].sdm_freq)) {
			sdm_info[i].sdm_freq = FREQ_31_5;
		} else {
			if (sdm_info[i].sdm_freq < 0 || sdm_info[i].sdm_freq > 3) {
				sunxi_err(NULL, "clk: %s invaild sdm_freq : %d, should be within 0~3, use default FREQ_31_5\n", child_node->name, sdm_info[i].sdm_freq);
				sdm_info[i].sdm_freq = FREQ_31_5;
			}
		}

		sdm_info[i].clk_name = child_node->name;

		sunxi_info(NULL, "%s: The %s has enabled Spread Spectrum in DTS\n", __func__, child_node->name);
		sunxi_info(NULL, "%s: sdm-enable: %d sdm-factor: %d freq-mode: %d sdm-freq: %d \n", __func__, sdm_info[i].sdm_enable, sdm_info[i].sdm_factor, sdm_info[i].freq_mode, sdm_info[i].sdm_freq);
		i++;
	}
	len = i;
	return 0;
}
EXPORT_SYMBOL_GPL(sunxi_parse_sdm_info);

struct clk_sdm_info *sunxi_clk_get_sdm_info(const char *clk_name)
{
	int i;
	struct clk_sdm_info *sdm = NULL;

	if (!sdm_info)
		return NULL;

	for (i = 0; i < len; i++) {
		if (!strcmp(clk_name, sdm_info[i].clk_name)) {
			sdm = &sdm_info[i];
			break;
		}
	}
	return sdm;
}
EXPORT_SYMBOL_GPL(sunxi_clk_get_sdm_info);

u32 ccu_get_sdmval(unsigned long rate, struct ccu_common *common, u32 m, u32 n)
{
	u32 sdm_val, sdm_freq, step_value;
	u64 x2, wave_step;
	struct clk_sdm_info *sdm_info = common->sdm_info;

	sdm_freq = 315 + sdm_info->sdm_freq * 5;

	x2 = sdm_info->sdm_factor * (n + 1);
	if (x2 >= 1000) {
		sunxi_err(NULL, "clk: invalid sdm_factor: %d\n", sdm_info->sdm_factor);
		return -1;
	}
	/*
	 * 1. m=0
	 *	SDM_CLK_SEL->24M
	 *	fix coefficient=2^17*2/24 = 10922.5
	 * 2. m=1
	 *	SDM_CLK_SEL->12M
	 *	fix coefficient=02^17*2/12 = 21845
	 **/
	if (m)
		wave_step = 109225 * x2 * sdm_freq;
	else
		wave_step = 218450 * x2 * sdm_freq;

	do_div(wave_step, 100000000);
	step_value = (u32)wave_step;

	sdm_val = (wave_step << 20);
	/* enanle SDM */
	sdm_val = SET_BITS(31, 1, sdm_val, 1);
	/* choose freq_mode */
	sdm_val = SET_BITS(29, 2, sdm_val, sdm_info->freq_mode << 1);
	/* choose sdm_freq */
	sdm_val = SET_BITS(17, 2, sdm_val, sdm_info->sdm_freq);
	/* choose sdm_clk */
	sdm_val = SET_BITS(19, 1, sdm_val, m);

	sunxi_debug(NULL, "sdm_val: 0x%x wave_stop: %llu, sdm_freq: %d freq_mode: %d\n", sdm_val, wave_step, sdm_info->sdm_freq, sdm_info->freq_mode);

	return sdm_val;
}

void ccu_common_set_sdm_value(struct ccu_common *common, struct ccu_sdm_internal *sdm, u32 sdmval)
{
	if (sdm->enable)
		set_bits(common->base + common->reg, sdm->enable);

	set_field(common->base + sdm->tuning_reg, BITS_WIDTH(0, 32), sdmval);

	if (sdm->pattern1_reg)
		set_field(common->base + sdm->pattern1_reg, BITS_WIDTH(0, 32), sdm->pattern1_enable);
}

bool ccu_sdm_helper_is_enabled(struct ccu_common *common,
			       struct ccu_sdm_internal *sdm)
{
	if (!(common->features & CCU_FEATURE_SIGMA_DELTA_MOD))
		return false;

	if (sdm->enable && !(readl(common->base + common->reg) & sdm->enable))
		return false;

	if (sdm->pattern1_reg) {
		if (((readl(common->base + sdm->pattern1_reg) & sdm->pattern1_enable)) != sdm->pattern1_enable)
			return false;
	}

	if (sdm->tuning_enable && !(readl(common->base + sdm->tuning_reg) & sdm->tuning_reg))
		return false;

	return true;
}

void ccu_sdm_helper_enable(struct ccu_common *common,
			   struct ccu_sdm_internal *sdm,
			   u64 rate)
{
	unsigned long flags;
	unsigned int i;
	u32 reg;

	if (!(common->features & CCU_FEATURE_SIGMA_DELTA_MOD))
		return;

	/* Make sure SDM is enabled */
	spin_lock_irqsave(common->lock, flags);
	reg = readl(common->base + common->reg);
	if (sdm->enable)
		writel(reg | sdm->enable, common->base + common->reg);

	spin_unlock_irqrestore(common->lock, flags);

	/* Set the pattern */
	for (i = 0; i < sdm->table_size; i++) {
		if (sdm->table[i].rate == rate) {
			if (!sdm->table[i].pattern)
				return;
			writel(sdm->table[i].pattern,
			       common->base + sdm->tuning_reg);
		}
	}

	/* Make sure SDM is enabled */
	spin_lock_irqsave(common->lock, flags);
	reg = readl(common->base + sdm->tuning_reg);
	writel(reg | sdm->tuning_enable, common->base + sdm->tuning_reg);
	if (sdm->pattern1_reg) {
		reg = readl(common->base + sdm->pattern1_reg);
		writel(reg | sdm->pattern1_enable, common->base + sdm->pattern1_reg);
	}
	spin_unlock_irqrestore(common->lock, flags);
}

void ccu_sdm_helper_disable(struct ccu_common *common,
			    struct ccu_sdm_internal *sdm)
{
	unsigned long flags;
	u32 reg;

	if (!(common->features & CCU_FEATURE_SIGMA_DELTA_MOD))
		return;

	spin_lock_irqsave(common->lock, flags);
	reg = readl(common->base + common->reg);
	if (sdm->enable)
		writel(reg & ~sdm->enable, common->base + common->reg);

	spin_unlock_irqrestore(common->lock, flags);

	spin_lock_irqsave(common->lock, flags);
	reg = readl(common->base + sdm->tuning_reg);
	writel(reg & ~sdm->tuning_enable, common->base + sdm->tuning_reg);
	if (sdm->pattern1_reg) {
		reg = readl(common->base + sdm->pattern1_reg);
		writel(reg & ~sdm->pattern1_enable, common->base + sdm->pattern1_reg);
	}
	spin_unlock_irqrestore(common->lock, flags);
}

/*
 * Sigma delta modulation provides a way to do fractional-N frequency
 * synthesis, in essence allowing the PLL to output any frequency
 * within its operational range. On earlier SoCs such as the A10/A20,
 * some PLLs support this. On later SoCs, all PLLs support this.
 *
 * The datasheets do not explain what the "wave top" and "wave bottom"
 * parameters mean or do, nor how to calculate the effective output
 * frequency. The only examples (and real world usage) are for the audio
 * PLL to generate 24.576 and 22.5792 MHz clock rates used by the audio
 * peripherals. The author lacks the underlying domain knowledge to
 * pursue this.
 *
 * The goal and function of the following code is to support the two
 * clock rates used by the audio subsystem, allowing for proper audio
 * playback and capture without any pitch or speed changes.
 */
bool ccu_sdm_helper_has_rate(struct ccu_common *common,
			     struct ccu_sdm_internal *sdm,
			     u64 rate)
{
	unsigned int i;

	if (!(common->features & CCU_FEATURE_SIGMA_DELTA_MOD))
		return false;

	for (i = 0; i < sdm->table_size; i++)
		if (sdm->table[i].rate == rate)
			return true;

	return false;
}

unsigned long ccu_sdm_helper_read_rate(struct ccu_common *common,
				       struct ccu_sdm_internal *sdm,
				       u32 m, u32 n)
{
	unsigned int i;
	u32 reg;

	sunxi_debug(NULL, "%s: Read sigma-delta modulation setting\n",
		 clk_hw_get_name(&common->hw));

	if (!(common->features & CCU_FEATURE_SIGMA_DELTA_MOD))
		return 0;

	sunxi_debug(NULL, "%s: clock is sigma-delta modulated\n",
		 clk_hw_get_name(&common->hw));

	reg = readl(common->base + sdm->tuning_reg);

	sunxi_debug(NULL, "%s: pattern reg is 0x%x",
		 clk_hw_get_name(&common->hw), reg);

	for (i = 0; i < sdm->table_size; i++)
		if (sdm->table[i].pattern == reg &&
		    sdm->table[i].m == m && sdm->table[i].n == n)
			return sdm->table[i].rate;

	/* We can't calculate the effective clock rate, so just fail. */
	return 0;
}

int ccu_sdm_helper_get_factors(struct ccu_common *common,
			       struct ccu_sdm_internal *sdm,
			       u64 rate,
			       unsigned long *m, unsigned long *n)
{
	unsigned int i;

	if (!(common->features & CCU_FEATURE_SIGMA_DELTA_MOD))
		return -EINVAL;

	for (i = 0; i < sdm->table_size; i++)
		if (sdm->table[i].rate == rate) {
			*m = sdm->table[i].m;
			*n = sdm->table[i].n;
			return 0;
		}

	/* nothing found */
	return -EINVAL;
}

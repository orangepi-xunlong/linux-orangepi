/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 *
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#ifndef _PHY_TOP_H
#define _PHY_TOP_H

#include <linux/kernel.h>
#include <linux/slab.h>

/**
 * @desc: if new platform, you need add new version num.
 */
typedef enum {
	TOP_PHY_V0 = 0,
	TOP_PHY_V1 = 1, /* sun55iw3 */
	TOP_PHY_V2 = 2, /* sun60iw2 */
} top_phy_version_t;

typedef enum {
	SUNXI_TOP_PHY_POWER_ON  = 0,
	SUNXI_TOP_PHY_POWER_OFF = 1,
	SUNXI_TOP_PHY_POWER_LOW = 2,
} top_phy_power_t;

union REG_0000_t {
	u32 dwval;
	struct { /* phy config 0 */
		u32 phy_reset:1;
		u32 phy_pddq:1;
		u32 phy_txpwron:1;
		u32 phy_svsret_mode:1;
		u32 phy_enhpdrxsense:1;
		u32 res0:3;
		u32 phy_cont_en:1;
		u32 phy_bisten:1;
		u32 res1:21;
		u32 phyctl_external:1;
	} sun60i;
};

union REG_0004_t {
	u32 dwval;
	struct {
		u32 hdmi_pad_sel:1;
		u32 res0:31;
	} sun60i;
};

union REG_0010_t {
	u32 dwval;
	struct {
		u32 phy_txready:1;
		u32 phy_snkdet:1;
		u32 phy_rxsense:1;
		u32 res0:1;
		u32 phy_dtb:2;
		u32 res1:2;
		u32 phy_bistdone:1;
		u32 phy_bistok:1;
		u32 res2:22;
	} sun60i;
};

union REG_0014_t {
	u32 dwval;
	struct {
		u32 phy_cont_data:10;
		u32 res0:22;
	} sun60i;
};

union REG_0020_t {
	u32 dwval;
	struct {
		u32 res0:1;
		u32 pll_input_div2:1;
		u32 res1:3;
		u32 pll_lock_model:1;
		u32 pll_unlock_model:2;
		u32 pll_n:8;
		u32 pll_p0:7;
		u32 res2:4;
		u32 pll_output_gate:1;
		u32 res3:1;
		u32 lock_enable:1;
		u32 pll_ldo_en:1;
		u32 pll_en:1;
	} sun60i;
};

union REG_0024_t {
	u32 dwval;
	struct {
		u32 res0:7;
		u32 pll_level_shifter_gate:1;
		u32 hdmi_outclk_sel:1;
		u32 pll_unlock_irq_en:1;
		u32 res1:22;
	} sun60i;
};

union REG_0028_t {
	u32 dwval;
	struct {
		u32 test_en:1;
		u32 st:1;
		u32 sdiv:2;
		u32 pad_out_en:1;
		u32 res0:3;
		u32 ck_test_sel:1;
		u32 res1:3;
		u32 vco_start_en:1;
		u32 mbias_en:1;
		u32 ldo_vset:3;
		u32 ldo_enable:1;
		u32 res2:14;
	} sun60i;
};

union REG_002C_t {
	u32 dwval;
	struct {
		u32 wave_bot:17;
		u32 freq:2;
		u32 sdm_direction:1;
		u32 wave_step:9;
		u32 spr_freq_mode:2;
		u32 sig_delt_pat_en:1;
	} sun60i;
};

union REG_0030_t {
	u32 dwval;
	struct {
		u32 frac_in:17;
		u32 res0:3;
		u32 frac_en:1;
		u32 res1:3;
		u32 dither_en:1;
		u32 smooth_en:1;
		u32 res2:1;
		u32 pll_sdm_en:1;
		u32 pll_pi_cfg:3;
		u32 pll_pi_en:1;
	} sun60i;
};

union REG_0034_t {
	u32 dwval;
	struct {
		u32 res0:16;
		u32 pll_cp:5;
		u32 res1:11;
	} sun60i;
};

union REG_0040_t {
	u32 dwval;
	struct {
		u32 lock_status:1;
		u32 pll_hdmiphy_unlock_state:1;
		u32 sdm_busy:1;
		u32 res0:29;
	} sun60i;
};

struct top_phy_regs {
	/* 0x0000~0x000C */
	union REG_0000_t		reg_0000;
	union REG_0004_t		reg_0004;
	u32 regs0[2];
	/* 0x0010~0x001C */
	union REG_0010_t		reg_0010;
	union REG_0014_t		reg_0014;
	u32 regs1[2];
	/* 0x0020~0x002C */
	union REG_0020_t		reg_0020;
	union REG_0024_t		reg_0024;
	union REG_0028_t		reg_0028;
	union REG_002C_t		reg_002C;
	/* 0x0030~0x003C */
	union REG_0030_t		reg_0030;
	union REG_0034_t		reg_0034;
	u32 regs2[2];
	/* 0x0040~0x004C */
	union REG_0040_t		reg_0040;
	u32 regs3[3];
	/* end */
};

void top_phy_write(u32 offset, u32 data);
u32 top_phy_read(u32 offset);
u8 top_phy_get_clock_select(void);
void top_phy_set_clock_select(u8 sel);
u8 top_phy_get_pad_select(void);
void top_phy_set_pad_select(u8 sel);
u8 top_phy_pll_get_lock(void);
int top_phy_config(void);
void top_phy_power(top_phy_power_t type);
ssize_t top_phy_dump(char *buf);
int top_phy_init(void);

#endif /* _PHY_TOP_H */
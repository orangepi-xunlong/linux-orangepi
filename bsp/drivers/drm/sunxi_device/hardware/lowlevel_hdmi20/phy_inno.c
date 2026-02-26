/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/

#include <linux/delay.h>
#include <linux/slab.h>
#include <sunxi-sid.h>

#include "dw_dev.h"
#include "dw_phy.h"
#include "dw_mc.h"
#include "phy_inno.h"

#define INNO_PHY_TIMEOUT			1000
#define INNO_PHY_REG_OFFSET			0x10000

#define phy_diff(x, y)            ((x > y) ? (x - y) : (y - x))
#define phy_diff_range(x, y, r)   (phy_diff(x, y) <= r)

enum {
	INNO_PHY_VERSION_0 = 0,
	INNO_PHY_VERSION_1,
	INNO_PHY_VERSION_2,
};

struct inno_phy_pll {
	u32 pixel_clk;
	u32 tmds_clk;
	u32 pre_fbdiv;   /* fbdiv1 << 16 || fbdiv0 << 8 || prediv*/
	u32 pre_clkdiv0; /* tmdsdiv << 16 || linkdiv << 8 || linktmdsdiv */
	u32 pre_clkdiv1; /* auxclkdiv << 8 || mainclkdiv */
	u32 pre_clkdiv2; /* pclk_bp << 16 || pclk_div << 8 || refclk_div */
	u32 pre_fra;     /* fra_ctl << 24 || fra_div0 << 16 || fra_div0 << 8 || fra_div2 */
	u32 post_fbdiv;  /* diven << 24 || fbdiv0 << 16 || fbdiv1 << 8 || preddiv */
	u8  post_div;    /* postdiv */
};

struct inno_phy_drive {
	u32 min_clk; /* min tmds clock, KHz */
	u32 max_clk; /* max tmds clock, KHz */
	u32 cur_bias;
	u32 vlevel;
	u32 pre_empl;
	u32 post_empl;
};

struct inno_phy_s {
	struct inno_phy_pll *mpll;
	struct inno_phy_drive *drive;
	u32 mpll_size;
	u32 drive_size;
	u32 tmds_clock;
	u32 pixel_clock;
	u8 version;
};

static struct inno_phy_s inno_phy;
static DECLARE_WAIT_QUEUE_HEAD(phy_wq);

static volatile struct __inno_phy_reg_t *phy_base;

static struct inno_phy_drive drive_tab0[] = {
	{ 25200, 160000, 0x00020202, 0x1c1c1c1c, 0x00000000, 0x00000000}, /* [ 25200, 160000) */
	{160000, 300000, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x03030300}, /* [160000, 300000) */
	{300000, 600000, 0x020f0f0f, 0x1c1c1c1c, 0x00000000, 0x03030300}, /* [300000, 600000) */
};

static struct inno_phy_drive drive_tab1[] = {
	{ 25200, 160000, 0x01000102, 0x1c1c1c1c, 0x00000000, 0x02020200}, /* [ 25200, 160000) */
	{160000, 300000, 0x01040506, 0x1c1c1c1c, 0x00000000, 0x03030300}, /* [240000, 300000) */
	{300000, 600000, 0x010f0f0f, 0x1c1c1c1c, 0x00000000, 0x00000000}, /* [300000, 600000) */
};

static struct inno_phy_drive drive_tab2[] = {
	{ 25200, 160000, 0x01010102, 0x1c1c1c1c, 0x00000000, 0x00000000}, /* [ 25200, 160000) */
	{160000, 300000, 0x01020304, 0x1c1c1c1c, 0x00000000, 0x05050500}, /* [160000, 300000) */
	{300000, 600000, 0x010e0f0f, 0x1c1c1c1c, 0x00000000, 0x00000000}, /* [300000, 600000) */
};

static struct inno_phy_pll phy_mpll[] = {
	{25175,  25175,  0x00002901, 0x00010103, 0x00000103, 0x00000403, 0x005555f5, 0x03280001, 0x03},
	{25200,  25200,  0x00002a01, 0x00010103, 0x00000103, 0x00000403, 0x03000000, 0x03280000, 0x03},
	{27000,  27000,  0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03},
	{27000,  33750,  0x00008702, 0x00020202, 0x00000602, 0x00000503, 0x03000000, 0x03280001, 0x03},
	{36000,  36000,  0x00002401, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x03280001, 0x03},
	{40000,  40000,  0x00002801, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{54000,  54000,  0x00003601, 0x00010102, 0x00000300, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{65000,  65000,  0x00004101, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{68250,  68250,  0x00005b01, 0x00030300, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{71000,  71000,  0x00004701, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{72000,  72000,  0x00002401, 0x00000002, 0x00000101, 0x00000202, 0x03000000, 0x03140001, 0x01},
	/* pixel clock = 74.25M */
	{74250,  37125,  0x00006302, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03280001, 0x03},
	{74250,  46406,  0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01},
	{74250,  74250,  0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{74250,  92812,  0x00007B01, 0x00020201, 0x00000103, 0x00000403, 0x000000C0, 0x03140001, 0x01},
	{75000,  75000,  0x00003201, 0x00020200, 0x00000100, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{79500,  79500,  0x00003501, 0x00020200, 0x00000100, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{85500,  85500,  0x00003901, 0x00020200, 0x00000100, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{88750,  88750,  0x00002c01, 0x00000002, 0x00000101, 0x00000202, 0x00000060, 0x03140001, 0x01},
	{101000, 101000, 0x00006501, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{108000, 108000, 0x00002401, 0x00010100, 0x00000100, 0x00000202, 0x03000000, 0x030a0001, 0x00},
	{119000, 119000, 0x00007701, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	/* pixel clock = 148.5M */
	{148500,  74250, 0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{148500,  92812, 0x00007B01, 0x00020201, 0x00000103, 0x00000202, 0x000000C0, 0x03140001, 0x01},
	{148500, 148500, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00},
	{148500, 185625, 0x00007B01, 0x00010101, 0x00000500, 0x00000202, 0x000000C0, 0x03140002, 0x00},
	{154000, 154000, 0x00004d01, 0x00000002, 0x00000101, 0x00000202, 0x03000000, 0x030a0001, 0x00},
	{233500, 116750, 0x00007501, 0x00010102, 0x00000101, 0x00000202, 0x03000000, 0x030a0000, 0x00},
	{233500, 233500, 0x00007501, 0x00000002, 0x00000303, 0x00000202, 0x03000000, 0x030a0000, 0x00},
	{234000, 117000, 0x00007501, 0x00010102, 0x00000101, 0x00000202, 0x03000000, 0x030a0000, 0x00},
	{234000, 234000, 0x00007501, 0x00000002, 0x00000303, 0x00000202, 0x03000000, 0x030a0000, 0x00},
	/* pixel clock = 297M */
	{297000, 148500, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00},
	{297000, 185625, 0x00007B01, 0x00010101, 0x00000500, 0x00000101, 0x000000C0, 0x03140002, 0x00},
	{297000, 297000, 0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00},
	{297000, 371250, 0x00007B01, 0x00010300, 0x00000103, 0x00000101, 0x000000C0, 0x000a0002, 0x00},
	/* pixel clock = 594M */
	{594000, 297000, 0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00},
	{594000, 371250, 0x00007B01, 0x00010300, 0x00000103, 0x00000101, 0x000000C0, 0x000a0002, 0x00},
	{594000, 594000, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00},
};

static void inno_phy_set_version(void)
{
	unsigned int id = sunxi_get_soc_markid();
	unsigned int version = sunxi_get_soc_ver();

	if (version < 0x2) {
		inno_phy.version = INNO_PHY_VERSION_0; /* A/T - AB */
	} else {
		if ((id == 0x5000) || (id == 0x5100) || (id == 0x5500) || (id == 0x5700) ||
			(id == 0x5f00) || (id == 0x5f10) || (id == 0x5f30)) {
			inno_phy.version = INNO_PHY_VERSION_1; /* T - C */
		} else {
			inno_phy.version = INNO_PHY_VERSION_2; /* A/H - C */
		}
	}
}

static void _inno_phy_analog_reset(void)
{
	phy_base->hdmi_phy_ctl0_0.bits.rst_an = 0x0;
	mdelay(10);
	phy_base->hdmi_phy_ctl0_0.bits.rst_an = 0x1;
}

static void _inno_phy_digital_reset(void)
{
	phy_base->hdmi_phy_ctl0_0.bits.rst_di = 0x0;
	mdelay(10);
	phy_base->hdmi_phy_ctl0_0.bits.rst_di = 0x1;
}

/**
 * @state: 1: turn on; 0: turn off
 */
static void _inno_turn_dirver_ctrl(u8 state)
{
	phy_base->hdmi_phy_dr0_2.bits.ch0_dr_en = state;
	phy_base->hdmi_phy_dr0_2.bits.ch1_dr_en = state;
	phy_base->hdmi_phy_dr0_2.bits.ch2_dr_en = state;
	phy_base->hdmi_phy_dr0_2.bits.clk_dr_en = state;
}

/**
 * @state: 1: turn on; 0: turn off
 */
static void _inno_turn_serializer_ctrl(u8 state)
{
	phy_base->hdmi_phy_dr3_2.bits.ch0_seri_en = state;
	phy_base->hdmi_phy_dr3_2.bits.ch1_seri_en = state;
	phy_base->hdmi_phy_dr3_2.bits.ch2_seri_en = state;
}

/**
 * @state: 1: turn on; 0: turn off
 */
static void _inno_turn_ldo_ctrl(u8 state)
{
	phy_base->hdmi_phy_dr1_0.bits.clk_LDO_en = state;
	phy_base->hdmi_phy_dr1_0.bits.ch0_LDO_en = state;
	phy_base->hdmi_phy_dr1_0.bits.ch1_LDO_en = state;
	phy_base->hdmi_phy_dr1_0.bits.ch2_LDO_en = state;

	if (inno_phy.version != INNO_PHY_VERSION_0) {
		phy_base->hdmi_phy_dr2_1.bits.ch0_LDO_cur = state;
		phy_base->hdmi_phy_dr2_1.bits.ch1_LDO_cur = state;
		phy_base->hdmi_phy_dr2_1.bits.ch2_LDO_cur = state;
		hdmi_trace("inno phy turn %s LDO when phy version %d\n",
			state ? "on" : "off", inno_phy.version);
	}
}

/**
 * @state: 1: turn on; 0: turn off
*/
static void _inno_turn_pll_ctrl(u8 state)
{
	phy_base->hdmi_phy_pll2_2.bits.postpll_pow = !state;
	phy_base->hdmi_phy_pll0_0.bits.prepll_pow  = !state;
}

/**
 * @state: 1: turn on; 0: turn off
*/
static void _inno_turn_resense_ctrl(u8 state)
{
	phy_base->hdmi_phy_rxsen_esd_0.bits.ch0_rxsense_en = state;
	phy_base->hdmi_phy_rxsen_esd_0.bits.ch1_rxsense_en = state;
	phy_base->hdmi_phy_rxsen_esd_0.bits.ch2_rxsense_en = state;
	phy_base->hdmi_phy_rxsen_esd_0.bits.clk_rxsense_en = state;
}

/**
 * @state: 1: turn on; 0: turn off
 */
static void _inno_turn_biascircuit_ctrl(u8 state)
{
	phy_base->hdmi_phy_dr0_0.bits.bias_en = state;
}

/**
 * @state: 1: select resistor on-chip; 0: select resistor off-chip
 */
static void _inno_turn_resistor_ctrl(u8 state)
{
	phy_base->hdmi_phy_dr0_0.bits.refres = state;

	if (inno_phy.version != INNO_PHY_VERSION_0) {
		/* 0x00000100 off  0x00000033 on */
		*((u32 *)((void *)phy_base + 0x8004)) = 0x00000100;
	}
}

void _inno_phy_config_4k60(void)
{
	/* resence config */
	phy_base->hdmi_phy_dr5_2.bits.terrescal_clkdiv0 = 0xF0;
	phy_base->hdmi_phy_dr5_1.bits.terrescal_clkdiv1 = 0x0;//24M/240 = 100K

	if (inno_phy.version == INNO_PHY_VERSION_0) {
		/* config resistance_div */
		phy_base->hdmi_phy_dr6_0.bits.clkterres_ndiv = 0x28;
		phy_base->hdmi_phy_dr6_1.bits.ch2terres_ndiv = 0x28;
		phy_base->hdmi_phy_dr6_2.bits.ch1terres_ndiv = 0x28;
		phy_base->hdmi_phy_dr6_3.bits.ch0terres_ndiv = 0x28;
		hdmi_trace("inno phy config clkterres ndiv when phy version 0\n");
	}

	/* config resistance 100 */
	phy_base->hdmi_phy_dr5_3.bits.terres_val = 0x0;

	/* configure channel control register */
	phy_base->hdmi_phy_dr5_3.bits.ch2_terrescal = 0x1;
	phy_base->hdmi_phy_dr5_3.bits.ch1_terrescal = 0x1;
	phy_base->hdmi_phy_dr5_3.bits.ch0_terrescal = 0x1;

	/* config the calibration by pass */
	phy_base->hdmi_phy_dr5_1.bits.terrescal_bp = 0x1;
	udelay(5);
	phy_base->hdmi_phy_dr5_1.bits.terrescal_bp = 0x0;
}

void _inno_phy_config_4k30(void)
{
	if (inno_phy.version == INNO_PHY_VERSION_0)
		return ;

	/* resence config */
	phy_base->hdmi_phy_dr5_2.bits.terrescal_clkdiv0 = 0xF0;
	phy_base->hdmi_phy_dr5_1.bits.terrescal_clkdiv1 = 0x0;//24M/240 = 100K

	/* config resistance 200 */
	phy_base->hdmi_phy_dr5_3.bits.terres_val = 0x3;

	/* configure channel control register */
	phy_base->hdmi_phy_dr5_3.bits.ch2_terrescal = 0x1;
	phy_base->hdmi_phy_dr5_3.bits.ch1_terrescal = 0x1;
	phy_base->hdmi_phy_dr5_3.bits.ch0_terrescal = 0x1;

	/* config the calibration by pass */
	phy_base->hdmi_phy_dr5_1.bits.terrescal_bp = 0x1;
	udelay(5);
	phy_base->hdmi_phy_dr5_1.bits.terrescal_bp = 0x0;
}

/**
 * @desc: get inno phy all lane rxsense lock status
 * @return: 1: rxsense lock; 0: rxsense unlock
 */
static u8 _inno_phy_get_rxsense_lock(void)
{
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch0_rxsense_de_sta == 0x0)
		return 0x0;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch1_rxsense_de_sta == 0x0)
		return 0x0;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch2_rxsense_de_sta == 0x0)
		return 0x0;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.clk_rxsense_de_sta == 0x0)
		return 0x0;
	return 0x1;
}

/**
 * @desc: get inno phy one lane rxsense lock status
 * @return: 1: rxsense lock; 0: rxsense unlock
 */
static int _inno_phy_get_only_rxsense_lock(void)
{
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch0_rxsense_de_sta)
		return 0x1;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch1_rxsense_de_sta)
		return 0x1;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch2_rxsense_de_sta)
		return 0x1;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.clk_rxsense_de_sta)
		return 0x1;
	return 0x0;
}

/**
 * @desc: get inno phy pre pll and post pll lock status
 * @return: 1: pll lock; 0: pll unlock
 */
static u8 _inno_phy_get_pll_lock(void)
{
	if (phy_base->hdmi_phy_pll2_1.bits.prepll_lock_state == 0x0)
		return 0x0;
	if (phy_base->hdmi_phy_pll3_3.bits.postpll_lock_state == 0x0)
		return 0x0;
	return 0x1;
}

static void _inno_phy_cfg_cur_bias(u32 data)
{
	phy_base->hdmi_phy_dr4_0.bits.ch0_cur_bias = dword_to_byte(data, 0);
	phy_base->hdmi_phy_dr4_0.bits.ch1_cur_bias = dword_to_byte(data, 1);
	phy_base->hdmi_phy_dr3_3.bits.ch2_cur_bias = dword_to_byte(data, 2);
	phy_base->hdmi_phy_dr3_3.bits.clk_cur_bias = dword_to_byte(data, 3);
}

static void _inno_phy_cfg_vlevel(u32 data)
{
	phy_base->hdmi_phy_dr1_1.bits.clk_vlevel = dword_to_byte(data, 0);
	phy_base->hdmi_phy_dr1_2.bits.ch2_vlevel = dword_to_byte(data, 1);
	phy_base->hdmi_phy_dr1_3.bits.ch1_vlevel = dword_to_byte(data, 2);
	phy_base->hdmi_phy_dr2_0.bits.ch0_vlevel = dword_to_byte(data, 3);
}

static void _inno_phy_cfg_pre_empl(u32 data)
{
	phy_base->hdmi_phy_dr0_3.bits.clk_pre_empl = dword_to_byte(data, 0);
	phy_base->hdmi_phy_dr2_3.bits.ch2_pre_empl = dword_to_byte(data, 1);
	phy_base->hdmi_phy_dr3_0.bits.ch1_pre_empl = dword_to_byte(data, 2);
	phy_base->hdmi_phy_dr3_1.bits.ch0_pre_empl = dword_to_byte(data, 3);
}

static void _inno_phy_cfg_post_empl(u32 data)
{
	phy_base->hdmi_phy_dr0_3.bits.clk_post_empl = dword_to_byte(data, 0);
	phy_base->hdmi_phy_dr2_3.bits.ch2_post_empl = dword_to_byte(data, 1);
	phy_base->hdmi_phy_dr3_0.bits.ch1_post_empl = dword_to_byte(data, 2);
	phy_base->hdmi_phy_dr3_1.bits.ch0_post_empl = dword_to_byte(data, 3);
}

static void _inno_phy_enable_data_sync(void)
{
	phy_base->hdmi_phy_ctl0_2.bits.data_sy_ctl = 0x0;
	mdelay(1);
	phy_base->hdmi_phy_ctl0_2.bits.data_sy_ctl = 0x1;
}

static void _inno_phy_all_rst(void)
{
	dw_phy_config_svsret();

	dw_mc_sw_reset(DW_MC_SWRST_PHY, 0x0);
	udelay(5);
	dw_mc_sw_reset(DW_MC_SWRST_PHY, 0x1);

	_inno_phy_analog_reset();
	udelay(5);
	_inno_phy_digital_reset();
	hdmi_trace("inno phy reset done\n");
}

static void _inno_phy_output_en(void)
{
	_inno_turn_ldo_ctrl(0x1);
	_inno_turn_serializer_ctrl(0x1);
	_inno_turn_dirver_ctrl(0x1);

	udelay(5);
	_inno_phy_digital_reset();

	udelay(5);
	_inno_phy_enable_data_sync();
}

static int _inno_phy_wait_pll_lock(void)
{
	int ret = 0;

	ret = wait_event_timeout(phy_wq, _inno_phy_get_pll_lock(),
				msecs_to_jiffies(10));
	if (ret == 0) {
		hdmi_err("inno phy wait pre-pll and post-pll lock timeout\n");
		return -1;
	}
	return 0;
}

static int _inno_phy_wait_rxsense(void)
{
	int ret = 0;

	ret = wait_event_timeout(phy_wq, _inno_phy_get_rxsense_lock(),
				msecs_to_jiffies(10));
	if (ret > 0)
		return 0;

	ret = wait_event_timeout(phy_wq, _inno_phy_get_only_rxsense_lock(),
				msecs_to_jiffies(10));
	if (ret > 0)
		return 0;

	return -1;
}

static int _inno_phy_config_drive(void)
{
	struct inno_phy_drive *table = inno_phy.drive;
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	u32 tmds_clk = hdmi->tmds_clk;
	int i = 0;

	for (i = 0; i < inno_phy.drive_size; i++) {
		if (table[i].min_clk == table[i].max_clk) {
			if (tmds_clk != table[i].min_clk)
				continue;
		} else {
			if (tmds_clk < table[i].min_clk)
				continue;
			if (tmds_clk >= table[i].max_clk)
				continue;
		}

		hdmi_inf("inno phy config drive:\n");
		hdmi_inf(" - %dKHz [%dKHz~%dKHz]: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			tmds_clk, table[i].min_clk, table[i].max_clk,
			table[i].cur_bias, table[i].vlevel,
			table[i].pre_empl, table[i].post_empl);

		_inno_phy_cfg_cur_bias(table[i].cur_bias);
		_inno_phy_cfg_vlevel(table[i].vlevel);
		_inno_phy_cfg_pre_empl(table[i].pre_empl);
		_inno_phy_cfg_post_empl(table[i].post_empl);

		return 0;
	}

	hdmi_err("inno phy %dKHz clock can't get drive params!\n", tmds_clk);
	return -1;
}

static void _inno_phy_turn_off(void)
{
	_inno_turn_dirver_ctrl(0x0);
	_inno_turn_serializer_ctrl(0x0);
	_inno_turn_ldo_ctrl(0x0);
	_inno_turn_pll_ctrl(0x0);
	_inno_turn_resense_ctrl(0x0);
	_inno_turn_biascircuit_ctrl(0x0);
}

static void _inno_phy_turn_on(void)
{
	int ret = 0;

	_inno_turn_biascircuit_ctrl(0x1);

	_inno_turn_resistor_ctrl(0x0);

	_inno_turn_resense_ctrl(0x1);
	udelay(1);
	ret = _inno_phy_wait_rxsense();
	if (ret != 0)
		hdmi_wrn("inno phy wait rxsense timeout!\n");
}

static int _inno_phy_config_pll(void)
{
	int i = 0, index = 0;

	if (inno_phy.tmds_clock == 594000)
		_inno_phy_config_4k60();
	else if (inno_phy.tmds_clock == 297000)
		_inno_phy_config_4k30();

	/* 1. find tmds&pixel macth mpll params */
	for (i = 0; i < inno_phy.mpll_size; i++) {
		if (phy_diff_range(inno_phy.tmds_clock, inno_phy.mpll[i].tmds_clk, 0) &&
				phy_diff_range(inno_phy.pixel_clock, inno_phy.mpll[i].pixel_clk, 0)) {
			index = i;
			goto mpll_cfg;
		}
	}

	/* 2. find tmds&pixel diff 100KHz mpll params */
	for (i = 0; i < inno_phy.mpll_size; i++) {
		if (phy_diff_range(inno_phy.tmds_clock, inno_phy.mpll[i].tmds_clk, 100) &&
				phy_diff_range(inno_phy.pixel_clock, inno_phy.mpll[i].pixel_clk, 100)) {
			index = i;
			goto mpll_cfg;
		}
	}

	/* 3.find near tmds mpll params */
	for (i = 0; i < (inno_phy.mpll_size - 1); i++) {
		if (inno_phy.tmds_clock < inno_phy.mpll[i].tmds_clk)
			continue;
		if (inno_phy.tmds_clock > inno_phy.mpll[i + 1].tmds_clk)
			continue;

		if (phy_diff(inno_phy.tmds_clock, inno_phy.mpll[i].tmds_clk) >
				phy_diff(inno_phy.tmds_clock, inno_phy.mpll[i + 1].tmds_clk))
			index = i + 1;
		else
			index = i;
		hdmi_wrn("inno phy use tmds clock %dKhz -> %dKhz, index = %d\n",
			inno_phy.tmds_clock, inno_phy.mpll[index].tmds_clk, index);
		goto mpll_cfg;
	}

	hdmi_err("inno phy mpll unsupport tmds clock %dKHz!\n", inno_phy.tmds_clock);
	return -1;

mpll_cfg:
	phy_base->hdmi_phy_pll0_1.bits.prepll_div    = dword_to_byte(inno_phy.mpll[index].pre_fbdiv, 0);
	phy_base->hdmi_phy_pll0_3.bits.prepll_fbdiv0 = dword_to_byte(inno_phy.mpll[index].pre_fbdiv, 1);
	phy_base->hdmi_phy_pll0_2.bits.prepll_fbdiv1 = dword_to_byte(inno_phy.mpll[index].pre_fbdiv, 2);

	phy_base->hdmi_phy_pll1_0.bits.prepll_linktmdsclk_div = dword_to_byte(inno_phy.mpll[index].pre_clkdiv0, 0);
	phy_base->hdmi_phy_pll1_0.bits.prepll_linkclk_div     = dword_to_byte(inno_phy.mpll[index].pre_clkdiv0, 1);
	phy_base->hdmi_phy_pll1_0.bits.prepll_tmdsclk_div     = dword_to_byte(inno_phy.mpll[index].pre_clkdiv0, 2);

	phy_base->hdmi_phy_pll1_1.bits.prepll_mainclk_div = dword_to_byte(inno_phy.mpll[index].pre_clkdiv1, 0);
	phy_base->hdmi_phy_pll1_1.bits.prepll_auxclk_div  = dword_to_byte(inno_phy.mpll[index].pre_clkdiv1, 1);

	phy_base->hdmi_phy_pll1_2.bits.prepll_reclk_div  = dword_to_byte(inno_phy.mpll[index].pre_clkdiv2, 0);
	phy_base->hdmi_phy_pll1_2.bits.prepll_pixclk_div = dword_to_byte(inno_phy.mpll[index].pre_clkdiv2, 1);
	phy_base->hdmi_phy_pll0_0.bits.pix_clk_bp        = dword_to_byte(inno_phy.mpll[index].pre_clkdiv2, 2);

	phy_base->hdmi_phy_pll_fra_1.bits.prepll_fra_div2 = dword_to_byte(inno_phy.mpll[index].pre_fra, 0);
	phy_base->hdmi_phy_pll_fra_2.bits.prepll_fra_div1 = dword_to_byte(inno_phy.mpll[index].pre_fra, 1);
	phy_base->hdmi_phy_pll_fra_3.bits.prepll_fra_div0 = dword_to_byte(inno_phy.mpll[index].pre_fra, 2);
	phy_base->hdmi_phy_pll0_2.bits.prepll_fra_ctl     = dword_to_byte(inno_phy.mpll[index].pre_fra, 3);

	phy_base->hdmi_phy_pll2_3.bits.postpll_pred_div   = dword_to_byte(inno_phy.mpll[index].post_fbdiv, 0);
	phy_base->hdmi_phy_pll3_1.bits.postpll_fbdiv1     = dword_to_byte(inno_phy.mpll[index].post_fbdiv, 1);
	phy_base->hdmi_phy_pll3_0.bits.postpll_fbdiv0     = dword_to_byte(inno_phy.mpll[index].post_fbdiv, 2);
	phy_base->hdmi_phy_pll2_2.bits.postpll_postdiv_en = dword_to_byte(inno_phy.mpll[index].post_fbdiv, 3);


	phy_base->hdmi_phy_pll3_1.bits.postpll_postdiv = dword_to_byte(inno_phy.mpll[index].post_div, 0);

	_inno_turn_pll_ctrl(0x1);
	hdmi_inf("inno phy config mpll:\n");
	hdmi_inf(" - %6dKhz %6dKhz [refs: %6dKhz]: 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
		inno_phy.pixel_clock, inno_phy.tmds_clock, inno_phy.mpll[index].tmds_clk, inno_phy.mpll[index].pre_fbdiv,
		inno_phy.mpll[index].pre_clkdiv0, inno_phy.mpll[index].pre_clkdiv1,
		inno_phy.mpll[index].pre_clkdiv2, inno_phy.mpll[index].pre_fra,
		inno_phy.mpll[index].post_fbdiv, inno_phy.mpll[index].post_div);
	return 0;
}

void inno_phy_set_reg_base(uintptr_t base)
{
	phy_base = (struct __inno_phy_reg_t *)(base + INNO_PHY_REG_OFFSET);
}

uintptr_t inno_phy_get_reg_base(void)
{
	return (uintptr_t)phy_base;
}

int inno_phy_write(u8 addr, void *data)
{
	u8 *value = (u8 *)data;

	if (IS_ERR_OR_NULL(value)) {
		shdmi_err(value);
		return -1;
	}

	*((u8 *)((void *)phy_base + addr)) = *value;
	return 0;
}

int inno_phy_read(u8 addr, void *data)
{
	u8 *value = (u8 *)data;
	if (!value) {
		hdmi_err("check read point value is null\n");
		return -1;
	}
	*value = *((u8 *)((void *)phy_base + addr));
	return 0;
}

int inno_phy_init(void)
{
	int ret = 0, i = 0, read_size;
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	u32 *read_buf;
	struct device_node *node = hdmi->dev->of_node;
	char phy_name[20];

	hdmi_trace("inno phy init\n");
	inno_phy_set_reg_base(hdmi->addr);

	inno_phy.mpll = phy_mpll;
	inno_phy.mpll_size = ARRAY_SIZE(phy_mpll);

	inno_phy_set_version();

	sprintf(phy_name, "inno_phy%d", inno_phy.version);
	/* parse dts */
	read_size = of_property_count_elems_of_size(node, phy_name, sizeof(u32));
	if (read_size <= 0) {
		hdmi_inf("inno phy not get table from dts, use default\n");
		goto use_default;
	}

	inno_phy.drive_size = read_size / 6;
	read_buf = kmalloc(inno_phy.drive_size * sizeof(struct inno_phy_drive), GFP_KERNEL | __GFP_ZERO);
	if (!read_buf) {
		hdmi_err("inno phy alloc buffer failed\n");
		goto use_default;
	}

	ret = of_property_read_u32_array(node, phy_name, read_buf, read_size);
	if (ret < 0) {
		hdmi_err("inno phy get dts table value failed\n");
		goto use_default;
	}
	inno_phy.drive = (struct inno_phy_drive *)read_buf;
	goto exit;

use_default:
	hdmi_wrn("inno phy use default table!\n");
	if (inno_phy.version == INNO_PHY_VERSION_0) {
		inno_phy.drive = drive_tab0;
		inno_phy.drive_size = ARRAY_SIZE(drive_tab0);
	} else if (inno_phy.version == INNO_PHY_VERSION_1) {
		inno_phy.drive = drive_tab1;
		inno_phy.drive_size = ARRAY_SIZE(drive_tab1);
	} else {
		inno_phy.drive = drive_tab2;
		inno_phy.drive_size = ARRAY_SIZE(drive_tab2);
	}

exit:
	hdmi_inf("inno phy drive:\n");
	for (i = 0; i < inno_phy.drive_size; i++) {
		hdmi_inf(" - [%d - %d]: 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
			inno_phy.drive[i].min_clk, inno_phy.drive[i].max_clk,
			inno_phy.drive[i].cur_bias, inno_phy.drive[i].vlevel,
			inno_phy.drive[i].pre_empl, inno_phy.drive[i].post_empl);
	}

	return 0;
}

int inno_phy_config(void)
{
	int ret = 0;
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();

	inno_phy.tmds_clock = hdmi->tmds_clk;
	inno_phy.pixel_clock = hdmi->pixel_clk;

	_inno_phy_all_rst();

	_inno_phy_turn_off();

	_inno_phy_turn_on();

	ret = _inno_phy_config_pll();
	if (ret != 0) {
		hdmi_err("inno phy mpll config failed\n");
		return -1;
	}

	ret = _inno_phy_wait_pll_lock();
	if (ret != 0) {
		hdmi_err("inno phy wait pll lock timeout\n");
		return -1;
	}

	ret = _inno_phy_config_drive();
	if (ret != 0) {
		hdmi_err("inno phy config drive failed\n");
		return -1;
	}

	_inno_phy_output_en();

	ret = dw_phy_wait_lock();
	hdmi_inf("dw phy wait pll: %s\n", ret == 1 ? "lock" : "unlock");
	return (ret == 1) ? 0 : -1;
}

ssize_t inno_phy_dump(char *buf)
{
	ssize_t n = 0;

	n += sprintf(buf + n, "\n[inno phy %d]\n", inno_phy.version);
	n += sprintf(buf + n, " - link clock[%s]\n",
		phy_base->hdmi_phy_pll3_1.bits.linkcolor ? "Pre-PLL" : "Post-PLL");
	n += sprintf(buf + n, " - Pre-PLL : power[%s], status[%s] ssc[%s], mode[%s]\n",
		phy_base->hdmi_phy_pll0_0.bits.prepll_pow ? "off" : "on",
		phy_base->hdmi_phy_pll2_1.bits.prepll_lock_state ? "lock" : "unlock",
		phy_base->hdmi_phy_pll0_2.bits.prepll_SSC_mdu ? "disable" : "enable",
		phy_base->hdmi_phy_pll0_2.bits.prepll_SSC_md  ? "down" : "center");
	n += sprintf(buf + n, " - Post-PLL: power[%s], status[%s]\n",
		phy_base->hdmi_phy_pll2_2.bits.postpll_pow ? "off" : "on",
		phy_base->hdmi_phy_pll3_3.bits.postpll_lock_state ? "lock" : "unlock");

	n += sprintf(buf + n, " - CLK: driver[%s], bias[%duA], level[%d]\n",
		phy_base->hdmi_phy_dr0_2.bits.clk_dr_en ? "enable" : "disable",
		(phy_base->hdmi_phy_dr3_3.bits.clk_cur_bias * 20) + 320,
		phy_base->hdmi_phy_dr1_1.bits.clk_vlevel);

	n += sprintf(buf + n, " - CH0: driver[%s], bias[%duA], level[%d], bist[%s], ldo[%s]\n",
		phy_base->hdmi_phy_dr0_2.bits.ch0_dr_en ? "enable" : "disable",
		(phy_base->hdmi_phy_dr4_0.bits.ch0_cur_bias * 20) + 320,
		phy_base->hdmi_phy_dr2_0.bits.ch0_vlevel,
		phy_base->hdmi_phy_dr1_0.bits.ch0_BIST_en ? "enable" : "disable",
		phy_base->hdmi_phy_dr1_0.bits.ch0_LDO_en ? "enable" : "disable");

	n += sprintf(buf + n, " - CH1: driver[%s], bias[%duA], level[%d], bist[%s], ldo[%s]\n",
		phy_base->hdmi_phy_dr0_2.bits.ch1_dr_en ? "enable" : "disable",
		(phy_base->hdmi_phy_dr4_0.bits.ch1_cur_bias * 20) + 320,
		phy_base->hdmi_phy_dr1_3.bits.ch1_vlevel,
		phy_base->hdmi_phy_dr1_0.bits.ch1_BIST_en ? "enable" : "disable",
		phy_base->hdmi_phy_dr1_0.bits.ch1_LDO_en ? "enable" : "disable");

	n += sprintf(buf + n, " - CH2: driver[%s], bias[%duA], level[%d], bist[%s], ldo[%s]\n",
		phy_base->hdmi_phy_dr0_2.bits.ch2_dr_en ? "enable" : "disable",
		(phy_base->hdmi_phy_dr3_3.bits.ch2_cur_bias * 20) + 320,
		phy_base->hdmi_phy_dr1_2.bits.ch2_vlevel,
		phy_base->hdmi_phy_dr1_0.bits.ch2_BIST_en ? "enable" : "disable",
		phy_base->hdmi_phy_dr1_0.bits.ch2_LDO_en ? "enable" : "disable");

	return n;
}

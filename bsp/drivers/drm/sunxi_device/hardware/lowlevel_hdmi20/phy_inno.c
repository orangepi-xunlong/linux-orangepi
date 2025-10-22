/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#include <sunxi-sid.h>
#include <linux/delay.h>

#include "dw_dev.h"
#include "dw_phy.h"
#include "dw_mc.h"
#include "phy_inno.h"

#define INNO_PHY_TIMEOUT			1000
#define INNO_PHY_REG_OFFSET			0x10000

enum {
	INNO_PHY_VERSION_0 = 0,
	INNO_PHY_VERSION_1,
	INNO_PHY_VERSION_2,
};

struct inno_phy_pll {
	u32 tmds_clk;        /* tmds clock: unit:kHZ */
	u32 prepll_div;      /* fbdiv1 << 16 || fbdiv0 << 8 || prediv*/
	u32 prepll_clk_div;

	/* prepll_clk_div1 = (prepll_auxclk_div << 8) || prepll_mainclk_div */
	u32 prepll_clk_div1;

	/* prepll_clk_div2 = (pix_clk_bp << 16)|| (prepll_pixclk_div << 8) || prepll_reclk_div */
	u32 prepll_clk_div2;

	/* prepll_fra = (prepll_fra_ctl << 24) || (prepll_fra_div0 << 16) || (prepll_fra_div1 << 8) || (prepll_fra_div2) */
	u32 prepll_fra;

	u32 postpll;         /* diven << 24 || fbdiv0 << 16 || fbdiv1 << 8 || preddiv */
	u8 postpll_postdiv ; /* postdiv */
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
	int version;
	int drive_size;
	struct inno_phy_drive *drive;
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
	{25200,  0x00002a01, 0x00010103, 0x00000103, 0x00000403, 0x03000000, 0x03280000, 0x03},
	{27000,  0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03},
	{33750,  0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03},
	{36000,  0x00002401, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x03280001, 0x03},
	{37125,  0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01},
	{40000,  0x00002801, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{46406,  0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01},
	{65000,  0x00004101, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{68250,  0x00005b01, 0x00030300, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{71000,  0x00004701, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{72000,  0x00002401, 0x00000002, 0x00000101, 0x00000202, 0x03000000, 0x03140001, 0x01},
	{74250,  0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{75000,  0x00003201, 0x00020200, 0x00000100, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{79500,  0x00003501, 0x00020200, 0x00000100, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{85500,  0x00003901, 0x00020200, 0x00000100, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{88750,  0x00002c01, 0x00000002, 0x00000101, 0x00000202, 0x00000060, 0x03140001, 0x01},
	{92813,  0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01},
	{101000, 0x00006501, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{108000, 0x00002401, 0x00010100, 0x00000100, 0x00000202, 0x03000000, 0x030a0001, 0x00},
	{119000, 0x00007701, 0x00010102, 0x00000101, 0x00000403, 0x03000000, 0x030a0001, 0x00},
	{148500, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00},
	{154000, 0x00004d01, 0x00000002, 0x00000101, 0x00000202, 0x03000000, 0x030a0001, 0x00},
	{185625, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00},
	{234000, 0x00007501, 0x00000002, 0x00000303, 0x00000202, 0x03000000, 0x030a0000, 0x00},
	{297000, 0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00},
	{312250, 0x00006801, 0x00000001, 0x00000403, 0x00000101, 0x00555515, 0x030a0000, 0x00},
	{371250, 0x00007B01, 0x00000201, 0x00000103, 0x00000101, 0x000000C0, 0x000a0002, 0x00},
	{594000, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00},
	{594000, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00},
	{0,      0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00}
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

static struct inno_phy_pll *_inno_phy_get_mpll_params(void)
{
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	int size = 0, index = 0;
	u32 ref_clk = 0x0, tmds_clk = hdmi->tmds_clk;

	size = ARRAY_SIZE(phy_mpll) - 1;

	/* check min clock */
	if (tmds_clk < phy_mpll[0].tmds_clk) {
		ref_clk = phy_mpll[0].tmds_clk;
		hdmi_wrn("raw clock is %ukHz, change use min ref clock %ukHz\n",
			tmds_clk, ref_clk);
		goto clk_cfg;
	}

	/* check max clock */
	if (tmds_clk > phy_mpll[size - 1].tmds_clk) {
		ref_clk = phy_mpll[size - 1].tmds_clk;
		hdmi_wrn("raw clock is %ukHz, change use min ref clock %ukHz\n",
			tmds_clk, ref_clk);
		goto clk_cfg;
	}

	for (index = 0; index < size; index++) {
		/* check clock is match in table */
		if (tmds_clk == phy_mpll[index].tmds_clk) {
			ref_clk = phy_mpll[index].tmds_clk;
			goto clk_cfg;
		}
		/* check clock is match in table */
		if (tmds_clk == phy_mpll[index + 1].tmds_clk) {
			ref_clk = phy_mpll[index + 1].tmds_clk;
			goto clk_cfg;
		}
		/* clock unmatch and use near clock */
		if ((tmds_clk > phy_mpll[index].tmds_clk) &&
				(tmds_clk < phy_mpll[index + 1].tmds_clk)) {
			/* clock unmatch and use near clock */
			if ((tmds_clk - phy_mpll[index].tmds_clk) >
					(phy_mpll[index + 1].tmds_clk - tmds_clk))
				ref_clk = phy_mpll[index + 1].tmds_clk;
			else
				ref_clk = phy_mpll[index].tmds_clk;

			hdmi_inf("raw clock is %ukHz, change use near ref clock %ukHz\n",
				tmds_clk, ref_clk);
			goto clk_cfg;
		}
	}
	hdmi_err("inno phy clock %uHz auto approach mode failed!\n", tmds_clk);
	return NULL;

clk_cfg:
	for (index = 0; phy_mpll[index].tmds_clk != 0; index++) {
		if (ref_clk == phy_mpll[index].tmds_clk) {
			hdmi_trace("inno phy mpll use table[%d]\n", index);
			return &(phy_mpll[index]);
		}
	}
	hdmi_err("inno phy get mpll failed when tmds clock %dKHz, ref clock %dKHz\n",
			tmds_clk, ref_clk);
	return NULL;
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

static void _inno_phy_turn_off(void)
{
	/* turn off diver */
	_inno_turn_dirver_ctrl(0x0);
	/* turn off serializer */
	_inno_turn_serializer_ctrl(0x0);
	/* turn off LDO */
	_inno_turn_ldo_ctrl(0x0);
	/* turn off prePLL AND post_pll */
	_inno_turn_pll_ctrl(0x0);
	/* turn off resense */
	_inno_turn_resense_ctrl(0x0);
	/* turn off bias circuit */
	_inno_turn_biascircuit_ctrl(0x0);
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
	if (inno_phy.version != INNO_PHY_VERSION_1)
		return;

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

static int _inno_phy_mpll_config(void)
{
	struct inno_phy_pll *config = NULL;

	config = _inno_phy_get_mpll_params();
	if (!config) {
		hdmi_err("inno phy con not get mpll table\n");
		return -1;
	}

	if (config->tmds_clk == 594000) {
		_inno_phy_config_4k60();
	} else if (config->tmds_clk == 297000) {
		_inno_phy_config_4k30();
	}

	phy_base->hdmi_phy_pll0_1.bits.prepll_div =
		dw_to_byte(config->prepll_div, 0);
	phy_base->hdmi_phy_pll0_3.bits.prepll_fbdiv0 =
		dw_to_byte(config->prepll_div, 1);
	phy_base->hdmi_phy_pll0_2.bits.prepll_fbdiv1 =
		dw_to_byte(config->prepll_div, 2);

	phy_base->hdmi_phy_pll1_0.bits.prepll_linktmdsclk_div =
		dw_to_byte(config->prepll_clk_div, 0);
	phy_base->hdmi_phy_pll1_0.bits.prepll_linkclk_div =
		dw_to_byte(config->prepll_clk_div, 1);
	phy_base->hdmi_phy_pll1_0.bits.prepll_tmdsclk_div =
		dw_to_byte(config->prepll_clk_div, 2);

	phy_base->hdmi_phy_pll1_1.bits.prepll_mainclk_div =
		dw_to_byte(config->prepll_clk_div1, 0);
	phy_base->hdmi_phy_pll1_1.bits.prepll_auxclk_div =
		dw_to_byte(config->prepll_clk_div1, 1);

	phy_base->hdmi_phy_pll1_2.bits.prepll_reclk_div =
		dw_to_byte(config->prepll_clk_div2, 0);
	phy_base->hdmi_phy_pll1_2.bits.prepll_pixclk_div =
		dw_to_byte(config->prepll_clk_div2, 1);
	phy_base->hdmi_phy_pll0_0.bits.pix_clk_bp =
		dw_to_byte(config->prepll_clk_div2, 2);

	phy_base->hdmi_phy_pll_fra_1.bits.prepll_fra_div2 =
		dw_to_byte(config->prepll_fra, 0);
	phy_base->hdmi_phy_pll_fra_2.bits.prepll_fra_div1 =
		dw_to_byte(config->prepll_fra, 1);
	phy_base->hdmi_phy_pll_fra_3.bits.prepll_fra_div0 =
		dw_to_byte(config->prepll_fra, 2);
	phy_base->hdmi_phy_pll0_2.bits.prepll_fra_ctl =
		dw_to_byte(config->prepll_fra, 3);

	phy_base->hdmi_phy_pll2_3.bits.postpll_pred_div =
		dw_to_byte(config->postpll, 0);
	phy_base->hdmi_phy_pll3_1.bits.postpll_fbdiv1 =
		dw_to_byte(config->postpll, 1);
	phy_base->hdmi_phy_pll3_0.bits.postpll_fbdiv0 =
		dw_to_byte(config->postpll, 2);
	phy_base->hdmi_phy_pll2_2.bits.postpll_postdiv_en =
		dw_to_byte(config->postpll, 3);

	phy_base->hdmi_phy_pll3_1.bits.postpll_postdiv =
		config->postpll_postdiv;

	_inno_turn_pll_ctrl(0x1);

	return 0;
}

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
 * @Desc: get inno phy pre pll and post pll lock status
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
	phy_base->hdmi_phy_dr4_0.bits.ch0_cur_bias = dw_to_byte(data, 0);
	phy_base->hdmi_phy_dr4_0.bits.ch1_cur_bias = dw_to_byte(data, 1);
	phy_base->hdmi_phy_dr3_3.bits.ch2_cur_bias = dw_to_byte(data, 2);
	phy_base->hdmi_phy_dr3_3.bits.clk_cur_bias = dw_to_byte(data, 3);
}

static void _inno_phy_cfg_vlevel(u32 data)
{
	phy_base->hdmi_phy_dr1_1.bits.clk_vlevel = dw_to_byte(data, 0);
	phy_base->hdmi_phy_dr1_2.bits.ch2_vlevel = dw_to_byte(data, 1);
	phy_base->hdmi_phy_dr1_3.bits.ch1_vlevel = dw_to_byte(data, 2);
	phy_base->hdmi_phy_dr2_0.bits.ch0_vlevel = dw_to_byte(data, 3);
}

static void _inno_phy_cfg_pre_empl(u32 data)
{
	phy_base->hdmi_phy_dr0_3.bits.clk_pre_empl = dw_to_byte(data, 0);
	phy_base->hdmi_phy_dr2_3.bits.ch2_pre_empl = dw_to_byte(data, 1);
	phy_base->hdmi_phy_dr3_0.bits.ch1_pre_empl = dw_to_byte(data, 2);
	phy_base->hdmi_phy_dr3_1.bits.ch0_pre_empl = dw_to_byte(data, 3);
}

static void _inno_phy_cfg_post_empl(u32 data)
{
	phy_base->hdmi_phy_dr0_3.bits.clk_post_empl = dw_to_byte(data, 0);
	phy_base->hdmi_phy_dr2_3.bits.ch2_post_empl = dw_to_byte(data, 1);
	phy_base->hdmi_phy_dr3_0.bits.ch1_post_empl = dw_to_byte(data, 2);
	phy_base->hdmi_phy_dr3_1.bits.ch0_post_empl = dw_to_byte(data, 3);
}

static void _inno_phy_enable_data_sync(void)
{
	phy_base->hdmi_phy_ctl0_2.bits.data_sy_ctl = 0x0;
	mdelay(1);
	phy_base->hdmi_phy_ctl0_2.bits.data_sy_ctl = 0x1;
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

		hdmi_inf("inno phy tmds clock: %dKHz match in [%dKHz~%dKHz]\n",
			tmds_clk, table[i].min_clk, table[i].max_clk);
		hdmi_inf(" - drive use: 0x%08x, 0x%08x, 0x%08x, 0x%08x\n",
			table[i].cur_bias, table[i].vlevel,
			table[i].pre_empl, table[i].post_empl);

		_inno_phy_cfg_cur_bias(table[i].cur_bias);

		_inno_phy_cfg_vlevel(table[i].vlevel);

		_inno_phy_cfg_pre_empl(table[i].pre_empl);

		_inno_phy_cfg_post_empl(table[i].post_empl);

		return 0;
	}

	hdmi_err("inno phy not config electrical\n");
	return -1;
}

static int _inno_phy_config_flow(void)
{
	int ret = 0;

	/* inno phy turn off */
	_inno_phy_turn_off();
	mdelay(1);

	/* turn on bias circuit */
	_inno_turn_biascircuit_ctrl(0x1);
	/* cal resistance config on chip out phy */
	_inno_turn_resistor_ctrl(0x0);
	/* turn on rxsense */
	_inno_turn_resense_ctrl(0x1);
	mdelay(1);

	ret = wait_event_timeout(phy_wq, _inno_phy_get_rxsense_lock(),
				msecs_to_jiffies(10));
	if (ret == 0)
		hdmi_wrn("inno phy wait rxsense lock timeout\n");

	/* start config inno phy mpll */
	ret = _inno_phy_mpll_config();
	if (ret != 0) {
		hdmi_err("inno phy mpll config failed\n");
		return -1;
	}

	/* wait for pre-PLL and post-PLL lock */
	ret = wait_event_timeout(phy_wq, _inno_phy_get_pll_lock(),
				msecs_to_jiffies(10));
	if (ret == 0) {
		hdmi_err("inno phy wait pre-pll and post-pll lock timeout\n");
		return -1;
	}

	/* turn on LDO */
	_inno_turn_ldo_ctrl(0x1);
	/* turn on serializer */
	_inno_turn_serializer_ctrl(0x1);
	/* turn on diver */
	_inno_turn_dirver_ctrl(0x1);

	/* config electrical capability */
	ret = _inno_phy_config_drive();
	if (ret != 0) {
		hdmi_err("inno phy config drive failed\n");
		return -1;
	}

	_inno_phy_digital_reset();

	_inno_phy_enable_data_sync();

	hdmi_trace("inno phy config done!\n");
	return 0;
}

static void _inno_phy_reset(void)
{
	dw_phy_config_svsret();

	dw_mc_sw_reset(DW_MC_SWRST_PHY, 0x0);
	udelay(5);
	dw_mc_sw_reset(DW_MC_SWRST_PHY, 0x1);

	_inno_phy_analog_reset();

	_inno_phy_digital_reset();
	hdmi_trace("inno phy reset done\n");
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
	u32 *tmp_buf;
	struct device_node *hdmi_node = hdmi->dev->of_node;
	char phy_name[20];

	hdmi_trace("inno phy init\n");
	phy_base = (struct __inno_phy_reg_t *)(hdmi->addr + INNO_PHY_REG_OFFSET);

	inno_phy_set_version();

	sprintf(phy_name, "inno_phy%d", inno_phy.version);
	/* parse dts */
	read_size = of_property_count_elems_of_size(hdmi_node, phy_name, sizeof(u32));
	if (read_size <= 0) {
		hdmi_inf("inno phy not get table from dts, use default\n");
		goto use_default;
	}

	inno_phy.drive_size = read_size / 6;
	tmp_buf = kmalloc(inno_phy.drive_size * sizeof(struct inno_phy_drive), GFP_KERNEL | __GFP_ZERO);
	if (!tmp_buf) {
		hdmi_err("inno phy alloc buffer failed\n");
		goto use_default;
	}

	ret = of_property_read_u32_array(hdmi_node, phy_name, tmp_buf, read_size);
	if (ret < 0) {
		hdmi_err("inno phy get dts table value failed\n");
		goto use_default;
	}
	inno_phy.drive = (struct inno_phy_drive *)tmp_buf;
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

	_inno_phy_reset();

	ret = _inno_phy_config_flow();
	if (ret < 0) {
		hdmi_err("inno phy config failed!!!\n");
		return -1;
	}

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

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include <linux/delay.h>
#include "dw_access.h"
#include "dw_phy.h"
#include "dw_mc.h"
#include "phy_inno.h"
#include <sunxi-sid.h>

static unsigned int phy_version;
static volatile struct __inno_phy_reg_t *phy_base;

/* inno_phy clock: unit:kHZ */
static struct inno_phy_config inno_phy_version_0[] = {
	{25200,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00002a01, 0x00010103, 0x00000103, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{27000,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{33750,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{37125,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{46406,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{74250,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{74250,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{92813,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{148500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{148500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{185625, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{297000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{297000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{371250, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00007B01, 0x00000201, 0x00000103, 0x00000101, 0x000000C0, 0x000a0002, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{594000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x020f0f0f, 0x1c1c1c1c, 0x00000000, 0x03030300},
	{594000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x020f0f0f, 0x1c1c1c1c, 0x00000000, 0x03030300},
	{742500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x020f0f0f, 0x1c1c1c1c, 0x00000000, 0x03030300},
	{0, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_NULL, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000}
};

static struct inno_phy_config inno_phy_version_1[] = {
	{25200,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00002a01, 0x00010103, 0x00000103, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x00020203, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{27000,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x01000102, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{33750,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{37125,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{46406,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{74250,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01, 0x01000102, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{74250,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{92813,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{148500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00, 0x01000102, 0x1c1c1c1c, 0x00000000, 0x02020200},
	{148500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{185625, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{297000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00, 0x01040506, 0x1c1c1c1c, 0x00000000, 0x03030300},
	{297000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{371250, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00007B01, 0x00000201, 0x00000103, 0x00000101, 0x000000C0, 0x000a0002, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{594000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x010f0f0f, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{594000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x020f0f0f, 0x1c1c1c1c, 0x00000000, 0x03030300},
	{742500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x020f0f0f, 0x1c1c1c1c, 0x00000000, 0x03030300},
	{0, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_NULL, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000}
};

static struct inno_phy_config inno_phy_version_2[] = {
	{25200,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00002a01, 0x00010103, 0x00000103, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x00020203, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{27000,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x01010102, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{33750,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00003601, 0x00020202, 0x00000603, 0x00000403, 0x03000000, 0x03280000, 0x03, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{37125,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{46406,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00030301, 0x00000102, 0x00000403, 0x03000000, 0x03500004, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{74250,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01, 0x01010102, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{74250,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00020201, 0x00000102, 0x00000403, 0x03000000, 0x03140001, 0x01, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{92813,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00007B01, 0x00020201, 0x00000103, 0x00000403, 0x000000C0, 0x030a0001, 0x00, 0x01010102, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{148500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00, 0x01010102, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{148500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00010101, 0x00000102, 0x00000202, 0x03000000, 0x030a0001, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{185625, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00007B01, 0x00010101, 0x00000103, 0x00000202, 0x000000C0, 0x030a0001, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{297000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00, 0x01020304, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{297000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000001, 0x00000102, 0x00000101, 0x03000000, 0x03140002, 0x00, 0x02060708, 0x1c1c1c1c, 0x00000000, 0x05050500},
	{371250, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00007B01, 0x00000201, 0x00000103, 0x00000101, 0x000000C0, 0x000a0002, 0x00, 0x010e0f0f, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{594000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x010e0f0f, 0x1c1c1c1c, 0x00000000, 0x00000000},
	{594000, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x020f0f0f, 0x1c1c1c1c, 0x00000000, 0x03030300},
	{742500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10, 0x00006301, 0x00000200, 0x00000100, 0x00000101, 0x03000000, 0x00140004, 0x00, 0x020f0f0f, 0x1c1c1c1c, 0x00000000, 0x03030300},
	{0, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_NULL, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00, 0x00000000, 0x00000000, 0x00000000, 0x00000000}
};

enum {
	INNO_PHY_VERSION_0 = 0,
	INNO_PHY_VERSION_1,
	INNO_PHY_VERSION_2,
};

static void inno_phy_set_version(void)
{
	unsigned int id = sunxi_get_soc_markid();
	unsigned int version = sunxi_get_soc_ver();

	if (id == 0x5700 && version == 2)
		phy_version = INNO_PHY_VERSION_1;	/* t - c */
	else if (id == 0x5c00 || id == 0xff10)
		phy_version = INNO_PHY_VERSION_2;	/* a */
	else
		phy_version = INNO_PHY_VERSION_0;	/* t - ab */
}

static void inno_phy_get_param(struct inno_phy_config **inno_phy, int *size)
{
	switch (phy_version) {
	case INNO_PHY_VERSION_1:
		*inno_phy = inno_phy_version_1;
		*size = (sizeof(inno_phy_version_1) / sizeof(struct inno_phy_config) - 1);
		break;
	case INNO_PHY_VERSION_2:
		*inno_phy = inno_phy_version_2;
		*size = (sizeof(inno_phy_version_2) / sizeof(struct inno_phy_config) - 1);
		break;
	case INNO_PHY_VERSION_0:
	default:
		*inno_phy = inno_phy_version_0;
		*size = (sizeof(inno_phy_version_0) / sizeof(struct inno_phy_config) - 1);
		break;
	}
}

static struct inno_phy_config *_inno_phy_auto_get_config(u32 pClk,
		dw_color_depth_t color, dw_pixel_repetition_t pixel)
{
	int index = 0;
	u32 ref_clk = 0;
	struct inno_phy_config *inno_phy = NULL;
	int size = 0;

	inno_phy_get_param(&inno_phy, &size);

	/* check min clock */
	if (pClk < inno_phy[0].clock) {
		ref_clk = inno_phy[0].clock;
		hdmi_wrn("raw clock is %ukHz, change use min ref clock %ukHz\n",
			pClk, ref_clk);
		goto clk_cfg;
	}

	/* check max clock */
	if (pClk > inno_phy[size - 1].clock) {
		ref_clk = inno_phy[size - 1].clock;
		hdmi_wrn("raw clock is %ukHz, change use min ref clock %ukHz\n",
			pClk, ref_clk);
		goto clk_cfg;
	}

	for (index = 0; index < size; index++) {
		/* check clock is match in table */
		if (pClk == inno_phy[index].clock) {
			ref_clk = inno_phy[index].clock;
			goto clk_cfg;
		}
		/* check clock is match in table */
		if (pClk == inno_phy[index + 1].clock) {
			ref_clk = inno_phy[index + 1].clock;
			goto clk_cfg;
		}
		/* clock unmatch and use near clock */
		if ((pClk > inno_phy[index].clock) &&
				(pClk < inno_phy[index + 1].clock)) {
			/* clock unmatch and use near clock */
			if ((pClk - inno_phy[index].clock) >
					(inno_phy[index + 1].clock - pClk))
				ref_clk = inno_phy[index + 1].clock;
			else
				ref_clk = inno_phy[index].clock;

			hdmi_inf("raw clock is %ukHz, change use near ref clock %ukHz\n",
				pClk, ref_clk);
			goto clk_cfg;
		}
	}
	hdmi_err("%s: inno phy clock %uHz auto approach mode failed! need check!!!\n",
			__func__, pClk);
	return NULL;

clk_cfg:
	for (index = 0; inno_phy[index].clock != 0; index++) {
		if ((ref_clk == inno_phy[index].clock) &&
				(color == inno_phy[index].color) &&
				(pixel == inno_phy[index].pixel)) {
			hdmi_inf("inno phy param use table[%d]\n", index);
			return &(inno_phy[index]);
		}
	}
	hdmi_err("%s: inno phy get config param failed!\n", __func__);
	hdmi_err(" - ref clock    : %uHz.\n", ref_clk);
	hdmi_err(" - color depth  : %d.\n", color);
	hdmi_err(" - pixel repeat : %d.\n", pixel);
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
	LOG_TRACE1(state);
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
	LOG_TRACE1(state);
	phy_base->hdmi_phy_dr3_2.bits.ch0_seri_en = state;
	phy_base->hdmi_phy_dr3_2.bits.ch1_seri_en = state;
	phy_base->hdmi_phy_dr3_2.bits.ch2_seri_en = state;
}

/**
 * @state: 1: turn on; 0: turn off
*/
static void _inno_turn_ldo_ctrl(u8 state)
{
	LOG_TRACE1(state);
	phy_base->hdmi_phy_dr1_0.bits.clk_LDO_en = state;
	phy_base->hdmi_phy_dr1_0.bits.ch0_LDO_en = state;
	phy_base->hdmi_phy_dr1_0.bits.ch1_LDO_en = state;
	phy_base->hdmi_phy_dr1_0.bits.ch2_LDO_en = state;

	if (phy_version != INNO_PHY_VERSION_0) {
		phy_base->hdmi_phy_dr2_1.bits.ch0_LDO_cur = state;
		phy_base->hdmi_phy_dr2_1.bits.ch1_LDO_cur = state;
		phy_base->hdmi_phy_dr2_1.bits.ch2_LDO_cur = state;
	}
}

/**
 * @state: 1: turn on; 0: turn off
*/
static void _inno_turn_pll_ctrl(u8 state)
{
	LOG_TRACE1(state);
	phy_base->hdmi_phy_pll2_2.bits.postpll_pow = !state;
	phy_base->hdmi_phy_pll0_0.bits.prepll_pow  = !state;
}

/**
 * @state: 1: turn on; 0: turn off
*/
static void _inno_turn_resense_ctrl(u8 state)
{
	LOG_TRACE1(state);
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
	LOG_TRACE1(state);
	phy_base->hdmi_phy_dr0_0.bits.bias_en = state;
}

/**
 * @state: 1: select resistor on-chip; 0: select resistor off-chip
*/
static void _inno_turn_resistor_ctrl(u8 state)
{
	LOG_TRACE1(state);
	phy_base->hdmi_phy_dr0_0.bits.refres = state;
}

void _inno_phy_config_4k60(void)
{
	// resence config
	phy_base->hdmi_phy_dr5_2.bits.terrescal_clkdiv0 = 0xF0;
	phy_base->hdmi_phy_dr5_1.bits.terrescal_clkdiv1 = 0x0;//24M/240 = 100K

	if (phy_version == INNO_PHY_VERSION_0) {
		//config resistance_div
		phy_base->hdmi_phy_dr6_0.bits.clkterres_ndiv = 0x28;
		phy_base->hdmi_phy_dr6_1.bits.ch2terres_ndiv = 0x28;
		phy_base->hdmi_phy_dr6_2.bits.ch1terres_ndiv = 0x28;
		phy_base->hdmi_phy_dr6_3.bits.ch0terres_ndiv = 0x28;
	}

	//config resistance 100
	phy_base->hdmi_phy_dr5_3.bits.terres_val = 0x0;

	//configure channel control register
	phy_base->hdmi_phy_dr5_3.bits.ch2_terrescal = 0x1;
	phy_base->hdmi_phy_dr5_3.bits.ch1_terrescal = 0x1;
	phy_base->hdmi_phy_dr5_3.bits.ch0_terrescal = 0x1;

	//config the calibration by pass
	phy_base->hdmi_phy_dr5_1.bits.terrescal_bp = 0x1;
	udelay(5);
	phy_base->hdmi_phy_dr5_1.bits.terrescal_bp = 0x0;
}

void _inno_phy_config_4k30(void)
{
	if ((phy_version == INNO_PHY_VERSION_0) ||
			(phy_version == INNO_PHY_VERSION_2))
		return ;

	// resence config
	phy_base->hdmi_phy_dr5_2.bits.terrescal_clkdiv0 = 0xF0;
	phy_base->hdmi_phy_dr5_1.bits.terrescal_clkdiv1 = 0x0;//24M/240 = 100K

	//config resistance 200
	phy_base->hdmi_phy_dr5_3.bits.terres_val = 0x3;

	//configure channel control register
	phy_base->hdmi_phy_dr5_3.bits.ch2_terrescal = 0x1;
	phy_base->hdmi_phy_dr5_3.bits.ch1_terrescal = 0x1;
	phy_base->hdmi_phy_dr5_3.bits.ch0_terrescal = 0x1;

	//config the calibration by pass
	phy_base->hdmi_phy_dr5_1.bits.terrescal_bp = 0x1;
	udelay(5);
	phy_base->hdmi_phy_dr5_1.bits.terrescal_bp = 0x0;
}

static int _inno_phy_pll_cfg(struct inno_phy_config *config)
{
	if (!config) {
		hdmi_err("%s: param is null!!!\n", __func__);
		return false;
	}

	phy_base->hdmi_phy_pll0_1.bits.prepll_div =
		config->prepll_div & 0xFF;

	phy_base->hdmi_phy_pll0_3.bits.prepll_fbdiv0 =
		(config->prepll_div >> 8) & 0xFF;

	phy_base->hdmi_phy_pll0_2.bits.prepll_fbdiv1 =
		(config->prepll_div >> 16) & 0xFF;

	phy_base->hdmi_phy_pll1_0.bits.prepll_linktmdsclk_div =
		config->prepll_clk_div & 0xFF;
	phy_base->hdmi_phy_pll1_0.bits.prepll_linkclk_div =
		(config->prepll_clk_div >> 8) & 0xFF;
	phy_base->hdmi_phy_pll1_0.bits.prepll_tmdsclk_div =
		(config->prepll_clk_div >> 16) & 0xFF;

	phy_base->hdmi_phy_pll1_1.bits.prepll_mainclk_div =
		config->prepll_clk_div1 & 0xFF;
	phy_base->hdmi_phy_pll1_1.bits.prepll_auxclk_div =
		(config->prepll_clk_div1 >> 8) & 0xFF;

	phy_base->hdmi_phy_pll1_2.bits.prepll_reclk_div =
		config->prepll_clk_div2 & 0xFF;
	phy_base->hdmi_phy_pll1_2.bits.prepll_pixclk_div =
		(config->prepll_clk_div2 >> 8) & 0xFF;
	phy_base->hdmi_phy_pll0_0.bits.pix_clk_bp =
		(config->prepll_clk_div2 >> 16) & 0xFF;

	phy_base->hdmi_phy_pll_fra_1.bits.prepll_fra_div2 =
		config->prepll_fra & 0xFF;

	phy_base->hdmi_phy_pll_fra_2.bits.prepll_fra_div1 =
		(config->prepll_fra >> 8) & 0xFF;

	phy_base->hdmi_phy_pll_fra_3.bits.prepll_fra_div0 =
		(config->prepll_fra >> 16) & 0xFF;

	phy_base->hdmi_phy_pll0_2.bits.prepll_fra_ctl =
		(config->prepll_fra >> 24) & 0xFF;

	phy_base->hdmi_phy_pll2_3.bits.postpll_pred_div =
		config->postpll & 0xFF;

	phy_base->hdmi_phy_pll3_1.bits.postpll_fbdiv1 =
		(config->postpll >> 8) & 0xFF;

	phy_base->hdmi_phy_pll3_0.bits.postpll_fbdiv0 =
		(config->postpll >> 16) & 0xFF;

	phy_base->hdmi_phy_pll2_2.bits.postpll_postdiv_en =
		(config->postpll >> 24) & 0xFF;

	phy_base->hdmi_phy_pll3_1.bits.postpll_postdiv =
		config->postpll_postdiv;

	_inno_turn_pll_ctrl(0x1);

	return true;
}

static int _inno_phy_get_rxsense_lock(void)
{
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch0_rxsense_de_sta == 0x0)
		return false;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch1_rxsense_de_sta == 0x0)
		return false;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch2_rxsense_de_sta == 0x0)
		return false;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.clk_rxsense_de_sta == 0x0)
		return false;

	return true;
}

static int _inno_phy_get_only_rxsense_lock(void)
{
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch0_rxsense_de_sta)
		return true;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch1_rxsense_de_sta)
		return true;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.ch2_rxsense_de_sta)
		return true;
	if (phy_base->hdmi_phy_rxsen_esd_1.bits.clk_rxsense_de_sta)
		return true;

	return false;
}

/**
 * @Desc: get inno phy pre pll and post pll lock status
*/
static inline int _inno_phy_get_pll_lock(void)
{
	if (phy_base->hdmi_phy_pll2_1.bits.prepll_lock_state == 0x0)
		return false;
	if (phy_base->hdmi_phy_pll3_3.bits.postpll_lock_state == 0x0)
		return false;

	return true;
}

static void _inno_phy_cfg_cur_bias(u32 bias)
{
	LOG_TRACE1(bias);
	phy_base->hdmi_phy_dr4_0.bits.ch0_cur_bias =
		(bias >> 0) & 0x000000ff;
	phy_base->hdmi_phy_dr4_0.bits.ch1_cur_bias =
		(bias >> 8) & 0x000000ff;
	phy_base->hdmi_phy_dr3_3.bits.ch2_cur_bias =
		(bias >> 16) & 0x000000ff;
	phy_base->hdmi_phy_dr3_3.bits.clk_cur_bias =
		(bias >> 24) & 0x000000ff;
}

static void _inno_phy_cfg_vlevel(u32 vlevel)
{
	LOG_TRACE1(vlevel);
	phy_base->hdmi_phy_dr1_1.bits.clk_vlevel =
		(vlevel >> 0) & 0x000000ff;
	phy_base->hdmi_phy_dr1_2.bits.ch2_vlevel =
		(vlevel >> 8) & 0x000000ff;
	phy_base->hdmi_phy_dr1_3.bits.ch1_vlevel =
		(vlevel >> 16) & 0x000000ff;
	phy_base->hdmi_phy_dr2_0.bits.ch0_vlevel =
		(vlevel >> 24) & 0x000000ff;
}

static void _inno_phy_cfg_pre_empl(u32 empl)
{
	LOG_TRACE1(empl);
	phy_base->hdmi_phy_dr0_3.bits.clk_pre_empl =
		(empl >> 0) & 0x000000ff;
	phy_base->hdmi_phy_dr2_3.bits.ch2_pre_empl =
		(empl >> 8) & 0x000000ff;
	phy_base->hdmi_phy_dr3_0.bits.ch1_pre_empl =
		(empl >> 16) & 0x000000ff;
	phy_base->hdmi_phy_dr3_1.bits.ch0_pre_empl =
		(empl >> 24) & 0x000000ff;
}

static void _inno_phy_cfg_post_empl(u32 empl)
{
	LOG_TRACE1(empl);
	phy_base->hdmi_phy_dr0_3.bits.clk_post_empl =
		(empl >> 0) & 0x000000ff;
	phy_base->hdmi_phy_dr2_3.bits.ch2_post_empl =
		(empl >> 8) & 0x000000ff;
	phy_base->hdmi_phy_dr3_0.bits.ch1_post_empl =
		(empl >> 16) & 0x000000ff;
	phy_base->hdmi_phy_dr3_1.bits.ch0_post_empl =
		(empl >> 24) & 0x000000ff;
}

static void _inno_phy_enable_data_sync(void)
{
	LOG_TRACE();
	phy_base->hdmi_phy_ctl0_2.bits.data_sy_ctl = 0x0;
	mdelay(1);
	phy_base->hdmi_phy_ctl0_2.bits.data_sy_ctl = 0x1;
}

static int _inno_phy_config_flow(struct inno_phy_config *config)
{
	int i = 0, status = 0;

	//start********power off IP**************//
	//turn off diver
	_inno_turn_dirver_ctrl(0x0);
	//turn off serializer
	_inno_turn_serializer_ctrl(0x0);
	//turn off LDO
	_inno_turn_ldo_ctrl(0x0);
	//turn off prePLL AND post_pll
	_inno_turn_pll_ctrl(0x0);
	//turn off resense
	_inno_turn_resense_ctrl(0x0);
	//turn off bias circuit
	_inno_turn_biascircuit_ctrl(0x0);
	//***********power off IP***********end//

	//turn on bias circuit
	_inno_turn_biascircuit_ctrl(0x1);
	//cal resistance config on chip out phy
	_inno_turn_resistor_ctrl(0x0);

	if (phy_version != INNO_PHY_VERSION_0) {
		// 0x00000100 off  0x00000033 on
		*((u32 *)((void *)phy_base + 0x8004)) = 0x00000100;
	}

	//turn on rxsense
	_inno_turn_resense_ctrl(0x1);

	//waite for rxsense detection result ture
	for (i = 0; i < INNO_PHY_TIMEOUT; i++) {
		udelay(5);

		if (i >= INNO_PHY_RXSENSE_TIMEOUT) {
			status = _inno_phy_get_only_rxsense_lock();
		} else {
			status = _inno_phy_get_rxsense_lock();
		}
		if (status) {
			VIDEO_INF("%s: phy rxsense detection result\n", __func__);
			break;
		}
	}
	if ((i >= INNO_PHY_TIMEOUT) && (!status)) {
		hdmi_wrn("phy rxsense detect timeout, and force config!\n");
	}

	//only 4k60 need it
	if (config->clock == 594000) {
		_inno_phy_config_4k60();
	}

	//only 4k30 need it
	if (config->clock == 297000) {
		_inno_phy_config_4k30();
	}

	_inno_phy_pll_cfg(config);

	//wait for pre-PLL and post-PLL lock
	for (i = 0; i < INNO_PHY_TIMEOUT; i++) {
		udelay(5);
		status = _inno_phy_get_pll_lock();
		if (status & 0x1) {
			VIDEO_INF("%s: phy pll lock detection result\n", __func__);
			break;
		}
	}
	if ((i >= INNO_PHY_TIMEOUT) && !status) {
		hdmi_err("phy pll lock Timeout! status = 0x%x\n", status);
		return -1;
	}

	//turn on LDO
	_inno_turn_ldo_ctrl(0x1);

	//turn on serializer
	_inno_turn_serializer_ctrl(0x1);

	//turn on diver
	_inno_turn_dirver_ctrl(0x1);

	_inno_phy_cfg_cur_bias(config->cur_bias);

	_inno_phy_cfg_vlevel(config->vlevel);

	_inno_phy_cfg_pre_empl(config->pre_empl);

	_inno_phy_cfg_post_empl(config->post_empl);

	_inno_phy_digital_reset();

	_inno_phy_enable_data_sync();

	return 0;
}

void inno_phy_set_reg_base(uintptr_t base)
{
	phy_base = (struct __inno_phy_reg_t *)(base + PHY_REG_OFFSET);
}

uintptr_t inno_phy_get_reg_base(void)
{
	return (uintptr_t)phy_base;
}

void inno_phy_write(u8 offset, u8 value)
{
	*((u8 *)((void *)phy_base + offset)) = value;
}

void inno_phy_read(u8 offset, u8 *value)
{
	*value = *((u8 *)((void *)phy_base + offset));
}

int inno_phy_configure(dw_hdmi_dev_t *dev)
{
	struct inno_phy_config *config = NULL;
	u32 pClk = dev->snps_hdmi_ctrl.tmds_clk;
	dw_color_depth_t color = dev->snps_hdmi_ctrl.color_resolution;
	dw_pixel_repetition_t pixel = dev->snps_hdmi_ctrl.pixel_repetition;

	inno_phy_set_version();
	hdmi_inf("phy_version: %d\n", phy_version);

	LOG_TRACE();

	/* Color resolution 0 is 8 bit color depth */
	if (color == 0)
		color = DW_COLOR_DEPTH_8;

	config = _inno_phy_auto_get_config(pClk, color, pixel);
	if (config == NULL) {
		hdmi_err("%s: failed to get phy config when clock(%dhz), color depth(%d), pixel repetition(%d)\n",
			__func__, pClk, color, pixel);
		return false;
	}

	dw_phy_config_svsret(dev, 0);
	udelay(5);
	dw_phy_config_svsret(dev, 1);

	dw_mc_phy_reset(dev, 0);
	udelay(5);
	dw_mc_phy_reset(dev, 1);

	_inno_phy_analog_reset();

	_inno_phy_digital_reset();

	if (_inno_phy_config_flow(config) < 0) {
		hdmi_err("%s: inno phy config failed!!!\n", __func__);
		return false;
	}

	if (dw_phy_wait_lock(dev) == 1) {
		hdmi_inf("inno phy pll locked!\n");
		return true;
	}

	hdmi_err("%s: inno phy pll not locked\n", __func__);
	return false;
}

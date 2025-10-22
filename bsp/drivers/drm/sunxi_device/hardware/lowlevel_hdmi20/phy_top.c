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
#include <linux/delay.h>
#include <linux/clk.h>

#include "dw_dev.h"
#include "phy_top.h"

static DECLARE_WAIT_QUEUE_HEAD(top_phy_wq);

#define PHY_DCXO_24M	(24000000)
#define PHY_DCXO_19_2M	(19200000)
#define PHY_DCXO_26M	(26000000)

struct top_phy_pll_s {
	u32 ref_clk; /* KHZ */
	u32 pll_value;
	u32 ldo_value;
	u32 pll_patern0;
	u32 pll_patern1;
};

struct top_phy_pll_s sun60i_phypll_26m[] = {
	{ 13500, 0xE8673500, 0x00035000, 0x00000000, 0x30000000},
	{ 27000, 0xE8595C00, 0x00035000, 0x80000000, 0x30000000},
	{ 54000, 0xE80C1A00, 0x00035000, 0x80000000, 0x30000000},
	{ 65000, 0xE8235A00, 0x00035000, 0x80000000, 0x30000000},
	{ 74250, 0xE81F5A00, 0x00035000, 0x80000000, 0x30000000},
	{108000, 0xE80C3500, 0x00035000, 0x00000000, 0x30000000},
	{148500, 0xE80F5A00, 0x00035000, 0x80000000, 0x30000000},
	{185625, 0xE80F5A00, 0x00035000, 0x80000000, 0x30000000},
	{297000, 0xE807B602, 0x00035000, 0x00000000, 0x30000000},
	{371250, 0xE807B602, 0x00035000, 0x00000000, 0x30000000},
	{594000, 0xE803B602, 0x00035000, 0x00000000, 0x30000000},
};

struct top_phy_pll_s sun60i_phypll_24m[] = {
	{ 13500, 0xE85F3500, 0x00035000, 0x00000000, 0x30000000},
	{ 27000, 0xE8576200, 0x00035000, 0x80000000, 0x30000000},
	{ 54000, 0xE82B6200, 0x00035000, 0x80000000, 0x30000000},
	{ 65000, 0xE8246200, 0x00035000, 0x80000000, 0x30000000},
	{ 74250, 0xE81F6200, 0x00035000, 0x80000000, 0x30000000},
	{148500, 0xE80F6200, 0x00035000, 0x80000000, 0x30000000},
	{185625, 0xE80F6200, 0x00035000, 0x80000000, 0x30000000},
	{297000, 0xE8076200, 0x00035000, 0x00000000, 0x30000000},
	{371250, 0xE8076200, 0x00035000, 0x00000000, 0x30000000},
	{594000, 0xE8036200, 0x00035000, 0x00000000, 0x30000000},
};

struct top_phy_s {
	u32 dcxo_khz;
	char *rate_name;
	u32  offset;
	u32  pll_size;
	struct top_phy_pll_s  *pll_data;
	volatile struct top_phy_regs	*phy_addr;
};

static struct top_phy_s   top_phy;

struct tphy_plat_s {
	u8   plat_id;
	u32  offset;
	struct {
		char name[10];
		u32 rate;
		u32 size;
		struct top_phy_pll_s  *mpll;
	} mpll_table[3];
};

struct tphy_plat_s  sun60i_plat = {
	.plat_id  = HDMI_SUN60I_W2_P1,
	.offset   = 0xE0000,
	.mpll_table = {
		{
			.name = "24M",
			.rate = PHY_DCXO_24M,
			.mpll = sun60i_phypll_24m,
			.size = ARRAY_SIZE(sun60i_phypll_24m),
		},
		{
			.name = "26M",
			.rate = PHY_DCXO_26M,
			.mpll = sun60i_phypll_26m,
			.size = ARRAY_SIZE(sun60i_phypll_26m),
		},
		{
			.name = "19.2M",
			.rate = PHY_DCXO_19_2M,
			.mpll = NULL,
			.size = 0,
		}
	},
};

static struct tphy_plat_s *sunxi_plat[] = {
	&sun60i_plat,
};

void top_phy_write(u32 offset, u32 data)
{
	*((volatile u32 *)top_phy.phy_addr + offset) = data;
}

u32 top_phy_read(u32 offset)
{
	return *((volatile u32 *)top_phy.phy_addr + offset);
}

u8 top_phy_get_clock_select(void)
{
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;
	u8 sel = 0;

	sel = phy_reg->reg_0024.sun60i.hdmi_outclk_sel;
	return sel;
}

void top_phy_set_clock_select(u8 sel)
{
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;

	phy_reg->reg_0024.sun60i.hdmi_outclk_sel = sel;
	hdmi_trace("top phy output clock from: %s\n", sel ? "ccmu" : "phy pll");
}

u8 top_phy_get_pad_select(void)
{
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;
	u8 sel = 0;

	sel = phy_reg->reg_0004.sun60i.hdmi_pad_sel;
	hdmi_trace("top phy get pad select: %s\n", sel ? "gpio pad" : "hdmi pad");
	return sel;
}

void top_phy_set_pad_select(u8 sel)
{
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;

	phy_reg->reg_0004.sun60i.hdmi_pad_sel = sel;
	hdmi_trace("top phy set pad select: %s\n", sel ? "gpio pad" : "hdmi pad");
}

u8 top_phy_pll_get_lock(void)
{
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;
	u8 state = (u8)phy_reg->reg_0040.sun60i.lock_status;

	return state;
}

void top_phy_pll_set_output(u8 state)
{
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;

	phy_reg->reg_0020.sun60i.pll_output_gate = state;
	hdmi_trace("top phy pll output gate: %s\n", state ? "enable" : "disable");
}

static u32 _top_phy_cal_clk(union REG_0020_t value)
{
	u32 clock;
	clock = top_phy.dcxo_khz * (value.sun60i.pll_n + 1);
	clock = clock / (value.sun60i.pll_input_div2 + 1);
	return clock / (value.sun60i.pll_p0 + 1);
}

int top_phy_config(void)
{
	int ret = 0, i = 0;
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	struct top_phy_pll_s *pll_cfg = top_phy.pll_data;
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;
	u32 clock = hdmi->pixel_repeat ? hdmi->pixel_clk : hdmi->tmds_clk;
	u32 size = top_phy.pll_size;
	u8 index = 0;

	top_phy_pll_set_output(0x0);

	if (hdmi->clock_src) {
		hdmi_inf("not-config phy pll when use ccmu mode\n");
		return 0;
	}

	/* check min clock */
	if (clock < pll_cfg[0].ref_clk) {
		index = 0;
		goto cfg_pll;
	}

	if (clock > pll_cfg[size - 1].ref_clk) {
		index = size - 1;
		goto cfg_pll;
	}

	for (i = 0; i < size; i++) {
		if (clock == pll_cfg[i].ref_clk) {
			index = i;
			goto cfg_pll;
		}

		if (clock > pll_cfg[i].ref_clk &&
				clock < pll_cfg[i + 1].ref_clk) {
			if ((pll_cfg[i + 1].ref_clk - clock) >
					(clock - pll_cfg[i].ref_clk))
				index = i;
			else
				index = i + 1;
			goto cfg_pll;
		}
	}

cfg_pll:
	phy_reg->reg_0020.sun60i.pll_input_div2 =
			((pll_cfg[index].pll_value & 0x00000002) >> 1);

	phy_reg->reg_0020.sun60i.pll_lock_model =
			((pll_cfg[index].pll_value & 0x00000020) >> 5);

	phy_reg->reg_0020.sun60i.pll_unlock_model =
			((pll_cfg[index].pll_value & 0x000000C0) >> 6);

	phy_reg->reg_0020.sun60i.pll_n =
			((pll_cfg[index].pll_value & 0x0000FF00) >> 8);

	phy_reg->reg_0020.sun60i.pll_p0 =
			((pll_cfg[index].pll_value & 0x007F0000) >> 16);

	phy_reg->reg_0028.dwval = pll_cfg[index].ldo_value;

	phy_reg->reg_002C.dwval = pll_cfg[index].pll_patern0;

	phy_reg->reg_0030.dwval = pll_cfg[index].pll_patern1;

	/* check pll open */
	if (!phy_reg->reg_0020.sun60i.pll_en)
		phy_reg->reg_0020.sun60i.pll_en = 0x1;

	if (!phy_reg->reg_0020.sun60i.pll_ldo_en)
		phy_reg->reg_0020.sun60i.pll_ldo_en = 0x1;

	phy_reg->reg_0024.sun60i.pll_level_shifter_gate = 0x1;

	/* enable lock check */
	phy_reg->reg_0020.sun60i.lock_enable = 0x1;

	hdmi_inf("[top phy]\n");
	hdmi_inf(" - %dKHz, %s clock\n", clock, hdmi->pixel_repeat ? "pixel" : "tmds");
	hdmi_inf("   - pll: 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
		pll_cfg[index].pll_value, pll_cfg[index].ldo_value, pll_cfg[index].pll_patern0, pll_cfg[index].pll_patern1);

	/* wait top phy pll lock */
	ret = wait_event_timeout(top_phy_wq, top_phy_pll_get_lock(), msecs_to_jiffies(20));
	if (ret == 0) {
		hdmi_err("top phy wait pll lock timeout\n");
		return -1;
	}

	udelay(20);
	top_phy_pll_set_output(0x1);

	hdmi_inf("top phy config done\n");
	return 0;
}

void top_phy_power(top_phy_power_t type)
{
	char *power_type[] = {"on", "off", "low"};
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;

	switch (type) {
	case SUNXI_TOP_PHY_POWER_ON:
		phy_reg->reg_0000.sun60i.phy_reset         = 0x1;
		phy_reg->reg_0000.sun60i.phy_pddq          = 0x1;
		phy_reg->reg_0000.sun60i.phy_txpwron       = 0x1;
		phy_reg->reg_0000.sun60i.phy_enhpdrxsense  = 0x1;
	break;
	case SUNXI_TOP_PHY_POWER_LOW:
		phy_reg->reg_0000.sun60i.phy_reset         = 0x0;
		phy_reg->reg_0000.sun60i.phy_pddq          = 0x0;
		phy_reg->reg_0000.sun60i.phy_txpwron       = 0x0;
		phy_reg->reg_0000.sun60i.phy_svsret_mode   = 0x1;
		phy_reg->reg_0000.sun60i.phy_enhpdrxsense  = 0x0;
	break;
	case SUNXI_TOP_PHY_POWER_OFF:
	default:
		phy_reg->reg_0000.sun60i.phy_reset         = 0x0;
		phy_reg->reg_0000.sun60i.phy_pddq          = 0x0;
		phy_reg->reg_0000.sun60i.phy_txpwron       = 0x0;
		phy_reg->reg_0000.sun60i.phy_svsret_mode   = 0x0;
		phy_reg->reg_0000.sun60i.phy_enhpdrxsense  = 0x0;
	break;
	}

	hdmi_trace("top phy power: %s\n", power_type[type]);
}

static int _top_phy_match_plat(u8 plat_id)
{
	int i = 0, j = 0;
	u32 dcxo_rate = 0;
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	struct device *dev = hdmi->dev;
	struct clk *clk_dcxo = NULL;

	clk_dcxo = devm_clk_get(dev, "clk_dcxo");
	if (IS_ERR_OR_NULL(clk_dcxo))
		hdmi_inf("top phy can not get clk dcxo rate\n");
	else
		dcxo_rate = clk_get_rate(clk_dcxo);

	for (i = 0; i < ARRAY_SIZE(sunxi_plat); i++) {
		if (sunxi_plat[i]->plat_id != plat_id)
			continue;
		/* match plat id and start get data */
		top_phy.offset = sunxi_plat[i]->offset;
		for (j = 0; j < ARRAY_SIZE(sunxi_plat[i]->mpll_table); j++) {
			if (sunxi_plat[i]->mpll_table[j].rate != dcxo_rate)
				continue;
			top_phy.dcxo_khz  = sunxi_plat[i]->mpll_table[j].rate / 1000;
			top_phy.rate_name = sunxi_plat[i]->mpll_table[j].name;
			top_phy.pll_data  = sunxi_plat[i]->mpll_table[j].mpll;
			top_phy.pll_size  = sunxi_plat[i]->mpll_table[j].size;
			return 0;
		}
	}
	return -1;
}

int top_phy_init(void)
{
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();
	int ret = 0;

	ret = _top_phy_match_plat(hdmi->plat_id);
	if (ret != 0) {
		hdmi_err("top phy get plat id [%d] data failed\n", hdmi->plat_id);
		return -1;
	}

	top_phy.phy_addr = (struct top_phy_regs *)(hdmi->addr + top_phy.offset);

	if (hdmi->sw_init)
		return 0;

	top_phy_power(SUNXI_TOP_PHY_POWER_ON);

	top_phy_pll_set_output(0x0);

	top_phy_set_clock_select(hdmi->clock_src);

	return 0;
}

ssize_t top_phy_dump(char *buf)
{
	ssize_t n = 0;
	volatile struct top_phy_regs *phy_reg = top_phy.phy_addr;

	n += sprintf(buf + n, "\n[top phy]\n");

	if (top_phy_get_clock_select()) {
		n += sprintf(buf + n, " - ccmu mode\n");
		goto exit;
	}

	n += sprintf(buf + n, "| name  | ref clk | output | state | cal clock |\n");
	n += sprintf(buf + n, "|-------+---------+--------+-------|-----------|\n");
	n += sprintf(buf + n, "| state |  %-5s  |   %-3s  | %-6s| %-6dKHz |\n",
		top_phy.rate_name,
		phy_reg->reg_0020.sun60i.pll_output_gate ? "on" : "off",
		top_phy_pll_get_lock() ? "lock" : "unlock",
		_top_phy_cal_clk(phy_reg->reg_0020));
	n += sprintf(buf + n, " - pll: 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
		phy_reg->reg_0020.dwval, phy_reg->reg_0028.dwval,
		phy_reg->reg_002C.dwval, phy_reg->reg_0030.dwval);
exit:
	return n;
}

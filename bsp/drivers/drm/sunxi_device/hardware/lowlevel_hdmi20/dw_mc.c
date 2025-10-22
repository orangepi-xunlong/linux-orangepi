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
#include <linux/delay.h>
#include "dw_dev.h"
#include "dw_mc.h"

typedef struct {
	dw_mc_swrst_t type;
	u32 reg;
	u32 mask;
} mc_swrst_t;

typedef struct {
	dw_mc_lock_e type;
	u32 mask;
} mc_clk_lock;

typedef struct {
	dw_mc_clk_e  type;
	u32 mask;
} mc_clk_ctrl;

typedef struct {
	dw_mc_irq_e irq;
	u32 stat_reg;
	u32 mute_reg;
} irq_vector_t;

static mc_swrst_t mc_swrst[] = {
	{DW_MC_SWRST_PIXEL,    MC_SWRSTZREQ,   MC_SWRSTZREQ_PIXELSWRST_REQ_MASK},
	{DW_MC_SWRST_TMDS,     MC_SWRSTZREQ,   MC_SWRSTZREQ_TMDSSWRST_REQ_MASK},
	{DW_MC_SWRST_PREP,     MC_SWRSTZREQ,   MC_SWRSTZREQ_PREPSWRST_REQ_MASK},
	{DW_MC_SWRST_I2S,      MC_SWRSTZREQ,   MC_SWRSTZREQ_II2SSWRST_REQ_MASK},
	{DW_MC_SWRST_SPDIF,    MC_SWRSTZREQ,   MC_SWRSTZREQ_ISPDIFSWRST_REQ_MASK},
	{DW_MC_SWRST_CEC,      MC_SWRSTZREQ,   MC_SWRSTZREQ_CECSWRST_REQ_MASK},
	{DW_MC_SWRST_IGPA,     MC_SWRSTZREQ,   MC_SWRSTZREQ_IGPASWRST_REQ_MASK},
	{DW_MC_SWRST_PHY,      MC_PHYRSTZ,     MC_PHYRSTZ_PHYRSTZ_MASK},
	{DW_MC_SWRST_HEACPHY,  MC_HEACPHYRSTZ, MC_HEACPHYRSTZ_MASK},
	{DW_MC_SWRST_ADMA,     MC_AHBDMARSTZ,  MC_AHBDMARSTZ_MASK},
};

static irq_vector_t irq_vec[] = {
	{DW_MC_IRQ_FC0,        IH_FC_STAT0,        IH_MUTE_FC_STAT0},
	{DW_MC_IRQ_FC1,        IH_FC_STAT1,        IH_MUTE_FC_STAT1},
	{DW_MC_IRQ_FC2,        IH_FC_STAT2,	       IH_MUTE_FC_STAT2},
	{DW_MC_IRQ_AS,	       IH_AS_STAT0,        IH_MUTE_AS_STAT0},
	{DW_MC_IRQ_PHY,        IH_PHY_STAT0,       IH_MUTE_PHY_STAT0},
	{DW_MC_IRQ_I2CM,       IH_I2CM_STAT0,      IH_MUTE_I2CM_STAT0},
	{DW_MC_IRQ_CEC,        IH_CEC_STAT0,       IH_MUTE_CEC_STAT0},
	{DW_MC_IRQ_VP,         IH_VP_STAT0,	       IH_MUTE_VP_STAT0},
	{DW_MC_IRQ_PHYI2C,     IH_I2CMPHY_STAT0,   IH_MUTE_I2CMPHY_STAT0},
	{DW_MC_IRQ_AUDIO_DMA,  IH_AHBDMAAUD_STAT0, IH_MUTE_AHBDMAAUD_STAT0},
};

static mc_clk_lock mc_lock[] = {
	{DW_MC_CLK_LOCK_CEC,   MC_LOCKONCLOCK_CECCLK_MASK},
	{DW_MC_CLK_LOCK_SPDIF, MC_LOCKONCLOCK_SPDIFCLK_MASK},
	{DW_MC_CLK_LOCK_I2S,   MC_LOCKONCLOCK_I2SCLK_MASK},
	{DW_MC_CLK_LOCK_PREP,  MC_LOCKONCLOCK_PREPCLK_MASK},
	{DW_MC_CLK_LOCK_TMDS,  MC_LOCKONCLOCK_TCLK_MASK},
	{DW_MC_CLK_LOCK_PIXEL, MC_LOCKONCLOCK_PCLK_MASK},
	{DW_MC_CLK_LOCK_IGPA,  MC_LOCKONCLOCK_IGPACLK_MASK},
};

static mc_clk_ctrl mc_clk[] = {
	{DW_MC_CLK_PIXEL, MC_CLKDIS_PIXELCLK_DISABLE_MASK},
	{DW_MC_CLK_TMDS,  MC_CLKDIS_TMDSCLK_DISABLE_MASK},
	{DW_MC_CLK_PREP,  MC_CLKDIS_PREPCLK_DISABLE_MASK},
	{DW_MC_CLK_AUDIO, MC_CLKDIS_AUDCLK_DISABLE_MASK},
	{DW_MC_CLK_CSC,   MC_CLKDIS_CSCCLK_DISABLE_MASK},
	{DW_MC_CLK_CEC,   MC_CLKDIS_CECCLK_DISABLE_MASK},
	{DW_MC_CLK_HDCP,  MC_CLKDIS_HDCPCLK_DISABLE_MASK},
};

void _dw_mc_set_csc_bypass(u8 bit)
{
	dw_write_mask(MC_FLOWCTRL, MC_FLOWCTRL_FEED_THROUGH_OFF_MASK, !bit);
}

int dw_mc_sw_reset(dw_mc_swrst_t type, u8 state)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(mc_swrst); i++) {
		if (mc_swrst[i].type == type) {
			dw_write_mask(mc_swrst[i].reg, mc_swrst[i].mask, state);
			return 0;
		}
	}
	hdmi_err("dw mc sw reset unsupport type: %d\n", type);
	return -1;
}

int dw_mc_set_clk(dw_mc_clk_e type, u8 state)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(mc_clk); i++) {
		if (mc_clk[i].type == type) {
			dw_write_mask(MC_CLKDIS, mc_clk[i].mask, !state);
			return 0;
		}
	}
	hdmi_err("dw mc set clk unsupport type: %d\n", type);
	return -1;
}

u8 dw_mc_get_clk(dw_mc_clk_e type)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(mc_clk); i++) {
		if (mc_clk[i].type == type)
			return !dw_read_mask(MC_CLKDIS, mc_clk[i].mask);
	}
	hdmi_err("dw mc get clk unsupport type: %d\n", type);
	return DW_HDMI_DISABLE;
}

u8 dw_mc_get_lock(dw_mc_lock_e type)
{
	int i = 0;

	for (i = 0; ARRAY_SIZE(mc_lock); i++) {
		if (mc_lock[i].type == type)
			return dw_read_mask(MC_LOCKONCLOCK, mc_lock[i].mask);
	}
	hdmi_err("dw mc get lock unsupport type: %d\n", type);
	return DW_HDMI_DISABLE;
}

void dw_mc_clk_all_enable(void)
{
	struct dw_hdmi_dev_s *hdmi = dw_get_hdmi();

	hdmi_trace("dw hdmi mc enable all clock\n");

	dw_mc_set_clk(DW_MC_CLK_AUDIO, hdmi->audio_on ? DW_HDMI_ENABLE : DW_HDMI_DISABLE);
	/* make sure audio clock enable */
	mdelay(20);

	_dw_mc_set_csc_bypass(DW_HDMI_ENABLE);
	dw_mc_set_clk(DW_MC_CLK_PIXEL, DW_HDMI_ENABLE);
	dw_mc_set_clk(DW_MC_CLK_TMDS,  DW_HDMI_ENABLE);
	dw_mc_set_clk(DW_MC_CLK_PREP, hdmi->pixel_repeat ? DW_HDMI_ENABLE : DW_HDMI_DISABLE);
	dw_mc_set_clk(DW_MC_CLK_PREP, DW_HDMI_ENABLE);
	dw_mc_set_clk(DW_MC_CLK_CSC, DW_HDMI_ENABLE);
	dw_mc_set_clk(DW_MC_CLK_HDCP, DW_HDMI_DISABLE);
}

void dw_mc_clk_all_disable(void)
{
	hdmi_trace("dw hdmi mc disable all clock\n");
	dw_mc_set_clk(DW_MC_CLK_PIXEL, DW_HDMI_DISABLE);
	dw_mc_set_clk(DW_MC_CLK_TMDS,  DW_HDMI_DISABLE);
	dw_mc_set_clk(DW_MC_CLK_PREP,  DW_HDMI_DISABLE);
	dw_mc_set_clk(DW_MC_CLK_CSC,   DW_HDMI_DISABLE);
	dw_mc_set_clk(DW_MC_CLK_AUDIO, DW_HDMI_DISABLE);
	dw_mc_set_clk(DW_MC_CLK_CEC,   DW_HDMI_DISABLE);
	dw_mc_set_clk(DW_MC_CLK_HDCP,  DW_HDMI_DISABLE);
}

u8 dw_mc_irq_get_state(dw_mc_irq_e irq)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(irq_vec); i++) {
		if (irq_vec[i].irq == irq)
			return (u8)dw_read(irq_vec[i].stat_reg);
	}
	hdmi_err("dw irq get state unsupport irq: %d\n", irq);
	return 0;
}

int dw_mc_irq_clear_state(dw_mc_irq_e irq, u8 state)
{
	int i = 0;

	for (i = 0; i <  ARRAY_SIZE(irq_vec); i++) {
		if (irq_vec[i].irq == irq) {
			dw_write(irq_vec[i].stat_reg, state);
			return 0;
		}
	}
	hdmi_err("dw irq clear state unsupport irq: %d\n", irq);
	return -1;
}

int dw_mc_irq_mute(dw_mc_irq_e irq)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(irq_vec); i++) {
		if (irq_vec[i].irq == irq) {
			dw_write(irq_vec[i].mute_reg, 0xFF);
			return 0;
		}
	}
	hdmi_err("dw irq mute unsupport irq: %d\n", irq);
	return -1;
}

int dw_mc_irq_unmute(dw_mc_irq_e irq)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(irq_vec); i++) {
		if (irq_vec[i].irq == irq) {
			dw_write(irq_vec[i].mute_reg, 0x00);
			return 0;
		}
	}
	hdmi_err("dw irq unmute unsupport irq: %d\n", irq);
	return -1;
}

void dw_mc_set_main_irq(u8 state)
{
	hdmi_trace("dw hdmi mc %s main irq\n", state ? "enable" : "disable");
	dw_write(IH_MUTE, state ? 0x0 : 0x3);
}

void dw_mc_irq_mask_all(void)
{
	hdmi_trace("dw hdmi mc mask all irq\n");
	dw_mc_irq_mute(DW_MC_IRQ_FC0);
	dw_mc_irq_mute(DW_MC_IRQ_FC1);
	dw_mc_irq_mute(DW_MC_IRQ_FC2);
	dw_mc_irq_mute(DW_MC_IRQ_AS);
	dw_mc_irq_mute(DW_MC_IRQ_PHY);
	dw_mc_irq_mute(DW_MC_IRQ_I2CM);
	dw_mc_irq_mute(DW_MC_IRQ_VP);
	dw_mc_irq_mute(DW_MC_IRQ_PHYI2C);
	dw_mc_irq_mute(DW_MC_IRQ_AUDIO_DMA);

	dw_mc_set_main_irq(DW_HDMI_ENABLE);
}

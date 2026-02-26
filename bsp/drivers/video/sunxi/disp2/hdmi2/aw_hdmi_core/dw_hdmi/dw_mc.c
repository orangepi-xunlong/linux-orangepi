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
#include "dw_access.h"
#include "dw_mc.h"

typedef struct irq_vector {
	irq_sources_t source;
	unsigned int stat_reg;
	unsigned int mute_reg;
} irq_vector_t;

static irq_vector_t irq_vec[] = {
	{IRQ_AUDIO_PACKETS,	   IH_FC_STAT0,        IH_MUTE_FC_STAT0},
	{IRQ_OTHER_PACKETS,	   IH_FC_STAT1,        IH_MUTE_FC_STAT1},
	{IRQ_PACKETS_OVERFLOW, IH_FC_STAT2,	       IH_MUTE_FC_STAT2},
	{IRQ_AUDIO_SAMPLER,	   IH_AS_STAT0,        IH_MUTE_AS_STAT0},
	{IRQ_PHY,              IH_PHY_STAT0,       IH_MUTE_PHY_STAT0},
	{IRQ_I2C_DDC,          IH_I2CM_STAT0,      IH_MUTE_I2CM_STAT0},
	{IRQ_CEC,              IH_CEC_STAT0,       IH_MUTE_CEC_STAT0},
	{IRQ_VIDEO_PACKETIZER, IH_VP_STAT0,	       IH_MUTE_VP_STAT0},
	{IRQ_I2C_PHY,          IH_I2CMPHY_STAT0,   IH_MUTE_I2CMPHY_STAT0},
	{IRQ_AUDIO_DMA,        IH_AHBDMAAUD_STAT0, IH_MUTE_AHBDMAAUD_STAT0},
	{0, 0, 0},
};

void _dw_mc_csc_clock_enable(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_CLKDIS, MC_CLKDIS_CSCCLK_DISABLE_MASK, bit);
}

void _dw_mc_pixel_repetition_clock_enable(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_CLKDIS, MC_CLKDIS_PREPCLK_DISABLE_MASK, bit);
}

void _dw_mc_tmds_clock_enable(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_CLKDIS, MC_CLKDIS_TMDSCLK_DISABLE_MASK, bit);
}

void _dw_mc_pixel_clock_enable(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_CLKDIS, MC_CLKDIS_PIXELCLK_DISABLE_MASK, bit);
}

void _dw_mc_video_feed_through_off(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_FLOWCTRL, MC_FLOWCTRL_FEED_THROUGH_OFF_MASK, bit);
}

void dw_mc_hdcp_clock_disable(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_CLKDIS, MC_CLKDIS_HDCPCLK_DISABLE_MASK, bit);
}

void dw_mc_cec_clock_enable(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_CLKDIS, MC_CLKDIS_CECCLK_DISABLE_MASK, bit);
}

void dw_mc_audio_sampler_clock_enable(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_CLKDIS, MC_CLKDIS_AUDCLK_DISABLE_MASK, bit);
}

void dw_mc_audio_i2s_reset(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_SWRSTZREQ,
			MC_SWRSTZREQ_II2SSWRST_REQ_MASK, bit);
}

void dw_mc_tmds_clock_reset(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, MC_SWRSTZREQ, MC_SWRSTZREQ_TMDSSWRST_REQ_MASK, bit);
}

void dw_mc_phy_reset(dw_hdmi_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	/* active low */
	dev_write_mask(dev, MC_PHYRSTZ, MC_PHYRSTZ_PHYRSTZ_MASK, bit);
}

void dw_mc_all_clock_enable(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	_dw_mc_video_feed_through_off(dev, 0);
	_dw_mc_pixel_clock_enable(dev, 0);
	_dw_mc_tmds_clock_enable(dev, 0);
	_dw_mc_pixel_repetition_clock_enable(dev,
			(dev->snps_hdmi_ctrl.pixel_repetition > 0) ? 0 : 1);
	_dw_mc_csc_clock_enable(dev, 0);
	dw_mc_audio_sampler_clock_enable(dev,
				dev->snps_hdmi_ctrl.audio_on ? 0 : 1);
#if !IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	dw_mc_cec_clock_enable(dev, dev->snps_hdmi_ctrl.cec_on ? 0 : 1);
#endif
	dw_mc_hdcp_clock_disable(dev, 1);/* disable it */
}

void dw_mc_all_clock_disable(dw_hdmi_dev_t *dev)
{
	_dw_mc_pixel_clock_enable(dev, 1);
	_dw_mc_tmds_clock_enable(dev, 1);
	_dw_mc_pixel_repetition_clock_enable(dev, 1);
	_dw_mc_csc_clock_enable(dev, 1);
	dw_mc_audio_sampler_clock_enable(dev, 1);
#if !IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	dw_mc_cec_clock_enable(dev, 1);
#endif
	dw_mc_hdcp_clock_disable(dev, 1);
}

void dw_mc_all_clock_standby(dw_hdmi_dev_t *dev)
{
	_dw_mc_pixel_clock_enable(dev, 1);
	_dw_mc_tmds_clock_enable(dev, 1);
	_dw_mc_pixel_repetition_clock_enable(dev, 1);
	_dw_mc_csc_clock_enable(dev, 1);
	dw_mc_audio_sampler_clock_enable(dev, 1);
#if !IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	dw_mc_cec_clock_enable(dev, 0);
#endif
	dw_mc_hdcp_clock_disable(dev, 1);
}

u32 dw_mc_get_i2cm_irq_mask(dw_hdmi_dev_t *dev)
{
	return (u32)dev_read_mask(dev, IH_I2CM_STAT0,
		IH_I2CM_STAT0_I2CMASTERERROR_MASK | IH_I2CM_STAT0_I2CMASTERDONE_MASK);
}

int dw_mc_irq_mute_source(dw_hdmi_dev_t *dev, irq_sources_t irq_source)
{
	int i = 0;

	for (i = 0; irq_vec[i].source != 0; i++) {
		if (irq_vec[i].source == irq_source) {
			dev_write(dev, irq_vec[i].mute_reg,  0xff);
			return true;
		}
	}
	hdmi_err("%s: irq source [%d] is not supported\n", __func__, irq_source);
	return false;
}

int dw_mc_irq_unmute_source(dw_hdmi_dev_t *dev, irq_sources_t irq_source)
{
	int i = 0;

	for (i = 0; irq_vec[i].source != 0; i++) {
		if (irq_vec[i].source == irq_source) {
			VIDEO_INF("irq write unmute: irq[%d] mask[%d]\n",
							irq_source, 0x0);
			dev_write(dev, irq_vec[i].mute_reg,  0x00);
			return true;
		}
	}
	hdmi_err("%s: irq source [%d] is not supported\n", __func__, irq_source);
	return false;
}

void dw_mc_irq_all_mute(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	dev_write(dev, IH_MUTE,  0x3);
}

void dw_mc_irq_all_unmute(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	dev_write(dev, IH_MUTE,  0x0);
}

void dw_mc_irq_mask_all(dw_hdmi_dev_t *dev)
{
	LOG_TRACE();
	dw_mc_irq_all_mute(dev);
	dw_mc_irq_mute_source(dev, IRQ_AUDIO_PACKETS);
	dw_mc_irq_mute_source(dev, IRQ_OTHER_PACKETS);
	dw_mc_irq_mute_source(dev, IRQ_PACKETS_OVERFLOW);
	dw_mc_irq_mute_source(dev, IRQ_AUDIO_SAMPLER);
	dw_mc_irq_mute_source(dev, IRQ_PHY);
	dw_mc_irq_mute_source(dev, IRQ_I2C_DDC);
#if !IS_ENABLED(CONFIG_AW_HDMI2_CEC_SUNXI)
	dw_mc_irq_mute_source(dev, IRQ_CEC);
#endif
	dw_mc_irq_mute_source(dev, IRQ_VIDEO_PACKETIZER);
	dw_mc_irq_mute_source(dev, IRQ_I2C_PHY);
	dw_mc_irq_mute_source(dev, IRQ_AUDIO_DMA);
}

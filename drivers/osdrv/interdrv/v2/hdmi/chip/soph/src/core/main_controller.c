#include "core/hdmi_reg.h"
#include "core/main_controller.h"
#include "bsp/access.h"
#include "core/irq.h"

void mc_hdcp_clock_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_CLKDIS, MC_CLKDIS_HDCPCLK_DISABLE_MASK, bit);
}

void mc_cec_clock_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_CLKDIS, MC_CLKDIS_CECCLK_DISABLE_MASK, bit);
}

void mc_colorspace_converter_clock_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_CLKDIS, MC_CLKDIS_CSCCLK_DISABLE_MASK, bit);
}

void mc_audio_sampler_clock_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_CLKDIS, MC_CLKDIS_AUDCLK_DISABLE_MASK, bit);
}

void mc_pixel_repetition_clock_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_CLKDIS, MC_CLKDIS_PREPCLK_DISABLE_MASK, bit);
}

void mc_tmds_clock_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_CLKDIS, MC_CLKDIS_TMDSCLK_DISABLE_MASK, bit);
}

void mc_pixel_clock_enable(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_CLKDIS, MC_CLKDIS_PIXELCLK_DISABLE_MASK, bit);
}

void mc_pixel_repetition_clock_reset(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_SWRSTZREQ, MC_SWRSTZREQ_PREPSWRST_REQ_MASK, bit);
}

void mc_tmds_clock_reset(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_SWRSTZREQ, MC_SWRSTZREQ_TMDSSWRST_REQ_MASK, bit);
}

void mc_pixel_clock_reset(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_SWRSTZREQ, MC_SWRSTZREQ_PIXELSWRST_REQ_MASK, bit);
}

void mc_video_feed_through_off(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(MC_FLOWCTRL, MC_FLOWCTRL_FEED_THROUGH_OFF_MASK, bit);
}

void mc_phy_reset(hdmi_tx_dev_t *dev, u8 bit)
{
	/* active high for (gen2) */
	/* active low  for (thrd_party ack) */
	dev_write_mask(MC_PHYRSTZ, MC_PHYRSTZ_PHYRSTZ_MASK, bit);
	dev->snps_hdmi_ctrl.phy_enable = bit ? 0 : 1;
}

u8 mc_lock_on_clock_status(hdmi_tx_dev_t *dev, u8 clockDomain)
{
	return (u8)((dev_read(MC_LOCKONCLOCK) >> clockDomain) & 0x1);
}

void mc_lock_on_clock_clear(hdmi_tx_dev_t *dev, u8 clockDomain)
{
	dev_write_mask(MC_LOCKONCLOCK, (1<<clockDomain), 1);
}

void mc_disable_all_clocks(hdmi_tx_dev_t *dev)
{
	mc_pixel_clock_enable(dev, 1);
	mc_tmds_clock_enable(dev, 1);
	mc_pixel_repetition_clock_enable(dev, 1);
	mc_colorspace_converter_clock_enable(dev, 1);
	mc_audio_sampler_clock_enable(dev, 1);
//	mc_cec_clock_enable(dev, 1);
	mc_hdcp_clock_enable(dev, 1);
}

void mc_enable_all_clocks(hdmi_tx_dev_t *dev)
{
	mc_video_feed_through_off(dev, dev->snps_hdmi_ctrl.csc_on ? 1 : 0);
	mc_pixel_clock_enable(dev, 0);
	mc_tmds_clock_enable(dev, 0);
	mc_pixel_repetition_clock_enable(dev, (dev->snps_hdmi_ctrl.pixel_repetition > 0) ? 0 : 1);
	mc_colorspace_converter_clock_enable(dev, 0);
	mc_audio_sampler_clock_enable(dev, dev->snps_hdmi_ctrl.audio_on ? 0 : 1);
//	mc_cec_clock_enable(dev, dev->snps_hdmi_ctrl.cec_on ? 0 : 1);
	mc_hdcp_clock_enable(dev, dev->snps_hdmi_ctrl.hdcp_on ? 0 : 1);
}

int control_initialize(hdmi_tx_dev_t *dev)
{
	mc_disable_all_clocks(dev);
	return TRUE;
}

int control_standby(hdmi_tx_dev_t *dev)
{
	mc_disable_all_clocks(dev);
	return TRUE;
}

int control_interrupt_clear_all(hdmi_tx_dev_t *dev)
{
	irq_clear_all(dev);
	return TRUE;
}
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "aw_phy.h"
#include "util.h"
#include "core/irq.h"

static void phy_interrupt_mask(hdmi_tx_dev_t *dev, u8 mask);
static int phy_phase_lock_loop_state(hdmi_tx_dev_t *dev);

static volatile struct __aw_phy_reg_t *phy_base;

void phy_set_reg_base(uintptr_t base)
{
	phy_base = (struct __aw_phy_reg_t *)(base + PHY_REG_OFFSET);
}

uintptr_t phy_get_reg_base(void)
{
	return (uintptr_t)phy_base;
}

static void phy_power_down(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SPARES_2_MASK, (bit ? 1 : 0));
}

static void phy_enable_tmds(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SPARES_1_MASK, (bit ? 1 : 0));
}

static void phy_gen2_pddq(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, (bit ? 1 : 0));
}

static void phy_gen2_tx_power_on(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, (bit ? 1 : 0));
}

static void phy_gen2_en_hpd_rx_sense(hdmi_tx_dev_t *dev, u8 bit)
{
	LOG_TRACE1(bit);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_ENHPDRXSENSE_MASK,
					(bit ? 1 : 0));
}

static void phy_encoding_adapt(encoding_t EncodingOut)
{
	if (EncodingOut != YCC422) {
		phy_base->pll_ctl0.bits.bypass_clrdpth = 1;
		phy_base->pll_ctl0.bits.clr_dpth = 1;
		phy_base->pll_ctl0.bits.div2_ckbit = 1;
		phy_base->pll_ctl0.bits.div2_cktmds = 1;
	}
}

void aw_phy_write(u8 offset, u32 value)
{
	*((unsigned int *)((void *)phy_base + offset)) = value;
	return;
}

void aw_phy_read(u8 offset, u32 *value)
{
	*value = *((unsigned int *)((void *)phy_base + offset));
	return;
}


/*aw_phy clock: unit:kHZ*/
static struct aw_phy_config aw_phy[] = {
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_8,    0x133, 0x0, 0x0, 0x4},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_10,   0x2173, 0x0, 0x0, 0x4},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_12,   0x41B3, 0x0, 0x0, 0x4},
	{13500, PIXEL_REPETITION_1, COLOR_DEPTH_16,   0x6132, 0x8, 0x1, 0x4},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_8,    0x132, 0x8, 0x1, 0x4},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_10,   0x2172, 0x8, 0x1, 0x4},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_12,   0x41B2, 0x8, 0x1, 0x4},
	{13500, PIXEL_REPETITION_3, COLOR_DEPTH_16,   0x6131, 0x18, 0x2, 0x4},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_8,    0x131, 0x18, 0x2, 0x4},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_10,   0x2171, 0x18, 0x2, 0x4},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_12,   0x41B1, 0x18, 0x2, 0x4},
	{13500, PIXEL_REPETITION_7, COLOR_DEPTH_16,   0x6130, 0x30, 0x3, 0x4},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_8,    0x00F2, 0x8, 0x1, 0x4},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_10,   0x2162, 0x8, 0x1, 0x4},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_12,   0x41A2, 0x8, 0x1, 0x4},
	{18000, PIXEL_REPETITION_2, COLOR_DEPTH_16,   0x60F1, 0x18, 0x2, 0x4},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_8,    0x00F1, 0x18, 0x2, 0x4},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_10,   0x2161, 0x18, 0x2, 0x4},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_12,   0x41A1, 0x18, 0x2, 0x4},
	{18000, PIXEL_REPETITION_5, COLOR_DEPTH_16,   0x60F0, 0x31, 0x3, 0x4},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_8,    0x151, 0x18, 0x2, 0x4},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_12,   0x4161, 0x18, 0x2, 0x4},
	{21600, PIXEL_REPETITION_4, COLOR_DEPTH_16,   0x6150, 0x31, 0x3, 0x4},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_8,    0x150, 0x31, 0x3, 0x4},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_12,   0x4160, 0x30, 0x3, 0x4},
	{21600, PIXEL_REPETITION_9, COLOR_DEPTH_16,   0x7B70, 0x001B, 0x3, 0x4},
	{24000, PIXEL_REPETITION_8, COLOR_DEPTH_8,    0x00E0, 0x32, 0x3, 0x4},
	{24000, PIXEL_REPETITION_8, COLOR_DEPTH_16,   0x7BA0, 0x001B, 0x3, 0x4},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{25175, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_8,  0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_10, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_12, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_1, COLOR_DEPTH_16, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_8,  0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_10, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_12, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_3, COLOR_DEPTH_16, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_8,  0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_10, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_12, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{27000, PIXEL_REPETITION_7, COLOR_DEPTH_16, 0x0, 0x0, 0xFFFF, 0xC0D0D0D, 0, 0x10000},
	{31500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x00B3, 0x0, 0x0, 0x4},
	{33750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x00B3, 0x0, 0x0, 0x4},
	{35500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x00B3, 0x0, 0x0, 0x4},
	{36000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x00B3, 0x0, 0x0, 0x4},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_8,  0x00A1, 0x001A, 0x2, 0x4},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_10, 0x2165, 0x18, 0x2, 0x4},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_12, 0x40E1, 0x19, 0x2, 0x4},
	{36000, PIXEL_REPETITION_2, COLOR_DEPTH_16, 0x60A0, 0x32, 0x3, 0x4},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_8,  0x00A0, 0x32, 0x3, 0x4},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_10, 0x2164, 0x30, 0x3, 0x4},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_12, 0x40E0, 0x32, 0x3, 0x4},
	{36000, PIXEL_REPETITION_5, COLOR_DEPTH_16, 0x7AF0, 0x001B, 0x3, 0x4},
	{40000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x00B3, 0x0, 0x0, 0x4},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_8,   0x140, 0x33, 0x3, 0x4},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_12,  0x4164, 0x30, 0x3, 0x4},
	{43200, PIXEL_REPETITION_4, COLOR_DEPTH_16,  0x7B50, 0x001B, 0x3, 0x4},
	{44900, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x00B3, 0x0, 0x0, 0x4},
	{49500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x72, 0x8, 0x1, 0x4},
	{50000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x72, 0x8, 0x1, 0x4},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_8, 0x72, 0x8, 0x1, 0x4},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x2142, 0x8, 0x1, 0x4},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x40A2, 0x8, 0x1, 0x4},
	{50350, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x6071, 0x001A, 0x2, 0x4},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x72, 0x8, 0x1, 0x4},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x2142, 0x8, 0x1, 0x4},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x40A2, 0x8, 0x1, 0x4},
	{54000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x6071, 0x001A, 0x2, 0x4},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_8,    0x71, 0x001A, 0x2, 0x4},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_10,   0x2141, 0x001A, 0x2, 0x4},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_12,   0x40A1, 0x001A, 0x2, 0x4},
	{54000, PIXEL_REPETITION_1, COLOR_DEPTH_16,   0x6070, 0x33, 0x3, 0x4},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_8,    0x70, 0x33, 0x3, 0x4},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_10,   0x2140, 0x33, 0x3, 0x4},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_12,   0x40A0, 0x32, 0x3, 0x4},
	{54000, PIXEL_REPETITION_3, COLOR_DEPTH_16,   0x7AB0, 0x001B, 0x3, 0x4},
	{56250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x72, 0x8, 0x1, 0x4},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x72, 0x8, 0x1, 0x4},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x2142, 0x8, 0x1, 0x4},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x40A2, 0x8, 0x1, 0x4},
	{59400, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x6071, 0x001A, 0x2, 0x4},
	{65000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x72, 0x8, 0x1, 0x4},
	{68250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x72, 0x8, 0x1, 0x4},
	{71000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x72, 0x8, 0x1, 0x4},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x72, 0x8, 0x1, 0x4},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10,  0x2142, 0x8, 0x1, 0x4},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12,  0x4061, 0x001B, 0x2, 0x4},
	{72000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16,  0x6071, 0x001A, 0x2, 0x4},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_8,     0x60, 0x34, 0x3, 0x4},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_10,    0x216C, 0x30, 0x3, 0x4},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_12,    0x40E4, 0x32, 0x3, 0x4},
	{72000, PIXEL_REPETITION_2, COLOR_DEPTH_16,    0x7AA0, 0x001B, 0x3, 0x4},
	{73250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x0, 0x0, 0xFFFF, 0xC0D0D0D},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_10,  0x0, 0x0, 0xFFFF, 0xC0D0D0D},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_12,  0x0, 0x0, 0xFFFF, 0xC0D0D0D},
	{74250, PIXEL_REPETITION_OFF, COLOR_DEPTH_16,  0x0, 0x0, 0xFFFF, 0xC0D0D0D},
	{75000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{78750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{79500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_10,  0x2145, 0x001A, 0x2, 0x4},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_12,  0x4061, 0x001B, 0x2, 0x4},
	{82500, PIXEL_REPETITION_OFF, COLOR_DEPTH_16,  0x6071, 0x001A, 0x2, 0x4},
	{83500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{85500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{88750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x72, 0x8, 0x1, 0x4},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10,  0x2145, 0x001A, 0x2, 0x4},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12,  0x4061, 0x001B, 0x2, 0x4},
	{90000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16,  0x6071, 0x001A, 0x2, 0x4},
	{94500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x51, 0x001B, 0x2, 0x4},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,   0x51, 0x001B, 0x2, 0x4},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10,  0x2145, 0x001A, 0x2, 0x4},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12,  0x4061, 0x001B, 0x2, 0x4},
	{99000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16,  0x6050, 0x35, 0x3, 0x4},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x2145, 0x001A, 0x2, 0x4},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4061, 0x001B, 0x2, 0x4},
	{100700, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x6050, 0x35, 0x3, 0x4},
	{101000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{102250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{106500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x2145, 0x001A, 0x2, 0x4},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4061, 0x001B, 0x2, 0x4},
	{108000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x6050, 0x35, 0x3, 0x4},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_8,    0x50, 0x35, 0x3, 0x4},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_10,   0x2144, 0x33, 0x3, 0x4},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_12,   0x4060, 0x34, 0x3, 0x4},
	{108000, PIXEL_REPETITION_1, COLOR_DEPTH_16,   0x7A70, 0x33, 0x3, 0x4},
	{115500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{117500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x2145, 0x001A, 0x2, 0x4},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4061, 0x001B, 0x2, 0x4},
	{118800, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x6050, 0x35, 0x3, 0x4},
	{119000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{121750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{122500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{135000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{136750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{140250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x2145, 0x001A, 0x2, 0x4},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4064, 0x34, 0x3, 0x4},
	{144000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x6050, 0x35, 0x3, 0x4},
	{146250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{148250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x0, 0x0, 0xFFFF, 0xC0D0D0D},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x0, 0x0, 0xFFFF, 0xC0D0D0D},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x0, 0x0, 0xFFFF, 0xC0D0D0D},
	{148500, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x0, 0x0, 0xFFFF, 0xC0D0D0D},
	{154000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{156000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{157000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{157500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{162000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x214C, 0x33, 0x3, 0x4},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4064, 0x34, 0x3, 0x4},
	{165000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x6050, 0x35, 0x3, 0x4},
	{175500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{179500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x214C, 0x33, 0x3, 0x4},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4064, 0x34, 0x3, 0x4},
	{180000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x7A50, 0x001B, 0x3, 0x4},
	{182750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x51, 0x001B, 0x2, 0x4},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x214C, 0x33, 0x3, 0x4},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4064, 0x34, 0x3, 0x4},
	{185625, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x7A50, 0x001B, 0x3, 0x4},
	{187000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{187250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{189000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{193250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x214C, 0x33, 0x3, 0x4},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4064, 0x34, 0x3, 0x4},
	{198000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x7A50, 0x001B, 0x3, 0x4},
	{202500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{204750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{208000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{214750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x214C, 0x33, 0x3, 0x4},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x4064, 0x34, 0x3, 0x4},
	{216000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x7A50, 0x001B, 0x3, 0x4},
	{218250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{229500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{234000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x214C, 0x33, 0x3, 0x4},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x5A64, 0x001B, 0x3, 0x4},
	{237600, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x7A50, 0x001B, 0x3, 0x4},
	{245250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{245500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{261000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{268250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{268500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{281250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x3B4C, 0x001B, 0x3, 0x4},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x5A64, 0x001B, 0x3, 0x4},
	{288000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x7A50, 0x003D, 0x3, 0x4},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x0, 0x01084F, 0x0444, 0x1F1F1F1F},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x0, 0x01084F, 0x0444, 0x1F1F1F1F},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x0, 0x01084F, 0x0444, 0x1F1F1F1F},
	{297000, PIXEL_REPETITION_OFF, COLOR_DEPTH_16, 0x0, 0x01084F, 0x0444, 0x1F1F1F1F},
	{317000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x3B4C, 0x001B, 0x3, 0x4},
	{330000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x5A64, 0x001B, 0x3, 0x4},
	{333250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{340000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x40, 0x36, 0x3, 0x4},
	{348500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{356500, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x3B4C, 0x001B, 0x3, 0x4},
	{360000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x5A64, 0x001B, 0x3, 0x4},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x3B4C, 0x001B, 0x3, 0x4},
	{371250, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x5A64, 0x001B, 0x3, 0x4},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x3B4C, 0x001B, 0x3, 0x4},
	{396000, PIXEL_REPETITION_OFF, COLOR_DEPTH_12, 0x5A64, 0x001B, 0x3, 0x4},
	{432000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{432000, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x3B4C, 0x001B, 0x3, 0x4},
	{443250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{475200, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{475200, PIXEL_REPETITION_OFF, COLOR_DEPTH_10, 0x3B4C, 0x001B, 0x3, 0x4},
	{495000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{505250, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{552750, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A40, 0x003F, 0x3, 0x4},
	{594000, PIXEL_REPETITION_OFF, COLOR_DEPTH_8,  0x1A7c, 0x0010, 0x3, 0x0},
	{0, PIXEL_REPETITION_OFF, COLOR_DEPTH_INVALID, 0, 0, 0, 0}
};

static u32 phy_get_freq(u32 pClk)
{
	if (((pClk >= 25175) && (pClk <= 25180)) ||
			((pClk >= 25195) && (pClk <= 25205)))
		return 25175;
	else if (((pClk >= 26995) && (pClk <= 27005)) ||
			((pClk >= 27022) && (pClk <= 27032)))
		return 27000;
	else if (pClk == 31500)
		return 31500;
	else if (pClk == 33750)
		return 33750;
	else if (pClk == 35500)
		return 35500;
	else if (((pClk >= 35995) && (pClk <= 36005)) ||
			((pClk >= 36031) && (pClk <= 36041)))
		return 36000;
	else if (pClk == 40000)
		return 40000;
	else if (pClk == 44900)
		return 44900;
	else if (pClk == 49500)
		return 49500;
	else if (pClk == 50000)
		return 50000;
	else if (((pClk >= 50345) && (pClk <= 50355)) ||
					((pClk >= 50395) && (pClk <= 50405)))
		return 50350;
	else if (((pClk >= 53995) && (pClk <= 54005)) ||
					((pClk >= 50049) && (pClk <= 54059)))
		return 54000;
	else if (pClk == 56250)
		return 56250;
	else if (((pClk >= 59336) && (pClk <= 59346)) ||
					((pClk >= 59395) && (pClk <= 59405)))
		return 59400;
	else if (pClk == 65000)
		return 65000;
	else if (pClk == 68250)
		return 68250;
	else if (pClk == 71000)
		return 71000;
	else if (pClk == 72000)
		return 72000;
	else if (pClk == 73250)
		return 73250;
	else if (((pClk >= 74171) && (pClk <= 74181)) ||
					((pClk >= 74245) && (pClk <= 74255)))
		return 74250;
	else if (pClk == 75000)
		return 75000;
	else if (pClk == 78750)
		return 78750;
	else if (pClk == 79500)
		return 79500;
	else if (((pClk >= 82143) && (pClk <= 82153)) ||
					((pClk >= 82495) && (pClk <= 82505)))
		return 82500;
	else if (pClk == 83500)
		return 83500;
	else if (pClk == 85500)
		return 85500;
	else if (pClk == 88750)
		return 88750;
	else if (pClk == 90000)
		return 90000;
	else if (pClk == 94500)
		return 94500;
	else if (((pClk >= 98896) && (pClk <= 98906)) ||
					((pClk >= 98995) && (pClk <= 99005)))
		return 99000;
	else if (((pClk >= 100695) && (pClk <= 100705)) ||
					((pClk >= 100795) && (pClk <= 100805)))
		return 100700;
	else if (pClk == 101000)
		return  101000;
	else if (pClk == 102250)
		return 102250;
	else if (pClk == 106500)
		return 106500;
	else if (((pClk > 107994) && (pClk < 108006)) ||
					((pClk > 108102) && (pClk < 108114)))
		return 108000;
	else if (pClk == 115500)
		return 115500;
	else if (pClk == 117500)
		return 117500;
	else if (((pClk >= 118795) && (pClk <= 118805)) ||
					((pClk >= 118677) && (pClk <= 118687)))
		return 118800;
	else if (pClk == 119000)
		return 119000;
	else if (pClk == 121750)
		return 122500;
	else if (pClk == 122500)
		return 121750;
	else if (pClk == 135000)
		return 135000;
	else if (pClk == 136750)
		return 136750;
	else if (pClk == 140250)
		return 140250;
	else if ((pClk == 144000) || (pClk == 142250) || (pClk == 142000))
		return 144000;
	else if (pClk == 146250)
		return 146250;
	else if (pClk == 148250)
		return 148250;
	else if (((pClk >= 148347) && (pClk <= 148357)) ||
					((pClk >= 148495) && (pClk <= 148505)))
		return 148500;
	else if (pClk == 154000)
		return 154000;
	else if (pClk == 156000)
		return 156000;
	else if (pClk == 157000)
		return 157000;
	else if (pClk == 157500)
		return 157500;
	else if (pClk == 162000)
		return 162000;
	else if (((pClk >= 164830) && (pClk <= 164840)) ||
					((pClk >= 164995) && (pClk <= 165005)))
		return 165000;
	else if (pClk == 175500)
		return 175500;
	else if (pClk == 179500)
		return 179500;
	else if (pClk == 180000)
		return 180000;
	else if (pClk == 182750)
		return 182750;
	else if (((pClk >= 185435) && (pClk <= 185445)) ||
				((pClk >= 185620) && (pClk <= 185630)))
		return 185625;
	else if (pClk == 187000)
		return 187000;
	else if (pClk == 187250)
		return 187250;
	else if (pClk == 189000)
		return 189000;
	else if (pClk == 193250)
		return 193250;
	else if (((pClk >= 197797) && (pClk <= 197807)) ||
				((pClk >= 197995) && (pClk <= 198005)))
		return 198000;
	else if (pClk == 202500)
		return 202500;
	else if (pClk == 204750)
		return 204750;
	else if (pClk == 208000)
		return 208000;
	else if (pClk == 214750)
		return 214750;
	else if (((pClk >= 216211) && (pClk <= 216221)) ||
				((pClk >= 215995) && (pClk <= 216005)))
		return 216000;
	else if (pClk == 218250)
		return 218250;
	else if (pClk == 229500)
		return 229500;
	else if ((pClk == 234000) || (pClk == 233500))
		return 234000;
	else if (((pClk >= 237359) && (pClk <= 237369)) ||
				((pClk >= 237595) && (pClk <= 237605)))
		return 237600;
	else if (pClk == 245250)
		return 245250;
	else if (pClk == 245500)
		return 245500;
	else if (pClk == 261000)
		return 261000;
	else if (pClk == 268250)
		return 268250;
	else if (pClk == 268500)
		return 268500;
	else if ((pClk == 281250) || (pClk == 280000))
		return 281250;
	else if (pClk == 288000)
		return 288000;
	else if (((pClk >= 296698) && (pClk <= 296708)) ||
				((pClk >= 296995) && (pClk <= 297005)))
		return 297000;
	else if (pClk == 317000)
		return 317000;
	else if (pClk == 330000)
		return 330000;
	else if (pClk == 333250)
		return 333250;
	else if (((pClk >= 339655) && (pClk <= 339665)) ||
				((pClk >= 339995) && (pClk <= 340005)))
		return 340000;
	else if (pClk == 348500)
		return 348500;
	else if (pClk == 356500)
		return 356500;
	else if (pClk == 360000)
		return 360000;
	else if (((pClk >= 370874) && (pClk <= 370884)) ||
				((pClk >= 371245) && (pClk <= 371255)))
		return 371250;
	else if (pClk == 380500)
		return 380500;
	else if (((pClk >= 395599) && (pClk <= 395609)) ||
				((pClk >= 395995) && (pClk <= 396005)))
		return 396000;
	else if (((pClk >= 431952) && (pClk <= 431967)) ||
			((pClk >= 431995) && (pClk <= 432005)) ||
			((pClk >= 432427) && (pClk <= 432437)))
		return 432000;
	else if (pClk == 443250)
		return 443250;
	else if (((pClk >= 475148) && (pClk <= 475158)) ||
		((pClk >= 475195) && (pClk <= 475205)) ||
		((pClk >= 474723) && (pClk <= 474733)))
		return 475200;
	else if (((pClk >= 494500) && (pClk <= 494510)) ||
		((pClk >= 494995) && (pClk <= 495005)))
		return 495000;
	else if (pClk == 505250)
		return 505250;
	else if (pClk == 552750)
		return 552750;
	else if (((pClk >= 593995) && (pClk <= 594005)) ||
		((pClk >= 593403) && (pClk <= 593413)))
		return 594000;
	else {
		pr_err("Error:Unable to map input pixel clock frequency %d kHz\n", pClk);
	}
	return 1000;
}

static struct aw_phy_config *phy_get_configs(u32 pClk, color_depth_t color,
		pixel_repetition_t pixel)
{

	int i = 0;

	for (i = 0; aw_phy[i].clock != 0; i++) {

		pClk = phy_get_freq(pClk);

		if ((pClk == aw_phy[i].clock) &&
				(color == aw_phy[i].color) &&
				(pixel == aw_phy[i].pixel)) {
			return &(aw_phy[i]);
		}
	}
	pr_info("EEROR: do not get phy_config\n");

	return NULL;
}

void phy_reset(void)
{
	phy_base->phy_ctl0.bits.entx = 0;
	phy_base->phy_ctl5.bits.enresck = 0;
	phy_base->phy_ctl5.bits.enres = 0;
	phy_base->phy_ctl5.bits.enp2s = 0;
	phy_base->phy_ctl0.bits.enck = 0;
	phy_base->phy_ctl0.bits.enbi = 0;
	phy_base->phy_ctl5.bits.encalog = 0;
	phy_base->phy_ctl5.bits.enrcal = 0;
	phy_base->phy_ctl0.bits.enldo_fs = 0;
	phy_base->phy_ctl0.bits.enldo = 0;
	phy_base->phy_ctl5.bits.enib = 0;
	phy_base->pll_ctl1.bits.reset = 1;
	phy_base->pll_ctl1.bits.pwron = 0;
	phy_base->pll_ctl0.bits.envbs = 0;

}


static void phy_set_mpll(void)
{
	phy_base->pll_ctl0.bits.cko_sel = 0x3;
	phy_base->pll_ctl0.bits.bypass_ppll = 0x1;
	phy_base->pll_ctl1.bits.drv_ana = 1;
	phy_base->pll_ctl1.bits.ctrl_modle_clksrc = 0x0; //0: PLL_video   1: MPLL
	phy_base->pll_ctl1.bits.sdm_en = 0x0;            //mpll sdm jitter 很大，暂不使用
	phy_base->pll_ctl1.bits.sckref = 0;        //默认值为1
	phy_base->pll_ctl0.bits.slv = 4;
	phy_base->pll_ctl0.bits.prop_cntrl = 7;   //默认值7
	phy_base->pll_ctl0.bits.gmp_cntrl = 3;    //默认值1
	phy_base->pll_ctl1.bits.ref_cntrl = 0;
	phy_base->pll_ctl0.bits.vcorange = 1;

}

static void phy_set_div(void)
{
	phy_base->pll_ctl0.bits.div_pre = 0;      //div7 = n+1
	phy_base->pll_ctl1.bits.pcnt_en = 0;
	phy_base->pll_ctl1.bits.pcnt_n = 1;       //div6 = 1 (pcnt_en=0)    [div6 = n (pcnt_en = n 注意部分倍数有问题)]  4-256
	phy_base->pll_ctl1.bits.pixel_rep = 0;    //div5 = n+1
	phy_base->pll_ctl0.bits.bypass_clrdpth = 0;
	phy_base->pll_ctl0.bits.clr_dpth = 0;     //div4 = 1 (bypass_clrdpth = 0)
	//00: 2    01: 2.5  10: 3   11: 4
	phy_base->pll_ctl0.bits.n_cntrl = 1;      //div
	phy_base->pll_ctl0.bits.div2_ckbit = 0;   //div1 = n+1
	phy_base->pll_ctl0.bits.div2_cktmds = 0;  //div2 = n+1
	phy_base->pll_ctl0.bits.bcr = 0;          //div3    0: [1:10]  1: [1:40]

	phy_base->pll_ctl1.bits.pwron = 1;
	phy_base->pll_ctl1.bits.reset = 0;
}

static void phy_set_clk(void)
{
	phy_base->phy_ctl6.bits.switch_clkch_data_corresponding = 0;
	phy_base->phy_ctl6.bits.clk_greate0_340m = 0x3FF;
	phy_base->phy_ctl6.bits.clk_greate1_340m = 0x3FF;
	phy_base->phy_ctl6.bits.clk_greate2_340m = 0x0;
	phy_base->phy_ctl7.bits.clk_greate3_340m = 0x0;
	phy_base->phy_ctl7.bits.clk_low_340m = 0x3E0;
	phy_base->phy_ctl6.bits.en_ckdat = 1;       //默认值为0

	//phy_base->phy_ctl2.bits.reg_resdi = 0x18;
	//phy_base->phy_ctl4.bits.reg_slv = 3;         //low power voltage 1.08V, 默认值是3

	phy_base->phy_ctl1.bits.res_scktmds = 0;  //
	phy_base->phy_ctl0.bits.reg_csmps = 2;
	phy_base->phy_ctl0.bits.reg_ck_test_sel = 0;  //?
	phy_base->phy_ctl0.bits.reg_ck_sel = 1;
	phy_base->phy_indbg_ctrl.bits.txdata_debugmode = 0;
}

static void phy_i2c_slave_address(hdmi_tx_dev_t *dev, u8 value)
{
	LOG_TRACE1(value);
	dev_write_mask(dev, PHY_I2CM_SLAVE,
			PHY_I2CM_SLAVE_SLAVEADDR_MASK, value);
}

static int phy_slave_address(hdmi_tx_dev_t *dev, u8 value)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
	case PHY_I2C:
		phy_i2c_slave_address(dev, value);
		return 0;
	default:
		pr_err("PHY interface not defined");
	}
	return -1;
}

static int phy_reconfigure_interface(hdmi_tx_dev_t *dev)
{
	switch (dev->snps_hdmi_ctrl.phy_access) {
	case PHY_I2C:
		dev_write(dev, JTAG_PHY_CONFIG, JTAG_PHY_CONFIG_I2C_JTAGZ_MASK);
		phy_slave_address(dev, PHY_I2C_SLAVE_ADDR);
		break;
	default:
		pr_err("PHY interface not defined");
		return -1;
	}
	VIDEO_INF("PHY interface reconfiguration, set to %s",
		dev->snps_hdmi_ctrl.phy_access == PHY_I2C ? "I2C" : "JTAG");
	return 0;

}

static int phy_enable(void)
{
	int i = 0, status = 0;
	//enib -> enldo -> enrcal -> encalog -> enbi[3:0] -> enck -> enp2s[3:0] -> enres -> enresck -> entx[3:0]
	phy_base->phy_ctl4.bits.reg_slv = 4;     //low power voltage 1.08V, default is 3, set 4 as well as pll_ctl0 bit [24:26]
	phy_base->phy_ctl5.bits.enib = 1;
	phy_base->phy_ctl0.bits.enldo = 1;
	phy_base->phy_ctl0.bits.enldo_fs = 1;
	phy_base->phy_ctl5.bits.enrcal = 1;

	phy_base->phy_ctl5.bits.encalog = 1;

	for (i = 0; i < AW_PHY_TIMEOUT; i++) {
		snps_sleep(5);
		status = phy_base->phy_pll_sts.bits.phy_rcalend2d_status;
		if (status & 0x1) {
			VIDEO_INF("[%s]:phy_rcalend2d_status\n", __func__);
			break;
		}
	}
	if ((i == AW_PHY_TIMEOUT) && !status) {
		pr_err("phy_rcalend2d_status Timeout !\n");
		return -1;
	}

	phy_base->phy_ctl0.bits.enbi = 0xF;
	for (i = 0; i < AW_PHY_TIMEOUT; i++) {
		snps_sleep(5);
		status = phy_base->phy_pll_sts.bits.pll_lock_status;
		if (status & 0x1) {
			VIDEO_INF("[%s]:pll_lock_status\n", __func__);
			break;
		}
	}
	if ((i == AW_PHY_TIMEOUT) && !status) {
		pr_err("pll_lock_status Timeout! status = 0x%x\n", status);
		return -1;
	}

	phy_base->phy_ctl0.bits.enck = 1;
	phy_base->phy_ctl5.bits.enp2s = 0xF;
	phy_base->phy_ctl5.bits.enres = 1;
	phy_base->phy_ctl5.bits.enresck = 1;
	phy_base->phy_ctl0.bits.entx = 0xF;

	for (i = 0; i < AW_PHY_TIMEOUT; i++) {
		snps_sleep(5);
		status = phy_base->phy_pll_sts.bits.tx_ready_dly_status;
		if (status & 0x1) {
			VIDEO_INF("[%s]:tx_ready_status\n", __func__);
			break;
		}
	}
	if ((i == AW_PHY_TIMEOUT) && !status) {
		pr_err("tx_ready_status Timeout ! status = 0x%x\n", status);
		return -1;
	}

	phy_base->phy_ctl0.bits.sda_en = 1;
	phy_base->phy_ctl0.bits.scl_en = 1;
	phy_base->phy_ctl0.bits.hpd_en = 1;
	phy_base->phy_ctl0.bits.reg_den = 0xF;
	phy_base->pll_ctl0.bits.envbs = 1;
	return 0;
}

int phy_config_resume(void)
{
	int ret = 0;
	/* close phy and mpLL*/
	phy_reset();

	//mpll configuration
	phy_set_mpll();

	//Frequency division configuration
	phy_set_div();

	//clk configuration
	phy_set_clk();

	//enable
	ret = phy_enable();
	if (ret < 0)
		pr_err("[%s]: phy enable failed!\n", __func__);
	return ret;
}

static int __phy_config(struct aw_phy_config *config)
{
	int ret = 0;
	/* 先关闭PHY 和 mPLL */
	phy_reset();

	//配置mpll
	phy_set_mpll();

	//分频配置
	phy_set_div();

	//配置phy
	phy_base->phy_ctl1.dwval = ((phy_base->phy_ctl1.dwval & 0xFFC0FFFF) | config->phy_ctl1);
	phy_base->phy_ctl2.dwval = ((phy_base->phy_ctl2.dwval & 0xFF000000) | config->phy_ctl2);
	phy_base->phy_ctl3.dwval = ((phy_base->phy_ctl3.dwval & 0xFFFF0000) | config->phy_ctl3);
	phy_base->phy_ctl4.dwval = ((phy_base->phy_ctl4.dwval & 0xE0000000) | config->phy_ctl4);
	//phy_base->pll_ctl0.dwval |= config->pll_ctl0;
	//phy_base->pll_ctl1.dwval |= config->pll_ctl1;

	//配置时钟
	phy_set_clk();

	//使能
	ret = phy_enable();
	if (ret < 0) {
		pr_err("[%s]: phy enable failed!\n", __func__);
	}
	return ret;
}
static int phy_aw_configure(hdmi_tx_dev_t *dev, u32 pClk, color_depth_t color,
						pixel_repetition_t pixel,
						encoding_t EncodingOut)
{
	int i = 0, ret = 0;
	u8 lock = 0;
	struct aw_phy_config *config = NULL;
	/* value = 0;*/

	LOG_TRACE();
	/* Color resolution 0 is 8 bit color depth */
	if (color == 0)
		color = COLOR_DEPTH_8;

	config = phy_get_configs(pClk, color, pixel);

	if (config == NULL) {
		pr_err("Error:failed to get phy config\n");
		pr_err("Configuration for clk %dkhz color depth %d pixel repetition %d\n",
				  pClk, color, pixel);
		return -1;
	}

	mc_phy_reset(dev, 1);
	phy_reconfigure_interface(dev);

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, 0);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, 1);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SVSRET_MASK, 0);

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_SVSRET_MASK, 1);

	mc_phy_reset(dev, 0);

	/* enable all channel */
	phy_base->phy_ctl5.bits.reg_p1opt = 0xF;

	ret = __phy_config(config);
	if (ret < 0)
		return FALSE;

	if (color == COLOR_DEPTH_10) {
		phy_encoding_adapt(EncodingOut);
	}

	if (EncodingOut == YCC420)
		phy_base->pll_ctl0.bits.div_pre = 1;

	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, 0);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, 1);

	/* wait LOCK_TIMEOUT no of cycles at most for
	the PLL lock signal to raise ~around 20us max */
	for (i = 0; i < LOCK_TIMEOUT; i++) {
		snps_sleep(5);
		lock = phy_phase_lock_loop_state(dev);
		if (lock & 0x1) {
			VIDEO_INF("[%s]:PHY PLL locked\n", __func__);
			return TRUE;
		}
	}

	pr_err("PHY PLL not locked\n");
	return FALSE;
}

int phy_configure(hdmi_tx_dev_t *dev, u16 phy_model, encoding_t EncodingOut)
{
	LOG_TRACE();
	return phy_aw_configure(dev, dev->snps_hdmi_ctrl.pixel_clock,
					dev->snps_hdmi_ctrl.color_resolution,
					dev->snps_hdmi_ctrl.pixel_repetition,
					EncodingOut);
}

int phy_standby(hdmi_tx_dev_t *dev)
{
	phy_interrupt_mask(dev, PHY_MASK0_TX_PHY_LOCK_MASK |
				PHY_MASK0_RX_SENSE_0_MASK |
				PHY_MASK0_RX_SENSE_1_MASK |
				PHY_MASK0_RX_SENSE_2_MASK |
				PHY_MASK0_RX_SENSE_3_MASK);
				/* mask phy interrupts - leave HPD */
	phy_enable_tmds(dev, 0);
	phy_power_down(dev, 0);	/*  disable PHY */
	phy_gen2_tx_power_on(dev, 0);
	phy_gen2_pddq(dev, 1);
	return TRUE;
}

int phy_enable_hpd_sense(hdmi_tx_dev_t *dev)
{
	phy_gen2_en_hpd_rx_sense(dev, 1);
	return TRUE;
}

int phy_disable_hpd_sense(hdmi_tx_dev_t *dev)
{
	phy_gen2_en_hpd_rx_sense(dev, 0);
	return TRUE;
}



static int phy_phase_lock_loop_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_TX_PHY_LOCK_MASK);
}

static void phy_interrupt_mask(hdmi_tx_dev_t *dev, u8 mask)
{
	LOG_TRACE1(mask);
	/* Mask will determine which bits will be enabled */
	dev_write_mask(dev, PHY_MASK0, mask, 0xff);
}

u8 phy_hot_plug_state(hdmi_tx_dev_t *dev)
{
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_HPD_MASK);
}

u8 phy_rxsense_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return (u8)(dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_0_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_1_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_2_MASK) |
		dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_RX_SENSE_3_MASK));
}
u8 phy_pll_lock_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, (PHY_STAT0), PHY_STAT0_TX_PHY_LOCK_MASK);
}

u8 phy_power_state(hdmi_tx_dev_t *dev)
{
	LOG_TRACE();
	return dev_read_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK);
}
void phy_power_enable(hdmi_tx_dev_t *dev, u8 enable)
{
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_PDDQ_MASK, !enable);
	dev_write_mask(dev, PHY_CONF0, PHY_CONF0_TXPWRON_MASK, enable);

}

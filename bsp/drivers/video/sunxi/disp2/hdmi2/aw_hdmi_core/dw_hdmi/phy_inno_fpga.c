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
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/acpi.h>
#include <linux/err.h>
#include <linux/regmap.h>
#include "dw_access.h"
#include "dw_phy.h"
#include "dw_mc.h"
#include "phy_inno_fpga.h"

#define INNOY_PHY_I2C_DEV1_ADDR 0x46
#define INNOY_PHY_I2C_DEV2_ADDR 0x56

static struct inno_phy_fpga_config inno_phy_fpga[] = {
	{27000,  DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x02, 0xf0, 0x28, 0x35, 0x61, 0x64, 0x01, 0x28, 0x03, 0x0e, 0x00},
	{148500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_8,  0x10, 0xf0, 0x6e, 0x30, 0x60, 0x42, 0x04, 0x50, 0x01, 0x0e, 0x00},
	{148500, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_10,  0x10, 0xf0, 0x6e, 0x30, 0x60, 0x42, 0x04, 0x50, 0x01, 0x0e, 0x00},
	{0, DW_PIXEL_REPETITION_OFF, DW_COLOR_DEPTH_NULL, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
};

void *get_innophy_regmap(u16 addr)
{
	if ((addr >> 8 & 0xff) == INNOY_PHY_I2C_DEV1_ADDR)
		return get_innophy46_regmap();
	else if ((addr >> 8 & 0xff) == INNOY_PHY_I2C_DEV2_ADDR)
		return get_innophy56_regmap();
	else
		hdmi_err("I2C device address is error");
	return NULL;
}

void inno_phy_fpga_write(u16 addr, u8 value)
{
	struct regmap *regmap = get_innophy_regmap(addr);

	if (!regmap)
		regmap_write(regmap, (addr & 0xff), value);
	return;
}

void inno_phy_fpga_read(u16 addr, u32 *value)
{
	struct regmap *regmap = get_innophy_regmap(addr);

	if (!regmap)
		regmap_read(regmap, (addr & 0xff), value);
	return;
}

void _inno_phy_fpga_init(struct regmap *regmap46)
{
	regmap_write(regmap46, 0x00, 0x43);
	regmap_write(regmap46, 0x00, 0x03);
	regmap_write(regmap46, 0x00, 0x43);
	regmap_write(regmap46, 0x00, 0x63);
}

void _inno_phy_fpga_power_on(struct regmap *regmap46)
{
	regmap_write(regmap46, 0x00, 0x61);
	regmap_write(regmap46, 0xce, 0x00);
	regmap_write(regmap46, 0xce, 0x01);
}

static struct inno_phy_fpga_config *_inno_phy_fpga_auto_get_config(u32 pClk,
		dw_color_depth_t color, dw_pixel_repetition_t pixel)
{
	int index = 0;
	u32 ref_clk = 0;
	struct inno_phy_fpga_config *inno_phy = NULL;
	int size = 0;

	inno_phy = inno_phy_fpga;
	size = (sizeof(inno_phy_fpga) / sizeof(struct inno_phy_fpga_config) - 1);

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

int inno_phy_fpga_configure(dw_hdmi_dev_t *dev)
{
	//u32 val;
	struct inno_phy_fpga_config *config = NULL;
	u32 pClk = dev->snps_hdmi_ctrl.tmds_clk;
	dw_color_depth_t color = dev->snps_hdmi_ctrl.color_resolution;
	dw_pixel_repetition_t pixel = dev->snps_hdmi_ctrl.pixel_repetition;

	struct regmap *regmap46 = get_innophy46_regmap();
	struct regmap *regmap56 = get_innophy56_regmap();

	/* Color resolution 0 is 8 bit color depth */
	if (color == 0)
		color = DW_COLOR_DEPTH_8;

	config = _inno_phy_fpga_auto_get_config(pClk, color, pixel);
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

	_inno_phy_fpga_init(regmap46);

	regmap_write(regmap56, 0xa0, 0x01);
	regmap_write(regmap56, 0xaa, 0x0f);

	regmap_write(regmap56, 0xa1, config->prell_addr_56a1);
	regmap_write(regmap56, 0xa2, config->prell_addr_56a2);
	regmap_write(regmap56, 0xa3, config->prell_addr_56a3);
	regmap_write(regmap56, 0xa4, config->prell_addr_56a4);

	regmap_write(regmap56, 0xa5, config->prell_addr_56a5);
	regmap_write(regmap56, 0xa6, config->prell_addr_56a6);
	regmap_write(regmap56, 0xab, config->prell_addr_56ab);
	regmap_write(regmap56, 0xac, config->prell_addr_56ac);
	regmap_write(regmap56, 0xad, config->prell_addr_56ad);
	regmap_write(regmap56, 0xaa, config->prell_addr_56aa);
	regmap_write(regmap56, 0xa0, config->prell_addr_56a0);

	regmap_write(regmap46, 0x9f, 0x06);

	regmap_write(regmap46, 0xa7, config->prell_addr_46a7);

	//regmap_write(regmap46, 0xc9, 0x00);  //colorbar
	regmap_write(regmap46, 0xc9, 0x04);   //normal

	_inno_phy_fpga_power_on(regmap46);

	if (dw_phy_wait_lock(dev) == 1) {
		hdmi_inf("inno phy pll locked!\n");
		return true;
	}

	hdmi_err("%s: inno phy pll not locked\n", __func__);
	return false;
}

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

#ifndef INNO_PHY_FPGA_H_
#define INNO_PHY_FPGA_H_

#include "dw_dev.h"

struct inno_phy_fpga_config {
	u32 clock;/* tmds clock: unit:kHZ */
	dw_pixel_repetition_t	pixel;
	dw_color_depth_t		color;

	u8 prell_addr_46a7;

	u8 prell_addr_56a1;

	u8 prell_addr_56a2;

	u8 prell_addr_56a3;

	u8 prell_addr_56a4;

	u8 prell_addr_56a5;

	u8 prell_addr_56a6;

	u8 prell_addr_56ab;

	u8 prell_addr_56ac;

	u8 prell_addr_56ad;

	u8 prell_addr_56aa;

	u8 prell_addr_56a0;

};

/**
 * Bring up PHY and start sending media for a specified pixel clock, pixel
 * repetition and color resolution (to calculate TMDS) - this fields must
 * be configured in the dev->snps_hdmi_ctrl variables
 * @param dev device structure
 * return true if success, false if not success and -1 if PHY configurations
 * are not supported.
 */

extern void *get_innophy46_regmap(void);

extern void *get_innophy56_regmap(void);

void *get_innophy_regmap(u16 addr);

void _inno_phy_fpga_init(struct regmap *regmap46);

void _inno_phy_fpga_power_on(struct regmap *regmap46);

int inno_phy_fpga_configure(dw_hdmi_dev_t *dev);

void inno_phy_fpga_read(u16 addr, u32 *value);

void inno_phy_fpga_write(u16 addr, u8 value);

#endif	/* INNO_PHY_H_ */

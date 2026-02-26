/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef _SUNXI_POWER_MFD_H_
#define _SUNXI_POWER_MFD_H_

/*------------------------------
 * AW Power Core
 *------------------------------*/
#include "sunxi-power-core.h"

/*------------------------------
 * ACPI Support
 *------------------------------*/
#include <linux/acpi.h>

/*------------------------------
 * MFD (Multi-Function Device) Core Support
 *------------------------------*/
#include <linux/mfd/core.h>

/*------------------------------
 * I2C Communication
 *------------------------------*/
#include <linux/i2c.h>

/*------------------------------
 * AW Power MFD Struct
 *------------------------------*/
struct sunxi_power_dev {
	/* main */
	struct device					*dev;
	struct mfd_cell					*cells;
	int								nr_cells;
	/* regmap */
	struct regmap					*regmap;
	const struct regmap_config		*regmap_cfg;
	/* regmap-irq */
	int								irq;
	struct regmap_irq_chip_data		*regmap_irqc;
	const struct regmap_irq_chip	*regmap_irq_chip;
	/* regmap-irq */
	struct regmap_irq_chip_data		*regmap_pdirqc;
	const struct regmap_irq_chip	*regmap_pdirq_chip;
	/* others */
	long							variant;
	void (*dts_parse)				(struct sunxi_power_dev *);
};

#endif /*  _SUNXI_POWER_MFD_H_ */
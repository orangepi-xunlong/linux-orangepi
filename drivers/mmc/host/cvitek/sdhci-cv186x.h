/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * drivers/mmc/host/sdhci-cvi.c - CVITEK SDHCI Platform driver
 *
 * Copyright (c) 2013-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __SDHCI_CV_H
#define __SDHCI_CV_H

#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/mmc/mmc.h>
#include <linux/slab.h>

#define MAX_TUNING_CMD_RETRY_COUNT 50
#define TUNE_MAX_PHCODE	128
#define TAP_WINDOW_THLD 20

#define PWRSW_CTRL_MS_BASE	0x281051e0
#define SD0_MS_MASK		(1 << 6)
#define SD1_MS_MASK		(1 << 4)

#define SDHCI_VENDOR_OFFSET		0x200
#define SDHCI_VENDOR_MSHC_CTRL_R	(SDHCI_VENDOR_OFFSET + 0x0)
#define SDHCI_PHY_TX_RX_DLY		(SDHCI_VENDOR_OFFSET + 0x40)
#define SDHCI_PHY_DS_DLY			(SDHCI_VENDOR_OFFSET + 0x44)
#define SDHCI_PHY_DLY_STS		(SDHCI_VENDOR_OFFSET + 0x48)
#define SDHCI_PHY_CONFIG			(SDHCI_VENDOR_OFFSET + 0x4C)

#define SDHCI_GPIO_CD_DEBOUNCE_TIME	10
#define SDHCI_GPIO_CD_DEBOUNCE_DELAY_TIME	200

#define	SD0_CD_PIN	(0x28104900 + 0x48)
#define	SD1_CD_PIN	(0x28104C00 + 0x5c)
#define PIN_MUX_MASK	(0xf << 4)

#define G8_IO_BASE      0x28104800
#define G11_IO_BASE     0x28104b00
#define PULL_MASK		(0x3 << 2)
#define DRIVE_MASK		(0xf << 8)
#define PULL_DOWN		(0x1 << 3)
#define PULL_UP			(0x1 << 2)
#define DRIVE_VALUE		(0x4 << 8)

struct sdhci_cvi_host {
	struct sdhci_host *host;
	struct platform_device *pdev;
	void __iomem *core_mem; /* mmio address */
	struct clk *clk;    /* main SD/MMC bus clock */
	struct clk *clk100k;
	struct clk *clkaxi;
	struct mmc_host *mmc;
	struct reset_control *reset;

	struct reset_control *clk_rst_axi_emmc_ctrl;
	struct reset_control *clk_rst_emmc_ctrl;
	struct reset_control *clk_rst_100k_emmc_ctrl;

	void __iomem *pwr_ms_reg;
	void __iomem *pinmuxbase;
	void __iomem *clkgenbase;

	u32 reg_ctrl2;
	u32 reg_clk_ctrl;
	u32 reg_host_ctrl;
	u8 final_tap;
	u8 sd_voltage_1_8_v;
	int sd_save_count;
	struct mmc_gpio *cvi_gpio;
	int pwr_gpio;
	struct delayed_work cd_debounce_work;
	spinlock_t cd_debounce_lock;
	int pre_gpio_cd;
	bool is_debounce_work_running;
};
#endif

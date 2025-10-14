/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Set hardware register information of VI for Cix sky SOC
 *
 * Copyright (c) 2024 CIX Semiconductor
 *
 */
#include "types_utils.h"
#include <linux/mutex.h>

#ifndef CIX_HW_REGLIST_H
#define CIX_HW_REGLIST_H

#define CSI_PLL_CLK 650
#define CSI_0_1_DIV_REG 0x310

#define ISP_DIV0_CLK 0
#define ISP_DIV1_CLK 1
#define ISP_DIV2_CLK 2
#define ISP_DIV3_CLK 3

#define ISP_FREQ_1200M 1200
#define ISP_FREQ_600M 600
#define ISP_FREQ_300M 300
#define ISP_FREQ_150M 150

#define MIPI_INFO_IRQS (0x40)
#define MIPI_ERROR_IRQS (0x48)
#define DPHY_ERR_STATUS_IRQ (0x58)

struct cix_vi_hw_dev {
	struct device *dev;

	struct mutex mutex;

	struct clk *phy_psm_clk[2];
	struct clk *phy_apb_clk[2];
	struct clk *csi_p_clk[4];
	struct clk *csi_sys_clk[4];
	struct clk *csi0_pixel_clk[4];
	struct clk *csi1_pixel_clk;
	struct clk *csi2_pixel_clk[4];
	struct clk *csi3_pixel_clk;
	struct clk *csidma_apbclk[4];

	struct reset_control *rst_dphy[2];
	struct reset_control *cmnrst_phy[2];
	struct reset_control *csi_reset[4];
	struct reset_control *csibridge_reset[4];

	unsigned int ahb_dphy_base[2];
	unsigned int ahb_dphy_size[2];
	unsigned int ahb_csi_base[4];
	unsigned int ahb_csi_size[4];
	unsigned int ahb_csidma_base[4];
	unsigned int ahb_csidma_size[4];
	unsigned int ahb_csircsu_base[2];
	unsigned int ahb_csircsu_size[2];

	void __iomem *ahb_dphy_base_addrs[2];
	void __iomem *ahb_csi_base_addrs[4];
	void __iomem *ahb_csidma_base_addrs[4];
	void __iomem *ahb_csircsu_base_addrs[2];
};

u32 cix_ahb_dphy_read_reg(u32 reg_addr);
void cix_ahb_dphy_write_reg(u32 reg_addr, u32 value);

u32 cix_ahb_csi_read_reg(u32 reg_addr);
void cix_ahb_csi_write_reg(u32 reg_addr, u32 value);

u32 cix_ahb_csidma_read_reg(u32 reg_addr);
void cix_ahb_csidma_write_reg(u32 reg_addr, u32 value);

u32 cix_ahb_csircsu_read_reg(u32 reg_addr);
void cix_ahb_csircsu_write_reg(u32 reg_addr, u32 value);

void cix_enable_dphy_clk(u32 reg_addr, u32 value);
void cix_set_csi_clk_rate(u32 reg_addr, u32 value);
void cix_enable_csidma_clk(u32 reg_addr, u32 value);

void *cix_vi_hw_instance(void);
void cix_vi_hw_destroy(void);
#endif

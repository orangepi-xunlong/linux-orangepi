/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN8IW1_H
#define __MACH_SUNXI_CLK_SUN8IW1_H

#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/io.h>
#include <mach/hardware.h>


/* register list */
#define PLL1_CFG            0x0000
#define PLL2_CFG            0x0008
#define PLL3_CFG            0x0010
#define PLL4_CFG            0x0018
#define PLL5_CFG            0x0020
#define PLL6_CFG            0x0028
#define PLL7_CFG            0x0030
#define PLL8_CFG            0x0038
#define MIPI_PLL            0x0040
#define PLL9_CFG            0x0044
#define PLL10_CFG           0x0048
#define CPU_CFG             0x0050
#define AHB1_CFG            0x0054
#define APB2_CFG            0x0058
#define AXI_GATE            0x005C
#define AHB1_GATE0          0x0060
#define AHB1_GATE1          0x0064
#define APB1_GATE           0x0068
#define APB2_GATE           0x006C
#define NAND0_CFG           0x0080
#define NAND1_CFG           0x0084
#define SD0_CFG             0x0088
#define SD1_CFG             0x008C
#define SD2_CFG             0x0090
#define SD3_CFG             0x0094
#define TS_CFG              0x0098
#define SS_CFG              0x009C
#define SPI0_CFG            0x00A0
#define SPI1_CFG            0x00A4
#define SPI2_CFG            0x00A8
#define SPI3_CFG            0x00AC
#define I2S0_CFG            0x00B0
#define I2S1_CFG            0x00B4
#define SPDIF_CFG           0x00C0
#define USB_CFG             0x00CC
#define GMAC_CFG            0x00D0
#define MDFS_CFG            0x00F0
#define DRAM_CFG            0x00F4
#define DRAM_GATE           0x0100
#define BE0_CFG             0x0104
#define BE1_CFG             0x0108
#define FE0_CFG             0x010C
#define FE1_CFG             0x0110
#define MP_CFG              0x0114
#define LCD0_CH0            0x0118
#define LCD1_CH0            0x011C
#define LCD0_CH1            0x012C
#define LCD1_CH1            0x0130
#define CSI0_CFG            0x0134
#define CSI1_CFG            0x0134
#define VE_CFG              0x013C
#define ADDA_CFG            0x0140
#define AVS_CFG             0x0144
#define DMIC_CFG            0x0148
#define HDMI_CFG            0x0150
#define PS_CFG              0x0154
#define MTCACC_CFG          0x0158
#define MBUS0_CFG           0x015C
#define MBUS1_CFG           0x015C
#define MIPI_DSI            0x0168
#define MIPI_CSI            0x016C
#define DRC0_CFG            0x0180
#define DRC1_CFG            0x0184
#define DEU0_CFG            0x0188
#define DEU1_CFG            0x018C
#define GPU_CORE            0x01A0
#define GPU_MEM             0x01A4
#define GPU_HYD             0x01A8
#define PLL_LOCK            0x0200
#define CPU_LOCK            0x0204
#define AHB1_RST0           0x02C0
#define AHB1_RST1           0x02C4
#define AHB1_RST2           0x02C8
#define APB1_RST            0x02D0
#define APB2_RST            0x02D8
#define SUNXI_CLK_MAX_REG   0x02D8


#define CPUS_APB0_GATE      0x0028
#define CPUS_ONEWRITE       0x0050
#define CPUS_CIR            0x0054
#define CPUS_APB0_RST       0x00B0
#define CPUS_CLK_MAX_REG    0x00B0
 
#define F_N8X7_M0X4(nv,mv) FACTOR_ALL(nv,8,7,0,0,0,mv,0,4,0,0,0,0,0,0,0,0,0)	
#define F_N8X5_K4X2(nv,kv) FACTOR_ALL(nv,8,5,kv,4,2,0,0,0,0,0,0,0,0,0,0,0,0)	
#define F_N8X5_K4X2_M0X2(nv,kv,mv) FACTOR_ALL(nv,8,5,kv,4,2,mv,0,2,0,0,0,0,0,0,0,0,0)	
#endif

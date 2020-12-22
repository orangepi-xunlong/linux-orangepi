/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN8IW3_H
#define __MACH_SUNXI_CLK_SUN8IW3_H

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
#define PLL8_CFG            0x0038
#define MIPI_PLL            0x0040
#define PLL9_CFG            0x0044
#define PLL10_CFG           0x0048
#define CPU_CFG             0x0050
#define AHB1_CFG            0x0054
#define APB2_CFG            0x0058
#define AHB1_GATE0          0x0060
#define AHB1_GATE1          0x0064
#define APB1_GATE           0x0068
#define APB2_GATE           0x006c
#define NAND_CFG            0x0080
#define SD0_CFG             0x0088
#define SD1_CFG             0x008c
#define SD2_CFG             0x0090
#define SPI0_CFG            0x00A0
#define SPI1_CFG            0x00A4
#define I2S0_CFG            0x00B0
#define I2S1_CFG            0x00B4
#define USB_CFG             0x00CC
#define DRAM_CFG            0x00F4
#define DRAM_GATE           0x0100
#define BE_CFG              0x0104
#define FE_CFG              0x010C
#define LCD_CH0             0x0118
#define LCD_CH1             0x012C
#define CSI_CFG             0x0134
#define VE_CFG              0x013C
#define ADDA_CFG            0x0140
#define AVS_CFG             0x0144
#define MBUS_CFG            0x015C
#define MIPI_DSI            0x0168
#define DRC_CFG             0x0180
#define GPU_CFG             0x01A0
#define ATS_CFG             0x01B0
#define PLL_LOCK            0x0200
#define CPU_LOCK            0x0204
#define PLL_VIDEOPAT        0x0288
#define PLL_MIPIPAT         0x02A0
#define AHB1_RST0           0x02C0
#define AHB1_RST1           0x02C4
#define AHB1_RST2           0x02C8
#define APB1_RST            0x02D0
#define APB2_RST            0x02D8

#define SUNXI_CLK_MAX_REG   0x02D8
#define LOSC_OUT_GATE       0x01F00060
#define F_N8X7_M0X4(nv,mv) FACTOR_ALL(nv,8,7,0,0,0,mv,0,4,0,0,0,0,0,0,0,0,0)	
#define F_N8X5_K4X2(nv,kv) FACTOR_ALL(nv,8,5,kv,4,2,0,0,0,0,0,0,0,0,0,0,0,0)	
#define F_N8X5_K4X2_M0X2(nv,kv,mv) FACTOR_ALL(nv,8,5,kv,4,2,mv,0,2,0,0,0,0,0,0,0,0,0)	
#define F_N8X5_K4X2_M0X2_P16x2(nv,kv,mv,pv) \
               FACTOR_ALL(nv,8,5, \
                          kv,4,2, \
                          mv,0,2, \
                          pv,16,2, \
                          0,0,0,0,0,0)

struct sun8iw3_factor_config{
		u32		factor;	
    u32   freq;	
};




#endif

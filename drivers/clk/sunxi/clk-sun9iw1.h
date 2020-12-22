/*
 * Copyright (C) 2013 Allwinnertech, kevin.z.m <kevin@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#ifndef __MACH_SUNXI_CLK_SUN9IW1_H
#define __MACH_SUNXI_CLK_SUN9IW1_H

#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/io.h>
#include <mach/hardware.h>

/* register list */
#define PLL1_CFG            0x0000
#define PLL2_CFG            0x0004
#define PLL3_CFG            0x0008
#define PLL4_CFG            0x000c
#define PLL5_CFG            0x0010
#define PLL6_CFG            0x0014
#define PLL7_CFG            0x0018
#define PLL8_CFG            0x001c
#define PLL9_CFG            0x0020
#define PLL10_CFG           0x0024
#define PLL11_CFG           0x0028
#define PLL12_CFG           0x002c

#define CPU_CFG             0x0050
#define AXI0_CFG	    0x0054
#define	AXI1_CFG	    0x0058
#define	GT_CFG		    0x005c
#define AHB0_CFG            0x0060
#define AHB1_CFG            0x0064
#define AHB2_CFG            0x0068
#define APB0_CFG            0x0070
#define APB1_CFG            0x0074
#define CCI400_CFG          0x0078

#define ATS_CFG             0x0080
#define TRACE_CFG           0x0084
#define	LOCK_STAT			0x009c
#define PLL1_BIAS           0x00a0
#define PLL2_BIAS           0x00a4
#define PLL3_BIAS           0x00a8
#define PLL4_BIAS           0x00ac
#define PLL5_BIAS           0x00b0
#define PLL6_BIAS           0x00b4
#define PLL7_BIAS           0x00b8
#define PLL8_BIAS           0x00bc
#define PLL9_BIAS           0x00c0
#define PLL10_BIAS          0x00c4
#define PLL11_BIAS          0x00c8
#define PLL12_BIAS          0x00cc
#define PLL3_PAT            0x108
#define PLL7_PAT            0x118
#define PLL8_PAT            0x11c
#define PLL9_PAT            0x120
#define CLK_OUTA	          0x0180
#define CLK_OUTB          	0x0184

#define NAND0_CFG0          0x0400
#define NAND0_CFG1          0x0404
#define NAND1_CFG0          0x0408
#define NAND1_CFG1          0x040c
#define SD0_CFG             0x0410
#define SD1_CFG             0x0414
#define SD2_CFG             0x0418
#define SD3_CFG             0x041c
#define	TS_CFG							0x0428
#define	SS_CFG							0x042c
#define SPI0_CFG            0x0430
#define SPI1_CFG            0x0434
#define SPI2_CFG            0x0438
#define SPI3_CFG            0x043c
#define I2S0_CFG            0x0440
#define I2S1_CFG            0x0444
#define SPDIF_CFG           0x044c
#define USB_CFG             0x0450

#define DRAM_CFG            0x0484


#define DE_CFG              0x0490
#define EDP_CFG             0x0494

#define MP_CFG             	0x0498
#define LCD0_CFG            0x049c
#define LCD1_CFG            0x04a0
#define MIPI_DSI0		        0x04a8
#define MIPI_DSI1           0x04ac
#define HDMI_CFG						0x04b0
#define HDMI_SLOW						0x04b4
#define MIPI_CSI						0x04bc
#define CSI_ISP							0x04c0
#define	CSI0_MCLK						0x04c4
#define	CSI1_MCLK						0x04c8
#define FD_CFG							0x04cc
#define	VE_CFG							0x04d0
#define	AVS_CFG							0x04d4
#define GPU_CORE						0x04f0
#define	GPU_MEM							0x04f4
#define	GPU_AXI							0x04f8
#define	SATA_CFG						0x0500
#define	AC97_CFG						0x0504
#define MIPI_HSI						0x0508
#define	GP_ADC							0x050c
#define	CIR_TX							0x0510

#define	AHB0_GATE						0x0580
#define	AHB1_GATE						0x0584
#define AHB2_GATE						0x0588

#define	APB0_GATE						0x0590
#define	APB1_GATE						0x0594

#define	AHB0_RST						0x05a0
#define	AHB1_RST						0x05a4
#define AHB2_RST						0x05a8

#define	APB0_RST 						0x05b0
#define	APB1_RST						0x05b4
#define SUNXI_CLK_MAX_REG   0x05b4


#define CPUS_APB0_GATE      0x0028
#define CPUS_ONEWRITE       0x0050
#define CPUS_CIR            0x0054
#define CPUS_I2S0           0x0058
#define CPUS_I2S1           0x005c
#define CPUS_APB0_RST       0x00B0
#define CPUS_VDDSYS_GATE    0x0110
#define CPUS_CLK_MAX_REG    0x0110

#define F_N8X8_P16X1(nv,pv) 		          FACTOR_ALL(nv,8,8, 0,0,0, 0,0,0, pv,16,1, 0,  0, 0, 0,0,0)
#define F_N8X8_D1S16X1(nv,d1v)             FACTOR_ALL(nv,8,8, 0,0,0, 0,0,0, 0, 0, 0, d1v,16,1, 0,0,0)
#define F_N8X8_P0X2_D1S16X1(nv,pv,d1v) 	      FACTOR_ALL(nv,8,8, 0,0,0, 0,0,0, pv,0, 2, d1v,16,1, 0,0,0)
#define F_N8X8_D1S16X1_D2S18X1(nv,d1v,d2v)    FACTOR_ALL(nv,8,8, 0,0,0, 0,0,0, 0, 0, 0, d1v,16,1, d2v,18,1)

#define PLLCPU(n,p,freq)            {F_N8X8_P16X1(n, p),  freq}
#define PLLPERIPH0(n,d1,d2,freq)    {F_N8X8_D1S16X1_D2S18X1(n,d1,d2),freq}
#define PLLVE(n,d1,d2,freq)         {F_N8X8_D1S16X1_D2S18X1(n,d1,d2),freq}
#define PLLDDR(n,d1,d2,freq)        {F_N8X8_D1S16X1_D2S18X1(n,d1,d2),freq}
#define PLLVIDEO0(n,d1,freq)        {F_N8X8_D1S16X1(n, d1),  freq}
#define PLLVIDEO1(n,p,d1,freq)      {F_N8X8_P0X2_D1S16X1(n,p,d1),  freq}
#define PLLGPU(n,d1,d2,freq)        {F_N8X8_D1S16X1_D2S18X1(n,d1,d2),freq}
#define PLLDE(n,d1,d2,freq)         {F_N8X8_D1S16X1_D2S18X1(n,d1,d2),freq}
#define PLLISP(n,d1,d2,freq)        {F_N8X8_D1S16X1_D2S18X1(n,d1,d2),freq}
#define PLLPERIPH1(n,d1,d2,freq)    {F_N8X8_D1S16X1_D2S18X1(n,d1,d2),freq}
#endif

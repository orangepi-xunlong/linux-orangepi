// SPDX-License-Identifier: (GPL-2.0+ or MIT)
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2022 rengaomin@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN8IW18_H_
#define _DT_BINDINGS_CLK_SUN8IW18_H_

#define CLK_PLL_CPU0		0
#define CLK_PLL_DDR		1
#define CLK_PLL_PERI0_2X	2
#define CLK_PLL_PERI0_1X	3
#define CLK_PLL_PERI1_2X	4
#define CLK_PLL_PERI1_1X	5
#define CLK_PLL_AUDIO0_4X	6
#define CLK_PLL_AUDIO0_2X	7
#define CLK_PLL_32K_CTRL	8
#define CLK_CPUX		9
#define CLK_PSI_AHB1_AHB2	10
#define CLK_AHB3		11
#define CLK_APB1		12
#define CLK_APB2		13
#define CLK_CE			14
#define CLK_BUS_CE		15
#define CLK_BUS_DMA		16
#define CLK_BUS_HSTIMER		17
#define CLK_AVS			18
#define CLK_DBGSYS		19
#define CLK_PSI			20
#define CLK_PWM			21
#define CLK_DRAM		22
#define CLK_MBUS_NAND0		23
#define CLK_MBUS_CE		24
#define CLK_MBUS_DMA		25
#define CLK_MBUS_DRAM		26
#define CLK_NAND0_0		27
#define CLK_NAND0_1		28
#define CLK_BUS_NAND0		29
#define CLK_SMHC1		30
#define CLK_BUS_SMHC2		31
#define CLK_BUS_SMHC1		32
#define CLK_BUS_SMHC0		33
#define CLK_BUS_UART3		34
#define CLK_BUS_UART2		35
#define CLK_BUS_UART1		36
#define CLK_BUS_UART0		37
#define CLK_TWI2		38
#define CLK_TWI1		39
#define CLK_TWI0		40
#define CLK_SPI0		41
#define CLK_SPI1		42
#define CLK_BUS_SPI1		43
#define CLK_BUS_SPI0		44
#define CLK_BUS_GPADC		45
#define CLK_BUS_THS		46
#define CLK_I2S_PCM0		47
#define CLK_I2S_PCM1		48
#define CLK_I2S_PCM2		49
#define CLK_BUS_I2S_PCM0	50
#define CLK_BUS_I2S_PCM1	51
#define CLK_BUS_I2S_PCM2	52
#define CLK_SPDIF		53
#define CLK_BUS_SPDIF		54
#define CLK_DMIC		55
#define CLK_BUS_DMIC		56
#define CLK_AUDIO_CODEC_1X	57
#define CLK_AUDIO_CODEC_4X	58
#define CLK_AUDIO_CODEC		59
#define CLK_USB0		60
#define CLK_USBPHY0		61
#define CLK_USBOTG		62
#define CLK_USBEHCI1		63
#define CLK_USBEHCI0		64
#define CLK_USBOHCI1		65
#define CLK_USBOHCI0		66
#define CLK_MAD_CFG		67
#define CLK_MAD_AD		68
#define CLK_MAD			69
#define CLK_LPSD		70
#define CLK_LEDC		71
#define CLK_BUS_LEDC		72
#define CLK_ACODEC_24M		73

#define CLK_MAX_NO		(CLK_ACODEC_24M + 1)

#endif /* _DT_BINDINGS_CLK_SUN8IW18_H_ */

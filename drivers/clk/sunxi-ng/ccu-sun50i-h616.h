/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright 2020 Arm Ltd.
 */

#ifndef _CCU_SUN50I_H616_H_
#define _CCU_SUN50I_H616_H_

#include <dt-bindings/clock/sun50i-h616-ccu.h>
#include <dt-bindings/reset/sun50i-h616-ccu.h>

#define CLK_OSC12M		0
#define CLK_PLL_CPUX		1
#define CLK_PLL_DDR0		2
#define CLK_PLL_DDR1		3

/* PLL_PERIPH0 exported for PRCM */

#define CLK_PLL_PERIPH0_2X	5
#define CLK_PLL_PERIPH0_4X	6
#define CLK_PLL_PERIPH1		7
#define CLK_PLL_PERIPH1_2X	8
#define CLK_PLL_PERIPH1_4X	9
#define CLK_PLL_GPU		10
#define CLK_PLL_VIDEO0		11
#define CLK_PLL_VIDEO0_4X	12
#define CLK_PLL_VIDEO1		13
#define CLK_PLL_VIDEO1_4X	14
#define CLK_PLL_VIDEO2		15
#define CLK_PLL_VIDEO2_4X	16
#define CLK_PLL_VE		17
#define CLK_PLL_DE		18
#define CLK_PLL_AUDIO_HS	19
#define CLK_PLL_AUDIO_1X	20
#define CLK_PLL_AUDIO_2X	21
#define CLK_PLL_AUDIO_4X	22

/* CPUX clock exported for DVFS */

#define CLK_AXI			24
#define CLK_CPUX_APB		25
#define CLK_PSI_AHB1_AHB2	26
#define CLK_AHB3		27

/* APB1 clock exported for PIO */

#define CLK_APB2		29
#define CLK_MBUS		30

/* All module clocks and bus gates are exported except DRAM */

#define CLK_DRAM		51

#define CLK_BUS_DRAM		58

#define CLK_NUMBER		(CLK_BUS_HDCP + 1)

#endif /* _CCU_SUN50I_H616_H_ */

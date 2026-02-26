/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 liujuan1@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN60IW1_DSP_CCU_H_
#define _DT_BINDINGS_CLK_SUN60IW1_DSP_CCU_H_

#define CLK_PLL_AUDIO1		0
#define CLK_PLL_AUDIO1_DIV2	1
#define CLK_STANDBY		2
#define CLK_PLL_AUDIO1_DIV5	3
#define CLK_PLL_AUDIO1_4X	4
#define CLK_RV			5
#define CLK_RV_BUS		6
#define CLK_RV_CFG		7
#define CLK_RISCV_MSG		8
#define CLK_RV_TIMER0		9
#define CLK_RV_TIMER1		10
#define CLK_RV_TIMER2		11
#define CLK_RV_TIMER3		12
#define CLK_BUS_RV_TIMER	13
#define CLK_MCU_TZMA1		14
#define CLK_MCU_TZMA0		15
#define CLK_DMA_MCLK		16
#define CLK_DMA_HCLK		17
#define CLK_I2S			18
#define CLK_MCU_I2S		19
#define CLK_DSP_TIMER0		20
#define CLK_DSP_TIMER1		21
#define CLK_DSP_TIMER2		22
#define CLK_DSP_TIMER3		23
#define CLK_BUS_DSP_TIMER	24
#define CLK_DSP_DSP		25
#define CLK_DSP_CFG		26
#define CLK_DSP_MSG		27
#define CLK_DSP_TZMA		28
#define CLK_DSP_SPINLOCK	29
#define CLK_DMIC		30
#define CLK_DMIC_BUS		31
#define CLK_MSI_BUS		32
#define CLK_TBU_BUS		33
#define CLK_PUBSRAM0		34
#define CLK_PUBSRAM1		35

#define CLK_DSP_NUMBER		(CLK_PUBSRAM1 + 1)

#endif /* _DT_BINDINGS_CLK_SUN60IW1_DSP_CCU_H_ */

/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2022 rengaomin@allwinnertech.com
 */

#ifndef _DT_BINDINGS_CLK_SUN55IW3_MCU_CCU_H_
#define _DT_BINDINGS_CLK_SUN55IW3_MCU_CCU_H_

#define CLK_PLL_MCU_AUDIO1	0
#define CLK_PLL_MCU_AUDIO1_DIV2	1
#define CLK_PLL_MCU_AUDIO1_DIV5	2
#define CLK_PLL_MCU_AUDIO_OUT	3
#define CLK_DSP_DSP		4
#define CLK_MCU_I2S0		5
#define CLK_MCU_I2S1		6
#define CLK_MCU_I2S2		7
#define CLK_MCU_I2S3		8
#define CLK_MCU_I2S3_ASRC	9
#define CLK_BUS_MCU_I2S0	10
#define CLK_BUS_MCU_I2S1	11
#define CLK_BUS_MCU_I2S2	12
#define CLK_BUS_MCU_I2S3	13
#define CLK_MCU_OWA_TX		14
#define CLK_MCU_OWA_RX		15
#define CLK_BUS_MCU_OWA		16
#define CLK_MCU_DMIC		17
#define CLK_BUS_MCU_DMIC	18
#define CLK_MCU_AUDIO_CODEC_DAC	19
#define CLK_MCU_AUDIO_CODEC_ADC	20
#define CLK_BUS_MCU_AUDIO_CODEC	21
#define CLK_BUS_DSP_MSG		22
#define CLK_BUS_DSP_CFG		23
#define CLK_BUS_MCU_NPU_ACLK	24
#define CLK_BUS_MCU_NPU_HCLK	25
#define CLK_MCU_TIMER0		27
#define CLK_MCU_TIMER1		28
#define CLK_MCU_TIMER2		29
#define CLK_MCU_TIMER3		30
#define CLK_MCU_TIMER4		31
#define CLK_MCU_TIMER5		32
#define CLK_BUS_MCU_TIMER	33
#define CLK_BUS_MCU_DMA		34
#define CLK_BUS_MCU_TZMA0	35
#define CLK_BUS_MCU_TZMA1	36
#define CLK_BUS_PUBSRAM		37
#define CLK_BUS_MCU_MBUS	38
#define CLK_BUS_MCU_DMA_MBUS	39
#define CLK_BUS_RV		40
#define CLK_BUS_RV_CFG		41
#define CLK_BUS_MCU_RISCV_MSG	42
#define CLK_MCU_PWM		43
#define CLK_BUS_MCU_PWM		44
#define CLK_BUS_MCU_AHB_AUTO	45

#define CLK_MCU_NUMBER		(CLK_BUS_MCU_AHB_AUTO + 1)

#endif /* _DT_BINDINGS_CLK_SUN55IW3_MCU_CCU_H_ */

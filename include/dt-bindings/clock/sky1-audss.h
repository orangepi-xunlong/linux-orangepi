/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * This header provides constants for Cixtech Sky1 audio subsystem clock controller.
 */

#ifndef _DT_BINDINGS_CLOCK_SKY1_AUDSS_H_
#define _DT_BINDINGS_CLOCK_SKY1_AUDSS_H_

#define SKY1_AUDSS_AUDIO_CLK0_RATE	294912000
#define SKY1_AUDSS_AUDIO_CLK1_RATE	344064000
#define SKY1_AUDSS_AUDIO_CLK2_RATE	270950400
#define SKY1_AUDSS_AUDIO_CLK3_RATE	316108800
#define SKY1_AUDSS_AUDIO_CLK4_RATE	800000000
#define SKY1_AUDSS_AUDIO_CLK5_RATE	48000000

#define CLK_AUD_CLK4_DIV2	0
#define CLK_AUD_CLK4_DIV4	1
#define CLK_AUD_CLK5_DIV2	2

#define CLK_DSP_CLK		3
#define CLK_DSP_BCLK		4
#define CLK_DSP_PBCLK		5

#define CLK_SRAM_AXI		6

#define CLK_HDA_SYS		7
#define CLK_HDA_HDA		8

#define CLK_DMAC_AXI		9

#define CLK_WDG_APB		10
#define CLK_WDG_WDG		11

#define CLK_TIMER_APB		12
#define CLK_TIMER_TIMER		13

#define CLK_MB_0_APB		14	/* MB0: ap->dsp */
#define CLK_MB_1_APB		15	/* MB1: dsp->ap */

#define CLK_I2S0_APB		16
#define CLK_I2S1_APB		17
#define CLK_I2S2_APB		18
#define CLK_I2S3_APB		19
#define CLK_I2S4_APB		20
#define CLK_I2S5_APB		21
#define CLK_I2S6_APB		22
#define CLK_I2S7_APB		23
#define CLK_I2S8_APB		24
#define CLK_I2S9_APB		25
#define CLK_I2S0		26
#define CLK_I2S1		27
#define CLK_I2S2		28
#define CLK_I2S3		29
#define CLK_I2S4		30
#define CLK_I2S5		31
#define CLK_I2S6		32
#define CLK_I2S7		33
#define CLK_I2S8		34
#define CLK_I2S9		35

#define CLK_MCLK0		36
#define CLK_MCLK1		37
#define CLK_MCLK2		38
#define CLK_MCLK3		39
#define CLK_MCLK4		40

#define AUDSS_MAX_CLKS		41

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2023, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_DAP_H
#define __SND_SUNXI_DAP_H

/* use for audiocodec dap default only */

#define SUNXI_DAC_DAP_CTL	0xF0
#define SUNXI_ADC_DAP_CTL	0xF8

#define SUNXI_DAC_DRC_HHPFC	0x100
#define SUNXI_DAC_DRC_LHPFC	0x104
#define SUNXI_DAC_DRC_CTRL	0x108
#define SUNXI_DAC_DRC_LPFHAT	0x10C
#define SUNXI_DAC_DRC_LPFLAT	0x110
#define SUNXI_DAC_DRC_RPFHAT	0x114
#define SUNXI_DAC_DRC_RPFLAT	0x118
#define SUNXI_DAC_DRC_LPFHRT	0x11C
#define SUNXI_DAC_DRC_LPFLRT	0x120
#define SUNXI_DAC_DRC_RPFHRT	0x124
#define SUNXI_DAC_DRC_RPFLRT	0x128
#define SUNXI_DAC_DRC_LRMSHAT	0x12C
#define SUNXI_DAC_DRC_LRMSLAT	0x130
#define SUNXI_DAC_DRC_RRMSHAT	0x134
#define SUNXI_DAC_DRC_RRMSLAT	0x138
#define SUNXI_DAC_DRC_HCT	0x13C
#define SUNXI_DAC_DRC_LCT	0x140
#define SUNXI_DAC_DRC_HKC	0x144
#define SUNXI_DAC_DRC_LKC	0x148
#define SUNXI_DAC_DRC_HOPC	0x14C
#define SUNXI_DAC_DRC_LOPC	0x150
#define SUNXI_DAC_DRC_HLT	0x154
#define SUNXI_DAC_DRC_LLT	0x158
#define SUNXI_DAC_DRC_HKI	0x15C
#define SUNXI_DAC_DRC_LKI	0x160
#define SUNXI_DAC_DRC_HOPL	0x164
#define SUNXI_DAC_DRC_LOPL	0x168
#define SUNXI_DAC_DRC_HET	0x16C
#define SUNXI_DAC_DRC_LET	0x170
#define SUNXI_DAC_DRC_HKE	0x174
#define SUNXI_DAC_DRC_LKE	0x178
#define SUNXI_DAC_DRC_HOPE	0x17C
#define SUNXI_DAC_DRC_LOPE	0x180
#define SUNXI_DAC_DRC_HKN	0x184
#define SUNXI_DAC_DRC_LKN	0x188
#define SUNXI_DAC_DRC_SFHAT	0x18C
#define SUNXI_DAC_DRC_SFLAT	0x190
#define SUNXI_DAC_DRC_SFHRT	0x194
#define SUNXI_DAC_DRC_SFLRT	0x198
#define SUNXI_DAC_DRC_MXGHS	0x19C
#define SUNXI_DAC_DRC_MXGLS	0x1A0
#define SUNXI_DAC_DRC_MNGHS	0x1A4
#define SUNXI_DAC_DRC_MNGLS	0x1A8
#define SUNXI_DAC_DRC_EPSHC	0x1AC
#define SUNXI_DAC_DRC_EPSLC	0x1B0
#define SUNXI_DAC_DRC_OPT	0x1B4
#define SUNXI_DAC_DRC_HPFHGAIN	0x1B8
#define SUNXI_DAC_DRC_HPFLGAIN	0x1BC

#define SUNXI_ADC_DRC_HHPFC	0x200
#define SUNXI_ADC_DRC_LHPFC	0x204
#define SUNXI_ADC_DRC_CTRL	0x208
#define SUNXI_ADC_DRC_LPFHAT	0x20C
#define SUNXI_ADC_DRC_LPFLAT	0x210
#define SUNXI_ADC_DRC_RPFHAT	0x214
#define SUNXI_ADC_DRC_RPFLAT	0x218
#define SUNXI_ADC_DRC_LPFHRT	0x21C
#define SUNXI_ADC_DRC_LPFLRT	0x220
#define SUNXI_ADC_DRC_RPFHRT	0x224
#define SUNXI_ADC_DRC_RPFLRT	0x228
#define SUNXI_ADC_DRC_LRMSHAT	0x22C
#define SUNXI_ADC_DRC_LRMSLAT	0x230
#define SUNXI_ADC_DRC_RRMSHAT	0x234
#define SUNXI_ADC_DRC_RRMSLAT	0x238
#define SUNXI_ADC_DRC_HCT	0x23C
#define SUNXI_ADC_DRC_LCT	0x240
#define SUNXI_ADC_DRC_HKC	0x244
#define SUNXI_ADC_DRC_LKC	0x248
#define SUNXI_ADC_DRC_HOPC	0x24C
#define SUNXI_ADC_DRC_LOPC	0x250
#define SUNXI_ADC_DRC_HLT	0x254
#define SUNXI_ADC_DRC_LLT	0x258
#define SUNXI_ADC_DRC_HKI	0x25C
#define SUNXI_ADC_DRC_LKI	0x260
#define SUNXI_ADC_DRC_HOPL	0x264
#define SUNXI_ADC_DRC_LOPL	0x268
#define SUNXI_ADC_DRC_HET	0x26C
#define SUNXI_ADC_DRC_LET	0x270
#define SUNXI_ADC_DRC_HKE	0x274
#define SUNXI_ADC_DRC_LKE	0x278
#define SUNXI_ADC_DRC_HOPE	0x27C
#define SUNXI_ADC_DRC_LOPE	0x280
#define SUNXI_ADC_DRC_HKN	0x284
#define SUNXI_ADC_DRC_LKN	0x288
#define SUNXI_ADC_DRC_SFHAT	0x28C
#define SUNXI_ADC_DRC_SFLAT	0x290
#define SUNXI_ADC_DRC_SFHRT	0x294
#define SUNXI_ADC_DRC_SFLRT	0x298
#define SUNXI_ADC_DRC_MXGHS	0x29C
#define SUNXI_ADC_DRC_MXGLS	0x2A0
#define SUNXI_ADC_DRC_MNGHS	0x2A4
#define SUNXI_ADC_DRC_MNGLS	0x2A8
#define SUNXI_ADC_DRC_EPSHC	0x2AC
#define SUNXI_ADC_DRC_EPSLC	0x2B0
#define SUNXI_ADC_DRC_OPT	0x2B4
#define SUNXI_ADC_DRC_HPFHGAIN	0x2B8
#define SUNXI_ADC_DRC_HPFLGAIN	0x2BC

/* SUNXI_DAC_DAP_CTL:0xf0 */
#define	DAC_DAP_EN		31
#define	DDAP_DRC_EN		29
#define	DDAP_HPF_EN		28

/* SUNXI_ADC_DAP_CTL:0xf8 */
#define	ADC_DAP_EN		31
#define	ADAP_DRC_EN		29
#define	ADAP_HPF_EN		28

/* SUNXI_DAC_DRC_CTRL:0x108*/
#define DAC_DRC_DELAY_OUT_STATE		15
#define DAC_DRC_SIGNAL_DELAY		8
#define DAC_DRC_DELAY_BUF_EN		7
#define DAC_DRC_GAIN_MAX_EN		6
#define DAC_DRC_GAIN_MIN_EN		5
#define DAC_DRC_NOISE_DET_EN		4
#define DAC_DRC_SIGNAL_SEL		3
#define DAC_DRC_DELAY_EN		2
#define DAC_DRC_LT_EN			1
#define DAC_DRC_ET_EN			0

/* SUNXI_ADC_DRC_CTRL:0x208 */
#define ADC_DRC_DELAY_OUT_STATE		15
#define ADC_DRC_SIGNAL_DELAY		8
#define ADC_DRC_DELAY_BUF_EN		7
#define ADC_DRC_GAIN_MAX_EN		6
#define ADC_DRC_GAIN_MIN_EN		5
#define ADC_DRC_NOISE_DET_EN		4
#define ADC_DRC_SIGNAL_SEL		3
#define ADC_DRC_DELAY_EN		2
#define ADC_DRC_LT_EN			1
#define ADC_DRC_ET_EN			0

/* Add this to the corresponding codec Makefile compilation.
 * Call this function in the codec_init function corresponding to the codec.
 */
void snd_sunxi_dap_dacdrc(struct regmap *regmap);
void snd_sunxi_dap_dachpf(struct regmap *regmap);
void snd_sunxi_dap_adcdrc(struct regmap *regmap);
void snd_sunxi_dap_adchpf(struct regmap *regmap);

#endif /* __SND_SUNXI_DAP_H */

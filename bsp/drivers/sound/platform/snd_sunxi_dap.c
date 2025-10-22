// SPDX-License-Identifier: GPL-2.0-or-later
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

#define SUNXI_MODNAME		"sound-dap"
#include "snd_sunxi_log.h"
#include <linux/regmap.h>

#include "snd_sunxi_dap.h"

void snd_sunxi_dap_dacdrc(struct regmap *regmap)
{
	SND_LOG_DEBUG("\n");

	if (!regmap) {
		SND_LOG_ERR("regmap is invailed\n");
		return;
	}

	regmap_write(regmap, SUNXI_DAC_DRC_CTRL, 0x00DB);

	/* left peak filter attack time */
	regmap_write(regmap, SUNXI_DAC_DRC_LPFHAT, 0x0000);
	regmap_write(regmap, SUNXI_DAC_DRC_LPFLAT, 0x9DE7);

	/* right peak filter attack time */
	regmap_write(regmap, SUNXI_DAC_DRC_RPFHAT, 0x000B);
	regmap_write(regmap, SUNXI_DAC_DRC_RPFLAT, 0x77F0);

	/* Left peak filter release time */
	regmap_write(regmap, SUNXI_DAC_DRC_LPFHRT, 0x00FF);
	regmap_write(regmap, SUNXI_DAC_DRC_LPFLRT, 0x4F8C);

	/* Right peak filter release time */
	regmap_write(regmap, SUNXI_DAC_DRC_RPFHRT, 0x00FE);
	regmap_write(regmap, SUNXI_DAC_DRC_RPFLRT, 0xD450);

	/* Left RMS filter attack time */
	regmap_write(regmap, SUNXI_DAC_DRC_LRMSHAT, 0x0001);
	regmap_write(regmap, SUNXI_DAC_DRC_LRMSLAT, 0x2BB0);

	/* Right RMS filter attack time */
	regmap_write(regmap, SUNXI_DAC_DRC_RRMSHAT, 0x0001);
	regmap_write(regmap, SUNXI_DAC_DRC_RRMSLAT, 0x2BB0);

	/* CT */
	regmap_write(regmap, SUNXI_DAC_DRC_HCT, 0x0352);
	regmap_write(regmap, SUNXI_DAC_DRC_LCT, 0x69E0);

	/* Kc */
	regmap_write(regmap, SUNXI_DAC_DRC_HKC, 0x00A2);
	regmap_write(regmap, SUNXI_DAC_DRC_LKC, 0x2222);

	/* OPC */
	regmap_write(regmap, SUNXI_DAC_DRC_HOPC, 0xFE56);
	regmap_write(regmap, SUNXI_DAC_DRC_LOPC, 0xCB10);

	/* LT */
	regmap_write(regmap, SUNXI_DAC_DRC_HLT, 0x01A9);
	regmap_write(regmap, SUNXI_DAC_DRC_LLT, 0x34F0);

	/* Ki */
	regmap_write(regmap, SUNXI_DAC_DRC_HKI, 0x0033);
	regmap_write(regmap, SUNXI_DAC_DRC_LKI, 0x3334);

	/* OPL */
	regmap_write(regmap, SUNXI_DAC_DRC_HOPL, 0xFF64);
	regmap_write(regmap, SUNXI_DAC_DRC_LOPL, 0x1741);

	/* ET */
	regmap_write(regmap, SUNXI_DAC_DRC_HET, 0x04FB);
	regmap_write(regmap, SUNXI_DAC_DRC_LET, 0x9ED1);

	/* Ke */
	regmap_write(regmap, SUNXI_DAC_DRC_HKE, 0x02AA);
	regmap_write(regmap, SUNXI_DAC_DRC_LKE, 0xAAAC);

	/* OPE */
	regmap_write(regmap, SUNXI_DAC_DRC_HOPE, 0xFCAD);
	regmap_write(regmap, SUNXI_DAC_DRC_LOPE, 0x9620);

	/* Kn */
	regmap_write(regmap, SUNXI_DAC_DRC_HKN, 0x0100);
	regmap_write(regmap, SUNXI_DAC_DRC_LKN, 0x0000);

	/* smooth filter attack time */
	regmap_write(regmap, SUNXI_DAC_DRC_SFHAT, 0x0001);
	regmap_write(regmap, SUNXI_DAC_DRC_SFLAT, 0x7665);

	/* gain smooth filter release time */
	regmap_write(regmap, SUNXI_DAC_DRC_SFHRT, 0x0000);
	regmap_write(regmap, SUNXI_DAC_DRC_SFLRT, 0x0F04);

	/* MXG */
	regmap_write(regmap, SUNXI_DAC_DRC_MXGHS, 0x0352);
	regmap_write(regmap, SUNXI_DAC_DRC_MXGLS, 0x69E0);

	/* MNG */
	regmap_write(regmap, SUNXI_DAC_DRC_MNGHS, 0xF95B);
	regmap_write(regmap, SUNXI_DAC_DRC_MNGLS, 0x2C3F);

	/* EPS */
	regmap_write(regmap, SUNXI_DAC_DRC_EPSHC, 0x0002);
	regmap_write(regmap, SUNXI_DAC_DRC_EPSLC, 0x5600);

	regmap_write(regmap, SUNXI_DAC_DRC_OPT, 0x0000);
	regmap_write(regmap, SUNXI_DAC_DRC_HPFHGAIN, 0x0100);
	regmap_write(regmap, SUNXI_DAC_DRC_HPFLGAIN, 0x0000);
}

void snd_sunxi_dap_dachpf(struct regmap *regmap)
{
	SND_LOG_DEBUG("\n");

	if (!regmap) {
		SND_LOG_ERR("regmap is invailed\n");
		return;
	}

	regmap_write(regmap, SUNXI_DAC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	regmap_write(regmap, SUNXI_DAC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

void snd_sunxi_dap_adcdrc(struct regmap *regmap)
{
	SND_LOG_DEBUG("\n");

	if (!regmap) {
		SND_LOG_ERR("regmap is invailed\n");
		return;
	}

	regmap_write(regmap, SUNXI_ADC_DRC_CTRL, 0x00DB);

	/* left peak filter attack time */
	regmap_write(regmap, SUNXI_ADC_DRC_LPFHAT, 0x0001);
	regmap_write(regmap, SUNXI_ADC_DRC_LPFLAT, 0x7665);

	/* right peak filter attack time */
	regmap_write(regmap, SUNXI_ADC_DRC_RPFHAT, 0x000B);
	regmap_write(regmap, SUNXI_ADC_DRC_RPFLAT, 0x77F0);

	/* Left peak filter release time */
	regmap_write(regmap, SUNXI_ADC_DRC_LPFHRT, 0x00FE);
	regmap_write(regmap, SUNXI_ADC_DRC_LPFLRT, 0xD450);

	/* Right peak filter release time */
	regmap_write(regmap, SUNXI_ADC_DRC_RPFHRT, 0x00FE);
	regmap_write(regmap, SUNXI_ADC_DRC_RPFLRT, 0xD450);

	/* Left RMS filter attack time */
	regmap_write(regmap, SUNXI_ADC_DRC_LRMSHAT, 0x0001);
	regmap_write(regmap, SUNXI_ADC_DRC_LRMSLAT, 0x2BB0);

	/* Right RMS filter attack time */
	regmap_write(regmap, SUNXI_ADC_DRC_RRMSHAT, 0x0001);
	regmap_write(regmap, SUNXI_ADC_DRC_RRMSLAT, 0x2BB0);

	/* CT */
	regmap_write(regmap, SUNXI_ADC_DRC_HCT, 0x0352);
	regmap_write(regmap, SUNXI_ADC_DRC_LCT, 0x69E0);

	/* Kc */
	regmap_write(regmap, SUNXI_ADC_DRC_HKC, 0x0099);
	regmap_write(regmap, SUNXI_ADC_DRC_LKC, 0x999A);

	/* OPC */
	regmap_write(regmap, SUNXI_ADC_DRC_HOPC, 0xFE56);
	regmap_write(regmap, SUNXI_ADC_DRC_LOPC, 0xCB10);

	/* LT */
	regmap_write(regmap, SUNXI_ADC_DRC_HLT, 0x01A9);
	regmap_write(regmap, SUNXI_ADC_DRC_LLT, 0x34F0);

	/* Ki */
	regmap_write(regmap, SUNXI_ADC_DRC_HKI, 0x0033);
	regmap_write(regmap, SUNXI_ADC_DRC_LKI, 0x3333);

	/* OPL */
	regmap_write(regmap, SUNXI_ADC_DRC_HOPL, 0xFF55);
	regmap_write(regmap, SUNXI_ADC_DRC_LOPL, 0xEAD3);

	/* ET */
	regmap_write(regmap, SUNXI_ADC_DRC_HET, 0x04FB);
	regmap_write(regmap, SUNXI_ADC_DRC_LET, 0x9ED1);

	/* Ke */
	regmap_write(regmap, SUNXI_ADC_DRC_HKE, 0x2800);
	regmap_write(regmap, SUNXI_ADC_DRC_LKE, 0x0000);

	/* OPE */
	regmap_write(regmap, SUNXI_ADC_DRC_HOPE, 0xFCAD);
	regmap_write(regmap, SUNXI_ADC_DRC_LOPE, 0x9620);

	/* Kn */
	regmap_write(regmap, SUNXI_ADC_DRC_HKN, 0x0100);
	regmap_write(regmap, SUNXI_ADC_DRC_LKN, 0x0000);

	/* smooth filter attack time */
	regmap_write(regmap, SUNXI_ADC_DRC_SFHAT, 0x0001);
	regmap_write(regmap, SUNXI_ADC_DRC_SFLAT, 0x7665);

	/* gain smooth filter release time */
	regmap_write(regmap, SUNXI_ADC_DRC_SFHRT, 0x0000);
	regmap_write(regmap, SUNXI_ADC_DRC_SFLRT, 0x0F04);

	/* MXG */
	regmap_write(regmap, SUNXI_ADC_DRC_MXGHS, 0x04FB);
	regmap_write(regmap, SUNXI_ADC_DRC_MXGLS, 0x9ED1);

	/* MNG */
	regmap_write(regmap, SUNXI_ADC_DRC_MNGHS, 0xFB04);
	regmap_write(regmap, SUNXI_ADC_DRC_MNGLS, 0x612F);

	/* EPS */
	regmap_write(regmap, SUNXI_ADC_DRC_EPSHC, 0x0002);
	regmap_write(regmap, SUNXI_ADC_DRC_EPSLC, 0x5600);

	regmap_write(regmap, SUNXI_ADC_DRC_OPT, 0x0000);
	regmap_write(regmap, SUNXI_ADC_DRC_HPFHGAIN, 0x0100);
	regmap_write(regmap, SUNXI_ADC_DRC_HPFLGAIN, 0x0000);
}

void snd_sunxi_dap_adchpf(struct regmap *regmap)
{
	SND_LOG_DEBUG("\n");

	if (!regmap) {
		SND_LOG_ERR("regmap is invailed\n");
		return;
	}

	regmap_write(regmap, SUNXI_ADC_DRC_HHPFC, (0xFFFAC1 >> 16) & 0xFFFF);
	regmap_write(regmap, SUNXI_ADC_DRC_LHPFC, 0xFFFAC1 & 0xFFFF);
}

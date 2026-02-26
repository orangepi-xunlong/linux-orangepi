/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's ALSA SoC Audio driver
 *
 * Copyright (c) 2022, Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_AHUB_H
#define __SND_SUNXI_AHUB_H

#include "snd_sunxi_ahub_dam.h"

struct sunxi_ahub_pinctl {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;

	bool pinctrl_used;
};

struct sunxi_ahub_dts {
	unsigned int dai_type;
	unsigned int apb_num;
	unsigned int tdm_num;
	unsigned int tx_pin;
	unsigned int rx_pin;

	/* value must be (2^n)Kbyte */
	size_t playback_cma;
	size_t playback_fifo_size;
	size_t capture_cma;
	size_t capture_fifo_size;
};

struct sunxi_ahub_rglt {
	struct regulator *ahub_rglt;
	const char *rglt_name;
};

struct sunxi_ahub {
	struct device *dev;

	struct sunxi_ahub_mem mem;
	struct sunxi_ahub_clk clk;
	struct sunxi_ahub_pinctl pin;
	struct sunxi_ahub_dts dts;
	struct sunxi_ahub_rglt rglt;

	struct sunxi_dma_params playback_dma_param;
	struct sunxi_dma_params capture_dma_param;

	/* for Hardware param setting */
	unsigned int fmt;
	unsigned int pllclk_freq;
	unsigned int moduleclk_freq;
	unsigned int mclk_freq;
	unsigned int lrck_freq;
	unsigned int bclk_freq;
	unsigned int data_late;
	bool tx_lsb_first;
	bool rx_lsb_first;
};

#endif /* __SND_SUNXI_AHUB_H */

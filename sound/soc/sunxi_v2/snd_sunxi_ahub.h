/* sound\soc\sunxi\snd_sunxi_ahub.h
 * (C) Copyright 2021-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Dby <dby@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __SND_SUNXI_AHUB_H
#define __SND_SUNXI_AHUB_H

#include "snd_sunxi_ahub_dam.h"

struct sunxi_ahub_pinctl_info {
	struct pinctrl *pinctrl;
	struct pinctrl_state *pinstate;
	struct pinctrl_state *pinstate_sleep;

	bool pinctrl_used;
};

struct sunxi_ahub_dts_info {
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

struct sunxi_ahub_regulator_info {
	struct regulator *regulator;
	const char *regulator_name;
};

struct sunxi_ahub_info {
	struct device *dev;

	struct sunxi_ahub_mem_info mem_info;
	struct sunxi_ahub_clk_info clk_info;
	struct sunxi_ahub_pinctl_info pin_info;
	struct sunxi_ahub_dts_info dts_info;
	struct sunxi_ahub_regulator_info regulator_info;

	//struct sunxi_dma_params playback_dma_param;
	//struct sunxi_dma_params capture_dma_param;
        struct snd_dmaengine_dai_dma_data playback_dma_param;
        struct snd_dmaengine_dai_dma_data capture_dma_param;

	/* for Hardware param setting */
	unsigned int fmt;
	unsigned int pllclk_freq;
	unsigned int moduleclk_freq;
	unsigned int mclk_freq;
	unsigned int lrck_freq;
	unsigned int bclk_freq;
};

#endif /* __SND_SUNXI_AHUB_H */

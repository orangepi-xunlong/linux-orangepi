/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-3-Clause) */
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * Author: Joakim Zhang <joakim.zhang@cixtech.com>
 */

#ifndef __INCLUDE_SOUND_SOF_DAI_CIX_H__
#define __INCLUDE_SOUND_SOF_DAI_CIX_H__

#include <sound/sof/header.h>

/* I2S_SC Configuration Request - SOF_IPC_DAI_I2S_SC_CONFIG */
struct sof_ipc_dai_i2s_sc_params {
	struct sof_ipc_hdr hdr;

	uint32_t rate;
	uint16_t channels;
	uint16_t format;

	/* MCLK */
	uint16_t mclk_id;
	uint16_t mclk_fs;

	uint16_t playback_dma_ch;
	uint16_t capture_dma_ch;

	/* TDM */
	uint32_t tdm_slots;
	uint32_t tdm_rx_slot_mask;
	uint32_t tdm_tx_slot_mask;
	uint32_t tdm_slot_width;

} __packed;

/* I2S_MC Configuration Request - SOF_IPC_DAI_I2S_MC_CONFIG */
struct sof_ipc_dai_i2s_mc_params {
	struct sof_ipc_hdr hdr;

	uint32_t rate;
	uint16_t channels;
	uint16_t format;

	/* MCLK */
	uint16_t mclk_id;
	uint16_t mclk_fs;

	uint16_t playback_dma_ch;
	uint16_t capture_dma_ch;

	uint16_t pin_rx_mask;
	uint16_t pin_tx_mask;

} __packed;

#endif

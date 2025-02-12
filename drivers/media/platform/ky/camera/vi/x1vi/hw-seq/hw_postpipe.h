// SPDX-License-Identifier: GPL-2.0
/*
 * hw_postpipe.h - postpipe hw sequence
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _HW_POSTPIPE_H_
#define _HW_POSTPIPE_H_
#include "../../cam_block.h"
#define KY_POSTPIPE_OFFSET	(0x00010000)

enum {
	NV12 = 2,
	NV21,
	P210,
	Y210,
	P010,
	RGB888,
	RGB565,
};

enum {
	MUX_SEL_FORMATTER2 = 0,
	MUX_SEL_FORMATTER3,
	MUX_SEL_FORMATTER4,
	MUX_SEL_FORMATTER5,
	MUX_SEL_DWT0_LAYER1,
	MUX_SEL_DWT0_LAYER2,
	MUX_SEL_DWT0_LAYER3,
	MUX_SEL_DWT0_LAYER4,
};

enum {
	DWT_SRC_SEL_FORMATTER0 = 0,
	DWT_SRC_SEL_FORMATTER1,
	DWT_SRC_SEL_FORMATTER2,
	DWT_SRC_SEL_FORMATTER3,
	DWT_SRC_SEL_FORMATTER4,
	DWT_SRC_SEL_FORMATTER5,
};

enum {
	SCL_SRC_SEL_PIPE0 = 0,
	SCL_SRC_SEL_PIPE1,
};

void hw_postpipe_set_formatter_format(struct spm_camera_block *sc_block, unsigned int idx, int format);
void hw_postpipe_dma_mux_enable(struct spm_camera_block *sc_block, int mux_select);
void hw_postpipe_enable_dwt(struct spm_camera_block *sc_block, unsigned int idx, int src, int enable);
//void hw_postpipe_set_scaler(struct spm_camera_block *sc_block, unsigned int idx,
//							unsigned int in_width, unsigned int in_height,
//							unsigned int out_width, unsigned int out_height);
void hw_postpipe_set_scaler_source(struct spm_camera_block *sc_block, unsigned int idx, int source);
#endif

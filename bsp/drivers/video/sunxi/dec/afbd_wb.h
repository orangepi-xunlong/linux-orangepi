/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * afbd_wb.h
 *
 * Copyright (c) 2007-2023 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _AFBD_WB_H
#define _AFBD_WB_H

#ifdef __cplusplus
extern "C" {
#endif
#include "dma_buf.h"
#include <video/decoder_display.h>

enum wb_src_pixel_fmt {
	AFBD_YUV420 = 0x0,
	AFBD_YUV420_10B = 0x1,
	AFBD_YUV422 = 0x2,
	AFBD_YUV422_10B = 0x3,
	AFBD_YUV444 = 0x4,
	AFBD_YUV444_10B = 0x5,
	AFBD_P010_H10 = 0x6,
	AFBD_P010_L10 = 0x7,
	AFBD_RGB888 = 0x17,
	AFBD_RGB565 = 0x18,
	AFBD_RGBA8888 = 0x19,
	AFBD_RGBA5551 = 0x1a,
	AFBD_RGBA4444 = 0x1b,
	AFBD_R8 = 0x1c,
	AFBD_RG88 = 0x1d,
	AFBD_R10G10B10A2 = 0x1e,
	AFBD_R11G11B10 = 0x1f,
};

enum afbd_wb_state {
	AFBD_WB_IDLE,
	AFBD_WB_COMMIT,
	AFBD_WB_SYNC,
};

struct afbd_wb_dev_t {
	struct afbd_wb_info wb_info;
	struct dec_dmabuf_t *buf;
	struct afbd_reg_t *p_afbd_reg;
	struct device *p_device;
	struct mutex state_lock;
	enum afbd_wb_state wb_state;
	bool offline_mode;
	enum afbd_wb_state (*get_wb_state)(struct afbd_wb_dev_t *pdev);
	int (*afbd_set_wb_state)(struct afbd_wb_dev_t *pdev, enum afbd_wb_state state);
	int (*afbd_wb_set_outbuf_addr)(struct afbd_wb_dev_t *pdev, struct afbd_wb_info *wb_info,
				       dma_addr_t out_addr);
	int (*afbd_wb_en)(struct afbd_wb_dev_t *pdev, int ch_id, int en);
	int (*afbd_is_compressed)(struct afbd_wb_dev_t *pdev, int ch_id);
	int (*afbd_is_ch_en)(struct afbd_wb_dev_t *pdev, int ch_id);
	int (*afbd_get_pixel_fmt)(struct afbd_wb_dev_t *pdev, int ch_id);
	int (*afbd_get_pic_size)(struct afbd_wb_dev_t *pdev, struct afbd_wb_info *wb_info,
				 unsigned int *w, unsigned int *h);
	int (*afbd_wait_fb_finish)(struct afbd_wb_dev_t *pdev, int ch_id);
	int (*afbd_wb_commit)(struct afbd_wb_dev_t *pdev, struct afbd_wb_info *wb_info);
};

struct afbd_wb_dev_t *get_afbd_wb_inst(void);
struct afbd_wb_dev_t *afbd_wb_init(struct afbd_reg_t *p_reg,
				   struct device *p_device, bool offline_en);
void afbd_wb_deinit(void);


#ifdef __cplusplus
}
#endif

#endif /*End of file*/

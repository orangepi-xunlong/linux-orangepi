/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec_video_info.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: yajianz <yajianz@allwinnertech.com>
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

#ifndef _DEC_VIDEO_INFO_H_
#define _DEC_VIDEO_INFO_H_

#include "linux/types.h"
#include <linux/dma-buf.h>
#include <linux/dma-mapping.h>

#include "dec.h"

struct memory_block {
	void *va;
	dma_addr_t physical;
	int token;
	struct frame_dma_buf_t *parent;
	struct list_head link;
};

int  video_info_memory_block_init(struct device *dev);
void video_info_memory_block_exit(void);

int dump_video_info(char *strbuf, struct dma_buf *metadata);
int dump_video_info_memory_list(char *strbuf);

dma_addr_t video_info_buffer_init_dmabuf(struct frame_dma_buf_t *frame);
dma_addr_t video_info_buffer_init(dma_addr_t metadata_buf);

void video_info_buffer_deinit(struct frame_dma_buf_t *frame);

void video_info_buffer_update_overscan(struct frame_dma_buf_t *frame, const struct overscan_info *info);

#endif

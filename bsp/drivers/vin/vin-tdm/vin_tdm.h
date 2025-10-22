/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * Copyright (c) 2007-2019 Allwinnertech Co., Ltd.
 *
 * Authors:  Zheng Zequn <zequnzheng@allwinnertech.com>
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

#ifndef _VIN_TDM_H_
#define _VIN_TDM_H_

#include <linux/kernel.h>
#include "../platform/platform_cfg.h"

#ifdef TDM_V200
#include "tdm200/tdm200_reg.h"
#else
#include "tdm100/tdm_reg.h"
#endif
#include "../vin-video/vin_core.h"

#ifdef TDM_V200
/* #define TDM_USE_VGM */
#define TDM_BUFS_NUM 6
#define TDM_TX_HBLANK 256
#define TDM_TX_HBLANK_OFFLINE 128
#define TDM_TX_VBLANK 36
#else
#define TDM_BUFS_NUM 5
#define TDM_TX_HBLANK 96
#define TDM_TX_VBLANK 36
#endif

struct tdm_buffer {
	void *virt_addr;
	void *dma_addr;
	u32 buf_size;
	u8 empty;
};

enum tdm_state_t {
	TDM_USED = 0,
	TDM_NO_USED,
};

enum tdm_pad {
	TDM_PAD_SINK,
	TDM_PAD_SOURCE,
	TDM_PAD_NUM,
};

enum input_bit_width {
	RAW_8BIT = 8,
	RAW_10BIT = 10,
	RAW_12BIT = 12,
};

struct tdm_format {
	u32 code;
	enum input_bit_width input_bit_width;
	enum input_image_type_sel input_type;
	enum tdm_input_fmt raw_fmt;
};

#ifdef TDM_V200
struct tdm_work_status {
	bool wdr_work;
	bool speed_dn_en;
	bool tdm_en;
	enum  rx_chn_cfg_mode rx_chn_mode;
	enum  tx_chn_cfg_mode tx_chn_mode;
	unsigned int chn_total;
};

struct rx_work_status {
	u8 wdr_mode;
	bool tx_func_en;
	bool lbc_en;
	bool pkg_en;
	bool sync_en;
	bool line_num_ddr_en;
	u16 data_fifo_depth;
	u16 head_fifo_depth;
	u8 tx_rate_numerator;
	u8 tx_rate_denominator;
};
#endif

struct tdm_rx_dev {
	unsigned char id;
	struct v4l2_subdev subdev;
	struct media_pad tdm_pads[TDM_PAD_NUM];
	struct mutex subdev_lock;
	struct tdm_format *tdm_fmt;
	struct v4l2_mbus_framefmt format;
	u32 stream_cnt;
	u32 width;
	u32 height;
	u8 large_image;
	u32 sensor_fps;
	u32 isp_clk;
	u32 event_type;
	u32 vts;
#ifdef TDM_V200
	bool streaming;
	struct rx_work_status ws;
	struct tdm_rx_lbc lbc;
	struct list_head *list;
#endif
	/* Buffer */
	u32 buf_size;
	u8 buf_cnt;
	struct vin_mm ion_man[TDM_BUFS_NUM]; /* for ion alloc/free manage */
	struct tdm_buffer buf[TDM_BUFS_NUM];
	unsigned int stream_count;
};

struct rx_chn_fmt {
	struct list_head list;
	u32 wdr_mode;
	struct tdm_rx_dev *rx_dev;
};

struct tdm_dev {
	void __iomem *base;
	unsigned char id;
	int irq;
	char is_empty;
	struct platform_device *pdev;
	spinlock_t slock;

	struct tdm_rx_dev tdm_rx[TDM_RX_NUM];
	bool tdm_rx_buf_en[TDM_RX_NUM];
	bool tdm_rx_reset[TDM_RX_NUM];

	/* Control */
	bool configured;
	enum tdm_state_t state;	/* enabling/disabling state */
	struct mutex ioctl_lock; /* serialize private ioctl */
	u32 stream_cnt;
#ifdef TDM_V200
	struct tdm_work_status ws;
	struct tdm_tx_cfg tx_cfg;
	struct list_head working_chn_fmt;
	struct work_struct tdm_reset_task;
	u32 work_mode;
	u32 bitmap_chn_use;
	u32 rx_stream_cnt[TDM_RX_NUM];
#endif
};

int sunxi_tdm_subdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param);
void sunxi_tdm_fps_clk(struct v4l2_subdev *sd, int fps, unsigned int isp_clk, unsigned int vts);
void sunxi_tdm_buffer_free(unsigned int id);
int sunxi_tdm_platform_register(void);
void sunxi_tdm_platform_unregister(void);
struct v4l2_subdev *sunxi_tdm_get_subdev(int id, int tdm_rx_num);

#endif /* _VIN_TDM_H_ */

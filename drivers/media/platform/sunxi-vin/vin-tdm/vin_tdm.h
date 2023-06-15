/*
 * linux-5.4/drivers/media/platform/sunxi-vin/vin-tdm/vin_tdm.h
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
#include "tdm_reg.h"
#include "../vin-video/vin_core.h"

#define TDM_BUFS_NUM 5
#define TDM_TX_HBLANK 96
#define TDM_TX_VBLANK 36

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
};

struct tdm_rx_dev {
	unsigned char id;
	struct v4l2_subdev subdev;
	struct media_pad tdm_pads[TDM_PAD_NUM];
	struct mutex subdev_lock;
	struct tdm_format *tdm_fmt;
	struct v4l2_mbus_framefmt format;
	u32 stream_cnt;
	u32 width;
	u32 heigtht;

	/* Buffer */
	u32 buf_size;
	u32 buf_cnt;
	struct vin_mm ion_man[TDM_BUFS_NUM]; /* for ion alloc/free manage */
	struct tdm_buffer buf[TDM_BUFS_NUM];
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

	/* Control */
	bool configured;
	enum tdm_state_t state;	/* enabling/disabling state */
	struct mutex ioctl_lock; /* serialize private ioctl */
	u32 stream_cnt;
};

int sunxi_tdm_platform_register(void);
void sunxi_tdm_platform_unregister(void);
struct v4l2_subdev *sunxi_tdm_get_subdev(int id, int tdm_rx_num);

#endif /*_VIN_TDM_H_*/


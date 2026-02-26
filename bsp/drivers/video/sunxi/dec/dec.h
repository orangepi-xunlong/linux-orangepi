/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
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
#ifndef _DEC_H
#define _DEC_H

#ifdef __cplusplus
extern "C" {
#endif
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/stddef.h>
#include <linux/kfifo.h>
#include <linux/kref.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <video/decoder_display.h>
#include "dec_reg.h"
#include "dec_fence.h"

#include "frame_manager/dec_frame_manager.h"

#define DEC_ALIGN(value, align)												   \
	((align == 0) ? value : (((value) + ((align)-1)) & ~((align)-1)))


/* dec_format_attr - display format attribute
 *
 * @format: pixel format
 * @bits: bits of each component
 * @hor_rsample_u: reciprocal of horizontal sample rate
 * @hor_rsample_v: reciprocal of horizontal sample rate
 * @ver_rsample_u: reciprocal of vertical sample rate
 * @hor_rsample_v: reciprocal of vertical sample rate
 * @uvc: 1: u & v component combined
 * @interleave: 0: progressive, 1: interleave
 * @factor & div: bytes of pixel = factor / div (bytes)
 *
 */
struct dec_format_attr {
	enum dec_pixel_format_t format;
	unsigned int bits;
	unsigned int hor_rsample_u;
	unsigned int hor_rsample_v;
	unsigned int ver_rsample_u;
	unsigned int ver_rsample_v;
	unsigned int uvc;
	unsigned int interleave;
	unsigned int factor;
	unsigned int div;
};

struct memory_block;

struct frame_dma_buf_t {
	struct list_head list;
	unsigned int id;
	struct dec_dmabuf_t *image_item;
	struct dec_dmabuf_t *info_item;
	unsigned long long frame_addr[2];
	unsigned long long info_addr;
	struct memory_block *vinfo_block;

	struct dma_fence *fence;

	/* second field addr for interlace frame */
	unsigned int interlace;
	unsigned int top_field_first;
	unsigned long long second_field_addr[2];

	bool is_compressed;
	enum dec_pixel_format_t format;
	unsigned int width;
	unsigned int height;

	struct kref refcount;
};

#define VSYNC_NUM 8
struct dec_vsync_t {
	spinlock_t slock;
	ktime_t vsync_timestamp[VSYNC_NUM];
	u32 vsync_timestamp_head;
	u32 vsync_timestamp_tail;
};

struct dec_dump_info {
	unsigned int skip_cnt;
	unsigned int irq_cnt;
	unsigned long sync_time[100];/* for_debug */
	unsigned int sync_time_index;/* for_debug */
};

struct dec_device {
	struct mutex dev_lock;

	struct device *p_device;

	struct device attr_device;
	struct attribute_group *p_attr_grp;

	struct afbd_reg_t *p_reg;
	u32 dec_irq_no;

	struct dec_dump_info *p_info;

	u32 afbd_clk_frequency;
	struct clk *afbd_clk;
	struct clk *disp_gate;
	struct reset_control *rst_bus_disp;

	struct dec_fence_context *fence_context;
	struct dec_vsync_t *p_vsync_info;

	_Bool enabled;

	struct dec_frame_manager *fmgr;
	wait_queue_head_t event_wait;
#if FPGA_DEBUG_ENABLE == 1
	struct task_struct *vsync_task;
#endif

	_Bool (*is_enabled)(struct dec_device *p_dev);
	int (*enable)(struct dec_device *p_dev);
	int (*disable)(struct dec_device *p_dev);
	int (*frame_submit)(struct dec_device *p_dev, struct dec_frame_config *p_cfg, int *fence, int repeat_count);
	int (*interlace_setup)(struct dec_device *p_dev, struct dec_frame_config *p_cfg);
	int (*stop_video_stream)(struct dec_device *p_dev);
	int (*bypass_config)(struct dec_device *p_dev, int enable);
	int (*reg_protect)(struct dec_device *p_dev, _Bool protect);
	int (*suspend)(struct dec_device *p_dev);
	int (*resume)(struct dec_device *p_dev);
	void (*sync_finish)(struct dec_device *p_dev);

	int (*vsync_timestamp_get)(struct dec_device *p_dev, struct dec_vsync_timestamp *ts);
	unsigned int (*poll)(struct dec_device *p_dev, struct file *filp, poll_table *wait);

	int (*set_overscan)(struct dec_device *p_dev, struct overscan_info *info);
};

struct dec_device *dec_init(struct device *p_device);

int dec_exit(struct dec_device *p_dec);

void dec_debug_set_register_base(void *base);
void dec_debug_trace_dma_buf_map(void *token, uint32_t addr, uint32_t size, struct memory_block *vinfo);
void dec_debug_trace_dma_buf_unmap(void *token, uint32_t addr);
void dec_debug_trace_frame(uint32_t addr, int interlace);
void dec_debug_init(void);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/

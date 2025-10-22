/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c)	2011 Allwinnertech Co., Ltd.
 *					2011 Yupu Tang
 *
 * @ G2D driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 */

#ifndef __G2D_DRIVER_I_H
#define __G2D_DRIVER_I_H

#include <sunxi-log.h>
#include "g2d_bsp.h"

/* #include "g2d_bsp_v2.h" */

#define G2D_DBG(format, args...) \
	sunxi_debug(NULL, "[G2D]: " format, ## args)
#define G2D_ERR(format, args...) \
	sunxi_err(NULL, "[G2D]: " format, ## args)
#define G2D_WARN(format, args...) \
	sunxi_warn(NULL, "[G2D]: " format, ## args)
#define G2D_INFO(format, args...) \
	sunxi_info(NULL, "[G2D]: " format, ## args)

#define MAX_G2D_MEM_INDEX	10
#define	INTC_IRQNO_DE_MIX	SUNXI_IRQ_MP

#define G2DALIGN(value, align) ((align == 0) ? \
				value : \
				(((value) + ((align) - 1)) & ~((align) - 1)))

struct info_mem {
	unsigned long phy_addr;
	void *virt_addr;
	__u32 b_used;
	__u32 mem_len;
};

typedef struct {
	struct device *dev;
	struct resource *mem;
	void __iomem *io;
	__u32 irq;
	struct mutex mutex;
	struct clk *clk;
	bool opened;
	__u32 user_cnt;
	struct clk *clk_parent;
	struct clk *bus_clk;
	struct clk *mbus_clk;
	struct reset_control *reset;
	__u32 iommu_master_id;
} __g2d_info_t;

typedef struct {
	__u32 mid;
	__u32 used;
	__u32 status;
	struct semaphore *g2d_finished_sem;
	struct semaphore *event_sem;
	wait_queue_head_t queue;
	__u32 finish_flag;
} __g2d_drv_t;

struct g2d_alloc_struct {
	__u32 address;
	__u32 size;
	__u32 u_size;
	struct g2d_alloc_struct *next;
};

/* g2d_format_attr - g2d format attribute
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
 * @addr[out]: address for each plane
 * @trd_addr[out]: address for each plane of right eye buffer
 */
struct g2d_format_attr {
	g2d_fmt_enh format;
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


irqreturn_t g2d_handle_irq(int irq, void *dev_id);
int g2d_init(g2d_init_para *para);
int g2d_blit(g2d_blt *para);
int g2d_fill(g2d_fillrect *para);
int g2d_stretchblit(g2d_stretchblt *para);
/* int g2d_set_palette_table(g2d_palette *para); */
int g2d_wait_cmd_finish(void);
int g2d_cmdq(unsigned int para);
int g2d_suspend(struct device *dev);
int g2d_resume(struct device *dev);

#if IS_ENABLED(CONFIG_AW_IOMMU)
extern void sunxi_reset_device_iommu(unsigned int master_id);
#endif

#endif /* __G2D_DRIVER_I_H */

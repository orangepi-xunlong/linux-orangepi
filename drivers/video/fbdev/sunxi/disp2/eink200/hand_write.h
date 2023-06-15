/*
 * hand_write.h
 *
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
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
#ifndef _HAND_WRITE_H
#define _HAND_WRITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include "include/eink_sys_source.h"

/**
 * hand dma buffer
 */
struct hand_write_dma_t {
	int fd;
	struct dmabuf_item *item;
	struct list_head   node;
};

struct hand_write_t {
	struct list_head	hw_used_list;
	struct list_head	hw_free_list;
	struct delayed_work hw_delay_work;
	struct workqueue_struct *p_hw_wq;
	struct mutex list_lock;
	void (*handwrite_unmap_work)(struct work_struct *work);
};

int get_hand_write_phaddr(int fd, u32 *paddr);

void release_hand_write_memory(int fd);

int hand_write_init(void);

#ifdef __cplusplus
}
#endif

#endif /*End of file*/

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "disp_manager.h"
#include "disp_display.h"
#include <linux/kfifo.h>

#define DATA_SIZE		5
#define TAG_SIZE		32
struct mgr_debug {
	int disp;
	unsigned int line;
	char tag[TAG_SIZE];
	unsigned int scanf_line;
	unsigned long time;
	int user_data0[DATA_SIZE];
	unsigned int user_data1[DATA_SIZE];
	unsigned long user_data2[DATA_SIZE];
};

/* CNT_PER_FRAME_MAX * FRAME_CNT should be a power of 2 */
#define CNT_PER_FRAME_MAX	64
#define FRAME_CNT		8

#define ELEMENT_SIZE		(CNT_PER_FRAME_MAX * FRAME_CNT)
#define SIZE_THRESHOLD		(ELEMENT_SIZE * 3 / 4)

/* note kfifo must be with the same fifo elements and element size */

DEFINE_KFIFO(disp0_debug_fifo, struct mgr_debug, ELEMENT_SIZE);
DEFINE_KFIFO(disp0_debug_fifo_irq, struct mgr_debug, ELEMENT_SIZE);

DEFINE_KFIFO(disp1_debug_fifo, struct mgr_debug, ELEMENT_SIZE);
DEFINE_KFIFO(disp1_debug_fifo_irq, struct mgr_debug, ELEMENT_SIZE);

struct mgr_debug outdated[CNT_PER_FRAME_MAX];

static inline typeof(&disp0_debug_fifo) get_kfifo_ptr(int disp, bool irq)
{
	if (disp == 0)
		return irq ? (void *)&disp0_debug_fifo_irq : (void *)&disp0_debug_fifo;
	else
		return irq ? (void *)&disp1_debug_fifo_irq : (void *)&disp1_debug_fifo;
}

static inline int get_dmabuf_size(struct dmabuf_item *item)
{
	int i = 0;
	int size = 0;
	struct scatterlist *sgl;
	struct sg_table *sgt = item->sgt;

	for_each_sg(sgt->sgl, sgl, sgt->nents, i) {
		size += sg_dma_len(sgl);
	}
	return size;
}

static inline void debug_info_init(int disp, struct mgr_debug *debug, unsigned int line, const char *tag)
{
	memset(debug, 0, sizeof(*debug));
	memcpy(debug->tag, tag, strlen(tag) <= TAG_SIZE ? strlen(tag) : TAG_SIZE);
	debug->tag[TAG_SIZE - 1] = 0;
	debug->disp = disp;
	debug->line = line;
	debug->scanf_line = bsp_disp_get_cur_line(disp);
	debug->time = (unsigned long)ktime_to_us(ktime_get());
}

static inline void mgr_debug_info_put(int disp, struct mgr_debug *debug, bool irq)
{
	typeof(&disp0_debug_fifo) fifop = get_kfifo_ptr(disp, irq);
	kfifo_put(fifop, *debug);
}

#define DECLARE_DEBUG	struct mgr_debug debug
#define mgr_debug_simple_add(disp, irq, tag)				\
	{							\
		debug_info_init(disp, &debug, __LINE__, tag);	\
		mgr_debug_info_put(disp, &debug, irq);		\
	}

static inline void mgr_debug_info_out_resize(int disp)
{
	int ret;
	int irq;
	typeof(&disp0_debug_fifo) fifop;

	for (irq = 0; irq <= 1; irq++) {
		fifop = get_kfifo_ptr(disp, irq);
		if (kfifo_len(fifop) >= SIZE_THRESHOLD)
			ret = kfifo_out(fifop, outdated, CNT_PER_FRAME_MAX);
	}
}

/* read but not remove, avoid keep adding useless debug info while print */
static inline void print_mgr_debug_info(int disp, bool irq)
{
	typeof(&disp0_debug_fifo) fifop = get_kfifo_ptr(disp, irq);
	struct mgr_debug *fifo;
	struct mgr_debug *fifo_bak;
	int cnt;
	char *tmp;
	int i, j;
	const int size = 4096;
	u32 count = 0;
	tmp = kzalloc(size, GFP_KERNEL);
	fifo = kzalloc(sizeof(*fifo) * ELEMENT_SIZE, GFP_KERNEL);
	fifo_bak = fifo;
	if (!(fifo && tmp)) {
		DE_WARN("dump kfifo fail, nomem. disp %d irq %d\n", disp, irq);
		goto done;
	}

	cnt = kfifo_out_peek(fifop, fifo, ELEMENT_SIZE);
	DE_WARN("kfifo size %d disp %d irq %d\n", cnt, disp, irq);

	for (i = 0; i < cnt; i++, count = 0) {
		memset(tmp, 0, size);
		for (j = 0; j < DATA_SIZE; j++) {
			count += sprintf(tmp + count, "%d \t%u \t%lx\n",
				  fifo->user_data0[j], fifo->user_data1[j], fifo->user_data2[j]);
		}

		DE_INFO("disp_mgr_debug: disp:%d l:%u %s sl:%u t:%lu:\n%s",
			  fifo->disp, fifo->line, fifo->tag, fifo->scanf_line, fifo->time, tmp);
		/* avoid keep logging */
		msleep(2);
		fifo += 1;
	}

done:
	if (fifo_bak)
		kfree(fifo_bak);
	if (tmp)
		kfree(tmp);
}

static inline void mgr_debug_info_out_reset(int disp)
{
	int irq;
	typeof(&disp0_debug_fifo) fifop;

	for (irq = 0; irq <= 1; irq++) {
		fifop = get_kfifo_ptr(disp, irq);
		kfifo_reset_out(fifop);
	}
}

static inline void mgr_debug_commit_routine(int disp, bool *stop)
{
	static int once;
	if (*stop && !once) {
		print_mgr_debug_info(disp, 0);
		print_mgr_debug_info(disp, 1);
//		mgr_debug_info_out_reset(disp);
		*stop = false;
		once = 1;
	} else {
		mgr_debug_info_out_resize(disp);
	}
}

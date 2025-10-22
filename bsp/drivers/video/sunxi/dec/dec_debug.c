/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2022  Allwinnertech
 * Author: yajianz <yajianz@allwinnertech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <asm/io.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <sunxi-iommu.h>

#include "dec_video_info.h"
#include "video_decoder_datatype.h"

typedef struct trace_buf_info {
	uint32_t address;
	int interlace;
} trace_buf_t;

#define FRAME_TRACE_LEN 64
typedef struct frame_trace_data {
	int wpos;
	trace_buf_t data[FRAME_TRACE_LEN];
} frame_trace_t;

typedef struct trace_dma_map_info {
	void *token;
	uint32_t iommu_addr;
	uint32_t length;

	/*
	 * image buffer info
	 */
	uint32_t width;
	uint32_t height;
	uint32_t format;

	int map;
} dma_map_info_t;

#define DMA_MAP_TRACE_LEN 64
typedef struct dma_map_trace_data {
	spinlock_t lock;
	int wpos;
	dma_map_info_t data[DMA_MAP_TRACE_LEN];
} dma_map_trace_t;

#define DEBUG_REG_LEN 128
typedef struct dec_debug_data {
	int exception;
	void __iomem *regbase;
	uint32_t registers[DEBUG_REG_LEN];
	// frame trace info
	frame_trace_t frame;
	dma_map_trace_t dmamap;

	struct work_struct work;
} dec_debug_data_t;

static dec_debug_data_t dbgdat;

void dec_debug_set_register_base(void *base)
{
	dbgdat.regbase = base;
}

/*
static void dec_debug_record_exception(void)
{
	int i;
	void __iomem *base;

	if (dbgdat.exception) {
		pr_err("exception had already record !\n");
		return;
	}
	dbgdat.exception = 1;

	for (i = 0; i < DEBUG_REG_LEN; i++) {
		base = dbgdat.regbase + (4 * i);
		dbgdat.registers[i] = readl(base);
	}

	schedule_work(&dbgdat.work);
}
*/

void dec_debug_trace_dma_buf_map(void *token, uint32_t addr, uint32_t size, struct memory_block *vinfo)
{
	int index = 0;
	unsigned long flags;
	VideoDecoderSignalInfo *info = NULL;

	if (dbgdat.exception)
		return;

	spin_lock_irqsave(&dbgdat.dmamap.lock, flags);
	index = dbgdat.dmamap.wpos % DMA_MAP_TRACE_LEN;
	dbgdat.dmamap.data[index].iommu_addr = addr;
	dbgdat.dmamap.data[index].length = size;
	dbgdat.dmamap.data[index].token = token;
	dbgdat.dmamap.data[index].map = 1;
	if (vinfo && vinfo->va) {
		info = (VideoDecoderSignalInfo *)vinfo->va;
		dbgdat.dmamap.data[index].format = info->color_format;
		dbgdat.dmamap.data[index].width  = info->pic_hsize_after_scaler;
		dbgdat.dmamap.data[index].height = info->pic_vsize_after_scaler;
	}
	dbgdat.dmamap.wpos++;
	spin_unlock_irqrestore(&dbgdat.dmamap.lock, flags);
}

void dec_debug_trace_dma_buf_unmap(void *token, uint32_t addr)
{
	int i;
	unsigned long flags;
	dma_map_info_t *record;

	if (dbgdat.exception)
		return;

	spin_lock_irqsave(&dbgdat.dmamap.lock, flags);
	for (i = 0; i < DMA_MAP_TRACE_LEN; i++) {
		record = &dbgdat.dmamap.data[i];
		if (record->token == token && record->iommu_addr == addr) {
			record->map = 0;
			break;
		}
	}
	spin_unlock_irqrestore(&dbgdat.dmamap.lock, flags);
}

void dec_debug_trace_frame(uint32_t addr, int interlace)
{
	int index = 0;

	if (dbgdat.exception)
		return;

	index = dbgdat.frame.wpos % FRAME_TRACE_LEN;
	dbgdat.frame.data[index].address = addr;
	dbgdat.frame.data[index].interlace = interlace;
	dbgdat.frame.wpos++;
}

int dec_debug_dump(char *linebuf, size_t linebuflen)
{
	int i;
	int count = 0;

	count = hex_dump_to_buffer(
			dbgdat.registers, sizeof(uint32_t) * DEBUG_REG_LEN, 16, 4,
			linebuf, linebuflen, 0);

	count += sprintf(linebuf+count, "\nframe info: wpos=%d\n", dbgdat.frame.wpos);
	for (i = 0; i < FRAME_TRACE_LEN; i++) {
		count += sprintf(linebuf+count, "[%02d] %08x %d\n", i, dbgdat.frame.data[i].address, dbgdat.frame.data[i].interlace);
	}

	return count;
}

static void dec_debug_work_func(struct work_struct *work)
{
	int i;

	pr_err("--- DEC_DEBUG: decoder display register ---\n");
	print_hex_dump(
			KERN_ERR, "DEC_DEBUG ", DUMP_PREFIX_OFFSET, 16, 4,
			dbgdat.registers, sizeof(uint32_t) * DEBUG_REG_LEN, 0);
	pr_err("-------------------------------------------\n");

	pr_err("frame info: wpos=%d\n", dbgdat.frame.wpos % FRAME_TRACE_LEN);
	for (i = 0; i < FRAME_TRACE_LEN; i++) {
		pr_err("[%02d] %08x %d\n", i, dbgdat.frame.data[i].address, dbgdat.frame.data[i].interlace);
	}

	pr_err("dma map info: wpos=%d\n", dbgdat.dmamap.wpos % DMA_MAP_TRACE_LEN);
	for (i = 0; i < DMA_MAP_TRACE_LEN; i++) {
		pr_err("[%02d] %08x size %08x %dx%d format %d map %d\n",
				i, dbgdat.dmamap.data[i].iommu_addr, dbgdat.dmamap.data[i].length,
				dbgdat.dmamap.data[i].width, dbgdat.dmamap.data[i].height, dbgdat.dmamap.data[i].format,
				dbgdat.dmamap.data[i].map);
	}
}

void dec_debug_init(void)
{
	INIT_WORK(&dbgdat.work, dec_debug_work_func);
	spin_lock_init(&dbgdat.dmamap.lock);
	/*sunxi_iommu_register_fault_cb(dec_debug_record_exception, 2);*/
}


/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * dec_video_info.c
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

#include <linux/of_reserved_mem.h>

#include "dma_buf.h"
#include "dec_video_info.h"
#include "video_decoder_datatype.h"

#include "dec_trace.h"

#define METADATA_BUFFER_SIZE	 (4096 * 8)
#define VIDEO_INFORMATION_OFFSET (4096)
#define VIDEO_INFORMATION_SIZE	 (sizeof(VideoDecoderSignalInfo) + sizeof(VideoDecoderHdrStaticMetadata) + sizeof(VideoDecoderHdr10PlusMetadata))

dma_addr_t video_info_buffer_init(dma_addr_t metadata_buf)
{
	return metadata_buf + VIDEO_INFORMATION_OFFSET;
}

typedef struct {
	int id;
	const char *tag;
} idname_t;

#define __NAME_ITEM(_id) { .id = _id, .tag = #_id }

static const char *hdr_scheme_name(uint32_t m)
{
	size_t i = 0;
	const idname_t enhance_mode_names[] = {
		__NAME_ITEM(kVideoDecoderHdrScheme_Sdr),
		__NAME_ITEM(kVideoDecoderHdrScheme_Hdr10),
		__NAME_ITEM(kVideoDecoderHdrScheme_Hdr10Plus),
		__NAME_ITEM(kVideoDecoderHdrScheme_Sl_Hdr1),
		__NAME_ITEM(kVideoDecoderHdrScheme_Sl_Hdr2),
		__NAME_ITEM(kVideoDecoderHdrScheme_Hlg),
		__NAME_ITEM(kVideoDecoderHdrScheme_DolbyVison),
		__NAME_ITEM(kVideoDecoderHdrScheme_Max),
	};

	for (i = 0; i < sizeof(enhance_mode_names)/sizeof(idname_t); i++) {
		if (enhance_mode_names[i].id == m) {
			return enhance_mode_names[i].tag;
		}
	}
	return "unknow";
}

static const char *color_format_name(uint32_t fmt)
{
	size_t i = 0;
	const idname_t color_format_names[] = {
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv420_888),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv420_1088),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv420_101010),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv420_121212),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv422_888),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv422_1088),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv422_101010),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv422_121212),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv444_888),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv444_101010),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv444_121212),
		__NAME_ITEM(kVideoDecoderColorFormat_Rgb_888),
		__NAME_ITEM(kVideoDecoderColorFormat_Rgb_101010),
		__NAME_ITEM(kVideoDecoderColorFormat_Rgb_121212),
		__NAME_ITEM(kVideoDecoderColorFormat_Rgb_565),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv420_101010_AFBD_P010_High),
		__NAME_ITEM(kVideoDecoderColorFormat_Yuv420_101010_AFBD_P010_Low),
		__NAME_ITEM(kVideoDecoderColorFormat_Max),
	};

	for (i = 0; i < sizeof(color_format_names)/sizeof(idname_t); i++) {
		if (color_format_names[i].id == fmt) {
			return color_format_names[i].tag;
		}
	}
	return "unknow";
}

static const char *color_space_name(uint32_t s)
{
	size_t i = 0;
	const idname_t color_space_names[] = {
		__NAME_ITEM(kVideoDecoderColorSpace_Bt601),
		__NAME_ITEM(kVideoDecoderColorSpace_Bt709),
		__NAME_ITEM(kVideoDecoderColorSpace_Bt2020_Nclycc),
		__NAME_ITEM(kVideoDecoderColorSpace_Bt2020_Clycc),
		__NAME_ITEM(kVideoDecoderColorSpace_Xvycc),
		__NAME_ITEM(kVideoDecoderColorSpace_Rgb),
		__NAME_ITEM(kVideoDecoderColorSpace_Max),
	};

	for (i = 0; i < sizeof(color_space_names)/sizeof(idname_t); i++) {
		if (color_space_names[i].id == s) {
			return color_space_names[i].tag;
		}
	}
	return "unknow";
}

static int dump_info(char *strbuf, VideoDecoderSignalInfo *info)
{
	int printed = 0;

	printed += sprintf(strbuf + printed, "	version: %zu 0x%08x\n",
			sizeof(VideoDecoderSignalInfo),
			info->interface_version_0);

	printed += sprintf(strbuf + printed, "	frame info: size %d(stride=%d)x%d, %d fps(x1000), scan=[%s], AFBC compress [%s]\n",
			info->pic_hsize_after_scaler, info->buffer_stride, info->pic_vsize_after_scaler,
			info->frame_rate_before_frame_rate_change, info->b_interlace ? "interlaced" : "progressive",
			info->b_compress_en ? "YES" : "NO");
	printed += sprintf(strbuf + printed, "	%s %s %s\n",
			color_format_name(info->color_format), color_space_name(info->color_space), hdr_scheme_name(info->hdr_scheme));

	printed += sprintf(strbuf + printed, "	offset [%d %d %d %d]\n",
			info->display_properties.destination.h_start >> 4,
			info->display_properties.destination.h_size  >> 4,
			info->display_properties.destination.v_start >> 4,
			info->display_properties.destination.v_size  >> 4);

	return printed;
}

int dump_video_info(char *strbuf, struct dma_buf *metadata)
{
	int ret;
	int printed = 0;
	char *p_metadata = 0;
	VideoDecoderSignalInfo *info;
	struct dma_buf_map map;

	if (metadata) {
		ret = dma_buf_begin_cpu_access(metadata, DMA_FROM_DEVICE);
		if (ret) {
			pr_err("dma_buf_begin_cpu_access failed\n");
			return 0;
		}

		ret = dma_buf_vmap(metadata, &map);
		if (ret) {
			dma_buf_end_cpu_access(metadata, DMA_FROM_DEVICE);
			pr_err("dma_buf_vmap failed\n");
			return 0;
		}
		p_metadata = (char *)map.vaddr;
	}

	if (!p_metadata) {
		pr_err("no metadata buf input\n");
		return 0;
	}

	info = (VideoDecoderSignalInfo *)(p_metadata + VIDEO_INFORMATION_OFFSET);
	printed = dump_info(strbuf, info);

	dma_buf_vunmap(metadata, &map);
	dma_buf_end_cpu_access(metadata, DMA_FROM_DEVICE);

	return printed;
}

#define VIDEO_INFO_BLOCK_SIZE (4096)

static struct list_head memory_list_head;
static struct list_head onused_list_head;
static spinlock_t list_lock;
static void *video_info_block_start;

static void *kmap_memory_block(unsigned long phys_addr, unsigned long size)
{
	int npages = PAGE_ALIGN(size) / PAGE_SIZE;
	struct page **pages = vmalloc(sizeof(struct page *) * npages);
	struct page **tmp = pages;
	struct page *cur_page = phys_to_page(phys_addr);
	pgprot_t pgprot;
	void *vaddr = NULL;
	int i;

	if (!pages)
		return NULL;

	for (i = 0; i < npages; i++)
		*(tmp++) = cur_page++;

	pgprot = pgprot_noncached(PAGE_KERNEL);
	vaddr = vmap(pages, npages, VM_MAP, pgprot);

	vfree(pages);
	return vaddr;
}

void clean_video_info_free_list(void)
{
	struct memory_block *block, *next;

	pr_info("try clean video info free list: %d\n", VIDEO_INFO_BLOCK_SIZE);

	block = NULL;
	list_for_each_entry_safe(block, next, &memory_list_head, link) {
		memset(block->va,	0, VIDEO_INFO_BLOCK_SIZE);
		pr_info("clean : %pK\n", block->va);
	}
}

static int video_info_reserved_mem_get(
		struct device *dev, dma_addr_t *addr, size_t *size)
{
	struct device_node *mem_node;
	struct reserved_mem *rmem;

	mem_node = of_parse_phandle(dev->of_node, "memory-region", 0);
	if (!mem_node) {
		dev_err(dev, "No memory-region found\n");
		return -ENODEV;
	}

	rmem = of_reserved_mem_lookup(mem_node);
	if (!rmem) {
		dev_err(dev, "of_reserved_mem_lookup() returned NULL\n");
		return -ENODEV;
	}
	*addr = rmem->base;
	*size = rmem->size;
	return 0;
}

int video_info_memory_block_init(struct device *dev)
{
	int i = 0, block_num = 0;
	void *video_info_block = 0;
	dma_addr_t reserved_mem_start;
	size_t reserved_mem_size;

	if (video_info_reserved_mem_get(dev, &reserved_mem_start, &reserved_mem_size) != 0) {
		dev_err(dev, "video info reserved memory not define!\n");
		return -1;
	}

	block_num = reserved_mem_size / VIDEO_INFO_BLOCK_SIZE;
	video_info_block = kmap_memory_block(reserved_mem_start, reserved_mem_size);

	if (!video_info_block) {
		pr_err("map video information block failed\n");
		return -1;
	}

	pr_info("video_info_block: %pK\n", video_info_block);

	INIT_LIST_HEAD(&memory_list_head);
	INIT_LIST_HEAD(&onused_list_head);

	for (i = 0; i < block_num; i++) {
		struct memory_block *block = kmalloc(
				sizeof(struct memory_block), GFP_KERNEL | __GFP_ZERO);
		if (block == NULL) {
			pr_err("kmalloc for memory_block failed\n");
			return -ENOMEM;
		}

		block->va		= video_info_block   + i * VIDEO_INFO_BLOCK_SIZE;
		block->physical = reserved_mem_start + i * VIDEO_INFO_BLOCK_SIZE;
		block->token	= i;
		list_add(&block->link, &memory_list_head);
	}

	clean_video_info_free_list();

	spin_lock_init(&list_lock);

	video_info_block_start = video_info_block;

	return 0;
}

void video_info_memory_block_exit(void)
{
	struct memory_block *block, *next;
	unsigned long flags;

	spin_lock_irqsave(&list_lock, flags);
	list_for_each_entry_safe(block, next, &memory_list_head, link) {
		list_del(&block->link);
		pr_info("remove block: %d\n", block->token);
		kfree(block);
	}
	spin_unlock_irqrestore(&list_lock, flags);

	if (video_info_block_start) {
		vunmap(video_info_block_start);
		video_info_block_start = 0;
	}

	INIT_LIST_HEAD(&memory_list_head);
}

int dump_video_info_memory_list(char *strbuf)
{
#define MAX_PRINT_COUNT (8)
	int printed = 0;
	struct memory_block *block, *next;
	unsigned long flags;
	VideoDecoderSignalInfo *info = NULL;
	int total_onused = 0;

	printed += sprintf(strbuf + printed, "video info free list:\n");

	spin_lock_irqsave(&list_lock, flags);
	list_for_each_entry_safe(block, next, &memory_list_head, link) {
		printed += sprintf(strbuf + printed, "	%02d: p:%08x v:0x%p\n",
				block->token, (int)block->physical, block->va);
	}
	spin_unlock_irqrestore(&list_lock, flags);

	printed += sprintf(strbuf + printed, "video info using list:\n");
	block = NULL;
	next  = NULL;

	spin_lock_irqsave(&list_lock, flags);
	list_for_each_entry_safe(block, next, &onused_list_head, link) {
		printed += sprintf(strbuf + printed, "	%02d: p:%08x v:0x%p\n",
				block->token, (int)block->physical, block->va);
		printed += sprintf(strbuf + printed, "	parent: 0x%p\n", block->parent);

		info = (VideoDecoderSignalInfo *)block->va;

		if (total_onused++ < MAX_PRINT_COUNT)
			printed += dump_info(strbuf + printed, info);
	}
	spin_unlock_irqrestore(&list_lock, flags);

	if (total_onused > MAX_PRINT_COUNT)
		printed += sprintf(strbuf + printed,
				"!!! [%d] video info free list not print.\n",
				total_onused - MAX_PRINT_COUNT);

	return printed;
}

struct memory_block *alloc_video_info(void)
{
	struct memory_block *retval = NULL;
	struct memory_block *block, *next;
	unsigned long flags;

	spin_lock_irqsave(&list_lock, flags);
	list_for_each_entry_safe(block, next, &memory_list_head, link) {
		list_del_init(&block->link);
		list_add_tail(&block->link, &onused_list_head);
		retval = block;
		break;
	}
	spin_unlock_irqrestore(&list_lock, flags);

	return retval;
}

void free_video_info(struct memory_block *block)
{
	unsigned long flags;

	DEC_TRACE_FUNC();
	spin_lock_irqsave(&list_lock, flags);
	list_del_init(&block->link);
	list_add_tail(&block->link, &memory_list_head);
	spin_unlock_irqrestore(&list_lock, flags);
	DEC_TRACE_END("");
}

void video_info_buffer_deinit(struct frame_dma_buf_t *frame)
{
	DEC_TRACE_FUNC();

	if (frame && frame->vinfo_block)
		free_video_info(frame->vinfo_block);

	DEC_TRACE_END("");
}

void video_info_buffer_update_overscan(struct frame_dma_buf_t *frame, const struct overscan_info *info)
{
	struct memory_block *block = NULL;
	VideoDecoderSignalInfo *signal_info = NULL;

	if (!frame || !frame->vinfo_block) {
		pr_warn("update overscan failed! invalid input params.\n");
		return;
	}

	block = alloc_video_info();
	if (!block) {
		pr_warn("update overscan failed! alloc_video_info error.\n");
		return;
	}

	// copy video info from legacy block to the new one
	memcpy(block->va, frame->vinfo_block->va, VIDEO_INFORMATION_SIZE);
	signal_info = (VideoDecoderSignalInfo *)block->va;
	signal_info->display_top_offset = info->display_top_offset;
	signal_info->display_bottom_offset = info->display_bottom_offset;
	signal_info->display_left_offset = info->display_left_offset;
	signal_info->display_right_offset = info->display_right_offset;
	signal_info->display_properties.source.h_start = info->source_win.h_start;
	signal_info->display_properties.source.h_size = info->source_win.h_size;
	signal_info->display_properties.source.v_start = info->source_win.v_start;
	signal_info->display_properties.source.v_size = info->source_win.v_size;

	free_video_info(frame->vinfo_block);
	frame->vinfo_block = block;
	frame->info_addr = block->physical;
}

dma_addr_t video_info_buffer_init_dmabuf(struct frame_dma_buf_t *frame)
{
	int ret;
	char *p_metadata = 0;
	VideoDecoderSignalInfo *info;
	VideoDecoderSignalInfo *dst;
	struct dma_buf *metadata;
	struct memory_block *block;
	struct dma_buf_map map;

	if (!frame || !frame->info_item) {
		pr_info("invalid input params\n");
		return 0;
	}

	metadata = frame->info_item->dmabuf;

	if (metadata) {
		ret = dma_buf_begin_cpu_access(metadata, DMA_FROM_DEVICE);
		if (ret) {
			pr_err("dma_buf_begin_cpu_access failed\n");
			return 0;
		}

		ret = dma_buf_vmap(metadata, &map);
		if (ret) {
			dma_buf_end_cpu_access(metadata, DMA_FROM_DEVICE);
			pr_err("dma_buf_vmap failed\n");
			return 0;
		}
		p_metadata = (char *)map.vaddr;

	}

	if (!p_metadata) {
		pr_err("no metadata buf input\n");
		return 0;
	}

	info = (VideoDecoderSignalInfo *)(p_metadata + VIDEO_INFORMATION_OFFSET);

	block = alloc_video_info();
	if (!block) {
		pr_err("alloc video info block failed\n");
		dma_buf_vunmap(metadata, &map);
		dma_buf_end_cpu_access(metadata, DMA_FROM_DEVICE);
		return 0;
	}
	pr_debug("info block %02d: p:%pad v:%pK\n",
			block->token, &block->physical, block->va);

	frame->vinfo_block = block;
	block->parent = frame;

	// copy video info from dmabuf to shared memory
	memcpy(block->va, info, VIDEO_INFORMATION_SIZE);

	// init hdr metadata buffer address for mips
	dst = (VideoDecoderSignalInfo *)block->va;
	dst->hdr_static_metadata_offset  = block->physical + sizeof(VideoDecoderSignalInfo);
	dst->hdr_dynamic_metadata_offset = block->physical + sizeof(VideoDecoderSignalInfo) + sizeof(VideoDecoderHdrStaticMetadata);

	if (dst->interface_version_0 != 0x61770000) {
		pr_err("invalid video info version: 0x%08x\n", dst->interface_version_0);
	}

	dma_buf_vunmap(metadata, &map);
	dma_buf_end_cpu_access(metadata, DMA_FROM_DEVICE);

	return block->physical;
}


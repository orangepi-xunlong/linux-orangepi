/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* sunxi_de_v35x.c
 *
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#include <linux/version.h>
#include <linux/of_device.h>
#include <linux/hrtimer.h>
#include <linux/dma-mapping.h>
#include <linux/pm_runtime.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/wait.h>
#include <linux/of_graph.h>
#include <linux/component.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <drm/drm_atomic_helper.h>
#include <uapi/drm/drm_fourcc.h>
#include <drm/drm_print.h>
#include <drm/drm_drv.h>
#include <drm/drm_vblank.h>
#include <linux/proc_fs.h>
#if defined(CONFIG_PM_DEVFREQ)
#include <../drivers/devfreq/governor.h>
#include <linux/devfreq.h>
#endif
#include <linux/pm_opp.h>

#include "sunxi_drm_drv.h"
#include "sunxi_drm_crtc.h"
#include "sunxi_de.h"
#include "de_channel.h"
#include "de_bld.h"
#include "de_top.h"
#include "de_wb.h"
#include "de_backend.h"
#include "de_fmt.h"
#include "sunxi_drm_debug.h"
#include "sunxi_drm_trace.h"

#define CHANNEL_MAX			(6)
#define DE_BLOCK_SIZE			(256000)

#if defined(CONFIG_PM_DEVFREQ)
struct devfreq_dev_profile sunxi_defreq_userspace_profile;
#endif

struct de_match_data {
	unsigned int version;
	enum de_update_mode update_mode;
	/* ui channel no csc, must blending in rgb */
	bool blending_in_rgb;
	bool rcq_wait_line;
	bool rcq_skip_read;
};

struct de_reg_buffer {
	unsigned long virt_addr;
	unsigned long phys_addr;
	unsigned long used_byte;
};

struct de_rcq_mem_info {
	u8 __iomem *phy_addr;
	struct de_rcq_head *vir_addr;
	struct de_reg_block **reg_blk;
	u32 block_num;
	u32 block_num_aligned;
};

struct sunxi_de_wb {
	struct sunxi_drm_wb *drm_wb;
	struct de_wb_handle *wb_hdl;
	bool wb_enable;
	unsigned int wb_disp;
};

enum de_update_status {
	DE_UPDATE_FINISHED = 0,
	DE_UPDATE_PENDING,
};

struct sunxi_de_debug {
	struct proc_dir_entry *procfs_de_top;
	int force_offline_mode;
};

struct sunxi_de_out {
	int id;
	int port_id;
	struct device *dev;
	struct hrtimer rcq_timer;
	bool enable;
	atomic_t update_finish;
	atomic_t schedule_next_frame;
	struct sunxi_drm_crtc *scrtc;
	struct device_node *port;
	struct de_bld_handle *bld_hdl;
	struct de_fmt_handle *fmt_hdl;
	struct de_backend_handle *backend_hdl;
	unsigned int ch_cnt;
	struct de_channel_handle *ch_hdl[CHANNEL_MAX];
	struct de_rcq_mem_info rcq_info;
	struct de_rcq_mem_info rcq_info_test0;
	struct de_rcq_mem_info rcq_info_test1;
	bool work;
	struct work_struct pq_work;
	struct de_output_info output_info;
	unsigned int kHZ_pixelclk;
	unsigned int htotal;
	unsigned int vtotal;
	unsigned int pixel_mode;

	unsigned int vsync_count;
	unsigned int last_rcq_vsync;
};

struct sunxi_display_engine {
	struct device *dev;
	struct device *disp_sys;
	const struct de_match_data *match_data;
	void __iomem *reg_base;
	struct clk *mclk;
	struct clk *mclk_bus;
	struct clk *mclk_ahb;
	struct clk *ahb_vid_out; /* PPU clk */
	struct clk *mbus_vo_sys; /* PPU clk */
	struct reset_control *rst_bus_de;
	struct reset_control *rst_bus_de_sys;
	int irq_no;
	int crc_irq_no;
	unsigned long clk_freq;
	bool sw_enable;

	struct sunxi_de_wb wb;
	struct de_top_handle *top_hdl;
	unsigned int chn_cfg_mode;
	unsigned char display_out_cnt;
	struct sunxi_de_out *display_out;
	struct de_reg_buffer reg;
	bool de_devfreq_auto;
	struct sunxi_de_debug *debug;
};

static void de_process_late_work_next_frame(struct sunxi_de_out *hwde);
struct sunxi_drm_crtc *sunxi_drm_crtc_init_one(struct sunxi_de_info *info);
void sunxi_drm_crtc_destory(struct sunxi_drm_crtc *scrtc);
struct sunxi_drm_wb *sunxi_drm_wb_init_one(struct sunxi_de_wb_info *wb_info);
void sunxi_drm_wb_destory(struct sunxi_drm_wb *wb);
int sunxi_tcon_top_clk_enable(struct device *tcon_top);
int sunxi_tcon_top_clk_disable(struct device *tcon_top);


static inline bool check_update_finished(struct sunxi_de_out *hwde)
{
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	return de_top_check_display_update_finish_with_clear(engine->top_hdl, hwde->id);
}

//test rcq fifo for a537
static int __maybe_unused test_rcq_fifo_create(struct sunxi_display_engine *de, int fifo_id)
{
	struct sunxi_de_out *disp = &de->display_out[0];
	struct de_rcq_head *rcq_hd = NULL;
	struct de_rcq_mem_info *rcq_info = fifo_id ? &disp->rcq_info_test1 : &disp->rcq_info_test0;
	unsigned int block_cnt = 2;
	static struct de_reg_block block0[2];
	static struct de_reg_block block1[2];
	struct de_reg_block *block = fifo_id ? block1 : block0;
	void *tmp;
	volatile u32 *reg;
	unsigned long ioaddr;

	tmp = sunxi_de_reg_buffer_alloc(de, 32, (void *)&ioaddr, 1);

	block[0].phy_addr = (void *)ioaddr;
	block[0].vir_addr = tmp;
	block[0].size = 8;//byte  2reg
	block[0].reg_addr = (void *)0x101000;//vch0

	block[1].phy_addr = (void *)(ioaddr + 16);
	block[1].vir_addr = (tmp + 16);
	block[1].size = 8;//byte  2reg
	block[1].reg_addr = (void *)0x1010c0;//vch0 fill color

	reg = tmp;
	reg[0] = 0xff008115;
	reg[1] = 0x00ef013f;
	reg[2] = fifo_id ? 0xffff0000 : 0xff0000ff;
	reg[3] = 0;
	reg[4] = fifo_id ? 0xffff0000 : 0xff0000ff;
	reg[5] = 0;

	rcq_info->reg_blk = kmalloc(sizeof(*(rcq_info->reg_blk)) * block_cnt,
					GFP_KERNEL | __GFP_ZERO);
	if (rcq_info->reg_blk == NULL) {
		DRM_ERROR("kalloc for de_reg_block failed\n");
		return -1;
	}
	rcq_info->reg_blk[0] = &block[0];
	rcq_info->reg_blk[1] = &block[1];


	rcq_info->vir_addr = sunxi_de_reg_buffer_alloc(de,
		2 * sizeof(*(rcq_info->vir_addr)),
		(dma_addr_t *)&(rcq_info->phy_addr), 1);
	rcq_info->block_num = 2;
	rcq_info->block_num_aligned = 2;

	rcq_hd = rcq_info->vir_addr;

	rcq_hd->low_addr = (u32)((uintptr_t)(block[0].phy_addr));
	rcq_hd->dw0.bits.high_addr = 0;
	rcq_hd->dw0.bits.len = 8;//byte  2reg
	rcq_hd->dirty.dwval = 1;
	rcq_hd->reg_offset = (unsigned int)(unsigned long)block[0].reg_addr;

	rcq_hd++;

	rcq_hd->low_addr = (u32)((uintptr_t)(block[1].phy_addr));
	rcq_hd->dw0.bits.high_addr = 0;
	rcq_hd->dw0.bits.len = 8;//byte  2reg
	rcq_hd->dirty.dwval = 1;
	rcq_hd->reg_offset = (unsigned int)(unsigned long)block[1].reg_addr;

	return 0;
}

static void __maybe_unused test_rcq_fifo(struct sunxi_display_engine *de)
{
	struct sunxi_de_out *disp = &de->display_out[0];
	struct de_rcq_mem_info *rcq_info0 = &disp->rcq_info_test0;
	struct de_rcq_mem_info *rcq_info1 = &disp->rcq_info_test1;
	printk("s\n");
	de_top_request_rcq_fifo_update(de->top_hdl, 0, (unsigned long)(rcq_info1->phy_addr), 16 * 2);
	de_top_request_rcq_fifo_update(de->top_hdl, 0, (unsigned long)(rcq_info0->phy_addr), 16 * 2);
	printk("e\n");
}

static int de_rtmx_init_rcq(struct sunxi_display_engine *de, unsigned int id)
{
	struct sunxi_de_out *disp = &de->display_out[id];
	struct de_rcq_mem_info *rcq_info = &disp->rcq_info;
	unsigned int block_cnt = 0;
	unsigned int cur_cnt = 0;
	struct de_top_handle *detop_hdl = de->top_hdl;
	struct de_channel_handle *ch_hdl;
	struct de_wb_handle *wb_hdl = de->wb.wb_hdl;
	struct de_bld_handle *bld_hdl = disp->bld_hdl;
	struct de_backend_handle *backend_hdl = disp->backend_hdl;
	struct de_fmt_handle *fmt_hdl = disp->fmt_hdl;
	struct de_reg_block **p_reg_blks;
	struct de_rcq_head *rcq_hd = NULL;
	int i, reg_blk_num;

	/* cal block cnt and malloc */
	block_cnt += bld_hdl->block_num;

	if (detop_hdl) {
		block_cnt += detop_hdl->block_num;
	}

	if (backend_hdl)
		block_cnt += backend_hdl->block_num;

	if (fmt_hdl)
		block_cnt += fmt_hdl->block_num;

	if (wb_hdl)
		block_cnt += wb_hdl->block_num;

	for (i = 0; i < disp->ch_cnt; i++) {
		ch_hdl = disp->ch_hdl[i];
		block_cnt += ch_hdl->block_num;
	}

	rcq_info->reg_blk = kmalloc(sizeof(*(rcq_info->reg_blk)) * block_cnt,
					GFP_KERNEL | __GFP_ZERO);
	if (rcq_info->reg_blk == NULL) {
		DRM_ERROR("kalloc for de_reg_block failed\n");
		return -1;
	}

	/* copy block */
	memcpy(&rcq_info->reg_blk[cur_cnt], bld_hdl->block,
		    sizeof(bld_hdl->block[0]) * bld_hdl->block_num);
	cur_cnt += bld_hdl->block_num;

	if (detop_hdl) {
		memcpy(&rcq_info->reg_blk[cur_cnt], detop_hdl->block, sizeof(detop_hdl->block[0]) * detop_hdl->block_num);
		cur_cnt += detop_hdl->block_num;
	}
	if (backend_hdl) {
		memcpy(&rcq_info->reg_blk[cur_cnt], backend_hdl->block,
			    sizeof(backend_hdl->block[0]) * backend_hdl->block_num);
		cur_cnt += backend_hdl->block_num;
	}

	if (fmt_hdl) {
		memcpy(&rcq_info->reg_blk[cur_cnt], fmt_hdl->block,
		       sizeof(fmt_hdl->block[0]) * fmt_hdl->block_num);
		cur_cnt += fmt_hdl->block_num;
	}

	if (wb_hdl) {
		memcpy(&rcq_info->reg_blk[cur_cnt], wb_hdl->block,
			    sizeof(wb_hdl->block[0]) * wb_hdl->block_num);
		cur_cnt += wb_hdl->block_num;
	}

	for (i = 0; i < disp->ch_cnt; i++) {
		ch_hdl = disp->ch_hdl[i];
		memcpy(&rcq_info->reg_blk[cur_cnt], ch_hdl->block,
			    sizeof(ch_hdl->block[0]) * ch_hdl->block_num);
		cur_cnt += ch_hdl->block_num;
	}

	/* setup rcq head based on blocks */
	rcq_info->block_num = block_cnt;
	if (de->match_data->update_mode == RCQ_MODE) {
		/* RCQ header block cnt must be 2 align */
		rcq_info->block_num_aligned = ALIGN(rcq_info->block_num, 2);
		rcq_info->vir_addr = sunxi_de_reg_buffer_alloc(de,
			rcq_info->block_num_aligned * sizeof(*(rcq_info->vir_addr)),
			(dma_addr_t *)&(rcq_info->phy_addr), 1);
		if (rcq_info->vir_addr == NULL) {
			DRM_ERROR("alloc for de_rcq_head failed\n");
			return -1;
		}
		p_reg_blks  = rcq_info->reg_blk;
		rcq_hd = rcq_info->vir_addr;
		for (reg_blk_num = 0; reg_blk_num < rcq_info->block_num;
			++reg_blk_num) {
			struct de_reg_block *reg_blk = *p_reg_blks;

			rcq_hd->low_addr = (u32)((uintptr_t)(reg_blk->phy_addr));
			rcq_hd->dw0.bits.high_addr =
				(u8)((uintptr_t)(reg_blk->phy_addr) >> 32);
			rcq_hd->dw0.bits.len = reg_blk->size;
			rcq_hd->dirty.dwval = reg_blk->dirty;
			rcq_hd->reg_offset = (u32)(uintptr_t)
				(reg_blk->reg_addr - (u8 *)(de->reg_base));
			reg_blk->rcq_hd = rcq_hd;
			//printk("%s header addr 0x%x, len %d , reg offset 0x%x dirty %d hd %lx\n",__func__,  rcq_hd->low_addr, rcq_hd->dw0.bits.len, rcq_hd->reg_offset, rcq_hd->dirty.dwval,(unsigned long)rcq_hd);
			rcq_hd++;
			p_reg_blks++;
		}
		if (rcq_info->block_num_aligned > rcq_info->block_num) {
			rcq_hd = rcq_info->vir_addr + rcq_info->block_num_aligned;
			rcq_hd->dirty.dwval = 0;
		}
/*		if (id == 0) {
			test_rcq_fifo_create(de, 0);
			test_rcq_fifo_create(de, 1);
		}*/
	} else {
		rcq_hd = NULL;
	}
	return 0;
}

/*  request a disp to update wb reg by set rcq head,
 *    this make wb_set_block_dirty() only set the request disp's rcq head dirty
 */
static int wb_rcq_head_switch(struct sunxi_de_out *hwde)
{
	int i;
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	struct de_rcq_mem_info *rcq_info = &hwde->rcq_info;
	struct de_rcq_head *hd = rcq_info->vir_addr;
	struct de_rcq_head *hd_end = NULL;
	unsigned int wb_block_num = engine->wb.wb_hdl->block_num;
	struct de_reg_block *reg_blk_wb = engine->wb.wb_hdl->block[0];
	u64 reg_addr = (u64)reg_blk_wb->reg_addr - (u64)engine->reg_base;

	if (hd == NULL) {
		DRM_ERROR("rcq head is null\n");
		return -1;
	}

	hd_end = hd + rcq_info->block_num;
	/* find wb block by reg_addr, switch rcq head */
	for (; hd != hd_end && wb_block_num; hd++) {
		if (hd->reg_offset == (u32)((uintptr_t)(reg_addr))) {
			for (i = 0; i < wb_block_num; i++) {
				reg_blk_wb[i].rcq_hd = &hd[i];
				//printk("%s head 0x%lx block %lx reg_offset%lx\n",__func__, (unsigned long)(&hd[i]), (unsigned long)&reg_blk_wb[i], (unsigned long)(hd[i].reg_offset));
			}
			return 0;
		}
	}

	DRM_ERROR("found wb rcq block fail\n");
	return -1;
}

//TODO
/*static*/ int de_rtmx_exit_rcq(struct sunxi_display_engine *de)
{
/*
	struct de_rtmx_context *ctx = de_rtmx_get_context(disp);
	struct de_rcq_mem_info *rcq_info = &ctx->rcq_info;

	if (rcq_info->reg_blk)
		kfree(rcq_info->reg_blk);
	if (rcq_info->vir_addr)
		de_top_reg_memory_free(rcq_info->vir_addr, rcq_info->phy_addr,
			rcq_info->block_num_aligned * sizeof(*(rcq_info->vir_addr)));
*/
	return 0;
}

static int de_rtmx_set_all_reg_dirty(struct sunxi_de_out *hwde, u32 dirty)
{
	struct de_rcq_mem_info *rcq_info = &hwde->rcq_info;
	struct de_reg_block **p_reg_blk = rcq_info->reg_blk;
	struct de_reg_block **p_reg_blk_end =
		p_reg_blk + rcq_info->block_num;

	for (; p_reg_blk != p_reg_blk_end; ++p_reg_blk) {
		struct de_reg_block *reg_blk = *p_reg_blk;
		reg_blk->dirty = dirty;
		if (reg_blk->rcq_hd)
			reg_blk->rcq_hd->dirty.dwval = dirty;
	}

	return 0;
}

static int __maybe_unused de_rtmx_check_rcq_head_dirty(struct sunxi_de_out *hwde)
{
	struct de_rcq_mem_info *rcq_info = &hwde->rcq_info;
	struct de_rcq_head *hd = rcq_info->vir_addr;
	struct de_rcq_head *hd_end = NULL;

	if (hd == NULL) {
		DRM_ERROR("rcq head is null\n");
		return -1;
	}

	hd_end = hd + rcq_info->block_num;

	for (; hd != hd_end; hd++) {
		printk("%s header addr 0x%x, len %d , reg offset 0x%x dirty %d hd %lx\n",
			__func__, hd->low_addr, hd->dw0.bits.len, hd->reg_offset, hd->dirty.dwval, (unsigned long)hd);
	}
	return 0;
}

static void __maybe_unused de_memcpy_toio(volatile void __iomem *to, const void *from, size_t count)
{
	WARN_ON(count % 4);
	while (count >= 4) {
		writel(*(u32 *)from, to);
		from += 4;
		to += 4;
		count -= 4;
	}
}

/* for debug */
static int __maybe_unused de_update_ahb(struct sunxi_de_out *hwde)
{
	struct de_rcq_mem_info *rcq_info = &hwde->rcq_info;

	struct de_reg_block **p_reg_blk = rcq_info->reg_blk;
	struct de_reg_block **p_reg_blk_end =
		p_reg_blk + rcq_info->block_num;

	for (; p_reg_blk != p_reg_blk_end; ++p_reg_blk) {
		struct de_reg_block *reg_blk = *p_reg_blk;
		if (reg_blk->dirty || (reg_blk->rcq_hd && reg_blk->rcq_hd->dirty.dwval)) {
			de_memcpy_toio((void *)reg_blk->reg_addr,
				(void *)reg_blk->vir_addr, reg_blk->size);
		}
	}
	return 0;
}

void *sunxi_de_dma_alloc_coherent(struct sunxi_display_engine *de, u32 size, dma_addr_t *phy_addr)
{
	return dma_alloc_coherent(de->dev, size, phy_addr, GFP_KERNEL);
}

void sunxi_de_dma_free_coherent(struct sunxi_display_engine *de, u32 size, dma_addr_t phy_addr, void *virt_addr)
{
	return dma_free_coherent(de->dev, size, virt_addr, phy_addr);
}

void *sunxi_de_reg_buffer_alloc(struct sunxi_display_engine *de, u32 size, dma_addr_t *phy_addr, u32 rcq_used)
{
	unsigned long use_byte = de->reg.used_byte;
	unsigned long phys = de->reg.phys_addr;
	unsigned long virt = de->reg.virt_addr;
	unsigned long add = ALIGN(size, 32);

	if (de->match_data->update_mode == RCQ_MODE) {
		if (use_byte + add > DE_BLOCK_SIZE) {
			DRM_ERROR("%s fail, malloc %ld byte fail\n", __func__, add);
			virt = 0;
			phys = 0;
		} else {
			de->reg.used_byte += add;
			de->reg.phys_addr += add;
			de->reg.virt_addr += add;
		}
	} else {
		phys = 0;
		virt = (unsigned long)kmalloc(size, GFP_KERNEL | __GFP_ZERO);
	}

	*phy_addr = (dma_addr_t)phys;
	return (void *)virt;
}

void sunxi_de_reg_buffer_free(struct sunxi_display_engine *de,
	void *virt_addr, void *phys_addr, u32 num_bytes)
{
	if (de->match_data->update_mode != RCQ_MODE) {
		kfree(virt_addr);
	}
}

static int sunxi_de_reg_mem_init(struct sunxi_display_engine *de)
{
	if (de->match_data->update_mode == RCQ_MODE) {
		de->reg.virt_addr = (unsigned long)dma_alloc_coherent(de->dev, DE_BLOCK_SIZE, (dma_addr_t *)(&de->reg.phys_addr), GFP_KERNEL);
		de->reg.used_byte = 0;
		return de->reg.virt_addr ? 0 : -ENOMEM;
	}
	return 0;
}

void sunxi_de_reg_mem_deinit(struct sunxi_display_engine *de)
{
	if (de->match_data->update_mode == RCQ_MODE && de->reg.virt_addr) {
		dma_free_coherent(de->dev, DE_BLOCK_SIZE, (void *)(de->reg.virt_addr - de->reg.used_byte),
			    de->reg.phys_addr - de->reg.used_byte);
	}
}

int sunxi_de_backend_get_pqd_config(struct sunxi_de_out *hwde, struct de_backend_data *data)
{
	if (hwde->backend_hdl) {
		return de_backend_get_pqd_config(hwde->backend_hdl, data);
	}
	return -ENODEV;
}

void sunxi_de_atomic_begin(struct sunxi_de_out *hwde)
{
//	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
//	de_top_set_rcq_update(engine->top_hdl, hwde->id, 0);
}

static void sunxi_de_update_regs(struct sunxi_de_out *hwde)
{
	u32 i = 0;
	struct de_channel_handle *ch;

	for (i = 0; i < hwde->ch_cnt; i++) {
		ch = hwde->ch_hdl[i];
		channel_update_regs(ch);
	}

	//bld_update_regs(hwde->bld_hdl);
}

static int sunxi_de_exconfig_check_and_update(struct sunxi_de_out *hwde, struct de_backend_data *data, struct sunxi_de_flush_cfg *cfg)
{
	bool dirty = false;
	struct de_backend_apply_cfg bcfg;
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	struct de_output_info *output_info = &hwde->output_info;

	memset(&bcfg, 0, sizeof(bcfg));
	bcfg.w = hwde->output_info.width;
	bcfg.h = hwde->output_info.height;

	if (data)
		dirty = true;

	if (hwde->backend_hdl) {
		if (cfg->gamma_dirty) {
			bcfg.gamma_lut = cfg->gamma_lut;
			bcfg.gamma_dirty = cfg->gamma_dirty;
			dirty = true;
		}
		if (cfg->ctm_dirty) {
			bcfg.ctm = cfg->ctm;
			bcfg.ctm_dirty = cfg->ctm_dirty;
			dirty = true;
		}
		if (cfg->bcsh_dirty) {
			bcfg.in_csc.px_fmt_space = engine->match_data->blending_in_rgb ? DE_FORMAT_SPACE_RGB : hwde->output_info.px_fmt_space;
			bcfg.in_csc.color_space = hwde->output_info.color_space;
			bcfg.in_csc.color_range = hwde->output_info.color_range;
			bcfg.in_csc.eotf = hwde->output_info.eotf;
			bcfg.in_csc.width = bcfg.w;
			bcfg.in_csc.height = bcfg.w;

			bcfg.out_csc.px_fmt_space = hwde->output_info.px_fmt_space;
			bcfg.out_csc.color_space = hwde->output_info.color_space;
			bcfg.out_csc.color_range = hwde->output_info.color_range;
			bcfg.out_csc.eotf = hwde->output_info.eotf;
			bcfg.out_csc.width = bcfg.w;
			bcfg.out_csc.height = bcfg.w;

			bcfg.bits = output_info->data_bits;

			bcfg.brightness = cfg->brightness;
			bcfg.contrast = cfg->contrast;
			bcfg.saturation = cfg->saturation;
			bcfg.hue = cfg->hue;
			bcfg.csc_dirty = cfg->bcsh_dirty;
			dirty = true;
		}
	}
	if (dirty)
		de_backend_apply(hwde->backend_hdl, data, &bcfg);
	sunxi_de_update_regs(hwde);
	return 0;
}

int sunxi_de_event_proc(struct sunxi_de_out *hwde, bool timeout)
{
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	bool use_double_buffer = engine->match_data->update_mode == DOUBLE_BUFFER_MODE;
	bool use_ahb = engine->match_data->update_mode == AHB_MODE;
	bool pending = atomic_read(&hwde->update_finish) == DE_UPDATE_PENDING;

	hwde->vsync_count++;

	if (pending && (use_double_buffer || (use_ahb && !timeout))) {
		if (use_ahb && !timeout)
			de_update_ahb(hwde);
		atomic_set(&hwde->update_finish, DE_UPDATE_FINISHED);
	}
	return 0;
}

static enum hrtimer_restart timer_handler_rcq_update(struct hrtimer *timer)
{
	/* note: irq context */
	struct sunxi_de_out *de_out = container_of(timer, struct sunxi_de_out, rcq_timer);
	struct sunxi_display_engine *engine = dev_get_drvdata(de_out->dev);
	de_top_set_rcq_update(engine->top_hdl, de_out->id, 1);
	//printk("update line %d\n", sunxi_drm_crtc_get_output_current_line(de_out->scrtc));
	return HRTIMER_NORESTART;
}

static int rcq_update_timer_start(struct sunxi_de_out *de_out)
{
	struct sunxi_display_engine *engine = dev_get_drvdata(de_out->dev);
	u32 vt = de_out->vtotal;
	u64 ns_per_line = (u64)(de_out->htotal) * 1000000;
	u32 cur_line;
	u32 min_line = vt / 4;
	u32 max_line = vt / 4 * 3;
	u64 delay_ns = 0;

	SUNXIDRM_TRACE_BEGIN(__func__);
	do_div(ns_per_line, de_out->kHZ_pixelclk);
	cur_line = sunxi_drm_crtc_get_output_current_line(de_out->scrtc);
	if (cur_line < min_line) {
		delay_ns = (min_line - cur_line) * ns_per_line;
	} else if (cur_line > max_line) {
		delay_ns = (vt - cur_line + min_line) * ns_per_line;
	}

	if (delay_ns) {
		hrtimer_start(&de_out->rcq_timer, ktime_set(0, delay_ns), HRTIMER_MODE_REL);
	} else {
		de_top_set_rcq_update(engine->top_hdl, de_out->id, 1);
	}
	SUNXIDRM_TRACE_END(__func__);
	return 0;
}

void sunxi_de_atomic_flush(struct sunxi_de_out *hwde, struct de_backend_data *data, struct sunxi_de_flush_cfg *cfg)
{
	int disp = hwde->id;
	bool is_finished = false;
	bool timeout = true;
	bool queue = true;
	u32 cur_line;
	unsigned int retry_cnt = 0;
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	bool use_rcq = engine->match_data->update_mode == RCQ_MODE;
	bool use_double_buffer = engine->match_data->update_mode == DOUBLE_BUFFER_MODE;
	if (!hwde->enable) {
		DRM_INFO("%s de %d not enable, skip\n", __func__, disp);
		return;
	}

	/* update dep */
	if ((cfg || data) && (unsigned long)cfg != FORCE_ATOMIC_FLUSH)
		sunxi_de_exconfig_check_and_update(hwde, data, cfg);

	if ((unsigned long)cfg == FORCE_ATOMIC_FLUSH) {
		de_update_ahb(hwde);
		if (use_double_buffer)
			de_top_set_double_buffer_ready(engine->top_hdl, hwde->id);
		return;
	}

	/* request update */
	if (use_rcq) {
		//make sure clear finish flag before enable rcq
		SUNXIDRM_TRACE_BEGIN("check_update_finished");
		check_update_finished(hwde);
		SUNXIDRM_TRACE_END(" ");

		if (engine->de_devfreq_auto)
			sunxi_de_auto_calc_freq_and_apply(engine->display_out);
		de_top_update_force_by_ahb(engine->top_hdl);

		if (engine->match_data->rcq_wait_line)
			rcq_update_timer_start(hwde);
		else
			de_top_set_rcq_update(engine->top_hdl, hwde->id, 1);

		sunxi_drm_crtc_prepare_vblank_event(hwde->scrtc);

		// record the rcq request at which vsync count
		hwde->last_rcq_vsync = hwde->vsync_count;

	} else if (use_double_buffer) {
		de_update_ahb(hwde);
		de_top_set_double_buffer_ready(engine->top_hdl, hwde->id);
	}

	/* block until finish except calling from crash dump in interrupt */
	if (in_task()) {
		if (use_rcq) {
			SUNXIDRM_TRACE_BEGIN("wait_rcq_finish");
			timeout = read_poll_timeout(check_update_finished, is_finished,
					  is_finished, 2000, 50000, false, hwde);
			SUNXIDRM_TRACE_END(" ");
		} else {
			atomic_set(&hwde->update_finish, DE_UPDATE_PENDING);
			while (1) {
				sunxi_drm_crtc_wait_one_vblank(hwde->scrtc);
				retry_cnt++;
				if (atomic_read(&hwde->update_finish) == DE_UPDATE_FINISHED) {
					timeout = false;
					break;
				}
				if (retry_cnt > 10) {
					timeout = true;
					break;
				}
			};
		}
	}

	/* update finish post proc */
	//de_rtmx_check_rcq_head_dirty(hwde);
	if (timeout)
		DRM_INFO("%s timeout\n", __func__);
	else {
		de_rtmx_set_all_reg_dirty(hwde, 0);
		if (hwde->work) {
			if (atomic_read(&hwde->schedule_next_frame)) {
				de_process_late_work_next_frame(hwde);
				atomic_set(&hwde->schedule_next_frame, false);
			}
			if (use_rcq && engine->match_data->rcq_skip_read) {
				cur_line = sunxi_drm_crtc_get_output_current_line(hwde->scrtc);
				if (cur_line < 20 || cur_line > 128)
					queue = false;
			}
			if (queue)
				queue_work(system_long_wq, &hwde->pq_work);
		}
	}
}

static int rtmx_start(struct sunxi_display_engine *engine, unsigned int id, unsigned int hwdev_index)
{
	int i;
	int port;
	struct de_top_display_cfg cfg;
	struct sunxi_de_out *hwde = &engine->display_out[id];
	struct de_channel_handle *ch;
	struct de_rcq_mem_info *rcq_info = &hwde->rcq_info;
	unsigned int w = hwde->output_info.width;
	unsigned int h = hwde->output_info.height;
	unsigned int pixel_mode = hwde->pixel_mode;
	bool use_rcq = engine->match_data->update_mode == RCQ_MODE;
	struct de_fmt_info info;
	struct dfs_cfg dfs_cfg;
	struct offline_cfg offline;

	memset(&cfg, 0, sizeof(cfg));
	cfg.display_id = id;
	cfg.enable = 1;
	cfg.pixel_mode = pixel_mode;
	cfg.w = w;
	cfg.h = h;
	cfg.interlaced = hwde->output_info.interlaced;
	cfg.device_index = hwdev_index;
	cfg.rcq_header_addr = use_rcq ? (unsigned long)rcq_info->phy_addr : 0;
	cfg.rcq_header_byte = use_rcq ? rcq_info->block_num_aligned * sizeof(*(rcq_info->vir_addr)) : 0;
	de_top_display_config(engine->top_hdl, &cfg);

	if (engine->debug->force_offline_mode)
		offline.enable = true;
	else
		offline.enable = false;
	offline.mode = ONE_FRAME_DELAY;
	offline.w = w;
	offline.h = h;
	de_top_offline_mode_config(engine->top_hdl, &offline);

	memset(&dfs_cfg, 0, sizeof(dfs_cfg));
	dfs_cfg.enable = true;
	dfs_cfg.display_id = id;
	dfs_cfg.de_clk = hwde->output_info.de_clk_freq;
	dfs_cfg.dclk = hwde->kHZ_pixelclk * 1000;
	de_top_dfs_config_enable(engine->top_hdl, &dfs_cfg);

	for (i = 0; i < hwde->ch_cnt; i++) {
		ch = hwde->ch_hdl[i];
		port = de_bld_get_chn_mux_port(engine->display_out[id].bld_hdl, engine->chn_cfg_mode,
						ch->is_video, ch->type_hw_id);
		if (port < 0) {
			/* should not happened */
			DRM_ERROR("bld config error\n");
			return -EINVAL;
		}
		port = de_top_set_chn_mux(engine->top_hdl, id, port, ch->type_hw_id, ch->is_video);
	}

	/* rtmx en & tcon en & bld disable (rcq copy needs to wait until the next plane update)
	 * which will cause tcon to think that de is enabled, but bld has no output, resulting in missing data
	 */
	de_bld_output_set_attr(hwde->bld_hdl, w, h,
			       hwde->output_info.px_fmt_space == DE_FORMAT_SPACE_RGB ? 0 : 1,
			       hwde->output_info.interlaced,
			       true);

	if (hwde->fmt_hdl) {
		info.px_fmt_space = hwde->output_info.px_fmt_space;
		info.yuv_sampling = hwde->output_info.yuv_sampling;
		info.bits         = hwde->output_info.data_bits;
		info.width        = w;
		info.height       = h;
		de_fmt_apply(hwde->fmt_hdl, &info);
	}
	return 0;
}

int sunxi_de_enable(struct sunxi_de_out *hwde,
		    const struct sunxi_de_out_cfg *cfg)
{
	int ret = 0;
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);

	DRM_INFO("%s index %d\n", __FUNCTION__, hwde->id);
	if (hwde->enable) {
		DRM_INFO("[SUNXI-DE]WARN:sunxi has been enable");
		return 0;
	}

	hwde->enable = true;

	if (engine->disp_sys)
		sunxi_tcon_top_clk_enable(engine->disp_sys);

	if (engine->rst_bus_de_sys) {
		ret = reset_control_deassert(engine->rst_bus_de_sys);
		if (ret) {
			DRM_ERROR("reset_control_deassert for rst_bus_de_sys failed\n\n");
			return -1;
		}
	}

	ret = reset_control_deassert(engine->rst_bus_de);
	if (ret) {
		DRM_ERROR("reset_control_deassert for rst_bus_de failed\n\n");
		return -1;
	}

	if (engine->ahb_vid_out) {
		ret = clk_prepare_enable(engine->ahb_vid_out);
		if (ret < 0) {
			DRM_ERROR("Enable de module ahb_vid_out clk failed\n");
			return -1;
		}
	}

	if (engine->mbus_vo_sys) {
		ret = clk_prepare_enable(engine->mbus_vo_sys);
		if (ret < 0) {
			DRM_ERROR("Enable de module mbus_vo_sys clk failed\n");
			return -1;
		}
	}

	if (engine->mclk_ahb) {
		ret = clk_prepare_enable(engine->mclk_ahb);
		if (ret < 0) {
			DRM_ERROR("Enable de module ahb clk failed\n");
			return -1;
		}
	}

	ret = clk_prepare_enable(engine->mclk_bus);
	if (ret < 0) {
		DRM_ERROR("Enable de module bus clk failed\n");
		return -1;
	}

	ret = clk_prepare_enable(engine->mclk);
	if (ret < 0) {
		DRM_ERROR("Enable de module clk failed\n\n");
		return -1;
	}

	pm_runtime_get_sync(hwde->dev);

	hwde->output_info.width = cfg->width;
	hwde->output_info.height = cfg->height;
	hwde->output_info.htotal = cfg->htotal;
	hwde->output_info.pclk_khz = cfg->kHZ_pixelclk;
	hwde->output_info.device_fps = cfg->device_fps;
	hwde->output_info.max_device_fps = cfg->max_device_fps;
	hwde->output_info.de_clk_freq = engine->clk_freq;
	hwde->output_info.interlaced = cfg->interlaced;
	hwde->output_info.px_fmt_space = cfg->px_fmt_space;
	hwde->output_info.yuv_sampling = cfg->yuv_sampling;
	hwde->output_info.eotf = cfg->eotf;
	hwde->output_info.color_space = cfg->color_space;
	hwde->output_info.color_range = cfg->color_range;
	hwde->output_info.data_bits = cfg->data_bits;
	hwde->kHZ_pixelclk = cfg->kHZ_pixelclk;
	hwde->htotal = cfg->htotal;
	hwde->vtotal = cfg->vtotal;
	hwde->pixel_mode = cfg->pixel_mode;
	#if defined(CONFIG_PM_DEVFREQ)
	if (sunxi_defreq_userspace_profile.freq_table) {
		sunxi_defreq_userspace_profile.freq_table[0] = cfg->kHZ_pixelclk * 1000;
	}
	#endif
	rtmx_start(engine, hwde->id, cfg->hwdev_index);
	DRM_INFO("%s finish sw en=%d\n", __FUNCTION__, cfg->sw_enable);
	return 0;
}

void sunxi_de_disable(struct sunxi_de_out *hwde)
{
	int id = hwde->id, i;
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	struct de_top_display_cfg cfg;

	DRM_INFO("%s de %d\n", __FUNCTION__, id);
	if (!hwde->enable) {
		DRM_INFO("[SUNXI-DE]WARN:sunxi has NOT been enable");
		return;
	}

	hwde->enable = false;

	de_backend_disable(hwde->backend_hdl);
	/* we need to disbale all bld pipe to avoid enable not used pipe after crtc enable */
	for (i = 0; i < CHANNEL_MAX; i++) {
		de_bld_pipe_reset(hwde->bld_hdl, i, -255);
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.display_id = id;
	cfg.enable = 0;
	/* pixel_mode: 0:1pixel 1:1pixel 2:2pixel 4:4pixel */
	cfg.pixel_mode = 0;
	de_top_display_config(engine->top_hdl, &cfg);

//TODO add more clk
	pm_runtime_put_sync(hwde->dev);

	clk_disable_unprepare(engine->mclk);
	clk_disable_unprepare(engine->mclk_bus);
	clk_disable_unprepare(engine->mclk_ahb);
	if (engine->mbus_vo_sys)
		clk_disable_unprepare(engine->mbus_vo_sys);
	if (engine->ahb_vid_out)
		clk_disable_unprepare(engine->ahb_vid_out);
	reset_control_assert(engine->rst_bus_de);
	reset_control_assert(engine->rst_bus_de_sys);

	if (engine->disp_sys)
		sunxi_tcon_top_clk_disable(engine->disp_sys);
}

static int sunxi_de_parse_dts(struct device *dev,
			      struct sunxi_display_engine *engine)
{
	struct device_node *node;
	struct platform_device *pdev = to_platform_device(dev);
	struct resource *res;

	node = dev->of_node;
	if (!node) {
		DRM_ERROR("get sunxi-de node err.\n ");
		return -EINVAL;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	engine->reg_base = devm_ioremap_resource(dev, res);

	if (!engine->reg_base) {
		DRM_ERROR("unable to map de registers\n");
		return -EINVAL;
	}

	engine->mclk = devm_clk_get(dev, "clk_de");
	if (IS_ERR(engine->mclk)) {
		DRM_ERROR("fail to get clk for de\n");
		return -EINVAL;
	}

	engine->mclk_bus = devm_clk_get(dev, "clk_bus_de");
	if (IS_ERR(engine->mclk_bus)) {
		DRM_ERROR("fail to get bus clk for de\n");
		return -EINVAL;
	}

	engine->mclk_ahb = devm_clk_get_optional(dev, "clk_ahb_de");
	if (IS_ERR(engine->mclk_ahb)) {
		DRM_ERROR("fail to get ahb clk for de\n");
		return -EINVAL;
	}
	engine->ahb_vid_out = devm_clk_get_optional(dev, "ahb_vid_out");
	if (IS_ERR(engine->ahb_vid_out)) {
		DRM_ERROR("fail to get ahb_vid_out clk for de\n");
		return -EINVAL;
	}
	engine->mbus_vo_sys = devm_clk_get_optional(dev, "mbus_vo_sys");
	if (IS_ERR(engine->mbus_vo_sys)) {
		DRM_ERROR("fail to get mbus_vo_sys clk for de\n");
		return -EINVAL;
	}

	engine->rst_bus_de = devm_reset_control_get_shared(dev, "rst_bus_de");
	if (IS_ERR(engine->rst_bus_de)) {
		DRM_ERROR("fail to get reset clk for rst_bus_de\n");
		return -EINVAL;
	}

	engine->rst_bus_de_sys = devm_reset_control_get_optional_shared(dev, "rst_bus_de_sys");
	if (IS_ERR(engine->rst_bus_de_sys)) {
		DRM_ERROR("fail to get reset clk for rst_bus_de_sys\n");
		return -EINVAL;
	}

	engine->irq_no = platform_get_irq(pdev, 0);
	if (engine->irq_no < 0) {
		DRM_ERROR("irq_of_parse_and_map de irq fail\n");
		return -EINVAL;
	}

	engine->crc_irq_no = platform_get_irq_optional(pdev, 1);
	if (engine->crc_irq_no < 0) {
		engine->crc_irq_no = 0;
	}

	if (of_property_read_u32(node, "chn_cfg_mode", &engine->chn_cfg_mode)) {
		DRM_INFO("[SUNXI-DE] chn_cfg_mode not found, used def val\n");
	}

	return 0;
}

static inline int get_de_output_display_cnt(struct device *dev)
{
	struct device_node *port;
	int i = 0;

	while (true) {
		port = of_graph_get_port_by_id(dev->of_node, i);
		if (!port)
			break;
		i++;
		of_node_put(port);
	};
	return i;
}

static irqreturn_t sunxi_de_crc_irq_event_proc(int irq, void *parg)
{
	struct sunxi_display_engine *engine = parg;
	struct sunxi_de_out *display_out;
	int i;
	u32 mask;
	for (i = 0; i < engine->display_out_cnt; i++) {
		display_out = &engine->display_out[i];
		if (display_out->backend_hdl && display_out->backend_hdl->feat.mod.module.crc)
			mask = de_backend_check_crc_status_with_clear(display_out->backend_hdl, 0xff);
	}
	DRM_INFO("crc irq mask %x\n", mask);
	return IRQ_HANDLED;
}

static int sunxi_de_request_irq(struct sunxi_display_engine *engine)
{
	int ret = -1;

	if (engine->crc_irq_no) {
		ret = devm_request_irq(engine->dev, engine->crc_irq_no,
				       sunxi_de_crc_irq_event_proc, 0, dev_name(engine->dev),
				       engine);
		if (ret) {
			DRM_ERROR("Couldn't request crc IRQ\n");
		}
	}
	return ret;
}

static int sunxi_display_engine_init(struct device *dev)
{
	int ret;
	struct sunxi_display_engine *engine;

	engine = devm_kzalloc(dev, sizeof(*engine), GFP_KERNEL);
	engine->dev = dev;
	engine->match_data = of_device_get_match_data(dev);
	engine->debug = devm_kzalloc(dev, sizeof(struct sunxi_de_debug), GFP_KERNEL);

	if (!engine->match_data) {
		DRM_ERROR("sunxi display engine fail to get match data\n");
		return -ENODEV;
	}
	dev_set_drvdata(dev, engine);

	ret = sunxi_de_parse_dts(dev, engine);
	if (ret < 0) {
		DRM_ERROR("Parse de dts failed!\n");
		goto de_err;
	}
	engine->de_devfreq_auto = true;
	engine->clk_freq = clk_get_rate(engine->mclk);
	engine->display_out_cnt = get_de_output_display_cnt(dev);
	if ((engine->display_out_cnt <= 0)) {
		DRM_ERROR("get wrong display_out_cnt");
		goto de_err;
	}
	sunxi_de_request_irq(engine);
	return 0;
de_err:
	DRM_ERROR("%s FAILED\n", __FUNCTION__);
	return -EINVAL;
}

static int sunxi_display_engine_exit(struct device *dev)
{
//	sunxi_de_v35x_al_exit(dev);
	return 0;
}

#if IS_ENABLED(CONFIG_PROC_FS)
#define DE_TOP_PROCFS(name) \
static const struct proc_ops de_top_##name##_proc_ops = { \
	.proc_open      = de_top_##name##_proc_open, \
	.proc_read      = seq_read, \
	.proc_lseek     = seq_lseek, \
	.proc_release   = single_release, \
	.proc_write     = de_top_##name##_proc_write, \
};

static ssize_t de_top_offline_mode_proc_write(struct file *file,
			const char __user *buffer, size_t count, loff_t *ppos)
{
	char enable[128];
	struct inode *inode = file_inode(file);
	struct sunxi_display_engine *engine = inode->i_private;
	struct sunxi_de_out *hwde = &engine->display_out[0];
	unsigned int w = hwde->output_info.width;
	unsigned int h = hwde->output_info.height;
	struct offline_cfg offline;

	if (count >= sizeof(enable))
		count = sizeof(enable) - 1;

	if (copy_from_user(enable, buffer, count))
		return -EFAULT;

	if (strncmp(enable, "on", 2) == 0) {
		offline.enable = true;
		engine->debug->force_offline_mode = true;
	} else if (strncmp(enable, "off", 3) == 0) {
		offline.enable = false;
		engine->debug->force_offline_mode = false;
	} else {
		DRM_ERROR("invalid param, select from: \"on\"/\"off\" !\n");
		return count;
	}

	offline.mode = CURRENT_FRAME;
	offline.w = w;
	offline.h = h;
	de_top_offline_mode_config(engine->top_hdl, &offline);


	return count;
}


static int de_top_offline_mode_show(struct seq_file *m, void *v)
{
	struct sunxi_display_engine *engine = (struct sunxi_display_engine *)m->private;
	s32 offline_mode_status = -1;

	if (engine->top_hdl) {
		offline_mode_status = de_top_get_offline_mode_status(engine->top_hdl);
		seq_printf(m, "%s|%s|%s\n",
			   (offline_mode_status == 1) ? "[on]" : "on",
			   (offline_mode_status == 0) ? "[off]" : "off",
			   (offline_mode_status == -1) ? "[not_support]" : "not_support");
	} else
		seq_printf(m, "on|off|[not_support]\n");

	return 0;
}


static int de_top_offline_mode_proc_open(struct inode *inode, struct file *file)
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
	return single_open(file, de_top_offline_mode_show, PDE_DATA(inode));
#else
	return single_open(file, de_top_offline_mode_show, pde_data(inode));
#endif
}


DE_TOP_PROCFS(offline_mode)
#endif

static int sunxi_de_bind(struct device *dev, struct device *master, void *data)
{
	int i, j, ret;
	struct drm_device *drm = data;
	struct sunxi_display_engine *engine = dev_get_drvdata(dev);
	struct sunxi_de_out *display_out;
	struct sunxi_de_info info;
	struct sunxi_de_wb_info wb_info;
	struct proc_dir_entry *procfs_parent;

	DRM_INFO("[SUNXI-DE] %s start\n", __FUNCTION__);

	for (i = 0; i < engine->display_out_cnt; i++) {
		display_out = &engine->display_out[i];
		engine->sw_enable |= sunxi_drm_check_de_boot_enabled(drm, display_out->port_id);
	}
	/* sw_enable means the device is enabled in uboot, so set pm runtime to active */
	if (engine->sw_enable) {
		pm_runtime_set_active(dev);
		pm_runtime_enable(dev);
	} else {
		pm_runtime_enable(dev);
	}

	if (of_find_property(dev->of_node, "iommus", NULL)) {
		ret = of_dma_configure(drm->dev, dev->of_node, true);
		if (ret) {
			DRM_ERROR("of_dma_configure fail\n");
			return ret;
		}
	}

	//create crtc
	for (i = 0; i < engine->display_out_cnt; i++) {
		display_out = &engine->display_out[i];
		memset(&info, 0, sizeof(info));
		info.drm = drm;
		info.de_out = display_out;
		info.port = display_out->port;
		info.hw_id = display_out->id;
		info.clk_freq = engine->clk_freq;
		memcpy(&info.feat.support, &display_out->backend_hdl->feat.mod,
		       sizeof(display_out->backend_hdl->feat.mod));
		info.feat.hw_id = display_out->id;
		info.feat.feat.share_scaler = engine->top_hdl->share_scaler;
		info.gamma_lut_len = display_out->backend_hdl->feat.gamma_lut_len;
		info.hue_default_value = display_out->backend_hdl->feat.hue_default_value;

		info.plane_cnt = display_out->ch_cnt;
		info.planes = devm_kzalloc(dev, sizeof(*info.planes) * display_out->ch_cnt, GFP_KERNEL);
		for (j = 0; j < display_out->ch_cnt; j++) {
			info.planes[j].name = display_out->ch_hdl[j]->name;
			info.planes[j].is_primary = j == 0; /* use the first channel as primary plane */
			info.planes[j].afbc_rot_support = display_out->ch_hdl[j]->afbc_rot_support;
			info.planes[j].index = j;
			info.planes[j].hdl = display_out->ch_hdl[j];
			info.planes[j].formats = display_out->ch_hdl[j]->formats;
			info.planes[j].format_count = display_out->ch_hdl[j]->format_count;
			info.planes[j].format_modifiers = display_out->ch_hdl[j]->format_modifiers_comb;
			info.planes[j].layer_cnt = display_out->ch_hdl[j]->layer_cnt;
		}
		display_out->scrtc = sunxi_drm_crtc_init_one(&info);
		DRM_INFO("%s crtc init for de %d %s\n", __FUNCTION__,
			 info.hw_id, !IS_ERR_OR_NULL(display_out->scrtc) ? "ok" : "fail");
	}

	ret = drm_vblank_init(drm, drm->mode_config.num_crtc);
	if (ret) {
		DRM_ERROR("failed to init vblank.\n");
		return ret;
	}

	//create drm wb
	wb_info.drm = drm;
	wb_info.support_disp_mask = (1 << engine->display_out_cnt) - 1;
	wb_info.wb = &engine->wb;
	engine->wb.drm_wb = sunxi_drm_wb_init_one(&wb_info);

#if IS_ENABLED(CONFIG_PROC_FS)
	procfs_parent = sunxi_drm_get_procfs_dir();
	if (procfs_parent != NULL) {
		if (engine->top_hdl) {
			engine->debug->procfs_de_top = proc_mkdir("de_top", procfs_parent);
			proc_create_data("offline_mode", 0664, engine->debug->procfs_de_top,
					&de_top_offline_mode_proc_ops, engine);
		}

	}
#endif


	return 0;
}

static void sunxi_de_unbind(struct device *dev, struct device *master,
			    void *data)
{
	int i;
	struct sunxi_display_engine *engine = dev_get_drvdata(dev);
	DRM_INFO("[SUNXI-DE] %s\n", __FUNCTION__);
	for (i = 0; i < engine->display_out_cnt; i++) {
		sunxi_drm_crtc_destory(engine->display_out[i].scrtc);
	}
	pm_runtime_disable(dev);

#if IS_ENABLED(CONFIG_PROC_FS)
	/* remove de_top debug */
	remove_proc_subtree("sunxi-drm/de_top", NULL);
#endif
}

static const struct component_ops sunxi_de_component_ops = {
	.bind = sunxi_de_bind,
	.unbind = sunxi_de_unbind,
};

static int sunxi_de_get_disp_sys(struct sunxi_display_engine *engine)
{
	struct device_node *sys_node =
		of_parse_phandle(engine->dev->of_node, "sys", 0);
	struct platform_device *pdev = sys_node ? of_find_device_by_node(sys_node) : NULL;
	if (!pdev) {
		DRM_INFO("de use no display sys\n");
		return 0;
	}
	engine->disp_sys = &pdev->dev;
	return 0;
}

static void de_process_late_work(struct work_struct *work)
{
	struct sunxi_de_out *hwde = container_of(work, struct sunxi_de_out, pq_work);
	struct de_backend_tasklet_state btstate = {0};
	struct de_channel_handle *ch;
	u32 i = 0;

	for (i = 0; i < hwde->ch_cnt; i++) {
		ch = hwde->ch_hdl[i];
		if (ch->routine_job)
			channel_process_late(ch);
	}

	if (hwde->backend_hdl->routine_job) {
		btstate.device_support_bk = sunxi_drm_crtc_is_support_backlight(hwde->scrtc);
		btstate.backlight = sunxi_drm_crtc_get_backlight(hwde->scrtc);
		de_backend_process_late(hwde->backend_hdl, &btstate);
	}

	/* wait for the config to take effect before executing */
	if (hwde->backend_hdl->vblank_work && btstate.schedule)
		atomic_set(&hwde->schedule_next_frame, true);
}

static void de_process_late_work_next_frame(struct sunxi_de_out *hwde)
{
	struct de_backend_tasklet_state btstate = {0};
	int backlight;

	if (hwde->backend_hdl->vblank_work) {
		btstate.device_support_bk = sunxi_drm_crtc_is_support_backlight(hwde->scrtc);
		de_backend_vblank_work(hwde->backend_hdl, &btstate);

		if (btstate.dimming_changed) {
			backlight = btstate.backlight_user_set * btstate.dimming / 256;
			sunxi_drm_crtc_set_backlight_value(hwde->scrtc, backlight);
		}
	}

}

void sunxi_de_dump_state(struct drm_printer *p, struct sunxi_de_out *hwde)
{
	drm_printf(p, "\t    vsync: %d last rcq at vsync: %d\n", hwde->vsync_count, hwde->last_rcq_vsync);
}

#if defined(CONFIG_PM_DEVFREQ)
static int sunxi_de_devfreq_target(struct device *dev, unsigned long *freq, u32 flags)
{
	int m, n;
	unsigned long rate_in_khz, de_clk_set_khz;
	struct sunxi_display_engine *engine;
	unsigned int kHZ_pixelclk;
	de_clk_set_khz = *freq / (unsigned long)1000;
	engine = dev_get_drvdata(dev);
	rate_in_khz = (unsigned long)engine->clk_freq / 1000;
	kHZ_pixelclk = engine->display_out->kHZ_pixelclk;
	if (de_clk_set_khz <= rate_in_khz) {
		if (de_clk_set_khz > kHZ_pixelclk) {
			sunxi_de_div_calc_mn(rate_in_khz, de_clk_set_khz, &m, &n);
			DRM_INFO("sunxi_de_devfreq_target %ld %d %d , hex %x %x\n", de_clk_set_khz, m, n, m, n);
			de_top_freq_div_apply(engine->top_hdl, m, n);
			return 0;
		} else {
			DRM_ERROR("error clk_set <= clock-frequency\n");
			return -1;
		}
	} else {
		DRM_ERROR("error clk_set > rate_in\n");
		return -1;
	}
	return -1;
}

static int sunxi_defreq_get_cur_freq(struct device *dev, unsigned long *freq)
{
	int m, n;
	struct clk *de_clk;
	unsigned long rate_in;
	struct sunxi_display_engine *engine;
	engine = dev_get_drvdata(dev);
	de_top_freq_div_get(engine->top_hdl, &m, &n);
	de_clk = devm_clk_get(dev, "clk_de");
	rate_in = clk_get_rate(de_clk);
	*freq = rate_in * (n + 1) / (m + 1);
	DRM_INFO("sunxi_defreq_get_cur_freq %ld %d %d %ld\n", rate_in, m, n, *freq);
	return 0;
}

static void sunxi_de_devfreq_exit(struct device *dev)
{
	dev_pm_opp_of_remove_table(dev);
	devm_kfree(dev, sunxi_defreq_userspace_profile.freq_table);
}

int sunxi_deauto_governor_event_handler(struct devfreq *devfreq, unsigned int event, void *data)
{
	struct sunxi_display_engine *engine = dev_get_drvdata(devfreq->dev.parent);
	switch (event) {
	case DEVFREQ_GOV_START:
		engine->de_devfreq_auto = true;
		DRM_INFO("de_devfreq_auto true\n");
		break;
	case DEVFREQ_GOV_STOP:
		engine->de_devfreq_auto = false;
		DRM_INFO("de_devfreq_auto false\n");
		break;
	default:
		break;
	}
	return 0;
}

static struct devfreq_governor sunxi_devfreq_deauto_governor = {
	.name = "sunxi_deauto",
	.event_handler = sunxi_deauto_governor_event_handler,
};
#endif

static int sunxi_de_probe(struct platform_device *pdev)
{
	int i, j, ret;
	struct sunxi_display_engine *engine;
	struct sunxi_de_out *display_out;
	struct de_channel_handle *ch_hdl;
	struct module_create_info cinfo;
	unsigned int err_create = 0;

	ret = sunxi_display_engine_init(&pdev->dev);
	if (ret)
		goto OUT;
	//create hw
	memset(&cinfo, 0, sizeof(cinfo));
	engine = dev_get_drvdata(&pdev->dev);
	sunxi_de_get_disp_sys(engine);

	cinfo.de_version = engine->match_data->version;
	cinfo.id = 0;
	cinfo.de_reg_base = engine->reg_base;
	cinfo.de = engine;
	cinfo.update_mode = engine->match_data->update_mode;

	sunxi_de_reg_mem_init(engine);
	engine->top_hdl = de_top_create(&cinfo);
	engine->wb.wb_hdl = de_wb_create(&cinfo);

	engine->display_out = devm_kzalloc(&pdev->dev, sizeof(*engine->display_out) * engine->display_out_cnt, GFP_KERNEL);
	for (i = 0; i < engine->display_out_cnt; i++) {
		display_out = &engine->display_out[i];
		display_out->id = i;
		display_out->dev = &pdev->dev;
		hrtimer_init(&display_out->rcq_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		display_out->rcq_timer.function = timer_handler_rcq_update;
		display_out->port = of_graph_get_port_by_id(pdev->dev.of_node, i);
		if (of_property_read_u32(display_out->port, "reg", &display_out->port_id))
			DRM_INFO("[SUNXI-DE] port reg not found\n");

		cinfo.id = display_out->id;
		display_out->bld_hdl = de_blender_create(&cinfo, engine->chn_cfg_mode);
		if (!display_out->bld_hdl) {
			if (engine->display_out_cnt - (++err_create) == 0) {
				ret = -EINVAL;
				DRM_ERROR("[SUNXI-DE] display_out_cnt %d, and can not create any bld\n", engine->display_out_cnt);
				goto EXIT;
			}
			continue;
		}
		cinfo.reg_offset = display_out->bld_hdl->disp_reg_base;
		display_out->backend_hdl = de_backend_create(&cinfo);
		display_out->fmt_hdl = de_fmt_create(&cinfo);
		if (display_out->backend_hdl->routine_job)
			display_out->work = true;
	}
	if (err_create) {
		DRM_INFO("[SUNXI-DE] display_out_cnt %d, create display_cnt(bld_cnt) %d\n",
			  engine->display_out_cnt, engine->display_out_cnt - err_create);
		engine->display_out_cnt -= err_create;
	}


	//create channel
	for (i = 0; ; i++) {
		cinfo.id = i;
		ch_hdl = de_channel_create(&cinfo);
		if (!ch_hdl)
			break;
		for (j = 0; j < engine->display_out_cnt; j++) {
			display_out = &engine->display_out[j];
			if (de_bld_get_chn_mux_port(display_out->bld_hdl, engine->chn_cfg_mode,
						ch_hdl->is_video, ch_hdl->type_hw_id) >= 0) {
				display_out->ch_hdl[display_out->ch_cnt] = ch_hdl;
				display_out->ch_cnt++;
				if (ch_hdl->routine_job)
					display_out->work = true;
				break;
			}
		}
	}

	//setup rcq
	for (i = 0; i < engine->display_out_cnt; i++) {
		de_rtmx_init_rcq(engine, i);
		INIT_WORK(&engine->display_out[i].pq_work, de_process_late_work);

		engine->display_out[i].vsync_count = 0;
		engine->display_out[i].last_rcq_vsync = 0;
	}

	sunxidrm_debug_init(pdev);

	ret = component_add(&pdev->dev, &sunxi_de_component_ops);
	if (ret) {
		DRM_ERROR("failed to add component de\n");
		goto EXIT;
	}

	#if defined(CONFIG_PM_DEVFREQ)
	memset(&sunxi_defreq_userspace_profile, 0, sizeof(sunxi_defreq_userspace_profile));
	if (dev_pm_opp_of_add_table(&pdev->dev) == 0) {
		devfreq_add_governor(&sunxi_devfreq_deauto_governor);

		sunxi_defreq_userspace_profile.target = sunxi_de_devfreq_target;
		sunxi_defreq_userspace_profile.exit = sunxi_de_devfreq_exit;
		sunxi_defreq_userspace_profile.initial_freq = engine->clk_freq;
		sunxi_defreq_userspace_profile.get_cur_freq = sunxi_defreq_get_cur_freq;
		sunxi_defreq_userspace_profile.freq_table = devm_kcalloc(&pdev->dev, 2, sizeof(*sunxi_defreq_userspace_profile.freq_table), GFP_KERNEL);
		sunxi_defreq_userspace_profile.freq_table[0] = engine->display_out->kHZ_pixelclk;
		sunxi_defreq_userspace_profile.freq_table[1] = engine->clk_freq;
		sunxi_defreq_userspace_profile.max_state = 2;

		devm_devfreq_add_device(&pdev->dev, &sunxi_defreq_userspace_profile, "sunxi_deauto", NULL);
	} else {
		dev_pm_opp_of_remove_table(&pdev->dev);
	}
	#endif

	return ret;
EXIT:
	sunxi_display_engine_exit(&pdev->dev);
OUT:
	return ret;
}

static int sunxi_de_remove(struct platform_device *pdev)
{
	struct sunxi_display_engine *engine;
	engine = dev_get_drvdata(&pdev->dev);

	sunxidrm_debug_term();
	component_del(&pdev->dev, &sunxi_de_component_ops);
	sunxi_de_reg_mem_deinit(engine);
	sunxi_display_engine_exit(&pdev->dev);
	return 0;
}

static const struct de_match_data de350_data = {
	.version = 0x350,
	.update_mode = RCQ_MODE,
	.rcq_skip_read = true,
};

static const struct de_match_data de355_data = {
	.version = 0x355,
	.update_mode = RCQ_MODE,
};

static const struct de_match_data de352_data = {
	.version = 0x352,
	.update_mode = RCQ_MODE,
};

static const struct de_match_data de210_data = {
	.version = 0x210,
	.update_mode = RCQ_MODE,
	.rcq_wait_line = true,
};

static const struct de_match_data de201_data = {
	.version = 0x201,
	.update_mode = DOUBLE_BUFFER_MODE,
	.blending_in_rgb = true,
};

static const struct of_device_id sunxi_de_match[] = {
	{
		.compatible = "allwinner,display-engine-v350",
		.data = &de350_data,
	},
	{
		.compatible = "allwinner,display-engine-v355",
		.data = &de355_data,
	},
	{
		.compatible = "allwinner,display-engine-v352",
		.data = &de352_data,
	},
	{
		.compatible = "allwinner,display-engine-v210",
		.data = &de210_data,
	},
	{
		.compatible = "allwinner,display-engine-v201",
		.data = &de201_data,
	},
	{},
};

struct platform_driver sunxi_de_platform_driver = {
	.probe = sunxi_de_probe,
	.remove = sunxi_de_remove,
	.driver = {
		   .name = "sunxi-display-engine",
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_de_match,
	},
};

bool sunxi_de_format_mod_supported(struct sunxi_de_out *hwde, struct de_channel_handle *hdl,
					    uint32_t format, uint64_t modifier)
{
	return channel_format_mod_supported(hdl, format, modifier);
}

int sunxi_de_channel_update(struct sunxi_de_channel_update *info)
{
	struct sunxi_de_out *hwde = info->hwde;
	struct de_channel_handle *hdl = info->hdl;
	struct display_channel_state *new_state = info->new_state;
	struct display_channel_state *old_state = info->old_state;
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	unsigned int old_zorder = old_state->base.normalized_zpos;
	unsigned int new_zorder = new_state->base.normalized_zpos;
	struct de_channel_output_info channel_out;
	struct de_output_info *output_info = &hwde->output_info;
	unsigned int port_id;

	DRM_DEBUG_DRIVER("[SUNXI-DE] %s %s update\n", __FUNCTION__, hdl->name);
	/* fbdev channel always on, reserve zpos for fbdev channel*/
	if (info->fbdev_output) {
		old_zorder += info->is_fbdev ? 0 : 1;
		new_zorder += info->is_fbdev ? 0 : 1;
	}

	port_id = de_bld_get_chn_mux_port(hwde->bld_hdl, engine->chn_cfg_mode, hdl->is_video, hdl->type_hw_id);
	/* disable channel */
	if (new_state->base.fb == NULL) {
		channel_apply(hdl, new_state, output_info,  &channel_out, engine->match_data->blending_in_rgb);
		if (info->force)
			de_bld_pipe_reset(hwde->bld_hdl, port_id, -255);
		else
			de_bld_pipe_reset(hwde->bld_hdl, old_zorder, port_id);

		return 0;
	}

	channel_apply(hdl, new_state, output_info, &channel_out, engine->match_data->blending_in_rgb);

	/* disable not used pipe */
	if (old_zorder != new_zorder)
		de_bld_pipe_reset(hwde->bld_hdl, old_zorder, port_id);

	de_bld_pipe_set_attr(hwde->bld_hdl, new_zorder, port_id, &channel_out.disp_win, channel_out.is_premul);
	return 0;
}

void sunxi_de_dump_channel_state(struct drm_printer *p, struct sunxi_de_out *hwde, struct de_channel_handle *hdl,
				    const struct display_channel_state *state, bool state_only)
{
//	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	dump_channel_state(p, hdl, state, state_only);
	if (hdl == hwde->ch_hdl[hwde->ch_cnt - 1] && !state_only) {
		dump_bld_state(p, hwde->bld_hdl);
		de_backend_dump_state(p, hwde->backend_hdl);
		if (hwde->fmt_hdl)
			de_fmt_dump_state(p, hwde->fmt_hdl);
		drm_printf(p, "\n");
//		if (hwde->id == 0)
//			test_rcq_fifo(engine);
	}
}

int sunxi_de_write_back(struct sunxi_de_out *hwde, struct sunxi_de_wb *wb, struct drm_framebuffer *fb)
{
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	struct de_top_wb_cfg cfg;
	struct wb_in_config in_info;
	struct de_wb_handle *wb_hdl = wb->wb_hdl;

	if (!wb->wb_hdl) {
		DRM_ERROR("wb hdl null\n");
		return -ENODEV;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.disp =  hwde->id;
	cfg.enable = false;
	cfg.mode = TIMING_FROM_TCON;
	cfg.pos = FROM_BLENER;

	DRM_INFO("[SUNXI-DE] wb %s %d enter\n", __func__, hwde->id);
	if (engine->match_data->update_mode == RCQ_MODE)
		wb_rcq_head_switch(hwde);

	/*disable wb*/
	if (fb == NULL && engine->wb.wb_enable) {
		de_top_wb_config(engine->top_hdl, &cfg);
		de_wb_apply(wb_hdl, NULL, NULL);
		engine->wb.wb_enable = false;
		DRM_INFO("[SUNXI-DE] wb disable\n");
		return 0;
	}

	/*disable wb and enable again when cfg change */
	if (engine->wb.wb_enable && engine->wb.wb_disp != hwde->id) {
		de_top_wb_config(engine->top_hdl, &cfg);
		cfg.enable = true;
		de_top_wb_config(engine->top_hdl, &cfg);
	/* enable wb and config wb */
	} else if (!engine->wb.wb_enable) {
		cfg.enable = true;
		de_top_wb_config(engine->top_hdl, &cfg);
	}
	engine->wb.wb_enable = true;
	engine->wb.wb_disp = hwde->id;

	/* update wb config */
	in_info.width = hwde->output_info.width;
	in_info.height = hwde->output_info.height;
	in_info.csc_info.px_fmt_space = hwde->output_info.px_fmt_space;
	in_info.csc_info.color_space = hwde->output_info.color_space;
	in_info.csc_info.color_range = hwde->output_info.color_range;
	in_info.csc_info.eotf = hwde->output_info.eotf;
	return de_wb_apply(wb_hdl, &in_info, fb);
}

bool sunxi_de_query_de_busy(struct sunxi_de_out *hwde)
{
	struct sunxi_display_engine *engine = dev_get_drvdata(hwde->dev);
	return de_top_query_de_busy_state(engine->top_hdl, hwde->id);
}

int sunxi_de_div_calc_mn(unsigned long freq_in_kHZ, unsigned long freq_out_kHZ, unsigned int *m, unsigned int *n)
{
	unsigned int i, j;
	unsigned long diff_min, diff_tmp;
	diff_min = freq_in_kHZ;
	if (freq_out_kHZ < freq_in_kHZ) {
		*m = freq_in_kHZ / freq_out_kHZ;
		if (*m <= 16) {
			for (i = 0; i <= 15; i++) {
				for (j = 0; j <= i; j++) {
					if (freq_in_kHZ * (j + 1) / (i + 1) >= freq_out_kHZ) {
						diff_tmp = freq_in_kHZ * (j + 1) / (i + 1) - freq_out_kHZ;
						if (diff_tmp < diff_min) {
							diff_min = diff_tmp;
							*m = i;
							*n = j;
						}
						break;
					}
				}
			}
		} else {
			*m = 15;
			*n = 0;
		}
	} else {
		*m = 0;
		*n = 0;
	}
	return 0;
}

int sunxi_de_auto_calc_freq_and_apply(struct sunxi_de_out *hwde)
{
	struct sunxi_display_engine *engine;
	struct sunxi_de_out *display_out;
	unsigned long de_clk_tmp_khz, de_clk_ret_khz, c0, c1, c2, c3;
	unsigned int i, j, m, n, width, height, width_scn, height_scn;

	engine = dev_get_drvdata(hwde->dev);
	de_clk_ret_khz = 0;
	for (i = 0; i < engine->display_out_cnt; i++) {
		display_out = &engine->display_out[i];
		for (j = 0; j < display_out->ch_cnt; j++) {
			get_size_by_chn(display_out->ch_hdl[j], &width, &height);
			get_chn_size_on_scn(display_out->ch_hdl[j], &width_scn, &height_scn);
			c0 = c1 = 100;
			if (width > width_scn) {
				c0 = (unsigned long)width * (unsigned long)100 / (unsigned long)width_scn;
			}
			if (height > height_scn) {
				c1 = (unsigned long)height * (unsigned long)100 / (unsigned long)height_scn;
			}
			c2 = (unsigned long)display_out->kHZ_pixelclk * c0 * c1;
			c2 = c2 / (unsigned long)10000;
			if (c2 <= display_out->kHZ_pixelclk)
				c2 = display_out->kHZ_pixelclk;
			c3 = c2 * (unsigned long)14;
			de_clk_tmp_khz = c3 / 10;
			DRM_DEBUG_DRIVER("de_clk_tmp_khz %lu , c0/c1 %lu , c0 %lu , c1 %lu , c2 %lu , c3 %lu\n", de_clk_tmp_khz, c0 / c1, c0, c1, c2, c3);
			if (de_clk_tmp_khz > (engine->clk_freq / 1000))
				de_clk_tmp_khz = (engine->clk_freq / 1000);
			de_clk_ret_khz = (de_clk_ret_khz > de_clk_tmp_khz) ? de_clk_ret_khz : de_clk_tmp_khz;
			if (de_clk_ret_khz == (engine->clk_freq / 1000))
				break;
		}
	}

	c0 = (unsigned long)engine->clk_freq / 1000;
	sunxi_de_div_calc_mn(c0, de_clk_ret_khz, &m, &n);

	DRM_DEBUG_DRIVER("c0 %lu , c1 %lu , c2 %lu \n", c0, c1, c2);
	DRM_DEBUG_DRIVER("de_clk_ret_khz %lu , m %d , n %d\n", de_clk_ret_khz, m, n);
	DRM_DEBUG_DRIVER("de_clk_ret_khz %lu , ret clk %lu , ret_clk %d\n", de_clk_ret_khz, c0 * (n + 1) / (m + 1), (c0 * (n + 1) / (m + 1)) >= de_clk_ret_khz);
	de_top_freq_div_apply(engine->top_hdl, m, n);
	return 0;
}

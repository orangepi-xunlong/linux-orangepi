/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * afbd_wb/afbd_wb.c
 *
 * Copyright (c) 2007-2023 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
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

#include "afbd_wb.h"
#include "dec_reg.h"

#define AFBD_WB_ALIGN(value, align)												   \
	((align == 0) ? value : (((value) + ((align)-1)) & ~((align)-1)))

static struct afbd_wb_dev_t *g_afbd_wb_inst;

static int __afbd_wb_set_outbuf_addr(struct afbd_wb_dev_t *pdev, struct afbd_wb_info *pinfo,
				     dma_addr_t out_addr)
{
	int w = 0, h = 0, uv_offset = 0, ret = -1;

	ret = pdev->afbd_get_pic_size(pdev, pinfo, &w, &h);
	if (ret) {
		pr_err("afbd_get_pic_size fail!\n");
		return ret;
	}
	switch (pinfo->ch_id) {
	case 0: {
		// ch0 write back always output 10bit data
		uv_offset = w * h * 2;
		pdev->p_afbd_reg->p_wb_reg->wb_ch0_y_addr =
		    (unsigned int)out_addr;
		pdev->p_afbd_reg->p_wb_reg->wb_ch0_c_addr =
		    (unsigned int)(out_addr + uv_offset);
	} break;
	default:
		pr_err("Wrong channel id:%d\n", pinfo->ch_id);
		return -1;
		break;
	}

	pdev->p_afbd_reg->p_wb_reg->wb_ch0to4_fifo_ctrl = 0x22222222;
	pdev->p_afbd_reg->p_wb_reg->wb_ch5to7_fifo_ctrl = 0x22222222;
	return 0;
}

static int __afbd_wb_en(struct afbd_wb_dev_t *pdev, int ch_id, int en)
{

	unsigned int val;

	pdev->p_afbd_reg->p_wb_reg->wb_ctrl.bits.wb_mode = pdev->offline_mode;
	val = pdev->p_afbd_reg->p_wb_reg->wb_ch_en.dwval;

	if (en) {
		val |= (1 << (ch_id * 4));
	} else {
		val &= ~(1 << (ch_id * 4));
	}

	pdev->p_afbd_reg->p_wb_reg->wb_ch_en.dwval = val;
	pdev->p_afbd_reg->p_wb_reg->wb_ctrl.bits.wb_start = en;
	pdev->p_afbd_reg->p_top_reg->reg_ready = en;
	if (!en) {
		if (pdev->buf) {
			dec_dma_unmap(pdev->buf);
			pdev->buf = NULL;
		}
	}
	return 0;
}

static int __afbd_is_ch_en(struct afbd_wb_dev_t *pdev, int ch_id)
{
	switch (ch_id) {
	case 0:
		return pdev->p_afbd_reg->p_top_reg->ctrl.bits.en;
		break;
	default:
		pr_err("Unsupport channel :%d\n", ch_id);
		return 0;
		break;
	}
	return 0;
}

static int __afbd_get_pixel_fmt(struct afbd_wb_dev_t *pdev, int ch_id)
{
	switch (ch_id) {
	case 0:
		return pdev->p_afbd_reg->p_top_reg->attr.bits.format;
		break;
	default:
		pr_err("Unsupport channel id:%d\n", ch_id);
		return -1;
		break;
	}
	return 0;
}

static int __afbd_get_pic_size(struct afbd_wb_dev_t *pdev, struct afbd_wb_info *wb_info,
			 unsigned int *w, unsigned int *h)
{
	enum wb_src_pixel_fmt fmt;
	int Bpp = 4;
	int is_compressed = 0;

	if (!w || !h) {
		pr_err("NULL pointer!\n");
		return -1;
	}

	is_compressed = pdev->afbd_is_compressed(pdev, wb_info->ch_id);
	fmt = pdev->afbd_get_pixel_fmt(pdev, wb_info->ch_id);
	if (!is_compressed) {
		switch (fmt) {
		case AFBD_RGB888:
			Bpp = 3;
			break;
		case AFBD_RGBA5551:
		case AFBD_RGBA4444:
		case AFBD_RGB565:
		case AFBD_RG88:
			Bpp = 2;
			break;
		case AFBD_R10G10B10A2:
		case AFBD_R11G11B10:
		case AFBD_RGBA8888:
			Bpp = 4;
			break;
		case AFBD_R8:
			Bpp = 1;
			break;
		default:
			Bpp = 1;
			break;
		}
	} else {
		Bpp = 1;
	}

	switch (wb_info->ch_id) {
	case 0:
		*w = wb_info->width;
		*h = wb_info->height;
		break;
	default:
		pr_err("Unsupport channel id:%d\n", wb_info->ch_id);
		return -1;
		break;
	}

	if (*w == 0 || *h == 0) {
		pr_err("w or h is zero!\n");
		return -1;
	}
	*w /= Bpp;
	pr_err("size [%d x %d] fmt:%d is_compressed:%d\n", *w, *h, fmt, is_compressed);

	return 0;
}

struct afbd_wb_dev_t *get_afbd_wb_inst(void)
{
	return g_afbd_wb_inst;
}

static int __afbd_wait_fb_finish(struct afbd_wb_dev_t *pdev, int ch_id)
{
	unsigned int status_reg = 0;

	switch (ch_id) {
	case 0:
		status_reg = pdev->p_afbd_reg->p_top_reg->status.dwval;
		break;
	default:
		pr_err("Unsupport channel :%d\n", ch_id);
		break;
	}

	return (status_reg & 0x1);
}

int __afbd_wb_commit(struct afbd_wb_dev_t *pdev, struct afbd_wb_info *wb_info)
{
	int ret = -1;
	if (pdev && wb_info) {
		if (!pdev->afbd_is_ch_en(pdev, wb_info->ch_id)) {
			pr_err("channel %d is not enabled!\n", wb_info->ch_id);
			return ret;
		}
		memcpy(&pdev->wb_info, wb_info, sizeof(struct afbd_wb_info));
		pdev->buf = dec_dma_map(wb_info->dmafd, pdev->p_device);
		ret = pdev->afbd_wb_set_outbuf_addr(pdev, &pdev->wb_info, pdev->buf->dma_addr);
		if (ret) {
			pr_err("afbd_wb_set_outbuf_addr fail!\n");
			dec_dma_unmap(pdev->buf);
			pdev->buf = NULL;
		}
	}
	return ret;
}

static int __set_wb_state(struct afbd_wb_dev_t *pdev, enum afbd_wb_state state)
{
	if (!pdev)
		return -1;
	mutex_lock(&pdev->state_lock);
	pdev->wb_state = state;
	mutex_unlock(&pdev->state_lock);
	return 0;
}

static enum afbd_wb_state __get_wb_state(struct afbd_wb_dev_t *pdev)
{
	enum afbd_wb_state state;

	if (!pdev)
		return AFBD_WB_IDLE;

	mutex_lock(&pdev->state_lock);
	state =  pdev->wb_state;
	mutex_unlock(&pdev->state_lock);
	return state;
}

static int __afbd_is_compressed(struct afbd_wb_dev_t *pdev, int ch_id)
{
	switch (ch_id) {
	case 0:
		return pdev->p_afbd_reg->p_top_reg->attr.bits.is_compressed;
		break;
	default:
		pr_err("unsupport channel :%d\n", ch_id);
		return 0;
		break;
	}
	return 0;

}

struct afbd_wb_dev_t *afbd_wb_init(struct afbd_reg_t *p_reg,
				   struct device *p_device, bool offline_en)
{

	struct afbd_wb_dev_t *p_wb_dev = NULL;

	if (!p_reg || !p_reg->p_top_reg || !p_reg->p_dec_reg ||
	    !p_reg->p_wb_reg) {
		pr_err("NULL reg define!\n");
		goto OUT;
	}

	if (g_afbd_wb_inst) {
		pr_err("afbd wb has been inited!\n");
		goto OUT;
	}

	p_wb_dev = kmalloc(sizeof(struct afbd_wb_dev_t), __GFP_ZERO | GFP_KERNEL);
	if (!p_wb_dev) {
		pr_err("malloc afbd_wb_dev_t fail!\n");
		goto OUT;
	}
	p_wb_dev->p_afbd_reg = p_reg;
	p_wb_dev->p_device = p_device;
	mutex_init(&p_wb_dev->state_lock);

	p_wb_dev->afbd_set_wb_state = __set_wb_state;
	p_wb_dev->get_wb_state = __get_wb_state;
	p_wb_dev->afbd_wb_set_outbuf_addr = __afbd_wb_set_outbuf_addr;
	p_wb_dev->afbd_wb_en = __afbd_wb_en;
	p_wb_dev->afbd_is_ch_en = __afbd_is_ch_en;
	p_wb_dev->afbd_is_compressed = __afbd_is_compressed;
	p_wb_dev->afbd_get_pixel_fmt = __afbd_get_pixel_fmt;
	p_wb_dev->afbd_get_pic_size = __afbd_get_pic_size;
	p_wb_dev->afbd_wait_fb_finish = __afbd_wait_fb_finish;
	p_wb_dev->afbd_wb_commit = __afbd_wb_commit;
	p_wb_dev->offline_mode = offline_en;

	g_afbd_wb_inst = p_wb_dev;
	p_wb_dev->p_afbd_reg->p_wb_reg->wb_ctrl.bits.wb_mode = offline_en;
	goto OUT;
OUT:
	return g_afbd_wb_inst;
}

void afbd_wb_deinit(void)
{
	if (g_afbd_wb_inst) {
		kfree(g_afbd_wb_inst);
		g_afbd_wb_inst = NULL;
	}
}


//End of File


//End of File

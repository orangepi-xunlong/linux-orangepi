/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/*
 * vin_video.c for video api
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 * Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
  */
#include "../utility/vin_log.h"
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <linux/sort.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/moduleparam.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-dma-contig.h>
#include <linux/dma-buf.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
#include <linux/dma-heap.h>
#endif
#include <linux/dma-mapping.h>

#include <linux/regulator/consumer.h>
#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY)
#include <linux/sunxi_dramfreq.h>
#endif

#include "../utility/config.h"
#include "../modules/sensor/sensor_helper.h"
#include "../utility/vin_io.h"
#include "../vin-csi/sunxi_csi.h"
#include "../vin-isp/sunxi_isp.h"
#include "../vin-vipp/sunxi_scaler.h"
#include "../vin-mipi/sunxi_mipi.h"
#include "../vin-tdm/vin_tdm.h"
#include "../vin.h"
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
#include "../vin-isp/isp_tuning_priv.h"
#endif

#define VIN_MAJOR_VERSION 1
#define VIN_MINOR_VERSION 1
#define VIN_RELEASE       0

#define MIN_IN_WIDTH			192
#define MIN_IN_HEIGHT			128

#define VIN_VERSION \
		KERNEL_VERSION(VIN_MAJOR_VERSION, VIN_MINOR_VERSION, VIN_RELEASE)

extern struct vin_core *vin_core_gbl[VIN_MAX_DEV];
extern struct scaler_dev *glb_vipp[VIN_MAX_SCALER];
#define GET_BIT(x, bit) ((x & (1 << bit)) >> bit)

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
void *vin_map_kernel(unsigned long phys_addr, unsigned long size)
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

	pgprot = PAGE_KERNEL;
	vaddr = vmap(pages, npages, VM_MAP, pgprot);
	vfree(pages);
	return vaddr;
}
EXPORT_SYMBOL(vin_map_kernel);

void vin_unmap_kernel(void *vaddr)
{
	vunmap(vaddr);
}
EXPORT_SYMBOL(vin_unmap_kernel);
#endif

void __vin_s_stream_handle(struct work_struct *work)
{
	int ret = 0;
	struct vin_vid_cap *cap =
			container_of(work, struct vin_vid_cap, s_stream_task);

	vin_timer_init(cap->vinc);
	ret = vin_pipeline_call(cap->vinc, set_stream, &cap->pipe, cap->vinc->stream_idx);
	if (ret < 0) {
		vin_err("%s error!\n", __func__);
		return;
	}
	/* set saved exp and gain for reopen */
	if (cap->vinc->exp_gain.exp_val && cap->vinc->exp_gain.gain_val) {
		v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl,
			VIDIOC_VIN_SENSOR_EXP_GAIN, &cap->vinc->exp_gain);
	}

	vin_log(VIN_LOG_VIDEO, "%s done, id = %d!\n", __func__, cap->vinc->id);
}

/* make sure addr was update to register */
#if defined CSIC_DMA_VER_140_000
static int __check_bk_bufaddr(struct vin_core *vinc, struct vin_addr *paddr)
{
	unsigned int y, cb, cr;

	if (vinc->vid_cap.frame.fmt.fourcc == V4L2_PIX_FMT_YVU420) {
		csic_dma_get_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, &y);
		csic_dma_get_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, &cr);
		csic_dma_get_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, &cb);
	} else {
		csic_dma_get_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, &y);
		csic_dma_get_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, &cb);
		csic_dma_get_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, &cr);
	}

	if (paddr->y != y || paddr->cb != cb || paddr->cr != cr) {
		vin_err("vinc%d cannot write and read the right addr to register,y 0x%x, cb 0x%x,cr 0x%x\n", vinc->id, y, cb, cr);
		return -EINVAL;
	}

	return 0;
}
#else
static int __check_bk_bufaddr(struct vin_core *vinc, struct vin_addr *paddr)
{
	unsigned int y, cb, cr;
	/* unsigned int cnt = 0; */

	if (vinc->vid_cap.frame.fmt.fourcc == V4L2_PIX_FMT_YVU420) {
		y = readl(vinc->base + 0x20) << 2;
		cr = readl(vinc->base + 0x28) << 2;
		cb = readl(vinc->base + 0x30) << 2;
	} else {
		y = readl(vinc->base + 0x20) << 2;
		cb = readl(vinc->base + 0x28) << 2;
		cr = readl(vinc->base + 0x30) << 2;
	}

	/*
	while ((paddr->y != y || paddr->cb != cb || paddr->cr != cr) && (cnt < 2)) {
		if(vinc->vid_cap.frame.fmt.fourcc == V4L2_PIX_FMT_YVU420) {
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cb);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cr);
			y = readl(vinc->base + 0x20) << 2;
			cr = readl(vinc->base + 0x28) << 2;
			cb = readl(vinc->base + 0x30) << 2;

		} else {
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr);
			y = readl(vinc->base + 0x20) << 2;
			cb = readl(vinc->base + 0x28) << 2;
			cr = readl(vinc->base + 0x30) << 2;
		}
		cnt++;
	}
	 */

	if (paddr->y != y || paddr->cb != cb || paddr->cr != cr) {
		vin_err("vinc%d cannot write and read the right addr to register!!\n", vinc->id);
		return -EINVAL;
	}

	return 0;
}
#endif

/*  The color format (colplanes, memplanes) must be already configured.  */
int vin_set_addr(struct vin_core *vinc, struct vb2_buffer *vb,
		      struct vin_frame *frame, struct vin_addr *paddr)
{
	u32 pix_size, depth, y_stride, u_stride, v_stride;
	struct vb2_v4l2_buffer *vb2_v4l2;
	struct vin_buffer *buf;
	__maybe_unused int offset_width;
	__maybe_unused struct vin_core *vinc_bind = NULL;

	vb2_v4l2 = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	buf = container_of(vb2_v4l2, struct vin_buffer, vb);

	if (vinc->vid_cap.special_active == 1) {
		if (buf == NULL || buf->paddr == NULL)
			return -EINVAL;
	} else {
		if (vb == NULL || frame == NULL)
			return -EINVAL;
	}

#if !defined CONFIG_ARCH_SUN8IW21
	pix_size = ALIGN(frame->o_width, VIN_ALIGN_WIDTH) * frame->o_height;
#else
	pix_size = ALIGN(frame->o_width, VIN_ALIGN_WIDTH) * ALIGN(frame->o_height, VIN_ALIGN_HEIGHT);
#endif
	depth = frame->fmt.depth[0] + frame->fmt.depth[1] + frame->fmt.depth[2];

	if (vinc->vid_cap.special_active == 1) {
		paddr->y = (dma_addr_t)buf->paddr;
		frame->fmt.memplanes = 1;
	} else
		paddr->y = vb2_dma_contig_plane_dma_addr(vb, 0);

	if (frame->fmt.memplanes == 1) {
		switch (frame->fmt.colplanes) {
		case 1:
			paddr->cb = 0;
			paddr->cr = 0;
			break;
		case 2:
			/*  decompose Y into Y/Cb  */

			if (frame->fmt.fourcc == V4L2_PIX_FMT_FBC) {
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1)
				paddr->cb = (u32)(paddr->y + CEIL_EXP(frame->o_width, 7) * CEIL_EXP(frame->o_height, 5) * 96);
#else
				paddr->cb = paddr->y + ALIGN(DIV_ROUND_UP(frame->o_width, 128) * DIV_ROUND_UP(frame->o_height, 32) * 96, 64);
#endif
				paddr->cr = 0;

			} else {
				paddr->cb = (u32)(paddr->y + pix_size);
				paddr->cr = 0;
			}
			break;
		case 3:
			paddr->cb = (u32)(paddr->y + pix_size);
			/*  420  */
			if (frame->fmt.depth[0] == 12)
				paddr->cr = (u32)(paddr->cb + (pix_size >> 2));
			else /*  422  */
				paddr->cr = (u32)(paddr->cb + (pix_size >> 1));
			break;
		default:
			return -EINVAL;
		}
	} else if (!frame->fmt.mdataplanes) {
		if (frame->fmt.memplanes >= 2)
			paddr->cb = vb2_dma_contig_plane_dma_addr(vb, 1);

		if (frame->fmt.memplanes == 3)
			paddr->cr = vb2_dma_contig_plane_dma_addr(vb, 2);
	}

#ifdef NO_SUPPROT_HARDWARE_CALCULATE
	if ((vinc->vflip == 1) && (frame->fmt.fourcc == V4L2_PIX_FMT_FBC)) {
		paddr->y += CEIL_EXP(frame->o_width, 7) * (CEIL_EXP(frame->o_height, 5) - 1) *  96;
		paddr->cb += CEIL_EXP(frame->o_width, 4) * (CEIL_EXP(frame->o_height, 2) - 1) * 96;
		paddr->cr = 0;
	} else if (vinc->vflip == 1) {
		switch (frame->fmt.colplanes) {
		case 1:
			paddr->y += (pix_size - frame->o_width) * frame->fmt.depth[0] / 8;
			paddr->cb = 0;
			paddr->cr = 0;
			break;
		case 2:
			paddr->y += pix_size - frame->o_width;
			/*  420  */
			if (depth == 12)
				paddr->cb += pix_size / 2 - frame->o_width;
			else /*  422  */
				paddr->cb += pix_size - frame->o_width;
			paddr->cr = 0;
			break;
		case 3:
			paddr->y += pix_size - frame->o_width;
			if (depth == 12) {
				paddr->cb += pix_size / 4 - frame->o_width / 2;
				paddr->cr += pix_size / 4 - frame->o_width / 2;
			} else {
				paddr->cb += pix_size / 2 - frame->o_width / 2;
				paddr->cr += pix_size / 2 - frame->o_width / 2;
			}
			break;
		default:
			return -EINVAL;
		}
	}
#endif
	if ((vinc->large_image == 2) && (vinc->vin_status.frame_cnt % 2)) {
		if (frame->fmt.colplanes == 3) {
			if (depth == 12) {
				/*  420  */
				y_stride = frame->o_width / 2;
				u_stride = frame->o_width / 2 / 2;
				v_stride = frame->o_width / 2 / 2;
			} else {
				/*  422  */
				y_stride = frame->o_width / 2;
				u_stride = frame->o_width / 2;
				v_stride = frame->o_width / 2;
			}
		} else {
			y_stride = frame->o_width / 2;
			u_stride = frame->o_width / 2;
			v_stride = frame->o_width / 2;
		}

		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y + y_stride);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb + u_stride);
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr + v_stride);
	} else {
		if (vinc->vid_cap.frame.fmt.fourcc == V4L2_PIX_FMT_YVU420) {
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cb);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cr);
		} else {
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr);
		}
		if (__check_bk_bufaddr(vinc, paddr))
			return -EINVAL;
	}

	if (vinc->dma_merge_mode == 1) {
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			offset_width = frame->o_width/2;
			vinc_bind = vin_core_gbl[vinc->id - 1];
			csic_dma_buffer_address(vinc_bind->vipp_sel, CSI_BUF_0_A, paddr->y);
			csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, paddr->y + offset_width);
			if (paddr->cr && paddr->cb) {
				csic_dma_buffer_address(vinc_bind->vipp_sel, CSI_BUF_1_A, paddr->cb);
				csic_dma_buffer_address(vinc_bind->vipp_sel, CSI_BUF_2_A, paddr->cr);
				csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb + (offset_width >> 1));
				csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr + (offset_width >> 1));
			} else if (paddr->cb) {
				csic_dma_buffer_address(vinc_bind->vipp_sel, CSI_BUF_1_A, paddr->cb);
				csic_dma_buffer_address(vinc_bind->vipp_sel, CSI_BUF_2_A, paddr->cr);
				csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb + (offset_width));
				csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr);
			} else {
				csic_dma_buffer_address(vinc_bind->vipp_sel, CSI_BUF_1_A, paddr->cb);
				csic_dma_buffer_address(vinc_bind->vipp_sel, CSI_BUF_2_A, paddr->cr);
				csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_1_A, paddr->cb);
				csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_2_A, paddr->cr);
			}
		}
	}

	return 0;
}

void vin_set_next_buf_addr(struct vin_core *vinc)
{
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct vin_buffer *buf;
	__maybe_unused struct list_head *buf_next;
	__maybe_unused int i;

	if (vinc->large_image == 1)
		return;

	vinc->vid_cap.first_flag = 0;
	vinc->vin_status.frame_cnt = 0;
	vinc->vin_status.err_cnt = 0;
	vinc->vin_status.lost_cnt = 0;

#ifndef BUF_AUTO_UPDATE
	buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);
	vin_set_addr(vinc, &buf->vb.vb2_buf, &vinc->vid_cap.frame, &vinc->vid_cap.frame.paddr);
#else
	vin_get_rest_buf_cnt(vinc);
	cap->threshold.bufa_fifo_threshold = 1;
	cap->threshold.stored_frm_threshold = 2;
	cap->threshold.bufa_fifo_total = vinc->vin_status.buf_rest;
	csic_buf_addr_fifo_en(vinc->vipp_sel, 1);
	csic_set_threshold_for_bufa_mode(vinc->vipp_sel, &cap->threshold);
	buf_next = cap->vidq_active.next;
	for (i = 0; i < cap->threshold.bufa_fifo_total; i++) {
		buf = list_entry(buf_next, struct vin_buffer, list);
		vin_set_addr(vinc, &buf->vb.vb2_buf, &vinc->vid_cap.frame, &vinc->vid_cap.frame.paddr);
		buf_next = buf_next->next;
	}
#endif
}

static int lbc_mode_select(struct dma_lbc_cmp *lbc_cmp, unsigned int fourcc)
{
	switch (fourcc) {
	case V4L2_PIX_FMT_LBC_2_0X: /*  2x  */
		lbc_cmp->is_lossy = 1;
		lbc_cmp->bit_depth = 8;
		lbc_cmp->glb_enable = 1;
		lbc_cmp->dts_enable = 1;
		lbc_cmp->ots_enable = 1;
		lbc_cmp->msq_enable = 1;
		lbc_cmp->cmp_ratio_even = 600;
		lbc_cmp->cmp_ratio_odd  = 450;
		lbc_cmp->mb_mi_bits[0]  = 55;
		lbc_cmp->mb_mi_bits[1]  = 110;
		lbc_cmp->rc_adv[0] = 60;
		lbc_cmp->rc_adv[1] = 30;
		lbc_cmp->rc_adv[2] = 15;
		lbc_cmp->rc_adv[3] = 8;
		lbc_cmp->lmtqp_en  = 1;
		lbc_cmp->lmtqp_min = 1;
		lbc_cmp->updata_adv_en = 1;
		lbc_cmp->updata_adv_ratio = 2;
		break;
	case V4L2_PIX_FMT_LBC_2_5X: /*  2.5x  */
		lbc_cmp->is_lossy = 1;
		lbc_cmp->bit_depth = 8;
		lbc_cmp->glb_enable = 1;
		lbc_cmp->dts_enable = 1;
		lbc_cmp->ots_enable = 1;
		lbc_cmp->msq_enable = 1;
		lbc_cmp->cmp_ratio_even = 440;
		lbc_cmp->cmp_ratio_odd  = 380;
		lbc_cmp->mb_mi_bits[0]  = 55;
		lbc_cmp->mb_mi_bits[1]  = 94;
		lbc_cmp->rc_adv[0] = 60;
		lbc_cmp->rc_adv[1] = 30;
		lbc_cmp->rc_adv[2] = 15;
		lbc_cmp->rc_adv[3] = 8;
		lbc_cmp->lmtqp_en  = 1;
		lbc_cmp->lmtqp_min = 1;
		lbc_cmp->updata_adv_en = 1;
		lbc_cmp->updata_adv_ratio = 2;
		break;
	case V4L2_PIX_FMT_LBC_1_0X: /*  lossless  */
		lbc_cmp->is_lossy = 0;
		lbc_cmp->bit_depth = 8;
		lbc_cmp->glb_enable = 1;
		lbc_cmp->dts_enable = 1;
		lbc_cmp->ots_enable = 1;
		lbc_cmp->msq_enable = 1;
		lbc_cmp->cmp_ratio_even = 1000;
		lbc_cmp->cmp_ratio_odd  = 1000;
		lbc_cmp->mb_mi_bits[0]  = 55;
		lbc_cmp->mb_mi_bits[1]  = 94;
		lbc_cmp->rc_adv[0] = 60;
		lbc_cmp->rc_adv[1] = 30;
		lbc_cmp->rc_adv[2] = 15;
		lbc_cmp->rc_adv[3] = 8;
		lbc_cmp->lmtqp_en  = 1;
		lbc_cmp->lmtqp_min = 1;
		lbc_cmp->updata_adv_en = 1;
		lbc_cmp->updata_adv_ratio = 2;
		break;
	case V4L2_PIX_FMT_LBC_1_5X: /* 1.5x */
		lbc_cmp->is_lossy = 1;
		lbc_cmp->bit_depth = 8;
		lbc_cmp->glb_enable = 1;
		lbc_cmp->dts_enable = 1;
		lbc_cmp->ots_enable = 1;
		lbc_cmp->msq_enable = 1;
		lbc_cmp->cmp_ratio_even = 670;
		lbc_cmp->cmp_ratio_odd	= 658;
		lbc_cmp->mb_mi_bits[0]	= 87;
		lbc_cmp->mb_mi_bits[1]	= 167;
		lbc_cmp->rc_adv[0] = 60;
		lbc_cmp->rc_adv[1] = 30;
		lbc_cmp->rc_adv[2] = 15;
		lbc_cmp->rc_adv[3] = 8;
		lbc_cmp->lmtqp_en  = 1;
		lbc_cmp->lmtqp_min = 1;
		lbc_cmp->updata_adv_en = 1;
		lbc_cmp->updata_adv_ratio = 2;
		break;
	default:
		return -1;
	}
	return 0;
}

/*
 * Videobuf operations
  */
static int queue_setup(struct vb2_queue *vq,
		       unsigned int *nbuffers, unsigned int *nplanes,
		       unsigned int sizes[], struct device *alloc_devs[])
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vq);
	unsigned int size;
	int buf_max_flag = 0;
	int wth;
	int i;

	cap->frame.bytesperline[0] = cap->frame.o_width * cap->frame.fmt.depth[0] / 8;
	cap->frame.bytesperline[1] = cap->frame.o_width * cap->frame.fmt.depth[1] / 8;
	cap->frame.bytesperline[2] = cap->frame.o_width * cap->frame.fmt.depth[2] / 8;

#if VIN_FALSE
	size = cap->frame.o_width * cap->frame.o_height;
#else
	size = roundup(cap->frame.o_width, VIN_ALIGN_WIDTH) * roundup(cap->frame.o_height, VIN_ALIGN_HEIGHT);
#endif

	switch (cap->frame.fmt.fourcc) {
	case V4L2_PIX_FMT_FBC:
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1)
		cap->frame.payload[0] = (CEIL_EXP(cap->frame.o_width, 7) * CEIL_EXP(cap->frame.o_height, 5) +
			CEIL_EXP(cap->frame.o_width, 4) * CEIL_EXP(cap->frame.o_height, 2)) * 96;
#else
		cap->frame.payload[0] = (ALIGN(cap->frame.o_width / 16 * 96, 64) * (cap->frame.o_height / 4)) +
			ALIGN(DIV_ROUND_UP(cap->frame.o_width, 128) * DIV_ROUND_UP(cap->frame.o_height, 32) * 96, 64);
#endif
		break;
	case V4L2_PIX_FMT_LBC_2_0X:
	case V4L2_PIX_FMT_LBC_2_5X:
	case V4L2_PIX_FMT_LBC_1_0X:
	case V4L2_PIX_FMT_LBC_1_5X:
		lbc_mode_select(&cap->lbc_cmp, cap->frame.fmt.fourcc);
		wth = roundup(cap->frame.o_width, 32);
		if (cap->lbc_cmp.is_lossy) {
			cap->lbc_cmp.line_tar_bits[0] = roundup(cap->lbc_cmp.cmp_ratio_even * wth * cap->lbc_cmp.bit_depth/1000, 512);
			cap->lbc_cmp.line_tar_bits[1] = roundup(cap->lbc_cmp.cmp_ratio_odd * wth * cap->lbc_cmp.bit_depth/500, 512);
		} else {
			cap->lbc_cmp.line_tar_bits[0] = roundup(wth * cap->lbc_cmp.bit_depth * 1 + (wth * 1 / 16 * 2), 512);
			cap->lbc_cmp.line_tar_bits[1] = roundup(wth * cap->lbc_cmp.bit_depth * 2 + (wth * 2 / 16 * 2), 512);
		}
		/* add 1KB buffer to fix ve-lbc error */
		cap->frame.payload[0] = (cap->lbc_cmp.line_tar_bits[0] + cap->lbc_cmp.line_tar_bits[1]) * cap->frame.o_height/2/8 + 1024;
		break;
	default:
		cap->frame.payload[0] = size * cap->frame.fmt.depth[0] / 8;
		break;
	}
	cap->frame.payload[1] = size * cap->frame.fmt.depth[1] / 8;
	cap->frame.payload[2] = size * cap->frame.fmt.depth[2] / 8;
	cap->buf_byte_size =
		PAGE_ALIGN(cap->frame.payload[0]) +
		PAGE_ALIGN(cap->frame.payload[1]) +
		PAGE_ALIGN(cap->frame.payload[2]);

	size = cap->buf_byte_size;

	if (size == 0)
		return -EINVAL;

	if (*nbuffers == 0)
		*nbuffers = 8;

	while (size * *nbuffers > MAX_FRAME_MEM) {
		(*nbuffers)--;
		buf_max_flag = 1;
		if (*nbuffers == 0)
			vin_err("Buffer size > max frame memory! count = %d\n",
			     *nbuffers);
	}

	if (buf_max_flag == 0) {
		if (cap->capture_mode == V4L2_MODE_IMAGE) {
			if (*nbuffers != 1) {
				*nbuffers = 1;
				vin_err("buffer count != 1 in capture mode\n");
			}
		} else {
			if (cap->vinc->ve_online_cfg.dma_buf_num == BK_TWO_BUFFER && cap->vinc->id == CSI_VE_ONLINE_VIDEO && *nbuffers != 2) {
				*nbuffers = 2;
				vin_print("video%d:buffer count is invalid, set to 2\n", cap->vinc->id);
			} else if (cap->vinc->ve_online_cfg.dma_buf_num == BK_ONE_BUFFER && cap->vinc->id == CSI_VE_ONLINE_VIDEO && *nbuffers != 1) {
				*nbuffers = 1;
				vin_print("video%d:buffer count is invalid, set to 1\n", cap->vinc->id);
			} else {
				if (*nbuffers < 3) {
#if IS_ENABLED(CONFIG_DISPPLAY_SYNC)
					if (cap->vinc->id != disp_sync_video)
						*nbuffers = 3;
					vin_warn("buffer count is %d\n", *nbuffers);
#else
					*nbuffers = 3;
					vin_err("buffer count is invalid, set to 3\n");
#endif
				}
			}
		}
	}

	*nplanes = cap->frame.fmt.memplanes;
	for (i = 0; i < *nplanes; i++) {
		sizes[i] = cap->frame.payload[i];
		alloc_devs[i] = cap->dev;
	}
	vin_log(VIN_LOG_VIDEO, "%s, buf count = %d, nplanes = %d, size = %d\n",
		__func__, *nbuffers, *nplanes, size);
	cap->vinc->vin_status.buf_cnt = *nbuffers;
	cap->vinc->vin_status.buf_size = size;
	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vvb = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	struct vin_buffer *buf = container_of(vvb, struct vin_buffer, vb);
	int i;

	if (cap->frame.o_width < MIN_WIDTH || cap->frame.o_width > MAX_WIDTH ||
	    cap->frame.o_height < MIN_HEIGHT || cap->frame.o_height > MAX_HEIGHT) {
		return -EINVAL;
	}
	/* size = dev->buf_byte_size; */

	for (i = 0; i < cap->frame.fmt.memplanes; i++) {
		if (vb2_plane_size(vb, i) < cap->frame.payload[i]) {
			vin_err("%s data will not fit into plane (%lu < %lu)\n",
				__func__, vb2_plane_size(vb, i),
				cap->frame.payload[i]);
			return -EINVAL;
		}
		vb2_set_plane_payload(&buf->vb.vb2_buf, i, cap->frame.payload[i]);
		vb->planes[i].m.offset = vb2_dma_contig_plane_dma_addr(vb, i);
	}

	return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vb->vb2_queue);
	struct vb2_v4l2_buffer *vvb = container_of(vb, struct vb2_v4l2_buffer, vb2_buf);
	struct vin_buffer *buf = container_of(vvb, struct vin_buffer, vb);
	unsigned long flags = 0;

	spin_lock_irqsave(&cap->slock, flags);
	if (cap->vinc->id == CSI_VE_ONLINE_VIDEO && cap->vinc->ve_online_cfg.dma_buf_num == BK_TWO_BUFFER) {
		if (buf->first_qbuf == 0) {
			list_add_tail(&buf->list, &cap->vidq_active);
			buf->first_qbuf = 1;
		}
		buf->qbufed = 1;
	} else {
		list_add_tail(&buf->list, &cap->vidq_active);
	}
#ifdef BUF_AUTO_UPDATE
	vin_set_addr(cap->vinc, &buf->vb.vb2_buf, &cap->frame, &cap->frame.paddr);
#endif
	spin_unlock_irqrestore(&cap->slock, flags);
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	return 0;
}

/*  abort streaming and wait for last buffer  */
static void stop_streaming(struct vb2_queue *vq)
{
	struct vin_vid_cap *cap = vb2_get_drv_priv(vq);
	unsigned long flags = 0;

	spin_lock_irqsave(&cap->slock, flags);
	/*  Release all active buffers  */
	while (!list_empty(&cap->vidq_active)) {
		struct vin_buffer *buf;

		buf = list_entry(cap->vidq_active.next, struct vin_buffer, list);
		list_del(&buf->list);
		if (cap->vinc->id == CSI_VE_ONLINE_VIDEO && cap->vinc->ve_online_cfg.dma_buf_num == BK_TWO_BUFFER) {
			if (!buf->qbufed)
				continue;
			buf->first_qbuf = 0;
		}
		vb2_buffer_done(&buf->vb.vb2_buf, VB2_BUF_STATE_ERROR);
		vin_log(VIN_LOG_VIDEO, "buf %d stop\n", buf->vb.vb2_buf.index);
	}
	spin_unlock_irqrestore(&cap->slock, flags);
}

static const struct vb2_ops vin_video_qops = {
	.queue_setup = queue_setup,
	.buf_prepare = buffer_prepare,
	.buf_queue = buffer_queue,
	.start_streaming = start_streaming,
	.stop_streaming = stop_streaming,
	.wait_prepare = vb2_ops_wait_prepare,
	.wait_finish = vb2_ops_wait_finish,
};

/*
 * IOCTL vidioc handling
  */
static int vidioc_querycap(struct file *file, void *priv,
			   struct v4l2_capability *cap)
{
	strcpy(cap->driver, "sunxi-vin");
	strcpy(cap->card, "sunxi-vin");

	cap->version = VIN_VERSION;
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE_MPLANE | V4L2_CAP_STREAMING |
			V4L2_CAP_READWRITE | V4L2_CAP_DEVICE_CAPS;

	return 0;
}

static int vidioc_enum_fmt_vid_cap_mplane(struct file *file, void *priv,
					  struct v4l2_fmtdesc *f)
{
	struct vin_fmt *fmt;

	fmt = vin_find_format(NULL, NULL, VIN_FMT_ALL, f->index, false);
	if (!fmt)
		return -EINVAL;

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

static int vidioc_enum_framesizes(struct file *file, void *fh,
				  struct v4l2_frmsizeenum *fsize)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_subdev_frame_size_enum fse;
	int ret;

	if (vinc == NULL)
		return -EINVAL;

	memset(&fse, 0, sizeof(fse));
	fse.index = fsize->index;
	fse.which = V4L2_SUBDEV_FORMAT_ACTIVE;

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], pad,
				enum_frame_size, NULL, &fse);
	if (ret < 0)
		return -1;

	fsize->type = V4L2_FRMSIZE_TYPE_CONTINUOUS;
	fsize->stepwise.min_width = MIN_IN_WIDTH;
	fsize->stepwise.min_height = MIN_IN_HEIGHT;
	fsize->stepwise.step_width = 2;
	fsize->stepwise.max_width = fse.max_width;
	fsize->stepwise.max_height = fse.max_height;
	fsize->stepwise.step_height = 2;

	return 0;
}

static int vidioc_enum_frameintervals(struct file *file, void *fh,
					  struct v4l2_frmivalenum *fival)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_subdev_frame_interval_enum fie;
	int ret;

	if (vinc == NULL)
		return -EINVAL;

	memset(&fie, 0, sizeof(fie));
	fie.index = fival->index;
	fie.width = fival->width;
	fie.height = fival->height;
	fie.which = V4L2_SUBDEV_FORMAT_ACTIVE;

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], pad,
				enum_frame_interval, NULL, &fie);
	if (ret < 0)
		return -1;

	fival->index = fie.index;	/* update index */
	fival->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	fival->discrete = fie.interval;

	return 0;
}

static int vidioc_g_fmt_vid_cap_mplane(struct file *file, void *priv,
				       struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	int i;

	pixm->width = vinc->vid_cap.frame.o_width;
	pixm->height = vinc->vid_cap.frame.o_height;
	pixm->field = V4L2_FIELD_NONE;
	pixm->pixelformat = vinc->vid_cap.frame.fmt.fourcc;
	pixm->colorspace = vinc->vid_cap.frame.fmt.color;/* V4L2_COLORSPACE_JPEG; */
	pixm->num_planes = vinc->vid_cap.frame.fmt.memplanes;

	for (i = 0; i < pixm->num_planes; ++i) {
		pixm->plane_fmt[i].bytesperline = vinc->vid_cap.frame.bytesperline[i];
		pixm->plane_fmt[i].sizeimage = vinc->vid_cap.frame.payload[i];
	}
	return 0;
}

static int vin_pipeline_try_format(struct vin_core *vinc,
				    struct v4l2_mbus_framefmt *tfmt,
				    struct vin_fmt **fmt_id,
				    bool set)
{
	struct v4l2_subdev *sd = vinc->vid_cap.pipe.sd[VIN_IND_SENSOR];
	struct v4l2_subdev_format sfmt;
	struct media_entity *me;
	struct vin_fmt *ffmt;
	unsigned int mask;
	struct media_graph graph;
	int ret, i = 0, sd_ind;
	int ch_id;

	if (WARN_ON(!sd || !tfmt || !fmt_id))
		return -EINVAL;

	memset(&sfmt, 0, sizeof(sfmt));
	sfmt.format = *tfmt;
	sfmt.which = set ? V4L2_SUBDEV_FORMAT_ACTIVE : V4L2_SUBDEV_FORMAT_TRY;

	mask = (*fmt_id)->flags;
	if ((mask & VIN_FMT_YUV) && (vinc->support_raw == 0))
		mask = VIN_FMT_YUV;

	/*  when diffrent video output have same sensor,
	 * this pipeline try fmt will lead to false result.
	 * so it should be updated at later.
	  */
	while (1) {

		ffmt = vin_find_format(NULL, sfmt.format.code != 0 ? &sfmt.format.code : NULL,
					mask, i++, true);
		if (ffmt == NULL) {
			/*
			 * Notify user-space if common pixel code for
			 * host and sensor does not exist.
			  */
			vin_err("vin is not support this pixelformat\n");
			return -EINVAL;
		}

		sfmt.format.code = tfmt->code = ffmt->mbus_code;
		me = &vinc->vid_cap.subdev.entity;

		mutex_lock(&vinc->vid_cap.vdev.entity.graph_obj.mdev->graph_mutex);
		if (media_graph_walk_init(&graph, me->graph_obj.mdev) != 0) {
			mutex_unlock(&vinc->vid_cap.vdev.entity.graph_obj.mdev->graph_mutex);
			return -EINVAL;
		}

		media_graph_walk_start(&graph, me);
		while ((me = media_graph_walk_next(&graph)) &&
			me != &vinc->vid_cap.subdev.entity) {

			sd = media_entity_to_v4l2_subdev(me);
			switch (sd->grp_id) {
			case VIN_GRP_ID_SENSOR:
				sd_ind = VIN_IND_SENSOR;
				break;
			case VIN_GRP_ID_MIPI:
				sd_ind = VIN_IND_MIPI;
				break;
			case VIN_GRP_ID_CSI:
				sd_ind = VIN_IND_CSI;
				break;
			case VIN_GRP_ID_TDM_RX:
				sd_ind = VIN_IND_TDM_RX;
				break;
			case VIN_GRP_ID_ISP:
				sd_ind = VIN_IND_ISP;
				break;
			case VIN_GRP_ID_SCALER:
				sd_ind = VIN_IND_SCALER;
				break;
			case VIN_GRP_ID_CAPTURE:
				sd_ind = VIN_IND_CAPTURE;
				break;
			default:
				sd_ind = VIN_IND_SENSOR;
				break;
			}

			if (sd != vinc->vid_cap.pipe.sd[sd_ind])
				continue;
			vin_log(VIN_LOG_FMT, "found %s in this pipeline\n", me->name);

			if (me->num_pads == 1 &&
				(me->pads[0].flags & MEDIA_PAD_FL_SINK)) {
				vin_log(VIN_LOG_FMT, "skip %s.\n", me->name);
				continue;
			}

			/* find ch id to set for tvin*/
			if (vinc->tvin.flag) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
				if (vinc->id > TVIN_VIDEO_MAX)
					ch_id = vinc->id / TVIN_VIDEO_STRIP + vinc->id % TVIN_VIDEO_STRIP;
				else
					ch_id = vinc->id / TVIN_VIDEO_STRIP;
#else
				ch_id = vinc->id;
#endif
				if (ch_id >= TVIN_SEPARATE)
					sfmt.reserved[0] = ch_id - TVIN_SEPARATE;
				else
					sfmt.reserved[0] = ch_id;
			}

			sfmt.pad = 0;
			ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &sfmt);
			if (ret) {
				mutex_unlock(&vinc->vid_cap.vdev.entity.graph_obj.mdev->graph_mutex);
				media_graph_walk_cleanup(&graph);
				return ret;
			}

			/* set isp input win size for isp server call sensor_req_cfg */
			if (sd->grp_id == VIN_GRP_ID_ISP)
				sensor_isp_input(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], &sfmt.format);

			/* change output resolution of scaler */
			if (sd->grp_id == VIN_GRP_ID_SCALER) {
				sfmt.format.width = tfmt->width;
				sfmt.format.height = tfmt->height;
			}

			if (me->pads[0].flags & MEDIA_PAD_FL_SINK) {
				sfmt.pad = me->num_pads - 1;
				ret = v4l2_subdev_call(sd, pad, set_fmt, NULL, &sfmt);
				if (ret) {
					mutex_unlock(&vinc->vid_cap.vdev.entity.graph_obj.mdev->graph_mutex);
					media_graph_walk_cleanup(&graph);
					return ret;
				}
			}
		}
		mutex_unlock(&vinc->vid_cap.vdev.entity.graph_obj.mdev->graph_mutex);
		media_graph_walk_cleanup(&graph);
		if (sfmt.format.code != tfmt->code)
			continue;

		if (ffmt->mbus_code)
			sfmt.format.code = ffmt->mbus_code;

		break;
	}

	if (ffmt)
		*fmt_id = ffmt;
	*tfmt = sfmt.format;

	return 0;
}

static int vin_pipeline_set_mbus_config(struct vin_core *vinc)
{
	struct vin_pipeline *pipe = &vinc->vid_cap.pipe;
	struct v4l2_subdev *sd = pipe->sd[VIN_IND_SENSOR];
	struct v4l2_mbus_config mcfg;
	struct media_entity *me;
	struct media_graph graph;
	struct csi_dev *csi = NULL;
	int ret;

	ret = v4l2_subdev_call(sd, pad, get_mbus_config, 0, &mcfg);
	if (ret < 0) {
		vin_err("%s get_mbus_config error!\n", sd->name);
		goto out;
	}
	/*  s_mbus_config on all mipi and csi  */
	me = &vinc->vid_cap.subdev.entity;
	mutex_lock(&vinc->vid_cap.vdev.entity.graph_obj.mdev->graph_mutex);
	if (media_graph_walk_init(&graph, me->graph_obj.mdev) != 0) {
			mutex_unlock(&vinc->vid_cap.vdev.entity.graph_obj.mdev->graph_mutex);
			return -EINVAL;
	}
	media_graph_walk_start(&graph, me);
	while ((me = media_graph_walk_next(&graph)) &&
		me != &vinc->vid_cap.subdev.entity) {
		sd = media_entity_to_v4l2_subdev(me);
		if ((sd == pipe->sd[VIN_IND_MIPI]) ||
		    (sd == pipe->sd[VIN_IND_CSI])) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
			ret = sd->ops->pad->get_mbus_config(sd, 0, &mcfg);
#else
			ret = sd->ops->pad->set_mbus_config(sd, 0, &mcfg);
#endif
			/* ret = v4l2_subdev_call(sd, pad, set_mbus_config,0, &mcfg); */
			if (ret < 0) {
				vin_err("%s set_mbus_config error!\n", me->name);
				media_graph_walk_cleanup(&graph);
				goto out;
			}
		}
	}
	mutex_unlock(&vinc->vid_cap.vdev.entity.graph_obj.mdev->graph_mutex);
	media_graph_walk_cleanup(&graph);
	csi = v4l2_get_subdevdata(pipe->sd[VIN_IND_CSI]);
	vinc->total_rx_ch = csi->bus_info.ch_total_num;
	vinc->vid_cap.frame.fmt.mbus_type = mcfg.type;
#ifdef SUPPORT_PTN
	if (vinc->ptn_cfg.ptn_en) {
		csi->bus_info.bus_if = V4L2_MBUS_PARALLEL;
		switch (vinc->ptn_cfg.ptn_dw) {
		case 0:
			csi->csi_fmt->data_width = 8;
			break;
		case 1:
			csi->csi_fmt->data_width = 10;
			break;
		case 2:
			csi->csi_fmt->data_width = 12;
			break;
		default:
			csi->csi_fmt->data_width = 12;
			break;
		}
	}
#endif
	return 0;
out:
	return ret;

}

static int vidioc_try_fmt_vid_cap_mplane(struct file *file, void *priv,
					 struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_mbus_framefmt mf;
	struct vin_fmt *ffmt = NULL;

	ffmt = vin_find_format(&f->fmt.pix_mp.pixelformat, NULL,
		VIN_FMT_ALL, -1, false);
	if (ffmt == NULL) {
		vin_err("vin is not support this pixelformat\n");
		return -EINVAL;
	}

	mf.width = f->fmt.pix_mp.width;
	mf.height = f->fmt.pix_mp.height;
	mf.code = ffmt->mbus_code;
	vin_pipeline_try_format(vinc, &mf, &ffmt, true);

	f->fmt.pix_mp.width = mf.width;
	f->fmt.pix_mp.height = mf.height;
	f->fmt.pix_mp.colorspace = mf.colorspace;
	f->fmt.pix_mp.field = V4L2_FIELD_NONE;
	return 0;
}


static int __vin_set_fmt(struct vin_core *vinc, struct v4l2_format *f)
{
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct sensor_win_size win_cfg;
	struct v4l2_mbus_framefmt mf;
	struct vin_fmt *ffmt = NULL;
	struct mbus_framefmt_res *res = (void *)mf.reserved;
	__maybe_unused struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	int ret = 0, i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
	struct v4l2_subdev_state state;
#endif
	struct v4l2_subdev_pad_config cfg;
	struct v4l2_subdev_selection sel;
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;

	if (vin_streaming(cap)) {
		vin_err("%s device busy\n", __func__);
		return -EBUSY;
	}

	ffmt = vin_find_format(&f->fmt.pix_mp.pixelformat, NULL,
					VIN_FMT_ALL, -1, false);
	if (ffmt == NULL) {
		vin_err("vin does not support this pixelformat 0x%x\n",
				f->fmt.pix_mp.pixelformat);
		return -EINVAL;
	}

	cap->frame.fmt = *ffmt;
	mf.width = f->fmt.pix_mp.width;
	mf.height = f->fmt.pix_mp.height;
	mf.field = f->fmt.pix_mp.field;
	mf.colorspace = f->fmt.pix_mp.colorspace;
	mf.code = ffmt->mbus_code;
	res->res_pix_fmt = f->fmt.pix_mp.pixelformat;
	ret = vin_pipeline_try_format(vinc, &mf, &ffmt, true);
	if (ret < 0) {
		vin_err("vin_pipeline_try_format failed\n");
		return -EINVAL;
	}
	cap->frame.fmt.mbus_code = mf.code;
	cap->frame.fmt.field = mf.field;
	cap->frame.fmt.color = mf.colorspace;

	f->fmt.pix_mp.colorspace = mf.colorspace;

	vin_log(VIN_LOG_FMT, "pipeline try fmt %d*%d code %x field %d colorspace %d\n",
		mf.width, mf.height, mf.code, mf.field, mf.colorspace);

	vin_pipeline_set_mbus_config(vinc);

	/* get current win configs */
	memset(&win_cfg, 0, sizeof(win_cfg));
	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl,
			     GET_CURRENT_WIN_CFG, &win_cfg);
#ifdef CSIC_SDRAM_DFS
	if (win_cfg.wdr_mode != ISP_NORMAL_MODE) {
		vinc->vin_dfs.vinc_sdram_status = SDRAM_NOT_USED;
		vin_warn("the sensor is wdr sensor, csic sdeam function is prohibited\n");
	}
#endif
	if (ret == 0) {
		sunxi_isp_sensor_fps(cap->pipe.sd[VIN_IND_ISP], win_cfg.fps_fixed);
#ifdef SUPPORT_ISP_TDM
		if (cap->pipe.sd[VIN_IND_TDM_RX])
			sunxi_tdm_fps_clk(cap->pipe.sd[VIN_IND_TDM_RX], win_cfg.fps_fixed,
						clk_get_rate(vind->clk[VIN_TOP_CLK].clock), win_cfg.vts);
#endif
		vinc->vin_status.width = win_cfg.width;
		vinc->vin_status.height = win_cfg.height;
		vinc->vin_status.h_off = win_cfg.hoffset;
		vinc->vin_status.v_off = win_cfg.voffset;
		/* parser crop */
		memset(&cfg, 0, sizeof(cfg));
		memset(&sel, 0, sizeof(sel));
		cfg.try_crop.width = win_cfg.width;
		cfg.try_crop.height = win_cfg.height;
		cfg.try_crop.left = win_cfg.hoffset;
		cfg.try_crop.top = win_cfg.voffset;
		sel.which = V4L2_SUBDEV_FORMAT_TRY;
		sel.pad = cap->pipe.sd[VIN_IND_CSI]->entity.num_pads - 1;

		if (cap->pipe.sd[VIN_IND_CSI]) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
			memset(&state, 0, sizeof(state));
			state.pads = &cfg;
			ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_CSI], pad,
						set_selection, &state, &sel);
#else
			ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_CSI], pad,
						set_selection, &cfg, &sel);
#endif
			if (ret < 0) {
				vin_err("csi parser set_selection error! code = %d, %d\n", ret, -ENODEV);
				goto out;
			}

		} else {
				vin_err("csi sd is null\n");

		}

		/* vipp crop */
		if ((win_cfg.vipp_hoff != 0) || (win_cfg.vipp_voff != 0)) {
			if ((win_cfg.vipp_w + win_cfg.vipp_hoff > win_cfg.width_input) || (win_cfg.vipp_w == 0))
				win_cfg.vipp_w = win_cfg.width_input - win_cfg.vipp_hoff;
			if ((win_cfg.vipp_h + win_cfg.vipp_voff > win_cfg.height_input) || (win_cfg.vipp_h == 0))
				win_cfg.vipp_h = win_cfg.height_input - win_cfg.vipp_voff;
			sel.target = V4L2_SEL_TGT_CROP;
			sel.pad = SCALER_PAD_SINK;
			sel.which = V4L2_SUBDEV_FORMAT_ACTIVE;
			sel.r.width = win_cfg.vipp_w;
			sel.r.height = win_cfg.vipp_h;
			sel.r.left = win_cfg.vipp_hoff;
			sel.r.top = win_cfg.vipp_voff;
			ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SCALER],
					pad, set_selection, NULL, &sel);
			if (ret < 0) {
				vin_err("vipp set_selection crop error!\n");
				goto out;
			}
		}

		/* vipp shrink */
		if ((win_cfg.vipp_wshrink != 0) && (win_cfg.vipp_hshrink != 0)) {
			sel.target = V4L2_SEL_TGT_CROP;
			sel.pad = SCALER_PAD_SINK;
			sel.which = V4L2_SUBDEV_FORMAT_ACTIVE;
			sel.reserved[0] = VIPP_ONLY_SHRINK;
			sel.r.width = win_cfg.vipp_wshrink;
			sel.r.height = win_cfg.vipp_hshrink;
			sel.r.left = 0;
			sel.r.top = 0;
			ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SCALER],
					pad, set_selection, NULL, &sel);
			if (ret < 0) {
				vin_err("vipp set_selection shrink error!\n");
				goto out;
			}
		}
	} else {
		ret = 0;
		vinc->vin_status.width = mf.width;
		vinc->vin_status.height = mf.height;
		vinc->vin_status.h_off = 0;
		vinc->vin_status.v_off = 0;
		vin_warn("get sensor win_cfg failed!\n");
	}

	if (vinc->vid_cap.frame.fmt.mbus_type == V4L2_MBUS_SUBLVDS ||
	    vinc->vid_cap.frame.fmt.mbus_type == V4L2_MBUS_HISPI) {
		struct combo_sync_code sync;
		struct combo_lane_map map;
		struct combo_wdr_cfg wdr;

		memset(&sync, 0, sizeof(sync));
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core,
				ioctl, GET_COMBO_SYNC_CODE, &sync);
		if (ret < 0) {
			vin_err("get combo sync code error!\n");
			goto out;
		}
		sunxi_combo_set_sync_code(cap->pipe.sd[VIN_IND_MIPI], &sync);

		memset(&map, 0, sizeof(map));
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core,
				ioctl, GET_COMBO_LANE_MAP, &map);
		if (ret < 0) {
			vin_err("get combo lane map error!\n");
			goto out;
		}
		sunxi_combo_set_lane_map(cap->pipe.sd[VIN_IND_MIPI], &map);

		if (res->res_combo_mode & 0xf) {
			memset(&wdr, 0, sizeof(wdr));
			ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core,
					ioctl, GET_COMBO_WDR_CFG, &wdr);
			if (ret < 0) {
				vin_err("get combo wdr cfg error!\n");
				goto out;
			}
			sunxi_combo_wdr_config(cap->pipe.sd[VIN_IND_MIPI], &wdr);
		}
	}
	cap->isp_wdr_mode = res->res_wdr_mode;

	if (cap->capture_mode == V4L2_MODE_IMAGE) {
		sunxi_flash_check_to_start(cap->pipe.sd[VIN_IND_FLASH],
					   SW_CTRL_FLASH_ON);
	} else {
		sunxi_flash_stop(vinc->vid_cap.pipe.sd[VIN_IND_FLASH]);
	}

	if ((mf.width < f->fmt.pix_mp.width) || (mf.height < f->fmt.pix_mp.height)) {
		f->fmt.pix_mp.width = mf.width;
		f->fmt.pix_mp.height = mf.height;
	}
	/* for csi dma size set */
	cap->frame.offs_h = (mf.width - f->fmt.pix_mp.width) / 2;
	cap->frame.offs_v = (mf.height - f->fmt.pix_mp.height) / 2;
	cap->frame.o_width = f->fmt.pix_mp.width;
	cap->frame.o_height = f->fmt.pix_mp.height;

	pixm->colorspace = vinc->vid_cap.frame.fmt.color;
	pixm->num_planes = vinc->vid_cap.frame.fmt.memplanes;

	for (i = 0; i < pixm->num_planes; ++i) {
		pixm->plane_fmt[i].bytesperline = f->fmt.pix_mp.width * cap->frame.fmt.depth[i] / 8;
		pixm->plane_fmt[i].sizeimage = f->fmt.pix_mp.width * f->fmt.pix_mp.height * cap->frame.fmt.depth[i] / 8;
	}

out:
	return ret;
}


static int vidioc_s_fmt_vid_cap_mplane(struct file *file, void *priv,
				struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);
	int ret;
	__maybe_unused struct vin_core *vinc_bind = NULL;

	if (vinc->dma_merge_mode == 1) {
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			vinc_bind = vin_core_gbl[vinc->id - 1];
			ret = __vin_set_fmt(vinc_bind, f);
			if (ret < 0) {
				vin_err("set fmt0 error");
				return ret;
			}
		}
	}

	ret = __vin_set_fmt(vinc, f);
	if (ret < 0)
		vin_err("set fmt error");

	return ret;
}

int vidioc_s_selection(struct file *file, void *fh,
				struct v4l2_selection *s)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_subdev_selection sel;
	int ret = 0;
	__maybe_unused struct vin_core *vinc_bind = NULL;
	__maybe_unused struct v4l2_subdev_selection sel_bind;

	if (vinc->dma_merge_mode == 1) {
		sel_bind.which = V4L2_SUBDEV_FORMAT_ACTIVE;
		sel_bind.pad = SCALER_PAD_SINK;
		sel_bind.target = s->target;
		sel_bind.flags = s->flags;
		sel_bind.r = s->r;
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			vinc_bind = vin_core_gbl[vinc->id - 1];
			ret = v4l2_subdev_call(vinc_bind->vid_cap.pipe.sd[VIN_IND_SCALER], pad,
						set_selection, NULL, &sel_bind);
			if (ret < 0) {
				vin_err("v4l2 sub device scaler set_selection error!\n");
				return ret;
			}
		}
	}
	sel.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	sel.pad = SCALER_PAD_SINK;
	sel.target = s->target;
	sel.flags = s->flags;
	sel.r = s->r;
	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SCALER], pad,
				set_selection, NULL, &sel);
	if (ret < 0)
		vin_err("v4l2 sub device scaler set_selection error!\n");
	return ret;
}
int vidioc_g_selection(struct file *file, void *fh,
				struct v4l2_selection *s)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_subdev_selection sel;
	int ret = 0;

	sel.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	sel.pad = SCALER_PAD_SINK;
	sel.target = s->target;
	sel.flags = s->flags;
	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SCALER], pad,
				get_selection, NULL, &sel);
	if (ret < 0)
		vin_err("v4l2 sub device scaler get_selection error!\n");
	else
		s->r = sel.r;
	return ret;

}

static int vidioc_enum_fmt_vid_overlay(struct file *file, void *__fh,
					    struct v4l2_fmtdesc *f)
{
	struct vin_fmt *fmt;

	fmt = vin_find_format(NULL, NULL, VIN_FMT_OSD, f->index, false);
	if (!fmt)
		return -EINVAL;

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

static int vidioc_g_fmt_vid_overlay(struct file *file, void *__fh,
					struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);

	f->fmt.win.w.left = 0;
	f->fmt.win.w.top = 0;
	f->fmt.win.w.width = vinc->vid_cap.frame.o_width;
	f->fmt.win.w.height = vinc->vid_cap.frame.o_height;
	f->fmt.win.clipcount = vinc->vid_cap.osd.overlay_cnt;
	f->fmt.win.chromakey = vinc->vid_cap.osd.chromakey;

	return 0;
}
static void __osd_win_check(struct v4l2_window *win)
{
	if (win->w.width > MAX_WIDTH)
		win->w.width = MAX_WIDTH;
	if (win->w.width < MIN_WIDTH)
		win->w.width = MIN_WIDTH;
	if (win->w.height > MAX_HEIGHT)
		win->w.height = MAX_HEIGHT;
	if (win->w.height < MIN_HEIGHT)
		win->w.height = MIN_HEIGHT;

	if (win->bitmap) {
		if (win->clipcount > MAX_OVERLAY_NUM)
			win->clipcount = MAX_OVERLAY_NUM;
	} else {
		if (MAX_ORL_NUM) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
			if (win->clipcount / 2 > MAX_ORL_NUM)
				win->clipcount = MAX_ORL_NUM;
			else
				win->clipcount /= 2;
#else
			if (win->clipcount > MAX_ORL_NUM)
				win->clipcount = MAX_ORL_NUM;
#endif
		} else {
			if (win->clipcount > MAX_COVER_NUM)
				win->clipcount = MAX_COVER_NUM;
		}
	}
}

static int vidioc_try_fmt_vid_overlay(struct file *file, void *__fh,
					struct v4l2_format *f)
{
	__osd_win_check(&f->fmt.win);

	return 0;
}

void __osd_bitmap2dram(struct vin_osd *osd, void *databuf)
{
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1)
	int i, j, k, m = 0, n = 0;
	int kend = 0, idx = 0, ww = 0, ysn = 0;
	int y_num = 0, *y_temp = NULL;
	int *hor_num = NULL, *hor_index = NULL;
	int *x_temp = NULL, *xbuf = NULL, *x_idx = NULL;
	int addr_offset = 0, pix_size = osd->fmt->depth[0]/8;
	int cnt = osd->overlay_cnt;
	void *dram_addr = osd->ov_mask[osd->ov_set_cnt % 2].vir_addr;

	y_temp = (int *)kzalloc(2 * cnt * sizeof(int), GFP_KERNEL);
	for (i = 0; i < cnt; i++) {
		y_temp[i] = osd->ov_win[i].top;
		y_temp[i + cnt] = osd->ov_win[i].top + osd->ov_win[i].height - 1;
	}
	sort(y_temp, 2 * cnt, sizeof(int), vin_cmp, vin_swap);
	y_num = vin_unique(y_temp, 2 * cnt);
	hor_num = (int *)kzalloc(y_num * sizeof(int), GFP_KERNEL); /* 0~y_num-1 */
	hor_index = (int *)kzalloc(y_num * cnt * sizeof(int), GFP_KERNEL); /* (0~y_num-1) * (0~N+1) */

	for (j = 0; j < y_num; j++) {
		ysn = 0;
		for (i = 0; i < cnt; i++) {
			if (osd->ov_win[i].top <= y_temp[j] &&
			   (osd->ov_win[i].top + osd->ov_win[i].height) > y_temp[j]) {
				hor_num[j]++;
				hor_index[j * cnt + ysn] = i;
				ysn = ysn + 1;
			}
		}
	}

	for (j = 0; j < y_num; j++) {
		x_temp = (int *)kzalloc(hor_num[j] * sizeof(int), GFP_KERNEL);
		xbuf = (int *)kzalloc(hor_num[j] * sizeof(int), GFP_KERNEL);
		x_idx = (int *)kzalloc(hor_num[j] * sizeof(int), GFP_KERNEL);
		for (k = 0; k < hor_num[j]; k++)
			x_temp[k] = osd->ov_win[hor_index[j * cnt + k]].left;
		memcpy(xbuf, x_temp, hor_num[j] * sizeof(int));
		sort(x_temp, hor_num[j], sizeof(int), vin_cmp, vin_swap);

		for (k = 0; k < hor_num[j]; k++)	{
			for (m = 0; m < hor_num[j]; m++) {
				if (x_temp[k] == xbuf[m]) {
					x_idx[k] = m;
					break;
				}
			}
		}

		if (j == y_num - 1)
			kend = y_temp[j];
		else
			kend = y_temp[j + 1] - 1;
		for (k = y_temp[j]; k <= kend; k++) {
			for (i = 0; i < hor_num[j]; i++)	{
				idx = hor_index[j * cnt + x_idx[i]];
				addr_offset = 0;
				for (n = 0; n < idx; n++)
					addr_offset +=	(osd->ov_win[n].width * osd->ov_win[n].height) * pix_size;
				ww = osd->ov_win[idx].width;
				if (k < (osd->ov_win[idx].top + osd->ov_win[idx].height)) {
					memcpy(dram_addr, databuf + addr_offset
						+ ww * (k - osd->ov_win[idx].top) * pix_size,
						ww * pix_size);
					dram_addr += ww * pix_size;
				}
			}
		}
		kfree(x_temp);
		kfree(xbuf);
		kfree(x_idx);
		x_temp = NULL;
		xbuf = NULL;
		x_idx = NULL;
	}
	kfree(hor_num);
	kfree(hor_index);
	kfree(y_temp);
	y_temp = NULL;
	hor_index = NULL;
	hor_num = NULL;
#else
	memcpy(osd->ov_mask[osd->ov_set_cnt % 2].vir_addr, databuf, osd->ov_mask[osd->ov_set_cnt % 2].size);
#endif
}

static void __osd_rgb_to_yuv(u8 r, u8 g, u8 b, u8 *y, u8 *u, u8 *v)
{
	int jc0	= 0x00000132;
	int jc1 = 0x00000259;
	int jc2 = 0x00000075;
	int jc3 = 0xffffff53;
	int jc4 = 0xfffffead;
	int jc5 = 0x00000200;
	int jc6 = 0x00000200;
	int jc7 = 0xfffffe53;
	int jc8 = 0xffffffad;
	int jc9 = 0x00000000;
	int jc10 = 0x00000080;
	int jc11 = 0x00000080;
	u32 y_tmp, u_tmp, v_tmp;

	y_tmp = (((jc0 * r >> 6) + (jc1 * g >> 6) + (jc2 * b >> 6)) >> 4) + jc9;
	*y = clamp_val(y_tmp, 0, 255);

	u_tmp = (((jc3 * r >> 6) + (jc4 * g >> 6) + (jc5 * b >> 6)) >> 4) + jc10;
	*u = clamp_val(u_tmp, 0, 255);

	v_tmp = (((jc6 * r >> 6) + (jc7 * g >> 6) + (jc8 * b >> 6)) >> 4) + jc11;
	*v = clamp_val(v_tmp, 0, 255);
}

static void __osd_bmp_to_yuv(struct vin_osd *osd, void *databuf)
{
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1)
	u8 alpha, r, g, b, y, u, v;
	int i, j, y_sum, bmp;

	for (i = 0; i < osd->overlay_cnt; i++) {
		int bmp_size = osd->ov_win[i].height * osd->ov_win[i].width;
		int valid_pix = 1;

		y_sum = 0;
		for (j = 0; j < bmp_size; j++) {
			switch (osd->overlay_fmt) {
			case 0:
				bmp = *(short *)databuf;
				alpha = (int)((bmp >> 15) & 0x01) * 100;
				r = (bmp >> 10) & 0x1f;
				r = (r << 3) + (r >> 2);
				g = (bmp >> 5) & 0x1f;
				g = (g << 3) + (g >> 2);
				b = bmp & 0x1f;
				b = (b << 3) + (b >> 2);
				databuf += 2;
				break;
			case 1:
				bmp = *(short *)databuf;
				alpha = (int)((bmp >> 12) & 0x0f) * 100 / 15;
				r = (bmp >> 8) & 0x0f;
				r = (r << 4) + r;
				g = (bmp >> 4) & 0x0f;
				g = (g << 4) + g;
				b = bmp & 0x0f;
				b = (b << 4) + b;
				databuf += 2;
				break;
			case 2:
				bmp = *(int *)databuf;
				alpha = (int)((bmp >> 24) & 0xff) * 100 / 255;
				r = (bmp >> 16) & 0xff;
				g = (bmp >> 8) & 0xff;
				b = bmp & 0xff;
				databuf += 4;
				break;
			default:
				bmp = *(int *)databuf;
				alpha = (int)((bmp >> 24) & 0xff) * 100 / 255;
				r = (bmp >> 16) & 0xff;
				g = (bmp >> 8) & 0xff;
				b = bmp & 0xff;
				databuf += 4;
				break;
			}
			if (alpha >= 80) {
				__osd_rgb_to_yuv(r, g, b, &y, &u, &v);
				y_sum += y;
				valid_pix++;
			}
		}
		osd->y_bmp_avp[i] = y_sum / valid_pix;
	}
#endif
}

static int vidioc_s_fmt_vid_overlay(struct file *file, void *__fh,
					struct v4l2_format *f)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_osd *osd = &vinc->vid_cap.osd;
	struct v4l2_clip *clip = NULL;
	void *bitmap = NULL;
	unsigned int bitmap_size = 0, pix_size = 0;
	int ret = 0, i = 0;

	__osd_win_check(&f->fmt.win);

	osd->chromakey = f->fmt.win.chromakey;

	if (f->fmt.win.bitmap) {
		if (f->fmt.win.clipcount <= 0) {
			osd->overlay_en = 0;
			goto osd_reset;
		} else {
			if (MAX_OVERLAY_NUM) {
				osd->overlay_en = 1;
				osd->overlay_cnt = f->fmt.win.clipcount;
			} else {
				osd->overlay_en = 0;
				vin_err("VIPP overlay is not exist!!\n");
				goto osd_reset;
			}
		}

		clip = vmalloc(sizeof(*clip) * osd->overlay_cnt * 2);
		if (clip == NULL) {
			vin_err("%s - Alloc of clip mask failed\n", __func__);
			return -ENOMEM;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		memcpy(clip, f->fmt.win.clips, sizeof(*clip) * f->fmt.win.clipcount * 2);
#else
		if (copy_from_user(clip, f->fmt.win.clips,
			sizeof(*clip) * osd->overlay_cnt * 2)) {
			vfree(clip);
			return -EFAULT;
		}
#endif

		/* save global alpha in the win top for diff overlay */
		for (i = 0; i < osd->overlay_cnt; i++) {
			osd->ov_win[i] = clip[i].c;
			bitmap_size += clip[i].c.width * clip[i].c.height;
			if (f->fmt.win.global_alpha == 255)
				osd->global_alpha[i] = clamp_val(clip[i + osd->overlay_cnt].c.top, 0, 16);
			else
				osd->global_alpha[i] = clamp_val(f->fmt.win.global_alpha, 0, 16);
			osd->inverse_close[i] = clip[i + osd->overlay_cnt].c.left & 0xff;
			osd->inv_th = (clip[i + osd->overlay_cnt].c.left >> 8) & 0xff;
			osd->inv_w_rgn[i] = clamp_val(clip[i + osd->overlay_cnt].c.width, 0, 15);
			osd->inv_h_rgn[i] = clamp_val(clip[i + osd->overlay_cnt].c.height, 0, 15);
		}
		vfree(clip);

		osd->fmt = vin_find_format(&f->fmt.win.chromakey, NULL,
				VIN_FMT_OSD, -1, false);
		if (osd->fmt == NULL) {
			vin_err("osd is not support this chromakey\n");
			return -EINVAL;
		}
		pix_size = osd->fmt->depth[0]/8;

		bitmap = vmalloc(bitmap_size * pix_size);
		if (bitmap == NULL) {
			vin_err("%s - Alloc of bitmap buf failed\n", __func__);
			return -ENOMEM;
		}
		if (copy_from_user(bitmap, f->fmt.win.bitmap,
				bitmap_size * pix_size)) {
			vfree(bitmap);
			return -EFAULT;
		}

		osd->ov_set_cnt++;

		if (osd->ov_mask[osd->ov_set_cnt % 2].size != bitmap_size * pix_size) {
			if (osd->ov_mask[osd->ov_set_cnt % 2].phy_addr) {
				os_mem_free(&vinc->pdev->dev, &osd->ov_mask[osd->ov_set_cnt % 2]);
				osd->ov_mask[osd->ov_set_cnt % 2].phy_addr = NULL;
			}
			osd->ov_mask[osd->ov_set_cnt % 2].size = bitmap_size * pix_size;
			ret = os_mem_alloc(&vinc->pdev->dev, &osd->ov_mask[osd->ov_set_cnt % 2]);
			if (ret < 0) {
				vin_err("osd bitmap load addr requset failed!\n");
				vfree(bitmap);
				return -ENOMEM;
			}
		}
		memset(osd->ov_mask[osd->ov_set_cnt % 2].vir_addr, 0, bitmap_size * pix_size);
		__osd_bitmap2dram(osd, bitmap);

		switch (osd->chromakey) {
		case V4L2_PIX_FMT_RGB555:
			osd->overlay_fmt = ARGB1555;
			break;
		case V4L2_PIX_FMT_RGB444:
			osd->overlay_fmt = ARGB4444;
			break;
		case V4L2_PIX_FMT_RGB32:
			osd->overlay_fmt = ARGB8888;
			break;
		default:
			osd->overlay_fmt = ARGB8888;
			break;
		}
		__osd_bmp_to_yuv(osd, bitmap);
		vfree(bitmap);
	} else {
		if (f->fmt.win.clipcount <= 0) {
			osd->cover_en = 0;
			osd->orl_en = 0;
			goto osd_reset;
		}

		clip = vmalloc(sizeof(*clip) * f->fmt.win.clipcount * 2);
		if (clip == NULL) {
			vin_err("%s - Alloc of clip mask failed\n", __func__);
			return -ENOMEM;
		}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
		memcpy(clip, f->fmt.win.clips, sizeof(*clip) * f->fmt.win.clipcount * 2);
#else
		if (copy_from_user(clip, f->fmt.win.clips,
			sizeof(*clip) * f->fmt.win.clipcount * 2)) {
			vfree(clip);
			return -EFAULT;
		}
#endif

		/* save rgb in the win top for diff cover */
		osd->orl_width = clip[f->fmt.win.clipcount].c.width;
		if (osd->orl_width) {
			if (MAX_ORL_NUM) {
				osd->orl_en = 1;
				osd->orl_cnt = f->fmt.win.clipcount;
			} else {
				osd->orl_en = 0;
				vin_err("VIPP orl is not exist!!\n");
				goto osd_reset;
			}
		} else {
			if (MAX_COVER_NUM) {
				osd->cover_en = 1;
				osd->cover_cnt = f->fmt.win.clipcount;
			} else {
				osd->cover_en = 0;
				vin_err("VIPP cover is not exist!!\n");
				goto osd_reset;
			}
		}

		if (osd->orl_en) {
			for (i = 0; i < osd->orl_cnt; i++) {
				u8 r, g, b;

				osd->orl_win[i] = clip[i].c;
				osd->rgb_orl[i] = clip[i + osd->orl_cnt].c.top;

				r = (osd->rgb_orl[i] >> 16) & 0xff;
				g = (osd->rgb_orl[i] >> 8) & 0xff;
				b = osd->rgb_orl[i] & 0xff;
				__osd_rgb_to_yuv(r, g, b, &osd->yuv_orl[0][i],
					&osd->yuv_orl[1][i], &osd->yuv_orl[2][i]);
			}
		}

		if (osd->cover_en) {
			for (i = 0; i < osd->cover_cnt; i++) {
				u8 r, g, b;

				osd->cv_win[i] = clip[i].c;
				osd->rgb_cover[i] = clip[i + osd->cover_cnt].c.top;

				r = (osd->rgb_cover[i] >> 16) & 0xff;
				g = (osd->rgb_cover[i] >> 8) & 0xff;
				b = osd->rgb_cover[i] & 0xff;
				__osd_rgb_to_yuv(r, g, b, &osd->yuv_cover[0][i],
					&osd->yuv_cover[1][i], &osd->yuv_cover[2][i]);

			}
		}

		vfree(clip);
	}
osd_reset:
	osd->is_set = 0;

	return ret;
}

static int __osd_reg_setup(struct vin_core *vinc, struct vin_osd *osd)
{
	struct vipp_osd_config *osd_cfg = NULL;
	struct vipp_osd_para_config *para = NULL;
	struct vipp_rgb2yuv_factor rgb2yuv_def = {
		.jc0 = 0x00000132,
		.jc1 = 0x00000259,
		.jc2 = 0x00000075,
		.jc3 = 0xffffff53,
		.jc4 = 0xfffffead,
		.jc5 = 0x00000200,
		.jc6 = 0x00000200,
		.jc7 = 0xfffffe53,
		.jc8 = 0xffffffad,
		.jc9 = 0x00000000,
		.jc10 = 0x00000080,
		.jc11 = 0x00000080,
	};
	struct scaler_dev *scaler = container_of(vinc->vid_cap.pipe.sd[VIN_IND_SCALER], struct scaler_dev, subdev);
	int id = vinc->vipp_sel;
	int i;
	int act_width;

	osd_cfg = kzalloc(sizeof(*osd_cfg), GFP_KERNEL);
	if (osd_cfg == NULL) {
		vin_err("%s - Alloc of osd_cfg failed\n", __func__);
		return -ENOMEM;
	}

	para = kzalloc(sizeof(*para), GFP_KERNEL);
	if (para == NULL) {
		vin_err("%s - Alloc of osd_para failed\n", __func__);
		kfree(osd_cfg);
		return -ENOMEM;
	}

	if (osd->overlay_en == 1) {
		osd_cfg->osd_argb_mode = osd->overlay_fmt;
		osd_cfg->osd_ov_num = osd->overlay_cnt - 1;
		osd_cfg->osd_ov_en = 1;
		osd_cfg->osd_stat_en = 1;

		for (i = 0; i < osd->overlay_cnt; i++) {
			if (vinc->hflip)
				para->overlay_cfg[i].h_start = vinc->vid_cap.frame.o_width - osd->ov_win[i].width - osd->ov_win[i].left;
			else
				para->overlay_cfg[i].h_start = osd->ov_win[i].left;
			para->overlay_cfg[i].h_end = para->overlay_cfg[i].h_start + osd->ov_win[i].width - 1;

			if (vinc->vflip)
				para->overlay_cfg[i].v_start = vinc->vid_cap.frame.o_height - osd->ov_win[i].height - osd->ov_win[i].top;
			else
				para->overlay_cfg[i].v_start = osd->ov_win[i].top;
			para->overlay_cfg[i].v_end = para->overlay_cfg[i].v_start + osd->ov_win[i].height - 1;

			para->overlay_cfg[i].alpha = osd->global_alpha[i];
			para->overlay_cfg[i].inv_en = !osd->inverse_close[i];
			para->overlay_cfg[i].inv_th = osd->inv_th;
			para->overlay_cfg[i].inv_w_rgn = osd->inv_w_rgn[i];
			para->overlay_cfg[i].inv_h_rgn = osd->inv_h_rgn[i];
		}
		vipp_set_osd_bm_load_addr(id, (unsigned long)osd->ov_mask[osd->ov_set_cnt % 2].dma_addr);
	} else {
		osd_cfg->osd_ov_num = -1;
	}

	if (osd->cover_en == 1) {
		osd_cfg->osd_cv_num = osd->cover_cnt - 1;
		osd_cfg->osd_cv_en = 1;

		for (i = 0; i < osd->cover_cnt; i++) {
			if (vinc->hflip)
				para->cover_cfg[i].h_start = vinc->vid_cap.frame.o_width - osd->cv_win[i].width - osd->cv_win[i].left;
			else
				para->cover_cfg[i].h_start = osd->cv_win[i].left;
			para->cover_cfg[i].h_end = para->cover_cfg[i].h_start + osd->cv_win[i].width - 1;

			if (vinc->vflip)
				para->cover_cfg[i].v_start = vinc->vid_cap.frame.o_height - osd->cv_win[i].height - osd->cv_win[i].top;
			else
				para->cover_cfg[i].v_start = osd->cv_win[i].top;
			para->cover_cfg[i].v_end = para->cover_cfg[i].v_start + osd->cv_win[i].height - 1;

			para->cover_data[i].y = osd->yuv_cover[0][i];
			para->cover_data[i].u = osd->yuv_cover[1][i];
			para->cover_data[i].v = osd->yuv_cover[2][i];
		}
	} else {
		osd_cfg->osd_cv_num = -1;
	}

	if (osd->orl_en == 1) {
		osd_cfg->osd_orl_num = osd->orl_cnt - 1;
		osd_cfg->osd_orl_en = 1;
		osd_cfg->osd_orl_width = osd->orl_width;
		act_width = 2 * (osd_cfg->osd_orl_width + 1);
		for (i = 0; i < osd->orl_cnt; i++) {
			if (osd->orl_win[i].height < 2 * act_width)
				osd->orl_win[i].height = 2 * act_width;
			if (osd->orl_win[i].width < 2 * act_width)
				osd->orl_win[i].width = 2 * act_width;

			if (vinc->hflip)
				para->orl_cfg[i].h_start = vinc->vid_cap.frame.o_width - osd->orl_win[i].width - osd->orl_win[i].left;
			else
				para->orl_cfg[i].h_start = osd->orl_win[i].left;
			para->orl_cfg[i].h_end = para->orl_cfg[i].h_start + osd->orl_win[i].width - 1;

			if (vinc->vflip)
				para->orl_cfg[i].v_start = vinc->vid_cap.frame.o_height - osd->orl_win[i].height - osd->orl_win[i].top;
			else
				para->orl_cfg[i].v_start = osd->orl_win[i].top;
			para->orl_cfg[i].v_end = para->orl_cfg[i].v_start + osd->orl_win[i].height - 1;

			para->orl_data[i].y = osd->yuv_orl[0][i];
			para->orl_data[i].u = osd->yuv_orl[1][i];
			para->orl_data[i].v = osd->yuv_orl[2][i];
		}
	} else {
		osd_cfg->osd_orl_num = -1;
	}

	if (!scaler->noneed_register) {
		vipp_osd_cfg(id, osd_cfg);
		vipp_osd_rgb2yuv(id, &rgb2yuv_def);
		vipp_osd_para_cfg(id, para, osd_cfg);
		vipp_osd_hvflip(id, vinc->hflip, vinc->vflip);
#if defined VIPP_200
		vipp_clear_status(id,
			CHN0_REG_LOAD_PD << (vipp_virtual_find_ch[id] * VIPP_CHN_INT_AMONG_OFFSET));
		vipp_irq_enable(id,
			CHN0_REG_LOAD_EN << (vipp_virtual_find_ch[id] * VIPP_CHN_INT_AMONG_OFFSET));
#endif
	}
	kfree(osd_cfg);
	kfree(para);
	return 0;
}

static int vidioc_overlay(struct file *file, void *__fh, unsigned int on)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_osd *osd = &vinc->vid_cap.osd;
	int i;
	int ret = 0;

	if (!on) {
		for (i = 0; i < 2; i++) {
			if (osd->ov_mask[i].phy_addr) {
				os_mem_free(&vinc->pdev->dev, &osd->ov_mask[i]);
				osd->ov_mask[i].phy_addr = NULL;
				osd->ov_mask[i].size = 0;
			}
		}
		osd->ov_set_cnt = 0;
		osd->overlay_en = 0;
		osd->cover_en = 0;
		osd->orl_en = 0;
	} else {
		if (osd->is_set)
			return ret;
	}

	ret = __osd_reg_setup(vinc, osd);
	osd->is_set = 1;
	return ret;
}

int vin_timer_init(struct vin_core *vinc)
{
	return 0;
}

void vin_timer_del(struct vin_core *vinc)
{

}

void vin_timer_update(struct vin_core *vinc, int ms)
{

}

static int __vin_sensor_setup_link(struct vin_core *vinc, struct modules_config *module,
					int i, int en)
{
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct v4l2_subdev *sensor = module->modules.sensor[i].sd;
	struct v4l2_subdev *subdev;
	struct media_entity *entity = NULL;
	struct media_link *link = NULL;
	__maybe_unused struct sensor_info *info = to_state(sensor);
	int ret;

	if (sensor == NULL)
		return -1;

	if (vinc->mipi_sel != 0xff)
		subdev = vind->mipi[vinc->mipi_sel].sd;
	else
		subdev = vind->csi[vinc->csi_sel].sd;

	entity = &sensor->entity;
	list_for_each_entry(link, &entity->links, list) {
		if (link->source->entity == entity && link->sink->entity == &subdev->entity)
				break;
	}
	if (link == NULL)
		return -1;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (info->stream_count >= 1)
		return 0;
#else
	if (sensor->entity.stream_count >= 1)
		return 0;
#endif
	vin_log(VIN_LOG_VIDEO, "setup link: [%s] %c> [%s]\n",
		sensor->name, en ? '=' : '-', link->sink->entity->name);
	if (en)
		ret = media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
	else
		ret = __media_entity_setup_link(link, 0);
		/* When the this function is called by the close
		function, the mutex conflicts with the close mutex,
		so the function without the mutex is used. */

	if (ret) {
		vin_warn("%s setup link %s fail!\n", sensor->name,
				link->sink->entity->name);
		return -1;
	}
	return 0;
}

static int __csi_isp_setup_link(struct vin_core *vinc, int en)
{
#ifndef SUPPORT_ISP_TDM
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct v4l2_subdev *csi, *isp;
	struct media_link *link = NULL;
	__maybe_unused struct csi_dev *csi_dev = NULL;
	int ret;

	/* CSI */
	if (vinc->csi_sel == 0xff)
		csi = NULL;
	else
		csi = vind->csi[vinc->csi_sel].sd;

	/* ISP */
	if (vinc->isp_sel == 0xff)
		isp = NULL;
	else
		isp = vind->isp[vinc->isp_sel].sd;

	if (csi && isp) {
		link = media_entity_find_link(&csi->entity.pads[CSI_PAD_SOURCE],
					  &isp->entity.pads[ISP_PAD_SINK]);
	}
	if (link == NULL) {
		vin_err("%s:media_entity_find_link null\n", __func__);
		return -1;
	}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	csi_dev = v4l2_get_subdevdata(csi);
	if (csi_dev->stream_count >= 1)
		return 0;
#else
	if (csi->entity.stream_count >= 1)
		return 0;
#endif
	vin_log(VIN_LOG_MD, "link: source %s sink %s\n",
			link->source->entity->name,
			link->sink->entity->name);
	if (en)
		ret = __media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
	else
		ret = __media_entity_setup_link(link, 0);
		/* When the this function is called by the close
		function, the mutex conflicts with the close mutex,
		so the function without the mutex is used. */
	if (ret) {
		vin_warn("%s setup link %s fail!\n", link->source->entity->name,
									link->sink->entity->name);
		return -1;
	}
#else
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct v4l2_subdev *csi, *tdm_rx;
	struct media_link *link = NULL;
	__maybe_unused struct tdm_rx_dev *tdm_dev = NULL;
	int ret;

	/*CSI*/
	if (vinc->csi_sel == 0xff)
		csi = NULL;
	else
		csi = vind->csi[vinc->csi_sel].sd;

	/*TDM_RX*/
	if (vinc->tdm_rx_sel == 0xff)
		tdm_rx = NULL;
	else
		tdm_rx = vind->tdm[vinc->tdm_rx_sel/TDM_RX_NUM].tdm_rx[vinc->tdm_rx_sel].sd;

	if (csi && tdm_rx)
		link = media_entity_find_link(&csi->entity.pads[CSI_PAD_SOURCE],
						  &tdm_rx->entity.pads[ISP_PAD_SINK]);
	if (link == NULL) {
		vin_err("%s:media_entity_find_link null\n", __func__);
		return -1;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	tdm_dev = v4l2_get_subdevdata(tdm_rx);
	if (tdm_dev->stream_count >= 1)
		return 0;
#else
	if (tdm_rx->entity.stream_count >= 1)
		return 0;
#endif

	vin_log(VIN_LOG_MD, "setup link: [%s] %c> [%s]\n",
			link->source->entity->name, en ? '=' : '-',
			link->sink->entity->name);
	if (en)
		ret = __media_entity_setup_link(link, MEDIA_LNK_FL_ENABLED);
	else
		ret = __media_entity_setup_link(link, 0);
		/* When the this function is called by the close
		function, the mutex conflicts with the close mutex,
		so the function without the mutex is used. */
	if (ret) {
		vin_warn("%s setup link %s fail!\n", link->source->entity->name,
									link->sink->entity->name);
		return -1;
	}
#endif
	return 0;
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct modules_config *module = &vind->modules[vinc->sensor_sel];
	int valid_idx = module->sensors.valid_idx;
	int ret = 0;
	__maybe_unused struct vin_core *vinc_bind = NULL;

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = -EINVAL;
		goto streamon_error;
	}

	if (vin_streaming(cap)) {
		vin_err("video%d has already stream on\n", vinc->id);
		ret = -1;
		goto streamon_error;
	}

	ret = vb2_ioctl_streamon(file, priv, i);
	if (ret)
		goto streamon_error;

	if (vinc->large_image == 1) {
#ifdef SUPPORT_PTN
		vinc->ptn_cfg.ptn_w = cap->frame.o_width;
		vinc->ptn_cfg.ptn_h = cap->frame.o_height;
		vinc->ptn_cfg.ptn_mode = 12;
		vinc->ptn_cfg.ptn_buf.size = cap->buf_byte_size;
		os_mem_alloc(&vinc->pdev->dev, &vinc->ptn_cfg.ptn_buf);
		if (vinc->ptn_cfg.ptn_buf.vir_addr == NULL) {
			vin_err("ptn buffer 0x%x alloc failed!\n", cap->buf_byte_size);
			return -ENOMEM;
		}
		csic_dma_buffer_address(vinc->vipp_sel, CSI_BUF_0_A, (unsigned long)vinc->ptn_cfg.ptn_buf.dma_addr);
#endif
	}

#if VIN_FALSE
	schedule_work(&vinc->vid_cap.s_stream_task);
#else
	if (vin_lpm(cap)) {
		if (__vin_sensor_setup_link(vinc, module, valid_idx, 1) < 0) {
			vin_err("sensor setup link failed\n");
			return -EINVAL;
		}
		if (__csi_isp_setup_link(vinc, 1) < 0) {
			vin_err("csi&isp setup link failed\n");
			return -EINVAL;
		}
		clear_bit(VIN_LPM, &cap->state);
	}

	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	vin_timer_init(cap->vinc);

	if (vinc->dma_merge_mode == 1) {
		vin_set_next_buf_addr(cap->vinc);
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			vinc_bind = vin_core_gbl[vinc->id - 1];
			ret = vin_pipeline_call(vinc_bind, set_stream, &vinc_bind->vid_cap.pipe, vinc_bind->stream_idx);
			if (ret < 0)
				vin_err("video%d %s error!\n", vinc_bind->id, __func__);
			set_bit(VIN_STREAM, &vinc_bind->vid_cap.state);
		}
	}

	ret = vin_pipeline_call(cap->vinc, set_stream, &cap->pipe, cap->vinc->stream_idx);
	if (ret < 0)
		vin_err("video%d %s error!\n", vinc->id, __func__);
	set_bit(VIN_STREAM, &cap->state);

	/* set saved exp and gain for reopen, you can call the api in sensor_reg_init */
	/*
	if (cap->vinc->exp_gain.exp_val && cap->vinc->exp_gain.gain_val) {
		v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl,
			VIDIOC_VIN_SENSOR_EXP_GAIN, &cap->vinc->exp_gain);
	}
	 */
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
#if VIN_FALSE
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
	if (isp->gtm_type == 4) {
		if ((vinc->id == 0) && (!check_ldci_video_relate(vinc->id, LDCI0_VIDEO_CHN))) {
			enable_ldci_video(LDCI0_VIDEO_CHN);
		} else if ((vinc->id == 1) && (!check_ldci_video_relate(vinc->id, LDCI1_VIDEO_CHN))) {
			enable_ldci_video(LDCI1_VIDEO_CHN);
		}
	}
#endif
#endif
#endif
streamon_error:

	return ret;
}

static void vin_queue_free(struct file *file)
{
	struct video_device *vdev = video_devdata(file);

	if (file->private_data == vdev->queue->owner) {
		vb2_queue_release(vdev->queue);
		vdev->queue->owner = NULL;
	}

}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct modules_config *module = &vind->modules[vinc->sensor_sel];
	int valid_idx = module->sensors.valid_idx;
	int ret = 0;
	__maybe_unused struct vin_core *vinc_bind = NULL;
	if (!vin_streaming(cap)) {
		vin_err("video%d has already stream off\n", vinc->id);
		goto streamoff_error;
	}
	vin_timer_del(vinc);

#if VIN_FALSE
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
	if (isp->gtm_type == 4) {
		if ((vinc->id == 0) && (!check_ldci_video_relate(vinc->id, LDCI0_VIDEO_CHN))) {
			disable_ldci_video(LDCI0_VIDEO_CHN);
		} else if ((vinc->id == 1) && (!check_ldci_video_relate(vinc->id, LDCI1_VIDEO_CHN))) {
			disable_ldci_video(LDCI1_VIDEO_CHN);
		}
	}
#endif
#endif

	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	clear_bit(VIN_STREAM, &cap->state);
	vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);
	if (vinc->dma_merge_mode == 1) {
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			vinc_bind = vin_core_gbl[vinc->id - 1];
			clear_bit(VIN_STREAM, &vinc_bind->vid_cap.state);
			vin_pipeline_call(vinc_bind, set_stream, &vinc_bind->vid_cap.pipe, 0);
			__csi_isp_setup_link(vinc_bind, 0);
			__vin_sensor_setup_link(vinc_bind, module, valid_idx, 0);
		}
	}
	set_bit(VIN_LPM, &cap->state);
	__csi_isp_setup_link(vinc, 0);
	__vin_sensor_setup_link(vinc, module, valid_idx, 0);
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = -EINVAL;
		goto streamoff_error;
	}

	ret = vb2_ioctl_streamoff(file, priv, i);
	if (ret != 0) {
		vin_err("video%d stream off error!\n", vinc->id);
		goto streamoff_error;
	}
	vin_queue_free(file);
streamoff_error:

	return ret;
}

static int vidioc_enum_input(struct file *file, void *priv,
			     struct v4l2_input *inp)
{
	if (inp->index != 0)
		return -EINVAL;

	inp->type = V4L2_INPUT_TYPE_CAMERA;
	inp->std = V4L2_STD_UNKNOWN;
	strcpy(inp->name, "sunxi-vin");

	return 0;
}

static int vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct vin_core *vinc = video_drvdata(file);

	if (vinc->sensor_sel == vinc->rear_sensor)
		*i = 0;
	else
		*i = 1;

	return 0;
}

static int __vin_actuator_set_power(struct v4l2_subdev *sd, int on)
{
#if 0 /* need to review */
	int *use_count;
	int ret;

	if (sd == NULL)
		return -ENXIO;

	use_count = &sd->entity.use_count;
	if (on && (*use_count)++ > 0)
		return 0;
	else if (!on && (*use_count == 0 || --(*use_count) > 0))
		return 0;
	ret = v4l2_subdev_call(sd, core, ioctl, ACT_SOFT_PWDN, 0);

	return ret != -ENOIOCTLCMD ? ret : 0;
#else
	int use_count;
	int ret;

	if (sd == NULL)
		return -ENXIO;

	use_count = &sd->entity.use_count;
	if (on && (use_count)++ > 0)
		return 0;
	else if (!on && (use_count == 0 || --(use_count) > 0))
		return 0;
	ret = v4l2_subdev_call(sd, core, ioctl, ACT_SOFT_PWDN, 0);

	return ret != -ENOIOCTLCMD ? ret : 0;
#endif
}

static int __vin_s_input(struct vin_core *vinc, unsigned int i)
{
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct modules_config *module = NULL;
	struct sensor_instance *inst = NULL;
	struct sensor_info *info = NULL;
	struct mipi_dev *mipi = NULL;
	int valid_idx = -1;
	int ret;

	i = i > 1 ? 0 : i;

	if (i == 0)
		vinc->sensor_sel = vinc->rear_sensor;
	else
		vinc->sensor_sel = vinc->front_sensor;

	module = &vind->modules[vinc->sensor_sel];
	valid_idx = module->sensors.valid_idx;

	if (valid_idx == NO_VALID_SENSOR) {
		vin_err("there is no valid sensor\n");
		return -EINVAL;
	}

	if (__vin_sensor_setup_link(vinc, module, valid_idx, 1) < 0) {
		vin_err("sensor setup link failed\n");
		return -EINVAL;
	}
	if (__csi_isp_setup_link(vinc, 1) < 0) {
		vin_err("csi&isp setup link failed\n");
		return -EINVAL;
	}
	inst = &module->sensors.inst[valid_idx];

	sunxi_isp_sensor_type(cap->pipe.sd[VIN_IND_ISP], inst->is_isp_used);
	vinc->support_raw = inst->is_isp_used;

	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	ret = vin_pipeline_call(vinc, open, &cap->pipe, &cap->vdev.entity, true);
	if (ret < 0) {
		vin_err("vin pipeline open failed (%d)!\n", ret);
		return ret;
	}
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);

	if (module->modules.act[valid_idx].sd != NULL) {
		cap->pipe.sd[VIN_IND_ACTUATOR] = module->modules.act[valid_idx].sd;
		ret = __vin_actuator_set_power(cap->pipe.sd[VIN_IND_ACTUATOR], 1);
		if (ret < 0) {
			vin_err("actutor power off failed (%d)!\n", ret);
			return ret;
		}
	}

	if (module->modules.flash.sd != NULL)
		cap->pipe.sd[VIN_IND_FLASH] = module->modules.flash.sd;

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 1);
	if (ret < 0) {
		vin_err("ISP init error at %s\n", __func__);
		return ret;
	}

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SCALER], core, init, 1);
	if (ret < 0) {
		vin_err("SCALER init error at %s\n", __func__);
		return ret;
	}

	/* save exp and gain for reopen, sensor init may reset gain to 0, so save before init! */
	info = container_of(cap->pipe.sd[VIN_IND_SENSOR], struct sensor_info, sd);
	if (info) {
		vinc->exp_gain.exp_val = info->exp;
		vinc->exp_gain.gain_val = info->gain;
		vinc->stream_idx = info->stream_seq + 2;
#ifdef CSIC_SDRAM_DFS
		vinc->vin_dfs.stable_frame_cnt = info->stable_frame_cnt;
		vinc->vin_dfs.sensor_status = info->sdram_dfs_flag;
#endif
	}
#ifdef CSIC_SDRAM_DFS
	if (vinc->vin_dfs.sensor_status == SENSOR_NOT_DEBUG)
		vin_warn("the sensor is not debugged. Please debug it before using the sdram dfs function\n");

	if (vin_core_gbl[dma_virtual_find_logic[vinc->id]]->work_mode == BK_ONLINE)
		vinc->vin_dfs.vinc_sdram_status = SDRAM_CAN_ENABLE;
	else {
		vinc->vin_dfs.vinc_sdram_status = SDRAM_NOT_USED;
		vin_warn("video%d works in online mode, csic sdeam function is prohibited\n", vinc->id);

	}
#endif
	if (cap->pipe.sd[VIN_IND_MIPI] != NULL) {
		mipi = container_of(cap->pipe.sd[VIN_IND_MIPI], struct mipi_dev, subdev);
		if (mipi)
			mipi->sensor_flags = vinc->sensor_sel;
	}

	if (!vinc->ptn_cfg.ptn_en) {
		ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, init, 1);
		if (ret) {
			vin_err("sensor initial error when selecting target device!\n");
			return ret;
		}
	}
	clear_bit(VIN_LPM, &cap->state);

	/* setup the current ctrl value */
	/*
	v4l2_ctrl_handler_setup(&vinc->vid_cap.ctrl_handler);
	v4l2_ctrl_handler_setup(cap->pipe.sd[VIN_IND_SENSOR]->ctrl_handler);
	 */

	vinc->hflip = inst->hflip;
	vinc->vflip = inst->vflip;

	return ret;
}

static int vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	struct vin_core *vinc = video_drvdata(file);
	int ret;
	__maybe_unused struct vin_core *vinc_bind = NULL;

	if (vinc->dma_merge_mode == 1) {
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			vinc_bind = vin_core_gbl[vinc->id - 1];
			ret = __vin_s_input(vinc_bind, i);
			if (ret < 0)
				return ret;
		}
	}

	ret = __vin_s_input(vinc, i);
	if (ret < 0)
		return ret;

	return 0;
}

static const char *const sensor_info_type[] = {
	"YUV",
	"RAW",
	NULL,
};

static int vidioc_g_pixelaspect(struct file *file, void *fh,
				    int buf_type, struct v4l2_fract *fract)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_fract sensor_fract;
	int ret = 0;

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], video,
				g_pixelaspect, &sensor_fract);
	if (ret < 0) {
		vin_warn("sensor g_pixelaspect fail!\n");
		return ret;
	}

	fract->numerator = sensor_fract.numerator;
	fract->denominator = sensor_fract.denominator;

	return 0;
}

static int vidioc_g_parm(struct file *file, void *priv,
			 struct v4l2_streamparm *parms)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;

	int ret;

	ret = sensor_g_parm(cap->pipe.sd[VIN_IND_SENSOR], parms);
	if (ret < 0)
		vin_warn("v4l2 sub device g_parm fail!\n");

	return ret;

}

static int __vin_s_parm(struct vin_core *vinc, struct v4l2_streamparm *parms)
{
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct sensor_instance *inst = get_valid_sensor(vinc);
	int ret = 0;

	if (parms->parm.capture.capturemode != V4L2_MODE_VIDEO &&
	    parms->parm.capture.capturemode != V4L2_MODE_IMAGE &&
	    parms->parm.capture.capturemode != V4L2_MODE_PREVIEW) {
		parms->parm.capture.capturemode = V4L2_MODE_PREVIEW;
	}

	if (vinc->dma_merge_mode == 1)
		parms->parm.capture.reserved[2] = 3;

	if ((vinc->csi_ch != 0xff) && (vinc->csi_ch & 0x20))
		sunxi_csi_set_ch_mode(cap->pipe.sd[VIN_IND_CSI]);

	cap->capture_mode = parms->parm.capture.capturemode;
	vinc->large_image = parms->parm.capture.reserved[2];

	if (WARN_ON(!cap->pipe.sd[VIN_IND_SENSOR] || !cap->pipe.sd[VIN_IND_CSI]))
		return -EINVAL;

	ret = sensor_s_parm(cap->pipe.sd[VIN_IND_SENSOR], parms);
	if (ret < 0)
		vin_warn("v4l2 subdev sensor s_parm error!\n");

	ret = sunxi_csi_subdev_s_parm(cap->pipe.sd[VIN_IND_CSI], parms);
	if (ret < 0)
		vin_warn("v4l2 subdev csi s_parm error!\n");

#ifdef SUPPORT_ISP_TDM
	if (vinc->tdm_rx_sel != 0xff) {
		ret = sunxi_tdm_subdev_s_parm(cap->pipe.sd[VIN_IND_TDM_RX], parms);
		if (ret < 0)
			vin_warn("v4l2 subdev tdm s_parm error!\n");
	}
#endif

	if (inst->is_isp_used && cap->pipe.sd[VIN_IND_ISP]) {
		ret = sunxi_isp_s_parm(cap->pipe.sd[VIN_IND_ISP], parms);
		if (ret < 0)
			vin_warn("v4l2 subdev isp s_parm error!\n");
	}

	ret = sunxi_scaler_subdev_s_parm(cap->pipe.sd[VIN_IND_SCALER], parms);
		if (ret < 0)
			vin_warn("v4l2 subdev scaler s_parm error!\n");

	return ret;
}

static int vidioc_s_parm(struct file *file, void *priv,
			struct v4l2_streamparm *parms)
{
	struct vin_core *vinc = video_drvdata(file);
	int ret;
	__maybe_unused struct vin_core *vinc_bind = NULL;

	if (vinc->dma_merge_mode == 1) {
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			vinc_bind = vin_core_gbl[vinc->id - 1];
			ret = __vin_s_parm(vinc_bind, parms);
			if (ret < 0)
				return ret;
		}
	}

	ret = __vin_s_parm(vinc, parms);
	if (ret < 0)
		return ret;
	return 0;
}

static int vidioc_s_dv_timings(struct file *file, void *fh,
				   struct v4l2_dv_timings *timings)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int ret = 0;

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], video, s_dv_timings, timings);
	if (ret)
		vin_err("sensor set dv timings error!\n");

	return ret;
}
static int vidioc_g_dv_timings(struct file *file, void *fh,
				   struct v4l2_dv_timings *timings)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int ret = 0;

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], video, g_dv_timings, timings);
	if (ret)
		vin_err("sensor set dv timings error!\n");

	return ret;
}

static int vidioc_query_dv_timings(struct file *file, void *fh,
				       struct v4l2_dv_timings *timings)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int ret = 0;

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], video, query_dv_timings, timings);
	if (ret)
		vin_err("sensor query dv timings error!\n");

	return ret;
}

static int vidioc_enum_dv_timings(struct file *file, void *fh,
				      struct v4l2_enum_dv_timings *timings)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int ret = 0;

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], pad, enum_dv_timings, timings);
	if (ret)
		vin_err("sensor enum dv timings error!\n");

	return ret;
}

static int vidioc_dv_timings_cap(struct file *file, void *fh,
				     struct v4l2_dv_timings_cap *cap)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *vid_cap = &vinc->vid_cap;
	int ret = 0;

	ret = v4l2_subdev_call(vid_cap->pipe.sd[VIN_IND_SENSOR], pad, dv_timings_cap, cap);
	if (ret)
		vin_err("sensor dv timings cap error!\n");

	return ret;
}

static int __vin_sensor_line2time(struct v4l2_subdev *sd, u32 exp_line)
{
	struct sensor_info *info = to_state(sd);
	u32 overflow = 0xffffffff / 1000000, pclk = 0;
	int exp_time = 0;

	if (NULL == info->current_wins) {
		vin_err("%s format is not initialized\n", sd->name);
		return 0;
	}

	if ((exp_line / 16) > overflow) {
		exp_line = exp_line / 16;
		pclk = info->current_wins->pclk / 1000000;
	} else if ((exp_line / 16) > (overflow / 10)) {
		exp_line = exp_line * 10 / 16;
		pclk = info->current_wins->pclk / 100000;
	} else if ((exp_line / 16) > (overflow / 100)) {
		exp_line = exp_line * 100 / 16;
		pclk = info->current_wins->pclk / 10000;
	} else if ((exp_line / 16) > (overflow / 1000)) {
		exp_line = exp_line * 1000 / 16;
		pclk = info->current_wins->pclk / 1000;
	} else {
		exp_line = exp_line * 10000 / 16;
		pclk = info->current_wins->pclk / 100;
	}

	if (pclk)
		exp_time = exp_line * info->current_wins->hts / pclk;

	return exp_time;
}

static int __vin_sensor_set_af_win(struct vin_vid_cap *cap)
{
	struct vin_pipeline *pipe = &cap->pipe;
	struct v4l2_win_setting af_win;
	int ret = 0;

	af_win.coor.x1 = cap->af_win[0]->val;
	af_win.coor.y1 = cap->af_win[1]->val;
	af_win.coor.x2 = cap->af_win[2]->val;
	af_win.coor.y2 = cap->af_win[3]->val;

	ret = v4l2_subdev_call(pipe->sd[VIN_IND_SENSOR],
				core, ioctl, SET_AUTO_FOCUS_WIN, &af_win);
	return ret;
}

static int __vin_sensor_set_ae_win(struct vin_vid_cap *cap)
{
	struct vin_pipeline *pipe = &cap->pipe;
	struct v4l2_win_setting ae_win;
	int ret = 0;

	ae_win.coor.x1 = cap->ae_win[0]->val;
	ae_win.coor.y1 = cap->ae_win[1]->val;
	ae_win.coor.x2 = cap->ae_win[2]->val;
	ae_win.coor.y2 = cap->ae_win[3]->val;
	ret = v4l2_subdev_call(pipe->sd[VIN_IND_SENSOR],
				core, ioctl, SET_AUTO_EXPOSURE_WIN, &ae_win);
	return ret;
}

int vidioc_sync_ctrl(struct file *file, struct v4l2_fh *fh,
			struct csi_sync_ctrl *sync)
{
	struct vin_core *vinc = video_drvdata(file);

	if (!sync->type) {
		csic_prs_sync_en_cfg(vinc->csi_sel, sync);
		csic_prs_sync_cfg(vinc->csi_sel, sync);
		csic_prs_sync_wait_N(vinc->csi_sel, sync);
		csic_prs_sync_wait_M(vinc->csi_sel, sync);
		csic_frame_cnt_enable(vinc->vipp_sel);
		csic_dma_frm_cnt(vinc->vipp_sel, sync);
		csic_prs_sync_en(vinc->csi_sel, sync);
	} else {
		csic_prs_xs_en(vinc->csi_sel, sync);
		csic_prs_xs_period_len_register(vinc->csi_sel, sync);
	}
	return 0;
}

static int vidioc_set_top_clk(struct file *file, struct v4l2_fh *fh,
			struct vin_top_clk *clk)
{
	struct vin_core *vinc = video_drvdata(file);

	vinc->vin_clk = clk->clk_rate;

	return 0;
}

static int vidioc_set_fps_ds(struct file *file, struct v4l2_fh *fh,
			struct vin_fps_ds *fps_down_sample)
{
	struct vin_core *vinc = video_drvdata(file);

	vinc->fps_ds = fps_down_sample->fps_ds;

	return 0;
}

static int vidioc_set_isp_debug(struct file *file, struct v4l2_fh *fh,
			struct isp_debug_mode *isp_debug)
{
	struct vin_core *vinc = video_drvdata(file);

	vinc->isp_dbg = *isp_debug;
	sunxi_isp_debug(vinc->vid_cap.pipe.sd[VIN_IND_ISP], isp_debug);

	return 0;
}

static int vidioc_vin_ptn_config(struct file *file, struct v4l2_fh *fh,
			struct vin_pattern_config *ptn)
{
	/* SUPPORT_PTN */
	struct vin_core *vinc = video_drvdata(file);
	struct isp_dev *isp = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_ISP]);
	int ret = 0;
	int i;
	printk("vidioc_vin_ptn_config work!\n");

	if (ptn->ptn_en) {
		vinc->ptn_cfg.ptn_en = 1;
		vinc->ptn_cfg.ptn_w = ptn->ptn_w;
		vinc->ptn_cfg.ptn_h = ptn->ptn_h;
		vinc->ptn_cfg.ptn_mode = 12;
		vinc->ptn_cfg.ptn_buf.size = ptn->ptn_size + 64; /* read 16word for vblanks */
		/* ptn_type: 0:recycle the first picture 1:recycle all picture */
		vinc->ptn_cfg.ptn_type = ptn->ptn_type;
		sunxi_isp_ptn(vinc->vid_cap.pipe.sd[VIN_IND_ISP], vinc->ptn_cfg.ptn_type);
		isp->ptn_cfg = &vinc->ptn_cfg;
		switch (ptn->ptn_fmt) {
		/* the data needs to be converted to 10bit format */
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGBRG8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SRGGB8:
			vinc->ptn_cfg.ptn_dw = 0;
			vinc->ptn_cfg.ptn_size = ptn->ptn_w * ptn->ptn_h * 2;
			break;
		case V4L2_PIX_FMT_SBGGR10:
		case V4L2_PIX_FMT_SGBRG10:
		case V4L2_PIX_FMT_SGRBG10:
		case V4L2_PIX_FMT_SRGGB10:
			vinc->ptn_cfg.ptn_dw = 1;
			vinc->ptn_cfg.ptn_size = ptn->ptn_w * ptn->ptn_h * 2;
			break;
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
			vinc->ptn_cfg.ptn_dw = 2;
			vinc->ptn_cfg.ptn_size = ptn->ptn_w * ptn->ptn_h * 2;
			break;
		default:
			vinc->ptn_cfg.ptn_dw = 0;
			vinc->ptn_cfg.ptn_size = ptn->ptn_w * ptn->ptn_h * 2;
			break;
		}

		if (vinc->ptn_cfg.ptn_buf.size < vinc->ptn_cfg.ptn_size) {
			vin_err("ptn need size is 0x%x, but intput picture size is 0x%x\n", (unsigned int)vinc->ptn_cfg.ptn_size, (unsigned int)vinc->ptn_cfg.ptn_buf.size);
			vinc->ptn_cfg.ptn_en = 0;
			return -EINVAL;
		}
		vinc->ptn_cfg.ptn_cnt = vinc->ptn_cfg.ptn_buf.size / vinc->ptn_cfg.ptn_size;
		vin_print("ptn_cnt is %d\n", vinc->ptn_cfg.ptn_cnt);

		/* 1080P:ptn_gen_dly:0xf:2fps 0x4:8fps 0x2:15fps 0x1:27fps 0x0:125fsp */
		if (ptn->ptn_w >= 2560)
			vinc->ptn_cfg.ptn_gen_dly = 0x1;
		else if (ptn->ptn_w >= 1920)
			vinc->ptn_cfg.ptn_gen_dly = 0x0;
		else if (ptn->ptn_w >= 1280)
			vinc->ptn_cfg.ptn_gen_dly = 0x4;
		else if (ptn->ptn_w >= 640)
			vinc->ptn_cfg.ptn_gen_dly = 0x8;
		else
			vinc->ptn_cfg.ptn_gen_dly = 0xf;

		if (ptn->ptn_addr) {
			if (vinc->ptn_cfg.ptn_buf.vir_addr == NULL)
				os_mem_alloc(&vinc->pdev->dev, &vinc->ptn_cfg.ptn_buf);
			if (vinc->ptn_cfg.ptn_buf.vir_addr == NULL) {
				vin_err("ptn buffer 0x%x alloc failed!\n", ptn->ptn_size);
				vinc->ptn_cfg.ptn_en = 0;
				return -ENOMEM;
			}

			vin_print("ptn dma_addr is 0x%p, size is 0x%x\n", vinc->ptn_cfg.ptn_buf.dma_addr, (unsigned int)vinc->ptn_cfg.ptn_buf.size);

			for (i = 0; i < vinc->ptn_cfg.ptn_cnt; i++) {
				ret = copy_from_user(vinc->ptn_cfg.ptn_buf.vir_addr + vinc->ptn_cfg.ptn_size * i, (void *)ptn->ptn_addr + vinc->ptn_cfg.ptn_size * i, vinc->ptn_cfg.ptn_size);
				if (ret < 0) {
					vin_err("copy ptn buffer from usr error!\n");
					vinc->ptn_cfg.ptn_en = 0;
					return ret;
				}
			}
		}
		/*set ptn driver output size*/
		if (vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]) {
			ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR],
						core, ioctl, SET_PTN, ptn);
			if (ret)
				vin_err("set %s ptn size fail", vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name);
		}
	} else {
		vinc->ptn_cfg.ptn_en = 0;
		if (vinc->ptn_cfg.ptn_buf.vir_addr)
			os_mem_free(&vinc->pdev->dev, &vinc->ptn_cfg.ptn_buf);
	}
// #endif
	return 0;
}

static int vidioc_vin_set_reset_time(struct file *file, struct v4l2_fh *fh,
			struct vin_reset_time *time)
{
	struct vin_core *vinc = video_drvdata(file);
	struct csi_dev *csi = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_CSI]);

	csi->reset_time = time->reset_time;

	return 0;
}

static int vidioc_set_parser_fps(struct file *file, struct v4l2_fh *fh,
			struct parser_fps_ds *parser_fps_ds)
{
	struct vin_core *vinc = video_drvdata(file);
	struct csi_dev *csi = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_CSI]);

	csi->prs_fps_ds.ch0_fps_ds = parser_fps_ds->ch0_fps_ds & 0xf;
	csi->prs_fps_ds.ch1_fps_ds = parser_fps_ds->ch1_fps_ds & 0xf;
	csi->prs_fps_ds.ch2_fps_ds = parser_fps_ds->ch2_fps_ds & 0xf;
	csi->prs_fps_ds.ch3_fps_ds = parser_fps_ds->ch3_fps_ds & 0xf;

	return 0;
}

static int vidioc_set_standby(struct file *file, struct v4l2_fh *fh,
		struct sensor_standby_status *stby_status)
{
	struct vin_core *vinc = video_drvdata(file);
	int ret;

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core,
			s_power, stby_status->stby_stat);

	if (ret != 0) {
		vin_warn("%s:sensor failed to set sensor standby !\n",
				__func__);
		return ret;
	}

	return 0;
}

/* must set after VIDIOC_S_PARM and before VIDIOC_S_FMT */
static int vidioc_set_sensor_isp_cfg(struct file *file, struct v4l2_fh *fh,
			struct sensor_isp_cfg *sensor_isp_cfg)
{
	struct vin_core *vinc = video_drvdata(file);
	struct isp_dev *isp = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_ISP]);
	struct v4l2_subdev *sd	= vinc->vid_cap.pipe.sd[VIN_IND_SENSOR];
	struct sensor_info *info = container_of(sd, struct sensor_info, sd);

	if (vinc->dma_merge_mode != 1) {
		isp->large_image = sensor_isp_cfg->large_image;
	}

	info->isp_wdr_mode = sensor_isp_cfg->isp_wdr_mode;

	return 0;
}

/* must set after VIDIOC_S_INPUT and before VIDIOC_S_PARM */
static int vidioc_set_ve_online_cfg(struct file *file, struct v4l2_fh *fh,
			struct csi_ve_online_cfg *cfg)
{
	struct vin_core *vinc = video_drvdata(file);

	if (!cfg->ve_online_en) {
		vinc->ve_online_cfg.ve_online_en = 0;
		vinc->ve_online_cfg.dma_buf_num = BK_MUL_BUFFER;
		vin_print("ve_online close\n");
		return 0;
	}

	if (vinc->work_mode == BK_OFFLINE) {
		vin_err("ve online mode need video%d work in online\n", vinc->id);
		return -1;
	}

	if (vinc->id == CSI_VE_ONLINE_VIDEO) {
		memcpy(&vinc->ve_online_cfg, cfg, sizeof(struct csi_ve_online_cfg));
	} else {
		vin_err("only video%d supply ve online\n", CSI_VE_ONLINE_VIDEO);
		return -1;
	}

	vin_print("ve_online %s, buffer_num is %d\n", vinc->ve_online_cfg.ve_online_en ? "open" : "close", vinc->ve_online_cfg.dma_buf_num);

	return 0;
}

/* must set after VIDIOC_S_FMT and before VIDIOC_STREAMON */
static int vidioc_set_vipp_shrink_cfg(struct file *file, struct v4l2_fh *fh,
			struct vipp_shrink_cfg *cfg)
{
	struct vin_core *vinc = video_drvdata(file);
	struct v4l2_subdev_selection sel;
	int ret;

	sel.target = V4L2_SEL_TGT_CROP;
	sel.pad = SCALER_PAD_SINK;
	sel.which = V4L2_SUBDEV_FORMAT_ACTIVE;
	sel.reserved[0] = VIPP_ONLY_SHRINK;

	sel.r.width = vinc->vid_cap.frame.o_width;
	sel.r.height = vinc->vid_cap.frame.o_height;
	sel.r.left = cfg->left;
	sel.r.top = cfg->top;
	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SCALER],
			pad, set_selection, NULL, &sel);
	if (ret < 0) {
		vin_err("vipp set_selection shrink error!\n");
	}

	return ret;
}

/* must set after VIDIOC_S_FMT and before VIDIOC_STREAMON */
static int vidioc_set_tdm_speeddn_cfg(struct file *file, struct v4l2_fh *fh,
			struct tdm_speeddn_cfg *cfg)
{
#if defined SUPPORT_ISP_TDM && defined TDM_V200
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct mipi_dev *mipi = container_of(cap->pipe.sd[VIN_IND_MIPI], struct mipi_dev, subdev);
	struct tdm_rx_dev *tdm_rx = container_of(cap->pipe.sd[VIN_IND_TDM_RX], struct tdm_rx_dev, subdev);
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	struct csi_dev *csi = v4l2_get_subdevdata(cap->pipe.sd[VIN_IND_CSI]);

	if (csi->bus_info.bus_if == V4L2_MBUS_CSI2_DPHY) {
		if (cfg->pix_num == MIPI_TWO_PIXEL && mipi->cmb_csi_cfg.lane_num != 4) {
			cfg->pix_num = MIPI_ONE_PIXEL;
			vin_warn("mipi %d lane cannot support two pixel, set to one pexel\n", mipi->cmb_csi_cfg.lane_num);
		}

		if (cfg->pix_num == MIPI_TWO_PIXEL && !cfg->tdm_speed_down_en) {
			cfg->tdm_speed_down_en = 1;
			vin_warn("when mipi set to two pixel, must open tdm speed_dn\n");
		}

		mipi->cmb_csi_cfg.pix_num = (enum cmb_csi_pix_num)cfg->pix_num;
	}

	tdm->ws.speed_dn_en = cfg->tdm_speed_down_en;
	if (cfg->tdm_tx_valid_num || cfg->tdm_tx_invalid_num) {
		tdm->tx_cfg.valid_num = cfg->tdm_tx_valid_num;
		tdm->tx_cfg.invalid_num = cfg->tdm_tx_invalid_num;
	} else {
		tdm->tx_cfg.valid_num = 1;
		tdm->tx_cfg.invalid_num = 0;
	}
#endif
	return 0;
}

#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
static int vin_get_isp_encpp_attr_cfg(struct file *file, struct v4l2_fh *fh, struct isp_encpp_cfg_attr_data *encpp_attr_cfg)
{

	struct vin_core *vinc = video_drvdata(file);
	struct isp_dev *isp = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_ISP]);

	if (encpp_attr_cfg != NULL) {
		if (isp->h3a_stat.state == ISPSTAT_ENABLED) {
			encpp_attr_cfg->encpp_en = isp->encpp_en;
			/*  encpp_static_sharp_config */
			memcpy(&encpp_attr_cfg->encpp_static_sharp_cfg, &isp->encpp_static_sharp_cfg,
				sizeof(struct encpp_static_sharp_config));
			/*  encpp_dynamic_sharp_config */
			memcpy(&encpp_attr_cfg->encpp_dynamic_sharp_cfg, &isp->encpp_dynamic_sharp_cfg,
				sizeof(struct encpp_dynamic_sharp_config));
			/*  encoder_3dnr_config */
			memcpy(&encpp_attr_cfg->encoder_3dnr_cfg, &isp->encoder_3dnr_cfg,
				sizeof(struct encoder_3dnr_config));
			/*  encoder_2dnr_config */
			memcpy(&encpp_attr_cfg->encoder_2dnr_cfg, &isp->encoder_2dnr_cfg,
				sizeof(struct encoder_2dnr_config));
		} else {
			vin_warn("h3a_stat.state is ISPSTAT_DISABLED, will not get isp encpp attr cfg\n");
		}
	} else {
		vin_err("encpp_attr_cfg is NULL!!!\n");
		return -1;
	}

	return 0;
}

static int vin_set_isp_param_bin_cfg(struct isp_dev *isp, char *bin_path)
{
	int idx;
	struct file *file_fd = NULL;
	struct isp_param_config *param = NULL;
	unsigned int size, len;
	char time[20], notes[50], fdstr[100];
	int rpmsg_data[110];
	char *param_start = NULL;
	loff_t pos = 0;

	sprintf(fdstr, "%s/isp_param_config.bin", bin_path);
	file_fd = filp_open(fdstr, O_RDONLY, 0);
	if (IS_ERR(file_fd)) {
		vin_err("open %s failed.\n", fdstr);
		return -1;
	} else {
		vfs_read(file_fd, (char *)&size, sizeof(unsigned int), &pos);
		if (size != sizeof(struct isp_param_config)) {
			vin_err("%s -- read size %d != isp_param size %ld!\n", fdstr, size, sizeof(struct isp_param_config));
			filp_close(file_fd, NULL);
			return -1;
		} else {
			param = kzalloc(sizeof(struct isp_param_config), GFP_KERNEL);
			vfs_read(file_fd, (char *)time, 20, &pos);
			vfs_read(file_fd, (char *)notes, 50, &pos);
			vfs_read(file_fd, (char *)param, size, &pos);

			rpmsg_data[0] = VIN_SET_ATTR_IOCTL;
			rpmsg_data[1] = ISP_CTRL_READ_BIN_PARAM;
			//isp_test_settings + isp_3a_settings + isp_tunning_settings
			len = sizeof(struct isp_test_param) + sizeof(struct isp_3a_param) + sizeof(struct isp_tunning_param);
			param_start = (char *)&param->isp_test_settings;
			rpmsg_data[2] = SET_BIN_TEST_3A_TUNING;//flag;
			idx = 0;
			while (len) {
				rpmsg_data[3] = idx;
				if (len >= 400) {
					rpmsg_data[4] = 400;
					len -= 400;
				} else {
					rpmsg_data[4] = len;
					len = 0;
				}
				memcpy(&rpmsg_data[5], &param_start[idx * 400], rpmsg_data[4]);
				idx++;
				isp_rpmsg_send(isp, rpmsg_data, 110 * 4);
				usleep_range(500, 550);
			}
			//isp_iso_settings
			//triger
			rpmsg_data[2] = SET_BIN_ISO_TRIGER;//flag;
			rpmsg_data[3] = 0;
			rpmsg_data[4] = sizeof(param->isp_iso_settings.triger);
			rpmsg_data[5] = param->isp_iso_settings.triger.sharp_triger;
#ifdef USE_ENCPP
			rpmsg_data[6] = param->isp_iso_settings.triger.encpp_sharp_triger;
			rpmsg_data[7] = param->isp_iso_settings.triger.encoder_denoise_triger;
#endif
			rpmsg_data[8] = param->isp_iso_settings.triger.denoise_triger;
			rpmsg_data[9] = param->isp_iso_settings.triger.black_level_triger;
			rpmsg_data[10] = param->isp_iso_settings.triger.dpc_triger;
			rpmsg_data[11] = param->isp_iso_settings.triger.defog_value_triger;
			rpmsg_data[12] = param->isp_iso_settings.triger.pltm_dynamic_triger;
			rpmsg_data[13] = param->isp_iso_settings.triger.brightness_triger;
			rpmsg_data[14] = param->isp_iso_settings.triger.gcontrast_triger;
			rpmsg_data[15] = param->isp_iso_settings.triger.cem_triger;
			rpmsg_data[16] = param->isp_iso_settings.triger.tdf_triger;
			rpmsg_data[17] = param->isp_iso_settings.triger.color_denoise_triger;
			rpmsg_data[18] = param->isp_iso_settings.triger.ae_cfg_triger;
			rpmsg_data[19] = param->isp_iso_settings.triger.gtm_cfg_triger;
			rpmsg_data[20] = param->isp_iso_settings.triger.lca_cfg_triger;
			rpmsg_data[21] = param->isp_iso_settings.triger.wdr_cfg_triger;
			rpmsg_data[22] = param->isp_iso_settings.triger.cfa_triger;
			rpmsg_data[23] = param->isp_iso_settings.triger.shading_triger;
			isp_rpmsg_send(isp, rpmsg_data, 24 * 4);
			usleep_range(500, 550);
			//isp_lum_mapping_point + isp_gain_mapping_point + isp_dynamic_cfg
			len = sizeof(param->isp_iso_settings.isp_lum_mapping_point) +
				sizeof(param->isp_iso_settings.isp_gain_mapping_point) +
				sizeof(param->isp_iso_settings.isp_dynamic_cfg);
			param_start = (char *)&param->isp_iso_settings.isp_lum_mapping_point[0];
			rpmsg_data[2] = SET_BIN_ISO_OTHER;//flag;
			idx = 0;
			while (len) {
				rpmsg_data[3] = idx;
				if (len >= 400) {
					rpmsg_data[4] = 400;
					len -= 400;
				} else {
					rpmsg_data[4] = len;
					len = 0;
				}
				memcpy(&rpmsg_data[5], &param_start[idx * 400], rpmsg_data[4]);
				idx++;
				isp_rpmsg_send(isp, rpmsg_data, 110 * 4);
				usleep_range(500, 550);
			}

			//finish
			rpmsg_data[2] = SET_BIN_TUNING_UPDATE;//flag
			rpmsg_data[3] = 0;
			rpmsg_data[4] = 0;
			isp_rpmsg_send(isp, rpmsg_data, 5 * 4);
			vin_print("Read %s seccess... Time:%s  Notes:%s\n", fdstr, time, notes);
			kfree(param);
		}
	}

	filp_close(file_fd, NULL);
	return 0;
}

static int vin_set_isp_attr_cfg_ctrl(struct vin_core *vinc, struct isp_cfg_attr_data *attr_cfg)
{
	struct isp_dev *isp = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_ISP]);
	struct ae_table_info *user_ae_table;
	int data[124] = {0};
	int *ptr = NULL;
	unsigned char *ptr_u8 = NULL;
	int ret = 0;
	int i = 0;

	if (isp->h3a_stat.state == ISPSTAT_ENABLED) {
		data[0] = VIN_SET_ATTR_IOCTL;
		data[1] = attr_cfg->cfg_id;
		switch (attr_cfg->cfg_id) {
		case ISP_CTRL_DN_STR:
			data[2] = attr_cfg->denoise_level;
			isp_rpmsg_send(isp, data, 3*4);
			break;
		case ISP_CTRL_3DN_STR:
			data[2] = attr_cfg->tdf_level;
			isp_rpmsg_send(isp, data, 3*4);
			break;
		case ISP_CTRL_PLTMWDR_STR:
			data[2] = attr_cfg->pltmwdr_level;
			isp_rpmsg_send(isp, data, 3*4);
			break;
		case ISP_CTRL_IR_STATUS:
			data[2] = attr_cfg->ir_status;
			isp->isp_cfg_attr.ir_status = attr_cfg->ir_status;
			isp_rpmsg_send(isp, data, 3*4);
			break;
		case ISP_CTRL_EV_IDX:
			data[2] = attr_cfg->ae_ev_idx;
			isp_rpmsg_send(isp, data, 3*4);
			break;
		case ISP_CTRL_AE_LOCK:
			data[2] = attr_cfg->ae_lock;
			isp_rpmsg_send(isp, data, 3*4);
			break;
		case ISP_CTRL_AE_TABLE:
			user_ae_table = (struct ae_table_info *)kzalloc(sizeof(struct ae_table_info), GFP_KERNEL);
			if (user_ae_table == NULL) {
				vin_err("kzalloc user_ae_table error!!!\n");
				return -1;
			}
			ret = copy_from_user(user_ae_table, attr_cfg->ae_table, sizeof(struct ae_table_info));
			if (ret != 0) {
				vin_err("copy attr_cfg->ae_table from usr error!\n");
				return -1;
			}
			ptr = &(user_ae_table->ae_tbl[0].min_exp);
			for (i = 2; i < (user_ae_table->length*6 + 3); i++, ptr++) {
				data[i] = *ptr;
			}
			data[63] = user_ae_table->length;
			data[64] = user_ae_table->ev_step;
			data[65] = user_ae_table->shutter_shift;
			kfree(user_ae_table);
			isp_rpmsg_send(isp, data, 66*4);
			break;
		case ISP_CTRL_READ_BIN_PARAM:
			vin_set_isp_param_bin_cfg(isp, attr_cfg->path);
			break;
		case ISP_CTRL_AE_ROI_TARGET:
			data[2] = attr_cfg->ae_roi_area.enable;
			data[3] = attr_cfg->ae_roi_area.force_ae_target;
			data[4] = attr_cfg->ae_roi_area.coor.x1;
			data[5] = attr_cfg->ae_roi_area.coor.y1;
			data[6] = attr_cfg->ae_roi_area.coor.x2;
			data[7] = attr_cfg->ae_roi_area.coor.y2;
			isp_rpmsg_send(isp, data, 8*4);
			break;
		case ISP_CTRL_VENC2ISP_PARAM:
			ptr_u8 = (unsigned char *)&data[2];
			ptr_u8[0] = attr_cfg->VencVe2IspParam.mMovingLevelInfo.is_overflow;
			for (i = 0; i < ISP_MSC_TBL_SIZE; i++) {
				ptr_u8[i + 1] = attr_cfg->VencVe2IspParam.mMovingLevelInfo.moving_level_table[i] >> 4;
			}
			isp_rpmsg_send(isp, data, 496);
			break;
		}
	} else {
		vin_warn("h3a_stat.state is ISPSTAT_DISABLED, will not set isp attr cfg\n");
	}

	return 0;
}

static int vin_get_isp_attr_cfg_ctrl(struct vin_core *vinc, struct isp_cfg_attr_data *attr_cfg)
{
	struct isp_dev *isp = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_ISP]);
	unsigned int data[2];
	unsigned int timeout_cnt = 100;
	int i;

	if (isp->h3a_stat.state == ISPSTAT_ENABLED) {
		data[0] = VIN_REQUEST_ATTR_IOCTL;
		data[1] = attr_cfg->cfg_id;
		/* reset update_flag for update isp_cfg action start */
		isp->isp_cfg_attr.update_flag = 0;
		isp_rpmsg_send(isp, data, 2*4);

		for (i = 0; i < timeout_cnt; i++) {
			/* wait for update_flag ok */
			if (isp->isp_cfg_attr.update_flag)
				break;
			usleep_range(500, 550);
		}

		if (i == timeout_cnt) {
			vin_err("VIDIOC_GET_ISP_CFG_ATTR timeout!!!\n");
			return -1;
		} else {
			/* update isp_attr_cfg */
			memcpy(attr_cfg, &(isp->isp_cfg_attr), sizeof(struct isp_cfg_attr_data));
		}
	} else {
		vin_warn("h3a_stat.state is ISPSTAT_DISABLED, will not update isp attr cfg\n");
	}

	return 0;
}
#endif

static int vidioc_set_isp_attr_cfg(struct file *file, struct v4l2_fh *fh,
			struct isp_cfg_attr_data *attr_cfg)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	struct vin_core *vinc = video_drvdata(file);

	ret = vin_set_isp_attr_cfg_ctrl(vinc, attr_cfg);
#endif
	return ret;
}

static int vidioc_get_isp_attr_cfg(struct file *file, struct v4l2_fh *fh,
			struct isp_cfg_attr_data *attr_cfg)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	struct vin_core *vinc = video_drvdata(file);

	ret = vin_get_isp_attr_cfg_ctrl(vinc, attr_cfg);
#endif
	return ret;
}

static int vidioc_set_phy2vir_cfg(struct file *file, struct v4l2_fh *fh,
			struct isp_memremap_cfg *isp_memremap)
{
#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	__maybe_unused struct vin_core *vinc = video_drvdata(file);
	unsigned long viraddr;
	struct vm_area_struct *vma;
	void *vaddr = NULL;
	__maybe_unused struct isp_autoflash_config_s *isp_autoflash_cfg = NULL;
	__maybe_unused unsigned int map_addr = 0;
	__maybe_unused unsigned int check_sign = 0;

#ifndef GET_RV_YUV
	if (vinc->mipi_sel == 0) {
		map_addr = VIN_SENSOR0_RESERVE_ADDR;
		check_sign = 0xAA11AA11;
	} else {
		map_addr = VIN_SENSOR1_RESERVE_ADDR;
		check_sign = 0xBB11BB11;
	}

	vaddr = vin_map_kernel(map_addr, VIN_RESERVE_SIZE + VIN_THRESHOLD_PARAM_SIZE); /* map unit is page, page is align of 4k */
	if (vaddr == NULL) {
		vin_err("%s:map 0x%x paddr err!!!", __func__, map_addr);
		return -EFAULT;
	}

	isp_autoflash_cfg = (struct isp_autoflash_config_s *)(vaddr + VIN_RESERVE_SIZE);

	/* check id */
	if (isp_autoflash_cfg->melisyuv_sign_id != check_sign) {
		vin_warn("%s:sign is 0x%x but not 0x%x\n", __func__, isp_autoflash_cfg->sensorlist_sign_id, check_sign);
		vin_unmap_kernel(vaddr);
		return -EFAULT;
	}

	if (isp_memremap->en) {
		viraddr = vm_mmap(NULL, 0, isp_autoflash_cfg->melisyuv_size, PROT_READ, MAP_SHARED | MAP_NORESERVE, 0);
		vma = find_vma(current->mm, viraddr);
		remap_pfn_range(vma, vma->vm_start, __phys_to_pfn(isp_autoflash_cfg->melisyuv_paddr), isp_autoflash_cfg->melisyuv_size, vma->vm_page_prot);
		isp_memremap->vir_addr = (void *)viraddr;
		isp_memremap->size = isp_autoflash_cfg->melisyuv_size;
		vin_print("0x%x mmap viraddr is 0x%lx\n", isp_autoflash_cfg->melisyuv_paddr, viraddr);
	} else {
		if (isp_memremap->vir_addr && isp_memremap->size) {
			vm_munmap((unsigned long)isp_memremap->vir_addr, isp_memremap->size);
			vin_print("0x%x ummap viraddr is 0x%lx\n", isp_autoflash_cfg->melisyuv_paddr, (unsigned long)isp_memremap->vir_addr);

			isp_autoflash_cfg->melisyuv_sign_id = 0XFFFFFFFF;
			memblock_free(isp_autoflash_cfg->melisyuv_paddr, isp_autoflash_cfg->melisyuv_size);
			free_reserved_area(__va(isp_autoflash_cfg->melisyuv_paddr), __va(isp_autoflash_cfg->melisyuv_paddr + isp_autoflash_cfg->melisyuv_size), -1, "isp_reserved");
		}
	}
#else
	vaddr = vin_map_kernel(YUV_MEMRESERVE, YUV_MEMRESERVE_SIZE); /* map unit is page, page is align of 4k */ /* map unit is page, page is align of 4k */
	if (vaddr == NULL) {
		vin_err("%s:map 0x%x paddr err!!!", __func__, map_addr);
		return -EFAULT;
	}

	if (isp_memremap->en) {
		viraddr = vm_mmap(NULL, 0, YUV_MEMRESERVE_SIZE, PROT_READ, MAP_SHARED | MAP_NORESERVE, 0);
		vma = find_vma(current->mm, viraddr);
		remap_pfn_range(vma, vma->vm_start, __phys_to_pfn(YUV_MEMRESERVE), YUV_MEMRESERVE_SIZE, vma->vm_page_prot);
		isp_memremap->vir_addr = (void *)viraddr;
		isp_memremap->size = YUV_MEMRESERVE_SIZE;
		vin_print("0x%x mmap viraddr is 0x%lx\n", YUV_MEMRESERVE, viraddr);
	} else {
		if (isp_memremap->vir_addr && isp_memremap->size) {
			vm_munmap((unsigned long)isp_memremap->vir_addr, isp_memremap->size);
			vin_print("0x%x ummap viraddr is 0x%lx\n", YUV_MEMRESERVE, (unsigned long)isp_memremap->vir_addr);

			memblock_free(YUV_MEMRESERVE, YUV_MEMRESERVE_SIZE);
			free_reserved_area(__va(YUV_MEMRESERVE), __va(YUV_MEMRESERVE + YUV_MEMRESERVE_SIZE), -1, "isp_reserved");
		}
	}
#endif

	vin_unmap_kernel(vaddr);
#endif
	return 0;
}

/* must set before VIDIOC_S_INPUT */
static int vidioc_merge_int_ch_cfg(struct file *file, struct v4l2_fh *fh,
			struct mrg_int_ch_cfg *cfg)
{
#if defined MULTI_FRM_MERGE_INT
	struct vin_core *vinc = video_drvdata(file);
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	unsigned int bk_ch_sel = cfg->mrg_ch_sel;
	int i = 0;
#if IS_ENABLED(CONFIG_ARCH_SUN60IW1)
	int bk_ch_intpool_sel[VIN_MAX_DEV] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 16, 20,
						24, 25, 26, 27, 28, 29, 30, 31, 32, 36, 40};
#elif IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	int bk_ch_intpool_sel[VIN_MAX_DEV] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	int bk_ch_intpool_sel[VIN_MAX_DEV] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
#endif
	if (cfg->trig_level < 1) {
		cfg->trig_level = 1;
		vin_warn("trig_level must be greater than or equal to 1\n");
	}
	/* max trigger level is 63 */
	vind->bk_intpool.trig_level = cfg->trig_level > 63 ? 63 : cfg->trig_level;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (GET_BIT(bk_ch_sel, i)) {
			if (bk_ch_intpool_sel[i] < 16)
				vind->bk_intpool.mask_cfg0 |= 1 << bk_ch_intpool_sel[i];
			else
				vind->bk_intpool.mask_cfg1 |= 1 << (bk_ch_intpool_sel[i] - 16);
		}
	}
#endif
	return 0;
}

static int __tvin_info_check(struct tvin_init_info *info)
{
	if (info->work_mode >= Tvd_Input_Type_SIZE) {
		vin_err("tvin not support this work mode\n");
		return -1;
	}
	if (info->ch_id >= TVIN_CH_SIZE) {
		vin_err("[%s]ch_id can not over %d\n", __func__, TVIN_CH_SIZE);
		return -1;
	}

	if (info->input_fmt[info->ch_id] > INPUT_FMT_SIZE) {
		vin_err("tvin not support ch%d fmt = %d\n", info->ch_id, info->input_fmt[info->ch_id]);
		return -1;
	}
	return 0;
}

static int vidioc_tvin_init(struct file *file,
			struct v4l2_fh *fh, struct tvin_init_info *info)
{
	struct vin_core *vinc = video_drvdata(file);
	int ret = 0;

	if (__tvin_info_check(info))
		return -1;

	vinc->tvin.work_mode = info->work_mode;
	vinc->tvin.input_fmt = info->input_fmt[info->ch_id];
	vinc->tvin.flag = true;

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
			SENSOR_TVIN_INIT, info);
	if (ret)
		vin_err("sensor tvin init fail!\n");

	if (vinc->vid_cap.pipe.sd[VIN_IND_MIPI] != NULL) {
		ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_MIPI], core, ioctl,
				MIPI_TVIN_INIT, info);
		if (ret)
			vin_err("mipi tvin init fail!\n");
	}

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_CSI], core, ioctl,
			PARSER_TVIN_INIT, info);
	if (ret)
		vin_err("csi tvin init fail!\n");

	vin_log(VIN_LOG_FMT, "%s mode %d, fmt %d\n", __func__,
				vinc->tvin.work_mode, vinc->tvin.input_fmt);

	return ret;
}

static int vidioc_set_dma_merge(struct file *file, struct v4l2_fh *fh,
			unsigned char *dma_merge)
{
	struct vin_core *vinc = video_drvdata(file);
	__maybe_unused struct vin_core *vinc_bind = NULL;

	vinc->dma_merge_mode = *dma_merge;

	if (vinc->dma_merge_mode == 1) {
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			vinc_bind = vin_core_gbl[vinc->id - 1];
			vinc_bind->dma_merge_mode = *dma_merge;
		}
	}

	return 0;
}

static long vin_param_handler(struct file *file, void *priv,
			      bool valid_prio, unsigned int cmd, void *param)
{
	int ret = 0;
	struct v4l2_fh *fh = (struct v4l2_fh *)priv;

	switch (cmd) {
	case VIDIOC_ISP_EXIF_REQ:
		break;
	case VIDIOC_SYNC_CTRL:
		ret = vidioc_sync_ctrl(file, fh, param);
		break;
	case VIDIOC_SET_TOP_CLK:
		ret = vidioc_set_top_clk(file, fh, param);
		break;
	case VIDIOC_SET_FPS_DS:
		ret = vidioc_set_fps_ds(file, fh, param);
		break;
	case VIDIOC_ISP_DEBUG:
		ret = vidioc_set_isp_debug(file, fh, param);
		break;
	case VIDIOC_VIN_PTN_CFG:
		ret = vidioc_vin_ptn_config(file, fh, param);
		break;
	case VIDIOC_VIN_RESET_TIME:
		ret = vidioc_vin_set_reset_time(file, fh, param);
		break;
	case VIDIOC_SET_PARSER_FPS:
		ret = vidioc_set_parser_fps(file, fh, param);
		break;
	case VIDIOC_SET_SENSOR_ISP_CFG:
		ret = vidioc_set_sensor_isp_cfg(file, fh, param);
		break;
	case VIDIOC_SET_STANDBY:
		ret = vidioc_set_standby(file, fh, param);
		break;
	case VIDIOC_SET_VE_ONLINE:
		ret = vidioc_set_ve_online_cfg(file, fh, param);
		break;
	case VIDIOC_SET_VIPP_SHRINK:
		ret = vidioc_set_vipp_shrink_cfg(file, fh, param);
		break;
	case VIDIOC_SET_TDM_SPEEDDN_CFG:
		ret = vidioc_set_tdm_speeddn_cfg(file, fh, param);
		break;
	case VIDIOC_SET_ISP_CFG_ATTR:
		ret = vidioc_set_isp_attr_cfg(file, fh, param);
		break;
	case VIDIOC_GET_ISP_CFG_ATTR:
		ret = vidioc_get_isp_attr_cfg(file, fh, param);
		break;
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	case VIDIOC_GET_ISP_ENCPP_CFG_ATTR:
		ret = vin_get_isp_encpp_attr_cfg(file, fh, param);
		break;
#endif
	case VIDIOC_SET_PHY2VIR:
		ret = vidioc_set_phy2vir_cfg(file, fh, param);
		break;
	case VIDIOC_MERGE_INT_CH_CFG:
		ret = vidioc_merge_int_ch_cfg(file, fh, param);
		break;
	case VIDIOC_TVIN_INIT:
		ret = vidioc_tvin_init(file, fh, param);
		break;
	case VIDIOC_SET_DMA_MERGE:
		ret = vidioc_set_dma_merge(file, fh, param);
		break;
	default:
		ret = -ENOTTY;
	}
	return ret;
}

static int vin_subscribe_event(struct v4l2_fh *fh,
		const struct v4l2_event_subscription *sub)
{
	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_ctrl_subscribe_event(fh, sub);
	else
		return v4l2_event_subscribe(fh, sub, 1, NULL);
}

int vidioc_s_edid(struct file *file, void *fh, struct v4l2_edid *edid)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	int ret = 0;

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], pad, set_edid, edid);
	if (ret)
		vin_err("sensor set edid error!\n");

	return ret;
}

/* when camera on, close dfs */
#if IS_ENABLED(CONFIG_AW_DMC_DEVFREQ) && (IS_ENABLED(CONFIG_ARCH_SUN50IW10) || !defined CSIC_SDRAM_DFS)
static int vin_dfs_handler(struct vin_md *vind, bool en)
{
	struct device *csi_dfs_dev = vind->v4l2_dev.dev;
	char *envp[3] = {
		"SYSTEM=CAMERA",
		NULL,
		NULL};

	if (en)
		envp[1] = "EVENT=ON";
	else
		envp[1] = "EVENT=OFF";

	kobject_uevent_env(&csi_dfs_dev->kobj, KOBJ_CHANGE, envp);
	vin_print("camera is %s, %s dfs\n",
				en ? "on" : "off", en ? "close" : "open");
	return 0;
}
#endif

static int vin_open(struct file *file)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	__maybe_unused struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);

	if (vin_busy(cap)) {
		vin_err("video%d open busy\n", vinc->id);
		return -EBUSY;
	}

#if IS_ENABLED(CONFIG_RV_RUN_CAR_REVERSE) && !defined (CONFIG_ARCH_SUN60IW2)
	if (CONTROL_BY_RTOS == vinc->rpmsg.control) {
		vin_err("video%d is controlling by rtos\n", vinc->id);
		return -EBUSY;
	}

	vinc_status_rpmsg_send(ARM_VIN_START, &vinc->rpmsg);
#endif

	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	set_bit(VIN_LPM, &cap->state);
	set_bit(VIN_BUSY, &cap->state);
	v4l2_fh_open(file);/*  create event queue  */

#if IS_ENABLED(CONFIG_AW_DMC_DEVFREQ) && (IS_ENABLED(CONFIG_ARCH_SUN50IW10) || !defined CSIC_SDRAM_DFS)
	vin_dfs_handler(vind, 1);
#endif
#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY)
	dramfreq_master_access(MASTER_CSI, true);
#endif

	vin_log(VIN_LOG_VIDEO, "video%d open\n", vinc->id);
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	return 0;
}

static int vin_close(struct file *file)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct modules_config *module = &vind->modules[vinc->sensor_sel];
	int valid_idx = module->sensors.valid_idx;
	int ret;
	__maybe_unused struct vin_core *vinc_bind = NULL;

	if (!vin_busy(cap)) {
		vin_warn("video%d have been closed!\n", vinc->id);
		return 0;
	}

#if IS_ENABLED(CONFIG_RV_RUN_CAR_REVERSE) && !defined (CONFIG_ARCH_SUN60IW2)
	vinc_status_rpmsg_send(ARM_VIN_STOP, &vinc->rpmsg);
#endif

	if (vin_streaming(cap))
		vin_timer_del(vinc);

	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	if (!cap->pipe.sd[VIN_IND_SENSOR] || !cap->pipe.sd[VIN_IND_SENSOR]->entity.use_count) {
		mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
		vb2_fop_release(file);
		set_bit(VIN_LPM, &cap->state);
		clear_bit(VIN_BUSY, &cap->state);
		if (cap->pipe.sd[VIN_IND_SENSOR])
			vin_err("%s is not used, video%d cannot be close!\n", cap->pipe.sd[VIN_IND_SENSOR]->name, vinc->id);
		return -1;
	}

	if (vin_streaming(cap)) {
		clear_bit(VIN_STREAM, &cap->state);
		vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);
		if (vinc->dma_merge_mode == 1) {
			if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
				vinc_bind = vin_core_gbl[vinc->id - 1];
				clear_bit(VIN_STREAM, &vinc_bind->vid_cap.state);
				vin_pipeline_call(vinc_bind, set_stream, &vinc_bind->vid_cap.pipe, 0);
			}
		}
		vb2_ioctl_streamoff(file, NULL, V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE);
	}

	if (!vin_lpm(cap)) {
		set_bit(VIN_LPM, &cap->state);
		__csi_isp_setup_link(vinc, 0);
		__vin_sensor_setup_link(vinc, module, valid_idx, 0);
		if (vinc->dma_merge_mode == 1) {
			if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
				vinc_bind = vin_core_gbl[vinc->id - 1];
				__csi_isp_setup_link(vinc_bind, 0);
				__vin_sensor_setup_link(vinc_bind, module, valid_idx, 0);
			}
		}
	}

	if (cap->pipe.sd[VIN_IND_ACTUATOR] != NULL) {
		ret = __vin_actuator_set_power(cap->pipe.sd[VIN_IND_ACTUATOR], 0);
		if (ret < 0)
			vin_err("actutor power off failed (%d)!\n", ret);
	}

	if (cap->pipe.sd[VIN_IND_FLASH] != NULL)
		io_set_flash_ctrl(cap->pipe.sd[VIN_IND_FLASH], SW_CTRL_FLASH_OFF);

	ret = vin_pipeline_call(vinc, close, &cap->pipe);
	if (ret)
		vin_err("vin pipeline close failed!\n");

	v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 0);

	if (vinc->dma_merge_mode == 1) {
		if (vinc->id % 4 == 1 && vin_core_gbl[vinc->id - 1]) {
			vinc_bind = vin_core_gbl[vinc->id - 1];
			ret = vin_pipeline_call(vinc_bind, close, &vinc_bind->vid_cap.pipe);
			if (ret)
				vin_err("vin pipeline close failed!\n");

			v4l2_subdev_call(vinc_bind->vid_cap.pipe.sd[VIN_IND_ISP], core, init, 0);
			vinc_bind->dma_merge_mode = 0;
		}
	}
	vinc->dma_merge_mode = 0;

#ifdef SUPPORT_PTN
	if ((vinc->large_image == 2) && vinc->ptn_cfg.ptn_en) {
		os_mem_free(&vinc->pdev->dev, &vinc->ptn_cfg.ptn_buf);
		vinc->ptn_cfg.ptn_en = 0;
	}
#endif
	/* vb2_fop_release will use graph_mutex */
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);

	ret = vb2_fop_release(file); /* vb2_queue_release(&cap->vb_vidq); */
#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY)
	dramfreq_master_access(MASTER_CSI, false);
#endif
#if IS_ENABLED(CONFIG_AW_DMC_DEVFREQ) && (IS_ENABLED(CONFIG_ARCH_SUN50IW10) || !defined CSIC_SDRAM_DFS)
	vin_dfs_handler(vind, 0);
#endif
	clear_bit(VIN_BUSY, &cap->state);
	vin_log(VIN_LOG_VIDEO, "video%d close\n", vinc->id);
	return 0;
}

static unsigned int vin_poll(struct file *file, poll_table *wait)
{
	struct vin_core *vinc = video_drvdata(file);
	struct vin_vid_cap *cap = &vinc->vid_cap;

	if (!vin_streaming(cap))
		return 0;

	return vb2_fop_poll(file, wait);
}

static int vin_try_ctrl(struct v4l2_ctrl *ctrl)
{
	/*
	 * to cheat control framework, because of  when ctrl->cur.val == ctrl->val
	 * s_ctrl would not be called
	  */
	if ((ctrl->minimum == 0) && (ctrl->maximum == 1)) {
		if (ctrl->val)
			ctrl->cur.val = 0;
		else
			ctrl->cur.val = 1;
	} else {
		if (ctrl->val == ctrl->maximum)
			ctrl->cur.val = ctrl->val - 1;
		else
			ctrl->cur.val = ctrl->val + 1;
	}

	/*
	 * to cheat control framework, because of  when ctrl->flags is
	 * V4L2_CTRL_FLAG_VOLATILE, s_ctrl would not be called
	  */
	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_EXPOSURE_ABSOLUTE:
	case V4L2_CID_GAIN:
		if (ctrl->val != ctrl->cur.val)
			ctrl->flags &= ~V4L2_CTRL_FLAG_VOLATILE;
		break;
	default:
		break;
	}
	return 0;
}
static int vin_g_volatile_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vin_vid_cap *cap = container_of(ctrl->handler, struct vin_vid_cap, ctrl_handler);
	struct sensor_instance *inst = get_valid_sensor(cap->vinc);
	struct v4l2_subdev *sensor = cap->pipe.sd[VIN_IND_SENSOR];
	struct v4l2_subdev *flash = cap->pipe.sd[VIN_IND_FLASH];
	struct v4l2_control c;
	int ret = 0;

	c.id = ctrl->id;
	if (inst->is_isp_used && inst->is_bayer_raw) {
		switch (ctrl->id) {
		case V4L2_CID_EXPOSURE:
			v4l2_g_ctrl(sensor->ctrl_handler, &c);
			ctrl->val = c.value;
			break;
		case V4L2_CID_EXPOSURE_ABSOLUTE:
			c.id = V4L2_CID_EXPOSURE;
			v4l2_g_ctrl(sensor->ctrl_handler, &c);
			ctrl->val = __vin_sensor_line2time(sensor, c.value);
			break;
		case V4L2_CID_GAIN:
			v4l2_g_ctrl(sensor->ctrl_handler, &c);
			ctrl->val = c.value;
			break;
		case V4L2_CID_AE_WIN_X1:
			break;
		case V4L2_CID_AF_WIN_X1:
			break;
		case V4L2_CID_HOR_VISUAL_ANGLE:
		case V4L2_CID_VER_VISUAL_ANGLE:
		case V4L2_CID_FOCUS_LENGTH:
		case V4L2_CID_3A_LOCK:
		case V4L2_CID_AUTO_FOCUS_STATUS: /* Read-Only */
			break;
		case V4L2_CID_SENSOR_TYPE:
			ctrl->val = inst->is_bayer_raw;
			break;
		case V4L2_CID_VFLIP:
			ctrl->val = cap->vinc->vflip;
			break;
		case V4L2_CID_HFLIP:
			ctrl->val = cap->vinc->hflip;
			break;
		default:
			return -EINVAL;
		}
		return ret;
	} else {
		switch (ctrl->id) {
		case V4L2_CID_SENSOR_TYPE:
			c.value = inst->is_bayer_raw;
			break;
		case V4L2_CID_FLASH_LED_MODE:
			ret = v4l2_g_ctrl(flash->ctrl_handler, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_STATUS:
			ret = v4l2_g_ctrl(sensor->ctrl_handler, &c);
			if (c.value != V4L2_AUTO_FOCUS_STATUS_BUSY)
				sunxi_flash_stop(flash);
			break;
		case V4L2_CID_VFLIP:
			ctrl->val = cap->vinc->vflip;
			break;
		case V4L2_CID_HFLIP:
			ctrl->val = cap->vinc->hflip;
			break;
		default:
			ret = v4l2_g_ctrl(sensor->ctrl_handler, &c);
			break;
		}
		ctrl->val = c.value;
		if (ret < 0)
			vin_warn("v4l2 sub device g_ctrl fail!\n");
	}
	return ret;
}

int sensor_flip_option(struct vin_vid_cap *cap, struct v4l2_control c)
{
#if !defined ISP_600
	struct v4l2_subdev *isp = cap->pipe.sd[VIN_IND_ISP];
	struct v4l2_subdev *csi =  cap->pipe.sd[VIN_IND_CSI];
	struct v4l2_subdev *sensor = cap->pipe.sd[VIN_IND_SENSOR];
	struct isp_dev *isp_device = v4l2_get_subdevdata(isp);
	struct csi_dev *csi_device = v4l2_get_subdevdata(csi);
	struct vin_md *vind = dev_get_drvdata(isp->v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	unsigned int isp_stream_count;
	int i = 0;
	int input_seq = 0;
	int sensor_fmt_code = 0;
	int ret;
	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	isp_stream_count = isp_device->subdev.entity.stream_count;
	isp_device->subdev.entity.stream_count = 0;
	csic_prs_capture_stop(csi_device->id);

	ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
	v4l2_subdev_call(sensor, core, ioctl, VIDIOC_VIN_GET_SENSOR_CODE, &sensor_fmt_code);
	switch (sensor_fmt_code) {
	case MEDIA_BUS_FMT_SBGGR8_1X8:
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	case MEDIA_BUS_FMT_SBGGR12_1X12:
		input_seq = ISP_BGGR;
		break;
	case MEDIA_BUS_FMT_SGBRG8_1X8:
	case MEDIA_BUS_FMT_SGBRG10_1X10:
	case MEDIA_BUS_FMT_SGBRG12_1X12:
		input_seq = ISP_GBRG;
		break;
	case MEDIA_BUS_FMT_SGRBG8_1X8:
	case MEDIA_BUS_FMT_SGRBG10_1X10:
	case MEDIA_BUS_FMT_SGRBG12_1X12:
		input_seq = ISP_GRBG;
		break;
	case MEDIA_BUS_FMT_SRGGB8_1X8:
	case MEDIA_BUS_FMT_SRGGB10_1X10:
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		input_seq = ISP_RGGB;
		break;
	default:
		input_seq = ISP_BGGR;
		break;
	}
	if (isp_device->use_isp) {
		vin_print("%s:isp%d reset!!!\n", __func__, isp_device->id);
		bsp_isp_set_para_ready(isp_device->id, PARA_NOT_READY);
	}
#if IS_ENABLED(CONFIG_D3D)
	if (isp_device->use_isp && (isp_device->load_shadow[0x2d4 + 0x3]) & (1<<1)) {
		/*  clear D3D rec_en 0x2d4 bit25 */
		isp_device->load_shadow[0x2d4 + 0x3] = (isp_device->load_shadow[0x2d4 + 0x3]) & (~(1<<1));
		memcpy(isp_device->isp_load.vir_addr, &isp_device->load_shadow[0], ISP_LOAD_DRAM_SIZE);
	}
#endif
	/* ****************stop****************** */
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
	if (csi_device->id == 0)
		cmb_rx_disable(csi_device->id);
#endif
	csic_prs_disable(csi_device->id);

	if (isp_device->use_isp) {
		csic_isp_bridge_disable(0);

		bsp_isp_clr_irq_status(isp_device->id, ISP_IRQ_EN_ALL);
		bsp_isp_enable(isp_device->id, 0);
		bsp_isp_capture_stop(isp_device->id);
	}
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		if (vind->vinc[i]->csi_sel == csi_device->id) {
			vinc = vind->vinc[i];

			vinc->vid_cap.frame_delay_cnt = 2;
			vipp_disable(vinc->vipp_sel);
			vipp_top_clk_en(vinc->vipp_sel, 0);
			csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
			csic_dma_top_disable(vinc->vipp_sel);
		}
	}

	/* ****************start****************** */
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		if (vind->vinc[i]->csi_sel == csi_device->id) {
			vinc = vind->vinc[i];

			csic_dma_top_enable(vinc->vipp_sel);
			vipp_top_clk_en(vinc->vipp_sel, 1);
			vipp_enable(vinc->vipp_sel);
			vinc->vin_status.frame_cnt = 0;
			vinc->vin_status.lost_cnt = 0;
		}
	}
	if (isp_device->use_isp) {
		bsp_isp_enable(isp_device->id, 1);
		bsp_isp_set_para_ready(isp_device->id, PARA_READY);
		bsp_isp_set_input_fmt(isp_device->id, input_seq);
		bsp_isp_capture_start(isp_device->id);
		isp_device->isp_frame_number = 0;

		csic_isp_bridge_enable(0);
	}

	csic_prs_enable(csi_device->id);

#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
	if (vinc->mipi_sel == 0)
		cmb_rx_enable(vinc->mipi_sel);
#endif

	csic_prs_capture_start(csi_device->id, csi_device->bus_info.ch_total_num, &mode);

	isp_device->subdev.entity.stream_count = isp_stream_count;
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
#else /* else ISP_600 */
	struct v4l2_subdev *isp = cap->pipe.sd[VIN_IND_ISP];
	struct v4l2_subdev *sensor = cap->pipe.sd[VIN_IND_SENSOR];
	struct isp_dev *isp_device = v4l2_get_subdevdata(isp);
	int input_seq;
	int sensor_fmt_code = 0;
	int ret;

	ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
	v4l2_subdev_call(sensor, core, ioctl, VIDIOC_VIN_GET_SENSOR_CODE, &sensor_fmt_code);
	if (sensor_fmt_code <= 0) {
		vin_err("cannot get sensor code!\n");
	} else {
		switch (sensor_fmt_code) {
		case MEDIA_BUS_FMT_SBGGR8_1X8:
		case MEDIA_BUS_FMT_SBGGR10_1X10:
		case MEDIA_BUS_FMT_SBGGR12_1X12:
			input_seq = ISP_BGGR;
			break;
		case MEDIA_BUS_FMT_SGBRG8_1X8:
		case MEDIA_BUS_FMT_SGBRG10_1X10:
		case MEDIA_BUS_FMT_SGBRG12_1X12:
			input_seq = ISP_GBRG;
			break;
		case MEDIA_BUS_FMT_SGRBG8_1X8:
		case MEDIA_BUS_FMT_SGRBG10_1X10:
		case MEDIA_BUS_FMT_SGRBG12_1X12:
			input_seq = ISP_GRBG;
			break;
		case MEDIA_BUS_FMT_SRGGB8_1X8:
		case MEDIA_BUS_FMT_SRGGB10_1X10:
		case MEDIA_BUS_FMT_SRGGB12_1X12:
			input_seq = ISP_RGGB;
			break;
		default:
			input_seq = ISP_BGGR;
			break;
		}
	vin_log(VIN_LOG_VIDEO, "sensor_flip_option sensor_fmt=%d\n", input_seq);
	isp_device->isp_fmt->infmt = input_seq;
	}
	cap->frame_delay_cnt = 3;
#endif
	return ret;
}

static int vin_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct vin_vid_cap *cap = container_of(ctrl->handler, struct vin_vid_cap, ctrl_handler);
	struct sensor_instance *inst = get_valid_sensor(cap->vinc);
	struct v4l2_subdev *sensor = cap->pipe.sd[VIN_IND_SENSOR];
	struct v4l2_subdev *flash = cap->pipe.sd[VIN_IND_FLASH];
	struct v4l2_subdev *act = cap->pipe.sd[VIN_IND_ACTUATOR];
	struct v4l2_subdev *isp = cap->pipe.sd[VIN_IND_ISP];
	struct actuator_ctrl_word_t vcm_ctrl;
	struct v4l2_control c;
#if !IS_ENABLED(CONFIG_ENABLE_SENSOR_FLIP_OPTION)
	struct csic_dma_flip flip;
#endif
	int ret = 0;

	/*
	 * make sure g_ctrl will get the value that hardware is using
	 * so that ctrl->flags should be V4L2_CTRL_FLAG_VOLATILE, after s_ctrl
	  */
	switch (ctrl->id) {
	case V4L2_CID_VFLIP:
	case V4L2_CID_HFLIP:
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_EXPOSURE_ABSOLUTE:
	case V4L2_CID_GAIN:
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
		break;
	default:
		break;
	}

	c.id = ctrl->id;
	c.value = ctrl->val;
	switch (ctrl->id) {
#if IS_ENABLED(CONFIG_ENABLE_SENSOR_FLIP_OPTION)
	case V4L2_CID_VFLIP:
		if (!vin_streaming(cap)) {
			vin_err("cannot set sensor flip before stream on!\n");
			return -1;
		}
		cap->vinc->sensor_vflip = c.value;
		ret = sensor_flip_option(cap, c);
		return ret;
	case V4L2_CID_HFLIP:
		if (!vin_streaming(cap)) {
			vin_err("cannot set sensor flip before stream on!\n");
			return -1;
		}
		cap->vinc->sensor_hflip = c.value;
		ret = sensor_flip_option(cap, c);
		return ret;
#else
	case V4L2_CID_VFLIP:
		if (cap->frame.fmt.fourcc == V4L2_PIX_FMT_LBC_2_0X ||
		    cap->frame.fmt.fourcc == V4L2_PIX_FMT_LBC_2_5X ||
		    cap->frame.fmt.fourcc == V4L2_PIX_FMT_LBC_1_0X) {
			vin_warn("when out fmt is LBC, FLIP is not support!\n");
			return -1;
		}
		cap->vinc->vflip = c.value;
		if (!vin_lpm(cap)) {
			flip.hflip_en = cap->vinc->hflip;
			flip.vflip_en = cap->vinc->vflip;
			csic_dma_flip_en(cap->vinc->vipp_sel, &flip);
			__osd_reg_setup(cap->vinc, &cap->osd);
			return 0;
		} else {
			vin_err("cannot set vflip before s_input, in low power mode!\n");
			return -1;
		}
	case V4L2_CID_HFLIP:
		if (cap->frame.fmt.fourcc == V4L2_PIX_FMT_LBC_2_0X ||
		    cap->frame.fmt.fourcc == V4L2_PIX_FMT_LBC_2_5X ||
		    cap->frame.fmt.fourcc == V4L2_PIX_FMT_LBC_1_0X) {
			vin_warn("when out fmt is LBC, FLIP is not support!\n");
			return -1;
		}
		cap->vinc->hflip = c.value;
		if (!vin_lpm(cap)) {
			flip.hflip_en = cap->vinc->hflip;
			flip.vflip_en = cap->vinc->vflip;
			csic_dma_flip_en(cap->vinc->vipp_sel, &flip);
			__osd_reg_setup(cap->vinc, &cap->osd);
			return 0;
		} else {
			vin_err("cannot set hflip before s_input, in low power mode!\n");
			return -1;
		}
#endif
	default:
		break;
	}

	/*
	 * make sure g_ctrl will get the value that hardware is using
	 * so that ctrl->flags should be V4L2_CTRL_FLAG_VOLATILE, after s_ctrl
	  */
	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
	case V4L2_CID_EXPOSURE_ABSOLUTE:
	case V4L2_CID_GAIN:
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
		break;
	default:
		break;
	}

	if (inst->is_isp_used && inst->is_bayer_raw) {
		switch (ctrl->id) {
		case V4L2_CID_BRIGHTNESS:
		case V4L2_CID_CONTRAST:
		case V4L2_CID_SATURATION:
		case V4L2_CID_HUE:
		case V4L2_CID_AUTO_WHITE_BALANCE:
		case V4L2_CID_EXPOSURE:
		case V4L2_CID_AUTOGAIN:
		case V4L2_CID_GAIN:
		case V4L2_CID_POWER_LINE_FREQUENCY:
		case V4L2_CID_HUE_AUTO:
		case V4L2_CID_WHITE_BALANCE_TEMPERATURE:
		case V4L2_CID_SHARPNESS:
		case V4L2_CID_CHROMA_AGC:
		case V4L2_CID_COLORFX:
		case V4L2_CID_AUTOBRIGHTNESS:
		case V4L2_CID_BAND_STOP_FILTER:
		case V4L2_CID_ILLUMINATORS_1:
		case V4L2_CID_ILLUMINATORS_2:
		case V4L2_CID_EXPOSURE_AUTO:
		case V4L2_CID_EXPOSURE_ABSOLUTE:
		case V4L2_CID_EXPOSURE_AUTO_PRIORITY:
		case V4L2_CID_FOCUS_ABSOLUTE:
		case V4L2_CID_FOCUS_RELATIVE:
		case V4L2_CID_FOCUS_AUTO:
		case V4L2_CID_AUTO_EXPOSURE_BIAS:
		case V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE:
		case V4L2_CID_WIDE_DYNAMIC_RANGE:
		case V4L2_CID_IMAGE_STABILIZATION:
		case V4L2_CID_ISO_SENSITIVITY:
		case V4L2_CID_ISO_SENSITIVITY_AUTO:
		case V4L2_CID_EXPOSURE_METERING:
		case V4L2_CID_SCENE_MODE:
		case V4L2_CID_3A_LOCK:
		case V4L2_CID_AUTO_FOCUS_START:
		case V4L2_CID_AUTO_FOCUS_STOP:
		case V4L2_CID_AUTO_FOCUS_RANGE:
		case V4L2_CID_AUTO_FOCUS_INIT:
		case V4L2_CID_AUTO_FOCUS_RELEASE:
		case V4L2_CID_GSENSOR_ROTATION:
		case V4L2_CID_TAKE_PICTURE:
			ret = v4l2_s_ctrl(NULL, isp->ctrl_handler, &c);
			break;
		case V4L2_CID_FLASH_LED_MODE:
			ret = v4l2_s_ctrl(NULL, isp->ctrl_handler, &c);
			 if (flash)
				ret = v4l2_s_ctrl(NULL, flash->ctrl_handler, &c);
			break;
		case V4L2_CID_FLASH_LED_MODE_V1:
			ret = v4l2_s_ctrl(NULL, isp->ctrl_handler, &c);
			if (flash)
				ret = v4l2_s_ctrl(NULL, flash->ctrl_handler, &c);
			break;
		default:
			ret = -EINVAL;
			break;
		}
	} else {
		switch (ctrl->id) {
		case V4L2_CID_FOCUS_ABSOLUTE:
			vcm_ctrl.code = ctrl->val;
			vcm_ctrl.sr = 0x0;
			ret = v4l2_subdev_call(act, core, ioctl, ACT_SET_CODE, &vcm_ctrl);
			break;
		case V4L2_CID_FLASH_LED_MODE:
			if (flash)
				ret = v4l2_s_ctrl(NULL, flash->ctrl_handler, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_START:
			if (flash)
				sunxi_flash_check_to_start(flash, SW_CTRL_TORCH_ON);
			ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
			break;
		case V4L2_CID_AUTO_FOCUS_STOP:
			if (flash)
				sunxi_flash_stop(flash);
			ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
			break;
		case V4L2_CID_AE_WIN_X1:
			ret = __vin_sensor_set_ae_win(cap);
			break;
		case V4L2_CID_AF_WIN_X1:
			ret = __vin_sensor_set_af_win(cap);
			break;
		case V4L2_CID_AUTO_EXPOSURE_BIAS:
			c.value = ctrl->val;
			ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
			break;
		default:
			ret = v4l2_s_ctrl(NULL, sensor->ctrl_handler, &c);
			break;
		}
	}
	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)
static long vin_compat_ioctl32(struct file *file, unsigned int cmd,
			       unsigned long arg)
{
	void __user *up = compat_ptr(arg);
	long err = 0;

	err = video_ioctl2(file, cmd, (unsigned long)up);
	return err;
}
#endif
/*  ------------------------------------------------------------------
 *File operations for the device
 *------------------------------------------------------------------ */

static const struct v4l2_ctrl_ops vin_ctrl_ops = {
	.g_volatile_ctrl = vin_g_volatile_ctrl,
	.s_ctrl = vin_s_ctrl,
	.try_ctrl = vin_try_ctrl,
};

static const struct v4l2_file_operations vin_fops = {
	.owner = THIS_MODULE,
	.open = vin_open,
	.release = vin_close,
	.read = vb2_fop_read,
	.poll = vin_poll,
	.unlocked_ioctl = video_ioctl2,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = vin_compat_ioctl32,
#endif
	.mmap = vb2_fop_mmap,
};

static const struct v4l2_ioctl_ops vin_ioctl_ops = {
	.vidioc_querycap = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap = vidioc_enum_fmt_vid_cap_mplane,
	.vidioc_enum_framesizes = vidioc_enum_framesizes,
	.vidioc_enum_frameintervals = vidioc_enum_frameintervals,
	.vidioc_g_fmt_vid_cap_mplane = vidioc_g_fmt_vid_cap_mplane,
	.vidioc_try_fmt_vid_cap_mplane = vidioc_try_fmt_vid_cap_mplane,
	.vidioc_s_fmt_vid_cap_mplane = vidioc_s_fmt_vid_cap_mplane,
	.vidioc_enum_fmt_vid_overlay = vidioc_enum_fmt_vid_overlay,
	.vidioc_g_fmt_vid_overlay = vidioc_g_fmt_vid_overlay,
	.vidioc_try_fmt_vid_overlay = vidioc_try_fmt_vid_overlay,
	.vidioc_s_fmt_vid_overlay = vidioc_s_fmt_vid_overlay,
	.vidioc_overlay = vidioc_overlay,
	.vidioc_reqbufs = vb2_ioctl_reqbufs,
	.vidioc_querybuf = vb2_ioctl_querybuf,
	.vidioc_qbuf = vb2_ioctl_qbuf,
	.vidioc_dqbuf = vb2_ioctl_dqbuf,
	.vidioc_expbuf = vb2_ioctl_expbuf,
	.vidioc_enum_input = vidioc_enum_input,
	.vidioc_g_input = vidioc_g_input,
	.vidioc_s_input = vidioc_s_input,
	.vidioc_streamon = vidioc_streamon,
	.vidioc_streamoff = vidioc_streamoff,
	.vidioc_g_pixelaspect = vidioc_g_pixelaspect,
	.vidioc_g_parm = vidioc_g_parm,
	.vidioc_s_parm = vidioc_s_parm,
	.vidioc_g_selection = vidioc_g_selection,
	.vidioc_s_selection = vidioc_s_selection,
	.vidioc_s_dv_timings = vidioc_s_dv_timings,
	.vidioc_g_dv_timings = vidioc_g_dv_timings,
	.vidioc_query_dv_timings = vidioc_query_dv_timings,
	.vidioc_enum_dv_timings = vidioc_enum_dv_timings,
	.vidioc_dv_timings_cap = vidioc_dv_timings_cap,
	.vidioc_default = vin_param_handler,
	.vidioc_subscribe_event = vin_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_s_edid = vidioc_s_edid,
};

#if IS_ENABLED(CONFIG_VIDEO_SUNXI_VIN_SPECIAL)

#define VIN_VIDEO_SOURCE_WIDTH_DEFAULT  1280
#define VIN_VIDEO_SOURCE_HEIGHT_DEFAULT 720

struct device *vin_get_dev(int id)
{
	struct vin_core *vinc = vin_core_gbl[id];

	return get_device(&vinc->pdev->dev);
}
EXPORT_SYMBOL(vin_get_dev);

int vin_s_ctrl_special(int id, unsigned int ctrl_id, int val)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct v4l2_control c;

	c.id = ctrl_id;
	c.value = val;

	return v4l2_s_ctrl(NULL, &cap->ctrl_handler, &c);
}
EXPORT_SYMBOL(vin_s_ctrl_special);

int vin_open_special(int id)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct list_head *active = &vinc->vid_cap.vidq_active;
	struct list_head *done = &vinc->vid_cap.vidq_done;
	struct vin_vid_cap *cap = &vinc->vid_cap;

	if (vin_busy(&vinc->vid_cap)) {
		vin_err("device open busy\n");
		return -EBUSY;
	}

#if IS_ENABLED(CONFIG_RV_RUN_CAR_REVERSE)
	if (CONTROL_BY_RTOS == vinc->rpmsg.control) {
		vin_err("video%d is controlling by rtos\n", vinc->id);
		return -EBUSY;
	}

	vinc_status_rpmsg_send(ARM_VIN_START, &vinc->rpmsg);
#endif

	INIT_LIST_HEAD(active);
	INIT_LIST_HEAD(done);
	vinc->vid_cap.special_active = 1;

	set_bit(VIN_BUSY, &vinc->vid_cap.state);
	set_bit(VIN_LPM, &cap->state);

#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY)
	dramfreq_master_access(MASTER_CSI, true);
#endif
	return 0;
}
EXPORT_SYMBOL(vin_open_special);

int vin_s_input_special(int id, int i)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_md *vind = NULL;
	struct vin_vid_cap *cap = NULL;
	struct modules_config *module = NULL;
	struct sensor_instance *inst = NULL;
	struct sensor_info *info = NULL;

	int valid_idx = -1;
	int ret;

	if (!vinc) {
		vin_err("%s vinc %px\n", __FUNCTION__, vinc);
		return -EINVAL;
	}
	if (!vinc->v4l2_dev) {
		vin_err("%s vinc->v4l2_dev %px\n", __FUNCTION__, vinc->v4l2_dev);
		return -EINVAL;
	}
	if (!vinc->v4l2_dev->dev) {
		vin_err("%s vinc->v4l2_dev->dev %px\n", __FUNCTION__, vinc->v4l2_dev->dev);
		return -EINVAL;
	}

	vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	cap = &vinc->vid_cap;
	i = i > 1 ? 0 : i;

	if (i == 0)
		vinc->sensor_sel = vinc->rear_sensor;
	else
		vinc->sensor_sel = vinc->front_sensor;

	module = &vind->modules[vinc->sensor_sel];
	valid_idx = module->sensors.valid_idx;

	if (valid_idx == NO_VALID_SENSOR) {
		vin_err("there is no valid sensor\n");
		return -EINVAL;
	}

	if (__vin_sensor_setup_link(vinc, module, valid_idx, 1) < 0) {
		vin_err("sensor setup link failed\n");
		return -EINVAL;
	}
	if (__csi_isp_setup_link(vinc, 1) < 0) {
		vin_err("csi&isp setup link failed\n");
		return -EINVAL;
	}

	ret = vin_pipeline_call(vinc, open, &cap->pipe, &cap->vdev.entity, true);
	if (ret < 0) {
		vin_err("vin pipeline open failed (%d)!\n", ret);
		return ret;
	}

	inst = &module->sensors.inst[valid_idx];
	sunxi_isp_sensor_type(cap->pipe.sd[VIN_IND_ISP], inst->is_isp_used);
	vinc->support_raw = inst->is_isp_used;

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 1);
	if (ret < 0) {
		vin_err("ISP init error at %s\n", __func__);
		return ret;
	}

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SCALER], core, init, 1);
	if (ret < 0) {
		vin_err("SCALER init error at %s\n", __func__);
		return ret;
	}

	/* save exp and gain for reopen, sensor init may reset gain to 0, so save before init! */
	info = container_of(cap->pipe.sd[VIN_IND_SENSOR], struct sensor_info, sd);
	if (info) {
		vinc->exp_gain.exp_val = info->exp;
		vinc->exp_gain.gain_val = info->gain;
		vinc->stream_idx = info->stream_seq + 2;
	}

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, init, 1);
	if (ret) {
		vin_err("sensor initial error when selecting target device!\n");
		return ret;
	}
	clear_bit(VIN_LPM, &cap->state);

	vinc->hflip = inst->hflip;
	vinc->vflip = inst->vflip;

	return ret;
}
EXPORT_SYMBOL(vin_s_input_special);

int vin_s_parm_special(int id, void *priv,
			 struct v4l2_streamparm *parms)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct sensor_instance *inst = get_valid_sensor(vinc);
	int ret = 0;
	if (parms->parm.capture.capturemode != V4L2_MODE_VIDEO &&
	    parms->parm.capture.capturemode != V4L2_MODE_IMAGE &&
	    parms->parm.capture.capturemode != V4L2_MODE_PREVIEW) {
		parms->parm.capture.capturemode = V4L2_MODE_PREVIEW;
	}
	cap->capture_mode = parms->parm.capture.capturemode;
	vinc->large_image = parms->parm.capture.reserved[2];
	ret = sensor_s_parm(cap->pipe.sd[VIN_IND_SENSOR], parms);
	if (ret < 0)
		vin_warn("v4l2 subdev sensor s_parm error!\n");
	ret = sunxi_csi_subdev_s_parm(cap->pipe.sd[VIN_IND_CSI], parms);
	if (ret < 0)
		vin_warn("v4l2 subdev csi s_parm error!\n");
	if (inst->is_isp_used) {
		ret = sunxi_isp_s_parm(cap->pipe.sd[VIN_IND_ISP], parms);
		if (ret < 0)
			vin_warn("v4l2 subdev isp s_parm error!\n");
	}
	return ret;
}
EXPORT_SYMBOL(vin_s_parm_special);

int vin_tvin_special(int id, struct tvin_init_info *info)
{
	struct vin_core *vinc = vin_core_gbl[id];
	int ret = 0;

	if (__tvin_info_check(info))
		return -1;

	vinc->tvin.work_mode = info->work_mode;
	vinc->tvin.input_fmt = info->input_fmt[info->ch_id];
	vinc->tvin.flag = true;

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
			SENSOR_TVIN_INIT, info);
	if (ret)
		vin_err("sensor tvin init fail!\n");

	if (vinc->vid_cap.pipe.sd[VIN_IND_MIPI] != NULL) {
		ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_MIPI], core, ioctl,
				MIPI_TVIN_INIT, info);
		if (ret)
			vin_err("mipi tvin init fail!\n");
	}

	ret = v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_CSI], core, ioctl,
			PARSER_TVIN_INIT, info);
	if (ret)
		vin_err("csi tvin init fail!\n");

	vin_log(VIN_LOG_FMT, "%s mode %d, fmt %d\n", __func__,
				vinc->tvin.work_mode, vinc->tvin.input_fmt);

	return ret;
}
EXPORT_SYMBOL(vin_tvin_special);

int vin_close_special(int id)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct list_head *active = &vinc->vid_cap.vidq_active;
	struct list_head *done = &vinc->vid_cap.vidq_done;
	struct modules_config *module = &vind->modules[vinc->sensor_sel];
	int valid_idx = module->sensors.valid_idx;
	int ret;

	INIT_LIST_HEAD(active);
	INIT_LIST_HEAD(done);
	vinc->vid_cap.special_active = 0;

	if (!vin_busy(cap)) {
		vin_warn("video%d device have been closed!\n", vinc->id);
		return 0;
	}

#if IS_ENABLED(CONFIG_RV_RUN_CAR_REVERSE)
	vinc_status_rpmsg_send(ARM_VIN_STOP, &vinc->rpmsg);
#endif

	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	if (!cap->pipe.sd[VIN_IND_SENSOR] || !cap->pipe.sd[VIN_IND_SENSOR]->entity.use_count) {
		clear_bit(VIN_BUSY, &cap->state);
		vin_err("%s is not used, video%d cannot be close!\n", cap->pipe.sd[VIN_IND_SENSOR]->name, vinc->id);
		mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
		return -1;
	}

	if (vin_streaming(cap)) {
#if VIN_FALSE
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
		isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
		if (isp->gtm_type == 4) {
			if ((vinc->id == 0) && (!check_ldci_video_relate(vinc->id, LDCI0_VIDEO_CHN))) {
				disable_ldci_video(LDCI0_VIDEO_CHN);
			} else if ((vinc->id == 1) && (!check_ldci_video_relate(vinc->id, LDCI1_VIDEO_CHN))) {
				disable_ldci_video(LDCI1_VIDEO_CHN);
			}
		}
#endif
#endif
		clear_bit(VIN_STREAM, &cap->state);
		vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);
	}

	if (!vin_lpm(cap)) {
			set_bit(VIN_LPM, &cap->state);
		__csi_isp_setup_link(vinc, 0);
		__vin_sensor_setup_link(vinc, module, valid_idx, 0);
	}

	ret = vin_pipeline_call(vinc, close, &cap->pipe);
	if (ret)
		vin_err("vin pipeline close failed!\n");

	ret = v4l2_subdev_call(cap->pipe.sd[VIN_IND_ISP], core, init, 0);

	/* software */
	clear_bit(VIN_BUSY, &cap->state);
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
#if IS_ENABLED(CONFIG_DEVFREQ_DRAM_FREQ_WITH_SOFT_NOTIFY)
	dramfreq_master_access(MASTER_CSI, false);
#endif
	vin_log(VIN_LOG_VIDEO, "video%d close\n", vinc->id);
	return 0;
}
EXPORT_SYMBOL(vin_close_special);

int vin_s_fmt_special(int id, struct v4l2_format *f)
{
	struct vin_core *vinc = vin_core_gbl[id];

	return __vin_set_fmt(vinc, f);
}
EXPORT_SYMBOL(vin_s_fmt_special);

int vin_g_fmt_special(int id, struct v4l2_format *f)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct v4l2_pix_format_mplane *pixm = &f->fmt.pix_mp;
	int i;

	pixm->width = vinc->vid_cap.frame.o_width;
	pixm->height = vinc->vid_cap.frame.o_height;
	pixm->field = V4L2_FIELD_NONE;
	pixm->pixelformat = vinc->vid_cap.frame.fmt.fourcc;
	pixm->colorspace = vinc->vid_cap.frame.fmt.color;/* V4L2_COLORSPACE_JPEG; */
	pixm->num_planes = vinc->vid_cap.frame.fmt.memplanes;
	for (i = 0; i < pixm->num_planes; ++i) {
		pixm->plane_fmt[i].bytesperline = vinc->vid_cap.frame.bytesperline[i];
		pixm->plane_fmt[i].sizeimage = vinc->vid_cap.frame.payload[i];
	}

	return 0;
}
EXPORT_SYMBOL(vin_g_fmt_special);

int vin_g_fmt_special_ext(int id, struct v4l2_format *f)
{
	struct vin_core *vinc = vin_core_gbl[id];

	/* size resolution should be configurated flexiable for various platform*/
#if defined VIN_VIDEO_SOURCE_WIDTH
	f->fmt.pix.width        = VIN_VIDEO_SOURCE_WIDTH;
#else
	f->fmt.pix.width        = VIN_VIDEO_SOURCE_WIDTH_DEFAULT;
#endif

#if defined VIN_VIDEO_SOURCE_HEIGHT
	f->fmt.pix.height       = VIN_VIDEO_SOURCE_HEIGHT;
#else
	f->fmt.pix.height        = VIN_VIDEO_SOURCE_HEIGHT_DEFAULT;
#endif
	f->fmt.pix.field        = vinc->vid_cap.frame.fmt.field;
	f->fmt.pix.pixelformat  = vinc->vid_cap.frame.fmt.mbus_code;

	return 0;
}
EXPORT_SYMBOL(vin_g_fmt_special_ext);

int vin_dqbuffer_special(int id, struct vin_buffer **buf)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct list_head *done = &vinc->vid_cap.vidq_done;
	struct vin_vid_cap *cap = &vinc->vid_cap;

	int ret = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&cap->slock, flags);
	/*  Dequeue buffers have handled done */
	if (!list_empty(done)) {
		*buf = list_first_entry(done, struct vin_buffer, list);
		list_del(&((*buf)->list));
		(*buf)->state = VB2_BUF_STATE_DEQUEUED;
		spin_unlock_irqrestore(&cap->slock, flags);

		dma_buf_unmap_attachment((*buf)->attachment, (*buf)->sgt, DMA_FROM_DEVICE);
		dma_buf_detach((*buf)->dmabuf, (*buf)->attachment);

		dma_buf_put((*buf)->dmabuf);
	} else {
		spin_unlock_irqrestore(&cap->slock, flags);
		ret = -1;
	}

	return ret;
}
EXPORT_SYMBOL(vin_dqbuffer_special);

int vin_qbuffer_special(int id, struct vin_buffer *buf)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_vid_cap *cap  = &vinc->vid_cap;
	struct dma_buf_attachment *attachment;
	struct device *dev = vin_get_dev(id);
	struct sg_table *sgt;
	unsigned long flags = 0;
	int ret = 0;

	if (buf == NULL) {
		vin_err("vin buf is NULL, cannot qbuf\n");
		return -1;
	}

	buf->dmabuf = dma_buf_get(buf->dmabuf_fd);

	attachment = dma_buf_attach(buf->dmabuf, dev);
	if (IS_ERR(attachment)) {
		pr_err("dma_buf_attach failed\n");
		goto err_buf_put;
	}
	sgt = dma_buf_map_attachment(attachment, DMA_BIDIRECTIONAL);
	if (IS_ERR_OR_NULL(sgt)) {
		pr_warn("dma_buf_map_attachment failed\n");
		goto err_buf_detach;
	}

	buf->attachment = attachment;
	buf->sgt = sgt;

	buf->paddr = (void *)sg_dma_address(sgt->sgl);

	spin_lock_irqsave(&cap->slock, flags);
	list_add_tail(&buf->list, &cap->vidq_active);
	buf->state = VB2_BUF_STATE_QUEUED;
	spin_unlock_irqrestore(&cap->slock, flags);

	return ret;

err_buf_detach:
	dma_buf_detach(buf->dmabuf, attachment);
err_buf_put:
	dma_buf_put(buf->dmabuf);
	return -ENOMEM;
}
EXPORT_SYMBOL(vin_qbuffer_special);


int vin_rt_dqbuffer_special(int id, struct vin_buffer **buf)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct list_head *done = &vinc->vid_cap.vidq_done;
	struct vin_vid_cap *cap = &vinc->vid_cap;

	int ret = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&cap->slock, flags);
	/* Release all active buffers */
	if (!list_empty(done)) {
		*buf = list_first_entry(done, struct vin_buffer, list);
		list_del(&((*buf)->list));
		(*buf)->state = VB2_BUF_STATE_DEQUEUED;
	} else {
		ret = -1;
	}
	spin_unlock_irqrestore(&cap->slock, flags);

	return ret;
}
EXPORT_SYMBOL(vin_rt_dqbuffer_special);

int vin_rt_qbuffer_special(int id, struct vin_buffer *buf)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_vid_cap *cap  = &vinc->vid_cap;
	unsigned long flags = 0;
	int ret = 0;

	if (buf == NULL) {
		vin_err("buf is NULL, cannot qbuf\n");
		return -1;
	}

	spin_lock_irqsave(&cap->slock, flags);
	list_add_tail(&buf->list, &cap->vidq_active);
	buf->qbufed = 1;
	buf->state = VB2_BUF_STATE_QUEUED;
	spin_unlock_irqrestore(&cap->slock, flags);

	return ret;
}
EXPORT_SYMBOL(vin_rt_qbuffer_special);



int vin_streamon_special(int video_id, enum v4l2_buf_type i)
{
	struct vin_core *vinc = vin_core_gbl[video_id];
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct modules_config *module = &vind->modules[vinc->sensor_sel];
	int valid_idx = module->sensors.valid_idx;
	__maybe_unused struct isp_dev *isp = NULL;
	int ret = 0;
	int wth;

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = -EINVAL;
		goto streamon_error;
	}

	if (vin_streaming(cap)) {
		vin_err("stream has been already on\n");
		ret = -1;
		goto streamon_error;
	}

	switch (cap->frame.fmt.fourcc) {
	case V4L2_PIX_FMT_LBC_2_0X:
	case V4L2_PIX_FMT_LBC_2_5X:
	case V4L2_PIX_FMT_LBC_1_0X:
	case V4L2_PIX_FMT_LBC_1_5X:
		lbc_mode_select(&cap->lbc_cmp, cap->frame.fmt.fourcc);
		wth = roundup(cap->frame.o_width, 32);
		if (cap->lbc_cmp.is_lossy) {
			cap->lbc_cmp.line_tar_bits[0] = roundup(cap->lbc_cmp.cmp_ratio_even * wth * cap->lbc_cmp.bit_depth/1000, 512);
			cap->lbc_cmp.line_tar_bits[1] = roundup(cap->lbc_cmp.cmp_ratio_odd * wth * cap->lbc_cmp.bit_depth/500, 512);
		} else {
			cap->lbc_cmp.line_tar_bits[0] = roundup(wth * cap->lbc_cmp.bit_depth * 1 + (wth * 1 / 16 * 2), 512);
			cap->lbc_cmp.line_tar_bits[1] = roundup(wth * cap->lbc_cmp.bit_depth * 2 + (wth * 2 / 16 * 2), 512);
		}
		break;
	default:
		break;
	}
#if VIN_FALSE
	schedule_work(&vinc->vid_cap.s_stream_task);
#else
	if (vin_lpm(cap)) {
		if (__vin_sensor_setup_link(vinc, module, valid_idx, 1) < 0) {
			vin_err("sensor setup link failed\n");
			return -EINVAL;
		}
		if (__csi_isp_setup_link(vinc, 1) < 0) {
			vin_err("csi&isp setup link failed\n");
			return -EINVAL;
		}
		clear_bit(VIN_LPM, &cap->state);
	}
	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	ret = vin_pipeline_call(cap->vinc, set_stream, &cap->pipe, cap->vinc->stream_idx);
	if (ret < 0)
		vin_err("video%d %s error!\n", vinc->id, __func__);
	set_bit(VIN_STREAM, &cap->state);
	/* set saved exp and gain for reopen, you can call the api in sensor_reg_init */
	/*
	if (cap->vinc->exp_gain.exp_val && cap->vinc->exp_gain.gain_val) {
		v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl,
		VIDIOC_VIN_SENSOR_EXP_GAIN, &cap->vinc->exp_gain);
	}
	 */
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);

#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
	if (isp->gtm_type == 4) {
		if ((vinc->id == 0) && (!check_ldci_video_relate(vinc->id, LDCI0_VIDEO_CHN))) {
			enable_ldci_video(LDCI0_VIDEO_CHN);
		} else if ((vinc->id == 1) && (!check_ldci_video_relate(vinc->id, LDCI1_VIDEO_CHN))) {
			enable_ldci_video(LDCI1_VIDEO_CHN);
		} else if ((vinc->id == 2) && (!check_ldci_video_relate(vinc->id, LDCI2_VIDEO_CHN))) {
			enable_ldci_video(LDCI2_VIDEO_CHN);
		}
	}
#endif
#endif

streamon_error:
	return ret;
}
EXPORT_SYMBOL(vin_streamon_special);

int vin_force_reset_buffer(int video_id)
{
	struct vin_core *vinc = vin_core_gbl[video_id];
	struct vin_vid_cap *cap  = &vinc->vid_cap;
	struct vin_buffer *buf;
	unsigned long flags = 0;

	spin_lock_irqsave(&cap->slock, flags);
	while (!list_empty(&cap->vidq_active)) {
		buf =
		    list_first_entry(&cap->vidq_active, struct vin_buffer, list);
		list_del(&buf->list);
		list_add(&buf->list, &cap->vidq_done);
	}
	spin_unlock_irqrestore(&cap->slock, flags);

	return 0;
}
EXPORT_SYMBOL(vin_force_reset_buffer);

int vin_streamoff_special(int video_id, enum v4l2_buf_type i)
{
	struct vin_core *vinc = vin_core_gbl[video_id];
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct modules_config *module = &vind->modules[vinc->sensor_sel];
	int valid_idx = module->sensors.valid_idx;
	__maybe_unused struct isp_dev *isp = NULL;
	int ret = 0;

	if (!vin_streaming(cap)) {
		vin_err("video%d has been already streaming off\n", vinc->id);
		goto streamoff_error;
	}

#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
	if (isp->gtm_type == 4) {
		if ((vinc->id == 0) && (!check_ldci_video_relate(vinc->id, LDCI0_VIDEO_CHN))) {
			disable_ldci_video(LDCI0_VIDEO_CHN);
		} else if ((vinc->id == 1) && (!check_ldci_video_relate(vinc->id, LDCI1_VIDEO_CHN))) {
			disable_ldci_video(LDCI1_VIDEO_CHN);
		} else if ((vinc->id == 2) && (!check_ldci_video_relate(vinc->id, LDCI2_VIDEO_CHN))) {
			disable_ldci_video(LDCI2_VIDEO_CHN);
		}
	}
#endif

	mutex_lock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);
	clear_bit(VIN_STREAM, &cap->state);
	vin_pipeline_call(vinc, set_stream, &cap->pipe, 0);
	set_bit(VIN_LPM, &cap->state);
	__csi_isp_setup_link(vinc, 0);
	__vin_sensor_setup_link(vinc, module, valid_idx, 0);
	mutex_unlock(&cap->vdev.entity.graph_obj.mdev->graph_mutex);

	if (i != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE) {
		ret = -EINVAL;
		goto streamoff_error;
	}

streamoff_error:
	return ret;
}
EXPORT_SYMBOL(vin_streamoff_special);
#endif

void vin_register_buffer_done_callback(int id, void *func)
{
	struct vin_core *vinc = vin_core_gbl[id];

	vinc->vid_cap.vin_buffer_process = func;
}
EXPORT_SYMBOL(vin_register_buffer_done_callback);

int vin_get_encpp_cfg(int id, unsigned char ctrl_id, void *value)
{
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	struct vin_core *vinc = vin_core_gbl[id];
	struct isp_dev *isp = v4l2_get_subdevdata(vinc->vid_cap.pipe.sd[VIN_IND_ISP]);

	switch (ctrl_id) {
	case ISP_CTRL_ENCPP_EN:
		*(int *)value = isp->encpp_en;
		break;
	case ISP_CTRL_ENCPP_STATIC_CFG:
		*(struct encpp_static_sharp_config *)value = isp->encpp_static_sharp_cfg;
		break;
	case ISP_CTRL_ENCPP_DYNAMIC_CFG:
		*(struct encpp_dynamic_sharp_config *)value = isp->encpp_dynamic_sharp_cfg;
		break;
	case ISP_CTRL_ENCODER_3DNR_CFG:
		*(struct encoder_3dnr_config *)value = isp->encoder_3dnr_cfg;
		break;
	case ISP_CTRL_ENCODER_2DNR_CFG:
		*(struct encoder_2dnr_config *)value = isp->encoder_2dnr_cfg;
		break;
	default:
		vin_err("%s: Unknown ctrl.\n", __FUNCTION__);
		return -1;
	}
#endif
	return 0;
}
EXPORT_SYMBOL(vin_get_encpp_cfg);

/* must set after vin_s_input_special and before vin_s_parm_special */
int vin_set_ve_online_cfg_special(int id, struct csi_ve_online_cfg *cfg)
{
	struct vin_core *vinc = vin_core_gbl[id];

	if (!cfg->ve_online_en) {
		vinc->ve_online_cfg.ve_online_en = 0;
		vinc->ve_online_cfg.dma_buf_num = BK_MUL_BUFFER;
		vin_print("ve_online close\n");
		return 0;
	}

	if (vinc->work_mode == BK_OFFLINE) {
		vin_err("ve online mode need video%d work in online\n", vinc->id);
		return -1;
	}

	if (vinc->id == CSI_VE_ONLINE_VIDEO) {
		memcpy(&vinc->ve_online_cfg, cfg, sizeof(struct csi_ve_online_cfg));
	} else {
		vin_err("only video%d supply ve online\n", CSI_VE_ONLINE_VIDEO);
		return -1;
	}

	vin_print("ve_online %s, buffer_num is %d\n", vinc->ve_online_cfg.ve_online_en ? "open" : "close", vinc->ve_online_cfg.dma_buf_num);

	return 0;
}
EXPORT_SYMBOL(vin_set_ve_online_cfg_special);

/* must set after vin_s_fmt_special and before vin_streamon_special */
int vin_set_tdm_speeddn_cfg_special(int id, struct tdm_speeddn_cfg *cfg)
{
#if defined SUPPORT_ISP_TDM && defined TDM_V200
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct mipi_dev *mipi = NULL;
	struct tdm_rx_dev *tdm_rx = NULL;
	struct tdm_dev *tdm = NULL;

	if (vinc->mipi_sel != 0xff) {
		if (cfg->pix_num == MIPI_TWO_PIXEL && mipi->cmb_csi_cfg.lane_num != 4) {
			cfg->pix_num = MIPI_ONE_PIXEL;
			vin_warn("mipi %d lane cannot support two pixel, set to one pexel\n", mipi->cmb_csi_cfg.lane_num);
		}

		if (cfg->pix_num == MIPI_TWO_PIXEL && !cfg->tdm_speed_down_en) {
			cfg->tdm_speed_down_en = 1;
			vin_warn("when mipi set to two pixel, must open tdm speed_dn\n");
		}

		mipi = container_of(cap->pipe.sd[VIN_IND_MIPI], struct mipi_dev, subdev);
		mipi->cmb_csi_cfg.pix_num = (enum cmb_csi_pix_num)cfg->pix_num;
	}

	if (vinc->tdm_rx_sel != 0xff) {
		tdm_rx = container_of(cap->pipe.sd[VIN_IND_TDM_RX], struct tdm_rx_dev, subdev);
		tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
		tdm->ws.speed_dn_en = cfg->tdm_speed_down_en;
		if (cfg->tdm_tx_valid_num || cfg->tdm_tx_invalid_num) {
			tdm->tx_cfg.valid_num = cfg->tdm_tx_valid_num;
			tdm->tx_cfg.invalid_num = cfg->tdm_tx_invalid_num;
		} else {
			tdm->tx_cfg.valid_num = 1;
			tdm->tx_cfg.invalid_num = 0;
		}
	}
#endif
	return 0;
}
EXPORT_SYMBOL(vin_set_tdm_speeddn_cfg_special);

int vin_s_fmt_overlay_special(int id, struct v4l2_format *f)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_osd *osd = &vinc->vid_cap.osd;
	struct v4l2_clip *clip = NULL;
	int ret = 0, i = 0;

	__osd_win_check(&f->fmt.win);

	osd->overlay_en = 0;
	osd->cover_en = 0;

	if (!f->fmt.win.bitmap) {
		if (f->fmt.win.clipcount <= 0) {
			osd->orl_en = 0;
			goto osd_reset;
		}

		clip = vmalloc(sizeof(struct v4l2_clip) * f->fmt.win.clipcount * 2);
		if (clip == NULL) {
			vin_err("%s - Alloc of clip mask failed\n", __func__);
			return -ENOMEM;
		}
		if (!memcpy(clip, f->fmt.win.clips,
			sizeof(struct v4l2_clip) * f->fmt.win.clipcount * 2)) {
			vfree(clip);
			return -EFAULT;
		}

		/*save rgb in the win top for diff cover*/
		osd->orl_width = clip[f->fmt.win.clipcount].c.width;
		if (osd->orl_width) {
			if (MAX_ORL_NUM) {
				osd->orl_en = 1;
				osd->orl_cnt = f->fmt.win.clipcount;
			} else {
				osd->orl_en = 0;
				vin_err("VIPP orl is not exist!!\n");
				goto osd_reset;
			}
		}

		if (osd->orl_en) {
			for (i = 0; i < osd->orl_cnt; i++) {
				u8 r, g, b;

				osd->orl_win[i] = clip[i].c;
				osd->rgb_orl[i] = clip[i + osd->orl_cnt].c.top;

				r = (osd->rgb_orl[i] >> 16) & 0xff;
				g = (osd->rgb_orl[i] >> 8) & 0xff;
				b = osd->rgb_orl[i] & 0xff;
				__osd_rgb_to_yuv(r, g, b, &osd->yuv_orl[0][i],
					&osd->yuv_orl[1][i], &osd->yuv_orl[2][i]);
			}
		}

		vfree(clip);
	}
osd_reset:
	osd->is_set = 0;

	return ret;
}
EXPORT_SYMBOL(vin_s_fmt_overlay_special);

int vin_overlay_special(int id, unsigned int on)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_osd *osd = &vinc->vid_cap.osd;
	int i;
	int ret = 0;

	if (!on) {
		for (i = 0; i < 2; i++) {
			if (osd->ov_mask[i].phy_addr) {
				os_mem_free(&vinc->pdev->dev, &osd->ov_mask[i]);
				osd->ov_mask[i].phy_addr = NULL;
				osd->ov_mask[i].size = 0;
			}
		}
		osd->ov_set_cnt = 0;
		osd->overlay_en = 0;
		osd->cover_en = 0;
		osd->orl_en = 0;
	} else {
		if (osd->is_set)
			return ret;
	}

	ret = __osd_reg_setup(vinc, osd);
	osd->is_set = 1;
	return ret;
}
EXPORT_SYMBOL(vin_overlay_special);

void vin_server_reset_special(int id, int mode_flag, int ir_on, int ir_flash_on)
{
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct ir_switch ir_switch;
	unsigned int data[2];
	struct isp_dev *isp = container_of(cap->pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
	struct sensor_info *info = container_of(cap->pipe.sd[VIN_IND_SENSOR], struct sensor_info, sd);

	if (ir_on && mode_flag == 2) {
		if (info->ir_state == NIGHT_STATE)
			return;

		data[0] = VIN_SET_SERVER_RESET;
		data[1] = mode_flag;
		isp_rpmsg_send(isp, data, 2 * sizeof(unsigned int));

		usleep_range(250000, 260000);/* make sure isp change to black&white mode and then open ircut*/

		ir_switch.ir_hold = 0;
		ir_switch.ir_on = ir_on;
		ir_switch.ir_flash_on = ir_flash_on;
		v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl, VIDIOC_VIN_SET_IR, &ir_switch);
	} else if (!ir_on && mode_flag != 2) {
		if (info->ir_state == DAY_STATE)
			return;

		ir_switch.ir_hold = 0;
		ir_switch.ir_on = ir_on;
		ir_switch.ir_flash_on = ir_flash_on;
		v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR], core, ioctl, VIDIOC_VIN_SET_IR, &ir_switch);

		usleep_range(350000, 360000);/* make sure ircut off and then change isp to color mode*/

		data[0] = VIN_SET_SERVER_RESET;
		data[1] = mode_flag;
		isp_rpmsg_send(isp, data, 2 * sizeof(unsigned int));
	}
#endif
}
EXPORT_SYMBOL(vin_server_reset_special);

int vin_set_isp_attr_cfg_special(int id, void *value)
{
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	int ret = 0;
	struct vin_core *vinc = vin_core_gbl[id];
	struct isp_cfg_attr_data *attr_cfg_ptr = (struct isp_cfg_attr_data *)value;

	ret = vin_set_isp_attr_cfg_ctrl(vinc, attr_cfg_ptr);

	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(vin_set_isp_attr_cfg_special);

int vin_get_isp_attr_cfg_special(int id, void *value)
{
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	int ret = 0;
	struct vin_core *vinc = vin_core_gbl[id];
	struct isp_cfg_attr_data *attr_cfg_ptr = (struct isp_cfg_attr_data *)value;

	ret = vin_get_isp_attr_cfg_ctrl(vinc, attr_cfg_ptr);

	return ret;
#else
	return 0;
#endif
}
EXPORT_SYMBOL(vin_get_isp_attr_cfg_special);

void vin_get_sensor_resolution_special(int id, struct sensor_resolution *sensor_resolution)
{
	struct vin_core *vinc = vin_core_gbl[id];

	sensor_get_resolution(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], sensor_resolution);
}
EXPORT_SYMBOL(vin_get_sensor_resolution_special);

void vin_sensor_fps_change_callback(int id, void *func)
{
	struct vin_core *vinc = vin_core_gbl[id];
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct sensor_info *info = container_of(cap->pipe.sd[VIN_IND_SENSOR], struct sensor_info, sd);

	info->sensor_fps_change_callback = func;
}
EXPORT_SYMBOL(vin_sensor_fps_change_callback);

int vin_isp_get_hist_special(int id, unsigned int *hist)
{
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	struct vin_core *vinc = vin_core_gbl[id];
	struct isp_dev *isp = container_of(vinc->vid_cap.pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);

	return vin_isp_get_hist(isp, hist);
#else
	return 0;
#endif
}
EXPORT_SYMBOL(vin_isp_get_hist_special);

int vin_set_phy2vir_special(int id, struct isp_memremap_cfg *isp_memremap)
{
#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	struct vin_core *vinc = vin_core_gbl[id];
	unsigned long viraddr;
	struct vm_area_struct *vma;
	void *vaddr = NULL;
	struct isp_autoflash_config_s *isp_autoflash_cfg = NULL;
	unsigned int map_addr = 0;
	unsigned int check_sign = 0;

	if (vinc->mipi_sel == 0) {
		map_addr = VIN_SENSOR0_RESERVE_ADDR;
		check_sign = 0xAA11AA11;
	} else {
		map_addr = VIN_SENSOR1_RESERVE_ADDR;
		check_sign = 0xBB11BB11;
	}

	vaddr = vin_map_kernel(map_addr, VIN_RESERVE_SIZE + VIN_THRESHOLD_PARAM_SIZE); /* map unit is page, page is align of 4k */
	if (vaddr == NULL) {
		vin_err("%s:map 0x%x paddr err!!!", __func__, map_addr);
		return -EFAULT;
	}

	isp_autoflash_cfg = (struct isp_autoflash_config_s *)(vaddr + VIN_RESERVE_SIZE);

	/* check id */
	if (isp_autoflash_cfg->melisyuv_sign_id != check_sign) {
		vin_warn("%s:sign is 0x%x but not 0x%x\n", __func__, isp_autoflash_cfg->sensorlist_sign_id, check_sign);
		vin_unmap_kernel(vaddr);
		return -EFAULT;
	}

	if (isp_memremap->en) {
		viraddr = vm_mmap(NULL, 0, isp_autoflash_cfg->melisyuv_size, PROT_READ, MAP_SHARED | MAP_NORESERVE, 0);
		vma = find_vma(current->mm, viraddr);
		remap_pfn_range(vma, vma->vm_start, __phys_to_pfn(isp_autoflash_cfg->melisyuv_paddr), isp_autoflash_cfg->melisyuv_size, vma->vm_page_prot);
		isp_memremap->vir_addr = (void *)viraddr;
		isp_memremap->size = isp_autoflash_cfg->melisyuv_size;
		vin_print("0x%x mmap viraddr is 0x%lx\n", isp_autoflash_cfg->melisyuv_paddr, viraddr);
	} else {
		if (isp_memremap->vir_addr && isp_memremap->size) {
			vm_munmap((unsigned long)isp_memremap->vir_addr, isp_memremap->size);
			vin_print("0x%x ummap viraddr is 0x%lx\n", isp_autoflash_cfg->melisyuv_paddr, (unsigned long)isp_memremap->vir_addr);

			isp_autoflash_cfg->melisyuv_sign_id = 0XFFFFFFFF;
			memblock_free(isp_autoflash_cfg->melisyuv_paddr, isp_autoflash_cfg->melisyuv_size);
			free_reserved_area(__va(isp_autoflash_cfg->melisyuv_paddr), __va(isp_autoflash_cfg->melisyuv_paddr + isp_autoflash_cfg->melisyuv_size), -1, "isp_reserved");
		}
	}

	vin_unmap_kernel(vaddr);
#endif
	return 0;
}
EXPORT_SYMBOL(vin_set_phy2vir_special);

void vin_isp_reset_done_callback(int id, void *func)
{
	struct vin_core *vinc = vin_core_gbl[id];

	vinc->vid_cap.online_csi_reset_callback = func;
}
EXPORT_SYMBOL(vin_isp_reset_done_callback);

static const struct v4l2_ctrl_config ae_win_ctrls[] = {
	{
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X1,
		.name = "AutoExposure Win X1",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y1,
		.name = "AutoExposure Win Y1",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X2,
		.name = "AutoExposure Win X2",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y2,
		.name = "AutoExposure Win Y2",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config af_win_ctrls[] = {
	{
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X1,
		.name = "AutoFocus Win X1",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y1,
		.name = "AutoFocus Win Y1",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X2,
		.name = "AutoFocus Win X2",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y2,
		.name = "AutoFocus Win Y2",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}
};

static const struct v4l2_ctrl_config custom_ctrls[] = {
	{
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_HOR_VISUAL_ANGLE,
		.name = "Horizontal Visual Angle",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 360,
		.step = 1,
		.def = 60,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_VER_VISUAL_ANGLE,
		.name = "Vertical Visual Angle",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 360,
		.step = 1,
		.def = 60,
		.flags = V4L2_CTRL_FLAG_VOLATILE | V4L2_CTRL_FLAG_READ_ONLY,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_FOCUS_LENGTH,
		.name = "Focus Length",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 1000,
		.step = 1,
		.def = 280,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AUTO_FOCUS_INIT,
		.name = "AutoFocus Initial",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_AUTO_FOCUS_RELEASE,
		.name = "AutoFocus Release",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_GSENSOR_ROTATION,
		.name = "Gsensor Rotaion",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = -180,
		.max = 180,
		.step = 90,
		.def = 0,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_TAKE_PICTURE,
		.name = "Take Picture",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 16,
		.step = 1,
		.def = 0,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_SENSOR_TYPE,
		.name = "Sensor type",
		.type = V4L2_CTRL_TYPE_MENU,
		.min = 0,
		.max = 1,
		.def = 0,
		.menu_skip_mask = 0x0,
		.qmenu = sensor_info_type,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &vin_ctrl_ops,
		.id = V4L2_CID_FLASH_LED_MODE_V1,
		.name = "VIN Flash ctrl",
		.type = V4L2_CTRL_TYPE_MENU,
		.min = 0,
		.max = 2,
		.def = 0,
		.menu_skip_mask = 0x0,
		.qmenu = flash_led_mode_v1,
		.flags = 0,
		.step = 0,
	},
};
static const s64 iso_qmenu[] = {
	100, 200, 400, 800, 1600, 3200, 6400,
};
static const s64 exp_bias_qmenu[] = {
	-4, -3, -2, -1, 0, 1, 2, 3, 4,
};

int vin_init_controls(struct v4l2_ctrl_handler *hdl, struct vin_vid_cap *cap)
{
	struct v4l2_ctrl *ctrl;
	unsigned int i, ret = 0;

	v4l2_ctrl_handler_init(hdl, 40 + ARRAY_SIZE(custom_ctrls)
		+ ARRAY_SIZE(ae_win_ctrls) + ARRAY_SIZE(af_win_ctrls));
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_BRIGHTNESS, -128, 512, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_CONTRAST, -128, 512, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_SATURATION, -256, 512, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_HUE, -180, 180, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE, 1, 65536 * 16, 1, 1);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTOGAIN, 0, 1, 1, 1);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_GAIN, 16, 6000 * 16, 1, 16);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_HFLIP, 0, 1, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_VFLIP, 0, 1, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_POWER_LINE_FREQUENCY,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 0,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_HUE_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops,
			  V4L2_CID_WHITE_BALANCE_TEMPERATURE, 2800, 10000, 1, 6500);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_SHARPNESS, 0, 1000, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_CHROMA_AGC, 0, 1, 1, 1);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_COLORFX,
			       V4L2_COLORFX_SET_CBCR, 0, V4L2_COLORFX_NONE);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTOBRIGHTNESS, 0, 1, 1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_BAND_STOP_FILTER, 0, 1, 1, 1);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_ILLUMINATORS_1, 0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_ILLUMINATORS_2, 0, 1, 1, 0);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_AUTO,
			       V4L2_EXPOSURE_APERTURE_PRIORITY, 0,
			       V4L2_EXPOSURE_AUTO);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_ABSOLUTE, 1, 30 * 1000000, 1, 1);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_EXPOSURE_AUTO_PRIORITY, 0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_ABSOLUTE, 0, 127, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_RELATIVE, -127, 127, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_FOCUS_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_int_menu(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_EXPOSURE_BIAS,
			       ARRAY_SIZE(exp_bias_qmenu) - 1,
			       ARRAY_SIZE(exp_bias_qmenu) / 2, exp_bias_qmenu);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			       V4L2_WHITE_BALANCE_SHADE, 0,
			       V4L2_WHITE_BALANCE_AUTO);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_WIDE_DYNAMIC_RANGE, 0, 1, 1, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_IMAGE_STABILIZATION, 0, 1, 1, 0);
	v4l2_ctrl_new_int_menu(hdl, &vin_ctrl_ops, V4L2_CID_ISO_SENSITIVITY,
			       ARRAY_SIZE(iso_qmenu) - 1,
			       ARRAY_SIZE(iso_qmenu) / 2 - 1, iso_qmenu);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_ISO_SENSITIVITY_AUTO,
			       V4L2_ISO_SENSITIVITY_AUTO, 0,
			       V4L2_ISO_SENSITIVITY_AUTO);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops,
			       V4L2_CID_EXPOSURE_METERING,
			       V4L2_EXPOSURE_METERING_MATRIX, 0,
			       V4L2_EXPOSURE_METERING_AVERAGE);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_SCENE_MODE,
			       V4L2_SCENE_MODE_TEXT, 0, V4L2_SCENE_MODE_NONE);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_3A_LOCK, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_START, 0, 0, 0, 0);
	v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_STOP, 0, 0, 0, 0);
	ctrl = v4l2_ctrl_new_std(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_STATUS, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_AUTO_FOCUS_RANGE,
			       V4L2_AUTO_FOCUS_RANGE_INFINITY, 0,
			       V4L2_AUTO_FOCUS_RANGE_AUTO);
	v4l2_ctrl_new_std_menu(hdl, &vin_ctrl_ops, V4L2_CID_FLASH_LED_MODE,
			       V4L2_FLASH_LED_MODE_RED_EYE, 0,
			       V4L2_FLASH_LED_MODE_NONE);

	for (i = 0; i < ARRAY_SIZE(custom_ctrls); i++)
		v4l2_ctrl_new_custom(hdl, &custom_ctrls[i], NULL);

	for (i = 0; i < ARRAY_SIZE(ae_win_ctrls); i++)
		cap->ae_win[i] = v4l2_ctrl_new_custom(hdl,
						&ae_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(ae_win_ctrls), &cap->ae_win[0]);

	for (i = 0; i < ARRAY_SIZE(af_win_ctrls); i++)
		cap->af_win[i] = v4l2_ctrl_new_custom(hdl,
						&af_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(af_win_ctrls), &cap->af_win[0]);

	if (hdl->error) {
		ret = hdl->error;
		v4l2_ctrl_handler_free(hdl);
	}
	return ret;
}

int vin_init_video(struct v4l2_device *v4l2_dev, struct vin_vid_cap *cap)
{
	int ret = 0;
	struct vb2_queue *q;
	struct vin_buffer *vin_buffer_size;

	snprintf(cap->vdev.name, sizeof(cap->vdev.name),
		"vin_video%d", cap->vinc->id);
	cap->vdev.fops = &vin_fops;
	cap->vdev.ioctl_ops = &vin_ioctl_ops;
	cap->vdev.device_caps = V4L2_CAP_VIDEO_CAPTURE_MPLANE |
							V4L2_CAP_STREAMING | V4L2_CAP_READWRITE;
	cap->vdev.release = video_device_release_empty;
	cap->vdev.ctrl_handler = &cap->ctrl_handler;
	cap->vdev.v4l2_dev = v4l2_dev;
	cap->vdev.queue = &cap->vb_vidq;
	cap->vdev.lock = &cap->lock;
	cap->vdev.flags = V4L2_FL_USES_V4L2_FH;
	ret = video_register_device(&cap->vdev, VFL_TYPE_VIDEO, cap->vinc->id);
	if (ret < 0) {
		vin_err("Error video_register_device!!\n");
		return -1;
	}
	video_set_drvdata(&cap->vdev, cap->vinc);
	vin_log(VIN_LOG_VIDEO, "V4L2 device registered as %s\n",
		video_device_node_name(&cap->vdev));

	/*  Initialize videobuf2 queue as per the buffer type  */
	ret = dma_set_mask(&cap->vinc->pdev->dev, DMA_BIT_MASK(32));
	if (ret < 0) {
		vin_err("Error dma_set_mask!!\n");
		return -1;
	}
	ret = dma_set_coherent_mask(&cap->vinc->pdev->dev, DMA_BIT_MASK(32));
	if (ret < 0) {
		vin_err("Error dma_set_coherent_mask!!\n");
		return -1;
	}
	cap->dev = &cap->vinc->pdev->dev;
	if (!cap->dev->dma_parms) {
		ret = vb2_dma_contig_set_max_seg_size(&cap->vinc->pdev->dev, DMA_BIT_MASK(32));
		if (ret < 0 || IS_ERR_OR_NULL(cap->dev->dma_parms)) {
			vin_err("Failed to get the context\n");
			return -1;
		}
		cap->dma_parms_alloc = true;
	}
	/*  initialize queue  */
	q = &cap->vb_vidq;
	q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	q->io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ;
	q->drv_priv = cap;
	q->buf_struct_size = sizeof(*vin_buffer_size);
	q->ops = &vin_video_qops;
	q->mem_ops = &vb2_dma_contig_memops;
	q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	q->lock = &cap->lock;

	ret = vb2_queue_init(q);
	if (ret) {
		vin_err("vb2_queue_init() failed\n");
		if (cap->dma_parms_alloc)
			vb2_dma_contig_clear_max_seg_size(cap->dev);
		return ret;
	}

	cap->vd_pad.flags = MEDIA_PAD_FL_SINK;
	ret = media_entity_pads_init(&cap->vdev.entity, 1, &cap->vd_pad);
	if (ret)
		return ret;

	INIT_WORK(&cap->s_stream_task, __vin_s_stream_handle);

	cap->state = 0;
	cap->registered = 1;
	/*  initial state  */
	cap->capture_mode = V4L2_MODE_PREVIEW;
	/*  init video dma queues  */
	INIT_LIST_HEAD(&cap->vidq_active);
	mutex_init(&cap->lock);
	spin_lock_init(&cap->slock);

	return 0;
}

static int vin_link_setup(struct media_entity *entity,
			  const struct media_pad *local,
			  const struct media_pad *remote, u32 flags)
{
	return 0;
}

static const struct media_entity_operations vin_sd_media_ops = {
	.link_setup = vin_link_setup,
};

static int vin_video_core_s_power(struct v4l2_subdev *sd, int on)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);

	if (on) {
		pm_runtime_get_sync(&vinc->pdev->dev);/*  call pm_runtime resume  */
	} else {
		pm_runtime_put_sync(&vinc->pdev->dev);/*  call pm_runtime suspend  */
	}
	return 0;
}

static int vin_subdev_logic_s_stream(unsigned char virtual_id, int on)
{
#if defined CSIC_DMA_VER_140_000
	unsigned char logic_id = dma_virtual_find_logic[virtual_id];
	struct vin_core *logic_vinc = vin_core_gbl[logic_id];

	if (logic_vinc->work_mode == BK_ONLINE && virtual_id != logic_id) {
		vin_err("video%d work on online mode, video%d cannot to work!!\n", logic_id, virtual_id);
		return -1;
	}

	if (on && (logic_vinc->logic_top_stream_count)++ > 0)
		return 0;
	else if (!on && (logic_vinc->logic_top_stream_count == 0 || --(logic_vinc->logic_top_stream_count) > 0))
		return 0;

	if (on) {
		csic_dma_top_enable(logic_id);
		csic_dma_mul_ch_enable(logic_id, logic_vinc->work_mode);
		if (logic_vinc->id == CSI_VE_ONLINE_VIDEO && logic_vinc->ve_online_cfg.ve_online_en) {
			csic_ve_online_hs_enable(logic_id);
			logic_vinc->ve_ol_ch = CSI_VE_ONLINE_VIDEO;
			csic_ve_online_ch_sel(logic_id, logic_vinc->ve_ol_ch);
		}
		csic_dma_buf_length_software_enable(logic_vinc->vipp_sel, 0);
		csi_dam_flip_software_enable(logic_vinc->vipp_sel, 0);
		//csic_dma_top_interrupt_en(logic_id, VIDEO_INPUT_TO_INT | CLR_FS_FRM_CNT_INT | FS_PUL_INT);//for debug
	} else {
		//csic_dma_top_interrupt_disable(logic_id, DMA_TOP_INT_ALL);
		csic_ve_online_hs_disable(logic_id);
		csic_dma_top_disable(logic_id);
	}
	vin_log(VIN_LOG_FMT, "dma%d top init by video%d, %s\n", logic_id, virtual_id, on ? "steram on" : "steam off");
#endif
	return 0;
}

static int vin_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	struct vin_vid_cap *cap = &vinc->vid_cap;
	struct csic_dma_cfg cfg;
	struct csic_dma_flip flip;
	struct dma_output_size size;
	struct dma_buf_len buf_len;
	struct dma_flip_size flip_size;
	struct sensor_output_fmt sensor_fmt;
	int flag = 0;
	int flip_mul = 2;
	int ch_id = 0;

	if (enable) {
		memset(&cfg, 0, sizeof(cfg));
		memset(&size, 0, sizeof(size));
		memset(&buf_len, 0, sizeof(buf_len));

		switch (cap->frame.fmt.field) {
		case V4L2_FIELD_ANY:
		case V4L2_FIELD_NONE:
			cfg.field = FIELD_EITHER;
			break;
		case V4L2_FIELD_TOP:
			cfg.field = FIELD_1;
			flag = 1;
			break;
		case V4L2_FIELD_BOTTOM:
			cfg.field = FIELD_2;
			flag = 1;
			break;
		case V4L2_FIELD_INTERLACED:
			cfg.field = FIELD_EITHER;
			flag = 1;
			break;
		default:
			cfg.field = FIELD_EITHER;
			break;
		}

		if (vinc->tvin.flag) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
			if (vinc->id > TVIN_VIDEO_MAX)
				ch_id = vinc->id / TVIN_VIDEO_STRIP + vinc->id % TVIN_VIDEO_STRIP;
			else
				ch_id = vinc->id / TVIN_VIDEO_STRIP;
#else
			ch_id = vinc->id;
#endif
			if (ch_id < TVIN_SEPARATE)
				sensor_fmt.ch_id = ch_id;
			else
				sensor_fmt.ch_id = ch_id - TVIN_SEPARATE;

			if (!v4l2_subdev_call(cap->pipe.sd[VIN_IND_SENSOR],
					core, ioctl, GET_SENSOR_CH_OUTPUT_FMT, &sensor_fmt))
				flag = sensor_fmt.field;
		}

		switch (cap->frame.fmt.fourcc) {
		case V4L2_PIX_FMT_NV12:
		case V4L2_PIX_FMT_NV12M:
		case V4L2_PIX_FMT_FBC:
			cfg.fmt = flag ? FRAME_UV_CB_YUV420 : FIELD_UV_CB_YUV420;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_LBC_2_0X:
		case V4L2_PIX_FMT_LBC_2_5X:
		case V4L2_PIX_FMT_LBC_1_0X:
		case V4L2_PIX_FMT_LBC_1_5X:
			cfg.fmt = LBC_MODE_OUTPUT;
			buf_len.buf_len_y = cap->lbc_cmp.line_tar_bits[1] >> 3;
			buf_len.buf_len_c = cap->lbc_cmp.line_tar_bits[0] >> 3;
			break;
		case V4L2_PIX_FMT_NV21:
		case V4L2_PIX_FMT_NV21M:
			cfg.fmt = flag ? FRAME_VU_CB_YUV420 : FIELD_VU_CB_YUV420;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_YVU420:
		case V4L2_PIX_FMT_YUV420:
		case V4L2_PIX_FMT_YUV420M:
			cfg.fmt = flag ? FRAME_PLANAR_YUV420 : FIELD_PLANAR_YUV420;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y >> 1;
			break;
		case V4L2_PIX_FMT_GREY:
			cfg.fmt = flag ? FRAME_CB_YUV400 : FIELD_CB_YUV400;
			buf_len.buf_len_y = cap->frame.o_width;
			break;
		case V4L2_PIX_FMT_YUV422P:
			cfg.fmt = flag ? FRAME_PLANAR_YUV422 : FIELD_PLANAR_YUV422;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y >> 1;
			break;
		case V4L2_PIX_FMT_NV61:
		case V4L2_PIX_FMT_NV61M:
			cfg.fmt = flag ? FRAME_VU_CB_YUV422 : FIELD_VU_CB_YUV422;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_NV16:
		case V4L2_PIX_FMT_NV16M:
			cfg.fmt = flag ? FRAME_UV_CB_YUV422 : FIELD_UV_CB_YUV422;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR8:
		case V4L2_PIX_FMT_SGBRG8:
		case V4L2_PIX_FMT_SGRBG8:
		case V4L2_PIX_FMT_SRGGB8:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_8 : FIELD_RAW_8;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR10:
		case V4L2_PIX_FMT_SGBRG10:
		case V4L2_PIX_FMT_SGRBG10:
		case V4L2_PIX_FMT_SRGGB10:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_10 : FIELD_RAW_10;
			buf_len.buf_len_y = cap->frame.o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR12:
		case V4L2_PIX_FMT_SGBRG12:
		case V4L2_PIX_FMT_SGRBG12:
		case V4L2_PIX_FMT_SRGGB12:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_12 : FIELD_RAW_12;
			buf_len.buf_len_y = cap->frame.o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
#if defined CSIC_DMA_VER_140_000
		case V4L2_PIX_FMT_SBGGR14:
		case V4L2_PIX_FMT_SGBRG14:
		case V4L2_PIX_FMT_SGRBG14:
		case V4L2_PIX_FMT_SRGGB14:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_14 : FIELD_RAW_14;
			buf_len.buf_len_y = cap->frame.o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_SBGGR16:
		case V4L2_PIX_FMT_SGBRG16:
		case V4L2_PIX_FMT_SGRBG16:
		case V4L2_PIX_FMT_SRGGB16:
			flip_mul = 1;
			cfg.fmt = flag ? FRAME_RAW_16 : FIELD_RAW_16;
			buf_len.buf_len_y = cap->frame.o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
#endif
		case V4L2_PIX_FMT_RGB24:
		case V4L2_PIX_FMT_BGR24:
			cfg.fmt = FIELD_RGB888;
			buf_len.buf_len_y = cap->frame.o_width * 3;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		case V4L2_PIX_FMT_RGB565:
			cfg.fmt = FIELD_RGB565;
			buf_len.buf_len_y = cap->frame.o_width * 2;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		default:
			cfg.fmt = flag ? FRAME_UV_CB_YUV420 : FIELD_UV_CB_YUV420;
			buf_len.buf_len_y = cap->frame.o_width;
			buf_len.buf_len_c = buf_len.buf_len_y;
			break;
		}

		if (vinc->isp_dbg.debug_en) {
			buf_len.buf_len_y = 0;
			buf_len.buf_len_c = 0;
		}

		cfg.ds = vinc->fps_ds;

		if (vin_subdev_logic_s_stream(vinc->id, enable))
			return -1;

		csic_dma_config(vinc->vipp_sel, &cfg);
		size.hor_len = vinc->isp_dbg.debug_en ? 0 : cap->frame.o_width;
		size.ver_len = vinc->isp_dbg.debug_en ? 0 : cap->frame.o_height;
		size.hor_start = vinc->isp_dbg.debug_en ? 0 : cap->frame.offs_h;
		size.ver_start = vinc->isp_dbg.debug_en ? 0 : cap->frame.offs_v;
		flip_size.hor_len = vinc->isp_dbg.debug_en ? 0 : cap->frame.o_width * flip_mul;
		flip_size.ver_len = vinc->isp_dbg.debug_en ? 0 : cap->frame.o_height;
		flip.hflip_en = vinc->hflip;
		flip.vflip_en = vinc->vflip;

		if (vinc->large_image == 2) {
			size.hor_len /= 2;
			flip_size.hor_len /= 2;
		}

		csic_dma_output_size_cfg(vinc->vipp_sel, &size);
		/*  csic_dma_10bit_cut2_8bit_enable(vinc->vipp_sel);  */

#if !defined CSIC_DMA_VER_140_000
		switch (cap->frame.fmt.fourcc) {
		case V4L2_PIX_FMT_FBC:
			csic_dma_buf_length_software_enable(vinc->vipp_sel, 0);
			csi_dam_flip_software_enable(vinc->vipp_sel, 0);
			csic_dma_flip_en(vinc->vipp_sel, &flip);
			csic_fbc_enable(vinc->vipp_sel);
			break;
		case V4L2_PIX_FMT_LBC_2_0X:
		case V4L2_PIX_FMT_LBC_2_5X:
		case V4L2_PIX_FMT_LBC_1_0X:
		case V4L2_PIX_FMT_LBC_1_5X:
			csi_dam_flip_software_enable(vinc->vipp_sel, 1);
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
			csic_dma_buf_length_software_enable(vinc->vipp_sel, 1);
#else
			csic_dma_buf_length_software_enable(vinc->vipp_sel, 0);
#endif
			csic_lbc_enable(vinc->vipp_sel);
			csic_lbc_cmp_ratio(vinc->vipp_sel, &cap->lbc_cmp);
			break;
		default:
			csic_dma_buf_length_software_enable(vinc->vipp_sel, 0);
			csi_dam_flip_software_enable(vinc->vipp_sel, 0);
			csic_dma_flip_en(vinc->vipp_sel, &flip);
			csic_dma_enable(vinc->vipp_sel);
			break;
		}

		csic_dma_buffer_length(vinc->vipp_sel, &buf_len);
		csic_dma_flip_size(vinc->vipp_sel, &flip_size);

		/*  give up line_cnt interrupt. process in vsync and frame_done isr. */
		/* csic_dma_line_cnt(vinc->vipp_sel, cap->frame.o_height / 16 * 12); */
		csic_frame_cnt_enable(vinc->vipp_sel);

#ifndef BUF_AUTO_UPDATE
		vin_set_next_buf_addr(vinc);
		csic_dma_top_enable(vinc->vipp_sel);

		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);

		csic_dma_int_enable(vinc->vipp_sel, DMA_INT_BUF_0_OVERFLOW | DMA_INT_BUF_1_OVERFLOW |
			DMA_INT_BUF_2_OVERFLOW | DMA_INT_HBLANK_OVERFLOW | DMA_INT_VSYNC_TRIG |
			DMA_INT_CAPTURE_DONE | DMA_INT_FRAME_DONE | DMA_INT_LBC_HB);
#else
		csic_dma_top_enable(vinc->vipp_sel);
		vin_set_next_buf_addr(vinc);

		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);

		csic_dma_int_enable(vinc->vipp_sel, DMA_INT_BUF_0_OVERFLOW | DMA_INT_BUF_1_OVERFLOW |
			DMA_INT_BUF_2_OVERFLOW | DMA_INT_HBLANK_OVERFLOW | DMA_INT_VSYNC_TRIG |
			DMA_INT_CAPTURE_DONE | DMA_INT_STORED_FRM_CNT | DMA_INT_FRM_LOST | DMA_INT_LBC_HB);
#endif
#else /* else CSIC_DMA_VER_140_000 */
		csic_dma_buffer_length(vinc->vipp_sel, &buf_len);
		csic_dma_flip_size(vinc->vipp_sel, &flip_size);

#ifndef BUF_AUTO_UPDATE
		if (vinc->large_image != 3)
			vin_set_next_buf_addr(vinc);

		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
		csic_dma_int_enable(vinc->vipp_sel, DMA_INT_BUF_0_OVERFLOW | DMA_INT_HBLANK_OVERFLOW |
			DMA_INT_VSYNC_TRIG | DMA_INT_CAPTURE_DONE | DMA_INT_FRAME_DONE | DMA_INT_LBC_HB);
#else
		if (vinc->large_image != 3)
			vin_set_next_buf_addr(vinc);

		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
		csic_dma_int_enable(vinc->vipp_sel, DMA_INT_BUF_0_OVERFLOW | DMA_INT_HBLANK_OVERFLOW |
			DMA_INT_VSYNC_TRIG | DMA_INT_CAPTURE_DONE | DMA_INT_STORED_FRM_CNT |
			DMA_INT_FRM_LOST | DMA_INT_LBC_HB);
#endif
		if ((vinc->dma_merge_mode == 1) && (vinc->vipp_sel % 4 == 0))
			csic_dma_int_disable(vinc->vipp_sel, DMA_INT_ALL);

		csic_dma_int_enable(vinc->vipp_sel, DMA_INT_ADDR_NO_READY | DMA_INT_ADDR_OVERFLOW);

		if (vinc->large_image != 3)
			csic_dma_int_enable(vinc->vipp_sel, DMA_INT_PIXEL_MISS | DMA_INT_LINE_MISS);

		switch (cap->frame.fmt.fourcc) {
		case V4L2_PIX_FMT_LBC_2_0X:
		case V4L2_PIX_FMT_LBC_2_5X:
		case V4L2_PIX_FMT_LBC_1_5X:
		case V4L2_PIX_FMT_LBC_1_0X:
			csic_lbc_enable(vinc->vipp_sel);
			csic_lbc_cmp_ratio(vinc->vipp_sel, &cap->lbc_cmp);
			break;
		default:
			csic_dma_flip_en(vinc->vipp_sel, &flip);
			csic_dma_enable(vinc->vipp_sel);
			break;
		}
#endif
	} else {
		vinc->tvin.flag = false;
#if !defined CSIC_DMA_VER_140_000
		csic_dma_top_disable(vinc->vipp_sel);
#endif
		csic_dma_int_disable(vinc->vipp_sel, DMA_INT_ALL);
		csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
		switch (cap->frame.fmt.fourcc) {
		case V4L2_PIX_FMT_FBC:
			csic_fbc_disable(vinc->vipp_sel);
			break;
		case V4L2_PIX_FMT_LBC_2_0X:
		case V4L2_PIX_FMT_LBC_2_5X:
		case V4L2_PIX_FMT_LBC_1_0X:
		case V4L2_PIX_FMT_LBC_1_5X:
			csic_lbc_disable(vinc->vipp_sel);
			break;
		default:
			csic_dma_disable(vinc->vipp_sel);
			break;
		}
		vin_subdev_logic_s_stream(vinc->id, enable);
	}
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
		cancel_work_sync(&vinc->ldci_buf_send_task);
#endif
	vin_log(VIN_LOG_FMT, "csic_dma%d %s, %d*%d hoff: %d voff: %d\n",
		vinc->id, enable ? "stream on" : "stream off",
		cap->frame.o_width, cap->frame.o_height,
		cap->frame.offs_h, cap->frame.offs_v);

	return 0;
}

static struct v4l2_subdev_core_ops vin_subdev_core_ops = {
	.s_power = vin_video_core_s_power,
};

static const struct v4l2_subdev_video_ops vin_subdev_video_ops = {
	.s_stream = vin_subdev_s_stream,
};

static struct v4l2_subdev_ops vin_subdev_ops = {
	.core = &vin_subdev_core_ops,
	.video = &vin_subdev_video_ops,
};

static int vin_capture_subdev_registered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);
	int ret;

	vinc->vid_cap.vinc = vinc;
	if (vin_init_controls(&vinc->vid_cap.ctrl_handler, &vinc->vid_cap)) {
		vin_err("Error v4l2 ctrls new!!\n");
		return -1;
	}

	vinc->pipeline_ops = v4l2_get_subdev_hostdata(sd);
	if (vin_init_video(sd->v4l2_dev, &vinc->vid_cap)) {
		vin_err("vin init video!!!!\n");
		vinc->pipeline_ops = NULL;
	}
	ret = sysfs_create_link(&vinc->vid_cap.vdev.dev.kobj,
		&vinc->pdev->dev.kobj, "vin_dbg");
	if (ret)
		vin_err("sysfs_create_link failed\n");

	return 0;
}

static void vin_capture_subdev_unregistered(struct v4l2_subdev *sd)
{
	struct vin_core *vinc = v4l2_get_subdevdata(sd);

	if (vinc == NULL)
		return;

	if (video_is_registered(&vinc->vid_cap.vdev)) {
		sysfs_remove_link(&vinc->vid_cap.vdev.dev.kobj, "vin_dbg");
		vin_log(VIN_LOG_VIDEO, "unregistering %s\n",
			video_device_node_name(&vinc->vid_cap.vdev));
		media_entity_cleanup(&vinc->vid_cap.vdev.entity);
		if (vinc->vid_cap.dma_parms_alloc && !IS_ERR_OR_NULL(vinc->vid_cap.dev->dma_parms))
			vb2_dma_contig_clear_max_seg_size(vinc->vid_cap.dev);
		video_unregister_device(&vinc->vid_cap.vdev);
		mutex_destroy(&vinc->vid_cap.lock);
	}
	v4l2_ctrl_handler_free(&vinc->vid_cap.ctrl_handler);
	vinc->pipeline_ops = NULL;
}

static const struct v4l2_subdev_internal_ops vin_capture_sd_internal_ops = {
	.registered = vin_capture_subdev_registered,
	.unregistered = vin_capture_subdev_unregistered,
};

int vin_initialize_capture_subdev(struct vin_core *vinc)
{
	struct v4l2_subdev *sd = &vinc->vid_cap.subdev;
	int ret;

	v4l2_subdev_init(sd, &vin_subdev_ops);
	sd->grp_id = VIN_GRP_ID_CAPTURE;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "vin_cap.%d", vinc->id);

	vinc->vid_cap.sd_pads[VIN_SD_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	vinc->vid_cap.sd_pads[VIN_SD_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_IO_V4L;
	ret = media_entity_pads_init(&sd->entity, VIN_SD_PADS_NUM,
				vinc->vid_cap.sd_pads);
	if (ret)
		return ret;

	sd->entity.ops = &vin_sd_media_ops;
	sd->internal_ops = &vin_capture_sd_internal_ops;
	v4l2_set_subdevdata(sd, vinc);
	return 0;
}

void vin_cleanup_capture_subdev(struct vin_core *vinc)
{
	struct v4l2_subdev *sd = &vinc->vid_cap.subdev;

	media_entity_cleanup(&sd->entity);
	v4l2_set_subdevdata(sd, NULL);
}

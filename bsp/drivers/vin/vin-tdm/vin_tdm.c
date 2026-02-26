/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
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
#include "../utility/vin_log.h"
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include "vin_tdm.h"
#include "../vin-video/vin_video.h"

#define TDM_MODULE_NAME "vin_tdm"

struct tdm_dev *glb_tdm[VIN_MAX_TDM];

#if IS_ENABLED(CONFIG_TDM_LBC_EN)
#define TDM_LBC_RATIO			393	/*1.0x:1024; 1.5x:683; 2.0x 512; 2.6x:393; 3x:342; 3.5x:293; 4x:256*/
#else
#define TDM_LBC_RATIO			1024
#endif

static struct tdm_format sunxi_tdm_formats[] = {
	{
		.code = MEDIA_BUS_FMT_SBGGR8_1X8,
		.input_type = INPUTTPYE_8BIT,
		.input_bit_width = RAW_8BIT,
		.raw_fmt = BAYER_BGGR,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG8_1X8,
		.input_type = INPUTTPYE_8BIT,
		.input_bit_width = RAW_8BIT,
		.raw_fmt = BAYER_GBRG,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG8_1X8,
		.input_type = INPUTTPYE_8BIT,
		.input_bit_width = RAW_8BIT,
		.raw_fmt = BAYER_GRBG,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB8_1X8,
		.input_type = INPUTTPYE_8BIT,
		.input_bit_width = RAW_8BIT,
		.raw_fmt = BAYER_RGGB,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.input_type = INPUTTPYE_10BIT,
		.input_bit_width = RAW_10BIT,
		.raw_fmt = BAYER_BGGR,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.input_type = INPUTTPYE_10BIT,
		.input_bit_width = RAW_10BIT,
		.raw_fmt = BAYER_GBRG,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.input_type = INPUTTPYE_10BIT,
		.input_bit_width = RAW_10BIT,
		.raw_fmt = BAYER_GRBG,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.input_type = INPUTTPYE_10BIT,
		.input_bit_width = RAW_10BIT,
		.raw_fmt = BAYER_RGGB,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR12_1X12,
		.input_type = INPUTTPYE_12BIT,
		.input_bit_width = RAW_12BIT,
		.raw_fmt = BAYER_BGGR,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.input_type = INPUTTPYE_12BIT,
		.input_bit_width = RAW_12BIT,
		.raw_fmt = BAYER_GBRG,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG12_1X12,
		.input_type = INPUTTPYE_12BIT,
		.input_bit_width = RAW_12BIT,
		.raw_fmt = BAYER_GRBG,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.input_type = INPUTTPYE_12BIT,
		.input_bit_width = RAW_12BIT,
		.raw_fmt = BAYER_RGGB,
	}
};

static void tdm_rx_bufs_free(struct tdm_rx_dev *tdm_rx)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	int i;

	if (tdm->tdm_rx_buf_en[tdm_rx->id] == 0)
		return;

	if (tdm_rx->buf_cnt == 0)
		return;

	for (i = 0; i < tdm_rx->buf_cnt; i++) {
		struct tdm_buffer *buf = &tdm_rx->buf[i];
		struct vin_mm *mm = &tdm_rx->ion_man[i];

		mm->size = tdm_rx->buf_size;
		if (!buf->virt_addr)
			continue;

		mm->vir_addr = buf->virt_addr;
		mm->dma_addr = buf->dma_addr;
		os_mem_free(&tdm->pdev->dev, mm);

		buf->dma_addr = NULL;
		buf->virt_addr = NULL;
	}

	vin_log(VIN_LOG_TDM, "%s: all buffers were freed.\n", tdm_rx->subdev.name);

	tdm->tdm_rx_buf_en[tdm_rx->id] = 0;
	tdm_rx->buf_size = 0;
	tdm_rx->buf_cnt = 0;
}

static int tdm_rx_bufs_alloc(struct tdm_rx_dev *tdm_rx, u32 size, u32 count)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	int i;

	if (tdm->tdm_rx_buf_en[tdm_rx->id] == 1)
		return 0;

	tdm_rx->buf_size = size;
	tdm_rx->buf_cnt = count;

	for (i = 0; i < tdm_rx->buf_cnt; i++) {
		struct tdm_buffer *buf = &tdm_rx->buf[i];
		struct vin_mm *mm = &tdm_rx->ion_man[i];

		mm->size = size;
		if (!os_mem_alloc(&tdm->pdev->dev, mm)) {
			buf->virt_addr = mm->vir_addr;
			buf->dma_addr = mm->dma_addr;
		} else {
			vin_err("%s: Can't acquire memory for DMA buffer %d\n",	tdm_rx->subdev.name, i);
			return -ENOMEM;
		}
		if (!buf->virt_addr || !buf->dma_addr) {
			vin_err("%s: Can't acquire memory for DMA buffer %d\n",	tdm_rx->subdev.name, i);
			tdm_rx_bufs_free(tdm_rx);
			return -ENOMEM;
		}
		vin_log(VIN_LOG_TDM, "rx%d:buf%d:dma_addr is 0x%px ~ 0x%px\n", tdm_rx->id, i, buf->dma_addr, buf->dma_addr + size);
	}

	vin_log(VIN_LOG_TDM, "%s: allocing buffers successfully, buf_cnt and size is %d and %d.\n",
				tdm_rx->subdev.name, tdm_rx->buf_cnt, tdm_rx->buf_size);

	tdm->tdm_rx_buf_en[tdm_rx->id] = 1;
	return 0;
}

#ifdef TDM_V200
static void tdm_set_tx_blank(struct tdm_dev *tdm)
{
	csic_tdm_omode(tdm->id, 1);
	if (tdm->work_mode == TDM_OFFLINE)
		csic_tdm_set_hblank(tdm->id, TDM_TX_HBLANK_OFFLINE);
	else
		csic_tdm_set_hblank(tdm->id, TDM_TX_HBLANK);
	csic_tdm_set_bblank_fe(tdm->id, TDM_TX_VBLANK/2);
	csic_tdm_set_bblank_be(tdm->id, TDM_TX_VBLANK/2);
}

static void tdm_rx_lbc_cal_para(struct tdm_rx_dev *tdm_rx)
{
	struct tdm_rx_lbc *lbc = &tdm_rx->lbc;
	unsigned int width_stride;
	unsigned int bit_depth;
	unsigned int mb_max_bits_ratio = 204;/*default*/
	unsigned int mb_min_bits_ratio = 820;/*default*/
	unsigned int align = 256;
	unsigned int mb_len = 48;

	if (TDM_LBC_RATIO != 1024 && tdm_rx->tdm_fmt->input_bit_width == RAW_10BIT)
		lbc->cmp_ratio = 293;/*3.5x*/
	else
		lbc->cmp_ratio = clamp(TDM_LBC_RATIO, 256, 1024);

	if (tdm_rx->tdm_fmt->input_bit_width == RAW_12BIT) {
		if (lbc->cmp_ratio >= 682)
			lbc->start_qp = 1;
		else if (lbc->cmp_ratio >= 512)
			lbc->start_qp = 2;
		else
			lbc->start_qp = 3;
	} else {
		if (lbc->cmp_ratio > 512)
			lbc->start_qp = 0;
		else
			lbc->start_qp = 1;
	}

	if (lbc->cmp_ratio == 1024)
		lbc->is_lossy = 0;
	else
		lbc->is_lossy = 1;

	lbc->mb_th = 0x18;/*12, 24, 36, 48*/
	lbc->mb_num = clamp((int)DIV_ROUND_UP(tdm_rx->format.width, mb_len), 3, 88);

	bit_depth = 12;
	width_stride = ALIGN(tdm_rx->format.width, 48);
	if (lbc->is_lossy) {
		lbc->line_tar_bits = roundup((lbc->cmp_ratio * width_stride * bit_depth) / 1024, align);
		lbc->line_tar_bits = min(lbc->line_tar_bits, roundup(width_stride * bit_depth, align));
	} else
		lbc->line_tar_bits = roundup(width_stride / 24 * (bit_depth * 24 + 24 + 2), align);

	if (lbc->is_lossy) {
		lbc->line_max_bit = roundup((mb_max_bits_ratio + 1024) * lbc->line_tar_bits / 1024, align);
		lbc->line_max_bit = min(lbc->line_max_bit, roundup(width_stride * bit_depth, align));
	} else
		lbc->line_max_bit = roundup(width_stride / 24 * (bit_depth * 24 + 24 + 2), align);
	lbc->mb_min_bit = clamp((int)((lbc->cmp_ratio * bit_depth * mb_len) / 1024 / 2 * mb_min_bits_ratio / 1024), 36, 288);
	lbc->gbgr_ratio = 0x200;
}

static void tdm_rx_cpy(struct tdm_rx_dev *source, struct tdm_rx_dev *rx)
{
	if (!source || !rx)
		return;

	memcpy(rx->tdm_fmt, source->tdm_fmt, sizeof(struct tdm_format));
	memcpy(&rx->format, &source->format, sizeof(struct v4l2_mbus_framefmt));
	rx->buf_size = source->buf_size;
	rx->buf_cnt = source->buf_cnt;
	rx->ws.wdr_mode = source->ws.wdr_mode;
	rx->ws.data_fifo_depth = source->ws.data_fifo_depth;
	rx->ws.head_fifo_depth = source->ws.head_fifo_depth;
}

static int set_chn_use(struct tdm_dev *tdm, unsigned int bits)
{
	return tdm->bitmap_chn_use |= bits;
}

static void clr_chn_use(struct tdm_dev *tdm, unsigned int bits)
{
	tdm->bitmap_chn_use &= ~bits;
}

static int get_chn_can_use(struct tdm_dev *tdm, unsigned int bits)
{
	return tdm->bitmap_chn_use & bits & 0xf;
}

static int get_chn_use_tatol(struct tdm_dev *tdm)
{
	unsigned int cnt = 0, i;

	for (i = 0; i < TDM_RX_NUM; i++)
		if (tdm->bitmap_chn_use & (0x1 << i))
			cnt++;
	return cnt;
}

static int tdm_check_wdr_mode(struct tdm_rx_dev *tdm_rx)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	struct rx_chn_fmt *rcf;
	struct list_head *head = &tdm->working_chn_fmt;

	if (tdm->work_mode == TDM_ONLINE) {
		if (!list_empty(head)) {
			rcf = list_entry(head->next, struct rx_chn_fmt, list);
			vin_err("tdm%d working online mode, rx%d working, tdm can not be open again!",
				tdm->id, rcf->rx_dev->id);
			return -1;
		}
		if ((tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE || tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) && tdm_rx->id != 0) {
			vin_err("tdm%d working online mode, wdr mode must work in rx0 instead of rx%d",
				tdm->id, tdm_rx->id);
			return -1;
		}
		return 0;
	} else {
		switch (tdm_rx->ws.wdr_mode) {
		case ISP_DOL_WDR_MODE:
			if (tdm_rx->id != 0x0 && tdm_rx->id != 0x2)
				vin_err("tdm_rx%d not support 2f-dol_wdr", tdm_rx->id);
			else if (tdm_rx->id == 0x0 && get_chn_can_use(tdm, 0x3))
				vin_err("tdm_rx%d set to 2f-dol_wdr, but rx0/rx1 is %s/%s", tdm_rx->id,
					get_chn_can_use(tdm, 0x1) ? "used":"unused", get_chn_can_use(tdm, 0x2) ? "used":"unused");
			else if (tdm_rx->id == 0x2 && get_chn_can_use(tdm, 0xc))
				vin_err("tdm_rx%d set to 2f-dol_wdr, but rx2/rx3 is %s/%s", tdm_rx->id,
					get_chn_can_use(tdm, 0x4) ? "used":"unused", get_chn_can_use(tdm, 0x8) ? "used":"unused");

			return (tdm_rx->id != 0x0 && tdm_rx->id != 0x2) ||
				get_chn_can_use(tdm, (tdm_rx->id) ? 0xc : 0x3);
		case ISP_3FDOL_WDR_MODE:
			if (tdm_rx->id != 0x0)
				vin_err("tdm_rx%d not support 3f-dol_wdr", tdm_rx->id);
			else if (tdm_rx->id == 0x0 && get_chn_can_use(tdm, 0x7))
				vin_err("tdm_rx%d set to 2f-dol_wdr, but rx0/rx1/rx2 is %s/%s/%s", tdm_rx->id,
					get_chn_can_use(tdm, 0x1) ? "used":"unused", get_chn_can_use(tdm, 0x2) ? "used":"unused",
					get_chn_can_use(tdm, 0x4) ? "used":"unused");

			return (tdm_rx->id != 0x0) || get_chn_can_use(tdm, 0x7);
		case ISP_SEHDR_MODE:
		case ISP_COMANDING_MODE:
		case ISP_NORMAL_MODE:
			if (get_chn_can_use(tdm, 0x1 << tdm_rx->id))
				vin_err("tdm_rx%d is used!", tdm_rx->id);

			return get_chn_can_use(tdm, 0x1 << tdm_rx->id);
		}
		return 0;
	}
}

static void tdm_save_wdr_mode(struct tdm_rx_dev *tdm_rx, struct rx_chn_fmt *chn_fmt)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	int ret;

	tdm_rx->list = &chn_fmt->list;
	list_add_tail(&chn_fmt->list, &tdm->working_chn_fmt);
	if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE)
		ret = set_chn_use(tdm, 0x3 << ((tdm_rx->id / 2) * 2));
	else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE)
		ret = set_chn_use(tdm, 0x7 << (tdm_rx->id / 3));
	else
		ret = set_chn_use(tdm, 0x1 << tdm_rx->id);
	vin_log(VIN_LOG_TDM, "save fmt %d, map = 0x%x\n", tdm_rx->ws.wdr_mode, ret);
}

static void tdm_del_wdr_mode(struct tdm_rx_dev *tdm_rx)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);

	list_del(tdm_rx->list);
	if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE)
		clr_chn_use(tdm, 0x3 << (tdm_rx->id / 2) * 2);
	else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE)
		clr_chn_use(tdm, 0x7 << (tdm_rx->id / 3));
	else
		clr_chn_use(tdm, 0x1 << tdm_rx->id);

	kfree(list_entry(tdm_rx->list, struct rx_chn_fmt, list));
}

static int tdm_is_wdr_working(struct tdm_rx_dev *tdm_rx)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	struct rx_chn_fmt *chn_fmt;

	list_for_each_entry(chn_fmt, &tdm->working_chn_fmt, list) {
		if (chn_fmt->wdr_mode == ISP_DOL_WDR_MODE || chn_fmt->wdr_mode == ISP_3FDOL_WDR_MODE)
			return chn_fmt->wdr_mode;
	}
	return 0;
}

static int tdm_set_rx_cfg(struct tdm_rx_dev *tdm_rx, unsigned int en)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	u32 size = 0, tatol;
	u16 rx_pkg_line_words = 0;
	int ret, i;
	unsigned int tdm_buf_num;
	unsigned int overlayer;

	if (en) {
		if (tdm_rx->large_image == 3) {
			overlayer = vin_set_large_overlayer(tdm_rx->format.width);
			if (tdm_rx->id == 0) {
				tdm_rx->format.width = tdm_rx->format.width / 2 + overlayer;
				csic_tdm_set_tx_chn_cfg_mode(tdm->id, CH0_1);
			} else if (tdm_rx->id == 1) {
				tdm_rx->format.width = tdm_rx->format.width / 2 + overlayer;
			}
		} else
			csic_tdm_set_tx_chn_cfg_mode(tdm->id, TX_NONE);
		csic_tdm_rx_set_min_ddr_size(tdm->id, tdm_rx->id, DDRSIZE_512b);
		csic_tdm_rx_input_bit(tdm->id, tdm_rx->id, tdm_rx->tdm_fmt->input_type);
		csic_tdm_rx_input_fmt(tdm->id, tdm_rx->id, tdm_rx->tdm_fmt->raw_fmt);
		csic_tdm_rx_input_size(tdm->id, tdm_rx->id, tdm_rx->format.width, tdm_rx->format.height);

		tatol = get_chn_use_tatol(tdm);
		vin_log(VIN_LOG_TDM, "work tatol %d\n", tatol);
		if (tdm->work_mode == TDM_ONLINE) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1)
			csic_tdm_rx_data_fifo_depth(tdm->id, tdm_rx->id, tdm->tx_cfg.data_depth);
			csic_tdm_rx_head_fifo_depth(tdm->id, tdm_rx->id, tdm->tx_cfg.head_depth);
#else
			csic_tdm_rx_data_fifo_depth(tdm->id, tdm_rx->id, tdm_rx->ws.data_fifo_depth);
			csic_tdm_rx_head_fifo_depth(tdm->id, tdm_rx->id, tdm_rx->ws.head_fifo_depth);
#endif
		} else {
#if IS_ENABLED(CONFIG_AVG_TDM_FIFO)
			if (tdm_rx->ws.data_fifo_depth && tdm_rx->ws.head_fifo_depth) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1)
				csic_tdm_rx_data_fifo_depth(tdm->id, tdm_rx->id, tdm_rx->ws.data_fifo_depth);
				csic_tdm_rx_head_fifo_depth(tdm->id, tdm_rx->id, tdm_rx->ws.head_fifo_depth);
#else
				csic_tdm_rx_data_fifo_depth(tdm->id, tdm_rx->id, MAX_RX_DATA_FIFO_DEPTH / 4);
				csic_tdm_rx_head_fifo_depth(tdm->id, tdm_rx->id, MAX_RX_HEAD_FIFO_DEPTH / 4);
#endif
			} else {
				csic_tdm_rx_data_fifo_depth(tdm->id, tdm_rx->id, (512)/4);
				csic_tdm_rx_head_fifo_depth(tdm->id, tdm_rx->id, (32)/4);
			}
#else
			if (tdm_rx->ws.data_fifo_depth && tdm_rx->ws.head_fifo_depth) {
				csic_tdm_rx_data_fifo_depth(tdm->id, tdm_rx->id, tdm_rx->ws.data_fifo_depth);
				csic_tdm_rx_head_fifo_depth(tdm->id, tdm_rx->id, tdm_rx->ws.head_fifo_depth);
			} else {
				if (tdm_rx->id == 0 || tdm_rx->id == 1) {
					csic_tdm_rx_data_fifo_depth(tdm->id, tdm_rx->id, (512)/2);
					csic_tdm_rx_head_fifo_depth(tdm->id, tdm_rx->id, (32)/2);
				} else {
					csic_tdm_rx_data_fifo_depth(tdm->id, tdm_rx->id, (512)/4);
					csic_tdm_rx_head_fifo_depth(tdm->id, tdm_rx->id, (32)/4);
					vin_warn("use ioctl VIDIOC_SET_TDM_DEPTH setting max_ch to make work stable!\n");
				}
			}
#endif
		}
		if (tdm_rx->ws.pkg_en) {
			csic_tdm_rx_pkg_enable(tdm->id, tdm_rx->id);
			rx_pkg_line_words = DIV_ROUND_UP(tdm_rx->tdm_fmt->input_bit_width * tdm_rx->format.width, 32);
			csic_tdm_rx_pkg_line_words(tdm->id, tdm_rx->id, ALIGN(rx_pkg_line_words, 8));
		} else if (tdm_rx->ws.lbc_en) {
			if (tdm_rx->id >= 2) {
				vin_err("Only tdm_rx0/1 support lbc!, set to pkg\n");
				csic_tdm_rx_pkg_enable(tdm->id, tdm_rx->id);
				rx_pkg_line_words = DIV_ROUND_UP(tdm_rx->tdm_fmt->input_bit_width * tdm_rx->format.width, 32);
				csic_tdm_rx_pkg_line_words(tdm->id, tdm_rx->id, ALIGN(rx_pkg_line_words, 8));
			} else {
				csic_tdm_rx_lbc_enable(tdm->id, tdm_rx->id);
				tdm_rx_lbc_cal_para(tdm_rx);
				rx_pkg_line_words = roundup(tdm_rx->lbc.line_max_bit + tdm_rx->lbc.mb_min_bit, 512) / 32;
				csic_tdm_rx_pkg_line_words(tdm->id, tdm_rx->id, ALIGN(rx_pkg_line_words, 8));
				csic_tdm_lbc_cfg(tdm->id, tdm_rx->id, &tdm_rx->lbc);

				if (tdm->work_mode == TDM_OFFLINE)
					tdm_rx->ws.line_num_ddr_en = 1;
				else
					tdm_rx->ws.line_num_ddr_en = 0;
				csic_tdm_rx_set_line_num_ddr(tdm->id, tdm_rx->id, tdm_rx->ws.line_num_ddr_en);
			}
		} else if (tdm_rx->ws.sync_en) {
			csic_tdm_rx_sync_enable(tdm->id, tdm_rx->id);
		}

		tdm_rx->width = tdm_rx->format.width;
		tdm_rx->height = tdm_rx->format.height;

		if (tdm->work_mode == TDM_ONLINE) {
			if (tdm_rx->ws.pkg_en || tdm_rx->ws.lbc_en)
				tdm_buf_num = 1;
			else
				tdm_buf_num = 0;
		} else {
#if !IS_ENABLED(CONFIG_TDM_ONE_BUFFER)
			if (tdm_rx->sensor_fps <= 30)
				tdm_buf_num = 2;
			else if (tdm_rx->sensor_fps <= 60)
				tdm_buf_num = 3;
			else if (tdm_rx->sensor_fps <= 120)
				tdm_buf_num = 4;
			else
				tdm_buf_num = TDM_BUFS_NUM;
#else
			tdm_buf_num = 1;
#endif
		}
		if (tdm_buf_num) {
			if (tdm_rx->ws.pkg_en)
				size = ALIGN(tdm_rx->width * tdm_rx->tdm_fmt->input_bit_width, 512) * tdm_rx->height / 8 + ALIGN(tdm_rx->height, 64);
			else if (tdm_rx->ws.lbc_en)
				size = ALIGN(rx_pkg_line_words, 8) * tdm_rx->height * 4 + ALIGN(tdm_rx->height, 64);
			ret = tdm_rx_bufs_alloc(tdm_rx, size, tdm_buf_num);
			if (ret)
				return ret;
#if !IS_ENABLED(CONFIG_TDM_ONE_BUFFER)
			csic_tdm_rx_set_buf_num(tdm->id, tdm_rx->id, tdm_buf_num - 1);
			for (i = 0; i < tdm_buf_num; i++) {
				csic_tdm_rx_set_address(tdm->id, tdm_rx->id, (unsigned long)tdm_rx->buf[i].dma_addr);
			}

			for (i = 0; i < 16 - tdm_buf_num; i++)
				csic_tdm_rx_set_address(tdm->id, tdm_rx->id, 0);
#else
			/* tdm need two buffer */
			csic_tdm_rx_set_buf_num(tdm->id, tdm_rx->id, (tdm_buf_num + 1) - 1);
			for (i = 0; i < tdm_buf_num; i++) {
				csic_tdm_rx_set_address(tdm->id, tdm_rx->id, (unsigned long)tdm_rx->buf[i].dma_addr);
				csic_tdm_rx_set_address(tdm->id, tdm_rx->id, (unsigned long)tdm_rx->buf[i].dma_addr);
			}
#endif
		} else
			csic_tdm_rx_set_buf_num(tdm->id, tdm_rx->id, 0);

		if (tdm_rx->ws.tx_func_en)
			csic_tdm_rx_tx_enable(tdm->id, tdm_rx->id);
		else
			csic_tdm_rx_tx_disable(tdm->id, tdm_rx->id);
	} else {
		csic_tdm_rx_tx_disable(tdm->id, tdm_rx->id);
		csic_tdm_rx_pkg_disable(tdm->id, tdm_rx->id);
		csic_tdm_rx_lbc_disable(tdm->id, tdm_rx->id);
		csic_tdm_rx_sync_disable(tdm->id, tdm_rx->id);
		csic_tdm_rx_set_line_num_ddr(tdm->id, tdm_rx->id, 0);
	}

	vin_log(VIN_LOG_TDM, "tdm%d set rx%d tx en OK!\n", tdm->id, tdm_rx->id);
	return 0;
}

static int tdm_cal_rx_chn_cfg_mode(struct tdm_rx_dev *tdm_rx, enum rx_chn_cfg_mode *mode)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	unsigned int wdr_mode;
	enum rx_chn_cfg_mode last_mode;

	last_mode = tdm->ws.rx_chn_mode;
	wdr_mode = tdm_is_wdr_working(tdm_rx);

	if (tdm->work_mode == TDM_OFFLINE) {
		switch (wdr_mode) {
		case ISP_3FDOL_WDR_MODE:
			if (tdm_rx->ws.wdr_mode == ISP_NORMAL_MODE)
				*mode = WDR_3F_AND_LINEAR;
			else
				return -1;
			break;
		case ISP_DOL_WDR_MODE:
			if (tdm_rx->ws.wdr_mode == ISP_NORMAL_MODE) {
				if (last_mode == WDR_2Fx2) {
					vin_err("tdm_rx2 work in wdr mode, so tdm_rx0&tdm_rx1 cannot work in normal mode!\n");
					return -1;
				} else
					*mode = WDR_2F_AND_LINEARx2;
			} else if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE)
				*mode = WDR_2Fx2;
			else
				return -1;
			break;
		default:
			if (tdm_rx->ws.wdr_mode == ISP_NORMAL_MODE)
				*mode = LINEARx4;
			else if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
				if (tdm_rx->id == 2)
					*mode = WDR_2Fx2;
				else
					*mode = WDR_2F_AND_LINEARx2;
			} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE)
				*mode = WDR_3F_AND_LINEAR;
			else
				return -1;
			break;
		}
	} else {
		if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE)
			*mode = WDR_2F_AND_LINEARx2;
		else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE)
			*mode = WDR_3F_AND_LINEAR;
		else
			*mode = LINEARx4;
	}
	if (tdm->stream_cnt && last_mode != *mode)
		csic_tdm_set_rx_chn_cfg_mode(tdm->id, *mode);

	vin_log(VIN_LOG_TDM, "tdm%d work on %s mode, now rx chn cfg mode is%d\n",
		tdm->id, tdm->work_mode ? "offline":"online", *mode);
	return 0;
}
#else /* else not TDM_V200 */
static void tdm_set_tx_blank(struct tdm_dev *tdm)
{
	csic_tdm_omode(tdm->id, 1);
	csic_tdm_set_hblank(tdm->id, TDM_TX_HBLANK);
	csic_tdm_set_bblank_fe(tdm->id, TDM_TX_VBLANK/2);
	csic_tdm_set_bblank_be(tdm->id, TDM_TX_VBLANK/2);
}

static int tdm_set_rx_cfg(struct tdm_rx_dev *tdm_rx, unsigned int en)
{
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	__maybe_unused u32 size, tatol;
	int ret, i;

	csic_tdm_rx_set_buf_num(tdm->id, tdm_rx->id, TDM_BUFS_NUM - 1);
	csic_tdm_rx_ch0_en(tdm->id, tdm_rx->id, 1);
	csic_tdm_rx_set_min_ddr_size(tdm->id, tdm_rx->id, DDRSIZE_256b);
	csic_tdm_rx_input_bit(tdm->id, tdm_rx->id, tdm_rx->tdm_fmt->input_type);
	csic_tdm_rx_input_size(tdm->id, tdm_rx->id, tdm_rx->format.width, tdm_rx->format.height);

	tdm_rx->width = tdm_rx->format.width;
	tdm_rx->height = tdm_rx->format.height;
	size = (roundup(tdm_rx->width, 512)) * tdm_rx->height * tdm_rx->tdm_fmt->input_bit_width / 8;
	ret = tdm_rx_bufs_alloc(tdm_rx, size, TDM_BUFS_NUM);
	if (ret)
		return ret;
	for (i = 0; i < TDM_BUFS_NUM; i++)
		csic_tdm_rx_set_address(tdm->id, tdm_rx->id, (unsigned long)tdm_rx->buf[i].dma_addr);

	vin_log(VIN_LOG_TDM, "tdm%d set rx%d tx en OK!\n", tdm->id, tdm_rx->id);
	return 0;

}
#endif

#ifdef TDM_V200
static int sunxi_tdm_top_s_stream(struct tdm_rx_dev *rx, int enable)
{
	struct tdm_dev *tdm = container_of(rx, struct tdm_dev, tdm_rx[rx->id]);
	unsigned int ini_en = 0;

	if (tdm->work_mode == TDM_ONLINE && rx->id != 0) {
		vin_err("tdm%d work on online mode, tdm_rx%d cannot to work!!\n", tdm->id, rx->id);
		return -1;
	}

	if (enable && (tdm->stream_cnt)++ > 0)
		return 0;
	else if (!enable && (tdm->stream_cnt == 0 || --(tdm->stream_cnt) > 0))
		return 0;

	if (enable) {
		csic_tdm_top_enable(tdm->id);
		if (tdm->ws.tdm_en)
			csic_tdm_enable(tdm->id);
		vin_log(VIN_LOG_TDM, "%s rx mode %d, tx mode %di, work mode %d\n",
			__func__, tdm->ws.rx_chn_mode, tdm->ws.tx_chn_mode, tdm->work_mode);
		csic_tdm_set_rx_chn_cfg_mode(tdm->id, tdm->ws.rx_chn_mode);
		csic_tdm_set_tx_chn_cfg_mode(tdm->id, tdm->ws.tx_chn_mode);
		csic_tdm_set_work_mode(tdm->id, tdm->work_mode);
/*		if (tdm->work_mode == TDM_OFFLINE)
			tdm->ws.speed_dn_en = 1;
*/
		csic_tdm_set_speed_dn(tdm->id, tdm->ws.speed_dn_en);

		/*tx init*/
		tdm_set_tx_blank(tdm);
		/* rx fifo clear */
		csic_tdm_rx_data_fifo_clear(tdm->id);
		csic_tdm_set_tx_t1_cycle(tdm->id, tdm->tx_cfg.t1_cycle = 0x40);
		if (tdm->work_mode == TDM_ONLINE) {
			csic_tdm_fifo_mode(tdm->id, tdm->tx_cfg.fifo_mode);
			csic_tdm_set_tx_fifo_depth(tdm->id, tdm->tx_cfg.head_depth, tdm->tx_cfg.data_depth);
			csic_tdm_set_tx_id0_fifo_depth(tdm->id, tdm->tx_cfg.head_depth, tdm->tx_cfg.data_depth);
			csic_tdm_set_tx_t2_cycle(tdm->id, tdm->tx_cfg.t2_cycle = 0xfa0);
		} else {
			if (rx->ws.data_fifo_depth && rx->ws.head_fifo_depth) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1)
				csic_tdm_set_tx_fifo_depth(tdm->id, rx->ws.head_fifo_depth, min(rx->ws.data_fifo_depth, (u16)192));
#else
				csic_tdm_fifo_mode(tdm->id, tdm->tx_cfg.fifo_mode);
				csic_tdm_set_tx_fifo_depth(tdm->id, tdm->tx_cfg.head_depth, tdm->tx_cfg.data_depth);
				csic_tdm_set_tx_id0_fifo_depth(tdm->id, tdm->tx_cfg.head_depth, tdm->tx_cfg.data_depth);
#endif
			} else {
#if IS_ENABLED(CONFIG_WDR)
				csic_tdm_set_tx_fifo_depth(tdm->id, 32/4, 512/4);
#else
				if (rx->id == 0 || rx->id == 1) {
					csic_tdm_set_tx_fifo_depth(tdm->id, 32/2, min(512/2, 192));
				} else {
					csic_tdm_set_tx_fifo_depth(tdm->id, 32/4, 512/4);
					vin_warn("use ioctl VIDIOC_SET_TDM_DEPTH setting max_ch to make work stable!\n");
				}
#endif
			}
			csic_tdm_set_tx_t2_cycle(tdm->id, tdm->tx_cfg.t2_cycle = 0x0);
		}
		if (tdm->ws.speed_dn_en) {
			csic_tdm_set_tx_data_rate(tdm->id, tdm->tx_cfg.valid_num, tdm->tx_cfg.invalid_num);
		}
		csic_tdm_tx_enable(tdm->id);
		csic_tdm_tx_cap_enable(tdm->id);
		ini_en = RX_FRM_LOST_INT_EN | RX_FRM_ERR_INT_EN | RX_BTYPE_ERR_INT_EN |
				RX_BUF_FULL_INT_EN | RX_HB_SHORT_INT_EN | RX_FIFO_FULL_INT_EN |
				TDM_LBC_ERR_INT_EN | TDM_FIFO_UNDER_INT_EN | SPEED_DN_FIFO_FULL_INT_EN |
				SPEED_DN_HSYNC_INT_EN | RX_CHN_CFG_MODE_INT_EN | TX_CHN_CFG_MODE_INT_EN |
				RDM_LBC_FIFO_FULL_INT_EN;
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		if (tdm->work_mode == TDM_ONLINE && tdm->ws.speed_dn_en) {
			if (rx->ws.wdr_mode == ISP_NORMAL_MODE)
				ini_en |= RX0_FRM_DONE_INT_EN;
			else if (rx->ws.wdr_mode == ISP_DOL_WDR_MODE)
				ini_en |= RX1_FRM_DONE_INT_EN;
		}
#endif
		csic_tdm_int_clear_status(tdm->id, TDM_INT_ALL);
		csic_tdm_int_enable(tdm->id, ini_en);
		vin_log(VIN_LOG_TDM, "tdm%d open first, setting the interrupt and tx configuration!\n", tdm->id);

	} else {
		tdm->ws.speed_dn_en = 0;
		csic_tdm_int_disable(tdm->id, TDM_INT_ALL);
		if (tdm->work_mode == TDM_ONLINE) {
			csic_tdm_set_speed_dn(tdm->id, tdm->ws.speed_dn_en);
			csic_tdm_tx_cap_disable(tdm->id);
			csic_tdm_tx_disable(tdm->id);
			csic_tdm_disable(tdm->id);
			csic_tdm_top_disable(tdm->id);
		}
	}

	return 0;
}

static void tdm_set_rx_data_rate(struct tdm_rx_dev *tdm_rx, unsigned int en)
{
#if VIN_FALSE
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	struct rx_chn_fmt *chn_fmt = NULL;
	unsigned int tx_need_clk = 0;
	unsigned int total_tx_rate_numerator = 0;

	if (tdm->work_mode == TDM_ONLINE)
		return;

	if (en) {
		tx_need_clk = roundup((tdm_rx->format.width + TDM_TX_HBLANK) * (tdm_rx->format.height + TDM_TX_VBLANK) * tdm_rx->sensor_fps, 1000000);
		tdm_rx->ws.tx_rate_denominator = 255;
		tdm_rx->ws.tx_rate_numerator = tdm_rx->ws.tx_rate_denominator * (tx_need_clk / 1000000) / (tdm_rx->isp_clk / 100 * 90 / 1000000);
		total_tx_rate_numerator += tdm_rx->ws.tx_rate_numerator;
		vin_log(VIN_LOG_TDM, "tdm_rx%d tx_need_clk is %d, tx_rate_numerator is %d\n", tdm_rx->id, tx_need_clk, tdm_rx->ws.tx_rate_numerator);
	}

	list_for_each_entry(chn_fmt, &tdm->working_chn_fmt, list) {
		if (chn_fmt->rx_dev->id == tdm_rx->id)
			continue;
		total_tx_rate_numerator += chn_fmt->rx_dev->ws.tx_rate_numerator;
	}

	if (total_tx_rate_numerator > 255) {
		total_tx_rate_numerator = 255;
		vin_warn("tdm tx valid_num morn than 255, you maybe need increase isp_clk\n");
	}
	tdm->tx_cfg.valid_num = total_tx_rate_numerator;
	if (tdm->tx_cfg.valid_num)
		tdm->tx_cfg.invalid_num = 255 - tdm->tx_cfg.valid_num;
	else
		tdm->tx_cfg.invalid_num = 0;
	csic_tdm_set_tx_data_rate(tdm->id, tdm->tx_cfg.valid_num, tdm->tx_cfg.invalid_num);
	vin_log(VIN_LOG_TDM, "tdm_rx%d %s, tx valid_num is %d, invalid_num is %d\n",
				tdm_rx->id, en ? "enable" : "disable", tdm->tx_cfg.valid_num, tdm->tx_cfg.invalid_num);
#else
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	struct rx_chn_fmt *chn_fmt = NULL;
	unsigned int width[TDM_RX_NUM], height[TDM_RX_NUM], fps[TDM_RX_NUM], vts[TDM_RX_NUM];
	bool rx_stream[TDM_RX_NUM] = {false, false, false, false};
	unsigned int wh_fps = 0, vw_fps = 0, hh_fps = 0, vh_fps = 0;
	__maybe_unused unsigned int vb_percentage[TDM_RX_NUM], vb_percent_min = 100, isp_need_clk, onebuffer_isp_need_clk;
	unsigned int isp_clk = 0;
	unsigned int i;
	unsigned int n_rx_use = 0;

	if (tdm->work_mode == TDM_ONLINE || !tdm->ws.speed_dn_en)
		return;

	if (en) {
		rx_stream[tdm_rx->id] = true;
		width[tdm_rx->id] = tdm_rx->format.width;
		height[tdm_rx->id] = tdm_rx->format.height;
		fps[tdm_rx->id] = tdm_rx->sensor_fps;
		vts[tdm_rx->id] = tdm_rx->vts;
		isp_clk = tdm_rx->isp_clk / 100 * 90; /* 90% */
	} else {
		rx_stream[tdm_rx->id] = false;
		width[tdm_rx->id] = 0;
		height[tdm_rx->id] = 0;
		fps[tdm_rx->id] = 0;
		vts[tdm_rx->id] = 0;
		isp_clk = tdm_rx->isp_clk / 100 * 90; /* 90% */
	}
	vin_log(VIN_LOG_TDM, "tdm_rx%d %s, size is %dfps@%dx%d, isp_clk is %d\n",
		tdm_rx->id, en ? "enable" : "disable",
		tdm_rx->sensor_fps, tdm_rx->format.width, tdm_rx->format.height,
		tdm_rx->isp_clk);
	list_for_each_entry(chn_fmt, &tdm->working_chn_fmt, list) {
		if (chn_fmt->rx_dev->id == tdm_rx->id)
			continue;
		rx_stream[chn_fmt->rx_dev->id] = true;
		width[chn_fmt->rx_dev->id] = chn_fmt->rx_dev->format.width;
		height[chn_fmt->rx_dev->id] = chn_fmt->rx_dev->format.height;
		fps[chn_fmt->rx_dev->id] = chn_fmt->rx_dev->sensor_fps;
		vts[chn_fmt->rx_dev->id] = chn_fmt->rx_dev->vts;
		vin_log(VIN_LOG_TDM, "at the same time tdm_rx%d %s, size is %dfps@%dx%d, vts is %d\n",
				chn_fmt->rx_dev->id, "enabled",
				chn_fmt->rx_dev->sensor_fps, chn_fmt->rx_dev->format.width, chn_fmt->rx_dev->format.height,
				chn_fmt->rx_dev->vts);
	}
	for (i = 0; i < TDM_RX_NUM; i++) {
		if (rx_stream[i] == false) {
			width[i] = 0;
			height[i] = 0;
			fps[i] = 0;
			vts[i] = 0;
		} else
			n_rx_use++;
	}


	/*
	 * m: valid num, n: invalid num, m + n = 255
	 * rx0 real work clock num: sum0 = ((w0*(255/m)+hb)*h0+(w0+hb)*vb)*fps0
	 * rx1 real work clock num: sum1 = ((w1*(255/m)+hb)*h1+(w1+hb)*vb)*fps1
	 * rx2 real work clock num: sum2 = ((w2*(255/m)+hb)*h2+(w2+hb)*vb)*fps2
	 * rx3 real work clock num: sum3 = ((w3*(255/m)+hb)*h3+(w3+hb)*vb)*fps3
	 * sum0 + sum1 + sum2 + sum3 = isp_clk
	 * 255/m * wh_fps = (isp_clk - hh_fps - vw_fps - vh_fps)
	 *
	 * tdm one buffer:
	 * sum0 + sum1 + sum2 + sum3 = isp_need_clk_cycle
	 * wh_fps + hh_fps + vw_fps + vh_fps = isp_need_clk_cycle
	 * isp_need_clk_cycle = isp_need_clk_cycle / (n - 1), n mean use n rx
	 * (isp_need_clk_cycle/onebuffer_isp_need_clk = T) < (T1 = 1* vb% = 1 * ((vs - w)/vs)
	 * onebuffer_isp_need_clk / isp_clk = m/255
	 */
#if !IS_ENABLED(CONFIG_TDM_ONE_BUFFER)
	for (i = 0; i < TDM_RX_NUM; i++) {
		wh_fps += (width[i] * height[i] * fps[i]);
		hh_fps += (TDM_TX_HBLANK_OFFLINE * height[i] * fps[i]);
		vw_fps += (TDM_TX_VBLANK * width[i] * fps[i]);
		vh_fps += (TDM_TX_HBLANK_OFFLINE * TDM_TX_VBLANK * fps[i]);
	}
	vin_log(VIN_LOG_TDM, "wh_fps %d, hh_fps %d, vw_fps %d, vh_fps %d\n", wh_fps, hh_fps, vw_fps, vh_fps);

	if (wh_fps) {
		wh_fps = roundup(wh_fps, 100);
		tdm->tx_cfg.valid_num = DIV_ROUND_UP(10 * 255 * 128 / (128 * ((isp_clk - hh_fps - vw_fps - vh_fps) / 100) / (wh_fps / 100)), 10);
	} else
		tdm->tx_cfg.valid_num = 0;
#else
	for (i = 0; i < TDM_RX_NUM; i++) {
		wh_fps += (width[i] * height[i] * fps[i]);
		hh_fps += (TDM_TX_HBLANK_OFFLINE * height[i] * fps[i]);
		vw_fps += (TDM_TX_VBLANK * width[i] * fps[i]);
		vh_fps += (TDM_TX_HBLANK_OFFLINE * TDM_TX_VBLANK * fps[i]);

		if (height[i] && vts[i]) {
			if (vts[i] <= height[i]) {
				vin_err("tdm_rx%d vts(%d) is small than height(%d)\n", i, vts[i], height[i]);
				return;
			}
			vb_percentage[i] = 100 * (vts[i] - height[i]) / vts[i];
			vin_print("tdm_rx%d vb is %d%%\n", i, vb_percentage[i]);
		} else
			vb_percentage[i] = 0;
		if (vb_percentage[i] != 0)
			vb_percent_min = min(vb_percent_min, vb_percentage[i]);
	}
	isp_need_clk = wh_fps + hh_fps + vw_fps + vh_fps;
	if (n_rx_use > 1)
		isp_need_clk = isp_need_clk / n_rx_use * (n_rx_use - 1);
	onebuffer_isp_need_clk = isp_need_clk / vb_percent_min * 100;

	vin_print("wh_fps %d, hh_fps %d, vw_fps %d, vh_fps %d, vb_percent_min is %d, isp_need_clk is %d, onebuffer_isp_need_clk is %d\n",
					wh_fps, hh_fps, vw_fps, vh_fps, vb_percent_min, isp_need_clk, onebuffer_isp_need_clk);

	tdm->tx_cfg.valid_num = onebuffer_isp_need_clk / (isp_clk / 255);
	tdm->tx_cfg.valid_num += 8;
#endif
	if (tdm->tx_cfg.valid_num > 255) {
		tdm->tx_cfg.valid_num = 255;
		vin_warn("tdm tx valid_num morn than 255, you maybe need increase isp_clk\n");
	}
	if (tdm->tx_cfg.valid_num)
		tdm->tx_cfg.invalid_num = 255 - tdm->tx_cfg.valid_num;
	else
		tdm->tx_cfg.invalid_num = 0;
	csic_tdm_set_tx_data_rate(tdm->id, tdm->tx_cfg.valid_num, tdm->tx_cfg.invalid_num);
	vin_log(VIN_LOG_TDM, "tdm_rx%d %s, tx valid_num is %d, invalid_num is %d\n",
				tdm_rx->id, en ? "enable" : "disable", tdm->tx_cfg.valid_num, tdm->tx_cfg.invalid_num);
#endif
}
#endif

static int sunxi_tdm_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct tdm_rx_dev *tdm_rx = v4l2_get_subdevdata(sd);
	struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	struct v4l2_mbus_framefmt *mf = &tdm_rx->format;
	struct mbus_framefmt_res *res = (void *)mf->reserved;
	__maybe_unused struct tdm_rx_dev *tdm_rx1 = NULL, *tdm_rx2 = NULL, *tdm_rx3 = NULL;
	__maybe_unused struct rx_chn_fmt *chn_fmt;
	__maybe_unused int i;

	switch (res->res_pix_fmt) {
	case V4L2_PIX_FMT_SBGGR8:
	case V4L2_PIX_FMT_SGBRG8:
	case V4L2_PIX_FMT_SGRBG8:
	case V4L2_PIX_FMT_SRGGB8:
	case V4L2_PIX_FMT_SBGGR10:
	case V4L2_PIX_FMT_SGBRG10:
	case V4L2_PIX_FMT_SGRBG10:
	case V4L2_PIX_FMT_SRGGB10:
	case V4L2_PIX_FMT_SBGGR12:
	case V4L2_PIX_FMT_SGBRG12:
	case V4L2_PIX_FMT_SGRBG12:
	case V4L2_PIX_FMT_SRGGB12:
		vin_log(VIN_LOG_FMT, "%s output fmt is raw, return directly\n", __func__);
		return 0;
	default:
		break;
	}
	if (enable) {
#ifdef TDM_V200
		tdm_rx->ws.wdr_mode = res->res_wdr_mode;
		if (tdm_check_wdr_mode(tdm_rx)) {
			tdm_rx->streaming = false;
			return 0;
		} else
			tdm_rx->streaming = true;
		if (tdm_cal_rx_chn_cfg_mode(tdm_rx, &tdm->ws.rx_chn_mode)) {
			vin_err("tdm%d get rx chn fail\n", tdm->id);
			tdm_rx->streaming = false;
			return 0;
		} else
			tdm_rx->streaming = true;
		chn_fmt = kzalloc(sizeof(struct rx_chn_fmt), GFP_KERNEL);
		chn_fmt->wdr_mode = res->res_wdr_mode;
		chn_fmt->rx_dev = tdm_rx;
		tdm_save_wdr_mode(tdm_rx, chn_fmt);
		tdm->rx_stream_cnt[tdm_rx->id] = 0;
		if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
			vin_log(VIN_LOG_FMT, "rx%d wdr mode is 3f wdr\n", tdm_rx->id);
			tdm->ws.tdm_en = 1;
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1)
			tdm->tx_cfg.data_depth = 512/3;
			tdm->tx_cfg.head_depth = 32/3;
#else
			tdm->tx_cfg.fifo_mode = 1;
			tdm->tx_cfg.data_depth = MAX_TX_DATA_FIFO_DEPTH/4;
			tdm->tx_cfg.head_depth = MAX_TX_HEAD_FIFO_DEPTH/4;
			tdm_rx->ws.data_fifo_depth = MAX_RX_DATA_FIFO_DEPTH/4;
			tdm_rx->ws.head_fifo_depth = MAX_RX_HEAD_FIFO_DEPTH/4;
#endif
			tdm_rx->ws.tx_func_en = 1;
			tdm_rx->ws.pkg_en = 1;
			tdm_rx->ws.lbc_en = 0;
			tdm_rx->ws.sync_en = 0;
			tdm_rx1 = &tdm->tdm_rx[1];
			tdm_rx2 = &tdm->tdm_rx[2];
			tdm_rx_cpy(tdm_rx, tdm_rx1);
			tdm_rx_cpy(tdm_rx, tdm_rx2);
			tdm->rx_stream_cnt[1] = 0;
			tdm->rx_stream_cnt[2] = 0;
			if (tdm->work_mode == TDM_OFFLINE) {
				tdm_rx1->ws.tx_func_en = 1;
				tdm_rx2->ws.tx_func_en = 1;
				tdm_rx1->ws.pkg_en = 1;
				tdm_rx2->ws.pkg_en = 1;
				tdm_rx1->ws.lbc_en = 0;
				tdm_rx2->ws.lbc_en = 0;
				tdm_rx1->ws.sync_en = 0;
				tdm_rx2->ws.sync_en = 0;
			} else {
				tdm_rx1->ws.tx_func_en = 1;
				tdm_rx2->ws.tx_func_en = 0;
				tdm_rx1->ws.pkg_en = 1;
				tdm_rx2->ws.pkg_en = 0;
				tdm_rx1->ws.lbc_en = 0;
				tdm_rx2->ws.lbc_en = 0;
				tdm_rx1->ws.sync_en = 0;
				tdm_rx2->ws.sync_en = 0;
			}
		} else if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
			vin_log(VIN_LOG_FMT, "rx%d wdr mode is 2f wdr\n", tdm_rx->id);
			tdm->ws.tdm_en = 1;
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1)
			tdm->tx_cfg.data_depth = 512/2;
			tdm->tx_cfg.head_depth = 32/2;
#else
			tdm->tx_cfg.fifo_mode = 1;
			tdm->tx_cfg.data_depth = MAX_TX_DATA_FIFO_DEPTH/2;
			tdm->tx_cfg.head_depth = MAX_TX_HEAD_FIFO_DEPTH/2;
			tdm_rx->ws.data_fifo_depth = MAX_RX_DATA_FIFO_DEPTH/2;
			tdm_rx->ws.head_fifo_depth = MAX_RX_HEAD_FIFO_DEPTH/2;
#endif
			tdm_rx->ws.tx_func_en = 1;
			if (tdm_rx->id == 0) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6)
				tdm_rx->ws.pkg_en = 1;
				tdm_rx->ws.lbc_en = 0;
#else
				tdm_rx->ws.pkg_en = 0;
				tdm_rx->ws.lbc_en = 1;
#endif
				tdm_rx->ws.sync_en = 0;
				tdm_rx1 = &tdm->tdm_rx[1];
				tdm_rx_cpy(tdm_rx, tdm_rx1);
				tdm->rx_stream_cnt[1] = 0;
				if (tdm->work_mode == TDM_OFFLINE) {
					tdm_rx1->ws.tx_func_en = 1;
					tdm_rx1->ws.pkg_en = 1;
					tdm_rx1->ws.lbc_en = 0;
					tdm_rx1->ws.sync_en = 0;
				} else {
					tdm_rx1->ws.tx_func_en = 0;
					tdm_rx1->ws.pkg_en = 0;
					tdm_rx1->ws.lbc_en = 0;
					tdm_rx1->ws.sync_en = 0;
				}
			} else if (tdm_rx->id == 2) {
				tdm_rx->ws.pkg_en = 1;
				tdm_rx->ws.lbc_en = 0;
				tdm_rx->ws.sync_en = 0;
				tdm_rx3 = &tdm->tdm_rx[3];
				tdm_rx_cpy(tdm_rx, tdm_rx3);
				tdm->rx_stream_cnt[3] = 0;
				if (tdm->work_mode == TDM_OFFLINE) {
					tdm_rx3->ws.tx_func_en = 1;
					tdm_rx3->ws.pkg_en = 1;
					tdm_rx3->ws.lbc_en = 0;
					tdm_rx3->ws.sync_en = 0;
				} else {
					tdm_rx3->ws.tx_func_en = 0;
					tdm_rx3->ws.pkg_en = 0;
					tdm_rx3->ws.lbc_en = 0;
					tdm_rx3->ws.sync_en = 0;
				}
			}
		} else {
			vin_log(VIN_LOG_FMT, "rx%d wdr mode is normal\n", tdm_rx->id);
			if (tdm->work_mode == TDM_OFFLINE) {
				tdm->ws.tdm_en = 1;
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1)
				tdm->tx_cfg.data_depth = 512;
				tdm->tx_cfg.head_depth = 32;
#else
				tdm->tx_cfg.fifo_mode = 0;
				tdm->tx_cfg.data_depth = MAX_TX_DATA_FIFO_DEPTH;
				tdm->tx_cfg.head_depth = MAX_TX_HEAD_FIFO_DEPTH;
				tdm_rx->ws.data_fifo_depth = MAX_RX_DATA_FIFO_DEPTH;
				tdm_rx->ws.head_fifo_depth = MAX_RX_HEAD_FIFO_DEPTH;
#endif
				tdm_rx->ws.tx_func_en = 1;
				if (tdm_rx->id == 0 || tdm_rx->id == 1) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6)
					tdm_rx->ws.pkg_en = 1;
					tdm_rx->ws.lbc_en = 0;
#else
					tdm_rx->ws.pkg_en = 0;
					tdm_rx->ws.lbc_en = 1;
#endif
					tdm_rx->ws.sync_en = 0;
#if !IS_ENABLED(CONFIG_ARCH_SUN55IW3) && !IS_ENABLED(CONFIG_ARCH_SUN60IW1)
					tdm_rx->ws.data_fifo_depth = MAX_RX_DATA_FIFO_DEPTH/2;
					tdm_rx->ws.head_fifo_depth = MAX_RX_HEAD_FIFO_DEPTH/2;
#endif
				} else {
					tdm_rx->ws.pkg_en = 1;
					tdm_rx->ws.lbc_en = 0;
					tdm_rx->ws.sync_en = 0;
#if !IS_ENABLED(CONFIG_ARCH_SUN55IW3) && !IS_ENABLED(CONFIG_ARCH_SUN60IW1)
					tdm_rx->ws.data_fifo_depth = MAX_RX_DATA_FIFO_DEPTH/4;
					tdm_rx->ws.head_fifo_depth = MAX_RX_HEAD_FIFO_DEPTH/4;
#endif
				}
			} else {
				tdm->ws.tdm_en = 1;
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1)
				tdm->tx_cfg.data_depth = 512;
				tdm->tx_cfg.head_depth = 32;
#else
				tdm->tx_cfg.fifo_mode = 0;
				tdm->tx_cfg.data_depth = MAX_TX_DATA_FIFO_DEPTH;
				tdm->tx_cfg.head_depth = MAX_TX_HEAD_FIFO_DEPTH;
				tdm_rx->ws.data_fifo_depth = MAX_RX_DATA_FIFO_DEPTH;
				tdm_rx->ws.head_fifo_depth = MAX_RX_HEAD_FIFO_DEPTH;
#endif
				tdm_rx->ws.tx_func_en = 0;
				tdm_rx->ws.pkg_en = 0;
				tdm_rx->ws.lbc_en = 0;
				tdm_rx->ws.sync_en = 0;
				if (!tdm->ws.speed_dn_en)
					return 0;
			}
		}

		/*reg int*/
		sunxi_tdm_top_s_stream(tdm_rx, enable);
		tdm_set_rx_data_rate(tdm_rx, enable);
		tdm->tdm_rx_reset[tdm_rx->id] = 0;
		tdm_set_rx_cfg(tdm_rx, 1);
		csic_tdm_rx_enable(tdm->id, tdm_rx->id);
		csic_tdm_rx_cap_enable(tdm->id, tdm_rx->id);
		vin_log(VIN_LOG_FMT, "rx%d init end\n", tdm_rx->id);
		if (tdm_rx1) {
			tdm->tdm_rx_reset[1] = 0;
			tdm_set_rx_cfg(tdm_rx1, 1);
			csic_tdm_rx_enable(tdm->id, tdm_rx1->id);
			csic_tdm_rx_cap_enable(tdm->id, tdm_rx1->id);
			vin_log(VIN_LOG_FMT, "rx1 init end\n");
		}
		if (tdm_rx2) {
			tdm->tdm_rx_reset[2] = 0;
			tdm_set_rx_cfg(tdm_rx2, 1);
			csic_tdm_rx_enable(tdm->id, tdm_rx2->id);
			csic_tdm_rx_cap_enable(tdm->id, tdm_rx2->id);
			vin_log(VIN_LOG_FMT, "rx2 init end\n");
		}
		if (tdm_rx3) {
			tdm->tdm_rx_reset[3] = 0;
			tdm_set_rx_cfg(tdm_rx3, 1);
			csic_tdm_rx_enable(tdm->id, tdm_rx3->id);
			csic_tdm_rx_cap_enable(tdm->id, tdm_rx3->id);
			vin_log(VIN_LOG_FMT, "rx3 init end\n");
		}
#else  /* else not TDM_V200 */
		tdm->stream_cnt++;
		tdm_rx->stream_cnt++;

		if (tdm->stream_cnt == 1)
			csic_tdm_top_enable(tdm->id);

		if (tdm_rx->stream_cnt == 1) {
			tdm_set_rx_cfg(tdm_rx, 1);
			vin_log(VIN_LOG_TDM, "tdm_rx%d open first, setting rx configuration!\n", tdm_rx->id);
		} else {
			if (tdm_rx->width != tdm_rx->format.width || tdm_rx->height != tdm_rx->format.height) {
				vin_err("The size of the %d time opening tdm_rx%d is different from the first time opened\n",
					tdm_rx->stream_cnt, tdm_rx->id);
				return -1;
			}
		}
		if (tdm->stream_cnt == 1) {
			tdm_set_tx_blank(tdm);
			csic_tdm_fifo_max_layer_en(tdm->id, 1);
			csic_tdm_int_enable(tdm->id, RX_FRM_LOST_INT_EN | RX_FRM_ERR_INT_EN |
					RX_BTYPE_ERR_INT_EN | RX_BUF_FULL_INT_EN | RX_COMP_ERR_INT_EN |
					RX_HB_SHORT_INT_EN | RX_FIFO_FULL_INT_EN);
			csic_tdm_enable(tdm->id);
			csic_tdm_tx_cap_enable(tdm->id);
			vin_log(VIN_LOG_TDM, "tdm%d open first, setting the interrupt and tx configuration!\n", tdm->id);
		}
		csic_tdm_rx_enable(tdm->id, tdm_rx->id);
		csic_tdm_rx_cap_enable(tdm->id, tdm_rx->id);
#endif
	} else {
#ifdef TDM_V200
		if (!tdm_rx->streaming)
			return 0;
		if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
			tdm_rx1 = &tdm->tdm_rx[1];
			tdm_rx2 = &tdm->tdm_rx[2];
		} else if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
			if (tdm_rx->id == 0)
				tdm_rx1 = &tdm->tdm_rx[1];
			else if (tdm_rx->id == 2)
				tdm_rx3 = &tdm->tdm_rx[3];
		}
		csic_tdm_rx_disable(tdm->id, tdm_rx->id);
		csic_tdm_int_clear_status(tdm->id, TX_FRM_DONE_INT_EN);
		csic_tdm_rx_cap_disable(tdm->id, tdm_rx->id);
		tdm_set_rx_cfg(tdm_rx, 0);
		if (tdm_rx1) {
			csic_tdm_rx_disable(tdm->id, tdm_rx1->id);
			csic_tdm_rx_cap_disable(tdm->id, tdm_rx1->id);
			tdm_set_rx_cfg(tdm_rx1, 0);
			memset(&tdm_rx1->ws, 0, sizeof(struct rx_work_status));
		}
		if (tdm_rx2) {
			csic_tdm_rx_disable(tdm->id, tdm_rx2->id);
			csic_tdm_rx_cap_disable(tdm->id, tdm_rx2->id);
			tdm_set_rx_cfg(tdm_rx2, 0);
			memset(&tdm_rx2->ws, 0, sizeof(struct rx_work_status));
		}
		if (tdm_rx3) {
			csic_tdm_rx_disable(tdm->id, tdm_rx3->id);
			csic_tdm_rx_cap_disable(tdm->id, tdm_rx3->id);
			tdm_set_rx_cfg(tdm_rx3, 0);
			memset(&tdm_rx3->ws, 0, sizeof(struct rx_work_status));
		}
		tdm_del_wdr_mode(tdm_rx);
		memset(&tdm_rx->ws, 0, sizeof(struct rx_work_status));
		tdm_set_rx_data_rate(tdm_rx, enable);
		sunxi_tdm_top_s_stream(tdm_rx, enable);

		if (tdm->stream_cnt == 0)
			usleep_range(1000, 1100);
		else {
			if (csic_tdm_get_tx_ctrl_status(tdm->id) == 0)
				usleep_range(1000, 1100);
			else {
				for (i = 0; i < 100; i++) {
					if (csic_tdm_internal_get_status(tdm->id, TX_FRM_DONE_PD_MASK)) {
						break;
					}
					if (tdm->stream_cnt == 0) {
						usleep_range(1000, 1100);
						break;
					}
					usleep_range(1000, 1100);
					if (i >= 99)
						vin_warn("rx%d wait %dms to free buffer\n", tdm_rx->id, i + 1);
				}
			}
		}

		if ((tdm->work_mode == TDM_ONLINE && tdm->stream_cnt == 0) || tdm->stream_cnt) {
			tdm_rx_bufs_free(tdm_rx);
			if (tdm_rx1)
				tdm_rx_bufs_free(tdm_rx1);
			if (tdm_rx2)
				tdm_rx_bufs_free(tdm_rx2);
			if (tdm_rx3)
				tdm_rx_bufs_free(tdm_rx3);
		}

		cancel_work_sync(&tdm->tdm_reset_task);
#else /* else not TDM_V200 */
		tdm->stream_cnt--;
		if (tdm->stream_cnt == 0) {
			csic_tdm_int_disable(tdm->id, TDM_INT_ALL);
			csic_tdm_rx_cap_disable(tdm->id, tdm_rx->id);
			csic_tdm_tx_cap_disable(tdm->id);
			csic_tdm_top_disable(tdm->id);

			for (i = 0; i < TDM_RX_NUM; i++) {
				if (tdm->tdm_rx_buf_en[i] == 1)
					tdm_rx_bufs_free(&tdm->tdm_rx[i]);
				tdm->tdm_rx[i].stream_cnt = 0;
			}
			vin_log(VIN_LOG_TDM, "tdm%d close, closing the interrupt and tx/rx configuration!\n", tdm->id);
		} else
			vin_warn("TDM is used, cannot stream off!\n");
#endif
	}

	vin_log(VIN_LOG_FMT, "tdm_rx%d %s, %d*%d==%d*%d?\n",
			tdm_rx->id, enable ? "stream on" : "stream off",
			tdm_rx->width, tdm_rx->height,
			tdm_rx->format.width, tdm_rx->format.height);
	vin_log(VIN_LOG_LARGE, "tdm_rx%d %s, %d*%d\n",
			tdm_rx->id, enable ? "stream on" : "stream off",
			tdm_rx->width, tdm_rx->height);
	return 0;
}

static struct tdm_format *__tdm_try_format(struct v4l2_mbus_framefmt *mf)
{
	struct tdm_format *tdm_fmt = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_tdm_formats); i++)
		if (mf->code == sunxi_tdm_formats[i].code)
			tdm_fmt = &sunxi_tdm_formats[i];

	if (tdm_fmt == NULL)
		tdm_fmt = &sunxi_tdm_formats[0];

	mf->code = tdm_fmt->code;

	return tdm_fmt;
}

int sunxi_tdm_subdev_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct v4l2_captureparm *cp = &param->parm.capture;
	struct tdm_rx_dev *tdm_rx = v4l2_get_subdevdata(sd);

	tdm_rx->large_image = cp->reserved[2];
	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static int sunxi_tdm_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *state,
				    struct v4l2_subdev_format *fmt)
{
	struct tdm_rx_dev *tdm_rx = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = &tdm_rx->format;
	if (!mf)
		return -EINVAL;

	mutex_lock(&tdm_rx->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&tdm_rx->subdev_lock);
	return 0;
}

static int sunxi_tdm_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *state,
				    struct v4l2_subdev_format *fmt)
{
	struct tdm_rx_dev *tdm_rx = v4l2_get_subdevdata(sd);
	__maybe_unused struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	struct v4l2_mbus_framefmt *mf;
	__maybe_unused struct mbus_framefmt_res *res;
	struct tdm_format *tdm_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = &tdm_rx->format;

	if (fmt->pad == MIPI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&tdm_rx->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&tdm_rx->subdev_lock);
		}
		return 0;
	}

	tdm_fmt = __tdm_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&tdm_rx->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
			tdm_rx->tdm_fmt = tdm_fmt;
#ifdef TDM_V200
			res = (void *)mf->reserved;
			if (res->res_wdr_mode == ISP_DOL_WDR_MODE) {
				if (tdm_rx->id == 0)
					glb_tdm[tdm->id]->tdm_rx[1].tdm_fmt = tdm_fmt;
				else if (tdm_rx->id == 2)
					glb_tdm[tdm->id]->tdm_rx[3].tdm_fmt = tdm_fmt;
			} else if (res->res_wdr_mode == ISP_3FDOL_WDR_MODE) {
				if (tdm_rx->id == 0) {
					glb_tdm[tdm->id]->tdm_rx[1].tdm_fmt = tdm_fmt;
					glb_tdm[tdm->id]->tdm_rx[2].tdm_fmt = tdm_fmt;
				}
			}
#endif
		}
		mutex_unlock(&tdm_rx->subdev_lock);
	}

	return 0;
}

#else /* before linux-5.15 */
static int sunxi_tdm_subdev_get_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_pad_config *cfg,
				     struct v4l2_subdev_format *fmt)
{
	struct tdm_rx_dev *tdm_rx = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = &tdm_rx->format;
	if (!mf)
		return -EINVAL;

	mutex_lock(&tdm_rx->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&tdm_rx->subdev_lock);
	return 0;
}

static int sunxi_tdm_subdev_set_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_pad_config *cfg,
				     struct v4l2_subdev_format *fmt)
{
	struct tdm_rx_dev *tdm_rx = v4l2_get_subdevdata(sd);
	__maybe_unused struct tdm_dev *tdm = container_of(tdm_rx, struct tdm_dev, tdm_rx[tdm_rx->id]);
	struct v4l2_mbus_framefmt *mf;
	__maybe_unused struct mbus_framefmt_res *res;
	struct tdm_format *tdm_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = &tdm_rx->format;

	if (fmt->pad == MIPI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&tdm_rx->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&tdm_rx->subdev_lock);
		}
		return 0;
	}

	tdm_fmt = __tdm_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&tdm_rx->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE) {
			tdm_rx->tdm_fmt = tdm_fmt;
#ifdef TDM_V200
			res = (void *)mf->reserved;
			if (res->res_wdr_mode == ISP_DOL_WDR_MODE) {
				if (tdm_rx->id == 0)
					glb_tdm[tdm->id]->tdm_rx[1].tdm_fmt = tdm_fmt;
				else if (tdm_rx->id == 2)
					glb_tdm[tdm->id]->tdm_rx[3].tdm_fmt = tdm_fmt;
			} else if (res->res_wdr_mode == ISP_3FDOL_WDR_MODE) {
				if (tdm_rx->id == 0) {
					glb_tdm[tdm->id]->tdm_rx[1].tdm_fmt = tdm_fmt;
					glb_tdm[tdm->id]->tdm_rx[2].tdm_fmt = tdm_fmt;
				}
			}
#endif
		}
		mutex_unlock(&tdm_rx->subdev_lock);
	}

	return 0;
}
#endif

static const struct v4l2_subdev_video_ops sunxi_tdm_subdev_video_ops = {
	.s_stream = sunxi_tdm_subdev_s_stream,
};

static const struct v4l2_subdev_pad_ops sunxi_tdm_subdev_pad_ops = {
	.get_fmt = sunxi_tdm_subdev_get_fmt,
	.set_fmt = sunxi_tdm_subdev_set_fmt,
};

static struct v4l2_subdev_ops sunxi_tdm_subdev_ops = {
	.video = &sunxi_tdm_subdev_video_ops,
	.pad = &sunxi_tdm_subdev_pad_ops,
};

static int __tdm_init_subdev(struct tdm_rx_dev *tdm_rx)
{
	struct v4l2_subdev *sd = &tdm_rx->subdev;
	int ret;

	mutex_init(&tdm_rx->subdev_lock);
	v4l2_subdev_init(sd, &sunxi_tdm_subdev_ops);
	sd->grp_id = VIN_GRP_ID_TDM_RX;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_tdm_rx.%u", tdm_rx->id);
	v4l2_set_subdevdata(sd, tdm_rx);

	tdm_rx->tdm_pads[TDM_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	tdm_rx->tdm_pads[TDM_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_IO_V4L;

	ret = media_entity_pads_init(&sd->entity, TDM_PAD_NUM, tdm_rx->tdm_pads);
	if (ret < 0)
		return ret;

	return 0;
}

#ifdef TDM_V200
static void __sunxi_tdm_wdr_reset(struct tdm_dev *tdm)
{
#if 1
	struct vin_md *vind = dev_get_drvdata(tdm->tdm_rx[0].subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct csi_dev *csi = NULL;
	struct tdm_rx_dev *tdm_rx = NULL;
	struct isp_dev *isp = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	bool flags = 1;
	int i = 0;
	bool need_ve_callback = false;
	static bool line_num_set = 1;

	if (tdm->work_mode == TDM_OFFLINE)
		return;

	/*****************stop*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		tdm_rx = &tdm->tdm_rx[vind->vinc[i]->tdm_rx_sel];

		if (vind->vinc[i]->tdm_rx_sel == tdm_rx->id) {
			vinc = vind->vinc[i];
			vinc->vid_cap.frame_delay_cnt = 1;
			if (i == 0)
				need_ve_callback = true;

			if (flags) {
				isp = container_of(vinc->vid_cap.pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
				bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
#if IS_ENABLED(CONFIG_D3D)
				if ((isp->load_shadow[0x320 + 0x0]) & (1<<0)) {
					/* clear D3D rec_en */
					isp->load_shadow[0x320 + 0x0] = (isp->load_shadow[0x320 + 0x0]) & (~(1<<0));
					memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
				}
#endif
				csic_prs_capture_stop(vinc->csi_sel);

				csic_prs_disable(vinc->csi_sel);
				csic_isp_bridge_disable(0);

				csic_tdm_disable(0);
				csic_tdm_top_disable(0);

				bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
				bsp_isp_capture_stop(isp->id);
				bsp_isp_top_capture_stop(isp->id);
				bsp_isp_enable(isp->id, 0);

				flags = 0;
			}

			vipp_chn_cap_disable(vinc->vipp_sel);
			vipp_cap_disable(vinc->vipp_sel);
			vipp_clear_status(vinc->vipp_sel, VIPP_STATUS_ALL);
			vipp_top_clk_en(vinc->vipp_sel, 0);

			csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
			csic_dma_top_disable(vinc->vipp_sel);
		}
	}

	/*****************start*******************/
	flags = 1;
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		tdm_rx = &tdm->tdm_rx[vind->vinc[i]->tdm_rx_sel];

		if (vind->vinc[i]->tdm_rx_sel == tdm_rx->id) {
			vinc = vind->vinc[i];

			csic_dma_top_enable(vinc->vipp_sel);

			vipp_clear_status(vinc->vipp_sel, VIPP_STATUS_ALL);
			vipp_cap_enable(vinc->vipp_sel);
			vipp_top_clk_en(vinc->vipp_sel, 1);
			vipp_chn_cap_enable(vinc->vipp_sel);
			vinc->vin_status.frame_cnt = 0;
			vinc->vin_status.lost_cnt = 0;

			if (flags) {
				isp = container_of(vinc->vid_cap.pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
				bsp_isp_enable(isp->id, 1);
				bsp_isp_top_capture_start(isp->id);
				bsp_isp_set_para_ready(isp->id, PARA_READY);
				bsp_isp_capture_start(isp->id);
				isp->isp_frame_number = 0;

				csic_isp_bridge_enable(0);

				csic_tdm_top_enable(0);
				csic_tdm_enable(0);
				csic_tdm_tx_enable(0);
				csic_tdm_tx_cap_enable(0);
				csic_tdm_int_clear_status(0, TDM_INT_ALL);

				vin_print("%s:tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel);
				if (line_num_set) {
					csic_tdm_rx_set_line_num_ddr(tdm->id, tdm_rx->id, 1);//delayline > 64
					line_num_set = 0;
				} else {
					csic_tdm_rx_set_line_num_ddr(tdm->id, tdm_rx->id, 0);//delayline < 64
					line_num_set = 1;
				}
				csic_tdm_rx_enable(0, vinc->tdm_rx_sel);
				csic_tdm_rx_cap_enable(0, vinc->tdm_rx_sel);
				vin_print("%s:tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel + 1);
				csic_tdm_rx_enable(0, vinc->tdm_rx_sel + 1);
				csic_tdm_rx_cap_enable(0, vinc->tdm_rx_sel + 1);
				if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
					vin_print("%s:tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel + 2);
					csic_tdm_rx_enable(0, vinc->tdm_rx_sel + 2);
					csic_tdm_rx_cap_enable(0, vinc->tdm_rx_sel + 2);
				}
				csic_prs_enable(vinc->csi_sel);

				csi = container_of(vinc->vid_cap.pipe.sd[VIN_IND_CSI], struct csi_dev, subdev);
				csic_prs_capture_start(vinc->csi_sel, csi->bus_info.ch_total_num, &mode);
				flags = 0;
			}
		}
	}

	if (need_ve_callback) {
		vinc = vind->vinc[0];
		if (vinc->ve_online_cfg.ve_online_en && vinc->vid_cap.online_csi_reset_callback)
			vinc->vid_cap.online_csi_reset_callback(vinc->id);
	}
#endif
}

#if VIN_FALSE
static void __sunxi_tdm_reset_v2(struct tdm_dev *tdm)
{
	struct vin_md *vind = dev_get_drvdata(tdm->tdm_rx[0].subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct csi_dev *csi = NULL;
	struct tdm_rx_dev *tdm_rx = NULL;
	struct isp_dev *isp = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	bool flags = 1;
	int i = 0;
	bool need_ve_callback = false;

	/*****************stop*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;
		if (!tdm->tdm_rx_reset[vind->vinc[i]->tdm_rx_sel])
			continue;

		tdm_rx = &tdm->tdm_rx[vind->vinc[i]->tdm_rx_sel];

		if (vind->vinc[i]->tdm_rx_sel == tdm_rx->id) {
			vinc = vind->vinc[i];
			vinc->vid_cap.frame_delay_cnt = 1;
			if (i == 0)
				need_ve_callback = true;

			if (flags) {
				isp = container_of(vinc->vid_cap.pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
				bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
#if IS_ENABLED(CONFIG_D3D)
				bsp_isp_clr_d3d_rec_en(isp->id);
				isp->d3d_rec_reset = 1;
#endif
				csic_prs_capture_stop(vinc->csi_sel);

				csic_prs_disable(vinc->csi_sel);
				//csic_isp_bridge_disable(0);

				csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel);
				csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel);
				vin_print("%s:tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel);
				tdm_rx = &tdm->tdm_rx[vinc->tdm_rx_sel];
				if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
					csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 1);
					vin_print("%s:tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel + 1);
				} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
					csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 2);
					csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 2);
					vin_print("%s:tdm_rx%d && tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel + 1, vinc->tdm_rx_sel + 2);
				}

				bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
				bsp_isp_capture_stop(isp->id);

				flags = 0;
			}

			vipp_chn_cap_disable(vinc->vipp_sel);

			//csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
			//csic_dma_top_disable(vinc->vipp_sel);
		}
	}

	/*****************start*******************/
	flags = 1;
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;
		if (!tdm->tdm_rx_reset[vind->vinc[i]->tdm_rx_sel])
			continue;

		tdm_rx = &tdm->tdm_rx[vind->vinc[i]->tdm_rx_sel];

		if (vind->vinc[i]->tdm_rx_sel == tdm_rx->id) {
			vinc = vind->vinc[i];

			//csic_dma_top_enable(vinc->vipp_sel);

			vipp_chn_cap_enable(vinc->vipp_sel);
			vinc->vin_status.frame_cnt = 0;
			vinc->vin_status.lost_cnt = 0;

			if (flags) {
				isp = container_of(vinc->vid_cap.pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
				bsp_isp_set_para_ready(isp->id, PARA_READY);
				bsp_isp_capture_start(isp->id);
				isp->isp_frame_number = 0;

				//csic_isp_bridge_enable(0);

				csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel);
				csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel);
				tdm_rx = &tdm->tdm_rx[vinc->tdm_rx_sel];
				if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
					csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 1);
				} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
					csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 2);
					csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 2);
				}
				csic_prs_enable(vinc->csi_sel);

				csi = container_of(vinc->vid_cap.pipe.sd[VIN_IND_CSI], struct csi_dev, subdev);
				csic_prs_capture_start(vinc->csi_sel, csi->bus_info.ch_total_num, &mode);
				flags = 0;
			}
		}
	}

	if (vinc) {
		tdm->tdm_rx_reset[vinc->tdm_rx_sel] = 0;
		tdm_rx = &tdm->tdm_rx[vinc->tdm_rx_sel];
		if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 1] = 0;
		} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 1] = 0;
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 2] = 0;
		}
	}

	if (need_ve_callback) {
		vinc = vind->vinc[0];
		if (vinc->ve_online_cfg.ve_online_en && vinc->vid_cap.online_csi_reset_callback)
			vinc->vid_cap.online_csi_reset_callback(vinc->id);
	}
}
#endif

static void __sunxi_tdm_reset_v2_handle(struct work_struct *work)
{
	struct tdm_dev *tdm = container_of(work, struct tdm_dev, tdm_reset_task);
	struct vin_md *vind = dev_get_drvdata(tdm->tdm_rx[0].subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct csi_dev *csi = NULL;
	struct tdm_rx_dev *tdm_rx = NULL;
	struct isp_dev *isp = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	bool reset_flag = 1;
	int i = 0;
	bool need_ve_callback = false;
	unsigned long flags;
	struct prs_signal_status prs_sig_status;

	/*****************stop*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;
		if (!tdm->tdm_rx_reset[vind->vinc[i]->tdm_rx_sel])
			continue;

		tdm_rx = &tdm->tdm_rx[vind->vinc[i]->tdm_rx_sel];

		if (vind->vinc[i]->tdm_rx_sel == tdm_rx->id) {
			vinc = vind->vinc[i];
			vinc->vid_cap.frame_delay_cnt = 1;
			if (i == 0)
				need_ve_callback = true;

			if (reset_flag) {
				isp = container_of(vinc->vid_cap.pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
				csi = container_of(vinc->vid_cap.pipe.sd[VIN_IND_CSI], struct csi_dev, subdev);
				bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
#if IS_ENABLED(CONFIG_D3D)
				bsp_isp_clr_d3d_rec_en(isp->id);
				isp->d3d_rec_reset = 1;
#endif
				csic_prs_capture_stop(vinc->csi_sel);
				if (csi->bus_info.bus_if == V4L2_MBUS_CSI2_DPHY) {
					cmb_port_clr_ch0_int_status(vinc->mipi_sel);
					for (i = 0; i < 200; i++) {
						if (cmb_port_check_ch0_int_status(vinc->mipi_sel, CMB_MIPI_FRAME_END_SYNC_MASK))
							break;
						usleep_range(500, 510);
						if (i >= 199)
							vin_warn("%s parser%d wait:%d/2 ms to close!\n", __func__, csi->id, i);
					}
					vin_warn("%s parser%d wait:%d/2 ms to close!\n", __func__, csi->id, i);
				} else {
					for (i = 0; i < 200; i++) {
						memset(&prs_sig_status, 0, sizeof(struct prs_signal_status));
						csic_prs_signal_status(csi->id, &prs_sig_status);
						if (prs_sig_status.vsync_sta)
							break;
						usleep_range(500, 510);

						if (i >= 199)
							vin_warn("%s parser%d wait:%d/2 ms to close!\n", __func__, csi->id, i);
					}
				}
				csic_prs_disable(vinc->csi_sel);
				//csic_isp_bridge_disable(0);

				csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel);
				csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel);
				vin_print("%s:workqueue tdm_rx%d reset!!!, isp frame count is %d\n", __func__, vinc->tdm_rx_sel, isp->isp_frame_number);
				tdm_rx = &tdm->tdm_rx[vinc->tdm_rx_sel];
				if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
					csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 1);
					vin_print("%s:tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel + 1);
				} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
					csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 2);
					csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 2);
					vin_print("%s:tdm_rx%d && tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel + 1, vinc->tdm_rx_sel + 2);
				}

				bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
				bsp_isp_capture_stop(isp->id);

				reset_flag = 0;
			}

			vipp_chn_cap_disable(vinc->vipp_sel);

			//csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
			//csic_dma_top_disable(vinc->vipp_sel);
		}
	}

	usleep_range(5000, 5200);

	/*****************start*******************/
	reset_flag = 1;
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;
		if (!tdm->tdm_rx_reset[vind->vinc[i]->tdm_rx_sel])
			continue;

		tdm_rx = &tdm->tdm_rx[vind->vinc[i]->tdm_rx_sel];

		if (vind->vinc[i]->tdm_rx_sel == tdm_rx->id) {
			vinc = vind->vinc[i];

			//csic_dma_top_enable(vinc->vipp_sel);

			vipp_chn_cap_enable(vinc->vipp_sel);
			vinc->vin_status.frame_cnt = 0;
			vinc->vin_status.lost_cnt = 0;

			if (reset_flag) {
				isp = container_of(vinc->vid_cap.pipe.sd[VIN_IND_ISP], struct isp_dev, subdev);
				bsp_isp_set_para_ready(isp->id, PARA_READY);
				bsp_isp_capture_start(isp->id);
				isp->isp_frame_number = 0;

				//csic_isp_bridge_enable(0);

				csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel);
				csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel);
				tdm_rx = &tdm->tdm_rx[vinc->tdm_rx_sel];
				if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
					csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 1);
				} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
					csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 2);
					csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 2);
				}
				csic_prs_enable(vinc->csi_sel);

				csi = container_of(vinc->vid_cap.pipe.sd[VIN_IND_CSI], struct csi_dev, subdev);
				csic_prs_capture_start(vinc->csi_sel, csi->bus_info.ch_total_num, &mode);
				reset_flag = 0;
			}
		}
	}
	mutex_unlock(&vind->media_dev.graph_mutex);

	spin_lock_irqsave(&tdm->slock, flags);
	if (vinc) {
		tdm->tdm_rx_reset[vinc->tdm_rx_sel] = 0;
		tdm_rx = &tdm->tdm_rx[vinc->tdm_rx_sel];
		if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 1] = 0;
		} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 1] = 0;
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 2] = 0;
		}
	}

	if (tdm->work_mode == TDM_ONLINE && need_ve_callback) {
		vinc = vind->vinc[0];
		if (vinc->ve_online_cfg.ve_online_en && vinc->vid_cap.online_csi_reset_callback)
			vinc->vid_cap.online_csi_reset_callback(vinc->id);
	}
	spin_unlock_irqrestore(&tdm->slock, flags);
}
#endif

static void __sunxi_tdm_reset(struct tdm_dev *tdm)
{
#ifndef TDM_V200
	struct tdm_rx_dev *tdm_rx;
	struct vin_md *vind = dev_get_drvdata(tdm->tdm_rx[0].subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	bool flags = 1;
	int i, j, w;

	vin_print("%s:tdm%d reset!!!\n", __func__, tdm->id);
	/*****************stop*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		for (j = 0; j < TDM_RX_NUM; j++) {
			if (!tdm->tdm_rx[j].stream_cnt)
				continue;
			if (vind->vinc[i]->tdm_rx_sel == tdm->tdm_rx[j].id) {
				vinc = vind->vinc[i];
				tdm_rx = &tdm->tdm_rx[j];

				vin_print("video%d reset, frame count is %d\n", vinc->id, vinc->vin_status.frame_cnt);

				if (flags) {
					csic_prs_capture_stop(vinc->csi_sel);

					csic_tdm_int_clear_status(tdm->id, TDM_INT_ALL);
					csic_tdm_rx_cap_disable(tdm->id, tdm_rx->id);
					csic_tdm_rx_disable(tdm->id, tdm_rx->id);
					csic_tdm_tx_cap_disable(tdm->id);
					csic_tdm_disable(tdm->id);
					csic_tdm_top_disable(tdm->id);

					bsp_isp_clr_irq_status(vinc->isp_sel, ISP_IRQ_EN_ALL);
					for (w = 0; w < VIN_MAX_ISP; w++)
						bsp_isp_enable(w, 0);
					bsp_isp_capture_stop(vinc->isp_sel);
				}
				vipp_disable(vinc->vipp_sel);
				vipp_top_clk_en(vinc->vipp_sel, 0);
				csic_dma_int_clear_status(vinc->vipp_sel, DMA_INT_ALL);
				csic_dma_top_disable(vinc->vipp_sel);

				if (flags) {
					csic_prs_disable(vinc->csi_sel);
					csic_isp_bridge_disable(0);
				}
			}
		}
	}

	/*****************start*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		for (j = 0; j < TDM_RX_NUM; j++) {
			if (!tdm->tdm_rx[j].stream_cnt)
				continue;
			if (vind->vinc[i]->tdm_rx_sel == tdm->tdm_rx[j].id) {
				vinc = vind->vinc[i];
				tdm_rx = &tdm->tdm_rx[j];

				csic_dma_top_enable(vinc->vipp_sel);
				vipp_top_clk_en(vinc->vipp_sel, 1);
				vipp_enable(vinc->vipp_sel);
				vinc->vin_status.frame_cnt = 0;
				vinc->vin_status.lost_cnt = 0;

				if (flags) {
					for (w = 0; w < VIN_MAX_ISP; w++)
						bsp_isp_enable(w, 1);
					bsp_isp_capture_start(vinc->isp_sel);

					csic_isp_bridge_enable(0);

					csic_tdm_top_enable(tdm->id);
					csic_tdm_enable(tdm->id);
					csic_tdm_tx_cap_enable(tdm->id);
					csic_tdm_rx_enable(tdm->id, tdm_rx->id);
					csic_tdm_rx_cap_enable(tdm->id, tdm_rx->id);

					csic_prs_enable(vinc->csi_sel);
					csic_prs_capture_start(vinc->csi_sel, 1, &mode);
				}
			}
		}
	}

#else /* else TDM_V200 */
#if VIN_FALSE
	struct tdm_rx_dev *tdm_rx;
	struct vin_md *vind = dev_get_drvdata(tdm->tdm_rx[0].subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct csi_dev *csi = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	int i;

	if (tdm->work_mode == TDM_ONLINE)
		return;

	/*****************stop*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;
		if (!tdm->tdm_rx_reset[vind->vinc[i]->tdm_rx_sel])
			continue;

		vinc = vind->vinc[i];

		csic_prs_capture_stop(vinc->csi_sel);
		csic_prs_disable(vinc->csi_sel);

		csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel);
		csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel);
		vin_print("%s:tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel);
		tdm_rx = &tdm->tdm_rx[vinc->tdm_rx_sel];
		if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
			csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 1);
			csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 1);
			vin_print("%s:tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel + 1);
		} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
			csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 1);
			csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 1);
			csic_tdm_rx_disable(tdm->id, vinc->tdm_rx_sel + 2);
			csic_tdm_rx_cap_disable(tdm->id, vinc->tdm_rx_sel + 2);
			vin_print("%s:tdm_rx%d && tdm_rx%d reset!!!\n", __func__, vinc->tdm_rx_sel + 1, vinc->tdm_rx_sel + 2);
		}
		break;
	}

	/*****************start*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;
		if (!tdm->tdm_rx_reset[vind->vinc[i]->tdm_rx_sel])
			continue;

		vinc = vind->vinc[i];

		tdm->tdm_rx_reset[vinc->tdm_rx_sel] = 0;
		csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel);
		csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel);
		tdm_rx = &tdm->tdm_rx[vinc->tdm_rx_sel];
		if (tdm_rx->ws.wdr_mode == ISP_DOL_WDR_MODE) {
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 1] = 0;
			csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 1);
			csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 1);
		} else if (tdm_rx->ws.wdr_mode == ISP_3FDOL_WDR_MODE) {
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 1] = 0;
			tdm->tdm_rx_reset[vinc->tdm_rx_sel + 2] = 0;
			csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 1);
			csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 1);
			csic_tdm_rx_enable(tdm->id, vinc->tdm_rx_sel + 2);
			csic_tdm_rx_cap_enable(tdm->id, vinc->tdm_rx_sel + 2);
		}

		csic_prs_enable(vinc->csi_sel);
		csi = container_of(vinc->vid_cap.pipe.sd[VIN_IND_CSI], struct csi_dev, subdev);
		csic_prs_capture_start(vinc->csi_sel, csi->bus_info.ch_total_num, &mode);
		break;
	}
#endif
#endif
}

static void tdm_rx_set_reset(struct tdm_dev *tdm, unsigned int rx_id)
{
#ifdef TDM_V200
	if (tdm->work_mode == TDM_ONLINE)
		return;
	if (rx_id == 0) {
		tdm->tdm_rx_reset[0] = 1;
	} else if (rx_id == 1) {
		if (tdm->tdm_rx[0].ws.wdr_mode == ISP_DOL_WDR_MODE || tdm->tdm_rx[0].ws.wdr_mode == ISP_3FDOL_WDR_MODE)
			tdm->tdm_rx_reset[0] = 1;
		else
			tdm->tdm_rx_reset[1] = 1;

	} else if (rx_id == 2) {
		if (tdm->tdm_rx[0].ws.wdr_mode == ISP_3FDOL_WDR_MODE)
			tdm->tdm_rx_reset[0] = 1;
		else
			tdm->tdm_rx_reset[2] = 1;
	} else if (rx_id == 3) {
		if (tdm->tdm_rx[2].ws.wdr_mode == ISP_DOL_WDR_MODE)
			tdm->tdm_rx_reset[2] = 1;
		else
			tdm->tdm_rx_reset[3] = 1;
	}
#endif
}

static irqreturn_t tdm_isr(int irq, void *priv)
{
	struct tdm_dev *tdm = (struct tdm_dev *)priv;
	struct tdm_int_status status;
	__maybe_unused unsigned int hb_min = 0xffff, hb_max = 0;
	unsigned int width = 0, height = 0;
	unsigned long flags;

	if (tdm->stream_cnt == 0) {
		csic_tdm_int_clear_status(tdm->id, TDM_INT_ALL);
		return IRQ_HANDLED;
	}

	memset(&status, 0, sizeof(struct tdm_int_status));
	csic_tdm_int_get_status(tdm->id, &status);

	spin_lock_irqsave(&tdm->slock, flags);

	if (status.rx_frm_lost) {
		csic_tdm_int_clear_status(tdm->id, RX_FRM_LOST_INT_EN);
		if (csic_tdm_internal_get_status0(tdm->id, RX0_FRM_LOST_PD)) {
			vin_err("tdm%d rx0 frame lost!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX0_FRM_LOST_PD);
			tdm_rx_set_reset(tdm, 0);
		}
		if (csic_tdm_internal_get_status0(tdm->id, RX1_FRM_LOST_PD)) {
			vin_err("tdm%d rx1 frame lost!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX1_FRM_LOST_PD);
			tdm_rx_set_reset(tdm, 1);
		}
#ifdef TDM_V200
		if (csic_tdm_internal_get_status0(tdm->id, RX2_FRM_LOST_PD)) {
			vin_err("tdm%d rx2 frame lost!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX2_FRM_LOST_PD);
			tdm_rx_set_reset(tdm, 2);
		}
		if (csic_tdm_internal_get_status0(tdm->id, RX3_FRM_LOST_PD)) {
			vin_err("tdm%d rx2 frame lost!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX3_FRM_LOST_PD);
			tdm_rx_set_reset(tdm, 3);
		}
		schedule_work(&tdm->tdm_reset_task);
#endif
		__sunxi_tdm_reset(tdm);
	}

	if (status.rx_frm_err) {
		csic_tdm_int_clear_status(tdm->id, RX_FRM_ERR_INT_EN);
		if (csic_tdm_internal_get_status0(tdm->id, RX0_FRM_ERR_PD)) {
			csic_tdm_rx_get_size(tdm->id, 0, &width, &height);
			vin_err("tdm%d rx0 frame error! width is %d, height is %d\n", tdm->id, width, height);
			csic_tdm_internal_clear_status0(tdm->id, RX0_FRM_ERR_PD);
			tdm_rx_set_reset(tdm, 0);
		}
		if (csic_tdm_internal_get_status0(tdm->id, RX1_FRM_ERR_PD)) {
			csic_tdm_rx_get_size(tdm->id, 1, &width, &height);
			vin_err("tdm%d rx1 frame error! width is %d, height is %d\n", tdm->id, width, height);
			csic_tdm_internal_clear_status0(tdm->id, RX1_FRM_ERR_PD);
			tdm_rx_set_reset(tdm, 1);
		}
#ifdef TDM_V200
		if (csic_tdm_internal_get_status0(tdm->id, RX2_FRM_ERR_PD)) {
			csic_tdm_rx_get_size(tdm->id, 0, &width, &height);
			vin_err("tdm%d rx2 frame error! width is %d, height is %d\n", tdm->id, width, height);
			csic_tdm_internal_clear_status0(tdm->id, RX2_FRM_ERR_PD);
			tdm_rx_set_reset(tdm, 2);
		}
		if (csic_tdm_internal_get_status0(tdm->id, RX3_FRM_ERR_PD)) {
			csic_tdm_rx_get_size(tdm->id, 1, &width, &height);
			vin_err("tdm%d rx3 frame error! width is %d, height is %d\n", tdm->id, width, height);
			csic_tdm_internal_clear_status0(tdm->id, RX3_FRM_ERR_PD);
			tdm_rx_set_reset(tdm, 3);
		}
		schedule_work(&tdm->tdm_reset_task);
#endif
		__sunxi_tdm_reset(tdm);
	}

	if (status.rx_btype_err) {
		csic_tdm_int_clear_status(tdm->id, RX_BTYPE_ERR_INT_EN);
		if (csic_tdm_internal_get_status0(tdm->id, RX0_BTYPE_ERR_PD)) {
			vin_err("tdm%d rx0 btype error!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX0_BTYPE_ERR_PD);
			tdm_rx_set_reset(tdm, 0);
		}
		if (csic_tdm_internal_get_status0(tdm->id, RX1_BTYPE_ERR_PD)) {
			vin_err("tdm%d rx1 btype error!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX1_BTYPE_ERR_PD);
			tdm_rx_set_reset(tdm, 1);
		}
#ifdef TDM_V200
		if (csic_tdm_internal_get_status0(tdm->id, RX2_BTYPE_ERR_PD)) {
			vin_err("tdm%d rx2 btype error!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX2_BTYPE_ERR_PD);
			tdm_rx_set_reset(tdm, 2);
		}
		if (csic_tdm_internal_get_status0(tdm->id, RX3_BTYPE_ERR_PD)) {
			vin_err("tdm%d rx3 btype error!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX3_BTYPE_ERR_PD);
			tdm_rx_set_reset(tdm, 3);
		}
		schedule_work(&tdm->tdm_reset_task);
#endif
		__sunxi_tdm_reset(tdm);
	}

	if (status.rx_buf_full) {
		csic_tdm_int_clear_status(tdm->id, RX_BUF_FULL_INT_EN);
		if (csic_tdm_internal_get_status0(tdm->id, RX0_BUF_FULL_PD)) {
			vin_err("tdm%d rx0 buffer full!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX0_BUF_FULL_PD);
			tdm_rx_set_reset(tdm, 0);
		}
		if (csic_tdm_internal_get_status0(tdm->id, RX1_BUF_FULL_PD)) {
			vin_err("tdm%d rx1 buffer full!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX1_BUF_FULL_PD);
			tdm_rx_set_reset(tdm, 1);
		}
#ifdef TDM_V200
		if (csic_tdm_internal_get_status0(tdm->id, RX2_BUF_FULL_PD)) {
			vin_err("tdm%d rx2 buffer full!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX2_BUF_FULL_PD);
			tdm_rx_set_reset(tdm, 2);
		}
		if (csic_tdm_internal_get_status0(tdm->id, RX3_BUF_FULL_PD)) {
			vin_err("tdm%d rx3 buffer full!\n", tdm->id);
			csic_tdm_internal_clear_status0(tdm->id, RX3_BUF_FULL_PD);
			tdm_rx_set_reset(tdm, 3);
		}
		schedule_work(&tdm->tdm_reset_task);
#endif
		__sunxi_tdm_reset(tdm);
	}

#ifndef TDM_V200
	if (status.rx_comp_err) {
		csic_tdm_int_clear_status(tdm->id, RX_COMP_ERR_INT_EN);
		if (csic_tdm_internal_get_status1(tdm->id, RX0_COMP_ERR_PD)) {
			vin_err("tdm%d rx0 compose error!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX0_COMP_ERR_PD);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX1_COMP_ERR_PD)) {
			vin_err("tdm%d rx1 compose error!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX1_COMP_ERR_PD);
		}
		__sunxi_tdm_reset(tdm);
	}
#endif

	if (status.rx_hb_short) {
		csic_tdm_int_clear_status(tdm->id, RX_HB_SHORT_INT_EN);
		if (csic_tdm_internal_get_status1(tdm->id, RX0_HB_SHORT_PD)) {
			csic_tdm_rx_get_hblank(tdm->id, 0, &hb_min, &hb_max);
			vin_err("tdm%d rx0 hblank short! min is %d, max is %d\n", tdm->id, hb_min, hb_max);
			csic_tdm_internal_clear_status1(tdm->id, RX0_HB_SHORT_PD);
			tdm_rx_set_reset(tdm, 0);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX1_HB_SHORT_PD)) {
			csic_tdm_rx_get_hblank(tdm->id, 1, &hb_min, &hb_max);
			vin_err("tdm%d rx1 hblank short! min is %d, max is %d\n", tdm->id, hb_min, hb_max);
			csic_tdm_internal_clear_status1(tdm->id, RX1_HB_SHORT_PD);
			tdm_rx_set_reset(tdm, 1);
		}
#ifdef TDM_V200
		if (csic_tdm_internal_get_status1(tdm->id, RX2_HB_SHORT_PD)) {
			csic_tdm_rx_get_hblank(tdm->id, 2, &hb_min, &hb_max);
			vin_err("tdm%d rx2 hblank short! min is %d, max is %d\n", tdm->id, hb_min, hb_max);
			csic_tdm_internal_clear_status1(tdm->id, RX2_HB_SHORT_PD);
			tdm_rx_set_reset(tdm, 2);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX3_HB_SHORT_PD)) {
			csic_tdm_rx_get_hblank(tdm->id, 3, &hb_min, &hb_max);
			vin_err("tdm%d rx3 hblank short! min is %d, max is %d\n", tdm->id, hb_min, hb_max);
			csic_tdm_internal_clear_status1(tdm->id, RX3_HB_SHORT_PD);
			tdm_rx_set_reset(tdm, 3);
		}
		schedule_work(&tdm->tdm_reset_task);
#endif
		__sunxi_tdm_reset(tdm);
	}

	if (status.rx_fifo_full) {
		csic_tdm_int_clear_status(tdm->id, RX_FIFO_FULL_INT_EN);
		if (csic_tdm_internal_get_status1(tdm->id, RX0_FIFO_FULL_PD)) {
			vin_err("tdm%d rx0 write DMA fifo overflow!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX0_FIFO_FULL_PD);
			tdm_rx_set_reset(tdm, 0);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX1_FIFO_FULL_PD)) {
			vin_err("tdm%d rx1 write DMA fifo overflow!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX1_FIFO_FULL_PD);
			tdm_rx_set_reset(tdm, 1);
		}
#ifdef TDM_V200
		if (csic_tdm_internal_get_status1(tdm->id, RX2_FIFO_FULL_PD)) {
			vin_err("tdm%d rx2 write DMA fifo overflow!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX2_FIFO_FULL_PD);
			tdm_rx_set_reset(tdm, 2);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX3_FIFO_FULL_PD)) {
			vin_err("tdm%d rx3 write DMA fifo overflow!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX3_FIFO_FULL_PD);
			tdm_rx_set_reset(tdm, 3);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX0_HEAD_FIFO_FULL_0)) {
			vin_err("tdm%d rx0 write line number fifo overflow from bandwidth!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX0_HEAD_FIFO_FULL_0);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX0_HEAD_FIFO_FULL_1)) {
			vin_err("tdm%d rx0 write line number fifo overflow from para_cfg!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX0_HEAD_FIFO_FULL_1);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX1_HEAD_FIFO_FULL_0)) {
			vin_err("tdm%d rx1 write line number fifo overflow from bandwidth!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX1_HEAD_FIFO_FULL_0);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX1_HEAD_FIFO_FULL_1)) {
			vin_err("tdm%d rx1 write line number fifo overflow from para_cfg!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX1_HEAD_FIFO_FULL_1);
		}
		schedule_work(&tdm->tdm_reset_task);
#endif
		__sunxi_tdm_reset(tdm);
	}

#ifdef TDM_V200

	if (status.rx0_frm_done) {
		csic_tdm_int_clear_status(tdm->id, RX0_FRM_DONE_INT_EN);
		vin_log(VIN_LOG_TDM, "tdm%d rx0 frame done!\n", tdm->id);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		tdm->rx_stream_cnt[0]++;
		if (tdm->rx_stream_cnt[0] > 100) {
			csic_tdm_int_disable(tdm->id, RX0_FRM_DONE_INT_EN);
			csic_tdm_int_enable(tdm->id, SPEED_DN_FIFO_FULL_INT_EN | SPEED_DN_FIFO_FULL_INT_EN);
		}
#endif
	}
	if (status.rx1_frm_done) {
		csic_tdm_int_clear_status(tdm->id, RX1_FRM_DONE_INT_EN);
		vin_log(VIN_LOG_TDM, "tdm%d rx1 frame done!\n", tdm->id);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		tdm->rx_stream_cnt[1]++;
		if (tdm->rx_stream_cnt[1] > 100) {
			csic_tdm_int_disable(tdm->id, RX1_FRM_DONE_INT_EN);
			csic_tdm_int_enable(tdm->id, SPEED_DN_FIFO_FULL_INT_EN | SPEED_DN_FIFO_FULL_INT_EN);
		}
#endif
	}
	if (status.rx2_frm_done) {
		csic_tdm_int_clear_status(tdm->id, RX2_FRM_DONE_INT_EN);
		vin_log(VIN_LOG_TDM, "tdm%d rx2 frame done!\n", tdm->id);
	}
	if (status.rx3_frm_done) {
		csic_tdm_int_clear_status(tdm->id, RX3_FRM_DONE_INT_EN);
		vin_log(VIN_LOG_TDM, "tdm%d rx3 frame done!\n", tdm->id);
	}
	if (status.tx_frm_done) {
		csic_tdm_int_clear_status(tdm->id, TX_FRM_DONE_INT_EN);
		vin_log(VIN_LOG_TDM, "tdm%d tx frame done!\n", tdm->id);
	}
	if (status.rx_chn_cfg_mode) {
		csic_tdm_int_clear_status(tdm->id, RX_CHN_CFG_MODE_INT_EN);
		vin_log(VIN_LOG_TDM, "tdm%d rx chn cfg!\n", tdm->id);
	}
	if (status.tx_chn_cfg_mode) {
		csic_tdm_int_clear_status(tdm->id, TX_CHN_CFG_MODE_INT_EN);
		vin_log(VIN_LOG_TDM, "tdm%d tx chn cfg!\n", tdm->id);
	}

	if (status.speed_dn_hsync) {
		csic_tdm_int_clear_status(tdm->id, SPEED_DN_HSYNC_INT_EN);
		vin_err("tdm%d speed dn hsync!\n", tdm->id);
	}

	if (status.speed_dn_fifo_full) {
		csic_tdm_int_clear_status(tdm->id, SPEED_DN_FIFO_FULL_INT_EN);
		vin_err("tdm%d speed dn fifo full!\n", tdm->id);
	}

	if (status.tx_fifo_under) {
		csic_tdm_int_clear_status(tdm->id, TDM_FIFO_UNDER_INT_EN);
		if (csic_tdm_internal_get_status1(tdm->id, TX0_FIFO_UNDER)) {
			vin_err("tdm%d tx0 fifo under!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, TX0_FIFO_UNDER);
		}
		if (csic_tdm_internal_get_status1(tdm->id, TX1_FIFO_UNDER)) {
			vin_err("tdm%d tx1 fifo under!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, TX1_FIFO_UNDER);
		}
		if (csic_tdm_internal_get_status1(tdm->id, TX2_FIFO_UNDER)) {
			vin_err("tdm%d tx2 fifo under!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, TX2_FIFO_UNDER);
		}
		__sunxi_tdm_wdr_reset(tdm);
	}

	if (status.tdm_lbc_err) {
		csic_tdm_int_clear_status(tdm->id, TDM_LBC_ERR_INT_EN);
		if (csic_tdm_internal_get_status1(tdm->id, LBC0_ERROR)) {
			vin_err("tdm%d rx0 lbc error!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, LBC0_ERROR);
			tdm_rx_set_reset(tdm, 0);
		}
		if (csic_tdm_internal_get_status1(tdm->id, LBC1_ERROR)) {
			vin_err("tdm%d rx1 lbc error!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, LBC1_ERROR);
			tdm_rx_set_reset(tdm, 1);
		}
		schedule_work(&tdm->tdm_reset_task);
	}

	if (status.tdm_lbc_fifo_full) {
		csic_tdm_int_clear_status(tdm->id, RDM_LBC_FIFO_FULL_INT_EN);
		vin_err("tdm%d lbc fifo overflow!\n", tdm->id);
		schedule_work(&tdm->tdm_reset_task);
	}
#else
	if (status.rx_comp_err) {
		csic_tdm_int_clear_status(tdm->id, RX_COMP_ERR_INT_EN);
		if (csic_tdm_internal_get_status1(tdm->id, RX0_COMP_ERR_PD)) {
			vin_err("tdm%d rx0 compose error!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX0_COMP_ERR_PD);
		}
		if (csic_tdm_internal_get_status1(tdm->id, RX1_COMP_ERR_PD)) {
			vin_err("tdm%d rx1 compose error!\n", tdm->id);
			csic_tdm_internal_clear_status1(tdm->id, RX1_COMP_ERR_PD);
		}
		__sunxi_tdm_reset(tdm);
	}
#endif
	spin_unlock_irqrestore(&tdm->slock, flags);

	return IRQ_HANDLED;
}

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
static int tdm_enable_irq(void *dev, void *data, int len)
{
	struct tdm_dev *tdm = dev;
	int ret = 0;

	if (!data || !strncmp(data, "return", len)) {
		ret = request_irq(tdm->irq, tdm_isr, IRQF_SHARED, tdm->pdev->name, tdm);
		if (ret)
			vin_err("tdm%d request tdm failed\n", tdm->id);
	} else if (!strncmp(data, "get", len)) {
		if (tdm->irq)
			free_irq(tdm->irq, tdm);
	}

	return ret;
}
#endif

static int tdm_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct tdm_dev *tdm = NULL;
	unsigned int i;
	int ret = 0;

	if (np == NULL) {
		vin_err("TDM failed to get of node\n");
		return -ENODEV;
	}

	tdm = kzalloc(sizeof(*tdm), GFP_KERNEL);
	if (!tdm) {
		ret = -ENOMEM;
		goto ekzalloc;
	}
	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("TDM failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}

	tdm->id = pdev->id;
	tdm->pdev = pdev;
	tdm->stream_cnt = 0;

#ifdef TDM_V200
	tdm->work_mode = 0xff;
	of_property_read_u32(np, "work_mode", &tdm->work_mode);
	tdm->work_mode = clamp_t(unsigned int, tdm->work_mode, TDM_ONLINE, TDM_OFFLINE);
	vin_log(VIN_LOG_VIDEO, "tdm%d work mode is %d\n", tdm->id, tdm->work_mode);
	INIT_LIST_HEAD(&tdm->working_chn_fmt);

	INIT_WORK(&tdm->tdm_reset_task, __sunxi_tdm_reset_v2_handle);
#endif
	tdm->base = of_iomap(np, 0);
	if (!tdm->base) {
		tdm->is_empty = 1;
	} else {
		tdm->is_empty = 0;
		/* get irq resource */
		tdm->irq = irq_of_parse_and_map(np, 0);
		if (tdm->irq <= 0) {
			vin_err("failed to get TDM IRQ resource\n");
			goto unmap;
		}
#if !defined CONFIG_VIN_INIT_MELIS
		ret = request_irq(tdm->irq, tdm_isr, IRQF_SHARED, tdm->pdev->name, tdm);
		if (ret) {
			vin_err("tdm%d request tdm failed\n", tdm->id);
			goto unmap;
		}
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		vin_iommu_en(ISP_IOMMU_MASTER, true);
#endif
#else
		of_property_read_u32(np, "delay_init", &tdm->delay_init);
		if (tdm->delay_init == 0) {
			ret = request_irq(tdm->irq, tdm_isr, IRQF_SHARED, tdm->pdev->name, tdm);
			if (ret) {
				vin_err("tdm%d request irq failed\n", tdm->id);
				goto unmap;
			}
		} else {
			sprintf(name, "tdm%d", tdm->id);
			rpmsg_notify_add("7130000.e906_rproc", name, tdm_enable_irq, tdm);
		}
#endif
	}

	spin_lock_init(&tdm->slock);

	csic_tdm_set_base_addr(tdm->id, (unsigned long)tdm->base);

	for (i = 0; i < TDM_RX_NUM; i++) {
		tdm->tdm_rx[i].id = i;
		ret = __tdm_init_subdev(&tdm->tdm_rx[i]);
		if (ret < 0) {
			vin_err("tdm_rx%d init error!\n", i);
			goto unmap;
		}
		tdm->tdm_rx[i].stream_cnt = 0;
		tdm->tdm_rx_buf_en[i] = 0;
	}

	platform_set_drvdata(pdev, tdm);
	glb_tdm[tdm->id] = tdm;

	vin_log(VIN_LOG_TDM, "tdm%d probe end!\n", tdm->id);
	return 0;

unmap:
	iounmap(tdm->base);
freedev:
	kfree(tdm);
ekzalloc:
	vin_err("tdm probe err!\n");
	return ret;
}

static int tdm_remove(struct platform_device *pdev)
{
	struct tdm_dev *tdm = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd;
	unsigned int i;

	platform_set_drvdata(pdev, NULL);

	if (!tdm->is_empty) {
		free_irq(tdm->irq, tdm);
		if (tdm->base)
			iounmap(tdm->base);
	}

	for (i = 0; i < TDM_RX_NUM; i++) {
		sd = &tdm->tdm_rx[i].subdev;
		v4l2_set_subdevdata(sd, NULL);
		media_entity_cleanup(&tdm->tdm_rx[i].subdev.entity);
	}

	kfree(tdm);
	return 0;
}

static const struct of_device_id sunxi_tdm_match[] = {
	{.compatible = "allwinner,sunxi-tdm",},
	{},
};

static struct platform_driver tdm_platform_driver = {
	.probe = tdm_probe,
	.remove = tdm_remove,
	.driver = {
		.name = TDM_MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_tdm_match,
	}
};

void sunxi_tdm_fps_clk(struct v4l2_subdev *sd, int fps, unsigned int isp_clk, unsigned int vts)
{
	struct tdm_rx_dev *tdm_rx = v4l2_get_subdevdata(sd);

	tdm_rx->sensor_fps = fps;
	tdm_rx->isp_clk = roundup(isp_clk, 1000000);
	tdm_rx->vts = vts;
}

void sunxi_tdm_buffer_free(unsigned int id)
{
	struct tdm_rx_dev *tdm_rx = NULL;
	unsigned int i;

	usleep_range(1000, 1100);

	for (i = 0; i < TDM_RX_NUM; i++) {
		tdm_rx = &glb_tdm[id]->tdm_rx[i];
		tdm_rx_bufs_free(tdm_rx);
	}
}

struct v4l2_subdev *sunxi_tdm_get_subdev(int id, int tdm_rx_num)
{
	if (id < VIN_MAX_TDM && glb_tdm[id])
		return &glb_tdm[id]->tdm_rx[tdm_rx_num].subdev;
	else
		return NULL;
}

int sunxi_tdm_platform_register(void)
{
	return platform_driver_register(&tdm_platform_driver);
}

void sunxi_tdm_platform_unregister(void)
{
	platform_driver_unregister(&tdm_platform_driver);
	vin_log(VIN_LOG_TDM, "tdm_exit end\n");
}
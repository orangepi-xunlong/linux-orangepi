/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
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
#include <linux/rpbuf.h>
#include <linux/aw_rpmsg.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include <linux/memblock.h>
#include "../platform/platform_cfg.h"
#include "sunxi_isp.h"

#include "../vin-csi/sunxi_csi.h"
#include "../vin-vipp/sunxi_scaler.h"
#include "../vin-video/vin_core.h"
#include "../utility/vin_io.h"
#include "isp_default_tbl.h"

#define ISP_MODULE_NAME "vin_isp"

struct isp_dev *glb_isp[VIN_MAX_ISP];

#if !defined ISP_600
#if IS_ENABLED(CONFIG_D3D_COMPRESS_EN)
#define D3D_RAW_LBC_MODE		16 /* <=10 = lossless, 12 = 1.2x, 30 = 3x(< 3x)*/
#define D3D_K_LBC_MODE			20 /* <=10 = lossless, 12 = 1.2x, 20 = 2x(< 2x)*/
#else
#define D3D_RAW_LBC_MODE		10 /* <=10 = lossless, 12 = 1.2x, 30 = 3x(< 3x)*/
#define D3D_K_LBC_MODE			10 /* <=10 = lossless, 12 = 1.2x, 20 = 2x(< 2x)*/
#endif

#if IS_ENABLED(CONFIG_WDR_COMPRESS_EN)
#define WDR_RAW_LBC_MODE		26 /* <=10 = lossless, 12 = 1.2x, 30 = 3x(< 3x)*/
#else
#define WDR_RAW_LBC_MODE		10 /* <=10 = lossless, 12 = 1.2x, 30 = 3x(< 3x)*/
#endif
#else /* else ISP_600 */
#if IS_ENABLED(CONFIG_D3D_COMPRESS_EN)
#define D3D_LBC_MODE			150//167 /* <=100 = lossless, 120 = 1.2x, 400 = 4x(< 4x)*/
#else
#define D3D_LBC_MODE			100 /* <=100 = lossless, 120 = 1.2x, 400 = 4x(< 4x)*/
#endif
#endif

#define LARGE_IMAGE_OFF			32

#define MIN_IN_WIDTH			192
#define MIN_IN_HEIGHT			128
#define MAX_IN_WIDTH			4224
#define MAX_IN_HEIGHT			4224

static struct isp_pix_fmt sunxi_isp_formats[] = {
	{
		.fourcc = V4L2_PIX_FMT_SBGGR8,
		.mbus_code = MEDIA_BUS_FMT_SBGGR8_1X8,
		.infmt = ISP_BGGR,
		.input_bit = RAW_8,
	}, {
		.fourcc = V4L2_PIX_FMT_SGBRG8,
		.mbus_code = MEDIA_BUS_FMT_SGBRG8_1X8,
		.infmt = ISP_GBRG,
		.input_bit = RAW_8,
	}, {
		.fourcc = V4L2_PIX_FMT_SGRBG8,
		.mbus_code = MEDIA_BUS_FMT_SGRBG8_1X8,
		.infmt = ISP_GRBG,
		.input_bit = RAW_8,
	}, {
		.fourcc = V4L2_PIX_FMT_SRGGB8,
		.mbus_code = MEDIA_BUS_FMT_SRGGB8_1X8,
		.infmt = ISP_RGGB,
		.input_bit = RAW_8,
	}, {
		.fourcc = V4L2_PIX_FMT_SBGGR10,
		.mbus_code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.infmt = ISP_BGGR,
		.input_bit = RAW_10,
	}, {
		.fourcc = V4L2_PIX_FMT_SGBRG8,
		.mbus_code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.infmt = ISP_GBRG,
		.input_bit = RAW_10,
	}, {
		.fourcc = V4L2_PIX_FMT_SGRBG10,
		.mbus_code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.infmt = ISP_GRBG,
		.input_bit = RAW_10,
	}, {
		.fourcc = V4L2_PIX_FMT_SRGGB10,
		.mbus_code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.infmt = ISP_RGGB,
		.input_bit = RAW_10,
	}, {
		.fourcc = V4L2_PIX_FMT_SBGGR12,
		.mbus_code = MEDIA_BUS_FMT_SBGGR12_1X12,
		.infmt = ISP_BGGR,
		.input_bit = RAW_12,
	}, {
		.fourcc = V4L2_PIX_FMT_SGBRG12,
		.mbus_code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.infmt = ISP_GBRG,
		.input_bit = RAW_12,
	}, {
		.fourcc = V4L2_PIX_FMT_SGRBG12,
		.mbus_code = MEDIA_BUS_FMT_SGRBG12_1X12,
		.infmt = ISP_GRBG,
		.input_bit = RAW_12,
	}, {
		.fourcc = V4L2_PIX_FMT_SRGGB12,
		.mbus_code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.infmt = ISP_RGGB,
		.input_bit = RAW_12,
	}, {
		.fourcc = V4L2_PIX_FMT_SBGGR14,
		.mbus_code = MEDIA_BUS_FMT_SBGGR14_1X14,
		.infmt = ISP_BGGR,
		.input_bit = RAW_14,
		.byr_max_bit = BYR_RAW_14,
	}, {
		.fourcc = V4L2_PIX_FMT_SGBRG14,
		.mbus_code = MEDIA_BUS_FMT_SGBRG14_1X14,
		.infmt = ISP_GBRG,
		.input_bit = RAW_14,
		.byr_max_bit = BYR_RAW_14,
	}, {
		.fourcc = V4L2_PIX_FMT_SGRBG14,
		.mbus_code = MEDIA_BUS_FMT_SGRBG14_1X14,
		.infmt = ISP_GRBG,
		.input_bit = RAW_14,
		.byr_max_bit = BYR_RAW_14,
	}, {
		.fourcc = V4L2_PIX_FMT_SRGGB14,
		.mbus_code = MEDIA_BUS_FMT_SRGGB14_1X14,
		.infmt = ISP_RGGB,
		.input_bit = RAW_14,
		.byr_max_bit = BYR_RAW_14,
	}, {
		.fourcc = V4L2_PIX_FMT_SBGGR16,
		.mbus_code = MEDIA_BUS_FMT_SBGGR16_1X16,
		.infmt = ISP_BGGR,
		.input_bit = RAW_16,
		.byr_max_bit = BYR_RAW_16,
	}, {
		.fourcc = V4L2_PIX_FMT_SGBRG16,
		.mbus_code = MEDIA_BUS_FMT_SGBRG16_1X16,
		.infmt = ISP_GBRG,
		.input_bit = RAW_16,
		.byr_max_bit = BYR_RAW_16,
	}, {
		.fourcc = V4L2_PIX_FMT_SGRBG16,
		.mbus_code = MEDIA_BUS_FMT_SGRBG16_1X16,
		.infmt = ISP_GRBG,
		.input_bit = RAW_16,
		.byr_max_bit = BYR_RAW_16,
	}, {
		.fourcc = V4L2_PIX_FMT_SRGGB16,
		.mbus_code = MEDIA_BUS_FMT_SRGGB16_1X16,
		.infmt = ISP_RGGB,
		.input_bit = RAW_16,
		.byr_max_bit = BYR_RAW_16,
	},
};

static void __isp_s_sensor_stby_handle(struct work_struct *work)
{
	int sensor_stby_stat, i;
	struct isp_dev *isp =
			container_of(work, struct isp_dev, s_sensor_stby_task);
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		vinc = vind->vinc[i];
		if (vinc->isp_sel == isp->id)
			break;
	}

	sensor_stby_stat = STBY_ON;
	v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
				SET_SENSOR_STANDBY, &sensor_stby_stat);

	sensor_stby_stat = STBY_OFF;
	v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core, ioctl,
				SET_SENSOR_STANDBY, &sensor_stby_stat);

	vin_print("%s done, %s is standby and then on!\n", __func__, vinc->vid_cap.pipe.sd[VIN_IND_SENSOR]->name);
}

#if IS_ENABLED(CONFIG_D3D)
static int isp_3d_pingpong_alloc(struct isp_dev *isp)
{
#if !defined ISP_600
	int ret = 0;
#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
	int cmp_ratio, bitdepth, wth;

	if (D3D_K_LBC_MODE > 10) {
		if (D3D_K_LBC_MODE > 20)
			cmp_ratio = 1000/2;
		else
			cmp_ratio = 1000*10/D3D_K_LBC_MODE;
	} else
		cmp_ratio = 1000;
	bitdepth = 5;
	wth = roundup(isp->mf.width, 16);
	if (D3D_K_LBC_MODE <= 10)
		isp->d3d_k_lbc.line_tar_bits = roundup(wth*bitdepth + wth/16*2, 512);
	else
		isp->d3d_k_lbc.line_tar_bits = roundup(cmp_ratio*wth*bitdepth/1000, 512);
	isp->d3d_k_lbc.mb_min_bit = clamp((cmp_ratio*bitdepth*16)/1000, 0, 127);

	if (D3D_RAW_LBC_MODE > 10) {
		if (D3D_RAW_LBC_MODE > 30)
			cmp_ratio = 1000/3;
		else
			cmp_ratio = 1000*10/D3D_RAW_LBC_MODE;
	} else
		cmp_ratio = 1000;
	bitdepth = 12;
	wth = roundup(isp->mf.width, 32);
	if (D3D_RAW_LBC_MODE <= 10)
		isp->d3d_raw_lbc.line_tar_bits = roundup(wth*bitdepth + wth/32*2, 512);
	else
		isp->d3d_raw_lbc.line_tar_bits = roundup(cmp_ratio*wth*bitdepth/1000, 512);
	isp->d3d_raw_lbc.mb_min_bit = clamp(((cmp_ratio*bitdepth + 500)/1000-1)*32+6, 0, 511);

	isp->d3d_pingpong[0].size = isp->d3d_k_lbc.line_tar_bits * isp->mf.height / 8;
	isp->d3d_pingpong[1].size = isp->d3d_raw_lbc.line_tar_bits * isp->mf.height / 8;
#if IS_ENABLED(CONFIG_D3D_LTF_EN)
	isp->d3d_pingpong[2].size = isp->d3d_raw_lbc.line_tar_bits * isp->mf.height / 8;
	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[2]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf2 requset failed!\n");
		return -ENOMEM;
	}
#endif
#elif IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
#if IS_ENABLED(CONFIG_D3D_LTF_EN)
	isp->d3d_pingpong[0].size = roundup(isp->mf.width, 64) * isp->mf.height * 29 / 8;
	isp->d3d_pingpong[1].size = roundup(isp->mf.width, 64) * isp->mf.height * 29 / 8;
#else
	isp->d3d_pingpong[0].size = roundup(isp->mf.width, 64) * isp->mf.height * 17 / 8;
	isp->d3d_pingpong[1].size = roundup(isp->mf.width, 64) * isp->mf.height * 17 / 8;
#endif
#else
	isp->d3d_pingpong[0].size = roundup(isp->mf.width, 64) * isp->mf.height * 29 / 8;
	isp->d3d_pingpong[1].size = roundup(isp->mf.width, 64) * isp->mf.height * 29 / 8;
#endif
	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[0]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf0 requset failed!\n");
		return -ENOMEM;
	}

	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[1]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf1 requset failed!\n");
		return -ENOMEM;
	}
	return ret;
#else /* else ISP_600 */
	int ret = 0;
	int kb_wnum, kb_hnum, hgt;
	int bitdepth = 12;
	unsigned int overlayer = vin_set_large_overlayer(isp->mf.width);

#if !defined CONFIG_ARCH_SUN55IW3 && IS_ENABLED(CONFIG_D3D_LBC_MODE)
	int mb_len, mb_min_bits_ratio, mb_max_bits_ratio, align;
	int wth, width_bitdepth;

	mb_min_bits_ratio = 820;
	mb_max_bits_ratio = 204;
	mb_len = 32;
	align = 256;

	if (!isp->d3d_lbc_ratio)
		isp->d3d_lbc_ratio = D3D_LBC_MODE;

	if (isp->d3d_lbc_ratio > 100) {
		if (isp->d3d_lbc_ratio > 400)
			isp->d3d_lbc.cmp_ratio = 1024/4;
		else
			isp->d3d_lbc.cmp_ratio = ((1024*100*10/isp->d3d_lbc_ratio) + 5) / 10;
	} else
		isp->d3d_lbc.cmp_ratio = 1024;

	if (isp->large_image == 3)
		isp->d3d_lbc.mb_num = DIV_ROUND_UP(isp->mf.width / 2 + overlayer, mb_len);
	else
		isp->d3d_lbc.mb_num = DIV_ROUND_UP(isp->mf.width, mb_len);

	isp->d3d_lbc.mb_min_bit = isp->d3d_lbc.cmp_ratio * bitdepth * mb_len / 1024 / 2 * mb_min_bits_ratio / 1024;

	if (isp->d3d_lbc.cmp_ratio >= 683) {
		isp->d3d_lbc.start_qp_std[0] = 0;
		isp->d3d_lbc.start_qp_std[1] = 0;
		isp->d3d_lbc.start_qp_std[2] = 0;
		isp->d3d_lbc.start_qp_std[3] = 0;
		isp->d3d_lbc.mb_min_bits_std[0] = 5;
		isp->d3d_lbc.mb_min_bits_std[1] = 6;
		isp->d3d_lbc.mb_min_bits_std[2] = 7;
		isp->d3d_lbc.mb_min_bits_std[3] = 8;
		isp->d3d_lbc.log_en = 0;
	} else if (isp->d3d_lbc.cmp_ratio < 683 && isp->d3d_lbc.cmp_ratio >= 512) {
		isp->d3d_lbc.start_qp_std[0] = 0;
		isp->d3d_lbc.start_qp_std[1] = 1;
		isp->d3d_lbc.start_qp_std[2] = 1;
		isp->d3d_lbc.start_qp_std[3] = 2;
		isp->d3d_lbc.mb_min_bits_std[0] = 4;
		isp->d3d_lbc.mb_min_bits_std[1] = 5;
		isp->d3d_lbc.mb_min_bits_std[2] = 6;
		isp->d3d_lbc.mb_min_bits_std[3] = 7;
		isp->d3d_lbc.log_en = 0;
	} else if (isp->d3d_lbc.cmp_ratio < 512 && isp->d3d_lbc.cmp_ratio >= 393) {
		isp->d3d_lbc.start_qp_std[0] = 0;
		isp->d3d_lbc.start_qp_std[1] = 1;
		isp->d3d_lbc.start_qp_std[2] = 2;
		isp->d3d_lbc.start_qp_std[3] = 3;
		isp->d3d_lbc.mb_min_bits_std[0] = 3;
		isp->d3d_lbc.mb_min_bits_std[1] = 4;
		isp->d3d_lbc.mb_min_bits_std[2] = 5;
		isp->d3d_lbc.mb_min_bits_std[3] = 6;
		isp->d3d_lbc.log_en = 1;
	} else {
		isp->d3d_lbc.start_qp_std[0] = 0;
		isp->d3d_lbc.start_qp_std[1] = 1;
		isp->d3d_lbc.start_qp_std[2] = 2;
		isp->d3d_lbc.start_qp_std[3] = 3;
		isp->d3d_lbc.mb_min_bits_std[0] = 2;
		isp->d3d_lbc.mb_min_bits_std[1] = 3;
		isp->d3d_lbc.mb_min_bits_std[2] = 4;
		isp->d3d_lbc.mb_min_bits_std[3] = 5;
		isp->d3d_lbc.log_en = 1;
	}

	if (isp->large_image == 3)
		wth = roundup(isp->mf.width / 2 + overlayer, 32);
	else
		wth = roundup(isp->mf.width, 32);
	if (isp->d3d_lbc_ratio <= 100) {
		isp->d3d_lbc.line_tar_bits = roundup((bitdepth * 16 + 2) * wth / 16, align);;
		isp->d3d_lbc.line_max_bits = isp->d3d_lbc.line_tar_bits;
		isp->d3d_lbc.is_lossy = 0;
	} else {
		width_bitdepth = roundup(bitdepth * wth, align);
		isp->d3d_lbc.line_tar_bits = min(roundup(isp->d3d_lbc.cmp_ratio * bitdepth * wth / 1024, align), width_bitdepth);
		isp->d3d_lbc.line_max_bits = min(roundup((mb_max_bits_ratio + 1024) * isp->d3d_lbc.line_tar_bits / 1024, align), width_bitdepth);
		isp->d3d_lbc.is_lossy = 1;
	}

	isp->d3d_lbc.line_stride = roundup(isp->d3d_lbc.line_max_bits + isp->d3d_lbc.mb_min_bit, 512) / 32;
#else
	isp->d3d_lbc.line_stride = roundup(isp->mf.width * bitdepth, 512) / 32;
#endif

	if (isp->large_image == 3)
		kb_wnum = roundup((isp->mf.width / 2 + overlayer) / 9, 8);
	else
		kb_wnum = roundup(isp->mf.width / 9, 8);
	kb_hnum = isp->mf.height / 9;
	hgt = roundup(isp->mf.height, 64);

	isp->d3d_pingpong[0].size = hgt + isp->mf.height * (isp->d3d_lbc.line_stride << 2); /* d3d_byr_addr */
	isp->d3d_pingpong[1].size = kb_wnum * kb_hnum; /* d3d_k0_addr */
	isp->d3d_pingpong[2].size = isp->d3d_pingpong[1].size; 	/* d3d_k1_addr */
	isp->d3d_pingpong[3].size = isp->d3d_pingpong[1].size; 	/* d3d_status_addr */

	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[0]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf0-d3d_byr_addr requset failed!\n");
		return -ENOMEM;
	}

	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[1]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf1-d3d_k0_addr requset failed!\n");
		return -ENOMEM;
	}

	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[2]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf2-d3d_k1_addr requset failed!\n");
		return -ENOMEM;
	}

	ret = os_mem_alloc(&isp->pdev->dev, &isp->d3d_pingpong[3]);
	if (ret < 0) {
		vin_err("isp 3d pingpong buf3-d3d_status_addr requset failed!\n");
		return -ENOMEM;
	}
	return ret;
#endif
}

static void isp_3d_pingpong_free(struct isp_dev *isp)
{
	os_mem_free(&isp->pdev->dev, &isp->d3d_pingpong[0]);
	os_mem_free(&isp->pdev->dev, &isp->d3d_pingpong[1]);
#if !defined ISP_600
#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
#if IS_ENABLED(CONFIG_D3D_LTF_EN)
	os_mem_free(&isp->pdev->dev, &isp->d3d_pingpong[2]);
#endif
#endif
#else /* else ISP_600 */
	os_mem_free(&isp->pdev->dev, &isp->d3d_pingpong[2]);
	os_mem_free(&isp->pdev->dev, &isp->d3d_pingpong[3]);
#endif
}

static int isp_3d_pingpong_update(struct isp_dev *isp)
{
	dma_addr_t addr;
#if !defined ISP_600
#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
	addr = (dma_addr_t)isp->d3d_pingpong[0].dma_addr;
	bsp_isp_set_d3d_ref_k_addr(isp->id, addr);
	addr = (dma_addr_t)isp->d3d_pingpong[1].dma_addr;
	bsp_isp_set_d3d_ref_raw_addr(isp->id, addr);
#if IS_ENABLED(CONFIG_D3D_LTF_EN)
	addr = (dma_addr_t)isp->d3d_pingpong[2].dma_addr;
	bsp_isp_set_d3d_ltf_raw_addr(isp->id, addr);
#endif

	if (D3D_K_LBC_MODE <= 10)
		bsp_isp_set_d3d_k_lbc_ctrl(isp->id, &isp->d3d_k_lbc, 0);
	else
		bsp_isp_set_d3d_k_lbc_ctrl(isp->id, &isp->d3d_k_lbc, 1);

	if (D3D_RAW_LBC_MODE <= 10)
		bsp_isp_set_d3d_raw_lbc_ctrl(isp->id, &isp->d3d_raw_lbc, 0);
	else
		bsp_isp_set_d3d_raw_lbc_ctrl(isp->id, &isp->d3d_raw_lbc, 1);

	bsp_isp_set_d3d_stride(isp->id, isp->d3d_k_lbc.line_tar_bits/32, isp->d3d_raw_lbc.line_tar_bits/32);
	bsp_isp_d3d_fifo_en(isp->id, 1);
#else
	struct vin_mm tmp;

	tmp = isp->d3d_pingpong[0];
	isp->d3d_pingpong[0] = isp->d3d_pingpong[1];
	isp->d3d_pingpong[1] = tmp;

	addr = (dma_addr_t)isp->d3d_pingpong[0].dma_addr;
	bsp_isp_set_d3d_addr0(isp->id, addr);
	addr = (dma_addr_t)isp->d3d_pingpong[1].dma_addr;
	bsp_isp_set_d3d_addr1(isp->id, addr);
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1) && !defined CONFIG_D3D_LTF_EN
	/* close d3d long time frame */
	writel(readl(isp->isp_load.vir_addr + 0x2d4) & ~(1 << 24), isp->isp_load.vir_addr + 0x2d4);
#endif
#endif
#else /* else ISP_600 */
	addr = (dma_addr_t)isp->d3d_pingpong[0].dma_addr;
	bsp_isp_set_d3d_bayer_addr(isp->id, addr);
	addr = (dma_addr_t)isp->d3d_pingpong[1].dma_addr;
	bsp_isp_set_d3d_k0_addr(isp->id, addr);
	addr = (dma_addr_t)isp->d3d_pingpong[2].dma_addr;
	bsp_isp_set_d3d_k1_addr(isp->id, addr);
	addr = (dma_addr_t)isp->d3d_pingpong[3].dma_addr;
	bsp_isp_set_d3d_status_addr(isp->id, addr);

	bsp_isp_set_d3d_lbc_cfg(isp->id, isp->d3d_lbc);
#endif
	return 0;
}
#else
static int isp_3d_pingpong_alloc(struct isp_dev *isp)
{
	return 0;
}
static void isp_3d_pingpong_free(struct isp_dev *isp)
{

}
static int isp_3d_pingpong_update(struct isp_dev *isp)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_WDR) && !defined ISP_600
static void isp_wdr_table_init(struct isp_dev *isp)
{
	int i;
#if !defined CONFIG_ARCH_SUN8IW16P1 && !defined CONFIG_ARCH_SUN8IW19P1
	short *wdr_tbl = isp->isp_lut_tbl.vir_addr + ISP_WDR_GAMMA_FE_MEM_OFS;
#else
	short *wdr_tbl = isp->isp_load.vir_addr + ISP_LOAD_REG_SIZE + ISP_FE_TBL_SIZE + ISP_S0_LC_TBL_SIZE;
#endif
	for (i = 0; i < 4096; i++) {
		wdr_tbl[i] = i;
		wdr_tbl[i + 4096] = i*16;
	}
}

static int isp_wdr_pingpong_alloc(struct isp_dev *isp)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
	int cmp_ratio, bitdepth, wth;

	if (WDR_RAW_LBC_MODE > 10) {
		if (WDR_RAW_LBC_MODE > 30)
			cmp_ratio = 1000/3;
		else
			cmp_ratio = 1000*10/WDR_RAW_LBC_MODE;
	} else
		cmp_ratio = 1000;
	bitdepth = 12;
	wth = roundup(isp->mf.width, 32);
	if (WDR_RAW_LBC_MODE <= 10)
		isp->wdr_raw_lbc.line_tar_bits = roundup(wth*bitdepth + wth/32*2, 512);
	else
		isp->wdr_raw_lbc.line_tar_bits = roundup(cmp_ratio*wth*bitdepth/1000, 512);
	isp->wdr_raw_lbc.mb_min_bit = clamp(((cmp_ratio*bitdepth + 500)/1000-1)*32+16, 0, 511);

	isp->wdr_pingpong[0].size = isp->wdr_raw_lbc.line_tar_bits * isp->mf.height / 8;
#else
	isp->wdr_pingpong[0].size = isp->mf.width * isp->mf.height * 2;
	isp->wdr_pingpong[1].size = isp->mf.width * isp->mf.height * 2;

	ret = os_mem_alloc(&isp->pdev->dev, &isp->wdr_pingpong[1]);
	if (ret < 0) {
		vin_err("isp wdr pingpong buf1 requset failed!\n");
		return -ENOMEM;
	}
#endif
	ret = os_mem_alloc(&isp->pdev->dev, &isp->wdr_pingpong[0]);
	if (ret < 0) {
		vin_err("isp wdr pingpong buf0 requset failed!\n");
		return -ENOMEM;
	}

	return ret;
}

static void isp_wdr_pingpong_free(struct isp_dev *isp)
{
#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
	os_mem_free(&isp->pdev->dev, &isp->wdr_pingpong[0]);
#else
	os_mem_free(&isp->pdev->dev, &isp->wdr_pingpong[0]);
	os_mem_free(&isp->pdev->dev, &isp->wdr_pingpong[1]);
#endif
}

static int isp_wdr_pingpong_set(struct isp_dev *isp)
{
	dma_addr_t addr;
#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
	addr = (dma_addr_t)isp->wdr_pingpong[0].dma_addr;
	bsp_isp_set_wdr_addr0(isp->id, addr);
	if (WDR_RAW_LBC_MODE <= 10)
		bsp_isp_set_wdr_raw_lbc_ctrl(isp->id, &isp->wdr_raw_lbc, 0);
	else
		bsp_isp_set_wdr_raw_lbc_ctrl(isp->id, &isp->wdr_raw_lbc, 1);
	bsp_isp_set_wdr_stride(isp->id, isp->wdr_raw_lbc.line_tar_bits / 32);
	bsp_isp_wdr_fifo_en(isp->id, 1);
#else
	addr = (dma_addr_t)isp->wdr_pingpong[0].dma_addr;
	bsp_isp_set_wdr_addr0(isp->id, addr);
	addr = (dma_addr_t)isp->wdr_pingpong[1].dma_addr;
	bsp_isp_set_wdr_addr1(isp->id, addr);
#endif
	return 0;
}
#else /* isp600 */
static void isp_wdr_table_init(struct isp_dev *isp) {}

static int isp_wdr_pingpong_alloc(struct isp_dev *isp)
{
#if !defined ISP_600
	isp->wdr_mode = ISP_NORMAL_MODE;
#endif
	return 0;
}

static void isp_wdr_pingpong_free(struct isp_dev *isp) {}

static int isp_wdr_pingpong_set(struct isp_dev *isp)
{
	return 0;
}
#endif

#if defined SUPPORT_ISP_TDM && !defined ISP_600
static int __sunxi_isp_tdm_off(struct isp_dev *isp)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	int i, j;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;
		vinc = vind->vinc[i];
		for (j = 0; j < VIN_MAX_ISP; j++) {
			if (vinc->isp_sel == j)
				return -1;
		}
	}
	return 0;
}
#endif

#if defined ISP_600
static int sunxi_isp_logic_s_stream(unsigned char virtual_id, int on)
{
	unsigned char logic_id = isp_virtual_find_logic[virtual_id];
	struct isp_dev *logic_isp = glb_isp[logic_id];

	if (logic_isp->work_mode == ISP_ONLINE && virtual_id != logic_id) {
		vin_err("isp%d work on online mode, isp%d cannot to work!!\n", logic_id, virtual_id);
		return -1;
	}

	if (on && (logic_isp->logic_top_stream_count)++ > 0)
		return 0;
	else if (!on && (logic_isp->logic_top_stream_count == 0 || --(logic_isp->logic_top_stream_count) > 0))
		return 0;

	if (on) {
		bsp_isp_sram_boot_mode_ctrl(logic_isp->id, SRAM_NORMAL_MODE);
		bsp_isp_enable(logic_isp->id, on);
		bsp_isp_mode(logic_isp->id, logic_isp->work_mode);
		//bsp_isp_set_clk_back_door(logic_isp->id, on);
		bsp_isp_top_capture_start(logic_isp->id);
	} else {
		if (logic_isp->work_mode == ISP_ONLINE) {
			bsp_isp_top_capture_stop(logic_isp->id);
			bsp_isp_enable(logic_isp->id, on);
			bsp_isp_sram_boot_mode_ctrl(logic_isp->id, SRAM_BOOT_MODE);
		}

#if VIN_FALSE
#if defined SUPPORT_ISP_TDM && defined TDM_V200
		/* in offline mode, top tdm must close after top isp, otherwise isp int will occur err*/
		if (logic_isp->work_mode == ISP_OFFLINE) {
			csic_tdm_set_speed_dn(logic_id, 0);
			csic_tdm_tx_cap_disable(logic_id);
			csic_tdm_tx_disable(logic_id);
			csic_tdm_disable(logic_id);
			csic_tdm_top_disable(logic_id);
			sunxi_tdm_buffer_free(logic_id);
		}
#endif
#endif
	}

	return 0;
}

static void sunxi_isp_set_load_ddr(struct isp_dev *isp)
{
/*
	struct v4l2_mbus_framefmt *mf = &isp->mf;

	bsp_isp_set_line_int_num(isp->id, mf->height - 1);
*/
#if !defined CONFIG_ARCH_SUN55IW3 && IS_ENABLED(CONFIG_D3D_PKG_MODE)
	bsp_isp_d3d_ref_frm_mode(isp->id, 1);
#endif
	bsp_isp_set_input_fmt(isp->id, isp->isp_fmt->infmt);
	if (isp->isp_fmt->byr_max_bit != 0) {
		bsp_isp_set_byr_max_bit(isp->id, isp->isp_fmt->byr_max_bit);
		bsp_isp_set_byr_act_bit(isp->id, isp->isp_fmt->input_bit);
	}
	bsp_isp_set_size(isp->id, &isp->isp_ob);
	bsp_isp_set_last_blank_cycle(isp->id, 5);
	bsp_isp_set_speed_mode(isp->id, 3);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN8IW21)
	bsp_isp_set_save_load_addr(isp->id, (dma_addr_t)isp->isp_save_load.dma_addr);
#endif
	if (isp->wdr_mode == ISP_NORMAL_MODE) {
		bsp_isp_set_ch_input_bit(isp->id, 0, isp->isp_fmt->input_bit);
		switch (isp->isp_fmt->input_bit) {
		case RAW_14:
		case RAW_16:
			bsp_isp_set_ch_output_bit(isp->id, 0, isp->isp_fmt->input_bit);
			break;
		default:
			break;
		}
	} else if (isp->wdr_mode == ISP_DOL_WDR_MODE) {
		bsp_isp_set_ch_input_bit(isp->id, 0, isp->isp_fmt->input_bit);
		bsp_isp_set_ch_input_bit(isp->id, 1, isp->isp_fmt->input_bit);
	}
}
#endif

#if !defined ISP_600
static void sunxi_isp_cal_bandwidth_memory(struct v4l2_subdev *sd, unsigned int on)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct v4l2_mbus_framefmt *mf = &isp->mf;
	__maybe_unused struct mbus_framefmt_res *res = (void *)mf->reserved;
	unsigned int bandw;

	bandw = 0;
#if IS_ENABLED(CONFIG_D3D)
	bandw += isp->mf.height * isp->mf.width * 5 / 8 * 2 * res->fps * 10 / D3D_RAW_LBC_MODE;
#endif

#if IS_ENABLED(CONFIG_WDR)
	if (res->res_wdr_mode == ISP_DOL_WDR_MODE)
		bandw += isp->mf.height * isp->mf.width * 3 / 2 * 2 * res->fps * 10 / WDR_RAW_LBC_MODE;
#endif

	if (on)
		vind->isp_bd_tatol += bandw;
	else
		vind->isp_bd_tatol -= bandw;

	vin_log(VIN_LOG_ISP, "isp%d %s bandwidth = %d.\n", isp->id, on ? "use" : "release", bandw);
}
#endif

static int sunxi_isp_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &isp->mf;
	struct mbus_framefmt_res *res = (void *)mf->reserved;
	__maybe_unused struct v4l2_event event;
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW19P1) || IS_ENABLED(CONFIG_ARCH_SUN50IW10)
	struct isp_wdr_mode_cfg wdr_cfg;
#endif
	unsigned int load_val;
	__maybe_unused int i;
	__maybe_unused unsigned int data[1];
	__maybe_unused unsigned int isp_cur_id, isp_status;

	if (!isp->use_isp)
		return 0;

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
	case V4L2_PIX_FMT_SBGGR14:
	case V4L2_PIX_FMT_SGBRG14:
	case V4L2_PIX_FMT_SGRBG14:
	case V4L2_PIX_FMT_SRGGB14:
	case V4L2_PIX_FMT_SBGGR16:
	case V4L2_PIX_FMT_SGBRG16:
	case V4L2_PIX_FMT_SGRBG16:
	case V4L2_PIX_FMT_SRGGB16:
#if IS_ENABLED(CONFIG_SENSOR_RGB)
	case V4L2_PIX_FMT_RGB24:
	case V4L2_PIX_FMT_BGR24:
	case V4L2_PIX_FMT_RGB565:
#endif
		vin_log(VIN_LOG_FMT, "%s output fmt is raw, return directly\n", __func__);
		if (isp->isp_dbg.debug_en) {
			bsp_isp_debug_output_cfg(isp->id, 1, isp->isp_dbg.debug_sel);
			break;
		} else {
			return 0;
		}
	default:
		break;
	}

	if (enable) {
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
		if (!isp->first_init_server) {
			if (isp->isp_server_reset) {
				isp_reset_config_sensor_info(isp, VIN_SET_ISP_START);
				isp->isp_server_reset = 0;
			} else
				isp_reset_config_sensor_info(isp, VIN_SET_ISP_RESET);
		} else {
			isp->load_select = false;
			bsp_isp_set_saved_addr(isp->id, (unsigned long)isp->isp_save.dma_addr);
			bsp_isp_set_load_addr(isp->id, (dma_addr_t)isp->load_para[0].dma_addr);
			bsp_isp_set_save_load_addr(isp->id, (dma_addr_t)isp->isp_save_load.dma_addr);
			isp->first_init_server = 0;
#if !IS_ENABLED(CONFIG_VIN_INIT_MELIS)
			isp_reset_config_sensor_info(isp, VIN_SET_ISP_START);
#endif
		}
#endif

#if defined ISP_600
		isp->init_done_flag = 0;
#endif
		isp->h3a_stat.frame_number = 0;
		isp->ptn_isp_cnt = 0;
		isp->isp_ob.set_cnt = 0;
		isp->sensor_lp_mode = res->res_lp_mode;
		/* when normal to wdr, old register would lead timeout, so we clean it up */
		if (isp->wdr_mode != res->res_wdr_mode) {
			isp->wdr_mode = res->res_wdr_mode;
			memcpy(isp->isp_load.vir_addr, &isp_default_reg[0], ISP_LOAD_REG_SIZE);
		}

#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
		if (isp->load_flag)
			memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
		else if (memcmp(&isp->save_size, &isp->isp_ob.ob_black, sizeof(struct isp_size)))
			memcpy(isp->isp_load.vir_addr, &isp_default_reg[0], ISP_LOAD_REG_SIZE);
		memcpy(&isp->save_size, &isp->isp_ob.ob_valid, sizeof(struct isp_size));
#else
		if (memcmp(&isp->save_size, &isp->isp_ob.ob_black, sizeof(struct isp_size)) != 0) {
			memcpy(&isp->save_size, &isp->isp_ob.ob_black, sizeof(struct isp_size));
			isp->load_flag = 0;
			memcpy(isp->isp_load.vir_addr, &isp_default_reg[0], ISP_LOAD_REG_SIZE);
		} else if (isp->large_image == 3) {
			memcpy(isp->isp_load.vir_addr, &isp_default_reg[0], ISP_LOAD_REG_SIZE);
		} else if (isp->load_flag) {
			memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
		}
#endif

		if (isp->large_image == 0 || isp->large_image == 3) {
			if (isp->runtime_flag == 0) {
				if (isp_3d_pingpong_alloc(isp))
					return -ENOMEM;
			} else
				isp->runtime_flag = 0;
			isp_3d_pingpong_update(isp);
		}
		if (isp->wdr_mode != ISP_NORMAL_MODE)
			isp_wdr_table_init(isp);
		if (isp->wdr_mode == ISP_DOL_WDR_MODE) {
			if (isp_wdr_pingpong_alloc(isp)) {
				isp_3d_pingpong_free(isp);
				return -ENOMEM;
			}
			isp_wdr_pingpong_set(isp);
		}
#if !defined ISP_600
#ifndef SUPPORT_ISP_TDM
		bsp_isp_enable(isp->id, 1);
#else
		for (i = 0; i < VIN_MAX_ISP; i++)
			bsp_isp_enable(i, 1);
#endif
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
#if !defined CONFIG_ARCH_SUN8IW16P1 && !defined CONFIG_ARCH_SUN8IW19P1 && !defined CONFIG_ARCH_SUN50IW10
		bsp_isp_irq_enable(isp->id, FINISH_INT_EN | PARA_LOAD_INT_EN | SRC0_FIFO_INT_EN
				     | FRAME_ERROR_INT_EN | FRAME_LOST_INT_EN);

		load_val = bsp_isp_load_update_flag(isp->id);
		if (isp->wdr_mode == ISP_DOL_WDR_MODE) {
			load_val = load_val | WDR_UPDATE;
			bsp_isp_module_enable(isp->id, WDR_EN);
			bsp_isp_set_wdr_mode(isp->id, ISP_DOL_WDR_MODE);
			bsp_isp_ch_enable(isp->id, ISP_CH1, 1);
		} else if (isp->wdr_mode == ISP_COMANDING_MODE) {
			load_val = load_val | WDR_UPDATE;
			bsp_isp_module_enable(isp->id, WDR_EN);
			bsp_isp_set_wdr_mode(isp->id, ISP_COMANDING_MODE);
		} else {
			load_val = load_val & ~WDR_UPDATE;
			bsp_isp_module_disable(isp->id, WDR_EN);
			bsp_isp_set_wdr_mode(isp->id, ISP_NORMAL_MODE);
		}
#else
		bsp_isp_irq_enable(isp->id, FINISH_INT_EN | S0_PARA_LOAD_INT_EN | S0_FIFO_INT_EN
				     | S0_FRAME_ERROR_INT_EN | S0_FRAME_LOST_INT_EN);
#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
		bsp_isp_irq_enable(isp->id, S0_BTYPE_ERROR_INT_EN | ADDR_ERROR_INT_EN | LBC_ERROR_INT_EN);
#endif

		load_val = bsp_isp_load_update_flag(isp->id);
		if (isp->wdr_mode == ISP_DOL_WDR_MODE) {
			load_val = load_val | WDR_UPDATE;
			wdr_cfg.wdr_exp_seq = 0;
			wdr_cfg.wdr_ch_seq = 0;
			wdr_cfg.wdr_mode = 0;
			bsp_isp_module_enable(isp->id, WDR_EN);
			bsp_isp_wdr_mode_cfg(isp->id, &wdr_cfg);
			bsp_isp_ch_enable(isp->id, ISP_CH1, 1);
		} else if (isp->wdr_mode == ISP_COMANDING_MODE) {
			load_val = load_val | WDR_UPDATE;
			wdr_cfg.wdr_exp_seq = 0;
			wdr_cfg.wdr_ch_seq = 0;
			wdr_cfg.wdr_mode = ISP_COMANDING_MODE;
			bsp_isp_module_enable(isp->id, WDR_EN);
			bsp_isp_wdr_mode_cfg(isp->id, &wdr_cfg);
		} else {
			load_val = load_val & ~WDR_UPDATE;
			bsp_isp_module_disable(isp->id, WDR_EN);
		}
#endif

#if !defined CONFIG_D3D
		bsp_isp_module_disable(isp->id, D3D_EN);
		load_val = load_val & ~D3D_UPDATE;
#endif
		if (isp->large_image == 2)
			bsp_isp_module_disable(isp->id, PLTM_EN | D3D_EN | AE_EN | AWB_EN | AF_EN | HIST_EN);
		bsp_isp_update_table(isp->id, load_val);

		bsp_isp_module_enable(isp->id, SRC0_EN);
		bsp_isp_set_input_fmt(isp->id, isp->isp_fmt->infmt);
		bsp_isp_set_size(isp->id, &isp->isp_ob);
		bsp_isp_set_para_ready_mode(isp->id, 1);
		bsp_isp_set_para_ready(isp->id, PARA_READY);
		bsp_isp_set_last_blank_cycle(isp->id, 5);
		bsp_isp_set_speed_mode(isp->id, 3);
#if !defined CONFIG_D3D_LTF_EN && !defined CONFIG_WDR
		bsp_isp_set_fifo_mode(isp->id, 0);
		bsp_isp_fifo_raw_write(isp->id, 0x200);
#endif
		bsp_isp_ch_enable(isp->id, ISP_CH0, 1);
		bsp_isp_capture_start(isp->id);
#else /* else ISP_600 */
		if (sunxi_isp_logic_s_stream(isp->id, enable))
			return -1;

		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_irq_enable(isp->id, PARA_LOAD_INT_EN | PARA_SAVE_INT_EN);
		bsp_isp_irq_enable(isp->id, DDR_RW_ERROR_INT_EN | HB_SHORT_INT_EN | CFG_ERROR_INT_EN | FRAME_LOST_INT_EN |
						INTER_FIFO_FULL_INT_EN | WDMA_FIFO_FULL_INT_EN | WDMA_OVER_BND_INT_EN |
						RDMA_FIFO_FULL_INT_EN | LBC_ERROR_INT_EN | AHB_MBUS_W_INT_EN);

		load_val = bsp_isp_load_update_flag(isp->id);
#if !defined CONFIG_D3D
		bsp_isp_module_disable(isp->id, D3D_EN);
		load_val = load_val & ~D3D_UPDATE;
#else
		bsp_isp_clr_d3d_rec_en(isp->id);
#endif
		load_val = load_val | (GAMMA_UPDATE | CEM_UPDATE);
		bsp_isp_update_table(isp->id, load_val);

		/* load addr*/
		sunxi_isp_set_load_ddr(isp);

		memcpy(isp->load_para[0].vir_addr, isp->isp_load.vir_addr, ISP_LOAD_DRAM_SIZE);
		/* when two video open and close in two pthread, open and close isp_init is not seted, load_para[1] is using lead to error */
		memcpy(isp->load_para[1].vir_addr, isp->isp_load.vir_addr, ISP_LOAD_DRAM_SIZE);

		bsp_isp_set_para_ready(isp->id, PARA_READY);
		bsp_isp_capture_start(isp->id);
#endif
	} else {
		if (!isp->nosend_ispoff && !isp->runtime_flag) {
#if !defined CONFIG_ISP_SERVER_MELIS
			memset(&event, 0, sizeof(event));
			event.type = V4L2_EVENT_VIN_ISP_OFF;
			event.id = 0;
			v4l2_event_queue(isp->subdev.devnode, &event);
#else /* CONFIG_ISP_SERVER_MELIS */
			data[0] = VIN_SET_CLOSE_ISP;
			isp_rpmsg_send(isp, data, 1*4);
#endif
		}
#if !defined ISP_600
		bsp_isp_capture_stop(isp->id);
		bsp_isp_module_disable(isp->id, SRC0_EN);
		bsp_isp_ch_enable(isp->id, ISP_CH0, 0);
		bsp_isp_irq_disable(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
#ifndef SUPPORT_ISP_TDM
		bsp_isp_enable(isp->id, 0);
#else
		if (__sunxi_isp_tdm_off(isp) == 0) {
			for (i = 0; i < VIN_MAX_ISP; i++)
				bsp_isp_enable(i, 0);
		} else
			vin_warn("ISP is used in TDM mode, ISP%d cannot be closing when other isp is used!\n", isp->id);
#endif
#else /* else ISP_600 */
		bsp_isp_capture_stop(isp->id);
		bsp_isp_irq_disable(isp->id, ISP_IRQ_EN_ALL);
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);

		if (glb_isp[isp->id/ISP_VIRT_NUM]->work_mode == ISP_OFFLINE) {
			for (i = 0; i < 200; i++) {
				isp_cur_id = bsp_isp_get_isp_cur_id(isp->id);
				if (isp_cur_id != isp->id)
					break;
				isp_status = bsp_isp_get_internal_status1(isp->id, ISP_STATUS_MASK);
				if (isp_status != ISP_STATUS_ISP_PRO0 && isp_status != ISP_STATUS_ISP_PRO1 \
					&& isp_status != ISP_STATUS_WRITE_SDRAM && isp_status != ISP_STATUS_FINISH_END_WAIT)
					break;
				usleep_range(500, 510);
				if (i >= 199)
					vin_warn("%s waiting for isp%d for %d ms to free resources", __func__, isp->id, i / 2);
			}
		}
		sunxi_isp_logic_s_stream(isp->id, enable);
#endif
		if (isp->large_image == 0 || isp->large_image == 3) {
			if (isp->runtime_flag == 0) {
				usleep_range(2000, 2100);
				isp_3d_pingpong_free(isp);
			}
		}
		if (isp->wdr_mode == ISP_DOL_WDR_MODE) {
#if !defined ISP_600
			bsp_isp_ch_enable(isp->id, ISP_CH1, 0);
#endif
			isp_wdr_pingpong_free(isp);
		}
		isp->f1_after_librun = 0;
		isp->load_flag = 0;
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
		cancel_work_sync(&isp->isp_rpmsg_send_task);
#endif
	}
#if !defined ISP_600
	sunxi_isp_cal_bandwidth_memory(sd, enable);
#endif

	vin_log(VIN_LOG_FMT, "isp%d %s, %d*%d hoff: %d voff: %d code: %x field: %d\n",
		isp->id, enable ? "stream on" : "stream off",
		isp->isp_ob.ob_valid.width, isp->isp_ob.ob_valid.height,
		isp->isp_ob.ob_start.hor, isp->isp_ob.ob_start.ver,
		mf->code, mf->field);
	vin_log(VIN_LOG_LARGE, "isp%d %s, %d*%d hoff: %d voff: %d code: %x field: %d\n",
		isp->id, enable ? "stream on" : "stream off",
		isp->isp_ob.ob_valid.width, isp->isp_ob.ob_valid.height,
		isp->isp_ob.ob_start.hor, isp->isp_ob.ob_start.ver,
		mf->code, mf->field);

	return 0;
}

static struct isp_pix_fmt *__isp_try_format(struct isp_dev *isp,
					struct v4l2_mbus_framefmt *mf)
{
	struct isp_pix_fmt *isp_fmt = NULL;
	struct isp_size_settings *ob = &isp->isp_ob;
	unsigned int i;
	unsigned int overlayer;

	for (i = 0; i < ARRAY_SIZE(sunxi_isp_formats); ++i)
		if (mf->code == sunxi_isp_formats[i].mbus_code)
			isp_fmt = &sunxi_isp_formats[i];

	if (isp_fmt == NULL)
		isp_fmt = &sunxi_isp_formats[0];

	if (isp->large_image == 3) {
		overlayer = vin_set_large_overlayer(mf->width);
		if (isp->id == 0) {
			ob->ob_black.width = mf->width / 2 + overlayer;
			ob->ob_valid.width = mf->width / 2 + overlayer;
		} else if (isp->id == 1) {
			ob->ob_black.width = mf->width / 2 + overlayer;
			ob->ob_valid.width = mf->width / 2 + overlayer;
		}
		ob->ob_black.height = mf->height;
		ob->ob_valid.height = mf->height;
	} else {
		ob->ob_black.width = mf->width;
		ob->ob_black.height = mf->height;

		if (!isp->large_image) {
			if (isp->id == 1) {
				mf->width = clamp_t(u32, mf->width, MIN_IN_WIDTH, 3264);
				mf->height = clamp_t(u32, mf->height, MIN_IN_HEIGHT, 3264);
			} else {
				mf->width = clamp_t(u32, mf->width, MIN_IN_WIDTH, 4224);
				mf->height = clamp_t(u32, mf->height, MIN_IN_HEIGHT, 4224);
			}
		}

		ob->ob_valid.width = mf->width;
		ob->ob_valid.height = mf->height;
	}

	ob->ob_start.hor = (ob->ob_black.width - ob->ob_valid.width) / 2;
	ob->ob_start.ver = (ob->ob_black.height - ob->ob_valid.height) / 2;

	if (isp->large_image == 2) {
		isp->left_right = 0;
		isp->isp_ob.ob_valid.width = mf->width / 2 + LARGE_IMAGE_OFF;
	}

	switch (mf->colorspace) {
	case V4L2_COLORSPACE_REC709:
		mf->colorspace = V4L2_COLORSPACE_REC709;
		break;
	case V4L2_COLORSPACE_BT2020:
		mf->colorspace = V4L2_COLORSPACE_BT2020;
		break;
	default:
		mf->colorspace = V4L2_COLORSPACE_JPEG;
		break;
	}
	return isp_fmt;
}

int sunxi_isp_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	isp->capture_mode = cp->capturemode;
	isp->large_image = cp->reserved[2];

	return 0;
}

int sunxi_isp_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE)
		return -EINVAL;

	memset(cp, 0, sizeof(*cp));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = isp->capture_mode;

	return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static int sunxi_isp_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *state,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	mutex_lock(&isp->subdev_lock);
	fmt->format = isp->mf;
	mutex_unlock(&isp->subdev_lock);
	return 0;
}

static int sunxi_isp_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *state,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &isp->mf;
	struct isp_pix_fmt *isp_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	if (fmt->pad == ISP_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&isp->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&isp->subdev_lock);
		}
		return 0;
	}
	isp_fmt = __isp_try_format(isp, &fmt->format);
	if (mf) {
		mutex_lock(&isp->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			isp->isp_fmt = isp_fmt;
		mutex_unlock(&isp->subdev_lock);
	}

	return 0;

}

#else /* before linux-5.15 */
static int sunxi_isp_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	mutex_lock(&isp->subdev_lock);
	fmt->format = isp->mf;
	mutex_unlock(&isp->subdev_lock);
	return 0;
}

static int sunxi_isp_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_format *fmt)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &isp->mf;
	struct isp_pix_fmt *isp_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	if (fmt->pad == ISP_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&isp->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&isp->subdev_lock);
		}
		return 0;
	}
	isp_fmt = __isp_try_format(isp, &fmt->format);
	if (mf) {
		mutex_lock(&isp->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			isp->isp_fmt = isp_fmt;
		mutex_unlock(&isp->subdev_lock);
	}

	return 0;

}
#endif

int sunxi_isp_subdev_init(struct v4l2_subdev *sd, u32 val)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);

	if (!isp->use_isp)
		return 0;

	if (val && isp->use_cnt++ > 0)
		return 0;
	else if (!val && (isp->use_cnt == 0 || --isp->use_cnt > 0))
		return 0;

	vin_log(VIN_LOG_ISP, "isp%d %s use_cnt = %d.\n", isp->id,
		val ? "init" : "uninit", isp->use_cnt);

	if (val) {
		bsp_isp_ver_read_en(isp->id, 1);
		bsp_isp_get_isp_ver(isp->id, &vind->isp_ver_major, &vind->isp_ver_minor);
		bsp_isp_ver_read_en(isp->id, 0);
		if (!isp->have_init) {
			if (!isp->load_flag) {
				memcpy(isp->isp_load.vir_addr, &isp_default_reg[0], ISP_LOAD_REG_SIZE);
				memset(&isp->load_shadow[0], 0, ISP_LOAD_DRAM_SIZE);
				isp->load_flag = 0;
			}
			isp->have_init = 1;
		} else {
#if IS_ENABLED(CONFIG_D3D)
#if !defined ISP_600
			if ((isp->load_shadow[0x2d4 + 0x3]) & (1<<1)) {
				/* clear D3D rec_en */
				isp->load_shadow[0x2d4 + 0x3] = (isp->load_shadow[0x2d4 + 0x3]) & (~(1<<1));
				memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
			}
#else /* ISP_600 */
			bsp_isp_clr_d3d_rec_en(isp->id);
#endif
#endif
		}
		isp->isp_frame_number = 0;
		isp->h3a_stat.buf[0].empty = 1;
		isp->h3a_stat.buf[0].state = ISPSTAT_LOAD_SET;
		isp->h3a_stat.buf[0].dma_addr = isp->isp_stat.dma_addr;
		isp->h3a_stat.buf[0].virt_addr = isp->isp_stat.vir_addr;
		bsp_isp_set_statistics_addr(isp->id, (dma_addr_t)isp->isp_stat.dma_addr);
		bsp_isp_set_saved_addr(isp->id, (unsigned long)isp->isp_save.dma_addr);
#if !defined ISP_600
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW17P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW15P1)
		bsp_isp_set_load_addr(isp->id, (unsigned long)isp->isp_load.dma_addr);
		bsp_isp_set_table_addr(isp->id, LENS_GAMMA_TABLE, (unsigned long)(isp->isp_lut_tbl.dma_addr));
		bsp_isp_set_table_addr(isp->id, DRC_TABLE, (unsigned long)(isp->isp_drc_tbl.dma_addr));
#elif IS_ENABLED(CONFIG_ARCH_SUN8IW16P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW19P1) || IS_ENABLED(CONFIG_ARCH_SUN50IW10)
		bsp_isp_set_load_addr0(isp->id, (dma_addr_t)isp->isp_load.dma_addr);
		bsp_isp_set_load_addr1(isp->id, (dma_addr_t)isp->isp_load.dma_addr);
#endif
#else /* isp600 */
		isp->load_select = false;
		bsp_isp_set_load_addr(isp->id, (dma_addr_t)isp->load_para[0].dma_addr);
		bsp_isp_set_save_load_addr(isp->id, (dma_addr_t)isp->isp_save_load.dma_addr);
#endif
		bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
	}

	return 0;
}

static int __isp_set_load_reg(struct v4l2_subdev *sd, struct isp_table_reg_map *reg)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int ret = 0;

	if (!isp->use_isp)
		return 0;

	if (reg->size > ISP_LOAD_DRAM_SIZE) {
		vin_err("user ask for 0x%x data, it more than isp load_data 0x%x\n", reg->size, ISP_LOAD_DRAM_SIZE);
		return -EINVAL;
	}

#if defined ISP_600
	if (isp->large_image == 3 && glb_isp[0]) {
		if (isp->id == 0) {
			//ae stat region
			ret = copy_from_user(&isp->ae_stat_save[0], reg->addr + ISP_AE_REG_OFS, ISP_AE_REG_SIZE);
			//af stat region
			ret = copy_from_user(&isp->af_stat_save[0], reg->addr + ISP_AF_REG_OFS, ISP_AF_REG_SIZE);
			//awb stat region
			ret = copy_from_user(&isp->awb_stat_save[0], reg->addr + ISP_AWB_REG_OFS, ISP_AWB_REG_SIZE);
			//hist stat region
			ret = copy_from_user(&isp->hist_stat_save[0], reg->addr + ISP_HIST_REG_OFS, ISP_HIST_REG_SIZE);
			//nr msc table
			ret = copy_from_user(&isp->nr_msc_load_save[0], reg->addr + ISP_LOAD_REG_SIZE + ISP_FE_TBL_SIZE + ISP_RSC_TBL_SIZE + ISP_RSC_TBL_SIZE, ISP_MSC_NR_TBL_SIZE);
			//return ret;
			if (!isp->init_done_flag) {
				isp->load_save = 1;
				return ret;
			}
		} else {
			ret = copy_from_user(&isp->load_shadow[0], reg->addr, reg->size);
			if (glb_isp[0]->use_isp) {
				memcpy(&glb_isp[0]->load_shadow[0], &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
				//AHB Registers
				memcpy(&glb_isp[0]->load_shadow[ISP_AHB0_MEM_OFS], &isp->load_shadow[ISP_AHB1_MEM_OFS], ISP_AHB_MEM_SIZE);
				//lsc table copy to msc table
				memcpy(&glb_isp[0]->load_shadow[ISP_LOAD_REG_SIZE + ISP_FE_TBL_SIZE + ISP_RSC_TBL_SIZE],
						&isp->load_shadow[ISP_LOAD_REG_SIZE + ISP_FE_TBL_SIZE], ISP_RSC_TBL_SIZE);
				//nr msc copy
				memcpy(&glb_isp[0]->load_shadow[ISP_LOAD_REG_SIZE + ISP_FE_TBL_SIZE + ISP_RSC_TBL_SIZE + ISP_RSC_TBL_SIZE],
						&glb_isp[0]->nr_msc_load_save[0], ISP_MSC_NR_TBL_SIZE);
				//ae stat region
				memcpy(&glb_isp[0]->load_shadow[ISP_AE_REG_OFS], &glb_isp[0]->ae_stat_save[0], ISP_AE_REG_SIZE);
				//af stat region
				memcpy(&glb_isp[0]->load_shadow[ISP_AF_REG_OFS], &glb_isp[0]->af_stat_save[0], ISP_AF_REG_SIZE);
				//awb stat region
				memcpy(&glb_isp[0]->load_shadow[ISP_AWB_REG_OFS], &glb_isp[0]->awb_stat_save[0], ISP_AWB_REG_SIZE);
				//hist stat region
				memcpy(&glb_isp[0]->load_shadow[ISP_HIST_REG_OFS], &glb_isp[0]->hist_stat_save[0], ISP_HIST_REG_SIZE);
				//glb_isp[0]->load_flag = 1;
				glb_isp[0]->init_done_flag = 1;
			} else {
				vin_err("isp merge mode, but isp0 is not used\n");
			}
		}
	} else {
		ret = copy_from_user(&isp->load_shadow[0], reg->addr, reg->size);
	}
#else
	ret = copy_from_user(&isp->load_shadow[0], reg->addr, reg->size);
#endif
	isp->load_flag = 1;
	isp->load_save = 1;
	return copy_from_user(&isp->load_shadow[0], reg->addr, reg->size);
}

#if !defined CONFIG_ARCH_SUN8IW16P1 && !defined CONFIG_ARCH_SUN8IW19P1 && !defined CONFIG_ARCH_SUN50IW10 && \
	!defined ISP_600
static int __isp_set_table1_map(struct v4l2_subdev *sd, struct isp_table_reg_map *tbl)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int ret;

	if (!isp->use_isp)
		return 0;

	if (tbl->size > ISP_TABLE_MAPPING1_SIZE) {
		vin_err("user ask for 0x%x data, it more than isp table1_data 0x%x\n", tbl->size, ISP_TABLE_MAPPING1_SIZE);
		return -EINVAL;
	}

	ret = copy_from_user(&isp->load_shadow[0] + ISP_LOAD_REG_SIZE, tbl->addr, tbl->size);
	if (ret < 0) {
		vin_err("copy table mapping1 from usr error!\n");
		return ret;
	}

	return 0;
}

static int __isp_set_table2_map(struct v4l2_subdev *sd, struct isp_table_reg_map *tbl)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	int ret;

	if (!isp->use_isp)
		return 0;

	if (tbl->size > ISP_TABLE_MAPPING2_SIZE) {
		vin_err("user ask for 0x%x data, it more than isp table1_data 0x%x\n", tbl->size, ISP_TABLE_MAPPING2_SIZE);
		return -EINVAL;
	}

	ret = copy_from_user(&isp->load_shadow[0] + ISP_LOAD_REG_SIZE + ISP_TABLE_MAPPING1_SIZE, tbl->addr, tbl->size);
	if (ret < 0) {
		vin_err("copy table mapping2 from usr error!\n");
		return ret;
	}

	return 0;
}
#endif

static long sunxi_isp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
				   void *arg)
{
	int ret = 0;

	switch (cmd) {
	case VIDIOC_VIN_ISP_LOAD_REG:
		ret = __isp_set_load_reg(sd, (struct isp_table_reg_map *)arg);
		break;
#if !defined CONFIG_ARCH_SUN8IW16P1 && !defined CONFIG_ARCH_SUN8IW19P1 && !defined CONFIG_ARCH_SUN50IW10 && \
	!defined ISP_600
	case VIDIOC_VIN_ISP_TABLE1_MAP:
		ret = __isp_set_table1_map(sd, (struct isp_table_reg_map *)arg);
		break;
	case VIDIOC_VIN_ISP_TABLE2_MAP:
		ret = __isp_set_table2_map(sd, (struct isp_table_reg_map *)arg);
		break;
#endif
	default:
		return -ENOIOCTLCMD;
	}

	return ret;
}

#if IS_ENABLED(CONFIG_COMPAT)

struct isp_table_reg_map32 {
	compat_caddr_t addr;
	unsigned int size;
};

#define VIDIOC_VIN_ISP_LOAD_REG32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 70, struct isp_table_reg_map32)

#define VIDIOC_VIN_ISP_TABLE1_MAP32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 71, struct isp_table_reg_map32)

#define VIDIOC_VIN_ISP_TABLE2_MAP32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 72, struct isp_table_reg_map32)

static int get_isp_table_reg_map32(struct isp_table_reg_map *kp,
			      struct isp_table_reg_map32 __user *up)
{
	u32 tmp;

	if (!access_ok(up, sizeof(*up)) ||
	    get_user(kp->size, &up->size) || get_user(tmp, &up->addr))
		return -EFAULT;
	kp->addr = compat_ptr(tmp);
	return 0;
}

static int put_isp_table_reg_map32(struct isp_table_reg_map *kp,
			      struct isp_table_reg_map32 __user *up)
{
	u32 tmp = (u32) ((unsigned long)kp->addr);

	if (!access_ok(up, sizeof(*up)) ||
	    put_user(kp->size, &up->size) || put_user(tmp, &up->addr))
		return -EFAULT;
	return 0;
}

static long isp_compat_ioctl32(struct v4l2_subdev *sd,
		unsigned int cmd, unsigned long arg)
{
	union {
		struct isp_table_reg_map isd;
	} karg;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;
	long err = 0;

	vin_log(VIN_LOG_ISP, "%s cmd is %d\n", __func__, cmd);

	switch (cmd) {
	case VIDIOC_VIN_ISP_LOAD_REG32:
		cmd = VIDIOC_VIN_ISP_LOAD_REG;
		break;
	case VIDIOC_VIN_ISP_TABLE1_MAP32:
		cmd = VIDIOC_VIN_ISP_TABLE1_MAP;
		break;
	case VIDIOC_VIN_ISP_TABLE2_MAP32:
		cmd = VIDIOC_VIN_ISP_TABLE2_MAP;
		break;
	}

	switch (cmd) {
	case VIDIOC_VIN_ISP_LOAD_REG:
	case VIDIOC_VIN_ISP_TABLE1_MAP:
	case VIDIOC_VIN_ISP_TABLE2_MAP:
		err = get_isp_table_reg_map32(&karg.isd, up);
		compatible_arg = 0;
		break;
	}

	if (err)
		return err;

	if (compatible_arg)
		err = sunxi_isp_subdev_ioctl(sd, cmd, up);
	else {
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0) /* after linux 5.15, set_fs/get_fs abandoned */
		mm_segment_t old_fs = get_fs();
		set_fs(KERNEL_DS);
#endif
		err = sunxi_isp_subdev_ioctl(sd, cmd, &karg);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 15, 0)
		set_fs(old_fs);
#endif
	}

	switch (cmd) {
	case VIDIOC_VIN_ISP_LOAD_REG:
	case VIDIOC_VIN_ISP_TABLE1_MAP:
	case VIDIOC_VIN_ISP_TABLE2_MAP:
		err = put_isp_table_reg_map32(&karg.isd, up);
		break;
	}

	return err;
}
#endif

/*
 * must reset all the pipeline through isp.
 */
void sunxi_isp_reset(struct isp_dev *isp)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct csi_dev *csi = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	bool flags = 1;
	int i = 0;
	bool need_ve_callback = false;

	if (!isp->use_isp)
		return;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (!isp->stream_count) {
#else
	if (!isp->subdev.entity.stream_count) {
#endif
		vin_err("isp%d is not used, cannot be resetted!!!\n", isp->id);
		return;
	}

#if defined ISP_600
	if (glb_isp[isp->id/ISP_VIRT_NUM]->work_mode == ISP_OFFLINE)
		return;
#endif

	vin_print("%s:isp%d reset!!!,ISP frame number is %d\n", __func__, isp->id, isp->isp_frame_number);

	bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
#if IS_ENABLED(CONFIG_D3D)
#if !defined ISP_600
	if ((isp->load_shadow[0x2d4 + 0x3]) & (1<<1)) {
		/* clear D3D rec_en 0x2d4 bit25 */
		isp->load_shadow[0x2d4 + 0x3] = (isp->load_shadow[0x2d4 + 0x3]) & (~(1<<1));
		memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
	}
#else /* isp600 */
	bsp_isp_clr_d3d_rec_en(isp->id);
	isp->d3d_rec_reset = 1;
#endif
#endif

	/*****************stop*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		if (vind->vinc[i]->isp_sel == isp->id) {
			vinc = vind->vinc[i];
			vinc->vid_cap.frame_delay_cnt = 1;

			if (i == 0)
				need_ve_callback = true;

			if (flags) {
				csic_prs_capture_stop(vinc->csi_sel);

#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
				if (vinc->mipi_sel == 0)
					cmb_rx_disable(vinc->mipi_sel);
#endif
				csic_prs_disable(vinc->csi_sel);
				csic_isp_bridge_disable(0);
#if defined SUPPORT_ISP_TDM && defined TDM_V200
				if (isp->wdr_mode == ISP_DOL_WDR_MODE || isp->wdr_mode == ISP_3FDOL_WDR_MODE) {
					csic_tdm_disable(0);
					csic_tdm_top_disable(0);
				}
#endif
				bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
				bsp_isp_capture_stop(isp->id);
#if defined ISP_600
				bsp_isp_top_capture_stop(isp->id);
#endif
				bsp_isp_enable(isp->id, 0);
				flags = 0;
			}
#if !defined VIPP_200
			vipp_disable(vinc->vipp_sel);
			vipp_top_clk_en(vinc->vipp_sel, 0);
#else
			vipp_chn_cap_disable(vinc->vipp_sel);
			vipp_cap_disable(vinc->vipp_sel);
			vipp_clear_status(vinc->vipp_sel, VIPP_STATUS_ALL);
			vipp_top_clk_en(vinc->vipp_sel, 0);
#endif
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

		if (vind->vinc[i]->isp_sel == isp->id) {
			vinc = vind->vinc[i];

			csic_dma_top_enable(vinc->vipp_sel);
#if !defined VIPP_200
			vipp_top_clk_en(vinc->vipp_sel, 1);
			vipp_enable(vinc->vipp_sel);
#else
			vipp_clear_status(vinc->vipp_sel, VIPP_STATUS_ALL);
			vipp_cap_enable(vinc->vipp_sel);
			vipp_top_clk_en(vinc->vipp_sel, 1);
			vipp_chn_cap_enable(vinc->vipp_sel);
#endif
			vinc->vin_status.frame_cnt = 0;
			vinc->vin_status.lost_cnt = 0;

			if (flags) {
				bsp_isp_enable(isp->id, 1);
#if defined ISP_600
				bsp_isp_top_capture_start(isp->id);
#endif
				bsp_isp_set_para_ready(isp->id, PARA_READY);
				bsp_isp_capture_start(isp->id);
				isp->isp_frame_number = 0;

				csic_isp_bridge_enable(0);
#if defined SUPPORT_ISP_TDM && defined TDM_V200
				if (isp->wdr_mode == ISP_DOL_WDR_MODE || isp->wdr_mode == ISP_3FDOL_WDR_MODE) {
					csic_tdm_top_enable(0);
					csic_tdm_enable(0);
					csic_tdm_tx_enable(0);
					csic_tdm_tx_cap_enable(0);
					csic_tdm_int_clear_status(0, TDM_INT_ALL);

					csic_tdm_rx_enable(0, vinc->tdm_rx_sel);
					csic_tdm_rx_cap_enable(0, vinc->tdm_rx_sel);
					csic_tdm_rx_enable(0, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_enable(0, vinc->tdm_rx_sel + 1);
					if (isp->wdr_mode == ISP_3FDOL_WDR_MODE) {
						csic_tdm_rx_enable(0, vinc->tdm_rx_sel + 2);
						csic_tdm_rx_cap_enable(0, vinc->tdm_rx_sel + 2);
					}
				}
#endif
				csic_prs_enable(vinc->csi_sel);

#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
				if (vinc->mipi_sel == 0)
					cmb_rx_enable(vinc->mipi_sel);
#endif

				csi = container_of(vinc->vid_cap.pipe.sd[VIN_IND_CSI], struct csi_dev, subdev);
				csic_prs_capture_start(vinc->csi_sel, csi->bus_info.ch_total_num, &mode);
				flags = 0;
			}
		}
	}

	if (isp->sensor_lp_mode)
		schedule_work(&isp->s_sensor_stby_task);
	if (need_ve_callback) {
		vinc = vind->vinc[0];
		if (vinc->ve_online_cfg.ve_online_en && vinc->vid_cap.online_csi_reset_callback)
			vinc->vid_cap.online_csi_reset_callback(vinc->id);
	}
}
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
static void __isp_rpmsg_send_handle(struct work_struct *work)
{
	struct isp_dev *isp = container_of(work, struct isp_dev, isp_rpmsg_send_task);
	static short isp_log_param;
	unsigned int data[5];
	bool send_event = 0;

	if (!isp->subdev.entity.stream_count) {
		vin_err("isp%d is not used, cannot handle run server!!!\n", isp->id);
		return;
	}

	if (isp->h3a_stat.state != ISPSTAT_ENABLED)
		return;

	data[1] = 1;/*load type (0: load separate; 1: load together)*/
	switch (isp->mf.colorspace) {
	case V4L2_COLORSPACE_REC709:
		data[2] = 1;
		break;
	case V4L2_COLORSPACE_BT2020:
		data[2] = 2;
		break;
	case V4L2_COLORSPACE_REC709_PART_RANGE:
		data[2] = 4;
		break;
	default:
		data[2] = 0;
		break;
	}
	if (isp_log_param != (vin_log_mask >> 16)) {
		isp_log_param = vin_log_mask >> 16;
		send_event = 1;
	} else {
		send_event = 0;
	}
	data[3] = isp_log_param;
	data[4] = isp_log_param >> 8;
	if ((isp->h3a_stat.frame_number < 2) || send_event) {
		data[0] = VIN_SET_FRAME_SYNC;
		isp_rpmsg_send(isp, data, 5*4);
	}

	isp_stat_request_statistics(&isp->h3a_stat);
}
#endif
#if defined ISP_600
static void __sunxi_isp_reset_v2(struct isp_dev *isp)
{
	if (!isp->use_isp)
		return;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (!isp->stream_count) {
#else
	if (!isp->subdev.entity.stream_count) {
#endif
		vin_err("isp%d is not used, cannot be resetted!!!\n", isp->id);
		return;
	}

	vin_print("%s:isp%d reset!!!,ISP frame number is %d\n", __func__, isp->id, isp->isp_frame_number);

	isp->d3d_rec_reset = 1;
}

static void __sunxi_isp_reset_v3(struct isp_dev *isp)
{
	struct vin_md *vind = dev_get_drvdata(isp->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	struct csi_dev *csi = NULL;
	struct prs_cap_mode mode = {.mode = VCAP};
	bool flags = 1;
	int i = 0;
	bool need_ve_callback = false;

	if (!isp->use_isp)
		return;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (!isp->stream_count) {
#else
	if (!isp->subdev.entity.stream_count) {
#endif
		vin_err("isp%d is not used, cannot be resetted!!!\n", isp->id);
		return;
	}

	if (glb_isp[isp->id/ISP_VIRT_NUM]->work_mode == ISP_ONLINE)
		return;

	vin_print("%s:isp%d reset!!!,ISP frame number is %d\n", __func__, isp->id, isp->isp_frame_number);

	bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
#if IS_ENABLED(CONFIG_D3D)
	bsp_isp_clr_d3d_rec_en(isp->id);
	isp->d3d_rec_reset = 1;
#endif

	/*****************stop*******************/
	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;

		if (vind->vinc[i]->isp_sel == isp->id) {
			vinc = vind->vinc[i];
			vinc->vid_cap.frame_delay_cnt = 1;

			if (i == 0)
				need_ve_callback = true;

			if (flags) {
				csic_prs_capture_stop(vinc->csi_sel);

				csic_prs_disable(vinc->csi_sel);
				csic_isp_bridge_disable(0);
#if defined SUPPORT_ISP_TDM
				if (isp->wdr_mode == ISP_DOL_WDR_MODE || isp->wdr_mode == ISP_3FDOL_WDR_MODE) {
					csic_tdm_rx_disable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel);
					csic_tdm_rx_cap_disable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel);
					csic_tdm_rx_disable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_disable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel + 1);
					if (isp->wdr_mode == ISP_3FDOL_WDR_MODE) {
						csic_tdm_rx_disable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel + 2);
						csic_tdm_rx_cap_disable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel + 2);
					}
				} else {
					if (vinc->tdm_rx_sel != 0xff) {
						csic_tdm_rx_disable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel);
						csic_tdm_rx_cap_disable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel);
					}
				}
#endif
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

		if (vind->vinc[i]->isp_sel == isp->id) {
			vinc = vind->vinc[i];

			//csic_dma_top_enable(vinc->vipp_sel);

			vipp_chn_cap_enable(vinc->vipp_sel);
			vinc->vin_status.frame_cnt = 0;
			vinc->vin_status.lost_cnt = 0;
#if IS_ENABLED(CONFIG_TDM_ONE_BUFFER_WITH_TWORX)
			vinc->vid_cap.frame_delay_cnt = 2;
#endif

			if (flags) {
				bsp_isp_set_para_ready(isp->id, PARA_READY);
				bsp_isp_capture_start(isp->id);
				isp->isp_frame_number = 0;

				csic_isp_bridge_enable(0);

#if defined SUPPORT_ISP_TDM
				if (isp->wdr_mode == ISP_DOL_WDR_MODE || isp->wdr_mode == ISP_3FDOL_WDR_MODE) {
					csic_tdm_rx_enable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel);
					csic_tdm_rx_cap_enable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel);
					csic_tdm_rx_enable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel + 1);
					csic_tdm_rx_cap_enable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel + 1);
					if (isp->wdr_mode == ISP_3FDOL_WDR_MODE) {
						csic_tdm_rx_enable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel + 2);
						csic_tdm_rx_cap_enable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel + 2);
					}
				} else {
					if (vinc->tdm_rx_sel != 0xff) {
						csic_tdm_rx_enable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel);
						csic_tdm_rx_cap_enable(vinc->tdm_rx_sel/TDM_RX_NUM, vinc->tdm_rx_sel);
					}
				}
#endif
				csic_prs_enable(vinc->csi_sel);
				csi = container_of(vinc->vid_cap.pipe.sd[VIN_IND_CSI], struct csi_dev, subdev);
				csic_prs_capture_start(vinc->csi_sel, csi->bus_info.ch_total_num, &mode);
				flags = 0;
			}
		}
	}
}
#endif
void sunxi_isp_frame_sync_isr(struct v4l2_subdev *sd)
{
#if !defined CONFIG_ISP_SERVER_MELIS
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	struct v4l2_event event;
	static short isp_log_param;
	bool send_event = 0;

	memset(&event, 0, sizeof(event));
	event.type = V4L2_EVENT_FRAME_SYNC;
	event.id = 0;
	event.u.data[0] = 1;/* load type (0: load seperate; 1: load together) */
	switch (isp->mf.colorspace) {
	case V4L2_COLORSPACE_REC709:
		event.u.data[1] = 1;
		break;
	case V4L2_COLORSPACE_BT2020:
		event.u.data[1] = 2;
		break;
	default:
		event.u.data[1] = 0;
		break;
	}
	if (isp_log_param != (vin_log_mask >> 16)) {
		isp_log_param = vin_log_mask >> 16;
		send_event = 1;
	} else {
		send_event = 0;
	}
	event.u.data[2] = isp_log_param;
	event.u.data[3] = isp_log_param >> 8;
	if ((isp->h3a_stat.frame_number < 2) || send_event)
		v4l2_event_queue(isp->subdev.devnode, &event);

	isp_stat_isr(&isp->h3a_stat);
#else /* CONFIG_ISP_SERVER_MELIS */
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	schedule_work(&isp->isp_rpmsg_send_task);
	isp_stat_isr(&isp->h3a_stat);
#endif
}

#if !defined CONFIG_ISP_SERVER_MELIS
int sunxi_isp_subscribe_event(struct v4l2_subdev *sd,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub)
{
	vin_log(VIN_LOG_ISP, "%s id = %d\n", __func__, sub->id);
	if (sub->type == V4L2_EVENT_CTRL)
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	else
		return v4l2_event_subscribe(fh, sub, 1, NULL);
}
#endif
static const struct v4l2_subdev_core_ops sunxi_isp_subdev_core_ops = {
	.init = sunxi_isp_subdev_init,
	.ioctl = sunxi_isp_subdev_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = isp_compat_ioctl32,
#endif
#if !defined CONFIG_ISP_SERVER_MELIS
	.subscribe_event = sunxi_isp_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
#endif
};

static const struct v4l2_subdev_video_ops sunxi_isp_subdev_video_ops = {
	.s_stream = sunxi_isp_subdev_s_stream,
};

static const struct v4l2_subdev_pad_ops sunxi_isp_subdev_pad_ops = {
	.get_fmt = sunxi_isp_subdev_get_fmt,
	.set_fmt = sunxi_isp_subdev_set_fmt,
};

static struct v4l2_subdev_ops sunxi_isp_subdev_ops = {
	.core = &sunxi_isp_subdev_core_ops,
	.video = &sunxi_isp_subdev_video_ops,
	.pad = &sunxi_isp_subdev_pad_ops,
};

static int __sunxi_isp_ctrl(struct isp_dev *isp, struct v4l2_ctrl *ctrl)
{
	int ret = 0;
	__maybe_unused int data[3];

	if (ctrl->flags & V4L2_CTRL_FLAG_INACTIVE)
		return 0;

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
	case V4L2_CID_FLASH_LED_MODE:
	case V4L2_CID_AUTO_FOCUS_INIT:
	case V4L2_CID_AUTO_FOCUS_RELEASE:
	case V4L2_CID_FLASH_LED_MODE_V1:
	case V4L2_CID_TAKE_PICTURE:
		break;
	default:
		break;
	}
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	if (isp->h3a_stat.state == ISPSTAT_ENABLED) {
		data[0] = VIN_SET_V4L2_IOCTL;
		data[1] = ctrl->id;
		data[2] = ctrl->val;
		isp_rpmsg_send(isp, data, 3*4);
	}
#endif
	return ret;
}

#define ctrl_to_sunxi_isp(ctrl) \
	container_of(ctrl->handler, struct isp_dev, ctrls.handler)

static int sunxi_isp_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct isp_dev *isp = ctrl_to_sunxi_isp(ctrl);
	unsigned long flags;
	int ret;

	vin_log(VIN_LOG_ISP, "%s, val = %d, cur.val = %d\n",
		v4l2_ctrl_get_name(ctrl->id), ctrl->val, ctrl->cur.val);
	spin_lock_irqsave(&isp->slock, flags);
	ret = __sunxi_isp_ctrl(isp, ctrl);
	spin_unlock_irqrestore(&isp->slock, flags);

	return ret;
}

static int sunxi_isp_try_ctrl(struct v4l2_ctrl *ctrl)
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

static const struct v4l2_ctrl_ops sunxi_isp_ctrl_ops = {
	.s_ctrl = sunxi_isp_s_ctrl,
	.try_ctrl = sunxi_isp_try_ctrl,
};

static const struct v4l2_ctrl_config ae_win_ctrls[] = {
	{
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AE_WIN_Y2,
		.name = "R GAIN",
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
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y1,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_X2,
		.name = "R GAIN",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 32,
		.max = 3264,
		.step = 16,
		.def = 256,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AF_WIN_Y2,
		.name = "R GAIN",
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
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_FOCUS_LENGTH,
		.name = "Focus Length",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 1000,
		.step = 1,
		.def = 280,
		.flags = V4L2_CTRL_FLAG_VOLATILE,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AUTO_FOCUS_INIT,
		.name = "AutoFocus Initial",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_AUTO_FOCUS_RELEASE,
		.name = "AutoFocus Release",
		.type = V4L2_CTRL_TYPE_BUTTON,
		.min = 0,
		.max = 0,
		.step = 0,
		.def = 0,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
		.id = V4L2_CID_TAKE_PICTURE,
		.name = "Take Picture",
		.type = V4L2_CTRL_TYPE_INTEGER,
		.min = 0,
		.max = 16,
		.step = 1,
		.def = 0,
	}, {
		.ops = &sunxi_isp_ctrl_ops,
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

int __isp_init_subdev(struct isp_dev *isp)
{
	struct v4l2_ctrl_handler *handler = &isp->ctrls.handler;
	struct v4l2_subdev *sd = &isp->subdev;
	struct sunxi_isp_ctrls *ctrls = &isp->ctrls;
	struct v4l2_ctrl *ctrl;
	int i, ret;

	mutex_init(&isp->subdev_lock);
	v4l2_subdev_init(sd, &sunxi_isp_subdev_ops);
	sd->grp_id = VIN_GRP_ID_ISP;
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_isp.%u", isp->id);
	v4l2_set_subdevdata(sd, isp);

	v4l2_ctrl_handler_init(handler, 38 + ARRAY_SIZE(ae_win_ctrls)
		+ ARRAY_SIZE(af_win_ctrls) + ARRAY_SIZE(custom_ctrls));

	for (i = 0; i < ARRAY_SIZE(ae_win_ctrls); i++)
		ctrls->ae_win[i] = v4l2_ctrl_new_custom(handler,
						&ae_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(ae_win_ctrls), &ctrls->ae_win[0]);

	for (i = 0; i < ARRAY_SIZE(af_win_ctrls); i++)
		ctrls->af_win[i] = v4l2_ctrl_new_custom(handler,
						&af_win_ctrls[i], NULL);
	v4l2_ctrl_cluster(ARRAY_SIZE(af_win_ctrls), &ctrls->af_win[0]);

	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_BRIGHTNESS, -128, 128, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_CONTRAST, -128, 128, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_SATURATION, -256, 512, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_HUE, -180, 180, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_WHITE_BALANCE, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_EXPOSURE, 1, 65536 * 16, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTOGAIN, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_GAIN, 16, 6000 * 16, 1, 16);

	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops,
			       V4L2_CID_POWER_LINE_FREQUENCY,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO, 0,
			       V4L2_CID_POWER_LINE_FREQUENCY_AUTO);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_HUE_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops,
			  V4L2_CID_WHITE_BALANCE_TEMPERATURE, 2800, 10000, 1, 6500);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_SHARPNESS, 0, 1000, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_CHROMA_AGC, 0, 1, 1, 1);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_COLORFX,
			       V4L2_COLORFX_SET_CBCR, 0, V4L2_COLORFX_NONE);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTOBRIGHTNESS, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_BAND_STOP_FILTER, 0, 1, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_ILLUMINATORS_1, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_ILLUMINATORS_2, 0, 1, 1, 0);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_EXPOSURE_AUTO,
			       V4L2_EXPOSURE_APERTURE_PRIORITY, 0,
			       V4L2_EXPOSURE_AUTO);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_EXPOSURE_ABSOLUTE, 1, 30 * 1000000, 1, 1);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_EXPOSURE_AUTO_PRIORITY, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_FOCUS_ABSOLUTE, 0, 127, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_FOCUS_RELATIVE, -127, 127, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_FOCUS_AUTO, 0, 1, 1, 1);
	v4l2_ctrl_new_int_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_EXPOSURE_BIAS,
			       ARRAY_SIZE(exp_bias_qmenu) - 1,
			       ARRAY_SIZE(exp_bias_qmenu) / 2, exp_bias_qmenu);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops,
			       V4L2_CID_AUTO_N_PRESET_WHITE_BALANCE,
			       V4L2_WHITE_BALANCE_SHADE, 0,
			       V4L2_WHITE_BALANCE_AUTO);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_WIDE_DYNAMIC_RANGE, 0, 1, 1, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_IMAGE_STABILIZATION, 0, 1, 1, 0);
	v4l2_ctrl_new_int_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_ISO_SENSITIVITY,
			       ARRAY_SIZE(iso_qmenu) - 1,
			       ARRAY_SIZE(iso_qmenu) / 2 - 1, iso_qmenu);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops,
			       V4L2_CID_ISO_SENSITIVITY_AUTO,
			       V4L2_ISO_SENSITIVITY_AUTO, 0,
			       V4L2_ISO_SENSITIVITY_AUTO);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops,
			       V4L2_CID_EXPOSURE_METERING,
			       V4L2_EXPOSURE_METERING_MATRIX, 0,
			       V4L2_EXPOSURE_METERING_AVERAGE);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_SCENE_MODE,
			       V4L2_SCENE_MODE_TEXT, 0, V4L2_SCENE_MODE_NONE);
	ctrl = v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_3A_LOCK, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_FOCUS_START, 0, 0, 0, 0);
	v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_FOCUS_STOP, 0, 0, 0, 0);
	ctrl = v4l2_ctrl_new_std(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_FOCUS_STATUS, 0, 7, 0, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_AUTO_FOCUS_RANGE,
			       V4L2_AUTO_FOCUS_RANGE_INFINITY, 0,
			       V4L2_AUTO_FOCUS_RANGE_AUTO);
	v4l2_ctrl_new_std_menu(handler, &sunxi_isp_ctrl_ops, V4L2_CID_FLASH_LED_MODE,
			       V4L2_FLASH_LED_MODE_RED_EYE, 0,
			       V4L2_FLASH_LED_MODE_NONE);

	for (i = 0; i < ARRAY_SIZE(custom_ctrls); i++)
		v4l2_ctrl_new_custom(handler, &custom_ctrls[i], NULL);

	if (handler->error)
		return handler->error;

	/* sd->entity->ops = &isp_media_ops; */
	isp->isp_pads[ISP_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	isp->isp_pads[ISP_PAD_SOURCE_ST].flags = MEDIA_PAD_FL_SOURCE;
	isp->isp_pads[ISP_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_PROC_VIDEO_PIXEL_FORMATTER;

	ret = media_entity_pads_init(&sd->entity, ISP_PAD_NUM, isp->isp_pads);
	if (ret < 0)
		return ret;

	sd->ctrl_handler = handler;
	/* sd->internal_ops = &sunxi_isp_sd_internal_ops; */
	return 0;
}

static int isp_resource_alloc(struct isp_dev *isp)
{
	int ret = 0;
#if !defined ISP_600
#if !defined CONFIG_ARCH_SUN8IW16P1 && !defined CONFIG_ARCH_SUN8IW19P1 && !defined CONFIG_ARCH_SUN50IW10
	isp->isp_stat.size = ISP_SAVE_DRAM_SIZE + ISP_LOAD_DRAM_SIZE;
	ret = os_mem_alloc(&isp->pdev->dev, &isp->isp_stat);
	if (ret < 0) {
		vin_err("isp statistic buf requset failed!\n");
		return -ENOMEM;
	}
	isp->isp_save.dma_addr = isp->isp_stat.dma_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_save.vir_addr = isp->isp_stat.vir_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_load.dma_addr = isp->isp_save.dma_addr + ISP_SAVED_REG_SIZE;
	isp->isp_load.vir_addr = isp->isp_save.vir_addr + ISP_SAVED_REG_SIZE;
	isp->isp_lut_tbl.dma_addr = isp->isp_load.dma_addr + ISP_LOAD_REG_SIZE;
	isp->isp_lut_tbl.vir_addr = isp->isp_load.vir_addr + ISP_LOAD_REG_SIZE;
	isp->isp_drc_tbl.dma_addr = isp->isp_lut_tbl.dma_addr + ISP_TABLE_MAPPING1_SIZE;
	isp->isp_drc_tbl.vir_addr = isp->isp_lut_tbl.vir_addr + ISP_TABLE_MAPPING1_SIZE;
#else
	isp->isp_stat.size = ISP_SAVE_DRAM_SIZE + ISP_LOAD_DRAM_SIZE;
	ret = os_mem_alloc(&isp->pdev->dev, &isp->isp_stat);
	if (ret < 0) {
		vin_err("isp statistic buf requset failed!\n");
		return -ENOMEM;
	}
	isp->isp_load.dma_addr = isp->isp_stat.dma_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_load.vir_addr = isp->isp_stat.vir_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_save.dma_addr = isp->isp_stat.dma_addr;
	isp->isp_save.vir_addr = isp->isp_stat.vir_addr;
#endif
#else /* else ISP_600 */
	isp->isp_stat.size = ISP_SAVE_DRAM_SIZE + ISP_SAVE_LOAD_DRAM_SIZE + 3 * ISP_LOAD_DRAM_SIZE;
	ret = os_mem_alloc(&isp->pdev->dev, &isp->isp_stat);
	if (ret < 0) {
		vin_err("isp statistic buf requset failed!\n");
		return -ENOMEM;
	}
	isp->isp_save.dma_addr = isp->isp_stat.dma_addr;
	isp->isp_save.vir_addr = isp->isp_stat.vir_addr;
	isp->isp_save_load.dma_addr = isp->isp_stat.dma_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_save_load.vir_addr = isp->isp_stat.vir_addr + ISP_STAT_TOTAL_SIZE;
	isp->isp_load.dma_addr = isp->isp_stat.dma_addr + ISP_STAT_TOTAL_SIZE + ISP_SAVE_LOAD_DRAM_SIZE;
	isp->isp_load.vir_addr = isp->isp_stat.vir_addr + ISP_STAT_TOTAL_SIZE + ISP_SAVE_LOAD_DRAM_SIZE;
	isp->load_para[0].dma_addr = isp->isp_load.dma_addr + ISP_LOAD_DRAM_SIZE;
	isp->load_para[0].vir_addr = isp->isp_load.vir_addr + ISP_LOAD_DRAM_SIZE;
	isp->load_para[1].dma_addr = isp->isp_load.dma_addr + ISP_LOAD_DRAM_SIZE * 2;
	isp->load_para[1].vir_addr = isp->isp_load.vir_addr + ISP_LOAD_DRAM_SIZE * 2;
#endif
	return ret;
}

static void isp_resource_free(struct isp_dev *isp)
{
	os_mem_free(&isp->pdev->dev, &isp->isp_stat);
}

static irqreturn_t isp_isr(int irq, void *priv)
{
	struct isp_dev *isp = (struct isp_dev *)priv;
	unsigned int load_val;
	unsigned long flags;

	if (!isp->use_isp)
		return 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (isp->stream_count == 0) {
#else
	if (isp->subdev.entity.stream_count == 0) {
#endif
		bsp_isp_clr_irq_status(isp->id, ISP_IRQ_EN_ALL);
		return IRQ_HANDLED;
	}

	vin_log(VIN_LOG_ISP, "isp%d interrupt, status is 0x%x!!!\n", isp->id,
		bsp_isp_get_irq_status(isp->id, ISP_IRQ_STATUS_ALL));

	spin_lock_irqsave(&isp->slock, flags);
#if !defined ISP_600
#if !defined CONFIG_ARCH_SUN8IW16P1 && !defined CONFIG_ARCH_SUN8IW19P1 && !defined CONFIG_ARCH_SUN50IW10
	if (bsp_isp_get_irq_status(isp->id, SRC0_FIFO_OF_PD)) {
		vin_err("isp%d source0 fifo overflow\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, SRC0_FIFO_OF_PD);
		if (bsp_isp_get_irq_status(isp->id, CIN_FIFO_OF_PD)) {
			vin_err("isp%d Cin0 fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, CIN_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, DPC_FIFO_OF_PD)) {
			vin_err("isp%d DPC fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, DPC_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D2D_FIFO_OF_PD)) {
			vin_err("isp%d D2D fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D2D_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, BIS_FIFO_OF_PD)) {
			vin_err("isp%d BIS fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, BIS_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, CNR_FIFO_OF_PD)) {
			vin_err("isp%d CNR fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, CNR_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, PLTM_FIFO_OF_PD)) {
			vin_err("isp%d PLTM fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, PLTM_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D3D_WRITE_FIFO_OF_PD)) {
			vin_err("isp%d D3D cmp write to DDR fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D3D_WRITE_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D3D_READ_FIFO_OF_PD)) {
			vin_err("isp%d D3D umcmp read from DDR fifo empty\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D3D_READ_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D3D_WT2CMP_FIFO_OF_PD)) {
			vin_err("isp%d D3D write to cmp fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D3D_WT2CMP_FIFO_OF_PD);
		}

		if (bsp_isp_get_irq_status(isp->id, WDR_WRITE_FIFO_OF_PD)) {
			vin_err("isp%d WDR cmp write to DDR fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, WDR_WRITE_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, WDR_READ_FIFO_OF_PD)) {
			vin_err("isp%d WDR umcmp read from DDR fifo empty\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, WDR_READ_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, WDR_WT2CMP_FIFO_OF_PD)) {
			vin_err("isp%d WDR write to cmp fifo overflow\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, WDR_WT2CMP_FIFO_OF_PD);
		}
		if (bsp_isp_get_irq_status(isp->id, D3D_HB_PD)) {
			vin_err("isp%d Hblanking is not enough for D3D\n", isp->id);
			bsp_isp_clr_irq_status(isp->id, D3D_HB_PD);
		}
		/* isp reset */
		sunxi_isp_reset(isp);
	}
#else
	if (bsp_isp_get_irq_status(isp->id, S0_FIFO_OF_PD)) {
		vin_err("isp%d source0 fifo overflow\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, S0_FIFO_OF_PD);
		if (bsp_isp_get_internal_status0(isp->id, S0_CIN_FIFO_OF_PD)) {
			vin_err("isp%d Cin0 fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, S0_CIN_FIFO_OF_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, BIS_FIFO_OF_PD)) {
			vin_err("isp%d BIS fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, BIS_FIFO_OF_PD);
		}
#if !defined CONFIG_ARCH_SUN50IW10
		if (bsp_isp_get_internal_status0(isp->id, DPC_FIFO_OF_PD)) {
			vin_err("isp%d DPC fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, DPC_FIFO_OF_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, CNR_FIFO_OF_PD)) {
			vin_err("isp%d CNR fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, CNR_FIFO_OF_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, PLTM_FIFO_OF_PD)) {
			vin_err("isp%d PLTM fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, PLTM_FIFO_OF_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, D3D_WRITE_FIFO_OF_PD)) {
			vin_err("isp%d D3D cmp write to DDR fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, D3D_WRITE_FIFO_OF_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, D3D_READ_FIFO_OF_PD)) {
			vin_err("isp%d D3D umcmp read from DDR fifo empty\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, D3D_READ_FIFO_OF_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDR_WRITE_FIFO_OF_PD)) {
			vin_err("isp%d WDR cmp write to DDR fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDR_WRITE_FIFO_OF_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDR_READ_FIFO_OF_PD)) {
			vin_err("isp%d WDR umcmp read from DDR fifo empty\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDR_READ_FIFO_OF_PD);
		}
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
		if (bsp_isp_get_internal_status0(isp->id, LCA_RGB_FIFO_R_EMP_PD)) {
			vin_err("isp%d RGB fifo of LCA read empty\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, LCA_RGB_FIFO_R_EMP_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, LCA_RGB_FIFO_W_FULL_PD)) {
			vin_err("isp%d RGB fifo of LCA write full\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, LCA_RGB_FIFO_W_FULL_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, LCA_BY_FIFO_R_EMP_PD)) {
			vin_err("isp%d bayer fifo of LCA read empty\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, LCA_BY_FIFO_R_EMP_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, LCA_BY_FIFO_W_FULL_PD)) {
			vin_err("isp%d bayer fifo of LCA write full\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, LCA_BY_FIFO_W_FULL_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, D3D_K_FIFO_W_FULL_PD)) {
			vin_err("isp%d write fifo of D3D K data full\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, D3D_K_FIFO_W_FULL_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, D3D_RAW_FIFO_W_FULL_PD)) {
			vin_err("isp%d write fifo of D3D RAW data full\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, D3D_RAW_FIFO_W_FULL_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, D3D_K_FIFO_R_EMP_PD)) {
			vin_err("isp%d read fifo of D3D K data empty\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, D3D_K_FIFO_R_EMP_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, D3D_REF_FIFO_R_EMP_PD)) {
			vin_err("isp%d read fifo of D3D REF data empty\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, D3D_REF_FIFO_R_EMP_PD);
		}
		if (bsp_isp_get_internal_status0(isp->id, D3D_LTF_FIFO_R_EMP_PD)) {
			vin_err("isp%d read fifo of D3D LTF data empty\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, D3D_LTF_FIFO_R_EMP_PD);
		}
#endif
		/* isp reset */
		sunxi_isp_reset(isp);
	}
#endif
	if (bsp_isp_get_irq_status(isp->id, HB_SHORT_PD)) {
		vin_err("isp%d Hblanking is short (less than 96 cycles)\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, HB_SHORT_PD);
	}

	if (bsp_isp_get_irq_status(isp->id, FRAME_ERROR_PD)) {
		bsp_isp_get_s0_ch_fmerr_cnt(isp->id, &isp->err_size);
		bsp_isp_get_s0_ch_hb_cnt(isp->id, &isp->hb_max, &isp->hb_min);
		vin_err("isp%d frame error, size %d %d, hblank max %d min %d!!\n", isp->id,
			isp->err_size.width, isp->err_size.height, isp->hb_max, isp->hb_min);
		bsp_isp_clr_irq_status(isp->id, FRAME_ERROR_PD);
		sunxi_isp_reset(isp);
	}

	if (bsp_isp_get_irq_status(isp->id, FRAME_LOST_PD)) {
		vin_err("isp%d frame lost\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, FRAME_LOST_PD);
		sunxi_isp_reset(isp);
	}

#if IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
	if (bsp_isp_get_irq_status(isp->id, S0_BTYPE_ERROR_PD)) {
		vin_err("isp%d input btype error\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, S0_BTYPE_ERROR_PD);
	}

	if (bsp_isp_get_irq_status(isp->id, ADDR_ERROR_PD)) {
		vin_err("isp%d aligned addr error\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, ADDR_ERROR_PD);
	}

	if (bsp_isp_get_irq_status(isp->id, LBC_ERROR_PD)) {
		vin_err("isp%d LBC de-compress error\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, LBC_ERROR_PD);
		if (bsp_isp_get_lbc_internal_status(isp->id, WDR_LBC_DEC_ERR_PD)) {
			vin_err("isp%d WDR LBC decode error\n", isp->id);
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_MSQ_DEC_ERR_PD << WDR_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d msq decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_MSQ_DEC_ERR_PD << WDR_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_DTS_DEC_ERR_PD << WDR_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d dts decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_DTS_DEC_ERR_PD << WDR_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_QP_DEC_ERR_PD << WDR_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d qp decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_QP_DEC_ERR_PD << WDR_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_CODEC_BIT_LOST_PD << WDR_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d codec bit lost error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_CODEC_BIT_LOST_PD << WDR_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_CODEC_RED_ERR_PD << WDR_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d codec redundancy error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_CODEC_RED_ERR_PD << WDR_LBC_DEC_ERR_OFF);
			}
		}
		if (bsp_isp_get_lbc_internal_status(isp->id, D3D_K_LBC_DEC_ERR_PD)) {
			vin_err("isp%d D3D K LBC decode error\n", isp->id);
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_MSQ_DEC_ERR_PD << D3D_K_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d msq decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_MSQ_DEC_ERR_PD << D3D_K_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_DTS_DEC_ERR_PD << D3D_K_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d dts decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_DTS_DEC_ERR_PD << D3D_K_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_QP_DEC_ERR_PD << D3D_K_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d qp decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_QP_DEC_ERR_PD << D3D_K_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_CODEC_BIT_LOST_PD << D3D_K_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d codec bit lost error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_CODEC_BIT_LOST_PD << D3D_K_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_CODEC_RED_ERR_PD << D3D_K_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d codec redundancy error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_CODEC_RED_ERR_PD << D3D_K_LBC_DEC_ERR_OFF);
			}
		}
		if (bsp_isp_get_lbc_internal_status(isp->id, D3D_REF_LBC_DEC_ERR_PD)) {
			vin_err("isp%d D3D reference frame LBC decode error\n", isp->id);
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_MSQ_DEC_ERR_PD << D3D_REF_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d msq decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_MSQ_DEC_ERR_PD << D3D_REF_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_DTS_DEC_ERR_PD << D3D_REF_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d dts decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_DTS_DEC_ERR_PD << D3D_REF_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_QP_DEC_ERR_PD << D3D_REF_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d qp decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_QP_DEC_ERR_PD << D3D_REF_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_CODEC_BIT_LOST_PD << D3D_REF_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d codec bit lost error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_CODEC_BIT_LOST_PD << D3D_REF_LBC_DEC_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_CODEC_RED_ERR_PD << D3D_REF_LBC_DEC_ERR_OFF)) {
				vin_err("isp%d codec redundancy error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_CODEC_RED_ERR_PD << D3D_REF_LBC_DEC_ERR_OFF);
			}
		}
		if (bsp_isp_get_lbc_internal_status(isp->id, D3D_LTF_LBC_DEV_ERR_PD)) {
			vin_err("isp%d D3D long time reference frame LBC decode error\n", isp->id);
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_MSQ_DEC_ERR_PD << D3D_LTF_LBC_DEV_ERR_OFF)) {
				vin_err("isp%d msq decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_MSQ_DEC_ERR_PD << D3D_LTF_LBC_DEV_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_DTS_DEC_ERR_PD << D3D_LTF_LBC_DEV_ERR_OFF)) {
				vin_err("isp%d dts decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_DTS_DEC_ERR_PD << D3D_LTF_LBC_DEV_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_QP_DEC_ERR_PD << D3D_LTF_LBC_DEV_ERR_OFF)) {
				vin_err("isp%d qp decode error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_QP_DEC_ERR_PD << D3D_LTF_LBC_DEV_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_CODEC_BIT_LOST_PD << D3D_LTF_LBC_DEV_ERR_OFF)) {
				vin_err("isp%d codec bit lost error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_CODEC_BIT_LOST_PD << D3D_LTF_LBC_DEV_ERR_OFF);
			}
			if (bsp_isp_get_lbc_internal_status(isp->id, LBC_CODEC_RED_ERR_PD << D3D_LTF_LBC_DEV_ERR_OFF)) {
				vin_err("isp%d codec redundancy error\n", isp->id);
				bsp_isp_clr_lbc_internal_status(isp->id, LBC_CODEC_RED_ERR_PD << D3D_LTF_LBC_DEV_ERR_OFF);
			}
		}
	}
#endif

	if (bsp_isp_get_irq_status(isp->id, PARA_LOAD_PD)) {
		bsp_isp_clr_irq_status(isp->id, PARA_LOAD_PD);
		// if (isp->ptn_type) {
		// 	spin_unlock_irqrestore(&isp->slock, flags);
		// 	return IRQ_HANDLED;
		// }
		bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
		if (isp->load_flag)
			memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
		isp->isp_ob.set_cnt++;
		load_val = bsp_isp_load_update_flag(isp->id);

		if (isp->wdr_mode == ISP_DOL_WDR_MODE)
			isp_wdr_pingpong_set(isp);
		else if (isp->wdr_mode == ISP_NORMAL_MODE) {
			vin_log(VIN_LOG_ISP, "please close wdr in normal mode!!\n");
			load_val = load_val & ~WDR_UPDATE;
			bsp_isp_module_disable(isp->id, WDR_EN);
#if !defined CONFIG_ARCH_SUN8IW16P1 && !defined CONFIG_ARCH_SUN8IW19P1 && !defined CONFIG_ARCH_SUN50IW10
			bsp_isp_set_wdr_mode(isp->id, ISP_NORMAL_MODE);
#endif
		}
		if (isp->large_image == 2) {
			bsp_isp_module_disable(isp->id, PLTM_EN | D3D_EN | AE_EN | AWB_EN | AF_EN | HIST_EN);
			if (isp->left_right == 1) {
				isp->left_right = 0;
				isp->isp_ob.ob_start.hor -= isp->isp_ob.ob_valid.width - LARGE_IMAGE_OFF * 2;
			} else {
				isp->left_right = 1;
				isp->isp_ob.ob_start.hor += isp->isp_ob.ob_valid.width - LARGE_IMAGE_OFF * 2;
			}
		}
#if !defined CONFIG_D3D
		bsp_isp_module_disable(isp->id, D3D_EN);
		load_val = load_val & ~D3D_UPDATE;
#endif
		bsp_isp_set_size(isp->id, &isp->isp_ob);
		bsp_isp_update_table(isp->id, (unsigned short)load_val);
#if IS_ENABLED(CONFIG_ARCH_SUN8IW12P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW17P1) || IS_ENABLED(CONFIG_ARCH_SUN8IW19P1)
		isp_3d_pingpong_update(isp);
#endif
		bsp_isp_set_para_ready(isp->id, PARA_READY);
		if (isp->ptn_cfg && isp->ptn_cfg->ptn_en) {
			if (isp->ptn_cfg->ptn_type)
				csic_ptn_addr(0, (unsigned long)(isp->ptn_cfg->ptn_buf.dma_addr + (isp->ptn_isp_cnt % isp->ptn_cfg->ptn_cnt) * isp->ptn_cfg->ptn_size));
			if (isp->ptn_isp_cnt > 0)
				csic_ptn_generation_en(0, 1);
			isp->ptn_isp_cnt++;
		}
	}
	spin_unlock_irqrestore(&isp->slock, flags);

	if (bsp_isp_get_irq_status(isp->id, FINISH_PD)) {
		isp->isp_frame_number++;
		bsp_isp_clr_irq_status(isp->id, FINISH_PD);
#ifdef SUPPORT_PTN
		if (isp->ptn_type) {
			bsp_isp_set_para_ready(isp->id, PARA_NOT_READY);
			memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0] + (isp->ptn_isp_cnt%3) * ISP_LOAD_DRAM_SIZE, ISP_LOAD_DRAM_SIZE);
			isp->ptn_isp_cnt++;
			load_val = bsp_isp_load_update_flag(isp->id);
			bsp_isp_set_size(isp->id, &isp->isp_ob);
			bsp_isp_update_table(isp->id, (unsigned short)load_val);
			isp_3d_pingpong_update(isp);
			bsp_isp_set_para_ready(isp->id, PARA_READY);
		}
#endif
		if (!isp->f1_after_librun) {
			sunxi_isp_frame_sync_isr(&isp->subdev);
			if (isp->h3a_stat.stat_en_flag)
				isp->f1_after_librun = 1;
		} else {
			if (isp->load_flag || (isp->event_lost_cnt == 10)) {
				sunxi_isp_frame_sync_isr(&isp->subdev);
				isp->event_lost_cnt = 0;
			} else {
				isp->event_lost_cnt++;
			}
		}
		isp->load_flag = 0;
	}
#else /* else ISP_600 */
	if (bsp_isp_get_irq_status(isp->id, DDR_RW_ERROR_PD)) {
		vin_err("isp%d ddr read and write error!\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, DDR_RW_ERROR_PD);
	}
	if (bsp_isp_get_irq_status(isp->id, LBC_ERROR_PD)) {
		vin_err("isp%d lbc decompress error\n", isp->id);
		if (bsp_isp_get_internal_status1(isp->id, LBC_DEC_LONG_ERROR))
			vin_err("isp%d the data for LBC decode is too long\n", isp->id);
		if (bsp_isp_get_internal_status1(isp->id, LBC_DEC_SHORT_ERROR))
			vin_err("isp%d the data for LBC decode is too short\n", isp->id);
		if (bsp_isp_get_internal_status1(isp->id, LBC_DEC_ERROR))
			vin_err("isp%d the data for LBC decode is error\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, LBC_ERROR_PD);
		__sunxi_isp_reset_v2(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, AHB_MBUS_W_PD)) {
		vin_err("isp%d AHB and MBUS write conflict\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, AHB_MBUS_W_PD);
	}
	if (bsp_isp_get_irq_status(isp->id, HB_SHORT_PD)) {
		vin_err("isp%d hblank short, hblank need morn than 128 cycles!\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, HB_SHORT_PD);
		sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, FRAME_LOST_PD)) {
		vin_err("isp%d frame lost!\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, FRAME_LOST_PD);
		sunxi_isp_reset(isp);
		__sunxi_isp_reset_v3(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, CFG_ERROR_PD)) {
		vin_err("isp%d configuration error\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, CFG_ERROR_PD);
		if (bsp_isp_get_internal_status0(isp->id, TWO_BYTE_ERROR)) {
			vin_err("isp%d two byte input mode encountered\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, TWO_BYTE_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, FMT_CHG_ERROR)) {
			vin_err("isp%d the bayer format is changed when d3d is open\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, FMT_CHG_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, CH0_BTYPE_ERROR)) {
			vin_err("isp%d the bytpe signal of channel 0 is changed\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, CH0_BTYPE_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, CH1_BTYPE_ERROR)) {
			vin_err("isp%d the bytpe signal of channel 1 is changed\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, CH1_BTYPE_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, CH2_BTYPE_ERROR)) {
			vin_err("isp%d the bytpe signal of channel 2 is changed\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, CH2_BTYPE_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, DVLD_ERROR)) {
			vin_err("isp%d the dvld signal is not aligned among different input channels\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, DVLD_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, HSYNC_ERROR)) {
			vin_err("isp%d the input hsync signal is not matched with the input channel cfg\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, HSYNC_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, VSYNC_ERROR)) {
			vin_err("isp%d the input vsync signal is not matched with the input channel cfg\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, VSYNC_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, WIDTH_ERROR)) {
			vin_err("isp%d width error\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WIDTH_ERROR);
		}
		if (bsp_isp_get_internal_status0(isp->id, HEIGHT_ERROR)) {
			vin_err("isp%d height error\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, HEIGHT_ERROR);
		}
		sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, INTER_FIFO_FULL_PD)) {
		vin_err("isp%d internal fifo full\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, INTER_FIFO_FULL_PD);
		if (bsp_isp_get_internal_status2(isp->id, LCA_BYR_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of LCA_BYR_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, LCA_BYR_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, LCA_BYR_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of LCA_BYR_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, LCA_BYR_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, LCA_RGB0_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of LCA_RGB0_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, LCA_RGB0_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, LCA_RGB0_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of LCA_RGB0_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, LCA_RGB0_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_AVG_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of DMSC_AVG_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_AVG_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_AVG_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of DMSC_AVG_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_AVG_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_RGB_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of DMSC_RGB_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_RGB_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_RGB_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of DMSC_RGB_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_RGB_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_RATIO_FIFO_W_FULL)) {
			vin_err("isp%d the write full flag of DMSC_RATIO_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_RATIO_FIFO_W_FULL);
		}
		if (bsp_isp_get_internal_status2(isp->id, DMSC_RATIO_FIFO_R_EMP)) {
			vin_err("isp%d the read empty flag of DMSC_RATIO_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, DMSC_RATIO_FIFO_R_EMP);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_DIFF_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_DIFF_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_DIFF_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_LP0_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_LP0_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_LP0_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_LP1_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_LP1_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_LP1_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_PCNT_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_PCNT_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_PCNT_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_KPACK_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_KPACK_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_KPACK_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_CRKB_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_CRKB_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_CRKB_FIFO_OV);
		}
		if (bsp_isp_get_internal_status2(isp->id, D3D_LBC_FIFO_OV)) {
			vin_err("isp%d the overflow flag of D3D_LBC_FIFO\n", isp->id);
			bsp_isp_clr_internal_status2(isp->id, D3D_LBC_FIFO_OV);
		}
		sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, WDMA_FIFO_FULL_PD)) {
		vin_err("isp%d wdma fifo full\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, WDMA_FIFO_FULL_PD);
		if (bsp_isp_get_internal_status0(isp->id, WDMA_PLTM_LUM_FIFO_OV)) {
			vin_err("isp%d wdma fifo of pltm lum fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_PLTM_LUM_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_PLTM_PKX_FIFO_OV)) {
			vin_err("isp%d wdma fifo of pltm ptk fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_PLTM_PKX_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_D3D_REC_FIFO_OV)) {
			vin_err("isp%d wdma fifo of d3d rec fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_D3D_REC_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_D3D_KSTB_FIFO_OV)) {
			vin_err("isp%d wdma fifo of d3d ktsb fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_D3D_KSTB_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_D3D_CNTR_FIFO_OV)) {
			vin_err("isp%d wdma fifo of d3d cntr fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_D3D_CNTR_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_D3D_CURK_FIFO_OV)) {
			vin_err("isp%d wdma fifo of d3d curk fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_D3D_CURK_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_LBC_H4DDR_FIFO_OV)) {
			vin_err("isp%d wdma fifo of lbc head for ddr output fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_LBC_H4DDR_FIFO_OV);
		}
		if (bsp_isp_get_internal_status0(isp->id, WDMA_LBC_H4DT_FIFO_OV)) {
			vin_err("isp%d wdma fifo of lbc head for data fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, WDMA_LBC_H4DT_FIFO_OV);
		}
		sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, WDMA_OVER_BND_PD)) {
		vin_err("isp%d wdma over border\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, WDMA_OVER_BND_PD);
		sunxi_isp_reset(isp);
	}
	if (bsp_isp_get_irq_status(isp->id, RDMA_FIFO_FULL_PD)) {
		vin_err("isp%d rdma fifo full\n", isp->id);
		bsp_isp_clr_irq_status(isp->id, RDMA_FIFO_FULL_PD);
		if (bsp_isp_get_internal_status0(isp->id, RDMA_PLTM_PKX_FIFO_UV)) {
			vin_err("isp%d rdma fifo of pltm pkx fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_PLTM_PKX_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_REC_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d rec fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_REC_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_KSTB_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d kstb fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_KSTB_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_CNTR_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d cntr fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_CNTR_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_CURK_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d curk fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_CURK_FIFO_UV);
		}
		if (bsp_isp_get_internal_status0(isp->id, RDMA_D3D_PREK_FIFO_UV)) {
			vin_err("isp%d rdma fifo of d3d prek fifo overflow\n", isp->id);
			bsp_isp_clr_internal_status0(isp->id, RDMA_D3D_PREK_FIFO_UV);
		}
		sunxi_isp_reset(isp);
		__sunxi_isp_reset_v2(isp);
	}
	spin_unlock_irqrestore(&isp->slock, flags);

	if (bsp_isp_get_irq_status(isp->id, PARA_SAVE_PD)) {
		isp->isp_frame_number++;
		bsp_isp_clr_irq_status(isp->id, PARA_SAVE_PD);

		if (!isp->f1_after_librun) {
			sunxi_isp_frame_sync_isr(&isp->subdev);
			if (isp->h3a_stat.stat_en_flag)
				isp->f1_after_librun = 1;
		} else {
			if (isp->save_get_flag || isp->load_save || (isp->event_lost_cnt == 10)) {
				sunxi_isp_frame_sync_isr(&isp->subdev);
				isp->event_lost_cnt = 0;
			} else {
				isp->event_lost_cnt++;
			}
		}
		isp->load_save = 0;
	}

	spin_lock_irqsave(&isp->slock, flags);
	if (bsp_isp_get_irq_status(isp->id, PARA_LOAD_PD)) {
		bsp_isp_clr_irq_status(isp->id, PARA_LOAD_PD);

		isp_stat_load_set(&isp->h3a_stat);

		if (isp->large_image == 3 && glb_isp[1]) {
			if (isp->id == 0 && isp->load_flag) {
				memcpy(glb_isp[1]->isp_load.vir_addr, &glb_isp[1]->load_shadow[0], ISP_LOAD_DRAM_SIZE);
				memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
				isp->load_flag = 0;
			}
		} else {
			if (isp->load_flag) {
				memcpy(isp->isp_load.vir_addr, &isp->load_shadow[0], ISP_LOAD_DRAM_SIZE);
				isp->load_flag = 0;
			}
		}

		if (isp->d3d_rec_reset) {
			bsp_isp_clr_d3d_rec_en(isp->id);
			isp->d3d_rec_reset = 0;
		}

		load_val = bsp_isp_load_update_flag(isp->id);
#if !defined CONFIG_D3D
		bsp_isp_module_disable(isp->id, D3D_EN);
		load_val = load_val & ~D3D_UPDATE;
#endif
		load_val = load_val | (GAMMA_UPDATE | CEM_UPDATE);
		bsp_isp_update_table(isp->id, load_val);

		/*change load ddr data*/
		sunxi_isp_set_load_ddr(isp);

		isp_3d_pingpong_update(isp);

		if (isp->load_select) {
			memcpy(isp->load_para[1].vir_addr, isp->isp_load.vir_addr, ISP_LOAD_DRAM_SIZE);
			bsp_isp_set_load_addr(isp->id, (unsigned long)isp->load_para[1].dma_addr);
			isp->load_select = false;
		} else {
			memcpy(isp->load_para[0].vir_addr, isp->isp_load.vir_addr, ISP_LOAD_DRAM_SIZE);
			bsp_isp_set_load_addr(isp->id, (unsigned long)isp->load_para[0].dma_addr);
			isp->load_select = true;
		}

		bsp_isp_set_para_ready(isp->id, PARA_READY);
		if (isp->ptn_cfg && isp->ptn_cfg->ptn_en) {
			printk("isp ptn_cfg->ptn_en work!!\n");
			if (isp->ptn_cfg->ptn_type)
				csic_ptn_addr(0, (unsigned long)(isp->ptn_cfg->ptn_buf.dma_addr + (isp->ptn_isp_cnt % isp->ptn_cfg->ptn_cnt) * isp->ptn_cfg->ptn_size));
			if (glb_isp[isp->id/ISP_VIRT_NUM]->work_mode == ISP_OFFLINE || isp->ptn_isp_cnt > 0)
				csic_ptn_generation_en(0, 1);
			isp->ptn_isp_cnt++;
		}
	}
	spin_unlock_irqrestore(&isp->slock, flags);

/*
	if (bsp_isp_get_irq_status(isp->id, FINISH_PD)) {
		bsp_isp_clr_irq_status(isp->id, FINISH_PD);
	}
	if (bsp_isp_get_irq_status(isp->id, N_LINE_START_PD)) {
		bsp_isp_clr_irq_status(isp->id, N_LINE_START_PD);
	}
	if (bsp_isp_get_irq_status(isp->id, START_PD)) {
		bsp_isp_clr_irq_status(isp->id, START_PD);
	}
*/
#endif
	return IRQ_HANDLED;
}

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
#if !IS_ENABLED(CONFIG_RV_RUN_CAR_REVERSE)
static void isp_reserve_for_yuv(unsigned int *phy_addr, unsigned int *buf_size, unsigned int *not_frame_loss_flag)
{
	unsigned int i;
	void *vaddr = NULL;
	struct isp_autoflash_config_s *isp_autoflash_cfg = NULL;
	unsigned int map_addr = 0;
	unsigned int check_sign = 0;
	unsigned int paddr_trf;
	unsigned int bsize_trf;
	unsigned int not_frame_loss_temp;

	for (i = 0; i < 2; i++) {
		if (i == 0) {
			map_addr = VIN_SENSOR0_RESERVE_ADDR;
			check_sign = 0xAA11AA11;
		} else {
			map_addr = VIN_SENSOR1_RESERVE_ADDR;
			check_sign = 0xBB11BB11;
		}

		vaddr = vin_map_kernel(map_addr, VIN_RESERVE_SIZE + VIN_THRESHOLD_PARAM_SIZE); /* map unit is page, page is align of 4k */
		if (vaddr == NULL) {
			vin_err("%s:map 0x%x paddr err!!!", __func__, map_addr);
			return;
		}

		isp_autoflash_cfg = (struct isp_autoflash_config_s *)(vaddr + VIN_RESERVE_SIZE);

		/* check id */
		if (isp_autoflash_cfg->melisyuv_sign_id != check_sign) {
			vin_warn("%s:sign is 0x%x but not 0x%x\n", __func__, isp_autoflash_cfg->sensorlist_sign_id, check_sign);
			vin_unmap_kernel(vaddr);
			continue;
		}

		phy_addr[i] = isp_autoflash_cfg->melisyuv_paddr;
		buf_size[i] = isp_autoflash_cfg->melisyuv_size;
		not_frame_loss_flag[i] = isp_autoflash_cfg->not_frame_loss_flag;

		vin_unmap_kernel(vaddr);
	}

	if (phy_addr[0] > phy_addr[1]) {
		paddr_trf = phy_addr[0];
		phy_addr[0] = phy_addr[1];
		phy_addr[1] = paddr_trf;

		bsize_trf = buf_size[0];
		buf_size[0] = buf_size[1];
		buf_size[1] = bsize_trf;

		not_frame_loss_temp = not_frame_loss_flag[0];
		not_frame_loss_flag[0] = not_frame_loss_flag[1];
		not_frame_loss_flag[1] = not_frame_loss_temp;
	}
}
#endif

static int isp_enable_irq(void *dev, void *data, int len)
{
	struct isp_dev *isp = dev;
	int ret = 0;

#if !IS_ENABLED(CONFIG_RV_RUN_CAR_REVERSE)
	unsigned int phy_addr[2] = {0, 0};
	unsigned int buf_size[2] = {0, 0};
	unsigned int not_frame_loss_flag[2] = {0, 0};
	unsigned int total_size;

	isp_reserve_for_yuv(phy_addr, buf_size, not_frame_loss_flag);

	if (isp->isp_reserved_np && isp->reserved_r.start && isp->reserved_len) {
		/*2 * (VIN_RESERVE_SIZE + VIN_THRESHOLD_PARAM_SIZE) reserved for vin_set_from_partition*/
		total_size = isp->reserved_len - 2 * (VIN_RESERVE_SIZE + VIN_THRESHOLD_PARAM_SIZE);

		if ((phy_addr[0] == 0 && phy_addr[1] == 0) || (not_frame_loss_flag[1] == 1 && phy_addr[0] == 0) || (not_frame_loss_flag[0] == 1 && not_frame_loss_flag[1] == 1)) {
			memblock_free(isp->reserved_r.start, total_size);
			free_reserved_area(__va(isp->reserved_r.start), __va(isp->reserved_r.start + total_size), -1, "isp_reserved");
		} else if (phy_addr[0] == 0 && phy_addr[1] != 0) {
			memblock_free(isp->reserved_r.start, phy_addr[1] - isp->reserved_r.start);
			free_reserved_area(__va(isp->reserved_r.start), __va(phy_addr[1]), -1, "isp_reserved");

			memblock_free(phy_addr[1] + buf_size[1], total_size - (phy_addr[1] + buf_size[1] - isp->reserved_r.start));
			free_reserved_area(__va(phy_addr[1] + buf_size[1]), __va(isp->reserved_r.start + total_size), -1, "isp_reserved");
		} else if (phy_addr[0] != 0 && phy_addr[1] != 0) {
			memblock_free(isp->reserved_r.start, phy_addr[0] - isp->reserved_r.start);
			free_reserved_area(__va(isp->reserved_r.start), __va(phy_addr[0]), -1, "isp_reserved");

			memblock_free(phy_addr[0] + buf_size[0], phy_addr[1] - (phy_addr[0] + buf_size[0]));
			free_reserved_area(__va(phy_addr[0] + buf_size[0]), __va(phy_addr[1]), -1, "isp_reserved");

			memblock_free(phy_addr[1] + buf_size[1], total_size - (phy_addr[1] + buf_size[1] - isp->reserved_r.start));
			free_reserved_area(__va(phy_addr[1] + buf_size[1]), __va(isp->reserved_r.start + total_size), -1, "isp_reserved");
		}
	}
#endif

	if (!data || !strncmp(data, "return", len)) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
		vin_iommu_en(ISP_IOMMU_MASTER, true);
#endif
		ret = request_irq(isp->irq, isp_isr, IRQF_SHARED, isp->pdev->name, isp);
		if (ret)
			vin_err("isp%d request irq failed\n", isp->id);
	} else if (!strncmp(data, "get", len)) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3)
		vin_iommu_en(ISP_IOMMU_MASTER, false);
#endif
		if (isp->irq)
			free_irq(isp->irq, isp);

	}

	return ret;
}
#endif

static int isp_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct isp_dev *isp = NULL;
	__maybe_unused struct isp_dev *logic_isp = NULL;
	__maybe_unused struct device_node *isp_reserved_np = NULL;
	__maybe_unused char name[16];
	int ret = 0;

	if (np == NULL) {
		vin_err("ISP failed to get of node\n");
		return -ENODEV;
	}

	isp = kzalloc(sizeof(*isp), GFP_KERNEL);
	if (!isp) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("ISP failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}

	isp->id = pdev->id;
	isp->pdev = pdev;
	isp->nosend_ispoff = 0;
	isp->load_flag = 0;

#if defined ISP_600
	of_property_read_u32(np, "work_mode", &isp->work_mode);
	if (isp->work_mode < 0) {
		vin_err("isp%d failed to get work_mode\n", isp->id);
		ret = -EINVAL;
		goto freedev;
	} else {
		if (isp->id == isp_virtual_find_logic[isp->id]) {
			isp->work_mode = clamp_t(unsigned int, isp->work_mode, ISP_ONLINE, ISP_OFFLINE);
		} else if (isp->work_mode == 0xff) {
			logic_isp = glb_isp[isp_virtual_find_logic[isp->id]];
			if (logic_isp->work_mode == ISP_ONLINE) { /*logic isp work in online*/
				vin_log(VIN_LOG_VIDEO, "isp%d work in online mode, isp%d cannot to work!\n", logic_isp->id, isp->id);
				isp->is_empty = 1;
				goto init;
			}
		}
	}
#endif

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	isp->isp_reserved_np = of_parse_phandle(np, "isp-region", 0);
	if (isp->isp_reserved_np) {
		ret = of_address_to_resource(isp->isp_reserved_np, 0, &isp->reserved_r);
		if (!ret) {
			isp->reserved_len = resource_size(&isp->reserved_r);
			vin_log(VIN_LOG_VIDEO, "register memory isp start 0x%x, size:0x%x.\n", isp->reserved_r.start, isp->reserved_len);
		} else
			vin_err("Failed to get isp-region\n");
	}
#endif

#if !defined CONFIG_VIN_INIT_MELIS
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		vin_iommu_en(ISP_IOMMU_MASTER, true);
#endif
#endif
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	isp->first_init_server = 1;
	if (isp->id == 0 || isp->id == 1 || isp->id == 2) {
		isp->controller = rpbuf_get_controller_by_of_node(np, 0);
		if (!isp->controller) {
			vin_warn("isp%d cannot get rpbuf controller\n", isp->id);
		}
	}
#endif
	isp->base = of_iomap(np, 0);
	if (!isp->base) {
		isp->is_empty = 1;
	} else {
		isp->is_empty = 0;
		/* get irq resource */
		isp->irq = irq_of_parse_and_map(np, 0);
		if (isp->irq <= 0) {
			vin_err("failed to get ISP IRQ resource\n");
			goto unmap;
		}
#if !defined CONFIG_VIN_INIT_MELIS
		ret = request_irq(isp->irq, isp_isr, IRQF_SHARED, isp->pdev->name, isp);
		if (ret) {
			vin_err("isp%d request irq failed\n", isp->id);
			goto unmap;
		}
#else
		of_property_read_u32(np, "delay_init", &isp->delay_init);
		if (isp->delay_init == 0) {
			ret = request_irq(isp->irq, isp_isr, IRQF_SHARED, isp->pdev->name, isp);
			if (ret) {
				vin_err("isp%d request irq failed\n", isp->id);
				goto unmap;
			}
		} else {
			sprintf(name, "isp%d", isp->id);
			rpmsg_notify_add("7130000.e906_rproc", name, isp_enable_irq, isp);
		}
#endif
		if (isp_resource_alloc(isp) < 0) {
			ret = -ENOMEM;
			goto freeirq;
		}
		bsp_isp_map_reg_addr(isp->id, (unsigned long)isp->base);
		bsp_isp_map_load_dram_addr(isp->id, (unsigned long)isp->isp_load.vir_addr);
#if defined ISP_600
		bsp_isp_map_save_load_dram_addr(isp->id, (unsigned long)isp->isp_save_load.vir_addr);
#if IS_ENABLED(CONFIG_ARCH_SUN8IW21)
		if (isp->id == 0) {
			isp->syscfg_base = of_iomap(np, 1);
			if (!isp->syscfg_base)
				vin_warn("syscfg get base addrsess fail\n");
			else
				bsp_isp_map_syscfg_addr(isp->id, (unsigned long)isp->syscfg_base);
		}
#endif
#endif
	}
#if defined ISP_600
init:
#endif
	__isp_init_subdev(isp);

	spin_lock_init(&isp->slock);

	ret = vin_isp_h3a_init(isp);
	if (ret < 0) {
		vin_err("VIN H3A initialization failed\n");
			goto free_res;
	}

	INIT_WORK(&isp->s_sensor_stby_task, __isp_s_sensor_stby_handle);
#if IS_ENABLED(CONFIG_ISP_SERVER_MELIS)
	INIT_WORK(&isp->isp_rpmsg_send_task, __isp_rpmsg_send_handle);
#endif
	platform_set_drvdata(pdev, isp);
	glb_isp[isp->id] = isp;

	vin_log(VIN_LOG_ISP, "isp%d probe end!\n", isp->id);
	return 0;
free_res:
	isp_resource_free(isp);
freeirq:
	if (!isp->is_empty)
		free_irq(isp->irq, isp);
unmap:
	if (!isp->is_empty)
		iounmap(isp->base);
	else
		kfree(isp->base);
freedev:
	kfree(isp);
ekzalloc:
	vin_err("isp probe err!\n");
	return ret;
}

static int isp_remove(struct platform_device *pdev)
{
	struct isp_dev *isp = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &isp->subdev;
	__maybe_unused char name[16];

#if IS_ENABLED(CONFIG_VIN_INIT_MELIS)
	if (isp->delay_init) {
		sprintf(name, "isp%d", isp->id);
		rpmsg_notify_del("7130000.e906_rproc", "isp0");
	}
#endif
	platform_set_drvdata(pdev, NULL);
	v4l2_ctrl_handler_free(sd->ctrl_handler);
	v4l2_set_subdevdata(sd, NULL);

	if (!isp->is_empty) {
		isp_resource_free(isp);
		free_irq(isp->irq, isp);
		if (isp->base)
			iounmap(isp->base);
	}
	vin_isp_h3a_cleanup(isp);
	media_entity_cleanup(&isp->subdev.entity);
	kfree(isp);
	return 0;
}

static const struct of_device_id sunxi_isp_match[] = {
	{.compatible = "allwinner,sunxi-isp",},
	{},
};

static struct platform_driver isp_platform_driver = {
	.probe = isp_probe,
	.remove = isp_remove,
	.driver = {
		   .name = ISP_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_isp_match,
		   },
};

void sunxi_isp_sensor_type(struct v4l2_subdev *sd, int use_isp)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	isp->use_isp = use_isp;
	if (isp->is_empty)
		isp->use_isp = 0;
}

void sunxi_isp_sensor_fps(struct v4l2_subdev *sd, int fps)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	isp->h3a_stat.sensor_fps = fps;
}

void sunxi_isp_debug(struct v4l2_subdev *sd, struct isp_debug_mode *isp_debug)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);

	isp->isp_dbg = *isp_debug;
}

void sunxi_isp_ptn(struct v4l2_subdev *sd, unsigned int ptn_type)
{
	struct isp_dev *isp = v4l2_get_subdevdata(sd);
	isp->ptn_type = ptn_type;
}

struct v4l2_subdev *sunxi_isp_get_subdev(int id)
{
	if (id < VIN_MAX_ISP)
		return &glb_isp[id]->subdev;
	else
		return NULL;
}

struct v4l2_subdev *sunxi_stat_get_subdev(int id)
{
#if !defined CONFIG_ISP_SERVER_MELIS
	if (id < VIN_MAX_ISP && glb_isp[id])
		return &glb_isp[id]->h3a_stat.sd;
	else
		return NULL;
#else
		return NULL;
#endif
}

int sunxi_isp_platform_register(void)
{
	return platform_driver_register(&isp_platform_driver);
}

void sunxi_isp_platform_unregister(void)
{
	platform_driver_unregister(&isp_platform_driver);
	vin_log(VIN_LOG_ISP, "isp_exit end\n");
}

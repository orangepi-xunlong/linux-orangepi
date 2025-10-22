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
#include <linux/delay.h>

#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/sunxi_camera_v2.h>

#include "parser_reg.h"
#include "sunxi_csi.h"
#include "../vin-video/vin_core.h"
#include "../vin-mipi/combo_csi/combo_csi_reg.h"
#include "../platform/platform_cfg.h"

#define CSI_MODULE_NAME "vin_csi"

#define IS_FLAG(x, y) (((x)&(y)) == y)

struct csi_dev *glb_parser[VIN_MAX_CSI];

static struct csi_format sunxi_csi_formats[] = {
	{
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
		.seq = SEQ_YUYV,
		.infmt = FMT_YUV422,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_YVYU8_2X8,
		.seq = SEQ_YVYU,
		.infmt = FMT_YUV422,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.seq = SEQ_UYVY,
		.infmt = FMT_YUV422,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_VYUY8_2X8,
		.seq = SEQ_VYUY,
		.infmt = FMT_YUV422,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_YUYV8_1X16,
		.seq = SEQ_YUYV,
		.infmt = FMT_YUV422,
		.data_width = 16,
	}, {
		.code = MEDIA_BUS_FMT_YVYU8_1X16,
		.seq = SEQ_YVYU,
		.infmt = FMT_YUV422,
		.data_width = 16,
	}, {
		.code = MEDIA_BUS_FMT_UYVY8_1X16,
		.seq = SEQ_UYVY,
		.infmt = FMT_YUV422,
		.data_width = 16,
	}, {
		.code = MEDIA_BUS_FMT_VYUY8_1X16,
		.seq = SEQ_VYUY,
		.infmt = FMT_YUV422,
		.data_width = 16,
	}, {
		.code = MEDIA_BUS_FMT_UYVY10_2X10,
		.seq = SEQ_UYVY,
		.infmt = FMT_YUV422,
		.data_width = 10,
	}, {
		.code = MEDIA_BUS_FMT_VYUY10_2X10,
		.seq = SEQ_VYUY,
		.infmt = FMT_YUV422,
		.data_width = 10,
	}, {
		.code = MEDIA_BUS_FMT_YVYU10_2X10,
		.seq = SEQ_YVYU,
		.infmt = FMT_YUV422,
		.data_width = 10,
	}, {
		.code = MEDIA_BUS_FMT_YUYV10_2X10,
		.seq = SEQ_YUYV,
		.infmt = FMT_YUV422,
		.data_width = 10,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR8_1X8,
		.infmt = FMT_RAW,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG8_1X8,
		.infmt = FMT_RAW,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG8_1X8,
		.infmt = FMT_RAW,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB8_1X8,
		.infmt = FMT_RAW,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.infmt = FMT_RAW,
		.data_width = 10,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.infmt = FMT_RAW,
		.data_width = 10,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.infmt = FMT_RAW,
		.data_width = 10,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.infmt = FMT_RAW,
		.data_width = 10,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR12_1X12,
		.infmt = FMT_RAW,
		.data_width = 12,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.infmt = FMT_RAW,
		.data_width = 12,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG12_1X12,
		.infmt = FMT_RAW,
		.data_width = 12,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.infmt = FMT_RAW,
		.data_width = 12,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR14_1X14,
		.infmt = FMT_RAW,
		.data_width = 14,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG14_1X14,
		.infmt = FMT_RAW,
		.data_width = 14,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG14_1X14,
		.infmt = FMT_RAW,
		.data_width = 14,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB14_1X14,
		.infmt = FMT_RAW,
		.data_width = 14,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR16_1X16,
		.infmt = FMT_RAW,
		.data_width = 16,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG16_1X16,
		.infmt = FMT_RAW,
		.data_width = 16,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG16_1X16,
		.infmt = FMT_RAW,
		.data_width = 16,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB16_1X16,
		.infmt = FMT_RAW,
		.data_width = 16,
	}, {
		.code = MEDIA_BUS_FMT_RGB888_1X24,
		.infmt = FMT_RAW,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_BGR888_1X24,
		.infmt = FMT_RAW,
		.data_width = 8,
	}, {
		.code = MEDIA_BUS_FMT_GBR888_1X24,
		.infmt = FMT_RAW,
		.data_width = 8,
	}
};

static int __csi_pin_config(struct csi_dev *dev, int enable)
{
#ifndef FPGA_VER
	char pinctrl_names[10] = "";

	if (dev->bus_info.bus_if == V4L2_MBUS_CSI2_DPHY ||
		dev->bus_info.bus_if == V4L2_MBUS_CSI2_CPHY ||
		dev->bus_info.bus_if == V4L2_MBUS_CSI1)
		return 0;

	if (!IS_ERR_OR_NULL(dev->pctrl))
		devm_pinctrl_put(dev->pctrl);

	if (enable == 1)
		strcpy(pinctrl_names, "default");
	else
		strcpy(pinctrl_names, "sleep");

	dev->pctrl = devm_pinctrl_get_select(&dev->pdev->dev, pinctrl_names);
	if (IS_ERR_OR_NULL(dev->pctrl)) {
		vin_err("csi%d request pinctrl handle failed!\n", dev->id);
		return -EINVAL;
	}
	usleep_range(100, 120);
#endif
	return 0;
}

static int __csi_pin_release(struct csi_dev *dev)
{
#ifndef FPGA_VER
	if (!IS_ERR_OR_NULL(dev->pctrl))
		devm_pinctrl_put(dev->pctrl);
#endif
	return 0;
}

static void csi_par_g_output_size(struct csi_dev *csi,
	struct prs_output_size *size, int csi_ch)
{
	if (!csi->tvin.flag)
		return;

	switch (csi->tvin.tvin_info.input_fmt[csi_ch]) {
	case YCBCR_576P:
		size->hor_len = 720;
		size->ver_len = 576;
		csi->tvin.tvin_info.field = 0; // progress
		break;
	case CVBS_PAL:
	case YCBCR_576I:
		size->hor_len = 720;
		size->ver_len = 576;
		csi->tvin.tvin_info.field = 1; // interlace
		break;
	case CVBS_NTSC:
	case YCBCR_480I:
		size->hor_len = 720;
		size->ver_len = 480;
		csi->tvin.tvin_info.field = 1; // interlace
		break;
	case YCBCR_480P:
		size->hor_len = 720;
		size->ver_len = 480;
		csi->tvin.tvin_info.field = 0; // progress
		break;
	case AHD720P25:
	case AHD720P30:
		size->hor_len = 1280;
		size->ver_len = 720;
		csi->tvin.tvin_info.field = 0; // progress
		break;
	case AHD1080P25:
	case AHD1080P30:
		size->hor_len = 1920;
		size->ver_len = 1080;
		csi->tvin.tvin_info.field = 0; // progress
		break;
	case CVBS_H1440_PAL:
		size->hor_len = 1440;
		size->ver_len = 576;
		csi->tvin.tvin_info.field = 1; // interlace
		break;
	case CVBS_H1440_NTSC:
		size->hor_len = 1440;
		size->ver_len = 480;
		csi->tvin.tvin_info.field = 1; // interlace
		break;
	case CVBS_FRAME_PAL:
		size->hor_len = 720;
		size->ver_len = 604;
		csi->tvin.tvin_info.field = 1; // interlace
		break;
	case CVBS_FRAME_NTSC:
		size->hor_len = 720;
		size->ver_len = 510;
		csi->tvin.tvin_info.field = 1; // interlace
		break;
	default:
		size->hor_len = 1280;
		size->ver_len = 720;
		csi->tvin.tvin_info.field = 0; // progress
	}
}

static int __csi_set_fmt_hw(struct csi_dev *csi)
{
	struct v4l2_mbus_framefmt *mf = &csi->mf;
	struct prs_ncsi_bt656_header bt656_header;
	struct prs_mcsi_if_cfg mcsi_if;
	struct prs_cap_mode mode;
	struct mbus_framefmt_res *res = (void *)mf->reserved;
	int i;
	unsigned int overlayer;

	memset(&bt656_header, 0, sizeof(bt656_header));
	memset(&mcsi_if, 0, sizeof(mcsi_if));

	csi->ncsi_if.seq = csi->csi_fmt->seq;
	mcsi_if.seq = csi->csi_fmt->seq;

	switch (csi->csi_fmt->data_width) {
	case 8:
		csi->ncsi_if.dw = DW_8BIT;
		break;
	case 10:
		csi->ncsi_if.dw = DW_10BIT;
		break;
	case 12:
		csi->ncsi_if.dw = DW_12BIT;
		break;
	default:
		csi->ncsi_if.dw = DW_8BIT;
		break;
	}

	switch (mf->field) {
	case V4L2_FIELD_ANY:
	case V4L2_FIELD_NONE:
		csi->ncsi_if.type = PROGRESSED;
		csi->ncsi_if.mode = FRAME_MODE;
		mcsi_if.mode = FIELD_MODE;
		break;
	case V4L2_FIELD_TOP:
	case V4L2_FIELD_BOTTOM:
		csi->ncsi_if.type = INTERLACE;
		csi->ncsi_if.mode = FIELD_MODE;
		mcsi_if.mode = FIELD_MODE;
		break;
	case V4L2_FIELD_INTERLACED:
		csi->ncsi_if.type = INTERLACE;
		csi->ncsi_if.mode = FRAME_MODE;
		mcsi_if.mode = FRAME_MODE;
		break;
	default:
		csi->ncsi_if.type = PROGRESSED;
		csi->ncsi_if.mode = FRAME_MODE;
		mcsi_if.mode = FIELD_MODE;
		break;
	}

	if (csi->tvin.flag) {
		csi->ncsi_if.mode = FRAME_MODE;
		mcsi_if.mode = FRAME_MODE;
	}

	switch (csi->bus_info.bus_if) {
	case V4L2_MBUS_PARALLEL:
		if (csi->csi_fmt->data_width == 16)
			csi->ncsi_if.intf = PRS_IF_INTLV_16BIT;
		else
			csi->ncsi_if.intf = PRS_IF_INTLV;
		if (res->pclk_dly > 0x00 && res->pclk_dly <= 0x1F) {
			csic_prs_set_pclk_dly(csi->id, res->pclk_dly);
		} else {
			if (csi->ncsi_if.ddr_sample == 1) {
				csic_prs_set_pclk_dly(csi->id, 0xb);
			} else {
				csic_prs_set_pclk_dly(csi->id, 0x9);
			}
		}
		csic_prs_mode(csi->id, PRS_NCSI);
		csic_prs_ncsi_if_cfg(csi->id, &csi->ncsi_if);
		csic_prs_ncsi_en(csi->id, 1);
		break;
	case V4L2_MBUS_BT656:
		if (csi->csi_fmt->data_width == 16) {
			if (csi->bus_info.ch_total_num == 1)
				csi->ncsi_if.intf = PRS_IF_BT1120_1CH;
			else if (csi->bus_info.ch_total_num == 2)
				csi->ncsi_if.intf = PRS_IF_BT1120_2CH;
			else if (csi->bus_info.ch_total_num == 4)
				csi->ncsi_if.intf = PRS_IF_BT1120_4CH;
			if (res->pclk_dly > 0x00 && res->pclk_dly <= 0x1F) {
				csic_prs_set_pclk_dly(csi->id, res->pclk_dly);
			} else {
				if (csi->ncsi_if.ddr_sample == 1) {
					csic_prs_set_pclk_dly(csi->id, 0xb);
				} else {
					csic_prs_set_pclk_dly(csi->id, 0x9);
				}
			}
		} else {
			if (csi->bus_info.ch_total_num == 1)
				csi->ncsi_if.intf = PRS_IF_BT656_1CH;
			else if (csi->bus_info.ch_total_num == 2)
				csi->ncsi_if.intf = PRS_IF_BT656_2CH;
			else if (csi->bus_info.ch_total_num == 4)
				csi->ncsi_if.intf = PRS_IF_BT656_4CH;
			if (res->pclk_dly > 0x00 && res->pclk_dly <= 0x1F) {
				csic_prs_set_pclk_dly(csi->id, res->pclk_dly);
			} else {
				if (csi->ncsi_if.ddr_sample == 1) {
					csic_prs_set_pclk_dly(csi->id, 0x9);
				}
			}
		}
		csic_prs_mode(csi->id, PRS_NCSI);
		bt656_header.ch0_id = 0;
		bt656_header.ch1_id = 1;
		bt656_header.ch2_id = 2;
		bt656_header.ch3_id = 3;
		csic_prs_ncsi_bt656_header_cfg(csi->id, &bt656_header);
		csic_prs_ncsi_if_cfg(csi->id, &csi->ncsi_if);
		csic_prs_ncsi_en(csi->id, 1);
		break;
	case V4L2_MBUS_CSI2_DPHY:
		csic_prs_mode(csi->id, PRS_MCSI);
		csic_prs_mcsi_if_cfg(csi->id, &mcsi_if);
		csic_prs_mcsi_en(csi->id, 1);
		break;
	default:
		csic_prs_mode(csi->id, PRS_MCSI);
		csic_prs_mcsi_if_cfg(csi->id, &mcsi_if);
		csic_prs_mcsi_en(csi->id, 1);
		break;
	}

	if (csi->capture_mode == V4L2_MODE_IMAGE)
		mode.mode = SCAP;
	else
		mode.mode = VCAP;

	if (csi->out_size.hor_len != mf->width ||
	    csi->out_size.ver_len != mf->height) {
		csi->out_size.hor_len = mf->width;
		csi->out_size.ver_len = mf->height;
		csi->out_size.hor_start = 0;
		csi->out_size.ver_start = 0;
	}

	if (mf->field == V4L2_FIELD_INTERLACED || mf->field == V4L2_FIELD_TOP ||
	    mf->field == V4L2_FIELD_BOTTOM)
		csi->out_size.ver_len = csi->out_size.ver_len / 2;

	for (i = 0; i < csi->bus_info.ch_total_num; i++) {
		if (csi->large_image == 3) {
			overlayer = vin_set_large_overlayer(mf->width);
			csic_prs_ch_en(csi->id, 1);
			csi->bus_info.ch_total_num = 2;
			if (i == 0) {
				csi->out_size.hor_len = mf->width/2 + overlayer;
				csi->out_size.ver_len = mf->height;
				csi->out_size.hor_start = 0;
				csi->out_size.ver_start = 0;
			} else if (i == 1) {
				csi->out_size.hor_len = mf->width/2 + overlayer;
				csi->out_size.ver_len = mf->height;
				csi->out_size.hor_start = mf->width/2 - overlayer;
				csi->out_size.ver_start = 0;
			}
		}
		if (csi->prs_ch_mode.mode_en == 1) {
			if (csi->prs_ch_mode.mode_sel == 1)
				csic_prs_ch_en(csi->id, 1);
		}
		csic_prs_input_fmt_cfg(csi->id, i, csi->csi_fmt->infmt);
		csic_prs_output_size_cfg(csi->id, i, &csi->out_size);
	}

	if (res->res_wdr_mode == ISP_SEHDR_MODE)
		csic_prs_ch_en(csi->id, 1);

	csic_prs_fps_ds(csi->id, &csi->prs_fps_ds);
	csic_prs_capture_start(csi->id, csi->bus_info.ch_total_num, &mode);
	return 0;
}

#if defined SUPPORT_ISP_TDM && !defined TDM_V200
static int __sunxi_csi_tdm_off(struct csi_dev *csi)
{
	struct vin_md *vind = dev_get_drvdata(csi->subdev.v4l2_dev->dev);
	struct vin_core *vinc = NULL;
	int i, j;

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (!vin_streaming(&vind->vinc[i]->vid_cap))
			continue;
		vinc = vind->vinc[i];
		for (j = 0; j < VIN_MAX_CSI; j++) {
			if (vinc->csi_sel == j)
				return -1;
		}
	}
	return 0;
}
#endif

static int sunxi_csi_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	__maybe_unused int i;
	__maybe_unused struct prs_signal_status prs_sig_status;
	__maybe_unused struct vin_md *vind = dev_get_drvdata(csi->subdev.v4l2_dev->dev);
	__maybe_unused struct vin_core *vinc = NULL;

	__csi_pin_config(csi, enable);
	csic_prs_pclk_en(csi->id, enable);
	if (enable) {
		csic_prs_enable(csi->id);
		csic_prs_disable(csi->id);
		csic_prs_enable(csi->id);
		__csi_set_fmt_hw(csi);
	} else {
		csi->tvin.flag = false;
#ifndef SUPPORT_ISP_TDM
		switch (csi->bus_info.bus_if) {
		case V4L2_MBUS_PARALLEL:
		case V4L2_MBUS_BT656:
			csic_prs_ncsi_en(csi->id, 0);
			break;
		case V4L2_MBUS_CSI2_DPHY:
			csic_prs_mcsi_en(csi->id, 0);
			break;
		default:
			return -1;
		}
#endif
		csic_prs_capture_stop(csi->id);
#if !defined SUPPORT_ISP_TDM || defined TDM_V200
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1)
		if (csi->bus_info.bus_if == V4L2_MBUS_CSI2_DPHY) {
			for (i = 0; i < VIN_MAX_DEV; i++) {
				if (vind->vinc[i] == NULL)
					continue;
				if (vind->vinc[i]->csi_sel == csi->id)
					vinc = vind->vinc[i];
			}
			cmb_port_clr_ch0_int_status(vinc->mipi_sel);
			for (i = 0; i < 200; i++) {
				if (cmb_port_check_ch0_int_status(vinc->mipi_sel, CMB_MIPI_FRAME_END_SYNC_MASK))
					break;
				usleep_range(500, 510);

				if (i >= 199)
					vin_warn("parser%d wait:%d/2 ms to close!\n", csi->id, i);
			}
		} else {
			for (i = 0; i < 200; i++) {
				memset(&prs_sig_status, 0, sizeof(struct prs_signal_status));
				csic_prs_signal_status(csi->id, &prs_sig_status);
				if (prs_sig_status.vsync_sta)
					break;
				usleep_range(500, 510);

				if (i >= 199)
					vin_warn("parser%d wait:%d/2 ms to close!\n", csi->id, i);
			}
		}
#endif
		csic_prs_disable(csi->id);
#else
		if (__sunxi_csi_tdm_off(csi) == 0) {
			for (i = 0; i < VIN_MAX_CSI; i++)
				csic_prs_disable(i);
		} else
			vin_warn("ISP is used in TDM mode, PARSER%d cannot be closing when other isp is used!\n", csi->id);
#endif
	}

	vin_log(VIN_LOG_FMT, "parser%d %s, %d*%d hoff: %d voff: %d code: %x field: %d\n",
		csi->id, enable ? "stream on" : "stream off",
		csi->out_size.hor_len, csi->out_size.ver_len,
		csi->out_size.hor_start, csi->out_size.ver_start,
		csi->mf.code, csi->mf.field);
	vin_log(VIN_LOG_LARGE, "parser%d %s, %d*%d hoff: %d voff: %d code: %x field: %d\n",
		csi->id, enable ? "stream on" : "stream off",
		csi->out_size.hor_len, csi->out_size.ver_len,
		csi->out_size.hor_start, csi->out_size.ver_start,
		csi->mf.code, csi->mf.field);

	return 0;
}

static struct csi_format *__csi_try_format(struct v4l2_mbus_framefmt *mf)
{
	struct csi_format *csi_fmt = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_csi_formats); i++)
		if (mf->code == sunxi_csi_formats[i].code)
			csi_fmt = &sunxi_csi_formats[i];

	if (csi_fmt == NULL)
		csi_fmt = &sunxi_csi_formats[0];

	mf->code = csi_fmt->code;
	v4l_bound_align_image(&mf->width, 1, 0xffff, 1, &mf->height, 1, 0xffff, 1, 0);

	return csi_fmt;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static int sunxi_csi_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *state,
				    struct v4l2_subdev_format *fmt)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;
	struct csi_format *csi_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = &csi->mf;

	if (fmt->pad == CSI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&csi->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&csi->subdev_lock);
		}
		return 0;
	}
	csi_fmt = __csi_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&csi->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			csi->csi_fmt = csi_fmt;
		mutex_unlock(&csi->subdev_lock);
	}

	return 0;
}

static int sunxi_csi_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *state,
				    struct v4l2_subdev_format *fmt)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	mutex_lock(&csi->subdev_lock);
	fmt->format = csi->mf;
	mutex_unlock(&csi->subdev_lock);
	return 0;
}

static int sunxi_csi_subdev_set_selection(struct v4l2_subdev *sd,
			    struct v4l2_subdev_state *state,
			    struct v4l2_subdev_selection *sel)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	csi->out_size.hor_len = state->pads->try_crop.width;
	csi->out_size.ver_len = state->pads->try_crop.height;
	csi->out_size.hor_start = state->pads->try_crop.left;
	csi->out_size.ver_start = state->pads->try_crop.top;
	return 0;
}

#else /* before linux-5.15 */
static int sunxi_csi_subdev_set_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_format *fmt)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;
	struct csi_format *csi_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = &csi->mf;

	if (fmt->pad == CSI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&csi->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&csi->subdev_lock);
		}
		return 0;
	}
	csi_fmt = __csi_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&csi->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			csi->csi_fmt = csi_fmt;
		mutex_unlock(&csi->subdev_lock);
	}

	return 0;
}

static int sunxi_csi_subdev_get_fmt(struct v4l2_subdev *sd,
				    struct v4l2_subdev_pad_config *cfg,
				    struct v4l2_subdev_format *fmt)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	mutex_lock(&csi->subdev_lock);
	fmt->format = csi->mf;
	mutex_unlock(&csi->subdev_lock);
	return 0;
}

static int sunxi_csi_subdev_set_selection(struct v4l2_subdev *sd,
			    struct v4l2_subdev_pad_config *cfg,
			    struct v4l2_subdev_selection *sel)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	csi->out_size.hor_len = cfg->try_crop.width;
	csi->out_size.ver_len = cfg->try_crop.height;
	csi->out_size.hor_start = cfg->try_crop.left;
	csi->out_size.ver_start = cfg->try_crop.top;
	return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int sunxi_csi_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				   struct v4l2_mbus_config *cfg)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2_DPHY || cfg->type == V4L2_MBUS_SUBLVDS ||
	    cfg->type == V4L2_MBUS_HISPI) {
		csi->bus_info.bus_if = V4L2_MBUS_CSI2_DPHY;
		csi->bus_info.ch_total_num = 0;
		if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_CHANNEL_0))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_CHANNEL_1))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_CHANNEL_2))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_CHANNEL_3))
			csi->bus_info.ch_total_num++;
		if (csi->large_image == 3)
			csi->bus_info.ch_total_num = 2;
	} else if (cfg->type == V4L2_MBUS_PARALLEL) {
		csi->bus_info.bus_if = V4L2_MBUS_PARALLEL;
		csi->bus_info.ch_total_num = 1;
		if (IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_MASTER)) {
			if (IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_HSYNC_ACTIVE_HIGH)) {
				csi->bus_info.bus_tmg.href_pol = ACTIVE_HIGH;
				csi->ncsi_if.href = REF_POSITIVE;
			} else {
				csi->bus_info.bus_tmg.href_pol = ACTIVE_LOW;
				csi->ncsi_if.href = REF_NEGATIVE;
			}
			if (IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_VSYNC_ACTIVE_HIGH)) {
				csi->bus_info.bus_tmg.vref_pol = ACTIVE_HIGH;
				csi->ncsi_if.vref = REF_POSITIVE;
			} else {
				csi->bus_info.bus_tmg.vref_pol = ACTIVE_LOW;
				csi->ncsi_if.vref = REF_NEGATIVE;
			}
			if (IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_PCLK_SAMPLE_RISING) &&
					IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
				csi->ncsi_if.ddr_sample = 1;
			} else if (IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_PCLK_SAMPLE_RISING)) {
				csi->bus_info.bus_tmg.pclk_sample = RISING;
				csi->ncsi_if.clk = CLK_RISING;
				csi->ncsi_if.ddr_sample = 0;
			} else {
				csi->bus_info.bus_tmg.pclk_sample = FALLING;
				csi->ncsi_if.clk = CLK_FALLING;
				csi->ncsi_if.ddr_sample = 0;
			}
			if (IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_FIELD_EVEN_HIGH)) {
				csi->bus_info.bus_tmg.field_even_pol = ACTIVE_HIGH;
				csi->ncsi_if.field = FIELD_POS;
			} else {
				csi->bus_info.bus_tmg.field_even_pol = ACTIVE_LOW;
				csi->ncsi_if.field = FIELD_NEG;
			}
		} else {
			vin_err("Do not support V4L2_MBUS_SLAVE!\n");
			return -1;
		}
	} else if (cfg->type == V4L2_MBUS_BT656) {
		csi->bus_info.bus_if = V4L2_MBUS_BT656;
		csi->bus_info.ch_total_num = 0;
		if (IS_FLAG(cfg->bus.parallel.data_shift, CSI_CH_0))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->bus.parallel.data_shift, CSI_CH_1))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->bus.parallel.data_shift, CSI_CH_2))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->bus.parallel.data_shift, CSI_CH_3))
			csi->bus_info.ch_total_num++;
		if (csi->bus_info.ch_total_num == 4) {
			csi->arrange.column = 2;
			csi->arrange.row = 2;
		} else if (csi->bus_info.ch_total_num == 2) {
			csi->arrange.column = 2;
			csi->arrange.row = 1;
		} else {
			csi->bus_info.ch_total_num = 1;
			csi->arrange.column = 1;
			csi->arrange.row = 1;
		}
		if (IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_PCLK_SAMPLE_RISING) &&
				IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
			csi->ncsi_if.ddr_sample = 1;
		} else if (IS_FLAG(cfg->bus.parallel.flags, V4L2_MBUS_PCLK_SAMPLE_RISING)) {
			csi->bus_info.bus_tmg.pclk_sample = RISING;
			csi->ncsi_if.clk = CLK_RISING;
			csi->ncsi_if.ddr_sample = 0;
		} else {
			csi->bus_info.bus_tmg.pclk_sample = FALLING;
			csi->ncsi_if.clk = CLK_FALLING;
			csi->ncsi_if.ddr_sample = 0;
		}
	}
	vin_log(VIN_LOG_CSI, "csi%d total ch = %d\n", csi->id, csi->bus_info.ch_total_num);

	return 0;
}
#else
static int sunxi_csi_s_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				   struct v4l2_mbus_config *cfg)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2_DPHY || cfg->type == V4L2_MBUS_SUBLVDS ||
	    cfg->type == V4L2_MBUS_HISPI) {
		csi->bus_info.bus_if = V4L2_MBUS_CSI2_DPHY;
		csi->bus_info.ch_total_num = 0;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3))
			csi->bus_info.ch_total_num++;
		if (csi->large_image == 3)
			csi->bus_info.ch_total_num = 2;
		if (csi->prs_ch_mode.mode_en) {
			if (csi->bus_info.ch_total_num == 1)
				csi->prs_ch_mode.mode_sel = 1;
			else if (csi->bus_info.ch_total_num == 2)
				//csi->prs_ch_mode.mode_sel = 2;
				vin_warn("2 channel sensor does not support this function\n");
			else {
				vin_err("twice the number of sensor channels is:%d, more than csi total channel\n", csi->bus_info.ch_total_num * 2);
				return -1;
			}
			csi->bus_info.ch_total_num = csi->prs_ch_mode.channel_num;
		}
	} else if (cfg->type == V4L2_MBUS_PARALLEL) {
		csi->bus_info.bus_if = V4L2_MBUS_PARALLEL;
		csi->bus_info.ch_total_num = 1;
		if (IS_FLAG(cfg->flags, V4L2_MBUS_MASTER)) {
			if (IS_FLAG(cfg->flags, V4L2_MBUS_HSYNC_ACTIVE_HIGH)) {
				csi->bus_info.bus_tmg.href_pol = ACTIVE_HIGH;
				csi->ncsi_if.href = REF_POSITIVE;
			} else {
				csi->bus_info.bus_tmg.href_pol = ACTIVE_LOW;
				csi->ncsi_if.href = REF_NEGATIVE;
			}
			if (IS_FLAG(cfg->flags, V4L2_MBUS_VSYNC_ACTIVE_HIGH)) {
				csi->bus_info.bus_tmg.vref_pol = ACTIVE_HIGH;
				csi->ncsi_if.vref = REF_POSITIVE;
			} else {
				csi->bus_info.bus_tmg.vref_pol = ACTIVE_LOW;
				csi->ncsi_if.vref = REF_NEGATIVE;
			}
			if (IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_RISING) &&
					IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
				csi->ncsi_if.ddr_sample = 1;
			} else if (IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_RISING)) {
				csi->bus_info.bus_tmg.pclk_sample = RISING;
				csi->ncsi_if.clk = CLK_RISING;
				csi->ncsi_if.ddr_sample = 0;
			} else {
				csi->bus_info.bus_tmg.pclk_sample = FALLING;
				csi->ncsi_if.clk = CLK_FALLING;
				csi->ncsi_if.ddr_sample = 0;
			}
			if (IS_FLAG(cfg->flags, V4L2_MBUS_FIELD_EVEN_HIGH)) {
				csi->bus_info.bus_tmg.field_even_pol = ACTIVE_HIGH;
				csi->ncsi_if.field = FIELD_POS;
			} else {
				csi->bus_info.bus_tmg.field_even_pol = ACTIVE_LOW;
				csi->ncsi_if.field = FIELD_NEG;
			}
		} else {
			vin_err("Do not support V4L2_MBUS_SLAVE!\n");
			return -1;
		}
	} else if (cfg->type == V4L2_MBUS_BT656) {
		csi->bus_info.bus_if = V4L2_MBUS_BT656;
		csi->bus_info.ch_total_num = 0;
		if (IS_FLAG(cfg->flags, CSI_CH_0))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, CSI_CH_1))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, CSI_CH_2))
			csi->bus_info.ch_total_num++;
		if (IS_FLAG(cfg->flags, CSI_CH_3))
			csi->bus_info.ch_total_num++;
		if (csi->bus_info.ch_total_num == 4) {
			csi->arrange.column = 2;
			csi->arrange.row = 2;
		} else if (csi->bus_info.ch_total_num == 2) {
			csi->arrange.column = 2;
			csi->arrange.row = 1;
		} else {
			csi->bus_info.ch_total_num = 1;
			csi->arrange.column = 1;
			csi->arrange.row = 1;
		}
		if (IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_RISING) &&
				IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_FALLING)) {
			csi->ncsi_if.ddr_sample = 1;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_PCLK_SAMPLE_RISING)) {
			csi->bus_info.bus_tmg.pclk_sample = RISING;
			csi->ncsi_if.clk = CLK_RISING;
			csi->ncsi_if.ddr_sample = 0;
		} else {
			csi->bus_info.bus_tmg.pclk_sample = FALLING;
			csi->ncsi_if.clk = CLK_FALLING;
			csi->ncsi_if.ddr_sample = 0;
		}
	}
	vin_log(VIN_LOG_CSI, "csi%d total ch = %d\n", csi->id, csi->bus_info.ch_total_num);

	return 0;
}
#endif

static int csi_tvin_init(struct v4l2_subdev *sd,
			struct tvin_init_info *info)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	int ch = info->ch_id;

	if (ch > TVIN_SEPARATE) {
		vin_err("[%s]parser ch can not over %d\n", __func__, TVIN_SEPARATE);
		return -1;
	}

	csi->tvin.tvin_info.input_fmt[ch] = info->input_fmt[ch];

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (csi->stream_count != 0) {
#else
	if (sd->entity.stream_count != 0) {
#endif
		csi_par_g_output_size(csi, &csi->tvin.out_size[ch], ch);
		if (csi->tvin.tvin_info.input_fmt[ch] == CVBS_H1440_PAL ||
			csi->tvin.tvin_info.input_fmt[ch] == CVBS_H1440_NTSC ||
			csi->tvin.tvin_info.input_fmt[ch] == CVBS_PAL ||
			csi->tvin.tvin_info.input_fmt[ch] == CVBS_NTSC)
			csi->tvin.out_size[ch].ver_len = csi->tvin.out_size[ch].ver_len / 2;
		csic_prs_output_size_cfg(csi->id, ch, &csi->tvin.out_size[ch]);
		vin_log(VIN_LOG_CSI, "======= csic field is %d!=========\r\n", csi->tvin.tvin_info.field);
		csic_prs_ncsi_field_cfg(csi->id, ch, csi->tvin.tvin_info.field);
	}
	csi->tvin.flag = true;
	return 0;
}

static long csi_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {
	case PARSER_TVIN_INIT:
		ret = csi_tvin_init(sd, (struct tvin_init_info *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static const struct v4l2_subdev_core_ops csi_core_ops = {
	.ioctl = csi_ioctl,
};

static const struct v4l2_subdev_video_ops sunxi_csi_subdev_video_ops = {
	.s_stream = sunxi_csi_subdev_s_stream,
};

static const struct v4l2_subdev_pad_ops sunxi_csi_subdev_pad_ops = {
	.set_selection = sunxi_csi_subdev_set_selection,
	.get_fmt = sunxi_csi_subdev_get_fmt,
	.set_fmt = sunxi_csi_subdev_set_fmt,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	.get_mbus_config = sunxi_csi_g_mbus_config,
#else
	.set_mbus_config = sunxi_csi_s_mbus_config,
#endif
};

static struct v4l2_subdev_ops sunxi_csi_subdev_ops = {
	.core = &csi_core_ops,
	.video = &sunxi_csi_subdev_video_ops,
	.pad = &sunxi_csi_subdev_pad_ops,
};

static int __csi_init_subdev(struct csi_dev *csi)
{
	struct v4l2_subdev *sd = &csi->subdev;
	int ret;

	mutex_init(&csi->subdev_lock);
	csi->arrange.row = 1;
	csi->arrange.column = 1;
	csi->bus_info.ch_total_num = 1;
	v4l2_subdev_init(sd, &sunxi_csi_subdev_ops);
	sd->grp_id = VIN_GRP_ID_CSI;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_csi.%u", csi->id);
	v4l2_set_subdevdata(sd, csi);

	csi->csi_pads[CSI_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	csi->csi_pads[CSI_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_IO_V4L;

	ret = media_entity_pads_init(&sd->entity, CSI_PAD_NUM, csi->csi_pads);
	if (ret < 0)
		return ret;

	return 0;
}

static int csi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct csi_dev *csi = NULL;
	int ret = 0;

	if (np == NULL) {
		vin_err("CSI failed to get of node\n");
		return -ENODEV;
	}
	csi = kzalloc(sizeof(*csi), GFP_KERNEL);
	if (!csi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("CSI failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}
	csi->id = pdev->id;
	csi->pdev = pdev;

	/* just for test because the csi1 is virtual node */
	csi->base = of_iomap(np, 0);
	if (!csi->base) {
		ret = -EIO;
		goto freedev;
	}

	ret = csic_prs_set_base_addr(csi->id, (unsigned long)csi->base);
	if (ret < 0)
		goto unmap;

	mutex_init(&csi->reset_lock);
	__csi_init_subdev(csi);

	platform_set_drvdata(pdev, csi);
	glb_parser[csi->id] = csi;

	vin_log(VIN_LOG_CSI, "csi%d probe end!\n", csi->id);

	return 0;

unmap:
	iounmap(csi->base);
freedev:
	kfree(csi);
ekzalloc:
	vin_log(VIN_LOG_CSI, "csi probe err!\n");
	return ret;
}

static int csi_remove(struct platform_device *pdev)
{
	struct csi_dev *csi = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &csi->subdev;

	platform_set_drvdata(pdev, NULL);
	v4l2_set_subdevdata(sd, NULL);
	__csi_pin_release(csi);
	mutex_destroy(&csi->subdev_lock);
	if (csi->base)
		iounmap(csi->base);
	mutex_destroy(&csi->reset_lock);
	media_entity_cleanup(&csi->subdev.entity);
	kfree(csi);
	return 0;
}

static const struct of_device_id sunxi_csi_match[] = {
	{.compatible = "allwinner,sunxi-csi",},
	{},
};

static struct platform_driver csi_platform_driver = {
	.probe = csi_probe,
	.remove = csi_remove,
	.driver = {
		   .name = CSI_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_csi_match,
		   },
};

int sunxi_csi_set_ch_mode(struct v4l2_subdev *sd)
{
	struct csi_dev *csi = v4l2_get_subdevdata(sd);
	struct vin_md *vind = dev_get_drvdata(csi->subdev.v4l2_dev->dev);
	int i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (csi->stream_count != 0)
#else
	if (sd->entity.stream_count != 0)
#endif
		return 0;

	memset(&csi->prs_ch_mode, 0, sizeof(struct prs_ch_mode));

	for (i = 0; i < VIN_MAX_DEV; i++) {
		if (vind->vinc[i] == NULL)
			continue;
		if (vind->vinc[i]->csi_ch == 0xff)
			continue;
		if (csi->prs_ch_mode.channel_num  < ((vind->vinc[i]->csi_ch & 0xf) + 1))
			csi->prs_ch_mode.channel_num = (vind->vinc[i]->csi_ch & 0xf) + 1;
	}
	csi->prs_ch_mode.mode_en = 1;

	return 0;
}

int sunxi_csi_subdev_s_parm(struct v4l2_subdev *sd,
				   struct v4l2_streamparm *param)
{
	struct v4l2_captureparm *cp = &param->parm.capture;
	struct csi_dev *csi = v4l2_get_subdevdata(sd);

	csi->capture_mode = param->parm.capture.capturemode;
	csi->large_image = cp->reserved[2];
	return 0;
}

struct v4l2_subdev *sunxi_csi_get_subdev(int id)
{
	if (id < VIN_MAX_CSI && glb_parser[id])
		return &glb_parser[id]->subdev;
	else
		return NULL;
}

int sunxi_csi_platform_register(void)
{
	return platform_driver_register(&csi_platform_driver);
}

void sunxi_csi_platform_unregister(void)
{
	platform_driver_unregister(&csi_platform_driver);
	vin_log(VIN_LOG_CSI, "csi_exit end\n");
}

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mipi subdev driver module
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "../utility/vin_log.h"
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <linux/debugfs.h>
#include "bsp_mipi_csi.h"
#include "combo_rx/combo_rx_reg.h"
#include "combo_rx/combo_rx_reg_i.h"
#include "combo_csi/combo_csi_reg.h"
#include "sunxi_mipi.h"
#include "../platform/platform_cfg.h"
#include "../vin-video/vin_core.h"
#define MIPI_MODULE_NAME "vin_mipi"

#define IS_FLAG(x, y) (((x)&(y)) == y)

/* mipi_lane_spec[m][n]:
* m: maximum number of mipi
* n: fixed to 2
* mipi_lane_spec[][0] : the number of lanes supported by this mipi
* mipi_lane_spec[][1] : the maximum number of lanes supported by this mipi
*/
struct mipi_dev *glb_mipi[VIN_MAX_MIPI];

#define MIPI_DEBUGFS_BUF_SIZE 768
struct mipi_debugfs_buffer {
	size_t count;
	char data[MIPI_DEBUGFS_BUF_SIZE * VIN_MAX_DEV];
};

struct dentry *mipi_debugfs_root, *mipi_node;
size_t mipi_status_size[VIN_MAX_DEV];
size_t mipi_status_size_sum;

#if IS_ENABLED(CONFIG_ARCH_SUN300IW1)
int mipi_lane_spec[VIN_MAX_MIPI][2] = {
	{1, 2}, {1, 1},
};
#elif IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6)
int mipi_lane_spec[VIN_MAX_MIPI][2] = {
	{2, 4}, {2, 2}, {2, 4}, {2, 2},
};
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW10)
int mipi_lane_spec[VIN_MAX_MIPI][2] = {
	{4, 4}, {2, 2},
};
#elif IS_ENABLED(CONFIG_ARCH_SUN60IW2)
int mipi_lane_spec[VIN_MAX_MIPI][2] = {
	{4, 4}, {4, 4}, {2, 2},
};
#else
int mipi_lane_spec[VIN_MAX_MIPI][2] = {
};
#endif

struct mipi_dev *glb_mipi[VIN_MAX_MIPI];

static void mipi_set_st_time(int id, unsigned int time)
{
	struct mipi_dev *mipi;

	if (id >= VIN_MAX_MIPI) {
		vin_print("mipi id error,set it less than %d\n", VIN_MAX_MIPI);
		return;
	}

	vin_print("Set mipi%d settle time as 0x%x\n", id, time);
	mipi = glb_mipi[id];
	if (mipi) {
		mipi->settle_time = time;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
		if (mipi->stream_count > 0) {
#else
		if (mipi->subdev.entity.stream_count > 0) {
#endif
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
			cmb_rx_mipi_stl_time(mipi->id, time);
#elif defined MIPI_COMBO_CSI
			cmb_phy0_s2p_dly(mipi->id, time);
#else
			mipi_dphy_cfg_1data(mipi->id, 0x75, time);
#endif
		}
	}
}

static ssize_t mipi_settle_time_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct mipi_dev *mipi;
	int i, cnt = 0;

	for (i = 0; i < VIN_MAX_MIPI; i++) {
		mipi = glb_mipi[i];
		if (mipi)
			cnt += sprintf(buf + cnt, "mipi%d settle time = 0x%x\n",
										mipi->id, mipi->settle_time);
	}
	return cnt;
}

static ssize_t mipi_settle_time_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int mipi_id;

	val = simple_strtoul(buf, NULL, 16);
	mipi_id = val >> 8;
	mipi_set_st_time(mipi_id, val & 0xff);
	return count;
}

static DEVICE_ATTR(settle_time, S_IRUGO | S_IWUSR | S_IWGRP,
		   mipi_settle_time_show, mipi_settle_time_store);

static struct combo_format sunxi_mipi_formats[] = {
	{
		.code = MEDIA_BUS_FMT_SBGGR8_1X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG8_1X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG8_1X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB8_1X8,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR10_1X10,
		.bit_width = RAW10,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG10_1X10,
		.bit_width = RAW10,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG10_1X10,
		.bit_width = RAW10,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB10_1X10,
		.bit_width = RAW10,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR12_1X12,
		.bit_width = RAW12,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG12_1X12,
		.bit_width = RAW12,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG12_1X12,
		.bit_width = RAW12,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB12_1X12,
		.bit_width = RAW12,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR14_1X14,
		.bit_width = RAW14,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG14_1X14,
		.bit_width = RAW14,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG14_1X14,
		.bit_width = RAW14,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB14_1X14,
		.bit_width = RAW14,
	}, {
		.code = MEDIA_BUS_FMT_SBGGR16_1X16,
		.bit_width = RAW16,
	}, {
		.code = MEDIA_BUS_FMT_SGBRG16_1X16,
		.bit_width = RAW16,
	}, {
		.code = MEDIA_BUS_FMT_SGRBG16_1X16,
		.bit_width = RAW16,
	}, {
		.code = MEDIA_BUS_FMT_SRGGB16_1X16,
		.bit_width = RAW16,
	}, {
		.code = MEDIA_BUS_FMT_UYVY8_2X8,
		.bit_width = YUV8,
		.yuv_seq = UYVY,
	}, {
		.code = MEDIA_BUS_FMT_VYUY8_2X8,
		.bit_width = YUV8,
		.yuv_seq = VYUY,
	}, {
		.code = MEDIA_BUS_FMT_YUYV8_2X8,
		.bit_width = YUV8,
		.yuv_seq = YUYV,
	}, {
		.code = MEDIA_BUS_FMT_YVYU8_2X8,
		.bit_width = YUV8,
		.yuv_seq = YVYU,
	}, {
		.code = MEDIA_BUS_FMT_UYVY8_1X16,
		.bit_width = YUV8,
		.yuv_seq = UYVY,
	}, {
		.code = MEDIA_BUS_FMT_VYUY8_1X16,
		.bit_width = YUV8,
		.yuv_seq = VYUY,
	}, {
		.code = MEDIA_BUS_FMT_YUYV8_1X16,
		.bit_width = YUV8,
		.yuv_seq = YUYV,
	}, {
		.code = MEDIA_BUS_FMT_YVYU8_1X16,
		.bit_width = YUV8,
		.yuv_seq = YVYU,
	}, {
		.code = MEDIA_BUS_FMT_UYVY10_2X10,
		.bit_width = YUV10,
		.yuv_seq = UYVY,
	}, {
		.code = MEDIA_BUS_FMT_VYUY10_2X10,
		.bit_width = YUV10,
		.yuv_seq = VYUY,
	}, {
		.code = MEDIA_BUS_FMT_YUYV10_2X10,
		.bit_width = YUV10,
		.yuv_seq = YUYV,
	}, {
		.code = MEDIA_BUS_FMT_YVYU10_2X10,
		.bit_width = YUV10,
		.yuv_seq = YVYU,
	}, {
		.code = MEDIA_BUS_FMT_RGB888_1X24,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_BGR888_1X24,
		.bit_width = RAW8,
	}, {
		.code = MEDIA_BUS_FMT_GBR888_1X24,
		.bit_width = RAW8,
	}
};

static enum pkt_fmt get_pkt_fmt(u32 code)
{
	switch (code) {
	case MEDIA_BUS_FMT_RGB565_2X8_BE:
		return MIPI_RGB565;
	case MEDIA_BUS_FMT_RGB888_1X24:
	case MEDIA_BUS_FMT_BGR888_1X24:
	case MEDIA_BUS_FMT_GBR888_1X24:
		return MIPI_RGB888;
	case MEDIA_BUS_FMT_UYVY8_1X16:
	case MEDIA_BUS_FMT_VYUY8_1X16:
	case MEDIA_BUS_FMT_YUYV8_1X16:
	case MEDIA_BUS_FMT_YVYU8_1X16:
	case MEDIA_BUS_FMT_UYVY8_2X8:
	case MEDIA_BUS_FMT_VYUY8_2X8:
	case MEDIA_BUS_FMT_YUYV8_2X8:
	case MEDIA_BUS_FMT_YVYU8_2X8:
		return MIPI_YUV422;
	case MEDIA_BUS_FMT_UYVY10_2X10:
	case MEDIA_BUS_FMT_VYUY10_2X10:
	case MEDIA_BUS_FMT_YUYV10_2X10:
	case MEDIA_BUS_FMT_YVYU10_2X10:
		return MIPI_YUV422_10;
	case MEDIA_BUS_FMT_SBGGR8_1X8:
	case MEDIA_BUS_FMT_SGBRG8_1X8:
	case MEDIA_BUS_FMT_SGRBG8_1X8:
	case MEDIA_BUS_FMT_SRGGB8_1X8:
		return MIPI_RAW8;
	case MEDIA_BUS_FMT_SBGGR10_1X10:
	case MEDIA_BUS_FMT_SGBRG10_1X10:
	case MEDIA_BUS_FMT_SGRBG10_1X10:
	case MEDIA_BUS_FMT_SRGGB10_1X10:
		return MIPI_RAW10;
	case MEDIA_BUS_FMT_SBGGR12_1X12:
	case MEDIA_BUS_FMT_SGBRG12_1X12:
	case MEDIA_BUS_FMT_SGRBG12_1X12:
	case MEDIA_BUS_FMT_SRGGB12_1X12:
		return MIPI_RAW12;
	case MEDIA_BUS_FMT_SBGGR14_1X14:
	case MEDIA_BUS_FMT_SGBRG14_1X14:
	case MEDIA_BUS_FMT_SGRBG14_1X14:
	case MEDIA_BUS_FMT_SRGGB14_1X14:
		return MIPI_RAW14;
	case MEDIA_BUS_FMT_SBGGR16_1X16:
	case MEDIA_BUS_FMT_SGBRG16_1X16:
	case MEDIA_BUS_FMT_SGRBG16_1X16:
	case MEDIA_BUS_FMT_SRGGB16_1X16:
		return MIPI_RAW16;
	default:
		return MIPI_RAW8;
	}
}

static unsigned int data_formats_type(u32 code)
{
	switch (code) {
	case MIPI_RAW8:
	case MIPI_RAW12:
	case MIPI_RAW14:
	case MIPI_RAW16:
		return RAW;
	case MIPI_YUV422:
	case MIPI_YUV422_10:
		return YUV;
	case MIPI_RGB565:
	case MIPI_RGB888:
		return RGB;
	default:
		return RAW;
	}
}

#if defined MIPI_PING_CONFIG
static int __mcsi_pin_config(struct mipi_dev *dev, int enable)
{
#ifndef FPGA_VER
	char pinctrl_names[20] = "";

	if (!IS_ERR_OR_NULL(dev->pctrl))
		devm_pinctrl_put(dev->pctrl);

	switch (enable) {
	case 1:
		sprintf(pinctrl_names, "mipi%d-default", dev->id);
		break;
	case 0:
		sprintf(pinctrl_names, "mipi%d-sleep", dev->id);
		break;
	default:
		return -1;
	}

	dev->pctrl = devm_pinctrl_get_select(&dev->pdev->dev, pinctrl_names);
	if (IS_ERR_OR_NULL(dev->pctrl)) {
		vin_err("mipi%d request pinctrl handle failed!\n", dev->id);
		return -EINVAL;
	}
	usleep_range(100, 120);

	/* if need two mipi dev,needs set it */
	if (dev->cmb_csi_cfg.lane_num > mipi_lane_spec[dev->id][0]) {
		if (dev->cmb_csi_cfg.lane_num <= mipi_lane_spec[dev->id][1]) {
			if (enable == 1)
				sprintf(pinctrl_names, "mipi%d-%dlane-default", dev->id + 1, mipi_lane_spec[dev->id][1]);
			else
				sprintf(pinctrl_names, "mipi%d-%dlane-sleep", dev->id + 1, mipi_lane_spec[dev->id][1]);
			dev->pctrl = devm_pinctrl_get_select(&dev->pdev->dev, pinctrl_names);
			if (IS_ERR_OR_NULL(dev->pctrl)) {
				vin_err("mipi%d %dlane request pinctrl handle failed!\n", dev->id, dev->cmb_csi_cfg.lane_num);
				return -EINVAL;
			}
			usleep_range(100, 120);
		} else {
			vin_err("mipi%d not support %dlane!\n", dev->id, dev->cmb_csi_cfg.lane_num);
			return -EINVAL;
		}
	}

#endif
	return 0;
}

static int __mcsi_pin_release(struct mipi_dev *dev)
{
#ifndef FPGA_VER
	if (!IS_ERR_OR_NULL(dev->pctrl))
		devm_pinctrl_put(dev->pctrl);
#endif
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
void combo_rx_mipi_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct mipi_ctr mipi_ctr;
	struct mipi_lane_map mipi_map;

	memset(&mipi_ctr, 0, sizeof(mipi_ctr));
	mipi_ctr.mipi_lane_num = mipi->cmb_cfg.mipi_ln;
	mipi_ctr.mipi_msb_lsb_sel = 1;

	if (mipi->cmb_mode == MIPI_NORMAL_MODE) {
		mipi_ctr.mipi_wdr_mode_sel = 0;
	} else if (mipi->cmb_mode == MIPI_VC_WDR_MODE) {
		mipi_ctr.mipi_wdr_mode_sel = 0;
		mipi_ctr.mipi_open_multi_ch = 1;
		mipi_ctr.mipi_ch0_height = mipi->format.height;
		mipi_ctr.mipi_ch1_height = mipi->format.height;
		mipi_ctr.mipi_ch2_height = mipi->format.height;
		mipi_ctr.mipi_ch3_height = mipi->format.height;
	} else if (mipi->cmb_mode == MIPI_DOL_WDR_MODE) {
		mipi_ctr.mipi_wdr_mode_sel = 2;
	}

	mipi_map.mipi_lane0 = MIPI_IN_L0_USE_PAD_LANE0;
	mipi_map.mipi_lane1 = MIPI_IN_L1_USE_PAD_LANE1;
	mipi_map.mipi_lane2 = MIPI_IN_L2_USE_PAD_LANE2;
	mipi_map.mipi_lane3 = MIPI_IN_L3_USE_PAD_LANE3;

	cmb_rx_mode_sel(mipi->id, D_PHY);
	cmb_rx_app_pixel_out(mipi->id, TWO_PIXEL);
	cmb_rx_mipi_stl_time(mipi->id, mipi->time_hs);
	cmb_rx_mipi_ctr(mipi->id, &mipi_ctr);
	cmb_rx_mipi_dphy_mapping(mipi->id, &mipi_map);
}

void combo_rx_sub_lvds_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct lvds_ctr lvds_ctr;

	memset(&lvds_ctr, 0, sizeof(lvds_ctr));
	lvds_ctr.lvds_bit_width = mipi->cmb_fmt->bit_width;
	lvds_ctr.lvds_lane_num = mipi->cmb_cfg.lvds_ln;

	if (mipi->cmb_mode == LVDS_NORMAL_MODE) {
		lvds_ctr.lvds_line_code_mode = 1;
	} else if (mipi->cmb_mode == LVDS_4CODE_WDR_MODE) {
		lvds_ctr.lvds_line_code_mode = mipi->wdr_cfg.line_code_mode;

		lvds_ctr.lvds_wdr_lbl_sel = 1;
		lvds_ctr.lvds_sync_code_line_cnt = mipi->wdr_cfg.line_cnt;

		lvds_ctr.lvds_code_mask = mipi->wdr_cfg.code_mask;
		lvds_ctr.lvds_wdr_fid_mode_sel = mipi->wdr_cfg.wdr_fid_mode_sel;
		lvds_ctr.lvds_wdr_fid_map_en = mipi->wdr_cfg.wdr_fid_map_en;
		lvds_ctr.lvds_wdr_fid0_map_sel = mipi->wdr_cfg.wdr_fid0_map_sel;
		lvds_ctr.lvds_wdr_fid1_map_sel = mipi->wdr_cfg.wdr_fid1_map_sel;
		lvds_ctr.lvds_wdr_fid2_map_sel = mipi->wdr_cfg.wdr_fid2_map_sel;
		lvds_ctr.lvds_wdr_fid3_map_sel = mipi->wdr_cfg.wdr_fid3_map_sel;

		lvds_ctr.lvds_wdr_en_multi_ch = mipi->wdr_cfg.wdr_en_multi_ch;
		lvds_ctr.lvds_wdr_ch0_height = mipi->wdr_cfg.wdr_ch0_height;
		lvds_ctr.lvds_wdr_ch1_height = mipi->wdr_cfg.wdr_ch1_height;
		lvds_ctr.lvds_wdr_ch2_height = mipi->wdr_cfg.wdr_ch2_height;
		lvds_ctr.lvds_wdr_ch3_height = mipi->wdr_cfg.wdr_ch3_height;
	} else if (mipi->cmb_mode == LVDS_5CODE_WDR_MODE) {
		lvds_ctr.lvds_line_code_mode = mipi->wdr_cfg.line_code_mode;
		lvds_ctr.lvds_wdr_lbl_sel = 2;
		lvds_ctr.lvds_sync_code_line_cnt = mipi->wdr_cfg.line_cnt;

		lvds_ctr.lvds_code_mask = mipi->wdr_cfg.code_mask;
	}

	cmb_rx_mode_sel(mipi->id, SUB_LVDS);
	cmb_rx_app_pixel_out(mipi->id, ONE_PIXEL);
	cmb_rx_lvds_ctr(mipi->id, &lvds_ctr);
	cmb_rx_lvds_mapping(mipi->id, &mipi->lvds_map);
	cmb_rx_lvds_sync_code(mipi->id, &mipi->sync_code);
}

void combo_rx_hispi_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct lvds_ctr lvds_ctr;
	struct hispi_ctr hispi_ctr;

	memset(&hispi_ctr, 0, sizeof(hispi_ctr));
	memset(&lvds_ctr, 0, sizeof(lvds_ctr));
	lvds_ctr.lvds_bit_width = mipi->cmb_fmt->bit_width;
	lvds_ctr.lvds_lane_num = mipi->cmb_cfg.lvds_ln;

	if (mipi->cmb_mode == HISPI_NORMAL_MODE) {
		lvds_ctr.lvds_line_code_mode = 0;
		lvds_ctr.lvds_pix_lsb = 1;

		hispi_ctr.hispi_normal = 1;
		hispi_ctr.hispi_trans_mode = PACKETIZED_SP;
	} else if (mipi->cmb_mode == HISPI_WDR_MODE) {
		lvds_ctr.lvds_line_code_mode = mipi->wdr_cfg.line_code_mode;

		lvds_ctr.lvds_wdr_lbl_sel = 1;

		lvds_ctr.lvds_pix_lsb = mipi->wdr_cfg.pix_lsb;
		lvds_ctr.lvds_sync_code_line_cnt = mipi->wdr_cfg.line_cnt;

		lvds_ctr.lvds_wdr_fid_mode_sel = mipi->wdr_cfg.wdr_fid_mode_sel;
		lvds_ctr.lvds_wdr_fid_map_en = mipi->wdr_cfg.wdr_fid_map_en;
		lvds_ctr.lvds_wdr_fid0_map_sel = mipi->wdr_cfg.wdr_fid0_map_sel;
		lvds_ctr.lvds_wdr_fid1_map_sel = mipi->wdr_cfg.wdr_fid1_map_sel;
		lvds_ctr.lvds_wdr_fid2_map_sel = mipi->wdr_cfg.wdr_fid2_map_sel;
		lvds_ctr.lvds_wdr_fid3_map_sel = mipi->wdr_cfg.wdr_fid3_map_sel;

		hispi_ctr.hispi_normal = 1;
		hispi_ctr.hispi_trans_mode = PACKETIZED_SP;
		hispi_ctr.hispi_wdr_en = 1;
		hispi_ctr.hispi_wdr_sof_fild = mipi->wdr_cfg.wdr_sof_fild;
		hispi_ctr.hispi_wdr_eof_fild = mipi->wdr_cfg.wdr_eof_fild;
		hispi_ctr.hispi_code_mask = mipi->wdr_cfg.code_mask;
	}

	cmb_rx_mode_sel(mipi->id, SUB_LVDS);
	cmb_rx_app_pixel_out(mipi->id, ONE_PIXEL);
	cmb_rx_lvds_ctr(mipi->id, &lvds_ctr);
	cmb_rx_lvds_mapping(mipi->id, &mipi->lvds_map);
	cmb_rx_lvds_sync_code(mipi->id, &mipi->sync_code);

	cmb_rx_hispi_ctr(mipi->id, &hispi_ctr);
}

void combo_rx_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	/* comnbo rx phya init */
	cmb_rx_phya_config(mipi->id);

	if (mipi->terminal_resistance) {
		vin_log(VIN_LOG_MIPI, "open combo terminal resitance!\n");
		cmb_rx_te_auto_disable(mipi->id, 1);
		cmb_rx_phya_a_d0_en(mipi->id, 1);
		cmb_rx_phya_b_d0_en(mipi->id, 1);
		cmb_rx_phya_c_d0_en(mipi->id, 1);
		cmb_rx_phya_a_d1_en(mipi->id, 1);
		cmb_rx_phya_b_d1_en(mipi->id, 1);
		cmb_rx_phya_c_d1_en(mipi->id, 1);
		cmb_rx_phya_a_d2_en(mipi->id, 1);
		cmb_rx_phya_b_d2_en(mipi->id, 1);
		cmb_rx_phya_c_d2_en(mipi->id, 1);
		cmb_rx_phya_a_d3_en(mipi->id, 1);
		cmb_rx_phya_b_d3_en(mipi->id, 1);
		cmb_rx_phya_c_d3_en(mipi->id, 1);
		cmb_rx_phya_a_ck_en(mipi->id, 1);
		cmb_rx_phya_b_ck_en(mipi->id, 1);
		cmb_rx_phya_c_ck_en(mipi->id, 1);
	}
	cmb_rx_phya_signal_dly_en(mipi->id, 1);
	cmb_rx_phya_offset(mipi->id, mipi->pyha_offset);

	switch (mipi->if_type) {
	case V4L2_MBUS_PARALLEL:
	case V4L2_MBUS_BT656:
		cmb_rx_mode_sel(mipi->id, CMOS);
		cmb_rx_app_pixel_out(mipi->id, ONE_PIXEL);
		break;
	case V4L2_MBUS_CSI2:
		cmb_rx_phya_ck_mode(mipi->id, 0);
		combo_rx_mipi_init(sd);
		break;
	case V4L2_MBUS_SUBLVDS:
		cmb_rx_phya_ck_mode(mipi->id, 1);
		combo_rx_sub_lvds_init(sd);
		break;
	case V4L2_MBUS_HISPI:
		cmb_rx_phya_ck_mode(mipi->id, 0);
		combo_rx_hispi_init(sd);
		break;
	default:
		combo_rx_mipi_init(sd);
		break;
	}

	cmb_rx_enable(mipi->id);
}
#elif defined MIPI_PING_CONFIG
void cmb_phy_init(struct mipi_dev *mipi)
{
	cmb_phy_lane_num_en(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_phy0_work_mode(mipi->id + mipi->phy_offset, 0);
	cmb_phy0_ofscal_cfg(mipi->id + mipi->phy_offset);
	//cmb_phy_deskew_en(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_term_ctl(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_hs_ctl(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_s2p_ctl(mipi->id + mipi->phy_offset, mipi->time_hs, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_mipirx_ctl(mipi->id + mipi->phy_offset, mipi->cmb_csi_cfg.phy_lane_cfg);
	cmb_phy0_en(mipi->id + mipi->phy_offset, 1);
	cmb_phy0_freq_en(mipi->id, 1);
	cmb_phy_deskew1_cfg(mipi->id + mipi->phy_offset, mipi->deskew, mipi->cmb_mode == MIPI_VC_WDR_MODE ? true : false);
}

static void combo_csi_link_mode_set(struct v4l2_subdev *sd)
{
	int link_mode_tmp;
	__maybe_unused struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	cmb_phy_link_mode_get(&link_mode_tmp);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	if (mipi->set_lane_choice[0] == 4 || mipi->set_lane_choice[2] == 4) {
		if (mipi->set_lane_choice[0] == 4 &&  mipi->set_lane_choice[1] == 0 && link_mode_tmp == FOUR_2LANE)
			cmb_phy_link_mode_set(ONE_4LANE_PHYA);
		else if ((mipi->set_lane_choice[2] == 4 &&  mipi->set_lane_choice[3] == 0 && link_mode_tmp == FOUR_2LANE))
			cmb_phy_link_mode_set(ONE_4LANE_PHYC);
		else if ((mipi->set_lane_choice[0] == 4 &&  mipi->set_lane_choice[1] == 0 && link_mode_tmp == ONE_4LANE_PHYC)
		|| (mipi->set_lane_choice[2] == 4 &&  mipi->set_lane_choice[3] == 0 && link_mode_tmp == ONE_4LANE_PHYA))
			cmb_phy_link_mode_set(TWO_4LANE);
		else if ((mipi->set_lane_choice[0] == 4 &&  mipi->set_lane_choice[1] == 0 && link_mode_tmp == ONE_4LANE_PHYA)
		|| (mipi->set_lane_choice[0] == 4 &&  mipi->set_lane_choice[1] == 0 && link_mode_tmp == TWO_4LANE)
		|| (mipi->set_lane_choice[2] == 4 &&  mipi->set_lane_choice[3] == 0 && link_mode_tmp == ONE_4LANE_PHYC)
		|| (mipi->set_lane_choice[2] == 4 &&  mipi->set_lane_choice[3] == 0 && link_mode_tmp == TWO_4LANE))
			vin_log(VIN_LOG_MIPI, "mipi link mode already set!\n");
		else
			vin_err("phy link mode set error!\n");
	} else {
		vin_err("phy link mode set error, mipi sel set error!\n");
	}
#elif IS_ENABLED(CONFIG_ARCH_SUN300IW1)
	cmb_phy_link_mode_set(ONE_2LANE);
#endif
}

static void combo_csi_mipi_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &mipi->format;
	int i;

	mipi->cmb_csi_cfg.phy_lane_cfg.phy_laneck_en = CK_1LANE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpck_en = LPCK_1LANE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_termck_en = TERMCK_CLOSE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_termdt_en = TERMDT_CLOSE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_s2p_en = S2PDT_CLOSE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_hsck_en = HSCK_CLOSE;
	mipi->cmb_csi_cfg.phy_lane_cfg.phy_hsdt_en = HSDT_CLOSE;

	cmb_phy_init(mipi);

	/* if need two mipi dev,needs set it */
	if (mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][0]) {
		mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpck_en = LPCK_CLOSE;
		mipi->phy_offset = 1;
		combo_csi_link_mode_set(sd);
		cmb_phy_init(mipi);
		mipi->phy_offset = 0;
	}

	for (i = 0; i < mipi->cmb_csi_cfg.lane_num; i++)
		mipi->cmb_csi_cfg.mipi_lane[i] = cmb_port_set_lane_map(mipi->id, i);

	for (i = 0; i < mipi->cmb_csi_cfg.total_rx_ch; i++) {
		mipi->cmb_csi_cfg.mipi_datatype[i] = get_pkt_fmt(mipi->format.code);
		mipi->cmb_csi_cfg.vc[i] = i;
		mipi->cmb_csi_cfg.field[i] = mf->field;
		cmb_port_mipi_ch_trigger_en(mipi->id, i, 1);
		if (mipi->cmb_fmt->bit_width == RAW16) {
			cmb_port_mipi_raw_extend_en(mipi->id, i, 1);
			mipi->cmb_csi_cfg.mipi_datatype[i] = MIPI_RAW8;
		}
	}

	cmb_port_lane_num(mipi->id, mipi->cmb_csi_cfg.lane_num);
	if (mipi->cmb_csi_cfg.lane_num == 4)
		cmb_port_out_num(mipi->id, TWO_DATA);
	else
		cmb_port_out_num(mipi->id, ONE_DATA);
	if (mipi->cmb_mode == MIPI_DOL_WDR_MODE)
		cmb_port_out_chnum(mipi->id, 0);
	else
		cmb_port_out_chnum(mipi->id, mipi->cmb_csi_cfg.total_rx_ch - 1);
	cmb_port_lane_map(mipi->id, mipi->cmb_csi_cfg.mipi_lane);
	cmb_port_mipi_cfg(mipi->id, mipi->cmb_fmt->yuv_seq);
	cmb_port_set_mipi_datatype(mipi->id, &mipi->cmb_csi_cfg);
	cmb_port_mipi_set_field(mipi->id, mipi->cmb_csi_cfg.total_rx_ch, &mipi->cmb_csi_cfg);
	if (mipi->cmb_mode == MIPI_DOL_WDR_MODE)
		cmb_port_set_mipi_wdr(mipi->id, 0, 2);
	cmb_port_enable(mipi->id);
}

void combo_csi_init(struct v4l2_subdev *sd)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	switch (mipi->if_type) {
	case V4L2_MBUS_PARALLEL:
	case V4L2_MBUS_BT656:
		break;
	case V4L2_MBUS_CSI2_DPHY:
		combo_csi_mipi_init(sd);
		break;
	case V4L2_MBUS_CSI2_CPHY:
		break;
	case V4L2_MBUS_CSI1:
		break;
	case V4L2_MBUS_CCP2:
		break;
	case V4L2_MBUS_SUBLVDS:
		break;
	case V4L2_MBUS_HISPI:
		break;
	case V4L2_MBUS_UNKNOWN:
		break;
	default:
		break;
	}
}
#endif

static int sunxi_mipi_subdev_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf = &mipi->format;
	struct mbus_framefmt_res *res = (void *)mf->reserved;
	int i;
	__maybe_unused int cur_phy_link;

	mipi->csi2_cfg.bps = res->res_mipi_bps;
	mipi->csi2_cfg.auto_check_bps = 0;
	mipi->csi2_cfg.dphy_freq = DPHY_CLK;

	for (i = 0; i < mipi->csi2_cfg.total_rx_ch; i++) {
		mipi->csi2_fmt.packet_fmt[i] = get_pkt_fmt(mf->code);
		mipi->csi2_fmt.field[i] = mf->field;
		mipi->csi2_fmt.vc[i] = i;
	}

	mipi->csi2_fmt.fmt_type = data_formats_type(get_pkt_fmt(mf->code));
	mipi->cmb_mode = res->res_combo_mode & 0xf;
	mipi->terminal_resistance = res->res_combo_mode & CMB_TERMINAL_RES;
	mipi->pyha_offset = (res->res_combo_mode & 0x70) >> 4;
	if (mipi->settle_time > 0) {
		mipi->time_hs = mipi->settle_time;
	} else if (res->res_time_hs)
		mipi->time_hs = res->res_time_hs;
	else {
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
		mipi->time_hs = 0x30;
#else
		mipi->time_hs = 0x28;
#endif
	}
	if (res->res_deskew)
		mipi->deskew = res->res_deskew;

#if defined MIPI_PING_CONFIG
	__mcsi_pin_config(mipi, enable);
#endif

	if (enable) {
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
		combo_rx_init(sd);
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW10) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
		combo_csi_init(sd);
#else
		bsp_mipi_csi_dphy_init(mipi->id);
		mipi_dphy_cfg_1data(mipi->id, 0x75,
			mipi->settle_time ? mipi->settle_time : 0xa0);
		bsp_mipi_csi_set_para(mipi->id, &mipi->csi2_cfg);
		bsp_mipi_csi_set_fmt(mipi->id, mipi->csi2_cfg.total_rx_ch,
				     &mipi->csi2_fmt);
		if (mipi->cmb_mode == MIPI_DOL_WDR_MODE)
			bsp_mipi_csi_set_dol(mipi->id, 0, 2);
		/* for dphy clock async */
		bsp_mipi_csi_dphy_disable(mipi->id, mipi->sensor_flags);
		bsp_mipi_csi_dphy_enable(mipi->id, mipi->sensor_flags);
		bsp_mipi_csi_protocol_enable(mipi->id);
#endif
	} else {
#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
		cmb_rx_disable(mipi->id);
#elif defined MIPI_COMBO_CSI
		cmb_port_disable(mipi->id);
		cmb_phy0_en(mipi->id, 0);
		cmb_phy0_freq_en(mipi->id, 0);
#else
		bsp_mipi_csi_dphy_disable(mipi->id, mipi->sensor_flags);
		bsp_mipi_csi_protocol_disable(mipi->id);
		bsp_mipi_csi_dphy_exit(mipi->id);
#endif
	}

	vin_log(VIN_LOG_FMT, "%s%d %s, lane_num %d, code: %x field: %d\n",
		mipi->if_name, mipi->id, enable ? "stream on" : "stream off",
		mipi->cmb_cfg.lane_num, mf->code, mf->field);

	return 0;
}

static struct combo_format *__mipi_try_format(struct v4l2_mbus_framefmt *mf)
{
	struct combo_format *mipi_fmt = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_mipi_formats); i++)
		if (mf->code == sunxi_mipi_formats[i].code)
			mipi_fmt = &sunxi_mipi_formats[i];

	if (mipi_fmt == NULL)
		mipi_fmt = &sunxi_mipi_formats[0];

	mf->code = mipi_fmt->code;

	return mipi_fmt;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
static int sunxi_mipi_subdev_set_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *state,
				     struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;
	struct combo_format *mipi_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = &mipi->format;

	if (fmt->pad == MIPI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&mipi->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&mipi->subdev_lock);
		}
		return 0;
	}

	mipi_fmt = __mipi_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&mipi->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			mipi->cmb_fmt = mipi_fmt;
		mutex_unlock(&mipi->subdev_lock);
	}

	return 0;
}

static int sunxi_mipi_subdev_get_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_state *state,
				     struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = &mipi->format;
	if (!mf)
		return -EINVAL;

	mutex_lock(&mipi->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&mipi->subdev_lock);
	return 0;
}

#else /* before linux-5.15 */
static int sunxi_mipi_subdev_set_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_pad_config *cfg,
				     struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;
	struct combo_format *mipi_fmt;

	vin_log(VIN_LOG_FMT, "%s %d*%d %x %d\n", __func__,
		fmt->format.width, fmt->format.height,
		fmt->format.code, fmt->format.field);

	mf = &mipi->format;

	if (fmt->pad == MIPI_PAD_SOURCE) {
		if (mf) {
			mutex_lock(&mipi->subdev_lock);
			fmt->format = *mf;
			mutex_unlock(&mipi->subdev_lock);
		}
		return 0;
	}

	mipi_fmt = __mipi_try_format(&fmt->format);
	if (mf) {
		mutex_lock(&mipi->subdev_lock);
		*mf = fmt->format;
		if (fmt->which == V4L2_SUBDEV_FORMAT_ACTIVE)
			mipi->cmb_fmt = mipi_fmt;
		mutex_unlock(&mipi->subdev_lock);
	}

	return 0;
}

static int sunxi_mipi_subdev_get_fmt(struct v4l2_subdev *sd,
				     struct v4l2_subdev_pad_config *cfg,
				     struct v4l2_subdev_format *fmt)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	struct v4l2_mbus_framefmt *mf;

	mf = &mipi->format;
	if (!mf)
		return -EINVAL;

	mutex_lock(&mipi->subdev_lock);
	fmt->format = *mf;
	mutex_unlock(&mipi->subdev_lock);
	return 0;
}
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static int sunxi_mipi_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				    struct v4l2_mbus_config *cfg)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2_DPHY) {
		mipi->if_type = V4L2_MBUS_CSI2_DPHY;
		memcpy(mipi->if_name, "mipi_dphy", sizeof("mipi_dphy"));
		if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_4_LANE)) {
			mipi->csi2_cfg.lane_num = 4;
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.mipi_ln = MIPI_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_4LANE;
			mipi->cmb_csi_cfg.lane_num = 4;
			if (mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][1])
				vin_err("PORT%d supports a maximum of %dlane!\n", mipi->id, mipi_lane_spec[mipi->id][1]);
			if (mipi->id < (VIN_MAX_MIPI - 1) && mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][0] && glb_mipi[mipi->id + 1]->stream_flag)
				vin_err("PORT%d in using, PORT%d cannot %dlane!\n", mipi->id + 1, mipi->id, mipi->cmb_csi_cfg.lane_num);
			mipi->set_lane_choice[mipi->id] = 4;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_3_LANE)) {
			mipi->csi2_cfg.lane_num = 3;
			mipi->cmb_cfg.lane_num = 3;
			mipi->cmb_cfg.mipi_ln = MIPI_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_3LANE;
			mipi->cmb_csi_cfg.lane_num = 3;
			if (mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][1])
				vin_err("PORT%d supports a maximum of %dlane!\n", mipi->id, mipi_lane_spec[mipi->id][1]);
			if (mipi->id < (VIN_MAX_MIPI - 1) && mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][0] && glb_mipi[mipi->id + 1]->stream_flag)
				vin_err("PORT%d in using, PORT%d cannot %dlane!\n", mipi->id + 1, mipi->id, mipi->cmb_csi_cfg.lane_num);
			mipi->set_lane_choice[mipi->id] = 4;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_2_LANE)) {
			mipi->csi2_cfg.lane_num = 2;
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.mipi_ln = MIPI_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_2LANE;
			mipi->cmb_csi_cfg.lane_num = 2;
			mipi->set_lane_choice[mipi->id] = 2;
		} else {
			mipi->cmb_cfg.lane_num = 1;
			mipi->csi2_cfg.lane_num = 1;
			mipi->cmb_cfg.mipi_ln = MIPI_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_1LANE;
			mipi->cmb_csi_cfg.lane_num = 1;
			mipi->set_lane_choice[mipi->id] = 2;
		}
	}  else if (cfg->type == V4L2_MBUS_HISPI) {
			mipi->if_type = V4L2_MBUS_CSI2_CPHY;
			memcpy(mipi->if_name, "mipi_cphy", sizeof("mipi_cphy"));
	} else if (cfg->type == V4L2_MBUS_SUBLVDS) {
		mipi->if_type = V4L2_MBUS_SUBLVDS;
		memcpy(mipi->if_name, "sublvds", sizeof("sublvds"));
		if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		}
	} else if (cfg->type == V4L2_MBUS_HISPI) {
		mipi->if_type = V4L2_MBUS_HISPI;
		memcpy(mipi->if_name, "hispi", sizeof("hispi"));
		if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		}
	}  else if (cfg->type == V4L2_MBUS_CCP2) {
			mipi->if_type = V4L2_MBUS_CCP2;
			memcpy(mipi->if_name, "ccp2", sizeof("ccp2"));

	} else if (cfg->type == V4L2_MBUS_CSI1) {
			mipi->if_type = V4L2_MBUS_CSI1;
			memcpy(mipi->if_name, "mipi_csi1", sizeof("mipi_csi1"));

	} else if (cfg->type == V4L2_MBUS_PARALLEL) {
			mipi->if_type = V4L2_MBUS_PARALLEL;
			memcpy(mipi->if_name, "combo_parallel", sizeof("combo_parallel"));

	} else {
			memcpy(mipi->if_name, "combo_unknown", sizeof("combo_unknown"));
			mipi->if_type = V4L2_MBUS_UNKNOWN;
	}

	mipi->csi2_cfg.total_rx_ch = 0;
	mipi->cmb_csi_cfg.total_rx_ch = 0;
	if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_CHANNEL_0)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_CHANNEL_1)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_CHANNEL_2)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->bus.mipi_csi2.num_data_lanes, V4L2_MBUS_CSI2_CHANNEL_3)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}

	return 0;
}
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
static int sunxi_mipi_s_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				    struct v4l2_mbus_config *cfg)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2_DPHY) {
		mipi->if_type = V4L2_MBUS_CSI2_DPHY;
		memcpy(mipi->if_name, "mipi_dphy", sizeof("mipi_dphy"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_4_LANE)) {
			mipi->csi2_cfg.lane_num = 4;
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.mipi_ln = MIPI_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_4LANE;
			mipi->cmb_csi_cfg.lane_num = 4;
			if (mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][1])
				vin_err("PORT%d supports a maximum of %dlane!\n", mipi->id, mipi_lane_spec[mipi->id][1]);
			if (mipi->id < (VIN_MAX_MIPI - 1) && mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][0] && glb_mipi[mipi->id + 1]->stream_flag)
				vin_err("PORT%d in using, PORT%d cannot %dlane!\n", mipi->id + 1, mipi->id, mipi->cmb_csi_cfg.lane_num);

			mipi->set_lane_choice[mipi->id] = 4;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_3_LANE)) {
			mipi->csi2_cfg.lane_num = 3;
			mipi->cmb_cfg.lane_num = 3;
			mipi->cmb_cfg.mipi_ln = MIPI_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_3LANE;
			mipi->cmb_csi_cfg.lane_num = 3;
			if (mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][1])
				vin_err("PORT%d supports a maximum of %dlane!\n", mipi->id, mipi_lane_spec[mipi->id][1]);
			if (mipi->id < (VIN_MAX_MIPI - 1) && mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][0] && glb_mipi[mipi->id + 1]->stream_flag)
				vin_err("PORT%d in using, PORT%d cannot %dlane!\n", mipi->id + 1, mipi->id, mipi->cmb_csi_cfg.lane_num);
			mipi->set_lane_choice[mipi->id] = 4;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_2_LANE)) {
			mipi->csi2_cfg.lane_num = 2;
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.mipi_ln = MIPI_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_2LANE;
			mipi->cmb_csi_cfg.lane_num = 2;
			mipi->set_lane_choice[mipi->id] = 2;
		} else {
			mipi->cmb_cfg.lane_num = 1;
			mipi->csi2_cfg.lane_num = 1;
			mipi->cmb_cfg.mipi_ln = MIPI_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_1LANE;
			mipi->cmb_csi_cfg.lane_num = 1;
			mipi->set_lane_choice[mipi->id] = 2;
		}
	} else if (cfg->type == V4L2_MBUS_SUBLVDS) {
		mipi->if_type = V4L2_MBUS_SUBLVDS;
		memcpy(mipi->if_name, "sublvds", sizeof("sublvds"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		}
	} else if (cfg->type == V4L2_MBUS_HISPI) {
		mipi->if_type = V4L2_MBUS_HISPI;
		memcpy(mipi->if_name, "hispi", sizeof("hispi"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		}
	}  else if (cfg->type == V4L2_MBUS_CCP2) {
			mipi->if_type = V4L2_MBUS_CCP2;
			memcpy(mipi->if_name, "ccp2", sizeof("ccp2"));

	} else if (cfg->type == V4L2_MBUS_CSI1) {
			mipi->if_type = V4L2_MBUS_CSI1;
			memcpy(mipi->if_name, "mipi_csi1", sizeof("mipi_csi1"));

	} else if (cfg->type == V4L2_MBUS_PARALLEL) {
			mipi->if_type = V4L2_MBUS_PARALLEL;
			memcpy(mipi->if_name, "combo_parallel", sizeof("combo_parallel"));

	} else {
			memcpy(mipi->if_name, "combo_unknown", sizeof("combo_unknown"));
			mipi->if_type = V4L2_MBUS_UNKNOWN;
	}

	mipi->csi2_cfg.total_rx_ch = 0;
	mipi->cmb_csi_cfg.total_rx_ch = 0;
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}

	return 0;
}
#else
static int sunxi_mipi_s_mbus_config(struct v4l2_subdev *sd,
				    const struct v4l2_mbus_config *cfg)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);

	if (cfg->type == V4L2_MBUS_CSI2_DPHY) {
		mipi->if_type = V4L2_MBUS_CSI2_DPHY;
		memcpy(mipi->if_name, "mipi_dphy", sizeof("mipi_dphy"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_4_LANE)) {
			mipi->csi2_cfg.lane_num = 4;
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.mipi_ln = MIPI_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_4LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_4LANE;
			mipi->cmb_csi_cfg.lane_num = 4;
			if (mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][1])
				vin_err("PORT%d supports a maximum of %dlane!\n", mipi->id, mipi_lane_spec[mipi->id][1]);
			if (mipi->id < (VIN_MAX_MIPI - 1) && mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][0] && glb_mipi[mipi->id + 1]->stream_flag)
				vin_err("PORT%d in using, PORT%d cannot %dlane!\n", mipi->id + 1, mipi->id, mipi->cmb_csi_cfg.lane_num);

			mipi->set_lane_choice[mipi->id] = 4;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_3_LANE)) {
			mipi->csi2_cfg.lane_num = 3;
			mipi->cmb_cfg.lane_num = 3;
			mipi->cmb_cfg.mipi_ln = MIPI_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_3LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_3LANE;
			mipi->cmb_csi_cfg.lane_num = 3;
			if (mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][1])
				vin_err("PORT%d supports a maximum of %dlane!\n", mipi->id, mipi_lane_spec[mipi->id][1]);
			if (mipi->id < (VIN_MAX_MIPI - 1) && mipi->cmb_csi_cfg.lane_num > mipi_lane_spec[mipi->id][0] && glb_mipi[mipi->id + 1]->stream_flag)
				vin_err("PORT%d in using, PORT%d cannot %dlane!\n", mipi->id + 1, mipi->id, mipi->cmb_csi_cfg.lane_num);
			mipi->set_lane_choice[mipi->id] = 4;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_2_LANE)) {
			mipi->csi2_cfg.lane_num = 2;
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.mipi_ln = MIPI_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_2LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_2LANE;
			mipi->cmb_csi_cfg.lane_num = 2;
			mipi->set_lane_choice[mipi->id] = 2;
		} else {
			mipi->cmb_cfg.lane_num = 1;
			mipi->csi2_cfg.lane_num = 1;
			mipi->cmb_cfg.mipi_ln = MIPI_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_lanedt_en = DT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_mipi_lpdt_en = LPDT_1LANE;
			mipi->cmb_csi_cfg.phy_lane_cfg.phy_deskew_en = DK_1LANE;
			mipi->cmb_csi_cfg.lane_num = 1;
			mipi->set_lane_choice[mipi->id] = 2;
		}
	} else if (cfg->type == V4L2_MBUS_SUBLVDS) {
		mipi->if_type = V4L2_MBUS_SUBLVDS;
		memcpy(mipi->if_name, "sublvds", sizeof("sublvds"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		}
	} else if (cfg->type == V4L2_MBUS_HISPI) {
		mipi->if_type = V4L2_MBUS_HISPI;
		memcpy(mipi->if_name, "hispi", sizeof("hispi"));
		if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_12_LANE)) {
			mipi->cmb_cfg.lane_num = 12;
			mipi->cmb_cfg.lvds_ln = LVDS_12LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_10_LANE)) {
			mipi->cmb_cfg.lane_num = 10;
			mipi->cmb_cfg.lvds_ln = LVDS_10LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_8_LANE)) {
			mipi->cmb_cfg.lane_num = 8;
			mipi->cmb_cfg.lvds_ln = LVDS_8LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_4_LANE)) {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		} else if (IS_FLAG(cfg->flags, V4L2_MBUS_SUBLVDS_2_LANE)) {
			mipi->cmb_cfg.lane_num = 2;
			mipi->cmb_cfg.lvds_ln = LVDS_2LANE;
		} else {
			mipi->cmb_cfg.lane_num = 4;
			mipi->cmb_cfg.lvds_ln = LVDS_4LANE;
		}
	}  else if (cfg->type == V4L2_MBUS_CCP2) {
			mipi->if_type = V4L2_MBUS_CCP2;
			memcpy(mipi->if_name, "ccp2", sizeof("ccp2"));

	} else if (cfg->type == V4L2_MBUS_CSI1) {
			mipi->if_type = V4L2_MBUS_CSI1;
			memcpy(mipi->if_name, "mipi_csi1", sizeof("mipi_csi1"));

	} else if (cfg->type == V4L2_MBUS_PARALLEL) {
			mipi->if_type = V4L2_MBUS_PARALLEL;
			memcpy(mipi->if_name, "combo_parallel", sizeof("combo_parallel"));

	} else {
			memcpy(mipi->if_name, "combo_unknown", sizeof("combo_unknown"));
			mipi->if_type = V4L2_MBUS_UNKNOWN;
	}

	mipi->csi2_cfg.total_rx_ch = 0;
	mipi->cmb_csi_cfg.total_rx_ch = 0;
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_0)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_1)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_2)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}
	if (IS_FLAG(cfg->flags, V4L2_MBUS_CSI2_CHANNEL_3)) {
		mipi->csi2_cfg.total_rx_ch++;
		mipi->cmb_csi_cfg.total_rx_ch++;
	}

	return 0;
}
#endif

static void mipi_get_ch_field(struct mipi_dev *mipi, int ch, int *field)
{
	if (!mipi->tvin.flag)
		return;

	switch (mipi->tvin.tvin_info.input_fmt[ch]) {
	case CVBS_PAL:
	case CVBS_NTSC:
	case YCBCR_576I:
	case YCBCR_480I:
	case CVBS_H1440_PAL:
	case CVBS_H1440_NTSC:
	case CVBS_FRAME_PAL:
	case CVBS_FRAME_NTSC:
		*field = 1; // interlace
		break;
	case AHD720P25:
	case AHD720P30:
	case AHD1080P25:
	case AHD1080P30:
	case YCBCR_480P:
	case YCBCR_576P:
		*field = 0; // progress
		break;
	default:
		*field = 0;
	}
}

static int mipi_tvin_init(struct v4l2_subdev *sd,
			struct tvin_init_info *info)
{
	struct mipi_dev *mipi = v4l2_get_subdevdata(sd);
	int ch = info->ch_id;
	int ch_field;

	if (ch > TVIN_SEPARATE) {
		vin_err("[%s]mipi ch can not over %d\n", __func__, TVIN_SEPARATE);
		return -1;
	}

	mipi->tvin.tvin_info.input_fmt[ch] = info->input_fmt[ch];
	mipi->tvin.flag = true;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (mipi->stream_count != 0) {
#else
	if (sd->entity.stream_count != 0) {
#endif
		mipi_get_ch_field(mipi, ch, &ch_field);
		vin_log(VIN_LOG_MIPI, "mipi%d get field is %d!\r\n", mipi->id, ch_field);
#if defined  MIPI_COMBO_CSI
		cmb_port_mipi_set_ch_field(mipi->id, ch, ch_field);
#endif
	}
	return 0;
}

static long mipi_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;

	switch (cmd) {
	case MIPI_TVIN_INIT:
		ret = mipi_tvin_init(sd, (struct tvin_init_info *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static const struct v4l2_subdev_core_ops sunxi_mipi_subdev_core_ops = {
	.ioctl = mipi_ioctl,
};
static const struct v4l2_subdev_video_ops sunxi_mipi_subdev_video_ops = {
	.s_stream = sunxi_mipi_subdev_s_stream,
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0)
	.s_mbus_config = sunxi_mipi_s_mbus_config,
#endif
};

static const struct v4l2_subdev_pad_ops sunxi_mipi_subdev_pad_ops = {
	.get_fmt = sunxi_mipi_subdev_get_fmt,
	.set_fmt = sunxi_mipi_subdev_set_fmt,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	.get_mbus_config = sunxi_mipi_g_mbus_config,
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	.set_mbus_config = sunxi_mipi_s_mbus_config,
#endif
};

static struct v4l2_subdev_ops sunxi_mipi_subdev_ops = {
	.core = &sunxi_mipi_subdev_core_ops,
	.video = &sunxi_mipi_subdev_video_ops,
	.pad = &sunxi_mipi_subdev_pad_ops,
};

static int __mipi_init_subdev(struct mipi_dev *mipi)
{
	struct v4l2_subdev *sd = &mipi->subdev;
	int ret;

	mutex_init(&mipi->subdev_lock);
	v4l2_subdev_init(sd, &sunxi_mipi_subdev_ops);
	sd->grp_id = VIN_GRP_ID_MIPI;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	snprintf(sd->name, sizeof(sd->name), "sunxi_mipi.%u", mipi->id);
	v4l2_set_subdevdata(sd, mipi);

	/* sd->entity->ops = &isp_media_ops; */
	mipi->mipi_pads[MIPI_PAD_SINK].flags = MEDIA_PAD_FL_SINK;
	mipi->mipi_pads[MIPI_PAD_SOURCE].flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_IO_V4L;

	ret = media_entity_pads_init(&sd->entity, MIPI_PAD_NUM, mipi->mipi_pads);
	if (ret < 0)
		return ret;

	return 0;
}

static int mipi_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct mipi_dev *mipi = NULL;
	int ret = 0;

	if (np == NULL) {
		vin_err("MIPI failed to get of node\n");
		return -ENODEV;
	}

	mipi = kzalloc(sizeof(*mipi), GFP_KERNEL);
	if (!mipi) {
		ret = -ENOMEM;
		goto ekzalloc;
	}

	of_property_read_u32(np, "device_id", &pdev->id);
	if (pdev->id < 0) {
		vin_err("MIPI failed to get device id\n");
		ret = -EINVAL;
		goto freedev;
	}

	mipi->base = of_iomap(np, 0);
	if (!mipi->base) {
		ret = -EIO;
		goto freedev;
	}
	mipi->id = pdev->id;
	mipi->pdev = pdev;

#if IS_ENABLED(CONFIG_ARCH_SUN8IW16P1)
	cmb_rx_set_base_addr(mipi->id, (unsigned long)mipi->base);
#elif IS_ENABLED(CONFIG_ARCH_SUN50IW10) || IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW1) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	cmb_csi_set_phy_base_addr(mipi->id, (unsigned long)mipi->base);
	mipi->port_base = of_iomap(np, 1);
	if (!mipi->port_base) {
		ret = -EIO;
		goto freedev;
	}
	cmb_csi_set_port_base_addr(mipi->id, (unsigned long)mipi->port_base);
#else
	bsp_mipi_csi_set_base_addr(mipi->id, (unsigned long)mipi->base);
	bsp_mipi_dphy_set_base_addr(mipi->id, (unsigned long)mipi->base + 0x1000);
#endif

	ret = __mipi_init_subdev(mipi);
	if (ret < 0) {
		vin_err("mipi init error!\n");
		goto unmap;
	}

	platform_set_drvdata(pdev, mipi);
	glb_mipi[mipi->id] = mipi;
	ret = device_create_file(&pdev->dev, &dev_attr_settle_time);
	if (ret) {
		vin_err("mipi settle time node register fail\n");
		return ret;
	}
	vin_log(VIN_LOG_MIPI, "mipi%d probe end!\n", mipi->id);
	return 0;

unmap:
	iounmap(mipi->base);
freedev:
	kfree(mipi);
ekzalloc:
	vin_err("mipi probe err!\n");
	return ret;
}

static int mipi_remove(struct platform_device *pdev)
{
	struct mipi_dev *mipi = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &mipi->subdev;

	device_remove_file(&pdev->dev, &dev_attr_settle_time);
	platform_set_drvdata(pdev, NULL);
	v4l2_set_subdevdata(sd, NULL);
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	__mcsi_pin_release(mipi);
#endif
	if (mipi->port_base)
		iounmap(mipi->port_base);
	if (mipi->base)
		iounmap(mipi->base);
	media_entity_cleanup(&mipi->subdev.entity);
	kfree(mipi);
	return 0;
}

static size_t phy_common_status_dump(char *buf, size_t size)
{
	size_t count = 0;

#if defined MIPI_COMBO_CSI
	int cur_phy_link;

	cmb_phy_link_mode_get(&cur_phy_link);
	switch (cur_phy_link) {
#if IS_ENABLED(CONFIG_ARCH_SUN55IW3) || IS_ENABLED(CONFIG_ARCH_SUN55IW6)
	case FOUR_2LANE:
		count += scnprintf(buf + count, size - count,
				"phy_link_mode:\t 2'b00 (4x2-lane)\n");
		break;
	case ONE_4LANE_PHYA:
		count += scnprintf(buf + count, size - count,
				"phy_link_mode:\t 2'b01 (2x2-lane + 1x4-lane(phya+phyb))\n");
		break;
	case ONE_4LANE_PHYC:
		count += scnprintf(buf + count, size - count,
				"phy_link_mode:\t 2'b10 (2x2-lane + 1x4-lane(phyc+phyd))\n");
		break;
	case TWO_4LANE:
		count += scnprintf(buf + count, size - count,
				"phy_link_mode:\t 2'b11 (2x4-lane(phya+phyb), phyc+phyd))\n");
		break;
#endif
	default:
		count += scnprintf(buf + count, size - count,
				"phy_link_mode:\t 0x%x (unrecognized link mode)\n", cur_phy_link);
		break;
	}
	count += scnprintf(buf + count, size - count, "\n");
#endif

	return count;
}

static size_t phy_digital_status_dump(struct mipi_dev *mipi, char *buf, size_t size)
{
	size_t count = 0;

#if defined MIPI_COMBO_CSI
	struct vin_md *vind = dev_get_drvdata(mipi->subdev.v4l2_dev->dev);
	unsigned int phy_en_status;
	unsigned int phy_lanedt_en_status;
	unsigned int phy_laneck_en_status;
	unsigned int phy_deskew_laneck0;
	unsigned int phy0_s2p_dly;
	unsigned int dphy_clk;
	unsigned int phy_freq_cnt;
	unsigned int mipi_bps;
	unsigned int phy_sta_d0;
	unsigned int phy_sta_d1;
	unsigned int phy_sta_ck0;

	count += scnprintf(buf + count, size - count, "[mipi%u]\n", mipi->id);

	// 0x0000: PHY Control Register (PHY_EN, PHY_LANEDT_EN, PHY_LANECK_EN)
	phy_en_status = cmb_phy_en_status_get(mipi->id);
	count += scnprintf(buf + count, size - count, "phy_en:\t\t %s\n",
					phy_en_status ? "enabled" : "disabled");
	if (!phy_en_status) {
		count += scnprintf(buf + count, size - count, "\n");
		return count;
	}

	phy_lanedt_en_status = cmb_phy_lanedt_en_status_get(mipi->id);
	switch (phy_lanedt_en_status) {
	case 0:
		count += scnprintf(buf + count, size - count, "data_lane_en:\t disabled\n");
		break;
	case DT_1LANE:
		count += scnprintf(buf + count, size - count,
					"data_lane_en:\t 0x%x (lane0 enabled)\n", phy_lanedt_en_status);
		break;
	case DT_2LANE:
		count += scnprintf(buf + count, size - count,
					"data_lane_en:\t 0x%x (lane0/1 enabled)\n", phy_lanedt_en_status);
		break;
	default:
		count += scnprintf(buf + count, size - count,
					"data_lane_en:\t 0x%x (invalid value)\n", phy_lanedt_en_status);
		break;
	}

	phy_laneck_en_status = cmb_phy_laneck_en_status_get(mipi->id);
	count += scnprintf(buf + count, size - count,
			"clk_lane_en:\t %s\n", phy_laneck_en_status ? "enabled" : "disabled");

	// 0x0018: deskew
	phy_deskew_laneck0 = cmb_phy_deskew1_cfg_get(mipi->id);
	count += scnprintf(buf + count, size - count, "deskew:\t\t 0x%x\n", phy_deskew_laneck0);

	// 0x0028: settle_time
	phy0_s2p_dly = cmb_phy0_s2p_dly_get(mipi->id);
	count += scnprintf(buf + count, size - count, "settle_time:\t 0x%x\n", phy0_s2p_dly);

	// 0x0034: PHY Frequency Counter Register (mipi_bps: Mbps)
	dphy_clk = clk_get_rate(vind->clk[VIN_TOP_CLK].clock) / 1000000;
	phy_freq_cnt = cmb_phy_freq_cnt_get(mipi->id);
	mipi_bps = 1000 * dphy_clk * 8 / phy_freq_cnt;
	count += scnprintf(buf + count, size - count, "mipi_bps:\t %u Mbps\n", mipi_bps);

	// 0x00f0: PHY Debug Register (PHY clock lane0 status, PHYC data lane0/1 status)
	phy_sta_d0 = cmb_phy_sta_d0_get(mipi->id);
	if (0x5 == phy_sta_d0)
		count += scnprintf(buf + count, size - count, "data_lane0:\t 0x%x (HS_S, ok)\n", phy_sta_d0);
	else if (0x3 == phy_sta_d0)
		count += scnprintf(buf + count, size - count, "data_lane0:\t 0x%x (LP11S, ok)\n", phy_sta_d0);
	else
		count += scnprintf(buf + count, size - count, "data_lane0:\t 0x%x (error)\n", phy_sta_d0);

	phy_sta_d1 = cmb_phy_sta_d1_get(mipi->id);
	if (0x5 == phy_sta_d1)
		count += scnprintf(buf + count, size - count, "data_lane1:\t 0x%x (HS_S, ok)\n", phy_sta_d1);
	else if (0x3 == phy_sta_d1)
		count += scnprintf(buf + count, size - count, "data_lane1:\t 0x%x (LP11S, ok)\n", phy_sta_d1);
	else
		count += scnprintf(buf + count, size - count, "data_lane1:\t 0x%x (error)\n", phy_sta_d1);

	phy_sta_ck0 = cmb_phy_sta_ck0_get(mipi->id);
	if ((mipi->id & 1) && cmb_port_lane_num_get(mipi->id - 1) == 4) {
		if (0x0 == phy_sta_ck0) {
			count += scnprintf(buf + count, size - count,
				"clk_lane:\t 0x%x (mipi%d used for 4_lane mode, clk_lane is the same with mipi%d)\n",
				phy_sta_ck0, mipi->id, mipi->id - 1);
		} else {
			count += scnprintf(buf + count, size - count, "clk_lane:\t 0x%x (error)\n", phy_sta_ck0);
		}
	} else {
		if (0x5 == phy_sta_ck0)
			count += scnprintf(buf + count, size - count, "clk_lane:\t 0x%x (HS_S, ok)\n", phy_sta_ck0);
		else if (0x3 == phy_sta_ck0)
			count += scnprintf(buf + count, size - count, "clk_lane:\t 0x%x (LP11S, ok)\n", phy_sta_ck0);
		else
			count += scnprintf(buf + count, size - count, "clk_lane:\t 0x%x (error)\n", phy_sta_ck0);
	}

	count += scnprintf(buf + count, size - count, "\n");
#endif

	return count;
}

static size_t port_payload_status_dump(struct mipi_dev *mipi, char *buf, size_t size)
{
	size_t count = 0;

#if defined MIPI_COMBO_CSI
	int ch_i = 0;
	unsigned int port_en_status;
	unsigned int port_lane_num;
	unsigned int port_channel_num;
	unsigned int port_out_num;
	unsigned int port_wdr_mode;
	unsigned int unpack_enable;
	unsigned int yuv_seq;
	unsigned int mipi_ch_field;
	unsigned int int_pending_status;
	unsigned int mipi_ch_cur_dt;
	char mipi_ch_cur_dt_desc[32] = "";
	unsigned int mipi_ch_cur_vc;

	count += scnprintf(buf + count, size - count, "[mipi%u]\n", mipi->id);

	// 0x0000: PORT Control Register (port_en, port_lane_num, port_channel_num, port_out_num)
	port_en_status = cmb_port_en_status_get(mipi->id);
	count += scnprintf(buf + count, size - count, "port_en:\t %s\n",
					port_en_status ? "enabled" : "disabled");
	if (!port_en_status) {
		count += scnprintf(buf + count, size - count, "\n");
		return count;
	}

	port_lane_num = cmb_port_lane_num_get(mipi->id);
	count += scnprintf(buf + count, size - count, "lane_num:\t %d lane\n", port_lane_num);

	port_channel_num = cmb_port_channel_num_get(mipi->id);
	count += scnprintf(buf + count, size - count, "channel_num:\t %d %s\n",
					port_channel_num + 1, port_channel_num ? "channels" : "channel");

	port_out_num = cmb_port_out_num_get(mipi->id);
	count += scnprintf(buf + count, size - count, "out_data_num:\t %s\n",
					port_out_num ? "2 data" : "1 data");

	// 0x000c: PORT WDR Mode Register
	port_wdr_mode = cmb_port_wdr_mode_get(mipi->id);
	switch (port_wdr_mode) {
	case 0:
		count += scnprintf(buf + count, size - count, "wdr_mode:\t normal\n");
		break;
	case 2:
		count += scnprintf(buf + count, size - count, "wdr_mode:\t WDR\n");
		break;
	default:
		count += scnprintf(buf + count, size - count, "wdr_mode:\t reserved\n");
		break;
	}

	// 0x0100: PORT MIPI Configuration Register (unpack_en, yuv_seq)
	unpack_enable = cmb_port_mipi_unpack_en_status_get(mipi->id);
	count += scnprintf(buf + count, size - count, "unpack_en:\t %s\n",
				unpack_enable ? "enabled" : "disabled");
	yuv_seq = cmb_port_mipi_yuv_seq_get(mipi->id);
	switch (yuv_seq) {
	case YUYV:
		count += scnprintf(buf + count, size - count, "yuv_seq:\t YUYV\n");
		break;
	case YVYU:
		count += scnprintf(buf + count, size - count, "yuv_seq:\t YVYU\n");
		break;
	case UYVY:
		count += scnprintf(buf + count, size - count, "yuv_seq:\t UYVY\n");
		break;
	case VYUY:
		count += scnprintf(buf + count, size - count, "yuv_seq:\t VYUY\n");
		break;
	default:
		break;
	}

	for (ch_i = 0; ch_i <= port_channel_num; ch_i++) {
		if (port_channel_num > 0)
			count += scnprintf(buf + count, size - count, "[mipi%u_channel%u]\n", mipi->id, ch_i);

		// 0x0110: PORT MIPI Channel0 Data Type Trigger Register (mipi_src_is_filed)
		mipi_ch_field = cmb_port_mipi_ch_field_get(mipi->id, ch_i);
		count += scnprintf(buf + count, size - count, "ch_field:\t %s\n",
					mipi_ch_field ? "interlaced" : "progressive");

		// 0x0114: PORT MIPI Channel0 interrupt Enable Register

		// 0x0118: PORT MIPI Channel0 interrupt Pending Register
		int_pending_status = cmb_port_int_status_get(mipi->id, ch_i, NULL);
		count += scnprintf(buf + count, size - count, "int_pending:\t 0x%x (%s)\n", int_pending_status,
					(0xf == int_pending_status || 0x4f == int_pending_status || 0x3f == int_pending_status)
					? "ok" : "error");

		// 0x011c: PORT MIPI Channel0 Packet Header Register (port_mipi_cur_dt, mipi_cur_vc)
		mipi_ch_cur_dt = cmb_port_mipi_ch_cur_dt_get(mipi->id, ch_i);
		switch (mipi_ch_cur_dt) {
		case MIPI_FS:
			strcpy(mipi_ch_cur_dt_desc, "Frame Start");
			break;
		case MIPI_FE:
			strcpy(mipi_ch_cur_dt_desc, "Frame End");
			break;
		case MIPI_LS:
			strcpy(mipi_ch_cur_dt_desc, "Line Start");
			break;
		case MIPI_LE:
			strcpy(mipi_ch_cur_dt_desc, "Line End");
			break;
		case MIPI_SDAT0:
		case MIPI_SDAT1:
		case MIPI_SDAT2:
		case MIPI_SDAT3:
		case MIPI_SDAT4:
		case MIPI_SDAT5:
		case MIPI_SDAT6:
		case MIPI_SDAT7:
			strcpy(mipi_ch_cur_dt_desc, "Special Data Type");
			break;

		case MIPI_BLK:
			strcpy(mipi_ch_cur_dt_desc, "Blanking");
			break;

		case MIPI_EMBD:
			strcpy(mipi_ch_cur_dt_desc, "Embedded");
			break;
		case MIPI_YUV420:
			strcpy(mipi_ch_cur_dt_desc, "YUV420 8-bit");
			break;
		case MIPI_YUV420_10:
			strcpy(mipi_ch_cur_dt_desc, "YUV420 10-bit");
			break;
		case MIPI_YUV420_CSP:
			strcpy(mipi_ch_cur_dt_desc, "YUV420 8-bit (CSP)");
			break;
		case MIPI_YUV420_CSP_10:
			strcpy(mipi_ch_cur_dt_desc, "YUV420 10-bit (CSP)");
			break;
		case MIPI_YUV422:
			strcpy(mipi_ch_cur_dt_desc, "YUV422 8-bit");
			break;
		case MIPI_YUV422_10:
			strcpy(mipi_ch_cur_dt_desc, "YUV422 10-bit");
			break;
		case MIPI_RGB565:
			strcpy(mipi_ch_cur_dt_desc, "RGB565");
			break;
		case MIPI_RGB888:
			strcpy(mipi_ch_cur_dt_desc, "RGB888");
			break;
		case MIPI_RAW8:
			strcpy(mipi_ch_cur_dt_desc, "RAW8");
			break;
		case MIPI_RAW10:
			strcpy(mipi_ch_cur_dt_desc, "RAW10");
			break;
		case MIPI_RAW12:
			strcpy(mipi_ch_cur_dt_desc, "RAW12");
			break;
		case MIPI_USR_DAT0:
		case MIPI_USR_DAT1:
		case MIPI_USR_DAT2:
		case MIPI_USR_DAT3:
		case MIPI_USR_DAT4:
		case MIPI_USR_DAT5:
		case MIPI_USR_DAT6:
		case MIPI_USR_DAT7:
			strcpy(mipi_ch_cur_dt_desc, "User Data Type");
			break;
		default:
			strcpy(mipi_ch_cur_dt_desc, "Unrecognized Data Type");
			break;
		}

		count += scnprintf(buf + count, size - count, "cur_data_type:\t 0x%x (%s)\n",
					mipi_ch_cur_dt, mipi_ch_cur_dt_desc);

		mipi_ch_cur_vc = cmb_port_mipi_ch_cur_vc_get(mipi->id, ch_i);
		count += scnprintf(buf + count, size - count, "vir_ch:\t\t %u\n", mipi_ch_cur_vc);
	}

	count += scnprintf(buf + count, size - count, "\n");
#endif

	return count;
}

static int mipi_debugfs_open(struct inode *inode, struct file *file)
{
	struct mipi_debugfs_buffer *buf;
	struct mipi_dev *mipi;
	int i = 0;

	buf = kmalloc(sizeof(*buf), GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	mipi_status_size_sum = 0;

	mipi_status_size_sum += scnprintf(buf->data + mipi_status_size_sum,
							sizeof(buf->data) - mipi_status_size_sum,
							"*************** MIPI Register Status List **************\n");

	/* PHY COMMON Register (PHY Top Control Register) */
	mipi_status_size_sum += scnprintf(buf->data + mipi_status_size_sum,
							sizeof(buf->data) - mipi_status_size_sum,
							"------------------ PHY COMMON Register -----------------\n");
	mipi_status_size_sum += phy_common_status_dump(buf->data + mipi_status_size_sum,
					sizeof(buf->data) - mipi_status_size_sum);

	/* PHY Digital Layer Register */
	mipi_status_size_sum += scnprintf(buf->data + mipi_status_size_sum,
							sizeof(buf->data) - mipi_status_size_sum,
							"-------------- PHY Digital Layer Register --------------\n");
	for (i = 0; i < VIN_MAX_MIPI; i++) {
		mipi = glb_mipi[i];
		if (mipi != NULL) {
			mipi_status_size[i] = phy_digital_status_dump(mipi, buf->data + mipi_status_size_sum,
						sizeof(buf->data) - mipi_status_size_sum);
			mipi_status_size_sum += mipi_status_size[i];
		}
	}

	/* PORT Payload Layer Register*/
	mipi_status_size_sum += scnprintf(buf->data + mipi_status_size_sum,
							sizeof(buf->data) - mipi_status_size_sum,
							"-------------- PORT Payload Layer Register -------------\n");
	for (i = 0; i < VIN_MAX_MIPI; i++) {
		mipi = glb_mipi[i];
		if (mipi != NULL) {
			mipi_status_size[i] = port_payload_status_dump(mipi, buf->data + mipi_status_size_sum,
						sizeof(buf->data) - mipi_status_size_sum);
			mipi_status_size_sum += mipi_status_size[i];
		}
	}

	mipi_status_size_sum += scnprintf(buf->data + mipi_status_size_sum,
							sizeof(buf->data) - mipi_status_size_sum,
							"********************************************************\n\n");

	buf->count = mipi_status_size_sum;
	file->private_data = buf;

	return 0;
}

static ssize_t mipi_debugfs_read(struct file *file, char __user *user_buf,
				      size_t nbytes, loff_t *ppos)
{
	struct mipi_debugfs_buffer *buf = file->private_data;

	return simple_read_from_buffer(user_buf, nbytes, ppos, buf->data,
				       buf->count);
}

static int mipi_debugfs_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	file->private_data = NULL;

	return 0;
}

static const struct file_operations mipi_debugfs_fops = {
	.owner = THIS_MODULE,
	.open = mipi_debugfs_open,
	.llseek = no_llseek,
	.read = mipi_debugfs_read,
	.release = mipi_debugfs_release,
};

#if IS_ENABLED(CONFIG_DEBUG_FS)
int sunxi_mipi_debug_register_driver(void)
{
#if IS_ENABLED(CONFIG_SUNXI_MPP)
	mipi_debugfs_root = debugfs_mpp_root;
#else
	mipi_debugfs_root = debugfs_lookup("mpp", NULL);
	if (NULL == mipi_debugfs_root)
		mipi_debugfs_root = debugfs_create_dir("mpp", NULL);
#endif
	if (NULL == mipi_debugfs_root) {
		vin_err("Unable to lookup or create mipi debugfs root dir.\n");
		return -ENOENT;
	}

	mipi_node = debugfs_create_file("mipi", 0444, mipi_debugfs_root,
				   NULL, &mipi_debugfs_fops);
	if (IS_ERR_OR_NULL(mipi_node)) {
		vin_err("Unable to create mipi debugfs status file.\n");
		mipi_debugfs_root = NULL;
		return -ENODEV;
	}

	return 0;
}
#else
int sunxi_mipi_debug_register_driver(void)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_DEBUG_FS)
void sunxi_mipi_debug_unregister_driver(void)
{
	if (mipi_debugfs_root == NULL)
		return;
#if IS_ENABLED(CONFIG_SUNXI_MPP)
	debugfs_remove_recursive(mipi_node);
#else
	debugfs_remove_recursive(mipi_debugfs_root);
	mipi_debugfs_root = NULL;
#endif
}
#else
void sunxi_mipi_debug_unregister_driver(void)
{
	return;
}
#endif

static const struct of_device_id sunxi_mipi_match[] = {
	{.compatible = "allwinner,sunxi-mipi",},
	{},
};

static struct platform_driver mipi_platform_driver = {
	.probe = mipi_probe,
	.remove = mipi_remove,
	.driver = {
		.name = MIPI_MODULE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = sunxi_mipi_match,
	}
};

void sunxi_combo_set_sync_code(struct v4l2_subdev *sd,
		struct combo_sync_code *sync)
{
	struct mipi_dev *combo = v4l2_get_subdevdata(sd);

	memset(&combo->sync_code, 0, sizeof(combo->sync_code));
	combo->sync_code = *sync;
}

void sunxi_combo_set_lane_map(struct v4l2_subdev *sd,
		struct combo_lane_map *map)
{
	struct mipi_dev *combo = v4l2_get_subdevdata(sd);

	memset(&combo->lvds_map, 0, sizeof(combo->lvds_map));
	combo->lvds_map = *map;
}

void sunxi_combo_wdr_config(struct v4l2_subdev *sd,
		struct combo_wdr_cfg *wdr)
{
	struct mipi_dev *combo = v4l2_get_subdevdata(sd);

	memset(&combo->wdr_cfg, 0, sizeof(combo->wdr_cfg));
	combo->wdr_cfg = *wdr;
}

struct v4l2_subdev *sunxi_mipi_get_subdev(int id)
{
	if (id < VIN_MAX_MIPI && glb_mipi[id])
		return &glb_mipi[id]->subdev;
	else
		return NULL;
}

int sunxi_mipi_platform_register(void)
{
	return platform_driver_register(&mipi_platform_driver);
}

void sunxi_mipi_platform_unregister(void)
{
	platform_driver_unregister(&mipi_platform_driver);
	vin_log(VIN_LOG_MIPI, "mipi_exit end\n");
}

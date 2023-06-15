
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

#ifndef _SUNXI_MIPI__H_
#define _SUNXI_MIPI__H_

#include "../platform/platform_cfg.h"
#include "combo_rx/combo_rx_reg.h"
#include "combo_csi/combo_csi_reg.h"

enum mipi_pad {
	MIPI_PAD_SINK,
	MIPI_PAD_SOURCE,
	MIPI_PAD_NUM,
};

struct combo_config {
	enum lvds_lane_num lvds_ln;
	enum mipi_lane_num mipi_ln;
	unsigned int lane_num;
	unsigned int total_rx_ch;
};

struct combo_format {
	u32 code;
	enum lvds_bit_width bit_width;
	enum cmb_mipi_yuv_seq yuv_seq;
};

struct mipi_dev {
	struct v4l2_subdev subdev;
	struct media_pad mipi_pads[MIPI_PAD_NUM];
	struct platform_device *pdev;
	struct mutex subdev_lock;
	struct mipi_para csi2_cfg;
	struct mipi_fmt_cfg csi2_fmt;
	struct combo_config cmb_cfg;
	struct combo_format *cmb_fmt;
	struct combo_sync_code sync_code;
	struct combo_lane_map lvds_map;
	struct combo_wdr_cfg wdr_cfg;
	struct combo_csi_cfg cmb_csi_cfg;
	struct v4l2_mbus_framefmt format;
	void __iomem *base;
	void __iomem *port_base;
	char if_name[20];
	unsigned char id;
	unsigned char if_type;
	unsigned char cmb_mode;
	unsigned char pyha_offset;
	unsigned char time_hs;
	unsigned char terminal_resistance;
	unsigned char sensor_flags; /*0 means choose phy0,1 means choose phy1*/
	unsigned int settle_time;
};

void sunxi_combo_set_sync_code(struct v4l2_subdev *sd,
		struct combo_sync_code *sync);
void sunxi_combo_set_lane_map(struct v4l2_subdev *sd,
		struct combo_lane_map *map);
void sunxi_combo_wdr_config(struct v4l2_subdev *sd,
		struct combo_wdr_cfg *wdr);
struct v4l2_subdev *sunxi_mipi_get_subdev(int id);
int sunxi_mipi_platform_register(void);
void sunxi_mipi_platform_unregister(void);

#endif /*_SUNXI_MIPI__H_*/

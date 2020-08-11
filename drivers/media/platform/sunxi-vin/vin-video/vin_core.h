/*
 * vin_core.h for video manage
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *	Yang Feng <yangfeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _VIN_CORE_H_
#define _VIN_CORE_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/pm_runtime.h>

#include <media/sunxi_camera_v2.h>
#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>

#include "../platform/platform_cfg.h"
#include "../modules/flash/flash.h"

#include "../modules/actuator/actuator.h"
#include "../vin-mipi/bsp_mipi_csi.h"
#include "../utility/vin_supply.h"
#include "vin_video.h"
#include "../vin.h"

#define MAX_FRAME_MEM   (150*1024*1024)
#define MIN_WIDTH       (32)
#define MIN_HEIGHT      (32)
#define MAX_WIDTH       (4800)
#define MAX_HEIGHT      (4800)
#define DUMP_CSI		(1 << 0)
#define DUMP_ISP		(1 << 1)

struct vin_status_info {
	unsigned int width;
	unsigned int height;
	unsigned int h_off;
	unsigned int v_off;
	unsigned int err_cnt;
	unsigned int buf_cnt;
	unsigned int buf_size;
	unsigned int buf_rest;
	unsigned int frame_cnt;
	unsigned int lost_cnt;
	unsigned int frame_internal;
	unsigned int max_internal;
	unsigned int min_internal;
};

struct vin_coor {
	unsigned int x1;
	unsigned int y1;
	unsigned int x2;
	unsigned int y2;
};

struct vin_ptn_cfg {
	__u32 ptn_en;
	__u32 ptn_w;
	__u32 ptn_h;
	__u32 ptn_mode;
	__u32 ptn_dw;
	struct vin_mm ptn_buf;
};

struct vin_core {
	struct platform_device *pdev;
	unsigned int is_empty;
	int id;
	void __iomem *base;
	/* various device info */
	struct vin_vid_cap vid_cap;
	/* about vin channel */
	unsigned int total_rx_ch;
	unsigned int cur_ch;
	/* about some global info */
	unsigned int rear_sensor;
	unsigned int front_sensor;
	unsigned int sensor_sel;
	unsigned int csi_sel;
	unsigned int mipi_sel;
	unsigned int isp_sel;
	unsigned int vipp_sel;
	unsigned int isp_tx_ch;
	unsigned int hflip;
	unsigned int vflip;
	unsigned int vflip_delay;
	unsigned int stream_idx;
	unsigned int vin_clk;
	unsigned int fps_ds;
	unsigned int large_image;/*2:get merge yuv, 1: get pattern raw (save in kernel), 0: normal*/
	struct isp_debug_mode isp_dbg;
	struct sensor_exp_gain exp_gain;
	struct v4l2_device *v4l2_dev;
	const struct vin_pipeline_ops *pipeline_ops;
	int support_raw;
	int irq;
	struct vin_status_info vin_status;
	struct timer_list timer_for_reset;
	struct vin_ptn_cfg ptn_cfg;
};

static inline struct sensor_instance *get_valid_sensor(struct vin_core *vinc)
{
	struct vin_md *vind = dev_get_drvdata(vinc->v4l2_dev->dev);
	int valid_idx = NO_VALID_SENSOR;

	valid_idx = vind->modules[vinc->sensor_sel].sensors.valid_idx;

	if (valid_idx == NO_VALID_SENSOR)
		return NULL;

	return &vind->modules[vinc->sensor_sel].sensors.inst[valid_idx];
}
int sunxi_vin_debug_register_driver(void);
void sunxi_vin_debug_unregister_driver(void);
int sunxi_vin_core_register_driver(void);
void sunxi_vin_core_unregister_driver(void);
struct vin_core *sunxi_vin_core_get_dev(int index);
void vin_get_fmt_mplane(struct vin_frame *frame, struct v4l2_format *f);
struct vin_fmt *vin_find_format(const u32 *pixelformat, const u32 *mbus_code,
				  unsigned int mask, int index, bool have_code);
#endif /*_VIN_CORE_H_*/



/*
 ******************************************************************************
 *
 * sunxi_scaler.h
 *
 * Hawkview ISP - sunxi_scaler.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Zhao Wei   	2014/12/11	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef _SUNXI_SCALER_H_
#define _SUNXI_SCALER_H_
#include <linux/videodev2.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-subdev.h>
#include "../vin-video/vin_core.h"

enum scaler_pad {
	SCALER_PAD_SINK,
	SCALER_PAD_SOURCE,
	SCALER_PAD_NUM,
};

struct scaler_yuv_size_addr_info {
	unsigned int isp_byte_size;
	unsigned int line_stride_y;
	unsigned int line_stride_c;
	unsigned int buf_height_y;
	unsigned int buf_height_cb;
	unsigned int buf_height_cr;

	unsigned int valid_height_y;
	unsigned int valid_height_cb;
	unsigned int valid_height_cr;
	struct isp_yuv_channel_addr yuv_addr;
};

struct scaler_ratio {
	u32 horz;
	u32 vert;
};

struct scaler_dev {
	int use_cnt;
	struct v4l2_subdev subdev;
	struct media_pad scaler_pads[SCALER_PAD_NUM];
	struct v4l2_event event;
	struct v4l2_mbus_framefmt formats[SCALER_PAD_NUM];
	struct platform_device *pdev;
	unsigned int id;
	spinlock_t slock;
	struct mutex subdev_lock;
	wait_queue_head_t wait;
	void __iomem *base;
	struct resource *ioarea;

	struct {
		struct v4l2_rect request;
		struct v4l2_rect active;
	} crop;
	struct scaler_ratio ratio;
	struct list_head scaler_list;
};

unsigned int sunxi_scaler_set_size(unsigned int *fmt,
				   struct isp_size_settings *size_settings);
void sunxi_scaler_set_output_addr(unsigned long buf_base_addr);
struct v4l2_subdev *sunxi_scaler_get_subdev(int id);
int sunxi_scaler_platform_register(void);
void sunxi_scaler_platform_unregister(void);

#endif /*_SUNXI_SCALER_H_*/


/*
 ******************************************************************************
 *
 * vin_video.h
 *
 * Hawkview ISP - vin_video.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Zhao Wei   	2015/12/01	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef _VIN_VIDEO_H_
#define _VIN_VIDEO_H_

#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-common.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/videobuf2-core.h>
#include "../platform/platform_cfg.h"

/* buffer for one video frame */
struct vin_buffer {
	struct vb2_buffer vb;
	struct list_head list;
};

#define VIN_SD_PAD_SINK		0
#define VIN_SD_PAD_SOURCE	1
#define VIN_SD_PADS_NUM		2

enum vin_subdev_ind {
	VIN_IND_SENSOR,
	VIN_IND_MIPI,
	VIN_IND_CSI,
	VIN_IND_ISP,
	VIN_IND_SCALER,
	VIN_IND_ACTUATOR,
	VIN_IND_FLASH,
	VIN_IND_STAT,
	VIN_IND_MAX,
};

struct vin_pipeline {
	struct media_pipeline pipe;
	struct v4l2_subdev *sd[VIN_IND_MAX];
	atomic_t frame_number;
};

#define to_vin_pipeline(e) (((e)->pipe == NULL) ? NULL : \
	container_of((e)->pipe, struct vin_pipeline, pipe))


enum vin_fmt_flags {
	VIN_FMT_YUV = (1 << 0),
	VIN_FMT_RAW = (1 << 1),
	VIN_FMT_RGB = (1 << 2),
	VIN_FMT_MAX,
	/* all possible flags raised */
	VIN_FMT_ALL = (((VIN_FMT_MAX - 1) << 1) - 1),
};


struct vin_fmt {
	enum v4l2_mbus_pixelcode mbus_code;
	char	*name;
	u32	fourcc;
	u16	memplanes;
	u16	colplanes;
	u8	depth[VIDEO_MAX_PLANES];
	u16	mdataplanes;
	u16	flags;
	enum v4l2_colorspace color;
	enum v4l2_field	field;
};

struct vin_addr {
	u32	y;
	u32	cb;
	u32	cr;
};

struct vin_dma_offset {
	int	y_h;
	int	y_v;
	int	cb_h;
	int	cb_v;
	int	cr_h;
	int	cr_v;
};

struct vin_frame {
	u32	f_width;
	u32	f_height;
	u32	o_width;
	u32	o_height;
	u32	offs_h;
	u32	offs_v;
	u32	width;
	u32	height;
	unsigned long		payload[VIDEO_MAX_PLANES];
	unsigned long		bytesperline[VIDEO_MAX_PLANES];
	struct vin_addr	paddr;
	struct vin_dma_offset	dma_offset;
	struct vin_fmt	*fmt;
};


struct vin_vid_cap {
	struct video_device vdev;
	struct vin_frame frame;
	struct v4l2_ctrl_handler ctrl_handler;
	unsigned int isp_sel;
	/* video capture */
	struct vb2_queue vb_vidq;
	struct mutex buf_lock;
	struct vb2_alloc_ctx *alloc_ctx;
	struct list_head vidq_active;
	unsigned int capture_mode;
	unsigned int width;
	unsigned int height;
	unsigned int buf_byte_size; /* including main and thumb buffer */
	/*working state */
	unsigned long generating;
	unsigned long opened;
	unsigned long registered;
	struct mutex opened_lock;
	struct mutex stream_lock;
	unsigned int first_flag; /* indicate the first time triggering irq */
	long unsigned int sec, usec;
	spinlock_t slock;
	struct vin_pipeline pipe;
	struct vin_core *vinc;
	struct v4l2_subdev subdev;
	struct media_pad vd_pad;
	struct media_pad sd_pads[VIN_SD_PADS_NUM];
	bool user_subdev_api;
	struct v4l2_ctrl *ae_win[4];	/* wb win cluster */
	struct v4l2_ctrl *af_win[4];	/* af win cluster */
};

#define pipe_to_vin_video(p) ((p == NULL) ? NULL : \
		container_of(p, struct vin_vid_cap, pipe))
int vin_is_generating(struct vin_vid_cap *cap);
void vin_start_generating(struct vin_vid_cap *cap);
void vin_stop_generating(struct vin_vid_cap *cap);
int vin_is_opened(struct vin_vid_cap *cap);
int vin_set_addr(struct vin_core *vinc, struct vb2_buffer *vb,
		      struct vin_frame *frame, struct vin_addr *paddr);

int vin_initialize_capture_subdev(struct vin_core *vinc);
void vin_cleanup_capture_subdev(struct vin_core *vinc);

#endif /*_VIN_VIDEO_H_*/

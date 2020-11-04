/*
 * Fast car reverse head file
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */


#ifndef __sunxi_tvd_h__
#define __sunxi_tvd_h__

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <media/videobuf2-core.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-ctrls.h>
#include <media/sunxi_camera.h>
#include <linux/media-bus-format.h>
#include <media/videobuf2-v4l2.h>

#ifdef CONFIG_VIDEO_SUNXI_TVD
#include "../sunxi-tvd/tvd.h"
#endif
/* tvd interface */
#define CVBS_INTERFACE   0
#define YPBPRI_INTERFACE 1
#define YPBPRP_INTERFACE 2

/* output resolution */
#define NTSC             0
#define PAL              1



struct vin_buffer {
	struct vb2_v4l2_buffer vb;
	struct list_head list;
	void *paddr;
	enum vb2_buffer_state	state;
};

#ifdef VIDEO_SUNXI_TVD_SPECIAL
extern int tvd_open_special(int tvd_fd);
extern int tvd_close_special(int tvd_fd);

extern int vidioc_s_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);
extern int vidioc_g_fmt_vid_cap_special(int tvd_fd, struct v4l2_format *f);

extern int dqbuffer_special(int tvd_fd, struct tvd_buffer **buf);
extern int qbuffer_special(int tvd_fd, struct tvd_buffer *buf);

extern int vidioc_streamon_special(int tvd_fd, enum v4l2_buf_type i);
extern int vidioc_streamoff_special(int tvd_fd, enum v4l2_buf_type i);

extern void tvd_register_buffer_done_callback(void *func);

#endif



#ifdef CONFIG_VIDEO_SUNXI_VIN_SPECIAL
extern int vin_open_special(int tvd_fd);
extern int vin_close_special(int tvd_fd);

extern int vin_s_input_special(int id, int i);
extern int vin_s_fmt_special(int tvd_fd, struct v4l2_format *f);
extern int vin_g_fmt_special(int tvd_fd, struct v4l2_format *f);

extern int vin_dqbuffer_special(int tvd_fd, struct vin_buffer **buf);
extern int vin_qbuffer_special(int tvd_fd, struct vin_buffer *buf);

extern int vin_streamon_special(int tvd_fd, enum v4l2_buf_type i);
extern int vin_streamoff_special(int tvd_fd, enum v4l2_buf_type i);
extern  struct device *vin_get_dev(int id);
extern void vin_register_buffer_done_callback(int tvd_fd, void *func);
#endif



#endif

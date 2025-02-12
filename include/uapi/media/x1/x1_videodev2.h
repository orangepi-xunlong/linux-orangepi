/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * x1_videodev2.h - Driver uapi for KY X1 videvdev2
 *
 * Copyright (C) 2019 KY Micro Limited
 */

#ifndef _UAPI_LINUX_X1_VIDEODEV2_H_
#define _UAPI_LINUX_X1_VIDEODEV2_H_
#include <linux/videodev2.h>

/* afbc yuv */
#define V4L2_PIX_FMT_NV12_AFBC	v4l2_fourcc('A', 'F', '1', '2')
/* 10bit yuv */
#define V4L2_PIX_FMT_Y210	v4l2_fourcc('Y', '2', '1', '0')
#define V4L2_PIX_FMT_P210	v4l2_fourcc('P', '2', '1', '0')
#define V4L2_PIX_FMT_P010	v4l2_fourcc('P', '0', '1', '0')
#define V4L2_PIX_FMT_D010_1	v4l2_fourcc('D', '0', '1', '1')
#define V4L2_PIX_FMT_D010_2	v4l2_fourcc('D', '0', '1', '2')
#define V4L2_PIX_FMT_D010_3	v4l2_fourcc('D', '0', '1', '3')
#define V4L2_PIX_FMT_D010_4	v4l2_fourcc('D', '0', '1', '4')
#define V4L2_PIX_FMT_D210_1	v4l2_fourcc('D', '2', '1', '1')
#define V4L2_PIX_FMT_D210_2	v4l2_fourcc('D', '2', '1', '2')
#define V4L2_PIX_FMT_D210_3	v4l2_fourcc('D', '2', '1', '3')
#define V4L2_PIX_FMT_D210_4	v4l2_fourcc('D', '2', '1', '4')
/* Bayer raw ky packed */
#define V4L2_PIX_FMT_KYGB8P	v4l2_fourcc('p', 'R', 'W', '8')
#define V4L2_PIX_FMT_KYGB10P	v4l2_fourcc('p', 'R', 'W', 'A')
#define V4L2_PIX_FMT_KYGB12P	v4l2_fourcc('p', 'R', 'W', 'C')
#define V4L2_PIX_FMT_KYGB14P	v4l2_fourcc('p', 'R', 'W', 'E')

#define V4L2_BUF_FLAG_IGNOR			(1 << 31)
#define V4L2_BUF_FLAG_ERROR_HW		(1 << 30)
#define V4L2_BUF_FLAG_IDI_OVERRUN	(1 << 29)
#define V4L2_BUF_FLAG_SLICES_DONE	(1 << 28)
#define V4L2_BUF_FLAG_CLOSE_DOWN	(1 << 27)
#define V4l2_BUF_FLAG_FORCE_SHADOW	(1 << 26)
#define V4L2_BUF_FLAG_ERROR_SW		(0)

#define V4L2_VI_PORT_USAGE_SNAPSHOT			(1 << 0)

struct v4l2_vi_port_cfg {
	unsigned int port_entity_id;
	unsigned int offset;
	unsigned int depth;
	unsigned int weight;
	unsigned int div_mode;
	unsigned int usage;
};

#define KY_VI_ENTITY_NAME_LEN	(32)

struct v4l2_vi_entity_info {
	unsigned int id;
	char name[KY_VI_ENTITY_NAME_LEN];
};

struct v4l2_vi_dbg_reg {
	unsigned int addr;
	unsigned int value;
	unsigned int mask;
};

struct v4l2_vi_input_interface {
	unsigned int type;
	unsigned int ccic_idx;
	unsigned int ccic_trigger_line;
};

struct v4l2_vi_selection {
	unsigned int pad;
	struct v4l2_selection v4l2_sel;
};

enum {
	VI_INPUT_INTERFACE_OFFLINE = 0,
	VI_INPUT_INTERFACE_OFFLINE_SLICE,
	VI_INPUT_INTERFACE_MIPI,
};

enum {
	VI_PIPE_RESET_STAGE1 = 0,
	VI_PIPE_RESET_STAGE2,
	VI_PIPE_RESET_STAGE3,
	VI_PIPE_RESET_STAGE_CNT,
};

struct v4l2_vi_bandwidth_info {
	int rsum;
	int wsum;
};

struct v4l2_vi_slice_info {
	unsigned int timeout;
	int slice_id;
	int total_slice_cnt;
};

struct v4l2_vi_debug_dump {
	int reason;
};

#define BASE_VIDIOC_VI			(BASE_VIDIOC_PRIVATE + 20)
#define VIDIOC_GET_PIPELINE		_IO('V', BASE_VIDIOC_VI + 1)
#define VIDIOC_PUT_PIPELINE		_IOW('V', BASE_VIDIOC_VI + 2, int)
#define VIDIOC_APPLY_PIPELINE	_IO('V', BASE_VIDIOC_VI + 3)
#define VIDIOC_START_PIPELINE	_IO('V', BASE_VIDIOC_VI + 4)
#define VIDIOC_STOP_PIPELINE	_IO('V', BASE_VIDIOC_VI + 5)
#define VIDIOC_S_PORT_CFG		_IOW('V', BASE_VIDIOC_VI + 6, struct v4l2_vi_port_cfg)
#define VIDIOC_DBG_REG_WRITE	_IOWR('V', BASE_VIDIOC_VI + 7, struct v4l2_vi_dbg_reg)
#define VIDIOC_DBG_REG_READ		_IOWR('V', BASE_VIDIOC_VI + 8, struct v4l2_vi_dbg_reg)
#define VIDIOC_CFG_INPUT_INTF	_IOW('V', BASE_VIDIOC_VI + 9, struct v4l2_vi_input_interface)
#define VIDIOC_RESET_PIPELINE	_IOW('V', BASE_VIDIOC_VI + 10, int)
#define VIDIOC_G_PIPE_STATUS	_IOR('V', BASE_VIDIOC_VI + 11, unsigned int)
#define VIDIOC_SET_SELECTION	_IOW('V', BASE_VIDIOC_VI + 12, struct v4l2_vi_selection)
#define VIDIOC_G_SLICE_MODE		_IOR('V', BASE_VIDIOC_VI + 13, int)
#define VIDIOC_QUERY_SLICE_READY	_IOWR('V', BASE_VIDIOC_VI + 14, struct v4l2_vi_slice_info)
#define VIDIOC_S_SLICE_DONE		_IOW('V', BASE_VIDIOC_VI + 15, int)
#define VIDIOC_GLOBAL_RESET		_IO('V', BASE_VIDIOC_VI + 16)
#define VIDIOC_FLUSH_BUFFERS	_IO('V', BASE_VIDIOC_VI + 17)
#define VIDIOC_CPU_Z1			_IOR('V', BASE_VIDIOC_VI + 18, int)
#define VIDIOC_DEBUG_DUMP		_IOWR('V', BASE_VIDIOC_VI + 19, struct v4l2_vi_debug_dump)
#define VIDIOC_S_BANDWIDTH		_IOW('V', BASE_VIDIOC_VI + 39, struct v4l2_vi_bandwidth_info)
#define VIDIOC_G_ENTITY_INFO	_IOWR('V', BASE_VIDIOC_VI + 40, struct v4l2_vi_entity_info)
#endif

/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * x1_cpp_uapi.h - Driver uapi for KY X1 Camera Post Process
 *
 * Copyright (C) 2022 KY Micro Limited
 */

#ifndef __X1_CPP_UAPI_H__
#define __X1_CPP_UAPI_H__

#include <linux/videodev2.h>

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

/*
 * hw version info:
 * 31:16  Version bits
 * 15:0   Revision bits
 */
#define CPP_HW_VERSION_1_0 (0x00010000)
#define CPP_HW_VERSION_1_1 (0x00010001)
#define CPP_HW_VERSION_2_0 (0x00020000)
#define CPP_HW_VERSION_2_1 (0x00020001)

typedef struct reg_val_mask_info {
	uint32_t reg_offset;
	uint32_t val;
	uint32_t mask;
} hw_reg_t;

enum cpp_reg_cfg_type {
	CPP_WRITE32,
	CPP_READ32,
	CPP_WRITE32_RLX,
	CPP_WRITE32_NOP,
};

enum cpp_pix_format {
	PIXFMT_NV12_DWT = 0,
	PIXFMT_FBC_DWT,
};

#define CPP_MAX_PLANAR (2)
#define CPP_MAX_LAYERS (5)

typedef struct cpp_reg_cfg_cmd {
	enum cpp_reg_cfg_type reg_type;
	uint32_t reg_len;
	struct reg_val_mask_info *reg_data;
} cpp_reg_cfg_cmd_t;

typedef struct cpp_plane_info {
	uint32_t		type;
	uint32_t		bytesused;
	uint32_t		length;
	union {
		uint64_t	userptr;
		int32_t		fd;
		uint64_t    phyAddr;
	} m;
	uint32_t		data_offset;
} cpp_plane_info_t;

typedef struct cpp_buffer_info {
	unsigned int index;
	unsigned int num_layers;
	unsigned int kgain_used;
	enum cpp_pix_format format;
	struct cpp_plane_info dwt_planes[CPP_MAX_LAYERS][CPP_MAX_PLANAR];
	struct cpp_plane_info kgain_planes[CPP_MAX_LAYERS];
} cpp_buffer_info_t;

#define MAX_REG_CMDS (3)
#define MAX_REG_DATA (640)
typedef struct cpp_frame_info {
	uint32_t frame_id;
	uint32_t client_id;
	struct cpp_reg_cfg_cmd regs[MAX_REG_CMDS];
	struct cpp_buffer_info src_buf_info;
	struct cpp_buffer_info dst_buf_info;
	struct cpp_buffer_info pre_buf_info;
} cpp_frame_info_t;

struct x1_cpp_reg_cfg {
	enum cpp_reg_cfg_type cmd_type;
	union {
		struct reg_val_mask_info rw_info;
	} u;
};

struct cpp_hw_info {
	uint32_t cpp_hw_version;
	uint32_t low_pwr_mode;
};

struct cpp_bandwidth_info {
	int32_t rsum;
	int32_t wsum;
};

struct cpp_clock_info {
	uint32_t func_rate;
};

#define VIDIOC_X1_CPP_REG_CFG \
	_IOWR('V', BASE_VIDIOC_PRIVATE, struct x1_cpp_reg_cfg)

#define VIDIOC_X1_CPP_PROCESS_FRAME \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 1, struct cpp_frame_info)

#define VIDIOC_X1_CPP_HW_RST \
	_IO('V', BASE_VIDIOC_PRIVATE + 2)

#define VIDIOC_X1_CPP_LOW_PWR \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 3, int)

#define VIDIOC_X1_CPP_FLUSH_QUEUE \
	_IO('V', BASE_VIDIOC_PRIVATE + 4)

#define VIDIOC_X1_CPP_HW_INFO \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 5, struct cpp_hw_info)

#define VIDIOC_X1_CPP_IOMMU_ATTACH \
	_IO('V', BASE_VIDIOC_PRIVATE + 6)

#define VIDIOC_X1_CPP_IOMMU_DETACH \
	_IO('V', BASE_VIDIOC_PRIVATE + 7)

#define VIDIOC_X1_CPP_UPDATE_BANDWIDTH \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 8, struct cpp_bandwidth_info)

#define VIDIOC_X1_CPP_UPDATE_CLOCKRATE \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 9, struct cpp_clock_info)

struct x1_cpp_done_info {
	uint32_t frame_id;
	uint32_t client_id;
	uint64_t seg_reg_cfg;
	uint64_t seg_stream;
	uint8_t success;
};

struct x1_cpp_error_info {
	uint32_t frame_id;
	uint32_t client_id;
	uint32_t err_type;
};

struct x1_cpp_event_data {
	union {
		struct x1_cpp_done_info done_info;
		struct x1_cpp_error_info err_info;
	} u; /* union can have max 64 bytes */
};

#define V4L2_EVENT_CPP_FRAME_DONE (V4L2_EVENT_PRIVATE_START + 0)
#define V4L2_EVENT_CPP_FRAME_ERR (V4L2_EVENT_PRIVATE_START + 1)

#endif /* __X1_CPP_UAPI_H__ */

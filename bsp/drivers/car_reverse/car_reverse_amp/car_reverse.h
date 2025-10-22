/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Fast car reverse image preview module
 *
 * Copyright (C) 2015-2023 AllwinnerTech, Inc.
 *
 * Authors:  Huangyongxing <huangyongxing@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __car_reverse_h__
#define __car_reverse_h__

#include <sunxi-log.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#define CAR_REVERSE_PRINT(fmt, ...)			sunxi_info(NULL, "[%s]-<%d>:" fmt, __func__, __LINE__, ##__VA_ARGS__);
#define CAR_REVERSE_ERR(fmt, ...)			sunxi_err(NULL, "[CAR_ERR]: "fmt, ##__VA_ARGS__)
#define CAR_REVERSE_WRN(fmt, ...)			sunxi_warn(NULL, "[CAR_WRN]: "fmt, ##__VA_ARGS__)
#define CAR_REVERSE_INFO(fmt, ...)			sunxi_info(NULL, "[CAR_INFO]: "fmt,  ##__VA_ARGS__)
#define CAR_REVERSE_DEV_ERR(dev, fmt, ...)	sunxi_err(dev, "[CAR_ERR]: "fmt, ##__VA_ARGS__)
#define CAR_REVERSE_DEV_INFO(dev, fmt, ...)	sunxi_info(dev, "[CAR_INFO]: "fmt,  ##__VA_ARGS__)

#define CAR_DRIVER_DBG(fmt, ...) \
			do { \
				if (reverse_loglevel_debug & 0x1) \
					sunxi_info(NULL, "[CAR_DRVIVER]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[CAR_DRVIVER]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
			} while (0)

#define BUFFER_POOL_DBG(fmt, ...) \
			do { \
				if (reverse_loglevel_debug & 0x2) \
					sunxi_info(NULL, "[CAR_BUFFER_POOL]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[CAR_BUFFER_POOL]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
			} while (0)

#define VIDEO_SOURCE_DBG(fmt, ...) \
			do { \
				if (reverse_loglevel_debug & 0x4) \
					sunxi_info(NULL, "[CAR_VIDEO_SOURCE]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[CAR_VIDEO_SOURCE]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
			} while (0)

#define PREVIEW_DISPLAY_DBG(fmt, ...) \
			do { \
				if (reverse_loglevel_debug & 0x8) \
					sunxi_info(NULL, "[CAR_PREVIEW_DISPLAY]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[CAR_PREVIEW_DISPLAY]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
			} while (0)

#define PREVIEW_ENHANCER_DBG(fmt, ...) \
			do { \
				if (reverse_loglevel_debug & 0x10) \
					sunxi_info(NULL, "[CAR_PREVIEW_ENHANCER]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[CAR_PREVIEW_ENHANCER]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
			} while (0)

#define PREVIEW_ROTATOR_DBG(fmt, ...) \
			do { \
				if (reverse_loglevel_debug & 0x10) \
					sunxi_info(NULL, "[CAR_PREVIEW_ROTATOR]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[CAR_PREVIEW_ROTATOR]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
			} while (0)

#define PREVIEW_AUXLINE_DBG(fmt, ...) \
			do { \
				if (reverse_loglevel_debug & 0x20) \
					sunxi_info(NULL, "[CAR_AUX_LINE]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
				else \
					sunxi_debug(NULL, "[CAR_AUX_LINE]-<%s>: "fmt, __func__, ##__VA_ARGS__); \
			} while (0)

enum car_reverse_ctrl {
	CTRL_NULL  = 0,
	ARM_CTRL = 1,
	RV_CTRL  = 2,
};

enum car_reverse_status {
	CAR_REVERSE_NULL  = 0,
	CAR_REVERSE_START = 1,
	CAR_REVERSE_STOP  = 2,
	CAR_REVERSE_HOLD  = 3,
	CAR_REVERSE_BUSY  = 4,
	ARM_TRY_GET_CRTL  = 5,
	ARM_GET_CRTL_OK  = 6,
	ARM_GET_CRTL_FAIL  = 7,
	CAR_REVERSE_STATUS_MAX,
};

enum car_reverse_amp_packet_type {
	TPYE_NULL = 0,
	SYNC_CAR_REVERSE_STATUS = 1,
	ARM_TRY_GET_CAR_REVERSE_CRTL = 2,
};

struct car_reverse_amp_packet {
	int magic;
	int type;
	int car_status;
	int arm_car_reverse_status;
	int rv_car_status;
};

enum car_reverse_ioctl_cmd {
	CMD_NONE = 0,
	CMD_CAR_REVERSE_GET_CTRL_TO_AMP = 0x1,
	CMD_CAR_REVERSE_CHECK_STATE = 0x3,
	CMD_CAR_REVERSE_TEST = 0x5,
	CMD_NUM,
};

#endif

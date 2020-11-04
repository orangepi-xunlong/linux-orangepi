/*
 * Fast car reverse image preview module
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

#ifndef __car_reverse_h__
#define __car_reverse_h__

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>
#include "sunxi_tvd.h"
#define CAR_MAX_CH 4

#undef		_VIRTUAL_DEBUG_
#undef		_REVERSE_DEBUG_

#define __DEBUG__

#ifdef __DEBUG__
#define logdebug(fmt, args...)	pr_info("car_reverse: "fmt, ##args)
#define loginfo(fmt, args...)	pr_info("car_reverse: "fmt, ##args)
#define logerror(fmt, args...)	pr_err("car_reverse: "fmt, ##args)
#else
#define logdebug(fmt, args...)
#define loginfo(fmt, args...)	pr_info("car_reverse: "fmt, ##args)
#define logerror(fmt, args...)	pr_err("car_reverse: "fmt, ##args)
#endif

enum car_reverse_status {
	CAR_REVERSE_HOLD  = 0,
	CAR_REVERSE_START = 1,
	CAR_REVERSE_STOP  = 2,
};




struct preview_params {
	struct device *dev;
	struct platform_device *pdev;
	int tvd_id;
	int src_width;
	int src_height;
	int screen_width;
	int screen_height;
	/*
	 * tv decode input interface
	 *  - 0: CVBS_INTERFACE
	 *  - 1: YPBPRP_INTERFACE
	 *  - 2: YPBPRP_INTERFACE
	 */
	int interface;
	/*
	 * resolution quality
	 *  - 0: NTSC
	 *  - 1: PAL
	 */
	int system;
	/*
	 * yuv format
	 *  - 0: TVD_PL_YUV420
	 *  - 1: TVD_MB_YUV420
	 *  - 2: TVD_PL_YUV422
	 */
	int format;
	int rotation;
	int car_direct;
	int lr_direct;
	int pr_mirror;
	int input_src;
	int car_oview_mode;
	int viewthread;
	int locked;  /*1 locked, 0 unlocked*/
};

struct buffer_node {
	struct list_head list;

	int size;
	void *phy_address;
	void *vir_address;
    struct ion_handle *handle;
	union{
		#ifdef CONFIG_VIDEO_SUNXI_TVD
		struct tvd_buffer handler;
		#endif
		struct vin_buffer handler_vin;
	};
};

struct buffer_pool {

	spinlock_t lock;
	struct list_head head;
	int depth;
	struct buffer_node **pool;

	struct buffer_node *(*dequeue_buffer)(struct buffer_pool *bp);
	void (*queue_buffer)(struct buffer_pool *bp, struct buffer_node *node);
};

struct buffer_ops {
	struct buffer_node *(*dequeue_buffer)(void);
	void (*queue_buffer)(struct buffer_node *node);
	void (*release_buffer)(struct buffer_node *node);
};

#endif

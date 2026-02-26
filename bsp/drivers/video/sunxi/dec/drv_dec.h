/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * drv_dec.h
 *
 * Copyright (c) 2007-2020 Allwinnertech Co., Ltd.
 * Author: zhengxiaobin <zhengxiaobin@allwinnertech.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef _DRV_DEC_H
#define _DRV_DEC_H
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/compat.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/dma-fence.h>
#include <linux/poll.h>
#include <linux/pm_runtime.h>
#include <video/decoder_display.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dec_drv_info {
	_Bool inited;
	struct cdev *p_cdev;
	dev_t dev_id;
	struct class *p_class;
	struct device *p_device;
	struct platform_driver *p_dec_driver;
	struct file_operations *p_dec_fops;
	atomic_t driver_ref_count;
	struct platform_device *p_plat_dev;
	struct dec_device *p_dec_device;
};


#ifdef __cplusplus
}
#endif

#endif /*End of file*/

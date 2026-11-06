/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2021. All rights reserved.
 *
 * File Name: cvi_ive_interface.h
 * Description: ive driver interface header file
 */

#ifndef __CVI_IVE_INTERFACE_H__
#define __CVI_IVE_INTERFACE_H__

#include <linux/cdev.h>
#include <linux/completion.h>
#include <linux/version.h>
#include <linux/i2c.h>

#ifdef DEBUG
#define CVI_DBG_INFO(fmt, ...) pr_info(fmt, ##__VA_ARGS__)
#else
#define CVI_DBG_INFO(fmt, ...)
#endif

struct cvi_gy_device {
	int						major;
	int						minor;
	dev_t					devid;
	spinlock_t				lock;
	struct cdev				cdev;
	struct class			*class;
	struct device			*device;
	struct proc_dir_entry	*proc_dir;
	struct i2c_client		*client;
};

#endif /* __CVI_IVE_INTERFACE_H__ */

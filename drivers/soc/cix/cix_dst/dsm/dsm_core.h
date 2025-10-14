/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * dsm_core.h
 *
 * Huawei device state monitor head file
 *
 * Copyright (c) 2015-2019 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef _DSM_CORE_H
#define _DSM_CORE_H

#ifdef __KERNEL__
#include <linux/soc/cix/dsm_pub.h>
#include <linux/err.h>
#endif
#include <linux/ioctl.h>
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
#include <linux/kfifo.h>
#endif
#ifdef __KERNEL__
#include <linux/mutex.h>
#endif
#include "../dst_print.h"

#ifdef __KERNEL__
#define dsm_log_info(format, arg...) DST_PN("[dsm]: " format, ##arg)
#define dsm_log_err(format, arg...) DST_ERR("[dsm]: " format, ##arg)
#define dsm_log_debug(format, arg...) DST_DBG("[dsm]: " format, ##arg)
#endif

#define CLIENT_SIZE 128
#define UINT_BUF_MAX 12

#define CBUFF_OCCUPY_BIT 0
#define CBUFF_READY_BIT 1

#define DSM_MAX_LOG_SIZE (1024 * 8)
#define DSM_MIN_LOG_SIZE 5
#define DSM_EXTERN_CLIENT_MAX_BUF_SIZE (1024 * 30) /* 30K */

#define DSM_IOCTL_GET_CLIENT_COUNT \
	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF0, CLIENT_NAME_LEN)
#define DSM_IOCTL_BIND _IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF1, CLIENT_NAME_LEN)
#define DSM_IOCTL_POLL_CLIENT_STATE \
	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF2, CLIENT_NAME_LEN)
#define DSM_IOCTL_FORCE_DUMP \
	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF3, CLIENT_NAME_LEN)
#define DSM_IOCTL_GET_CLIENT_ERROR \
	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF4, CLIENT_NAME_LEN)
#define DSM_IOCTL_GET_DEVICE_NAME \
	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF5, CLIENT_NAME_LEN)
#define DSM_IOCTL_GET_MODULE_NAME \
	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF6, CLIENT_NAME_LEN)
#define DSM_IOCTL_GET_IC_NAME \
	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF7, CLIENT_NAME_LEN)

enum {
	DSM_CLIENT_NOTIFY_BIT = 0,
	DSM_CLIENT_VAILD_BIT = 31,
	DSM_CLIENT_NOTIFY = 1 << DSM_CLIENT_NOTIFY_BIT,
	DSM_CLIENT_VAILD = 1 << DSM_CLIENT_VAILD_BIT,
};

enum {
	DSM_SERVER_UNINIT = 0,
	DSM_SERVER_INITED = 1,
};

#ifdef __KERNEL__
struct dsm_server {
	unsigned long client_flag[CLIENT_SIZE];
	struct dsm_client *client_list[CLIENT_SIZE];
	int client_count;
	int server_state;
	struct workqueue_struct *dsm_wq;
	struct mutex mtx_lock;
};
#endif

#endif

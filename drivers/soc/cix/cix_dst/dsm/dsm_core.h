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

#ifdef __KERNEL__
#define dsm_log_info(x...)	pr_info(x)
#define dsm_log_err(x...) pr_err(x)
#define dsm_log_debug(x...)	pr_debug(x)
#endif

#define CLIENT_SIZE			128
#define UINT_BUF_MAX			12

#define CBUFF_OCCUPY_BIT		0
#define CBUFF_READY_BIT			1

#define DSM_MAX_LOG_SIZE		(1024 * 8)
#define DSM_MIN_LOG_SIZE		5
#define DSM_EXTERN_CLIENT_MAX_BUF_SIZE	(1024 * 30) /* 30K */

#define DSM_IOCTL_GET_CLIENT_COUNT	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF0, CLIENT_NAME_LEN)
#define DSM_IOCTL_BIND			_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF1, CLIENT_NAME_LEN)
#define DSM_IOCTL_POLL_CLIENT_STATE	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF2, CLIENT_NAME_LEN)
#define DSM_IOCTL_FORCE_DUMP		_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF3, CLIENT_NAME_LEN)
#define DSM_IOCTL_GET_CLIENT_ERROR	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF4, CLIENT_NAME_LEN)
#define DSM_IOCTL_GET_DEVICE_NAME	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF5, CLIENT_NAME_LEN)
#define DSM_IOCTL_GET_MODULE_NAME	_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF6, CLIENT_NAME_LEN)
#define DSM_IOCTL_GET_IC_NAME		_IOC(_IOC_READ | _IOC_WRITE, 'x', 0xF7, CLIENT_NAME_LEN)

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
#define FMD_MAX_BSN_LEN 20
#define FMD_MAX_STATION_LEN 8
#define FMD_MAX_MSG_EVENT_NUM 4
#define FMD_MAX_DEVICE_NAME_LEN 32
#define FMD_MAX_TEST_NAME_LEN 32
#define FMD_MAX_VALUE_LEN 64
#define FMD_MAX_RESULT_LEN 8
#define FMD_MAX_TIME_LEN 20
#define FMD_MAX_FIRMWARE_LEN 32
#define FMD_MAX_DESCRIPTION_LEN 512
#define FMD_NO 924005999
#endif

#ifdef CONFIG_HUAWEI_SDCARD_VOLD
#define SDCARD_ERR_LEN			5
#endif

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

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
struct fmd_event {
	int error_code;                                 //  error code
	int item_id;                                    //  index of test item
	int cycle;                                      //  record the count in RT
	char result[FMD_MAX_RESULT_LEN];                //  test result, "pass"
	char station[FMD_MAX_STATION_LEN];              //  station, "rt"
	char bsn[FMD_MAX_BSN_LEN];                      //  bsn, "013GLX1566004289"
	char time[FMD_MAX_TIME_LEN];                    //  test time, "2017-03-06 16:02:01"
	char device_name[FMD_MAX_DEVICE_NAME_LEN];      //  device name, "camera4_back"
	char test_name[FMD_MAX_TEST_NAME_LEN];          //  test item name,  "RT_KERNEL_DATA_TEST"
	char value[FMD_MAX_VALUE_LEN];                  //  test return value
	char min_threshold[FMD_MAX_VALUE_LEN];          //  the min value
	char max_threshold[FMD_MAX_VALUE_LEN];          //  the max value
	char firmware[FMD_MAX_FIRMWARE_LEN];            //  the firmware version, "NXP3304_VER1.0",  "NA"
	char description[FMD_MAX_DESCRIPTION_LEN];      //  description, "not used"  "NA"
};

/* data structs */
struct fmd_msg {
	int version;                                     //  fmd version,  1
	int data_source;                                 //  where is data from?  FMD_DATA_FROM_HAL
	int num_events;                                  //  events counts
	struct fmd_event events[FMD_MAX_MSG_EVENT_NUM];  // events
};

/* keep size with power of 2 */
#define MSGQ_SIZE			(32 * 1024)
struct dsm_msgq {
	struct kfifo msg_fifo;
	spinlock_t fifo_lock;
	struct mutex read_lock;
};
#endif

#endif

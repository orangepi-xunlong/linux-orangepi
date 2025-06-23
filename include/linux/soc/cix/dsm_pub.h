/*
 * dsm_pub.h
 *
 * huawei device state monitor public head file
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

#ifndef _DSM_PUB_H
#define _DSM_PUB_H

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

/* max client name length */
#define CLIENT_NAME_LEN				32
/* max device name length */
#define DSM_MAX_DEVICE_NAME_LEN			32
/* max module name length */
#define DSM_MAX_MODULE_NAME_LEN			16
/* max ic name length */
#define DSM_MAX_IC_NAME_LEN			16
/* dsm error no define */
#define DSM_ERR_NO_ERROR			0
#define DSM_ERR_I2C_TIMEOUT			1

#define DSM_PM_ERROR_START					927000000

enum DSM_KEYS_TYPE {
	DSM_VOL_KEY = 0,
	DSM_VOL_UP_KEY,
	DSM_VOL_DOWN_KEY,
	DSM_POW_KEY,
	DSM_HALL_IRQ,
};

#define DSM_POWER_KEY_VAL			116
#define DSM_VOL_DOWN_KEY_VAL		114
#define DSM_SENSOR_BUF_MAX			2048
#define DSM_SENSOR_BUF_COM			2048

struct dsm_client_ops {
	int (*poll_state)(void);
	int (*dump_func)(int type, void *buff, int size);
};

struct dsm_dev {
	const char *name;
	const char *device_name;
	const char *ic_name;
	const char *module_name;
	struct dsm_client_ops *fops;
	size_t buff_size;
};

struct dsm_client {
	char client_name[CLIENT_NAME_LEN];
	char device_name[DSM_MAX_DEVICE_NAME_LEN];
	char ic_name[DSM_MAX_IC_NAME_LEN];
	char module_name[DSM_MAX_MODULE_NAME_LEN];
	int client_id;
	int error_no;
	unsigned long buff_flag;
	struct dsm_client_ops *cops;
	wait_queue_head_t waitq;
	size_t read_size;
	size_t used_size;
	size_t buff_size;
	u8 dump_buff[];
};

/* for userspace client, such as sensor service, please refer to it */

struct dsm_extern_client {
	char client_name[CLIENT_NAME_LEN];
	int buf_size;
};

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
#define MAX_BSN_LEN		20
#define MAX_STATION_LEN		8
#define MAX_DEVICE_NAME_LEN	32
#define MAX_TEST_NAME_LEN	32
#define MAX_VAL_LEN		64
#define MAX_RESULT_LEN		8
#define MAX_TIME_LEN		20
#define MAX_FIRMWARE_LEN	32
#define MAX_DESCRIPTION_LEN	512
#define MAX_MSG_EVENT_NUM	4
#define DATA_FROM_KERNEL	1
#define ITEM_ID_MIN		700000000
#define ITEM_ID_MAX		799999999
#define DA_MIN_ERROR_NO		924005000
#define DA_MAX_ERROR_NO		924005999

struct event {
	int error_code;				/* error code (errno) */
	int item_id;				/* index of test item */
	int cycle;				/* which round in aging test */
	char result[MAX_RESULT_LEN];		/* actual test result */
	char station[MAX_STATION_LEN];		/* name of station */
	char bsn[MAX_BSN_LEN];			/* serial number of board */
	char time[MAX_TIME_LEN];		/* test time */
	char device_name[MAX_DEVICE_NAME_LEN];	/* name of device */
	char test_name[MAX_TEST_NAME_LEN];	/* name of test item */
	char value[MAX_VAL_LEN];		/* measured value */
	char min_threshold[MAX_VAL_LEN];	/* min limit of measured value */
	char max_threshold[MAX_VAL_LEN];	/* max limit of measured value */
	char firmware[MAX_FIRMWARE_LEN];	/* firmware information */
	char description[MAX_DESCRIPTION_LEN];	/* brief description */
};

struct message {
	int version;				/* message version */
	int data_source;			/* where is data from? */
	int num_events;				/* event counts */
	struct event events[MAX_MSG_EVENT_NUM];	/* store event entity */
};
#endif

#ifdef CONFIG_PLAT_DSM
struct dsm_client *dsm_register_client(struct dsm_dev *dev);
void dsm_unregister_client(struct dsm_client *dsm_client, struct dsm_dev *dev);
struct dsm_client *dsm_find_client(char *dsm_name);
int dsm_client_ocuppy(struct dsm_client *client);
int dsm_client_unocuppy(struct dsm_client *client);
int dsm_client_record(struct dsm_client *client, const char *fmt, ...);
int dsm_client_copy(struct dsm_client *client, void *src, int sz);

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
int dsm_client_copy_ext(struct dsm_client *client, void *src, int sz);
#endif

void dsm_client_notify(struct dsm_client *client, int error_no);
extern void dsm_key_pressed(int type);
int dsm_update_client_vendor_info(struct dsm_dev *dev);

#else
static inline struct dsm_client *dsm_register_client(struct dsm_dev *dev)
{
	return NULL;
}

static inline struct dsm_client *dsm_find_client(char *dsm_name)
{
	return NULL;
}

static inline int dsm_client_ocuppy(struct dsm_client *client)
{
	return 1;
}

static inline int dsm_client_unocuppy(struct dsm_client *client)
{
	return 0;
}

static inline int dsm_client_record(struct dsm_client *client,
				    const char *fmt, ...)
{
	return 0;
}

static inline int dsm_client_copy(struct dsm_client *client, void *src, int sz)
{
	return 0;
}

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
static inline int dsm_client_copy_ext(struct dsm_client *client,
				      void *src, int sz)
{
	return 0;
}
#endif

static inline void dsm_client_notify(struct dsm_client *client, int error_no)
{

}

static inline int dsm_update_client_vendor_info(struct dsm_dev *dev)
{
	return 0;
}
#endif

#endif

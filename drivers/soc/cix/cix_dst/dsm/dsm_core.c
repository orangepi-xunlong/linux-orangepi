/*
 * dsm_core.c
 *
 * device state monitor driver
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

#include "dsm_core.h"

#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

#ifndef DSM_MINOR
#define DSM_MINOR		255
#endif
#define QUOTIENT_BIT		3
#define SURPLUS_BIT		7
#define MAP_NUMBER		32
#define ERROR_NO_MAX_LEN	9
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
static struct dsm_msgq g_dsm_msgq;
#endif

static struct dsm_server g_dsm_server;
static struct work_struct dsm_work;

extern int dsm_report(const struct dsm_client *client);
static void report_hievent(const struct dsm_client *client)
{
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	int ret;
	struct hiview_hievent *event = hiview_hievent_create(client->error_no);
	if (event == NULL) {
		dsm_log_err("create hievent failed: %d \n", client->error_no);
		return;
	}
	hiview_hievent_put_string(event, "CONTENT", client->dump_buff);
	hiview_hievent_put_string(event, "MODULE_NAME", client->module_name);
	hiview_hievent_put_string(event, "IC_NAME", client->ic_name);
	ret = hiview_hievent_report(event);
	if (ret < 0)
		dsm_log_err("report hievent failed: %d \n", client->error_no);
	hiview_hievent_destroy(event);
#else
	dsm_report(client);
#endif
}

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
static void report_fmd_msg(const struct fmd_msg *msg, int index)
{
	struct hiview_hievent *event = NULL;
	int ret;
	event = hiview_hievent_create(FMD_NO);
	if (event == NULL) {
		dsm_log_err("create hievent failed: %d \n", msg->events->error_code);
		return;
	}

	hiview_hievent_put_integral(event, "ERROR_CODE", msg->events[index].error_code);
	hiview_hievent_put_integral(event, "VERSION", msg->version);
	hiview_hievent_put_integral(event, "DATA_SOURCE", msg->data_source);
	hiview_hievent_put_integral(event, "NUMBER_EVENTS", msg->num_events);
	hiview_hievent_put_integral(event, "ITEM_ID", msg->events[index].item_id);
	hiview_hievent_put_integral(event, "CYCLE", msg->events[index].cycle);
	hiview_hievent_put_string(event, "RESULT", msg->events[index].result);
	hiview_hievent_put_string(event, "STATION", msg->events[index].station);
	hiview_hievent_put_string(event, "BSN", msg->events[index].bsn);
	hiview_hievent_put_string(event, "DEVICE_NAME", msg->events[index].device_name);
	hiview_hievent_put_string(event, "TEST_NAME", msg->events[index].test_name);
	hiview_hievent_put_string(event, "VALUE", msg->events[index].value);
	hiview_hievent_put_string(event, "MIN_THRESHOLD", msg->events[index].min_threshold);
	hiview_hievent_put_string(event, "MAX_THRESHOLD", msg->events[index].max_threshold);
	hiview_hievent_put_string(event, "FIRMWARE", msg->events[index].firmware);
	hiview_hievent_put_string(event, "DESCRIPTION", msg->events[index].description);

	ret = hiview_hievent_report(event);
	if (ret < 0)
		dsm_log_err("report hievent failed: %d \n", msg->events[index].error_code);
	hiview_hievent_destroy(event);
}
#endif

/*
 * This is the preferred style for multi.line
 * comments in the Linux kernel source code.
 * Please use it consistently.
 *
 * Description:This function is used to update the variables
 * 'ic_name' & 'module_name' of the struct dsm_client.
 */
int dsm_update_client_vendor_info(struct dsm_dev *dev)
{
	int client_num;
	int is_found;
	int retval = -1;

	if ((!dev) || (!dev->name)) {
		dsm_log_err("dev or dev->name is NULL\n");
		return retval;
	}

	mutex_lock(&g_dsm_server.mtx_lock);
	for (client_num = 0; client_num < g_dsm_server.client_count; client_num++) {
		struct dsm_client *tmp = g_dsm_server.client_list[client_num];

		is_found = strncmp(tmp->client_name, dev->name, CLIENT_NAME_LEN);
		if (is_found)
			continue;
		if (dev->ic_name &&
		    strncmp(tmp->ic_name, dev->ic_name, DSM_MAX_IC_NAME_LEN)) {
			strncpy(tmp->ic_name,
				dev->ic_name,
				DSM_MAX_IC_NAME_LEN - 1);
			tmp->ic_name[DSM_MAX_IC_NAME_LEN - 1] = '\0';
		}
		if (dev->module_name &&
		    strncmp(tmp->module_name, dev->module_name, DSM_MAX_MODULE_NAME_LEN)) {
			strncpy(tmp->module_name,
				dev->module_name,
				DSM_MAX_MODULE_NAME_LEN - 1);
			tmp->module_name[DSM_MAX_MODULE_NAME_LEN - 1] = '\0';
		}
		retval = 0;
		break;
	}
	mutex_unlock(&g_dsm_server.mtx_lock);

	return retval;
}
EXPORT_SYMBOL(dsm_update_client_vendor_info);

struct dsm_client *dsm_register_client(struct dsm_dev *dev)
{
	int i;
	int conflict = -1;
	struct dsm_client *ptr = NULL;

	if (g_dsm_server.server_state != DSM_SERVER_INITED) {
		dsm_log_err("dsm server uninited\n");
		return NULL;
	}

	if ((!dev) || (dev->buff_size > DSM_EXTERN_CLIENT_MAX_BUF_SIZE)) {
		dsm_log_err("dsm_dev is NULL or buffer size is too big\n");
		return NULL;
	}

	/*
	 * before add lock , ensure read and write operations
	 * of g_dsm_server are ordered
	 */
	smp_rmb();
	mutex_lock(&g_dsm_server.mtx_lock);
	if (g_dsm_server.client_count >= CLIENT_SIZE) {
		dsm_log_info("clients has full\n");
		goto out;
	}

	ptr = vzalloc(sizeof(*ptr) + dev->buff_size);
	if (!ptr) {
		dsm_log_err("clients malloc failed\n");
		goto out;
	}

	for (i = 0; i < CLIENT_SIZE; i++) {
		if (!test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i]))
			break;
		if (!dev->name) {
			dsm_log_err("Please specify the dsm device name\n");
			vfree(ptr);
			ptr = NULL;
			goto out;
		}
		conflict = strncmp(g_dsm_server.client_list[i]->client_name,
				   dev->name,
				   CLIENT_NAME_LEN);
		if (!conflict) {
			dsm_log_err("new client %s conflict with No.%d client %s\n",
				    dev->name, i,
				    g_dsm_server.client_list[i]->client_name);
			break;
		}
	}

	if ((i >= CLIENT_SIZE) || (!conflict)) {
		dsm_log_err("clients register failed, index %d, conflict %d\n",
			    i, conflict);
		vfree(ptr);
		ptr = NULL;
		goto out;
	}

	strncpy(ptr->client_name, dev->name, CLIENT_NAME_LEN - 1);
	ptr->client_name[CLIENT_NAME_LEN - 1] = '\0';

	if (dev->device_name) {
		strncpy(ptr->device_name,
			dev->device_name,
			DSM_MAX_DEVICE_NAME_LEN - 1);
		ptr->device_name[DSM_MAX_DEVICE_NAME_LEN - 1] = '\0';
	}

	if (dev->ic_name) {
		strncpy(ptr->ic_name,
			dev->ic_name,
			DSM_MAX_IC_NAME_LEN - 1);
		ptr->ic_name[DSM_MAX_IC_NAME_LEN - 1] = '\0';
	}

	if (dev->module_name) {
		strncpy(ptr->module_name,
			dev->module_name,
			DSM_MAX_MODULE_NAME_LEN - 1);
		ptr->module_name[DSM_MAX_MODULE_NAME_LEN - 1] = '\0';
	}

	ptr->client_id = i;
	ptr->cops = dev->fops;
	ptr->buff_size = dev->buff_size;
	init_waitqueue_head(&ptr->waitq);
	g_dsm_server.client_list[i] = ptr;
	set_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i]);
	g_dsm_server.client_count++;
	dsm_log_info("client %s register success\n", ptr->client_name);

	/*
	 * before release lock. ensure read and write operations
	 * of g_dsm_server are ordered
	 */
	smp_wmb();
out:
	mutex_unlock(&g_dsm_server.mtx_lock);
	return ptr;
}
EXPORT_SYMBOL(dsm_register_client);

/*
 * This is the preferred style for multi.line
 * comments in the Linux kernel source code.
 * Please use it consistently.
 *
 * Description:find out the same name of dev->name
 * in the server's client_list, and clear the bit,
 * free the dsm_client
 */
void dsm_unregister_client(struct dsm_client *dsm_client, struct dsm_dev *dev)
{
	int i;
	int conflict;

	if ((!dsm_client) || (!dev) || (!dev->name)) {
		pr_info("pointer is NULL, please check the parameters\n");
		return;
	}

	mutex_lock(&g_dsm_server.mtx_lock);
	for (i = 0; i < CLIENT_SIZE; i++) {
		/* find the client and free it */
		conflict = strncmp(g_dsm_server.client_list[i]->client_name,
				   dev->name,
				   CLIENT_NAME_LEN);
		if (!conflict) {
			__clear_bit(DSM_CLIENT_VAILD_BIT,
				    &g_dsm_server.client_flag[i]);
			g_dsm_server.client_list[i] = NULL;
			g_dsm_server.client_count--;
			vfree(dsm_client);
			dsm_client = NULL;
			break;
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
}
EXPORT_SYMBOL(dsm_unregister_client);

int dsm_client_ocuppy(struct dsm_client *client)
{
	int ret = -1;

	if (client) {
		/* ensure set client buff flag in order  */
		smp_rmb();
		ret = test_and_set_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
	}

	return ret;
}
EXPORT_SYMBOL(dsm_client_ocuppy);

int dsm_client_unocuppy(struct dsm_client *client)
{
	int ret = -1;

	if (client) {
		clear_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
		/* ensure clean client buff flag in order */
		smp_wmb();
		ret = 0;
	}

	return ret;
}
EXPORT_SYMBOL(dsm_client_unocuppy);

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
static bool dsm_is_errno_in_da_range(int error_no)
{
	return ((error_no >= DA_MIN_ERROR_NO) && (error_no <= DA_MAX_ERROR_NO));
}
#endif

void dsm_client_notify(struct dsm_client *client, int error_no)
{
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	if (dsm_is_errno_in_da_range(error_no))
		return;
#endif
	if (client) {
		client->error_no = error_no;
		set_bit(CBUFF_READY_BIT, &client->buff_flag);
		set_bit(DSM_CLIENT_NOTIFY_BIT,
			&g_dsm_server.client_flag[client->client_id]);
		/* ensure dsm work start notify in order */
		smp_wmb();
		queue_work(g_dsm_server.dsm_wq, &dsm_work);
	}
}
EXPORT_SYMBOL(dsm_client_notify);

int dsm_client_record(struct dsm_client *client, const char *fmt, ...)
{
	va_list ap;
	int size;
	size_t avail;

	if (!client) {
		dsm_log_err("%s no client to record\n", __func__);
		return 0;
	}

	if ((client->used_size >= DSM_EXTERN_CLIENT_MAX_BUF_SIZE) ||
	    (client->buff_size <= (client->used_size + 1))) {
		dsm_log_err("%s no buffer to record\n", __func__);
		return 0;
	}

	avail = client->buff_size - client->used_size - 1;
	va_start(ap, fmt);
	size = vsnprintf((char *)&client->dump_buff[client->used_size],
			 avail, fmt, ap);
	va_end(ap);

	if (size < 0) {
		dsm_log_err("%s:record buffer failed\n", __func__);
		return size;
	}
	client->used_size += size;
	if (client->used_size >= client->buff_size)
		client->used_size = client->buff_size - 1;

	return size;
}
EXPORT_SYMBOL(dsm_client_record);

int dsm_client_copy(struct dsm_client *client, void *src, int sz)
{
	int size;

	if (!client) {
		dsm_log_err("%s no client to record\n", __func__);
		return 0;
	}

	if ((client->used_size + sz) > client->buff_size) {
		dsm_log_err("%s no enough buffer to record\n", __func__);
		return 0;
	}
	size = sz;
	memcpy(&client->dump_buff[client->used_size], src, size);
	client->used_size += size;

	return size;
}
EXPORT_SYMBOL(dsm_client_copy);

static void dsm_client_set_idle(struct dsm_client *client)
{
	if (!client) {
		dsm_log_err("client is null\n");
		return;
	}
	client->used_size = 0;
	client->read_size = 0;
	client->error_no = 0;
	memset(client->dump_buff, 0, client->buff_size);
	clear_bit(CBUFF_READY_BIT, &client->buff_flag);
	clear_bit(CBUFF_OCCUPY_BIT, &client->buff_flag);
	clear_bit(DSM_CLIENT_NOTIFY_BIT,
		&g_dsm_server.client_flag[client->client_id]);
	/* ensure clean dsm cleint of value in order */
	smp_wmb();
}

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
static unsigned int dsm_kfifo_get_data_len(void)
{
	unsigned int fifo_data_len = 0;
	unsigned long flags = 0;

	spin_lock_irqsave(&g_dsm_msgq.fifo_lock, flags);
	fifo_data_len = kfifo_len(&g_dsm_msgq.msg_fifo);
	spin_unlock_irqrestore(&g_dsm_msgq.fifo_lock, flags);

	return fifo_data_len;
}

static void dsm_kfifo_reset(void)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&g_dsm_msgq.fifo_lock, flags);
	kfifo_reset(&g_dsm_msgq.msg_fifo);
	spin_unlock_irqrestore(&g_dsm_msgq.fifo_lock, flags);
}

int dsm_client_copy_ext(struct dsm_client *client, void *src, int sz)
{
	int fmd_msg_size = sizeof(struct fmd_msg);
	struct fmd_msg *fmsg = NULL;
	int msg_count = 0;
	int i = 0;
	int j = 0;
	if ((!client) || (!src) || (sz <= 0)) {
		dsm_log_err("%s invlaid params\n", __func__);
		dsm_client_set_idle(client);
		return 0;
	}
	fmsg = (struct fmd_msg *)kzalloc(fmd_msg_size, GFP_KERNEL);
	if (!fmsg) {
		dsm_log_err("fmsg malloc failed\n");
		dsm_client_set_idle(client);
		return -1;
	}
	msg_count = sz / fmd_msg_size;
	for (i = 0; i < msg_count; i++) {
		memcpy(fmsg, (src + i * fmd_msg_size), fmd_msg_size);
		if (fmsg->num_events > 0 && fmsg->num_events <= FMD_MAX_MSG_EVENT_NUM) {
			for (j = 0; j < fmsg->num_events; j++)
				report_fmd_msg(fmsg, j);
		}
	}
	kfree(fmsg);
	dsm_client_set_idle(client);
	return msg_count;
}
EXPORT_SYMBOL(dsm_client_copy_ext);
#endif

static int dsm_client_readable(struct dsm_client *client)
{
	int ret = 0;

	if (client) {
		/* ensure set client buff flag in order */
		smp_rmb();
		ret = test_bit(CBUFF_READY_BIT, &client->buff_flag);
	}

	return ret;
}

static int dsm_atoi(const char *p)
{
	int val = 0;
	int count = 0;

	if (!p)
		return -1;
	while (isdigit(*p)) {
		val = val * 10 + (*p++ - '0');
		count++;
		if (count > ERROR_NO_MAX_LEN) {
			dsm_log_err("error_no is illegal\n");
			return -1;
		}
	}

	return val;
}

static char *dsm_strtok(char *string_org, const char *demial)
{
	static unsigned char *last;
	unsigned char *str = NULL;
	const unsigned char *ctrl = (const unsigned char *)demial;
	unsigned char map[MAP_NUMBER];
	int count;

	if (!demial)
		return last;
	for (count = 0; count < MAP_NUMBER; count++)
		map[count] = 0;

	do
		map[*ctrl >> QUOTIENT_BIT] |= (1 << (*ctrl & SURPLUS_BIT));
	while (*ctrl++);

	if (string_org)
		str = (unsigned char *)string_org;
	else
		str = last;

	while ((map[*str >> QUOTIENT_BIT] & (1 << (*str & SURPLUS_BIT))) && (*str))
		str++;
	string_org = (char *)str;
	for (; *str; str++) {
		if (map[*str >> QUOTIENT_BIT] & (1 << (*str & SURPLUS_BIT))) {
			*str++ = '\0';
			break;
		}
	}
	last = str;

	if (string_org == (char *)str)
		return NULL;
	else
		return string_org;
}

static int copy_int_to_user(void __user *argp, int val)
{
	int ret;
	int size;
	char buff[UINT_BUF_MAX] = {0};

	size = snprintf(buff, UINT_BUF_MAX, "%d\n", val);
	ret = copy_to_user(argp, buff, size);
	dsm_log_debug("%s result %d\n", __func__, ret);

	return ret;
}

struct dsm_client *dsm_find_client(char *cname)
{
	int i;
	struct dsm_client *client = NULL;

	if ((!cname) || (!strlen(cname))) {
		dsm_log_err("cname is NULL\n");
		return NULL;
	}

	if (g_dsm_server.server_state != DSM_SERVER_INITED)
		return NULL;
	mutex_lock(&g_dsm_server.mtx_lock);
	/* ensure find dsm client in order */
	smp_rmb();
	for (i = 0; i < CLIENT_SIZE; i++) {
		if ((test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i])) &&
		    (!strncasecmp(g_dsm_server.client_list[i]->client_name, cname, CLIENT_NAME_LEN))) {
			client = g_dsm_server.client_list[i];
			break;
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
	dsm_log_debug("cname: %s find %s\n",
		      cname,
		      client ? "success" : "failed");

	return client;
}
EXPORT_SYMBOL(dsm_find_client);

static void dsm_work_func(struct work_struct *work)
{
	int i;
	struct dsm_client *client = NULL;
	dsm_log_debug("%s enter\n", __func__);
	mutex_lock(&g_dsm_server.mtx_lock);
	/* ensure wake notify client in order */
	smp_rmb();
	for (i = 0; i < CLIENT_SIZE; i++) {
		if (test_bit(DSM_CLIENT_VAILD_BIT, &g_dsm_server.client_flag[i])) {
			dsm_log_debug("No.%d client name %s flag 0x%lx\n", i,
				      g_dsm_server.client_list[i]->client_name,
				      g_dsm_server.client_flag[i]);
			if (!test_and_clear_bit(DSM_CLIENT_NOTIFY_BIT, &g_dsm_server.client_flag[i]))
				continue;
			client = g_dsm_server.client_list[i];
			report_hievent(client);
			dsm_log_info("%s finish notify\n", client->client_name);
			dsm_client_set_idle(client);
		}
	}
	mutex_unlock(&g_dsm_server.mtx_lock);
	dsm_log_debug("%s exit\n", __func__);
}

static void dsm_get_report_info(char *string_buff)
{
	char client_name[CLIENT_NAME_LEN] = {'\0'};
	int size;
	int error_no = 0;
	struct dsm_client *client = NULL;
	char *buff = string_buff;
	char *ptr = NULL;

	mutex_lock(&g_dsm_server.mtx_lock);
	/* get client name */
	ptr = dsm_strtok(buff, ",");
	if (ptr) {
		size = strlen(ptr);
		size = (size < CLIENT_NAME_LEN) ? size : (CLIENT_NAME_LEN - 1);
		memcpy(client_name, ptr, size);
	}

	/* get error no */
	ptr = dsm_strtok(NULL, ",");
	if (ptr)
		error_no = dsm_atoi(ptr);

	/* get notify content */
	ptr = dsm_strtok(NULL, NULL);
	dsm_log_info("client name - %s, error no - %d\n",
		     client_name,
		     error_no);
	if (ptr)
		dsm_log_info("content - %s\n", ptr);
	mutex_unlock(&g_dsm_server.mtx_lock);
	client = dsm_find_client(client_name);

	if (client && (!dsm_client_ocuppy(client))) {
		dsm_log_debug("dsm write find client - %s\n", client_name);
		if (ptr)
			dsm_client_copy(client, ptr, strlen(ptr));
		dsm_client_notify(client, error_no);
	} else {
		dsm_log_info("dsm notify can't find client - %s\n",
			     client_name);
	}
}

static ssize_t dsm_read(struct file *file, char __user *buf,
			size_t count, loff_t *ppos)
{
	struct dsm_client *client = file->private_data;
	size_t copy_size = 0;

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	size_t len;
	int ret;
	unsigned int copied = 0;
#endif
	dsm_log_debug("%s enter\n", __func__);
	if (!client) {
		dsm_log_err("client not bind\n");
		return 0;
	}
	if (dsm_client_readable(client) == 0)
		return 0;

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	if (dsm_is_errno_in_da_range(client->error_no)) {
		len = dsm_kfifo_get_data_len();
		dsm_log_debug("[msgQ_len] - %zu bytes, [count] - %zu bytes\n",
			      len, count);
		if (len > 0) {
			if (mutex_lock_interruptible(&g_dsm_msgq.read_lock))
				return -ERESTARTSYS;
			ret = kfifo_to_user(&g_dsm_msgq.msg_fifo,
					    buf,
					    min(len, count),
					    &copied);
			mutex_unlock(&g_dsm_msgq.read_lock);
			if (ret)
				dsm_log_err("copy msgQ to user failed, ret %d\n", ret);
			else
				copy_size = copied;
			if (copied == len) {
				dsm_log_debug("all msgQ data is ready for reset\n");
				dsm_kfifo_reset();
			}
		} else {
			dsm_log_err("ignore coping to user as msgQ is empty\n");
		}
		dsm_client_set_idle(client);
		dsm_log_info("total %zu bytes read from msgQ to user\n",
			     copy_size);
	} else {
#endif
		copy_size = min(count, (client->used_size - client->read_size));
		if (copy_to_user(buf, &client->dump_buff[client->read_size], copy_size))
			dsm_log_err("copy to user failed\n");
		client->read_size += copy_size;
		if (client->read_size >= client->used_size)
			dsm_client_set_idle(client);
		dsm_log_debug("%zu bytes read to user\n", copy_size);
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	}
#endif
	dsm_log_debug("%s exit\n", __func__);

	return copy_size;
}

static ssize_t dsm_write(struct file *file, const char __user *buf,
			 size_t count, loff_t *ppos)
{
	char *buff = NULL;

	dsm_log_debug("%s enter\n", __func__);

	if (!buf) {
		dsm_log_err("buf is null\n");
		goto out;
	}

	if ((count > DSM_MAX_LOG_SIZE) || (count < DSM_MIN_LOG_SIZE)) {
		dsm_log_err("count is %zu,out of range\n", count);
		goto out;
	}

	buff = kzalloc(count, GFP_KERNEL);
	if (!buff) {
		dsm_log_err("dsm write malloc failed\n");
		goto out;
	}

	if (copy_from_user(buff, buf, (count - 1))) {
		dsm_log_err("dsm write copy failed\n");
		goto out;
	}

	*(buff + count - 1) = '\0';
	dsm_get_report_info(buff);
out:
	kfree(buff);
	dsm_log_debug("%s exit\n", __func__);
	return count;
}

static unsigned int dsm_poll(struct file *file, poll_table *wait)
{
	struct dsm_client *client = file->private_data;
	unsigned int mask = 0;

	dsm_log_debug("%s enter\n", __func__);

	if (!client) {
		dsm_log_err("dsm can't poll without client\n");
		goto out;
	}

	dsm_log_debug("client name :%s\n", client->client_name);
	poll_wait(file, &client->waitq, wait);

	if (test_bit(CBUFF_READY_BIT, &client->buff_flag)) {
	#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
		if (dsm_is_errno_in_da_range(client->error_no)) {
			if (dsm_kfifo_get_data_len() > 0)
				mask = POLLIN | POLLRDNORM;
			goto out;
		}
	#endif
		mask = POLLIN | POLLRDNORM;
	}
out:
	dsm_log_debug("%s exit, mask:%d\n", __func__, mask);
	return mask;
}

static int dsm_open(struct inode *inode, struct file *file)
{
	dsm_log_debug("%s enter\n", __func__);
	file->private_data = NULL;
	dsm_log_debug("%s exit\n", __func__);

	return 0;
}

static int dsm_close(struct inode *inode, struct file *file)
{
	struct dsm_client *client = file->private_data;

	dsm_log_debug("%s enter\n", __func__);

	if (client)
		dsm_log_debug("dsm client is null\n");
	dsm_log_debug("%s exit\n", __func__);

	return 0;
}

static long dsm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	struct dsm_client *client = (struct dsm_client *)file->private_data;
	long ret = 0;
	int error;
	char buff[CLIENT_NAME_LEN] = {0};

	dsm_log_debug("%s enter,\n", __func__);

	switch (cmd) {
	case DSM_IOCTL_GET_CLIENT_COUNT:
		mutex_lock(&g_dsm_server.mtx_lock);
		error = g_dsm_server.client_count;
		mutex_unlock(&g_dsm_server.mtx_lock);
		dsm_log_info("client count :%d\n", error);
		ret = copy_int_to_user(argp, error);
		break;
	case DSM_IOCTL_BIND:
		if (copy_from_user(buff, argp, CLIENT_NAME_LEN - 1)) {
			dsm_log_err("copy from user failed\n");
			ret = -EFAULT;
			break;
		}
		dsm_log_debug("try bind client %s\n", buff);
		client = dsm_find_client(buff);
		if (client)
			file->private_data = (void *)client;
		else
			ret = -ENXIO;
		break;
	case DSM_IOCTL_POLL_CLIENT_STATE:
		if (client && client->cops && client->cops->poll_state) {
			error = client->cops->poll_state();
			dsm_log_info("poll %s state result :%d\n",
				     client->client_name,
				     error);
			ret = copy_int_to_user(argp, error);
			break;
		}
		dsm_log_err("dsm client not bound or poll not support\n");
		ret = -ENXIO;
		break;
	case DSM_IOCTL_FORCE_DUMP:
		if (copy_from_user(buff, argp, UINT_BUF_MAX)) {
			dsm_log_err("copy from user failed\n");
			ret = -EFAULT;
			break;
		}
		if ((!client) || (!client->cops) || (!client->cops->dump_func)) {
			dsm_log_err("dsm client not bound or dump not support\n");
			ret = -ENXIO;
			break;
		}
		if (dsm_client_ocuppy(client)) {
			dsm_log_info("client %s's buff ocuppy failed\n",
				     client->client_name);
			ret = -EBUSY;
			break;
		}
		client->error_no = dsm_atoi(buff);
		client->used_size = client->cops->dump_func(client->error_no,
							    (void *)client->dump_buff,
							    (int)client->buff_size);
		set_bit(CBUFF_READY_BIT, &client->buff_flag);
		break;
	case DSM_IOCTL_GET_CLIENT_ERROR:
		if (client) {
			ret = copy_int_to_user(argp, client->error_no);
			break;
		}
		dsm_log_err("dsm find client failed\n");
		ret = -ENXIO;
		break;
	case DSM_IOCTL_GET_DEVICE_NAME:
		if (client && (strlen(client->device_name) > 0)) {
			ret = copy_to_user(argp,
					   client->device_name,
					   DSM_MAX_DEVICE_NAME_LEN);
			break;
		}
		ret = -ENXIO;
		break;
	case DSM_IOCTL_GET_IC_NAME:
		if (client && (strlen(client->ic_name) > 0)) {
			ret = copy_to_user(argp,
					   client->ic_name,
					   DSM_MAX_IC_NAME_LEN);
			break;
		}
		ret = -ENXIO;
		break;
	case DSM_IOCTL_GET_MODULE_NAME:
		if (client && (strlen(client->module_name) > 0)) {
			ret = copy_to_user(argp,
					   client->module_name,
					   DSM_MAX_MODULE_NAME_LEN);
			break;
		}
		ret = -ENXIO;
		break;
	default:
		dsm_log_err("unknown ioctl command :%d\n", cmd);
		ret = -EINVAL;
		break;
	}
	dsm_log_debug("%s exit\n", __func__);

	return ret;
}

static const struct file_operations dsm_fops = {
	.owner		= THIS_MODULE,
	.llseek		= no_llseek,
	.unlocked_ioctl	= dsm_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= dsm_ioctl,
#endif
	.open		= dsm_open,
	.release	= dsm_close,
	.read		= dsm_read,
	.write		= dsm_write,
	.poll		= dsm_poll,
};

/* cnotify format: |client name|,|error no|,|contents| */
static ssize_t dsm_cnotify_store(struct device *dev, struct device_attribute *attr,
				 const char *buf, size_t count)
{
	char *strings = NULL;

	dsm_log_debug("%s enter\n", __func__);

	if (!buf) {
		dsm_log_err("buf is null\n");
		goto out;
	}

	if ((count > DSM_MAX_LOG_SIZE) || (count < DSM_MIN_LOG_SIZE)) {
		dsm_log_err("count is %zu,out of range\n", count);
		goto out;
	}

	strings = kzalloc(count, GFP_KERNEL);
	if (!strings) {
		dsm_log_err("dsm write malloc failed\n");
		goto out;
	}

	memcpy(strings, buf, count - 1);
	*(strings + count - 1) = '\0';
	dsm_get_report_info(strings);
out:
	kfree(strings);
	dsm_log_debug("%s exit\n", __func__);
	return count;
}

static struct device_attribute dsm_interface_attrs[] = {
	__ATTR(client_notify, 0200, NULL, dsm_cnotify_store),
};

static struct miscdevice dsm_miscdev = {
	.minor	= DSM_MINOR,
	.name	= "dsm",
	.fops	= &dsm_fops,
};

static int __init dsm_init(void)
{
	int ret = -EIO;
	int i;

	memset(&g_dsm_server, 0, sizeof(struct dsm_server));
	g_dsm_server.server_state = DSM_SERVER_UNINIT;
	mutex_init(&g_dsm_server.mtx_lock);
	g_dsm_server.dsm_wq = create_singlethread_workqueue("dsm_wq");

	if (IS_ERR(g_dsm_server.dsm_wq)) {
		dsm_log_err("alloc workqueue failed\n");
		goto out;
	}

	INIT_WORK(&dsm_work, dsm_work_func);
	ret = misc_register(&dsm_miscdev);
	if (ret) {
		dsm_log_err("misc register failed\n");
		goto out;
	}

	g_dsm_server.server_state = DSM_SERVER_INITED;
	for (i = 0; i < ARRAY_SIZE(dsm_interface_attrs); i++) {
		ret = device_create_file(dsm_miscdev.this_device,
					 &dsm_interface_attrs[i]);
		if (ret < 0)
			dsm_log_err("creating sysfs attribute %s failed: %d\n",
				    dsm_interface_attrs[i].attr.name,
				    ret);
	}

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	memset(&g_dsm_msgq, 0, sizeof(struct dsm_msgq));
	spin_lock_init(&g_dsm_msgq.fifo_lock);
	mutex_init(&g_dsm_msgq.read_lock);

	if (kfifo_alloc(&g_dsm_msgq.msg_fifo, MSGQ_SIZE, GFP_KERNEL)) {
		dsm_log_err("alloc message queue failed\n");
		ret = -ENOMEM;
		goto out;
	}
#endif

out:
	dsm_log_info("%s called, ret %d\n", __func__, ret);
	return ret;
}

module_init(dsm_init);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Device state monitor");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");

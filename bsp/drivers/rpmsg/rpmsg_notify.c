/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi's rpmsg notify driver
 *
 * the driver provides notification mechanism base on rpmsg.
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/workqueue.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include "linux/types.h"
#include <uapi/linux/sched/types.h>
#include <linux/aw_rpmsg.h>

#define AW_RPMSG_NOTIFY_VERSION "1.0.2"

#define RPMSG_NOTIFY_NAME_MAX				32
#define RPMSG_NOTIFY_TIMEOUT				500

static DEFINE_SPINLOCK(g_spin_lock);
static LIST_HEAD(g_srm_list);
static LIST_HEAD(g_device_list);
static LIST_HEAD(g_wait_list);

struct rpmsg_notify_wait {
	char *ser_name;
	struct list_head list;
	struct delayed_work work;
	int len;
	unsigned int timeout;
	uint8_t data[];
};

struct rpmsg_notify_entry {
	char name[RPMSG_NOTIFY_NAME_MAX];
	char ser[RPMSG_NOTIFY_NAME_MAX];
	notify_callback callback;
	void *dev;
	struct list_head list;
};

struct rpmsg_notify_service {
	char name[RPMSG_NOTIFY_NAME_MAX];
	struct device *dev;
	struct rpmsg_device *rpdev;

	spinlock_t list_lock;
	struct list_head list;
	struct list_head notify;
	struct task_struct *daemon;

	struct sk_buff_head queue;
	wait_queue_head_t rq;
	spinlock_t queue_lock;
};

struct rpmsg_device *rpmsg_notify_rpdev;

static void rpmsg_notify_check_wait(struct rpmsg_notify_entry *notify);

struct rpmsg_device *rpmsg_notify_get_rpdev(void)
{
	return rpmsg_notify_rpdev;
}
EXPORT_SYMBOL(rpmsg_notify_get_rpdev);

static struct rpmsg_notify_service *
rpmsg_notify_get_service_by_name(const char *name)
{
	struct rpmsg_notify_service *pos, *tmp;

	list_for_each_entry_safe(pos, tmp, &g_srm_list, list) {
		if (strncmp(pos->name, name, RPMSG_NOTIFY_NAME_MAX))
			continue;
		return pos;
	}

	return NULL;
}

int rpmsg_notify_add(const char *ser_name, const char *name, notify_callback cb, void *dev)
{
	bool no_ser = false;
	struct rpmsg_notify_service *ser;
	struct rpmsg_notify_entry *notify;

	ser = rpmsg_notify_get_service_by_name(ser_name);
	if (!ser)
		no_ser = true;

	if (strlen(name) >= RPMSG_NOTIFY_NAME_MAX) {
		pr_err("srm name to long, expect < %d.\n", RPMSG_NOTIFY_NAME_MAX);
		return -EINVAL;
	}

	notify = kmalloc(sizeof(*notify), GFP_KERNEL);
	if (IS_ERR_OR_NULL(notify)) {
		pr_err("failed to alloc rpmsg_notify_entry.\n");
		return -ENOMEM;
	}

	memcpy(notify->name, name, strlen(name) + 1);
	memcpy(notify->ser, ser_name, strlen(ser_name) + 1);
	notify->callback = cb;
	notify->dev = dev;

	if (!no_ser) {
		spin_lock(&ser->list_lock);
		list_add(&notify->list, &ser->notify);
		spin_unlock(&ser->list_lock);

		dev_dbg(ser->dev, "add srm: %s.\n", name);
		rpmsg_notify_check_wait(notify);
	} else {
		spin_lock(&g_spin_lock);
		list_add(&notify->list, &g_device_list);
		spin_unlock(&g_spin_lock);
		pr_debug("add %s(ser:%s) srm to global device list.\n", name, ser_name);
	}

	return 0;
}
EXPORT_SYMBOL(rpmsg_notify_add);

int rpmsg_notify_del(const char *ser_name, const char *name)
{
	bool no_ser = false;
	struct rpmsg_notify_entry *pos, *tmp;
	struct rpmsg_notify_service *ser;

	ser = rpmsg_notify_get_service_by_name(ser_name);
	if (!ser)
		no_ser = true;

	if (strlen(name) >= RPMSG_NOTIFY_NAME_MAX) {
		pr_err("srm name to long, expect < %d.\n", RPMSG_NOTIFY_NAME_MAX);
		return -EINVAL;
	}

	if (no_ser)
		goto no_ser;
	spin_lock(&ser->list_lock);
	list_for_each_entry_safe(pos, tmp, &ser->notify, list) {
		if (strncmp(pos->name, name, RPMSG_NOTIFY_NAME_MAX))
			continue;

		list_del_init(&pos->list);
		kfree(pos);
		spin_unlock(&ser->list_lock);

		dev_dbg(ser->dev, "del srm: %s.\n", name);

		return 0;
	}
	spin_unlock(&ser->list_lock);
	return -ENXIO;

no_ser:
	list_for_each_entry_safe(pos, tmp, &g_device_list, list) {
		if (strncmp(pos->name, name, RPMSG_NOTIFY_NAME_MAX))
			continue;

		spin_lock(&g_spin_lock);
		list_del_init(&pos->list);
		spin_unlock(&g_spin_lock);
		kfree(pos);

		pr_debug("del srm: %s.\n", name);
		return 0;
	}

	pr_warn("no such %s srm.\n", name);
	return -ENXIO;
}
EXPORT_SYMBOL(rpmsg_notify_del);

static int rpmsg_notify_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct rpmsg_notify_service *ser = dev_get_drvdata(&rpdev->dev);
	struct sk_buff *skb;

	dev_dbg(ser->dev, "ept rx package.\n");

	skb = alloc_skb(len, GFP_ATOMIC);
	if (IS_ERR_OR_NULL(skb))
		return -ENOMEM;

	memcpy(skb_put(skb, len), data, len);

	spin_lock(&ser->queue_lock);
	skb_queue_tail(&ser->queue, skb);
	spin_unlock(&ser->queue_lock);

	wake_up_interruptible(&ser->rq);

	return 0;
}

static void rpmsg_notify_work_func(struct work_struct *work)
{
	struct rpmsg_notify_wait *wait;
	char *name;

	wait = container_of(to_delayed_work(work), struct rpmsg_notify_wait, work);

	name = wait->data;
	if (wait->len > RPMSG_NOTIFY_NAME_MAX)
		name[RPMSG_NOTIFY_NAME_MAX - 1] = '\0';

	pr_debug("rpmsg_notify : cancel %s wait.\n", name);

	/* maybe this work has been removed by rpmsg_notify_remove */
	if (!list_empty(&wait->list)) {
		spin_lock(&g_spin_lock);
		list_del_init(&wait->list);
		spin_unlock(&g_spin_lock);
	}
	kfree(wait);
}

static void rpmsg_notify_add_wait(char *ser_name, void *data, int len, int timeout)
{
	struct rpmsg_notify_wait *wait;

	wait = kmalloc(sizeof(*wait) + len, GFP_KERNEL);
	if (IS_ERR_OR_NULL(wait)) {
		pr_err("rpmsg_notify_add_wait failed,no memory.\n");
		return;
	}

	wait->ser_name = ser_name;
	wait->len = len;
	wait->timeout = msecs_to_jiffies(timeout);
	memcpy(wait->data, data, len);
	INIT_DELAYED_WORK(&wait->work, rpmsg_notify_work_func);

	spin_lock(&g_spin_lock);
	list_add(&wait->list, &g_wait_list);
	spin_unlock(&g_spin_lock);
	schedule_delayed_work(&wait->work, wait->timeout);
}

static void rpmsg_notify_check_wait(struct rpmsg_notify_entry *notify)
{
	void *buf = NULL;
	char *name = NULL;
	int len = 0;
	struct rpmsg_notify_wait *pos, *tmp;

	spin_lock(&g_spin_lock);
	list_for_each_entry_safe(pos, tmp, &g_wait_list, list) {

		name = pos->data;
		if (pos->len > RPMSG_NOTIFY_NAME_MAX) {
			name[RPMSG_NOTIFY_NAME_MAX - 1] = '\0';
			len = pos->len - RPMSG_NOTIFY_NAME_MAX;
			buf = &name[RPMSG_NOTIFY_NAME_MAX];
		} else {
			len = 0;
			buf = NULL;
		}

		if (strncmp(notify->ser, pos->ser_name, RPMSG_NOTIFY_NAME_MAX))
			continue;
		if (strncmp(notify->name, name, RPMSG_NOTIFY_NAME_MAX))
			continue;

		/* it will be free by rpmsg_notify_work_func */
		list_del_init(&pos->list);

		dev_dbg(notify->dev, "Calling %s cb.\n", notify->name);
		spin_unlock(&g_spin_lock);
		notify->callback(notify->dev, buf, len);
		spin_lock(&g_spin_lock);

	}
	spin_unlock(&g_spin_lock);
}

static int rpmsg_notify_thread(void *data)
{
	struct rpmsg_notify_service *ser = data;
	unsigned long flags;
	struct sk_buff *skb;
	int len;
	void *buf;
	char *name;
	struct rpmsg_notify_entry *pos, *tmp, *entry;

	while (1) {
		name = buf = NULL;
		len = 0;

		/* wait for data in queue */
		if (skb_queue_empty(&ser->queue)) {
			if (wait_event_interruptible(ser->rq,
							!skb_queue_empty(&ser->queue) ||
							!ser->rpdev->ept ||
							kthread_should_stop()))
				break;

			if (!ser->rpdev->ept) {
				dev_dbg(ser->dev, "ept is destroy.\n");
				break;
			}
			if (kthread_should_stop()) {
				dev_dbg(ser->dev, "notify thread stop.\n");
				break;
			}
		}

		spin_lock_irqsave(&ser->queue_lock, flags);
		skb = skb_dequeue(&ser->queue);
		spin_unlock_irqrestore(&ser->queue_lock, flags);
		if (!skb) {
			dev_warn(ser->dev, "queue is empty.\n");
			continue;
		}
		entry = NULL;
		name = skb->data;
		/* We only check the first 32 characters. */
		if (skb->len > RPMSG_NOTIFY_NAME_MAX) {
			name[RPMSG_NOTIFY_NAME_MAX - 1] = '\0';
			len = skb->len - RPMSG_NOTIFY_NAME_MAX;
			buf = &name[RPMSG_NOTIFY_NAME_MAX];
		}

		spin_lock(&ser->list_lock);
		list_for_each_entry_safe(pos, tmp, &ser->notify, list) {
			if (strncmp(pos->name, name, RPMSG_NOTIFY_NAME_MAX))
				continue;
			entry = pos;
			break;
		}
		spin_unlock(&ser->list_lock);

		if (!entry) {
			/* trn again in next time */
			dev_dbg(ser->dev, "move %s notify to wait queue.\n", name);
			rpmsg_notify_add_wait(ser->name, skb->data, skb->len,
										RPMSG_NOTIFY_TIMEOUT);
			goto _clean_skb;
		}

		dev_dbg(ser->dev, "Calling %s cb.\n", entry->name);
		entry->callback(entry->dev, buf, len);
_clean_skb:
		kfree_skb(skb);
	}

	do_exit(0);

	return 0;
}

static int rpmsg_notify_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_notify_service *service;
	struct rpmsg_notify_entry *pos, *tmp;
	struct sched_param parm;

	service = devm_kmalloc(&rpdev->dev, sizeof(*service), GFP_KERNEL);
	if (IS_ERR_OR_NULL(service)) {
		dev_err(&rpdev->dev, "failed alloc rpmsg_notify_service.\n");
		return -ENOMEM;
	}
	/*
	 * Normally, the rpmsg device is created from:
	 *     remoteproc device    <--- defined by vendors, usually in device tree
	 *     |--- rproc     <--- rproc_alloc()
	 *          |--- rproc_vdev    <---- rproc_handle_vdev()
	 *               |--- virtio    <--- register_virtio_device()
	 *                    |--- rpmsg    <--- rpmsg_register_device()
	 *
	 * Therefore the 4th parent of rpmsg device is the underlying remoteproc
	 * device.

	 */
	service->dev = &rpdev->dev;
	strncpy(&service->name[0], dev_name(service->dev->parent->parent->parent->parent),
					RPMSG_NOTIFY_NAME_MAX);
	service->rpdev = rpdev;
	spin_lock_init(&service->list_lock);
	INIT_LIST_HEAD(&service->notify);
	spin_lock(&g_spin_lock);
	list_add(&service->list, &g_srm_list);
	spin_unlock(&g_spin_lock);

	spin_lock_init(&service->queue_lock);
	skb_queue_head_init(&service->queue);
	init_waitqueue_head(&service->rq);

	dev_set_drvdata(&rpdev->dev, service);
	dev_dbg(service->dev, "register srm service:%s.\n", service->name);

	spin_lock(&service->list_lock);
	list_for_each_entry_safe(pos, tmp, &g_device_list, list) {
		if (strncmp(pos->ser, service->name, RPMSG_NOTIFY_NAME_MAX))
			continue;
		spin_lock(&g_spin_lock);
		list_del_init(&pos->list);
		spin_unlock(&g_spin_lock);
		list_add(&pos->list, &service->notify);
		dev_dbg(service->dev, "move srm: %s.\n", pos->name);
	}
	spin_unlock(&service->list_lock);

	service->daemon = kthread_run(rpmsg_notify_thread, service, "rpmsg_notify");
	/* increase thread priority */
	parm.sched_priority = MAX_RT_PRIO - 10;
	sched_setscheduler(service->daemon, SCHED_FIFO, &parm);

	/* wo need to announce the new ept to remote */
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	rpmsg_notify_rpdev = rpdev;

	return 0;
}

static void rpmsg_notify_remove(struct rpmsg_device *rpdev)
{
	struct rpmsg_notify_service *ser = dev_get_drvdata(&rpdev->dev);
	struct rpmsg_notify_entry *pos, *tmp;
	struct rpmsg_notify_wait *wpos, *wtmp;

	dev_info(&rpdev->dev, "rpmsg srm driver %s is removed\n", &ser->name[0]);

	kthread_stop(ser->daemon);
	spin_lock(&g_spin_lock);
	list_del_init(&ser->list);
	spin_unlock(&g_spin_lock);

	spin_lock(&ser->list_lock);
	list_for_each_entry_safe(pos, tmp, &ser->notify, list) {
		list_del_init(&pos->list);
		kfree(pos);
	}
	spin_unlock(&ser->list_lock);

	/* if all device is remove, we need clean resource */
	if (!list_empty(&g_srm_list))
		return;

	list_for_each_entry_safe(pos, tmp, &g_device_list, list) {
		spin_lock(&g_spin_lock);
		list_del_init(&pos->list);
		spin_unlock(&g_spin_lock);
		kfree(pos);
	}

	spin_lock(&g_spin_lock);
	list_for_each_entry_safe(wpos, wtmp, &g_wait_list, list) {
		list_del_init(&wpos->list);
		/* it will be free by rpmsg_notify_work_func */
	}
	spin_unlock(&g_spin_lock);
}

static struct rpmsg_device_id rpmsg_driver_srm_id_table[] = {
	{ .name	= "sunxi,notify" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_srm_id_table);

static struct rpmsg_driver rpmsg_notify_client = {
	.drv.name	= KBUILD_MODNAME,
	.id_table	= rpmsg_driver_srm_id_table,
	.probe		= rpmsg_notify_probe,
	.callback	= rpmsg_notify_cb,
	.remove		= rpmsg_notify_remove,
};
#if IS_ENABLED(CONFIG_SUNXI_RPROC_FASTBOOT)
fast_rpmsg_driver(rpmsg_notify_client);
#else
module_rpmsg_driver(rpmsg_notify_client);
#endif

MODULE_DESCRIPTION("Remote processor messaging sample client driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("lijiajian@allwinnertech.com");
MODULE_VERSION(AW_RPMSG_NOTIFY_VERSION);

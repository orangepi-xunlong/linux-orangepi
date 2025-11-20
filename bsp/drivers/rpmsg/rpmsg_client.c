/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi's rpmsg client driver.
 *
 * the driver will register the rpmsg device as a character
 * device and provide file read and write interface.
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/idr.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/rpmsg.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <uapi/linux/rpmsg.h>

#include "rpmsg_internal.h"
#include "rpmsg_master.h"

#include <linux/aw_rpmsg.h>

#define RPMSG_EPT_RX_QUEUE_MAX_LEN			(32)

#define INVALID_EPT_DEV_DEVT 0xFFFFFFFF

#define dev_to_eptdev(dev) container_of(dev, struct rpmsg_eptdev, dev)
#define cdev_to_eptdev(i_cdev) container_of(i_cdev, struct rpmsg_eptdev, cdev)

/**
 * struct rpmsg_eptdev - endpoint device context
 * @dev:	endpoint device
 * @cdev:	cdev for the endpoint device
 * @rpdev:	underlaying rpmsg device
 * @rpdev_refcnt:	rpdev's reference count, rpdev will be removed when it drops to 0
 * @sem:	post sem when rpdev is removed, wait it to ensure that rpdev is no longer in use
 * @removed:	set to 1 after rpdev is removed, and it means that file should be closed
 * @chinfo:	info used to open the endpoint
 * @ept_lock:	synchronization of write file operations
 * @queue_lock:	synchronization of @queue operations
 * @queue:	incoming message queue
 * @readq:	wait object for incoming queue
 */
struct rpmsg_eptdev {
	struct device dev;
	struct cdev cdev;

	struct rpmsg_device *rpdev;
	atomic_long_t rpdev_refcnt;
	struct semaphore sem;
	int removed;

	u32 ctrl_id;
	u32 id;

	struct mutex ept_lock;

	spinlock_t queue_lock;
	struct sk_buff_head queue;
	wait_queue_head_t readq;

#ifdef CONFIG_AW_RPMSG_PERF_TRACE
	int is_deliver_perf_data;
#endif
};

struct async_work_t {
	struct work_struct work;
	void (*func)(void *);
	void *priv;
};

static void async_worker_func(struct work_struct *work)
{
	struct async_work_t *async_work = container_of(work, struct async_work_t, work);

	async_work->func(async_work->priv);

	kfree(async_work);
}

static int async_work(const char *name, void (*func)(void *), void *priv)
{
	struct async_work_t *async_work = kzalloc(sizeof(*async_work), GFP_KERNEL);

	if (!async_work)
		return -ENOMEM;

	async_work->func = func;
	async_work->priv = priv;
	INIT_WORK(&async_work->work, async_worker_func);

	queue_work(system_unbound_wq, &async_work->work);
	return 0;
}

static int rpmsg_eptdev_get_rpdev(struct rpmsg_eptdev *eptdev)
{
	if (atomic_long_inc_not_zero(&eptdev->rpdev_refcnt))
		return 0;

	return -1;
}

static void rpmsg_eptdev_put_rpdev(struct rpmsg_eptdev *eptdev)
{
	if (atomic_long_dec_and_test(&eptdev->rpdev_refcnt)) {
		eptdev->rpdev = NULL;
		smp_mb();
		up(&eptdev->sem);
	}
}

static int rpmsg_eptdev_official_cb(struct rpmsg_device *rpdev, void *buf, int len,
			void *priv, u32 addr)
{
	struct rpmsg_eptdev *eptdev = priv;
	struct sk_buff *skb;
	int data_len;

#ifdef CONFIG_AW_RPMSG_PERF_TRACE
	int ret, is_deliver_perf_data;
	rpmsg_delivery_perf_data_t delivery_perf_data;
#endif

	if (eptdev->removed)
		return 0;

	if (skb_queue_len(&eptdev->queue) > RPMSG_EPT_RX_QUEUE_MAX_LEN) {
		dev_info_ratelimited(&eptdev->dev, "rx queue is full.\r\n");
		goto out;
	}

	data_len = len;

#ifdef CONFIG_AW_RPMSG_PERF_TRACE
	is_deliver_perf_data = eptdev->is_deliver_perf_data;
	if (is_deliver_perf_data) {
		data_len = sizeof(rpmsg_delivery_perf_data_t) + len;
	}
#endif

	skb = alloc_skb(data_len, GFP_KERNEL);
	if (!skb)
		return -ENOMEM;

#ifdef CONFIG_AW_RPMSG_PERF_TRACE
	if (is_deliver_perf_data && !rpmsg_eptdev_get_rpdev(eptdev)) {
		rpmsg_record_receiver_end_ts(rpdev->ept);
		ret = rpmsg_get_perf_data(rpdev->ept, &delivery_perf_data.data);
		rpmsg_eptdev_put_rpdev(eptdev);
		if (ret) {
			dev_info_ratelimited(&eptdev->dev, "get rpmsg performance data failed, ret: %d\n", ret);
			delivery_perf_data.magic = 0;
		} else {
			delivery_perf_data.magic = RPMSG_DELIVERY_PERF_DATA_MAGIC;
			delivery_perf_data.data_len = sizeof(rpmsg_perf_data_t);
		}

		skb_put_data(skb, &delivery_perf_data, sizeof(delivery_perf_data));
	}
#endif

	memcpy(skb_put(skb, len), buf, len);

	spin_lock(&eptdev->queue_lock);
	skb_queue_tail(&eptdev->queue, skb);
	spin_unlock(&eptdev->queue_lock);

out:
	/* wake up any blocking processes, waiting for new data */
	wake_up_interruptible(&eptdev->readq);

	return 0;
}

static int rpmsg_eptdev_open(struct inode *inode, struct file *filp)
{
	struct rpmsg_eptdev *eptdev = cdev_to_eptdev(inode->i_cdev);
	struct device *dev = &eptdev->dev;

	get_device(dev);

	filp->private_data = eptdev;

	return 0;
}

static int rpmsg_eptdev_release(struct inode *inode, struct file *filp)
{
	struct rpmsg_eptdev *eptdev = cdev_to_eptdev(inode->i_cdev);
	struct device *dev = &eptdev->dev;

	put_device(dev);

	return 0;
}

static ssize_t rpmsg_eptdev_read(struct file *filp, char __user *buf,
				 size_t len, loff_t *f_pos)
{
	struct rpmsg_eptdev *eptdev = filp->private_data;
	unsigned long flags;
	struct sk_buff *skb;
	int use;

	if (eptdev->removed)
		return -EPIPE;

	spin_lock_irqsave(&eptdev->queue_lock, flags);

	/* Wait for data in the queue */
	if (skb_queue_empty(&eptdev->queue)) {
		spin_unlock_irqrestore(&eptdev->queue_lock, flags);

		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;

		/* Wait until we get data or the endpoint goes away */
		if (wait_event_interruptible(eptdev->readq,
					     !skb_queue_empty(&eptdev->queue) ||
					     eptdev->removed))
			return -ERESTARTSYS;

		/* We lost the endpoint while waiting */
		if (eptdev->removed)
			return -EPIPE;

		spin_lock_irqsave(&eptdev->queue_lock, flags);
	}

	skb = skb_dequeue(&eptdev->queue);
	spin_unlock_irqrestore(&eptdev->queue_lock, flags);
	if (!skb)
		return -EFAULT;

	use = min_t(size_t, len, skb->len);
	if (copy_to_user(buf, skb->data, use))
		use = -EFAULT;

	kfree_skb(skb);

	return use;
}

static ssize_t rpmsg_eptdev_write(struct file *filp, const char __user *buf,
				  size_t len, loff_t *f_pos)
{
	struct rpmsg_eptdev *eptdev = filp->private_data;
	void *kbuf;
	int ret;

	if (eptdev->removed) {
		return -EPIPE;
	}

	kbuf = memdup_user(buf, len);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	if (mutex_lock_interruptible(&eptdev->ept_lock)) {
		ret = -ERESTARTSYS;
		goto free_kbuf;
	}

	if (rpmsg_eptdev_get_rpdev(eptdev)) {
		ret = -EPIPE;
		goto unlock_eptdev;
	}

	if (filp->f_flags & O_NONBLOCK)
		ret = rpmsg_trysend(eptdev->rpdev->ept, kbuf, len);
	else
		ret = rpmsg_send(eptdev->rpdev->ept, kbuf, len);

	rpmsg_eptdev_put_rpdev(eptdev);

unlock_eptdev:
	mutex_unlock(&eptdev->ept_lock);

free_kbuf:
	kfree(kbuf);
	return ret < 0 ? ret : len;
}

static unsigned int rpmsg_eptdev_poll(struct file *filp, poll_table *wait)
{
	struct rpmsg_eptdev *eptdev = filp->private_data;
	unsigned int mask = 0;

	if (eptdev->removed)
		return POLLERR;

	poll_wait(filp, &eptdev->readq, wait);

	if (!skb_queue_empty(&eptdev->queue))
		mask |= POLLIN | POLLRDNORM;

	if (rpmsg_eptdev_get_rpdev(eptdev))
		return POLLERR;

	mask |= rpmsg_poll(eptdev->rpdev->ept, filp, wait);

	rpmsg_eptdev_put_rpdev(eptdev);

	return mask;
}

static long rpmsg_eptdev_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	struct rpmsg_eptdev *eptdev = filp->private_data;
	void __user *argp = (void __user *)arg;
	int is_delivery;

	switch (cmd) {
	case RPMSG_EPTDEV_DELIVER_PERF_DATA_IOCTL:
		if (!argp)
			return -EINVAL;

		if (copy_from_user(&is_delivery, argp, sizeof(is_delivery)))
			return -EFAULT;

#ifdef CONFIG_AW_RPMSG_PERF_TRACE
		eptdev->is_deliver_perf_data = is_delivery;
		return 0;
#else
		return -EFAULT;
#endif
	default:
		break;
	}

	dev_warn(&eptdev->dev, "Undown konw cmd=0x%x\r\n", cmd);

	return -EINVAL;
};

static const struct file_operations rpmsg_eptdev_fops = {
	.owner = THIS_MODULE,
	.open = rpmsg_eptdev_open,
	.release = rpmsg_eptdev_release,
	.read = rpmsg_eptdev_read,
	.write = rpmsg_eptdev_write,
	.poll = rpmsg_eptdev_poll,
	.unlocked_ioctl = rpmsg_eptdev_ioctl,
	.compat_ioctl = rpmsg_eptdev_ioctl,

};

static ssize_t name_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct rpmsg_eptdev *eptdev = dev_get_drvdata(dev);

	return sprintf(buf, "%s\n", dev_name(&eptdev->dev));
}
static DEVICE_ATTR_RO(name);

static ssize_t src_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct rpmsg_eptdev *eptdev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", eptdev->rpdev->src);
}
static DEVICE_ATTR_RO(src);

static ssize_t dst_show(struct device *dev, struct device_attribute *attr,
			 char *buf)
{
	struct rpmsg_eptdev *eptdev = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", eptdev->rpdev->dst);
}
static DEVICE_ATTR_RO(dst);

static struct attribute *rpmsg_eptdev_attrs[] = {
	&dev_attr_name.attr,
	&dev_attr_src.attr,
	&dev_attr_dst.attr,
	NULL
};
ATTRIBUTE_GROUPS(rpmsg_eptdev);

static void rpmsg_eptdev_release_device(struct device *dev)
{
	struct rpmsg_eptdev *eptdev = dev_to_eptdev(dev);

	dev_dbg(&eptdev->dev, "release rpmsg ept device, src: 0x%x, dst: 0x%x\n",
		eptdev->rpdev->src, eptdev->rpdev->dst);

	if (eptdev->dev.devt != INVALID_EPT_DEV_DEVT) {
		dev_dbg(&eptdev->dev, "release the related id of rpmsg%d\n", eptdev->dev.devt);
		rpmsg_ctrldev_put_devt(eptdev->dev.devt);
		rpmsg_ctrldev_release_id(eptdev->ctrl_id, eptdev->id);
	}

	kfree(eptdev);
}

static int rpmsg_eptdev_create(struct rpmsg_eptdev *eptdev, int master, int id)
{
	struct device *dev;
	int ret;

	dev = &eptdev->dev;

	dev->devt = rpmsg_ctrldev_get_devt();
	dev->id = id;
	dev_set_name(dev, "rpmsg%d", id);

	ret = cdev_device_add(&eptdev->cdev, dev);
	if (ret) {
		dev_err(dev, "cdev_device_add failed: %d\n", ret);
		goto release_ept_dev_id;
	}

	return 0;

release_ept_dev_id:
	/* release related id timely, eptdev will be freed when rpmsg device remove */
	rpmsg_ctrldev_put_devt(dev->devt);
	rpmsg_ctrldev_release_id(eptdev->ctrl_id, id);
	dev->devt = INVALID_EPT_DEV_DEVT;

	return ret;
}

#ifdef DEBUG
static void rpmsg_dump_msg(struct rpmsg_ctrl_msg *msg)
{
	pr_info("message info:\n");
	pr_info("\t name = %s\n", msg->name);
	pr_info("\t id = %d\n", msg->id);
	pr_info("\t ctrl_id = %d\n", msg->ctrl_id);
	pr_info("\t cmd = 0x%x\n", msg->cmd);
}
#endif

static int rpmsg_eptdev_temp_cb(struct rpmsg_device *rpdev, void *buf, int len,
			void *priv, u32 addr)
{
	struct rpmsg_eptdev *eptdev = priv;
	struct rpmsg_ctrl_msg *msg = buf;
	struct rpmsg_ctrl_msg_ack ack;
	int ret;

	dev_dbg(&rpdev->dev, "Rx len=%d\n", len);

	if (eptdev->removed)
		return 0;

	if (len != sizeof(*msg))
		return 0;
#ifdef DEBUG
	rpmsg_dump_msg(msg);
#endif

	if (msg->cmd != RPMSG_CREATE_CLIENT) {
		dev_dbg(&rpdev->dev, "Invalid Data,Create rpmsg%d failed\n", msg->id);
		return 0;
	}

	if (rpmsg_eptdev_get_rpdev(eptdev))
		return 0;

	/* create device file */
	ret = rpmsg_eptdev_create(eptdev, msg->ctrl_id, msg->id);
	if (ret) {
		ack.ack = RPMSG_ACK_NOMEM;
	} else {
		rpdev->ept->cb = rpmsg_eptdev_official_cb;
		ack.ack = RPMSG_ACK_OK;
	}

	/* send ack */
	ack.id = msg->id;
	eptdev->ctrl_id = msg->ctrl_id;
	eptdev->id = msg->id;
	rpmsg_send(eptdev->rpdev->ept, &ack, sizeof(ack));

	rpmsg_eptdev_put_rpdev(eptdev);

	return 0;
}

static int rpmsg_eptdev_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_eptdev *eptdev;
	struct device *dev;

	eptdev = kzalloc(sizeof(*eptdev), GFP_KERNEL);
	if (!eptdev)
		return -ENOMEM;

	dev = &eptdev->dev;

	/* init eptdev member */
	eptdev->rpdev = rpdev;
	atomic_long_set(&eptdev->rpdev_refcnt, 1);
	sema_init(&eptdev->sem, 0);
	eptdev->removed = 0;

	mutex_init(&eptdev->ept_lock);
	spin_lock_init(&eptdev->queue_lock);
	skb_queue_head_init(&eptdev->queue);
	init_waitqueue_head(&eptdev->readq);
	cdev_init(&eptdev->cdev, &rpmsg_eptdev_fops);
	eptdev->cdev.owner = THIS_MODULE;

	/* init device */
	device_initialize(dev);
#ifdef CONFIG_AW_RPMSG_CLASS
	dev->class = g_aw_rpmsg_class;
#else
	dev->class = rpmsg_class;
#endif
	dev->groups = rpmsg_eptdev_groups;
	dev->devt = INVALID_EPT_DEV_DEVT;

	/* We can now rely on the release function for cleanup */
	dev->release = rpmsg_eptdev_release_device;


	dev_set_drvdata(&eptdev->dev, eptdev);
	dev_set_drvdata(&rpdev->dev, eptdev);

	rpdev->ept->priv = eptdev;
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	return 0;
}

static void async_rpmsg_destroy_ept(void *priv)
{
	rpmsg_destroy_ept((struct rpmsg_endpoint *)priv);
}

static void rpmsg_eptdev_remove(struct rpmsg_device *rpdev)
{
	struct rpmsg_eptdev *eptdev = dev_get_drvdata(&rpdev->dev);
	struct device *dev = &eptdev->dev;
	struct sk_buff *skb;
	unsigned long flags;
	int ret;

	eptdev->removed = 1;
	smp_mb();
	rpmsg_eptdev_put_rpdev(eptdev);

	/* Discard all SKBs */
	spin_lock_irqsave(&eptdev->queue_lock, flags);
	while (!skb_queue_empty(&eptdev->queue)) {
		skb = skb_dequeue(&eptdev->queue);
		spin_unlock_irqrestore(&eptdev->queue_lock, flags);
		kfree_skb(skb);
		spin_lock_irqsave(&eptdev->queue_lock, flags);
	}
	spin_unlock_irqrestore(&eptdev->queue_lock, flags);

	wake_up_interruptible(&eptdev->readq);

	// wait rpdev = NULL
	while (1) {
		ret = down_interruptible(&eptdev->sem);
		if (ret == 0)
			break;
		if (ret != -EINTR) {
			dev_err(&rpdev->dev, "wait sem return %d! rpdev: %p\n", ret, eptdev->rpdev);
			break;
		}
	}
	if (async_work(dev_name(&eptdev->dev), async_rpmsg_destroy_ept, (void *)rpdev->ept)) {
		dev_err(&rpdev->dev, "async rpmsg_destroy_ept failed\n");
		// it may cuase ept->cb_lock recursive locking in current context
		rpmsg_destroy_ept(rpdev->ept);
	}
	// not need to destroy ept again
	rpdev->ept = NULL;

	if (eptdev->dev.devt != INVALID_EPT_DEV_DEVT) {
		cdev_device_del(&eptdev->cdev, dev);
	}

	put_device(dev);
}

static struct rpmsg_device_id rpmsg_driver_eptdev_id_table[] = {
	{ .name = "sunxi,rpmsg_client" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_eptdev_id_table);

static struct rpmsg_driver rpmsg_eptdev_driver = {
	.probe = rpmsg_eptdev_probe,
	.remove = rpmsg_eptdev_remove,
	.callback = rpmsg_eptdev_temp_cb,
	.drv = {
		.name = "sunxi,rpmsg_client",
	},
	.id_table = rpmsg_driver_eptdev_id_table,
};

module_rpmsg_driver(rpmsg_eptdev_driver);

MODULE_AUTHOR("lijiajian@allwinnertech.com");
MODULE_LICENSE("GPL v2");

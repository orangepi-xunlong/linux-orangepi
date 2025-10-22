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
#include <uapi/linux/rpmsg.h>

#include "rpmsg_internal.h"
#include "rpmsg_master.h"

#define RPMSG_EPT_RX_QUEUE_MAX_LEN			(32)

#define dev_to_eptdev(dev) container_of(dev, struct rpmsg_eptdev, dev)
#define cdev_to_eptdev(i_cdev) container_of(i_cdev, struct rpmsg_eptdev, cdev)

/**
 * struct rpmsg_eptdev - endpoint device context
 * @dev:	endpoint device
 * @cdev:	cdev for the endpoint device
 * @rpdev:	underlaying rpmsg device
 * @chinfo:	info used to open the endpoint
 * @ept_lock:	synchronization of @ept modifications
 * @ept:	rpmsg endpoint reference, when open
 * @queue_lock:	synchronization of @queue operations
 * @queue:	incoming message queue
 * @readq:	wait object for incoming queue
 */
struct rpmsg_eptdev {
	struct device dev;
	struct cdev cdev;

	struct rpmsg_device *rpdev;
	u32 ctrl_id;
	u32 id;

	struct mutex ept_lock;
	struct rpmsg_endpoint *ept;

	spinlock_t queue_lock;
	struct sk_buff_head queue;
	wait_queue_head_t readq;
};

static int rpmsg_ept_cb(struct rpmsg_device *rpdev, void *buf, int len,
			void *priv, u32 addr)
{
	struct rpmsg_eptdev *eptdev = priv;
	struct sk_buff *skb;

	if (skb_queue_len(&eptdev->queue) > RPMSG_EPT_RX_QUEUE_MAX_LEN) {
		dev_info_ratelimited(&eptdev->dev, "rx queue is full.\r\n");
		goto out;
	}

	skb = alloc_skb(len, GFP_KERNEL);
	if (!skb)
		return -ENOMEM;

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

	if (!eptdev->ept)
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
					     !eptdev->ept))
			return -ERESTARTSYS;

		/* We lost the endpoint while waiting */
		if (!eptdev->ept)
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

	kbuf = memdup_user(buf, len);
	if (IS_ERR(kbuf))
		return PTR_ERR(kbuf);

	if (mutex_lock_interruptible(&eptdev->ept_lock)) {
		ret = -ERESTARTSYS;
		goto free_kbuf;
	}

	if (!eptdev->ept) {
		ret = -EPIPE;
		goto unlock_eptdev;
	}

	if (filp->f_flags & O_NONBLOCK)
		ret = rpmsg_trysend(eptdev->ept, kbuf, len);
	else
		ret = rpmsg_send(eptdev->ept, kbuf, len);

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

	if (!eptdev->ept)
		return POLLERR;

	poll_wait(filp, &eptdev->readq, wait);

	if (!skb_queue_empty(&eptdev->queue))
		mask |= POLLIN | POLLRDNORM;

	mask |= rpmsg_poll(eptdev->ept, filp, wait);

	return mask;
}

static const struct file_operations rpmsg_eptdev_fops = {
	.owner = THIS_MODULE,
	.open = rpmsg_eptdev_open,
	.release = rpmsg_eptdev_release,
	.read = rpmsg_eptdev_read,
	.write = rpmsg_eptdev_write,
	.poll = rpmsg_eptdev_poll,
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

	dev_dbg(&eptdev->dev, "release device rpmsg%d\n", eptdev->dev.devt);
	rpmsg_ctrldev_put_devt(eptdev->dev.devt);
	rpmsg_ctrldev_notify(eptdev->ctrl_id, eptdev->id);
}

static int rpmsg_eptdev_create(struct rpmsg_eptdev *eptdev, int master, int id)
{
	struct device *dev;
	int ret;

	dev = &eptdev->dev;

	dev->devt = rpmsg_ctrldev_get_devt();
	dev->id = id;
	dev_set_name(dev, "rpmsg%d", id);

	ret = cdev_add(&eptdev->cdev, dev->devt, 1);
	if (ret)
		goto free_ept_devt;

	/* We can now rely on the release function for cleanup */
	dev->release = rpmsg_eptdev_release_device;

	ret = device_add(dev);
	if (ret) {
		dev_err(dev, "device_add failed: %d\n", ret);
		put_device(dev);
	}

	return ret;

free_ept_devt:
	rpmsg_ctrldev_put_devt(dev->devt);

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

static int rpmsg_ept_temp_cb(struct rpmsg_device *rpdev, void *buf, int len,
			void *priv, u32 addr)
{
	struct rpmsg_eptdev *eptdev = priv;
	struct rpmsg_ctrl_msg *msg = buf;
	struct rpmsg_ctrl_msg_ack ack;

	dev_dbg(&rpdev->dev, "Rx len=%d\n", len);

	if (len != sizeof(*msg))
		return 0;
#ifdef DEBUG
	rpmsg_dump_msg(msg);
#endif

	if (msg->cmd != RPMSG_CREATE_CLIENT) {
		dev_dbg(&rpdev->dev, "Invalid Data,Create rpmsg%d failed\n", msg->id);
		return 0;
	}

	rpdev->ept->cb = rpmsg_ept_cb;
	/* create device file */
	rpmsg_eptdev_create(eptdev, msg->ctrl_id, msg->id);
	/* send ack */
	ack.id = msg->id;
	ack.ack = RPMSG_ACK_OK;
	eptdev->ctrl_id = msg->ctrl_id;
	eptdev->id = msg->id;
	rpmsg_send(eptdev->rpdev->ept, &ack, sizeof(ack));

	return 0;
}

static int rpmsg_trans_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_eptdev *eptdev;
	struct device *dev;

	eptdev = kzalloc(sizeof(*eptdev), GFP_KERNEL);
	if (!eptdev)
		return -ENOMEM;

	dev = &eptdev->dev;

	/* init eptdev member */
	eptdev->rpdev = rpdev;
	eptdev->ept = rpdev->ept;
	mutex_init(&eptdev->ept_lock);
	spin_lock_init(&eptdev->queue_lock);
	skb_queue_head_init(&eptdev->queue);
	init_waitqueue_head(&eptdev->readq);
	cdev_init(&eptdev->cdev, &rpmsg_eptdev_fops);
	eptdev->cdev.owner = THIS_MODULE;

	/* init device */
	device_initialize(dev);
#if IS_ENABLED(CONFIG_AW_KERNEL_ORIGIN)
	dev->class = aw_rpmsg_class;
#else
	dev->class = rpmsg_class;
#endif
	dev->groups = rpmsg_eptdev_groups;
	dev->release = NULL;

	dev_set_drvdata(&eptdev->dev, eptdev);
	dev_set_drvdata(&rpdev->dev, eptdev);

	eptdev->ept->priv = eptdev;
	rpdev->announce = rpdev->src != RPMSG_ADDR_ANY;

	return 0;
}

static void rpmsg_trans_remove(struct rpmsg_device *rpdev)
{
	struct rpmsg_eptdev *eptdev = dev_get_drvdata(&rpdev->dev);
	struct device *dev = &eptdev->dev;
	struct sk_buff *skb;

	/* Discard all SKBs */
	mutex_lock(&eptdev->ept_lock);
	while (!skb_queue_empty(&eptdev->queue)) {
		skb = skb_dequeue(&eptdev->queue);
		kfree_skb(skb);
	}
	mutex_unlock(&eptdev->ept_lock);

	eptdev->ept = NULL;

	wake_up_interruptible(&eptdev->readq);

	if (dev->release == NULL) {
		rpmsg_ctrldev_put_devt(eptdev->dev.devt);
		rpmsg_ctrldev_notify(eptdev->ctrl_id, eptdev->id);
		put_device(dev);
		kfree(eptdev);
	} else {
		device_del(dev);
		cdev_del(&eptdev->cdev);
		put_device(dev);
	}
}

static struct rpmsg_device_id rpmsg_driver_trans_id_table[] = {
	{ .name = "sunxi,rpmsg_client" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_chrdev_id_table);

static struct rpmsg_driver rpmsg_chrtrans_driver = {
	.probe = rpmsg_trans_probe,
	.remove = rpmsg_trans_remove,
	.callback	= rpmsg_ept_temp_cb,
	.drv = {
		.name = "sunxi,rpmsg_client",
	},
	.id_table = rpmsg_driver_trans_id_table,
};

module_rpmsg_driver(rpmsg_chrtrans_driver);

MODULE_AUTHOR("lijiajian@allwinnertech.com");
MODULE_LICENSE("GPL v2");

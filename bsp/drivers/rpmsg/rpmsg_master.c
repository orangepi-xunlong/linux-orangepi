/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi's rpmsg ctrl driver
 *
 * the driver register the rpmsg_ctrl device node,which
 * controls the creation and release of rpmsg device nodes.
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* #define DEBUG */
#include <linux/cdev.h>
#include <linux/mutex.h>
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
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/atomic.h>
#include <linux/semaphore.h>
#include <uapi/linux/rpmsg.h>
#include <linux/aw_rpmsg.h>

#include "rpmsg_internal.h"
#include "rpmsg_master.h"

#define SUNXI_RPMSG_CTRL_VERSION "1.0.9"

#define WAIT_TIMEOUT	(msecs_to_jiffies(500))

#ifdef CONFIG_AW_RPMSG_CLASS

#if IS_ENABLED(CONFIG_RPMSG_CHAR)
#define AW_RPMSG_CLASS_NAME "aw_rpmsg"
#else
#define AW_RPMSG_CLASS_NAME "rpmsg"
#endif

struct class *g_aw_rpmsg_class;
EXPORT_SYMBOL(g_aw_rpmsg_class);
#endif

static dev_t rpmsg_major;

/* record all rpmsg_ctrl device id */
static DEFINE_IDA(rpmsg_ctrl_ida);
/* record all rpmsg_client device id */
static DEFINE_IDA(rpmsg_ept_ida);
/* record all rpmsg_ctrl device devt */
static DEFINE_IDA(rpmsg_minor_ida);

#define dev_to_ctrldev(dev) container_of(dev, struct rpmsg_ctrldev, dev)
#define cdev_to_ctrldev(i_cdev) container_of(i_cdev, struct rpmsg_ctrldev, cdev)

struct rpmsg_ept_notify {
	struct rpmsg_ctrldev *ctrl;
	char name[32];
	int id;
	struct list_head list;
	struct list_head notify;
	struct completion complete;
	uint32_t status;
};

/**
 * struct rpmsg_ctrldev - control device for instantiating endpoint devices
 * @rpdev:	underlaying rpmsg device
 * @cdev:	cdev for the ctrl device
 * @dev:	device for the ctrl device
 */
struct rpmsg_ctrldev {
	struct rpmsg_device *rpdev;
	atomic_long_t rpdev_refcnt;
	struct semaphore sem;
	struct device dev;
	struct cdev cdev;

	struct mutex lock;
	struct list_head epts;
	struct list_head wait;

	struct rpmsg_ept_notify ctrl_notify;
};

struct rpmsg_ctrldev_file_priv {
	struct rpmsg_ctrldev *ctrldev;
	struct mutex epts_lock;
	struct list_head alloc_epts;
	bool epts_auto_release;
};

struct rpmsg_ept_mgr_node {
	struct list_head list;
	struct rpmsg_ept_info info;
};

dev_t rpmsg_ctrldev_get_devt(void)
{
	int ret;
	ret = ida_simple_get(&rpmsg_minor_ida, 0, RPMSG_DEV_MAX, GFP_KERNEL);
	if (ret < 0)
		return ret;
	return MKDEV(MAJOR(rpmsg_major), ret);
}
EXPORT_SYMBOL(rpmsg_ctrldev_get_devt);

void rpmsg_ctrldev_put_devt(dev_t devt)
{
	ida_simple_remove(&rpmsg_minor_ida, MINOR(devt));
}
EXPORT_SYMBOL(rpmsg_ctrldev_put_devt);

static void rpmsg_print_ack_str(struct device *dev, uint32_t ack)
{
	switch (ack) {
	case RPMSG_ACK_FAILED:
		dev_err(dev, "Operation failed\r\n");
		break;
	case RPMSG_ACK_NOLISTEN:
		dev_err(dev, "Remote don't listen this name\r\n");
		break;
	case RPMSG_ACK_BUSY:
		dev_err(dev, "Remote is busy\r\n");
		break;
	case RPMSG_ACK_NOMEM:
		dev_err(dev, "Remote not enought memory\r\n");
		break;
	case RPMSG_ACK_NOENT:
		dev_err(dev, "Remote is full\r\n");
		break;
	}
}

static int rpmsg_ctrldev_get_rpdev(struct rpmsg_ctrldev *ctrldev)
{
	if (atomic_long_inc_not_zero(&ctrldev->rpdev_refcnt))
		return 0;

	return -1;
}

static void rpmsg_ctrldev_put_rpdev(struct rpmsg_ctrldev *ctrldev)
{
	if (atomic_long_dec_and_test(&ctrldev->rpdev_refcnt)) {
		ctrldev->rpdev = NULL;
		smp_mb();
		up(&ctrldev->sem);
	}
}

static struct rpmsg_ept_notify *
rpmsg_ctrldev_notify_find(struct rpmsg_ctrldev *ctrldev, int id)
{
	struct rpmsg_ept_notify *pos, *tmp;

	mutex_lock(&ctrldev->lock);
	list_for_each_entry_safe(pos, tmp, &ctrldev->wait, notify) {
		if (id == pos->id) {
			mutex_unlock(&ctrldev->lock);
			return pos;
		}
	}
	mutex_unlock(&ctrldev->lock);

	return NULL;
}

static void rpmsg_ctrldev_notify_add(struct rpmsg_ctrldev *ctrldev,
				struct rpmsg_ept_notify *notify)
{
	mutex_lock(&ctrldev->lock);
	list_add(&notify->notify, &ctrldev->wait);
	mutex_unlock(&ctrldev->lock);
}

static void rpmsg_ctrldev_notify_del(struct rpmsg_ctrldev *ctrldev,
				struct rpmsg_ept_notify *notify)
{
	mutex_lock(&ctrldev->lock);
	if (!list_empty(&notify->notify))
		list_del_init(&notify->notify);
	mutex_unlock(&ctrldev->lock);
}

static struct rpmsg_ept_notify *
rpmsg_ctrldev_epts_find(struct rpmsg_ctrldev *ctrldev, int id)
{
	struct rpmsg_ept_notify *pos, *tmp;

	mutex_lock(&ctrldev->lock);
	list_for_each_entry_safe(pos, tmp, &ctrldev->epts, list) {
		if (id == pos->id) {
			mutex_unlock(&ctrldev->lock);
			return pos;
		}
	}
	mutex_unlock(&ctrldev->lock);

	return NULL;
}

static void rpmsg_ctrldev_epts_add(struct rpmsg_ctrldev *ctrldev,
				struct rpmsg_ept_notify *notify)
{
	mutex_lock(&ctrldev->lock);
	list_add(&notify->list, &ctrldev->epts);
	mutex_unlock(&ctrldev->lock);
}

static void rpmsg_ctrldev_epts_del_without_lock(struct rpmsg_ctrldev *ctrldev,
				struct rpmsg_ept_notify *notify)
{
	if (!list_empty(&notify->list))
		list_del_init(&notify->list);
}

static void rpmsg_ctrldev_epts_del(struct rpmsg_ctrldev *ctrldev,
				struct rpmsg_ept_notify *notify)
{
	mutex_lock(&ctrldev->lock);
	rpmsg_ctrldev_epts_del_without_lock(ctrldev, notify);
	mutex_unlock(&ctrldev->lock);
}

int rpmsg_ctrldev_release_id(int ctrl_id, int id)
{
	ida_simple_remove(&rpmsg_ept_ida, id);
	return 0;
}
EXPORT_SYMBOL(rpmsg_ctrldev_release_id);

/* tell remote to free (rpmsg%d, id) ept */
static int rpmsg_ctrldev_release_eptdev(struct rpmsg_ctrldev *ctrldev, int id)
{
	int ret;
	struct rpmsg_ctrl_msg msg;
	struct rpmsg_ept_notify *notify;

	msg.ctrl_id = ctrldev->dev.id;
	msg.id = id;
	msg.cmd = RPMSG_CLOSE_CLIENT;

	notify = rpmsg_ctrldev_epts_find(ctrldev, id);
	if (!notify) {
		dev_err(&ctrldev->dev, "/dev/rpmsg%d dev not exits \n", id);
		return -ENODEV;
	}

	if (rpmsg_ctrldev_get_rpdev(ctrldev)) {
		dev_warn(&ctrldev->dev, "rpdev has been removed, skip notify\n");
		ret = 0;
		goto out;
	}

	reinit_completion(&notify->complete);
	/* add to notify chain */
	rpmsg_ctrldev_notify_add(ctrldev, notify);

	strncpy(msg.name, notify->name, 32);
	/* tell remote close this endpoint */
	ret = rpmsg_send(ctrldev->rpdev->ept, &msg, sizeof(msg));
	rpmsg_ctrldev_put_rpdev(ctrldev);
	if (ret) {
		rpmsg_ctrldev_notify_del(ctrldev, notify);
		return ret;
	}

	/* wait endpoint notify */
	ret = wait_for_completion_timeout(&notify->complete, WAIT_TIMEOUT);
	if (!ret) {
		dev_err(&ctrldev->dev, "close %s eptdev failed(timeout)\n", notify->name);
		ret = -ETIMEDOUT;
		goto out;
	}

	if (notify->status != RPMSG_ACK_OK) {
		rpmsg_print_ack_str(&ctrldev->dev, notify->status);
		ret = -EFAULT;
		goto out;
	}

	ret = 0;
	/* delete will complete by rpmsg_ctrldev_notify */

out:
	rpmsg_ctrldev_notify_del(ctrldev, notify);

	rpmsg_ctrldev_epts_del(ctrldev, notify);
	devm_kfree(&notify->ctrl->dev, notify);

	return ret;
}

static int rpmsg_ctrldev_create_eptdev(struct rpmsg_ctrldev *ctrldev, const char *name, int id)
{
	int ret;
	struct rpmsg_ctrl_msg msg;
	struct rpmsg_ept_notify *notify = NULL;

	if (rpmsg_ctrldev_get_rpdev(ctrldev)) {
		dev_warn(&ctrldev->dev, "rpdev has been removed, can not create\n");
		return -ENODEV;
	}

	notify = devm_kzalloc(&ctrldev->dev, sizeof(*notify), GFP_KERNEL);
	if (!notify) {
		rpmsg_ctrldev_put_rpdev(ctrldev);
		return -ENOMEM;
	}

	notify->ctrl = ctrldev;
	notify->id = id;
	strncpy(notify->name, name, 32);
	init_completion(&notify->complete);

	/* fill message struct */
	strncpy(msg.name, name, 32);
	msg.ctrl_id = ctrldev->dev.id;
	msg.id = id;
	msg.cmd = RPMSG_CREATE_CLIENT;

	/* add ctrl to notify chain */
	rpmsg_ctrldev_notify_add(ctrldev, notify);

	/* tell remote to create a new endpoint */
	ret = rpmsg_send(ctrldev->rpdev->ept, &msg, sizeof(msg));
	rpmsg_ctrldev_put_rpdev(ctrldev);
	if (ret) {
		dev_err(&ctrldev->dev, "send data failed\n");
		ret = -EFAULT;
		goto del_notify;
	}

	/* wait endpoint notify */
	notify->status = RPMSG_ACK_FAILED;
	ret = wait_for_completion_timeout(&notify->complete, WAIT_TIMEOUT);
	if (!ret) {
		dev_err(&ctrldev->dev, "create %s eptdev failed(timeout)\n", name);
		ret = -ETIMEDOUT;
		goto del_notify;
	}

	if (notify->status != RPMSG_ACK_OK) {
		rpmsg_print_ack_str(&ctrldev->dev, notify->status);
		ret = -EFAULT;
		goto del_notify;
	}

	rpmsg_ctrldev_notify_del(ctrldev, notify);

	/* success create client,add it to epts list */
	rpmsg_ctrldev_epts_add(ctrldev, notify);

	return 0;

del_notify:
	rpmsg_ctrldev_notify_del(ctrldev, notify);

	devm_kfree(&notify->ctrl->dev, notify);

	return ret;
}

static int rpmsg_ctrldev_clear_eptdev(struct rpmsg_ctrldev *ctrldev, const char *name)
{
	int ret;
	struct rpmsg_ctrl_msg msg;
	struct rpmsg_ept_notify *notify, *pos, *tmp;

	if (rpmsg_ctrldev_get_rpdev(ctrldev)) {
		dev_warn(&ctrldev->dev, "rpdev has been removed, can not clear\n");
		return -ENODEV;
	}

	msg.ctrl_id = ctrldev->dev.id;
	msg.id = 0;
	msg.cmd = RPMSG_RESET_GRP_CLIENT;

	notify = &ctrldev->ctrl_notify;

	/* add to notify chain */
	rpmsg_ctrldev_notify_add(ctrldev, notify);
	strncpy(msg.name, name, 32);
	reinit_completion(&notify->complete);
	/* tell remote close this endpoint */
	ret = rpmsg_trysend(ctrldev->rpdev->ept, &msg, sizeof(msg));
	rpmsg_ctrldev_put_rpdev(ctrldev);
	if (ret) {
		rpmsg_ctrldev_notify_del(ctrldev, notify);
		return ret;
	}

	/* wait endpoint notify */
	ret = wait_for_completion_timeout(&notify->complete, WAIT_TIMEOUT);
	if (!ret) {
		dev_err(&ctrldev->dev, "clear %s group failed(timeout)\n", name);
		ret = -ETIMEDOUT;
		goto out;
	}

	if (notify->status != RPMSG_ACK_OK) {
		rpmsg_print_ack_str(&ctrldev->dev, notify->status);
		ret = -EFAULT;
		goto out;
	}

	ret = 0;
out:
	rpmsg_ctrldev_notify_del(ctrldev, notify);

	mutex_lock(&ctrldev->lock);
	list_for_each_entry_safe(pos, tmp, &ctrldev->epts, list) {
		rpmsg_ctrldev_epts_del_without_lock(ctrldev, pos);
		devm_kfree(&ctrldev->dev, pos);
	}
	mutex_unlock(&ctrldev->lock);

	return ret;
}

static int rpmsg_ctrldev_reset_eptdev(struct rpmsg_ctrldev *ctrldev)
{
	int ret;
	struct rpmsg_ctrl_msg msg;
	struct rpmsg_ept_notify *notify;

	if (rpmsg_ctrldev_get_rpdev(ctrldev)) {
		dev_warn(&ctrldev->dev, "rpdev has been removed, can not reset\n");
		return -ENODEV;
	}

	notify = &ctrldev->ctrl_notify;

	msg.ctrl_id = ctrldev->dev.id;
	msg.id = 0;
	msg.cmd = RPMSG_RESET_ALL_CLIENT;

	reinit_completion(&notify->complete);
	/* add to notify chain */
	rpmsg_ctrldev_notify_add(ctrldev, notify);
	/* tell remote close this endpoint */
	ret = rpmsg_trysend(ctrldev->rpdev->ept, &msg, sizeof(msg));
	rpmsg_ctrldev_put_rpdev(ctrldev);
	if (ret)
		goto out;

	/* wait endpoint notify */
	ret = wait_for_completion_timeout(&notify->complete, WAIT_TIMEOUT);
	if (!ret) {
		dev_err(&ctrldev->dev, "reset rpmsg_ctrl group failed(timeout)\n");
		ret = -ETIMEDOUT;
		goto out;
	}

	if (notify->status != RPMSG_ACK_OK) {
		rpmsg_print_ack_str(&ctrldev->dev, notify->status);
		ret = -EFAULT;
		goto out;
	}

	ret = 0;
out:
	rpmsg_ctrldev_notify_del(ctrldev, notify);
	return ret;
}

static inline
int rpmsg_ctrldev_eptdev_bind(struct rpmsg_ctrldev_file_priv *ctrldev_priv,
			      struct rpmsg_ept_info *info)
{
	struct rpmsg_ctrldev *ctrldev = ctrldev_priv->ctrldev;
	struct rpmsg_ept_mgr_node *node, *tmp;
	int ret;

	mutex_lock(&ctrldev_priv->epts_lock);

	ctrldev_priv->epts_auto_release = true;
	list_for_each_entry_safe(node, tmp, &ctrldev_priv->alloc_epts, list) {
		if (node->info.id == info->id) {
			dev_err(&ctrldev->dev, "bind failed, already exists ept: %u(%s)",
				node->info.id, node->info.name);
			ret = -EEXIST;
			goto exit;
		}
	}

	node = devm_kzalloc(&ctrldev->dev, sizeof(*node), GFP_KERNEL);
	if (!node) {
		dev_err(&ctrldev->dev, "no memory for bind ept: %u(%s)",
			node->info.id, node->info.name);
		ret = -ENOMEM;
		goto exit;
	}

	memcpy(&node->info, info, sizeof(node->info));
	INIT_LIST_HEAD(&node->list);
	list_add_tail(&node->list, &ctrldev_priv->alloc_epts);
	dev_info(&ctrldev->dev, "bind ept: %u(%s)", node->info.id, node->info.name);

	ret = 0;
exit:
	mutex_unlock(&ctrldev_priv->epts_lock);
	return ret;
}

static inline
int rpmsg_ctrldev_eptdev_unbind(struct rpmsg_ctrldev_file_priv *ctrldev_priv,
				struct rpmsg_ept_info *info)
{
	struct rpmsg_ctrldev *ctrldev = ctrldev_priv->ctrldev;
	struct rpmsg_ept_mgr_node *node, *tmp;
	int ret = 0;

	mutex_lock(&ctrldev_priv->epts_lock);

	if (!ctrldev_priv->epts_auto_release)
		goto exit;

	list_for_each_entry_safe(node, tmp, &ctrldev_priv->alloc_epts, list) {
		if (node->info.id == info->id) {
			dev_info(&ctrldev->dev, "unbind ept: %u(%s)",
				 node->info.id, node->info.name);
			list_del(&node->list);
			devm_kfree(&ctrldev->dev, node);
			goto exit;
		}
	}

	dev_err(&ctrldev->dev, "unbind failed, not found ept: %u(%s)",
		node->info.id, node->info.name);
	ret = -ENODEV;
exit:
	mutex_unlock(&ctrldev_priv->epts_lock);
	return ret;
}

static inline
int rpmsg_ctrldev_eptdev_auto_release(struct rpmsg_ctrldev_file_priv *ctrldev_priv)
{
	struct rpmsg_ctrldev *ctrldev = ctrldev_priv->ctrldev;
	struct rpmsg_ept_mgr_node *node, *tmp;

	mutex_lock(&ctrldev_priv->epts_lock);

	if (!ctrldev_priv->epts_auto_release)
		goto exit;

	list_for_each_entry_safe(node, tmp, &ctrldev_priv->alloc_epts, list) {
		dev_info(&ctrldev->dev, "release ept: %u(%s)", node->info.id, node->info.name);
		rpmsg_ctrldev_release_eptdev(ctrldev, node->info.id);
		devm_kfree(&ctrldev->dev, node);
	}

exit:
	mutex_unlock(&ctrldev_priv->epts_lock);
	return 0;
}

static int rpmsg_ctrldev_open(struct inode *inode, struct file *filp)
{
	struct rpmsg_ctrldev *ctrldev = cdev_to_ctrldev(inode->i_cdev);
	struct rpmsg_ctrldev_file_priv *ctrldev_priv;

	get_device(&ctrldev->dev);

	ctrldev_priv = devm_kzalloc(&ctrldev->dev, sizeof(*ctrldev_priv), GFP_KERNEL);
	if (!ctrldev_priv) {
		put_device(&ctrldev->dev);
		return -ENOMEM;
	}

	ctrldev_priv->ctrldev = ctrldev;
	INIT_LIST_HEAD(&ctrldev_priv->alloc_epts);
	mutex_init(&ctrldev_priv->epts_lock);
	ctrldev_priv->epts_auto_release = false;

	filp->private_data = ctrldev_priv;
	return 0;
}

static int rpmsg_ctrldev_release(struct inode *inode, struct file *filp)
{
	struct rpmsg_ctrldev *ctrldev = cdev_to_ctrldev(inode->i_cdev);
	struct rpmsg_ctrldev_file_priv *ctrldev_priv = filp->private_data;

	rpmsg_ctrldev_eptdev_auto_release(ctrldev_priv);
	devm_kfree(&ctrldev->dev, ctrldev_priv);
	put_device(&ctrldev->dev);

	return 0;
}

static long rpmsg_ctrldev_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	int ret;
	struct rpmsg_ctrldev_file_priv *ctrldev_priv = filp->private_data;
	struct rpmsg_ctrldev *ctrldev = ctrldev_priv->ctrldev;
	void __user *argp = (void __user *)arg;
	struct rpmsg_ept_info info;

	if (copy_from_user(&info, argp, sizeof(info)))
		return -EFAULT;

	switch (cmd) {
	case RPMSG_CREATE_AF_EPT_IOCTL:
	case RPMSG_CREATE_EPT_IOCTL: {
		if (!argp)
			return -EINVAL;
		/* get unique id for ept */
		ret = ida_simple_get(&rpmsg_ept_ida, 1, 0, GFP_KERNEL);
		if (ret < 0)
			return -EFAULT;

		info.id = ret;
		/* notify remote create endpoint */
		ret = rpmsg_ctrldev_create_eptdev(ctrldev, info.name, info.id);
		if (ret) {
			rpmsg_ctrldev_release_id(ctrldev->dev.id, info.id);
			return ret;
		}

		if (cmd == RPMSG_CREATE_AF_EPT_IOCTL) {
			ret = rpmsg_ctrldev_eptdev_bind(ctrldev_priv, &info);
			if (ret)
				return ret;
		}

		if (copy_to_user(argp, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	} break;

	case RPMSG_DESTROY_EPT_IOCTL: {
		if (!argp)
			return -EINVAL;

		ret = rpmsg_ctrldev_eptdev_unbind(ctrldev_priv, &info);
		if (ret)
			return ret;

		dev_info(&ctrldev->dev, "close /dev/rpmsg%d endpoint\r\n", info.id);
		ret = rpmsg_ctrldev_release_eptdev(ctrldev, info.id);
		if (ret)
			return ret;
		return 0;
	} break;

	case RPMSG_REST_EPT_GRP_IOCTL: {
		if (!argp)
			return -EINVAL;
		dev_info(&ctrldev->dev, "clear %s group\r\n", info.name);
		ret = rpmsg_ctrldev_clear_eptdev(ctrldev, info.name);
		if (ret)
			return ret;
		return 0;
	} break;
	case RPMSG_DESTROY_ALL_EPT_IOCTL: {
		dev_info(&ctrldev->dev, "reset %s\r\n", dev_name(&ctrldev->dev));
		ret = rpmsg_ctrldev_reset_eptdev(ctrldev);
		if (ret)
			return ret;
		return 0;
	} break;

	default:
		break;
	}

	dev_warn(&ctrldev->dev, "Undown konw cmd=0x%x\r\n", cmd);

	return -EINVAL;
};

static const struct file_operations rpmsg_ctrldev_fops = {
	.owner = THIS_MODULE,
	.open = rpmsg_ctrldev_open,
	.release = rpmsg_ctrldev_release,
	.unlocked_ioctl = rpmsg_ctrldev_ioctl,
	.compat_ioctl = rpmsg_ctrldev_ioctl,
};

static int rpmsg_ctrldev_cb(struct rpmsg_device *rpdev, void *data, int len,
						void *priv, u32 src)
{
	struct rpmsg_ctrldev *ctrldev = dev_get_drvdata(&rpdev->dev);
	struct rpmsg_ctrl_msg_ack *cell = data;
	struct rpmsg_ept_notify *notify = NULL;

	if (len != sizeof(*cell)) {
		dev_err(&rpdev->dev, "Invalid len:expect:%d,Rx:%d\n", (int)sizeof(*cell), len);
		return 0;
	}

	dev_dbg(&ctrldev->dev, "cb rpmsg%d ack=0x%x\n", cell->id, cell->ack);

	notify = rpmsg_ctrldev_notify_find(ctrldev, cell->id);
	if (!notify) {
		dev_err(&ctrldev->dev, "/dev/rpmsg%d dev not exits \n", cell->id);
		return -ENOENT;
	}

	notify->status = cell->ack;
	complete(&notify->complete);

	return 0;
}

static void rpmsg_ctrldev_release_device(struct device *dev)
{
	struct rpmsg_ctrldev *ctrldev = dev_to_ctrldev(dev);
	struct rpmsg_ept_notify *pos, *tmp;

	/* remove all sub rpmsg client */
	list_for_each_entry_safe(pos, tmp, &ctrldev->epts, list) {
		rpmsg_ctrldev_epts_del(ctrldev, pos);
	}

	ida_simple_remove(&rpmsg_ctrl_ida, dev->id);
	rpmsg_ctrldev_put_devt(dev->devt);
	kfree(ctrldev);
}

static ssize_t open_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret, id;
	struct rpmsg_ctrldev *ctrl = dev_get_drvdata(dev);
	char name[32];

	if (count == 0)
		return count;

	memcpy(name, buf, count > 32 ? 32:count);
	if (name[count - 1] == '\n')
		name[count - 1] = '\0';
	else
		name[count] = '\0';

	/* get unique id for ept */
	ret = ida_simple_get(&rpmsg_ept_ida, 1, 0, GFP_KERNEL);
	if (ret < 0)
		return -EFAULT;

	dev_dbg(&ctrl->dev, "create /dev/rpmsg%d endpoint\r\n", ret);
	id = ret;

	/* notify remote create endpoint */
	ret = rpmsg_ctrldev_create_eptdev(ctrl, name, ret);
	if (ret) {
		rpmsg_ctrldev_release_id(ctrl->dev.id, id);
		return ret;
	}


	return count;
}
static DEVICE_ATTR_WO(open);

static ssize_t close_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret;
	struct rpmsg_ctrldev *ctrl = dev_get_drvdata(dev);
	int id;

	if (count == 0)
		return count;

	ret = sscanf(buf, "%d", &id);;

	if (ret != 1)
		return -EINVAL;

	if (id < 0)
		return -EINVAL;

	dev_dbg(&ctrl->dev, "close /dev/rpmsg%d endpoint\r\n", id);

	/* notify remote close endpoint */
	ret = rpmsg_ctrldev_release_eptdev(ctrl, id);
	if (ret)
		return ret;

	return count;
}
static DEVICE_ATTR_WO(close);

static ssize_t clear_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret;
	struct rpmsg_ctrldev *ctrl = dev_get_drvdata(dev);
	char name[32];

	if (count == 0)
		return count;

	memcpy(name, buf, count > 32 ? 32:count);
	if (name[count - 1] == '\n')
		name[count - 1] = '\0';
	else
		name[count] = '\0';

	dev_dbg(&ctrl->dev, "clear %s endpoint\r\n", name);

	/* notify remote create endpoint */
	ret = rpmsg_ctrldev_clear_eptdev(ctrl, name);
	if (ret)
		return ret;

	return count;

}
static DEVICE_ATTR_WO(clear);

static ssize_t reset_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int ret;
	struct rpmsg_ctrldev *ctrl = dev_get_drvdata(dev);
	char name[32];

	if (count == 0)
		return count;

	memcpy(name, buf, count > 32 ? 32:count);
	if (name[count - 1] == '\n')
		name[count - 1] = '\0';
	else
		name[count] = '\0';

	dev_dbg(&ctrl->dev, "reset rpmsg_ctrl\r\n");

	/* notify remote create endpoint */
	ret = rpmsg_ctrldev_reset_eptdev(ctrl);
	if (ret)
		return ret;

	return count;

}
static DEVICE_ATTR_WO(reset);

static int rpmsg_ctrldev_probe(struct rpmsg_device *rpdev)
{
	struct rpmsg_ctrldev *ctrldev;
	struct device *dev, *rproc_dev;
	const char *rproc_name;
	int ret;

	ctrldev = kzalloc(sizeof(*ctrldev), GFP_KERNEL);
	if (!ctrldev)
		return -ENOMEM;

	ctrldev->rpdev = rpdev;
	atomic_long_set(&ctrldev->rpdev_refcnt, 1);
	sema_init(&ctrldev->sem, 0);

	dev = &ctrldev->dev;
	device_initialize(dev);
	dev->parent = &rpdev->dev;
#ifdef CONFIG_AW_RPMSG_CLASS
	dev->class = g_aw_rpmsg_class;
#else
	dev->class = rpmsg_class;
#endif
	INIT_LIST_HEAD(&ctrldev->epts);
	INIT_LIST_HEAD(&ctrldev->wait);
	mutex_init(&ctrldev->lock);

	cdev_init(&ctrldev->cdev, &rpmsg_ctrldev_fops);
	ctrldev->cdev.owner = THIS_MODULE;

	ret = rpmsg_ctrldev_get_devt();
	if (ret < 0)
		goto free_ctrldev;
	dev->devt = ret;

	ret = ida_simple_get(&rpmsg_ctrl_ida, 0, 0, GFP_KERNEL);
	if (ret < 0)
		goto free_minor_ida;

	/*
	 *	rpmsg block diagram.
	 *	rproc device				<--- define by user,usually in dts.
	 *	|--- remoteproc				<--- rproc_alloc()
	 *		 |--- rproc_vdev		<--- rproc_handle_vdev()
	 *			  |--- virtio		<--- register_virtio_device()
	 *				   |--- rpmsg	<--- rpmsg_register_device()
	 *
	 *	rpmsg_ctrl device name is consists of rpmsg_ctrl-rproc_name
	 */
	rproc_dev = rpdev->dev.parent->parent->parent->parent;
	if (!rproc_dev) {
		dev_err(dev, "Can't get rpdev grandparent.\n");
		goto free_ctrldev;
	}
	rproc_name = rproc_dev->of_node->full_name;

	dev->id = ret;
	dev_set_name(&ctrldev->dev, "rpmsg_ctrl-%s", rproc_name);

	ret = cdev_device_add(&ctrldev->cdev, &ctrldev->dev);
	if (ret) {
		dev_err(&rpdev->dev, "cdev_device_add failed: %d\n", ret);
		goto free_ctrl_ida;
	}

	/* We can now rely on the release function for cleanup */
	dev->release = rpmsg_ctrldev_release_device;


	device_create_file(&ctrldev->dev, &dev_attr_open);
	device_create_file(&ctrldev->dev, &dev_attr_close);
	device_create_file(&ctrldev->dev, &dev_attr_clear);
	device_create_file(&ctrldev->dev, &dev_attr_reset);

	dev_set_drvdata(&rpdev->dev, ctrldev);
	dev_set_drvdata(&ctrldev->dev, ctrldev);

	init_completion(&ctrldev->ctrl_notify.complete);
	rpmsg_ctrldev_notify_add(ctrldev, &ctrldev->ctrl_notify);

	/* wo need to announce the new ept to remote */
	rpdev->announce = true;

	return ret;

free_ctrl_ida:
	ida_simple_remove(&rpmsg_ctrl_ida, dev->id);
free_minor_ida:
	rpmsg_ctrldev_put_devt(dev->devt);
free_ctrldev:
	put_device(dev);
	kfree(ctrldev);

	return ret;
}

static void rpmsg_ctrldev_remove(struct rpmsg_device *rpdev)
{
	struct rpmsg_ctrldev *ctrldev = dev_get_drvdata(&rpdev->dev);
	int ret;

	dev_dbg(&rpdev->dev, "%s is removed\n", dev_name(&rpdev->dev));

	rpmsg_ctrldev_notify_del(ctrldev, &ctrldev->ctrl_notify);

	cdev_device_del(&ctrldev->cdev, &ctrldev->dev);

	// set rpdev = NULL when rpdev not in use
	rpmsg_ctrldev_put_rpdev(ctrldev);
	// wait rpdev = NULL
	while (1) {
		ret = down_interruptible(&ctrldev->sem);
		if (ret == 0)
			break;
		if (ret != -EINTR) {
			dev_err(&rpdev->dev, "wait sem return %d! rpdev: %p\n", ret, ctrldev->rpdev);
			break;
		}
	}
	put_device(&ctrldev->dev);
}

static struct rpmsg_device_id rpmsg_driver_ctrldev_id_table[] = {
	{ .name = "sunxi,rpmsg_ctrl" },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_driver_crtldev_id_table);

static struct rpmsg_driver rpmsg_ctrldev_driver = {
	.probe = rpmsg_ctrldev_probe,
	.remove = rpmsg_ctrldev_remove,
	.callback	= rpmsg_ctrldev_cb,
	.drv = {
		.name = "sunxi,rpmsg_master",
	},
	.id_table = rpmsg_driver_ctrldev_id_table,
};

static int rpmsg_ctrldev_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&rpmsg_major, 0, RPMSG_DEV_MAX, "aw_rpmsg");
	if (ret < 0) {
		pr_err("aw_rpmsg: failed to allocate char dev region\n");
		return ret;
	}

#ifdef CONFIG_AW_RPMSG_CLASS
	g_aw_rpmsg_class = class_create(THIS_MODULE, AW_RPMSG_CLASS_NAME);
	if (IS_ERR(g_aw_rpmsg_class)) {
		pr_err("failed to create aw rpmsg class\n");
		unregister_chrdev_region(rpmsg_major, RPMSG_DEV_MAX);
		return PTR_ERR(g_aw_rpmsg_class);
	}
#endif

	ret = register_rpmsg_driver(&rpmsg_ctrldev_driver);
	if (ret < 0) {
		pr_err("aw_rpmsg: failed to register rpmsg driver for rpmsg ctrl\n");
#ifdef CONFIG_AW_RPMSG_CLASS
		class_destroy(g_aw_rpmsg_class);
#endif
		unregister_chrdev_region(rpmsg_major, RPMSG_DEV_MAX);
	}

	return ret;
}
#if IS_ENABLED(CONFIG_AW_RPROC_FAST_BOOT)
fast_rpmsg_initcall(rpmsg_ctrldev_init);
#else
module_init(rpmsg_ctrldev_init);
#endif

static void rpmsg_ctrldev_exit(void)
{
	unregister_rpmsg_driver(&rpmsg_ctrldev_driver);
#ifdef CONFIG_AW_RPMSG_CLASS
	class_destroy(g_aw_rpmsg_class);
#endif
	unregister_chrdev_region(rpmsg_major, RPMSG_DEV_MAX);
}
module_exit(rpmsg_ctrldev_exit);

MODULE_ALIAS("rpmsg:rpmsg_ctrl");
MODULE_AUTHOR("lijiajian@allwinnertech.com");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPMSG_CTRL_VERSION);

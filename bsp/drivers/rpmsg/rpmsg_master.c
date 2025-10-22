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
#include <uapi/linux/rpmsg.h>

#include "rpmsg_internal.h"
#include "rpmsg_master.h"

#define SUNXI_RPMSG_CTRL_VERSION "1.0.0"

#define WAIT_TIMEOUT	(msecs_to_jiffies(500))

static dev_t rpmsg_major;
#if IS_ENABLED(CONFIG_AW_KERNEL_ORIGIN)
struct class *aw_rpmsg_class;
EXPORT_SYMBOL(aw_rpmsg_class);
#endif

static DEFINE_MUTEX(g_list_lock);
static LIST_HEAD(g_rpmsg_ctrldev_list);

/* record all rpmsg_ctrl device id */
static DEFINE_IDA(rpmsg_ctrl_ida);
/* record all rpmsg_client device id */
static DEFINE_IDA(rpmsg_ept_ida);
/* record all rpmsg_ctrl device devt */
static DEFINE_IDA(rpmsg_minor_ida);

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
	struct device dev;
	struct cdev cdev;
	struct list_head list;
	u32    alive:1;

	struct mutex lock;
	struct list_head epts;
	struct list_head wait;

	struct rpmsg_ept_notify ctrl_notify;
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

static struct rpmsg_ctrldev *rpmsg_ctrldev_get_ctrl(int id)
{
	struct rpmsg_ctrldev *pos, *tmp;

	mutex_lock(&g_list_lock);
	list_for_each_entry_safe(pos, tmp, &g_rpmsg_ctrldev_list, list) {
		if (id == pos->dev.id) {
			mutex_unlock(&g_list_lock);
			return pos;
		}
	}
	mutex_unlock(&g_list_lock);

	return NULL;
}

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

static void rpmsg_ctrldev_epts_del(struct rpmsg_ctrldev *ctrldev,
				struct rpmsg_ept_notify *notify)
{
	mutex_lock(&ctrldev->lock);
	if (!list_empty(&notify->list))
		list_del_init(&notify->list);
	mutex_unlock(&ctrldev->lock);
}

/*
 *	rpmsg_ctrldev_notify will call by rpmsg_client.c
 *	to free ept instance.
 */
int rpmsg_ctrldev_notify(int ctrl_id, int id)
{
	struct rpmsg_ctrldev *ctrldev;
	struct rpmsg_ept_notify *notify;

	ctrldev = rpmsg_ctrldev_get_ctrl(ctrl_id);
	if (!ctrldev) {
		pr_err("/dev/rpmsg_ctrl%d dev not exits \n", ctrl_id);
		return -ENOENT;
	}

	notify = rpmsg_ctrldev_epts_find(ctrldev, id);
	if (!notify) {
		dev_err(&ctrldev->dev, "/dev/rpmsg%d dev not exits \n", id);
		return -ENOENT;
	}

	rpmsg_ctrldev_epts_del(ctrldev, notify);

	devm_kfree(&notify->ctrl->dev, notify);

	ida_simple_remove(&rpmsg_ept_ida, id);

	return 0;
}
EXPORT_SYMBOL(rpmsg_ctrldev_notify);

/* tell remote to free (rpmsg%d, id) ept */
static int rpmsg_eptdev_release(struct rpmsg_ctrldev *ctrldev, int id)
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

	reinit_completion(&notify->complete);
	/* add to notify chain */
	rpmsg_ctrldev_notify_add(ctrldev, notify);

	strncpy(msg.name, notify->name, 32);
	/* tell remote close this endpoint */
	ret = rpmsg_send(ctrldev->rpdev->ept, &msg, sizeof(msg));
	if (ret)
		goto out;

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
	return ret;
}

static int rpmsg_eptdev_create(struct rpmsg_ctrldev *ctrldev, const char *name, int id)
{
	int ret;
	struct rpmsg_ctrl_msg msg;
	struct rpmsg_ept_notify *notify = NULL;

	notify = devm_kzalloc(&ctrldev->dev, sizeof(*notify), GFP_KERNEL);
	if (!notify)
		return -ENOMEM;

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

static int rpmsg_eptdev_clear(struct rpmsg_ctrldev *ctrldev, const char *name)
{
	int ret;
	struct rpmsg_ctrl_msg msg;
	struct rpmsg_ept_notify *notify;

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
	if (ret)
		goto out;

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
	return ret;
}

static int rpmsg_eptdev_reset(struct rpmsg_ctrldev *ctrldev)
{
	int ret;
	struct rpmsg_ctrl_msg msg;
	struct rpmsg_ept_notify *notify;

	notify = &ctrldev->ctrl_notify;

	msg.ctrl_id = ctrldev->dev.id;
	msg.id = 0;
	msg.cmd = RPMSG_RESET_ALL_CLIENT;

	reinit_completion(&notify->complete);
	/* add to notify chain */
	rpmsg_ctrldev_notify_add(ctrldev, notify);
	/* tell remote close this endpoint */
	ret = rpmsg_trysend(ctrldev->rpdev->ept, &msg, sizeof(msg));
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

static int rpmsg_ctrldev_open(struct inode *inode, struct file *filp)
{
	struct rpmsg_ctrldev *ctrldev = cdev_to_ctrldev(inode->i_cdev);

	get_device(&ctrldev->dev);
	filp->private_data = ctrldev;

	return 0;
}

static int rpmsg_ctrldev_release(struct inode *inode, struct file *filp)
{
	struct rpmsg_ctrldev *ctrldev = cdev_to_ctrldev(inode->i_cdev);

	put_device(&ctrldev->dev);

	return 0;
}

static long rpmsg_ctrldev_ioctl(struct file *fp, unsigned int cmd,
				unsigned long arg)
{
	int ret;
	struct rpmsg_ctrldev *ctrldev = fp->private_data;
	void __user *argp = (void __user *)arg;
	struct rpmsg_ept_info info;

	if (copy_from_user(&info, argp, sizeof(info)))
		return -EFAULT;

	switch (cmd) {
	case RPMSG_CREATE_EPT_IOCTL: {
		if (!argp)
			return -EINVAL;
		/* get unique id for ept */
		ret = ida_simple_get(&rpmsg_ept_ida, 1, 0, GFP_KERNEL);
		if (ret < 0)
			return -EFAULT;

		info.id = ret;
		/* notify remote create endpoint */
		ret = rpmsg_eptdev_create(ctrldev, info.name, info.id);
		if (ret)
			return ret;

		if (copy_to_user(argp, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	} break;

	case RPMSG_DESTROY_EPT_IOCTL: {
		if (!argp)
			return -EINVAL;
		dev_info(&ctrldev->dev, "close /dev/rpmsg%d endpoint\r\n", info.id);
		ret = rpmsg_eptdev_release(ctrldev, info.id);
		if (ret)
			return ret;
		return 0;
	} break;

	case RPMSG_REST_EPT_GRP_IOCTL: {
		if (!argp)
			return -EINVAL;
		dev_info(&ctrldev->dev, "clear %s group\r\n", info.name);
		ret = rpmsg_eptdev_clear(ctrldev, info.name);
		if (ret)
			return ret;
		return 0;
	} break;
	case RPMSG_DESTROY_ALL_EPT_IOCTL: {
		dev_info(&ctrldev->dev, "reset %s\r\n", dev_name(&ctrldev->dev));
		ret = rpmsg_eptdev_reset(ctrldev);
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
	ida_simple_remove(&rpmsg_ctrl_ida, dev->id);
	rpmsg_ctrldev_put_devt(MINOR(dev->devt));
}

static ssize_t open_store(struct device *dev, struct device_attribute *attr,
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

	/* get unique id for ept */
	ret = ida_simple_get(&rpmsg_ept_ida, 1, 0, GFP_KERNEL);
	if (ret < 0)
		return -EFAULT;

	dev_dbg(&ctrl->dev, "create /dev/rpmsg%d endpoint\r\n", ret);

	/* notify remote create endpoint */
	ret = rpmsg_eptdev_create(ctrl, name, ret);
	if (ret)
		return ret;

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
	ret = rpmsg_eptdev_release(ctrl, id);
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
	ret = rpmsg_eptdev_clear(ctrl, name);
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
	ret = rpmsg_eptdev_reset(ctrl);
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

	ctrldev = devm_kzalloc(&rpdev->dev, sizeof(*ctrldev), GFP_KERNEL);
	if (!ctrldev)
		return -ENOMEM;

	ctrldev->rpdev = rpdev;

	dev = &ctrldev->dev;
	device_initialize(dev);
	dev->parent = &rpdev->dev;
#if IS_ENABLED(CONFIG_AW_KERNEL_ORIGIN)
	dev->class = aw_rpmsg_class;
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

	ret = cdev_add(&ctrldev->cdev, dev->devt, 1);
	if (ret)
		goto free_ctrl_ida;

	/* We can now rely on the release function for cleanup */
	dev->release = rpmsg_ctrldev_release_device;

	ret = device_add(dev);
	if (ret) {
		dev_err(&rpdev->dev, "device_add failed: %d\n", ret);
		put_device(dev);
	}

	device_create_file(&ctrldev->dev, &dev_attr_open);
	device_create_file(&ctrldev->dev, &dev_attr_close);
	device_create_file(&ctrldev->dev, &dev_attr_clear);
	device_create_file(&ctrldev->dev, &dev_attr_reset);

	dev_set_drvdata(&rpdev->dev, ctrldev);
	dev_set_drvdata(&ctrldev->dev, ctrldev);

	list_add(&ctrldev->list, &g_rpmsg_ctrldev_list);

	init_completion(&ctrldev->ctrl_notify.complete);
	rpmsg_ctrldev_notify_add(ctrldev, &ctrldev->ctrl_notify);

	/* wo need to announce the new ept to remote */
	rpdev->announce = true;

	return ret;

free_ctrl_ida:
	ida_simple_remove(&rpmsg_ctrl_ida, dev->id);
free_minor_ida:
	rpmsg_ctrldev_put_devt(MINOR(dev->devt));
free_ctrldev:
	put_device(dev);
	devm_kfree(&rpdev->dev, ctrldev);

	return ret;
}

static void rpmsg_ctrldev_remove(struct rpmsg_device *rpdev)
{
	struct rpmsg_ctrldev *ctrldev = dev_get_drvdata(&rpdev->dev);
	struct rpmsg_ept_notify *pos, *tmp;

	dev_dbg(&rpdev->dev, "%s is removed\n", dev_name(&rpdev->dev));

	/* remove all sub rpmsg client */
	list_for_each_entry_safe(pos, tmp, &ctrldev->epts, list) {
		rpmsg_ctrldev_epts_del(ctrldev, pos);
		ida_simple_remove(&rpmsg_ept_ida, pos->id);
		devm_kfree(&ctrldev->dev, pos);
	}

	rpmsg_ctrldev_notify_del(ctrldev, &ctrldev->ctrl_notify);
	if (!list_empty(&ctrldev->list))
		list_del_init(&ctrldev->list);
	device_del(&ctrldev->dev);
	put_device(&ctrldev->dev);
	cdev_del(&ctrldev->cdev);
	devm_kfree(&rpdev->dev, ctrldev);
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

	ret = alloc_chrdev_region(&rpmsg_major, 0, RPMSG_DEV_MAX, "rpmsg");
	if (ret < 0) {
		pr_err("rpmsg: failed to allocate char dev region\n");
		return ret;
	}
#if IS_ENABLED(CONFIG_AW_KERNEL_ORIGIN)
	aw_rpmsg_class = class_create(THIS_MODULE, "aw_rpmsg");
	if (IS_ERR(aw_rpmsg_class)) {
		pr_err("failed to create aw_rpmsg class\n");
		return PTR_ERR(aw_rpmsg_class);
	}
#endif
	ret = register_rpmsg_driver(&rpmsg_ctrldev_driver);
	if (ret < 0) {
		pr_err("rpmsgchr: failed to register rpmsg driver\n");
		unregister_chrdev_region(rpmsg_major, RPMSG_DEV_MAX);
	}

	return ret;
}
module_init(rpmsg_ctrldev_init);

static void rpmsg_ctrldev_exit(void)
{
	unregister_rpmsg_driver(&rpmsg_ctrldev_driver);
	unregister_chrdev_region(rpmsg_major, RPMSG_DEV_MAX);
}
module_exit(rpmsg_ctrldev_exit);

MODULE_ALIAS("rpmsg:rpmsg_ctrl");
MODULE_AUTHOR("lijiajian@allwinnertech.com");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPMSG_CTRL_VERSION);

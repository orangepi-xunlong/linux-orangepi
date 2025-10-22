// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 *
 * (C) Copyright 2020-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Junyan Lin <junyanlin@allwinnertech.com>
 *
 * RPBuf devices.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */


#define pr_fmt(fmt)	"[RPBUF DEV](%s:%d) " fmt, __func__, __LINE__

#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/idr.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/compat.h>
#include <linux/mm.h>
#include <linux/atomic.h>
#include <linux/wait.h>
#include <uapi/linux/rpbuf.h>

#include "rpbuf_internal.h"

#define SUNXI_RPBUF_DEV_VERSION "1.1.2"

#define RPBUF_DEV_MAX	(MINORMASK + 1)

#define RPBUF_DEV_WAIT_TIMEOUT_MSEC	15000

static dev_t rpbuf_major;
static struct class *rpbuf_class;

static DEFINE_IDA(rpbuf_minor_ida);

#define dev_to_ctrl_dev(dev) container_of(dev, struct rpbuf_ctrl_dev, dev)
#define cdev_to_ctrl_dev(i_cdev) container_of(i_cdev, struct rpbuf_ctrl_dev, cdev)

#define dev_to_buf_dev(dev) container_of(dev, struct rpbuf_buf_dev, dev)
#define cdev_to_buf_dev(i_cdev) container_of(i_cdev, struct rpbuf_buf_dev, cdev)

#define RPBUF_CTRL_DEV_IOCTL_CMD_VALID(cmd) (_IOC_TYPE((cmd)) == RPBUF_CTRL_DEV_IOCTL_MAGIC)
#define RPBUF_BUF_DEV_IOCTL_CMD_VALID(cmd) (_IOC_TYPE((cmd)) == RPBUF_BUF_DEV_IOCTL_MAGIC)

struct rpbuf_ctrl_dev {
	struct rpbuf_controller *controller;

	struct cdev cdev;
	struct device dev;

	struct list_head buf_devs;
	struct mutex buf_devs_lock;

	struct list_head list;
};

struct rpbuf_buf_dev {
	struct rpbuf_buffer_info info;
	struct rpbuf_buffer *buffer;

	struct rpbuf_controller *controller;

	struct cdev cdev;
	struct device dev;

	atomic_t can_be_opened;

	struct completion available;

	int recv_data_offset;
	int recv_data_len;
	spinlock_t recv_lock;
	wait_queue_head_t recv_wq;

	struct list_head list;
	struct fasync_struct *async_queue;
};

static LIST_HEAD(__rpbuf_ctrl_devs);
static DEFINE_MUTEX(__rpbuf_ctrl_devs_lock);

static inline struct rpbuf_ctrl_dev *rpbuf_find_ctrl_dev_by_id(int id)
{
	struct rpbuf_ctrl_dev *ctrl_dev;

	list_for_each_entry(ctrl_dev, &__rpbuf_ctrl_devs, list) {
		if (ctrl_dev->dev.id == id)
			return ctrl_dev;
	}
	return NULL;
}

static inline struct rpbuf_buf_dev *rpbuf_find_buf_dev_by_name(
		struct list_head *buf_devs, const char *name)
{
	struct rpbuf_buf_dev *buf_dev;

	list_for_each_entry(buf_dev, buf_devs, list) {
		if (0 == strncmp(buf_dev->info.name, name, RPBUF_NAME_SIZE))
			return buf_dev;
	}
	return NULL;
}

static void rpbuf_buf_dev_buffer_avaiable_cb(struct rpbuf_buffer *buffer, void *priv)
{
	struct rpbuf_buf_dev *buf_dev = priv;

	complete(&buf_dev->available);
}

static int rpbuf_buf_dev_buffer_rx_cb(struct rpbuf_buffer *buffer,
				      void *data, int data_len, void *priv)
{
	struct rpbuf_buf_dev *buf_dev = priv;
	struct device *dev = &buf_dev->dev;
	unsigned long flags;
	int print_warn = 0;

	spin_lock_irqsave(&buf_dev->recv_lock, flags);
	if (buf_dev->recv_data_offset >= 0 || buf_dev->recv_data_len >= 0)
		print_warn = 1;

	buf_dev->recv_data_offset = data - buffer->va;
	buf_dev->recv_data_len = data_len;
	spin_unlock_irqrestore(&buf_dev->recv_lock, flags);

	if (print_warn)
		dev_warn(dev, "previous recv data not treated\n");

	wake_up_interruptible(&buf_dev->recv_wq);

	kill_fasync(&buf_dev->async_queue, SIGIO, POLLIN | POLLRDNORM);

	return 0;
}

static int rpbuf_buf_dev_buffer_destroyed_cb(struct rpbuf_buffer *buffer, void *priv)
{
	struct rpbuf_buf_dev *buf_dev = priv;
	struct device *dev = &buf_dev->dev;

	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);

	kill_fasync(&buf_dev->async_queue, SIGIO, POLLHUP | POLLRDNORM);

	return 0;
}

static const struct rpbuf_buffer_cbs rpbuf_buf_dev_buffer_cbs = {
	.available_cb = rpbuf_buf_dev_buffer_avaiable_cb,
	.rx_cb = rpbuf_buf_dev_buffer_rx_cb,
	.destroyed_cb = rpbuf_buf_dev_buffer_destroyed_cb,
};

static int rpbuf_buf_dev_open(struct inode *inode, struct file *file)
{
	struct rpbuf_buf_dev *buf_dev = cdev_to_buf_dev(inode->i_cdev);
	struct device *dev = &buf_dev->dev;
	struct rpbuf_buffer *buffer;
	int ret;

	nonseekable_open(inode, file);

	if (!atomic_dec_and_test(&buf_dev->can_be_opened)) {
		atomic_inc(&buf_dev->can_be_opened);
		dev_err(dev, "buffer %s already in use\n", buf_dev->info.name);
		ret = -EBUSY;
		goto err_out;
	}

	get_device(dev);

	reinit_completion(&buf_dev->available);

	buffer = rpbuf_alloc_buffer(buf_dev->controller,
				    buf_dev->info.name, buf_dev->info.len, NULL,
				    &rpbuf_buf_dev_buffer_cbs, (void *)buf_dev);
	if (!buffer) {
		dev_err(dev, "rpbuf_alloc_buffer for %s failed\n", buf_dev->info.name);
		ret = -ENOENT;
		goto err_put_device;
	}
	buffer->is_used_by_buf_dev = true;
	buf_dev->buffer = buffer;

	ret = wait_for_completion_killable_timeout(&buf_dev->available,
						   msecs_to_jiffies(RPBUF_DEV_WAIT_TIMEOUT_MSEC));

	if (ret == -ERESTARTSYS) {
		dev_info(dev, "(%s:%d) kill signal occurs\n", __func__, __LINE__);
		goto err_free_buffer;
	} else if (ret < 0) {
		dev_err(dev, "wait for buffer %s available error (ret: %d)\n",
			buf_dev->info.name, ret);
		ret = -EIO;
		goto err_free_buffer;
	} else if (ret == 0) {
		dev_err(dev, "wait for buffer %s available timeout\n", buf_dev->info.name);
		ret = -EIO;
		goto err_free_buffer;
	}

	file->private_data = buf_dev;

	dev_dbg(dev, "%s, %d", __func__, __LINE__);
	return 0;

err_free_buffer:
	buf_dev->buffer = NULL;
	rpbuf_free_buffer(buffer);
err_put_device:
	put_device(dev);
	atomic_inc(&buf_dev->can_be_opened);
err_out:
	return ret;
}

static int rpbuf_buf_dev_release(struct inode *inode, struct file *file)
{
	struct rpbuf_buf_dev *buf_dev = cdev_to_buf_dev(inode->i_cdev);
	struct device *dev = &buf_dev->dev;
	struct rpbuf_buffer *buffer;
	int ret;

	buffer = buf_dev->buffer;
	if (buffer) {
		ret = rpbuf_free_buffer(buffer);
		if (ret < 0) {
			dev_err(dev, "rpbuf_free_buffer for %s failed\n",
				buf_dev->info.name);
			goto err_out;
		}
		buf_dev->buffer = NULL;
	}

	put_device(&buf_dev->dev);

	atomic_inc(&buf_dev->can_be_opened);

	if (buf_dev->async_queue)
		fasync_helper(-1, file, 0, &buf_dev->async_queue);

	return 0;
err_out:
	return ret;
}

static int rpbuf_buf_dev_mmap_default(struct rpbuf_buf_dev *buf_dev, struct vm_area_struct *vma)
{
	struct rpbuf_buffer *buffer = buf_dev->buffer;
	struct device *dev = &buf_dev->dev;
	int ret;

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	ret = remap_pfn_range(vma, vma->vm_start,
			      (unsigned long)(buffer->pa) >> PAGE_SHIFT,
			      vma->vm_end - vma->vm_start, vma->vm_page_prot);
	if (ret < 0) {
		dev_err(dev, "remap_pfn_rag failed (ret: %d)\n", ret);
		goto err_out;
	}

	return 0;

err_out:
	return ret;
}

static int rpbuf_buf_dev_mmap(struct file *file, struct vm_area_struct *vma)
{
	struct rpbuf_buf_dev *buf_dev = file->private_data;
	struct rpbuf_buffer *buffer = buf_dev->buffer;
	struct rpbuf_controller *controller = buffer->controller;
	struct rpbuf_link *link = controller->link;
	int ret;

	if (buffer->ops && buffer->ops->buf_dev_mmap)
		return buffer->ops->buf_dev_mmap(buffer, buffer->priv, vma);

	mutex_lock(&link->controller_lock);
	if (controller->ops && controller->ops->buf_dev_mmap) {
		ret = controller->ops->buf_dev_mmap(controller, buffer,
						    controller->priv, vma);
		mutex_unlock(&link->controller_lock);
		return ret;
	}
	mutex_unlock(&link->controller_lock);

	return rpbuf_buf_dev_mmap_default(buf_dev, vma);
}

static int rpbuf_buf_dev_fasync(int fd, struct file *file, int mode)
{
	struct rpbuf_buf_dev *buf_dev = file->private_data;

	return fasync_helper(fd, file, mode, &buf_dev->async_queue);
}

static unsigned int rpbuf_buf_dev_poll(struct file *file, struct poll_table_struct *wait)
{
	struct rpbuf_buf_dev *buf_dev = file->private_data;
	unsigned int mask = 0;

	poll_wait(file, &buf_dev->recv_wq, wait);

	if (buf_dev->recv_data_offset > 0 && buf_dev->recv_data_len > 0)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int rpbuf_buf_dev_ioctl_check_avail(struct rpbuf_buf_dev *buf_dev, void __user *argp)
{
	u8 is_available = rpbuf_buffer_is_available(buf_dev->buffer);
	put_user(is_available, (u8 __user *)argp);

	return 0;
}

static int rpbuf_buf_dev_ioctl_transmit_buf(struct rpbuf_buf_dev *buf_dev, void __user *argp)
{
	struct device *dev = &buf_dev->dev;
	struct rpbuf_buffer_xfer __user *buf_xfer_user = argp;
	struct rpbuf_buffer_xfer buf_xfer;
	struct rpbuf_buffer *buffer = buf_dev->buffer;
	int ret;

	if (copy_from_user(&buf_xfer, buf_xfer_user, sizeof(struct rpbuf_buffer_xfer))) {
		dev_err(dev, "copy_from_user rpbuf_buffer_xfer failed\n");
		ret = -EIO;
		goto err_out;
	}

	ret = rpbuf_transmit_buffer(buffer, buf_xfer.offset, buf_xfer.data_len);
	if (ret < 0) {
		dev_err(dev, "transmit to remote failed\n");
		goto err_out;
	}

	return 0;

err_out:
	return ret;
}

static int rpbuf_buf_dev_ioctl_receive_buf(struct rpbuf_buf_dev *buf_dev,
					   void __user *argp)
{
	struct device *dev = &buf_dev->dev;
	struct rpbuf_buffer_xfer __user *buf_xfer_user = argp;
	struct rpbuf_buffer_xfer buf_xfer;
	unsigned long flags;
	int timeout_ms;
	long timeout_jiffies;
	long remain;
	int ret;

	if (copy_from_user(&buf_xfer, buf_xfer_user, sizeof(struct rpbuf_buffer_xfer))) {
		dev_err(dev, "copy_from_user rpbuf_buffer_xfer failed\n");
		ret = -EIO;
		goto err_out;
	}
	timeout_ms = buf_xfer.timeout_ms;

	spin_lock_irqsave(&buf_dev->recv_lock, flags);

	/* Wait for new received data */
	while ((buf_dev->recv_data_offset < 0 || buf_dev->recv_data_len < 0)
			&& timeout_ms != 0) {
		spin_unlock_irqrestore(&buf_dev->recv_lock, flags);

		if (timeout_ms < 0) {
			ret = wait_event_interruptible(buf_dev->recv_wq,
						       (buf_dev->recv_data_offset >= 0
							&& buf_dev->recv_data_len >= 0));
			if (ret) {
				dev_info(dev, "(%s:%d) signal occurs\n", __func__, __LINE__);
				ret = -ERESTARTSYS;
				goto err_out;
			}
		} else {
			timeout_jiffies = msecs_to_jiffies(timeout_ms);
			remain = wait_event_interruptible_timeout(
					buf_dev->recv_wq,
					(buf_dev->recv_data_offset >= 0
					 && buf_dev->recv_data_len >= 0),
					timeout_jiffies);
			if (remain < 0) {
				dev_info(dev, "(%s:%d) signal occurs (remain: %ld)\n",
					 __func__, __LINE__, remain);
				ret = remain;
				goto err_out;
			} else if (remain == 0) {
				dev_dbg(dev, "(%s:%d) %dms elapsed\n",
					__func__, __LINE__, timeout_ms);
				timeout_ms = -ETIMEDOUT;
				spin_lock_irqsave(&buf_dev->recv_lock, flags);
				break;
			} else {
				timeout_ms = jiffies_to_msecs(remain);
			}
		}

		spin_lock_irqsave(&buf_dev->recv_lock, flags);
	}

	if (buf_dev->recv_data_offset >= 0 && buf_dev->recv_data_len >= 0) {
		buf_xfer.offset = buf_dev->recv_data_offset;
		buf_xfer.data_len = buf_dev->recv_data_len;
	}
	buf_xfer.offset = buf_dev->recv_data_offset;
	buf_xfer.data_len = buf_dev->recv_data_len;
	buf_xfer.timeout_ms = timeout_ms;

	buf_dev->recv_data_offset = -1;
	buf_dev->recv_data_len = -1;

	spin_unlock_irqrestore(&buf_dev->recv_lock, flags);

	if (copy_to_user(buf_xfer_user, &buf_xfer, sizeof(struct rpbuf_buffer_xfer))) {
		dev_err(dev, "copy_to_user rpbuf_buffer_xfer failed\n");
		ret = -EIO;
		goto err_out;
	}

	ret = rpbuf_notify_remotebuffer_complete(buf_dev->controller, buf_dev->buffer);
	if (ret < 0) {
		dev_warn(buf_dev->controller->dev, "buffer \"%s\" (id:%d) notify remote failed\n",
					rpbuf_buffer_name(buf_dev->buffer), rpbuf_buffer_id(buf_dev->buffer));
		ret = -EFAULT;
		goto err_out;
	}

	return 0;

err_out:
	return ret;
}

static int rpbuf_buf_dev_ioctl_set_sync_buf(struct rpbuf_buf_dev *buf_dev,
					   void __user *argp)
{
	struct device *dev = &buf_dev->dev;
	uint32_t __user *sync_user = argp;
	uint32_t is_sync;
	int ret = 0;

	if (copy_from_user(&is_sync, sync_user, sizeof(uint32_t))) {
		dev_err(dev, "copy_from_user uint32_t failed\n");
		ret = -EIO;
		goto err_out;
	}

	if (is_sync)
		ret = rpbuf_buffer_set_sync(buf_dev->buffer, true);
	else
		ret = rpbuf_buffer_set_sync(buf_dev->buffer, false);

err_out:
	return ret;
}

static long rpbuf_buf_dev_ioctl(struct file *file, unsigned int cmd,
				void __user *argp)
{
	struct rpbuf_buf_dev *buf_dev = file->private_data;
	struct device *dev = &buf_dev->dev;
	int ret;

	if (!RPBUF_BUF_DEV_IOCTL_CMD_VALID(cmd))
		return -EINVAL;

	switch (cmd) {
	case RPBUF_BUF_DEV_IOCTL_CHECK_AVAIL:
		ret = rpbuf_buf_dev_ioctl_check_avail(buf_dev, argp);
		break;
	case RPBUF_BUF_DEV_IOCTL_TRANSMIT_BUF:
		ret = rpbuf_buf_dev_ioctl_transmit_buf(buf_dev, argp);
		break;
	case RPBUF_BUF_DEV_IOCTL_RECEIVE_BUF:
		ret = rpbuf_buf_dev_ioctl_receive_buf(buf_dev, argp);
		break;
	case RPBUF_BUF_DEV_IOCTL_SET_SYNC_BUF:
		ret = rpbuf_buf_dev_ioctl_set_sync_buf(buf_dev, argp);
		break;
	default:
		dev_err(dev, "invalid rpbuf buf_dev ioctl cmd: 0x%x\n", cmd);
		return -EINVAL;
	}

	return ret;
}

static long rpbuf_buf_dev_unlocked_ioctl(struct file *file, unsigned int cmd,
					 unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	return rpbuf_buf_dev_ioctl(file, cmd, argp);
}

#ifdef CONFIG_COMPAT
static long rpbuf_buf_dev_compat_ioctl(struct file *file, unsigned int cmd,
				       unsigned long arg)
{
	void __user *argp = compat_ptr(arg);

	return rpbuf_buf_dev_ioctl(file, cmd, argp);
}
#endif /* CONFIG_COMPAT */

static const struct file_operations rpbuf_buf_dev_fops = {
	.owner = THIS_MODULE,
	.open = rpbuf_buf_dev_open,
	.release = rpbuf_buf_dev_release,
	.llseek = no_llseek,
	.mmap = rpbuf_buf_dev_mmap,
	.fasync = rpbuf_buf_dev_fasync,
	.poll = rpbuf_buf_dev_poll,
	.unlocked_ioctl = rpbuf_buf_dev_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = rpbuf_buf_dev_compat_ioctl,
#endif
};

static void rpbuf_buf_dev_device_release(struct device *dev)
{
	struct rpbuf_buf_dev *buf_dev = dev_to_buf_dev(dev);

	dev_dbg(dev->parent, "%s:%d\n", __func__, __LINE__);

	ida_simple_remove(&rpbuf_minor_ida, MINOR(dev->devt));
	cdev_del(&buf_dev->cdev);
	devm_kfree(dev->parent, buf_dev);
}

static int rpbuf_ctrl_dev_ioctl_create_buf(struct rpbuf_ctrl_dev *ctrl_dev,
					   struct rpbuf_buf_dev **buf_dev_created,
					   void __user *argp)
{
	struct device *dev = &ctrl_dev->dev;
	struct rpbuf_buffer_info __user *buf_info_user = argp;
	struct rpbuf_buffer_info buf_info;
	struct rpbuf_buf_dev *buf_dev;
	int ret;

	if (copy_from_user(&buf_info, buf_info_user, sizeof(struct rpbuf_buffer_info))) {
		dev_err(dev, "copy_from_user rpbuf_buffer_info failed\n");
		ret = -EIO;
		goto err_out;
	}

	mutex_lock(&ctrl_dev->buf_devs_lock);
	buf_dev = rpbuf_find_buf_dev_by_name(&ctrl_dev->buf_devs, buf_info.name);
	if (buf_dev) {
		dev_err(dev, "buf_dev named %s already exists\n", buf_info.name);
		mutex_unlock(&ctrl_dev->buf_devs_lock);
		ret = -EEXIST;
		goto err_out;
	}

	buf_dev = devm_kzalloc(dev, sizeof(struct rpbuf_buf_dev), GFP_KERNEL);
	if (!buf_dev) {
		dev_err(dev, "kzalloc for rpbuf_buf_dev failed\n");
		mutex_unlock(&ctrl_dev->buf_devs_lock);
		ret = -ENOMEM;
		goto err_out;
	}
	memcpy(&buf_dev->info, &buf_info, sizeof(struct rpbuf_buffer_info));
	buf_dev->controller = ctrl_dev->controller;
	atomic_set(&buf_dev->can_be_opened, 1);
	init_completion(&buf_dev->available);
	spin_lock_init(&buf_dev->recv_lock);
	init_waitqueue_head(&buf_dev->recv_wq);
	buf_dev->recv_data_offset = -1;
	buf_dev->recv_data_len = -1;

	device_initialize(&buf_dev->dev);
	buf_dev->dev.parent = dev;
	buf_dev->dev.class = rpbuf_class;

	cdev_init(&buf_dev->cdev, &rpbuf_buf_dev_fops);
	buf_dev->cdev.owner = THIS_MODULE;

	ret = ida_simple_get(&rpbuf_minor_ida, 0, RPBUF_DEV_MAX, GFP_KERNEL);
	if (ret < 0) {
		dev_err(dev, "ida_simple_get for rpbuf minor failed\n");
		mutex_unlock(&ctrl_dev->buf_devs_lock);
		goto err_put_device;
	}
	buf_dev->dev.devt = MKDEV(MAJOR(rpbuf_major), ret);

	dev_set_name(&buf_dev->dev, "rpbuf-%s", buf_dev->info.name);

	ret = cdev_add(&buf_dev->cdev, buf_dev->dev.devt, 1);
	if (ret < 0) {
		dev_err(dev, "cdev_add for rpbuf buf_dev failed\n");
		mutex_unlock(&ctrl_dev->buf_devs_lock);
		goto err_remove_minor_ida;
	}

	/* We can now rely on the release function for cleanup */
	buf_dev->dev.release = rpbuf_buf_dev_device_release;

	ret = device_add(&buf_dev->dev);
	if (ret < 0) {
		dev_err(dev, "device_add for rpbuf buf_dev failed\n");
		mutex_unlock(&ctrl_dev->buf_devs_lock);
		put_device(&buf_dev->dev);
		goto err_out;
	}

	list_add_tail(&buf_dev->list, &ctrl_dev->buf_devs);
	*buf_dev_created = buf_dev;

	mutex_unlock(&ctrl_dev->buf_devs_lock);

	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);
	return 0;

err_remove_minor_ida:
	ida_simple_remove(&rpbuf_minor_ida, MINOR(buf_dev->dev.devt));
err_put_device:
	put_device(&buf_dev->dev);
	devm_kfree(dev, buf_dev);
err_out:
	return ret;
}

static int rpbuf_ctrl_dev_ioctl_destroy_buf(struct rpbuf_ctrl_dev *ctrl_dev,
					    void __user *argp)
{
	struct device *dev = &ctrl_dev->dev;
	struct rpbuf_buffer_info __user *buf_info_user = argp;
	struct rpbuf_buffer_info buf_info;
	struct rpbuf_buf_dev *buf_dev;
	int ret;

	if (copy_from_user(&buf_info, buf_info_user, sizeof(struct rpbuf_buffer_info))) {
		dev_err(dev, "copy_from_user rpbuf_buffer_info failed\n");
		ret = -EIO;
		goto err_out;
	}

	mutex_lock(&ctrl_dev->buf_devs_lock);
	buf_dev = rpbuf_find_buf_dev_by_name(&ctrl_dev->buf_devs, buf_info.name);
	if (!buf_dev) {
		dev_err(dev, "buf_dev named %s not found\n", buf_info.name);
		mutex_unlock(&ctrl_dev->buf_devs_lock);
		ret = -ENOENT;
		goto err_out;
	}
	list_del(&buf_dev->list);
	mutex_unlock(&ctrl_dev->buf_devs_lock);

	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);

	device_del(&buf_dev->dev);
	put_device(&buf_dev->dev);

	return 0;
err_out:
	return ret;
}

static long rpbuf_ctrl_dev_ioctl(struct file *file, unsigned int cmd,
				 void __user *argp)
{
	struct rpbuf_ctrl_dev *ctrl_dev = cdev_to_ctrl_dev(file->f_inode->i_cdev);
	struct rpbuf_buf_dev *buf_dev;
	struct device *dev = &ctrl_dev->dev;
	int ret;

	if (!RPBUF_CTRL_DEV_IOCTL_CMD_VALID(cmd))
		return -EINVAL;

	switch (cmd) {
	case RPBUF_CTRL_DEV_IOCTL_CREATE_BUF:
		ret = rpbuf_ctrl_dev_ioctl_create_buf(ctrl_dev, &buf_dev, argp);
		if (ret < 0)
			return ret;
		file->private_data = buf_dev;
		break;
	case RPBUF_CTRL_DEV_IOCTL_DESTROY_BUF:
		ret = rpbuf_ctrl_dev_ioctl_destroy_buf(ctrl_dev, argp);
		if (ret < 0)
			return ret;
		file->private_data = NULL;
		break;
	default:
		dev_err(dev, "invalid rpbuf ctrl_dev ioctl cmd: 0x%x\n", cmd);
		return -EINVAL;
	}

	return ret;
}

static long rpbuf_ctrl_dev_unlocked_ioctl(struct file *file, unsigned int cmd,
					  unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	return rpbuf_ctrl_dev_ioctl(file, cmd, argp);
}

#ifdef CONFIG_COMPAT
static long rpbuf_ctrl_dev_compat_ioctl(struct file *file, unsigned int cmd,
					unsigned long arg)
{
	void __user *argp = compat_ptr(arg);

	return rpbuf_ctrl_dev_ioctl(file, cmd, argp);
}
#endif /* CONFIG_COMPAT */

static int rpbuf_ctrl_dev_open(struct inode *inode, struct file *file)
{
	struct rpbuf_ctrl_dev *ctrl_dev = cdev_to_ctrl_dev(inode->i_cdev);

	nonseekable_open(inode, file);

	get_device(&ctrl_dev->dev);

	return 0;
}

static int rpbuf_ctrl_dev_release(struct inode *inode, struct file *file)
{
	struct rpbuf_ctrl_dev *ctrl_dev = cdev_to_ctrl_dev(inode->i_cdev);
	struct rpbuf_buf_dev *buf_dev = file->private_data;

	if (buf_dev) {
		/*
		 * User space program forgot to call ioctl(RPBUF_CTRL_DEV_IOCTL_DESTROY_BUF)
		 * to destroy buf_dev. Now we destroy it manually.
		 */
		mutex_lock(&ctrl_dev->buf_devs_lock);
		list_del(&buf_dev->list);
		mutex_unlock(&ctrl_dev->buf_devs_lock);

		device_del(&buf_dev->dev);
		put_device(&buf_dev->dev);

		file->private_data = NULL;
	}

	put_device(&ctrl_dev->dev);

	return 0;
}

static const struct file_operations rpbuf_ctrl_dev_fops = {
	.owner = THIS_MODULE,
	.open = rpbuf_ctrl_dev_open,
	.release = rpbuf_ctrl_dev_release,
	.unlocked_ioctl = rpbuf_ctrl_dev_unlocked_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = rpbuf_ctrl_dev_compat_ioctl,
#endif
	.llseek = no_llseek,
};

static void rpbuf_ctrl_dev_device_release(struct device *dev)
{
	struct rpbuf_ctrl_dev *ctrl_dev = dev_to_ctrl_dev(dev);

	dev_dbg(dev->parent, "%s:%d\n", __func__, __LINE__);

	mutex_destroy(&ctrl_dev->buf_devs_lock);
	ida_simple_remove(&rpbuf_minor_ida, MINOR(dev->devt));
	cdev_del(&ctrl_dev->cdev);
	devm_kfree(dev->parent, ctrl_dev);
}

int rpbuf_register_ctrl_dev(struct device *dev, int id, struct rpbuf_controller *controller)
{
	struct rpbuf_ctrl_dev *ctrl_dev;
	int ret;

	if (IS_ERR_OR_NULL(dev) || IS_ERR_OR_NULL(controller)) {
		pr_err("invalid arguments\n");
		ret = -EINVAL;
		goto err_out;
	}

	mutex_lock(&__rpbuf_ctrl_devs_lock);
	ctrl_dev = rpbuf_find_ctrl_dev_by_id(id);
	if (ctrl_dev) {
		dev_err(dev, "ctrl_dev with id %d already exists\n", id);
		mutex_unlock(&__rpbuf_ctrl_devs_lock);
		ret = -EEXIST;
		goto err_out;
	}

	ctrl_dev = devm_kzalloc(dev, sizeof(struct rpbuf_ctrl_dev), GFP_KERNEL);
	if (!ctrl_dev) {
		dev_err(dev, "kzalloc for rpbuf_ctrl_dev failed\n");
		mutex_unlock(&__rpbuf_ctrl_devs_lock);
		ret = -ENOMEM;
		goto err_out;
	}

	ctrl_dev->controller = controller;

	device_initialize(&ctrl_dev->dev);
	ctrl_dev->dev.parent = dev;
	ctrl_dev->dev.class = rpbuf_class;

	cdev_init(&ctrl_dev->cdev, &rpbuf_ctrl_dev_fops);
	ctrl_dev->cdev.owner = THIS_MODULE;

	ret = ida_simple_get(&rpbuf_minor_ida, 0, RPBUF_DEV_MAX, GFP_KERNEL);
	if (ret < 0) {
		dev_err(dev, "ida_simple_get for rpbuf minor failed\n");
		mutex_unlock(&__rpbuf_ctrl_devs_lock);
		goto err_free_ctrl_dev;
	}
	ctrl_dev->dev.devt = MKDEV(MAJOR(rpbuf_major), ret);

	ctrl_dev->dev.id = id;
	dev_set_name(&ctrl_dev->dev, "rpbuf_ctrl%d", id);

	ret = cdev_add(&ctrl_dev->cdev, ctrl_dev->dev.devt, 1);
	if (ret < 0) {
		dev_err(dev, "cdev_add for rpbuf ctrl_dev failed\n");
		mutex_unlock(&__rpbuf_ctrl_devs_lock);
		goto err_remove_minor_ida;
	}

	INIT_LIST_HEAD(&ctrl_dev->buf_devs);
	mutex_init(&ctrl_dev->buf_devs_lock);

	/* We can now rely on the release function for cleanup */
	ctrl_dev->dev.release = rpbuf_ctrl_dev_device_release;

	ret = device_add(&ctrl_dev->dev);
	if (ret < 0) {
		dev_err(dev, "device_add for rpbuf ctrl_dev failed\n");
		mutex_unlock(&__rpbuf_ctrl_devs_lock);
		put_device(&ctrl_dev->dev);
		goto err_out;
	}

	list_add_tail(&ctrl_dev->list, &__rpbuf_ctrl_devs);

	mutex_unlock(&__rpbuf_ctrl_devs_lock);

	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);
	return 0;

err_remove_minor_ida:
	ida_simple_remove(&rpbuf_minor_ida, MINOR(ctrl_dev->dev.devt));
err_free_ctrl_dev:
	put_device(&ctrl_dev->dev);
	devm_kfree(dev, ctrl_dev);
err_out:
	return ret;
}
EXPORT_SYMBOL(rpbuf_register_ctrl_dev);

int rpbuf_unregister_ctrl_dev(struct device *dev, int id)
{
	struct rpbuf_ctrl_dev *ctrl_dev;
	struct rpbuf_buf_dev *buf_dev;
	struct rpbuf_buf_dev *tmp;
	int ret;

	if (IS_ERR_OR_NULL(dev)) {
		pr_err("invalid arguments\n");
		ret = -EINVAL;
		goto err_out;
	}

	mutex_lock(&__rpbuf_ctrl_devs_lock);
	ctrl_dev = rpbuf_find_ctrl_dev_by_id(id);
	if (!ctrl_dev) {
		dev_err(dev, "ctrl_dev with id %d not found\n", id);
		mutex_unlock(&__rpbuf_ctrl_devs_lock);
		ret = -ENOENT;
		goto err_out;
	}
	list_del(&ctrl_dev->list);
	mutex_unlock(&__rpbuf_ctrl_devs_lock);

	dev_dbg(dev, "%s:%d\n", __func__, __LINE__);

	list_for_each_entry_safe(buf_dev, tmp, &ctrl_dev->buf_devs, list) {
		list_del(&buf_dev->list);
		device_del(&buf_dev->dev);
		put_device(&buf_dev->dev);
	}

	device_del(&ctrl_dev->dev);
	put_device(&ctrl_dev->dev);

	return 0;

err_out:
	return ret;
}
EXPORT_SYMBOL(rpbuf_unregister_ctrl_dev);

static int rpbuf_dev_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&rpbuf_major, 0, RPBUF_DEV_MAX, "rpbuf");
	if (ret < 0) {
		pr_err("rpbuf: failed to allocate char dev region\n");
		goto err_out;
	}

	rpbuf_class = class_create(THIS_MODULE, "rpbuf");
	if (IS_ERR(rpbuf_class)) {
		pr_err("failed to create rpbuf class\n");
		ret = PTR_ERR(rpbuf_class);
		goto err_unregister_chrdev_region;
	}

	return 0;

err_unregister_chrdev_region:
	unregister_chrdev_region(rpbuf_major, RPBUF_DEV_MAX);
err_out:
	return ret;
}
postcore_initcall(rpbuf_dev_init);

static void rpbuf_dev_exit(void)
{
	class_destroy(rpbuf_class);
	unregister_chrdev_region(rpbuf_major, RPBUF_DEV_MAX);
}
module_exit(rpbuf_dev_exit);

MODULE_DESCRIPTION("RPBuf devices");
MODULE_AUTHOR("Junyan Lin <junyanlin@allwinnertech.com>");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(SUNXI_RPBUF_DEV_VERSION);

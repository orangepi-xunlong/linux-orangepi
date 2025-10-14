// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 Huawei Technologies Co., Ltd. All rights reserved.
 */

#define pr_fmt(fmt) "hievent_driver " fmt

#include "hievent_driver.h"

#include <linux/device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/version.h>
#include <linux/uio.h>
#include <linux/list.h>
#include <linux/wait.h>

static struct class *hievent_class;
static dev_t hievent_devno;

#define HIEVENT_BUFFER ((size_t)CONFIG_BBOX_BUFFER_SIZE)
#define HIEVENT_DRIVER "/dev/bbox"
#define HIEVENT_DEV_NAME "bbox"
#define HIEVENT_DEV_NR 1

struct hievent_entry {
	unsigned short len;
	unsigned short header_size;
	char msg[0];
};

struct hievent_char_device {
	struct cdev devm;
	int flag;
	struct mutex mtx; /* lock to protect read/write buffer */
	unsigned char *buffer;
	wait_queue_head_t wq;
	size_t write_offset;
	size_t head_offset;
	size_t size;
	size_t count;
} hievent_dev;

static inline unsigned char *hievent_buffer_head(void)
{
	if (hievent_dev.head_offset > HIEVENT_BUFFER)
		hievent_dev.head_offset =
			hievent_dev.head_offset % HIEVENT_BUFFER;

	return hievent_dev.buffer + hievent_dev.head_offset;
}

static void hievent_buffer_inc(size_t sz)
{
	if (hievent_dev.size + sz <= HIEVENT_BUFFER) {
		hievent_dev.size += sz;
		hievent_dev.write_offset += sz;
		hievent_dev.write_offset %= HIEVENT_BUFFER;
		hievent_dev.count++;
	}
}

static void hievent_buffer_dec(size_t sz)
{
	if (hievent_dev.size >= sz) {
		hievent_dev.size -= sz;
		hievent_dev.head_offset += sz;
		hievent_dev.head_offset %= HIEVENT_BUFFER;
		hievent_dev.count--;
	}
}

static int hievent_read_ring_buffer(unsigned char __user *buffer,
				    size_t buf_len)
{
	size_t retval;
	size_t buf_left = HIEVENT_BUFFER - hievent_dev.head_offset;

	if (buf_left > buf_len) {
		retval = copy_to_user(buffer, hievent_buffer_head(), buf_len);
	} else {
		size_t mem_len = (buf_len > buf_left) ? buf_left : buf_len;

		retval = copy_to_user(buffer, hievent_buffer_head(), mem_len);
		if ((ssize_t)retval < 0)
			return (int)retval;

		retval = copy_to_user(buffer + buf_left, hievent_dev.buffer,
				      buf_len - buf_left);
	}
	return (int)retval;
}

static int hievent_read_ring_head_buffer(unsigned char *const buffer,
					 size_t buf_len)
{
	size_t buf_left = HIEVENT_BUFFER - hievent_dev.head_offset;

	if (buf_left > buf_len) {
		memcpy(buffer, hievent_buffer_head(), buf_len);
	} else {
		size_t mem_len = (buf_len > buf_left) ? buf_left : buf_len;

		memcpy(buffer, hievent_buffer_head(), mem_len);
		memcpy(buffer + buf_left, hievent_dev.buffer,
		       buf_len - buf_left);
	}
	return 0;
}

static ssize_t hievent_read(struct file *file, char __user *user_buf,
			    size_t count, loff_t *ppos)
{
	size_t retval;
	struct hievent_entry header;

	(void)file;

	if (file->f_flags & O_NONBLOCK && hievent_dev.size == 0)
		return -EAGAIN;

	if (wait_event_interruptible(hievent_dev.wq, (hievent_dev.size > 0)))
		return -EINVAL;

	(void)mutex_lock(&hievent_dev.mtx);

	if (hievent_dev.size == 0) {
		retval = 0;
		goto out;
	}

	retval = hievent_read_ring_head_buffer((unsigned char *)&header,
					       sizeof(header));
	if ((int)retval < 0) {
		retval = -EINVAL;
		goto out;
	}

	if (count < header.len + sizeof(header)) {
		retval = -ENOMEM;
		goto out;
	}

	hievent_buffer_dec(sizeof(header));

	retval = hievent_read_ring_buffer((unsigned char __user *)(user_buf),
					  header.len);
	if ((int)retval < 0) {
		retval = -EINVAL;
		goto out;
	}
	hievent_buffer_dec(header.len);

	retval = header.len;
out:
	if (retval == -ENOMEM) {
		// clean ring buffer
		hievent_dev.write_offset = 0;
		hievent_dev.head_offset = 0;
		hievent_dev.size = 0;
		hievent_dev.count = 0;
	}
	(void)mutex_unlock(&hievent_dev.mtx);

	return (ssize_t)retval;
}

static int hievent_write_ring_head_buffer(const unsigned char *buffer,
					  size_t buf_len)
{
	size_t buf_left = HIEVENT_BUFFER - hievent_dev.write_offset;

	if (buf_len > buf_left) {
		memcpy(hievent_dev.buffer + hievent_dev.write_offset, buffer,
		       buf_left);
		memcpy(hievent_dev.buffer, buffer + buf_left,
		       min(HIEVENT_BUFFER, buf_len - buf_left));
	} else {
		memcpy(hievent_dev.buffer + hievent_dev.write_offset, buffer,
		       min(buf_left, buf_len));
	}

	return 0;
}

static void hievent_head_init(struct hievent_entry *const header, size_t len)
{
	header->len = (unsigned short)len;
	header->header_size = sizeof(struct hievent_entry);
}

static void hievent_cover_old_log(size_t buf_len)
{
	int retval;
	struct hievent_entry header;
	size_t total_size = buf_len + sizeof(struct hievent_entry);

	while (total_size + hievent_dev.size > HIEVENT_BUFFER) {
		retval = hievent_read_ring_head_buffer((unsigned char *)&header,
						       sizeof(header));
		if (retval < 0)
			break;

		/* let count decrease twice */
		hievent_buffer_dec(sizeof(header));
		hievent_buffer_dec(header.len);
	}
}

int hievent_write_internal(const char *buffer, size_t buf_len)
{
	struct hievent_entry header;
	int retval;

	if (buf_len < sizeof(int) ||
	    buf_len > HIEVENT_BUFFER - sizeof(struct hievent_entry))
		return -EINVAL;

	(void)mutex_lock(&hievent_dev.mtx);

	hievent_cover_old_log(buf_len);

	hievent_head_init(&header, buf_len);
	retval = hievent_write_ring_head_buffer((unsigned char *)&header,
						sizeof(header));
	if (retval) {
		retval = -EINVAL;
		goto out;
	}
	hievent_buffer_inc(sizeof(header));

	retval = hievent_write_ring_head_buffer((unsigned char *)(buffer),
						header.len);
	if (retval) {
		retval = -EINVAL;
		goto out;
	}

	hievent_buffer_inc(header.len);

	retval = header.len;

out:
	(void)mutex_unlock(&hievent_dev.mtx);
	if (retval > 0)
		wake_up_interruptible(&hievent_dev.wq);

	return retval;
}
EXPORT_SYMBOL(hievent_write_internal);

static unsigned int hievent_poll(struct file *filep, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(filep, &hievent_dev.wq, wait);
	if (hievent_dev.size > 0) {
		mask |= POLLIN | POLLRDNORM;
		return mask;
	}

	return 0;
}

static ssize_t hievent_write_iter(struct kiocb *iocb, struct iov_iter *from)
{
	int check_code = 0;
	unsigned char *temp_buffer = NULL;
	const struct iovec *iov = iter_iov(from);
	size_t retval;
	size_t buf_len;
	(void)iocb;

	if (from->nr_segs != 2) { /* must contain 2 segments */
		pr_err("invalid nr_segs: %ld", from->nr_segs);
		retval = -EINVAL;
		goto out;
	}

	/* seg 0 info is checkcode*/
	retval = copy_from_user(&check_code, iov[0].iov_base,
				sizeof(check_code));
	if (retval || check_code != CHECK_CODE) {
		retval = -EINVAL;
		goto out;
	}

	/* seg 1 info */
	buf_len = iov[1].iov_len;
	if (buf_len > HIEVENT_BUFFER - sizeof(struct hievent_entry)) {
		retval = -ENOMEM;
		goto out;
	}

	temp_buffer = kmalloc(buf_len, GFP_KERNEL);
	if (!temp_buffer) {
		retval = -ENOMEM;
		goto out;
	}

	retval = copy_from_user(temp_buffer, iov[1].iov_base, iov[1].iov_len);
	if (retval) {
		retval = -EIO;
		goto free_mem;
	}

	retval = hievent_write_internal(temp_buffer, buf_len);
	if ((int)retval < 0) {
		retval = -EIO;
		goto free_mem;
	}
	retval = buf_len + iov[0].iov_len;

free_mem:
	kfree(temp_buffer);

out:
	return (ssize_t)retval;
}

static const struct file_operations hievent_fops = {
	.read = hievent_read, /* read */
	.poll = hievent_poll, /* poll */
	.write_iter = hievent_write_iter, /* write_iter */
};

static int hievent_device_init(void)
{
	hievent_dev.buffer = kmalloc(HIEVENT_BUFFER, GFP_KERNEL);
	if (!hievent_dev.buffer)
		return -ENOMEM;

	init_waitqueue_head(&hievent_dev.wq);
	mutex_init(&hievent_dev.mtx);
	hievent_dev.write_offset = 0;
	hievent_dev.head_offset = 0;
	hievent_dev.size = 0;
	hievent_dev.count = 0;

	return 0;
}

static int __init hieventdev_init(void)
{
	int result;
	struct device *dev_ret = NULL;

	result = alloc_chrdev_region(&hievent_devno, 0, HIEVENT_DEV_NR,
				     HIEVENT_DEV_NAME);
	if (result < 0) {
		pr_err("register %s failed", HIEVENT_DRIVER);
		return -ENODEV;
	}

	cdev_init(&hievent_dev.devm, &hievent_fops);
	hievent_dev.devm.owner = THIS_MODULE;

	result = cdev_add(&hievent_dev.devm, hievent_devno, HIEVENT_DEV_NR);
	if (result < 0) {
		pr_err("cdev_add failed");
		goto unreg_dev;
	}

	result = hievent_device_init();
	if (result < 0) {
		pr_err("hievent_device_init failed");
		goto del_dev;
	}

	hievent_class = class_create(HIEVENT_DEV_NAME);
	if (IS_ERR(hievent_class)) {
		pr_err("class_create failed");
		goto del_buffer;
	}

	dev_ret = device_create(hievent_class, 0, hievent_devno, 0,
				HIEVENT_DEV_NAME);
	if (IS_ERR(dev_ret)) {
		pr_err("device_create failed");
		goto del_class;
	}

	return 0;

del_class:
	class_destroy(hievent_class);
del_buffer:
	kfree(hievent_dev.buffer);
del_dev:
	cdev_del(&hievent_dev.devm);
unreg_dev:
	unregister_chrdev_region(hievent_devno, HIEVENT_DEV_NR);

	return -ENODEV;
}

static void __exit hievent_exit_module(void)
{
	device_destroy(hievent_class, hievent_devno);
	class_destroy(hievent_class);
	kfree(hievent_dev.buffer);
	cdev_del(&hievent_dev.devm);
	unregister_chrdev_region(hievent_devno, HIEVENT_DEV_NR);
}

static int __init hievent_init_module(void)
{
	int state;

	state = hieventdev_init();
	return state;
}

module_init(hievent_init_module);
module_exit(hievent_exit_module);

MODULE_AUTHOR("OHOS");
MODULE_DESCRIPTION("User mode hievent device interface");
MODULE_LICENSE("GPL");
MODULE_ALIAS("hievent");
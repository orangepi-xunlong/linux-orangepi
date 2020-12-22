/* secure storage driver for sunxi platform 
 *
 * Copyright (C) 2014 Allwinner Ltd. 
 *
 * Author:
 *	Ryan Chen <ryanchen@allwinnertech.com>
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>

#include "sst.h"

static dev_t dev; 
static struct cdev c_dev; 
static struct class *cl; 

static int sst_open(struct inode *i, struct file *f)
{

	printk(KERN_INFO "sst Driver: open()\n");
	return 0;
}
static int sst_close(struct inode *i, struct file *f)
{

	sst_user_file_test();

	printk(KERN_INFO "sst Driver: close()\n");
	return 0;
}
static ssize_t sst_read(struct file *f, char __user *buf, size_t
		len, loff_t *off)
{   

	return 0;
}
static ssize_t sst_write(struct file *f, const char __user *buf,
		size_t len, loff_t *off)
{
	return len;
}
static struct file_operations pugs_fops =
{

	.owner = THIS_MODULE,
	.open = sst_open,
	.release = sst_close,
	.read = sst_read,
	.write = sst_write
};

int  test_init(void) /* Constructor */
{

	if (alloc_chrdev_region(&dev, 0, 1, "sst") < 0)
	{
		return -1;
	}
	if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
	{
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	if (device_create(cl, NULL, dev, NULL, "sst_test") == NULL)
	{

		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	cdev_init(&c_dev, &pugs_fops);
	if (cdev_add(&c_dev, dev, 1) == -1)
	{

		device_destroy(cl, dev);
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	return 0;
}

/* Test interface in device tree */
void  test_exit(void) /* Destructor */
{

	cdev_del(&c_dev);
	device_destroy(cl, dev);
	class_destroy(cl);
	unregister_chrdev_region(dev, 1);
}

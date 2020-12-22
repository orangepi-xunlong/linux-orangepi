/*
 * linux/security/fivm/fivm_dev.c
 *
 * Copyright (C) 2014 Allwinner Ltd. 
 *
 * Author: ryan.chen <ryanchen@allwinnertech.com>
 *
 * allwinner fivm controller device
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>

#include "fivm.h"

#define CMD_FIVM_INIT		_IO('M', 0)
#define CMD_FIVM_ENABLE		_IO('M', 1)
#define CMD_FIVM_SET		_IO('M', 2)

static dev_t dev; 
static struct cdev c_dev; 
static struct class *cl; 

static int fivm_open(struct inode *i, struct file *f)
{
	return 0;
}
static int fivm_close(struct inode *i, struct file *f)
{
	return 0;
}

extern int fivm_init(void);
static long fivm_ioctl (
		struct file *file, 
		unsigned int cmd, 
		unsigned long arg
		)
{
	int rc = 0;
	switch(cmd){
		case CMD_FIVM_ENABLE:
			rc = fivm_enable();
			break ;
		case CMD_FIVM_SET:
			rc = fivm_set(arg);
			break;
		default:
			rc = - EINVAL;
			break;

	}
	return rc ;
}
static struct file_operations pugs_fops =
{

	.owner = THIS_MODULE,
	.open = fivm_open,
	.release = fivm_close,
	.unlocked_ioctl= fivm_ioctl,
};

static int __init fivm_dev_init(void) /* Constructor */
{
	if (alloc_chrdev_region(&dev, 0, 1, "fivm_dev") < 0)
	{
		pr_err("fivm chrdev create fail\n");
		return -1;
	}
	if ((cl = class_create(THIS_MODULE, "fivm_drv")) == NULL)
	{
		pr_err("fivm class create fail\n");
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	if (device_create(cl, NULL, dev, NULL, "fivm") == NULL)
	{
		pr_err("fivm device create fail\n");
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	cdev_init(&c_dev, &pugs_fops);
	if (cdev_add(&c_dev, dev, 1) == -1)
	{
		pr_err("fivm cdev add fail\n");
		device_destroy(cl, dev);
		class_destroy(cl);
		unregister_chrdev_region(dev, 1);
		return -1;
	}
	if( fivm_init() <0 ){
		pr_err("fivm init fail\n");
		return -1 ;
	}

	pr_info("FIVM device register ok \n");
	return 0;
}

static void __exit fivm_dev_exit(void) /* Destructor */
{
	cdev_del(&c_dev);
	device_destroy(cl, dev);
	class_destroy(cl);
	unregister_chrdev_region(dev, 1);
	fivm_cleanup();
}

module_init(fivm_dev_init);
module_exit(fivm_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ryan chen<ryanchen@allsoftwinnertech.com>");
MODULE_DESCRIPTION("Sunxi file integrity verify module for system");


/* linux/drivers/video/sunxi/lcd/dev_lcd.c
 *
 * Copyright (c) 2013 Allwinnertech Co., Ltd.
 * Author: Tyle <tyle@allwinnertech.com>
 *
 * LCD driver for sunxi platform
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "dev_lcd.h"

struct sunxi_lcd_drv g_lcd_drv;

static int lcd_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int lcd_release(struct inode *inode, struct file *file)
{
	return 0;
}


static ssize_t lcd_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

static ssize_t lcd_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

static int lcd_mmap(struct file *file, struct vm_area_struct * vma)
{
	return 0;
}

static long lcd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}

static const struct file_operations lcd_fops =
{
	.owner          = THIS_MODULE,
	.open           = lcd_open,
	.release        = lcd_release,
	.write          = lcd_write,
	.read           = lcd_read,
	.unlocked_ioctl = lcd_ioctl,
	.mmap           = lcd_mmap,
};

static int lcd_init(void)
{
	sunxi_disp_get_source_ops(&g_lcd_drv.src_ops);
	LCD_set_panel_funs();

	return 0;
}

static int __init lcd_module_init(void)
{
	int err;

	pr_info("[LCD]==lcd_module_init==\n");

	alloc_chrdev_region(&g_lcd_drv.devid, 0, 1, "lcd");
	g_lcd_drv.lcd_cdev = cdev_alloc();
	cdev_init(g_lcd_drv.lcd_cdev, &lcd_fops);
	g_lcd_drv.lcd_cdev->owner = THIS_MODULE;
	err = cdev_add(g_lcd_drv.lcd_cdev, g_lcd_drv.devid, 1);
	if (err) {
		__wrn("cdev_add fail.\n");
		return -1;
	}

	g_lcd_drv.lcd_class = class_create(THIS_MODULE, "lcd");
	if (IS_ERR(g_lcd_drv.lcd_class)) {
		__wrn("class_create fail\n");
		return -1;
	}

	device_create(g_lcd_drv.lcd_class, NULL, g_lcd_drv.devid, NULL, "lcd");

	lcd_init();

	pr_info("[LCD]==lcd_module_init finish==\n");

	return 0;
}

static void __exit lcd_module_exit(void)
{
	__inf("lcd_module_exit\n");

	device_destroy(g_lcd_drv.lcd_class,  g_lcd_drv.devid);

	class_destroy(g_lcd_drv.lcd_class);

	cdev_del(g_lcd_drv.lcd_cdev);
}

late_initcall(lcd_module_init);
module_exit(lcd_module_exit);

MODULE_AUTHOR("tyle");
MODULE_DESCRIPTION("lcd driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:lcd");



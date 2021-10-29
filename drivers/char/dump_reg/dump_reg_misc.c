/*
 * misc dump registers driver -
 * User space application could use dump-reg functions through file operations
 * (open/read/write/close) to the sysfs node created by this driver.
 *
 * Copyright(c) 2015-2018 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: Liugang <liugang@allwinnertech.com>
 *         Xiafeng <xiafeng@allwinnertech.com>
 *         Martin  <wuyan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/seq_file.h>
#include "dump_reg.h"

/* for dump_reg misc driver */
static struct dump_addr misc_dump_para;
static struct write_group *misc_wt_group;
static struct compare_group *misc_cmp_group;

static ssize_t
misc_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return __dump_regs_ex(&misc_dump_para, buf, PAGE_SIZE);
}

static ssize_t
misc_dump_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	int index;
	unsigned long start_reg = 0;
	unsigned long end_reg = 0;

	if (__parse_dump_str(buf, size, &start_reg, &end_reg)) {
		pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
		goto err;
	}

	index = __addr_valid(start_reg);
	if ((index < 0) || (index != __addr_valid(end_reg))) {
		pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
		goto err;
	}

	misc_dump_para.uaddr_start = start_reg;
	misc_dump_para.uaddr_end = end_reg;
	pr_debug("%s,%d, start_reg:" PRINT_ADDR_FMT ", end_reg:" PRINT_ADDR_FMT
		 "\n", __func__, __LINE__, start_reg, end_reg);

	return size;

err:
	misc_dump_para.uaddr_start = 0;
	misc_dump_para.uaddr_end = 0;

	return -EINVAL;
}

static ssize_t
misc_write_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* display write result */
	return __write_show(misc_wt_group, buf, PAGE_SIZE);
}

static ssize_t
misc_write_store(struct device *dev, struct device_attribute *attr,
		 const char *buf, size_t size)
{
	int i;
	int index;
	unsigned long reg;
	u32 val;
	const struct dump_struct *dump;
	struct dump_addr dump_addr;

	/* free if not NULL */
	if (misc_wt_group) {
		__write_item_deinit(misc_wt_group);
		misc_wt_group = NULL;
	}

	/* parse input buf for items that will be dumped */
	if (__write_item_init(&misc_wt_group, buf, size) < 0)
		return -EINVAL;

	/**
	 * write reg
	 * it is better if the regs been remaped and unmaped only once,
	 * but we map everytime for the range between min and max address
	 * maybe too large.
	 */
	for (i = 0; i < misc_wt_group->num; i++) {
		reg = misc_wt_group->pitem[i].reg_addr;
		dump_addr.uaddr_start = reg;
		val = misc_wt_group->pitem[i].val;
		index = __addr_valid(reg);
		dump = &dump_table[index];
		if (dump->remap)
			dump_addr.vaddr_start = dump->remap(reg, 4);
		else
			dump_addr.vaddr_start = (void __iomem *)reg;
		dump->write(val, dump->get_vaddr(&dump_addr, reg));
		if (dump->unmap)
			dump->unmap(dump_addr.vaddr_start);
	}

	return size;
}

static ssize_t
misc_compare_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	/* dump the items */
	return __compare_regs_ex(misc_cmp_group, buf, PAGE_SIZE);
}

static ssize_t
misc_compare_store(struct device *dev, struct device_attribute *attr,
		   const char *buf, size_t size)
{
	/* free if struct not null */
	if (misc_cmp_group) {
		__compare_item_deinit(misc_cmp_group);
		misc_cmp_group = NULL;
	}

	/* parse input buf for items that will be dumped */
	if (__compare_item_init(&misc_cmp_group, buf, size) < 0)
		return -EINVAL;

	return size;
}

static DEVICE_ATTR(dump, S_IWUSR | S_IRUGO, misc_dump_show, misc_dump_store);
static DEVICE_ATTR(write, S_IWUSR | S_IRUGO, misc_write_show, misc_write_store);
static DEVICE_ATTR(compare, S_IWUSR | S_IRUGO, misc_compare_show,
		   misc_compare_store);

static struct attribute *misc_attributes[] = {  /* files under '/sys/devices/virtual/misc/sunxi-reg/rw/' */
	&dev_attr_dump.attr,
	&dev_attr_write.attr,
	&dev_attr_compare.attr,
	NULL,
};

static struct attribute_group misc_attribute_group = {
	.name = "rw",  /* directory: '/sys/devices/virtual/misc/sunxi-reg/rw/' */
	.attrs = misc_attributes,
};

static struct miscdevice dump_reg_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "sunxi-reg",  /* device node: '/dev/sunxi-reg' */
};

static int __init misc_dump_reg_init(void)
{
	int err;

	pr_info("misc dump reg init\n");
	err = misc_register(&dump_reg_dev);
	if (err) {
		pr_err("dump register driver as misc device error!\n");
		goto exit;
	}

	err = sysfs_create_group(&dump_reg_dev.this_device->kobj,
				 &misc_attribute_group);
	if (err)
		pr_err("dump register sysfs create group failed!\n");

exit:
	return err;
}

static void __exit misc_dump_reg_exit(void)
{
	pr_info("misc dump reg exit\n");

	sysfs_remove_group(&(dump_reg_dev.this_device->kobj),
			   &misc_attribute_group);
	misc_deregister(&dump_reg_dev);
}

module_init(misc_dump_reg_init);
module_exit(misc_dump_reg_exit);

MODULE_ALIAS("misc dump reg driver");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.1");
MODULE_AUTHOR("xiafeng <xiafeng@allwinnertech.com>");
MODULE_DESCRIPTION("misc dump registers driver");

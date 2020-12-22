/* driver/misc/sunxi-reg.c
 *
 *  Copyright (C) 2011 Reuuimlla Technology Co.Ltd
 *  Tom Cubie <tangliang@reuuimllatech.com>
 *  update by panlong <panlong@reuuimllatech.com> , 2012-4-19 15:39
 *
 *  www.reuuimllatech.com
 *
 *  User access to the registers driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <mach/hardware.h>
#include <mach/platform.h>

#ifdef CONFIG_ARCH_SUN8I
    #ifdef CONFIG_ARCH_SUN8IW7P1
        #define SUNXI_HW_TEST_START	SUNXI_BROM1_S_VBASE
    #elif defined CONFIG_ARCH_SUN8IW6P1
        #define SUNXI_HW_TEST_START	SUNXI_BROM1_S_VBASE
    #else
        #define SUNXI_HW_TEST_START	SUNXI_BROM_VBASE
    #endif
#elif defined CONFIG_ARCH_SUN9I
#define SUNXI_HW_TEST_START	SUNXI_BROM1_S_VBASE
#else
#error "please define SUNXI_HW_TEST_START=(brom virt base) in driver/misc/sunxi_hw_test.c !\n"
#endif
#define SUNXI_HW_TEST_SIZE	1024

static ssize_t in_show(struct file *filp, struct kobject *kobj, struct bin_attribute *a,
				char *buf, loff_t off, size_t count)
{
	memcpy(buf, (char *)SUNXI_HW_TEST_START, SUNXI_HW_TEST_SIZE);
	if(off + count > SUNXI_HW_TEST_SIZE)
		count= SUNXI_HW_TEST_SIZE - off;
	return count;
}

static struct bin_attribute sunxi_hw_attrs = {
	.attr = {
			.name = "sunxi_hw",
			.mode = 0444,
		},
		.size = 1024 * 2,
		.read = in_show,
		.write= NULL,
};

static struct miscdevice sunxi_hw_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name =	"sunxi_hw",
};

static int __init sunxi_hw_init(void) {
	int err;

	pr_info("%s: sunxi debug register driver init\n", __func__);
	err = misc_register(&sunxi_hw_dev);
	if(err) {
		pr_err("%s register sunxi debug register driver as misc device error\n", __func__);
		goto exit;
	}

	err=sysfs_create_bin_file(&sunxi_hw_dev.this_device->kobj, &sunxi_hw_attrs);
	if(err)
		pr_err("%s sysfs_create_group  error\n", __func__);
exit:
	return err;
}

static void __exit sunxi_hw_exit(void) {
	pr_info("%s: exit\n", __func__);
	misc_deregister(&sunxi_hw_dev);
	sysfs_remove_bin_file(&sunxi_hw_dev.this_device->kobj, &sunxi_hw_attrs);
}

module_init(sunxi_hw_init);
module_exit(sunxi_hw_exit);

MODULE_DESCRIPTION("a simple sunxi register driver");
MODULE_AUTHOR("panlong");
MODULE_LICENSE("GPL");


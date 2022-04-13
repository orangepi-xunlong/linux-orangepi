/*
 * Based on drivers/char/sunxi-sysinfo/sunxi-sysinfo.c
 *
 * Copyright (C) 2015 Allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/compat.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/of.h>

extern int sunxi_get_soc_chipid(unsigned char *chipid);
extern int sunxi_get_serial(unsigned  char *serial);

struct sunxi_info_quirks {
	char * platform_name;
};

static const struct sunxi_info_quirks sun5i_h6_info_quirks = {
	.platform_name  = "sun50i-h6",
};

static const struct sunxi_info_quirks sun5i_h616_info_quirks = {
	.platform_name  = "sun50i-h616",
};

struct sunxi_info_quirks *quirks;

static int soc_info_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int soc_info_release(struct inode *inode, struct file *file)
{
	return 0;
}

static const struct file_operations soc_info_ops = {
	.owner   = THIS_MODULE,
	.open    = soc_info_open,
	.release = soc_info_release,
};

struct miscdevice soc_info_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "sunxi_soc_info",
	.fops  = &soc_info_ops,
};

static ssize_t sys_info_show(struct class *class,
			     struct class_attribute *attr, char *buf)
{
	int i;
	int databuf[4] = {0};
	char tmpbuf[129] = {0};
	size_t size = 0;

	/* platform */
	size += sprintf(buf + size, "sunxi_platform    : %s\n", quirks->platform_name);

	/* chipid */
	sunxi_get_soc_chipid((u8 *)databuf);

	for (i = 0; i < 4; i++)
		sprintf(tmpbuf + i*8, "%08x", databuf[i]);
	tmpbuf[128] = 0;
	size += sprintf(buf + size, "sunxi_chipid      : %s\n", tmpbuf);

	/* serial */
	sunxi_get_serial((u8 *)databuf);
	for (i = 0; i < 4; i++)
		sprintf(tmpbuf + i*8, "%08x", databuf[i]);
	tmpbuf[128] = 0;
	size += sprintf(buf + size, "sunxi_serial      : %s\n", tmpbuf);

	return size;
}

static struct class_attribute info_class_attrs[] = {
	__ATTR(sys_info, 0644, sys_info_show, NULL),
};

static struct class info_class = {
	.name           = "sunxi_info",
	.owner          = THIS_MODULE,
};

static const struct of_device_id sunxi_info_match[] = {
        {
		.compatible = "allwinner,sun50i-h6-sys-info",
		.data = &sun5i_h6_info_quirks,
        },
        {
		.compatible = "allwinner,sun50i-h616-sys-info",
		.data = &sun5i_h616_info_quirks,
        },
        {}
};

static int sunxi_info_probe(struct platform_device *pdev)
{
	int i, ret = 0;

	quirks = of_device_get_match_data(&pdev->dev);
	if (quirks == NULL) {
		dev_err(&pdev->dev, "Failed to determine the quirks to use\n");
		return -ENODEV;
	}

	ret = class_register(&info_class);
	if (ret != 0)
		return ret;

	/* need some class specific sysfs attributes */
	for (i = 0; i < ARRAY_SIZE(info_class_attrs); i++) {
		ret = class_create_file(&info_class, &info_class_attrs[i]);
		if (ret)
			goto out_class_create_file_failed;
	}

	ret = misc_register(&soc_info_device);
	if (ret != 0) {
		pr_err("%s: misc_register() failed!(%d)\n", __func__, ret);
		class_unregister(&info_class);
		return ret;
	}

	return ret;

out_class_create_file_failed:
	class_unregister(&info_class);

	return ret;
}

static int sunxi_info_remove(struct platform_device *pdev)
{
	misc_deregister(&soc_info_device);
	class_unregister(&info_class);

	return 0;
}

static struct platform_driver sunxi_info_driver = {
        .probe  = sunxi_info_probe,
        .remove = sunxi_info_remove,
        .driver = {
                .name   = "sunxi_info",
                .owner  = THIS_MODULE,
                .of_match_table = sunxi_info_match,
        },
};
module_platform_driver(sunxi_info_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("xiafeng<xiafeng@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi sys info.");

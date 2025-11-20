/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>

static u32 dram_clk;
static u32 dram_type;
static u32 dram_width;
static u32 dram_bandwidth;
static char dram_clk_str[11];
static char dram_type_str[11];
static char dram_width_str[5];
static char dram_bandwidth_str[11];

static struct device *dram_info_device;
static struct class *dram_info_class;
static dev_t dram_dev;

static ssize_t dram_clk_show(struct device *dev, struct device_attribute *attr,
							 char *buf)
{
	return sprintf(buf, "%s MHz\n", dram_clk_str);
}
static struct device_attribute dev_attr_dram_clk = __ATTR(dram_clk, 0444, dram_clk_show, NULL);

static ssize_t dram_type_show(struct device *dev, struct device_attribute *attr,
							  char *buf)
{
	return sprintf(buf, "%s\n", dram_type_str);
}
static struct device_attribute dev_attr_dram_type = __ATTR(dram_type, 0444, dram_type_show, NULL);

static ssize_t dram_width_show(struct device *dev, struct device_attribute *attr,
							   char *buf)
{
	return sprintf(buf, "%s bit\n", dram_width_str);
}
static struct device_attribute dev_attr_dram_width = __ATTR(dram_width, 0444, dram_width_show, NULL);

static ssize_t dram_bandwidth_show(struct device *dev, struct device_attribute *attr,
							   char *buf)
{
	return sprintf(buf, "%s MB\n", dram_bandwidth_str);
}
static struct device_attribute dev_attr_dram_bandwidth = __ATTR(dram_bandwidth, 0444, dram_bandwidth_show, NULL);

static int dram_info_probe(void)
{
	struct device_node *dram_node;
	int len;
	const __be32 *val;

	dram_node = of_find_node_by_path("/dram");
	if (!dram_node) {
		pr_err("Failed to find dram node\n");
		return -ENODEV;
	}

	val = of_get_property(dram_node, "dram_para00", &len);
	if (!val) {
		pr_err("Failed to read dram_para00\n");
		of_node_put(dram_node);
		return -EINVAL;
	}
	dram_clk = be32_to_cpup(val);
	sprintf(dram_clk_str, "%d", dram_clk);

	val = of_get_property(dram_node, "dram_para01", &len);
	if (!val) {
		pr_err("Failed to read dram_para01\n");
		of_node_put(dram_node);
		return -EINVAL;
	}
	dram_type = be32_to_cpup(val);
	switch (dram_type) {
	case 2:
		strcpy(dram_type_str, "DDR2");
		break;
	case 3:
		strcpy(dram_type_str, "DDR3");
		break;
	case 4:
		strcpy(dram_type_str, "DDR4");
		break;
	case 5:
		strcpy(dram_type_str, "DDR5");
		break;
	case 6:
		strcpy(dram_type_str, "LPDDR2");
		break;
	case 7:
		strcpy(dram_type_str, "LPDDR3");
		break;
	case 8:
		strcpy(dram_type_str, "LPDDR4");
		break;
	case 9:
		strcpy(dram_type_str, "LPDDR5");
		break;
	default:
		break;
	}

	val = of_get_property(dram_node, "dram_para07", &len);
	if (!val) {
		pr_err("Failed to read dram_para07\n");
		of_node_put(dram_node);
		return -EINVAL;
	}
	if ((be32_to_cpup(val)& 0x0F) == 0) {
		dram_width = 32;
	} else if ((be32_to_cpup(val)& 0x0F) == 1) {
		dram_width = 16;
	} else {
		dram_width = 0xffff;
	}
	sprintf(dram_width_str, "%d", dram_width);

	dram_bandwidth = dram_clk * dram_width / 8;
	sprintf(dram_bandwidth_str, "%d", dram_bandwidth);

	of_node_put(dram_node);

	return 0;
}

/*
 * Initialize the dram info module
 * Returns 0 on success, negative error code on failure
 */
static int __init sunxi_dram_info_init(void)
{
	int ret = 0;

	ret = alloc_chrdev_region(&dram_dev, 0, 1, "dram_info");
	if (ret < 0) {
		pr_err("Failed to allocate chrdev region\n");
		goto err_out;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
	dram_info_class = class_create("dram");
#else
	dram_info_class = class_create(THIS_MODULE, "dram");
#endif
	if (IS_ERR(dram_info_class)) {
		ret = PTR_ERR(dram_info_class);
		pr_err("Failed to create class\n");
		goto err_unregister_chrdev;
	}

	dram_info_device = device_create(dram_info_class, NULL, dram_dev, NULL, "dram_info");
	if (IS_ERR(dram_info_device)) {
		ret = PTR_ERR(dram_info_device);
		pr_err("Failed to create the device\n");
		goto err_destroy_class;
	}

	ret = dram_info_probe();
	if (ret) {
		pr_err("Failed to probe dram info\n");
		goto err_destroy_device;
	}

	// Create device files sequentially with individual error checking
	ret = device_create_file(dram_info_device, &dev_attr_dram_clk);
	if (ret) {
		pr_err("Failed to create dram_clk device file\n");
		goto err_remove_files;
	}

	ret = device_create_file(dram_info_device, &dev_attr_dram_type);
	if (ret) {
		pr_err("Failed to create dram_type device file\n");
		goto err_remove_files;
	}

	ret = device_create_file(dram_info_device, &dev_attr_dram_width);
	if (ret) {
		pr_err("Failed to create dram_width device file\n");
		goto err_remove_files;
	}

	ret = device_create_file(dram_info_device, &dev_attr_dram_bandwidth);
	if (ret) {
		pr_err("Failed to create dram_bandwidth device file\n");
		goto err_remove_files;
	}

	pr_info("sunxi_dram_info module loaded\n");
	return 0;

err_remove_files:
	// Remove files in reverse creation order to maintain consistency
	device_remove_file(dram_info_device, &dev_attr_dram_bandwidth);
	device_remove_file(dram_info_device, &dev_attr_dram_width);
	device_remove_file(dram_info_device, &dev_attr_dram_type);
	device_remove_file(dram_info_device, &dev_attr_dram_clk);
err_destroy_device:
	device_destroy(dram_info_class, dram_dev);
err_destroy_class:
	class_destroy(dram_info_class);
err_unregister_chrdev:
	unregister_chrdev_region(dram_dev, 1);
err_out:
	return ret;
}

/*
 * Cleanup the dram info module
 */
static void __exit sunxi_dram_info_exit(void)
{
	device_remove_file(dram_info_device, &dev_attr_dram_clk);
	device_remove_file(dram_info_device, &dev_attr_dram_type);
	device_remove_file(dram_info_device, &dev_attr_dram_width);
	device_remove_file(dram_info_device, &dev_attr_dram_bandwidth);
	device_destroy(dram_info_class, dram_dev);
	class_destroy(dram_info_class);
	unregister_chrdev_region(dram_dev, 1);
	pr_info("sunxi_dram_info module unloaded\n");
}

module_init(sunxi_dram_info_init);
module_exit(sunxi_dram_info_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("xue fan");
MODULE_DESCRIPTION("Sunxi dram information driver");
MODULE_VERSION("1.0.0");

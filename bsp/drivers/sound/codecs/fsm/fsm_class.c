/* SPDX-License-Identifier: GPL-2.0-or-later */
/**
 * Copyright (C) Fourier Semiconductor Inc. 2016-2020. All rights reserved.
 * 2020-01-20 File created.
 */

#include "fsm_public.h"
#if defined(CONFIG_FSM_SYSFS)
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/kobject.h>
#include <linux/version.h>


static int g_fsm_class_inited;

static ssize_t fsm_re25_calib_show(struct class *class,
							struct class_attribute *attr, char *buf)
{
	struct fsm_cal_result *result = NULL;
	struct fsm_calib *info;
	int size;
	int dev;
	int ret;

	size = sizeof(struct fsm_cal_result);
	result = fsm_alloc_mem(size);
	if (!result) {
		pr_err("allocate memory failed");
		return -EINVAL;
	}
	memset(result, 0, size);
	ret = fsm_test_result(result, size);
	pr_info("ndev:%d", result->ndev);
	size = 0;
	for (dev = 0; dev < result->ndev; dev++) {
		info = &result->info[dev];
		// [addr,re25,errcode]
		size += scnprintf(buf + size, PAGE_SIZE, "[%02X,%d,%d]",
						info->addr, info->re25, info->errcode);
	}
	size += scnprintf(buf + size, PAGE_SIZE, "\n");
	fsm_free_mem(result);

	return size;
}

static ssize_t fsm_re25_calib_store(struct class *class,
							struct class_attribute *attr, const char *buf, size_t len)
{
	int value = simple_strtoul(buf, NULL, 0);

	fsm_re25_test(value != 0);

	return len;
}

static ssize_t fsm_f0_calib_show(struct class *class,
							struct class_attribute *attr, char *buf)
{
	struct fsm_cal_result *result = NULL;
	struct fsm_calib *data;
	int size;
	int dev;
	int ret;

	size = sizeof(struct fsm_cal_result);
	result = fsm_alloc_mem(size);
	if (!result) {
		pr_err("allocate memory failed");
		return -EINVAL;
	}
	memset(result, 0, size);
	ret = fsm_test_result(result, size);
	pr_info("ndev:%d", result->ndev);
	size = 0;
	for (dev = 0; dev < result->ndev; dev++) {
		data = &result->info[dev];
		// [addr,f0,errcode]
		size += scnprintf(buf + size, PAGE_SIZE, "[%02X,%d,%d]",
						data->addr, data->f0, data->errcode);
	}
	size += scnprintf(buf + size, PAGE_SIZE, "\n");
	fsm_free_mem(result);

	return size;
}

static ssize_t fsm_f0_calib_store(struct class *class,
						struct class_attribute *attr, const char *buf, size_t len)
{
	fsm_config_t *cfg = fsm_get_config();
	int start;
	int end;
	int step;

	pr_info("len:%zu", len);
	if (len > 5) {
		sscanf(buf, "%d,%d,%d", &start, &end, &step);
		pr_info("start:%d, end:%d, step:%d", start, end, step);
		cfg->freq_start = start;
		cfg->freq_end = end;
		cfg->freq_step = step;
		cfg->freq_count = COUNT(start, end, step);
	}
	fsm_f0_test();

	return len;
}

static ssize_t fsm_reg_show(struct class *class,
							struct class_attribute *attr, char *buf)
{
	fsm_dump();
	return scnprintf(buf, PAGE_SIZE, "%s\n", __func__);
}

static ssize_t fsm_reg_store(struct class *class,
							struct class_attribute *attr, const char *buf, size_t len)
{
	int i;

	pr_info("len:%zu", len);
	for (i = 0; i < len; i++) {
		pr_info("buf[%d]:%c", i, buf[i]);
	}

	return len;
}

static ssize_t fsm_dev_info_show(struct class *class,
							struct class_attribute *attr, char *buf)
{
	fsm_version_t version;
	struct preset_file *pfile;
	int dev_count;
	int len = 0;

	fsm_get_version(&version);
	len  = scnprintf(buf + len, PAGE_SIZE, "version: %s\n",
					version.code_version);
	len += scnprintf(buf + len, PAGE_SIZE, "branch : %s\n",
					version.git_branch);
	len += scnprintf(buf + len, PAGE_SIZE, "commit : %s\n",
					version.git_commit);
	len += scnprintf(buf + len, PAGE_SIZE, "date   : %s\n",
					version.code_date);
	pfile = fsm_get_presets();
	dev_count = (pfile ? pfile->hdr.ndev : 0);
	len += scnprintf(buf + len, PAGE_SIZE, "device : [%d, %d]\n",
					dev_count, fsm_dev_count());

	return len;
}

static ssize_t fsm_dev_debug_store(struct class *class,
					struct class_attribute *attr, const char *buf, size_t len)
{
	fsm_config_t *cfg = fsm_get_config();
	int value = simple_strtoul(buf, NULL, 0);

	if (!cfg) {
		return 0;
	}
	cfg->i2c_debug = !!value;
	pr_info("i2c debug: %s", (cfg->i2c_debug ? "ON" : "OFF"));

	return len;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
static struct class_attribute g_fsm_class_attrs[] = {
	__ATTR(re25, S_IRUGO | S_IWUSR, fsm_re25_calib_show, fsm_re25_calib_store),
	__ATTR(f0, S_IRUGO | S_IWUSR, fsm_f0_calib_show, fsm_f0_calib_store),
	__ATTR(reg, S_IRUGO | S_IWUSR, fsm_reg_show, fsm_reg_store),
	__ATTR(info, S_IRUGO, fsm_dev_info_show, NULL),
	__ATTR(debug, S_IWUSR, NULL, fsm_dev_debug_store),
	__ATTR_NULL
};

static struct class g_fsm_class = {
		.name = FSM_DRV_NAME,
		.class_attrs = g_fsm_class_attrs,
	};

#else
static CLASS_ATTR_RW(fsm_re25_calib);
static CLASS_ATTR_RW(fsm_f0_calib);
static CLASS_ATTR_RW(fsm_reg);
static CLASS_ATTR_RO(fsm_dev_info);
static CLASS_ATTR_WO(fsm_dev_debug);

static struct attribute *fsm_class_attrs[] = {
	&class_attr_fsm_re25_calib.attr,
	&class_attr_fsm_f0_calib.attr,
	&class_attr_fsm_reg.attr,
	&class_attr_fsm_dev_info.attr,
	&class_attr_fsm_dev_debug.attr,
	NULL,
};
ATTRIBUTE_GROUPS(fsm_class);

/** Device model classes */
struct class g_fsm_class = {
		.name = FSM_DRV_NAME,
		.class_groups = fsm_class_groups,
	};
#endif

int fsm_sysfs_init(struct device *dev)
{
	int ret;

	if (g_fsm_class_inited) {
		return MODULE_INITED;
	}
	// path: sys/class/$(FSM_DRV_NAME)
	ret = class_register(&g_fsm_class);
	if (!ret) {
		g_fsm_class_inited = 1;
	}

	return ret;
}

void fsm_sysfs_deinit(struct device *dev)
{
	class_unregister(&g_fsm_class);
	g_fsm_class_inited = 0;
}
#endif

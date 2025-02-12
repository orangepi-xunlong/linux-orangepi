// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/sysfs.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>

#include "../ky_lib.h"
#include "../ky_dpu.h"
#include "../ky_mipi_panel.h"
#include "sysfs_display.h"

#ifdef CONFIG_PM
static ssize_t ky_dpu_get_enable_auto_fc(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "enable_auto_fc = %d %d\n", dpu->enable_auto_fc, dpu->dev->power.runtime_status);
}
#endif

#ifdef CONFIG_PM
static ssize_t ky_dpu_set_enable_auto_fc(struct device *dev,
					struct device_attribute *attr,
					const char *buf,
					size_t count)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);
	int enable_auto_fc = 0;
	int ret = 0;

	if (dpu->dev->power.runtime_status != RPM_SUSPENDED) {
		pr_err("set dpu_enable_auto_fc only support when screen off!\n");
		return -EINVAL;
	}

	ret = sscanf(buf, "%d\n", &enable_auto_fc);
	if ((ret != 1) || (enable_auto_fc < 0) || (enable_auto_fc > 1)) {
		pr_err("Wrong parameter! Please echo 0 or 1\n");
		return -EINVAL;
	}

	if (enable_auto_fc == 0) {
		dpu->new_mclk = DPU_MCLK_DEFAULT;
		if (dpu->core && dpu->core->update_clk)
			dpu->core->update_clk(dpu, dpu->new_mclk);
	}

	dpu->enable_auto_fc = enable_auto_fc;

	return count;
}
#endif

static ssize_t ky_dpu_get_enable_dump_reg(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "enable_dump_reg = %d\n", dpu->enable_dump_reg);
}

static ssize_t ky_dpu_set_enable_dump_reg(struct device *dev,
						struct device_attribute *attr,
						const char *buf,
						size_t count)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);
	int enable_dump_reg = 0;
	int ret = 0;

	ret = sscanf(buf, "%d\n", &enable_dump_reg);
	if ((ret != 1) || (enable_dump_reg < 0) || (enable_dump_reg > 1)) {
		pr_err("Wrong parameter! Please echo 0 or 1\n");
		return -EINVAL;
	}

	dpu->enable_dump_reg = enable_dump_reg;

	return count;
}

static ssize_t ky_dpu_get_enable_dump_fps(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "enable_dump_fps = %d\n", dpu->enable_dump_fps);
}

static ssize_t ky_dpu_set_enable_dump_fps(struct device *dev,
						struct device_attribute *attr,
						const char *buf,
						size_t count)
{
	struct ky_dpu *dpu = dev_get_drvdata(dev);
	int enable_dump_fps = 0;
	int ret = 0;

	ret = sscanf(buf, "%d\n", &enable_dump_fps);
	if ((ret != 1) || (enable_dump_fps < 0) || (enable_dump_fps > 1)) {
		pr_err("Wrong parameter! Please echo 0 or 1\n");
		return -EINVAL;
	}

	dpu->enable_dump_fps = enable_dump_fps;

	return count;
}

static DEVICE_ATTR(dpu_enable_dump_fps, S_IRUGO | S_IWUSR, ky_dpu_get_enable_dump_fps, ky_dpu_set_enable_dump_fps);
static DEVICE_ATTR(dpu_enable_dump_reg, S_IRUGO | S_IWUSR, ky_dpu_get_enable_dump_reg, ky_dpu_set_enable_dump_reg);
#ifdef CONFIG_PM
static DEVICE_ATTR(dpu_enable_auto_fc, S_IRUGO | S_IWUSR, ky_dpu_get_enable_auto_fc, ky_dpu_set_enable_auto_fc);
#endif

int ky_dpu_sysfs_init(struct device *dev)
{

	int ret = 0;

	ret = device_create_file(dev, &dev_attr_dpu_enable_dump_reg);
	if (ret)
		DRM_ERROR("failed to create device file: enable_dump_reg\n");
	else
		DRM_DEBUG("create device file enable_dump_reg\n");

	ret = device_create_file(dev, &dev_attr_dpu_enable_dump_fps);
	if (ret)
		DRM_ERROR("failed to create device file: enable_dump_fps\n");
	else
		DRM_DEBUG("create device file enable_dump_fps\n");
#ifdef CONFIG_PM
	ret = device_create_file(dev, &dev_attr_dpu_enable_auto_fc);
	if (ret)
		DRM_ERROR("failed to create device file: enable_auto_fc\n");
	else
		DRM_DEBUG("create device file enable_auto_fc\n");
#endif
	return 0;
}
EXPORT_SYMBOL(ky_dpu_sysfs_init);

MODULE_DESCRIPTION("Provide dpu attribute nodes for userspace");
MODULE_LICENSE("GPL v2");

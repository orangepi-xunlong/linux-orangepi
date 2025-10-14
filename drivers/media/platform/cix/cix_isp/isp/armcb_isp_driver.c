// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021-2021, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "armcb_isp_driver.h"
#include "armcb_camera_io_drv.h"
#include "armcb_register.h"
#include "isp_hw_ops.h"
#include "system_logger.h"
#include <linux/of_address.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_ISP
#endif

#define ARMCB_ISPDRV_NAME "armcb,isp"

static struct armcb_isp_subdev *p_isp_subdev;

unsigned int armcb_isp_read_reg(unsigned int offset)
{
	unsigned int reg_val = 0;
#ifndef QEMU_ON_VEXPRESS
	void __iomem *virt_addr = p_isp_subdev->reg_base;

	if (virt_addr != NULL) {
		/* Ensure read order to prevent hardware register access reordering */
		rmb();
#ifndef ARMCB_CAM_DEBUG
		reg_val = readl(virt_addr + offset);
#endif
	} else {
		LOG(LOG_ERR, "read isp reg failed !");
	}
#endif
	return reg_val;
}

unsigned int armcb_isp_read_reg2(unsigned int offset)
{
	unsigned int reg_val = 0;
#ifndef QEMU_ON_VEXPRESS
	void __iomem *virt_addr = p_isp_subdev->reg_base2;

	if (virt_addr != NULL) {
		/* Ensure read order to prevent hardware register access reordering */
		rmb();
		reg_val = readl(virt_addr + offset);
	} else {
		LOG(LOG_ERR, "read isp reg failed !");
	}
#endif
	return reg_val;
}

void armcb_isp_write_reg(unsigned int offset, unsigned int value)
{
#ifndef QEMU_ON_VEXPRESS
	void __iomem *virt_addr = p_isp_subdev->reg_base;

	if (virt_addr != NULL) {
		/* Ensure write order to prevent hardware register access reordering */
		wmb();
#ifndef ARMCB_CAM_DEBUG
		writel(value, virt_addr + offset);
#endif
	} else {
		LOG(LOG_ERR, "read isp reg failed !");
	}
#endif
}

void armcb_isp_write_reg2(unsigned int offset, unsigned int value)
{
#ifndef QEMU_ON_VEXPRESS
	void __iomem *virt_addr = p_isp_subdev->reg_base2;

	if (virt_addr != NULL) {
		/* Ensure write order to prevent hardware register access reordering */
		wmb();
		writel(value, virt_addr + offset);
	} else {
		LOG(LOG_ERR, "read isp reg failed !");
	}
#endif
}

static int armcb_isp_parse(struct armcb_isp_subdev *pisp_sd)
{
	struct platform_device *ppdev = pisp_sd->ppdev;
	int res = 0;
	struct resource *rsr;

	rsr = platform_get_resource(ppdev, IORESOURCE_MEM, 0);
	pisp_sd->reg_base = devm_ioremap_resource(&ppdev->dev, rsr);
	if (pisp_sd->reg_base == 0) {
		res = -ENXIO;
		LOG(LOG_ERR, "failed to get and map isp base");
		goto EXIT_RET;
	}

	rsr = platform_get_resource(ppdev, IORESOURCE_MEM, 1);
	pisp_sd->reg_base2 = devm_ioremap_resource(&ppdev->dev, rsr);
	if (pisp_sd->reg_base2 == 0) {
		res = -ENXIO;
		LOG(LOG_ERR, "failed to get and map isp base");
		goto EXIT_RET;
	}
EXIT_RET:
	return res;
}

static long armcb_isp_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
				   void *arg)
{
	struct armcb_isp_subdev *pisp_sd = v4l2_get_subdevdata(sd);
	int res = 0;

	switch (cmd) {
	case ARMCB_VIDIOC_APPLY_CMD: {
		struct cmd_buf *pcmd_buf = (struct cmd_buf *)arg;

		res |= armcb_isp_hw_apply(pcmd_buf, pisp_sd);
		break;
	}
	case ARMCB_VIDIOC_SYS_BUS_TEST: {
		struct perf_bus_params *puser_bus_params =
			(struct perf_bus_params *)arg;
		res |= armcb_sys_bus_test(puser_bus_params);

		break;
	}
	default:
		return -ENOIOCTLCMD;
	}

	return res;
}

static int armcb_isp_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
					 struct v4l2_event_subscription *sub)
{
	return v4l2_event_subscribe(fh, sub, ISP_NEVENTS, NULL);
}

static int armcb_isp_unsubscribe_event(struct v4l2_subdev *sd,
					   struct v4l2_fh *fh,
					   struct v4l2_event_subscription *sub)
{
	return v4l2_event_unsubscribe(fh, sub);
}

static const struct v4l2_subdev_core_ops armcb_isp_subdev_core_ops = {
	.ioctl = &armcb_isp_subdev_ioctl,
	.subscribe_event = &armcb_isp_subscribe_event,
	.unsubscribe_event = &armcb_isp_unsubscribe_event,
};

static const struct v4l2_subdev_ops armcb_isp_subdev_ops = {
	.core = &armcb_isp_subdev_core_ops,
};

int armcb_isp_subdev_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	int ret = 0;
	/* empty funciton */
	return ret;
}

int armcb_isp_subdev_close(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	int ret = 0;
	/* empty funciton */
	return ret;
}

static const struct v4l2_subdev_internal_ops armcb_isp_sd_internal_ops = {
	.open = armcb_isp_subdev_open,
	.close = armcb_isp_subdev_close,
};

static int armcb_ispdrv_probe(struct platform_device *pdev)
{
	struct fwnode_handle *np = pdev->dev.fwnode;
	struct v4l2_subdev *sd = NULL;
	int res = 0;

	LOG(LOG_INFO, "+");
	if (!np) {
		LOG(LOG_ERR, "armcb isp get of node failed !");
		res = -ENODEV;
		goto EXIT_RET;
	}

	p_isp_subdev = devm_kzalloc(&pdev->dev, sizeof(struct armcb_isp_subdev),
					GFP_KERNEL);
	if (!p_isp_subdev) {
		res = -ENOMEM;
		goto EXIT_RET;
	}

	p_isp_subdev->ppdev = pdev;
	p_isp_subdev->fwnode = np;
	p_isp_subdev->id = pdev->id;
	p_isp_subdev->isp_sd.close_seq = ARMCB_SD_CLOSE_1ST_CATEGORY | 0x1;

	sd = &p_isp_subdev->isp_sd.sd;
	if (!sd) {
		LOG(LOG_ERR, "Invalid isp_sd is NULL");
		res = -EFAULT;
		goto ERR_FREE_RET1;
	}

	mutex_init(&p_isp_subdev->imutex);
	spin_lock_init(&p_isp_subdev->sdlock);

	v4l2_subdev_init(sd, &armcb_isp_subdev_ops);

	snprintf(sd->name, sizeof(sd->name), "isp-sub1");
	v4l2_set_subdevdata(sd, p_isp_subdev);

	sd->internal_ops = &armcb_isp_sd_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS | V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->entity.function = ARMCB_CAMERA_SUBDEV_ISP;
	sd->entity.name = sd->name;
	sd->grp_id = ARMCB_SUBDEV_NODE_HW_ISP;
	sd->dev = &pdev->dev;

	res = media_entity_pads_init(&sd->entity, 0, NULL);
	if (res) {
		LOG(LOG_ERR, "isp failed to init media entity. ret = %d", res);
		goto ERR_FREE_RET1;
	}

	res = armcb_subdev_register(&p_isp_subdev->isp_sd, 0);
	if (res < 0) {
		LOG(LOG_ERR, "isp failed to register sub-device.ret = %d", res);
		goto ERR_FREE_RET1;
	}

	init_waitqueue_head(&p_isp_subdev->state_wait);
	platform_set_drvdata(pdev, p_isp_subdev);

	res = armcb_isp_parse(p_isp_subdev);
	if (res < 0) {
		LOG(LOG_ERR, "isp failed to parse dt res = %d", res);
		goto ERR_FREE_RET2;
	}

	LOG(LOG_INFO, "-");
	return res;

ERR_FREE_RET2:
	v4l2_async_unregister_subdev(&p_isp_subdev->isp_sd.sd);
ERR_FREE_RET1:
	devm_kfree(&pdev->dev, p_isp_subdev);

EXIT_RET:
	return res;
}

static int armcb_ispdrv_remove(struct platform_device *pdev)
{
	int res = 0;
	struct armcb_isp_subdev *p_ispdev = platform_get_drvdata(pdev);

	if (p_ispdev) {
		list_del_init(&p_ispdev->isp_sd.sd.async_list);
#if (KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE)
		v4l2_device_unregister_subdev(&p_ispdev->isp_sd.sd);
#endif
	}

	return res;
}

static const struct of_device_id armcb_isp_match[] = {
#ifndef ARMCB_CAM_DEBUG
	{ .compatible = "armcb,sky1-isp" },
#else
	{ .compatible = "armcb,isp" },
#endif
	{}
};

MODULE_DEVICE_TABLE(of, armcb_isp_match);

static const struct acpi_device_id armcb_isp_acpi_match[] = {
	{ .id = "CIXH3021", .driver_data = 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, armcb_isp_acpi_match);

static struct platform_driver armcb_isp_platform_driver = {
	.probe = armcb_ispdrv_probe,
	.remove = armcb_ispdrv_remove,
	.driver = {
		.name = ARMCB_ISPDRV_NAME,
		.owner = THIS_MODULE,
		.of_match_table = armcb_isp_match,
		.acpi_match_table = ACPI_PTR(armcb_isp_acpi_match),
	},
};

#ifndef ARMCB_CAM_KO
static int __init armcb_isp_subdev_init(void)
{
	int res = 0;

	res = platform_driver_register(&armcb_isp_platform_driver);

	return res;
}

static void __exit armcb_isp_subdev_exit(void)
{
	platform_driver_unregister(&armcb_isp_platform_driver);
}

late_initcall(armcb_isp_subdev_init);
module_exit(armcb_isp_subdev_exit);

MODULE_AUTHOR("Armchina Inc.");
MODULE_DESCRIPTION("Armcb isp subdev driver");
MODULE_LICENSE("GPL v2");
#else
static void *g_instance;

void *armcb_get_isp_driver_instance(void)
{
	if (platform_driver_register(&armcb_isp_platform_driver) < 0) {
		LOG(LOG_ERR, "register isp driver failed.\n");
		return NULL;
	}
	g_instance = (void *)&armcb_isp_platform_driver;
	return g_instance;
}

void armcb_isp_driver_destroy(void)
{
	if (g_instance)
		platform_driver_unregister(
			(struct platform_driver *)g_instance);
}
#endif

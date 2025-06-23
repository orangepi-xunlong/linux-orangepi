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
#include "armcb_actuator.h"
#include "isp_hw_ops.h"
#include "system_logger.h"
#include <linux/clk.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <media/videobuf2-v4l2.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_SENSOR
#endif

static long armcb_motor_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
	void *arg)
{
	struct armcb_motor_subdev *pmotor_sd = v4l2_get_subdevdata(sd);
#ifdef CIX_I2C_ACTUATOR
	struct i2c_client *pcline_dev = pmotor_sd->pcline_dev;
#else
	struct spi_device *pspi_dev = pmotor_sd->pspi_dev;
#endif
	int res = 0;

#ifdef CIX_I2C_ACTUATOR
	if (WARN_ON(!pmotor_sd) || WARN_ON(!pcline_dev))
		LOG(LOG_ERR, "pmotor_sd/pcline_dev is NULL");
#else
	if (WARN_ON(!pmotor_sd) || WARN_ON(!pspi_dev))
		LOG(LOG_ERR, "pmotor_sd/pspi_dev is NULL");
#endif

	switch (cmd) {
	case ARMCB_VIDIOC_APPLY_CMD: {
		struct cmd_buf *pcmd_buf = (struct cmd_buf *)arg;
#ifdef CIX_I2C_ACTUATOR
		res |= armcb_isp_hw_apply(pcmd_buf, (void *)pcline_dev);
#else
		res |= armcb_isp_hw_apply(pcmd_buf, (void *)pspi_dev);
#endif
		break;
	}
	default:
		return -ENOIOCTLCMD;
	}

	return res;
}

static const struct v4l2_subdev_core_ops armcb_motor_subdev_core_ops = {
	.ioctl = &armcb_motor_subdev_ioctl,
};

static struct v4l2_subdev_ops armcb_motor_subdev_ops = {
	.core = &armcb_motor_subdev_core_ops,
};

static int armcb_motor_subdev_open(struct v4l2_subdev *sd,
				   struct v4l2_subdev_fh *fh)
{
	int ret = 0;
	/*empty function*/
	return ret;
}

static int armcb_motor_subdev_close(struct v4l2_subdev *sd,
					struct v4l2_subdev_fh *fh)
{
	int ret = 0;
	/*empty function*/
	return ret;
}

static const struct v4l2_subdev_internal_ops armcb_motor_sd_internal_ops = {
	.open = armcb_motor_subdev_open,
	.close = armcb_motor_subdev_close,
};

static int armcb_motor_configure_subdevs(struct armcb_motor_subdev *pmotor_sd)
{
	struct v4l2_subdev *sd = &pmotor_sd->motor_sd.sd;
#ifdef CIX_I2C_ACTUATOR
	struct i2c_client *pcline_dev = pmotor_sd->pcline_dev;
#else
	struct spi_device *spidev = pmotor_sd->pspi_dev;
#endif
	int res = 0;

#ifdef CIX_I2C_ACTUATOR
	v4l2_i2c_subdev_init(sd, pcline_dev, &armcb_motor_subdev_ops);
#else
	v4l2_spi_subdev_init(sd, spidev, &armcb_motor_subdev_ops);
#endif
	sd->owner = THIS_MODULE;
	v4l2_set_subdevdata(sd, pmotor_sd);
	snprintf(sd->name, sizeof(sd->name), "motor0");

	res = of_property_read_u32(pmotor_sd->of_node, CIX_CAMERA_MODULE_INDEX,
				   &pmotor_sd->cam_id);

	LOG(LOG_INFO, "motor id is %d", pmotor_sd->cam_id);

	sd->internal_ops = &armcb_motor_sd_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;

	sd->entity.function = MEDIA_ENT_F_V4L2_SUBDEV_UNKNOWN;
	res = media_entity_pads_init(&sd->entity, 0, NULL);
	if (res) {
		LOG(LOG_ERR, "media_entity_pads_init  failed");
		goto EXIT_RET;
	}

	sd->entity.function = ARMCB_CAMERA_SUBDEV_MOTOR;
	sd->entity.name = sd->name;
	sd->grp_id = ARMCB_SUBDEV_NODE_HW_MOTOR0;
	pmotor_sd->motor_sd.close_seq = ARMCB_SD_CLOSE_2ND_CATEGORY | 0x10;
	res = armcb_subdev_register(&pmotor_sd->motor_sd, pmotor_sd->cam_id);
	if (res) {
		LOG(LOG_ERR, "armcb_subdev_register  failed");
		goto EXIT_RET;
	}

EXIT_RET:
	return res;
}

#ifdef CIX_I2C_ACTUATOR
static const struct i2c_device_id armcb_motor_id[] = { { "motor,ms42919",
							 0x36 },
							   {} };
MODULE_DEVICE_TABLE(i2c, armcb_motor_id);
#else
static const struct spi_device_id armcb_motor_id[] = { { "motor,ms42919", 0 },
							   {} };
MODULE_DEVICE_TABLE(spi, armcb_motor_id);
#endif
static const struct of_device_id armcb_motor_of_match[] = {
	{ .compatible = "motor,ms42919" },
	{},
};
MODULE_DEVICE_TABLE(of, armcb_motor_of_match);

static const struct acpi_device_id armcb_motor_acpi_match[] = {
	{ .id = "CIXH3023", .driver_data = 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, armcb_motor_acpi_match);

#ifdef CIX_I2C_ACTUATOR
static int motor0_probe (struct i2c_client *pcline_dev,
	const struct i2c_device_id *id)
#else
static int motor0_probe(struct spi_device *pspi_dev)
#endif
{
	struct armcb_motor_subdev *pmotor_sd = NULL;
	struct device *dev = NULL;
	struct fwnode_handle *node = NULL;
	int res = 0;

#ifdef CIX_I2C_ACTUATOR
	if (WARN_ON(!pcline_dev)) {
		LOG(LOG_ERR, "armcb motor i2c is NULL");
		return -EINVAL;
	}
#else
	if (WARN_ON(!pspi_dev)) {
		LOG(LOG_ERR, "armcb motor spi is NULL");
		return -EINVAL;
	}
#endif

#ifdef CIX_I2C_ACTUATOR
	dev = &pcline_dev->dev;
#else
	dev = &pspi_dev->dev;
#endif
	if (WARN_ON(!dev)) {
		LOG(LOG_ERR, "armcb motor dev is NULL");
		return -EINVAL;
	}

#ifdef CIX_I2C_ACTUATOR
	node = pcline_dev->dev.fwnode;
#else
	node = pspi_dev->dev.fwnode;
#endif
	if (WARN_ON(!node)) {
		LOG(LOG_ERR, "armcb motor node is NULL");
		return -EINVAL;
	}

	if (has_acpi_companion(dev)) {
		if (!acpi_match_device(armcb_motor_acpi_match, dev)) {
			LOG(LOG_ERR, "armcb motor acpi_match_device failed");
			return -EINVAL;
		}
	} else {
		if (!of_match_device(armcb_motor_of_match, dev)) {
			LOG(LOG_ERR, "armcb motor of_match_device failed");
			return -EINVAL;
		}
	}

	/* Allocate subdev driver data */
	pmotor_sd = kzalloc(sizeof(*pmotor_sd), GFP_KERNEL);
	if (!pmotor_sd) {
#ifdef CIX_I2C_ACTUATOR
		LOG(LOG_ERR, "i2cmotor kzalloc failed");
#else
		LOG(LOG_ERR, "spimotor kzalloc failed");
#endif
		return -ENOMEM;
	}

	/* Initialize the driver data */
#ifdef CIX_I2C_ACTUATOR
	pmotor_sd->pcline_dev = pcline_dev;
#else
	pmotor_sd->pspi_dev = pspi_dev;
#endif
	spin_lock_init(&pmotor_sd->sdlock);
	mutex_init(&pmotor_sd->imutex);

#ifndef CIX_I2C_ACTUATOR
	dev_info(dev, "spi motor speed_hz(%d)", pspi_dev->max_speed_hz);
#endif
	res = armcb_motor_configure_subdevs(pmotor_sd);
	if (res < 0) {
		LOG(LOG_ERR, "armcb_imgsens_configure_subdevs failed res(%d)",
			res);
		goto EXIT_RET;
	}

#ifdef CIX_I2C_ACTUATOR
	i2c_set_clientdata(pcline_dev, (void *)pmotor_sd);

	LOG(LOG_INFO, "motor probe scuess , pcline_dev = %p ", pcline_dev);
#else
	spi_set_drvdata(pspi_dev, pmotor_sd);

	LOG(LOG_INFO, "motor probe scuess , p_spi_dev = %p ", pspi_dev);
#endif
	return res;

EXIT_RET:
	kfree(pmotor_sd);
	LOG(LOG_ERR, "motor probe failed. res = %d", res);

	return res;
}

#ifdef CIX_I2C_ACTUATOR
void motor0_remove(struct i2c_client *pcline_dev)
#else
void motor0_remove(struct spi_device *pspi_dev)
#endif
{
#ifdef CIX_I2C_ACTUATOR
	struct armcb_motor_subdev *pmotor_sd = i2c_get_clientdata(pcline_dev);

	if (!pmotor_sd) {
		LOG(LOG_ERR, "i2c_get_clientdata failed");
		return;
	}
#else
	struct armcb_motor_subdev *pmotor_sd = spi_get_drvdata(pspi_dev);

	if (!pmotor_sd) {
		LOG(LOG_ERR, "spi_get_drvdata failed");
		return;
	}
#endif
	// the async_list will be added to subdev_list global field again, delete it
	// twice
	list_del_init(&pmotor_sd->motor_sd.sd.async_list);
#if (KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE)
	v4l2_device_unregister_subdev(&pmotor_sd->motor_sd.sd);
#endif

	kfree(pmotor_sd);
}

#ifdef CIX_I2C_ACTUATOR
static struct i2c_driver armcb_motor_i2c_driver = {
#else
static struct spi_driver armcb_motor_spi_driver = {
#endif
	.driver = {
		.name = "motor,ms42919",
		.of_match_table = of_match_ptr(armcb_motor_of_match),
		.acpi_match_table = ACPI_PTR(armcb_motor_acpi_match),
	},
	.probe    = motor0_probe,
	.remove   = motor0_remove,
	.id_table = armcb_motor_id,
};

#ifndef ARMCB_CAM_KO
static int __init armcb_motor_subdev_init(void)
{
	int res = 0;

	LOG(LOG_INFO, "+");
#ifdef CIX_I2C_ACTUATOR
	res = i2c_add_driver(&armcb_motor_i2c_driver);
	if (res)
		LOG(LOG_ERR, "armcb_motor_i2c_driver failed res:%d", res);
#else
	res = spi_register_driver(&armcb_motor_spi_driver);
	if (res)
		LOG(LOG_ERR, "armcb_motor_spi_driver failed res:%d", res);
#endif
	LOG(LOG_INFO, "res:%d -", res);

	return res;
}

static void __exit armcb_motor_subdev_exit(void)
{
#ifdef CIX_I2C_ACTUATOR
	i2c_del_driver(&armcb_motor_i2c_driver);
#else
	spi_unregister_driver(&armcb_motor_spi_driver);
#endif
}

late_initcall(armcb_motor_subdev_init);
module_exit(armcb_motor_subdev_exit);

MODULE_AUTHOR("Armchina Inc.");
MODULE_DESCRIPTION("Armcb motor sensor subdev driver");
MODULE_LICENSE("GPL v2");
#else
static void *g_instance;

void *armcb_get_motor_driver_instance(void)
{
#ifdef CIX_I2C_ACTUATOR
	if (i2c_add_driver(&armcb_motor_i2c_driver) < 0) {
#else
	if (spi_register_driver(&armcb_motor_spi_driver) < 0) {
#endif
		LOG(LOG_ERR, "register spi motor driver failed.\n");
		return NULL;
	}

#ifdef CIX_I2C_ACTUATOR
	g_instance = (void *)&armcb_motor_i2c_driver;
#else
	g_instance = (void *)&armcb_motor_spi_driver;
#endif

	return g_instance;
}

void armcb_motor_driver_destroy(void)
{
	if (g_instance) {
#ifdef CIX_I2C_ACTUATOR
		i2c_del_driver((struct i2c_driver *)g_instance);
#else
		spi_unregister_driver((struct spi_driver *)g_instance);
#endif
	}
}
#endif

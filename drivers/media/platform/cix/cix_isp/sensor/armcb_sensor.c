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

#include "armcb_sensor.h"
#include "armcb_isp.h"
#include "armcb_v4l2_core.h"
#include "isp_hw_ops.h"
#include "system_logger.h"
#include <linux/v4l2-controls.h>
#include <media/media-device.h>
#include <media/v4l2-async.h>

#include <linux/gpio/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/reset.h>

#ifdef LOG_MODULE
#undef LOG_MODULE
#define LOG_MODULE LOG_MODULE_SENSOR
#endif

static int camera_common_powen(struct armcb_imgsens_subdev *pimgsens_sd)
{
	int ret = 0;
	struct device *dev = pimgsens_sd->pdev;

	if (pimgsens_sd->vsupply_0)
		ret = regulator_enable(pimgsens_sd->vsupply_0);
	if (pimgsens_sd->vsupply_1)
		ret = regulator_enable(pimgsens_sd->vsupply_1);

	if (ret)
		dev_err(dev, "fail to enable cam_v regulator\n");

	pimgsens_sd->power_on = TRUE;
	return ret;
}

static int camera_common_powdn(struct armcb_imgsens_subdev *pimgsens_sd)
{
	int ret = 0;
	struct device *dev = pimgsens_sd->pdev;

	if (pimgsens_sd->vsupply_0)
		ret = regulator_disable(pimgsens_sd->vsupply_0);
	if (pimgsens_sd->vsupply_1)
		ret = regulator_disable(pimgsens_sd->vsupply_1);

	if (ret)
		dev_err(dev, "fail to disable cam_v regulator\n");

	pimgsens_sd->power_on = FALSE;
	return ret;
}

static int armcb_camera_async_bound(struct v4l2_async_notifier *notifier,
					struct v4l2_subdev *sd,
					struct v4l2_async_subdev *asd)
{
	int ret = 0;
	return ret;
}

static void armcb_camera_async_unbind(struct v4l2_async_notifier *notifier,
					  struct v4l2_subdev *sd,
					  struct v4l2_async_subdev *asd)
{
}

static int
armcb_async_notifier_add_devname_subdev(struct v4l2_async_notifier *notifier,
					struct fwnode_handle *priv)
{
	struct v4l2_async_subdev *asd;
	int ret = 0;

#if (KERNEL_VERSION(4, 17, 0) <= LINUX_VERSION_CODE)
	struct fwnode_handle *fwnode = priv;

	asd = v4l2_async_nf_add_fwnode(notifier, fwnode,
					   struct v4l2_async_subdev);
	if (IS_ERR(asd)) {
		LOG(LOG_ERR, "fail to add asd = %p\n", asd);
		return PTR_ERR(asd);
	}

	fwnode_handle_put(priv);
#else
	asd = kzalloc(sizeof(*asd), GFP_KERNEL);
	if (!asd) {
		LOG(LOG_ERR, "alloc aysnc node failed\n");
		return -ENOMEM;
	}
	if (notifier->num_subdevs >= MAX_SUBDEV_NUM) {
		LOG(LOG_ERR, "too many subdevs\n");
		kfree(asd);
		return -EINVAL;
	}
	notifier->subdevs[notifier->num_subdevs] = asd;
	notifier->num_subdevs++;
#endif
	return ret;
}

static int armcb_register_async_node(struct device *pdev,
					 struct v4l2_async_notifier *notifier,
					 bool is_sensor)
{
	int ret = 0;
	struct fwnode_handle *parent_node = pdev->fwnode;
	struct fwnode_handle *actuator_node =
		fwnode_find_reference(parent_node, "actuator-src", 0);
	struct fwnode_handle *isp_node =
		fwnode_find_reference(parent_node, "isp-src", 0);
#if (KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE)
	struct v4l2_async_subdev **subdevs = NULL;

	subdevs = devm_kzalloc(pdev, sizeof(*subdevs) * MAX_SUBDEV_NUM,
				   GFP_KERNEL);
	if (subdevs == NULL) {
		LOG(LOG_ERR, "alloc async subdev list fail\n");
		return -ENOMEM;
	}
	notifier->subdevs = subdevs;
	notifier->num_subdevs = 0;
#endif

	// add sensor subdev
	if (!IS_ERR_OR_NULL(parent_node) && is_sensor) {
		(void)armcb_async_notifier_add_devname_subdev(notifier,
								  parent_node);
	}
	// add actuator subdev
	if (!IS_ERR(actuator_node)) {
		(void)armcb_async_notifier_add_devname_subdev(notifier,
								  actuator_node);
	}
	if (!IS_ERR(isp_node)) {
		(void)armcb_async_notifier_add_devname_subdev(notifier,
								  isp_node);
	}
	return ret;
}

int armcb_imgsens_hw_apply(struct cmd_buf *cmd,
			   struct armcb_imgsens_subdev *pimgsens_sd)
{
	int ret = 0;

	switch (cmd->settings.ahb_power->reg_addr) {
	case IMGS_POWER_AVDD:
		LOG(LOG_ERR, "avdd no resource");
		break;
	case IMGS_POWER_VIO:
		LOG(LOG_ERR, "VIO no resource");
		break;
	case IMGS_POWER_DVDD:
		LOG(LOG_ERR, "Dvdd no resource");
		break;
	case IMGS_POWER_ENABLE:
		if (cmd->settings.ahb_power->bit_mask)
			camera_common_powen(pimgsens_sd);
		else
			camera_common_powdn(pimgsens_sd);
		break;
	case IMGS_POWER_REST:
		LOG(LOG_INFO, "rst_gpio val is: %d\n",
			cmd->settings.ahb_power->bit_mask);
		gpiod_set_value_cansleep(pimgsens_sd->imgs_inst.rst_gpio,
					 cmd->settings.ahb_power->bit_mask);
		break;
	case IMGS_POWER_PWDN:
		LOG(LOG_INFO, "pwn_gpio val is: %d",
			cmd->settings.ahb_power->bit_mask);
		gpiod_set_value_cansleep(pimgsens_sd->imgs_inst.pwn_gpio,
					 cmd->settings.ahb_power->bit_mask);
		break;
	case IMGS_POWER_MCLK:
		LOG(LOG_INFO, "MCLK val is: %d\n",
			cmd->settings.ahb_power->bit_mask);
		if (cmd->settings.ahb_power->bit_mask)
			ret = clk_prepare_enable(pimgsens_sd->imgs_inst.mclk);
		else
			clk_disable_unprepare(pimgsens_sd->imgs_inst.mclk);
		break;
	default:
		LOG(LOG_ERR, "reg_addr is error");
		break;
	}
	msleep((cmd->settings.ahb_power->delay_us) / 1000);
	return ret;
}

static long armcb_imgsens_subdev_ioctl(struct v4l2_subdev *sd, unsigned int cmd,
					   void *arg)
{
	struct armcb_imgsens_subdev *pimgsens_sd = v4l2_get_subdevdata(sd);
	struct i2c_client *client = pimgsens_sd->imgs_inst.pi2c_client;
	int res = 0;

	switch (cmd) {
	case ARMCB_VIDIOC_APPLY_CMD: {
		struct cmd_buf *pcmd_buf = (struct cmd_buf *)arg;

		switch (pcmd_buf->static_info.bus) {
		case HW_BUS_AHB_POWER:
			res |= armcb_imgsens_hw_apply(pcmd_buf, pimgsens_sd);
			break;
		default:
			res |= armcb_isp_hw_apply(pcmd_buf, client);
			break;
		}
		break;
	}
	default:
		return -ENOIOCTLCMD;
	}
	return res;
}

static const struct v4l2_subdev_core_ops armcb_imgsens_subdev_core_ops = {
	.ioctl = &armcb_imgsens_subdev_ioctl,
};

static struct v4l2_subdev_ops armcb_imgsens_subdev_ops = {
	.core = &armcb_imgsens_subdev_core_ops,
};

static int armcb_imgsens_subdev_open(struct v4l2_subdev *sd,
					 struct v4l2_subdev_fh *fh)
{
	int ret = 0;
	/* empty function */
	return ret;
}

static int armcb_imgsens_subdev_close(struct v4l2_subdev *sd,
					  struct v4l2_subdev_fh *fh)
{
	int ret = 0;
	/* empty function */
	return ret;
}

static const struct v4l2_subdev_internal_ops armcb_imgsens_sd_internal_ops = {
	.open = armcb_imgsens_subdev_open,
	.close = armcb_imgsens_subdev_close,
};

static int
armcb_imgsens_configure_subdevs(struct armcb_imgsens_subdev *pimgsens_sd)
{
	struct v4l2_subdev *sd = &pimgsens_sd->imgsens_sd.sd;
	struct i2c_client *client = pimgsens_sd->imgs_inst.pi2c_client;
	int res = 0;

	v4l2_i2c_subdev_init(sd, client, &armcb_imgsens_subdev_ops);
	sd->owner = THIS_MODULE;
	v4l2_set_subdevdata(sd, pimgsens_sd);
	snprintf(sd->name, sizeof(sd->name), "sensor0");

	sd->internal_ops = &armcb_imgsens_sd_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->entity.function = MEDIA_ENT_F_V4L2_SUBDEV_UNKNOWN;
	sd->entity.function = ARMCB_CAMERA_SUBDEV_IMGSENS;
	sd->entity.name = sd->name;
	sd->grp_id = ARMCB_SUBDEV_NODE_HW_IMGSENS0;

	res = media_entity_pads_init(&sd->entity, 0, NULL);
	if (res) {
		LOG(LOG_ERR, "media_entity_pads_init  failed");
		goto EXIT_RET;
	}

	pimgsens_sd->imgsens_sd.close_seq = ARMCB_SD_CLOSE_2ND_CATEGORY | 0x1;
	res = armcb_subdev_register(&pimgsens_sd->imgsens_sd,
					pimgsens_sd->cam_id);
	if (res) {
		LOG(LOG_ERR, "armcb_subdev_register  failed");
		goto EXIT_RET;
	}

EXIT_RET:
	return res;
}

static const struct i2c_device_id armcb_imgsens_id[] = {
	{ IMGSENS_I2C_DRVNAME, 0x34 },
	{},
};
MODULE_DEVICE_TABLE(i2c, armcb_imgsens_id);

static const struct of_device_id armcb_imgsens_of_match[] = {
	{ .compatible = "image,sensor0" },
	{},
};
MODULE_DEVICE_TABLE(of, armcb_imgsens_of_match);

static const struct acpi_device_id armcb_imgsens_acpi_match[] = {
	{ .id = "CIXH3024", .driver_data = 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, armcb_imgsens_acpi_match);

#if (KERNEL_VERSION(4, 17, 0) <= LINUX_VERSION_CODE)
static const struct v4l2_async_notifier_operations async_notifier_ops = {
	.bound = armcb_camera_async_bound,
	.complete = armcb_camera_async_complete,
	.unbind = armcb_camera_async_unbind,
};
#endif

#ifdef QEMU_ON_VEXPRESS
static int imgsens_probe(struct platform_device *pdev)
{
	int res = 0;
	armcb_v4l2_dev_t *adev = NULL;
	u32 cam_id = 0;
	struct device_node *node = &pdeb->dev->of_node;

	LOG(LOG_INFO, "+");
	res = of_property_read_u32(node, CIX_CAMERA_MODULE_INDEX, &cam_id);
	adev = armcb_register_instance(NULL, &pdev->dev, cam_id);
	if (adev == NULL) {
		LOG(LOG_ERR, "create armcb_dev instance fail");
		return -ENOMEM;
	}

	LOG(LOG_INFO, "-");

	return 0;
}

static int imgsens_remove(struct platform_device *pdev)
{
	armcb_cam_instance_destroy();
	return 0;
}
#else
static int imgsens_probe(struct i2c_client    *client,
	const struct i2c_device_id *id)
{
	struct armcb_imgsens_subdev *pimgsens_sd = NULL;
	struct armcb_camera_inst *pimgs_inst = NULL;
	struct device *dev = NULL;
	struct fwnode_handle *node = NULL;
	armcb_v4l2_dev_t *adev = NULL;
	int res = 0;
	unsigned int cam_id = 0;

	LOG(LOG_INFO, "+");

	if (!client) {
		LOG(LOG_ERR, "armcb imgsens client is NULL");
		return -EINVAL;
	} else {
		LOG(LOG_INFO, "imgsens   client(%p)", client);
	}

	if (!id) {
		LOG(LOG_ERR, "armcb imgsens id is NULL");
		id = armcb_imgsens_id;
	}

	dev = &client->dev;
	node = client->dev.fwnode;
	res = of_property_read_u32(dev->of_node, CIX_CAMERA_MODULE_INDEX,
				   &cam_id);

	if (WARN_ON(!dev) || WARN_ON(!node)) {
		LOG(LOG_ERR, "armcb imgsens dev/node is NULL");
		return -EINVAL;
	}

	adev = armcb_register_instance(NULL, dev, cam_id);
	if (adev == NULL) {
		LOG(LOG_ERR, "create armcb_dev instance fail");
		return -ENOMEM;
	}

	LOG(LOG_INFO, "start configure notifier.");
#if (KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE)
	adev->dts_notifier.bound = armcb_camera_async_bound;
	adev->dts_notifier.complete = armcb_camera_async_complete;
	adev->dts_notifier.unbind = armcb_camera_async_unbind;
#else
	v4l2_async_nf_init(&adev->dts_notifier);
	adev->dts_notifier.ops = &async_notifier_ops;
#endif

	// register async node to notifier first
	res = armcb_register_async_node(&client->dev, &adev->dts_notifier,
					true);
	if (res < 0) {
		LOG(LOG_ERR, "attach async node to register failed\n");
		return -EINVAL;
	}

	res = v4l2_async_nf_register(&adev->v4l2_dev, &adev->dts_notifier);
	if (res < 0) {
		LOG(LOG_ERR, "register async notifier failed\n");
		return -EINVAL;
	}

	pimgsens_sd = kzalloc(sizeof(*pimgsens_sd), GFP_KERNEL);
	if (!pimgsens_sd) {
		LOG(LOG_ERR, "armcb imgsens kzalloc failed");
		return -ENOMEM;
	}
	pimgs_inst = &pimgsens_sd->imgs_inst;
	pimgs_inst->pi2c_client = client;

	pimgsens_sd->of_node = client->dev.of_node;
	pimgsens_sd->pdev = &client->dev;
	pimgsens_sd->cam_id = cam_id;

	res = armcb_imgsens_configure_subdevs(pimgsens_sd);
	if (res < 0) {
		LOG(LOG_ERR, "armcb_imgsens_configure_subdevs failed res(%d)",
			res);
		goto EXIT_RET;
	}
	pimgsens_sd->armcb_v4l2_dev = adev;
	i2c_set_clientdata(client, (void *)pimgsens_sd);

	pimgsens_sd->imgs_inst.pinctrl = devm_pinctrl_get(&client->dev);
	if (IS_ERR(pimgsens_sd->imgs_inst.pinctrl))
		LOG(LOG_ERR, "can't get pinctrl, bus recovery not supported\n");

	pimgsens_sd->imgs_inst.pins_default = pinctrl_lookup_state(
		pimgsens_sd->imgs_inst.pinctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(pimgsens_sd->imgs_inst.pins_default))
		LOG(LOG_ERR,
			"can't get pins_default, bus recovery not supported\n");

	pimgsens_sd->imgs_inst.pins_gpio =
		pinctrl_lookup_state(pimgsens_sd->imgs_inst.pinctrl, "gpio");

	if (IS_ERR(pimgsens_sd->imgs_inst.pins_gpio))
		LOG(LOG_ERR,
			"can't get pins_gpio, bus recovery not supported\n");

	pinctrl_select_state(pimgsens_sd->imgs_inst.pinctrl,
				 pimgsens_sd->imgs_inst.pins_gpio);

	pimgsens_sd->imgs_inst.pwn_gpio =
		devm_gpiod_get(&client->dev, "pwdn", GPIOD_OUT_LOW);
	if (PTR_ERR(pimgsens_sd->imgs_inst.pwn_gpio) == -EPROBE_DEFER ||
		PTR_ERR(pimgsens_sd->imgs_inst.pwn_gpio) == -EPROBE_DEFER)
		LOG(LOG_ERR,
			"can't get pwn_gpio pinctrl, bus recovery not supported\n");
	pimgsens_sd->imgs_inst.rst_gpio =
		devm_gpiod_get(&client->dev, "reset", GPIOD_OUT_LOW);
	if (PTR_ERR(pimgsens_sd->imgs_inst.rst_gpio) == -EPROBE_DEFER ||
		PTR_ERR(pimgsens_sd->imgs_inst.rst_gpio) == -EPROBE_DEFER)
		LOG(LOG_ERR,
			"can't get rst_gpio pinctrl, bus recovery not supported\n");

	pimgsens_sd->vsupply_0 =
		devm_regulator_get_optional(&client->dev, "power0");
	if (IS_ERR(pimgsens_sd->vsupply_0)) {
		dev_info(dev, "no power regulator vsupply_0 found\n");
		pimgsens_sd->vsupply_0 = NULL;
	}

	pimgsens_sd->vsupply_1 =
		devm_regulator_get_optional(&client->dev, "power1");
	if (IS_ERR(pimgsens_sd->vsupply_1)) {
		dev_info(dev, "no power regulator vsupply_1 found\n");
		pimgsens_sd->vsupply_1 = NULL;
	}

	pimgsens_sd->imgs_inst.mclk =
		devm_clk_get_optional(&client->dev, "mclk");
	if (IS_ERR(pimgsens_sd->imgs_inst.mclk))
		LOG(LOG_ERR, "failed to get cam mclk\n");

	LOG(LOG_INFO, "scuess - ");
	return res;

EXIT_RET:
	kfree(pimgsens_sd);
	LOG(LOG_INFO, "failed - ");
	return res;
}

void imgsens_remove(struct i2c_client *client)
{
	struct armcb_imgsens_subdev *pimgsens_sd = i2c_get_clientdata(client);

	if (pimgsens_sd) {
		if (pimgsens_sd->power_on) {
			if (pimgsens_sd->vsupply_0)
				regulator_disable(pimgsens_sd->vsupply_0);
			if (pimgsens_sd->vsupply_1)
				regulator_disable(pimgsens_sd->vsupply_1);
			LOG(LOG_INFO, "disable camera power");
			pimgsens_sd->power_on = 0;
		}

		list_del_init(&pimgsens_sd->imgsens_sd.sd.async_list);
#if (KERNEL_VERSION(4, 17, 0) > LINUX_VERSION_CODE)
		v4l2_device_unregister_subdev(&pimgsens_sd->imgsens_sd.sd);
#endif
		kfree(pimgsens_sd);
	} else {
		LOG(LOG_ERR, "pimgsens_sd is NULL !");
	}
}
#endif

#ifdef QEMU_ON_VEXPRESS
static struct platform_driver armcb_imgsens_i2c_driver = {
	.driver = {
		.name = IMGSENS_I2C_DRVNAME,
		.of_match_table = of_match_ptr(armcb_imgsens_of_match),
	},
	.probe = imgsens_probe,
	.remove = imgsens_remove,
};

static int __init armcb_imgsens_subdev_init(void)
{
	return platform_driver_register(&armcb_imgsens_i2c_driver);
}

static void __exit armcb_imgsens_subdev_exit(void)
{
	platform_driver_unregister(&armcb_imgsens_i2c_driver);
}

#else

static int imgsens_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct armcb_imgsens_subdev *pimgsens_sd = i2c_get_clientdata(client);

	pinctrl_select_state(pimgsens_sd->imgs_inst.pinctrl,
				 pimgsens_sd->imgs_inst.pins_gpio);

	return 0;
}

static int imgsens_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct armcb_imgsens_subdev *pimgsens_sd = i2c_get_clientdata(client);

	pinctrl_select_state(pimgsens_sd->imgs_inst.pinctrl,
				 pimgsens_sd->imgs_inst.pins_default);

	return 0;
}

static const struct dev_pm_ops imgsens_pm_ops = {
	.suspend = imgsens_suspend,
	.resume = imgsens_resume,
};

static struct i2c_driver armcb_imgsens_i2c_driver = {
	.driver = {
		.name = IMGSENS_I2C_DRVNAME,
		.pm = &imgsens_pm_ops,
		.of_match_table = of_match_ptr(armcb_imgsens_of_match),
		.acpi_match_table = ACPI_PTR(armcb_imgsens_acpi_match),
	},
	.probe    = imgsens_probe,
	.remove   = imgsens_remove,
	.id_table = armcb_imgsens_id,
};

#ifndef ARMCB_CAM_KO
static int __init armcb_imgsens_subdev_init(void)
{
	int res = 0;

	res = i2c_add_driver(&armcb_imgsens_i2c_driver);
	if (res)
		LOG(LOG_ERR, "i2c_add_driver failed res(%d)", res);

	return res;
}

static void __exit armcb_imgsens_subdev_exit(void)
{
	i2c_del_driver(&armcb_imgsens_i2c_driver);
}
#endif
#endif

#ifndef ARMCB_CAM_KO
late_initcall(armcb_imgsens_subdev_init);
module_exit(armcb_imgsens_subdev_exit);

MODULE_AUTHOR("Armchina Inc.");
MODULE_DESCRIPTION("Armchina image sensor subdev driver");
MODULE_LICENSE("GPL v2");
#else
static void *g_instance;

void *armcb_get_sensor_driver_instance(void)
{
	if (i2c_add_driver(&armcb_imgsens_i2c_driver) < 0) {
		LOG(LOG_ERR, "register spi motor driver failed.\n");
		return NULL;
	}
	g_instance = (void *)&armcb_imgsens_i2c_driver;
	return g_instance;
}

void armcb_sensor_driver_destroy(void)
{
	if (g_instance)
		i2c_del_driver((struct i2c_driver *)g_instance);
}
#endif

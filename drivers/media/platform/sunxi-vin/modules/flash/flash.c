/*
 *****************************************************************************
 *
 * sunxi_flash.c
 *
 * Hawkview ISP - sunxi_flash.c module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version        Author                 Date                   Description
 *
 *   3.0                  Zhao Wei      2015/02/27  ISP Tuning Tools Support
 *
 *****************************************************************************
 */
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>
#include <media/sunxi_camera_v2.h>

#include "../../platform/platform_cfg.h"
#include "../../vin-video/vin_core.h"

#include "flash.h"

static LIST_HEAD(flash_drv_list);

#define FLASH_MODULE_NAME "vin_flash"

#define FLASH_EN_POL 1
#define FLASH_MODE_POL 1

static int flash_power_flag;

int io_set_flash_ctrl(struct v4l2_subdev *sd, enum sunxi_flash_ctrl ctrl)
{
	int ret = 0;
	unsigned int flash_en, flash_dis, flash_mode, torch_mode;
	struct flash_dev_info *fls_info = NULL;
	struct flash_dev *flash = (sd == NULL) ? NULL : v4l2_get_subdevdata(sd);

	if (!flash)
		return 0;

	fls_info = &flash->fl_info;

	flash_en = (fls_info->en_pol != 0) ? 1 : 0;
	flash_dis = !flash_en;
	flash_mode = (fls_info->fl_mode_pol != 0) ? 1 : 0;
	torch_mode = !flash_mode;

	if (FLASH_RELATING == fls_info->flash_driver_ic) {
		switch (ctrl) {
		case SW_CTRL_FLASH_OFF:
			vin_log(VIN_LOG_FLASH, "FLASH_RELATING SW_CTRL_FLASH_OFF\n");
			vin_gpio_set_status(sd, FLASH_EN, 1);
			vin_gpio_set_status(sd, FLASH_MODE, 1);
			ret |= vin_gpio_write(sd, FLASH_EN, flash_dis);
			ret |= vin_gpio_write(sd, FLASH_MODE, torch_mode);
			break;
		case SW_CTRL_FLASH_ON:
			vin_log(VIN_LOG_FLASH, "FLASH_RELATING SW_CTRL_FLASH_ON\n");
			vin_gpio_set_status(sd, FLASH_EN, 1);
			vin_gpio_set_status(sd, FLASH_MODE, 1);
			ret |= vin_gpio_write(sd, FLASH_MODE, flash_mode);
			ret |= vin_gpio_write(sd, FLASH_EN, flash_en);
			break;
		case SW_CTRL_TORCH_ON:
			vin_log(VIN_LOG_FLASH, "FLASH_RELATING SW_CTRL_TORCH_ON\n");
			vin_gpio_set_status(sd, FLASH_EN, 1);
			vin_gpio_set_status(sd, FLASH_MODE, 1);
			ret |= vin_gpio_write(sd, FLASH_MODE, torch_mode);
			ret |= vin_gpio_write(sd, FLASH_EN, flash_en);
			break;
		default:
			return -EINVAL;
		}
	} else if (FLASH_EN_INDEPEND == fls_info->flash_driver_ic) {
		switch (ctrl) {
		case SW_CTRL_FLASH_OFF:
			vin_log(VIN_LOG_FLASH, "FLASH_EN_INDEPEND SW_CTRL_FLASH_OFF\n");
			vin_gpio_set_status(sd, FLASH_EN, 1);
			vin_gpio_set_status(sd, FLASH_MODE, 1);
			ret |= vin_gpio_write(sd, FLASH_EN, 0);
			ret |= vin_gpio_write(sd, FLASH_MODE, 0);
			break;
		case SW_CTRL_FLASH_ON:
			vin_log(VIN_LOG_FLASH, "FLASH_EN_INDEPEND SW_CTRL_FLASH_ON\n");
			vin_gpio_set_status(sd, FLASH_EN, 1);
			vin_gpio_set_status(sd, FLASH_MODE, 1);

			ret |= vin_gpio_write(sd, FLASH_MODE, 1);
			ret |= vin_gpio_write(sd, FLASH_EN, 0);
			break;
		case SW_CTRL_TORCH_ON:
			vin_log(VIN_LOG_FLASH, "FLASH_EN_INDEPEND SW_CTRL_TORCH_ON\n");
			vin_gpio_set_status(sd, FLASH_EN, 1);
			vin_gpio_set_status(sd, FLASH_MODE, 1);
			ret |= vin_gpio_write(sd, FLASH_MODE, 0);
			ret |= vin_gpio_write(sd, FLASH_EN, 1);
			break;
		default:
			return -EINVAL;
		}
	} else {
		switch (ctrl) {
		case SW_CTRL_FLASH_OFF:
			vin_log(VIN_LOG_FLASH, "FLASH_POWER SW_CTRL_FLASH_OFF\n");
			if (1 == flash_power_flag) {
				vin_set_pmu_channel(sd, FLVDD, OFF);
				flash_power_flag--;
			}
			break;
		case SW_CTRL_FLASH_ON:
			vin_log(VIN_LOG_FLASH, "FLASH_POWER SW_CTRL_FLASH_ON\n");
			if (0 == flash_power_flag) {
				vin_set_pmu_channel(sd, FLVDD, ON);
				flash_power_flag++;
			}
			break;
		case SW_CTRL_TORCH_ON:
			vin_log(VIN_LOG_FLASH, "FLASH_POWER SW_CTRL_TORCH_ON\n");
			if (0 == flash_power_flag) {
				vin_set_pmu_channel(sd, FLVDD, ON);
				flash_power_flag++;
			}
			break;
		default:
			return -EINVAL;
		}
	}
	if (ret != 0) {
		vin_log(VIN_LOG_FLASH, "flash set ctrl fail, force shut off\n");
		ret |= vin_gpio_write(sd, FLASH_EN, flash_dis);
		ret |= vin_gpio_write(sd, FLASH_MODE, torch_mode);
	}
	return ret;
}

int sunxi_flash_check_to_start(struct v4l2_subdev *sd,
			       enum sunxi_flash_ctrl ctrl)
{
	struct vin_core *vinc = (sd == NULL) ? NULL : sd_to_vin_core(sd);
	struct flash_dev *flash = (sd == NULL) ? NULL : v4l2_get_subdevdata(sd);
	unsigned int flag, to_flash;

	if (!flash)
		return 0;

	if (flash->fl_info.flash_mode == V4L2_FLASH_LED_MODE_FLASH) {
		to_flash = 1;
	} else if (flash->fl_info.flash_mode == V4L2_FLASH_LED_MODE_AUTO) {
		v4l2_subdev_call(vinc->vid_cap.pipe.sd[VIN_IND_SENSOR], core,
				 ioctl, GET_FLASH_FLAG, &flag);
		if (flag)
			to_flash = 1;
		else
			to_flash = 0;
	} else {
		to_flash = 0;
	}

	if (to_flash)
		io_set_flash_ctrl(sd, ctrl);

	return 0;
}

int sunxi_flash_stop(struct v4l2_subdev *sd)
{
	struct flash_dev *flash = (sd == NULL) ? NULL : v4l2_get_subdevdata(sd);

	if (!flash)
		return 0;

	if (flash->fl_info.flash_mode != V4L2_FLASH_LED_MODE_NONE)
		io_set_flash_ctrl(sd, SW_CTRL_FLASH_OFF);
	return 0;
}

static int config_flash_mode(struct v4l2_subdev *sd,
			     enum v4l2_flash_led_mode mode,
			     struct flash_dev_info *fls_info)
{
	if (fls_info == NULL) {
		vin_err("camera flash not support!\n");
		return -1;
	}
	if ((fls_info->light_src != 0x01) && (fls_info->light_src != 0x02)
	    && (fls_info->light_src != 0x10)) {
		vin_err("unsupported light source, force LEDx1\n");
		fls_info->light_src = 0x01;
	}
	fls_info->flash_mode = mode;
	if (mode == V4L2_FLASH_LED_MODE_TORCH) {
		io_set_flash_ctrl(sd, SW_CTRL_TORCH_ON);
	} else if (mode == V4L2_FLASH_LED_MODE_NONE) {
		io_set_flash_ctrl(sd, SW_CTRL_FLASH_OFF);
	}
	return 0;
}

static int sunxi_flash_queryctrl(struct v4l2_subdev *sd,
				 struct v4l2_queryctrl *qc)
{
	switch (qc->id) {
	case V4L2_CID_FLASH_LED_MODE:
		return v4l2_ctrl_query_fill(qc, 0, 5, 1, 0);
	default:
		break;
	}
	return -EINVAL;
}

static int sunxi_flash_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct flash_dev *flash = (sd == NULL) ? NULL : v4l2_get_subdevdata(sd);

	if (!flash)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_FLASH_LED_MODE:
		ctrl->value = flash->fl_info.flash_mode;
		return 0;
	default:
		break;
	}
	return -EINVAL;
}

static int sunxi_flash_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;
	struct flash_dev *flash = (sd == NULL) ? NULL : v4l2_get_subdevdata(sd);

	if (!flash)
		return 0;

	qc.id = ctrl->id;
	ret = sunxi_flash_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}
	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		return -ERANGE;
	}

	switch (ctrl->id) {
	case V4L2_CID_FLASH_LED_MODE:
		return config_flash_mode(sd, ctrl->value, &flash->fl_info);
	default:
		break;
	}
	return -EINVAL;
}
static const struct v4l2_subdev_core_ops sunxi_flash_core_ops = {
	.g_ctrl = sunxi_flash_g_ctrl,
	.s_ctrl = sunxi_flash_s_ctrl,
};

static struct v4l2_subdev_ops sunxi_flash_subdev_ops = {
	.core = &sunxi_flash_core_ops,
};

static int sunxi_flash_subdev_init(struct flash_dev *flash)
{
	struct v4l2_subdev *sd = &flash->subdev;
	v4l2_subdev_init(sd, &sunxi_flash_subdev_ops);
	snprintf(sd->name, sizeof(sd->name), "sunxi_flash.%d", flash->id);

	flash->fl_info.dev_if = 0;
	flash->fl_info.en_pol = FLASH_EN_POL;
	flash->fl_info.fl_mode_pol = FLASH_MODE_POL;
	flash->fl_info.light_src = 0x01;
	flash->fl_info.flash_intensity = 400;
	flash->fl_info.flash_level = 0x01;
	flash->fl_info.torch_intensity = 200;
	flash->fl_info.torch_level = 0x01;
	flash->fl_info.timeout_counter = 300 * 1000;

	config_flash_mode(sd, V4L2_FLASH_LED_MODE_NONE, &flash->fl_info);

	v4l2_set_subdevdata(sd, flash);

	media_entity_init(&sd->entity, 0, NULL, 0);
	sd->entity.type = MEDIA_ENT_T_V4L2_SUBDEV_FLASH;

	return 0;
}

static int flash_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct flash_dev *flash = NULL;
	char property_name[32] = { 0 };
	int ret = 0;

	if (np == NULL) {
		vin_err("flash failed to get of node\n");
		return -ENODEV;
	}

	flash = kzalloc(sizeof(struct flash_dev), GFP_KERNEL);
	if (!flash) {
		ret = -ENOMEM;
		vin_err("sunxi flash kzalloc failed!\n");
		goto ekzalloc;
	}

	pdev->id = of_alias_get_id(np, "flash");
	if (pdev->id < 0) {
		vin_err("flash failed to get alias id\n");
		ret = -EINVAL;
		goto freedev;
	}
	sprintf(property_name, "flash%d_type", pdev->id);
	ret = of_property_read_u32(np, property_name,
				 &flash->fl_info.flash_driver_ic);
	if (ret) {
		flash->fl_info.flash_driver_ic = 0;
		vin_warn("fetch %s from device_tree failed\n", property_name);
		return -EINVAL;
	}

	flash->id = pdev->id;
	flash->pdev = pdev;
	list_add_tail(&flash->flash_list, &flash_drv_list);

	sunxi_flash_subdev_init(flash);
	platform_set_drvdata(pdev, flash);
	vin_print("flash%d probe end!\n", flash->id);

	return 0;
freedev:
	kfree(flash);
ekzalloc:
	vin_print("flash probe err!\n");
	return ret;
}

static int flash_remove(struct platform_device *pdev)
{
	struct flash_dev *flash = platform_get_drvdata(pdev);
	struct v4l2_subdev *sd = &flash->subdev;

	platform_set_drvdata(pdev, NULL);
	v4l2_device_unregister_subdev(sd);
	v4l2_set_subdevdata(sd, NULL);
	list_del(&flash->flash_list);

	kfree(flash);
	return 0;
}

static const struct of_device_id sunxi_flash_match[] = {
	{.compatible = "allwinner,sunxi-flash",},
	{},
};

static struct platform_driver flash_platform_driver = {
	.probe = flash_probe,
	.remove = flash_remove,
	.driver = {
		   .name = FLASH_MODULE_NAME,
		   .owner = THIS_MODULE,
		   .of_match_table = sunxi_flash_match,
		   }
};
struct v4l2_subdev *sunxi_flash_get_subdev(int id)
{
	struct flash_dev *flash;
	list_for_each_entry(flash, &flash_drv_list, flash_list) {
		if (flash->id == id) {
			flash->use_cnt++;
			return &flash->subdev;
		}
	}
	return NULL;
}

int sunxi_flash_platform_register(void)
{
	return platform_driver_register(&flash_platform_driver);
}

void sunxi_flash_platform_unregister(void)
{
	platform_driver_unregister(&flash_platform_driver);
	vin_print("flash_exit end\n");
}

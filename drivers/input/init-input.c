/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/irqdesc.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include "init-input.h"

bool sensor_has_inited_flag;
/***************************CTP************************************/

/**
 * sunxi_ctp_startup() - get config info from sys_config.fex file.
 *@ctp_type:  sensor type
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int sunxi_ctp_startup(enum input_sensor_type *ctp_type)
{
	int ret = -1;
	struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);
	struct device_node *np = NULL;
	struct i2c_client *client = NULL;
	struct platform_device *pdev = NULL;
	const char *idx;
	char ctp_name[12];

	np = data->node;

	if (!np) {
		if (data->np_name != NULL) {
			np = of_find_node_by_name(NULL, data->np_name);
		} else {
			np = of_find_node_by_name(NULL, "ctp");
		}
	}
	if (!np) {
		pr_err("ERROR! get ctp_para failed, func:%s, line:%d\n", __func__, __LINE__);
		goto devicetree_get_item_err;
	}

	if (!of_device_is_available(np)) {
		pr_err("%s: ctp is not used\n", __func__);
		goto devicetree_get_item_err;
	} else
		data->ctp_used = 1;

	client = of_find_i2c_device_by_node(np);
	if (client) {
		data->dev = &client->dev;
		data->isI2CClient = 1;
	} else {
		pdev = of_find_device_by_node(np);
		if (pdev)
			data->dev = &pdev->dev;
	}
	if (!data->dev) {
		pr_err("get device_node fail\n");
		return -1;
	}

	ret = of_property_read_u32(np, "ctp_twi_id", &data->twi_id);
	if (ret) {
		pr_err("get twi_id is fail, %d\n", ret);
		goto devicetree_get_item_err;
	}

	ret = of_property_read_string(np,  "ctp_fw_idx", &idx);
	if (ret)
		idx = 0;

	snprintf(ctp_name, 12, "ctp_name%s", idx);
	ret = of_property_read_string(np,  ctp_name, &data->name);
	if (ret)
		ret = of_property_read_string(np,  "ctp_name", &data->name);
	if (ret)
		pr_err("get ctp_name is fail, %d\n", ret);

	ret = of_property_read_string(np,  "ctp_power_ldo", &data->ctp_power);
	if (ret)
		pr_err("get ctp_power is fail, %d\n", ret);

	ret = of_property_read_u32(np, "ctp_power_ldo_vol",
							&data->ctp_power_vol);
	if (ret)
		pr_err("get ctp_power_ldo_vol is fail, %d\n", ret);

	data->ctp_power_io.gpio = of_get_named_gpio_flags(np, "ctp_power_io", 0,
				(enum of_gpio_flags *)(&(data->ctp_power_io)));
	if (!gpio_is_valid(data->ctp_power_io.gpio))
		pr_err("%s: ctp_power_io is invalid.\n", __func__);

	data->wakeup_gpio.gpio = of_get_named_gpio_flags(np, "ctp_wakeup", 0,
				(enum of_gpio_flags *)(&(data->wakeup_gpio)));
	if (!gpio_is_valid(data->wakeup_gpio.gpio))
		pr_err("%s: wakeup_gpio is invalid.\n", __func__);

	ret = of_property_read_u32(np, "ctp_screen_max_x", &data->screen_max_x);
	if (ret)
		pr_err("get ctp_screen_max_x is fail, %d\n", ret);

	ret = of_property_read_u32(np, "ctp_screen_max_y", &data->screen_max_y);
	if (ret)
		pr_err("get screen_max_y is fail, %d\n", ret);

	ret = of_property_read_u32(np, "ctp_revert_x_flag",
							&data->revert_x_flag);
	if (ret)
		pr_err("get revert_x_flag is fail, %d\n", ret);

	ret = of_property_read_u32(np, "ctp_revert_y_flag",
							&data->revert_y_flag);
	if (ret)
		pr_err("get revert_y_flag is fail, %d\n", ret);

	ret = of_property_read_u32(np, "ctp_exchange_x_y_flag",
						&data->exchange_x_y_flag);
	if (ret)
		pr_err("get ctp_exchange_x_y_flag is fail, %d\n", ret);

	data->irq_gpio.gpio = of_get_named_gpio_flags(np, "ctp_int_port", 0,
				(enum of_gpio_flags *)(&(data->irq_gpio)));
	if (!gpio_is_valid(data->irq_gpio.gpio))
		pr_err("%s: irq_gpio is invalid.\n", __func__);
	else
		data->int_number = data->irq_gpio.gpio;

	ret = of_property_read_u32(np, "ctp_gesture_wakeup", &(data->ctp_gesture_wakeup));
	if (ret) {
		 pr_err("get ctp_gesture_wakeup fail, no gesture wakeup\n");
	}
#ifdef TOUCH_KEY_LIGHT_SUPPORT

	data->key_light_gpio.gpio = of_get_named_gpio(np, "ctp_light", 0);
	if (!gpio_is_valid(data->key_light_gpio.gpio))
		pr_err("%s: key_light_gpio is invalid.\n", __func__);
#endif
	return 0;

devicetree_get_item_err:
	pr_notice("=========script_get_item_err============\n");
	return ret;
}

/**
 * sunxi_ctp_free() - free ctp related resource
 * @ctp_type:sensor type
 */
static void sunxi_ctp_free(enum input_sensor_type *ctp_type)
{
	struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);

	gpio_free(data->wakeup_gpio.gpio);

#ifdef TOUCH_KEY_LIGHT_SUPPORT
	gpio_free(data->key_light_gpio.gpio);
#endif
	if (!IS_ERR_OR_NULL(data->ctp_power_ldo)) {
		regulator_disable(data->ctp_power_ldo);
		regulator_put(data->ctp_power_ldo);
		data->ctp_power_ldo = NULL;
	} else if (gpio_is_valid(data->ctp_power_io.gpio))
		gpio_free(data->ctp_power_io.gpio);
}

/**
 * sunxi_ctp_init - initialize platform related resource
 * @ctp_type:sensor type
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int sunxi_ctp_init(enum input_sensor_type *ctp_type)
{
	int ret = -1;
	struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);

	data->ctp_power_ldo = NULL;
	data->ctp_power_ldo = regulator_get(data->dev, "ctp");
	if (!IS_ERR_OR_NULL(data->ctp_power_ldo)) {
		regulator_set_voltage(data->ctp_power_ldo,
				(int)(data->ctp_power_vol)*1000,
				(int)(data->ctp_power_vol)*1000);
	} else if (gpio_is_valid(data->ctp_power_io.gpio)) {
		if (gpio_request(data->ctp_power_io.gpio, NULL) != 0)
			pr_err("ctp_power_io gpio_request is failed, check if ctp independent power supply by gpio, ignore firstly\n");
		else
			gpio_direction_output(data->ctp_power_io.gpio, 1);
	}
	if (gpio_request(data->wakeup_gpio.gpio, NULL) != 0) {
		pr_err("wakeup gpio_request is failed\n");
		return ret;
	}
	if (gpio_direction_output(data->wakeup_gpio.gpio, 1) != 0) {
		pr_err("wakeup gpio set err!");
		return ret;
	}

#ifdef TOUCH_KEY_LIGHT_SUPPORT
	if (gpio_request(data->key_light_gpio.gpio, NULL) != 0) {
		pr_err("key_light gpio_request is failed\n");
		return ret;
	}

	if (gpio_direction_output(data->key_light_gpio.gpio, 1) != 0) {
		pr_err("key_light gpio set err!");
		return ret;
	}
#endif
	ret = 0;
	return ret;
}
/*************************CTP END************************************/

/*************************GSENSOR************************************/

/*
 * sunxi_gsensor_free() - free gsensor related resource
 * @gsensor_type:sensor type
 */
static void sunxi_gsensor_free(enum input_sensor_type *gsensor_type)
{
	struct sensor_config_info *data = container_of(gsensor_type,
					struct sensor_config_info, input_type);

	if (!IS_ERR_OR_NULL(data->sensor_power_ldo)) {
		regulator_disable(data->sensor_power_ldo);
		regulator_put(data->sensor_power_ldo);
		data->sensor_power_ldo = NULL;
	}
}

/**
 * sunxi_gsensor_init() - initialize platform related resource
 * @gsensor_type:sensor type
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int sunxi_gsensor_init(enum input_sensor_type *gsensor_type)
{
	struct sensor_config_info *data = container_of(gsensor_type,
					struct sensor_config_info, input_type);
	data->sensor_power_ldo = regulator_get(data->dev, "gsensor");
	if (!IS_ERR_OR_NULL(data->sensor_power_ldo)) {
		regulator_set_voltage(data->sensor_power_ldo,
				(int)(data->sensor_power_vol)*1000,
				(int)(data->sensor_power_vol)*1000);
	}
	return 0;
}

/**
 * sunxi_gsensor_startup() - get config info from sys_config.fex file.
 * @gsensor_type:sensor type
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int sunxi_gsensor_startup(enum input_sensor_type *gsensor_type)
{
	int ret = -1;
	struct sensor_config_info *data = container_of(gsensor_type,
					struct sensor_config_info, input_type);
	struct device_node *np = NULL;
	struct i2c_client *client = NULL;
	struct platform_device *pdev = NULL;

	np = data->node;

	if (!np) {
		if (data->np_name != NULL) {
			np = of_find_node_by_name(NULL, data->np_name);
		} else {
			np = of_find_node_by_name(NULL, "gsensor");
		}
	}

	if (!np) {
		pr_err("ERROR! get gsensor_para failed, func:%s, line:%d\n",
							__func__, __LINE__);
		goto devicetree_get_item_err;
	}

	if (!of_device_is_available(np)) {
		pr_err("%s: gsensor is not used\n", __func__);
		goto devicetree_get_item_err;
	} else
		data->sensor_used = 1;

	client = of_find_i2c_device_by_node(np);
	if (client) {
		data->dev = &client->dev;
		data->isI2CClient = 1;
	} else {
		pdev = of_find_device_by_node(np);
		if (pdev)
			data->dev = &pdev->dev;
	}

	if (!data->dev) {
		pr_err("get device_node fail\n");
		return -1;
	}

	if (data->sensor_used == 1) {
		ret = of_property_read_u32(np, "gsensor_twi_id", &data->twi_id);
		if (ret) {
			pr_err("get gsensor_twi_id is fail, %d\n", ret);
			goto devicetree_get_item_err;
		}
	} else
		pr_err("%s gsensor_unused\n", __func__);
	ret = of_property_read_u32(np, "gsensor_vcc_io_val",
						&data->sensor_power_vol);
	if (ret) {
		pr_err("get gsensor_vcc_io_val is fail, %d\n", ret);
		goto devicetree_get_item_err;
	  }

	return ret;

devicetree_get_item_err:
	pr_notice("=========gsensor script_get_err============\n");
	ret = -1;
	return ret;

}

/**************************GSENSOR END*********************************/

/*************************** GYR *************************************/

/**
 * sunxi_gyr_free() - free gyr related resource
 * @gyr_type:sensor type
 * return value:
 */
static void sunxi_gyr_free(enum input_sensor_type *gyr_type)
{
}

/**
 * sunxi_gyr_init() - initialize platform related resource
 * @gyr_type:sensor type
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int sunxi_gyr_init(enum input_sensor_type *gyr_type)
{
	return 0;
}

/**
 * sunxi_gyr_startup() - get config info from sys_config.fex file.
 * @gyr_type:sensor type
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int sunxi_gyr_startup(enum input_sensor_type *gyr_type)
{
	int ret = -1;
	struct sensor_config_info *data = container_of(gyr_type,
					struct sensor_config_info, input_type);
	struct device_node *np = NULL;

	np = of_find_node_by_name(NULL, "gy");
	if (!np) {
		pr_err("ERROR! get gy_para failed, func:%s, line:%d\n",
							__func__, __LINE__);
		goto devicetree_get_item_err;
	}

	if (!of_device_is_available(np)) {
		pr_err("%s: gy is not used\n", __func__);
		goto devicetree_get_item_err;
	} else
		data->sensor_used = 1;

	if (data->sensor_used == 1) {
		ret = of_property_read_u32(np, "gy_twi_id", &data->twi_id);
		if (ret) {
			pr_err("get gy_twi_id is fail, %d\n", ret);
			goto devicetree_get_item_err;
		}
	} else
		pr_err("%s gy_unused\n", __func__);

	return ret;

devicetree_get_item_err:
	pr_notice("=========script_get_err============\n");
	ret = -1;
	return ret;
}

/****************************** GYR END **********************************/

/****************************** COMPASS *********************************/

/**
 * sunxi_ecompass_free() - free e_compass related resource
 * @e_compass_type:sensor type
 * return value:
 */
static void sunxi_ecompass_free(enum input_sensor_type *e_compass_type)
{
}

/**
 * sunxi_ecompass_init() - initialize platform related resource
 * @e_compass_type:sensor type
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int sunxi_ecompass_init(enum input_sensor_type *e_compass_type)
{
	return 0;
}

/**
 * sunxi_ecompass_staetup() - get config info from sys_config.fex file.
 * @e_compass_type:sensor type
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */

static int sunxi_ecompass_startup(enum input_sensor_type *e_compass_type)
{
	int ret = -1;
	struct sensor_config_info *data = container_of(e_compass_type,
					struct sensor_config_info, input_type);
	struct device_node *np = NULL;

	np = of_find_node_by_name(NULL, "compass");
	if (!np) {
		pr_err("ERROR! get compass_para failed, func:%s, line:%d\n",
							__func__, __LINE__);
		goto devicetree_get_item_err;
	}

	if (!of_device_is_available(np)) {
		pr_err("%s: compass is not used\n", __func__);
		goto devicetree_get_item_err;
	} else
		data->sensor_used = 1;

	if (data->sensor_used == 1) {
		ret = of_property_read_u32(np, "compass_twi_id", &data->twi_id);
		if (ret) {
			pr_err("get compass_twi_id is fail, %d\n", ret);
			goto devicetree_get_item_err;
		}
	} else
		pr_err("%s gsensor_unused\n", __func__);

	return ret;

devicetree_get_item_err:
	pr_notice("=========script_get_err============\n");
	ret = -1;
	return ret;

}
/***************************** COMPASS END ***********************************/

/**************************** LIGHT SENSOR ***********************************/

/**
 * sunxi_ls_free() - free ls related resource
 * @ls_type:sensor type
 * return value:
 */
static void sunxi_ls_free(enum input_sensor_type *ls_type)
{
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);
	/* disable ldo if it exist */

	if (!IS_ERR_OR_NULL(data->sensor_power_ldo)) {
		regulator_disable(data->sensor_power_ldo);
		regulator_put(data->sensor_power_ldo);
	}
}

/**
 * sunxi_ls_init() - initialize platform related resource
 * @ls_type:sensor type
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int sunxi_ls_init(enum input_sensor_type *ls_type)
{
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);
	data->sensor_power_ldo = regulator_get(data->dev, "lightsensor");
	if (!IS_ERR_OR_NULL(data->sensor_power_ldo)) {
		/*regulator_set_voltage(ldo, 3000000, 3000000);*/
		if (0 != regulator_enable(data->sensor_power_ldo))
			pr_err("%s: regulator_enable error!\n", __func__);
		usleep_range(10000, 15000);

	}
	return 0;
}

/**
 * sunxi_ls_startup() - get config info from sysconfig.fex file.
 * @ls_type:sensor type
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */

static int sunxi_ls_startup(enum input_sensor_type *ls_type)
{
	int ret = -1;
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);
	struct device_node *np = NULL;
	struct i2c_client *client = NULL;
	struct platform_device *pdev = NULL;

	np = data->node;
	if (!np) {
		if (data->np_name != NULL) {
			np = of_find_node_by_name(NULL, data->np_name);
		} else {
			np = of_find_node_by_name(NULL, "lightsensor");
		}
	}

	if (!np) {
		pr_err("ERROR! get ls_para failed, func:%s, line:%d\n",
							__func__, __LINE__);
		goto devicetree_get_item_err;
	}

	if (!of_device_is_available(np)) {
		pr_err("%s: ls is not used\n", __func__);
		goto devicetree_get_item_err;
	} else
		data->sensor_used = 1;

	client = of_find_i2c_device_by_node(np);
	if (client) {
		data->dev = &client->dev;
		data->isI2CClient = 1;
	} else {
		pdev = of_find_device_by_node(np);
		if (pdev)
			data->dev = &pdev->dev;
	}
	if (!data->dev) {
		pr_err("get device_node fail\n");
		return -1;
	}

	if (data->sensor_used == 1) {
		ret = of_property_read_u32(np, "ls_twi_id", &data->twi_id);
		if (ret) {
			pr_err("get compass_twi_id is fail, %d\n", ret);
			goto devicetree_get_item_err;
		}

		data->irq_gpio.gpio = of_get_named_gpio(np, "ls_int", 0);
		if (!gpio_is_valid(data->irq_gpio.gpio))
			pr_err("%s: irq_gpio is invalid.\n", __func__);
		else
			data->int_number = data->irq_gpio.gpio;
#if !IS_ENABLED(CONFIG_SUNXI_REGULATOR_DT)
		of_property_read_string(np, "ls_vcc", &data->sensor_power);
#endif
	} else {
		pr_err("%s light sensor unused \n", __func__);
	}

	return ret;

devicetree_get_item_err:
	ret = -1;
	return ret;

}
/**************** LIGHT SENSOR END ***********************/

/************************ MOTOR *************************/

/**
 * sunxi_motor_free()- free ths related resource
 * @motor_type:sensor type
 * return value:
 */
static void sunxi_motor_free(enum input_sensor_type *motor_type)
{
	struct motor_config_info *data = container_of(motor_type,
					struct motor_config_info, input_type);

	if (data->motor_gpio.gpio != 0)
		gpio_free(data->motor_gpio.gpio);
	if (data->dev != NULL) {
		if (data->dev->init_name)
			kfree(data->dev->init_name);
		kfree(data->dev);
		data->dev = NULL;
	}
}

/**
 * sunxi_motor_init() - initialize platform related resource
 * @motor_type:sensor type
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int sunxi_motor_init(enum input_sensor_type *motor_type)
{
	struct motor_config_info *data = container_of(motor_type,
					struct motor_config_info, input_type);

#ifdef CONFIG_SUNXI_REGULATOR_DT
	data->motor_power_ldo = regulator_get(data->dev, "motor");
	if (!IS_ERR(data->motor_power_ldo)) {
		regulator_set_voltage(data->motor_power_ldo,
				(int)(data->ldo_voltage)*1000,
				(int)(data->ldo_voltage)*1000);
	}
#endif
	if (gpio_is_valid(data->motor_gpio.gpio)) {
		if (gpio_request(data->motor_gpio.gpio, "vibe") != 0) {
			pr_err("ERROR: vibe Gpio_request is failed\n");
			return -1;
		}
		gpio_direction_output(data->motor_gpio.gpio, data->vibe_off);
	}

	return 0;
}

/**
 * sunxi_motor_startup() - get config info from sys_config.fex file.
 * @motor_type:sensor type
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int sunxi_motor_startup(enum input_sensor_type *motor_type)
{
	struct motor_config_info *data = container_of(motor_type,
					struct motor_config_info, input_type);
	struct device_node *np = NULL;
	int ret = -1;
	struct platform_device *pdev = NULL;
#ifdef CONFIG_SUNXI_REGULATOR_DT
	struct device *dev = NULL;
#endif

	np = of_find_node_by_name(NULL, "motor_para");
	if (!np) {
		pr_err("ERROR! get motor_para failed, func:%s, line:%d\n",
			__func__, __LINE__);
		return -1;
	}

	if (of_device_is_available(np))
		data->motor_used = 1;
	else {
		pr_err("%s: motor_para is not used\n", __func__);
		return -1;
	}
#ifdef CONFIG_SUNXI_REGULATOR_DT
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (dev) {
		char *dev_name = kzalloc(10, GFP_KERNEL);
		strcpy(dev_name, "motor");
		dev->init_name = dev_name;
		dev->of_node = np;
		data->dev = dev;
	}
#endif

	data->motor_gpio.gpio = of_get_named_gpio_flags(np, "motor_shake", 0,
				(enum of_gpio_flags *)(&(data->motor_gpio)));
	if (!gpio_is_valid(data->motor_gpio.gpio))
		pr_err("%s: motor_shake is invalid\n", __func__);

	ret = of_property_read_string(np, "motor_ldo", &data->ldo);
	if (ret)
		pr_err("get motor_ldo is fail, %d\n", ret);

	pdev = of_find_device_by_node(np);
	if (pdev)
		data->dev = &pdev->dev;

	if (!data->dev) {
		pr_err("get device_node fail\n");
		return -1;
	}

	data->motor_power_ldo = regulator_get(data->dev, "motor");
	if (!IS_ERR(data->motor_power_ldo)) {
		regulator_set_voltage(data->motor_power_ldo,
				(int)(data->ldo_voltage)*1000,
				(int)(data->ldo_voltage)*1000);
	}

	ret = of_property_read_u32(np, "motor_ldo_voltage", &data->ldo_voltage);
	if (ret)
		pr_err("get motor_ldo_voltage is fail, %d\n", ret);

	return 0;
}

/******************************** MOTOR END ***********************************/


static int (* const sunxi_startup[])(enum input_sensor_type *input_type) = {
	sunxi_ctp_startup,
	sunxi_gsensor_startup,
	sunxi_gyr_startup,
	sunxi_ecompass_startup,
	sunxi_ls_startup,
	sunxi_motor_startup,
};

static int (*sunxi_init[])(enum input_sensor_type *input_type) = {
	sunxi_ctp_init,
	sunxi_gsensor_init,
	sunxi_gyr_init,
	sunxi_ecompass_init,
	sunxi_ls_init,
	sunxi_motor_init,
};

static void (*sunxi_free[])(enum input_sensor_type *input_type) = {
	sunxi_ctp_free,
	sunxi_gsensor_free,
	sunxi_gyr_free,
	sunxi_ecompass_free,
	sunxi_ls_free,
	sunxi_motor_free,
};

int input_set_power_enable(enum input_sensor_type *input_type, u32 enable)
{
	int ret = -1;
	struct regulator *ldo = NULL;
	u32 power_io = 0;
	void *data = NULL;

	switch (*input_type) {
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		ldo = ((struct ctp_config_info *)data)->ctp_power_ldo;
		power_io = ((struct ctp_config_info *)data)->ctp_power_io.gpio;
		break;
	case GSENSOR_TYPE:
		data = container_of(input_type, struct sensor_config_info,
								input_type);
		ldo = ((struct sensor_config_info *)data)->sensor_power_ldo;
		break;
	case LS_TYPE:
		break;
	case MOTOR_TYPE:
		data = container_of(input_type, struct motor_config_info,
								input_type);
		ldo = ((struct motor_config_info *)data)->motor_power_ldo;
	default:
		break;
	}
	if ((enable != 0) && (enable != 1))
		return ret;

	if (!IS_ERR_OR_NULL(ldo)) {
		if (enable) {
			if (regulator_enable(ldo) != 0)
				pr_err("%s: enable ldo error!\n", __func__);
		} else {
			regulator_disable(ldo);
		}
	} else if (gpio_is_valid(power_io)) {
		if (enable)
			__gpio_set_value(power_io, 1);
		else
			__gpio_set_value(power_io, 0);
	}

	return 0;
}
EXPORT_SYMBOL(input_set_power_enable);
/**
 * input_set_int_enable() - input set irq enable
 * Input_type:sensor type
 *      enable:
 * return value: 0 : success
 *               -EIO :  i/o err.
 */
int input_set_int_enable(enum input_sensor_type *input_type, u32 enable)
{
	int ret = -1;
	u32 irq_number = 0;
	void *data = NULL;

	switch (*input_type) {
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		irq_number = gpio_to_irq(((struct ctp_config_info *)data)->int_number);
		break;
	case GSENSOR_TYPE:
		break;
	case LS_TYPE:
		data = container_of(input_type,
					struct sensor_config_info, input_type);
		irq_number = gpio_to_irq(((struct sensor_config_info *)data)->int_number);
		break;
	default:
		break;
	}

	if ((enable != 0) && (enable != 1))
		return ret;

	if (enable == 1)
		enable_irq(irq_number);
	else
		disable_irq_nosync(irq_number);
/*
	if (*input_type == CTP_TYPE && enable == 0)
		dump_stack();
*/
	return 0;
}
EXPORT_SYMBOL(input_set_int_enable);

int input_set_int_enable_force(enum input_sensor_type *input_type, u32 enable)
{
	int ret = -1;
	u32 irq_number = 0;
	void *data = NULL;
	struct irq_desc *desc = NULL;

	switch (*input_type) {
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		irq_number = gpio_to_irq(((struct ctp_config_info *)data)->int_number);
		break;
	case GSENSOR_TYPE:
		break;
	case LS_TYPE:
		data = container_of(input_type,
					struct sensor_config_info, input_type);
		irq_number = gpio_to_irq(((struct sensor_config_info *)data)->int_number);
		break;
	default:
		break;
	}

	if ((enable != 0) && (enable != 1))
		return ret;

	desc = irq_to_desc(irq_number);
	if (enable == 1) {
		while (desc->depth >= 1)
			enable_irq(irq_number);
	} else {
		disable_irq_nosync(irq_number);
	}
	return 0;
}
EXPORT_SYMBOL(input_set_int_enable_force);

/**
 * input_free_int - input free irq
 * Input_type:sensor type
 * return value: 0 : success
 *               -EIO :  i/o err.
 */
int input_free_int(enum input_sensor_type *input_type, void *para)
{
	int irq_number = 0;
	void *data = NULL;
	struct device *dev = NULL;

	switch (*input_type) {
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		irq_number = gpio_to_irq(((struct ctp_config_info *)data)->int_number);

		dev = ((struct ctp_config_info *)data)->dev;
		break;
	case GSENSOR_TYPE:
		break;
	case LS_TYPE:
		data = container_of(input_type,
					struct sensor_config_info, input_type);
		irq_number = gpio_to_irq(((struct sensor_config_info *)data)->int_number);

		dev = ((struct sensor_config_info *)data)->dev;
		break;
	default:
		break;
	}

	devm_free_irq(dev, irq_number, para);

	return 0;
}
EXPORT_SYMBOL(input_free_int);

/**
 * input_request_int() - input request irq
 * Input:
 *	type:
 *      handle:
 *      trig_gype:
 *      para:
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
int input_request_int(enum input_sensor_type *input_type, irq_handler_t handle,
			unsigned long trig_type, void *para)
{
	int ret = -1;
	int irq_number = 0;

	void *data = NULL;
	struct device *dev = NULL;

	switch (*input_type) {
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		irq_number = gpio_to_irq(((struct ctp_config_info *)data)->int_number);
		if (irq_number < 0) {
			pr_warn("map gpio to virq failed, errno = %d\n",
								irq_number);
			return -EINVAL;
		}

		dev = ((struct ctp_config_info *)data)->dev;
		break;
	case GSENSOR_TYPE:
		break;
	case LS_TYPE:
		data = container_of(input_type,
					struct sensor_config_info, input_type);
		irq_number = gpio_to_irq(((struct sensor_config_info *)data)->int_number);
		if (irq_number < 0) {
			pr_warn("map gpio to virq failed, errno = %d\n",
								irq_number);
			return -EINVAL;
		}

		dev = ((struct sensor_config_info *)data)->dev;
		break;
	default:
		break;
	}

	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(dev, irq_number, handle,
						trig_type, "PA3_EINT", para);
	if (ret < 0) {
		pr_warn("request virq %d failed, errno = %d\n",
							irq_number, ret);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL(input_request_int);

void input_sensor_free(enum input_sensor_type *input_type)
{
	(*sunxi_free[*input_type])(input_type);
}
EXPORT_SYMBOL(input_sensor_free);

int input_sensor_init(enum input_sensor_type *input_type)
{
	int ret = -1;

	ret = (*sunxi_init[*input_type])(input_type);

	return ret;
}
EXPORT_SYMBOL(input_sensor_init);

int input_sensor_startup(enum input_sensor_type *input_type)
{
	int ret = -1;

	ret = (*sunxi_startup[*input_type])(input_type);

	return ret;
}
EXPORT_SYMBOL(input_sensor_startup);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Provide function for allwinner sensor an touchscreen devices");
MODULE_AUTHOR("Allwinner");
MODULE_VERSION("1.0.2");

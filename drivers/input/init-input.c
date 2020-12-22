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
#include <linux/interrupt.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/init-input.h>
#include <linux/pinctrl/consumer.h>

/*********************************CTP*******************************************/

/**
 * ctp_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ctp_fetch_sysconfig_para(enum input_sensor_type *ctp_type)
{
	int ret = -1;
	script_item_u   val;
	script_item_value_type_e  type;
	struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);

	pr_info("=====%s=====. \n", __func__);

	type = script_get_item("ctp_para", "ctp_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: ctp_used script_get_item  err. \n", __func__);
		goto script_get_item_err;
	}
	data->ctp_used = val.val;
	
	if (1 != data->ctp_used) {
		pr_err("%s: ctp_unused. \n",  __func__);
		goto script_get_item_err;
	}

	type = script_get_item("ctp_para", "ctp_twi_id", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: ctp_twi_id script_get_item err. \n",__func__ );
		goto script_get_item_err;
	}
	data->twi_id = val.val;

	type = script_get_item("ctp_para", "ctp_name", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR!= type) {
		pr_err("%s: ctp_name script_get_item err. \n",__func__ );
	}
	else
		data->name = val.str;

	type = script_get_item("ctp_para", "ctp_power_ldo", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type) {
		pr_err("%s: ctp_power_ldo script_get_item err. \n",__func__ );
	}
	else
		data->ctp_power = val.str;
	type = script_get_item("ctp_para", "ctp_power_ldo_vol", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: ctp_power_ldo_vol script_get_item err. \n",__func__ );
	}
	else
		data->ctp_power_vol = val.val;
	type = script_get_item("ctp_para", "ctp_power_io", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("%s: ctp_power_io script_get_item err. \n",__func__ );
	}
	else
		data->ctp_power_io = val.gpio;
	type = script_get_item("ctp_para", "ctp_wakeup", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("script_get_item ctp_wakeup err\n");
	}
	else {
		data->wakeup_gpio = val.gpio;
	}
	pr_debug("ctp_wakeup gpio number is %d\n", data->wakeup_gpio.gpio);

		type = script_get_item("ctp_para", "ctp_screen_max_x", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("%s: ctp_screen_max_x script_get_item err. \n",__func__ );
			goto script_get_item_err;
		}
		data->screen_max_x = val.val;

	type = script_get_item("ctp_para", "ctp_screen_max_y", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("%s: ctp_screen_max_y script_get_item err. \n",__func__ );
			goto script_get_item_err;
	}
	data->screen_max_y = val.val;

	type = script_get_item("ctp_para", "ctp_revert_x_flag", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        pr_err("%s: ctp_revert_x_flag script_get_item err. \n",__func__ );
	        goto script_get_item_err;
	}
	data->revert_x_flag = val.val;

	type = script_get_item("ctp_para", "ctp_revert_y_flag", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        pr_err("%s: ctp_revert_y_flag script_get_item err. \n",__func__ );
	        goto script_get_item_err;
	}
	data->revert_y_flag = val.val;

	type = script_get_item("ctp_para", "ctp_exchange_x_y_flag", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
	        pr_err("%s: ctp_exchange_x_y_flag script_get_item err. \n",__func__ );
	        goto script_get_item_err;
	}
	data->exchange_x_y_flag = val.val;


	type = script_get_item("ctp_para", "ctp_int_port", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("script_get_item ctp_int_port type err\n");
		goto script_get_item_err;
	}
	data->irq_gpio = val.gpio;
    data->int_number = val.gpio.gpio;
    pr_err("ctp_irq gpio number is %d\n", data->int_number);
        
#ifdef TOUCH_KEY_LIGHT_SUPPORT 
	type = script_get_item("ctp_para", "ctp_light", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("script_get_item ctp_light err\n");
		goto script_get_item_err;
	}
	data->key_light_gpio = val.gpio;
#endif

	
	return 0;

script_get_item_err:
	pr_notice("=========script_get_item_err============\n");
	return ret;
}

/**
 * ctp_free_platform_resource - free ctp related resource
 * return value:
 */
static void ctp_free_platform_resource(enum input_sensor_type *ctp_type)
{
	struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);
	gpio_free(data->wakeup_gpio.gpio);

#ifdef TOUCH_KEY_LIGHT_SUPPORT
	gpio_free(data->key_light_gpio.gpio);
#endif	
	if(data->ctp_power_ldo) {
		regulator_put(data->ctp_power_ldo);
		data->ctp_power_ldo= NULL;
	} else if(0 != data->ctp_power_io.gpio) {
		gpio_free(data->ctp_power_io.gpio);
	}
	return;
}

/**
 * ctp_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ctp_init_platform_resource(enum input_sensor_type *ctp_type)
{	
	int ret = -1;
        struct ctp_config_info *data = container_of(ctp_type,
					struct ctp_config_info, input_type);
					
	if (data->ctp_power) {
		data->ctp_power_ldo = regulator_get(NULL, data->ctp_power);
		if (!data->ctp_power_ldo)
			pr_err("%s: could not get ctp ldo '%s' , check"
					"if ctp independent power supply by ldo,ignore"
					"firstly\n",__func__,data->ctp_power);
		else
			regulator_set_voltage(data->ctp_power_ldo,
					(int)(data->ctp_power_vol)*1000,
					(int)(data->ctp_power_vol)*1000);
	} else if(0 != data->ctp_power_io.gpio) {
		if(0 != gpio_request(data->ctp_power_io.gpio, NULL))
			pr_err("ctp_power_io gpio_request is failed,"
					"check if ctp independent power supply by gpio,"
					"ignore firstly\n");
		else
			gpio_direction_output(data->ctp_power_io.gpio, 1);
	}
	if(0 != gpio_request(data->wakeup_gpio.gpio, NULL)) {
		pr_err("wakeup gpio_request is failed\n");
		return ret;
	} 
	if (0 != gpio_direction_output(data->wakeup_gpio.gpio, 1)) {
		pr_err("wakeup gpio set err!");
		return ret;
	}
		
#ifdef TOUCH_KEY_LIGHT_SUPPORT 
	if(0 != gpio_request(data->key_light_gpio.gpio, NULL)) {
		pr_err("key_light gpio_request is failed\n");
		return ret;
	}

	if (0 != gpio_direction_output(data->key_light_gpio.gpio, 1)) {
		pr_err("key_light gpio set err!");
		return ret;
	}
#endif

        

	
	ret = 0;      
	return ret;
}
/*********************************CTP END***************************************/

/*********************************GSENSOR***************************************/

/**
 * gsensor_free_platform_resource - free gsensor related resource
 * return value:
 */
static void gsensor_free_platform_resource(enum input_sensor_type *gsensor_type)
{
}

/**
 * gsensor_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int gsensor_init_platform_resource(enum input_sensor_type *gsensor_type)
{
	return 0;
}

/**
 * gsensor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gsensor_fetch_sysconfig_para(enum input_sensor_type *gsensor_type)
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;
	struct sensor_config_info *data = container_of(gsensor_type,
					struct sensor_config_info, input_type);
	
	type = script_get_item("gsensor_para", "gsensor_used", &val);
 
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	data->sensor_used = val.val;
	
	if (1 == data->sensor_used) {
		type = script_get_item("gsensor_para", "gsensor_twi_id", &val);	
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("%s: type err twi_id = %d. \n", __func__, val.val);
			goto script_get_err;
		}
		data->twi_id = val.val;
		
		ret = 0;
	} else {
		pr_err("%s: gsensor_unused. \n",  __func__);
		return ret;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;

}

/*********************************GSENSOR END***********************************/

/********************************** GYR ****************************************/

/**
 * gyr_free_platform_resource - free gyr related resource
 * return value:
 */
static void gyr_free_platform_resource(enum input_sensor_type *gyr_type)
{
}

/**
 * gyr_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int gyr_init_platform_resource(enum input_sensor_type *gyr_type)
{
	return 0;
}

/**
 * gyr_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gyr_fetch_sysconfig_para(enum input_sensor_type *gyr_type)
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;
	struct sensor_config_info *data = container_of(gyr_type,
					struct sensor_config_info, input_type);
		
	type = script_get_item("gy_para", "gy_used", &val);
 
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	data->sensor_used = val.val;
	
	if (1 == data->sensor_used) {
		type = script_get_item("gy_para", "gy_twi_id", &val);	
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err twi_id = %d. \n", __func__, val.val);
			goto script_get_err;
		}
		data->twi_id = val.val;

		ret = 0;
		
	} else {
		pr_err("%s: gyr_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;
}

/********************************* GYR END *************************************/

/********************************* COMPASS *************************************/

/**
 * e_compass_free_platform_resource - free e_compass related resource
 * return value:
 */
static void e_compass_free_platform_resource(enum input_sensor_type *e_compass_type)
{
}

/**
 * e_compass_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int e_compass_init_platform_resource(enum input_sensor_type *e_compass_type)
{
	return 0;
}

/**
 * e_compass_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */

static int e_compass_fetch_sysconfig_para(enum input_sensor_type *e_compass_type)
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;
	struct sensor_config_info *data = container_of(e_compass_type,
					struct sensor_config_info, input_type);

	type = script_get_item("compass_para", "compass_used", &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	data->sensor_used = val.val;
	
	if (1 == data->sensor_used) {
		type = script_get_item("compass_para", "compass_twi_id", &val);	
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err twi_id = %d. \n", __func__, val.val);
			goto script_get_err;
		}
		data->twi_id = val.val;

		ret = 0;
		
	} else {
		pr_err("%s: compass_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;

}
/******************************* COMPASS END ***********************************/

/****************************** LIGHT SENSOR ***********************************/

/**
 * ls_free_platform_resource - free ls related resource
 * return value:
 */
static void ls_free_platform_resource(enum input_sensor_type *ls_type)
{
	struct regulator *ldo = NULL;
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);
	/* disable ldo if it exist */
	if (data->ldo) {
		ldo = regulator_get(NULL, data->ldo);
		if (!ldo) {
			pr_err("%s: could not get ldo '%s' in remove, something error ???, "
					"ignore it here !!!!!!!!!\n", __func__, data->ldo);
		} else {
			regulator_disable(ldo);
			regulator_put(ldo);
		}
	}
}

/**
 * ls_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ls_init_platform_resource(enum input_sensor_type *ls_type)
{
	struct regulator *ldo = NULL;
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);

	/* enalbe ldo if it exist */
	if (data->ldo) {
		ldo = regulator_get(NULL, data->ldo);
		if (!ldo) {
			pr_err("%s: could not get sensor ldo '%s' in probe, maybe config error,"
					"ignore firstly !!!!!!!\n", __func__, data->ldo);
		}
		regulator_set_voltage(ldo, 3000000, 3000000);
		regulator_enable(ldo);
		regulator_put(ldo);
		usleep_range(10000, 15000);
	}
	return 0;
}

/**
 * ls_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */

static int ls_fetch_sysconfig_para(enum input_sensor_type *ls_type)
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;
	struct sensor_config_info *data = container_of(ls_type,
					struct sensor_config_info, input_type);

	type = script_get_item("ls_para", "ls_used", &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	data->sensor_used = val.val;
	
	if (1 == data->sensor_used) {
		type = script_get_item("ls_para", "ls_twi_id", &val);	
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err twi_id = %d. \n", __func__, val.val);
			goto script_get_err;
		}
		data->twi_id = val.val;

		type = script_get_item("ls_para", "ls_int", &val);	
		if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
			pr_err("%s: type err twi int1 = %d. \n", __func__, val.gpio.gpio);
			goto script_get_err;
		}
		data->irq_gpio = val.gpio;
		data->int_number = val.gpio.gpio;
		
		type = script_get_item("ls_para", "ls_ldo", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_STR != type)
			pr_err("%s: no ldo for light distance sensor, ignore it\n", __func__);
		else
			data->ldo = val.str;

		ret = 0;
		
	} else {
		pr_err("%s: ls_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;

}
/**************************** LIGHT SENSOR END *********************************/


/********************************** IR *****************************************/

/**
 * gyr_free_platform_resource - free ir related resource
 * return value:
 */
static void ir_free_platform_resource(enum input_sensor_type *ir_type)
{
	struct ir_config_info *data = container_of(ir_type,
					struct ir_config_info, input_type);
	if (!IS_ERR_OR_NULL(data->pinctrl))
		devm_pinctrl_put(data->pinctrl);
}

/**
 * gyr_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ir_init_platform_resource(enum input_sensor_type *ir_type)
{
	struct ir_config_info *data = container_of(ir_type,
					struct ir_config_info, input_type);

	data->pinctrl = devm_pinctrl_get_select_default(data->dev);
	if (IS_ERR_OR_NULL(data->pinctrl)) {
		pr_warn("request pinctrl handle for device [%s] failed\n",
		dev_name(data->dev));
		return -EINVAL;
	}

	return 0;
}

/**
 * gyr_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ir_fetch_sysconfig_para(enum input_sensor_type *ir_type)
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;
	struct ir_config_info *data = container_of(ir_type,
					struct ir_config_info, input_type);
		
	type = script_get_item("s_cir0", "ir_used", &val);
 
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	data->ir_used = val.val;
	
	if (1 == data->ir_used) {
		type = script_get_item("s_cir0", "ir_rx", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_PIO != type){
			pr_err("%s: IR gpio type err! \n", __func__);
			goto script_get_err;
		}
		data->ir_gpio = val.gpio;

		type = script_get_item("s_cir0", "ir_power_key_code0", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: IR power key type err! \n", __func__);
		}
		data->power_key = val.val;

		ret = 0;
		
	} else {
		pr_err("%s: ir_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;
}

/********************************** IR END *************************************/

/*********************************** THS ***************************************/

/**
 * ths_free_platform_resource - free ths related resource
 * return value:
 */
static void ths_free_platform_resource(enum input_sensor_type *ths_type)
{
}

/**
 * ths_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int ths_init_platform_resource(enum input_sensor_type *ths_type)
{
	return 0;
}

/**
 * ths_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ths_fetch_sysconfig_para(enum input_sensor_type *ths_type)
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;
	struct ths_config_info *data = container_of(ths_type,
					struct ths_config_info, input_type);
		
	type = script_get_item("ths_para", "ths_used", &val);
 
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	data->ths_used = val.val;

	type = script_get_item("ths_para", "ths_trend", &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
	} else
		data->ths_trend = val.val;

	if (1 == data->ths_used) {
		type = script_get_item("ths_para", "ths_trip1_count", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err(KERN_INFO"%s: type err ths_trip1_count = %d. \n", __func__, val.val);
		} else
			data->trip1_count = val.val;

		type = script_get_item("ths_para", "ths_trip1_0", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_0 = %d. \n", __func__, val.val);
		} else
			data->trip1_0 = val.val;

		type = script_get_item("ths_para", "ths_trip1_1", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_tri1p_1 = %d. \n", __func__, val.val);
		} else
			data->trip1_1 = val.val;

		type = script_get_item("ths_para", "ths_trip1_2", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_2 = %d. \n", __func__, val.val);
		} else
			data->trip1_2 = val.val;

		type = script_get_item("ths_para", "ths_trip1_3", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_3 = %d. \n", __func__, val.val);
		} else
			data->trip1_3 = val.val;

		type = script_get_item("ths_para", "ths_trip1_4", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_4 = %d. \n", __func__, val.val);
		} else
			data->trip1_4 = val.val;

		type = script_get_item("ths_para", "ths_trip1_5", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_5 = %d. \n", __func__, val.val);
		} else
			data->trip1_5 = val.val;

		type = script_get_item("ths_para", "ths_trip1_6", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_6 = %d. \n", __func__, val.val);
		} else
			data->trip1_6 = val.val;

		type = script_get_item("ths_para", "ths_trip1_7", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_7 = %d. \n", __func__, val.val);
		} else
			data->trip1_7 = val.val;

		type = script_get_item("ths_para", "ths_trip1_0_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_0_min = %d. \n", __func__, val.val);
		} else
			data->trip1_0_min = val.val;

		type = script_get_item("ths_para", "ths_trip1_0_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_0_max = %d. \n", __func__, val.val);
		} else
			data->trip1_0_max = val.val;

		type = script_get_item("ths_para", "ths_trip1_1_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_1_min = %d. \n", __func__, val.val);
		} else
			data->trip1_1_min = val.val;

		type = script_get_item("ths_para", "ths_trip1_1_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_1_max = %d. \n", __func__, val.val);
		} else
			data->trip1_1_max = val.val;

		type = script_get_item("ths_para", "ths_trip1_2_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_2_min = %d. \n", __func__, val.val);
		} else
			data->trip1_2_min = val.val;

		type = script_get_item("ths_para", "ths_trip1_2_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_2_max = %d. \n", __func__, val.val);
		} else
			data->trip1_2_max = val.val;

		type = script_get_item("ths_para", "ths_trip1_3_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_3_min = %d. \n", __func__, val.val);
		} else
			data->trip1_3_min = val.val;

		type = script_get_item("ths_para", "ths_trip1_3_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_3_max = %d. \n", __func__, val.val);
		} else
			data->trip1_3_max = val.val;

		type = script_get_item("ths_para", "ths_trip1_4_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_4_min = %d. \n", __func__, val.val);
		} else
			data->trip1_4_min = val.val;

		type = script_get_item("ths_para", "ths_trip1_4_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_4_max = %d. \n", __func__, val.val);
		} else
			data->trip1_4_max = val.val;

		type = script_get_item("ths_para", "ths_trip1_5_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_5_min = %d. \n", __func__, val.val);
		} else
			data->trip1_5_min = val.val;

		type = script_get_item("ths_para", "ths_trip1_5_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_5_max = %d. \n", __func__, val.val);
		} else
			data->trip1_5_max = val.val;

		type = script_get_item("ths_para", "ths_trip1_6_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_6_min = %d. \n", __func__, val.val);
		} else
			data->trip1_6_min = val.val;

		type = script_get_item("ths_para", "ths_trip1_6_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip1_6_max = %d. \n", __func__, val.val);
		} else
			data->trip1_6_max = val.val;

		type = script_get_item("ths_para", "ths_trip2_count", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip2_count = %d. \n", __func__, val.val);
		} else
			data->trip2_count = val.val;

		type = script_get_item("ths_para", "ths_trip2_0", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err ths_trip2_0 = %d. \n", __func__, val.val);
		} else
			data->trip2_0 = val.val;

		ret = 0;
	} else {
		pr_err("%s: ths_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;
}

/********************************* THS END ************************************/

/*********************************** BAT ***************************************/

/**
 * ths_free_platform_resource - free ths related resource
 * return value:
 */
static void bat_free_platform_resource(enum input_sensor_type *ths_type)
{
}

/**
 * ths_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int bat_init_platform_resource(enum input_sensor_type *ths_type)
{
	return 0;
}

/**
 * ths_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int bat_fetch_sysconfig_para(enum input_sensor_type *ths_type)
{
	int ret = -1;
	script_item_u	val;
	script_item_value_type_e  type;
	struct ths_config_info *data = container_of(ths_type,
					struct ths_config_info, input_type);

	type = script_get_item("bat_ths_para", "bat_ths_used", &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	data->ths_used = val.val;

	type = script_get_item("bat_ths_para", "bat_ths_trend", &val);

	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
	} else
		data->ths_trend = val.val;

	if (1 == data->ths_used) {
		type = script_get_item("bat_ths_para", "bat_ths_trip1_count", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_count = %d. \n", __func__, val.val);
		} else
			data->trip1_count = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_0", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_0 = %d. \n", __func__, val.val);
		} else
			data->trip1_0 = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_1", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_tri1p_1 = %d. \n", __func__, val.val);
		} else
			data->trip1_1 = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_2", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_2 = %d. \n", __func__, val.val);
		} else
			data->trip1_2 = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_3", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_3 = %d. \n", __func__, val.val);
		} else
			data->trip1_3 = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_4", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_4 = %d. \n", __func__, val.val);
		} else
			data->trip1_4 = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_5", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_5 = %d. \n", __func__, val.val);
		} else
			data->trip1_5 = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_6", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_6 = %d. \n", __func__, val.val);
		} else
			data->trip1_6 = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_7", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_7 = %d. \n", __func__, val.val);
		} else
			data->trip1_7 = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_0_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_0_min = %d. \n", __func__, val.val);
		} else
			data->trip1_0_min = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_0_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_0_max = %d. \n", __func__, val.val);
		} else
			data->trip1_0_max = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_1_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_1_min = %d. \n", __func__, val.val);
		} else
			data->trip1_1_min = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_1_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_1_max = %d. \n", __func__, val.val);
		} else
			data->trip1_1_max = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_2_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_2_min = %d. \n", __func__, val.val);
		} else
			data->trip1_2_min = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_2_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_2_max = %d. \n", __func__, val.val);
		} else
			data->trip1_2_max = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_3_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_3_min = %d. \n", __func__, val.val);
		} else
			data->trip1_3_min = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_3_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_3_max = %d. \n", __func__, val.val);
		} else
			data->trip1_3_max = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_4_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_4_min = %d. \n", __func__, val.val);
		} else
			data->trip1_4_min = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_4_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_4_max = %d. \n", __func__, val.val);
		} else
			data->trip1_4_max = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_5_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_5_min = %d. \n", __func__, val.val);
		} else
			data->trip1_5_min = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_5_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_5_max = %d. \n", __func__, val.val);
		} else
			data->trip1_5_max = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_6_min", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_6_min = %d. \n", __func__, val.val);
		} else
			data->trip1_6_min = val.val;

		type = script_get_item("bat_ths_para", "bat_ths_trip1_6_max", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err bat_ths_trip1_6_max = %d. \n", __func__, val.val);
		} else
			data->trip1_6_max = val.val;

		ret = 0;
	} else {
		pr_err("%s: bat_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;
}

/********************************* BAT END ************************************/

/********************************** MOTOR *************************************/

/**
 * motor_free_platform_resource - free ths related resource
 * return value:
 */
static void motor_free_platform_resource(enum input_sensor_type *motor_type)
{
	struct motor_config_info *data = container_of(motor_type,
						struct motor_config_info, input_type);
	if (0 != data->motor_gpio.gpio) {
		gpio_free(data->motor_gpio.gpio);
	}

	return;
}

/**
 * motor_init_platform_resource - initialize platform related resource
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
static int motor_init_platform_resource(enum input_sensor_type *motor_type)
{
	struct motor_config_info *data = container_of(motor_type,
						struct motor_config_info, input_type);
	if (0 != data->motor_gpio.gpio) {
		if(0 != gpio_request(data->motor_gpio.gpio, "vibe")) {
			pr_err("ERROR: vibe Gpio_request is failed\n");
			goto exit;
		}
		gpio_direction_output(data->motor_gpio.gpio, data->vibe_off);
	}

	return 0;
exit:
	return -1;
}

/**
 * motor_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:
 *                    = 0; success;
 *                    < 0; err
 */
static int motor_fetch_sysconfig_para(enum input_sensor_type *motor_type)
{
	script_item_u	val;
	script_item_value_type_e  type;
	struct motor_config_info *data = container_of(motor_type,
					struct motor_config_info, input_type);

	type = script_get_item("motor_para", "motor_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s script_parser_fetch \"motor_para\" motor_used = %d\n",
				__FUNCTION__, val.val);
		goto script_get_err;
	}
	data->motor_used = val.val;

	if(!data->motor_used) {
		pr_err("%s motor is not used in config\n", __FUNCTION__);
		goto script_get_err;
	}

	type = script_get_item("motor_para", "motor_shake", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("no motor_shake, ignore it!");
	} else {
		data->motor_gpio = val.gpio;
		data->vibe_off = val.gpio.data;
	}

	type = script_get_item("motor_para", "motor_ldo", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_STR != type)
		pr_err("no ldo for moto, ignore it\n");
	else
		data->ldo = val.str;

	type = script_get_item("motor_para", "motor_ldo_voltage", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("no ldo moto voltage config, ignore it\n");
	} else
		data->ldo_voltage = val.val;

	return 0;
script_get_err:
	pr_notice("=========script_get_err============\n");
	return -1;
}

/******************************** MOTOR END ***********************************/


static int (* const fetch_sysconfig_para[])(enum input_sensor_type *input_type) = {
	ctp_fetch_sysconfig_para,
	gsensor_fetch_sysconfig_para,
	gyr_fetch_sysconfig_para,
	e_compass_fetch_sysconfig_para,
	ls_fetch_sysconfig_para,
	ir_fetch_sysconfig_para,
	ths_fetch_sysconfig_para,
	motor_fetch_sysconfig_para,
	bat_fetch_sysconfig_para
};

static int (*init_platform_resource[])(enum input_sensor_type *input_type) = {
	ctp_init_platform_resource,
	gsensor_init_platform_resource,
	gyr_init_platform_resource,
	e_compass_init_platform_resource,
	ls_init_platform_resource,
	ir_init_platform_resource,
	ths_init_platform_resource,
	motor_init_platform_resource,
	bat_init_platform_resource
};

static void (*free_platform_resource[])(enum input_sensor_type *input_type) = {
	ctp_free_platform_resource,
	gsensor_free_platform_resource,
	gyr_free_platform_resource,
	e_compass_free_platform_resource,
	ls_free_platform_resource,
	ir_free_platform_resource,
	ths_free_platform_resource,
	motor_free_platform_resource,
	bat_free_platform_resource
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
			break;
		case LS_TYPE:
			break;
		case IR_TYPE:
			break;
		default:
			break;
	}
	if ((enable != 0) && (enable != 1)) {
		return ret;
	}
	if(ldo) {
		if(enable) {
			regulator_enable(ldo);
		} else {
			if (regulator_is_enabled(ldo))
				regulator_disable(ldo);
		}
	} else if(power_io) {
		if(enable) {
			__gpio_set_value(power_io,1);
		} else {
			__gpio_set_value(power_io,0);
		}
	}
	return 0;
}
EXPORT_SYMBOL(input_set_power_enable);
/**
 * input_set_int_enable - input set irq enable
 * Input:
 * 	type:
 *      enable:
 * return value: 0 : success
 *               -EIO :  i/o err.
 */
int input_set_int_enable(enum input_sensor_type *input_type, u32 enable)
{
	int ret = -1;
	u32 irq_number = 0;
	void *data = NULL;

	switch (*input_type) 
	{
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
	case IR_TYPE:
		break;
	default:
		break;
	}
	
	if ((enable != 0) && (enable != 1)) {
		return ret;
	}
	if (1 == enable)
		enable_irq(irq_number);
	else
		disable_irq_nosync(irq_number);

	return 0;       
}
EXPORT_SYMBOL(input_set_int_enable);

/**
 * input_free_int - input free irq
 * Input:
 * 	type:
 * return value: 0 : success
 *               -EIO :  i/o err.
 */
int input_free_int(enum input_sensor_type *input_type, void *para)
{
	int irq_number = 0;
	void *data = NULL;
	struct device *dev = NULL;
	
	switch (*input_type) 
	{
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
	case IR_TYPE:
		break;
	default:
		break;
	}

	devm_free_irq(dev, irq_number, para);

	return 0;       
}
EXPORT_SYMBOL(input_free_int);

/**
 * input_request_int - input request irq
 * Input:
 * 	type:
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

	switch (*input_type) 
	{
	case CTP_TYPE:
		data = container_of(input_type,
					struct ctp_config_info, input_type);
		irq_number = gpio_to_irq(((struct ctp_config_info *)data)->int_number);
		if (IS_ERR_VALUE(irq_number)) {
			pr_warn("map gpio [%d] to virq failed, errno = %d\n", 
		         	GPIOA(3), irq_number);
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
		if (IS_ERR_VALUE(irq_number)) {
			pr_warn("map gpio [%d] to virq failed, errno = %d\n", 
		         	GPIOA(3), irq_number);
			return -EINVAL;
		}

		dev = ((struct sensor_config_info *)data)->dev;
		break;
	case IR_TYPE:
		break;
	default:
		break;
	}

	/* request virq, set virq type to high level trigger */
	ret = devm_request_irq(dev, irq_number, handle, 
			       trig_type, "PA3_EINT", para);
	if (IS_ERR_VALUE(ret)) {
		pr_warn("request virq %d failed, errno = %d\n", 
		         irq_number, ret);
		return -EINVAL;
	}

	return 0;     
}
EXPORT_SYMBOL(input_request_int);

/**
 * input_free_platform_resource - free platform related resource
 * Input:
 * 	event:
 * return value:
 */
void input_free_platform_resource(enum input_sensor_type *input_type)
{
        (*free_platform_resource[*input_type])(input_type);
	return;
}
EXPORT_SYMBOL(input_free_platform_resource);

/**
 * input_init_platform_resource - initialize platform related resource
 * Input:
 * 	type:
 * return value: 0 : success
 *               -EIO :  i/o err.
 *
 */
int input_init_platform_resource(enum input_sensor_type *input_type)
{
	int ret = -1;
	
	ret = (*init_platform_resource[*input_type])(input_type);
	
	return ret;	
}
EXPORT_SYMBOL(input_init_platform_resource);

/**
 * input_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * Input:
 * 	type:
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
int input_fetch_sysconfig_para(enum input_sensor_type *input_type)
{
	int ret = -1;
	
	ret = (*fetch_sysconfig_para[*input_type])(input_type);
	
	return ret;
}
EXPORT_SYMBOL(input_fetch_sysconfig_para);

/*
 * Copyright (c) 2013-2015 liming@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */
#ifndef _INIT_INPUT_H
#define _INIT_INPUT_H
#include <mach/gpio.h>
//#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>

typedef u32 (*gpio_int_handle)(void *para);

enum input_sensor_type{
	CTP_TYPE,
	GSENSOR_TYPE,
	GYR_TYPE,
	COMPASS_TYPE,
	LS_TYPE,
	IR_TYPE,
	THS_TYPE,
	MOTOR_TYPE,
	BAT_TYPE
};

struct sensor_config_info{
	enum input_sensor_type input_type;
	int sensor_used;
	__u32 twi_id;
	u32 int_number;
	struct gpio_config irq_gpio;
	char* ldo;
	struct device *dev;
	struct pinctrl *pinctrl;
};

struct ir_config_info{
	enum input_sensor_type input_type;
	int ir_used;
	int power_key;
	struct gpio_config ir_gpio;
	struct device *dev;
	struct pinctrl *pinctrl;
};

struct ctp_config_info{
	enum input_sensor_type input_type;
	int ctp_used;
	__u32 twi_id;
	char * name;
	int screen_max_x;
	int screen_max_y;
	int revert_x_flag;
	int revert_y_flag;
	int exchange_x_y_flag;
	u32 int_number;
	unsigned char device_detect;
	char *ctp_power;
	u32 ctp_power_vol;
	struct gpio_config ctp_power_io;
	struct regulator *ctp_power_ldo;
	struct gpio_config irq_gpio;
	struct gpio_config wakeup_gpio; 
#ifdef TOUCH_KEY_LIGHT_SUPPORT 
	struct gpio_config key_light_gpio;
#endif
	struct device *dev;
	struct pinctrl *pinctrl;
};

struct ths_config_info{
	enum input_sensor_type input_type;
	int ths_used;
	int ths_trend;
	int trip1_count;
	int trip1_0;
	int trip1_1;
	int trip1_2;
	int trip1_3;
	int trip1_4;
	int trip1_5;
	int trip1_6;
	int trip1_7;
	int trip1_0_min;
	int trip1_0_max;
	int trip1_1_min;
	int trip1_1_max;
	int trip1_2_min;
	int trip1_2_max;
	int trip1_3_min;
	int trip1_3_max;
	int trip1_4_min;
	int trip1_4_max;
	int trip1_5_min;
	int trip1_5_max;
	int trip1_6_min;
	int trip1_6_max;
	int trip2_count;
	int trip2_0;
};

struct motor_config_info{
	enum input_sensor_type input_type;
	int motor_used;
	int vibe_off;
	u32 ldo_voltage;
	char* ldo;
	struct gpio_config motor_gpio;
};

int input_fetch_sysconfig_para(enum input_sensor_type *input_type);
void input_free_platform_resource(enum input_sensor_type *input_type);
int input_init_platform_resource(enum input_sensor_type *input_type);
int input_request_int(enum input_sensor_type *input_type,irq_handler_t handle,
			unsigned long trig_type, void *para);
int input_free_int(enum input_sensor_type *input_type, void *para);
int input_set_int_enable(enum input_sensor_type *input_type, u32 enable);
int input_set_power_enable(enum input_sensor_type *input_type, u32 enable);
#endif

/*
 ******************************************************************************
 *
 * vin_supply.h
 *
 * Hawkview ISP - vin_supply.h module
 *
 * Copyright (c) 2015 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   3.0		  Yang Feng   	2015/12/02	ISP Tuning Tools Support
 *
 ******************************************************************************
 */

#ifndef __VIN__SUBDEV__H__
#define __VIN__SUBDEV__H__

enum pmic_channel {
	IOVDD = 0,
	AVDD,
	DVDD,
	AFVDD,
	FLVDD,
	MAX_POW_NUM,
};

enum gpio_type {
	POWER_EN = 0,
	PWDN,
	RESET,
	AF_PWDN,
	FLASH_EN,
	FLASH_MODE,
	MAX_GPIO_NUM,
};

enum gpio_fun {
	GPIO_INPUT = 0,
	GPIO_OUTPUT = 1,
	GPIO_DISABLE = 7,
};
enum on_off {
	OFF,
	ON,
};

enum power_seq_cmd {
	PWR_OFF = 0,
	PWR_ON = 1,
	STBY_OFF = 2,
	STBY_ON = 3,
};

extern struct vin_core *sd_to_vin_core(struct v4l2_subdev *sd);
extern int vin_set_pmu_channel(struct v4l2_subdev *sd,
			       enum pmic_channel pmic_ch,
			       enum on_off on_off);
extern int vin_set_mclk(struct v4l2_subdev *sd, enum on_off on_off);
extern int vin_set_mclk_freq(struct v4l2_subdev *sd, unsigned long freq);
extern int vin_gpio_write(struct v4l2_subdev *sd, enum gpio_type gpio_type,
			  unsigned int status);
extern int vin_gpio_set_status(struct v4l2_subdev *sd,
			       enum gpio_type gpio_type,
			       unsigned int status);

#endif	/*__VIN__SUBDEV__H__*/

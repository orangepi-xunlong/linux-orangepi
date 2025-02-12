/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
/*
 * cam_sensor_uapi.h.c - Driver uapi for camera sensor driver
 *
 * Copyright (C) 2022 KY Micro Limited
 * All Rights Reserved.
 */
#ifndef __CAM_SENSOR_UAPI_H__
#define __CAM_SENSOR_UAPI_H__

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#define CAM_SNS_MAX_DEV_NUM    3

#define CAM_SENSOR_DEV_NAME "/dev/cam_sensor"

#define CAM_SENSOR_IOC_MAGIC 'I'

typedef enum CAM_SENSOR_IOC {
	SENSOR_IOC_RESET = 1,
	SENSOR_IOC_UNRESET,
	SENSOR_IOC_I2C_WRITE,
	SENSOR_IOC_I2C_READ,
	SENSOR_IOC_I2C_BURST_WRITE,
	SENSOR_IOC_I2C_BURST_READ,
	SENSOR_IOC_GET_INFO,
	SENSOR_IOC_SET_MIPI_CLOCK,
	SENSOR_IOC_SET_POWER_VOLTAGE,
	SENSOR_IOC_SET_POWER_ON,
	SENSOR_IOC_SET_GPIO_ENABLE,
	SENSOR_IOC_SET_MCLK_ENABLE,
	SENSOR_IOC_SET_MCLK_RATE,
} CAM_SENSOR_IOC_E;

typedef unsigned int sns_rst_source_t;
typedef unsigned int sns_mipi_clock_t;

struct regval_tab {
	uint16_t reg;
	uint16_t val;
};

enum sensor_i2c_len {
	I2C_8BIT = 1,
	I2C_16BIT = 2,
	//I2C_24BIT = 3,
	//I2C_32BIT = 4,
};

struct cam_i2c_data {
	//uint8_t twsi_no;
	enum sensor_i2c_len reg_len;
	enum sensor_i2c_len val_len;
	uint8_t addr; /* 7 bit i2c address*/
	struct regval_tab tab;
};

struct cam_burst_i2c_data {
	//uint8_t twsi_no;
	enum sensor_i2c_len reg_len;
	enum sensor_i2c_len val_len;
	uint8_t addr; /* 7 bit i2c address*/
	struct regval_tab *tab;
	uint32_t num; /* the number of sensor regs*/
};

struct cam_sensor_info {
	uint8_t twsi_no;
};

typedef enum {
	SENSOR_REGULATOR_AFVDD,
	SENSOR_REGULATOR_AVDD,
	SENSOR_REGULATOR_DOVDD,
	SENSOR_REGULATOR_DVDD,
} cam_sensor_power_regulator_id;

typedef enum {
	SENSOR_GPIO_PWDN,
	SENSOR_GPIO_RST,
	SENSOR_GPIO_DVDDEN,
	SENSOR_GPIO_DCDCEN,
} cam_sensor_gpio_id;

struct cam_sensor_power {
	cam_sensor_power_regulator_id regulator_id;
	uint32_t voltage;
	uint8_t on;
};

struct cam_sensor_gpio {
	cam_sensor_gpio_id gpio_id;
	uint8_t enable;
};

#define CAM_SENSOR_RESET _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_RESET, sns_rst_source_t)
#define CAM_SENSOR_UNRESET _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_UNRESET, sns_rst_source_t)
#define CAM_SENSOR_I2C_WRITE _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_I2C_WRITE, struct cam_i2c_data)
#define CAM_SENSOR_I2C_READ _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_I2C_READ, struct cam_i2c_data)
#define CAM_SENSOR_I2C_BURST_WRITE _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_I2C_BURST_WRITE, struct cam_burst_i2c_data)
#define CAM_SENSOR_I2C_BURST_READ _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_I2C_BURST_READ, struct cam_burst_i2c_data)
#define CAM_SENSOR_GET_INFO _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_GET_INFO, struct cam_sensor_info)
#define CAM_SENSOR_SET_MIPI_CLOCK _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_SET_MIPI_CLOCK, sns_mipi_clock_t)

#define CAM_SENSOR_SET_POWER_VOLTAGE _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_SET_POWER_VOLTAGE, struct cam_sensor_power)
#define CAM_SENSOR_SET_POWER_ON _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_SET_POWER_ON, struct cam_sensor_power)
#define CAM_SENSOR_SET_GPIO_ENABLE _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_SET_GPIO_ENABLE, struct cam_sensor_gpio)
#define CAM_SENSOR_SET_MCLK_ENABLE _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_SET_MCLK_ENABLE, uint32_t)
#define CAM_SENSOR_SET_MCLK_RATE _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_SET_MCLK_RATE, uint32_t)

#if defined(__cplusplus)
}
#endif

#endif /* __CAM_SENSOR_UAPI_H__ */

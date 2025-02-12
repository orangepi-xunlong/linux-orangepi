#ifndef ICM42607_H
#define ICM42607_H

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include "input-polldev.h"
#include <linux/mutex.h>

#define ICM42607_DEVICE_CONFIG		0x01
#define ICM42607_SIGNAL_PATH_RESET	0x02
#define ICM42607_ACCEL_DATA_X1		0x0B
#define ICM42607_GYRO_DATA_X1		0x11
#define ICM42607_PWR_MGMT0			0x1F
#define ICM42607_GYRO_CONFIG0		0x20
#define ICM42607_ACCEL_CONFIG0		0x21
#define ICM42607_INTF_CONFIG0		0x35
#define ICM42607_INTF_CONFIG1		0x36
#define ICM42607_INT_STATUS			0x3A
#define ICM42607_WHO_AM_I			0x75

struct icm42607_device
{
	struct i2c_client *client;
	struct input_polled_dev *input_dev;
	struct mutex mutex;
	short  accel_status;
	short  gyro_status;
	unsigned int accel_poll;
	unsigned int gyro_poll;
	unsigned int poll_time;
	short accel_data[3];
	short gyro_data[3];
	short accel_offset[3];
	short gyro_offset[3];
};

#endif


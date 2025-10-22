/*
 * @section LICENSE
 * Copyright (c) 2022 Bosch Sensortec GmbH All Rights Reserved
 *
 * This software program is licensed subject to the GNU General
 * Public License (GPL).Version 2,June 1991,
 * available at http://www.fsf.org/copyleft/gpl.html
 *
 * @file	 bmi323_driver.c
 * @date	 10/10/2022
 * @version  1.6.0
 *
 * @brief	BMI323 Linux Driver
 */

/*********************************************************************/
/* System header files */
/*********************************************************************/
#include <linux/types.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>

/*********************************************************************/
/* Own header files */
/*********************************************************************/
#include "bmi323_driver.h"
#include "bs_log.h"

/*********************************************************************/
/* Local macro definitions */
/*********************************************************************/
#define DRIVER_VERSION		"1.6.0"
#define MS_TO_US(msec)		UINT32_C((msec) * 1000)

/*********************************************************************/
/* Global data */
/*********************************************************************/
struct bmi3_feature_enable feature = {0};
struct bmi3_map_int map_int = {0};

/**
 * bmi3_delay_us - Adds a delay in units of microsecs.
 *
 * @usec: Delay value in microsecs.
 */
static void bmi3_delay_us(u32 usec, void *intf_ptr)
{
	if (usec <= (MS_TO_US(20)))

		/* Delay range of usec to usec + 1 millisecs
		 * required due to kernel limitation
		 */
		usleep_range(usec, usec + 1000);
	else
		msleep(usec/1000);
}

/**
 * check_error - check error code and print error message if err is non 0.
 *
 * @print_msg	: print message to print on if err is not 0.
 * @err			: error code return to be checked.
 */
static void check_error(char *print_msg, int err)
{
	if (err)
		PERR("%s failed with return code:%d\n", print_msg, err);
}

/**
 * chip_id_show - sysfs callback for reading the chip id of the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t chip_id_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	u8 chip_id[2] = {0};
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	err = bmi323_get_regs(BMI3_REG_CHIP_ID, chip_id, 2,
						&client_data->device);
	check_error("read chip id", err);
	return scnprintf(buf, PAGE_SIZE, "chip_id=0x%x rev_id:0x%x\n",
												chip_id[0], chip_id[1]);
}

/**
 * softreset_store - sysfs write callback which performs the
 * soft rest in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t softreset_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long soft_reset;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	/* Base of decimal number system is 10 */
	err = kstrtoul(buf, 10, &soft_reset);
	check_error("softreset: input receive", err);

	if (soft_reset)
		err = bmi323_soft_reset(&client_data->device);
	check_error("softreset: trigger", err);

	return count;
}

/**
 * acc_mode_show - sysfs callback which tells accelerometer mode.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t acc_mode_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int err;
	struct bmi3_sens_config config;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	config.type = BMI323_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_mode: get config", err);

	return scnprintf(buf, PAGE_SIZE, "acc_mode:%d\n", config.cfg.acc.acc_mode);
}

/**
 * acc_mode_store - sysfs callback which sets accelerometer mode.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 * Accelerometer will not be disabled unless all the features related to
 * accelerometer are disabled.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t acc_mode_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	struct bmi3_sens_config config;
	unsigned long op_mode;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	err = kstrtoul(buf, 10, &op_mode);
	check_error("acc_mode: input receive", err);

	if (op_mode > 7 || op_mode == 1 || op_mode == 5 || op_mode == 6) {
		PDEBUG("acc_mode: Invalid input %d received\n", (u8)op_mode);
		return -EINVAL;
	}
	mutex_lock(&client_data->lock);
	config.type = BMI323_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_mode: get sensor config", err);
	config.cfg.acc.acc_mode = (u8)op_mode;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("acc_mode: set sensor config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * acc_range_show - sysfs read callback which gives the
 * accelerometer range which is set in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t acc_range_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	mutex_lock(&client_data->lock);
	config.type = BMI3_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_range: get sensor config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "acc_range:%d\n", config.cfg.acc.range);
}

/**
 * acc_range_store - sysfs write callback which sets the
 * accelerometer range to be set in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t acc_range_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int err;
	unsigned long range;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	err = kstrtoul(buf, 10, &range);
	check_error("acc_range: input receive", err);

	if (range > 3) {
		PDEBUG("acc_range Invalid input : %d\n", (u8)range);
		return -EINVAL;
	}

	mutex_lock(&client_data->lock);
	config.type = BMI3_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_range: get sensor config", err);
	config.cfg.acc.range = (u8)range;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("acc_range: set sensor config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * acc_bw_show - sysfs read callback which gives the
 * accelerometer bandwidth which is set in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t acc_bw_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	mutex_lock(&client_data->lock);
	config.type = BMI3_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_bw: get sensor config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "acc_bw:%d\n", config.cfg.acc.bwp);
}

/**
 * acc_bw_store - sysfs write callback which sets the
 * accelerometer bandwidth to be set in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t acc_bw_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int err;
	unsigned long bw;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	err = kstrtoul(buf, 10, &bw);
	check_error("acc_bw: input receive", err);

	if (bw > 1) {
		PDEBUG("acc_bw Invalid input : %d\n", (u8)bw);
		return -EINVAL;
	}
	mutex_lock(&client_data->lock);
	config.type = BMI3_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_bw: get sensor config", err);
	config.cfg.acc.bwp = (u8)bw;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("acc_bw: set sensor config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * acc_avg_num_show - sysfs read callback which gives the
 * accelerometer no of samples to average which is set in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t acc_avg_num_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	mutex_lock(&client_data->lock);
	config.type = BMI3_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_avg_num: get sensor config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "acc_avg_num:%d\n",
											config.cfg.acc.avg_num);
}

/**
 * acc_avg_num_store - sysfs write callback which sets the
 * accelerometer no of samples to average to be set in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t acc_avg_num_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int err;
	unsigned long avg_num;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	err = kstrtoul(buf, 10, &avg_num);
	check_error("acc_avg_num: input receive", err);

	if (avg_num > 7) {
		PDEBUG("acc_avg_num Invalid input : %d\n", (u8)avg_num);
		return -EINVAL;
	}

	mutex_lock(&client_data->lock);
	config.type = BMI3_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_avg_num: get sensor config", err);
	config.cfg.acc.avg_num = (u8)avg_num;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("acc_avg_num: set sensor config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * acc_odr_show - sysfs read callback which gives the
 * accelerometer output data rate of the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t acc_odr_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	mutex_lock(&client_data->lock);
	config.type = BMI3_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_odr: get sensor config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "acc_odr:%d\n", config.cfg.acc.odr);
}

/**
 * acc_odr_store - sysfs write callback which sets the
 * accelerometer output data rate in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t acc_odr_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int err;
	unsigned long odr;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	err = kstrtoul(buf, 10, &odr);
	check_error("acc_odr: acc_odr input receive", err);

	if (odr < 1 || odr > 14) {
		PDEBUG("acc_odr Invalid input : %d\n", (u8)odr);
		return -EINVAL;
	}
	mutex_lock(&client_data->lock);
	config.type = BMI3_ACCEL;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("acc_odr: get sensor config", err);
	config.cfg.acc.odr = (u8)odr;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("acc_odr: set sensor config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * acc_val_show - sysfs read callback which gives the
 * raw accelerometer value from the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t acc_val_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sensor_data sensor_data;

	sensor_data.type = BMI3_ACCEL;
	mutex_lock(&client_data->lock);
	err = bmi323_get_sensor_data(&sensor_data, 1, &client_data->device);
	check_error("acc_odr: get sensor config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "ACC X:%d Y:%d Z:%d\n",
			sensor_data.sens_data.acc.x,
			sensor_data.sens_data.acc.y,
			sensor_data.sens_data.acc.z);
}

/**
 * gyr_mode_show - sysfs callback which tells gyroscope mode.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t gyr_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{

	int err;
	struct bmi3_sens_config config;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_mode: get sensor config", err);

	return scnprintf(buf, PAGE_SIZE, "%d\n", config.cfg.gyr.gyr_mode);
}

/**
 * gyr_mode_store - sysfs callback which sets gyroscope mode.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t gyr_mode_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	int err;
	struct bmi3_sens_config config;
	unsigned long op_mode;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	err = kstrtoul(buf, 10, &op_mode);
	check_error("gyr_mode: input receive", err);

	if (op_mode > 7 || op_mode == 5 || op_mode == 6) {
		PDEBUG("gyr_mode Invalid input : %d\n", (u8)op_mode);
		return -EINVAL;
	}
	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_mode: get sensor config", err);
	config.cfg.gyr.gyr_mode = (u8)op_mode;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_mode: set sensor config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * gyr_val_show - sysfs read callback which gives the
 * raw gyroscope value from the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t gyr_val_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sensor_data sensor_data;

	sensor_data.type = BMI3_GYRO;
	mutex_lock(&client_data->lock);
	err = bmi323_get_sensor_data(&sensor_data, 1, &client_data->device);
	check_error("gyr_val: get gyro sensor data", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "GYRO X:%d Y:%d Z:%d\n",
			sensor_data.sens_data.gyr.x,
			sensor_data.sens_data.gyr.y,
			sensor_data.sens_data.gyr.z);
}

/**
 * gyr_range_show - sysfs read callback which gives the
 * gyroscope range of the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t gyr_range_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_range: get sensor config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "gyr_range:%d\n", config.cfg.gyr.range);
}

/**
 * gyr_range_store - sysfs write callback which sets the
 * gyroscope range in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t gyr_range_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long range;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	err = kstrtoul(buf, 10, &range);
	check_error("gyr_range input receive", err);
	if (range > 4) {
		PDEBUG("gyr_range Invalid input %d received", (u8)range);
		return -EINVAL;
	}
	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_range: get config", err);
	config.cfg.gyr.range = (u8)range;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_range: set config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * gyr_odr_show - sysfs read callback which gives the
 * gyroscope output data rate of the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t gyr_odr_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_odr: get config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "gyr_odr:%d\n", config.cfg.gyr.odr);
}

/**
 * gyr_odr_store - sysfs write callback which sets the
 * gyroscope output data rate in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t gyr_odr_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	int err;
	unsigned long odr;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	err = kstrtoul(buf, 10, &odr);
	check_error("gyr_odr input receive", err);
	if (odr < 1 || odr > 14) {
		PDEBUG("gyr_odr Invalid input %d provided\n", (u8)odr);
		return -EINVAL;
	}
	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_odr: get config", err);
	config.cfg.gyr.odr = (u8)odr;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_odr: set config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * gyr_bw_show - sysfs read callback which gives the
 * gyroscope bandwidth of the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t gyr_bw_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_bw: get config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "gyr_bw:%d\n", config.cfg.gyr.bwp);
}

/**
 * gyr_bw_store - sysfs write callback which sets the
 * gyroscope bandwidth in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t gyr_bw_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long bw;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	err = kstrtoul(buf, 10, &bw);
	check_error("gyr_bw input receive", err);
	if (bw > 1) {
		PDEBUG("Invalid input : %d\n", (u8)bw);
		return -EINVAL;
	}
	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_bw: get config", err);
	config.cfg.gyr.bwp = (u8)bw;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_bw: set config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * gyr_avg_num_show - sysfs read callback which gives the
 * gyroscope no of average samples of the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t gyr_avg_num_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_avg_num: get config", err);
	mutex_unlock(&client_data->lock);

	return scnprintf(buf, PAGE_SIZE, "gyr_avg_num:%d\n",
											config.cfg.gyr.avg_num);
}

/**
 * gyr_avg_num_store - sysfs write callback which sets the
 * gyroscope no of average samples in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t gyr_avg_num_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long avg_num;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_sens_config config;

	err = kstrtoul(buf, 10, &avg_num);
	check_error("gyr_avg_num input receive", err);
	if (avg_num > 7) {
		PDEBUG("gyr_avg_num: Invalid input %d provided\n", (u8)avg_num);
		return -EINVAL;
	}
	mutex_lock(&client_data->lock);
	config.type = BMI3_GYRO;
	err = bmi323_get_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_avg_num: get config", err);
	config.cfg.gyr.avg_num = (u8)avg_num;
	err = bmi323_set_sensor_config(&config, 1, &client_data->device);
	check_error("gyr_avg_num: set config", err);
	mutex_unlock(&client_data->lock);

	return count;
}

/**
 * temperature_data_show - sysfs read callback which gives the
 * temprature data value from the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 *
 * Return: Number of characters returned.
 */
static ssize_t temperature_data_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	u16 temperature_data = 0;

	mutex_lock(&client_data->lock);
	err = bmi323_get_temperature_data(&temperature_data, &client_data->device);
	mutex_unlock(&client_data->lock);
	check_error("temperature_data: get temperature data", err);

	return scnprintf(buf, PAGE_SIZE, "temperature data : %d\n",
			temperature_data);
}

/**
 * sensor_time_show - sysfs read callback which gives the
 * sensor time from the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 *
 * Return: Number of characters returned.
 */
static ssize_t sensor_time_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int err;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	u32 sensor_time = 0;

	mutex_lock(&client_data->lock);
	err = bmi323_get_sensor_time(&sensor_time, &client_data->device);
	mutex_unlock(&client_data->lock);
	check_error("sensor_time: get sensor time", err);

	return scnprintf(buf, PAGE_SIZE, "sensor time : %d\n",
			sensor_time);
}

/**
 * sensor_init_store - sysfs write callback which loads the
 * config stream in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t sensor_init_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	unsigned long choose = 0;
	int err = 0;
	u8 load_status = 0;
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	err = kstrtoul(buf, 10, &choose);
	check_error("sensor_init: receive input", err);
	if (choose != 1) {
		PERR("sensor_init: Invalid input provided, valid input is 1");
		return -EINVAL;
	}
	err = bmi323_init(&client_data->device);
	check_error("sensor_init: bmi323_init", err);

	bmi3_delay_us(MS_TO_US(20),
					&client_data->device.intf_ptr);
	/* Get the status */
	err = bmi323_get_regs(BMI3_REG_INT_STATUS_INT1, &load_status, 1,
						&client_data->device);
	check_error("sensor_init: int status read after load", err);
	PDEBUG("Init success %d\n", err);
	client_data->sensor_init_state = 1;

	return count;
}

/**
 * sensor_init_show - sysfs read callback for bmi3x0 sensor init.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t sensor_init_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	if (client_data->sensor_init_state)
		return scnprintf(buf, PAGE_SIZE, "sensor already initilized\n");
	else
		return scnprintf(buf, PAGE_SIZE, "sensor not initilized\n");
}

/**
 * reg_sel_show - sysfs read callback which provides the register
 * address selected.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t reg_sel_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);

	return scnprintf(buf, PAGE_SIZE, "reg=0X%02X, len=%d\n",
		client_data->reg_sel, client_data->reg_len);
}

/**
 * reg_sel_store - sysfs write callback which stores the register
 * address to be selected.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t reg_sel_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	int err;

	mutex_lock(&client_data->lock);
	err = sscanf(buf, "%11X %11d",
		&client_data->reg_sel, &client_data->reg_len);
	if ((err != 2) || (client_data->reg_len > 128)
		|| (client_data->reg_sel > 127)) {
		PERR("Invalid argument");
		mutex_unlock(&client_data->lock);
		return -EINVAL;
	}
	client_data->reg_len *= 2;
	mutex_unlock(&client_data->lock);
	return count;
}

/**
 * reg_val_show - sysfs read callback which shows the register
 * value which is read from the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t reg_val_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	int err;
	u8 reg_data[256];
	int i;
	int pos;

	mutex_lock(&client_data->lock);
	if ((client_data->reg_len > 256) || (client_data->reg_sel > 127)) {
		PERR("Invalid argument");
		mutex_unlock(&client_data->lock);
		return -EINVAL;
	}

	err = bmi323_get_regs(client_data->reg_sel, reg_data,
				client_data->reg_len, &client_data->device);
	check_error("reg_val: read register contents", err);
	pos = 0;
	for (i = 0; i < client_data->reg_len; ++i) {
		pos += scnprintf(buf + pos, 16, "%02X", reg_data[i]);
		buf[pos++] = (i + 1) % 16 == 0 ? '\n' : ' ';
	}
	mutex_unlock(&client_data->lock);
	if (buf[pos - 1] == ' ')
		buf[pos - 1] = '\n';
	return pos;
}

/**
 * reg_val_store - sysfs write callback which stores the register
 * value which is to be written in the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t reg_val_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	int err;
	u8 reg_data[256] = {0};
	int i, j, status, digit;

	status = 0;
	mutex_lock(&client_data->lock);
	/* Lint -save -e574 */
	for (i = j = 0; i < count && j < client_data->reg_len; ++i) {
		/* Lint -restore */
		if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t' ||
			buf[i] == '\r') {
			status = 0;
			++j;
			continue;
		}
		digit = buf[i] & 0x10 ? (buf[i] & 0xF) : ((buf[i] & 0xF) + 9);
		PDEBUG("digit is %d\n", digit);
		switch (status) {
		case 2:
			++j; /* Fall thru */
		case 0:
			reg_data[j] = digit;
			status = 1;
			break;
		case 1:
			reg_data[j] = reg_data[j] * 16 + digit;
			status = 2;
			break;
		}
	}
	if (status > 0)
		++j;
	if (j > client_data->reg_len)
		j = client_data->reg_len;
	else if (j < client_data->reg_len) {
		PERR("Invalid argument");
		mutex_unlock(&client_data->lock);
		return -EINVAL;
	}
	PDEBUG("Reg data read as");
	for (i = 0; i < j; ++i)
		PDEBUG("%d\n", reg_data[i]);
	err = client_data->device.write(client_data->reg_sel, reg_data,
					client_data->reg_len,
					client_data->device.intf_ptr);
	mutex_unlock(&client_data->lock);
	check_error("reg_val: write register contents", err);
	return count;
}

/**
 * driver_version_show - sysfs read callback which provides the driver
 * version.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t driver_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE,
		"Driver version: %s\n", DRIVER_VERSION);
}

/**
 * avail_sensor_show - sysfs read callback which provides the sensor-id
 * to the user.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t avail_sensor_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "323\n");
}

/**
 * axis_remap_show - sysfs read callback to reads axis remap information.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as output.
 *
 * Return: Number of characters returned.
 */
static ssize_t axis_remap_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_axes_remap remapped_axis = {0};
	int err = 0;

	err = bmi323_get_remap_axes(&remapped_axis, &client_data->device);
	check_error("get axis remap", err);

	return scnprintf(buf, PAGE_SIZE,
			"axis_map:%d invert_x:%d invert_y:%d invert_z:%d\n",
			remapped_axis.axis_map, remapped_axis.invert_x,
			remapped_axis.invert_y, remapped_axis.invert_z);
}

/**
 * axis_remap_store - sysfs write callback which performs the
 * axis remapping of the sensor.
 *
 * @dev: Device instance
 * @attr: Instance of device attribute file
 * @buf: Instance of the data buffer which serves as input.
 * @count: Number of characters in the buffer `buf`.
 *
 * Return: Number of characters used from buffer `buf`, which equals count.
 */
static ssize_t axis_remap_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned int data[4];
	struct input_dev *input = to_input_dev(dev);
	struct bmi3_client_data *client_data = input_get_drvdata(input);
	struct bmi3_axes_remap remapped_axis = {0};
	int err = 0;

	err = sscanf(buf, "%11d %11d %11d %11d\n",
		&data[0], &data[1], &data[2], &data[3]);

	if (err != 4) {
		PERR("Invalid argument");
		PERR("use echo axis_map invert_x invert_y invert_z > axis_remap");
		return -EINVAL;
	}

	err = bmi323_get_remap_axes(&remapped_axis, &client_data->device);
	check_error("get_remap_axes", err);

	remapped_axis.axis_map = (u8)data[0];
	remapped_axis.invert_x = (u8)data[1];
	remapped_axis.invert_y = (u8)data[2];
	remapped_axis.invert_z = (u8)data[3];

	PDEBUG("axis_map=%d, invert_x=%d, invert_y=%d invert_z=%d\n",
		remapped_axis.axis_map, remapped_axis.invert_x,
		remapped_axis.invert_y, remapped_axis.invert_z);

	err = bmi323_set_remap_axes(remapped_axis, &client_data->device);
	check_error("set axis remap", err);
	return count;
}

static DEVICE_ATTR_RO(chip_id);
static DEVICE_ATTR_RW(acc_mode);
static DEVICE_ATTR_RO(acc_val);
static DEVICE_ATTR_RW(acc_range);
static DEVICE_ATTR_RW(acc_bw);
static DEVICE_ATTR_RW(acc_avg_num);
static DEVICE_ATTR_RW(acc_odr);
static DEVICE_ATTR_RW(gyr_mode);
static DEVICE_ATTR_RO(gyr_val);
static DEVICE_ATTR_RW(gyr_range);
static DEVICE_ATTR_RW(gyr_odr);
static DEVICE_ATTR_RW(gyr_bw);
static DEVICE_ATTR_RW(gyr_avg_num);
static DEVICE_ATTR_RO(avail_sensor);
static DEVICE_ATTR_RW(reg_sel);
static DEVICE_ATTR_RW(reg_val);
static DEVICE_ATTR_RO(driver_version);
static DEVICE_ATTR_RW(sensor_init);
static DEVICE_ATTR_WO(softreset);
static DEVICE_ATTR_RW(axis_remap);
static DEVICE_ATTR_RO(temperature_data);
static DEVICE_ATTR_RO(sensor_time);

static struct attribute *bmi3_attributes[] = {
	&dev_attr_chip_id.attr,
	&dev_attr_acc_mode.attr,
	&dev_attr_acc_val.attr,
	&dev_attr_acc_range.attr,
	&dev_attr_acc_odr.attr,
	&dev_attr_acc_bw.attr,
	&dev_attr_acc_avg_num.attr,
	&dev_attr_gyr_mode.attr,
	&dev_attr_gyr_val.attr,
	&dev_attr_gyr_range.attr,
	&dev_attr_gyr_odr.attr,
	&dev_attr_gyr_bw.attr,
	&dev_attr_gyr_avg_num.attr,
	&dev_attr_sensor_init.attr,
	&dev_attr_avail_sensor.attr,
	&dev_attr_driver_version.attr,
	&dev_attr_reg_sel.attr,
	&dev_attr_reg_val.attr,
	&dev_attr_softreset.attr,
	&dev_attr_axis_remap.attr,
	&dev_attr_temperature_data.attr,
	&dev_attr_sensor_time.attr,
	NULL
};

static struct attribute_group bmi3_attribute_group = {
	.attrs = bmi3_attributes
};

/**
 * bmi3_acc_input_init - Register the accelerometer input device in the
 * system.
 * @client_data : Instance of client data.
 *
 * Return : Status of the function.
 * * 0 - OK
 * * Any Negative value - Error.
 */
static int bmi3_acc_input_init(struct bmi3_client_data *client_data)
{
	struct input_dev *dev;
	int err = 0;

	dev = input_allocate_device();
	if (dev == NULL) {
		PERR("error acc_input dev allocate is NULL\n");
		return -ENOMEM;
	}

	dev->name = SENSOR_NAME;
	dev->id.bustype = BUS_SPI;
	input_set_capability(dev, EV_ABS, ABS_MISC);
	input_set_capability(dev, EV_MSC, REL_HW_STATUS);
	input_set_drvdata(dev, client_data);
	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	client_data->acc_input = dev;
	return 0;
}

/**
 * bmi3_acc_input_destroy - Un-register the Accelerometer input device from
 * the system.
 *
 * @client_data :Instance of client data.
 */
static void bmi3_acc_input_destroy(struct bmi3_client_data *client_data)
{
	struct input_dev *dev = client_data->acc_input;

	input_unregister_device(dev);
}

int bmi3_probe(struct bmi3_client_data *client_data, struct device *dev)
{
	int err = 0;

	dev_set_drvdata(dev, client_data);
	if (client_data == NULL) {
		PERR("client_data NULL\n");
		return -EINVAL;
	}

	client_data->dev = dev;
	client_data->device.delay_us = bmi3_delay_us;
	/*lint -e86*/
	mutex_init(&client_data->lock);
	/*lint +e86*/
	err = bmi3_acc_input_init(client_data);
	check_error("bmi3 probe: acc input init", err);
	err = sysfs_create_group(&client_data->acc_input->dev.kobj,
			&bmi3_attribute_group);
	check_error("bmi3 probe: sysfs node creation", err);

	PINFO("sensor %s probed successfully", SENSOR_NAME);
	return err;
}
EXPORT_SYMBOL(bmi3_probe);

/**
 * bmi3_remove - This function removes the driver from the device.
 * @dev : Instance of the device.
 *
 * Return : Status of the suspend function.
 * * 0 - OK.
 * * Negative value : Error.
 */
int bmi3_remove(struct device *dev)
{
	int err = 0;
	struct bmi3_client_data *client_data = dev_get_drvdata(dev);

	if (client_data != NULL) {
		bmi3_delay_us(MS_TO_US(BMI3_I2C_WRITE_DELAY_TIME),
						&client_data->device.intf_ptr);
		sysfs_remove_group(&client_data->acc_input->dev.kobj,
				&bmi3_attribute_group);
		bmi3_acc_input_destroy(client_data);
		kfree(client_data);
	}
	return err;
}
/* Lint -save -e19 */
EXPORT_SYMBOL(bmi3_remove);
/* Lint -restore +e19*/

MODULE_LICENSE("GPL v2");

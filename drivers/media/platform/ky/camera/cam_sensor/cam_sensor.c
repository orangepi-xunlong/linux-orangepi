/* SPDX-License-Identifier: GPL-2.0 */
/*
 *cam_sensor.c - camera sensor driver
 *
 * Copyright (C) 2023 KY Micro Limited
 * All Rights Reserved.
 */
/* #define DEBUG */

#include <linux/atomic.h>
#include <linux/compat.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/delay.h>

#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/timekeeping.h>
#include <linux/regulator/driver.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>

#include "cam_dbg.h"
#include <media/x1/cam_sensor_uapi.h>
#include "cam_sensor.h"
#include "../cam_ccic/ccic_drv.h"

/* Standard module information, edit as appropriate */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("KY Inc.");
MODULE_DESCRIPTION("KY Camera Sensor Driver");

#undef CAM_MODULE_TAG
#define CAM_MODULE_TAG	CAM_MDL_SNR

#define DRIVER_NAME "cam_sensor"

#define BURST_I2C_REG_SIZE 1

static struct cam_sensor_device *g_sdev[CAM_SNS_MAX_DEV_NUM];
static int camsnr_major;
//static struct cdev camsnr_cdev;
static struct class *camsnr_class;
struct gpio_desc *gpio_dvdden = NULL;
struct gpio_desc *gpio_dcdcen = NULL;

#define SENSOR_DRIVER_CHECK_POINTER(ptr)			\
	do {							\
		if (NULL == (ptr)) {				\
			cam_err("%s: line %d: Invalid Pointer!", __func__, __LINE__); \
			return -EINVAL;				\
		}						\
    } while (0)

/*********************************************************************************/
#define SENSOR_MCLK_CLK_RATE    24000000
static int cam_sensor_power_set(struct cam_sensor_device *msnr_dev, u32 on)
{
	int ret = 0;

	SENSOR_DRIVER_CHECK_POINTER(msnr_dev);

	if (IS_ERR_OR_NULL(msnr_dev->pwdn) && IS_ERR_OR_NULL(msnr_dev->rst)) {
		cam_err("%s: sensor%d pwdn or reset gpio is error", __func__,
			msnr_dev->id);
		return -EINVAL;
	}

	if (on) {
		if (msnr_dev->mclk) {
			ret = clk_prepare_enable(msnr_dev->mclk);
			if (ret < 0)
				goto avdd_err;
			ret = clk_set_rate(msnr_dev->mclk, SENSOR_MCLK_CLK_RATE);
			if (ret)
				return ret;
		}

		if (!IS_ERR_OR_NULL(msnr_dev->avdd)) {
			regulator_set_voltage(msnr_dev->avdd, 2800000, 2800000);
			ret = regulator_enable(msnr_dev->avdd);
			if (ret < 0)
				goto avdd_err;
		}
		if (!IS_ERR_OR_NULL(msnr_dev->dovdd)) {
			regulator_set_voltage(msnr_dev->dovdd, 1800000, 1800000);
			ret = regulator_enable(msnr_dev->dovdd);
			if (ret < 0)
				goto dovdd_err;
		}
		if (!IS_ERR_OR_NULL(msnr_dev->dvdd)) {
			regulator_set_voltage(msnr_dev->dvdd, 1200000, 1200000);
			ret = regulator_enable(msnr_dev->dvdd);
			if (ret < 0)
				goto dvdd_err;
		}
		/* dvdden-gpios */
		if (!IS_ERR_OR_NULL(gpio_dvdden))	// msnr_dev->dvdden
			gpiod_direction_output(gpio_dvdden, 1);	// msnr_dev->dvdden
		if (!IS_ERR_OR_NULL(msnr_dev->afvdd)) {
			regulator_set_voltage(msnr_dev->afvdd, 2800000, 2800000);
			ret = regulator_enable(msnr_dev->afvdd);
			if (ret < 0)
				goto af_err;
		}

		/* pwdn-gpios */
		if (!IS_ERR_OR_NULL(msnr_dev->pwdn))
			gpiod_set_value_cansleep(msnr_dev->pwdn, 1);

		/* rst-gpios */
		if (!IS_ERR_OR_NULL(msnr_dev->rst)) {
			gpiod_set_value_cansleep(msnr_dev->rst, 0);
			usleep_range(5 * 1000, 5 * 1000);
			gpiod_set_value_cansleep(msnr_dev->rst, 1);
			usleep_range(10 * 1000, 10 * 1000);
		}
		cam_dbg("sensor%d unreset", msnr_dev->id);
	} else {
		/* rst-gpios */
		if (!IS_ERR_OR_NULL(msnr_dev->rst))
			gpiod_set_value_cansleep(msnr_dev->rst, 0);

		/* pwdn-gpios */
		if (!IS_ERR_OR_NULL(msnr_dev->pwdn))
			gpiod_set_value_cansleep(msnr_dev->pwdn, 0);

		if (!IS_ERR_OR_NULL(msnr_dev->dvdd))
			regulator_disable(msnr_dev->dvdd);
		if (!IS_ERR_OR_NULL(msnr_dev->avdd))
			regulator_disable(msnr_dev->avdd);
		if (!IS_ERR_OR_NULL(msnr_dev->dovdd))
			regulator_disable(msnr_dev->dovdd);
		/* dvdden-gpios */
		if (!IS_ERR_OR_NULL(gpio_dvdden))	// msnr_dev->dvdden
			gpiod_direction_output(gpio_dvdden, 0);	//msnr_dev->dvdden
		if (!IS_ERR_OR_NULL(msnr_dev->afvdd))
			regulator_disable(msnr_dev->afvdd);

		clk_disable_unprepare(msnr_dev->mclk);
		cam_dbg("sensor%d reset", msnr_dev->id);
	}

	return ret;

af_err:
	if (msnr_dev->dvdd)
		regulator_disable(msnr_dev->dvdd);
dvdd_err:
	if (msnr_dev->dovdd)
		regulator_disable(msnr_dev->dovdd);
dovdd_err:
	if (msnr_dev->avdd)
		regulator_disable(msnr_dev->afvdd);
avdd_err:
	return ret;
}

static int camsnr_set_power_voltage(unsigned long arg,
				    struct cam_sensor_device *msnr_dev)
{
	uint32_t voltage = 0;
	cam_sensor_power_regulator_id regulator_id = 0;
	struct cam_sensor_power sensor_power;

	SENSOR_DRIVER_CHECK_POINTER(msnr_dev);

	if (copy_from_user((void *)&sensor_power, (void *)arg, sizeof(sensor_power))) {
		cam_err("Failed to copy args from user");
		return -EFAULT;
	}
	regulator_id = sensor_power.regulator_id;
	voltage = sensor_power.voltage;

	switch (regulator_id) {
	case SENSOR_REGULATOR_AFVDD:
		if (!IS_ERR_OR_NULL(msnr_dev->afvdd)) {
			regulator_set_voltage(msnr_dev->afvdd, voltage, voltage);
		} else {
			cam_err("afvdd is NULL!");
			return -EINVAL;
		}
		break;
	case SENSOR_REGULATOR_AVDD:
		if (!IS_ERR_OR_NULL(msnr_dev->avdd)) {
			regulator_set_voltage(msnr_dev->avdd, voltage, voltage);
		} else {
			cam_err("avdd is NULL!");
			return -EINVAL;
		}
		break;
	case SENSOR_REGULATOR_DOVDD:
		if (!IS_ERR_OR_NULL(msnr_dev->dovdd)) {
			regulator_set_voltage(msnr_dev->dovdd, voltage, voltage);
		} else {
			cam_err("dovdd is NULL!");
			return -EINVAL;
		}
		break;
	case SENSOR_REGULATOR_DVDD:
		if (!IS_ERR_OR_NULL(msnr_dev->dvdd)) {
			regulator_set_voltage(msnr_dev->dvdd, voltage, voltage);
		} else {
			cam_err("dvdd is NULL!");
			return -EINVAL;
		}
		break;
	default:
		cam_err("err regulator id");
		break;
	}

	return 0;
}

static int camsnr_set_power_on(unsigned long arg, struct cam_sensor_device *msnr_dev)
{
	int ret = 0;
	uint32_t on = 0;
	cam_sensor_power_regulator_id regulator_id = 0;
	struct cam_sensor_power sensor_power;

	SENSOR_DRIVER_CHECK_POINTER(msnr_dev);

	if (copy_from_user((void *)&sensor_power, (void *)arg, sizeof(sensor_power))) {
		cam_err("Failed to copy args from user");
		return -EFAULT;
	}
	regulator_id = sensor_power.regulator_id;
	on = sensor_power.on;

	if (on) {
		switch (regulator_id) {
		case SENSOR_REGULATOR_AFVDD:
			if (!IS_ERR_OR_NULL(msnr_dev->afvdd)) {
				ret = regulator_enable(msnr_dev->afvdd);
				if (ret < 0) {
					cam_err("enable afvdd failed");
					return ret;
				}
			} else {
				cam_err("afvdd is NULL!");
				return -EINVAL;
			}
			break;
		case SENSOR_REGULATOR_AVDD:
			if (!IS_ERR_OR_NULL(msnr_dev->avdd)) {
				ret = regulator_enable(msnr_dev->avdd);
				if (ret < 0) {
					cam_err("enable avdd failed");
					return ret;
				}
			} else {
				cam_err("avdd is NULL!");
				return -EINVAL;
			}
			break;
		case SENSOR_REGULATOR_DOVDD:
			if (!IS_ERR_OR_NULL(msnr_dev->dovdd)) {
				ret = regulator_enable(msnr_dev->dovdd);
				if (ret < 0) {
					cam_err("enable dovdd failed");
					return ret;
				}
			} else {
				cam_err("dovdd is NULL!");
				return -EINVAL;
			}
			break;
		case SENSOR_REGULATOR_DVDD:
			if (!IS_ERR_OR_NULL(msnr_dev->dvdd)) {
				ret = regulator_enable(msnr_dev->dvdd);
				if (ret < 0) {
					cam_err("enable dvdd failed");
					return ret;
				}
			} else {
				cam_err("dvdd is NULL!");
				return -EINVAL;
			}
			break;
		default:
			cam_err("err regulator id");
			break;
		}
	} else {
		switch (regulator_id) {
		case SENSOR_REGULATOR_AFVDD:
			if (!IS_ERR_OR_NULL(msnr_dev->afvdd)) {
				ret = regulator_disable(msnr_dev->afvdd);
			} else {
				cam_err("afvdd is NULL!");
				return -EINVAL;
			}
			break;
		case SENSOR_REGULATOR_AVDD:
			if (!IS_ERR_OR_NULL(msnr_dev->avdd)) {
				ret = regulator_disable(msnr_dev->avdd);
			} else {
				cam_err("avdd is NULL!");
				return -EINVAL;
			}
			break;
		case SENSOR_REGULATOR_DOVDD:
			if (!IS_ERR_OR_NULL(msnr_dev->dovdd)) {
				ret = regulator_disable(msnr_dev->dovdd);
			} else {
				cam_err("dovdd is NULL!");
				return -EINVAL;
			}
			break;
		case SENSOR_REGULATOR_DVDD:
			if (!IS_ERR_OR_NULL(msnr_dev->dvdd)) {
				ret = regulator_disable(msnr_dev->dvdd);
			} else {
				cam_err("dvdd is NULL!");
				return -EINVAL;
			}
			break;
		default:
			cam_err("err regulator id");
			break;
		}
	}
	return 0;
}

static int camsnr_set_gpio_enable(unsigned long arg, struct cam_sensor_device *msnr_dev)
{
	uint8_t enable = 0;
	cam_sensor_gpio_id gpio_id = 0;
	struct cam_sensor_gpio sensor_gpio;

	SENSOR_DRIVER_CHECK_POINTER(msnr_dev);

	if (copy_from_user((void *)&sensor_gpio, (void *)arg, sizeof(sensor_gpio))) {
		cam_err("Failed to copy args from user");
		return -EFAULT;
	}
	gpio_id = sensor_gpio.gpio_id;
	enable = sensor_gpio.enable;

	switch (gpio_id) {
	case SENSOR_GPIO_PWDN:
		if (!IS_ERR_OR_NULL(msnr_dev->pwdn))
			gpiod_direction_output(msnr_dev->pwdn, enable);
		break;
	case SENSOR_GPIO_RST:
		if (!IS_ERR_OR_NULL(msnr_dev->rst))
			gpiod_direction_output(msnr_dev->rst, enable);
		break;
	case SENSOR_GPIO_DVDDEN:
		if (!IS_ERR_OR_NULL(gpio_dvdden))
			gpiod_direction_output(gpio_dvdden, enable);
		break;
	case SENSOR_GPIO_DCDCEN:
		if (!IS_ERR_OR_NULL(msnr_dev->dcdcen))
			gpiod_direction_output(msnr_dev->dcdcen, enable);
		break;
	default:
		cam_err("wrong gpio_id %d", gpio_id);
		return -EINVAL;
		break;
	}
	return 0;
}

static int camsnr_set_mclk_rate(unsigned long arg, struct cam_sensor_device *msnr_dev)
{
	int ret = 0;
	uint32_t clk_rate = 0;

	SENSOR_DRIVER_CHECK_POINTER(msnr_dev);

	if (copy_from_user((void *)&clk_rate, (void *)arg, sizeof(clk_rate))) {
		cam_err("Failed to copy args from user");
		return -EFAULT;
	}

	clk_rate = (clk_rate == 0) ? SENSOR_MCLK_CLK_RATE : clk_rate;
	if (msnr_dev->mclk) {
		ret = clk_set_rate(msnr_dev->mclk, clk_rate);
		if (ret)
			return ret;
	} else {
		cam_err("%s: mclk is NULL", __func__);
		return -EINVAL;
	}
	return 0;
}

static int camsnr_set_mclk_enable(unsigned long arg, struct cam_sensor_device *msnr_dev)
{
	int ret = 0;
	uint32_t clk_enable = 0;

	SENSOR_DRIVER_CHECK_POINTER(msnr_dev);

	if (copy_from_user((void *)&clk_enable, (void *)arg,
			   sizeof(clk_enable))) {
		cam_err("Failed to copy args from user");
		return -EFAULT;
	}

	if (msnr_dev->mclk) {
		if (clk_enable && !__clk_is_enabled(msnr_dev->mclk)) {
			ret = clk_prepare_enable(msnr_dev->mclk);
			if (ret < 0)
				return ret;
		} else if (!clk_enable && __clk_is_enabled(msnr_dev->mclk)) {
			clk_disable_unprepare(msnr_dev->mclk);
		} else {
			cam_warn("%s: mclk%d is already %s", __func__,
				 msnr_dev->id,
				 clk_enable ? "enabled" : "disabled");
		}
	} else {
		cam_err("%s: mclk is NULL", __func__);
		return -EINVAL;
	}
	return 0;
}

static int camsnr_reset_sensor(unsigned long arg)
{
	int ret = 0;
	sns_rst_source_t sns_reset_source;
	struct cam_sensor_device *msnr_dev;

	if (copy_from_user((void *)&sns_reset_source, (void *)arg,
			   sizeof(sns_reset_source))) {
		cam_err("%s: Failed to copy args from user", __func__);
		return -EFAULT;
	}

	if (sns_reset_source >= CAM_SNS_MAX_DEV_NUM) {
		cam_err("%s: Invalid snr reset source %d", __func__, sns_reset_source);
		return -EINVAL;
	}

	msnr_dev = g_sdev[sns_reset_source];
	if (IS_ERR_OR_NULL(msnr_dev)) {
		cam_err("%s: Invalid cam_sensor_device", __func__);
		return -ENODEV;
	}

	ret = cam_sensor_power_set(msnr_dev, 0);
	return ret;
}

static int camsnr_unreset_sensor(unsigned long arg)
{
	int ret = 0;
	sns_rst_source_t sns_reset_source;
	struct cam_sensor_device *msnr_dev;

	if (copy_from_user((void *)&sns_reset_source, (void *)arg,
			   sizeof(sns_reset_source))) {
		cam_err("Failed to copy args from user");
		return -EFAULT;
	}

	if (sns_reset_source >= CAM_SNS_MAX_DEV_NUM) {
		cam_err("Invalid snr reset source %d", sns_reset_source);
		return -EINVAL;
	}

	msnr_dev = g_sdev[sns_reset_source];
	if (IS_ERR_OR_NULL(msnr_dev)) {
		cam_err("%s: Invalid cam_sensor_device", __func__);
		return -ENODEV;
	}

	ret = cam_sensor_power_set(msnr_dev, 1);

	return ret;
}

static void cam_sensor_i2c_dumpinfo(struct i2c_msg *msg_array, int num)
{
	int i;

	cam_info("%s: dump i2c transfer data info, msg number is %d", __func__, num);
	for(i = 0; i < num; i ++)
		cam_info("%s, i2c transfer msg_array[%d], addr 0x%x, flags %d, len %d, val [0x%x, 0x%x, 0x%x, 0x%x]",
			 __func__, i, msg_array[i].addr, msg_array[i].flags, msg_array[i].len,
			 msg_array[i].buf[0], msg_array[i].buf[1], msg_array[i].buf[2], msg_array[i].buf[3]);
}

static int cam_sensor_write(struct cam_i2c_data *data,
			    struct cam_sensor_device *sensor_dev)
{
	struct i2c_adapter *adapter;
	struct i2c_msg msg;
	u8 val[4];
	int ret = 0;
	struct mutex *pcmd_mutex = NULL;
	u8 twsi_no, addr;
	u16 reg_len, val_len, reg, reg_val;

	SENSOR_DRIVER_CHECK_POINTER(data);
	SENSOR_DRIVER_CHECK_POINTER(sensor_dev);

	if (!data->addr || !data->reg_len || !data->val_len) {
		cam_err("%s: invalid addr 0x%x or reg_len %d or val_len %d",
			__func__, data->addr, data->reg_len, data->val_len);
		return -EINVAL;
	}

	twsi_no = sensor_dev->twsi_no;
	addr = data->addr;
	reg_len = data->reg_len;
	val_len = data->val_len;
	reg = data->tab.reg;
	reg_val = data->tab.val;

	adapter = i2c_get_adapter(twsi_no);
	if (!adapter) {
		cam_err("%s: i2c get adapter fail", __func__);
		return -ENODEV;
	}

	msg.addr = addr;
	msg.flags = 0;
	msg.buf = val;
	msg.len = reg_len + val_len;

	pcmd_mutex = &sensor_dev->lock;
	mutex_lock(pcmd_mutex);
	if (msg.len == 2) {
		/* reg:8bit; val:8bit */
		val[0] = reg & 0xff;
		val[1] = reg_val & 0xff;
	} else if (msg.len == 3) {
		/* reg:16bit; val:8bit */
		val[0] = (reg >> 8) & 0xff;
		val[1] = reg & 0xff;
		val[2] = reg_val & 0xff;
	} else if (msg.len == 4) {
		/* reg:16bit; val:16bit */
		val[0] = (reg >> 8) & 0xff;
		val[1] = reg & 0xff;
		val[2] = (reg_val >> 8) & 0xff;
		val[3] = reg_val & 0xff;
	}
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		cam_err("%s: i2c transfer data fail", __func__);
		cam_sensor_i2c_dumpinfo(&msg, 1);
		mutex_unlock(pcmd_mutex);
		return ret;
	}
	mutex_unlock(pcmd_mutex);

	return 0;
}

static int cam_sensor_read(struct cam_i2c_data *data,
			   struct cam_sensor_device *sensor_dev)
{
	int ret;
	u8 val[2];
	struct i2c_adapter *adapter;
	struct i2c_msg msg;
	struct mutex *pcmd_mutex = NULL;
	u8 twsi_no, addr;
	u16 reg_len, val_len, reg, reg_val = 0;

	SENSOR_DRIVER_CHECK_POINTER(data);
	SENSOR_DRIVER_CHECK_POINTER(sensor_dev);

	if (!data->addr || !data->reg_len || !data->val_len) {
		cam_err("%s: invalid addr 0x%x or reg_len %d or val_len %d",
			__func__, data->addr, data->reg_len, data->val_len);
		return -EINVAL;
	}

	twsi_no = sensor_dev->twsi_no;
	addr = data->addr;
	reg_len = data->reg_len;
	val_len = data->val_len;
	reg = data->tab.reg;

	adapter = i2c_get_adapter(twsi_no);
	if (!adapter) {
		cam_err("%s: i2c get adapter fail", __func__);
		return -ENODEV;
	}

	msg.addr = addr;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = val;

	pcmd_mutex = &sensor_dev->lock;
	mutex_lock(pcmd_mutex);
	if (reg_len == I2C_8BIT) {
		msg.len = 1;
		val[0] = reg & 0xff;
	} else if (reg_len == I2C_16BIT) {
		msg.len = 2;
		val[0] = (reg >> 8) & 0xff;
		val[1] = reg & 0xff;
	}
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		mutex_unlock(pcmd_mutex);
		goto err;
	}

	if (val_len == I2C_8BIT)
		msg.len = 1;
	else if (val_len == I2C_16BIT)
		msg.len = 2;
	msg.flags = I2C_M_RD;
	ret = i2c_transfer(adapter, &msg, 1);
	if (ret < 0) {
		mutex_unlock(pcmd_mutex);
		goto err;
	}
	if (val_len == I2C_8BIT)
		reg_val = val[0];
	else if (val_len == I2C_16BIT)
		reg_val = (val[0] << 8) + val[1];
	mutex_unlock(pcmd_mutex);

	data->tab.val = reg_val;

	return 0;

err:
	cam_err("%s: Failed reading register 0x%02x!", __func__, reg);
	return ret;
}

static int cam_sensor_burst_write(struct cam_burst_i2c_data *data,
				  struct cam_sensor_device *sensor_dev)
{
	struct i2c_adapter *adapter;
	struct regval_tab *tab;
	struct i2c_msg msg_array[BURST_I2C_REG_SIZE];
	u8 val[BURST_I2C_REG_SIZE][4];
	struct regval_tab buf[BURST_I2C_REG_SIZE];
	int ret = 0;
	struct mutex *pcmd_mutex = NULL;
	u32 num, i;
	u8 twsi_no;

	SENSOR_DRIVER_CHECK_POINTER(data);
	SENSOR_DRIVER_CHECK_POINTER(sensor_dev);

	if (!data->addr || !data->reg_len || !data->val_len) {
		cam_err("%s: invalid addr 0x%x or reg_len %d or val_len %d",
			__func__, data->addr, data->reg_len, data->val_len);
		return -EINVAL;
	}

	twsi_no = sensor_dev->twsi_no;
	adapter = i2c_get_adapter(twsi_no);
	if (!adapter) {
		cam_err("%s: i2c get adapter fail, twsi_no %d", __func__, twsi_no);
		return -ENODEV;
	}

	pcmd_mutex = &sensor_dev->lock;

	do {
		num = (data->num > BURST_I2C_REG_SIZE) ? BURST_I2C_REG_SIZE : data->num;
		if (copy_from_user(buf, data->tab, sizeof(struct regval_tab) * num)) {
			cam_err("%s: copy_from_user", __func__);
			return -EFAULT;
		}

		memset(msg_array, 0, BURST_I2C_REG_SIZE * sizeof(struct i2c_msg));
		mutex_lock(pcmd_mutex);
		for (i = 0; i < num; i++) {
			msg_array[i].addr = data->addr;
			msg_array[i].flags = 0;
			msg_array[i].len = 1;
			msg_array[i].buf = val[i];

			if (data->reg_len == I2C_8BIT)
				msg_array[i].len = 1;
			else if (data->reg_len == I2C_16BIT)
				msg_array[i].len = 2;

			if (data->val_len == I2C_8BIT)
				msg_array[i].len += 1;
			else if (data->val_len == I2C_16BIT)
				msg_array[i].len += 2;

			tab = &buf[i];
			if (msg_array[i].len == 2) {
				/* reg:8bit; val:8bit */
				val[i][0] = tab->reg & 0xff;
				val[i][1] = tab->val & 0xff;
			} else if (msg_array[i].len == 3) {
				/* reg:16bit; val:8bit */
				val[i][0] = (tab->reg >> 8) & 0xff;
				val[i][1] = tab->reg & 0xff;
				val[i][2] = tab->val & 0xff;
			} else if (msg_array[i].len == 4) {
				/* reg:16bit; val:16bit */
				val[i][0] = (tab->reg >> 8) & 0xff;
				val[i][1] = tab->reg & 0xff;
				val[i][2] = (tab->val >> 8) & 0xff;
				val[i][3] = tab->val & 0xff;
			}
		}

		ret = i2c_transfer(adapter, msg_array, i);
		if (ret != i) {
			cam_err("%s, i2c transfer fail, ret %d, i %d", __func__, ret, i);
			cam_sensor_i2c_dumpinfo(msg_array, i);
			ret = -EIO;
			mutex_unlock(pcmd_mutex);
			goto out;
		} else
			ret = 0;

		mutex_unlock(pcmd_mutex);
		data->num -= num;
		data->tab += num;
	} while (data->num > 0);

out:
	return ret;
}

static int cam_sensor_burst_read(struct cam_burst_i2c_data *data,
				 struct cam_sensor_device *sensor_dev)
{
	int ret = 0;
	struct regval_tab buf[BURST_I2C_REG_SIZE];
	u32 i, num;
	struct cam_i2c_data i2c_data;

	SENSOR_DRIVER_CHECK_POINTER(data);
	SENSOR_DRIVER_CHECK_POINTER(sensor_dev);

	if (!data->addr || !data->reg_len || !data->val_len) {
		cam_err("%s: invalid addr 0x%x or reg_len %d or val_len %d",
			__func__, data->addr, data->reg_len, data->val_len);
		return -EINVAL;
	}

	i2c_data.reg_len = data->reg_len;
	i2c_data.val_len = data->val_len;
	i2c_data.addr = data->addr;

	do {
		num = (data->num > BURST_I2C_REG_SIZE) ? BURST_I2C_REG_SIZE : data->num;
		if (copy_from_user(buf, data->tab, sizeof(struct regval_tab) * num)) {
			cam_err("%s: copy_from_user", __func__);
			return -EFAULT;
		}
		for (i = 0; i < num; i++) {
			i2c_data.tab.reg = buf[i].reg;
			ret = cam_sensor_read(&i2c_data, sensor_dev);
			if (ret < 0)
				return ret;
			buf[i].val = i2c_data.tab.val;
		}
		if (copy_to_user(data->tab, buf, sizeof(struct regval_tab) * num)) {
			cam_err("%s: copy read value to user failed!", __func__);
			ret = -EPERM;
		}
		data->num -= num;
		data->tab += num;
	} while (data->num > 0);

	return ret;
}

static int cam_sensor_get_info(struct cam_sensor_info *sensor_info,
			       struct cam_sensor_device *sensor_dev)
{
	SENSOR_DRIVER_CHECK_POINTER(sensor_info);
	SENSOR_DRIVER_CHECK_POINTER(sensor_dev);

	sensor_info->twsi_no = sensor_dev->twsi_no;

	return 0;
}

static int camsnr_mipi_clock_set(unsigned long arg, unsigned int dphy_no)
{
	int ret = 0;
	sns_mipi_clock_t sns_mipi_clock;
	if (copy_from_user((void *)&sns_mipi_clock, (void *)arg, sizeof(sns_mipi_clock))) {
		cam_err("Failed to copy args from user");
		return -EFAULT;
	}

	ret = ccic_dphy_hssettle_set(dphy_no, sns_mipi_clock);
	if (!ret)
		cam_dbg("mipi%d: set mipi clock\n", dphy_no);

	return ret;
}

static long camsnr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	struct cam_sensor_device *msnr_dev = NULL;

	SENSOR_DRIVER_CHECK_POINTER(file);
	msnr_dev = (struct cam_sensor_device *)file->private_data;

	if (_IOC_TYPE(cmd) != CAM_SENSOR_IOC_MAGIC) {
		cam_err("%s: invalid cmd %d", __func__, cmd);
		return -ENOTTY;
	}

	switch (cmd) {
	case CAM_SENSOR_RESET:
		ret = camsnr_reset_sensor(arg);
		break;
	case CAM_SENSOR_UNRESET:
		ret = camsnr_unreset_sensor(arg);
		break;
	case CAM_SENSOR_I2C_WRITE:{
		struct cam_i2c_data data;
		if (copy_from_user((void *)&data, (void *)arg, sizeof(data))) {
			cam_err("%s: Line %d: Failed to copy args from user",
				__func__, __LINE__);
			return -EFAULT;
		}

		ret = cam_sensor_write(&data, msnr_dev);
		break;
	}
	case CAM_SENSOR_I2C_READ:{
		struct cam_i2c_data data;
		if (copy_from_user((void *)&data, (void *)arg, sizeof(data))) {
			cam_err("%s: Line %d: Failed to copy args from user",
				__func__, __LINE__);
			return -EFAULT;
		}

		ret = cam_sensor_read(&data, msnr_dev);
		if (copy_to_user((void *)arg, (void *)&data, sizeof(data))) {
			cam_err("%s: Line %d: Failed to copy args to user",
				__func__, __LINE__);
			return -EFAULT;
		}
		break;
	}
	case CAM_SENSOR_I2C_BURST_WRITE:{
		struct cam_burst_i2c_data data;
		if (copy_from_user((void *)&data, (void *)arg, sizeof(data))) {
			cam_err("%s: Line %d: Failed to copy args from user",
				__func__, __LINE__);
			return -EFAULT;
		}

		ret = cam_sensor_burst_write(&data, msnr_dev);
		break;
	}
	case CAM_SENSOR_I2C_BURST_READ:{
		struct cam_burst_i2c_data data;
		if (copy_from_user((void *)&data, (void *)arg, sizeof(data))) {
			cam_err("%s: Line %d: Failed to copy args from user",
				__func__, __LINE__);
			return -EFAULT;
		}

		ret = cam_sensor_burst_read(&data, msnr_dev);
		break;
	}
	case CAM_SENSOR_GET_INFO:{
		struct cam_sensor_info data;

		ret = cam_sensor_get_info(&data, msnr_dev);
		if (copy_to_user((void *)arg, (void *)&data, sizeof(data))) {
			cam_err("%s: Line %d: Failed to copy args to user",
				__func__, __LINE__);
			return -EFAULT;
		}
		break;
	}
	case CAM_SENSOR_SET_MIPI_CLOCK:
		ret = camsnr_mipi_clock_set(arg, msnr_dev->dphy_no);
		break;
	case CAM_SENSOR_SET_POWER_VOLTAGE:
		ret = camsnr_set_power_voltage(arg, msnr_dev);
		break;
	case CAM_SENSOR_SET_POWER_ON:
		ret = camsnr_set_power_on(arg, msnr_dev);
		break;
	case CAM_SENSOR_SET_GPIO_ENABLE:
		ret = camsnr_set_gpio_enable(arg, msnr_dev);
		break;
	case CAM_SENSOR_SET_MCLK_RATE:
		ret = camsnr_set_mclk_rate(arg, msnr_dev);
		break;
	case CAM_SENSOR_SET_MCLK_ENABLE:
		ret = camsnr_set_mclk_enable(arg, msnr_dev);
		break;
	default:
		cam_err("unknown IOCTL code 0x%x", cmd);
		ret = -ENOTTY;
	}

	cam_dbg("%s IN, cmd %x", __func__, cmd);

	return ret;
}

//fixme: add compat in the future
#if 0
//#ifdef CONFIG_COMPAT

#define assign_in_user(to, from)					\
({									\
	typeof(*from) __assign_tmp;					\
									\
	get_user(__assign_tmp, from) || put_user(__assign_tmp, to);	\
})

struct cam_burst_i2c_data32 {
	enum sensor_i2c_len reg_len;
	enum sensor_i2c_len val_len;
	uint8_t addr;
	uint32_t tab;
	uint32_t num;
};

static int alloc_userspace(unsigned int size, u32 aux_space, void __user **new_p64)
{
	*new_p64 = compat_alloc_user_space(size + aux_space);
	if (!*new_p64)
		return -ENOMEM;
	if (clear_user(*new_p64, size))
		return -EFAULT;
	return 0;
}

#define CAM_SENSOR_I2C_BURST_WRITE32 _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_I2C_BURST_WRITE, struct cam_burst_i2c_data32)
#define CAM_SENSOR_I2C_BURST_READ32 _IOW(CAM_SENSOR_IOC_MAGIC, SENSOR_IOC_I2C_BURST_READ, struct cam_burst_i2c_data32)

static int get_burst_i2c_data(struct cam_burst_i2c_data __user *p64, struct cam_burst_i2c_data32 __user *p32)
{
	compat_caddr_t p = 0;
	struct regval_tab __user *tab32 = NULL;

	if (!access_ok(p32, sizeof(*p32))
	    || assign_in_user(&p64->reg_len, &p32->reg_len)
	    || assign_in_user(&p64->val_len, &p32->val_len)
	    || assign_in_user(&p64->addr, &p32->addr)
	    || assign_in_user(&p64->num, &p32->num)) {
		cam_err("%s assign in user failed", __func__);
		return -EFAULT;
	}
	if (get_user(p, &p32->tab)) {
		cam_err("%s get tab failed", __func__);
		return -EFAULT;
	}
	tab32 = compat_ptr(p);
	if (put_user(tab32, &p64->tab)) {
		cam_err("%s tab put user failed", __func__);
		return -EFAULT;
	}
	return 0;
}

static int put_burst_i2c_data(struct cam_burst_i2c_data __user *p64,
			      struct cam_burst_i2c_data32 __user *p32)
{
	if (!access_ok(p32, sizeof(*p32))
	    || assign_in_user(&p32->reg_len, &p64->reg_len)
	    || assign_in_user(&p32->val_len, &p64->val_len)
	    || assign_in_user(&p32->addr, &p64->addr)
	    || assign_in_user(&p32->num, &p64->num)) {
		cam_err("%s assign in user failed", __func__);
		return -EFAULT;
	}
	return 0;
}

static long camsnr_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *p32 = compat_ptr(arg);
	void __user *new_p64 = NULL;
	//void __user *aux_buf;
	//u32 aux_space;
	long err = 0;
	unsigned int ncmd = 0;
	//size_t size32 = 0;
	size_t size64 = 0;

	//size32 = _IOC_SIZE(cmd);
	//switch (_IOC_NR(cmd)) {
	//case _IOC_NR(CAM_SENSOR_RESET):
	//      size64 = sizeof(sns_rst_source_t);
	//      break;
	//case _IOC_NR(CAM_SENSOR_UNRESET):
	//      size64 = sizeof(sns_rst_source_t);
	//      break;
	//case _IOC_NR(CAM_SENSOR_I2C_WRITE):
	//      size64 = sizeof(struct cam_i2c_data);
	//      break;
	//case _IOC_NR(CAM_SENSOR_I2C_READ):
	//      size64 = sizeof(struct cam_i2c_data);
	//      break;
	//case _IOC_NR(CAM_SENSOR_I2C_BURST_WRITE):
	//      size64 = sizeof(struct cam_burst_i2c_data);
	//      break;
	//case _IOC_NR(CAM_SENSOR_I2C_BURST_READ):
	//      size64 = sizeof(struct cam_burst_i2c_data);
	//      break;
	//case _IOC_NR(CAM_SENSOR_GET_INFO):
	//      size64 = sizeof(struct cam_sensor_info);
	//      break;
	//case _IOC_NR(CAM_SENSOR_SET_MIPI_CLOCK):
	//      size64 = sizeof(sns_mipi_clock_t);
	//      break;
	//}
	//cam_dbg("%s:cmd_nr=%d size32=%u size64=%u", __func__, _IOC_NR(cmd), size32, size64);

	switch (cmd) {
	case CAM_SENSOR_I2C_BURST_WRITE32:
	case CAM_SENSOR_I2C_BURST_READ32:
		size64 = sizeof(struct cam_burst_i2c_data);
		if (cmd == CAM_SENSOR_I2C_BURST_WRITE32) {
			ncmd = CAM_SENSOR_I2C_BURST_WRITE;
		} else {
			ncmd = CAM_SENSOR_I2C_BURST_READ;
		}
		err = alloc_userspace(size64, 0, &new_p64);
		if (err) {
			cam_err("%s alloc userspace failed err=%l cmd=%d ioc_size=%u",
				__func__, err, _IOC_NR(cmd), size64);
			return err;
		}
		err = (long)get_burst_i2c_data(new_p64, p32);
		if (err) {
			return err;
		}
		break;
	default:
		size64 = _IOC_SIZE(cmd);
		ncmd = cmd;
		if (size64 > 0) {
			err = alloc_userspace(size64, 0, &new_p64);
			if (err) {
				cam_err
				    ("%s alloc userspace failed err=%l cmd=%d ioc_size=%u",
				     __func__, err, _IOC_NR(cmd), size64);
				return err;
			}
			err = copy_in_user(new_p64, p32, size64);
			if (err) {
				cam_err
				    ("%s copy in user 1 failed err=%l cmd=%d ioc_size=%u",
				     __func__, err, _IOC_NR(cmd), size64);
				return err;
			}
		}
		break;
	}

	err = camsnr_ioctl(file, ncmd, (unsigned long)new_p64);
	if (err == 0) {
		switch (cmd) {
		case CAM_SENSOR_I2C_BURST_WRITE32:
		case CAM_SENSOR_I2C_BURST_READ32:
			err = (long)put_burst_i2c_data(new_p64, p32);
			if (err) {
				cam_err("%s put_burst_i2c_data failed err=%l", __func__,
					err);
				return err;
			}
			break;
		default:
			err = copy_in_user(p32, new_p64, size64);
			if (err) {
				cam_err
				    ("%s copy in user 2 failed err=%l cmd=%d ioc_size=%u",
				     __func__, err, _IOC_NR(cmd), size64);
				return err;
			}
			break;
		}
	}
	return 0;
}
#endif
static bool use_pinmulti = false;
static int request_res_in_pinmulti_mode(struct cam_sensor_device *msnr_dev)
{
	struct device *dev = NULL;
	int ret = 0;
	u32 cell_id;

	dev = &msnr_dev->pdev->dev;
	cell_id = msnr_dev->id;

	// msnr_dev->pinctrl = devm_pinctrl_get (dev);
	msnr_dev->pinctrl = pinctrl_get_select (dev, "mclk_multi");
	if (IS_ERR(msnr_dev->pinctrl)) {
		cam_err("unable to get sensor%d mclk pinctrl\n", cell_id);
		return PTR_ERR(msnr_dev->pinctrl);
	}

	msnr_dev->pinctrl_state = pinctrl_lookup_state(msnr_dev->pinctrl, "mclk_multi");
	if (IS_ERR(msnr_dev->pinctrl_state)) {
		cam_err("unable to lookup sensor%d mclk pinctrl state\n", cell_id);
		pinctrl_put(msnr_dev->pinctrl);
		return PTR_ERR(msnr_dev->pinctrl_state);
	}

	pinctrl_select_state(msnr_dev->pinctrl, msnr_dev->pinctrl_state);

	/* mclks */
	msnr_dev->mclk = clk_get(dev, msnr_dev->mclk_name);
	if (IS_ERR(msnr_dev->mclk)) {
		cam_err("unable to get cam_mclk%d\n", cell_id);
		return PTR_ERR(msnr_dev->mclk);
	}

	/* pwdn-gpios */
	msnr_dev->pwdn = gpiod_get(dev, "pwdn", GPIOD_OUT_HIGH);
	if (IS_ERR(msnr_dev->pwdn)) {
		cam_info("%s: unable to parse sensor%d pwdn gpio", __func__, cell_id);
		return PTR_ERR(msnr_dev->pwdn);
	} else {
		ret = gpiod_direction_output(msnr_dev->pwdn, 0);
		if (ret < 0) {
			cam_err("%s: Failed to init sensor%d pwdn gpio", __func__, cell_id);
			return ret;
		}
	}

	/* rst-gpios */
	msnr_dev->rst = gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(msnr_dev->rst)) {
		cam_info("%s: unable to parse sensor%d reset gpio", __func__, cell_id);
		return PTR_ERR(msnr_dev->rst);
	} else {
		ret = gpiod_direction_output(msnr_dev->rst, 0);
		if (ret < 0) {
			cam_err("%s: Failed to init sensor%d reset gpio", __func__, cell_id);
			return ret;
		}
	}
	msnr_dev->req_pinmulti = true;
	use_pinmulti = true;

	return ret;
}
static int release_res_in_pinmulti_mode(struct cam_sensor_device *msnr_dev)
{
	if (msnr_dev->pinctrl)
		pinctrl_put(msnr_dev->pinctrl);
	msnr_dev->pinctrl = NULL;

	/* mclks */
	if (msnr_dev->mclk)
		clk_put(msnr_dev->mclk);
	msnr_dev->mclk = NULL;

	/* pwdn-gpios */
	if (msnr_dev->pwdn)
		gpiod_put(msnr_dev->pwdn);
	msnr_dev->pwdn = NULL;

	/* rst-gpios */
	if (msnr_dev->rst)
		gpiod_put(msnr_dev->rst);
	msnr_dev->rst = NULL;
	msnr_dev->req_pinmulti = false;
	use_pinmulti = false;

	return 0;
}
static int camsnr_open(struct inode *inode, struct file *file)
{
	struct cam_sensor_device *msnr_dev =
	    container_of(inode->i_cdev, struct cam_sensor_device, cdev);
	int ret = 0;

	file->private_data = msnr_dev;

	//for muse pi
	if (msnr_dev->is_pinmulti && msnr_dev->req_pinmulti == false && use_pinmulti == false)
		ret = request_res_in_pinmulti_mode(msnr_dev);
	else if (msnr_dev->is_pinmulti && msnr_dev->req_pinmulti == false && use_pinmulti == true)
		cam_warn("previous cam_sensor must be closed before open %s%d in pinmulti mode!", DRIVER_NAME, msnr_dev->id);

	cam_dbg("%s open %s%d, twsi_no %d, is_pinmulti %d, req_pinmulti %d, use_pinmulti %d\n", __func__, DRIVER_NAME, msnr_dev->id,
		msnr_dev->twsi_no, msnr_dev->is_pinmulti, msnr_dev->req_pinmulti, use_pinmulti);

	return ret;
}

static int camsnr_release(struct inode *inode, struct file *file)
{
	struct cam_sensor_device *msnr_dev =
	    container_of(inode->i_cdev, struct cam_sensor_device, cdev);

	//ctrl + c exit still use clk and gpio, close them
	if (msnr_dev->mclk && __clk_is_enabled(msnr_dev->mclk)) {
		cam_info("disable mclk and gpio low");
		/* rst-gpios */
		if (!IS_ERR_OR_NULL(msnr_dev->rst))
			gpiod_set_value_cansleep(msnr_dev->rst, 0);

		/* pwdn-gpios */
		if (!IS_ERR_OR_NULL(msnr_dev->pwdn))
			gpiod_set_value_cansleep(msnr_dev->pwdn, 0);

		clk_disable_unprepare(msnr_dev->mclk);
	}

	if (msnr_dev->is_pinmulti && msnr_dev->req_pinmulti == true && use_pinmulti == true)
		release_res_in_pinmulti_mode(msnr_dev);

	cam_dbg("%s close %s%d, twsi_no %d, is_pinmulti %d, req_pinmulti %d, use_pinmulti %d\n", __func__, DRIVER_NAME, msnr_dev->id,
		msnr_dev->twsi_no, msnr_dev->is_pinmulti, msnr_dev->req_pinmulti, use_pinmulti);

	return 0;
}

static const struct file_operations camsnr_fops = {
	.owner = THIS_MODULE,
	.open = camsnr_open,
	.release = camsnr_release,
	.unlocked_ioctl = camsnr_ioctl,
#ifdef CONFIG_COMPAT
	//fixme: add compat in the future
	//.compat_ioctl = camsnr_compat_ioctl,
#endif
};

static void cam_snr_drv_deinit(void)
{
	dev_t dev_id = MKDEV(camsnr_major, 0);

	unregister_chrdev_region(dev_id, CAM_SNS_MAX_DEV_NUM);
	if (camsnr_class)
		class_destroy(camsnr_class);
}

static int cam_snr_drv_init(void)
{
	int ret = 0;
	dev_t dev_id;

	ret = alloc_chrdev_region(&dev_id, 0, CAM_SNS_MAX_DEV_NUM, DRIVER_NAME);
	if (ret) {
		cam_err("%s: can't get major number", __func__);
		goto out;
	}

	camsnr_major = MAJOR(dev_id);

	camsnr_class = class_create(DRIVER_NAME);
	if (IS_ERR(camsnr_class)) {
		cam_err("%s: camsnr_class is error", __func__);
		ret = PTR_ERR(camsnr_class);
		goto error_cdev;
	}

out:
	return ret;

error_cdev:
	unregister_chrdev_region(dev_id, CAM_SNS_MAX_DEV_NUM);
	return ret;
}

static int cam_snr_dev_destroy(struct cdev *cdev, int index)
{
	SENSOR_DRIVER_CHECK_POINTER(cdev);
	device_destroy(camsnr_class, MKDEV(camsnr_major, index));
	cdev_del(cdev);

	return 0;
}

static int cam_snr_dev_create(struct cdev *cdev, int index)
{
	int ret = 0;

	SENSOR_DRIVER_CHECK_POINTER(cdev);

	cdev_init(cdev, &camsnr_fops);
	ret = cdev_add(cdev, MKDEV(camsnr_major, index), 1);
	if (ret < 0) {
		cam_err("add device %d cdev fail", index);
		goto out;
	}

	/* create device node */
	device_create(camsnr_class, NULL, MKDEV(camsnr_major, index), NULL, "%s%d",
		      DRIVER_NAME, index);

out:
	return ret;
}

static int camsnr_of_parse(struct cam_sensor_device *sensor)
{
	struct device *dev = NULL;
	struct device_node *of_node = NULL;
	u32 cell_id, twsi_no, dphy_no;
	int ret;
	//const char *mclk_name;

	SENSOR_DRIVER_CHECK_POINTER(sensor);
	dev = &sensor->pdev->dev;
	SENSOR_DRIVER_CHECK_POINTER(dev);
	of_node = dev->of_node;

	/* cell-index */
	ret = of_property_read_u32(of_node, "cell-index", &cell_id);
	if (ret < 0) {
		cam_err("%s: cell-index read failed", __func__);
		return ret;
	}
	if (cell_id >= CAM_SNS_MAX_DEV_NUM) {
		cam_err("%s: invaid cell-index %d", __func__, cell_id);
		return -EINVAL;
	}
	sensor->id = cell_id;
	if (g_sdev[cell_id]) {
		cam_err("%s: cell-index %d already exists", __func__, cell_id);
		return -EINVAL;
	}
	cam_snr_dev_create(&sensor->cdev, sensor->id);

	/*twsi_index */
	ret = of_property_read_u32(of_node, "twsi-index", &twsi_no);
	if (ret < 0) {
		cam_err("%s: twsi-index read failed", __func__);
		return ret;
	}
	sensor->twsi_no = (u8) twsi_no;

	/*dphy_index */
	ret = of_property_read_u32(of_node, "dphy-index", &dphy_no);
	if (ret < 0) {
		cam_err("%s: twsi-index read failed", __func__);
		return ret;
	}
	sensor->dphy_no = (u8) dphy_no;

	sensor->afvdd = devm_regulator_get(dev, "af_2v8");
	if (IS_ERR(sensor->afvdd)) {
		dev_warn(dev, "Failed to get regulator af_2v8\n");
		sensor->afvdd = NULL;
	}

	sensor->avdd = devm_regulator_get(dev, "avdd_2v8");
	if (IS_ERR(sensor->avdd)) {
		dev_warn(dev, "Failed to get regulator avdd_2v8\n");
		sensor->avdd = NULL;
	}

	sensor->dovdd = devm_regulator_get(dev, "dovdd_1v8");
	if (IS_ERR(sensor->dovdd)) {
		dev_warn(dev, "Failed to get regulator dovdd_1v8\n");
		sensor->dovdd = NULL;
	}

	sensor->dvdd = devm_regulator_get(dev, "dvdd_1v2");
	if (IS_ERR(sensor->dvdd)) {
		dev_warn(dev, "Failed to get regulator dvdd_1v2\n");
		sensor->dvdd = NULL;
	}

	ret = of_property_read_string(of_node, "clock-names", &sensor->mclk_name);
	if (!ret) {
		if (strcmp(sensor->mclk_name, "cam_mclk0") && strcmp(sensor->mclk_name, "cam_mclk1") && strcmp(sensor->mclk_name, "cam_mclk2")) {
			cam_err("%s: error! only support cam_mclk0~2!", __func__);
			return -EINVAL;
		}
	} else {
		cam_err("%s: clock-names read failed", __func__);
		return ret;
	}

	// mclk/pwdn/rst multiplex
	sensor->is_pinmulti = of_property_read_bool(of_node, "pinmulti-enable");
	if (!sensor->is_pinmulti) {
		/* mclks */
		sensor->mclk = devm_clk_get(dev, sensor->mclk_name);
		if (IS_ERR(sensor->mclk)) {
			cam_err("unable to get cam_mclk%d\n", cell_id);
			ret = PTR_ERR(sensor->mclk);
			goto st_err;
		}

		/* pwdn-gpios */
		sensor->pwdn = devm_gpiod_get(dev, "pwdn", GPIOD_OUT_HIGH);
		if (IS_ERR(sensor->pwdn)) {
			cam_info("%s: unable to parse sensor%d pwdn gpio", __func__, cell_id);
			//ret = PTR_ERR(sensor->pwdn);
		} else {
			ret = gpiod_direction_output(sensor->pwdn, 0);
			if (ret < 0) {
				cam_err("%s: Failed to init sensor%d pwdn gpio", __func__, cell_id);
				goto st_err;
			}
		}

		/* rst-gpios */
		sensor->rst = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
		if (IS_ERR(sensor->rst)) {
			cam_info("%s: unable to parse sensor%d reset gpio", __func__, cell_id);
		} else {
			ret = gpiod_direction_output(sensor->rst, 0);
			if (ret < 0) {
				cam_err("%s: Failed to init sensor%d reset gpio", __func__, cell_id);
				goto st_err;
			}
		}
	} else {
		cam_info("cam_sensor%d is_pinmulti: %d\n", sensor->id, sensor->is_pinmulti);
		sensor->req_pinmulti = false;
	}

#ifdef CONFIG_ARCH_ZYNQMP
	cam_dbg("dptc-gpios,cell_id =0x%x",cell_id);
	/* dptc-gpios */
	sensor->dptc = devm_gpiod_get(dev, "dptc", GPIOD_OUT_HIGH);
	if (IS_ERR(sensor->dptc)) {
		cam_err("%s: unable to parse sensor%d dptc gpio", __func__, cell_id);
		ret = PTR_ERR(sensor->dptc);
	} else {
		ret = gpiod_direction_output(sensor->dptc, 1);
		if (ret < 0) {
			cam_err("%s: Failed to init sensor%d dptc gpio", __func__, cell_id);
			goto st_err;
		}
		gpiod_set_value_cansleep(sensor->dptc, 1);
		usleep_range(100 * 1000, 100 * 1000);
		gpiod_set_value_cansleep(sensor->dptc, 0);
		usleep_range(100 * 1000, 100 * 1000);
		gpiod_set_value_cansleep(sensor->dptc, 1);
		usleep_range(100 * 1000, 100 * 1000);
	}
#endif
	if (IS_ERR(sensor->rst) && IS_ERR(sensor->pwdn)) {
		cam_err("rst and pwdn both fail to get");
		ret = -EAGAIN;
	}

	return ret;

st_err:
	return ret;
}

static int cam_sensor_remove(struct platform_device *pdev)
{
	struct cam_sensor_device *msnr_dev;

	msnr_dev = platform_get_drvdata(pdev);
	if (!msnr_dev) {
		dev_err(&pdev->dev, "camera sensor device is NULL");
		return 0;
	}
	mutex_destroy(&msnr_dev->lock);
	cam_snr_dev_destroy(&msnr_dev->cdev, msnr_dev->id);
	cam_dbg("camera sensor%d removed", msnr_dev->id);
	devm_kfree(&pdev->dev, msnr_dev);

	return 0;
}

static int cam_sensor_probe(struct platform_device *pdev)
{
	struct cam_sensor_device *msnr_dev;
	int ret;

	cam_dbg("camera sensor begin to probed");

	msnr_dev = devm_kzalloc(&pdev->dev, sizeof(struct cam_sensor_device),
				GFP_KERNEL);
	if (!msnr_dev)
		return -ENOMEM;

	platform_set_drvdata(pdev, msnr_dev);
	msnr_dev->pdev = pdev;

	ret = camsnr_of_parse(msnr_dev);
	if (ret)
		return ret;

	atomic_set(&msnr_dev->usr_cnt, 0);
	mutex_init(&msnr_dev->lock);

	g_sdev[msnr_dev->id] = msnr_dev;
	cam_info("camera sensor%d probed", msnr_dev->id);

	return ret;
}

static const struct of_device_id cam_sensor_dt_match[] = {
	{ .compatible = "ky,cam-sensor" },
	{}
};

MODULE_DEVICE_TABLE(of, cam_sensor_dt_match);

static struct platform_driver camsnr_driver = {
	.probe  = cam_sensor_probe,
	.remove = cam_sensor_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = cam_sensor_dt_match,
	},
};

static int __init cam_sensor_init(void)
{
	int ret;

	ret = cam_snr_drv_init();
	if (ret < 0) {
		printk("camsnr cdev create failed\n");
		return ret;
	}

	return platform_driver_register(&camsnr_driver);
}

static void __exit cam_sensor_exit(void)
{
	platform_driver_unregister(&camsnr_driver);
	cam_snr_drv_deinit();
}

late_initcall(cam_sensor_init);
module_exit(cam_sensor_exit);

/* drivers/hwmon/sc7a20.c
 *
 *allwinner platform

 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>

#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/fs.h>
#include <linux/input-polldev.h>
#include <linux/device.h>
#include "../init-input.h"
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#include <linux/hrtimer.h>
#include <linux/ktime.h>

#if IS_ENABLED(CONFIG_PM)
#include <linux/pm.h>
#endif

/*
 * Defines
 */
#define assert(expr) \
	do {\
		if (!(expr)) {\
			printk("Assertion failed! %s,%d,%s,%s\n",\
				__FILE__, __LINE__, __func__, #expr);\
		} \
	} while (0)

#define sc7a20_DRV_NAME		"sc7a20"
#define SENSOR_NAME 		sc7a20_DRV_NAME

#define POLL_INTERVAL_MAX	1000
#define POLL_INTERVAL		50
#define INPUT_FUZZ	0
#define INPUT_FLAT	2
#define TIANJU_FILTER

#define sc7a20_REG_CTRL				0x20
#define sc7a20_REG_CTRL_RANGE       0x23
#define sc7a20_REG_DATA		  		0x00

#define sc7a20_MEASURING_RANGE      0x30
#define sc7a20_MEASURING_RANGE_2G   0x0
#define sc7a20_MEASURING_RANGE_4G   0x10
#define sc7a20_MEASURING_RANGE_8G   0x20
#define sc7a20_MEASURING_RANGE_16G  0x30

/* sc7a20 control bit */
#define sc7a20_CTRL_PWRON			0x47	/* power on */
#define sc7a20_CTRL_PWRDN			0x00	/* power donw */

#define MODE_CHANGE_DELAY_MS		100

#define CALIBRATION_NUM				40
#define AXIS_X_Y_RANGE_LIMIT		200
#define AXIS_X_Y_AVG_LIMIT			400
#define AXIS_Z_RANGE				200
#define AXIS_Z_DFT_G				1000
#define GOTO_CALI	  				100
#define FAILTO_CALI			        101

#define SC7A20_ENABLE			1
#define SC7A20_XOUT_L			0x28
#define SC7A20_XOUT_H			0x29
#define SC7A20_YOUT_L			0x2A
#define SC7A20_YOUT_H			0x2B
#define SC7A20_ZOUT_L			0x2C
#define SC7A20_ZOUT_H			0x2D
#define SC7A20_MODE				0x20
#define SC7A20_MODE1			0x21
#define SC7A20_MODE2			0x22
#define SC7A20_MODE3			0x23
#define SC7A20_BOOT				0x24
#define SC7A20_STATUS			0x27

#define GSENSOR_REG_CALI

/*

#ifdef GSENSOR_REG_CALI
static int reg[29] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A,
						0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
						0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A};
static int reg_data[29] = {0};
#endif

*/
#define Z_OFF_DEFAULT  (8) 		/* (110) */
#define XY_THR_N	  (-15) 	/* (-240) */
#define XY_THR_P	  (15) 		/* (240) */
#define Z_THR_MIN_N   (-50) 	/* (-780) */
#define Z_THR_MIN_P   (50) 		/* (780) */
#define Z_THR_MAX_N   (-78) 	/* (-1200) */
#define Z_THR_MAX_P   (78) 		/* (1200) */
#define SUM_DOT	   (50)
#define THRESHOLD_VAL (8) 		/* (40) */

//static struct device  			*hwmon_dev;
static struct i2c_client		*sc7a20_i2c_client;
static struct input_dev  *sc7a20_idev;

struct sc7a20_data_s {
	    struct i2c_client		*client;
	    struct input_dev		*inputDev;
	    struct mutex			interval_mutex;

		struct delayed_work		dwork;//allen
		int	time_of_cali;
		atomic_t enable;

#if IS_ENABLED(CONFIG_PM)
	volatile int suspend_indator;
#endif
	struct hrtimer hr_timer;
	struct work_struct wq_hrtimer;
	ktime_t ktime;

} sc7a20_data;

/* Addresses to scan */
static const unsigned short normal_i2c[] = {0x19, I2C_CLIENT_END};
static __u32 twi_id;
static unsigned int   delayMs = POLL_INTERVAL;

static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask;
#define dprintk(level_mask, fmt, arg...) \
	do {\
		if (unlikely(debug_mask & (level_mask))) \
			printk(KERN_DEBUG fmt, ## arg); \
	} while (0)

module_param_named(debug_mask, debug_mask, int, S_IRUGO);

static void sc7a20_reset(void);

#if 0
static char Read_Reg(unsigned char reg)
{
	char ret;
	ret = i2c_smbus_read_byte_data(sc7a20_i2c_client, reg);
	return ret;
}
static void Write_Input(char addr, char thedata)
{
	int result;
	//result = sensor_write_reg(sc7a20_i2c_client, addr, thedata);
	result = i2c_smbus_write_byte_data(sc7a20_i2c_client, addr, thedata);
}

#endif


/**
 * gsensor_detect - Device detection callback for automatic device creation
 * return value:
 *	                = 0; success;
 *	                < 0; err
 */

static int sc7a20_gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	//__s32 ret = 0;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	if (twi_id == adapter->nr) {
		printk("%s: addr= %x\n", __func__, client->addr);
/*
		ret = gsensor_i2c_test(client);
		if (!ret) {
			pr_info("%s:I2C connection might be something wrong or maybe the other gsensor equipment! \n",__func__);
			return -ENODEV;
		} else
*/
		{
			dprintk(DEBUG_INIT, "%s: I2C connection sucess!\n", __func__);
			strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
			return 0;
		}

	} else {
		dprintk(DEBUG_INIT, "%s: I2C connection error!\n",  __func__);
		return -ENODEV;
	}
}

static ssize_t sc7a20_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = sc7a20_i2c_client;
	struct sc7a20_data_s *sc7a20 = NULL;

	sc7a20 = i2c_get_clientdata(client);
	return sprintf(buf, "%d\n", delayMs);
}

static ssize_t sc7a20_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	//int error;
	struct i2c_client *client = sc7a20_i2c_client;
	struct sc7a20_data_s *sc7a20  = NULL;

	sc7a20    = i2c_get_clientdata(client);
	dprintk(DEBUG_INIT, "delay store %d\n", __LINE__);

	data = simple_strtoul(buf, NULL, 10);
	//error = strict_strtoul(buf, 10, &data);
	//if (error)
	//	return error;

	if (data > POLL_INTERVAL_MAX)
		data = POLL_INTERVAL_MAX;
	if (0 >= data)
		data = 10;

	mutex_lock(&sc7a20->interval_mutex);
	delayMs = data;
	mutex_unlock(&sc7a20->interval_mutex);
	printk(KERN_INFO "%s: sc7a20 delay %d\n", __func__, delayMs);

	if (atomic_read(&sc7a20_data.enable)) {
		if (delayMs <= 10) {
			// early 0.5ms
			sc7a20_data.ktime = ktime_set(0, delayMs* NSEC_PER_MSEC - NSEC_PER_MSEC / 2);
		} else {
			sc7a20_data.ktime = ktime_set(0, delayMs* NSEC_PER_MSEC);
		}
		hrtimer_start(&sc7a20_data.hr_timer, sc7a20_data.ktime, HRTIMER_MODE_REL);
	}

	return count;
}

static void enable_sensor(int enable)
{
	int error;
	u32 val = 0;

	if (enable) {
		printk(KERN_INFO "%s: sc7a20_CTRL_PWRON\n", __func__);
		error = i2c_smbus_write_byte_data(sc7a20_i2c_client, sc7a20_REG_CTRL, sc7a20_CTRL_PWRON);
		assert(error == 0);

		// set sensor max range is 4G, because cts test request.
		val = i2c_smbus_read_byte_data(sc7a20_i2c_client, sc7a20_REG_CTRL_RANGE);
		val &= ~(sc7a20_MEASURING_RANGE);
		error = i2c_smbus_write_byte_data(sc7a20_i2c_client, sc7a20_REG_CTRL_RANGE, val | sc7a20_MEASURING_RANGE_4G);
		assert(error == 0);

		printk(KERN_INFO "%s: sc7a20 delay %d\n", __func__, delayMs);
		if (delayMs <= 10) {
			// early 0.5ms
			sc7a20_data.ktime = ktime_set(0, delayMs * NSEC_PER_MSEC - NSEC_PER_MSEC / 2);
		} else {
			sc7a20_data.ktime = ktime_set(0, delayMs * NSEC_PER_MSEC);
		}
		hrtimer_start(&sc7a20_data.hr_timer, sc7a20_data.ktime, HRTIMER_MODE_REL);
	} else {
		printk(KERN_INFO "%s: sc7a20_CTRL_PWRDN\n", __func__);
		hrtimer_cancel(&sc7a20_data.hr_timer);
		error = i2c_smbus_write_byte_data(sc7a20_i2c_client, sc7a20_REG_CTRL, sc7a20_CTRL_PWRDN);
		assert(error == 0);
	}

}

static ssize_t sc7a20_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = sc7a20_i2c_client;
	struct sc7a20_data_s *sc7a20 = NULL;

	sc7a20 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&sc7a20_data.enable));

}

static ssize_t sc7a20_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;

	struct i2c_client *client = sc7a20_i2c_client;
	struct sc7a20_data_s *sc7a20 = NULL;

	sc7a20 = i2c_get_clientdata(client);

	printk(KERN_INFO "%s: buf=%s\n", __func__, buf);
	data = simple_strtoul(buf, NULL, 10);
	//error = strict_strtoul(buf, 10, &data);

	//if (error) {
	//	pr_err("%s strict_strtoul error\n", __FUNCTION__);
	//	goto exit;
	//}

	atomic_set(&sc7a20_data.enable, data);
	enable_sensor(data);
	return count;

//exit:
//	return error;
}



static ssize_t SC7A20_register_store(struct device *dev,
	    struct device_attribute *attr,
	    const char *buf, size_t count)
{
	unsigned int address, value;
	int result = 0;
	sscanf(buf, "0x%x=0x%x", &address, &value);

	result = i2c_smbus_write_byte_data(sc7a20_i2c_client, address, value);

	if (result)
		printk("%s:fail to write sensor_register\n", __func__);

	return count;
}

static ssize_t SC7A20_register_show(struct device *dev,
	    struct device_attribute *attr, char *buf)
{
	size_t count = 0;
	//u8 reg[0x5b];
	char i;
	int buffer[3] = {0};
	i = 0x0f;
	*buffer = i;
	buffer[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, i);
	count += sprintf(buf, "0x%x: 0x%x\n", i, buffer[0]);
	for (i = 0x10; i < 0x5a; i++) {
		*buffer = i;
		buffer[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, i);
		count += sprintf(&buf[count], "0x%x: 0x%x\n", i, buffer[0]);
	}
	return count;
}
#if 0
static DEVICE_ATTR(enable, S_IRUGO,
		sc7a20_enable_show, sc7a20_enable_store);

static DEVICE_ATTR(delay, S_IRUGO,
		sc7a20_delay_show, sc7a20_delay_store);

static DEVICE_ATTR(reg, S_IRUGO,
		SC7A20_register_show, SC7A20_register_store);

static DEVICE_ATTR(reg13_reset, S_IRUGO,
		SC7A20_reg13_reset, NULL);

static DEVICE_ATTR(calibration_run, S_IRUGO, //S_IWUSR|S_IWGRP, //calibration_run
	    NULL, SC7A20_3_axis_Calibration);
#endif
static DEVICE_ATTR(enable, 0644,
		sc7a20_enable_show, sc7a20_enable_store);

static DEVICE_ATTR(delay, 0644,
		sc7a20_delay_show, sc7a20_delay_store);

static DEVICE_ATTR(reg, 0644,
		SC7A20_register_show, SC7A20_register_store);

//static DEVICE_ATTR(reg13_reset, 0644,
//		SC7A20_reg13_reset, NULL);

//static DEVICE_ATTR(calibration_run, 0644,//S_IWUSR|S_IWGRP, //calibration_run
 //	   NULL, SC7A20_3_axis_Calibration);

static struct attribute *sc7a20_attributes[] = {
//	&dev_attr_reg13_reset.attr,
	&dev_attr_reg.attr,
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
//	&dev_attr_calibration_run.attr,
	NULL
};

static struct attribute_group sc7a20_attribute_group = {
	.attrs = sc7a20_attributes
};

#if 0
static int report_abs(void)
{
	s8 xyz[6] = {0, 0, 0};
	s16   x   = 0, y = 0, z = 0;
	s8   ret  = 0;
	s16 conut = 0;
	while (1) {
		ret = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_STATUS);
		if ((ret & 0x08) != 0) {
			break;
		}
		msleep(1);
		conut++;
		if (conut > 40)
			break;
	}

	xyz[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_XOUT_L);
	xyz[1] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_XOUT_H);

	xyz[2] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_YOUT_L);
	xyz[3] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_YOUT_H);

	xyz[4] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_ZOUT_L);
	xyz[5] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_ZOUT_H);

	//x = ((xyz[1]) << 8) | xyz[0];
	//y = ((xyz[3]) << 8) | xyz[2];
	//z = ((xyz[5]) << 8) | xyz[4];

	x = xyz[1];
	y = xyz[3];
	z = xyz[5];


	input_report_abs(sc7a20_idev->input, ABS_X, x);
	input_report_abs(sc7a20_idev->input, ABS_Y, y);
	input_report_abs(sc7a20_idev->input, ABS_Z, z);

	input_sync(sc7a20_idev->input);
	return 1;
}


/*
 * I2C init/probing/exit functions
 */

////////////////////////////////djq////////////////////////////////////
static int sc7a20_acc_get_data(int *xyz)
{
	u8 acc_data[6];
	/* x,y,z hardware data */
	int hw_d[3] = { 0 };
	signed short temp_data[3];
	int count = 0;
	u8 buf[3];
	buf[0] = SC7A20_STATUS;
	while (1) {
		buf[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_STATUS);
		if ((buf[0] & 0x08) != 0) {
			break;
		}
		msleep(1);
		count++;
		if (count > 40)
			break;
	}

	buf[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_XOUT_L);
	acc_data[0] = buf[0];
	buf[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_XOUT_H);

	acc_data[1] = buf[0];

	buf[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_YOUT_L);
	acc_data[2] = buf[0];
	buf[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_YOUT_H);
	acc_data[3] = buf[0];

	buf[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_ZOUT_L);
	acc_data[4] = buf[0];
	buf[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_ZOUT_H);
	acc_data[5] = buf[0];

	/* 12-bit */
	temp_data[0] = (signed short)((acc_data[1] << 8 | acc_data[0]) >> 4);
	temp_data[1] = (signed short)((acc_data[3] << 8 | acc_data[2]) >> 4);
	temp_data[2] = (signed short)((acc_data[5] << 8 | acc_data[4]) >> 4);

	hw_d[0] = (temp_data[0] & 0x0800) ? (temp_data[0] | 0xFFFFF800) : (temp_data[0]);
	hw_d[1] = (temp_data[1] & 0x0800) ? (temp_data[1] | 0xFFFFF800) : (temp_data[1]);
	hw_d[2] = (temp_data[2] & 0x0800) ? (temp_data[2] | 0xFFFFF800) : (temp_data[2]);



	xyz[0] = (hw_d[0]) * 1000;
	xyz[1] = (hw_d[1]) * 1000;
	xyz[2] = (hw_d[2]) * 1000;

	return 0;
}


static int sc7a20_acc_hw_init(void)
{
	char ret;
	ret = i2c_smbus_write_byte_data(sc7a20_i2c_client, SC7A20_MODE, 0x77);
	if (ret < 0) {
		return -1;
	}

	return 0;
}

#endif
static enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer)
{
	schedule_work(&sc7a20_data.wq_hrtimer);
	hrtimer_forward_now(&sc7a20_data.hr_timer, sc7a20_data.ktime);
	return HRTIMER_RESTART;
}

void find_max_min(s16 *array, int len, s16 *max, s16 *min)
{
	s16 temp_max = *array;
	s16 temp_min = *array;

	int i = 0;
	for (i = 0; i < len; i++) {
		if (*(array + i) > temp_max)
			temp_max = *(array + i);

		if (*(array + i) < temp_min)
			temp_min = *(array + i);
	}

	*max = temp_max;
	*min = temp_min;
}

static void wq_func_hrtimer(struct work_struct *work)
{
	s8 xyz[6] = {0, 0, 0};
	s16 x = 0, y = 0, z = 0;
/*
	static s16 axis_x_off = 0;  			//jq 2017-08-07
	static s16 axis_y_off = 0;  			//jq 2017-08-07
	static s16 axis_z_off = Z_OFF_DEFAULT;  //jq 2017-08-07
	static u16 index = 0;
	static s16 x_value[SUM_DOT] = {0};
	static s16 y_value[SUM_DOT] = {0};
	static s16 z_value[SUM_DOT] = {0};

	int flag_x = 0;
	int flag_y = 0;
	int flag_z = 0;

	int i = 0;

	s16 x_min = 0;
	s16 x_max = 0;
	s16 y_min = 0;
	s16 y_max = 0;
	s16 z_min = 0;
	s16 z_max = 0;

	s32 temp_x_value = 0;
	s32 temp_y_value = 0;
	s32 temp_z_value = 0;
*/
	xyz[0] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_XOUT_L);
	xyz[1] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_XOUT_H);

	xyz[2] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_YOUT_L);
	xyz[3] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_YOUT_H);

	xyz[4] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_ZOUT_L);
	xyz[5] = i2c_smbus_read_byte_data(sc7a20_i2c_client, SC7A20_ZOUT_H);
/*
	xyz[1] = ((xyz[1]) << 8) >> 8;
	xyz[3] = ((xyz[3]) << 8) >> 8;
	xyz[5] = ((xyz[5]) << 8) >> 8;

	if (index < SUM_DOT) {
		if (xyz[1] > XY_THR_N && xyz[1] < XY_THR_P)
			flag_x = 1;
		else
			flag_x = 0;

		if (xyz[3] > XY_THR_N && xyz[3] < XY_THR_P)
			flag_y = 1;
		else
			flag_y = 0;

	    if ((xyz[5] > Z_THR_MAX_N && xyz[5] < Z_THR_MIN_N)  || (xyz[5] > Z_THR_MIN_P && xyz[5] < Z_THR_MAX_P))
			flag_z = 1;
		else
			flag_z = 0;

		if (flag_x == 1 && flag_y == 1 && flag_z == 1) {
			x_value[index] = xyz[1];
			y_value[index] = xyz[3];
			z_value[index] = xyz[5];
			index = index + 1;
		} else
			index = 0;
	}

	if (index == SUM_DOT) {
		find_max_min(x_value, SUM_DOT, &x_max, &x_min);
		// printk("aaaaa %s:x_max=%d\n  x_min=%d\n ",__func__,x_max,x_min);
		find_max_min(y_value, SUM_DOT, &y_max, &y_min);
		// printk("aaaaa %s:y_max=%d\n  y_min=%d\n ",__func__,y_max,y_min);
		find_max_min(z_value, SUM_DOT, &z_max, &z_min);
		// printk("aaaaa %s:z_max=%d\n  z_min=%d\n",__func__,z_max,z_min);

		if (((x_max - x_min) < THRESHOLD_VAL)  && ((y_max - y_min) < THRESHOLD_VAL)
				&& ((z_max - z_min) < THRESHOLD_VAL)) {
			temp_x_value = 0;
			for (i = 0; i < SUM_DOT; i++) {
				temp_x_value += x_value[i];
			}
			temp_x_value = temp_x_value / SUM_DOT;
			axis_x_off = 0 - (s16)temp_x_value;

			temp_y_value = 0;
			for (i = 0; i < SUM_DOT; i++) {
					temp_y_value += y_value[i];
			}
			temp_y_value = temp_y_value / SUM_DOT;
			axis_y_off = 0 - (s16)temp_y_value;

			temp_z_value = 0;
			for (i = 0; i < SUM_DOT; i++) {
				temp_z_value += z_value[i];
			}
			temp_z_value = temp_z_value / SUM_DOT;

			if (temp_z_value > Z_THR_MAX_N && temp_z_value < Z_THR_MIN_N)
				axis_z_off = -64 - (s16)temp_z_value;
			else
				axis_z_off = 64 - (s16)temp_z_value;
			 // printk("aaaaa %s:axis_x_off=%d\n  axis_y_off=%d\n  axis_z_off=%d\n",__func__,axis_x_off,axis_y_off,axis_z_off);
		}
		index = 0;

	}
	//x += axis_x_off;
	//y += axis_y_off;
	//z += axis_z_off;
	xyz[1] += axis_x_off;
	xyz[3] += axis_y_off;
	xyz[5] += axis_z_off;

	//x = ((xyz[1]) << 8) >> 8;
	//y = ((xyz[3]) << 8) >> 8;
	//z = ((xyz[5]) << 8) >> 8;
*/
	x = xyz[1];
	y = xyz[3];
	z = xyz[5];

	if (x == 0 && y == 0 && z == 0) {
		printk("sc7a20 gsensor x y z all 0!!!\n");
		sc7a20_reset();
		return;
	}

	input_report_rel(sc7a20_data.inputDev, REL_X, x);
	input_report_rel(sc7a20_data.inputDev, REL_Y, y);
	input_report_rel(sc7a20_data.inputDev, REL_Z, z);

	input_sync(sc7a20_data.inputDev);
	//printk("chongyuzhao report %d - %d - %d\n", x, y, z);
}

static int sc7a20_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	char reg_cali;

	int result;
	struct i2c_adapter *adapter;
	struct sc7a20_data_s *data = &sc7a20_data;

	if (gsensor_info.dev == NULL)
		gsensor_info.dev = &client->dev;

	printk("sc7a20 probe\n");
	sc7a20_i2c_client = client;
	adapter = to_i2c_adapter(client->dev.parent);
	result = i2c_check_functionality(adapter,
					 I2C_FUNC_SMBUS_BYTE |
					 I2C_FUNC_SMBUS_BYTE_DATA);
	assert(result);
//hwmon_device_register_with_info
	//hwmon_dev = hwmon_device_register(&client->dev);
	//assert(!(IS_ERR(hwmon_dev)));

	/*input poll device register */
	sc7a20_idev = input_allocate_device();
	if (!sc7a20_idev) {
		dev_err(&client->dev, "alloc poll device failed!\n");
		result = -ENOMEM;
		return result;
	}

	sc7a20_idev->name = sc7a20_DRV_NAME;
	sc7a20_idev->id.bustype = BUS_I2C;

	mutex_init(&data->interval_mutex);
	input_set_capability(sc7a20_idev, EV_REL, REL_X);
	input_set_capability(sc7a20_idev, EV_REL, REL_Y);
	input_set_capability(sc7a20_idev, EV_REL, REL_Z);
	//input_set_abs_params(sc7a20_idev, ABS_X, -512, 512, INPUT_FUZZ, INPUT_FLAT);
	//input_set_abs_params(sc7a20_idev, ABS_Y, -512, 512, INPUT_FUZZ, INPUT_FLAT);
	//input_set_abs_params(sc7a20_idev, ABS_Z, -512, 512, INPUT_FUZZ, INPUT_FLAT);
	input_set_drvdata(sc7a20_idev, &sc7a20_data);

	result = input_register_device(sc7a20_idev);
	if (result) {
		dev_err(&client->dev, "registerdevice failed!\n");
		return result;
	}
	result = sysfs_create_group(&sc7a20_idev->dev.kobj, &sc7a20_attribute_group);

	if (result) {
		dev_err(&client->dev, "create sys failed\n");
	}
	kobject_uevent(&sc7a20_idev->dev.kobj, KOBJ_CHANGE);

	data->client  = client;
	data->inputDev = sc7a20_idev;
	i2c_set_clientdata(client, data);

	//result = i2c_smbus_write_byte_data(sc7a20_i2c_client, sc7a20_REG_CTRL, sc7a20_CTRL_PWRON);
	//assert(result == 0);

	reg_cali = i2c_smbus_read_byte_data(sc7a20_i2c_client, 0x13);
	printk("line %d reg_cali = 0x%x \n", __LINE__, reg_cali);

	hrtimer_init(&sc7a20_data.hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	sc7a20_data.hr_timer.function = my_hrtimer_callback;
	INIT_WORK(&sc7a20_data.wq_hrtimer, wq_func_hrtimer);

	return result;
}

static int sc7a20_remove(struct i2c_client *client)
{
	int result;
	result = i2c_smbus_write_byte_data(sc7a20_i2c_client, sc7a20_REG_CTRL, sc7a20_CTRL_PWRDN);
	assert(result == 0);
	//hwmon_device_unregister(hwmon_dev);

	hrtimer_cancel(&sc7a20_data.hr_timer);
	sysfs_remove_group(&sc7a20_idev->dev.kobj, &sc7a20_attribute_group);
	input_unregister_device(sc7a20_idev);
	i2c_set_clientdata(sc7a20_i2c_client, NULL);

	return result;
}

#if IS_ENABLED(CONFIG_PM)
static int sc7a20_resume(struct device *dev)
{
	int enable = atomic_read(&sc7a20_data.enable);
	input_set_power_enable(&(gsensor_info.input_type), 1);
	enable_sensor(enable);
	return 0;
}

static int sc7a20_suspend(struct device *dev)
{
	enable_sensor(0);
	input_set_power_enable(&(gsensor_info.input_type), 0);
	return 0;
}
#endif

static void sc7a20_reset(void)
{
	int result;
	int val_reg;
	int MTPSETTING, B57H, B1BH, i;
	printk("sc7a20 reset power down\n");
	result = i2c_smbus_write_byte_data(sc7a20_i2c_client, sc7a20_REG_CTRL, sc7a20_CTRL_PWRDN);
	assert(result == 0);
	msleep(50);

	printk("sc7a20 reset power on\n");
	val_reg = i2c_smbus_read_byte_data(sc7a20_i2c_client, 0x0f);
	if (val_reg != 0x11) {
		for (i = 0; i < 3; i++) {
			MTPSETTING = 0x07;
			B57H = 0x00;
			B1BH = 0x08;
			sc7a20_i2c_client->addr = 0x18;
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x59, MTPSETTING);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1b, B1BH);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x57, B57H);
			sc7a20_i2c_client->addr = 0x19;
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x59, MTPSETTING);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1b, B1BH);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x57, B57H);
			sc7a20_i2c_client->addr = 0x1a;
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x59, MTPSETTING);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1b, B1BH);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x57, B57H);
			sc7a20_i2c_client->addr = 0x1b;
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x59, MTPSETTING);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1b, B1BH);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x57, B57H);
			sc7a20_i2c_client->addr = 0x1c;
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x59, MTPSETTING);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1b, B1BH);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x57, B57H);
			sc7a20_i2c_client->addr = 0x1d;
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x59, MTPSETTING);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1b, B1BH);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x57, B57H);
			sc7a20_i2c_client->addr = 0x1e;
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x59, MTPSETTING);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1b, B1BH);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x57, B57H);
			sc7a20_i2c_client->addr = 0x1f;
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x59, MTPSETTING);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1b, B1BH);
			i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x57, B57H);
		}
		sc7a20_i2c_client->addr = 0x1d;
		msleep(100);
		i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x24, 0x80);
		i2c_smbus_write_byte_data(sc7a20_i2c_client, 0x1e, 0x05);
		val_reg = i2c_smbus_read_byte_data(sc7a20_i2c_client, 0x0f);
		if (val_reg != 0x11)
			printk("momkey sc7a20 resume fail! regchip_id=0x%x\n", val_reg);
	}

	result = i2c_smbus_write_byte_data(sc7a20_i2c_client, sc7a20_REG_CTRL, sc7a20_CTRL_PWRON);
	assert(result == 0);
}
static const struct of_device_id sc7a20_of_match[] = {
	{.compatible = "allwinner,sc7a20"},
	{},
};

static const struct i2c_device_id sc7a20_id[] = {
	{ sc7a20_DRV_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sc7a20_id);

#if IS_ENABLED(CONFIG_PM)
static UNIVERSAL_DEV_PM_OPS(sc7a20_pm_ops, sc7a20_suspend,
		sc7a20_resume, NULL);
#endif

static struct i2c_driver sc7a20_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.of_match_table = sc7a20_of_match,
		.name = sc7a20_DRV_NAME,
		.owner = THIS_MODULE,
#if IS_ENABLED(CONFIG_PM)
		.pm = &sc7a20_pm_ops,
#endif
	},
	.probe = sc7a20_probe,
	.remove	= sc7a20_remove,
	.id_table = sc7a20_id,
	.address_list = normal_i2c,
};

static int startup(void)
{
	int ret = -1;

	printk("function=%s=========LINE=%d. \n", __func__, __LINE__);

	if (input_sensor_startup(&(gsensor_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else
		ret = input_sensor_init(&(gsensor_info.input_type));

	if (0 != ret) {
	    printk("%s:gsensor.init_platform_resource err. \n", __func__);
	}

	twi_id = gsensor_info.twi_id;
	input_set_power_enable(&(gsensor_info.input_type), 1);
	return 0;
}

static int __init sc7a20_init(void)
{
	int ret = -1;

	if (startup() != 0)
		return -1;
	if (!gsensor_info.isI2CClient)
		sc7a20_driver.detect = sc7a20_gsensor_detect;
	ret = i2c_add_driver(&sc7a20_driver);
	if (ret < 0) {
		printk("add sc7a20 i2c driver failed\n");
		return -ENODEV;
	}

	return 0;
}

static void __exit sc7a20_exit(void)
{
	printk("remove sc7a20 i2c driver.\n");
	input_sensor_free(&(gsensor_info.input_type));
	input_set_power_enable(&(gsensor_info.input_type), 0);
	i2c_del_driver(&sc7a20_driver);
}

MODULE_AUTHOR("allwinner");
MODULE_DESCRIPTION("sc7a20 Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.3");

module_init(sc7a20_init);
module_exit(sc7a20_exit);


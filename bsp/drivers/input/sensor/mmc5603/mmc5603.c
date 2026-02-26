/*
 * Copyright (C) 2012 MEMSIC.
 *
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
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/miscdevice.h>
#include <linux/mutex.h>
#include <linux/mm.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/regulator/consumer.h>
#include <linux/input.h>
#include <linux/regmap.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/version.h>


#include <linux/hrtimer.h>
#include <linux/kthread.h>
#include <linux/sched/rt.h>
#include <linux/sched.h>
#include "mmc5603.h"
#include "../../init-input.h"

#define POLL_INTERVAL_DEFAULT 20

#define MMC5603X_MIN_DELAY 5
#define MMC5603X_MAX_DELAY 1000

#define MMC5603X_SINGLE_POWER 0

#define MMC5603X_DELAY_TM_MS    10
#define MMC5603X_DELAY_SET  75
#define MMC5603X_DELAY_RESET 75
#define MMC5603X_RETRY_COUNT    10
#define MMC5603X_DEFAULT_INTERVAL_MS    20
#define MMC5603X_TIMEOUT_SET_MS 15000

#define MISC_DEVICE_REGISTER 0

struct MMC5603X_data {
  uint8_t sensor_state;
	struct i2c_client *i2c;
	struct input_dev *idev;
	struct hrtimer work_timer;
	struct work_struct work;
	struct workqueue_struct *wq;
	bool hrtimer_running;
	int poll_interval;
	struct mutex lock;
	atomic_t enabled;
	unsigned int on_before_suspend;
};

struct i2c_client *this_client;

static struct sensor_config_info mag_config = {
	.input_type = MAGNETOMETER_TPYE,
	.np_name = "mmc5603",
};

static int MMC5603X_i2c_rxdata(struct i2c_client *i2c, unsigned char *rxData, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr = i2c->addr,
			.flags = 0,
			.len = 1,
			.buf = rxData,
		},
		{
			.addr = i2c->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = rxData,
		}, };
	unsigned char addr = rxData[0];

	if (i2c_transfer(i2c->adapter, msgs, 2) < 0) {
		dev_err(&i2c->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}

	dev_vdbg(&i2c->dev, "RxData: len=%02x, addr=%02x, data=%02x",
			length, addr, rxData[0]);
	return 0;
}

static int MMC5603X_i2c_txdata(struct i2c_client *i2c, unsigned char *txData, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = i2c->addr,
			.flags = 0,
			.len = length,
			.buf = txData,
		}, };

	if (i2c_transfer(i2c->adapter, msg, 1) < 0) {
		dev_err(&i2c->dev, "%s: transfer failed.", __func__);
		return -EIO;
	}

	dev_vdbg(&i2c->dev, "TxData: len=%02x, addr=%02x data=%02x",
			length, txData[0], txData[1]);
	return 0;
}

void MMC5603X_SET(struct MMC5603X_data *dev)
{
	unsigned char buffer[2];

	/* Write 0x08 to register 0x1B, set SET bit high */
	buffer[0] = MMC5603X_REG_CTRL0;
	buffer[1] = MMC5603X_CMD_SET;
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL0, __LINE__);
	}

	/* Delay to finish the SET operation */
	mdelay(1);

	return;
}

/*********************************************************************************
* decription: RESET operation
*********************************************************************************/
void MMC5603X_RESET(struct MMC5603X_data *dev)
{
  unsigned char buffer[2];

	/* Write 0x10 to register 0x1B, set RESET bit high */
  buffer[0] = MMC5603X_REG_CTRL0;
	buffer[1] = MMC5603X_CMD_RESET;
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL0, __LINE__);
	}

	/* Delay to finish the RESET operation */
	mdelay(1);

	return;
}

int MMC5603X_Auto_SelfTest(struct MMC5603X_data *dev)
{
	unsigned char buffer[2];
	int rc = 0;

	/* Write 0x40 to register 0x1B, set Auto_st_en bit high */
	buffer[0] = MMC5603X_REG_CTRL0;
	buffer[1] = MMC5603X_CMD_AUTO_ST_EN;
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL0, __LINE__);
	}

	/* Delay 15ms to finish the selftest process */
	mdelay(15);

	/* Read register 0x18, check Sat_sensor bit */
	buffer[0] = MMC5603X_REG_STATUS1;
	rc = MMC5603X_i2c_rxdata(dev->i2c, buffer, 1);
	if (rc) {
		dev_err(&dev->i2c->dev, "read reg 0x%x failed.(%d)\n", MMC5603X_REG_STATUS1, rc);
		return -1;
	}
	if ((buffer[0] & MMC5603X_SAT_SENSOR)) {
		return -1;
	}

	return 1;
}

int MMC5603X_Saturation_Checking(struct MMC5603X_data *dev)
{
	int ret = 0; //1 pass, -1 fail, 0 elapsed time is less 5 seconds

	/* If sampling rate is 50Hz, then do saturation checking every 250 loops, i.e. 5 seconds */
	static int NumOfSamples = 250;
	static int cnt;
	unsigned char buffer[2];

	if ((cnt++) >= NumOfSamples) {
		cnt = 0;
		ret = MMC5603X_Auto_SelfTest(dev);
		if (ret == -1) {
			/* Sensor is saturated, need to do SET operation */
			MMC5603X_SET(dev);
		}

		/* Do TM_M after selftest operation */
		buffer[0] = MMC5603X_REG_CTRL0;
		buffer[1] = MMC5603X_CMD_TMM;
		if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
			dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL0, __LINE__);
		}

		mdelay(8);
	}

	return ret;
}

static int myfabs(int data)
{
	if (data < 0) {
		return -data;
	} else {
		return data;
	}
}

void MMC5603_Auto_Switch(struct MMC5603X_data *dev, int *mag)
{
  unsigned char buffer[2];

	if (dev->sensor_state == 1) {
		/* If X or Y axis output exceed 10 Gauss, then switch to single mode */
		if ((myfabs(mag[0]) > 10240) || (myfabs(mag[1]) > 10240)) {
			dev->sensor_state = 2;

		/* Disable continuous mode */
		buffer[0] = MMC5603X_REG_CTRL2;
		buffer[1] = 0;
		if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
			dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL2, __LINE__);
		}
		mdelay(15); //Delay 15ms to finish the last sampling

		/* Do SET operation */
		MMC5603X_SET(dev);

		/* Do TM_M before next data reading */
		buffer[0] = MMC5603X_REG_CTRL0;
		buffer[1] = MMC5603X_CMD_TMM;
		if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
			dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL0, __LINE__);
		}
		mdelay(8);//Delay 8ms to finish the TM_M operation
		}
	} else if (dev->sensor_state == 2) {
		/* If both of X and Y axis output less than 8 Gauss, then switch to continuous mode with Auto_SR */
		if ((myfabs(mag[0]) < 8192) && (myfabs(mag[1]) < 8192)) {
			dev->sensor_state = 1;

			/* Enable continuous mode with Auto_SR */
			buffer[0] = MMC5603X_REG_CTRL0;
			buffer[1] = MMC5603X_CMD_CMM_FREQ_EN|MMC5603X_CMD_AUTO_SR_EN;
			if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
				dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL0, __LINE__);
			}

			buffer[0] = MMC5603X_REG_CTRL2;
			buffer[1] = MMC5603X_CMD_CMM_EN;
			if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
				dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL2, __LINE__);
			}
		} else {
			/* Sensor checking */
			if (MMC5603X_Saturation_Checking(dev) == 0) {
				/* Do TM_M before next data reading */
				buffer[0] = MMC5603X_REG_CTRL0;
				buffer[1] = MMC5603X_CMD_TMM;
				if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
					dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL0, __LINE__);
				}
			}
		}
	}

	return;
}


static int MMC5603X_read_xyz(struct MMC5603X_data *dev, int *vec)
{
	unsigned char data[6];
	unsigned int tmp[3] = {0};
	int rc = 0;

	/* read xyz raw data */
	data[0] = MMC5603X_REG_DATA;
	rc = MMC5603X_i2c_rxdata(dev->i2c, data, 6);
	if (rc) {
		dev_err(&dev->i2c->dev, "read reg id failed.(%d)\n", rc);
		return rc;
	}
	tmp[0] = data[0] << 8 | data[1];
	tmp[1] = data[2] << 8 | data[3];
	tmp[2] = data[4] << 8 | data[5];

	/*Remap here according to IC and board coordination*/
	vec[0] = tmp[0] - MMC5603X_OFFSET_X;
	vec[1] = tmp[1] - MMC5603X_OFFSET_Y;
	vec[2] = tmp[2] - MMC5603X_OFFSET_Z;

	MMC5603_Auto_Switch(dev, vec);

	return rc;
}

static void MMC5603X_report_values(struct MMC5603X_data *dev, int *xyz)
{
	input_report_abs(dev->idev, ABS_RX, xyz[0]);
	input_report_abs(dev->idev, ABS_RY, xyz[1]);
	input_report_abs(dev->idev, ABS_RZ, xyz[2]);
	input_sync(dev->idev);
}

static void MMC5603X_input_work_func(struct MMC5603X_data *dev)
{
	int xyz[3] = { 0 };
	int err;

	err = MMC5603X_read_xyz(dev, xyz);

	if (err < 0) {
		printk("MMC5603X_read_xyz failed\n");
	} else {
		MMC5603X_report_values(dev, xyz);
	}
}

static enum hrtimer_restart MMC5603X_work(struct hrtimer *timer)
{
	struct MMC5603X_data *mag;
	ktime_t poll_delay;
	mag = container_of((struct hrtimer *)timer, struct MMC5603X_data, work_timer);

	queue_work(mag->wq, &mag->work);
	if (mag->poll_interval > 0) {
		poll_delay = ktime_set(0, mag->poll_interval * NSEC_PER_MSEC);
	} else {
		poll_delay = ktime_set(0, POLL_INTERVAL_DEFAULT * NSEC_PER_MSEC);
	}
	mag->hrtimer_running = true;
	hrtimer_forward_now(&mag->work_timer, poll_delay);

	return HRTIMER_RESTART;
}

static void report_event(struct work_struct *work)
{
	struct MMC5603X_data *mag = container_of(work, struct MMC5603X_data, work);

	if (atomic_read(&mag->enabled) <= 0) {
		return;
	}
	MMC5603X_input_work_func(mag);

	return;
}

static struct input_dev *MMC5603X_init_input(struct MMC5603X_data *dev)
{
	int status;
	//struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };
	struct input_dev *input = NULL;

	printk("%s\n", __FUNCTION__);
	hrtimer_init(&dev->work_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	dev->work_timer.function = MMC5603X_work;
	dev->hrtimer_running = false;

	INIT_WORK(&dev->work, report_event);
	dev->wq = create_singlethread_workqueue("magsensor_event");
	if (IS_ERR(dev->wq)) {
		dev_err(&dev->i2c->dev, "error create work queue\n");
		return NULL;
	}
	flush_workqueue(dev->wq);

	input = input_allocate_device();
	if (!input) {
		dev_err(&dev->i2c->dev, "error alloc input device\n");
		goto error_alloc_device;
	}

	input->name = "mmc5603x";
	input->id.bustype = BUS_I2C;

	__set_bit(EV_ABS, input->evbit);
	input_set_events_per_packet(input, 100);
	input_set_abs_params(input, ABS_RX, -32768, 32767, 0, 0);
	input_set_abs_params(input, ABS_RY, -32768, 32767, 0, 0);
	input_set_abs_params(input, ABS_RZ, -32768, 32767, 0, 0);

	/* Report the dummy value */
	input_set_abs_params(input, ABS_MISC, INT_MIN, INT_MAX, 0, 0);

	input_set_capability(input, EV_REL, REL_X);
	input_set_capability(input, EV_REL, REL_Y);
	input_set_capability(input, EV_REL, REL_Z);

	status = input_register_device(input);
	if (status) {
		dev_err(&dev->i2c->dev, "error registering input device\n");
		goto error_register;
	}

	return input;

error_register:
	input_free_device(input);
error_alloc_device:
	destroy_workqueue(dev->wq);
	return NULL;
}


static int MMC5603X_check_device(struct MMC5603X_data *dev)
{
	int rc;
	unsigned char rd_buffer[2];
	rd_buffer[0] = MMC5603X_REG_PRODUCTID_1;
	rc = MMC5603X_i2c_rxdata(dev->i2c, rd_buffer, 1);
	if (rc) {
		dev_err(&dev->i2c->dev, "read reg id failed.(%d)\n", rc);
		return rc;
	}
	printk("MMC5603X read ID is 0x%x\n", rd_buffer[0]);

	if (rd_buffer[0] != MMC5603X_PRODUCT_ID)
		return -ENODEV;

	return 0;
}



void MMC5603X_Auto_SelfTest_Configuration(struct MMC5603X_data *dev)
{
	int i;
	unsigned char reg_value[3];
	unsigned short st_thr_data[3] = {0};
	unsigned short st_thr_new[3] = {0};

	unsigned short st_thd[3] = {0};
	unsigned char st_thd_reg[3];
	unsigned char buffer[2];
	int rc = 0;

	/* Read trim data from reg 0x27-0x29 */
	reg_value[0] = MMC5603X_REG_ST_X_VAL;
	rc = MMC5603X_i2c_rxdata(dev->i2c, reg_value, 3);
	if (rc) {
		dev_err(&dev->i2c->dev, "read reg OTP failed.(%d)\n", rc);
		return;
	}

	for (i = 0; i < 3; i++) {
		st_thr_data[i] = (int16_t)(reg_value[i]-128)*32;
		if (st_thr_data[i] < 0) {
			st_thr_data[i] = -st_thr_data[i];
		}
		st_thr_new[i] = st_thr_data[i] - (st_thr_data[i]/5);

		st_thd[i] = st_thr_new[i]/8;
		if (st_thd[i] > 255) {
			st_thd_reg[i] = 0xFF;
		} else {
			st_thd_reg[i] = (uint8_t)st_thd[i];
		}
	}
	/* Write threshold into the reg 0x1E-0x20 */
	buffer[0] = MMC5603X_REG_X_THD;
	buffer[1] = st_thd_reg[0];
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_X_THD, __LINE__);
	}

	buffer[0] = MMC5603X_REG_Y_THD;
	buffer[1] = st_thd_reg[1];
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_Y_THD, __LINE__);
	}

	buffer[0] = MMC5603X_REG_Z_THD;
	buffer[1] = st_thd_reg[2];
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_Z_THD, __LINE__);
	}

	return;
}

void MMC5603X_Continuous_Mode_With_Auto_SR(struct MMC5603X_data *dev, uint8_t bandwith, uint8_t sampling_rate)
{
	unsigned char buffer[2];

	/* Write reg 0x1C, Set BW<1:0> = bandwith */
	buffer[0] = MMC5603X_REG_CTRL1;
	buffer[1] = bandwith;
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL1, __LINE__);
	}

	/* Write reg 0x1A, set ODR<7:0> = sampling_rate */
	buffer[0] = MMC5603X_REG_ODR;
	buffer[1] = sampling_rate;
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_ODR, __LINE__);
	}

	/* Write reg 0x1B */
	/* Set Auto_SR_en bit '1', Enable the function of automatic set/reset */
	/* Set Cmm_freq_en bit '1', Start the calculation of the measurement period according to the ODR*/
	buffer[0] = MMC5603X_REG_CTRL0;
	buffer[1] = MMC5603X_CMD_CMM_FREQ_EN|MMC5603X_CMD_AUTO_SR_EN;
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL0, __LINE__);
	}

	/* Write reg 0x1D */
	/* Set Cmm_en bit '1', Enter continuous mode */
	buffer[0] = MMC5603X_REG_CTRL2;
	buffer[1] = MMC5603X_CMD_CMM_EN;
	if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
		dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL2, __LINE__);
	}

	return;
}

static int MMC5603X_set_poll_delay(struct MMC5603X_data *dev, unsigned int delay_msec)
{
	ktime_t poll_delay;
	int sampling_rate;

	if (atomic_read(&dev->enabled)) {
		if (dev->poll_interval > 0) {
			poll_delay = ktime_set(0, dev->poll_interval * NSEC_PER_MSEC);
			sampling_rate = (1000/(dev->poll_interval + 1)) + 10;
		} else {
			poll_delay = ktime_set(0, POLL_INTERVAL_DEFAULT * NSEC_PER_MSEC);
			sampling_rate = (1000/POLL_INTERVAL_DEFAULT) + 10;
		}

		MMC5603X_Continuous_Mode_With_Auto_SR(dev, MMC5603X_CMD_BW01, sampling_rate);
		hrtimer_start(&dev->work_timer, poll_delay, HRTIMER_MODE_REL);
	}

	return 0;
}

static int MMC5603X_set_enable(struct MMC5603X_data *dev, unsigned int enable)
{
	unsigned char buffer[2];
	ktime_t poll_delay;
	int sampling_rate;
	if (!dev) {
		printk("dev pointer is NULL\n");
		return -1;
	}

	if (enable) {
	    if (atomic_read(&dev->enabled)) {
			return 0;
	    }

		dev->hrtimer_running = true;
		if (dev->poll_interval > 0) {
			poll_delay = ktime_set(0, dev->poll_interval * NSEC_PER_MSEC);
			sampling_rate = (1000/(dev->poll_interval + 1)) + 10;
		} else {
			poll_delay = ktime_set(0, POLL_INTERVAL_DEFAULT * NSEC_PER_MSEC);
			sampling_rate = (1000/POLL_INTERVAL_DEFAULT) + 10;
		}

		MMC5603X_Auto_SelfTest_Configuration(dev);
		MMC5603X_Continuous_Mode_With_Auto_SR(dev, MMC5603X_CMD_BW01, sampling_rate);

		hrtimer_start(&dev->work_timer, poll_delay, HRTIMER_MODE_REL);
		atomic_set(&dev->enabled, 1);
	} else {
		if (atomic_read(&dev->enabled)) {
			buffer[0] = MMC5603X_REG_CTRL2;
			buffer[1] = 0x00;
			if (MMC5603X_i2c_txdata(dev->i2c, buffer, 2)) {
				dev_warn(&dev->i2c->dev, "write reg 0x%x failed at %d\n", MMC5603X_REG_CTRL2, __LINE__);
			}
			if (dev->hrtimer_running) {
				dev->hrtimer_running = false;
				hrtimer_cancel(&dev->work_timer);
				cancel_work_sync(&dev->work);
				flush_workqueue(dev->wq);
			}
			atomic_set(&dev->enabled, 0);
		}
	}
	dev->sensor_state = 1;

	return 0;
}
static ssize_t MMC5603X_chip_info(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%s\n", "mmc5603x chip");
}

static ssize_t MMC5603X_layout_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

static ssize_t MMC5603X_layout_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	  return 0;
}

static ssize_t MMC5603X_value_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	int vec[3];
	struct MMC5603X_data *dev_priv = dev_get_drvdata(dev);

	ret = MMC5603X_read_xyz(dev_priv, vec);
	if (ret) {
		dev_warn(&dev_priv->i2c->dev, "read xyz failed\n");
	}

	return sprintf(buf, "%d %d %d\n", vec[0], vec[1], vec[2]);
}

static ssize_t MMC5603X_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct MMC5603X_data *clientdata = i2c_get_clientdata(this_client);
	unsigned long data;

	data = simple_strtoul(buf, NULL, 10);

	if (data > MMC5603X_MAX_DELAY) {
		data = MMC5603X_MAX_DELAY;
	}
	if (data < MMC5603X_MIN_DELAY) {
		data = MMC5603X_MIN_DELAY;
	}

	clientdata->poll_interval = data - 1;
	MMC5603X_set_poll_delay(clientdata, clientdata->poll_interval);

	return count;
}

static ssize_t MMC5603X_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct MMC5603X_data *clientdata = i2c_get_clientdata(this_client);
	return sprintf(buf, "%d\n", clientdata->poll_interval + 1);
}

static ssize_t MMC5603X_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct MMC5603X_data *clientdata = i2c_get_clientdata(this_client);
	unsigned long data;

	data = simple_strtoul(buf, NULL, 10);

	if (data == 1) {
		MMC5603X_set_enable(clientdata, 1);
	} else if (data == 0) {
		MMC5603X_set_enable(clientdata, 0);
	}
	return count;
}

static ssize_t MMC5603X_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct MMC5603X_data *clientdata = i2c_get_clientdata(this_client);
	int enable = atomic_read(&clientdata->enabled);

	return sprintf(buf, "%d\n", enable);
}

static DEVICE_ATTR(chipinfo, S_IRUSR|S_IRGRP, MMC5603X_chip_info, NULL);
static DEVICE_ATTR(layout, S_IRUSR|S_IRGRP|S_IWUSR, MMC5603X_layout_show, MMC5603X_layout_store);
static DEVICE_ATTR(value, S_IRUSR|S_IRGRP, MMC5603X_value_show, NULL);
static DEVICE_ATTR(delay, 0644, MMC5603X_delay_show, MMC5603X_delay_store);
static DEVICE_ATTR(enable, 0644, MMC5603X_enable_show, MMC5603X_enable_store);

static struct attribute *MMC5603X_attributes[] = {
	&dev_attr_chipinfo.attr,
	&dev_attr_layout.attr,
	&dev_attr_value.attr,
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	NULL,
};

static const struct attribute_group MMC5603X_attr_group = {
		.attrs = MMC5603X_attributes,
};
#if MISC_DEVICE_REGISTER
static int MMC5603X_open(struct inode *inode, struct file *file)
{
	int ret = -1;
	ret = nonseekable_open(inode, file);

	return ret;
}

static int MMC5603X_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long MMC5603X_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;

	int delay;				/* for GET_DELAY */
	int set_delay;
	struct MMC5603X_data *clientdata = i2c_get_clientdata(this_client);
	uint32_t enable;

		switch (cmd) {
		case MMC5603X_IOC_SET_DELAY:
			if (copy_from_user(&set_delay, argp, sizeof(set_delay))) {
				printk("dev add set delay failed\n");
				return -EFAULT;
			}

			if (set_delay < MMC5603X_MIN_DELAY) {
				set_delay = MMC5603X_MIN_DELAY;
			}

			printk("%s %s %d xiexie\n", __func__, "MMC5603X_IOC_SET_DELAY", set_delay);
			clientdata->poll_interval = set_delay;
			MMC5603X_set_poll_delay(clientdata, clientdata->poll_interval);
			break;

		case ECOMPASS_IOC_GET_DELAY:
			delay = clientdata->poll_interval;
			if (copy_to_user(argp, &delay, sizeof(delay))) {
				printk(KERN_ERR "copy_to_user failed.");
				return -EFAULT;
			}
			break;

		case MSENSOR_IOCTL_MSENSOR_ENABLE:
			if (copy_from_user(&enable, argp, sizeof(enable))) {
				return -EFAULT;
			}

			printk("MSENSOR_ENABLE  %s enable = %d xiexie \n", __FUNCTION__, enable);
			MMC5603X_set_enable(clientdata, enable);
			break;

		case MSENSOR_IOCTL_OSENSOR_ENABLE:
			if (copy_from_user(&enable, argp, sizeof(enable))) {
				return -EFAULT;
			}

			printk("OSENSOR_ENABLE  %s enable = %d xiexie \n", __FUNCTION__, enable);
			MMC5603X_set_enable(clientdata, enable);
			break;

		default:
			printk(KERN_ERR "%s not supported = 0x%04x", __FUNCTION__, cmd);
			break;
	}

	return 0;
}

static struct file_operations MMC5603X_fops = {
	.owner = THIS_MODULE,
	.open = MMC5603X_open,
	.release = MMC5603X_release,
	.unlocked_ioctl = MMC5603X_unlocked_ioctl,
};

static struct miscdevice MMC5603X_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "MMC5603X",
    .fops = &MMC5603X_fops,
};
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int MMC5603X_probe(struct i2c_client *client)
#else
static int MMC5603X_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
	int res = 0;
	struct MMC5603X_data *dev;
#if MISC_DEVICE_REGISTER
	int err = 0;
#endif
	printk("dev probing MMC5603X\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("MMC5603X i2c functionality check failed.\n");
		res = -ENODEV;
		goto out;
	}

	dev = devm_kzalloc(&client->dev, sizeof(struct MMC5603X_data),
			GFP_KERNEL);
	if (!dev) {
		dev_err(&client->dev, "memory allocation failed.\n");
		res = -ENOMEM;
		goto out;
	}

	dev->poll_interval = MMC5603X_DEFAULT_INTERVAL_MS;
	atomic_set(&dev->enabled, 0);
	dev->on_before_suspend = 0;

	this_client = client;
	dev->i2c = client;
	dev_set_drvdata(&client->dev, dev);
	i2c_set_clientdata(dev->i2c, dev);
	this_client = client;

	mutex_init(&dev->lock);

	res = MMC5603X_check_device(dev);
	if (res) {
		dev_err(&client->dev, "Check device failed\n");
		goto out;
	}

	dev->idev = MMC5603X_init_input(dev);
	if (!dev->idev) {
		dev_err(&client->dev, "init input device failed\n");
		res = -ENODEV;
		goto out;
	}
	res = sysfs_create_group(&dev->idev->dev.kobj, &MMC5603X_attr_group);
	if (res) {
		dev_err(&client->dev, "Unable to creat sysfs group\n");
		goto exit_misc_device_register_failed;
	}

	kobject_uevent(&dev->idev->dev.kobj, KOBJ_CHANGE);
#if MISC_DEVICE_REGISTER
	err = misc_register(&MMC5603X_device);
	if (err) {
		printk(KERN_ERR "MMC5603X misc device register failed\n");
	}
	/* create sysfs group */
	res = sysfs_create_group(&client->dev.kobj, &MMC5603X_attr_group);
	if (res) {
		res = -EROFS;
		dev_err(&client->dev, "Unable to creat sysfs group\n");
	}
#endif
	printk("MMC5603X successfully probed\n");

	return 0;

exit_misc_device_register_failed:
	input_unregister_device(dev->idev);
	input_free_device(dev->idev);
out:
	return res;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void MMC5603X_remove(struct i2c_client *client)
#else
static int MMC5603X_remove(struct i2c_client *client)
#endif
{
	struct MMC5603X_data *dev_priv = dev_get_drvdata(&client->dev);

	printk("dev remove MMC5603X\n");
	MMC5603X_set_enable(dev_priv, false);

	if (dev_priv->idev) {
		destroy_workqueue(dev_priv->wq);
		sysfs_remove_group(&dev_priv->idev->dev.kobj, &MMC5603X_attr_group);
		input_unregister_device(dev_priv->idev);
	}

#if MISC_DEVICE_REGISTER
	sysfs_remove_group(&client->dev.kobj, &MMC5603X_attr_group);
	misc_deregister(&MMC5603X_device);
#endif
	input_sensor_free(&(mag_config.input_type));
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	return;
#else
	return 0;
#endif
}

static int MMC5603X_suspend(struct device *dev)
{
	int res = 0;
	struct MMC5603X_data *dev_priv = dev_get_drvdata(dev);
	dev_dbg(dev, "suspended\n");
	input_set_power_enable(&(mag_config.input_type), 0);
	dev_priv->on_before_suspend = atomic_read(&dev_priv->enabled);
	if (dev_priv->on_before_suspend) {
		return MMC5603X_set_enable(dev_priv, false);
	}

	return res;
}

static int MMC5603X_resume(struct device *dev)
{
	int res = 0;
	struct MMC5603X_data *dev_priv = dev_get_drvdata(dev);

	dev_dbg(dev, "resumed\n");

	input_set_power_enable(&(mag_config.input_type), 1);
	if (dev_priv->on_before_suspend) {
		return MMC5603X_set_enable(dev_priv, true);
	}

	return res;
}

static const struct i2c_device_id MMC5603X_id[] = {
	{ MMC5603X_I2C_NAME, 0 },
	{ }
};

static struct of_device_id MMC5603X_match_table[] = {
	{ .compatible = "memsic,mmc5603x", },
	{ },
};

static const struct dev_pm_ops MMC5603X_pm_ops = {
	.suspend = MMC5603X_suspend,
	.resume = MMC5603X_resume,
};

static struct i2c_driver MMC5603X_driver = {
	.probe 		= MMC5603X_probe,
	.remove 	= MMC5603X_remove,
	.id_table	= MMC5603X_id,
	.driver 	= {
		.owner	= THIS_MODULE,
		.name	= MMC5603X_I2C_NAME,
		.of_match_table = MMC5603X_match_table,
		.pm = &MMC5603X_pm_ops,
	},
};

static int startup(void)
{
	int ret = -1;

	printk("function=%s=========LINE=%d. \n", __func__, __LINE__);

	if (input_sensor_startup(&(mag_config.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else {
		ret = input_sensor_init(&(mag_config.input_type));
	}

	if (0 != ret) {
	    printk("%s:gsensor.init_platform_resource err. \n", __func__);
	}

	input_set_power_enable(&(mag_config.input_type), 1);
	return 0;
}

static int __init MMC5603X_init(void)
{
	if (startup() != 0)
		return -1;

	return i2c_add_driver(&MMC5603X_driver);
}

static void __exit MMC5603X_exit(void)
{
	i2c_del_driver(&MMC5603X_driver);
}

MODULE_DESCRIPTION("MEMSIC MMC5603X Magnetic Sensor Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

late_initcall(MMC5603X_init);
module_exit(MMC5603X_exit);

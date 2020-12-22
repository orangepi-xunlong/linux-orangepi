/* linux/driver/input/misc/ltr.c
 * Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/suspend.h>
#include <linux/init-input.h>
#include <mach/sys_config.h>
#ifdef CONFIG_SCENELOCK
#include <linux/power/scenelock.h>
#endif
#ifdef CONFIG_PM
#include <linux/pm.h>
#endif

#include <linux/gfp.h>

#include "ltr_501als.h"

/* Note about power vs enable/disable:
 *  The chip has two functions, proximity and ambient light sensing.
 *  There is no separate power enablement to the two functions (unlike
 *  the Capella CM3602/3623).
 *  This module implements two drivers: /dev/proximity and /dev/light.
 *  When either driver is enabled (via sysfs attributes), we give power
 *  to the chip.  When both are disabled, we remove power from the chip.
 *  In suspend, we remove power if light is disabled but not if proximity is
 *  enabled (proximity is allowed to wakeup from suspend).
 *
 *  There are no ioctls for either driver interfaces.  Output is via
 *  input device framework and control via sysfs attributes.
 */

#define PS_DISTANCE  100

enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

static const int chip_id_value[] = {0x05,0};

static struct sensor_config_info ls_sensor_info = {
	.input_type = LS_TYPE,
	.int_number = 0,
	.ldo = NULL,
	.dev = NULL,
};

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {LTR558_SLAVE_ADDR,I2C_CLIENT_END};
static int i2c_num = 0;
static const unsigned short i2c_address[] = {LTR558_SLAVE_ADDR, LTR558_SLAVE_ADDR};

static u32 debug_mask = 0;

/* driver data */
struct ltr_data {
	struct input_dev *proximity_input_dev;
	struct input_dev *light_input_dev;
	struct delayed_work ps_delay_work;
	struct work_struct irq_workqueue;
	struct i2c_client *i2c_client;
	int irq;
	struct work_struct work_light;
	struct hrtimer timer;
	ktime_t light_poll_delay;
	bool on;
	u8 power_state;
	unsigned long ps_poll_delay;
	struct mutex power_lock;
	struct wake_lock prx_wake_lock;
	struct workqueue_struct *wq;
	atomic_t ltr558_init;
	atomic_t ltr558_suspend;
};

static void ltr558_resume_events(struct work_struct *work);
static void ltr558_init_events(struct work_struct *work);
static struct workqueue_struct *ltr558_resume_wq;
static struct workqueue_struct *ltr558_init_wq;
static DECLARE_WORK(ltr558_resume_work, ltr558_resume_events);
static DECLARE_WORK(ltr558_init_work, ltr558_init_events);
static struct ltr_data *ltr558_data;

int ltr558_als_read(void)
{
	int div_tmp = 0;
	
	int alsval_ch0_lo = 0, alsval_ch0_hi = 0;
	int alsval_ch1_lo = 0, alsval_ch1_hi = 0;
	int luxdata_int = 0;
	int ratio = 0;
	int alsval_ch0 = 0, alsval_ch1 = 0;
	int ch0_coeff = 0, ch1_coeff = 0;

	alsval_ch0_lo = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_0);
	alsval_ch0_hi = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_1);
	alsval_ch1_lo = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH1_0);
	alsval_ch1_hi = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH1_1);
	
	alsval_ch0 = (alsval_ch0_hi << 8) + alsval_ch0_lo;
	alsval_ch1 = (alsval_ch1_hi << 8) + alsval_ch1_lo;
	dprintk(DEBUG_REPORT_ALS_DATA, "light alsval_ch1=%d alsval_ch0=%d\n",  alsval_ch1, alsval_ch0);

	div_tmp = ( (alsval_ch1 + alsval_ch0) != 0)?(alsval_ch1 + alsval_ch0):1;
	ratio = (100 * alsval_ch1)/div_tmp;
  
	if (ratio < 45)
	{
		ch0_coeff = 17743;
		ch1_coeff = -11059;
	}
	else if ((ratio >= 45) && (ratio < 64))
	{
		ch0_coeff = 37725;
		ch1_coeff = 13363;
	}
	else if ((ratio >= 64) && (ratio < 85))
	{
		ch0_coeff = 16900;
		ch1_coeff = 1690;
	}
	else if (ratio >= 85)
	{
		ch0_coeff = 0;
		ch1_coeff = 0;
	}

	luxdata_int = ((alsval_ch0 * ch0_coeff) - (alsval_ch1 * ch1_coeff))/10000;

  return luxdata_int;
}
int ltr558_ps_read(void)
{
	int psval_lo, psval_hi, psdata;

	psval_lo = ltr558_i2c_read_reg(LTR558_PS_DATA_0);
	if (psval_lo < 0){
		psdata = psval_lo;
		goto out;
	}

	psval_hi = ltr558_i2c_read_reg(LTR558_PS_DATA_1);
	if (psval_hi < 0){
		psdata = psval_hi;
		goto out;
	}
		
	psdata = ((psval_hi & 7) << 8) + psval_lo;
	dprintk(DEBUG_REPORT_PS_DATA, " ps val =%d\n", psdata);

	out:
	return psdata;
}
#if 0
int ltr558_i2c_read_reg(u8 regnum)
{
	info("xdafsdf\n");
}
#endif
struct ltr558_data {
	struct i2c_client *client;
};
static struct ltr558_data the_data;

// I2C Read
int ltr558_i2c_read_reg(unsigned char regnum)
{
	int readdata;

	/*
	 * i2c_smbus_read_byte_data - SMBus "read byte" protocol
	 * @client: Handle to slave device
	 * @command: Byte interpreted by slave
	 *
	 * This executes the SMBus "read byte" protocol, returning negative errno
	 * else a data byte received from the device.
	 */
	readdata = i2c_smbus_read_byte_data(the_data.client, regnum);
	return readdata;
}


// I2C Write
static int ltr558_i2c_write_reg(unsigned char regnum, unsigned char value)
{
	int writeerror;

	/*
	 * i2c_smbus_write_byte_data - SMBus "write byte" protocol
	 * @client: Handle to slave device
	 * @command: Byte interpreted by slave
	 * @value: Byte being written
	 *
	 * This executes the SMBus "write byte" protocol, returning negative errno
	 * else zero on success.
	 */

	writeerror = i2c_smbus_write_byte_data(the_data.client, regnum, value);

	if (writeerror < 0)
		return writeerror;
	else
		return 0;
}



static int ltr558_ps_enable(void)
{
	int error;
	int setgain;
	int gainrange = PS_RANGE8;

	switch (gainrange) {
		case PS_RANGE1:
			setgain = MODE_PS_ON_Gain1;
			break;

		case PS_RANGE4:
			setgain = MODE_PS_ON_Gain4;
			break;

		case PS_RANGE8:
			setgain = MODE_PS_ON_Gain8;
			break;

		case PS_RANGE16:
			setgain = MODE_PS_ON_Gain16;
			break;

		default:
			setgain = MODE_PS_ON_Gain1;
			break;
	}

	error = ltr558_i2c_write_reg(LTR558_PS_CONTR, setgain | (1<<1)); 
	msleep(WAKEUP_DELAY);

	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */

	return error;
}


// Put PS into Standby mode
static int ltr558_ps_disable(void)
{
	int error;
	error = ltr558_i2c_write_reg(LTR558_PS_CONTR, MODE_PS_StdBy); 
	return error;
}

static int ltr558_als_enable(void)
{
	int error;
	int gainrange = 2;//lkj Dynamic Range 2 (2 lux to 64k lux) 

	if (gainrange == 1)
		error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_ON_Range1);
	else if (gainrange == 2)
		error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_ON_Range2);
	else
		error = -1;

	msleep(WAKEUP_DELAY);

	/* =============== 
	 * ** IMPORTANT **
	 * ===============
	 * Other settings like timing and threshold to be set here, if required.
 	 * Not set and kept as device default for now.
 	 */

	return error;
}


// Put ALS into Standby mode
static int ltr558_als_disable(void)
{
	int error;
	error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_StdBy); 
	return error;
}


int ltr558_als_power(bool enable)
{
	int ret;

	if (enable)
		ret=ltr558_als_enable();
	else 
		ret=ltr558_als_disable();

	return ret;
}

int ltr558_ps_power(bool enable)
{
	int ret;

	if (enable)
		ret = ltr558_ps_enable();
	else
		ret = ltr558_ps_disable();

	return ret;
}


int ltr558_devinit(void)
{
	int error;

	msleep(PON_DELAY);

	mutex_lock(&ltr558_data->power_lock);
	error = ltr558_i2c_write_reg(LTR558_INTERRUPT, 0x1); // 0 active, ps interrupt, latched until read
	if (error < 0)
		goto out;
	// Enable PS to Gain1 at startup
	error = ltr558_ps_enable();
	if (error < 0)
		goto out;

	// Enable ALS to Full Range at startup
	error = ltr558_als_enable();
	if (error < 0)
		goto out;

	ltr558_i2c_write_reg(LTR558_PS_LED, 0x7f );
	ltr558_i2c_write_reg(LTR558_PS_N_PULSES, 0x08);
	
	ltr558_i2c_write_reg(LTR558_INTERRUPT_PERSIST, 0x70);
	/* 0 ~ 500 */
	ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_0, 0xf4);
	ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_1, 0x1);
	ltr558_i2c_write_reg(LTR558_PS_THRES_UP_0, 0xff);
	ltr558_i2c_write_reg(LTR558_PS_THRES_UP_1, 0x07);
	mutex_unlock(&ltr558_data->power_lock);

	error = 0;
        error = ltr558_ps_read();
	dprintk(DEBUG_INIT, "first read ps=%d\n", error);

#if 0
//printf reg
	unsigned char upper_low,upper_high;
	unsigned char lower_low,lower_high;

	upper_low = ltr558_i2c_read_reg(LTR558_PS_THRES_UP_0);
	upper_high = ltr558_i2c_read_reg(LTR558_PS_THRES_UP_1);

	lower_low = ltr558_i2c_read_reg(LTR558_ALS_THRES_LOW_0);
	lower_high = ltr558_i2c_read_reg(LTR558_ALS_THRES_LOW_1);

	printk(" read thres upper =%d\n", ((upper_high & 0x7) << 7) |  upper_low );
	printk(" read thres lower =%d\n", ((lower_high & 0x7) << 7) |  lower_low );


//	error = ltr558_i2c_write_reg(LTR558_PS_LED, 0x6b & 0xf8);���б仯��

	error = ltr558_i2c_read_reg(LTR558_PS_CONTR);
	printk(" LTR558_PS_CONTR =0x%0x\n", error );

	error = ltr558_i2c_read_reg(LTR558_PS_LED);
	printk(" LTR558_PS_LED =0x%0x\n", error );

	error = ltr558_i2c_read_reg(LTR558_PS_N_PULSES);
	printk(" LTR558_PS_N_PULSES =0x%0x\n", error );

	error = ltr558_i2c_read_reg(LTR558_PS_MEAS_RATE);
	printk(" LTR558_PS_MEAS_RATE =0x%0x\n", error );

	error = ltr558_i2c_read_reg(LTR558_INTERRUPT);
	printk(" LTR558_INTERRUPT =0x%0x\n", error );

	error = ltr558_i2c_read_reg(LTR558_INTERRUPT_PERSIST);
	printk(" LTR558_INTERRUPT_PERSIST =0x%0x\n", error );

#endif


	out:
	return error;
}

void ltr558_set_client(struct i2c_client *client)
{
	the_data.client = client;
}


static void ltr_ps_enable(struct ltr_data *ltr)
{
	dprintk(DEBUG_CONTROL_INFO, "starting ps work");
	queue_delayed_work(ltr->wq, &ltr->ps_delay_work, 0); 
}

static void ltr_ps_disable(struct ltr_data *ltr)
{
	dprintk(DEBUG_CONTROL_INFO, "cancelling ps work\n");
	cancel_delayed_work_sync(&ltr->ps_delay_work);
}

static void ltr_light_enable(struct ltr_data *ltr)
{
	dprintk(DEBUG_CONTROL_INFO, "starting poll timer, delay %lldns\n", ktime_to_ns(ltr->light_poll_delay));
	hrtimer_start(&ltr->timer, ltr->light_poll_delay, HRTIMER_MODE_REL);
}

static void ltr_light_disable(struct ltr_data *ltr)
{
	dprintk(DEBUG_CONTROL_INFO, "cancelling poll timer\n");
	hrtimer_cancel(&ltr->timer);
	cancel_work_sync(&ltr->work_light);
}

static ssize_t ls_poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct ltr_data *ltr = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(ltr->light_poll_delay));
}


static ssize_t ls_poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct ltr_data *ltr = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	dprintk(DEBUG_CONTROL_INFO, "ls new delay = %lldns, old delay = %lldns\n",
		    new_delay, ktime_to_ns(ltr->light_poll_delay));
	mutex_lock(&ltr->power_lock);
	if (new_delay != ktime_to_ns(ltr->light_poll_delay)) {
		ltr->light_poll_delay = ns_to_ktime(new_delay);
		if (ltr->power_state & LIGHT_ENABLED) {
			ltr_light_disable(ltr);
			ltr_light_enable(ltr);
		}
	}
	mutex_unlock(&ltr->power_lock);

	return size;
}

static ssize_t ps_poll_delay_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct ltr_data *ltr = dev_get_drvdata(dev);
	return sprintf(buf, "%ld\n", ltr->ps_poll_delay);
}


static ssize_t ps_poll_delay_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct ltr_data *ltr = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	dprintk(DEBUG_CONTROL_INFO, "ps new delay = %lldms, old delay = %ldms\n",
		    new_delay,ltr->ps_poll_delay);
	
	ltr->ps_poll_delay = new_delay;

	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct ltr_data *ltr = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (ltr->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static ssize_t proximity_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct ltr_data *ltr = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (ltr->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static ssize_t light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct ltr_data *ltr = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		printk("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if ((atomic_read(&ltr->ltr558_init) == 0) || (atomic_read(&ltr->ltr558_suspend) == 1)) {
		mutex_lock(&ltr->power_lock);
		ltr->power_state |= LIGHT_ENABLED;
		mutex_unlock(&ltr->power_lock);
		return size;
	}

	dprintk(DEBUG_CONTROL_INFO, "new_value = %d, old state = %d\n",
		    new_value, (ltr->power_state & LIGHT_ENABLED) ? 1 : 0);
	
	mutex_lock(&ltr->power_lock);
	if (new_value && !(ltr->power_state & LIGHT_ENABLED)) {
		ltr558_als_power(true);
		ltr->power_state |= LIGHT_ENABLED;
		ltr_light_enable(ltr);
	} else if (!new_value && (ltr->power_state & LIGHT_ENABLED)) {
		ltr_light_disable(ltr);
		ltr->power_state &= ~LIGHT_ENABLED;
		ltr558_als_power(false);
	}
	mutex_unlock(&ltr->power_lock);

	return size;
}

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct ltr_data *ltr = dev_get_drvdata(dev);
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		printk("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if ((atomic_read(&ltr->ltr558_init) == 0) || (atomic_read(&ltr->ltr558_suspend) == 1)) {
		mutex_lock(&ltr->power_lock);
		ltr->power_state |= PROXIMITY_ENABLED;
		mutex_unlock(&ltr->power_lock);
		return size;
	}

	mutex_lock(&ltr->power_lock);
	dprintk(DEBUG_CONTROL_INFO, "new_value = %d, old state = %d\n",  new_value, (ltr->power_state & PROXIMITY_ENABLED) ? 1 : 0);
	if (new_value && !(ltr->power_state & PROXIMITY_ENABLED)) {
		ltr558_ps_power(true);
		ltr->power_state |= PROXIMITY_ENABLED;
		ltr_ps_enable(ltr);
	} else if (!new_value && (ltr->power_state & PROXIMITY_ENABLED)) {
		ltr_ps_disable(ltr);
		ltr558_ps_power(false);
		ltr->power_state &= ~PROXIMITY_ENABLED;
	}
	mutex_unlock(&ltr->power_lock);
	return size;
}

static DEVICE_ATTR(ls_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   ls_poll_delay_show, ls_poll_delay_store);

static DEVICE_ATTR(ps_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   ps_poll_delay_show, ps_poll_delay_store);

static struct device_attribute dev_attr_light_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       light_enable_show, light_enable_store);

static struct device_attribute dev_attr_proximity_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       proximity_enable_show, proximity_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_ls_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	&dev_attr_ps_poll_delay.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

static void ltr_work_func_light(struct work_struct *work)
{
	struct ltr_data *ltr = container_of(work, struct ltr_data,
			work_light);

	int adc = ltr558_als_read();
	if (adc < 0)
	{
		printk("light val err");
		adc = 0; // no light
	}

	dprintk(DEBUG_REPORT_ALS_DATA, "light val=%d\n",  adc);
	input_report_abs(ltr->light_input_dev, ABS_MISC, adc);
	input_sync(ltr->light_input_dev);
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart ltr_timer_func(struct hrtimer *timer)
{
	struct ltr_data *ltr = container_of(timer, struct ltr_data, timer);
	queue_work(ltr->wq, &ltr->work_light);
	hrtimer_forward_now(&ltr->timer, ltr->light_poll_delay);
	return HRTIMER_RESTART;
}

#if 0
/* interrupt happened due to transition/change of near/far proximity state */
static irqreturn_t ltr_irq_handler(int irq, void *dev_id)
{
//	struct ltr_data *ltr =(struct ltr_data *)data;
//	schedule_work(&ltr->irq_workqueue);
	dprintk(DEBUG_CONTROL_INFO, "in irq\n");
	return 0;
}
#endif

static void ltr558_schedwork(struct work_struct *work)
{
	struct ltr_data *ltr = container_of((struct delayed_work *)work, struct ltr_data,ps_delay_work);
	int val=-1;
#if 0
	int als_ps_status;
	int interrupt, newdata, val=-1;

	als_ps_status = ltr558_i2c_read_reg(LTR558_ALS_PS_STATUS);

	interrupt = als_ps_status & 10;
	newdata = als_ps_status & 5;

	switch (interrupt){
		case 2:
			// PS interrupt
			if ((newdata == 1) | (newdata == 5)){
				val = ltr558_ps_read();
			}
			break;

		case 8:
			info("!!!!!!!!!impossible irq als insterrupt!!!!!!!!");
			// ALS interrupt
			if ((newdata == 4) | (newdata == 5)){
				; //als_data_changed = 1;
			}
			break;

		case 10:
			info("!!!!!!!!!impossible irq ps and als insterrupt!!!!!!!!");
			// Both interrupt
			if ((newdata == 1) | (newdata == 5)){
				val = ltr558_ps_read();
				; //ps_data_changed = 1;
			}
			if ((newdata == 4) | (newdata == 5)){
				;//als_data_changed = 1;
			}
			break;
	}
#endif
	val = ltr558_ps_read();
	dprintk(DEBUG_REPORT_PS_DATA, " ps val =%d\n", val);
	
	/* 0 is close, 1 is far */
	val = val >= PS_DISTANCE ? 0:1;

	input_report_abs(ltr->proximity_input_dev, ABS_DISTANCE, val);
	input_sync(ltr->proximity_input_dev);

	queue_delayed_work(ltr->wq, &ltr->ps_delay_work, msecs_to_jiffies(ltr->ps_poll_delay));
	//wake_lock_timeout(&ip->prx_wake_lock, 3*HZ);
}

#if 0
static int ltr_setup_irq(struct ltr_data *ltr)
{
	int ret = -EIO;

	dprintk(DEBUG_INT, "%s:light sensor irq_number= %d\n", __func__,
			ls_sensor_info.int_number);

	ls_sensor_info.dev = &(ltr->proximity_input_dev->dev);
	if (0 != ls_sensor_info.int_number) {
		ret = input_request_int(&(ls_sensor_info.input_type), ltr_irq_handler,
				IRQF_TRIGGER_FALLING, ltr);
		if (ret) {
			printk("Failed to request gpio irq \n");
			return ret;
		}
	}

	ret = 0;
	return ret;
}
#endif

static void ltr558_init_events (struct work_struct *work)
{
	ltr558_devinit();

	mutex_lock(&ltr558_data->power_lock);
	if (ltr558_data->power_state &= LIGHT_ENABLED) {
		ltr558_als_power(true);
		ltr_light_enable(ltr558_data);
	}

	if (ltr558_data->power_state &= PROXIMITY_ENABLED) {
		ltr558_ps_power(true);
		ltr_ps_enable(ltr558_data);
	}
	mutex_unlock(&ltr558_data->power_lock);

	atomic_set(&ltr558_data->ltr558_init, 1);
}

static int ltr_i2c_probe(struct i2c_client *client,
			  const struct i2c_device_id *id)
{
	struct input_dev *input_dev;
	struct ltr_data *ltr;
	int ret = -ENODEV;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_WRITE_BYTE | I2C_FUNC_SMBUS_READ_BYTE_DATA))
	{
		printk("%s,LTR-558ALS functionality check failed.\n", __func__);
		return -EIO;
	}

	ltr = kzalloc(sizeof(struct ltr_data), GFP_KERNEL);
	if (!ltr) {
		printk("%s: failed to alloc memory for module data\n", __func__);
		return -ENOMEM;
	}
	
	ltr->i2c_client = client;
	i2c_set_clientdata(client, ltr);
	ltr->ps_poll_delay = 100;

	/* the timer just fires off a work queue request.  we need a thread
	to read the i2c (can be slow and blocking). */
	ltr->wq = create_singlethread_workqueue("ltr_wq");
	if (!ltr->wq) {
		ret = -ENOMEM;
		printk("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}
	
	/* ==================proximity  sensor====================== */
	mutex_init(&ltr->power_lock);
	INIT_DELAYED_WORK(&ltr->ps_delay_work, ltr558_schedwork);

	/* allocate proximity input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		printk("%s: could not allocate input device\n", __func__);
		goto err_input_allocate_device_proximity;
	}
	ltr->proximity_input_dev = input_dev;
	input_set_drvdata(input_dev, ltr);
	input_dev->name = "proximity";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	/* 0,close ,1 far */ 
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	printk("registering proximity input device\n");
	ret = input_register_device(input_dev);
	if (ret < 0) {
		printk("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_proximity;
	}
	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (ret) {
		printk("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_proximity;
	}



	/* ==================light sensor====================== */
	/* hrtimer settings.  we poll for light values using a timer. */
	hrtimer_init(&ltr->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ltr->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	ltr->timer.function = ltr_timer_func;

	/* this is the thread function we run on the work queue */
	INIT_WORK(&ltr->work_light, ltr_work_func_light);

	/* allocate lightsensor-level input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		printk("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_light;
	}
	input_set_drvdata(input_dev, ltr);
	input_dev->name = "lightsensor";
	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	//max 16bit 
	input_set_abs_params(input_dev, ABS_MISC, 0, 65535, 0, 0);

	dprintk(DEBUG_INIT, "registering lightsensor-level input device\n");
	ret = input_register_device(input_dev);
	if (ret < 0) {
		printk("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}
	ltr->light_input_dev = input_dev;
	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &light_attribute_group);
	if (ret) {
		printk("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	ltr558_set_client(ltr->i2c_client);

	atomic_set(&ltr->ltr558_init, 0);
	atomic_set(&ltr->ltr558_suspend, 0);
	ltr558_data = ltr;
	ltr558_resume_wq = create_singlethread_workqueue("ltr558_resume");
	if (ltr558_resume_wq == NULL) {
		printk("create ltr558_resume_wq fail!\n");
		return -ENOMEM;
	}

	ltr558_init_wq = create_singlethread_workqueue("ltr558_init");
	if (ltr558_init_wq == NULL) {
		printk("create ltr558_init_wq fail!\n");
		return -ENOMEM;
	}

	/* Initialize the ltr558 chip */
	queue_work(ltr558_init_wq, &ltr558_init_work);
//	ret = ltr_setup_irq(ltr);
//	if (ret) {
//		printk("%s: could not setup irq\n", __func__);
//		goto err_sysfs_create_group_light;
//	}

	dprintk(DEBUG_INIT, "LTR probe OK!");
	return 0;

	/* error, unwind it all */
err_sysfs_create_group_light:
	input_unregister_device(ltr->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	sysfs_remove_group(&ltr->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	input_unregister_device(ltr->proximity_input_dev);
err_input_register_device_proximity:
err_input_allocate_device_proximity:
	mutex_destroy(&ltr->power_lock);
	destroy_workqueue(ltr->wq);
err_create_workqueue:
	kfree(ltr);

	return ret;
}

static int ltr_i2c_remove(struct i2c_client *client)
{
	struct ltr_data *ltr = i2c_get_clientdata(client);

	cancel_work_sync(&ltr558_init_work);
	destroy_workqueue(ltr558_init_wq);
	cancel_work_sync(&ltr558_resume_work);
	destroy_workqueue(ltr558_resume_wq);
	sysfs_remove_group(&ltr->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(ltr->light_input_dev);
	sysfs_remove_group(&ltr->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
	input_unregister_device(ltr->proximity_input_dev);
	//if (0 != ls_sensor_info.int_number)
		//input_free_int(&(ls_sensor_info.input_type), ltr);
	if (ltr->power_state) {
		if (ltr->power_state & LIGHT_ENABLED)
			ltr_light_disable(ltr);
		ltr558_als_power(false);
		ltr558_ps_power(false);
		ltr->power_state = 0;
	}
	cancel_delayed_work_sync(&ltr->ps_delay_work);
	destroy_workqueue(ltr->wq);
	mutex_destroy(&ltr->power_lock);
	kfree(ltr);
	return 0;
}

static void ltr558_resume_events (struct work_struct *work)
{
	ltr558_devinit();

	mutex_lock(&ltr558_data->power_lock);	
	if (ltr558_data->power_state & LIGHT_ENABLED) {
		ltr558_als_power(true);
		ltr_light_enable(ltr558_data);
	}
	if (ltr558_data->power_state & PROXIMITY_ENABLED) {
		ltr558_ps_power(true);
		ltr_ps_enable(ltr558_data);
	}
	mutex_unlock(&ltr558_data->power_lock);

	atomic_set(&ltr558_data->ltr558_suspend, 0);
}

#ifdef CONFIG_PM
static int ltr_suspend(struct i2c_client *client, pm_message_t mesg)
{
        /* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   ltr->power_state because we use that state in resume.
	*/
	struct ltr_data *ltr = i2c_get_clientdata(client);

	dprintk(DEBUG_SUSPEND, "==suspend=\n");

	atomic_set(&ltr->ltr558_suspend, 1);

	if (ltr->power_state & LIGHT_ENABLED){
		ltr_light_disable(ltr);
		ltr558_als_power(false);
	}

	if (ltr->power_state & PROXIMITY_ENABLED){
		ltr_ps_disable(ltr);
#ifdef CONFIG_SCENELOCK
		if (check_scene_locked(SCENE_TALKING_STANDBY) != 0)
#endif
		ltr558_ps_power(false);
	}

	return 0;
}
static int ltr_resume(struct i2c_client *client)
{
	/* Turn power back on if we were before suspend. */
	struct ltr_data *ltr = i2c_get_clientdata(client);

	dprintk(DEBUG_SUSPEND, "==resume=\n");
	
	if (NORMAL_STANDBY == standby_type) {
		if (ltr->power_state & LIGHT_ENABLED){
			ltr558_als_power(true);
			ltr_light_enable(ltr);
		}

		if (ltr->power_state & PROXIMITY_ENABLED){
#ifdef CONFIG_SCENELOCK
			if (check_scene_locked(SCENE_TALKING_STANDBY) != 0)
#endif
			ltr558_ps_power(true);
			ltr_ps_enable(ltr);
		}
		atomic_set(&ltr->ltr558_suspend, 0);
	} else if (SUPER_STANDBY == standby_type)
		queue_work(ltr558_resume_wq, &ltr558_resume_work);

	return 0;
		
}
#endif

/**
 * ls_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int ls_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
            return -ENODEV;
            
	if (ls_sensor_info.twi_id == adapter->nr) {
		for (i2c_num = 0; i2c_num < (sizeof(i2c_address)/sizeof(i2c_address[0]));i2c_num++) {	    
			client->addr = i2c_address[i2c_num];
			dprintk(DEBUG_INIT, "%s:addr= 0x%x,i2c_num:%d\n",__func__,client->addr,i2c_num);
			ret = i2c_smbus_read_byte_data(client,LTR558_MANUFACTURER_ID);
			dprintk(DEBUG_INIT, "Read MID value is :0x%x\n",ret);
			if ((ret &0x00FF) == LTR558_MID) {
				ret = i2c_smbus_read_byte_data(client,LTR558_REG_PORT_ID);
				dprintk(DEBUG_INIT, "Read PID value is :0x%x\n",ret);
				if ((ret &0x00FF) == LTR558_PID) {
					dprintk(DEBUG_INIT, "LS Device detected!\n" );
					strlcpy(info->type, LTR558_NAME, I2C_NAME_SIZE);
					return 0;
				}
			}                                                           
		}
        
		pr_info("%s:LS Device not found, \
		maybe the other gsensor equipment! \n",__func__);
		return -ENODEV;
	} else {
		return -ENODEV;
	}
}

static const struct i2c_device_id ltr_device_id[] = {
	{LTR558_NAME, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ltr_device_id);

static struct i2c_driver ltr_i2c_driver = {
	.class = I2C_CLASS_HWMON,
	.probe		= ltr_i2c_probe,
	.remove		= __devexit_p(ltr_i2c_remove),
	.id_table	= ltr_device_id,
	.detect		= ls_detect,
	.address_list   = normal_i2c,
#ifdef CONFIG_PM
	.suspend  	= ltr_suspend,
	.resume  	= ltr_resume,
#endif
	.driver 	= {
		.name = LTR558_NAME,
		.owner = THIS_MODULE,
	 },
};

static int ltr_init(void)
{
	int ret = 0;
	dprintk(DEBUG_INIT, "%s:light sensor driver init\n", __func__ );

	if (input_fetch_sysconfig_para(&(ls_sensor_info.input_type))) {
		printk("%s: ls_fetch_sysconfig_para err.\n", __func__);
		return 0;
	} else {
		ret = input_init_platform_resource(&(ls_sensor_info.input_type));
		if (0 != ret) {
			printk("%s:ls_init_platform_resource err. \n", __func__);    
		}
	}

	if (ls_sensor_info.sensor_used == 0) {
		printk("*** ls_used set to 0 !\n");
		printk("*** if use light_sensor,please put the sys_config.fex ls_used set to 1. \n");
		return 0;
	}
	return i2c_add_driver(&ltr_i2c_driver);
}

static void ltr_exit(void)
{
	i2c_del_driver(&ltr_i2c_driver);

	input_free_platform_resource(&(ls_sensor_info.input_type));
}

module_init(ltr_init);
module_exit(ltr_exit);
module_param_named(debug_mask, debug_mask, int, 0644);

MODULE_AUTHOR("nsdfsdf@sta.samsung.com");
MODULE_DESCRIPTION("Optical Sensor driver for ltrp002a00f");
MODULE_LICENSE("GPL");


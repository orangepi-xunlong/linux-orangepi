/*
 *  kionix_kj9.c - Linux kernel modules for 3-Axis Orientation/Motion
 *  Detection Sensor 
 *
 *  Copyright (C) 2009-2010 Freescale Semiconductor Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/device.h>
#include <linux/init-input.h>

//#include <mach/system.h>
#include <mach/hardware.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif

/*
 * Defines
 */
#define assert(expr)\
	if (!(expr)) {\
		printk(KERN_ERR "Assertion failed! %s,%d,%s,%s\n",\
			__FILE__, __LINE__, __func__, #expr);\
	}

#define KXTF9_I2C_SLAVE_ADDR		0x0F



/* KXTF9 Internal Command  (Please refer to KXTF9 Specifications) */

#define KXTF9_REG_XOUT_L			0x06

#define KXTF9_REG_CTRL_REG1			0x1B

#define KXTF9_REG_CTRL_REG2			0x1C

#define KXTF9_REG_CTRL_REG3			0x1D

#define KXTF9_REG_DATA_CTRL			0x21



#define KXTF9_OPERATION				0x80

#define KXTF9_12BIT					0x40

#define KXTF9_DATA_ODR_400HZ		0x05


#define KIONIXKJ9_DRV_NAME         "kionixtj9"
#define SENSOR_NAME              KIONIXKJ9_DRV_NAME
//#define KIONIXKJ9_XOUT             0x00
//#define KIONIXKJ9_YOUT             0x01
//#define KIONIXKJ9_ZOUT             0x02
//#define KIONIXKJ9_TILT             0x03
//#define KIONIXKJ9_SRST             0x04
//#define KIONIXKJ9_SPCNT            0x05
//#define KIONIXKJ9_INTSU            0x06
//#define KIONIXKJ9_MODE             0x07
//#define KIONIXKJ9_SR               0x08
//#define KIONIXKJ9_PDET             0x09
//#define KIONIXKJ9_PD               0x0A
//#define KIONIXKJ9ADDRESS           0x4c

#define POLL_INTERVAL_MAX        500
#define POLL_INTERVAL            50
#define INPUT_FUZZ               2
#define INPUT_FLAT               2

#define MK_KIONIXKJ9_SR(FILT, AWSR, AMSR)\
	(FILT<<5 | AWSR<<3 | AMSR)

#define MK_KIONIXKJ9_MODE(IAH, IPP, SCPS, ASE, AWE, TON, MODE)\
	(IAH<<7 | IPP<<6 | SCPS<<5 | ASE<<4 | AWE<<3 | TON<<2 | MODE)

#define MK_KIONIXKJ9_INTSU(SHINTX, SHINTY, SHINTZ, GINT, ASINT, PDINT, PLINT, FBINT)\
	(SHINTX<<7 | SHINTY<<6 | SHINTZ<<5 | GINT<<4 | ASINT<<3 | PDINT<<2 | PLINT<<1 | FBINT)

#define MODE_CHANGE_DELAY_MS 100

static struct device *hwmon_dev;
static struct i2c_client *kionix_kj9_i2c_client;

static struct kionix_kj9_data_s {
	struct i2c_client       *client;
	struct input_polled_dev *pollDev; 
	struct mutex interval_mutex;
	struct mutex init_mutex;
    
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
	volatile int suspend_indator;
#endif
} kionix_kj9_data;

static struct input_polled_dev *kionix_kj9_idev;

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {KXTF9_I2C_SLAVE_ADDR, I2C_CLIENT_END};
static __u32 twi_id = 0;
static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

static void kionix_kj9_resume_events(struct work_struct *work);
static void kionix_kj9_init_events(struct work_struct *work);
static struct workqueue_struct *kionix_kj9_resume_wq;
static struct workqueue_struct *kionix_kj9_init_wq;
static DECLARE_WORK(kionix_kj9_resume_work, kionix_kj9_resume_events);
static DECLARE_WORK(kionix_kj9_init_work, kionix_kj9_init_events);

#ifdef CONFIG_HAS_EARLYSUSPEND
static void kionix_kj9_early_suspend(struct early_suspend *h);
static void kionix_kj9_late_resume(struct early_suspend *h);
#endif

//Function as i2c_master_send, and return 1 if operation is successful. 
static int i2c_write_bytes(struct i2c_client *client, uint8_t *data, uint16_t len)
{
	struct i2c_msg msg;
	int ret=-1;
	
	msg.flags = !I2C_M_RD;
	msg.addr = client->addr;
	msg.len = len;
	msg.buf = data;		
	
	ret=i2c_transfer(client->adapter, &msg,1);
	return ret;
}
static bool gsensor_i2c_test(struct i2c_client * client)
{
	int ret, retry;
	uint8_t test_data[1] = { 0 };	//only write a data address.
	
	for(retry=0; retry < 2; retry++)
	{
		ret =i2c_write_bytes(client, test_data, 1);	//Test i2c.
		if (ret == 1)
			break;
		msleep(5);
	}
	
	return ret==1 ? true : false;
}

/**
 * gsensor_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gsensor_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;

	dprintk(DEBUG_INIT, "%s enter \n", __func__);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        	return -ENODEV;
    
	if(twi_id == adapter->nr){
            pr_info("%s: addr= %x\n",__func__,client->addr);

            ret = gsensor_i2c_test(client);
        	if(!ret){
        		pr_info("%s:I2C connection might be something wrong or maybe the other gsensor equipment! \n",__func__);
        		return -ENODEV;
        	}else{           	    
            	pr_info("I2C connection sucess!\n");
            	strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
    		    return 0;	
	             }

	}else{
		return -ENODEV;
	}
}

static int kionix_kj9_read_xyz(int idx, s32 *x, s32 *y, s32 *z)
{
	int ax, ay, az;
	s32 result;
	u8 databuf[8];

	assert(kionix_kj9_i2c_client);
	result=i2c_smbus_read_i2c_block_data(kionix_kj9_i2c_client, KXTF9_REG_XOUT_L, 6, &databuf[0]);


    ax = ((int) databuf[1]) << 4 | ((int) databuf[0]) >> 4;

    ay = ((int) databuf[3]) << 4 | ((int) databuf[2]) >> 4;

    az = ((int) databuf[5]) << 4 | ((int) databuf[4]) >> 4;


	*x = ax;
	*y = ay;
	*z = az;

	return 0;
}
#if 0
static ssize_t kionix_kj9_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client       = kionix_kj9_i2c_client;
	struct kionix_kj9_data_s *kionix_kj9    = NULL;

	kionix_kj9    = i2c_get_clientdata(client);


	return sprintf(buf, "%d\n", kionix_kj9->pollDev->poll_interval);

}
#endif

static ssize_t kionix_kj9_delay_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = kionix_kj9_i2c_client;
	struct kionix_kj9_data_s *kionix_kj9 = NULL;


	kionix_kj9 = i2c_get_clientdata(client);


	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (data > POLL_INTERVAL_MAX)
		data = POLL_INTERVAL_MAX;
    
	mutex_lock(&kionix_kj9->interval_mutex);
	kionix_kj9->pollDev->poll_interval = data;
	mutex_unlock(&kionix_kj9->interval_mutex);

	return count;
}


static ssize_t kionix_kj9_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int i = 0;
	int result = 0;
	s32 xyz[3]; 
	s16 x, y, z;

	//for (i=0; i<3; i++) {
		result = kionix_kj9_read_xyz(i, &xyz[0], &xyz[1], &xyz[2]);
		if (result < 0) {
			dprintk(DEBUG_DATA_INFO, "kionix_kj9 read data failed\n");
			return 0;
		}
	//}

	/* convert signed 8bits to signed 16bits */
	x = (((short)xyz[0]) << 20)>> 20;
	y = (((short)xyz[1]) << 20)>> 20;
	z = (((short)xyz[2]) << 20)>> 20;

	return sprintf(buf, "x= %d y= %d z= %d\n", x, y, z);

}

static ssize_t kionix_kj9_enable_store(struct device *dev,struct device_attribute *attr,const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	
	if (error) {
		pr_err("%s strict_strtoul error\n", __FUNCTION__);
		goto exit;
	}

	if (data) {
		mutex_lock(&kionix_kj9_data.init_mutex);
		//error = i2c_smbus_write_byte_data(kionix_kj9_i2c_client, KIONIXKJ9_MODE,
		//			MK_KIONIXKJ9_MODE(0, 1, 0, 0, 0, 0, 1));
		mutex_unlock(&kionix_kj9_data.init_mutex);
		assert(error==0);
#if defined(CONFIG_PM) || defined(CONFIG_HAS_EARLYSUSPEND)
		if (kionix_kj9_data.suspend_indator == 0)
#endif
			kionix_kj9_idev->input->open(kionix_kj9_idev->input);

		dprintk(DEBUG_CONTROL_INFO, "kionix_kj9 enable setting active \n");
	} else {
		kionix_kj9_idev->input->close(kionix_kj9_idev->input);

		mutex_lock(&kionix_kj9_data.init_mutex);
		//error = i2c_smbus_write_byte_data(kionix_kj9_i2c_client, KIONIXKJ9_MODE,
		//			MK_KIONIXKJ9_MODE(0, 0, 0, 0, 0, 0, 0));
		mutex_unlock(&kionix_kj9_data.init_mutex);
		assert(error==0);
		dprintk(DEBUG_CONTROL_INFO, "kionix_kj9 enable setting inactive \n");
	}

	return count;

exit:
	return error;
}


static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP,
		NULL, kionix_kj9_enable_store);

static DEVICE_ATTR(value, S_IRUGO|S_IWUSR|S_IWGRP,
		kionix_kj9_value_show, NULL);

static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP,
		NULL, kionix_kj9_delay_store);

static struct attribute *kionix_kj9_attributes[] = {
	&dev_attr_value.attr,
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	NULL
};

static struct attribute_group kionix_kj9_attribute_group = {
	.attrs = kionix_kj9_attributes
};


/*
 * Initialization function
 */
static int kionix_kj9_init_client(struct i2c_client *client)
{
	int result;

	kionix_kj9_i2c_client = client;

	mutex_lock(&kionix_kj9_data.init_mutex);
#if 0
	{
		/*
		 * Probe the device. We try to set the device to Test Mode and then to
		 * write & verify XYZ registers
		 */
		result = i2c_smbus_write_byte_data(client, KIONIXKJ9_MODE,MK_KIONIXKJ9_MODE(0, 0, 0, 0, 0, 1, 0));
		assert(result==0);
		mdelay(MODE_CHANGE_DELAY_MS);

		result = i2c_smbus_write_byte_data(client, KIONIXKJ9_XOUT, 0x2a);
		assert(result==0);
		
		result = i2c_smbus_write_byte_data(client, KIONIXKJ9_YOUT, 0x15);
		assert(result==0);

		result = i2c_smbus_write_byte_data(client, KIONIXKJ9_ZOUT, 0x3f);
		assert(result==0);

		result = i2c_smbus_read_byte_data(client, KIONIXKJ9_XOUT);

		result= i2c_smbus_read_byte_data(client, KIONIXKJ9_YOUT);

		result= i2c_smbus_read_byte_data(client, KIONIXKJ9_ZOUT);
		assert(result=0x3f);
	}
#endif
/*
	// Enable Orientation Detection Logic
	result = i2c_smbus_write_byte_data(client, 
		KIONIXKJ9_MODE, MK_KIONIXKJ9_MODE(0, 0, 0, 0, 0, 0, 0)); //enter standby
	assert(result==0);

	result = i2c_smbus_write_byte_data(client, 
		KIONIXKJ9_SR, MK_KIONIXKJ9_SR(2, 2, 1)); 
	assert(result==0);

	result = i2c_smbus_write_byte_data(client, 
		KIONIXKJ9_INTSU, MK_KIONIXKJ9_INTSU(0, 0, 0, 0, 1, 0, 1, 1)); 
	assert(result==0);

	result = i2c_smbus_write_byte_data(client, 
		KIONIXKJ9_SPCNT, 0xA0); 
	assert(result==0);

	result = i2c_smbus_write_byte_data(client, 
		KIONIXKJ9_MODE, MK_KIONIXKJ9_MODE(0, 1, 0, 0, 0, 0, 1)); 
	assert(result==0);
*/



	printk("init kionix_kj9 client 1\n");

    result = i2c_smbus_write_byte_data(client, KXTF9_REG_CTRL_REG1, KXTF9_OPERATION | KXTF9_12BIT);



    result = i2c_smbus_write_byte_data(client, KXTF9_REG_DATA_CTRL, KXTF9_DATA_ODR_400HZ);


	mutex_unlock(&kionix_kj9_data.init_mutex);

	mdelay(MODE_CHANGE_DELAY_MS);

	kionix_kj9_idev->input->open(kionix_kj9_idev->input);

	return result;
}



static void report_abs(void)
{
	int i = 0;
	int result = 0;
	s32 xyz[3]; 
	s16 x, y, z;

		result = kionix_kj9_read_xyz(i, &xyz[0], &xyz[1], &xyz[2]);
		if (result < 0) {
			printk("kionix_kj9 read data failed\n");
			return;
		}

	/* convert signed 8bits to signed 16bits */
	x = (((short)xyz[0]) << 20)>> 20;
	y = (((short)xyz[1]) << 20)>> 20;
	z = (((short)xyz[2]) << 20)>> 20;
	//pr_info("xyz[0] = 0x%hx, xyz[1] = 0x%hx, xyz[2] = 0x%hx. \n", xyz[0], xyz[1], xyz[2]);
	//pr_info("x[0] = 0x%hx, y[1] = 0x%hx, z[2] = 0x%hx. \n", x, y, z);
	
	//x = x / 3;
	//y = y / 3;
	//z = z / 3;

	input_report_abs(kionix_kj9_idev->input, ABS_X, x);
	input_report_abs(kionix_kj9_idev->input, ABS_Y, y);
	input_report_abs(kionix_kj9_idev->input, ABS_Z, z);

	input_sync(kionix_kj9_idev->input);

	dprintk(DEBUG_DATA_INFO, "x= 0x%hx, y = 0x%hx, z = 0x%hx\n", x, y, z);
}

static void kionix_kj9_dev_poll(struct input_polled_dev *dev)
{
	report_abs();
} 

static void kionix_kj9_init_events (struct work_struct *work)
{
	int result = 0;

	result = kionix_kj9_init_client(kionix_kj9_i2c_client);
	assert(result==0);
	if(result != 0)
	{
		printk("<%s> init err !", __func__);
		return;
	}
	dprintk(DEBUG_INIT, "kionix_kj9 init events end\n");
}

/*
 * I2C init/probing/exit functions
 */

static int kionix_kj9_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int result;
	struct input_dev *idev;
	struct i2c_adapter *adapter;
	struct kionix_kj9_data_s* data = &kionix_kj9_data;
 
	dprintk(DEBUG_INIT, "kionix_kj9 probe\n");
	kionix_kj9_i2c_client = client;
	adapter = to_i2c_adapter(client->dev.parent);
 	result = i2c_check_functionality(adapter,
 					 I2C_FUNC_SMBUS_BYTE |
 					 I2C_FUNC_SMBUS_BYTE_DATA);
	assert(result);

	//result = 1; // debug by lchen
	dprintk(DEBUG_INIT, "<%s> kionix_kj9_init_client result %d\n", __func__, result);

	hwmon_dev = hwmon_device_register(&client->dev);
	assert(!(IS_ERR(hwmon_dev)));

	dev_info(&client->dev, "build time %s %s\n", __DATE__, __TIME__);
  
	/*input poll device register */
	kionix_kj9_idev = input_allocate_polled_device();
	if (!kionix_kj9_idev) {
		dev_err(&client->dev, "alloc poll device failed!\n");
		result = -ENOMEM;
		return result;
	}
	kionix_kj9_idev->poll = kionix_kj9_dev_poll;
	kionix_kj9_idev->poll_interval = POLL_INTERVAL;
	kionix_kj9_idev->poll_interval_max = POLL_INTERVAL_MAX;
	idev = kionix_kj9_idev->input;
	idev->name = KIONIXKJ9_DRV_NAME;
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
	mutex_init(&data->interval_mutex);
	mutex_init(&data->init_mutex);

	input_set_abs_params(idev, ABS_X, -2048, 2048, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(idev, ABS_Y, -2048, 2048, INPUT_FUZZ, INPUT_FLAT);
	input_set_abs_params(idev, ABS_Z, -2048, 2048, INPUT_FUZZ, INPUT_FLAT);
	
	result = input_register_polled_device(kionix_kj9_idev);
	if (result) {
		dev_err(&client->dev, "register poll device failed!\n");
		return result;
	}
	kionix_kj9_idev->input->close(kionix_kj9_idev->input);

	result = sysfs_create_group(&kionix_kj9_idev->input->dev.kobj, &kionix_kj9_attribute_group);
	//result = device_create_file(&kionix_kj9_idev->input->dev, &dev_attr_enable);
	//result = device_create_file(&kionix_kj9_idev->input->dev, &dev_attr_value);

	if(result) {
		dev_err(&client->dev, "create sys failed\n");
	}

	data->client  = client;
	data->pollDev = kionix_kj9_idev;
	i2c_set_clientdata(client, data);

	kionix_kj9_init_wq = create_singlethread_workqueue("kionix_kj9_init");
	if (kionix_kj9_init_wq == NULL) {
		printk("create kionix_kj9_init_wq fail!\n");
		return -ENOMEM;
	}

	/* Initialize the KIONIXKJ9 chip */
	queue_work(kionix_kj9_init_wq, &kionix_kj9_init_work);

	kionix_kj9_resume_wq = create_singlethread_workqueue("kionix_kj9_resume");
	if (kionix_kj9_resume_wq == NULL) {
		printk("create kionix_kj9_resume_wq fail!\n");
		return -ENOMEM;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	kionix_kj9_data.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	kionix_kj9_data.early_suspend.suspend = kionix_kj9_early_suspend;
	kionix_kj9_data.early_suspend.resume = kionix_kj9_late_resume;
	register_early_suspend(&kionix_kj9_data.early_suspend);
	kionix_kj9_data.suspend_indator = 0;
#endif
	dprintk(DEBUG_INIT, "kionix_kj9 probe end\n");
	return result;
}

static int kionix_kj9_remove(struct i2c_client *client)
{
	int result = 0;

	mutex_lock(&kionix_kj9_data.init_mutex);
	//result = i2c_smbus_write_byte_data(client,KIONIXKJ9_MODE, MK_KIONIXKJ9_MODE(0, 0, 0, 0, 0, 0, 0));
	//assert(result==0);
	mutex_unlock(&kionix_kj9_data.init_mutex);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&kionix_kj9_data.early_suspend);
#endif
	cancel_work_sync(&kionix_kj9_resume_work);
	destroy_workqueue(kionix_kj9_resume_wq);
	sysfs_remove_group(&kionix_kj9_idev->input->dev.kobj, &kionix_kj9_attribute_group);
	kionix_kj9_idev->input->close(kionix_kj9_idev->input);
	input_unregister_polled_device(kionix_kj9_idev);
	input_free_polled_device(kionix_kj9_idev);
	hwmon_device_unregister(hwmon_dev);
	cancel_work_sync(&kionix_kj9_init_work);
	destroy_workqueue(kionix_kj9_init_wq);
	i2c_set_clientdata(client, NULL);

	return result;
}
static void kionix_kj9_resume_events (struct work_struct *work)
{
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
	int result = 0;
	dprintk(DEBUG_INIT, "kionix_kj9 device init\n");
	result = kionix_kj9_init_client(kionix_kj9_i2c_client);
	assert(result==0);

	mutex_lock(&kionix_kj9_data.init_mutex);
	kionix_kj9_data.suspend_indator = 0;
	//result = i2c_smbus_write_byte_data(kionix_kj9_i2c_client,
	//	KIONIXKJ9_MODE, MK_KIONIXKJ9_MODE(0, 1, 0, 0, 0, 0, 1));
	mutex_unlock(&kionix_kj9_data.init_mutex);
	//assert(result==0);

	kionix_kj9_idev->input->open(kionix_kj9_idev->input);

	dprintk(DEBUG_INIT, "kionix_kj9 device init end\n");
	return;
#endif
}


#ifdef CONFIG_HAS_EARLYSUSPEND
static void kionix_kj9_early_suspend(struct early_suspend *h)
{
	//int result;
	dprintk(DEBUG_SUSPEND, "kionix_kj9 early suspend\n");

	flush_workqueue(kionix_kj9_resume_wq);

	kionix_kj9_idev->input->close(kionix_kj9_idev->input);
	
	mutex_lock(&kionix_kj9_data.init_mutex);
	kionix_kj9_data.suspend_indator = 1;
	//result = i2c_smbus_write_byte_data(kionix_kj9_i2c_client, 
	//	KIONIXKJ9_MODE, MK_KIONIXKJ9_MODE(0, 0, 0, 0, 0, 0, 0));
	mutex_unlock(&kionix_kj9_data.init_mutex);
	//assert(result==0);
	return;
}

static void kionix_kj9_late_resume(struct early_suspend *h)
{
	//int result;
	dprintk(DEBUG_SUSPEND, "kionix_kj9 late resume\n");
	
	if (SUPER_STANDBY == standby_type)
		queue_work(kionix_kj9_resume_wq, &kionix_kj9_resume_work);

	if (NORMAL_STANDBY == standby_type) {
		mutex_lock(&kionix_kj9_data.init_mutex);
		kionix_kj9_data.suspend_indator = 0;
//		result = i2c_smbus_write_byte_data(kionix_kj9_i2c_client,
//			KIONIXKJ9_MODE, MK_KIONIXKJ9_MODE(0, 1, 0, 0, 0, 0, 1));
		//assert(result==0);
		mutex_unlock(&kionix_kj9_data.init_mutex);
		kionix_kj9_idev->input->open(kionix_kj9_idev->input);
	}
	return;
}
#else
#ifdef CONFIG_PM
static int kionix_kj9_resume(struct i2c_client *client)
{
	int result = 0;
	dprintk(DEBUG_SUSPEND, "kionix_kj9 resume\n");
	
	if (SUPER_STANDBY == standby_type) {
		queue_work(kionix_kj9_resume_wq, &kionix_kj9_resume_work);
	}
	if (SUPER_STANDBY == standby_type) {
		mutex_lock(&kionix_kj9_data.init_mutex);
		kionix_kj9_data.suspend_indator = 0;
//		result = i2c_smbus_write_byte_data(kionix_kj9_i2c_client,
//			KIONIXKJ9_MODE, MK_KIONIXKJ9_MODE(0, 1, 0, 0, 0, 0, 1));
		mutex_unlock(&kionix_kj9_data.init_mutex);
		assert(result==0);
		kionix_kj9_idev->input->open(kionix_kj9_idev->input);
	}
	return result;
}

static int kionix_kj9_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int result = 0;
	dprintk(DEBUG_SUSPEND, "kionix_kj9 suspend\n");

	flush_workqueue(kionix_kj9_resume_wq);
	
	kionix_kj9_idev->input->close(kionix_kj9_idev->input);

	mutex_lock(&kionix_kj9_data.init_mutex);
	kionix_kj9_data.suspend_indator = 1;
	//result = i2c_smbus_write_byte_data(kionix_kj9_i2c_client, 
	//	KIONIXKJ9_MODE, MK_KIONIXKJ9_MODE(0, 0, 0, 0, 0, 0, 0));
	mutex_unlock(&kionix_kj9_data.init_mutex);
	assert(result==0);
	return result;
}
#endif
#endif /* CONFIG_HAS_EARLYSUSPEND */

static const struct i2c_device_id kionix_kj9_id[] = {
	{ KIONIXKJ9_DRV_NAME, 1 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, kionix_kj9_id);

static struct i2c_driver kionix_kj9_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		.name	= KIONIXKJ9_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.probe	= kionix_kj9_probe,
	.remove	= kionix_kj9_remove,
#ifdef CONFIG_HAS_EARLYSUSPEND
#else
#ifdef CONFIG_PM
	.suspend = kionix_kj9_suspend,
	.resume = kionix_kj9_resume,
#endif
#endif
	.id_table = kionix_kj9_id,
	.detect = gsensor_detect,
	.address_list	= normal_i2c,
};

static int __init kionix_kj9_init(void)
{
	int ret = -1;
	printk("======%s=========. \n", __func__);
	
	if (input_fetch_sysconfig_para(&(gsensor_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else
		twi_id = gsensor_info.twi_id;

	dprintk(DEBUG_INIT, "%s i2c twi is %d \n", __func__, twi_id);

	ret = i2c_add_driver(&kionix_kj9_driver);
	if (ret < 0) {
		printk(KERN_INFO "add kionix_kj9 i2c driver failed\n");
		return -ENODEV;
	}
	dprintk(DEBUG_INIT, "add kionix_kj9 i2c driver\n");
	


	return ret;
}

static void __exit kionix_kj9_exit(void)
{
	printk(KERN_INFO "remove kionix_kj9 i2c driver.\n");
	i2c_del_driver(&kionix_kj9_driver);
}

MODULE_AUTHOR("Chen Gang <gang.chen@freescale.com>");
MODULE_DESCRIPTION("KIONIX 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.1");

module_init(kionix_kj9_init);
module_exit(kionix_kj9_exit);


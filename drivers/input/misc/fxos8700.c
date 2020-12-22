/*
 *  fxos8700.c - Linux kernel modules for 3-Axis M+G Sensor
 *
 *  Copyright (C) 2012 Freescale Semiconductor, Inc. All Rights Reserved.
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
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/input-polldev.h>
#include <linux/init-input.h>

#include "fxos8700.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif


#define FXOS8700_TYPE_ACC 	0x00
#define FXOS8700_TYPE_MAG 	0x01

#define FXOS8700_STANDBY 	0x00
#define FXOS8700_ACTIVED 	0x01

#define ABS_STATUS 			ABS_WHEEL
struct fxos8700_data_axis{
	short x;
	short y;
	short z;
};

struct fxos8700_data{
	struct i2c_client * client;
	struct input_polled_dev *acc_poll_dev;
	struct input_polled_dev *mag_poll_dev;
	struct input_dev *cal_input;
	struct mutex data_lock;
	int acc_active;
	int mag_active;
	int position;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};
static int fxos8700_position_settings[8][3][3] =
{
   {{ 0, -1,  0}, { 1,  0,  0}, {0, 0,  1}},
   {{-1,  0,  0}, { 0, -1,  0}, {0, 0,	1}},
   {{ 0,  1,  0}, {-1,  0,  0}, {0, 0,	1}},
   {{ 1,  0,  0}, { 0,  1,  0}, {0, 0,	1}},

   {{ 0, -1,  0}, {-1,  0,  0}, {0, 0, -1}},
   {{-1,  0,  0}, { 0,  1,  0}, {0, 0, -1}},
   {{ 0,  1,  0}, { 1,  0,  0}, {0, 0, -1}},
   {{ 1,  0,  0}, { 0, -1,  0}, {0, 0, -1}},
};

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x1c,I2C_CLIENT_END};
static __u32 twi_id = 0;
static int i2c_num = 0;
static const unsigned short i2c_address[] = {0x1c, 0x1d, 0x1e, 0x1f};
static struct sensor_config_info e_compass_info = {
	.input_type = COMPASS_TYPE,
};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,
	DEBUG_REPORT_DATA = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_INT = 1U << 4,
};

static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);


static int fxos8700_data_convert(struct fxos8700_data_axis *axis_data,int position)
{
	short rawdata[3],data[3];
	int i,j;
	if(position < 0 || position > 7 )
		position = 0;
	rawdata [0] = axis_data->x;
	rawdata [1] = axis_data->y;
	rawdata [2] = axis_data->z;
	for(i = 0; i < 3 ; i++)
	{
		data[i] = 0;
		for(j = 0; j < 3; j++)
			data[i] += rawdata[j] * fxos8700_position_settings[position][i][j];
	}
	axis_data->x = data[0];
	axis_data->y = data[1];
	axis_data->z = data[2];
	return 0;
}

/**
 * e_compass_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int e_compass_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;

	dprintk(DEBUG_INIT, "%s enter \n", __func__);
	
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;
            
	if (twi_id == adapter->nr) {
		for (i2c_num = 0; i2c_num < (sizeof(i2c_address)/sizeof(i2c_address[0]));i2c_num++) {	    
			client->addr = i2c_address[i2c_num];
			dprintk(DEBUG_INIT, "%s:addr= 0x%x,i2c_num:%d\n",__func__,client->addr,i2c_num);
			ret = i2c_smbus_read_byte_data(client,FXOS8700_WHO_AM_I);
			dprintk(DEBUG_INIT, "Read ID value is :%d",ret);
			if ((ret &0x00FF) == FXOS8700_DEVICE_ID) {
				dprintk(DEBUG_INIT, "FXOS8700 Device detected!\n" );
				strlcpy(info->type, FXOS8700_DEV_NAME, I2C_NAME_SIZE);
				return 0; 
			}                                                           
		}
        
		pr_info("%s:FXOS8700 Device not found, \
		maybe the other gsensor equipment! \n",__func__);
		return -ENODEV;
	} else {
		return -ENODEV;
	}
}


static int fxos8700_change_mode(struct i2c_client *client, int type,int active)
{
	u8 data;
	int acc_act,mag_act;
	struct fxos8700_data *pdata =  i2c_get_clientdata(client);
	acc_act = pdata->acc_active;
	mag_act = pdata->mag_active;
	data = i2c_smbus_read_byte_data(client, FXOS8700_CTRL_REG1);
	data &= ~0x01;
	i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1,data); //change to standby

	data = i2c_smbus_read_byte_data(client, FXOS8700_M_CTRL_REG1);
	data &= ~0x03;     //clear the m_hms bits
	if(type == FXOS8700_TYPE_ACC)
		acc_act = active;
	else
		mag_act = active;
	if(acc_act == FXOS8700_ACTIVED || mag_act == FXOS8700_ACTIVED)
	  data |= 0x03;
	else
	  data |= 0x00;
	i2c_smbus_write_byte_data(client, FXOS8700_M_CTRL_REG1,data);

	data = i2c_smbus_read_byte_data(client, FXOS8700_CTRL_REG1);
	if(acc_act ==  FXOS8700_ACTIVED || mag_act == FXOS8700_ACTIVED)
		data |= 0x01;
	else
		data &= ~0x01;
	i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1,data);
	return 0;

}
static int fxos8700_device_init(struct i2c_client *client)
{
	int result;
	struct fxos8700_data *pdata =  i2c_get_clientdata(client);
	pdata->acc_active = FXOS8700_STANDBY;
	pdata->mag_active = FXOS8700_STANDBY;
	result = i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1, 0x00); //standby mode
	if (result < 0)
		goto out;
	result = i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1, 0x18); //0dr 100hz
	if (result < 0)
		goto out;
	result = i2c_smbus_write_byte_data(client, FXOS8700_M_CTRL_REG1,0x3); //hybrid mode
	if (result < 0)
		goto out;
	pdata->position = 3;
	return 0;
out:
	dev_err(&client->dev, "error when init fxos8700 device:(%d)", result);
	return result;
}

static int fxos8700_device_stop(struct i2c_client *client)
{
	i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1, 0x00);
	return 0;
}

static int fxos8700_read_data(struct i2c_client *client,struct fxos8700_data_axis *data, int type)
{
	u8 tmp_data[FXOS8700_DATA_BUF_SIZE];
	int ret;
	u8 reg;
	if(type == FXOS8700_TYPE_ACC)
		reg = FXOS8700_OUT_X_MSB;
	else
		reg = FXOS8700_M_OUT_X_MSB;

	ret = i2c_smbus_read_i2c_block_data(client, reg, FXOS8700_DATA_BUF_SIZE, tmp_data);
	if (ret < FXOS8700_DATA_BUF_SIZE) {
		dev_err(&client->dev, "i2c block read %s failed\n", (type == FXOS8700_TYPE_ACC ? "acc" : "mag"));
		return -EIO;
	}
	data->x = ((tmp_data[0] << 8) & 0xff00) | tmp_data[1];
	data->y = ((tmp_data[2] << 8) & 0xff00) | tmp_data[3];
	data->z = ((tmp_data[4] << 8) & 0xff00) | tmp_data[5];
	if(type == FXOS8700_TYPE_MAG)
	{
		data->x *= -1;
		data->y *= -1;
		data->z *= -1;
	}
	return 0;
}


static int fxos8700_report_data(struct input_dev *idev, struct fxos8700_data_axis *data)
{
	if(idev != NULL){
		input_report_abs(idev, ABS_X, data->x);
		input_report_abs(idev, ABS_Y, data->y);
		input_report_abs(idev, ABS_Z, data->z);
		input_sync(idev);
		dprintk(DEBUG_REPORT_DATA, "fxos8700 x = %d, y = %d, z = %d. \n", \
		data->x, data->y, data->z); 
	}
	return 0;
}

static void fxos8700_acc_dev_poll(struct input_polled_dev *dev)
{
	struct fxos8700_data *pdata = (struct fxos8700_data *)dev->private;
	struct i2c_client *client = pdata->client;
	struct fxos8700_data_axis data;
	mutex_lock(&pdata->data_lock);
	if(pdata->acc_active ==  FXOS8700_ACTIVED){
		fxos8700_read_data(client,&data,FXOS8700_TYPE_ACC);
		fxos8700_data_convert(&data,pdata->position);
		fxos8700_report_data(dev->input,&data);
		if(dev->poll_interval == POLL_STOP_TIME)
			dev->poll_interval = POLL_INTERVAL;
	}
	else
		dev->poll_interval = POLL_STOP_TIME;  //longer the interval
	mutex_unlock(&pdata->data_lock);
}

static void fxos8700_mag_dev_poll(struct input_polled_dev *dev)
{
	struct fxos8700_data *pdata = (struct fxos8700_data *)dev->private;
	struct i2c_client *client = pdata->client;
	struct fxos8700_data_axis data;
	mutex_lock(&pdata->data_lock);
	if(pdata->mag_active ==  FXOS8700_ACTIVED){
		fxos8700_read_data(client,&data,FXOS8700_TYPE_MAG);
		fxos8700_data_convert(&data,pdata->position);
		fxos8700_report_data(dev->input,&data);
		if(dev->poll_interval == POLL_STOP_TIME)
			dev->poll_interval = POLL_INTERVAL;
	}
	else
		dev->poll_interval = POLL_STOP_TIME;  // if standby ,set as 10s to slow the poll,
	mutex_unlock(&pdata->data_lock);
}

static int fxos8700_register_poll_device(struct fxos8700_data *pdata)
{
	struct input_dev *idev = NULL;
	struct input_polled_dev * poll_idev = NULL;
	struct i2c_client *client = pdata->client;
	int err = -1;
	/*register poll deivce for g-sensor*/
	poll_idev = input_allocate_polled_device();
	if(!poll_idev)
		goto out;
	idev = poll_idev->input;
	poll_idev->private = pdata;
	poll_idev->poll = fxos8700_acc_dev_poll;
	poll_idev->poll_interval = POLL_STOP_TIME;
	poll_idev->poll_interval_min = POLL_INTERVAL_MIN;
	poll_idev->poll_interval_max = POLL_INTERVAL_MAX;
	idev = poll_idev->input;
	idev->name = "FreescaleAccelerometer";
	idev->uniq = "Freescale fxos8700";
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
	input_set_abs_params(idev, ABS_X,  -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(idev, ABS_Y,  -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(idev, ABS_Z,  -0x7fff, 0x7fff, 0, 0);
	pdata->acc_poll_dev = poll_idev;
	err = input_register_polled_device(pdata->acc_poll_dev);
	if (err) {
		dev_err(&client->dev, "register poll device failed!\n");
		goto err_register_acc;
	}
	/*register poll deivce for m-sensor*/
	poll_idev = input_allocate_polled_device();
	if(!poll_idev)
		goto err_alloc_mag;
	idev = poll_idev->input;
	poll_idev->private = pdata;
	poll_idev->poll = fxos8700_mag_dev_poll;
	poll_idev->poll_interval = POLL_INTERVAL;
	poll_idev->poll_interval_min = POLL_INTERVAL_MIN;
	poll_idev->poll_interval_max = POLL_INTERVAL_MAX;
	idev = poll_idev->input;
	idev->name = "FreescaleMagnetometer";
	idev->uniq = "Freescale fxos8700";
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
	input_set_abs_params(idev, ABS_X,  -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(idev, ABS_Y,  -0x7fff, 0x7fff, 0, 0);
	input_set_abs_params(idev, ABS_Z,  -0x7fff, 0x7fff, 0, 0);
	pdata->mag_poll_dev = poll_idev;
	err = input_register_polled_device(pdata->mag_poll_dev);
	if (err) {
		dev_err(&client->dev, "register poll device failed!\n");
		goto err_register_mag;
	}
	return 0;
err_register_mag:
	input_free_polled_device(pdata->mag_poll_dev);
err_alloc_mag:
	input_unregister_polled_device(pdata->acc_poll_dev);
err_register_acc:
	input_free_polled_device(pdata->acc_poll_dev);
out:
	pdata->acc_poll_dev = NULL;
	pdata->mag_poll_dev = NULL;
	return err;
}
static int fxos8700_unregister_poll_device(struct fxos8700_data *pdata)
{
	if(pdata->acc_poll_dev){
		input_unregister_polled_device(pdata->acc_poll_dev);
		input_free_polled_device(pdata->acc_poll_dev);
	}
	if(pdata->mag_poll_dev){
		input_unregister_polled_device(pdata->mag_poll_dev);
		input_free_polled_device(pdata->mag_poll_dev);
	}
   return 0;
}

static int fxos8700_reigister_caldata_input(struct fxos8700_data * pdata)
{
	struct input_dev *idev;
	struct i2c_client * client = pdata->client;
	int ret;
	idev = input_allocate_device();
	if(!idev){
		dev_err(&client->dev,"alloc calibrated data device error\n");
		return -EINVAL;
	}
	idev->name = "eCompass";
	idev->id.bustype = BUS_I2C;
	idev->evbit[0] = BIT_MASK(EV_ABS);
	input_set_abs_params(idev, ABS_X, -15000, 15000, 0, 0);
	input_set_abs_params(idev, ABS_Y, -15000, 15000, 0, 0);
	input_set_abs_params(idev, ABS_Z, -15000, 15000, 0, 0);
	input_set_abs_params(idev, ABS_RX, 	   0, 36000, 0, 0);
	input_set_abs_params(idev, ABS_RY,-18000, 18000, 0, 0);
	input_set_abs_params(idev, ABS_RZ, -9000,  9000, 0, 0);
	input_set_abs_params(idev, ABS_STATUS, 0,     3, 0, 0);
	ret = input_register_device(idev);
	if(ret){
		dev_err(&client->dev, "register poll device failed!\n");
		return -EINVAL;
	}
	pdata->cal_input = idev;
	return 0;
}
static int fxos8700_unreigister_caldata_input(struct fxos8700_data * pdata)
{
	struct input_dev *idev = pdata->cal_input;
	if(idev){
		input_unregister_device(idev);
	}
	pdata->cal_input = NULL;
	return 0;
}

static ssize_t fxos8700_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct fxos8700_data *pdata = (struct fxos8700_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	u8 val,mode;
	int enable;
	mutex_lock(&pdata->data_lock);
	val = i2c_smbus_read_byte_data(client, FXOS8700_CTRL_REG1);
	mode = i2c_smbus_read_byte_data(client, FXOS8700_M_CTRL_REG1);
	enable = 0;
	if(pdata->acc_poll_dev == poll_dev){
		if((val &0x01) && ((mode &0x03) == 0x03) && pdata->acc_active == FXOS8700_ACTIVED)
			enable = FXOS8700_ACTIVED;
		else
			enable = FXOS8700_STANDBY;
	}
	if(pdata->mag_poll_dev == poll_dev){
		if((val &0x01) && ((mode &0x03) == 0x03) && pdata->mag_active == FXOS8700_ACTIVED)
			enable = FXOS8700_ACTIVED;
		else
			enable = FXOS8700_STANDBY;
	}
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", enable);
}


static ssize_t fxos8700_enable_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct fxos8700_data *pdata = (struct fxos8700_data *)(poll_dev->private);
	struct i2c_client *client = pdata->client;
	int type;
	int enable;
	int ret;
	enable = simple_strtoul(buf, NULL, 10);
	dprintk(DEBUG_CONTROL_INFO, "%s: start\n", __func__);
	mutex_lock(&pdata->data_lock);
	if(poll_dev == pdata->acc_poll_dev)
		type = FXOS8700_TYPE_ACC;
	if(poll_dev == pdata->mag_poll_dev)
		type = FXOS8700_TYPE_MAG;
	enable = (enable > 0 ? FXOS8700_ACTIVED : FXOS8700_STANDBY);
	ret = fxos8700_change_mode(client,type,enable);
	if(!ret){
		if(type == FXOS8700_TYPE_ACC)
			pdata->acc_active = enable;
		else
			pdata->mag_active = enable;
	}
	mutex_unlock(&pdata->data_lock);
	return count;
}
static ssize_t fxos8700_position_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct fxos8700_data *pdata = (struct fxos8700_data *)(poll_dev->private);
	int position = 0;
	mutex_lock(&pdata->data_lock);
	position = pdata->position ;
	mutex_unlock(&pdata->data_lock);
	return sprintf(buf, "%d\n", position);
}

static ssize_t fxos8700_position_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	int  position;
	struct input_polled_dev *poll_dev = dev_get_drvdata(dev);
	struct fxos8700_data *pdata = (struct fxos8700_data *)(poll_dev->private);
	position = simple_strtoul(buf, NULL, 10);
	mutex_lock(&pdata->data_lock);
	pdata->position = position;
	mutex_unlock(&pdata->data_lock);
	return count;
}

static DEVICE_ATTR(enable, S_IWUSR | S_IRUGO,
		   fxos8700_enable_show, fxos8700_enable_store);
static DEVICE_ATTR(position, S_IWUSR | S_IRUGO,
		   fxos8700_position_show, fxos8700_position_store);

static struct attribute *fxos8700_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_position.attr,
	NULL
};

static const struct attribute_group fxos8700_attr_group = {
	.attrs = fxos8700_attributes,
};

static int fxos8700_register_sysfs_device(struct fxos8700_data *pdata)
{
	struct input_dev *idev = NULL;
	struct input_polled_dev * poll_idev = NULL;
	struct i2c_client * client = pdata->client;
	int err = -1;
	/*register sysfs for acc*/
	poll_idev = pdata->acc_poll_dev;
	idev = poll_idev->input;
	err = sysfs_create_group(&idev->dev.kobj, &fxos8700_attr_group);
	if (err) {
		dev_err(&client->dev, "register poll device failed!\n");
		goto out;
	}

	/*register sysfs for mag*/
	poll_idev = pdata->mag_poll_dev;
	idev = poll_idev->input;
	err = sysfs_create_group(&idev->dev.kobj, &fxos8700_attr_group);
	if (err) {
		dev_err(&client->dev, "register poll device failed!\n");
		goto err_register_sysfs;
	}
	return 0;
err_register_sysfs:
	poll_idev = pdata->acc_poll_dev;
	idev = poll_idev->input;
	sysfs_remove_group(&idev->dev.kobj,&fxos8700_attr_group);  /*remove accel senosr sysfs*/
out:
	return err;
}
static int fxos8700_unregister_sysfs_device(struct fxos8700_data *pdata)
{
	struct input_dev *idev = NULL;
	struct input_polled_dev * poll_idev = NULL;
	poll_idev = pdata->acc_poll_dev;
	idev = poll_idev->input;
	sysfs_remove_group(&idev->dev.kobj,&fxos8700_attr_group);  /*remove accel senosr sysfs*/

	poll_idev = pdata->mag_poll_dev;
	idev = poll_idev->input;
	sysfs_remove_group(&idev->dev.kobj,&fxos8700_attr_group);  /*remove accel senosr sysfs*/
	return 0;
}

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static int fxos8700_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fxos8700_data *pdata =  i2c_get_clientdata(client);
	dprintk(DEBUG_SUSPEND, "%s: start\n", __func__);
	if (pdata->acc_poll_dev->input->users)
		pdata->acc_poll_dev->input->close(pdata->acc_poll_dev->input);
	if (pdata->mag_poll_dev->input->users)
		pdata->mag_poll_dev->input->close(pdata->mag_poll_dev->input);
	mutex_lock(&pdata->data_lock);
	if(pdata->acc_active || pdata->mag_active)
		fxos8700_device_stop(client);
	mutex_unlock(&pdata->data_lock);
	return 0;
}

static int fxos8700_resume(struct device *dev)
{
	int ret = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct fxos8700_data *pdata =  i2c_get_clientdata(client);
	dprintk(DEBUG_SUSPEND, "%s: start\n", __func__);
	if (NORMAL_STANDBY == standby_type) {
		mutex_lock(&pdata->data_lock);
		if(pdata->acc_active)
			fxos8700_change_mode(client,FXOS8700_TYPE_ACC,FXOS8700_ACTIVED);
		if(pdata->mag_active)
			fxos8700_change_mode(client,FXOS8700_TYPE_MAG,FXOS8700_ACTIVED);
		mutex_unlock(&pdata->data_lock);
	} else if (SUPER_STANDBY == standby_type) {
		ret = i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1, 0x00); //standby mode
		if (ret < 0)
			goto out;
		ret = i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1, 0x18); //0dr 100hz
		if (ret < 0)
			goto out;
		ret = i2c_smbus_write_byte_data(client, FXOS8700_M_CTRL_REG1,0x3); //hybrid mode
		if (ret < 0)
			goto out;
		
		mutex_lock(&pdata->data_lock);
		if(pdata->acc_active)
			fxos8700_change_mode(client,FXOS8700_TYPE_ACC,FXOS8700_ACTIVED);
		if(pdata->mag_active)
			fxos8700_change_mode(client,FXOS8700_TYPE_MAG,FXOS8700_ACTIVED);
		mutex_unlock(&pdata->data_lock);
	}
	if (pdata->acc_poll_dev->input->users)
		pdata->acc_poll_dev->input->open(pdata->acc_poll_dev->input);
	if (pdata->mag_poll_dev->input->users)
		pdata->mag_poll_dev->input->open(pdata->mag_poll_dev->input);
out:
	return ret;

}
#else
#define fxos8700_suspend  NULL;
#define fxos8700_resume   NULL;
#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
static void fxos8700_early_suspend(struct early_suspend *h)
{
	struct fxos8700_data *pdata =  container_of(h, struct fxos8700_data, early_suspend);
	struct i2c_client *client =  pdata->client;
	dprintk(DEBUG_SUSPEND, "%s: start\n", __func__);
	if (pdata->acc_poll_dev->input->users)
		pdata->acc_poll_dev->input->close(pdata->acc_poll_dev->input);
	if (pdata->mag_poll_dev->input->users)
		pdata->mag_poll_dev->input->close(pdata->mag_poll_dev->input);
	mutex_lock(&pdata->data_lock);
	if(pdata->acc_active || pdata->mag_active)
		fxos8700_device_stop(client);
	mutex_unlock(&pdata->data_lock);
}
static void fxos8700_late_resume(struct early_suspend *h)
{
	int ret = 0;
	
	struct fxos8700_data *pdata =  container_of(h, struct fxos8700_data, early_suspend);
	struct i2c_client *client =  pdata->client;
	dprintk(DEBUG_SUSPEND, "%s: start\n", __func__);
	if (NORMAL_STANDBY == standby_type) {
		mutex_lock(&pdata->data_lock);
		if(pdata->acc_active)
			fxos8700_change_mode(client,FXOS8700_TYPE_ACC,FXOS8700_ACTIVED);
		if(pdata->mag_active)
			fxos8700_change_mode(client,FXOS8700_TYPE_MAG,FXOS8700_ACTIVED);
		mutex_unlock(&pdata->data_lock);
	} else if (SUPER_STANDBY == standby_type) {
		mutex_lock(&pdata->data_lock);
		ret = i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1, 0x00); //standby mode
		if (ret < 0)
			printk("fxos8700 write register failed\n");
		ret = i2c_smbus_write_byte_data(client, FXOS8700_CTRL_REG1, 0x18); //0dr 100hz
		if (ret < 0)
			printk("fxos8700 write register failed\n");
		ret = i2c_smbus_write_byte_data(client, FXOS8700_M_CTRL_REG1,0x3); //hybrid mode
		if (ret < 0)
			printk("fxos8700 write register failed\n");
		if(pdata->acc_active)
			fxos8700_change_mode(client,FXOS8700_TYPE_ACC,FXOS8700_ACTIVED);
		if(pdata->mag_active)
			fxos8700_change_mode(client,FXOS8700_TYPE_MAG,FXOS8700_ACTIVED);
		mutex_unlock(&pdata->data_lock);
	}
	if (pdata->acc_poll_dev->input->users)
		pdata->acc_poll_dev->input->open(pdata->acc_poll_dev->input);
	if (pdata->mag_poll_dev->input->users)
		pdata->mag_poll_dev->input->open(pdata->mag_poll_dev->input);
}
#endif

static int fxos8700_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int result, client_id;
	struct fxos8700_data * pdata;
	struct i2c_adapter *adapter;

	dprintk(DEBUG_INIT, "fxos8700 probe i2c address is %d \n",i2c_address[i2c_num]);
	client->addr = i2c_address[i2c_num];

	adapter = to_i2c_adapter(client->dev.parent);
	result = i2c_check_functionality(adapter,
					 I2C_FUNC_SMBUS_BYTE |
					 I2C_FUNC_SMBUS_BYTE_DATA);
	if (!result)
		goto err_out;

	client_id = i2c_smbus_read_byte_data(client, FXOS8700_WHO_AM_I);
	if (client_id !=  FXOS8700_DEVICE_ID ) {
		dev_err(&client->dev,
			"read chip ID 0x%x is not equal to 0x%x \n",
			result, FXOS8700_DEVICE_ID);
		result = -EINVAL;
		goto err_out;
	}
	pdata = kzalloc(sizeof(struct fxos8700_data), GFP_KERNEL);
	if(!pdata){
		result = -ENOMEM;
		dev_err(&client->dev, "alloc data memory error!\n");
		goto err_out;
	}
	
	pdata->client = client;
	mutex_init(&pdata->data_lock);
	i2c_set_clientdata(client,pdata);
	result = fxos8700_register_poll_device(pdata);
	if (result) {
		dev_err(&client->dev, "create device file failed!\n");
		result = -EINVAL;
		goto err_register_poll_device;
	}
	result = fxos8700_register_sysfs_device(pdata);
	if (result) {
		dev_err(&client->dev, "create device file failed!\n");
		result = -EINVAL;
		goto err_register_sys;
	}
	result = fxos8700_reigister_caldata_input(pdata);
	if (result) {
		dev_err(&client->dev, "create calibrated device file failed!\n");
		result = -EINVAL;
		goto err_register_caldev;
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	pdata->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	pdata->early_suspend.suspend = fxos8700_early_suspend;
	pdata->early_suspend.resume = fxos8700_late_resume;
	register_early_suspend(&pdata->early_suspend);
#endif
	fxos8700_device_init(client);
	printk("%s succ\n",__FUNCTION__);
	return 0;
err_register_caldev:
	fxos8700_unregister_sysfs_device(pdata);
err_register_sys:
	fxos8700_unregister_poll_device(pdata);
err_register_poll_device:
	i2c_set_clientdata(client,NULL);
	kfree(pdata);
err_out:
	return result;
}

static int fxos8700_remove(struct i2c_client *client)
{
	struct fxos8700_data *pdata =  i2c_get_clientdata(client);
	if(!pdata)
		return 0;
	fxos8700_device_stop(client);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&pdata->early_suspend);
#endif
	fxos8700_unreigister_caldata_input(pdata);
	fxos8700_unregister_sysfs_device(pdata);
	pdata->acc_poll_dev->input->close(pdata->acc_poll_dev->input);
	pdata->mag_poll_dev->input->close(pdata->mag_poll_dev->input);
	fxos8700_unregister_poll_device(pdata);
	i2c_set_clientdata(client, NULL);
	kfree(pdata);
	return 0;
}


static const struct i2c_device_id fxos8700_id[] = {
	{"fxos8700", 0},
	{ }
};
MODULE_DEVICE_TABLE(i2c, fxos8700_id);

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static struct dev_pm_ops fxos8700_pm = {
	.suspend = fxos8700_suspend,
	.resume = fxos8700_resume,
};
#endif


static struct i2c_driver fxos8700_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
		   .name = FXOS8700_DEV_NAME,
		   .owner = THIS_MODULE,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
		   .pm = &fxos8700_pm,
#endif
		   },
	.probe = fxos8700_probe,
	.remove = fxos8700_remove,
	.id_table = fxos8700_id,
	.detect = e_compass_detect,
	.address_list	= normal_i2c,
};

static int __init fxos8700_init(void)
{
	/* register driver */
	int res;

	pr_info("%s driver: init\n", FXOS8700_DEV_NAME);
	if (input_fetch_sysconfig_para(&(e_compass_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else
		twi_id = e_compass_info.twi_id;

	dprintk(DEBUG_INIT, "%s i2c twi is %d \n", __func__, twi_id);

	res = i2c_add_driver(&fxos8700_driver);
	if (res < 0) {
		printk(KERN_INFO "add fxos8700 i2c driver failed\n");
		return -ENODEV;
	}
	return res;
}

static void __exit fxos8700_exit(void)
{
	i2c_del_driver(&fxos8700_driver);
}

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MMA845X 3-Axis Orientation/Motion Detection Sensor driver");
MODULE_LICENSE("GPL");

module_init(fxos8700_init);
module_exit(fxos8700_exit);

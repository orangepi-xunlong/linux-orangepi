/* drivers/misc/icm42607.c - icm42607  driver
 *
 * Copyright (C) 2003-2015 Invensense Corporation.
 * Author: Mingyang Li <myli@invensense.com>
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

#include "icm42607.h"

#define MPU_DEVICE_NAME             "icm42607"
#define ICM42607_POLL_INTERVAL      100
#define ICM42607_POLL_INTERVAL_MIN  5
#define ICM42607_POLL_INTERVAL_MAX 	500
#define ICM42607_CAPTURE_TIMES      10
#define ICM42607_ACCEL_SENSITIVE    16384

int inv_debug = 0;
#define inv_info(fmt, arg...)   \
    do{  \
        if (inv_debug) \
            printk("%s "fmt, MPU_DEVICE_NAME, ##arg);  \
    }while(0)

static int icm42607_read_accel_xyz(struct icm42607_device *dev);
static int icm42607_read_gyro_xyz(struct icm42607_device *dev);

static int icm42607_read(struct i2c_client *client, unsigned char addr)
{
	int ret = 0;

	ret = i2c_smbus_read_byte_data(client, addr);
	if (ret < 0) {
		printk("failed to read register 0x%02x, error %d\n", addr, ret);
    }

	return ret;
}

static int icm42607_write(struct i2c_client *client, unsigned char addr, unsigned char data)
{
	int ret = 0;

	ret = i2c_smbus_write_byte_data(client, addr, data);
	if (ret < 0) {
		printk("failed to write register 0x%02x, error %d\n", addr, ret);
		return ret;
	}

	return 0;
}

static void icm42607_accel_enable(struct icm42607_device *dev)
{
    struct i2c_client *client = dev->client;
	unsigned char data = 0;
	int res = 0;

    data = icm42607_read(client, ICM42607_PWR_MGMT0);
	data &= 0xFC;
    data |= 0x03;

	res = icm42607_write(client, ICM42607_PWR_MGMT0, data);
	if (res) {
	    printk("icm42607_accel_enable error res=%d\n", res);
	}
    msleep(10);
}

static void icm42607_accel_disable(struct icm42607_device *dev)
{
    struct i2c_client *client = dev->client;
	unsigned char data = 0;
	int res = 0;

    data = icm42607_read(client, ICM42607_PWR_MGMT0);
	data &= 0xFC;

	res = icm42607_write(client, ICM42607_PWR_MGMT0, data);
	if (res) {
	    printk("icm42607_accel_disable error res=%d\n", res);
	}
    msleep(1);
}

static void icm42607_gyro_enable(struct icm42607_device *dev)
{
    struct i2c_client *client = dev->client;
	unsigned char data = 0;
	int res = 0;

    data = icm42607_read(client, ICM42607_PWR_MGMT0);
	data &= 0xF3;
    data |= 0x0C;

	res = icm42607_write(client, ICM42607_PWR_MGMT0, data);
	if (res) {
	    printk("icm42607_gyro_enable error res=%d\n", res);
	}
    msleep(80);
}

static void icm42607_gyro_disable(struct icm42607_device *dev)
{
    struct i2c_client *client = dev->client;
	unsigned char data = 0;
	int res = 0;

    data = icm42607_read(client, ICM42607_PWR_MGMT0);
	data &= 0xF3;

	res = icm42607_write(client, ICM42607_PWR_MGMT0, data);
	if (res) {
	    printk("icm42607_gyro_disable error res=%d\n", res);
	}
    msleep(10);
}

static void icm42607_reset_delay(struct icm42607_device *dev)
{
	struct icm42607_device *mpu =dev;

	mutex_lock(&mpu->mutex);
	mpu->poll_time = (mpu->accel_poll) < (mpu->gyro_poll) ? (mpu->accel_poll) : (mpu->gyro_poll);
	mpu->input_dev->poll_interval = mpu->poll_time;
	mutex_unlock(&mpu->mutex);
}

static void icm42607_accel_gyro_calibration(struct icm42607_device *dev)
{
	int i = 0;
	int err = 0;
	long int sum_accel[3] = {0, 0, 0};
	long int sum_gyro[3] = {0, 0, 0};

	for (i = 0; i < ICM42607_CAPTURE_TIMES; i++) {
		err = icm42607_read_accel_xyz(dev);
		if (err < 0) {
			printk("in %s read accel data error\n", __func__);
        }

		err = icm42607_read_gyro_xyz(dev);
		if (err < 0) {
			printk("in %s read gyro data error\n", __func__);
        }

		sum_accel[0] += dev->accel_data[0];
		sum_accel[1] += dev->accel_data[1];
		sum_accel[2] += dev->accel_data[2];

		sum_gyro[0] += dev->gyro_data[0];
		sum_gyro[1] += dev->gyro_data[1];
		sum_gyro[2] += dev->gyro_data[2];

		inv_info("%d times, read accel data is %d %d %d, gyro data is %d %d %d \n",
			i,  dev->accel_data[0],  dev->accel_data[1],  dev->accel_data[2],
			dev->gyro_data[0], dev->gyro_data[1], dev->gyro_data[2]);
	}

	dev->accel_offset[0] = sum_accel[0]/ICM42607_CAPTURE_TIMES;
	dev->accel_offset[1] = sum_accel[1]/ICM42607_CAPTURE_TIMES;
	dev->accel_offset[2] = ICM42607_ACCEL_SENSITIVE - sum_accel[2]/ICM42607_CAPTURE_TIMES;

	dev->gyro_offset[0] = sum_gyro[0]/ICM42607_CAPTURE_TIMES;
	dev->gyro_offset[1] = sum_gyro[1]/ICM42607_CAPTURE_TIMES;
	dev->gyro_offset[2] = sum_gyro[2]/ICM42607_CAPTURE_TIMES;

	inv_info("accel offset is %d %d %d, gyro offset is %d %d %d \n",
		dev->accel_offset[0],  dev->accel_offset[1],  dev->accel_offset[2],
		dev->gyro_offset[0], dev->gyro_offset[1], dev->gyro_offset[2]);
}

static ssize_t icm42607_accel_enable_show(struct device *dev, struct device_attribute *attr,
				 char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;

	inv_info("%s: called\n", __func__);
	return sprintf(buf, "%d\n", mpu->accel_status);
}

static ssize_t icm42607_accel_enable_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;

	int changed = 0;
	unsigned long val;

    val = simple_strtoul(buf, NULL, 10);

	inv_info("in %s: data is %ld\n", __func__, val);

	changed = (val == mpu->accel_status) ? 0 : 1;

	if (changed) {
		printk("accel status changed, now state is %s\n", (val > 0) ? "active" : "disable");
		if (val) {
			icm42607_accel_enable(mpu);
			mpu->accel_status = 1;
		} else {
			icm42607_accel_disable(mpu);
			mpu->accel_status = 0;
		}
	}

	return count;
}

static ssize_t icm42607_accel_delay_show(struct device *dev, struct device_attribute *attr,
				 char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;

	inv_info("%s: called\n", __func__);
	return sprintf(buf, "%d\n", mpu->accel_poll);
}

static ssize_t icm42607_accel_delay_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;
	unsigned long val;

    val = simple_strtoul(buf, NULL, 10);
	inv_info("in %s: data is %ld\n", __func__, val);

	mpu->accel_poll = val;
	icm42607_reset_delay(mpu);
	return count;
}

static ssize_t icm42607_gyro_enable_show(struct device *dev, struct device_attribute *attr,
				 char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;

	inv_info("%s : called name %s\n", __func__, mpu->input_dev->input->name);
	return sprintf(buf, "%d\n", mpu->gyro_status);
}

static ssize_t icm42607_gyro_enable_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;
	int changed = 0;
	unsigned long val;

	val = simple_strtoul(buf, NULL, 10);
	inv_info("in %s: data is %ld\n", __func__, val);

	changed = (val == mpu->gyro_status) ? 0 : 1;

	if (changed) {
		printk("gyro status changed, now state is %s\n", (val > 0) ? "active" : "disable");
		if (val) {
			icm42607_gyro_enable(mpu);
			mpu->gyro_status = 1;
		} else {
			icm42607_gyro_disable(mpu);
			mpu->gyro_status = 0;
		}
	}

	return count;
}

static ssize_t icm42607_gyro_delay_show(struct device *dev, struct device_attribute *attr,
				 char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;

	inv_info("%s : called name %s\n", __func__, mpu->input_dev->input->name);
	return sprintf(buf, "%d\n", mpu->gyro_poll);
}

static ssize_t icm42607_gyro_delay_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;
	unsigned long val;

	val = simple_strtoul(buf, NULL, 10);
	inv_info("in %s: data is %ld\n",__func__,val);
	mpu->gyro_poll = val;
	icm42607_reset_delay(mpu);
	return count;
}

static ssize_t icm42607_device_delay_show(struct device *dev, struct device_attribute *attr,
				 char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;

	inv_info("%s : called name %s (delay: %d)\n", __func__, mpu->input_dev->input->name, mpu->poll_time);
	return sprintf(buf, "%d\n", mpu->input_dev->poll_interval);
}

static ssize_t icm42607_device_delay_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;
	unsigned long val;

	val = simple_strtoul(buf, NULL, 10);
	inv_info("in %s: data is %ld\n",__func__,val);
	mpu->poll_time = val;
	mpu->input_dev->poll_interval = mpu->poll_time;
	return count;
}

static ssize_t icm42607_calibration_show(struct device *dev, struct device_attribute *attr,
				 char *buf)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;

	inv_info("%s : called name %s\n", __func__, mpu->input_dev->input->name);

	return sprintf(buf, "AX%d AY%d AZ%d GX%d GY%d GZ%d\n", mpu->accel_offset[0], mpu->accel_offset[1], mpu->accel_offset[2],
				mpu->gyro_offset[0], mpu->gyro_offset[1], mpu->gyro_offset[2]);

}

static ssize_t icm42607_calibration_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	struct input_polled_dev *polldev = dev_get_drvdata(dev);
	struct icm42607_device *mpu = (struct icm42607_device *)polldev->private;
	unsigned long val;

	//before calibration, save accel and gyro status
	int pre_accel = mpu->accel_status;
	int pre_gyro = mpu->gyro_status;

	val = simple_strtoul(buf, NULL, 10);
	if (val != 1) {
		return count;
    }

	//close accel and gyro to calibration
	mpu->accel_status = 0;
	mpu->gyro_status = 0;

	if (pre_accel == 0) {
		icm42607_accel_enable(mpu);
	}

	if (pre_gyro == 0) {
		icm42607_gyro_enable(mpu);
	}

	icm42607_accel_gyro_calibration(mpu);

	//after calibration restore previous accel and gyro status
	mpu->accel_status = pre_accel;
	mpu->gyro_status = pre_gyro;

	if (pre_accel == 0) {
		icm42607_accel_disable(mpu);
	}

	if (pre_gyro == 0) {
		icm42607_gyro_disable(mpu);
	}

	return count;
}


static ssize_t icm42607_debug_show(struct device *dev, struct device_attribute *attr,
				 char *buf)
{
	inv_info("%s : called inv_debug is %d\n", __func__, inv_debug);
	return sprintf(buf, "%d\n", inv_debug);
}

static ssize_t icm42607_debug_store(struct device *dev, struct device_attribute *attr,
			    const char *buf, size_t count)
{
	unsigned long val;

	val = simple_strtoul(buf, NULL, 10);
	inv_info("in %s: data is %ld\n",__func__, val);
	inv_debug = val;
	return count;
}

/* S_IWUGO | S_IRUGO -> 0644 */
static DEVICE_ATTR(accel_enable, 0644, icm42607_accel_enable_show, icm42607_accel_enable_store);
static DEVICE_ATTR(accel_delay, 0644, icm42607_accel_delay_show, icm42607_accel_delay_store);
static DEVICE_ATTR(gyro_enable, 0644, icm42607_gyro_enable_show, icm42607_gyro_enable_store);
static DEVICE_ATTR(gyro_delay, 0644, icm42607_gyro_delay_show, icm42607_gyro_delay_store);
static DEVICE_ATTR(device_delay, 0644, icm42607_device_delay_show, icm42607_device_delay_store);
static DEVICE_ATTR(calibration, 0644, icm42607_calibration_show, icm42607_calibration_store);
static DEVICE_ATTR(debug, 0644, icm42607_debug_show, icm42607_debug_store);

static int icm42607_read_accel_xyz(struct icm42607_device *dev)
{
	struct i2c_client *client = dev->client;
	u8 buff[6];
	int err = 0;

	err = i2c_smbus_read_i2c_block_data(client, ICM42607_ACCEL_DATA_X1, 6, buff);
	if (err < 0) {
		printk("failed to read acc data, err=%d\n", err);
		return err;
	}

	dev->accel_data[0] = (short)(buff[0]<<8 | buff[1]);
	dev->accel_data[1] = (short)(buff[2]<<8 | buff[3]);
	dev->accel_data[2] = (short)(buff[4]<<8 | buff[5]);

	return 0;
}

static int icm42607_read_gyro_xyz(struct icm42607_device *dev)
{
	struct i2c_client *client = dev->client;
	u8 buff[6];
	int err = 0;

	err = i2c_smbus_read_i2c_block_data(client, ICM42607_GYRO_DATA_X1, 6, buff);
	if (err < 0) {
		printk("failed to read gyro data error=%d\n", err);
		return err;
	}

	dev->gyro_data[0] = (short)(buff[0]<<8 | buff[1]);
	dev->gyro_data[1] = (short)(buff[2]<<8 | buff[3]);
	dev->gyro_data[2] = (short)(buff[4]<<8 | buff[5]);

	return 0;
}

static void icm42607_read_sensors_data(struct icm42607_device *device)
{
	struct icm42607_device *dev = device;

	if (dev->accel_status) {
		icm42607_read_accel_xyz(dev);
		inv_info("accel data is read finished, %d  %d  %d\n", dev->accel_data[0],
			dev->accel_data[1], dev->accel_data[2]);
	}

	if (dev->gyro_status) {
		icm42607_read_gyro_xyz(dev);
		inv_info("gyro data is read finished,  %d  %d  %d\n", dev->gyro_data[0],
			dev->gyro_data[1], dev->gyro_data[2]);
	}
}

static void icm42607_report_value(struct icm42607_device *device)
{
	struct icm42607_device *dev = device;

	if (dev->accel_status) {
		input_report_abs(dev->input_dev->input,ABS_X, dev->accel_data[0]);
		input_report_abs(dev->input_dev->input,ABS_Y, dev->accel_data[1]);
		input_report_abs(dev->input_dev->input,ABS_Z, dev->accel_data[2]);
		input_sync(dev->input_dev->input);
	}

	if (dev->gyro_status) {
		input_report_abs(dev->input_dev->input,ABS_RX, dev->gyro_data[0]);
		input_report_abs(dev->input_dev->input,ABS_RY, dev->gyro_data[1]);
		input_report_abs(dev->input_dev->input,ABS_RZ, dev->gyro_data[2]);
		input_sync(dev->input_dev->input);
	}
}

static void icm42607_poll(struct input_polled_dev *dev)
{
	struct icm42607_device *mpu = (struct icm42607_device *)dev->private;

	icm42607_read_sensors_data(mpu);
	icm42607_report_value(mpu);
}

static int icm42607_device_init(struct i2c_client *client)
{
	int res = 0;
    int val = 0;

	res = icm42607_write(client, ICM42607_SIGNAL_PATH_RESET, 0x10);
	if (res) {
	    printk("ICM42607 soft reset error res=%d\n", res);
		return res;
	}
	msleep(100);

    val = icm42607_read(client, ICM42607_INT_STATUS);
    printk("ICM42607_INT_STATUS=%d\n", val);

    val = icm42607_read(client, ICM42607_WHO_AM_I);
    printk("ICM42607 whoami=%d\n", val);

	res = icm42607_write(client, ICM42607_INTF_CONFIG1, 0x01); //disable i3c sdr/ddr
	if (res) {
		printk("ICM42607 config gyro error res=%d!\n", res);
		return res;
	}

	res = icm42607_write(client, ICM42607_GYRO_CONFIG0, 0x0B); //config gyro for 2000dps 25Hz
	if (res) {
		printk("ICM42607 config gyro error res=%d!\n", res);
		return res;
	}

	res = icm42607_write(client, ICM42607_ACCEL_CONFIG0, 0x6B); //config Accel for 2g 25Hz
	if (res) {
		printk("ICM42607 config accel error res=%d!\n", res);
		return res;
	}

    val = icm42607_read(client, ICM42607_GYRO_CONFIG0);
    printk("ICM42607_GYRO_CONFIG0=%d\n", val);
    val = icm42607_read(client, ICM42607_ACCEL_CONFIG0);
    printk("ICM42607_ACCEL_CONFIG0=%d\n", val);
	return 0;
}

static struct attribute *icm42607_sysfs_attrs[] = {
	&dev_attr_accel_enable.attr,
	&dev_attr_accel_delay.attr,
	&dev_attr_gyro_enable.attr,
	&dev_attr_gyro_delay.attr,
	&dev_attr_device_delay.attr,
	&dev_attr_calibration.attr,
	&dev_attr_debug.attr,
	NULL
};

struct attribute_group icm42607_attrs = {
	.attrs = icm42607_sysfs_attrs,
};

int icm42607_probe(struct i2c_client *client, const struct i2c_device_id *devid)
{
	struct icm42607_device *mpu;
	int err;
    printk("icm42607_probe start\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk("platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	mpu = kzalloc(sizeof(struct icm42607_device), GFP_KERNEL);
	if (!mpu) {
		printk("failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	mutex_init(&mpu->mutex);
	i2c_set_clientdata(client, mpu);

	err  = icm42607_device_init(client);
	if (err) {
		printk("icm42607_device_init fail\n");
		goto exit_input_dev_alloc_failed;
	}
	mpu->client = client;
	mpu->input_dev = input_allocate_polled_device();
	if (!mpu->input_dev) {
		err = -ENOMEM;
		printk("input_allocate_polled_device failed\n");
		goto exit_input_dev_alloc_failed;
	}

	set_bit(EV_ABS, mpu->input_dev->input->evbit);

	/* x-axis acceleration */
	input_set_abs_params(mpu->input_dev->input, ABS_X, -32768, 32767, 0, 0);
	/* y-axis acceleration */
	input_set_abs_params(mpu->input_dev->input, ABS_Y, -32768, 32767, 0, 0);
	/* z-axis acceleration */
	input_set_abs_params(mpu->input_dev->input, ABS_Z, -32768, 32767, 0, 0);

	/* Gyro X-axis */
	input_set_abs_params(mpu->input_dev->input, ABS_RX, -32768, 32767, 0, 0);
	/* Gyro Y-axis */
	input_set_abs_params(mpu->input_dev->input, ABS_RY, -32768, 32767, 0, 0);
	/* Gyro Z-axis */
	input_set_abs_params(mpu->input_dev->input, ABS_RZ, -32768, 32767, 0, 0);
	mpu->input_dev->private = mpu;
	mpu->input_dev->input->name	= MPU_DEVICE_NAME;
	mpu->input_dev->input->id.bustype = BUS_I2C;
	mpu->input_dev->poll = icm42607_poll;
	mpu->input_dev->poll_interval = ICM42607_POLL_INTERVAL;
	/* diff */
	/*
	mpu->input_dev->poll_interval_max = ICM42607_POLL_INTERVAL_MAX;
	mpu->input_dev->poll_interval_min = ICM42607_POLL_INTERVAL_MIN;*/

	mpu->input_dev->poll_interval = ICM42607_POLL_INTERVAL;

	mpu->accel_poll = ICM42607_POLL_INTERVAL;
	mpu->gyro_poll = ICM42607_POLL_INTERVAL;

	err = input_register_polled_device(mpu->input_dev);
	if (err) {
		printk("icm42607_probe: Unable to register input device: %s\n",
					 mpu->input_dev->input->name);
		goto exit_input_register_device_failed;
	}

    //mpu->input_dev->private = mpu;
    //mpu->input_dev->close(mpu->input_dev);
	if (sysfs_create_group(&mpu->input_dev->input->dev.kobj, &icm42607_attrs)) {
		printk("icm42607_probe create sysfile error\n");
		goto exit_input_register_device_failed;
	}

	printk("icm42607 probe successfully\n");
	return 0;

exit_input_register_device_failed:
	input_unregister_polled_device(mpu->input_dev);
exit_input_dev_alloc_failed:
	kfree(mpu);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

/* diff */
static void icm42607_remove(struct i2c_client *client)
{
	struct icm42607_device *mpu = i2c_get_clientdata(client);
	input_unregister_polled_device(mpu->input_dev);
	kfree(mpu);
}

static const struct i2c_device_id icm42607_id[] = {
	{"icm42607",0},
	{}
};

MODULE_DEVICE_TABLE(i2c, icm42607_id);

static const struct of_device_id icm42607_match[] = {
    {.compatible = "invn,icm42607",},
	{}
};
MODULE_DEVICE_TABLE(of, icm42607_match);

static struct i2c_driver icm42607_driver = {
	.probe = icm42607_probe,
	.remove = icm42607_remove,
	.id_table = icm42607_id,
	.driver = {
		.owner = THIS_MODULE,
		.of_match_table = icm42607_match,
		.name = "icm42607",
	},
};

static int __init icm42607_init(void)
{
	printk("icm42607 driver: init\n");
	return i2c_add_driver(&icm42607_driver);
}

static void __exit icm42607_exit(void)
{
	printk("icm42607 driver: exit\n");
	i2c_del_driver(&icm42607_driver);
}

module_init(icm42607_init);
module_exit(icm42607_exit);

MODULE_AUTHOR("Invensense Corporation");
MODULE_DESCRIPTION("Invensense I2C device driver");
MODULE_LICENSE("GPL");


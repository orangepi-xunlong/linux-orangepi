/* For AllWinner android platform.
 *
 * msa_cust.c - Linux kernel modules for 3-Axis Accelerometer
 *
 * Copyright (C) 2007-2016 MEMS Sensing Technology Co., Ltd.
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

//#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include "../../init-input.h"


#include "msa_core.h"
#include "msa_cust.h"

#define MSA_DEVICE_NAME			"msa"
#define MSA_DRV_NAME			"msa"
#define MSA_INPUT_DEV_NAME	 	MSA_DRV_NAME
#define MSA_MISC_NAME			MSA_DRV_NAME

#define POLL_INTERVAL_MAX		500
#define POLL_INTERVAL			39
#define INPUT_FUZZ				0
#define INPUT_FLAT				0

static int msa_startup(void);

static const unsigned short normal_i2c[] = {0x62, I2C_CLIENT_END};

static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
	.np_name = MSA_DEVICE_NAME,
};

static struct input_dev		*msa_idev;
// static struct device		*hwmon_dev;
static struct hrtimer		hr_timer;
static ktime_t 				myktime;
static struct work_struct 	wq_hrtimer;
static MSA_HANDLE			msa_handle;

static unsigned int			delayMs = POLL_INTERVAL;
static unsigned char 		twi_id;

extern int					Log_level;

#define MI_DATA(format, ...) 	\
	do {\
		if (DEBUG_DATA & Log_level) {\
			printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);\
		} \
	} while (0)
#define MI_MSG(format, ...)		\
	do {\
		if (DEBUG_MSG & Log_level) {\
			printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);\
		} \
	} while (0)
#define MI_ERR(format, ...)		\
	do {\
		if (DEBUG_ERR & Log_level) {\
			printk(KERN_ERR MI_TAG format "\n", ## __VA_ARGS__);\
		} \
	} while (0)
#define MI_FUN					\
	do {\
		if (DEBUG_FUNC & Log_level) {\
			printk(KERN_ERR MI_TAG "%s is called, line: %d\n", __FUNCTION__, __LINE__);\
		} \
	} while (0)
#define MI_ASSERT(expr)				 \
	do {\
		if (!(expr)) {\
			printk(KERN_ERR "Assertion failed! %s,%d,%s,%s\n",\
				__FILE__, __LINE__, __func__, #expr);\
		} \
	} while (0)


/*----------------------------------------------------------------------------*/
#if MSA_OFFSET_TEMP_SOLUTION
static char OffsetFileName[] = "/data/misc/msaGSensorOffset.txt";
static char OffsetFolerName[] = "/data/misc/";
#define OFFSET_STRING_LEN			   26
struct work_info {
	char		tst1[20];
	char		tst2[20];
	char		buffer[OFFSET_STRING_LEN];
	struct	  workqueue_struct *wq;
	struct	  delayed_work read_work;
	struct	  delayed_work write_work;
	struct	  completion completion;
	int		 len;
	int		 rst;
};

static struct work_info m_work_info = {{0}};
/*----------------------------------------------------------------------------*/
static void sensor_write_work(struct work_struct *work)
{
	struct work_info *pWorkInfo;
	struct file		 *filep;
	mm_segment_t		orgfs;
	int				 ret;

	orgfs = get_fs();
	set_fs(KERNEL_DS);

	pWorkInfo = container_of((struct delayed_work *)work, struct work_info, write_work);
	if (pWorkInfo == NULL) {
		MI_ERR("get pWorkInfo failed!");
		return;
	}

	filep = filp_open(OffsetFileName, O_RDWR|O_CREAT, 0600);
	if (IS_ERR(filep)) {
		MI_ERR("write, sys_open %s error!!.\n", OffsetFileName);
		ret =  -1;
	} else {
		filep->f_op->write(filep, pWorkInfo->buffer, pWorkInfo->len, &filep->f_pos);
		filp_close(filep, NULL);
		ret = 0;
	}

	set_fs(orgfs);
	pWorkInfo->rst = ret;
	complete(&pWorkInfo->completion);
}
/*----------------------------------------------------------------------------*/
static void sensor_read_work(struct work_struct *work)
{
	mm_segment_t orgfs;
	struct file *filep;
	int ret;
	struct work_info *pWorkInfo;

	orgfs = get_fs();
	set_fs(KERNEL_DS);

	pWorkInfo = container_of((struct delayed_work *)work, struct work_info, read_work);
	if (pWorkInfo == NULL) {
		MI_ERR("get pWorkInfo failed!");
		return;
	}

	filep = filp_open(OffsetFileName, O_RDONLY, 0600);
	if (IS_ERR(filep)) {
		MI_ERR("read, sys_open %s error!!.\n", OffsetFileName);
		set_fs(orgfs);
		ret =  -1;
	} else {
		filep->f_op->read(filep, pWorkInfo->buffer,  sizeof(pWorkInfo->buffer), &filep->f_pos);
		filp_close(filep, NULL);
		set_fs(orgfs);
		ret = 0;
	}

	pWorkInfo->rst = ret;
	complete(&(pWorkInfo->completion));
}
/*----------------------------------------------------------------------------*/
static int sensor_sync_read(u8 *offset)
{
	int	 err;
	int	 off[MSA_OFFSET_LEN] = {0};
	struct work_info *pWorkInfo = &m_work_info;

	init_completion(&pWorkInfo->completion);
	queue_delayed_work(pWorkInfo->wq, &pWorkInfo->read_work, msecs_to_jiffies(0));
	err = wait_for_completion_timeout(&pWorkInfo->completion, msecs_to_jiffies(2000));
	if (err == 0) {
		MI_ERR("wait_for_completion_timeout TIMEOUT");
		return -1;
	}

	if (pWorkInfo->rst != 0) {
		MI_ERR("work_info.rst  not equal 0");
		return pWorkInfo->rst;
	}

	sscanf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x", &off[0], &off[1], &off[2],
			&off[3], &off[4], &off[5], &off[6], &off[7], &off[8]);

	offset[0] = (u8)off[0];
	offset[1] = (u8)off[1];
	offset[2] = (u8)off[2];
	offset[3] = (u8)off[3];
	offset[4] = (u8)off[4];
	offset[5] = (u8)off[5];
	offset[6] = (u8)off[6];
	offset[7] = (u8)off[7];
	offset[8] = (u8)off[8];

	return 0;
}
/*----------------------------------------------------------------------------*/
static int sensor_sync_write(u8 *off)
{
	int err = 0;
	struct work_info *pWorkInfo = &m_work_info;

	init_completion(&pWorkInfo->completion);

	sprintf(m_work_info.buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x\n", off[0], off[1], off[2],
			off[3], off[4], off[5], off[6], off[7], off[8]);

	pWorkInfo->len = sizeof(m_work_info.buffer);

	queue_delayed_work(pWorkInfo->wq, &pWorkInfo->write_work, msecs_to_jiffies(0));
	err = wait_for_completion_timeout(&pWorkInfo->completion, msecs_to_jiffies(2000));
	if (err == 0) {
		MI_ERR("wait_for_completion_timeout TIMEOUT");
		return -1;
	}

	if (pWorkInfo->rst != 0) {
		MI_ERR("work_info.rst  not equal 0");
		return pWorkInfo->rst;
	}

	return 0;
}
/*----------------------------------------------------------------------------*/
static int check_califolder_exist(void)
{
	mm_segment_t	 orgfs;
	struct  file *filep;

	orgfs = get_fs();
	set_fs(KERNEL_DS);

	filep = filp_open(OffsetFolerName, O_RDONLY, 0600);
	if (IS_ERR(filep)) {
		MI_ERR("%s read, sys_open %s error!!.\n", __func__, OffsetFolerName);
		set_fs(orgfs);
		return 0;
	}

	filp_close(filep, NULL);
	set_fs(orgfs);

	return 1;
}
/*----------------------------------------------------------------------------*/
static int support_fast_auto_cali(void)
{
#if MSA_SUPPORT_FAST_AUTO_CALI
	return 1;
#else
	return 0;
#endif
}
#endif
/*----------------------------------------------------------------------------*/
static int get_address(PLAT_HANDLE handle)
{
	if (NULL == handle) {
		MI_ERR("chip init failed !\n");
		return -1;
	}

	return ((struct i2c_client *)handle)->addr;
}
/*----------------------------------------------------------------------------*/
static void report_abs(void)
{
	short x = 0, y = 0, z = 0;
	MSA_HANDLE handle = msa_handle;

	if (msa_read_data(handle, &x, &y, &z) != 0) {
		MI_ERR("MSA data read failed!\n");
		return;
	}
//	printk("x = %d, y = %d, z = %d \n",x,y,z);
	input_report_abs(msa_idev, ABS_X, x);
	input_report_abs(msa_idev, ABS_Y, y);
	input_report_abs(msa_idev, ABS_Z, z);
	input_sync(msa_idev);
}
/*----------------------------------------------------------------------------*/
#if 0
static void msa_dev_poll(struct input_polled_dev *dev)
{
	dev->poll_interval = delayMs;
	report_abs();
}
#endif
/*----------------------------------------------------------------------------*/
static long msa_misc_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	void __user	 *argp = (void __user *)arg;
	int			 err = 0;
	int			 interval = 0;
	char			bEnable = 0;
	short		   xyz[3] = {0};
	MSA_HANDLE	  handle = msa_handle;

	if (_IOC_DIR(cmd) & _IOC_READ) {
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	} else if (_IOC_DIR(cmd) & _IOC_WRITE) {
		err = !access_ok((void __user *)arg, _IOC_SIZE(cmd));
	}

	if (err) {
		return -EFAULT;
	}

	switch (cmd) {
	case MSA_ACC_IOCTL_GET_DELAY:
		interval = POLL_INTERVAL;
		if (copy_to_user(argp, &interval, sizeof(interval)))
			return -EFAULT;
		break;

	case MSA_ACC_IOCTL_SET_DELAY:
		if (copy_from_user(&interval, argp, sizeof(interval)))
			return -EFAULT;
		if (interval < 0 || interval > 1000) {
			MI_ERR("msa_misc_ioctl interval 大于1000\n");
			return -EINVAL;
		}
		//if((interval <=30)&&(interval > 10))
		//{
		//	interval = 10;
		//}

		if ((interval < 20)) {
			//if(interval <= 10) {
			//	delayMs = 3;
			//} else {
				delayMs = 7;
			//}
		} else {
			delayMs = interval - 1;
		}
		MI_ERR("msa_misc_ioctl interval = %d, delayMs = %d\n", interval, delayMs);
		//delayMs = interval;
		break;

	case MSA_ACC_IOCTL_SET_ENABLE:
		if (copy_from_user(&bEnable, argp, sizeof(bEnable)))
			return -EFAULT;

		err = msa_set_enable(handle, bEnable);
		if (err < 0)
			return EINVAL;
		break;

	case MSA_ACC_IOCTL_GET_ENABLE:
		err = msa_get_enable(handle, &bEnable);
		if (err < 0) {
			return -EINVAL;
		}

		if (copy_to_user(argp, &bEnable, sizeof(bEnable)))
				return -EINVAL;
		break;

#if MSA_OFFSET_TEMP_SOLUTION
	case MSA_ACC_IOCTL_CALIBRATION:
		int z_dir = 0;
		if (copy_from_user(&z_dir, argp, sizeof(z_dir)))
			return -EFAULT;

		if (msa_calibrate(handle, z_dir)) {
			return -EFAULT;
		}

		if (copy_to_user(argp, &z_dir, sizeof(z_dir)))
			return -EFAULT;
		break;

	case MSA_ACC_IOCTL_UPDATE_OFFSET:
		manual_load_cali_file(handle);
		break;
#endif

	case MSA_ACC_IOCTL_GET_COOR_XYZ:

		if (msa_read_data(handle, &xyz[0], &xyz[1], &xyz[2]))
			return -EFAULT;

		if (copy_to_user((void __user *)arg, xyz, sizeof(xyz)))
			return -EFAULT;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/*----------------------------------------------------------------------------*/
static const struct file_operations msa_misc_fops = {
		.owner = THIS_MODULE,
		.unlocked_ioctl = msa_misc_ioctl,
};

static struct miscdevice misc_msa = {
		.minor = MISC_DYNAMIC_MINOR,
		.name = MSA_MISC_NAME,
		.fops = &msa_misc_fops,
};
/*----------------------------------------------------------------------------*/
static ssize_t msa_enable_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int			 ret;
	char			bEnable;
	MSA_HANDLE	  handle = msa_handle;

	ret = msa_get_enable(handle, &bEnable);
	if (ret < 0) {
		ret = -EINVAL;
	} else {
		ret = sprintf(buf, "%d\n", bEnable);
	}

	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_enable_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	int			 ret;
	char			bEnable;
	unsigned long   enable;
	MSA_HANDLE	  handle = msa_handle;

	if (buf == NULL) {
		return -1;
	}

	enable = simple_strtoul(buf, NULL, 10);
	bEnable = (enable > 0) ? 1 : 0;

	ret = msa_set_enable(handle, bEnable);
	if (ret < 0) {
		ret = -EINVAL;
	} else {
		ret = count;
	}

	if (bEnable) {
		myktime = ns_to_ktime(delayMs * NSEC_PER_MSEC);
		hrtimer_start(&hr_timer, myktime, HRTIMER_MODE_REL);
	} else {
		hrtimer_cancel(&hr_timer);
	}

	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", delayMs + 1);
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_delay_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int interval = 0;

	interval = simple_strtoul(buf, NULL, 10);

	if (interval < 10) {
		interval = 10;
	} else if (interval > 1000) {
		interval = 1000;
	}

	delayMs = interval - 1;

	MI_ERR("msa_delay_store interval = %d, delayMs = %d\n", interval, delayMs);

	myktime = ns_to_ktime(delayMs * NSEC_PER_MSEC);

	//myktime = ktime_set(0, delayMs* NSEC_PER_MSEC);
	hrtimer_start(&hr_timer, myktime, HRTIMER_MODE_REL);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_axis_data_show(struct device *dev,
		   struct device_attribute *attr, char *buf)
{
	int result;
	short x, y, z;
	int count = 0;
	MSA_HANDLE	  handle = msa_handle;

	result = msa_read_data(handle, &x, &y, &z);
	if (result == 0)
		count += sprintf(buf+count, "x= %d;y=%d;z=%d\n", x, y, z);
	else
		count += sprintf(buf+count, "reading failed!");

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_reg_data_store(struct device *dev,
		   struct device_attribute *attr, const char *buf, size_t count)
{
	int				 addr, data;
	int				 result;
	MSA_HANDLE		  handle = msa_handle;

	sscanf(buf, "0x%x, 0x%x\n", &addr, &data);

	result = msa_register_write(handle, addr, data);

	MI_ASSERT(result == 0);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_reg_data_show(struct device *dev,
		   struct device_attribute *attr, char *buf)
{
	MSA_HANDLE		  handle = msa_handle;

	return msa_get_reg_data(handle, buf);
}
/*----------------------------------------------------------------------------*/
#if 0
static ssize_t msa_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	//if(bLoad==FILE_EXIST)
	//	count += sprintf(buf,"%s",m_work_info.buffer);
	//else
		count += sprintf(buf, "%s", "Calibration file not exist!\n");

	return count;
}
#endif
/*----------------------------------------------------------------------------*/
#if FILTER_AVERAGE_ENHANCE
static ssize_t msa_average_enhance_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int							 ret = 0;
	struct msa_filter_param_s	param = {0};

	ret = msa_get_filter_param(&param);
	ret |= sprintf(buf, "%d %d %d\n", param.filter_param_l, param.filter_param_h, param.filter_threhold);

	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_average_enhance_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int							 ret = 0;
	struct msa_filter_param_s	param = {0};

	sscanf(buf, "%d %d %d\n", &param.filter_param_l, &param.filter_param_h, &param.filter_threhold);

	ret = msa_set_filter_param(&param);

	return count;
}
#endif
/*----------------------------------------------------------------------------*/
#if MSA_OFFSET_TEMP_SOLUTION
int bCaliResult = -1;
static ssize_t msa_calibrate_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", bCaliResult);
	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_calibrate_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	s8			  z_dir = 0;
	MSA_HANDLE	  handle = msa_handle;

	z_dir = simple_strtol(buf, NULL, 10);
	bCaliResult = msa_calibrate(handle, z_dir);

	return count;
}
#endif
/*----------------------------------------------------------------------------*/
static ssize_t msa_log_level_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int ret;

	ret = sprintf(buf, "%d\n", Log_level);

	return ret;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_log_level_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	Log_level = simple_strtoul(buf, NULL, 10);

	return count;
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_primary_offset_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	MSA_HANDLE   handle = msa_handle;
	int x = 0, y = 0, z = 0;

	msa_get_primary_offset(handle, &x, &y, &z);

	return sprintf(buf, "x=%d ,y=%d ,z=%d\n", x, y, z);

}
/*----------------------------------------------------------------------------*/
static ssize_t msa_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{

	return sprintf(buf, "%s_%s\n", DRI_VER, CORE_VER);
}
/*----------------------------------------------------------------------------*/
static ssize_t msa_vendor_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", "MsaMEMS");
}
/*----------------------------------------------------------------------------*/
static DEVICE_ATTR(enable,		  0644,  msa_enable_show,			 msa_enable_store);
static DEVICE_ATTR(delay,	  0644,  msa_delay_show,			  msa_delay_store);
static DEVICE_ATTR(axis_data,	   S_IRUGO |S_IWUSR | S_IWGRP,	msa_axis_data_show,		  NULL);
static DEVICE_ATTR(reg_data,		S_IWUSR | S_IWGRP | S_IRUGO,  msa_reg_data_show,		   msa_reg_data_store);
static DEVICE_ATTR(log_level,	   S_IWUSR | S_IWGRP | S_IRUGO,  msa_log_level_show,		  msa_log_level_store);
#if MSA_OFFSET_TEMP_SOLUTION
static DEVICE_ATTR(offset,		  S_IWUSR | S_IWGRP | S_IRUGO,  msa_offset_show,			 NULL);
static DEVICE_ATTR(calibrate_msaGSensor,	   S_IWUSR | S_IWGRP | S_IRUGO,  msa_calibrate_show,		  msa_calibrate_store);
#endif
#if FILTER_AVERAGE_ENHANCE
static DEVICE_ATTR(average_enhance, S_IWUSR | S_IWGRP | S_IRUGO,  msa_average_enhance_show,	msa_average_enhance_store);
#endif
static DEVICE_ATTR(primary_offset,  S_IWUSR | S_IWGRP,			msa_primary_offset_show,			NULL);
static DEVICE_ATTR(version,		 S_IRUGO,			msa_version_show,			NULL);
static DEVICE_ATTR(vendor,		  S_IRUGO,			msa_vendor_show,			 NULL);
/*----------------------------------------------------------------------------*/
static struct attribute *msa_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_delay.attr,
	&dev_attr_axis_data.attr,
	&dev_attr_reg_data.attr,
	&dev_attr_log_level.attr,
#if MSA_OFFSET_TEMP_SOLUTION
	&dev_attr_offset.attr,
	&dev_attr_calibrate_msaGSensor.attr,
#endif
#if FILTER_AVERAGE_ENHANCE
	&dev_attr_average_enhance.attr,
#endif /* ! FILTER_AVERAGE_ENHANCE */
	&dev_attr_primary_offset.attr,
	&dev_attr_version.attr,
	&dev_attr_vendor.attr,
	NULL
};

static const struct attribute_group msa_attr_group = {
	.attrs  = msa_attributes,
};
/*----------------------------------------------------------------------------*/
int i2c_smbus_read(PLAT_HANDLE handle, u8 addr, u8 *data)
{
	int				 res = 0;
	struct i2c_client   *client = (struct i2c_client *)handle;

	*data = i2c_smbus_read_byte_data(client, addr);

	return res;
}
/*----------------------------------------------------------------------------*/
int i2c_smbus_read_block(PLAT_HANDLE handle, u8 addr, u8 count, u8 *data)
{
	int				 res = 0;
	struct i2c_client   *client = (struct i2c_client *)handle;

	res = i2c_smbus_read_i2c_block_data(client, addr, count, data);

	return res;
}
/*----------------------------------------------------------------------------*/
int i2c_smbus_write(PLAT_HANDLE handle, u8 addr, u8 data)
{
	int				 res = 0;
	struct i2c_client   *client = (struct i2c_client *)handle;

	res = i2c_smbus_write_byte_data(client, addr, data);

	return res;
}
/*----------------------------------------------------------------------------*/
void msdelay(int ms)
{
	mdelay(ms);
}

static void wq_func_hrtimer(struct work_struct *work)
{
	report_abs();
}

static enum hrtimer_restart my_hrtimer_callback(struct hrtimer *timer)
{
	schedule_work(&wq_hrtimer);
	hrtimer_forward_now(&hr_timer, myktime);
	return HRTIMER_RESTART;
}

#if MSA_OFFSET_TEMP_SOLUTION
MSA_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write,
		sensor_sync_write, sensor_sync_read, check_califolder_exist, get_address,
		support_fast_auto_cali, msdelay, sprintf);
#else
MSA_GENERAL_OPS_DECLARE(ops_handle, i2c_smbus_read, i2c_smbus_read_block, i2c_smbus_write,
		NULL, NULL, NULL, get_address, NULL, msdelay,  sprintf);
#endif


static int i2c_write_bytes(struct i2c_client *client,  uint8_t *data,  uint16_t len)
{
		struct i2c_msg msg;
		int ret = -1;

		msg.flags = !I2C_M_RD;
		msg.addr = client->addr;
	msg.len = len;
		msg.buf = data;

		ret = i2c_transfer(client->adapter,  &msg,  1);
		return ret;
}

static bool gsensor_i2c_test(struct i2c_client *client)
{
		int ret,  retry;
		uint8_t test_data[1] = { 0 };   //only write a data address.

		for (retry = 0; retry < 2; retry++) {
				ret = i2c_write_bytes(client, test_data, 1);     //Test i2c.
				if (ret == 1)
					break;
				msleep(5);
		}

		return ret == 1 ? true : false;
}


/*----------------------------------------------------------------------------*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0))
static int msa_probe(struct i2c_client *client)
#else
static int msa_probe(struct i2c_client *client,  const struct i2c_device_id *id)
#endif
{
	int	result = -1;
	struct input_dev	*idev;
	unsigned char chip_id = 0;
	unsigned char i = 0;

	if (gsensor_info.dev == NULL)
		gsensor_info.dev  =  &client->dev;

	if (client->addr !=  0x62) {
		client->addr = 0x62;//water force i2c

	}

	if (msa_install_general_ops(&ops_handle)) {
		MI_ERR("Install ops failed !\n");
		goto err_detach_client;
	}

#if MSA_OFFSET_TEMP_SOLUTION
	m_work_info.wq  =  create_singlethread_workqueue("oo");
	if (NULL  ==  m_work_info.wq) {
		MI_ERR("Failed to create workqueue !");
		goto err_detach_client;
	}

	INIT_DELAYED_WORK(&m_work_info.read_work,  sensor_read_work);
	INIT_DELAYED_WORK(&m_work_info.write_work,  sensor_write_work);
#endif

	i2c_smbus_read((PLAT_HANDLE) client,  NSA_REG_WHO_AM_I,  &chip_id);
	if (chip_id !=  0x13) {
		for (i = 0; i < 5; i++) {
			mdelay(5);
			i2c_smbus_read((PLAT_HANDLE) client,  NSA_REG_WHO_AM_I,  &chip_id);
			if (chip_id  ==  0x13)
				break;
		}
	}

	/* Initialize the MSA chip */
	msa_handle  =  msa_core_init((PLAT_HANDLE)client);
	if (NULL  ==  msa_handle) {
		MI_ERR("chip init failed !\n");
		input_set_power_enable(&(gsensor_info.input_type),  0);
		result = -ENODEV;
		goto err_detach_client;
	}

	//hwmon_dev  =  hwmon_device_register(&client->dev);
	//MI_ASSERT(!(IS_ERR(hwmon_dev)));

	/* input poll device register */
	//msa_idev  =  input_allocate_polled_device();
	result = gsensor_i2c_test(client);
	if (!result) {
		input_set_power_enable(&(gsensor_info.input_type),  0);
		pr_info("%s:I2C connection might be something wrong or maybe the other gsensor equipment! \n", __func__);
		return -ENODEV;
	}
	msa_idev  =  input_allocate_device();
	if (!msa_idev) {
		MI_ERR("alloc poll device failed!\n");
		result  =  -ENOMEM;
		goto err_hwmon_device_unregister;
	}

	//msa_idev->poll  =  msa_dev_poll;
	//msa_idev->poll_interval  =  POLL_INTERVAL;
	delayMs  =  POLL_INTERVAL;
	//msa_idev->poll_interval_max  =  POLL_INTERVAL_MAX;
	idev  =  msa_idev;

	idev->name  =  MSA_INPUT_DEV_NAME;
	idev->id.bustype  =  BUS_I2C;
	idev->evbit[0]  =  BIT_MASK(EV_ABS);

	input_set_abs_params(idev,   ABS_X,   -16384,   16383,   INPUT_FUZZ,   INPUT_FLAT);
	input_set_abs_params(idev,   ABS_Y,   -16384,   16383,   INPUT_FUZZ,   INPUT_FLAT);
	input_set_abs_params(idev,   ABS_Z,   -16384,   16383,   INPUT_FUZZ,   INPUT_FLAT);

	//result  =  input_register_polled_device(msa_idev);
	result  =  input_register_device(msa_idev);
	if (result) {
		MI_ERR("register poll device failed!\n");
		goto err_free_polled_device;
	}

	/* Sys Attribute Register */
	result  =  sysfs_create_group(&idev->dev.kobj,   &msa_attr_group);
	if (result) {
		MI_ERR("create device file failed!\n");
		result  =  -EINVAL;
		goto err_unregister_polled_device;
	}

	kobject_uevent(&idev->dev.kobj,   KOBJ_CHANGE);

	/* Misc device interface Register */
	result  =  misc_register(&misc_msa);
	if (result) {
		MI_ERR("%s: msa_dev register failed",   __func__);
		goto err_remove_sysfs_group;
	}


	//start timer
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr_timer.function  =  my_hrtimer_callback;
	myktime  =  ns_to_ktime(POLL_INTERVAL *NSEC_PER_MSEC);
	INIT_WORK(&wq_hrtimer, wq_func_hrtimer);

	return result;

err_remove_sysfs_group:
	sysfs_remove_group(&idev->dev.kobj, &msa_attr_group);
err_unregister_polled_device:
	//input_unregister_polled_device(msa_idev);
	input_unregister_device(msa_idev);
err_free_polled_device:
	//input_free_polled_device(msa_idev);
	input_free_device(msa_idev);
err_hwmon_device_unregister:
	//hwmon_device_unregister(&client->dev);
err_detach_client:
	return result;
}
/*----------------------------------------------------------------------------*/
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
static void msa_remove(struct i2c_client *client)
#else
static int msa_remove(struct i2c_client *client)
#endif
{
	MSA_HANDLE  handle = msa_handle;

	msa_set_enable(handle, 0);

	misc_deregister(&misc_msa);

	sysfs_remove_group(&msa_idev->dev.kobj, &msa_attr_group);

	//input_unregister_polled_device(msa_idev);
	input_unregister_device(msa_idev);

	//input_free_polled_device(msa_idev);
#if MSA_OFFSET_TEMP_SOLUTION
	flush_workqueue(m_work_info.wq);

	destroy_workqueue(m_work_info.wq);
#endif
	//hwmon_device_unregister(hwmon_dev);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0))
	return;
#else
	return 0;
#endif
}
/*----------------------------------------------------------------------------*/
static int msa_suspend(struct device *dev)
{
	int result  =  0;
	MSA_HANDLE	handle = msa_handle;

	MI_FUN;

	result = msa_set_enable(handle,   0);
	if (result) {
		MI_ERR("%s:set disable fail!!\n",   __func__);
		return result;
	}
	//msa_idev->close(msa_idev);

	input_set_power_enable(&(gsensor_info.input_type),   0);
	return result;
}
/*----------------------------------------------------------------------------*/
static int msa_resume(struct device *dev)
{
	int result = 0;
	MSA_HANDLE	handle = msa_handle;

	MI_FUN;
	input_set_power_enable(&(gsensor_info.input_type),   1);
	result  =  msa_chip_resume(handle);
	if (result) {
		MI_ERR("%s:chip resume fail!!\n", __func__);
		return result;
	}

	result  =  msa_set_enable(handle, 1);
	if (result) {
		MI_ERR("%s:set enable fail!!\n", __func__);
		return result;
	}

	//msa_idev->open(msa_idev);

	return result;
}

#if IS_ENABLED(CONFIG_PM)
static UNIVERSAL_DEV_PM_OPS(msa_pm_ops, msa_suspend,
		msa_resume, NULL);
#endif
/*----------------------------------------------------------------------------*/
static int msa_detect(struct i2c_client *new_client,
		struct i2c_board_info *info)
{
	struct i2c_adapter *adapter  =  new_client->adapter;

	MI_MSG("%s:bus[%d] addr[0x%x]\n", __func__, adapter->nr, new_client->addr);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		//input_set_power_enable(&(gsensor_info.input_type),   0);
		return -ENODEV;
	}

	if (twi_id  ==  adapter->nr) {
		if (msa_install_general_ops(&ops_handle)) {
			MI_ERR("Install ops failed !\n");
			return -ENODEV;
		}

		if (msa_module_detect((PLAT_HANDLE)new_client)) {
			MI_ERR("%s: Can't find Msa gsensor!!", __func__);
			input_set_power_enable(&(gsensor_info.input_type),   0);
		} else {
			MI_ERR("'Find Msa gsensor!!");
			strlcpy(info->type,   MSA_DRV_NAME,   I2C_NAME_SIZE);
			return 0;
		}
	}
	return  -ENODEV;
}
/*----------------------------------------------------------------------------*/


static int msa_startup(void)
{
	int ret  =  -1;

	printk("function = %s =  =  =  =  =  =  =  =  = LINE = %d. \n",   __func__,   __LINE__);
#ifdef MSA_DEVICE_NAME
	printk("msa use sepical device node name  =  %s\n",   MSA_DEVICE_NAME);
	gsensor_info.node  =  of_find_node_by_name(NULL,   MSA_DEVICE_NAME);
#endif

	if (input_sensor_startup(&(gsensor_info.input_type))) {
		printk("%s: err.\n",   __func__);
		return -1;
	} else
		ret  =  input_sensor_init(&(gsensor_info.input_type));

	if (0 != ret) {
		printk("%s:gsensor.init_platform_resource err. \n",   __func__);
	}

	twi_id  =  gsensor_info.twi_id;
	input_set_power_enable(&(gsensor_info.input_type),   1);
	return 0;
}

/*----------------------------------------------------------------------------*/
static const struct of_device_id msa_of_match[]  =  {
	{.compatible  =  "allwinner,  msa"},
	{},
};

static const struct i2c_device_id msa_id[]  =  {
	{MSA_DRV_NAME,   0},
	{}
};

MODULE_DEVICE_TABLE(i2c,   msa_id);
/*----------------------------------------------------------------------------*/
static struct i2c_driver msa_driver  =  {
	.class		 =  I2C_CLASS_HWMON,
	.driver  =  {
		.name	 =  MSA_DRV_NAME,
		.owner	 =  THIS_MODULE,
		.pm  =  &msa_pm_ops,
		.of_match_table  =  msa_of_match,
	},

	.probe		 =  msa_probe,
	.remove		 =  msa_remove,
	.id_table 	 =  msa_id,
	.address_list	 =  normal_i2c,
};
/*----------------------------------------------------------------------------*/
static int __init msa_init(void)
{
	int res;

	MI_FUN;

	if (msa_startup() !=  0)
		return -1;

	if (!gsensor_info.isI2CClient)
		msa_driver.detect  =  msa_detect;
	res  =  i2c_add_driver(&msa_driver);
	if (res < 0) {
		MI_ERR("add msa i2c driver failed\n");
		input_set_power_enable(&(gsensor_info.input_type),   0);
		input_sensor_free(&(gsensor_info.input_type));
		return -ENODEV;
	}

	return (res);
}
/*----------------------------------------------------------------------------*/
static void __exit msa_exit(void)
{
	MI_FUN;

	i2c_del_driver(&msa_driver);
	input_set_power_enable(&(gsensor_info.input_type),   0);
	input_sensor_free(&(gsensor_info.input_type));
}
/*----------------------------------------------------------------------------*/
MODULE_AUTHOR("MsaMEMS <lctang@memesensing.com>");
MODULE_DESCRIPTION("MSA 3-Axis Accelerometer driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

module_init(msa_init);
module_exit(msa_exit);

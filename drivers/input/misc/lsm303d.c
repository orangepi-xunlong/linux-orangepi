/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
*
* File Name          : lsm303d.h
* Authors            : MSH - C&I BU - Application Team
*		     : Matteo Dameno (matteo.dameno@st.com)
*		     : Denis Ciocca (denis.ciocca@st.com)
*		     : Both authors are willing to be considered the contact
*		     : and update points for the driver.
* Version            : V.2.0.1
* Date               : 2012/May/07
* Description        : LSM303D accelerometer sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
********************************************************************************
Version History.

Revision 2-0-0 2012/05/04
 first revision
Revision 2-0-1 2012/05/07
 New sysfs architecture
 Support antialiasing filter
*******************************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kobject.h>

//#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif

#include "lsm303d.h"

#define	I2C_AUTO_INCREMENT	(0x80)

#define	ACC_G_MAX_POS		1495040	/** max positive value acc [ug] */
#define	ACC_G_MAX_NEG		1495770	/** max negative value acc [ug] */
#define	MAG_G_MAX_POS		983520	/** max positive value mag [ugauss] */
#define	MAG_G_MAX_NEG		983040	/** max negative value mag [ugauss] */

#define FUZZ			0
#define FLAT			0

/* Address registers */
#define REG_WHOAMI_ADDR		(0x0F)
#define REG_CNTRL0_ADDR		(0x1F)
#define REG_CNTRL1_ADDR		(0x20)
#define REG_CNTRL2_ADDR		(0x21)
#define REG_CNTRL3_ADDR		(0x22)
#define REG_CNTRL4_ADDR		(0x23)
#define REG_CNTRL5_ADDR		(0x24)
#define REG_CNTRL6_ADDR		(0x25)
#define REG_CNTRL7_ADDR		(0x26)

#define REG_ACC_DATA_ADDR	(0x28)
#define REG_MAG_DATA_ADDR	(0x08)

/* Sensitivity */
#define SENSITIVITY_ACC_2G	60	/**	ug/LSB	*/
#define SENSITIVITY_ACC_4G	120	/**	ug/LSB	*/
#define SENSITIVITY_ACC_8G	240	/**	ug/LSB	*/
#define SENSITIVITY_ACC_16G	730	/**	ug/LSB	*/

#define SENSITIVITY_MAG_2G	80	/**	ugauss/LSB	*/
#define SENSITIVITY_MAG_4G	160	/**	ugauss/LSB	*/
#define SENSITIVITY_MAG_8G	320	/**	ugauss/LSB	*/
#define SENSITIVITY_MAG_12G	480	/**	ugauss/LSB	*/

/* ODR */
#define ODR_ACC_MASK		(0XF0)	/* Mask for odr change on acc */
#define LSM303D_ACC_ODR_OFF	(0x00)  /* Power down */
#define LSM303D_ACC_ODR3_125	(0x10)  /* 3.25Hz output data rate */
#define LSM303D_ACC_ODR6_25	(0x20)  /* 6.25Hz output data rate */
#define LSM303D_ACC_ODR12_5	(0x30)  /* 12.5Hz output data rate */
#define LSM303D_ACC_ODR25	(0x40)  /* 25Hz output data rate */
#define LSM303D_ACC_ODR50	(0x50)  /* 50Hz output data rate */
#define LSM303D_ACC_ODR100	(0x60)  /* 100Hz output data rate */
#define LSM303D_ACC_ODR200	(0x70)  /* 200Hz output data rate */
#define LSM303D_ACC_ODR400	(0x80)  /* 400Hz output data rate */
#define LSM303D_ACC_ODR800	(0x90)  /* 800Hz output data rate */
#define LSM303D_ACC_ODR1600	(0xA0)  /* 1600Hz output data rate */

#define ODR_MAG_MASK		(0X1C)	/* Mask for odr change on mag */
#define LSM303D_MAG_ODR3_125	(0x00)  /* 3.25Hz output data rate */
#define LSM303D_MAG_ODR6_25	(0x04)  /* 6.25Hz output data rate */
#define LSM303D_MAG_ODR12_5	(0x08)  /* 12.5Hz output data rate */
#define LSM303D_MAG_ODR25	(0x0C)  /* 25Hz output data rate */
#define LSM303D_MAG_ODR50	(0x10)  /* 50Hz output data rate */
#define LSM303D_MAG_ODR100	(0x14)  /* 100Hz output data rate */

/* Magnetic sensor mode */
#define MSMS_MASK		(0x03)	/* Mask magnetic sensor mode */
#define POWEROFF_MAG		(0x02)	/* Power Down */
#define CONTINUOS_CONVERSION	(0x00)

/* Default values loaded in probe function */
#define WHOIAM_VALUE		(0x49)
#define REG_DEF_CNTRL0		(0x00)
#define REG_DEF_CNTRL1		(0x07)
#define REG_DEF_CNTRL2		(0x00)
#define REG_DEF_CNTRL3		(0x00)
#define REG_DEF_CNTRL4		(0x00)
#define REG_DEF_CNTRL5		(0x18)
#define REG_DEF_CNTRL6		(0x20)
#define REG_DEF_CNTRL7		(0x02)

/* Accelerometer Filter */
#define LSM303D_ACC_FILTER_MASK	(0xC0)
#define FILTER_773		773
#define FILTER_362		362
#define FILTER_194		194
#define FILTER_50		50

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x1d,I2C_CLIENT_END};
static __u32 twi_id = 0;
static int i2c_num = 0;
static const unsigned short i2c_address[] = {0x1e, 0x1d};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_REPORT_ACC_DATA = 1U << 1,
	DEBUG_REPORT_MAG_DATA = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_CONTROL_INFO = 1U << 4,
	DEBUG_INT = 1U << 5,
};

static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);


#define to_dev(obj) container_of(obj, struct device, kobj)
#define to_dev_attr(_attr) container_of(_attr, struct device_attribute, attr)

static struct kobject *acc_kobj;
static struct kobject *mag_kobj;

static struct {
	unsigned int cutoff_us;
	u8 value;
} lsm303d_acc_odr_table[] = {
		{    625, LSM303D_ACC_ODR1600 },
		{   1250, LSM303D_ACC_ODR800  },
		{   2500, LSM303D_ACC_ODR400  },
		{   5000, LSM303D_ACC_ODR200  },
		{  10000, LSM303D_ACC_ODR100  },
		{  20000, LSM303D_ACC_ODR50   },
		{  40000, LSM303D_ACC_ODR25   },
		{  80000, LSM303D_ACC_ODR12_5 },
		{ 160000, LSM303D_ACC_ODR6_25 },
		{ 320000, LSM303D_ACC_ODR3_125},
};

static struct {
	unsigned int cutoff_us;
	u8 value;
} lsm303d_mag_odr_table[] = {
		{  10000, LSM303D_MAG_ODR100  },
		{  20000, LSM303D_MAG_ODR50   },
		{  40000, LSM303D_MAG_ODR25   },
		{  80000, LSM303D_MAG_ODR12_5 },
		{ 160000, LSM303D_MAG_ODR6_25 },
		{ 320000, LSM303D_MAG_ODR3_125},
};

struct lsm303d_status {
	struct i2c_client *client;
	struct lsm303d_acc_platform_data *pdata_acc;
	struct lsm303d_acc_platform_data *pdata_mag;

	struct mutex lock;
	struct delayed_work input_work_acc;
	struct delayed_work input_work_mag;

	struct input_dev *input_dev_acc;
	struct input_dev *input_dev_mag;

	int hw_initialized;

	/* hw_working=-1 means not tested yet */
	int hw_working;

	atomic_t enabled_acc;
	atomic_t enabled_mag;
	int acc_on_before_suspend;
	int mag_on_before_suspend;

	int on_before_suspend;
	int use_smbus;

	u16 sensitivity_acc;
	u16 sensitivity_mag;

	#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	#endif
};

static const struct lsm303d_acc_platform_data default_lsm303d_acc_pdata = {
	.fs_range = LSM303D_ACC_FS_2G,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
	.poll_interval = 100,
	.min_interval = LSM303D_ACC_MIN_POLL_PERIOD_US,
	.aa_filter_bandwidth = ANTI_ALIASING_773,
};

static const struct lsm303d_mag_platform_data default_lsm303d_mag_pdata = {
	.poll_interval = 100,
	.min_interval = LSM303D_MAG_MIN_POLL_PERIOD_US,
	.fs_range = LSM303D_MAG_FS_2G,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
};

struct reg_rw {
	u8 address;
	u8 default_value;
	u8 resume_value;
};

struct reg_r {
	u8 address;
	u8 value;
};

static struct status_registers {
	struct reg_r who_am_i;
	struct reg_rw cntrl0;
	struct reg_rw cntrl1;
	struct reg_rw cntrl2;
	struct reg_rw cntrl3;
	struct reg_rw cntrl4;
	struct reg_rw cntrl5;
	struct reg_rw cntrl6;
	struct reg_rw cntrl7;
} status_registers = {
	.who_am_i.address=REG_WHOAMI_ADDR, .who_am_i.value=WHOIAM_VALUE,
	.cntrl0.address=REG_CNTRL0_ADDR, .cntrl0.default_value=REG_DEF_CNTRL0,
	.cntrl1.address=REG_CNTRL1_ADDR, .cntrl1.default_value=REG_DEF_CNTRL1,
	.cntrl2.address=REG_CNTRL2_ADDR, .cntrl2.default_value=REG_DEF_CNTRL2,
	.cntrl3.address=REG_CNTRL3_ADDR, .cntrl3.default_value=REG_DEF_CNTRL3,
	.cntrl4.address=REG_CNTRL4_ADDR, .cntrl4.default_value=REG_DEF_CNTRL4,
	.cntrl5.address=REG_CNTRL5_ADDR, .cntrl5.default_value=REG_DEF_CNTRL5,
	.cntrl6.address=REG_CNTRL6_ADDR, .cntrl6.default_value=REG_DEF_CNTRL6,
	.cntrl7.address=REG_CNTRL7_ADDR, .cntrl7.default_value=REG_DEF_CNTRL7,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void lsm303d_early_suspend(struct early_suspend *h);
static void lsm303d_late_resume(struct early_suspend *h);
#endif

static int lsm303d_i2c_read(struct lsm303d_status *stat, u8 *buf, int len)
{
	int ret;
	u8 reg = buf[0];
	u8 cmd = reg;
#ifdef DEBUG
	unsigned int ii;
#endif

	if (len > 1)
		cmd = (I2C_AUTO_INCREMENT | reg);
	if (stat->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_read_byte_data(stat->client, cmd);
			buf[0] = ret & 0xff;
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_read_byte_data: ret=0x%02x, len:%d ,"
				"command=0x%02x, buf[0]=0x%02x\n",
				ret, len, cmd , buf[0]);
#endif
		} else if (len > 1) {
			ret = i2c_smbus_read_i2c_block_data(stat->client,
								cmd, len, buf);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_read_i2c_block_data: ret:%d len:%d, "
				"command=0x%02x, ",
				ret, len, cmd);
			for (ii = 0; ii < len; ii++)
				printk(KERN_DEBUG "buf[%d]=0x%02x,",
								ii, buf[ii]);

			printk("\n");
#endif
		} else
			ret = -1;

		if (ret < 0) {
			dev_err(&stat->client->dev,
				"read transfer error: len:%d, command=0x%02x\n",
				len, cmd);
			return 0;
		}
		return len;
	}

	//i2c read protocal?
	ret = i2c_master_send(stat->client, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return ret;

	return i2c_master_recv(stat->client, buf, len);
}

static int lsm303d_i2c_write(struct lsm303d_status *stat, u8 *buf, int len)
{
	int ret;
	u8 reg, value;
#ifdef DEBUG
	unsigned int ii;
#endif

	if (len > 1)
		buf[0] = (I2C_AUTO_INCREMENT | buf[0]);

	reg = buf[0];
	value = buf[1];

	if (stat->use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_write_byte_data(stat->client,
								reg, value);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_write_byte_data: ret=%d, len:%d, "
				"command=0x%02x, value=0x%02x\n",
				ret, len, reg , value);
#endif
			return ret;
		} else if (len > 1) {
			ret = i2c_smbus_write_i2c_block_data(stat->client,
							reg, len, buf + 1);
#ifdef DEBUG
			dev_warn(&stat->client->dev,
				"i2c_smbus_write_i2c_block_data: ret=%d, "
				"len:%d, command=0x%02x, ",
				ret, len, reg);
			for (ii = 0; ii < (len + 1); ii++)
				printk(KERN_DEBUG "value[%d]=0x%02x,",
								ii, buf[ii]);

			printk("\n");
#endif
			return ret;
		}
	}

	ret = i2c_master_send(stat->client, buf, len+1);
	return (ret == len+1) ? 0 : ret;
}

/**
 * e_compass_fetch_sysconfig_para - get config info from sysconfig.fex file.
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */

static int e_compass_fetch_sysconfig_para(void)
{
	int ret = -1;
	int device_used = -1;
	script_item_u	val;
	script_item_value_type_e  type;	
		
	dprintk(DEBUG_INIT, "========%s===================\n", __func__);
	
	type = script_get_item("compass_para", "compass_used", &val);
 
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: type err  device_used = %d. \n", __func__, val.val);
		goto script_get_err;
	}
	device_used = val.val;
	
	if (1 == device_used) {
		type = script_get_item("compass_para", "compass_twi_id", &val);	
		if(SCIRPT_ITEM_VALUE_TYPE_INT != type){
			pr_err("%s: type err twi_id = %d. \n", __func__, val.val);
			goto script_get_err;
		}
		twi_id = val.val;
		
		dprintk(DEBUG_INIT, "%s: twi_id is %d. \n", __func__, twi_id);

		ret = 0;
		
	} else {
		pr_err("%s: compass_unused. \n",  __func__);
		ret = -1;
	}

	return ret;

script_get_err:
	pr_notice("=========script_get_err============\n");
	return ret;

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
	int err = -1;
	u8 buf[1];
	u8 cmd;

	dprintk(DEBUG_INIT, "enter func %s. \n", __FUNCTION__);
	
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	if(twi_id == adapter->nr){
		for(i2c_num = 0; i2c_num < (sizeof(i2c_address)/sizeof(i2c_address[0]));i2c_num++){
			client->addr = i2c_address[i2c_num];

			dprintk(DEBUG_INIT, "check i2c addr: %x .\n", client->addr);
			buf[0] = status_registers.who_am_i.address;
			cmd = buf[0];

			//err = lsm303d_i2c_read(stat, buf, 1);
			ret = i2c_master_send(client, &cmd, sizeof(cmd));
			if (ret != sizeof(cmd))
				continue;

			dprintk(DEBUG_INIT, "check i2c addr: %x after send cmd.\n", client->addr);

			err = i2c_master_recv(client, buf, 1);

			dprintk(DEBUG_INIT, "check i2c addr: %x after recv cmd.\n", client->addr);

			if (err < 0) {
				dev_warn(&client->dev, "Error reading WHO_AM_I: is device"
					" available/working?\n");
				
			}else	if (buf[0] != status_registers.who_am_i.value) {
				dev_err(&client->dev,
					"device unknown. Expected: 0x%02x,"
					" Replies: 0x%02x\n", status_registers.who_am_i.value, buf[0]);
			}else {
				pr_info("%s:addr= 0x%x,i2c_num:%d\n",__func__,client->addr,i2c_num);
				strlcpy(info->type, LSM303D_DEV_NAME, I2C_NAME_SIZE);	
				return 0;
			}

		}
	
		pr_info("%s:lsm303d Device not found, \
			 maybe the other e_compass equipment! \n",__func__);
		return -ENODEV;
	}else{
		return -ENODEV;
	}
}

static int lsm303d_hw_init(struct lsm303d_status *stat)
{
	int err = -1;
	u8 buf[1];

	pr_info("%s: hw init start\n", LSM303D_DEV_NAME);

	buf[0] = status_registers.who_am_i.address;
	err = lsm303d_i2c_read(stat, buf, 1);
	
	if (err < 0) {
		dev_warn(&stat->client->dev, "Error reading WHO_AM_I: is device"
		" available/working?\n");
		goto err_firstread;
	} else
		stat->hw_working = 1;

	if (buf[0] != status_registers.who_am_i.value) {
	dev_err(&stat->client->dev,
		"device unknown. Expected: 0x%02x,"
		" Replies: 0x%02x\n", status_registers.who_am_i.value, buf[0]);
		err = -1;
		goto err_unknown_device;
	}

	status_registers.cntrl1.resume_value = 
					status_registers.cntrl1.default_value;
	status_registers.cntrl2.resume_value = 
					status_registers.cntrl2.default_value;
	status_registers.cntrl3.resume_value = 
					status_registers.cntrl3.default_value;
	status_registers.cntrl4.resume_value = 
					status_registers.cntrl4.default_value;
	status_registers.cntrl5.resume_value = 
					status_registers.cntrl5.default_value;
	status_registers.cntrl6.resume_value = 
					status_registers.cntrl6.default_value;
	status_registers.cntrl7.resume_value = 
					status_registers.cntrl7.default_value;

	stat->hw_initialized = 1;
	pr_info("%s: hw init done\n", LSM303D_DEV_NAME);

	return 0;

err_unknown_device:
err_firstread:
	stat->hw_working = 0;
	stat->hw_initialized = 0;
	return err;
}

static int lsm303d_acc_device_power_off(struct lsm303d_status *stat)
{
	int err;
	u8 buf[2];

	buf[0] = status_registers.cntrl1.address;
	buf[1] = ((ODR_ACC_MASK & LSM303D_ACC_ODR_OFF) | 
		((~ODR_ACC_MASK) & status_registers.cntrl1.resume_value));

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		dev_err(&stat->client->dev, "accelerometer soft power off "
							"failed: %d\n", err);

	if (stat->pdata_acc->power_off) {
		stat->pdata_acc->power_off();
	}
	
	atomic_set(&stat->enabled_acc, 0);

	return 0;
}

static int lsm303d_mag_device_power_off(struct lsm303d_status *stat)
{
	int err;
	u8 buf[2];

	buf[0] = status_registers.cntrl7.address;
	buf[1] = ((MSMS_MASK & POWEROFF_MAG) | 
		((~MSMS_MASK) & status_registers.cntrl7.resume_value));

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		dev_err(&stat->client->dev, "magnetometer soft power off "
							"failed: %d\n", err);

	if (stat->pdata_mag->power_off) {
		stat->pdata_mag->power_off();
	}

	atomic_set(&stat->enabled_mag, 0);

	return 0;
}

static int lsm303d_acc_device_power_on(struct lsm303d_status *stat)
{
	int err = -1;
	u8 buf[5];

	if (stat->pdata_acc->power_on) {
		err = stat->pdata_acc->power_on();
		if (err < 0) {
			dev_err(&stat->client->dev,
				"accelerometer power_on failed: %d\n", err);
			return err;
		}
	}

	buf[0] = status_registers.cntrl0.address;
	buf[1] = status_registers.cntrl0.resume_value;
	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = status_registers.cntrl1.address;
	buf[1] = status_registers.cntrl1.resume_value;
	buf[2] = status_registers.cntrl2.resume_value;
	buf[3] = status_registers.cntrl3.resume_value;
	buf[4] = status_registers.cntrl4.resume_value;
	err = lsm303d_i2c_write(stat, buf, 4);
	if (err < 0)
		goto err_resume_state;	

	buf[0] = status_registers.cntrl7.address;
	buf[1] = status_registers.cntrl7.resume_value;
	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;

	atomic_set(&stat->enabled_acc, 1);

	return 0;

err_resume_state:
	atomic_set(&stat->enabled_acc, 0);
	dev_err(&stat->client->dev, "accelerometer hw power on error "
				"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_mag_device_power_on(struct lsm303d_status *stat)
{
	int err = -1;
	u8 buf[6];

	if (stat->pdata_mag->power_on) {
		err = stat->pdata_mag->power_on();
		if (err < 0) {
			dev_err(&stat->client->dev,
				"magnetometer power_on failed: %d\n", err);
			return err;
		}
	}
	
	buf[0] = status_registers.cntrl0.address;
	buf[1] = status_registers.cntrl0.resume_value;
	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = status_registers.cntrl3.address;
	buf[1] = status_registers.cntrl3.resume_value;
	buf[2] = status_registers.cntrl4.resume_value;
	buf[3] = status_registers.cntrl5.resume_value;
	buf[4] = status_registers.cntrl6.resume_value;
	buf[5] = ((MSMS_MASK & CONTINUOS_CONVERSION) | 
		((~MSMS_MASK) & status_registers.cntrl7.resume_value));


	err = lsm303d_i2c_write(stat, buf, 5);
	if (err < 0)
		goto err_resume_state;	

	atomic_set(&stat->enabled_mag, 1);

	return 0;

err_resume_state:
	atomic_set(&stat->enabled_mag, 0);
	dev_err(&stat->client->dev, "magnetometer hw power on error "
				"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_acc_update_filter(struct lsm303d_status *stat, 
							u8 new_bandwidth)
{
	int err=-1;

	u8 updated_val;
	u8 buf[2];

	switch (new_bandwidth) {
	case ANTI_ALIASING_50:
		break;
	case ANTI_ALIASING_194:
		break;
	case ANTI_ALIASING_362:
		break;
	case ANTI_ALIASING_773:
		break;
	default:
		dev_err(&stat->client->dev, "invalid accelerometer "
			"update bandwidth requested: %u\n", new_bandwidth);
		return -EINVAL;
	}

	buf[0] = status_registers.cntrl2.address;
	err = lsm303d_i2c_read(stat, buf, 1);
	if (err < 0)
		goto error;

	status_registers.cntrl2.resume_value = buf[0];	
	updated_val = ((LSM303D_ACC_FILTER_MASK & new_bandwidth) | 
					((~LSM303D_ACC_FILTER_MASK) & buf[0]));
	buf[1] = updated_val;
	buf[0] = status_registers.cntrl2.address;

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto error;
	status_registers.cntrl2.resume_value = updated_val;

	return err;
	
error:
	dev_err(&stat->client->dev, "update accelerometer fs range failed "
		"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_acc_update_fs_range(struct lsm303d_status *stat, 
								u8 new_fs_range)
{
	int err=-1;

	u16 sensitivity;
	u8 updated_val;
	u8 buf[2];

	switch (new_fs_range) {
	case LSM303D_ACC_FS_2G:
		sensitivity = SENSITIVITY_ACC_2G;
		break;
	case LSM303D_ACC_FS_4G:
		sensitivity = SENSITIVITY_ACC_4G;
		break;
	case LSM303D_ACC_FS_8G:
		sensitivity = SENSITIVITY_ACC_8G;
		break;
	case LSM303D_ACC_FS_16G:
		sensitivity = SENSITIVITY_ACC_16G;
		break;
	default:
		dev_err(&stat->client->dev, "invalid accelerometer "
				"fs range requested: %u\n", new_fs_range);
		return -EINVAL;
	}

	buf[0] = status_registers.cntrl2.address;
	err = lsm303d_i2c_read(stat, buf, 1);
	if (err < 0)
		goto error;

	status_registers.cntrl2.resume_value = buf[0];	
	updated_val = ((LSM303D_ACC_FS_MASK & new_fs_range) | 
					((~LSM303D_ACC_FS_MASK) & buf[0]));
	buf[1] = updated_val;
	buf[0] = status_registers.cntrl2.address;

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto error;
	status_registers.cntrl2.resume_value = updated_val;
	stat->sensitivity_acc = sensitivity;

	return err;
	
error:
	dev_err(&stat->client->dev, "update accelerometer fs range failed "
		"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_mag_update_fs_range(struct lsm303d_status *stat, 
								u8 new_fs_range)
{
	int err=-1;

	u16 sensitivity;
	u8 updated_val;
	u8 buf[2];

	switch (new_fs_range) {
	case LSM303D_MAG_FS_2G:
		sensitivity = SENSITIVITY_MAG_2G;
		break;
	case LSM303D_MAG_FS_4G:
		sensitivity = SENSITIVITY_MAG_4G;
		break;
	case LSM303D_MAG_FS_8G:
		sensitivity = SENSITIVITY_MAG_8G;
		break;
	case LSM303D_MAG_FS_12G:
		sensitivity = SENSITIVITY_MAG_12G;
		break;
	default:
		dev_err(&stat->client->dev, "invalid magnetometer "
				"fs range requested: %u\n", new_fs_range);
		return -EINVAL;
	}

	buf[0] = status_registers.cntrl6.address;
	err = lsm303d_i2c_read(stat, buf, 1);
	if (err < 0)
		goto error;

	status_registers.cntrl6.resume_value = buf[0];	
	updated_val = (LSM303D_MAG_FS_MASK & new_fs_range);
	buf[1] = updated_val;
	buf[0] = status_registers.cntrl6.address;

	err = lsm303d_i2c_write(stat, buf, 1);
	if (err < 0)
		goto error;
	status_registers.cntrl6.resume_value = updated_val;
	stat->sensitivity_mag = sensitivity;

	return err;
	
error:
	dev_err(&stat->client->dev, "update magnetometer fs range failed "
		"0x%02x,0x%02x: %d\n", buf[0], buf[1], err);
	return err;
}

static int lsm303d_acc_update_odr(struct lsm303d_status *stat,
						unsigned int poll_interval_us)
{
	int err = -1;
	u8 config[2];
	int i;

	for (i = ARRAY_SIZE(lsm303d_acc_odr_table) - 1; i >= 0; i--) {
		if ((lsm303d_acc_odr_table[i].cutoff_us <= poll_interval_us)
								|| (i == 0))
			break;
	}

	config[1] = ((ODR_ACC_MASK & lsm303d_acc_odr_table[i].value) | 
		((~ODR_ACC_MASK) & status_registers.cntrl1.resume_value));

	if (atomic_read(&stat->enabled_acc)) {
		config[0] = status_registers.cntrl1.address;
		err = lsm303d_i2c_write(stat, config, 1);
		if (err < 0)
			goto error;
		status_registers.cntrl1.resume_value = config[1];
	}

	return err;

error:
	dev_err(&stat->client->dev, "update accelerometer odr failed "
			"0x%02x,0x%02x: %d\n", config[0], config[1], err);

	return err;
}

static int lsm303d_mag_update_odr(struct lsm303d_status *stat,
						unsigned int poll_interval_us)
{
	int err = -1;
	u8 config[2];
	int i;

	for (i = ARRAY_SIZE(lsm303d_mag_odr_table) - 1; i >= 0; i--) {
		if ((lsm303d_mag_odr_table[i].cutoff_us <= poll_interval_us)
								|| (i == 0))
			break;
	}

	config[1] = ((ODR_MAG_MASK & lsm303d_mag_odr_table[i].value) | 
		((~ODR_MAG_MASK) & status_registers.cntrl5.resume_value));

	if (atomic_read(&stat->enabled_mag)) {
		config[0] = status_registers.cntrl5.address;
		err = lsm303d_i2c_write(stat, config, 1);
		if (err < 0)
			goto error;
		status_registers.cntrl5.resume_value = config[1];
	}

	return err;

error:
	dev_err(&stat->client->dev, "update magnetometer odr failed "
			"0x%02x,0x%02x: %d\n", config[0], config[1], err);

	return err;
}

static int lsm303d_validate_polling(unsigned int *min_interval, 
					unsigned int *poll_interval, 
					unsigned int min, u8 *axis_map_x, 
					u8 *axis_map_y, u8 *axis_map_z,  
					struct i2c_client *client)
{
	*min_interval = max(min, *min_interval);
	*poll_interval = max(*poll_interval, *min_interval);

	if (*axis_map_x > 2 || *axis_map_y > 2 || *axis_map_z > 2) {
		dev_err(&client->dev, "invalid axis_map value "
		"x:%u y:%u z%u\n", *axis_map_x, *axis_map_y, *axis_map_z);
		return -EINVAL;
	}

	return 0;
}

static int lsm303d_validate_negate(u8 *negate_x, u8 *negate_y, u8 *negate_z,
						struct i2c_client *client)
{
	if (*negate_x > 1 || *negate_y > 1 || *negate_z > 1) {
		dev_err(&client->dev, "invalid negate value "
			"x:%u y:%u z:%u\n", *negate_x, *negate_y, *negate_z);
		return -EINVAL;
	}
	return 0;
}

static int lsm303d_acc_validate_pdata(struct lsm303d_status *stat)
{
	int res=-1;
	res=lsm303d_validate_polling(&stat->pdata_acc->min_interval,
				&stat->pdata_acc->poll_interval,
				(unsigned int)LSM303D_ACC_MIN_POLL_PERIOD_US,
				&stat->pdata_acc->axis_map_x,
				&stat->pdata_acc->axis_map_y,
				&stat->pdata_acc->axis_map_z,
				stat->client);
	if(res<0)
		return -EINVAL;

	res=lsm303d_validate_negate(&stat->pdata_acc->negate_x,
				&stat->pdata_acc->negate_y,
				&stat->pdata_acc->negate_z,
				stat->client);
	if(res<0)
		return -EINVAL;

	res=-1;
	switch (stat->pdata_acc->aa_filter_bandwidth) {
		case ANTI_ALIASING_50:
			res=1;
			break;
		case ANTI_ALIASING_194:
			res=1;
			break;
		case ANTI_ALIASING_362:
			res=1;
			break;
		case ANTI_ALIASING_773:
			res=1;
			break;
		default:
			dev_err(&stat->client->dev, "invalid accelerometer "
				"bandwidth selected: %u\n", 
					stat->pdata_acc->aa_filter_bandwidth);
			return -EINVAL;
	}

	return res;
}

static int lsm303d_mag_validate_pdata(struct lsm303d_status *stat)
{
	int res=-1;
	res=lsm303d_validate_polling(&stat->pdata_mag->min_interval,
				&stat->pdata_mag->poll_interval,
				(unsigned int)LSM303D_MAG_MIN_POLL_PERIOD_US,
				&stat->pdata_mag->axis_map_x,
				&stat->pdata_mag->axis_map_y,
				&stat->pdata_mag->axis_map_z,
				stat->client);
	if(res<0)
		return -EINVAL;

	res=lsm303d_validate_negate(&stat->pdata_mag->negate_x,
				&stat->pdata_mag->negate_y,
				&stat->pdata_mag->negate_z,
				stat->client);
	if(res<0)
		return -EINVAL;

	return 0;
}

static int lsm303d_acc_enable(struct lsm303d_status *stat)
{
	int err;

	if (!atomic_cmpxchg(&stat->enabled_acc, 0, 1)) {
		err = lsm303d_acc_device_power_on(stat);
		if (err < 0) {
			atomic_set(&stat->enabled_acc, 0);
			return err;
		}
		schedule_delayed_work(&stat->input_work_acc,
			usecs_to_jiffies(stat->pdata_acc->poll_interval));
	}

	return 0;
}

static int lsm303d_acc_disable(struct lsm303d_status *stat)
{
	if (atomic_cmpxchg(&stat->enabled_acc, 1, 0)) {
		cancel_delayed_work_sync(&stat->input_work_acc);
		lsm303d_acc_device_power_off(stat);
	}

	return 0;
}

static int lsm303d_mag_enable(struct lsm303d_status *stat)
{
	int err;

	if (!atomic_cmpxchg(&stat->enabled_mag, 0, 1)) {
		err = lsm303d_mag_device_power_on(stat);
		if (err < 0) {
			atomic_set(&stat->enabled_mag, 0);
			return err;
		}
		schedule_delayed_work(&stat->input_work_mag,
			usecs_to_jiffies(stat->pdata_mag->poll_interval));
	}

	return 0;
}

static int lsm303d_mag_disable(struct lsm303d_status *stat)
{
	if (atomic_cmpxchg(&stat->enabled_mag, 1, 0)) {
		cancel_delayed_work_sync(&stat->input_work_mag);
		lsm303d_mag_device_power_off(stat);
	}

	return 0;
}

static void lsm303d_acc_input_cleanup(struct lsm303d_status *stat)
{
	input_unregister_device(stat->input_dev_acc);
}

static void lsm303d_mag_input_cleanup(struct lsm303d_status *stat)
{
	input_unregister_device(stat->input_dev_mag);
}

static ssize_t attr_get_polling_rate_acc(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	char *buf)
{
	unsigned int val;
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	mutex_lock(&stat->lock);
	val = stat->pdata_acc->poll_interval;
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%u\n", val);
}

static ssize_t attr_get_polling_rate_mag(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	char *buf)
{
	unsigned int val;
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	mutex_lock(&stat->lock);
	val = stat->pdata_mag->poll_interval;
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%u\n", val);
}

static ssize_t attr_set_polling_rate_acc(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	const char *buf, size_t size)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long interval_us;

	if (strict_strtoul(buf, 10, &interval_us))
		return -EINVAL;
	if (!interval_us)
		return -EINVAL;
	interval_us = (unsigned int)max((unsigned int)interval_us,
						stat->pdata_acc->min_interval);
	mutex_lock(&stat->lock);
	stat->pdata_acc->poll_interval = (unsigned int)interval_us;
	lsm303d_acc_update_odr(stat, interval_us);
	mutex_unlock(&stat->lock);
	return size;
}

static ssize_t attr_set_polling_rate_mag(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	const char *buf, size_t size)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long interval_us;

	if (strict_strtoul(buf, 10, &interval_us))
		return -EINVAL;
	if (!interval_us)
		return -EINVAL;
	interval_us = (unsigned int)max((unsigned int)interval_us,
						stat->pdata_mag->min_interval);
	mutex_lock(&stat->lock);
	stat->pdata_mag->poll_interval = (unsigned int)interval_us;
	lsm303d_mag_update_odr(stat, interval_us);
	mutex_unlock(&stat->lock);
	return size;
}

static ssize_t attr_get_enable_acc(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	char *buf)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int val = (int)atomic_read(&stat->enabled_acc);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_get_enable_mag(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	char *buf)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int val = (int)atomic_read(&stat->enabled_mag);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_enable_acc(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	const char *buf, size_t size)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	dprintk(DEBUG_CONTROL_INFO, "%s: val is %ld. \n", __func__, val);

	if (val)
		lsm303d_acc_enable(stat);
	else
		lsm303d_acc_disable(stat);

	return size;
}

static ssize_t attr_set_enable_mag(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	const char *buf, size_t size)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	dprintk(DEBUG_CONTROL_INFO, "%s: val is %ld. \n", __func__, val);

	if (val)
		lsm303d_mag_enable(stat);
	else
		lsm303d_mag_disable(stat);

	return size;
}

static ssize_t attr_get_range_acc(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	char *buf)
{
	struct device *dev = to_dev(kobj->parent);
	u8 val;
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int range = 2;
	mutex_lock(&stat->lock);
	val = stat->pdata_acc->fs_range ;
	switch (val) {
	case LSM303D_ACC_FS_2G:
		range = 2;
		break;
	case LSM303D_ACC_FS_4G:
		range = 4;
		break;
	case LSM303D_ACC_FS_8G:
		range = 8;
		break;
	case LSM303D_ACC_FS_16G:
		range = 16;
		break;
	}
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_get_range_mag(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	char *buf)
{
	struct device *dev = to_dev(kobj->parent);
	u8 val;
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int range = 2;
	mutex_lock(&stat->lock);
	val = stat->pdata_mag->fs_range ;
	switch (val) {
	case LSM303D_MAG_FS_2G:
		range = 2;
		break;
	case LSM303D_MAG_FS_4G:
		range = 4;
		break;
	case LSM303D_MAG_FS_8G:
		range = 8;
		break;
	case LSM303D_MAG_FS_12G:
		range = 12;
		break;
	}
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_set_range_acc(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	const char *buf, size_t size)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;
	u8 range;
	int err;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	switch (val) {
	case 2:
		range = LSM303D_ACC_FS_2G;
		break;
	case 4:
		range = LSM303D_ACC_FS_4G;
		break;
	case 8:
		range = LSM303D_ACC_FS_8G;
		break;
	case 16:
		range = LSM303D_ACC_FS_16G;
		break;
	default:
		dev_err(&stat->client->dev, "accelerometer invalid range "
					"request: %lu, discarded\n", val);
		return -EINVAL;
	}
	mutex_lock(&stat->lock);
	err = lsm303d_acc_update_fs_range(stat, range);
	if (err < 0) {
		mutex_unlock(&stat->lock);
		return err;
	}
	stat->pdata_acc->fs_range = range;
	mutex_unlock(&stat->lock);
	dev_info(&stat->client->dev, "accelerometer range set to:"
							" %lu g\n", val);

	return size;
}

static ssize_t attr_set_range_mag(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	const char *buf, size_t size)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;
	u8 range;
	int err;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	switch (val) {
	case 2:
		range = LSM303D_MAG_FS_2G;
		break;
	case 4:
		range = LSM303D_MAG_FS_4G;
		break;
	case 8:
		range = LSM303D_MAG_FS_8G;
		break;
	case 16:
		range = LSM303D_MAG_FS_12G;
		break;
	default:
		dev_err(&stat->client->dev, "magnetometer invalid range "
					"request: %lu, discarded\n", val);
		return -EINVAL;
	}
	mutex_lock(&stat->lock);
	err = lsm303d_mag_update_fs_range(stat, range);
	if (err < 0) {
		mutex_unlock(&stat->lock);
		return err;
	}
	stat->pdata_mag->fs_range = range;
	mutex_unlock(&stat->lock);
	dev_info(&stat->client->dev, "magnetometer range set to:"
							" %lu g\n", val);

	return size;
}

static ssize_t attr_get_aa_filter(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	char *buf)
{
	struct device *dev = to_dev(kobj->parent);
	u8 val;
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	int frequency=FILTER_773;
	mutex_lock(&stat->lock);
	val = stat->pdata_acc->aa_filter_bandwidth;
	switch (val) {
	case ANTI_ALIASING_50:
		frequency = FILTER_50;
		break;
	case ANTI_ALIASING_194:
		frequency = FILTER_194;
		break;
	case ANTI_ALIASING_362:
		frequency = FILTER_362;
		break;
	case ANTI_ALIASING_773:
		frequency = FILTER_773;
		break;
	}
	mutex_unlock(&stat->lock);
	return sprintf(buf, "%d\n", frequency);
}

static ssize_t attr_set_aa_filter(struct kobject *kobj,
					struct kobj_attribute *attr,
				     	const char *buf, size_t size)
{
	struct device *dev = to_dev(kobj->parent);
	struct lsm303d_status *stat = dev_get_drvdata(dev);
	unsigned long val;
	u8 frequency;
	int err;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	switch (val) {
	case FILTER_50:
		frequency = ANTI_ALIASING_50;
		break;
	case FILTER_194:
		frequency = ANTI_ALIASING_194;
		break;
	case FILTER_362:
		frequency = ANTI_ALIASING_362;
		break;
	case FILTER_773:
		frequency = ANTI_ALIASING_773;
		break;
	default:
		dev_err(&stat->client->dev, "accelerometer invalid filter "
					"request: %lu, discarded\n", val);
		return -EINVAL;
	}
	mutex_lock(&stat->lock);
	err = lsm303d_acc_update_filter(stat, frequency);
	if (err < 0) {
		mutex_unlock(&stat->lock);
		return err;
	}
	stat->pdata_acc->aa_filter_bandwidth = frequency;
	mutex_unlock(&stat->lock);
	dev_info(&stat->client->dev, "accelerometer anti-aliasing filter "
					"set to: %lu Hz\n", val);

	return size;
}

static struct kobj_attribute poll_attr_acc =
	__ATTR(pollrate_us, 0666, attr_get_polling_rate_acc, 
						attr_set_polling_rate_acc);
static struct kobj_attribute enable_attr_acc =
	__ATTR(enable_device, 0666, attr_get_enable_acc, 
						attr_set_enable_acc);
static struct kobj_attribute fs_attr_acc =
	__ATTR(full_scale, 0666, attr_get_range_acc, attr_set_range_acc);

static struct kobj_attribute aa_filter_attr  =
	__ATTR(anti_aliasing_frequency, 0666, attr_get_aa_filter, 
							attr_set_aa_filter);

static struct kobj_attribute poll_attr_mag =
	__ATTR(pollrate_us, 0666, attr_get_polling_rate_mag, 
						attr_set_polling_rate_mag);
static struct kobj_attribute enable_attr_mag =
	__ATTR(enable_device, 0666, attr_get_enable_mag, 
						attr_set_enable_mag);
static struct kobj_attribute fs_attr_mag =
	__ATTR(full_scale, 0666, attr_get_range_mag, attr_set_range_mag);

static struct attribute *attributes_acc[] = {
	&poll_attr_acc.attr,
	&enable_attr_acc.attr,
	&fs_attr_acc.attr,
	&aa_filter_attr.attr,
	NULL,
};

static struct attribute *attributes_mag[] = {
	&poll_attr_mag.attr,
	&enable_attr_mag.attr,
	&fs_attr_mag.attr,
	NULL,
};

static struct attribute_group attr_group_acc = {	
	.attrs = attributes_acc,
};

static struct attribute_group attr_group_mag = {
	.attrs = attributes_mag,
};

static int create_sysfs_interfaces(struct device *dev)
{
	int err;

	acc_kobj = kobject_create_and_add("accelerometer", &dev->kobj);
	if(!acc_kobj)
		return -ENOMEM;

	mag_kobj = kobject_create_and_add("magnetometer", &dev->kobj);
	if(!mag_kobj)
		return -ENOMEM;

	err = sysfs_create_group(acc_kobj, &attr_group_acc);
	if (err)
		kobject_put(acc_kobj);

	err = sysfs_create_group(mag_kobj, &attr_group_mag);
	if (err)
		kobject_put(mag_kobj);

	return err;
}

static void remove_sysfs_interfaces(struct device *dev)
{
	kobject_put(acc_kobj);
	kobject_put(mag_kobj);
}

static int lsm303d_acc_input_open(struct input_dev *input)
{
	struct lsm303d_status *stat = input_get_drvdata(input);

	return lsm303d_acc_enable(stat);
}

static void lsm303d_acc_input_close(struct input_dev *dev)
{
	struct lsm303d_status *stat = input_get_drvdata(dev);

	lsm303d_acc_disable(stat);
}

static int lsm303d_mag_input_open(struct input_dev *input)
{
	struct lsm303d_status *stat = input_get_drvdata(input);

	return lsm303d_mag_enable(stat);
}

static void lsm303d_mag_input_close(struct input_dev *dev)
{
	struct lsm303d_status *stat = input_get_drvdata(dev);

	lsm303d_mag_disable(stat);
}

static int lsm303d_acc_get_data(struct lsm303d_status *stat, int *xyz)
{
	int err = -1;
	u8 acc_data[6];
	s32 hw_d[3] = { 0 };

	acc_data[0] = (REG_ACC_DATA_ADDR);
	err = lsm303d_i2c_read(stat, acc_data, 6);
	if (err < 0)
		return err;

	hw_d[0] = ((s32)( (s16)((acc_data[1] << 8) | (acc_data[0]))));
	hw_d[1] = ((s32)( (s16)((acc_data[3] << 8) | (acc_data[2]))));
	hw_d[2] = ((s32)( (s16)((acc_data[5] << 8) | (acc_data[4]))));

#ifdef DEBUG
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_ACC_DEV_NAME, acc_data[1], acc_data[0], hw_d[0]);
	pr_debug("%s read y=%X %X(regH regL), y=%d(dec) [ug]\n",
		LSM303D_ACC_DEV_NAME, acc_data[3], acc_data[2], hw_d[1]);
	pr_debug("%s read z=%X %X(regH regL), z=%d(dec) [ug]\n",
		LSM303D_ACC_DEV_NAME, acc_data[5], acc_data[4], hw_d[2]);
#endif

	hw_d[0] = hw_d[0] * stat->sensitivity_acc;
	hw_d[1] = hw_d[1] * stat->sensitivity_acc;
	hw_d[2] = hw_d[2] * stat->sensitivity_acc;

	xyz[0] = ((stat->pdata_acc->negate_x) ? 
				(-hw_d[stat->pdata_acc->axis_map_x])
		   			: (hw_d[stat->pdata_acc->axis_map_x]));
	xyz[1] = ((stat->pdata_acc->negate_y) ? 
				(-hw_d[stat->pdata_acc->axis_map_y])
		   			: (hw_d[stat->pdata_acc->axis_map_y]));
	xyz[2] = ((stat->pdata_acc->negate_z) ? 
				(-hw_d[stat->pdata_acc->axis_map_z])
		   			: (hw_d[stat->pdata_acc->axis_map_z]));

	return err;
}

static int lsm303d_mag_get_data(struct lsm303d_status *stat, int *xyz)
{
	int err = -1;
	u8 mag_data[6];
	s32 hw_d[3] = { 0 };

	mag_data[0] = (REG_MAG_DATA_ADDR);
	err = lsm303d_i2c_read(stat, mag_data, 6);
	if (err < 0)
		return err;

	hw_d[0] = ((s32)( (s16)((mag_data[1] << 8) | (mag_data[0]))));
	hw_d[1] = ((s32)( (s16)((mag_data[3] << 8) | (mag_data[2]))));
	hw_d[2] = ((s32)( (s16)((mag_data[5] << 8) | (mag_data[4]))));

#ifdef DEBUG
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_MAG_DEV_NAME, mag_data[1], mag_data[0], hw_d[0]);
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_MAG_DEV_NAME, mag_data[3], mag_data[2], hw_d[1]);
	pr_debug("%s read x=%X %X(regH regL), x=%d(dec) [ug]\n",
		LSM303D_MAG_DEV_NAME, mag_data[5], mag_data[4], hw_d[2]);
#endif

	hw_d[0] = hw_d[0] * stat->sensitivity_mag;
	hw_d[1] = hw_d[1] * stat->sensitivity_mag;
	hw_d[2] = hw_d[2] * stat->sensitivity_mag;

	xyz[0] = ((stat->pdata_mag->negate_x) ? 
				(-hw_d[stat->pdata_mag->axis_map_x])
		   			: (hw_d[stat->pdata_mag->axis_map_x]));
	xyz[1] = ((stat->pdata_mag->negate_y) ? 
				(-hw_d[stat->pdata_mag->axis_map_y])
		   			: (hw_d[stat->pdata_mag->axis_map_y]));
	xyz[2] = ((stat->pdata_mag->negate_z) ? 
				(-hw_d[stat->pdata_mag->axis_map_z])
		   			: (hw_d[stat->pdata_mag->axis_map_z]));

	return err;
}

static void lsm303d_acc_report_values(struct lsm303d_status *stat, int *xyz)
{

	xyz[0] /= 1000;
	xyz[1] /= 1000;
	xyz[2] /= 1000;

	input_report_abs(stat->input_dev_acc, ABS_X, xyz[0]);
	input_report_abs(stat->input_dev_acc, ABS_Y, xyz[1]);
	input_report_abs(stat->input_dev_acc, ABS_Z, xyz[2]);
	dprintk(DEBUG_REPORT_ACC_DATA, "stat->input_dev_acc x = %d, y = %d, z = %d. \n", \
		xyz[0], xyz[1], xyz[2]); 
	
	input_sync(stat->input_dev_acc);
}

static void lsm303d_mag_report_values(struct lsm303d_status *stat, int *xyz)
{

	xyz[0] /= 1000;
	xyz[1] /= 1000;
	xyz[2] /= 1000;

	input_report_abs(stat->input_dev_mag, ABS_X, xyz[0]);
	input_report_abs(stat->input_dev_mag, ABS_Y, xyz[1]);
	input_report_abs(stat->input_dev_mag, ABS_Z, xyz[2]);
	dprintk(DEBUG_REPORT_MAG_DATA, "stat->input_dev_mag x = %d, y = %d, z = %d. \n", \
		xyz[0], xyz[1], xyz[2]); 
	
	input_sync(stat->input_dev_mag);
}

static void lsm303d_acc_input_work_func(struct work_struct *work)
{
	struct lsm303d_status *stat;
	int xyz[3] = { 0 };
	int err;

	dprintk(DEBUG_REPORT_ACC_DATA, "enter func %s. \n", __FUNCTION__);
	
	stat = container_of((struct delayed_work *)work,
			struct lsm303d_status, input_work_acc);

	mutex_lock(&stat->lock);
	err = lsm303d_acc_get_data(stat, xyz);
	if (err < 0)
		dev_err(&stat->client->dev, "get_accelerometer_data failed\n");
	else
		lsm303d_acc_report_values(stat, xyz);
		
	schedule_delayed_work(&stat->input_work_acc, usecs_to_jiffies(
			stat->pdata_acc->poll_interval));
	mutex_unlock(&stat->lock);
}

static void lsm303d_mag_input_work_func(struct work_struct *work)
{
	struct lsm303d_status *stat;
	int xyz[3] = { 0 };
	int err;

	dprintk(DEBUG_REPORT_MAG_DATA, "enter func %s. \n", __FUNCTION__);

	stat = container_of((struct delayed_work *)work,
			struct lsm303d_status, input_work_mag);

	mutex_lock(&stat->lock);
	err = lsm303d_mag_get_data(stat, xyz);
	if (err < 0)
		dev_err(&stat->client->dev, "get_magnetometer_data failed\n");
	else
		lsm303d_mag_report_values(stat, xyz);
		
	schedule_delayed_work(&stat->input_work_mag, usecs_to_jiffies(
			stat->pdata_mag->poll_interval));
	mutex_unlock(&stat->lock);
}

static int lsm303d_acc_input_init(struct lsm303d_status *stat)
{
	int err;

	INIT_DELAYED_WORK(&stat->input_work_acc, lsm303d_acc_input_work_func);
	stat->input_dev_acc = input_allocate_device();
	if (!stat->input_dev_acc) {
		err = -ENOMEM;
		dev_err(&stat->client->dev, "accelerometer "
					"input device allocation failed\n");
		goto err0;
	}

	stat->input_dev_acc->open = lsm303d_acc_input_open;
	stat->input_dev_acc->close = lsm303d_acc_input_close;
	stat->input_dev_acc->name = LSM303D_ACC_DEV_NAME;
	stat->input_dev_acc->id.bustype = BUS_I2C;
	stat->input_dev_acc->dev.parent = &stat->client->dev;

	input_set_drvdata(stat->input_dev_acc, stat);

	set_bit(EV_ABS, stat->input_dev_acc->evbit);

	input_set_abs_params(stat->input_dev_acc, ABS_X, 
					-ACC_G_MAX_NEG, ACC_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev_acc, ABS_Y, 
					-ACC_G_MAX_NEG, ACC_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev_acc, ABS_Z, 
					-ACC_G_MAX_NEG, ACC_G_MAX_POS, FUZZ, FLAT);

	err = input_register_device(stat->input_dev_acc);
	if (err) {
		dev_err(&stat->client->dev,
			"unable to register accelerometer input device %s\n",
				stat->input_dev_acc->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(stat->input_dev_acc);
err0:
	return err;
}

static int lsm303d_mag_input_init(struct lsm303d_status *stat)
{
	int err;

	INIT_DELAYED_WORK(&stat->input_work_mag, lsm303d_mag_input_work_func);
	stat->input_dev_mag = input_allocate_device();
	if (!stat->input_dev_mag) {
		err = -ENOMEM;
		dev_err(&stat->client->dev, "magnetometer "
					"input device allocation failed\n");
		goto err0;
	}

	stat->input_dev_mag->open = lsm303d_mag_input_open;
	stat->input_dev_mag->close = lsm303d_mag_input_close;
	stat->input_dev_mag->name = LSM303D_MAG_DEV_NAME;
	stat->input_dev_mag->id.bustype = BUS_I2C;
	stat->input_dev_mag->dev.parent = &stat->client->dev;

	input_set_drvdata(stat->input_dev_mag, stat);

	set_bit(EV_ABS, stat->input_dev_mag->evbit);

	input_set_abs_params(stat->input_dev_mag, ABS_X, 
					-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev_mag, ABS_Y, 
					-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);
	input_set_abs_params(stat->input_dev_mag, ABS_Z, 
					-MAG_G_MAX_NEG, MAG_G_MAX_POS, FUZZ, FLAT);

	err = input_register_device(stat->input_dev_mag);
	if (err) {
		dev_err(&stat->client->dev,
			"unable to register magnetometer input device %s\n",
				stat->input_dev_mag->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(stat->input_dev_mag);
err0:
	return err;
}

static void lsm303d_input_cleanup(struct lsm303d_status *stat)
{
	input_unregister_device(stat->input_dev_acc);

	input_unregister_device(stat->input_dev_mag);
}

static int lsm303d_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct lsm303d_status *stat;

	u32 smbus_func = I2C_FUNC_SMBUS_BYTE_DATA | 
			I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK;

	int err = -1;
	dev_info(&client->dev, "probe start.\n");

	dprintk(DEBUG_INIT, "lsm303d probe i2c address is %d \n",i2c_address[i2c_num]);
	client->addr = i2c_address[i2c_num];
	
	stat = kzalloc(sizeof(struct lsm303d_status), GFP_KERNEL);
	if (stat == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for module data: "
					"%d\n", err);
		goto exit_check_functionality_failed;
	}

	stat->use_smbus = 0;
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_warn(&client->dev, "client not i2c capable\n");
		if (i2c_check_functionality(client->adapter, smbus_func)){
			stat->use_smbus = 1;
			dev_warn(&client->dev, "client using SMBUS\n");
		} else {			
			err = -ENODEV;
			dev_err(&client->dev, "client nor SMBUS capable\n");
			goto exit_check_functionality_failed;
		}
	}

	mutex_init(&stat->lock);
	mutex_lock(&stat->lock);

	stat->client = client;
	i2c_set_clientdata(client, stat);

	stat->pdata_acc = kmalloc(sizeof(*stat->pdata_acc), GFP_KERNEL);
	stat->pdata_mag = kmalloc(sizeof(*stat->pdata_mag), GFP_KERNEL);
	if ((stat->pdata_acc == NULL)||(stat->pdata_mag == NULL)) {
		err = -ENOMEM;
		dev_err(&client->dev,
			"failed to allocate memory for pdata: %d\n", err);
		goto err_mutexunlock;
	}

	if (client->dev.platform_data == NULL) {
		memcpy(stat->pdata_acc, &default_lsm303d_acc_pdata,
						sizeof(*stat->pdata_acc));
		memcpy(stat->pdata_mag, &default_lsm303d_mag_pdata,
						sizeof(*stat->pdata_mag));
		dev_info(&client->dev, "using default plaform_data for "
					"accelerometer and magnetometer\n");
	} else {
		struct lsm303d_main_platform_data *tmp;
		tmp = kzalloc(sizeof(struct lsm303d_main_platform_data), 
								GFP_KERNEL);
		if(tmp == NULL)
			goto exit_kfree_pdata;
		memcpy(tmp, client->dev.platform_data, sizeof(*tmp));
		if(tmp->pdata_acc == NULL) {
			memcpy(stat->pdata_acc, &default_lsm303d_acc_pdata,
						sizeof(*stat->pdata_acc));
			dev_info(&client->dev, "using default plaform_data for "
							"accelerometer\n");
		} else {
			memcpy(stat->pdata_acc, tmp->pdata_acc, 
						sizeof(*stat->pdata_acc));
		}
		if(tmp->pdata_mag == NULL) {
			memcpy(stat->pdata_mag, &default_lsm303d_mag_pdata,
						sizeof(*stat->pdata_mag));
			dev_info(&client->dev, "using default plaform_data for "
							"magnetometer\n");
		} else {
			memcpy(stat->pdata_mag, tmp->pdata_mag, 
						sizeof(*stat->pdata_mag));
		}
		kfree(tmp);
	}

	err = lsm303d_acc_validate_pdata(stat);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data for "
							"accelerometer \n");
		goto exit_kfree_pdata;
	}

	err = lsm303d_mag_validate_pdata(stat);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data for "
							"magnetometer\n");
		goto exit_kfree_pdata;
	}

	if (stat->pdata_acc->init) {
		err = stat->pdata_acc->init();
		if (err < 0) {
			dev_err(&client->dev, "accelerometer init failed: "
								"%d\n", err);
			goto err_pdata_acc_init;
		}
	}
	if (stat->pdata_mag->init) {
		err = stat->pdata_mag->init();
		if (err < 0) {
			dev_err(&client->dev, "magnetometer init failed: "
								"%d\n", err);
			goto err_pdata_mag_init;
		}
	}

	err = lsm303d_hw_init(stat);
	if (err < 0) {
		dev_err(&client->dev, "hw init failed: %d\n", err);
		goto err_hw_init;
	}

	err = lsm303d_acc_device_power_on(stat);
	if (err < 0) {
		dev_err(&client->dev, "accelerometer power on failed: "
								"%d\n", err);
		goto err_pdata_init;
	}
	err = lsm303d_mag_device_power_on(stat);
	if (err < 0) {
		dev_err(&client->dev, "magnetometer power on failed: "
								"%d\n", err);
		goto err_pdata_init;
	}

	err = lsm303d_acc_update_fs_range(stat, stat->pdata_acc->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range on accelerometer "
								"failed\n");
		goto  err_power_off_acc;
	}

	err = lsm303d_mag_update_fs_range(stat, stat->pdata_mag->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range on magnetometer "
								"failed\n");
		goto  err_power_off_mag;
	}

	err = lsm303d_acc_update_odr(stat, stat->pdata_acc->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr on accelerometer failed\n");
		goto  err_power_off;
	}

	err = lsm303d_mag_update_odr(stat, stat->pdata_mag->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr on magnetometer failed\n");
		goto  err_power_off;
	}

	err = lsm303d_acc_update_filter(stat, 
					stat->pdata_acc->aa_filter_bandwidth);
	if (err < 0) {
		dev_err(&client->dev, "update_filter on accelerometer "
								"failed\n");
		goto  err_power_off;
	}

	err = lsm303d_acc_input_init(stat);
	if (err < 0) {
		dev_err(&client->dev, "accelerometer input init failed\n");
		goto err_power_off;
	}

	err = lsm303d_mag_input_init(stat);
	if (err < 0) {
		dev_err(&client->dev, "magnetometer input init failed\n");
		goto err_power_off;
	}

	err = create_sysfs_interfaces(&client->dev);
	if (err < 0) {
		dev_err(&client->dev,
		   "device LSM303D_DEV_NAME sysfs register failed\n");
		goto err_input_cleanup;
	}

	lsm303d_acc_device_power_off(stat);
	lsm303d_mag_device_power_off(stat);

	mutex_unlock(&stat->lock);

#ifdef CONFIG_HAS_EARLYSUSPEND
	stat->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	stat->early_suspend.suspend = lsm303d_early_suspend;
	stat->early_suspend.resume = lsm303d_late_resume;
	register_early_suspend(&stat->early_suspend);
#endif
	
	dev_info(&client->dev, "%s: probed\n", LSM303D_DEV_NAME);
	return 0;

err_input_cleanup:
	lsm303d_input_cleanup(stat);
err_power_off:
err_power_off_mag:
	lsm303d_mag_device_power_off(stat);
err_power_off_acc:
	lsm303d_acc_device_power_off(stat);
err_hw_init:
err_pdata_init:
err_pdata_mag_init:
	if (stat->pdata_mag->exit)
		stat->pdata_mag->exit();
err_pdata_acc_init:
	if (stat->pdata_acc->exit)
		stat->pdata_acc->exit();
exit_kfree_pdata:
	kfree(stat->pdata_acc);
	kfree(stat->pdata_mag);
err_mutexunlock:
	mutex_unlock(&stat->lock);
	//kfree(stat);
exit_check_functionality_failed:
	pr_err("%s: Driver Init failed\n", LSM303D_DEV_NAME);
	if(NULL != stat)
		kfree(stat);
	return err;
}

static int lsm303d_remove(struct i2c_client *client)
{
	struct lsm303d_status *stat = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&stat->early_suspend);
#endif

	lsm303d_acc_disable(stat);
	lsm303d_mag_disable(stat);
	lsm303d_acc_input_cleanup(stat);
	lsm303d_mag_input_cleanup(stat);

	remove_sysfs_interfaces(&client->dev);

	if (stat->pdata_acc->exit)
		stat->pdata_acc->exit();

	if (stat->pdata_mag->exit)
		stat->pdata_mag->exit();

	i2c_set_clientdata(client, NULL);

	kfree(stat->pdata_acc);
	kfree(stat->pdata_mag);
	kfree(stat);
	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void lsm303d_early_suspend(struct early_suspend *h)
{
	struct lsm303d_status *data =
		container_of(h, struct lsm303d_status, early_suspend);

	dprintk(DEBUG_SUSPEND, "%s: start\n", __func__);
	data->acc_on_before_suspend = atomic_read(&data->enabled_acc);
	data->mag_on_before_suspend = atomic_read(&data->enabled_mag);

	if (data->acc_on_before_suspend) {
		cancel_delayed_work_sync(&data->input_work_acc);
		lsm303d_acc_device_power_off(data);
	}
	if (data->mag_on_before_suspend) {
		cancel_delayed_work_sync(&data->input_work_mag);
		lsm303d_mag_device_power_off(data);
	}

	dprintk(DEBUG_SUSPEND, "%s: end\n", __func__);
	return;
}

static void lsm303d_late_resume(struct early_suspend *h)
{
	int err = 0;
	struct lsm303d_status *data =
		container_of(h, struct lsm303d_status, early_suspend);

	dprintk(DEBUG_SUSPEND, "%s: start\n", __func__);

	if (data->acc_on_before_suspend) {
		err = lsm303d_acc_device_power_on(data);
		if (err < 0) {
			atomic_set(&data->enabled_acc, 0);
			return;
		}
		schedule_delayed_work(&data->input_work_acc,
			usecs_to_jiffies(data->pdata_acc->poll_interval));
	}
	if (data->mag_on_before_suspend) {
		err = lsm303d_mag_device_power_on(data);
		if (err < 0) {
			atomic_set(&data->enabled_mag, 0);
			return;
		}
		schedule_delayed_work(&data->input_work_mag,
			usecs_to_jiffies(data->pdata_mag->poll_interval));
	}

	dprintk(DEBUG_SUSPEND, "%s: end\n", __func__);
	return;
}
#else
#ifdef CONFIG_PM
static int lsm303d_suspend(struct device *dev)
{
	int err = 0;
	
	struct i2c_client *client = to_i2c_client(dev);
	struct lsm303d_status *data = i2c_get_clientdata(client);

	dev_info(&client->dev, "suspend\n");

	data->acc_on_before_suspend = atomic_read(&data->enabled_acc);
	data->mag_on_before_suspend = atomic_read(&data->enabled_mag);

	if (data->acc_on_before_suspend) {
		cancel_delayed_work_sync(&data->input_work_acc);
		lsm303d_acc_device_power_off(data);
	}
	if (data->mag_on_before_suspend) {
		cancel_delayed_work_sync(&data->input_work_mag);
		lsm303d_mag_device_power_off(data);
	}

	return err;
}

static int lsm303d_resume(struct device *dev)
{
	int err = 0;

	struct i2c_client *client = to_i2c_client(dev);
	struct lsm303d_status *data = i2c_get_clientdata(client);

	dev_info(&client->dev, "resume\n");
	
	if (data->acc_on_before_suspend) {
		err = lsm303d_acc_device_power_on(data);
		if (err < 0) {
			atomic_set(&data->enabled_acc, 0);
			return err;
		}
		schedule_delayed_work(&data->input_work_acc,
			usecs_to_jiffies(data->pdata_acc->poll_interval));
	}
	if (data->mag_on_before_suspend) {
		err = lsm303d_mag_device_power_on(data);
		if (err < 0) {
			atomic_set(&data->enabled_mag, 0);
			return err;
		}
		schedule_delayed_work(&data->input_work_mag,
			usecs_to_jiffies(data->pdata_mag->poll_interval));
	}

	return err;
}
#endif /*CONFIG_PM*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/

static const struct i2c_device_id lsm303d_id[] 
					= { { LSM303D_DEV_NAME, 0 }, { }, };

MODULE_DEVICE_TABLE(i2c, lsm303d_id);

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static struct dev_pm_ops lsm303d_pm = {
	.suspend = lsm303d_suspend,
	.resume = lsm303d_resume,
};
#endif

static struct i2c_driver lsm303d_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
			.owner = THIS_MODULE,
			.name = LSM303D_DEV_NAME,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
			.pm = &lsm303d_pm,
#endif
		  },
	.probe = lsm303d_probe,
	.remove = lsm303d_remove,
	.id_table = lsm303d_id,
	.address_list	= normal_i2c,
};

static int __init lsm303d_init(void)
{
	pr_info("%s driver: init\n", LSM303D_DEV_NAME);
	
	if (e_compass_fetch_sysconfig_para()) {
		printk("%s: err.\n", __func__);
		return -1;
	}

	lsm303d_driver.detect = e_compass_detect;
	
	return i2c_add_driver(&lsm303d_driver);
}

static void __exit lsm303d_exit(void)
{
	pr_info("%s driver exit\n", LSM303D_DEV_NAME);
	i2c_del_driver(&lsm303d_driver);
}

module_init(lsm303d_init);
module_exit(lsm303d_exit);

MODULE_DESCRIPTION("lsm303d accelerometer and magnetometer driver");
MODULE_AUTHOR("Matteo Dameno, Denis Ciocca, STMicroelectronics");
MODULE_LICENSE("GPL");

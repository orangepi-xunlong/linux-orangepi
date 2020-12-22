/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
 *
 * File Name          : lis3dh_acc.c
 * Authors            : MSH - Motion Mems BU - Application Team
 *		      : Matteo Dameno (matteo.dameno@st.com)
 *		      : Carmine Iascone (carmine.iascone@st.com)
 *		      : Both authors are willing to be considered the contact
 *		      : and update points for the driver.
 * Version            : V.1.0.9
 * Date               : 2010/May/23
 * Description        : LIS3DH accelerometer sensor API
 *
 *******************************************************************************
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
 ******************************************************************************
 Revision 1.0.0 05/11/09
 First Release;
 Revision 1.0.3 22/01/2010
  Linux K&R Compliant Release;
 Revision 1.0.5 16/08/2010
  modified _get_acceleration_data function;
  modified _update_odr function;
  manages 2 interrupts;
 Revision 1.0.6 15/11/2010
  supports sysfs;
  no more support for ioctl;
 Revision 1.0.7 26/11/2010
  checks for availability of interrupts pins
  correction on FUZZ and FLAT values;
 Revision 1.0.8 2010/Apr/01
  corrects a bug in interrupt pin management in 1.0.7
 Revision 1.0.9: 2011/May/23
  update_odr func correction;
 Revision 1.0.9.1: 2012/Mar/21 by morris chen
  change the sysf attribute for android CTS
 ******************************************************************************/

#include <linux/err.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
/* #include	<linux/gpio.h> */
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/init-input.h>

#include "lis3dh.h"

#define	DEBUG               1

#define	G_MAX               16000

#define SENSITIVITY_2G      1	  /**	mg/LSB	*/
#define SENSITIVITY_4G      2	  /**	mg/LSB	*/
#define SENSITIVITY_8G      4	  /**	mg/LSB	*/
#define SENSITIVITY_16G     12	/**	mg/LSB	*/

/* Accelerometer Sensor Operating Mode */
#define LIS3DH_ACC_ENABLE   0x01
#define LIS3DH_ACC_DISABLE  0x00

#define	HIGH_RESOLUTION     0x08

#define	AXISDATA_REG        0x28
#define WHOAMI_LIS3DH_ACC   0x33	/*	Expected content for WAI */

/*	CONTROL REGISTERS	*/
#define WHO_AM_I            0x0F	/*	WhoAmI register		*/
#define	TEMP_CFG_REG        0x1F	/*	temper sens control reg	*/
/* ctrl 1: ODR3 ODR2 ODR ODR0 LPen Zenable Yenable Zenable */
#define	CTRL_REG1           0x20	/*	control reg 1		*/
#define	CTRL_REG2           0x21	/*	control reg 2		*/
#define	CTRL_REG3           0x22	/*	control reg 3		*/
#define	CTRL_REG4           0x23	/*	control reg 4		*/
#define	CTRL_REG5           0x24	/*	control reg 5		*/
#define	CTRL_REG6           0x25	/*	control reg 6		*/

#define	FIFO_CTRL_REG       0x2E	/*	FiFo control reg	*/

#define	INT_CFG1            0x30	/*	interrupt 1 config	*/
#define	INT_SRC1            0x31	/*	interrupt 1 source	*/
#define	INT_THS1            0x32	/*	interrupt 1 threshold	*/
#define	INT_DUR1            0x33	/*	interrupt 1 duration	*/


#define	TT_CFG              0x38	/*	tap config		*/
#define	TT_SRC              0x39	/*	tap source		*/
#define	TT_THS              0x3A	/*	tap threshold		*/
#define	TT_LIM              0x3B	/*	tap time limit		*/
#define	TT_TLAT             0x3C	/*	tap time latency	*/
#define	TT_TW               0x3D	/*	tap time window		*/
/*	end CONTROL REGISTRES	*/


#define ENABLE_HIGH_RESOLUTION	1

#define LIS3DH_ACC_PM_OFF   0x00
#define LIS3DH_ACC_ENABLE_ALL_AXES	0x07


#define PMODE_MASK          0x08
#define ODR_MASK            0XF0

#define ODR1                0x10  /* 1Hz output data rate */
#define ODR10               0x20  /* 10Hz output data rate */
#define ODR25               0x30  /* 25Hz output data rate */
#define ODR50               0x40  /* 50Hz output data rate */
#define ODR100              0x50  /* 100Hz output data rate */
#define ODR200              0x60  /* 200Hz output data rate */
#define ODR400              0x70  /* 400Hz output data rate */
#define ODR1250             0x90  /* 1250Hz output data rate */

#define	IA                  0x40
#define	ZH                  0x20
#define	ZL                  0x10
#define	YH                  0x08
#define	YL                  0x04
#define	XH                  0x02
#define	XL                  0x01
/* */
/* CTRL REG BITS*/
#define	CTRL_REG3_I1_AOI1   0x40
#define	CTRL_REG6_I2_TAPEN  0x80
#define	CTRL_REG6_HLACTIVE  0x02
/* */
#define NO_MASK             0xFF
#define INT1_DURATION_MASK  0x7F
#define	INT1_THRESHOLD_MASK 0x7F
#define TAP_CFG_MASK        0x3F
#define	TAP_THS_MASK        0x7F
#define	TAP_TLIM_MASK	      0x7F
#define	TAP_TLAT_MASK       NO_MASK
#define	TAP_TW_MASK         NO_MASK

/* TAP_SOURCE_REG BIT */
#define	DTAP                0x20
#define	STAP                0x10
#define	SIGNTAP             0x08
#define	ZTAP                0x04
#define	YTAP                0x02
#define	XTAZ                0x01

#define	FUZZ                0
#define	FLAT                0
#define	I2C_RETRY_DELAY     5
#define	I2C_RETRIES         5
#define	I2C_AUTO_INCREMENT  0x80

/* RESUME STATE INDICES */
#define	RES_CTRL_REG1       0
#define	RES_CTRL_REG2       1
#define	RES_CTRL_REG3       2
#define	RES_CTRL_REG4       3
#define	RES_CTRL_REG5       4
#define	RES_CTRL_REG6       5

#define	RES_INT_CFG1        6
#define	RES_INT_THS1        7
#define	RES_INT_DUR1        8

#define	RES_TT_CFG          9
#define	RES_TT_THS          10
#define	RES_TT_LIM          11
#define	RES_TT_TLAT         12
#define	RES_TT_TW           13

#define	RES_TEMP_CFG_REG    14
#define	RES_REFERENCE_REG   15
#define	RES_FIFO_CTRL_REG   16

#define	RESUME_ENTRIES      17
/* end RESUME STATE INDICES */

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_CONTROL_INFO = 1U << 1,	
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
	DEBUG_INT = 1U << 4,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(KERN_DEBUG fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);

static struct {
	unsigned int cutoff_ms;
	unsigned int mask;
} lis3dh_acc_odr_table[] = {
		{    1, ODR1250 },
		{    3, ODR400  },
		{    5, ODR200  },
		{   10, ODR100  },
		{   20, ODR50   },
		{   40, ODR25   },
		{  100, ODR10   },
		{ 1000, ODR1    },
};

struct lis3dh_acc_data {
	struct i2c_client *client;
	struct lis3dh_acc_platform_data *pdata;

	struct mutex lock;
	struct delayed_work input_work;

	struct input_dev *input_dev;

	int hw_initialized;
	/* hw_working=-1 means not tested yet */
	int hw_working;
	atomic_t enabled;
	int on_before_suspend;

	u8 sensitivity;

	u8 resume_state[RESUME_ENTRIES];

	int irq1;
	struct work_struct irq1_work;
	struct workqueue_struct *irq1_work_queue;
	int irq2;
	struct work_struct irq2_work;
	struct workqueue_struct *irq2_work_queue;

	#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	#endif

#ifdef DEBUG
	u8 reg_addr;
#endif
};

static struct lis3dh_acc_platform_data lis3dh_dev_data;

/* Addresses to scan */
static const unsigned short normal_i2c[] = {0x18, I2C_CLIENT_END};
static __u32 twi_id = 0;
static int i2c_num = 0;
static const unsigned short i2c_address[3] = {0x18,0x19,0x38};
static struct sensor_config_info gsensor_info = {
	.input_type = GSENSOR_TYPE,
};

static void lis3dh_acc_set_data(struct lis3dh_acc_platform_data *dev_data)
{
	dev_data->poll_interval = 400;
	dev_data->min_interval = 100;
	dev_data->gpio_int1 = -1;
	dev_data->gpio_int2 = -1;
	dev_data->axis_map_x = 0;
	dev_data->axis_map_y = 1;
	dev_data->axis_map_z = 2;
	dev_data->g_range = LIS3DH_ACC_G_2G;
	dev_data->negate_x = 0;
	dev_data->negate_y = 0;
	dev_data->negate_z = 0;
	dev_data->exit = NULL;
	dev_data->init = NULL;
	dev_data->power_off = NULL;
	dev_data->power_on = NULL;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void lis3dh_early_suspend(struct early_suspend *h);
static void lis3dh_late_resume(struct early_suspend *h);
#endif

static int lis3dh_acc_i2c_read(struct lis3dh_acc_data *acc,
				u8 * buf, int len)
{
	int err;
	int tries = 0;

	struct i2c_msg	msgs[] = {
		{
			.addr = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
			.len = 1,
			.buf = buf,
		},
		{
			.addr = acc->client->addr,
			.flags = (acc->client->flags & I2C_M_TEN) | I2C_M_RD,
			.len = len,
			.buf = buf,
		},
	};

	do {
		err = i2c_transfer(acc->client->adapter, msgs, 2);
		if (err != 2)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 2) && (++tries < I2C_RETRIES));

	if (err != 2) {
		dev_err(&acc->client->dev, "read transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
}

static int lis3dh_acc_i2c_write(struct lis3dh_acc_data *acc, u8 * buf, int len)
{
	int err;
	int tries = 0;
	struct i2c_msg msgs[] = {
		{
		 .addr = acc->client->addr,
			.flags = acc->client->flags & I2C_M_TEN,
		 .len = len + 1,
		 .buf = buf,
		 },
	};

	do {
		err = i2c_transfer(acc->client->adapter, msgs, 1);
		if (err != 1)
			msleep_interruptible(I2C_RETRY_DELAY);
	} while ((err != 1) && (++tries < I2C_RETRIES));

	if (err != 1) {
		dev_err(&acc->client->dev, "write transfer error\n");
		err = -EIO;
	} else {
		err = 0;
	}

	return err;
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
	
	lis3dh_acc_set_data(&lis3dh_dev_data);
            
	if (twi_id == adapter->nr) {
		for(i2c_num = 0; i2c_num < (sizeof(i2c_address)/sizeof(i2c_address[0]));i2c_num++)
		{
			client->addr = i2c_address[i2c_num];
			pr_info("%s:addr= 0x%x,i2c_num:%d\n",__func__,client->addr,i2c_num);
			ret = i2c_smbus_read_byte_data(client,WHO_AM_I);
			pr_info("Read ID value is :%d",ret);
			if ((ret &0x00FF) == WHOAMI_LIS3DH_ACC) {
				pr_info("lis3dh_acc Device detected!\n");
    				strlcpy(info->type, SENSOR_NAME, I2C_NAME_SIZE);
				info->platform_data = &lis3dh_dev_data;
				return 0; 
			}                                                        
		}
        
		pr_info("%s:lis3dh_acc Device not found, \
			maybe the other gsensor equipment! \n",__func__);
		return -ENODEV;
	} else {
		return -ENODEV;
	}
}


static int lis3dh_acc_hw_init(struct lis3dh_acc_data *acc)
{
	int err = -1;
	u8 buf[7];

	dprintk(DEBUG_INIT, "%s: hw init start\n", LIS3DH_ACC_DEV_NAME);

	buf[0] = WHO_AM_I;
	err = lis3dh_acc_i2c_read(acc, buf, 1);
	if (err < 0) {
	dev_warn(&acc->client->dev, "Error reading WHO_AM_I: is device "
		"available/working?\n");
		goto err_firstread;
	} else
		acc->hw_working = 1;
	if (buf[0] != WHOAMI_LIS3DH_ACC) {
	dev_err(&acc->client->dev,
		"device unknown. Expected: 0x%x,"
		" Replies: 0x%x\n", WHOAMI_LIS3DH_ACC, buf[0]);
		err = -1; /* choose the right coded error */
		goto err_unknown_device;
	}

	buf[0] = CTRL_REG1;
	buf[1] = acc->resume_state[RES_CTRL_REG1];
	err = lis3dh_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = TEMP_CFG_REG;
	buf[1] = acc->resume_state[RES_TEMP_CFG_REG];
	err = lis3dh_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = FIFO_CTRL_REG;
	buf[1] = acc->resume_state[RES_FIFO_CTRL_REG];
	err = lis3dh_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = (I2C_AUTO_INCREMENT | TT_THS);
	buf[1] = acc->resume_state[RES_TT_THS];
	buf[2] = acc->resume_state[RES_TT_LIM];
	buf[3] = acc->resume_state[RES_TT_TLAT];
	buf[4] = acc->resume_state[RES_TT_TW];
	err = lis3dh_acc_i2c_write(acc, buf, 4);
	if (err < 0)
		goto err_resume_state;
	buf[0] = TT_CFG;
	buf[1] = acc->resume_state[RES_TT_CFG];
	err = lis3dh_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		goto err_resume_state;

	buf[0] = (I2C_AUTO_INCREMENT | INT_THS1);
	buf[1] = acc->resume_state[RES_INT_THS1];
	buf[2] = acc->resume_state[RES_INT_DUR1];
	err = lis3dh_acc_i2c_write(acc, buf, 2);
	if (err < 0)
		goto err_resume_state;
	buf[0] = INT_CFG1;
	buf[1] = acc->resume_state[RES_INT_CFG1];
	err = lis3dh_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		goto err_resume_state;


	buf[0] = (I2C_AUTO_INCREMENT | CTRL_REG2);
	buf[1] = acc->resume_state[RES_CTRL_REG2];
	buf[2] = acc->resume_state[RES_CTRL_REG3];
	buf[3] = acc->resume_state[RES_CTRL_REG4];
	buf[4] = acc->resume_state[RES_CTRL_REG5];
	buf[5] = acc->resume_state[RES_CTRL_REG6];
	err = lis3dh_acc_i2c_write(acc, buf, 5);
	if (err < 0)
		goto err_resume_state;

	acc->hw_initialized = 1;
	dprintk(DEBUG_INIT, "%s: hw init done\n", LIS3DH_ACC_DEV_NAME);
	return 0;

err_firstread:
	acc->hw_working = 0;
err_unknown_device:
err_resume_state:
	acc->hw_initialized = 0;
	dev_err(&acc->client->dev, "hw init error 0x%x,0x%x: %d\n", buf[0],
			buf[1], err);
	return err;
}

static void lis3dh_acc_device_power_off(struct lis3dh_acc_data *acc)
{
	int err;
	u8 buf[2] = { CTRL_REG1, LIS3DH_ACC_PM_OFF };

	err = lis3dh_acc_i2c_write(acc, buf, 1);
	if (err < 0)
		dev_err(&acc->client->dev, "soft power off failed: %d\n", err);

	if (acc->pdata->power_off) {
		if(acc->pdata->gpio_int1 >= 0)
			disable_irq_nosync(acc->irq1);
		if(acc->pdata->gpio_int2 >= 0)
			disable_irq_nosync(acc->irq2);
		acc->pdata->power_off();
		acc->hw_initialized = 0;
	}
	if (acc->hw_initialized) {
		if(acc->pdata->gpio_int1 >= 0)
			disable_irq_nosync(acc->irq1);
		if(acc->pdata->gpio_int2 >= 0)
			disable_irq_nosync(acc->irq2);
		acc->hw_initialized = 0;
	}

}

static int lis3dh_acc_device_power_on(struct lis3dh_acc_data *acc)
{
	int err = -1;

	if (acc->pdata->power_on) {
		err = acc->pdata->power_on();
		if (err < 0) {
			dev_err(&acc->client->dev,
					"power_on failed: %d\n", err);
			return err;
		}
		if(acc->pdata->gpio_int1 >= 0)
			enable_irq(acc->irq1);
		if(acc->pdata->gpio_int2 >= 0)
			enable_irq(acc->irq2);
	}

	if (!acc->hw_initialized) {
		err = lis3dh_acc_hw_init(acc);
		if (acc->hw_working == 1 && err < 0) {
			lis3dh_acc_device_power_off(acc);
			return err;
		}
	}

	if (acc->hw_initialized) {
		if(acc->pdata->gpio_int1 >= 0)
			enable_irq(acc->irq1);
		if(acc->pdata->gpio_int2 >= 0)
			enable_irq(acc->irq2);
	}
	return 0;
}

static irqreturn_t lis3dh_acc_isr1(int irq, void *dev)
{
	struct lis3dh_acc_data *acc = dev;

	disable_irq_nosync(irq);
	queue_work(acc->irq1_work_queue, &acc->irq1_work);
	dprintk(DEBUG_INT, "%s: isr1 queued\n", LIS3DH_ACC_DEV_NAME);

	return IRQ_HANDLED;
}

static irqreturn_t lis3dh_acc_isr2(int irq, void *dev)
{
	struct lis3dh_acc_data *acc = dev;

	disable_irq_nosync(irq);
	queue_work(acc->irq2_work_queue, &acc->irq2_work);
	dprintk(DEBUG_INT, "%s: isr2 queued\n", LIS3DH_ACC_DEV_NAME);

	return IRQ_HANDLED;
}

static void lis3dh_acc_irq1_work_func(struct work_struct *work)
{

	struct lis3dh_acc_data *acc =
	container_of(work, struct lis3dh_acc_data, irq1_work);
	/* TODO  add interrupt service procedure.
		 ie:lis3dh_acc_get_int1_source(acc); */
	;
	/*  */
	dprintk(DEBUG_INT, "%s: IRQ1 triggered\n", LIS3DH_ACC_DEV_NAME);
//exit:
	enable_irq(acc->irq1);
}

static void lis3dh_acc_irq2_work_func(struct work_struct *work)
{

	struct lis3dh_acc_data *acc =
	container_of(work, struct lis3dh_acc_data, irq2_work);
	/* TODO  add interrupt service procedure.
		 ie:lis3dh_acc_get_tap_source(acc); */
	;
	/*  */

	dprintk(DEBUG_INT, "%s: IRQ2 triggered\n", LIS3DH_ACC_DEV_NAME);
//exit:
	enable_irq(acc->irq2);
}

static int lis3dh_acc_update_g_range(struct lis3dh_acc_data *acc, u8 new_g_range)
{
	int err=-1;

	u8 sensitivity;
	u8 buf[2];
	u8 updated_val;
	u8 init_val;
	u8 new_val;
	u8 mask = LIS3DH_ACC_FS_MASK | HIGH_RESOLUTION;

	switch (new_g_range) {
	case LIS3DH_ACC_G_2G:

		sensitivity = SENSITIVITY_2G;
		break;
	case LIS3DH_ACC_G_4G:

		sensitivity = SENSITIVITY_4G;
		break;
	case LIS3DH_ACC_G_8G:

		sensitivity = SENSITIVITY_8G;
		break;
	case LIS3DH_ACC_G_16G:

		sensitivity = SENSITIVITY_16G;
		break;
	default:
		dev_err(&acc->client->dev, "invalid g range requested: %u\n",
				new_g_range);
		return -EINVAL;
	}

	if (atomic_read(&acc->enabled)) {
		/* Updates configuration register 4,
		* which contains g range setting */
		buf[0] = CTRL_REG4;
		err = lis3dh_acc_i2c_read(acc, buf, 1);
		if (err < 0)
			goto error;
		init_val = buf[0];
		acc->resume_state[RES_CTRL_REG4] = init_val;
		new_val = new_g_range | HIGH_RESOLUTION;
		updated_val = ((mask & new_val) | ((~mask) & init_val));
		buf[1] = updated_val;
		buf[0] = CTRL_REG4;
		err = lis3dh_acc_i2c_write(acc, buf, 1);
		if (err < 0)
			goto error;
		acc->resume_state[RES_CTRL_REG4] = updated_val;
		acc->sensitivity = sensitivity;
	}


	return err;
error:
	dev_err(&acc->client->dev, "update g range failed 0x%x,0x%x: %d\n",
			buf[0], buf[1], err);

	return err;
}

static int lis3dh_acc_update_odr(struct lis3dh_acc_data *acc, int poll_interval_ms)
{
	int err = -1;
	int i;
	u8 config[2];

	/* Following, looks for the longest possible odr interval scrolling the
	 * odr_table vector from the end (shortest interval) backward (longest
	 * interval), to support the poll_interval requested by the system.
	 * It must be the longest interval lower then the poll interval.*/
	for (i = ARRAY_SIZE(lis3dh_acc_odr_table) - 1; i >= 0; i--) {
		if ((lis3dh_acc_odr_table[i].cutoff_ms <= poll_interval_ms)
								|| (i == 0))
			break;
	}
	config[1] = lis3dh_acc_odr_table[i].mask;

	config[1] |= LIS3DH_ACC_ENABLE_ALL_AXES;

	/* If device is currently enabled, we need to write new
	 *  configuration out to it */
	if (atomic_read(&acc->enabled)) {
		config[0] = CTRL_REG1;
		err = lis3dh_acc_i2c_write(acc, config, 1);
		if (err < 0)
			goto error;
		acc->resume_state[RES_CTRL_REG1] = config[1];
	}

	return err;

error:
	dev_err(&acc->client->dev, "update odr failed 0x%x,0x%x: %d\n",
			config[0], config[1], err);

	return err;
}



static int lis3dh_acc_register_write(struct lis3dh_acc_data *acc, u8 *buf,
		u8 reg_address, u8 new_value)
{
	int err = -1;

	/* Sets configuration register at reg_address
	 *  NOTE: this is a straight overwrite  */
		buf[0] = reg_address;
		buf[1] = new_value;
		err = lis3dh_acc_i2c_write(acc, buf, 1);
		if (err < 0)
			return err;
	return err;
}

/*static int lis3dh_acc_register_read(struct lis3dh_acc_data *acc, u8 *buf,
		u8 reg_address)
{

	int err = -1;
	buf[0] = (reg_address);
	err = lis3dh_acc_i2c_read(acc, buf, 1);
	return err;
}

static int lis3dh_acc_register_update(struct lis3dh_acc_data *acc, u8 *buf,
		u8 reg_address, u8 mask, u8 new_bit_values)
{
	int err = -1;
	u8 init_val;
	u8 updated_val;
	err = lis3dh_acc_register_read(acc, buf, reg_address);
	if (!(err < 0)) {
		init_val = buf[1];
		updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
		err = lis3dh_acc_register_write(acc, buf, reg_address,
				updated_val);
	}
	return err;
}*/



static int lis3dh_acc_get_acceleration_data(struct lis3dh_acc_data *acc,
		int *xyz)
{
	int err = -1;
	/* Data bytes from hardware xL, xH, yL, yH, zL, zH */
	u8 acc_data[6];
	/* x,y,z hardware data */
	s16 hw_d[3] = { 0 };

	acc_data[0] = (I2C_AUTO_INCREMENT | AXISDATA_REG);
	err = lis3dh_acc_i2c_read(acc, acc_data, 6);
	if (err < 0)
		return err;

	hw_d[0] = (((s16) ((acc_data[1] << 8) | acc_data[0])) >> 4);
	hw_d[1] = (((s16) ((acc_data[3] << 8) | acc_data[2])) >> 4);
	hw_d[2] = (((s16) ((acc_data[5] << 8) | acc_data[4])) >> 4);

	hw_d[0] = hw_d[0] * acc->sensitivity;
	hw_d[1] = hw_d[1] * acc->sensitivity;
	hw_d[2] = hw_d[2] * acc->sensitivity;


	xyz[0] = ((acc->pdata->negate_x) ? (-hw_d[acc->pdata->axis_map_x])
		   : (hw_d[acc->pdata->axis_map_x]));
	xyz[1] = ((acc->pdata->negate_y) ? (-hw_d[acc->pdata->axis_map_y])
		   : (hw_d[acc->pdata->axis_map_y]));
	xyz[2] = ((acc->pdata->negate_z) ? (-hw_d[acc->pdata->axis_map_z])
		   : (hw_d[acc->pdata->axis_map_z]));

	#ifdef DEBUG
	/*
		printk(KERN_INFO "%s read x=%d, y=%d, z=%d\n",
			LIS3DH_ACC_DEV_NAME, xyz[0], xyz[1], xyz[2]);
	*/
	#endif
	return err;
}

static void lis3dh_acc_report_values(struct lis3dh_acc_data *acc,
					int *xyz)
{
	dprintk(DEBUG_DATA_INFO, "x= 0x%hx, y = 0x%hx, z = 0x%hx\n", xyz[0], xyz[1], xyz[2]);
	input_report_abs(acc->input_dev, ABS_X, xyz[0]);
	input_report_abs(acc->input_dev, ABS_Y, xyz[1]);
	input_report_abs(acc->input_dev, ABS_Z, xyz[2]);
	input_sync(acc->input_dev);
}

static int lis3dh_acc_enable(struct lis3dh_acc_data *acc)
{
	int err;

	if (!atomic_cmpxchg(&acc->enabled, 0, 1)) {
		err = lis3dh_acc_device_power_on(acc);
		if (err < 0) {
			atomic_set(&acc->enabled, 0);
			return err;
		}
		schedule_delayed_work(&acc->input_work,
		msecs_to_jiffies(acc->pdata->poll_interval));
		dprintk(DEBUG_CONTROL_INFO, "lis3dh_acc enable\n");
	}

	return 0;
}

static int lis3dh_acc_disable(struct lis3dh_acc_data *acc)
{
	if (atomic_cmpxchg(&acc->enabled, 1, 0)) {
		cancel_delayed_work_sync(&acc->input_work);
		lis3dh_acc_device_power_off(acc);
		dprintk(DEBUG_CONTROL_INFO, "lis3dh_acc disable\n");
	}

	return 0;
}


static ssize_t read_single_reg(struct device *dev, char *buf, u8 reg)
{
	ssize_t ret;
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	int err;

	u8 data = reg;
	err = lis3dh_acc_i2c_read(acc, &data, 1);
	if (err < 0)
		return err;
	ret = sprintf(buf, "0x%02x\n", data);
	return ret;

}

static int write_reg(struct device *dev, const char *buf, u8 reg,
		u8 mask, int resumeIndex)
{
	int err = -1;
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	u8 x[2];
	u8 new_val;
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	new_val=((u8) val & mask);
	x[0] = reg;
	x[1] = new_val;
	err = lis3dh_acc_register_write(acc, x,reg,new_val);
	if (err < 0)
		return err;
	acc->resume_state[resumeIndex] = new_val;
	return err;
}

static ssize_t attr_get_polling_rate(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int val;
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	mutex_lock(&acc->lock);
	val = acc->pdata->poll_interval;
	mutex_unlock(&acc->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_polling_rate(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	unsigned long interval_ms;

	if (strict_strtoul(buf, 10, &interval_ms))
		return -EINVAL;
	if (!interval_ms)
		return -EINVAL;
	interval_ms = max((unsigned int)interval_ms,acc->pdata->min_interval);
	mutex_lock(&acc->lock);
	acc->pdata->poll_interval = interval_ms;
	lis3dh_acc_update_odr(acc, interval_ms);
	mutex_unlock(&acc->lock);
	return size;
}

static ssize_t attr_get_range(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	char val;
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	char range = 2;
	mutex_lock(&acc->lock);
	val = acc->pdata->g_range ;
	switch (val) {
	case LIS3DH_ACC_G_2G:
		range = 2;
		break;
	case LIS3DH_ACC_G_4G:
		range = 4;
		break;
	case LIS3DH_ACC_G_8G:
		range = 8;
		break;
	case LIS3DH_ACC_G_16G:
		range = 16;
		break;
	}
	mutex_unlock(&acc->lock);
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_set_range(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	unsigned long val;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	mutex_lock(&acc->lock);
	acc->pdata->g_range = val;
	lis3dh_acc_update_g_range(acc, val);
	mutex_unlock(&acc->lock);
	return size;
}

static ssize_t attr_get_enable(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	int val = atomic_read(&acc->enabled);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_set_enable(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		lis3dh_acc_enable(acc);
	else
		lis3dh_acc_disable(acc);

	return size;
}

static ssize_t attr_set_intconfig1(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev,buf,INT_CFG1,NO_MASK,RES_INT_CFG1);
}

static ssize_t attr_get_intconfig1(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev,buf,INT_CFG1);
}

static ssize_t attr_set_duration1(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev,buf,INT_DUR1,INT1_DURATION_MASK,RES_INT_DUR1);
}

static ssize_t attr_get_duration1(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev,buf,INT_DUR1);
}

static ssize_t attr_set_thresh1(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev,buf,INT_THS1,INT1_THRESHOLD_MASK,RES_INT_THS1);
}

static ssize_t attr_get_thresh1(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev,buf,INT_THS1);
}

static ssize_t attr_get_source1(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return read_single_reg(dev,buf,INT_SRC1);
}

static ssize_t attr_set_click_cfg(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev,buf,TT_CFG,TAP_CFG_MASK,RES_TT_CFG);
}

static ssize_t attr_get_click_cfg(struct device *dev,
		struct device_attribute *attr,	char *buf)
{

	return read_single_reg(dev,buf,TT_CFG);
}

static ssize_t attr_get_click_source(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev,buf,TT_SRC);
}

static ssize_t attr_set_click_ths(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev,buf,TT_THS,TAP_THS_MASK,RES_TT_THS);
}

static ssize_t attr_get_click_ths(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev,buf,TT_THS);
}

static ssize_t attr_set_click_tlim(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev,buf,TT_LIM,TAP_TLIM_MASK,RES_TT_LIM);
}

static ssize_t attr_get_click_tlim(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev,buf,TT_LIM);
}

static ssize_t attr_set_click_tlat(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev,buf,TT_TLAT,TAP_TLAT_MASK,RES_TT_TLAT);
}

static ssize_t attr_get_click_tlat(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev,buf,TT_TLAT);
}

static ssize_t attr_set_click_tw(struct device *dev,
		struct device_attribute *attr,	const char *buf, size_t size)
{
	return write_reg(dev,buf,TT_TLAT,TAP_TW_MASK,RES_TT_TLAT);
}

static ssize_t attr_get_click_tw(struct device *dev,
		struct device_attribute *attr,	char *buf)
{
	return read_single_reg(dev,buf,TT_TLAT);
}


#ifdef DEBUG
/* PAY ATTENTION: These DEBUG funtions don't manage resume_state */
static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	u8 x[2];
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&acc->lock);
	x[0] = acc->reg_addr;
	mutex_unlock(&acc->lock);
	x[1] = val;
	rc = lis3dh_acc_i2c_write(acc, x, 1);
	/*TODO: error need to be managed */
	return size;
}

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t ret;
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	int rc;
	u8 data;

	mutex_lock(&acc->lock);
	data = acc->reg_addr;
	mutex_unlock(&acc->lock);
	rc = lis3dh_acc_i2c_read(acc, &data, 1);
	/*TODO: error need to be managed */
	ret = sprintf(buf, "0x%02x\n", data);
	return ret;
}

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct lis3dh_acc_data *acc = dev_get_drvdata(dev);
	unsigned long val;
	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&acc->lock);
	acc->reg_addr = val;
	mutex_unlock(&acc->lock);
	return size;
}
#endif

static DEVICE_ATTR(delay, 0664,
		attr_get_polling_rate, attr_set_polling_rate);
static DEVICE_ATTR(range, 0664,
		attr_get_range, attr_set_range);
static DEVICE_ATTR(enable, 0664,
		attr_get_enable, attr_set_enable);
static DEVICE_ATTR(int1_config, 0664,
		attr_get_intconfig1, attr_set_intconfig1);
static DEVICE_ATTR(int1_duration, 0664,
		attr_get_duration1, attr_set_duration1);
static DEVICE_ATTR(int1_threshold, 0664,
		attr_get_thresh1, attr_set_thresh1);
static DEVICE_ATTR(int1_source, 0444,
		attr_get_source1, NULL);
static DEVICE_ATTR(click_config, 0664,
		attr_get_click_cfg, attr_set_click_cfg);
static DEVICE_ATTR(click_source, 0444,
		attr_get_click_source, NULL);
static DEVICE_ATTR(click_threshold, 0664,
		attr_get_click_ths, attr_set_click_ths);
static DEVICE_ATTR(click_timelimit, 0664,
		attr_get_click_tlim, attr_set_click_tlim);
static DEVICE_ATTR(click_timelatency, 0664,
		attr_get_click_tlat, attr_set_click_tlat);
static DEVICE_ATTR(click_timewindow, 0664,
		attr_get_click_tw, attr_set_click_tw);
#ifdef DEBUG
static DEVICE_ATTR(reg_value, 0600,
		attr_reg_get, attr_reg_set);
static DEVICE_ATTR(reg_addr, 0200,
		NULL, attr_addr_set);
#endif


static struct attribute *lis3de_attributes[] = {
	&dev_attr_delay.attr,
	&dev_attr_range.attr,
	&dev_attr_enable.attr,
	&dev_attr_int1_config.attr,
	&dev_attr_int1_duration.attr,
	&dev_attr_int1_threshold.attr,
	&dev_attr_int1_source.attr,
	&dev_attr_click_config.attr,
	&dev_attr_click_source.attr,
	&dev_attr_click_threshold.attr,
	&dev_attr_click_timelimit.attr,
	&dev_attr_click_timelatency.attr,
	&dev_attr_click_timewindow.attr,
#ifdef DEBUG
	&dev_attr_reg_value.attr,
	&dev_attr_reg_addr.attr,
#endif
	NULL
};

static struct attribute_group lis3de_attribute_group = {
	.attrs = lis3de_attributes
};


static int create_sysfs_interfaces(struct device *dev)
{
	int err = 0;
	
	err = sysfs_create_group(&dev->kobj, &lis3de_attribute_group);
	if (err < 0)
		goto error;
	return 0;

error:
	
	sysfs_remove_group(&dev->kobj, &lis3de_attribute_group);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &lis3de_attribute_group);
	return 0;
}


static void lis3dh_acc_input_work_func(struct work_struct *work)
{
	struct lis3dh_acc_data *acc;

	int xyz[3] = { 0 };
	int err;

	acc = container_of((struct delayed_work *)work,
			struct lis3dh_acc_data,	input_work);

	mutex_lock(&acc->lock);
	err = lis3dh_acc_get_acceleration_data(acc, xyz);
	if (err < 0)
		dev_err(&acc->client->dev, "get_acceleration_data failed\n");
	else
		lis3dh_acc_report_values(acc, xyz);

	schedule_delayed_work(&acc->input_work, msecs_to_jiffies(
			acc->pdata->poll_interval));
	mutex_unlock(&acc->lock);
}

static int lis3dh_acc_input_open(struct input_dev *input)
{
	struct lis3dh_acc_data *acc = input_get_drvdata(input);

	return lis3dh_acc_enable(acc);
}

static void lis3dh_acc_input_close(struct input_dev *dev)
{
	struct lis3dh_acc_data *acc = input_get_drvdata(dev);

	lis3dh_acc_disable(acc);
}

static int lis3dh_acc_validate_pdata(struct lis3dh_acc_data *acc)
{
	/* checks for correctness of minimal polling period */
	acc->pdata->min_interval =
		max((unsigned int)LIS3DLH_ACC_MIN_POLL_PERIOD_MS,
						acc->pdata->min_interval);

	acc->pdata->poll_interval = max(acc->pdata->poll_interval,
			acc->pdata->min_interval);

	if (acc->pdata->axis_map_x > 2 ||
		acc->pdata->axis_map_y > 2 ||
		 acc->pdata->axis_map_z > 2) {
		dev_err(&acc->client->dev, "invalid axis_map value "
			"x:%u y:%u z%u\n", acc->pdata->axis_map_x,
				acc->pdata->axis_map_y, acc->pdata->axis_map_z);
		return -EINVAL;
	}

	/* Only allow 0 and 1 for negation boolean flag */
	if (acc->pdata->negate_x > 1 || acc->pdata->negate_y > 1
			|| acc->pdata->negate_z > 1) {
		dev_err(&acc->client->dev, "invalid negate value "
			"x:%u y:%u z:%u\n", acc->pdata->negate_x,
				acc->pdata->negate_y, acc->pdata->negate_z);
		return -EINVAL;
	}

	/* Enforce minimum polling interval */
	if (acc->pdata->poll_interval < acc->pdata->min_interval) {
		dev_err(&acc->client->dev, "minimum poll interval violated\n");
		return -EINVAL;
	}

	return 0;
}

static int lis3dh_acc_input_init(struct lis3dh_acc_data *acc)
{
	int err;

	INIT_DELAYED_WORK(&acc->input_work, lis3dh_acc_input_work_func);
	acc->input_dev = input_allocate_device();
	if (!acc->input_dev) {
		err = -ENOMEM;
		dev_err(&acc->client->dev, "input device allocation failed\n");
		goto err0;
	}

	acc->input_dev->open = lis3dh_acc_input_open;
	acc->input_dev->close = lis3dh_acc_input_close;
	acc->input_dev->name = LIS3DH_ACC_DEV_NAME;
	//acc->input_dev->name = "accelerometer";
	acc->input_dev->id.bustype = BUS_I2C;
	//acc->input_dev->dev.parent = &acc->client->dev;

	input_set_drvdata(acc->input_dev, acc);

	set_bit(EV_ABS, acc->input_dev->evbit);
	/*	next is used for interruptA sources data if the case */
	set_bit(ABS_MISC, acc->input_dev->absbit);
	/*	next is used for interruptB sources data if the case */
	set_bit(ABS_WHEEL, acc->input_dev->absbit);

	input_set_abs_params(acc->input_dev, ABS_X, -G_MAX, G_MAX, FUZZ, FLAT);
	input_set_abs_params(acc->input_dev, ABS_Y, -G_MAX, G_MAX, FUZZ, FLAT);
	input_set_abs_params(acc->input_dev, ABS_Z, -G_MAX, G_MAX, FUZZ, FLAT);
	/*	next is used for interruptA sources data if the case */
	input_set_abs_params(acc->input_dev, ABS_MISC, INT_MIN, INT_MAX, 0, 0);
	/*	next is used for interruptB sources data if the case */
	input_set_abs_params(acc->input_dev, ABS_WHEEL, INT_MIN, INT_MAX, 0, 0);


	err = input_register_device(acc->input_dev);
	if (err) {
		dev_err(&acc->client->dev,
				"unable to register input device %s\n",
				acc->input_dev->name);
		goto err1;
	}

	return 0;

err1:
	input_free_device(acc->input_dev);
err0:
	return err;
}

static void lis3dh_acc_input_cleanup(struct lis3dh_acc_data *acc)
{
	input_unregister_device(acc->input_dev);
}

static int lis3dh_acc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{

	struct lis3dh_acc_data *acc;

	int err = -1;

	pr_info("%s: probe start.\n", LIS3DH_ACC_DEV_NAME);

	dprintk(DEBUG_INIT, "lis3dh_acc probe i2c address is %d \n",i2c_address[i2c_num]);
	client->addr =i2c_address[i2c_num];

	if (client->dev.platform_data == NULL) {
		dev_err(&client->dev, "platform data is NULL. exiting.\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "client not i2c capable\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	/*
	if (!i2c_check_functionality(client->adapter,
					I2C_FUNC_SMBUS_BYTE |
					I2C_FUNC_SMBUS_BYTE_DATA |
					I2C_FUNC_SMBUS_WORD_DATA)) {
		dev_err(&client->dev, "client not smb-i2c capable:2\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}


	if (!i2c_check_functionality(client->adapter,
						I2C_FUNC_SMBUS_I2C_BLOCK)){
		dev_err(&client->dev, "client not smb-i2c capable:3\n");
		err = -EIO;
		goto exit_check_functionality_failed;
	}
	*/

	acc = kzalloc(sizeof(struct lis3dh_acc_data), GFP_KERNEL);
	if (acc == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for module data: "
					"%d\n", err);
		goto exit_check_functionality_failed;
	}


	mutex_init(&acc->lock);
	mutex_lock(&acc->lock);

	acc->client = client;
	i2c_set_clientdata(client, acc);

	acc->pdata = kmalloc(sizeof(*acc->pdata), GFP_KERNEL);
	if (acc->pdata == NULL) {
		err = -ENOMEM;
		dev_err(&client->dev,
				"failed to allocate memory for pdata: %d\n",
				err);
		goto err_mutexunlock;
	}

	memcpy(acc->pdata, client->dev.platform_data, sizeof(*acc->pdata));

	err = lis3dh_acc_validate_pdata(acc);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto exit_kfree_pdata;
	}


	if (acc->pdata->init) {
		err = acc->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err_pdata_init;
		}
	}

	if(acc->pdata->gpio_int1 >= 0){
		/* acc->irq1 = gpio_to_irq(acc->pdata->gpio_int1); */
		dprintk(DEBUG_INIT, "%s: %s has set irq1 to irq: %d "
							"mapped on gpio:%d\n",
			LIS3DH_ACC_DEV_NAME, __func__, acc->irq1,
							acc->pdata->gpio_int1);
	}

	if(acc->pdata->gpio_int2 >= 0){
		/*acc->irq2 = gpio_to_irq(acc->pdata->gpio_int2); */
		dprintk(DEBUG_INIT, "%s: %s has set irq2 to irq: %d "
							"mapped on gpio:%d\n",
			LIS3DH_ACC_DEV_NAME, __func__, acc->irq2,
							acc->pdata->gpio_int2);
	}

	memset(acc->resume_state, 0, ARRAY_SIZE(acc->resume_state));

	acc->resume_state[RES_CTRL_REG1] = LIS3DH_ACC_ENABLE_ALL_AXES;
	acc->resume_state[RES_CTRL_REG2] = 0x00;
	acc->resume_state[RES_CTRL_REG3] = 0x00;
	acc->resume_state[RES_CTRL_REG4] = 0x00;
	acc->resume_state[RES_CTRL_REG5] = 0x00;
	acc->resume_state[RES_CTRL_REG6] = 0x00;

	acc->resume_state[RES_TEMP_CFG_REG] = 0x00;
	acc->resume_state[RES_FIFO_CTRL_REG] = 0x00;
	acc->resume_state[RES_INT_CFG1] = 0x00;
	acc->resume_state[RES_INT_THS1] = 0x00;
	acc->resume_state[RES_INT_DUR1] = 0x00;

	acc->resume_state[RES_TT_CFG] = 0x00;
	acc->resume_state[RES_TT_THS] = 0x00;
	acc->resume_state[RES_TT_LIM] = 0x00;
	acc->resume_state[RES_TT_TLAT] = 0x00;
	acc->resume_state[RES_TT_TW] = 0x00;

	err = lis3dh_acc_device_power_on(acc);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err_pdata_init;
	}

	atomic_set(&acc->enabled, 1);

	err = lis3dh_acc_update_g_range(acc, acc->pdata->g_range);
	if (err < 0) {
		dev_err(&client->dev, "update_g_range failed\n");
		goto  err_power_off;
	}

	err = lis3dh_acc_update_odr(acc, acc->pdata->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr failed\n");
		goto  err_power_off;
	}

	err = lis3dh_acc_input_init(acc);
	if (err < 0) {
		dev_err(&client->dev, "input init failed\n");
		goto err_power_off;
	}


	err = create_sysfs_interfaces(&acc->input_dev->dev);
	if (err < 0) {
		printk(KERN_ERR
		   "device LIS3DE_ACC_DEV_NAME sysfs register failed\n");
		goto err_input_cleanup;
	}


	lis3dh_acc_device_power_off(acc);

	/* As default, do not report information */
	atomic_set(&acc->enabled, 0);

	if(acc->pdata->gpio_int1 >= 0){
		INIT_WORK(&acc->irq1_work, lis3dh_acc_irq1_work_func);
		acc->irq1_work_queue =
			create_singlethread_workqueue("lis3dh_acc_wq1");
		if (!acc->irq1_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev,
					"cannot create work queue1: %d\n", err);
			goto err_remove_sysfs_int;
		}
		err = request_irq(acc->irq1, lis3dh_acc_isr1,
				IRQF_TRIGGER_RISING, "lis3dh_acc_irq1", acc);
		if (err < 0) {
			dev_err(&client->dev, "request irq1 failed: %d\n", err);
			goto err_destoyworkqueue1;
		}
		disable_irq_nosync(acc->irq1);
	}

	if(acc->pdata->gpio_int2 >= 0){
		INIT_WORK(&acc->irq2_work, lis3dh_acc_irq2_work_func);
		acc->irq2_work_queue =
			create_singlethread_workqueue("lis3dh_acc_wq2");
		if (!acc->irq2_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev,
					"cannot create work queue2: %d\n", err);
			goto err_free_irq1;
		}
		err = request_irq(acc->irq2, lis3dh_acc_isr2,
				IRQF_TRIGGER_RISING, "lis3dh_acc_irq2", acc);
		if (err < 0) {
			dev_err(&client->dev, "request irq2 failed: %d\n", err);
			goto err_destoyworkqueue2;
		}
		disable_irq_nosync(acc->irq2);
	}

	mutex_unlock(&acc->lock);

#ifdef CONFIG_HAS_EARLYSUSPEND
	acc->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	acc->early_suspend.suspend = lis3dh_early_suspend;
	acc->early_suspend.resume = lis3dh_late_resume;
	register_early_suspend(&acc->early_suspend);
#endif

	dev_info(&client->dev, "%s: probed\n", LIS3DH_ACC_DEV_NAME);

	return 0;

err_destoyworkqueue2:
	if(acc->pdata->gpio_int2 >= 0)
		destroy_workqueue(acc->irq2_work_queue);
err_free_irq1:
	free_irq(acc->irq1, acc);
err_destoyworkqueue1:
	if(acc->pdata->gpio_int1 >= 0)
		destroy_workqueue(acc->irq1_work_queue);
err_remove_sysfs_int:
	remove_sysfs_interfaces(&acc->input_dev->dev);
err_input_cleanup:
	lis3dh_acc_input_cleanup(acc);
err_power_off:
	lis3dh_acc_device_power_off(acc);
err_pdata_init:
	if (acc->pdata->exit)
		acc->pdata->exit();
exit_kfree_pdata:
	kfree(acc->pdata);
err_mutexunlock:
	mutex_unlock(&acc->lock);
//err_freedata:
	kfree(acc);
exit_check_functionality_failed:
	printk(KERN_ERR "%s: Driver Init failed\n", LIS3DH_ACC_DEV_NAME);
	return err;
}

static int lis3dh_acc_remove(struct i2c_client *client)
{

	struct lis3dh_acc_data *acc = i2c_get_clientdata(client);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&acc->early_suspend);
#endif

	if(acc->pdata->gpio_int1 >= 0){
		free_irq(acc->irq1, acc);
		//gpio_free(acc->pdata->gpio_int1);
		destroy_workqueue(acc->irq1_work_queue);
	}

	if(acc->pdata->gpio_int2 >= 0){
		free_irq(acc->irq2, acc);
		//gpio_free(acc->pdata->gpio_int2);
		destroy_workqueue(acc->irq2_work_queue);
	}

	lis3dh_acc_input_cleanup(acc);
	lis3dh_acc_device_power_off(acc);
	remove_sysfs_interfaces(&acc->input_dev->dev);
	i2c_set_clientdata(client, NULL);

	if (acc->pdata->exit)
		acc->pdata->exit();
	kfree(acc->pdata);
	kfree(acc);

	return 0;
}
#ifdef CONFIG_HAS_EARLYSUSPEND
static void lis3dh_early_suspend(struct early_suspend *h)
{
	struct lis3dh_acc_data *acc =
		container_of(h, struct lis3dh_acc_data, early_suspend);
	
	dprintk(DEBUG_SUSPEND, "lis3dh_acc early suspend\n");
	acc->on_before_suspend = atomic_read(&acc->enabled);
	lis3dh_acc_disable(acc);
	return;
}

static void lis3dh_late_resume(struct early_suspend *h)
{
	struct lis3dh_acc_data *acc =
		container_of(h, struct lis3dh_acc_data, early_suspend);

	dprintk(DEBUG_SUSPEND, "lis3dh_acc late resume\n");
	if (acc->on_before_suspend)
		lis3dh_acc_enable(acc);
	return;
}
#else
#ifdef CONFIG_PM
static int lis3dh_acc_resume(struct i2c_client *client)
{
	struct lis3dh_acc_data *acc = i2c_get_clientdata(client);
	
	dprintk(DEBUG_SUSPEND, "lis3dh_acc resume\n");
	if (acc->on_before_suspend)
		return lis3dh_acc_enable(acc);
	return 0;
}

static int lis3dh_acc_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct lis3dh_acc_data *acc = i2c_get_clientdata(client);
	
	dprintk(DEBUG_SUSPEND, "lis3dh_acc suspend\n");
	acc->on_before_suspend = atomic_read(&acc->enabled);
	return lis3dh_acc_disable(acc);
}
#else
#define lis3dh_acc_suspend	NULL
#define lis3dh_acc_resume	NULL
#endif /* CONFIG_PM */
#endif /* CONFIG_HAS_EARLYSUSPEND */

static const struct i2c_device_id lis3dh_acc_id[]
		= { { LIS3DH_ACC_DEV_NAME, 0 }, { }, };

MODULE_DEVICE_TABLE(i2c, lis3dh_acc_id);

static struct i2c_driver lis3dh_acc_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
			.owner = THIS_MODULE,
			.name = LIS3DH_ACC_DEV_NAME,
		  },
	.probe = lis3dh_acc_probe,
	.remove = lis3dh_acc_remove,
#ifdef CONFIG_HAS_EARLYSUSPEND
#else
	.suspend = lis3dh_acc_suspend,
	.resume = lis3dh_acc_resume,
#endif
	.id_table = lis3dh_acc_id,
	.detect = gsensor_detect,
	.address_list	= normal_i2c,
};

static int __init lis3dh_acc_init(void)
{
	dprintk(DEBUG_INIT, "%s accelerometer driver: init\n",
						LIS3DH_ACC_DEV_NAME);

	if (input_fetch_sysconfig_para(&(gsensor_info.input_type))) {
		printk("%s: err.\n", __func__);
		return -1;
	} else
		twi_id = gsensor_info.twi_id;

	dprintk(DEBUG_INIT, "%s i2c twi is %d \n", __func__, twi_id);

	return i2c_add_driver(&lis3dh_acc_driver);
}

static void __exit lis3dh_acc_exit(void)
{
#ifdef DEBUG
	dprintk(DEBUG_INIT, "%s accelerometer driver exit\n",
						LIS3DH_ACC_DEV_NAME);
#endif /* DEBUG */
	i2c_del_driver(&lis3dh_acc_driver);
	return;
}

module_init(lis3dh_acc_init);
module_exit(lis3dh_acc_exit);

MODULE_DESCRIPTION("lis3dh digital accelerometer sysfs driver");
MODULE_AUTHOR("Matteo Dameno, Carmine Iascone, STMicroelectronics");
MODULE_LICENSE("GPL");


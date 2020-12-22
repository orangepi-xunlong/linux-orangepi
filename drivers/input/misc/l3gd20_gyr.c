/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
*
* File Name		: l3gd20_gyr_sysfs.c
* Authors		: MH - C&I BU - Application Team
*			: Carmine Iascone (carmine.iascone@st.com)
*			: Matteo Dameno (matteo.dameno@st.com)
*			: Both authors are willing to be considered the contact
*			: and update points for the driver.
* Version		: V 1.1.5.2 sysfs
* Date			: 2011/Nov/11
* Description		: L3GD20 digital output gyroscope sensor API
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
* REVISON HISTORY
*
* VERSION	| DATE		| AUTHORS		| DESCRIPTION
* 1.0		| 2010/May/02	| Carmine Iascone	| First Release
* 1.1.3		| 2011/Jun/24	| Matteo Dameno		| Corrects ODR Bug
* 1.1.4		| 2011/Sep/02	| Matteo Dameno		| SMB Bus Mng,
*		|		|			| forces BDU setting
* 1.1.5		| 2011/Sep/24	| Matteo Dameno		| Introduces FIFO Feat.
* 1.1.5.2	| 2011/Nov/11	| Matteo Dameno		| enable gpio_int to be
*		|		|			| passed as parameter at
*		|		|			| module loading time;
*		|		|			| corrects polling
*		|		|			| bug at end of probing;
* 1.1.5.3	| 2011/Dec/20	| Matteo Dameno		| corrects error in
*		|		|			| I2C SADROOT; Modifies
*		|		|			| resume suspend func.
*******************************************************************************/
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/input-polldev.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/moduleparam.h>
#include <linux/platform_device.h>

#include "l3gd20.h"

//#include <mach/system.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/init-input.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif


/** Maximum polled-device-reported rot speed value value in dps*/
#define FS_MAX                32768

/* l3gd20 gyroscope registers */
#define WHO_AM_I              0x0F

#define CTRL_REG1             0x20    /* CTRL REG1 */
#define CTRL_REG2             0x21    /* CTRL REG2 */
#define CTRL_REG3             0x22    /* CTRL_REG3 */
#define CTRL_REG4             0x23    /* CTRL_REG4 */
#define CTRL_REG5             0x24    /* CTRL_REG5 */
#define	REFERENCE             0x25    /* REFERENCE REG */
#define	FIFO_CTRL_REG         0x2E    /* FIFO CONTROL REGISTER */
#define FIFO_SRC_REG          0x2F    /* FIFO SOURCE REGISTER */
#define	OUT_X_L               0x28    /* 1st AXIS OUT REG of 6 */

#define AXISDATA_REG	      OUT_X_L

/* CTRL_REG1 */
#define ALL_ZEROES            0x00
#define PM_OFF                0x00
#define PM_NORMAL             0x08
#define ENABLE_ALL_AXES       0x07
#define ENABLE_NO_AXES        0x00
#define BW00                  0x00
#define BW01                  0x10
#define BW10                  0x20
#define BW11                  0x30
#define ODR095                0x00  /* ODR =  95Hz */
#define ODR190                0x40  /* ODR = 190Hz */
#define ODR380                0x80  /* ODR = 380Hz */
#define ODR760                0xC0  /* ODR = 760Hz */

/* CTRL_REG3 bits */
#define	I2_DRDY               0x08
#define	I2_WTM                0x04
#define	I2_OVRUN              0x02
#define	I2_EMPTY              0x01
#define	I2_NONE               0x00
#define	I2_MASK               0x0F

/* CTRL_REG4 bits */
#define	FS_MASK               0x30
#define	BDU_ENABLE            0x80

/* CTRL_REG5 bits */
#define	FIFO_ENABLE           0x40
#define HPF_ENALBE            0x11

/* FIFO_CTRL_REG bits */
#define	FIFO_MODE_MASK        0xE0
#define	FIFO_MODE_BYPASS      0x00
#define	FIFO_MODE_FIFO        0x20
#define	FIFO_MODE_STREAM      0x40
#define	FIFO_MODE_STR2FIFO    0x60
#define	FIFO_MODE_BYPASS2STR  0x80
#define	FIFO_WATERMARK_MASK   0x1F

#define FIFO_STORED_DATA_MASK 0x1F


#define FUZZ                  0
#define FLAT                  0
#define I2C_AUTO_INCREMENT    0x80

/* RESUME STATE INDICES */
#define	RES_CTRL_REG1         0
#define	RES_CTRL_REG2         1
#define	RES_CTRL_REG3         2
#define	RES_CTRL_REG4         3
#define	RES_CTRL_REG5         4
#define	RES_FIFO_CTRL_REG     5
#define	RESUME_ENTRIES        6


#define DEBUG 0

/** Registers Contents */
#define WHOAMI_L3GD20         0x00D4	/* Expected content for WAI register*/

static int int1_gpio = DEFAULT_INT1_GPIO;
static int int2_gpio = DEFAULT_INT2_GPIO;
/* module_param(int1_gpio, int, S_IRUGO); */
module_param(int2_gpio, int, S_IRUGO);

struct reg_r {
	u8 address;
	u8 value;
};

static struct status_registers {
	struct reg_r who_am_i;
} status_registers = {
	.who_am_i.address=WHO_AM_I, .who_am_i.value=WHOAMI_L3GD20,
};

/* Addresses to scan */
static const unsigned short normal_i2c[2] = {0x6a,I2C_CLIENT_END};
static int i2c_num = 0;
static const unsigned short i2c_address[] = {0x6a, 0x6b};
static struct sensor_config_info config_info = {
	.input_type = GYR_TYPE,
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

/*
 * L3GD20 gyroscope data
 * brief structure containing gyroscope values for yaw, pitch and roll in
 * signed short
 */

struct l3gd20_triple {
	short	x,	/* x-axis angular rate data. */
		y,	/* y-axis angluar rate data. */
		z;	/* z-axis angular rate data. */
};

struct output_rate {
	int poll_rate_ms;
	u8 mask;
};

static const struct output_rate odr_table[] = {

	{	2,	ODR760|BW10},
	{	3,	ODR380|BW01},
	{	6,	ODR190|BW00},
	{	11,	ODR095|BW00},
};

static int use_smbus;

static struct l3gd20_gyr_platform_data default_l3gd20_gyr_pdata = {
	.fs_range = L3GD20_GYR_FS_2000DPS,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,

	.poll_interval = 100,
	.min_interval = L3GD20_MIN_POLL_PERIOD_MS, /* 2ms */

	.gpio_int1 = DEFAULT_INT1_GPIO,
	.gpio_int2 = DEFAULT_INT2_GPIO,		/* int for fifo */

	.watermark = 0,
	.fifomode = 0,

	.init =	NULL,
	.exit =	NULL,
	.power_on = NULL,
	.power_off = NULL,
};

struct l3gd20_data {
	struct i2c_client *client;
	struct l3gd20_gyr_platform_data *pdata;

	struct mutex lock;

	struct input_polled_dev *input_poll_dev;
	int hw_initialized;

	atomic_t enabled;
	int on_before_suspend;

	u8 reg_addr;
	u8 resume_state[RESUME_ENTRIES];

	int irq2;
	struct work_struct irq2_work;
	struct workqueue_struct *irq2_work_queue;

	bool polling_enabled;
	
	#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
	#endif
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void l3gd20_early_suspend(struct early_suspend *h);
static void l3gd20_late_resume(struct early_suspend *h);
#endif


/**
 * gyr_detect - Device detection callback for automatic device creation
 * return value:  
 *                    = 0; success;
 *                    < 0; err
 */
static int gyr_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret;
	int err = -1;
	u8 buf[1];
	u8 cmd;
	
	dprintk(DEBUG_INIT, "enter func %s. \n", __FUNCTION__);
	
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	if(config_info.twi_id == adapter->nr){
		for(i2c_num = 0; i2c_num < (sizeof(i2c_address)/sizeof(i2c_address[0]));i2c_num++){	    
			client->addr = i2c_address[i2c_num];

			dprintk(DEBUG_INIT, "check i2c addr: %x .\n", client->addr);
			buf[0] = status_registers.who_am_i.address;
			cmd = buf[0];

			//err = lsm303d_i2c_read(stat, buf, 1);
			ret = i2c_master_send(client, &cmd, sizeof(cmd));
			if (ret != sizeof(cmd))
				return ret;

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
				strlcpy(info->type, L3GD20_GYR_DEV_NAME, I2C_NAME_SIZE);
				return 0;
			}

		}
	
		pr_info("%s:l3gd20 Device not found, \
			 maybe the other e_compass equipment! \n",__func__);
		return -ENODEV;
	}else{
		return -ENODEV;
	}

}

static int l3gd20_i2c_read(struct l3gd20_data *gyr, u8 *buf, int len)
{
	int ret;
	u8 reg = buf[0];
	u8 cmd = reg;

/*
	if (len > sizeof(buf))
			dev_err(&gyr->client->dev,
				"read error insufficient buffer length: "
				"len:%d, buf size=%d\n",
				len, sizeof(buf));
*/

	if (use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_read_byte_data(gyr->client, cmd);
			buf[0] = ret & 0xff;
#if DEBUG
			dev_warn(&gyr->client->dev,
				"i2c_smbus_read_byte_data: ret=0x%02x, len:%d ,"
				"command=0x%02x, buf[0]=0x%02x\n",
				ret, len, cmd , buf[0]);
#endif
		} else if (len > 1) {
			/* cmd =  = I2C_AUTO_INCREMENT | reg; */
			ret = i2c_smbus_read_i2c_block_data(gyr->client,
								cmd, len, buf);
#if DEBUG
			dev_warn(&gyr->client->dev,
				"i2c_smbus_read_i2c_block_data: ret:%d len:%d, "
				"command=0x%02x, ",
				ret, len, cmd);
			unsigned int ii;
			for (ii = 0; ii < len; ii++)
				printk(KERN_DEBUG "buf[%d]=0x%02x,",
								ii, buf[ii]);

			printk("\n");
#endif
		} else
			ret = -1;

		if (ret < 0) {
			dev_err(&gyr->client->dev,
				"read transfer error: len:%d, command=0x%02x\n",
				len, cmd);
			return 0; /* failure */
		}
		return len; /* success */
	}

	/* cmd =  = I2C_AUTO_INCREMENT | reg; */
	ret = i2c_master_send(gyr->client, &cmd, sizeof(cmd));
	if (ret != sizeof(cmd))
		return ret;

	return i2c_master_recv(gyr->client, buf, len);
}

static int l3gd20_i2c_write(struct l3gd20_data *gyr, u8 *buf, int len)
{
	int ret;
	u8 reg, value;

	reg = buf[0];
	value = buf[1];

	if (use_smbus) {
		if (len == 1) {
			ret = i2c_smbus_write_byte_data(gyr->client,
								reg, value);
#if DEBUG
			dev_warn(&gyr->client->dev,
				"i2c_smbus_write_byte_data: ret=%d, len:%d, "
				"command=0x%02x, value=0x%02x\n",
				ret, len, reg , value);
#endif
			return ret;
		} else if (len > 1) {
			ret = i2c_smbus_write_i2c_block_data(gyr->client,
							reg, len, buf + 1);
#if DEBUG
			dev_warn(&gyr->client->dev,
				"i2c_smbus_write_i2c_block_data: ret=%d, "
				"len:%d, command=0x%02x, ",
				ret, len, reg);
			unsigned int ii;
			for (ii = 0; ii < (len + 1); ii++)
				printk(KERN_DEBUG "value[%d]=0x%02x,",
								ii, buf[ii]);

			printk("\n");
#endif
			return ret;
		}
	}

	ret = i2c_master_send(gyr->client, buf, len+1);
	return (ret == len+1) ? 0 : ret;
}


static int l3gd20_register_write(struct l3gd20_data *gyro, u8 *buf,
		u8 reg_address, u8 new_value)
{
	int err;

		/* Sets configuration register at reg_address
		 *  NOTE: this is a straight overwrite  */
		buf[0] = reg_address;
		buf[1] = new_value;
		err = l3gd20_i2c_write(gyro, buf, 1);
		if (err < 0)
			return err;

	return err;
}

static int l3gd20_register_read(struct l3gd20_data *gyro, u8 *buf,
		u8 reg_address)
{

	int err = -1;
	buf[0] = (reg_address);
	err = l3gd20_i2c_read(gyro, buf, 1);
	return err;
}

static int l3gd20_register_update(struct l3gd20_data *gyro, u8 *buf,
		u8 reg_address, u8 mask, u8 new_bit_values)
{
	int err = -1;
	u8 init_val;
	u8 updated_val;
	err = l3gd20_register_read(gyro, buf, reg_address);
	if (!(err < 0)) {
		init_val = buf[0];
		updated_val = ((mask & new_bit_values) | ((~mask) & init_val));
		err = l3gd20_register_write(gyro, buf, reg_address,
				updated_val);
	}
	return err;
}

static int l3gd20_update_watermark(struct l3gd20_data *gyro,
								u8 watermark)
{
	int res = 0;
	u8 buf[2];
	u8 new_value;

	mutex_lock(&gyro->lock);
	new_value = (watermark % 0x20);
	res = l3gd20_register_update(gyro, buf, FIFO_CTRL_REG,
			 FIFO_WATERMARK_MASK, new_value);
	if (res < 0) {
		pr_err("%s : failed to update watermark\n", __func__);
		return res;
	}
	pr_err("%s : new_value:0x%02x,watermark:0x%02x\n",
			__func__, new_value, watermark);

	gyro->resume_state[RES_FIFO_CTRL_REG] =
		((FIFO_WATERMARK_MASK & new_value) |
		(~FIFO_WATERMARK_MASK &
				gyro->resume_state[RES_FIFO_CTRL_REG]));
	gyro->pdata->watermark = new_value;
	mutex_unlock(&gyro->lock);
	return res;
}

static int l3gd20_update_fifomode(struct l3gd20_data *gyro,
								u8 fifomode)
{
	int res;
	u8 buf[2];
	u8 new_value;

	new_value = fifomode;
	res = l3gd20_register_update(gyro, buf, FIFO_CTRL_REG,
					FIFO_MODE_MASK, new_value);
	if (res < 0) {
		pr_err("%s : failed to update fifoMode\n", __func__);
		return res;
	}
	/*
	pr_debug("%s : new_value:0x%02x,prev fifomode:0x%02x\n",
				__func__, new_value, gyro->pdata->fifomode);
	 */
	gyro->resume_state[RES_FIFO_CTRL_REG] =
		((FIFO_MODE_MASK & new_value) |
		(~FIFO_MODE_MASK &
				gyro->resume_state[RES_FIFO_CTRL_REG]));
	gyro->pdata->fifomode = new_value;

	return res;
}

#if 0
static int l3gd20_fifo_reset(struct l3gd20_data *gyro)
{
	u8 oldmode;
	int res;

	oldmode = gyro->pdata->fifomode;
	res = l3gd20_update_fifomode(gyro, FIFO_MODE_BYPASS);
	if (res < 0)
		return res;
	res = l3gd20_update_fifomode(gyro, oldmode);
	if (res >= 0)
		pr_debug("%s : fifo reset to: 0x%02x\n", __func__, oldmode);

	return res;
}
#endif

static int l3gd20_fifo_hwenable(struct l3gd20_data *gyro,
								u8 enable)
{
	int res;
	u8 buf[2];
	u8 set = 0x00;
	if (enable)
		set = FIFO_ENABLE;
	res = l3gd20_register_update(gyro, buf, CTRL_REG5,
			FIFO_ENABLE, set);
	if (res < 0) {
		pr_err("%s : fifo_hw switch to:0x%02x failed\n", __func__, set);
		return res;
	}
	gyro->resume_state[RES_CTRL_REG5] =
		((FIFO_ENABLE & set) |
		(~FIFO_ENABLE & gyro->resume_state[RES_CTRL_REG5]));
	pr_debug("%s : fifo_hw_enable set to:0x%02x\n", __func__, set);
	return res;
}

static int l3gd20_manage_int2settings(struct l3gd20_data *gyro,
								u8 fifomode)
{
	int res;
	u8 buf[2];
	bool enable_fifo_hw;
	bool recognized_mode = false;
	u8 int2bits = I2_NONE;
/*
	if (gyro->polling_enabled) {
		fifomode = FIFO_MODE_BYPASS;
		pr_warn("%s : in polling mode, fifo mode forced"
							" to BYPASS mode\n",
			L3GD20_GYR_DEV_NAME);
	}
*/


	switch (fifomode) {
	case FIFO_MODE_FIFO:
		recognized_mode = true;

		if (gyro->polling_enabled) {
			int2bits = I2_NONE;
			enable_fifo_hw = false;
		} else {
		int2bits = (I2_WTM | I2_OVRUN);
			enable_fifo_hw = true;
		}
		res = l3gd20_register_update(gyro, buf, CTRL_REG3,
					I2_MASK, int2bits);
		if (res < 0) {
			pr_err("%s : failed to update CTRL_REG3:0x%02x\n",
				__func__, fifomode);
			goto err_mutex_unlock;
		}
		gyro->resume_state[RES_CTRL_REG3] =
			((I2_MASK & int2bits) |
			(~(I2_MASK) & gyro->resume_state[RES_CTRL_REG3]));
		/* enable_fifo_hw = true; */
		break;

	case FIFO_MODE_BYPASS:
		recognized_mode = true;

		if (gyro->polling_enabled)
			int2bits = I2_NONE;
		else
			int2bits = I2_DRDY;

		res = l3gd20_register_update(gyro, buf, CTRL_REG3,
					I2_MASK, int2bits);
		if (res < 0) {
			pr_err("%s : failed to update to CTRL_REG3:0x%02x\n",
				__func__, fifomode);
			goto err_mutex_unlock;
		}
		gyro->resume_state[RES_CTRL_REG3] =
			((I2_MASK & int2bits) |
			(~I2_MASK & gyro->resume_state[RES_CTRL_REG3]));
		enable_fifo_hw = false;
		break;

	default:
		recognized_mode = false;
		res = l3gd20_register_update(gyro, buf, CTRL_REG3,
					I2_MASK, I2_NONE);
		if (res < 0) {
			pr_err("%s : failed to update CTRL_REG3:0x%02x\n",
				__func__, fifomode);
			goto err_mutex_unlock;
		}
		enable_fifo_hw = false;
		gyro->resume_state[RES_CTRL_REG3] =
			((I2_MASK & 0x00) |
			(~I2_MASK & gyro->resume_state[RES_CTRL_REG3]));
		break;

	}
	if (recognized_mode) {
		res = l3gd20_update_fifomode(gyro, fifomode);
		if (res < 0) {
			pr_err("%s : failed to set fifoMode\n", __func__);
			goto err_mutex_unlock;
		}
	}
	res = l3gd20_fifo_hwenable(gyro, enable_fifo_hw);

err_mutex_unlock:

	return res;
}

static int l3gd20_update_fs_range(struct l3gd20_data *gyro,
							u8 new_fs)
{
	int res ;
	u8 buf[2];

	buf[0] = CTRL_REG4;

	res = l3gd20_register_update(gyro, buf, CTRL_REG4,
							FS_MASK, new_fs);

	if (res < 0) {
		pr_err("%s : failed to update fs:0x%02x\n",
							__func__, new_fs);
		return res;
	}
	gyro->resume_state[RES_CTRL_REG4] =
		((FS_MASK & new_fs) |
		(~FS_MASK & gyro->resume_state[RES_CTRL_REG4]));

	return res;
}


static int l3gd20_update_odr(struct l3gd20_data *gyro,
			unsigned int poll_interval_ms)
{
	int err = -1;
	int i;
	u8 config[2];

	for (i = ARRAY_SIZE(odr_table) - 1; i >= 0; i--) {
		if ((odr_table[i].poll_rate_ms <= poll_interval_ms) || (i == 0))
			break;
	}

	config[1] = odr_table[i].mask;
	config[1] |= (ENABLE_ALL_AXES + PM_NORMAL);

	/* If device is currently enabled, we need to write new
	 *  configuration out to it */
	if (atomic_read(&gyro->enabled)) {
		config[0] = CTRL_REG1;
		err = l3gd20_i2c_write(gyro, config, 1);
		if (err < 0)
			return err;
		gyro->resume_state[RES_CTRL_REG1] = config[1];
	}


	return err;
}

/* gyroscope data readout */
static int l3gd20_get_data(struct l3gd20_data *gyro,
			     struct l3gd20_triple *data)
{
	int err;
	unsigned char gyro_out[6];
	/* y,p,r hardware data */
	s16 hw_d[3] = { 0 };

	gyro_out[0] = (I2C_AUTO_INCREMENT | AXISDATA_REG);

	err = l3gd20_i2c_read(gyro, gyro_out, 6);

	if (err < 0)
		return err;

	hw_d[0] = (s16) (((gyro_out[1]) << 8) | gyro_out[0]);
	hw_d[1] = (s16) (((gyro_out[3]) << 8) | gyro_out[2]);
	hw_d[2] = (s16) (((gyro_out[5]) << 8) | gyro_out[4]);

	data->x = ((gyro->pdata->negate_x) ? (-hw_d[gyro->pdata->axis_map_x])
		   : (hw_d[gyro->pdata->axis_map_x]));
	data->y = ((gyro->pdata->negate_y) ? (-hw_d[gyro->pdata->axis_map_y])
		   : (hw_d[gyro->pdata->axis_map_y]));
	data->z = ((gyro->pdata->negate_z) ? (-hw_d[gyro->pdata->axis_map_z])
		   : (hw_d[gyro->pdata->axis_map_z]));

#if DEBUG
	/* printk(KERN_INFO "gyro_out: x = %d, y = %d, z = %d\n",
		data->x, data->y, data->z); */
#endif

	return err;
}

static void l3gd20_report_values(struct l3gd20_data *gyr,
						struct l3gd20_triple *data)
{
	struct input_dev *input = gyr->input_poll_dev->input;
	
	input_report_abs(input, ABS_X, data->x);
	input_report_abs(input, ABS_Y, data->y);
	input_report_abs(input, ABS_Z, data->z);
	dprintk(DEBUG_REPORT_DATA, "gyr->input_poll_dev->input x = %d, y = %d, z = %d. \n", \
		data->x, data->y, data->z);
	
	input_sync(input);
}

static int l3gd20_hw_init(struct l3gd20_data *gyro)
{
	int err;
	u8 buf[6];

	pr_info("%s hw init\n", L3GD20_GYR_DEV_NAME);

	buf[0] = (I2C_AUTO_INCREMENT | CTRL_REG1);
	buf[1] = gyro->resume_state[RES_CTRL_REG1];
	buf[2] = gyro->resume_state[RES_CTRL_REG2];
	buf[3] = gyro->resume_state[RES_CTRL_REG3];
	buf[4] = gyro->resume_state[RES_CTRL_REG4];
	buf[5] = gyro->resume_state[RES_CTRL_REG5];

	err = l3gd20_i2c_write(gyro, buf, 5);
	if (err < 0)
		return err;

	buf[0] = FIFO_CTRL_REG;
	buf[1] = gyro->resume_state[RES_FIFO_CTRL_REG];
	err = l3gd20_i2c_write(gyro, buf, 1);
	if (err < 0)
			return err;

	gyro->hw_initialized = 1;

	return err;
}

static void l3gd20_device_power_off(struct l3gd20_data *dev_data)
{
	int err;
	u8 buf[2];

	pr_info("%s power off\n", L3GD20_GYR_DEV_NAME);

	buf[0] = CTRL_REG1;
	buf[1] = PM_OFF;
	err = l3gd20_i2c_write(dev_data, buf, 1);
	if (err < 0)
		dev_err(&dev_data->client->dev, "soft power off failed\n");

	if (dev_data->pdata->power_off) {
		/* disable_irq_nosync(acc->irq1); */
		disable_irq_nosync(dev_data->irq2);
		dev_data->pdata->power_off();
		dev_data->hw_initialized = 0;
	}

	if (dev_data->hw_initialized) {
		/*if (dev_data->pdata->gpio_int1 >= 0)*/
		/*	disable_irq_nosync(dev_data->irq1);*/
		if (dev_data->pdata->gpio_int2 >= 0) {
			disable_irq_nosync(dev_data->irq2);
			pr_info("%s: power off: irq2 disabled\n",
						L3GD20_GYR_DEV_NAME);
		}
		dev_data->hw_initialized = 0;
	}
}

static int l3gd20_device_power_on(struct l3gd20_data *dev_data)
{
	int err;

	if (dev_data->pdata->power_on) {
		err = dev_data->pdata->power_on();
		if (err < 0)
			return err;
		if (dev_data->pdata->gpio_int2 >= 0)
			enable_irq(dev_data->irq2);
	}


	if (!dev_data->hw_initialized) {
		err = l3gd20_hw_init(dev_data);
		if (err < 0) {
			l3gd20_device_power_off(dev_data);
			return err;
		}
	}

	if (dev_data->hw_initialized) {
		/* if (dev_data->pdata->gpio_int1) {
			enable_irq(dev_data->irq1);
			pr_info("%s: power on: irq1 enabled\n",
					L3GD20_GYR_DEV_NAME);
		} */
		printk(KERN_DEBUG "dev_data->pdata->gpio_int2 = %d\n",
						dev_data->pdata->gpio_int2);
		if (dev_data->pdata->gpio_int2 >= 0) {
			enable_irq(dev_data->irq2);
			pr_info("%s: power on: irq2 enabled\n",
						L3GD20_GYR_DEV_NAME);

		}
	}

	return 0;
}

static int l3gd20_enable(struct l3gd20_data *dev_data)
{
	int err;

	dprintk(DEBUG_CONTROL_INFO, "enter func %s. \n", __FUNCTION__);
	if (!atomic_cmpxchg(&dev_data->enabled, 0, 1)) {

		err = l3gd20_device_power_on(dev_data);
		if (err < 0) {
			atomic_set(&dev_data->enabled, 0);
			return err;
		}
		dprintk(DEBUG_CONTROL_INFO, "dev_data->polling_enabled = %d. \n", dev_data->polling_enabled);
		if (dev_data->polling_enabled) {
			dev_data->input_poll_dev->poll_interval =
						dev_data->pdata->poll_interval;
			mutex_lock(&dev_data->input_poll_dev->input->mutex);
			if (dev_data->input_poll_dev->input->users) {
				err = cancel_delayed_work_sync(&dev_data->
							input_poll_dev->work);
				dprintk(DEBUG_CONTROL_INFO, "%s cancel_delayed_work_sync"
						" result: %d", __func__, err);
				if (dev_data->input_poll_dev->
							poll_interval > 0) {

					dprintk(DEBUG_CONTROL_INFO, "%s: queue work\n", __func__);
					err = schedule_delayed_work(
						&dev_data->input_poll_dev->work,
							      msecs_to_jiffies(dev_data->
							      pdata->poll_interval));

					dprintk(DEBUG_CONTROL_INFO, "%s schedule_delayed_work "
								"result: %d",
								__func__, err);
				}
			}
			mutex_unlock(&dev_data->input_poll_dev->input->mutex);
		}

	}

	return 0;
}

static int l3gd20_disable(struct l3gd20_data *dev_data)
{
	int err;
	printk(KERN_DEBUG "%s: dev_data->enabled = %d\n", __func__,
		atomic_read(&dev_data->enabled));

	if (atomic_cmpxchg(&dev_data->enabled, 1, 0)) {
		if (dev_data->polling_enabled) {
			dev_data->input_poll_dev->poll_interval = 0;
		}
		l3gd20_device_power_off(dev_data);
		err = cancel_delayed_work_sync(&dev_data->input_poll_dev->work);
		dprintk(DEBUG_CONTROL_INFO, "%s cancel_delayed_work_sync result: %d",
								__func__, err);
	}
	return 0;
}

static ssize_t attr_polling_rate_show(struct device *dev,
				     struct device_attribute *attr,
				     char *buf)
{
	int val;
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	mutex_lock(&gyro->lock);
	val = gyro->pdata->poll_interval;
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_polling_rate_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	unsigned long interval_ms;

	if (strict_strtoul(buf, 10, &interval_ms))
		return -EINVAL;
	if (!interval_ms)
		return -EINVAL;
	interval_ms = max((unsigned int)interval_ms, gyro->pdata->min_interval);
	mutex_lock(&gyro->lock);
	if (atomic_read(&gyro->enabled) && &gyro->polling_enabled)
		gyro->input_poll_dev->poll_interval = interval_ms;
	gyro->pdata->poll_interval = interval_ms;
	dprintk(DEBUG_CONTROL_INFO, "gyro->pdata->poll_interval = %d. \n", gyro->pdata->poll_interval);
	
	l3gd20_update_odr(gyro, interval_ms);
	mutex_unlock(&gyro->lock);
	return size;
}

static ssize_t attr_range_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	int range = 0;
	u8 val;
	mutex_lock(&gyro->lock);
	val = gyro->pdata->fs_range;

	switch (val) {
	case L3GD20_GYR_FS_250DPS:
		range = 250;
		break;
	case L3GD20_GYR_FS_500DPS:
		range = 500;
		break;
	case L3GD20_GYR_FS_2000DPS:
		range = 2000;
		break;
	}
	mutex_unlock(&gyro->lock);
	/* return sprintf(buf, "0x%02x\n", val); */
	return sprintf(buf, "%d\n", range);
}

static ssize_t attr_range_store(struct device *dev,
			      struct device_attribute *attr,
			      const char *buf, size_t size)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	unsigned long val;
	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;
	mutex_lock(&gyro->lock);
	gyro->pdata->fs_range = val;
	l3gd20_update_fs_range(gyro, val);
	mutex_unlock(&gyro->lock);
	return size;
}

static ssize_t attr_enable_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	int val = atomic_read(&gyro->enabled);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_enable_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	if (val)
		l3gd20_enable(gyro);
	else
		l3gd20_disable(gyro);

	return size;
}

static ssize_t attr_polling_mode_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	int val = 0;
	struct l3gd20_data *gyro = dev_get_drvdata(dev);

	mutex_lock(&gyro->lock);
	if (gyro->polling_enabled)
		val = 1;
	mutex_unlock(&gyro->lock);
	return sprintf(buf, "%d\n", val);
}

static ssize_t attr_polling_mode_store(struct device *dev,
			       struct device_attribute *attr,
			       const char *buf, size_t size)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 10, &val))
		return -EINVAL;

	mutex_lock(&gyro->lock);
	if (val) {
		gyro->polling_enabled = true;
		l3gd20_manage_int2settings(gyro, gyro->pdata->fifomode);
		dev_info(dev, "polling enabled\n");
		if (atomic_read(&gyro->enabled)) {
			gyro->input_poll_dev->poll_interval =
						gyro->pdata->poll_interval;
			schedule_delayed_work(&gyro->input_poll_dev->work,
					msecs_to_jiffies(gyro->
							pdata->poll_interval));
		}
	} else {
		if (gyro->polling_enabled) {
			if (atomic_read(&gyro->enabled))
				gyro->input_poll_dev->poll_interval = 0;
			cancel_delayed_work_sync(&gyro->input_poll_dev->work);
		}
		gyro->polling_enabled = false;
		l3gd20_manage_int2settings(gyro, gyro->pdata->fifomode);
		dev_info(dev, "polling disabled\n");
	}
	mutex_unlock(&gyro->lock);
	return size;
}

static ssize_t attr_watermark_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	unsigned long watermark;
	int res;

	if (strict_strtoul(buf, 16, &watermark))
		return -EINVAL;

	res = l3gd20_update_watermark(gyro, watermark);
	if (res < 0)
		return res;

	return size;
}

static ssize_t attr_watermark_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	int val = gyro->pdata->watermark;
	return sprintf(buf, "0x%02x\n", val);
}

static ssize_t attr_fifomode_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	unsigned long fifomode;
	int res;

	if (strict_strtoul(buf, 16, &fifomode))
		return -EINVAL;
	/* if (!fifomode)
		return -EINVAL; */

	dev_dbg(dev, "%s, got value:0x%02x\n", __func__, (u8)fifomode);

	mutex_lock(&gyro->lock);
	res = l3gd20_manage_int2settings(gyro, (u8) fifomode);
	mutex_unlock(&gyro->lock);

	if (res < 0)
		return res;
	return size;
}

static ssize_t attr_fifomode_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	u8 val = gyro->pdata->fifomode;
	return sprintf(buf, "0x%02x\n", val);
}

#ifdef DEBUG
static ssize_t attr_reg_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	int rc;
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	u8 x[2];
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;
	mutex_lock(&gyro->lock);
	x[0] = gyro->reg_addr;
	mutex_unlock(&gyro->lock);
	x[1] = val;
	rc = l3gd20_i2c_write(gyro, x, 1);
	return size;
}

static ssize_t attr_reg_get(struct device *dev, struct device_attribute *attr,
				char *buf)
{
	ssize_t ret;
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	int rc;
	u8 data;

	mutex_lock(&gyro->lock);
	data = gyro->reg_addr;
	mutex_unlock(&gyro->lock);
	rc = l3gd20_i2c_read(gyro, &data, 1);
	ret = sprintf(buf, "0x%02x\n", data);
	return ret;
}

static ssize_t attr_addr_set(struct device *dev, struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct l3gd20_data *gyro = dev_get_drvdata(dev);
	unsigned long val;

	if (strict_strtoul(buf, 16, &val))
		return -EINVAL;

	mutex_lock(&gyro->lock);

	gyro->reg_addr = val;

	mutex_unlock(&gyro->lock);

	return size;
}
#endif /* DEBUG */

static struct device_attribute attributes[] = {
	__ATTR(pollrate_ms, 0666, attr_polling_rate_show,
						attr_polling_rate_store),
	__ATTR(range, 0666, attr_range_show, attr_range_store),
	__ATTR(enable_device, 0666, attr_enable_show, attr_enable_store),
	__ATTR(enable_polling, 0666, attr_polling_mode_show,
						attr_polling_mode_store),
	__ATTR(fifo_samples, 0666, attr_watermark_show, attr_watermark_store),
	__ATTR(fifo_mode, 0666, attr_fifomode_show, attr_fifomode_store),
#ifdef DEBUG
	__ATTR(reg_value, 0600, attr_reg_get, attr_reg_set),
	__ATTR(reg_addr, 0200, NULL, attr_addr_set),
#endif
};

static int create_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		if (device_create_file(dev, attributes + i))
			goto error;
	return 0;

error:
	for (; i >= 0; i--)
		device_remove_file(dev, attributes + i);
	dev_err(dev, "%s:Unable to create interface\n", __func__);
	return -1;
}

static int remove_sysfs_interfaces(struct device *dev)
{
	int i;
	for (i = 0; i < ARRAY_SIZE(attributes); i++)
		device_remove_file(dev, attributes + i);
	return 0;
}

#if 0
static void report_triple(struct l3gd20_data *gyro)
{
	int err;
	struct l3gd20_triple data_out;

	err = l3gd20_get_data(gyro, &data_out);
	if (err < 0)
		dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
	else
		l3gd20_report_values(gyro, &data_out);
}
#endif

static void l3gd20_input_poll_func(struct input_polled_dev *dev)
{
	struct l3gd20_data *gyro = dev->private;

	struct l3gd20_triple data_out;

	int err;

	dev_dbg(&gyro->client->dev, "%s++\n", __func__);

	/* dev_data = container_of((struct delayed_work *)work,
				 struct l3gd20_data, input_work); */

	mutex_lock(&gyro->lock);
	err = l3gd20_get_data(gyro, &data_out);
	if (err < 0)
		dev_err(&gyro->client->dev, "get_gyroscope_data failed\n");
	else
		l3gd20_report_values(gyro, &data_out);

	mutex_unlock(&gyro->lock);

}

#if 0
static void l3gd20_irq2_fifo(struct l3gd20_data *gyro)
{
	int err;
	u8 buf[2];
	u8 int_source;
	u8 samples;
	u8 workingmode;
	u8 stored_samples;

	mutex_lock(&gyro->lock);

	workingmode = gyro->pdata->fifomode;


	dev_dbg(&gyro->client->dev, "%s : fifomode:0x%02x\n", __func__,
								workingmode);


	switch (workingmode) {
	case FIFO_MODE_BYPASS:
	{
		dev_dbg(&gyro->client->dev, "%s : fifomode:0x%02x\n", __func__,
							gyro->pdata->fifomode);
		report_triple(gyro);
		break;
	}
	case FIFO_MODE_FIFO:
		samples = (gyro->pdata->watermark)+1;
		dev_dbg(&gyro->client->dev,
			"%s : FIFO_SRC_REG init samples:%d\n",
							__func__, samples);
		err = l3gd20_register_read(gyro, buf, FIFO_SRC_REG);
		if (err < 0)
			dev_err(&gyro->client->dev,
					"error reading fifo source reg\n");

		int_source = buf[0];
		dev_dbg(&gyro->client->dev, "%s :FIFO_SRC_REG content:0x%02x\n",
							__func__, int_source);

		stored_samples = int_source & FIFO_STORED_DATA_MASK;
		dev_dbg(&gyro->client->dev, "%s : fifomode:0x%02x\n", __func__,
						gyro->pdata->fifomode);

		dev_dbg(&gyro->client->dev, "%s : samples:%d stored:%d\n",
				__func__, samples,stored_samples);

		for (; samples > 0; samples--) {
#if DEBUG
			input_report_abs(gyro->input_poll_dev->input,
								ABS_MISC, 1);
			input_sync(gyro->input_poll_dev->input);
#endif
			dev_dbg(&gyro->client->dev, "%s : current sample:%d\n",
							__func__, samples);

			report_triple(gyro);

#if DEBUG
			input_report_abs(gyro->input_poll_dev->input,
								ABS_MISC, 0);
			input_sync(gyro->input_poll_dev->input);
#endif
		}
		l3gd20_fifo_reset(gyro);
		break;
	}
#if DEBUG
	input_report_abs(gyro->input_poll_dev->input, ABS_MISC, 3);
	input_sync(gyro->input_poll_dev->input);
#endif

	mutex_unlock(&gyro->lock);
}

static irqreturn_t l3gd20_isr2(int irq, void *dev)
{
	struct l3gd20_data *gyro = dev;

	disable_irq_nosync(irq);
#if DEBUG
	input_report_abs(gyro->input_poll_dev->input, ABS_MISC, 2);
	input_sync(gyro->input_poll_dev->input);
#endif
	queue_work(gyro->irq2_work_queue, &gyro->irq2_work);
	pr_debug("%s %s: isr2 queued\n", L3GD20_GYR_DEV_NAME, __func__);

	return IRQ_HANDLED;
}

static void l3gd20_irq2_work_func(struct work_struct *work)
{

	struct l3gd20_data *gyro =
		container_of(work, struct l3gd20_data, irq2_work);
	/* TODO  add interrupt service procedure.
		 ie:l3gd20_XXX(gyro); */
	l3gd20_irq2_fifo(gyro);
	/*  */
	pr_debug("%s %s: IRQ2 served\n", L3GD20_GYR_DEV_NAME, __func__);
/* exit: */
	enable_irq(gyro->irq2);
}
#endif

static int l3gd20_input_open(struct input_dev *input)
{
	struct l3gd20_data *gyro = input_get_drvdata(input);
	pr_debug("%s\n", __func__);
	return l3gd20_enable(gyro);
}

static void l3gd20_input_close(struct input_dev *dev)
{
	struct l3gd20_data *gyro = input_get_drvdata(dev);
	pr_debug("%s\n", __func__);
	l3gd20_disable(gyro);
}

static int l3gd20_validate_pdata(struct l3gd20_data *gyro)
{
	/* checks for correctness of minimal polling period */
	gyro->pdata->min_interval =
		max((unsigned int) L3GD20_MIN_POLL_PERIOD_MS,
						gyro->pdata->min_interval);

	gyro->pdata->poll_interval = max(gyro->pdata->poll_interval,
			gyro->pdata->min_interval);

	if (gyro->pdata->axis_map_x > 2 ||
	    gyro->pdata->axis_map_y > 2 ||
	    gyro->pdata->axis_map_z > 2) {
		dev_err(&gyro->client->dev,
			"invalid axis_map value x:%u y:%u z%u\n",
			gyro->pdata->axis_map_x,
			gyro->pdata->axis_map_y,
			gyro->pdata->axis_map_z);
		return -EINVAL;
	}

	/* Only allow 0 and 1 for negation boolean flag */
	if (gyro->pdata->negate_x > 1 ||
	    gyro->pdata->negate_y > 1 ||
	    gyro->pdata->negate_z > 1) {
		dev_err(&gyro->client->dev,
			"invalid negate value x:%u y:%u z:%u\n",
			gyro->pdata->negate_x,
			gyro->pdata->negate_y,
			gyro->pdata->negate_z);
		return -EINVAL;
	}

	/* Enforce minimum polling interval */
	if (gyro->pdata->poll_interval < gyro->pdata->min_interval) {
		dev_err(&gyro->client->dev,
			"minimum poll interval violated\n");
		return -EINVAL;
	}
	return 0;
}

static int l3gd20_input_init(struct l3gd20_data *gyro)
{
	int err = -1;
	struct input_dev *input;

	pr_debug("%s++\n", __func__);

	gyro->input_poll_dev = input_allocate_polled_device();
	if (!gyro->input_poll_dev) {
		err = -ENOMEM;
		dev_err(&gyro->client->dev,
			"input device allocation failed\n");
		goto err0;
	}

	gyro->input_poll_dev->private = gyro;
	gyro->input_poll_dev->poll = l3gd20_input_poll_func;
	gyro->input_poll_dev->poll_interval = gyro->pdata->poll_interval;

	input = gyro->input_poll_dev->input;

	input->open = l3gd20_input_open;
	input->close = l3gd20_input_close;

	input->id.bustype = BUS_I2C;
	input->dev.parent = &gyro->client->dev;

	input_set_drvdata(gyro->input_poll_dev->input, gyro);

	set_bit(EV_ABS, input->evbit);

#if DEBUG
	set_bit(EV_KEY, input->keybit);
	set_bit(KEY_LEFT, input->keybit);
	input_set_abs_params(input, ABS_MISC, 0, 1, 0, 0);
#endif

	input_set_abs_params(input, ABS_X, -FS_MAX, FS_MAX, FUZZ, FLAT);
	input_set_abs_params(input, ABS_Y, -FS_MAX, FS_MAX, FUZZ, FLAT);
	input_set_abs_params(input, ABS_Z, -FS_MAX, FS_MAX, FUZZ, FLAT);

	input->name = L3GD20_GYR_DEV_NAME;

	err = input_register_polled_device(gyro->input_poll_dev);
	if (err) {
		dev_err(&gyro->client->dev,
			"unable to register input polled device %s\n",
			gyro->input_poll_dev->input->name);
		goto err1;
	}

	/* leaves the input poll dev stopped*/
	gyro->input_poll_dev->poll_interval = 0;
	err = cancel_delayed_work_sync(&gyro->input_poll_dev->work);
	pr_debug("%s cancel_delayed_work_sync result: %d",__func__, err);

	return 0;

err1:
	input_free_polled_device(gyro->input_poll_dev);
err0:
	return err;
}

static void l3gd20_input_cleanup(struct l3gd20_data *gyro)
{
	input_unregister_polled_device(gyro->input_poll_dev);
	input_free_polled_device(gyro->input_poll_dev);
}

static int l3gd20_probe(struct i2c_client *client,
					const struct i2c_device_id *devid)
{
	struct l3gd20_data *gyro;

	u32 smbus_func = I2C_FUNC_SMBUS_BYTE_DATA |
			I2C_FUNC_SMBUS_WORD_DATA | I2C_FUNC_SMBUS_I2C_BLOCK ;

	int err = -1;

	dprintk(DEBUG_INIT, "%s: probe start.\n", L3GD20_GYR_DEV_NAME);

	/* Support for both I2C and SMBUS adapter interfaces. */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		dev_warn(&client->dev, "client not i2c capable\n");
		if (i2c_check_functionality(client->adapter, smbus_func)) {
			use_smbus = 1;
			dev_warn(&client->dev, "client using SMBUS\n");
		} else {
			err = -ENODEV;
			dev_err(&client->dev, "client nor SMBUS capable\n");
			goto err0;
		}
	}

	gyro = kzalloc(sizeof(*gyro), GFP_KERNEL);
	if (gyro == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto err0;
	}

	dprintk(DEBUG_INIT, "l3gd20 probe i2c address is %d \n",i2c_address[i2c_num]);
	client->addr = i2c_address[i2c_num];

	mutex_init(&gyro->lock);
	mutex_lock(&gyro->lock);
	gyro->client = client;

	gyro->pdata = kmalloc(sizeof(*gyro->pdata), GFP_KERNEL);
	if (gyro->pdata == NULL) {
		dev_err(&client->dev,
			"failed to allocate memory for pdata: %d\n", err);
		goto err1;
	}

	if (client->dev.platform_data == NULL) {
		default_l3gd20_gyr_pdata.gpio_int1 = int1_gpio;
		default_l3gd20_gyr_pdata.gpio_int2 = int2_gpio;
		memcpy(gyro->pdata, &default_l3gd20_gyr_pdata,
							sizeof(*gyro->pdata));
	} else {
		memcpy(gyro->pdata, client->dev.platform_data,
						sizeof(*gyro->pdata));
	}

	err = l3gd20_validate_pdata(gyro);
	if (err < 0) {
		dev_err(&client->dev, "failed to validate platform data\n");
		goto err1_1;
	}

	i2c_set_clientdata(client, gyro);

	if (gyro->pdata->init) {
		err = gyro->pdata->init();
		if (err < 0) {
			dev_err(&client->dev, "init failed: %d\n", err);
			goto err1_1;
		}
	}


	memset(gyro->resume_state, 0, ARRAY_SIZE(gyro->resume_state));

	gyro->resume_state[RES_CTRL_REG1] = ALL_ZEROES | ENABLE_ALL_AXES;
	gyro->resume_state[RES_CTRL_REG2] = ALL_ZEROES;
	gyro->resume_state[RES_CTRL_REG3] = ALL_ZEROES;
	gyro->resume_state[RES_CTRL_REG4] = ALL_ZEROES | BDU_ENABLE;
	gyro->resume_state[RES_CTRL_REG5] = ALL_ZEROES;
	gyro->resume_state[RES_FIFO_CTRL_REG] = ALL_ZEROES;

	gyro->polling_enabled = true;
	dev_info(&client->dev, "polling enabled\n");

	err = l3gd20_device_power_on(gyro);
	if (err < 0) {
		dev_err(&client->dev, "power on failed: %d\n", err);
		goto err2;
	}

	atomic_set(&gyro->enabled, 1);

	err = l3gd20_update_fs_range(gyro, gyro->pdata->fs_range);
	if (err < 0) {
		dev_err(&client->dev, "update_fs_range failed\n");
		goto err2;
	}

	err = l3gd20_update_odr(gyro, gyro->pdata->poll_interval);
	if (err < 0) {
		dev_err(&client->dev, "update_odr failed\n");
		goto err2;
	}

	err = l3gd20_input_init(gyro);
	if (err < 0)
		goto err3;

	err = create_sysfs_interfaces(&client->dev);
	if (err < 0) {
		dev_err(&client->dev,
			"%s device register failed\n", L3GD20_GYR_DEV_NAME);
		goto err4;
	}

	l3gd20_device_power_off(gyro);

	/* As default, do not report information */
	atomic_set(&gyro->enabled, 0);

#if 0
	if (gyro->pdata->gpio_int2 >= 0) {
		gyro->irq2 = gpio_to_irq(gyro->pdata->gpio_int2);
		dev_info(&client->dev, "%s: %s has set irq2 to irq:"
						" %d mapped on gpio:%d\n",
			L3GD20_GYR_DEV_NAME, __func__, gyro->irq2,
							gyro->pdata->gpio_int2);

		INIT_WORK(&gyro->irq2_work, l3gd20_irq2_work_func);
		gyro->irq2_work_queue =
			create_singlethread_workqueue("r3gd20_gyr_irq2_wq");
		if (!gyro->irq2_work_queue) {
			err = -ENOMEM;
			dev_err(&client->dev, "cannot create "
						"work queue2: %d\n", err);
			goto err5;
		}

		err = request_irq(gyro->irq2, l3gd20_isr2,
				IRQF_TRIGGER_HIGH, "l3gd20_gyr_irq2", gyro);

		if (err < 0) {
			dev_err(&client->dev, "request irq2 failed: %d\n", err);
			goto err6;
		}
		disable_irq_nosync(gyro->irq2);
	}
#endif

	mutex_unlock(&gyro->lock);

	#ifdef CONFIG_HAS_EARLYSUSPEND
	gyro->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	gyro->early_suspend.suspend = l3gd20_early_suspend;
	gyro->early_suspend.resume = l3gd20_late_resume;
	register_early_suspend(&gyro->early_suspend);
	#endif


	dprintk(DEBUG_INIT, "%s probed: device created successfully\n",
							L3GD20_GYR_DEV_NAME);
	return 0;

/*err7:
	free_irq(gyro->irq2, gyro);
*/
#if 0
err6:
	destroy_workqueue(gyro->irq2_work_queue);
err5:
	l3gd20_device_power_off(gyro);
	remove_sysfs_interfaces(&client->dev);
#endif
err4:
	l3gd20_input_cleanup(gyro);
err3:
	l3gd20_device_power_off(gyro);
err2:
	if (gyro->pdata->exit)
		gyro->pdata->exit();
err1_1:
	mutex_unlock(&gyro->lock);
	kfree(gyro->pdata);
err1:
	kfree(gyro);
err0:
		pr_err("%s: Driver Initialization failed\n",
							L3GD20_GYR_DEV_NAME);
		return err;
}

static int l3gd20_remove(struct i2c_client *client)
{
	struct l3gd20_data *gyro = i2c_get_clientdata(client);

	dprintk(DEBUG_INIT, "%s: driver removing\n", L3GD20_GYR_DEV_NAME);

	/*
	if (gyro->pdata->gpio_int1 >= 0)
	{
		free_irq(gyro->irq1, gyro);
		gpio_free(gyro->pdata->gpio_int1);
		destroy_workqueue(gyro->irq1_work_queue);
	}
	*/
#if 0
	if (gyro->pdata->gpio_int2 >= 0) {
		free_irq(gyro->irq2, gyro);
		gpio_free(gyro->pdata->gpio_int2);
		destroy_workqueue(gyro->irq2_work_queue);
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&gyro->early_suspend);
#endif
	
	l3gd20_disable(gyro);
	l3gd20_input_cleanup(gyro);

	remove_sysfs_interfaces(&client->dev);

	i2c_set_clientdata(client, NULL);

	kfree(gyro->pdata);
	kfree(gyro);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void l3gd20_early_suspend(struct early_suspend *h)
{
	struct l3gd20_data *data =
		container_of(h, struct l3gd20_data, early_suspend);
	u8 buf[2];

	dprintk(DEBUG_SUSPEND, "l3gd20 early suspend\n");

	data->on_before_suspend = atomic_read(&data->enabled);
	
	if (data->on_before_suspend) {
		l3gd20_disable(data);

		mutex_lock(&data->lock);
		l3gd20_register_update(data, buf, CTRL_REG1,
						0x08, PM_OFF);
		mutex_unlock(&data->lock);
	}
	return;
}

static void l3gd20_late_resume(struct early_suspend *h)
{
	struct l3gd20_data *data =
		container_of(h, struct l3gd20_data, early_suspend);
	u8 buf[2];

	dprintk(DEBUG_SUSPEND, "l3gd20 late resume\n");

	if (data->on_before_suspend) {
		l3gd20_enable(data);

		mutex_lock(&data->lock);
		l3gd20_register_update(data, buf, CTRL_REG1,
				0x0F, (ENABLE_ALL_AXES | PM_NORMAL));
		mutex_unlock(&data->lock);
	}
	return;
}
#else
#ifdef CONFIG_PM
static int l3gd20_suspend(struct device *dev)
{
	int err = 0;
	
	struct i2c_client *client = to_i2c_client(dev);
	struct l3gd20_data *data = i2c_get_clientdata(client);
	u8 buf[2];

	dprintk(DEBUG_SUSPEND, "l3gd20 suspend\n");

	data->on_before_suspend = atomic_read(&data->enabled);
	
	if (data->on_before_suspend) {
		l3gd20_disable(data);

		mutex_lock(&data->lock);
		l3gd20_register_update(data, buf, CTRL_REG1,
						0x08, PM_OFF);
		mutex_unlock(&data->lock);
	}

	return err;
}

static int l3gd20_resume(struct device *dev)
{
	int err = 0;
	
	struct i2c_client *client = to_i2c_client(dev);
	struct l3gd20_data *data = i2c_get_clientdata(client);
	u8 buf[2];

	dprintk(DEBUG_SUSPEND, "l3gd20 resume\n");
	
	if (data->on_before_suspend) {
		l3gd20_enable(data);

		mutex_lock(&data->lock);
		l3gd20_register_update(data, buf, CTRL_REG1,
				0x0F, (ENABLE_ALL_AXES | PM_NORMAL));
		mutex_unlock(&data->lock);
	}

	return err;
}
#endif /*CONFIG_PM*/
#endif /*CONFIG_HAS_EARLYSUSPEND*/

static const struct i2c_device_id l3gd20_id[] = {
	{ L3GD20_GYR_DEV_NAME , 0 },
	{},
};

MODULE_DEVICE_TABLE(i2c, l3gd20_id);

#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
static struct dev_pm_ops l3gd20_pm = {
	.suspend = l3gd20_suspend,
	.resume = l3gd20_resume,
};
#endif

static struct i2c_driver l3gd20_driver = {
	.class = I2C_CLASS_HWMON,
	.driver = {
			.owner = THIS_MODULE,
			.name = L3GD20_GYR_DEV_NAME,
#if defined(CONFIG_PM) && !defined(CONFIG_HAS_EARLYSUSPEND)
			.pm = &l3gd20_pm,
#endif
	},
	.probe = l3gd20_probe,
	.remove = l3gd20_remove,
	.id_table = l3gd20_id,
	.address_list	= normal_i2c,

};

static int __init l3gd20_init(void)
{

	dprintk(DEBUG_INIT, "%s: gyroscope sysfs driver init\n", L3GD20_GYR_DEV_NAME);

	if(input_fetch_sysconfig_para(&(config_info.input_type))){
		printk("%s: err.\n", __func__);
		return -1;
	}

	if(config_info.sensor_used == 0){
		printk("*** used set to 0 !\n");
		printk("*** if use sensor,please put the sys_config.fex gy_used set to 1. \n");
		return 0;
	}

	l3gd20_driver.detect = gyr_detect;

	return i2c_add_driver(&l3gd20_driver);
}

static void __exit l3gd20_exit(void)
{
#if DEBUG
	pr_info("%s exit\n", L3GD20_GYR_DEV_NAME);
#endif
	i2c_del_driver(&l3gd20_driver);
	return;
}

module_init(l3gd20_init);
module_exit(l3gd20_exit);

MODULE_DESCRIPTION("l3gd20 digital gyroscope sysfs driver");
MODULE_AUTHOR("Matteo Dameno, Carmine Iascone, STMicroelectronics");
MODULE_LICENSE("GPL");


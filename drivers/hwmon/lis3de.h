
/******************** (C) COPYRIGHT 2012 STMicroelectronics ********************
*
* File Name	: lis3de.h
* Authors	: AMS - Motion Mems Division - Application Team
*		: Matteo Dameno (matteo.dameno@st.com)
*		: Denis Ciocca (denis.ciocca@st.com)
*		: Both authors are willing to be considered the contact
*		: and update points for the driver.
* Version	: V.1.0.1
* Date		: 2012/Oct/12
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
*******************************************************************************/
/*******************************************************************************
Version History.

 Revision 1.0.1: 2012/Oct/12
*******************************************************************************/

#ifndef	__LIS3DE_H__
#define	__LIS3DE_H__

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif


#define	LIS3DE_ACC_DEV_NAME		"lis3de_acc"
#define SENSOR_NAME                     LIS3DE_ACC_DEV_NAME


#define	LIS3DE_ACC_MIN_POLL_PERIOD_MS	1

#define LIS3DE_ACC_SENSITIVITY_2G		15.6f	/** mg/digit */
#define LIS3DE_ACC_SENSITIVITY_4G		31.2f	/** mg/digit */
#define LIS3DE_ACC_SENSITIVITY_8G		62.5f	/** mg/digit */
#define LIS3DE_ACC_SENSITIVITY_16G		187.5f	/** mg/digit */

#ifdef __KERNEL__

#define LIS3DE_SAD0L				(0x00)
#define LIS3DE_SAD0H				(0x01)
#define LIS3DE_ACC_I2C_SADROOT			(0x0C)

/* I2C address if acc SA0 pin to GND */
#define LIS3DE_ACC_I2C_SAD_L		((LIS3DE_ACC_I2C_SADROOT<<1)| \
						LIS3DE_SAD0L)

/* I2C address if acc SA0 pin to Vdd */
#define LIS3DE_ACC_I2C_SAD_H		((LIS3DE_ACC_I2C_SADROOT<<1)| \
						LIS3DE_SAD0H)

/* to set gpios numb connected to interrupt pins, 
* the unused ones have to be set to -EINVAL
*/
#define LIS3DE_ACC_DEFAULT_INT1_GPIO		(-EINVAL)
#define LIS3DE_ACC_DEFAULT_INT2_GPIO		(-EINVAL)

/* Accelerometer Sensor Full Scale */
#define	LIS3DE_ACC_FS_MASK		(0x30)
#define LIS3DE_ACC_G_2G			(0x00)
#define LIS3DE_ACC_G_4G			(0x10)
#define LIS3DE_ACC_G_8G			(0x20)
#define LIS3DE_ACC_G_16G		(0x30)

struct lis3de_acc_platform_data {
	unsigned int poll_interval;
	unsigned int min_interval;

	u8 fs_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);

	/* set gpio_int[1,2] either to the choosen gpio pin number or to -EINVAL
	 * if leaved unconnected
	 */
	int gpio_int1;
	int gpio_int2;
};

#endif	/* __KERNEL__ */

#endif	/* __LIS3DE_H__ */




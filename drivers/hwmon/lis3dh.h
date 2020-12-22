
/******************** (C) COPYRIGHT 2010 STMicroelectronics ********************
*
* File Name          : lis3dh_sysfs.h
* Authors            : MH - C&I BU - Application Team
*		     : Matteo Dameno (matteo.dameno@st.com)
*		     : Carmine Iascone (carmine.iascone@st.com)
* Version            : V 1.0.9
* Date               : 2011/May/23
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

Revision 1-0-0 05/11/2009
	First Release
Revision 1-0-1 26/01/2010
	Linux K&R Compliant Release
Revision 1-0-5 16/08/2010
	Interrupt Management
Revision 1-0-6 10/11/2010
	ioclt no more supported
	sysfs support
Revision 1-0-7 26/11/2010
	moved to input/misc
	manages use/non-use of interrupts
 Revision 1.0.8 2010/Apr/01
  corrects a bug in interrupt pin management in 1.0.7 acc side
 Revision 1.0.9: 2011/May/23
	SLEEP_MODE correction; update_odr correction; get/set_polling_rate corr.
*******************************************************************************/

#ifndef	__LIS3DH_H__
#define	__LIS3DH_H__

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
#include <linux/pm.h>
#endif


#define SAD0L			0x00
#define SAD0H			0x01
#define LIS3DH_ACC_I2C_SADROOT	0x0C
#define LIS3DH_ACC_I2C_SAD_L	((LIS3DH_ACC_I2C_SADROOT<<1)|SAD0L)
#define LIS3DH_ACC_I2C_SAD_H	((LIS3DH_ACC_I2C_SADROOT<<1)|SAD0H)
#define	LIS3DH_ACC_DEV_NAME	"lis3dh_acc"
#define SENSOR_NAME             LIS3DH_ACC_DEV_NAME 

/************************************************/
/* 	Accelerometer defines section	 	*/
/************************************************/

#define	LIS3DLH_ACC_MIN_POLL_PERIOD_MS	1

/* Accelerometer Sensor Full Scale */
#define	LIS3DH_ACC_FS_MASK		0x30
#define LIS3DH_ACC_G_2G 		0x00
#define LIS3DH_ACC_G_4G 		0x10
#define LIS3DH_ACC_G_8G 		0x20
#define LIS3DH_ACC_G_16G		0x30


#ifdef	__KERNEL__
struct lis3dh_acc_platform_data {
	unsigned int poll_interval;
	unsigned int min_interval;

	u8 g_range;

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

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif		
};
#endif	/* __KERNEL__ */

#endif	/* __LIS3DH_H__ */




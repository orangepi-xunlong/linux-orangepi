/**
 * @section LICENSE
 * Copyright (c) 2022 Bosch Sensortec GmbH All Rights Reserved
 *
 * This software program is licensed subject to the GNU General
 * Public License (GPL).Version 2,June 1991,
 * available at http://www.fsf.org/copyleft/gpl.html
 *
 * @file     bmi323_driver.h
 * @date	 10/10/2022
 * @version  1.6.0
 *
 * @brief    BMI3 Linux Driver
 */

#ifndef BMI3_DRIVER_H
#define BMI3_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************************************************************/
/* System header files */
/*********************************************************************/
#include <linux/types.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/firmware.h>

/*********************************************************************/
/* Own header files */
/*********************************************************************/
/* BMI3 variants. Only one should be enabled */
#include "bmi323.h"

/*********************************************************************/
/* Macro definitions */
/*********************************************************************/
/** Name of the device driver and accel input device*/
#define SENSOR_NAME "bmi323_inertial"
/** Name of the feature input device*/
#define SENSOR_NAME_FEAT "bmi323_feat"

/* Generic */
#define BMI3_DISABLE_CONTINUOUS_INT   (1)
#define BMI3_ENABLE_INT1				(1)
#define BMI3_MAX_RETRY_I2C_XFER		(10)
#define BMI3_I2C_WRITE_DELAY_TIME		(1)
#define REL_FEAT_STATUS				(1)
#define REL_HW_STATUS				(2)

/**
 *  struct bmi3_client_data - Client structure which holds sensor-specific
 *  information.
 */
struct bmi3_client_data {
	struct bmi3_dev device;
	struct device *dev;
	struct input_dev *acc_input;
	struct mutex lock;
	unsigned int IRQ;
	u16 fw_version;
	u8 sensor_init_state;
	int reg_sel;
	int reg_len;
	u8 context_selection;
};

/*********************************************************************/
/* Function prototype declarations */
/*********************************************************************/

/**
 * bmi323_probe - This is the probe function for bmi323 sensor.
 * Called from the I2C driver probe function to initialize the sensor.
 *
 * @client_data : Structure instance of client data.
 * @dev : Structure instance of device.
 *
 * Return : Result of execution status
 * * 0 - Success
 * * negative value -> Error
 */
int bmi3_probe(struct bmi3_client_data *client_data, struct device *dev);

/**
 * bmi323_suspend - This function puts the driver and device to suspend mode.
 *
 * @dev : Structure instance of device.
 *
 * Return : Result of execution status
 * * 0 - Success
 * * negative value -> Error
 */
int bmi3_suspend(struct device *dev);

/**
 * bmi323_resume - This function is used to bring back device from suspend
 *	mode.
 *
 * @dev  : Structure instance of device.
 *
 * Return : Result of execution status
 * * 0 - Success
 * * negative value -> Error
 */
int bmi3_resume(struct device *dev);

/**
 * bmi323_remove - This function removes the driver from the device.
 *
 * @dev : Structure instance of device.
 *
 * Return : Result of execution status
 * * 0 - Success
 * * negative value -> Error
 */
int bmi3_remove(struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* BMI3_DRIVER_H_  */

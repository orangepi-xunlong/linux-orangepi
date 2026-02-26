/**
 * Copyright (c) 2022 Bosch Sensortec GmbH. All rights reserved.
 *
 * BSD-3-Clause
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * @file       bmi3.c
 * @date       2022-08-31
 * @version    v2.0.0
 *
 */

/******************************************************************************/

/*!  @name          Header Files                                  */
/******************************************************************************/
#include "bmi3.h"
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include "stdio.h"
#endif

/***************************************************************************/

/*!              Static Variable
 ****************************************************************************/

/*! Array to store config array code */
static uint8_t bmi3_config_array_code[] = {
    0x48, 0x02, 0xad, 0x01, 0x01, 0x2e, 0xa0, 0xf2, 0x09, 0xbc, 0x20, 0x50, 0x0d, 0xb8, 0x03, 0x2e, 0xeb, 0x03, 0x01,
    0x1a, 0xf0, 0x7f, 0xeb, 0x7f, 0x17, 0x2f, 0x20, 0x30, 0x21, 0x2e, 0xd2, 0x01, 0x98, 0x2e, 0x1d, 0xc7, 0x98, 0x2e,
    0x2f, 0xc7, 0x98, 0x2e, 0x8e, 0xc0, 0x03, 0x2e, 0xf6, 0x01, 0x12, 0x24, 0xff, 0xe3, 0x4a, 0x08, 0x00, 0x30, 0x21,
    0x2e, 0xcd, 0x01, 0xf2, 0x6f, 0x21, 0x2e, 0xd1, 0x01, 0x25, 0x2e, 0xeb, 0x03, 0x23, 0x2e, 0xf6, 0x01, 0xeb, 0x6f,
    0xe0, 0x5f, 0xb8, 0x2e, 0x03, 0x2e, 0xac, 0x01, 0x01, 0x2e, 0x21, 0xf1, 0x92, 0xbc, 0x41, 0x0a, 0x01, 0x2e, 0xea,
    0x03, 0x01, 0x90, 0x23, 0x2e, 0x21, 0xf1, 0x14, 0x2f, 0x10, 0x24, 0xe0, 0xf4, 0x01, 0x30, 0x12, 0x24, 0x00, 0x10,
    0x13, 0x24, 0x00, 0x20, 0x04, 0x30, 0x05, 0x40, 0x95, 0x09, 0x80, 0xb3, 0x01, 0x2f, 0x5d, 0x0b, 0x05, 0x42, 0x01,
    0x89, 0x02, 0x80, 0x03, 0xa3, 0xf5, 0x2f, 0xc0, 0x2e, 0x23, 0x2e, 0xea, 0x03, 0xb8, 0x2e, 0xb8, 0x2e, 0x01, 0x2e,
    0xf6, 0x01, 0x86, 0xbc, 0x9f, 0xb8, 0x41, 0x90, 0x06, 0x2f, 0x11, 0x24, 0xff, 0xfd, 0x01, 0x08, 0x21, 0x2e, 0xf6,
    0x01, 0x80, 0x2e, 0x43, 0xeb, 0xb8, 0x2e, 0x10, 0x50, 0xfb, 0x7f, 0x98, 0x2e, 0x48, 0x02, 0xfb, 0x6f, 0xf0, 0x5f,
    0x80, 0x2e, 0x16, 0x03, 0x80, 0x2e, 0x48, 0x02, 0x01, 0x2e, 0xec, 0x03, 0x10, 0x50, 0x00, 0x90, 0x0e, 0x2f, 0x01,
    0x2e, 0xa0, 0xf2, 0x09, 0xbc, 0x0d, 0xb8, 0x31, 0x30, 0x08, 0x04, 0xfb, 0x7f, 0x21, 0x2e, 0xd5, 0x01, 0x98, 0x2e,
    0x18, 0xe5, 0x10, 0x30, 0x21, 0x2e, 0xec, 0x03, 0xfb, 0x6f, 0xf0, 0x5f, 0xb8, 0x2e, 0x05, 0x2e, 0xf6, 0x01, 0x01,
    0x2e, 0xae, 0x00, 0xab, 0xbc, 0x0a, 0xbc, 0x9f, 0xb8, 0x0f, 0xb8, 0x30, 0x50, 0x41, 0x08, 0x40, 0xb2, 0xfb, 0x7f,
    0x1d, 0x2f, 0xf4, 0x3e, 0x10, 0x24, 0x16, 0xf1, 0x14, 0x09, 0x29, 0x2e, 0xf6, 0x01, 0x11, 0x40, 0x02, 0x40, 0x09,
    0x2e, 0xad, 0x00, 0x13, 0x24, 0xac, 0x00, 0x98, 0x2e, 0xcd, 0x03, 0x12, 0x30, 0x11, 0x24, 0xf9, 0x01, 0xc2, 0x08,
    0x40, 0x40, 0x14, 0x24, 0xff, 0xdf, 0xbd, 0xbd, 0x04, 0x08, 0x03, 0x0a, 0x50, 0x42, 0x00, 0x2e, 0x40, 0x40, 0x02,
    0x0a, 0x02, 0x2c, 0x40, 0x42, 0x12, 0x30, 0x10, 0x24, 0x00, 0x10, 0x03, 0x2e, 0x80, 0xf0, 0x41, 0x08, 0x40, 0xb2,
    0x19, 0x2f, 0x03, 0x2e, 0xf6, 0x01, 0x9b, 0xbc, 0x9f, 0xb8, 0x41, 0x90, 0x19, 0x2f, 0x25, 0x2e, 0xea, 0x03, 0x11,
    0x24, 0xec, 0x00, 0x02, 0x30, 0xe1, 0x7f, 0xd2, 0x7f, 0x12, 0x24, 0xff, 0x0f, 0x41, 0x40, 0x98, 0x2e, 0x64, 0xce,
    0xd2, 0x6f, 0xe1, 0x6f, 0x81, 0x84, 0x50, 0x42, 0x83, 0xa2, 0xf2, 0x2f, 0x00, 0x2e, 0x06, 0x2d, 0x05, 0x2e, 0x05,
    0xf8, 0x01, 0x32, 0x8a, 0x0a, 0x25, 0x2e, 0x05, 0xf8, 0xfb, 0x6f, 0xd0, 0x5f, 0xb8, 0x2e, 0xb8, 0x2e, 0x01, 0x2e,
    0x12, 0xf1, 0x03, 0x2e, 0xbc, 0x00, 0x96, 0xbc, 0x20, 0x50, 0x96, 0xb8, 0x00, 0xa6, 0x54, 0xb0, 0xfb, 0x7f, 0x53,
    0x2f, 0x03, 0x2e, 0xee, 0x03, 0x40, 0x90, 0x26, 0x25, 0x01, 0x30, 0x07, 0x30, 0x13, 0x30, 0x19, 0x2f, 0x09, 0x2e,
    0xf0, 0x03, 0x44, 0x0e, 0x02, 0x2f, 0x02, 0x01, 0x29, 0x2e, 0xf0, 0x03, 0x09, 0x2e, 0xf0, 0x03, 0xc4, 0x0e, 0x0e,
    0x2f, 0x27, 0x2e, 0xee, 0x03, 0x44, 0x31, 0x05, 0x30, 0x41, 0x8b, 0x20, 0x19, 0x50, 0xa1, 0xfb, 0x2f, 0xf4, 0x3f,
    0x20, 0x05, 0x94, 0x04, 0x25, 0x2e, 0xf1, 0x03, 0x2d, 0x2e, 0xef, 0x03, 0x05, 0x2e, 0xee, 0x03, 0x81, 0x90, 0xe0,
    0x7f, 0x0f, 0x2f, 0x05, 0x2e, 0xed, 0x03, 0x50, 0x0f, 0x07, 0x2f, 0x05, 0x2e, 0xbc, 0x00, 0x13, 0x24, 0x00, 0xfc,
    0x93, 0x08, 0x25, 0x2e, 0xbc, 0x00, 0x04, 0x2d, 0x27, 0x2e, 0xf2, 0x03, 0x23, 0x2e, 0xee, 0x03, 0x05, 0x2e, 0xf2,
    0x03, 0x81, 0x90, 0x15, 0x2f, 0x05, 0x2e, 0xf1, 0x03, 0x42, 0x0e, 0x11, 0x2f, 0x23, 0x2e, 0xf2, 0x03, 0x50, 0x30,
    0x98, 0x2e, 0x04, 0xeb, 0x01, 0x2e, 0xef, 0x03, 0x05, 0x2e, 0xbc, 0x00, 0x11, 0x24, 0xff, 0x03, 0x13, 0x24, 0x00,
    0xfc, 0x93, 0x08, 0x01, 0x08, 0x02, 0x0a, 0x21, 0x2e, 0xbc, 0x00, 0xe0, 0x6f, 0x21, 0x2e, 0xed, 0x03, 0xfb, 0x6f,
    0xe0, 0x5f, 0xb8, 0x2e, 0x50, 0x50, 0x0a, 0x25, 0x3b, 0x80, 0xe3, 0x7f, 0x02, 0x42, 0xc1, 0x7f, 0xf1, 0x7f, 0xdb,
    0x7f, 0x22, 0x30, 0x03, 0x30, 0x10, 0x25, 0x98, 0x2e, 0x8e, 0xdf, 0xf1, 0x6f, 0x41, 0x16, 0x81, 0x08, 0xe3, 0x6f,
    0x4b, 0x08, 0x0a, 0x1a, 0xdb, 0x6f, 0x11, 0x30, 0x03, 0x30, 0xc0, 0x2e, 0x19, 0x22, 0xb0, 0x5f, 0x40, 0x50, 0x0a,
    0x25, 0x3c, 0x80, 0xfb, 0x7f, 0x01, 0x42, 0xd2, 0x7f, 0xe3, 0x7f, 0x32, 0x30, 0x03, 0x30, 0x10, 0x25, 0x98, 0x2e,
    0x8e, 0xdf, 0xfb, 0x6f, 0xc0, 0x5f, 0xb8, 0x2e, 0x02, 0x25, 0x23, 0x25, 0x30, 0x25, 0x50, 0x50, 0x00, 0x30, 0xbb,
    0x7f, 0x00, 0x43, 0xf4, 0x7f, 0xe2, 0x7f, 0xd3, 0x7f, 0xc1, 0x7f, 0x98, 0x2e, 0x78, 0x03, 0x01, 0xb2, 0x19, 0x2f,
    0xc1, 0x6f, 0xf2, 0x30, 0x8a, 0x08, 0x13, 0x24, 0xf0, 0x00, 0x4b, 0x08, 0x24, 0xbd, 0x94, 0xb8, 0x4a, 0x0a, 0x81,
    0x16, 0xd3, 0x6f, 0xe4, 0x6f, 0x93, 0x08, 0x4c, 0x08, 0x4a, 0x0a, 0x12, 0x24, 0x00, 0xff, 0x8a, 0x08, 0x13, 0x24,
    0xff, 0x00, 0x4b, 0x08, 0x98, 0xbc, 0x28, 0xb9, 0xf3, 0x6f, 0x4a, 0x0a, 0xc1, 0x42, 0x00, 0x2e, 0xbb, 0x6f, 0xb0,
    0x5f, 0xb8, 0x2e, 0x20, 0x50, 0xf3, 0x7f, 0xeb, 0x7f, 0xa3, 0x32, 0x01, 0x2e, 0xf3, 0x03, 0x00, 0xb2, 0x08, 0x2f,
    0x24, 0x25, 0x07, 0x2e, 0xf4, 0x03, 0xa1, 0x32, 0xf4, 0x6f, 0x98, 0x2e, 0xa0, 0x03, 0x08, 0x2c, 0x00, 0x30, 0x98,
    0x2e, 0x91, 0x03, 0xf1, 0x6f, 0x21, 0x2e, 0xf4, 0x03, 0x40, 0x42, 0x10, 0x30, 0xeb, 0x6f, 0xe0, 0x5f, 0x21, 0x2e,
    0xf3, 0x03, 0xb8, 0x2e, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/*! Array to store config array table */
static uint8_t bmi3_config_array_table[] = {
    0x05, 0x02, 0x0c, 0x00, 0xa9, 0x02, 0xa7, 0x02, 0x9f, 0x02, 0x6e, 0x02, 0x00, 0x00, 0x90, 0x02, 0xbf, 0x02, 0x15,
    0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x91, 0x02
};

/*! Array to store config version */
static uint8_t bmi3_config_version[] = {
    0xad, 0x00, 0x01, 0x00, 0x09, 0x08
};

/******************************************************************************/

/*!         Local Function Prototypes
 ******************************************************************************/

/*!
 * @brief This internal API sets accelerometer configurations like ODR, accel mode,
 * bandwidth, averaging samples and range.
 *
 * @param[in,out] config    : Structure instance of bmi3_accel_config.
 * @param[in,out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_accel_config(struct bmi3_accel_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API validates bandwidth and averaging samples of the
 * accelerometer set by the user.
 *
 * @param[in, out] bandwidth : Pointer to bandwidth value set by the user.
 * @param[in, out] acc_mode  : Pointer to accel mode set by the user.
 * @param[in, out] avg_num   : Pointer to average samples set by the user.
 * @param[in, out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @note dev->info contains two warnings: BMI3_I_MIN_VALUE and BMI3_I_MAX_VALUE
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 * @return > 0 -> Warning
 *
 */
static int8_t validate_bw_avg_acc_mode(uint8_t *bandwidth, uint8_t *acc_mode, uint8_t *avg_num, struct bmi3_dev *dev);

/*!
 * @brief This internal API validates ODR and range of the accelerometer set by
 * the user.
 *
 * @param[in, out] odr    : Pointer to ODR value set by the user.
 * @param[in, out] range  : Pointer to range value set by the user.
 * @param[in, out] dev    : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @note dev->info contains two warnings: BMI3_I_MIN_VALUE and BMI3_I_MAX_VALUE
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 * @return > 0 -> Warning
 */
static int8_t validate_acc_odr_range(uint8_t *odr, uint8_t *range, struct bmi3_dev *dev);

/*!
 * @brief This internal API is used to check the boundary conditions.
 *
 * @param[in,out] val    : Pointer to the value to be validated.
 * @param[in]     min    : Minimum value.
 * @param[in]     max    : Maximum value.
 * @param[in]     dev    : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t check_boundary_val(uint8_t *val, uint8_t min, uint8_t max, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets accelerometer configurations like ODR, accel mode,
 * bandwidth, averaging samples and range.
 *
 * @param[out] config    : Structure instance of bmi3_accel_config.
 * @param[in]  dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_accel_config(struct bmi3_accel_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the accelerometer data from the register.
 *
 * @param[out] data         : Structure instance of sensor_data.
 * @param[in]  reg_addr     : Register address where data is stored.
 * @param[in]  dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_accel_sensor_data(struct bmi3_sens_axes_data *data, uint8_t reg_addr, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets gyroscope configurations like ODR, gyro mode,
 * bandwidth, averaging samples and range.
 *
 * @param[in] config    : Structure instance of bmi3_gyro_config.
 * @param[in, out] dev  : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_gyro_config(struct bmi3_gyro_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API validates bandwidth and averaging samples of the
 * gyroscope set by the user.
 *
 * @param[in, out] bandwidth : Pointer to bandwidth value set by the user.
 * @param[in, out] gyr_mode  : Pointer to gyro mode set by the user.
 * @param[in, out] avg_num   : Pointer to average samples set by the user.
 * @param[in, out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @note dev->info contains two warnings: BMI3_I_MIN_VALUE and BMI3_I_MAX_VALUE
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t validate_bw_avg_gyr_mode(uint8_t *bandwidth,
				       uint8_t *gyr_mode,
				       const uint8_t *avg_num,
				       struct bmi3_dev *dev);

/*!
 * @brief This internal API validates ODR and range of the gyroscope set by
 * the user.
 *
 * @param[in, out] odr    : Pointer to ODR value set by the user.
 * @param[in, out] range  : Pointer to range value set by the user.
 * @param[in, out] dev    : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @note dev->info contains two warnings: BMI3_I_MIN_VALUE and BMI3_I_MAX_VALUE
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t validate_gyr_odr_range(uint8_t *odr, uint8_t *range, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the gyroscope data from the register.
 *
 * @param[out] data         : Structure instance of bmi3_sens_axes_data.
 * @param[in]  reg_addr     : Register address where data is stored.
 * @param[in]  dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_gyro_sensor_data(struct bmi3_sens_axes_data *data, uint8_t reg_addr, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the step counter data from the register.
 *
 * @param[out] step_count   : Variable to get step counter data.
 * @param[in]  reg_addr     : Register address where data is stored.
 * @param[in]  dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_step_counter_sensor_data(uint32_t *step_count, uint8_t reg_addr, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the orientation output data from the register.
 *
 * @param[out] orient_out   : Structure instance of bmi3_orientation_output.
 * @param[in]  reg_addr     : Register address where data is stored.
 * @param[in]  dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_orient_output_data(struct bmi3_orientation_output *orient_out, uint8_t reg_addr,
				     struct bmi3_dev *dev);

/*!
 * @brief This internal API gets gyroscope configurations like ODR, gyro mode,
 * bandwidth, averaging samples and range.
 *
 * @param[out] config    : Structure instance of bmi3_gyro_config.
 * @param[in]  dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_gyro_config(struct bmi3_gyro_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the accelerometer data.
 *
 * @param[out] data         : Structure instance of bmi3_sens_axes_data.
 * @param[in]  reg_data     : Data stored in the register.
 *
 * @return None
 *
 * @retval None
 */
static void get_acc_data(struct bmi3_sens_axes_data *data, const uint16_t *reg_data);

/*!
 * @brief This internal API gets the gyroscope data.
 *
 * @param[out] data         : Structure instance of bmi3_sens_axes_data.
 * @param[in]  reg_data     : Data stored in the register.
 *
 * @return None
 *
 * @retval None
 */
static void get_gyr_data(struct bmi3_sens_axes_data *data, const uint16_t *reg_data);

/*!
 * @brief This internal API is used to validate the device pointer for
 * null conditions.
 *
 * @param[in] dev : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t null_ptr_check(const struct bmi3_dev *dev);

/*!
 * @brief This internal API is used to set the feature.
 *
 * @param[in] dev    : Structure instance of bmi3_dev.
 * @param[in] enable : Structure instance of bmi3_feature_enable.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_feature_enable(const struct bmi3_feature_enable *enable, struct bmi3_dev *dev);

/*!
 * @brief This internal API is used to get the feature.
 *
 * @param[in] dev    : Structure instance of bmi3_dev.
 * @param[in] enable : Structure instance of bmi3_feature_enable.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_feature_enable(struct bmi3_feature_enable *enable, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets any-motion configurations like slope_thres,
 * duration, hysteresis, accel ref up and wait time.
 *
 * @param[out]      config     : Structure instance of bmi3_any_motion_config.
 * @param[in, out] dev         : Structure instance of bmi3_dev.
 *
 * @verbatim
 *----------------------------------------------------------------------------
 *  bmi3_any_motion_config  |
 *  Structure parameters    |                   Description
 *--------------------------|--------------------------------------------------
 *                          | Minimum slope of acceleration signal for
 *                          | motion detection.
 *   slope_thres            | Default value = 10, Range = 0 to 4095, Unit = 1.953mg.
 *                          |
 * -------------------------|---------------------------------------------------
 *   accel reference up     |  Mode of the acceleration reference update
 * -------------------------|---------------------------------------------------
 *                          | Hysteresis for the slope of the acceleration
 *   hysteresis             | signal. Default value = 2, Range = 0 to 1023,
 *                          | Unit = 1.953mg.
 * -------------------------|---------------------------------------------------
 *                          | Defines the number of  consecutive data points for
 *                          | which the threshold condition must be respected,
 *                          | for interrupt assertion.
 *   duration               | unit is 20ms, range
 *                          | Default value is 10 = 2000ms.
 * -------------------------|---------------------------------------------------
 *                          |
 *   wait time              | Wait time for clearing the event after slope
 *                          | is below threshold. Default value = 3, Range = 0 to 7, Unit =
 *                          | 20ms.
 * -----------------------------------------------------------------------------
 *
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_any_motion_config(struct bmi3_any_motion_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets any-motion configurations like slope threshold,
 * duration, hysteresis, accel ref up and wait time.
 *
 * @param[in]      config      : Structure instance of bmi3_any_motion_config.
 * @param[in, out] dev         : Structure instance of bmi3_dev.
 *
 * @verbatim
 *----------------------------------------------------------------------------
 *  bmi3_any_motion_config  |
 *  Structure parameters    |                   Description
 *--------------------------|--------------------------------------------------
 *                          | Minimum slope of acceleration signal for
 *                          | motion detection.
 *   slope_thres            | Default value = 10, Range = 0 to 4095, Unit = 1.953mg.
 *                          |
 * -------------------------|---------------------------------------------------
 *   accel reference up     |  Mode of the acceleration reference update
 * -------------------------|---------------------------------------------------
 *                          | Hysteresis for the slope of the acceleration
 *   hysteresis             | signal. Default value = 2, Range = 0 to 1023,
 *                          | Unit = 1.953mg.
 * -------------------------|---------------------------------------------------
 *                          | Defines the number of  consecutive data points for
 *                          | which the threshold condition must be respected,
 *                          | for interrupt assertion.
 *   duration               | unit is 20ms, range
 *                          | Default value is 10 = 2000ms.
 * -------------------------|---------------------------------------------------
 *                          |
 *   wait time              | Wait time for clearing the event after slope
 *                          | is below threshold. Default value = 3, Range = 0 to 7, Unit =
 *                          | 20ms.
 * -----------------------------------------------------------------------------
 *
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_any_motion_config(const struct bmi3_any_motion_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets no-motion configurations like slope threshold,
 * duration, hysteresis, accel ref up and wait time.
 *
 * @param[out]      config     : Structure instance of bmi3_no_motion_config.
 * @param[in, out] dev         : Structure instance of bmi3_dev.
 *
 * @verbatim
 *----------------------------------------------------------------------------
 *  bmi3_no_motion_config   |
 *  Structure parameters    |                   Description
 *--------------------------|--------------------------------------------------
 *                          | Minimum slope of acceleration signal for
 *                          | motion detection.
 *   slope_thres            | Default value = 10, Range = 0 to 4095, Unit = 1.953mg.
 *                          |
 * -------------------------|---------------------------------------------------
 *   accel reference up     |  Mode of the acceleration reference update
 * -------------------------|---------------------------------------------------
 *                          | Hysteresis for the slope of the acceleration
 *   hysteresis             | signal. Default value = 2, Range = 0 to 1023,
 *                          | Unit = 1.953mg.
 * -------------------------|---------------------------------------------------
 *                          | Defines the number of  consecutive data points for
 *                          | which the threshold condition must be respected,
 *                          | for interrupt assertion.
 *   duration               | unit is 20ms, range
 *                          | Default value is 10 = 2000ms.
 * -------------------------|---------------------------------------------------
 *                          |
 *   wait time              | Wait time for clearing the event after slope
 *                          | is below threshold. Default value = 3, Range = 0 to 7, Unit =
 *                          | 20ms.
 * -----------------------------------------------------------------------------
 *
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_no_motion_config(struct bmi3_no_motion_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets no-motion configurations like slope threshold,
 * duration, hysteresis, accel ref up and wait time.
 *
 * @param[in]      config      : Structure instance of bmi3_no_motion_config.
 * @param[in, out] dev         : Structure instance of bmi3_dev.
 *
 * @verbatim
 *----------------------------------------------------------------------------
 *  bmi3_no_motion_config   |
 *  Structure parameters    |                   Description
 *--------------------------|--------------------------------------------------
 *                          | Minimum slope of acceleration signal for
 *                          | motion detection.
 *   slope_thres            | Default value = 10, Range = 0 to 4095, Unit = 1.953mg.
 *                          |
 * -------------------------|---------------------------------------------------
 *   accel reference up     |  Mode of the acceleration reference update
 * -------------------------|---------------------------------------------------
 *                          | Hysteresis for the slope of the acceleration
 *   hysteresis             | signal. Default value = 2, Range = 0 to 1023,
 *                          | Unit = 1.953mg.
 * -------------------------|---------------------------------------------------
 *                          | Defines the number of  consecutive data points for
 *                          | which the threshold condition must be respected,
 *                          | for interrupt assertion.
 *   duration               | unit is 20ms, range
 *                          | Default value is 10 = 2000ms.
 * -------------------------|---------------------------------------------------
 *                          |
 *   wait time              | Wait time for clearing the event after slope
 *                          | is below threshold. Default value = 3, Range = 0 to 7, Unit =
 *                          | 20ms.
 * -----------------------------------------------------------------------------
 *
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_no_motion_config(const struct bmi3_no_motion_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets flat configurations like theta, blocking,
 * hold-time, hysteresis, and slope threshold.
 *
 * @param[out]     config           : Structure instance of bmi3_flat_config.
 * @param[in, out] dev              : Structure instance of bmi3_dev.
 *
 * @verbatim
 *----------------------------------------------------------------------------
 *      bmi3_flat_config    |
 *      Structure parameters|               Description
 *--------------------------|--------------------------------------------------
 *                          | Theta angle, used for detecting flat
 *                          | position.
 *      theta               | Theta = 64 * (tan(angle)^2);
 *                          | Default value is 8, equivalent to 20 degrees angle
 * -------------------------|---------------------------------------------------
 *                          | Hysteresis for Theta Flat detection.
 *      hysteresis          | Default value is 9 = 2.5 degrees, corresponding
 *                          | to the default Theta angle of 20 degrees.
 * -------------------------|---------------------------------------------------
 *                          | Sets the blocking mode. If blocking is set, no
 *                          | Flat interrupt will be triggered.
 *      blocking            | Default value is 2, the most restrictive blocking
 *                          | mode.
 * -------------------------|---------------------------------------------------
 *                          | Holds the duration in 50Hz samples for which the
 *                          | condition has to be respected.
 *      hold-time           | Default value is 32 = 640 m-sec.
 *                          | Range is 0 to 5.1 sec.
 * -------------------------|---------------------------------------------------
 *                          | Minimum slope between consecutive acceleration
 *      slope_thres         | samples to prevent the change of flat status during
 *                          | large movement. Default value is 0xcd = 400.365mg,
 *                          | Range = 498.015mg, Unit = 1.953mg.
 * -------------------------|---------------------------------------------------
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_flat_config(struct bmi3_flat_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets flat configurations like theta, blocking,
 * hold-time, hysteresis, and slope threshold.
 *
 * @param[in]     config           : Structure instance of bmi3_flat_config.
 * @param[in, out] dev             : Structure instance of bmi3_dev.
 *
 * @verbatim
 *----------------------------------------------------------------------------
 *      bmi3_flat_config    |
 *      Structure parameters|               Description
 *--------------------------|--------------------------------------------------
 *                          | Theta angle, used for detecting flat
 *                          | position.
 *      theta               | Theta = 64 * (tan(angle)^2);
 *                          | Default value is 8, equivalent to 20 degrees angle
 * -------------------------|---------------------------------------------------
 *                          | Hysteresis for Theta Flat detection.
 *      hysteresis          | Default value is 9 = 2.5 degrees, corresponding
 *                          | to the default Theta angle of 20 degrees.
 * -------------------------|---------------------------------------------------
 *                          | Sets the blocking mode. If blocking is set, no
 *                          | Flat interrupt will be triggered.
 *      blocking            | Default value is 2, the most restrictive blocking
 *                          | mode.
 * -------------------------|---------------------------------------------------
 *                          | Holds the duration in 50Hz samples for which the
 *                          | condition has to be respected.
 *      hold-time           | Default value is 32 = 640 m-sec.
 *                          | Range is 0 to 5.1 sec.
 * -------------------------|---------------------------------------------------
 *                          | Minimum slope between consecutive acceleration
 *      slope_threshold     | samples to prevent the change of flat status during
 *                          | large movement. Default value is 0xcd = 400.365mg,
 *                          | Range = 498.015mg, Unit = 1.953mg.
 * -------------------------|---------------------------------------------------
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_flat_config(const struct bmi3_flat_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets sig-motion configurations like block-size,
 * peak_2_peak_min, mcr_min, peak_2_peak_max and mcr_max parameters.
 *
 * @param[out]      config    : Structure instance of bmi3_sig_motion_config.
 * @param[in, out]  dev       : Structure instance of bmi3_dev.
 *
 *----------------------------------------------------------------------------
 *  bmi3_sig_motion_config  |
 *  Structure parameters    |                   Description
 * -------------------------|---------------------------------------------------
 *                          | Defines the duration after which the significant
 *  block_size              | motion interrupt is triggered. It is expressed in
 *                          | 50 Hz samples (20 ms). Default value is 0xFA=5sec.
 *--------------------------|---------------------------------------------------
 *                          | Minimum value of the peak to peak acceleration
 *  peak_2_peak_min         | magnitude. Default value 0x26 = 74.214mg, Range = 0 to 1023,
 *                          | Unit = 1.953mg
 *--------------------------|---------------------------------------------------
 *                          | Minimum number of mean crossing per second in accel
 *  mcr_min                 | magnitude. Default value = 17, Range = 0 to 63.
 *--------------------------|---------------------------------------------------
 *                          | Maximum value of the peak to peak acceleration
 *  peak_2_peak_max         | magnitude. Default value 0x253 = 1162.035mg, Range = 0 to 1023,
 *                          | Unit = 1.953mg
 *--------------------------|---------------------------------------------------
 *                          | Maximum number of mean crossing per second in accel
 *  mcr_max                 | magnitude. Default value = 17, Range = 0 to 63.
 *--------------------------|---------------------------------------------------
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_sig_motion_config(struct bmi3_sig_motion_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets sig-motion configurations like block-size,
 * peak_2_peak_min, mcr_min, peak_2_peak_max and mcr_max parameters.
 *
 * @param[in]      config    : Structure instance of bmi3_sig_motion_config.
 * @param[in, out]  dev      : Structure instance of bmi3_dev.
 *
 *----------------------------------------------------------------------------
 *  bmi3_sig_motion_config  |
 *  Structure parameters    |                   Description
 * -------------------------|---------------------------------------------------
 *                          | Defines the duration after which the significant
 *  block_size              | motion interrupt is triggered. It is expressed in
 *                          | 50 Hz samples (20 ms). Default value is 0xFA=5sec.
 *--------------------------|---------------------------------------------------
 *                          | Minimum value of the peak to peak acceleration
 *  peak_2_peak_min         | magnitude. Default value 0x26 = 74.214mg, Range = 0 to 1023,
 *                          | Unit = 1.953mg
 *--------------------------|---------------------------------------------------
 *                          | Minimum number of mean crossing per second in accel
 *  mcr_min                 | magnitude. Default value = 17, Range = 0 to 63.
 *--------------------------|---------------------------------------------------
 *                          | Maximum value of the peak to peak acceleration
 *  peak_2_peak_max         | magnitude. Default value 0x253 = 1162.035mg, Range = 0 to 1023,
 *                          | Unit = 1.953mg
 *--------------------------|---------------------------------------------------
 *                          | Maximum number of mean crossing per second in accel
 *  mcr_max                 | magnitude. Default value = 17, Range = 0 to 63.
 *--------------------------|---------------------------------------------------
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_sig_motion_config(const struct bmi3_sig_motion_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the latch mode from register address
 *
 * @param[out]      int_cfg   : Structure instance of bmi3_int_pin_config.
 * @param[in, out]  dev       : Structure instance of bmi3_dev.
 *
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_latch_mode(struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets the latch mode to register address
 *
 * @param[out]      int_cfg   : Structure instance of bmi3_int_pin_config.
 * @param[in, out]  dev       : Structure instance of bmi3_dev.
 *
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_latch_mode(const struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets tilt configurations like segment size, minimum tilt angle
 * and beta accel mean.
 *
 * @param[out]      config        : Structure instance of bmi3_tilt_config.
 * @param[in, out] dev            : Structure instance of bmi3_dev.
 *
 * @verbatim
 *----------------------------------------------------------------------------
 *  bmi3_tilt_config        |
 *  Structure parameters    |                   Description
 *------------------------- |--------------------------------------------------
 *                          | Duration for which the acceleration vector is
 *                          | averaged to be reference vector. Default value = 100, Range = 0
 *   segment size           | to 255, Unit = 20ms.
 * ------------------------ |---------------------------------------------------
 *                          | Minimum angle by which the device shall be
 *                          | tilted for event detection.
 *  minimum tilt angle      | Angle is computed as 256 * cos(angle).
 *                          | Default value = 210, Range = 0 to 255.
 * ------------------------ |---------------------------------------------------
 *                          | Exponential smoothing coefficient for
 *  beta accel mean         | computing low-pass mean of acceleration vector.
 *                          | Coefficient is computed as beta * 65536.
 *                          | Default value = 61545, Range = 0 to 65535.
 * ------------------------ |---------------------------------------------------
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_tilt_config(struct bmi3_tilt_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets tilt configurations like segment size, minimum tilt angle
 * and beta accel mean.
 *
 * @param[in]      config         : Structure instance of bmi3_tilt_config.
 * @param[in, out] dev            : Structure instance of bmi3_dev.
 *
 * @verbatim
 *----------------------------------------------------------------------------
 *  bmi3_tilt_config        |
 *  Structure parameters    |                   Description
 *------------------------- |--------------------------------------------------
 *                          | Duration for which the acceleration vector is
 *                          | averaged to be reference vector. Default value = 100, Range = 0
 *   segment size           | to 255, Unit = 20ms.
 * ------------------------ |---------------------------------------------------
 *                          | Minimum angle by which the device shall be
 *                          | tilted for event detection.
 *  minimum tilt angle      | Angle is computed as 256 * cos(angle).
 *                          | Default value = 210, Range = 0 to 255.
 * ------------------------ |---------------------------------------------------
 *                          | Exponential smoothing coefficient for
 *  beta accel mean         | computing low-pass mean of acceleration vector.
 *                          | Coefficient is computed as beta * 65536.
 *                          | Default value = 61545, Range = 0 to 65535.
 * ------------------------ |---------------------------------------------------
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_tilt_config(const struct bmi3_tilt_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets orientation configurations like upside/down
 * enable, symmetrical modes, blocking mode, theta, hysteresis, slope threshold and
 * hold time configuration.
 *
 * @param[out]      config        : Structure instance of bmi3_orientation_config.
 * @param[in, out] dev            : Structure instance of bmi3_dev.
 *
 * @verbatim
 *-----------------------------------------------------------------------------
 *   bmi3_orientation_config|
 *   Structure parameters   |               Description
 *--------------------------|--------------------------------------------------
 *  upside/down             |  Enables upside/down detection, if set to 1.
 *  detection               |
 * -------------------------|---------------------------------------------------
 *                          | Sets the mode:
 *  mode                    | Values 0 or 3 - symmetrical.
 *                          | Value 1       - high asymmetrical
 *                          | Value 2       - low asymmetrical
 * -------------------------|---------------------------------------------------
 *                          | Enable - no orientation interrupt will be set.
 *  blocking                | Default value: 3, the most restrictive blocking.
 * -------------------------|---------------------------------------------------
 *                          | Threshold angle used in blocking mode.
 *  theta                   | Theta = 64 * (tan(angle)^2)
 *                          | Default value: 39, range 0 to 63.
 * -------------------------|---------------------------------------------------
 *                          | Hysteresis of acceleration in orientation change
 *  hysteresis              | change detection.
 *                          | Default value = 32, range 0 to 255, unit = 1.93mg.
 * -------------------------|---------------------------------------------------
 *                          | Minimum duration the device shall be in new orientation
 *  hold_time               | for change detection. default value = 5, range 0 to 255,
 *                          | unit = 20ms.
 * -------------------------|---------------------------------------------------
 *                          | Minimum slope between consecutive acceleration samples to
 *  slope_thres             | prevent the change of orientation during large movement.
 *                          | Default value = 205, range = 0 t0 255, uint = 1.953mg.
 * -------------------------|---------------------------------------------------
 *
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_orientation_config(struct bmi3_orientation_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets orientation configurations like upside/down
 * enable, symmetrical modes, blocking mode, theta, hysteresis, slope threshold and
 * hold time configuration.
 *
 * @param[in]      config         : Structure instance of bmi3_orientation_config.
 * @param[in, out] dev            : Structure instance of bmi3_dev.
 *
 * @verbatim
 *-----------------------------------------------------------------------------
 *   bmi3_orientation_config|
 *   Structure parameters   |               Description
 *--------------------------|--------------------------------------------------
 *  upside/down             |  Enables upside/down detection, if set to 1.
 *  detection               |
 * -------------------------|---------------------------------------------------
 *                          | Sets the mode:
 *  mode                    | Values 0 or 3 - symmetrical.
 *                          | Value 1       - high asymmetrical
 *                          | Value 2       - low asymmetrical
 * -------------------------|---------------------------------------------------
 *                          | Enable - no orientation interrupt will be set.
 *  blocking                | Default value: 3, the most restrictive blocking.
 * -------------------------|---------------------------------------------------
 *                          | Threshold angle used in blocking mode.
 *  theta                   | Theta = 64 * (tan(angle)^2)
 *                          | Default value: 39, range 0 to 63.
 * -------------------------|---------------------------------------------------
 *                          | Hysteresis of acceleration in orientation change
 *  hysteresis              | change detection.
 *                          | Default value = 32, range 0 to 255, unit = 1.93mg.
 * -------------------------|---------------------------------------------------
 *                          | Minimum duration the device shall be in new orientation
 *  hold_time               | for change detection. default value = 5, range 0 to 255,
 *                          | unit = 20ms.
 * -------------------------|---------------------------------------------------
 *                          | Minimum slope between consecutive acceleration samples to
 *  slope_thres             | prevent the change of orientation during large movement.
 *                          | Default value = 205, range = 0 t0 255, uint = 1.953mg.
 * -------------------------|---------------------------------------------------
 *
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_orientation_config(const struct bmi3_orientation_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets step counter/detector/activity configurations.
 *
 * @param[out] config     : Structure instance of bmi3_step_counter_config.
 * @param[in] dev         : Structure instance of bmi3_dev.
 *
 *---------------------------------------------------------------------------
 *  bmi3_step_counter_config|
 *  Structure parameters    |                 Description
 *--------------------------|--------------------------------------------------
 *                          | The Step-counter will trigger output every time
 *                          | the number of steps are counted. Holds implicitly
 *  water-mark level        | a 20x factor, so the range is 0 to 20460,
 *                          | with resolution of 20 steps. If 0 output disabled.
 * -------------------------|---------------------------------------------------
 *  reset counter           | Flag to reset the counted steps.
 * -------------------------|---------------------------------------------------
 *  step_counter_params     | 1 - 19 step_counter parameters for different settings.
 * -------------------------|---------------------------------------------------
 *                          | Switch between the devices.
 *                          | value 0 = Smart phone
 *  sc_12_res               | value 1 = Wrist wearable
 *                          | value 2 = Hearable
 * -------------------------|---------------------------------------------------
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_step_config(struct bmi3_step_counter_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets step counter/detector/activity configurations.
 *
 * @param[in] config      : Structure instance of bmi3_step_counter_config.
 * @param[in] dev         : Structure instance of bmi3_dev.
 *
 *---------------------------------------------------------------------------
 *  bmi3_step_counter_config|
 *  Structure parameters    |                 Description
 *--------------------------|--------------------------------------------------
 *                          | The Step-counter will trigger output every time
 *                          | the number of steps are counted. Holds implicitly
 *  water-mark level        | a 20x factor, so the range is 0 to 20460,
 *                          | with resolution of 20 steps. If 0 output disabled.
 * -------------------------|---------------------------------------------------
 *  reset counter           | Flag to reset the counted steps.
 * -------------------------|---------------------------------------------------
 *  step_counter_params     | 1 - 19 step_counter parameters for different settings.
 * -------------------------|---------------------------------------------------
 *                          | Switch between the devices.
 *                          | value 0 = Smart phone
 *  sc_12_res               | value 1 = Wrist wearable
 *                          | value 2 = Hearable
 * -------------------------|---------------------------------------------------
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_step_config(const struct bmi3_step_counter_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets wake-up configurations like axis sel, wait for time out,
 * max peaks for tap, mode, tap peaks threshold, max gesture duration, max dur between peaks,
 * tap_shock_settling_dur, min_quite_dur_between_taps, quite_time_after_gesture.
 *
 * @param[out]      config    : Structure instance of bmi3_tap_detector_config.
 * @param[in, out]  dev       : Structure instance of bmi3_dev.
 *
 * @verbatim
 *-----------------------------|------------------------------------------------
 *  bmi3_tap_detector_config   |
 *  Structure parameters       |               Description
 *-----------------------------|--------------------------------------------------
 *                             | Accelerometer sensing axis selection for tap
 *                             | detection. Default value = 2 (z-axis)
 *  axis_sel                   | Value  Name    Description
 *                             | 00    axis_x   Use x-axis for tap detection
 *                             | 01    axis_y   Use y-axis for tap detection
 *                             | 10    axis_z   Use z-axis for tap detection
 * ----------------------------|---------------------------------------------------
 *                             | Perform gesture confirmation with wait time
 *  wait_for_time_out          | set by max_gesture_dur. Default value = 1 (Enabled)
 * ----------------------------|---------------------------------------------------
 *                             | Maximum number of zero crossing expected
 *  max_peaks_for_tap          | around a tap. Default value = 6, Range = 0 to 7
 *                             |
 * ----------------------------|---------------------------------------------------
 *                             | Mode for detection of tap gesture. Default value = Normal.
 *  mode                       | In stable position of device, to improve detection accuracy,
 *                             | sensitive mode can be used.
 *                             | Under noisy scenarios, the false detection can be suppressed with Robust mode.
 *                             | Value    Name        Description
 *                             |  00     sensitive  Sensitive detection mode
 *                             |  01     normal     Normal detection mode
 *                             |  10     robust     Robust detection mode
 * ----------------------------|---------------------------------------------------
 *                             | Minimum threshold for peak resulting from the tap.
 *  tap_peak_threshold         | Default value 2d = 87.885mg, Range = 1997.919mg, Resolution = 1.953mg
 * ----------------------------|---------------------------------------------------
 *                             | Maximum duration from first tap within the
 *  max_gest_duration          | second and/or third tap is expected to happen.
 *                             | Default value = 16(640ms), Range = 2.520s Resolution = 40ms
 * ----------------------------|---------------------------------------------------
 *                             | Maximum duration between positive and negative peaks to tap.
 *  max_dur_between_peaks      |  Default value = 4(20ms), Range = 75ms, Resolution = 5ms
 * ----------------------------|---------------------------------------------------
 *                             | Maximum duration for which tap impact is observed.
 *  tap_shock_settling_dur     | Default value = 6(30ms), Range = 75ms, Resolution = 5ms
 * ----------------------------|---------------------------------------------------
 *                             | Minimum duration between two tap impact.
 *  min_quite_dur_between_taps | Default value = 8(40ms), Range = 1.2s, Resolution = 5ms.
 * ----------------------------|---------------------------------------------------
 *                             | Minimum quite duration between two gestures.
 *  quite_time_after_gesture   | Default value = 6(240ms), Range = 600ms , Resolution = 40ms.
 * ----------------------------|---------------------------------------------------
 *
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_tap_config(struct bmi3_tap_detector_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets wake-up configurations like axis sel, wait for time out,
 * max peaks for tap, mode, tap peaks threshold, max gesture duration, max dur between peaks,
 * tap_shock_settling_dur, min_quite_dur_between_taps, quite_time_after_gesture.
 *
 * @param[in]      config    : Structure instance of bmi3_tap_detector_config.
 * @param[in, out]  dev      : Structure instance of bmi3_dev.
 *
 * @verbatim
 *-----------------------------|------------------------------------------------
 *  bmi3_tap_detector_config   |
 *  Structure parameters       |               Description
 *-----------------------------|--------------------------------------------------
 *                             | Accelerometer sensing axis selection for tap
 *                             | detection. Default value = 2 (z-axis)
 *  axis_sel                   | Value  Name    Description
 *                             | 00    axis_x   Use x-axis for tap detection
 *                             | 01    axis_y   Use y-axis for tap detection
 *                             | 10    axis_z   Use z-axis for tap detection
 * ----------------------------|---------------------------------------------------
 *                             | Perform gesture confirmation with wait time
 *  wait_for_time_out          | set by max_gesture_dur. Default value = 1 (Enabled)
 * ----------------------------|---------------------------------------------------
 *                             | Maximum number of zero crossing expected
 *  max_peaks_for_tap          | around a tap. Default value = 6, Range = 0 to 7
 *                             |
 * ----------------------------|---------------------------------------------------
 *                             | Mode for detection of tap gesture. Default value = Normal.
 *  mode                       | In stable position of device, to improve detection accuracy,
 *                             | sensitive mode can be used.
 *                             | Under noisy scenarios, the false detection can be suppressed with Robust mode.
 *                             | Value    Name        Description
 *                             |  00     sensitive  Sensitive detection mode
 *                             |  01     normal     Normal detection mode
 *                             |  10     robust     Robust detection mode
 * ----------------------------|---------------------------------------------------
 *                             | Minimum threshold for peak resulting from the tap.
 *  tap_peak_threshold         | Default value 2d = 87.885mg, Range = 1997.919mg, Resolution = 1.953mg
 * ----------------------------|---------------------------------------------------
 *                             | Maximum duration from first tap within the
 *  max_gest_duration          | second and/or third tap is expected to happen.
 *                             | Default value = 16(640ms), Range = 2.520s Resolution = 40ms
 * ----------------------------|---------------------------------------------------
 *                             | Maximum duration between positive and negative peaks to tap.
 *  max_dur_between_peaks      |  Default value = 4(20ms), Range = 75ms, Resolution = 5ms
 * ----------------------------|---------------------------------------------------
 *                             | Maximum duration for which tap impact is observed.
 *  tap_shock_settling_dur     | Default value = 6(30ms), Range = 75ms, Resolution = 5ms
 * ----------------------------|---------------------------------------------------
 *                             | Minimum duration between two tap impact.
 *  min_quite_dur_between_taps | Default value = 8(40ms), Range = 1.2s, Resolution = 5ms.
 * ----------------------------|---------------------------------------------------
 *                             | Minimum quite duration between two gestures.
 *  quite_time_after_gesture   | Default value = 6(240ms), Range = 600ms , Resolution = 40ms.
 * ----------------------------|---------------------------------------------------
 *
 * @endverbatim
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_tap_config(const struct bmi3_tap_detector_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API is used to parse accelerometer data from the FIFO
 * data.
 *
 * @param[out] acc              : Structure instance of bmi3_fifo_sens_axes_data
 *                                where the parsed data bytes are stored.
 * @param[in]  data_start_index : Index value of the accelerometer data bytes
 *                                which is to be parsed from the FIFO data.
 * @param[in]  fifo             : Structure instance of bmi3_fifo_frame.
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t unpack_accel_data(struct bmi3_fifo_sens_axes_data *acc,
				uint16_t data_start_index,
				const struct bmi3_fifo_frame *fifo);

/*!
 * @brief This internal API is used to parse temperature data from the FIFO
 * data.
 *
 * @param[out] temp              : Structure instance of bmi3_fifo_temperature_data
 *                                 where the parsed data bytes are stored.
 * @param[in]  data_start_index  : Index value of the temperature data bytes
 *                                 which is to be parsed from the FIFO data.
 * @param[in]  fifo              : Structure instance of bmi3_fifo_frame.
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t unpack_temperature_data(struct bmi3_fifo_temperature_data *temp,
				      uint16_t data_start_index,
				      const struct bmi3_fifo_frame *fifo);

/*!
 * @brief This internal API is used to parse gyroscope data from the FIFO
 * data.
 *
 * @param[out] gyro              : Structure instance of bmi3_fifo_sens_axes_data
 *                                 where the parsed data bytes are stored.
 * @param[in]  data_start_index  : Index value of the gyro data bytes
 *                                 which is to be parsed from the FIFO data.
 * @param[in]  fifo              : Structure instance of bmi3_fifo_frame.
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t unpack_gyro_data(struct bmi3_fifo_sens_axes_data *gyro,
			       uint16_t data_start_index,
			       const struct bmi3_fifo_frame *fifo);

/*!
 * @brief This internal API is used to parse the accel, gyro, temperature and sensortime data from the
 * FIFO data in header-less mode. It updates the current data byte to be parsed.
 *
 * @param[in,out] fifo_data       : Void pointer where accel, gyro and temperature parsed data
 *                                  bytes are stored.
 * @param[in,out] idx             : Index value of number of bytes parsed.
 * @param[in,out] updated_frm_idx : Index value of accel, gyro, temperature data (x,y,z axes)
 *                                  frame to be parsed.
 * @param[in]     frame           : Data is enabled by user.
 *
 * @param[in]     fifo            : Structure instance of bmi3_fifo_frame.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t unpack_fifo_data_frame(void *fifo_data,
				     uint16_t *idx,
				     uint16_t *updated_frm_idx,
				     uint16_t frame,
				     const struct bmi3_fifo_frame *fifo);

/*!
 * @brief This internal API is used to update data index based on the frame length
 *
 * @param[in,out] idx             : Index value of number of bytes parsed.
 * @param[in,out] updated_frm_idx : Index value of accel, gyro, temperature data (x,y,z axes)
 *                                  frame to be parsed.
 * @param[in]     len             : Length of the frame
 *
 * @param[in]     rslt            : Result value after unpacking frames
 *
 * @return Result of API execution status
 *
 * @return  None
 */
static void update_data_index(uint16_t *idx, uint16_t *updated_frm_idx, uint8_t len, int8_t rslt);

/*!
 * @brief This internal API sets the precondition settings such as alternate accelerometer and
 * gyroscope enable bits, accelerometer mode and output data rate.
 *
 *
 * @param[in] st_select : Variable denoting the self-test mode.
 * @param[in] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t st_precondition(uint8_t st_select, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets and sets the self-test mode given
 *  by the user in the self-test dma register.
 *
 * @param[in] st_selection : Variable denoting the self-test mode.
 * @param[out] dev         : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_set_st_dma(uint8_t st_selection, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets the self-test preconditions, individual gyroscope
 * filter coefficients and triggers the self-test in the command register.
 *
 * @param[in] st_selection : Variable denoting the self-test mode.
 * @param[in] dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t self_test_conditions(uint8_t st_selection, struct bmi3_dev *dev);

/*!
 * @brief This internal API is used to disable alternate config accel and gyro mode for self-test precondition.
 *
 * @param[in] dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t disable_alt_conf_acc_gyr_mode(struct bmi3_dev *dev);

/*!
 * @brief This internal API gets status of self-test and the result of self-test along with
 * the feature engine error status.
 *
 * @param[in] st_selection     : Variable denoting the self-test mode.
 * @param[in] st_result_status : Structure instance of bmi3_st_result.
 * @param[out] dev             : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_st_status_rslt(uint8_t st_selection, struct bmi3_st_result *st_result_status, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets status of self-calibration and the result of self-calibration along with the feature
 * engine error status.
 *
 * @param[in] sc_rslt : Structure instance of bmi3_self_calib_rslt.
 * @param[out] dev    : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_sc_gyro_rslt(struct bmi3_self_calib_rslt *sc_rslt, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets and sets the self-calibration mode given
 *  by the user in the self-calibration dma register.
 *
 * @param[in] sc_selection : Variable denoting the self-calibration mode.
 * @param[in] apply_corr   : Variable choosing to apply correction.
 * @param[out] dev         : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_set_sc_dma(uint8_t sc_selection, uint8_t apply_corr, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the i3c sync accelerometer data from the register.
 *
 * @param[out] data         : Structure instance of bmi3_i3c_sync_data.
 * @param[out]  dev         : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */

static int8_t get_i3c_sync_accel_sensor_data(struct bmi3_i3c_sync_data *data, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the i3c sync gyroscope data from the register.
 *
 * @param[out] data         : Structure instance of bmi3_i3c_sync_data.
 * @param[out]  dev         : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */

static int8_t get_i3c_sync_gyro_sensor_data(struct bmi3_i3c_sync_data *data, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets the i3c sync temperature data from the register and can be
 * converted into degree celsius using the below formula.
 * Formula: temperature_value = (float)(((float)((int16_t)temperature_data)) / 512.0) + 23.0
 * @param[out] data         : Structure instance of bmi3_i3c_sync_data.
 * @param[in]  dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_i3c_sync_temp_data(struct bmi3_i3c_sync_data *data, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets alternate accelerometer configurations like ODR,
 * accel mode and average number of samples.
 *
 * @param[out] config    : Structure instance of bmi3_alt_accel_config.
 * @param[in]  dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_alternate_accel_config(const struct bmi3_alt_accel_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets alternate accelerometer configurations like ODR,
 * accel mode and average number of samples.
 *
 * @param[out] config    : Structure instance of bmi3_alt_accel_config.
 * @param[in]  dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_alternate_accel_config(struct bmi3_alt_accel_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets alternate gyro configurations like ODR,
 * gyro mode and average number of samples.
 *
 * @param[out] config    : Structure instance of bmi3_alt_gyro_config.
 * @param[in]  dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_alternate_gyro_config(const struct bmi3_alt_gyro_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets alternate gyro configurations like ODR,
 * gyro mode and average number of samples.
 *
 * @param[out] config    : Structure instance of bmi3_alt_gyro_config.
 * @param[out]  dev      : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_alternate_gyro_config(struct bmi3_alt_gyro_config *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API gets alternate auto configurations for feature interrupts
 *
 * @param[out] config    : Structure instance of bmi3_auto_config_change.
 * @param[out]  dev      : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t get_alternate_auto_config(struct bmi3_auto_config_change *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets alternate auto configurations for feature interrupts.
 *
 * @param[out] config    : Structure instance of bmi3_auto_config_change.
 * @param[in]  dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_alternate_auto_config(const struct bmi3_auto_config_change *config, struct bmi3_dev *dev);

/*!
 * @brief This internal API is used to monitor the accel power mode during the axis remap.
 *
 * @param[in] config       : Structure instance of bmi3_sens_config.
 * @param[in] dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t axes_remap_acc_power_mode_status(struct bmi3_sens_config config, struct bmi3_dev *dev);

/*!
 * @brief This internal API is used to verify the right position of the sensor before doing accel FOC
 *
 * @param[in] sens_list     : Sensor type
 * @param[in] accel_g_axis  : Accel Foc axis and sign input
 * @param[in] dev           : Structure instance of bmi3_dev
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t verify_foc_position(uint8_t sens_list,
				  const struct bmi3_accel_foc_g_value *accel_g_axis,
				  struct bmi3_dev *dev);

/*!
 * @brief This internal API reads and provides average for 128 samples of sensor data for accel FOC.
 *
 * @param[in] sens_list     : Sensor type.
 * @param[in] temp_foc_data : To store data samples
 * @param[in] dev           : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t get_average_of_sensor_data(uint8_t sens_list,
					 struct bmi3_foc_temp_value *temp_foc_data,
					 struct bmi3_dev *dev);

/*!
 * @brief This internal API validates accel FOC position as per the range
 *
 * @param[in] sens_list     : Sensor type
 * @param[in] accel_g_axis  : Accel axis to FOC.
 * @param[in] avg_foc_data  : Average value of sensor sample datas
 * @param[in] dev           : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t validate_foc_position(uint8_t sens_list,
				    const struct bmi3_accel_foc_g_value *accel_g_axis,
				    struct bmi3_sens_axes_data avg_foc_data,
				    struct bmi3_dev *dev);

/*!
 * @brief This internal API validates accel FOC axis given as input
 *
 * @param[in] avg_foc_data : Average value of sensor sample datas
 * @param[in] dev          : Structure instance of bmi2_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t validate_foc_accel_axis(int16_t avg_foc_data, struct bmi3_dev *dev);

/*!
 * @brief This internal API sets configurations for performing accelerometer FOC.
 *
 * @param[in] dev       : Structure instance of bmi3_dev
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t set_accel_foc_config(struct bmi3_dev *dev);

/*!
 * @brief This internal API performs Fast Offset Compensation for accelerometer.
 *
 * @param[in] accel_g_value : This parameter selects the accel FOC
 * axis to be performed
 *
 * Input format is {x, y, z, sign}. '1' to enable. '0' to disable
 *
 * Eg: To choose x axis  {1, 0, 0, 0}
 * Eg: To choose -x axis {1, 0, 0, 1}
 *
 * @param[in] acc_cfg       : Accelerometer configuration value
 * @param[in] dev           : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 */
static int8_t perform_accel_foc(const struct bmi3_accel_foc_g_value *accel_g_value,
				const struct bmi3_accel_config *acc_cfg,
				struct bmi3_dev *dev);

/*!
 * @brief This internal API converts the range value into accelerometer
 * corresponding integer value.
 *
 * @param[in] range_in      : Input range value.
 * @param[out] range_out    : Stores the integer value of range.
 *
 * @return None
 * @retval None
 */
static void map_accel_range(uint8_t range_in, uint8_t *range_out);

/*!
 * @brief This internal API compensate the accelerometer data against gravity.
 *
 * @param[in] lsb_per_g     : LSB value per 1g.
 * @param[in] g_val         : Gravity reference value of all axis.
 * @param[in] data          : Accelerometer data
 * @param[out] comp_data    : Stores the data that is compensated by taking the
 *                            difference in accelerometer data and lsb_per_g
 *                            value.
 *
 * @return None
 * @retval None
 */
static void comp_for_gravity(uint16_t lsb_per_g,
			     const struct bmi3_accel_foc_g_value *g_val,
			     const struct bmi3_sens_axes_data *data,
			     struct bmi3_offset_delta *comp_data);

/*!
 * @brief This internal API scales the compensated accelerometer data according
 * to the offset register resolution.
 *
 * @param[in] range         : Gravity range of the accelerometer.
 * @param[out] comp_data    : Data that is compensated by taking the
 *                            difference in accelerometer data and lsb_per_g
 *                            value.
 * @param[out] data         : Stores accel offset data
 *
 * @return None
 * @retval None
 */
static void scale_accel_offset(uint8_t range,
			       const struct bmi3_offset_delta *comp_data,
			       struct bmi3_acc_usr_gain_offset *data);

/*!
 * @brief This internal API finds the bit position of 3.9mg according to given
 * range and resolution.
 *
 * @param[in] range     : Gravity range of the accelerometer.
 *
 * @return Result of API execution status
 * @retval Bit position of 3.9mg
 */
static int8_t get_bit_pos_3_9mg(uint8_t range);

/*!
 * @brief This internal API inverts the accelerometer offset data.
 *
 * @param[out] offset_data  : Stores the inverted offset data
 *
 * @return None
 * @retval None
 */
static void invert_accel_offset(struct bmi3_acc_usr_gain_offset *offset_data);

/*!
 * @brief This internal API is used to calculate the power of a value.
 *
 * @param[in] base          : Base for power calculation.
 * @param[in] resolution    : Exponent for power calculation.
 *
 * @return the calculated power
 * @retval the power value
 */
static int32_t power(int16_t base, uint8_t resolution);

/*!
 * @brief This internal API sets the individual gyroscope
 * filter coefficients in the respective dma registers.
 *
 * @param[in] dev          : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t set_gyro_filter_coefficients(struct bmi3_dev *dev);

/*!
 * @brief This internal API check the data index for the fifo
 * data processing.
 *
 * @param[in] data_index   : Variable for data index
 * @param[in] fifo         : Structure instance of fifo frame
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t check_data_index(uint16_t data_index, const struct bmi3_fifo_frame *fifo);

/*!
 * @brief This internal API is used to validate ODR and AVG combinations for accel
 *
 * @param[in] acc_odr   : Variable to store accel ODR
 * @param[in] acc_avg   : Variable to store accel AVG
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t validate_acc_odr_avg(uint8_t acc_odr, uint8_t acc_avg);

/*!
 * @brief This internal API is used to validate ODR and AVG combinations for gyro
 *
 * @param[in] gyr_odr   : Variable to store gyro ODR
 * @param[in] gyr_avg   : Variable to store gyro AVG
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t validate_gyr_odr_avg(uint8_t gyr_odr, uint8_t gyr_avg);

/*!
 * @brief This internal API writes the command register value to enable cfg res.
 *
 * @param[in,out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t config_array_set_command(struct bmi3_dev *dev);

/*!
 * @brief This internal API sets the value_one to feature engine in cfg res.
 *
 * @param[in,out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t config_array_set_value_one_page(struct bmi3_dev *dev);

/*!
 * @brief This internal API writes the config array in cfg res.
 *
 * @param[in,out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t write_config_array(struct bmi3_dev *dev);

/*!
 * @brief This internal API writes config array.
 *
 * @param[in] config_array  : Pointer variable to store config array.
 * @param[in] config_size   : Variable to store size of config array.
 * @param[in,out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t load_config_array(const uint8_t *config_array, uint16_t config_size, struct bmi3_dev *dev);

/*!
 * @brief This internal API writes config array.
 *
 * @param[in] config_array  : Pointer variable to store config array.
 * @param[in] config_size   : Variable to store size of config array.
 * @param[in,out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t upload_file(uint16_t config_base_addr, const uint8_t *config_array, uint16_t indx, struct bmi3_dev *dev);

/*!
 * @brief This internal API writes config version array to feature engine register.
 *
 * @param[in,out] dev       : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
static int8_t write_config_version(struct bmi3_dev *dev);

/******************************************************************************/
/*!  @name      User Interface Definitions                            */
/******************************************************************************/

/*!
 * @brief This API is the entry point for bmi3 sensor. It reads and validates the
 * chip-id of the sensor.
 */
int8_t bmi3_init(struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to assign chip id */
    uint8_t chip_id[2] = { 0 };

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if (rslt == BMI3_OK)
    {
	dev->chip_id = 0;

	/* An extra dummy byte is read during SPI read */
	if (dev->intf == BMI3_SPI_INTF)
	{
	    dev->dummy_byte = 1;
        } else
	{
	    dev->dummy_byte = 2;
	}
    }

    if (rslt == BMI3_OK)
    {
	/* Perform soft-reset to bring all register values to their default values */
	rslt = bmi3_soft_reset(dev);

	if (rslt == BMI3_OK)
	{
	    /* Read chip-id of the BMI3 sensor */
	    rslt = bmi3_get_regs(BMI3_REG_CHIP_ID, chip_id, 2, dev);

	    if (rslt == BMI3_OK)
	    {
		dev->chip_id = chip_id[0];
	    }
	}
    }

    return rslt;
}

/*!
 * @brief This API reads the data from the given register address of bmi3
 * sensor.
 *
 * @note For most of the registers auto address increment applies, with the
 * exception of a few special registers, which trap the address. For e.g.,
 * Register address - 0x03.
 */
int8_t bmi3_get_regs(uint8_t reg_addr, uint8_t *data, uint16_t len, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define temporary buffer */
    uint8_t temp_buf[BMI3_MAX_LEN];

    /* Variable to define loop */
    uint16_t index = 0;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if ((rslt == BMI3_OK) && (data != NULL))
    {
	/* Configuring reg_addr for SPI Interface */
	if (dev->intf == BMI3_SPI_INTF)
	{
	    reg_addr = (reg_addr | BMI3_SPI_RD_MASK);
	}

	dev->intf_rslt = dev->read(reg_addr, temp_buf, len + dev->dummy_byte, dev->intf_ptr);
	dev->delay_us(2, dev->intf_ptr);

	if (dev->intf_rslt == BMI3_INTF_RET_SUCCESS)
	{
	    /* Read the data from the position next to dummy byte */
	    while (index < len)
	    {
		data[index] = temp_buf[index + dev->dummy_byte];
		index++;
	    }
        } else
	{
	    rslt = BMI3_E_COM_FAIL;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API writes data to the given register address of bmi3 sensor.
 */
int8_t bmi3_set_regs(uint8_t reg_addr, const uint8_t *data, uint16_t len, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if ((rslt == BMI3_OK) && (data != NULL))
    {
	/* Configuring reg_addr for SPI Interface */
	if (dev->intf == BMI3_SPI_INTF)
	{
	    reg_addr = (reg_addr & BMI3_SPI_WR_MASK);
	}

	dev->intf_rslt = dev->write(reg_addr, data, len, dev->intf_ptr);
	dev->delay_us(2, dev->intf_ptr);

	if (dev->intf_rslt != BMI3_INTF_RET_SUCCESS)
	{
	    rslt = BMI3_E_COM_FAIL;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API resets bmi3 sensor. All registers are overwritten with
 * their default values.
 */
int8_t bmi3_soft_reset(struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to read the dummy byte */
    uint8_t dummy_byte[2] = { 0 };

    /* Variable to store feature data array */
    uint8_t feature_data[2] = { 0x2c, 0x01 };

    /* Variable to enable feature engine bit */
    uint8_t feature_engine_en[2] = { BMI3_ENABLE, 0 };

    /* Variable to store status value for feature engine enable */
    uint8_t reg_data[2] = { 0 };

    /* Array variable to store feature IO status */
    uint8_t feature_io_status[2] = { BMI3_ENABLE, 0 };

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if (rslt == BMI3_OK)
    {
	/* Reset bmi3 device */
	rslt = bmi3_set_command_register(BMI3_CMD_SOFT_RESET, dev);
	dev->delay_us(BMI3_SOFT_RESET_DELAY, dev->intf_ptr);

	/* Performing a dummy read after a soft-reset */
	if ((rslt == BMI3_OK) && (dev->intf == BMI3_SPI_INTF))
	{
	    rslt = bmi3_get_regs(BMI3_REG_CHIP_ID, dummy_byte, 2, dev);
	}

	/* Enabling Feature engine */
	if (rslt == BMI3_OK)
	{
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_IO2, feature_data, 2, dev);
	}

	if (rslt == BMI3_OK)
	{
	    /* Enabling feature status bit */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_IO_STATUS, feature_io_status, 2, dev);
	}

	if (rslt == BMI3_OK)
	{
	    /* Enable feature engine bit */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_CTRL, feature_engine_en, 2, dev);
	}
		if (rslt == BMI3_OK)
	{
			/* Checking the status bit for feature engine enable */
			while (!(reg_data[0] & BMI3_FEATURE_ENGINE_ENABLE_MASK))
			{
				rslt = bmi3_get_regs(BMI3_REG_FEATURE_IO1, reg_data, 2, dev);
			}
		}
    }

    return rslt;
}

/*!
 * @brief This API writes the available sensor specific commands to the sensor.
 */
int8_t bmi3_set_command_register(uint16_t command, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array variable to store command value */
    uint8_t reg_data[2] = { 0 };

    reg_data[0] = (uint8_t)(command & BMI3_SET_LOW_BYTE);
    reg_data[1] = (uint8_t)((command & BMI3_SET_HIGH_BYTE) >> 8);

    /* Set the command in the command register */
    rslt = bmi3_set_regs(BMI3_REG_CMD, reg_data, 2, dev);

    return rslt;
}

/*!
 * @brief This API gets the interrupt 1 status of both feature and data
 * interrupts
 */
int8_t bmi3_get_int1_status(uint16_t *int_status, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    if (int_status != NULL)
    {
	/* Get the interrupt status */
	rslt = bmi3_get_regs(BMI3_REG_INT_STATUS_INT1, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    *int_status = (uint16_t)(data_array[0] | ((uint16_t)data_array[1] << 8));
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets the interrupt 2 status of both feature and data
 * interrupts
 */
int8_t bmi3_get_int2_status(uint16_t *int_status, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    if (int_status != NULL)
    {
	/* Get the interrupt status */
	rslt = bmi3_get_regs(BMI3_REG_INT_STATUS_INT2, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    *int_status = (uint16_t)(data_array[0] | ((uint16_t)data_array[1] << 8));
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets the re-mapped x, y and z axes from the sensor and
 * updates the values in the device structure.
 *
 * @note: XYZ axis denotes x = x, y = y, z = z
 * Similarly,
 *    YXZ, x = y, y = x, z = z
 *
 */
int8_t bmi3_get_remap_axes(struct bmi3_axes_remap *remapped_axis, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array variable to get remapped axis data */
    uint8_t remap_data[4];

    /* Array variable to store base address of remap axis data */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_AXIS_REMAP, 0 };

    /* Variable to store axis remap data */
    uint8_t reg_data;

    if (remapped_axis != NULL)
    {
	/* Set the configuration to feature engine register */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where axis
	     * re-mapping feature resides
	     */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, remap_data, 4, dev);

	    if (rslt == BMI3_OK)
	    {
		reg_data = remap_data[0];

		/* Get the re-mapped axis */
		remapped_axis->axis_map = BMI3_GET_BIT_POS0(reg_data, BMI3_XYZ_AXIS);

		/* Get the re-mapped x-axis polarity */
		remapped_axis->invert_x = BMI3_GET_BITS(reg_data, BMI3_X_AXIS_SIGN);

		/* Get the re-mapped y-axis polarity */
		remapped_axis->invert_y = BMI3_GET_BITS(reg_data, BMI3_Y_AXIS_SIGN);

		/* Get the re-mapped z-axis polarity */
		remapped_axis->invert_z = BMI3_GET_BITS(reg_data, BMI3_Z_AXIS_SIGN);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets the re-mapped x, y and z axes to the sensor and
 * updates them in the device structure.
 */
int8_t bmi3_set_remap_axes(const struct bmi3_axes_remap remapped_axis, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Structure instance of sensor config */
    struct bmi3_sens_config config = { 0 };

    /* Variable to set the re-mapped axes in the sensor */
    uint8_t axis_map;

    /* Variable to set the re-mapped x-axes sign in the sensor */
    uint8_t invert_x;

    /* Variable to set the re-mapped y-axes sign in the sensor */
    uint8_t invert_y;

    /* Variable to set the re-mapped z-axes sign in the sensor */
    uint8_t invert_z;

    /* Variable to define the register address */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_AXIS_REMAP, 0 };

    /* Array variable to get remapped axis data */
    uint8_t remap_data[4];

    /* Set the configuration to feature engine register */
    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

    if (rslt == BMI3_OK)
    {
	/* Set the value of re-mapped axis */
	axis_map = remapped_axis.axis_map & BMI3_XYZ_AXIS_MASK;

	/* Set the value of re-mapped x-axis sign */
	invert_x = ((remapped_axis.invert_x << BMI3_X_AXIS_SIGN_POS) & BMI3_X_AXIS_SIGN_MASK);

	/* Set the value of re-mapped y-axis sign */
	invert_y = ((remapped_axis.invert_y << BMI3_Y_AXIS_SIGN_POS) & BMI3_Y_AXIS_SIGN_MASK);

	/* Set the value of re-mapped z-axis sign */
	invert_z = ((remapped_axis.invert_z << BMI3_Z_AXIS_SIGN_POS) & BMI3_Z_AXIS_SIGN_MASK);

	remap_data[0] = (axis_map | invert_x | invert_y | invert_z);

	/* Set the configuration back to the page */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, remap_data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    rslt = axes_remap_acc_power_mode_status(config, dev);
	}
    }

    return rslt;
}

/*!
 * @brief This API sets the sensor/feature configuration.
 */
int8_t bmi3_set_sensor_config(struct bmi3_sens_config *sens_cfg, uint8_t n_sens, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define loop */
    uint8_t loop;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if ((rslt == BMI3_OK) && (sens_cfg != NULL))
    {
	for (loop = 0; loop < n_sens; loop++)
	{
	    switch (sens_cfg[loop].type)
	    {
		case BMI3_ACCEL:
		    rslt = set_accel_config(&sens_cfg[loop].cfg.acc, dev);
		    break;

		case BMI3_GYRO:
		    rslt = set_gyro_config(&sens_cfg[loop].cfg.gyr, dev);
		    break;

		case BMI3_ANY_MOTION:
		    rslt = set_any_motion_config(&sens_cfg[loop].cfg.any_motion, dev);
		    break;

		case BMI3_NO_MOTION:
		    rslt = set_no_motion_config(&sens_cfg[loop].cfg.no_motion, dev);
		    break;

		case BMI3_SIG_MOTION:
		    rslt = set_sig_motion_config(&sens_cfg[loop].cfg.sig_motion, dev);
		    break;

		case BMI3_FLAT:
		    rslt = set_flat_config(&sens_cfg[loop].cfg.flat, dev);
		    break;

		case BMI3_TILT:
		    rslt = set_tilt_config(&sens_cfg[loop].cfg.tilt, dev);
		    break;

		case BMI3_ORIENTATION:
		    rslt = set_orientation_config(&sens_cfg[loop].cfg.orientation, dev);
		    break;

		case BMI3_STEP_COUNTER:
		    rslt = set_step_config(&sens_cfg[loop].cfg.step_counter, dev);
		    break;

		case BMI3_TAP:
		    rslt = set_tap_config(&sens_cfg[loop].cfg.tap, dev);
		    break;

		case BMI3_ALT_ACCEL:
		    rslt = set_alternate_accel_config(&sens_cfg[loop].cfg.alt_acc, dev);
		    break;

		case BMI3_ALT_GYRO:
		    rslt = set_alternate_gyro_config(&sens_cfg[loop].cfg.alt_gyr, dev);
		    break;

		case BMI3_ALT_AUTO_CONFIG:
		    rslt = set_alternate_auto_config(&sens_cfg[loop].cfg.alt_auto_cfg, dev);
		    break;

		default:
		    rslt = BMI3_E_INVALID_SENSOR;
		    break;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets the sensor/feature configuration.
 */
int8_t bmi3_get_sensor_config(struct bmi3_sens_config *sens_cfg, uint8_t n_sens, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define loop */
    uint8_t loop = 0;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if ((rslt == BMI3_OK) && (sens_cfg != NULL))
    {
	for (loop = 0; loop < n_sens; loop++)
	{
	    switch (sens_cfg[loop].type)
	    {
		case BMI3_ACCEL:
		    rslt = get_accel_config(&sens_cfg[loop].cfg.acc, dev);
		    break;

		case BMI3_GYRO:
		    rslt = get_gyro_config(&sens_cfg[loop].cfg.gyr, dev);
		    break;

		case BMI3_ANY_MOTION:
		    rslt = get_any_motion_config(&sens_cfg[loop].cfg.any_motion, dev);
		    break;

		case BMI3_NO_MOTION:
		    rslt = get_no_motion_config(&sens_cfg[loop].cfg.no_motion, dev);
		    break;

		case BMI3_SIG_MOTION:
		    rslt = get_sig_motion_config(&sens_cfg[loop].cfg.sig_motion, dev);
		    break;

		case BMI3_FLAT:
		    rslt = get_flat_config(&sens_cfg[loop].cfg.flat, dev);
		    break;

		case BMI3_TILT:
		    rslt = get_tilt_config(&sens_cfg[loop].cfg.tilt, dev);
		    break;

		case BMI3_ORIENTATION:
		    rslt = get_orientation_config(&sens_cfg[loop].cfg.orientation, dev);
		    break;

		case BMI3_STEP_COUNTER:
		    rslt = get_step_config(&sens_cfg[loop].cfg.step_counter, dev);
		    break;

		case BMI3_TAP:
		    rslt = get_tap_config(&sens_cfg[loop].cfg.tap, dev);
		    break;

		case BMI3_ALT_ACCEL:
		    rslt = get_alternate_accel_config(&sens_cfg[loop].cfg.alt_acc, dev);
		    break;

		case BMI3_ALT_GYRO:
		    rslt = get_alternate_gyro_config(&sens_cfg[loop].cfg.alt_gyr, dev);
		    break;

		case BMI3_ALT_AUTO_CONFIG:
		    rslt = get_alternate_auto_config(&sens_cfg[loop].cfg.alt_auto_cfg, dev);
		    break;

		default:
		    rslt = BMI3_E_INVALID_SENSOR;
		    break;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API maps/un-maps data interrupts to that of interrupt pins.
 */
int8_t bmi3_map_interrupt(struct bmi3_map_int map_int, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to store register data */
    uint8_t reg_data[4] = { 0 };

    uint16_t no_motion_out, any_motion_out, flat_out, orientation_out, step_detector_out, step_counter_out,
	     sig_motion_out, tilt_out;
    uint16_t tap_out, i3c_out, err_status, temp, gyr, acc, fwm, ffull;

    /* Read interrupt map1 and map2 and register */
    rslt = bmi3_get_regs(BMI3_REG_INT_MAP1, reg_data, 4, dev);

    if (rslt == BMI3_OK)
    {
	no_motion_out = BMI3_SET_BIT_POS0(reg_data[0], BMI3_NO_MOTION_OUT, map_int.no_motion_out);
	any_motion_out = BMI3_SET_BITS(reg_data[0], BMI3_ANY_MOTION_OUT, map_int.any_motion_out);
	flat_out = BMI3_SET_BITS(reg_data[0], BMI3_FLAT_OUT, map_int.flat_out);
	orientation_out = BMI3_SET_BITS(reg_data[0], BMI3_ORIENTATION_OUT, map_int.orientation_out);
	step_detector_out = BMI3_SET_BITS(reg_data[1], BMI3_STEP_DETECTOR_OUT, map_int.step_detector_out);
	step_counter_out = BMI3_SET_BITS(reg_data[1], BMI3_STEP_COUNTER_OUT, map_int.step_counter_out);
	sig_motion_out = BMI3_SET_BITS(reg_data[1], BMI3_SIG_MOTION_OUT, map_int.sig_motion_out);
	tilt_out = BMI3_SET_BITS(reg_data[1], BMI3_TILT_OUT, map_int.tilt_out);

	reg_data[0] = (uint8_t)(no_motion_out | any_motion_out | flat_out | orientation_out);
	reg_data[1] = (uint8_t)((step_detector_out | step_counter_out | sig_motion_out | tilt_out) >> 8);

	tap_out = BMI3_SET_BIT_POS0(reg_data[2], BMI3_TAP_OUT, map_int.tap_out);
	i3c_out = BMI3_SET_BITS(reg_data[2], BMI3_I3C_OUT, map_int.i3c_out);
	err_status = BMI3_SET_BITS(reg_data[2], BMI3_ERR_STATUS, map_int.err_status);
	temp = BMI3_SET_BITS(reg_data[2], BMI3_TEMP_DRDY_INT, map_int.temp_drdy_int);
	gyr = BMI3_SET_BITS(reg_data[3], BMI3_GYR_DRDY_INT, map_int.gyr_drdy_int);
	acc = BMI3_SET_BITS(reg_data[3], BMI3_ACC_DRDY_INT, map_int.acc_drdy_int);
	fwm = BMI3_SET_BITS(reg_data[3], BMI3_FIFO_WATERMARK_INT, map_int.fifo_watermark_int);
	ffull = BMI3_SET_BITS(reg_data[3], BMI3_FIFO_FULL_INT, map_int.fifo_full_int);

	reg_data[2] = (uint8_t)(tap_out | i3c_out | err_status | temp);
	reg_data[3] = (uint8_t)((gyr | acc | fwm | ffull) >> 8);

	rslt = bmi3_set_regs(BMI3_REG_INT_MAP1, reg_data, 4, dev);
    }

    return rslt;
}

/*!
 * @brief This API selects the sensors/features to be enabled or disabled.
 */
int8_t bmi3_select_sensor(struct bmi3_feature_enable *enable, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    if (enable != NULL)
    {
	rslt = set_feature_enable(enable, dev);

	if (rslt == BMI3_OK)
	{
	    rslt = get_feature_enable(enable, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets the sensor/feature data for accelerometer, gyroscope,
 * step counter, orientation, i3c sync accel, i3c sync gyro and i3c sync temperature.
 */
int8_t bmi3_get_sensor_data(struct bmi3_sensor_data *sensor_data, uint8_t n_sens, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define loop */
    uint8_t loop;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if ((rslt == BMI3_OK) && (sensor_data != NULL))
    {
	for (loop = 0; loop < n_sens; loop++)
	{
	    switch (sensor_data[loop].type)
	    {
		case BMI3_ACCEL:
		    rslt = get_accel_sensor_data(&sensor_data[loop].sens_data.acc, BMI3_REG_ACC_DATA_X, dev);
		    break;

		case BMI3_GYRO:
		    rslt = get_gyro_sensor_data(&sensor_data[loop].sens_data.gyr, BMI3_REG_GYR_DATA_X, dev);
		    break;

		case BMI3_STEP_COUNTER:
		    rslt = get_step_counter_sensor_data(&sensor_data[loop].sens_data.step_counter_output,
							BMI3_REG_FEATURE_IO2,
							dev);
		    break;

		case BMI3_ORIENTATION:
		    rslt = get_orient_output_data(&sensor_data[loop].sens_data.orient_output,
						  BMI3_REG_FEATURE_EVENT_EXT,
						  dev);
		    break;

		case BMI3_I3C_SYNC_ACCEL:
		    rslt = get_i3c_sync_accel_sensor_data(&sensor_data[loop].sens_data.i3c_sync, dev);
		    break;

		case BMI3_I3C_SYNC_GYRO:
		    rslt = get_i3c_sync_gyro_sensor_data(&sensor_data[loop].sens_data.i3c_sync, dev);
		    break;

		case BMI3_I3C_SYNC_TEMP:
		    rslt = get_i3c_sync_temp_data(&sensor_data[loop].sens_data.i3c_sync, dev);
		    break;

		default:
		    rslt = BMI3_E_INVALID_SENSOR;
		    break;
	    }

	    /* Return error if any of the get sensor data fails */
	    if (rslt != BMI3_OK)
	    {
		break;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 *  @brief This API reads the error status from the sensor.
 */
int8_t bmi3_get_error_status(struct bmi3_err_reg *err_reg, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array variable to get error status from register */
    uint8_t data[2] = { 0 };

    uint16_t reg_data;

    if (err_reg != NULL)
    {
	/* Read the error codes */
	rslt = bmi3_get_regs(BMI3_REG_ERR_REG, data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    reg_data = data[0];

	    /* Fatal error */
	    err_reg->fatal_err = BMI3_GET_BIT_POS0(reg_data, BMI3_FATAL_ERR);

	    /* Interrupt request overrun error */
	    err_reg->feat_eng_ovrld = BMI3_GET_BITS(reg_data, BMI3_FEAT_ENG_OVRLD);

	    /* Indicates watch cell code */
	    err_reg->feat_eng_wd = BMI3_GET_BITS(reg_data, BMI3_FEAT_ENG_WD);

	    /* Indicates accel configuration error */
	    err_reg->acc_conf_err = BMI3_GET_BITS(reg_data, BMI3_ACC_CONF_ERR);

	    /* Indicates gyro configuration error */
	    err_reg->gyr_conf_err = BMI3_GET_BITS(reg_data, BMI3_GYR_CONF_ERR);

	    reg_data = data[1];

	    /* Indicates SDR parity error */
	    err_reg->i3c_error0 = BMI3_GET_BITS(reg_data, BMI3_I3C_ERROR0);

	    /* Indicates I3C error */
	    err_reg->i3c_error3 = BMI3_GET_BITS(reg_data, BMI3_I3C_ERROR3);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 *  @brief This API reads the feature engine error status from the sensor.
 */
int8_t bmi3_get_feature_engine_error_status(uint8_t *feature_engine_err_reg_lsb,
					    uint8_t *feature_engine_err_reg_msb,
					    struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array variable to get error status from register */
    uint8_t data[2];

    if ((feature_engine_err_reg_lsb != NULL) && (feature_engine_err_reg_msb != NULL))
    {
	/* Read the feature engine error codes */
	rslt = bmi3_get_regs(BMI3_REG_FEATURE_IO1, data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    *feature_engine_err_reg_lsb = data[0];

	    *feature_engine_err_reg_msb = data[1];
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets:
 *        1) The output configuration of the selected interrupt pin:
 *           INT1 or INT2.
 *        2) The interrupt mode: permanently latched or non-latched.
 */
int8_t bmi3_set_int_pin_config(const struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define data array */
    uint8_t data_array[3] = { 0 };

    /* Variable to store register data */
    uint16_t reg_data = 0;

    /* Variable to define type of interrupt pin  */
    uint8_t int_pin = 0;

    uint16_t lvl, od, output_en;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if ((rslt == BMI3_OK) && (int_cfg != NULL))
    {
	/* Copy the pin type to a local variable */
	int_pin = int_cfg->pin_type;

	if ((int_pin > BMI3_INT_NONE) && (int_pin < BMI3_INT_PIN_MAX))
	{
	    /* Get the previous configuration data */
	    rslt = bmi3_get_regs(BMI3_REG_IO_INT_CTRL, data_array, 2, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Set interrupt pin 1 configuration */
		if (int_pin == BMI3_INT1)
		{
		    reg_data = data_array[0];

		    /* Configure active low or high */
		    lvl = BMI3_SET_BIT_POS0(reg_data, BMI3_INT1_LVL, int_cfg->pin_cfg[0].lvl);

		    /* Configure push-pull or open drain */
		    od = BMI3_SET_BITS(reg_data, BMI3_INT1_OD, int_cfg->pin_cfg[0].od);

		    /* Configure output enable */
		    output_en = BMI3_SET_BITS(reg_data, BMI3_INT1_OUTPUT_EN, int_cfg->pin_cfg[0].output_en);

		    /* Copy the data to be written in the respective array */
		    data_array[0] = (uint8_t)(lvl | od | output_en);
		}

		/* Set interrupt pin 2 configuration */
		if (int_pin == BMI3_INT2)
		{
		    reg_data = ((uint16_t)data_array[1] << 8);

		    /* Configure active low or high */
		    lvl = BMI3_SET_BITS(reg_data, BMI3_INT2_LVL, int_cfg->pin_cfg[1].lvl);

		    /* Configure push-pull or open drain */
		    od = BMI3_SET_BITS(reg_data, BMI3_INT2_OD, int_cfg->pin_cfg[1].od);

		    /* Configure output enable */
		    output_en = BMI3_SET_BITS(reg_data, BMI3_INT2_OUTPUT_EN, int_cfg->pin_cfg[1].output_en);

		    /* Copy the data to be written in the respective array */
		    data_array[1] = (uint8_t)((lvl | od | output_en) >> 8);
		}

		/* Set the configurations simultaneously as
		 * INT1_IO_CTRL, INT2_IO_CTRL, and INT_LATCH lie
		 * in consecutive addresses
		 */
		rslt = bmi3_set_regs(BMI3_REG_IO_INT_CTRL, data_array, 2, dev);

		if (rslt == BMI3_OK)
		{
		    rslt = set_latch_mode(int_cfg, dev);
		}
	    }
        } else
	{
	    rslt = BMI3_E_INVALID_INT_PIN;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets:
 *        1) The output configuration of the selected interrupt pin:
 *           INT1 or INT2.
 *        2) The interrupt mode: permanently latched or non-latched.
 */
int8_t bmi3_get_int_pin_config(struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define data array */
    uint8_t data_array[3] = { 0 };

    /* Variable to define type of interrupt pin  */
    uint8_t int_pin = 0;

    uint16_t reg_data;

    if (int_cfg != NULL)
    {
	/* Copy the pin type to a local variable */
	int_pin = int_cfg->pin_type;

	/* Get the previous configuration data */
	rslt = bmi3_get_regs(BMI3_REG_IO_INT_CTRL, data_array, 3, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get interrupt pin 1 configuration */
	    if (int_pin == BMI3_INT1)
	    {
		/* Get active low or high */
		int_cfg->pin_cfg[0].lvl = BMI3_GET_BIT_POS0(data_array[0], BMI3_INT1_LVL);

		/* Get push-pull or open drain */
		int_cfg->pin_cfg[0].od = BMI3_GET_BITS(data_array[0], BMI3_INT1_OD);

		/* Get output enable */
		int_cfg->pin_cfg[0].output_en = BMI3_GET_BITS(data_array[0], BMI3_INT1_OUTPUT_EN);
	    }

	    /* Get interrupt pin 2 configuration */
	    if (int_pin == BMI3_INT2)
	    {
		reg_data = ((uint16_t)data_array[1] << 8);

		/* Get active low or high */
		int_cfg->pin_cfg[1].lvl = BMI3_GET_BITS(reg_data, BMI3_INT2_LVL);

		/* Get push-pull or open drain */
		int_cfg->pin_cfg[1].od = BMI3_GET_BITS(reg_data, BMI3_INT2_OD);

		/* Get output enable */
		int_cfg->pin_cfg[1].output_en = BMI3_GET_BITS(reg_data, BMI3_INT2_OUTPUT_EN);
	    }

	    rslt = get_latch_mode(int_cfg, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API is used to get the sensor time.
 */
int8_t bmi3_get_sensor_time(uint32_t *sensor_time, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t reg_data[4] = { 0 };

    if (sensor_time != NULL)
    {
	rslt = bmi3_get_regs(BMI3_REG_SENSOR_TIME_0, reg_data, 4, dev);
	if (rslt == BMI3_OK)
	{
	    *sensor_time =
		(uint32_t)(reg_data[0] | (uint32_t)reg_data[1] << 8 | (uint32_t)reg_data[2] << 16 |
			   (uint32_t)reg_data[3] <<
		    24);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API reads the raw temperature data from the register and can be
 * converted into degree celsius.
 */
int8_t bmi3_get_temperature_data(uint16_t *temp_data, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define data stored in register */
    uint8_t reg_data[2] = { 0 };

    if (temp_data != NULL)
    {
	/* Read the sensor data */
	rslt = bmi3_get_regs(BMI3_REG_TEMP_DATA, reg_data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    *temp_data = (uint16_t)(reg_data[0] | ((uint16_t)reg_data[1] << 8));
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API reads the FIFO data.
 */
int8_t bmi3_read_fifo_data(struct bmi3_fifo_frame *fifo, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store FIFO configuration data */
    uint8_t config_data[2] = { 0 };

    /* Variable to store FIFO data address */
    uint8_t reg_addr = BMI3_REG_FIFO_DATA;

    /* Null-pointer check */
    if (fifo != NULL)
    {
	/* Get the set FIFO frame configurations */
	rslt = bmi3_get_regs(BMI3_REG_FIFO_CONF, config_data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get sensor enable status, of which the data is to be read */
	    fifo->available_fifo_sens =
		(uint16_t)(((config_data[0]) | ((uint16_t) config_data[1] << 8)) & BMI3_FIFO_ALL_EN);

	    if (fifo->length != 0)
	    {
		/* Read FIFO data */
		if (dev->intf == BMI3_SPI_INTF)
		{
		    reg_addr = (reg_addr | BMI3_SPI_RD_MASK);
		}

		rslt = dev->read(reg_addr, fifo->data, (uint32_t)fifo->length, dev->intf_ptr);
            } else
	    {
		rslt = BMI3_E_COM_FAIL;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API parses and extracts the accelerometer frames from FIFO data
 * read by the "bmi3_read_fifo_data" API and stores it in the "accel_data"
 * structure instance.
 */
int8_t bmi3_extract_accel(struct bmi3_fifo_sens_axes_data *accel_data,
			  struct bmi3_fifo_frame *fifo,
			  const struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to index the bytes */
    uint16_t data_index = 0;

    /* Variable to index accelerometer frames */
    uint16_t accel_index = 0;

    /* Variable to define the data enable byte */
    uint16_t data_enable = 0;

    rslt = null_ptr_check(dev);

    /* Null-pointer check */
    if ((rslt == BMI3_OK) && (accel_data != NULL) && (fifo != NULL))
    {
	data_index = dev->dummy_byte;

	/* Convert word to byte since all sensor enables are in a byte */
	data_enable = fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_ACC_FRM;

	for (; (data_index < fifo->length);)
	{
	    rslt = check_data_index(data_index, fifo);

	    if (rslt == BMI3_OK)
	    {
		/* Unpack frame to get the accelerometer data */
		rslt = unpack_fifo_data_frame(accel_data, &data_index, &accel_index, data_enable, fifo);
	    }

	    if (rslt != BMI3_W_PARTIAL_READ)
	    {
		/* If gyro enable with the accel, increment the accel index by 6
		 * since gyro frame has 6 bytes of data
		 */
		if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_GYR_FRM)
		{
		    data_index = data_index + BMI3_LENGTH_FIFO_GYR;
		}

		/* If sensor time enable with the accel, increment the accel index by 2
		 * since sensor time frame has 2 bytes of data
		 */
		if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_SENS_TIME_FRM)
		{
		    data_index = data_index + BMI3_LENGTH_SENSOR_TIME;
		}

		/* If temperature enable with the accel, increment the accel index by 2
		 * since temperature frame has 2 bytes of data
		 */
		if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_TEMP_FRM)
		{
		    data_index = data_index + BMI3_LENGTH_TEMPERATURE;
		}
	    }
	}

	/* Update number of accelerometer frames to be read */
	(fifo->avail_fifo_accel_frames) = accel_index;

	if (fifo->avail_fifo_accel_frames != 0)
	{
	    rslt = BMI3_OK;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API parses and extracts the temperature frames from FIFO data
 * read by the "bmi3_read_fifo_data" API and stores it in the "temp_data"
 * structure instance.
 */
int8_t bmi3_extract_temperature(struct bmi3_fifo_temperature_data *temp_data,
				struct bmi3_fifo_frame *fifo,
				const struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to index the bytes */
    uint16_t data_index = 0;

    /* Variable to index temperature frames */
    uint16_t temp_index = 0;

    /* Variable to define the data enable byte */
    uint16_t data_enable = 0;

    rslt = null_ptr_check(dev);

    /* Null-pointer check */
    if ((rslt == BMI3_OK) && (temp_data != NULL) && (fifo != NULL))
    {
	data_index = dev->dummy_byte;

	/* Convert word to byte since all sensor enables are in a byte */
	data_enable = fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_TEMP_FRM;

	for (; (data_index < fifo->length);)
	{
	    /* If accel enable with the temperature, increment the temperature index by 6
	     * since accel frame has 6 bytes of data
	     */
	    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_ACC_FRM)
	    {
		data_index = data_index + BMI3_LENGTH_FIFO_ACC;
	    }

	    /* If gyro enable with the temperature, increment the temperature index by 6
	     * since gyro frame has 6 bytes of data
	     */
	    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_GYR_FRM)
	    {
		data_index = data_index + BMI3_LENGTH_FIFO_GYR;
	    }

	    rslt = check_data_index(data_index, fifo);

	    if (rslt == BMI3_OK)
	    {
		/* Unpack frame to get the temperature data */
		rslt = unpack_fifo_data_frame(temp_data, &data_index, &temp_index, data_enable, fifo);
	    }

	    if (rslt != BMI3_W_PARTIAL_READ)
	    {
		/* If sensor time enable with the temperature, increment the temperature index by 2
		 * since sensor time frame has 2 bytes of data
		 */
		if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_SENS_TIME_FRM)
		{
		    data_index = data_index + BMI3_LENGTH_SENSOR_TIME;
		}
	    }
	}

	/* Update number of temperature frames to be read */
	(fifo->avail_fifo_temp_frames) = temp_index;

	if (fifo->avail_fifo_temp_frames != 0)
	{
	    rslt = BMI3_OK;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API parses and extracts the gyro frames from FIFO data
 * read by the "bmi3_read_fifo_data" API and stores it in the "gyro_data"
 * structure instance.
 */
int8_t bmi3_extract_gyro(struct bmi3_fifo_sens_axes_data *gyro_data,
			 struct bmi3_fifo_frame *fifo,
			 const struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to index the bytes */
    uint16_t data_index = 0;

    /* Variable to index gyro frames */
    uint16_t gyro_index = 0;

    /* Variable to define the data enable byte */
    uint16_t data_enable = 0;

    rslt = null_ptr_check(dev);

    /* Null-pointer check */
    if ((rslt == BMI3_OK) && (gyro_data != NULL) && (fifo != NULL))
    {
	data_index = dev->dummy_byte;

	/* Convert word to byte since all sensor enables are in a byte */
	data_enable = fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_GYR_FRM;

	for (; (data_index < fifo->length);)
	{
	    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_ACC_FRM)
	    {
		data_index = data_index + BMI3_LENGTH_FIFO_ACC;
	    }

	    rslt = check_data_index(data_index, fifo);

	    if (rslt == BMI3_OK)
	    {
		/* Unpack frame to get the gyro data */
		rslt = unpack_fifo_data_frame(gyro_data, &data_index, &gyro_index, data_enable, fifo);
	    }

	    if (rslt != BMI3_W_PARTIAL_READ)
	    {
		/* If sensor time enable with the gyro, increment the sensor time index by 2
		 * since sensor time frame has 2 bytes of data
		 */
		if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_SENS_TIME_FRM)
		{
		    data_index = data_index + BMI3_LENGTH_SENSOR_TIME;
		}

		/* If temperature enable with the gyro, increment the temperature index by 2
		 * since temperature frame has 2 bytes of data
		 */
		if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_TEMP_FRM)
		{
		    data_index = data_index + BMI3_LENGTH_TEMPERATURE;
		}
	    }
	}

	/* Update number of gyro frames to be read */
	(fifo->avail_fifo_gyro_frames) = gyro_index;

	if (fifo->avail_fifo_gyro_frames != 0)
	{
	    rslt = BMI3_OK;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets the FIFO water-mark level in the sensor.
 */
int8_t bmi3_set_fifo_wm(uint16_t fifo_wm, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data[2] = { 0 };

    /* Get LSB value of FIFO water-mark */
    data[0] = BMI3_GET_LSB(fifo_wm);

    /* Get MSB value of FIFO water-mark */
    data[1] = BMI3_GET_MSB(fifo_wm);

    /* Set the FIFO water-mark level */
    rslt = bmi3_set_regs(BMI3_REG_FIFO_WATERMARK, data, 2, dev);

    return rslt;
}

/*!
 * @brief This API reads the FIFO water-mark level set in the sensor.
 */
int8_t bmi3_get_fifo_wm(uint16_t *fifo_wm, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data[2] = { 0 };

    if (fifo_wm != NULL)
    {
	/* Read the FIFO water-mark level */
	rslt = bmi3_get_regs(BMI3_REG_FIFO_WATERMARK, data, BMI3_LENGTH_FIFO_WM, dev);

	if (rslt == BMI3_OK)
	{
	    (*fifo_wm) = (uint16_t)((data[0] | (uint16_t) data[1] << 8));
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets the FIFO configuration in the sensor.
 */
int8_t bmi3_set_fifo_config(uint16_t config, uint8_t enable, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array variable to store fifo config data */
    uint8_t data[2] = { 0 };

    /* Variable to store data of FIFO configuration register */
    uint16_t fifo_config = config & BMI3_FIFO_CONFIG_MASK;

    rslt = bmi3_get_regs(BMI3_REG_FIFO_CONF, data, 2, dev);

    if (rslt == BMI3_OK)
    {
	if (enable == BMI3_ENABLE)
	{
	    data[0] = data[0] | (uint8_t)fifo_config;
	    data[1] = data[1] | (uint8_t)(fifo_config >> 8);
        } else
	{
	    data[0] = data[0] & (~fifo_config);
	    data[1] = data[1] & (~(fifo_config >> 8));
	}

	rslt = bmi3_set_regs(BMI3_REG_FIFO_CONF, data, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This API reads the FIFO configuration from the sensor.
 */
int8_t bmi3_get_fifo_config(uint16_t *fifo_config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data[2] = { 0 };

    if (fifo_config != NULL)
    {
	/* Get the FIFO configuration value */
	rslt = bmi3_get_regs(BMI3_REG_FIFO_CONF, data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    (*fifo_config) = ((data[0] & BMI3_FIFO_CONFIG_MASK) | (((uint16_t)data[1] << 8) & BMI3_FIFO_CONFIG_MASK));
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets the length of FIFO data available in the sensor in
 * bytes.
 */
int8_t bmi3_get_fifo_length(uint16_t *fifo_avail_len, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define byte index */
    uint16_t reg_data;

    /* Array to store FIFO data length */
    uint8_t data[BMI3_LENGTH_FIFO_DATA] = { 0 };

    if (fifo_avail_len != NULL)
    {
	/* Read FIFO length */
	rslt = bmi3_get_regs(BMI3_REG_FIFO_FILL_LEVEL, data, BMI3_LENGTH_FIFO_DATA, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the MSB byte of FIFO length */
	    data[0] = BMI3_GET_BIT_POS0(data[0], BMI3_FIFO_FILL_LEVEL);

	    reg_data = ((uint16_t)data[1] << 8);

	    reg_data = BMI3_GET_BIT_POS0(reg_data, BMI3_FIFO_FILL_LEVEL);

	    /* Get total FIFO length */
	    *fifo_avail_len = (uint16_t)(reg_data | data[0]);

	    if (*fifo_avail_len == 0)
	    {
		rslt = BMI3_W_FIFO_EMPTY;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API is used to perform the self-test for either accel or gyro or both.
 */
int8_t bmi3_perform_self_test(uint8_t st_selection, struct bmi3_st_result *st_result_status, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint16_t reg_data[9];

    uint8_t data_array[18] = { 0 };

    /* Variable to store gyro filter coefficient base address */
    uint8_t gyro_filter_coeff_base_addr[2] = { BMI3_BASE_ADDR_GYRO_SC_ST_COEFFICIENTS, 0 };

    if (st_result_status != NULL)
    {
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, gyro_filter_coeff_base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, data_array, 18, dev);

	    if (rslt == BMI3_OK)
	    {
		rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, gyro_filter_coeff_base_addr, 2, dev);

		if (rslt == BMI3_OK)
		{
		    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, data_array, 18, dev);
		}
	    }
	}

	reg_data[0] = (uint16_t)(data_array[0] | (uint16_t)data_array[1] << 8);
	reg_data[1] = (uint16_t)(data_array[2] | (uint16_t)data_array[3] << 8);
	reg_data[2] = (uint16_t)(data_array[4] | (uint16_t)data_array[5] << 8);
	reg_data[3] = (uint16_t)(data_array[6] | (uint16_t)data_array[7] << 8);
	reg_data[4] = (uint16_t)(data_array[8] | (uint16_t)data_array[9] << 8);
	reg_data[5] = (uint16_t)(data_array[10] | (uint16_t)data_array[11] << 8);
	reg_data[6] = (uint16_t)(data_array[12] | (uint16_t)data_array[13] << 8);
	reg_data[7] = (uint16_t)(data_array[14] | (uint16_t)data_array[15] << 8);
	reg_data[8] = (uint16_t)(data_array[16] | (uint16_t)data_array[17] << 8);

	if ((reg_data[3] != BMI3_SC_ST_VALUE_3) &&
	    ((reg_data[0] != BMI3_SC_ST_VALUE_0) || (reg_data[1] != BMI3_SC_ST_VALUE_1) ||
	     (reg_data[2] != BMI3_SC_ST_VALUE_2) || (reg_data[4] != BMI3_SC_ST_VALUE_4) ||
	     (reg_data[5] != BMI3_SC_ST_VALUE_5) || (reg_data[6] != BMI3_SC_ST_VALUE_6) ||
	     (reg_data[7] != BMI3_SC_ST_VALUE_7) || (reg_data[8] != BMI3_SC_ST_VALUE_8)))
	{
	    rslt = set_gyro_filter_coefficients(dev);
	}

	if (rslt == BMI3_OK)
	{
	    /* Get and set the self-test mode given by the user in the self-test dma register. */
	    rslt = get_set_st_dma(st_selection, dev);
	}

	if (rslt == BMI3_OK)
	{
	    /* Sets the self-test preconditions and triggers the self-test in the
	     * command register. */
	    rslt = self_test_conditions(st_selection, dev);

	    if (rslt == BMI3_OK)
	    {
		rslt = get_st_status_rslt(st_selection, st_result_status, dev);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API writes the config array and config version in cfg res.
 */
int8_t bmi3_configure_enhanced_flexibility(struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if (rslt == BMI3_OK)
    {
	/* Bytes written are multiples of 2 */
	if ((dev->read_write_len % 2) != 0)
	{
	    dev->read_write_len = dev->read_write_len - 1;
	}

	/* BMI3 has 16 bit address and hence the minimum read write length should be 2 bytes */
	if (dev->read_write_len < 2)
	{
	    dev->read_write_len = 2;
	}

	rslt = config_array_set_command(dev);

	if (rslt == BMI3_OK)
	{
	    rslt = config_array_set_value_one_page(dev);

	    if (rslt == BMI3_OK)
	    {
		/* Write the config array */
		rslt = write_config_array(dev);
	    }
	}
    }

    return rslt;
}

/*!
 * @brief This API is used to get the config version.
 */
int8_t bmi3_get_config_version(struct bmi3_config_version *config_version, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_OK;

    /* Array to store base address of config */
    uint8_t data[2] = { BMI3_BASE_ADDR_CONFIG_VERSION, 0 };

    /* Variable to store version number */
    uint8_t version[6];

    uint16_t lsb_msb;

    if (config_version != NULL)
    {
	/* Set the config version base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, version, 6, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate config major and minor version from same word */
		lsb_msb = (uint16_t)(version[0] | ((uint16_t) version[1] << 8));

		config_version->config1_minor_version = (uint8_t)(lsb_msb & BMI3_CONFIG_1_MINOR_MASK);

		config_version->config1_major_version = (lsb_msb & BMI3_CONFIG_1_MAJOR_MASK) >> BMI3_CONFIG_POS;

		/* Get word to calculate config major and minor version from same word */
		lsb_msb = (uint16_t)(version[2] | ((uint16_t) version[3] << 8));

		config_version->config2_minor_version = (uint8_t)(lsb_msb & BMI3_CONFIG_2_MINOR_MASK);

		config_version->config2_major_version = (lsb_msb & BMI3_CONFIG_2_MAJOR_MASK) >> BMI3_CONFIG_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API is used to perform the self-calibration for either sensitivity or offset or both.
 */
int8_t bmi3_perform_gyro_sc(uint8_t sc_selection,
			    uint8_t apply_corr,
			    struct bmi3_self_calib_rslt *sc_rslt,
			    struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Structure to store sensor configuration */
    struct bmi3_sens_config config;

    if (sc_rslt != NULL)
    {
	/* Enable accelerometer */
	config.type = BMI3_ACCEL;

	/* Definition of accel configuration which are the preconditions for self-calibration */
	config.cfg.acc.acc_mode = BMI3_ACC_MODE_HIGH_PERF;
	config.cfg.acc.odr = BMI3_ACC_ODR_100HZ;
	config.cfg.acc.range = BMI3_ACC_RANGE_8G;

	/* Write the sensor configurations */
	rslt = bmi3_set_sensor_config(&config, 1, dev);

	if (rslt == BMI3_OK)
	{
	    /* Disable alternate accel and gyro mode */
	    rslt = disable_alt_conf_acc_gyr_mode(dev);
	}

	if (rslt == BMI3_OK)
	{
	    /* Get and set the self-calibration mode given by the user in the self-calibration dma register. */
	    rslt = get_set_sc_dma(sc_selection, apply_corr, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Trigger the self-calibration command */
		rslt = bmi3_set_command_register(BMI3_CMD_SELF_CALIB_TRIGGER, dev);

		if (rslt == BMI3_OK)
		{
		    /* Get the self-calibration status and result */
		    rslt = get_sc_gyro_rslt(sc_rslt, dev);
		}
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API is used to set the data sample rate for i3c sync
 */
int8_t bmi3_set_i3c_tc_sync_tph(uint16_t sample_rate, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint16_t reg_data;

    /* To get the second byte data in word calculation */
    uint16_t tc_tph;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    /* Get the data sample rate of i3c sync */
    rslt = bmi3_get_regs(BMI3_REG_I3C_TC_SYNC_TPH, data_array, 2, dev);

    if (rslt == BMI3_OK)
    {
	data_array[0] = (uint8_t)BMI3_SET_BIT_POS0(data_array[0], BMI3_I3C_TC_SYNC_TPH, sample_rate);

	reg_data = ((uint16_t)data_array[1] << 8);

	tc_tph = (uint8_t)BMI3_SET_BIT_POS0(reg_data, BMI3_I3C_TC_SYNC_TPH, sample_rate);

	data_array[1] = (uint8_t)(tc_tph >> 8);

	rslt = bmi3_set_regs(BMI3_REG_I3C_TC_SYNC_TPH, data_array, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This API is used to get the data sample rate for i3c sync
 */
int8_t bmi3_get_i3c_tc_sync_tph(uint16_t *sample_rate, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    if (sample_rate != NULL)
    {
	/* Get the data sample rate of i3c sync */
	rslt = bmi3_get_regs(BMI3_REG_I3C_TC_SYNC_TPH, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get i3c sync tph sample rate */
	    *sample_rate = (uint16_t)(data_array[0] | (uint16_t)data_array[1] << 8);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API is used set the TU(time unit) value is used to scale the delay time payload
 * according to the hosts needs
 */
int8_t bmi3_set_i3c_tc_sync_tu(uint8_t delay_time, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define data array */
    uint8_t data_array[2] = { 0 };

    /* Get the i3c sync time unit */
    rslt = bmi3_get_regs(BMI3_REG_I3C_TC_SYNC_TU, data_array, 2, dev);

    if (rslt == BMI3_OK)
    {
	/* Set the i3c sync time unit  */
	data_array[0] = BMI3_SET_BIT_POS0(data_array[0], BMI3_I3C_TC_SYNC_TU, delay_time);

	rslt = bmi3_set_regs(BMI3_REG_I3C_TC_SYNC_TU, data_array, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This API is used get the TU(time unit) value is used to scale the delay time payload
 * according to the hosts needs
 */
int8_t bmi3_get_i3c_tc_sync_tu(uint8_t *delay_time, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define data array */
    uint8_t data_array[2] = { 0 };

    if (delay_time != NULL)
    {
	/* Get the i3c sync time unit */
	rslt = bmi3_get_regs(BMI3_REG_I3C_TC_SYNC_TU, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    *delay_time = BMI3_GET_BIT_POS0(data_array[0], BMI3_I3C_TC_SYNC_TU);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API is used to set the i3c sync ODR.
 */
int8_t bmi3_set_i3c_tc_sync_odr(uint8_t odr, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define data array */
    uint8_t data_array[2] = { 0 };

    /* Get the i3c sync ODR */
    rslt = bmi3_get_regs(BMI3_REG_I3C_TC_SYNC_ODR, data_array, 2, dev);

    if (rslt == BMI3_OK)
    {
	/* Set the i3c sync ODR */
	data_array[0] = BMI3_SET_BIT_POS0(data_array[0], BMI3_I3C_TC_SYNC_ODR, odr);

	rslt = bmi3_set_regs(BMI3_REG_I3C_TC_SYNC_ODR, data_array, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This API is used to get the i3c sync ODR.
 */
int8_t bmi3_get_i3c_tc_sync_odr(uint8_t *odr, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    if (odr != NULL)
    {
	/* Get the i3c sync ODR */
	rslt = bmi3_get_regs(BMI3_REG_I3C_TC_SYNC_ODR, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    *odr = BMI3_GET_BIT_POS0(data_array[0], BMI3_I3C_TC_SYNC_ODR);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets i3c sync i3c_tc_res
 */
int8_t bmi3_get_i3c_sync_i3c_tc_res(uint8_t *i3c_tc_res, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t i3c_sync_i3c_tc_res[2] = { 0 };

    /* Array to set the base address of i3c sync feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_I3C_SYNC, 0 };

    /* Variable to define array offset */
    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    if (i3c_tc_res != NULL)
    {
	/* Set the tap base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where tap feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, i3c_sync_i3c_tc_res, 2, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate i3c_tc_res from the same word */
		lsb = (uint16_t) i3c_sync_i3c_tc_res[idx++];
		msb = ((uint16_t) i3c_sync_i3c_tc_res[idx++] << 8);
		lsb_msb = lsb | msb;

		/* Get i3c_tc_res */
		*i3c_tc_res = (uint8_t)(lsb_msb & BMI3_I3C_SYNC_FILTER_EN_MASK);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets i3c sync i3c_tc_res
 */
int8_t bmi3_set_i3c_sync_i3c_tc_res(uint8_t i3c_tc_res, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t i3c_sync_i3c_tc_res[2] = { 0 };

    /* Array to set the base address of i3c sync feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_I3C_SYNC, 0 };

    /* Set the tap base address to feature engine transmission address to start DMA transaction */
    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

    if (rslt == BMI3_OK)
    {
	/* Set i3c_tc_res */
	i3c_sync_i3c_tc_res[0] = BMI3_SET_BIT_POS0(i3c_sync_i3c_tc_res[0], BMI3_I3C_SYNC_FILTER_EN, i3c_tc_res);

	/* Left shift by 8 times so that we can set rest of the values of i3c_tc_res data in word */
	i3c_sync_i3c_tc_res[1] = (uint8_t)(i3c_sync_i3c_tc_res[1] << 8);

	/* Set the configuration back to the feature engine register */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, i3c_sync_i3c_tc_res, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This API is used to enable accel and gyro for alternate configuration
 */
int8_t bmi3_alternate_config_ctrl(uint8_t config_en, uint8_t alt_rst_conf, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t data[2] = { 0 };

    rslt = bmi3_get_regs(BMI3_REG_ALT_CONF, data, 2, dev);

    if (rslt == BMI3_OK)
    {
	data[0] = config_en;
	data[1] = alt_rst_conf;

	rslt = bmi3_set_regs(BMI3_REG_ALT_CONF, data, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This API is used to read the status of alternate configuration
 */
int8_t bmi3_read_alternate_status(struct bmi3_alt_status *alt_status, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t data[2] = { 0 };

    if (alt_status != NULL)
    {
	rslt = bmi3_get_regs(BMI3_REG_ALT_STATUS, data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    alt_status->alt_accel_status = (data[0] & BMI3_ALT_ACCEL_STATUS_MASK);
	    alt_status->alt_gyro_status = (data[0] & BMI3_ALT_GYRO_STATUS_MASK) >> BMI3_ALT_GYRO_STATUS_POS;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets offset dgain for the sensor which stores self-calibrated values for accel.
 */
int8_t bmi3_get_acc_dp_off_dgain(struct bmi3_acc_dp_gain_offset *acc_dp_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t reg_data[12] = { 0 };

    /* NULL pointer check */
    if (acc_dp_gain_offset != NULL)
    {
	/* Get temperature user offset for accel */
	rslt = bmi3_get_regs(BMI3_REG_ACC_DP_OFF_X, reg_data, 12, dev);

	if (rslt == BMI3_OK)
	{
	    acc_dp_gain_offset->acc_dp_off_x = (uint16_t)(((uint16_t)reg_data[1] << 8) | reg_data[0]);
	    acc_dp_gain_offset->acc_dp_gain_x = reg_data[2];
	    acc_dp_gain_offset->acc_dp_off_y = (uint16_t)(((uint16_t)reg_data[5] << 8) | reg_data[4]);
	    acc_dp_gain_offset->acc_dp_gain_y = reg_data[6];
	    acc_dp_gain_offset->acc_dp_off_z = (uint16_t)(((uint16_t)reg_data[9] << 8) | reg_data[8]);
	    acc_dp_gain_offset->acc_dp_gain_z = reg_data[10];
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets offset dgain for the sensor which stores self-calibrated values for gyro.
 */
int8_t bmi3_get_gyro_dp_off_dgain(struct bmi3_gyr_dp_gain_offset *gyr_dp_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t reg_data[12];

    /* NULL pointer check */
    if (gyr_dp_gain_offset != NULL)
    {
	/* Get temperature user offset for gyro */
	rslt = bmi3_get_regs(BMI3_REG_GYR_DP_OFF_X, reg_data, 12, dev);

	if (rslt == BMI3_OK)
	{
	    gyr_dp_gain_offset->gyr_dp_off_x = (uint16_t)(((uint16_t)reg_data[1] << 8) | reg_data[0]);
	    gyr_dp_gain_offset->gyr_dp_gain_x = reg_data[2];
	    gyr_dp_gain_offset->gyr_dp_off_y = (uint16_t)(((uint16_t)reg_data[5] << 8) | reg_data[4]);
	    gyr_dp_gain_offset->gyr_dp_gain_y = reg_data[6];
	    gyr_dp_gain_offset->gyr_dp_off_z = (uint16_t)(((uint16_t)reg_data[9] << 8) | reg_data[8]);
	    gyr_dp_gain_offset->gyr_dp_gain_z = reg_data[10];
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets offset dgain for the sensor which stores self-calibrated values for accel.
 */
int8_t bmi3_set_acc_dp_off_dgain(const struct bmi3_acc_dp_gain_offset *acc_dp_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t acc_dp_gain_x, acc_dp_gain_y, acc_dp_gain_z;

    uint16_t acc_dp_off_x1, acc_dp_off_y1, acc_dp_off_z1;

    uint16_t acc_dp_off_x2, acc_dp_off_y2, acc_dp_off_z2;

    uint16_t acc_dp_off_x, acc_dp_off_y, acc_dp_off_z;

    uint8_t reg_data[12] = { 0 };

    uint8_t acc_off_gain[12] = { 0 };

    /* NULL pointer check */
    if (acc_dp_gain_offset != NULL)
    {
	acc_dp_off_x1 = BMI3_SET_BIT_POS0(reg_data[0], BMI3_ACC_DP_OFF_X, acc_dp_gain_offset->acc_dp_off_x);

	acc_dp_off_x = ((uint16_t)reg_data[1] << 8);

	acc_dp_off_x2 = BMI3_SET_BIT_POS0(acc_dp_off_x, BMI3_ACC_DP_OFF_X, acc_dp_gain_offset->acc_dp_off_x);

	acc_dp_gain_x = BMI3_SET_BIT_POS0(reg_data[2], BMI3_ACC_DP_DGAIN_X, acc_dp_gain_offset->acc_dp_gain_x);

	acc_dp_off_y1 = BMI3_SET_BIT_POS0(reg_data[4], BMI3_ACC_DP_OFF_Y, acc_dp_gain_offset->acc_dp_off_y);

	acc_dp_off_y = ((uint16_t)reg_data[5] << 8);

	acc_dp_off_y2 = BMI3_SET_BIT_POS0(acc_dp_off_y, BMI3_ACC_DP_OFF_Y, acc_dp_gain_offset->acc_dp_off_y);

	acc_dp_gain_y = BMI3_SET_BIT_POS0(reg_data[6], BMI3_ACC_DP_DGAIN_Y, acc_dp_gain_offset->acc_dp_gain_y);

	acc_dp_off_z1 = BMI3_SET_BIT_POS0(reg_data[8], BMI3_ACC_DP_OFF_Z, acc_dp_gain_offset->acc_dp_off_z);

	acc_dp_off_z = ((uint16_t)reg_data[9] << 8);

	acc_dp_off_z2 = BMI3_SET_BIT_POS0(acc_dp_off_z, BMI3_ACC_DP_OFF_Z, acc_dp_gain_offset->acc_dp_off_z);

	acc_dp_gain_z = BMI3_SET_BIT_POS0(reg_data[10], BMI3_ACC_DP_DGAIN_Z, acc_dp_gain_offset->acc_dp_gain_z);

	acc_off_gain[0] = (uint8_t)acc_dp_off_x1;
	acc_off_gain[1] = acc_dp_off_x2 >> 8;
	acc_off_gain[2] = acc_dp_gain_x;
	acc_off_gain[4] = (uint8_t)acc_dp_off_y1;
	acc_off_gain[5] = acc_dp_off_y2 >> 8;
	acc_off_gain[6] = acc_dp_gain_y;
	acc_off_gain[8] = (uint8_t)acc_dp_off_z1;
	acc_off_gain[9] = acc_dp_off_z2 >> 8;
	acc_off_gain[10] = acc_dp_gain_z;

	/* Set dp offset for accel */
	rslt = bmi3_set_regs(BMI3_REG_ACC_DP_OFF_X, acc_off_gain, 12, dev);
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets offset dgain for the sensor which stores self-calibrated values for gyro.
 */
int8_t bmi3_set_gyro_dp_off_dgain(const struct bmi3_gyr_dp_gain_offset *gyr_dp_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t gyr_dp_gain_x, gyr_dp_gain_y, gyr_dp_gain_z;

    uint16_t gyr_dp_off_x1, gyr_dp_off_y1, gyr_dp_off_z1;

    uint16_t gyr_dp_off_x2, gyr_dp_off_y2, gyr_dp_off_z2;

    uint16_t gyr_dp_off_x, gyr_dp_off_y, gyr_dp_off_z;

    uint8_t reg_data[12] = { 0 };

    uint8_t gyr_off_gain[12] = { 0 };

    /* NULL pointer check */
    if (gyr_dp_gain_offset != NULL)
    {
	gyr_dp_off_x1 = BMI3_SET_BIT_POS0(reg_data[0], BMI3_GYR_DP_OFF_X, gyr_dp_gain_offset->gyr_dp_off_x);

	gyr_dp_off_x = ((uint16_t)reg_data[1] << 8);

	gyr_dp_off_x2 = BMI3_SET_BIT_POS0(gyr_dp_off_x, BMI3_GYR_DP_OFF_X, gyr_dp_gain_offset->gyr_dp_off_x);

	gyr_dp_gain_x = BMI3_SET_BIT_POS0(reg_data[2], BMI3_GYR_DP_DGAIN_X, gyr_dp_gain_offset->gyr_dp_gain_x);

	gyr_dp_off_y1 = BMI3_SET_BIT_POS0(reg_data[4], BMI3_GYR_DP_OFF_Y, gyr_dp_gain_offset->gyr_dp_off_y);

	gyr_dp_off_y = ((uint16_t)reg_data[5] << 8);

	gyr_dp_off_y2 = BMI3_SET_BIT_POS0(gyr_dp_off_y, BMI3_GYR_DP_OFF_Y, gyr_dp_gain_offset->gyr_dp_off_y);

	gyr_dp_gain_y = BMI3_SET_BIT_POS0(reg_data[6], BMI3_GYR_DP_DGAIN_Y, gyr_dp_gain_offset->gyr_dp_gain_y);

	gyr_dp_off_z1 = BMI3_SET_BIT_POS0(reg_data[8], BMI3_GYR_DP_OFF_Z, gyr_dp_gain_offset->gyr_dp_off_z);

	gyr_dp_off_z = ((uint16_t)reg_data[9] << 8);

	gyr_dp_off_z2 = BMI3_SET_BIT_POS0(gyr_dp_off_z, BMI3_GYR_DP_OFF_Z, gyr_dp_gain_offset->gyr_dp_off_z);

	gyr_dp_gain_z = BMI3_SET_BIT_POS0(reg_data[10], BMI3_GYR_DP_DGAIN_Z, gyr_dp_gain_offset->gyr_dp_gain_z);

	gyr_off_gain[0] = (uint8_t)gyr_dp_off_x1;
	gyr_off_gain[1] = gyr_dp_off_x2 >> 8;
	gyr_off_gain[2] = gyr_dp_gain_x;
	gyr_off_gain[4] = (uint8_t)gyr_dp_off_y1;
	gyr_off_gain[5] = gyr_dp_off_y2 >> 8;
	gyr_off_gain[6] = gyr_dp_gain_y;
	gyr_off_gain[8] = (uint8_t)gyr_dp_off_z1;
	gyr_off_gain[9] = gyr_dp_off_z2 >> 8;
	gyr_off_gain[10] = gyr_dp_gain_z;

	/* Set dp offset for gyro */
	rslt = bmi3_set_regs(BMI3_REG_GYR_DP_OFF_X, gyr_off_gain, 12, dev);
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets user offset dgain for the sensor which stores self-calibrated values for accel.
 */
int8_t bmi3_get_user_acc_off_dgain(struct bmi3_acc_usr_gain_offset *acc_usr_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t reg_data[12];

    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ACC_OFFSET_GAIN, 0 };

    /* NULL pointer check */
    if (acc_usr_gain_offset != NULL)
    {
	/* Set the user accel offset base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, 12, dev);

	    if (rslt == BMI3_OK)
	    {
		acc_usr_gain_offset->acc_usr_off_x = (uint16_t)(((uint16_t)reg_data[1] << 8) | reg_data[0]);
		acc_usr_gain_offset->acc_usr_off_y = (uint16_t)(((uint16_t)reg_data[3] << 8) | reg_data[2]);
		acc_usr_gain_offset->acc_usr_off_z = (uint16_t)(((uint16_t)reg_data[5] << 8) | reg_data[4]);
		acc_usr_gain_offset->acc_usr_gain_x = reg_data[6];
		acc_usr_gain_offset->acc_usr_gain_y = reg_data[8];
		acc_usr_gain_offset->acc_usr_gain_z = reg_data[10];
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets user offset dgain for the sensor which stores self-calibrated values for gyro.
 */
int8_t bmi3_get_user_gyro_off_dgain(struct bmi3_gyr_usr_gain_offset *gyr_usr_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t reg_data[12];

    uint8_t base_addr[2] = { BMI3_BASE_ADDR_GYRO_OFFSET_GAIN, 0 };

    /* NULL pointer check */
    if (gyr_usr_gain_offset != NULL)
    {
	/* Set the user gyro offset base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, 12, dev);

	    if (rslt == BMI3_OK)
	    {
		gyr_usr_gain_offset->gyr_usr_off_x = (uint16_t)(((uint16_t)reg_data[1] << 8) | reg_data[0]);
		gyr_usr_gain_offset->gyr_usr_off_y = (uint16_t)(((uint16_t)reg_data[3] << 8) | reg_data[2]);
		gyr_usr_gain_offset->gyr_usr_off_z = (uint16_t)(((uint16_t)reg_data[5] << 8) | reg_data[4]);
		gyr_usr_gain_offset->gyr_usr_gain_x = reg_data[6];
		gyr_usr_gain_offset->gyr_usr_gain_y = reg_data[8];
		gyr_usr_gain_offset->gyr_usr_gain_z = reg_data[10];
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets user offset dgain for the sensor which stores self-calibrated values for accel.
 */
int8_t bmi3_set_user_acc_off_dgain(const struct bmi3_acc_usr_gain_offset *acc_usr_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t acc_usr_gain_x, acc_usr_gain_y, acc_usr_gain_z;

    uint16_t acc_usr_off_x1, acc_usr_off_y1, acc_usr_off_z1;

    uint16_t acc_usr_off_x2, acc_usr_off_y2, acc_usr_off_z2;

    uint16_t acc_usr_off_x, acc_usr_off_y, acc_usr_off_z;

    uint8_t reg_data[12] = { 0 };

    uint8_t user_acc_off_gain[12] = { 0 };

    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ACC_OFFSET_GAIN, 0 };

    /* NULL pointer check */
    if (acc_usr_gain_offset != NULL)
    {
	/* Set the user accel offset base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    acc_usr_off_x1 = BMI3_SET_BIT_POS0(reg_data[0], BMI3_ACC_USR_OFF_X, acc_usr_gain_offset->acc_usr_off_x);

	    acc_usr_off_x = ((uint16_t)reg_data[1] << 8);

	    acc_usr_off_x2 = BMI3_SET_BIT_POS0(acc_usr_off_x, BMI3_ACC_USR_OFF_X, acc_usr_gain_offset->acc_usr_off_x);

	    acc_usr_off_y1 = BMI3_SET_BIT_POS0(reg_data[2], BMI3_ACC_USR_OFF_Y, acc_usr_gain_offset->acc_usr_off_y);

	    acc_usr_off_y = ((uint16_t)reg_data[3] << 8);

	    acc_usr_off_y2 = BMI3_SET_BIT_POS0(acc_usr_off_y, BMI3_ACC_USR_OFF_Y, acc_usr_gain_offset->acc_usr_off_y);

	    acc_usr_off_z1 = BMI3_SET_BIT_POS0(reg_data[4], BMI3_ACC_USR_OFF_Z, acc_usr_gain_offset->acc_usr_off_z);

	    acc_usr_off_z = ((uint16_t)reg_data[5] << 8);

	    acc_usr_off_z2 = BMI3_SET_BIT_POS0(acc_usr_off_z, BMI3_ACC_USR_OFF_Z, acc_usr_gain_offset->acc_usr_off_z);

	    acc_usr_gain_x = BMI3_SET_BIT_POS0(reg_data[6], BMI3_ACC_USR_GAIN_X, acc_usr_gain_offset->acc_usr_gain_x);

	    acc_usr_gain_y = BMI3_SET_BIT_POS0(reg_data[8], BMI3_ACC_USR_GAIN_Y, acc_usr_gain_offset->acc_usr_gain_y);

	    acc_usr_gain_z = BMI3_SET_BIT_POS0(reg_data[10], BMI3_ACC_USR_GAIN_Z, acc_usr_gain_offset->acc_usr_gain_z);

	    user_acc_off_gain[0] = (uint8_t)acc_usr_off_x1;
	    user_acc_off_gain[1] = acc_usr_off_x2 >> 8;
	    user_acc_off_gain[2] = (uint8_t)acc_usr_off_y1;
	    user_acc_off_gain[3] = acc_usr_off_y2 >> 8;
	    user_acc_off_gain[4] = (uint8_t)acc_usr_off_z1;
	    user_acc_off_gain[5] = acc_usr_off_z2 >> 8;
	    user_acc_off_gain[6] = acc_usr_gain_x;
	    user_acc_off_gain[8] = acc_usr_gain_y;
	    user_acc_off_gain[10] = acc_usr_gain_z;

	    /* Set the configuration to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, user_acc_off_gain, 12, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets user offset dgain for the sensor which stores self-calibrated values for gyro.
 */
int8_t bmi3_set_user_gyr_off_dgain(const struct bmi3_gyr_usr_gain_offset *gyr_usr_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t gyr_usr_gain_x, gyr_usr_gain_y, gyr_usr_gain_z;

    uint16_t gyr_usr_off_x1, gyr_usr_off_y1, gyr_usr_off_z1;

    uint16_t gyr_usr_off_x2, gyr_usr_off_y2, gyr_usr_off_z2;

    uint16_t gyr_usr_off_x, gyr_usr_off_y, gyr_usr_off_z;

    uint8_t reg_data[12] = { 0 };

    uint8_t user_gyr_off_gain[12];

    uint8_t base_addr[2] = { BMI3_BASE_ADDR_GYRO_OFFSET_GAIN, 0 };

    /* NULL pointer check */
    if (gyr_usr_gain_offset != NULL)
    {
	/* Set the user gyro offset base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    gyr_usr_off_x1 = BMI3_SET_BIT_POS0(reg_data[0], BMI3_GYR_USR_OFF_X, gyr_usr_gain_offset->gyr_usr_off_x);

	    gyr_usr_off_x = ((uint16_t)reg_data[1] << 8);

	    gyr_usr_off_x2 = BMI3_SET_BIT_POS0(gyr_usr_off_x, BMI3_GYR_USR_OFF_X, gyr_usr_gain_offset->gyr_usr_off_x);

	    gyr_usr_off_y1 = BMI3_SET_BIT_POS0(reg_data[2], BMI3_GYR_USR_OFF_Y, gyr_usr_gain_offset->gyr_usr_off_y);

	    gyr_usr_off_y = ((uint16_t)reg_data[3] << 8);

	    gyr_usr_off_y2 = BMI3_SET_BIT_POS0(gyr_usr_off_y, BMI3_GYR_USR_OFF_Y, gyr_usr_gain_offset->gyr_usr_off_y);

	    gyr_usr_off_z1 = BMI3_SET_BIT_POS0(reg_data[4], BMI3_GYR_USR_OFF_Z, gyr_usr_gain_offset->gyr_usr_off_z);

	    gyr_usr_off_z = ((uint16_t)reg_data[5] << 8);

	    gyr_usr_off_z2 = BMI3_SET_BIT_POS0(gyr_usr_off_z, BMI3_GYR_USR_OFF_Z, gyr_usr_gain_offset->gyr_usr_off_z);

	    gyr_usr_gain_x = BMI3_SET_BIT_POS0(reg_data[6], BMI3_GYR_USR_GAIN_X, gyr_usr_gain_offset->gyr_usr_gain_x);

	    gyr_usr_gain_y = BMI3_SET_BIT_POS0(reg_data[8], BMI3_GYR_USR_GAIN_Y, gyr_usr_gain_offset->gyr_usr_gain_y);

	    gyr_usr_gain_z = BMI3_SET_BIT_POS0(reg_data[10], BMI3_GYR_USR_GAIN_Z, gyr_usr_gain_offset->gyr_usr_gain_z);

	    user_gyr_off_gain[0] = (uint8_t)gyr_usr_off_x1;
	    user_gyr_off_gain[1] = gyr_usr_off_x2 >> 8;
	    user_gyr_off_gain[2] = (uint8_t)gyr_usr_off_y1;
	    user_gyr_off_gain[3] = gyr_usr_off_y2 >> 8;
	    user_gyr_off_gain[4] = (uint8_t)gyr_usr_off_z1;
	    user_gyr_off_gain[5] = gyr_usr_off_z2 >> 8;
	    user_gyr_off_gain[6] = gyr_usr_gain_x;
	    user_gyr_off_gain[8] = gyr_usr_gain_y;
	    user_gyr_off_gain[10] = gyr_usr_gain_z;

	    /* Set the configuration to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, user_gyr_off_gain, 12, dev);

	    if (rslt == BMI3_OK)
	    {
		rslt = bmi3_set_command_register(BMI3_CMD_USR_GAIN_OFFS_UPDATE, dev);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API performs Fast Offset Compensation for accelerometer.
 */
int8_t bmi3_perform_accel_foc(const struct bmi3_accel_foc_g_value *accel_g_value, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Structure to define the accelerometer configurations */
    struct bmi3_accel_config acc_cfg = { 0 };
    struct bmi3_sens_config config = { 0 };

    /* Configure the type */
    config.type = BMI3_ACCEL;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if ((rslt == BMI3_OK) && (accel_g_value != NULL))
    {
	/* Check for input validity */
	if ((((BMI3_ABS(accel_g_value->x)) + (BMI3_ABS(accel_g_value->y)) + (BMI3_ABS(accel_g_value->z))) == 1) &&
	    ((accel_g_value->sign == 1) || (accel_g_value->sign == 0)))
	{
	    rslt = verify_foc_position(BMI3_ACCEL, accel_g_value, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get accelerometer configurations */
		rslt = bmi3_get_sensor_config(&config, 1, dev);
	    }

	    /* Set configurations for FOC */
	    if (rslt == BMI3_OK)
	    {
		rslt = set_accel_foc_config(dev);
	    }

	    /* Perform accelerometer FOC */
	    if (rslt == BMI3_OK)
	    {
		rslt = perform_accel_foc(accel_g_value, &acc_cfg, dev);
	    }

	    /* Set the configurations */
	    if (rslt == BMI3_OK)
	    {
		rslt = bmi3_set_sensor_config(&config, 1, dev);
	    }
        } else
	{
	    rslt = BMI3_E_INVALID_INPUT;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets the data ready status of power on reset, accelerometer, gyroscope
 * and temperature.
 */
int8_t bmi3_get_sensor_status(uint16_t *status, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t data[2] = { 0 };

    if (status != NULL)
    {
	/* Read the status register */
	rslt = bmi3_get_regs(BMI3_REG_STATUS, data, 2, dev);

	*status = (uint16_t)(data[0] | ((uint16_t)data[1] << 8));
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets the I3C IBI status of both feature and data
 * interrupts
 */
int8_t bmi3_get_i3c_ibi_status(uint16_t *int_status, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    if (int_status != NULL)
    {
	/* Get the interrupt status */
	rslt = bmi3_get_regs(BMI3_REG_INT_STATUS_IBI, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    *int_status = (uint16_t) data_array[0] | ((uint16_t) data_array[1] << 8);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets accel gyro offset gain reset values.
 */
int8_t bmi3_get_acc_gyr_off_gain_reset(uint8_t *acc_off_gain_reset, uint8_t *gyr_off_gain_reset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the accel gyro user gain offset reset values */
    uint8_t acc_gyr_off_gain_reset[2] = { 0 };

    /* Array to set the base address of accel gyro offset and gain reset */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ACC_GYR_OFFSET_GAIN_RESET, 0 };

    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    if ((acc_off_gain_reset != NULL) && (gyr_off_gain_reset != NULL))
    {
	/* Set the accel gyro offset and gain reset base address to feature engine transmission address to start DMA
	 * transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where accel gyro offset and gain reset value
	     * resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, acc_gyr_off_gain_reset, 2, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate acc_off_gain_reset and gyr_off_gain_reset */
		lsb = (uint16_t) acc_gyr_off_gain_reset[idx++];
		msb = ((uint16_t) acc_gyr_off_gain_reset[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get accel offset gain reset */
		*acc_off_gain_reset = (uint8_t)(lsb_msb & BMI3_ACC_OFF_GAIN_RESET_MASK);

		/* Get gyro offset gain reset */
		*gyr_off_gain_reset = (uint8_t)(lsb_msb & BMI3_GYR_OFF_GAIN_RESET_MASK) >> BMI3_GYR_OFF_GAIN_RESET_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets accel gyro offset gain reset values.
 */
int8_t bmi3_set_acc_gyr_off_gain_reset(uint8_t acc_off_gain_reset, uint8_t gyr_off_gain_reset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the accel gyro user gain offset reset values */
    uint8_t acc_gyr_off_gain_reset[2] = { 0 };

    /* Array to set the base address of accel gyro offset and gain reset */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ACC_GYR_OFFSET_GAIN_RESET, 0 };

    uint8_t data_array[2] = { 0 };

    /* Set the accel gyro offset and gain reset base address to feature engine transmission address to start DMA
     * transaction */
    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

    if (rslt == BMI3_OK)
    {
	/* Set accel offset gain reset */
	acc_off_gain_reset = BMI3_SET_BIT_POS0(data_array[0], BMI3_ACC_OFF_GAIN_RESET, acc_off_gain_reset);

	/* Set gyro offset gain reset */
	gyr_off_gain_reset = BMI3_SET_BITS(data_array[0], BMI3_GYR_OFF_GAIN_RESET, gyr_off_gain_reset);

	acc_gyr_off_gain_reset[0] = (acc_off_gain_reset | gyr_off_gain_reset);

	/* Set the configurations back to the feature engine register */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, acc_gyr_off_gain_reset, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This API sets accel offset and gain values.
 */
int8_t bmi3_set_acc_usr_off_gain(const struct bmi3_acc_usr_gain_offset *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the accel user gain offset values */
    uint8_t acc_usr_gain_offset[12] = { 0 };

    /* Array to set the base address of accel offset and gain */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ACC_OFFSET_GAIN, 0 };

    uint8_t data_array[12] = { 0 };

    uint16_t acc_usr_off_x, acc_usr_off_x1, acc_usr_off_x2;
    uint16_t acc_usr_off_y, acc_usr_off_y1, acc_usr_off_y2;
    uint16_t acc_usr_off_z, acc_usr_off_z1, acc_usr_off_z2;
    uint8_t acc_usr_gain_x, acc_usr_gain_y, acc_usr_gain_z;

    if (config != NULL)
    {
	/* Set the accel offset and gain base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set accel user offset for x-axis for lsb 8 bits */
	    acc_usr_off_x1 = BMI3_SET_BIT_POS0(data_array[0], BMI3_ACC_USR_OFF_X, config->acc_usr_off_x);

	    acc_usr_off_x = ((uint16_t)data_array[1] << 8);

	    /* Set accel user offset for x-axis for msb 8 bits */
	    acc_usr_off_x2 = BMI3_SET_BIT_POS0(acc_usr_off_x, BMI3_ACC_USR_OFF_X, config->acc_usr_off_x);

	    /* Set accel user offset for y-axis for lsb 8 bits */
	    acc_usr_off_y1 = BMI3_SET_BIT_POS0(data_array[2], BMI3_ACC_USR_OFF_Y, config->acc_usr_off_y);

	    acc_usr_off_y = ((uint16_t)data_array[3] << 8);

	    /* Set accel user offset for y-axis for msb 8 bits */
	    acc_usr_off_y2 = BMI3_SET_BIT_POS0(acc_usr_off_y, BMI3_ACC_USR_OFF_Y, config->acc_usr_off_y);

	    /* Set accel user offset for z-axis for lsb 8 bits */
	    acc_usr_off_z1 = BMI3_SET_BIT_POS0(data_array[4], BMI3_ACC_USR_OFF_Z, config->acc_usr_off_z);

	    acc_usr_off_z = ((uint16_t)data_array[5] << 8);

	    /* Set accel user offset for z-axis for msb 8 bits */
	    acc_usr_off_z2 = BMI3_SET_BIT_POS0(acc_usr_off_z, BMI3_ACC_USR_OFF_Z, config->acc_usr_off_z);

	    /* Set accel user gain for x-axis */
	    acc_usr_gain_x = BMI3_SET_BIT_POS0(data_array[6], BMI3_ACC_USR_GAIN_X, config->acc_usr_gain_x);

	    /* Set accel user gain for y-axis */
	    acc_usr_gain_y = BMI3_SET_BIT_POS0(data_array[8], BMI3_ACC_USR_GAIN_Y, config->acc_usr_gain_y);

	    /* Set accel user gain for z-axis */
	    acc_usr_gain_z = BMI3_SET_BIT_POS0(data_array[10], BMI3_ACC_USR_GAIN_Z, config->acc_usr_gain_z);

	    acc_usr_gain_offset[0] = (uint8_t)acc_usr_off_x1;
	    acc_usr_gain_offset[1] = (uint8_t)(acc_usr_off_x2 >> 8);
	    acc_usr_gain_offset[2] = (uint8_t)(acc_usr_off_y1);
	    acc_usr_gain_offset[3] = (uint8_t)(acc_usr_off_y2 >> 8);
	    acc_usr_gain_offset[4] = (uint8_t)(acc_usr_off_z1);
	    acc_usr_gain_offset[5] = (uint8_t)(acc_usr_off_z2 >> 8);
	    acc_usr_gain_offset[6] = acc_usr_gain_x;
	    acc_usr_gain_offset[7] = data_array[7];
	    acc_usr_gain_offset[8] = acc_usr_gain_y;
	    acc_usr_gain_offset[9] = data_array[9];
	    acc_usr_gain_offset[10] = acc_usr_gain_z;
	    acc_usr_gain_offset[11] = data_array[11];

	    /* Set the configurations back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, acc_usr_gain_offset, 12, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API gets gyro offset and gain values.
 */
int8_t bmi3_get_gyr_usr_off_gain(struct bmi3_gyr_usr_gain_offset *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the gyro user gain offset values */
    uint8_t gyr_usr_gain_offset[12] = { 0 };

    /* Array to set the base address of gyro offset and gain */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_GYRO_OFFSET_GAIN, 0 };

    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    if (config != NULL)
    {
	/* Set the gyro offset and gain base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where gyro offset and gain resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, gyr_usr_gain_offset, 12, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate gyr_usr_off_x */
		lsb = (uint16_t) gyr_usr_gain_offset[idx++];
		msb = ((uint16_t) gyr_usr_gain_offset[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get gyro user offset for x-axis */
		config->gyr_usr_off_x = (lsb_msb & BMI3_GYR_USR_OFF_X_MASK);

		/* Get word to calculate gyr_usr_off_y */
		lsb = (uint16_t) gyr_usr_gain_offset[idx++];
		msb = ((uint16_t) gyr_usr_gain_offset[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get gyro user offset for y-axis */
		config->gyr_usr_off_y = (lsb_msb & BMI3_GYR_USR_OFF_Y_MASK);

		/* Get word to calculate gyr_usr_off_z */
		lsb = (uint16_t) gyr_usr_gain_offset[idx++];
		msb = ((uint16_t) gyr_usr_gain_offset[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get gyro user offset for z-axis */
		config->gyr_usr_off_z = (lsb_msb & BMI3_GYR_USR_OFF_Z_MASK);

		/* Get word to calculate gyr_usr_gain_x */
		lsb = (uint16_t) gyr_usr_gain_offset[idx++];
		msb = ((uint16_t) gyr_usr_gain_offset[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get gyro user gain for x-axis */
		config->gyr_usr_gain_x = (uint8_t)(lsb_msb & BMI3_GYR_USR_GAIN_X_MASK);

		/* Get word to calculate gyr_usr_gain_y */
		lsb = (uint16_t) gyr_usr_gain_offset[idx++];
		msb = ((uint16_t) gyr_usr_gain_offset[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get gyro user gain for y-axis */
		config->gyr_usr_gain_y = (uint8_t)(lsb_msb & BMI3_GYR_USR_GAIN_Y_MASK);

		/* Get word to calculate gyr_usr_gain_z */
		lsb = (uint16_t) gyr_usr_gain_offset[idx++];
		msb = ((uint16_t) gyr_usr_gain_offset[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get gyro user gain for z-axis */
		config->gyr_usr_gain_z = (uint8_t)(lsb_msb & BMI3_GYR_USR_GAIN_Z_MASK);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This API sets gyro offset and gain values.
 */
int8_t bmi3_set_gyr_usr_off_gain(const struct bmi3_gyr_usr_gain_offset *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the gyro user gain offset values */
    uint8_t gyr_usr_gain_offset[12] = { 0 };

    /* Array to set the base address of gyro offset and gain */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_GYRO_OFFSET_GAIN, 0 };

    uint8_t data_array[12] = { 0 };

    uint16_t gyr_usr_off_x, gyr_usr_off_x1, gyr_usr_off_x2;
    uint16_t gyr_usr_off_y, gyr_usr_off_y1, gyr_usr_off_y2;
    uint16_t gyr_usr_off_z, gyr_usr_off_z1, gyr_usr_off_z2;
    uint8_t gyr_usr_gain_x, gyr_usr_gain_y, gyr_usr_gain_z;

    if (config != NULL)
    {
	/* Set the gyro offset and gain base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set gyro user offset for x-axis for lsb 8 bits */
	    gyr_usr_off_x1 = BMI3_SET_BIT_POS0(data_array[0], BMI3_GYR_USR_OFF_X, config->gyr_usr_off_x);

	    gyr_usr_off_x = ((uint16_t)data_array[1] << 8);

	    /* Set gyro user offset for x-axis for msb 8 bits */
	    gyr_usr_off_x2 = BMI3_SET_BIT_POS0(gyr_usr_off_x, BMI3_GYR_USR_OFF_X, config->gyr_usr_off_x);

	    /* Set gyro user offset for y-axis for lsb 8 bits */
	    gyr_usr_off_y1 = BMI3_SET_BIT_POS0(data_array[2], BMI3_GYR_USR_OFF_Y, config->gyr_usr_off_y);

	    gyr_usr_off_y = ((uint16_t)data_array[3] << 8);

	    /* Set gyro user offset for y-axis for msb 8 bits */
	    gyr_usr_off_y2 = BMI3_SET_BIT_POS0(gyr_usr_off_y, BMI3_GYR_USR_OFF_Y, config->gyr_usr_off_y);

	    /* Set gyro user offset for z-axis for lsb 8 bits */
	    gyr_usr_off_z1 = BMI3_SET_BIT_POS0(data_array[4], BMI3_GYR_USR_OFF_Z, config->gyr_usr_off_z);

	    gyr_usr_off_z = ((uint16_t)data_array[5] << 8);

	    /* Set gyro user offset for z-axis for msb 8 bits */
	    gyr_usr_off_z2 = BMI3_SET_BIT_POS0(gyr_usr_off_z, BMI3_GYR_USR_OFF_Z, config->gyr_usr_off_z);

	    /* Set gyro user gain for x-axis */
	    gyr_usr_gain_x = BMI3_SET_BIT_POS0(data_array[6], BMI3_GYR_USR_GAIN_X, config->gyr_usr_gain_x);

	    /* Set gyro user gain for y-axis */
	    gyr_usr_gain_y = BMI3_SET_BIT_POS0(data_array[8], BMI3_GYR_USR_GAIN_Y, config->gyr_usr_gain_y);

	    /* Set gyro user gain for z-axis */
	    gyr_usr_gain_z = BMI3_SET_BIT_POS0(data_array[10], BMI3_GYR_USR_GAIN_Z, config->gyr_usr_gain_z);

	    gyr_usr_gain_offset[0] = (uint8_t)gyr_usr_off_x1;
	    gyr_usr_gain_offset[1] = (uint8_t)(gyr_usr_off_x2 >> 8);
	    gyr_usr_gain_offset[2] = (uint8_t)(gyr_usr_off_y1);
	    gyr_usr_gain_offset[3] = (uint8_t)(gyr_usr_off_y2 >> 8);
	    gyr_usr_gain_offset[4] = (uint8_t)(gyr_usr_off_z1);
	    gyr_usr_gain_offset[5] = (uint8_t)(gyr_usr_off_z2 >> 8);
	    gyr_usr_gain_offset[6] = gyr_usr_gain_x;
	    gyr_usr_gain_offset[7] = data_array[7];
	    gyr_usr_gain_offset[8] = gyr_usr_gain_y;
	    gyr_usr_gain_offset[9] = data_array[9];
	    gyr_usr_gain_offset[10] = gyr_usr_gain_z;
	    gyr_usr_gain_offset[11] = data_array[11];

	    /* Set the configurations back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, gyr_usr_gain_offset, 12, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/***************************************************************************/

/*!                   Local Function Definitions
 ****************************************************************************/

/*!
 * @brief This internal API sets accelerometer configurations like ODR, accel mode,
 * bandwidth, average samples and range.
 */
static int8_t set_accel_config(struct bmi3_accel_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to store data */
    uint8_t reg_data[2] = { 0 };

    uint16_t odr, range, bwp, avg_num, acc_mode;

    if (config != NULL)
    {
	/* Validate bandwidth and averaging samples */
	rslt = validate_bw_avg_acc_mode(&config->bwp, &config->acc_mode, &config->avg_num, dev);

	if (rslt == BMI3_OK)
	{
	    /* Validate ODR and range */
	    rslt = validate_acc_odr_range(&config->odr, &config->range, dev);
	}

	if (rslt == BMI3_OK)
	{
	    if (config->acc_mode == BMI3_ACC_MODE_LOW_PWR)
	    {
		rslt = validate_acc_odr_avg(config->odr, config->avg_num);
	    }
	}

	if (rslt == BMI3_OK)
	{
	    /* Set accelerometer ODR */
	    odr = BMI3_SET_BIT_POS0(reg_data[0], BMI3_ACC_ODR, config->odr);

	    /* Set accelerometer range */
	    range = BMI3_SET_BITS(reg_data[0], BMI3_ACC_RANGE, config->range);

	    /* Set accelerometer bandwidth */
	    bwp = BMI3_SET_BITS(reg_data[0], BMI3_ACC_BW, config->bwp);

	    /* Set accelerometer average number of samples */
	    avg_num = BMI3_SET_BITS(reg_data[1], BMI3_ACC_AVG_NUM, config->avg_num);

	    /* Set accelerometer accel mode */
	    acc_mode = BMI3_SET_BITS(reg_data[1], BMI3_ACC_MODE, config->acc_mode);

	    reg_data[0] = (uint8_t)(odr | range | bwp);
	    reg_data[1] = (uint8_t)((avg_num | acc_mode) >> 8);

	    /* Set configurations for accel */
	    rslt = bmi3_set_regs(BMI3_REG_ACC_CONF, reg_data, 2, dev);
        } else
	{
	    rslt = BMI3_E_ACC_INVALID_CFG;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets accelerometer configurations like ODR,
 * bandwidth, accel mode, average samples and gravity range.
 */
static int8_t get_accel_config(struct bmi3_accel_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    uint16_t reg_data;

    if (config != NULL)
    {
	/* Read the sensor configuration details */
	rslt = bmi3_get_regs(BMI3_REG_ACC_CONF, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    reg_data = data_array[0];

	    /* Get accelerometer ODR */
	    config->odr = BMI3_GET_BIT_POS0(reg_data, BMI3_ACC_ODR);

	    /* Get accelerometer range */
	    config->range = BMI3_GET_BITS(reg_data, BMI3_ACC_RANGE);

	    /* Get accelerometer bandwidth */
	    config->bwp = BMI3_GET_BITS(reg_data, BMI3_ACC_BW);

	    reg_data = (uint16_t)data_array[1] << 8;

	    /* Get accelerometer average samples */
	    config->avg_num = BMI3_GET_BITS(reg_data, BMI3_ACC_AVG_NUM);

	    /* Get accel mode */
	    config->acc_mode = BMI3_GET_BITS(reg_data, BMI3_ACC_MODE);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API validates bandwidth and accel mode of the
 * accelerometer set by the user.
 */
static int8_t validate_bw_avg_acc_mode(uint8_t *bandwidth, uint8_t *acc_mode, uint8_t *avg_num, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    if ((bandwidth != NULL) && (acc_mode != NULL) && (avg_num != NULL))
    {
	/* Validate and auto-correct accel mode */
	rslt = check_boundary_val(acc_mode, BMI3_ACC_MODE_DISABLE, BMI3_ACC_MODE_HIGH_PERF, dev);

	if (rslt == BMI3_OK)
	{
	    /* Validate for averaging number of samples */
	    rslt = check_boundary_val(avg_num, BMI3_ACC_AVG1, BMI3_ACC_AVG64, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Validate bandwidth */
		rslt = check_boundary_val(bandwidth, BMI3_ACC_BW_ODR_HALF, BMI3_ACC_BW_ODR_QUARTER, dev);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API validates ODR and range of the accelerometer set by
 * the user.
 */
static int8_t validate_acc_odr_range(uint8_t *odr, uint8_t *range, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    if ((odr != NULL) && (range != NULL))
    {
	/* Validate and auto correct ODR */
	rslt = check_boundary_val(odr, BMI3_ACC_ODR_0_78HZ, BMI3_ACC_ODR_6400HZ, dev);

	if (rslt == BMI3_OK)
	{
	    /* Validate and auto correct Range */
	    rslt = check_boundary_val(range, BMI3_ACC_RANGE_2G, BMI3_ACC_RANGE_16G, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API is used to validate the boundary conditions.
 */
static int8_t check_boundary_val(uint8_t *val, uint8_t min, uint8_t max, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if ((rslt == BMI3_OK) && (val != NULL))
    {
	/* Check if value is below minimum value */
	if (*val < min)
	{
	    /* Auto correct the invalid value to minimum value */
	    *val = min;
	    dev->info |= BMI3_I_MIN_VALUE;
	}

	/* Check if value is above maximum value */
	if (*val > max)
	{
	    /* Auto correct the invalid value to maximum value */
	    *val = max;
	    dev->info |= BMI3_I_MAX_VALUE;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets gyroscope configurations like ODR,
 * bandwidth, gyro mode, average samples and dps range.
 */
static int8_t set_gyro_config(struct bmi3_gyro_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to store data */
    uint8_t reg_data[2] = { 0 };

    uint16_t odr, range, bwp, avg_num, gyr_mode;

    if (config != NULL)
    {
	/* Validate bandwidth, average samples and mode */
	rslt = validate_bw_avg_gyr_mode(&config->bwp, &config->gyr_mode, &config->avg_num, dev);

	if (rslt == BMI3_OK)
	{
	    /* Validate ODR and range */
	    rslt = validate_gyr_odr_range(&config->odr, &config->range, dev);
	}

	if (rslt == BMI3_OK)
	{
	    if (config->gyr_mode == BMI3_GYR_MODE_LOW_PWR)
	    {
		rslt = validate_gyr_odr_avg(config->odr, config->avg_num);
	    }
	}

	if (rslt == BMI3_OK)
	{
	    /* Set gyroscope ODR */
	    odr = BMI3_SET_BIT_POS0(reg_data[0], BMI3_GYR_ODR, config->odr);

	    /* Set gyroscope range */
	    range = BMI3_SET_BITS(reg_data[0], BMI3_GYR_RANGE, config->range);

	    /* Set gyroscope bandwidth */
	    bwp = BMI3_SET_BITS(reg_data[0], BMI3_GYR_BW, config->bwp);

	    /* Set gyroscope average sample */
	    avg_num = BMI3_SET_BITS(reg_data[1], BMI3_GYR_AVG_NUM, config->avg_num);

	    /* Set gyroscope mode */
	    gyr_mode = BMI3_SET_BITS(reg_data[1], BMI3_GYR_MODE, config->gyr_mode);

	    reg_data[0] = (uint8_t)(odr | range | bwp);
	    reg_data[1] = (uint8_t)((avg_num | gyr_mode) >> 8);

	    /* Set gyro configurations */
	    rslt = bmi3_set_regs(BMI3_REG_GYR_CONF, reg_data, 2, dev);
        } else
	{
	    rslt = BMI3_E_GYRO_INVALID_CFG;
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API validates bandwidth, average samples and gyr mode of the
 * gyroscope set by the user.
 */
static int8_t validate_bw_avg_gyr_mode(uint8_t *bandwidth,
				       uint8_t *gyr_mode,
				       const uint8_t *avg_num,
				       struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    if ((bandwidth != NULL) && (gyr_mode != NULL) && (avg_num != NULL))
    {
	/* Validate and auto-correct gyro mode */
	rslt = check_boundary_val(gyr_mode, BMI3_GYR_MODE_DISABLE, BMI3_GYR_MODE_HIGH_PERF, dev);

	if (rslt == BMI3_OK)
	{
	    /* Validate for averaging mode */
	    rslt = check_boundary_val(bandwidth, BMI3_GYR_AVG1, BMI3_GYR_AVG64, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Validate for bandwidth */
		rslt = check_boundary_val(bandwidth, BMI3_GYR_BW_ODR_HALF, BMI3_GYR_BW_ODR_QUARTER, dev);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API validates ODR and range of the gyroscope set by
 * the user.
 */
static int8_t validate_gyr_odr_range(uint8_t *odr, uint8_t *range, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    if ((odr != NULL) && (range != NULL))
    {
	/* Validate and auto correct ODR */
	rslt = check_boundary_val(odr, BMI3_GYR_ODR_0_78HZ, BMI3_GYR_ODR_6400HZ, dev);

	if (rslt == BMI3_OK)
	{
	    /* Validate and auto correct Range */
	    rslt = check_boundary_val(range, BMI3_GYR_RANGE_125DPS, BMI3_GYR_RANGE_2000DPS, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets the accelerometer data from the register.
 */
static int8_t get_accel_sensor_data(struct bmi3_sens_axes_data *data, uint8_t reg_addr, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define data stored in register */
    uint8_t reg_data[BMI3_ACC_NUM_BYTES] = { 0 };

    /* Stores the accel x, y and z axis data from register */
    uint16_t acc_data[6];

    if (data != NULL)
    {
	/* Read the sensor data */
	rslt = bmi3_get_regs(reg_addr, reg_data, BMI3_ACC_NUM_BYTES, dev);

	if (rslt == BMI3_OK)
	{
	    acc_data[0] = (reg_data[0] | (uint16_t)reg_data[1] << 8);
	    acc_data[1] = (reg_data[2] | (uint16_t)reg_data[3] << 8);
	    acc_data[2] = (reg_data[4] | (uint16_t)reg_data[5] << 8);
	    acc_data[3] = (reg_data[14] | (uint16_t)reg_data[15] << 8);
	    acc_data[4] = (reg_data[16] | (uint16_t)reg_data[17] << 8);
	    acc_data[5] = reg_data[18];

	    /* Get accelerometer data from the register */
	    get_acc_data(data, acc_data);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets the gyroscope data from the register.
 */
static int8_t get_gyro_sensor_data(struct bmi3_sens_axes_data *data, uint8_t reg_addr, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define data stored in register */
    uint8_t reg_data[BMI3_GYR_NUM_BYTES] = { 0 };

    /* Variable to store x, y and z axis gyro data */
    uint16_t gyr_data[6];

    if (data != NULL)
    {
	/* Read the sensor data */
	rslt = bmi3_get_regs(reg_addr, reg_data, BMI3_GYR_NUM_BYTES, dev);

	if (rslt == BMI3_OK)
	{
	    gyr_data[0] = (reg_data[0] | (uint16_t)reg_data[1] << 8);
	    gyr_data[1] = (reg_data[2] | (uint16_t)reg_data[3] << 8);
	    gyr_data[2] = (reg_data[4] | (uint16_t)reg_data[5] << 8);
	    gyr_data[3] = (reg_data[8] | (uint16_t)reg_data[9] << 8);
	    gyr_data[4] = (reg_data[10] | (uint16_t)reg_data[11] << 8);
	    gyr_data[5] = reg_data[12];

	    /* Get gyro data from the register */
	    get_gyr_data(data, gyr_data);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets the step counter data from the register.
 */
static int8_t get_step_counter_sensor_data(uint32_t *step_count, uint8_t reg_addr, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define data stored in register */
    uint8_t reg_data[4] = { 0 };

    if (step_count != NULL)
    {
	/* Read the sensor data */
	rslt = bmi3_get_regs(reg_addr, reg_data, 4, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the step counter output in 4 bytes */
	    *step_count = (uint32_t) reg_data[0];
	    *step_count |= ((uint32_t) reg_data[1] << 8);
	    *step_count |= ((uint32_t) reg_data[2] << 16);
	    *step_count |= ((uint32_t) reg_data[3] << 24);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets the output values of orientation: portrait-
 * landscape and face up-down.
 */
static int8_t get_orient_output_data(struct bmi3_orientation_output *orient_out, uint8_t reg_addr, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define data stored in register */
    uint8_t reg_data[2] = { 0 };

    if (orient_out != NULL)
    {
	/* Read the data from feature engine status register */
	rslt = bmi3_get_regs(reg_addr, reg_data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the output value of the orientation detection feature */
	    orient_out->orientation_portrait_landscape = BMI3_GET_BIT_POS0(reg_data[0],
									   BMI3_ORIENTATION_PORTRAIT_LANDSCAPE);

	    /* Get the output value of the orientation face up-down feature */
	    orient_out->orientation_faceup_down = BMI3_GET_BITS(reg_data[0], BMI3_ORIENTATION_FACEUP_DOWN);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets gyroscope configurations like ODR, gyro mode,
 * bandwidth, averaging samples and range.
 */
static int8_t get_gyro_config(struct bmi3_gyro_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    uint16_t reg_data;

    if (config != NULL)
    {
	/* Read the sensor configuration details */
	rslt = bmi3_get_regs(BMI3_REG_GYR_CONF, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    reg_data = data_array[0];

	    /* Get gyro ODR */
	    config->odr = BMI3_GET_BIT_POS0(reg_data, BMI3_GYR_ODR);

	    /* Get gyro range */
	    config->range = BMI3_GET_BITS(reg_data, BMI3_GYR_RANGE);

	    /* Get gyro bandwidth */
	    config->bwp = BMI3_GET_BITS(reg_data, BMI3_GYR_BW);

	    reg_data = (uint16_t)data_array[1] << 8;

	    /* Get gyro average sample */
	    config->avg_num = BMI3_GET_BITS(reg_data, BMI3_GYR_AVG_NUM);

	    /* Get gyro mode */
	    config->gyr_mode = BMI3_GET_BITS(reg_data, BMI3_GYR_MODE);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets the accelerometer data.
 */
static void get_acc_data(struct bmi3_sens_axes_data *data, const uint16_t *reg_data)
{
    /* Stores accel x-axis data */
    data->x = (int16_t)reg_data[0];

    /* Stores accel y-axis data */
    data->y = (int16_t)reg_data[1];

    /* Stores accel z-axis data */
    data->z = (int16_t)reg_data[2];

    /* Stores sensor time data */
    data->sens_time = (reg_data[3] | ((uint32_t)reg_data[4] << 16));

    /* Stores saturation x-axis data */
    data->sat_x = (reg_data[5] & BMI3_SATF_ACC_X_MASK);

    /* Stores saturation y-axis data */
    data->sat_y = (reg_data[5] & BMI3_SATF_ACC_Y_MASK) >> BMI3_SATF_ACC_Y_POS;

    /* Stores saturation z-axis data */
    data->sat_z = (reg_data[5] & BMI3_SATF_ACC_Z_MASK) >> BMI3_SATF_ACC_Z_POS;
}

/*!
 * @brief This internal API gets the gyroscope data.
 */
static void get_gyr_data(struct bmi3_sens_axes_data *data, const uint16_t *reg_data)
{
    /* Stores accel x-axis data */
    data->x = (int16_t)reg_data[0];

    /* Stores accel y-axis data */
    data->y = (int16_t)reg_data[1];

    /* Stores accel z-axis data */
    data->z = (int16_t)reg_data[2];

    /* Stores sensor time data */
    data->sens_time = (reg_data[3] | ((uint32_t)reg_data[4] << 16));

    /* Stores saturation x-axis data */
    data->sat_x = (reg_data[5] & BMI3_SATF_GYR_X_MASK) >> BMI3_SATF_GYR_X_POS;

    /* Stores saturation y-axis data */
    data->sat_y = (reg_data[5] & BMI3_SATF_GYR_Y_MASK) >> BMI3_SATF_GYR_Y_POS;

    /* Stores saturation z-axis data */
    data->sat_z = (reg_data[5] & BMI3_SATF_GYR_Z_MASK) >> BMI3_SATF_GYR_Z_POS;
}

/*!
 * @brief This internal API is used to validate the device structure pointer for
 * null conditions.
 */
static int8_t null_ptr_check(const struct bmi3_dev *dev)
{
    int8_t rslt;

    if ((dev == NULL) || (dev->read == NULL) || (dev->write == NULL) || (dev->delay_us == NULL))
    {
	rslt = BMI3_E_NULL_PTR;
    } else
    {
	/* Device structure is fine */
	rslt = BMI3_OK;
    }

    return rslt;
}

/*!
 * @brief This internal API is used to get the enabled feature.
 */
static int8_t get_feature_enable(struct bmi3_feature_enable *enable, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_OK;

    uint8_t feature[2];
    uint8_t reg_data;
    uint16_t feature_config;

    if (enable != NULL)
    {
	rslt = bmi3_get_regs(BMI3_REG_FEATURE_IO0, feature, 2, dev);

	if (rslt == BMI3_OK)
	{
	    reg_data = feature[0];
	    enable->no_motion_x_en = BMI3_GET_BIT_POS0(reg_data, BMI3_NO_MOTION_X_EN);
	    enable->no_motion_y_en = BMI3_GET_BITS(reg_data, BMI3_NO_MOTION_Y_EN);
	    enable->no_motion_z_en = BMI3_GET_BITS(reg_data, BMI3_NO_MOTION_Z_EN);
	    enable->any_motion_x_en = BMI3_GET_BITS(reg_data, BMI3_ANY_MOTION_X_EN);
	    enable->any_motion_y_en = BMI3_GET_BITS(reg_data, BMI3_ANY_MOTION_Y_EN);
	    enable->any_motion_z_en = BMI3_GET_BITS(reg_data, BMI3_ANY_MOTION_Z_EN);
	    enable->flat_en = BMI3_GET_BITS(reg_data, BMI3_FLAT_EN);
	    enable->orientation_en = BMI3_GET_BITS(reg_data, BMI3_ORIENTATION_EN);

	    feature_config = (uint16_t)feature[1] << 8;

	    enable->step_detector_en = BMI3_GET_BITS(feature_config, BMI3_STEP_DETECTOR_EN);
	    enable->step_counter_en = BMI3_GET_BITS(feature_config, BMI3_STEP_COUNTER_EN);
	    enable->sig_motion_en = BMI3_GET_BITS(feature_config, BMI3_SIG_MOTION_EN);
	    enable->tilt_en = BMI3_GET_BITS(feature_config, BMI3_TILT_EN);
	    enable->tap_detector_s_tap_en = BMI3_GET_BITS(feature_config, BMI3_TAP_DETECTOR_S_TAP_EN);
	    enable->tap_detector_d_tap_en = BMI3_GET_BITS(feature_config, BMI3_TAP_DETECTOR_D_TAP_EN);
	    enable->tap_detector_t_tap_en = BMI3_GET_BITS(feature_config, BMI3_TAP_DETECTOR_T_TAP_EN);
	    enable->i3c_sync_en = BMI3_GET_BITS(feature_config, BMI3_I3C_SYNC_EN);
	}
    }

    return rslt;
}

/*!
 * @brief This internal API is used to set the feature.
 */
static int8_t set_feature_enable(const struct bmi3_feature_enable *enable, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t feature[2] = { 0 };
    uint16_t reg_data;

    uint16_t no_motion_x_en;
    uint16_t no_motion_y_en;
    uint16_t no_motion_z_en;
    uint16_t any_motion_x_en;
    uint16_t any_motion_y_en;
    uint16_t any_motion_z_en;
    uint16_t flat_en;
    uint16_t orientation_en;
    uint16_t step_detector_en;
    uint16_t step_counter_en;
    uint16_t sig_motion_en;
    uint16_t tilt_en;
    uint16_t tap_detector_s_tap_en;
    uint16_t tap_detector_d_tap_en;
    uint16_t tap_detector_t_tap_en;
    uint16_t i3c_sync_en;

    /* Array variable to clear feature before enabling into register */
    uint8_t reset[2] = { 0 };

    /* Array to set feature_engine_gp_status in order to enable the feature */
    uint8_t gp_status[2] = { 1, 0 };

    if (enable != NULL)
    {
	rslt = bmi3_get_regs(BMI3_REG_FEATURE_IO0, feature, 2, dev);

	if (rslt == BMI3_OK)
	{
	    reg_data = feature[0];
	    no_motion_x_en = BMI3_SET_BIT_POS0(reg_data, BMI3_NO_MOTION_X_EN, enable->no_motion_x_en);
	    no_motion_y_en = BMI3_SET_BITS(reg_data, BMI3_NO_MOTION_Y_EN, enable->no_motion_y_en);
	    no_motion_z_en = BMI3_SET_BITS(reg_data, BMI3_NO_MOTION_Z_EN, enable->no_motion_z_en);
	    any_motion_x_en = BMI3_SET_BITS(reg_data, BMI3_ANY_MOTION_X_EN, enable->any_motion_x_en);
	    any_motion_y_en = BMI3_SET_BITS(reg_data, BMI3_ANY_MOTION_Y_EN, enable->any_motion_y_en);
	    any_motion_z_en = BMI3_SET_BITS(reg_data, BMI3_ANY_MOTION_Z_EN, enable->any_motion_z_en);
	    flat_en = BMI3_SET_BITS(reg_data, BMI3_FLAT_EN, enable->flat_en);
	    orientation_en = BMI3_SET_BITS(reg_data, BMI3_ORIENTATION_EN, enable->orientation_en);

	    reg_data = ((uint16_t)feature[1] << 8);

	    step_detector_en = BMI3_SET_BITS(reg_data, BMI3_STEP_DETECTOR_EN, enable->step_detector_en);
	    step_counter_en = BMI3_SET_BITS(reg_data, BMI3_STEP_COUNTER_EN, enable->step_counter_en);
	    sig_motion_en = BMI3_SET_BITS(reg_data, BMI3_SIG_MOTION_EN, enable->sig_motion_en);
	    tilt_en = BMI3_SET_BITS(reg_data, BMI3_TILT_EN, enable->tilt_en);
	    tap_detector_s_tap_en = BMI3_SET_BITS(reg_data, BMI3_TAP_DETECTOR_S_TAP_EN, enable->tap_detector_s_tap_en);
	    tap_detector_d_tap_en = BMI3_SET_BITS(reg_data, BMI3_TAP_DETECTOR_D_TAP_EN, enable->tap_detector_d_tap_en);
	    tap_detector_t_tap_en = BMI3_SET_BITS(reg_data, BMI3_TAP_DETECTOR_T_TAP_EN, enable->tap_detector_t_tap_en);
	    i3c_sync_en = BMI3_SET_BITS(reg_data, BMI3_I3C_SYNC_EN, enable->i3c_sync_en);

	    feature[0] =
		(uint8_t)(no_motion_x_en | no_motion_y_en | no_motion_z_en | any_motion_x_en | any_motion_y_en |
			  any_motion_z_en | flat_en | orientation_en);
	    feature[1] =
		(uint8_t)((step_detector_en | step_counter_en | sig_motion_en | tilt_en | tap_detector_s_tap_en |
			   tap_detector_d_tap_en | tap_detector_t_tap_en | i3c_sync_en) >> 8);

	    /* Reset the register before updating new values */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_IO0, reset, 2, dev);

	    if (rslt == BMI3_OK)
	    {
		rslt = bmi3_set_regs(BMI3_REG_FEATURE_IO0, feature, 2, dev);
	    }

	    if (rslt == BMI3_OK)
	    {
		rslt = bmi3_set_regs(BMI3_REG_FEATURE_IO_STATUS, gp_status, 2, dev);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets the latch mode from register address
 */
static int8_t get_latch_mode(struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define data array */
    uint8_t data_array[2] = { 0 };

    if (int_cfg != NULL)
    {
	rslt = bmi3_get_regs(BMI3_REG_INT_CONF, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    int_cfg->int_latch = BMI3_GET_BIT_POS0(data_array[0], BMI3_INT_LATCH);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets the latch mode to register address
 */
static int8_t set_latch_mode(const struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define data array */
    uint8_t data_array[2] = { 0 };

    if (int_cfg != NULL)
    {
	rslt = bmi3_get_regs(BMI3_REG_INT_CONF, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Configure the interrupt mode */
	    data_array[0] = BMI3_SET_BIT_POS0(data_array[0], BMI3_INT_LATCH, int_cfg->int_latch);

	    rslt = bmi3_set_regs(BMI3_REG_INT_CONF, data_array, 2, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets any-motion configurations like threshold,
 * duration, accel reference up, hysteresis and wait time.
 */
static int8_t get_any_motion_config(struct bmi3_any_motion_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t any_mot_config[6] = { 0 };

    /* Array to set the base address of any-motion feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ANY_MOTION, 0 };

    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    if (config != NULL)
    {
	/* Set the any-motion base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where any-motion feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, any_mot_config, 6, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate threshold and accel reference up from same word */
		lsb = (uint16_t) any_mot_config[idx++];
		msb = ((uint16_t) any_mot_config[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get threshold */
		config->slope_thres = (lsb_msb & BMI3_ANY_NO_SLOPE_THRESHOLD_MASK);

		/* Get accel reference up */
		config->acc_ref_up = (lsb_msb & BMI3_ANY_NO_ACC_REF_UP_MASK) >> BMI3_ANY_NO_ACC_REF_UP_POS;

		/* Get word to calculate hysteresis from the word */
		lsb = (uint16_t) any_mot_config[idx++];
		msb = ((uint16_t) any_mot_config[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get hysteresis */
		config->hysteresis = (lsb_msb & BMI3_ANY_NO_HYSTERESIS_MASK);

		/* Get word to calculate duration and wait time from the same word */
		lsb = (uint16_t) any_mot_config[idx++];
		msb = ((uint16_t) any_mot_config[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get duration */
		config->duration = (lsb_msb & BMI3_ANY_NO_DURATION_MASK);

		/* Get wait time */
		config->wait_time = (lsb_msb & BMI3_ANY_NO_WAIT_TIME_MASK) >> BMI3_ANY_NO_WAIT_TIME_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets any-motion configurations like threshold,
 * duration, accel reference up, hysteresis and wait time.
 */
static int8_t set_any_motion_config(const struct bmi3_any_motion_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to set the base address of any-motion feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ANY_MOTION, 0 };

    /* Array to define the feature configuration */
    uint8_t any_mot_config[6] = { 0 };

    uint8_t data_array[6] = { 0 };

    uint16_t duration1, duration2;

    uint16_t threshold1, threshold2;

    uint16_t hysteresis1, hysteresis2;

    uint16_t threshold, hysteresis, duration, wait_time1;

    uint16_t acc_ref_up;

    /* Wait time for clearing the event after slope is below threshold */
    uint16_t wait_time;

    if (config != NULL)
    {
	/* Set the any-motion base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set threshold for lsb 8 bits */
	    threshold1 = BMI3_SET_BIT_POS0(data_array[0], BMI3_ANY_NO_SLOPE_THRESHOLD, config->slope_thres);

	    threshold = ((uint16_t)data_array[1] << 8);

	    /* Set threshold for msb 8 bits */
	    threshold2 = BMI3_SET_BIT_POS0(threshold, BMI3_ANY_NO_SLOPE_THRESHOLD, config->slope_thres);

	    /* Set accel reference */
	    acc_ref_up = ((uint16_t)data_array[1] << 8);

	    acc_ref_up = BMI3_SET_BITS(acc_ref_up, BMI3_ANY_NO_ACC_REF_UP, config->acc_ref_up);

	    /* Set hysteresis for lsb 8 bits */
	    hysteresis1 = BMI3_SET_BIT_POS0(data_array[2], BMI3_ANY_NO_HYSTERESIS, config->hysteresis);

	    hysteresis = ((uint16_t)data_array[2] << 8);

	    /* Set hysteresis for msb 8 bits */
	    hysteresis2 = BMI3_SET_BIT_POS0(hysteresis, BMI3_ANY_NO_HYSTERESIS, config->hysteresis);

	    /* Set duration for lsb 8 bits */
	    duration1 = BMI3_SET_BIT_POS0(data_array[3], BMI3_ANY_NO_DURATION, config->duration);

	    duration = ((uint16_t)data_array[4] << 8);

	    /* Set duration for msb 8 bits */
	    duration2 = BMI3_SET_BIT_POS0(duration, BMI3_ANY_NO_DURATION, config->duration);

	    wait_time1 = ((uint16_t)data_array[5] << 8);

	    /* Set wait time */
	    wait_time = BMI3_SET_BITS(wait_time1, BMI3_ANY_NO_WAIT_TIME, config->wait_time);

	    any_mot_config[0] = (uint8_t)threshold1;
	    any_mot_config[1] = (uint8_t)((threshold2 | acc_ref_up) >> 8);
	    any_mot_config[2] = (uint8_t)(hysteresis1);
	    any_mot_config[3] = (uint8_t)((hysteresis2) >> 8);
	    any_mot_config[4] = (uint8_t)(duration1);
	    any_mot_config[5] = (uint8_t)((duration2 | wait_time) >> 8);

	    /* Set the configurations back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, any_mot_config, 6, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets no-motion configurations like threshold,
 * duration, accel reference up, hysteresis and wait time.
 */
static int8_t get_no_motion_config(struct bmi3_no_motion_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t no_mot_config[6] = { 0 };

    /* Array to set the base address of no-motion feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_NO_MOTION, 0 };

    /* Variable to define array offset */
    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    if (config != NULL)
    {
	/* Set the no-motion base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where no-motion feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, no_mot_config, 6, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate threshold and accel reference up from same word */
		lsb = (uint16_t) no_mot_config[idx++];
		msb = ((uint16_t) no_mot_config[idx++] << 8);
		lsb_msb = (uint16_t)(lsb | msb);

		/* Get threshold */
		config->slope_thres = (lsb_msb & BMI3_ANY_NO_SLOPE_THRESHOLD_MASK);

		/* Get accel reference up */
		config->acc_ref_up = (lsb_msb & BMI3_ANY_NO_ACC_REF_UP_MASK) >> BMI3_ANY_NO_ACC_REF_UP_POS;

		/* Get word to calculate hysteresis */
		lsb = (uint16_t) no_mot_config[idx++];
		msb = ((uint16_t) no_mot_config[idx++] << 8);
		lsb_msb = (uint16_t)(lsb | msb);

		/* Get hysteresis */
		config->hysteresis = (lsb_msb & BMI3_ANY_NO_HYSTERESIS_MASK);

		/* Get word to calculate duration and wait time from same word */
		lsb = (uint16_t) no_mot_config[idx++];
		msb = ((uint16_t) no_mot_config[idx++] << 8);
		lsb_msb = (uint16_t)(lsb | msb);

		/* Get duration */
		config->duration = (lsb_msb & BMI3_ANY_NO_DURATION_MASK);

		/* Get wait time */
		config->wait_time = (lsb_msb & BMI3_ANY_NO_WAIT_TIME_MASK) >> BMI3_ANY_NO_WAIT_TIME_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets no-motion configurations like threshold,
 * duration, accel reference up, hysteresis and wait time.
 */
static int8_t set_no_motion_config(const struct bmi3_no_motion_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to set the base address of no-motion feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_NO_MOTION, 0 };

    /* Array to define the feature configuration */
    uint8_t no_mot_config[6] = { 0 };

    uint8_t data_array[6] = { 0 };

    uint16_t duration1, duration2;

    uint16_t threshold1, threshold2;

    uint16_t hysteresis1, hysteresis2;

    uint16_t threshold, hysteresis, duration, wait_time1;

    uint16_t acc_ref_up;

    uint16_t wait_time;

    if (config != NULL)
    {
	/* Set the no-motion base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set threshold for lsb 8 bits */
	    threshold1 = BMI3_SET_BIT_POS0(data_array[0], BMI3_ANY_NO_SLOPE_THRESHOLD, config->slope_thres);

	    threshold = ((uint16_t)data_array[1] << 8);

	    /* Set threshold for msb 8 bits */
	    threshold2 = BMI3_SET_BIT_POS0(threshold, BMI3_ANY_NO_SLOPE_THRESHOLD, config->slope_thres);

	    /* Set accel reference */
	    acc_ref_up = BMI3_SET_BITS(threshold, BMI3_ANY_NO_ACC_REF_UP, config->acc_ref_up);

	    /* Set hysteresis for lsb 8 bits */
	    hysteresis1 = BMI3_SET_BIT_POS0(data_array[2], BMI3_ANY_NO_HYSTERESIS, config->hysteresis);

	    hysteresis = ((uint16_t)data_array[2] << 8);

	    /* Set hysteresis for msb 8 bits */
	    hysteresis2 = BMI3_SET_BIT_POS0(hysteresis, BMI3_ANY_NO_HYSTERESIS, config->hysteresis);

	    /* Set duration for lsb 8 bits */
	    duration1 = BMI3_SET_BIT_POS0(data_array[3], BMI3_ANY_NO_DURATION, config->duration);

	    duration = ((uint16_t)data_array[4] << 8);

	    /* Set duration for msb 8 bits */
	    duration2 = BMI3_SET_BIT_POS0(duration, BMI3_ANY_NO_DURATION, config->duration);

	    wait_time1 = ((uint16_t)data_array[5] << 8);

	    /* Set wait time */
	    wait_time = BMI3_SET_BITS(wait_time1, BMI3_ANY_NO_WAIT_TIME, config->wait_time);

	    no_mot_config[0] = (uint8_t)threshold1;
	    no_mot_config[1] = (uint8_t)((threshold2 | acc_ref_up) >> 8);
	    no_mot_config[2] = (uint8_t)(hysteresis1);
	    no_mot_config[3] = (uint8_t)((hysteresis2) >> 8);
	    no_mot_config[4] = (uint8_t)(duration1);
	    no_mot_config[5] = (uint8_t)((duration2 | wait_time) >> 8);

	    /* Set the configuration back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, no_mot_config, 6, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets flat configurations like theta, blocking,
 * hold-time, hysteresis, and slope threshold.
 */
static int8_t get_flat_config(struct bmi3_flat_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t flat_config[4] = { 0 };

    /* Array to set the base address of flat feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_FLAT, 0 };

    /* Variable to define the array offset */
    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    if (config != NULL)
    {
	/* Set the flat base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where flat feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, flat_config, 4, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate theta, blocking and hold time from the same word */
		lsb = (uint16_t) flat_config[idx++];
		msb = ((uint16_t) flat_config[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get theta */
		config->theta = lsb_msb & BMI3_FLAT_THETA_MASK;

		/* Get blocking */
		config->blocking = (lsb_msb & BMI3_FLAT_BLOCKING_MASK) >> BMI3_FLAT_BLOCKING_POS;

		/* Get hold time */
		config->hold_time = (lsb_msb & BMI3_FLAT_HOLD_TIME_MASK) >> BMI3_FLAT_HOLD_TIME_POS;

		/* Get word to calculate slope threshold and hysteresis from the same word */
		lsb = (uint16_t) flat_config[idx++];
		msb = ((uint16_t) flat_config[idx++] << 8);
		lsb_msb = lsb | msb;

		/* Get slope threshold */
		config->slope_thres = lsb_msb & BMI3_FLAT_SLOPE_THRES_MASK;

		/* Get hysteresis */
		config->hysteresis = (lsb_msb & BMI3_FLAT_HYST_MASK) >> BMI3_FLAT_HYST_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets flat configurations like theta, blocking,
 * hold-time, hysteresis, and slope threshold.
 */
static int8_t set_flat_config(const struct bmi3_flat_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t flat_config[4] = { 0 };

    /* Array to set the base address of flat feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_FLAT, 0 };

    uint16_t theta, blocking, holdtime, slope_thres, hyst;

    if (config != NULL)
    {
	/* Set the flat base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set theta */
	    theta = BMI3_SET_BIT_POS0(flat_config[0], BMI3_FLAT_THETA, config->theta);

	    /* Set blocking */
	    blocking = BMI3_SET_BITS(flat_config[0], BMI3_FLAT_BLOCKING, config->blocking);

	    /* Set hold time */
	    holdtime = ((uint16_t)flat_config[1] << 8);
	    holdtime = BMI3_SET_BITS(holdtime, BMI3_FLAT_HOLD_TIME, config->hold_time);

	    /* Set slope threshold */
	    slope_thres = BMI3_SET_BIT_POS0(flat_config[2], BMI3_FLAT_SLOPE_THRES, config->slope_thres);

	    /* Set hysteresis */
	    hyst = ((uint16_t)flat_config[3] << 8);
	    hyst = BMI3_SET_BITS(hyst, BMI3_FLAT_HYST, config->hysteresis);

	    flat_config[0] = (uint8_t)(theta | blocking);
	    flat_config[1] = (uint8_t)(holdtime >> 8);
	    flat_config[2] = (uint8_t)(slope_thres);
	    flat_config[3] = (uint8_t)(hyst >> 8);

	    /* Set the configuration back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, flat_config, 4, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets sig-motion configurations like block size,
 * peak 2 peak min, mcr min, peak 2 peak max and mcr max.
 */
static int8_t get_sig_motion_config(struct bmi3_sig_motion_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t sig_mot_config[6];

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    /* Array to set the base address of sig-motion feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_SIG_MOTION, 0 };

    /* Variable to define the array offset */
    uint8_t idx = 0;

    if (config != NULL)
    {
	/* Set the sig-motion base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where sig motion feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, sig_mot_config, 6, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate block size */
		lsb = (uint16_t) sig_mot_config[idx++];
		msb = ((uint16_t) sig_mot_config[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get block size */
		config->block_size = lsb_msb & BMI3_SIG_BLOCK_SIZE_MASK;

		/* Get word to calculate peak 2 peak minimum from the same word */
		lsb = (uint16_t) sig_mot_config[idx++];
		msb = ((uint16_t) sig_mot_config[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get peak 2 peak minimum */
		config->peak_2_peak_min = (lsb_msb & BMI3_SIG_P2P_MIN_MASK);

		/* Get mcr minimum */
		config->mcr_min = (lsb_msb & BMI3_SIG_MCR_MIN_MASK) >> BMI3_SIG_MCR_MIN_POS;

		/* Get word to calculate peak 2 peak maximum and mcr maximum from the same word */
		lsb = (uint16_t) sig_mot_config[idx++];
		msb = ((uint16_t) sig_mot_config[idx++] << 8);
		lsb_msb = (lsb | msb);

		/* Get peak 2 peak maximum */
		config->peak_2_peak_max = (lsb_msb & BMI3_SIG_P2P_MAX_MASK);

		/* Get mcr maximum */
		config->mcr_max = (lsb_msb & BMI3_MCR_MAX_MASK) >> BMI3_MCR_MAX_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets sig-motion configurations like block size,
 * peak 2 peak min, mcr min, peak 2 peak max and mcr max.
 */
static int8_t set_sig_motion_config(const struct bmi3_sig_motion_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to set the base address of sig-motion feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_SIG_MOTION, 0 };

    /* Array to define the feature configuration */
    uint8_t sig_mot_config[6] = { 0 };

    uint8_t data_array[6] = { 0 };

    uint16_t block_size1, block_size2;

    uint16_t p2p_min1, p2p_min2;

    uint16_t p2p_max1, p2p_max2;

    uint16_t block_size, p2p_min, p2p_max;

    uint16_t mcr_min;

    uint16_t mcr_max;

    if (config != NULL)
    {
	/* Set the sig-motion base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set block size for lsb 8 bits */
	    block_size1 = BMI3_SET_BIT_POS0(data_array[0], BMI3_SIG_BLOCK_SIZE, config->block_size);

	    block_size = ((uint16_t)data_array[1] << 8);

	    /* Set block size for msb 8 bits */
	    block_size2 = BMI3_SET_BIT_POS0(block_size, BMI3_SIG_BLOCK_SIZE, config->block_size);

	    /* Set peak to peak minimum for lsb 8 bits */
	    p2p_min1 = BMI3_SET_BIT_POS0(data_array[2], BMI3_SIG_P2P_MIN, config->peak_2_peak_min);

	    p2p_min = ((uint16_t)data_array[3] << 8);

	    /* Set peak to peak minimum for msb 8 bits */
	    p2p_min2 = BMI3_SET_BIT_POS0(p2p_min, BMI3_SIG_P2P_MIN, config->peak_2_peak_min);

	    mcr_min = ((uint16_t)data_array[3] << 8);

	    /* Set mcr minimum */
	    mcr_min = BMI3_SET_BITS(mcr_min, BMI3_SIG_MCR_MIN, config->mcr_min);

	    /* Set peak to peak maximum for lsb 8 bits */
	    p2p_max1 = BMI3_SET_BIT_POS0(data_array[4], BMI3_SIG_P2P_MAX, config->peak_2_peak_max);

	    p2p_max = ((uint16_t)data_array[5] << 8);

	    /* Set peak to peak maximum for msb 8 bits */
	    p2p_max2 = BMI3_SET_BIT_POS0(p2p_max, BMI3_SIG_P2P_MAX, config->peak_2_peak_max);

	    mcr_max = ((uint16_t)data_array[5] << 8);

	    /* Set mcr maximum */
	    mcr_max = BMI3_SET_BITS(mcr_max, BMI3_MCR_MAX, config->mcr_max);

	    sig_mot_config[0] = (uint8_t)(block_size1);
	    sig_mot_config[1] = (uint8_t)(block_size2 >> 8);
	    sig_mot_config[2] = (uint8_t)(p2p_min1);
	    sig_mot_config[3] = (uint8_t)((p2p_min2 | mcr_min) >> 8);
	    sig_mot_config[4] = (uint8_t)(p2p_max1);
	    sig_mot_config[5] = (uint8_t)((p2p_max2 | mcr_max) >> 8);

	    /* Set the configuration back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, sig_mot_config, 6, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets tilt configurations like segment size,
 * tilt angle, beta accel mean.
 */
static int8_t get_tilt_config(struct bmi3_tilt_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t tilt_config[4] = { 0 };

    /* Array to set the base address of tilt feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_TILT, 0 };

    /* Variable to define the array offset */
    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define word */
    uint16_t lsb_msb;

    if (config != NULL)
    {
	/* Set the tilt base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where tilt feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, tilt_config, 4, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate segment size and minimum tilt angle from the same word */
		lsb = ((uint16_t)tilt_config[idx++]);
		msb = ((uint16_t)tilt_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get segment size */
		config->segment_size = lsb_msb & BMI3_TILT_SEGMENT_SIZE_MASK;

		/* Get minimum tilt angle */
		config->min_tilt_angle = (lsb_msb & BMI3_TILT_MIN_TILT_ANGLE_MASK) >> BMI3_TILT_MIN_TILT_ANGLE_POS;

		/* Get word to calculate beta accel mean */
		lsb = ((uint16_t)tilt_config[idx++]);
		msb = ((uint16_t)tilt_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get beta accel mean */
		config->beta_acc_mean = lsb_msb & BMI3_TILT_BETA_ACC_MEAN_MASK;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets tilt configurations like segment size,
 * tilt angle, beta accel mean.
 */
static int8_t set_tilt_config(const struct bmi3_tilt_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to set the base address of tilt feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_TILT, 0 };

    /* Array to define the feature configuration */
    uint8_t tilt_config[4] = { 0 };

    uint8_t data_array[4] = { 0 };

    uint16_t min_tilt_angle1;

    uint16_t beta_acc_mean1, beta_acc_mean2;

    uint16_t min_tilt_angle, beta_acc_mean;

    uint16_t segment_size;

    if (config != NULL)
    {
	/* Set the tilt base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set segment size */
	    segment_size = BMI3_SET_BIT_POS0(data_array[0], BMI3_TILT_SEGMENT_SIZE, config->segment_size);

	    min_tilt_angle1 = ((uint16_t)data_array[1] << 8);

	    /* Set minimum tilt angle */
	    min_tilt_angle = BMI3_SET_BITS(min_tilt_angle1, BMI3_TILT_MIN_TILT_ANGLE, config->min_tilt_angle);

	    /* Set beta accel mean for lsb 8 bits */
	    beta_acc_mean1 = BMI3_SET_BIT_POS0(data_array[2], BMI3_TILT_BETA_ACC_MEAN, config->beta_acc_mean);

	    beta_acc_mean = ((uint16_t)data_array[3] << 8);

	    /* Set beta accel mean for msb 8 bits */
	    beta_acc_mean2 = BMI3_SET_BIT_POS0(beta_acc_mean, BMI3_TILT_BETA_ACC_MEAN, config->beta_acc_mean);

	    tilt_config[0] = (uint8_t)segment_size;
	    tilt_config[1] = (uint8_t)(min_tilt_angle >> 8);
	    tilt_config[2] = (uint8_t)(beta_acc_mean1);
	    tilt_config[3] = (uint8_t)(beta_acc_mean2 >> 8);

	    /* Set the configuration back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, tilt_config, 4, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets orientation configurations like upside enable,
 * mode, blocking, theta, hold time, slope threshold and hysteresis.
 */
static int8_t get_orientation_config(struct bmi3_orientation_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t orient_config[4] = { 0 };

    /* Array to set the base address of orient feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ORIENT, 0 };

    /* Variable to define the array offset */
    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    if (config != NULL)
    {
	/* Set the orient base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where orientation feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, orient_config, 4, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate upside down enable, mode, blocking, theta and hold time
		 * from the same word */
		lsb = (uint16_t) orient_config[idx++];
		msb = ((uint16_t) orient_config[idx++] << 8);
		lsb_msb = lsb | msb;

		/* Get upside enable */
		config->ud_en = lsb_msb & BMI3_ORIENT_UD_EN_MASK;

		/* Get mode */
		config->mode = (lsb_msb & BMI3_ORIENT_MODE_MASK) >> BMI3_ORIENT_MODE_POS;

		/* Get blocking */
		config->blocking = (lsb_msb & BMI3_ORIENT_BLOCKING_MASK) >> BMI3_ORIENT_BLOCKING_POS;

		/* Get theta */
		config->theta = (lsb_msb & BMI3_ORIENT_THETA_MASK) >> BMI3_ORIENT_THETA_POS;

		/* Get hold time */
		config->hold_time = (lsb_msb & BMI3_ORIENT_HOLD_TIME_MASK) >> BMI3_ORIENT_HOLD_TIME_POS;

		/* Get word to calculate slope threshold and hysteresis from the same word */
		lsb = (uint16_t) orient_config[idx++];
		msb = ((uint16_t) orient_config[idx++] << 8);
		lsb_msb = lsb | msb;

		/* Get slope threshold */
		config->slope_thres = lsb_msb & BMI3_ORIENT_SLOPE_THRES_MASK;

		/* Get hysteresis */
		config->hysteresis = (lsb_msb & BMI3_ORIENT_HYST_MASK) >> BMI3_ORIENT_HYST_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets orientation configurations like upside enable,
 * mode, blocking, theta, hold time, slope threshold and hysteresis.
 */
static int8_t set_orientation_config(const struct bmi3_orientation_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t orient_config[4] = { 0 };

    /* Array to set the base address of orient feature */
    uint8_t base_aadr[2] = { BMI3_BASE_ADDR_ORIENT, 0 };

    uint16_t ud_en, mode, blocking, theta, theta1, holdtime, slope_thres, hyst;

    if (config != NULL)
    {
	/* Set the orient base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_aadr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set upside down bit */
	    ud_en = BMI3_SET_BIT_POS0(orient_config[0], BMI3_ORIENT_UD_EN, config->ud_en);

	    /* Set mode */
	    mode = BMI3_SET_BITS(orient_config[0], BMI3_ORIENT_MODE, config->mode);

	    /* Set blocking */
	    blocking = BMI3_SET_BITS(orient_config[0], BMI3_ORIENT_BLOCKING, config->blocking);

	    /* Set theta for lsb 8 bits */
	    theta1 = BMI3_SET_BITS(orient_config[0], BMI3_ORIENT_THETA, config->theta);

	    theta = ((uint16_t)orient_config[1] << 8);

	    /* Set theta for msb 8 bits */
	    theta = BMI3_SET_BITS(theta, BMI3_ORIENT_THETA, config->theta);

	    /* Set hold time */
	    holdtime = BMI3_SET_BITS(orient_config[1], BMI3_ORIENT_HOLD_TIME, config->hold_time);

	    /* Set slope threshold */
	    slope_thres = BMI3_SET_BIT_POS0(orient_config[2], BMI3_ORIENT_SLOPE_THRES, config->slope_thres);

	    /* Set hysteresis */
	    hyst = BMI3_SET_BITS(orient_config[3], BMI3_ORIENT_HYST, config->hysteresis);

	    orient_config[0] = (uint8_t)(ud_en | mode | blocking | theta1);
	    orient_config[1] = (uint8_t)((theta | holdtime) >> 8);
	    orient_config[2] = (uint8_t)(slope_thres);
	    orient_config[3] = (uint8_t)(hyst >> 8);

	    /* Set the configuration back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, orient_config, 4, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets step counter configurations like water-mark level,
 * reset counter, step counter parameters and sc_12_res.
 */
static int8_t get_step_config(struct bmi3_step_counter_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t step_config[24] = { 0 };

    /* Array to set the base address of step counter feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_STEP_CNT, 0 };

    /* Variable to define array offset */
    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define word */
    uint16_t lsb_msb;

    if (config != NULL)
    {
	/* Set the step counter base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where step counter feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, step_config, 24, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate water-mark level, reset counter from the same word */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get water-mark level */
		config->watermark_level = lsb_msb & BMI3_STEP_WATERMARK_MASK;

		/* Get reset counter */
		config->reset_counter = (lsb_msb & BMI3_STEP_RESET_COUNTER_MASK) >> BMI3_STEP_RESET_COUNTER_POS;

		/* Get word to calculate minimum distance up */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get minimum distance up */
		config->env_min_dist_up = (lsb_msb & BMI3_STEP_ENV_MIN_DIST_UP_MASK);

		/* Get word to calculate env coefficient up */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get env coefficient up */
		config->env_coef_up = (lsb_msb & BMI3_STEP_ENV_COEF_UP_MASK);

		/* Get word to calculate env minimum distance down */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get env minimum distance down */
		config->env_min_dist_down = (lsb_msb & BMI3_STEP_ENV_MIN_DIST_DOWN_MASK);

		/* Get word to calculate env coefficient down */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get env coefficient down */
		config->env_coef_down = (lsb_msb & BMI3_STEP_ENV_COEF_DOWN_MASK);

		/* Get word to calculate mean val decay */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get mean val decay */
		config->mean_val_decay = (lsb_msb & BMI3_STEP_MEAN_VAL_DECAY_MASK);

		/* Get word to calculate mean step duration */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get mean step duration */
		config->mean_step_dur = (lsb_msb & BMI3_STEP_MEAN_STEP_DUR_MASK);

		/* Get word to calculate step buffer size, filter cascade enabled and step counter increment
		 * from the same word */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get step buffer size */
		config->step_buffer_size = lsb_msb & BMI3_STEP_BUFFER_SIZE_MASK;

		/* Get filter cascade enable */
		config->filter_cascade_enabled = (lsb_msb & BMI3_STEP_FILTER_CASCADE_ENABLED_MASK) >>
						 BMI3_STEP_FILTER_CASCADE_ENABLED_POS;

		/* Get step counter increment */
		config->step_counter_increment = (lsb_msb & BMI3_STEP_COUNTER_INCREMENT_MASK) >>
						 BMI3_STEP_COUNTER_INCREMENT_POS;

		/* Get word to calculate peak duration minimum walking and peak duration minimum running */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get peak duration minimum walking */
		config->peak_duration_min_walking = lsb_msb & BMI3_STEP_PEAK_DURATION_MIN_WALKING_MASK;

		/* Get peak duration minimum running */
		config->peak_duration_min_running = (lsb_msb & BMI3_STEP_PEAK_DURATION_MIN_RUNNING_MASK) >>
						    BMI3_STEP_PEAK_DURATION_MIN_RUNNING_POS;

		/* Get word to calculate activity detection factor and activity detection threshold
		 * from the same word */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get activity detection factor */
		config->activity_detection_factor = lsb_msb & BMI3_STEP_ACTIVITY_DETECTION_FACTOR_MASK;

		/* Get activity detection threshold */
		config->activity_detection_thres = (lsb_msb & BMI3_STEP_ACTIVITY_DETECTION_THRESHOLD_MASK) >>
						   BMI3_STEP_ACTIVITY_DETECTION_THRESHOLD_POS;

		/* Get word to calculate step duration max and step duration window from the same word */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get step duration max */
		config->step_duration_max = lsb_msb & BMI3_STEP_DURATION_MAX_MASK;

		/* Get step duration window */
		config->step_duration_window = (lsb_msb & BMI3_STEP_DURATION_WINDOW_MASK) >>
					       BMI3_STEP_DURATION_WINDOW_POS;

		/* Get word to calculate step duration pp enabled, duration threshold,
		 * mean crossing pp enabled, mcr threshold, sc_12_res from the same word */
		lsb = ((uint16_t)step_config[idx++]);
		msb = ((uint16_t)step_config[idx++]);
		lsb_msb = (uint16_t)(lsb | (msb << 8));

		/* Get step duration pp enable */
		config->step_duration_pp_enabled = lsb_msb & BMI3_STEP_DURATION_PP_ENABLED_MASK;

		/* Get step duration threshold */
		config->step_duration_thres = (lsb_msb & BMI3_STEP_DURATION_THRESHOLD_MASK) >>
					      BMI3_STEP_DURATION_THRESHOLD_POS;

		/* Get mean crossing pp enabled */
		config->mean_crossing_pp_enabled = (lsb_msb & BMI3_STEP_MEAN_CROSSING_PP_ENABLED_MASK) >>
						   BMI3_STEP_MEAN_CROSSING_PP_ENABLED_POS;

		/* Get mcr threshold */
		config->mcr_threshold = (lsb_msb & BMI3_STEP_MCR_THRESHOLD_MASK) >> BMI3_STEP_MCR_THRESHOLD_POS;

		/* Get sc_12_res selection */
		config->sc_12_res = (lsb_msb & BMI3_STEP_SC_12_RES_MASK) >> BMI3_STEP_SC_12_RES_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_INVALID_SENSOR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets step counter configurations like water-mark level,
 * reset counter, step counter parameters and sc_12_res.
 */
static int8_t set_step_config(const struct bmi3_step_counter_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to set the base address of step counter feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_STEP_CNT, 0 };

    /* Array to define the feature configuration */
    uint8_t step_config[24] = { 0 };

    uint8_t data_array[24] = { 0 };

    uint16_t watermark1, watermark2, activity_detection_threshold1, activity_detection_threshold2;

    uint16_t env_min_dist_up1, env_min_dist_up2, env_coef_down1, env_coef_down2;

    uint16_t env_coef_up1, env_coef_up2, mean_val_decay1, mean_val_decay2, step_counter_increment1,
	     step_counter_increment2;

    uint16_t mean_step_dur1, mean_step_dur2, env_min_dist_down1, env_min_dist_down2;

    uint16_t step_buffer_size, filter_cascade_enabled, peak_duration_min_walking, peak_duration_min_running;

    uint16_t watermark, env_min_dist_up, env_coef_up, env_min_dist_down, env_coef_down, mean_val_decay;

    uint16_t mean_step_dur, activity_detection_threshold, mcr_threshold;

    uint16_t reset_counter, activity_detection_factor, step_duration_max, step_duration_window,
	     step_duration_pp_enabled;

    uint16_t step_duration_threshold, mean_crossing_pp_enabled, mcr_threshold1, mcr_threshold2, sc_12_res;

    if (config != NULL)
    {
	/* Set the step counter base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set water-mark for lsb 8 bits */
	    watermark1 = BMI3_SET_BIT_POS0(data_array[0], BMI3_STEP_WATERMARK, config->watermark_level);

	    watermark = ((uint16_t)data_array[1] << 8);

	    /* Set water-mark for msb 8 bits */
	    watermark2 = BMI3_SET_BIT_POS0(watermark, BMI3_STEP_WATERMARK, config->watermark_level);

	    reset_counter = ((uint16_t)data_array[1] << 8);

	    /* Set reset counter */
	    reset_counter = BMI3_SET_BITS(reset_counter, BMI3_STEP_RESET_COUNTER, config->reset_counter);

	    /* Set env_min_dist_up for lsb 8 bits */
	    env_min_dist_up1 = BMI3_SET_BIT_POS0(data_array[2], BMI3_STEP_ENV_MIN_DIST_UP, config->env_min_dist_up);

	    env_min_dist_up = ((uint16_t)data_array[3] << 8);

	    /* Set env_min_dist_up for msb 8 bits */
	    env_min_dist_up2 = BMI3_SET_BIT_POS0(env_min_dist_up, BMI3_STEP_ENV_MIN_DIST_UP, config->env_min_dist_up);

	    /* Set env_coef_up for lsb 8 bits */
	    env_coef_up1 = BMI3_SET_BIT_POS0(data_array[4], BMI3_STEP_ENV_COEF_UP, config->env_coef_up);

	    env_coef_up = ((uint16_t)data_array[5] << 8);

	    /* Set env_coef_up for msb 8 bits */
	    env_coef_up2 = BMI3_SET_BIT_POS0(env_coef_up, BMI3_STEP_ENV_COEF_UP, config->env_coef_up);

	    /* Set env_min_dist_down for lsb 8 bits */
	    env_min_dist_down1 =
		BMI3_SET_BIT_POS0(data_array[6], BMI3_STEP_ENV_MIN_DIST_DOWN, config->env_min_dist_down);

	    env_min_dist_down = ((uint16_t)data_array[7] << 8);

	    /* Set env_min_dist_down for msb 8 bits */
	    env_min_dist_down2 = BMI3_SET_BIT_POS0(env_min_dist_down,
						   BMI3_STEP_ENV_MIN_DIST_DOWN,
						   config->env_min_dist_down);

	    /* Set env_coef_down for lsb 8 bits */
	    env_coef_down1 = BMI3_SET_BIT_POS0(data_array[8], BMI3_STEP_ENV_COEF_DOWN, config->env_coef_down);

	    env_coef_down = ((uint16_t)data_array[9] << 8);

	    /* Set env_coef_down for msb 8 bits */
	    env_coef_down2 = BMI3_SET_BIT_POS0(env_coef_down, BMI3_STEP_ENV_COEF_DOWN, config->env_coef_down);

	    /* Set mean_val_decay for lsb 8 bits */
	    mean_val_decay1 = BMI3_SET_BIT_POS0(data_array[10], BMI3_STEP_MEAN_VAL_DECAY, config->mean_val_decay);

	    mean_val_decay = ((uint16_t)data_array[11] << 8);

	    /* Set mean_val_decay for msb 8 bits */
	    mean_val_decay2 = BMI3_SET_BIT_POS0(mean_val_decay, BMI3_STEP_MEAN_VAL_DECAY, config->mean_val_decay);

	    /* Set mean_step_dur for lsb 8 bits */
	    mean_step_dur1 = BMI3_SET_BIT_POS0(data_array[12], BMI3_STEP_MEAN_STEP_DUR, config->mean_step_dur);

	    mean_step_dur = ((uint16_t)data_array[13] << 8);

	    /* Set mean_step_dur for msb 8 bits */
	    mean_step_dur2 = BMI3_SET_BIT_POS0(mean_step_dur, BMI3_STEP_MEAN_STEP_DUR, config->mean_step_dur);

	    /* Set step buffer size */
	    step_buffer_size = BMI3_SET_BIT_POS0(data_array[14], BMI3_STEP_BUFFER_SIZE, config->step_buffer_size);

	    /* Set filter cascade */
	    filter_cascade_enabled = BMI3_SET_BITS(data_array[14],
						   BMI3_STEP_FILTER_CASCADE_ENABLED,
						   config->filter_cascade_enabled);

	    /* Set step_counter_increment for lsb 8 bits */
	    step_counter_increment1 = BMI3_SET_BITS(data_array[14],
						    BMI3_STEP_COUNTER_INCREMENT,
						    config->step_counter_increment);

	    step_counter_increment2 = ((uint16_t)data_array[15] << 8);

	    /* Set step_counter_increment for msb 8 bits */
	    step_counter_increment2 = BMI3_SET_BITS(step_counter_increment2,
						    BMI3_STEP_COUNTER_INCREMENT,
						    config->step_counter_increment);

	    /* Set peak_duration_min_walking for lsb 8 bits */
	    peak_duration_min_walking = BMI3_SET_BIT_POS0(data_array[16],
							  BMI3_STEP_PEAK_DURATION_MIN_WALKING,
							  config->peak_duration_min_walking);

	    peak_duration_min_running = ((uint16_t)data_array[17] << 8);

	    /* Set peak_duration_min_walking for msb 8 bits */
	    peak_duration_min_running = BMI3_SET_BITS(peak_duration_min_running,
						      BMI3_STEP_PEAK_DURATION_MIN_RUNNING,
						      config->peak_duration_min_running);

	    /* Set activity detection fsctor */
	    activity_detection_factor = BMI3_SET_BIT_POS0(data_array[18],
							  BMI3_STEP_ACTIVITY_DETECTION_FACTOR,
							  config->activity_detection_factor);

	    /* Set activity_detection_threshold for lsb 8 bits */
	    activity_detection_threshold1 = BMI3_SET_BITS(data_array[18],
							  BMI3_STEP_ACTIVITY_DETECTION_THRESHOLD,
							  config->activity_detection_thres);

	    activity_detection_threshold = ((uint16_t)data_array[19] << 8);

	    /* Set activity_detection_threshold for msb 8 bits */
	    activity_detection_threshold2 = BMI3_SET_BITS(activity_detection_threshold,
							  BMI3_STEP_ACTIVITY_DETECTION_THRESHOLD,
							  config->activity_detection_thres);

	    /* Set maximum step duration */
	    step_duration_max = BMI3_SET_BIT_POS0(data_array[20], BMI3_STEP_DURATION_MAX, config->step_duration_max);

	    step_duration_window = ((uint16_t)data_array[21] << 8);

	    /* Set step duration window */
	    step_duration_window = BMI3_SET_BITS(step_duration_window,
						 BMI3_STEP_DURATION_WINDOW,
						 config->step_duration_window);

	    step_duration_pp_enabled = BMI3_SET_BIT_POS0(data_array[22],
							 BMI3_STEP_DURATION_PP_ENABLED,
							 config->step_duration_pp_enabled);

	    step_duration_threshold = BMI3_SET_BITS(data_array[22],
						    BMI3_STEP_DURATION_THRESHOLD,
						    config->step_duration_thres);

	    mean_crossing_pp_enabled = BMI3_SET_BITS(data_array[22],
						     BMI3_STEP_MEAN_CROSSING_PP_ENABLED,
						     config->mean_crossing_pp_enabled);

	    /* Set mcr_threshold for lsb 8 bits */
	    mcr_threshold1 = BMI3_SET_BITS(data_array[22], BMI3_STEP_MCR_THRESHOLD, config->mcr_threshold);

	    mcr_threshold = ((uint16_t)data_array[23] << 8);

	    /* Set mcr_threshold for msb 8 bits */
	    mcr_threshold2 = BMI3_SET_BITS(mcr_threshold, BMI3_STEP_MCR_THRESHOLD, config->mcr_threshold);

	    sc_12_res = ((uint16_t)data_array[23] << 8);

	    sc_12_res = BMI3_SET_BITS(sc_12_res, BMI3_STEP_SC_12_RES, config->sc_12_res);

	    step_config[0] = (uint8_t)watermark1;
	    step_config[1] = (uint8_t)((watermark2 | reset_counter) >> 8);
	    step_config[2] = (uint8_t)env_min_dist_up1;
	    step_config[3] = (uint8_t)(env_min_dist_up2 >> 8);
	    step_config[4] = (uint8_t)env_coef_up1;
	    step_config[5] = (uint8_t)(env_coef_up2 >> 8);
	    step_config[6] = (uint8_t)env_min_dist_down1;
	    step_config[7] = (uint8_t)(env_min_dist_down2 >> 8);
	    step_config[8] = (uint8_t)env_coef_down1;
	    step_config[9] = (uint8_t)(env_coef_down2 >> 8);
	    step_config[10] = (uint8_t)mean_val_decay1;
	    step_config[11] = (uint8_t)(mean_val_decay2 >> 8);
	    step_config[12] = (uint8_t)mean_step_dur1;
	    step_config[13] = (uint8_t)(mean_step_dur2 >> 8);
	    step_config[14] = (uint8_t)(step_buffer_size | filter_cascade_enabled | step_counter_increment1);
	    step_config[15] = (uint8_t)(step_counter_increment2 >> 8);
	    step_config[16] = (uint8_t)peak_duration_min_walking;
	    step_config[17] = (uint8_t)(peak_duration_min_running >> 8);
	    step_config[18] = (uint8_t)(activity_detection_factor | activity_detection_threshold1);
	    step_config[19] = (uint8_t)(activity_detection_threshold2 >> 8);
	    step_config[20] = (uint8_t)step_duration_max;
	    step_config[21] = (uint8_t)(step_duration_window >> 8);
	    step_config[22] =
		(uint8_t)(step_duration_pp_enabled | step_duration_threshold | mean_crossing_pp_enabled |
			  mcr_threshold1);
	    step_config[23] = (uint8_t)((mcr_threshold2 | sc_12_res) >> 8);

	    /* Set the configuration back to feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, step_config, 24, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets tap configurations like axes select, wait for time out, mode,
 * max peaks for tap, duration, tap peak threshold, max gest duration, max dur bw peaks,
 * shock settling duration.
 */
static int8_t get_tap_config(struct bmi3_tap_detector_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t tap_config[6] = { 0 };

    /* Array to set the base address of tap feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_TAP, 0 };

    /* Variable to define array offset */
    uint8_t idx = 0;

    /* Variable to define LSB */
    uint16_t lsb;

    /* Variable to define MSB */
    uint16_t msb;

    /* Variable to define a word */
    uint16_t lsb_msb;

    if (config != NULL)
    {
	/* Set the tap base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where tap feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, tap_config, 6, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get word to calculate axis select, wait for time out, max peaks for tap and mode
		 * from the same word */
		lsb = (uint16_t) tap_config[idx++];
		msb = ((uint16_t) tap_config[idx++] << 8);
		lsb_msb = lsb | msb;

		/* Get axis selection */
		config->axis_sel = lsb_msb & BMI3_TAP_AXIS_SEL_MASK;

		/* Get wait for time out */
		config->wait_for_timeout = (lsb_msb & BMI3_TAP_WAIT_FR_TIME_OUT_MASK) >> BMI3_TAP_WAIT_FR_TIME_OUT_POS;

		/* Get max peaks for tap */
		config->max_peaks_for_tap = (lsb_msb & BMI3_TAP_MAX_PEAKS_MASK) >> BMI3_TAP_MAX_PEAKS_POS;

		/* Get mode */
		config->mode = (lsb_msb & BMI3_TAP_MODE_MASK) >> BMI3_TAP_MODE_POS;

		/* Get word to calculate threshold, output configuration from the same word */
		lsb = (uint16_t) tap_config[idx++];
		msb = ((uint16_t) tap_config[idx++] << 8);
		lsb_msb = lsb | msb;

		/* Get tap peak threshold */
		config->tap_peak_thres = lsb_msb & BMI3_TAP_PEAK_THRES_MASK;

		/* Get max gesture duration */
		config->max_gest_dur = (lsb_msb & BMI3_TAP_MAX_GEST_DUR_MASK) >> BMI3_TAP_MAX_GEST_DUR_POS;

		/* Get word to calculate max_dur_between_peaks, tap_shock_settling_dur, min_quite_dur_between_taps
		 *  and quite_time_after_gest from the same word */
		lsb = (uint16_t) tap_config[idx++];
		msb = ((uint16_t) tap_config[idx++] << 8);
		lsb_msb = lsb | msb;

		/* Get maximum duration between peaks */
		config->max_dur_between_peaks = lsb_msb & BMI3_TAP_MAX_DUR_BW_PEAKS_MASK;

		/* Get tap shock settling duration */
		config->tap_shock_settling_dur = (lsb_msb & BMI3_TAP_SHOCK_SETT_DUR_MASK) >>
						 BMI3_TAP_SHOCK_SETT_DUR_POS;

		/* Get minimum quite duration between taps */
		config->min_quite_dur_between_taps = (lsb_msb & BMI3_TAP_MIN_QUITE_DUR_BW_TAPS_MASK) >>
						     BMI3_TAP_MIN_QUITE_DUR_BW_TAPS_POS;

		/* Get quite time after gesture */
		config->quite_time_after_gest = (lsb_msb & BMI3_TAP_QUITE_TIME_AFTR_GEST_MASK) >>
						BMI3_TAP_QUITE_TIME_AFTR_GEST_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets tap configurations like axes select, wait for time out, mode,
 * max peaks for tap, duration, tap peak threshold, max gest duration, max dur bw peaks,
 * shock settling duration.
 */
static int8_t set_tap_config(const struct bmi3_tap_detector_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the feature configuration */
    uint8_t tap_config[6] = { 0 };

    /* Array to set the base address of tap feature */
    uint8_t base_aadr[2] = { BMI3_BASE_ADDR_TAP, 0 };

    uint16_t axis_sel, wait_fr_time_out, max_peaks_for_tap, mode;
    uint16_t tap_peak_thres, tap_peak_thres1, tap_peak_thres2, max_gest_dur;
    uint16_t max_dur_between_peaks, tap_shock_setting_dur, min_quite_dur_between_taps, quite_time_after_gest;

    if (config != NULL)
    {
	/* Set the tap base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_aadr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set axis_sel */
	    axis_sel = BMI3_SET_BIT_POS0(tap_config[0], BMI3_TAP_AXIS_SEL, config->axis_sel);

	    /* Set wait for time out */
	    wait_fr_time_out = BMI3_SET_BITS(tap_config[0], BMI3_TAP_WAIT_FR_TIME_OUT, config->wait_for_timeout);

	    /* Set maximum peaks for tap */
	    max_peaks_for_tap = BMI3_SET_BITS(tap_config[0], BMI3_TAP_MAX_PEAKS, config->max_peaks_for_tap);

	    /* Set mode */
	    mode = BMI3_SET_BITS(tap_config[0], BMI3_TAP_MODE, config->mode);

	    /* Set peak threshold first byte in word */
	    tap_peak_thres = BMI3_SET_BIT_POS0(tap_config[2], BMI3_TAP_PEAK_THRES, config->tap_peak_thres);

	    /* Left shift by 8 times so that we can set rest of the values of tap peak threshold conf in word */
	    tap_peak_thres1 = ((uint16_t)tap_config[3] << 8);

	    /* Set peak threshold second byte in word */
	    tap_peak_thres2 = BMI3_SET_BIT_POS0(tap_peak_thres1, BMI3_TAP_PEAK_THRES, config->tap_peak_thres);

	    max_gest_dur = ((uint16_t)tap_config[3] << 8);

	    /* Set max gesture duration */
	    max_gest_dur = BMI3_SET_BITS(max_gest_dur, BMI3_TAP_MAX_GEST_DUR, config->max_gest_dur);

	    /* Set max duration between peaks */
	    max_dur_between_peaks = BMI3_SET_BIT_POS0(tap_config[4],
						      BMI3_TAP_MAX_DUR_BW_PEAKS,
						      config->max_dur_between_peaks);

	    /* Set shock settling duration */
	    tap_shock_setting_dur =
		BMI3_SET_BITS(tap_config[4], BMI3_TAP_SHOCK_SETT_DUR, config->tap_shock_settling_dur);

	    min_quite_dur_between_taps = ((uint16_t)tap_config[5] << 8);

	    /* Set quite duration between taps */
	    min_quite_dur_between_taps = BMI3_SET_BITS(min_quite_dur_between_taps,
						       BMI3_TAP_MIN_QUITE_DUR_BW_TAPS,
						       config->min_quite_dur_between_taps);

	    quite_time_after_gest = ((uint16_t)tap_config[5] << 8);

	    /* Set quite time after gesture */
	    quite_time_after_gest = BMI3_SET_BITS(quite_time_after_gest,
						  BMI3_TAP_QUITE_TIME_AFTR_GEST,
						  config->quite_time_after_gest);

	    /* Copy all the configurations back to the tap configuration array */
	    tap_config[0] = (uint8_t)(axis_sel | wait_fr_time_out | max_peaks_for_tap | mode);
	    tap_config[2] = (uint8_t)(tap_peak_thres);
	    tap_config[3] = (uint8_t)((tap_peak_thres2 | max_gest_dur) >> 8);
	    tap_config[4] = (uint8_t)(max_dur_between_peaks | tap_shock_setting_dur);
	    tap_config[5] = (uint8_t)((min_quite_dur_between_taps | quite_time_after_gest) >> 8);

	    /* Set the configuration back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, tap_config, 6, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API is used to update data index based on the frame length
 */
static void update_data_index(uint16_t *idx, uint16_t *updated_frm_idx, uint8_t len, int8_t rslt)
{
    /* Update data index */
    (*idx) = (*idx) + len;

    if (rslt == 0 || rslt == BMI3_W_ST_PARTIAL_READ)
    {
	/* Update accelerometer, gyro and temperature frame index */
	(*updated_frm_idx)++;
    }
}

/*!
 * @brief This internal API is used to parse the accelerometer, gyroscope and temperature data from the
 * FIFO data. It updates the current data byte to be parsed.
 */
static int8_t unpack_fifo_data_frame(void *fifo_data,
				     uint16_t *idx,
				     uint16_t *updated_frm_idx,
				     uint16_t frame,
				     const struct bmi3_fifo_frame *fifo)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_OK;

    /* Variable to store fifo available length in bytes */
    uint16_t fifo_available_len_in_bytes;

    struct bmi3_fifo_temperature_data *fifo_temp_data = (struct bmi3_fifo_temperature_data *)fifo_data;
    struct bmi3_fifo_sens_axes_data *fifo_gyr_data = (struct bmi3_fifo_sens_axes_data *)fifo_data;
    struct bmi3_fifo_sens_axes_data *fifo_acc_data = (struct bmi3_fifo_sens_axes_data *)fifo_data;

    /* Convert available fifo length from word to byte */
    fifo_available_len_in_bytes = (uint16_t)(fifo->available_fifo_len * 2);

    switch (frame)
    {
	/* If frame contains only accelerometer data */
	case BMI3_FIFO_HEAD_LESS_ACC_FRM:

	    /* Get the accelerometer data */
	    rslt = unpack_accel_data(&fifo_acc_data[(*updated_frm_idx)], *idx, fifo);

	    if (rslt == 0 || rslt == BMI3_W_FIFO_ACCEL_DUMMY_FRAME || rslt == BMI3_W_ST_PARTIAL_READ)
	    {
		update_data_index(idx, updated_frm_idx, BMI3_LENGTH_FIFO_ACC, rslt);
            } else if (rslt == BMI3_W_PARTIAL_READ)
	    {
		/* Since partial occur making data start index to FIFO available length
		 * to end of the loop */
		*idx = fifo_available_len_in_bytes;
	    }

	    break;

	/* If frame contains gyro frame */
	case BMI3_FIFO_HEAD_LESS_GYR_FRM:

	    /* Get the gyro data */
	    rslt = unpack_gyro_data(&fifo_gyr_data[(*updated_frm_idx)], *idx, fifo);

	    if (rslt == 0 || rslt == BMI3_W_FIFO_GYRO_DUMMY_FRAME || rslt == BMI3_W_ST_PARTIAL_READ)
	    {
		update_data_index(idx, updated_frm_idx, BMI3_LENGTH_FIFO_GYR, rslt);
            } else if (rslt == BMI3_W_PARTIAL_READ)
	    {
		/* Since partial occur making data start index to FIFO available length
		 * to end of the loop */
		*idx = fifo_available_len_in_bytes;
	    }

	    break;

	/* If frame contains temperature frame */
	case BMI3_FIFO_HEAD_LESS_TEMP_FRM:

	    /* Get the temperature data */
	    rslt = unpack_temperature_data(&fifo_temp_data[(*updated_frm_idx)], *idx, fifo);

	    if (rslt == BMI3_OK || rslt == BMI3_W_FIFO_TEMP_DUMMY_FRAME || rslt == BMI3_W_ST_PARTIAL_READ)
	    {
		update_data_index(idx, updated_frm_idx, BMI3_LENGTH_TEMPERATURE, rslt);
            } else if (rslt == BMI3_W_PARTIAL_READ)
	    {
		/* Since partial occur making data start index to FIFO available length
		 * to end of the loop */
		*idx = fifo_available_len_in_bytes;
	    }

	    break;

	default:
	    rslt = BMI3_W_FIFO_INVALID_FRAME;

	    break;
    }

    return rslt;
}

/*!
 * @brief This internal API is used to parse accelerometer data from the
 * FIFO data.
 */
static int8_t unpack_accel_data(struct bmi3_fifo_sens_axes_data *acc,
				uint16_t data_start_index,
				const struct bmi3_fifo_frame *fifo)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_OK;

    /* Variable to store the sensor time index value */
    uint16_t sens_time_data_idx;

    /* Variable to store LSB value */
    uint16_t data_lsb;

    /* Variables to store MSB value */
    uint16_t data_msb;

    /* Variable to store dummy data value which will get in FIFO data */
    uint16_t dummy_data;

    /* Variable to store fifo available length in bytes */
    uint16_t fifo_available_len_in_bytes;

    /* Convert available fifo length from word to byte */
    fifo_available_len_in_bytes = (uint16_t)(fifo->available_fifo_len * 2);

    if ((data_start_index + BMI3_LENGTH_FIFO_ACC) <= fifo_available_len_in_bytes)
    {
	/* Accelerometer raw x data */
	data_lsb = fifo->data[data_start_index++];
	data_msb = fifo->data[data_start_index++];

	/* To store the dummy data */
	dummy_data = (uint16_t)(((uint16_t)data_msb << 8) | data_lsb);

	if (dummy_data != BMI3_FIFO_ACCEL_DUMMY_FRAME)
	{
	    acc->x = (int16_t)(((uint16_t)data_msb << 8) | data_lsb);

	    /* Accelerometer raw y data */
	    data_lsb = fifo->data[data_start_index++];
	    data_msb = fifo->data[data_start_index++];
	    acc->y = (int16_t)(((uint16_t)data_msb << 8) | data_lsb);

	    /* Accelerometer raw z data */
	    data_lsb = fifo->data[data_start_index++];
	    data_msb = fifo->data[data_start_index++];
	    acc->z = (int16_t)(((uint16_t)data_msb << 8) | data_lsb);
        } else
	{
	    rslt = BMI3_W_FIFO_ACCEL_DUMMY_FRAME;
	}
    } else
    {
	rslt = BMI3_W_PARTIAL_READ;
    }

    sens_time_data_idx = data_start_index;
    acc->sensor_time = 0;

    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_SENS_TIME_FRM)
    {
	if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_GYR_FRM)
	{
	    sens_time_data_idx = sens_time_data_idx + BMI3_LENGTH_FIFO_GYR;
	}

	if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_TEMP_FRM)
	{
	    sens_time_data_idx = sens_time_data_idx + BMI3_LENGTH_TEMPERATURE;
	}

	if ((sens_time_data_idx + BMI3_LENGTH_SENSOR_TIME) <= fifo_available_len_in_bytes)
	{
	    /* Sensor time raw data */
	    data_lsb = fifo->data[sens_time_data_idx++];
	    data_msb = fifo->data[sens_time_data_idx++];

	    acc->sensor_time = (uint16_t)(((uint16_t)data_msb << 8) | data_lsb);
        } else
	{
	    rslt = BMI3_W_ST_PARTIAL_READ;
	}
    }

    return rslt;
}

/*!
 * @brief This internal API is used to parse temperature data from the
 * FIFO data.
 */
static int8_t unpack_temperature_data(struct bmi3_fifo_temperature_data *temp,
				      uint16_t data_start_index,
				      const struct bmi3_fifo_frame *fifo)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_OK;

    /* Variable to store the sensor time index value */
    uint16_t sens_time_data_idx;

    /* Variables to store LSB value */
    uint16_t data_lsb;

    /* Variables to store MSB value */
    uint16_t data_msb;

    /* Variable to store dummy data value which will get in FIFO data */
    uint16_t dummy_data;

    /* Variable to store fifo available length in bytes */
    uint16_t fifo_available_len_in_bytes;

    /* Convert available fifo length from word to byte */
    fifo_available_len_in_bytes = (uint16_t)(fifo->available_fifo_len * 2);

    if ((data_start_index + BMI3_LENGTH_TEMPERATURE) <= fifo_available_len_in_bytes)
    {
	/* Temperature raw data */
	data_lsb = fifo->data[data_start_index++];
	data_msb = fifo->data[data_start_index++];

	/* To store the dummy data */
	dummy_data = (uint16_t)(((uint16_t)data_msb << 8) | data_lsb);

	temp->temp_data = (uint16_t)(((uint16_t)data_msb << 8) | data_lsb);

	if (dummy_data != BMI3_FIFO_TEMP_DUMMY_FRAME)
	{
	    temp->temp_data = (uint16_t)(((uint16_t)data_msb << 8) | data_lsb);
        } else
	{
	    rslt = BMI3_W_FIFO_TEMP_DUMMY_FRAME;
	}
    } else
    {
	rslt = BMI3_W_PARTIAL_READ;
    }

    sens_time_data_idx = data_start_index;
    temp->sensor_time = 0;

    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_SENS_TIME_FRM)
    {
	if ((sens_time_data_idx + BMI3_LENGTH_SENSOR_TIME) <= fifo_available_len_in_bytes)
	{
	    /* Sensor time raw data */
	    data_lsb = fifo->data[sens_time_data_idx++];
	    data_msb = fifo->data[sens_time_data_idx++];
	    temp->sensor_time = (uint16_t)(((uint16_t)data_msb << 8) | data_lsb);
        } else
	{
	    rslt = BMI3_W_ST_PARTIAL_READ;
	}
    }

    return rslt;
}

/*!
 * @brief This internal API is used to parse gyro data from the
 * FIFO data.
 */
static int8_t unpack_gyro_data(struct bmi3_fifo_sens_axes_data *gyro,
			       uint16_t data_start_index,
			       const struct bmi3_fifo_frame *fifo)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_OK;

    /* Variable to store the sensor time index value */
    uint16_t sens_time_data_idx;

    /* Variables to store LSB value */
    uint16_t data_lsb;

    /* Variables to store MSB value */
    uint16_t data_msb;

    /* Variable to store dummy data value which will get in FIFO data */
    uint16_t dummy_data;

    /* Variable to store fifo available length in bytes */
    uint16_t fifo_available_len_in_bytes;

    /* Convert available fifo length from word to byte */
    fifo_available_len_in_bytes = (uint16_t)(fifo->available_fifo_len * 2);

    if ((data_start_index + BMI3_LENGTH_FIFO_GYR) <= fifo_available_len_in_bytes)
    {
	/* Gyro raw x data */
	data_lsb = fifo->data[data_start_index++];
	data_msb = fifo->data[data_start_index++];

	/* To store the dummy data */
	dummy_data = (uint16_t)(((uint16_t)data_msb << 8) | data_lsb);

	gyro->x = (int16_t)(((uint16_t)data_msb << 8) | data_lsb);

	/* Gyro raw y data */
	data_lsb = fifo->data[data_start_index++];
	data_msb = fifo->data[data_start_index++];
	gyro->y = (int16_t)(((uint16_t)data_msb << 8) | data_lsb);

	/* Gyro raw z data */
	data_lsb = fifo->data[data_start_index++];
	data_msb = fifo->data[data_start_index++];
	gyro->z = (int16_t)(((uint16_t)data_msb << 8) | data_lsb);

	if (dummy_data == BMI3_FIFO_GYRO_DUMMY_FRAME)
	{
	    rslt = BMI3_W_FIFO_GYRO_DUMMY_FRAME;
	}
    } else
    {
	rslt = BMI3_W_PARTIAL_READ;
    }

    sens_time_data_idx = data_start_index;
    gyro->sensor_time = 0;

    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_SENS_TIME_FRM)
    {
	if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_TEMP_FRM)
	{
	    sens_time_data_idx = sens_time_data_idx + BMI3_LENGTH_TEMPERATURE;
	}

	if ((sens_time_data_idx + BMI3_LENGTH_SENSOR_TIME) <= fifo_available_len_in_bytes)
	{
	    /* Sensor time raw data */
	    data_lsb = fifo->data[sens_time_data_idx++];
	    data_msb = fifo->data[sens_time_data_idx++];
	    gyro->sensor_time = (uint16_t)(((uint16_t)data_msb << 8) | data_lsb);
        } else
	{
	    rslt = BMI3_W_ST_PARTIAL_READ;
	}
    }

    return rslt;
}

/*!
 * @brief This internal API sets the precondition settings such as alternate accelerometer and
 * gyroscope enable bits, accelerometer mode and output data rate.
 */
static int8_t st_precondition(uint8_t st_select, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_OK;

    /* Structure instance of sensor config */
    struct bmi3_sens_config config;

    config.type = BMI3_ACCEL;

    switch (st_select)
    {
	case BMI3_ST_ACCEL_ONLY:
	    rslt = BMI3_OK;
	    break;
	case BMI3_ST_GYRO_ONLY:
	case BMI3_ST_BOTH_ACC_GYR:
	    rslt = BMI3_OK;
	    config.cfg.acc.acc_mode = BMI3_ACC_MODE_HIGH_PERF;
	    config.cfg.acc.odr = BMI3_ACC_ODR_100HZ;
	    break;
	default:
	    rslt = BMI3_E_INVALID_ST_SELECTION;
	    break;
    }

    if (rslt == BMI3_OK)
    {
	rslt = bmi3_set_sensor_config(&config, 1, dev);
    }

    return rslt;
}

/*!
 * @brief This internal API gets and sets the self-test mode given
 *  by the user in the self-test dma register.
 */
static int8_t get_set_st_dma(uint8_t st_selection, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t reg_data[2];

    /* Array to set the base address of self-test feature */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ST_SELECT, 0 };

    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

    if (rslt == BMI3_OK)
    {
	rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, 2, dev);

	reg_data[0] = st_selection;

	if (rslt == BMI3_OK)
	{
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	    if (rslt == BMI3_OK)
	    {
		rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, 2, dev);
	    }
	}
    }

    return rslt;
}

/*!
 * @brief This internal API sets the self-test preconditions and triggers the self-test in the command register.
 */
static int8_t self_test_conditions(uint8_t st_selection, struct bmi3_dev *dev)
{
    int8_t rslt;

    /* Pre conditions to be checked */
    rslt = st_precondition(st_selection, dev);

    if (rslt == BMI3_OK)
    {
	/* Disable alternate accel and gyro mode */
	rslt = disable_alt_conf_acc_gyr_mode(dev);

	if (rslt == BMI3_OK)
	{
	    rslt = bmi3_set_command_register(BMI3_CMD_SELF_TEST_TRIGGER, dev);
	}
    }

    return rslt;
}

/*!
 * @brief This internal API is used to disable alternate config accel and gyro mode for self-test precondition.
 */
static int8_t disable_alt_conf_acc_gyr_mode(struct bmi3_dev *dev)
{
    int8_t rslt;

    struct bmi3_alt_accel_config alt_acc_cfg;

    struct bmi3_alt_gyro_config alt_gyr_cfg;

    rslt = get_alternate_accel_config(&alt_acc_cfg, dev);

    if (rslt == BMI3_OK)
    {
	alt_acc_cfg.alt_acc_mode = BMI3_DISABLE;

	rslt = set_alternate_accel_config(&alt_acc_cfg, dev);

	if (rslt == BMI3_OK)
	{
	    rslt = get_alternate_gyro_config(&alt_gyr_cfg, dev);

	    if (rslt == BMI3_OK)
	    {
		alt_gyr_cfg.alt_gyro_mode = BMI3_DISABLE;

		rslt = set_alternate_gyro_config(&alt_gyr_cfg, dev);
	    }
	}
    }

    return rslt;
}

/*!
 * @brief This internal API is used to get the status of gyro self-test and the result of the event.
 */
static int8_t get_st_status_rslt(uint8_t st_selection, struct bmi3_st_result *st_result_status, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define data array */
    uint8_t data_array[2];

    /* Variable to define reg data */
    uint8_t reg_data[2];

    /* Variable to store the self-test status if it is ongoing or completed */
    uint8_t st_status;

    /* Variable to define index */
    uint8_t idx;

    /* Variable to define the number of iterations */
    uint8_t limit = 10;

    /* Variable to define feature engine errors */
    uint8_t feature_engine_err_reg_lsb, feature_engine_err_reg_msb;

    /* Array to set the base address of self-test feature */
    uint8_t sc_st_base_addr[2] = { BMI3_BASE_ADDR_ST_RESULT, 0 };

    st_result_status->self_test_err_rslt = 0;

    for (idx = 0; idx < limit; idx++)
    {
	/* A delay of 120ms is required to read the error status register */
	dev->delay_us(120000, dev->intf_ptr);

	rslt = bmi3_get_regs(BMI3_REG_FEATURE_IO1, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    st_status = (data_array[0] & BMI3_SC_ST_STATUS_MASK) >> BMI3_SC_ST_COMPLETE_POS;

	    if (st_status == BMI3_TRUE)
	    {
		st_result_status->self_test_rslt = (data_array[0] & BMI3_ST_RESULT_MASK) >> BMI3_ST_RESULT_POS;

		if (st_result_status->self_test_rslt != BMI3_TRUE)
		{
		    rslt = bmi3_get_feature_engine_error_status(&feature_engine_err_reg_lsb,
								&feature_engine_err_reg_msb,
								dev);
		    st_result_status->self_test_err_rslt = feature_engine_err_reg_lsb & BMI3_SET_LOW_NIBBLE;
                } else
		{
		    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, sc_st_base_addr, 2, dev);

		    if (rslt == BMI3_OK)
		    {
			rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, 2, dev);

			if ((rslt == BMI3_OK) && (st_selection == BMI3_ST_ACCEL_ONLY))
			{
			    st_result_status->acc_sens_x_ok = reg_data[0] & BMI3_ST_ACC_X_OK_MASK;
			    st_result_status->acc_sens_y_ok = (reg_data[0] & BMI3_ST_ACC_Y_OK_MASK) >>
							      BMI3_ST_ACC_Y_OK_POS;
			    st_result_status->acc_sens_z_ok = (reg_data[0] & BMI3_ST_ACC_Z_OK_MASK) >>
							      BMI3_ST_ACC_Z_OK_POS;

			    st_result_status->gyr_sens_x_ok = BMI3_DISABLE;
			    st_result_status->gyr_sens_y_ok = BMI3_DISABLE;
			    st_result_status->gyr_sens_z_ok = BMI3_DISABLE;
			    st_result_status->gyr_drive_ok = BMI3_DISABLE;
			}

			if ((rslt == BMI3_OK) && (st_selection == BMI3_ST_GYRO_ONLY))
			{
			    st_result_status->acc_sens_x_ok = BMI3_DISABLE;
			    st_result_status->acc_sens_y_ok = BMI3_DISABLE;
			    st_result_status->acc_sens_z_ok = BMI3_DISABLE;

			    st_result_status->gyr_sens_x_ok = (reg_data[0] & BMI3_ST_GYR_X_OK_MASK) >>
							      BMI3_ST_GYR_X_OK_POS;
			    st_result_status->gyr_sens_y_ok = (reg_data[0] & BMI3_ST_GYR_Y_OK_MASK) >>
							      BMI3_ST_GYR_Y_OK_POS;
			    st_result_status->gyr_sens_z_ok = (reg_data[0] & BMI3_ST_GYR_Z_OK_MASK) >>
							      BMI3_ST_GYR_Z_OK_POS;
			    st_result_status->gyr_drive_ok = (reg_data[0] & BMI3_ST_GYR_DRIVE_OK_MASK) >>
							     BMI3_ST_GYR_DRIVE_OK_POS;
			}

			if ((rslt == BMI3_OK) && (st_selection == BMI3_ST_BOTH_ACC_GYR))
			{
			    st_result_status->acc_sens_x_ok = reg_data[0] & BMI3_ST_ACC_X_OK_MASK;
			    st_result_status->acc_sens_y_ok = (reg_data[0] & BMI3_ST_ACC_Y_OK_MASK) >>
							      BMI3_ST_ACC_Y_OK_POS;
			    st_result_status->acc_sens_z_ok = (reg_data[0] & BMI3_ST_ACC_Z_OK_MASK) >>
							      BMI3_ST_ACC_Z_OK_POS;

			    st_result_status->gyr_sens_x_ok = (reg_data[0] & BMI3_ST_GYR_X_OK_MASK) >>
							      BMI3_ST_GYR_X_OK_POS;
			    st_result_status->gyr_sens_y_ok = (reg_data[0] & BMI3_ST_GYR_Y_OK_MASK) >>
							      BMI3_ST_GYR_Y_OK_POS;
			    st_result_status->gyr_sens_z_ok = (reg_data[0] & BMI3_ST_GYR_Z_OK_MASK) >>
							      BMI3_ST_GYR_Z_OK_POS;
			    st_result_status->gyr_drive_ok = (reg_data[0] & BMI3_ST_GYR_DRIVE_OK_MASK) >>
							     BMI3_ST_GYR_DRIVE_OK_POS;
			}
		    }
		}

		break;
            } else
	    {
		/* If limit elapses returning the error code, error status is returned */
		rslt = bmi3_get_feature_engine_error_status(&feature_engine_err_reg_lsb,
							    &feature_engine_err_reg_msb,
							    dev);
		st_result_status->self_test_err_rslt = feature_engine_err_reg_lsb;
	    }

	    /* Checking the condition where the error status is no error */
	    if ((st_result_status->self_test_err_rslt & BMI3_SET_LOW_NIBBLE) == BMI3_NO_ERROR_MASK)
	    {
		st_result_status->self_test_err_rslt = BMI3_OK;
	    }
	}
    }

    return rslt;
}

/*!
 * @brief This internal API gets and sets the self-calibration mode given
 *  by the user in the self-calibration dma register.
 */
static int8_t get_set_sc_dma(uint8_t sc_selection, uint8_t apply_corr, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint8_t reg_data[2];

    /* Array to set the base address of self-calibration feature */
    uint8_t sc_base_addr[2] = { BMI3_BASE_ADDR_GYRO_SC_SELECT, 0 };

    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, sc_base_addr, 2, dev);

    if (rslt == BMI3_OK)
    {
	rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, 2, dev);

	/* The value of apply correction is appended with the selection given by the user */
	reg_data[0] = (apply_corr | sc_selection);

	if (rslt == BMI3_OK)
	{
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, sc_base_addr, 2, dev);

	    if (rslt == BMI3_OK)
	    {
		rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, 2, dev);
	    }
	}
    }

    return rslt;
}

/* This internal API is used to get the status of gyro self-calibration and the result of the event */
static int8_t get_sc_gyro_rslt(struct bmi3_self_calib_rslt *sc_rslt, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to define index */
    uint8_t idx;

    /* Variable to define limit */
    uint8_t limit = 25;

    uint8_t data_array[2];

    uint8_t sc_status;

    /* Variable to define feature engine errors */
    uint8_t feature_engine_err_reg_lsb, feature_engine_err_reg_msb;

    sc_rslt->sc_error_rslt = 0;

    for (idx = 0; idx < limit; idx++)
    {
	/* A delay of 120ms is required to read the error status register */
	dev->delay_us(120000, dev->intf_ptr);

	rslt = bmi3_get_regs(BMI3_REG_FEATURE_IO1, data_array, 2, dev);

	sc_status = (data_array[0] & BMI3_SC_ST_STATUS_MASK) >> BMI3_SC_ST_COMPLETE_POS;

	if ((sc_status == BMI3_TRUE) && (rslt == BMI3_OK))
	{
	    sc_rslt->gyro_sc_rslt = (data_array[0] & BMI3_GYRO_SC_RESULT_MASK) >> BMI3_GYRO_SC_RESULT_POS;

	    rslt = bmi3_get_feature_engine_error_status(&feature_engine_err_reg_lsb, &feature_engine_err_reg_msb, dev);
	    sc_rslt->sc_error_rslt = feature_engine_err_reg_lsb;
        } else
	{
	    /* If limit elapses returning the error code, error status is returned */
	    rslt = bmi3_get_feature_engine_error_status(&feature_engine_err_reg_lsb, &feature_engine_err_reg_msb, dev);
	    sc_rslt->sc_error_rslt = feature_engine_err_reg_lsb;
	}
    }

    return rslt;
}

/*!
 * @brief This internal API gets the i3c sync accelerometer data from the register.
 */
static int8_t get_i3c_sync_accel_sensor_data(struct bmi3_i3c_sync_data *data, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define data stored in register */
    uint8_t reg_data[BMI3_NUM_BYTES_I3C_SYNC_ACC] = { 0 };

    /* Array to set the base address of i3c sync accel data */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_I3C_SYNC_ACC, 0 };

    if (data != NULL)
    {
	/* Set the i3c sync accelerometer base address to feature engine transmission address to start DMA transaction
	 * */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where i3c sync accel data resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, BMI3_NUM_BYTES_I3C_SYNC_ACC, dev);

	    data->sync_x = (reg_data[0] | (uint16_t)reg_data[1] << 8);
	    data->sync_y = (reg_data[2] | (uint16_t)reg_data[3] << 8);
	    data->sync_z = (reg_data[4] | (uint16_t)reg_data[5] << 8);
	    data->sync_time = (reg_data[14] | (uint16_t)reg_data[15] << 8);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets the i3c sync gyroscope data from the register.
 */
static int8_t get_i3c_sync_gyro_sensor_data(struct bmi3_i3c_sync_data *data, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define data stored in register */
    uint8_t reg_data[BMI3_NUM_BYTES_I3C_SYNC_GYR] = { 0 };

    /* Array to set the base address of i3c sync gyro data */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_I3C_SYNC_GYR, 0 };

    if (data != NULL)
    {
	/* Set the i3c sync gyroscope base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where i3c sync gyro data resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, BMI3_NUM_BYTES_I3C_SYNC_GYR, dev);

	    if (rslt == BMI3_OK)
	    {
		data->sync_x = (reg_data[0] | (uint16_t)reg_data[1] << 8);
		data->sync_y = (reg_data[2] | (uint16_t)reg_data[3] << 8);
		data->sync_z = (reg_data[4] | (uint16_t)reg_data[5] << 8);
		data->sync_time = (reg_data[8] | (uint16_t)reg_data[9] << 8);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets the i3c sync temperature data from the register.
 */
static int8_t get_i3c_sync_temp_data(struct bmi3_i3c_sync_data *data, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define data stored in register */
    uint8_t reg_data[BMI3_NUM_BYTES_I3C_SYNC_TEMP] = { 0 };

    /* Array to set the base address of i3c sync temperature data */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_I3C_SYNC_TEMP, 0 };

    if (data != NULL)
    {
	/* Set the i3c sync temperature base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where i3c sync temperature data resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, reg_data, BMI3_NUM_BYTES_I3C_SYNC_TEMP, dev);

	    if (rslt == BMI3_OK)
	    {
		data->sync_temp = (reg_data[0] | (uint16_t)reg_data[1] << 8);
		data->sync_time = (reg_data[2] | (uint16_t)reg_data[3] << 8);
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets alternate accelerometer configurations like ODR,
 * accel mode and average number of samples.
 */
static int8_t set_alternate_accel_config(const struct bmi3_alt_accel_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to store data */
    uint8_t reg_data[2] = { 0 };

    uint16_t alt_acc_odr, alt_acc_avg_num, alt_acc_mode;

    if (config != NULL)
    {
	/* Set alternate accelerometer ODR */
	alt_acc_odr = BMI3_SET_BIT_POS0(reg_data[0], BMI3_ALT_ACC_ODR, config->alt_acc_odr);

	/* Set alternate accelerometer average number of samples */
	alt_acc_avg_num = BMI3_SET_BITS(reg_data[1], BMI3_ALT_ACC_AVG_NUM, config->alt_acc_avg_num);

	/* Set alternate accelerometer mode */
	alt_acc_mode = BMI3_SET_BITS(reg_data[1], BMI3_ALT_ACC_MODE, config->alt_acc_mode);

	reg_data[0] = (uint8_t)(alt_acc_odr);
	reg_data[1] = (uint8_t)((alt_acc_avg_num | alt_acc_mode) >> 8);

	/* Set configurations of alternate accel */
	rslt = bmi3_set_regs(BMI3_REG_ALT_ACC_CONF, reg_data, 2, dev);
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets alternate accelerometer configurations like ODR,
 * accel mode and average number of samples.
 */
static int8_t get_alternate_accel_config(struct bmi3_alt_accel_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    uint16_t reg_data;

    if (config != NULL)
    {
	/* Read the sensor configuration details */
	rslt = bmi3_get_regs(BMI3_REG_ALT_ACC_CONF, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    reg_data = data_array[0];

	    /* Get alternate accelerometer ODR */
	    config->alt_acc_odr = BMI3_GET_BIT_POS0(reg_data, BMI3_ALT_ACC_ODR);

	    reg_data = ((uint16_t)data_array[1] << 8);

	    /* Get alternate accelerometer average samples */
	    config->alt_acc_avg_num = BMI3_GET_BITS(reg_data, BMI3_ALT_ACC_AVG_NUM);

	    /* Get alternate accel mode */
	    config->alt_acc_mode = BMI3_GET_BITS(reg_data, BMI3_ALT_ACC_MODE);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets alternate gyro configurations like ODR,
 * gyro mode and average number of samples.
 */
static int8_t set_alternate_gyro_config(const struct bmi3_alt_gyro_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to store data */
    uint8_t reg_data[2] = { 0 };

    uint16_t alt_gyro_odr, alt_gyro_avg_num, alt_gyro_mode;

    if (config != NULL)
    {
	/* Set alternate gyro ODR */
	alt_gyro_odr = BMI3_SET_BIT_POS0(reg_data[0], BMI3_ALT_GYR_ODR, config->alt_gyro_odr);

	/* Set alternate gyro average number of samples */
	alt_gyro_avg_num = BMI3_SET_BITS(reg_data[1], BMI3_ALT_GYR_AVG_NUM, config->alt_gyro_avg_num);

	/* Set alternate gyro mode */
	alt_gyro_mode = BMI3_SET_BITS(reg_data[1], BMI3_ALT_GYR_MODE, config->alt_gyro_mode);

	reg_data[0] = (uint8_t)(alt_gyro_odr);
	reg_data[1] = (uint8_t)((alt_gyro_avg_num | alt_gyro_mode) >> 8);

	/* Set configurations of alternate gyro */
	rslt = bmi3_set_regs(BMI3_REG_ALT_GYR_CONF, reg_data, 2, dev);
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets alternate gyro configurations like ODR,
 * gyro mode and average number of samples.
 */
static int8_t get_alternate_gyro_config(struct bmi3_alt_gyro_config *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to store data */
    uint8_t data_array[2] = { 0 };

    uint16_t reg_data;

    if (config != NULL)
    {
	/* Read the sensor configuration details */
	rslt = bmi3_get_regs(BMI3_REG_ALT_GYR_CONF, data_array, 2, dev);

	if (rslt == BMI3_OK)
	{
	    reg_data = data_array[0];

	    /* Get alternate gyro ODR */
	    config->alt_gyro_odr = BMI3_GET_BIT_POS0(reg_data, BMI3_ALT_GYR_ODR);

	    reg_data = ((uint16_t)data_array[1] << 8);

	    /* Get alternate gyro average samples */
	    config->alt_gyro_avg_num = BMI3_GET_BITS(reg_data, BMI3_ALT_GYR_AVG_NUM);

	    /* Get alternate gyro mode */
	    config->alt_gyro_mode = BMI3_GET_BITS(reg_data, BMI3_ALT_GYR_MODE);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API sets alternate auto configurations for feature interrupts.
 */
static int8_t set_alternate_auto_config(const struct bmi3_auto_config_change *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the alternate auto configuration */
    uint8_t alt_auto_config[2] = { 0 };

    /* Array to set the base address of alternate auto config */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ALT_AUTO_CONFIG, 0 };

    uint8_t alt_switch, user_switch;

    if (config != NULL)
    {
	/* Set the alternate auto config base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Set alternate switch config */
	    alt_switch = BMI3_SET_BIT_POS0(alt_auto_config[0],
					   BMI3_ALT_CONF_ALT_SWITCH,
					   config->alt_conf_alt_switch_src_select);

	    /* Set alternate user config */
	    user_switch = BMI3_SET_BITS(alt_auto_config[0],
					BMI3_ALT_CONF_USER_SWITCH,
					config->alt_conf_user_switch_src_select);

	    alt_auto_config[0] = alt_switch | user_switch;

	    /* Set the configuration back to the feature engine register */
	    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, alt_auto_config, 2, dev);
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API gets alternate auto configurations for feature interrupts.
 */
static int8_t get_alternate_auto_config(struct bmi3_auto_config_change *config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Array to define the alternate auto configuration */
    uint8_t alt_auto_config[2] = { 0 };

    /* Array to set the base address of alternate auto config */
    uint8_t base_addr[2] = { BMI3_BASE_ADDR_ALT_AUTO_CONFIG, 0 };

    if (config != NULL)
    {
	/* Set the alternate auto config base address to feature engine transmission address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Get the configuration from the feature engine register where alternate config feature resides */
	    rslt = bmi3_get_regs(BMI3_REG_FEATURE_DATA_TX, alt_auto_config, 2, dev);

	    if (rslt == BMI3_OK)
	    {
		/* Get alternate switch config */
		config->alt_conf_alt_switch_src_select = alt_auto_config[0] & BMI3_ALT_CONF_ALT_SWITCH_MASK;

		/* Get alternate user config */
		config->alt_conf_user_switch_src_select = (alt_auto_config[0] & BMI3_ALT_CONF_USER_SWITCH_MASK) >>
							  BMI3_ALT_CONF_USER_SWITCH_POS;
	    }
	}
    } else
    {
	rslt = BMI3_E_NULL_PTR;
    }

    return rslt;
}

/*!
 * @brief This internal API is used to monitor the accel power mode during the axis remap.
 */
static int8_t axes_remap_acc_power_mode_status(struct bmi3_sens_config config, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    uint16_t int_status;

    /* Variable to define feature engine errors */
    uint8_t feature_engine_err_reg_lsb, feature_engine_err_reg_msb;

    /* Variable to store the accel power mode during axis remap */
    uint8_t acc_power_mode_status;

    /* Variable to loop */
    uint8_t index;

    /* Variable to store time out value */
    uint8_t time_out = 20;

    /* Delay to read feature engine error status */
    uint16_t axis_remap_delay = 5000;

    config.type = BMI3_ACCEL;

    rslt = bmi3_get_sensor_config(&config, 1, dev);

    if ((rslt == BMI3_OK) && (config.cfg.acc.acc_mode != BMI3_ACC_MODE_DISABLE))
    {
	acc_power_mode_status = config.cfg.acc.acc_mode;

	config.cfg.acc.acc_mode = BMI3_ACC_MODE_DISABLE;

	rslt = bmi3_set_sensor_config(&config, 1, dev);

	if (rslt == BMI3_OK)
	{
	    /* Axis mapping gets updated */
	    rslt = bmi3_set_command_register(BMI3_CMD_AXIS_MAP_UPDATE, dev);

	    for (index = 0; index < time_out; index++)
	    {
		dev->delay_us(axis_remap_delay, dev->intf_ptr);

		rslt = bmi3_get_feature_engine_error_status(&feature_engine_err_reg_lsb,
							    &feature_engine_err_reg_msb,
							    dev);

		if ((feature_engine_err_reg_lsb & BMI3_NO_ERROR_MASK) &&
		    (feature_engine_err_reg_msb & (BMI3_AXIS_MAP_COMPLETE_MASK >> 8)))
		{
		    break;
		}
	    }

	    config.cfg.acc.acc_mode = acc_power_mode_status;

	    if (rslt == BMI3_OK)
	    {
		/* Clearing status registers by reading it, before powering up accelerometer */
		rslt = bmi3_get_int1_status(&int_status, dev);

		if (rslt == BMI3_OK)
		{
		    rslt = bmi3_set_sensor_config(&config, 1, dev);
		}
	    }
	}
    } else if (rslt == BMI3_OK)
    {
	/* Axis mapping gets updated */
	rslt = bmi3_set_command_register(BMI3_CMD_AXIS_MAP_UPDATE, dev);
    }

    return rslt;
}

/*!
 * @brief This internal API verifies and allows only the correct position to do Fast Offset Compensation for
 * accelerometer.
 */
static int8_t verify_foc_position(uint8_t sens_list,
				  const struct bmi3_accel_foc_g_value *accel_g_axis,
				  struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Structure to define accelerometer sensor axes */
    struct bmi3_sens_axes_data avg_foc_data = { 0 };

    /* Structure to store temporary accelerometer values */
    struct bmi3_foc_temp_value temp_foc_data = { 0 };

    rslt = get_average_of_sensor_data(sens_list, &temp_foc_data, dev);

    if (rslt == BMI3_OK)
    {
	if (sens_list == BMI3_ACCEL)
	{
	    /* Taking modulus to make negative values as positive */
	    if ((accel_g_axis->x == 1) && (accel_g_axis->sign == 1))
	    {
		temp_foc_data.x = temp_foc_data.x * BMI3_FOC_INVERT_VALUE;
            } else if ((accel_g_axis->y == 1) && (accel_g_axis->sign == 1))
	    {
		temp_foc_data.y = temp_foc_data.y * BMI3_FOC_INVERT_VALUE;
            } else if ((accel_g_axis->z == 1) && (accel_g_axis->sign == 1))
	    {
		temp_foc_data.z = temp_foc_data.z * BMI3_FOC_INVERT_VALUE;
	    }
	}

	avg_foc_data.x = (int16_t)(temp_foc_data.x);
	avg_foc_data.y = (int16_t)(temp_foc_data.y);
	avg_foc_data.z = (int16_t)(temp_foc_data.z);

	rslt = validate_foc_position(sens_list, accel_g_axis, avg_foc_data, dev);
    }

    return rslt;
}

/*!
 * @brief This internal API reads and provides average for 128 samples of sensor data for accel FOC.
 */
static int8_t get_average_of_sensor_data(uint8_t sens_list,
					 struct bmi3_foc_temp_value *temp_foc_data,
					 struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Structure to store sensor data */
    struct bmi3_sensor_data sensor_data = { 0 };

    uint8_t sample_count = 0;
    uint8_t datardy_try_cnt;
    uint16_t drdy_status = 0;

    /* Assign sensor list to type */
    sensor_data.type = sens_list;

    rslt = null_ptr_check(dev);

    if (rslt == BMI3_OK)
    {
	/* Read sensor values before FOC */
	while (sample_count < BMI3_FOC_SAMPLE_LIMIT)
	{
	    datardy_try_cnt = 5;
	    do
	    {
		dev->delay_us(20000, dev->intf_ptr);
		rslt = bmi3_get_sensor_status(&drdy_status, dev);
		datardy_try_cnt--;
	    } while ((rslt == BMI3_OK) && (!(drdy_status)) && (datardy_try_cnt));

	    if ((rslt != BMI3_OK) || (datardy_try_cnt == 0))
	    {
		rslt = BMI3_E_DATA_RDY_INT_FAILED;
		break;
	    }

	    rslt = bmi3_get_sensor_data(&sensor_data, 1, dev);

	    if (rslt == BMI3_OK)
	    {
		if (sensor_data.type == BMI3_ACCEL)
		{
		    temp_foc_data->x += sensor_data.sens_data.acc.x;
		    temp_foc_data->y += sensor_data.sens_data.acc.y;
		    temp_foc_data->z += sensor_data.sens_data.acc.z;
                } else if (sensor_data.type == BMI3_GYRO)
		{
		    temp_foc_data->x += sensor_data.sens_data.gyr.x;
		    temp_foc_data->y += sensor_data.sens_data.gyr.y;
		    temp_foc_data->z += sensor_data.sens_data.gyr.z;
		}
            } else
	    {
		break;
	    }

	    sample_count++;
	}

	if (rslt == BMI3_OK)
	{
	    temp_foc_data->x = (temp_foc_data->x / BMI3_FOC_SAMPLE_LIMIT);
	    temp_foc_data->y = (temp_foc_data->y / BMI3_FOC_SAMPLE_LIMIT);
	    temp_foc_data->z = (temp_foc_data->z / BMI3_FOC_SAMPLE_LIMIT);
	}
    }

    return rslt;
}

/*!
 * @brief This internal API validates accel FOC position as per the range
 */
static int8_t validate_foc_position(uint8_t sens_list,
				    const struct bmi3_accel_foc_g_value *accel_g_axis,
				    struct bmi3_sens_axes_data avg_foc_data,
				    struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_E_INVALID_INPUT;

    if (sens_list == BMI3_ACCEL)
    {
	if (accel_g_axis->x == 1)
	{
	    rslt = validate_foc_accel_axis(avg_foc_data.x, dev);
        } else if (accel_g_axis->y == 1)
	{
	    rslt = validate_foc_accel_axis(avg_foc_data.y, dev);
        } else
	{
	    rslt = validate_foc_accel_axis(avg_foc_data.z, dev);
	}
    }

    return rslt;
}

/*!
 * @brief This internal API validates accel FOC axis given as input
 */
static int8_t validate_foc_accel_axis(int16_t avg_foc_data, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Structure to store sensor configurations */
    struct bmi3_sens_config sens_cfg = { 0 };

    /* Variable to store accel range */
    uint8_t range;

    /* Assign the accel type */
    sens_cfg.type = BMI3_ACCEL;

    /* Get accel configurations */
    rslt = bmi3_get_sensor_config(&sens_cfg, 1, dev);

    if (rslt == BMI3_OK)
    {
	/* Assign accel range to variable */
	range = sens_cfg.cfg.acc.range;

	/* Reference LSB value of 2G */
	if ((range == BMI3_ACC_RANGE_2G) && (avg_foc_data > BMI3_MIN_NOISE_LIMIT(BMI3_ACC_FOC_2G_REF)) &&
	    (avg_foc_data < BMI3_MAX_NOISE_LIMIT(BMI3_ACC_FOC_2G_REF)))
	{
	    rslt = BMI3_OK;
	}
	/* Reference LSB value of 4G */
	else if ((range == BMI3_ACC_RANGE_4G) && (avg_foc_data > BMI3_MIN_NOISE_LIMIT(BMI3_ACC_FOC_4G_REF)) &&
		 (avg_foc_data < BMI3_MAX_NOISE_LIMIT(BMI3_ACC_FOC_4G_REF)))
	{
	    rslt = BMI3_OK;
	}
	/* Reference LSB value of 8G */
	else if ((range == BMI3_ACC_RANGE_8G) && (avg_foc_data > BMI3_MIN_NOISE_LIMIT(BMI3_ACC_FOC_8G_REF)) &&
		 (avg_foc_data < BMI3_MAX_NOISE_LIMIT(BMI3_ACC_FOC_8G_REF)))
	{
	    rslt = BMI3_OK;
	}
	/* Reference LSB value of 16G */
	else if ((range == BMI3_ACC_RANGE_16G) && (avg_foc_data > BMI3_MIN_NOISE_LIMIT(BMI3_ACC_FOC_16G_REF)) &&
		 (avg_foc_data < BMI3_MAX_NOISE_LIMIT(BMI3_ACC_FOC_16G_REF)))
	{
	    rslt = BMI3_OK;
        } else
	{
	    rslt = BMI3_E_INVALID_FOC_POSITION;
	}
    }

    return rslt;
}

/*!
 * @brief This internal API sets configurations for performing accelerometer FOC.
 */
static int8_t set_accel_foc_config(struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    /* Variable to set the accelerometer configuration value */
    uint8_t acc_conf_data[2] = { BMI3_FOC_ACC_CONF_VAL_LSB, BMI3_FOC_ACC_CONF_VAL_MSB };

    /* Set accelerometer configurations to 50Hz */
    rslt = bmi3_set_regs(BMI3_REG_ACC_CONF, acc_conf_data, 2, dev);

    return rslt;
}

/*!
 * @brief This internal API performs Fast Offset Compensation for accelerometer.
 */
static int8_t perform_accel_foc(const struct bmi3_accel_foc_g_value *accel_g_value,
				const struct bmi3_accel_config *acc_cfg,
				struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt = BMI3_E_INVALID_STATUS;

    /* Variable to define count */
    uint8_t loop;

    /* Variable to store status read from the status register */
    uint16_t reg_status = 0;

    /* Array of structure to store accelerometer data */
    struct bmi3_sensor_data accel_value[64] = { { 0 } };

    /* Structure to store accelerometer data temporarily */
    struct bmi3_foc_temp_value temp = { 0, 0, 0 };

    /* Structure to store the average of accelerometer data */
    struct bmi3_sens_axes_data accel_avg = { 0 };

    /* Variable to define LSB per g value */
    uint16_t lsb_per_g = 0;

    /* Variable to define range */
    uint8_t range = 0;

    /* Structure to store accelerometer data deviation from ideal value */
    struct bmi3_offset_delta delta = { 0, 0, 0 };

    /* Structure to store accelerometer offset values */
    struct bmi3_acc_usr_gain_offset offset = { 0 };

    /* Variable tries max 5 times for interrupt then generates timeout */
    uint8_t try_cnt;

    /* Disable accel */
    uint8_t acc_conf_data[2] = { 0 };

    /* Variable to store time out value */
    uint8_t time_out = 20;

    /* Delay to read feature engine error status */
    uint16_t accel_foc_delay = 5000;

    /* Variable to define feature engine errors */
    uint8_t feature_engine_err_reg_lsb = 0, feature_engine_err_reg_msb = 0;

    for (loop = 0; loop < BMI3_FOC_SAMPLE_LIMIT; loop++)
    {
	try_cnt = 5;

	while (try_cnt && (!(reg_status)))
	{
	    /* 20ms delay for 50Hz ODR */
	    dev->delay_us(20000, dev->intf_ptr);
	    rslt = bmi3_get_sensor_status(&reg_status, dev);
	    try_cnt--;
	}

	if ((rslt == BMI3_OK) && (reg_status))
	{
	    rslt = get_accel_sensor_data(&accel_value[loop].sens_data.acc, BMI3_REG_ACC_DATA_X, dev);
	}

	if (rslt == BMI3_OK)
	{
	    /* Store the data in a temporary structure */
	    temp.x = temp.x + (int32_t)accel_value[loop].sens_data.acc.x;
	    temp.y = temp.y + (int32_t)accel_value[loop].sens_data.acc.y;
	    temp.z = temp.z + (int32_t)accel_value[loop].sens_data.acc.z;
        } else
	{
	    break;
	}
    }

    if (rslt == BMI3_OK)
    {
	/* Take average of x, y and z data for lesser noise */
	accel_avg.x = (int16_t)(temp.x / 128);
	accel_avg.y = (int16_t)(temp.y / 128);
	accel_avg.z = (int16_t)(temp.z / 128);

	/* Get the exact range value */
	map_accel_range(acc_cfg->range, &range);

	/* Get the smallest possible measurable acceleration level given the range and
	 * resolution */
	lsb_per_g = (uint16_t)(power(2, dev->resolution) / (2 * range));

	/* Compensate acceleration data against gravity */
	comp_for_gravity(lsb_per_g, accel_g_value, &accel_avg, &delta);

	/* Scale according to offset register resolution */
	scale_accel_offset(range, &delta, &offset);

	/* Invert the accelerometer offset data */
	invert_accel_offset(&offset);

	/* Disable accel */
	rslt = bmi3_set_regs(BMI3_REG_ACC_CONF, acc_conf_data, 2, dev);

	if (rslt == BMI3_OK)
	{
	    /* Write offset data in the offset compensation register */
	    rslt = bmi3_set_user_acc_off_dgain(&offset, dev);

	    if (rslt == BMI3_OK)
	    {
		rslt = bmi3_set_command_register(BMI3_CMD_USR_GAIN_OFFS_UPDATE, dev);
	    }
	}

	if (rslt == BMI3_OK)
	{
	    for (loop = 0; loop < time_out; loop++)
	    {
		dev->delay_us(accel_foc_delay, dev->intf_ptr);

		rslt = bmi3_get_feature_engine_error_status(&feature_engine_err_reg_lsb,
							    &feature_engine_err_reg_msb,
							    dev);

		if ((feature_engine_err_reg_lsb & BMI3_NO_ERROR_MASK) &&
		    (feature_engine_err_reg_msb & (BMI3_UGAIN_OFFS_UPD_COMPLETE_MASK >> 8)))
		{
		    break;
		}
	    }
	}
    }

    return rslt;
}

/*!
 * @brief This internal API converts the accelerometer range value into
 * corresponding integer value.
 */
static void map_accel_range(uint8_t range_in, uint8_t *range_out)
{
    switch (range_in)
    {
	case BMI3_ACC_RANGE_2G:
	    *range_out = 2;
	    break;
	case BMI3_ACC_RANGE_4G:
	    *range_out = 4;
	    break;
	case BMI3_ACC_RANGE_8G:
	    *range_out = 8;
	    break;
	case BMI3_ACC_RANGE_16G:
	    *range_out = 16;
	    break;
	default:

	    /* By default RANGE 8G is set */
	    *range_out = 8;
	    break;
    }
}

/*!
 * @brief This internal API is used to calculate the power of a value.
 */
static int32_t power(int16_t base, uint8_t resolution)
{
    /* Initialize loop */
    uint8_t loop = 1;

    /* Initialize variable to store the power of 2 value */
    int32_t value = 1;

    for (; loop <= resolution; loop++)
    {
	value = (int32_t)(value * base);
    }

    return value;
}

/*!
 * @brief This internal API compensate the accelerometer data against gravity.
 */
static void comp_for_gravity(uint16_t lsb_per_g,
			     const struct bmi3_accel_foc_g_value *g_val,
			     const struct bmi3_sens_axes_data *data,
			     struct bmi3_offset_delta *comp_data)
{
    /* Array to store the accelerometer values in LSB */
    int16_t accel_value_lsb[3] = { 0 };

    /* Convert g-value to LSB */
    accel_value_lsb[BMI3_X_AXIS] = (int16_t)(lsb_per_g * g_val->x);
    accel_value_lsb[BMI3_Y_AXIS] = (int16_t)(lsb_per_g * g_val->y);
    accel_value_lsb[BMI3_Z_AXIS] = (int16_t)(lsb_per_g * g_val->z);

    /* Get the compensated values for X, Y and Z axis */
    comp_data->x = (data->x - accel_value_lsb[BMI3_X_AXIS]);
    comp_data->y = (data->y - accel_value_lsb[BMI3_Y_AXIS]);
    comp_data->z = (data->z - accel_value_lsb[BMI3_Z_AXIS]);
}

/*!
 * @brief This internal API scales the compensated accelerometer data according
 * to the offset register resolution.
 *
 * @note The bit position is always greater than 0 since accelerometer data is
 * 16 bit wide.
 */
static void scale_accel_offset(uint8_t range,
			       const struct bmi3_offset_delta *comp_data,
			       struct bmi3_acc_usr_gain_offset *data)
{
    /* Variable to store the position of bit having 3.9mg resolution */
    int8_t bit_pos_3_9mg;

    /* Variable to store the position previous of bit having 3.9mg resolution */
    int8_t bit_pos_3_9mg_prev_bit;

    /* Variable to store the round-off value */
    uint8_t round_off;

    /* Find the bit position of 3.9mg */
    bit_pos_3_9mg = get_bit_pos_3_9mg(range);

    /* Round off, consider if the next bit is high */
    bit_pos_3_9mg_prev_bit = bit_pos_3_9mg - 1;
    round_off = (uint8_t)(power(2, ((uint8_t) bit_pos_3_9mg_prev_bit)));

    /* Scale according to offset register resolution */
    data->acc_usr_off_x = (uint8_t)((comp_data->x + round_off) / power(2, ((uint8_t) bit_pos_3_9mg)));
    data->acc_usr_off_y = (uint8_t)((comp_data->y + round_off) / power(2, ((uint8_t) bit_pos_3_9mg)));
    data->acc_usr_off_z = (uint8_t)((comp_data->z + round_off) / power(2, ((uint8_t) bit_pos_3_9mg)));
}

/*!
 * @brief This internal API finds the bit position of 3.9mg according to given
 * range and resolution.
 */
static int8_t get_bit_pos_3_9mg(uint8_t range)
{
    /* Variable to store the bit position of 3.9mg resolution */
    int8_t bit_pos_3_9mg;

    /* Variable to shift the bits according to the resolution */
    uint32_t divisor = 1;

    /* Scaling factor to get the bit position of 3.9 mg resolution */
    int16_t scale_factor = BMI3_FOC_INVERT_VALUE;

    /* Variable to store temporary value */
    uint16_t temp;

    /* Shift left by the times of resolution */
    divisor = divisor << BMI3_16_BIT_RESOLUTION;

    /* Get the bit position to be shifted */
    temp = (uint16_t)(divisor / (range * 256));

    /* Get the scaling factor until bit position is shifted to last bit */
    while (temp != 1)
    {
	scale_factor++;
	temp = temp >> 1;
    }

    /* Scaling factor is the bit position of 3.9 mg resolution */
    bit_pos_3_9mg = (int8_t) scale_factor;

    return bit_pos_3_9mg;
}

/*!
 * @brief This internal API inverts the accelerometer offset data.
 */
static void invert_accel_offset(struct bmi3_acc_usr_gain_offset *offset_data)
{
    /* Get the offset data */
    offset_data->acc_usr_off_x = (uint8_t)((offset_data->acc_usr_off_x) * (BMI3_FOC_INVERT_VALUE));
    offset_data->acc_usr_off_y = (uint8_t)((offset_data->acc_usr_off_y) * (BMI3_FOC_INVERT_VALUE));
    offset_data->acc_usr_off_z = (uint8_t)((offset_data->acc_usr_off_z) * (BMI3_FOC_INVERT_VALUE));
}

/*!
 * @brief This internal API sets the individual gyroscope
 * filter coefficients in the respective dma registers.
 */
static int8_t set_gyro_filter_coefficients(struct bmi3_dev *dev)
{
    int8_t rslt;

    uint8_t data_array[18];

    uint8_t gyro_filter_coeff_base_addr[2] = { BMI3_BASE_ADDR_GYRO_SC_ST_COEFFICIENTS, 0x00 };

    data_array[0] = BMI3_SC_ST_VALUE_0 & BMI3_SET_LOW_BYTE;
    data_array[1] = (BMI3_SC_ST_VALUE_0 & BMI3_SET_HIGH_BYTE) >> 8;
    data_array[2] = BMI3_SC_ST_VALUE_1 & BMI3_SET_LOW_BYTE;
    data_array[3] = (BMI3_SC_ST_VALUE_1 & BMI3_SET_HIGH_BYTE) >> 8;
    data_array[4] = BMI3_SC_ST_VALUE_2 & BMI3_SET_LOW_BYTE;
    data_array[5] = (BMI3_SC_ST_VALUE_2 & BMI3_SET_HIGH_BYTE) >> 8;
    data_array[6] = BMI3_SC_ST_VALUE_3 & BMI3_SET_LOW_BYTE;
    data_array[7] = (BMI3_SC_ST_VALUE_3 & BMI3_SET_HIGH_BYTE) >> 8;
    data_array[8] = BMI3_SC_ST_VALUE_4 & BMI3_SET_LOW_BYTE;
    data_array[9] = (BMI3_SC_ST_VALUE_4 & BMI3_SET_HIGH_BYTE) >> 8;
    data_array[10] = BMI3_SC_ST_VALUE_5 & BMI3_SET_LOW_BYTE;

    /* Since high byte for BMI3_SC_ST_VALUE_5 is zero, this variable has been directly assigned to zero */
    data_array[11] = 0;

    data_array[12] = BMI3_SC_ST_VALUE_6 & BMI3_SET_LOW_BYTE;
    data_array[13] = (BMI3_SC_ST_VALUE_6 & BMI3_SET_HIGH_BYTE) >> 8;
    data_array[14] = BMI3_SC_ST_VALUE_7 & BMI3_SET_LOW_BYTE;
    data_array[15] = (BMI3_SC_ST_VALUE_7 & BMI3_SET_HIGH_BYTE) >> 8;
    data_array[16] = BMI3_SC_ST_VALUE_8 & BMI3_SET_LOW_BYTE;
    data_array[17] = (BMI3_SC_ST_VALUE_8 & BMI3_SET_HIGH_BYTE) >> 8;

    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, gyro_filter_coeff_base_addr, 2, dev);

    if (rslt == BMI3_OK)
    {
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, data_array, 18, dev);
    }

    return rslt;
}

/*!
 * @brief This internal API checks data index for data parsing.
 */
static int8_t check_data_index(uint16_t data_index, const struct bmi3_fifo_frame *fifo)
{
    int8_t rslt;
    uint16_t fifo_index = 0;

    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_ACC_FRM)
    {
	fifo_index += BMI3_LENGTH_FIFO_ACC;
    }

    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_GYR_FRM)
    {
	fifo_index += BMI3_LENGTH_FIFO_GYR;
    }

    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_TEMP_FRM)
    {
	fifo_index += BMI3_LENGTH_TEMPERATURE;
    }

    if (fifo->available_fifo_sens & BMI3_FIFO_HEAD_LESS_SENS_TIME_FRM)
    {
	fifo_index += BMI3_LENGTH_SENSOR_TIME;
    }

    if ((data_index + fifo_index) < fifo->length)
    {
	rslt = BMI3_OK;
    } else
    {
	rslt = BMI3_W_FIFO_INVALID_FRAME;
    }

    return rslt;
}

/*!
 * @brief This internal API is used to validate ODR and AVG combinations for accel
 */
static int8_t validate_acc_odr_avg(uint8_t acc_odr, uint8_t acc_avg)
{
    int8_t rslt;

    uint32_t max_odr = 6400000;

    uint32_t odr = 0, avg = 0, skipped_samples = 0;

    switch (acc_odr)
    {
	case BMI3_ACC_ODR_0_78HZ:
	    odr = 781;
	    break;
	case BMI3_ACC_ODR_1_56HZ:
	    odr = 1562;
	    break;
	case BMI3_ACC_ODR_3_125HZ:
	    odr = 3125;
	    break;
	case BMI3_ACC_ODR_6_25HZ:
	    odr = 6250;
	    break;
	case BMI3_ACC_ODR_12_5HZ:
	    odr = 12500;
	    break;
	case BMI3_ACC_ODR_25HZ:
	    odr = 25000;
	    break;
	case BMI3_ACC_ODR_50HZ:
	    odr = 50000;
	    break;
	case BMI3_ACC_ODR_100HZ:
	    odr = 100000;
	    break;
	case BMI3_ACC_ODR_200HZ:
	    odr = 200000;
	    break;
	case BMI3_ACC_ODR_400HZ:
	    odr = 400000;
	    break;
	default:
	    break;
    }

    switch (acc_avg)
    {
	case BMI3_ACC_AVG1:
	    avg = 1;
	    break;
	case BMI3_ACC_AVG2:
	    avg = 2;
	    break;
	case BMI3_ACC_AVG4:
	    avg = 4;
	    break;
	case BMI3_ACC_AVG8:
	    avg = 8;
	    break;
	case BMI3_ACC_AVG16:
	    avg = 16;
	    break;
	case BMI3_ACC_AVG32:
	    avg = 32;
	    break;
	case BMI3_ACC_AVG64:
	    avg = 64;
	    break;
	default:
	    break;
    }

    if ((odr > 0) && (avg > 0))
    {
	skipped_samples = (max_odr / odr) - avg;

	if (skipped_samples > 0)
	{
	    rslt = BMI3_OK;
        } else
	{
	    rslt = BMI3_E_ACC_INVALID_CFG;
	}
    } else
    {
	rslt = BMI3_E_ACC_INVALID_CFG;
    }

    return rslt;
}

/*!
 * @brief This internal API is used to validate ODR and AVG combinations for gyro
 */
static int8_t validate_gyr_odr_avg(uint8_t gyr_odr, uint8_t gyr_avg)
{
    int8_t rslt;

    uint32_t max_odr = 6400000;

    uint32_t odr = 0, avg = 0, skipped_samples = 0;

    switch (gyr_odr)
    {
	case BMI3_GYR_ODR_0_78HZ:
	    odr = 781;
	    break;
	case BMI3_GYR_ODR_1_56HZ:
	    odr = 1562;
	    break;
	case BMI3_GYR_ODR_3_125HZ:
	    odr = 3125;
	    break;
	case BMI3_GYR_ODR_6_25HZ:
	    odr = 6250;
	    break;
	case BMI3_GYR_ODR_12_5HZ:
	    odr = 12500;
	    break;
	case BMI3_GYR_ODR_25HZ:
	    odr = 25000;
	    break;
	case BMI3_GYR_ODR_50HZ:
	    odr = 50000;
	    break;
	case BMI3_GYR_ODR_100HZ:
	    odr = 100000;
	    break;
	case BMI3_GYR_ODR_200HZ:
	    odr = 200000;
	    break;
	case BMI3_GYR_ODR_400HZ:
	    odr = 400000;
	    break;
	default:
	    break;
    }

    switch (gyr_avg)
    {
	case BMI3_GYR_AVG1:
	    avg = 1;
	    break;
	case BMI3_GYR_AVG2:
	    avg = 2;
	    break;
	case BMI3_GYR_AVG4:
	    avg = 4;
	    break;
	case BMI3_GYR_AVG8:
	    avg = 8;
	    break;
	case BMI3_GYR_AVG16:
	    avg = 16;
	    break;
	case BMI3_GYR_AVG32:
	    avg = 32;
	    break;
	case BMI3_GYR_AVG64:
	    avg = 64;
	    break;
	default:
	    break;
    }

    if ((odr > 0) && (avg > 0))
    {
	skipped_samples = (max_odr / odr) - avg;

	if (skipped_samples > 0)
	{
	    rslt = BMI3_OK;
        } else
	{
	    rslt = BMI3_E_GYRO_INVALID_CFG;
	}
    } else
    {
	rslt = BMI3_E_GYRO_INVALID_CFG;
    }

    return rslt;
}

/*!
 * @brief This internal API writes the command register value to enable cfg res.
 */
static int8_t config_array_set_command(struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Array to read feature engine GP1 error status */
    uint8_t feature_engine_gp_1[2] = { 0 };

    /* Array to get default value of cfg res register */
    uint8_t cfg_res[2] = { 0 };

    /* Get error status of feature engine GP1 register */
    rslt = bmi3_get_regs(BMI3_REG_FEATURE_IO1, feature_engine_gp_1, 2, dev);

    /* Check whether feature engine is activated in error status */
    if ((rslt == BMI3_OK) && (feature_engine_gp_1[0] == BMI3_FEAT_ENG_ACT_MASK))
    {
	/* Read default value of cfg res address */
	rslt = bmi3_get_regs(BMI3_REG_CFG_RES, cfg_res, 2, dev);
    }

    /* Burst write the command register value to enable cfg res */
    if (rslt == BMI3_OK)
    {
	rslt = bmi3_set_command_register(BMI3_CMD_2, dev);

	if (rslt == BMI3_OK)
	{
	    rslt = bmi3_set_command_register(BMI3_CMD_1, dev);
	}
    }

    return rslt;
}

/*!
 * @brief This internal API sets the value_one to feature engine in cfg res.
 */
static int8_t config_array_set_value_one_page(struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Array to get value of cfg res register after writing the command register */
    uint8_t cfg_res[2] = { 0 };

    /* Array to store value_one */
    uint8_t cfg_res_value[2] = { BMI3_CFG_RES_VALUE_ONE, BMI3_CFG_RES_MASK };

    /* Read cfg res address */
    rslt = bmi3_get_regs(BMI3_REG_CFG_RES, cfg_res, 2, dev);

    if ((rslt == BMI3_OK) && (cfg_res[1] == BMI3_CFG_RES_MASK))
    {
	rslt = bmi3_set_regs(BMI3_REG_CFG_RES, cfg_res_value, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This internal API writes the config array.
 */
static int8_t write_config_array(struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Array to reset the BMI3_REG_CFG_RES register */
    uint8_t reset[2] = { 0 };

    /* Download config code array */
    rslt = load_config_array(bmi3_config_array_code, sizeof(bmi3_config_array_code), dev);

    if (rslt == BMI3_OK)
    {
	/* Download config array table array */
	rslt = load_config_array(bmi3_config_array_table, sizeof(bmi3_config_array_table), dev);

	if (rslt == BMI3_OK)
	{
	    /* Download config version */
	    rslt = write_config_version(dev);
	}
    }

    if (rslt == BMI3_OK)
    {
	/* Reset the BMI3_REG_CFG_RES to return back to user page */
	rslt = bmi3_set_regs(BMI3_REG_CFG_RES, reset, 2, dev);
    }

    return rslt;
}

/*!
 * @brief This internal API writes config array.
 */
static int8_t load_config_array(const uint8_t *config_array, uint16_t config_size, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt = BMI3_OK;

    /* Variable to loop */
    uint16_t indx;

    /* Variable to get the remainder */
    uint8_t remain = (uint8_t)((config_size - BMI3_CONFIG_ARRAY_DATA_START_ADDR) % dev->read_write_len);

    /* Variable to get the balance bytes */
    uint16_t bal_byte = 0;

    /* Variable to define temporary read/write length */
    uint16_t read_write_len = 0;

    /* Variable to store base address of config array */
    uint16_t config_base_addr;

    /* First two bytes of config array denotes the base address */
    config_base_addr = (config_array[0] | (uint16_t)(config_array[1] << 8));

    if (!remain)
    {
	for (indx = BMI3_CONFIG_ARRAY_DATA_START_ADDR; indx < config_size; indx += dev->read_write_len)
	{
	    rslt = upload_file(config_base_addr, config_array, indx, dev);

	    /* Increment the base address */
	    config_base_addr = config_base_addr + (dev->read_write_len / 2);
	}
    } else
    {
	/* Get the balance bytes */
	bal_byte = (uint16_t) config_size - (uint16_t) remain;

	/* Write the configuration file for the balance bytes */
	for (indx = BMI3_CONFIG_ARRAY_DATA_START_ADDR; indx < bal_byte; indx += dev->read_write_len)
	{
	    rslt = upload_file(config_base_addr, config_array, indx, dev);

	    /* Increment the base address */
	    config_base_addr = config_base_addr + (dev->read_write_len / 2);
	}

	if (rslt == BMI3_OK)
	{
	    /* Update length in a temporary variable */
	    read_write_len = dev->read_write_len;

	    /* Write the remaining bytes in 2 bytes length */
	    dev->read_write_len = 2;

	    /* Check if balance byte is zero, then replace with config array data start address. */
	    if (bal_byte == 0)
	    {
		bal_byte = BMI3_CONFIG_ARRAY_DATA_START_ADDR;
	    }

	    /* Write the configuration file for the remaining bytes */
	    for (indx = bal_byte; indx < config_size; indx += dev->read_write_len)
	    {
		rslt = upload_file(config_base_addr, config_array, indx, dev);

		/* Increment the base address */
		config_base_addr = config_base_addr + (dev->read_write_len / 2);
	    }

	    /* Restore the user set length back from the temporary variable */
	    dev->read_write_len = read_write_len;
	}
    }

    return rslt;
}

/*!
 * @brief This internal API writes config array code array and table array to feature engine register.
 */
static int8_t upload_file(uint16_t config_base_addr, const uint8_t *config_array, uint16_t indx, struct bmi3_dev *dev)
{
    int8_t rslt;

    uint8_t base_addr[2];

    base_addr[0] = config_base_addr & BMI3_SET_LOW_BYTE;
    base_addr[1] = (uint8_t)((config_base_addr & BMI3_SET_HIGH_BYTE) >> 8);

    /* Set the config array base address to feature engine transmission address to start DMA transaction */
    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, base_addr, 2, dev);

    if (rslt == BMI3_OK)
    {
	/* Set the config array data to feature engine transmission data address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, &config_array[indx], dev->read_write_len, dev);
    }

    return rslt;
}

/*!
 * @brief This internal API writes config version array to feature engine register.
 */
static int8_t write_config_version(struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Index variable to store the start address of data */
    uint8_t indx = BMI3_CONFIG_ARRAY_DATA_START_ADDR;

    /* Set the config array base address to feature engine transmission address to start DMA transaction */
    rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_ADDR, bmi3_config_version, 2, dev);

    if (rslt == BMI3_OK)
    {
	/* Set the config version data to feature engine transmission data address to start DMA transaction */
	rslt = bmi3_set_regs(BMI3_REG_FEATURE_DATA_TX, &bmi3_config_version[indx], 2, dev);
    }

    return rslt;
}
/*lint -e578 -e132 */
EXPORT_SYMBOL(bmi3_set_acc_gyr_off_gain_reset);
EXPORT_SYMBOL(bmi3_get_acc_gyr_off_gain_reset);
EXPORT_SYMBOL(bmi3_get_i3c_ibi_status);
EXPORT_SYMBOL(bmi3_get_sensor_status);
EXPORT_SYMBOL(bmi3_perform_accel_foc);
EXPORT_SYMBOL(bmi3_set_user_gyr_off_dgain);
EXPORT_SYMBOL(bmi3_set_user_acc_off_dgain);
EXPORT_SYMBOL(bmi3_get_user_gyro_off_dgain);
EXPORT_SYMBOL(bmi3_get_user_acc_off_dgain);
EXPORT_SYMBOL(bmi3_set_gyro_dp_off_dgain);
EXPORT_SYMBOL(bmi3_set_acc_dp_off_dgain);
EXPORT_SYMBOL(bmi3_get_gyro_dp_off_dgain);
EXPORT_SYMBOL(bmi3_get_acc_dp_off_dgain);
EXPORT_SYMBOL(bmi3_read_alternate_status);
EXPORT_SYMBOL(bmi3_alternate_config_ctrl);
EXPORT_SYMBOL(bmi3_set_i3c_sync_i3c_tc_res);
EXPORT_SYMBOL(bmi3_get_i3c_sync_i3c_tc_res);
EXPORT_SYMBOL(bmi3_get_i3c_tc_sync_odr);
EXPORT_SYMBOL(bmi3_set_i3c_tc_sync_odr);
EXPORT_SYMBOL(bmi3_get_i3c_tc_sync_tu);
EXPORT_SYMBOL(bmi3_set_i3c_tc_sync_tu);
EXPORT_SYMBOL(bmi3_get_i3c_tc_sync_tph);
EXPORT_SYMBOL(bmi3_set_i3c_tc_sync_tph);
EXPORT_SYMBOL(bmi3_perform_gyro_sc);
EXPORT_SYMBOL(bmi3_get_config_version);
EXPORT_SYMBOL(bmi3_configure_enhanced_flexibility);
EXPORT_SYMBOL(bmi3_perform_self_test);
EXPORT_SYMBOL(bmi3_init);
EXPORT_SYMBOL(bmi3_get_fifo_length);
EXPORT_SYMBOL(bmi3_get_fifo_config);
EXPORT_SYMBOL(bmi3_set_fifo_config);
EXPORT_SYMBOL(bmi3_get_fifo_wm);
EXPORT_SYMBOL(bmi3_set_fifo_wm);
EXPORT_SYMBOL(bmi3_extract_gyro);
EXPORT_SYMBOL(bmi3_extract_temperature);
EXPORT_SYMBOL(bmi3_extract_accel);
EXPORT_SYMBOL(bmi3_read_fifo_data);
EXPORT_SYMBOL(bmi3_get_temperature_data);
EXPORT_SYMBOL(bmi3_get_sensor_time);
EXPORT_SYMBOL(bmi3_get_int_pin_config);
EXPORT_SYMBOL(bmi3_set_int_pin_config);
EXPORT_SYMBOL(bmi3_get_feature_engine_error_status);
EXPORT_SYMBOL(bmi3_get_error_status);
EXPORT_SYMBOL(bmi3_get_sensor_data);
EXPORT_SYMBOL(bmi3_select_sensor);
EXPORT_SYMBOL(bmi3_map_interrupt);
EXPORT_SYMBOL(bmi3_get_sensor_config);
EXPORT_SYMBOL(bmi3_set_sensor_config);
EXPORT_SYMBOL(bmi3_set_remap_axes);
EXPORT_SYMBOL(bmi3_get_remap_axes);
EXPORT_SYMBOL(bmi3_get_int2_status);
EXPORT_SYMBOL(bmi3_get_int1_status);
EXPORT_SYMBOL(bmi3_set_command_register);
EXPORT_SYMBOL(bmi3_soft_reset);
EXPORT_SYMBOL(bmi3_set_regs);
EXPORT_SYMBOL(bmi3_get_regs);
/*lint +e578 +e132 */

/*lint -e533 -e2 -e10 -e102 -e14 */
MODULE_LICENSE("Dual BSD/GPL");
/*lint +e533 +e2 +e10 +e102 -e14 */

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
 * @file       bmi323.h
 * @date       2022-08-31
 * @version    v2.0.0
 *
 */

/*!
 * @defgroup bmi323 BMI323
 */

/**
 * \ingroup bmi323
 * \defgroup bmi3 BMI3
 * @brief Sensor driver for BMI3 sensor
 */

#ifndef _BMI323_H
#define _BMI323_H

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************/

/*!             Header files
 ****************************************************************************/
#include "bmi3.h"
#include "bmi323_defs.h"

/***************************************************************************/

/*!     BMI3XY User Interface function prototypes
 ****************************************************************************/

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiInit Initialization
 * @brief Initialize the sensor and device structure
 */

/*!
 * \ingroup bmi323ApiInit
 * \page bmi323_api_bmi323_init bmi323_init
 * \code
 * int8_t bmi323_init(struct bmi3_dev *dev);
 * \endcode
 * @details This API is the entry point for bmi323 sensor. It also reads the chip-id of
 * the sensor.
 *
 * @param[in,out] dev  : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_init(struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiRegs Registers
 * @brief Set / Get data from the given register address of the sensor
 */

/*!
 * \ingroup bmi323ApiRegs
 * \page bmi323_api_bmi323_set_regs bmi323_set_regs
 * \code
 * int8_t bmi323_set_regs(uint8_t reg_addr, const uint8_t *data, uint16_t len, struct bmi3_dev *dev);
 * \endcode
 * @details This API writes data to the given register address of bmi323 sensor.
 *
 * @param[in] reg_addr  : Register address to which the data is written.
 * @param[in] data      : Pointer to data buffer in which data to be written
 *                        is stored.
 * @param[in] len       : No. of bytes of data to be written.
 * @param[in] dev       : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_regs(uint8_t reg_addr, const uint8_t *data, uint16_t len, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiRegs
 * \page bmi323_api_bmi323_get_regs bmi323_get_regs
 * \code
 * int8_t bmi323_get_regs(uint8_t reg_addr, uint8_t *data, uint16_t len, struct bmi3_dev *dev);
 * \endcode
 * @details This API reads the data from the given register address of bmi323
 * sensor.
 *
 * @param[in] reg_addr  : Register address from which data is read.
 * @param[out] data     : Pointer to data buffer where read data is stored.
 * @param[in] len       : No. of bytes of data to be read.
 * @param[in] dev       : Structure instance of bmi3_dev.
 *
 * @note For most of the registers auto address increment applies, with the
 * exception of a few special registers, which trap the address.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_regs(uint8_t reg_addr, uint8_t *data, uint16_t len, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ContextSel Context switch
 * @brief This API writes the configurations of context feature.
 */

/*!
 * \ingroup bmi323ContextSel
 * \page bmi323_api_bmi323_context_switch_selection bmi323_context_switch_selection
 * \code
 * int8_t bmi323_context_switch_selection(uint8_t context_sel, struct bmi3_dev *dev);
 * \endcode
 *
 * @details This API writes the configurations of context feature for smart phone, wearables and hearables.
 *
 * @param[in] context_sel  : Variable to select the context feature between smart phone, wearables and hearables.
 *
 * @Note: Default is wearable feature.
 *@verbatim
 *---------------------------|------------------------
 *     context_sel           |        Values
 *---------------------------|------------------------
 *  BMI323_SMART_PHONE_SEL   |          0
 *---------------------------|------------------------
 *  BMI323_WEARABLE_SEL      |          1
 *---------------------------|------------------------
 *  BMI323_HEARABLE_SEL      |          2
 *---------------------------|------------------------
 *@endverbatim
 *
 * @param[in] dev          : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_context_switch_selection(uint8_t context_sel, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiSR Soft-reset
 * @brief Set / Get data from the given register address of the sensor
 */

/*!
 * \ingroup bmi323ApiSR
 * \page bmi323_api_bmi323_soft_reset bmi323_soft_reset
 * \code
 * int8_t bmi323_soft_reset(struct bmi3_dev *dev);
 * \endcode
 * @details This API resets bmi323 sensor. All registers are overwritten with
 * their default values.
 *
 * @note If selected interface is SPI, an extra dummy byte is read to bring the
 * interface back to SPI from default, after the soft-reset command.
 *
 * @param[in] dev : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_soft_reset(struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiInt Interrupt
 * @brief Interrupt operations of the sensor
 */

/*!
 * \ingroup bmi323ApiInt
 * \page bmi323_api_bmi323_set_int_pin_config bmi323_set_int_pin_config
 * \code
 * int8_t bmi323_set_int_pin_config(const struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets:
 *        1) The input output configuration of the selected interrupt pin:
 *           INT1 or INT2.
 *        2) The interrupt mode: Permanently latched or non-latched.
 *
 * @param[in] int_cfg       : Structure instance of bmi3_int_pin_config.
 * @param[in] dev           : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_int_pin_config(const struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiInt
 * \page bmi323_api_bmi323_get_int_pin_config bmi323_get_int_pin_config
 * \code
 * int8_t bmi323_get_int_pin_config(struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets:
 *        1) The input output configuration of the selected interrupt pin:
 *           INT1 or INT2.
 *        2) The interrupt mode: Permanently latched or non-latched.
 *
 * @param[in,out] int_cfg  : Structure instance of bmi3_int_pin_config.
 * @param[in]     dev      : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_int_pin_config(struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiInt
 * \page bmi323_api_bmi323_get_int1_status bmi323_get_int_status
 * \code
 * int8_t bmi323_get_int1_status(uint16_t *int_status, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets the interrupt 1 status of both feature and data
 * interrupts.
 *
 * @param[out] int_status    : Pointer to get the status of the interrupts.
 * @param[in]  dev           : Structure instance of bmi3_dev.
 *
 *@verbatim
 * int_status   |  Status
 * -------------|------------
 * 0x0001       |  BMI3_INT_STATUS_NO_MOTION
 * 0x0002       |  BMI3_INT_STATUS_ANY_MOTION
 * 0x0004       |  BMI3_INT_STATUS_FLAT
 * 0x0008       |  BMI3_INT_STATUS_ORIENTATION
 * 0x0010       |  BMI3_INT_STATUS_STEP_DETECTOR
 * 0x0020       |  BMI3_INT_STATUS_STEP_COUNTER
 * 0x0040       |  BMI3_INT_STATUS_SIG_MOTION
 * 0x0080       |  BMI3_INT_STATUS_TILT
 * 0x0100       |  BMI3_INT_STATUS_TAP
 * 0x0200       |  BMI3_INT_STATUS_I3C
 * 0x0400       |  BMI3_INT_STATUS_ERR
 * 0x0800       |  BMI3_INT_STATUS_TEMP_DRDY
 * 0x1000       |  BMI3_INT_STATUS_GYR_DRDY
 * 0x2000       |  BMI3_INT_STATUS_ACC_DRDY
 * 0x4000       |  BMI3_INT_STATUS_FWM
 * 0x8000       |  BMI3_INT_STATUS_FFULL
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_int1_status(uint16_t *int_status, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiInt
 * \page bmi323_api_bmi323_get_int2_status bmi323_get_int_status
 * \code
 * int8_t bmi323_get_int2_status(uint16_t *int_status, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets the interrupt 2 status of both feature and data
 * interrupts.
 *
 * @param[out] int_status    : Pointer to get the status of the interrupts.
 * @param[in]  dev           : Structure instance of bmi3_dev.
 *
 *@verbatim
 * int_status   |  Status
 * -------------|------------
 * 0x0001       |  BMI3_INT_STATUS_NO_MOTION
 * 0x0002       |  BMI3_INT_STATUS_ANY_MOTION
 * 0x0004       |  BMI3_INT_STATUS_FLAT
 * 0x0008       |  BMI3_INT_STATUS_ORIENTATION
 * 0x0010       |  BMI3_INT_STATUS_STEP_DETECTOR
 * 0x0020       |  BMI3_INT_STATUS_STEP_COUNTER
 * 0x0040       |  BMI3_INT_STATUS_SIG_MOTION
 * 0x0080       |  BMI3_INT_STATUS_TILT
 * 0x0100       |  BMI3_INT_STATUS_TAP
 * 0x0200       |  BMI3_INT_STATUS_I3C
 * 0x0400       |  BMI3_INT_STATUS_ERR
 * 0x0800       |  BMI3_INT_STATUS_TEMP_DRDY
 * 0x1000       |  BMI3_INT_STATUS_GYR_DRDY
 * 0x2000       |  BMI3_INT_STATUS_ACC_DRDY
 * 0x4000       |  BMI3_INT_STATUS_FWM
 * 0x8000       |  BMI3_INT_STATUS_FFULL
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_int2_status(uint16_t *int_status, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiRemap Remap Axes
 * @brief Set / Get remap axes values from the sensor
 */

/*!
 * \ingroup bmi323ApiRemap
 * \page bmi323_api_bmi323_get_remap_axes bmi323_get_remap_axes
 * \code
 * int8_t bmi323_get_remap_axes(struct bmi3_remap *remapped_axis, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets the re-mapped x, y and z axes from the sensor and
 * updates the values in the device structure.
 *
 * @param[out] remapped_axis : Structure that stores re-mapped axes.
 * @param[in] dev            : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_remap_axes(struct bmi3_axes_remap *remapped_axis, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiRemap
 * \page bmi323_api_bmi323_set_remap_axes bmi323_set_remap_axes
 * \code
 * int8_t bmi323_set_remap_axes(const struct bmi3_remap remapped_axis, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets the re-mapped x, y and z axes to the sensor and
 * updates them in the device structure.
 *
 * @param[in] remapped_axis  : Structure that stores re-mapped axes.
 * @param[in] dev            : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_remap_axes(const struct bmi3_axes_remap remapped_axis, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiErrorStatus Error Status
 * @brief Read error status of the sensor
 */

/*!
 * \ingroup bmi323ApiErrorStatus
 * \page bmi323_api_bmi323_get_error_status bmi323_get_error_status
 * \code
 * int8_t bmi323_get_error_status(struct bmi3_err_reg *err_reg, struct bmi3_dev *dev);;
 * \endcode
 * @details This API reads the error status from the sensor.
 *
 * Below table mention the types of error which can occur in the sensor.
 *
 *@verbatim
 *************************************************************************
 *        Error           |       Description
 *************************|***********************************************
 *                        |       Fatal Error, chip is not in operational
 *        fatal_err       |       state (Boot-, power-system).
 *                        |       This flag will be reset only by
 *                        |       power-on-reset or soft-reset.
 *************************|***********************************************
 *                        |
 *        feat_eng_ovrld  |       Overload of the feature engine detected.
 *                        |       This flag is clear-on-read.
 *                        |
 *************************|***********************************************
 *                        |       Watchdog timer of the feature engine triggered.
 *        feat_eng_wd     |       This flag is clear-on-read.
 *                        |
 *************************|***********************************************
 *                        |
 *        acc_conf_err    |     Unsupported accelerometer configuration
 *                        |      set by user.This flag will be reset
 *                        |      when configuration has been corrected.
 *************************|***********************************************
 *                        |
 *        gyr_conf_err    |     Unsupported gyroscope configuration
 *                        |      set by user.This flag will be reset
 *                        |      when configuration has been corrected.
 *************************|***********************************************
 *                        |
 *        i3c_error0      |     SDR parity error or read abort condition
 *                        |      (maximum clock stall time for I3C Read
 *                        |      Transfer) occurred.
 *************************|***********************************************
 *                        |
 *        i3c_error3      |       This flag is clear-on-read type. It is
 *                        |       cleared automatically once read.
 *                        |
 *************************|***********************************************
 *
 *@endverbatim
 *
 * @param[in,out] err_reg : Pointer to structure variable which stores the
 *                          error status read from the sensor.
 * @param[in] dev         : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status.
 *
 * @return 0 -> Success
 * @return < 0 -> Fail
 *
 */
int8_t bmi323_get_error_status(struct bmi3_err_reg *err_reg, struct bmi3_dev *dev);

/*! @cond DOXYGEN_SUPRESS */

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiSensor Feature Set
 * @brief Enable / Disable features of the sensor
 */

/*!
 * \ingroup bmi323ApiSensor
 * \page bmi323_api_bmi323_select_sensor bmi323_select_sensor
 * \code
 * int8_t bmi323_select_sensor(struct bmi3_feature_enable *enable, struct bmi3_dev *dev);
 * \endcode
 * @details This API selects the sensors/features to be enabled or disabled.
 *
 * @param[in]       enable      : Enable the sensor.
 * @param[in, out]  dev         : Structure instance of bmi3_dev.
 *
 * @note Sensors/features that can be enabled.
 *
 *@verbatim
 *    sens_list             |  Values
 * -------------------------|-----------
 * BMI323_ACCEL             |  0
 * BMI323_GYRO              |  1
 * BMI323_SIG_MOTION        |  2
 * BMI323_ANY_MOTION        |  3
 * BMI323_NO_MOTION         |  4
 * BMI323_STEP_DETECTOR     |  5
 * BMI323_STEP_COUNTER      |  6
 * BMI323_TILT              |  7
 * BMI323_ORIENTATION       |  8
 * BMI323_FLAT              |  9
 * BMI323_WAKEUP            |  10
 * BMI323_I3C_SYC           |  11
 * BMI323_GYRO_GAIN_UPDATE  |  12
 * BMI323_TEMP              |  16
 * BMI323_GYRO_SELF_OFF     |  18
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_select_sensor(struct bmi3_feature_enable *enable, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiSensorConfig Sensor Configuration
 * @brief Enable / Disable feature configuration of the sensor
 */

/*!
 * \ingroup bmi323ApiSensorConfig
 * \page bmi323_api_bmi323_set_sensor_config bmi323_set_sensor_config
 * \code
 * int8_t bmi323_set_sensor_config(struct bmi3_sens_config *sens_cfg, uint8_t n_sens, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets the sensor/feature configuration.
 *
 * @param[in]       sens_cfg     : Structure instance of bmi3_sens_config.
 * @param[in]       n_sens       : Number of sensors selected.
 * @param[in, out]  dev          : Structure instance of bmi3_dev.
 *
 * @note Sensors/features that can be configured
 *
 *@verbatim
 *    sens_list                  |  Values
 * ------------------------------|-----------
 * BMI323_ACCEL                  |  0
 * BMI323_GYRO                   |  1
 * BMI323_SIG_MOTION             |  2
 * BMI323_ANY_MOTION             |  3
 * BMI323_NO_MOTION              |  4
 * BMI323_STEP_DETECTOR          |  5
 * BMI323_STEP_COUNTER           |  6
 * BMI323_TILT                   |  7
 * BMI323_ORIENTATION            |  8
 * BMI323_FLAT                   |  9
 * BMI323_WAKEUP                 |  10
 * BMI323_I3C_SYC                |  11
 * BMI323_GYRO_GAIN_UPDATE       |  12
 * BMI323_ALT_ACCEL              |  13
 * BMI323_ALT_GYRO               |  14
 * BMI323_ALT_AUTO_CONFIG        |  15
 * BMI323_TEMP                   |  16
 * BMI323_GYRO_SELF_OFF          |  18
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_sensor_config(struct bmi3_sens_config *sens_cfg, uint8_t n_sens, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiSensorConfig
 * \page bmi323_api_bmi323_get_sensor_config bmi323_get_sensor_config
 * \code
 * int8_t bmi323_get_sensor_config(struct bmi3_sens_config *sens_cfg, uint8_t n_sens, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets the sensor/feature configuration.
 *
 * @param[in]       sens_cfg     : Structure instance of bmi3_sens_config.
 * @param[in]       n_sens       : Number of sensors selected.
 * @param[in, out]  dev          : Structure instance of bmi3_dev.
 *
 * @note Sensors/features whose configurations can be read.
 *
 *@verbatim
 *    sens_list                  |  Values
 * ------------------------------|-----------
 * BMI323_ACCEL                  |  0
 * BMI323_GYRO                   |  1
 * BMI323_SIG_MOTION             |  2
 * BMI323_ANY_MOTION             |  3
 * BMI323_NO_MOTION              |  4
 * BMI323_STEP_DETECTOR          |  5
 * BMI323_STEP_COUNTER           |  6
 * BMI323_TILT                   |  7
 * BMI323_ORIENTATION            |  8
 * BMI323_FLAT                   |  9
 * BMI323_WAKEUP                 |  10
 * BMI323_I3C_SYC                |  11
 * BMI323_GYRO_GAIN_UPDATE       |  12
 * BMI323_ALT_ACCEL              |  13
 * BMI323_ALT_GYRO               |  14
 * BMI323_ALT_AUTO_CONFIG        |  15
 * BMI323_TEMP                   |  16
 * BMI323_GYRO_SELF_OFF          |  18
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_sensor_config(struct bmi3_sens_config *sens_cfg, uint8_t n_sens, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiSensorD Sensor Data
 * @brief Get sensor data
 */

/*!
 * \ingroup bmi323ApiSensorD
 * \page bmi323_api_bmi323_get_sensor_data bmi323_get_sensor_data
 * \code
 * int8_t bmi323_get_sensor_data(struct bmi3_sensor_data *sensor_data, uint8_t n_sens, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets the sensor/feature data for accelerometer, gyroscope,
 *  step counter, orientation, single, double, triple tap, gyroscope user-gain update
 *  and gyroscope cross sensitivity.
 *
 * @param[out] sensor_data   : Structure instance of bmi3_sensor_data.
 * @param[in]  n_sens        : Number of sensors selected.
 * @param[in]  dev           : Structure instance of bmi3_dev.
 *
 * @note Sensors/features whose data can be read
 *
 *@verbatim
 *  sens_list                 |  Values
 * ---------------------------|-----------
 * BMI323_ACCEL               |   0
 * BMI323_GYRO                |   1
 * BMI323_STEP_COUNTER        |   6
 * BMI323_ORIENTATION         |   8
 * BMI323_WAKEUP              |   10
 * BMI323_GYRO_GAIN_UPDATE    |   12
 * BMI323_GYRO_CROSS_SENSE    |   21
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_sensor_data(struct bmi3_sensor_data *sensor_data, uint8_t n_sens, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiInt
 * \page bmi323_api_bmi323_map_interrupt bmi323_map_interrupt
 * \code
 * int8_t bmi323_map_interrupt(struct bmi3_map_int map_int, struct bmi3_dev *dev);
 * \endcode
 * @details This API maps/unmaps feature interrupts to that of interrupt pins.
 *
 * @param[in] map_int      : Structure instance of bmi3_map_int.
 * @param[in] dev          : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *@verbatim
 * feature_int             |  Mask values
 * ------------------------|---------------------
 * BMI3_NO_MOTION_OUT      |  0x0003
 * BMI3_ANY_MOTION_OUT     |  0x000C
 * BMI3_FLAT_OUT           |  0x0030
 * BMI3_ORIENTATION_OUT    |  0x00C0
 * BMI3_STEP_DETECTOR_OUT  |  0x0300
 * BMI3_STEP_COUNTER_OUT   |  0x0C00
 * BMI3_SIG_MOTION_OUT     |  0x3000
 * BMI3_TILT_OUT           |  0xC000
 * BMI3_TAP_OUT            |  0x0003
 * BMI3_I3C_OUT            |  0x000C
 * BMI3_ERR_STATUS         |  0x0030
 * BMI3_TEMP_DRDY_INT      |  0x00C0
 * BMI3_GYR_DRDY_INT       |  0x0300
 * BMI3_ACC_DRDY_INT       |  0xC000
 * BMI3_FWM_INT            |  0x3000
 * BMI3_FFULL_INT          |  0xC000
 *@endverbatim
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_map_interrupt(struct bmi3_map_int map_int, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiCmd Command Register
 * @brief Write commands to the sensor
 */

/*!
 * \ingroup bmi323ApiCmd
 * \page bmi323_api_bmi323_set_command_register bmi323_set_command_register
 * \code
 * int8_t bmi323_set_command_register(uint16_t command, struct bmi3_dev *dev);
 * \endcode
 * @details This API writes the available sensor specific commands to the sensor.
 *
 * @param[in] command     : Commands to be given to the sensor.
 * @param[in] dev         : Structure instance of bmi3_dev.
 *
 *@verbatim
 * Commands                       |  Values
 * -------------------------------|---------------------
 * BMI3_CMD_SELF_TEST_TRIGGER     |  0x0100
 * BMI3_CMD_SELF_CALIB_TRIGGER    |  0x0101
 * BMI3_CMD_SELF_CALIB_ABORT      |  0x0200
 * BMI3_CMD_I3C_TCSYNC_UPDATE     |  0x0201
 * BMI3_CMD_AXIS_MAP_UPDATE       |  0x300
 * BMI3_CMD_USR_GAIN_OFFS_UPDATE  |  0x301
 * BMI3_CMD_1                     |  0x64AD
 * BMI3_CMD_2                     |  0xD3AC
 * BMI3_CMD_SOFT_RESET            |  0xDEAF
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_command_register(uint16_t command, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiSensorTime Sensor Time
 * @brief Read sensor time of the sensor
 */

/*!
 * \ingroup bmi323ApiSensorTime
 * \page bmi323_api_bmi323_get_sensor_time bmi323_get_sensor_time
 * \code
 * int8_t bmi323_get_sensor_time(uint32_t *sensor_time, struct bmi3_dev *dev);
 * \endcode
 * @details This API reads the sensor time of Sensor time gets updated
 *  with every update of data register or FIFO.
 *
 * @param[in] sensor_time : Pointer variable which stores sensor time
 * @param[in] dev : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_sensor_time(uint32_t *sensor_time, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiTemperature Temperature
 * @brief Read temperature of the sensor.
 */

/*!
 * \ingroup bmi323ApiTemperature
 * \page bmi323_api_bmi323_get_temperature_data bmi323_get_temperature_data
 * \code
 * int8_t bmi323_get_temperature_data(uint16_t *temp_data, struct bmi3_dev *dev);
 * \endcode
 * @details This API reads the raw temperature data from the register and can be
 * converted into degree celsius using the below formula.
 * Formula: temperature_value = (float)(((float)((int16_t)temperature_data)) / 512.0) + 23.0
 *
 * @note Enable accel or gyro to read temperature
 *
 * @param[out] temp_data : Pointer variable which stores the raw temperature value.
 * @param[in] dev   : Structure instance of bmi3_dev.
 *
 *
 *  @return Result of API execution status
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_temperature_data(uint16_t *temp_data, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apireadfifodata readfifodata
 * @brief Read fifo data
 */

/*!
 * \ingroup bmi3ApiFIFO
 * \page bmi3xo_api_bmi323_read_fifo_data bmi323_read_fifo_data
 * \code
 * int8_t bmi323_read_fifo_data(struct bmi3_fifo_frame *fifo, struct bmi3_dev *dev);
 * \endcode
 * @details This API reads FIFO data.
 *
 * @param[in, out] fifo     : Structure instance of bmi3_fifo_frame.
 * @param[in]      dev      : Structure instance of bmi3_dev.
 *
 * @note APS has to be disabled before calling this function.
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_read_fifo_data(struct bmi3_fifo_frame *fifo, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apiextractaccel extract accel
 * @brief Read fifo data
 */

/*!
 * \ingroup bmi323ApiFIFO
 * \page bmi323_api_bmi323_extract_accel bmi323_extract_accel
 * \code
 * int8_t bmi323_extract_accel(struct bmi3_fifo_sens_axes_data *accel_data,
 *                             struct bmi3_fifo_frame *fifo,
 *                             const struct bmi3_dev *dev)
 * \endcode
 * @details This API parses and extracts the accelerometer frames from FIFO data read by
 * the "bmi323_read_fifo_data" API and stores it in the "accel_data" structure
 * instance.
 *
 * @param[out]    accel_data   : Structure instance of bmi3_fifo_sens_axes_data
 *                               where the parsed data bytes are stored.
 * @param[in,out] fifo         : Structure instance of bmi3_fifo_frame.
 * @param[in]    dev           : Structure instance of bmi3_dev
 *
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_extract_accel(struct bmi3_fifo_sens_axes_data *accel_data,
			    struct bmi3_fifo_frame *fifo,
			    const struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apiextractgyro extract gyro
 * @brief Read fifo data
 */

/*!
 * \ingroup bmi323ApiFIFO
 * \page bmi323_api_bmi323_extract_gyro bmi323_extract_gyro
 * \code
 * int8_t bmi323_extract_gyro(struct bmi3_fifo_sens_axes_data *gyro_data,
 *                            struct bmi3_fifo_frame *fifo,
 *                            const struct bmi3_dev *dev)
 * \endcode
 * @details This API parses and extracts the gyro frames from FIFO data read by
 * the "bmi323_read_fifo_data" API and stores it in the "gyro_data" structure
 * instance.
 *
 * @param[out]    gyro_data   : Structure instance of bmi3_fifo_sens_axes_data
 *                               where the parsed data bytes are stored.
 * @param[in,out] fifo        : Structure instance of bmi3_fifo_frame.
 * @param[in]  dev            : Structure instance of bmi3_dev
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_extract_gyro(struct bmi3_fifo_sens_axes_data *gyro_data,
			   struct bmi3_fifo_frame *fifo,
			   const struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apiextracttemperature extract temperature
 * @brief Read fifo data
 */

/*!
 * \ingroup bmi323ApiFIFO
 * \page bmi323_api_bmi323_extract_temperature bmi323_extract_temperature
 * \code
 * int8_t bmi323_extract_temperature(struct bmi3_fifo_temperature_data *temp_data,
 *                                   struct bmi3_fifo_frame *fifo,
 *                                   const struct bmi3_dev *dev)
 * \endcode
 * @details This API parses and extracts the temperature frames from FIFO data read by
 * the "bmi323_read_fifo_data" API and stores it in the "temp_data" structure
 * instance.
 *
 * @param[out]    temp_data    : Structure instance of bmi3_fifo_temperature_data
 *                               where the parsed data bytes are stored.
 * @param[in,out] fifo         : Structure instance of bmi3_fifo_frame.
 * @param[in]    dev           : Structure instance of bmi3_dev
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_extract_temperature(struct bmi3_fifo_temperature_data *temp_data,
				  struct bmi3_fifo_frame *fifo,
				  const struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apisetfifowatermark fifowatermark
 * @brief Read fifo data
 */

/*!
 * \ingroup bmi323ApiFIFO
 * \page bmi323_api_bmi323_set_fifo_wm bmi323_set_fifo_wm
 * \code
 * int8_t bmi323_set_fifo_wm(uint16_t fifo_wm, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets the FIFO water-mark level which is set in the sensor.
 *
 * @param[in] fifo_wm          : Variable to set FIFO water-mark level.
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_set_fifo_wm(uint16_t fifo_wm, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apisetfifowatermark fifo watermark
 * @brief Read fifo data
 */

/*!
 * \ingroup bmi323ApiFIFO
 * \page bmi323_api_bmi323_get_fifo_wm bmi323_get_fifo_wm
 * \code
 * int8_t bmi323_get_fifo_wm(uint16_t *fifo_wm, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets the FIFO water-mark level which is set in the sensor.
 *
 * @param[out] fifo_wm         : Variable to set FIFO water-mark level.
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_get_fifo_wm(uint16_t *fifo_wm, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiFIFO FIFO
 * @brief FIFO operations of the sensor
 */

/*!
 * \ingroup bmi323ApiFIFO
 * \page bmi323_api_bmi323_set_fifo_config bmi323_set_fifo_config
 * \code
 * int8_t bmi323_set_fifo_config(uint16_t config, uint8_t enable, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets the FIFO configuration in the sensor.
 *
 * @param[in] config        : FIFO configurations to be enabled/disabled.
 * @param[in] enable        : Enable/Disable FIFO configurations.
 * @param[in] dev           : Structure instance of bmi3_dev.
 *
 *@verbatim
 *  enable        |  Description
 * ---------------|---------------
 * BMI323_DISABLE | Disables FIFO configuration.
 * BMI323_ENABLE  | Enables FIFO configuration.
 *@endverbatim
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_set_fifo_config(uint16_t config, uint8_t enable, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiFIFO FIFO
 * @brief FIFO operations of the sensor
 */

/*!
 * \ingroup bmi323ApiFIFO
 * \page bmi323_api_bmi323_get_fifo_config bmi323_get_fifo_config
 * \code
 * int8_t bmi323_get_fifo_config(uint16_t *fifo_config, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets the FIFO configuration in the sensor.
 *
 * @param[out] fifo_config  : Get FIFO configurations to be enabled/disabled.
 * @param[in] dev           : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_get_fifo_config(uint16_t *fifo_config, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apigetfifolen getfifolength
 * @brief FIFO operations of the sensor
 */

/*!
 * \ingroup bmi323ApiFIFO
 * \page bmi323_api_bmi323_get_fifo_length bmi323_get_fifo_length
 * \code
 * int8_t bmi323_get_fifo_length(uint16_t *fifo_avail_len, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets the length of FIFO data available in the sensor in
 * bytes.
 *
 * @param[out] fifo_avail_len  : Pointer variable to store the value of FIFO byte
 *                               counter.
 * @param[in]  dev             : Structure instance of bmi3_dev.
 *
 * @note The byte counter is updated each time a complete frame is read or
 * written.
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_get_fifo_length(uint16_t *fifo_avail_len, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apiselftest Perform self-test
 * @brief Performs self-test for the sensor.
 */

/*!
 * \ingroup bmi323Apiselftest
 * \page bmi323_api_bmi323_perform_self_test bmi323_perform_self_test
 * \code
 * int8_t bmi323_perform_self_test(uint8_t st_selection, struct bmi3_st_result *st_result_status , struct bmi3_dev *dev);
 * \endcode
 * @details This API Performs self-test for the sensor based on user selection.
 *
 * @param[in] st_selection     : Commands to be given to the sensor.
 * @param[in] st_result_status : Structure instance of bmi3_st_result.
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 *@verbatim
 * st_selection                   |  Values
 * -------------------------------|---------------------
 * BMI3_ST_ACCEL_ONLY             |  1
 * BMI3_ST_ACCEL_ONLY             |  2
 * BMI3_ST_BOTH_ACC_GYR           |  3
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_perform_self_test(uint8_t st_selection, struct bmi3_st_result *st_result_status, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiFeatEngErrStatus Read Feature engine error status
 * @brief Read Feature engine error status
 */

/*!
 * \ingroup bmi323ApiFeatEngErrStatus
 * \page bmi323_api_bmi323_get_feature_engine_error_status bmi323_get_feature_engine_error_status
 * \code
 * int8_t bmi323_get_feature_engine_error_status(uint8_t *feature_engine_err_reg_lsb,
 *                                             uint8_t *feature_engine_err_reg_msb,
 *                                             struct bmi3_dev *dev);
 * \endcode
 * @details This API reads the feature engine error status.
 *
 * @param[out] feature_engine_err_reg_lsb  : Variable to store feature engine error status lsb values.
 * @param[out] feature_engine_err_reg_msb  : Variable to store feature engine error status msb values.
 * @param[out] dev         : Structure instance of bmi3_dev.
 *
 *@verbatim
 * feature_engine_err_reg_lsb                     |  Values
 * -----------------------------------------------|---------------------
 * BMI3_FEAT_ENG_INACT_MASK                       |  0x0
 * BMI3_FEAT_ENG_ACT_MASK                         |  0x1
 * BMI3_INIT_CRC_ERR_MASK                         |  0x3
 * BMI3_UGAIN_OFFS_UPD_ERR_MASK                   |  0x4
 * BMI3_NO_ERROR_MASK                             |  0x5
 * BMI3_AXIS_MAP_ERR_MASK                         |  0x6
 * BMI3_TCSYNC_CONF_ERR_MASK                      |  0x8
 * BMI3_SC_ST_ABORTED_MASK                        |  0x9
 * BMI3_SC_IGNORED_MASK                           |  0xA
 * BMI3_ST_IGNORED_MASK                           |  0xB
 * BMI3_SC_ST_PRECON_ERR_MASK                     |  0xC
 * BMI3_MODE_CHANGE_WHILE_SC_ST_MASK              |  0xD
 * BMI3_POSTPONE_I3C_SYNC_MASK                    |  0xE
 * BMI3_MODE_CHANGE_WHILE_I3C_SYNC_MASK           |  0xF
 *@endverbatim
 *
 *@verbatim
 * feature_engine_err_reg_msb                     |  Values
 * -----------------------------------------------|---------------------
 * BMI3_SC_ST_COMPLETE_MASK                       |  0x0010
 * BMI3_GYRO_SC_RESULT_MASK                       |  0x0020
 * BMI3_ST_RESULT_MASK                            |  0x0040
 * BMI3_SAMPLE_RATE_ERR_MASK                      |  0x0080
 * BMI3_UGAIN_OFFS_UPD_COMPLETE_MASK              |  0x0100
 * BMI3_AXIS_MAP_COMPLETE_MASK                    |  0x0400
 * BMI3_STATE_MASK                                |  0x1800
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_feature_engine_error_status(uint8_t *feature_engine_err_reg_lsb,
					      uint8_t *feature_engine_err_reg_msb,
					      struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323WriteConfigArray Writes config array
 * @brief Writes config array and config version
 */

/*!
 * \ingroup bmi323WriteConfigArray
 * \page bmi323_api_bmi323_configure_enhanced_flexibility bmi323_configure_enhanced_flexibility
 * \code
 * int8_t bmi323_configure_enhanced_flexibility(struct bmi3_dev *dev);
 * \endcode
 * @details This API writes the config array and config version in cfg res.
 *
 * @param[in]  dev             : Structure instance of bmi3_dev.
 *
 * @return Result of API execution status
 *
 * @retval 0 -> Success
 * @retval < 0 -> Fail
 *
 */
int8_t bmi323_configure_enhanced_flexibility(struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ConfigVersion Config version
 * @brief This API is used to get the config file version of bmi323
 */

/*!
 * \ingroup bmi323ConfigVersion
 * \page bmi323_api_bmi323_get_config_version bmi323_get_config_version
 * \code
 * int8_t bmi323_get_config_version(struct bmi3_config_version *version, struct bmi3_dev *dev);
 * \endcode
 * @details This API is used to get the config file version of bmi323
 *
 * @param[in,out] version  : Structure instance of bmi3_config_version.
 * @param[in] dev          : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_config_version(struct bmi3_config_version *version, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apiselfcalibration Perform self-calibration
 * @brief Performs self-calibration for the sensor.
 */

/*!
 * \ingroup bmi323Apiselfcalibration
 * \page bmi323_api_bmi323_perform_gyro_sc bmi323_perform_gyro_sc
 * \code
 * int8_t bmi323_perform_gyro_sc(uint8_t sc_selection, uint8_t apply_corr , struct bmi3_self_calib_rslt *sc_rslt, struct bmi3_dev *dev);
 * \endcode
 * @details This API performs self-calibration for the sensor based on user selection.
 *
 * @param[in] sc_selection     : Commands to be given to the sensor.
 * @param[in] apply_corr       : Command to apply correction
 * @param[in] sc_rslt          : Structure instance of bmi3_self_calib_rslt.
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 *@verbatim
 * sc_selection                   |  Values
 * -------------------------------|---------------------
 * BMI3_SC_SENSITIVITY_EN         |  1
 * BMI3_SC_OFFSET_EN              |  2
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_perform_gyro_sc(uint8_t sc_selection,
			      uint8_t apply_corr,
			      struct bmi3_self_calib_rslt *sc_rslt,
			      struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Apii3c_sync i3c_sync
 * @brief i3c_sync configurations
 */

/*!
 * \ingroup bmi323Apii3c_sync
 * \page bmi323_api_set_i3c_tc_sync_tph set_i3c_tc_sync_tph
 * \code
 * int8_t set_i3c_tc_sync_tph(uint16_t sample_rate, struct bmi3_dev *dev);
 * \endcode
 * @details This API used to set the i3c_sync data sample rate
 *
 * @param[in] sample_rate     : To set the data sample rate.
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_i3c_tc_sync_tph(uint16_t sample_rate, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323Apii3c_sync
 * \page bmi323_api_get_i3c_tc_sync_tph get_i3c_tc_sync_tph
 * \code
 * int8_t get_i3c_tc_sync_tph(uint16_t *sample_rate, struct bmi3_dev *dev);
 * \endcode
 * @details This API used to get the i3c_sync data sample rate.
 *
 * @param[out] sample_rate     : To get the data sample rate.
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_i3c_tc_sync_tph(uint16_t *sample_rate, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323Apii3c_sync
 * \page bmi323_api_set_i3c_tc_sync_tu set_i3c_tc_sync_tu
 * \code
 * int8_t set_i3c_tc_sync_tu(uint8_t delay_time, struct bmi3_dev *dev);
 * \endcode
 * @details This API used to set the TU(time unit) value is used to scale the delay time payload.
 *
 * @param[in] delay_time       : To set the delay time of i3c_sync.
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_i3c_tc_sync_tu(uint8_t delay_time, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323Apii3c_sync
 * \page bmi323_api_get_i3c_tc_sync_tu get_i3c_tc_sync_tu
 * \code
 * int8_t get_i3c_tc_sync_tu(uint8_t *delay_time, struct bmi3_dev *dev);
 * \endcode
 * @details This API used to enable the i3c_sync.
 *
 * @param[out] delay_time       : To get the delay time of i3c_sync.
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_i3c_tc_sync_tu(uint8_t *delay_time, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323Apii3c_sync
 * \page bmi323_api_set_i3c_tc_sync_odr set_i3c_tc_sync_odr
 * \code
 * int8_t set_i3c_tc_sync_odr(uint8_t odr, struct bmi3_dev *dev);
 * \endcode
 * @details This API used to set the i3c_sync ODR.
 *
 * @param[in] odr       : To set the i3c_sync ODR.
 * @param[in] dev       : Structure instance of bmi3_dev.
 *
 *@verbatim
 *            Value            |  SYNC_ODR
 * ----------------------------|-------------
 * BMI3_I3C_SYNC_ODR_6_25HZ    |  6.25Hz
 * BMI3_I3C_SYNC_ODR_12_5HZ    |  12.5Hz
 * BMI3_I3C_SYNC_ODR_25HZ      |  25Hz
 * BMI3_I3C_SYNC_ODR_50HZ      |  50Hz
 * BMI3_I3C_SYNC_ODR_100HZ     |  100Hz
 * BMI3_I3C_SYNC_ODR_200HZ     |  200Hz
 * BMI3_I3C_SYNC_ODR_400HZ     |  400Hz
 * BMI3_I3C_SYNC_ODR_800HZ     |  800Hz
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_i3c_tc_sync_odr(uint8_t odr, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323Apii3c_sync
 * \page bmi323_api_get_i3c_tc_sync_odr get_i3c_tc_sync_odr
 * \code
 * int8_t get_i3c_tc_sync_odr(uint8_t *odr, struct bmi3_dev *dev);
 * \endcode
 * @details This API used to enable the i3c_sync filter.
 *
 * @param[out] odr       : To get the i3c_sync ODR.
 * @param[in] dev        : Structure instance of bmi3_dev.
 *
 *@verbatim
 *            Value            |  SYNC_ODR
 * ----------------------------|-------------
 * BMI3_I3C_SYNC_ODR_6_25HZ    |  6.25Hz
 * BMI3_I3C_SYNC_ODR_12_5HZ    |  12.5Hz
 * BMI3_I3C_SYNC_ODR_25HZ      |  25Hz
 * BMI3_I3C_SYNC_ODR_50HZ      |  50Hz
 * BMI3_I3C_SYNC_ODR_100HZ     |  100Hz
 * BMI3_I3C_SYNC_ODR_200HZ     |  200Hz
 * BMI3_I3C_SYNC_ODR_400HZ     |  400Hz
 * BMI3_I3C_SYNC_ODR_800HZ     |  800Hz
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_i3c_tc_sync_odr(uint8_t *odr, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323Apii3c_sync
 * \page bmi323_api_set_i3c_sync_i3c_tc_res set_i3c_sync_i3c_tc_res
 * \code
 * int8_t set_i3c_sync_i3c_tc_res(uint8_t i3c_tc_res, struct bmi3_dev *dev);
 * \endcode
 * @details This API used to enable the i3c_sync i3c_tc_res.
 *
 * @param[out] i3c_tc_res       : To enable the i3c_sync i3c_tc_res.
 * @param[in] dev               : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_i3c_sync_i3c_tc_res(uint8_t i3c_tc_res, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323Apii3c_sync
 * \page bmi323_api_get_i3c_sync_i3c_tc_res get_i3c_sync_i3c_tc_res
 * \code
 * int8_t get_i3c_sync_i3c_tc_res(uint8_t *i3c_tc_res, struct bmi3_dev *dev);
 * \endcode
 * @details This API used to get the i3c_sync i3c_tc_res.
 *
 * @param[out] i3c_tc_res     : To get the i3c_sync i3c_tc_res.
 * @param[in] dev             : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_i3c_sync_i3c_tc_res(uint8_t *i3c_tc_res, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Alternateconfig Alternate configuration control
 * @brief Enable/Disable alternate configuration for accel and gyro.
 */

/*!
 * \ingroup bmi323Alternateconfig
 * \page bmi323_api_bmi323_alternate_config_ctrl bmi323_alternate_config_ctrl
 * \code
 * int8_t bmi323_alternate_config_ctrl(uint8_t config_en, uint8_t alt_rst_conf, struct bmi3_dev *dev);
 * \endcode
 * @details This API is used to enable accel and gyro for alternate configuration
 *
 * @param[in] config_en        : Variable to enable alternate configuration for accel and gyro or both
 * @param[in] alt_rst_conf     : Variable to reset alternate configuration
 * @param[in] dev              : Structure instance of bmi3_dev.
 *
 *@verbatim
 * config_en                                |  Values
 * -----------------------------------------|---------------------
 * BMI3_ALT_ACC_ENABLE                      |  1
 * BMI3_ALT_GYR_ENABLE                      |  16
 *@endverbatim
 *
 *@verbatim
 * alt_rst_conf                             |  Values
 * -----------------------------------------|---------------------
 * BMI3_ALT_CONF_RESET_OFF                  |  0
 * BMI3_ALT_CONF_RESET_ON                   |  1
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_alternate_config_ctrl(uint8_t config_en, uint8_t alt_rst_conf, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323Alternatestatus Read alternate config status
 * @brief  Read alternate configuration status
 */

/*!
 * \ingroup bmi323Alternatestatus
 * \page bmi323_api_bmi323_read_alternate_status bmi323_read_alternate_status
 * \code
 * int8_t bmi323_read_alternate_status(struct bmi3_alt_status *alt_status, struct bmi3_dev *dev);
 * \endcode
 * @details This API is used to read the status of alternate configuration
 *
 * @param[in] alt_status  : Structure instance of bmi3_alt_status.
 * @param[in] dev         : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_read_alternate_status(struct bmi3_alt_status *alt_status, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi3x0ApiAccGyrOffsetGain Perform accel gyro offset dgain
 * @brief Performs accel gyro offset dgain for the sensor.
 */

/*!
 * \ingroup bmi3x0ApiAccGyrOffsetGain
 * \page bmi323_api_bmi323_get_acc_dp_off_dgain bmi323_get_acc_dp_off_dgain
 * \code
 * int8_t bmi323_get_acc_dp_off_dgain(struct bmi3_acc_usr_gain_offset *acc_dp_gain_offset, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets offset dgain for the sensor which stores self-calibrated values for accel.
 *
 * @param[in, out] acc_dp_gain_offset  : Structure instance of bmi3_acc_dp_gain_offset.
 * @param[in] dev                      : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_acc_dp_off_dgain(struct bmi3_acc_dp_gain_offset *acc_dp_gain_offset, struct bmi3_dev *dev);

/*!
 * \ingroup bmi3x0ApiAccGyrOffsetGain
 * \page bmi323_api_bmi323_get_gyro_dp_off_dgain bmi323_get_gyro_dp_off_dgain
 * \code
 * int8_t bmi323_get_gyro_dp_off_dgain(struct bmi3_gyr_dp_gain_offset *gyr_dp_gain_offset, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets offset dgain for the sensor which stores self-calibrated values for gyro.
 *
 * @param[in, out] gyr_dp_gain_offset  : Structure instance of bmi3_gyr_dp_gain_offset.
 * @param[in] dev                      : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_gyro_dp_off_dgain(struct bmi3_gyr_dp_gain_offset *gyr_dp_gain_offset, struct bmi3_dev *dev);

/*!
 * \ingroup bmi3x0ApiAccGyrOffsetGain
 * \page bmi323_api_bmi323_set_acc_dp_off_dgain bmi323_set_acc_dp_off_dgain
 * \code
 * int8_t bmi323_set_acc_dp_off_dgain(const struct bmi3_acc_dp_gain_offset *acc_dp_gain_offset, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets offset dgain for the sensor which stores self-calibrated values for accel.
 *
 * @param[in, out] acc_dp_gain_offset  : Structure instance of bmi3_acc_dp_gain_offset.
 * @param[in] dev                      : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_acc_dp_off_dgain(const struct bmi3_acc_dp_gain_offset *acc_dp_gain_offset, struct bmi3_dev *dev);

/*!
 * \ingroup bmi3x0ApiAccGyrOffsetGain
 * \page bmi323_api_bmi323_set_gyro_dp_off_dgain bmi323_set_gyro_dp_off_dgain
 * \code
 * int8_t bmi323_set_gyro_dp_off_dgain(const struct bmi3_gyr_dp_gain_offset *gyr_dp_gain_offset, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets offset dgain for the sensor which stores self-calibrated values for gyro.
 *
 * @param[in, out] gyr_dp_gain_offset  : Structure instance of bmi3_gyr_dp_gain_offset.
 * @param[in] dev                      : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_gyro_dp_off_dgain(const struct bmi3_gyr_dp_gain_offset *gyr_dp_gain_offset, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiUserOffset Perform user offset dgain
 * @brief Performs user offset dgain for the sensor.
 */

/*!
 * \ingroup bmi323ApiUserOffset
 * \page bmi323_api_bmi323_get_user_acc_off_dgain bmi323_get_user_acc_off_dgain
 * \code
 * int8_t bmi323_get_user_acc_off_dgain(struct bmi3_acc_usr_gain_offset *acc_usr_gain_offset, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets user offset dgain for the sensor which stores self-calibrated values for accel.
 *
 * @param[in, out] acc_usr_gain_offset  : Structure instance of bmi3_acc_usr_gain_offset.
 * @param[in] dev                       : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_user_acc_off_dgain(struct bmi3_acc_usr_gain_offset *acc_usr_gain_offset, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiUserOffset
 * \page bmi323_api_bmi323_get_user_gyro_off_dgain bmi323_get_user_gyro_off_dgain
 * \code
 * int8_t bmi323_get_user_gyro_off_dgain(struct bmi3_gyr_usr_gain_offset *gyr_usr_gain_offset, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets user offset dgain for the sensor which stores self-calibrated values for gyro.
 *
 * @param[in, out] gyr_usr_gain_offset  : Structure instance of bmi3_gyr_usr_gain_offset.
 * @param[in] dev                       : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_user_gyro_off_dgain(struct bmi3_gyr_usr_gain_offset *gyr_usr_gain_offset, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiUserOffset
 * \page bmi323_api_bmi323_set_user_acc_off_dgain bmi323_set_user_acc_off_dgain
 * \code
 * int8_t bmi323_set_user_acc_off_dgain(const struct bmi3_acc_usr_gain_offset *acc_usr_gain_offset, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets user offset dgain for the sensor which stores self-calibrated values for accel.
 *
 * @param[in, out] acc_usr_gain_offset  : Structure instance of bmi3_acc_usr_gain_offset.
 * @param[in] dev                       : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_user_acc_off_dgain(const struct bmi3_acc_usr_gain_offset *acc_usr_gain_offset, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiUserOffset
 * \page bmi323_api_bmi323_set_user_gyr_off_dgain bmi323_set_user_gyr_off_dgain
 * \code
 * int8_t bmi323_set_user_gyr_off_dgain(const struct bmi3_gyr_usr_gain_offset *gyr_usr_gain_offset, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets user offset dgain for the sensor which stores self-calibrated values for gyro.
 *
 * @param[in, out] gyr_usr_gain_offset  : Structure instance of bmi3_gyr_usr_gain_offset.
 * @param[in] dev                       : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_user_gyr_off_dgain(const struct bmi3_gyr_usr_gain_offset *gyr_usr_gain_offset, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiFOC FOC
 * @brief FOC operations of the sensor
 */

/*!
 * \ingroup bmi323ApiFOC
 * \page bmi323_api_bmi323_perform_accel_foc bmi323_perform_accel_foc
 * \code
 * int8_t bmi323_perform_accel_foc(const struct bmi3_accel_foc_g_value *accel_g_value, struct bmi3_dev *dev);
 * \endcode
 * @details This API performs Fast Offset Compensation for accelerometer.
 *
 * @param[in] accel_g_value : This parameter selects the accel FOC
 * axis to be performed
 *
 * Input format is {x, y, z, sign}. '1' to enable. '0' to disable
 *
 * Eg: To choose x axis  {1, 0, 0, 0}
 * Eg: To choose -x axis {1, 0, 0, 1}
 *
 * @param[in]  dev              : Structure instance of bmi3_dev.
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_perform_accel_foc(const struct bmi3_accel_foc_g_value *accel_g_value, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323ApiStatus Sensor Status
 * @brief Get sensor status
 */

/*!
 * \ingroup bmi323ApiStatus
 * \page bmi323_api_bmi323_get_sensor_status bmi323_get_sensor_status
 * \code
 * int8_t bmi323_get_sensor_status(uint8_t *status, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets the data ready status of power on reset, accelerometer, gyroscope,
 * and temperature
 *
 * @param[out] status     : Pointer variable to the status.
 * @param[in]  dev        : Structure instance of bmi3_dev.
 *
 *@verbatim
 * Value    |  Status
 * ---------|---------------------
 * 0x01     |  BMI3_STATUS_POR
 * 0x20     |  BMI3_STATUS_DRDY_TEMP
 * 0x40     |  BMI3_STATUS_DRDY_GYR
 * 0x80     |  BMI3_STATUS_DRDY_ACC
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_sensor_status(uint16_t *status, struct bmi3_dev *dev);

/*!
 * \ingroup bmi323ApiInt
 * \page bmi323_api_bmi323_get_i3c_ibi_status bmi323_get_i3c_ibi_status
 * \code
 * int8_t bmi323_get_i3c_ibi_status(uint16_t *int_status, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets the i3c ibi status of both feature and data
 * interrupts.
 *
 * @param[out] int_status    : Pointer to get the status of the interrupts.
 * @param[in]  dev           : Structure instance of bmi3_dev.
 *
 *@verbatim
 * int_status   |  Status
 * -------------|------------
 * 0x0001       |  BMI3_IBI_STATUS_NO_MOTION
 * 0x0002       |  BMI3_IBI_STATUS_ANY_MOTION
 * 0x0004       |  BMI3_IBI_STATUS_FLAT
 * 0x0008       |  BMI3_IBI_STATUS_ORIENTATION
 * 0x0010       |  BMI3_IBI_STATUS_STEP_DETECTOR
 * 0x0020       |  BMI3_IBI_STATUS_STEP_COUNTER
 * 0x0040       |  BMI3_IBI_STATUS_SIG_MOTION
 * 0x0080       |  BMI3_IBI_STATUS_TILT
 * 0x0100       |  BMI3_IBI_STATUS_TAP
 * 0x0200       |  BMI3_IBI_STATUS_I3C
 * 0x0400       |  BMI3_IBI_STATUS_ERR_STATUS
 * 0x0800       |  BMI3_IBI_STATUS_TEMP_DRDY
 * 0x1000       |  BMI3_IBI_STATUS_GYR_DRDY
 * 0x2000       |  BMI3_IBI_STATUS_ACC_DRDY
 * 0x4000       |  BMI3_IBI_STATUS_FWM
 * 0x8000       |  BMI3_IBI_FFULL
 *@endverbatim
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_i3c_ibi_status(uint16_t *int_status, struct bmi3_dev *dev);

/**
 * \ingroup bmi323
 * \defgroup bmi323AccGyrUsrGainOff Accel and gyro user gain and offset
 * @brief Accel and gyro user gain and offset
 */

/*!
 * \ingroup bmi323AccGyrUsrGainOff
 * \page bmi323_api_bmi323_get_acc_gyr_off_gain_reset bmi323_get_acc_gyr_off_gain_reset
 * \code
 * int8_t bmi323_get_acc_gyr_off_gain_reset(uint8_t *acc_off_gain_reset, uint8_t *gyr_off_gain_reset, struct bmi3_dev *dev);
 * \endcode
 * @details This API gets accel gyro offset gain reset values.
 *
 * @param[out] acc_off_gain_reset     : Pointer variable to store accel offset gain reset value.
 * @param[out] gyr_off_gain_reset     : Pointer variable to store gyro offset gain reset value.
 * @param[in]  dev                    : Structure instance of bmi3_dev.
 *
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_get_acc_gyr_off_gain_reset(uint8_t *acc_off_gain_reset, uint8_t *gyr_off_gain_reset,
					 struct bmi3_dev *dev);

/*!
 * \ingroup bmi323AccGyrUsrGainOff
 * \page bmi323_api_bmi323_set_acc_gyr_off_gain_reset bmi323_set_acc_gyr_off_gain_reset
 * \code
 * int8_t bmi323_set_acc_gyr_off_gain_reset(uint8_t acc_off_gain_reset, uint8_t gyr_off_gain_reset, struct bmi3_dev *dev);
 * \endcode
 * @details This API sets accel gyro offset gain reset values.
 *
 * @param[out] acc_off_gain_reset     : Variable to store accel offset gain reset value.
 * @param[out] gyr_off_gain_reset     : Variable to store gyro offset gain reset value.
 * @param[in]  dev                    : Structure instance of bmi3_dev.
 *
 *
 *  @return Result of API execution status
 *
 *  @retval 0 -> Success
 *  @retval < 0 -> Fail
 */
int8_t bmi323_set_acc_gyr_off_gain_reset(uint8_t acc_off_gain_reset, uint8_t gyr_off_gain_reset, struct bmi3_dev *dev);

#endif /* End of _BMI323_H */

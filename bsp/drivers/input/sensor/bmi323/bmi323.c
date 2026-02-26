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
 * @file       bmi323.c
 * @date       2022-08-31
 * @version    v2.0.0
 *
 */

/******************************************************************************/

/*!  @name          Header Files                                  */
/******************************************************************************/

#include "bmi323.h"
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/kernel.h>
#else
#include "stdio.h"
#endif

/***************************************************************************/

/*!              Static Variable
 ****************************************************************************/

/*! Any-motion context parameter set for smart phone, wearable and hearable */
static uint16_t any_motion_param_set[BMI323_PARAM_LIMIT_CONTEXT][BMI323_PARAM_LIMIT_ANY_MOT] = {
{ 8, 1, 5, 250, 5 }, { 8, 1, 5, 250, 5 }, { 8, 1, 6, 250, 5 } };

/*! No-motion context parameter set for smart phone, wearable and hearable */
static uint16_t no_motion_param_set[BMI323_PARAM_LIMIT_CONTEXT][BMI323_PARAM_LIMIT_NO_MOT] = {
{ 30, 1, 5, 250, 5 }, { 30, 1, 3, 250, 5 }, { 10, 1, 3, 250, 5 } };

/*! Tap context parameter set for smart phone, wearable and hearable */
static uint16_t tap_param_set[BMI323_PARAM_LIMIT_CONTEXT][BMI323_PARAM_LIMIT_WAKEUP] = {
{ 2, 1, 6, 1, 143, 25, 4, 6, 8, 6 }, { 2, 1, 6, 2, 250, 25, 4, 6, 8, 6 }, { 2, 1, 6, 1, 750, 25, 4, 6, 8, 6 } };

/*! Step counter context parameter set for smart phone, wearable and hearable */
static uint16_t step_counter_param_set[BMI323_PARAM_LIMIT_CONTEXT][BMI323_PARAM_LIMIT_STEP_COUNT] = {
{ 0, 0, 306, 61900, 132, 55608, 60104, 64852, 7, 1, 256, 12, 12, 3, 3900, 74, 160, 0, 0, 0, 0, 0 },
  { 0, 0, 307, 61932, 80, 55706, 63102, 58982, 4, 1, 256, 15, 14, 3, 3900, 150, 160, 1, 3, 1, 10, 3 },
  { 0, 0, 307, 61932, 133, 55706, 62260, 58982, 7, 1, 256, 13, 12, 3, 3900, 74, 160, 1, 3, 1, 8, 2 } };

/*! Sig-motion context parameter set for smart phone, wearable and hearable */
static uint16_t sig_motion_param_set[BMI323_PARAM_LIMIT_CONTEXT][BMI323_PARAM_LIMIT_SIG_MOT] = {
{ 250, 60, 11, 595, 17 }, { 250, 150, 8, 595, 17 }, { 250, 38, 8, 400, 17 } };

/*! Orientation context parameter set for smart phone, wearable and hearable */
static uint16_t orientation_param_set[BMI323_PARAM_LIMIT_CONTEXT][BMI323_PARAM_LIMIT_ORIENT] = {
{ 0, 0, 3, 38, 10, 50, 32 }, { 0, 0, 3, 38, 10, 50, 32 }, { 0, 0, 3, 38, 10, 50, 32 } };

/******************************************************************************/

/*!         Local Function Prototypes
 ******************************************************************************/

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

/******************************************************************************/
/*!  @name      User Interface Definitions                            */
/******************************************************************************/

/*!
 * @brief This API is the entry point for bmi323 sensor. It reads and validates the
 * chip-id of the sensor.
 */
int8_t bmi323_init(struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Null-pointer check */
    rslt = null_ptr_check(dev);

    if (rslt == BMI323_OK)
    {
	rslt = bmi3_init(dev);
    }

    if (rslt == BMI323_OK)
    {
	/* Validate chip-id */
	if (dev->chip_id == BMI323_CHIP_ID)
	{
	    /* Assign resolution to the structure */
	    dev->resolution = BMI323_16_BIT_RESOLUTION;
        } else
	{
	    rslt = BMI323_E_DEV_NOT_FOUND;
	}
    }

    if (rslt == BMI323_OK)
    {
	rslt = bmi323_context_switch_selection(BMI323_WEARABLE_SEL, dev);
    }

    return rslt;
}

/*!
 * @brief This API reads the data from the given register address of bmi323
 * sensor.
 *
 * @note For most of the registers auto address increment applies, with the
 * exception of a few special registers, which trap the address. For e.g.,
 * Register address - 0x03.
 */
int8_t bmi323_get_regs(uint8_t reg_addr, uint8_t *data, uint16_t len, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_regs(reg_addr, data, len, dev);

    return rslt;
}

/*!
 * @brief This API writes data to the given register address of bmi323 sensor.
 */
int8_t bmi323_set_regs(uint8_t reg_addr, const uint8_t *data, uint16_t len, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_regs(reg_addr, data, len, dev);

    return rslt;
}

/*!
 * @brief This API resets bmi323 sensor. All registers are overwritten with
 * their default values.
 */
int8_t bmi323_soft_reset(struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_soft_reset(dev);

    return rslt;
}

/*!
 * @brief This API writes the available sensor specific commands to the sensor.
 */
int8_t bmi323_set_command_register(uint16_t command, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Set the command in the command register */
    rslt = bmi3_set_command_register(command, dev);

    return rslt;
}

/*!
 * @brief This API gets the interrupt 1 status of both feature and data
 * interrupts
 */
int8_t bmi323_get_int1_status(uint16_t *int_status, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Get the interrupt status */
    rslt = bmi3_get_int1_status(int_status, dev);

    return rslt;
}

/*!
 * @brief This API gets the interrupt 2 status of both feature and data
 * interrupts
 */
int8_t bmi323_get_int2_status(uint16_t *int_status, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Get the interrupt status */
    rslt = bmi3_get_int2_status(int_status, dev);

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
int8_t bmi323_get_remap_axes(struct bmi3_axes_remap *remapped_axis, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Get remapped axes */
    rslt = bmi3_get_remap_axes(remapped_axis, dev);

    return rslt;
}

/*!
 * @brief This API sets the re-mapped x, y and z axes to the sensor and
 * updates them in the device structure.
 */
int8_t bmi323_set_remap_axes(const struct bmi3_axes_remap remapped_axis, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Set remapped axes */
    rslt = bmi3_set_remap_axes(remapped_axis, dev);

    return rslt;
}

/*!
 * @brief This API sets the sensor/feature configuration.
 */
int8_t bmi323_set_sensor_config(struct bmi3_sens_config *sens_cfg, uint8_t n_sens, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_sensor_config(sens_cfg, n_sens, dev);

    return rslt;
}

/*!
 * @brief This API gets the sensor/feature configuration.
 */
int8_t bmi323_get_sensor_config(struct bmi3_sens_config *sens_cfg, uint8_t n_sens, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_sensor_config(sens_cfg, n_sens, dev);

    return rslt;
}

/*!
 * @brief This API maps/un-maps data interrupts to that of interrupt pins.
 */
int8_t bmi323_map_interrupt(struct bmi3_map_int map_int, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Read map interrupt data */
    rslt = bmi3_map_interrupt(map_int, dev);

    return rslt;
}

/*!
 * @brief This API selects the sensors/features to be enabled or disabled.
 */
int8_t bmi323_select_sensor(struct bmi3_feature_enable *enable, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_select_sensor(enable, dev);

    return rslt;
}

/*!
 * @brief This API gets the sensor/feature data for accelerometer, gyroscope,
 * step counter, high-g, gyroscope user-gain update,
 * orientation, gyroscope cross sensitivity and error status for NVM and VFRM.
 */
int8_t bmi323_get_sensor_data(struct bmi3_sensor_data *sensor_data, uint8_t n_sens, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_sensor_data(sensor_data, n_sens, dev);

    return rslt;
}

/*!
 *  @brief This API reads the error status from the sensor.
 */
int8_t bmi323_get_error_status(struct bmi3_err_reg *err_reg, struct bmi3_dev *dev)
{
    /* Variable to hold execution status */
    int8_t rslt;

    rslt = bmi3_get_error_status(err_reg, dev);

    return rslt;
}

/*!
 *  @brief This API reads the feature engine error status from the sensor.
 */
int8_t bmi323_get_feature_engine_error_status(uint8_t *feature_engine_err_reg_lsb,
					      uint8_t *feature_engine_err_reg_msb,
					      struct bmi3_dev *dev)
{
    /* Variable to hold execution status */
    int8_t rslt;

    /* Read the feature engine error codes */
    rslt = bmi3_get_feature_engine_error_status(feature_engine_err_reg_lsb, feature_engine_err_reg_msb, dev);

    return rslt;
}

/*!
 * @brief This API sets:
 *        1) The input output configuration of the selected interrupt pin:
 *           INT1 or INT2.
 *        2) The interrupt mode: permanently latched or non-latched.
 */
int8_t bmi323_set_int_pin_config(const struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_int_pin_config(int_cfg, dev);

    return rslt;
}

/*!
 * @brief This API gets:
 *        1) The input output configuration of the selected interrupt pin:
 *           INT1 or INT2.
 *        2) The interrupt mode: permanently latched or non-latched.
 */
int8_t bmi323_get_int_pin_config(struct bmi3_int_pin_config *int_cfg, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_int_pin_config(int_cfg, dev);

    return rslt;
}

/*!
 * @brief This API is used to get the sensor time.
 */
int8_t bmi323_get_sensor_time(uint32_t *sensor_time, struct bmi3_dev *dev)
{
    int8_t rslt;

    rslt = bmi3_get_sensor_time(sensor_time, dev);

    return rslt;
}

/*!
 * @brief This API reads the raw temperature data from the register and can be
 * converted into degree celsius.
 */
int8_t bmi323_get_temperature_data(uint16_t *temp_data, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_temperature_data(temp_data, dev);

    return rslt;
}

/*!
 * @brief This API reads the FIFO data.
 */
int8_t bmi323_read_fifo_data(struct bmi3_fifo_frame *fifo, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_read_fifo_data(fifo, dev);

    return rslt;
}

/*!
 * @brief This API parses and extracts the accelerometer frames from FIFO data
 * read by the "bmi323_read_fifo_data" API and stores it in the "accel_data"
 * structure instance.
 */
int8_t bmi323_extract_accel(struct bmi3_fifo_sens_axes_data *accel_data,
			    struct bmi3_fifo_frame *fifo,
			    const struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_extract_accel(accel_data, fifo, dev);

    return rslt;
}

/*!
 * @brief This API parses and extracts the temperature frames from FIFO data
 * read by the "bmi323_read_fifo_data" API and stores it in the "temp_data"
 * structure instance.
 */
int8_t bmi323_extract_temperature(struct bmi3_fifo_temperature_data *temp_data,
				  struct bmi3_fifo_frame *fifo,
				  const struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_extract_temperature(temp_data, fifo, dev);

    return rslt;
}

/*!
 * @brief This API parses and extracts the gyro frames from FIFO data
 * read by the "bmi323_read_fifo_data" API and stores it in the "gyro_data"
 * structure instance.
 */
int8_t bmi323_extract_gyro(struct bmi3_fifo_sens_axes_data *gyro_data,
			   struct bmi3_fifo_frame *fifo,
			   const struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_extract_gyro(gyro_data, fifo, dev);

    return rslt;
}

/*!
 * @brief This API sets the FIFO water-mark level in the sensor.
 */
int8_t bmi323_set_fifo_wm(uint16_t fifo_wm, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_fifo_wm(fifo_wm, dev);

    return rslt;
}

/*!
 * @brief This API reads the FIFO water-mark level set in the sensor.
 */
int8_t bmi323_get_fifo_wm(uint16_t *fifo_wm, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_fifo_wm(fifo_wm, dev);

    return rslt;
}

/*!
 * @brief This API sets the FIFO configuration in the sensor.
 */
int8_t bmi323_set_fifo_config(uint16_t config, uint8_t enable, struct bmi3_dev *dev)
{
    int8_t rslt;

    rslt = bmi3_set_fifo_config(config, enable, dev);

    return rslt;
}

/*!
 * @brief This API reads the FIFO configuration from the sensor.
 */
int8_t bmi323_get_fifo_config(uint16_t *fifo_config, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_fifo_config(fifo_config, dev);

    return rslt;
}

/*!
 * @brief This API gets the length of FIFO data available in the sensor in
 * bytes.
 */
int8_t bmi323_get_fifo_length(uint16_t *fifo_avail_len, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_fifo_length(fifo_avail_len, dev);

    return rslt;
}

/*!
 * @brief This API writes the configurations of context feature for smart phone, wearables and hearables.
 */
int8_t bmi323_context_switch_selection(uint8_t context_sel, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    struct bmi3_sens_config sens_cfg[BMI323_MAX_FEATURE];

    uint8_t index = 0;

    if (context_sel < BMI323_SEL_MAX)
    {
	/* Set any-motion configuration */
	sens_cfg[0].type = BMI323_ANY_MOTION;
	sens_cfg[0].cfg.any_motion.slope_thres = any_motion_param_set[context_sel][index++];
	sens_cfg[0].cfg.any_motion.acc_ref_up = (uint8_t)(any_motion_param_set[context_sel][index++]);
	sens_cfg[0].cfg.any_motion.hysteresis = any_motion_param_set[context_sel][index++];
	sens_cfg[0].cfg.any_motion.duration = any_motion_param_set[context_sel][index++];
	sens_cfg[0].cfg.any_motion.wait_time = any_motion_param_set[context_sel][index++];

	/* Set no-motion configuration */
	index = 0;
	sens_cfg[1].type = BMI323_NO_MOTION;
	sens_cfg[1].cfg.no_motion.slope_thres = no_motion_param_set[context_sel][index++];
	sens_cfg[1].cfg.no_motion.acc_ref_up = (uint8_t)(no_motion_param_set[context_sel][index++]);
	sens_cfg[1].cfg.no_motion.hysteresis = no_motion_param_set[context_sel][index++];
	sens_cfg[1].cfg.no_motion.duration = no_motion_param_set[context_sel][index++];
	sens_cfg[1].cfg.no_motion.wait_time = no_motion_param_set[context_sel][index++];

	/* Set tap configuration */
	index = 0;
	sens_cfg[2].type = BMI323_TAP;
	sens_cfg[2].cfg.tap.axis_sel = (uint8_t)tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.wait_for_timeout = (uint8_t)tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.max_peaks_for_tap = (uint8_t)tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.mode = (uint8_t)tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.tap_peak_thres = tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.max_gest_dur = (uint8_t)tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.max_dur_between_peaks = (uint8_t)tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.tap_shock_settling_dur = (uint8_t)tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.min_quite_dur_between_taps = (uint8_t)tap_param_set[context_sel][index++];
	sens_cfg[2].cfg.tap.quite_time_after_gest = (uint8_t)tap_param_set[context_sel][index++];

	/* Set step counter configuration */
	index = 0;
	sens_cfg[3].type = BMI323_STEP_COUNTER;
	sens_cfg[3].cfg.step_counter.watermark_level = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.reset_counter = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.env_min_dist_up = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.env_coef_up = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.env_min_dist_down = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.env_coef_down = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.mean_val_decay = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.mean_step_dur = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.step_buffer_size = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.filter_cascade_enabled = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.step_counter_increment = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.peak_duration_min_walking = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.peak_duration_min_running = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.activity_detection_factor = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.activity_detection_thres = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.step_duration_max = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.step_duration_window = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.step_duration_pp_enabled = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.step_duration_thres = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.mean_crossing_pp_enabled = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.mcr_threshold = step_counter_param_set[context_sel][index++];
	sens_cfg[3].cfg.step_counter.sc_12_res = step_counter_param_set[context_sel][index++];

	/* Set significant motion configuration */
	index = 0;
	sens_cfg[4].type = BMI323_SIG_MOTION;
	sens_cfg[4].cfg.sig_motion.block_size = sig_motion_param_set[context_sel][index++];
	sens_cfg[4].cfg.sig_motion.peak_2_peak_min = sig_motion_param_set[context_sel][index++];
	sens_cfg[4].cfg.sig_motion.mcr_min = (uint8_t)sig_motion_param_set[context_sel][index++];
	sens_cfg[4].cfg.sig_motion.peak_2_peak_max = sig_motion_param_set[context_sel][index++];
	sens_cfg[4].cfg.sig_motion.mcr_max = (uint8_t)sig_motion_param_set[context_sel][index++];

	/* Set orientation configuration */
	index = 0;
	sens_cfg[5].type = BMI323_ORIENTATION;
	sens_cfg[5].cfg.orientation.ud_en = (uint8_t)orientation_param_set[context_sel][index++];
	sens_cfg[5].cfg.orientation.mode = (uint8_t)orientation_param_set[context_sel][index++];
	sens_cfg[5].cfg.orientation.blocking = (uint8_t)orientation_param_set[context_sel][index++];
	sens_cfg[5].cfg.orientation.theta = (uint8_t)orientation_param_set[context_sel][index++];
	sens_cfg[5].cfg.orientation.hold_time = (uint8_t)orientation_param_set[context_sel][index++];
	sens_cfg[5].cfg.orientation.slope_thres = (uint8_t)orientation_param_set[context_sel][index++];
	sens_cfg[5].cfg.orientation.hysteresis = (uint8_t)orientation_param_set[context_sel][index++];

	/* Set the context configurations */
	rslt = bmi323_set_sensor_config(sens_cfg, BMI323_MAX_FEATURE, dev);
    } else
    {
	rslt = BMI323_E_INVALID_CONTEXT_SEL;
    }

    return rslt;
}

/*!
 * @brief This API is used to perform the self-test for either accel or gyro or both.
 */
int8_t bmi323_perform_self_test(uint8_t st_selection, struct bmi3_st_result *st_result_status, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_perform_self_test(st_selection, st_result_status, dev);

    return rslt;
}

/*!
 * @brief This API writes the config array and config version in cfg res.
 */
int8_t bmi323_configure_enhanced_flexibility(struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_configure_enhanced_flexibility(dev);

    return rslt;
}

/*!
 * @brief This API is used to get the config version.
 */
int8_t bmi323_get_config_version(struct bmi3_config_version *config_version, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_config_version(config_version, dev);

    return rslt;
}

/*!
 * @brief This API is used to perform the self-calibration for either sensitivity or offset or both.
 */
int8_t bmi323_perform_gyro_sc(uint8_t sc_selection,
			      uint8_t apply_corr,
			      struct bmi3_self_calib_rslt *sc_rslt,
			      struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_perform_gyro_sc(sc_selection, apply_corr, sc_rslt, dev);

    return rslt;
}

/*!
 * @brief This API is used to set the data sample rate for i3c sync
 */
int8_t bmi323_set_i3c_tc_sync_tph(uint16_t sample_rate, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_i3c_tc_sync_tph(sample_rate, dev);

    return rslt;
}

/*!
 * @brief This API is used to get the data sample rate for i3c sync
 */
int8_t bmi323_get_i3c_tc_sync_tph(uint16_t *sample_rate, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_i3c_tc_sync_tph(sample_rate, dev);

    return rslt;
}

/*!
 * @brief This API is used set the TU(time unit) value is used to scale the delay time payload
 * according to the hosts needs
 */
int8_t bmi323_set_i3c_tc_sync_tu(uint8_t delay_time, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_i3c_tc_sync_tu(delay_time, dev);

    return rslt;
}

/*!
 * @brief This API is used get the TU(time unit) value is used to scale the delay time payload
 * according to the hosts needs
 */
int8_t bmi323_get_i3c_tc_sync_tu(uint8_t *delay_time, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_i3c_tc_sync_tu(delay_time, dev);

    return rslt;
}

/*!
 * @brief This API is used to set the i3c sync ODR.
 */
int8_t bmi323_set_i3c_tc_sync_odr(uint8_t odr, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_i3c_tc_sync_odr(odr, dev);

    return rslt;
}

/*!
 * @brief This API is used to get the i3c sync ODR.
 */
int8_t bmi323_get_i3c_tc_sync_odr(uint8_t *odr, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_i3c_tc_sync_odr(odr, dev);

    return rslt;
}

/*!
 * @brief This internal API gets i3c sync i3c_tc_res
 */
int8_t bmi323_get_i3c_sync_i3c_tc_res(uint8_t *i3c_tc_res, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_i3c_sync_i3c_tc_res(i3c_tc_res, dev);

    return rslt;
}

/*!
 * @brief This internal API sets i3c sync i3c_tc_res
 */
int8_t bmi323_set_i3c_sync_i3c_tc_res(uint8_t i3c_tc_res, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_i3c_sync_i3c_tc_res(i3c_tc_res, dev);

    return rslt;
}

/*!
 * @brief This API is used to enable accel and gyro for alternate configuration
 */
int8_t bmi323_alternate_config_ctrl(uint8_t config_en, uint8_t alt_rst_conf, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_alternate_config_ctrl(config_en, alt_rst_conf, dev);

    return rslt;
}

/*!
 * @brief This API is used to read the status of alternate configuration
 */
int8_t bmi323_read_alternate_status(struct bmi3_alt_status *alt_status, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_read_alternate_status(alt_status, dev);

    return rslt;
}

/*
 * @brief This API gets offset dgain for the sensor which stores self-calibrated values for accel.
 */
int8_t bmi323_get_acc_dp_off_dgain(struct bmi3_acc_dp_gain_offset *acc_dp_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_acc_dp_off_dgain(acc_dp_gain_offset, dev);

    return rslt;
}

/*
 * @brief This API gets offset dgain for the sensor which stores self-calibrated values for gyro.
 */
int8_t bmi323_get_gyro_dp_off_dgain(struct bmi3_gyr_dp_gain_offset *gyr_dp_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_gyro_dp_off_dgain(gyr_dp_gain_offset, dev);

    return rslt;
}

/*
 * @brief This API sets offset dgain for the sensor which stores self-calibrated values for accel.
 */
int8_t bmi323_set_acc_dp_off_dgain(const struct bmi3_acc_dp_gain_offset *acc_dp_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_acc_dp_off_dgain(acc_dp_gain_offset, dev);

    return rslt;
}

/*
 * @brief This API sets offset dgain for the sensor which stores self-calibrated values for gyro.
 */
int8_t bmi323_set_gyro_dp_off_dgain(const struct bmi3_gyr_dp_gain_offset *gyr_dp_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_gyro_dp_off_dgain(gyr_dp_gain_offset, dev);

    return rslt;
}

/*
 * @brief This API gets user offset dgain for the sensor which stores self-calibrated values for accel.
 */
int8_t bmi323_get_user_acc_off_dgain(struct bmi3_acc_usr_gain_offset *acc_usr_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_user_acc_off_dgain(acc_usr_gain_offset, dev);

    return rslt;
}

/*
 * @brief This API gets user offset dgain for the sensor which stores self-calibrated values for gyro.
 */
int8_t bmi323_get_user_gyro_off_dgain(struct bmi3_gyr_usr_gain_offset *gyr_usr_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_user_gyro_off_dgain(gyr_usr_gain_offset, dev);

    return rslt;
}

/*
 * @brief This API sets user offset dgain for the sensor which stores self-calibrated values for accel.
 */
int8_t bmi323_set_user_acc_off_dgain(const struct bmi3_acc_usr_gain_offset *acc_usr_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_user_acc_off_dgain(acc_usr_gain_offset, dev);

    return rslt;
}

/*
 * @brief This API sets user offset dgain for the sensor which stores self-calibrated values for gyro.
 */
int8_t bmi323_set_user_gyr_off_dgain(const struct bmi3_gyr_usr_gain_offset *gyr_usr_gain_offset, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_set_user_gyr_off_dgain(gyr_usr_gain_offset, dev);

    return rslt;
}

/*!
 * @brief This API performs Fast Offset Compensation for accelerometer.
 */
int8_t bmi323_perform_accel_foc(const struct bmi3_accel_foc_g_value *accel_g_value, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_perform_accel_foc(accel_g_value, dev);

    return rslt;
}

/*!
 * @brief This API gets the data ready status of power on reset, accelerometer, gyroscope
 * and temperature.
 */
int8_t bmi323_get_sensor_status(uint16_t *status, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    /* Read the status register */
    rslt = bmi3_get_sensor_status(status, dev);

    return rslt;
}

/*!
 * @brief This API gets the I3C IBI status of both feature and data
 * interrupts
 */
int8_t bmi323_get_i3c_ibi_status(uint16_t *int_status, struct bmi3_dev *dev)
{
    /* Variable to define error */
    int8_t rslt;

    rslt = bmi3_get_i3c_ibi_status(int_status, dev);

    return rslt;
}

/*!
 * @brief This API gets accel gyro offset gain reset values.
 */
int8_t bmi323_get_acc_gyr_off_gain_reset(uint8_t *acc_off_gain_reset, uint8_t *gyr_off_gain_reset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    rslt = bmi3_get_acc_gyr_off_gain_reset(acc_off_gain_reset, gyr_off_gain_reset, dev);

    return rslt;
}

/*!
 * @brief This API sets accel gyro offset gain reset values.
 */
int8_t bmi323_set_acc_gyr_off_gain_reset(uint8_t acc_off_gain_reset, uint8_t gyr_off_gain_reset, struct bmi3_dev *dev)
{
    /* Variable to store result of API */
    int8_t rslt;

    rslt = bmi3_set_acc_gyr_off_gain_reset(acc_off_gain_reset, gyr_off_gain_reset, dev);

    return rslt;
}

/***************************************************************************/

/*!                   Local Function Definitions
 ****************************************************************************/

/*!
 * @brief This internal API is used to validate the device structure pointer for
 * null conditions.
 */
static int8_t null_ptr_check(const struct bmi3_dev *dev)
{
    int8_t rslt;

    if ((dev == NULL) || (dev->read == NULL) || (dev->write == NULL) || (dev->delay_us == NULL))
    {
	rslt = BMI323_E_NULL_PTR;
    } else
    {
	/* Device structure is fine */
	rslt = BMI323_OK;
    }

    return rslt;
}
/*lint -e578 -e132 */
EXPORT_SYMBOL(bmi323_get_sensor_data);
EXPORT_SYMBOL(bmi323_get_regs);
EXPORT_SYMBOL(bmi323_soft_reset);
EXPORT_SYMBOL(bmi323_init);
EXPORT_SYMBOL(bmi323_get_temperature_data);
EXPORT_SYMBOL(bmi323_get_remap_axes);
EXPORT_SYMBOL(bmi323_set_remap_axes);
EXPORT_SYMBOL(bmi323_get_sensor_time);
EXPORT_SYMBOL(bmi323_get_sensor_config);
EXPORT_SYMBOL(bmi323_set_sensor_config);
/*lint +e578 +e132 */

/*lint -e533 -e2 -e10 -e102 -e14 */
MODULE_LICENSE("Dual BSD/GPL");
/*lint +e533 +e2 +e10 +e102 +e14 */

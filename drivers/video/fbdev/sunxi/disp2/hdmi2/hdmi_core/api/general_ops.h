/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef GENERAL_OPS_H_
#define GENERAL_OPS_H_

#include "../../config.h"

#if defined(__LINUX_PLAT__)

#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pwm.h>
#include <asm/div64.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_iommu.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/compat.h>
#else
#include "../../hdmi_boot.h"

#endif
/**
 * @file
 * Define error codes and set error global variable
 */

typedef enum {
	NO_ERROR = 0,	/* No error has occurred, or  buffer has been emptied */
	ERR_NOT_IMPLEMENTED,	/*  */

	ERR_INVALID_PARAM_VENDOR_NAME,
	ERR_INVALID_PARAM_PRODUCT_NAME,
	ERR_3D_STRUCT_NOT_SUPPORTED,
	ERR_FORCING_VSD_3D_IGNORED,
	ERR_PRODUCT_INFO_NOT_PROVIDED,

	ERR_MEM_ALLOCATION,
	ERR_HDCP_MEM_ACCESS,
	ERR_HDCP_FAIL,
	ERR_SRM_INVALID,

	ERR_PHY_TEST_CONTROL,
	ERR_PHY_NOT_LOCKED,

	ERR_PIXEL_CLOCK_NOT_SUPPORTED,
	ERR_PIXEL_REPETITION_NOT_SUPPORTED,
	ERR_COLOR_DEPTH_NOT_SUPPORTED,

	ERR_DVI_MODE_WITH_PIXEL_REPETITION,
	ERR_CHROMA_DECIMATION_FILTER_INVALID,
	ERR_CHROMA_INTERPOLATION_FILTER_INVALID,
	ERR_COLOR_REMAP_SIZE_INVALID,
	ERR_INPUT_ENCODING_TYPE_INVALID,
	ERR_OUTPUT_ENCODING_TYPE_INVALID,
	ERR_AUDIO_CLOCK_NOT_SUPPORTED,
	ERR_SFR_CLOCK_NOT_SUPPORTED,
	ERR_INVALID_I2C_ADDRESS,
	ERR_BLOCK_CHECKSUM_INVALID,
	ERR_HPD_LOST,
	ERR_DTD_BUFFER_FULL,
	ERR_SHORT_AUDIO_DESC_BUFFER_FULL,
	ERR_SHORT_VIDEO_DESC_BUFFER_FULL,
	ERR_DTD_INVALID_CODE,
	ERR_PIXEL_REPETITION_FOR_VIDEO_MODE,
	ERR_OVERFLOW,

	ERR_SINK_DOES_NOT_SUPPORT_AUDIO,
	ERR_SINK_DOES_NOT_SUPPORT_AUDIO_SAMPLING_FREQ,
	ERR_SINK_DOES_NOT_SUPPORT_AUDIO_SAMPLE_SIZE,
	ERR_SINK_DOES_NOT_SUPPORT_ATTRIBUTED_CHANNELS,
	ERR_SINK_DOES_NOT_SUPPORT_AUDIO_TYPE,
	ERR_SINK_DOES_NOT_SUPPORT_30BIT_DC,
	ERR_SINK_DOES_NOT_SUPPORT_36BIT_DC,
	ERR_SINK_DOES_NOT_SUPPORT_48BIT_DC,
	ERR_SINK_DOES_NOT_SUPPORT_YCC444_DC,
	ERR_SINK_DOES_NOT_SUPPORT_TMDS_CLOCK,
	ERR_SINK_DOES_NOT_SUPPORT_HDMI,
	ERR_SINK_DOES_NOT_SUPPORT_YCC444_ENCODING,
	ERR_SINK_DOES_NOT_SUPPORT_YCC422_ENCODING,
	ERR_SINK_DOES_NOT_SUPPORT_XVYCC601_COLORIMETRY,
	ERR_SINK_DOES_NOT_SUPPORT_XVYCC709_COLORIMETRY,
	ERR_SINK_DOES_NOT_SUPPORT_SYCC601_COLORIMETRY,
	ERR_SINK_DOES_NOT_SUPPORT_ADOBEYCC601_COLORIMETRY,
	ERR_SINK_DOES_NOT_SUPPORT_ADOBERGB_COLORIMETRY,
	ERR_EXTENDED_COLORIMETRY_PARAMS_INVALID,
	ERR_SINK_DOES_NOT_SUPPORT_EXTENDED_COLORIMETRY,
	ERR_DVI_MODE_WITH_YCC_ENCODING,
	ERR_DVI_MODE_WITH_EXTENDED_COLORIMETRY,
	ERR_SINK_DOES_NOT_SUPPORT_SELECTED_DTD,

	ERR_DATA_WIDTH_INVALID,
	ERR_DATA_GREATER_THAN_WIDTH,
	ERR_CANNOT_CREATE_MUTEX,
	ERR_CANNOT_DESTROY_MUTEX,
	ERR_CANNOT_LOCK_MUTEX,
	ERR_CANNOT_UNLOCK_MUTEX,
	ERR_INTERRUPT_NOT_MAPPED,
	ERR_HW_NOT_SUPPORTED,
	ERR_UNDEFINED_HTX
} errorType_t;


/**
 * Concatenate two parts of two 8-bit bytes into a new 16-bit word
 * @param bHi first byte
 * @param oHi shift part of first byte (to be place as most significant
 * bits)
 * @param nHi width part of first byte (to be place as most significant
 * bits)
 * @param bLo second byte
 * @param oLo shift part of second byte (to be place as least
 * significant bits)
 * @param nLo width part of second byte (to be place as least
 * significant bits)
 * @returns 16-bit concatenated word as part of the first byte and part of
 * the second byte
 */
u16 concat_bits(u8 bHi, u8 oHi, u8 nHi, u8 bLo, u8 oLo, u8 nLo);

/** Concatenate two full bytes into a new 16-bit word
 * @param hi
 * @param lo
 * @returns hi as most significant bytes concatenated with lo as least
 * significant bits.
 */
u16 byte_to_word(const u8 hi, const u8 lo);

/** Extract the content of a certain part of a byte
 * @param data 8bit byte
 * @param shift shift from the start of the bit (0)
 * @param width width of the desired part starting from shift
 * @returns an 8bit byte holding only the desired part of the passed on
 * data byte
 */
u8 bit_field(const u16 data, u8 shift, u8 width);

/** Concatenate four 8-bit bytes into a new 32-bit word
 * @param b3 assumed as most significant 8-bit byte
 * @param b2
 * @param b1
 * @param b0 assumed as least significant 8bit byte
 * @returns a 2D word, 32bits, composed of the 4 passed on parameters
 */
u32 byte_to_dword(u8 b3, u8 b2, u8 b1, u8 b0);

/**
 * Set the global error variable to the error code
 * @param err the error code
 */
void error_set(errorType_t err);

/**
 * Pop the last error set
 * @return the error code of the last occurred error
 * @note calling this function will clear the error code and set it to NO_ERROR
 */
errorType_t error_Get(void);

#endif /* GENERAL_OPS_H_ */

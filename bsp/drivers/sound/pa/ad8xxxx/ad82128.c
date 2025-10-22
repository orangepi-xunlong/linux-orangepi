/*
 * ad82128.c  --  ad82128 ALSA SoC Audio driver
 *
 * Copyright 1998 Elite Semiconductor Memory Technology
 *
 * Author: ESMT Audio/Power Product BU Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/device.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <linux/regmap.h>

#include "ad82128.h"
#include "../platform/snd_sunxi_common.h"

/* after enable,you can use "tinymix" or "amixer" cmd to change eq mode. */
/* #define	AD82128_CHANGE_EQ_MODE_EN */

#ifdef AD82128_CHANGE_EQ_MODE_EN
	/* eq file, add Parameter file in "codec\ad82128_eq", and then include the header file. */
	#include "ad82128_eq/ad82128_eq_mode_1.h"
	#include "ad82128_eq/ad82128_eq_mode_2.h"

	/* the value range of eq mode, modify them according to your request. */
	#define AD82128_EQ_MODE_MIN 1
	#define AD82128_EQ_MODE_MAX 2
#endif

/* Define how often to check (and clear) the fault status register (in ms) */
#define AD82128_FAULT_CHECK_INTERVAL		500

/* you can read out the register and ram data, and check them. */
#define AD82128_REG_RAM_CHECK

enum ad82128_type {
	AD82128,
};

static const char * const ad82128_supply_names[] = {
	"dvdd",		/* Digital power supply. Connect to 3.3-V supply. */
	"pvdd",		/* Class-D amp and analog power supply (connected). */
};

#define AD82128_NUM_SUPPLIES	ARRAY_SIZE(ad82128_supply_names)

static int m_reg_tab[AD82128_REGISTER_COUNT][2] = {
                     {0x00, 0x00},//##State_Control_1
                     {0x01, 0x81},//##State_Control_2
                     {0x02, 0xff},//##State_Control_3
                     {0x03, 0x0e},//##Master_volume_control
                     {0x04, 0x18},//##Channel_1_volume_control
                     {0x05, 0x18},//##Channel_2_volume_control
                     {0x06, 0x18},//##Channel_3_volume_control
                     {0x07, 0x18},//##Channel_4_volume_control
                     {0x08, 0x18},//##Channel_5_volume_control
                     {0x09, 0x18},//##Channel_6_volume_control
                     {0x0a, 0x00},//##Reserve
                     {0x0b, 0x00},//##Reserve
                     {0x0c, 0x98},//##State_Control_4
                     {0x0d, 0x80},//##Channel_1_configuration_registers
                     {0x0e, 0x80},//##Channel_2_configuration_registers
                     {0x0f, 0x80},//##Channel_3_configuration_registers
                     {0x10, 0x80},//##Channel_4_configuration_registers
                     {0x11, 0x80},//##Channel_5_configuration_registers
                     {0x12, 0x80},//##Channel_6_configuration_registers
                     {0x13, 0x80},//##Channel_7_configuration_registers
                     {0x14, 0x80},//##Channel_8_configuration_registers
                     {0x15, 0x6a},//##Reserve
                     {0x16, 0x6a},//##Reserve
                     {0x17, 0x6a},//##Reserve
                     {0x18, 0x6a},//##Reserve
                     {0x19, 0x00},//##Reserve
                     {0x1a, 0x28},//##State_Control_5
                     {0x1b, 0x80},//##State_Control_6
                     {0x1c, 0x20},//##State_Control_7
                     {0x1d, 0x7f},//##Coefficient_RAM_Base_Address
                     {0x1e, 0xf0},//##First_4-bits_of_coefficients_A1
                     {0x1f, 0x00},//##Second_8-bits_of_coefficients_A1
                     {0x20, 0x00},//##Third_8-bits_of_coefficients_A1
                     {0x21, 0x00},//##Fourth-bits_of_coefficients_A1
                     {0x22, 0x01},//##First_4-bits_of_coefficients_A2
                     {0x23, 0x88},//##Second_8-bits_of_coefficients_A2
                     {0x24, 0x66},//##Third_8-bits_of_coefficients_A2
                     {0x25, 0x59},//##Fourth_8-bits_of_coefficients_A2
                     {0x26, 0x03},//##First_4-bits_of_coefficients_B1
                     {0x27, 0x76},//##Second_8-bits_of_coefficients_B1
                     {0x28, 0xd3},//##Third_8-bits_of_coefficients_B1
                     {0x29, 0x34},//##Fourth_8-bits_of_coefficients_B1
                     {0x2a, 0xfe},//##First_4-bits_of_coefficients_B2
                     {0x2b, 0x69},//##Second_8-bits_of_coefficients_B2
                     {0x2c, 0xe4},//##Third_8-bits_of_coefficients_B2
                     {0x2d, 0x28},//##Fourth-bits_of_coefficients_B2
                     {0x2e, 0x02},//##First_4-bits_of_coefficients_A0
                     {0x2f, 0x0d},//##Second_8-bits_of_coefficients_A0
                     {0x30, 0xb5},//##Third_8-bits_of_coefficients_A0
                     {0x31, 0x7e},//##Fourth_8-bits_of_coefficients_A0
                     {0x32, 0x40},//##Coefficient_RAM_R/W_control
                     {0x33, 0x06},//##State_Control_8
                     {0x34, 0xf0},//##State_Control_9
                     {0x35, 0x00},//##Volume_Fine_tune
                     {0x36, 0x00},//##Volume_Fine_tune
                     {0x37, 0x00},//##Device_ID_register
                     {0x38, 0x00},//##Level_Meter_Clear
                     {0x39, 0x00},//##Power_Meter_Clear
                     {0x3a, 0x00},//##First_8bits_of_C1_Level_Meter
                     {0x3b, 0xb7},//##Second_8bits_of_C1_Level_Meter
                     {0x3c, 0x1c},//##Third_8bits_of_C1_Level_Meter
                     {0x3d, 0x05},//##Fourth_8bits_of_C1_Level_Meter
                     {0x3e, 0x00},//##First_8bits_of_C2_Level_Meter
                     {0x3f, 0xbd},//##Second_8bits_of_C2_Level_Meter
                     {0x40, 0x3a},//##Third_8bits_of_C2_Level_Meter
                     {0x41, 0x19},//##Fourth_8bits_of_C2_Level_Meter
                     {0x42, 0x00},//##First_8bits_of_C3_Level_Meter
                     {0x43, 0x00},//##Second_8bits_of_C3_Level_Meter
                     {0x44, 0x00},//##Third_8bits_of_C3_Level_Meter
                     {0x45, 0x00},//##Fourth_8bits_of_C3_Level_Meter
                     {0x46, 0x00},//##First_8bits_of_C4_Level_Meter
                     {0x47, 0x00},//##Second_8bits_of_C4_Level_Meter
                     {0x48, 0x00},//##Third_8bits_of_C4_Level_Meter
                     {0x49, 0x00},//##Fourth_8bits_of_C4_Level_Meter
                     {0x4a, 0x00},//##First_8bits_of_C5_Level_Meter
                     {0x4b, 0x00},//##Second_8bits_of_C5_Level_Meter
                     {0x4c, 0x00},//##Third_8bits_of_C5_Level_Meter
                     {0x4d, 0x00},//##Fourth_8bits_of_C5_Level_Meter
                     {0x4e, 0x00},//##First_8bits_of_C6_Level_Meter
                     {0x4f, 0x00},//##Second_8bits_of_C6_Level_Meter
                     {0x50, 0x00},//##Third_8bits_of_C6_Level_Meter
                     {0x51, 0x00},//##Fourth_8bits_of_C6_Level_Meter
                     {0x52, 0x00},//##First_8bits_of_C7_Level_Meter
                     {0x53, 0x00},//##Second_8bits_of_C7_Level_Meter
                     {0x54, 0x00},//##Third_8bits_of_C7_Level_Meter
                     {0x55, 0x00},//##Fourth_8bits_of_C7_Level_Meter
                     {0x56, 0x00},//##First_8bits_of_C8_Level_Meter
                     {0x57, 0x00},//##Second_8bits_of_C8_Level_Meter
                     {0x58, 0x00},//##Third_8bits_of_C8_Level_Meter
                     {0x59, 0x00},//##Fourth_8bits_of_C8_Level_Meter
                     {0x5a, 0x05},//##I2S_data_output_selection_register
                     {0x5b, 0x00},//##Mono_Key_High_Byte
                     {0x5c, 0x00},//##Mono_Key_Low_Byte
                     {0x5d, 0x07},//##Hi-res_Item
                     {0x5e, 0x00},//##Analog_gain
                     {0x5f, 0x00},//##Reserve
                     {0x60, 0x00},//##Reserve
                     {0x61, 0x00},//##Reserve
                     {0x62, 0x00},//##Reserve
                     {0x63, 0x00},//##Reserve
                     {0x64, 0x00},//##Reserve
                     {0x65, 0x00},//##Reserve
                     {0x66, 0x00},//##Reserve
                     {0x67, 0x00},//##Reserve
                     {0x68, 0x00},//##Reserve
                     {0x69, 0x00},//##Reserve
                     {0x6a, 0x00},//##Reserve
                     {0x6b, 0x00},//##Reserve
                     {0x6c, 0x01},//##FS_and_PMF_read_out
                     {0x6d, 0x00},//##OC_level_setting
                     {0x6e, 0x40},//##DTC_setting
                     {0x6f, 0x74},//##Testmode_register0
                     {0x70, 0x07},//##Reserve
                     {0x71, 0x40},//##Testmode_register1
                     {0x72, 0x38},//##Testmode_register2
                     {0x73, 0x18},//##Dither_signal_setting
                     {0x74, 0x06},//##Error_delay
                     {0x75, 0x55},//##First_8bits_of_MBIST_User_Program_Even
                     {0x76, 0x55},//##Second_8bits_of_MBIST_User_Program_Even
                     {0x77, 0x55},//##Third_8bits_of_MBIST_User_Program_Even
                     {0x78, 0x55},//##Fourth_8bits_of_MBIST_User_Program_Even
                     {0x79, 0x55},//##First_8bits_of_MBIST_User_Program_Odd
                     {0x7a, 0x55},//##Second_8bits_of_MBIST_User_Program_Odd
                     {0x7b, 0x55},//##Third_8bits_of_MBIST_User_Program_Odd
                     {0x7c, 0x55},//##Fourth_8bits_of_MBIST_User_Program_Odd
                     {0x7d, 0xfe},//##Error_register
                     {0x7e, 0x7a},//##Error_latch_register
                     {0x7f, 0x00},//##Error_clear_register
                     {0x80, 0x00},//##Protection_register_set
                     {0x81, 0x00},//##Memory_MBIST_status
                     {0x82, 0x00},//##PWM_output_control
                     {0x83, 0x00},//##Testmode_control_register
                     {0x84, 0x00},//##RAM1_test_register_address
                     {0x85, 0x00},//##First_8bits_of_RAM1_data
};

static int m_ram1_tab[][5] = {
                     {0x00, 0x0c, 0x02, 0x26, 0xe5},//##Channel_1_EQ1_A1
                     {0x01, 0x01, 0xfe, 0xec, 0x8d},//##Channel_1_EQ1_A2
                     {0x02, 0x03, 0xfd, 0xd8, 0x8a},//##Channel_1_EQ1_B1
                     {0x03, 0x0e, 0x02, 0x26, 0x54},//##Channel_1_EQ1_B2
                     {0x04, 0x01, 0xfe, 0xec, 0x8d},//##Channel_1_EQ1_A0
                     {0x05, 0x0c, 0x02, 0x26, 0xe5},//##Channel_1_EQ2_A1
                     {0x06, 0x01, 0xfe, 0xec, 0x8d},//##Channel_1_EQ2_A2
                     {0x07, 0x03, 0xfd, 0xd8, 0x8a},//##Channel_1_EQ2_B1
                     {0x08, 0x0e, 0x02, 0x26, 0x54},//##Channel_1_EQ2_B2
                     {0x09, 0x01, 0xfe, 0xec, 0x8d},//##Channel_1_EQ2_A0
                     {0x0a, 0x0c, 0x02, 0xf8, 0x1b},//##Channel_1_EQ3_A1
                     {0x0b, 0x01, 0xfb, 0x92, 0x5b},//##Channel_1_EQ3_A2
                     {0x0c, 0x03, 0xfd, 0x07, 0xe5},//##Channel_1_EQ3_B1
                     {0x0d, 0x0e, 0x02, 0xf4, 0xf5},//##Channel_1_EQ3_B2
                     {0x0e, 0x02, 0x01, 0x78, 0xaf},//##Channel_1_EQ3_A0
                     {0x0f, 0x0c, 0x0b, 0x59, 0xf4},//##Channel_1_EQ4_A1
                     {0x10, 0x01, 0xf5, 0xfd, 0xb9},//##Channel_1_EQ4_A2
                     {0x11, 0x03, 0xf4, 0xa6, 0x0c},//##Channel_1_EQ4_B1
                     {0x12, 0x0e, 0x0b, 0x27, 0xfa},//##Channel_1_EQ4_B2
                     {0x13, 0x01, 0xfe, 0xda, 0x4d},//##Channel_1_EQ4_A0
                     {0x14, 0x0c, 0x11, 0xd0, 0xdc},//##Channel_1_EQ5_A1
                     {0x15, 0x01, 0xf1, 0x41, 0x4d},//##Channel_1_EQ5_A2
                     {0x16, 0x03, 0xee, 0x2f, 0x24},//##Channel_1_EQ5_B1
                     {0x17, 0x0e, 0x10, 0x6f, 0x5e},//##Channel_1_EQ5_B2
                     {0x18, 0x01, 0xfe, 0x4f, 0x54},//##Channel_1_EQ5_A0
                     {0x19, 0x0c, 0x2b, 0x05, 0xf7},//##Channel_1_EQ6_A1
                     {0x1a, 0x01, 0xdf, 0x50, 0x0a},//##Channel_1_EQ6_A2
                     {0x1b, 0x03, 0xd4, 0xfa, 0x09},//##Channel_1_EQ6_B1
                     {0x1c, 0x0e, 0x26, 0x46, 0xdc},//##Channel_1_EQ6_B2
                     {0x1d, 0x01, 0xfa, 0x69, 0x19},//##Channel_1_EQ6_A0
                     {0x1e, 0x0c, 0x89, 0x2c, 0xcc},//##Channel_1_EQ7_A1
                     {0x1f, 0x01, 0x88, 0x66, 0x59},//##Channel_1_EQ7_A2
                     {0x20, 0x03, 0x76, 0xd3, 0x34},//##Channel_1_EQ7_B1
                     {0x21, 0x0e, 0x69, 0xe4, 0x28},//##Channel_1_EQ7_B2
                     {0x22, 0x02, 0x0d, 0xb5, 0x7e},//##Channel_1_EQ7_A0
                     {0x23, 0x0c, 0xf2, 0xb5, 0x68},//##Channel_1_EQ8_A1
                     {0x24, 0x01, 0x97, 0xf3, 0x1e},//##Channel_1_EQ8_A2
                     {0x25, 0x03, 0x0d, 0x4a, 0x98},//##Channel_1_EQ8_B1
                     {0x26, 0x0e, 0x79, 0xd7, 0xb8},//##Channel_1_EQ8_B2
                     {0x27, 0x01, 0xee, 0x35, 0x29},//##Channel_1_EQ8_A0
                     {0x28, 0x00, 0xbe, 0x69, 0x2e},//##Channel_1_EQ9_A1
                     {0x29, 0x00, 0x5f, 0x34, 0x97},//##Channel_1_EQ9_A2
                     {0x2a, 0x00, 0xec, 0x13, 0xea},//##Channel_1_EQ9_B1
                     {0x2b, 0x0f, 0x97, 0x19, 0xb8},//##Channel_1_EQ9_B2
                     {0x2c, 0x00, 0x5f, 0x34, 0x97},//##Channel_1_EQ9_A0
                     {0x2d, 0x0c, 0x1a, 0xb4, 0xee},//##Channel_1_EQ10_A1
                     {0x2e, 0x01, 0xde, 0x7a, 0x39},//##Channel_1_EQ10_A2
                     {0x2f, 0x03, 0xe5, 0x4b, 0x12},//##Channel_1_EQ10_B1
                     {0x30, 0x0e, 0x19, 0xef, 0xf2},//##Channel_1_EQ10_B2
                     {0x31, 0x02, 0x07, 0x95, 0xd4},//##Channel_1_EQ10_A0
                     {0x32, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ11_A1
                     {0x33, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ11_A2
                     {0x34, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ11_B1
                     {0x35, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ11_B2
                     {0x36, 0x02, 0x00, 0x00, 0x00},//##Channel_1_EQ11_A0
                     {0x37, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ12_A1
                     {0x38, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ12_A2
                     {0x39, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ12_B1
                     {0x3a, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ12_B2
                     {0x3b, 0x02, 0x00, 0x00, 0x00},//##Channel_1_EQ12_A0
                     {0x3c, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ13_A1
                     {0x3d, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ13_A2
                     {0x3e, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ13_B1
                     {0x3f, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ13_B2
                     {0x40, 0x02, 0x00, 0x00, 0x00},//##Channel_1_EQ13_A0
                     {0x41, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ14_A1
                     {0x42, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ14_A2
                     {0x43, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ14_B1
                     {0x44, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ14_B2
                     {0x45, 0x02, 0x00, 0x00, 0x00},//##Channel_1_EQ14_A0
                     {0x46, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ15_A1
                     {0x47, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ15_A2
                     {0x48, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ15_B1
                     {0x49, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ15_B2
                     {0x4a, 0x02, 0x00, 0x00, 0x00},//##Channel_1_EQ15_A0
                     {0x4b, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ16_A1
                     {0x4c, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ16_A2
                     {0x4d, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ16_B1
                     {0x4e, 0x00, 0x00, 0x00, 0x00},//##Channel_1_EQ16_B2
                     {0x4f, 0x02, 0x00, 0x00, 0x00},//##Channel_1_EQ16_A0
                     {0x50, 0x07, 0xff, 0xff, 0xf0},//##Channel_1_Mixer1
                     {0x51, 0x00, 0x00, 0x00, 0x00},//##Channel_1_Mixer2
                     {0x52, 0x00, 0x7e, 0x88, 0xe0},//##Channel_1_Prescale
                     {0x53, 0x02, 0x00, 0x00, 0x00},//##Channel_1_Postscale
                     {0x54, 0x02, 0x00, 0x00, 0x00},//##CH1.2_Power_Clipping
                     {0x55, 0x00, 0x00, 0x01, 0xa0},//##Noise_Gate_Attack_Level
                     {0x56, 0x00, 0x00, 0x05, 0x30},//##Noise_Gate_Release_Level
                     {0x57, 0x00, 0x01, 0x00, 0x00},//##DRC1_Energy_Coefficient
                     {0x58, 0x00, 0x01, 0x00, 0x00},//##DRC2_Energy_Coefficient
                     {0x59, 0x00, 0x01, 0x00, 0x00},//##DRC3_Energy_Coefficient
                     {0x5a, 0x00, 0x01, 0x00, 0x00},//##DRC4_Energy_Coefficient
                     {0x5b, 0x00, 0x17, 0xc5, 0xe5},//##DRC1_Power_Meter
                     {0x5c, 0x00, 0x00, 0x00, 0x00},//##DRC3_Power_Meter
                     {0x5d, 0x00, 0x00, 0x00, 0x00},//##DRC5_Power_Meter
                     {0x5e, 0x00, 0x00, 0x00, 0x00},//##DRC7_Power_Meter
                     {0x5f, 0x02, 0x00, 0x00, 0x00},//##Channel_1_DRC_GAIN1
                     {0x60, 0x02, 0x00, 0x00, 0x00},//##Channel_1_DRC_GAIN2
                     {0x61, 0x0e, 0x00, 0x00, 0x00},//##Channel_1_DRC_GAIN3
                     {0x62, 0x0c, 0x9a, 0xfb, 0xcf},//##DRC1_FF_threshold
                     {0x63, 0x02, 0x00, 0x00, 0x00},//##DRC1_FF_slope
                     {0x64, 0x00, 0x00, 0x2d, 0x80},//##DRC1_FF_aa
                     {0x65, 0x00, 0x00, 0x0d, 0xa7},//##DRC1_FF_da
                     {0x66, 0x0e, 0x01, 0xc0, 0x70},//##DRC2_FF_threshold
                     {0x67, 0x02, 0x00, 0x00, 0x00},//##DRC2_FF_slope
                     {0x68, 0x00, 0x00, 0x40, 0x00},//##DRC2_FF_aa
                     {0x69, 0x00, 0x00, 0x40, 0x00},//##DRC2_FF_da
                     {0x6a, 0x0e, 0x01, 0xc0, 0x70},//##DRC3_FF_threshold
                     {0x6b, 0x02, 0x00, 0x00, 0x00},//##DRC3_FF_slope
                     {0x6c, 0x00, 0x00, 0x40, 0x00},//##DRC3_FF_aa
                     {0x6d, 0x00, 0x00, 0x40, 0x00},//##DRC3_FF_da
                     {0x6e, 0x0e, 0x01, 0xc0, 0x70},//##DRC4_FF_threshold
                     {0x6f, 0x02, 0x00, 0x00, 0x00},//##DRC4_FF_slope
                     {0x70, 0x00, 0x00, 0x40, 0x00},//##DRC4_FF_aa
                     {0x71, 0x00, 0x00, 0x40, 0x00},//##DRC4_FF_da
                     {0x72, 0x00, 0x75, 0xff, 0x98},//##DRC1_gain
                     {0x73, 0x00, 0x00, 0x00, 0x00},//##DRC3_gain
                     {0x74, 0x00, 0x00, 0x00, 0x00},//##DRC5_gain
                     {0x75, 0x00, 0x00, 0x00, 0x00},//##DRC7_gain
                     {0x76, 0x00, 0x80, 0x00, 0x00},//##I2SO_LCH_gain
                     {0x77, 0x02, 0x00, 0x00, 0x00},//##SRS_gain
};

static int m_ram2_tab[][5]= {
                     {0x00, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ1_A1
                     {0x01, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ1_A2
                     {0x02, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ1_B1
                     {0x03, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ1_B2
                     {0x04, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ1_A0
                     {0x05, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ2_A1
                     {0x06, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ2_A2
                     {0x07, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ2_B1
                     {0x08, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ2_B2
                     {0x09, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ2_A0
                     {0x0a, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ3_A1
                     {0x0b, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ3_A2
                     {0x0c, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ3_B1
                     {0x0d, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ3_B2
                     {0x0e, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ3_A0
                     {0x0f, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ4_A1
                     {0x10, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ4_A2
                     {0x11, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ4_B1
                     {0x12, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ4_B2
                     {0x13, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ4_A0
                     {0x14, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ5_A1
                     {0x15, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ5_A2
                     {0x16, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ5_B1
                     {0x17, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ5_B2
                     {0x18, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ5_A0
                     {0x19, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ6_A1
                     {0x1a, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ6_A2
                     {0x1b, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ6_B1
                     {0x1c, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ6_B2
                     {0x1d, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ6_A0
                     {0x1e, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ7_A1
                     {0x1f, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ7_A2
                     {0x20, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ7_B1
                     {0x21, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ7_B2
                     {0x22, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ7_A0
                     {0x23, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ8_A1
                     {0x24, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ8_A2
                     {0x25, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ8_B1
                     {0x26, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ8_B2
                     {0x27, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ8_A0
                     {0x28, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ9_A1
                     {0x29, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ9_A2
                     {0x2a, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ9_B1
                     {0x2b, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ9_B2
                     {0x2c, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ9_A0
                     {0x2d, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ10_A1
                     {0x2e, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ10_A2
                     {0x2f, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ10_B1
                     {0x30, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ10_B2
                     {0x31, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ10_A0
                     {0x32, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ11_A1
                     {0x33, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ11_A2
                     {0x34, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ11_B1
                     {0x35, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ11_B2
                     {0x36, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ11_A0
                     {0x37, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ12_A1
                     {0x38, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ12_A2
                     {0x39, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ12_B1
                     {0x3a, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ12_B2
                     {0x3b, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ12_A0
                     {0x3c, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ13_A1
                     {0x3d, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ13_A2
                     {0x3e, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ13_B1
                     {0x3f, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ13_B2
                     {0x40, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ13_A0
                     {0x41, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ14_A1
                     {0x42, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ14_A2
                     {0x43, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ14_B1
                     {0x44, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ14_B2
                     {0x45, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ14_A0
                     {0x46, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ15_A1
                     {0x47, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ15_A2
                     {0x48, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ15_B1
                     {0x49, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ15_B2
                     {0x4a, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ15_A0
                     {0x4b, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ16_A1
                     {0x4c, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ16_A2
                     {0x4d, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ16_B1
                     {0x4e, 0x00, 0x00, 0x00, 0x00},//##Channel_2_EQ16_B2
                     {0x4f, 0x02, 0x00, 0x00, 0x00},//##Channel_2_EQ16_A0
                     {0x50, 0x00, 0x00, 0x00, 0x00},//##Channel_2_Mixer1
                     {0x51, 0x07, 0xff, 0xff, 0xf0},//##Channel_2_Mixer2
                     {0x52, 0x00, 0x7e, 0x88, 0xe0},//##Channel_2_Prescale
                     {0x53, 0x02, 0x00, 0x00, 0x00},//##Channel_2_Postscale
                     {0x54, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x55, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x56, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x57, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x58, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x59, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x5a, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x5b, 0x00, 0x05, 0x64, 0xd1},//##DRC2_Power_Meter
                     {0x5c, 0x00, 0x00, 0x00, 0x00},//##DRC4_Power_Mete
                     {0x5d, 0x00, 0x00, 0x00, 0x00},//##DRC6_Power_Meter
                     {0x5e, 0x00, 0x00, 0x00, 0x00},//##DRC8_Power_Meter
                     {0x5f, 0x02, 0x00, 0x00, 0x00},//##Channel_2_DRC_GAIN1
                     {0x60, 0x02, 0x00, 0x00, 0x00},//##Channel_2_DRC_GAIN2
                     {0x61, 0x0e, 0x00, 0x00, 0x00},//##Channel_2_DRC_GAIN3
                     {0x62, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x63, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x64, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x65, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x66, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x67, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x68, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x69, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x6a, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x6b, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x6c, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x6d, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x6e, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x6f, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x70, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x71, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     {0x72, 0x00, 0x7f, 0xda, 0x80},//##DRC2_gain
                     {0x73, 0x00, 0x00, 0x00, 0x00},//##DRC4_gain
                     {0x74, 0x00, 0x00, 0x00, 0x00},//##DRC6_gain
                     {0x75, 0x00, 0x00, 0x00, 0x00},//##DRC8_gain
                     {0x76, 0x00, 0x80, 0x00, 0x00},//##I2SO_RCH_gain
                     {0x77, 0x00, 0x00, 0x00, 0x00},//##Reserve
                     };


struct ad82128_data {
	int pd_gpio;
	struct snd_soc_component *component;
	struct regmap *regmap;
	struct i2c_client *ad82128_client;
	/* enum ad82128_type devtype; */
	struct regulator_bulk_data supplies[AD82128_NUM_SUPPLIES];
	struct delayed_work fault_check_work;
	unsigned int last_fault;
	struct snd_sunxi_rglt *rglt;
#ifdef AD82128_CHANGE_EQ_MODE_EN
	unsigned int eq_mode;
	unsigned char (*m_ram_tab)[5];
#endif

};
struct ad82128_data *ad82128_data_new = NULL;

static int ad82128_pd_gpio_set(struct snd_soc_component *component, bool enable)
{
	struct ad82128_data *ad82128 =
		snd_soc_component_get_drvdata(component);

	if (!(gpio_is_valid(ad82128->pd_gpio)))
	{
	    dev_err(component->dev, "ad82128 the pd gpio is un-valid\n");
	    return 0;
	}

	if (enable) {
		gpio_set_value(ad82128->pd_gpio, GPIOF_OUT_INIT_HIGH);
		dev_info(component->dev, "enable: ad82128 pd pin status = %d\n",
			gpio_get_value(ad82128->pd_gpio));
	} else {
		gpio_set_value(ad82128->pd_gpio, GPIOF_OUT_INIT_LOW);
		dev_info(component->dev, "disable: ad82128 pd pin status = %d\n",
			gpio_get_value(ad82128->pd_gpio));
	}

	return 0;
}

static int ad82128_hw_params(struct snd_pcm_substream *substream,
			     struct snd_pcm_hw_params *params,
			     struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;
	unsigned int rate = params_rate(params);
	bool ssz_ds;
	int ret;

	switch (rate) {
	case 44100:
	case 48000:
		ssz_ds = false;
		break;
	case 88200:
	case 96000:
		ssz_ds = true;
		break;
	default:
		dev_err(component->dev, "unsupported sample rate: %u\n", rate);
		return -EINVAL;
	}

	ad82128_pd_gpio_set(component, true);

	msleep(100);

	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL2_REG,
				  AD82128_SSZ_DS, ssz_ds);
	if (ret < 0) {
		dev_err(component->dev, "error setting sample rate: %d\n", ret);
		return ret;
	}

	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL3_REG,
				  AD82128_MUTE, false);
	if (ret < 0) {
		dev_err(component->dev, "error setting mute ctl: %d\n", ret);
		return ret;
	}

	return 0;
}

static int ad82128_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct snd_soc_component *component = dai->component;
	u8 serial_format;
	int ret;

	if ((fmt & SND_SOC_DAIFMT_MASTER_MASK) != SND_SOC_DAIFMT_CBS_CFS) {
		dev_vdbg(component->dev, "DAI Format master is not found\n");
		return -EINVAL;
	}

	switch (fmt & (SND_SOC_DAIFMT_FORMAT_MASK |
		       SND_SOC_DAIFMT_INV_MASK)) {
	case (SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF):
		/* 1st data bit occur one BCLK cycle after the frame sync */
		serial_format = AD82128_SAIF_I2S;
		break;
	case (SND_SOC_DAIFMT_DSP_A | SND_SOC_DAIFMT_NB_NF):
		/*
		 * Note that although the AD82128 does not have a dedicated DSP
		 * mode it doesn't care about the LRCLK duty cycle during TDM
		 * operation. Therefore we can use the device's I2S mode with
		 * its delaying of the 1st data bit to receive DSP_A formatted
		 * data. See device datasheet for additional details.
		 */
		serial_format = AD82128_SAIF_I2S;
		break;
	case (SND_SOC_DAIFMT_DSP_B | SND_SOC_DAIFMT_NB_NF):
		/*
		 * Similar to DSP_A, we can use the fact that the AD82128 does
		 * not care about the LRCLK duty cycle during TDM to receive
		 * DSP_B formatted data in LEFTJ mode (no delaying of the 1st
		 * data bit).
		 */
		serial_format = AD82128_SAIF_LEFTJ;
		break;
	case (SND_SOC_DAIFMT_LEFT_J | SND_SOC_DAIFMT_NB_NF):
		/* No delay after the frame sync */
		serial_format = AD82128_SAIF_LEFTJ;
		break;
	default:
		dev_vdbg(component->dev, "DAI Format is not found\n");
		return -EINVAL;
	}

	ad82128_pd_gpio_set(component, true);	/* pull high amp PD pin */
	msleep(20);

	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL1_REG,
				  AD82128_SAIF_FORMAT_MASK,
				  serial_format);
	if (ret < 0) {
		dev_err(component->dev, "error setting SAIF format: %d\n", ret);
		return ret;
	}

	return 0;
}

#ifdef AD82128_CHANGE_EQ_MODE_EN
static int ad82128_change_eq_mode(struct snd_soc_component *component, int channel)
{
	struct ad82128_data *ad82128 = snd_soc_component_get_drvdata(component);
	int eq_seg = 0;
	int i = 0;
	int cmd = 0;

	for (i = 0; i < 16; i++) {
		/* ram addr */
		regmap_write(ad82128->regmap, 0x1d, ad82128->m_ram_tab[eq_seg][0]);
		
		/* write A1 */
		regmap_write(ad82128->regmap, 0x1e, ad82128->m_ram_tab[eq_seg][1]);
		regmap_write(ad82128->regmap, 0x1f, ad82128->m_ram_tab[eq_seg][2]);
		regmap_write(ad82128->regmap, 0x20, ad82128->m_ram_tab[eq_seg][3]);
		regmap_write(ad82128->regmap, 0x21, ad82128->m_ram_tab[eq_seg][4]);

		eq_seg += 1;
		/* write A2 */
		regmap_write(ad82128->regmap, 0x22, ad82128->m_ram_tab[eq_seg][1]);
		regmap_write(ad82128->regmap, 0x23, ad82128->m_ram_tab[eq_seg][2]);
		regmap_write(ad82128->regmap, 0x24, ad82128->m_ram_tab[eq_seg][3]);
		regmap_write(ad82128->regmap, 0x25, ad82128->m_ram_tab[eq_seg][4]);

		eq_seg += 1;
		/* write B1 */
		regmap_write(ad82128->regmap, 0x26, ad82128->m_ram_tab[eq_seg][1]);
		regmap_write(ad82128->regmap, 0x27, ad82128->m_ram_tab[eq_seg][2]);
		regmap_write(ad82128->regmap, 0x28, ad82128->m_ram_tab[eq_seg][3]);
		regmap_write(ad82128->regmap, 0x29, ad82128->m_ram_tab[eq_seg][4]);

		eq_seg += 1;
		/* write B2 */
		regmap_write(ad82128->regmap, 0x2a, ad82128->m_ram_tab[eq_seg][1]);
		regmap_write(ad82128->regmap, 0x2b, ad82128->m_ram_tab[eq_seg][2]);
		regmap_write(ad82128->regmap, 0x2c, ad82128->m_ram_tab[eq_seg][3]);
		regmap_write(ad82128->regmap, 0x2d, ad82128->m_ram_tab[eq_seg][4]);

		eq_seg += 1;
		/* write A0 */
		regmap_write(ad82128->regmap, 0x2e, ad82128->m_ram_tab[eq_seg][1]);
		regmap_write(ad82128->regmap, 0x2f, ad82128->m_ram_tab[eq_seg][2]);
		regmap_write(ad82128->regmap, 0x30, ad82128->m_ram_tab[eq_seg][3]);
		regmap_write(ad82128->regmap, 0x31, ad82128->m_ram_tab[eq_seg][4]);

		if(channel == 1)
	  		cmd = 0x02;
		else if(channel == 2)
	  		cmd = 0x42;

		regmap_write(ad82128->regmap, 0x32, cmd);

		eq_seg += 1;

		if(eq_seg > 0x4f)
	  	break;
	}

	for (eq_seg = 0x50; eq_seg < 0x78; eq_seg++) {

		if((eq_seg >= 0x5B) && (eq_seg <= 0x5E))
			continue;

		if((eq_seg >= 0x72) && (eq_seg <= 0x75))
			continue;

		regmap_write(ad82128->regmap, CFADDR, ad82128->m_ram_tab[eq_seg][0]);
		regmap_write(ad82128->regmap, A1CF1, ad82128->m_ram_tab[eq_seg][1]);
		regmap_write(ad82128->regmap, A1CF2, ad82128->m_ram_tab[eq_seg][2]);
		regmap_write(ad82128->regmap, A1CF3, ad82128->m_ram_tab[eq_seg][3]);
		regmap_write(ad82128->regmap, A1CF4, ad82128->m_ram_tab[eq_seg][4]);

		if(channel == 1)
	  		cmd = 0x01;
	  	else if(channel == 2)
	  		cmd = 0x41;
		regmap_write(ad82128->regmap, CFUD, cmd);
	}

	return 0;
}

static int ad82128_eq_mode_info(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_info *uinfo)
{
	uinfo->type   = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->access = (SNDRV_CTL_ELEM_ACCESS_TLV_READ | SNDRV_CTL_ELEM_ACCESS_READWRITE);
	uinfo->count  = 1;

	uinfo->value.integer.min  = AD82128_EQ_MODE_MIN;
	uinfo->value.integer.max  = AD82128_EQ_MODE_MAX;
	uinfo->value.integer.step = 1;

	return 0;
}

static int ad82128_eq_mode_get(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ad82128_data *ad82128 = snd_soc_component_get_drvdata(component);

	ucontrol->value.integer.value[0] = ad82128->eq_mode;

	return 0;
}

static int ad82128_eq_mode_put(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_component *component = snd_soc_kcontrol_component(kcontrol);
	struct ad82128_data *ad82128 = snd_soc_component_get_drvdata(component);
	int id_reg = 0xff;

	if((ucontrol->value.integer.value[0] > AD82128_EQ_MODE_MAX) || \
		(ucontrol->value.integer.value[0] < AD82128_EQ_MODE_MIN))
	{
		dev_err(component->dev, "error mode value setting, please check!\n");
		return -1;
	}

	id_reg=snd_soc_component_read(component, AD82128_DEVICE_ID_REG);
	if((id_reg&0xf0) != AD82128_DEVICE_ID)	/* amp i2c have not ack ,i2c error */
	{
		dev_err(component->dev, "error device id 0x%02x, please check!\n", id_reg);
		return -1;
	}

	ad82128->eq_mode = ucontrol->value.integer.value[0];

	dev_dbg(component->dev, "change ad82128 eq mode = %d\n", ad82128->eq_mode);

	if (ad82128->eq_mode == 1) {
			ad82128->m_ram_tab = eq_mode_1_ram1_tab;
			ad82128_change_eq_mode(component, 1);
		#ifdef CONFIG_SND_SOC_AD82128_2CHANNEL
			ad82128->m_ram_tab = eq_mode_1_ram2_tab;
			ad82128_change_eq_mode(component, 2);
		#endif
	}
	if(ad82128->eq_mode == 2) {
			ad82128->m_ram_tab = eq_mode_2_ram1_tab;
			ad82128_change_eq_mode(component, 1);
		#ifdef CONFIG_SND_SOC_AD82128_2CHANNEL
			ad82128->m_ram_tab = eq_mode_2_ram2_tab;
			ad82128_change_eq_mode(component, 2);
		#endif
	}

	/* add your other eq mode here
	 * ... 
	 */

	return 0;
}

static const struct snd_kcontrol_new ad82128_eq_mode_control[] = {
	{
		.iface = SNDRV_CTL_ELEM_IFACE_MIXER,
		.name  = "AD82128 EQ Mode",  /* Just fake the name */
		.info  = ad82128_eq_mode_info,
		.get   = ad82128_eq_mode_get,
		.put   = ad82128_eq_mode_put,
	},
};
#endif

static int ad82182_hw_free(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct snd_soc_component *component = dai->component;

	int ret;

	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL3_REG,
				AD82128_MUTE, AD82128_MUTE);
	if (ret < 0)
		dev_err(component->dev, "failed to write MUTE register: %d\n", ret);

	return 0;
}

static void ad82128_fault_check_work(struct work_struct *work)
{
	struct ad82128_data *ad82128 = container_of(work, struct ad82128_data,
			fault_check_work.work);
	struct device *dev = ad82128->component->dev;
	unsigned int curr_fault;
	int ret;

	ret = regmap_read(ad82128->regmap, AD82128_FAULT_REG, &curr_fault);
	if (ret < 0) {
		dev_err(dev, "failed to read FAULT register: %d\n", ret);
		goto out;
	}

	/* Check/handle all errors except SAIF clock errors */
	curr_fault &= AD82128_OCE | AD82128_DCE | AD82128_OTE | AD82128_UVE | AD82128_BSUVE;

	/*
	 * Only flag errors once for a given occurrence. This is needed as
	 * the AD82128 will take time clearing the fault condition internally
	 * during which we don't want to bombard the system with the same
	 * error message over and over.
	 */
	if (!(curr_fault & AD82128_OCE) && (ad82128->last_fault & AD82128_OCE))
		dev_crit(dev, "experienced an over current hardware fault\n");

	if (!(curr_fault & AD82128_DCE) && (ad82128->last_fault & AD82128_DCE))
		dev_crit(dev, "experienced a DC detection fault\n");

	if (!(curr_fault & AD82128_OTE) && (ad82128->last_fault & AD82128_OTE))
		dev_crit(dev, "experienced an over temperature fault\n");

	if (!(curr_fault & AD82128_UVE) && (ad82128->last_fault & AD82128_UVE))
		dev_crit(dev, "experienced an UV temperature fault\n");

	if (!(curr_fault & AD82128_BSUVE) && (ad82128->last_fault & AD82128_BSUVE))
		dev_crit(dev, "experienced an BSUV temperature fault\n");

	/* Store current fault value so we can detect any changes next time */
	ad82128->last_fault = curr_fault;

	if (curr_fault == (AD82128_OCE | AD82128_DCE | AD82128_OTE | AD82128_UVE | AD82128_BSUVE))
		goto out;

	/*
	 * Periodically toggle SDZ (shutdown bit) H->L->H to clear any latching
	 * faults as long as a fault condition persists. Always going through
	 * the full sequence no matter the first return value to minimizes
	 * chances for the device to end up in shutdown mode.
	 */
	dev_crit(dev, "toggle pd pin H->L->H to clear latching faults\n");

	ad82128_pd_gpio_set(ad82128->component, false);
	msleep(20);
	ad82128_pd_gpio_set(ad82128->component, true);

out:
	/* Schedule the next fault check at the specified interval */
	schedule_delayed_work(&ad82128->fault_check_work,
			      msecs_to_jiffies(AD82128_FAULT_CHECK_INTERVAL));
}

static int ad82128_codec_probe(struct snd_soc_component *component)
{
	struct ad82128_data *ad82128 = snd_soc_component_get_drvdata(component);
	/* unsigned int device_id, expected_device_id; */
	int ret;
	int i;
	int reg_data;
#ifdef AD82128_REG_RAM_CHECK
	int ram_h8_data;
	int ram_m8_data;
	int ram_l8_data;
	int ram_ll8_data;
#endif
	ad82128->component = component;

	ret = regulator_bulk_enable(ARRAY_SIZE(ad82128->supplies),
				    ad82128->supplies);
	if (ret != 0) {
		dev_err(component->dev, "failed to enable supplies: %d\n", ret);
		return ret;
	}

	msleep(10);

	ad82128_pd_gpio_set(component, true);	/* pull high amp PD pin */

	msleep(150);

	/* software reset amp */
	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL5_REG,
			  AD82128_SW_RESET, 0);
	if (ret < 0)
	goto error_snd_soc_component_update_bits;

	msleep(5);

	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL5_REG,
			  AD82128_SW_RESET, AD82128_SW_RESET);
	if (ret < 0)
	goto error_snd_soc_component_update_bits;

	msleep(20);

	/* Set device to mute */
	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL3_REG,
				  AD82128_MUTE, AD82128_MUTE);
	if (ret < 0)
		goto error_snd_soc_component_update_bits;

	/* Write register table */
	for(i = 0; i < AD82128_REGISTER_COUNT; i++)
	{
		reg_data = m_reg_tab[i][1];

		if(m_reg_tab[i][0] == 0x02)
			continue;

		if((m_reg_tab[i][0] >= 0x71)&&(m_reg_tab[i][0] <= 0x7C))
			continue;

		/* set stereo */
		if(m_reg_tab[i][0] == 0x1A)
		{
			reg_data &= (~0x40);
		}
		if(m_reg_tab[i][0] == 0x5B)
		{
			reg_data = 0x00;
		}
		if(m_reg_tab[i][0] == 0x5C)
		{
			reg_data = 0x00;
		}
		/* set stereo end */

		ret = regmap_write(ad82128->regmap, m_reg_tab[i][0], reg_data);
		if (ret < 0) {
			goto error_snd_soc_component_update_bits;
		}
	}

	/* Write ram1 */
	for (i = 0; i < AD82128_RAM_TABLE_COUNT; i++) {
		regmap_write(ad82128->regmap, CFADDR, m_ram1_tab[i][0]);
		regmap_write(ad82128->regmap, A1CF1, m_ram1_tab[i][1]);
		regmap_write(ad82128->regmap, A1CF2, m_ram1_tab[i][2]);
		regmap_write(ad82128->regmap, A1CF3, m_ram1_tab[i][3]);
		regmap_write(ad82128->regmap, A1CF4, m_ram1_tab[i][4]);
		regmap_write(ad82128->regmap, CFUD, 0x01);
	}
	/* Write ram2 */
	for (i = 0; i < AD82128_RAM_TABLE_COUNT; i++) {
		regmap_write(ad82128->regmap, CFADDR, m_ram2_tab[i][0]);
		regmap_write(ad82128->regmap, A1CF1, m_ram2_tab[i][1]);
		regmap_write(ad82128->regmap, A1CF2, m_ram2_tab[i][2]);
		regmap_write(ad82128->regmap, A1CF3, m_ram2_tab[i][3]);
		regmap_write(ad82128->regmap, A1CF4, m_ram2_tab[i][4]);
		regmap_write(ad82128->regmap, CFUD, 0x41);
	}

	msleep(2);

	/* Set device to unmute */
	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL3_REG,
				  AD82128_MUTE, 0);
	if (ret < 0)
		goto error_snd_soc_component_update_bits;

	INIT_DELAYED_WORK(&ad82128->fault_check_work, ad82128_fault_check_work);

#ifdef AD82128_CHANGE_EQ_MODE_EN
	ret = snd_soc_add_component_controls(component, ad82128_eq_mode_control, 1);
	if (ret != 0)
	{
	   	dev_err(component->dev, "Failed to register ad82128_eq_mode_control (%d)\n", ret);
	}
#endif

#ifdef AD82128_REG_RAM_CHECK

	msleep(1000);

	for(i = 0; i < AD82128_REGISTER_COUNT; i++) {
		reg_data=snd_soc_component_read(component, m_reg_tab[i][0]);
		/* printk("read ad82128 register {addr, data} = {0x%02x, 0x%02x}\n",
			   m_reg_tab[i][0], reg_data); */
	}

	for(i = 0; i < AD82128_RAM_TABLE_COUNT; i++) {
		regmap_write(ad82128->regmap, CFADDR, m_ram1_tab[i][0]); /* write ram addr */
		regmap_write(ad82128->regmap, CFUD, 0x04); /* write read a single ram data cmd */

		ram_h8_data=snd_soc_component_read(component, A1CF1);
		ram_m8_data=snd_soc_component_read(component, A1CF2);
		ram_l8_data=snd_soc_component_read(component, A1CF3);
		ram_ll8_data=snd_soc_component_read(component, A1CF4);
		/* printk("read ad82128 ram1 {addr, H8, M8, L8, LL8} = {0x%02x, 0x%02x, 0x%02x,
			   0x%02x, 0x%02x}\n", m_ram1_tab[i][0], ram_h8_data, ram_m8_data,
			   ram_l8_data, ram_ll8_data); */
	}
#endif

	return 0;

error_snd_soc_component_update_bits:
	dev_err(component->dev, "error configuring device registers: %d\n", ret);

/* probe_fail:
	regulator_bulk_disable(ARRAY_SIZE(ad82128->supplies),
			       ad82128->supplies); */
	return ret;
}

static void ad82128_codec_remove(struct snd_soc_component *component)
{
	struct ad82128_data *ad82128 = snd_soc_component_get_drvdata(component);
	int ret;

	cancel_delayed_work_sync(&ad82128->fault_check_work);

	ret = regulator_bulk_disable(ARRAY_SIZE(ad82128->supplies),
				     ad82128->supplies);
	if (ret < 0)
		dev_err(component->dev, "failed to disable supplies: %d\n", ret);
};

static int ad82128_dac_event(struct snd_soc_dapm_widget *w,
			     struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *component = snd_soc_dapm_to_component(w->dapm);
	struct ad82128_data *ad82128 = snd_soc_component_get_drvdata(component);
	int ret;

	if (event & SND_SOC_DAPM_POST_PMU) {
		/* Take AD82128 out of shutdown mode */

		ad82128_pd_gpio_set(component, true);	/* pull high amp PD pin */

		/*
		 * Observe codec shutdown-to-active time. The datasheet only
		 * lists a nominal value however just use-it as-is without
		 * additional padding to minimize the delay introduced in
		 * starting to play audio (actually there is other setup done
		 * by the ASoC framework that will provide additional delays,
		 * so we should always be safe).
		 */
		msleep(25);

		ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL3_REG,
			  AD82128_MUTE, 0x00);
		if (ret < 0)
			dev_err(component->dev, "failed to write MUTE register: %d\n", ret);

		/* Turn on AD82128 periodic fault checking/handling */
		ad82128->last_fault = 0xFE;
		schedule_delayed_work(&ad82128->fault_check_work,
				msecs_to_jiffies(AD82128_FAULT_CHECK_INTERVAL));
	} else if (event & SND_SOC_DAPM_PRE_PMD) {
		/* Disable AD82128 periodic fault checking/handling */
		cancel_delayed_work_sync(&ad82128->fault_check_work);

		/* Place AD82128 in shutdown mode to minimize current draw */
		ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL3_REG,
					  AD82128_MUTE, AD82128_MUTE);
		if (ret < 0)
			dev_err(component->dev, "failed to write MUTE register: %d\n", ret);

		msleep(20);

		ad82128_pd_gpio_set(component, false);	/* pull low amp PD pin */
	}

	return 0;
}

#ifdef CONFIG_PM
static int ad82128_suspend(struct snd_soc_component *component)
{
	struct ad82128_data *ad82128 = snd_soc_component_get_drvdata(component);

	snd_sunxi_regulator_disable(ad82128->rglt);

	return 0;
}

static int ad82128_resume(struct snd_soc_component *component)
{
	struct ad82128_data *ad82128 = snd_soc_component_get_drvdata(component);
	int ret;
	int i;
	int reg_data;

	ret = snd_sunxi_regulator_enable(ad82128->rglt);
	if (ret)
		return ret;

	msleep(20);
	ad82128_pd_gpio_set(component, true);
	msleep(20);
	/* Set device to mute */
	ret = snd_soc_component_update_bits(component, AD82128_STATE_CTRL3_REG,
				  AD82128_MUTE, AD82128_MUTE);
	if (ret < 0)
		goto error_snd_soc_component_update_bits;

	/* Write register table */
	for(i = 0; i < AD82128_REGISTER_COUNT; i++)
	{
		reg_data = m_reg_tab[i][1];

		if(m_reg_tab[i][0] == 0x02)
			continue;

		if((m_reg_tab[i][0] >= 0x71)&&(m_reg_tab[i][0] <= 0x7C))
			continue;

		/* set stereo */
		if(m_reg_tab[i][0] == 0x1A)
		{
			reg_data &= (~0x40);
		}
		if(m_reg_tab[i][0] == 0x5B)
		{
			reg_data = 0x00;
		}
		if(m_reg_tab[i][0] == 0x5C)
		{
			reg_data = 0x00;
		}
		/* set stereo end */

		ret = regmap_write(ad82128->regmap, m_reg_tab[i][0], reg_data);
		if (ret < 0) {
			goto error_snd_soc_component_update_bits;
		}
	}

	/* Write ram1 */
	for (i = 0; i < AD82128_RAM_TABLE_COUNT; i++) {
		regmap_write(ad82128->regmap, CFADDR, m_ram1_tab[i][0]);
		regmap_write(ad82128->regmap, A1CF1, m_ram1_tab[i][1]);
		regmap_write(ad82128->regmap, A1CF2, m_ram1_tab[i][2]);
		regmap_write(ad82128->regmap, A1CF3, m_ram1_tab[i][3]);
		regmap_write(ad82128->regmap, A1CF4, m_ram1_tab[i][4]);
		regmap_write(ad82128->regmap, CFUD, 0x01);
	}
	/* Write ram2 */
	for (i = 0; i < AD82128_RAM_TABLE_COUNT; i++) {
		regmap_write(ad82128->regmap, CFADDR, m_ram2_tab[i][0]);
		regmap_write(ad82128->regmap, A1CF1, m_ram2_tab[i][1]);
		regmap_write(ad82128->regmap, A1CF2, m_ram2_tab[i][2]);
		regmap_write(ad82128->regmap, A1CF3, m_ram2_tab[i][3]);
		regmap_write(ad82128->regmap, A1CF4, m_ram2_tab[i][4]);
		regmap_write(ad82128->regmap, CFUD, 0x41);
	}

	/* msleep(2); */

	/* regcache_cache_only(ad82128->regmap, false);

	 * ret = regcache_sync(ad82128->regmap);
	 * if (ret < 0) {
	 *	dev_err(component->dev, "failed to sync regcache: %d\n", ret);
	 *	return ret;
	 * }
	 */
	return 0;

error_snd_soc_component_update_bits:
	dev_err(component->dev, "error configuring device registers: %d\n", ret);

	return ret;
}
#else
#define ad82128_suspend NULL
#define ad82128_resume NULL
#endif

static bool ad82128_is_volatile_reg(struct device *dev, unsigned int reg)
{
#ifdef	AD82128_REG_RAM_CHECK
	if(reg <= AD82128_MAX_REG)
		return true;
	else
		return false;
#else
	switch (reg) {
		case AD82128_FAULT_REG:
		case AD82128_STATE_CTRL1_REG:
		case AD82128_STATE_CTRL2_REG:
		case AD82128_STATE_CTRL3_REG:
		case AD82128_STATE_CTRL5_REG:
		case AD82128_DEVICE_ID_REG:
			return true;
		default:
			return false;
	}
#endif
}

static const struct regmap_config ad82128_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = AD82128_MAX_REG,
	/* .reg_defaults = m_reg_tab, */
	.cache_type = REGCACHE_RBTREE,
	.volatile_reg = ad82128_is_volatile_reg,
};

/*
 * DAC analog gain. There are four discrete values to select from, ranging
 * from 19.2 dB to 26.3dB.
 */
static const DECLARE_TLV_DB_RANGE(dac_analog_tlv,
	0x0, 0x0, TLV_DB_SCALE_ITEM(1920, 0, 0),
	0x1, 0x1, TLV_DB_SCALE_ITEM(2070, 0, 0),
	0x2, 0x2, TLV_DB_SCALE_ITEM(2350, 0, 0),
	0x3, 0x3, TLV_DB_SCALE_ITEM(2630, 0, 0),
);

/*
 * DAC digital volumes. From -103.5 to 24 dB in 0.5 dB steps. Note that
 * setting the gain below -100 dB (register value <0x7) is effectively a MUTE
 * as per device datasheet.
 */
static DECLARE_TLV_DB_SCALE(dac_tlv, -10350, 50, 0);

static const struct snd_kcontrol_new ad82128_snd_controls[] = {
	SOC_SINGLE_TLV("Speaker Driver Playback Volume",
		       AD82128_VOLUME_CTRL_REG, 0, 0xff, 0, dac_tlv),
	SOC_SINGLE_TLV("Speaker Driver Analog Gain", AD82128_ANALOG_CTRL_REG,
		       AD82128_ANALOG_GAIN_SHIFT, 3, 0, dac_analog_tlv),
};

static const struct snd_soc_dapm_widget ad82128_dapm_widgets[] = {
	SND_SOC_DAPM_AIF_IN("DAC IN", "Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_DAC_E("DAC", NULL, SND_SOC_NOPM, 0, 0, ad82128_dac_event,
			   SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_OUTPUT("OUT")
};

static const struct snd_soc_dapm_route ad82128_audio_map[] = {
	{ "DAC", NULL, "DAC IN" },
	{ "OUT", NULL, "DAC" },
};

static const struct snd_soc_component_driver soc_component_dev_ad82128 = {
	.probe			= ad82128_codec_probe,
	.remove			= ad82128_codec_remove,
	.suspend		= ad82128_suspend,
	.resume			= ad82128_resume,
	.controls		= ad82128_snd_controls,
	.num_controls		= ARRAY_SIZE(ad82128_snd_controls),
	.dapm_widgets		= ad82128_dapm_widgets,
	.num_dapm_widgets	= ARRAY_SIZE(ad82128_dapm_widgets),
	.dapm_routes		= ad82128_audio_map,
	.num_dapm_routes	= ARRAY_SIZE(ad82128_audio_map),
	.idle_bias_on		= 1,
	.use_pmdown_time	= 1,
	.endianness		= 1,
	/* .non_legacy_dai_naming	= 1, */
	.legacy_dai_naming      = 1,
};

/* PCM rates supported by the AD82128 driver */
#define AD82128_RATES	(SNDRV_PCM_RATE_44100 | SNDRV_PCM_RATE_48000 |\
			 SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000)

/* Formats supported by AD82128 driver */
/*
#define AD82128_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S18_LE |\
			 SNDRV_PCM_FMTBIT_S20_LE | SNDRV_PCM_FMTBIT_S24_LE)
*/
#define AD82128_FORMATS (SNDRV_PCM_FMTBIT_S16_LE |\
			 SNDRV_PCM_FMTBIT_S20_LE | SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static ssize_t __enable_sound(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	msleep(30);

	ad82128_pd_gpio_set(ad82128_data_new->component, true);

	return 0;
}
static DEVICE_ATTR(enable_sound, 0664, __enable_sound, NULL);

static ssize_t __disable_sound(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ad82128_pd_gpio_set(ad82128_data_new->component, false);

	return 0;
}
static DEVICE_ATTR(disable_sound, 0664, __disable_sound, NULL);

static struct attribute *ad82128_sound_attrs[] = {
	&dev_attr_enable_sound.attr,
	&dev_attr_disable_sound.attr,
	NULL,
};

static struct attribute_group ad82128_sound_attr_group = {
	.attrs = ad82128_sound_attrs,
};

static const struct snd_soc_dai_ops ad82128_speaker_dai_ops = {
	.hw_params	= ad82128_hw_params,
	.set_fmt	= ad82128_set_dai_fmt,
	/* .digital_mute	= ad82128_mute, */
	/* .mute_stream = ad82128_mute, */
	.hw_free = ad82182_hw_free,
};

/*
 * AD82128 DAI structure
 *
 * Note that were are advertising .playback.channels_max = 2 despite this being
 * a mono amplifier. The reason for that is that some serial ports such as ESMT's
 * McASP module have a minimum number of channels (2) that they can output.
 * Advertising more channels than we have will allow us to interface with such
 * a serial port without really any negative side effects as the AD82128 will
 * simply ignore any extra channel(s) asides from the one channel that is
 * configured to be played back.
 */
static struct snd_soc_dai_driver ad82128_dai[] = {
	{
		.name = "ad82128",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = AD82128_RATES,
			.formats = AD82128_FORMATS,
		},
		.ops = &ad82128_speaker_dai_ops,
	},
};

static int ad82128_parse_dt(struct ad82128_data *ad82128, struct device *dev)
{
    int ret = 0;
    struct device_node *np = dev->of_node;

    ad82128->pd_gpio = of_get_named_gpio(np, "pd-gpio", 0);

    if (!gpio_is_valid(ad82128->pd_gpio))
    {
        dev_err(dev, "%s get invalid extamp-pd-gpio %d\n", __func__, ad82128->pd_gpio);
        ret = -EINVAL;
    }

    ret = devm_gpio_request_one(dev, ad82128->pd_gpio, GPIOF_OUT_INIT_HIGH, "ad82128-pd-pin");
    if (ret < 0)
    {
	dev_err(dev, "%s: Failed to request GPIO %d, error %d\n", __func__, ad82128->pd_gpio, ret);
        return ret;
    }
    return 0;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int ad82128_probe(struct i2c_client *client)
#else
static int ad82128_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
#endif
{
	struct device *dev = &client->dev;
	struct ad82128_data *data;
	const struct regmap_config *regmap_config;
	int ret;
	int i;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	data->ad82128_client = client;

	regmap_config = &ad82128_regmap_config;

	data->regmap = devm_regmap_init_i2c(client, regmap_config);
	if (IS_ERR(data->regmap)) {
		ret = PTR_ERR(data->regmap);
		dev_err(dev, "failed to allocate register map: %d\n", ret);
		return ret;
	}

	for (i = 0; i < ARRAY_SIZE(data->supplies); i++)
		data->supplies[i].supply = ad82128_supply_names[i];

	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(data->supplies),
				      data->supplies);
	if (ret != 0) {
		dev_err(dev, "failed to request supplies: %d\n", ret);
		return ret;
	}

	dev_set_drvdata(dev, data);

	ad82128_parse_dt(data, &client->dev);

	data->rglt = snd_sunxi_regulator_init(&client->dev);

	i2c_set_clientdata(client, data);

	ret = devm_snd_soc_register_component(&client->dev,
				     &soc_component_dev_ad82128,
				     ad82128_dai, ARRAY_SIZE(ad82128_dai));
	if (ret < 0) {
		dev_err(dev, "failed to register component: %d\n", ret);
		return ret;
	}

	ret = sysfs_create_group(&client->dev.kobj, &ad82128_sound_attr_group);
	if (ret) {
		dev_err(&client->dev, "failed to create attr group\n");
	}

	return 0;
}

static const struct i2c_device_id ad82128_id[] = {
	{ "ad82128", AD82128 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ad82128_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id ad82128_of_match[] = {
	{ .compatible = "ESMT,ad82128", },
	{ },
};
MODULE_DEVICE_TABLE(of, ad82128_of_match);
#endif

static struct i2c_driver ad82128_i2c_driver = {
	.driver = {
		.name = "ad82128",
		.of_match_table = of_match_ptr(ad82128_of_match),
	},
	.probe = ad82128_probe,
	.id_table = ad82128_id,
};

module_i2c_driver(ad82128_i2c_driver);

MODULE_AUTHOR("ESMT BU2");
MODULE_DESCRIPTION("AD82128 Audio amplifier driver");
MODULE_LICENSE("GPL");

// SPDX-License-Identifier: GPL-2.0
/*
 * ov13850 driver
 *
 * Copyright (C) 2017 Fuzhou Rockchip Electronics Co., Ltd.
 *
 * V0.0X01.0X01 add poweron function.
 *
 * Copyright (C) 2021 Sebastian Fricke
 *
 * V0.0X01.0X02
 *	- Refactor the driver to use macros as labels
 *	- Add the get_selection subdevice ioctl
 *	- Add the ability to subscribe to events
 *	- Remove the rockchip camera module
 *	- Add the hflip and vflip controls
 *	- Remove unnecessary if-guards
 */

#include <linux/clk.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <media/media-entity.h>
#include <media/v4l2-async.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/v4l2-subdev.h>
#include <linux/pinctrl/consumer.h>
#include <linux/compat.h>

#define DRIVER_VERSION				KERNEL_VERSION(0, 0x01, 0x05)

#ifndef V4L2_CID_DIGITAL_GAIN
#define V4L2_CID_DIGITAL_GAIN			V4L2_CID_GAIN
#endif

#define OV13850_LINK_FREQ_300MHZ		300000000
/* pixel rate = link frequency * 2 * lanes / BITS_PER_SAMPLE */
#define OV13850_PIXEL_RATE			(OV13850_LINK_FREQ_300MHZ * 2 * 2 / 10)
#define OV13850_XVCLK_FREQ			24000000

#define CHIP_ID					0x00d850
#define OV13850_REG_CHIP_ID			0x300a

#define OV13850_REG_CTRL_MODE			0x0100
#define OV13850_MODE_SW_STANDBY			0x0
#define OV13850_MODE_STREAMING			BIT(0)

#define OV13850_SOFTWARE_RESET			0x0103
#define OV13850_DEBUG_MODE			0x3210

/* Programmable clock divider */
#define OV13850_PLL1_PRE_DIVIDER_P		0x030a
#define OV13850_PLL1_CLOCK_PRE_DIVIDER		0x0300
#define OV13850_PLL1_CLOCK_MULTIPLIER_1		0x0301
#define OV13850_PLL1_CLOCK_MULTIPLIER_2		0x0302
#define OV13850_PLL1_CLOCK_DIVIDER_M		0x0303

/* MIPI */
#define OV13850_MIPI_SYSTEM_CONTROL		0x300f
#define OV13850_MIPI_PHY_HIGH			0x3010
#define OV13850_MIPI_PHY_LOW			0x3011
#define OV13850_MIPI_SYSTEM_CONTROL_CTRL_0	0x3012
#define OV13850_MIPI_SYSTEM_CONTROL_CTRL_1	0x3013
#define OV13850_MIPI_SYSTEM_CONTROL_CTRL_2	0x3014
#define OV13850_MIPI_SYSTEM_CONTROL_CTRL_3	0x3015
#define OV13850_SYSTEM_CONTROL_MISC_CTRL	0x301f

/* Exposure */
#define OV13850_REG_LONG_EXPOSURE		0x3500
#define OV13850_REG_LONG_EXPOSURE_2		0x3501
#define OV13850_REG_LONG_EXPOSURE_3		0x3502
#define OV13850_REG_SHORT_EXPOSURE		0x3506
#define OV13850_REG_SHORT_EXPOSURE_2		0x3507
#define OV13850_REG_SHORT_EXPOSURE_3		0x3508
#define	OV13850_EXPOSURE_MIN			4
#define	OV13850_EXPOSURE_STEP			1

#define OV13850_VTS_MAX				0x7fff

/* Gain */
#define OV13850_REG_GAIN_H			0x350a
#define OV13850_REG_GAIN_L			0x350b
#define OV13850_REG_GAIN_H_SHORT		0x350e
#define OV13850_REG_GAIN_L_SHORT		0x350f
#define OV13850_GAIN_H_MASK			0x07
#define OV13850_GAIN_H_SHIFT			8
#define OV13850_GAIN_L_MASK			0xff
#define OV13850_GAIN_MIN			0x10
#define OV13850_GAIN_MAX			0xf8
#define OV13850_GAIN_STEP			1
#define OV13850_GAIN_DEFAULT			0x10

/* Analog control registers */
#define OV13850_REG_ANALOG_CTRL_1		0x3600
#define OV13850_REG_ANALOG_CTRL_2		0x3601
#define OV13850_REG_ANALOG_CTRL_3		0x3602
#define OV13850_REG_ANALOG_CTRL_4		0x3603
#define OV13850_REG_ANALOG_CTRL_5		0x3604
#define OV13850_REG_ANALOG_CTRL_6		0x3605
#define OV13850_REG_ANALOG_CTRL_7		0x3606
#define OV13850_REG_ANALOG_CTRL_8		0x3607
#define OV13850_REG_ANALOG_CTRL_9		0x3608
#define OV13850_REG_ANALOG_CTRL_10		0x3609
#define OV13850_REG_ANALOG_CTRL_11		0x360a
#define OV13850_REG_ANALOG_CTRL_12		0x360b
#define OV13850_REG_ANALOG_CTRL_13		0x360c
#define OV13850_REG_ANALOG_CTRL_14		0x360d
#define OV13850_REG_ANALOG_CTRL_15		0x360e
#define OV13850_REG_ANALOG_CTRL_16		0x360f
#define OV13850_REG_ANALOG_SIGNAL_PROCESSING_17 0x3611
#define OV13850_REG_ANALOG_SIGNAL_PROCESSING_18 0x3612
#define OV13850_REG_ANALOG_SIGNAL_PROCESSING_19 0x3613
#define OV13850_REG_ANALOG_SIGNAL_PROCESSING_21 0x3615

/* One time programmable registers */
#define OV13850_REG_OTP_MODE_CTRL		0x3d84
#define OV13850_REG_OTP_85			0x3d85
#define OV13850_REG_OTP_LOAD_START_HIGH_ADDR	0x3d8c
#define OV13850_REG_OTP_LOAD_START_LOW_ADDR	0x3d8d

/* Cropping & Windowing registers */
#define OV13850_REG_H_CROP_START_HIGH		0x3800
#define OV13850_REG_H_CROP_START_LOW		0x3801
#define OV13850_REG_V_CROP_START_HIGH		0x3802
#define OV13850_REG_V_CROP_START_LOW		0x3803
#define OV13850_REG_H_CROP_END_HIGH		0x3804
#define OV13850_REG_H_CROP_END_LOW		0x3805
#define OV13850_REG_V_CROP_END_HIGH		0x3806
#define OV13850_REG_V_CROP_END_LOW		0x3807
#define OV13850_REG_H_CROP_OUTPUT_SIZE_HIGH	0x3808
#define OV13850_REG_H_CROP_OUTPUT_SIZE_LOW	0x3809
#define OV13850_REG_V_CROP_OUTPUT_SIZE_HIGH	0x380a
#define OV13850_REG_V_CROP_OUTPUT_SIZE_LOW	0x380b
#define OV13850_REG_TIMING_HTS_HIGH		0x380c
#define OV13850_REG_TIMING_HTS_LOW		0x380d
#define OV13850_REG_TIMING_VTS_HIGH		0x380e
#define OV13850_REG_TIMING_VTS_LOW		0x380f
#define OV13850_REG_H_WIN_OFFSET_HIGH		0x3810
#define OV13850_REG_H_WIN_OFFSET_LOW		0x3811
#define OV13850_REG_V_WIN_OFFSET_HIGH		0x3812
#define OV13850_REG_V_WIN_OFFSET_LOW		0x3813
#define OV13850_REG_H_SUB_SAMPLE_INCREASE_NUM	0x3814
#define OV13850_REG_V_SUB_SAMPLE_INCREASE_NUM	0x3815
#define OV13850_REG_FORMAT_0			0x3820
#define OV13850_REG_FORMAT_1			0x3821
#define OV13850_REG_34				0x3834
#define OV13850_REG_35				0x3835

/* Black Level Calibration (BLC) */
#define OV13850_REG_BLC_CTRL00			0x4000
#define OV13850_REG_BLC_CTRL01			0x4001

/* VFIFO - Virtual First In First Out - special DMA */
#define OV13850_REG_VFIFO_READ_START		0x4601
#define OV13850_REG_VFIFO_2			0x4602
#define OV13850_REG_VFIFO_3			0x4603

/* ISP general controls */
#define OV13850_REG_ISP_CTRL0			0x5000
#define OV13850_REG_ISP_CTRL1			0x5001
/* Not found in the datasheet */
#define OV13850_REG_ISP_CTRL2			0x5002
#define OV13850_REG_ISP_CTRL5			0x5005

/* Defect Pixel cancelation */
#define OV13850_REG_DPC_CTRL0			0x5300
#define OV13850_REG_DPC_CTRL1			0x5301
#define OV13850_REG_DPC_CTRL2			0x5302
#define OV13850_REG_WTH_REGLIST1		0x5303
#define OV13850_REG_BTH_REGLIST2		0x5304
#define OV13850_REG_THRE_1			0x5305
#define OV13850_REG_THRE_2			0x5306
#define OV13850_REG_THRE_3			0x5307
#define OV13850_REG_THRE_4			0x5308
#define OV13850_REG_WTHRE_LIST0			0x5309
#define OV13850_REG_WTHRE_LIST1			0x530a
#define OV13850_REG_WTHRE_LIST2			0x530b
#define OV13850_REG_WTHRE_LIST3			0x530c
#define OV13850_REG_BTHRE_LIST0			0x530d
#define OV13850_REG_BTHRE_LIST1			0x530e
#define OV13850_REG_BTHRE_LIST2			0x530f
#define OV13850_REG_BTHRE_LIST3			0x5310

#define OV13850_REG_TEST_PATTERN		0x5e00
#define	OV13850_TEST_PATTERN_ENABLE		0x80
#define	OV13850_TEST_PATTERN_DISABLE		0x0

#define REG_NULL				0xFFFF

#define OV13850_REG_VALUE_08BIT			1
#define OV13850_REG_VALUE_16BIT			2
#define OV13850_REG_VALUE_24BIT			3

#define OV13850_LANES				2
#define OV13850_BITS_PER_SAMPLE			10

#define OV13850_CHIP_REVISION_REG		0x302A
#define OV13850_R1A				0xb1
#define OV13850_R2A				0xb2

#define OF_CAMERA_PINCTRL_STATE_DEFAULT		"rockchip,camera_default"
#define OF_CAMERA_PINCTRL_STATE_SLEEP		"rockchip,camera_sleep"

#define OV13850_NAME				"ov13850"

/* OV13850 active pixel array size */
#define OV13850_PIXEL_ARRAY_LEFT		16U
#define OV13850_PIXEL_ARRAY_TOP			8U
#define OV13850_PIXEL_ARRAY_WIDTH		4224U
#define OV13850_PIXEL_ARRAY_HEIGHT		3136U
#define OV13850_NATIVE_WIDTH			4256U
#define OV13850_NATIVE_HEIGHT			3152U

static const struct regval *ov13850_global_regs;

static const char * const ov13850_supply_names[] = {
	"avdd",		/* Analog power */
	"dovdd",	/* Digital I/O power */
	"dvdd",		/* Digital core power */
};

#define OV13850_NUM_SUPPLIES ARRAY_SIZE(ov13850_supply_names)

struct regval {
	u16 addr;
	u8 val;
};

struct ov13850_mode {
	/* Frame width & height */
	u32 width;
	u32 height;

	/* Analog crop rectangle */
	struct v4l2_rect crop;

	struct v4l2_fract max_fps;
	u32 hts_def;
	u32 vts_def;
	u32 exp_def;
	const struct regval *reg_list;
};

struct ov13850 {
	struct i2c_client	*client;
	struct clk		*xvclk;
	struct gpio_desc	*power_gpio;
	struct gpio_desc	*reset_gpio;
	struct gpio_desc	*pwdn_gpio;
	struct regulator_bulk_data supplies[OV13850_NUM_SUPPLIES];

	struct pinctrl		*pinctrl;
	struct pinctrl_state	*pins_default;
	struct pinctrl_state	*pins_sleep;

	struct v4l2_subdev	subdev;
	struct media_pad	pad;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl	*exposure;
	struct v4l2_ctrl	*anal_gain;
	struct v4l2_ctrl	*digi_gain;
	struct v4l2_ctrl	*hblank;
	struct v4l2_ctrl	*vblank;
	struct v4l2_ctrl	*test_pattern;
	struct v4l2_ctrl	*hflip;
	struct v4l2_ctrl	*vflip;

	struct mutex		mutex;
	bool			streaming;
	bool			power_on;
	const struct ov13850_mode *cur_mode;
	u32			module_index;
};

#define to_ov13850(sd) container_of(sd, struct ov13850, subdev)

/*
 * Xclk 24Mhz
 */
static const struct regval ov13850_global_regs_r1a[] = {
	{OV13850_SOFTWARE_RESET,			0x01},
	{OV13850_PLL1_CLOCK_PRE_DIVIDER,		0x00},
	{OV13850_PLL1_CLOCK_MULTIPLIER_1,		0x00},
	{OV13850_PLL1_CLOCK_MULTIPLIER_2,		0x32},
	{OV13850_PLL1_CLOCK_DIVIDER_M,			0x01},
	{OV13850_PLL1_PRE_DIVIDER_P,			0x00},
	/* enalbed and 8-bit mode */
	{OV13850_MIPI_SYSTEM_CONTROL,			0x11},
	{OV13850_MIPI_PHY_HIGH,				0x01},
	{OV13850_MIPI_PHY_LOW,				0x76},
	{OV13850_MIPI_SYSTEM_CONTROL_CTRL_0,		0x21},
	{OV13850_MIPI_SYSTEM_CONTROL_CTRL_1,		0x12},
	{OV13850_MIPI_SYSTEM_CONTROL_CTRL_2,		0x11},
	{OV13850_MIPI_SYSTEM_CONTROL_CTRL_3,		0xc0},
	{OV13850_SYSTEM_CONTROL_MISC_CTRL,		0x03},
	{0x3106,					0x00},
	{OV13850_DEBUG_MODE,				0x47},
	{OV13850_REG_LONG_EXPOSURE,			0x00},
	{OV13850_REG_LONG_EXPOSURE_2,			0x60},
	{OV13850_REG_LONG_EXPOSURE_3,			0x00},
	{OV13850_REG_SHORT_EXPOSURE,			0x00},
	{OV13850_REG_SHORT_EXPOSURE_2,			0x02},
	{OV13850_REG_SHORT_EXPOSURE_3,			0x00},
	{OV13850_REG_GAIN_H,				0x00},
	{OV13850_REG_GAIN_L,				0x80},
	{OV13850_REG_GAIN_H_SHORT,			0x00},
	{OV13850_REG_GAIN_L_SHORT,			0x10},
	{OV13850_REG_ANALOG_CTRL_1,			0x40},
	{OV13850_REG_ANALOG_CTRL_2,			0xfc},
	{OV13850_REG_ANALOG_CTRL_3,			0x02},
	{OV13850_REG_ANALOG_CTRL_4,			0x48},
	{OV13850_REG_ANALOG_CTRL_5,			0xa5},
	{OV13850_REG_ANALOG_CTRL_6,			0x9f},
	{OV13850_REG_ANALOG_CTRL_8,			0x00},
	{OV13850_REG_ANALOG_CTRL_11,			0x40},
	{OV13850_REG_ANALOG_CTRL_12,			0x91},
	{OV13850_REG_ANALOG_CTRL_13,			0x49},
	{OV13850_REG_ANALOG_CTRL_16,			0x8a},
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_17,	0x10},
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_18,	0x27},
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_19,	0x33},
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_21,	0x08},
	{0x3641,					0x02},
	{0x3660,					0x82},
	{0x3668,					0x54},
	{0x3669,					0x40},
	{0x3667,					0xa0},
	{0x3702,					0x40},
	{0x3703,					0x44},
	{0x3704,					0x2c},
	{0x3705,					0x24},
	{0x3706,					0x50},
	{0x3707,					0x44},
	{0x3708,					0x3c},
	{0x3709,					0x1f},
	{0x370a,					0x26},
	{0x370b,					0x3c},
	{0x3720,					0x66},
	{0x3722,					0x84},
	{0x3728,					0x40},
	{0x372a,					0x00},
	{0x372f,					0x90},
	{0x3710,					0x28},
	{0x3716,					0x03},
	{0x3718,					0x10},
	{0x3719,					0x08},
	{0x371c,					0xfc},
	{0x3760,					0x13},
	{0x3761,					0x34},
	{0x3767,					0x24},
	{0x3768,					0x06},
	{0x3769,					0x45},
	{0x376c,					0x23},
	{OV13850_REG_OTP_MODE_CTRL,			0x00},
	{OV13850_REG_OTP_85,				0x17},
	{OV13850_REG_OTP_LOAD_START_HIGH_ADDR,		0x73},
	{OV13850_REG_OTP_LOAD_START_LOW_ADDR,		0xbf},
	{OV13850_REG_H_CROP_START_HIGH,			0x00},
	{OV13850_REG_H_CROP_START_LOW,			0x08},
	{OV13850_REG_V_CROP_START_HIGH,			0x00},
	{OV13850_REG_V_CROP_START_LOW,			0x04},
	{OV13850_REG_H_CROP_END_HIGH,			0x10},
	{OV13850_REG_H_CROP_END_LOW,			0x97},
	{OV13850_REG_V_CROP_END_HIGH,			0x0c},
	{OV13850_REG_V_CROP_END_LOW,			0x4b},
	{OV13850_REG_H_CROP_OUTPUT_SIZE_HIGH,		0x08},
	{OV13850_REG_H_CROP_OUTPUT_SIZE_LOW,		0x40},
	{OV13850_REG_V_CROP_OUTPUT_SIZE_HIGH,		0x06},
	{OV13850_REG_V_CROP_OUTPUT_SIZE_LOW,		0x20},
	{OV13850_REG_TIMING_HTS_HIGH,			0x12},
	{OV13850_REG_TIMING_HTS_LOW,			0xc0},
	{OV13850_REG_TIMING_VTS_HIGH,			0x06},
	{OV13850_REG_TIMING_VTS_LOW,			0x80},
	{OV13850_REG_H_WIN_OFFSET_HIGH,			0x00},
	{OV13850_REG_H_WIN_OFFSET_LOW,			0x04},
	{OV13850_REG_V_WIN_OFFSET_HIGH,			0x00},
	{OV13850_REG_V_WIN_OFFSET_LOW,			0x02},
	{OV13850_REG_H_SUB_SAMPLE_INCREASE_NUM,		0x31},
	{OV13850_REG_V_SUB_SAMPLE_INCREASE_NUM,		0x31},
	 /* Vertical binning */
	{OV13850_REG_FORMAT_0,				0x02},
	 /* Mirror & horizontal binning */
	{OV13850_REG_FORMAT_1,				0x05},
	{OV13850_REG_34,				0x00},
	 /* cut_en, vts_auto_en, blk_col_dis */
	{OV13850_REG_35,				0x1c},
	{0x3836,					0x08},
	{0x3837,					0x02},
	/* Back light compensation:
	 * Enable offset out of range signal
	 * Enable format change signal
	 * Enable gain change signal
	 * Enable exposure change signal
	 * Enable 5-point median filter function signal
	 */
	{OV13850_REG_BLC_CTRL00,			0xf1},
	{OV13850_REG_BLC_CTRL01,			0x00},
	{0x400b,					0x0c},
	{0x4011,					0x00},
	{0x401a,					0x00},
	{0x401b,					0x00},
	{0x401c,					0x00},
	{0x401d,					0x00},
	{0x4020,					0x00},
	{0x4021,					0xE4},
	{0x4022,					0x07},
	{0x4023,					0x5F},
	{0x4024,					0x08},
	{0x4025,					0x44},
	{0x4026,					0x08},
	{0x4027,					0x47},
	{0x4028,					0x00},
	{0x4029,					0x02},
	{0x402a,					0x04},
	{0x402b,					0x08},
	{0x402c,					0x02},
	{0x402d,					0x02},
	{0x402e,					0x0c},
	{0x402f,					0x08},
	{0x403d,					0x2c},
	{0x403f,					0x7f},
	{0x4500,					0x82},
	{0x4501,					0x38},
	{OV13850_REG_VFIFO_READ_START,			0x04},
	{OV13850_REG_VFIFO_2,				0x22},
	{OV13850_REG_VFIFO_3,				0x01},
	{0x4837,					0x1b},
	{0x4d00,					0x04},
	{0x4d01,					0x42},
	{0x4d02,					0xd1},
	{0x4d03,					0x90},
	{0x4d04,					0x66},
	{0x4d05,					0x65},
	{OV13850_REG_ISP_CTRL0,				0x0e},
	{OV13850_REG_ISP_CTRL1,				0x01},
	{OV13850_REG_ISP_CTRL2,				0x07},
	{0x5013,					0x40},
	{0x501c,					0x00},
	{0x501d,					0x10},
	{0x5242,					0x00},
	{0x5243,					0xb8},
	{0x5244,					0x00},
	{0x5245,					0xf9},
	{0x5246,					0x00},
	{0x5247,					0xf6},
	{0x5248,					0x00},
	{0x5249,					0xa6},
	{OV13850_REG_DPC_CTRL0,				0xfc},
	{OV13850_REG_DPC_CTRL1,				0xdf},
	{OV13850_REG_DPC_CTRL2,				0x3f},
	{OV13850_REG_WTH_REGLIST1,			0x08},
	{OV13850_REG_BTH_REGLIST2,			0x0c},
	{OV13850_REG_THRE_1,				0x10},
	{OV13850_REG_THRE_2,				0x20},
	{OV13850_REG_THRE_3,				0x40},
	{OV13850_REG_THRE_4,				0x08},
	{OV13850_REG_WTHRE_LIST0,			0x08},
	{OV13850_REG_WTHRE_LIST1,			0x02},
	{OV13850_REG_WTHRE_LIST2,			0x01},
	{OV13850_REG_WTHRE_LIST3,			0x01},
	{OV13850_REG_BTHRE_LIST0,			0x0c},
	{OV13850_REG_BTHRE_LIST1,			0x02},
	{OV13850_REG_BTHRE_LIST2,			0x01},
	{OV13850_REG_BTHRE_LIST3,			0x01},
	{0x5400,					0x00},
	{0x5401,					0x61},
	{0x5402,					0x00},
	{0x5403,					0x00},
	{0x5404,					0x00},
	{0x5405,					0x40},
	{0x540c,					0x05},
	{0x5b00,					0x00},
	{0x5b01,					0x00},
	{0x5b02,					0x01},
	{0x5b03,					0xff},
	{0x5b04,					0x02},
	{0x5b05,					0x6c},
	{0x5b09,					0x02},
	{OV13850_REG_TEST_PATTERN,			0x00},
	{0x5e10,					0x1c},
	{REG_NULL,					0x00},
};

/*
 * Xclk 24Mhz
 */
static const struct regval ov13850_global_regs_r2a[] = {
	{OV13850_PLL1_CLOCK_PRE_DIVIDER,		0x01},
	{OV13850_PLL1_CLOCK_MULTIPLIER_1,		0x00},
	{OV13850_PLL1_CLOCK_MULTIPLIER_2,		0x28},
	{OV13850_PLL1_CLOCK_DIVIDER_M,			0x00},
	{OV13850_PLL1_PRE_DIVIDER_P,			0x00},
	/* enabled and 8-bit mode */
	{OV13850_MIPI_SYSTEM_CONTROL,			0x11},
	{OV13850_MIPI_PHY_HIGH,				0x01},
	{OV13850_MIPI_PHY_LOW,				0x76},
	/* phy pad enabled and 2-lane mode */
	{OV13850_MIPI_SYSTEM_CONTROL_CTRL_0,		0x21},
	{OV13850_MIPI_SYSTEM_CONTROL_CTRL_1,		0x12},
	{OV13850_MIPI_SYSTEM_CONTROL_CTRL_2,		0x11},
	{OV13850_SYSTEM_CONTROL_MISC_CTRL,		0x03},
	{0x3106,					0x00},
	{OV13850_DEBUG_MODE,				0x47},
	{OV13850_REG_LONG_EXPOSURE,			0x00},
	{OV13850_REG_LONG_EXPOSURE_2,			0x60},
	{OV13850_REG_LONG_EXPOSURE_3,			0x00},
	{OV13850_REG_SHORT_EXPOSURE,			0x00},
	{OV13850_REG_SHORT_EXPOSURE_2,			0x02},
	{OV13850_REG_SHORT_EXPOSURE_3,			0x00},
	{OV13850_REG_GAIN_H,				0x00},
	{OV13850_REG_GAIN_L,				0x80},
	{OV13850_REG_GAIN_H_SHORT,			0x00},
	{OV13850_REG_GAIN_L_SHORT,			0x10},
	{0x351a,					0x00},
	{0x351b,					0x10},
	{0x351c,					0x00},
	{0x351d,					0x20},
	{0x351e,					0x00},
	{0x351f,					0x40},
	{0x3520,					0x00},
	{0x3521,					0x80},
	{OV13850_REG_ANALOG_CTRL_1,			0xc0},
	{OV13850_REG_ANALOG_CTRL_2,			0xfc},
	{OV13850_REG_ANALOG_CTRL_3,			0x02},
	{OV13850_REG_ANALOG_CTRL_4,			0x78},
	{OV13850_REG_ANALOG_CTRL_5,			0xb1},
	{OV13850_REG_ANALOG_CTRL_6,			0xb5},
	{OV13850_REG_ANALOG_CTRL_7,			0x73},
	{OV13850_REG_ANALOG_CTRL_8,			0x07},
	{OV13850_REG_ANALOG_CTRL_10,			0x40},
	{OV13850_REG_ANALOG_CTRL_11,			0x30},
	{OV13850_REG_ANALOG_CTRL_12,			0x91},
	{OV13850_REG_ANALOG_CTRL_13,			0x09},
	{OV13850_REG_ANALOG_CTRL_16,			0x02},
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_17,	0x10},
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_18,	0x27},
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_19,	0x33},
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_21,	0x0c},
	{0x3616,					0x0e},
	{0x3641,					0x02},
	{0x3660,					0x82},
	{0x3668,					0x54},
	{0x3669,					0x00},
	{0x366a,					0x3f},
	{0x3667,					0xa0},
	{0x3702,					0x40},
	{0x3703,					0x44},
	{0x3704,					0x2c},
	{0x3705,					0x01},
	{0x3706,					0x15},
	{0x3707,					0x44},
	{0x3708,					0x3c},
	{0x3709,					0x1f},
	{0x370a,					0x27},
	{0x370b,					0x3c},
	{0x3720,					0x55},
	{0x3722,					0x84},
	{0x3728,					0x40},
	{0x372a,					0x00},
	{0x372b,					0x02},
	{0x372e,					0x22},
	{0x372f,					0x90},
	{0x3730,					0x00},
	{0x3731,					0x00},
	{0x3732,					0x00},
	{0x3733,					0x00},
	{0x3710,					0x28},
	{0x3716,					0x03},
	{0x3718,					0x10},
	{0x3719,					0x0c},
	{0x371a,					0x08},
	{0x371c,					0xfc},
	{0x3748,					0x00},
	{0x3760,					0x13},
	{0x3761,					0x33},
	{0x3762,					0x86},
	{0x3763,					0x16},
	{0x3767,					0x24},
	{0x3768,					0x06},
	{0x3769,					0x45},
	{0x376c,					0x23},
	{0x376f,					0x80},
	{0x3773,					0x06},
	{OV13850_REG_OTP_MODE_CTRL,			0x00},
	{OV13850_REG_OTP_85,				0x17},
	{OV13850_REG_OTP_LOAD_START_HIGH_ADDR,		0x73},
	{OV13850_REG_OTP_LOAD_START_LOW_ADDR,		0xbf},
	{OV13850_REG_H_CROP_START_HIGH,			0x00},
	{OV13850_REG_H_CROP_START_LOW,			0x08},
	{OV13850_REG_V_CROP_START_HIGH,			0x00},
	{OV13850_REG_V_CROP_START_LOW,			0x04},
	{OV13850_REG_H_CROP_END_HIGH,			0x10},
	{OV13850_REG_H_CROP_END_LOW,			0x97},
	{OV13850_REG_V_CROP_END_HIGH,			0x0c},
	{OV13850_REG_V_CROP_END_LOW,			0x4b},
	{OV13850_REG_H_CROP_OUTPUT_SIZE_HIGH,		0x08},
	{OV13850_REG_H_CROP_OUTPUT_SIZE_LOW,		0x40},
	{OV13850_REG_V_CROP_OUTPUT_SIZE_HIGH,		0x06},
	{OV13850_REG_V_CROP_OUTPUT_SIZE_LOW,		0x20},
	{OV13850_REG_TIMING_HTS_HIGH,			0x12},
	{OV13850_REG_TIMING_HTS_LOW,			0xc0},
	{OV13850_REG_TIMING_VTS_HIGH,			0x06},
	{OV13850_REG_TIMING_VTS_LOW,			0x80},
	{OV13850_REG_H_WIN_OFFSET_HIGH,			0x00},
	{OV13850_REG_H_WIN_OFFSET_LOW,			0x04},
	{OV13850_REG_V_WIN_OFFSET_HIGH,			0x00},
	{OV13850_REG_V_WIN_OFFSET_LOW,			0x02},
	{OV13850_REG_H_SUB_SAMPLE_INCREASE_NUM,		0x31},
	{OV13850_REG_V_SUB_SAMPLE_INCREASE_NUM,		0x31},
	/* Vertical binning */
	{OV13850_REG_FORMAT_0,				0x02},
	/* Mirror & digital subsample */
	{OV13850_REG_FORMAT_1,				0x06},
	{0x3823,					0x00},
	{0x3826,					0x00},
	{0x3827,					0x02},
	{OV13850_REG_34,				0x00},
	/* cut_en, vts_auto_en, blk_col_dis */
	{OV13850_REG_35,				0x1c},
	{0x3836,					0x08},
	{0x3837,					0x02},
	/* Back light compensation:
	 * Enable offset out of range signal
	 * Enable format change signal
	 * Enable gain change signal
	 * Enable exposure change signal
	 * Enable 5-point median filter function signal
	 */
	{OV13850_REG_BLC_CTRL00,			0xf1},
	{OV13850_REG_BLC_CTRL01,			0x00},
	{0x4006,					0x04},
	{0x4007,					0x04},
	{0x400b,					0x0c},
	{0x4011,					0x00},
	{0x401a,					0x00},
	{0x401b,					0x00},
	{0x401c,					0x00},
	{0x401d,					0x00},
	{0x4020,					0x00},
	{0x4021,					0xe4},
	{0x4022,					0x04},
	{0x4023,					0xd7},
	{0x4024,					0x05},
	{0x4025,					0xbc},
	{0x4026,					0x05},
	{0x4027,					0xbf},
	{0x4028,					0x00},
	{0x4029,					0x02},
	{0x402a,					0x04},
	{0x402b,					0x08},
	{0x402c,					0x02},
	{0x402d,					0x02},
	{0x402e,					0x0c},
	{0x402f,					0x08},
	{0x403d,					0x2c},
	{0x403f,					0x7f},
	{0x4041,					0x07},
	{0x4500,					0x82},
	{0x4501,					0x3c},
	{0x458b,					0x00},
	{0x459c,					0x00},
	{0x459d,					0x00},
	{0x459e,					0x00},
	{OV13850_REG_VFIFO_READ_START,			0x83},
	{OV13850_REG_VFIFO_2,				0x22},
	{OV13850_REG_VFIFO_3,				0x01},
	{0x4837,					0x19},
	{0x4d00,					0x04},
	{0x4d01,					0x42},
	{0x4d02,					0xd1},
	{0x4d03,					0x90},
	{0x4d04,					0x66},
	{0x4d05,					0x65},
	{0x4d0b,					0x00},
	{OV13850_REG_ISP_CTRL0,				0x0e},
	{OV13850_REG_ISP_CTRL1,				0x01},
	{OV13850_REG_ISP_CTRL2,				0x07},
	{0x5013,					0x40},
	{0x501c,					0x00},
	{0x501d,					0x10},
	{0x510f,					0xfc},
	{0x5110,					0xf0},
	{0x5111,					0x10},
	{0x536d,					0x02},
	{0x536e,					0x67},
	{0x536f,					0x01},
	{0x5370,					0x4c},
	{0x5400,					0x00},
	{0x5400,					0x00},
	{0x5401,					0x61},
	{0x5402,					0x00},
	{0x5403,					0x00},
	{0x5404,					0x00},
	{0x5405,					0x40},
	{0x540c,					0x05},
	{0x5501,					0x00},
	{0x5b00,					0x00},
	{0x5b01,					0x00},
	{0x5b02,					0x01},
	{0x5b03,					0xff},
	{0x5b04,					0x02},
	{0x5b05,					0x6c},
	{0x5b09,					0x02},
	{OV13850_REG_TEST_PATTERN,			0x00},
	{0x5e10,					0x1c},
	{REG_NULL,					0x00},
};

/*
 * Xclk 24Mhz
 * max_framerate 30fps
 * mipi_datarate per lane 600Mbps
 */
static const struct regval ov13850_2112x1568_regs[] = {
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_18,	0x27},
	{0x370a,					0x26},
	{0x372a,					0x00},
	{0x372f,					0x90},
	{OV13850_REG_H_CROP_START_LOW,			0x08},
	{OV13850_REG_H_CROP_END_LOW,			0x97},
	{OV13850_REG_V_CROP_END_LOW,			0x4b},
	{OV13850_REG_H_CROP_OUTPUT_SIZE_HIGH,		0x08},
	{OV13850_REG_H_CROP_OUTPUT_SIZE_LOW,		0x40},
	{OV13850_REG_V_CROP_OUTPUT_SIZE_HIGH,		0x06},
	{OV13850_REG_V_CROP_OUTPUT_SIZE_LOW,		0x20},
	{OV13850_REG_TIMING_HTS_HIGH,			0x12},
	{OV13850_REG_TIMING_HTS_LOW,			0xc0},
	{OV13850_REG_TIMING_VTS_HIGH,			0x06},
	{OV13850_REG_TIMING_VTS_LOW,			0x80},
	{OV13850_REG_V_WIN_OFFSET_LOW,			0x02},
	{OV13850_REG_H_SUB_SAMPLE_INCREASE_NUM,		0x31},
	{OV13850_REG_V_SUB_SAMPLE_INCREASE_NUM,		0x31},
	/* Vertical binning (f?) */
	{OV13850_REG_FORMAT_0,				0x02},
	/* Mirror & Horizontal binning */
	{OV13850_REG_FORMAT_1,				0x05},
	{0x3836,					0x08},
	{0x3837,					0x02},
	{OV13850_REG_VFIFO_READ_START,			0x04},
	{OV13850_REG_VFIFO_3,				0x00},
	{0x4020,					0x00},
	{0x4021,					0xE4},
	{0x4022,					0x07},
	{0x4023,					0x5F},
	{0x4024,					0x08},
	{0x4025,					0x44},
	{0x4026,					0x08},
	{0x4027,					0x47},
	{0x5401,					0x61},
	{0x5405,					0x40},
	{REG_NULL,					0x00},
};

/*
 * Xclk 24Mhz
 * max_framerate 7fps
 * mipi_datarate per lane 600Mbps
 */
static const struct regval ov13850_4224x3136_regs[] = {
	{OV13850_REG_ANALOG_SIGNAL_PROCESSING_18,	0x2f},
	{0x370a,					0x24},
	{0x372a,					0x04},
	{0x372f,					0xa0},
	{OV13850_REG_H_CROP_START_LOW,			0x0C},
	{OV13850_REG_H_CROP_END_LOW,			0x93},
	{OV13850_REG_V_CROP_END_LOW,			0x4B},
	{OV13850_REG_H_CROP_OUTPUT_SIZE_HIGH,		0x10},
	{OV13850_REG_H_CROP_OUTPUT_SIZE_LOW,		0x80},
	{OV13850_REG_V_CROP_OUTPUT_SIZE_HIGH,		0x0c},
	{OV13850_REG_V_CROP_OUTPUT_SIZE_LOW,		0x40},
	{OV13850_REG_TIMING_VTS_HIGH,			0x0d},
	{OV13850_REG_TIMING_VTS_LOW,			0x00},
	{OV13850_REG_V_WIN_OFFSET_LOW,			0x04},
	{OV13850_REG_H_SUB_SAMPLE_INCREASE_NUM,		0x11},
	{OV13850_REG_V_SUB_SAMPLE_INCREASE_NUM,		0x11},
	{OV13850_REG_FORMAT_0,				0x00},
	/* Mirror on */
	{OV13850_REG_FORMAT_1,				0x04},
	{0x3836,					0x04},
	{0x3837,					0x01},
	{OV13850_REG_VFIFO_READ_START,			0x87},
	{OV13850_REG_VFIFO_3,				0x01},
	{0x4020,					0x02},
	{0x4021,					0x4C},
	{0x4022,					0x0E},
	{0x4023,					0x37},
	{0x4024,					0x0F},
	{0x4025,					0x1C},
	{0x4026,					0x0F},
	{0x4027,					0x1F},
	{0x5401,					0x71},
	{0x5405,					0x80},
	{REG_NULL,					0x00},
};

static const struct ov13850_mode supported_modes[] = {
	/* 2x2 binned 30 fps mode */
	{
		.width = 2112,
		.height = 1568,
		.crop = {
			.left = OV13850_PIXEL_ARRAY_LEFT,
			.top = OV13850_PIXEL_ARRAY_TOP,
			.width = OV13850_PIXEL_ARRAY_WIDTH,
			.height = OV13850_PIXEL_ARRAY_HEIGHT
		},
		.max_fps = {
			.numerator = 10000,
			.denominator = 300000,
		},
		.exp_def = 0x0600,
		.hts_def = 0x12c0,
		.vts_def = 0x0680,
		.reg_list = ov13850_2112x1568_regs,
	},
	/* 13.2 megapixel 30fps full resolution mode */
	{
		.width = 4224,
		.height = 3136,
		.crop = {
			.left = OV13850_PIXEL_ARRAY_LEFT,
			.top = OV13850_PIXEL_ARRAY_TOP,
			.width = OV13850_PIXEL_ARRAY_WIDTH,
			.height = OV13850_PIXEL_ARRAY_HEIGHT
		},
		.max_fps = {
			.numerator = 20000,
			.denominator = 150000,
		},
		.exp_def = 0x0600,
		.hts_def = 0x12c0,
		.vts_def = 0x0d00,
		.reg_list = ov13850_4224x3136_regs,
	}
};

static const s64 link_freq_menu_items[] = {
	OV13850_LINK_FREQ_300MHZ
};

static const char * const ov13850_test_pattern_menu[] = {
	"Disabled",
	"Vertical Color Bar Type 1",
	"Vertical Color Bar Type 2",
	"Vertical Color Bar Type 3",
	"Vertical Color Bar Type 4"
};

/* Write registers up to 4 at a time */
static int ov13850_write_reg(struct i2c_client *client, u16 reg,
			     u32 len, u32 val)
{
	u32 buf_i, val_i;
	u8 buf[6];
	u8 *val_p;
	__be32 val_be;

	if (len > 4)
		return -EINVAL;

	buf[0] = reg >> 8;
	buf[1] = reg & 0xff;

	val_be = cpu_to_be32(val);
	val_p = (u8 *)&val_be;
	buf_i = 2;
	val_i = 4 - len;

	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	if (i2c_master_send(client, buf, len + 2) != len + 2)
		return -EIO;

	return 0;
}

static int ov13850_write_array(struct i2c_client *client,
			       const struct regval *regs)
{
	u32 i;
	int ret = 0;

	for (i = 0; ret == 0 && regs[i].addr != REG_NULL; i++)
		ret = ov13850_write_reg(client, regs[i].addr,
					OV13850_REG_VALUE_08BIT,
					regs[i].val);

	return ret;
}

/* Read registers up to 4 at a time */
static int ov13850_read_reg(struct i2c_client *client, u16 reg,
			    unsigned int len, u32 *val)
{
	struct i2c_msg msgs[2];
	u8 *data_be_p;
	__be32 data_be = 0;
	__be16 reg_addr_be = cpu_to_be16(reg);
	int ret;

	if (len > 4 || !len)
		return -EINVAL;

	data_be_p = (u8 *)&data_be;
	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = (u8 *)&reg_addr_be;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_be_p[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = be32_to_cpu(data_be);

	return 0;
}

static int ov13850_get_reso_dist(const struct ov13850_mode *mode,
				 struct v4l2_mbus_framefmt *framefmt)
{
	return abs(mode->width - framefmt->width) +
	       abs(mode->height - framefmt->height);
}

static const struct ov13850_mode *
ov13850_find_best_fit(struct v4l2_subdev_format *fmt)
{
	struct v4l2_mbus_framefmt *framefmt = &fmt->format;
	int dist;
	int cur_best_fit = 0;
	int cur_best_fit_dist = -1;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(supported_modes); i++) {
		dist = ov13850_get_reso_dist(&supported_modes[i], framefmt);
		if (cur_best_fit_dist == -1 || dist < cur_best_fit_dist) {
			cur_best_fit_dist = dist;
			cur_best_fit = i;
		}
	}

	return &supported_modes[cur_best_fit];
}

static int ov13850_set_fmt(struct v4l2_subdev *sd,
			   struct v4l2_subdev_state *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct ov13850 *ov13850 = to_ov13850(sd);
	const struct ov13850_mode *mode;
	s64 h_blank, vblank_def;

	mutex_lock(&ov13850->mutex);

	mode = ov13850_find_best_fit(fmt);
	fmt->format.code = MEDIA_BUS_FMT_SBGGR10_1X10;
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.field = V4L2_FIELD_NONE;
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		*v4l2_subdev_get_try_format(sd, cfg, fmt->pad) = fmt->format;
	} else {
		ov13850->cur_mode = mode;
		h_blank = mode->hts_def - mode->width;
		__v4l2_ctrl_modify_range(ov13850->hblank, h_blank,
					 h_blank, 1, h_blank);
		vblank_def = mode->vts_def - mode->height;
		__v4l2_ctrl_modify_range(ov13850->vblank, vblank_def,
					 OV13850_VTS_MAX - mode->height,
					 1, vblank_def);
	}

	mutex_unlock(&ov13850->mutex);

	return 0;
}

static int ov13850_get_fmt(struct v4l2_subdev *sd,
			   struct v4l2_subdev_state *cfg,
			   struct v4l2_subdev_format *fmt)
{
	struct ov13850 *ov13850 = to_ov13850(sd);
	const struct ov13850_mode *mode = ov13850->cur_mode;

	mutex_lock(&ov13850->mutex);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		fmt->format = *v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
	} else {
		fmt->format.width = mode->width;
		fmt->format.height = mode->height;
		fmt->format.code = MEDIA_BUS_FMT_SBGGR10_1X10;
		fmt->format.field = V4L2_FIELD_NONE;
	}
	mutex_unlock(&ov13850->mutex);

	return 0;
}

static const struct v4l2_rect *
__ov13850_get_pad_crop(struct ov13850 *ov13850,
		       struct v4l2_subdev_state *cfg,
		       unsigned int pad, enum v4l2_subdev_format_whence which)
{
	switch (which) {
	case V4L2_SUBDEV_FORMAT_TRY:
		return v4l2_subdev_get_try_crop(&ov13850->subdev, cfg, pad);
	case V4L2_SUBDEV_FORMAT_ACTIVE:
		return &ov13850->cur_mode->crop;
	}

	return NULL;
}

static int ov13850_get_selection(struct v4l2_subdev *sd,
				 struct v4l2_subdev_state *cfg,
				 struct v4l2_subdev_selection *sel)
{
	switch (sel->target) {
	case V4L2_SEL_TGT_CROP: {
		struct ov13850 *ov13850 = to_ov13850(sd);

		mutex_lock(&ov13850->mutex);
		sel->r = *__ov13850_get_pad_crop(ov13850, cfg, sel->pad,
						 sel->which);
		mutex_unlock(&ov13850->mutex);

		return 0;
	}

	case V4L2_SEL_TGT_NATIVE_SIZE:
		sel->r.top = 0;
		sel->r.left = 0;
		sel->r.width = OV13850_NATIVE_WIDTH;
		sel->r.height = OV13850_NATIVE_HEIGHT;

		return 0;

	case V4L2_SEL_TGT_CROP_DEFAULT:
	case V4L2_SEL_TGT_CROP_BOUNDS:
		sel->r.top = OV13850_PIXEL_ARRAY_TOP;
		sel->r.left = OV13850_PIXEL_ARRAY_LEFT;
		sel->r.width = OV13850_PIXEL_ARRAY_WIDTH;
		sel->r.height = OV13850_PIXEL_ARRAY_HEIGHT;

		return 0;
	};

	return -EINVAL;
}

static int ov13850_enum_mbus_code(struct v4l2_subdev *sd,
				  struct v4l2_subdev_state *cfg,
				  struct v4l2_subdev_mbus_code_enum *code)
{
	if (code->index != 0)
		return -EINVAL;
	code->code = MEDIA_BUS_FMT_SBGGR10_1X10;

	return 0;
}

static int ov13850_enum_frame_sizes(struct v4l2_subdev *sd,
				    struct v4l2_subdev_state *cfg,
				   struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index >= ARRAY_SIZE(supported_modes))
		return -EINVAL;

	if (fse->code != MEDIA_BUS_FMT_SBGGR10_1X10)
		return -EINVAL;

	fse->min_width  = supported_modes[fse->index].width;
	fse->max_width  = supported_modes[fse->index].width;
	fse->max_height = supported_modes[fse->index].height;
	fse->min_height = supported_modes[fse->index].height;

	return 0;
}

static int ov13850_enable_test_pattern(struct ov13850 *ov13850, u32 pattern)
{
	u32 val;

	if (pattern)
		val = (pattern - 1) | OV13850_TEST_PATTERN_ENABLE;
	else
		val = OV13850_TEST_PATTERN_DISABLE;

	return ov13850_write_reg(ov13850->client,
				 OV13850_REG_TEST_PATTERN,
				 OV13850_REG_VALUE_08BIT,
				 val);
}

static int ov13850_g_frame_interval(struct v4l2_subdev *sd,
				    struct v4l2_subdev_frame_interval *fi)
{
	struct ov13850 *ov13850 = to_ov13850(sd);
	const struct ov13850_mode *mode = ov13850->cur_mode;

	mutex_lock(&ov13850->mutex);
	fi->interval = mode->max_fps;
	mutex_unlock(&ov13850->mutex);

	return 0;
}

static int __ov13850_start_stream(struct ov13850 *ov13850)
{
	int ret;

	ret = ov13850_write_array(ov13850->client, ov13850->cur_mode->reg_list);
	if (ret)
		return ret;

	/* In case these controls are set before streaming */
	mutex_unlock(&ov13850->mutex);
	ret = v4l2_ctrl_handler_setup(&ov13850->ctrl_handler);
	mutex_lock(&ov13850->mutex);
	if (ret)
		return ret;

	return ov13850_write_reg(ov13850->client,
				 OV13850_REG_CTRL_MODE,
				 OV13850_REG_VALUE_08BIT,
				 OV13850_MODE_STREAMING);
}

static int __ov13850_stop_stream(struct ov13850 *ov13850)
{
	return ov13850_write_reg(ov13850->client,
				 OV13850_REG_CTRL_MODE,
				 OV13850_REG_VALUE_08BIT,
				 OV13850_MODE_SW_STANDBY);
}

static int ov13850_s_stream(struct v4l2_subdev *sd, int on)
{
	struct ov13850 *ov13850 = to_ov13850(sd);
	struct i2c_client *client = ov13850->client;
	int ret = 0;

	mutex_lock(&ov13850->mutex);
	on = !!on;
	if (on == ov13850->streaming)
		goto unlock_and_return;

	if (on) {
		ret = pm_runtime_get_sync(&client->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(&client->dev);
			goto unlock_and_return;
		}

		ret = __ov13850_start_stream(ov13850);
		if (ret) {
			v4l2_err(sd, "start stream failed while write regs\n");
			pm_runtime_put(&client->dev);
			goto unlock_and_return;
		}
	} else {
		__ov13850_stop_stream(ov13850);
		pm_runtime_put(&client->dev);
	}

	ov13850->streaming = on;

unlock_and_return:
	mutex_unlock(&ov13850->mutex);

	return ret;
}

static int ov13850_s_power(struct v4l2_subdev *sd, int on)
{
	struct ov13850 *ov13850 = to_ov13850(sd);
	struct i2c_client *client = ov13850->client;
	int ret = 0;

	mutex_lock(&ov13850->mutex);

	/* If the power state is not modified - no work to do. */
	if (ov13850->power_on == !!on)
		goto unlock_and_return;

	if (on) {
		ret = pm_runtime_get_sync(&client->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(&client->dev);
			goto unlock_and_return;
		}

		ret = ov13850_write_array(ov13850->client, ov13850_global_regs);
		if (ret) {
			v4l2_err(sd, "could not set init registers\n");
			pm_runtime_put_noidle(&client->dev);
			goto unlock_and_return;
		}

		ov13850->power_on = true;
	} else {
		pm_runtime_put(&client->dev);
		ov13850->power_on = false;
	}

unlock_and_return:
	mutex_unlock(&ov13850->mutex);

	return ret;
}

/* Calculate the delay in us by clock rate and clock cycles */
static inline u32 ov13850_cal_delay(u32 cycles)
{
	return DIV_ROUND_UP(cycles, OV13850_XVCLK_FREQ / 1000 / 1000);
}

static int __ov13850_power_on(struct ov13850 *ov13850)
{
	int ret;
	u32 delay_us;
	struct device *dev = &ov13850->client->dev;

	if (!IS_ERR(ov13850->power_gpio))
		gpiod_set_value_cansleep(ov13850->power_gpio, 1);

	if (!IS_ERR_OR_NULL(ov13850->pins_default)) {
		ret = pinctrl_select_state(ov13850->pinctrl,
					   ov13850->pins_default);
		if (ret < 0)
			dev_err(dev, "could not set pins\n");
	}

	ret = clk_prepare_enable(ov13850->xvclk);
	if (ret < 0) {
		dev_err(dev, "Failed to enable xvclk\n");
		return ret;
	}

	if (!IS_ERR(ov13850->reset_gpio))
		gpiod_set_value_cansleep(ov13850->reset_gpio, 0);

	ret = regulator_bulk_enable(OV13850_NUM_SUPPLIES, ov13850->supplies);
	if (ret < 0) {
		dev_err(dev, "Failed to enable regulators\n");
		goto disable_clk;
	}

	if (!IS_ERR(ov13850->reset_gpio))
		gpiod_set_value_cansleep(ov13850->reset_gpio, 1);

	usleep_range(500, 1000);
	if (!IS_ERR(ov13850->pwdn_gpio))
		gpiod_set_value_cansleep(ov13850->pwdn_gpio, 1);

	/* 8192 cycles prior to first SCCB transaction */
	delay_us = ov13850_cal_delay(8192);
	usleep_range(delay_us, delay_us * 2);

	return 0;

disable_clk:
	clk_disable_unprepare(ov13850->xvclk);

	return ret;
}

static void __ov13850_power_off(struct ov13850 *ov13850)
{
	int ret;
	struct device *dev = &ov13850->client->dev;

	if (!IS_ERR(ov13850->pwdn_gpio))
		gpiod_set_value_cansleep(ov13850->pwdn_gpio, 0);
	clk_disable_unprepare(ov13850->xvclk);
	if (!IS_ERR(ov13850->reset_gpio))
		gpiod_set_value_cansleep(ov13850->reset_gpio, 0);
	if (!IS_ERR_OR_NULL(ov13850->pins_sleep)) {
		ret = pinctrl_select_state(ov13850->pinctrl,
					   ov13850->pins_sleep);
		if (ret < 0)
			dev_dbg(dev, "could not set pins\n");
	}
	if (!IS_ERR(ov13850->power_gpio))
		gpiod_set_value_cansleep(ov13850->power_gpio, 0);

	regulator_bulk_disable(OV13850_NUM_SUPPLIES, ov13850->supplies);
}

static int ov13850_runtime_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13850 *ov13850 = to_ov13850(sd);

	return __ov13850_power_on(ov13850);
}

static int ov13850_runtime_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13850 *ov13850 = to_ov13850(sd);

	__ov13850_power_off(ov13850);

	return 0;
}

static int ov13850_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct ov13850 *ov13850 = to_ov13850(sd);
	struct v4l2_mbus_framefmt *try_fmt =
				v4l2_subdev_get_try_format(sd, fh->state, 0);
	const struct ov13850_mode *def_mode = &supported_modes[0];

	mutex_lock(&ov13850->mutex);
	/* Initialize try_fmt */
	try_fmt->width = def_mode->width;
	try_fmt->height = def_mode->height;
	try_fmt->code = MEDIA_BUS_FMT_SBGGR10_1X10;
	try_fmt->field = V4L2_FIELD_NONE;

	mutex_unlock(&ov13850->mutex);
	/* No crop or compose */

	return 0;
}

static const struct dev_pm_ops ov13850_pm_ops = {
	SET_RUNTIME_PM_OPS(ov13850_runtime_suspend,
			   ov13850_runtime_resume, NULL)
};

static const struct v4l2_subdev_internal_ops ov13850_internal_ops = {
	.open = ov13850_open,
};

static const struct v4l2_subdev_core_ops ov13850_core_ops = {
	.s_power = ov13850_s_power,
	.subscribe_event = v4l2_ctrl_subdev_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops ov13850_video_ops = {
	.s_stream = ov13850_s_stream,
	.g_frame_interval = ov13850_g_frame_interval,
};

static const struct v4l2_subdev_pad_ops ov13850_pad_ops = {
	.enum_mbus_code = ov13850_enum_mbus_code,
	.enum_frame_size = ov13850_enum_frame_sizes,
	.get_fmt = ov13850_get_fmt,
	.set_fmt = ov13850_set_fmt,
	.get_selection = ov13850_get_selection,
};

static const struct v4l2_subdev_ops ov13850_subdev_ops = {
	.core	= &ov13850_core_ops,
	.video	= &ov13850_video_ops,
	.pad	= &ov13850_pad_ops,
};

static int ov13850_set_hflip(struct ov13850 *ov13850, int value)
{
	int cur_reg_value = 0;
	ov13850_read_reg(ov13850->client, OV13850_REG_FORMAT_1,
			 OV13850_REG_VALUE_08BIT, &cur_reg_value);
	if (cur_reg_value & BIT(2)) {
		if (value)
			return 0;
		else
			cur_reg_value ^= BIT(2);
	} else {
		if (value)
			cur_reg_value ^= BIT(2);
		else
			return 0;
	}

	return ov13850_write_reg(ov13850->client, OV13850_REG_FORMAT_1,
				 OV13850_REG_VALUE_08BIT, cur_reg_value);
}

static int ov13850_set_vflip(struct ov13850 *ov13850, int value)
{
	int cur_reg_value = 0;
	ov13850_read_reg(ov13850->client, OV13850_REG_FORMAT_0,
			 OV13850_REG_VALUE_08BIT, &cur_reg_value);
	if (cur_reg_value & BIT(2)) {
		if (value)
			return 0;
		else
			cur_reg_value ^= BIT(2);
	} else {
		if (value)
			cur_reg_value ^= BIT(2);
		else
			return 0;
	}

	return ov13850_write_reg(ov13850->client, OV13850_REG_FORMAT_0,
				 OV13850_REG_VALUE_08BIT, cur_reg_value);
}

static int ov13850_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct ov13850 *ov13850 = container_of(ctrl->handler,
					     struct ov13850, ctrl_handler);
	struct i2c_client *client = ov13850->client;
	s64 max;
	int ret = 0;

	/* Propagate change of current control to all related controls */
	switch (ctrl->id) {
	case V4L2_CID_VBLANK:
		/* Update max exposure while meeting expected vblanking */
		max = ov13850->cur_mode->height + ctrl->val - 4;
		__v4l2_ctrl_modify_range(ov13850->exposure,
					 ov13850->exposure->minimum, max,
					 ov13850->exposure->step,
					 ov13850->exposure->default_value);
		break;
	}

	if (pm_runtime_get(&client->dev) <= 0)
		return 0;

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		/* 4 least significant bits of exposure are fractional part */
		ret = ov13850_write_reg(ov13850->client,
					OV13850_REG_LONG_EXPOSURE,
					OV13850_REG_VALUE_24BIT,
					ctrl->val << 4);
		break;
	case V4L2_CID_ANALOGUE_GAIN:
		ret = ov13850_write_reg(ov13850->client,
					OV13850_REG_GAIN_H,
					OV13850_REG_VALUE_08BIT,
					(ctrl->val >> OV13850_GAIN_H_SHIFT) &
					OV13850_GAIN_H_MASK);
		ret |= ov13850_write_reg(ov13850->client,
					 OV13850_REG_GAIN_L,
					 OV13850_REG_VALUE_08BIT,
					 ctrl->val & OV13850_GAIN_L_MASK);
		break;
	case V4L2_CID_VBLANK:
		ret = ov13850_write_reg(ov13850->client,
					OV13850_REG_TIMING_VTS_HIGH,
					OV13850_REG_VALUE_16BIT,
					ctrl->val + ov13850->cur_mode->height);
		break;
	case V4L2_CID_TEST_PATTERN:
		ret = ov13850_enable_test_pattern(ov13850, ctrl->val);
		break;
	case V4L2_CID_HFLIP:
		ret = ov13850_set_hflip(ov13850, ctrl->val);
		break;
	case V4L2_CID_VFLIP:
		ret = ov13850_set_vflip(ov13850, ctrl->val);
		break;
	default:
		dev_warn(&client->dev, "%s Unhandled id:0x%x, val:0x%x\n",
			 __func__, ctrl->id, ctrl->val);
		break;
	}

	pm_runtime_put(&client->dev);

	return ret;
}

static const struct v4l2_ctrl_ops ov13850_ctrl_ops = {
	.s_ctrl = ov13850_set_ctrl,
};

static int ov13850_initialize_controls(struct ov13850 *ov13850)
{
	const struct ov13850_mode *mode;
	struct v4l2_ctrl_handler *handler;
	struct v4l2_ctrl *ctrl;
	s64 exposure_max, vblank_def;
	u32 h_blank;
	int ret;

	handler = &ov13850->ctrl_handler;
	mode = ov13850->cur_mode;
	ret = v4l2_ctrl_handler_init(handler, 8);
	if (ret)
		return ret;
	handler->lock = &ov13850->mutex;

	ctrl = v4l2_ctrl_new_int_menu(handler, NULL, V4L2_CID_LINK_FREQ,
				      0, 0, link_freq_menu_items);
	if (ctrl)
		ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	v4l2_ctrl_new_std(handler, NULL, V4L2_CID_PIXEL_RATE,
			  0, OV13850_PIXEL_RATE, 1, OV13850_PIXEL_RATE);

	h_blank = mode->hts_def - mode->width;
	ov13850->hblank = v4l2_ctrl_new_std(handler, NULL, V4L2_CID_HBLANK,
				h_blank, h_blank, 1, h_blank);
	if (ov13850->hblank)
		ov13850->hblank->flags |= V4L2_CTRL_FLAG_READ_ONLY;

	vblank_def = mode->vts_def - mode->height;
	ov13850->vblank = v4l2_ctrl_new_std(handler, &ov13850_ctrl_ops,
				V4L2_CID_VBLANK, vblank_def,
				OV13850_VTS_MAX - mode->height,
				1, vblank_def);

	exposure_max = mode->vts_def - 4;
	ov13850->exposure = v4l2_ctrl_new_std(handler, &ov13850_ctrl_ops,
				V4L2_CID_EXPOSURE, OV13850_EXPOSURE_MIN,
				exposure_max, OV13850_EXPOSURE_STEP,
				mode->exp_def);

	ov13850->anal_gain = v4l2_ctrl_new_std(handler, &ov13850_ctrl_ops,
				V4L2_CID_ANALOGUE_GAIN, OV13850_GAIN_MIN,
				OV13850_GAIN_MAX, OV13850_GAIN_STEP,
				OV13850_GAIN_DEFAULT);

	ov13850->test_pattern = v4l2_ctrl_new_std_menu_items(handler,
				&ov13850_ctrl_ops, V4L2_CID_TEST_PATTERN,
				ARRAY_SIZE(ov13850_test_pattern_menu) - 1,
				0, 0, ov13850_test_pattern_menu);

	ov13850->hflip = v4l2_ctrl_new_std(handler, &ov13850_ctrl_ops,
					     V4L2_CID_HFLIP, 0, 1, 1, 1);
	ov13850->vflip = v4l2_ctrl_new_std(handler, &ov13850_ctrl_ops,
					     V4L2_CID_VFLIP, 0, 1, 1, 0);

	if (handler->error) {
		ret = handler->error;
		dev_err(&ov13850->client->dev,
			"Failed to init controls(%d)\n", ret);
		goto err_free_handler;
	}

	ov13850->subdev.ctrl_handler = handler;

	return 0;

err_free_handler:
	v4l2_ctrl_handler_free(handler);

	return ret;
}

static int ov13850_check_sensor_id(struct ov13850 *ov13850,
				   struct i2c_client *client)
{
	struct device *dev = &ov13850->client->dev;
	u32 id = 0;
	int ret;

	ret = ov13850_read_reg(client, OV13850_REG_CHIP_ID,
			       OV13850_REG_VALUE_16BIT, &id);
	if (id != CHIP_ID) {
		dev_err(dev, "Unexpected sensor id(%06x), ret(%d)\n", id, ret);
		return -ENODEV;
	}

	ret = ov13850_read_reg(client, OV13850_CHIP_REVISION_REG,
			       OV13850_REG_VALUE_08BIT, &id);
	if (ret) {
		dev_err(dev, "Read chip revision register error\n");
		return ret;
	}

	if (id == OV13850_R2A)
		ov13850_global_regs = ov13850_global_regs_r2a;
	else
		ov13850_global_regs = ov13850_global_regs_r1a;
	dev_info(dev, "Detected OV%06x sensor, REVISION 0x%x\n", CHIP_ID, id);

	return 0;
}

static int ov13850_configure_regulators(struct ov13850 *ov13850)
{
	unsigned int i;

	for (i = 0; i < OV13850_NUM_SUPPLIES; i++)
		ov13850->supplies[i].supply = ov13850_supply_names[i];

	return devm_regulator_bulk_get(&ov13850->client->dev,
				       OV13850_NUM_SUPPLIES,
				       ov13850->supplies);
}

static int ov13850_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct ov13850 *ov13850;
	struct v4l2_subdev *sd;
	int ret;

	dev_info(dev, "driver version: %02x.%02x.%02x",
		DRIVER_VERSION >> 16,
		(DRIVER_VERSION & 0xff00) >> 8,
		DRIVER_VERSION & 0x00ff);

	ov13850 = devm_kzalloc(dev, sizeof(*ov13850), GFP_KERNEL);
	if (!ov13850)
		return -ENOMEM;

	ov13850->client = client;
	ov13850->cur_mode = &supported_modes[0];

	ov13850->xvclk = devm_clk_get(dev, "xvclk");
	if (IS_ERR(ov13850->xvclk)) {
		dev_err(dev, "Failed to get xvclk\n");
		return -EINVAL;
	}
	ret = clk_set_rate(ov13850->xvclk, OV13850_XVCLK_FREQ);
	if (ret < 0) {
		dev_err(dev, "Failed to set xvclk rate (24MHz)\n");
		return ret;
	}
	if (clk_get_rate(ov13850->xvclk) != OV13850_XVCLK_FREQ)
		dev_warn(dev, "xvclk mismatched, modes are based on 24MHz\n");

	ov13850->power_gpio = devm_gpiod_get(dev, "power", GPIOD_OUT_LOW);
	if (IS_ERR(ov13850->power_gpio))
		dev_warn(dev, "Failed to get power-gpios, maybe no use\n");

	ov13850->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ov13850->reset_gpio))
		dev_warn(dev, "Failed to get reset-gpios\n");

	ov13850->pwdn_gpio = devm_gpiod_get(dev, "pwdn", GPIOD_OUT_LOW);
	if (IS_ERR(ov13850->pwdn_gpio))
		dev_warn(dev, "Failed to get pwdn-gpios\n");

	ret = ov13850_configure_regulators(ov13850);
	if (ret) {
		dev_err(dev, "Failed to get power regulators\n");
		return ret;
	}

	ov13850->pinctrl = devm_pinctrl_get(dev);
	if (!IS_ERR(ov13850->pinctrl)) {
		ov13850->pins_default =
			pinctrl_lookup_state(ov13850->pinctrl,
					     OF_CAMERA_PINCTRL_STATE_DEFAULT);
		if (IS_ERR(ov13850->pins_default))
			dev_err(dev, "could not get default pinstate\n");

		ov13850->pins_sleep =
			pinctrl_lookup_state(ov13850->pinctrl,
					     OF_CAMERA_PINCTRL_STATE_SLEEP);
		if (IS_ERR(ov13850->pins_sleep))
			dev_err(dev, "could not get sleep pinstate\n");
	}

	mutex_init(&ov13850->mutex);

	sd = &ov13850->subdev;
	v4l2_i2c_subdev_init(sd, client, &ov13850_subdev_ops);
	ret = ov13850_initialize_controls(ov13850);
	if (ret)
		goto err_destroy_mutex;

	ret = __ov13850_power_on(ov13850);
	if (ret)
		goto err_free_handler;

	ret = ov13850_check_sensor_id(ov13850, client);
	if (ret)
		goto err_power_off;

	sd->internal_ops = &ov13850_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE |
		     V4L2_SUBDEV_FL_HAS_EVENTS;

	ov13850->pad.flags = MEDIA_PAD_FL_SOURCE;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	ret = media_entity_pads_init(&sd->entity, 1, &ov13850->pad);
	if (ret < 0)
		goto err_power_off;

	ret = v4l2_async_register_subdev_sensor(sd);
	if (ret) {
		dev_err(dev, "v4l2 async register subdev failed\n");
		goto err_clean_entity;
	}

	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);
	pm_runtime_idle(dev);

	return 0;

err_clean_entity:
	media_entity_cleanup(&sd->entity);
err_power_off:
	__ov13850_power_off(ov13850);
err_free_handler:
	v4l2_ctrl_handler_free(&ov13850->ctrl_handler);
err_destroy_mutex:
	mutex_destroy(&ov13850->mutex);

	return ret;
}

static int ov13850_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov13850 *ov13850 = to_ov13850(sd);

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);

	v4l2_ctrl_handler_free(&ov13850->ctrl_handler);
	mutex_destroy(&ov13850->mutex);

	pm_runtime_disable(&client->dev);
	if (!pm_runtime_status_suspended(&client->dev))
		__ov13850_power_off(ov13850);
	pm_runtime_set_suspended(&client->dev);

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id ov13850_of_match[] = {
	{ .compatible = "ovti,ov13850" },
	{},
};
MODULE_DEVICE_TABLE(of, ov13850_of_match);
#endif

static const struct i2c_device_id ov13850_match_id[] = {
	{ "ovti,ov13850", 0 },
	{ },
};

static struct i2c_driver ov13850_i2c_driver = {
	.driver = {
		.name = OV13850_NAME,
		.pm = &ov13850_pm_ops,
		.of_match_table = of_match_ptr(ov13850_of_match),
	},
	.probe		= &ov13850_probe,
	.remove		= &ov13850_remove,
	.id_table	= ov13850_match_id,
};

static int __init sensor_mod_init(void)
{
	return i2c_add_driver(&ov13850_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&ov13850_i2c_driver);
}

device_initcall_sync(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("OmniVision ov13850 sensor driver");
MODULE_LICENSE("GPL v2");

/*
 * A V4L2 driver for GC5004 cameras.
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>
#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("lwj");
MODULE_DESCRIPTION("A low-level driver for GC5004 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define VREF_POL          V4L2_MBUS_VSYNC_ACTIVE_LOW
#define HREF_POL          V4L2_MBUS_HSYNC_ACTIVE_HIGH
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_FALLING
#define V4L2_IDENT_SENSOR 0x5004

#define EXP_HIGH		0xff
#define EXP_MID			0x03
#define EXP_LOW			0x04
#define GAIN_HIGH		0xff
#define GAIN_LOW		0x24

#define ID_REG_HIGH		0xf0
#define ID_REG_LOW		0xf1
#define ID_VAL_HIGH		((V4L2_IDENT_SENSOR) >> 8)
#define ID_VAL_LOW		((V4L2_IDENT_SENSOR) & 0xff)

#define ANALOG_GAIN_1	64	/*1.00x*/
#define ANALOG_GAIN_2	90	/*1.41x*/
#define ANALOG_GAIN_3	128	/*2.00x*/
#define ANALOG_GAIN_4	178	/*2.78x*/
#define ANALOG_GAIN_5	247	/*3.85x*/

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 15

/*
 * The gc5004 sits on i2c with ID 0x6c
 */
#define I2C_ADDR 0x6c
#define SENSOR_NAME "gc5004"


static struct v4l2_subdev *glb_sd;

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;	/* coming later */

struct cfg_array {		/* coming later */
	struct regval_list *regs;
	int size;
};

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}

/*
 * The default register settings
 *
 */

static struct regval_list sensor_default_regs[] = {
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xfe, 0x80},
	{0xf2, 0x00},
	{0xf6, 0x00},
	{0xfc, 0x06},
	{0xf7, 0x9d},
	{0xf8, 0x83},
	{0xf9, 0xfe},
	{0xfa, 0x00},
	{0xfe, 0x00},

	{0x00, 0x00},
	{0x05, 0x01},
	{0x06, 0xfa},
	{0x07, 0x00},
	{0x08, 0x1c},
	{0x0a, 0x02},
	{0x0c, 0x00},
	{0x0d, 0x07},
	{0x0e, 0xa8},
	{0x0f, 0x0a},
	{0x10, 0x50},
	{0x17, 0x15},
	{0x18, 0x02},
	{0x19, 0x0c},
	{0x1a, 0x13},
	{0x1b, 0x48},
	{0x1c, 0x05},
	{0x1e, 0xb8},
	{0x1f, 0x78},
	{0x20, 0xc5},
	{0x21, 0x4f},
	{0x22, 0x82},
	{0x23, 0x41},
	{0x24, 0x2f},
	{0x2b, 0x01},
	{0x2c, 0x68},




	{0x86, 0x0a},
	{0x89, 0x03},
	{0x8a, 0x83},
	{0x8b, 0x61},
	{0x8c, 0x10},
	{0x8d, 0x01},
	{0x90, 0x01},
	{0x92, 0x00},
	{0x94, 0x0d},
	{0x95, 0x07},
	{0x96, 0x98},
	{0x97, 0x0a},
	{0x98, 0x20},




	{0xfe, 0x00},
	{0x80, 0x10},
	{0xfe, 0x02},
	{0x86, 0x00},
	{0x89, 0x15},
	{0xfe, 0x00},




	{0x40, 0x22},
	{0x41, 0x80},
	{0x50, 0x00},
	{0x51, 0x00},
	{0x52, 0x00},
	{0x53, 0x00},
	{0x54, 0x00},
	{0x55, 0x00},
	{0x56, 0x00},
	{0x57, 0x00},
	{0x58, 0x00},
	{0x59, 0x00},
	{0x5a, 0x00},
	{0x5b, 0x00},
	{0x5c, 0x00},
	{0x5d, 0x00},
	{0x5e, 0x00},
	{0x5f, 0x00},
	{0xd0, 0x00},
	{0xd1, 0x00},
	{0xd2, 0x00},
	{0xd3, 0x00},
	{0xd4, 0x00},
	{0xd5, 0x00},
	{0xd6, 0x00},
	{0xd7, 0x00},
	{0xd8, 0x00},
	{0xd9, 0x00},
	{0xda, 0x00},
	{0xdb, 0x00},
	{0xdc, 0x00},
	{0xdd, 0x00},
	{0xde, 0x00},
	{0xdf, 0x00},
	{0x70, 0x00},
	{0x71, 0x00},
	{0x72, 0x00},
	{0x73, 0x00},
	{0x74, 0x20},
	{0x75, 0x20},
	{0x76, 0x20},
	{0x77, 0x20},




	{0xb0, 0x40},
	{0xb1, 0x01},
	{0xb2, 0x02},
	{0xb3, 0x60},
	{0xb4, 0x45},
	{0xb5, 0x52},
	{0xb6, 0x00},




	{0xfe, 0x03},
	{0x01, 0x00},
	{0x02, 0x00},
	{0x03, 0x00},
	{0x06, 0x00},
	{0x10, 0x00},
	{0x15, 0x00},
	{0x18, 0x00},
	{0x19, 0x00},
	{0x1a, 0x00},




	{0xfe, 0x00},
	{0x18, 0x02},
	{0x80, 0x08},
	{0x87, 0x12},
	{0x95, 0x07},
	{0x96, 0x98},
	{0x97, 0x0a},
	{0x98, 0x20},
	{0xfe, 0x02},
	{0x86, 0x00},




	{0xfe, 0x03},
	{0x06, 0xc0},
	{0x10, 0x94},
	{0x15, 0x10},
	{0x12, 0x40},
	{0x13, 0x06},
	{0x04, 0x10},
	{0x05, 0x00},
	{0x42, 0x10},
	{0x43, 0x05},
	{0xfe, 0x00},
	{0x80, 0x09},




	{0xfe, 0x00},
	{0xf2, 0x0f},
};


static struct regval_list sensor_qsxga_regs[] = {

	{0x03, 0x01},
	{0x04, 0x90},
	{REG_DLY, 0x32},
	{0xf7, 0x9d},
	{0xf8, 0x83},
	{0x05, 0x01},
	{0x06, 0x0a},
	{0x09, 0x00},
	{0x0a, 0x02},
	{0x0b, 0x00},
	{0x0c, 0x00},
	{0x0d, 0x07},
	{0x0e, 0xa8},
	{0x0f, 0x0a},
	{0x10, 0x50},
	{0x18, 0x02},
	{0x80, 0x10},

	{0x89, 0x03},
	{0x8b, 0x61},
	{0x40, 0x82},

	{0x92, 0x02},
	{0x94, 0x05},

	{0x95, 0x07},
	{0x96, 0x98},
	{0x97, 0x0a},
	{0x98, 0x20},
};


static struct regval_list sensor_1080p_regs[] = {

	{0x03, 0x01},
	{0x04, 0x90},
	{REG_DLY, 0x32},

	{0xf7, 0x9d},
	{0xf8, 0x83},
	{0x05, 0x01},
	{0x06, 0x35},
	{0x09, 0x01},
	{0x0a, 0xb0},
	{0x0b, 0x01},
	{0x0c, 0x10},
	{0x0d, 0x04},
	{0x0e, 0x48},
	{0x0f, 0x07},
	{0x10, 0xd0},
	{0x18, 0x02},
	{0x80, 0x10},

	{0x89, 0x03},
	{0x8b, 0x61},
	{0x40, 0x22},

	{0x92, 0x02},
	{0x94, 0x05},

	{0x95, 0x04},
	{0x96, 0x38},
	{0x97, 0x07},
	{0x98, 0x80},
};

static struct regval_list sensor_720p_regs[] = {

	{0x03, 0x01},
	{0x04, 0x90},
	{REG_DLY, 0x32},
	{0xf7, 0x9d},
	{0xf8, 0x85},
	{0x05, 0x01},
	{0x06, 0xfa},
	{0x09, 0x00},
	{0x0a, 0x02},
	{0x0b, 0x00},
	{0x0c, 0x00},
	{0x0d, 0x07},
	{0x0e, 0xa8},
	{0x0f, 0x0a},
	{0x10, 0x50},
	{0x18, 0x42},
	{0x80, 0x19},

	{0x89, 0x03},
	{0x8b, 0x61},
	{0x40, 0x22},

	{0x92, 0x03},
	{0x94, 0x07},

	{0x95, 0x03},
	{0x96, 0xcc},
	{0x97, 0x05},
	{0x98, 0x10},
};

static struct regval_list sensor_fmt_raw[] = {
};

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, expmid, exphigh;
	struct sensor_info *info = to_state(sd);
	if (exp_val > 8191 * 16)
		exp_val = 8191 * 16;

	sensor_write(sd, 0xfe, 0x00);

	exp_val = exp_val / 64;
	exp_val = exp_val * 64;

#ifdef FRACTION_EXP
	exphigh = (unsigned char)((0x0f0000 & exp_val) >> 16);
	expmid = (unsigned char)((0x00ff00 & exp_val) >> 8);
	explow = (unsigned char)((0x0000ff & exp_val));
#else
	exphigh = 0;
	expmid = (unsigned char)((0x0ff000 & exp_val) >> 12);
	explow = (unsigned char)((0x000ff0 & exp_val) >> 4);
#endif

	sensor_write(sd, EXP_HIGH, exphigh);
	sensor_write(sd, EXP_MID, expmid);
	sensor_write(sd, EXP_LOW, explow);

	info->exp = exp_val;
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->gain;
	sensor_dbg("sensor_get_gain = %d\n", info->gain);
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	unsigned char tmp;
	struct sensor_info *info = to_state(sd);
	gain_val = gain_val * 6;

	sensor_write(sd, 0xfe, 0x00);
	sensor_write(sd, 0xb1, 0x01);
	sensor_write(sd, 0xb2, 0x00);

	if (gain_val < 0x40)
		gain_val = 0x40;
	else if ((ANALOG_GAIN_1 <= gain_val) && (gain_val < ANALOG_GAIN_2)) {

		sensor_write(sd, 0xb6, 0x00);
		tmp = gain_val;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_2 <= gain_val) && (gain_val < ANALOG_GAIN_3)) {

		sensor_write(sd, 0xb6, 0x01);
		tmp = 64 * gain_val / ANALOG_GAIN_2;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_3 <= gain_val) && (gain_val < ANALOG_GAIN_4)) {

		sensor_write(sd, 0xb6, 0x02);
		tmp = 64 * gain_val / ANALOG_GAIN_3;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if ((ANALOG_GAIN_4 <= gain_val) && (gain_val < ANALOG_GAIN_5)) {

		sensor_write(sd, 0xb6, 0x03);
		tmp = 64 * gain_val / ANALOG_GAIN_4;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	} else if (ANALOG_GAIN_5 <= gain_val) {

		sensor_write(sd, 0xb6, 0x04);
		tmp = 64 * gain_val / ANALOG_GAIN_5;
		sensor_write(sd, 0xb1, tmp >> 6);
		sensor_write(sd, 0xb2, (tmp << 2) & 0xfc);
	}
	info->gain = gain_val;
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	sensor_s_exp(sd, exp_val);
	sensor_s_gain(sd, gain_val);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret;
	ret = 0;
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		usleep_range(10000, 12000);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval;

	sensor_read(sd, ID_REG_HIGH, &rdval);
	if (rdval != ID_VAL_HIGH)
		return -ENODEV;

	sensor_read(sd, ID_REG_LOW, &rdval);
	if (rdval != ID_VAL_LOW)
		return -ENODEV;

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = QSXGA_WIDTH;
	info->height = QSXGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 15;	/* 30fps */

	info->preview_first_flag = 1;

	return 0;
}

static void sensor_cfg_req(struct v4l2_subdev *sd,
						struct sensor_config *cfg)
{
	struct sensor_info *info = to_state(sd);
	if (info == NULL) {
		sensor_err("sensor is not initialized.\n");
		return;
	}
	if (info->current_wins == NULL) {
		sensor_err("sensor format is not initialized.\n");
		return;
	}

	cfg->width = info->current_wins->width;
	cfg->height = info->current_wins->height;
	cfg->hoffset = info->current_wins->hoffset;
	cfg->voffset = info->current_wins->voffset;
	cfg->hts = info->current_wins->hts;
	cfg->vts = info->current_wins->vts;
	cfg->pclk = info->current_wins->pclk;
	cfg->bin_factor = info->current_wins->bin_factor;
	cfg->intg_min = info->current_wins->intg_min;
	cfg->intg_max = info->current_wins->intg_max;
	cfg->gain_min = info->current_wins->gain_min;
	cfg->gain_max = info->current_wins->gain_max;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg,
			       info->current_wins,
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case ISP_SET_EXP_GAIN:
		sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct {
	__u8 *desc;
	enum v4l2_mbus_pixelcode mbus_code;
	struct regval_list *regs;
	int regs_size;
	int bpp;		/* Bytes per pixel */
} sensor_formats[] = {
	{
		.desc = "Raw RGB Bayer",
		.mbus_code = V4L2_MBUS_FMT_SBGGR10_1X10,
		.regs = sensor_fmt_raw,
		.regs_size = ARRAY_SIZE(sensor_fmt_raw),
		.bpp = 1
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	/* qsxga: 2592*1936 */
	{
	 .width = QSXGA_WIDTH,
	 .height = QSXGA_HEIGHT,
	 .hoffset = 0,
	 .voffset = 4,
	 .hts = 3840,
	 .vts = 1988,
	 .pclk = 96 * 1000 * 1000,
	 .fps_fixed = 1,
	 .bin_factor = 1,
	 .intg_min = 1 << 4,
	 .intg_max = (1988) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 7 << 4,
	 .regs = sensor_qsxga_regs,
	 .regs_size = ARRAY_SIZE(sensor_qsxga_regs),
	 .set_size = NULL,
	 },
	/* 1080P */
	{
	 .width = HD1080_WIDTH,
	 .height = HD1080_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 3371,
	 .vts = 1140,
	 .pclk = 96 * 1000 * 1000,
	 .fps_fixed = 1,
	 .bin_factor = 1,
	 .intg_min = 1 << 4,
	 .intg_max = 1140 << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 6 << 4,
	 .regs = sensor_1080p_regs,
	 .regs_size = ARRAY_SIZE(sensor_1080p_regs),
	 .set_size = NULL,
	 },
	/* SXGA */
	{
	 .width = SXGA_WIDTH,
	 .height = SXGA_HEIGHT,
	 .hoffset = 8,
	 .voffset = 6,
	 .hts = 2400,
	 .vts = 1988,
	 .pclk = 144 * 1000 * 1000,
	 .fps_fixed = 1,
	 .bin_factor = 1,
	 .intg_min = 1 << 4,
	 .intg_max = 1988 << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 7 << 4,
	 .regs = sensor_720p_regs,
	 .regs_size = ARRAY_SIZE(sensor_720p_regs),
	 .set_size = NULL,
	 },
	/* 720p */
	{
	 .width = HD720_WIDTH,
	 .height = HD720_HEIGHT,
	 .hoffset = 8,
	 .voffset = 126,
	 .hts = 2400,
	 .vts = 1988,
	 .pclk = 144 * 1000 * 1000,
	 .fps_fixed = 1,
	 .bin_factor = 1,
	 .intg_min = 1 << 4,
	 .intg_max = 1988 << 4,
	 .gain_min = 1 << 4,
	 .gain_max = 7 << 4,
	 .regs = sensor_720p_regs,
	 .regs_size = ARRAY_SIZE(sensor_720p_regs),
	 .set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_reg_init(struct sensor_info *info)
{

	int ret = 0;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
			       ARRAY_SIZE(sensor_default_regs));
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	ret = 0;
	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	sensor_print("s_fmt set width = %d, height = %d\n", wsize->width,
		      wsize->height);

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

	if (!enable)
		return 0;
	return sensor_reg_init(info);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_PARALLEL;
	cfg->flags = V4L2_MBUS_MASTER | VREF_POL | HREF_POL | CLK_POL;

	return 0;
}

/*
 * Implement G/S_PARM.  There is a "high quality" mode we could try
 * to do someday; for now, we just do the frame rate tweak.
 */
static int sensor_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	memset(cp, 0, sizeof(struct v4l2_captureparm));
	cp->capability = V4L2_CAP_TIMEPERFRAME;
	cp->capturemode = info->capture_mode;

	return 0;
}

static int sensor_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *parms)
{
	struct v4l2_captureparm *cp = &parms->parm.capture;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_s_parm\n");

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (info->tpf.numerator == 0)
		return -EINVAL;

	info->capture_mode = cp->capturemode;

	return 0;
}

static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	/* Fill in min, max, step and default value for these controls. */
	/* see include/linux/videodev2.h for details */

	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 1 * 16, 32 * 16, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 65535 * 16, 1, 0);
	}
	return -EINVAL;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->value);
	}
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct v4l2_queryctrl qc;
	int ret;

	qc.id = ctrl->id;
	ret = sensor_queryctrl(sd, &qc);
	if (ret < 0) {
		return ret;
	}

	if (ctrl->value < qc.minimum || ctrl->value > qc.maximum) {
		sensor_err("max gain qurery is %d,min gain qurey is %d\n",
			    qc.maximum, qc.minimum);
		return -ERANGE;
	}

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->value);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->value);
	}
	return -EINVAL;
}

static int sensor_g_chip_ident(struct v4l2_subdev *sd,
			       struct v4l2_dbg_chip_ident *chip)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	return v4l2_chip_ident_i2c_client(client, chip, V4L2_IDENT_SENSOR, 0);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.g_chip_ident = sensor_g_chip_ident,
	.g_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.queryctrl = sensor_queryctrl,
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
	.g_mbus_config = sensor_g_mbus_config,
};

static void sensor_fill_mbus_fmt(struct v4l2_mbus_framefmt *mf,
				 const struct sensor_win_size *ws, u32 code)
{
	mf->width = ws->width;
	mf->height = ws->height;
	mf->code = code;
	mf->colorspace = V4L2_COLORSPACE_JPEG;
	mf->field = V4L2_FIELD_NONE;
}


SENSOR_ENUM_MBUS_CODE;
SENSOR_ENUM_FRAME_SIZE;
SENSOR_FIND_MBUS_CODE;
SENSOR_FIND_FRAME_SIZE;
SENSOR_TRY_FORMAT;
SENSOR_GET_FMT;
SENSOR_SET_FMT;

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_8,
	.data_width = CCI_BITS_8,
};

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	glb_sd = sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);

	info->fmt = &sensor_formats[0];
	info->af_first_flag = 1;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id sensor_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, sensor_id);

static struct i2c_driver sensor_driver = {
	.driver = {
		   .owner = THIS_MODULE,
		   .name = SENSOR_NAME,
		   },
	.probe = sensor_probe,
	.remove = sensor_remove,
	.id_table = sensor_id,
};
static __init int init_sensor(void)
{
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

module_init(init_sensor);
module_exit(exit_sensor);

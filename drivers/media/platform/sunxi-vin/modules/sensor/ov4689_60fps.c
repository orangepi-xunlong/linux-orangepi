/*
 * A V4L2 driver for ov4689 cameras.
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

MODULE_AUTHOR("Chomoly");
MODULE_DESCRIPTION("A low-level driver for ov4689 sensors");
MODULE_LICENSE("GPL");

#define MCLK              (24*1000*1000)
#define V4L2_IDENT_SENSOR 0x4689

/*
 * Our nominal (default) frame rate.
 */

#define SENSOR_FRAME_RATE 30

/*
 * The ov4689 sits on i2c with ID 0x42
 */
#define I2C_ADDR 0x42
#define  SENSOR_NAME "ov4689_60fps"

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
	{0x0103, 0x01},
	{REG_DLY, 0x05},
	{0x3638, 0x00},
	{0x0300, 0x00},
	{0x0302, 0x1c},
	{0x0304, 0x03},
	{0x030b, 0x00},
	{0x030d, 0x1e},
	{0x030e, 0x04},
	{0x030f, 0x01},
	{0x0312, 0x01},
	{0x031e, 0x00},
	{0x3000, 0x20},
	{0x3002, 0x00},
	{0x3018, 0x72},
	{0x3020, 0x93},
	{0x3021, 0x03},
	{0x3022, 0x01},
	{0x3031, 0x0a},
	{0x303f, 0x0c},
	{0x3305, 0xf1},
	{0x3307, 0x04},
	{0x3309, 0x29},
	{0x3500, 0x00},
	{0x3501, 0x60},
	{0x3502, 0x00},
	{0x3503, 0x04},
	{0x3504, 0x00},
	{0x3505, 0x00},
	{0x3506, 0x00},
	{0x3507, 0x00},
	{0x3508, 0x00},
	{0x3509, 0x80},
	{0x350a, 0x00},
	{0x350b, 0x00},
	{0x350c, 0x00},
	{0x350d, 0x00},
	{0x350e, 0x00},
	{0x350f, 0x80},
	{0x3510, 0x00},
	{0x3511, 0x00},
	{0x3512, 0x00},
	{0x3513, 0x00},
	{0x3514, 0x00},
	{0x3515, 0x80},
	{0x3516, 0x00},
	{0x3517, 0x00},
	{0x3518, 0x00},
	{0x3519, 0x00},
	{0x351a, 0x00},
	{0x351b, 0x80},
	{0x351c, 0x00},
	{0x351d, 0x00},
	{0x351e, 0x00},
	{0x351f, 0x00},
	{0x3520, 0x00},
	{0x3521, 0x80},
	{0x3522, 0x08},
	{0x3524, 0x08},
	{0x3526, 0x08},
	{0x3528, 0x08},
	{0x352a, 0x08},
	{0x3602, 0x00},
	{0x3604, 0x02},
	{0x3605, 0x00},
	{0x3606, 0x00},
	{0x3607, 0x00},
	{0x3609, 0x12},
	{0x360a, 0x40},
	{0x360c, 0x08},
	{0x360f, 0xe5},
	{0x3608, 0x8f},
	{0x3611, 0x00},
	{0x3613, 0xf7},
	{0x3616, 0x58},
	{0x3619, 0x99},
	{0x361b, 0x60},
	{0x361c, 0x7a},
	{0x361e, 0x79},
	{0x361f, 0x02},
	{0x3632, 0x00},
	{0x3633, 0x10},
	{0x3634, 0x10},
	{0x3635, 0x10},
	{0x3636, 0x15},
	{0x3646, 0x86},
	{0x364a, 0x0b},
	{0x3700, 0x17},
	{0x3701, 0x22},
	{0x3703, 0x10},
	{0x370a, 0x37},
	{0x3705, 0x00},
	{0x3706, 0x63},
	{0x3709, 0x3c},
	{0x370b, 0x01},
	{0x370c, 0x30},
	{0x3710, 0x24},
	{0x3711, 0x0c},
	{0x3716, 0x00},
	{0x3720, 0x28},
	{0x3729, 0x7b},
	{0x372a, 0x84},
	{0x372b, 0xbd},
	{0x372c, 0xbc},
	{0x372e, 0x52},
	{0x373c, 0x0e},
	{0x373e, 0x33},
	{0x3743, 0x10},
	{0x3744, 0x88},
	{0x3745, 0xc0},
	{0x374a, 0x43},
	{0x374c, 0x00},
	{0x374e, 0x23},
	{0x3751, 0x7b},
	{0x3752, 0x84},
	{0x3753, 0xbd},
	{0x3754, 0xbc},
	{0x3756, 0x52},
	{0x375c, 0x00},
	{0x3760, 0x00},
	{0x3761, 0x00},
	{0x3762, 0x00},
	{0x3763, 0x00},
	{0x3764, 0x00},
	{0x3767, 0x04},
	{0x3768, 0x04},
	{0x3769, 0x08},
	{0x376a, 0x08},
	{0x376b, 0x20},
	{0x376c, 0x00},
	{0x376d, 0x00},
	{0x376e, 0x00},
	{0x3773, 0x00},
	{0x3774, 0x51},
	{0x3776, 0xbd},
	{0x3777, 0xbd},
	{0x3781, 0x18},
	{0x3783, 0x25},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3802, 0x00},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x97},
	{0x3806, 0x05},
	{0x3807, 0xfb},
	{0x3808, 0x0a},
	{0x3809, 0x80},
	{0x380a, 0x05},
	{0x380b, 0xf0},
	{0x380c, 0x05},
	{0x380d, 0x08},
	{0x380e, 0x06},
	{0x380f, 0x12},
	{0x3810, 0x00},
	{0x3811, 0x08},
	{0x3812, 0x00},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3815, 0x01},
	{0x3819, 0x01},
	{0x3820, 0x00},
	{0x3821, 0x06},
	{0x3829, 0x00},
	{0x382a, 0x01},
	{0x382b, 0x01},
	{0x382d, 0x7f},
	{0x3830, 0x04},
	{0x3836, 0x01},
	{0x3837, 0x00},
	{0x3841, 0x02},
	{0x3846, 0x08},
	{0x3847, 0x07},
	{0x3d85, 0x36},
	{0x3d8c, 0x71},
	{0x3d8d, 0xcb},
	{0x3f0a, 0x00},
	{0x4000, 0x71},
	{0x4001, 0x40},
	{0x4002, 0x04},
	{0x4003, 0x14},
	{0x400e, 0x00},
	{0x4011, 0x00},
	{0x401a, 0x00},
	{0x401b, 0x00},
	{0x401c, 0x00},
	{0x401d, 0x00},
	{0x401f, 0x00},
	{0x4020, 0x00},
	{0x4021, 0x10},
	{0x4022, 0x07},
	{0x4023, 0xcf},
	{0x4024, 0x09},
	{0x4025, 0x60},
	{0x4026, 0x09},
	{0x4027, 0x6f},
	{0x4028, 0x00},
	{0x4029, 0x02},
	{0x402a, 0x06},
	{0x402b, 0x04},
	{0x402c, 0x02},
	{0x402d, 0x02},
	{0x402e, 0x0e},
	{0x402f, 0x04},
	{0x4302, 0xff},
	{0x4303, 0xff},
	{0x4304, 0x00},
	{0x4305, 0x00},
	{0x4306, 0x00},
	{0x4308, 0x02},
	{0x4500, 0x6c},
	{0x4501, 0xc4},
	{0x4502, 0x40},
	{0x4503, 0x02},
	{0x4601, 0x04},
	{0x4800, 0x04},
	{0x4813, 0x08},
	{0x481f, 0x40},
	{0x4829, 0x78},
	{0x4837, 0x18},
	{0x4b00, 0x2a},
	{0x4b0d, 0x00},
	{0x4d00, 0x04},
	{0x4d01, 0x42},
	{0x4d02, 0xd1},
	{0x4d03, 0x93},
	{0x4d04, 0xf5},
	{0x4d05, 0xc1},
	{0x5000, 0xf3},
	{0x5001, 0x11},
	{0x5004, 0x00},
	{0x500a, 0x00},
	{0x500b, 0x00},
	{0x5032, 0x00},
	{0x5040, 0x00},
	{0x5050, 0x0c},
	{0x5500, 0x00},
	{0x5501, 0x10},
	{0x5502, 0x01},
	{0x5503, 0x0f},
	{0x8000, 0x00},
	{0x8001, 0x00},
	{0x8002, 0x00},
	{0x8003, 0x00},
	{0x8004, 0x00},
	{0x8005, 0x00},
	{0x8006, 0x00},
	{0x8007, 0x00},
	{0x8008, 0x00},
	{0x3638, 0x00},
	{0x3105, 0x31},
	{0x301a, 0xf9},
	{0x3508, 0x07},
	{0x484b, 0x05},
	{0x4805, 0x03},
	{0x3601, 0x01},
	{0x0100, 0x01},
	{REG_DLY, 0x02},
	{0x3105, 0x11},
	{0x301a, 0xf1},
	{0x4805, 0x00},
	{0x301a, 0xf0},
	{0x3208, 0x00},
	{0x302a, 0x00},
	{0x302a, 0x00},
	{0x302a, 0x00},
	{0x302a, 0x00},
	{0x302a, 0x00},
	{0x3601, 0x00},
	{0x3638, 0x00},
	{0x3208, 0x10},
	{0x3208, 0xa0},


	{0x500c, 0x08},
	{0x500d, 0x88},
	{0x500e, 0x04},
	{0x500f, 0x00},
	{0x5010, 0x06},
	{0x5011, 0x3d},
	{0x0100, 0x01},

};


static struct regval_list sensor_quxga_25fps_regs[] = {

	{0x0100, 0x00},
	{REG_DLY, 0x32},
	{0x3501, 0x60},
	{0x3632, 0x00},
	{0x376b, 0x20},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x97},
	{0x3807, 0xfb},
	{0x3808, 0x0a},
	{0x3809, 0x80},
	{0x380a, 0x05},
	{0x380b, 0xf0},
	{0x380d, 0x08},
	{0x380e, 0x0e},
	{0x380f, 0x8f},
	{0x3811, 0x08},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3820, 0x06},
	{0x3821, 0x00},
	{0x382a, 0x01},
	{0x3830, 0x04},
	{0x3836, 0x01},
	{0x4001, 0x40},
	{0x4022, 0x07},
	{0x4023, 0xcf},
	{0x4024, 0x09},
	{0x4025, 0x60},
	{0x4026, 0x09},
	{0x4027, 0x6f},
	{0x4502, 0x40},
	{0x4601, 0x04},
	{0x0100, 0x01},
};

static struct regval_list sensor_quxga_60fps_regs[] = {

	{0x0100, 0x00},
	{REG_DLY, 0x32},
	{0x3501, 0x60},
	{0x3632, 0x00},
	{0x376b, 0x20},
	{0x3800, 0x00},
	{0x3801, 0x08},
	{0x3803, 0x04},
	{0x3804, 0x0a},
	{0x3805, 0x97},
	{0x3807, 0xfb},
	{0x3808, 0x0a},
	{0x3809, 0x80},
	{0x380a, 0x05},
	{0x380b, 0xf0},
	{0x380d, 0x08},
	{0x380e, 0x06},
	{0x380f, 0x12},
	{0x3811, 0x08},
	{0x3813, 0x04},
	{0x3814, 0x01},
	{0x3820, 0x06},
	{0x3821, 0x00},
	{0x382a, 0x01},
	{0x3830, 0x04},
	{0x3836, 0x01},
	{0x4001, 0x40},
	{0x4022, 0x07},
	{0x4023, 0xcf},
	{0x4024, 0x09},
	{0x4025, 0x60},
	{0x4026, 0x09},
	{0x4027, 0x6f},
	{0x4502, 0x40},
	{0x4601, 0x04},
	{0x0100, 0x01},

};

/*
 * Here we'll try to encapsulate the changes for just the output
 * video format.
 *
 */

static struct regval_list sensor_fmt_raw[] = {

};

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	struct sensor_info *info = to_state(sd);

	*value = info->exp;
	sensor_dbg("sensor_get_exposure = %d\n", info->exp);
	return 0;
}

int ov4689_sensor_vts;
static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	int exp_val, gain_val, frame_length, shutter;
	unsigned char explow = 0, expmid = 0, exphigh = 0;
	unsigned char gainlow = 0, gainhigh = 0;
	struct sensor_info *info = to_state(sd);
	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;



	if (gain_val > 16 * 16 - 1)
		gain_val = 16 * 16 - 1;

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;


	gain_val *= 8;
	gain_val = gain_val & 0x7ff;

	if (gain_val < 2 * 16 * 8) {
		gainhigh = 0;
		gainlow = gain_val;
	} else if (2 * 16 * 8 <= gain_val && gain_val < 4 * 16 * 8) {
		gainhigh = 1;
		gainlow = gain_val / 2 - 8;
	} else if (4 * 16 * 8 <= gain_val && gain_val < 8 * 16 * 8) {
		gainhigh = 3;
		gainlow = gain_val / 4 - 12;
	} else {
		gainhigh = 7;
		gainlow = gain_val / 8 - 8;
	}

	exp_val >>= 4;

	exphigh = (unsigned char)((0xffff & exp_val) >> 12);
	expmid = (unsigned char)((0xfff & exp_val) >> 4);
	explow = (unsigned char)((0xf & exp_val) << 4);
	shutter = exp_val;
	if (shutter > ov4689_sensor_vts - 4)
		frame_length = shutter + 4;
	else
		frame_length = ov4689_sensor_vts;

	sensor_write(sd, 0x380f, (frame_length & 0xff));
	sensor_write(sd, 0x380e, (frame_length >> 8));

	sensor_write(sd, 0x3509, 0x0f);
	sensor_write(sd, 0x3508, 0x07);

	sensor_write(sd, 0x3502, 0x00);
	sensor_write(sd, 0x3501, 0xa0);
	sensor_write(sd, 0x3500, 0x01);

	info->exp = exp_val;
	info->gain = gain_val;
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	unsigned char explow, expmid, exphigh;
	unsigned int tmp;
	data_type tmp1, tmp2, tmp3;
	struct sensor_info *info = to_state(sd);

	if (exp_val > 0xfffff)
		exp_val = 0xfffff;

	exp_val >>= 4;
	exphigh = (unsigned char)((0xffff & exp_val) >> 12);
	expmid = (unsigned char)((0xfff & exp_val) >> 4);
	explow = (unsigned char)((0x0f & exp_val) << 4);

	sensor_write(sd, 0x3208, 0x00);

	sensor_write(sd, 0x3502, 0x00);
	sensor_write(sd, 0x3501, 0xa0);
	sensor_write(sd, 0x3500, 0x01);

	sensor_write(sd, 0x3208, 0x10);
	sensor_write(sd, 0x3208, 0xe0);

	sensor_read(sd, 0x3502, &tmp1);
	sensor_read(sd, 0x3501, &tmp2);
	sensor_read(sd, 0x3500, &tmp3);
	tmp = (tmp1 >> 4) + (tmp2 << 4) + (tmp3 << 12);



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
	struct sensor_info *info = to_state(sd);
	unsigned char gainlow = 0;
	unsigned char gainhigh = 0;
	unsigned int tmp;
	data_type tmp1, tmp2;

	if (gain_val < 1 * 16)
		gain_val = 16;
	if (gain_val > 64 * 16 - 1)
		gain_val = 64 * 16 - 1;

	gain_val *= 8;
	gain_val = gain_val & 0x7ff;

	if (gain_val < 2 * 16 * 8) {
		gainhigh = 0;
		gainlow = gain_val;
	} else if (2 * 16 * 8 <= gain_val && gain_val < 4 * 16 * 8) {
		gainhigh = 1;
		gainlow = gain_val / 2 - 8;
	} else if (4 * 16 * 8 <= gain_val && gain_val < 8 * 16 * 8) {
		gainhigh = 3;
		gainlow = gain_val / 4 - 12;
	} else {
		gainhigh = 7;
		gainlow = gain_val / 8 - 8;
	}

	sensor_write(sd, 0x3208, 0x11);
	sensor_write(sd, 0x3509, 0x0f);
	sensor_write(sd, 0x3508, 0x07);

	sensor_write(sd, 0x3208, 0x11);
	sensor_write(sd, 0x3208, 0xe1);
	sensor_read(sd, 0x3509, &tmp1);
	sensor_read(sd, 0x3508, &tmp2);
	if (tmp2 == 7)
		tmp = (tmp1 + 8) * 8;
	else if (tmp2 == 3)
		tmp = (tmp1 + 12) * 4;
	else if (tmp2 == 1)
		tmp = (tmp1 + 8) * 2;
	else if (tmp2 == 0)
		tmp = tmp1;


	info->gain = gain_val;
	return 0;
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret;
	data_type rdval;

	ret = sensor_read(sd, 0x0100, &rdval);
	if (ret != 0)
		return ret;

	if (on_off == CSI_GPIO_LOW) {
		ret = sensor_write(sd, 0x0100, rdval & 0xfe);
	} else {
		ret = sensor_write(sd, 0x0100, rdval | 0x01);
	}
	return ret;
}

/*
 * Stuff that knows about the sensor.
 */

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(10000, 12000);
		cci_lock(sd);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		cci_unlock(sd);
		vin_set_mclk(sd, OFF);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		ret = sensor_s_sw_stby(sd, CSI_GPIO_HIGH);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, 1);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, POWER_EN, 1);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, IOVDD, ON);
		usleep_range(10000, 12000);

		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(5000, 6000);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AFVDD, ON);
		usleep_range(5000, 6000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(10000, 12000);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		usleep_range(10000, 12000);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vin_set_pmu_channel(sd, AFVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		usleep_range(5000, 6000);
		vin_set_pmu_channel(sd, AVDD, OFF);

		usleep_range(5000, 6000);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vin_gpio_set_status(sd, RESET, 0);
		vin_gpio_set_status(sd, PWDN, 0);
		vin_gpio_set_status(sd, POWER_EN, 0);
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

	sensor_read(sd, 0x300a, &rdval);
	if (rdval != 0x46)
		return -ENODEV;

	sensor_read(sd, 0x300b, &rdval);
	if (rdval != 0x88)
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
	info->width = 2688;
	info->height = 1520;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

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
		ret = sensor_s_exp_gain(sd, (struct sensor_exp_gain *)arg);
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
	/* 2688*1520 */
	{
	 .width = 2688,
	 .height = 1520,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1288,
	 .vts = 3727,
	 .pclk = 120 * 1000 * 1000,
	 .mipi_bps = 672 * 1000 * 1000,
	 .fps_fixed = 2,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (3727 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = (12 << 4) - 1,
	 .regs = sensor_quxga_25fps_regs,
	 .regs_size = ARRAY_SIZE(sensor_quxga_25fps_regs),
	 .set_size = NULL,
	 },
	{
	 .width = 2560,
	 .height = 1440,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1288,
	 .vts = 3727,
	 .pclk = 120 * 1000 * 1000,
	 .mipi_bps = 672 * 1000 * 1000,
	 .fps_fixed = 2,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (3727 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = (12 << 4) - 1,
	 .width_input = 2688,
	 .height_input = 1520,
	 .regs = sensor_quxga_25fps_regs,
	 .regs_size = ARRAY_SIZE(sensor_quxga_25fps_regs),
	 .set_size = NULL,
	 },
#if 1
	{
	 .width = 1920,
	 .height = 1088,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1288,
	 .vts = 1554,
	 .pclk = 120 * 1000 * 1000,
	 .mipi_bps = 672 * 1000 * 1000,
	 .fps_fixed = 2,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (1554 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = (12 << 4) - 1,
	 .width_input = 2688,
	 .height_input = 1520,
	 .regs = sensor_quxga_60fps_regs,
	 .regs_size = ARRAY_SIZE(sensor_quxga_60fps_regs),
	 .set_size = NULL,
	 },
#endif
	{
	 .width = HD1080_WIDTH,
	 .height = HD1080_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1288,
	 .vts = 1554,
	 .pclk = 120 * 1000 * 1000,
	 .mipi_bps = 672 * 1000 * 1000,
	 .fps_fixed = 2,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (1554 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = (12 << 4) - 1,
	 .width_input = 2688,
	 .height_input = 1520,
	 .regs = sensor_quxga_60fps_regs,
	 .regs_size = ARRAY_SIZE(sensor_quxga_60fps_regs),
	 .set_size = NULL,
	 },
	{
	 .width = HD720_WIDTH,
	 .height = HD720_HEIGHT,
	 .hoffset = 0,
	 .voffset = 0,
	 .hts = 1288,
	 .vts = 1554,
	 .pclk = 120 * 1000 * 1000,
	 .mipi_bps = 672 * 1000 * 1000,
	 .fps_fixed = 2,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (1554 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = (12 << 4) - 1,
	 .width_input = 2688,
	 .height_input = 1520,
	 .regs = sensor_quxga_60fps_regs,
	 .regs_size = ARRAY_SIZE(sensor_quxga_60fps_regs),
	 .set_size = NULL,
	 },
	{
	 .width = VGA_WIDTH,
	 .height = VGA_HEIGHT,
	 .hoffset = 320,
	 .voffset = 0,
	 .hts = 1288,
	 .vts = 1554,
	 .pclk = 120 * 1000 * 1000,
	 .mipi_bps = 672 * 1000 * 1000,
	 .fps_fixed = 2,
	 .bin_factor = 1,
	 .intg_min = 16,
	 .intg_max = (1554 - 4) << 4,
	 .gain_min = 1 << 4,
	 .gain_max = (12 << 4) - 1,
	 .width_input = 2048,
	 .height_input = 1520,
	 .regs = sensor_quxga_60fps_regs,
	 .regs_size = ARRAY_SIZE(sensor_quxga_60fps_regs),
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

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	ov4689_sensor_vts = wsize->vts;

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
	cfg->type = V4L2_MBUS_CSI2;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

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
		return v4l2_ctrl_query_fill(qc, 1 * 16, 128 * 16 - 1, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 65536 * 16, 1, 0);
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
	.addr_width = CCI_BITS_16,
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

/*
 * A V4L2 driver for TW2866 cameras.
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

MODULE_AUTHOR("zw");
MODULE_DESCRIPTION("A low-level driver for TW2866 sensors");
MODULE_LICENSE("GPL");

#define DEV_DBG_EN      1
#if (DEV_DBG_EN == 1)
#define vfe_dev_dbg(x, arg...) printk("[TW2866]"x, ##arg)
#else
#define vfe_dev_dbg(x, arg...)
#endif
#define vfe_dev_err(x, arg...) printk("[TW2866]"x, ##arg)
#define vfe_dev_print(x, arg...) printk("[TW2866]"x, ##arg)

#define LOG_ERR_RET(x) { \
	int ret;  \
	ret = x; \
	if (ret < 0) {\
		vfe_dev_err("error at %s\n", __func__);  \
		return ret; \
	} \
}

#define MCLK              (24*1000*1000)
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING
#define V4L2_IDENT_SENSOR 0x00c8

#define CSI_CH_0	(1 << 10)
#define CSI_CH_1	(1 << 11)
#define CSI_CH_2	(1 << 12)
#define CSI_CH_3	(1 << 13)

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The TW2866 sits on i2c with ID 0x50
 */
#define I2C_ADDR 0x50
#define SENSOR_NAME "tw2866"
static struct v4l2_subdev *glb_sd;

/*
 * Information we maintain about a known sensor.
 */
struct sensor_format_struct;	/* coming later */

static inline struct sensor_info *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct sensor_info, sd);
}
#if 0
static struct regval_list read_reg[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x00},
	{0x03, 0x00},
	{0xFA, 0x00},		/*output clock, base of pclk, 1ch*d1 = 27m */
	{0xFB, 0x00},
	{0xFC, 0x00},
	{0x9C, 0x00},		/*A0 */
	{0x9E, 0x00},
	{0xF9, 0x00},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0x6A, 0x00},		/*CLKPO2/CLKNO2 off */
	{0x6B, 0x00},		/*CLKPO3/CLKNO3 off */
	{0x6C, 0x00},		/*CLKPO4/CLKNO4 off */
	{0x60, 0x00},		/*0x15/0x05¡£ */
	{0x61, 0x00},
	{0xca, 0x00},		/*chmd: 0=1ch 1=2ch 2=4ch */
	{0xcd, 0x00},		/*1st */
	{0x42, 0x00},		/*testpattern 75%color bar */
};

static struct regval_list reg_d1_1ch[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x64},
	{0x03, 0x11},
	{0xFA, 0x40},		/*output clock, base of pclk, 1ch*d1 = 27m */
	{0xFB, 0x2F},
	{0xFC, 0xFF},
	{0x9C, 0x20},		/*A0 */
	{0x9E, 0x52},
	{0xF9, 0x11},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0x6A, 0x0f},		/*CLKPO2/CLKNO2 off */
	{0x6B, 0x0f},		/*CLKPO3/CLKNO3 off */
	{0x6C, 0x0f},		/*CLKPO4/CLKNO4 off */
	{0x60, 0x15},		/*0x15/0x05*/
	{0x61, 0x03},
	{0xca, 0x00},		/*chmd: 0=1ch 1=2ch 2=4ch */
	{0xcd, 0xe4},		/*1st */
	{0x5b, 0x11},		/*pad drive set */
};

static struct regval_list reg_d1_2ch[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x64},
	{0x03, 0x11},
	{0xFA, 0x45},		/*0x45 *//*[7]:v-scale output clock, base of pclk, 2ch*d1 = 54m */
	{0xFB, 0x2F},
	{0xFC, 0xFF},
	{0x9C, 0x20},		/*A0 */
	{0x9E, 0x52},
	{0xF9, 0x11},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0xca, 0x01},		/*chmd: 0=1ch 1=2ch 2=4ch */
	{0xcd, 0xe4},		/*1st */
	{0xcc, 0x39},		/*2nd */
	{0xcb, 0x00},		/*4ch cif */
	{0x60, 0x15},		/*0x15/0x05*/
	{0x61, 0x03},
	{0x5b, 0x00},		/*pad drive set */
};
#endif

static struct regval_list reg_d1_4ch[] = {
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x64},
	{0x03, 0x11},
	{0xFA, 0x4a},		/*output clock, base of pclk, 4ch*cif = 54m  4ch*d1 = 108m */
	{0xFB, 0x2F},
	{0xFC, 0xFF},
	{0x9C, 0x20},		/*A0 */
	{0x9E, 0x52},
	{0xF9, 0x11},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0xca, 0x02},		/*chmd: 0=1ch 1=2ch 2=4ch */
	{0xcd, 0xe4},		/*1st */
	{0xcc, 0x39},		/*2nd */
	{0xcb, 0x00},		/*4ch cif */
	{0x60, 0x15},		/*0x15/0x05*/
	{0x61, 0x03},
	{0x5b, 0xff},		/*pad drive set */
	{REG_DLY, 0x20},
};
#if 0
static struct regval_list reg_cif_4ch[] = {
	/*CSI_MODE==CSI_MODE_TW2866_4CH_CIF */
	{0x00, 0x00},
	{0x01, 0x00},
	{0x02, 0x64},
	{0x03, 0x11},
	{0xFA, 0x45},		/*output clock, base of pclk, 4ch*cif = 54m  4ch*cif = 54m */
	{0xFB, 0x2F},
	{0xFC, 0xFF},
	{0x9C, 0x20},		/*A0 */
	{0x9E, 0x52},
	{0xF9, 0x11},		/*Video misc */
	{0xAA, 0x00},		/*Video AGC */
	{0xca, 0x00},		/*chmd: 0=1ch or 4ch-cif 1=2ch 2=4ch-d1 */
	{0xcd, 0xe4},		/*1st */
	{0xcc, 0x39},		/*2nd */
	{0xcb, 0x01},		/*4ch cif */
	{0x60, 0x17},		/*0x37/0x15/0x05*/
	{0x61, 0x03},
	{0x5b, 0x11},		/*pad drive set */
	{0x9f, 0x00},		/*p clock delay 7ns */
};
#endif

/*
 * Code for dealing with controls.
 * fill with different sensor module
 * different sensor module has different settings here
 * if not support the follow function ,retrun -EINVAL
 */

/* *********************************************begin of ******************************************** */

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	if (on_off)
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
	else
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	switch (on) {
	case CSI_SUBDEV_STBY_ON:
		vfe_dev_dbg("CSI_SUBDEV_STBY_ON!\n");
		sensor_s_sw_stby(sd, ON);
		break;
	case CSI_SUBDEV_STBY_OFF:
		vfe_dev_dbg("CSI_SUBDEV_STBY_OFF!\n");
		sensor_s_sw_stby(sd, OFF);
		break;
	case CSI_SUBDEV_PWR_ON:
		vfe_dev_dbg("CSI_SUBDEV_PWR_ON!\n");
		cci_lock(sd);
		vfe_gpio_set_status(sd, PWDN, 1);
		vfe_gpio_set_status(sd, RESET, 1);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vfe_set_mclk_freq(sd, MCLK);
		vfe_set_mclk(sd, ON);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		vfe_set_pmu_channel(sd, IOVDD, ON);
		vfe_set_pmu_channel(sd, AVDD, ON);
		vfe_set_pmu_channel(sd, DVDD, ON);
		vfe_set_pmu_channel(sd, AFVDD, ON);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_LOW);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(30000, 31000);
		cci_unlock(sd);
		break;
	case CSI_SUBDEV_PWR_OFF:
		vfe_dev_dbg("CSI_SUBDEV_PWR_OFF!\n");
		cci_lock(sd);
		vfe_set_mclk(sd, OFF);
		vfe_gpio_write(sd, POWER_EN, CSI_GPIO_LOW);
		vfe_set_pmu_channel(sd, AFVDD, OFF);
		vfe_set_pmu_channel(sd, DVDD, OFF);
		vfe_set_pmu_channel(sd, AVDD, OFF);
		vfe_set_pmu_channel(sd, IOVDD, OFF);
		usleep_range(10000, 12000);
		vfe_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vfe_gpio_set_status(sd, RESET, 0);
		vfe_gpio_set_status(sd, PWDN, 0);
		cci_unlock(sd);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	vfe_gpio_write(sd, RESET, CSI_GPIO_LOW);
	usleep_range(5000, 6000);
	vfe_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	usleep_range(5000, 6000);
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval;

	rdval = 0;
	LOG_ERR_RET(sensor_read(sd, 0xff, &rdval))
	    vfe_dev_print("reg 0xff rdval = 0x%x\n", rdval);

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);
	vfe_dev_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		vfe_dev_err("chip found is not an target chip.\n");
		return ret;
	}

	vfe_get_standby_mode(sd, &info->stby_mode);

	if ((info->stby_mode == HW_STBY || info->stby_mode == SW_STBY)
	    && info->init_first_flag == 0) {
		vfe_dev_print("stby_mode and init_first_flag = 0\n");
		return 0;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = VGA_WIDTH;
	info->height = VGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 30;	/* 30fps */

	if (info->stby_mode == 0)
		info->init_first_flag = 0;
	info->preview_first_flag = 1;
	return 0;
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
			vfe_dev_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case ISP_SET_EXP_GAIN:
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
	.desc = "BT656 4CH",
	.mbus_code = V4L2_MBUS_FMT_UYVY8_2X8,
	.regs = NULL,
	.regs_size = 0,
	.bpp = 2,
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */

static struct sensor_win_size sensor_win_sizes[] = {
	/* 480p */
	{
	 .width = 704,
	 .height = 576,
	 .hoffset = 0,
	 .voffset = 0,
	 .regs = reg_d1_4ch,
	 .regs_size = ARRAY_SIZE(reg_d1_4ch),
	 .set_size = NULL,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_enum_fmt(struct v4l2_subdev *sd, unsigned index,
			   enum v4l2_mbus_pixelcode *code)
{
	if (index >= N_FMTS)
		return -EINVAL;

	*code = sensor_formats[index].mbus_code;
	return 0;
}

static int sensor_enum_size(struct v4l2_subdev *sd,
			    struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index > N_WIN_SIZES - 1)
		return -EINVAL;

	fsize->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	fsize->discrete.width = sensor_win_sizes[fsize->index].width;
	fsize->discrete.height = sensor_win_sizes[fsize->index].height;

	return 0;
}

static int sensor_try_fmt_internal(struct v4l2_subdev *sd,
				   struct v4l2_mbus_framefmt *fmt,
				   struct sensor_format_struct **ret_fmt,
				   struct sensor_win_size **ret_wsize)
{
	int index;
	struct sensor_win_size *wsize, *wsize_last_ok = NULL;
	struct sensor_info *info = to_state(sd);

	for (index = 0; index < N_FMTS; index++)
		if (sensor_formats[index].mbus_code == fmt->code)
			break;

	if (index >= N_FMTS)
		return -EINVAL;

	if (ret_fmt != NULL)
		*ret_fmt = sensor_formats + index;

	/*
	 * Fields: the sensor devices claim to be progressive.
	 */

	fmt->field = V4L2_FIELD_INTERLACED;

	for (wsize = sensor_win_sizes; wsize < sensor_win_sizes + N_WIN_SIZES;
	     wsize++) {
		if (fmt->width >= wsize->width && fmt->height >= wsize->height) {
			wsize_last_ok = wsize;
			break;
		}
		wsize_last_ok = wsize;
	}

	if (wsize >= sensor_win_sizes + N_WIN_SIZES) {
		if (NULL != wsize_last_ok) {
			wsize = wsize_last_ok;
		} else {
			wsize--;	/* Take the smallest one */
		}
	}
	if (ret_wsize != NULL)
		*ret_wsize = wsize;
	info->current_wins = wsize;

	/*
	 * Note the size we'll actually handle.
	 */
	fmt->width = wsize->width;
	fmt->height = wsize->height;
	return 0;
}

static int sensor_try_fmt(struct v4l2_subdev *sd,
			  struct v4l2_mbus_framefmt *fmt)
{
	return sensor_try_fmt_internal(sd, fmt, NULL, NULL);
}

static int sensor_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_BT656;
	cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;

	return 0;
}

/*
 * Set a format.
 */
static int sensor_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
	int ret;

	struct sensor_format_struct *sensor_fmt;
	struct sensor_win_size *wsize;
	struct sensor_info *info = to_state(sd);

	vfe_dev_dbg("sensor_s_fmt\n");

	ret = sensor_try_fmt_internal(sd, fmt, &sensor_fmt, &wsize);
	if (ret)
		return ret;

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		LOG_ERR_RET(sensor_write_array
			    (sd, wsize->regs, wsize->regs_size))
		    if (wsize->set_size)
			ret = wsize->set_size(sd);

	if (ret < 0) {
		vfe_dev_err("write default_reg_init error\n");
		return ret;
	}

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	vfe_dev_print("s_fmt = %x, width = %d, height = %d\n",
		      sensor_fmt->mbus_code, wsize->width, wsize->height);

	vfe_dev_print("s_fmt end\n");
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

	vfe_dev_dbg("sensor_s_parm\n");

	if (parms->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	if (info->tpf.numerator == 0)
		return -EINVAL;

	info->capture_mode = cp->capturemode;
	return 0;
}

static int sensor_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	switch (qc->id) {
	case V4L2_CID_GAIN:
		return v4l2_ctrl_query_fill(qc, 0, 10000 * 10000, 1, 16);
	case V4L2_CID_EXPOSURE:
		return v4l2_ctrl_query_fill(qc, 0, 10000 * 10000, 1, 16);
	}
	return 0;
}

static int sensor_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
}

static int sensor_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	return 0;
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
	.enum_mbus_fmt = sensor_enum_fmt,
	.enum_framesizes = sensor_enum_size,
	.try_mbus_fmt = sensor_try_fmt,
	.s_mbus_fmt = sensor_s_fmt,
	.s_parm = sensor_s_parm,
	.g_parm = sensor_g_parm,
	.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
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
	info->init_first_flag = 1;
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

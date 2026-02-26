
/*
 * A V4L2 driver for TD100 YUV cameras.
 *
 * Copyright (c) 2020 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Lihuiyu<lihuiyu@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/io.h>

#include "../camera.h"
#include "../sensor_helper.h"
#include "td104_drv.h"
#include "../../../utility/vin_log.h"

MODULE_AUTHOR("lhy");
MODULE_DESCRIPTION("A low-level driver for TD104 sensors");
MODULE_LICENSE("GPL");

#define CODE_VERSION	"2024-03-23_V-2"
#define MCLK              (27*1000*1000)
#define CLK_POL           V4L2_MBUS_PCLK_SAMPLE_RISING // V4L2_MBUS_PCLK_SAMPLE_FALLING | V4L2_MBUS_PCLK_SAMPLE_RISING

#define V4L2_IDENT_SENSOR	0x0101
#define V4L2_IDENT_SENSOR1	0x0141
#define V4L2_IDENT_SENSOR2	0x0181

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

/*
 * The TD100 sits on i2c with ID 0xca
 */
#define I2C_ADDR        0xca
#define SENSOR_NAME     "td104"

struct v4l2_subdev *td100_sd;

extern unsigned int sensor_type;

//#define TD100_DEBUG_F
#ifdef TD100_DEBUG_F

struct td100_debug_struct {
	int ch;
	enum AGC_STATE agc_state;
	unsigned long agc_value;
};

struct td100_debug_struct td100_test;

static ssize_t ch_debug_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;
	count += sprintf(buf, "ch is %d\n", td100_test.ch);

	return count;
}

static ssize_t ch_debug_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	if (!strncmp(buf, "3", 1)) {
		td100_test.ch = 3;
	} else if (!strncmp(buf, "2", 1)) {
		td100_test.ch = 2;
	} else if (!strncmp(buf, "1", 1)) {
		td100_test.ch = 1;
	} else if (!strncmp(buf, "0", 1)) {
		td100_test.ch = 0;
	} else {
		printk("not support ch id!\r\n");
	}
	return count;
}

static ssize_t state_debug_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;
	count += sprintf(buf, "agc state is %s\n", td100_test.agc_state ? "open" : "close");

	return count;
}

static ssize_t state_debug_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int ret;

	if (!strncmp(buf, "1", 1)) {
		td100_test.agc_state = AGC_OPEN;
	} else if (!strncmp(buf, "0", 1)) {
		td100_test.agc_state = AGC_CLOSE;
	} else {
		printk("not support agc_state!\r\n");
		return -1;
	}

	ret = td100_agc_control(td100_test.ch, td100_test.agc_state, td100_test.agc_value);
	if (ret != 0) {
		printk("agc control err!\r\n");
	}

	return count;
}

static ssize_t agc_debug_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;

	count += sprintf(buf, "agc value is %lu\n", td100_test.agc_value);

	return count;
}

static ssize_t agc_debug_store(struct class *class,
					struct class_attribute *attr,
					const char *buf, size_t count)
{
	int err;
	int ret;

	err = kstrtoul(buf, 10, &td100_test.agc_value); /* strict_strtoul */
	if (err) {
		return err;
	}

	ret = td100_agc_control(td100_test.ch, td100_test.agc_state, td100_test.agc_value);
	if (ret != 0) {
		printk("agc control err!\r\n");
	}

	return count;
}

static struct class_attribute td100_attrs[] = {
	__ATTR(ch, 0775, ch_debug_show, ch_debug_store),
	__ATTR(agc_state, 0775, state_debug_show, state_debug_store),
	__ATTR(agc_value, 0775, agc_debug_show, agc_debug_store),
	__ATTR_NULL};

static struct class td100_class = {
	.name = "td100_class",
	.class_attrs = td100_attrs,
};

#endif

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	if (on_off)
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
	else
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	return 0;
}

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	sensor_err("%s td104 on:%d \n", __func__, on);
	switch (on) {
	case STBY_ON:
		sensor_dbg("CSI_SUBDEV_STBY_ON!\n");
		sensor_s_sw_stby(sd, ON);
		break;
	case STBY_OFF:
		sensor_dbg("CSI_SUBDEV_STBY_OFF!\n");
		sensor_s_sw_stby(sd, OFF);
		break;
	case PWR_ON:
		sensor_print("PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, PWDN, CSI_GPIO_HIGH);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(100, 120);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		vin_gpio_write(sd, POWER_EN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_print("PWR_OFF!\n");
		/*td100 need mclk away on*/
/*		vin_set_mclk(sd, OFF);
*		vin_gpio_write(sd, PWDN, CSI_GPIO_LOW);
*		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
*		vin_set_pmu_channel(sd, IOVDD, OFF);
*		vin_set_pmu_channel(sd, DVDD, OFF);
*		vin_set_pmu_channel(sd, AVDD, OFF);
*/
		usleep_range(1000, 1200);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
	usleep_range(5000, 6000);
	vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	usleep_range(5000, 6000);
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int i = 0;
	data_type rdval = 0;

	sensor_read(sd, 0x0000, &rdval);
	while ((V4L2_IDENT_SENSOR != rdval)
			&& (V4L2_IDENT_SENSOR1 != rdval)
			&& (V4L2_IDENT_SENSOR2 != rdval)) {
		i++;
		sensor_read(sd, 0x0000, &rdval);
		sensor_print("sensor id = 0x%x\n", rdval);
		if (i > 4)
			return -EINVAL;
	}
	sensor_type = rdval;
	sensor_print("sensor id = 0x%x\n", rdval);
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	static char flag;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	if (!flag) {
		/*reset td100*/
		sensor_write(sd, 0x0200, 0x0000);
		usleep_range(5000, 6000);
		sensor_write(sd, 0x0200, 0x0100);

		/*init td100 csi for n/p distinguish*/
		td100_hw_regs_init(sd, NULL);
	}
	flag = 1;

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = VGA_WIDTH;
	info->height = VGA_HEIGHT;
	info->hflip = 0;
	info->vflip = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 25;	/* 30fps */

	info->preview_first_flag = 1;
	return 0;
}

static int value_get_ch(int value)
{
	return (value & 0x3000) >> 12;
}

/**
static int td100_get_input_fmt(struct v4l2_subdev *sd, struct tvin_ch_fmt *fmt)
{
	return __td100_get_input_fmt(sd, value_get_ch(fmt->input_fmt),
			&fmt->input_fmt);
}
 */

static int td100_get_output_fmt(struct v4l2_subdev *sd,
								struct sensor_output_fmt *fmt)
{
	struct sensor_info *info = to_state(sd);
	__u32 *sensor_fmt = info->tvin.tvin_info.input_fmt;
	__u32 *out_feld = &fmt->field;
	__u32 ch_id = fmt->ch_id;

	if (ch_id >= TVD_CH_MAX)
		return -EINVAL;

	switch (sensor_fmt[ch_id]) {
	case CVBS_PAL:
	case CVBS_NTSC:
	case YCBCR_480I:
	case YCBCR_576I:
		/*Interlace ouput set out_feld as 1*/
		*out_feld = 1;
		break;
	case YCBCR_576P:
	case YCBCR_480P:
		/*Progressive ouput set out_feld as 0*/
		*out_feld = 0;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void __check_tvin_info(struct tvin_init_info *tvin_info)
{
	__u16 *sensor_wm = &tvin_info->work_mode;

	if (tvin_info->ch_id > 3) {
		tvin_info->ch_id = 3;
		sensor_print("ch id exceed 3, set 3\n");
	}

	switch (*sensor_wm) {
	case Tvd_Input_Type_1CH_CVBS:
	case Tvd_Input_Type_YCBCR:
		tvin_info->ch_num = 1;
		break;
	case Tvd_Input_Type_2CH_CVBS:
	case Tvd_Input_Type_YCBCR_CVBS:
		tvin_info->ch_num = 2;
		break;
	case Tvd_Input_Type_4CH_CVBS:
		tvin_info->ch_num = 4;
		break;
	default:
		sensor_err("no support work_mode = %d, set as 4 ch cvbs\n",
												tvin_info->ch_num);
		tvin_info->work_mode = Tvd_Input_Type_4CH_CVBS;
		tvin_info->ch_num = 4;
	}
}

__maybe_unused static int sensor_tvin_init(struct v4l2_subdev *sd,
		struct tvin_init_info *tvin_info)
{
	struct sensor_info *info = to_state(sd);
	__u16 *sensor_wm = &info->tvin.tvin_info.work_mode;
	__u32 *sensor_fmt = info->tvin.tvin_info.input_fmt;
	__u32 ch_id = tvin_info->ch_id;
	int ret;

	if (ch_id >= TVD_CH_MAX)
		return -1;

	sensor_print("set ch%d fmt as %d\n",
			ch_id, tvin_info->input_fmt[ch_id]);
	sensor_fmt[ch_id] = tvin_info->input_fmt[ch_id];

	if (sd->entity.stream_count == 0) {
		sensor_print("set 3d_ch as %d, wm %d\n",
				tvin_info->ch_3d_filter, tvin_info->work_mode);
		*sensor_wm = tvin_info->work_mode;
		info->tvin.tvin_info.ch_3d_filter = tvin_info->ch_3d_filter;
		__check_tvin_info(&info->tvin.tvin_info);
	} else {
		ret = td100_set_ch_fmt(sd, &info->tvin.tvin_info,
				ch_id, sensor_fmt[ch_id]);
	if (ret)
		return ret;
	}

	info->tvin.flag = true;
	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);
	switch (cmd) {
	case GET_CURRENT_WIN_CFG:
		if (info->current_wins != NULL) {
			memcpy(arg, info->current_wins,
			       sizeof(*info->current_wins));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		sensor_cfg_req(sd, (struct sensor_config *)arg);
		break;
	case SENSOR_TVIN_INIT:
		// ret = sensor_tvin_init(sd, (struct tvin_init_info *)arg);
		break;
	//case GET_SENSOR_CH_INPUT_FMT:
	//	ret = td100_get_input_fmt(sd, (struct tvin_ch_fmt *)arg);
	//	break;
	case GET_SENSOR_CH_OUTPUT_FMT:
		ret = td100_get_output_fmt(sd, (struct sensor_output_fmt *)arg);
		break;

	default:
		return -EINVAL;
	}
	return ret;
}

/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
	.desc = "BT656 4CH",
	.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
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
	{
	.width = 720,
	.height = 576,
	.hoffset = 0,
	.voffset = 0,
//	.pclk_dly = 0x06, // should not larger than 0x1f
	.set_size = NULL,
	 },

	{
	 .width = 720,
	 .height = 480,
	 .hoffset = 0,
	 .voffset = 0,
	 .set_size = NULL,
	 },

	 {
	 .width = 736,
	 .height = 576,
	 .hoffset = 0,
	 .voffset = 0,
	 .set_size = NULL,
	  },

	 {
	 .width = 736,
	 .height = 480,
	 .hoffset = 0,
	 .voffset = 0,
	 .set_size = NULL,
	  },

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);
	struct tvin_init_info *tvin_info = &info->tvin.tvin_info;

	cfg->type = V4L2_MBUS_BT656;
	if (info->tvin.flag) {
		if (tvin_info->ch_num == 1) {
			cfg->flags = CLK_POL | CSI_CH_0;
		} else if (tvin_info->ch_num == 2) {
			cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1;
		} else if (tvin_info->ch_num == 4) {
			cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
		} else {
			sensor_err("no support ch_num = %d, set to 4\n", tvin_info->ch_num);
			cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
		}
	} else {
			cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
	}

	sensor_dbg("%s flags = 0x%x, mode flag = %d\n",
				__func__, cfg->flags, info->tvin.flag);
	return 0;
}

int td100_set_contrast(struct v4l2_subdev *sd, int value)
{
	int ret, ch, offset = CONTRAST_REG;
	data_type rdval = 0;

	ch = value_get_ch(value);
	sensor_read(sd, ch * 0x10 + 0x80 + offset, &rdval);

	rdval = (rdval & 0x00ff) | (value & 0xff) << 8;
	ret = sensor_write(sd, ch * 0x10 + 0x80 + offset, rdval);
	sensor_dbg("%s set ch%d contrast 0x%x\n", __func__, ch, value);
	return ret;
}

int td100_set_brigth(struct v4l2_subdev *sd, int value)
{
	int ret, ch, offset = BRIGTH_REG;
	data_type rdval = 0;

	ch = value_get_ch(value);
	sensor_read(sd, ch * 0x10 + 0x80 + offset, &rdval);
	rdval = (rdval & 0x00ff) | (value & 0xff) << 8;
	ret = sensor_write(sd, ch * 0x10 + 0x80 + offset, rdval);
	sensor_dbg("%s set ch%d contrast 0x%x\n", __func__, ch, value);
	return ret;
}

int td100_set_saturation(struct v4l2_subdev *sd, int value)
{
	int ret, ch, offset = SATURATION_REG;
	data_type rdval = 0;

	ch = value_get_ch(value);
	sensor_read(sd, ch * 0x10 + 0x80 + offset, &rdval);
	rdval = (rdval & 0x00ff) | (value & 0xff) << 8;
	ret = sensor_write(sd, ch * 0x10 + 0x80 + offset, rdval);
	sensor_dbg("%s set ch%d contrast 0x%x\n", __func__, ch, value);
	return ret;
}

int __td100_set_Cb_Cr(struct v4l2_subdev *sd,
		int ch, int Set_Cb_or_Cr, int ctrl_en, int value)
{
	int ret, offset;
	data_type reg_val;

	if (Set_Cb_or_Cr == Set_cr)
		offset = CR_EN_REG; /*Cr reg offset*/
	else
		offset = CB_REG; /*Cb reg offset */

	sensor_read(sd, ch * 0x10 + 0x80 + offset, &reg_val);
	reg_val = (value & 0xff0f) | (reg_val & 0x00f0);
	ret = sensor_write(sd, ch * 0x10 + 0x80 + offset, reg_val);

/*
*	when set cr, here set conctrl enble
*	So we should set cb first
*/
	if (ctrl_en) {
		offset = CR_EN_REG;
		reg_val = reg_val | 0x0010; /*cb cr contorl enable*/
		sensor_write(sd, ch * 0x10 + 0x80 + offset, reg_val);
	}

	return ret;
}

int td100_set_Cb_Cr(struct v4l2_subdev *sd, int value)
{
	int ch, Set_Cb_or_Cr, ctrl_en;
	int reg_value;
	ch = value_get_ch(value);

	Set_Cb_or_Cr = (value & 0x10000) >> 16;
	ctrl_en = Set_Cb_or_Cr;
/*
*	Flip register data high and low bit
*/
	reg_value = ((value & 0xff) << 8) | ((value & 0x0f00) >> 8);

	sensor_dbg("%s, ch%d, flag = %d, value = 0x%x\n",
					__func__, ch, Set_Cb_or_Cr, reg_value);

	return __td100_set_Cb_Cr(sd, ch, Set_Cb_or_Cr, ctrl_en, reg_value);

}


static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	return -EINVAL;
}

static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		return td100_set_brigth(sd, ctrl->val);
	case V4L2_CID_CONTRAST:
		return td100_set_contrast(sd, ctrl->val);
	case V4L2_CID_SATURATION:
		return td100_set_saturation(sd, ctrl->val);
	case V4L2_CID_COLORFX_CBCR:
		return td100_set_Cb_Cr(sd, ctrl->val);
	default:
		return -EINVAL;
	}
	return -EINVAL;
}

static int sensor_reg_init(struct sensor_info *info)
{
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	sensor_dbg("sensor_reg_init\n");
	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);


	if (info->tvin.flag)
		td100_hw_regs_init(sd, &info->tvin.tvin_info);
	else
		td100_hw_regs_init(sd, NULL);

	sensor_dbg("sensor_reg_init end\n");

	if (wsize->set_size)
		wsize->set_size(sd);
	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	sensor_err("%s td104 enable:%d \n", __func__, enable);
	sensor_dbg("%s on = %d, %d*%d %x\n", __func__, enable,
								info->current_wins->width,
								info->current_wins->height,
								info->fmt->mbus_code);

	if (!enable) {
		info->tvin.flag = false;
		return 0;
	}
	return sensor_reg_init(info);
}

static void td100_set_input_size(struct sensor_info *info,
		struct v4l2_subdev_format *fmt, int ch_id)
{
		struct tvin_init_info *tvin_info = &info->tvin.tvin_info;

		switch (tvin_info->work_mode) {
		case Tvd_Input_Type_1CH_CVBS:
		case Tvd_Input_Type_2CH_CVBS:
		case Tvd_Input_Type_4CH_CVBS:
			if (tvin_info->input_fmt[ch_id] == CVBS_PAL)
				fmt->format.height = 576;
			else if (tvin_info->input_fmt[ch_id] == CVBS_NTSC)
				fmt->format.height = 480;
			break;
		case Tvd_Input_Type_YCBCR:
		case Tvd_Input_Type_YCBCR_CVBS:
			if (0 == ch_id) {
				if (tvin_info->input_fmt[ch_id] == YCBCR_576I
					|| tvin_info->input_fmt[ch_id] == YCBCR_576P)
					fmt->format.height = 576;
				else if (tvin_info->input_fmt[ch_id] == YCBCR_480I
					|| tvin_info->input_fmt[ch_id] == YCBCR_480P)
					fmt->format.height = 480;
			} else if (1 == ch_id) {
				if (tvin_info->input_fmt[ch_id] == CVBS_PAL)
					fmt->format.height = 576;
				else if (tvin_info->input_fmt[ch_id] == CVBS_NTSC)
					fmt->format.height = 480;
			}
			break;
		default:
			break;
		}
}


int td100_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	int ret = 0;

	if (!info->tvin.flag)
		return sensor_set_fmt(sd, cfg, fmt);

	if (sd->entity.stream_count == 0) {
		td100_set_input_size(info, fmt, fmt->reserved[0]);
		ret = sensor_set_fmt(sd, cfg, fmt);
		sensor_print("%s befor ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	} else {
		ret = sensor_set_fmt(sd, cfg, fmt);
		td100_set_input_size(info, fmt, fmt->reserved[0]);
		sensor_print("%s after ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	}

	return ret;

}

static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.try_ctrl = sensor_try_ctrl,
};

/* ----------------------------------------------------------------------- */


static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	//.s_parm = sensor_s_parm,
	//.g_parm = sensor_g_parm,
	.s_stream = sensor_s_stream,
	//.g_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.get_fmt = sensor_get_fmt,
	.set_fmt = td100_set_fmt,
	.get_mbus_config = sensor_g_mbus_config,
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
	.data_width = CCI_BITS_16,
};

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	int ret = 0;

	ret = v4l2_ctrl_handler_init(handler, 4);

	v4l2_ctrl_new_std(handler, ops, V4L2_CID_BRIGHTNESS, 0, 0xffff, 1, 0);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_CONTRAST, 0, 0xffff, 1, 0);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_SATURATION, 0, 0xffff, 1, 0);
	v4l2_ctrl_new_std(handler, ops, V4L2_CID_COLORFX_CBCR, 0, 0xfffff, 1, 0);

	if (handler->error) {
		ret = handler->error;
		sensor_dbg("%s error\n", __func__);
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct v4l2_subdev *sd;
	struct sensor_info *info;
	sensor_err("%s td104 CODE_VERSION:%s \n", __func__, CODE_VERSION);
	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	td100_sd = sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);
#ifdef TD100_DEBUG_F
	td100_test.ch = 0;
	td100_test.agc_state = AGC_OPEN;
	td100_test.agc_value = 0x0F;
	class_register(&td100_class);
#endif
	mutex_init(&info->lock);
	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_INTERLACED;
	info->tvin.tvin_info.ch_3d_filter = TVD_3D_CH;
	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	sensor_err("%s td104 ss CODE_VERSION:%s \n", __func__, CODE_VERSION);
	sd = cci_dev_remove_helper(client, &cci_drv);
	#ifdef TD100_DEBUG_F
	class_unregister(&td100_class);
	#endif
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
	sensor_err("%s td104  CODE_VERSION:%s \n", __func__, CODE_VERSION);
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

#if IS_ENABLED(CONFIG_VIDEO_SUNXI_VIN_SPECIAL)
subsys_initcall_sync(init_sensor);
#else
module_init(init_sensor);
#endif
module_exit(exit_sensor);



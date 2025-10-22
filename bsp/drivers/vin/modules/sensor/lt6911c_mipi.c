/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for lt6911c cameras and TVI Coax protocol.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Chenjinkun <chenjinkun@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "../../utility/vin_log.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <linux/v4l2-dv-timings.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-event.h>
#include <linux/io.h>
#include "camera.h"
#include "sensor_helper.h"

MODULE_AUTHOR("CJk");
MODULE_DESCRIPTION("A low-level driver for lt6911c mipi chip for TVI sensor");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

static char dev_info[128] = "s:0-w:0-h:0-fps:0-r:0"; //state-width-height-fps-rate
module_param_string(dev_info, dev_info, sizeof(dev_info), S_IRUGO | S_IWUSR);

#define MCLK              (27*1000*1000)
#define V4L2_IDENT_SENSOR  0x1605

/*
 * The lt6911c i2c address
 */
#define I2C_ADDR  0x56

/*static struct delayed_work sensor_s_ae_ratio_work;*/
static bool restart;

struct gpio_config power_switch_gpio;

#define SENSOR_NAME "lt6911c_mipi"

/*define module timing*/
static const struct v4l2_dv_timings_cap lt6911c_timings_cap = {
	.type = V4L2_DV_BT_656_1120,
	.reserved = { 0 },
	V4L2_INIT_BT_TIMINGS(
			640, 3840,       /* min/max width */
			480, 2160,       /* min/max height */
			0, 400000000,    /* min/max pixelclock*/
			V4L2_DV_BT_STD_CEA861 | V4L2_DV_BT_STD_DMT |
			V4L2_DV_BT_STD_GTF | V4L2_DV_BT_STD_CVT,  /* Supported standards */
			V4L2_DV_BT_CAP_PROGRESSIVE | V4L2_DV_BT_CAP_INTERLACED |
			V4L2_DV_BT_CAP_REDUCED_BLANKING |
			V4L2_DV_BT_CAP_CUSTOM)    /* capabilities */
};

static enum hotplut_state hotplut_status;

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {
};

static void sensor_ii2_en(struct v4l2_subdev *sd, unsigned int en)
{
	if (en) {
		sensor_write(sd, 0xff, 0x80);
		sensor_write(sd, 0xee, 0x01); /* iic en */
	} else {
		sensor_write(sd, 0xff, 0x80);
		sensor_write(sd, 0xee, 0x00); /* iic disable */
	}
}

static int sensor_g_exp(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_exp(struct v4l2_subdev *sd, unsigned int exp_val)
{
	return 0;
}

static int sensor_g_gain(struct v4l2_subdev *sd, __s32 *value)
{
	return 0;
}

static int sensor_s_gain(struct v4l2_subdev *sd, int gain_val)
{
	return 0;
}

static int sensor_s_exp_gain(struct v4l2_subdev *sd,
			     struct sensor_exp_gain *exp_gain)
{
	return 0;
}


static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	int ret = 0;
	return ret;
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
		vin_set_mclk(sd, OFF);
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
		ret = sensor_s_sw_stby(sd, CSI_GPIO_LOW);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		usleep_range(20000, 22000);

		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		cci_lock(sd);

		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);

		cci_unlock(sd);

		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF1123\n");
		cci_lock(sd);

		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);

		usleep_range(1000, 1200);
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
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);//HIGH
		usleep_range(30000, 32000);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(60000, 62000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval = 0;
	unsigned int SENSOR_ID = 0;
	int cnt = 0;

	sensor_ii2_en(sd, 1);

	sensor_write(sd, 0xff, 0xa0); /* FF save Bank addr */
	sensor_read(sd, 0x00, &rdval);
	SENSOR_ID |= (rdval << 8);
	sensor_read(sd, 0x01, &rdval);
	SENSOR_ID |= (rdval);
	pr_err("V4L2_IDENT_SENSOR = 0x%x\n", SENSOR_ID);

	while ((SENSOR_ID != V4L2_IDENT_SENSOR) && (cnt < 5)) {
		sensor_read(sd, 0x00, &rdval);
		SENSOR_ID |= (rdval << 8);
		sensor_read(sd, 0x01, &rdval);
		SENSOR_ID |= (rdval);
		sensor_print("retry = %d, V4L2_IDENT_SENSOR = %x\n", cnt, SENSOR_ID);
		cnt++;
	}

	sensor_ii2_en(sd, 0);

	return 0;
}

static unsigned int fps_calc(const struct v4l2_bt_timings *t)
{
	if (!V4L2_DV_BT_FRAME_HEIGHT(t) || !V4L2_DV_BT_FRAME_WIDTH(t))
		return 0;

	return DIV_ROUND_CLOSEST((unsigned int)t->pixelclock,
			V4L2_DV_BT_FRAME_HEIGHT(t) * V4L2_DV_BT_FRAME_WIDTH(t));
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");
	restart = 0;


	/*Make sure it is a target sensor */
	ret = sensor_detect(sd);

	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1920;
	info->height = 1080;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->tpf.numerator = 1;
	info->tpf.denominator = 25;

	return 0;
}

static int lt6911c_get_detected_timings(struct v4l2_subdev *sd, struct v4l2_dv_timings *timings)
{
	//struct sensor_info *info = to_state(sd);
	struct v4l2_bt_timings *bt = &timings->bt;
	u32 hact, vact, htotal, vtotal;
	u32 hbp, hs, hfp, vbp, vs, vfp;
	u32 pixel_clock, halt_pix_clk, fps;
	data_type clk_h = 0, clk_m = 0, clk_l = 0;
	data_type value = 0, val_h = 0, val_l = 0;

	memset(timings, 0, sizeof(struct v4l2_dv_timings));

	sensor_ii2_en(sd, 1);

	sensor_write(sd, 0xff, 0xa0);
	sensor_write(sd, 0x34, 0x21);
	usleep_range(10000, 10100);
	sensor_write(sd, 0xff, 0xb8);
	sensor_read(sd, 0xb1, &clk_h);
	sensor_read(sd, 0xb2, &clk_m);
	sensor_read(sd, 0xb3, &clk_l);
	halt_pix_clk = (((clk_h & 0xf) << 16) | (clk_m << 8) | clk_l);
	pixel_clock = halt_pix_clk * 2 * 1000;

	sensor_write(sd, 0xff, 0xd2);
	sensor_write(sd, 0x83, 0x11);
	usleep_range(10000, 10100);
	sensor_read(sd, 0x89, &val_h);
	sensor_read(sd, 0x8a, &val_l);
	htotal = ((val_h << 8) | val_l) * 2;
	sensor_read(sd, 0x9e, &val_h);
	sensor_read(sd, 0x9f, &val_l);
	vtotal = (val_h << 8) | val_l;
	sensor_read(sd, 0x8b, &val_h);
	sensor_read(sd, 0x8c, &val_l);
	hact = ((val_h << 8) | val_l) * 2;
	sensor_read(sd, 0x96, &val_h);
	sensor_read(sd, 0x97, &val_l);
	vact = (val_h << 8) | val_l;
	sensor_read(sd, 0x94, &val_h);
	sensor_read(sd, 0x95, &val_l);
	hs = ((val_h << 8) | val_l) * 2;
	sensor_read(sd, 0x86, &value);
	vs = value;
	sensor_read(sd, 0x9c, &val_h);
	sensor_read(sd, 0x9d, &val_l);
	hfp = ((val_h << 8) | val_l) * 2;
	sensor_read(sd, 0x88, &value);
	vfp = value;
	sensor_read(sd, 0x98, &val_h);
	sensor_read(sd, 0x99, &val_l);
	hbp = ((val_h << 8) | val_l) * 2;
	sensor_read(sd, 0x87, &value);
	vbp = value;

	sensor_ii2_en(sd, 0);

	timings->type = V4L2_DV_BT_656_1120;
	bt->interlaced = V4L2_DV_PROGRESSIVE;
	bt->width = hact;
	bt->height = vact;
	bt->vsync = vs;
	bt->hsync = hs;
	bt->pixelclock = pixel_clock;
	bt->hfrontporch = hfp;
	bt->vfrontporch = vfp;
	bt->hbackporch = hbp;
	bt->vbackporch = vbp;
	fps = fps_calc(bt);

	sensor_print("act:%dx%d, total:%dx%d, pixclk:%d, fps:%d\n",
			hact, vact, htotal, vtotal, pixel_clock, fps);
	sensor_print("hfp:%d, hs:%d, hbp:%d, vfp:%d, vs:%d, vbp:%d\n",
			hfp, hs, hbp, vfp, vs, vbp);

#if VIN_FALSE
	sensor_ii2_en(sd, 1);

	sensor_write(sd, 0xff, 0xd8);
	sensor_write(sd, 0x55, 0x80);
	sensor_write(sd, 0x52, 0x00); // DPCD addr[19:16]
	sensor_write(sd, 0x51, 0x01); // DPCD addr[15:8]
	sensor_write(sd, 0x50, 0x01); // DPCD addr[7:0]
	sensor_read(sd, 0x54, &value);  // DPCD 00101h read DP lane count 84

	sensor_ii2_en(sd, 0);

	vin_print("DP lane count is 0x%x(0x81=1lane 0x82=2lane 0x84=4lane)\n", value);
#endif
	return 0;
}

static int lt6911c_get_audio_sampling_rate(struct v4l2_subdev *sd, int *fs_value)
{
	data_type rate = 0;

	sensor_ii2_en(sd, 1);
	sensor_write(sd, 0xff, 0xd1);
	sensor_read(sd, 0x55, &rate);
	sensor_ii2_en(sd, 0);

	if (rate == 44)
		*fs_value = 44100;
	else if (rate == 47)
		*fs_value = 48000;
	else
		*fs_value = 1000 * rate;

	sensor_print("read 0x55 reg, val: %d, audio sample rate: %d Hz\n", rate, *fs_value);

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
			       sizeof(struct sensor_win_size));
			ret = 0;
		} else {
			sensor_err("empty wins!\n");
			ret = -1;
		}
		break;
	case SET_FPS:
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
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

static struct sensor_format_struct sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= MEDIA_BUS_FMT_UYVY8_2X8,  //  MEDIA_BUS_FMT_UYVY8_2X8
		.regs 		= sensor_default_regs,
		.regs_size = ARRAY_SIZE(sensor_default_regs),
		.bpp		= 2,
	}
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	{
	  .width = 720,
	  .height = 480,
	  .hoffset = 0,
	  .voffset = 0,
	  .fps_fixed = 60,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
	{
	  .width = 720,
	  .height = 576,
	  .hoffset = 0,
	  .voffset = 0,
	  .fps_fixed = 50,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
	{
	  .width = 1280,
	  .height = 720,
	  .hoffset = 0,
	  .voffset = 0,
	  .fps_fixed = 50,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
	{
	  .width = 1280,
	  .height = 720,
	  .hoffset = 0,
	  .voffset = 0,
	  .fps_fixed = 60,
	  .regs = sensor_default_regs,
	  .regs_size = ARRAY_SIZE(sensor_default_regs),
	  .set_size = NULL,
	},
	{
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .fps_fixed = 50,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	},
	{
	 .width = 1920,
	 .height = 1080,
	 .hoffset = 0,
	 .voffset = 0,
	 .fps_fixed = 60,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	},
	{
	 .width = 3840,
	 .height = 2160,
	 .hoffset = 0,
	 .voffset = 0,
	 .fps_fixed = 30,
	 .regs = sensor_default_regs,
	 .regs_size = ARRAY_SIZE(sensor_default_regs),
	 .set_size = NULL,
	},
};
#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	cfg->type = V4L2_MBUS_CSI2_DPHY;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	cfg->bus.mipi_csi2.num_data_lanes = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#else
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
#endif
	return 0;
}

static int sensor_g_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
		container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_g_gain(sd, &ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_g_exp(sd, &ctrl->val);
	}
	return -EINVAL;
}


static int sensor_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct sensor_info *info =
			container_of(ctrl->handler, struct sensor_info, handler);
	struct v4l2_subdev *sd = &info->sd;

	switch (ctrl->id) {
	case V4L2_CID_GAIN:
		return sensor_s_gain(sd, ctrl->val);
	case V4L2_CID_EXPOSURE:
		return sensor_s_exp(sd, ctrl->val);
	}

	return 0;
}

static int sensor_s_dv_timings(struct v4l2_subdev *sd,
				 struct v4l2_dv_timings *timings)
{
	struct sensor_info *info = to_state(sd);

	if (!timings)
		return -EINVAL;

	//v4l2_print_dv_timings(sd->name, "s_dv_timings: ", timings, false);

	if (v4l2_match_dv_timings(&info->timings, timings, 0, false)) {
		sensor_err("%s: timings no change\n", __func__);
		return 0;
	}

	if (!v4l2_valid_dv_timings(timings,
				&lt6911c_timings_cap, NULL, NULL)) {
		sensor_err("%s: timings out of range\n", __func__);
		return -ERANGE;
	}

	sensor_print("%s timings is get form input dp size, cannot set by users\n", sd->name);

	return 0;
}

static int sensor_g_dv_timings(struct v4l2_subdev *sd,
				struct v4l2_dv_timings *timings)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;

	if (!timings)
		return -EINVAL;

	mutex_lock(&sensor_indet->detect_lock);
	lt6911c_get_detected_timings(sd, timings);
	memcpy(&info->timings, timings, sizeof(*timings));
	mutex_unlock(&sensor_indet->detect_lock);

	//v4l2_print_dv_timings(sd->name, "g_dv_timings: ", timings, false);

	return 0;
}

static int sensor_query_dv_timings(struct v4l2_subdev *sd,
				struct v4l2_dv_timings *timings)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;

	if (!timings)
		return -EINVAL;

	mutex_lock(&sensor_indet->detect_lock);
	//lt6911c_get_detected_timings(sd, timings);
	memcpy(timings, &info->timings, sizeof(*timings));
	//v4l2_print_dv_timings(sd->name, "query_dv_timings: ", timings, false);

	if (!v4l2_valid_dv_timings(timings, &lt6911c_timings_cap, NULL, NULL)) {
		sensor_err("%s: timings out of range\n", __func__);
		//return -ERANGE;
	}
	mutex_unlock(&sensor_indet->detect_lock);

	return 0;
}

static int sensor_enum_dv_timings(struct v4l2_subdev *sd,
				struct v4l2_enum_dv_timings *timings)
{
	if (timings->pad != 0)
		return -EINVAL;

	return v4l2_enum_dv_timings_cap(timings,
			&lt6911c_timings_cap, NULL, NULL);
}

static int sensor_dv_timings_cap(struct v4l2_subdev *sd,
				struct v4l2_dv_timings_cap *cap)
{
	if (cap->pad != 0)
		return -EINVAL;

	*cap = lt6911c_timings_cap;

	return 0;
}

static int sensor_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				    struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		sensor_dbg("sensor subscribe event -- V4L2_EVENT_SOURCE_CHANGE\n");
		return v4l2_src_change_event_subdev_subscribe(sd, fh, sub);
	case V4L2_EVENT_CTRL:
		sensor_dbg("sensor subscribe event -- V4L2_EVENT_CTRL\n");
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	default:
		return -EINVAL;
	}
}

static int sensor_reg_init(struct sensor_info *info)
{
	int ret;
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	ret = sensor_write_array(sd, sensor_default_regs,
					ARRAY_SIZE(sensor_default_regs));

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);
	if (ret < 0) {
		sensor_err("write sensor_default_regs error\n");
		return ret;
	}
	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	sensor_dbg("s_fmt set width = %d, height = %d\n", wsize->width,
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


/* ----------------------------------------------------------------------- */
static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.try_ctrl = sensor_try_ctrl,
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.subscribe_event = sensor_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
	.ioctl = sensor_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_dv_timings = sensor_s_dv_timings,
	.g_dv_timings = sensor_g_dv_timings,
	.query_dv_timings = sensor_query_dv_timings,
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_dv_timings = sensor_enum_dv_timings,
	.dv_timings_cap = sensor_dv_timings_cap,
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
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
	.addr_width = CCI_BITS_8,
	.data_width = CCI_BITS_8,
};

static bool sensor_detect_res(u32 width, u32 height, u32 fps)
{
	u32 i;

	for (i = 0; i < N_WIN_SIZES; i++) {
		if ((sensor_win_sizes[i].width == width) &&
			(sensor_win_sizes[i].height == height) &&
			(sensor_win_sizes[i].fps_fixed == fps)) {
			break;
		}
	}

	if (i == N_WIN_SIZES) {
		sensor_err("%s:do not support res wxh@fps: %dx%d@%dfps\n", __func__,
				width, height, fps);
		return false;
	} else {
		return true;
	}
}

static int __sensor_insert_detect(data_type *val)
{
	if (hotplut_status == HOTPLUT_DP_OUT) {
		sensor_print("hotplut status is hotplug hdmi-in out!\n");
		*val = 0x00;
	} else if (hotplut_status == HOTPLUT_DP_IN) {
		sensor_print("hotplut status is hotplug hdmi-in in!\n");
		*val = 0x01;
	} else if (hotplut_status == HOTPLUT_DP_NOSUPPRT) {
		sensor_print("hotplut status is hotplug not support resolution!\n");
		*val = 0x00;
	} else if (hotplut_status == HOTPLUT_DP_RESOLUTION) {
		sensor_print("hotplut status is resolution change!\n");
		*val = 0x00;
	} else {
		sensor_print("hotplut status is not support!\n");
		*val = 0x00;
	}

	return 0;
}

static ssize_t get_det_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	data_type val;
	__sensor_insert_detect(&val);
	return sprintf(buf, "0x%x\n", val);
}

static struct device_attribute detect_dev_attrs = {
	.attr = {
		.name = "online",
		.mode = S_IRUGO,
	},
	.show = get_det_status_show,
	.store = NULL,
};

static void sensor_uevent_notifiy(struct v4l2_subdev *sd, int w, int h, int fps, int s_rate, int status)
{
	char state[16], width[16], height[16], frame[16], rate[16];
	char *envp[7] = {
		"SYSTEM=HDMIIN",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL };
	snprintf(state, sizeof(state), "STATE=%d", status);
	snprintf(width, sizeof(width), "WIDTH=%d", w);
	snprintf(height, sizeof(height), "HEIGHT=%d", h);
	snprintf(frame, sizeof(frame), "FPS=%d", fps);
	snprintf(rate, sizeof(rate), "RATE=%d", s_rate);
	envp[1] = width;
	envp[2] = height;
	envp[3] = frame;
	envp[4] = rate;
	envp[5] = state;
	snprintf(dev_info, sizeof(dev_info), "s:%d-w:%d-h:%d-fps:%d-r:%d", status, w, h, fps, s_rate);
	kobject_uevent_env(&sd->dev->kobj, KOBJ_CHANGE, envp);
}

static unsigned int fs_value_old;
static void sensor_det_work(struct work_struct *work)
{
	struct delayed_work *delayed_work = to_delayed_work(work);
	struct sensor_indetect *sensor_indet = container_of(delayed_work, struct sensor_indetect, sensor_work);
	struct sensor_info *info = container_of(sensor_indet, struct sensor_info, sensor_indet);
	struct v4l2_subdev *sd = &info->sd;
	struct v4l2_dv_timings timings;
	struct v4l2_event sensor_ev_fmt;
	unsigned int changes = 0;
	bool timing_changed = 0;
	bool fs_value_changed = 0;
	unsigned int stream_count;
	unsigned int fs_value = 0;

	mutex_lock(&sensor_indet->detect_lock);

	lt6911c_get_detected_timings(sd, &timings);
	timing_changed = v4l2_match_dv_timings(&info->timings, &timings, 0, false) ? 0 : 1;

	lt6911c_get_audio_sampling_rate(sd, &fs_value);
	if (fs_value != fs_value_old) {
		fs_value_old = fs_value;
		fs_value_changed = 1;
		sensor_print("audio sampling rate has been changed to %d HZ.\n", fs_value);
	} else {
		fs_value_changed = 0;
		sensor_print("audio sampling rate(%d HZ) has not changed.\n", fs_value);
	}

	if (!timing_changed && !fs_value_changed) {
		sensor_err("%s: timings and audio sampling rate has not changed, do nothing!\n", __func__);
	} else {
		/* At least one of the timings and audio sampling rate has changed */
		if (!v4l2_valid_dv_timings(&timings, &lt6911c_timings_cap, NULL, NULL)) {
			sensor_err("%s: timings out of range\n", __func__);
			hotplut_status = HOTPLUT_DP_OUT;
			v4l2_ctrl_s_ctrl(info->sensor_indet.ctrl_hotplug, HOTPLUT_DP_OUT); /* not signal -- dp out */
			sensor_uevent_notifiy(sd, 0, 0, 0, 0, 0);
			sensor_print("%s send hotplug dp out to user\n", sd->name);
		} else {
			if (!sensor_detect_res(timings.bt.width, timings.bt.height, fps_calc(&timings.bt))) {
				sensor_err("%s: driver not support %dfps@%dx%d, need add to sensor win_size\n",
							__func__, fps_calc(&timings.bt), timings.bt.width, timings.bt.height);
				hotplut_status = HOTPLUT_DP_NOSUPPRT;
				v4l2_ctrl_s_ctrl(info->sensor_indet.ctrl_hotplug, HOTPLUT_DP_NOSUPPRT); /* signal -- resolution not support */
				sensor_uevent_notifiy(sd, 0, 0, 0, 0, -1);
				sensor_print("%s send hotplug not support resolution to user\n", sd->name);
			} else  {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
				stream_count = info->stream_count;
#else
				stream_count = sd->entity.stream_count;
#endif
				if (stream_count && timing_changed) {
					changes |= V4L2_EVENT_SRC_CH_RESOLUTION;  /* signal -- resolution change */
			} else {
					//lt6911c_get_audio_sampling_rate(sd, &fs_value);
					v4l2_ctrl_s_ctrl(info->sensor_indet.ctrl_hotplug, HOTPLUT_DP_IN); /* signal -- dp in */
					hotplut_status = HOTPLUT_DP_IN;
					sensor_uevent_notifiy(sd, timings.bt.width, timings.bt.height,
									fps_calc(&timings.bt), fs_value, 1);
					sensor_print("%s send hotplug dp in to user\n", sd->name);
				}
			}
			if (changes) {
				sensor_ev_fmt.id = 0;
				sensor_ev_fmt.type = V4L2_EVENT_SOURCE_CHANGE,
				sensor_ev_fmt.u.src_change.changes = changes;
				v4l2_subdev_notify_event(sd, &sensor_ev_fmt);
				sensor_print("%s send resolution change to user\n", sd->name);
			}
		}
	}
	memcpy(&info->timings, &timings, sizeof(timings));

	mutex_unlock(&sensor_indet->detect_lock);
}

static irqreturn_t sensor_det_irq_func(int irq, void *priv)
{
	struct v4l2_subdev *sd = priv;
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;

	schedule_delayed_work(&sensor_indet->sensor_work, 0);

	return IRQ_HANDLED;
}

static int sensor_det_init(struct v4l2_subdev *sd)
{
	int ret;
	struct device_node *np = NULL;
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	enum of_gpio_flags gc;
#endif
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;
	char *node_name = "sensor_detect";
	char *hotplug_gpio_name = "hotplug_gpios";
	char *power_gpio_name = "power_gpios";
	char *power_switch_gpio_name = "power1_gpios";
	char *reset_gpio_name = "reset_gpios";

	np = of_find_node_by_name(NULL, node_name);
	if (np == NULL) {
		sensor_err("can not find the %s node\n", node_name);
		return -EINVAL;
	}

	/* power on sensor to detect hotplug*/
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	sensor_indet->power_gpio.gpio = of_get_named_gpio_flags(np, power_gpio_name, 0, &gc);
#else
	sensor_indet->power_gpio.gpio = of_get_named_gpio(np, power_gpio_name, 0);
#endif
	sensor_dbg("get form %s gpio is %d\n", power_gpio_name, sensor_indet->power_gpio.gpio);
	if (!gpio_is_valid(sensor_indet->power_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed\n", power_gpio_name);
		return -ENODEV;
	} else {
		ret = gpio_request(sensor_indet->power_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", power_gpio_name);
			return -1;
		}
		gpio_direction_output(sensor_indet->power_gpio.gpio, 1);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	power_switch_gpio.gpio = of_get_named_gpio_flags(np, power_switch_gpio_name, 0, &gc);
#else
	power_switch_gpio.gpio = of_get_named_gpio(np, power_switch_gpio_name, 0);
#endif
	sensor_dbg("get form %s gpio is %d\n", power_switch_gpio_name, power_switch_gpio.gpio);
	if (!gpio_is_valid(power_switch_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed, maybe not necessary\n", power_switch_gpio_name);
		//return -ENODEV;
	} else {
		ret = gpio_request(power_switch_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", power_switch_gpio_name);
			return -1;
		}
		gpio_direction_output(power_switch_gpio.gpio, 1);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	sensor_indet->reset_gpio.gpio = of_get_named_gpio_flags(np, reset_gpio_name, 0, &gc);
#else
	sensor_indet->reset_gpio.gpio = of_get_named_gpio(np, reset_gpio_name, 0);
#endif
	sensor_dbg("get form %s gpio is %d\n", reset_gpio_name, sensor_indet->reset_gpio.gpio);
	if (!gpio_is_valid(sensor_indet->reset_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed\n", reset_gpio_name);
		return -ENODEV;
	} else {
		ret = gpio_request(sensor_indet->reset_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", reset_gpio_name);
			return -1;
		}
		gpio_direction_output(sensor_indet->reset_gpio.gpio, 0);
		usleep_range(1000, 1200);
		gpio_direction_output(sensor_indet->reset_gpio.gpio, 1);
	}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0)
	sensor_indet->hotplug_det_gpio.gpio = of_get_named_gpio_flags(np, hotplug_gpio_name, 0, &gc);
#else
	sensor_indet->hotplug_det_gpio.gpio = of_get_named_gpio(np, hotplug_gpio_name, 0);
#endif
	sensor_dbg("get form %s gpio is %d\n", hotplug_gpio_name, sensor_indet->hotplug_det_gpio.gpio);
	if (!gpio_is_valid(sensor_indet->hotplug_det_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed\n", hotplug_gpio_name);
		return -ENODEV;
	} else {
		ret = gpio_request(sensor_indet->hotplug_det_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", hotplug_gpio_name);
			return -1;
		}

		sensor_indet->hotplug_det_irq = gpio_to_irq(sensor_indet->hotplug_det_gpio.gpio);
		if (sensor_indet->hotplug_det_irq <= 0) {
			sensor_err("gpio %d get irq err\n", sensor_indet->hotplug_det_gpio.gpio);
			return -1;
		}
		ret = request_irq(sensor_indet->hotplug_det_irq, sensor_det_irq_func,
				IRQF_TRIGGER_RISING,/* IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, */
				"sensor_detect", sd);
	}

	INIT_DELAYED_WORK(&sensor_indet->sensor_work, sensor_det_work);
	mutex_init(&sensor_indet->detect_lock);

	sensor_dbg("%s seccuss\n", __func__);
	return 0;
}

static void sensor_det_exit(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;

	cancel_delayed_work_sync(&sensor_indet->sensor_work);
	if (sensor_indet->hotplug_det_irq > 0) {
		disable_irq(sensor_indet->hotplug_det_irq);
		free_irq(sensor_indet->hotplug_det_irq, NULL);
	}

	if (sensor_indet->hotplug_det_gpio.gpio > 0)
		gpio_free(sensor_indet->hotplug_det_gpio.gpio);

	if (sensor_indet->power_gpio.gpio > 0)
		gpio_free(sensor_indet->power_gpio.gpio);

	if (power_switch_gpio.gpio > 0)
		gpio_free(power_switch_gpio.gpio);
}


static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 3);

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
			      65536 * 16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	info->sensor_indet.ctrl_hotplug = v4l2_ctrl_new_std(handler, ops, V4L2_CID_DV_TX_HOTPLUG,
				HOTPLUT_DP_OUT, HOTPLUT_DP_MAX, 0, HOTPLUT_DP_OUT);

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	sd->ctrl_handler = handler;

	return ret;

}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sensor_probe(struct i2c_client *client)
#else
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{
	int ret;
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS;
	sensor_init_controls(sd, &sensor_ctrl_ops);
	mutex_init(&info->lock);
	restart = 0;

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x18;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;
	info->not_detect_use_count = 1;
#if IS_ENABLED(CONFIG_SAME_I2C)
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif

	sensor_det_init(sd);

	ret = device_create_file(sd->dev, &detect_dev_attrs);
	if (ret) {
		sensor_err("class_create file fail!\n");
	}

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int sensor_remove(struct i2c_client *client)
#else
static void sensor_remove(struct i2c_client *client)
#endif
{
	struct v4l2_subdev *sd;

	if (client)
		sensor_det_exit(i2c_get_clientdata(client));
	else
		sensor_det_exit(cci_drv.sd);

	sd = cci_dev_remove_helper(client, &cci_drv);
	device_remove_file(sd->dev, &detect_dev_attrs);
	kfree(to_state(sd));

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#endif
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

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);

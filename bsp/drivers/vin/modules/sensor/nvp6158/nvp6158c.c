/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for nvp6158 cameras.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 * Authors: Zheng Zequn <zequnzheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "../../../utility/vin_log.h"
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/videodev2.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-subdev.h>

#include "../camera.h"
#include "../sensor_helper.h"
#include "nvp6158_drv.h"
#include "common.h"

struct v4l2_subdev *gl_sd;

MODULE_AUTHOR("zzq");
MODULE_DESCRIPTION("A low-level driver for bt1120 sensors");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define MCLK (27 * 1000 * 1000)
#define CLK_POL V4L2_MBUS_PCLK_SAMPLE_FALLING
#define DOUBLE_CLK_POL (V4L2_MBUS_PCLK_SAMPLE_FALLING | V4L2_MBUS_PCLK_SAMPLE_RISING)
#define V4L2_IDENT_SENSOR 0x00a0

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 30

#define I2C_ADDR 0x64
#define SENSOR_NAME "nvp6158"

static struct regval_list sensor_regs[] = {

};

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
		sensor_dbg("CSI_SUBDEV_PWR_ON!\n");
		cci_lock(sd);
		vin_gpio_set_status(sd, RESET, CSI_GPIO_HIGH);
		vin_gpio_set_status(sd, PWDN, CSI_GPIO_HIGH);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
/*		vin_set_pmu_channel(sd, RESERVEVDD, ON); */ /* not support yet */
		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("CSI_SUBDEV_PWR_OFF!\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_set_status(sd, RESET, CSI_GPIO_HIGH);
/*		vin_set_pmu_channel(sd, RESERVEVDD, OFF);  *//* not support yet */
		vin_set_pmu_channel(sd, CAMERAVDD, OFF);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		vin_gpio_set_status(sd, RESET, CSI_GPIO_LOW);
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
	vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
	usleep_range(5000, 6000);
	vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
	usleep_range(5000, 6000);
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	data_type rdval;

	rdval = check_nvp6158_id(0x62);
	sensor_print("sensor id = 0x%x\n", rdval);

	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	/* Make sure it is a target sensor */
	ret = sensor_detect(sd);
	if (ret) {
		sensor_err("chip found is not an target chip.\n");
		return ret;
	}

	info->focus_status = 0;
	info->low_speed = 0;
	info->width = 1920; /* VGA_WIDTH; */
	info->height = 1080; /* VGA_HEIGHT; */
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;

	info->tpf.numerator = 1;
	info->tpf.denominator = 25; /* 30fps */

	info->preview_first_flag = 1;
	return 0;
}

static int sensor_tvin_init(struct v4l2_subdev *sd,
		struct tvin_init_info *tvin_info)
{
	struct sensor_info *info = to_state(sd);
	__u32 *sensor_fmt = info->tvin.tvin_info.input_fmt;
	__u32 ch_id = tvin_info->ch_id;
	__maybe_unused unsigned int stream_count;

	sensor_print("set ch%d fmt as %d\n",
			ch_id, tvin_info->input_fmt[ch_id]);
	sensor_fmt[ch_id] = tvin_info->input_fmt[ch_id];
	info->tvin.tvin_info.ch_id = ch_id;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	stream_count = info->stream_count;
#else
	stream_count = sd->entity.stream_count;
#endif
	if (stream_count != 0) {
		nvp6158_init_ch_hardware(&info->tvin.tvin_info);
		sensor_print("sensor_tvin_init nvp6158_init_ch_hardware\n");
	}
	info->tvin.flag = true;

	return 0;
}

static int sensor_get_output_fmt(struct v4l2_subdev *sd,
								struct sensor_output_fmt *fmt)
{
	struct sensor_info *info = to_state(sd);
	__u32 *sensor_fmt = info->tvin.tvin_info.input_fmt;
	__u32 *out_feld = &fmt->field;
	__u32 ch_id = fmt->ch_id;

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
	case CVBS_H1440_PAL:
		*out_feld = 1;
		sensor_print("out_feld = 1\n");
		break;
	case CVBS_H1440_NTSC:
		*out_feld = 1;
		sensor_print("out_feld = 1\n");
		break;
	case AHD720P25:
	case AHD720P30:
		*out_feld = 0;
		sensor_print("out_feld = 0\n");
		break;
	case AHD1080P25:
	case AHD1080P30:
		*out_feld = 0;
		sensor_print("out_feld = 0\n");
		break;
	default:
		return -EINVAL;
	}
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
		ret = sensor_tvin_init(sd, (struct tvin_init_info *)arg);
		break;
	case GET_SENSOR_CH_OUTPUT_FMT:
		ret = sensor_get_output_fmt(sd, (struct sensor_output_fmt *)arg);
		break;
	default:
		return -EINVAL;
	}
	return ret;
}

static void nvp6158c_set_input_size(struct sensor_info *info,
		struct v4l2_subdev_format *fmt, int ch_id)
{
		struct tvin_init_info *tvin_info = &info->tvin.tvin_info;

		switch (tvin_info->input_fmt[ch_id]) {
		case AHD720P25:
		case AHD720P30:
			fmt->format.width = 1280;
			fmt->format.height = 720;
			break;
		case AHD1080P25:
		case AHD1080P30:
			fmt->format.width = 1920;
			fmt->format.height = 1080;
			break;
		case CVBS_H1440_PAL:
			fmt->format.width = 1440;
			fmt->format.height = 576;
			break;
		case CVBS_H1440_NTSC:
			fmt->format.width = 1440;
			fmt->format.height = 480;
			break;
		default:
			break;
		}
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
int nvp6158c_sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_state *state,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	int ret;
	__maybe_unused unsigned int stream_count;

	sensor_print("fmt->format.width = %d\n", fmt->format.width);

	if (fmt->format.width == 1440 || fmt->format.width == 720)  // NTSC/PAL
		info->sensor_field = V4L2_FIELD_INTERLACED;
	else
		info->sensor_field = V4L2_FIELD_NONE;

	if (!info->tvin.flag)
		return sensor_set_fmt(sd, state, fmt);


#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	stream_count = info->stream_count;
#else
	sensor_print("[%s]sd->entity.stream_count == %d\n", __func__, sd->entity.stream_count);
	stream_count = sd->entity.stream_count;
#endif
	if (stream_count == 0) {
		nvp6158c_set_input_size(info, fmt, fmt->reserved[0]);
		ret = sensor_set_fmt(sd, state, fmt);
		sensor_print("%s befor ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	} else {
		ret = sensor_set_fmt(sd, state, fmt);
		nvp6158c_set_input_size(info, fmt, fmt->reserved[0]);
		sensor_print("%s after ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	}

	return ret;
}
#else
int nvp6158c_sensor_set_fmt(struct v4l2_subdev *sd,
			struct v4l2_subdev_pad_config *cfg,
			struct v4l2_subdev_format *fmt)
{
	struct sensor_info *info = to_state(sd);
	int ret;

	sensor_print("fmt->format.width = %d\n", fmt->format.width);

	if (fmt->format.width == 1440 || fmt->format.width == 720)  // NTSC/PAL
		info->sensor_field = V4L2_FIELD_INTERLACED;
	else
		info->sensor_field = V4L2_FIELD_NONE;

	if (!info->tvin.flag)
		return sensor_set_fmt(sd, cfg, fmt);

	sensor_print("[%s]sd->entity.stream_count == %d\n", __func__, sd->entity.stream_count);

	if (sd->entity.stream_count == 0) {
		nvp6158c_set_input_size(info, fmt, fmt->reserved[0]);
		ret = sensor_set_fmt(sd, cfg, fmt);
		sensor_print("%s befor ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	} else {
		ret = sensor_set_fmt(sd, cfg, fmt);
		nvp6158c_set_input_size(info, fmt, fmt->reserved[0]);
		sensor_print("%s after ch%d %d*%d \n", __func__,
			fmt->reserved[0], fmt->format.width, fmt->format.height);
	}

	return ret;
}
#endif
/*
 * Store information about the video data format.
 */
static struct sensor_format_struct sensor_formats[] = {
	{
	.desc = "BT656 4CH",
#if 1 /* BT1120 */
	.mbus_code = MEDIA_BUS_FMT_YUYV8_1X16,
#else /* BT656 */
	.mbus_code = MEDIA_BUS_FMT_YUYV8_2X8,
#endif
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
	.width = 1280,
	.height = 720,
	.hoffset = 0,
	.voffset = 0,
	.fps_fixed = 25,
	.regs = sensor_regs,
	.regs_size = ARRAY_SIZE(sensor_regs),
	.pclk_dly = 0x06, // should not larger than 0x1f
	.set_size = NULL,
	},
	{
	.width = 1280,
	.height = 720,
	.hoffset = 0,
	.voffset = 0,
	.fps_fixed = 30,
	.regs = sensor_regs,
	.regs_size = ARRAY_SIZE(sensor_regs),
	.pclk_dly = 0x06, // should not larger than 0x1f
	.set_size = NULL,
	},
	{
	.width = 1920,
	.height = 1080,
	.hoffset = 0,
	.voffset = 0,
	.fps_fixed = 25,
	.regs = sensor_regs,
	.regs_size = ARRAY_SIZE(sensor_regs),
	.pclk_dly = 0x06, // should not larger than 0x1f
	.set_size = NULL,
	},
	{
	.width = 1920,
	.height = 1080,
	.hoffset = 0,
	.voffset = 0,
	.fps_fixed = 30,
	.regs = sensor_regs,
	.regs_size = ARRAY_SIZE(sensor_regs),
	.pclk_dly = 0x06, // should not larger than 0x1f
	.set_size = NULL,
	},
	{
	.width = 1440,
	.height = 576,
	.hoffset = 0,
	.voffset = 0,
	.fps_fixed = 25,
	.regs = sensor_regs,
	.regs_size = ARRAY_SIZE(sensor_regs),
	.pclk_dly = 0x06, // should not larger than 0x1f
	.set_size = NULL,
	},
	{
	.width = 1440,
	.height = 480,
	.hoffset = 0,
	.voffset = 0,
	.fps_fixed = 25,
	.regs = sensor_regs,
	.regs_size = ARRAY_SIZE(sensor_regs),
	.pclk_dly = 0x06, // should not larger than 0x1f
	.set_size = NULL,
	},
	{
	.width = 720,
	.height = 480,
	.hoffset = 0,
	.voffset = 0,
	.fps_fixed = 25,
	.regs = sensor_regs,
	.regs_size = ARRAY_SIZE(sensor_regs),
	.pclk_dly = 0x06,
	.set_size = NULL,
	},
	{
	.width = 720,
	.height = 576,
	.hoffset = 0,
	.voffset = 0,
	.fps_fixed = 25,
	.regs = sensor_regs,
	.regs_size = ARRAY_SIZE(sensor_regs),
	.pclk_dly = 0x06,
	.set_size = NULL,
	},

};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	struct sensor_info *info = to_state(sd);

	cfg->type = V4L2_MBUS_BT656;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
	if (info->current_wins->width_input == 1920 && info->current_wins->height_input == 1080)
		cfg->bus.parallel.flags = DOUBLE_CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
	else
		cfg->bus.parallel.flags = CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
		/* cfg->flags = CLK_POL | CSI_CH_0; */
#else
	if (info->current_wins->width_input == 1920 && info->current_wins->height_input == 1080)
		cfg->flags = DOUBLE_CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
	else
		cfg->flags = CLK_POL | CSI_CH_0 | CSI_CH_1 | CSI_CH_2 | CSI_CH_3;
		/* cfg->flags = CLK_POL | CSI_CH_0; */
#endif

	return 0;
}

static int sensor_reg_init(struct sensor_info *info)
{
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	sensor_write_array(sd, sensor_fmt->regs, sensor_fmt->regs_size);

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;

	if (info->width == 1920 && info->height == 1080) {
		if (wsize->fps_fixed == 25) {
			nvp6158_init_hardware(AHD20_1080P_25P);
		} else {
			nvp6158_init_hardware(AHD20_1080P_30P);
		}
	} else if (info->width == 1280 && info->height == 720) {
		if (wsize->fps_fixed == 25) {
			nvp6158_init_hardware(AHD20_720P_25P);
		} else {
			nvp6158_init_hardware(AHD20_720P_30P);
		}
	} else if (info->width == 1440 && info->height == 576) {
		nvp6158_init_hardware(AHD20_SD_H1440_PAL);
	} else if (info->width == 1440 && info->height == 480) {
		nvp6158_init_hardware(AHD20_SD_H1440_NT);
	} else if (info->width == 720 && info->height == 576) {
		nvp6158_init_hardware(AHD20_SD_SH720_PAL);
	} else if (info->width == 720 && info->height == 480) {
		nvp6158_init_hardware(AHD20_SD_SH720_NT);
	}

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);

	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		     info->current_wins->width, info->current_wins->height,
		     info->fmt->mbus_code);

	if (!enable) {
		info->tvin.flag = false;
		return 0;
	}

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
	.get_fmt = sensor_get_fmt,
	.set_fmt = nvp6158c_sensor_set_fmt,
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

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
static int sensor_probe(struct i2c_client *client)
#else
static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
#endif
{

	struct sensor_info *info;

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	gl_sd = &info->sd;
	cci_dev_probe_helper(gl_sd, client, &sensor_ops, &cci_drv);
	mutex_init(&info->lock);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;

	return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int sensor_remove(struct i2c_client *client)
#else
static void sensor_remove(struct i2c_client *client)
#endif
{
	struct v4l2_subdev *sd;

	sd = cci_dev_remove_helper(client, &cci_drv);
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

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/* A V4L2 driver for ROHM BU18RM84 serdes sensor.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Hongyi <hongyi@allwinnertech.com>
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

#include "rm84_reg.h"

MODULE_AUTHOR("HY");
MODULE_DESCRIPTION("A low-level driver for ROHM BU18RM84 serdes sensor");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

/*define module timing*/
#define MCLK              (27*1000*1000)

#define SENSOR_NAME "rm84_serdes"

struct v4l2_subdev *rm84_sd;

// #define __DEBUG_RM84
#ifdef __DEBUG_RM84

__maybe_unused static unsigned int gpio_i2c_read(struct v4l2_subdev *sd, unsigned short chip_addr, int reg, int bit_flag);
static unsigned int rm84_reg_write(struct v4l2_subdev *sd, rohm_reglist *reg);
static struct class *rm84_class;

#define __err(x, arg...) pr_err("[RM84]"x, ##arg)
#define __warn(x, arg...) pr_warn("[RM84]"x, ##arg)
#define __info(x, arg...) pr_info("[RM84]"x, ##arg)

typedef struct debug_info_t {
	unsigned long regaddr;
	unsigned int reg;
	unsigned int data;
	unsigned int device_addr;
	data_type regReadVal;
	unsigned int wr16_bit;
	unsigned int wr_bit;
} DEBUG_I;

DEBUG_I rm84_debug;

static ssize_t rm84_debug_store(struct class *class, struct class_attribute *attr,
							const char *buf, size_t count)
{
	int num = 1;
	unsigned int val = 0;
	rohm_reglist reg;
	int range_count = 0;
	int i;

	rm84_debug.regaddr = 0x00;

	if (num != sscanf(buf, "0x%lx\n", &rm84_debug.regaddr)) {
		__err("buf error!\r\n");
		return -1;
	}

	__info("regaddr is 0x%lx !\r\n", rm84_debug.regaddr);

	rm84_debug.data = rm84_debug.regaddr & 0x0000ffff;
	rm84_debug.reg = (rm84_debug.regaddr >> 16) & 0x0000ffff;
	rm84_debug.device_addr = (rm84_debug.regaddr >> 32) & 0x000000ff;
	rm84_debug.wr16_bit = (rm84_debug.regaddr >> 40) & 0x01;
	rm84_debug.wr_bit = (rm84_debug.regaddr >> 44) & 0x01;

	__info("data: 0x%x , reg: 0x%x , wr16_bit: 0x%x , wr_bit: 0x%x !\r\n", rm84_debug.data, rm84_debug.reg, rm84_debug.wr16_bit, rm84_debug.wr_bit);
	if (rm84_debug.wr_bit) {
		reg.data = rm84_debug.data;
		reg.reg_addr = rm84_debug.reg;
		reg.device_addr = rm84_debug.device_addr;
		reg.wr16 = rm84_debug.wr16_bit;
		rm84_reg_write(rm84_sd, &reg);
		__info("write!\r\n");
	} else {
		__info("read!\r\n");
		if (rm84_debug.data > rm84_debug.reg) {
			range_count = rm84_debug.data - rm84_debug.reg;

			for (i = 0; i <= range_count; i++) {
				val = gpio_i2c_read(rm84_sd, rm84_debug.device_addr, rm84_debug.reg + i, rm84_debug.wr16_bit);
				__info(" reg[0x%x]: 0x%x ", rm84_debug.reg + i, val);
				if (i / 10 > 0) {
					printk("\r\n");
				}
			}
		} else {
			val = gpio_i2c_read(rm84_sd, rm84_debug.device_addr, rm84_debug.reg, rm84_debug.wr16_bit);
			__info("chip 0x%x reg 0x%x val = 0x%x\r\n", rm84_debug.device_addr, rm84_debug.reg, val);
		}
	}

	return count;
}

static ssize_t rm84_debug_show(struct class *class,
					struct class_attribute *attr, char *buf)
{
	int count = 0;

	__info("echo 0x%lx > reg!\r\n", rm84_debug.regaddr);
	__info("bit[44] == 1 -> w (r) !\r\n");
	__info("bit[40] == 1 -> reg16 (reg8) !\r\n");
	__info("bit[32:39] -> chip addr !\r\n");
	__info("bit[16:31] -> reg addr !\r\n");
	__info("bit[0:15] -> data !\r\n");

	__info("chip: 0x%x , data: 0x%x , reg: 0x%x , wr16_bit: 0x%x , wr_bit: 0x%x !\r\n",
		rm84_debug.device_addr, rm84_debug.data, rm84_debug.reg, rm84_debug.wr16_bit, rm84_debug.wr_bit);

	return count;
}

static struct class_attribute rm84_attrs[] = {
	__ATTR(reg, 0755, rm84_debug_show, rm84_debug_store),
};
#endif

int gpio_write_a16_d8(struct v4l2_subdev *sd, unsigned short chip_addr, addr_type reg,
		  data_type value)
{
	int ret = 0, cnt = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	client->addr = chip_addr;

	ret = cci_write_a16_d8(sd, reg, (unsigned char)value);
	while ((ret != 0) && cnt < 2) {
		ret = cci_write_a16_d8(sd, reg, (unsigned char)value);
		cnt++;
	}
	if (cnt > 0)
		pr_err("%s sensor write retry = %d\n", sd->name, cnt);

	return ret;
}

int gpio_write_a8_d8(struct v4l2_subdev *sd, unsigned short chip_addr, addr_type reg,
		   data_type value)
{
	int ret = 0, cnt = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	client->addr = chip_addr;

	ret = cci_write_a8_d8(sd, (unsigned char)reg, (unsigned char)value);
	while ((ret != 0) && cnt < 2) {
		ret = cci_write_a8_d8(sd, (unsigned char)reg, (unsigned char)value);
		cnt++;
	}
	if (cnt > 0)
		pr_err("%s sensor write retry = %d\n", sd->name, cnt);

	return ret;
}

__maybe_unused static unsigned int gpio_i2c_read(struct v4l2_subdev *sd, unsigned short chip_addr, int reg, int bit_flag)
{
	int ret;
	unsigned char val;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	client->addr = chip_addr;

	if (bit_flag == 0)
		ret = cci_read_a8_d8(sd, (unsigned char)reg, (unsigned char *)&val);
	else
		ret = cci_read_a16_d8(sd, reg, (unsigned char *)&val);

	return (unsigned int)val;
}

static unsigned int rm84_reg_write(struct v4l2_subdev *sd, rohm_reglist *reg)
{
	int ret = 0;

	if (reg->wr16 == 1) {
		ret = gpio_write_a16_d8(sd, reg->device_addr, reg->reg_addr, reg->data);
	} else {
		ret = gpio_write_a8_d8(sd, reg->device_addr, reg->reg_addr, reg->data);
	}

	if (reg->delay_flag) {
		usleep_range(10000, 20000);
	}

	return ret;
}

__maybe_unused static unsigned int rm84_reg_init(struct v4l2_subdev *sd, rohm_reglist *reg, int reg_size)
{
	int i = 0;

	for (i = 0; i < reg_size; i++) {
			rm84_reg_write(sd, &reg[i]);
	}

	printk("write reg 0x%x successful!, count %d !\r\n", reg[reg_size - 1].reg_addr, reg_size);

	return 0;
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
	int exp_val, gain_val;
	struct sensor_info *info = to_state(sd);

	exp_val = exp_gain->exp_val;
	gain_val = exp_gain->gain_val;

	info->exp = exp_val;
	info->gain = gain_val;
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
		vin_gpio_set_status(sd, RESET, 1);
		vin_set_mclk_freq(sd, MCLK);
		vin_set_mclk(sd, ON);
		usleep_range(1000, 1200);

		vin_set_pmu_channel(sd, CAMERAVDD, ON);
		vin_set_pmu_channel(sd, IOVDD, ON);
		vin_set_pmu_channel(sd, DVDD, ON);
		vin_set_pmu_channel(sd, AVDD, ON);
		usleep_range(1000, 1200);
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);/*CSI_GPIO_HIGH*/
		usleep_range(20000, 22000);
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(10000, 10200);  //delay 10ms
		cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF\n");
		cci_lock(sd);
		vin_set_mclk(sd, OFF);
		vin_gpio_set_status(sd, RESET, 1);
		vin_gpio_set_status(sd, RESET, CSI_GPIO_LOW);
		vin_set_pmu_channel(sd, IOVDD, OFF);
		vin_set_pmu_channel(sd, DVDD, OFF);
		vin_set_pmu_channel(sd, AVDD, OFF);
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
	info->width = 1280;
	info->height = 720;
	info->hflip = 0;
	info->vflip = 0;
	info->gain = 0;
	info->tpf.numerator = 1;
	info->tpf.denominator = 25;

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
		.regs 		= NULL,
		.regs_size = 0,
		.bpp		= 2,
	}
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
	  .pclk = 150*1000*1000,
	  .mipi_bps = 600*1000*1000,
	  .fps_fixed = 25,
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
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0 | V4L2_MBUS_CSI2_CHANNEL_1 | V4L2_MBUS_CSI2_CHANNEL_2 | V4L2_MBUS_CSI2_CHANNEL_3;
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

static int sensor_reg_init(struct sensor_info *info)
{
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	if (wsize->regs)
		rm84_reg_init(sd, wsize->regs, wsize->regs_size);

	if (wsize->set_size)
		wsize->set_size(sd);

	info->fmt = sensor_fmt;
	info->width = wsize->width;
	info->height = wsize->height;
	sensor_dbg("s_fmt set width = %d, height = %d, fps = %d\n", wsize->width,
		      wsize->height, wsize->fps_fixed);

	sensor_dbg("sensor_reg_init\n");


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
};

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.ioctl = sensor_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
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

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_ops *ops)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	struct v4l2_ctrl *ctrl;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 2);

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_GAIN, 1 * 1600,
			      256 * 1600, 1, 1 * 1600);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	ctrl = v4l2_ctrl_new_std(handler, ops, V4L2_CID_EXPOSURE, 0,
			      65536 * 16, 1, 0);
	if (ctrl != NULL)
		ctrl->flags |= V4L2_CTRL_FLAG_VOLATILE;

	if (handler->error) {
		ret = handler->error;
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
	__maybe_unused int i;
	__maybe_unused int err;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sensor_init_controls(sd, &sensor_ctrl_ops);
	mutex_init(&info->lock);
#ifdef __DEBUG_RM84
	rm84_class = class_create(THIS_MODULE, "rm84_class");
	if (IS_ERR(rm84_class)) {
		__err("%s:%u class_create() failed\n", __func__, __LINE__);
		return PTR_ERR(rm84_class);
	}

	/* sys/class/car_reverse/xxx */
	for (i = 0; i < ARRAY_SIZE(rm84_attrs); i++) {
		err = class_create_file(rm84_class, &rm84_attrs[i]);
		if (err) {
			pr_err("%s:%u class_create_file() failed. err=%d\n", __func__, __LINE__, err);
			while (i--) {
				class_remove_file(rm84_class, &rm84_attrs[i]);
			}
			class_destroy(rm84_class);
			rm84_class = NULL;
			return err;
		}
	}
#endif

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x20;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;

	rm84_sd = sd;

	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd;
	__maybe_unused int i;

	sd = cci_dev_remove_helper(client, &cci_drv);
#ifdef __DEBUG_RM84
	for (i = 0; i < ARRAY_SIZE(rm84_attrs); i++) {
		class_remove_file(rm84_class, &rm84_attrs[i]);
	}
	class_destroy(rm84_class);
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
	return cci_dev_init_helper(&sensor_driver);
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);

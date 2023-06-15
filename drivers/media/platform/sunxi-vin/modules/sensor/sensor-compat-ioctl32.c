
/*
 * ioctl32.c: Conversion between 32bit and 64bit native ioctls.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zhao Wei <zhaowei@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/compat.h>
#include <linux/module.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-ioctl.h>

#include "sensor_helper.h"

#ifdef CONFIG_COMPAT
static long native_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	return v4l2_subdev_call(sd, core, ioctl, cmd, arg);
}

struct sensor_config32 {
	int width;
	int height;
	unsigned int hoffset;	/*receive hoffset from sensor output*/
	unsigned int voffset;	/*receive voffset from sensor output*/
	unsigned int hts;	/*h size of timing, unit: pclk */
	unsigned int vts;	/*v size of timing, unit: line */
	unsigned int pclk;	/*pixel clock in Hz */
	unsigned int fps_fixed;	/*sensor fps */
	unsigned int bin_factor;/*binning factor */
	unsigned int intg_min;	/*integration min, unit: line, Q4 */
	unsigned int intg_max;	/*integration max, unit: line, Q4 */
	unsigned int gain_min;	/*sensor gain min, Q4 */
	unsigned int gain_max;	/*sensor gain max, Q4 */
	unsigned int mbus_code;	/*media bus code */
	unsigned int wdr_mode;	/*isp wdr mode */
};

struct sensor_exp_gain32 {
	int exp_val;
	int gain_val;
	int r_gain;
	int b_gain;
};

struct actuator_ctrl32 {
	unsigned int code;
};

struct actuator_para32 {
	unsigned short code_min;
	unsigned short code_max;
};

struct flash_para32 {
   enum v4l2_flash_led_mode mode;
};

#define VIDIOC_VIN_SENSOR_CFG_REQ32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 60, struct sensor_config32)

#define VIDIOC_VIN_SENSOR_EXP_GAIN32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 61, struct sensor_exp_gain32)
#define VIDIOC_VIN_ACT_SET_CODE32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 64, struct actuator_ctrl32)
#define VIDIOC_VIN_ACT_INIT32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 65, struct actuator_para32)
#define VIDIOC_VIN_FLASH_EN32 \
	_IOWR('V', BASE_VIDIOC_PRIVATE + 66, struct flash_para32)

static int get_sensor_config32(struct sensor_config *kp,
			      struct sensor_config32 __user *up)
{
	if (!access_ok(up, sizeof(struct sensor_config32)) ||
	    get_user(kp->width, &up->width) || get_user(kp->height, &up->height) ||
	    get_user(kp->hoffset, &up->hoffset) || get_user(kp->voffset, &up->voffset) ||
	    get_user(kp->hts, &up->hts) || get_user(kp->vts, &up->vts) ||
	    get_user(kp->pclk, &up->pclk) || get_user(kp->fps_fixed, &up->fps_fixed) ||
	    get_user(kp->bin_factor, &up->bin_factor) || get_user(kp->intg_min, &up->intg_min) ||
	    get_user(kp->intg_max, &up->intg_max) || get_user(kp->gain_min, &up->gain_min) ||
	    get_user(kp->gain_max, &up->gain_max) || get_user(kp->mbus_code, &up->mbus_code) ||
	    get_user(kp->wdr_mode, &up->wdr_mode))
		return -EFAULT;
	return 0;
}

static int put_sensor_config32(struct sensor_config *kp,
			      struct sensor_config32 __user *up)
{
	if (!access_ok(up, sizeof(struct sensor_config32)) ||
	    put_user(kp->width, &up->width) || put_user(kp->height, &up->height) ||
	    put_user(kp->hoffset, &up->hoffset) || put_user(kp->voffset, &up->voffset) ||
	    put_user(kp->hts, &up->hts) || put_user(kp->vts, &up->vts) ||
	    put_user(kp->pclk, &up->pclk) || put_user(kp->fps_fixed, &up->fps_fixed) ||
	    put_user(kp->bin_factor, &up->bin_factor) || put_user(kp->intg_min, &up->intg_min) ||
	    put_user(kp->intg_max, &up->intg_max) || put_user(kp->gain_min, &up->gain_min) ||
	    put_user(kp->gain_max, &up->gain_max) || put_user(kp->mbus_code, &up->mbus_code) ||
	    put_user(kp->wdr_mode, &up->wdr_mode))
		return -EFAULT;
	return 0;
}

static int get_sensor_exp_gain32(struct sensor_exp_gain *kp,
			      struct sensor_exp_gain32 __user *up)
{
	if (!access_ok(up, sizeof(struct sensor_exp_gain32)) ||
	    get_user(kp->exp_val, &up->exp_val) || get_user(kp->gain_val, &up->gain_val) ||
	    get_user(kp->r_gain, &up->r_gain) || get_user(kp->b_gain, &up->b_gain))
		return -EFAULT;
	return 0;
}

static int put_sensor_exp_gain32(struct sensor_exp_gain *kp,
			      struct sensor_exp_gain32 __user *up)
{
	if (!access_ok(up, sizeof(struct sensor_exp_gain32)) ||
	    put_user(kp->exp_val, &up->exp_val) || put_user(kp->gain_val, &up->gain_val) ||
	    put_user(kp->r_gain, &up->r_gain) || put_user(kp->b_gain, &up->b_gain))
		return -EFAULT;
	return 0;
}

static int get_act_init_config32(struct actuator_para *kp,
			      struct actuator_para32 __user *up)
{
	if (!access_ok(up, sizeof(struct actuator_para32)) ||
	    get_user(kp->code_min, &up->code_min) || get_user(kp->code_max, &up->code_max))
		return -EFAULT;
	return 0;
}

static int put_act_init_config32(struct actuator_para *kp,
			      struct actuator_para32 __user *up)
{
	if (!access_ok(up, sizeof(struct actuator_para32)) ||/*VERIFY_WRITE, */
	    put_user(kp->code_min, &up->code_min) || put_user(kp->code_max, &up->code_max))
		return -EFAULT;
	return 0;
}

static int get_act_code32(struct actuator_ctrl *kp,
			      struct actuator_ctrl32 __user *up)
{
	if (!access_ok(up, sizeof(struct actuator_ctrl32)) ||
	    get_user(kp->code, &up->code))
		return -EFAULT;
	return 0;
}

static int put_act_code32(struct actuator_ctrl *kp,
			      struct actuator_ctrl32 __user *up)
{
	if (!access_ok(up, sizeof(struct actuator_ctrl32)) ||/*VERIFY_WRITE, */
	    put_user(kp->code, &up->code))
		return -EFAULT;
	return 0;
}

static int get_flash_mode32(struct flash_para *kp,
			      struct flash_para32 __user *up)
{
	if (!access_ok(up, sizeof(struct flash_para32)) ||
	    get_user(kp->mode, &up->mode))
		return -EFAULT;
	return 0;
}

static int put_flash_mode32(struct flash_para *kp,
			      struct flash_para32 __user *up)
{
	if (!access_ok(up, sizeof(struct flash_para32)) ||
	    put_user(kp->mode, &up->mode))
		return -EFAULT;
	return 0;
}

long sensor_compat_ioctl32(struct v4l2_subdev *sd,
		unsigned int cmd, unsigned long arg)
{
	union {
		struct sensor_config sc;
		struct sensor_exp_gain seg;
		struct actuator_ctrl ctrl;
		struct actuator_para para;
		struct flash_para flash;
	} karg;
	void __user *up = compat_ptr(arg);
	int compatible_arg = 1;
	long err = 0;

	vin_log(VIN_LOG_ISP, "%s cmd is 0x%x\n", __func__, cmd);

	switch (cmd) {
	case VIDIOC_VIN_SENSOR_CFG_REQ32:
		cmd = VIDIOC_VIN_SENSOR_CFG_REQ;
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN32:
		cmd = VIDIOC_VIN_SENSOR_EXP_GAIN;
		break;
	case VIDIOC_VIN_ACT_INIT32:
		cmd = VIDIOC_VIN_ACT_INIT;
		break;
	case VIDIOC_VIN_ACT_SET_CODE32:
		cmd = VIDIOC_VIN_ACT_SET_CODE;
		break;
	case VIDIOC_VIN_FLASH_EN32:
		cmd = VIDIOC_VIN_FLASH_EN;
		break;
	}

	switch (cmd) {
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		err = get_sensor_config32(&karg.sc, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		err = get_sensor_exp_gain32(&karg.seg, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_ACT_INIT:
		err = get_act_init_config32(&karg.para, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_ACT_SET_CODE:
		err = get_act_code32(&karg.ctrl, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_FLASH_EN:
		err = get_flash_mode32(&karg.flash, up);
		compatible_arg = 0;
		break;

	}

	if (err)
		return err;

	if (compatible_arg)
		err = native_ioctl(sd, cmd, up);
	else {
		mm_segment_t old_fs = get_fs();

		set_fs(KERNEL_DS);
		err = native_ioctl(sd, cmd, &karg);
		set_fs(old_fs);
	}

	switch (cmd) {
	case VIDIOC_VIN_SENSOR_CFG_REQ:
		err = put_sensor_config32(&karg.sc, up);
		break;
	case VIDIOC_VIN_SENSOR_EXP_GAIN:
		err = put_sensor_exp_gain32(&karg.seg, up);
		break;
	case VIDIOC_VIN_ACT_INIT:
		err = put_act_init_config32(&karg.para, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_ACT_SET_CODE:
		err = put_act_code32(&karg.ctrl, up);
		compatible_arg = 0;
		break;
	case VIDIOC_VIN_FLASH_EN:
		err = put_flash_mode32(&karg.flash, up);
		compatible_arg = 0;
		break;
	}

	return err;
}
EXPORT_SYMBOL_GPL(sensor_compat_ioctl32);

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for tc358743xbg_mipi
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "../../../utility/vin_log.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/videodev2.h>
#include <linux/clk.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-dv-timings.h>
#include <uapi/linux/v4l2-dv-timings.h>
#include <linux/hdmi.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <asm/io.h>
#include <linux/kernel.h>

#include "../camera.h"
#include "../sensor_helper.h"
#include "tc358743xbg_reg.h"
#include "tc358743xbg_edid.h"

MODULE_AUTHOR("GC");
MODULE_DESCRIPTION("A low-level driver for tc358743xbg mipi chip for HDMI to MIPI/CSI");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

#define V4L2_IDENT_SENSOR 0x0000

/*
 * Our nominal (default) frame rate.
 */
#define SENSOR_FRAME_RATE 60

/*
 * The tc358743xbg_mipi i2c address
 */
#define I2C_ADDR 0x1f

#define SENSOR_NAME "tc358743xbg_mipi"

#define V4L2_EVENT_SRC_CH_HPD_IN             (1 << 1)
#define V4L2_EVENT_SRC_CH_HPD_OUT            (1 << 2)

 const struct v4l2_dv_timings_cap tc358743_timings_cap = {
	.type = V4L2_DV_BT_656_1120,
	.reserved = { 0 },

	//min_width,max_width,min_height,max_height,min_pixelclock,max_pixelclock,
	//standards,capabilities
	V4L2_INIT_BT_TIMINGS(640, 1920, 350, 1200, 13000000, 165000000,
		V4L2_DV_BT_STD_CEA861 | V4L2_DV_BT_STD_DMT |
		V4L2_DV_BT_STD_GTF | V4L2_DV_BT_STD_CVT,
		V4L2_DV_BT_CAP_PROGRESSIVE | V4L2_DV_BT_CAP_INTERLACED |
		V4L2_DV_BT_CAP_REDUCED_BLANKING |
		V4L2_DV_BT_CAP_CUSTOM)
};

static enum hotplut_state hotplut_status;

static void tc_msleep(unsigned int ms)
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(msecs_to_jiffies(ms));
}

void set_edid(struct v4l2_subdev *sd, const unsigned char *data)
{
	int i = 0;

	tc_write8(sd, EDID_MODE, 0x02);
	tc_write8(sd, EDID_LEN1, 0x00);
	tc_write8(sd, EDID_LEN2, 0x01);

	for (i = 0; i < 0x100; i++) {
		tc_write8(sd, 0x8C00 + i, data[i]);
	}
}

void tc_reg_init(struct v4l2_subdev *sd)
{
	// Initialization for Stand-by (RS1)
	// Software Reset
	tc_write16(sd, 0x0002, 0x0F00); // SysCtl
	tc_write16(sd, 0x0002, 0x0000); // SysCtl

	//REM Data Format
	tc_write16(sd, 0x0004, 0x0000);
	tc_write16(sd, 0x0010, 0x0000);

	// PLL Setting
	tc_write16(sd, 0x0020, 0x305F); // PLLCtl0
	tc_write16(sd, 0x0022, 0x0203); // PLLCtl1
	udelay(10);
	tc_write16(sd, 0x0022, 0x0213); // PLLCtl1
	// HDMI Interrupt Control
	tc_write16(sd, 0x0016, 0x073F); // TOP_INTM
	tc_write8(sd, 0x8502, 0xFF); // SYS_INTS_C
	tc_write8(sd, 0x850B, 0x1F); // MISC_INTS_C
	tc_write16(sd, 0x0014, 0x073F); // TOP_INTS_C
	tc_write8(sd, 0x8512, 0xFE); // SYS_INTM
	tc_write8(sd, 0x851B, 0x1D); // MISC_INTM
	// HDMI PHY
	tc_write8(sd, 0x8532, 0x80); // PHY CTL1
	tc_write8(sd, 0x8536, 0x40); // PHY_BIAS
	tc_write8(sd, 0x853F, 0x0A); // PHY_CSQ
	tc_write8(sd, 0x8537, 0x02); // PHY_EQ
	// HDMI SYSTEM
	tc_write8(sd, 0x8543, 0x32); // DDC_CTL
	tc_write8(sd, 0x8544, 0x10); // HPD_CTL
	tc_write8(sd, 0x8545, 0x31); // ANA_CTL
	tc_write8(sd, 0x8546, 0x2D); // AVM_CTL
	// HDCP Setting
	tc_write8(sd, 0x85D1, 0x01); //
	tc_write8(sd, 0x8560, 0x24); // HDCP_MODE
	tc_write8(sd, 0x8563, 0x11); //
	tc_write8(sd, 0x8564, 0x0F); //
	// HDMI Audio REFCLK
	tc_write8(sd, 0x8531, 0x01); // PHY_CTL0
	tc_write8(sd, 0x8532, 0x80); // PHY_CTL1
	tc_write8(sd, 0x8540, 0x8C); // SYS_FREQ0
	tc_write8(sd, 0x8541, 0x0A); // SYS_FREQ1
	tc_write8(sd, 0x8630, 0xB0); // LOCKDET_REF0
	tc_write8(sd, 0x8631, 0x1E); // LOCKDET_REF1
	tc_write8(sd, 0x8632, 0x04); // LOCKDET_REF2
	tc_write8(sd, 0x8670, 0x01); // NCO_F0_MOD
	// HDMI Audio Setting
	tc_write8(sd, 0x8600, 0x00); // AUD_Auto_Mute
	tc_write8(sd, 0x8602, 0xF3); // Auto_CMD0
	tc_write8(sd, 0x8603, 0x02); // Auto_CMD1
	tc_write8(sd, 0x8604, 0x0C); // Auto_CMD2
	tc_write8(sd, 0x8606, 0x05); // BUFINIT_START
	tc_write8(sd, 0x8607, 0x00); // FS_MUTE
	tc_write8(sd, 0x8620, 0x22); // FS_IMODE
	tc_write8(sd, 0x8640, 0x01); // ACR_MODE
	tc_write8(sd, 0x8641, 0x65); // ACR_MDF0
	tc_write8(sd, 0x8642, 0x07); // ACR_MDF1
	tc_write8(sd, 0x8652, 0x02); // SDO_MODE1
	tc_write8(sd, 0x85AA, 0x50); // FH_MIN0
	tc_write8(sd, 0x85AF, 0xC6); // HV_RST
	tc_write8(sd, 0x85AB, 0x00); // FH_MIN1
	tc_write8(sd, 0x8665, 0x10); // DIV_MODE
	// Info Frame Extraction
	tc_write8(sd, 0x8709, 0xFF); // PK_INT_MODE
	tc_write8(sd, 0x870B, 0x2C); // NO_PKT_LIMIT
	tc_write8(sd, 0x870C, 0x53); // NO_PKT_CLR
	tc_write8(sd, 0x870D, 0x01); // ERR_PK_LIMIT
	tc_write8(sd, 0x870E, 0x30); // NO_PKT_LIMIT2
	tc_write8(sd, 0x9007, 0x10); // NO_GDB_LIMIT

	//EDID
	set_edid(sd, edid_data);

	// Enable Interrupt
	tc_write16(sd, 0x0016, 0x053F); // TOP_INTM
	// Enter Sleep
	tc_write16(sd, 0x0002, 0x0001); // SysCtl
	// Interrupt Service Routine(RS_Int)
	// Exit from Sleep
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	udelay(10);
	// Check Interrupt
	tc_write16(sd, 0x0016, 0x073F); // TOP_INTM
	tc_write8(sd, 0x8502, 0xFF); // SYS_INTS_C
	tc_write8(sd, 0x850B, 0x1F); // MISC_INTS_C
	tc_write16(sd, 0x0014, 0x073F); // TOP_INTS_C
	// Initialization for Ready (RS2)
	// Enable Interrupt
	tc_write16(sd, 0x0016, 0x053F); // TOP_INTM
	// Set HPDO to "H"
	tc_write8(sd, 0x854A, 0x01); // INIT_END
	// Interrupt Service Routine(RS_Int)
	// Exit from Sleep
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	udelay(10);
	// Check Interrupt
	tc_write16(sd, 0x0016, 0x073F); // TOP_INTM
	tc_write8(sd, 0x8502, 0xFF); // SYS_INTS_C
	tc_write8(sd, 0x850B, 0x1F); // MISC_INTS_C
	tc_write16(sd, 0x0014, 0x073F); // TOP_INTS_C
	// MIPI Output Enable(RS3)

	// MIPI Output Setting
	// Stop Video and Audio
	tc_write16(sd, 0x0004, 0x0CD4); // ConfCtl
	// Reset CSI-TX Block, Enter Sleep mode
	tc_write16(sd, 0x0002, 0x0200); // SysCtl
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	// PLL Setting in Sleep mode, Int clear
	tc_write16(sd, 0x0002, 0x0001); // SysCtl
	tc_write16(sd, 0x0020, 0x305F); // PLLCtl0
	tc_write16(sd, 0x0022, 0x0203); // PLLCtl1
	udelay(10);
	tc_write16(sd, 0x0022, 0x0213); // PLLCtl1
	tc_write16(sd, 0x0002, 0x0000); // SysCtl
	udelay(10);
	// Video Setting
	tc_write8(sd, 0x8573, 0xC1); // VOUT_SET2
	tc_write8(sd, 0x8574, 0x08); // VOUT_SET3
	tc_write8(sd, 0x8576, 0xA0); // VI_REP
	// FIFO Delay Setting
	tc_write16(sd, 0x0006, 0x015E); // FIFO Ctl
	// Special Data ID Setting.
	// CSI Lane Enable
	tc_write32(sd, 0x0140, 0x00000000); // CLW_CNTRL
	tc_write32(sd, 0x0144, 0x00000000); // D0W_CNTRL
	tc_write32(sd, 0x0148, 0x00000000); // D1W_CNTRL
	tc_write32(sd, 0x014C, 0x00000000); // D2W_CNTRL
	tc_write32(sd, 0x0150, 0x00000000); // D3W_CNTRL
	// CSI Transition Timing
	tc_write32(sd, 0x0210, 0x00001004); // LINEINITCNT
	tc_write32(sd, 0x0214, 0x00000004); // LPTXTIMECNT
	tc_write32(sd, 0x0218, 0x00001603); // TCLK_HEADERCNT
	tc_write32(sd, 0x021C, 0x00000001); // TCLK_TRAILCNT
	tc_write32(sd, 0x0220, 0x00000204); // THS_HEADERCNT
	tc_write32(sd, 0x0224, 0x00004268); // TWAKEUP
	tc_write32(sd, 0x0228, 0x00000008); // TCLK_POSTCNT
	tc_write32(sd, 0x022C, 0x00000003); // THS_TRAILCNT
	tc_write32(sd, 0x0230, 0x00000005); // HSTXVREGCNT
	tc_write32(sd, 0x0234, 0x0000001F); // HSTXVREGEN
	tc_write32(sd, 0x0238, 0x00000000); // TXOPTIONACNTRL
	tc_write32(sd, 0x0204, 0x00000001); // STARTCNTRL
	tc_write32(sd, 0x0518, 0x00000001); // CSI_START
	tc_write32(sd, 0x0500, 0xA3008087); // CSI_CONFW
	// Enable Interrupt
	tc_write8(sd, 0x8502, 0xFF); // SYS_INTS_C
	tc_write8(sd, 0x8503, 0x7F); // CLK_INTS_C
	tc_write8(sd, 0x8504, 0xFF); // PACKET_INTS_C
	tc_write8(sd, 0x8505, 0xFF); // AUDIO_INTS_C
	tc_write8(sd, 0x8506, 0xFF); // ABUF_INTS_C
	tc_write8(sd, 0x850B, 0x1F); // MISC_INTS_C
	tc_write16(sd, 0x0014, 0x073F); // TOP_INTS_C
	tc_write16(sd, 0x0016, 0x053F); // TOP_INTM
	// Start CSI output
	tc_write16(sd, 0x0004, 0x0CD7); // ConfCtl
}

static int sensor_s_sw_stby(struct v4l2_subdev *sd, int on_off)
{
	return 0;
}

/*
 * Stuff that knows about the sensor.
 */
static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret = 0;

	sensor_dbg("%s\n", __func__);
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		ret = sensor_s_sw_stby(sd, STBY_ON);
		if (ret < 0)
			sensor_err("soft stby falied!\n");
		usleep_range(1000, 1200);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		usleep_range(1000, 1200);
		ret = sensor_s_sw_stby(sd, STBY_OFF);
		if (ret < 0)
			sensor_err("soft stby off falied!\n");
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON!\n");
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF!\n");
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_reset(struct v4l2_subdev *sd, u32 val)
{
	sensor_dbg("%s\n", __func__);
	switch (val) {
	case 0:
		vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		usleep_range(100, 120);
		break;
	case 1:
		vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		usleep_range(100, 120);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret, i = 0;
	int rdval = -1;

	ret = tc_reg_read(sd, CHIPID, &rdval, 4, TC_FLIP_EN);
	while ((V4L2_IDENT_SENSOR != rdval)) {
		i++;
		ret = tc_reg_read(sd, CHIPID, &rdval, 4, TC_FLIP_EN);
		if (i > 4) {
			sensor_print("warning:chip_id(%d) is NOT equal to %d\n",
					rdval, V4L2_IDENT_SENSOR);
			break;
		}
	}
	if (rdval != V4L2_IDENT_SENSOR)
		return -ENODEV;

	sensor_print("TX358743XBG ChipID = 0x%04x\n", rdval);
	return 0;
}

static int sensor_init(struct v4l2_subdev *sd, u32 val)
{
	int ret;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("sensor_init\n");

	/* Make sure it is a target sensor */
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
	info->tpf.numerator = 1;
	info->tpf.denominator = 30;

	return 0;
}

static long sensor_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	int ret = 0;
	struct sensor_info *info = to_state(sd);

	sensor_dbg("%s cmd:%d\n", __func__, cmd);
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
		.desc = "mipi",
		.mbus_code = MEDIA_BUS_FMT_UYVY8_2X8,
		.regs = NULL,
	},
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	 {
	.width         = 1024,
	.height        = 768,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },

	{
	.width         = 720,
	.height        = 480,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },
	{
	.width         = 720,
	.height        = 576,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 50,
	 },
	{
	.width         = 1280,
	.height        = 720,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },
	{
	.width         = 1280,
	.height        = 720,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 50,
	 },
	 {
	.width         = 1920,
	.height        = 1080,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },
	{
	.width         = 1920,
	.height        = 1080,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 50,
	 },
	{
	.width         = 1920,
	.height        = 544,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 60,
	 },
	 {
	.width         = 1920,
	.height        = 544,
	.hoffset       = 0,
	.voffset       = 0,
	.fps_fixed     = 50,
	 },
};

#define N_WIN_SIZES (ARRAY_SIZE(sensor_win_sizes))

static int sensor_g_mbus_config(struct v4l2_subdev *sd, unsigned int pad,
				struct v4l2_mbus_config *cfg)
{
	sensor_dbg("%s\n", __func__);
	cfg->type = V4L2_MBUS_CSI2_DPHY;
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;

	return 0;
}

static int tc_enum_dv_timings(struct v4l2_subdev *sd,
			struct v4l2_enum_dv_timings *timings)
{
	sensor_print("subdev feekback function:%s\n", __func__);
	if (timings->pad != 0)
		return -EINVAL;

	return v4l2_enum_dv_timings_cap(timings,
		&tc358743_timings_cap, NULL, NULL);
}

static int tc_query_dv_timings(struct v4l2_subdev *sd,
       struct v4l2_dv_timings *timings)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;

	sensor_print("subdev feekback function:%s\n", __func__);

	// tc_get_detected_timings(sd, timings);
	mutex_lock(&sensor_indet->detect_lock);
	memcpy(timings, &info->timings, sizeof(*timings));
	mutex_unlock(&sensor_indet->detect_lock);

	if (!v4l2_valid_dv_timings(timings, &tc358743_timings_cap, NULL, NULL)) {
		sensor_err("%s: invalid timings, timings out of range\n", __func__);
		return -ERANGE;
	}

	return 0;
}

static int tc_get_dv_timings(struct v4l2_subdev *sd, struct v4l2_dv_timings *timings)
{
	int cnt = 0;
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;

	sensor_print("subdev feekback function:%s\n", __func__);

	/* read the timings until to get a valid timing,
	 * e.g. until the tc358743 to get the hdmi signal stably
	 * over 3s, then timeout!
	 */
	while (cnt < 15) {
		// tc_get_detected_timings(sd, timings);
		mutex_lock(&sensor_indet->detect_lock);
		memcpy(timings, &info->timings, sizeof(*timings));
		mutex_unlock(&sensor_indet->detect_lock);

		if (v4l2_valid_dv_timings(timings, &tc358743_timings_cap, NULL, NULL))
			break;
		else
			sensor_err("%s: invalid timings, timings out of range\n", __func__);

		usleep_range(200000, 210000);
		cnt++;
	}

	return 0;
}

static int tc_dv_timings_cap(struct v4l2_subdev *sd,
		struct v4l2_dv_timings_cap *cap)
{
	sensor_print("subdev feekback function:%s\n", __func__);
	if (cap->pad != 0)
		return -EINVAL;

	*cap = tc358743_timings_cap;

	return 0;
}

static int sensor_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct sensor_info *info = to_state(sd);
	//struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	sensor_print("%s on = %d, %d*%d fps: %d code: %x\n", __func__, enable,
		info->current_wins->width, info->current_wins->height,
		info->current_wins->fps_fixed, info->fmt->mbus_code);

	info->width = wsize->width;
	info->height = wsize->height;

	return 0;
}

static int tc_set_edid(struct v4l2_subdev *sd, struct v4l2_subdev_edid *edid)
{
	sensor_print("subdev feekback function:%s\n", __func__);
	sensor_dbg("%s, pad %d, start block %d, blocks %d\n",
	    __func__, edid->pad, edid->start_block, edid->blocks);

	if (edid->edid && edid->blocks) {
		set_edid(sd, edid->edid);
	} else {
		sensor_err("%s, edid->edid or edid->blocks is NULL!!!\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int tc_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				struct v4l2_event_subscription *sub)
{
	sensor_print("subdev feekback function:%s\n", __func__);

	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		sensor_print("V4L2_EVENT_SOURCE_CHANGE\n");
		return v4l2_src_change_event_subdev_subscribe(sd, fh, sub);
	case V4L2_EVENT_CTRL:
		sensor_print("V4L2_EVENT_CTRL\n");
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	default:
		return -EINVAL;
	}
}

static int tc_unsubscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
					struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		break;
	case V4L2_EVENT_CTRL:
		break;
	default:
		return -EINVAL;
	}

	return v4l2_event_subdev_unsubscribe(sd, fh, sub);
}

static const struct v4l2_subdev_core_ops sensor_core_ops = {
	.reset = sensor_reset,
	.init = sensor_init,
	.s_power = sensor_power,
	.subscribe_event = tc_subscribe_event,
	.unsubscribe_event = tc_unsubscribe_event,
	.ioctl = sensor_ioctl,
#if IS_ENABLED(CONFIG_COMPAT)
	.compat_ioctl32 = sensor_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops sensor_video_ops = {
	.g_dv_timings = tc_get_dv_timings,
	.query_dv_timings = tc_query_dv_timings,
	.s_stream = sensor_s_stream,
};

static const struct v4l2_subdev_pad_ops sensor_pad_ops = {
	.enum_dv_timings = tc_enum_dv_timings,
	.dv_timings_cap = tc_dv_timings_cap,
	.enum_mbus_code = sensor_enum_mbus_code,
	.enum_frame_size = sensor_enum_frame_size,
	.enum_frame_interval = sensor_enum_frame_interval,
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.set_edid = tc_set_edid,
    .get_mbus_config = sensor_g_mbus_config,
};

static const struct v4l2_subdev_ops sensor_ops = {
	.core = &sensor_core_ops,
	.video = &sensor_video_ops,
	.pad = &sensor_pad_ops,
};

static ssize_t tc_reg_dump_all_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	ssize_t n = 0;
	unsigned short i = 0;

	n += sprintf(buf + n, "\nGlobal Control Register:\n");
	for (i = 0; i < 0x0100; i += 2) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%04x ", tc_read16(sd, i));
	}

	n += sprintf(buf + n, "\nCSI2 TX PHY/PPI  Register:\n");
	for (i = 0x0100; i < 0x023c; i += 4) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%08x ", tc_read32(sd, i));
	}

	n += sprintf(buf + n, "\nCSI2 TX Control  Register:\n");
	for (i = 0x040c; i < 0x051c; i += 4) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%08x ", tc_read32(sd, i));
	}

	n += sprintf(buf + n, "\nHDMIRX  Register:\n");
	for (i = 0x8500; i < 0x8844; i++) {
		if ((i % 16) == 0) {
			if (i != 0)
				n += sprintf(buf + n, "\n");
			n += sprintf(buf + n, "0x%04x:", i);
		}
		n += sprintf(buf + n, "0x%02x ", tc_read8(sd, i));
	}
	n += sprintf(buf + n, "\n");

	return n;
}

static u32 tc_reg_read_start, tc_reg_read_end;
static ssize_t tc_read_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;
	ssize_t n = 0;
	u32 reg_index;

	if ((tc_reg_read_start > tc_reg_read_end)
		|| (tc_reg_read_end - tc_reg_read_start > 0x4ff)
		|| (tc_reg_read_end > 0xffff)) {
		n += sprintf(buf + n, "invalid reg addr input:(0x%x, 0x%x)\n",
				tc_reg_read_start, tc_reg_read_end);
		return n;
	}

	for (reg_index = tc_reg_read_start; reg_index <= tc_reg_read_end;) {
		if (reg_index < 0x0100) { //Global Control Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%04x ", tc_read16(sd, reg_index));
			reg_index += 2;
		} else if (reg_index < 0x051c) { //CSI2 TX PHY/PPI  Register and CSI2 TX Control  Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%08x ", tc_read32(sd, reg_index));
			reg_index += 4;
		} else if ((reg_index >= 0x8500) && (reg_index < 0xffff)) { // HDMI RX Register
			if ((reg_index % 16) == 0) {
				if (reg_index != 0)
					n += sprintf(buf + n, "\n");
				n += sprintf(buf + n, "0x%04x:", reg_index);
			}
			n += sprintf(buf + n, "0x%02x ", tc_read8(sd, reg_index));
			reg_index++;
		}
	}

	n += sprintf(buf + n, "\n");

	return n;
}

static ssize_t tc_read_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	u8 *separator;

	separator = strchr(buf, ',');
	if (separator != NULL) {
		tc_reg_read_start = simple_strtoul(buf, NULL, 0);
		tc_reg_read_end = simple_strtoul(separator + 1, NULL, 0);
	} else {
		sensor_err("Invalid input!must add a comma as separator\n");
	}

	return count;
}

static u32 tc_reg_write_addr,  tc_reg_write_value;
static ssize_t tc_write_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	ssize_t n = 0;
	n += sprintf(buf + n, "echo [addr] [value] > tc_write\n");
	return n;
}

static ssize_t tc_write_store(struct device *dev,
			struct device_attribute *attr, const char *buf,
			size_t count)
{
	struct cci_driver *cci_drv = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = cci_drv->sd;

	u8 *separator;

	separator = strchr(buf, ' ');
	if (separator != NULL) {
		tc_reg_write_addr = simple_strtoul(buf, NULL, 0);
		tc_reg_write_value = simple_strtoul(separator + 1, NULL, 0);
	} else {
		sensor_err("Invalid input!must add a space as separator\n");
	}

	if (tc_reg_write_addr < 0x0100)
		tc_write16(sd, tc_reg_write_addr, tc_reg_write_value);
	else if (tc_reg_write_addr >= 0x0100 && tc_reg_write_addr < 0x051c)
		tc_write32(sd, tc_reg_write_addr, tc_reg_write_value);
	else if (tc_reg_write_addr >= 0x051c && tc_reg_write_addr <= 0xffff)
		tc_write8(sd, tc_reg_write_addr, tc_reg_write_value);
	return count;
}

static int __sensor_insert_detect(data_type *val)
{
	if (hotplut_status == HOTPLUT_DP_OUT) {
		sensor_print("hotplut status is hotplug dp out!\n");
		*val = 0x00;
	} else if (hotplut_status == HOTPLUT_DP_IN) {
		sensor_print("hotplut status is hotplug dp in!\n");
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

static struct device_attribute tc_device_attrs[] = {
	__ATTR(tc_read, S_IWUSR | S_IRUGO, tc_read_show, tc_read_store),
	__ATTR(tc_write, S_IWUSR | S_IRUGO, tc_write_show, tc_write_store),
	__ATTR(reg_dump_all, S_IRUGO, tc_reg_dump_all_show, NULL),
	__ATTR(online, S_IRUGO, get_det_status_show, NULL),
};

/* ----------------------------------------------------------------------- */
static struct cci_driver cci_drv = {
	.name = SENSOR_NAME,
	.addr_width = CCI_BITS_16,
	.data_width = CCI_BITS_16,
};

static bool tc_signal(struct v4l2_subdev *sd)
{
	return (tc_read8(sd, SYS_STATUS) & MASK_S_TMDS);
}

static unsigned int fps_calc(const struct v4l2_bt_timings *t)
{
	if (!V4L2_DV_BT_FRAME_HEIGHT(t) || !V4L2_DV_BT_FRAME_WIDTH(t))
		return 0;

	return DIV_ROUND_CLOSEST((unsigned int)t->pixelclock,
			V4L2_DV_BT_FRAME_HEIGHT(t) * V4L2_DV_BT_FRAME_WIDTH(t));
}

static int tc_get_detected_timings(struct v4l2_subdev *sd,
			struct v4l2_dv_timings *timings)
{
	struct v4l2_bt_timings *bt = &timings->bt;
	unsigned width, height, frame_width, frame_height, frame_interval, fps;

	memset(timings, 0, sizeof(struct v4l2_dv_timings));

	if (!tc_signal(sd))
		return -ENOLINK;

	timings->type = V4L2_DV_BT_656_1120;
	bt->interlaced = tc_read8(sd, VI_STATUS1) & MASK_S_V_INTERLACE ?
	V4L2_DV_INTERLACED : V4L2_DV_PROGRESSIVE;

	width = ((tc_read8(sd, DE_WIDTH_H_HI) & 0x1f) << 8) + tc_read8(sd, DE_WIDTH_H_LO);
	height = ((tc_read8(sd, DE_WIDTH_V_HI) & 0x1f) << 8) + tc_read8(sd, DE_WIDTH_V_LO);
	frame_width = ((tc_read8(sd, H_SIZE_HI) & 0x1f) << 8) + tc_read8(sd, H_SIZE_LO);
	frame_height = (((tc_read8(sd, V_SIZE_HI) & 0x3f) << 8) + tc_read8(sd, V_SIZE_LO)) / 2;
	/* frame interval in milliseconds * 10
	Require SYS_FREQ0 and SYS_FREQ1 are precisely set */
	frame_interval = ((tc_read8(sd, FV_CNT_HI) & 0x3) << 8) + tc_read8(sd, FV_CNT_LO);
	fps = (frame_interval > 0) ? DIV_ROUND_CLOSEST(10000, frame_interval) : 0;
	bt->width = width;
	bt->height = height;
	bt->vsync = frame_height - height;
	bt->hsync = frame_width - width;
	bt->pixelclock = frame_width * frame_height * fps;
	if (bt->interlaced == V4L2_DV_INTERLACED) {
		bt->height *= 2;
		bt->il_vsync = bt->vsync + 1;
		bt->pixelclock /= 2;
	}

	return 0;
}

static void sensor_uevent_notifiy(struct v4l2_subdev *sd, int w, int h, int fps, int status)
{
	char state[16], width[16], height[16], frame[16];
	char *envp[6] = {
		"SYSTEM=HDMIIN",
		NULL,
		NULL,
		NULL,
		NULL,
		NULL };
	snprintf(state, sizeof(state), "STATE=%d", status);
	snprintf(width, sizeof(width), "WIDTH=%d", w);
	snprintf(height, sizeof(height), "HEIGHT=%d", h);
	snprintf(frame, sizeof(frame), "FPS=%d", fps);
	envp[1] = width;
	envp[2] = height;
	envp[3] = frame;
	envp[4] = state;
	kobject_uevent_env(&sd->dev->kobj, KOBJ_CHANGE, envp);
}

static int tc_run_thread(void *parg)
{
	bool signal = 0;
	struct v4l2_event tc_ev_fmt;
	struct v4l2_subdev *sd = (struct v4l2_subdev *)parg;
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;
	struct v4l2_dv_timings timings;
	unsigned int change = 0;

	// wait for stability of timings (vertical active line/0x8588/8589) register.
	usleep_range(100000, 110000);

	while (1) {
		if (kthread_should_stop())
			break;

		change = 0;
		signal = tc_signal(sd);
		sensor_dbg("signal: %d\n", signal);
		if (sensor_indet->sensor_detect_flag != signal) {
			/*for anti signal-shake*/
			usleep_range(100000, 110000);
			if (signal != tc_signal(sd))
				continue;

			sensor_print("tmds signal:%d\n", signal);
			sensor_indet->sensor_detect_flag = signal;

			change |= signal ? V4L2_EVENT_SRC_CH_HPD_IN : V4L2_EVENT_SRC_CH_HPD_OUT;
		}

		memset(&timings, 0, sizeof(timings));
		tc_get_detected_timings(sd, &timings);
		sensor_dbg("tc_get_detected_timings: width = %d, height = %d, fps = %d, feild = %s\n",
					timings.bt.width, timings.bt.height,
					fps_calc(&timings.bt),
					timings.bt.interlaced == V4L2_DV_PROGRESSIVE ?
					"V4L2_DV_PROGRESSIVE" : "V4L2_DV_INTERLACED");

		mutex_lock(&sensor_indet->detect_lock);
		if (!v4l2_match_dv_timings(&info->timings, &timings, 0, false)) {
			sensor_print("tc resolution change!\n");

			change |= V4L2_EVENT_SRC_CH_RESOLUTION;

			// store the new timings
			memcpy(&info->timings, &timings, sizeof(timings));
		}
		mutex_unlock(&sensor_indet->detect_lock);

		// notify user-space the resolution_change event
		if (change) {
			if (v4l2_ctrl_s_ctrl(sensor_indet->ctrl_hotplug, sensor_indet->sensor_detect_flag) < 0) {
				sensor_err("v4l2_ctrl_s_ctrl for tmds signal failed!\n");
			}

			memset(&tc_ev_fmt, 0, sizeof(tc_ev_fmt));
			tc_ev_fmt.type = V4L2_EVENT_SOURCE_CHANGE;
			tc_ev_fmt.u.src_change.changes = change;
			v4l2_subdev_notify_event(sd, &tc_ev_fmt);

			if (change & V4L2_EVENT_SRC_CH_HPD_OUT) {
				hotplut_status = HOTPLUT_DP_OUT;
				sensor_uevent_notifiy(sd, 0, 0, 0, 0);
				sensor_print("%s send hotplug hdmi out to user\n", sd->name);
			} else if (change & V4L2_EVENT_SRC_CH_HPD_IN || change & V4L2_EVENT_SRC_CH_RESOLUTION) {
				hotplut_status = (change & V4L2_EVENT_SRC_CH_HPD_IN) ?
						HOTPLUT_DP_IN : HOTPLUT_DP_RESOLUTION;

				mutex_lock(&sensor_indet->detect_lock);
				sensor_uevent_notifiy(sd, info->timings.bt.width, info->timings.bt.height,
										fps_calc(&info->timings.bt), 1);
				sensor_print("send hotplug hdmi in to user, width = %d, height = %d, fps = %d, feild = %s\n",
					info->timings.bt.width, info->timings.bt.height,
					fps_calc(&info->timings.bt),
					info->timings.bt.interlaced == V4L2_DV_PROGRESSIVE ?
					"V4L2_DV_PROGRESSIVE" : "V4L2_DV_INTERLACED");
				mutex_unlock(&sensor_indet->detect_lock);
			}
		}
		tc_msleep(100);
	}
	return 0;
}

#define TC358743_CID_TMDS_SIGNAL (V4L2_CID_USER_TC358743_BASE + 3)
static const struct v4l2_ctrl_config tc358743_ctrl_tmds_signal_present = {
	.id = TC358743_CID_TMDS_SIGNAL,
	.name = "tmds signal",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
	.flags = V4L2_CTRL_FLAG_READ_ONLY,
};

static int sensor_det_init(struct v4l2_subdev *sd)
{
	//int height_H = -1, height_L = -1, height = -1;
	int ret;
	struct device_node *np = NULL;
	enum of_gpio_flags gc;
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;
	char *node_name = "sensor_detect";
	char *reset_gpio_name = "reset_gpios";

	np = of_find_node_by_name(NULL, node_name);
	if (np == NULL) {
		sensor_err("can not find the %s node\n", node_name);
		return -EINVAL;
	}

	/* reset sensor to detect hotplug*/
	sensor_indet->reset_gpio.gpio = of_get_named_gpio_flags(np, reset_gpio_name, 0, &gc);
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
		usleep_range(100000, 120000);
		gpio_direction_output(sensor_indet->reset_gpio.gpio, 1);
	}
	// software reset
	tc_write16(sd, SYSCTL, 0x0F00);
	tc_write16(sd, SYSCTL, 0x0000);

	//Create hdmi thread to poll hpd and hdcp status and handle hdcp and hpd event
	mutex_init(&sensor_indet->detect_lock);
	sensor_indet->detect_task = kthread_create(tc_run_thread, (void *)sd, "tc358743_proc");
	if (IS_ERR(sensor_indet->detect_task)) {
		sensor_err("Unable to start kernel thread  tc358743_proc\n");
		sensor_indet->detect_task = NULL;
		return -1;
	}
	wake_up_process(sensor_indet->detect_task);

	sensor_dbg("%s seccuss\n", __func__);
	return 0;
}

static void sensor_det_exit(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);
	struct sensor_indetect *sensor_indet = &info->sensor_indet;

	kthread_stop(sensor_indet->detect_task);
	mutex_destroy(&sensor_indet->detect_lock);

	if (gpio_is_valid(sensor_indet->reset_gpio.gpio))
		gpio_free(sensor_indet->reset_gpio.gpio);
}

static int sensor_init_controls(struct v4l2_subdev *sd, const struct v4l2_ctrl_config *config)
{
	struct sensor_info *info = to_state(sd);
	struct v4l2_ctrl_handler *handler = &info->handler;
	int ret = 0;

	v4l2_ctrl_handler_init(handler, 1);

	info->sensor_indet.ctrl_hotplug = v4l2_ctrl_new_custom(handler, config, NULL);

	if (handler->error) {
		ret = handler->error;
		v4l2_ctrl_handler_free(handler);
	}

	ret = v4l2_ctrl_handler_setup(handler);
	if (ret) {
		sensor_err("v4l2_ctrl_handler_setup!\n");
		return -1;
	}

	sd->ctrl_handler = handler;

	return ret;
}

static int sensor_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret, i;
	struct v4l2_subdev *sd;
	struct sensor_info *info;

	info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	if (info == NULL)
		return -ENOMEM;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);
	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS;
	sensor_init_controls(sd, &tc358743_ctrl_tmds_signal_present);

	mutex_init(&info->lock);
	dev_set_drvdata(&client->dev, info);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->sensor_field = V4L2_FIELD_NONE;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->combo_mode = CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x20;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;
#if IS_ENABLED(CONFIG_SAME_I2C)
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif

	sensor_det_init(sd);

	for (i = 0; i < ARRAY_SIZE(tc_device_attrs); i++) {
		ret = device_create_file(&cci_drv.cci_device, &tc_device_attrs[i]);
		if (ret) {
			sensor_err("device_create_file failed, index:%d\n", i);
			continue;
		}
	}

	// Initialize registers of tc358743xbg
	tc_reg_init(sd);

	sensor_dbg("sensor_probe end!\n");
	return 0;
}

static int sensor_remove(struct i2c_client *client)
{
	int i;
	struct v4l2_subdev *sd = NULL;

	if (client)
		sensor_det_exit(i2c_get_clientdata(client));
	else
		sensor_det_exit(cci_drv.sd);

	sd = cci_dev_remove_helper(client, &cci_drv);

	for (i = 0; i < ARRAY_SIZE(tc_device_attrs); i++)
		device_remove_file(&cci_drv.cci_device, &tc_device_attrs[i]);

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
	int ret = 0;

	sensor_dbg("init_sensor!\n");
	ret = cci_dev_init_helper(&sensor_driver);
	if (ret < 0)
		sensor_err("cci_dev_init_helper failed\n");

	return ret;
}

static __exit void exit_sensor(void)
{
	cci_dev_exit_helper(&sensor_driver);
}

VIN_INIT_DRIVERS(init_sensor);
module_exit(exit_sensor);

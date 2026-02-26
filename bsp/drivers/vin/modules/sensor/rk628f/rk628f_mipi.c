/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * A V4L2 driver for tp2815 cameras and TVI Coax protocol.
 *
 * Copyright (c) 2017 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  GuCheng <gucheng@allwinnertech.com>
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
#include <linux/v4l2-dv-timings.h>
#include <media/v4l2-device.h>
#include <media/v4l2-mediabus.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-event.h>
#include <linux/io.h>
#include "../camera.h"
#include "../sensor_helper.h"
#include "rk628_twi_helper.h"
#include "rk628.h"
#include "rk628_combtxphy.h"
#include "rk628_combrxphy.h"
#include "rk628_csi.h"
#include "rk628_cru.h"
#include "rk628_hdmirx.h"
#include "rk628_mipi_dphy.h"
#include "rk628_post_process.h"
#include "rk628_v4l2_controls.h"

#include "../../../utility/vin_io.h"

MODULE_AUTHOR("GC");
MODULE_DESCRIPTION("A low-level driver for RK628 mipi chip for HDMI to MIPI CSI");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");

// using external 24MHz crystal oscillator
// #define MCLK              (24*1000*1000)

#define V4L2_IDENT_SENSOR  0x20230321	// GRF_SOC_VERSION

#define I2C_ADDR  0xa0

#define SENSOR_NAME "rk628f_mipi"

#define RK_HDMIRX_V4L2_EVENT_SIGNAL_LOST (V4L2_EVENT_PRIVATE_START + 1)

#define RK628_EDID_DEBUG 0

#define RK628_I2S_ENABLE 1

static char dev_info[128] = "s:0-w:0-h:0-fps:0-r:0"; //state-width-height-fps-rate
module_param_string(dev_info, dev_info, sizeof(dev_info), S_IRUGO | S_IWUSR);

struct rk628_csi {
	struct device *dev;
	struct i2c_client *i2c_client;
	struct rk628 *rk628;
	struct media_pad pad;
	struct sensor_info info;
	// struct v4l2_subdev sd;
	struct v4l2_dv_timings src_timings;
	struct v4l2_dv_timings timings;
	struct v4l2_ctrl_handler hdl;
	struct v4l2_ctrl *detect_tx_5v_ctrl;
	struct v4l2_ctrl *audio_sampling_rate_ctrl;
	struct v4l2_ctrl *audio_present_ctrl;
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *pixel_rate;
	// struct gpio_config enable_gpio;
	struct gpio_config reset_gpio;
	struct gpio_config power_gpio;
	struct gpio_config plugin_det_gpio;
	struct gpio_config int_det_gpio;
	//struct clk *soc_24M;
	struct clk *clk_hdmirx_aud;
	struct clk *clk_vop;
	struct clk *clk_rx_read;
	struct delayed_work delayed_work_enable_hotplug;
	struct delayed_work delayed_work_res_change;
	struct work_struct work_isr;
	struct timer_list timer;
	struct work_struct work_i2c_poll;
	struct mutex confctl_mutex;
	// struct rkmodule_multi_dev_info multi_dev_info;
	// const struct rk628_csi_mode *cur_mode;
	// const char *module_facing;
	// const char *module_name;
	// const char *len_name;
	//u32 module_index;
	u8 edid_blocks_written;
	u64 lane_mbps;
	u8 csi_lanes_in_use;
	// u32 mbus_fmt_code;
	u8 fps;
	u32 stream_state;
	int hdmirx_irq;
	int plugin_irq;
	int avi_rdy;
	bool nosignal;
	bool rxphy_pwron;
	bool txphy_pwron;
	bool enable_hdcp;
	bool scaler_en;
	bool hpd_output_inverted;
	//bool hdmirx_det_inverted;
	bool avi_rcv_rdy;
	bool vid_ints_en;
	bool continues_clk;
	bool cec_enable;
	struct rk628_hdmirx_cec *cec;
	bool is_dvi;
	struct rk628_hdcp hdcp;
	bool i2s_enable_default;
	HAUDINFO audio_info;
	struct rk628_combtxphy *txphy;
	//struct rk628_dsi dsi;
	//const struct rk628_plat_data *plat_data;
	struct device *classdev;
	bool is_streaming;
	bool csi_ints_en;
};

static const s64 link_freq_menu_items[] = {
	RK628_CSI_LINK_FREQ_LOW,
	RK628_CSI_LINK_FREQ_HIGH,
	RK628_CSI_LINK_FREQ_925M,
};

static const struct v4l2_dv_timings_cap rk628_csi_timings_cap = {
	.type = V4L2_DV_BT_656_1120,
	/* keep this initialization for compatibility with GCC < 4.4.6 */
	.reserved = { 0 },
	V4L2_INIT_BT_TIMINGS(1, 10000, 1, 10000, 0, 600000000,
			V4L2_DV_BT_STD_CEA861 | V4L2_DV_BT_STD_DMT |
			V4L2_DV_BT_STD_GTF | V4L2_DV_BT_STD_CVT,
			V4L2_DV_BT_CAP_PROGRESSIVE | V4L2_DV_BT_CAP_INTERLACED |
			V4L2_DV_BT_CAP_REDUCED_BLANKING |
			V4L2_DV_BT_CAP_CUSTOM)
};

__maybe_unused static u8 edid_init_data[] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
	0x49, 0x73, 0x8D, 0x62, 0x00, 0x88, 0x88, 0x88,
	0x08, 0x1E, 0x01, 0x03, 0x80, 0x00, 0x00, 0x78,
	0x0A, 0x0D, 0xC9, 0xA0, 0x57, 0x47, 0x98, 0x27,
	0x12, 0x48, 0x4C, 0x00, 0x00, 0x00, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3A,
	0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
	0x45, 0x00, 0xC4, 0x8E, 0x21, 0x00, 0x00, 0x1E,
	0x01, 0x1D, 0x00, 0x72, 0x51, 0xD0, 0x1E, 0x20,
	0x6E, 0x28, 0x55, 0x00, 0xC4, 0x8E, 0x21, 0x00,
	0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x54,
	0x37, 0x34, 0x39, 0x2D, 0x66, 0x48, 0x44, 0x37,
	0x32, 0x30, 0x0A, 0x20, 0x00, 0x00, 0x00, 0xFD,
	0x00, 0x14, 0x78, 0x01, 0xFF, 0x1D, 0x00, 0x0A,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x18,

	0x02, 0x03, 0x1A, 0x71, 0x47, 0x5F, 0x90, 0x22,
	0x04, 0x11, 0x02, 0x01, 0x23, 0x09, 0x07, 0x01,
	0x83, 0x01, 0x00, 0x00, 0x65, 0x03, 0x0C, 0x00,
	0x10, 0x00, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38,
	0x2D, 0x40, 0x58, 0x2C, 0x45, 0x00, 0x20, 0xC2,
	0x31, 0x00, 0x00, 0x1E, 0x01, 0x1D, 0x00, 0x72,
	0x51, 0xD0, 0x1E, 0x20, 0x6E, 0x28, 0x55, 0x00,
	0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E, 0x02, 0x3A,
	0x80, 0xD0, 0x72, 0x38, 0x2D, 0x40, 0x10, 0x2C,
	0x45, 0x80, 0x20, 0xC2, 0x31, 0x00, 0x00, 0x1E,
	0x01, 0x1D, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40,
	0x58, 0x2C, 0x45, 0x00, 0xC0, 0x6C, 0x00, 0x00,
	0x00, 0x18, 0x01, 0x1D, 0x80, 0x18, 0x71, 0x1C,
	0x16, 0x20, 0x58, 0x2C, 0x25, 0x00, 0xC0, 0x6C,
	0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC1,
};

__maybe_unused static u8 rk628f_edid_init_data[] = {
	0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00,
	0x24, 0xD0, 0x8F, 0x62, 0x01, 0x00, 0x00, 0x00,
	0x2D, 0x21, 0x01, 0x03, 0x80, 0x78, 0x44, 0x78,
	0x0A, 0xCF, 0x74, 0xA3, 0x57, 0x4C, 0xB0, 0x23,
	0x09, 0x48, 0x4C, 0x21, 0x08, 0x00, 0x61, 0x40,
	0x01, 0x01, 0x81, 0x00, 0x95, 0x00, 0xA9, 0xC0,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x08, 0xE8,
	0x00, 0x30, 0xF2, 0x70, 0x5A, 0x80, 0xB0, 0x58,
	0x8A, 0x00, 0xC4, 0x8E, 0x21, 0x00, 0x00, 0x1E,
	0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40,
	0x58, 0x2C, 0x45, 0x00, 0xB9, 0xA8, 0x42, 0x00,
	0x00, 0x1E, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x49,
	0x46, 0x50, 0x20, 0x44, 0x69, 0x73, 0x70, 0x6C,
	0x61, 0x79, 0x0A, 0x20, 0x00, 0x00, 0x00, 0xFD,
	0x00, 0x3B, 0x46, 0x1F, 0x8C, 0x3C, 0x00, 0x0A,
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0xA8,

	0x02, 0x03, 0x39, 0xF2, 0x4D, 0x01, 0x03, 0x12,
	0x13, 0x84, 0x22, 0x1F, 0x90, 0x5D, 0x5E, 0x5F,
	0x60, 0x61, 0x23, 0x09, 0x07, 0x07, 0x83, 0x01,
	0x00, 0x00, 0x6D, 0x03, 0x0C, 0x00, 0x10, 0x00,
	0x00, 0x44, 0x20, 0x00, 0x60, 0x03, 0x02, 0x01,
	0x67, 0xD8, 0x5D, 0xC4, 0x01, 0x78, 0x80, 0x00,
	0xE3, 0x05, 0x03, 0x01, 0xE4, 0x0F, 0x00, 0x18,
	0x00, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D,
	0x40, 0x58, 0x2C, 0x45, 0x00, 0xB9, 0xA8, 0x42,
	0x00, 0x00, 0x1E, 0x08, 0xE8, 0x00, 0x30, 0xF2,
	0x70, 0x5A, 0x80, 0xB0, 0x58, 0x8A, 0x00, 0xC4,
	0x8E, 0x21, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD3,
};

__maybe_unused static u8 rk628_edid_gc[] = {
// 1080p 12bit color depth
	// Explore Semiconductor, Inc. EDID Editor V2
	0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00,   // address 0x00
	0x21, 0x57, 0x36, 0x18, 0xbd, 0xe9, 0x02, 0x00,   //
	0x25, 0x1d, 0x01, 0x03, 0x80, 0x35, 0x1d, 0x78,   // address 0x01
	0x62, 0xee, 0x91, 0xa3, 0x54, 0x4c, 0x99, 0x26,   //
	0x0f, 0x50, 0x54, 0x21, 0x0f, 0x00, 0x81, 0x00,   // address 0x02
	0x81, 0x40, 0x81, 0x80, 0x90, 0x40, 0x95, 0x00,   //
	0x01, 0x01, 0xa9, 0x40, 0xb3, 0x00, 0x02, 0x3a,   // address 0x03
	0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58, 0x2c,   //
	0x55, 0x00, 0xe0, 0x0e, 0x11, 0x00, 0x00, 0x5f,   // address 0x04
	0x01, 0x1d, 0x00, 0x72, 0x51, 0xd0, 0x1e, 0x20,   //
	0x6e, 0x28, 0x55, 0x00, 0x0f, 0x48, 0x42, 0x00,   // address 0x05
	0x00, 0x1c, 0x00, 0x00, 0x00, 0xfd, 0x00, 0x17,   //
	0x55, 0x1e, 0x64, 0x1e, 0x00, 0x0a, 0x20, 0x20,   // address 0x06
	0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xfc,   //
	0x00, 0x50, 0x61, 0x69, 0x43, 0x61, 0x73, 0x74,   // address 0x07
	0x0a, 0x20, 0x20, 0x20, 0x20, 0x20, 0x01, 0x2d,   //
	0x02, 0x03, 0x1f, 0xf1, 0x4a, 0x90, 0x04, 0x1f,   // address 0x08
	0x02, 0x03, 0x11, 0x05, 0x14, 0x13, 0x12, 0x23,   //
	0x09, 0x1f, 0x07, 0x83, 0x01, 0x00, 0x00, 0x67,   // address 0x09
	0x03, 0x0c, 0x00, 0x10, 0x00, 0x38, 0x3c, 0x02,   //
	0x3a, 0x80, 0x18, 0x71, 0x38, 0x2d, 0x40, 0x58,   // address 0x0a
	0x2c, 0x45, 0x00, 0xdd, 0x0c, 0x11, 0x00, 0x00,   //
	0x1e, 0x66, 0x21, 0x56, 0xaa, 0x51, 0x00, 0x1e,   // address 0x0b
	0x30, 0x46, 0x8f, 0x33, 0x00, 0x0f, 0x48, 0x42,   //
	0x00, 0x00, 0x1e, 0x8c, 0x0a, 0xd0, 0x8a, 0x20,   // address 0x0c
	0xe0, 0x2d, 0x10, 0x10, 0x3e, 0x96, 0x00, 0x10,   //
	0x09, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00,   // address 0x0d
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0x0e
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,   // address 0x0f
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd8,   //
};

static const struct mipi_timing rk628d_csi_mipi = {
	0x4a, 0xf, 0x5d, 0x3a, 0x3a, 0x5a, 0x1f
};

static const struct mipi_timing rk628f_csi0_mipi = {
	0x4a, 0xf, 0x5d, 0x3a, 0x3a, 0x5a, 0x1f
};

static const struct mipi_timing rk628f_csi1_mipi = {
//data-pre, data-zero, data-trail, clk-pre, clk-zero, clk-trail, clk-post
	0x4a, 0xf, 0x66, 0x3a, 0x3a, 0x5a, 0x1f
};

static const struct mipi_timing rk628f_dsi0_mipi = {
	0x70, 0x1c, 0x7f, 0x70, 0x3f, 0x7f, 0x1f
};

static struct v4l2_dv_timings dst_timing = {
	.type = V4L2_DV_BT_656_1120,
	.bt = {
		.interlaced = V4L2_DV_PROGRESSIVE,
		.width = 1920,
		.height = 1080,
		.hfrontporch = 88,
		.hsync = 44,
		.hbackporch = 148,
		.vfrontporch = 4,
		.vsync = 5,
		.vbackporch = 36,
		.pixelclock = 148500000,
	},
};

static void rk628_post_process_setup(struct v4l2_subdev *sd);
static void rk628_csi_enable_interrupts(struct v4l2_subdev *sd, bool en);
static void rk628_csi_enable_csi_interrupts(struct v4l2_subdev *sd, bool en);
static void rk628_csi_clear_csi_interrupts(struct v4l2_subdev *sd);
//static int rk628_csi_s_ctrl_detect_tx_5v(struct v4l2_subdev *sd);
static int sensor_s_dv_timings(struct v4l2_subdev *sd, struct v4l2_dv_timings *timings);
static int sensor_set_edid(struct v4l2_subdev *sd, struct v4l2_subdev_edid *edid);
static int mipi_dphy_power_on(struct rk628_csi *csi);
static void mipi_dphy_power_off(struct rk628_csi *csi);
static int rk628_hdmirx_inno_phy_power_on(struct v4l2_subdev *sd);
static int rk628_hdmirx_inno_phy_power_off(struct v4l2_subdev *sd);
static int rk628_hdmirx_phy_setup(struct v4l2_subdev *sd);
static int rk628_csi_format_change(struct v4l2_subdev *sd);
static void enable_stream(struct v4l2_subdev *sd, bool enable);
static void rk628_hdmirx_vid_enable(struct v4l2_subdev *sd, bool en);
static void rk628_csi_set_csi(struct v4l2_subdev *sd);
static void rk628_hdmirx_hpd_ctrl(struct v4l2_subdev *sd, bool en);
static bool rk628_rcv_supported_res(struct v4l2_subdev *sd, u32 width, u32 height);
// static void rk628_dsi_set_scs(struct rk628_csi *csi);
// static void rk628_dsi_enable(struct v4l2_subdev *sd);
static void rk628_csi_disable_stream(struct v4l2_subdev *sd);

static inline struct rk628_csi *info_to_csi(struct sensor_info *info)
{
	return container_of(info, struct rk628_csi, info);
}

static inline struct rk628_csi *sd_to_csi(struct v4l2_subdev *sd)
{
	struct sensor_info *info = to_state(sd);
	return info_to_csi(info);
}

static bool tx_5v_power_present(struct v4l2_subdev *sd)
{
	bool ret;
	struct rk628_csi *csi = sd_to_csi(sd);

	ret = rk628_hdmirx_tx_5v_power_detect(&csi->plugin_det_gpio);
	sensor_dbg("%s: %d\n", __func__, ret);

	return ret;
}

static inline bool no_signal(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	sensor_dbg("%s no signal:%d\n", __func__, csi->nosignal);
	return csi->nosignal;
}

static inline bool audio_present(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	return rk628_hdmirx_audio_present(csi->audio_info);
}

static int get_audio_sampling_rate(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	if (no_signal(sd))
		return 0;

	return rk628_hdmirx_audio_fs(csi->audio_info);
}

static void rk628_hdmirx_ctrl_enable(struct v4l2_subdev *sd, int en)
{
	u32 mask;
	struct rk628_csi *csi = sd_to_csi(sd);

	if (en) {
		/* don't enable audio until N CTS updated */
		mask = HDMI_ENABLE_MASK;
		sensor_dbg("%s: %#x %d\n", __func__, mask, en);
		rk628_i2c_update_bits(csi->rk628, HDMI_RX_DMI_DISABLE_IF,
				   mask, HDMI_ENABLE(1) | AUD_ENABLE(1));
	} else {
		mask = AUD_ENABLE_MASK | HDMI_ENABLE_MASK;
		sensor_dbg("%s: %#x %d\n", __func__, mask, en);
		rk628_i2c_update_bits(csi->rk628, HDMI_RX_DMI_DISABLE_IF,
				   mask, HDMI_ENABLE(0) | AUD_ENABLE(0));
	}
}

static int rk628_csi_get_detected_timings(struct v4l2_subdev *sd,
				     struct v4l2_dv_timings *timings)
{
	struct rk628_csi *csi = sd_to_csi(sd);
	struct v4l2_bt_timings *bt = &timings->bt;
	int ret;

	ret = rk628_hdmirx_get_timings(csi->rk628, timings);
	if (ret)
		return ret;

	if (bt->pixelclock > 300000000 && csi->rk628->version >= RK628F_VERSION) {
		sensor_print("rk628f detect pixclk more than 300M, use dual mipi mode\n");
		csi->rk628->dual_mipi = true;
	} else {
		sensor_print("pixclk less than 300M, use single mipi mode\n");
		csi->rk628->dual_mipi = false;
	}

	sensor_dbg("hfp:%d, hs:%d, hbp:%d, vfp:%d, vs:%d, vbp:%d, interlace:%d\n",
		 bt->hfrontporch, bt->hsync, bt->hbackporch, bt->vfrontporch, bt->vsync,
		 bt->vbackporch, bt->interlaced);

	csi->src_timings = *timings;
	if (csi->scaler_en)
		*timings = csi->timings;

	return ret;
}

static void rk628_hdmirx_plugout(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	enable_stream(sd, false);
	csi->nosignal = true;
	rk628_csi_enable_interrupts(sd, false);
	cancel_delayed_work(&csi->delayed_work_res_change);
	rk628_hdmirx_audio_cancel_work_audio(csi->audio_info, true);
	rk628_hdmirx_hpd_ctrl(sd, false);
	rk628_hdmirx_inno_phy_power_off(sd);
	rk628_hdmirx_controller_reset(csi->rk628);
}

static void rk628_hdmirx_config_all(struct v4l2_subdev *sd)
{
	int ret;
	struct rk628_csi *csi = sd_to_csi(sd);

	ret = rk628_hdmirx_phy_setup(sd);
	if (ret >= 0 && !rk628_hdmirx_scdc_ced_err(csi->rk628)) {
		ret = rk628_csi_format_change(sd);
		if (!ret) {
			csi->nosignal = false;
			return;
		}
	}

	if (ret < 0 || rk628_hdmirx_scdc_ced_err(csi->rk628)) {
		rk628_hdmirx_plugout(sd);
		schedule_delayed_work(&csi->delayed_work_enable_hotplug, msecs_to_jiffies(800));
	}
}
static void sensor_uevent_notifiy(struct v4l2_subdev *sd, int w, int h, int fps, int status, int audiosamplingrate)
{
	char state[16], width[16], height[16], frame[16], audio_sampling_rate[32];
	char *envp[7] = {
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
	snprintf(audio_sampling_rate, sizeof(audio_sampling_rate), "RATE=%d", audiosamplingrate);
	envp[1] = width;
	envp[2] = height;
	envp[3] = frame;
	envp[4] = state;
	envp[5] = audio_sampling_rate;
	snprintf(dev_info, sizeof(dev_info), "s:%d-w:%d-h:%d-fps:%d-r:%d", status, w, h, fps, audiosamplingrate);
	kobject_uevent_env(&sd->dev->kobj, KOBJ_CHANGE, envp);
}

static inline unsigned int fps_calc(const struct v4l2_bt_timings *t);
static void rk628_csi_delayed_work_enable_hotplug(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct rk628_csi *csi = container_of(dwork, struct rk628_csi,
			delayed_work_enable_hotplug);
	struct v4l2_subdev *sd = &csi->info.sd;
	struct v4l2_dv_timings timings;
	bool plugin;

	mutex_lock(&csi->confctl_mutex);
	rk628_set_bg_enable(csi->rk628, false);
	csi->avi_rcv_rdy = false;
	plugin = tx_5v_power_present(sd);
	v4l2_ctrl_s_ctrl(csi->detect_tx_5v_ctrl, plugin);
	sensor_dbg("%s: 5v_det:%d\n", __func__, plugin);
	if (plugin) {
		rk628_csi_enable_interrupts(sd, false);
		rk628_hdmirx_audio_setup(csi->audio_info);
		rk628_hdmirx_set_hdcp(csi->rk628, &csi->hdcp, csi->enable_hdcp);
		//rk628_hdmirx_hpd_ctrl(sd, true);
		rk628_hdmirx_controller_setup(csi->rk628);
		rk628_hdmirx_hpd_ctrl(sd, true);
		rk628_hdmirx_config_all(sd);
		if (csi->cec && csi->cec->adap)
			rk628_hdmirx_cec_state_reconfiguration(csi->rk628, csi->cec);
		rk628_csi_enable_interrupts(sd, true);
		rk628_csi_get_detected_timings(sd, &timings);
		sensor_print("%sget h*v: %d*%d  fps:%d audio_sampling_rate:%d\n", sd->name,
			timings.bt.width, timings.bt.height,
			fps_calc(&timings.bt),
			get_audio_sampling_rate(sd));
		sensor_uevent_notifiy(sd, timings.bt.width, timings.bt.height, fps_calc(&timings.bt), 1, get_audio_sampling_rate(sd));
		sensor_print("%s send hotplug hdmi in to user\n", sd->name);
	} else {
		rk628_hdmirx_plugout(sd);
		sensor_uevent_notifiy(sd, 0, 0, 0, 0, 0);
		sensor_print("%s send hotplug hdmi out to user\n", sd->name);
	}
	mutex_unlock(&csi->confctl_mutex);
}

static int rk628_check_resulotion_change(struct v4l2_subdev *sd)
{
	u32 val;
	struct rk628_csi *csi = sd_to_csi(sd);
	u32 htotal, vtotal;
	u32 old_htotal, old_vtotal;
	struct v4l2_bt_timings *bt = &csi->src_timings.bt;

	if (csi->rk628->version >= RK628F_VERSION)
		return 1;

	rk628_i2c_read(csi->rk628, HDMI_RX_MD_HT1, &val);
	htotal = (val >> 16) & 0xffff;
	rk628_i2c_read(csi->rk628, HDMI_RX_MD_VTL, &val);
	vtotal = val & 0xffff;

	old_htotal = bt->hfrontporch + bt->hsync + bt->width + bt->hbackporch;
	old_vtotal = bt->vfrontporch + bt->vsync + bt->height + bt->vbackporch;

	sensor_dbg("new mode: %d x %d\n", htotal, vtotal);
	sensor_dbg("old mode: %d x %d\n", old_htotal, old_vtotal);

	if (htotal != old_htotal || vtotal != old_vtotal)
		return 1;

	return 0;
}

static void rk628_delayed_work_res_change(struct work_struct *work)
{
	struct delayed_work *dwork = to_delayed_work(work);
	struct rk628_csi *csi = container_of(dwork, struct rk628_csi,
			delayed_work_res_change);
	struct v4l2_subdev *sd = &csi->info.sd;
	bool plugin;

	mutex_lock(&csi->confctl_mutex);
	enable_stream(sd, false);
	csi->nosignal = true;
	csi->avi_rcv_rdy = false;
	plugin = tx_5v_power_present(sd);
	sensor_dbg("%s: 5v_det:%d\n", __func__, plugin);
	if (plugin) {
		if (rk628_check_resulotion_change(sd)) {
			sensor_dbg("res change, recfg ctrler and phy!\n");
			if (csi->rk628->version >= RK628F_VERSION) {
				rk628_csi_enable_interrupts(sd, false);
				rk628_hdmirx_audio_cancel_work_audio(csi->audio_info, true);
				rk628_hdmirx_hpd_ctrl(sd, false);
				rk628_hdmirx_inno_phy_power_off(sd);
				rk628_hdmirx_controller_reset(csi->rk628);
				schedule_delayed_work(&csi->delayed_work_enable_hotplug,
						      msecs_to_jiffies(1100));
			} else {
				rk628_hdmirx_audio_cancel_work_audio(csi->audio_info, true);
				rk628_hdmirx_inno_phy_power_off(sd);
				rk628_hdmirx_controller_reset(csi->rk628);
				rk628_hdmirx_audio_setup(csi->audio_info);
				rk628_hdmirx_set_hdcp(csi->rk628, &csi->hdcp, csi->enable_hdcp);
				rk628_hdmirx_controller_setup(csi->rk628);
				rk628_hdmirx_hpd_ctrl(sd, true);
				rk628_hdmirx_config_all(sd);
				rk628_csi_enable_interrupts(sd, true);
				rk628_i2c_update_bits(csi->rk628, GRF_SYSTEM_CON0,
						      SW_I2S_DATA_OEN_MASK,
						      SW_I2S_DATA_OEN(0));
			}
		} else {
			rk628_csi_format_change(sd);
			rk628_csi_enable_interrupts(sd, true);
		}
	}
	mutex_unlock(&csi->confctl_mutex);
}

static void rk628_hdmirx_hpd_ctrl(struct v4l2_subdev *sd, bool en)
{
	u8 en_level, set_level;
	struct rk628_csi *csi = sd_to_csi(sd);
	unsigned int rdval = 0xffffffff;

	sensor_dbg("%s: %sable, hpd invert:%d\n", __func__,
			en ? "en" : "dis", csi->hpd_output_inverted);
	en_level = csi->hpd_output_inverted ? 0 : 1;
	set_level = en ? en_level : !en_level;
	rk628_i2c_update_bits(csi->rk628, HDMI_RX_HDMI_SETUP_CTRL,
			HOT_PLUG_DETECT_MASK, HOT_PLUG_DETECT(set_level));

	if (csi->cec_enable && csi->cec)
		rk628_hdmirx_cec_hpd(csi->cec, en);

	read_rk628(sd, 0x00030000, &rdval);
	sensor_print("rk628_hdmirx_hpd_ctrl read RK628F HDMI_RX_HDMI_SETUP_CTRL = 0x%08x\n", rdval);
}


static int rk628_csi_s_ctrl_detect_tx_5v(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	return v4l2_ctrl_s_ctrl(csi->detect_tx_5v_ctrl, tx_5v_power_present(sd));
}

static int rk628_csi_s_ctrl_audio_sampling_rate(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	return v4l2_ctrl_s_ctrl(csi->audio_sampling_rate_ctrl, get_audio_sampling_rate(sd));
}

static int rk628_csi_s_ctrl_audio_present(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	return v4l2_ctrl_s_ctrl(csi->audio_present_ctrl, audio_present(sd));
}

static int rk628_csi_update_controls(struct v4l2_subdev *sd)
{
	int ret = 0;

	ret |= rk628_csi_s_ctrl_detect_tx_5v(sd);
	ret |= rk628_csi_s_ctrl_audio_sampling_rate(sd);
	ret |= rk628_csi_s_ctrl_audio_present(sd);

	return ret;
}

static void rk628_csi0_cru_reset(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	rk628_control_assert(csi->rk628, RGU_CSI);
	udelay(10);
	rk628_control_deassert(csi->rk628, RGU_CSI);
}

static void rk628_csi1_cru_reset(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	rk628_control_assert(csi->rk628, RGU_CSI1);
	udelay(10);
	rk628_control_deassert(csi->rk628, RGU_CSI1);
}

static void rk628_csi_soft_reset(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	rk628_i2c_write(csi->rk628, CSITX_SYS_CTRL0_IMD, 0x1);
	usleep_range(1000, 1100);
	rk628_i2c_write(csi->rk628, CSITX_SYS_CTRL0_IMD, 0x0);

	if (csi->rk628->version >= RK628F_VERSION) {
		rk628_i2c_write(csi->rk628, CSITX1_SYS_CTRL0_IMD, 0x1);
		usleep_range(1000, 1100);
		rk628_i2c_write(csi->rk628, CSITX1_SYS_CTRL0_IMD, 0x0);
	}
}

static void enable_csitx(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	//enable dphy1 and split mode
	rk628_i2c_update_bits(csi->rk628, GRF_SYSTEM_CON3, GRF_DPHY_CH1_EN_MASK,
			     csi->rk628->dual_mipi ? GRF_DPHY_CH1_EN(1) : 0);
	rk628_i2c_update_bits(csi->rk628, GRF_POST_PROC_CON, SW_SPLIT_EN,
			     csi->rk628->dual_mipi ? SW_SPLIT_EN : 0);
	rk628_csi_set_csi(sd);

	rk628_csi_soft_reset(sd);
	usleep_range(5000, 5500);
	//disabled csi state ints
	rk628_i2c_write(csi->rk628, CSITX_INTR_EN_IMD, 0x0fff0000);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_i2c_write(csi->rk628, CSITX1_INTR_EN_IMD, 0x0fff0000);

	rk628_i2c_update_bits(csi->rk628, CSITX_CSITX_EN,
					DPHY_EN_MASK |
					CSITX_EN_MASK,
					DPHY_EN(1) |
					CSITX_EN(1));
	rk628_i2c_write(csi->rk628, CSITX_CONFIG_DONE, CONFIG_DONE_IMD);
	if (csi->rk628->version >= RK628F_VERSION) {
		rk628_i2c_update_bits(csi->rk628, CSITX1_CSITX_EN,
					DPHY_EN_MASK |
					CSITX_EN_MASK,
					DPHY_EN(1) |
					CSITX_EN(1));
		rk628_i2c_write(csi->rk628, CSITX1_CONFIG_DONE, CONFIG_DONE_IMD);
	}

	rk628_i2c_write(csi->rk628, CSITX_ERR_INTR_CLR_IMD, 0xffffffff);
	if (csi->rk628->version <= RK628D_VERSION)
		rk628_i2c_update_bits(csi->rk628, CSITX_SYS_CTRL1,
				BYPASS_SELECT_MASK, BYPASS_SELECT(0));
	rk628_i2c_write(csi->rk628, CSITX_CONFIG_DONE, CONFIG_DONE_IMD);

	if (csi->rk628->version >= RK628F_VERSION) {
		rk628_i2c_write(csi->rk628, CSITX1_ERR_INTR_CLR_IMD, 0xffffffff);
		rk628_i2c_write(csi->rk628, CSITX1_CONFIG_DONE, CONFIG_DONE_IMD);
	}
}

static void rk628_csi_disable_stream(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	rk628_i2c_update_bits(csi->rk628, CSITX_CSITX_EN,
				DPHY_EN_MASK | CSITX_EN_MASK,
				DPHY_EN(0) | CSITX_EN(0));
	rk628_i2c_update_bits(csi->rk628, CSITX_SYS_CTRL3_IMD, CONT_MODE_CLK_CLR_MASK,
			csi->continues_clk ? CONT_MODE_CLK_CLR(1) : CONT_MODE_CLK_CLR(0));
	rk628_i2c_write(csi->rk628, CSITX_CONFIG_DONE, CONFIG_DONE);

	if (csi->rk628->version >= RK628F_VERSION) {
		rk628_i2c_update_bits(csi->rk628, CSITX1_CSITX_EN,
					DPHY_EN_MASK | CSITX_EN_MASK,
					DPHY_EN(0) | CSITX_EN(0));
		rk628_i2c_update_bits(csi->rk628, CSITX1_SYS_CTRL3_IMD, CONT_MODE_CLK_CLR_MASK,
				csi->continues_clk ? CONT_MODE_CLK_CLR(1) : CONT_MODE_CLK_CLR(0));
		rk628_i2c_write(csi->rk628, CSITX1_CONFIG_DONE, CONFIG_DONE);
	}
}

static void enable_stream(struct v4l2_subdev *sd, bool en)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	sensor_dbg("%s: %sable\n", __func__, en ? "en" : "dis");
	if (en) {
		if (rk628_hdmirx_scdc_ced_err(csi->rk628)) {
			rk628_hdmirx_plugout(sd);
			schedule_delayed_work(&csi->delayed_work_enable_hotplug,
					      msecs_to_jiffies(800));
			return;
		}

		rk628_hdmirx_vid_enable(sd, true);
		enable_csitx(sd);
		/* csitx int en */
		rk628_csi_enable_csi_interrupts(sd, true);
		csi->is_streaming = true;
	} else {
		rk628_csi_enable_csi_interrupts(sd, false);
		msleep(20);
		rk628_hdmirx_vid_enable(sd, false);
		rk628_csi_disable_stream(sd);
		csi->is_streaming = false;
	}
}

static void rk628_post_process_setup(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);
	struct v4l2_bt_timings *bt = &csi->src_timings.bt;
	struct v4l2_bt_timings *dst_bt = &csi->timings.bt;
	struct videomode src, dst;
	u64 dst_pclk;

	src.hactive = bt->width;
	src.hfront_porch = bt->hfrontporch;
	src.hsync_len = bt->hsync;
	src.hback_porch = bt->hbackporch;
	src.vactive = bt->height;
	src.vfront_porch = bt->vfrontporch;
	src.vsync_len = bt->vsync;
	src.vback_porch = bt->vbackporch;
	src.pixelclock = bt->pixelclock;
	src.flags = 0;
	if (bt->interlaced == V4L2_DV_INTERLACED)
		src.flags |= DISPLAY_FLAGS_INTERLACED;
	if (!src.pixelclock) {
		enable_stream(sd, false);
		csi->nosignal = true;
		schedule_delayed_work(&csi->delayed_work_enable_hotplug, HZ / 20);
		return;
	}

	dst.hactive = dst_bt->width;
	dst.hfront_porch = dst_bt->hfrontporch;
	dst.hsync_len = dst_bt->hsync;
	dst.hback_porch = dst_bt->hbackporch;
	dst.vactive = dst_bt->height;
	dst.vfront_porch = dst_bt->vfrontporch;
	dst.vsync_len = dst_bt->vsync;
	dst.vback_porch = dst_bt->vbackporch;
	dst.pixelclock = dst_bt->pixelclock;

	rk628_post_process_en(csi->rk628, &src, &dst, &dst_pclk);
	dst_bt->pixelclock = dst_pclk;
}

static void rk628_csi_set_csi(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);
	u8 video_fmt;
	u8 lanes = csi->csi_lanes_in_use;
	u8 lane_num;
	u32 wc_usrdef, val;

	lane_num = lanes - 1;
	csi->rk628->dphy_lane_en = (1 << (lanes + 1)) - 1;
	wc_usrdef = csi->timings.bt.width * 2;
	if (csi->rk628->dual_mipi)
		wc_usrdef = csi->timings.bt.width;
	sensor_print("%s mipi mode, word count user define: %d\n", csi->rk628->dual_mipi ? "dual" : "single", wc_usrdef);
	rk628_csi_disable_stream(sd);
	usleep_range(5000, 5500);
	rk628_csi0_cru_reset(sd);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_csi1_cru_reset(sd);
	rk628_mipi_dphy_reset(csi->rk628);
	rk628_post_process_setup(sd);

	if (csi->txphy_pwron) {
		sensor_dbg("%s: txphy already power on, power off\n", __func__);
		mipi_dphy_power_off(csi);
		csi->txphy_pwron = false;
	}

	mipi_dphy_power_on(csi);
	csi->txphy_pwron = true;
	sensor_dbg("%s: txphy power on!\n", __func__);
	usleep_range(1000, 1500);

	if (csi->rk628->version <= RK628D_VERSION) {
		rk628_i2c_update_bits(csi->rk628, CSITX_CSITX_EN,
			VOP_UV_SWAP_MASK |
			VOP_YUV422_EN_MASK |
			VOP_P2_EN_MASK |
			LANE_NUM_MASK |
			DPHY_EN_MASK |
			CSITX_EN_MASK,
			VOP_UV_SWAP(1) |
			VOP_YUV422_EN(1) |
			VOP_P2_EN(1) |
			LANE_NUM(lane_num) |
			DPHY_EN(0) |
			CSITX_EN(0));
		rk628_i2c_update_bits(csi->rk628, CSITX_SYS_CTRL1,
			BYPASS_SELECT_MASK,
			BYPASS_SELECT(1));
	} else {
		rk628_i2c_update_bits(csi->rk628, CSITX_CSITX_EN,
			VOP_UV_SWAP_MASK |
			VOP_YUV422_EN_MASK |
			VOP_YUV422_MODE_MASK |
			VOP_P2_EN_MASK |
			LANE_NUM_MASK |
			DPHY_EN_MASK |
			CSITX_EN_MASK,
			VOP_UV_SWAP(0) |
			VOP_YUV422_EN(1) |
			VOP_YUV422_MODE(2) |
			VOP_P2_EN(1) |
			LANE_NUM(lane_num) |
			DPHY_EN(0) |
			CSITX_EN(0));
		rk628_i2c_update_bits(csi->rk628, CSITX_SYS_CTRL1,
			BYPASS_SELECT_MASK,
			BYPASS_SELECT(0));
	}

	rk628_i2c_write(csi->rk628, CSITX_CONFIG_DONE, CONFIG_DONE_IMD);
	rk628_i2c_write(csi->rk628, CSITX_SYS_CTRL2, VOP_WHOLE_FRM_EN | VSYNC_ENABLE);
	if (csi->continues_clk)
		rk628_i2c_update_bits(csi->rk628, CSITX_SYS_CTRL3_IMD,
			CONT_MODE_CLK_CLR_MASK |
			CONT_MODE_CLK_SET_MASK |
			NON_CONTINUOUS_MODE_MASK,
			CONT_MODE_CLK_CLR(0) |
			CONT_MODE_CLK_SET(1) |
			NON_CONTINUOUS_MODE(0));
	else
		rk628_i2c_update_bits(csi->rk628, CSITX_SYS_CTRL3_IMD,
			CONT_MODE_CLK_CLR_MASK |
			CONT_MODE_CLK_SET_MASK |
			NON_CONTINUOUS_MODE_MASK,
			CONT_MODE_CLK_CLR(0) |
			CONT_MODE_CLK_SET(0) |
			NON_CONTINUOUS_MODE(1));

	rk628_i2c_write(csi->rk628, CSITX_VOP_PATH_CTRL,
			VOP_WC_USERDEFINE(wc_usrdef) |
			VOP_DT_USERDEFINE(YUV422_8BIT) |
			VOP_PIXEL_FORMAT(0) |
			VOP_WC_USERDEFINE_EN(1) |
			VOP_DT_USERDEFINE_EN(1) |
			VOP_PATH_EN(1));
	rk628_i2c_update_bits(csi->rk628, CSITX_DPHY_CTRL,
				CSI_DPHY_EN_MASK,
				CSI_DPHY_EN(csi->rk628->dphy_lane_en));
	rk628_i2c_update_bits(csi->rk628, CSITX_VOP_FILTER_CTRL,
				VOP_FILTER_EN_MASK | VOP_FILTER_MASK,
				VOP_FILTER_EN(1) | VOP_FILTER(3));
	rk628_i2c_write(csi->rk628, CSITX_CONFIG_DONE, CONFIG_DONE_IMD);
	sensor_dbg("%s csi config done\n", __func__);

	if (csi->rk628->version >= RK628F_VERSION) {
		rk628_i2c_update_bits(csi->rk628, CSITX1_CSITX_EN,
				VOP_UV_SWAP_MASK |
				VOP_YUV422_EN_MASK |
				VOP_YUV422_MODE_MASK |
				VOP_P2_EN_MASK |
				LANE_NUM_MASK |
				DPHY_EN_MASK |
				CSITX_EN_MASK,
				VOP_UV_SWAP(0) |
				VOP_YUV422_EN(1) |
				VOP_YUV422_MODE(2) |
				VOP_P2_EN(1) |
				LANE_NUM(lane_num) |
				DPHY_EN(0) |
				CSITX_EN(0));
		rk628_i2c_update_bits(csi->rk628, CSITX1_SYS_CTRL1,
				BYPASS_SELECT_MASK,
				BYPASS_SELECT(0));
		rk628_i2c_write(csi->rk628, CSITX1_CONFIG_DONE, CONFIG_DONE_IMD);
		rk628_i2c_write(csi->rk628, CSITX1_SYS_CTRL2, VOP_WHOLE_FRM_EN | VSYNC_ENABLE);
		if (csi->continues_clk)
			rk628_i2c_update_bits(csi->rk628, CSITX1_SYS_CTRL3_IMD,
				CONT_MODE_CLK_CLR_MASK |
				CONT_MODE_CLK_SET_MASK |
				NON_CONTINUOUS_MODE_MASK,
				CONT_MODE_CLK_CLR(0) |
				CONT_MODE_CLK_SET(1) |
				NON_CONTINUOUS_MODE(0));
		else
			rk628_i2c_update_bits(csi->rk628, CSITX1_SYS_CTRL3_IMD,
				CONT_MODE_CLK_CLR_MASK |
				CONT_MODE_CLK_SET_MASK |
				NON_CONTINUOUS_MODE_MASK,
				CONT_MODE_CLK_CLR(0) |
				CONT_MODE_CLK_SET(0) |
				NON_CONTINUOUS_MODE(1));

		rk628_i2c_write(csi->rk628, CSITX1_VOP_PATH_CTRL,
				VOP_WC_USERDEFINE(wc_usrdef) |
				VOP_DT_USERDEFINE(YUV422_8BIT) |
				VOP_PIXEL_FORMAT(0) |
				VOP_WC_USERDEFINE_EN(1) |
				VOP_DT_USERDEFINE_EN(1) |
				VOP_PATH_EN(1));
		rk628_i2c_update_bits(csi->rk628, CSITX1_DPHY_CTRL,
					CSI_DPHY_EN_MASK,
					CSI_DPHY_EN(csi->rk628->dphy_lane_en));
		rk628_i2c_update_bits(csi->rk628, CSITX1_VOP_FILTER_CTRL,
				VOP_FILTER_EN_MASK | VOP_FILTER_MASK,
				VOP_FILTER_EN(1) | VOP_FILTER(3));
		rk628_i2c_write(csi->rk628, CSITX1_CONFIG_DONE, CONFIG_DONE_IMD);
		sensor_dbg("%s csi1 config done\n", __func__);
	}

	mutex_lock(&csi->confctl_mutex);
	csi->avi_rdy = rk628_is_avi_ready(csi->rk628, csi->avi_rcv_rdy);
	mutex_unlock(&csi->confctl_mutex);

	rk628_i2c_read(csi->rk628, HDMI_RX_PDEC_AVI_PB, &val);
	video_fmt = (val & VIDEO_FORMAT_MASK) >> 5;
	sensor_dbg("%s PDEC_AVI_PB:%#x, video format:%d\n",
			__func__, val, video_fmt);
	if (csi->rk628->version == RK628D_VERSION) {
		if (video_fmt) {
			/* yuv data: cfg SW_YUV2VYU_SWP */
			rk628_i2c_write(csi->rk628, GRF_CSC_CTRL_CON,
				SW_YUV2VYU_SWP(1) |
				SW_R2Y_EN(0));
		} else {
			/* rgb data: cfg SW_R2Y_EN */
			rk628_i2c_write(csi->rk628, GRF_CSC_CTRL_CON,
				SW_YUV2VYU_SWP(0) |
				SW_R2Y_EN(1) | SW_R2Y_CSC_MODE(2));
		}
	} else {
		rk628_i2c_write(csi->rk628, GRF_CSC_CTRL_CON, SW_YUV2VYU_SWP(1));
		rk628_post_process_csc_en(csi->rk628);
	}
	/* if avi packet is not stable, reset ctrl*/
	if (!csi->avi_rdy) {
		csi->nosignal = true;
		schedule_delayed_work(&csi->delayed_work_enable_hotplug, HZ / 20);
	}
}

static int rk628_hdmirx_inno_phy_power_on(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);
	int ret, f;

	sensor_print("%s begin... csi->rxphy_pwron: %s", __func__, csi->rxphy_pwron ? "true" : "false");

	/* Bit31 is used to distinguish HDMI cable mode and direct connection
	 * mode in the rk628_combrxphy driver.
	 * Bit31: 0 -direct connection mode;
	 *        1 -cable mode;
	 * The cable mode is to know the input clock frequency through cdr_mode
	 * in the rk628_combrxphy driver, and the cable mode supports up to
	 * 297M, so 297M is passed uniformly here.
	 */
	f = 297000 | BIT(31);

	if (csi->rxphy_pwron) {
		sensor_dbg("rxphy already power on, power off!\n");
		ret = rk628_rxphy_power_off(csi->rk628);
		if (ret)
			sensor_err("hdmi rxphy power off failed!\n");
		else
			csi->rxphy_pwron = false;
		usleep_range(100, 110);
	}

	if (csi->rxphy_pwron == false) {
		rk628_hdmirx_ctrl_enable(sd, 0);
		ret = rk628_rxphy_power_on(csi->rk628, f);
		if (ret) {
			csi->rxphy_pwron = false;
			sensor_err("hdmi rxphy power on failed\n");
		} else {
			csi->rxphy_pwron = true;
		}
		rk628_hdmirx_ctrl_enable(sd, 1);
		msleep(100);
	}

	return ret;
}

static int rk628_hdmirx_inno_phy_power_off(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	if (csi->rk628->version >= RK628F_VERSION)
		return 0;

	if (csi->rxphy_pwron) {
		sensor_dbg("rxphy power off!\n");
		rk628_rxphy_power_off(csi->rk628);
		csi->rxphy_pwron = false;
	}
	usleep_range(100, 100);

	return 0;
}

static void rk628_hdmirx_vid_enable(struct v4l2_subdev *sd, bool en)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	sensor_dbg("%s: %sable\n", __func__, en ? "en" : "dis");
	if (en) {
		if (!csi->i2s_enable_default)
			rk628_hdmirx_audio_i2s_ctrl(csi->audio_info, true);
		rk628_i2c_update_bits(csi->rk628, HDMI_RX_DMI_DISABLE_IF,
				      VID_ENABLE_MASK, VID_ENABLE(1));
	} else {
		if (!csi->i2s_enable_default)
			rk628_hdmirx_audio_i2s_ctrl(csi->audio_info, false);
		rk628_i2c_update_bits(csi->rk628, HDMI_RX_DMI_DISABLE_IF,
				      VID_ENABLE_MASK, VID_ENABLE(0));
	}
}

static bool rk628_rcv_supported_res(struct v4l2_subdev *sd, u32 width,
		u32 height)
{
	// u32 i;
	struct rk628_csi *csi = sd_to_csi(sd);

	if (csi->rk628->version >= RK628F_VERSION)
		return true;

	return true;

	// for (i = 0; i < ARRAY_SIZE(supported_modes); i++) {
	// 	if ((supported_modes[i].width == width) &&
	// 	    (supported_modes[i].height == height)) {
	// 		break;
	// 	}
	// }
	// if (i == ARRAY_SIZE(supported_modes)) {
	// 	sensor_err("%s do not support res wxh: %dx%d\n", __func__, width, height);
	// 	return false;
	// } else {
	// 	return true;
	// }
}

static int rk628_hdmirx_phy_setup(struct v4l2_subdev *sd)
{
	u32 i, cnt, val;
	u32 width, height, frame_width, frame_height, status;
	struct rk628_csi *csi = sd_to_csi(sd);
	int ret = 0;

	for (i = 0; i < RXPHY_CFG_MAX_TIMES; i++) {
		if (csi->rk628->version < RK628F_VERSION)
			ret = rk628_hdmirx_inno_phy_power_on(sd);
		else
			rk628_hdmirx_verisyno_phy_power_on(csi->rk628);
		if (ret < 0) {
			msleep(50);
			continue;
		}
		cnt = 0;

		do {
			msleep(20);
			cnt++;
			rk628_i2c_read(csi->rk628, HDMI_RX_MD_HACT_PX, &val);
			width = val & 0xffff;	// 0x780 (1920)
			rk628_i2c_read(csi->rk628, HDMI_RX_MD_VAL, &val);
			height = val & 0xffff;	// 0x870 (2160)
			rk628_i2c_read(csi->rk628, HDMI_RX_MD_HT1, &val);
			frame_width = (val >> 16) & 0xffff;	// 0x898 (2200)
			rk628_i2c_read(csi->rk628, HDMI_RX_MD_VTL, &val);
			frame_height = val & 0xffff;		// 0x8ca (2250)
			rk628_i2c_read(csi->rk628, HDMI_RX_SCDC_REGS1, &val);	// 0xffff0f00
			status = val;
			sensor_dbg("%s read wxh:%dx%d, total:%dx%d, SCDC_REGS1:%#x, cnt:%d\n",
				 __func__, width, height, frame_width, frame_height, status, cnt);

			rk628_i2c_read(csi->rk628, HDMI_RX_PDEC_STS, &val);
			if (csi->rk628->version < RK628F_VERSION && (val & DVI_DET)) {
				csi->is_dvi = true;
				dev_info(csi->dev, "DVI mode detected\n");
			} else {
				csi->is_dvi = false;
			}

			if (!tx_5v_power_present(sd)) {
				sensor_print("HDMI pull out, return!\n");
				return -1;
			}

			if (cnt >= 15)
				break;
		} while (((status & 0xfff) < 0xf00) || (!rk628_rcv_supported_res(sd, width, height)));

		if (((status & 0xfff) < 0xf00) || (!rk628_rcv_supported_res(sd, width, height))) {
			sensor_err("%s hdmi rxphy lock failed, retry:%d\n", __func__, i);
			continue;
		} else {
			if (csi->rk628->version >= RK628F_VERSION)
				rk628_hdmirx_phy_prepclk_cfg(csi->rk628);
			break;
		}
	}

	if (i == RXPHY_CFG_MAX_TIMES)
		return -1;

	return 0;
}

static void rk628_csi_initial(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);
	struct v4l2_subdev_edid def_edid;
	u32 mask = SW_OUTPUT_MODE_MASK;
	u32 val = SW_OUTPUT_MODE(OUTPUT_MODE_CSI);

	/* select int io function */
	rk628_i2c_write(csi->rk628, GRF_GPIO3AB_SEL_CON, 0x30002000);
	rk628_i2c_write(csi->rk628, GRF_GPIO1AB_SEL_CON, HIWORD_UPDATE(0xf, 11, 8));
	/* I2S_SCKM0 */
	rk628_i2c_write(csi->rk628, GRF_GPIO0AB_SEL_CON, HIWORD_UPDATE(0x1, 2, 2));
	/* I2SLR_M0 */
	rk628_i2c_write(csi->rk628, GRF_GPIO0AB_SEL_CON, HIWORD_UPDATE(0x1, 3, 3));
	/* I2SM0D0 */
	rk628_i2c_write(csi->rk628, GRF_GPIO0AB_SEL_CON, HIWORD_UPDATE(0x1, 5, 4));
	/* hdmirx int en */
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_i2c_write(csi->rk628, GRF_INTR0_EN, 0x02000200);
	else
		rk628_i2c_write(csi->rk628, GRF_INTR0_EN, 0x01000100);

	udelay(10);
	rk628_control_assert(csi->rk628, RGU_HDMIRX);
	rk628_control_assert(csi->rk628, RGU_HDMIRX_PON);
	rk628_control_assert(csi->rk628, RGU_CSI);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_control_assert(csi->rk628, RGU_CSI1);
	udelay(10);
	rk628_control_deassert(csi->rk628, RGU_HDMIRX);
	rk628_control_deassert(csi->rk628, RGU_HDMIRX_PON);
	rk628_control_deassert(csi->rk628, RGU_CSI);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_control_deassert(csi->rk628, RGU_CSI1);
	udelay(10);

	if (csi->rk628->version >= RK628F_VERSION) {
		mask = SW_OUTPUT_COMBTX_MODE_MASK;
		val = SW_OUTPUT_COMBTX_MODE(OUTPUT_MODE_CSI - 1);
	}
	rk628_i2c_update_bits(csi->rk628, GRF_SYSTEM_CON0,
			SW_INPUT_MODE_MASK |
			mask |
			SW_EFUSE_HDCP_EN_MASK |
			SW_HSYNC_POL_MASK |
			SW_VSYNC_POL_MASK |
			SW_I2S_DATA_OEN_MASK,
			SW_INPUT_MODE(INPUT_MODE_HDMI) |
			val |
			SW_EFUSE_HDCP_EN(0) |
			SW_HSYNC_POL(1) |
			SW_VSYNC_POL(1) |
			SW_I2S_DATA_OEN(0));
	rk628_hdmirx_controller_reset(csi->rk628);

	def_edid.pad = 0;
	def_edid.start_block = 0;
	def_edid.blocks = 2;
	if (csi->rk628->version >= RK628F_VERSION)
		//def_edid.edid = rk628f_edid_init_data;
		//def_edid.edid = rk628_edid_gc;
		def_edid.edid = edid_init_data;
	else
		def_edid.edid = edid_init_data;
	sensor_set_edid(sd, &def_edid);
	rk628_hdmirx_set_hdcp(csi->rk628, &csi->hdcp, false);
}

static void rk628_csi_initial_setup(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	rk628_csi_initial(sd);
	if (csi->rk628->version >= RK628F_VERSION) {
		csi->rk628->mipi_timing[0] = rk628f_csi0_mipi;
		csi->rk628->mipi_timing[1] = rk628f_csi1_mipi;
	} else {
		csi->rk628->mipi_timing[0] = rk628d_csi_mipi;
	}

	csi->rk628->dphy_lane_en = 0x1f;
	rk628_mipi_dphy_reset(csi->rk628);
	mipi_dphy_power_on(csi);
	csi->txphy_pwron = true;
	if (tx_5v_power_present(sd))
		schedule_delayed_work(&csi->delayed_work_enable_hotplug, msecs_to_jiffies(4000));
}

static int rk628_csi_format_change(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);
	struct v4l2_dv_timings timings;
	const struct v4l2_event rk628_csi_ev_fmt = {
		.type = V4L2_EVENT_SOURCE_CHANGE,
		.u.src_change.changes = V4L2_EVENT_SRC_CH_RESOLUTION,
	};
	int ret;

	ret = rk628_csi_get_detected_timings(sd, &timings);
	if (ret) {
		sensor_dbg("%s: get timing fail\n", __func__);
		return ret;
	}
	if (!v4l2_match_dv_timings(&csi->timings, &timings, 0, false)) {
		/* automatically set timing rather than set by userspace */
		sensor_s_dv_timings(sd, &timings);
		v4l2_print_dv_timings(sd->name,
				"rk628_csi_format_change: New format: ",
				&timings, false);
	}

	if (sd->devnode)
		v4l2_subdev_notify_event(sd, &rk628_csi_ev_fmt);

	return 0;
}

static void rk628_csi_enable_csi_interrupts(struct v4l2_subdev *sd, bool en)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	rk628_csi_clear_csi_interrupts(sd);
	//disabled csi state ints
	rk628_i2c_write(csi->rk628, CSITX_INTR_EN_IMD, 0x0fff0000);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_i2c_write(csi->rk628, CSITX1_INTR_EN_IMD, 0x0fff0000);

	//enable csi error ints
	if (en) {
		if (csi->rk628->version >= RK628F_VERSION) {
			rk628_i2c_update_bits(csi->rk628,
					GRF_INTR0_EN, CSI_INT_EN_MASK | CSI_INT_WRITE_EN_MASK,
					CSI_INT_EN(3) | CSI_INT_WRITE_EN(3));
			rk628_i2c_write(csi->rk628, CSITX_ERR_INTR_EN_IMD, 0x0fff0fff);
			rk628_i2c_write(csi->rk628, CSITX1_ERR_INTR_EN_IMD, 0x0fff0fff);
		} else {
			rk628_i2c_update_bits(csi->rk628,
					GRF_INTR0_EN, CSI_INT_EN_MASK | CSI_INT_WRITE_EN_MASK,
					CSI_INT_EN(1) | CSI_INT_WRITE_EN(1));
			rk628_i2c_write(csi->rk628, CSITX_ERR_INTR_EN_IMD, 0x0fff0fff);
		}
		csi->csi_ints_en = true;
	} else {
		if (csi->rk628->version >= RK628F_VERSION) {
			rk628_i2c_update_bits(csi->rk628,
					GRF_INTR0_EN, CSI_INT_EN_MASK | CSI_INT_WRITE_EN_MASK,
					CSI_INT_EN(0) | CSI_INT_WRITE_EN(3));
			rk628_i2c_write(csi->rk628, CSITX_ERR_INTR_EN_IMD, 0x0fff0000);
			rk628_i2c_write(csi->rk628, CSITX1_ERR_INTR_EN_IMD, 0x0fff0000);
		} else {
			rk628_i2c_update_bits(csi->rk628,
					GRF_INTR0_EN, CSI_INT_EN_MASK | CSI_INT_WRITE_EN_MASK,
					CSI_INT_EN(0) | CSI_INT_WRITE_EN(1));
			rk628_i2c_write(csi->rk628, CSITX_ERR_INTR_EN_IMD, 0x0fff0000);
		}
		csi->csi_ints_en = false;
	}
}

static void rk628_csi_enable_interrupts(struct v4l2_subdev *sd, bool en)
{
	u32 pdec_ien, md_ien;
	u32 pdec_mask = 0, md_mask = 0;
	struct rk628_csi *csi = sd_to_csi(sd);

	pdec_mask = AVI_RCV_ENSET | AVI_CKS_CHG_ICLR;
	md_mask = VACT_LIN_ENSET | HACT_PIX_ENSET | HS_CLK_ENSET |
		  DE_ACTIVITY_ENSET | VS_ACT_ENSET | HS_ACT_ENSET | VS_CLK_ENSET;
	sensor_dbg("%s: %sable\n", __func__, en ? "en" : "dis");
	/* clr irq */
	rk628_i2c_write(csi->rk628, HDMI_RX_MD_ICLR, md_mask);
	rk628_i2c_write(csi->rk628, HDMI_RX_PDEC_ICLR, pdec_mask);
	if (en) {
		rk628_i2c_write(csi->rk628, HDMI_RX_MD_IEN_SET, md_mask);
		rk628_i2c_write(csi->rk628, HDMI_RX_PDEC_IEN_SET, pdec_mask);
		csi->vid_ints_en = true;
	} else {
		rk628_i2c_write(csi->rk628, HDMI_RX_MD_IEN_CLR, md_mask);
		rk628_i2c_write(csi->rk628, HDMI_RX_PDEC_IEN_CLR, pdec_mask);
		rk628_i2c_write(csi->rk628, HDMI_RX_AUD_FIFO_IEN_CLR, 0x1f);
		if (csi->cec && csi->cec->adap) {
			rk628_i2c_write(csi->rk628, HDMI_RX_AUD_CEC_IEN_SET, 0);
			rk628_i2c_write(csi->rk628, HDMI_RX_AUD_CEC_IEN_CLR, ~0);
		}
		csi->vid_ints_en = false;
	}
	usleep_range(5000, 5000);
	rk628_i2c_read(csi->rk628, HDMI_RX_MD_IEN, &md_ien);
	rk628_i2c_read(csi->rk628, HDMI_RX_PDEC_IEN, &pdec_ien);
	sensor_dbg("%s MD_IEN:%#x, PDEC_IEN:%#x\n", __func__, md_ien, pdec_ien);
}

static void rk628_csi_clear_csi_interrupts(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	//clr int status
	rk628_i2c_write(csi->rk628, CSITX_ERR_INTR_CLR_IMD, 0xffffffff);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_i2c_write(csi->rk628, CSITX1_ERR_INTR_CLR_IMD, 0xffffffff);

	if (csi->rk628->version >= RK628F_VERSION)
		rk628_i2c_update_bits(csi->rk628, GRF_INTR0_CLR_EN, CSI_INT_EN_MASK |
				CSI_INT_WRITE_EN_MASK, CSI_INT_EN(3) | CSI_INT_WRITE_EN(3));
	else
		rk628_i2c_update_bits(csi->rk628, GRF_INTR0_CLR_EN, CSI_INT_EN_MASK |
				CSI_INT_WRITE_EN_MASK, CSI_INT_EN(1) | CSI_INT_WRITE_EN(1));
}

static void rk628_csi_error_process(struct v4l2_subdev *sd)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	if (csi->is_streaming) {
		sensor_print("%s: csitx is streaming, reset csitx and restart cstitx!\n", __func__);
		rk628_csi_enable_csi_interrupts(sd, false);
		rk628_i2c_update_bits(csi->rk628, CSITX_CSITX_EN, CSITX_EN_MASK, CSITX_EN(0));
		if (csi->rk628->version >= RK628F_VERSION)
			rk628_i2c_update_bits(csi->rk628, CSITX1_CSITX_EN,
						CSITX_EN_MASK, CSITX_EN(0));
		rk628_i2c_write(csi->rk628, CSITX_CONFIG_DONE, CONFIG_DONE);
		if (csi->rk628->version >= RK628F_VERSION)
			rk628_i2c_write(csi->rk628, CSITX1_CONFIG_DONE, CONFIG_DONE);

		usleep_range(5000, 5500);
		rk628_csi_soft_reset(sd);
		usleep_range(5000, 5500);

		rk628_i2c_update_bits(csi->rk628, CSITX_CSITX_EN, CSITX_EN_MASK, CSITX_EN(1));
		rk628_i2c_write(csi->rk628, CSITX_CONFIG_DONE, CONFIG_DONE_IMD);
		if (csi->rk628->version >= RK628F_VERSION) {
			rk628_i2c_update_bits(csi->rk628, CSITX1_CSITX_EN,
						CSITX_EN_MASK, CSITX_EN(1));
			rk628_i2c_write(csi->rk628, CSITX1_CONFIG_DONE, CONFIG_DONE_IMD);
		}
		//clr int status
		rk628_csi_clear_csi_interrupts(sd);
		rk628_csi_enable_csi_interrupts(sd, true);
	} else {
		sensor_print("%s: csitx is not streaming\n", __func__);
	}
}

static void rk628_work_isr(struct work_struct *work)
{
	struct rk628_csi *csi = container_of(work, struct rk628_csi, work_isr);
	struct v4l2_subdev *sd = &csi->info.sd;
	u32 md_ints, pdec_ints, fifo_ints, hact, vact;
	bool plugin;
	void *audio_info = csi->audio_info;
	bool handled = false;
	u32 csi0_raw_ints, csi1_raw_ints = 0x0;
	u32 int0_status;
	const struct v4l2_event evt_signal_lost = {
		.type = RK_HDMIRX_V4L2_EVENT_SIGNAL_LOST,
		//.type = V4L2_EVENT_SRC_CH_RESOLUTION,
	};

	mutex_lock(&csi->rk628->rst_lock);
	rk628_i2c_read(csi->rk628, GRF_INTR0_STATUS, &int0_status);
	sensor_dbg("%s: int0 status: 0x%x\n", __func__, int0_status);

	rk628_i2c_read(csi->rk628, HDMI_RX_MD_ISTS, &md_ints);
	rk628_i2c_read(csi->rk628, HDMI_RX_PDEC_ISTS, &pdec_ints);
	if (csi->rk628->version >= RK628F_VERSION &&
	    rk628_hdmirx_is_signal_change_ists(csi->rk628))
		rk628_set_bg_enable(csi->rk628, true);

	plugin = tx_5v_power_present(sd);
	if (!plugin) {
		rk628_csi_enable_interrupts(sd, false);
		rk628_csi_enable_csi_interrupts(sd, false);
		goto __clear_int;
	}

	if (csi->rk628->version < RK628F_VERSION) {
		if (rk628_audio_ctsnints_enabled(audio_info)) {
			if (pdec_ints & (ACR_N_CHG_ICLR | ACR_CTS_CHG_ICLR)) {
				rk628_csi_isr_ctsn(audio_info, pdec_ints);
				pdec_ints &= ~(ACR_CTS_CHG_ICLR | ACR_CTS_CHG_ICLR);
				handled = true;
			}
		}
		if (rk628_audio_fifoints_enabled(audio_info)) {
			rk628_i2c_read(csi->rk628, HDMI_RX_AUD_FIFO_ISTS, &fifo_ints);
			if (fifo_ints & 0x18) {
				rk628_csi_isr_fifoints(audio_info, fifo_ints);
				handled = true;
			}
		}
	}
	if (csi->vid_ints_en) {
		sensor_dbg("%s: md_ints: %#x, pdec_ints:%#x, plugin: %d\n",
			 __func__, md_ints, pdec_ints, plugin);

		if (rk628_hdmirx_is_signal_change_ists(csi->rk628)) {
			rk628_i2c_read(csi->rk628, HDMI_RX_MD_HACT_PX, &hact);
			rk628_i2c_read(csi->rk628, HDMI_RX_MD_VAL, &vact);
			sensor_dbg("%s: HACT:%#x, VACT:%#x\n",
				 __func__, hact, vact);

			rk628_csi_enable_interrupts(sd, false);
			if (csi->rk628->version < RK628F_VERSION) {
				enable_stream(sd, false);
				csi->nosignal = true;
			}
			v4l2_event_queue(sd->devnode, &evt_signal_lost);
			sensor_dbg("rk628_work_isr v4l2_event_queue evt_signal_lost: V4L2_EVENT_SRC_CH_RESOLUTION\n");
			schedule_delayed_work(&csi->delayed_work_res_change, msecs_to_jiffies(100));

			sensor_dbg("%s: hact/vact change, md_ints: %#x\n",
				 __func__, (u32)(md_ints & (VACT_LIN_ISTS | HACT_PIX_ISTS)));
			handled = true;
		}

		if ((pdec_ints & AVI_RCV_ISTS) && plugin && !csi->avi_rcv_rdy) {
			sensor_dbg("%s: AVI RCV INT!\n", __func__);
			csi->avi_rcv_rdy = true;
			/* After get the AVI_RCV interrupt state, disable interrupt. */
			rk628_i2c_write(csi->rk628, HDMI_RX_PDEC_IEN_CLR, AVI_RCV_ISTS);

			handled = true;
		}
	}

	if (int0_status & (BIT(6) | BIT(7))) {
		rk628_i2c_read(csi->rk628, CSITX_ERR_INTR_RAW_STATUS_IMD, &csi0_raw_ints);
		if (csi->rk628->version >= RK628F_VERSION)
			rk628_i2c_read(csi->rk628, CSITX1_ERR_INTR_RAW_STATUS_IMD, &csi1_raw_ints);

		if (csi0_raw_ints || csi1_raw_ints) {
			sensor_print("%s: csi interrupt: csi0_raw_ints: 0x%x, csi1_raw_ints: 0x%x!\n",
						__func__, csi0_raw_ints, csi1_raw_ints);
			rk628_csi_error_process(sd);
		}
		handled = true;
	}

	if (!handled)
		sensor_dbg("%s: unhandled interrupt!\n", __func__);

__clear_int:
	/* clear interrupts */
	rk628_i2c_write(csi->rk628, HDMI_RX_MD_ICLR, 0xffffffff);
	rk628_i2c_write(csi->rk628, HDMI_RX_PDEC_ICLR, 0xffffffff);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_i2c_write(csi->rk628, GRF_INTR0_CLR_EN, 0x02000200);
	else
		rk628_i2c_write(csi->rk628, GRF_INTR0_CLR_EN, 0x01000100);
	rk628_csi_clear_csi_interrupts(sd);

	mutex_unlock(&csi->rk628->rst_lock);
}


static int rk628_csi_isr(struct v4l2_subdev *sd, u32 status, bool *handled)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	if (handled == NULL) {
		sensor_err("handled NULL, err return!\n");
		return -EINVAL;
	}

	schedule_work(&csi->work_isr);

	return 0;
}

__maybe_unused static irqreturn_t rk628_csi_irq_handler(int irq, void *dev_id)
{
	struct rk628_csi *csi = dev_id;
	bool handled = true;

	rk628_csi_isr(&csi->info.sd, 0, &handled);

	if (csi->cec_enable && csi->cec)
		rk628_hdmirx_cec_irq(csi->rk628, csi->cec);

	return handled ? IRQ_HANDLED : IRQ_NONE;
}

__maybe_unused static void rk628_csi_irq_poll_timer(struct timer_list *t)
{
	struct rk628_csi *csi = from_timer(csi, t, timer);

	schedule_work(&csi->work_i2c_poll);
	mod_timer(&csi->timer, jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));
}

__maybe_unused static void rk628_csi_work_i2c_poll(struct work_struct *work)
{
	struct rk628_csi *csi = container_of(work, struct rk628_csi, work_i2c_poll);
	struct v4l2_subdev *sd = &csi->info.sd;

	rk628_csi_format_change(sd);
}

static inline unsigned int fps_calc(const struct v4l2_bt_timings *t)
{
	if (!V4L2_DV_BT_FRAME_HEIGHT(t) || !V4L2_DV_BT_FRAME_WIDTH(t))
		return 0;

	return DIV_ROUND_CLOSEST((unsigned int)t->pixelclock,
			V4L2_DV_BT_FRAME_HEIGHT(t) * V4L2_DV_BT_FRAME_WIDTH(t));
}

static int sensor_get_edid(struct v4l2_subdev *sd,
		struct v4l2_subdev_edid *edid)
{
	struct rk628_csi *csi = sd_to_csi(sd);
	u32 i, val;

	memset(edid->reserved, 0, sizeof(edid->reserved));

	if (edid->pad != 0)
		return -EINVAL;

	if (edid->start_block == 0 && edid->blocks == 0) {
		edid->blocks = csi->edid_blocks_written;
		return 0;
	}

	if (csi->edid_blocks_written == 0)
		return -ENODATA;

	if (edid->start_block >= csi->edid_blocks_written ||
			edid->blocks == 0)
		return -EINVAL;

	if (edid->start_block + edid->blocks > csi->edid_blocks_written)
		edid->blocks = csi->edid_blocks_written - edid->start_block;

	/* edid access by apb when read, i2c slave addr: 0x0 */
	rk628_i2c_update_bits(csi->rk628, GRF_SYSTEM_CON0,
			SW_ADAPTER_I2CSLADR_MASK |
			SW_EDID_MODE_MASK,
			SW_ADAPTER_I2CSLADR(0) |
			SW_EDID_MODE(1));

	for (i = 0; i < (edid->blocks * EDID_BLOCK_SIZE); i++) {
		rk628_i2c_read(csi->rk628, EDID_BASE + ((edid->start_block *
				EDID_BLOCK_SIZE) + i) * 4, &val);
		edid->edid[i] = val;
	}

	rk628_i2c_update_bits(csi->rk628, GRF_SYSTEM_CON0,
			SW_EDID_MODE_MASK,
			SW_EDID_MODE(0));

	return 0;
}

static int sensor_set_edid(struct v4l2_subdev *sd,
				struct v4l2_subdev_edid *edid)
{
	struct rk628_csi *csi = sd_to_csi(sd);
	u16 edid_len = edid->blocks * EDID_BLOCK_SIZE;
	__maybe_unused u32 i, val;

	sensor_dbg("%s, pad %d, start block %d, blocks %d\n",
		 __func__, edid->pad, edid->start_block, edid->blocks);

	memset(edid->reserved, 0, sizeof(edid->reserved));

	if (edid->pad != 0)
		return -EINVAL;

	if (edid->start_block != 0)
		return -EINVAL;

	if (edid->blocks > EDID_NUM_BLOCKS_MAX) {
		edid->blocks = EDID_NUM_BLOCKS_MAX;
		return -E2BIG;
	}

	rk628_hdmirx_hpd_ctrl(sd, false);

	if (edid->blocks == 0) {
		csi->edid_blocks_written = 0;
		return 0;
	}

	/* edid access by apb when write, i2c slave addr: 0x0 */
	rk628_i2c_update_bits(csi->rk628, GRF_SYSTEM_CON0,
			SW_ADAPTER_I2CSLADR_MASK |
			SW_EDID_MODE_MASK,
			SW_ADAPTER_I2CSLADR(0) |
			SW_EDID_MODE(1));

	for (i = 0; i < edid_len; i++)
		rk628_i2c_write(csi->rk628, EDID_BASE + i * 4, edid->edid[i]);

	/* read out for debug */
#if RK628_EDID_DEBUG
	sensor_print("%s: Read EDID: =============================\n", __func__);
	for (i = 0; i < edid_len; i++) {
		rk628_i2c_read(csi->rk628, EDID_BASE + i * 4, &val);
		printk(KERN_CONT "0x%02x ", val);

		if ((i + 1) % 8 == 0)
			printk("\n");
	}
	sensor_print("%s: =============================\n", __func__);
#endif

	/* edid access by RX's i2c, i2c slave addr: 0x0 */
	rk628_i2c_update_bits(csi->rk628, GRF_SYSTEM_CON0,
			SW_ADAPTER_I2CSLADR_MASK |
			SW_EDID_MODE_MASK,
			SW_ADAPTER_I2CSLADR(0) |
			SW_EDID_MODE(0));
	csi->edid_blocks_written = edid->blocks;
	udelay(100);

	if (tx_5v_power_present(sd) && csi->rk628->version < RK628F_VERSION)
		rk628_hdmirx_hpd_ctrl(sd, true);

	return 0;
}

static int mipi_dphy_power_on(struct rk628_csi *csi)
{
	unsigned int val;
	u32 bus_width, mask;

	if (csi->timings.bt.pixelclock > 150000000 || csi->csi_lanes_in_use <= 2) {
		csi->lane_mbps = MIPI_DATARATE_MBPS_HIGH;
	} else {
		csi->lane_mbps = MIPI_DATARATE_MBPS_LOW;
	}
	if (csi->rk628->dual_mipi)
		csi->lane_mbps = MIPI_DATARATE_MBPS_HIGH;
	bus_width =  csi->lane_mbps << 8;
	bus_width |= COMBTXPHY_MODULEA_EN;
	if (csi->rk628->version >= RK628F_VERSION)
		bus_width |= COMBTXPHY_MODULEB_EN;
	sensor_dbg("%s mipi bitrate:%llu mbps\n", __func__,
			csi->lane_mbps);
	rk628_txphy_set_bus_width(csi->rk628, bus_width);
	rk628_txphy_set_mode(csi->rk628, PHY_MODE_VIDEO_MIPI);

	rk628_mipi_dphy_init_hsfreqrange(csi->rk628, csi->lane_mbps, 0);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_mipi_dphy_init_hsfreqrange(csi->rk628, csi->lane_mbps, 1);

	if (csi->rk628->dual_mipi) {
		rk628_mipi_dphy_init_hsmanual(csi->rk628, true, 0);
		rk628_mipi_dphy_init_hsmanual(csi->rk628, true, 1);
	} else if (csi->lane_mbps == MIPI_DATARATE_MBPS_HIGH && !csi->rk628->dual_mipi) {
		rk628_mipi_dphy_init_hsmanual(csi->rk628, true, 0);
		rk628_mipi_dphy_init_hsmanual(csi->rk628, false, 1);
	} else {
		rk628_mipi_dphy_init_hsmanual(csi->rk628, false, 0);
		rk628_mipi_dphy_init_hsmanual(csi->rk628, false, 1);
	}

	usleep_range(1500, 2000);
	rk628_txphy_power_on(csi->rk628);

	usleep_range(1500, 2000);
	mask = DPHY_PLL_LOCK;
	rk628_i2c_read(csi->rk628, CSITX_CSITX_STATUS1, &val);
	if ((val & mask) != mask) {
		dev_err(csi->dev, "PHY is not locked\n");
		return -1;
	}
	if (csi->rk628->version >= RK628F_VERSION) {
		rk628_i2c_read(csi->rk628, CSITX1_CSITX_STATUS1, &val);
		if ((val & mask) != mask) {
			dev_err(csi->dev, "PHY1 is not locked\n");
			return -1;
		}
	}
	udelay(10);

	return 0;
}

static void mipi_dphy_power_off(struct rk628_csi *csi)
{
	rk628_txphy_power_off(csi->rk628);
}

static irqreturn_t plugin_detect_irq(int irq, void *dev_id)
{
	struct rk628_csi *csi = dev_id;
	struct v4l2_subdev *sd = &csi->info.sd;
	const struct v4l2_event evt_signal_lost = {
		.type = RK_HDMIRX_V4L2_EVENT_SIGNAL_LOST,
	};

	/* control hpd after 50ms */
	schedule_delayed_work(&csi->delayed_work_enable_hotplug, HZ / 20);
	v4l2_event_queue(sd->devnode, &evt_signal_lost);
	sensor_dbg("v4l2_event_queue: RK_HDMIRX_V4L2_EVENT_SIGNAL_LOST\n");

	return IRQ_HANDLED;
}

static int rk628_csi_power_on(struct rk628_csi *csi)
{
	/* power on sensor for detect hotplug */
	if (gpio_is_valid(csi->power_gpio.gpio)) {
		gpio_direction_output(csi->power_gpio.gpio, 1);
		usleep_range(10000, 11000);
	}

	/* reset sensor */
	if (gpio_is_valid(csi->reset_gpio.gpio)) {
		gpio_direction_output(csi->reset_gpio.gpio, 1);
		usleep_range(10000, 11000);
		gpio_direction_output(csi->reset_gpio.gpio, 0);
		usleep_range(10000, 11000);
		gpio_direction_output(csi->reset_gpio.gpio, 1);
		usleep_range(10000, 11000);
	}

	return 0;
}

static int rk628_csi_power_off(struct rk628_csi *csi)
{
	/* reset sensor */
	if (gpio_is_valid(csi->reset_gpio.gpio)) {
		gpio_direction_output(csi->reset_gpio.gpio, 1);
		usleep_range(10000, 11000);
		gpio_direction_output(csi->reset_gpio.gpio, 0);
		usleep_range(10000, 11000);
		gpio_direction_output(csi->reset_gpio.gpio, 1);
		usleep_range(10000, 11000);
	}

	/* poweroff */
	if (gpio_is_valid(csi->power_gpio.gpio)) {
		gpio_direction_output(csi->power_gpio.gpio, 0);
		usleep_range(10000, 11000);
	}

	return 0;
}

#if IS_ENABLED(CONFIG_PM_SLEEP)
static int rk628_csi_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct rk628_csi *csi = sd_to_csi(sd);

	sensor_print("%s: suspend!\n", __func__);

	disable_irq(csi->hdmirx_irq);
	cancel_delayed_work_sync(&csi->delayed_work_res_change);
	cancel_delayed_work_sync(&csi->delayed_work_enable_hotplug);
	rk628_hdmirx_audio_cancel_work_audio(csi->audio_info, true);
	rk628_csi_power_off(csi);

	return 0;
}

static int rk628_csi_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct rk628_csi *csi = sd_to_csi(sd);

	sensor_print("%s: resume!\n", __func__);

	rk628_csi_power_on(csi);
	rk628_cru_initialize(csi->rk628);
	rk628_csi_initial(sd);
	rk628_hdmirx_plugout(sd);
	enable_irq(csi->hdmirx_irq);
	schedule_delayed_work(&csi->delayed_work_enable_hotplug,
			     msecs_to_jiffies(500));

	return 0;
}
#endif

#if IS_ENABLED(CONFIG_PM)
static int sensor_runtime_suspend(struct device *dev)
{
	sensor_dbg("%s\n", __func__);
	return 0;
}

static int sensor_runtime_resume(struct device *dev)
{
	sensor_dbg("%s\n", __func__);
	return 0;
}

static int sensor_runtime_idle(struct device *dev)
{
	if (dev) {
		pm_runtime_mark_last_busy(dev);
		pm_request_autosuspend(dev);
	} else {
		sensor_err("%s, sensor device is null\n", __func__);
	}
	return 0;
}
#endif

static const struct dev_pm_ops sensor_runtime_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(rk628_csi_suspend, rk628_csi_resume)
	SET_RUNTIME_PM_OPS(sensor_runtime_suspend, sensor_runtime_resume,
			       sensor_runtime_idle)
};

/*
 * The default register settings
 *
 */
static struct regval_list sensor_default_regs[] = {
};

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

static int sensor_power(struct v4l2_subdev *sd, int on)
{
	int ret;
	ret = 0;
	switch (on) {
	case STBY_ON:
		sensor_dbg("STBY_ON!\n");
		cci_lock(sd);
		cci_unlock(sd);
		break;
	case STBY_OFF:
		sensor_dbg("STBY_OFF!\n");
		cci_lock(sd);
		cci_unlock(sd);
		break;
	case PWR_ON:
		sensor_dbg("PWR_ON! do nothing!\n");
		//cci_lock(sd);
		// vin_gpio_set_status(sd, PWDN, 1);
		// vin_gpio_write(sd, PWDN, CSI_GPIO_HIGH);
		// usleep_range(10000, 11000);
		// vin_gpio_set_status(sd, RESET, 1);
		// vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		// usleep_range(10000, 11000);
		// vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		// usleep_range(10000, 11000);
		// vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		// usleep_range(10000, 11000);
		//cci_unlock(sd);
		break;
	case PWR_OFF:
		sensor_dbg("PWR_OFF! do nothing!\n");
		//cci_lock(sd);
		//vin_gpio_set_status(sd, PWDN, CSI_GPIO_LOW);
		//usleep_range(1000, 1200);
		//cci_unlock(sd);
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
		sensor_dbg("sensor_reset val: 0, do nothing!\n");
		// vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		// usleep_range(10000, 11000);
		break;
	case 1:
		sensor_dbg("sensor_reset val: 1, do nothing!\n");
		// vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		// usleep_range(10000, 11000);
		// vin_gpio_write(sd, RESET, CSI_GPIO_HIGH);
		// usleep_range(10000, 11000);
		// vin_gpio_write(sd, RESET, CSI_GPIO_LOW);
		// usleep_range(10000, 11000);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int sensor_detect(struct v4l2_subdev *sd)
{
	int ret, i = 0;
	unsigned int rdval = 0xffffffff;

	read_rk628(sd, 0x00000200, &rdval);

	while ((V4L2_IDENT_SENSOR != rdval)) {
		i++;
		ret = read_rk628(sd, 0x00000200, &rdval);
		if (i > 4) {
			sensor_print("warning: GRF_SOC_VERSION(0x%08x) is NOT equal to 0x%08x\n", rdval, V4L2_IDENT_SENSOR);
			break;
		}
	}
	if (rdval != V4L2_IDENT_SENSOR)
		return -ENODEV;

	sensor_print("read RK628F GRF_SOC_VERSION = 0x%08x\n", rdval);
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
	info->width = 1920;
	info->height = 1080;
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
	default:
		return -EINVAL;
	}
	return ret;
}

static struct sensor_format_struct sensor_formats[] = {
	{
		.desc		= "YUYV 4:2:2",
		.mbus_code	= MEDIA_BUS_FMT_UYVY8_2X8,
		.regs 		= sensor_default_regs,
		.regs_size	= ARRAY_SIZE(sensor_default_regs),
		.bpp		= 2,
	}
};
#define N_FMTS ARRAY_SIZE(sensor_formats)

/*
 * Then there is the issue of window sizes.  Try to capture the info here.
 */
static struct sensor_win_size sensor_win_sizes[] = {
	{
	  .width = 3840,
	  .height = 2160,
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
	  .width = 720,
	  .height = 480,
	  .hoffset = 0,
	  .voffset = 0,
	  .fps_fixed = 60,
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
	cfg->flags = 0 | V4L2_MBUS_CSI2_4_LANE | V4L2_MBUS_CSI2_CHANNEL_0;
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
	struct rk628_csi *csi = sd_to_csi(sd);

	if (!timings)
		return -EINVAL;

	if (v4l2_match_dv_timings(&csi->timings, timings, 0, false)) {
		sensor_dbg("%s: no change\n", __func__);
		return 0;
	}

	if (!v4l2_valid_dv_timings(timings, &rk628_csi_timings_cap, NULL,
				NULL)) {
		sensor_dbg("%s: timings out of range\n", __func__);
		return -ERANGE;
	}

	csi->timings = *timings;
	enable_stream(sd, false);

	return 0;
}

static int sensor_g_dv_timings(struct v4l2_subdev *sd,
		struct v4l2_dv_timings *timings)
{
	struct rk628_csi *csi = sd_to_csi(sd);

	*timings = csi->timings;

	return 0;
}

static int sensor_query_dv_timings(struct v4l2_subdev *sd,
				struct v4l2_dv_timings *timings)
{
	int ret;
	struct rk628_csi *csi = sd_to_csi(sd);
	struct v4l2_dv_timings default_timing = V4L2_DV_BT_CEA_640X480P59_94;

	if (!tx_5v_power_present(sd) || csi->nosignal) {
		*timings = default_timing;
		v4l2_info(sd, "%s: not detect 5v, set default timing\n", __func__);
		return 0;
	}
	mutex_lock(&csi->confctl_mutex);
	ret = rk628_csi_get_detected_timings(sd, timings);
	mutex_unlock(&csi->confctl_mutex);
	if (ret)
		return ret;

	if (!v4l2_valid_dv_timings(timings, &rk628_csi_timings_cap, NULL,
				NULL)) {
		sensor_dbg("%s: timings out of range\n", __func__);
		return -ERANGE;
	}

	return 0;
}

static int sensor_enum_dv_timings(struct v4l2_subdev *sd,
				struct v4l2_enum_dv_timings *timings)
{
	if (timings->pad != 0)
		return -EINVAL;

	return v4l2_enum_dv_timings_cap(timings, &rk628_csi_timings_cap, NULL,
			NULL);
}

static int sensor_dv_timings_cap(struct v4l2_subdev *sd,
				struct v4l2_dv_timings_cap *cap)
{
	if (cap->pad != 0)
		return -EINVAL;

	*cap = rk628_csi_timings_cap;

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
	case RK_HDMIRX_V4L2_EVENT_SIGNAL_LOST:
		return v4l2_event_subscribe(fh, sub, 0, NULL);
	default:
		return -EINVAL;
	}
}

static int sensor_reg_init(struct sensor_info *info)
{
	struct v4l2_subdev *sd = &info->sd;
	struct sensor_format_struct *sensor_fmt = info->fmt;
	struct sensor_win_size *wsize = info->current_wins;

	sensor_write_array(sd, sensor_default_regs, ARRAY_SIZE(sensor_default_regs));

	if (wsize->regs)
		sensor_write_array(sd, wsize->regs, wsize->regs_size);

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
	struct rk628_csi *csi = sd_to_csi(sd);

	sensor_print("%s on = %d, %d*%d %x\n", __func__, enable,
		  info->current_wins->width,
		  info->current_wins->height, info->fmt->mbus_code);

	enable_stream(sd, enable);

	sensor_print("%s: on: %d, %dx%d@%d\n", __func__, enable,
				csi->timings.bt.width,
				csi->timings.bt.height,
				fps_calc(&csi->timings.bt));

	return sensor_reg_init(info);
}

/* ----------------------------------------------------------------------- */
static const struct v4l2_ctrl_ops sensor_ctrl_ops = {
	.g_volatile_ctrl = sensor_g_ctrl,
	.s_ctrl = sensor_s_ctrl,
	.try_ctrl = sensor_try_ctrl,
};

static const struct v4l2_ctrl_config rk628_csi_ctrl_audio_sampling_rate = {
	.ops = &sensor_ctrl_ops,
	.id = RK_V4L2_CID_AUDIO_SAMPLING_RATE,
	.name = "Audio sampling rate",
	.type = V4L2_CTRL_TYPE_INTEGER,
	.min = 0,
	.max = 768000,
	.step = 1,
	.def = 0,
	.flags = V4L2_CTRL_FLAG_READ_ONLY,
};

static const struct v4l2_ctrl_config rk628_csi_ctrl_audio_present = {
	.id = RK_V4L2_CID_AUDIO_PRESENT,
	.name = "Audio present",
	.type = V4L2_CTRL_TYPE_BOOLEAN,
	.min = 0,
	.max = 1,
	.step = 1,
	.def = 0,
	.flags = V4L2_CTRL_FLAG_READ_ONLY,
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
	.get_fmt = sensor_get_fmt,
	.set_fmt = sensor_set_fmt,
	.get_edid = sensor_get_edid,
	.set_edid = sensor_set_edid,
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

static ssize_t get_det_status_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sensor_info *info = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = &info->sd;
	bool plugin = tx_5v_power_present(sd);

	sensor_dbg("%s, online: %d\n", __func__, plugin);
	return sprintf(buf, "0x%x\n", plugin);
}

static struct device_attribute detect_dev_attrs = {
	.attr = {
		.name = "online",
		.mode = S_IRUGO,
	},
	.show = get_det_status_show,
	.store = NULL,
};

static int rk628_csi_gpio_init(struct rk628_csi *csi)
{
	int ret;
	struct device_node *np = NULL;
	enum of_gpio_flags gc;
	char *node_name = "sensor_detect";
	char *power_gpio_name = "power_gpios";
	char *reset_gpio_name = "reset_gpios";
	char *plugin_det_gpio_name = "hotplug_gpios";
	char *int_det_gpio_name = "interrupt_gpios";

	np = of_find_node_by_name(NULL, node_name);
	if (np == NULL) {
		sensor_err("can not find the %s node\n", node_name);
		return -EINVAL;
	}

	/* power gpio is not necessary in some board, shouldn't return here */
	csi->power_gpio.gpio = of_get_named_gpio_flags(np, power_gpio_name, 0, &gc);
	sensor_dbg("get form %s gpio is %d\n", power_gpio_name, csi->power_gpio.gpio);
	if (!gpio_is_valid(csi->power_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed\n", power_gpio_name);
		/* return -ENODEV; */
	} else {
		ret = gpio_request(csi->power_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", power_gpio_name);
			/* return -ENODEV; */
		}
	}

	csi->reset_gpio.gpio = of_get_named_gpio_flags(np, reset_gpio_name, 0, &gc);
	sensor_dbg("get form %s gpio is %d\n", reset_gpio_name, csi->reset_gpio.gpio);
	if (!gpio_is_valid(csi->reset_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed\n", reset_gpio_name);
		return -ENODEV;
	} else {
		ret = gpio_request(csi->reset_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", reset_gpio_name);
			return ret;
		}
	}

	csi->plugin_det_gpio.gpio = of_get_named_gpio_flags(np, plugin_det_gpio_name, 0, &gc);
	sensor_dbg("get form %s gpio is %d\n", plugin_det_gpio_name, csi->plugin_det_gpio.gpio);
	if (!gpio_is_valid(csi->plugin_det_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed\n", plugin_det_gpio_name);
		return -ENODEV;
	} else {
		ret = gpio_request(csi->plugin_det_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", plugin_det_gpio_name);
			return ret;
		}
	}
	//gpio_direction_input(csi->plugin_det_gpio.gpio);

	csi->int_det_gpio.gpio = of_get_named_gpio_flags(np, int_det_gpio_name, 0, &gc);
	sensor_dbg("get form %s gpio is %d\n", int_det_gpio_name, csi->int_det_gpio.gpio);
	if (!gpio_is_valid(csi->int_det_gpio.gpio)) {
		sensor_err("fetch %s from device_tree failed\n", int_det_gpio_name);
		return -ENODEV;
	} else {
		ret = gpio_request(csi->int_det_gpio.gpio, NULL);
		if (ret < 0) {
			sensor_err("request %s fail!\n", int_det_gpio_name);
			return ret;
		}
	}
	//gpio_direction_input(csi->int_det_gpio.gpio);

	return 0;
}

static int rk628_csi_gpio_free(struct rk628_csi *csi)
{
	sensor_dbg("%s\n", __func__);
	if (NULL == csi) {
		sensor_err("%s: csi is NULL! failed to free gpio resources\n", __func__);
		return -ENODEV;
	}

	/* free gpio resources */
	if (csi->int_det_gpio.gpio > 0) {
		sensor_dbg("gpio_free int_det_gpio(%d)\n", csi->int_det_gpio.gpio);
		gpio_free(csi->int_det_gpio.gpio);
	}

	if (csi->plugin_det_gpio.gpio > 0) {
		sensor_dbg("gpio_free plugin_det_gpio(%d)\n", csi->plugin_det_gpio.gpio);
		gpio_free(csi->plugin_det_gpio.gpio);
	}

	if (csi->reset_gpio.gpio > 0) {
		sensor_dbg("gpio_free reset_gpio(%d)\n", csi->reset_gpio.gpio);
		gpio_free(csi->reset_gpio.gpio);
	}

	if (csi->power_gpio.gpio > 0) {
		sensor_dbg("gpio_free power_gpio(%d)\n", csi->power_gpio.gpio);
		gpio_free(csi->power_gpio.gpio);
	}

	return 0;
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

static int sensor_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret;
	//unsigned int val;
	struct rk628_csi *csi;
	struct rk628 *rk628;
	struct v4l2_subdev *sd;
	struct device *dev = &client->dev;
	struct sensor_info *info;
	struct sensor_indetect *sensor_indet;
	struct v4l2_dv_timings default_timing = V4L2_DV_BT_CEA_640X480P59_94;
#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	unsigned long config_set;
#else
	volatile void __iomem *ccu_cpu_base;
#endif

	csi = devm_kzalloc(dev, sizeof(*csi), GFP_KERNEL);
	if (NULL == csi)
		return -ENOMEM;
	csi->dev = dev;
	csi->i2c_client = client;
	info = &csi->info;

	rk628 = rk628_i2c_register(client);
	if (!rk628) {
		return -ENOMEM;
	}
	csi->rk628 = rk628;

	ret = rk628_csi_gpio_init(csi);
	if (ret) {
		sensor_err("rk628_csi_gpio_init failed! ret:%d\n", ret);
		return ret;
	}
	csi->rk628->hdmirx_det_gpio.gpio = csi->plugin_det_gpio.gpio;

	csi->timings = default_timing;

	csi->hpd_output_inverted = true;
	//csi->hdmirx_det_inverted = true;
	csi->cec_enable = false;
	csi->csi_lanes_in_use = 4;
	csi->enable_hdcp = false;
#if RK628_I2S_ENABLE
	csi->i2s_enable_default = true;
#else
	csi->i2s_enable_default = false;
#endif
	csi->scaler_en = false;
	if (csi->scaler_en)
		csi->timings = dst_timing;
	csi->continues_clk = false;
	csi->rxphy_pwron = false;
	csi->txphy_pwron = false;
	csi->nosignal = true;
	csi->stream_state = 0;
	csi->avi_rcv_rdy = false;

	// info = kzalloc(sizeof(struct sensor_info), GFP_KERNEL);
	// if (NULL == info)
	// 	return -ENOMEM;

	sensor_indet = &info->sensor_indet;
	sd = &info->sd;
	cci_dev_probe_helper(sd, client, &sensor_ops, &cci_drv);

	sd->flags |= V4L2_SUBDEV_FL_HAS_EVENTS;
	sensor_init_controls(sd, &sensor_ctrl_ops);
	mutex_init(&info->lock);
	dev_set_drvdata(&cci_drv.cci_device, info);

	info->fmt = &sensor_formats[0];
	info->fmt_pt = &sensor_formats[0];
	info->win_pt = &sensor_win_sizes[0];
	info->fmt_num = N_FMTS;
	info->win_size_num = N_WIN_SIZES;
	info->combo_mode = CMB_TERMINAL_RES | CMB_PHYA_OFFSET2 | MIPI_NORMAL_MODE;
	info->time_hs = 0x30;
	info->stream_seq = MIPI_BEFORE_SENSOR;
	info->af_first_flag = 1;
	info->exp = 0;
	info->gain = 0;
	info->not_detect_use_count = 1;
#if IS_ENABLED(CONFIG_SAME_I2C)
	info->sensor_i2c_addr = I2C_ADDR >> 1;
#endif

	/***************** Initialize registers of RK628 *****************/
	rk628_csi_power_on(csi);
	rk628_cru_initialize(csi->rk628);
	rk628_version_parse(rk628);

	mutex_init(&csi->confctl_mutex);

	if (NULL == rk628_txphy_register(rk628)) {
		sensor_err("rk628 register txphy failed\n");
		ret = -ENOMEM;
		goto power_off;
	}

	/* control handlers */
	v4l2_ctrl_handler_init(&csi->hdl, 4);
	csi->link_freq = v4l2_ctrl_new_int_menu(&csi->hdl, NULL,
			V4L2_CID_LINK_FREQ,
			ARRAY_SIZE(link_freq_menu_items) - 1,
			0, link_freq_menu_items);
	csi->pixel_rate = v4l2_ctrl_new_std(&csi->hdl, NULL,
			V4L2_CID_PIXEL_RATE, 0, RK628_CSI_PIXEL_RATE_HIGH, 1,
			RK628_CSI_PIXEL_RATE_HIGH);
	csi->detect_tx_5v_ctrl = v4l2_ctrl_new_std(&csi->hdl,
			NULL, V4L2_CID_DV_RX_POWER_PRESENT,
			0, 1, 0, 0);

	/* custom controls */
	csi->audio_sampling_rate_ctrl = v4l2_ctrl_new_custom(&csi->hdl,
			&rk628_csi_ctrl_audio_sampling_rate, NULL);
	csi->audio_present_ctrl = v4l2_ctrl_new_custom(&csi->hdl,
			&rk628_csi_ctrl_audio_present, NULL);

	sd->ctrl_handler = &csi->hdl;
	if (csi->hdl.error) {
		ret = csi->hdl.error;
		sensor_err("cfg v4l2 ctrls failed! err:%d\n", ret);
		goto err_hdl;
	}

	if (rk628_csi_update_controls(sd)) {
		ret = -ENODEV;
		sensor_err("update v4l2 ctrls failed! err:%d\n", ret);
		goto err_hdl;
	}

	ret = device_create_file(&cci_drv.cci_device, &detect_dev_attrs);
	if (ret) {
		sensor_err("class_create file fail!\n");
	}

	INIT_DELAYED_WORK(&csi->delayed_work_enable_hotplug, rk628_csi_delayed_work_enable_hotplug);
	INIT_DELAYED_WORK(&csi->delayed_work_res_change, rk628_delayed_work_res_change);
	INIT_WORK(&csi->work_isr, rk628_work_isr);

	csi->audio_info = rk628_hdmirx_audioinfo_alloc(dev,
						       &csi->confctl_mutex,
						       rk628,
						       csi->i2s_enable_default);
	if (!csi->audio_info) {
		sensor_err("request audio info fail\n");
		goto err_work_queues;
	}
	rk628_csi_initial_setup(sd);

	// rk628 interrupt
	if (gpio_is_valid(csi->int_det_gpio.gpio))
		csi->hdmirx_irq = gpio_to_irq(csi->int_det_gpio.gpio);
	if (csi->hdmirx_irq >= 0) {
		ret = devm_request_threaded_irq(dev, csi->hdmirx_irq, NULL,
				rk628_csi_irq_handler, IRQF_TRIGGER_RISING | IRQF_ONESHOT, "rk628_csi", csi);
		if (ret) {
			sensor_err("request rk628-csi irq failed! ret: %d\n", ret);
			goto err_work_queues;
		}
	} else {
		sensor_print("gpio %d request hdmirx irq err, cfg poll!\n", csi->int_det_gpio.gpio);
		INIT_WORK(&csi->work_i2c_poll, rk628_csi_work_i2c_poll);
		timer_setup(&csi->timer, rk628_csi_irq_poll_timer, 0);
		csi->timer.expires = jiffies + msecs_to_jiffies(POLL_INTERVAL_MS);
		add_timer(&csi->timer);
	}

	// rk628 plugin detect
	if (gpio_is_valid(csi->plugin_det_gpio.gpio)) {
		csi->plugin_irq = gpio_to_irq(csi->plugin_det_gpio.gpio);
		if (csi->plugin_irq < 0) {
			sensor_err("failed to get plugin det irq\n");
			ret = csi->plugin_irq;
			goto err_work_queues;
		}

		ret = devm_request_threaded_irq(dev, csi->plugin_irq, NULL,
				plugin_detect_irq, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_ONESHOT, "rk628_csi", csi);
		if (ret) {
			sensor_err("failed to register plugin det irq, ret: %d\n", ret);
			goto err_work_queues;
		}
	}

#if IS_ENABLED(CONFIG_ARCH_SUN60IW2)
	config_set = pinconf_to_config_packed(PIN_CONFIG_BIAS_PULL_UP, PINCTRL_PULL_UP);
	ret = pinctrl_gpio_set_config(csi->plugin_det_gpio.gpio, config_set);
	if (ret < 0) {
		sensor_err("plugin_det_gpio pinctrl pull fail\n");
	}
#else
	// configure plugin_det_gpio(PE1) as pull_up, only for debug.
	// 0b00: Pull_up/down disable, 0b01: Pull up, 0b10: Pull down, 0b11: Reserverd
	// vin_reg_clr_set(SYS_GPIO_BASE + PE_PULL_REG0_OFF, PE1_PULL_MASK, select << PE1_PULL);
	ccu_cpu_base = ioremap(0x02000000, 0x800);
	vin_reg_clr_set(ccu_cpu_base + 0xE4, 0b11 << 2, 0b01 << 2);
#endif

	ret = v4l2_ctrl_handler_setup(sd->ctrl_handler);
	if (ret) {
		sensor_err("v4l2 ctrl handler setup failed! ret:%d\n", ret);
		goto err_work_queues;
	}
	csi->rk628->dual_mipi = false;
	rk628_debugfs_create(csi->rk628);

	sensor_print("%s found @ 0x%x (%s)\n", client->name, client->addr << 1, client->adapter->name);

	/*****************************************************************/

	return 0;

err_work_queues:
	if (!csi->hdmirx_irq)
		flush_work(&csi->work_i2c_poll);
	cancel_delayed_work(&csi->delayed_work_enable_hotplug);
	cancel_delayed_work(&csi->delayed_work_res_change);
	cancel_work_sync(&csi->work_isr);
	rk628_hdmirx_audio_destroy(csi->audio_info);

err_hdl:
	mutex_destroy(&csi->confctl_mutex);
	v4l2_ctrl_handler_free(&csi->hdl);

power_off:
	rk628_csi_power_off(csi);
	rk628_csi_gpio_free(csi);

	return ret;
}

static int sensor_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct sensor_info *info = to_state(sd);
	struct rk628_csi *csi = info_to_csi(info);

	debugfs_remove_recursive(csi->rk628->debug_dir);
	if (!csi->hdmirx_irq) {
		del_timer_sync(&csi->timer);
		flush_work(&csi->work_i2c_poll);
	}

	if (csi->cec_enable && csi->cec)
		rk628_hdmirx_cec_unregister(csi->cec);

	rk628_hdmirx_audio_cancel_work_audio(csi->audio_info, true);
	rk628_hdmirx_audio_cancel_work_rate_change(csi->audio_info, true);
	cancel_delayed_work_sync(&csi->delayed_work_enable_hotplug);
	cancel_delayed_work_sync(&csi->delayed_work_res_change);
	cancel_work_sync(&csi->work_isr);

	if (csi->rxphy_pwron)
		rk628_rxphy_power_off(csi->rk628);
	if (csi->txphy_pwron)
		mipi_dphy_power_off(csi);

	mutex_destroy(&csi->confctl_mutex);

	rk628_control_assert(csi->rk628, RGU_HDMIRX);
	rk628_control_assert(csi->rk628, RGU_HDMIRX_PON);
	rk628_control_assert(csi->rk628, RGU_DECODER);
	rk628_control_assert(csi->rk628, RGU_CLK_RX);
	rk628_control_assert(csi->rk628, RGU_VOP);
	rk628_control_assert(csi->rk628, RGU_CSI);
	if (csi->rk628->version >= RK628F_VERSION)
		rk628_control_assert(csi->rk628, RGU_CSI1);
	rk628_csi_power_off(csi);

	rk628_csi_gpio_free(csi);
	sd = cci_dev_remove_helper(client, &cci_drv);
	kfree(to_state(sd));

	sensor_dbg("rk628 driver removed!\n");

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
		   .pm = &sensor_runtime_pm_ops,
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

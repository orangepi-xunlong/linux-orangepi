// SPDX-License-Identifier: GPL-2.0
/*
* Copyright (c) 2022 CIX Electronics Co. Ltd.
*
* lt7911uxc type-c/DP to MIPI CSI-2 bridge driver.
*/
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/hdmi.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_graph.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/v4l2-dv-timings.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/workqueue.h>
#include <linux/compat.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-event.h>
#include <media/v4l2-fwnode.h>
#include <media/v4l2-dv-timings.h>
#include <media/v4l2-mediabus.h>
#include <linux/pinctrl/consumer.h>

#include <sound/jack.h>
#include <sound/hdmi-codec.h>

static int debug = 3;
module_param(debug, int, 0644);
MODULE_PARM_DESC(debug, "debug level (0-3)");

#define TRANNING_TIME_OUT	(300)
#define I2C_MAX_XFER_SIZE	(128)

#define POLL_SHOW_INTERVAL_MS	(5000)
#define POLL_INTERVAL_MS	(200)

#define LT7911UXC_LINK_FREQ_1250M	(1250000000)
#define LT7911UXC_LINK_FREQ_860M	(860000000)
#define LT7911UXC_LINK_FREQ_700M	(700000000)
#define LT7911UXC_LINK_FREQ_400M	(400000000)
#define LT7911UXC_LINK_FREQ_300M	(300000000)
#define LT7911UXC_LINK_FREQ_200M	(200000000)
#define LT7911UXC_LINK_FREQ_100M	(100000000)

#define LT7911UXC_PIXEL_RATE		800000000


#define GET_PAGE_ID(x)		((x>>8)&0xFF)
#define GET_REG(x)		((x)&0xFF)

#define LT7911UXC_CHIPID	(0x0119)
#define CHIPID_REGH		(0xe101)
#define CHIPID_REGL		(0xe100)
#define I2C_EN_REG		(0xe0ee)
#define I2C_ENABLE		(0x1)
#define I2C_DISABLE		(0x0)

#define HTOTAL_H		(0xe088)
#define HTOTAL_L		(0xe089)
#define HACT_H			(0xe08c)
#define HACT_L			(0xe08d)

#define VTOTAL_H		(0xe08a)
#define VTOTAL_L		(0xe08b)
#define VACT_H			(0xe08e)
#define VACT_L			(0xe08f)
#define PORT_N			(0xe0a0)
#define MIPI_F			(0xe0a1)

#define PCLK_H			(0xe085)
#define PCLK_M			(0xe086)
#define PCLK_L			(0xe087)
#define BYTE_PCLK_H		(0xe092)
#define BYTE_PCLK_M		(0xe093)
#define BYTE_PCLK_L		(0xe094)
#define AUDIO_FS_VALUE_H	(0xe090)
#define AUDIO_FS_VALUE_L	(0xe091)
#define STREAM_CTL		(0xe0b0)
#define ENABLE_STREAM		(0x01)
#define DISABLE_STREAM		(0x00)
#define AUDIO_CH		(0xe0b0)

#ifdef LT7911UXC_OUT_RGB
#define LT7911UXC_MEDIA_BUS_FMT		MEDIA_BUS_FMT_BGR888_1X24
#else
#define LT7911UXC_MEDIA_BUS_FMT		MEDIA_BUS_FMT_UYVY8_2X8
#endif

#define LT7911UXC_NAME		"LT7911UXC"
#define CLOCK_UNIT_KHZ		(1000)

/* CSI-2 Virtual Channel identifiers. */
#define V4L2_MBUS_CSI2_CHANNEL_0		BIT(4)
#define V4L2_MBUS_CSI2_CHANNEL_1		BIT(5)
#define V4L2_MBUS_CSI2_CHANNEL_2		BIT(6)
#define V4L2_MBUS_CSI2_CHANNEL_3		BIT(7)

#define V4L2_MBUS_CSI2_CONTINUOUS_CLOCK		BIT(8)
#define CIX_LT7911UXC_SUBDEV_NAME		"lt7911uxc"

struct lt7911uxc_audio {
	struct platform_device *pdev;
	struct device *codec_dev;
	hdmi_codec_plugged_cb plugged_cb;
};

/* LT7911UXC PAD ID */
enum lt7911uxc_pad_id {
	LT7911UXC_SD_PAD_SOURCE_CSI,
	LT7911UXC_SD_PAD_MAX_NUM,
};

static const s64 link_freq_menu_items[] = {
	LT7911UXC_LINK_FREQ_1250M,
	LT7911UXC_LINK_FREQ_860M,
	LT7911UXC_LINK_FREQ_700M,
	LT7911UXC_LINK_FREQ_400M,
	LT7911UXC_LINK_FREQ_300M,
	LT7911UXC_LINK_FREQ_200M,
	LT7911UXC_LINK_FREQ_100M,
};

struct lt7911uxc {
	//struct v4l2_fwnode_bus_mipi_csi2 bus;
	struct v4l2_subdev sd;
	struct media_pad pad[LT7911UXC_SD_PAD_MAX_NUM];
	struct v4l2_ctrl_handler hdl;
	struct i2c_client *i2c_client;
	struct v4l2_fwnode_endpoint ep;
	struct mutex confctl_mutex;
	struct v4l2_ctrl *detect_tx_5v_ctrl;
	struct v4l2_ctrl *audio_sampling_rate_ctrl;
	struct v4l2_ctrl *audio_present_ctrl;
	struct v4l2_ctrl *link_freq;
	struct v4l2_ctrl *pixel_rate;
	unsigned int fps;
	unsigned int lane_rate;
	unsigned int lane_num;
	unsigned int byte_rate;
	struct delayed_work delayed_work_hotplug;
	struct delayed_work delayed_work_res_change;
	struct v4l2_dv_timings timings;
	struct clk *xvclk;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *pwdn_gpio;
	struct gpio_desc *plugin_det_gpio;
	struct gpio_desc *power_gpio;
	struct gpio_desc *power1_gpio;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_gpio;
	struct pinctrl_state *pins_default;
	struct work_struct work_i2c_poll;
	struct timer_list timer;
	const char *module_facing;
	const char *module_name;
	const char *len_name;
	int tranning_enable;
	int tranning_ready;
	int mode;
	const struct lt7911uxc_mode *cur_mode;
	const struct lt7911uxc_mode *support_modes;
	u32 cfg_num;
	u32 active_num;
	struct v4l2_fwnode_endpoint bus_cfg;
	bool nosignal;
	bool enable_hdcp;
	bool is_audio_present;
	bool power_on;
	bool stream_on;
	int plugin_irq;
	u32 mbus_fmt_code;
	u32 module_index;
	u32 audio_sampling_rate;

	struct device *dev;
	struct lt7911uxc_audio lt_audio;
};

struct lt7911uxc_mode {
	u32 width;
	u32 height;
	struct v4l2_fract max_fps;
	u32 hts_def;
	u32 vts_def;
	u32 exp_def;
	u32 mipi_freq_idx;
};

static const struct lt7911uxc_mode supported_modes_dphy[] = {
#ifdef SKY1_SOC

	{
		.width = 640,
		.height = 480,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 800,
		.vts_def = 525,
	},
	{
		.width = 3840,
		.height = 2160,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 4400,
		.vts_def = 2250,
		.mipi_freq_idx = 0,
	}, {
		.width = 1920,
		.height = 1080,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 2200,
		.vts_def = 1125,
		.mipi_freq_idx = 4,
	}, {
		.width = 1600,
		.height = 1200,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 2160,
		.vts_def = 1250,
		.mipi_freq_idx = 4,
	}, {
		.width = 1280,
		.height = 960,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 1712,
		.vts_def = 994,
		.mipi_freq_idx = 5,
	}, {
		.width = 1280,
		.height = 720,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 1650,
		.vts_def = 750,
		.mipi_freq_idx = 5,
	}, {
		.width = 800,
		.height = 600,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 1056,
		.vts_def = 628,
		.mipi_freq_idx = 6,
	}, {
		.width = 720,
		.height = 576,
		.max_fps = {
			.numerator = 10000,
			.denominator = 500000,
		},
		.hts_def = 864,
		.vts_def = 625,
		.mipi_freq_idx = 6,
	}, {
		.width = 720,
		.height = 480,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 858,
		.vts_def = 525,
		.mipi_freq_idx = 6,
	},
#else
	{
		.width = 640,
		.height = 480,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 800,
		.vts_def = 525,
		.mipi_freq_idx = 1,

	},

	{
		.width = 1920,
		.height = 1080,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 2200,
		.vts_def = 1125,
		.mipi_freq_idx = 3,
	},
	{
		.width = 3840,
		.height = 2160,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.hts_def = 4400,
		.vts_def = 2250,
		.mipi_freq_idx = 0,
	},

#endif
};

int lt7911_check_tranning_ready(struct lt7911uxc *lt7911uxc);

static inline struct lt7911uxc *to_lt7911uxc(struct v4l2_subdev *sd)
{
	return container_of(sd, struct lt7911uxc, sd);
}

static void i2c_rd(struct v4l2_subdev *sd, u8 reg, u8 *values, u32 n)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	struct i2c_client *client = lt7911uxc->i2c_client;
	struct i2c_msg msgs[2];
	unsigned char buf[1] = {reg};
	int ret;

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = buf;

	msgs[1].addr = client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = n;
	msgs[1].buf = values;

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs)) {
		v4l2_err(sd, "%s: reading register 0x%x from 0x%x failed\n",
				__func__, reg, client->addr);
	}
}

static void i2c_wr(struct v4l2_subdev *sd, u8 reg, u8 *values, u32 n)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	struct i2c_client *client = lt7911uxc->i2c_client;
	struct i2c_msg msgs[1];
	u8 buf[I2C_MAX_XFER_SIZE + 1] = {reg};
	int ret;

	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1 + n;
	msgs[0].buf = buf;

	memcpy(&buf[1],values,n);

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		v4l2_err(sd, "%s: writing register 0x%x from 0x%x failed\n",
				__func__, reg, client->addr);
		return;
	}
}

u8 i2c_rd8(struct v4l2_subdev *sd, u16 reg)
{
	u32 val;
	i2c_rd(sd, reg, (u8 __force *)&val, 1);
	return val;
}

static void i2c_wr8(struct v4l2_subdev *sd, u16 reg, u8 val)
{
	i2c_wr(sd, reg, &val, 1);
}

static void lt7911_select_page(struct v4l2_subdev *sd,unsigned char page_id)
{
	i2c_wr(sd, 0xff, &page_id,1);
}

static void lt7911uxc_i2c_enable(struct v4l2_subdev *sd)
{
	unsigned char buf[1] = {I2C_ENABLE};

	lt7911_select_page(sd,GET_PAGE_ID(I2C_EN_REG));
	i2c_wr(sd,GET_REG(I2C_EN_REG),buf,1);
}

static void lt7911uxc_i2c_disable(struct v4l2_subdev *sd)
{
	unsigned char buf[1] = {I2C_DISABLE};
	lt7911_select_page(sd,GET_PAGE_ID(I2C_EN_REG));
	i2c_wr(sd,GET_REG(I2C_EN_REG),buf,1);
}

static inline bool no_signal(struct v4l2_subdev *sd)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);

	v4l2_dbg(1, debug, sd, "%s no signal:%d\n", __func__,
			lt7911uxc->nosignal);

	return lt7911uxc->nosignal;
}

static inline unsigned int fps_calc(const struct v4l2_bt_timings *t)
{
	if (!V4L2_DV_BT_FRAME_HEIGHT(t) || !V4L2_DV_BT_FRAME_WIDTH(t))
		return 0;

	return DIV_ROUND_CLOSEST((unsigned int)t->pixelclock,
			V4L2_DV_BT_FRAME_HEIGHT(t) * V4L2_DV_BT_FRAME_WIDTH(t));
}

static bool lt7911uxc_rcv_supported_res(struct v4l2_subdev *sd, u32 width,
		u32 height,u32 *mode)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	u32 i;

	for (i = 0; i < lt7911uxc->cfg_num; i++) {
		if ((lt7911uxc->support_modes[i].width == width) &&
		    (lt7911uxc->support_modes[i].height == height)) {
			break;
		}
	}

	if (i == lt7911uxc->cfg_num) {
		return false;
	} else {
		*mode = i;
		return true;
	}
}

static int lt7911uxc_get_detected_timings(struct v4l2_subdev *sd,
				     struct v4l2_dv_timings *timings)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	struct device *dev = &lt7911uxc->i2c_client->dev;
	struct v4l2_bt_timings *bt = &timings->bt;
	unsigned int mode;
	static unsigned int poll_cnt = 0;

	u32 hact, vact, htotal, vtotal;
	u32 pixel_clock, fps, halt_pix_clk;
	u8 value[3];
	u64 byte_clk, mipi_clk, mipi_data_rate;

	memset(timings, 0, sizeof(struct v4l2_dv_timings));

	lt7911_select_page(sd,GET_PAGE_ID(PCLK_H));

	i2c_rd(sd,GET_REG(PCLK_H),value,3);
	halt_pix_clk = ((value[0] << 16) | (value[1] << 8) | value[2]);
	pixel_clock = halt_pix_clk * CLOCK_UNIT_KHZ;

	i2c_rd(sd,GET_REG(BYTE_PCLK_H),value,3);
	byte_clk = ((value[0] << 16) | (value[1] << 8) | value[2]) * CLOCK_UNIT_KHZ;
	mipi_clk = byte_clk * 4;	//here get the mipi clk
	mipi_data_rate = byte_clk * 8;	//mipi is ddr clk so data_clk = mipi_clk*2

	i2c_rd(sd,GET_REG(HTOTAL_H),value,2);
	htotal = ((value[0] << 8) | value[1]);

	i2c_rd(sd,GET_REG(VTOTAL_H),value,2);
	vtotal = (value[0] << 8) | value[1];

	i2c_rd(sd,GET_REG(HACT_H),value,2);
	hact = ((value[0] << 8) | value[1]);

	i2c_rd(sd,GET_REG(VACT_H),value,2);
	vact = ((value[0] << 8) | value[1]);

	i2c_rd(sd,GET_REG(PORT_N),value,2);//get the phy number & mipi data formate

	lt7911uxc->nosignal = false;
	lt7911uxc->is_audio_present = true;

	timings->type = V4L2_DV_BT_656_1120;
	bt->interlaced = V4L2_DV_PROGRESSIVE;
	bt->width = hact;
	bt->height = vact;
	bt->pixelclock = pixel_clock;

	fps = pixel_clock / (htotal * vtotal);

	poll_cnt++;

	if ((poll_cnt%(POLL_SHOW_INTERVAL_MS/POLL_INTERVAL_MS)) == 0) {
		dev_info(dev,"phy number %d mipi formate %d \n",value[0],value[1]);
		dev_info(dev,"fps %d htotal %d vtotal %d hact %d vact %d \n",fps,htotal,vtotal,hact,vact);
		dev_info(dev,"byte_clk:%llu, mipi_clk:%llu, mipi_data_rate:%llu\n",byte_clk, mipi_clk, mipi_data_rate);
	}

	if (!lt7911uxc_rcv_supported_res(sd, hact, vact,&mode)) {
		lt7911uxc->nosignal = true;
		return -EINVAL;
	} else {
		lt7911uxc->tranning_ready = 1;
		lt7911uxc->mode = mode;
		lt7911uxc->fps = fps;
		lt7911uxc->lane_rate = mipi_data_rate;
		lt7911uxc->byte_rate = pixel_clock;
	}

	return 0;
}

static inline void enable_stream(struct v4l2_subdev *sd, bool enable)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);

	lt7911_select_page(sd,GET_PAGE_ID(STREAM_CTL));

	if (enable) {
		i2c_wr8(&lt7911uxc->sd,GET_REG(STREAM_CTL),ENABLE_STREAM);
	} else {
		i2c_wr8(&lt7911uxc->sd,GET_REG(STREAM_CTL),DISABLE_STREAM);
	}

	msleep(1);
}

static int lt7911uxc_get_reso_dist(const struct lt7911uxc_mode *mode,
				struct v4l2_dv_timings *timings)
{
	struct v4l2_bt_timings *bt = &timings->bt;
	u32 cur_fps, dist_fps;

	cur_fps = fps_calc(bt);
	dist_fps = DIV_ROUND_CLOSEST(mode->max_fps.denominator, mode->max_fps.numerator);

	return abs(mode->width - bt->width) +
		abs(mode->height - bt->height) + abs(dist_fps - cur_fps);
}

static const struct lt7911uxc_mode *
lt7911uxc_find_best_fit(struct lt7911uxc *lt7911uxc)
{
	int dist;
	int cur_best_fit = 0;
	int cur_best_fit_dist = -1;
	unsigned int i;

	for (i = 0; i < lt7911uxc->cfg_num; i++) {
		dist = lt7911uxc_get_reso_dist(&lt7911uxc->support_modes[i], &lt7911uxc->timings);
		if (cur_best_fit_dist == -1 || dist < cur_best_fit_dist) {
			cur_best_fit_dist = dist;
			cur_best_fit = i;
		}
	}
	dev_dbg(&lt7911uxc->i2c_client->dev,
		"find current mode: support_mode[%d], %dx%d@%dfps\n",
		cur_best_fit, lt7911uxc->support_modes[cur_best_fit].width,
		lt7911uxc->support_modes[cur_best_fit].height,
		DIV_ROUND_CLOSEST(lt7911uxc->support_modes[cur_best_fit].max_fps.denominator,
		lt7911uxc->support_modes[cur_best_fit].max_fps.numerator));

	return &lt7911uxc->support_modes[cur_best_fit];
}

static void lt7911uxc_format_change(struct v4l2_subdev *sd)
{
	struct v4l2_dv_timings timings;
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	if (lt7911uxc->tranning_enable == 1) {
		if (lt7911uxc_get_detected_timings(sd, &timings)) {
		}
	}
}

static void lt7911uxc_irq_poll_timer(struct timer_list *t)
{
	struct lt7911uxc *lt7911uxc = from_timer(lt7911uxc, t, timer);
	schedule_work(&lt7911uxc->work_i2c_poll);
	mod_timer(&lt7911uxc->timer, jiffies + msecs_to_jiffies(POLL_INTERVAL_MS));
}

static void lt7911uxc_work_i2c_poll(struct work_struct *work)
{
	struct lt7911uxc *lt7911uxc = container_of(work,
			struct lt7911uxc, work_i2c_poll);
	struct v4l2_subdev *sd = &lt7911uxc->sd;

	lt7911uxc_format_change(sd);
}

static int lt7911uxc_subscribe_event(struct v4l2_subdev *sd, struct v4l2_fh *fh,
				    struct v4l2_event_subscription *sub)
{
	switch (sub->type) {
	case V4L2_EVENT_SOURCE_CHANGE:
		return v4l2_src_change_event_subdev_subscribe(sd, fh, sub);
	case V4L2_EVENT_CTRL:
		return v4l2_ctrl_subdev_subscribe_event(sd, fh, sub);
	default:
		return -EINVAL;
	}
}

static int lt7911uxc_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	*status = 0;
	*status |= no_signal(sd) ? V4L2_IN_ST_NO_SIGNAL : 0;
	v4l2_dbg(1, debug, sd, "%s: status = 0x%x\n", __func__, *status);

	return 0;
}

static int lt7911uxc_g_mbus_config(struct v4l2_subdev *sd,
			unsigned int pad, struct v4l2_mbus_config *cfg)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	u32 lane_num = lt7911uxc->bus_cfg.bus.mipi_csi2.num_data_lanes;
	u32 val = 0;

	val = 1 << (lane_num - 1) |
		V4L2_MBUS_CSI2_CHANNEL_0 |
		V4L2_MBUS_CSI2_CONTINUOUS_CLOCK;

	cfg->type = lt7911uxc->bus_cfg.bus_type;
	cfg->bus.mipi_csi2.flags = val;

	return 0;
}

static int lt7911uxc_s_stream(struct v4l2_subdev *sd, int on)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	struct device *dev = &lt7911uxc->i2c_client->dev;
	dev_info(dev,"lt7911 stream %s \n",on ? "on":"off");

	enable_stream(sd, on);
	if(on) {
		lt7911uxc->tranning_ready = 0;
		lt7911uxc->stream_on = 1;
	} else {
		lt7911uxc->stream_on = 0;
	}

	return 0;
}

static int lt7911uxc_enum_mbus_code(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *state,
			struct v4l2_subdev_mbus_code_enum *code)
{
	switch (code->index) {
		case 0:
			code->code = LT7911UXC_MEDIA_BUS_FMT;
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int lt7911uxc_enum_frame_sizes(struct v4l2_subdev *sd,
					struct v4l2_subdev_state *state,
					struct v4l2_subdev_frame_size_enum *fse)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	int ret;

	if (fse->index >= lt7911uxc->active_num)
		return -EINVAL;

	/*now we just support yuyv format*/
	if (fse->code != MEDIA_BUS_FMT_YUYV8_1X16) {
		return -EINVAL;
	}

	if (lt7911_check_tranning_ready(lt7911uxc)) {
		lt7911uxc->cur_mode = &lt7911uxc->support_modes[lt7911uxc->mode];
		fse->min_width = lt7911uxc->cur_mode->width;
		fse->max_width = lt7911uxc->cur_mode->width;
		fse->min_height = lt7911uxc->cur_mode->height;
		fse->max_height = lt7911uxc->cur_mode->height;
		ret = 0;
	} else {
		ret = -EINVAL;
	}

	return ret;
}

int lt7911_check_tranning_ready(struct lt7911uxc *lt7911uxc)
{
	unsigned int loop = 0;
	int ret = 0;

	lt7911uxc->tranning_enable = 1;
	for(loop = 0;loop < TRANNING_TIME_OUT;loop++) {
		if(lt7911uxc->tranning_ready) {
			break;
		}
		msleep(10);
	}
	lt7911uxc->tranning_enable = 0;

	dev_info(&lt7911uxc->i2c_client->dev,"wait tranning time %d ms\n",loop*10);

	if(loop != TRANNING_TIME_OUT) {
		ret = 1;
	}

	return ret;
}

static int lt7911uxc_get_fmt(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *state,
				struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *fmt = &format->format;
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	int ret = -1;

	if (format->pad != 0)
		return -EINVAL;

	if (lt7911_check_tranning_ready(lt7911uxc)) {

		/* retrieve pixel format */
		fmt->code  = LT7911UXC_MEDIA_BUS_FMT;
		format->format.field =
			lt7911uxc->timings.bt.interlaced ?
			V4L2_FIELD_INTERLACED : V4L2_FIELD_NONE;

		lt7911uxc->cur_mode = &lt7911uxc->support_modes[lt7911uxc->mode];

		/* retrieve active video frame size */
		fmt->width  = lt7911uxc->cur_mode->width;
		fmt->height = lt7911uxc->cur_mode->height;

		__v4l2_ctrl_s_ctrl_int64(lt7911uxc->pixel_rate,
				lt7911uxc->byte_rate);

		__v4l2_ctrl_s_ctrl(lt7911uxc->link_freq,
			lt7911uxc->cur_mode->mipi_freq_idx);

		dev_info(&lt7911uxc->i2c_client->dev, "%s: mode->mipi_freq_idx(%d)",
				__func__, lt7911uxc->cur_mode->mipi_freq_idx);

		dev_info(&lt7911uxc->i2c_client->dev, "%s: fmt code:%d, w:%d, h:%d, field code:%d\n",
				__func__, format->format.code, format->format.width,
				format->format.height, format->format.field);
		ret = 0;
	} else {
		dev_info(&lt7911uxc->i2c_client->dev, "lt7911uxc_get_fmt failed\n");
	}

	return ret;
}

static int lt7911uxc_enum_frame_interval(struct v4l2_subdev *sd,
						struct v4l2_subdev_state *state,
						struct v4l2_subdev_frame_interval_enum *fie)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);

	if (fie->index >= lt7911uxc->cfg_num)
		return -EINVAL;

	fie->code = LT7911UXC_MEDIA_BUS_FMT;

	fie->width = lt7911uxc->support_modes[fie->index].width;
	fie->height = lt7911uxc->support_modes[fie->index].height;
	fie->interval = lt7911uxc->support_modes[fie->index].max_fps;

	return 0;
}

static int lt7911uxc_set_fmt(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *state,
				struct v4l2_subdev_format *format)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	const struct lt7911uxc_mode *mode;

	/* is overwritten by get_fmt */
	u32 code = format->format.code;
	int ret = lt7911uxc_get_fmt(sd, state, format);

	format->format.code = code;

	if (ret)
		return ret;
#ifdef SKY1_SOC
	switch (code) {
	case LT7911UXC_MEDIA_BUS_FMT:
		break;
	default:
		return -EINVAL;
	}
#endif
	if (format->which == V4L2_SUBDEV_FORMAT_TRY)
		return 0;

	lt7911uxc->mbus_fmt_code = format->format.code;
	mode = lt7911uxc_find_best_fit(lt7911uxc);
	lt7911uxc->cur_mode = mode;
	dev_info(&lt7911uxc->i2c_client->dev, "lt7911uxc_set_fmt sus\n");

	return 0;
}

static int lt7911uxc_g_frame_interval(struct v4l2_subdev *sd,
			struct v4l2_subdev_frame_interval *fi)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	const struct lt7911uxc_mode *mode = lt7911uxc->cur_mode;

	mutex_lock(&lt7911uxc->confctl_mutex);
	fi->interval = mode->max_fps;
	mutex_unlock(&lt7911uxc->confctl_mutex);

	return 0;
}

static int lt7911uxc_s_frame_interval(struct v4l2_subdev *sd,
			struct v4l2_subdev_frame_interval *fi)
{
	return 0;
}

static int lt7911uxc_s_power(struct v4l2_subdev *sd, int on)
{
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);
	int ret = 0;

	if (lt7911uxc->power_on == true) {
		return 0;
	}

	mutex_lock(&lt7911uxc->confctl_mutex);

	if (on)
		lt7911uxc->power_on = true;
	else
		lt7911uxc->power_on = false;

	mutex_unlock(&lt7911uxc->confctl_mutex);
	return ret;
}

static const struct v4l2_subdev_core_ops lt7911uxc_core_ops = {
	.s_power = lt7911uxc_s_power,
	.subscribe_event = lt7911uxc_subscribe_event,
	.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops lt7911uxc_video_ops = {
	.g_input_status = lt7911uxc_g_input_status,
	.s_stream = lt7911uxc_s_stream,
	.g_frame_interval = lt7911uxc_g_frame_interval,
	.s_frame_interval = lt7911uxc_s_frame_interval,
};

static const struct v4l2_subdev_pad_ops lt7911uxc_pad_ops = {
	.enum_mbus_code = lt7911uxc_enum_mbus_code,
	.enum_frame_size = lt7911uxc_enum_frame_sizes,
	.enum_frame_interval = lt7911uxc_enum_frame_interval,
	.set_fmt = lt7911uxc_set_fmt,
	.get_fmt = lt7911uxc_get_fmt,
	.get_mbus_config = lt7911uxc_g_mbus_config,
};

static const struct v4l2_subdev_ops lt7911uxc_ops = {
	.core = &lt7911uxc_core_ops,
	.video = &lt7911uxc_video_ops,
	.pad = &lt7911uxc_pad_ops,
};

static int lt7911uxc_parse(struct lt7911uxc *lt7911uxc)
{
	struct device *dev = &lt7911uxc->i2c_client->dev;
	struct fwnode_handle *endpoint;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_gpio;
	int ret;

	/* Parse endpoint */
	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(&lt7911uxc->i2c_client->dev), NULL);
	if (!endpoint) {
		dev_err(&lt7911uxc->i2c_client->dev, "endpoint node not found\n");
		return -EINVAL;
	}

	ret = v4l2_fwnode_endpoint_parse(endpoint, &lt7911uxc->ep);
	fwnode_handle_put(endpoint);
	if (ret) {
		dev_err(&lt7911uxc->i2c_client->dev, "Could not parse endpoint\n");
		return ret;
	}

	lt7911uxc->power_gpio = devm_gpiod_get_optional(dev, "power",
			GPIOD_OUT_HIGH);
	if (IS_ERR(lt7911uxc->power_gpio)) {
		dev_err(dev, "failed to get power gpio\n");
		ret = PTR_ERR(lt7911uxc->power_gpio);
		return ret;
	}

	lt7911uxc->power1_gpio = devm_gpiod_get_optional(dev, "power1",
			GPIOD_OUT_HIGH);
	if (IS_ERR(lt7911uxc->power1_gpio)) {
		dev_err(dev, "failed to get power1 gpio\n");
		ret = PTR_ERR(lt7911uxc->power_gpio);
		return ret;
	}

	lt7911uxc->reset_gpio = devm_gpiod_get_optional(dev, "reset",
			GPIOD_OUT_HIGH);
	if (IS_ERR(lt7911uxc->reset_gpio)) {
		dev_err(dev, "failed to get reset gpio\n");
		ret = PTR_ERR(lt7911uxc->reset_gpio);
		return ret;
	}

	lt7911uxc->pwdn_gpio = devm_gpiod_get_optional(dev, "pwdn",
			GPIOD_OUT_HIGH);
	if (IS_ERR(lt7911uxc->pwdn_gpio)) {
		dev_err(dev, "failed to get pwdn gpio\n");
		ret = PTR_ERR(lt7911uxc->pwdn_gpio);
		return ret;
	}

	pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR(pinctrl)) {
		dev_err(dev, "failed to get pinctrl\n");
		return -1;
	}

	pins_default = pinctrl_lookup_state(pinctrl,PINCTRL_STATE_DEFAULT);
	if (IS_ERR(pins_default)) {
		dev_err(dev, "failed to get default state \n");
		return -1;
	}

	pins_gpio = pinctrl_lookup_state(pinctrl, "gpio");
	if (IS_ERR(pins_gpio)) {
		dev_err(dev, "failed to get gpio state \n");
		return -1;
	}

	pinctrl_select_state(pinctrl, pins_gpio);

	lt7911uxc->pinctrl = pinctrl;
	lt7911uxc->pins_gpio = pins_gpio;
	lt7911uxc->pins_default = pins_default;

	lt7911uxc->support_modes = supported_modes_dphy;
	lt7911uxc->cfg_num = ARRAY_SIZE(supported_modes_dphy);
	lt7911uxc->enable_hdcp = false;

	return ret;
}

static int lt7911uxc_power_on(struct lt7911uxc *lt7911uxc)
{
	struct device *dev = &lt7911uxc->i2c_client->dev;

	dev_info(dev, "lt7911uxc power on\n");

	gpiod_set_value(lt7911uxc->reset_gpio, 0);
	gpiod_set_value(lt7911uxc->power_gpio, 0);
	gpiod_set_value(lt7911uxc->power1_gpio,0);
	gpiod_set_value(lt7911uxc->pwdn_gpio, 0);

	usleep_range(20000, 25000);

	gpiod_set_value(lt7911uxc->power_gpio, 1);
	gpiod_set_value(lt7911uxc->power1_gpio, 1);

	//delay 20ms before reset
	usleep_range(25000, 30000);
	gpiod_set_value(lt7911uxc->reset_gpio, 1);
	gpiod_set_value(lt7911uxc->pwdn_gpio, 1);
	usleep_range(25000, 30000);

	return 0;
}

static void lt7911uxc_power_off(struct lt7911uxc *lt7911uxc)
{
	struct device *dev = &lt7911uxc->i2c_client->dev;

	dev_info(dev, "lt7911uxc power off\n");

	gpiod_set_value(lt7911uxc->reset_gpio, 0);
	gpiod_set_value(lt7911uxc->power_gpio, 0);
	gpiod_set_value(lt7911uxc->power1_gpio,0);
	gpiod_set_value(lt7911uxc->pwdn_gpio, 0);

}

static int lt7911uxc_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);

	pinctrl_select_state(lt7911uxc->pinctrl,lt7911uxc->pins_gpio);

	return lt7911uxc_power_on(lt7911uxc);
}

static int lt7911uxc_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);

	lt7911uxc_power_off(lt7911uxc);
	pinctrl_select_state(lt7911uxc->pinctrl,lt7911uxc->pins_default);

	return 0;
}

static const struct dev_pm_ops lt7911uxc_pm_ops = {

	.suspend = lt7911uxc_suspend,
	.resume = lt7911uxc_resume,
};

static int lt7911uxc_init_v4l2_ctrls(struct lt7911uxc *lt7911uxc)
{
	const struct lt7911uxc_mode *mode;
	struct v4l2_subdev *sd;
	int ret;

	mode = lt7911uxc->cur_mode;
	sd = &lt7911uxc->sd;
	ret = v4l2_ctrl_handler_init(&lt7911uxc->hdl, 2);
	if (ret)
		return ret;

	lt7911uxc->link_freq = v4l2_ctrl_new_int_menu(&lt7911uxc->hdl, NULL,
			V4L2_CID_LINK_FREQ,
			ARRAY_SIZE(link_freq_menu_items) - 1, 0,
			link_freq_menu_items);
	lt7911uxc->pixel_rate = v4l2_ctrl_new_std(&lt7911uxc->hdl, NULL,
			V4L2_CID_PIXEL_RATE,
			0, LT7911UXC_PIXEL_RATE, 1, LT7911UXC_PIXEL_RATE);

	sd->ctrl_handler = &lt7911uxc->hdl;
	if (lt7911uxc->hdl.error) {
		ret = lt7911uxc->hdl.error;
		v4l2_err(sd, "cfg v4l2 ctrls failed! ret:%d\n", ret);
		return ret;
	}

	__v4l2_ctrl_s_ctrl(lt7911uxc->link_freq, mode->mipi_freq_idx);
	__v4l2_ctrl_s_ctrl_int64(lt7911uxc->pixel_rate, LT7911UXC_PIXEL_RATE);

	return 0;
}

static int lt7911uxc_check_chip_id(struct lt7911uxc *lt7911uxc)
{
	struct device *dev = &lt7911uxc->i2c_client->dev;
	struct v4l2_subdev *sd = &lt7911uxc->sd;
	u8 id[2];
	u32 chipid;
	int ret = 0;

	lt7911uxc_i2c_enable(sd);
	lt7911_select_page(sd,GET_PAGE_ID(CHIPID_REGL));
	i2c_rd(sd,GET_REG(CHIPID_REGL),id,2);
	lt7911uxc_i2c_disable(sd);

	chipid = (id[1] << 8) | id[0];
	if (chipid != LT7911UXC_CHIPID) {
		dev_err(dev, "chipid err, read:%#x, expect:%#x\n",
				chipid, LT7911UXC_CHIPID);
		return -EINVAL;
	}

	dev_info(dev, "check chipid ok, id:%#x", chipid);

	return ret;
}

static void lt7911uxc_audio_handle_plugged_change(struct lt7911uxc_audio *lt7911uxc_aud,
						  bool plugged)
{
	if (lt7911uxc_aud->codec_dev && lt7911uxc_aud->plugged_cb)
		lt7911uxc_aud->plugged_cb(lt7911uxc_aud->codec_dev, plugged);
}

static int lt7911uxc_audio_startup(struct device *dev, void *data)
{
	return 0;
}

static void lt7911uxc_audio_shutdown(struct device *dev, void *data)
{
	return;
}

static int lt7911uxc_audio_hw_params(struct device *dev, void *data,
				     struct hdmi_codec_daifmt *daifmt,
				     struct hdmi_codec_params *params)
{
	struct lt7911uxc *lt7911uxc = dev_get_drvdata(dev);
	struct v4l2_subdev *sd = &lt7911uxc->sd;
	u8 fs_h_v, fs_l_v, ch_v;

	lt7911uxc_i2c_enable(sd);
	fs_h_v  = i2c_rd8(sd, AUDIO_FS_VALUE_H);
	fs_l_v  = i2c_rd8(sd, AUDIO_FS_VALUE_L);
	ch_v    = i2c_rd8(sd, AUDIO_CH);
	lt7911uxc_i2c_disable(sd);

	dev_dbg(dev,
		"%s, daifmt fmt:%d, bit_clk_inv:%d, frame_clk_inv:%d, bit_clk_provider:%d, frame_clk_provider:%d, params sample_rate:%d, sample_width:%d, channels:%d\n",
		__func__,
		daifmt->fmt, daifmt->bit_clk_inv, daifmt->frame_clk_inv,
		daifmt->bit_clk_provider, daifmt->frame_clk_provider,
		params->sample_rate, params->sample_width, params->channels);

	return 0;
}

static int lt7911uxc_audio_get_dai_id(struct snd_soc_component *comment,
				      struct device_node *endpoint)
{
	return 0;
}

static int lt7911uxc_audio_hook_plugged_cb(struct device *dev, void *data,
					   hdmi_codec_plugged_cb fn,
					   struct device *codec_dev)
{
	struct lt7911uxc *lt7911uxc = dev_get_drvdata(dev);
	struct lt7911uxc_audio *lt7911uxc_aud = &lt7911uxc->lt_audio;

	lt7911uxc_aud->plugged_cb = fn;
	lt7911uxc_aud->codec_dev = codec_dev;
	/* TODO: change to hot plug */
	lt7911uxc_audio_handle_plugged_change(lt7911uxc_aud, 1);

	return 0;
}

static const struct hdmi_codec_ops lt7911uxc_audio_codec_ops = {
	.hw_params = lt7911uxc_audio_hw_params,
	.audio_startup = lt7911uxc_audio_startup,
	.audio_shutdown = lt7911uxc_audio_shutdown,
	.get_dai_id = lt7911uxc_audio_get_dai_id,
	.hook_plugged_cb = lt7911uxc_audio_hook_plugged_cb
};

int lt7911uxc_register_audio_device(struct lt7911uxc *lt7911uxc)
{
	struct lt7911uxc_audio *lt7911uxc_aud = &lt7911uxc->lt_audio;
	struct hdmi_codec_pdata codec_data = {
		.ops = &lt7911uxc_audio_codec_ops,
		.spdif = 0,
		.i2s = 1,
		.max_i2s_channels = 8,
		.data = lt7911uxc,
	};

	lt7911uxc_aud->pdev = platform_device_register_data(lt7911uxc->dev,
						 HDMI_CODEC_DRV_NAME,
						 PLATFORM_DEVID_AUTO,
						 &codec_data,
						 sizeof(codec_data));

	return PTR_ERR_OR_ZERO(lt7911uxc_aud->pdev);
}

void lt7911uxc_unregister_audio_device(void *data)
{
	struct lt7911uxc *lt7911uxc = data;
	struct lt7911uxc_audio *lt7911uxc_aud = &lt7911uxc->lt_audio;

	if (lt7911uxc_aud->pdev) {
		platform_device_unregister(lt7911uxc_aud->pdev);
		lt7911uxc_aud->pdev = NULL;
	}
}

static int lt7911uxc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct v4l2_dv_timings default_timing =
				V4L2_DV_BT_CEA_640X480P59_94;
	struct lt7911uxc *lt7911uxc;
	struct v4l2_subdev *sd;
	struct device *dev = &client->dev;
	int ret;

	lt7911uxc = devm_kzalloc(dev, sizeof(struct lt7911uxc), GFP_KERNEL);
	if (!lt7911uxc)
		return -ENOMEM;

	lt7911uxc->i2c_client = client;
	lt7911uxc->dev = dev;
	lt7911uxc->tranning_ready = 0;
	lt7911uxc->tranning_enable = 0;
	lt7911uxc->active_num = 1;
	lt7911uxc->mbus_fmt_code = LT7911UXC_MEDIA_BUS_FMT;
	ret = lt7911uxc_parse(lt7911uxc);
	if (ret) {
		v4l2_err(sd, "lt7911uxc_parse failed! err:%d\n", ret);
		return ret;
	}

	lt7911uxc->timings = default_timing;
	lt7911uxc->cur_mode = &lt7911uxc->support_modes[0];

	/* FPGA: power will be controlled in download.tcl */
	lt7911uxc_power_on(lt7911uxc);

	ret = lt7911uxc_check_chip_id(lt7911uxc);
	if (ret < 0)
		return ret;

	INIT_WORK(&lt7911uxc->work_i2c_poll, lt7911uxc_work_i2c_poll);
	timer_setup(&lt7911uxc->timer, lt7911uxc_irq_poll_timer, 0);
	lt7911uxc->timer.expires = jiffies +
		msecs_to_jiffies(POLL_INTERVAL_MS);

	add_timer(&lt7911uxc->timer);

	ret = lt7911uxc_init_v4l2_ctrls(lt7911uxc);
	if (ret)
		return ret;

	sd = &lt7911uxc->sd;
	snprintf(sd->name, sizeof(sd->name), "%s",
			CIX_LT7911UXC_SUBDEV_NAME);

	v4l2_i2c_subdev_init(sd, client, &lt7911uxc_ops);

#if defined(CONFIG_MEDIA_CONTROLLER)
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	lt7911uxc->pad[LT7911UXC_SD_PAD_SOURCE_CSI].flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&sd->entity, LT7911UXC_SD_PAD_MAX_NUM, lt7911uxc->pad);
	if (ret) {
		dev_err(dev, "pads init failed %d", ret);
		return ret;
	}
#endif
	/* register v4l2_subdev device */
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_IS_I2C;
	lt7911uxc->cfg_num = ARRAY_SIZE(supported_modes_dphy);
	lt7911uxc->support_modes = supported_modes_dphy;
	ret = v4l2_async_register_subdev(sd);
	if (ret) {
		dev_err(dev, "lt7911uxc subdev registration failed\n");
		goto error;
	}

	ret = lt7911uxc_register_audio_device(lt7911uxc);
	dev_info(dev, "%s:%d, register audio device, ret:%d\n", __func__, __LINE__, ret);
	dev_info(dev, "lt7911 probe exit %s \n",ret == 0 ? "success":"failed");

	return 0;
error:
	return ret;
}

static void lt7911uxc_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct lt7911uxc *lt7911uxc = to_lt7911uxc(sd);

	dev_info(sd->dev,"lt7911 remove enter\n");

	lt7911uxc_unregister_audio_device(lt7911uxc);
	v4l2_ctrl_handler_free(&lt7911uxc->hdl);

	del_timer_sync(&lt7911uxc->timer);
	flush_work(&lt7911uxc->work_i2c_poll);

#if defined(CONFIG_MEDIA_CONTROLLER)
	media_entity_cleanup(&sd->entity);
#endif
	v4l2_async_unregister_subdev(sd);
	dev_info(sd->dev,"lt7911 remove exit\n");

	return;
}
#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id lt7911uxc_of_match[] = {
	{ .compatible = "lontium,lt7911uxc" },
	{},
};
MODULE_DEVICE_TABLE(of, lt7911uxc_of_match);
#endif

static const struct acpi_device_id lt7911uxc_acpi_match[] = {
	{ .id = "CIXH302C", .driver_data = 0 },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(acpi, lt7911uxc_acpi_match);

static struct i2c_driver lt7911uxc_driver = {
	.driver = {
		.name = LT7911UXC_NAME,
		.pm = &lt7911uxc_pm_ops,
		.of_match_table = of_match_ptr(lt7911uxc_of_match),
		.acpi_match_table = ACPI_PTR(lt7911uxc_acpi_match),
	},
	.probe = lt7911uxc_probe,
	.remove = lt7911uxc_remove,
};

static int __init lt7911uxc_driver_init(void)
{
	return i2c_add_driver(&lt7911uxc_driver);
}

static void __exit lt7911uxc_driver_exit(void)
{
	i2c_del_driver(&lt7911uxc_driver);
}

device_initcall_sync(lt7911uxc_driver_init);
module_exit(lt7911uxc_driver_exit);

MODULE_DESCRIPTION("Lontium lt7911uxc DP/type-c to CSI-2 bridge driver");
MODULE_AUTHOR("camera team of CIX");
MODULE_LICENSE("GPL");

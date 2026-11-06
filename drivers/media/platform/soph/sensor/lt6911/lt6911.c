// SPDX-License-Identifier: GPL-2.0
/*
 * lt6911 driver
 *
 * Copyright (C) 2024 Sophon Co., Ltd.
 *
 */
#include <linux/clk.h>
#include <linux/acpi.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/pm_runtime.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-device.h>
#include <media/v4l2-fwnode.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio/consumer.h>

#include <linux/comm_cif.h>
#include <linux/sns_v4l2_uapi.h>

#include "lt6911.h"

/* I2C per write of bits */
#define REG_VALUE_08BIT		1
#define REG_VALUE_16BIT		2
#define REG_VALUE_24BIT		3

/* Chip ID */
#define LT6911_CHIP_ID_ADDR_H	0xa000
#define LT6911_CHIP_ID_ADDR_L	0xa001

#define LT6911_FRAME_HALF_PIXEL_CLOCK_H	0xe085
#define LT6911_FRAME_HALF_PIXEL_CLOCK_M	0xe086
#define LT6911_FRAME_HALF_PIXEL_CLOCK_L	0xe087
#define LT6911_FRAME_HALF_WIDTH_ADDR_H	0xe08c
#define LT6911_FRAME_HALF_WIDTH_ADDR_L	0xe08d
#define LT6911_FRAME_HEIGHT_ADDR_H	0xe08e
#define LT6911_FRAME_HEIGHT_ADDR_L	0xe08f

#define LT6911_CHIP_ID			0x1605
#define LT6911_I2C_BANK_ADDR		0xff
/*Sensor type for isp middleware*/
int LT6911_SNS_TYPE_SDR = V4L2_LONTIUM_MIPI_LT6911_8M_60FPS_8BIT;

static const enum mipi_wdr_mode_e lt6911_wdr_mode = MIPI_WDR_MODE_NONE;

static int lt6911_count;
static int force_bus[MAX_SENSOR_DEVICE] = {[0 ... (MAX_SENSOR_DEVICE - 1)] = -1};
module_param_array(force_bus, int, &lt6911_count, 0644);

static int lt6911_probe_index;
static const unsigned short lt6911_i2c_list[] = {0x2b};
static int lt6911_bus_map[MAX_SENSOR_DEVICE] = {1, -1, -1, -1, -1, -1};

struct lt6911_reg_list {
	u32 num_of_regs;
	const struct lt6911_reg *regs;
};

/* Mode : resolution and related config&values */
struct lt6911_mode {
	u32 max_width;
	u32 max_height;
	u32 width;
	u32 height;
	u32 hts_def;
	u32 vts_def;
	u32 exp_def;
	u32 mipi_wdr_mode;
	struct v4l2_fract max_fps;
	struct v4l2_fract wdr_max_fps;
	sns_sync_info_t lt6911_sync_info;
	struct lt6911_reg_list reg_list;
	struct lt6911_reg_list wdr_reg_list;
};

/* Mode configs */
static struct lt6911_mode supported_modes[] = {
	{
		.max_width = 3840,
		.max_height = 2160,
		.width = 3840,
		.height = 2160,
		.exp_def = 0x2000,
		.hts_def = 2160,
		.vts_def = 3840,
		.max_fps = {
			.numerator = 10000,
			.denominator = 600000,
		},
		.reg_list = {
			.num_of_regs = ARRAY_SIZE(mode_3840x2160_8bit_regs),
			.regs = mode_3840x2160_8bit_regs,
		},
	},
};

struct lt6911 {
	struct v4l2_subdev sd;
	struct media_pad pad;
	struct i2c_client *client;
	struct v4l2_ctrl_handler ctrl_handler;
	/* V4L2 Controls */
	struct v4l2_ctrl *vblank;
	struct v4l2_ctrl *hblank;
	struct v4l2_ctrl *exposure;
	/* Current mode */
	struct lt6911_mode *cur_mode;
	/* Mutex for serialized access */
	struct mutex mutex;
	/* Streaming on/off */
	bool streaming;
	/*dtsi config*/
	struct clk       *xvclk;
	struct gpio_desc *power_gpio;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *irq_gpio;
	struct pinctrl   *pinctrl;

	unsigned int lane_num;
	unsigned int module_index;
};

#define to_lt6911(_sd)	container_of(_sd, struct lt6911, sd)

static u32 lt6911_i2c_read(struct lt6911 *lt6911, u32 addr);
static u32 lt6911_i2c_write(struct lt6911 *lt6911, u32 addr, u32 data);
u32 lt6911_read(struct lt6911 *lt6911, u32 addr);
u32 lt6911_write(struct lt6911 *lt6911, u32 addr, u32 data);


/* Read registers up to 4 at a time */
static int lt6911_read_reg(struct lt6911 *lt6911, u8 reg, u32 len,
			    u32 *val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);
	struct i2c_msg msgs[2];
	u8 *data_be_p;
	int ret;
	__be32 data_be = 0;
	u8 reg_addr_be = reg;

	if (len > 5)
		return -EINVAL;

	data_be_p = (u8 *)&data_be;
	/* Write register address */
	msgs[0].addr = client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = (u8 *)&reg_addr_be;

	/* Read data from register */
	msgs[1].addr = client->addr;
	msgs[1].flags =  I2C_M_RD;
	msgs[1].len = len;
	msgs[1].buf = &data_be_p[4 - len];

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	*val = be32_to_cpu(data_be);

	return 0;
}

/* Write registers up to 4 at a time */
static int lt6911_write_reg(struct lt6911 *lt6911, u16 reg, u32 len,
			     u32 __val)
{
	struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);
	int buf_i, val_i;
	u8 buf[6], *val_p;
	__be32 val;

	if (len > 4)
		return -EINVAL;

	buf[0] = reg;

	val = cpu_to_be32(__val);
	val_p = (u8 *)&val;
	buf_i = 1;
	val_i = 4 - len;

	while (val_i < 4)
		buf[buf_i++] = val_p[val_i++];

	if (i2c_master_send(client, buf, len + 1) != len + 1)
		return -EIO;

	return 0;
}

/* Write a list of registers */
static int lt6911_write_regs(struct lt6911 *lt6911,
			      const struct lt6911_reg *regs, u32 len)
{
	struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);
	int ret;
	u32 i;

	for (i = 0; i < len; i++) {
		ret = lt6911_write(lt6911, regs[i].address, regs[i].val);
		if (ret) {
			dev_err_ratelimited(&client->dev, "Failed to write reg 0x%4.4x. error=%d\n",
					    regs[i].address, ret);

			return ret;
		}
	}

	return 0;
}

u32 lt6911_i2c_read(struct lt6911 *lt6911, u32 addr)
{
	u8 bank = addr >> 8;
	u8 high_addr = addr & 0xff;
	u32 data;
	lt6911_write_reg(lt6911, LT6911_I2C_BANK_ADDR, REG_VALUE_08BIT, bank);
	lt6911_read_reg(lt6911, high_addr, REG_VALUE_08BIT, &data);
	return data;
}

u32 lt6911_i2c_write(struct lt6911 *lt6911, u32 addr, u32 data)
{
	uint8_t bank = addr >> 8;
	uint8_t high_addr = addr & 0xff;

	lt6911_write_reg(lt6911, LT6911_I2C_BANK_ADDR, REG_VALUE_08BIT, bank);
	return lt6911_write_reg(lt6911, high_addr, REG_VALUE_08BIT, data);
}

u32 lt6911_read(struct lt6911 *lt6911, u32 addr)
{
	u32 data = 0;

	lt6911_i2c_write(lt6911, 0x80ee, 0x01);
	data = lt6911_i2c_read(lt6911, addr);
	lt6911_i2c_write(lt6911, 0x80ee, 0x00);
	return data;
}

u32 lt6911_write(struct lt6911 *lt6911, u32 addr, u32 data)
{
	lt6911_i2c_write(lt6911, 0x80ee, 0x01);
	lt6911_i2c_write(lt6911, addr, data);
	lt6911_i2c_write(lt6911, 0x80ee, 0x00);
	return 0;
}

/* Open sub-device */
static int lt6911_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	struct lt6911 *lt6911 = to_lt6911(sd);
	struct v4l2_mbus_framefmt *try_fmt =
			v4l2_subdev_get_try_format(sd, fh->pad, 0);

	mutex_lock(&lt6911->mutex);

	/* Initialize try_fmt */
	try_fmt->width = lt6911->cur_mode->width;
	try_fmt->height = lt6911->cur_mode->height;
	try_fmt->code = MEDIA_BUS_FMT_SGRBG10_1X10;
	try_fmt->field = V4L2_FIELD_NONE;

	/* No crop or compose */
	mutex_unlock(&lt6911->mutex);

	return 0;
}

static int lt6911_set_ctrl(struct v4l2_ctrl *ctrl)
{
	struct lt6911 *lt6911 = container_of(ctrl->handler,
					       struct lt6911, ctrl_handler);
	struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);

	pm_runtime_put(&client->dev);

	return 0;
}

static const struct v4l2_ctrl_ops lt6911_ctrl_ops = {
	.s_ctrl = lt6911_set_ctrl,
};

static int g_mbus_config(struct v4l2_subdev *sd, unsigned int pad_id,
			 struct v4l2_mbus_config *config)
{
	config->type = V4L2_MBUS_CSI2_DPHY;
	return 0;
}

static int enum_frame_interval(struct v4l2_subdev *sd,
			      struct v4l2_subdev_pad_config *cfg,
			      struct v4l2_subdev_frame_interval_enum *fie)
{
	struct lt6911 *lt6911 = to_lt6911(sd);

	fie->width  = lt6911->cur_mode->width;
	fie->height = lt6911->cur_mode->height;

	if (lt6911->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {//linear
		fie->interval.numerator   = lt6911->cur_mode->max_fps.numerator;
		fie->interval.denominator = lt6911->cur_mode->max_fps.denominator;
	}

	return 0;
}

static int enum_mbus_code(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_mbus_code_enum *code)
{
	/* Only one bayer order(GRBG) is supported */
	if (code->index > 0)
		return -EINVAL;

	code->code = MEDIA_BUS_FMT_SGRBG10_1X10;

	return 0;
}

void auto_get_imgage_size(struct lt6911 *lt6911) {

	int height_h, height_l, width_h, width_l, height, width;
	int pixel_clock_h, pixel_clock_m, pixel_clock_l, pixel_clock;
	int tens_digit;

	width_h = lt6911_read(lt6911, LT6911_FRAME_HALF_WIDTH_ADDR_H);
	width_l = lt6911_read(lt6911, LT6911_FRAME_HALF_WIDTH_ADDR_L);
	width = (((width_h & 0xFF) << 8) | (width_l & 0xFF)) * 2;

	height_h = lt6911_read(lt6911, LT6911_FRAME_HEIGHT_ADDR_H);
	height_l = lt6911_read(lt6911, LT6911_FRAME_HEIGHT_ADDR_L);
	height = (((height_h & 0xFF) << 8) | (height_l & 0xFF));

	pixel_clock_h = lt6911_read(lt6911, LT6911_FRAME_HALF_PIXEL_CLOCK_H);
	pixel_clock_m = lt6911_read(lt6911, LT6911_FRAME_HALF_PIXEL_CLOCK_M);
	pixel_clock_l = lt6911_read(lt6911, LT6911_FRAME_HALF_PIXEL_CLOCK_L);
	pixel_clock = ((pixel_clock_h << 16) | (pixel_clock_m << 8) | pixel_clock_l) * 1000 * 2;

	tens_digit = pixel_clock/(width*height);
	tens_digit /= 10;
	switch(tens_digit) {
		case 6:
			LT6911_SNS_TYPE_SDR = V4L2_LONTIUM_MIPI_LT6911_8M_60FPS_8BIT;
			pr_info("Set fps = 60 on the input\n");
		break;
		case 3:
			LT6911_SNS_TYPE_SDR = V4L2_LONTIUM_MIPI_LT6911_8M_30FPS_8BIT;
			pr_info("Set fps = 30 on the input\n");
		break;
		default:
			LT6911_SNS_TYPE_SDR = V4L2_LONTIUM_MIPI_LT6911_8M_30FPS_8BIT;
			printk("no suitable fps, default fps = 30\n");
		break;
    	}

	lt6911->cur_mode->width = width;
	lt6911->cur_mode->max_width = width;
	lt6911->cur_mode->height = height;
	lt6911->cur_mode->max_height = height;

	pr_info("after irq width = %d, height = %d\n", width, height);
}

static int enum_frame_size(struct v4l2_subdev *sd,
			   struct v4l2_subdev_pad_config *cfg,
			   struct v4l2_subdev_frame_size_enum *fse)
{
	struct lt6911 *lt6911 = to_lt6911(sd);

	fse->min_width = lt6911->cur_mode->width;
	fse->max_width = lt6911->cur_mode->max_width;
	fse->min_height = lt6911->cur_mode->height;
	fse->max_height = lt6911->cur_mode->max_height;

	return 0;
}

static void update_pad_format(const struct lt6911_mode *mode, struct v4l2_subdev_format *fmt)
{
	fmt->format.width = mode->width;
	fmt->format.height = mode->height;
	fmt->format.code = MEDIA_BUS_FMT_SGRBG10_1X10;
	fmt->format.field = V4L2_FIELD_NONE;
}

static int get_pad_format(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct lt6911 *lt6911 = to_lt6911(sd);
	struct v4l2_mbus_framefmt *framefmt;
	int ret = 0;

	mutex_lock(&lt6911->mutex);

	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		fmt->format = *framefmt;
		ret = -ENOTTY;
	} else {
		update_pad_format(lt6911->cur_mode, fmt);
	}
	mutex_unlock(&lt6911->mutex);

	return ret;
}

static int set_pad_format(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *fmt)
{
	struct lt6911 *lt6911 = to_lt6911(sd);
	struct lt6911_mode *mode;
	struct v4l2_mbus_framefmt *framefmt;

	mutex_lock(&lt6911->mutex);

	/* Only one raw bayer(GRBG) order is supported */
	if (fmt->format.code != MEDIA_BUS_FMT_SGRBG10_1X10)
		fmt->format.code = MEDIA_BUS_FMT_SGRBG10_1X10;

	mode = v4l2_find_nearest_size(supported_modes,
				      ARRAY_SIZE(supported_modes),
				      width, height,
				      fmt->format.width, fmt->format.height);
	update_pad_format(mode, fmt);
	if (fmt->which == V4L2_SUBDEV_FORMAT_TRY) {
		framefmt = v4l2_subdev_get_try_format(sd, cfg, fmt->pad);
		*framefmt = fmt->format;
	} else {
		lt6911->cur_mode = mode;
	}

	mutex_unlock(&lt6911->mutex);

	return 0;
}

static void lt6911uxe_reset(struct lt6911 *lt6911)
{
	static int cnt = 0;
	if (!cnt) {
		gpiod_set_value(lt6911->reset_gpio, 0);
		usleep_range(20000, 21000);
		gpiod_set_value(lt6911->reset_gpio, 1);
		usleep_range(20000, 21000);
		gpiod_set_value(lt6911->reset_gpio, 0);
		usleep_range(20000, 21000);
		cnt++;
	}
}

/* Start streaming */
static int start_streaming(struct lt6911 *lt6911)
{
	const sns_sync_info_t *sync_info;
	const struct lt6911_reg_list *reg_list;
	int ret;
	struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);
	
	lt6911uxe_reset(lt6911);

	if (lt6911->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {//linear
		reg_list = &lt6911->cur_mode->reg_list;
	}

	ret = lt6911_write_regs(lt6911, reg_list->regs, reg_list->num_of_regs);
	if (ret) {
		dev_err(&client->dev, "%s failed to set mode\n", __func__);
		return ret;
	}

	sync_info = &lt6911->cur_mode->lt6911_sync_info;

	if (sync_info->num_of_regs > 0) {
		ret = lt6911_write_regs(lt6911, (struct lt6911_reg *)sync_info->regs,
					sync_info->num_of_regs);
		if (ret) {
			dev_err(&client->dev, "%s failed to set default\n", __func__);
			return ret;
		}
	}


	usleep_range(1000 * 1000, 2000 * 1000);
	/* Apply customized values from user */
	ret =  __v4l2_ctrl_handler_setup(lt6911->sd.ctrl_handler);
	if (ret)
		return ret;

	dev_info(&client->dev, "wdr_mode(%d) reg setting done\n", lt6911->cur_mode->mipi_wdr_mode);

	return ret;
}

/* Stop streaming */
static int stop_streaming(struct lt6911 *lt6911)
{
	return 0;
}

static int set_stream(struct v4l2_subdev *sd, int enable)
{
	struct lt6911 *lt6911 = to_lt6911(sd);
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int ret = 0;

	mutex_lock(&lt6911->mutex);
	if (lt6911->streaming == enable) {
		mutex_unlock(&lt6911->mutex);
		return 0;
	}

	if (enable) {
		ret = pm_runtime_get_sync(&client->dev);
		if (ret < 0) {
			pm_runtime_put_noidle(&client->dev);
			goto err_unlock;
		}

		/*
		 * Apply default & customized values
		 * and then start streaming.
		 */
		ret = start_streaming(lt6911);
		if (ret)
			goto err_rpm_put;
	} else {
		stop_streaming(lt6911);
		pm_runtime_put(&client->dev);
	}

	lt6911->streaming = enable;
	mutex_unlock(&lt6911->mutex);

	dev_info(&client->dev, "set stream(%d) success\n", enable);

	return ret;

err_rpm_put:
	pm_runtime_put(&client->dev);
err_unlock:
	mutex_unlock(&lt6911->mutex);

	return ret;
}

static int __maybe_unused suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct lt6911 *lt6911 = to_lt6911(sd);

	if (lt6911->streaming)
		stop_streaming(lt6911);

	return 0;
}

static int __maybe_unused resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct lt6911 *lt6911 = to_lt6911(sd);
	int ret;

	if (lt6911->streaming) {
		ret = start_streaming(lt6911);
		if (ret)
			goto error;
	}

	return 0;

error:
	stop_streaming(lt6911);
	lt6911->streaming = false;
	return ret;
}

static irqreturn_t lt6911_gpio_irq_thread_handler(int irq, void *data)
{
	auto_get_imgage_size(data);
	return IRQ_HANDLED;
}

/* Verify chip ID */
static int lt6911_identify_module(struct lt6911 *lt6911)
{
	struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);
	int nVal, nVal2;
	int read_data = 0;

	nVal = lt6911_read(lt6911, LT6911_CHIP_ID_ADDR_H);
	dev_info(&client->dev, "read id:0x%x\n", nVal);
	if (nVal == 0)
		return nVal;

	nVal2 = lt6911_read(lt6911, LT6911_CHIP_ID_ADDR_L);
	read_data = (((nVal & 0xFF) << 8) | (nVal2 & 0xFF));
	if (read_data != LT6911_CHIP_ID) {
		dev_err(&client->dev, "chip id(%x) mismatch, read(%x)\n",
			LT6911_CHIP_ID, read_data);
		return -EIO;
	}

	return 0;
}

static int lt6911_update_link_menu(struct lt6911 *lt6911)
{
	struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);
	struct v4l2_ctrl_handler *ctrl_hdlr;
	struct v4l2_ctrl *ctrl;
	int wdr_index = SNS_CFG_TYPE_WDR_MODE;
	int id = lt6911->module_index;
	int ret;
	int i;

	lt6911_link_cif_menu[id][wdr_index] = lt6911->cur_mode->mipi_wdr_mode;

	dev_info(&client->dev, "update mipi_mode:%lld", lt6911_link_cif_menu[id][wdr_index]);

	ctrl_hdlr = lt6911->sd.ctrl_handler;
	v4l2_ctrl_handler_free(ctrl_hdlr);

	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret) {
		dev_err(&client->dev, "%s ctrl handler init failed (%d)\n",
			__func__, ret);
		return ret;
	}

	for (i = 0; i < SNS_CFG_TYPE_MAX; i++) {
		ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr, &lt6911_ctrl_ops, V4L2_CID_LINK_FREQ,
					      lt6911_link_cif_menu[id][i], 0,
					       (const s64 *)lt6911_link_cif_menu[id]);

		if (ctrl)
			ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	}

	if (ctrl_hdlr->error) {
		ret = ctrl_hdlr->error;
		dev_err(&client->dev, "%s new int menu failed (%d)\n",
			__func__, ret);
		goto error;
	}

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdlr);

	return ret;
}

static long lt6911_ioctl(struct v4l2_subdev *sd, unsigned int cmd, void *arg)
{
	struct lt6911 *lt6911 = to_lt6911(sd);
	long ret = 0;

	switch (cmd) {
	case SNS_V4L2_GET_TYPE:
	{
		int type = 0;

		if (lt6911->cur_mode->mipi_wdr_mode == MIPI_WDR_MODE_NONE) {//linear
			type = LT6911_SNS_TYPE_SDR;
		}

		memcpy(arg, &type, sizeof(int));
		break;
	}

	case SNS_V4L2_SET_MIRROR_FLIP:
	{
		struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);

		dev_warn(&client->dev, "Not support Mirror and Filp!\n");
		break;
	}

	case SNS_V4L2_GET_I2C_INFO:
	{
		struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);
		sns_i2c_info_t i2c_info;

		i2c_info.i2c_addr =  client->addr;
		i2c_info.i2c_idx  =  client->adapter->i2c_idx;
		memcpy(arg, &i2c_info, sizeof(sns_i2c_info_t));
		break;
	}

	case SNS_V4L2_SET_HDR_ON:
	{
		int hdr_on = 0;
		struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);

		memcpy(&hdr_on, arg, sizeof(int));

		if (hdr_on)
			dev_warn(&client->dev, "Not support HDR!\n");
		else
			lt6911->cur_mode->mipi_wdr_mode = MIPI_WDR_MODE_NONE;

		lt6911_update_link_menu(lt6911);
		break;
	}

	case SNS_V4L2_SET_SNS_SYNC_INFO:
	{
		memcpy(&lt6911->cur_mode->lt6911_sync_info, arg, sizeof(sns_sync_info_t));
		break;
	}

	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}

#ifdef CONFIG_COMPAT
static long lt6911_compat_ioctl32(struct v4l2_subdev *sd,
				   unsigned int cmd, unsigned long arg)
{
	long ret;

	switch (cmd) {
	default:
		ret = -ENOIOCTLCMD;
		break;
	}

	return ret;
}
#endif

static const struct v4l2_subdev_core_ops lt6911_core_ops = {
	.ioctl = lt6911_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl32 = lt6911_compat_ioctl32,
#endif
};

static const struct v4l2_subdev_video_ops lt6911_video_ops = {
	.s_stream = set_stream,
};

static const struct v4l2_subdev_pad_ops lt6911_pad_ops = {
	.enum_mbus_code = enum_mbus_code,
	.get_fmt = get_pad_format,
	.set_fmt = set_pad_format,
	.enum_frame_size = enum_frame_size,
	.get_mbus_config = g_mbus_config,
	.enum_frame_interval = enum_frame_interval,
};

static const struct v4l2_subdev_ops lt6911_subdev_ops = {
	.core	= &lt6911_core_ops,
	.video  = &lt6911_video_ops,
	.pad    = &lt6911_pad_ops,
};

static const struct media_entity_operations lt6911_subdev_entity_ops = {
	.link_validate = v4l2_subdev_link_validate,
};

static const struct v4l2_subdev_internal_ops lt6911_internal_ops = {
	.open = lt6911_open,
};

/* Initialize control handlers */
static int lt6911_init_controls(struct lt6911 *lt6911, int index_id)
{
	struct i2c_client *client = v4l2_get_subdevdata(&lt6911->sd);
	struct v4l2_fwnode_device_properties props;
	struct v4l2_ctrl_handler *ctrl_hdlr;
	struct v4l2_ctrl *ctrl;
	int ret;
	int i;

	ctrl_hdlr = &lt6911->ctrl_handler;
	ret = v4l2_ctrl_handler_init(ctrl_hdlr, 10);
	if (ret) {
		dev_err(&client->dev, "%s ctrl handler init failed (%d)\n",
			__func__, ret);
		return ret;
	}

	mutex_init(&lt6911->mutex);
	ctrl_hdlr->lock = &lt6911->mutex;
	for (i = 0; i < SNS_CFG_TYPE_MAX; i++) {
		if (i == SNS_CFG_TYPE_WDR_MODE)
			lt6911->cur_mode->mipi_wdr_mode = lt6911_link_cif_menu[index_id][i];

		ctrl = v4l2_ctrl_new_int_menu(ctrl_hdlr, &lt6911_ctrl_ops, V4L2_CID_LINK_FREQ,
					      lt6911_link_cif_menu[index_id][i], 0,
					       (const s64 *)lt6911_link_cif_menu[index_id]);

		if (ctrl)
			ctrl->flags |= V4L2_CTRL_FLAG_READ_ONLY;
	}

	if (ctrl_hdlr->error) {
		ret = ctrl_hdlr->error;
		dev_err(&client->dev, "%s new std menu failed (%d)\n",
			__func__, ret);
		goto error;
	}

	ret = v4l2_fwnode_device_parse(&client->dev, &props);
	if (ret)
		goto error;

	ret = v4l2_ctrl_new_fwnode_properties(ctrl_hdlr, &lt6911_ctrl_ops,
					      &props);
	if (ret)
		goto error;

	lt6911->sd.ctrl_handler = ctrl_hdlr;

	return 0;

error:
	v4l2_ctrl_handler_free(ctrl_hdlr);
	mutex_destroy(&lt6911->mutex);

	return ret;
}

static void lt6911_free_controls(struct lt6911 *lt6911)
{
	v4l2_ctrl_handler_free(lt6911->sd.ctrl_handler);
	mutex_destroy(&lt6911->mutex);
}

static int lt6911_probe(struct i2c_client *client,
			 const struct i2c_device_id *devid)
{
	struct lt6911 *lt6911;
	struct v4l2_subdev *sd;
	struct device *dev = &client->dev;
	int index_id = lt6911_probe_index;
	int addr_num = sizeof(lt6911_i2c_list) / sizeof(unsigned short);
	int bus_id;
	int ret = -1;
	int i;

	dev_info(dev, "probe id[%d] start\n", lt6911_probe_index);

	lt6911_probe_index++;

	if (index_id >= MAX_SENSOR_DEVICE || index_id < 0) {
		dev_info(dev, "invalid devid(%d)\n", index_id);
		return ret;
	}

	lt6911 = devm_kzalloc(&client->dev, sizeof(*lt6911), GFP_KERNEL);
	if (!lt6911)
		return -ENOMEM;

	sd = &lt6911->sd;

	for (i = 0; i < addr_num; i++) {
		if (force_bus[index_id] < 0)
			bus_id = lt6911_bus_map[index_id];
		else
			bus_id = force_bus[index_id];

		if (bus_id < 0 || bus_id > MAX_I2C_BUS_NUM)
			return ret;

		client->addr = lt6911_i2c_list[i];
		client->adapter = i2c_get_adapter(bus_id);
		lt6911->client = client;
		v4l2_i2c_subdev_init(sd, client, &lt6911_subdev_ops);

		/* Check module identity */
		ret = lt6911_identify_module(lt6911);
		if (ret) {
			dev_info(dev, "id[%d] bus[%d] i2c_addr[%d][0x%x] no sensor found\n",
				 index_id, bus_id, i, client->addr);

			if (i == addr_num - 1)
				return ret;

			continue;
		} else {
			dev_info(dev, "id[%d] bus[%d] i2c_addr[0x%x] sensor found\n",
				 index_id, bus_id, client->addr);
			break;
		}
	}

	lt6911->module_index = index_id;

	if (index_id >= ARRAY_SIZE(supported_modes)) {
		lt6911->cur_mode = devm_kzalloc(&client->dev,
						 sizeof(struct lt6911_mode), GFP_KERNEL);
		memcpy(lt6911->cur_mode, &supported_modes[0], sizeof(struct lt6911_mode));
	} else {
		lt6911->cur_mode = devm_kzalloc(&client->dev,
						 sizeof(struct lt6911_mode), GFP_KERNEL);
		memcpy(lt6911->cur_mode, &supported_modes[index_id], sizeof(struct lt6911_mode));
	}

	memset(&lt6911->cur_mode->lt6911_sync_info, 0, sizeof(sns_sync_info_t));

	mutex_init(&lt6911->mutex);

	ret = lt6911_init_controls(lt6911, index_id);
	if (ret)
		return ret;

	/* Initialize subdev */
	sd->internal_ops = &lt6911_internal_ops;
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE;
	sd->entity.ops = &lt6911_subdev_entity_ops;
	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;

	/* Initialize source pad */
	lt6911->pad.flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&sd->entity, 1, &lt6911->pad);
	if (ret) {
		dev_err(&client->dev, "failed to init pads:%d\n", ret);
		goto error_handler_free;
	}

	snprintf(sd->name, sizeof(sd->name), "cam%d_%s %s",
		 lt6911->module_index, "lt6911", dev_name(sd->dev));

	ret = v4l2_async_register_subdev_sensor_common(sd);
	if (ret < 0) {
		dev_err(&client->dev, "failed to async subdev:%d\n", ret);
		goto error_media_entity;
	}

	/*
	 * Device is already turned on by i2c-core with ACPI domain PM.
	 * Enable runtime PM and turn off the device.
	 */
	pm_runtime_set_active(&client->dev);
	pm_runtime_enable(&client->dev);
	pm_runtime_idle(&client->dev);

	dev_info(dev, "sensor_%d probe success\n", index_id);

	lt6911->irq_gpio = devm_gpiod_get(dev, "irq", GPIOD_IN);
	if (IS_ERR(lt6911->irq_gpio)) {
		dev_err(dev, "cannot get irq gpio property\n");
		return PTR_ERR(lt6911->irq_gpio);
	}
	client->irq = gpiod_to_irq(lt6911->irq_gpio);
	if (client->irq < 0) {
		dev_err(&client->dev, "failed to get irq\n");
		return -EINVAL;
	}
	ret = devm_request_threaded_irq(dev, client->irq, NULL,
					lt6911_gpio_irq_thread_handler,
					IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING |
					IRQF_ONESHOT, "lt6911", lt6911);
	if (ret) {
		dev_err(&client->dev, "failed to request irq\n");
		return ret;
	}
	dev_info(dev, "sensor_%d request_irq success\n", index_id);

	lt6911_gpio_irq_thread_handler(client->irq, lt6911);

	lt6911->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(lt6911->reset_gpio)) {
		dev_err(dev, "cannot get reset gpio property\n");
		return PTR_ERR(lt6911->reset_gpio);
	}
	dev_info(dev, "sensor_%d request_reset success\n", index_id);

	return 0;

error_media_entity:
	media_entity_cleanup(&lt6911->sd.entity);

error_handler_free:
	lt6911_free_controls(lt6911);
	dev_err(&client->dev, "%s failed:%d\n", __func__, ret);

	return ret;
}

static int lt6911_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct lt6911 *lt6911 = to_lt6911(sd);

	lt6911_probe_index = 0;

	v4l2_async_unregister_subdev(sd);
	media_entity_cleanup(&sd->entity);
	lt6911_free_controls(lt6911);

	pm_runtime_disable(&client->dev);

	return 0;
}

static const struct of_device_id lt6911_of_match[] = {
	{ .compatible = "cvitek,sensor0" },
	{ .compatible = "cvitek,sensor1" },
	{ .compatible = "cvitek,sensor2" },
	{ .compatible = "cvitek,sensor3" },
	{ .compatible = "cvitek,sensor4" },
	{ .compatible = "cvitek,sensor5" },
	{},
};
MODULE_DEVICE_TABLE(of, lt6911_of_match);

static const struct dev_pm_ops lt6911_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(suspend, resume)
};

static struct i2c_driver lt6911_i2c_driver = {
	.driver = {
		.name = "lt6911",
		.pm = &lt6911_pm_ops,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(lt6911_of_match),
	},
	.probe    = lt6911_probe,
	.remove   = lt6911_remove,
};

static int __init sensor_mod_init(void)
{
	pr_info("== lt6911 mod add ==\n");

	return i2c_add_driver(&lt6911_i2c_driver);
}

static void __exit sensor_mod_exit(void)
{
	i2c_del_driver(&lt6911_i2c_driver);
}

module_init(sensor_mod_init);
module_exit(sensor_mod_exit);

MODULE_DESCRIPTION("lt6911 sensor driver");
MODULE_LICENSE("GPL v2");

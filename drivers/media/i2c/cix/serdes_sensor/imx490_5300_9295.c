// SPDX-License-Identifier: GPL-2.0

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <media/v4l2-fwnode.h>
#include "max96724.h"
#include "tegracam_core.h"

#include "imx490_5300_9295_mode_tbls.h"

#define MAX9295_ALTER_ADDR_BASE	(0x10)

static const struct of_device_id z_imx490_5300_9295_of_match[] = {
	{ .compatible = "cix,imx490_5300_9295"},
	{ },
};
MODULE_DEVICE_TABLE(of, z_imx490_5300_9295_of_match);

static const u32 ctrl_cid_list[] = {
	TEGRA_CAMERA_CID_SENSOR_MODE_ID,
};

#define IMX490_5300_9295_LINK_FREQ_750M (750000000)
#define IMX490_5300_9295_LINK_FREQ_375M (375000000)

static const s64 link_freq_menu_items[] = {
	IMX490_5300_9295_LINK_FREQ_375M,
};

struct z_imx490_5300_9295 {
	struct i2c_client	*i2c_client;
	const struct i2c_device_id *id;
	struct v4l2_subdev	sd;
	struct media_pad pad[1];
	struct v4l2_fwnode_endpoint ep;
	struct device		*dser_dev;
	struct gmsl_link_ctx    g_ctx;
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl *lane_rate;
	u32 def_addr;
	u32 act_addr;
	u32 des_link;
};

static const struct regmap_config sensor_regmap_config = {
	.reg_bits = 16,
	.val_bits = 8,
	.cache_type = REGCACHE_RBTREE,
	.use_single_read = true,
	.use_single_write = true,
};

static struct mutex serdes_lock__;
static int test_mode;
module_param(test_mode, int, 0644);

int z_imx490_5300_9295_read_reg(struct camera_common_data *s_data,
				u16 addr, u8 *val)
{
	int err = 0;
	u32 reg_val = 0;
	struct device *dev = s_data->dev;

	err = regmap_read(s_data->regmap, addr, &reg_val);
	/* delay before next i2c command as required for SERDES link */
	usleep_range(5000, 5100);
	if (!err) {
		*val = reg_val & 0xFF;
		dev_info(dev, "z_imx490_5300_9295:r addr(0x%04x)<-0x%02x\n", addr, *val);
	}

	if (err)
		dev_err(dev, "z_imx490_5300_9295: i2c read failed, addr=0x%x\n", addr);

	return err;
}

int z_imx490_5300_9295_write_reg(struct camera_common_data *s_data,
				u16 addr, u8 val)
{
	int err;
	struct device *dev = s_data->dev;

	err = regmap_write(s_data->regmap, addr, val);
	if (err)
		dev_err(dev, "%s:z_imx490_5300_9295:i2c write failed, 0x%x = %x\n",
			__func__, addr, val);

	/* delay before next i2c command as required for SERDES link */
	usleep_range(5000, 5100);
	if (!err)
		dev_info(dev, "z_imx490_5300_9295:w addr(0x%04x)->0x%02x\n", addr, val);

	return err;
}

static int iic_write(struct i2c_client *client, u8 slaveaddr, u16 regaddr, u8 data)
{
	struct i2c_msg msg[2];
	u8 buf[3];
	int ret;

	msg[0].addr = slaveaddr;
	msg[0].flags = 0;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	buf[0] = regaddr>>8;
	buf[1] = regaddr&0xff;
	buf[2] = data;
	ret = i2c_transfer(client->adapter, msg, 1);

	usleep_range(5000, 5100);

	if (ret == 1) { //send ok
		dev_info(&client->dev, "z_imx490_5300_9295:w slave=0x%02x addr(0x%04x)->0x%02x\n", slaveaddr, regaddr, data);
		return 0;
	}

	return -1;
}

static int iic_read(struct i2c_client *client, u8 slaveaddr, u16 regaddr, u8 *data)
{
	struct i2c_msg msg[2];
	u8 buf[3];
	int ret;

	msg[0].addr = slaveaddr;
	msg[0].flags = 0;
	msg[0].buf = buf;
	msg[0].len = 2;

	buf[0] = regaddr>>8;
	buf[1] = regaddr&0xff;

	msg[1].addr = slaveaddr;
	msg[1].buf = buf;
	msg[1].len = 1;
	msg[1].flags = I2C_M_RD;

	ret = i2c_transfer(client->adapter, msg, 2);

	usleep_range(5000, 5100);
	if (ret == 2) {
		*data = buf[0];
		dev_info(&client->dev, "z_imx490_5300_9295:r slave=0x%02x addr(0x%04x)<-0x%02x\n", slaveaddr, regaddr, *data);
		return 0;
	}

	return -1;
}

static int z_imx490_5300_9295_start_streaming(struct z_imx490_5300_9295 *sensor)
{
	struct z_imx490_5300_9295 *priv = sensor;
	struct device *dev = &priv->i2c_client->dev;
	int err;

	msleep(100);

	mutex_lock(&serdes_lock__);

	dev_info(dev, "z_imx490_5300_9295 start streaming - enter\n");

	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x0308, 0x64); //enable CSI-B
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x0311, 0x40); //start z from CSI-B
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x0318, 0x40 | priv->g_ctx.streams[0]);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x031a, 0x40 | priv->g_ctx.streams[0]);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x0002, 0x43); //enable z

	err = z_max96724_start_streaming(priv->dser_dev, &priv->g_ctx);
	if (err)
		goto exit;

#define MAX9295_GPIO_FRAME_TRIGGER 0x02D3

	//frame trigger
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER+1, 0xA0);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER+3+1, 0xA0);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER, 0x00);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER+3, 0x00);
	mdelay(50);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER, 0x10);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER+3, 0x10);
	mdelay(50);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER, 0x00);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER+3, 0x00);
	mdelay(50);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER, 0x10);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER+3, 0x10);
	mdelay(50);

	//receive remote
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER, 0x04);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER+1, 0x6A);
	iic_write(priv->i2c_client, MAX9295_ALTER_ADDR_BASE+priv->des_link, MAX9295_GPIO_FRAME_TRIGGER+2, 0x0A);

	dev_info(dev, "z_imx490_5300_9295 start streaming - exit\n");

	mutex_unlock(&serdes_lock__);

	return 0;

exit:
	mutex_unlock(&serdes_lock__);

	if (err)
		dev_err(dev, "%s: error setting stream\n", __func__);

	dev_info(dev, "z_imx490_5300_9295 start streaming - exit\n");

	return err;
}

static int z_imx490_5300_9295_stop_streaming(struct z_imx490_5300_9295 *sensor)
{
	struct z_imx490_5300_9295 *priv  = sensor;
	/* disable serdes streaming */
	z_max96724_stop_streaming(priv->dser_dev, &priv->g_ctx);

	return 0;
}

static int z_imx490_5300_9295_open(struct v4l2_subdev *sd, struct v4l2_subdev_fh *fh)
{
	pr_info("imx490 open enter");

	return 0;
}

static const struct v4l2_subdev_internal_ops z_imx490_5300_9295_subdev_internal_ops = {
	.open = z_imx490_5300_9295_open,
};

static int z_imx490_5300_9295_board_setup(struct z_imx490_5300_9295 *priv)
{
	struct device *dev = &priv->i2c_client->dev;
	struct device_node *node = dev->of_node;
	struct device_node *dser_node;
	struct i2c_client *dser_i2c = NULL;
	struct device_node *gmsl;
	int value = 0xFFFF;
	const char *str_value1[2], *str_value;
	int  i;
	int err;

	err = of_property_read_u32(node, "reg", &priv->act_addr);
	if (err < 0) {
		dev_err(dev, "reg not found\n");
		goto error;
	}

	err = of_property_read_u32(node, "def-addr",
					&priv->def_addr);
	if (err < 0) {
		dev_err(dev, "def-addr not found\n");
		goto error;
	}

	priv->g_ctx.ser_dev = dev;

	dser_node = of_parse_phandle(node, "nvidia,gmsl-dser-device", 0);
	if (dser_node == NULL) {
		dev_err(dev, "missing %s handle\n", "nvidia,gmsl-dser-device");
		err = -EINVAL;
		goto error;
	}

	dser_i2c = of_find_i2c_device_by_node(dser_node);
	of_node_put(dser_node);

	if (dser_i2c == NULL) {
		dev_err(dev, "missing deserializer dev handle\n");
		err = -EINVAL;
		goto error;
	}
	if (dser_i2c->dev.driver == NULL) {
		dev_err(dev, "missing deserializer driver\n");
		err = -EINVAL;
		goto error;
	}

	priv->dser_dev = &dser_i2c->dev;
	priv->g_ctx.des_dev =  &dser_i2c->dev;

	err = of_property_read_string(node, "des-link", &str_value);
	if (err < 0) {
		dev_err(dev, "des-link property is not found\n");
		return -EINVAL;
	}

	dev_info(dev, "link is %s\n", str_value);

	priv->des_link = str_value[0]-'A';
	priv->g_ctx.des_link = priv->des_link;

	/* populate g_ctx from DT */
	gmsl = of_get_child_by_name(node, "gmsl-link");
	if (gmsl == NULL) {
		dev_err(dev, "missing gmsl-link device node\n");
		err = -EINVAL;
		goto error;
	}

	err = of_property_read_u32(gmsl, "num-ser-lanes", &value);
	if (err < 0) {
		dev_err(dev, "No num-lanes info\n");
		goto error;
	}
	priv->g_ctx.num_ser_csi_lanes = value;

	priv->g_ctx.num_streams =
			of_property_count_strings(gmsl, "streams");
	if (priv->g_ctx.num_streams <= 0) {
		dev_err(dev, "No streams found\n");
		err = -EINVAL;
		goto error;
	}

	for (i = 0; i < priv->g_ctx.num_streams; i++) {

		of_property_read_string_index(gmsl, "streams", i,
						&str_value1[i]);
		if (!str_value1[i]) {
			dev_err(dev, ":invalid stream info\n");
			goto error;
		}

		if (!strcmp(str_value1[i], "raw12")) {
			priv->g_ctx.streams[i] =
							GMSL_CSI_DT_RAW_12;
		} else if (!strcmp(str_value1[i], "embed")) {
			priv->g_ctx.streams[i] =
							GMSL_CSI_DT_EMBED;
		} else if (!strcmp(str_value1[i], "ued-u1")) {
			priv->g_ctx.streams[i] =
							GMSL_CSI_DT_UED_U1;
		} else if (!strcmp(str_value1[i], "yuv16")) {
			priv->g_ctx.streams[i] =
							GMSL_CSI_DT_YUV16;
		}  else {
			dev_err(dev, "invalid stream data type\n");
			goto error;
		}
	}

	priv->g_ctx.sensor_dev = dev;

	return 0;

error:
	dev_err(dev, "board setup failed\n");

	if (err == 0)
		err = -EINVAL;

	return err;
}

static int imx490_5300_9295_s_power(struct v4l2_subdev *sd, int on)
{
	return 0;
}

static int imx490_5300_9295_g_input_status(struct v4l2_subdev *sd, u32 *status)
{
	*status = 0;
	return 0;
}

static int imx490_5300_9295_s_stream(struct v4l2_subdev *sd, int on)
{
	pr_info("imx490 stream %s\n", on ? "on" : "off");

	struct z_imx490_5300_9295 *priv = container_of(sd, struct z_imx490_5300_9295, sd);

	if (on)
		z_imx490_5300_9295_start_streaming(priv);
	else
		z_imx490_5300_9295_stop_streaming(priv);

	return 0;
}

static int imx490_5300_9295_g_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_frame_interval *fi)
{
	struct v4l2_fract max_fps = {
		.numerator = 10000,
		.denominator = 300000,
	};

	fi->interval = max_fps;

	return 0;
}

static int imx490_5300_9295_s_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_frame_interval *fi)
{
	return 0;
}

static int imx490_5300_9295_enum_mbus_code(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
		struct v4l2_subdev_mbus_code_enum *code)
{
	code->code = MEDIA_BUS_FMT_VYUY8_2X8;
	return 0;
}

static int imx490_5300_9295_enum_frame_sizes(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
		struct v4l2_subdev_frame_size_enum *fse)
{
	if (fse->index > 0)
		return -1;

	fse->min_width = 2880;
	fse->max_width = 2880;
	fse->min_height = 1860;
	fse->max_height = 1860;

	return 0;
}

static int imx490_5300_9295_enum_frame_interval(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
		struct v4l2_subdev_frame_interval_enum *fie)
{
	struct v4l2_fract max_fps = {
		.numerator = 10000,
		.denominator = 300000,
	};

	if (fie->index > 0)
		return -1;

	fie->code = MEDIA_BUS_FMT_VYUY8_2X8;
	fie->width = 2880;
	fie->height = 1860;
	fie->interval = max_fps;

	return 0;
}

static int imx490_5300_9295_get_fmt(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
		struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *fmt = &format->format;

	fmt->width = 2880;
	fmt->height = 1860;
	format->format.field = V4L2_FIELD_NONE;

	return 0;
}

static const struct v4l2_subdev_core_ops imx490_5300_9295_core_ops = {
	.s_power = imx490_5300_9295_s_power,
	//.subscribe_event = imx490_5300_9295_subscribe_event,
	//.unsubscribe_event = v4l2_event_subdev_unsubscribe,
};

static const struct v4l2_subdev_video_ops imx490_5300_9295_video_ops = {
	.g_input_status = imx490_5300_9295_g_input_status,
	.s_stream = imx490_5300_9295_s_stream,
	.g_frame_interval = imx490_5300_9295_g_frame_interval,
	.s_frame_interval = imx490_5300_9295_s_frame_interval,
};

static const struct v4l2_subdev_pad_ops imx490_5300_9295_pad_ops = {
	.enum_mbus_code = imx490_5300_9295_enum_mbus_code,
	.enum_frame_size = imx490_5300_9295_enum_frame_sizes,
	.enum_frame_interval = imx490_5300_9295_enum_frame_interval,
	.get_fmt = imx490_5300_9295_get_fmt,
};

static const struct v4l2_subdev_ops imx490_5300_9295_ops = {
	.core = &imx490_5300_9295_core_ops,
	.video = &imx490_5300_9295_video_ops,
	.pad = &imx490_5300_9295_pad_ops,
};

static int imx490_5300_9295_init_v4l2_ctrls(struct z_imx490_5300_9295 *priv)
{
	struct v4l2_subdev *sd;
	int ret;

	sd = &priv->sd;
	ret = v4l2_ctrl_handler_init(&priv->ctrl_handler, 1);
	if (ret)
		return ret;

	priv->lane_rate = v4l2_ctrl_new_int_menu(
			&priv->ctrl_handler, NULL, V4L2_CID_LINK_FREQ,
			ARRAY_SIZE(link_freq_menu_items), 0, link_freq_menu_items);

	if (priv->ctrl_handler.error) {
		pr_err("cfg v4l2 ctrls failed!\n");
		return ret;
	}

	sd->ctrl_handler = &priv->ctrl_handler;
	__v4l2_ctrl_s_ctrl(priv->lane_rate,0);

	return 0;
}

static int z_imx490_5300_9295_probe(struct i2c_client *client)
{
	struct device *dev = &client->dev;
	struct z_imx490_5300_9295 *priv;
	struct fwnode_handle *endpoint;
	int err;
	u8 val;

	dev_info(dev, "probing v4l2 sensor @0x%x.\n", client->addr);

	priv = devm_kzalloc(dev, sizeof(struct z_imx490_5300_9295), GFP_KERNEL);
	if (!priv)
		return -1;

	priv->i2c_client = client;

	/* Parse endpoint */
	endpoint = fwnode_graph_get_next_endpoint(dev_fwnode(dev), NULL);
	if (!endpoint) {
		dev_err(dev, "endpoint node not found\n");
		return -EINVAL;
	}

	err = v4l2_fwnode_endpoint_parse(endpoint, &priv->ep);
	fwnode_handle_put(endpoint);
	if (err) {
		dev_err(dev, "Could not parse endpoint\n");
		return err;
	}

	err = z_imx490_5300_9295_board_setup(priv);
	if (err) {
		dev_err(dev, "board setup failed\n");
		return err;
	}

	z_max96724_lock_link(priv->dser_dev);
	if (z_max96724_check_link_status(priv->dser_dev, priv->des_link)) { //link occupied
		z_max96724_unlock_link(priv->dser_dev);
		dev_err(dev, "link_%c is occupied\n", 'A' + priv->des_link);
		return -EINVAL;
	}

	z_max96724_monopolize_link(priv->dser_dev, priv->des_link);

	/*remap need close other reverse control channel*/
	if (priv->des_link == 0)
		z_max96724_reverse_contrl_link_disbale(priv->dser_dev, 0, 1, 1, 1);
	else if (priv->des_link == 1)
		z_max96724_reverse_contrl_link_disbale(priv->dser_dev, 1, 0, 1, 1);
	else if (priv->des_link == 2)
		z_max96724_reverse_contrl_link_disbale(priv->dser_dev, 1, 1, 0, 1);
	else if (priv->des_link == 3)
		z_max96724_reverse_contrl_link_disbale(priv->dser_dev, 1, 1, 1, 0);

	z_max96724_reverse_contrl_set_map_port(priv->dser_dev, priv->des_link);

	iic_write(client, 0x40, 0x0000, (MAX9295_ALTER_ADDR_BASE + priv->des_link)<<1);
	//iic_read(client, 0x40, 0x0000, &val);
	//err = iic_read(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x0000, &val);

	mdelay(100);
	iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x02bf, 0x60);
	iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x02be, 0x10); //power on

	mdelay(500);

	err = iic_read(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x000d, &val);
	if (err || val != 0x91) {
		dev_err(dev, "access z_imx490_5300_9295's 9295 failed\n");
		z_max96724_restore_link(priv->dser_dev);
		z_max96724_unlock_link(priv->dser_dev);
		return -EINVAL;
	}

       /* err = iic_read(client, priv->def_addr, 0x0000, &val);*/
	/*if(err){*/
		/*dev_err(dev, "access z_imx490_5300_9295's isp failed\n");*/
		/*z_max96724_restore_link(priv->dser_dev);*/
		/*z_max96724_unlock_link(priv->dser_dev);*/
		/*return -EINVAL;*/
	/*}*/

	iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x007b, 0x30 + priv->des_link);
	iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x0083, 0x30 + priv->des_link);
	iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x0093, 0x30 + priv->des_link);
	iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x009b, 0x30 + priv->des_link);

	mdelay(100);

	iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x00a3, 0x30 + priv->des_link);
	iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link, 0x00ab, 0x30 + priv->des_link);
	//iic_write(client, MAX9295_ALTER_ADDR_BASE+priv->des_link,0x008b, 0x30+priv->des_link);

	mdelay(100);
	z_max96724_reverse_contrl_link_enable_all_map_port(priv->dser_dev);

	z_max96724_enable_link(priv->dser_dev, priv->des_link);
	z_max96724_restore_link(priv->dser_dev);
	z_max96724_unlock_link(priv->dser_dev);

	dev_info(&client->dev, "Detected z_imx490_5300_9295 sensor\n");
{
	struct v4l2_subdev *sd;
	int ret;

	sd = &priv->sd;
	snprintf(sd->name, sizeof(sd->name), "%s", "imx490_5300_9295");

	v4l2_i2c_subdev_init(sd, client, &imx490_5300_9295_ops);

#if defined(CONFIG_MEDIA_CONTROLLER)

	sd->entity.function = MEDIA_ENT_F_CAM_SENSOR;
	priv->pad[0].flags = MEDIA_PAD_FL_SOURCE;
	ret = media_entity_pads_init(&sd->entity, 1, priv->pad);
	if (ret) {
		dev_err(dev, "pads init failed %d", ret);
		return ret;
	}
#endif
	/* register v4l2_subdev device */
	sd->flags |= V4L2_SUBDEV_FL_HAS_DEVNODE | V4L2_SUBDEV_FL_IS_I2C;
	ret = v4l2_async_register_subdev(sd);
	if (ret)
		dev_err(dev, "imx490_5300_9295 subdev registration failed\n");
}

	imx490_5300_9295_init_v4l2_ctrls(priv);

	return 0;
}

static void z_imx490_5300_9295_remove(struct i2c_client *client)
{

}

static const struct i2c_device_id z_imx490_5300_9295_id[] = {
	{ "z_imx490_5300_9295", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, z_imx490_5300_9295_id);

static struct i2c_driver z_imx490_5300_9295_i2c_driver = {
	.driver = {
		.name = "z_imx490_5300_9295",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(z_imx490_5300_9295_of_match),
	},
	.probe = z_imx490_5300_9295_probe,
	.remove = z_imx490_5300_9295_remove,
	.id_table = z_imx490_5300_9295_id,
};

static int __init z_imx490_5300_9295_init(void)
{
	mutex_init(&serdes_lock__);

	return i2c_add_driver(&z_imx490_5300_9295_i2c_driver);
}

static void __exit z_imx490_5300_9295_exit(void)
{
	mutex_destroy(&serdes_lock__);

	i2c_del_driver(&z_imx490_5300_9295_i2c_driver);
}

late_initcall(z_imx490_5300_9295_init);
module_exit(z_imx490_5300_9295_exit);

MODULE_DESCRIPTION("Media Controller driver for z_imx490_5300_9295");
MODULE_AUTHOR("zuojisi@163.com");
MODULE_LICENSE("GPL v2");

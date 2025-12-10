// SPDX-License-Identifier: GPL-2.0

#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/module.h>
#include "camera_common.h"
#include "max96724.h"

#define max96724_TX11_PIPE_X_EN_ADDR 0x90B
#define max96724_TX45_PIPE_X_DST_CTRL_ADDR 0x92D
#define max96724_PIPE_X_SRC_0_MAP_ADDR 0x90D

#define max96724_PHY_CLK (22)

struct max96724 {
	struct i2c_client *i2c_client;
	struct regmap *regmap;
	struct mutex lock;
	u8 link_lock_status;
	u8 link_remap_a;
	u8 link_remap_b;
	u8 link_remap_c;
	u8 link_remap_d;
	u8 ismaster;
};

#define CHECKNULL(x) do {                \
	if ((x) == NULL) {                                \
		pr_err("%s: %s is NULL\n", __func__, #x); \
		return -EINVAL;                                 \
	}                                                   \
} while (0)

static int vercheck;
static int check_version(struct i2c_client *client);
static int check_device_id_rev(struct i2c_client *client);
// static int check_link(struct i2c_client *client);

static int max96724_write_reg(struct device *dev,
	u16 addr, u8 val)
{
	struct max96724 *priv;
	struct i2c_msg msgs[1];
	struct i2c_client *i2c_client;
	unsigned char buf[3];
	int ret;

	priv = dev_get_drvdata(dev);
	i2c_client = priv->i2c_client;

	buf[0] = (addr >> 8) & 0xFF;
	buf[1] = addr & 0xFF;
	buf[2] = val;

	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 3;
	msgs[0].buf = buf;

	ret = i2c_transfer(i2c_client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs)) {
		dev_err(dev, "%s: write register 0x%x from 0x%x failed\n",
				__func__, addr, i2c_client->addr);
	}

	/*[> delay before next i2c command as required for SERDES link <]*/
	usleep_range(5000, 5100);

	return 0;
}

static int max96724_read_reg(struct device *dev,
	u16 addr, u32 *val)
{
	struct max96724 *priv = dev_get_drvdata(dev);
	struct i2c_client *i2c_client = priv->i2c_client;
	struct i2c_msg msgs[2];
	unsigned char buf[2];
	unsigned char value;
	int ret;

	buf[0] = (addr >> 8) & 0xFF;
	buf[1] = addr & 0xFF;

	msgs[0].addr = i2c_client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 2;
	msgs[0].buf = buf;

	msgs[1].addr = i2c_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = &value;

	ret = i2c_transfer(i2c_client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret != ARRAY_SIZE(msgs)) {
		dev_err(dev, "%s: reading register 0x%x from 0x%x failed\n",
				__func__, addr, i2c_client->addr);
	}

	*val = value;

	/* delay before next i2c command as required for SERDES link */
	usleep_range(5000, 5100);

	return 0;
}

int z_max96724_lock_link(struct device *dev)
{
	struct max96724 *priv = NULL;

	CHECKNULL(dev);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	mutex_lock(&priv->lock);
	dev_info(dev, "mutex lock");
	return 0;
}
EXPORT_SYMBOL(z_max96724_lock_link);

int z_max96724_unlock_link(struct device *dev)
{
	struct max96724 *priv = NULL;

	CHECKNULL(dev);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	dev_info(dev, "mutex unlock");
	mutex_unlock(&priv->lock);
	return 0;
}
EXPORT_SYMBOL(z_max96724_unlock_link);

int z_max96724_monopolize_link(struct device *dev, int link)
{
	struct max96724 *priv = NULL;
	u32 value;

	CHECKNULL(dev);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	dev_info(dev, "%s: monopolize link_%c\n", __func__, 'A' + link);

	max96724_read_reg(dev, 0x0006, &value);
	value = (value & 0xF0) | (1<<link);
	max96724_write_reg(dev, 0x0006, value);

	mdelay(150);

	return 0;
}
EXPORT_SYMBOL(z_max96724_monopolize_link);

int z_max96724_enable_link(struct device *dev, int link)
{
	struct max96724 *priv = NULL;

	CHECKNULL(dev);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	dev_info(dev, "%s: enable link_%c\n", __func__, 'A'+link);
	priv->link_lock_status |= (1<<link);
	return 0;
}
EXPORT_SYMBOL(z_max96724_enable_link);

int z_max96724_restore_link(struct device *dev)
{
	struct max96724 *priv = NULL;
	u32 value;

	CHECKNULL(dev);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	dev_info(dev, "restore links\n");
	max96724_read_reg(dev, 0x0006, &value);
	max96724_write_reg(dev, 0x0006, (value&0xF0) | (priv->link_lock_status&0x0F));
	mdelay(150);

	return 0;
}
EXPORT_SYMBOL(z_max96724_restore_link);

int z_max96724_check_link_status(struct device *dev, int link)
{
	struct max96724 *priv = NULL;

	CHECKNULL(dev);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	if (priv->link_lock_status & (0x1<<link))
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(z_max96724_check_link_status);

int z_max96724_set_link_bandwidth(struct device *dev, int link, int gbps)
{
	struct max96724 *priv = NULL;
	u32 value;
	u16 reg = 0x0010;
	u8 regval = 0x01;

	if (gbps != 3) //6Gbps
		regval = 0x02;

	CHECKNULL(dev);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	dev_info(dev, "set link_%c to %dGbps\n", link + 'A', gbps);
	if (link >= 2) {
		reg++;
		link -= 2;
	}

	max96724_read_reg(dev, reg, &value);
	value = (value & (~(0x3<<(link*4)))) | (regval<<(link*4));
	max96724_write_reg(dev, reg, value & 0xFF);
	max96724_write_reg(dev, 0x0018, (1<<link));
	mdelay(300);

	return 0;
}
EXPORT_SYMBOL(z_max96724_set_link_bandwidth);

int z_max96724_reverse_contrl_link_disbale(struct device *dev, int port_a, int port_b, int port_c, int port_d)
{
	u8 value = 0;
	struct max96724 *priv = NULL;

	priv = dev_get_drvdata(dev);

	if (port_a)
		value = (3 << 0);

	if (port_b)
		value = value | (3 << (port_b*2));

	if (port_c)
		value = value | (3 << (port_c*4));

	if (port_d)
		value = value | (3 << (port_d*6));

	pr_info("diable link value %x", value);

	max96724_write_reg(dev, 0x03, value);

	mdelay(100);

	return 0;
}
EXPORT_SYMBOL(z_max96724_reverse_contrl_link_disbale);

int z_max96724_reverse_contrl_link_enable_all_map_port(struct device *dev)
{
	struct max96724 *priv = dev_get_drvdata(dev);
	u8 value = 0xFF;

	if (priv->link_remap_a)
		value &= 0xFC;

	if (priv->link_remap_b)
		value &= 0xF3;

	if (priv->link_remap_c)
		value &= 0xCF;

	if (priv->link_remap_d)
		value &= 0x3F;

	pr_info("enable link value %x", value);

	max96724_write_reg(dev, 0x03, value);
	mdelay(100);

	return 0;
}
EXPORT_SYMBOL(z_max96724_reverse_contrl_link_enable_all_map_port);

int z_max96724_reverse_contrl_set_map_port(struct device *dev, int port)
{
	struct max96724 *priv = dev_get_drvdata(dev);

	if (port == 0)
		priv->link_remap_a = 1;
	else if (port == 1)
		priv->link_remap_b = 1;
	else if (port == 2)
		priv->link_remap_c = 1;
	else if (port == 3)
		priv->link_remap_d = 1;

	return 0;
}
EXPORT_SYMBOL(z_max96724_reverse_contrl_set_map_port);


static int max96724_setup_pipeline(struct device *dev, struct gmsl_link_ctx *g_ctx)
{
	struct max96724 *priv = NULL;
	int pipe_id = 0;
	u32 i = 0;
	u8 dst_vc = 0, src_vc = 0;

	CHECKNULL(dev);
	CHECKNULL(g_ctx);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	dev_info(dev, "96712: num_streams=%d\n", g_ctx->num_streams);

	for (i = 0; i < g_ctx->num_streams; i++) {

		pipe_id = g_ctx->des_link;

		dev_info(dev, "pipe_%d receive stream_id=%d, datatype=0x%x\n", pipe_id, i, g_ctx->streams[i]);

		if (g_ctx->des_link < 3)
			dst_vc = g_ctx->des_link;
		else
			dst_vc = 0;

		max96724_write_reg(dev, (0x40*pipe_id) + max96724_TX11_PIPE_X_EN_ADDR+0, 0x1f);
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_TX45_PIPE_X_DST_CTRL_ADDR + 0, (g_ctx->des_link < 3) ? 0xAA:0x55);  // 0x55=controller1  0xAA=controller2
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_TX45_PIPE_X_DST_CTRL_ADDR + 1, (g_ctx->des_link < 3) ? 0x02:0x01);
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+0, g_ctx->streams[i] | (src_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+1, g_ctx->streams[i] | (dst_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+2, 0x00 | (src_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+3, 0x00 | (dst_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+4, 0x01 | (src_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+5, 0x01 | (dst_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+6, 0x02 | (src_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+7, 0x02 | (dst_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+8, 0x03 | (src_vc << 6));
		max96724_write_reg(dev, (0x40*pipe_id) + max96724_PIPE_X_SRC_0_MAP_ADDR+9, 0x03 | (dst_vc << 6));
	}

	return 0;
}

int z_max96724_start_streaming(struct device *dev, struct gmsl_link_ctx *g_ctx)
{
	struct max96724 *priv = NULL;
	int err;
	u32 val;

	CHECKNULL(dev);
	CHECKNULL(g_ctx);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	dev_info(dev, "96712 start streaming, link=%c - enter\n", 'A' + g_ctx->des_link);

	mutex_lock(&priv->lock);
	err = max96724_setup_pipeline(dev, g_ctx);
	if (err)
		return err;

	max96724_read_reg(dev, 0xf4, &val);
	max96724_write_reg(dev, 0xf4, (val | (1<<g_ctx->des_link)) & 0xff);

	/* //reset mipi phy*/
	 /*if(s_priv->des_link==2 || s_priv->des_link==3){*/
		 /*max96724_write_reg(dev,0x8a2, 0x30);*/
	 /*}else{*/
		 /*max96724_write_reg(dev,0x8a2, 0xc0);*/
	 /*}*/
	 /*max96724_write_reg(dev,0x8a2, 0xf0);*/

	max96724_write_reg(dev, 0x8c9, 0xF);

	mdelay(150);
	max96724_write_reg(dev, 0x8c9, 0x0);

	mdelay(150);

	// reset link
	max96724_write_reg(dev, 0x018, (1<<g_ctx->des_link));
	mdelay(150);

	//skew
	if (g_ctx->des_link < 3) {
		//max96724_write_reg(dev,0x0943,0x10);
		//max96724_write_reg(dev,0x0943,0x33);
		max96724_write_reg(dev, 0x0943, 0x07);
		max96724_write_reg(dev, 0x0943, 0x01);

	} else {
		//max96724_write_reg(dev,0x0983,0x10);
		//max96724_write_reg(dev,0x0983,0x33);
	}

	mutex_unlock(&priv->lock);
	dev_info(dev, "96712 start streaming - exit\n");

	return 0;
}
EXPORT_SYMBOL(z_max96724_start_streaming);

int z_max96724_stop_streaming(struct device *dev, struct gmsl_link_ctx *g_ctx)
{
	struct max96724 *priv = NULL;
	u32 val;
	u8 tmp;

	CHECKNULL(dev);
	CHECKNULL(g_ctx);
	priv = dev_get_drvdata(dev);
	CHECKNULL(priv);

	dev_info(dev, "96712 stop streaming, link_%c - enter\n", 'A' + g_ctx->des_link);
	mutex_lock(&priv->lock);

	max96724_read_reg(dev, 0xf4, &val);
	tmp = (val & (~(1<<g_ctx->des_link))) & 0xff;
	max96724_write_reg(dev, 0xf4, tmp);

	if (tmp == 0) {
		max96724_write_reg(dev, 0x018, 0x0f);
		dev_info(dev, "all stopped, reset links\n");
		mdelay(30);
	}

	mutex_unlock(&priv->lock);
	dev_info(dev, "96712 stop streaming - exit\n");

	return 0;
}
EXPORT_SYMBOL(z_max96724_stop_streaming);

const struct of_device_id max96724_of_match[] = {
	{ .compatible = "cix,max96724", },
	{ },
};
MODULE_DEVICE_TABLE(of, max96724_of_match);

static int max96724_parse_dt(struct max96724 *priv,
				struct i2c_client *client)
{
	struct device_node *node = client->dev.of_node;
	const struct of_device_id *match;

	if (!node)
		return -EINVAL;

	match = of_match_device(max96724_of_match, &client->dev);
	if (!match) {
		dev_err(&client->dev, "Failed to find matching dt id\n");
		return -EFAULT;
	}

	return 0;
}

static int get_mipifreq(void)
{
	return (max96724_PHY_CLK&0x1f);
}

static int max96724_probe(struct i2c_client *client)
{
	struct max96724 *priv;
	int mipifreq = 0;
	int err = 0;
	u32 val = 0;

	dev_info(&client->dev, "[max96724]: probing GMSL Deserializer @0x%x\n", client->addr);

	priv = devm_kzalloc(&client->dev, sizeof(*priv), GFP_KERNEL);
	priv->i2c_client = client;

	err = max96724_parse_dt(priv, client);
	if (err) {
		dev_err(&client->dev, "unable to parse dt\n");
		return -EFAULT;
	}

	mutex_init(&priv->lock);

	dev_set_drvdata(&client->dev, priv);

	check_device_id_rev(client);

	priv->ismaster = 1;

	if (check_version(client) < 0)
		return -EFAULT;

	max96724_write_reg(&client->dev, 0x0013, 0x40); //reset all
	mdelay(100);
	max96724_write_reg(&client->dev, 0x0013, 0x00);
	mdelay(100);

	max96724_write_reg(&client->dev, 0x0005, 0x88);
	max96724_write_reg(&client->dev, 0x0310, 0x90);

	/*err=max96724_write_reg(&client->dev,0x0005,0xc8);*/

	//turn on LED
	max96724_write_reg(&client->dev, 0x0309, 0x90);

	//disable Links which are not Locked
	max96724_read_reg(&client->dev, 0x0006, &val);
	max96724_write_reg(&client->dev, 0x0006, (val & 0xF0) | (priv->link_lock_status & 0x0F));

	max96724_write_reg(&client->dev, 0x90a, 0xc0); //4 lanes, D-Phy, 2bits VC
	max96724_write_reg(&client->dev, 0x94a, 0xc0); //4 lanes, D-Phy, 2bits VC
	max96724_write_reg(&client->dev, 0x98a, 0xC0); //2 lanes, D-Phy, 2bits VC
	max96724_write_reg(&client->dev, 0x9ca, 0xC0); //2 lanes, D-Phy, 2bits VC

	max96724_write_reg(&client->dev, 0x8a3, 0xe4); //phy lane mapping
	max96724_write_reg(&client->dev, 0x8a4, 0xe4); //phy lane mapping

	max96724_write_reg(&client->dev, 0x8a0, 0x24); // 2x4 mode

	mipifreq = get_mipifreq();
	max96724_write_reg(&client->dev, 0x415, 0x20 | mipifreq);
	max96724_write_reg(&client->dev, 0x418, 0x20 | mipifreq);
	max96724_write_reg(&client->dev, 0x41b, 0x20 | mipifreq);
	max96724_write_reg(&client->dev, 0x41e, 0x20 | mipifreq);


	max96724_write_reg(&client->dev, 0xf0, 0x62); //pipe1 GMSLB, Z-pipe; pipe0 GMSLA, Z-pipe
	max96724_write_reg(&client->dev, 0xf1, 0xea); //pipe3 GMSLD, Z-pipe; pipe2 GMSLC, Z-pipe
	max96724_write_reg(&client->dev, 0xf4, 0x00); //disable pipes 0-4 (default val)

	//mfp0, enable TX with ID=A
	max96724_write_reg(&client->dev, 0x0300, 0x83); //TX on A
	max96724_write_reg(&client->dev, 0x0301, 0xaa); //TX on A
	max96724_write_reg(&client->dev, 0x0337, 0x2a); //TX on B
	max96724_write_reg(&client->dev, 0x036d, 0x2a); //TX on C
	max96724_write_reg(&client->dev, 0x03a4, 0x2a); //TX on D

	max96724_write_reg(&client->dev, 0x0027, 0x00); //disable unconcerned error report
	max96724_write_reg(&client->dev, 0x0029, 0xf0); //disable unconcerned error report

	// max96724_write_reg(&client->dev,0x1250,0xff); //set 128 ECC threshold, reset error cnt
	/* dev communication gets validated when GMSL link setup is done */
	dev_info(&client->dev, "%s:  success\n", __func__);

	return 0;
}

static int check_device_id_rev(struct i2c_client *client)
{
	unsigned int value = 0;

	max96724_read_reg(&client->dev, 0x004C, &value);
	dev_info(&client->dev, "device revision=0x%x\n", (value & 0x0f));

	return 0;
}

static int check_version(struct i2c_client *client)
{
	struct max96724 *priv = NULL;
	const uint16_t ver0addr = 0x0316; //ver0 : mfp7
	const uint16_t ver1addr = 0x0319; //ver1 : mfp8

	uint16_t veraddrs[2] = {ver0addr, ver1addr};
	uint32_t ver = 0, tmp;
	int i = 0, err = 0;

	priv = dev_get_drvdata(&client->dev);

	CHECKNULL(priv);

	if (priv->ismaster) {

		for (i = 0; i < 2; i++) {
			err = max96724_read_reg(&client->dev, veraddrs[i], &tmp);
			tmp = (tmp & 0x8) >> 3;
			ver = ver | (tmp << i);
		}

		dev_info(&client->dev, "max96724 version=%d\n", ver);

		if (ver == 0x01)
			vercheck = 1;
		else {
			vercheck = -1;
			err = -1;
		}
	} else {

		if (vercheck < 0)
			err = -1;
	}

	return err;
}

static void max96724_remove(struct i2c_client *client)
{
	struct max96724 *priv;

	if (client != NULL) {
		priv = dev_get_drvdata(&client->dev);
		mutex_destroy(&priv->lock);
	}
}

static const struct i2c_device_id max96724_id[] = {
	{ "max96724", 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, max96724_id);

static struct i2c_driver max96724_i2c_driver = {
	.driver = {
		.name = "max96724",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(max96724_of_match),
	},
	.probe = max96724_probe,
	.remove = max96724_remove,
	.id_table = max96724_id,
};

static int __init max96724_init(void)
{
	return i2c_add_driver(&max96724_i2c_driver);
}

static void __exit max96724_exit(void)
{
	i2c_del_driver(&max96724_i2c_driver);
}

module_init(max96724_init);
module_exit(max96724_exit);

MODULE_DESCRIPTION("GMSL Deserializer driver z_max96724");
MODULE_AUTHOR("zuojisi zuojisi@163.com");
MODULE_LICENSE("GPL v2");

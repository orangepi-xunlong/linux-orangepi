/*
 * driver/gpio/tda7729/tda7729.c
 *
 * audio dsp tda7729 sample code version 1.0
 *
 *  Create Date : 2019/11/26
 *
 * Create by   : wxh
 *
 *
 */
#include <linux/i2c.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#include <linux/pm.h>
#endif
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/file.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/sunxi-gpio.h>
#include <linux/of_gpio.h>
#include <linux/pm.h>
#define DEVICE_NAME "tda7729"

static struct tda7729_config {
	u32 tda7729_used;
	u32 twi_id;
	u32 address;
	u32 power_gpio;
} tda7729_config_info;

static struct i2c_client *this_client;
int major;

static struct class *tda7729_class;

static const struct i2c_device_id tda7729_id[] = {
{DEVICE_NAME, 0},
{}
};

#ifdef __DEBUG_ON
#define printinfo(fmt, arg...) \
	printk(KERR"%s-<%d>: " fmt, __func__, __LINE__, ##arg)
#else
#define printinfo(...)
#endif

static int aw_i2c_writebyte(u8 addr, u8 para)
{
	int ret;
	u8 buf[3] = {0};
	struct i2c_msg msg[] = {
	{
		.addr = this_client->addr, .flags = 0, .len = 2, .buf = buf,
	},
};

	buf[0] = addr;
	buf[1] = para;
	ret = i2c_transfer(this_client->adapter, msg, 1);
	return ret;
}

static int tda7729_probe(struct i2c_client *client,
const struct i2c_device_id *id)
{
	int err;
	int val;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		return err;
	}
	this_client = client;

	aw_i2c_writebyte(0xA0, 0x42);
	aw_i2c_writebyte(0xA1, 0x02);
	aw_i2c_writebyte(0xA2, 0x52);
	aw_i2c_writebyte(0xA3, 0x80);
	aw_i2c_writebyte(0xA4, 0x00);
	aw_i2c_writebyte(0xA5, 0xFF);
	aw_i2c_writebyte(0xA6, 0x00);
	aw_i2c_writebyte(0xA7, 0x04);
	aw_i2c_writebyte(0xA8, 0x11);
	aw_i2c_writebyte(0xA9, 0x1F);
	aw_i2c_writebyte(0xAA, 0x1F);
	aw_i2c_writebyte(0xAB, 0x1F);
	aw_i2c_writebyte(0xAC, 0x80);
	aw_i2c_writebyte(0xAD, 0x10);
	aw_i2c_writebyte(0xAE, 0x10);
	aw_i2c_writebyte(0xAF, 0x10);
	aw_i2c_writebyte(0xb0, 0x10);
	aw_i2c_writebyte(0xb1, 0x10);
	aw_i2c_writebyte(0xb2, 0x10);

	return 0;
}

static int tda7729_remove(struct i2c_client *client)
{
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const unsigned short normal_i2c[] = {0x88 >> 1, I2C_CLIENT_END};

static int tda7729_detect(struct i2c_client *client,
struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	if ((tda7729_config_info.twi_id == adapter->nr) &&
	(client->addr == tda7729_config_info.address)) {
		int ret = -1;
		char rdbuf[8];
		struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = rdbuf,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = 1,
			.buf = rdbuf,
		},
	};
	memset(rdbuf, 0, sizeof(rdbuf));
	ret = i2c_transfer(client->adapter, msgs, 2);
	if (ret < 0) {
		pr_err("i2c_transfer error\n");
		return -ENODEV;
	}

	strlcpy(info->type, DEVICE_NAME, I2C_NAME_SIZE);
	printinfo("detect tda7729 chip\n");

	return 0;
	} else {
		return -ENODEV;
	}
}

#if 0
static void tda7729_suspend(struct i2c_client *client)
{
	return 0;
}
static void tda7729_resume(struct i2c_client *client)
{
	return 0;
}
#endif
MODULE_DEVICE_TABLE(i2c, tda7729_id);

static struct i2c_driver tda7729_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = tda7729_probe,
	.remove = tda7729_remove,

	#if 0
	.suspend        = tda7729_suspend,
	.resume         = tda7729_resume,
	#endif
	.id_table = tda7729_id,
	.detect = tda7729_detect,
	.driver = {
	.name = DEVICE_NAME, .owner = THIS_MODULE,
	},
	.address_list = normal_i2c,
};

static int script_data_init(void)
{
	struct device_node *np = NULL;
	struct gpio_config config;
	int ret;

	np = of_find_node_by_name(NULL, "tda7729");

	if (!np)
		pr_err("ERROR! get TDA7729 failed\n");

	ret = of_property_read_u32(np, "tda7729_twi_id",
	&tda7729_config_info.twi_id);
	if (ret) {
		pr_err("get tda7729_twi_id is fail, %d\n", ret);
		return -1;
	}

	ret = of_property_read_u32(np, "twi_addr",
	&tda7729_config_info.address);
	if (ret) {
		pr_err("get twi_address is fail, %d\n", ret);
		return -1;
	}

	tda7729_config_info.power_gpio =
	of_get_named_gpio_flags(np, "tda7729_power", 0,
	(enum of_gpio_flags *)&config);

	if (!gpio_is_valid(tda7729_config_info.power_gpio)) {
		pr_err(" power_gpio is invalid\n");
		return -1;
	}
		if (gpio_request(tda7729_config_info.power_gpio, NULL) != 0) {
			pr_err("power_gpio_request is failed\n");
			return -1;
			}
		if (gpio_direction_output(tda7729_config_info.power_gpio, 1)
			!= 0) {
			pr_err("power_gpio set err!");
			return -1;
			}

	return 1;
}

static int dsp_ioread_open(struct inode *inode, struct file *filp)
{
	printinfo("debug: dsp_ioread_open\n");
	return 0;
}
static int dsp_io_release(struct inode *inode, struct file *filp)
{
	printinfo("my_io_release!\n");
	return 0;
}
static long dsp_io_ioctrl(struct file *filp,
unsigned int cmd, unsigned long arg)
{
	printinfo("debug: my_io_ioctrl cmd is %d\n", cmd);
	switch (cmd) {
	case 0:
		gpio_direction_output(tda7729_config_info.power_gpio, 0);
			break;
	case 1:
		gpio_direction_output(tda7729_config_info.power_gpio, 1);
			break;
	}
	return 0;
}

static const struct file_operations dsp_io_fops = {
	.owner  = THIS_MODULE,
	.open  = dsp_ioread_open,
	.unlocked_ioctl = dsp_io_ioctrl,
	.release = dsp_io_release,
};

static int __init tda7729_init(void)
{
	int ret = -1;

	major = register_chrdev(0, "tda7729", &dsp_io_fops);
	tda7729_class = class_create(THIS_MODULE, "tda7729");
	device_create(tda7729_class, NULL, MKDEV(major, 0), NULL, "tda7729");

	if (script_data_init() > 0)
		ret = i2c_add_driver(&tda7729_driver);

	return ret;
}

static void __exit tda7729_exit(void)
{
	printinfo("tda7729_exit\n");
	unregister_chrdev(major, "tda7729");
	device_destroy(tda7729_class, MKDEV(major, 0));
	class_destroy(tda7729_class);
	i2c_del_driver(&tda7729_driver);

	return 0;
}

module_init(tda7729_init);
module_exit(tda7729_exit);

MODULE_AUTHOR("wxh");
MODULE_DESCRIPTION("external asp driver");
MODULE_LICENSE("GPL");

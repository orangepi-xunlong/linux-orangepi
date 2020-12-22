/*
 * acx00-core.c  --  Device access for allwinnertech acx00
 *
 * Author: 
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mfd/core.h>
#include <linux/pm_runtime.h>
#include <linux/regmap.h>
#include <linux/mfd/acx00-mfd.h>
#include <linux/arisc/arisc.h>
#include <linux/pwm.h>
#include <mach/sys_config.h>
#include <linux/regulator/consumer.h>

//#define AC200_RSB_BUS
#define SUNXI_CHIP_NAME	"ACX00-CHIP"
static unsigned int twi_id = 3;
atomic_t acx00_en;

struct regmap_config acx00_base_regmap_config = {
	.reg_bits = 8,
	.val_bits = 16,
};
static struct regulator *vcc_ave = NULL;
#ifdef AC200_RSB_BUS
/**
 * acx00_rsb_reg_read: Read a single ACX00X00 register.
 *
 * @reg: Register to read.
 */
int acx00_rsb_reg_read(unsigned short reg)
{
	int	ret;
	arisc_rsb_block_cfg_t rsb_data;
	unsigned char addr;
	unsigned int data;

	addr = (unsigned char)reg;
	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_HWORD;
	rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	rsb_data.devaddr = RSB_RTSADDR_ACX00X00;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data;

	/* read axp registers */
	ret = arisc_rsb_read_block_data(&rsb_data);
	if (ret != 0) {
		pr_err("failed reads to 0x%02x\n", reg);
		return ret;
	}
	return data;
}
EXPORT_SYMBOL_GPL(acx00_rsb_reg_read);

/**
 * acx00_rsb_reg_write: Write a single ACX00 register.
 *
 * @reg: Register to write to.
 * @val: Value to write.
 */
int acx00_rsb_reg_write(unsigned short reg, unsigned short value)
{
	int	ret;
	arisc_rsb_block_cfg_t rsb_data;
	unsigned char addr;
	unsigned int data;

	addr = (unsigned char)reg;
	data = value;
	rsb_data.len = 1;
	rsb_data.datatype = RSB_DATA_TYPE_HWORD;
	rsb_data.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	rsb_data.devaddr = RSB_RTSADDR_ACX00X00;
	rsb_data.regaddr = &addr;
	rsb_data.data = &data;

	/* read axp registers */
	ret = arisc_rsb_write_block_data(&rsb_data);
	if (ret != 0) {
		pr_err("failed reads to 0x%02x\n", reg);
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(acx00_rsb_reg_write);
#endif

/**
 * acx00_reg_read: Read a single ACX00 register.
 *
 * @acx00: Device to read from.
 * @reg: Register to read.
 */
int acx00_reg_read(struct acx00 *acx00, unsigned short reg)
{
#ifdef AC200_RSB_BUS
 	return acx00_rsb_reg_read(reg);
#else
	unsigned int val;
	int ret;
	mutex_lock(&acx00->lock);
	regmap_write(acx00->regmap, 0xfe, reg>>8);
	ret = regmap_read(acx00->regmap, reg&0xff, &val);
	mutex_unlock(&acx00->lock);
	if (ret < 0)
		return ret;
	else
		return val;
#endif
}
EXPORT_SYMBOL_GPL(acx00_reg_read);

/**
 * acx00_reg_write: Write a single ACX00 register.
 *
 * @acx00: Device to write to.
 * @reg: Register to write to.
 * @val: Value to write.
 */
int acx00_reg_write(struct acx00 *acx00, unsigned short reg,
		     unsigned short val)
{
	int ret;
	#ifdef AC200_RSB_BUS
	return acx00_rsb_reg_write(reg, val);
	#else
	mutex_lock(&acx00->lock);
	regmap_write(acx00->regmap, 0xfe, reg>>8);
	ret = regmap_write(acx00->regmap, reg&0xff, val);
	mutex_unlock(&acx00->lock);	

	return ret;
	#endif
}
EXPORT_SYMBOL_GPL(acx00_reg_write);

int acx00_enable(void)
{
	return atomic_read(&acx00_en);
}
EXPORT_SYMBOL_GPL(acx00_enable);

static void acx00_init_work(struct work_struct *work)
{
	struct acx00 *acx00 = container_of(work, struct acx00, init_work);

	atomic_set(&acx00_en, 0);
	msleep(120);
	acx00_reg_write(acx00, 0x0002,0x1);
	atomic_set(&acx00_en, 1);
}

static struct mfd_cell acx00_devs[] = {
	{
		.name = "acx00-codec",
	},
	{
		.name = "rtc0_ac200",
	},
	{
		.name = "tv",
	},
	{
		.name = "acx-ephy",
	},
};

/*
 * Instantiate the generic non-control parts of the device.
 */
static __devinit int acx00_device_init(struct acx00 *acx00, int irq)
{
	int ret;

	dev_set_drvdata(acx00->dev, acx00);

	ret = mfd_add_devices(acx00->dev, -1,
			      acx00_devs, ARRAY_SIZE(acx00_devs),
			      NULL, 0);
	if (ret != 0) {
		dev_err(acx00->dev, "Failed to add children: %d\n", ret);
		goto err;
	}
	return 0;

err:
	mfd_remove_devices(acx00->dev);
	return ret;
}

static __devexit void acx00_device_exit(struct acx00 *acx00)
{
	mfd_remove_devices(acx00->dev);
}
/**************************read reg interface**************************/
static ssize_t acx00_dump_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	static int val = 0;
	int reg,num,i=0;
	int value_r[128];
	struct acx00 *acx00 = dev_get_drvdata(dev);
	val = simple_strtol(buf, NULL, 16);
	reg =(val>>8);
	num=val&0xff;
	printk("\n");
	printk("read:start add:0x%x,count:0x%x\n",reg,num);
	do{
		value_r[i] = acx00_reg_read(acx00, reg);
		printk("0x%x: 0x%04x ",reg,value_r[i]);
		reg+=1;
		i++;
		if(i == num)
			printk("\n");
		if(i%4==0)
			printk("\n");
	}while(i<num);
	return count;
}

static ssize_t acx00_dump_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk("echo reg|count > dump\n");
	printk("eg read star addres=0x0006,count 0x10:echo 0x610 > dump\n");
	printk("eg read star addres=0x2000,count 0x10:echo 0x200010 > dump\n");
    return 0;
}

/**************************write reg interface**************************/
static ssize_t acx00_write_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	static int val = 0;
	int reg;
	int value_w;
	struct acx00 *acx00 = dev_get_drvdata(dev);
	val = simple_strtol(buf, NULL, 16);

	reg = (val >> 16);
	value_w =  val & 0xFFFF;
	acx00_reg_write(acx00, reg, value_w);
	printk("write 0x%x to reg:0x%x\n",value_w,reg);
	return count;
}

static ssize_t acx00_write_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk("echo reg|val > write\n");
	printk("eg write value:0x13fe to address:0x0004 :echo 0x413fe > write\n");
	printk("eg write value:0x6 to address:0x2000 :echo 0x20000006 > write\n");
    return 0;
}

static DEVICE_ATTR(dump, 0644, acx00_dump_show, acx00_dump_store);
static DEVICE_ATTR(write, 0644, acx00_write_show, acx00_write_store);
static struct attribute *audio_debug_attrs[] = {
	&dev_attr_dump.attr,
	&dev_attr_write.attr,
	NULL,
};
static struct attribute_group audio_debug_attr_group = {
	.name   = "acx00_debug",
	.attrs  = audio_debug_attrs,
};

static __devinit int acx00_i2c_probe(struct i2c_client *i2c,
				      const struct i2c_device_id *id)
{
	struct acx00 *acx00;
#ifndef AC200_RSB_BUS
	int ret = 0;
#endif

	acx00 = devm_kzalloc(&i2c->dev, sizeof(struct acx00), GFP_KERNEL);
	if (acx00 == NULL)
		return -ENOMEM;

	i2c_set_clientdata(i2c, acx00);
	acx00->dev = &i2c->dev;
	acx00->irq = i2c->irq;
	mutex_init(&acx00->lock);
#ifdef AC200_RSB_BUS
	if (arisc_rsb_set_rtsaddr(RSB_DEVICE_SADDR7, RSB_RTSADDR_ACX00)) {
		pr_err("AUDIO config codec failed\n");
	}
#else
	acx00->regmap = devm_regmap_init_i2c(i2c, &acx00_base_regmap_config);
	if (IS_ERR(acx00->regmap)) {
		ret = PTR_ERR(acx00->regmap);
		dev_err(acx00->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}
	ret = sysfs_create_group(&i2c->dev.kobj, &audio_debug_attr_group);
	if (ret) {
		pr_err("failed to create attr group\n");
	}
	INIT_WORK(&acx00->init_work, acx00_init_work);
	acx00->pwm_ac200 = pwm_request(0, NULL);
	pwm_config(acx00->pwm_ac200, 20, 41);
	pwm_enable(acx00->pwm_ac200);
	atomic_set(&acx00_en, 0);
	schedule_work(&acx00->init_work);
#endif

	return acx00_device_init(acx00, i2c->irq);
}

static __devexit int acx00_i2c_remove(struct i2c_client *i2c)
{
	struct acx00 *acx00 = i2c_get_clientdata(i2c);

	sysfs_remove_group(&i2c->dev.kobj, &audio_debug_attr_group);
	acx00_device_exit(acx00);

	pwm_disable(acx00->pwm_ac200);
	pwm_free(acx00->pwm_ac200);
	return 0;
}
static int acx00_i2c_suspend(struct i2c_client *client,pm_message_t state)
{
	if(vcc_ave != NULL) {
		regulator_disable(vcc_ave);
		regulator_put(vcc_ave);
		vcc_ave = NULL;
	}
	atomic_set(&acx00_en, 0);
	return 0;
}

static int acx00_i2c_resume(struct i2c_client *client)
{
	struct acx00 *acx00 = i2c_get_clientdata(client);

	vcc_ave = regulator_get(NULL, "vcc-ave-33");
	if (IS_ERR_OR_NULL(vcc_ave)) {
		pr_err("get audio vcc-ave-33 failed\n");
	} else
		regulator_enable(vcc_ave);
	schedule_work(&acx00->init_work);
	return 0;
}

static int acx00_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;

	pr_info("%s, l:%d, twi_id:%d, adapter->nr:%d\n", __func__, __LINE__, twi_id, adapter->nr);
	if (twi_id == adapter->nr) {
		strlcpy(info->type, SUNXI_CHIP_NAME, I2C_NAME_SIZE);
		return 0;
	} else {
		return -ENODEV;
	}
}

static const unsigned short normal_i2c[] = {0x10, I2C_CLIENT_END};

static const struct i2c_device_id acx00_id[] = {
	{"ACX00-CHIP", 2},
	{}
};

static struct i2c_driver acx00_i2c_driver = {
	.class 		= I2C_CLASS_HWMON,
	.id_table 	= acx00_id,
	.probe 		= acx00_i2c_probe,
	.remove 	= __devexit_p(acx00_i2c_remove),
	.driver 	= {
		.owner 	= THIS_MODULE,
		.name 	= "ACX00-CHIP",
	},
	.address_list = normal_i2c,
	.suspend  = acx00_i2c_suspend,
	.resume = acx00_i2c_resume,
};

static int ac200_used = 0;
static int __init acx00_i2c_init(void)
{
	int ret = 0;
	script_item_value_type_e  type;
	script_item_u val;

	type = script_get_item("acx0", "ac200_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("[acx0] ac200_used type err!\n");
	}
	ac200_used = val.val;

	if (ac200_used) {
		type = script_get_item("acx0", "twi_ac200_used", &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			pr_err("[acx0] twi_ac200_used type err!\n");
		}else
			twi_id = val.val;

		acx00_i2c_driver.detect = acx00_detect;
		ret = i2c_add_driver(&acx00_i2c_driver);
		if (ret != 0)
			pr_err("Failed to register acx00 I2C driver: %d\n", ret);

		vcc_ave = regulator_get(NULL, "vcc-ave-33");
		if (IS_ERR_OR_NULL(vcc_ave)) {
			pr_err("get audio vcc-ave-33 failed\n");
		} else{
			regulator_enable(vcc_ave);

		}
	}

	return ret;
}
subsys_initcall_sync(acx00_i2c_init);

static void __exit acx00_i2c_exit(void)
{
	i2c_del_driver(&acx00_i2c_driver);
}
module_exit(acx00_i2c_exit);

MODULE_DESCRIPTION("Core support for the ACX00X00 audio CODEC");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("huangxin<huangxin@allwinnertech.com>");

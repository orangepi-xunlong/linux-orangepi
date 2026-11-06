// SPDX-License-Identifier: GPL-2.0-only
/*
 * max8907.c - mfd driver for MAX8907
 *
 * Copyright (C) 2010 Gyungoh Yoo <jack.yoo@maxim-ic.com>
 * Copyright (C) 2010-2012, NVIDIA CORPORATION. All rights reserved.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/reboot.h>

struct cs32l010 {
	struct device			*dev;
	struct i2c_client		*i2c_gen;
	int 				shutdown_gpio;
	struct delayed_work		watchdog_work;
	struct delayed_work		powerkey_work;
};

static struct cs32l010 *cs32l010_pm_off;

static void cs32l010_power_off(void)
{
	int ret;
	while(1)
	{
		printk("%s\n",__func__);
		i2c_smbus_write_byte_data(cs32l010_pm_off->i2c_gen, 0xAA,0xAA);
		gpio_set_value(cs32l010_pm_off->shutdown_gpio, 1);
		mdelay(500);
		ret=i2c_smbus_read_byte_data(cs32l010_pm_off->i2c_gen, 0x04);
		if (ret==3) {
			printk("MCU shutdowning the cpu power!\n");
			return;
		}
	}
}

static int reboot_notifier_callback(struct notifier_block *nb, unsigned long action, void *data) {
    switch (action) {
        case SYS_RESTART:
            i2c_smbus_write_byte_data(cs32l010_pm_off->i2c_gen, 0xAA,0xDD);
            printk("System is rebooting.MCU disable WD\n");
            break;
        case SYS_POWER_OFF:
            printk("System is powering off.\n");
            break;
        case SYS_HALT:
            printk("System is halting.\n");
            break;
        default:
            printk("Unknown system shutdown action: %lu\n", action);
            break;
    }
    return NOTIFY_DONE;
}

static struct notifier_block reboot_notifier = {
    .notifier_call = reboot_notifier_callback,
};

static void cpu_feedwdg_cs32l010_work(struct work_struct *work)
{
	i2c_smbus_write_byte_data(cs32l010_pm_off->i2c_gen, 0xAA,0xEE);
	schedule_delayed_work(&cs32l010_pm_off->watchdog_work, msecs_to_jiffies(5000));
}

static int cs32l010_i2c_probe(struct i2c_client *client,
				       const struct i2c_device_id *id)
{
	struct cs32l010 *cs32l010;
	int i,ret;

	printk("%s start!\n",__func__);

	cs32l010 = devm_kzalloc(&client->dev, sizeof(struct cs32l010), GFP_KERNEL);
	if (!cs32l010) {
		ret = -ENOMEM;
		goto err_alloc_drvdata;
	}

	cs32l010->dev = &client->dev;
	dev_set_drvdata(cs32l010->dev, cs32l010);

	cs32l010->i2c_gen = client;
	i2c_set_clientdata(client, cs32l010);

	cs32l010->shutdown_gpio = of_get_named_gpio(client->dev.of_node, "shutdown-gpio", 0);
	if (cs32l010->shutdown_gpio < 0) {
		printk("could not get pcie reset gpio\n");
	}

	ret = gpio_request_one(cs32l010->shutdown_gpio, GPIOF_OUT_INIT_LOW, "a2-shutdown-gpio");
	if (ret) {
		printk("could not request gpio %d failed!\n", cs32l010->shutdown_gpio);
	}

	for( i = 0; i < 3; i++ )
	{
		ret=i2c_smbus_read_byte_data(cs32l010->i2c_gen, 0x01);
		if (ret < 0) {
			dev_err(&client->dev, "could not read the chipID, %d\n", ret);
		}
		else
		{
			printk("chid id Low=%xc end!\n",ret);
			break;
		}
		mdelay(1);
	}
	for( i = 0; i < 3; i++ )
	{
		ret=i2c_smbus_read_byte_data(cs32l010->i2c_gen, 0x02);
		if (ret < 0) {
			dev_err(&client->dev, "could not read the chipID, %d\n", ret);
		}
		else
		{
			printk("chid id High=%x end!\n",ret);
			break;
		}
		mdelay(1);
	}
	for( i = 0; i < 3; i++ )
	{
		ret=i2c_smbus_read_byte_data(cs32l010->i2c_gen, 0x03);
		if (ret < 0) {
			dev_err(&client->dev, "could not read the CPU Reboot count, %d\n", ret);
		}
		else
		{
			printk("CPU Reboot count=%d\n", ret);
			break;
		}
		mdelay(1);
	}



	cs32l010_pm_off = cs32l010;
	pm_power_off = cs32l010_power_off;
	INIT_DELAYED_WORK(&cs32l010->watchdog_work,cpu_feedwdg_cs32l010_work);
	schedule_delayed_work(&cs32l010->watchdog_work, msecs_to_jiffies(30000));
	i2c_smbus_write_byte_data(cs32l010->i2c_gen, 0xAA,0xCC);
	
	register_reboot_notifier(&reboot_notifier);
	pr_info("Registering reboot notifier.\n");
	printk("%s end!\n",__func__);
	return 0;

err_alloc_drvdata:
	return ret;
}

static int cs32l010_i2c_remove(struct i2c_client *i2c)
{
	struct cs32l010 *cs32l010 = i2c_get_clientdata(i2c);

	i2c_unregister_device(cs32l010->i2c_gen);
	unregister_reboot_notifier(&reboot_notifier);	
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cs32l010_of_match[] = {
	{ .compatible = "BasisLink,cs32l010" },
	{ },
};
MODULE_DEVICE_TABLE(of, cs32l010_of_match);
#endif

static const struct i2c_device_id cs32l010_i2c_id[] = {
	{"cs32l010", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, cs32l010_i2c_id);

static struct i2c_driver cs32l010_i2c_driver = {
	.driver = {
		.name = "cs32l010",
		.of_match_table = of_match_ptr(cs32l010_of_match),
	},
	.probe = cs32l010_i2c_probe,
	.remove = cs32l010_i2c_remove,
	.id_table = cs32l010_i2c_id,
};

static int __init cs32l010_i2c_init(void)
{
	int ret = -ENODEV;

	ret = i2c_add_driver(&cs32l010_i2c_driver);
	if (ret != 0)
		pr_err("Failed to register I2C driver: %d\n", ret);
	return ret;
}
subsys_initcall(cs32l010_i2c_init);

static void __exit cs32l010_i2c_exit(void)
{
	i2c_del_driver(&cs32l010_i2c_driver);
	pr_info("Unregistering reboot notifier.\n");
}
module_exit(cs32l010_i2c_exit);

MODULE_DESCRIPTION("cs32l010 multi-function core driver");
MODULE_AUTHOR("Oscar <wenhao.chen@basislink.com>");
MODULE_LICENSE("GPL v2");

/*
 * Battery charger driver for allwinnertech AXP81X
 *
 * Copyright (C) 2014 ALLWINNERTECH.
 *  Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include "axp20-mfd.h"
#include "../axp-regu.h"
#include "axp20-regu.h"
#include "../axp-cfg.h"

struct axp20_config_info axp20_config;
static struct axp_dev axp20_dev;
static struct wakeup_source *axp20ws;
int axp20_suspend_flag = AXP20_NOT_SUSPEND;

static struct power_supply_info battery_data ={
	.name ="PTI PL336078",
	.technology = POWER_SUPPLY_TECHNOLOGY_LiFe,
	.voltage_max_design = 4200000,
	.voltage_min_design = 3500000,
	.use_for_apm = 1,
};


static struct axp_supply_init_data axp_sply_init_data = {
	.battery_info = &battery_data,
	.chgcur = 1500000,
	.chgvol = 4200000,
	.chgend = 10,
	.chgen = 1,
	.sample_time = 800,
	.chgpretime = 50,
	.chgcsttime = 720,
};

static struct axp_funcdev_info axp_splydev[]={
	{
		.name = "axp20-supplyer",
		.id = AXP20_ID_SUPPLY,
		.platform_data = &axp_sply_init_data,
	},
};

//static struct axp_funcdev_info axp_gpiodev[]={
//   	{
//		.name = "axp20-gpio",
//		.id = AXP20_ID_GPIO,
//	},
//};

static struct axp_platform_data axp_pdata = {
	.num_sply_devs = ARRAY_SIZE(axp_splydev),
//	.num_gpio_devs = ARRAY_SIZE(axp_gpiodev),
	.sply_devs = axp_splydev,
//	.gpio_devs = axp_gpiodev,
//	.gpio_base = 0,
};

static struct axp_mfd_chip_ops axp20_ops[] = {
	[0] = {
		.init_chip    = axp20_init_chip,
		.enable_irqs  = axp20_enable_irqs,
		.disable_irqs = axp20_disable_irqs,
		.read_irqs    = axp20_read_irqs,
	},
};

#ifdef	CONFIG_AXP_TWI_USED
static const struct i2c_device_id axp_i2c_id_table[] = {
	{ "axp20_board", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, axp_i2c_id_table);

static struct i2c_board_info __initdata axp_mfd_i2c_board_info[] = {
	{
		.type = "axp20_board",
		.addr = 0x34,
	},
};
#ifdef CONFIG_ARCH_SUN8IW8P1

#define SYSCTRL_ADDR	(0xf1c00000)
#define NMI_IRQ_CTRL	(SYSCTRL_ADDR + 0xd0)
#define NMI_IRQ_EN	(SYSCTRL_ADDR + 0xd4)
#define NMI_IRQ_STATUS	(SYSCTRL_ADDR + 0xd8)

#define NMI_IRQ_LOW_LEVEL	(0x0)
#define NMI_IRQ_NE_EDGE		(0x1)
#define NMI_IRQ_HIGH_LEVEL	(0x2)
#define NMI_IRQ_PO_EdGE		(0x3)

#define NMI_IRQ_ENABLE		(0x1)
#define NMI_IRQ_DISABLE		(0x0)
#endif
static irqreturn_t axp_mfd_irq_handler(int irq, void *data)
{
	struct axp_dev *chip = data;
	disable_irq_nosync(irq);
#ifdef CONFIG_ARCH_SUN8IW8P1
	if(axp20_config.pmu_irq_id)
		writel(0x1, (volatile void __iomem *)(NMI_IRQ_STATUS));
#endif
	if(AXP20_NOT_SUSPEND == axp20_suspend_flag)
		(void)schedule_work(&chip->irq_work);
	else if(AXP20_AS_SUSPEND == axp20_suspend_flag){
		__pm_wakeup_event(axp20ws, 0);
		axp20_suspend_flag = AXP20_SUSPEND_WITH_IRQ;
	}

	return IRQ_HANDLED;
}

static int axp_i2c_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	int ret = 0;
	unsigned long flags;

	axp20_dev.client = client;
	axp20_dev.dev = &client->dev;

	i2c_set_clientdata(client, &axp20_dev);

	axp20_dev.ops = &axp20_ops[0];
	axp20_dev.attrs = axp20_mfd_attrs;
	axp20_dev.attrs_number = ARRAY_SIZE(axp20_mfd_attrs);
	axp20_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp20_dev);
	if (ret < 0) {
		printk("axp mfd register failed\n");
		return ret;
	}
	if(axp20_config.pmu_irq_id)
		flags = IRQF_SHARED|IRQF_DISABLED |IRQF_NO_SUSPEND;
	else
		flags = IRQF_SHARED|IRQF_DISABLED |IRQF_NO_SUSPEND|IRQF_TRIGGER_LOW;

	ret = request_irq(client->irq, axp_mfd_irq_handler,
		flags, "axp20", &axp20_dev);
	if (ret) {
		dev_err(&client->dev, "failed to request irq %d\n",
				client->irq);
		goto out_free_chip;
	}
	axp20ws = wakeup_source_register("axp_wakeup_source");
#ifdef CONFIG_ARCH_SUN8IW8P1
	if(axp20_config.pmu_irq_id){
		writel(NMI_IRQ_LOW_LEVEL, (volatile void __iomem *)(NMI_IRQ_CTRL));
		writel(NMI_IRQ_ENABLE, (volatile void __iomem *)(NMI_IRQ_EN));
	}
#endif

	return ret;


out_free_chip:
	i2c_set_clientdata(client, NULL);
	return ret;
}

static int axp_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static struct i2c_driver axp_i2c_driver = {
	.driver	= {
		.name	= "axp20_board",
		.owner	= THIS_MODULE,
	},
	.probe		= axp_i2c_probe,
	.remove		= axp_i2c_remove,
	.id_table	= axp_i2c_id_table,
};
#else
static int  axp20_platform_probe(struct platform_device *pdev)
{
	int ret = 0;

	axp20_dev.dev = &pdev->dev;
	dev_set_drvdata(axp20_dev.dev, &axp20_dev);
	axp20_dev.client = (struct i2c_client *)pdev;

	axp20_dev.ops = &axp20_ops[0];
	axp20_dev.attrs = axp20_mfd_attrs;
	axp20_dev.attrs_number = ARRAY_SIZE(axp20_mfd_attrs);
	axp20_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp20_dev);
	if (ret < 0) {
		printk("axp81x mfd register failed\n");
		return ret;
	}
	return 0;
}

static struct platform_driver axp20_platform_driver = {
	.probe		= axp20_platform_probe,
	.driver		= {
		.name	= "axp20_board",
		.owner	= THIS_MODULE,
	},
};
#endif

static int __init axp20_board_init(void)
{
	int ret = 0;
	struct axp_funcdev_info *axp_regu_info = NULL;

	ret = axp20_fetch_sysconfig_para("pmu1_para", &axp20_config);
	if (ret) {
		printk("%s fetch sysconfig err\n", __func__);
		return -1;
	}

	if (axp20_config.pmu_used) {
		battery_data.voltage_max_design = axp20_config.pmu_init_chgvol;
		battery_data.voltage_min_design = axp20_config.pmu_pwroff_vol;
		battery_data.energy_full_design = axp20_config.pmu_battery_cap;
		axp_sply_init_data.chgcur = axp20_config.pmu_runtime_chgcur;
		axp_sply_init_data.chgvol = axp20_config.pmu_init_chgvol;
		axp_sply_init_data.chgend = axp20_config.pmu_init_chgend_rate;
		axp_sply_init_data.chgen = axp20_config.pmu_init_chg_enabled;
		axp_sply_init_data.sample_time = axp20_config.pmu_init_adc_freq;
		axp_sply_init_data.chgpretime = axp20_config.pmu_init_chg_pretime;
		axp_sply_init_data.chgcsttime = axp20_config.pmu_init_chg_csttime;
		axp_regu_info = axp20_regu_init();
		if (NULL == axp_regu_info) {
			printk("%s fetch regu tree err\n", __func__);
			return -1;
		} else {
			axp_pdata.num_regl_devs = 7;
			axp_pdata.regl_devs = (struct axp_funcdev_info *)axp_regu_info;
			if (NULL == axp_pdata.regl_devs) {
				printk(KERN_ERR "%s: get regl_devs failed\n", __func__);
				return -1;
			}
		}

#ifdef	CONFIG_AXP_TWI_USED
		
		axp_mfd_i2c_board_info[0].addr = axp20_config.pmu_twi_addr;
		if(axp20_config.pmu_irq_id)
			axp_mfd_i2c_board_info[0].irq = axp20_config.pmu_irq_id;
		else{
			axp20_config.pmu_irq_io_id = gpio_to_irq(axp20_config.pmu_irq_io.gpio);
			axp_mfd_i2c_board_info[0].irq = axp20_config.pmu_irq_io_id;
		}
		ret = i2c_add_driver(&axp_i2c_driver);
		if (ret < 0) {
			printk("axp_i2c_driver add failed\n");
			return ret;
		}

		ret = i2c_register_board_info(axp20_config.pmu_twi_id, axp_mfd_i2c_board_info, ARRAY_SIZE(axp_mfd_i2c_board_info));
		if (ret < 0) {
			printk("axp_i2c_board_info add failed\n");
			return ret;
		}
#else
		ret = platform_driver_register(&axp20_platform_driver);
		if (IS_ERR_VALUE(ret)) {
			printk("register axp20 platform driver failed\n");
			return ret;
		}
#endif
	} else {
		ret = -1;
	}
        return ret;
}

arch_initcall(axp20_board_init);

MODULE_DESCRIPTION("ALLWINNERTECH axp board");
MODULE_AUTHOR("Ming Li");
MODULE_LICENSE("GPL");


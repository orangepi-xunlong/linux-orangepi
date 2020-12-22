#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/module.h>
#include "axp19-mfd.h"
#include "axp19-regu.h"
#include "../axp-cfg.h"

struct axp_config_info axp19_config;
static struct axp_dev axp19_dev;

static struct power_supply_info battery_data ={
        .name ="PTI PL336078",
	.technology = POWER_SUPPLY_TECHNOLOGY_LiFe,
	.voltage_max_design = 4200000,
	.voltage_min_design = 2700000,
	.charge_full_design = 1450,
	.energy_full_design = 1450,
	.use_for_apm = 1,
};

static void axp_battery_low(void)
{
#if defined(CONFIG_APM_EMULATION)
	apm_queue_event(APM_LOW_BATTERY);
#endif
}

static void axp_battery_critical(void)
{
#if defined(CONFIG_APM_EMULATION)
	apm_queue_event(APM_CRITICAL_SUSPEND);
#endif
}

static struct axp_supply_init_data axp_sply_init_data = {
	.battery_info = &battery_data,
	.chgcur = 700,
	.chgvol = 4200,
	.chgend =  60,
	.chgen = 1,
	.sample_time = 25,
	.chgpretime = 40,
	.chgcsttime = 480,
	.battery_low = axp_battery_low,
	.battery_critical = axp_battery_critical,
};

static struct axp_funcdev_info axp_splydev[]={
	{   .name = "axp19-supplyer",
		.id = AXP19_ID_SUPPLY,
        .platform_data = &axp_sply_init_data,
    },
};

static struct axp_platform_data axp_pdata = {
	.num_sply_devs = ARRAY_SIZE(axp_splydev),
	.sply_devs = axp_splydev,
};

static struct axp_mfd_chip_ops axp19_ops[] = {
	[0] = {
		.init_chip    = axp19_init_chip,
		.enable_irqs  = axp19_enable_irqs,
		.disable_irqs = axp19_disable_irqs,
		.read_irqs    = axp19_read_irqs,
	},
};

#ifdef	CONFIG_AXP_TWI_USED
static const struct i2c_device_id axp_i2c_id_table[] = {
	{ "axp19_board", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, axp_i2c_id_table);

static struct i2c_board_info __initdata axp_mfd_i2c_board_info[] = {
	{
		.type = "axp19_board",
		.addr = 0x34,
	},
};

static irqreturn_t axp_mfd_irq_handler(int irq, void *data)
{
	struct axp_dev *chip = data;
	disable_irq_nosync(irq);
	(void)schedule_work(&chip->irq_work);

	return IRQ_HANDLED;
}

static int axp_i2c_probe(struct i2c_client *client,
				  const struct i2c_device_id *id)
{
	int ret = 0;

	axp19_dev.client = client;
	axp19_dev.dev = &client->dev;

	i2c_set_clientdata(client, &axp19_dev);

	axp19_dev.ops = &axp19_ops[0];
	axp19_dev.attrs = axp19_mfd_attrs;
	axp19_dev.attrs_number = ARRAY_SIZE(axp19_mfd_attrs);
	axp19_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp19_dev);
	if (ret < 0) {
		printk("axp mfd register failed\n");
		return ret;
	}

	ret = request_irq(client->irq, axp_mfd_irq_handler,
		IRQF_SHARED|IRQF_DISABLED, "axp19", &axp19_dev);
	if (ret) {
		dev_err(&client->dev, "failed to request irq %d\n",
				client->irq);
		goto out_free_chip;
	}

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
		.name	= "axp19_board",
		.owner	= THIS_MODULE,
	},
	.probe		= axp_i2c_probe,
	.remove		= axp_i2c_remove,
	.id_table	= axp_i2c_id_table,
};
#else
static int  axp19_platform_probe(struct platform_device *pdev)
{
	int ret = 0;

	axp19_dev.dev = &pdev->dev;
	dev_set_drvdata(axp19_dev.dev, &axp19_dev);
	axp19_dev.client = (struct i2c_client *)pdev;

	axp19_dev.ops = &axp19_ops[0];
	axp19_dev.attrs = axp19_mfd_attrs;
	axp19_dev.attrs_number = ARRAY_SIZE(axp19_mfd_attrs);
	axp19_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp19_dev);
	if (ret < 0) {
		printk("axp19 mfd register failed\n");
		return ret;
	}
	return 0;
}

static struct platform_device axp19_platform_device = {
	.name		    = "axp19_board",
	.id		        = PLATFORM_DEVID_NONE,
};

static struct platform_driver axp19_platform_driver = {
	.probe		= axp19_platform_probe,
	.driver		= {
		.name	= "axp19_board",
		.owner	= THIS_MODULE,
	},
};
#endif

static int __init axp19_board_init(void)
{
	int ret = 0;
	struct axp_funcdev_info (* axp_regu_info)[8] = NULL;

	ret = axp_fetch_sysconfig_para("pmu1_para", &axp19_config);
	if (ret) {
		printk("%s fetch sysconfig err\n", __func__);
		return -1;
	}

	if (axp19_config.pmu_used) {
		axp_regu_info = axp19_regu_init();
		if (NULL == axp_regu_info) {
			printk("%s fetch regu tree err\n", __func__);
			return -1;
		} else {
			axp_pdata.num_regl_devs = 8;
			axp_pdata.regl_devs = (struct axp_funcdev_info *)axp_regu_info;
		}

#ifdef	CONFIG_AXP_TWI_USED
		ret = i2c_add_driver(&axp_i2c_driver);
		if (ret < 0) {
			printk("axp_i2c_driver add failed\n");
			return ret;
		}

		ret = i2c_register_board_info(1, axp_mfd_i2c_board_info, ARRAY_SIZE(axp_mfd_i2c_board_info));
		if (ret < 0) {
			printk("axp_i2c_board_info add failed\n");
			return ret;
		}
#else
		ret = platform_driver_register(&axp19_platform_driver);
		if (IS_ERR_VALUE(ret)) {
			printk("register axp19 platform driver failed\n");
			return ret;
		}
		ret = platform_device_register(&axp19_platform_device);
		if (IS_ERR_VALUE(ret)) {
			printk("register axp19 platform device failed\n");
			return ret;
		}
#endif
	} else {
		ret = -1;
	}
        return ret;
}

#ifdef	CONFIG_AXP_TWI_USED
arch_initcall(axp19_board_init);
#else
subsys_initcall(axp19_board_init);
#endif


MODULE_DESCRIPTION("power axp19 board");
MODULE_AUTHOR("liming");
MODULE_LICENSE("GPL");


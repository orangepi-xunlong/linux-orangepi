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
#include "axp15-mfd.h"
#include "axp-cfg.h"

static struct axp_dev axp15_dev;

/* Reverse engineered partly from Platformx drivers */
enum axp_regls{
	axp155_vcc_dcdc1,
	axp155_vcc_dcdc2,
	axp155_vcc_dcdc3,
	axp155_vcc_dcdc4,
	axp155_vcc_dcdc5,
	axp155_vcc_ldo1,
	axp155_vcc_ldo2,
	axp155_vcc_ldo3,
	axp155_vcc_ldo4,
	axp155_vcc_ldo5,
	axp155_vcc_ldo6,
	axp155_vcc_ldo7,
	axp155_vcc_ldo8,
	axp155_vcc_ldo9,
	axp155_vcc_ldo10,

	axp155_vcc_sw0,
};

/* The values of the various regulator constraints are obviously dependent
 * on exactly what is wired to each ldo.  Unfortunately this information is
 * not generally available.  More information has been requested from Xbow
 * but as of yet they haven't been forthcoming.
 *
 * Some of these are clearly Stargate 2 related (no way of plugging
 * in an lcd on the IM2 for example!).
 */
static struct regulator_consumer_supply axp15_dcdc1_data[] = {
		{
			.supply = "axp15_dcdc1",
		},
	};

static struct regulator_consumer_supply axp15_dcdc2_data[] = {
		{
			.supply = "axp15_dcdc2",
		},
	};

static struct regulator_consumer_supply axp15_dcdc3_data[] = {
		{
			.supply = "axp15_dcdc3",
		},
	};

static struct regulator_consumer_supply axp15_dcdc4_data[] = {
		{
			.supply = "axp15_dcdc4",
		},
	};

static struct regulator_consumer_supply axp15_dcdc5_data[] = {
		{
			.supply = "axp15_dcdc5",
		},
	};


static struct regulator_consumer_supply axp15_ldo1_data[] = {
		{
			.supply = "axp15_aldo1",
		},
	};


static struct regulator_consumer_supply axp15_ldo2_data[] = {
		{
			.supply = "axp15_aldo2",
		},
	};

static struct regulator_consumer_supply axp15_ldo3_data[] = {
		{
			.supply = "axp15_aldo3",
		},
	};

static struct regulator_consumer_supply axp15_ldo4_data[] = {
		{
			.supply = "axp15_bldo1",
		},
	};

static struct regulator_consumer_supply axp15_ldo5_data[] = {
		{
			.supply = "axp15_bldo2",
		},
	};


static struct regulator_consumer_supply axp15_ldo6_data[] = {
		{
			.supply = "axp15_bldo3",
		},
	};

static struct regulator_consumer_supply axp15_ldo7_data[] = {
		{
			.supply = "axp15_bldo4",
		},
	};

static struct regulator_consumer_supply axp15_ldo8_data[] = {
		{
			.supply = "axp15_cldo1",
		},
	};

static struct regulator_consumer_supply axp15_ldo9_data[] = {
		{
			.supply = "axp15_cldo2",
		},
	};


static struct regulator_consumer_supply axp15_ldo10_data[] = {
		{
			.supply = "axp15_cldo3",
		},
	};

static struct regulator_consumer_supply axp15_sw_data[] = {
		{
			.supply = "axp15_sw0",
		},
	};

static struct regulator_init_data axp_regl_init_data[] = {
	[axp155_vcc_dcdc1] = {
		.constraints = {
			.name = "axp15_dcdc1",
			.min_uV = 600000,
			.max_uV = 1520000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_dcdc1_data),
		.consumer_supplies = axp15_dcdc1_data,
	},
	[axp155_vcc_dcdc2] = {
		.constraints = {
			.name = "axp15_dcdc2",
			.min_uV = 1000000,
			.max_uV = 2550000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_dcdc2_data),
		.consumer_supplies = axp15_dcdc2_data,
	},
	[axp155_vcc_dcdc3] = {
		.constraints = {
			.name = "axp15_dcdc3",
			.min_uV = 600000,
			.max_uV = 1520000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_dcdc3_data),
		.consumer_supplies = axp15_dcdc3_data,
	},
	[axp155_vcc_dcdc4] = {
		.constraints = {
			.name = "axp15_dcdc4",
			.min_uV = 600000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_dcdc4_data),
		.consumer_supplies = axp15_dcdc4_data,
	},
	[axp155_vcc_dcdc5] = {
		.constraints = {
			.name = "axp15_dcdc5",
			.min_uV = 1100000,
			.max_uV = 3400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_dcdc5_data),
		.consumer_supplies = axp15_dcdc5_data,
	},
	[axp155_vcc_ldo1] = {
		.constraints = { 
			.name = "axp15_aldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo1_data),
		.consumer_supplies = axp15_ldo1_data,
	},
	[axp155_vcc_ldo2] = {
		.constraints = { 
			.name = "axp15_aldo2",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo2_data),
		.consumer_supplies = axp15_ldo2_data,
	},
	[axp155_vcc_ldo3] = {
		.constraints = {
			.name = "axp15_aldo3",
			.min_uV =  700000,
			.max_uV =  3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo3_data),
		.consumer_supplies = axp15_ldo3_data,
	},
	[axp155_vcc_ldo4] = {
		.supply_regulator = "axp15_dcdc5",
		.constraints = {
			.name = "axp15_bldo1",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo4_data),
		.consumer_supplies = axp15_ldo4_data,
	},
	[axp155_vcc_ldo5] = {
		.supply_regulator = "axp15_dcdc5",
		.constraints = { 
			.name = "axp15_bldo2",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo5_data),
		.consumer_supplies = axp15_ldo5_data,
	},
	[axp155_vcc_ldo6] = {
		.supply_regulator = "axp15_dcdc5",
		.constraints = { 
			.name = "axp15_bldo3",
			.min_uV = 700000,
			.max_uV = 1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo6_data),
		.consumer_supplies = axp15_ldo6_data,
	},
	[axp155_vcc_ldo7] = {
		.supply_regulator = "axp15_dcdc5",
		.constraints = {
			.name = "axp15_bldo4",
			.min_uV =  700000,
			.max_uV =  1900000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo7_data),
		.consumer_supplies = axp15_ldo7_data,
	},
	[axp155_vcc_ldo8] = {
		.constraints = {
			.name = "axp15_cldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo8_data),
		.consumer_supplies = axp15_ldo8_data,
	},
	[axp155_vcc_ldo9] = {
		.constraints = { 
			.name = "axp15_cldo2",
			.min_uV = 700000,
			.max_uV = 4200000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo9_data),
		.consumer_supplies = axp15_ldo9_data,
	},
	[axp155_vcc_ldo10] = {
		.constraints = {
			.name = "axp15_cldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_ldo10_data),
		.consumer_supplies = axp15_ldo10_data,
	},
	[axp155_vcc_sw0] = {
		.constraints = {
			.name = "axp15_sw0",
			.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(axp15_sw_data),
		.consumer_supplies = axp15_sw_data,
	},
};

static struct axp_funcdev_info axp_regldevs[] = {
	{
		.name = "axp15-regulator",
		.id = AXP15_ID_DCDC1,
		.platform_data = &axp_regl_init_data[axp155_vcc_dcdc1],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_DCDC2,
		.platform_data = &axp_regl_init_data[axp155_vcc_dcdc2],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_DCDC3,
		.platform_data = &axp_regl_init_data[axp155_vcc_dcdc3],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_DCDC4,
		.platform_data = &axp_regl_init_data[axp155_vcc_dcdc4],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_DCDC5,
		.platform_data = &axp_regl_init_data[axp155_vcc_dcdc5],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO1,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo1],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO2,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo2],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO3,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo3],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO4,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo4],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO5,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo5],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO6,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo6],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO7,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo7],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO8,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo8],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO9,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo9],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_LDO10,
		.platform_data = &axp_regl_init_data[axp155_vcc_ldo10],
	}, {
		.name = "axp15-regulator",
		.id = AXP15_ID_SW0,
		.platform_data = &axp_regl_init_data[axp155_vcc_sw0],
	}, 
};


static struct axp_funcdev_info axp_splydev[]={
	{
		.name = "axp15-irq",
		.id = AXP15_ID_SUPPLY,
		.platform_data = NULL,
	},
};


static struct axp_platform_data axp_pdata = {
	.num_regl_devs = ARRAY_SIZE(axp_regldevs),
	.num_sply_devs = ARRAY_SIZE(axp_splydev),
	.regl_devs = axp_regldevs,
	.sply_devs = axp_splydev,
};

static struct axp_mfd_chip_ops axp15_ops[] = {
	[0] = {
		.init_chip    = axp15_init_chip,
		.enable_irqs  = axp15_enable_irqs,
		.disable_irqs = axp15_disable_irqs,
		.read_irqs    = axp15_read_irqs,
	},
};

#ifdef	CONFIG_AXP_TWI_USED
static const struct i2c_device_id axp_i2c_id_table[] = {
	{ "axp15_board", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, axp_i2c_id_table);

static struct i2c_board_info __initdata axp_mfd_i2c_board_info[] = {
	{
		.type = "axp15_board",
		.addr = 0x36,
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

	axp15_dev.client = client;
	axp15_dev.dev = &client->dev;

	i2c_set_clientdata(client, &axp15_dev);

	axp15_dev.ops = &axp15_ops[0];
	axp15_dev.attrs = axp15_mfd_attrs;
	axp15_dev.attrs_number = ARRAY_SIZE(axp15_mfd_attrs);
	axp15_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp15_dev);
	if (ret < 0) {
		printk("axp mfd register failed\n");
		return ret;
	}

	ret = request_irq(client->irq, axp_mfd_irq_handler,
		IRQF_SHARED|IRQF_DISABLED, "axp15", &axp15_dev);
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
		.name	= "axp15_board",
		.owner	= THIS_MODULE,
	},
	.probe		= axp_i2c_probe,
	.remove		= axp_i2c_remove,
	.id_table	= axp_i2c_id_table,
};
#else
static int  axp15_platform_probe(struct platform_device *pdev)
{
	int ret = 0;
#ifdef CONFIG_AXP15_IRQ
    struct platform_device *irqdev;
#endif

	axp15_dev.dev = &pdev->dev;
	dev_set_drvdata(axp15_dev.dev, &axp15_dev);
	axp15_dev.client = (struct i2c_client *)pdev;

	axp15_dev.ops = &axp15_ops[0];
	axp15_dev.attrs = axp15_mfd_attrs;
	axp15_dev.attrs_number = ARRAY_SIZE(axp15_mfd_attrs);
	axp15_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp15_dev);
	if (ret < 0) {
		printk("axp15 mfd register failed\n");
		return ret;
	}

#ifdef CONFIG_AXP15_IRQ
    /* register irq-handler for A80-P1 */
    irqdev = platform_device_alloc("axp15-irq-handler", 0);
	irqdev->dev.parent = axp15_dev.dev;
	ret = platform_device_add(irqdev);
	if (ret) {
		printk("axp15-irq-handler device register failed\n");
		return ret;
	}
#endif

	return 0;
}

static struct platform_device axp15_platform_device = {
	.name		    = "axp15_board",
	.id		        = PLATFORM_DEVID_NONE,
};

static struct platform_driver axp15_platform_driver = {
	.probe		= axp15_platform_probe,
	.driver		= {
		.name	= "axp15_board",
		.owner	= THIS_MODULE,
	},
};
#endif

static int __init axp15_board_init(void)
{
	int ret = 0;
	
	ret = axp_fetch_sysconfig_para("pmu2_para", &axp15_config);
	if (ret) {
		printk("%s fetch sysconfig err", __func__);
		return -1;
	}
	if (axp15_config.pmu_used) {
#ifdef	CONFIG_AXP_TWI_USED
		ret = i2c_add_driver(&axp_i2c_driver);
		if (ret < 0) {
			printk("axp_i2c_driver add failed\n");
			return ret;
		}

		axp_mfd_i2c_board_info[0].addr = axp15_config.pmu_twi_addr;
		axp_mfd_i2c_board_info[0].irq = axp15_config.pmu_irq_id;
		ret = i2c_register_board_info(axp15_config.pmu_twi_id, axp_mfd_i2c_board_info, ARRAY_SIZE(axp_mfd_i2c_board_info));
		if (ret < 0) {
			printk("axp_i2c_board_info add failed\n");
			return ret;
		}
#else
		ret = platform_driver_register(&axp15_platform_driver);
		if (IS_ERR_VALUE(ret)) {
			printk("register axp15 platform driver failed\n");
			return ret;
		}
		ret = platform_device_register(&axp15_platform_device);
		if (IS_ERR_VALUE(ret)) {
			printk("register axp15 platform device failed\n");
			return ret;
		}
#endif
        } else {
        	ret = -1;
        }
        return ret;
}

#ifdef	CONFIG_AXP_TWI_USED
arch_initcall(axp15_board_init);
#else
subsys_initcall(axp15_board_init);
#endif

MODULE_DESCRIPTION("X-POWERS axp board");
MODULE_AUTHOR("Weijin Zhong");
MODULE_LICENSE("GPL");

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <mach/irqs.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <linux/mfd/axp-mfd.h>
#include "axp22-mfd.h"
#include "axp-cfg.h"

static struct axp_dev axp22_dev;

/* Reverse engineered partly from Platformx drivers */
enum axp_regls{
	vcc_dcdc1,
	vcc_dcdc2,
	vcc_dcdc3,
	vcc_dcdc4,
	vcc_dcdc5,
	vcc_ldo1,
	vcc_ldo2,
	vcc_ldo3,
	vcc_ldo4,
	vcc_ldo5,
	vcc_ldo6,
	vcc_ldo7,
	vcc_ldo8,
	vcc_ldo9,
	vcc_ldo10,
	vcc_ldo11,
	vcc_ldo12,

	vcc_ldoio0,
	vcc_ldoio1,
#ifdef CONFIG_AXP809
	vcc_sw0,
#endif
	vcc_dc1sw,
};

/* The values of the various regulator constraints are obviously dependent
 * on exactly what is wired to each ldo.  Unfortunately this information is
 * not generally available.  More information has been requested from Xbow
 * but as of yet they haven't been forthcoming.
 *
 * Some of these are clearly Stargate 2 related (no way of plugging
 * in an lcd on the IM2 for example!).
 */

static struct regulator_consumer_supply ldo1_data[] = {
		{
			.supply = "axp22_rtc",
		},
	};


static struct regulator_consumer_supply ldo2_data[] = {
		{
			.supply = "axp22_aldo1",
		},
	};

static struct regulator_consumer_supply ldo3_data[] = {
		{
			.supply = "axp22_aldo2",
		},
	};

static struct regulator_consumer_supply ldo4_data[] = {
		{
			.supply = "axp22_aldo3",
		},
	};

static struct regulator_consumer_supply ldo5_data[] = {
		{
			.supply = "axp22_dldo1",
		},
	};

static struct regulator_consumer_supply ldo6_data[] = {
		{
			.supply = "axp22_dldo2",
		},
	};
#ifdef CONFIG_AXP809
#else
static struct regulator_consumer_supply ldo7_data[] = {
		{
			.supply = "axp22_dldo3",
		},
	};

static struct regulator_consumer_supply ldo8_data[] = {
		{
			.supply = "axp22_dldo4",
		},
	};
#endif


static struct regulator_consumer_supply ldo9_data[] = {
		{
			.supply = "axp22_eldo1",
		},
	};


static struct regulator_consumer_supply ldo10_data[] = {
		{
			.supply = "axp22_eldo2",
		},
	};

static struct regulator_consumer_supply ldo11_data[] = {
		{
			.supply = "axp22_eldo3",
		},
	};

static struct regulator_consumer_supply ldo12_data[] = {
		{
			.supply = "axp22_dc5ldo",
		},
	};

static struct regulator_consumer_supply ldoio0_data[] = {
		{
			.supply = "axp22_ldoio0",
		},
	};

static struct regulator_consumer_supply ldoio1_data[] = {
		{
			.supply = "axp22_ldoio1",
		},
	};

static struct regulator_consumer_supply dcdc1_data[] = {
		{
			.supply = "axp22_dcdc1",
		},
	};

static struct regulator_consumer_supply dcdc2_data[] = {
		{
			.supply = "axp22_dcdc2",
		},
	};

static struct regulator_consumer_supply dcdc3_data[] = {
		{
			.supply = "axp22_dcdc3",
		},
	};

static struct regulator_consumer_supply dcdc4_data[] = {
		{
			.supply = "axp22_dcdc4",
		},
	};

static struct regulator_consumer_supply dcdc5_data[] = {
		{
			.supply = "axp22_dcdc5",
		},
	};

#ifdef CONFIG_AXP809
static struct regulator_consumer_supply sw_data[] = {
		{
			.supply = "axp22_sw0",
		},
	};
#endif
static struct regulator_consumer_supply dc1sw_data[] = {
		{
			.supply = "axp22_dc1sw",
		},
	};

static struct regulator_init_data axp_regl_init_data[] = {
	[vcc_dcdc1] = {
		.constraints = {
			.name = "axp22_dcdc1",
			.min_uV = 1600000,
			.max_uV = 3400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(dcdc1_data),
		.consumer_supplies = dcdc1_data,
	},
	[vcc_dcdc2] = {
		.constraints = {
			.name = "axp22_dcdc2",
			.min_uV = 600000,
			.max_uV = 1540000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(dcdc2_data),
		.consumer_supplies = dcdc2_data,
	},
	[vcc_dcdc3] = {
		.constraints = {
			.name = "axp22_dcdc3",
			.min_uV = 600000,
			.max_uV = 1860000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(dcdc3_data),
		.consumer_supplies = dcdc3_data,
	},
	[vcc_dcdc4] = {
		.constraints = {
			.name = "axp22_dcdc4",
			.min_uV = 600000,
#ifdef CONFIG_AXP809
			.max_uV = 2600000,
#else
			.max_uV = 1540000,
#endif
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(dcdc4_data),
		.consumer_supplies = dcdc4_data,
	},
	[vcc_dcdc5] = {
		.constraints = {
			.name = "axp22_dcdc5",
			.min_uV = 1000000,
			.max_uV = 2550000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(dcdc5_data),
		.consumer_supplies = dcdc5_data,
	},
	[vcc_ldo1] = {
		.constraints = { 
			.name = "axp22_rtc",
			.min_uV =  AXP22LDO1 * 1000,
			.max_uV =  AXP22LDO1 * 1000,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo1_data),
		.consumer_supplies = ldo1_data,
	},
	[vcc_ldo2] = {
		.constraints = { 
			.name = "axp22_aldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo2_data),
		.consumer_supplies = ldo2_data,
	},
	[vcc_ldo3] = {
		.constraints = {
			.name = "axp22_aldo2",
			.min_uV =  700000,
			.max_uV =  3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo3_data),
		.consumer_supplies = ldo3_data,
	},
	[vcc_ldo4] = {
		.constraints = {
			.name = "axp22_aldo3",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo4_data),
		.consumer_supplies = ldo4_data,
	},
	[vcc_ldo5] = {
		.constraints = { 
			.name = "axp22_dldo1",
			.min_uV = 700000,
#ifdef CONFIG_AXP809
			.max_uV = 4200000,
#else
			.max_uV = 3300000,
#endif
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo5_data),
		.consumer_supplies = ldo5_data,
	},
	[vcc_ldo6] = {
		.constraints = { 
			.name = "axp22_dldo2",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo6_data),
		.consumer_supplies = ldo6_data,
	},
#ifdef CONFIG_AXP809
#else
	[vcc_ldo7] = {
		.constraints = {
			.name = "axp22_dldo3",
			.min_uV =  700000,
			.max_uV =  3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo7_data),
		.consumer_supplies = ldo7_data,
	},
	[vcc_ldo8] = {
		.constraints = {
			.name = "axp22_dldo4",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo8_data),
		.consumer_supplies = ldo8_data,
	},
#endif
	[vcc_ldo9] = {
#ifndef CONFIG_AXP809
		.supply_regulator = "axp22_dcdc1",
#endif
		.constraints = { 
			.name = "axp22_eldo1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo9_data),
		.consumer_supplies = ldo9_data,
	},
	[vcc_ldo10] = {
#ifndef CONFIG_AXP809
		.supply_regulator = "axp22_dcdc1",
#endif
		.constraints = {
			.name = "axp22_eldo2",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo10_data),
		.consumer_supplies = ldo10_data,
	},
	[vcc_ldo11] = {
#ifndef CONFIG_AXP809
		.supply_regulator = "axp22_dcdc1",
#endif
		.constraints = {
			.name = "axp22_eldo3",
			.min_uV =  700000,
			.max_uV =  3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo11_data),
		.consumer_supplies = ldo11_data,
	},
	[vcc_ldo12] = {
		.constraints = {
			.name = "axp22_dc5ldo",
			.min_uV = 700000,
			.max_uV = 1400000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldo12_data),
		.consumer_supplies = ldo12_data,
	},
	[vcc_ldoio0] = {
		.constraints = {
			.name = "axp22_ldoio0",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio0_data),
		.consumer_supplies = ldoio0_data,
	},
	[vcc_ldoio1] = {
		.constraints = {
			.name = "axp22_ldoio1",
			.min_uV = 700000,
			.max_uV = 3300000,
			.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(ldoio1_data),
		.consumer_supplies = ldoio1_data,
	},
#ifdef CONFIG_AXP809
	[vcc_sw0] = {
		.constraints = {
			.name = "axp22_sw0",
			.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(sw_data),
		.consumer_supplies = sw_data,
	},
#endif
	[vcc_dc1sw] = {
		.supply_regulator = "axp22_dcdc1",
		.constraints = {
			.name = "axp22_dc1sw",
			.valid_ops_mask = REGULATOR_CHANGE_STATUS,
		},
		.num_consumer_supplies = ARRAY_SIZE(dc1sw_data),
		.consumer_supplies = dc1sw_data,
	},
};

static struct axp_funcdev_info axp_regldevs[] = {
	{
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC1,
		.platform_data = &axp_regl_init_data[vcc_dcdc1],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC2,
		.platform_data = &axp_regl_init_data[vcc_dcdc2],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC3,
		.platform_data = &axp_regl_init_data[vcc_dcdc3],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC4,
		.platform_data = &axp_regl_init_data[vcc_dcdc4],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_DCDC5,
		.platform_data = &axp_regl_init_data[vcc_dcdc5],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO1,
		.platform_data = &axp_regl_init_data[vcc_ldo1],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO2,
		.platform_data = &axp_regl_init_data[vcc_ldo2],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO3,
		.platform_data = &axp_regl_init_data[vcc_ldo3],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO4,
		.platform_data = &axp_regl_init_data[vcc_ldo4],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO5,
		.platform_data = &axp_regl_init_data[vcc_ldo5],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO6,
		.platform_data = &axp_regl_init_data[vcc_ldo6],
	}, {
#ifdef CONFIG_AXP809
#else
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO7,
		.platform_data = &axp_regl_init_data[vcc_ldo7],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO8,
		.platform_data = &axp_regl_init_data[vcc_ldo8],
	}, {
#endif
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO9,
		.platform_data = &axp_regl_init_data[vcc_ldo9],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO10,
		.platform_data = &axp_regl_init_data[vcc_ldo10],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO11,
		.platform_data = &axp_regl_init_data[vcc_ldo11],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDO12,
		.platform_data = &axp_regl_init_data[vcc_ldo12],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDOIO0,
		.platform_data = &axp_regl_init_data[vcc_ldoio0],
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_LDOIO1,
		.platform_data = &axp_regl_init_data[vcc_ldoio1],
#ifdef CONFIG_AXP809
	}, {
		.name = "axp22-regulator",
		.id = AXP22_ID_SW0,
		.platform_data = &axp_regl_init_data[vcc_sw0],
#endif
	},{
		.name = "axp22-regulator",
		.id = AXP22_ID_SW1,
		.platform_data = &axp_regl_init_data[vcc_dc1sw],
	}
};

static struct power_supply_info battery_data ={
	.name ="PTI PL336078",
	.technology = POWER_SUPPLY_TECHNOLOGY_LiFe,
	.voltage_max_design = 4200000,
	.voltage_min_design = 3500000,
//	.energy_full_design = pmu_battery_cap,
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
		.name = "axp22-supplyer",
		.id = AXP22_ID_SUPPLY,
		.platform_data = &axp_sply_init_data,
	},
};

static struct axp_platform_data axp_pdata = {
	.num_regl_devs = ARRAY_SIZE(axp_regldevs),
	.num_sply_devs = ARRAY_SIZE(axp_splydev),
	.regl_devs = axp_regldevs,
	.sply_devs = axp_splydev,
};

static struct axp_mfd_chip_ops axp22_ops[] = {
	[0] = {
		.init_chip    = axp22_init_chip,
		.enable_irqs  = axp22_enable_irqs,
		.disable_irqs = axp22_disable_irqs,
		.read_irqs    = axp22_read_irqs,
	},
};


#ifdef	CONFIG_AXP_TWI_USED
static const struct i2c_device_id axp_i2c_id_table[] = {
	{ "axp22_board", 0 },
	{},
};
MODULE_DEVICE_TABLE(i2c, axp_i2c_id_table);

static struct i2c_board_info __initdata axp_mfd_i2c_board_info[] = {
	{
		.type = "axp22_board",
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

	axp22_dev.client = client;
	axp22_dev.dev = &client->dev;

	i2c_set_clientdata(client, &axp22_dev);

	axp22_dev.ops = &axp22_ops[0];
	axp22_dev.attrs = axp22_mfd_attrs;
	axp22_dev.attrs_number = ARRAY_SIZE(axp22_mfd_attrs);
	axp22_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp22_dev);
	if (ret < 0) {
		printk("axp mfd register failed\n");
		return ret;
	}

	ret = request_irq(client->irq, axp_mfd_irq_handler,
		IRQF_SHARED|IRQF_DISABLED, "axp22", &axp22_dev);
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
		.name	= "axp22_board",
		.owner	= THIS_MODULE,
	},
	.probe		= axp_i2c_probe,
	.remove		= axp_i2c_remove,
	.id_table	= axp_i2c_id_table,
};
#else
static int  axp22_platform_probe(struct platform_device *pdev)
{
	int ret = 0;

	axp22_dev.dev = &pdev->dev;
	dev_set_drvdata(axp22_dev.dev, &axp22_dev);
	axp22_dev.client = (struct i2c_client *)pdev;

	axp22_dev.ops = &axp22_ops[0];
	axp22_dev.attrs = axp22_mfd_attrs;
	axp22_dev.attrs_number = ARRAY_SIZE(axp22_mfd_attrs);
	axp22_dev.pdata = &axp_pdata;
	ret = axp_register_mfd(&axp22_dev);
	if (ret < 0) {
		printk("axp22 mfd register failed\n");
		return ret;
	}
	return 0;
}

static struct platform_device axp22_platform_device = {
	.name		    = "axp22_board",
	.id		        = PLATFORM_DEVID_NONE,
};

static struct platform_driver axp22_platform_driver = {
	.probe		= axp22_platform_probe,
	.driver		= {
		.name	= "axp22_board",
		.owner	= THIS_MODULE,
	},
};
#endif

static int __init axp22_board_init(void)
{
	int ret = 0;

	ret = axp_fetch_sysconfig_para("pmu1_para", &axp22_config);
	if (ret) {
		printk("%s fetch sysconfig err", __func__);
		return -1;
	}
	if (axp22_config.pmu_used) {
		battery_data.voltage_max_design = axp22_config.pmu_init_chgvol;
		battery_data.voltage_min_design = axp22_config.pmu_pwroff_vol;
		battery_data.energy_full_design = axp22_config.pmu_battery_cap;
		axp_sply_init_data.chgcur = axp22_config.pmu_runtime_chgcur;
		axp_sply_init_data.chgvol = axp22_config.pmu_init_chgvol;
		axp_sply_init_data.chgend = axp22_config.pmu_init_chgend_rate;
		axp_sply_init_data.chgen = axp22_config.pmu_init_chg_enabled;
		axp_sply_init_data.sample_time = axp22_config.pmu_init_adc_freq;
		axp_sply_init_data.chgpretime = axp22_config.pmu_init_chg_pretime;
		axp_sply_init_data.chgcsttime = axp22_config.pmu_init_chg_csttime;
#ifdef	CONFIG_AXP_TWI_USED
		axp_mfd_i2c_board_info[0].addr = axp22_config.pmu_twi_addr;
		axp_mfd_i2c_board_info[0].irq = axp22_config.pmu_irq_id;

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
		ret = platform_driver_register(&axp22_platform_driver);
		if (IS_ERR_VALUE(ret)) {
			printk("register axp22 platform driver failed\n");
			return ret;
		}
		ret = platform_device_register(&axp22_platform_device);
		if (IS_ERR_VALUE(ret)) {
			printk("register axp22 platform device failed\n");
			return ret;
		}
#endif
	}
	return ret;
}

static void __exit axp22_board_exit(void)
{

}

#ifdef	CONFIG_AXP_TWI_USED
arch_initcall(axp22_board_init);
#else
subsys_initcall(axp22_board_init);
#endif

module_exit(axp22_board_exit);

MODULE_DESCRIPTION("X-POWERS axp board");
MODULE_AUTHOR("Weijin Zhong");
MODULE_LICENSE("GPL");


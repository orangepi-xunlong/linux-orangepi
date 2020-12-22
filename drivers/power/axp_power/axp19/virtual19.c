/*
 * reg-virtual-consumer.c
 *
 * Copyright 2008 Wolfson Microelectronics PLC.
 *
 * Author: Mark Brown <broonie@opensource.wolfsonmicro.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include "../virtual.h"

static int regulator_virtual_consumer_probe(struct platform_device *pdev)
{
	char *reg_id = pdev->dev.platform_data;
	struct virtual_consumer_data *drvdata;
	int ret, i;

	drvdata = kzalloc(sizeof(struct virtual_consumer_data), GFP_KERNEL);
	if (drvdata == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	mutex_init(&drvdata->lock);

	//drvdata->regulator = regulator_get(&pdev->dev, reg_id);
	drvdata->regulator = regulator_get(NULL, reg_id);
	//drvdata->regulator = regulator_get(NULL, "axp20_analog/fm");
	if (IS_ERR(drvdata->regulator)) {
		ret = PTR_ERR(drvdata->regulator);
		goto err;
	}

	for (i = 0; i < ARRAY_SIZE(attributes_virtual); i++) {
		ret = device_create_file(&pdev->dev, attributes_virtual[i]);
		if (ret != 0)
			goto err;
	}

	drvdata->mode = regulator_get_mode(drvdata->regulator);

	platform_set_drvdata(pdev, drvdata);

	return 0;

err:
	for (i = 0; i < ARRAY_SIZE(attributes_virtual); i++)
		device_remove_file(&pdev->dev, attributes_virtual[i]);
	kfree(drvdata);
	return ret;
}

static int regulator_virtual_consumer_remove(struct platform_device *pdev)
{
	struct virtual_consumer_data *drvdata = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < ARRAY_SIZE(attributes_virtual); i++)
		device_remove_file(&pdev->dev, attributes_virtual[i]);
	if (drvdata->enabled)
		regulator_disable(drvdata->regulator);
	regulator_put(drvdata->regulator);

	kfree(drvdata);

	return 0;
}

static struct platform_driver regulator_virtual_consumer_driver[] = {
	{
		.probe		= regulator_virtual_consumer_probe,
		.remove		= regulator_virtual_consumer_remove,
		.driver		= {
			.name		= "reg-19-cs-ldo2",
		},
	},{
		.probe		= regulator_virtual_consumer_probe,
		.remove		= regulator_virtual_consumer_remove,
		.driver		= {
			.name		= "reg-19-cs-ldo3",
		},
	},{
		.probe		= regulator_virtual_consumer_probe,
		.remove		= regulator_virtual_consumer_remove,
		.driver		= {
			.name		= "reg-19-cs-ldo4",
		},
	},{
		.probe		= regulator_virtual_consumer_probe,
		.remove		= regulator_virtual_consumer_remove,
		.driver		= {
			.name		= "reg-19-cs-dcdc1",
		},
	},{
		.probe		= regulator_virtual_consumer_probe,
		.remove		= regulator_virtual_consumer_remove,
		.driver		= {
			.name		= "reg-19-cs-dcdc2",
		},
	},{
		.probe		= regulator_virtual_consumer_probe,
		.remove		= regulator_virtual_consumer_remove,
		.driver		= {
			.name		= "reg-19-cs-dcdc3",
		},
	},{
		.probe		= regulator_virtual_consumer_probe,
		.remove		= regulator_virtual_consumer_remove,
		.driver		= {
			.name		= "reg-19-cs-ldoio0",
		},
	},
};


static int __init regulator_virtual_consumer_init(void)
{
	int j,ret;
	for (j = 0; j < ARRAY_SIZE(regulator_virtual_consumer_driver); j++){
		ret =  platform_driver_register(&regulator_virtual_consumer_driver[j]);
		if (ret)
			goto creat_drivers_failed;
	}
	return ret;

creat_drivers_failed:
	while (j--)
		platform_driver_unregister(&regulator_virtual_consumer_driver[j]);
	return ret;
}
module_init(regulator_virtual_consumer_init);

static void __exit regulator_virtual_consumer_exit(void)
{
	int j;
	for (j = ARRAY_SIZE(regulator_virtual_consumer_driver) - 1; j >= 0; j--){
			platform_driver_unregister(&regulator_virtual_consumer_driver[j]);
	}
}
module_exit(regulator_virtual_consumer_exit);

MODULE_AUTHOR("Donglu Zhang Krosspower");
MODULE_DESCRIPTION("Virtual regulator consumer");
MODULE_LICENSE("GPL");

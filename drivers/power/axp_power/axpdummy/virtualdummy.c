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

#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/module.h>
#include "../virtual.h"
#include "axpdummy-regu.h"
#include "virtualdummy.h"

static int regulator_virtual_consumer_probe(struct platform_device *pdev)
{
	char *reg_id = pdev->dev.platform_data;
	struct virtual_consumer_data *drvdata;
	int ret, i;
	const char *pmu_name;

	for (i = 0; i < AXP_ONLINE_SUM; i++) {
		pmu_name = get_pmu_cur_name(i);
		if (pmu_name == NULL)
			continue;

		if (strncmp("axpdummy", pmu_name, 8))
			continue;
		else
			break;
	}

	if (i == AXP_ONLINE_SUM)
		return 0;

	drvdata = kzalloc(sizeof(struct virtual_consumer_data), GFP_KERNEL);
	if (drvdata == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	mutex_init(&drvdata->lock);
	sprintf(drvdata->regu_name, reg_id);

	for (i = 0; i < ARRAY_SIZE(attributes_virtual); i++) {
		ret = device_create_file(&pdev->dev, attributes_virtual[i]);
		if (ret != 0)
			goto err;
	}

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

	kfree(drvdata);

	return 0;
}

static struct platform_driver regulator_virtual_consumer_driver[] = {
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo1"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo2"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo3"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo4"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo5"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo6"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo7"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo8"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo9"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo10"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo11"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo12"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo13"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo14"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo15"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo16"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo17"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo18"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo19"),
	VIRTUAL_DUMMY_DRIVER_DATA("reg-dummy-cs-ldo20"),
};

static int __init regulator_virtual_consumer_init(void)
{
	int j, ret;

	for (j = 0; j < axpdummy_ldo_count; j++) {
		ret =  platform_driver_register(
					&regulator_virtual_consumer_driver[j]);
		if (ret)
			goto creat_drivers_failed;
	}

	return ret;

creat_drivers_failed:
	while (j--)
		platform_driver_unregister(
				&regulator_virtual_consumer_driver[j]);

	return ret;
}
module_init(regulator_virtual_consumer_init);

static void __exit regulator_virtual_consumer_exit(void)
{
	int j;

	for (j = axpdummy_ldo_count - 1; j >= 0; j--)
			platform_driver_unregister(
				&regulator_virtual_consumer_driver[j]);
}
module_exit(regulator_virtual_consumer_exit);

MODULE_AUTHOR("Kyle Cheung");
MODULE_DESCRIPTION("Virtual regulator consumer");
MODULE_LICENSE("GPL");

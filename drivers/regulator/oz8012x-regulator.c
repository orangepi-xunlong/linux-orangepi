/*
 * Copyright (c) 2013-2015 allwinner@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/device.h>
#include <linux/arisc/arisc.h>

#define OZ8012X_ID_BASE         0
#define OZ8012X_ID_REGLATOR0     (OZ8012X_ID_BASE+0)

#define OZ8012X_REGULATOR(_pmic, _id, _min, _max, _step, _ops)  \
{                                                               \
    .name = #_pmic"_regulator"#_id,                             \
    .type = REGULATOR_VOLTAGE,                                  \
    .id   = _pmic##_ID_REGLATOR##_id,                           \
    .n_voltages = ARRAY_SIZE(oz8012x_voltages),                 \
    .ops = &_ops,                                               \
    .owner = THIS_MODULE,                                       \
}

/* init data for oz8012x regulator dev */
static struct regulator_consumer_supply oz8012x_output0[] =
{
    {
        .supply = "oz8012x/output0",
    },
};

static struct regulator_init_data oz8012x_init_data[] =
{
    [0] = {
        .constraints = {
            .name = "oz8012x_output0",
            .min_uV =  900000,
            .max_uV = 1200000,
            .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | REGULATOR_CHANGE_STATUS,
        },
        .num_consumer_supplies = ARRAY_SIZE(oz8012x_output0),
        .consumer_supplies = oz8012x_output0,
    },
};

static const int oz8012x_voltages[] = {
	900000,
	1000000,
	1100000,
	1200000,
};

static inline struct oz8012x *dev_to_oz8012x(struct device *dev)
{
    return dev_get_drvdata(dev);
}

static int oz8012x_list_voltage(struct regulator_dev *rdev,
                unsigned int selector)
{
    int id = rdev_get_id(rdev);

    if (id != OZ8012X_ID_REGLATOR0) {
        pr_err("%s: unsupportable regulator id %d\n", __func__, id);
        return -EINVAL;
    }

    if (selector >= ARRAY_SIZE(oz8012x_voltages))
        return -EINVAL;

    return oz8012x_voltages[selector];
}

static int oz8012x_is_enabled(struct regulator_dev *rdev)
{
    int id = rdev_get_id(rdev);

    if (id != OZ8012X_ID_REGLATOR0) {
        pr_err("%s: unsupportable regulator id %d\n", __func__, id);
        return -EINVAL;
    }

    /* TODO */
    return 1;
}

static int oz8012x_enable(struct regulator_dev *rdev)
{
    int id = rdev_get_id(rdev);

    if (id != OZ8012X_ID_REGLATOR0) {
        pr_err("%s: unsupportable regulator id %d\n", __func__, id);
        return -EINVAL;
    }

    /* TODO */
    return 0;
}

static int oz8012x_disable(struct regulator_dev *rdev)
{
    int id = rdev_get_id(rdev);

    if (id != OZ8012X_ID_REGLATOR0) {
        pr_err("%s: unsupportable regulator id %d\n", __func__, id);
        return -EINVAL;
    }

    /* TODO */
    return 0;
}

static int oz8012x_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV,
                unsigned *selector)
{
    int id = rdev_get_id(rdev);
    int ret, i = 0;
    int target_mV;

    if (id != OZ8012X_ID_REGLATOR0) {
        pr_err("%s: unsupportable regulator id %d\n", __func__, id);
        return -EINVAL;
    }

    while( min_uV > oz8012x_voltages[i])
        i++;

    target_mV = oz8012x_voltages[i]/1000;
    *selector = i;


    ret = arisc_pmu_set_voltage(OZ80120_POWER_VOL_DCDC, target_mV);
    if (ret) {
        pr_err("%s: set vref%d target volatge %dmV failed\n", __func__, id, target_mV);
        return -EIO;
    }
	pr_info("arisc pmu set voltage return %d\n", target_mV);

    return 0;
}

static int oz8012x_get_voltage(struct regulator_dev *rdev)
{
    int id = rdev_get_id(rdev);
    int ret;

    if (id != OZ8012X_ID_REGLATOR0) {
        pr_err("%s: unsupportable regulator id %d\n", __func__, id);
        return -EINVAL;
    }

	/* return voltage in mV */
    ret = arisc_pmu_get_voltage(OZ80120_POWER_VOL_DCDC);
    if (ret < 0) {
        pr_err("%s: read vref%d failed\n", __func__, id);
        return -EIO;
    }
	pr_info("arisc pmu get voltage return %d\n", ret);

	/* return voltage in uV */
    return (ret*1000);
}

static struct regulator_ops oz8012x_regulator_ops = {
    .set_voltage         = oz8012x_set_voltage,
    .get_voltage         = oz8012x_get_voltage,
    .list_voltage        = oz8012x_list_voltage,
    .is_enabled          = oz8012x_is_enabled,
    .enable              = oz8012x_enable,
    .disable             = oz8012x_disable,
};

/* regulator desc for oz8012x */
struct regulator_desc regulators[] = {
    OZ8012X_REGULATOR(OZ8012X, 0, 900, 1200, 100, oz8012x_regulator_ops),
};

static struct regulator_dev *oz8012x_rdev;

static int __init oz8012x_regulator_init(void)
{
    pr_info("%s: oz8012x pmic init.\n", __func__);

    /* register regulator dev */
    oz8012x_rdev = regulator_register(&regulators[0], NULL,
                    &oz8012x_init_data[0], NULL, NULL);
    if (IS_ERR(oz8012x_rdev))
        return PTR_ERR(oz8012x_rdev);

    return 0;
}

static void __exit oz8012x_regulator_exit(void)
{
    pr_info("%s: module exit.\n", __func__);
    regulator_unregister(oz8012x_rdev);
}

subsys_initcall_sync(oz8012x_regulator_init);
module_exit(oz8012x_regulator_exit);

MODULE_DESCRIPTION("oz8012x voltage regulator Driver");
MODULE_LICENSE("GPL v2");

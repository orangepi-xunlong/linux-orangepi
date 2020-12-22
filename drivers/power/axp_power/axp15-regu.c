/*
 * Regulators driver for X-POWERS AXP15x
 *
 * Copyright (C) 2013 X-POWERS Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/module.h>
#include <linux/arisc/arisc.h>
#include "axp15-regu.h"

static inline struct device *to_axp_dev(struct regulator_dev *rdev)
{
	return rdev_get_dev(rdev)->parent->parent;
}

static inline int check_range(struct axp_regulator_info *info,
				int min_uV, int max_uV)
{
	if (min_uV < info->min_uV || min_uV > info->max_uV)
		return -EINVAL;

	return 0;
}


/* AXP common operations */
static int axp_set_voltage(struct regulator_dev *rdev,
				  int min_uV, int max_uV,unsigned *selector)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct device *axp_dev = to_axp_dev(rdev);
	uint8_t val, mask;


	if (check_range(info, min_uV, max_uV)) {
		pr_err("invalid voltage range (%d, %d) uV\n", min_uV, max_uV);
		return -EINVAL;
	}
	if ((info->switch_uV != 0) && (info->step2_uV!= 0) && (min_uV > info->switch_uV)) {
		val = (info->switch_uV- info->min_uV + info->step1_uV - 1) / info->step1_uV;
		val += (min_uV - info->switch_uV) / info->step2_uV;
		mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	} else {
		val = (min_uV - info->min_uV + info->step1_uV - 1) / info->step1_uV;
		val <<= info->vol_shift;
		mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	}

	return axp_update(axp_dev, info->vol_reg, val, mask);
}

static int axp_get_voltage(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct device *axp_dev = to_axp_dev(rdev);
	uint8_t val, mask;
	int ret, switch_val, vol;

	ret = axp_read(axp_dev, info->vol_reg, &val);
	if (ret)
		return ret;

	mask = ((1 << info->vol_nbits) - 1)  << info->vol_shift;
	if (info->step1_uV != 0) {
		switch_val = ((info->switch_uV- info->min_uV + info->step1_uV - 1) / info->step1_uV);
	} else {
		switch_val = 0;
	}
	val = (val & mask) >> info->vol_shift;

	if ((info->switch_uV != 0) && (info->step2_uV!= 0) && (val > switch_val)) {
		val -= switch_val;
		vol = info->switch_uV + info->step2_uV * val;
	} else {
		vol = info->min_uV + info->step1_uV * val;
	}

	return vol;
}

static int axp_enable(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct device *axp_dev = to_axp_dev(rdev);

	return axp_set_bits(axp_dev, info->enable_reg,
					1 << info->enable_bit);
}

static int axp_disable(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct device *axp_dev = to_axp_dev(rdev);

	return axp_clr_bits(axp_dev, info->enable_reg,
					1 << info->enable_bit);
}

static int axp_is_enabled(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct device *axp_dev = to_axp_dev(rdev);
	uint8_t reg_val;
	int ret;

	ret = axp_read(axp_dev, info->enable_reg, &reg_val);
	if (ret)
		return ret;

	return !!(reg_val & (1 << info->enable_bit));
}

static int axp_list_voltage(struct regulator_dev *rdev, unsigned selector)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	int ret;

	ret = info->min_uV + info->step1_uV * selector;
	if ((info->switch_uV != 0) && (info->step2_uV != 0) && (ret > info->switch_uV)) {
		selector -= ((info->switch_uV-info->min_uV)/info->step1_uV);
		ret = info->switch_uV + info->step2_uV * selector;
	}
	if (ret > info->max_uV)
		return -EINVAL;
	return ret;
}

static int axp_set_suspend_voltage(struct regulator_dev *rdev, int uV)
{
	return axp_set_voltage(rdev, uV, uV,NULL);
#if 0
	int ldo = rdev_get_id(rdev);

	switch (ldo) {
	case AXP15_ID_LDO1 ... AXP15_LDO10:
		return axp_set_voltage(rdev, uV, uV);
	case AXP15_ID_DCDC1 ... AXP15_ID_DCDC5:
		return axp_set_voltage(rdev, uV, uV);
	default:
		return -EINVAL;
	}
#endif
}

static int axp_enable_time_regulator(struct regulator_dev *rdev)
{
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);

	/* Per-regulator power on delay from spec */
	switch (info->vol_reg) {
	case AXP15_LDO1: /* Fallthrough */
	case AXP15_LDO2: /* Fallthrough */
	case AXP15_LDO3: /* Fallthrough */
	case AXP15_LDO4: /* Fallthrough */
	case AXP15_LDO5: /* Fallthrough */
	case AXP15_LDO6:
	case AXP15_LDO7:
	case AXP15_LDO8:
	case AXP15_LDO9:
	case AXP15_LDO10:
		return 400;
	case AXP15_DCDC1:
	case AXP15_DCDC2:
	case AXP15_DCDC3:
	case AXP15_DCDC4:
	case AXP15_DCDC5:
		return 1200;
	default:
		break;
	}
	return 0;
}

static struct regulator_ops axp15_ops = {
	.set_voltage	= axp_set_voltage,
	.get_voltage	= axp_get_voltage,
	.list_voltage	= axp_list_voltage,
	.enable		= axp_enable,
	.disable	= axp_disable,
	.is_enabled	= axp_is_enabled,
	.enable_time	= axp_enable_time_regulator,
	.set_suspend_enable		= axp_enable,
	.set_suspend_disable	= axp_disable,
	.set_suspend_voltage	= axp_set_suspend_voltage,
};

/*static struct regulator_ops axp15_ldoio01_ops = {
	.set_voltage	= axp_set_voltage,
	.get_voltage	= axp_get_voltage,
	.list_voltage	= axp_list_voltage,
	.enable		= axp_ldoio01_enable,
	.disable	= axp_ldoio01_disable,
	.is_enabled	= axp_ldoio01_is_enabled,
	.set_suspend_enable		= axp_ldoio01_enable,
	.set_suspend_disable	= axp_ldoio01_disable,
	.set_suspend_voltage	= axp_set_suspend_voltage,
};*/


#define AXP15_LDO(_id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2)	\
	AXP_LDO(AXP15, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2)

#define AXP15_DCDC(_id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2)	\
	AXP_DCDC(AXP15, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2)

#define AXP15_SW(_id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2)	\
	AXP_SW(AXP15, _id, min, max, step1, vreg, shift, nbits, ereg, ebit, switch_vol, step2)

static struct axp_regulator_info axp_regulator_info[] = {
	AXP15_LDO(1,	700,	3300,	100,	LDO1,	0,	5,	LDO1EN,	5, 0, 0),	//ldo1 for aldo1
	AXP15_LDO(2,	700,	3300,	100,	LDO2,	0,	5,	LDO2EN,	6, 0, 0),	//ldo2 for aldo2 
	AXP15_LDO(3,	700,	3300,	100,	LDO3,	0,	5,	LDO3EN,	7, 0, 0),	//ldo3 for aldo3 
	AXP15_LDO(4,	700,	1900,	100,	LDO4,	0,	5,	LDO4EN,	0, 0, 0),	//ldo3 for bldo1 
	AXP15_LDO(5,	700,	1900,	100,	LDO5,	0,	5,	LDO5EN,	1, 0, 0),	//ldo5 for bldo2 
	AXP15_LDO(6,	700,	1900,	100,	LDO6,	0,	5,	LDO6EN,	2, 0, 0),	//ldo6 for bldo3 
	AXP15_LDO(7,	700,	1900,	100,	LDO7,	0,	5,	LDO7EN,	3, 0, 0),	//ldo7 for bldo4 
	AXP15_LDO(8,	700,	3300,	100,	LDO8,	0,	5,	LDO8EN,	4, 0, 0),	//ldo8 for cldo1 
	AXP15_LDO(9,	700,	4200,	100,	LDO9,	0,	5,	LDO9EN,	5, 3400, 200),	//ldo9 for cldo2
	AXP15_LDO(10,	700,	3300,	100,	LDO10,	0,	5,	LDO10EN,6, 0, 0),	//ldo10 for cldo3
	AXP15_DCDC(1,	600,	1520,	10,	DCDC1,	0,	7,	DCDC1EN,0, 1100, 20),
	AXP15_DCDC(2,	1000,	2550,	50,	DCDC2,	0,	5,	DCDC2EN,1, 0, 0),
	AXP15_DCDC(3,	600,	1520,	10,	DCDC3,	0,	7,	DCDC3EN,2, 1100, 20),
	AXP15_DCDC(4,	600,	3300,	20,	DCDC4,	0,	6,	DCDC4EN,3, 1500, 100),
	AXP15_DCDC(5,	1100,	3400,	100,	DCDC5,	0,	5,	DCDC5EN,4, 0, 0),
	AXP15_SW(0,	700,	3300,	100,	SW0,	0,	0,	SW0EN,7, 0, 0),//SW0
};

static ssize_t workmode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct regulator_dev *rdev = dev_get_drvdata(dev);
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct device *axp_dev = to_axp_dev(rdev);
	int ret;
	uint8_t val;
	
	ret = axp_read(axp_dev, AXP15_BUCKMODE, &val);
	if (ret)
		return sprintf(buf, "IO ERROR\n");

	if(info->desc.id == AXP15_ID_DCDC1){
		switch (val & 0x04) {
			case 0:return sprintf(buf, "AUTO\n");
			case 4:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	else if(info->desc.id == AXP15_ID_DCDC2){
		switch (val & 0x02) {
			case 0:return sprintf(buf, "AUTO\n");
			case 2:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	else if(info->desc.id == AXP15_ID_DCDC3){
		switch (val & 0x02) {
			case 0:return sprintf(buf, "AUTO\n");
			case 2:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	else if(info->desc.id == AXP15_ID_DCDC4){
		switch (val & 0x02) {
			case 0:return sprintf(buf, "AUTO\n");
			case 2:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	else if(info->desc.id == AXP15_ID_DCDC5){
		switch (val & 0x02) {
			case 0:return sprintf(buf, "AUTO\n");
			case 2:return sprintf(buf, "PWM\n");
			default:return sprintf(buf, "UNKNOWN\n");
		}
	}
	else
		return sprintf(buf, "IO ID ERROR\n");
}

static ssize_t workmode_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct regulator_dev *rdev = dev_get_drvdata(dev);
	struct axp_regulator_info *info = rdev_get_drvdata(rdev);
	struct device *axp_dev = to_axp_dev(rdev);
	char mode;
	uint8_t val;
	
	if(  buf[0] > '0' && buf[0] < '9' )// 1/AUTO: auto mode; 2/PWM: pwm mode;
		mode = buf[0];
	else
		mode = buf[1];

	switch(mode){
	 case 'U':
	 case 'u':
	 case '1':
		val = 0;break;
	 case 'W':
	 case 'w':
	 case '2':
	 	val = 1;break;
	 default:
	    val =0;
	}

	if(info->desc.id == AXP15_ID_DCDC1){
		if(val)
			axp_set_bits(axp_dev, AXP15_BUCKMODE,0x01);
		else
			axp_clr_bits(axp_dev, AXP15_BUCKMODE,0x01);
	}
	else if(info->desc.id == AXP15_ID_DCDC2){
		if(val)
			axp_set_bits(axp_dev, AXP15_BUCKMODE,0x02);
		else
			axp_clr_bits(axp_dev, AXP15_BUCKMODE,0x02);
	}
	else if(info->desc.id == AXP15_ID_DCDC3){
		if(val)
			axp_set_bits(axp_dev, AXP15_BUCKMODE,0x04);
		else
			axp_clr_bits(axp_dev, AXP15_BUCKMODE,0x04);
	}
	else if(info->desc.id == AXP15_ID_DCDC4){
		if(val)
			axp_set_bits(axp_dev, AXP15_BUCKMODE,0x08);
		else
			axp_clr_bits(axp_dev, AXP15_BUCKMODE,0x08);
	}
	else if(info->desc.id == AXP15_ID_DCDC5){
		if(val)
			axp_set_bits(axp_dev, AXP15_BUCKMODE,0x10);
		else
			axp_clr_bits(axp_dev, AXP15_BUCKMODE,0x10);
	}
	return count;
}

static ssize_t frequency_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct regulator_dev *rdev = dev_get_drvdata(dev);
	struct device *axp_dev = to_axp_dev(rdev);
	int ret;
	uint8_t val;
	
	ret = axp_read(axp_dev, AXP15_BUCKFREQ, &val);
	if (ret)
		return ret;
	ret = val & 0x0F;
	return sprintf(buf, "%d\n",(ret*75 + 750));
}

static ssize_t frequency_store(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct regulator_dev *rdev = dev_get_drvdata(dev);
	struct device *axp_dev = to_axp_dev(rdev);
	uint8_t val,tmp;
	int var;
	
	var = simple_strtoul(buf, NULL, 10);
	if(var < 750)
		var = 750;
	if(var > 1875)
		var = 1875;

	val = (var -750)/75;
	val &= 0x0F;

	axp_read(axp_dev, AXP15_BUCKFREQ, &tmp);
	tmp &= 0xF0;
	val |= tmp;
	axp_write(axp_dev, AXP15_BUCKFREQ, val);
	return count;
}


static struct device_attribute axp_regu_attrs[] = {
	AXP_REGU_ATTR(workmode),
	AXP_REGU_ATTR(frequency),
};

static int axp_regu_create_attrs(struct platform_device *pdev)
{
	int j,ret;
	
	for (j = 0; j < ARRAY_SIZE(axp_regu_attrs); j++) {
		ret = device_create_file(&pdev->dev,&axp_regu_attrs[j]);
		if (ret)
			goto sysfs_failed;
	}
    goto succeed;

sysfs_failed:
	while (j--)
		device_remove_file(&pdev->dev,&axp_regu_attrs[j]);
succeed:
	return ret;
}

static inline struct axp_regulator_info *find_regulator_info(int id)
{
	struct axp_regulator_info *ri;
	int i;

	for (i = 0; i < ARRAY_SIZE(axp_regulator_info); i++) {
		ri = &axp_regulator_info[i];
		if (ri->desc.id == id) {
			return ri;
		}
	}
	return NULL;
}

static int __devinit axp_regulator_probe(struct platform_device *pdev)
{
	struct axp_regulator_info *ri = NULL;
	struct regulator_dev *rdev;
	int ret;

	ri = find_regulator_info(pdev->id);
	if (ri == NULL) {
		dev_err(&pdev->dev, "invalid regulator ID specified\n");
		return -EINVAL;
	}

	if (ri->desc.id == AXP15_ID_LDO1 || ri->desc.id == AXP15_ID_LDO2 \
		|| ri->desc.id == AXP15_ID_LDO3 || ri->desc.id == AXP15_ID_LDO4 \
		|| ri->desc.id == AXP15_ID_LDO5 || ri->desc.id == AXP15_ID_LDO6 \
		|| ri->desc.id == AXP15_ID_LDO7 || ri->desc.id == AXP15_ID_LDO8 \
		|| ri->desc.id == AXP15_ID_LDO9 || ri->desc.id == AXP15_ID_LDO10 \
		|| ri->desc.id == AXP15_ID_DCDC1 || ri->desc.id == AXP15_ID_DCDC2 \
		|| ri->desc.id == AXP15_ID_DCDC3 || ri->desc.id == AXP15_ID_DCDC4 \
		|| ri->desc.id == AXP15_ID_DCDC5 || ri->desc.id == AXP15_ID_SW0)
		ri->desc.ops = &axp15_ops;

//	ri->desc.irq = 32;
	rdev = regulator_register(&ri->desc, &pdev->dev, pdev->dev.platform_data, ri, NULL);
//	ri->desc.irq = 32;
	if (IS_ERR(rdev)) {
		dev_err(&pdev->dev, "failed to register regulator %s\n",
				ri->desc.name);
		return PTR_ERR(rdev);
	}
	platform_set_drvdata(pdev, rdev);

	if(ri->desc.id == AXP15_ID_DCDC1 ||ri->desc.id == AXP15_ID_DCDC2 \
		|| ri->desc.id == AXP15_ID_DCDC3 ||ri->desc.id == AXP15_ID_DCDC4 \
		|| ri->desc.id == AXP15_ID_DCDC5){
		ret = axp_regu_create_attrs(pdev);
		if(ret)
			return ret;
	}

	return 0;
}

static int __devexit axp_regulator_remove(struct platform_device *pdev)
{
	struct regulator_dev *rdev = platform_get_drvdata(pdev);

	regulator_unregister(rdev);
	return 0;
}

static struct platform_driver axp_regulator_driver = {
	.driver	= {
		.name	= "axp15-regulator",
		.owner	= THIS_MODULE,
	},
	.probe		= axp_regulator_probe,
	.remove		= axp_regulator_remove,
};

static int __init axp_regulator_init(void)
{
	return platform_driver_register(&axp_regulator_driver);
}
subsys_initcall(axp_regulator_init);

static void __exit axp_regulator_exit(void)
{
	platform_driver_unregister(&axp_regulator_driver);
}
module_exit(axp_regulator_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("King Zhong");
MODULE_DESCRIPTION("Regulator Driver for X-powers AXP15 PMIC");
MODULE_ALIAS("platform:axp-regulator");

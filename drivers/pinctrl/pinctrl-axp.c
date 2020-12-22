/*
 * driver/pinctrl/pinctrl-axp.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd. 
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner axp pinctrl driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <mach/sys_config.h>
#include <mach/platform.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/slab.h>
#include <linux/mfd/core.h>
#include <linux/seq_file.h>
#include <linux/i2c.h>
#include "core.h"
#include "pinctrl-axp.h"

#define MODULE_NAME "axp-pinctrl"

static const struct axp_desc_pin axp_pins[] = {
	AXP_PIN_DESC(AXP_PINCTRL_GPIO0,
		     AXP_FUNCTION(0x0, "gpio_in"),
		     AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO1,
		     AXP_FUNCTION(0x0, "gpio_in"),
		     AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO2,
		     AXP_FUNCTION(0x0, "gpio_in"),
		     AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO3,
		     AXP_FUNCTION(0x0, "gpio_in"),
		     AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO4,
		     AXP_FUNCTION(0x0, "gpio_in"),
		     AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO5,
		     AXP_FUNCTION(0x0, "gpio_in"),
		     AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO6,
		     AXP_FUNCTION(0x0, "gpio_in"),
		     AXP_FUNCTION(0x1, "gpio_out")),
	AXP_PIN_DESC(AXP_PINCTRL_GPIO7,
		     AXP_FUNCTION(0x0, "gpio_in"),
		     AXP_FUNCTION(0x1, "gpio_out")),
};

#define AXP_NUM_GPIOS   ARRAY_SIZE(axp_pins)

static struct axp_pinctrl_desc axp_pinctrl_pins_desc = {
	.pins  = axp_pins,
	.npins = AXP_NUM_GPIOS,
};

#define AXP_NUM_GPIOS   ARRAY_SIZE(axp_pins)

#ifdef CONFIG_AW_AXP19
/* axp driver porting interface */
int axp_gpio_get_io(int gpio, int *io_state)
{
	uint8_t val;
	struct axp_dev *axp = NULL;

	axp = axp_dev_lookup(AXP19);
	if (NULL == axp) {
		printk("%s: axp data is null\n", __func__);
		return -ENXIO;
	}

	switch(gpio)
	{
		case 0: axp_read(axp->dev,AXP19_GPIO0_CFG,&val);val &= 0x07;
				if(val == 0x00)
					*io_state = 1;
				else if (val == 0x01)
					*io_state = 0;
				else
					return -EIO;
				break;
		case 1: axp_read(axp->dev,AXP19_GPIO1_CFG,&val);val &= 0x07;
				if(val < 0x00)
					*io_state = 1;
				else if (val == 0x01)
					*io_state = 0;
				else
					return -EIO;
				break;
		case 2: axp_read(axp->dev,AXP19_GPIO2_CFG,&val);val &= 0x07;
				if(val == 0x0)
					*io_state = 1;
				else if (val == 0x01)
					*io_state = 0;
				else
					return -EIO;
				break;
		case 3: axp_read(axp->dev,AXP19_GPIO34_CFG,&val);val &= 0x03;
				if(val == 0x1)
					*io_state = 1;
				else if(val == 0x2)
					*io_state = 0;
				else
					return -EIO;
				break;
		case 4: axp_read(axp->dev,AXP19_GPIO34_CFG,&val);val &= 0x0C;
				if(val == 0x4)
					*io_state = 1;
				else if(val == 0x8)
					*io_state = 0;
				else
					return -EIO;
				break;
		case 5: axp_read(axp->dev,AXP19_GPIO5_CFG,&val);val &= 0x40;
				*io_state = val >> 6;
				break;
		case 6: axp_read(axp->dev,AXP19_GPIO67_CFG1,&val);*io_state = (val & 0x40)?0:1;break;
		case 7: axp_read(axp->dev,AXP19_GPIO67_CFG1,&val);*io_state = (val & 0x04)?0:1;break;
		default:return -ENXIO;
	}

		return 0;
}

static int axp_gpio_get_data(int gpio)
{
	int io_state;
	uint8_t ret;
	uint8_t val;
	int value;
	struct axp_dev *axp = NULL;

	axp = axp_dev_lookup(AXP19);
	if (NULL == axp) {
		printk("%s: axp data is null\n", __func__);
		return -ENXIO;
	}
	ret = axp_gpio_get_io(gpio,&io_state);
	if(ret)
		return ret;

	if(io_state){
		switch(gpio)
		{
			case 0:ret = axp_read(axp->dev,AXP19_GPIO012_STATE,&val); value = val & 0x01;break;
			case 1:ret = axp_read(axp->dev,AXP19_GPIO012_STATE,&val); value = (val & 0x02)?1:0;break;
			case 2:ret = axp_read(axp->dev,AXP19_GPIO012_STATE,&val); value = (val & 0x04)?1:0;break;
			case 3:ret = axp_read(axp->dev,AXP19_GPIO34_STATE,&val); value =val & 0x01;break;
			case 4:ret = axp_read(axp->dev,AXP19_GPIO34_STATE,&val); value =(val & 0x02)?1:0;break;
			case 5:ret = axp_read(axp->dev,AXP19_GPIO5_STATE,&val); value =(val & 0x20)?1:0;break;
			case 6:ret = axp_read(axp->dev,AXP19_GPIO67_STATE,&val); value =(val & 0x20)?1:0;break;
			case 7:ret = axp_read(axp->dev,AXP19_GPIO67_STATE,&val); value =(val & 0x02)?1:0;break;
			default:return -ENXIO;
		}
	}
	else{
		switch(gpio)
		{
			case 0:ret = axp_read(axp->dev,AXP19_GPIO012_STATE,&val); value = (val & 0x10)?1:0;break;
			case 1:ret = axp_read(axp->dev,AXP19_GPIO012_STATE,&val); value = (val & 0x20)?1:0;break;
			case 2:ret = axp_read(axp->dev,AXP19_GPIO012_STATE,&val); value = (val & 0x40)?1:0;break;
			case 3:ret = axp_read(axp->dev,AXP19_GPIO34_STATE,&val); value = (val & 0x10)?1:0;break;
			case 4:ret = axp_read(axp->dev,AXP19_GPIO34_STATE,&val); value = (val & 0x20)?1:0;break;
			case 5:ret = axp_read(axp->dev,AXP19_GPIO5_STATE,&val); value = (val & 0x10);break;
			case 6:ret = axp_read(axp->dev,AXP19_GPIO67_STATE,&val); value =(val & 0x10)?1:0;break;
			case 7:ret = axp_read(axp->dev,AXP19_GPIO67_STATE,&val); value =(val & 0x01)?1:0;break;
			default:return -ENXIO;
		}
	}
	return value;
}

static int axp_gpio_set_data(int gpio, int value)
{
	struct axp_dev *axp = NULL;

	axp = axp_dev_lookup(AXP19);
	if (NULL == axp) {
		printk("%s: %d axp data is null\n", __func__, __LINE__);
		return -ENXIO;
	}

	if(value){//high
		switch(gpio) {
			case 0: return axp_set_bits(axp->dev,AXP19_GPIO012_STATE,0x01);
			case 1: return axp_set_bits(axp->dev,AXP19_GPIO012_STATE,0x02);
			case 2: return axp_set_bits(axp->dev,AXP19_GPIO012_STATE,0x04);
			case 3: return axp_set_bits(axp->dev,AXP19_GPIO34_STATE,0x01);
			case 4: return axp_set_bits(axp->dev,AXP19_GPIO34_STATE,0x02);
			case 5: return axp_set_bits(axp->dev,AXP19_GPIO5_STATE,0x20);
			case 6: return axp_set_bits(axp->dev,AXP19_GPIO67_STATE,0x20);
			case 7: return axp_set_bits(axp->dev,AXP19_GPIO67_STATE,0x02);
			default:break;
		}
	}else{//low
		switch(gpio){
			case 0: return axp_clr_bits(axp->dev,AXP19_GPIO012_STATE,0x01);
			case 1: return axp_clr_bits(axp->dev,AXP19_GPIO012_STATE,0x02);
			case 2: return axp_clr_bits(axp->dev,AXP19_GPIO012_STATE,0x04);
			case 3: return axp_clr_bits(axp->dev,AXP19_GPIO34_STATE,0x01);
			case 4: return axp_clr_bits(axp->dev,AXP19_GPIO34_STATE,0x02);
			case 5: return axp_clr_bits(axp->dev,AXP19_GPIO5_STATE,0x20);
			case 6: return axp_clr_bits(axp->dev,AXP19_GPIO67_STATE,0x20);
			case 7: return axp_clr_bits(axp->dev,AXP19_GPIO67_STATE,0x02);
			default:break;
		}
	}
	return -ENXIO;
}

static int axp_pmx_set(int gpio, int mux)
{
	struct axp_dev *axp = NULL;

	axp = axp_dev_lookup(AXP19);
	if (NULL == axp) {
		printk("%s: %d axp data is null\n", __func__, __LINE__);
		return -ENXIO;
	}
	if(mux == 1){//output
		switch(gpio){
			case 0: return axp_clr_bits(axp->dev,AXP19_GPIO0_CFG,0x07);
			case 1: return axp_clr_bits(axp->dev,AXP19_GPIO1_CFG,0x07);
			case 2: return axp_clr_bits(axp->dev,AXP19_GPIO2_CFG,0x07);
			case 3: axp_set_bits(axp->dev,AXP19_GPIO34_CFG,0x81);
					return axp_clr_bits(axp->dev,AXP19_GPIO34_CFG,0x02);
			case 4: axp_set_bits(axp->dev,AXP19_GPIO34_CFG,0x84);
					return axp_clr_bits(axp->dev,AXP19_GPIO34_CFG,0x08);
			case 5: axp_set_bits(axp->dev,AXP19_GPIO5_CFG,0x80);
					return axp_clr_bits(axp->dev,AXP19_GPIO5_CFG,0x40);
			case 6: axp_set_bits(axp->dev,AXP19_GPIO67_CFG0,0x01);
					return axp_clr_bits(axp->dev,AXP19_GPIO67_CFG1,0x40);
			case 7: axp_set_bits(axp->dev,AXP19_GPIO67_CFG0,0x01);
					return axp_clr_bits(axp->dev,AXP19_GPIO67_CFG1,0x04);
			default:return -ENXIO;
		}
	}
	else if(mux == 0){//input
		switch(gpio){
			case 0: axp_clr_bits(axp->dev,AXP19_GPIO0_CFG,0x06);
					return axp_set_bits(axp->dev,AXP19_GPIO0_CFG,0x01);
			case 1: axp_clr_bits(axp->dev,AXP19_GPIO1_CFG,0x06);
					return axp_set_bits(axp->dev,AXP19_GPIO1_CFG,0x01);
			case 2: axp_clr_bits(axp->dev,AXP19_GPIO2_CFG,0x06);
					return axp_set_bits(axp->dev,AXP19_GPIO2_CFG,0x01);
			case 3: axp_set_bits(axp->dev,AXP19_GPIO34_CFG,0x82);
					return axp_clr_bits(axp->dev,AXP19_GPIO34_CFG,0x01);
			case 4: axp_set_bits(axp->dev,AXP19_GPIO34_CFG,0x88);
					return axp_clr_bits(axp->dev,AXP19_GPIO34_CFG,0x04);
			case 5: axp_set_bits(axp->dev,AXP19_GPIO5_CFG,0xC0);
			case 6: axp_set_bits(axp->dev,AXP19_GPIO67_CFG0,0x01);
					return axp_set_bits(axp->dev,AXP19_GPIO67_CFG1,0x40);
			case 7: axp_set_bits(axp->dev,AXP19_GPIO67_CFG0,0x01);
					return axp_set_bits(axp->dev,AXP19_GPIO67_CFG1,0x04);
			default:return -ENXIO;
		}
	}
	return -ENXIO;
}
#else
/* axp driver porting interface */
static int axp_gpio_get_data(int gpio)
{
	uint8_t ret;
	struct axp_dev *axp = NULL;

#ifdef CONFIG_AW_AXP22
	axp = axp_dev_lookup(AXP22);
#elif defined(CONFIG_AW_AXP81X)
	axp = axp_dev_lookup(AXP81X);
#endif
	if (NULL == axp) {
		printk("%s: axp data is null\n", __func__);
		return -ENXIO;
	}
	switch(gpio){
		case 0:axp_read(axp->dev,AXP_GPIO01_STATE,&ret);ret &= 0x10;ret = ret>>4;break;
		case 1:axp_read(axp->dev,AXP_GPIO01_STATE,&ret);ret &= 0x20;ret = ret>>5;break;
		case 2:printk("This IO is not an input,no return value!");return -ENXIO;
		case 3:printk("This IO is not an input,no return value!");return -ENXIO;
		case 4:printk("This IO is not an input,no return value!");return -ENXIO;
		case 5:printk("This IO is not an input,no return value!");return -ENXIO;
		default:return -ENXIO;
	}
	return ret;
}

static int axp_gpio_set_data(int gpio, int value)
{
	struct axp_dev *axp = NULL;

	if ((0 <= gpio) && (4 >= gpio)) {
#ifdef CONFIG_AW_AXP22
		axp = axp_dev_lookup(AXP22);
#elif defined(CONFIG_AW_AXP81X)
		axp = axp_dev_lookup(AXP81X);
#endif
		if (NULL == axp) {
			printk("%s: %d axp data is null\n", __func__, __LINE__);
			return -ENXIO;
		}
	} else if ((5 <= gpio) && (6 >= gpio)) {
		axp = axp_dev_lookup(AXP15);
		if (NULL == axp) {
			printk("%s: %d axp data is null\n", __func__, __LINE__);
			return -ENXIO;
		}
	} else {
		return -ENXIO;
	}

	if(value){//high
		switch(gpio) {
			case 0: 
				return axp_update_sync(axp->dev,AXP_GPIO0_CFG,0x01,0x07);
			case 1: 
				return axp_update_sync(axp->dev,AXP_GPIO1_CFG,0x01,0x07);
			case 2: 
				return axp_set_bits(axp->dev,AXP_GPIO2_CFG,0x80);
			case 3: 
				axp_clr_bits(axp->dev,AXP_GPIO3_CFG,0x30);
				return axp_clr_bits(axp->dev,AXP_GPIO3_CFG, 0x08);
			case 4: 
				axp_clr_bits(axp->dev,AXP_GPIO4_CFG, 0x10);
				return axp_set_bits(axp->dev,AXP_GPIO4_STA,0x04);
			case 5:
				return axp_set_bits(axp->dev,AXP_GPIO5_CFG,0x03);
			case 6:
				return axp_set_bits(axp->dev,AXP_GPIO6_CFG,0x80);
			default:break;
		}
	}else{//low
		switch(gpio){
			case 0:
				return axp_update_sync(axp->dev,AXP_GPIO0_CFG,0x0,0x07);
			case 1: 
				return axp_update_sync(axp->dev,AXP_GPIO1_CFG,0x0,0x07);
			case 2: 
				return axp_clr_bits(axp->dev,AXP_GPIO2_CFG,0x80);
			case 3: 
				axp_set_bits(axp->dev,AXP_GPIO3_CFG,0x30);
				return axp_clr_bits(axp->dev,AXP_GPIO3_CFG, 0x08);
			case 4: 
				axp_clr_bits(axp->dev,AXP_GPIO4_CFG, 0x10);
				return axp_clr_bits(axp->dev,AXP_GPIO4_STA,0x04);
			case 5:
				return axp_update_sync(axp->dev,AXP_GPIO5_CFG,0x02,0x07);
			case 6:
				return axp_clr_bits(axp->dev,AXP_GPIO6_CFG,0x80);
			default:break;
		}
	}
	return -ENXIO;
}

static int axp_pmx_set(int gpio, int mux)
{
	struct axp_dev *axp = NULL;

#ifdef CONFIG_AW_AXP22
	axp = axp_dev_lookup(AXP22);
#elif defined(CONFIG_AW_AXP81X)
	axp = axp_dev_lookup(AXP81X);
#endif

	if (NULL == axp) {
		printk("%s: %d axp data is null\n", __func__, __LINE__);
		return -ENXIO;
	}
	if(mux == 1){//output
		switch(gpio){
			case 0: return axp_clr_bits_sync(axp->dev,AXP_GPIO0_CFG, 0x06);
			case 1: return axp_clr_bits_sync(axp->dev,AXP_GPIO1_CFG, 0x06);
			case 2: return 0;
			case 3: return axp_clr_bits(axp->dev,AXP_GPIO3_CFG, 0x08);
			case 4: return axp_clr_bits(axp->dev,AXP_GPIO4_CFG, 0x10);
			case 5: return 0;
			default:return -ENXIO;
		}
	}
	else if(mux == 0){//input
		switch(gpio){
			case 0: axp_clr_bits_sync(axp->dev,AXP_GPIO0_CFG,0x05);
					return axp_set_bits_sync(axp->dev,AXP_GPIO0_CFG,0x02);
			case 1: axp_clr_bits_sync(axp->dev,AXP_GPIO1_CFG,0x05);
					return axp_set_bits_sync(axp->dev,AXP_GPIO1_CFG,0x02);
			case 2:	printk("This IO can not config as an input!");
				return -EINVAL;
			case 3:	printk("This IO can not config as an input!");
				return -EINVAL;
			case 4:	printk("This IO can not config as an input!");
				return -EINVAL;	
			case 5:	printk("This IO can not config as an input!");
				return -EINVAL;
			default:return -ENXIO;
		}
	}
	return -ENXIO;
}

static int axp_pmx_get(int gpio)
{
	struct axp_dev *axp = NULL;
	uint8_t data;

#ifdef CONFIG_AW_AXP22
	axp = axp_dev_lookup(AXP22);
#elif defined(CONFIG_AW_AXP81X)
	axp = axp_dev_lookup(AXP81X);
#endif

	if (NULL == axp) {
		printk("%s: %d axp data is null\n", __func__, __LINE__);
		return -ENXIO;
	}

	switch(gpio){
	case 0:
		axp_read(axp->dev, AXP_GPIO0_CFG, &data);
		if (0 == (data & 0x06))
			return 1;
		else if (0x02 == (data & 0x07))
			return 0;
		else
			return -ENXIO;
	case 1:
		axp_read(axp->dev, AXP_GPIO1_CFG, &data);
		if (0 == (data & 0x06))
			return 1;
		else if (0x02 == (data & 0x07))
			return 0;
		else
			return -ENXIO;
	case 2:
		return 1;
	case 3:
		axp_read(axp->dev, AXP_GPIO3_CFG, &data);
		if (0 == (data & 0x08))
			return 1;
		else
			return 0;
	case 4:
		axp_read(axp->dev, AXP_GPIO4_CFG, &data);
		if (0 == (data & 0x10))
			return 1;
		else
			return 0;
	case 5:
		return 1;
	default:return -ENXIO;
	}
	return -ENXIO;
}
#endif

static int axp_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return pinctrl_request_gpio(chip->base + offset);
}

static void axp_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	pinctrl_free_gpio(chip->base + offset);
}

static int axp_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	return pinctrl_gpio_direction_input(chip->base + offset);
}

static int axp_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	//struct axp_pinctrl *pc = dev_get_drvdata(chip->dev);

	return axp_gpio_get_data(offset);
}

static void axp_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	//struct axp_pinctrl *pc = dev_get_drvdata(chip->dev);

	axp_gpio_set_data(offset, value);
}

static int axp_gpio_direction_output(struct gpio_chip *chip,
		unsigned offset, int value)
{
	axp_gpio_set(chip, offset, value);
	return pinctrl_gpio_direction_output(chip->base + offset);
}

static struct gpio_chip axp_gpio_chip = {
	.label            = MODULE_NAME,
	.owner            = THIS_MODULE,
	.request          = axp_gpio_request,
	.free             = axp_gpio_free,
	.direction_input  = axp_gpio_direction_input,
	.direction_output = axp_gpio_direction_output,
	.get              = axp_gpio_get,
	.set              = axp_gpio_set,
	.base             = AXP_PIN_BASE,
	.ngpio            = AXP_NUM_GPIOS,
	.can_sleep        = 0,
};

static struct axp_desc_function *
axp_pinctrl_desc_find_function_by_name(struct axp_pinctrl *pctl,
				       const char *pin_name,
				       const char *func_name)
{
	int i;
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct axp_desc_pin *pin = pctl->desc->pins + i;
		if (!strcmp(pin->pin.name, pin_name)) {
			struct axp_desc_function *func = pin->functions;
			while (func->name) {
				if (!strcmp(func->name, func_name)) {
					return func;
				}
				func++;
			}
		}
	}
	return NULL;
}

static int axp_pinctrl_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->ngroups;
}

static const char *axp_pinctrl_get_group_name(struct pinctrl_dev *pctldev,
                                              unsigned selector)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->groups[selector].name;
}

static int axp_pinctrl_get_group_pins(struct pinctrl_dev *pctldev,
                                      unsigned selector,
                                      const unsigned **pins,
                                      unsigned *num_pins)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	*pins = (unsigned *)&pctl->groups[selector].pin;
	*num_pins = 1;

	return 0;
}

static void axp_pinctrl_pin_dbg_show(struct pinctrl_dev *pctldev,
		                     struct seq_file *s,
		                     unsigned offset)
{
	
}

static struct pinctrl_ops axp_pinctrl_ops = {
	.get_groups_count = axp_pinctrl_get_groups_count,
	.get_group_name   = axp_pinctrl_get_group_name,
	.get_group_pins   = axp_pinctrl_get_group_pins,
	.pin_dbg_show     = axp_pinctrl_pin_dbg_show,
};

static int axp_pmx_get_functions_count(struct pinctrl_dev *pctldev)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->nfunctions;
}

static const char *axp_pmx_get_function_name(struct pinctrl_dev *pctldev,
		                             unsigned selector)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);

	return pctl->functions[selector].name;
}

static int axp_pmx_get_function_groups(struct pinctrl_dev *pctldev,
		                       unsigned function,
		                       const char * const **groups,
		                       unsigned * const num_groups)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	
	*groups     = pctl->functions[function].groups;
	*num_groups = pctl->functions[function].ngroups;
	
	return 0;
}

static int axp_pmx_enable(struct pinctrl_dev *pctldev,
		          unsigned function,
		          unsigned group)
{
	struct axp_pinctrl           *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct axp_pinctrl_group      *g  = pctl->groups + group;
	struct axp_pinctrl_function *func = pctl->functions + function;
	struct axp_desc_function *desc    = 
	axp_pinctrl_desc_find_function_by_name(pctl, g->name, func->name);
	if (!desc) {
		return -EINVAL;
	}
	
	axp_pmx_set(g->pin, desc->muxval);

	return 0;
}

static void axp_pmx_disable(struct pinctrl_dev *pctldev,
		          unsigned function,
		          unsigned group)
{
	return ;
}		         

static int axp_pmx_gpio_set_direction(struct pinctrl_dev *pctldev,
		                      struct pinctrl_gpio_range *range,
		                      unsigned offset,
		                      bool input)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	struct axp_desc_function *desc;
	char        pin_name[AXP_PIN_NAME_MAX_LEN];
	const char *func;
	int         ret;
	
	ret = sprintf(pin_name, "GPIO%d", offset);
	if (!ret) {
		goto error;
	}
	if (input) {
		func = "gpio_in";
	} else {
		func = "gpio_out";
	}
	desc = axp_pinctrl_desc_find_function_by_name(pctl, pin_name, func);
	if (!desc) {
		ret = -EINVAL;
		goto error;
	}
	axp_pmx_set(offset, desc->muxval);
	ret = 0;
error:
	return ret;
}

static struct pinmux_ops axp_pmx_ops = {
	.get_functions_count = axp_pmx_get_functions_count,
	.get_function_name   = axp_pmx_get_function_name,
	.get_function_groups = axp_pmx_get_function_groups,
	.enable              = axp_pmx_enable,
	.disable	     = axp_pmx_disable,
	.gpio_set_direction  = axp_pmx_gpio_set_direction,
};

static int axp_pinconf_get(struct pinctrl_dev *pctldev,
		           unsigned pin,
			   unsigned long *config)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	int                 data;
	
	switch (SUNXI_PINCFG_UNPACK_TYPE(*config)) {
	case SUNXI_PINCFG_TYPE_DAT:
		data = axp_gpio_get_data(pin);
		*config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, data);
               pr_debug("axp pconf get pin [%s] data [%d]\n",
		         pin_get_name(pctl->pctl_dev, pin), data); 
		break;
	case SUNXI_PINCFG_TYPE_FUNC:
		data = axp_pmx_get(pin);
		*config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC, data);
               pr_debug("axp pconf get pin [%s] funcs [%d]\n",
		         pin_get_name(pctl->pctl_dev, pin), data);
		break;
	default:
               pr_debug("invalid axp pconf type for get\n");
		return -EINVAL;      
	}
	return 0;
}

static int axp_pinconf_set(struct pinctrl_dev *pctldev,
		           unsigned pin,
			   unsigned long config)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	int                  data;
	int                  func;
	
	switch (SUNXI_PINCFG_UNPACK_TYPE(config)) {
	case SUNXI_PINCFG_TYPE_DAT:
		data = SUNXI_PINCFG_UNPACK_VALUE(config);
		axp_gpio_set_data(pin, data);
               pr_debug("axp pconf set pin [%s] data to [%d]\n",
		         pin_get_name(pctl->pctl_dev, pin), data);
		break;
	case SUNXI_PINCFG_TYPE_FUNC:
		func = SUNXI_PINCFG_UNPACK_VALUE(config);
		axp_pmx_set(pin, func);
               pr_debug("axp pconf set pin [%s] func to [%d]\n",
		         pin_get_name(pctl->pctl_dev, pin), func);
	       break;
	default:
               pr_debug("invalid axp pconf type for set\n");
		return -EINVAL;      
	}
	return 0;
}

static int axp_pinconf_group_get(struct pinctrl_dev *pctldev,
			         unsigned group, 
			         unsigned long *config)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	return axp_pinconf_get(pctldev, pctl->groups[group].pin, config);
}

static int axp_pinconf_group_set(struct pinctrl_dev *pctldev,
			         unsigned group, 
			         unsigned long config)
{
	struct axp_pinctrl *pctl = pinctrl_dev_get_drvdata(pctldev);
	return axp_pinconf_set(pctldev, pctl->groups[group].pin, config);
}

static struct pinconf_ops axp_pinconf_ops = {
	.pin_config_get       = axp_pinconf_get,
	.pin_config_set       = axp_pinconf_set,
	.pin_config_group_get = axp_pinconf_group_get,
	.pin_config_group_set = axp_pinconf_group_set,
};

static struct pinctrl_desc axp_pinctrl_desc = {
	.name    = MODULE_NAME,
	.pctlops = &axp_pinctrl_ops,
	.pmxops  = &axp_pmx_ops,
	.confops = &axp_pinconf_ops,
	.owner   = THIS_MODULE,
};

static int axp_pinctrl_add_function(struct axp_pinctrl *pctl, const char *name)
{
	struct axp_pinctrl_function *func = pctl->functions;
	while (func->name) {
		/* function already there */
		if (strcmp(func->name, name) == 0) {
			func->ngroups++;
			return -EEXIST;
		}
		func++;
	}
	func->name = name;
	func->ngroups = 1;

	pctl->nfunctions++;

	return 0;
}

static struct axp_pinctrl_function *
axp_pinctrl_find_function_by_name(struct axp_pinctrl *pctl, const char *name)
{
	int    i;
	struct axp_pinctrl_function *func = pctl->functions;
	for (i = 0; i < pctl->nfunctions; i++) {
		if (!func[i].name) {
			break;
		}
		if (!strcmp(func[i].name, name)) {
			return func + i;
		}
	}
	return NULL;
}


static int axp_pinctrl_build_state(struct platform_device *pdev)
{
	int    i;
	struct axp_pinctrl *pctl = platform_get_drvdata(pdev);
	
	pctl->ngroups = pctl->desc->npins;

	/* Allocate groups */
	pctl->groups = devm_kzalloc(&pdev->dev,
				    pctl->ngroups * sizeof(*pctl->groups),
				    GFP_KERNEL);
	if (!pctl->groups) {
		return -ENOMEM;
	}
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct axp_desc_pin *pin = pctl->desc->pins + i;
		struct axp_pinctrl_group *group = pctl->groups + i;

		group->name = pin->pin.name;
		group->pin = pin->pin.number;
	}
	/*
	 * We suppose that we won't have any more functions than pins,
	 * we'll reallocate that later anyway
	 */
	pctl->functions = devm_kzalloc(&pdev->dev,
				pctl->desc->npins * sizeof(*pctl->functions),
				GFP_KERNEL);
	if (!pctl->functions) {
		return -ENOMEM;
	}

	/* Count functions and their associated groups */
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct axp_desc_pin *pin = pctl->desc->pins + i;
		struct axp_desc_function *func = pin->functions;
		while (func->name) {
			axp_pinctrl_add_function(pctl, func->name);
			func++;
		}
	}
	pctl->functions = krealloc(pctl->functions,
				pctl->nfunctions * sizeof(*pctl->functions),
				GFP_KERNEL);

	for (i = 0; i < pctl->desc->npins; i++) {
		const struct axp_desc_pin *pin = pctl->desc->pins + i;
		struct axp_desc_function *func = pin->functions;

		while (func->name) {
			struct axp_pinctrl_function *func_item;
			const char **func_grp;

			func_item = axp_pinctrl_find_function_by_name(pctl, func->name);
			if (!func_item) {
				return -EINVAL;
			}

			if (!func_item->groups) {
				func_item->groups =
					devm_kzalloc(&pdev->dev,
						     func_item->ngroups * sizeof(*func_item->groups),
						     GFP_KERNEL);
				if (!func_item->groups)
					return -ENOMEM;
			}

			func_grp = func_item->groups;
			while (*func_grp)
				func_grp++;

			*func_grp = pin->pin.name;
			func++;
		}
	}

	return 0;
}

static int axp_pin_cfg_to_pin_map(struct platform_device *pdev,
                                  struct gpio_config *cfg, 
                                  struct pinctrl_map *map,
                                  char  *mainkey_name)
{
	int          num_configs;
	unsigned long *configs;
	unsigned int pin_number;
	const char   *pin_name;
	const char   *ctrl_name = dev_name(&pdev->dev);
	const char   *function_name;
	struct axp_pinctrl *pctl = platform_get_drvdata(pdev);
	
	/* convert axp pin config number to pinctrl number */
	if (!IS_AXP_PIN(cfg->gpio)) {
		/* invalid axp pin config, skip it */
		return 0;
	}
	/* find pin name by number */
	pin_number = cfg->gpio - AXP_PIN_BASE;
	pin_name = pin_get_name(pctl->pctl_dev, pin_number);
	if (!pin_name) {
		pr_warn("invalid pin config under axp platform\n");
		return 0;
	}
	/* mux pinctrl map */
	if (cfg->mul_sel == AXP_PIN_INPUT_FUNC) {
		/* pin mux : input */
		function_name = "gpio_in";
	} else if (cfg->mul_sel == AXP_PIN_OUTPUT_FUNC) {
		/* pin mux : output */
		function_name = "gpio_out";
	} else {
		/* invalid pin mux for axp pinctrl */
		pr_warn("invalid pin mux value for axp pinctrl\n");
		return 0;
	}
	map[0].dev_name      = mainkey_name;
	map[0].name          = PINCTRL_STATE_DEFAULT;
	map[0].type          = PIN_MAP_TYPE_MUX_GROUP;
	map[0].ctrl_dev_name = ctrl_name;
	map[0].data.mux.function = function_name;
	map[0].data.mux.group    = pin_name;
    	
    	/* configuration pinctrl map,
	 * suppose max pin config is 1.
	 * axp have only 1 type configurations: output-data/.
	 * yes I know the configs memory will binding to pinctrl always,
	 * if we binding it to pinctrl, we don't free it anywhere!
	 * the configs memory will always allocate for pinctrl.
	 * by sunny at 2013-4-28 15:52:16.
	 */
	num_configs = 0;
	configs = kzalloc(sizeof(unsigned int) * 1, GFP_KERNEL);
	if (!configs) {
		pr_err("allocate memory for axp pin config failed\n");
		return -ENOMEM;
	}
	if (cfg->data != GPIO_DATA_DEFAULT) {
		configs[0] = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_DAT, cfg->data);
		num_configs++;
	}
	if (num_configs == 0) {
		/* this pin have no configurations,
		 * use hardware default value.
		 * we have only one map.
		 */
		 kfree(configs);
		 return 1;
	}
	map[1].dev_name      = mainkey_name;
	map[1].name          = PINCTRL_STATE_DEFAULT;
	map[1].type          = PIN_MAP_TYPE_CONFIGS_GROUP;
	map[1].ctrl_dev_name = ctrl_name;
	map[1].data.configs.group_or_pin = pin_name;
	map[1].data.configs.configs      = configs;
	map[1].data.configs.num_configs  = num_configs;
	
	/* we have two maps: mux + configs */
    	return 2;
}

static int axp_pinctrl_parse_pin_cfg(struct platform_device *pdev)
{
	int             mainkey_count;
	int             mainkey_idx;
	
	/* get main key count */
	mainkey_count = script_get_main_key_count();
       pr_debug("mainkey total count : %d\n", mainkey_count);
	for (mainkey_idx = 0; mainkey_idx < mainkey_count; mainkey_idx++) {
		char           *mainkey_name;
		script_item_u  *pin_list;
		int             pin_count;
		int 		pin_index;
		int 		map_index;
		struct pinctrl_map *maps;
		
		/* get main key name by index */
		mainkey_name = script_get_main_key_name(mainkey_idx);
		if (!mainkey_name) {
			/* get mainkey name failed */
                       pr_debug("get mainkey [%s] name failed\n", mainkey_name);
			continue;
		}
		
		/* get main-key(device) pin configuration */
		pin_count = script_get_pio_list(mainkey_name, &pin_list);
               pr_debug("mainkey name : %s, pin count : %d\n", mainkey_name, pin_count);
		if (pin_count == 0) {
			/* the mainkey have no pin configuration */
			continue;
		}
		/* allocate pinctrl_map table,
		 * max map table size = pin count * 2 : 
		 * mux map and config map.
		 */
		maps = kzalloc(sizeof(*maps) * (pin_count * 2), GFP_KERNEL);
		if (!maps) {
			pr_err("allocate memory for sunxi pinctrl map table failed\n");
			return -ENOMEM;
		}
		map_index = 0;
		for (pin_index = 0; pin_index < pin_count; pin_index++) {
			/* convert struct sunxi_pin_cfg to struct pinctrl_map */
			map_index += axp_pin_cfg_to_pin_map(pdev,
					&(pin_list[pin_index].gpio),
					&(maps[map_index]),
			             	mainkey_name);
		}
		if (map_index) {
			/* register maps to pinctrl */
                       pr_debug("map mainkey [%s] to pinctrl, map number [%d]\n",
			        mainkey_name, map_index);
			pinctrl_register_mappings(maps, map_index);
		}
		/* free pinctrl_map table directly, 
		 * pinctrl subsytem will dup this map table
		 */
		kfree(maps);
	}
	return 0;
}

static int axp_pinctrl_probe(struct platform_device *pdev)
{
	struct device           *dev = &pdev->dev;
	struct axp_pinctrl      *pctl;
	struct pinctrl_pin_desc *pins;
	int    ret;
	int    i, rc;
	
	/* allocate and initialize axp-pinctrl */
	pctl = devm_kzalloc(dev, sizeof(*pctl), GFP_KERNEL);
	if (!pctl) {
		dev_err(dev, "allocate memory for axp-pinctrl structure failed\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, pctl);
	pctl->dev  = dev;
	pctl->desc = &axp_pinctrl_pins_desc;
	
	ret = axp_pinctrl_build_state(pdev);
	if (ret) {
		dev_err(&pdev->dev, "dt probe failed: %d\n", ret);
		return ret;
	}
	/* register axp pinctrl */
	pins = devm_kzalloc(&pdev->dev,
			    pctl->desc->npins * sizeof(*pins),
			    GFP_KERNEL);
	if (!pins) {
		return -ENOMEM;
	}
	for (i = 0; i < pctl->desc->npins; i++) {
		pins[i] = pctl->desc->pins[i].pin;
	}
	axp_pinctrl_desc.name = dev_name(&pdev->dev);
	axp_pinctrl_desc.owner = THIS_MODULE;
	axp_pinctrl_desc.pins = pins;
	axp_pinctrl_desc.npins = pctl->desc->npins;
	pctl->pctl_dev = pinctrl_register(&axp_pinctrl_desc, dev, pctl);
	if (!pctl->pctl_dev) {
		rc = gpiochip_remove(pctl->gpio_chip);
		if (rc < 0)
			return rc;
		return -EINVAL;
	}
	/* initialize axp-gpio-chip */
	pctl->gpio_chip      = &axp_gpio_chip;
	pctl->gpio_chip->dev = dev;
	ret = gpiochip_add(pctl->gpio_chip);
	if (ret) {
		dev_err(dev, "could not add GPIO chip\n");
		return ret;
	}
	for (i = 0; i < pctl->desc->npins; i++) {
		const struct axp_desc_pin *pin = pctl->desc->pins + i;

		ret = gpiochip_add_pin_range(pctl->gpio_chip, dev_name(&pdev->dev),
					     pin->pin.number,
					     pin->pin.number, 1);
		if (ret) {
			dev_err(dev, "could not add GPIO pin range\n");
			rc = gpiochip_remove(pctl->gpio_chip);
			if (rc < 0)
				return rc;
			return ret;
		}
	}
	
	axp_pinctrl_parse_pin_cfg(pdev);
	
	return 0;
}

static int axp_pinctrl_remove(struct platform_device *pdev)
{
	struct axp_pinctrl *pc = platform_get_drvdata(pdev);
	int rc;

	pinctrl_unregister(pc->pctl_dev);
	rc = gpiochip_remove(pc->gpio_chip);
	if (rc < 0)
		return rc;

	return 0;
}

static struct platform_driver axp_pinctrl_driver = {
	.probe = axp_pinctrl_probe,
	.remove = axp_pinctrl_remove,
	.driver = {
		.name = MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static struct platform_device axp_pinctrl_device = {
	.name = MODULE_NAME,
	.id = PLATFORM_DEVID_NONE, /* this is only one device for pinctrl driver */
};

static int __init axp_pinctrl_init(void)
{
	int ret;
	ret = platform_device_register(&axp_pinctrl_device);
	if (IS_ERR_VALUE(ret)) {
               pr_err("register axp platform device failed, errno %d\n", ret);
		return -EINVAL;
	}
	ret = platform_driver_register(&axp_pinctrl_driver);
	if (IS_ERR_VALUE(ret)) {
               pr_err("register axp platform driver failed, errno %d\n", ret);
		return -EINVAL;
	}
	return 0;
}
postcore_initcall(axp_pinctrl_init);

MODULE_AUTHOR("sunny");
MODULE_DESCRIPTION("allwinner axp pin control driver");
MODULE_LICENSE("GPLv2");

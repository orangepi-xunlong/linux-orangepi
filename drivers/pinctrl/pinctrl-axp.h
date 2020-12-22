/*
 * Allwinner AXP SoCs pinctrl driver.
 *
 * Copyright (C) 2012 sunny
 *
 * sunny <sunny@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PINCTRL_AXP_H
#define __PINCTRL_AXP_H

#include <linux/kernel.h>
#include <mach/gpio.h>

#define AXP_PINCTRL_GPIO0	PINCTRL_PIN(0, "GPIO0")     /* axp22x GPIO0 */
#define AXP_PINCTRL_GPIO1	PINCTRL_PIN(1, "GPIO1")     /* axp22x GPIO1 */
#define AXP_PINCTRL_GPIO2	PINCTRL_PIN(2, "GPIO2")     /* axp22x DC1SW : power supply for LCD   OUTPUT */
#define AXP_PINCTRL_GPIO3	PINCTRL_PIN(3, "GPIO3")     /* axp22x CHGLED : power supply for moto OUTPUT */
#define AXP_PINCTRL_GPIO4	PINCTRL_PIN(4, "GPIO4")     /* axp22x N_VBUSEN : OUTPUT */
#define AXP_PINCTRL_GPIO5	PINCTRL_PIN(5, "GPIO5")     /* axp15x WAKEUP PIN*/
#define AXP_PINCTRL_GPIO6	PINCTRL_PIN(6, "GPIO6")     /*  */
#define AXP_PINCTRL_GPIO7	PINCTRL_PIN(7, "GPIO7")     /*  */

/*
 * GPIO Registers.
 */

/*    AXP22   
* GPIO0<-->GPIO0/LDO(PIN 37)
* GPIO1<-->GPIO1/LDO(PIN 31)
* GPIO2<-->DC1SW(PIN 22)
* GPIO3<-->CHGLED/MOTODRV(PIN 52)
* GPIO4<-->N_VBUSEN(PIN 11)
*/
#ifdef CONFIG_AW_AXP22
#define AXP_GPIO0_CFG                   (AXP22_GPIO0_CTL)//0x90
#define AXP_GPIO1_CFG                   (AXP22_GPIO1_CTL)//0x92
#define AXP_GPIO2_CFG                   (AXP22_LDO_DC_EN2)//0x12
#define AXP_GPIO3_CFG                   (AXP22_OFF_CTL)//0x32
#define AXP_GPIO4_CFG                   (AXP22_HOTOVER_CTL)//0x8f
#define AXP_GPIO4_STA                   (AXP22_IPS_SET)//0x30
#define AXP_GPIO01_STATE                (AXP22_GPIO01_SIGNAL)
#elif defined(CONFIG_AW_AXP81X)
#define AXP_GPIO0_CFG                   (AXP81X_GPIO0_CTL)//0x90
#define AXP_GPIO1_CFG                   (AXP81X_GPIO1_CTL)//0x92
#define AXP_GPIO2_CFG                   (AXP81X_LDO_DC_EN2)//0x12
#define AXP_GPIO3_CFG                   (AXP81X_OFF_CTL)//0x32
#define AXP_GPIO4_CFG                   (AXP81X_HOTOVER_CTL)//0x8f
#define AXP_GPIO4_STA                   (AXP81X_IPS_SET)//0x30
#define AXP_GPIO01_STATE                (AXP81X_GPIO01_SIGNAL)
#elif defined(CONFIG_AW_AXP19)
#define AXP19_GPIO0_CFG                   (POWER19_GPIO0_CTL)
#define AXP19_GPIO1_CFG                   (POWER19_GPIO1_CTL)
#define AXP19_GPIO2_CFG                   (POWER19_GPIO2_CTL)
#define AXP19_GPIO34_CFG                  (POWER19_SENSE_CTL)
#define AXP19_GPIO5_CFG                   (POWER19_RSTO_CTL)
#define AXP19_GPIO67_CFG0                 (POWER19_GPIO67_CFG)
#define AXP19_GPIO67_CFG1                 (POWER19_GPIO67_CTL)

#define AXP19_GPIO012_STATE               (POWER19_GPIO012_SIGNAL)
#define AXP19_GPIO34_STATE                (POWER19_SENSE_SIGNAL)
#define AXP19_GPIO5_STATE                 (POWER19_RSTO_CTL)
#define AXP19_GPIO67_STATE                (POWER19_GPIO67_CTL)
#endif
/*    AXP15
* GPIO5<-->GPIO/Wakeup(PIN 14)
* GPIO6<-->SWOUT(PIN 22)
*/
#ifdef CONFIG_AW_AXP15
#define AXP_GPIO5_CFG                   (AXP15_WAKEUP_FUNC)//0x35
#define AXP_GPIO6_CFG                   (AXP15_BC_LDO_EN)//0x11
#endif

#ifndef AXP_GPIO0_CFG
#define AXP_GPIO0_CFG                   (0)
#define AXP_GPIO1_CFG                   (0)
#define AXP_GPIO2_CFG                   (0)
#define AXP_GPIO3_CFG                   (0)
#define AXP_GPIO4_CFG                   (0)
#define AXP_GPIO4_STA                   (0)
#define AXP_GPIO01_STATE                (0)
#endif
#ifndef AXP_GPIO5_CFG
#define AXP_GPIO5_CFG                   (0)//0x35
#define AXP_GPIO6_CFG                   (0)//0x11
#endif

#define AXP_PIN_NAME_MAX_LEN	8

struct axp_desc_function {
	const char *name;
	u8         muxval;
};

struct axp_desc_pin {
	struct pinctrl_pin_desc	 pin;
	struct axp_desc_function *functions;
};

struct axp_pinctrl_desc {
	const struct axp_desc_pin *pins;
	int			  npins;
};

struct axp_pinctrl_function {
	const char	*name;
	const char	**groups;
	unsigned	ngroups;
};

struct axp_pinctrl_group {
	const char	*name;
	unsigned long	config;
	unsigned	pin;
};

struct axp_pinctrl {
	struct device                *dev;
	struct pinctrl_dev           *pctl_dev;
	struct gpio_chip             *gpio_chip;
	struct axp_pinctrl_desc	     *desc;
	struct axp_pinctrl_function  *functions;
	unsigned                     nfunctions;
	struct axp_pinctrl_group     *groups;
	unsigned                     ngroups;
};

#define AXP_PIN_DESC(_pin, ...)					\
	{							\
		.pin = _pin,					\
		.functions = (struct axp_desc_function[]){	\
			__VA_ARGS__, { } },			\
	}

#define AXP_FUNCTION(_val, _name)				\
	{							\
		.name = _name,					\
		.muxval = _val,					\
	}

#endif /* __PINCTRL_AXP_H */

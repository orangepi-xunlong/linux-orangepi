/*
 *
 * $Id: stk3x1x.h
 *
 * Copyright (C) 2012~2013 Lex Hsieh     <lex_hsieh@sensortek.com.tw>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive for
 * more details.
 *
 */
#ifndef __STK3X1X_H__
#define __STK3X1X_H__

#define STK_GPIO_PULL_DEFAULT   ((u32)-1)
#define STK_GPIO_DRVLVL_DEFAULT ((u32)-1)
#define STK_GPIO_DATA_DEFAULT   ((u32)-1)

#include <linux/pinctrl/pinconf-generic.h>
/*
 * PIN_CONFIG_PARAM_MAX - max available value of 'enum pin_config_param'.
 * see include/linux/pinctrl/pinconf-generic.h
 */
#define PIN_CONFIG_PARAM_MAX    PIN_CONFIG_PERSIST_STATE
enum stk_pin_config_param {
	STK_PINCFG_TYPE_FUNC = PIN_CONFIG_PARAM_MAX + 1,
	STK_PINCFG_TYPE_DAT,
	STK_PINCFG_TYPE_PUD,
	STK_PINCFG_TYPE_DRV,
};


/* platform data */

struct stk3x1x_platform_data {
	uint8_t state_reg;
	uint8_t psctrl_reg;
	uint8_t alsctrl_reg;
	uint8_t ledctrl_reg;
	uint8_t	wait_reg;
	uint16_t ps_thd_h;
	uint16_t ps_thd_l;
	int int_pin;
	uint32_t transmittance;
};

#endif /*__STK3X1X_H__*/

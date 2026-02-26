/* SPDX-License-Identifier: GPL-2.0 */
/*
 *
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __VIN__LOG__H__
#define __VIN__LOG__H__

#define SUNXI_MODNAME		"vin"
#include <sunxi-log.h>

#if IS_ENABLED(CONFIG_CCI_MODULE) || IS_ENABLED(CONFIG_CCI)
#include "../vin-cci/bsp_cci.h"
#define USE_SPECIFIC_CCI
#endif

#define USE_SUNXI_LOG		1

#define VIN_LOG_MD				(1 << 0)	/* 0x1 */
#define VIN_LOG_FLASH				(1 << 1)	/* 0x2 */
#define VIN_LOG_CCI				(1 << 2)	/* 0x4 */
#define VIN_LOG_CSI				(1 << 3)	/* 0x8 */
#define VIN_LOG_MIPI				(1 << 4)	/* 0x10 */
#define VIN_LOG_ISP				(1 << 5)	/* 0x20 */
#define VIN_LOG_STAT				(1 << 6)	/* 0x40 */
#define VIN_LOG_SCALER				(1 << 7)	/* 0x80 */
#define VIN_LOG_POWER				(1 << 8)	/* 0x100 */
#define VIN_LOG_CONFIG				(1 << 9)	/* 0x200 */
#define VIN_LOG_VIDEO				(1 << 10)	/* 0x400 */
#define VIN_LOG_FMT				(1 << 11)	/* 0x800 */
#define VIN_LOG_TDM				(1 << 12)	/* 0x1000 */
#define VIN_LOG_RP				(1 << 13)	/* 0x2000 */
#define VIN_LOG_LARGE				(1 << 14)	/*0x4000*/

extern unsigned int vin_log_mask;
#if USE_SUNXI_LOG
#if IS_ENABLED(CONFIG_VIN_LOG)
#define vin_log(flag, arg...) do { \
	if (flag & vin_log_mask) { \
		switch (flag) { \
		case VIN_LOG_MD: \
			sunxi_debug(NULL, "[VIN_LOG_MD]" arg); \
			break; \
		case VIN_LOG_FLASH: \
			sunxi_debug(NULL, "[VIN_LOG_FLASH]" arg); \
			break; \
		case VIN_LOG_CCI: \
			sunxi_debug(NULL, "[VIN_LOG_CCI]" arg); \
			break; \
		case VIN_LOG_CSI: \
			sunxi_debug(NULL, "[VIN_LOG_CSI]" arg); \
			break; \
		case VIN_LOG_MIPI: \
			sunxi_debug(NULL, "[VIN_LOG_MIPI]" arg); \
			break; \
		case VIN_LOG_ISP: \
			sunxi_debug(NULL, "[VIN_LOG_ISP]" arg); \
			break; \
		case VIN_LOG_STAT: \
			sunxi_debug(NULL, "[VIN_LOG_STAT]" arg); \
			break; \
		case VIN_LOG_SCALER: \
			sunxi_debug(NULL, "[VIN_LOG_SCALER]" arg); \
			break; \
		case VIN_LOG_POWER: \
			sunxi_debug(NULL, "[VIN_LOG_POWER]" arg); \
			break; \
		case VIN_LOG_CONFIG: \
			sunxi_debug(NULL, "[VIN_LOG_CONFIG]" arg); \
			break; \
		case VIN_LOG_VIDEO: \
			sunxi_debug(NULL, "[VIN_LOG_VIDEO]" arg); \
			break; \
		case VIN_LOG_FMT: \
			sunxi_debug(NULL, "[VIN_LOG_FMT]" arg); \
			break; \
		case VIN_LOG_TDM: \
			sunxi_debug(NULL, "[VIN_LOG_TDM]" arg); \
			break; \
		case VIN_LOG_RP: \
			sunxi_debug(NULL, "[VIN_LOG_RP]" arg); \
			break; \
		case VIN_LOG_LARGE: \
			sunxi_debug(NULL, "[VIN_LOG_LARGE]" arg); \
			break; \
		default: \
			sunxi_debug(NULL, "[VIN_LOG]" arg); \
			break; \
		} \
	} \
} while (0)
#else
#define vin_log(flag, arg...) do { } while (0)
#endif
#define vin_err(x, arg...) sunxi_err(NULL, x, ##arg)
#define vin_warn(x, arg...) sunxi_warn(NULL, x, ##arg)
#define vin_print(x, arg...) sunxi_info(NULL, x, ##arg)

#define DEV_DBG_EN   1
#if (DEV_DBG_EN == 1)
#define sensor_dbg(x, arg...) sunxi_debug(NULL, "[%s]"x, SENSOR_NAME, ##arg)
#else
#define sensor_dbg(x, arg...)
#endif
#define sensor_err(x, arg...) sunxi_err(NULL,  "[%s]"x, SENSOR_NAME, ##arg)
#define sensor_print(x, arg...) sunxi_info(NULL, "[%s]"x, SENSOR_NAME, ##arg)


#define ACT_DEV_DBG_EN 0
#define act_err(x, arg...) sunxi_err(NULL, "[%s]"x, SUNXI_ACT_NAME, ##arg)
#if ACT_DEV_DBG_EN
#define act_dbg(x, arg...) sunxi_debug(NULL, "[%s]"x, SUNXI_ACT_NAME, ##arg)
#else
#define act_dbg(x, arg...)
#endif

#define SENSOR_POWER_DBG_EN   0
#if (SENSOR_POWER_DBG_EN == 1)
#define sensor_power_dbg(x, arg...) sunxi_debug(NULL, "[sensor_power_debug]" x, ## arg)
#else
#define sensor_power_dbg(x, arg...)
#endif
#define sensor_power_err(x, arg...) sunxi_err(NULL, "[sensor_power_err]" x, ## arg)
#define sensor_power_warn(x, arg...) sunxi_warn(NULL, "[sensor_power_warn] "x, ##arg)
#define sensor_power_print(x, arg...) sunxi_info(NULL, "[sensor_power]" x, ## arg)

#ifdef USE_SPECIFIC_CCI
#define cci_print(x, arg...) sunxi_info(NULL, "[VIN_DEV_CCI]"x, ##arg)
#define cci_err(x, arg...) sunxi_err(NULL, "[VIN_DEV_CCI_ERR]"x, ##arg)
#else
#define cci_print(x, arg...) sunxi_info(NULL, "[VIN_DEV_I2C]"x, ##arg)
#define cci_err(x, arg...) sunxi_err(NULL, "[VIN_DEV_I2C_ERR]"x, ##arg)
#endif

#else /* NOT_USE_SUNXI_LOG */

#define vin_log(flag, arg...) do { \
	if (flag & vin_log_mask) { \
		switch (flag) { \
		case VIN_LOG_MD: \
			printk(KERN_DEBUG "[VIN_LOG_MD]" arg); \
			break; \
		case VIN_LOG_FLASH: \
			printk(KERN_DEBUG "[VIN_LOG_FLASH]" arg); \
			break; \
		case VIN_LOG_CCI: \
			printk(KERN_DEBUG "[VIN_LOG_CCI]" arg); \
			break; \
		case VIN_LOG_CSI: \
			printk(KERN_DEBUG "[VIN_LOG_CSI]" arg); \
			break; \
		case VIN_LOG_MIPI: \
			printk(KERN_DEBUG "[VIN_LOG_MIPI]" arg); \
			break; \
		case VIN_LOG_ISP: \
			printk(KERN_DEBUG "[VIN_LOG_ISP]" arg); \
			break; \
		case VIN_LOG_STAT: \
			printk(KERN_DEBUG "[VIN_LOG_STAT]" arg); \
			break; \
		case VIN_LOG_SCALER: \
			printk(KERN_DEBUG "[VIN_LOG_SCALER]" arg); \
			break; \
		case VIN_LOG_POWER: \
			printk(KERN_DEBUG "[VIN_LOG_POWER]" arg); \
			break; \
		case VIN_LOG_CONFIG: \
			printk(KERN_DEBUG "[VIN_LOG_CONFIG]" arg); \
			break; \
		case VIN_LOG_VIDEO: \
			printk(KERN_DEBUG "[VIN_LOG_VIDEO]" arg); \
			break; \
		case VIN_LOG_FMT: \
			printk(KERN_DEBUG "[VIN_LOG_FMT]" arg); \
			break; \
		case VIN_LOG_TDM: \
			printk(KERN_DEBUG "[VIN_LOG_TDM]" arg); \
			break; \
		case VIN_LOG_RP: \
			printk(KERN_DEBUG "[VIN_LOG_RP]" arg); \
			break; \
		case VIN_LOG_LARGE: \
			printk(KERN_DEBUG "[VIN_LOG_LARGE]" arg); \
			break; \
		default: \
			printk(KERN_DEBUG "[VIN_LOG]" arg); \
			break; \
		} \
	} \
} while (0)
#define vin_err(x, arg...) sunxi_err(NULL, x, ##arg)
#define vin_warn(x, arg...) sunxi_warn(NULL, x, ##arg)
#define vin_print(x, arg...) sunxi_info(NULL, x, ##arg)

#define DEV_DBG_EN   0
#if (DEV_DBG_EN == 1)
#define sensor_dbg(x, arg...) printk(KERN_DEBUG "[%s]"x, SENSOR_NAME, ##arg)
#else
#define sensor_dbg(x, arg...)
#endif
#define sensor_err(x, arg...) pr_err("[%s] error, "x, SENSOR_NAME, ##arg)
#define sensor_print(x, arg...) pr_info("[%s]"x, SENSOR_NAME, ##arg)

#define ACT_DEV_DBG_EN 0
#define act_err(x, arg...) pr_err("[%s] error, "x, SUNXI_ACT_NAME, ##arg)
#if ACT_DEV_DBG_EN
#define act_dbg(x, arg...) printk(KERN_DEBUG"[%s]"x, SUNXI_ACT_NAME, ##arg)
#else
#define act_dbg(x, arg...)
#endif

#define SENSOR_POWER_DBG_EN   0
#if (SENSOR_POWER_DBG_EN == 1)
#define sensor_power_dbg(x, arg...) printk(KERN_DEBUG "[sensor_power_debug]" x, ## arg)
#else
#define sensor_power_dbg(x, arg...)
#endif
#define sensor_power_err(x, arg...) pr_err("[sensor_power_err]" x, ## arg)
#define sensor_power_warn(x, arg...) pr_warn("[sensor_power_warn] "x, ##arg)
#define sensor_power_print(x, arg...) pr_info("[sensor_power]" x, ## arg)

#ifdef USE_SPECIFIC_CCI
#define cci_print(x, arg...) pr_info("[VIN_DEV_CCI]"x, ##arg)
#define cci_err(x, arg...) pr_err("[VIN_DEV_CCI_ERR]"x, ##arg)
#else
#define cci_print(x, arg...) pr_info("[VIN_DEV_I2C]"x, ##arg)
#define cci_err(x, arg...) pr_err("[VIN_DEV_I2C_ERR]"x, ##arg)
#endif

#endif

#endif	/* __VIN__LOG__H__ */

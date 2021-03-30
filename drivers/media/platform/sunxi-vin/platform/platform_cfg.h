
/*
 ******************************************************************************
 *
 * platform_cfg.h
 *
 * Hawkview ISP - platform_cfg.h module
 *
 * Copyright (c) 2014 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Version		  Author         Date		    Description
 *
 *   2.0		  Yang Feng   	2014/07/24	      Second Version
 *
 ******************************************************************************
 */

#ifndef __PLATFORM_CFG__H__
#define __PLATFORM_CFG__H__

/*#define FPGA_VER*/
/*#define SUNXI_MEM*/

#ifdef FPGA_VER
#define FPGA_PIN
#else
#define VIN_CLK
#define VIN_GPIO
#define VIN_PMU
#endif

#include <linux/gpio.h>

#ifdef VIN_CLK
#include <linux/clk.h>
#include <linux/clk/sunxi.h>
#include <linux/clk-private.h>
#endif

#ifdef VIN_GPIO
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#endif

#ifdef VIN_PMU
#include <linux/regulator/consumer.h>
#endif

#include <linux/sys_config.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>

#include <linux/slab.h>

#include "../utility/vin_os.h"

#ifdef FPGA_VER
#define DPHY_CLK (48*1000*1000)
#else
#define DPHY_CLK (150*1000*1000)
#endif

#if defined CONFIG_ARCH_SUN50I
#include "sun50iw1p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_SUN50IW1P1
#elif defined CONFIG_ARCH_SUN8IW10P1
#include "sun8iw10p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_NUM
#elif defined CONFIG_ARCH_SUN8IW11P1
#include "sun8iw11p1_vfe_cfg.h"
#define SUNXI_PLATFORM_ID ISP_PLATFORM_NUM
#endif

/*
 * The subdevices' group IDs.
 */
#define VIN_GRP_ID_SENSOR	(1 << 8)
#define VIN_GRP_ID_MIPI		(1 << 9)
#define VIN_GRP_ID_CSI		(1 << 10)
#define VIN_GRP_ID_ISP		(1 << 11)
#define VIN_GRP_ID_SCALER	(1 << 12)
#define VIN_GRP_ID_CAPTURE	(1 << 13)
#define VIN_GRP_ID_STAT		(1 << 14)


#define VIN_ALIGN_WIDTH 16
#define VIN_ALIGN_HEIGHT 16

#define ISP_LUT_MEM_OFS		0x0
#define ISP_LENS_MEM_OFS	(ISP_LUT_MEM_OFS + ISP_LUT_MEM_SIZE)
#define ISP_GAMMA_MEM_OFS	(ISP_LENS_MEM_OFS + ISP_LENS_MEM_SIZE)
#define ISP_LINEAR_MEM_OFS	(ISP_GAMMA_MEM_OFS + ISP_GAMMA_MEM_SIZE)

#define ISP_DRC_MEM_OFS		0x0
#define ISP_DISC_MEM_OFS	(ISP_DRC_MEM_OFS + ISP_DRC_MEM_SIZE)

#endif /*__PLATFORM_CFG__H__*/

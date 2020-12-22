
/*
 ***************************************************************************************
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
 ****************************************************************************************
 */

#ifndef __PLATFORM_CFG__H__
#define __PLATFORM_CFG__H__

#if defined CONFIG_FPGA_V4_PLATFORM
#define FPGA_VER
#elif defined CONFIG_FPGA_V7_PLATFORM
#define FPGA_VER
#endif

#define VFE_SYS_CONFIG
//#define SUNXI_MEM

#ifdef FPGA_VER
#define FPGA_PIN
#else
#define VFE_CLK
#define VFE_GPIO
#define VFE_PMU
#endif

#include <mach/irqs.h>
#include <mach/platform.h>
#include <linux/gpio.h>

#ifdef VFE_CLK
//#include <mach/clock.h>
#include <linux/clk.h>
#include <linux/clk/sunxi_name.h>
#endif

#ifdef VFE_GPIO
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <mach/gpio.h>
#endif

#ifdef VFE_PMU
#include <linux/regulator/consumer.h>
#endif

#ifdef VFE_SYS_CONFIG
#include <mach/sys_config.h>
#endif
#include <mach/sunxi-chip.h>

#ifdef FPGA_VER
#define DPHY_CLK (48*1000*1000)
#else
#define DPHY_CLK (150*1000*1000)
#endif

#if defined CONFIG_ARCH_SUN8IW1P1

#include "platform/sun8iw1p1_vfe_cfg.h"

#elif defined CONFIG_ARCH_SUN8IW3P1

#include "platform/sun8iw3p1_vfe_cfg.h"

#elif defined CONFIG_ARCH_SUN8IW5P1

#include "platform/sun8iw5p1_vfe_cfg.h"

#elif defined CONFIG_ARCH_SUN8IW6P1

#include "platform/sun8iw6p1_vfe_cfg.h"

#elif defined CONFIG_ARCH_SUN8IW7P1

#include "platform/sun8iw7p1_vfe_cfg.h"

#elif defined CONFIG_ARCH_SUN8IW8P1

#include "platform/sun8iw8p1_vfe_cfg.h"

#elif defined CONFIG_ARCH_SUN8IW9P1

#include "platform/sun8iw9p1_vfe_cfg.h"

#elif defined CONFIG_ARCH_SUN9IW1P1

#include "platform/sun9iw1p1_vfe_cfg.h"

#endif

#define ISP_LUT_MEM_OFS             0x0
#define ISP_LENS_MEM_OFS            (ISP_LUT_MEM_OFS + ISP_LUT_MEM_SIZE)
#define ISP_GAMMA_MEM_OFS           (ISP_LENS_MEM_OFS + ISP_LENS_MEM_SIZE)
#define ISP_LINEAR_MEM_OFS           (ISP_GAMMA_MEM_OFS + ISP_GAMMA_MEM_SIZE)

#define ISP_DRC_MEM_OFS            0x0
#define ISP_DISC_MEM_OFS          (ISP_DRC_MEM_OFS + ISP_DRC_MEM_SIZE)


#define VFE_CORE_CLK_RATE (500*1000*1000)
#define VFE_CLK_NOT_EXIST	NULL

#endif //__PLATFORM_CFG__H__

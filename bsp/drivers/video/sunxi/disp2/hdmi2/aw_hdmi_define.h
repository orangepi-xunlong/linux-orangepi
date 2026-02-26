/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
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

#ifndef _AW_CONFIG_H_
#define _AW_CONFIG_H_

#include <sunxi-log.h>
#include <asm/div64.h>
#include <linux/delay.h>
#include "aw_hdmi_log.h"

/* #define __FPGA_PLAT__ */

/* define only use hdmi14 */
#if (IS_ENABLED(CONFIG_ARCH_SUN8IW16))
#define SUPPORT_ONLY_HDMI14
#endif

/* define use color space convert */
#if (IS_ENABLED(CONFIG_ARCH_SUN8IW16) || \
     IS_ENABLED(CONFIG_ARCH_SUN55IW3) || \
     IS_ENABLED(CONFIG_ARCH_SUN60IW2))
#define USE_CSC
#endif

/* define use tcon pad */
#if (IS_ENABLED(CONFIG_ARCH_SUN8IW16) || \
		IS_ENABLED(CONFIG_ARCH_SUN8IW20) || \
		IS_ENABLED(CONFIG_ARCH_SUN20IW1) || \
		IS_ENABLED(CONFIG_ARCH_SUN50IW9))
#define TCON_PAN_SEL
#endif

/* define use hdmi phy model */
#if (IS_ENABLED(CONFIG_AW_AWPHY))
	#ifndef SUNXI_HDMI20_PHY_AW
	#define SUNXI_HDMI20_PHY_AW        /* allwinner phy */
	#endif
#elif (IS_ENABLED(CONFIG_AW_INNOPHY))
	#ifndef SUNXI_HDMI20_PHY_INNO
	#define SUNXI_HDMI20_PHY_INNO      /* innosilicon phy */
	#endif
#elif IS_ENABLED(CONFIG_AW_INNOPHY_FPGA)
	#ifndef SUNXI_HDMI20_PHY_INNO_FPGA
	#define SUNXI_HDMI20_PHY_INNO_FPGA  /* innosilicon phy for fpga */
	#endif
#elif IS_ENABLED(CONFIG_AW_SYNOPSYSPHY)
	#ifndef SUNXI_HDMI20_PHY_SNPS
	#define SUNXI_HDMI20_PHY_SNPS      /* synopsys phy */
	#endif
#endif

/**
 * @desc: allwinner hdmi log
 * @demo: hdmi20: xxx
*/
#ifdef hdmi_inf
#undef hdmi_inf
#endif
#define hdmi_inf(fmt, args...)                 \
	do {                                       \
		sunxi_info(NULL, "[ info] "fmt, ##args);        \
		hdmi_log("hdmi20: [ info] "fmt, ##args); \
	} while (0)

#ifdef hdmi_wrn
#undef hdmi_wrn
#endif
#define hdmi_wrn(fmt, args...)                  \
	do {                                        \
		sunxi_warn(NULL, "[ warn] "fmt, ##args);  \
		hdmi_log("hdmi20: [ warn] "fmt, ##args);  \
	} while (0)

#ifdef hdmi_err
#undef hdmi_err
#endif
#define hdmi_err(fmt, args...)                  \
	do {                                        \
		sunxi_err(NULL, "[error] "fmt, ##args);  \
		hdmi_log("hdmi20: [error] "fmt, ##args);  \
	} while (0)

extern u32 gHdmi_log_level;

#define VIDEO_INF(fmt, args...)                      \
	do {                                             \
		if ((gHdmi_log_level == 1) || (gHdmi_log_level == 4) \
				|| (gHdmi_log_level > 6))                \
			sunxi_info(NULL, "[video] "fmt, ##args);  \
		hdmi_log("hdmi20: [video] "fmt, ##args);  \
	} while (0)

#define EDID_INF(fmt, args...)                       \
	do {                                             \
		if ((gHdmi_log_level == 2) || (gHdmi_log_level == 4) \
				|| (gHdmi_log_level > 6))                \
			sunxi_info(NULL, "[ edid] "fmt, ##args);   \
		hdmi_log("hdmi20: [ edid] "fmt, ##args);   \
	} while (0)

#define AUDIO_INF(fmt, args...)                      \
	do {                                             \
		if ((gHdmi_log_level == 3) || (gHdmi_log_level == 4) \
				|| (gHdmi_log_level > 6))                \
			sunxi_info(NULL, "[audio] "fmt, ##args);  \
		hdmi_log("hdmi20: [audio] "fmt, ##args);  \
	} while (0)

#define CEC_INF(fmt, args...)                         \
	do {                                              \
		if ((gHdmi_log_level > 6) || (gHdmi_log_level == 5))  \
			sunxi_info(NULL, "[  cec] "fmt, ##args);     \
		hdmi_log("hdmi20: [  cec] "fmt, ##args);     \
	} while (0)

#define HDCP_INF(fmt, args...)                      \
	do {                                            \
		if (gHdmi_log_level > 5)                        \
			sunxi_info(NULL, "[ hdcp] "fmt, ##args);  \
		hdmi_log("hdmi20: [ hdcp] "fmt, ##args);  \
	} while (0)

#define LOG_TRACE()                                    \
	do {                                               \
		if (gHdmi_log_level > 7)                           \
			sunxi_debug(NULL, "[trace] %s\n", __func__); \
		hdmi_log("hdmi20: [trace] %s\n", __func__); \
	} while (0)

#define LOG_TRACE1(a)                              \
	do {                                           \
		if (gHdmi_log_level > 7)                       \
			sunxi_debug(NULL, "[trace] %s: %d\n",    \
				__func__, a);                      \
		hdmi_log("hdmi20: [trace] %s: %d\n",    \
				__func__, a);                      \
	} while (0)

#define LOG_TRACE2(a, b)                             \
	do {                                             \
		if (gHdmi_log_level > 7)                         \
			sunxi_debug(NULL, "[trace] %s: %d %d\n",   \
				__func__, a, b);                     \
		hdmi_log("hdmi20: [trace] %s: %d %d\n",   \
				__func__, a, b);                     \
	} while (0)

#define LOG_TRACE3(a, b, c)                            \
	do {                                               \
		if (gHdmi_log_level > 7)                           \
			sunxi_debug(NULL, "[trace] %s: %d %d %d\n",  \
				__func__, a, b, c);                    \
		hdmi_log("hdmi20: [trace] %s: %d %d %d\n",  \
				__func__, a, b, c);                    \
	} while (0)

#endif /* _AW_CONFIG_H_ */

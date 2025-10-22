/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2021 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _TCON_FEAT_H_
#define _TCON_FEAT_H_

#define DE_OUTPUT_TYPE_LCD   1
#define DE_OUTPUT_TYPE_TV    2
#define DE_OUTPUT_TYPE_HDMI  4
#define DE_OUTPUT_TYPE_VGA   8
#define DE_OUTPUT_TYPE_VDPO  16
#define DE_OUTPUT_TYPE_EDP   32
#define DE_OUTPUT_TYPE_RTWB  64

#define TCON_TO_RGB         1
#define TCON_TO_LVDS        2
#define TCON_TO_SINGLE_DSI  4
#define TCON_TO_DUAL_DSI    8

#define CVBS_PAL_WIDTH 720
#define CVBS_PAL_HEIGHT 576
#define CVBS_NTSC_WIDTH 720
#define CVBS_NTSC_HEIGHT 480

#if defined(CONFIG_ARCH_SUN55IW6)
#define DEVICE_NUM	1
#define SUPPORT_LVDS
#define HAVE_DEVICE_COMMON_MODULE
#define DEVICE_COMMON_VERSION2

#define SUPPORT_DSI
#define DSI_VERSION_40
#define CLK_NUM_PER_DSI 1

/* What is described here is how many dsi are in the system */
#define DEVICE_DSI_NUM 1
#define SUPPORT_COMBO_DPHY

#endif

#define SUPPORT_EDP
#define EDP_SUPPORT_TCON_UPDATE_SAFE_TIME

#ifndef CLK_NUM_PER_DSI
#define CLK_NUM_PER_DSI 1
#endif

#ifndef COMBPHY_CLK_NUM_IN_SYS
#define COMBPHY_CLK_NUM_IN_SYS 2
#endif

#ifndef DEVICE_DSI_NUM
#define DEVICE_DSI_NUM 1
#endif /* endif DEVICE_DSI_NUM */

#ifndef DEVICE_LVDS_NUM
#define DEVICE_LVDS_NUM 1
#endif

#if defined(TV_UGLY_CLK_RATE)
#define TV_COMPOSITE_CLK_RATE 27000000
#endif

/* total number of DSI clk */
#define CLK_DSI_NUM  (CLK_NUM_PER_DSI * DEVICE_DSI_NUM)

int de_feat_get_num_devices(void);
int de_feat_get_num_dsi(void);
int de_feat_is_supported_output_types(unsigned int disp,
	unsigned int output_type);
int is_tcon_support_output_types(unsigned int tcon_id, unsigned int output_type);

#endif /* #ifndef _TCON_FEAT_H_ */

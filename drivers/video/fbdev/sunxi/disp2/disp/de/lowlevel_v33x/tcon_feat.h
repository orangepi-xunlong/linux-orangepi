/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2017 Allwinner.
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

#define CVBS_PAL_WIDTH 720
#define CVBS_PAL_HEIGHT 576
#define CVBS_NTSC_WIDTH 720
#define CVBS_NTSC_HEIGHT 480

#if defined(CONFIG_ARCH_SUN50IW5T)
#define DEVICE_NUM	2
#define SUPPORT_HDMI
#define SUPPORT_TV
/*#define SUPPORT_LVDS*/
#define HAVE_DEVICE_COMMON_MODULE
#define DEVICE_COMMON_VERSION2
/*#define SUPPORT_DSI*/
#define DSI_VERSION_28
#define CLK_NUM_PER_DSI 2
#define DEVICE_DSI_NUM 1
/*#define SUPPORT_EDP*/
#elif defined(CONFIG_ARCH_SUN50IW9)
#define DEVICE_NUM	4
#define SUPPORT_HDMI
#define SUPPORT_TV
#define TV_UGLY_CLK_RATE 216000000
#define SUPPORT_LVDS
#define HAVE_DEVICE_COMMON_MODULE
#define DEVICE_COMMON_VERSION2
#define USE_CEC_DDC_PAD
/*#define SUPPORT_DSI*/
#define DSI_VERSION_28
#define CLK_NUM_PER_DSI 2
#define DEVICE_DSI_NUM 1
#if defined(CONFIG_FPGA_V7_PLATFORM) || defined(CONFIG_FPGA_V4_PLATFORM)
#define LVDS_REVERT
#endif
/*#define SUPPORT_EDP*/
#endif

#ifndef CLK_NUM_PER_DSI
#define CLK_NUM_PER_DSI 1
#endif

#ifndef DEVICE_DSI_NUM
#define DEVICE_DSI_NUM 1
#endif /*endif DEVICE_DSI_NUM */

#ifndef DEVICE_LVDS_NUM
#if defined(CONFIG_ARCH_SUN50IW9)
#define DEVICE_LVDS_NUM 2
#else
#define DEVICE_LVDS_NUM 1
#endif
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

#endif /* #ifndef _TCON_FEAT_H_ */

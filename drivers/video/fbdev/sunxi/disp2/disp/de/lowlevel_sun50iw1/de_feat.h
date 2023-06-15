/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_FEAT_H_
#define _DE_FEAT_H_

#define DE_NUM	2
#define DEVICE_NUM	2
#define CHN_NUM		4
#define VI_CHN_NUM	1
#define UI_CHN_NUM	(CHN_NUM - VI_CHN_NUM)
#define LAYER_NUM_PER_CHN_PER_VI_CHN	4
#define LAYER_NUM_PER_CHN_PER_UI_CHN	4
#define LAYER_MAX_NUM_PER_CHN 4
#define VEP_NUM  1
#define DEVICE_VDPO_NUM 0
#define DE_WB_RESET_SHARE
#define WB_HAS_CSC

/*#define SUPPORT_DSI*/
/*#define SUPPORT_SMBL*/
#define SUPPORT_HDMI
#define SUPPORT_TV
#define TV_UGLY_CLK_RATE 216000000
#if defined(TV_UGLY_CLK_RATE)
#define TV_COMPOSITE_CLK_RATE 27000000
#endif

/* clk */
#define DE_LCD_CLK0 "tcon0"
#define DE_LCD_CLK1 "lcd1"
#define DE_LVDS_CLK "lvds"
#define DE_DSI_CLK0 "mipi_dsi0"
#define DE_DSI_CLK1 "mipi_dsi1"
#define DE_LCD_CLK_SRC "pll_video"
#define DE_HDMI_CLK_SRC "pll_video"

#define DE_CLK_SRC "pll_de"
#define DE_CORE_CLK "de"
#define DE_CORE_CLK_RATE 432000000  /*288000000*/

#define SUPPORT_DSI
#ifdef CONFIG_DISP2_SUNXI_SUPPORT_SMBL
#define SUPPORT_SMBL
#endif
#define SUPPORT_HDMI
#define DSI_VERSION_40
#define SUPPORT_LVDS
/* #define SUPPORT_TV */
/* #define LVDS_REVERT */
/*common macro define*/

#ifndef DEVICE_LVDS_NUM
#define DEVICE_LVDS_NUM 1
#endif

#ifndef CLK_NUM_PER_DSI
#define CLK_NUM_PER_DSI 1
#endif

#if defined(DEVICE_DSI_NUM)
/* total number of DSI clk */
#define CLK_DSI_NUM  (CLK_NUM_PER_DSI * DEVICE_DSI_NUM)
#else
#define CLK_DSI_NUM  CLK_NUM_PER_DSI
#define DEVICE_DSI_NUM 1
#endif /*endif DEVICE_DSI_NUM */


struct de_feat {
	const int num_screens;/* indicate layer manager number */
	const int num_devices;/* indicate timing controller number */
	const int *num_chns;
	const int *num_vi_chns;
	const int *num_layers;
	const int *is_support_vep;
	const int *is_support_smbl;
	const int *is_support_wb;
	const int *supported_output_types;
	const int *is_support_scale;
};

int de_feat_init(void);
int de_feat_get_num_screens(void);
int de_feat_get_num_devices(void);
int de_feat_get_num_chns(unsigned int disp);
int de_feat_get_num_vi_chns(unsigned int disp);
int de_feat_get_num_ui_chns(unsigned int disp);
int de_feat_get_num_layers(unsigned int disp);
int de_feat_get_num_layers_by_chn(unsigned int disp, unsigned int chn);
int de_feat_is_support_vep(unsigned int disp);
int de_feat_is_support_vep_by_chn(unsigned int disp, unsigned int chn);
int de_feat_is_support_smbl(unsigned int disp);
int de_feat_is_supported_output_types(unsigned int  disp, unsigned int output_type);
int de_feat_is_support_wb(unsigned int disp);
int de_feat_is_support_scale(unsigned int disp);
int de_feat_is_support_scale_by_chn(unsigned int disp, unsigned int chn);
unsigned int de_feat_get_number_of_vdpo(void);
int de_feat_exit(void);

#endif


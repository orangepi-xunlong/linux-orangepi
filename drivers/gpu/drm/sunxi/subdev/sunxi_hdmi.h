/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*HDMI panel ops*/
#ifndef _SUNXI_HDMI_H_
#define _SUNXI_HDMI_H_
#include "hdmi_bsp.h"
#define Abort_Current_Operation             0
#define Special_Offset_Address_Read         1
#define Explicit_Offset_Address_Write       2
#define Implicit_Offset_Address_Write       3
#define Explicit_Offset_Address_Read        4
#define Implicit_Offset_Address_Read        5
#define Explicit_Offset_Address_E_DDC_Read  6
#define Implicit_Offset_Address_E_DDC_Read  7

#define HDMI1440_480I		6
#define HDMI1440_576I		21
#define HDMI480P			2
#define HDMI576P			17
#define HDMI720P_50			19
#define HDMI720P_60 		4
#define HDMI1080I_50		20
#define HDMI1080I_60		5
#define HDMI1080P_50		31
#define HDMI1080P_60 		16
#define HDMI1080P_24 		32
#define HDMI1080P_25 		33
#define HDMI1080P_30 		34
#define HDMI1080P_24_3D_FP  (HDMI1080P_24 +0x80)
#define HDMI720P_50_3D_FP   (HDMI720P_50  +0x80)
#define HDMI720P_60_3D_FP   (HDMI720P_60  +0x80)
#define HDMI3840_2160P_30   (1+0x100)
#define HDMI3840_2160P_25   (2+0x100)
#define HDMI3840_2160P_24   (3+0x100)
#define HDMI_EDID_LEN 1024

#define HDMI_State_Idle 			 0x00
#define HDMI_State_Wait_Hpd			 0x02
#define HDMI_State_Rx_Sense			 0x03
#define HDMI_State_EDID_Parse		 0x04
#define HDMI_State_HPD_Done			 0x05

#define HDMI_IO_NUM 5

struct sunxi_hdmi_private {
	int hdmi_id;
	struct sunxi_panel *sunxi_panel;
	struct disp_video_timings *timing;
	bool    hdmi_io_used[HDMI_IO_NUM];
	bool    power_on;
	bool    can_YCbCr444;
	bool    can_YCbCr422;
	bool    cts_compat;
	bool    hdcp_enable;
	bool    cec_support;
	bool    exp;
	disp_gpio_set_t hdmi_io[HDMI_IO_NUM];
	int             gpio_handle[HDMI_IO_NUM];
	char    hdmi_power[25];
	struct cea_sad    *hdmi_sads;
	int   nr_sad;
	struct video_para video_para;
	struct audio_para audio_para;
};

struct sunxi_panel *sunxi_hdmi_pan_init(struct sunxi_hardware_res *hw_res, int pan_id, int hdmi_id);

void sunxi_hdmi_pan_destroy(struct sunxi_panel *sunxi_panel,
	struct sunxi_hardware_res *hw_res);
enum drm_mode_status
	sunxi_hdmi_mode_timmings(void *timing, struct drm_display_mode *mode);
#endif

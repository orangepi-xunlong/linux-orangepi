/*
 * A V4L2 driver for nvp6158c yuv cameras.
 *
 * Copyright (c) 2019 by Allwinnertech Co., Ltd.  http://www.allwinnertech.com
 *
 * Authors:  Zheng Zequn<zequnzheng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _NVP6158_VIDEO_HI_
#define _NVP6158_VIDEO_HI_

#include "common.h"

/********************************************************************
 *  define and enum
 ********************************************************************/

typedef enum _nvp6158_outmode_sel {
	NVP6158_OUTMODE_1MUX_SD = 0,
	NVP6158_OUTMODE_1MUX_HD,
	NVP6158_OUTMODE_1MUX_FHD,
	NVP6158_OUTMODE_2MUX_SD,
	NVP6158_OUTMODE_2MUX_HD,
	NVP6158_OUTMODE_2MUX_MIX,
	NVP6158_OUTMODE_2MUX_FHD,
	NVP6158_OUTMODE_4MUX_SD,
	NVP6158_OUTMODE_4MUX_HD,
	NVP6158_OUTMODE_4MUX_MIX,
	NVP6158_OUTMODE_1MUX_297MHz,
	NVP6158_OUTMODE_1MUX_BT1120S,
	NVP6158_OUTMODE_2MUX_BT1120S,
	NVP6158_OUTMODE_4MUX_BT1120S,
	NVP6158_OUTMODE_BUTT
} NVP6158_OUTMODE_SEL;

typedef enum _nvp6158_det_sel {
	NVP6158_DET_MODE_AUTO = 0,
	NVP6158_DET_MODE_AHD,
	NVP6158_DET_MODE_CVI,
	NVP6158_DET_MODE_TVI,
	NVP6158_DET_MODE_OTHER,
	NVP6158_DET_MODE_BUTT
} NVP6158_DET_SEL;

/********************************************************************
 *  structure
 ********************************************************************/

/********************************************************************
 *  external api
 ********************************************************************/
void nvp6158_common_init(unsigned char chip);
int nvp6158_set_portmode(const unsigned char chip, const unsigned char portsel, const unsigned char portmode, const unsigned char chid);
int nvp6158_set_chnmode(const unsigned char ch, const unsigned char chnmode);
int nvp6168_set_chnmode(const unsigned char ch, const unsigned char chnmode);
void nvp6158_set_portcontrol(unsigned char chip, unsigned char portsel, unsigned char enclk, unsigned char endata);
/* 新增加通道模式函数 */
void nvp6158_set_chn_commonvalue(const unsigned char ch, const unsigned char chnmode);

unsigned int video_fmt_det(const unsigned char ch, NVP6158_INFORMATION_S *ps_nvp6158_vfmts);
unsigned int nvp6168_video_fmt_det(const unsigned char ch, NVP6158_INFORMATION_S *ps_nvp6158_vfmts);
unsigned char video_fmt_debounce(unsigned char ch, unsigned char keep_fmt, unsigned int keep_sync_width);
unsigned int nvp6158_getvideoloss(void);

void nvp6158_video_set_contrast(unsigned char ch, unsigned int value, unsigned int v_format);
void nvp6158_video_set_brightness(unsigned char ch, unsigned int value, unsigned int v_format);
void nvp6158_video_set_saturation(unsigned char ch, unsigned int value, unsigned int v_format);
void nvp6158_video_set_hue(unsigned char ch, unsigned int value, unsigned int v_format);
void nvp6158_video_set_sharpness(unsigned char ch, unsigned int value);
void nvp6158_video_set_ugain(unsigned char ch, unsigned int value);
void nvp6158_video_set_vgain(unsigned char ch, unsigned int value);
void nvp6158_video_set_adcclk(unsigned char ch, unsigned char value);
unsigned char nvp6158_video_get_adcclk(unsigned char ch);
void nvp6158_hide_ch(unsigned char ch);
void nvp6158_show_ch(unsigned char ch);

void nvp6158_vd_chnreset(unsigned char ch);
int nvp6158_GetAgcLockStatus(unsigned char ch);
int nvp6158_GetFSCLockStatus(unsigned char ch);
void nvp6158_ResetFSCLock(unsigned char ch);
void nvp6158_chn_killcolor(unsigned char ch, unsigned char onoff);
int nvp6158_acp_SetVFmt(unsigned char ch, const unsigned char vfmt);
void video_input_new_format_set(const unsigned char ch, const unsigned char chnmode);
void nvp6158_dump_reg(unsigned char ch, unsigned char bank);
NC_FORMAT_STANDARD NVP6158_GetFmtStd_from_Fmtdef(NC_VIVO_CH_FORMATDEF vivofmt);
void nvp6158_additional_for3MoverDef(unsigned char chip);
void nvp6158_video_powerdown(unsigned char ch);

#endif /* End of _NVP6158_VIDEO_HI_ */

/********************************************************************
 *  End of file
 ********************************************************************/

/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_CDC_TAB__
#define __DE_CDC_TAB__

#include "linux/types.h"

#define DE_CDC_LUT_NUM (8)

enum {
	DE_TFC_SDR2SDR     = 0x000,
	DE_TFC_SDR2WCG     = 0x001,
	DE_TFC_SDR2HDR10   = 0x002,
	DE_TFC_SDR2HLG     = 0x003,
	DE_TFC_WCG2SDR     = 0x004,
	DE_TFC_WCG2WCG     = 0x005,
	DE_TFC_WCG2HDR10   = 0x006,
	DE_TFC_WCG2HLG     = 0x007,
	DE_TFC_HDR102SDR   = 0x008,
	DE_TFC_HDR102WCG   = 0x009,
	DE_TFC_HDR102HDR10 = 0x00a,
	DE_TFC_HDR102HLG   = 0x00b,
	DE_TFC_HLG2SDR     = 0x00c,
	DE_TFC_HLG2WCG     = 0x00d,
	DE_TFC_HLG2HDR10   = 0x00e,
	DE_TFC_HLG2HLG     = 0x00f,
	DE_TFC_INIT        = 0x0ff, /* for initial */
};

extern u32 *cdc_lut_ptr_r[DE_TFC_HLG2HLG + 1][DE_CDC_LUT_NUM];
extern u32 *cdc_lut_ptr_y[DE_TFC_HLG2HLG + 1][DE_CDC_LUT_NUM];

#endif


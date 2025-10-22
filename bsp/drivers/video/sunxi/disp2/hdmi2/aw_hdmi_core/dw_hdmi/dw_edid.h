/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DW_EDID_H
#define _DW_EDID_H

#include "dw_dev.h"

typedef enum {
	EDID_ERROR = 0,
	EDID_IDLE,
	EDID_READING,
	EDID_DONE
} edid_status_t;

typedef struct supported_dtd {
	u32 refresh_rate;/* 1HZ * 1000 */
	dw_dtd_t dtd;
} supported_dtd_t;

typedef struct speaker_alloc_code {
	unsigned char byte;
	unsigned char code;
} speaker_alloc_code_t;

#define EDID_DETAIL_EST_TIMINGS 0xf7
#define EDID_DETAIL_CVT_3BYTE 0xf8
#define EDID_DETAIL_COLOR_MGMT_DATA 0xf9
#define EDID_DETAIL_STD_MODES 0xfa
#define EDID_DETAIL_MONITOR_CPDATA 0xfb
#define EDID_DETAIL_MONITOR_NAME 0xfc
#define EDID_DETAIL_MONITOR_RANGE 0xfd
#define EDID_DETAIL_MONITOR_STRING 0xfe
#define EDID_DETAIL_MONITOR_SERIAL 0xff


/* Short Audio Descriptor */
struct cea_sad {
	u8 format;
	u8 channels; /* max number of channels - 1 */
	u8 freq;
	u8 byte2; /* meaning depends on format */
};

extern u16 byte_to_word(const u8 hi, const u8 lo);
extern u8 bit_field(const u16 data, u8 shift, u8 width);
extern u16 concat_bits(u8 bHi, u8 oHi, u8 nHi, u8 bLo, u8 oLo, u8 nLo);

/**
 * @param dtd pointer to dw_dtd_t strucutute for the information to be save in
 * @param code the CEA 861-D video code.
 * @param refreshRate the specified vertical refresh rate.
 * @return true if success
 */
int dw_dtd_fill(dw_hdmi_dev_t *dev, dw_dtd_t *dtd, u32 code, u32 refreshRate);

/**
 * @param dtd Pointer to the current DTD parameters
 * @return The refresh rate if DTD parameters are correct and 0 if not
 */
u32 dw_dtd_get_rate(dw_dtd_t *dtd);

void dw_edid_reset(dw_hdmi_dev_t *dev, sink_edid_t *edidExt);

int dw_edid_parser(dw_hdmi_dev_t *dev, u8 *buffer, sink_edid_t *edidExt);

int dw_edid_read_extenal_block(dw_hdmi_dev_t *dev, int block, u8 *edid_ext);

int dw_edid_read_base_block(dw_hdmi_dev_t *dev, struct edid *edid);

#endif	/* _DW_EDID_H */

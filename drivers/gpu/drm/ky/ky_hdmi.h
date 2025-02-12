// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */
#ifndef _KY_HDMI_H_
#define _KY_HDMI_H_

#include <linux/notifier.h>
#include <soc/ky/ky_panel.h>

enum {
	INFOFRAME_VSI = 0x05,
	INFOFRAME_AVI = 0x06,
	INFOFRAME_AAI = 0x08,
};

enum PWR_MODE {
	NORMAL,
	LOWER_PWR,
};

enum {
	HDMI_TIMING_640_480P_60 = 0,
	HDMI_TIMING_720_480P_60 = 1,
	HDMI_TIMING_720_576P_50 = 2,
	HDMI_TIMING_1280_720P_30 = 3,
	HDMI_TIMING_1280_720P_50 = 4,
	HDMI_TIMING_1920_1080P_24 = 5,
	HDMI_TIMING_1920_1080P_60 = 6,
	HDMI_TIMING_MAX
};

enum {
	HDMI_SYNC_POL_POS,
	HDMI_SYNC_POL_NEG,
};

struct hdmi_timing
{
	unsigned short hfp;
	unsigned short hbp;
	unsigned short hsync;
	unsigned short hact;

	unsigned short vfp;
	unsigned short vbp;
	unsigned short vsync;
	unsigned short vact;

	unsigned short vic;
	unsigned short hpol;
	unsigned short vpol;
	unsigned short reserved;
};

static BLOCKING_NOTIFIER_HEAD(hdmi_notifier_list);

int ky_hdmi_notifier_call_chain(unsigned long val, void *v);

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2007-2020 allwinnertech Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SUNXI_IR_RX_MAP_H
#define SUNXI_IR_RX_MAP_H
#include <media/rc-map.h>

#define RC_MAP_SUNXI		"rc_map_sunxi"

static struct rc_map_table sunxi_nec_scan[] = {

	/* Key codes for the SMDT remote */
	{ 0x0f, KEY_POWER },
	{ 0x52, KEY_HOME },
	{ 0x10, KEY_MENU },
	{ 0x58, KEY_MUTE },
	{ 0x5e, KEY_EPG },		//mouse
	{ 0x4d, KEY_UP },
	{ 0x11, KEY_PREVIOUS },
	{ 0x5c, KEY_VOLUMEUP },
	{ 0x57, KEY_LEFT },
	{ 0x5b, KEY_OK },
	{ 0x5f, KEY_RIGHT },
	{ 0x54, KEY_VOLUMEDOWN },
	{ 0x56, KEY_TAB },		// tab
	{ 0x5a, KEY_DOWN },
	{ 0x12, KEY_NEXT },
	{ 0x53, KEY_BACK },
	{ 0x17, KEY_NUMERIC_1 },
	{ 0x1b, KEY_NUMERIC_2 },
	{ 0x1f, KEY_NUMERIC_3 },
	{ 0x16, KEY_NUMERIC_4 },
	{ 0x1a, KEY_NUMERIC_5 },
	{ 0x1e, KEY_NUMERIC_6 },
	{ 0x15, KEY_NUMERIC_7 },
	{ 0x19, KEY_NUMERIC_8 },
	{ 0x1d, KEY_NUMERIC_9 },
	{ 0x18, KEY_NUMERIC_0 },
	{ 0x14, KEY_DOT },
	{ 0x1c, KEY_BACKSPACE },
	{ 0x13, BTN_TRIGGER_HAPPY18 },
	{ 0x50, BTN_TRIGGER_HAPPY16 },
	{ 0x59, KEY_PAUSE },
	{ 0x55, KEY_PLAY },

	/* Key codes for the x96 remote */
	{ 0x140, KEY_POWER },
	{ 0x144, KEY_KPDOT },
	{ 0x155, KEY_REWIND },
	{ 0x15a, KEY_PLAYPAUSE },
	{ 0x152, KEY_STOP },
	{ 0x154, KEY_FASTFORWARD },
	{ 0x143, KEY_SETUP },
	{ 0x10f, BTN_TRIGGER_HAPPY14 },
	{ 0x110, KEY_VOLUMEDOWN },
	{ 0x118, KEY_VOLUMEUP },
	{ 0x111, KEY_HOME },
	{ 0x119, KEY_BACK },
	{ 0x116, KEY_UP },
	{ 0x151, KEY_LEFT },
	{ 0x113, KEY_OK },
	{ 0x150, KEY_RIGHT },
	{ 0x11a, KEY_DOWN },
	{ 0x14c, KEY_MENU },
	{ 0x100, KEY_EPG }, 		// mouse
	{ 0x14e, KEY_NUMERIC_1 },
	{ 0x10d, KEY_NUMERIC_2 },
	{ 0x10c, KEY_NUMERIC_3 },
	{ 0x14a, KEY_NUMERIC_4 },
	{ 0x109, KEY_NUMERIC_5 },
	{ 0x108, KEY_NUMERIC_6 },
	{ 0x146, KEY_NUMERIC_7 },
	{ 0x105, KEY_NUMERIC_8 },
	{ 0x104, KEY_NUMERIC_9 },
	{ 0x141, KEY_MUTE },
	{ 0x101, KEY_NUMERIC_0 },
	{ 0x142, KEY_BACKSPACE },

	/* ------ Key codes for the pin64 remote  ------ */
	{ 0x40404d, KEY_POWER },
	{ 0x404043, KEY_MUTE },
	{ 0x404017, KEY_VOLUMEDOWN },
	{ 0x404018, KEY_VOLUMEUP },
	{ 0x40400b, KEY_UP },
	{ 0x404010, KEY_LEFT },
	{ 0x40400d, KEY_OK },
	{ 0x404011, KEY_RIGHT },
	{ 0x40400e, KEY_DOWN },
	{ 0x40401a, KEY_HOME },
	{ 0x404045, KEY_MENU },
	{ 0x404042, KEY_BACK },
	{ 0x404001, KEY_NUMERIC_1 },
	{ 0x404002, KEY_NUMERIC_2 },
	{ 0x404003, KEY_NUMERIC_3 },
	{ 0x404004, KEY_NUMERIC_4 },
	{ 0x404005, KEY_NUMERIC_5 },
	{ 0x404006, KEY_NUMERIC_6 },
	{ 0x404007, KEY_NUMERIC_7 },
	{ 0x404008, KEY_NUMERIC_8 },
	{ 0x404009, KEY_NUMERIC_9 },
	{ 0x404047, KEY_EPG },		// mouse
	{ 0x404000, KEY_NUMERIC_0 },
	{ 0x40400c, KEY_BACKSPACE },

	/* Key codes for the 0xbf remote */
	{ 0xbf00, KEY_POWER },
	{ 0xbf01, KEY_MUTE },
	{ 0xbf02, KEY_NUMERIC_1 },
	{ 0xbf03, KEY_NUMERIC_2 },
	{ 0xbf04, KEY_NUMERIC_3 },
	{ 0xbf05, KEY_NUMERIC_4 },
	{ 0xbf06, KEY_NUMERIC_5 },
	{ 0xbf07, KEY_NUMERIC_6 },
	{ 0xbf08, KEY_NUMERIC_7 },
	{ 0xbf09, KEY_NUMERIC_8 },
	{ 0xbf0a, KEY_NUMERIC_9 },
	{ 0xbf0b, BTN_MISC },		// unknow
	{ 0xbf0c, KEY_NUMERIC_0 },
	{ 0xbf0d, BTN_MISC },		// unknow
	{ 0xbf41, BTN_MISC },		// unknow
	{ 0xbf42, BTN_MISC },		// unknow
	{ 0xbf44, BTN_MISC },		// unknow
	{ 0xbf47, BTN_MISC },		// unknow
	{ 0xbf40, BTN_MISC },		// unknow
	{ 0xbf13, BTN_MISC },
	{ 0xbf11, KEY_LEFT },
	{ 0xbf10, KEY_OK },
	{ 0xbf12, KEY_RIGHT },
	{ 0xbf14, KEY_DOWN },
	{ 0xbf16, KEY_MENU },
	{ 0xbf15, KEY_BACK },
	{ 0xbf48, KEY_VOLUMEUP },
	{ 0xbf49, KEY_VOLUMEDOWN },
	{ 0xbf0f, KEY_TV },
	{ 0xbf43, BTN_MISC },		// unknow
	{ 0xbf5b, BTN_MISC },		// unknow
	{ 0xbf4a, KEY_CHANNELUP },
	{ 0xbf4b, KEY_CHANNELDOWN },
	{ 0xbf4c, BTN_MISC },		// unknow
	{ 0xbf4d, BTN_MISC },		// unknow
	{ 0xbf4e, BTN_MISC },		// unknow
	{ 0xbf4f, BTN_MISC },		// unknow
	{ 0xbf51, KEY_PLAYPAUSE },
	{ 0xbf56, KEY_REWIND },
	{ 0xbf57, KEY_FASTFORWARD },
	{ 0xbf1c, BTN_MISC },		// unknow
	{ 0xbf52, KEY_STOPCD },
	{ 0xbf54, KEY_PREVIOUSSONG },
	{ 0xbf55, KEY_NEXTSONG },
	{ 0xbf60, BTN_MISC },		// unknow
	{ 0xbf62, BTN_MISC },		// unknow
	{ 0xbf63, BTN_MISC },		// unknow
	{ 0xbf58, BTN_MISC },		// unknow
	{ 0xbf61, KEY_HOME },

};

#endif /* SUNXI_IR_RX_MAP_H */

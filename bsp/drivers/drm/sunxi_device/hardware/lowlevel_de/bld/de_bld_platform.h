/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2023 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _DE_BLD_PLATFORM_H_
#define _DE_BLD_PLATFORM_H_

#include "de_base.h"

#define VIDEO_CHANNEL_ID_SHIFT		(0)
#define UI_CHANNEL_ID_SHIFT		(16)
#define BLD_PORT_MAX			(6)
#define CHN_MODE_FIX			(65535)

struct de_bld_desc {
	char name[32];
	unsigned int id;
	u32 disp_base;
	u32 bld_offset;
	unsigned int mode_cnt;
	const struct de_bld_port_mux_mode *mode;
};

struct de_bld_port_mux_mode {
	char name[32];
	unsigned int channel_cnt;
	unsigned int mode_id;
	unsigned long channel_id[BLD_PORT_MAX];
};

const struct de_bld_desc *get_bld_dsc(struct module_create_info *info);
#endif

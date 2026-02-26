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

#ifndef _DE_SCALER_PLATFORM_H_
#define _DE_SCALER_PLATFORM_H_

#include "de_base.h"

struct de_version_scaler {
	unsigned int version;
	unsigned int scaler_cnt;
	struct de_scaler_dsc *scaler;
};

enum scaler_type {
	DE_SCALER_TYPE_NONE = 0,
	DE_SCALER_TYPE_VSU8,
	DE_SCALER_TYPE_VSU10,
	DE_SCALER_TYPE_VSU_ED,
	DE_SCALER_TYPE_GSU,
	DE_SCALER_TYPE_ASU,
};

struct de_scaler_dsc {
	char name[32];
	unsigned int id;
	unsigned int offset;
	unsigned int linebuff_share_ids;
	bool need_switch_en;
	enum scaler_type type;
	unsigned int line_buffer_yuv;
	unsigned int line_buffer_rgb;
	unsigned int line_buffer_yuv_ed;
};

struct de_scaler_dsc *get_scaler_dsc(const struct module_create_info *info);

#endif

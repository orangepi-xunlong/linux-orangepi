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

#ifndef _DE_OVL_PLATFORM_H_
#define _DE_OVL_PLATFORM_H_

#include "de_base.h"
#include "de_ovl_type.h"

#define OVL_LAYER_CNT_MAX			(4)

struct de_ovl_dsc {
	char name[32];
	unsigned int id;
	unsigned int type_hw_id;
	unsigned int layer_cnt;
	u32 channel_base;
	u32 ovl_offset;
	enum ovl_type type;
};

const struct de_ovl_dsc *get_ovl_dsc(struct module_create_info *info);
#endif

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

#ifndef _DE_DLC_PLATFORM_H_
#define _DE_DLC_PLATFORM_H_

#include "de_base.h"

struct de_version_dlc {
	unsigned int version;
	unsigned int dlc_cnt;
	struct de_dlc_desc **dlc;
};

struct de_dlc_desc {
	char name[32];
	unsigned int id;
	unsigned int min_inwidth;
	unsigned int min_inheight;
	unsigned int reg_offset;
};

/* input info from channel */
const struct de_dlc_desc *get_dlc_desc(struct module_create_info *info);

#endif

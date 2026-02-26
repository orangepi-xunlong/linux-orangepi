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

#ifndef _DE_CSC_PLATFORM_H_
#define _DE_CSC_PLATFORM_H_

#include "de_base.h"
#include "de_csc.h"

struct channel_csc_id {
	unsigned int channel_id;
	unsigned int csc_id;
};

struct device_csc_id {
	unsigned int device_id;
};

struct de_csc_desc {
	char name[32];
	unsigned int version;
	union {
		struct channel_csc_id cid;
		struct device_csc_id did;
	};
	unsigned int csc_bit_width;
	int hue_default_value;
	enum de_csc_type type;
	unsigned int reg_offset;
};

const struct de_csc_desc *get_csc_desc(struct module_create_info *info);

#endif

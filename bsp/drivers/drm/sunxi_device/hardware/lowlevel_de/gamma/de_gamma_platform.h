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

#ifndef _DE_GAMMA_PLATFORM_H_
#define _DE_GAMMA_PLATFORM_H_

#include "de_base.h"
#include "de_gamma.h"

struct de_gamma_dsc {
	unsigned int id;
	unsigned int gamma_lut_len;
	unsigned int cm_bit_width;
	bool support_ctc;
	bool support_cm;
	bool support_demo_skin;
	enum de_gamma_type type;
	unsigned int reg_offset;
};

const struct de_gamma_dsc *get_gamma_dsc(struct module_create_info *info);

#endif

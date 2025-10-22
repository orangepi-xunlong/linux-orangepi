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

#ifndef _DE_CRC_PLATFORM_H_
#define _DE_CRC_PLATFORM_H_

#include "de_base.h"

struct de_crc_dsc {
	unsigned int id;
	unsigned int region_cnt;
};

const struct de_crc_dsc *get_crc_dsc(struct module_create_info *info);

#endif

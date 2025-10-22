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

#ifndef _DE_CRC_H_
#define _DE_CRC_H_

#include "de_base.h"

struct de_crc_handle {
	struct module_create_info cinfo;
	unsigned int region_cnt;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_crc_private *private;
};

enum de_crc_work_mode {
	CHECK_EQUAL = 0x0,
	CHECK_DIFF = 0x1,
};

struct de_crc_gbl_cfg {
	unsigned int w, h;
};

struct de_crc_region_cfg {
	unsigned int region_id;
	bool enable;
	bool irq_enable;
	enum de_crc_work_mode mode;
	unsigned int check_frame_step;
	unsigned int x_start, x_end;
	unsigned int y_start, y_end;
};

struct de_crc_handle *de_crc_create(struct module_create_info *info);
u32 de_crc_check_status_with_clear(struct de_crc_handle *hdl, unsigned int region_mask);
int de_crc_global_config(struct de_crc_handle *hdl, const struct de_crc_gbl_cfg *cfg);
int de_crc_region_config(struct de_crc_handle *hdl, const struct de_crc_region_cfg *cfg);
void de_crc_dump_state(struct drm_printer *p, struct de_crc_handle *hdl);

#endif

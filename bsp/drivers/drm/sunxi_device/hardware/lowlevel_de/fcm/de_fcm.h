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

#ifndef _DE_FCM_H_
#define _DE_FCM_H_

#include <linux/types.h>
#include "de_base.h"
#include "de_channel.h"
#include "de_fcm_platform.h"
#include "de_fcm_type.h"
#include "../csc/de_csc.h"
#include "../csc/de_csc_table.h"

struct de_fcm_handle {
	struct module_create_info cinfo;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_fcm_private *private;
};

struct de_fcm_handle *de_fcm_create(struct module_create_info *info);
bool de_fcm_is_enabled(struct de_fcm_handle *hdl);
int de_fcm_set_csc(struct de_fcm_handle *hdl,
				   struct de_csc_info *in_info, struct de_csc_info *out_info);
s32 de_fcm_enable(struct de_fcm_handle *hdl, u32 en);
void de_fcm_update_regs(struct de_fcm_handle *hdl);
s32 de_fcm_set_demo_mode(struct de_fcm_handle *hdl, bool enable);
s32 de_fcm_set_window(struct de_fcm_handle *hdl,
			u32 x, u32 y, u32 w, u32 h);
s32 de_fcm_set_size(struct de_fcm_handle *hdl, u32 width, u32 height);
s32 de_fcm_lut_proc(struct de_fcm_handle *hdl, struct fcm_info *info);
s32 de_fcm_dump_state(struct drm_printer *p, struct de_fcm_handle *hdl);


#endif /* #ifndef _DE_FCM_H_ */

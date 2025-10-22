/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

/**
 *	All Winner Tech, All Right Reserved. 2014-2015 Copyright (c)
 *
 *	File name   :       de_smbl.h
 *
 *	Description :       display engine 2.0 smbl basic function declaration
 *
 *	History     :       2014/05/15  vito cheng  v0.1  Initial version
 *
 */

#ifndef _DE_SMBL_H_
#define _DE_SMBL_H_

#include "de_base.h"
#include "de_csc.h"

struct de_smbl_handle {
	struct module_create_info cinfo;
	bool support_csc;
    int hue_default_value;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_smbl_private *private;
};

enum disp_smbl_dirty_flags {
	SMBL_DIRTY_NONE = 0x00000000,
	SMBL_DIRTY_ENABLE = 0x00000001,
	SMBL_DIRTY_WINDOW = 0x00000002,
	SMBL_DIRTY_SIZE = 0x00000004,
	SMBL_DIRTY_BL = 0x00000008,
	SMBL_DIRTY_ALL = 0x0000000F,
};

struct disp_smbl_info {
	u32 demo_en;
	struct drm_rect window;
	u32 enable;
	struct drm_rect size;
	u32 backlight;
	u32 backlight_after_dimming;
	u32 backlight_dimming;
	enum disp_smbl_dirty_flags flags;
};

int de_smbl_apply_csc(struct de_smbl_handle *hdl, u32 w, u32 h, const struct de_csc_info *in_info,
		    const struct de_csc_info *out_info, const struct bcsh_info *bcsh, const struct ctm_info *ctm);
s32 de_smbl_update_local_param(struct de_smbl_handle *hdl);
s32 de_smbl_apply(struct de_smbl_handle *hdl, struct disp_smbl_info *info);
s32 de_smbl_get_status(struct de_smbl_handle *hdl, struct disp_smbl_info *info);
void de_smbl_dump_state(struct drm_printer *p, struct de_smbl_handle *hdl);
struct de_smbl_handle *de_smbl_create(struct module_create_info *info);
s32 de_smbl_enable_ahb_read(struct de_smbl_handle *hdl, bool en);

#endif /* #ifndef _DE_SMBL_H_ */

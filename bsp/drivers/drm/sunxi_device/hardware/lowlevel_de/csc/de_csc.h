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

#ifndef _DE_CSC_H_
#define _DE_CSC_H_

#include <linux/types.h>
#include "de_base.h"
#include "de_channel.h"

/*
 *	  if not CHANNEL_CSC/DEVICE_CSC, csc's regs is not managed by csc,
 *	but the module which contains csc itself. becase csc's regs are part
 *	of module regs, managing them only by module itself is easily and
 *	may avoid potential trouble and issue.
*/
enum de_csc_type {
	CHANNEL_CSC,
	DEVICE_CSC,
	GAMMA_CSC,
	SMBL_CSC,
	FCM_CSC,
	CDC_CSC,
};

struct csc_extra_create_info {
	enum de_csc_type type;
	unsigned int extra_id;
};

struct de_csc_handle {
	struct module_create_info cinfo;
	struct csc_extra_create_info ex_cinfo;
	int hue_default_value;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_csc_private *private;
};

struct de_csc_info {
	enum de_format_space px_fmt_space;
	enum de_color_space color_space;
	enum de_color_range color_range;
	enum de_eotf eotf;
	u32 width;
	u32 height;
};

struct de_csc_handle *de_csc_create(struct module_create_info *info);

struct bcsh_info {
	bool enable;
	bool dirty;
	unsigned int brightness;
	unsigned int contrast;
	unsigned int saturation;
	unsigned int hue;
};

struct ctm_info {
	bool enable;
	bool dirty;
	struct de_color_ctm ctm;
};

struct de_csc_handle *de_csc_create(struct module_create_info *info);
void de_dcsc_pq_matrix_proc(struct matrix4x4 *conig, enum matrix_type type,
		   bool write);
s32 de_csc_apply(struct de_csc_handle *hdl, struct de_csc_info *in_info,
		    struct de_csc_info *out_info, int *csc_coeff, bool apply, bool en);
int de_dcsc_apply(struct de_csc_handle *hdl, const struct de_csc_info *in_info,
			const struct de_csc_info *out_info,
			const struct bcsh_info *bcsh, const struct ctm_info *ctm,
			int *csc_coeff, bool apply);
s32 de_csc_enable(struct de_csc_handle *hdl, u32 en);
void de_csc_dump_state(struct drm_printer *p, struct de_csc_handle *hdl);

#endif /* #ifndef _DE_CSC_H_ */

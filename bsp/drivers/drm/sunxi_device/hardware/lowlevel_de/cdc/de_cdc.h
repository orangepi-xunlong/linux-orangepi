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

#ifndef _DE_CDC_H_
#define _DE_CDC_H_

#include <linux/types.h>
#include "de_base.h"
#include "de_channel.h"
#include "../csc/de_csc.h"
#include "../csc/de_csc_table.h"

struct de_cdc_handle {
	struct module_create_info cinfo;
	unsigned int block_num;
	struct de_reg_block **block;
	struct de_cdc_private *private;
	bool support_gtm;
};

bool de_cdc_gtm_is_enabled(struct de_cdc_handle *hdl);
struct de_cdc_handle *de_cdc_create(struct module_create_info *info);
s32 de_cdc_apply(struct de_cdc_handle *hdl,
	struct de_csc_info *in_info, struct de_csc_info *out_info, bool gtm_enable);
s32 de_cdc_enable(struct de_cdc_handle *hdl, u32 enable);
void de_cdc_update_regs(struct de_cdc_handle *hdl);
s32 de_cdc_dump_state(struct drm_printer *p, struct de_cdc_handle *hdl);
s32 de_cdc_update_local_param(struct de_cdc_handle *hdl);
int de_gtm_pq_proc(struct de_cdc_handle *hdl, hdr_module_param_t *para);


#endif /* #ifndef _DE_CDC_H_ */

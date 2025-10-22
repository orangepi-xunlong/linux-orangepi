/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_DCSC_H__
#define __DE_DCSC_H__

#include "de_rtmx.h"

int de_dcsc_apply(unsigned int sel, struct disp_csc_config *config);
int de_dcsc_set_colormatrix(unsigned int sel, long long *matrix4x3,
			    bool is_identity);
s32 de_dcsc_get_reg_blocks(u32 disp, struct de_reg_block **blks,
			  u32 *blk_num);
int de_dcsc_init(uintptr_t reg_base);
int de_dcsc_exit(void);
int de_dcsc_update_regs(unsigned int sel);
int de_dcsc_get_config(unsigned int sel, struct disp_csc_config *config);

void de_dcsc_pq_get_enhance(u32 sel, int *pq_enh);
void de_dcsc_pq_set_enhance(u32 sel, int *pq_enh);

#endif

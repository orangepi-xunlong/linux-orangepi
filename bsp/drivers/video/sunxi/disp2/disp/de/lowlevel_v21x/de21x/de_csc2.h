/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_CSC2_H__
#define __DE_CSC2_H__

#include "de_rtmx.h"

int de_csc2_apply(struct disp_csc_config *config);
s32 de_csc2_enable(u32 disp, u32 chn, u32 csc2_id, u32 type, u8 en);
int de_csc2_set_colormatrix(unsigned int sel, long long *matrix4x3,
							bool is_identity);
s32 de_csc2_get_reg_blocks(u32 disp, struct de_reg_block **blks,
						   u32 *blk_num);
int de_csc2_init(u32 disp, u8 __iomem *de_reg_base);
int de_csc2_exit(void);
int de_csc2_update_regs(unsigned int sel);
int de_csc2_get_config(struct disp_csc_config *config);

void de_csc2_pq_get_enhance(u32 sel, int *pq_enh);
void de_csc2_pq_set_enhance(u32 sel, int *pq_enh);

int de_csc2_coeff_calc(const struct disp_csc_config *config, u32 *mat_sum, const int *mat[]);
int de_csc2_enhance_coeff_calc(struct disp_csc_config *config, u32 *mat_sum, const int *mat[]);

#endif /* __DE_CSC2_H__ */

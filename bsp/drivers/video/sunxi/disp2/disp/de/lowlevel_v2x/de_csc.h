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

#ifndef __DE_CSC_H__
#define __DE_CSC_H__

#include "de_rtmx.h"

int de_dcsc_apply(unsigned int sel, struct disp_csc_config *config);
int de_dcsc_set_colormatrix(unsigned int sel, long long *matrix4x3,
			    bool is_identity);
int de_dcsc_init(struct disp_bsp_init_para *para);
int de_dcsc_exit(void);
int de_dcsc_update_regs(unsigned int sel);
int de_dcsc_get_config(unsigned int sel, struct disp_csc_config *config);

int de_ccsc_apply(unsigned int sel, unsigned int ch_id,
		  struct disp_csc_config *config);
int de_ccsc_update_regs(unsigned int sel);
int de_ccsc_init(struct disp_bsp_init_para *para);
int de_ccsc_exit(void);
int de_csc_coeff_calc(unsigned int infmt, unsigned int incscmod,
		      unsigned int outfmt, unsigned int outcscmod,
		      unsigned int brightness, unsigned int contrast,
		      unsigned int saturation, unsigned int hue,
		      unsigned int out_color_range, int *csc_coeff,
		      bool use_user_matrix);
void de_dcsc_pq_get_enhance(u32 sel, int *pq_enh);
void de_dcsc_pq_set_enhance(u32 sel, int *pq_enh);

#endif

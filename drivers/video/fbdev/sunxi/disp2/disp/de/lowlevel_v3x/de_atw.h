/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __DE_ATW_H__
#define __DE_ATW_H__

#include "de_rtmx.h"

int de_atw_init(unsigned int sel, unsigned int chno, uintptr_t reg_base);
int de_atw_set_coeff_laddr(unsigned int sel, unsigned int chno,
			   unsigned int addr);
int de_atw_disable(unsigned int sel, unsigned int chno);
int de_atw_set_lay_cfg(unsigned int sel, unsigned int chno, unsigned int mode,
		       unsigned int m, unsigned int n,
		       struct __lay_para_t *cfg);
int de_atw_set_lay_laddr(unsigned int sel, unsigned int chno, unsigned char fmt,
			 struct de_rect crop, unsigned int *size,
			 unsigned int *align, unsigned long long *addr,
			 unsigned char *haddr);
int de_atw_set_overlay_size(unsigned int sel, unsigned int chno,
			    unsigned int w, unsigned int h);
int de_atw_set_reg_base(unsigned int sel, unsigned int chno, void *base);
int de_atw_update_regs(unsigned int sel, unsigned int chno);

#endif

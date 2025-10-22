/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2020 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include "../de/disp_private.h"
#include "drv_pq.h"
#include <linux/spinlock.h>

extern void pq_set_matrix(struct matrix4x4 *conig, int choice, int out, int write);

void de_set_pq_reg(unsigned long addr, u32 off, u32 value)
{
	*((unsigned int *)((void *)addr + off)) = value;
	return;
}

void de_get_pq_reg(unsigned long addr, u32 off, u32 *value)
{
	*value = *((unsigned int *)((void *)addr + off));
	return;
}

void pq_set_reg(u32 sel, u32 off, u32 value)
{
#ifdef SUPPORT_PQ
	unsigned long reg_base = 0;
	/* shadow_protect */
	reg_base = disp_al_get_reg_base(sel, &off, 1);
	if (reg_base)
		de_set_pq_reg(reg_base, off, value);
	else
		writel(value, ioremap(off, 4));
#else
	DE_WARN("Platform not support\n");
#endif
}

void pq_get_reg(u32 sel, u32 offset, u32 *value)
{
#ifdef SUPPORT_PQ
	unsigned long reg_base = 0;
	/* shadow_protect */
	reg_base = disp_al_get_reg_base(sel, &offset, 0);
	if (reg_base)
		de_get_pq_reg(reg_base, offset, value);
	else
		*value = readl(ioremap(offset, 4));
#else
	DE_WARN("Platform not support\n");
#endif
}

void pq_get_set_matrix(struct matrix4x4 *conig, int choice, int out, int write)
{
#ifdef SUPPORT_PQ
	pq_set_matrix(conig, choice, out, write);
#else
	DE_WARN("Platform not support\n");
#endif

}
void pq_get_set_enhance(u32 sel, struct color_enhanc *pq_enh, int read)
{
#ifdef SUPPORT_PQ
	struct disp_enhance *enh;
	int enhance[4] = {0};
	if (read) {
		de_dcsc_pq_get_enhance(sel, enhance);
		pq_enh->brightness = enhance[0];
		pq_enh->contrast = enhance[1];
		pq_enh->saturation = enhance[2];
		pq_enh->hue = enhance[3];
	} else {
		enhance[0] = pq_enh->brightness;
		enhance[1] = pq_enh->contrast;
		enhance[2] = pq_enh->saturation;
		enhance[3] = pq_enh->hue;
		de_dcsc_pq_set_enhance(sel, enhance);
		enh = disp_get_enhance(sel);
		enh->pq_update(enh, enhance);
	}
#else
	DE_WARN("Platform not support\n");
#endif
}

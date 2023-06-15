/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2020 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/spinlock.h>
#include "../de/disp_private.h"

static struct color_enhanc pq_enhance= {
	0,0,50,50,50,50
};

void de_set_pq_reg(unsigned long addr, u32 off, u32 value)
{
	pr_info("addr = 0x%lx, off = 0x%x, value = 0x%x\n", addr, off, value);
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
	unsigned long reg_base = 0;
	//shadow_protect
	reg_base = disp_al_get_reg_base(sel, &off, 1);
	pr_info("[%s]+++reg_base = 0x%lx, off = 0x%x\n", __func__, reg_base, off);
	if (reg_base)
		de_set_pq_reg(reg_base, off, value);
	else
		pr_err("Something err,set reg failed\n");

	return;
}

void pq_get_reg(u32 sel, u32 offset, u32 *value)
{
	unsigned long reg_base = 0;
	//shadow_protect
	reg_base = disp_al_get_reg_base(sel, &offset, 0);
	if (reg_base)
		de_get_pq_reg(reg_base, offset, value);
	else
		pr_err("Something err,Get reg failed\n");
}

void pq_get_enhance(struct disp_csc_config *conig)
{
	conig->brightness = pq_enhance.brightness;
	conig->contrast = pq_enhance.contrast;
	conig->saturation = pq_enhance.saturation;
	conig->hue = pq_enhance.hue;
}

void pq_set_enhance(struct color_enhanc *pq_enh, int read)
{
	if (read) {
		*pq_enh = pq_enhance;
	} else {
		pq_enhance = *pq_enh;
	}
}


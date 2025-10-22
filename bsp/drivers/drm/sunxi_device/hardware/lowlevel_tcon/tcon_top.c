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
#include "tcon_top_type.h"
#include "tcon_top.h"

#define TCON_DEVICE_MAX		8

static volatile struct __tcon_top_dev_t *tcon_top[TCON_DEVICE_MAX];//fixme

/**
 * @name       rgb_src_sel(for sun50iw6 soc)
 * @brief      select the source of tcon rgb output
 * @param[IN]  src:0-->tcon_lcd0,1-->tcon_tv0
 * @param[OUT] none
 * @return     return 0 if successfull
 */
s32 tcon_lcd_rgb_src_sel(u32 src)
{
	if (src > 1)
		return -1;
	tcon_top[0]->tcon_tv_setup.bits.rgb0_src_sel = src;
	return 0;
}

void tcon_lcd_dsc_src_sel(void)
{
	tcon_top[0]->dsc_top_ctrl.bits.dsc_slice_sel = 1;
	tcon_top[0]->dsc_top_ctrl.bits.dsc_enable = 1;

	tcon_top[0]->dsc_int_reg.bits.dwc_dsc_err_int_en = 1;
}
/**
 * @name       dsi_src_sel(for sun50iw3 soc)
 * @brief      select the video source of dsi module
 * @param[IN]  sel:dsi module index; src:0 or 1
 * @param[OUT] none
 * @return     return 0 if successful
 */
s32 tcon_lcd_dsi_src_sel(u32 sel, u32 src)
{
	if (src > 1)
		return -1;
	if (sel == 0)
		tcon_top[0]->dsi_src_select.bits.dsi0_src_sel = src;
	else
		tcon_top[0]->dsi_src_select.bits.dsi1_src_sel = src;
	return 0;
}

s32 tcon_lcd_dsi_clk_source(u32 sel, u32 dual_dsi_flag)
{
	u32 en = 1;

	if (sel == 0) {
		tcon_top[0]->tcon_clk_src.bits.lcd0_clk_src = en;
		tcon_top[0]->tcon_clk_src.bits.combo_phy0_clk_src = en;
		if (dual_dsi_flag)
			tcon_top[0]->tcon_clk_src.bits.combo_phy1_clk_src = en;
	} else if (sel == 1) {
		tcon_top[0]->tcon_clk_src.bits.lcd1_clk_src = en;
		tcon_top[0]->tcon_clk_src.bits.combo_phy1_clk_src = en;
	}
	return 0;
}

/* sel: the index of timing controller */
s32 tcon0_out_to_gpio(u32 sel)
{
	if (sel >= TCON_DEVICE_MAX)
		return -1;

	if (sel == 0)
		tcon_top[0]->tcon_tv_setup.bits.tv0_out = LCD_TO_GPIO;
	else if (sel == 1)
		tcon_top[0]->tcon_tv_setup.bits.tv1_out = LCD_TO_GPIO;

	return 0;
}

/* sel: the index of timing controller */
s32 tcon1_out_to_gpio(u32 sel)
{
	if (sel >= TCON_DEVICE_MAX)
		return -1;

	if (sel == 2)
		tcon_top[0]->tcon_tv_setup.bits.tv0_out = TV_TO_GPIO;
	else if (sel == 3)
		tcon_top[0]->tcon_tv_setup.bits.tv1_out = TV_TO_GPIO;

	return 0;
}

/* @sel: the index of timing controller
 * @en:  enable clock or not
 */
s32 tcon1_tv_clk_enable(u32 sel, u32 en)
{
	if (sel >= TCON_DEVICE_MAX)
		return -1;

	if (sel == 2) {
		tcon_top[0]->tcon_tv_setup.bits.tv0_clk_src = TV_CLK_F_TVE;
		tcon_top[0]->tcon_clk_gate.bits.tv0_clk_gate = en;
	} else if (sel == 3) {
		tcon_top[0]->tcon_tv_setup.bits.tv1_clk_src = TV_CLK_F_TVE;
		tcon_top[0]->tcon_clk_gate.bits.tv1_clk_gate = en;
	}

	return 0;
}

/* @sel: the index of timing controller
 * @en:  enable clock or not
 */
s32 tcon1_edp_clk_enable(u32 sel, u32 en)
{
	if (sel >= TCON_DEVICE_MAX)
		return -1;

#if (IS_ENABLED(CONFIG_ARCH_SUN60IW2)) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
		tcon_top[(sel > 2) ? 1 : 0]->tcon_tv_setup.bits.tv1_clk_src = en;
		tcon_top[(sel > 2) ? 1 : 0]->tcon_clk_gate.bits.tv1_clk_gate = en;
#else
	if (en) {
		tcon_top[0]->tcon_tv_setup.bits.tv1_clk_src = TV_CLK_F_TVE;
		tcon_top[0]->tcon_clk_gate.bits.tv1_clk_gate = TV_CLK_F_TVE;
		//tcon_top[0]->tcon_clk_gate.bits.hdmi_src = 2;
	} else {
		tcon_top[0]->tcon_tv_setup.bits.tv1_clk_src = 0;
		tcon_top[0]->tcon_clk_gate.bits.tv1_clk_gate = 0;
		//tcon_top[0]->tcon_clk_gate.bits.hdmi_src = 0;
	}
#endif

	return 0;
}

/**
 * tcon_top_hdmi_set_gate - enable tcon clk output to hdmi
 * @sel: The index of tcon selected for hdmi source
 * @en: Enable or not for tcon
 *
 * Returns 0.
 */
s32 tcon_top_hdmi_set_gate(u32 sel, u32 en)
{
	if (sel >= TCON_DEVICE_MAX)
		return -1;

#if (IS_ENABLED(CONFIG_ARCH_SUN60IW2)) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	tcon_top[(sel > 2) ? 1 : 0]->tcon_clk_gate.sun60i_bits.tv0_clk_gate  = en;
	tcon_top[(sel > 2) ? 1 : 0]->tcon_clk_gate.sun60i_bits.tv0_hdmi_gate = en;
#else
	if (sel == 2)
		tcon_top[0]->tcon_clk_gate.bits.tv0_clk_gate = en;

	if (en) {
		tcon_top[0]->tcon_clk_gate.bits.hdmi_src = 1;
	} else {
		/* disable tcon output to hdmi */
		if (((sel == 2) && (tcon_top[0]->tcon_clk_gate.bits.hdmi_src == 1))) {
			tcon_top[0]->tcon_clk_gate.bits.hdmi_src = 0;
		}
	}
#endif
	return 0;
}

s32 tcon_top_hdmi_set_clk_src(u32 sel, u32 src)
{
#if (IS_ENABLED(CONFIG_ARCH_SUN60IW2)) || IS_ENABLED(CONFIG_ARCH_SUN65IW1)
	tcon_top[sel > 2 ? 1 : 0]->tcon_tv_setup.sun60i_bits.tv0_hdmiphy_ccu_sel = src;
#endif
	return 0;
}

/**
 * tcon0_dsi_clk_enable - enable tcon clk output to dsi
 * @sel: The index of tcon selected for dsi source
 * @en: Enable or not for tcon
 *
 * Returns 0.
 */
s32 tcon_lcd_dsi_clk_enable(u32 sel, u32 en)
{
	/* only tcon0 support dsi on sun8iw11 platform */
	if (sel == 0)
		tcon_top[0]->tcon_clk_gate.bits.dsi0_clk_gate = en;
	else if (sel == 1)
		tcon_top[0]->tcon_clk_gate.bits.dsi1_clk_gate = en;

	return 0;
}
//EXPORT_SYMBOL(tcon_lcd_dsi_clk_enable);
/**
 * tcon_de_attach - attach tcon and de specified by de_index and tcon_index
 * @de_index: The index of de to be attached
 * @tcon_index: The index of tcon to be attached
 *
 * Returns 0 while successful, otherwise returns -1.
 */
s32 tcon_de_attach(u32 tcon_index, u32 de_index)
{
	/* FIXME:: there is no mux in tcon after sun50iw10 */
	return 0;

	if (de_index == 0) {
#if defined(CONFIG_ARCH_SUN50IW6)
		tcon_top[0]->tcon_de_perh.bits.de_port0_perh =
		    (tcon_index == 1) ? 2 : 0;
		tcon_top[0]->tcon_de_perh.bits.de_port1_perh =
		    (tcon_index == 1) ? 0 : 2;
#else
		if (tcon_top[0]->tcon_de_perh.bits.de_port1_perh == tcon_index)
			tcon_top[0]->tcon_de_perh.bits.de_port1_perh =
			    tcon_top[0]->tcon_de_perh.bits.de_port0_perh;

		tcon_top[0]->tcon_de_perh.bits.de_port0_perh = tcon_index;
#endif
	} else if (de_index == 1) {
#if defined(CONFIG_ARCH_SUN50IW6)
		tcon_top[0]->tcon_de_perh.bits.de_port1_perh =
		    (tcon_index == 1) ? 2 : 0;
		tcon_top[0]->tcon_de_perh.bits.de_port0_perh =
		    (tcon_index == 1) ? 0 : 2;
#else
		if (tcon_top[0]->tcon_de_perh.bits.de_port0_perh == tcon_index)
			tcon_top[0]->tcon_de_perh.bits.de_port0_perh =
			    tcon_top[0]->tcon_de_perh.bits.de_port1_perh;

		tcon_top[0]->tcon_de_perh.bits.de_port1_perh = tcon_index;
#endif
	}
	return 0;
}

/**
 * @name       edp_de_attach
 * @brief      attach tcon and de specified by de_index and tcon_index
 * @param[IN]  edp_index:index of edp,start from 0
 * @param[IN]  de_index:index of de,start from 0
 * @return
 */
s32 edp_de_attach(u32 edp_index, u32 de_index)
{
	if (de_index == 0)
		tcon_top[0]->tcon_de_perh.bits.de_port0_perh = edp_index + 8;
	else
		tcon_top[0]->tcon_de_perh.bits.de_port1_perh = edp_index + 8;

	return 0;
}

/**
 * tcon_get_attach_by_de_index - get the index of tcon by de_index
 * @de_index: The index of de to be attached
 *
 * Returns the index of tcon attached with the de specified
 * or -1 while not attach.
 */
s32 tcon_get_attach_by_de_index(u32 de_index)
{
	s32 tcon_index = 0;

	if (de_index == 0)
		tcon_index = tcon_top[0]->tcon_de_perh.bits.de_port0_perh;
	else if (de_index == 1)
		tcon_index = tcon_top[0]->tcon_de_perh.bits.de_port1_perh;

	return tcon_index;
}

s32 tcon_top_set_reg_base(u32 sel, uintptr_t base)
{
	tcon_top[sel] = (struct __tcon_top_dev_t *) (uintptr_t) (base);
	return 0;
}

uintptr_t tcon_top_get_reg_base(u32 sel)
{
	return (uintptr_t) tcon_top[sel];
}

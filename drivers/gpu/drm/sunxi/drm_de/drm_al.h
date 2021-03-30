/*
 * Copyright (C) 2016 Allwinnertech Co.Ltd
 * Authors: Jet Cui
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
/*de_al is the file contans the ops for the Display Engine hardware opration*/
#ifndef _SUNXI_DE_AL_H_
#define _SUNXI_DE_AL_H_
#include "de/include.h"

#if defined(CONFIG_ARCH_SUN50IW1P1)
#include "de/lowlevel_sun50iw1/de_hal.h"
#include "de/lowlevel_sun50iw1/de_clock.h"
#include "de/lowlevel_sun50iw1/de_lcd.h"
#include "de/lowlevel_sun50iw1/disp_al.h"
#include "de/lowlevel_sun50iw1/de_dsi.h"
#elif defined(CONFIG_ARCH_SUN8IW10)
#include "de/lowlevel_sun8iw10/de_hal.h"
#include "de/lowlevel_sun8iw10/de_clock.h"
#include "de/lowlevel_sun8iw10/de_lcd.h"
#include "de/lowlevel_sun8iw10/disp_al.h"
#include "de/lowlevel_sun8iw10/de_dsi.h"
#elif defined(CONFIG_ARCH_SUN8IW11)
#include "de/lowlevel_v2x/de_hal.h"
#include "de/lowlevel_v2x/de_clock.h"
#include "de/lowlevel_v2x/de_lcd.h"
#include "de/lowlevel_v2x/disp_al.h"
#include "de/lowlevel_v2x/de_dsi.h"
#elif defined(CONFIG_ARCH_SUN50IW2P1)
#include "de/lowlevel_v2x/de_hal.h"
#include "de/lowlevel_v2x/de_clock.h"
#include "de/lowlevel_v2x/de_lcd.h"
#include "de/lowlevel_v2x/disp_al.h"
#include "de/lowlevel_v2x/de_dsi.h"
#else
#error "undefined platform!!!"
#endif

#include "sunxi_drm_crtc.h"
#include "sunxi_drm_connector.h"

void sunxi_drm_crtc_clk_disable(int nr);

void sunxi_drm_crtc_clk_enable(int nr);

int sunxi_drm_get_max_connector(void);

int sunxi_drm_get_max_encoder(void);

int sunxi_drm_get_num_chns_by_crtc(int nr);

int sunxi_drm_get_max_crtc(void);

void sunxi_drm_crtc_updata_fps(int crtc, int fps);

int sunxi_drm_get_crtc_pipe_plane(int crtc, int channel);

bool sunxi_drm_init_al(disp_bsp_init_para * para);

int sunxi_drm_updata_reg(int crtc);

int sunxi_drm_sync_reg(int crtc);

int sunxi_drm_apply_cache(int crtc, struct disp_manager_data *data);

int sunxi_drm_get_plane_by_crtc(int crtc);

int sunxi_drm_get_vi_pipe_by_crtc(int crtc);

int sunxi_drm_encoder_support(int encoder, unsigned int output_type);

void sunxi_updata_crtc_freq(unsigned long rate);

void sunxi_drm_updata_crtc(struct sunxi_drm_crtc *sunxi_crtc,
	struct sunxi_drm_connector *connector);

#endif

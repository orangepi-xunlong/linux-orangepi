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
#ifndef _SUNXI_DRM_CORE_H_
#define _SUNXI_DRM_CORE_H_

#include <de/include.h>
#include <linux/types.h>
#include <sunxi_drm_crtc.h>
#include <video/sunxi_display2.h>

enum {
    QUERY_VSYNC_IRQ = 0,
};

struct sunxi_hardware_ops {
	bool    (*init)(void *init_data);
	bool    (*reset)(void *init_data);
	bool    (*enable)(void *enable_data);
	bool    (*disable)(void *disable_data);
	bool    (*updata_reg)(void *commit_data);
	void    (*vsync_proc)(void *irq_data);
	bool    (*irq_query)(void *irq_data, int need_irq);
	void    (*vsync_delayed_do)(void *data);
	int     (*set_timming)(void *data, struct drm_display_mode *mode);
	int     (*user_def)(void *data);
};

struct sunxi_hardware_res {
	disp_mod_id res_id;

	struct clk *clk;
	unsigned long clk_rate;
	struct clk *parent_clk;
	unsigned long parent_clk_rate;

	bool clk_enable;
	bool irq_enable;
	bool irq_uesd;
	bool en_ctl_by;
	irq_handler_t irq_handle;
	unsigned int irq_no;
	void *irq_arg;
	uintptr_t reg_base;
	struct sunxi_hardware_ops *ops;
	void *private;
};

struct sunxi_panel *sunxi_lcd_init(struct sunxi_hardware_res *hw_res, int panel_id, int lcd_id);

int sunxi_drm_init(struct drm_device *dev);

void sunxi_drm_destroy(struct drm_device *dev);

void sunxi_hwres_destroy(struct sunxi_hardware_res *res);

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2023 Allwinnertech Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _sunxi_drm_debug_h_
#define _sunxi_drm_debug_h_

#include <linux/platform_device.h>

void sunxidrm_debug_init(struct platform_device *pdev);
void sunxidrm_debug_term(void);
void sunxidrm_debug_trace_begin(u32 crtc);
void sunxidrm_debug_trace_frame(u32 crtc, u32 plane, struct drm_framebuffer *fb);
void sunxidrm_debug_trace_framebuffer_unmap(struct drm_framebuffer *fb);
int sunxidrm_debug_show(struct seq_file *sfile, void *offset);

#endif

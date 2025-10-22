/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs display driver.
 *
 * Copyright (C) 2022 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __FB_PLATFORM_H__
#define __FB_PLATFORM_H__

#include "fb_top.h"

int platform_get_private_size(void);
int platform_format_to_var(enum disp_pixel_format format, struct fb_var_screeninfo *var);
int platform_get_timing(void *hw_info, struct fb_var_screeninfo *var);
int platform_get_physical_size(void *hw_info, struct fb_var_screeninfo *var);
int platform_update_fb_output(void *hw_info, const struct fb_var_screeninfo *var);
int platform_fb_mmap(void *hw_info, struct vm_area_struct *vma);
int platform_fb_init_logo(void *hw_info, const struct fb_var_screeninfo *var);
int platform_fb_memory_alloc(void *hw_info, char **vir_addr, unsigned long long *device_addr, unsigned int size);
int platform_fb_memory_free(void *hw_info);
int platform_fb_pan_display_post_proc(void *hw_info);
int platform_fb_set_blank(void *hw_info, bool is_blank);
int platform_fb_init(struct fb_info_share *fb_init);
int platform_fb_post_init(void);
int platform_fb_init_for_each(void *hw_info, struct fb_init_info_each *fb, void **pseudo_palette, int fb_id);
int platform_fb_exit(void);
int platform_format_get_bpp(enum disp_pixel_format format);
void platform_get_fb_buffer_size_from_config(int hw_id, enum disp_output_type output_type,
						unsigned int output_mode, int *width, int *height);
int platform_fb_g2d_rot_init(void *hw_info);
int platform_fb_g2d_rot_exit(void *hw_info);
int platform_fb_exit(void);
int platform_fb_exit_for_each(void *hw_info);
int platform_fb_post_exit(void);
bool platform_fb_check_rotate(__u32 *fb_rotate);
int platform_fb_set_rotate(void *hw_info, int rot);
int platform_swap_rb_order(struct fb_info *info, bool swap);
struct dma_buf *platform_fb_get_dmabuf(void *hw_info);

#endif

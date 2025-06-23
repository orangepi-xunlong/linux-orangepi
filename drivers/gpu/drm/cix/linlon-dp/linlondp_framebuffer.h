/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _LINLONDP_FRAMEBUFFER_H_
#define _LINLONDP_FRAMEBUFFER_H_

#include <drm/drm_framebuffer.h>
#include "linlondp_format_caps.h"

/**
 * struct linlondp_fb - Entending drm_framebuffer with linlondp attribute
 */
struct linlondp_fb {
    /** @base: &drm_framebuffer */
    struct drm_framebuffer base;
    /**
     * @format_caps:
     * extends drm_format_info for linlondp specific information
     */
    const struct linlondp_format_caps *format_caps;
    /** @is_va: if smmu is enabled, it will be true */
    bool is_va;
    /** @aligned_w: aligned frame buffer width */
    u32 aligned_w;
    /** @aligned_h: aligned frame buffer height */
    u32 aligned_h;
    /** @afbc_size: minimum size of afbc */
    u32 afbc_size;
    /** @offset_payload: start of afbc body buffer */
    u32 offset_payload;
};

#define to_kfb(dfb)    container_of(dfb, struct linlondp_fb, base)

struct drm_framebuffer *
linlondp_fb_create(struct drm_device *dev, struct drm_file *file,
        const struct drm_mode_fb_cmd2 *mode_cmd);
int linlondp_fb_check_src_coords(const struct linlondp_fb *kfb,
                   u32 src_x, u32 src_y, u32 src_w, u32 src_h);
dma_addr_t
linlondp_fb_get_pixel_addr(struct linlondp_fb *kfb, int x, int y, int plane);
bool linlondp_fb_is_layer_supported(struct linlondp_fb *kfb, u32 layer_type,
        u32 rot);

#endif

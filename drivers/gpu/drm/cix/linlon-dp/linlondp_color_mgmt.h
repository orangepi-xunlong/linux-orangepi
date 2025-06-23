/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */

#ifndef _LINLONDP_COLOR_MGMT_H_
#define _LINLONDP_COLOR_MGMT_H_

#include <drm/drm_color_mgmt.h>
#include "linlondp_coeffs.h"

#define LINLONDP_N_YUV2RGB_COEFFS        12
#define LINLONDP_N_RGB2YUV_COEFFS        12
#define LINLONDP_COLOR_PRECISION        12
#define LINLONDP_N_GAMMA_COEFFS        65
#define LINLONDP_COLOR_LUT_SIZE        BIT(LINLONDP_COLOR_PRECISION)
#define LINLONDP_N_CTM_COEFFS        12

struct linlondp_color_manager {
    struct linlondp_coeffs_manager *igamma_mgr;
    struct linlondp_coeffs_manager *fgamma_mgr;
    bool has_ctm;
};

struct linlondp_color_state {
    struct linlondp_coeffs_table *igamma;
    struct linlondp_coeffs_table *fgamma;
};

void linlondp_color_duplicate_state(struct linlondp_color_state *new,
        struct linlondp_color_state *old);
void linlondp_color_cleanup_state(struct linlondp_color_state *color_st);
int linlondp_color_validate(struct linlondp_color_manager *mgr,
        struct linlondp_color_state *st,
        struct drm_property_blob *igamma_blob,
        struct drm_property_blob *fgamma_blob);

void drm_lut_to_coeffs(struct drm_property_blob *lut_blob,
        u32 *coeffs, bool igamma);
void drm_lut_to_fgamma_coeffs(struct drm_property_blob *lut_blob, u32 *coeffs);
void drm_ctm_to_coeffs(struct drm_property_blob *ctm_blob, u32 *coeffs);

const s32 *linlondp_select_yuv2rgb_coeffs(u32 color_encoding, u32 color_range);
const s32 *linlondp_select_rgb2yuv_coeffs(u32 color_encoding, u32 color_range);

#endif /*_LINLONDP_COLOR_MGMT_H_*/

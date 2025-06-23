// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#include <drm/drm_print.h>

#include "linlondp_utils.h"
#include "linlondp_color_mgmt.h"
#include "include/linlondp_drm.h"

/* 10bit precision YUV2RGB matrix */
static const s32 yuv2rgb_bt601_narrow[LINLONDP_N_YUV2RGB_COEFFS] = {
    1192,    0, 1634,
    1192, -401, -832,
    1192, 2066,    0,
      64,  512,  512
};

static const s32 yuv2rgb_bt601_wide[LINLONDP_N_YUV2RGB_COEFFS] = {
    1024,    0, 1436,
    1024, -352, -731,
    1024, 1815,    0,
       0,  512,  512
};

static const s32 yuv2rgb_bt709_narrow[LINLONDP_N_YUV2RGB_COEFFS] = {
    1192,    0, 1836,
    1192, -218, -546,
    1192, 2163,    0,
      64,  512,  512
};

static const s32 yuv2rgb_bt709_wide[LINLONDP_N_YUV2RGB_COEFFS] = {
    1024,    0, 1613,
    1024, -192, -479,
    1024, 1900,    0,
       0,  512,  512
};

static const s32 yuv2rgb_bt2020[LINLONDP_N_YUV2RGB_COEFFS] = {
    1024,    0, 1476,
    1024, -165, -572,
    1024, 1884,    0,
       0,  512,  512
};

static const s32 rgb2yuv_bt601_narrow[LINLONDP_N_RGB2YUV_COEFFS] = {
    1052,  2065,  401,
    -607, -1192, 1799,
    1799, -1506, -293,
     256,  2048, 2048
};

static const s32 rgb2yuv_bt601_wide[LINLONDP_N_RGB2YUV_COEFFS] = {
    1225,  2404,  467,
    -691, -1357, 2048,
    2048, -1715, -333,
       0,  2048, 2048
};

static const s32 rgb2yuv_bt709_narrow[LINLONDP_N_RGB2YUV_COEFFS] = {
     748,  2516,  254,
    -412, -1387, 1799,
    1799, -1634, -165,
     256,  2048, 2048
};

static const s32 rgb2yuv_bt709_wide[LINLONDP_N_RGB2YUV_COEFFS] = {
     871,  2929,  296,
    -469, -1579, 2048,
    2048, -1860, -188,
       0,  2048, 2048
};

const s32 *linlondp_select_yuv2rgb_coeffs(u32 color_encoding, u32 color_range)
{
    bool narrow = color_range == DRM_COLOR_YCBCR_LIMITED_RANGE;
    const s32 *coeffs;

    switch (color_encoding) {
    case DRM_COLOR_YCBCR_BT709:
        coeffs = narrow ? yuv2rgb_bt709_narrow : yuv2rgb_bt709_wide;
        break;
    case DRM_COLOR_YCBCR_BT601:
        coeffs = narrow ? yuv2rgb_bt601_narrow : yuv2rgb_bt601_wide;
        break;
    case DRM_COLOR_YCBCR_BT2020:
        coeffs = yuv2rgb_bt2020;
        break;
    default:
        coeffs = NULL;
        break;
    }

    return coeffs;
}

const s32 *linlondp_select_rgb2yuv_coeffs(u32 color_encoding, u32 color_range)
{
    const s32 *coeffs = NULL;
    bool narrow = color_range == DRM_COLOR_YCBCR_LIMITED_RANGE;

    switch (color_encoding) {
        case DRM_COLOR_YCBCR_BT709:
            coeffs = narrow ? rgb2yuv_bt709_narrow : rgb2yuv_bt709_wide;
            break;
        case DRM_COLOR_YCBCR_BT601:
            coeffs = narrow ? rgb2yuv_bt601_narrow : rgb2yuv_bt601_wide;
            break;
        default:
            coeffs = NULL;
            break;
    }
    return coeffs;
}

struct gamma_curve_sector {
    u32 boundary_start;
    u32 num_of_segments;
    u32 segment_width;
};

struct gamma_curve_segment {
    u32 start;
    u32 end;
};

static struct gamma_curve_sector fgamma_sector_tbl[] = {
    { 0,    4,  4   },
    { 16,   4,  4   },
    { 32,   4,  8   },
    { 64,   4,  16  },
    { 128,  4,  32  },
    { 256,  4,  64  },
    { 512,  16, 32  },
    { 1024, 24, 128 },
};

static struct gamma_curve_sector igamma_sector_tbl[] = {
    {0, 64, 64},
};

/**
 * linlondp_color_lut_extract - clamp and round LUT entries
 * @user_input: input value
 * @bit_precision: number of bits the hw LUT supports
 *
 * Extract a degamma/gamma LUT value provided by user (in the form of
 * &drm_color_lut entries) and round it to the precision supported by the
 * hardware.
 */
static uint32_t linlondp_color_lut_extract(uint32_t user_input, uint32_t bit_precision)
{
    uint32_t val = user_input;
    uint32_t max = 0xffff >> (16 - bit_precision);

    /* Round only if we're not using full precision. */
    if (bit_precision < 16) {
        val += 1UL << (16 - bit_precision - 1);
        val >>= 16 - bit_precision;
    }

    return clamp_val(val, 0, max);
}

/*

static struct gamma_curve_sector sector_tbl[] = {
    { 0,    4,  4   },
    { 16,   4,  4   },
    { 32,   4,  8   },
    { 64,   4,  16  },
    { 128,  4,  32  },
    { 256,  4,  64  },
    { 512,  16, 32  },
    { 1024, 24, 128 },
};

static void
drm_lut_to_coeffs(struct drm_property_blob *lut_blob, u32 *coeffs,
          struct gamma_curve_sector *sector_tbl, u32 num_sectors)
{
    struct drm_color_lut *lut;
    u32 i, j, in, num = 0;

    if (!lut_blob)
        return;

    lut = lut_blob->data;

    for (i = 0; i < num_sectors; i++) {
        for (j = 0; j < sector_tbl[i].num_of_segments; j++) {
            in = sector_tbl[i].boundary_start +
                 j * sector_tbl[i].segment_width;

            coeffs[num++] = drm_color_lut_extract(lut[in].red,
                        LINLONDP_COLOR_PRECISION);
        }
    }

    coeffs[num] = BIT(LINLONDP_COLOR_PRECISION);
}
*/
void drm_lut_to_coeffs(struct drm_property_blob *lut_blob,
               u32 *coeffs, bool igamma)
{
    struct gamma_curve_sector *sector_tbl;
    struct drm_color_lut *lut;
    u32 i, j, in, num = 0, num_sectors;

    if (!lut_blob)
        return;

    lut = lut_blob->data;

    sector_tbl = igamma ? igamma_sector_tbl : fgamma_sector_tbl;
    num_sectors = igamma ? ARRAY_SIZE(igamma_sector_tbl) :
                   ARRAY_SIZE(fgamma_sector_tbl);

    for (i = 0; i < num_sectors; i++) {
        for (j = 0; j < sector_tbl[i].num_of_segments; j++) {
            in = sector_tbl[i].boundary_start +
                 j * sector_tbl[i].segment_width;

            coeffs[num++] = linlondp_color_lut_extract(lut[in].red,
                        LINLONDP_COLOR_PRECISION);
        }
    }

    coeffs[num] = BIT(LINLONDP_COLOR_PRECISION);
}


void drm_lut_to_fgamma_coeffs(struct drm_property_blob *lut_blob, u32 *coeffs)
{
    drm_lut_to_coeffs(lut_blob, coeffs, false);
}

void drm_ctm_to_coeffs(struct drm_property_blob *ctm_blob, u32 *coeffs)
{
    struct color_ctm_ext* ctm_ext;
    u32 i;

    if (!ctm_blob)
        return;

    ctm_ext = ctm_blob->data;

    for (i = 0; i < LINLONDP_N_CTM_COEFFS; i++)
        coeffs[i] = drm_color_ctm_s31_32_to_qm_n(ctm_ext->matrix[i], 3, 12);
}

void linlondp_color_duplicate_state(struct linlondp_color_state *new,
        struct linlondp_color_state *old)
{
    new->igamma = linlondp_coeffs_get(old->igamma);
    new->fgamma = linlondp_coeffs_get(old->fgamma);
}

void linlondp_color_cleanup_state(struct linlondp_color_state *color_st)
{
    linlondp_coeffs_put(color_st->igamma);
    linlondp_coeffs_put(color_st->fgamma);
}

int linlondp_color_validate(struct linlondp_color_manager *mgr,
              struct linlondp_color_state *st,
              struct drm_property_blob *igamma_blob,
              struct drm_property_blob *fgamma_blob)
{
    u32 coeffs[LINLONDP_N_GAMMA_COEFFS];

    linlondp_color_cleanup_state(st);

    if (igamma_blob) {
        drm_lut_to_coeffs(igamma_blob, coeffs, true);
        st->igamma = linlondp_coeffs_request(mgr->igamma_mgr, coeffs);
        if (!st->igamma) {
            DRM_DEBUG_ATOMIC("request igamma table failed.\n");
            return -EBUSY;
        }
    }

    if (fgamma_blob) {
        drm_lut_to_coeffs(fgamma_blob, coeffs, false);
        st->fgamma = linlondp_coeffs_request(mgr->fgamma_mgr, coeffs);
        if (!st->fgamma) {
            DRM_DEBUG_ATOMIC("request fgamma table failed.\n");
            return -EBUSY;
        }
    }

    return 0;
}


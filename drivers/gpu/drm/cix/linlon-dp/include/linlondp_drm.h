/*
 *
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 */

#ifndef _LINLONDP_DRM_H_
#define _LINLONDP_DRM_H_

enum linlondp_hdr_eotf {
    LINLONDP_HDR_NONE,
    LINLONDP_HDR_ST2084,
    LINLONDP_HDR_HLG,
};

struct color_ctm_ext {
    uint64_t matrix[12];
};
#endif

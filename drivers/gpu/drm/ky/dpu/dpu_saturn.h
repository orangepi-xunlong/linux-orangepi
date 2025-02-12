// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _DPU_SATURN_H_
#define _DPU_SATURN_H_

#include "saturn_regs/reg_map.h"
#include "../ky_dpu.h"

#ifndef min
#define min(x, y) (((x)<(y))?(x):(y))
#endif

#ifndef max
#define max(x, y) (((x)>(y))?(x):(y))
#endif

#ifndef clip
#define clip(x, a, b) (max(a, min(x, b)))
#endif

/* Supported variants of the hardware */
enum {
	SATURN_HDMI = 0,
	SATURN_LE,
	/* keep the next entry last */
	DP_MAX_DEVICES
};

struct dpu_format_id {
	u32 format;		/* DRM fourcc */
	u8 id;			/* used internally */
	u8 bpp;			/* bit per pixel */
};

#define KY_DPU_INVALID_FORMAT_ID	0xff

enum format_features {
	FORMAT_RGB  = BIT(0),
	FORMAT_RAW_YUV  = BIT(1),
	FORMAT_AFBC = BIT(2),
};

enum rotation_features {
	ROTATE_COMMON = BIT(0),		/* supports rotation on raw and afbc 0/180, x and y */
	ROTATE_RAW_90_270 = BIT(1),	/* supports rotation on raw 90/270 */
	ROTATE_AFBC_90_270 = BIT(2),	/* supports rotation on afbc 90/270 */
};

struct ky_hw_rdma {
	u16 formats;
	u16 rots;
};

extern const u32 saturn_fbcmem_sizes[2];
extern const u32 saturn_le_fbcmem_sizes[2];

void ky_plane_update_hw_channel(struct drm_plane *plane);
void ky_update_hdr_matrix(struct drm_plane *plane, struct ky_plane_state *ky_pstate);
void ky_update_csc_matrix(struct drm_plane *plane, struct drm_plane_state *old_state);
void ky_plane_disable_hw_channel(struct drm_plane *plane, struct drm_plane_state *old_state);
void saturn_conf_dpuctrl_color_matrix(struct ky_dpu *dpu, struct drm_crtc_state *old_state);
u8 ky_plane_hw_get_format_id(u32 format);
extern struct ky_hw_device ky_dp_devices[DP_MAX_DEVICES];
#endif


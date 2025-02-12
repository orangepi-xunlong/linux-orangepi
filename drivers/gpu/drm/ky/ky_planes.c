// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <drm/drm_atomic.h>
#include <drm/drm_framebuffer.h>
#include <drm/drm_blend.h>
#include <drm/drm_atomic_uapi.h>
#include <drm/drm_atomic_helper.h>
#include <drm/drm_plane_helper.h>
#include <drm/drm_color_mgmt.h>
#include <linux/component.h>
#include <linux/dma-mapping.h>
#include "ky_cmdlist.h"
#include "ky_dmmu.h"
#include "ky_dpu.h"
#include "ky_drm.h"
#include "ky_gem.h"
#include "ky_lib.h"
#include "dpu/dpu_saturn.h"
#include "dpu/dpu_trace.h"

//as per weston requirement, suppoted modifiers need to be reported to user space,
//which is different from sf
static const uint64_t supported_format_modifiers[] = {
	DRM_FORMAT_MOD_LINEAR,
	DRM_FORMAT_MOD_INVALID
};

struct ky_plane *to_ky_plane(struct drm_plane *plane)
{
	return container_of(plane, struct ky_plane, plane);
}

static int ky_plane_check_rdma(const struct ky_hw_rdma *rdma, u32 rdma_id, struct drm_plane_state *state)
{
	unsigned int rot = state->rotation;
	unsigned int zpos = state->zpos;
	u32 format = state->fb->format->format;
	u16 hw_formats = rdma[rdma_id].formats;
	u16 hw_rots = rdma[rdma_id].rots;
	bool afbc = (state->fb->modifier > 0);
	const struct drm_format_info *info = drm_format_info(format);
	struct ky_dpu *dpu = crtc_to_dpu(state->crtc);

	trace_ky_plane_check_rdma(dpu->dev_id);
	if ((info->is_yuv) && !afbc && ((hw_formats & FORMAT_RAW_YUV) == 0)) {
		DRM_DEBUG("rdma%d doesn't support RAW YUV format with zpos%d!\n", rdma_id, zpos);
		return -EINVAL;
	}

	if (afbc && ((hw_formats & FORMAT_AFBC) == 0)) {
		DRM_DEBUG("rdma%d doesn't support AFBC format with zpos%d!\n", rdma_id, zpos);
		return -EINVAL;
	}

	if (rot == DRM_MODE_ROTATE_90 || rot == DRM_MODE_ROTATE_270) {
		if (afbc) {
			if ((hw_rots & ROTATE_AFBC_90_270) == 0) {
				DRM_DEBUG("rdma%d doesn't support AFBC 90/270 rotation with zpos%d\n", rdma_id, zpos);
				return -EINVAL;
			}
		} else {
			if ((hw_rots & ROTATE_RAW_90_270) == 0) {
				DRM_DEBUG("rdma%d doesn't support RAW 90/270 rotation with zpos%d!\n", rdma_id, zpos);
				return -EINVAL;
			}
		}
	} else {
		if ((hw_rots & ROTATE_COMMON) == 0) {
			DRM_DEBUG("rdma%d doesn't support common rotation with zpos%d!\n", rdma_id, zpos);
			return -EINVAL;
		}
	}

	return 0;
}

static int ky_plane_atomic_check_hdr_coefs (struct drm_plane *plane,
					  struct drm_plane_state *state)
{
	struct ky_plane_state *apstate = to_ky_plane_state(state);
	struct drm_property_blob *blob;
	struct ky_drm_private *priv = plane->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	int size = hwdev->hdr_coef_size;
	int *coef_data;
	int n = 0;

	if ((apstate->hdr_coefs_blob_prop)){
		blob = apstate->hdr_coefs_blob_prop;
		coef_data = (int *)blob->data;

		for (n = 0; n < size; n++){
			if ((coef_data[n] > 65535) || (coef_data[n] < 0)){
				DRM_ERROR("HDR coef is invalid %d\n", coef_data[n]);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static int ky_plane_atomic_check_scale_coefs (struct drm_plane *plane,
					  struct drm_plane_state *state)
{
	struct ky_plane_state *apstate = to_ky_plane_state(state);
	struct drm_property_blob *blob;
	struct ky_drm_private *priv = plane->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	int size = hwdev->scale_coef_size;
	int *coef_data;
	int n = 0;

	if ((apstate->scale_coefs_blob_prop)){
		blob = apstate->scale_coefs_blob_prop;
		coef_data = (int *)blob->data;

		for (n = 0; n < size; n++){
			if ((coef_data[n] > 32766) || (coef_data[n] < -32767)){
				DRM_ERROR("scale coef is invalid %d\n", coef_data[n]);
				return -EINVAL;
			}
		}
	}

	return 0;
}

static int ky_plane_atomic_check(struct drm_plane *plane,
				  struct drm_atomic_state *atomic_state)
{
	struct drm_plane_state *state = drm_atomic_get_new_plane_state(atomic_state, plane);
	struct drm_framebuffer *fb = state->fb;
	u16 pixel_alpha = state->pixel_blend_mode;
	u32 src_w, src_h, src_x, src_y;
	u32 crtc_w, crtc_h;
	struct ky_plane_state *cur_state = to_ky_plane_state(state);
	u32 cur_rdma_id = cur_state->rdma_id;
	struct ky_drm_private *priv = plane->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	const struct ky_hw_rdma *rdmas = hwdev->rdmas;
	struct ky_dpu *dpu = NULL;

	if (!state->crtc || WARN_ON(!state->fb)) {
		return 0;
	}

	dpu = crtc_to_dpu(state->crtc);
	trace_ky_plane_atomic_check(dpu->dev_id);

	src_x = state->src_x >> 16;
	src_y = state->src_y >> 16;
	src_w = state->src_w >> 16;
	src_h = state->src_h >> 16;
	crtc_w = state->crtc_w;
	crtc_h = state->crtc_h;

	/* For multi planes, only support planes with its offsets is set */
	if (fb->format->num_planes > 3 ||
	   (fb->format->num_planes > 2 && fb->offsets[2] == 0) ||
	   (fb->format->num_planes > 1 && fb->offsets[1] == 0)) {
		DRM_ERROR("%s, Unsupported plane format: plane_num:%d offsets[1]:%d offsets[2]:%d\n", \
			  __func__, fb->format->num_planes, fb->offsets[1], fb->offsets[2]);
		return -EINVAL;
	}

	if (fb->format->num_planes > 1 && fb->modifier) {
		DRM_ERROR("%s, Unsupported afbc with plane_num:%d\n", __func__, fb->format->num_planes);
		return -EINVAL;
	}

	if (fb->format->format == DRM_FORMAT_NV12 || fb->format->format == DRM_FORMAT_YUV420_8BIT) {
		if (src_x % 2 || src_y % 2 || src_w % 2 || src_h % 2) {
			DRM_ERROR("YUV420 coordinations must be even! src_x:%d src_y:%d \
				  src_w:%d src_h:%d\n", src_x, src_y, src_w, src_h);
			return -EINVAL;
		}
	}

	/* adjust rdma id */
	if (src_w == 0 && src_h == 0)
		cur_rdma_id = RDMA_INVALID_ID;
	else {
		/* In case the userspace hasn't set rdma id */
		if (cur_rdma_id == RDMA_INVALID_ID)
			cur_rdma_id = state->zpos;
	}
	cur_state->rdma_id = cur_rdma_id;

	/* Skip solid color */
	if (cur_rdma_id != RDMA_INVALID_ID) {
		if (src_w != crtc_w || src_h != crtc_h)
			cur_state->use_scl = true;
		else
			cur_state->use_scl = false;
		if (cur_rdma_id < hwdev->rdma_nums) {
			if (ky_plane_check_rdma(rdmas, cur_rdma_id, state))
				return -EINVAL;
		} else {
			DRM_ERROR("Invalid rdma id:%d\n", cur_rdma_id);
			return -EINVAL;
		}

		if (dpu->core->cal_layer_fbcmem_size(plane, state)) {
			DRM_ERROR("plane:%d Invalid fbcmem size\n", state->zpos);
			return -EINVAL;
		}

		if(dpu->core->calc_plane_mclk_bw(plane, state)) {
			DRM_INFO("plane:%d unsupported mclk or bandwidth\n", state->zpos);
			return -EINVAL;
		}
	}

	if (ky_plane_atomic_check_hdr_coefs(plane, state)){
		DRM_ERROR("The value of hdr coef is invalid\n");
		return -EINVAL;
	}

	if (ky_plane_atomic_check_scale_coefs(plane, state)){
		DRM_ERROR("The value of scale coef is invalid\n");
		return -EINVAL;
	}

	/* HW can't support plane + pixel blending */
	if ((state->alpha != DRM_BLEND_ALPHA_OPAQUE) &&
	    (pixel_alpha != DRM_MODE_BLEND_PIXEL_NONE) &&
	    fb->format->has_alpha) {
		DRM_ERROR("Can't support mixed blend mode!\n");
		return -EINVAL;
	}

	cur_state->format = ky_plane_hw_get_format_id(fb->format->format);
	if (cur_state->format == KY_DPU_INVALID_FORMAT_ID) {
		DRM_ERROR("Can't support format:0x%x\n", fb->format->format);
		return -EINVAL;
	}

	/* Use default values as they are not actually used now  */
	cur_state->right_image = 0;
	cur_state->is_offline = 1;
	return 0;
}

static void ky_plane_atomic_update(struct drm_plane *plane,
				    struct drm_atomic_state *state)
{
	int ret = 0;
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(state, plane);
	struct ky_dpu *dpu = crtc_to_dpu(plane->state->crtc);
	struct ky_plane_state *ky_pstate = to_ky_plane_state(plane->state);
	struct ky_drm_private *priv = plane->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	u32 rdma_id = ky_pstate->rdma_id;

	struct drm_crtc *dpu_crtc = &dpu->crtc;
	struct drm_display_mode *mode = NULL;
	u32 hdisplay;
	u32 vdisplay;
	u32 src_x, src_y, src_w, src_h;
	u32 crtc_x, crtc_y, crtc_w, crtc_h;

	DRM_DEBUG("%s()\n", __func__);
	trace_ky_plane_atomic_update(dpu->dev_id);

	mode = &dpu_crtc->mode;
	hdisplay = mode->hdisplay;
	vdisplay = mode->vdisplay;

	src_w = plane->state->src_w >> 16;
	src_h = plane->state->src_h >> 16;
	src_x = plane->state->src_x >> 16;
	src_y = plane->state->src_y >> 16;

	crtc_w = plane->state->crtc_w;
	crtc_h = plane->state->crtc_h;
	crtc_x = plane->state->crtc_x;
	crtc_y = plane->state->crtc_y;

	ky_pstate->screen_width = hdisplay;
	ky_pstate->screen_height = vdisplay;

	if ((plane->type == DRM_PLANE_TYPE_CURSOR) && ((crtc_x + crtc_w) > hdisplay)) {

		if ((crtc_x > (hdisplay - crtc_w)) && (crtc_x <= (hdisplay - ((crtc_w * 3) / 4)))) {
			ky_pstate->src_crop_x = src_x;
			ky_pstate->src_crop_y = src_y;
			ky_pstate->src_crop_w = ((src_w * 3) / 4);
			ky_pstate->src_crop_h = src_h;
			ky_pstate->dst_crop_x = crtc_x;
			ky_pstate->dst_crop_y = crtc_y;
			ky_pstate->dst_crop_w = ((crtc_w * 3) / 4);
			ky_pstate->dst_crop_h = crtc_h;
			ky_pstate->is_crop = true;
		} else if ((crtc_x > (hdisplay - ((crtc_w * 3) / 4))) && (crtc_x <= (hdisplay - (crtc_w  / 2)))) {
			ky_pstate->src_crop_x = src_x;
			ky_pstate->src_crop_y = src_y;
			ky_pstate->src_crop_w = (src_w / 2);
			ky_pstate->src_crop_h = src_h;
			ky_pstate->dst_crop_x = crtc_x;
			ky_pstate->dst_crop_y = crtc_y;
			ky_pstate->dst_crop_w = (crtc_w / 2);
			ky_pstate->dst_crop_h = crtc_h;
			ky_pstate->is_crop = true;
		} else if ((crtc_x > (hdisplay - (crtc_w / 2))) && (crtc_x <= (hdisplay - (crtc_w / 4)))){
			ky_pstate->src_crop_x = src_x;
			ky_pstate->src_crop_y = src_y;
			ky_pstate->src_crop_w = (src_w / 4);
			ky_pstate->src_crop_h = src_h;
			ky_pstate->dst_crop_x = crtc_x;
			ky_pstate->dst_crop_y = crtc_y;
			ky_pstate->dst_crop_w = (crtc_w / 4);
			ky_pstate->dst_crop_h = crtc_h;
			ky_pstate->is_crop = true;
		} else if ((crtc_x > (hdisplay - (crtc_w / 4))) && (crtc_x < (hdisplay - (crtc_w / 16)))) {
			ky_pstate->src_crop_x = src_x;
			ky_pstate->src_crop_y = src_y;
			ky_pstate->src_crop_w = (src_w / 4);
			ky_pstate->src_crop_h = src_h;
			ky_pstate->dst_crop_x = hdisplay - (crtc_w / 4);
			ky_pstate->dst_crop_y = crtc_y;
			ky_pstate->dst_crop_w = (crtc_w / 4);
			ky_pstate->dst_crop_h = crtc_h;
			ky_pstate->is_crop = true;
		} else if ((crtc_x >= (hdisplay - (crtc_w / 16))) && (crtc_x <= hdisplay)) {
			ky_pstate->src_crop_x = src_x;
			ky_pstate->src_crop_y = src_y;
			ky_pstate->src_crop_w = (src_w / 4);
			ky_pstate->src_crop_h = src_h;
			ky_pstate->dst_crop_x = -2;
			ky_pstate->dst_crop_y = crtc_y;
			ky_pstate->dst_crop_w = (crtc_w / 4);
			ky_pstate->dst_crop_h = crtc_h;
			ky_pstate->is_crop = true;
		} else {
			ky_pstate->is_crop = false;
		}

	} else {
		ky_pstate->is_crop = false;
	}

	ky_plane_update_hw_channel(plane);

	ky_update_hdr_matrix(plane, ky_pstate);
	ky_update_csc_matrix(plane, old_state);

	/* No need for solid color layer */
	if (rdma_id < hwdev->rdma_nums) {
		u8 tbu_id;

		ky_pstate->mmu_tbl.size = ((PAGE_ALIGN(plane->state->fb->obj[0]->size) >> PAGE_SHIFT) +
					   HW_ALIGN_TTB_NUM) * 4;
		ky_pstate->mmu_tbl.va = dma_alloc_coherent(dpu->dev, ky_pstate->mmu_tbl.size, \
							   &ky_pstate->mmu_tbl.pa, GFP_KERNEL | __GFP_ZERO);
		if (ky_pstate->mmu_tbl.va == NULL) {
			DRM_ERROR("Failed to allocate %d bytes for dpu plane%d mmu table\n",
				   ky_pstate->mmu_tbl.size, plane->state->zpos);
			return;
		}
		tbu_id = !ky_pstate->right_image ? (rdma_id * 2) : (rdma_id * 2 + 1);
		ret = ky_dmmu_map(plane, &ky_pstate->mmu_tbl, tbu_id, false);
		if (!ret)
			cmdlist_regs_packing(plane);
		else
			DRM_ERROR("%s failed to map plane with ret = %d\n", __func__, ret);
	}
}

static void ky_plane_atomic_disable(struct drm_plane *plane,
				     struct drm_atomic_state *state)
{
	struct drm_plane_state *old_state = drm_atomic_get_old_plane_state(state, plane);
	DRM_DEBUG("%s()\n", __func__);

	ky_dmmu_unmap(plane);
	ky_plane_disable_hw_channel(plane, old_state);
}

// static void ky_plane_atomic_cleanup_fb(struct drm_plane *plane,
// 				     struct drm_plane_state *old_state)
// {

// }

static void ky_plane_reset(struct drm_plane *plane)
{
	struct ky_plane *p = to_ky_plane(plane);
	struct ky_plane_state *s;
	struct ky_drm_private *priv = plane->dev->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	struct ky_dpu *dpu = NULL;

	DRM_DEBUG("%s()\n", __func__);

	if (plane->state) {
		s = to_ky_plane_state(plane->state);
		dpu = crtc_to_dpu(plane->crtc);
		if (dpu)
			trace_ky_plane_reset(dpu->dev_id);

		__drm_atomic_helper_plane_destroy_state(plane->state);
		kfree(s);
		plane->state = NULL;
	}

	s = kzalloc(sizeof(*s), GFP_KERNEL);
	if (s) {
		__drm_atomic_helper_plane_reset(plane, &s->state);
		s->state.zpos = hwdev->plane_nums - p->hw_pid - 1;
		s->rdma_id = RDMA_INVALID_ID;
		s->is_offline = 1;
		s->is_crop = false;
		s->scaler_id = SCALER_INVALID_ID;
	}
}

static struct drm_plane_state *
ky_plane_atomic_duplicate_state(struct drm_plane *plane)
{
	struct ky_plane_state *s;
	struct ky_plane_state *old_state = to_ky_plane_state(plane->state);
	struct ky_dpu *dpu = NULL;

	if (plane->crtc) {
		dpu = crtc_to_dpu(plane->crtc);
		if (dpu)
			trace_ky_plane_atomic_duplicate_state(dpu->dev_id);
	}
	DRM_DEBUG("%s()\n", __func__);

	s = kzalloc(sizeof(*s), GFP_KERNEL);
	if (!s)
		return NULL;

	__drm_atomic_helper_plane_duplicate_state(plane, &s->state);

	WARN_ON(s->state.plane != plane);

	s->is_offline = old_state->is_offline;
	s->rdma_id = old_state->rdma_id;
	s->format = old_state->format;
	s->right_image = old_state->right_image;
	s->scaler_id = SCALER_INVALID_ID;
	s->use_scl = false;
	s->fbcmem_size = 0;

	s->is_crop = old_state->is_crop;
	s->src_crop_x = old_state->src_crop_x;
	s->src_crop_y = old_state->src_crop_y;
	s->src_crop_w = old_state->src_crop_w;
	s->src_crop_h = old_state->src_crop_h;
	s->dst_crop_x = old_state->dst_crop_x;
	s->dst_crop_y = old_state->dst_crop_y;
	s->dst_crop_w = old_state->dst_crop_w;
	s->dst_crop_h = old_state->dst_crop_h;
	s->screen_width = old_state->screen_width;
	s->screen_height = old_state->screen_height;

	if (s->hdr_coefs_blob_prop)
		drm_property_blob_get(s->hdr_coefs_blob_prop);
	if (s->scale_coefs_blob_prop)
		drm_property_blob_get(s->scale_coefs_blob_prop);
	return &s->state;
}

static void ky_plane_atomic_destroy_state(struct drm_plane *plane,
					    struct drm_plane_state *state)
{
	struct ky_plane_state *ky_pstate = to_ky_plane_state(state);
	struct ky_dpu *dpu = NULL;
	DRM_DEBUG("%s()\n", __func__);

	if (state->crtc) {
		dpu = crtc_to_dpu(state->crtc);

		if (ky_pstate->mmu_tbl.va)
			dma_free_coherent(dpu->dev, ky_pstate->mmu_tbl.size, \
					  ky_pstate->mmu_tbl.va, ky_pstate->mmu_tbl.pa);

		if (ky_pstate->cl.va)
			dma_free_coherent(dpu->dev, ky_pstate->cl.size, \
					  ky_pstate->cl.va, ky_pstate->cl.pa);
		if (dpu)
			trace_ky_plane_atomic_destroy_state(dpu->dev_id);
	}
	__drm_atomic_helper_plane_destroy_state(state);

	if (ky_pstate->hdr_coefs_blob_prop)
		drm_property_blob_put(ky_pstate->hdr_coefs_blob_prop);

	if (ky_pstate->scale_coefs_blob_prop)
		drm_property_blob_put(ky_pstate->scale_coefs_blob_prop);

	kfree(to_ky_plane_state(state));
}

static int ky_plane_atomic_set_property(struct drm_plane *plane,
					  struct drm_plane_state *state,
					  struct drm_property *property,
					  u64 val)
{
	struct ky_plane *p = to_ky_plane(plane);
	struct ky_plane_state *s = to_ky_plane_state(state);
	bool replaced = false;
	int ret = 0;

	DRM_DEBUG("%s() name = %s, val = %llu\n",
		  __func__, property->name, val);

	if (property == p->rdma_id_property)
		s->rdma_id = val;
	else if (property == p->solid_color_property)
		s->solid_color = val;
	else if (property == p->hdr_coef_property) {
		ret = ky_atomic_replace_property_blob_from_id(plane->dev,
					&s->hdr_coefs_blob_prop,
					val,
					-1,
					sizeof(int),
					&replaced);
		return ret;
	} else if (property == p->scale_coef_property) {
		ret = ky_atomic_replace_property_blob_from_id(plane->dev,
					&s->scale_coefs_blob_prop,
					val,
					-1,
					sizeof(int),
					&replaced);
		return ret;
	} else {
		DRM_ERROR("property %s is invalid\n", property->name);
		return -EINVAL;
	}

	return 0;
}

static int ky_plane_atomic_get_property(struct drm_plane *plane,
					  const struct drm_plane_state *state,
					  struct drm_property *property,
					  u64 *val)
{
	struct ky_plane *p = to_ky_plane(plane);
	const struct ky_plane_state *s = to_ky_plane_state(state);

	DRM_DEBUG("%s() name = %s\n", __func__, property->name);

	if (property == p->rdma_id_property)
		*val = s->rdma_id;
	else if (property == p->solid_color_property)
		*val = s->solid_color;
	else if (property == p->hdr_coef_property){
		if (s->hdr_coefs_blob_prop)
			*val = (s->hdr_coefs_blob_prop) ? s->hdr_coefs_blob_prop->base.id : 0;
	}
	else if (property == p->scale_coef_property){
		if (s->scale_coefs_blob_prop)
			*val = (s->scale_coefs_blob_prop) ? s->scale_coefs_blob_prop->base.id : 0;
	}
	else {
		DRM_ERROR("property %s is invalid\n", property->name);
		return -EINVAL;
	}

	return 0;
}

static const char * const color_encoding_name[] = {
	[DRM_COLOR_YCBCR_BT601] = "ITU-R BT.601 YCbCr",
	[DRM_COLOR_YCBCR_BT709] = "ITU-R BT.709 YCbCr",
	[DRM_COLOR_YCBCR_BT2020] = "ITU-R BT.2020 YCbCr",
};

static const char * const color_range_name[] = {
	[DRM_COLOR_YCBCR_FULL_RANGE] = "YCbCr full range",
	[DRM_COLOR_YCBCR_LIMITED_RANGE] = "YCbCr limited range",
};

int ky_drm_plane_create_color_properties(struct drm_plane *plane,
				      u32 supported_encodings,
				      u32 supported_ranges,
				      enum drm_color_encoding default_encoding,
				      enum drm_color_range default_range)
{
	struct drm_device *dev = plane->dev;
	struct drm_property *prop;
	struct drm_prop_enum_list enum_list[max_t(int, DRM_COLOR_ENCODING_MAX,
						       DRM_COLOR_RANGE_MAX)];
	int i, len;

	if (WARN_ON(supported_encodings == 0 ||
		    (supported_encodings & -BIT(DRM_COLOR_ENCODING_MAX)) != 0 ||
		    (supported_encodings & BIT(default_encoding)) == 0))
		return -EINVAL;

	if (WARN_ON(supported_ranges == 0 ||
		    (supported_ranges & -BIT(DRM_COLOR_RANGE_MAX)) != 0 ||
		    (supported_ranges & BIT(default_range)) == 0))
		return -EINVAL;

	len = 0;
	for (i = 0; i < DRM_COLOR_ENCODING_MAX; i++) {
		if ((supported_encodings & BIT(i)) == 0)
			continue;

		enum_list[len].type = i;
		enum_list[len].name = color_encoding_name[i];
		len++;
	}

	prop = drm_property_create_enum(dev, 0, "COLOR_ENCODING",
					enum_list, len);
	if (!prop)
		return -ENOMEM;
	plane->color_encoding_property = prop;
	drm_object_attach_property(&plane->base, prop, default_encoding);
	if (plane->state)
		plane->state->color_encoding = default_encoding;

	len = 0;
	for (i = 0; i < DRM_COLOR_RANGE_MAX; i++) {
		if ((supported_ranges & BIT(i)) == 0)
			continue;

		enum_list[len].type = i;
		enum_list[len].name = color_range_name[i];
		len++;
	}

	prop = drm_property_create_enum(dev, 0,	"COLOR_RANGE",
					enum_list, len);
	if (!prop)
		return -ENOMEM;
	plane->color_range_property = prop;
	drm_object_attach_property(&plane->base, prop, default_range);
	if (plane->state)
		plane->state->color_range = default_range;

	return 0;
}

static int ky_plane_create_properties(struct ky_plane *p, int index)
{
	struct drm_property *prop;
	unsigned int support_modes = BIT(DRM_MODE_BLEND_PIXEL_NONE) |
		BIT(DRM_MODE_BLEND_PREMULTI) |
		BIT(DRM_MODE_BLEND_COVERAGE);
	int ret = 0;

	DRM_DEBUG("%s()\n", __func__);
	/* create rotation property */
	drm_plane_create_rotation_property(&p->plane,
					   DRM_MODE_ROTATE_0,
					   DRM_MODE_ROTATE_MASK |
					   DRM_MODE_REFLECT_MASK);

	/* create zpos property */
	drm_plane_create_zpos_immutable_property(&p->plane, index);

	/* create layer alpha property */
	drm_plane_create_alpha_property(&p->plane);

	/* create blend mode property */
	drm_plane_create_blend_mode_property(&p->plane, support_modes);

	prop = drm_property_create_range(p->plane.dev, 0,
			"RDMA_ID", 0, ULLONG_MAX);
	if (!prop)
		return -ENOMEM;
	drm_object_attach_property(&p->plane.base, prop, 0);
	p->rdma_id_property = prop;

	prop = drm_property_create_range(p->plane.dev, 0,
			"SOLID_COLOR", 0, ULLONG_MAX);
	if (!prop)
		return -ENOMEM;
	drm_object_attach_property(&p->plane.base, prop, 0);
	p->solid_color_property = prop;

	prop = drm_property_create(p->plane.dev,
			DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
			"hdr_coefs", 0);
	if (!prop)
		return -ENOMEM;
	drm_object_attach_property(&p->plane.base, prop, 0);
	p->hdr_coef_property = prop;

	prop = drm_property_create(p->plane.dev,
			DRM_MODE_PROP_ATOMIC | DRM_MODE_PROP_BLOB,
			"scale_coefs", 0);
	if (!prop)
		return -ENOMEM;
	drm_object_attach_property(&p->plane.base, prop, 0);
	p->scale_coef_property = prop;

	ret = ky_drm_plane_create_color_properties(&p->plane,
					BIT(DRM_COLOR_YCBCR_BT601) | \
					BIT(DRM_COLOR_YCBCR_BT709) | \
					BIT(DRM_COLOR_YCBCR_BT2020),
					BIT(DRM_COLOR_YCBCR_LIMITED_RANGE) | \
					BIT(DRM_COLOR_YCBCR_FULL_RANGE),
					DRM_COLOR_YCBCR_BT601, DRM_COLOR_YCBCR_LIMITED_RANGE);
	if(ret)
		DRM_ERROR("Failed to create color properties %d\n", ret);

	return 0;
}

static const struct drm_plane_helper_funcs ky_plane_helper_funcs = {
	.atomic_check = ky_plane_atomic_check,
	.atomic_update = ky_plane_atomic_update,
	.atomic_disable = ky_plane_atomic_disable,
};

static const struct drm_plane_funcs ky_plane_funcs = {
	.update_plane = drm_atomic_helper_update_plane,
	.disable_plane	= drm_atomic_helper_disable_plane,
	.destroy = drm_plane_cleanup,
	.reset = ky_plane_reset,
	.atomic_duplicate_state = ky_plane_atomic_duplicate_state,
	.atomic_destroy_state = ky_plane_atomic_destroy_state,
	.atomic_set_property = ky_plane_atomic_set_property,
	.atomic_get_property = ky_plane_atomic_get_property,
};

struct drm_plane *ky_plane_init(struct drm_device *drm,
					struct ky_dpu *dpu)
{
	struct drm_plane *primary = NULL;
	struct ky_plane *p = NULL;
	enum drm_plane_type plane_type;
	int err, i, j;
	u32 *formats;
	struct ky_drm_private *priv = drm->dev_private;
	struct ky_hw_device *hwdev = priv->hwdev;
	u8 n_planes = hwdev->plane_nums;
	u8 n_formats = hwdev->n_formats;
	u8 n_fbcmems = hwdev->n_fbcmems;
	u8 n_rdmas = hwdev->rdma_nums;
	u32 plane_crtc_mask;

	trace_ky_plane_init(dpu->dev_id);
	if (n_fbcmems * 2 != n_rdmas) {
		DRM_ERROR("Unmatched rdma and fbcmem numbers, \
			   n_rdmas:%d n_fbcmems:%d!\n", n_rdmas, n_fbcmems);
		err = -EINVAL;
		return ERR_PTR(err);
	}

	formats = kcalloc(n_formats, sizeof(*formats), GFP_KERNEL);
	if (!formats) {
		err = -ENOMEM;
		return ERR_PTR(err);
	}

	/* Create all planes first. They can all be put to any CRTC. */
	plane_crtc_mask = (1 << priv->num_pipes) - 1;

	for (i = 0; i < n_planes; i++) {
		p = devm_kzalloc(drm->dev, sizeof(*p), GFP_KERNEL);
		if (!p) {
			kfree(formats);
			return ERR_PTR(-ENOMEM);
		}

		/* build the list of DRM supported formats based on the map */
		for (j = 0; j < n_formats; j++)
			formats[j] = hwdev->formats[j].format;

		// plane_type = (i < priv->num_pipes)
		// 	   ? DRM_PLANE_TYPE_PRIMARY
		// 	   : DRM_PLANE_TYPE_OVERLAY;

		plane_type = (i < priv->num_pipes) ? DRM_PLANE_TYPE_PRIMARY :
			   (i == priv->num_pipes) ? DRM_PLANE_TYPE_CURSOR :
			   DRM_PLANE_TYPE_OVERLAY;

		err = drm_universal_plane_init(drm, &p->plane, plane_crtc_mask,
					       &ky_plane_funcs, formats,
					       n_formats, supported_format_modifiers,
					       plane_type, NULL);
		if (err) {
			DRM_ERROR("fail to init %s plane%d\n", (i < priv->num_pipes) ? "primary" : "overlay", i);
			return ERR_PTR(err);
		}

		drm_plane_helper_add(&p->plane, &ky_plane_helper_funcs);

		ky_plane_create_properties(p, i);

		p->hwdev = hwdev;
		p->hw_pid = n_planes - i - 1;
		if (i == 0)
			primary = &p->plane;
	}

	kfree(formats);

	if (p)
		DRM_INFO("dpu plane init ok\n");

	return primary;
}

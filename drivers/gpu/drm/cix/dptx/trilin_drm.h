/* SPDX-License-Identifier: GPL-2.0*/
#ifndef __TRILIN_DP_DRM_H__
#define __TRILIN_DP_DRM_H__

struct trilin_dpsub;
struct trilin_connector;

int trilin_dp_drm_init(struct trilin_dpsub *dpsub);
int trilin_connector_update_modes(struct drm_connector *connector,
				  struct edid *edid);
void trilin_dp_encoder_atomic_mode_set(
	struct drm_encoder *encoder, struct drm_crtc_state *crtc_state,
	struct drm_connector_state *connector_state);
int trilin_dp_encoder_compute_config(struct drm_encoder *encoder,
	struct drm_crtc_state *crtc_state,
	struct drm_connector_state *connector_state,
	u8 suggest_rpc);
int trilin_dp_encoder_atomic_adjust_mode(struct trilin_dp *dp,
	struct drm_display_mode *mode,
	struct drm_display_mode *adjusted_mode);
enum drm_mode_status
trilin_dp_connector_mode_valid(struct drm_connector *connector,
			       struct drm_display_mode *mode);
void trilin_dp_connector_reset(struct drm_connector *connector);
int trilin_dp_fill_modes(struct drm_connector *connector, uint32_t maxX,
			 uint32_t maxY);
#endif
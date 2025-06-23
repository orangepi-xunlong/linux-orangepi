#ifndef __TRILIN_DP_DRM_H__
#define __TRILIN_DP_DRM_H__

struct trilin_dpsub;

int	trilin_dp_drm_init(struct trilin_dpsub *dpsub);
int trilin_connector_update_modes(struct drm_connector *connector,
				struct edid *edid);
void trilin_dp_encoder_atomic_mode_set(struct drm_encoder *encoder,
				  struct drm_crtc_state *crtc_state,
				  struct drm_connector_state *connector_state);
int trilin_dp_encoder_atomic_check(struct drm_encoder *encoder,
			       struct drm_crtc_state *crtc_state,
			       struct drm_connector_state *conn_state);
enum drm_mode_status trilin_dp_connector_mode_valid(struct drm_connector *connector,
					  struct drm_display_mode *mode);
void trilin_dp_connector_reset(struct drm_connector *connector);
int trilin_dp_fill_modes(struct drm_connector *connector, uint32_t maxX, uint32_t maxY);
u8 trilin_dp_cal_bpc(struct drm_connector_state *connector_state);
u8 trilin_dp_cal_bpp(struct drm_connector_state *connector_state,
			u8 bpc, enum trilin_dpsub_format format);
#endif
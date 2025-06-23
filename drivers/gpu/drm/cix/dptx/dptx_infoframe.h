/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023, The Linux Foundation. All rights reserved.
 */

#ifndef _DP_INFOFRAME_H_
#define _DP_INFOFRAME_H_

#include <linux/platform_device.h>
#include <drm/display/drm_dp_helper.h>
#include <drm/drm_edid.h>
#include <drm/display/drm_hdcp.h>
#include <drm/drm_probe_helper.h>

#define CIX_AVI_INFO_SDP   0
#define CIX_VSC_SDP        1
#define CIX_AUDIO_INFO_SDP 2
#define CIX_HDR_SDP        3
#define CIX_MAX_SDP        7

enum trilin_dpsub_format {
	TRILIN_DPSUB_FORMAT_RGB,
	TRILIN_DPSUB_FORMAT_YCBCR444,
	TRILIN_DPSUB_FORMAT_YCBCR422,
	TRILIN_DPSUB_FORMAT_YCBCR420,
	TRILIN_DPSUB_FORMAT_YONLY,
	TRILIN_DPSUB_FORMAT_DSC,
	TRILIN_DPSUB_FORMAT_MAX,
};

struct trilin_dp;
struct trilin_dp_config;

void cix_infoframe_write_packet(struct trilin_dp *dp, u32 source, u32 select, struct dp_sdp *sdp, u32 rate);

int cix_dptx_setup_vsc_sdp(struct dp_sdp *sdp, struct trilin_dp_config* config);

int cix_dptx_setup_avi_infoframe(
		struct dp_sdp *sdp,
		enum trilin_dpsub_format format,
		const struct drm_connector_state *conn_state
);
ssize_t cix_dp_hdr_metadata_infoframe_sdp_pack(
		const struct hdmi_drm_infoframe *drm_infoframe,
		struct dp_sdp *sdp,
		size_t size);

#endif

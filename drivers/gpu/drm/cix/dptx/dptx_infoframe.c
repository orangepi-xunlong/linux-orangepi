#include <drm/drm_atomic_helper.h>
#include <drm/drm_connector.h>
#include <drm/drm_crtc.h>
#include <drm/drm_device.h>
#include <drm/drm_edid.h>
#include <drm/drm_encoder.h>
#include <drm/drm_managed.h>
#include <drm/drm_modes.h>
#include <drm/drm_of.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/display/drm_hdmi_helper.h>

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/phy/phy.h>
#include <linux/reset.h>

#include "trilin_dptx_reg.h"
#include "trilin_host_tmr.h"
#include "trilin_dptx.h"

#include "trilin_phy.h"
#include "dptx_infoframe.h"

#define DPTX_SDP_DB_MAX        32
#define DPTX_MST_SOURCE_NUMBER  2
#define DPTX_SDP_SELECT_NUMBER  7

static void dptx_infoframe_write_content(struct trilin_dp *dp, u32 source, u32 select, struct dp_sdp *sdp)
{
	int i;
	const u32 CUR_REG = TRILIN_DPTX_SEC0_INFOFRAME_DATA + TRILIN_DPTX_SOURCE_OFFSET * source;
	/* Secondary-Data Packet ID = 0 */
	trilin_dp_write(dp, CUR_REG, sdp->sdp_header.HB0);
	/* Secondary-data Packet Type */
	trilin_dp_write(dp, CUR_REG, sdp->sdp_header.HB1);
	/* Revision Number */
	trilin_dp_write(dp, CUR_REG, sdp->sdp_header.HB2);
	trilin_dp_write(dp, CUR_REG, sdp->sdp_header.HB3);

	for (i = 0; i < DPTX_SDP_DB_MAX; i++)
		trilin_dp_write(dp, CUR_REG, sdp->db[i]);
}

static void dptx_infoframe_select_buffer_pre(struct trilin_dp *dp, u32 source, u32 select)
{
	u32 curret_value;
	const u32 CUR_REG_EANBLE = TRILIN_DPTX_SEC0_INFOFRAME_ENABLE + TRILIN_DPTX_SOURCE_OFFSET * source;
	const u32 CUR_REG_SELECT = TRILIN_DPTX_SEC0_INFOFRAME_SELECT + TRILIN_DPTX_SOURCE_OFFSET * source;
	curret_value = trilin_dp_read(dp, CUR_REG_EANBLE);
	trilin_dp_write(dp, CUR_REG_EANBLE, (curret_value & ~(1 << select)));
	trilin_dp_write(dp, CUR_REG_SELECT, select);
}

static void dptx_infoframe_select_buffer_end(struct trilin_dp *dp, u32 source, u32 select)
{
	u32 curret_value;
	const u32 CUR_REG = TRILIN_DPTX_SEC0_INFOFRAME_ENABLE + TRILIN_DPTX_SOURCE_OFFSET * source;
	curret_value = trilin_dp_read(dp, CUR_REG);
	trilin_dp_write(dp, CUR_REG, (curret_value | (1 << select)));
}

static void dptx_infoframe_rate(struct trilin_dp *dp, u32 source, u32 select, u32 rate)
{
	u32 curret_value;
	const u32 CUR_REG = TRILIN_DPTX_SEC0_INFOFRAME_RATE + TRILIN_DPTX_SOURCE_OFFSET * source;

	curret_value = trilin_dp_read(dp, CUR_REG);

	if (rate) {
		trilin_dp_write(dp, CUR_REG, (curret_value | (1 << select)));
	} else {
		trilin_dp_write(dp, CUR_REG, (curret_value & ~(1 << select)));
	}
}

//------------------------------------------------------------------------------
//  Function: dptx_infoframe_write_packet
//      Write the contents of the InfoFrame packet to the core. The secondary
//      packet type will have a valid value for infoframe packets (0x80+).
//
//  Parameters:
//      dev_id - device ID
//      buffer_select - InfoFrame buffer select (0-6)

//  Returns:
//      None
//------------------------------------------------------------------------------
void cix_infoframe_write_packet(
		struct trilin_dp *dp,
		u32 source, u32 select, struct dp_sdp *sdp, u32 rate
		)
{
	if (source >= DPTX_MST_SOURCE_NUMBER)
		return;

	if (select >= CIX_MAX_SDP)
		return;

	dptx_infoframe_select_buffer_pre(dp, source, select);
	dptx_infoframe_write_content(dp, source, select, sdp);
	dptx_infoframe_rate(dp, source, select, rate);
	dptx_infoframe_select_buffer_end(dp, source, select);
}

static void pack_avi_infoframe(
		struct hdmi_avi_infoframe *frame,
		enum trilin_dpsub_format format,
		const struct drm_connector_state *conn_state
)
{
	hdmi_avi_infoframe_init(frame);

	frame->picture_aspect = HDMI_PICTURE_ASPECT_4_3;
	frame->content_type = HDMI_CONTENT_TYPE_GRAPHICS;
	frame->itc = 1;
	frame->active_aspect = HDMI_ACTIVE_ASPECT_4_3;
	frame->scan_mode = HDMI_SCAN_MODE_UNDERSCAN;
	frame->colorimetry = HDMI_COLORIMETRY_ITU_709;
	frame->extended_colorimetry = HDMI_EXTENDED_COLORIMETRY_XV_YCC_709;
	frame->quantization_range = HDMI_QUANTIZATION_RANGE_DEFAULT;
	frame->ycc_quantization_range = HDMI_YCC_QUANTIZATION_RANGE_LIMITED;
	frame->video_code = 1;

	switch(format) {
	case TRILIN_DPSUB_FORMAT_YCBCR444:
		frame->colorspace = HDMI_COLORSPACE_YUV444;
		break;
	case TRILIN_DPSUB_FORMAT_YCBCR422:
		frame->colorspace = HDMI_COLORSPACE_YUV422;
		break;
	case TRILIN_DPSUB_FORMAT_YCBCR420:
		frame->colorspace = HDMI_COLORSPACE_YUV420;
		break;
	default:
		frame->colorspace = HDMI_COLORSPACE_RGB;
		break;
	}

	drm_hdmi_avi_infoframe_bars(frame, conn_state);
}

int cix_dptx_setup_avi_infoframe(
		struct dp_sdp *sdp,
		enum trilin_dpsub_format format,
		const struct drm_connector_state *conn_state
)
{
	const int infoframe_size = HDMI_INFOFRAME_HEADER_SIZE + HDMI_AVI_INFOFRAME_SIZE;
	unsigned char buf[HDMI_INFOFRAME_HEADER_SIZE + HDMI_AVI_INFOFRAME_SIZE];
	struct hdmi_avi_infoframe frame;
	size_t len;

	memset(sdp, 0, sizeof(*sdp));
	pack_avi_infoframe(&frame, format, conn_state);
	len = hdmi_avi_infoframe_pack_only(&frame, buf, sizeof(buf));
	if (len < 0) {
		pr_info("buffer size is smaller than avi infoframe\n");
		return -1;
	}

	if (len != infoframe_size) {
		pr_info("wrong avi infoframe size\n");
		return -1;
	}

	sdp->sdp_header.HB0 = 0;
	sdp->sdp_header.HB1 = frame.type;
	sdp->sdp_header.HB2 = 0x1D;
	sdp->sdp_header.HB3 = (0x13 << 2);
	sdp->db[0] = frame.version;
	sdp->db[1] = frame.length;

	/*
	* Size of DP infoframe sdp packet for AVI Infoframe consists of
	* - DP SDP Header(struct dp_sdp_header): 4 bytes
	* - Two Data Blocks: 2 bytes
	* CTA Header Byte2 (INFOFRAME Version Number)
	* CTA Header Byte3 (Length of INFOFRAME)
	* HDMI_AVI_INFOFRAME_SIZE: 13 bytes
	*/
	sdp->db[0] |= buf[HDMI_INFOFRAME_HEADER_SIZE];
	sdp->db[1] |= buf[HDMI_INFOFRAME_HEADER_SIZE + 1];
	memcpy(&sdp->db[2], &buf[HDMI_INFOFRAME_HEADER_SIZE + 2], HDMI_AVI_INFOFRAME_SIZE - 2);

	return 0;
}

int cix_dptx_setup_vsc_sdp(
		struct dp_sdp *sdp,
		struct trilin_dp_config* config)
{
	sdp->sdp_header.HB0 = 0;
	sdp->sdp_header.HB1 = 0x7;
	sdp->sdp_header.HB2 = 0x5;
	sdp->sdp_header.HB3 = 0x13;

	switch(config->format) {
	case TRILIN_DPSUB_FORMAT_YCBCR444:
		sdp->db[16] = 0x1 << 4;
		break;
	case TRILIN_DPSUB_FORMAT_YCBCR422:
		sdp->db[16] = 0x1 << 4;
		break;
	case TRILIN_DPSUB_FORMAT_YCBCR420:
		sdp->db[16] = 0x3 << 4;
		break;
	default:
		break;
	}

	switch(config->colorspace) {
	case DRM_MODE_COLORIMETRY_BT709_YCC:
		sdp->db[16] |= 0x1;
		break;
	case DRM_MODE_COLORIMETRY_XVYCC_601:
		sdp->db[16] |= 0x2;
		break;
	case DRM_MODE_COLORIMETRY_XVYCC_709:
		sdp->db[16] |= 0x3;
		break;
	case DRM_MODE_COLORIMETRY_SYCC_601:
		sdp->db[16] |= 0x4;
		break;
	case DRM_MODE_COLORIMETRY_OPYCC_601:
		sdp->db[16] |= 0x5;
		break;
	case DRM_MODE_COLORIMETRY_BT2020_CYCC:
	case DRM_MODE_COLORIMETRY_BT2020_RGB:
		sdp->db[16] |= 0x6;
		break;
	case DRM_MODE_COLORIMETRY_BT2020_YCC:
		sdp->db[16] |= 0x7;
		break;
	case DRM_MODE_COLORIMETRY_DCI_P3_RGB_D65:
	case DRM_MODE_COLORIMETRY_DCI_P3_RGB_THEATER:
		sdp->db[16] |= 0x4;
		break;
	default:
		sdp->db[16] |= 0x1;
		break;
	}

	/* For pixel encoding formats YCbCr444, YCbCr422, YCbCr420, and Y Only,
	 * the following Component Bit Depth values are defined:
	 * 001b = 8bpc.
	 * 010b = 10bpc.
	 * 011b = 12bpc.
	 * 100b = 16bpc.
	 */
	switch (config->bpc) {
	case 16: sdp->db[17] = 0x4; break;
	case 12: sdp->db[17] = 0x3; break;
	case 10: sdp->db[17] = 0x2; break;
	case 8:
	default: sdp->db[17] = 0x1; break;
	}

	/* Dynamic Range and Component Bit Depth */
	if (config->dynamic_range == DP_DYNAMIC_RANGE_CTA)
		sdp->db[17] |= 0x80;  /* DB17[7] */

	/* Content Type */
	sdp->db[18] = config->content_type & 0x7;

	return 0;
}

ssize_t
cix_dp_hdr_metadata_infoframe_sdp_pack(const struct hdmi_drm_infoframe *drm_infoframe,
	struct dp_sdp *sdp, size_t size)
{
	size_t length = sizeof(struct dp_sdp);
	const int infoframe_size = HDMI_INFOFRAME_HEADER_SIZE + HDMI_DRM_INFOFRAME_SIZE;
	unsigned char buf[HDMI_INFOFRAME_HEADER_SIZE + HDMI_DRM_INFOFRAME_SIZE];
	ssize_t len;

	if (size < length)
		return -ENOSPC;

	memset(sdp, 0, size);

	len = hdmi_drm_infoframe_pack_only(drm_infoframe, buf, sizeof(buf));
	if (len < 0) {
		pr_err("buffer size is smaller than hdr metadata infoframe\n");
		return -ENOSPC;
	}

	if (len != infoframe_size) {
		pr_err("wrong static hdr metadata size (%d %d)\n", (int)len, infoframe_size);
		return -ENOSPC;
	}

	/*
	 * Set up the infoframe sdp packet for HDR static metadata.
	 * Prepare VSC Header for SU as per DP 1.4a spec,
	 * Table 2-100 and Table 2-101
	 */

	/* Secondary-Data Packet ID, 00h for non-Audio INFOFRAME */
	sdp->sdp_header.HB0 = 0;
	/*
	 * Packet Type 80h + Non-audio INFOFRAME Type value
	 * HDMI_INFOFRAME_TYPE_DRM: 0x87
	 * - 80h + Non-audio INFOFRAME Type value
	 * - InfoFrame Type: 0x07
	 *    [CTA-861-G Table-42 Dynamic Range and Mastering InfoFrame]
	 */
	sdp->sdp_header.HB1 = drm_infoframe->type;
	/*
	 * Least Significant Eight Bits of (Data Byte Count – 1)
	 * infoframe_size - 1
	 */
	sdp->sdp_header.HB2 = 0x1D;
	/* INFOFRAME SDP Version Number */
	sdp->sdp_header.HB3 = (0x13 << 2);
	/* CTA Header Byte 2 (INFOFRAME Version Number) */
	sdp->db[0] = drm_infoframe->version;
	/* CTA Header Byte 3 (Length of INFOFRAME): HDMI_DRM_INFOFRAME_SIZE */
	sdp->db[1] = drm_infoframe->length;
	/*
	 * Copy HDMI_DRM_INFOFRAME_SIZE size from a buffer after
	 * HDMI_INFOFRAME_HEADER_SIZE
	 */
	BUILD_BUG_ON(sizeof(sdp->db) < HDMI_DRM_INFOFRAME_SIZE + 2);
	memcpy(&sdp->db[2], &buf[HDMI_INFOFRAME_HEADER_SIZE],
	       HDMI_DRM_INFOFRAME_SIZE);
	/*
	 * Size of DP infoframe sdp packet for HDR static metadata consists of
	 * - DP SDP Header(struct dp_sdp_header): 4 bytes
	 * - Two Data Blocks: 2 bytes
	 *    CTA Header Byte2 (INFOFRAME Version Number)
	 *    CTA Header Byte3 (Length of INFOFRAME)
	 * - HDMI_DRM_INFOFRAME_SIZE: 26 bytes
	 *
	 * Prior to GEN11's GMP register size is identical to DP HDR static metadata
	 * infoframe size. But GEN11+ has larger than that size, write_infoframe
	 * will pad rest of the size.
	 */
	return sizeof(struct dp_sdp_header) + 2 + HDMI_DRM_INFOFRAME_SIZE;
}

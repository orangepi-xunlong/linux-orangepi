#include "core/packets.h"
#include "core/fc.h"
#include "core/hdmi_reg.h"
#include "core/hdmi_core.h"
#include "edid/edid.h"
#include "bsp/access.h"
#include "util/util.h"

#define ACP_PACKET_SIZE 	16
#define ISRC_PACKET_SIZE 	16

int packets_initialize(hdmi_tx_dev_t *dev)
{
	packets_disable_all_packets(dev);
	return TRUE;
}

int packets_configure(hdmi_tx_dev_t *dev, video_params_t * video)
{
	if (video->mhdmi_video_format == HDMI_3D_FORMAT) {
		pr_debug("%s:3D packet configuration", __func__);
		fc_packets_auto_send(dev,  0, VSD_TX);	/* prevent sending half the info. */
		// frame packing || tab || sbs
		if ((video->m3d_structure == FRAME_PACKING_3D)  ||
		    (video->m3d_structure == TOP_AND_BOTTOM_3D) ||
		    (video->m3d_structure == SIDE_BY_SIDE_3D)) {
			u8 packet_data[3] = {0,0,0}; //PB4-PB6
			packet_data[0] = set8(0, PACKET_HDMIVIDEOFORMAT_MASK, video->mhdmi_video_format);
			packet_data[1] = set8(0, PACKET_3D_STRUCTURE_MASK, video->m3d_structure);
			packet_data[2] = set8(0, PACKET_3D_EXT_DATA_MASK,  video->m3d_ext_data);

			packets_vendor_specific_info_frame(dev, HDMI_LICENSING_LLC_OUI, packet_data, sizeof(packet_data), 1);
			//send3d = TRUE;
		}
		else {
			pr_err("%s:3D structure not supported %d", __func__, video->m3d_structure);
			return HDMI_ERR_3DSTRUCTURE_NOT_SUPPORT;
		}
		fc_packets_auto_send(dev, 1, VSD_TX);
	} else if (video->mhdmi_video_format == HDMI_EXT_RES_FORMAT ) {
		u8 packet_data[3] = {0,0,0}; //PB4-PB6
		pr_debug("%s:4k packet configuration", __func__);
		fc_packets_auto_send(dev,  0, VSD_TX);	/* prevent sending half the info. */

		packet_data[0] = set8(0, PACKET_HDMIVIDEOFORMAT_MASK, video->mhdmi_video_format);
		//TODO: correct this - use the hdmivic information instead
		packet_data[1] = set8(0, PACKET_HDMI_VIC_MASK, video_params_get_hhdmi_vic_code(video->mdtd.m_code));
		packet_data[2] = 0;

		packets_vendor_specific_info_frame(dev, HDMI_LICENSING_LLC_OUI, packet_data, sizeof(packet_data), 1);

		fc_packets_auto_send(dev, 1, VSD_TX);
	} else {
		fc_packets_auto_send(dev,  0, VSD_TX);	//stop VSD packets
	}

	/*
	if (prod != 0) {
		fc_spd_info_t spd_data;
		spd_data.vname    = prod->mVendorName;
		spd_data.vlength  = prod->mVendorNameLength;
		spd_data.pname    = prod->mProductName;
		spd_data.plength  = prod->mProductNameLength;
		spd_data.code     = prod->mSourceType;
		spd_data.auto_send = 1;

		u32 oui = prod->mOUI;
		u8 *vendor_payload = prod->mVendorPayload;
		u8 payload_length  = prod->mVendorPayloadLength;

		fc_spd_config(dev, &spd_data);

		packets_vendor_specific_info_frame(dev, oui, vendor_payload, payload_length, 1);
	}
	else {
		LOGGER(SNPS_WARN,"No product info provided: not configured");
	}*/

	fc_packets_metadata_config(dev);

	// default phase 1 = true
	dev_write_mask(FC_GCP, FC_GCP_DEFAULT_PHASE_MASK, ((video->mpixel_packing_defaultphase == 1) ? 1 : 0));

	fc_gamut_config(dev);

	fc_avi_config(dev, video);

	/** Colorimetry */
	packets_colorimetry_config(dev, video);

	return 0;
}

void packets_audio_content_protection(hdmi_tx_dev_t *dev, u8 type, const u8 * fields, u8 length, u8 auto_send)
{
	u8 new_fields[ACP_PACKET_SIZE];
	u16 i = 0;

	fc_packets_auto_send(dev, 0, ACP_TX);
	fc_acp_type(dev, type);

	for (i = 0; i < length; i++) {
		new_fields[i] = fields[i];
	}
	if (length < ACP_PACKET_SIZE) {
		for (i = length; i < ACP_PACKET_SIZE; i++) {
			new_fields[i] = 0;	/* Padding */
		}
		length = ACP_PACKET_SIZE;
	}
	fc_acp_type_dependent_fields(dev, new_fields, length);
	if (!auto_send) {
		fc_packets_manual_send(dev, ACP_TX);
	} else {
		fc_packets_auto_send(dev, auto_send, ACP_TX);
	}

}

void packets_isrc_packets(hdmi_tx_dev_t *dev, u8 init_status, const u8 * codes, u8 length, u8 auto_send)
{
	u16 i = 0;
	u8 new_codes[ISRC_PACKET_SIZE * 2];

	fc_packets_auto_send(dev, 0, ISRC1_TX);
	fc_packets_auto_send(dev, 0, ISRC2_TX);

	fc_isrc_status(dev, init_status);

	for (i = 0; i < length; i++) {
		new_codes[i] = codes[i];
	}

	if (length > ISRC_PACKET_SIZE) {
		for (i = length; i < (ISRC_PACKET_SIZE * 2); i++) {
			new_codes[i] = 0;	/* Padding */
		}
		length = (ISRC_PACKET_SIZE * 2);

		fc_isrc_isrc2_codes(dev, new_codes + (ISRC_PACKET_SIZE * sizeof(u8)), length - ISRC_PACKET_SIZE);
		fc_isrc_cont(dev, 1);

		fc_packets_auto_send(dev, auto_send, ISRC2_TX);

		if (!auto_send) {
			fc_packets_manual_send(dev, ISRC2_TX);
		}
	}
	if (length < ISRC_PACKET_SIZE) {
		for (i = length; i < ISRC_PACKET_SIZE; i++) {
			new_codes[i] = 0;	/* Padding */
		}

		length = ISRC_PACKET_SIZE;

		fc_isrc_cont(dev, 0);
	}

	fc_isrc_isrc1_codes(dev, new_codes, length);	/* first part only */
	fc_isrc_valid(dev, 1);
	fc_packets_auto_send(dev, auto_send, ISRC1_TX);

	if (!auto_send) {
		fc_packets_manual_send(dev, ISRC1_TX);
	}
}

void packets_av_mute(hdmi_tx_dev_t *dev, u8 enable)
{
	dev_write_mask(FC_GCP, FC_GCP_SET_AVMUTE_MASK, (enable ? 1 : 0));
	dev_write_mask(FC_GCP, FC_GCP_CLEAR_AVMUTE_MASK, (enable ? 0 : 1));
}

void packets_isrc_status(hdmi_tx_dev_t *dev, u8 status)
{
	fc_isrc_status(dev, status);
}

void packets_stop_send_acp(hdmi_tx_dev_t *dev)
{
	fc_packets_auto_send(dev, 0, ACP_TX);
}

void packets_stop_send_isrc1(hdmi_tx_dev_t *dev)
{
	fc_packets_auto_send(dev, 0, ISRC1_TX);
	fc_packets_auto_send(dev, 0, ISRC2_TX);
}

void packets_stop_send_isrc2(hdmi_tx_dev_t *dev)
{
	fc_isrc_cont(dev, 0);
	fc_packets_auto_send(dev, 0, ISRC2_TX);
}

void packets_stop_send_spd(hdmi_tx_dev_t *dev)
{
	fc_packets_auto_send(dev, 0, SPD_TX);
}

void packets_stop_send_vsd(hdmi_tx_dev_t *dev)
{
	fc_packets_auto_send(dev, 0, VSD_TX);
}

void packets_disable_all_packets(hdmi_tx_dev_t *dev)
{
	fc_packets_disable_all(dev);
}

int packets_vendor_specific_info_frame(hdmi_tx_dev_t *dev, u32 oui, const u8 * payload, u8 length, u8 auto_send)
{
	fc_packets_auto_send(dev,  0, VSD_TX);	/* prevent sending half the info. */
	fc_vsd_vendor_oui(dev, oui);
	if (fc_vsd_vendor_payload(dev, payload, length)) {
		return FALSE;	/* DEFINE ERROR */
	}
	if (auto_send) {
		fc_packets_auto_send(dev, auto_send, VSD_TX);
	} else {
		fc_packets_manual_send(dev, VSD_TX);
	}
	return TRUE;
}

void packets_colorimetry_config(hdmi_tx_dev_t *dev, video_params_t * video)
{
	u8 gamut_metadata[28] = {0};
	int gdb_color_space = 0;

	fc_gamut_enable_tx(dev, 0);

	if(video->mcolorimetry == EXTENDED_COLORIMETRY){
		if(video->mext_colorimetry == XV_YCC601){
			gdb_color_space = 1;
		}
		else if(video->mext_colorimetry == XV_YCC709){
			gdb_color_space = 2;
			pr_debug("xv ycc709");
		}
		else if(video->mext_colorimetry == S_YCC601){
			gdb_color_space = 3;
		}
		else if(video->mext_colorimetry == ADOBE_YCC601){
			gdb_color_space = 3;
		}
		else if(video->mext_colorimetry == ADOBE_RGB){
			gdb_color_space = 3;
		}

		if(video->mcolorimetry_datablock == TRUE){
			gamut_metadata[0] = (1 << 7) | gdb_color_space;
			fc_gamut_packet_config(dev, gamut_metadata, (sizeof(gamut_metadata) / sizeof(u8)));
		}
	}
}

u8 vp_pixel_packing_phase(hdmi_tx_dev_t *dev)
{
	return (u8)(dev_read(VP_STATUS) & 0xF);
}

void vp_color_depth(hdmi_tx_dev_t *dev, u8 value)
{
	/* color depth */
	dev_write_mask(VP_PR_CD, VP_PR_CD_COLOR_DEPTH_MASK, value);
}

void vp_pixel_packing_default_phase(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(VP_STUFF, VP_STUFF_IDEFAULT_PHASE_MASK, bit);
}

void vp_pixel_repetition_factor(hdmi_tx_dev_t *dev, u8 value)
{
	/* desired factor */
	dev_write_mask(VP_PR_CD, VP_PR_CD_DESIRED_PR_FACTOR_MASK, value);
	/* enable stuffing */
	dev_write_mask(VP_STUFF, VP_STUFF_PR_STUFFING_MASK, 1);
	/* enable block */
	dev_write_mask(VP_CONF, VP_CONF_PR_EN_MASK, (value > 1) ? 1 : 0);
	/* bypass block */
	dev_write_mask(VP_CONF, VP_CONF_BYPASS_SELECT_MASK, (value > 1) ? 0 : 1);
}

void vp_ycc422_remap_size(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(VP_REMAP, VP_REMAP_YCC422_SIZE_MASK, value);
}

void vp_output_selector(hdmi_tx_dev_t *dev, u8 value)
{
	if (value == 0) {	/* pixel packing */
		dev_write_mask(VP_CONF, VP_CONF_BYPASS_EN_MASK, 0);
		/* enable pixel packing */
		dev_write_mask(VP_CONF, VP_CONF_PP_EN_MASK, 1);
		dev_write_mask(VP_CONF, VP_CONF_YCC422_EN_MASK, 0);
	} else if (value == 1) {	/* YCC422 */
		dev_write_mask(VP_CONF, VP_CONF_BYPASS_EN_MASK, 0);
		dev_write_mask(VP_CONF, VP_CONF_PP_EN_MASK, 0);
		/* enable YCC422 */
		dev_write_mask(VP_CONF, VP_CONF_YCC422_EN_MASK, 1);
	} else if (value == 2 || value == 3) {	/* bypass */
		/* enable bypass */
		dev_write_mask(VP_CONF, VP_CONF_BYPASS_EN_MASK, 1);
		dev_write_mask(VP_CONF, VP_CONF_PP_EN_MASK, 0);
		dev_write_mask(VP_CONF, VP_CONF_YCC422_EN_MASK, 0);
	} else {
		pr_err("%s Wrong output option: %d", __func__, value);
		return;
	}

	/* YCC422 stuffing */
	dev_write_mask(VP_STUFF, VP_STUFF_YCC422_STUFFING_MASK, 1);
	/* pixel packing stuffing */
	dev_write_mask(VP_STUFF, VP_STUFF_PP_STUFFING_MASK, 1);

	/* output selector */
	dev_write_mask(VP_CONF, VP_CONF_OUTPUT_SELECTOR_MASK, value);
}
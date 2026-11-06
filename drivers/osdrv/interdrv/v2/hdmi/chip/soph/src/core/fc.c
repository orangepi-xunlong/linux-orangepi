#include "core/hdmi_reg.h"
#include "core/hdmi_core.h"
#include "fc.h"
#include "video.h"
#include "bsp/access.h"
#include "util/util.h"


void fc_vsd_vendor_oui(hdmi_tx_dev_t *dev, u32 id)
{
	dev_write( (FC_VSDIEEEID0), id);
	dev_write( (FC_VSDIEEEID1), id >> 8);
	dev_write( (FC_VSDIEEEID2), id >> 16);
}

u8 fc_vsd_vendor_payload(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length)
{
	const unsigned short size = 24;
	unsigned i = 0;
	if (data == 0) {
		pr_err("invalid parameter");
		return 1;
	}
	if (length > size) {
		length = size;
		pr_err("vendor payload truncated");
	}
	for (i = 0; i < length; i++) {
		dev_write( (FC_VSDPAYLOAD0 + (i*4)), data[i]);
	}
	return 0;
}

void fc_spd_vendor_name(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length)
{
	unsigned short i = 0;
	for (i = 0; i < length; i++) {
		dev_write(FC_SPDVENDORNAME0 + (i*4), data[i]);
	}
}

void fc_spd_product_name(hdmi_tx_dev_t *dev, const u8 * data, unsigned short length)
{
	unsigned short i = 0;
	for (i = 0; i < length; i++) {
		dev_write(FC_SPDPRODUCTNAME0 + (i*4), data[i]);
	}
}

void fc_spd_source_device_info(hdmi_tx_dev_t *dev, u8 code)
{
	dev_write(FC_SPDDEVICEINF, code);
}

int fc_spd_config(hdmi_tx_dev_t *dev, fc_spd_info_t *spd_data)
{
	const unsigned short psize = 8;
	const unsigned short vsize = 16;

	if(spd_data == NULL){
		pr_err("Improper argument: spd_data");
		return FALSE;
	}

	fc_packets_auto_send(dev, 0, SPD_TX);	/* prevent sending half the info. */

	if (spd_data->vname == 0) {
		pr_err("invalid parameter");
		return FALSE;
	}
	if (spd_data->vlength > vsize) {
		spd_data->vlength = vsize;
		pr_err("vendor name truncated");
	}
	if (spd_data->pname == 0) {
		pr_err("invalid parameter");
		return FALSE;
	}
	if (spd_data->plength > psize) {
		spd_data->plength = psize;
		pr_err("product name truncated");
	}

	fc_spd_vendor_name(dev, spd_data->vname, spd_data->vlength);
	fc_spd_product_name(dev, spd_data->pname, spd_data->plength);

	fc_spd_source_device_info(dev, spd_data->code);

	if (spd_data->auto_send) {
		fc_packets_auto_send(dev, spd_data->auto_send, SPD_TX);
	} else {
		fc_packets_manual_send(dev, SPD_TX);
	}

	return TRUE;
}

void fc_video_hdcp_keepout(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_HDCP_KEEPOUT_MASK, 0);
}

void fc_video_vsync_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_VSYNC_IN_POLARITY_MASK, bit);
}

void fc_video_hsync_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_HSYNC_IN_POLARITY_MASK, bit);
}

void fc_video_data_enable_polarity(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_DE_IN_POLARITY_MASK, bit);
}

void fc_video_dvi_or_hdmi(hdmi_tx_dev_t *dev, u8 bit)
{
	/* 1: HDMI; 0: DVI */
	dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_DVI_MODEZ_MASK, bit);
}

void fc_video_vblank_osc(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_R_V_BLANK_IN_OSC_MASK, bit);
}

void fc_video_interlaced(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_INVIDCONF, FC_INVIDCONF_IN_I_P_MASK, bit);
}

void fc_video_hactive(hdmi_tx_dev_t *dev, u16 value)
{
	/* 12-bit width */

	dev_write((FC_INHACTIV0), (u8) (value));
	dev_write_mask(FC_INHACTIV1, FC_INHACTIV1_H_IN_ACTIV_MASK |
					FC_INHACTIV1_H_IN_ACTIV_12_MASK, (u8)(value >> 8));
}

void fc_video_hblank(hdmi_tx_dev_t *dev, u16 value)
{
	/* 10-bit width */
	dev_write((FC_INHBLANK0), (u8) (value));
	dev_write_mask(FC_INHBLANK1, FC_INHBLANK1_H_IN_BLANK_MASK |
					FC_INHBLANK1_H_IN_BLANK_12_MASK, (u8)(value >> 8));
}

void fc_video_vactive(hdmi_tx_dev_t *dev, u16 value)
{
	/* 11-bit width */
	dev_write((FC_INVACTIV0), (u8) (value));
	dev_write_mask(FC_INVACTIV1, FC_INVACTIV1_V_IN_ACTIV_MASK |
					FC_INVACTIV1_V_IN_ACTIV_12_11_MASK, (u8)(value >> 8));
}

void fc_video_vblank(hdmi_tx_dev_t *dev, u16 value)
{
	/* 8-bit width */
	dev_write((FC_INVBLANK), (u8) (value));
}

void fc_video_hsync_edge_delay(hdmi_tx_dev_t *dev, u16 value)
{
	/* 11-bit width */
	dev_write((FC_HSYNCINDELAY0), (u8) (value));
	dev_write_mask(FC_HSYNCINDELAY1, FC_HSYNCINDELAY1_H_IN_DELAY_MASK |
					FC_HSYNCINDELAY1_H_IN_DELAY_12_MASK, (u8)(value >> 8));
}

void fc_video_hsync_pulse_width(hdmi_tx_dev_t *dev, u16 value)
{
	/* 9-bit width */
	dev_write((FC_HSYNCINWIDTH0), (u8) (value));
	dev_write_mask(FC_HSYNCINWIDTH1, FC_HSYNCINWIDTH1_H_IN_WIDTH_MASK, (u8)(value >> 8));
}

void fc_video_vsync_edge_delay(hdmi_tx_dev_t *dev, u16 value)
{
	/* 8-bit width */
	dev_write((FC_VSYNCINDELAY), (u8) (value));
}

void fc_video_vsync_pulse_width(hdmi_tx_dev_t *dev, u16 value)
{
	dev_write_mask(FC_VSYNCINWIDTH, FC_VSYNCINWIDTH_V_IN_WIDTH_MASK, (u8)(value));
}

void fc_video_refresh_rate(hdmi_tx_dev_t *dev, u32 value)
{
	/* 20-bit width */

	dev_write((FC_INFREQ0), (u8) (value >> 0));
	dev_write((FC_INFREQ1), (u8) (value >> 8));
	dev_write_mask(FC_INFREQ2, FC_INFREQ2_INFREQ_MASK, (u8)(value >> 16));
}

void fc_video_control_period_min_duration(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write((FC_CTRLDUR), value);
}

void fc_video_extended_control_period_min_duration(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write((FC_EXCTRLDUR), value);
}

void fc_video_extended_control_period_max_spacing(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write((FC_EXCTRLSPAC), value);
}

void fc_video_preamble_filter(hdmi_tx_dev_t *dev, u8 value, unsigned channel)
{
	if (channel == 0)
		dev_write((FC_CH0PREAM), value);
	else if (channel == 1)
		dev_write_mask(FC_CH1PREAM, FC_CH1PREAM_CH1_PREAMBLE_FILTER_MASK, value);
	else if (channel == 2)
		dev_write_mask(FC_CH2PREAM, FC_CH2PREAM_CH2_PREAMBLE_FILTER_MASK, value);
	else
		pr_err("invalid channel number: %d", channel);
}

void fc_video_pixel_repetition_input(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_PRCONF, FC_PRCONF_INCOMING_PR_FACTOR_MASK, value);
}

int fc_video_config(hdmi_tx_dev_t *dev, video_params_t *video)
{
	const dtd_t *dtd = &video->mdtd;
	u16 i = 0;

	if((dev == NULL) || (video == NULL)){
		pr_err("Invalid video arguments");
		return HDMI_ERR_VIDEO_ARGS_INVALID;
	}

	dtd = &video->mdtd;

	fc_video_vsync_polarity(dev, dtd->m_vsync_polarity);
	fc_video_hsync_polarity(dev, dtd->m_hsync_polarity);
	fc_video_data_enable_polarity(dev, dev->snps_hdmi_ctrl.data_enable_polarity);
	fc_video_dvi_or_hdmi(dev, video->mhdmi);

	if (video->mhdmi_video_format == HDMI_3D_FORMAT) {
		if (video->m3d_structure == FRAME_PACKING_3D) {
			/* 3d data frame packing is transmitted as a progressive format */
			fc_video_vblank_osc(dev, 0);
			fc_video_interlaced(dev, 0);

			if (dtd->m_interlaced) {
				fc_video_vactive(dev, (dtd->m_vactive << 2) + 3 * dtd->m_vblanking + 2);
			}
			else {
				fc_video_vactive(dev, (dtd->m_vactive << 1) + dtd->m_vblanking);
			}
		}
		else {
			fc_video_vblank_osc(dev, dtd->m_interlaced);
			fc_video_interlaced(dev, dtd->m_interlaced);
			fc_video_vactive(dev, dtd->m_vactive);
		}
	}
	else {
		fc_video_vblank_osc(dev, dtd->m_interlaced);
		fc_video_interlaced(dev, dtd->m_interlaced);
		fc_video_vactive(dev, dtd->m_vactive);
	}

	if(video->mencodingout == YCC420){
		fc_video_hactive(dev, dtd->m_hactive / 2);
		fc_video_hblank(dev, dtd->m_hblanking  /2);
		fc_video_hsync_pulse_width(dev, dtd->m_hsync_pulse_width / 2);
		fc_video_hsync_edge_delay(dev, dtd->m_hsync_offset / 2);
	} else {
		fc_video_hactive(dev, dtd->m_hactive);
		fc_video_hblank(dev, dtd->m_hblanking);
		fc_video_hsync_pulse_width(dev, dtd->m_hsync_pulse_width);
		fc_video_hsync_edge_delay(dev, dtd->m_hsync_offset);
	}

	fc_video_vblank(dev, dtd->m_vblanking);
	fc_video_vsync_edge_delay(dev, dtd->m_vsync_offset);
	fc_video_vsync_pulse_width(dev, dtd->m_vsync_pulse_width);
	fc_video_control_period_min_duration(dev, 12);
	fc_video_extended_control_period_min_duration(dev, 32);

	/* spacing < 256^2 * config / tmdsClock, spacing <= 50ms
	 * worst case: tmdsClock == 25MHz => config <= 19
	 */
	fc_video_extended_control_period_max_spacing(dev, 1);

	for (i = 0; i < 3; i++)
		fc_video_preamble_filter(dev, (i + 1) * 11, i);

	fc_video_pixel_repetition_input(dev, dev->snps_hdmi_ctrl.pixel_repetition + 1);

	return 0;
}

void fc_packets_queue_priority_high(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_CTRLQHIGH, FC_CTRLQHIGH_ONHIGHATTENDED_MASK, value);
}

void fc_packets_queue_priority_low(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_CTRLQLOW, FC_CTRLQLOW_ONLOWATTENDED_MASK, value);
}

void fc_packets_metadata_frame_interpolation(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_DATAUTO1, FC_DATAUTO1_AUTO_FRAME_INTERPOLATION_MASK, value);
}

void fc_packets_metadata_frames_per_packet(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_DATAUTO2, FC_DATAUTO2_AUTO_FRAME_PACKETS_MASK, value);
}

void fc_packets_metadata_line_spacing(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_DATAUTO2, FC_DATAUTO2_AUTO_LINE_SPACING_MASK, value);
}

void fc_packets_auto_send(hdmi_tx_dev_t *dev, u8 enable, u8 mask)
{
	dev_write_mask(FC_DATAUTO0, (1 << mask), (enable ? 1 : 0));
}

void fc_packets_manual_send(hdmi_tx_dev_t *dev, u8 mask)
{
	dev_write_mask(FC_DATMAN, (1 << mask), 1);
}

void fc_packets_disable_all(hdmi_tx_dev_t *dev)
{
	u64 value = ~( BIT(ACP_TX) | BIT(ISRC1_TX) | BIT(ISRC2_TX) | BIT(SPD_TX) | BIT(VSD_TX) );
	dev_write(FC_DATAUTO0, value & dev_read(FC_DATAUTO0));
}

void fc_packets_metadata_config(hdmi_tx_dev_t *dev)
{
	fc_packets_metadata_frame_interpolation(dev, 1);
	fc_packets_metadata_frames_per_packet(dev, 1);
	fc_packets_metadata_line_spacing(dev, 1);
}

void fc_isrc_status(hdmi_tx_dev_t *dev, u8 code)
{
	dev_write_mask(FC_ISCR1_0, FC_ISCR1_0_ISRC_STATUS_MASK, code);
}

void fc_isrc_valid(hdmi_tx_dev_t *dev, u8 validity)
{
	dev_write_mask(FC_ISCR1_0, FC_ISCR1_0_ISRC_VALID_MASK, (validity ? 1 : 0));
}

void fc_isrc_cont(hdmi_tx_dev_t *dev, u8 is_continued)
{
	dev_write_mask(FC_ISCR1_0, FC_ISCR1_0_ISRC_CONT_MASK, (is_continued ? 1 : 0));
}

void fc_isrc_isrc1_codes(hdmi_tx_dev_t *dev, u8 * codes, u8 length)
{
	u8 c = 0;
	if (length > (FC_ISCR1_1 - FC_ISCR1_16 + 1)) {
		length = (FC_ISCR1_1 - FC_ISCR1_16 + 1);
		pr_debug("ISCR1 Codes Truncated");
	}

	for (c = 0; c < length; c++)
		dev_write(FC_ISCR1_1 - c, codes[c]);
}

void fc_isrc_isrc2_codes(hdmi_tx_dev_t *dev, u8 * codes, u8 length)
{
	u8 c = 0;
	if (length > (FC_ISCR2_0 - FC_ISCR2_15 + 1)) {
		length = (FC_ISCR2_0 - FC_ISCR2_15 + 1);
		pr_debug("ISCR2 Codes Truncated");
	}

	for (c = 0; c < length; c++)
		dev_write(FC_ISCR2_0 - c, codes[c]);
}

void fc_gamut_profile(hdmi_tx_dev_t *dev, u8 profile)
{
	dev_write_mask(FC_GMD_HB,FC_GMD_HB_GMDGBD_PROFILE_MASK, profile);
}

void fc_gamut_affected_seq_no(hdmi_tx_dev_t *dev, u8 no)
{
	dev_write_mask(FC_GMD_HB, FC_GMD_HB_GMDAFFECTED_GAMUT_SEQ_NUM_MASK, no);
}

void fc_gamut_packets_per_frame(hdmi_tx_dev_t *dev, u8 packets)
{
	dev_write_mask(FC_GMD_CONF, FC_GMD_CONF_GMDPACKETSINFRAME_MASK, packets);
}

void fc_gamut_packet_line_spacing(hdmi_tx_dev_t *dev, u8 lineSpacing)
{
	dev_write_mask(FC_GMD_CONF, FC_GMD_CONF_GMDPACKETLINESPACING_MASK, lineSpacing);
}

void fc_gamut_content(hdmi_tx_dev_t *dev, const u8 * content, u8 length)
{
	u8 i = 0;
	if (length > (FC_GMD_PB_SIZE)) {
		length = (FC_GMD_PB_SIZE);
		pr_debug("Gamut Content Truncated");
	}

	for (i = 0; i < length; i++)
		dev_write(FC_GMD_PB0 + (i*4), content[i]);
}

void fc_gamut_enable_tx(hdmi_tx_dev_t *dev, u8 enable)
{
	if(enable)
		enable = 1; // ensure value is 1
	dev_write_mask(FC_GMD_EN, FC_GMD_EN_GMDENABLETX_MASK, enable);
}

void fc_gamut_update_packet(hdmi_tx_dev_t *dev)
{
	dev_write_mask(FC_GMD_UP, FC_GMD_UP_GMDUPDATEPACKET_MASK, 1);
}

u8 fc_gamut_current_seq_no(hdmi_tx_dev_t *dev)
{
	return (u8)(dev_read(FC_GMD_STAT) & 0xF);
}

u8 fc_gamut_packet_seq(hdmi_tx_dev_t *dev)
{
	return (u8)(((dev_read(FC_GMD_STAT)) >> 4) & 0x3);
}

u8 fc_gamut_no_current_gbd(hdmi_tx_dev_t *dev)
{
	return (u8)(((dev_read(FC_GMD_STAT)) >> 7) & 0x1);
}

void fc_gamut_config(hdmi_tx_dev_t *dev)
{
	// P0
	fc_gamut_profile(dev, 0x0);

	// P0
	fc_gamut_packets_per_frame(dev, 0x1);
	fc_gamut_packet_line_spacing(dev, 0x1);
}

void fc_gamut_packet_config(hdmi_tx_dev_t *dev, const u8 * gbd_content, u8 length)
{
	fc_gamut_enable_tx(dev, 1);
	fc_gamut_affected_seq_no(dev, (fc_gamut_current_seq_no(dev) + 1) % 16); /* sequential */
	fc_gamut_content(dev, gbd_content, length);
	fc_gamut_update_packet(dev); /* set next_field to 1 */
}

void fc_force_audio(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_DBGFORCE, FC_DBGFORCE_FORCEAUDIO_MASK, bit);
}

void fc_force_video(hdmi_tx_dev_t *dev, u8 bit)
{
	/* avoid glitches */
	if (bit != 0) {
		dev_write(FC_DBGTMDS2, 0x00);	/* R */
		dev_write(FC_DBGTMDS1, 0x00);	/* G */
		dev_write(FC_DBGTMDS0, 0xFF);	/* B */
		dev_write_mask(FC_DBGFORCE, FC_DBGFORCE_FORCEVIDEO_MASK, 1);
	} else {
		dev_write_mask(FC_DBGFORCE, FC_DBGFORCE_FORCEVIDEO_MASK, 0);
		dev_write(FC_DBGTMDS2, 0x00);	/* R */
		dev_write(FC_DBGTMDS1, 0x00);	/* G */
		dev_write(FC_DBGTMDS0, 0x00);	/* B */
	}
}

void fc_force_output(hdmi_tx_dev_t *dev, int enable)
{
	fc_force_audio(dev, 0);
	fc_force_video(dev, (u8)enable);
}

void fc_rgb_ycc(hdmi_tx_dev_t *dev, u8 type)
{
	dev_write_mask(FC_AVICONF0, FC_AVICONF0_RGC_YCC_INDICATION_MASK, type);
}

void fc_scan_info(hdmi_tx_dev_t *dev, u8 left)
{
	dev_write_mask(FC_AVICONF0, FC_AVICONF0_SCAN_INFORMATION_MASK, left);
}

void fc_colorimetry(hdmi_tx_dev_t *dev, unsigned csc_itu)
{
	dev_write_mask(FC_AVICONF1, FC_AVICONF1_COLORIMETRY_MASK, csc_itu);
}

void fc_pic_aspect_ratio(hdmi_tx_dev_t *dev, u8 ar)
{
	dev_write_mask(FC_AVICONF1, FC_AVICONF1_PICTURE_ASPECT_RATIO_MASK, ar);
}

void fc_acctive_aspect_ratio_valid(hdmi_tx_dev_t *dev, u8 valid)
{
	dev_write_mask(FC_AVICONF0, FC_AVICONF0_ACTIVE_FORMAT_PRESENT_MASK, valid);
}

void fc_active_format_aspect_ratio(hdmi_tx_dev_t *dev, u8 left)
{
	dev_write_mask(FC_AVICONF1, FC_AVICONF1_ACTIVE_ASPECT_RATIO_MASK, left);
}

void fc_is_it_content(hdmi_tx_dev_t *dev, u8 it)
{
	dev_write_mask(FC_AVICONF2, FC_AVICONF2_IT_CONTENT_MASK, (it ? 1 : 0));
}

void fc_extended_colorimetry(hdmi_tx_dev_t *dev, u8 ext_color)
{
	dev_write_mask(FC_AVICONF2, FC_AVICONF2_EXTENDED_COLORIMETRY_MASK, ext_color);
	dev_write_mask(FC_AVICONF1, FC_AVICONF1_COLORIMETRY_MASK, 0x3);
}

void fc_quantization_range(hdmi_tx_dev_t *dev, u8 range)
{
	dev_write_mask(FC_AVICONF2, FC_AVICONF2_QUANTIZATION_RANGE_MASK, range);
}

void fc_non_uniform_pic_scaling(hdmi_tx_dev_t *dev, u8 scale)
{
	dev_write_mask(FC_AVICONF2, FC_AVICONF2_NON_UNIFORM_PICTURE_SCALING_MASK, scale);
}

void fc_video_code(hdmi_tx_dev_t *dev, u8 code)
{
	dev_write(FC_AVIVID, code);
}

void fc_horizontal_bars_valid(hdmi_tx_dev_t *dev, u8 validity)
{
	dev_write_mask(FC_AVICONF0, FC_AVICONF0_BAR_INFORMATION_MASK & 0x8, (validity ? 1 : 0));
}

void fc_horizontal_bars(hdmi_tx_dev_t *dev, u16 end_top, u16 start_bottom)
{
	dev_write(FC_AVIETB0, (u8) (end_top));
	dev_write(FC_AVIETB1, (u8) (end_top >> 8));
	dev_write(FC_AVISBB0, (u8) (start_bottom));
	dev_write(FC_AVISBB1, (u8) (start_bottom >> 8));
}

void fc_vertical_bars_valid(hdmi_tx_dev_t *dev, u8 validity)
{
	dev_write_mask(FC_AVICONF0, FC_AVICONF0_BAR_INFORMATION_MASK & 0x4, (validity ? 1 : 0));
}

void fc_vertical_bars(hdmi_tx_dev_t *dev, u16 end_left, u16 start_right)
{
	dev_write(FC_AVIELB0, (u8) (end_left));
	dev_write(FC_AVIELB1, (u8) (end_left >> 8));
	dev_write(FC_AVISRB0, (u8) (start_right));
	dev_write(FC_AVISRB1, (u8) (start_right >> 8));
}

void fc_out_pixel_repetition(hdmi_tx_dev_t *dev, u8 pr)
{
	dev_write_mask(FC_PRCONF, FC_PRCONF_OUTPUT_PR_FACTOR_MASK, pr);
}

u32 fc_get_info_frame_satus(hdmi_tx_dev_t *dev)
{
	return dev_read(FC_AVICONF0);
}

void fc_avi_config(hdmi_tx_dev_t *dev, video_params_t *video_params)
{
	u16 endTop = 0;
	u16 startBottom = 0;
	u16 endLeft = 0;
	u16 startRight = 0;
	dtd_t *dtd = &video_params->mdtd;

	if (video_params->mencodingout == RGB) {
		pr_debug("%s:rgb", __func__);
		fc_rgb_ycc(dev, 0);
	}
	else if (video_params->mencodingout == YCC422) {
		pr_debug("%s:ycc422", __func__);
		fc_rgb_ycc(dev, 1);
	}
	else if (video_params->mencodingout == YCC444) {
		pr_debug("%s:ycc444", __func__);
		fc_rgb_ycc(dev, 2);
	}
	else if (video_params->mencodingout == YCC420) {
		pr_debug("%s:ycc420", __func__);
		fc_rgb_ycc(dev, 3);
	}

	fc_active_format_aspect_ratio(dev, 0x8);

	pr_debug( "%s:infoframe status %x", __func__,
			fc_get_info_frame_satus(dev));

	fc_scan_info(dev, video_params->mscaninfo);

	if (dtd->m_himage_size != 0 || dtd->m_vimage_size != 0) {
		u8 pic = (dtd->m_himage_size * 10) % dtd->m_vimage_size;
		// 16:9 or 4:3
		fc_pic_aspect_ratio(dev, (pic > 5) ? 2 : 1);
	}
	else {
		// No Data
		fc_pic_aspect_ratio(dev, 0);
	}

	fc_is_it_content(dev, video_params->mitcontent);

	fc_quantization_range(dev, video_params->mrgb_quantization_range);
	fc_non_uniform_pic_scaling(dev, video_params->mnon_uniform_scaling);
	if (dtd->m_code != (u8) (-1)) {
		if (video_params->mhdmi20 == 1) {
			fc_video_code(dev, dtd->m_code);
		} else {
			if (dtd->m_code < 110) {
				fc_video_code(dev, dtd->m_code);
			} else {
				fc_video_code(dev, 0);
			}
		}
	} else {
		fc_video_code(dev, 0);
	}
	if (video_params->mcolorimetry == EXTENDED_COLORIMETRY) { /* ext colorimetry valid */
		if (video_params->mext_colorimetry != (u8) (-1)) {
			fc_extended_colorimetry(dev, video_params->mext_colorimetry);
			fc_colorimetry(dev, video_params->mcolorimetry);	/* EXT-3 */
		} else {
			fc_colorimetry(dev, 0);	/* No Data */
		}
	} else {
		fc_colorimetry(dev, video_params->mcolorimetry);	/* NODATA-0/ 601-1/ 709-2/ EXT-3 */
	}
	if (video_params->mactive_format_aspect_ratio != 0) {
		fc_active_format_aspect_ratio(dev, video_params->mactive_format_aspect_ratio);
		fc_acctive_aspect_ratio_valid(dev, 1);
	} else {
		fc_acctive_aspect_ratio_valid(dev, 0);
	}
	if (video_params->mend_top_bar != (u16) (-1) || video_params->mstart_bottom_bar != (u16) (-1)) {
		if (video_params->mend_top_bar != (u16) (-1)) {
			endTop = video_params->mend_top_bar;
		}
		if (video_params->mstart_bottom_bar != (u16) (-1)) {
			startBottom = video_params->mstart_bottom_bar;
		}
		fc_horizontal_bars(dev, endTop, startBottom);
		fc_horizontal_bars_valid(dev, 1);
	} else {
		fc_horizontal_bars_valid(dev, 0);
	}
	if (video_params->mend_left_bar != (u16) (-1) || video_params->mstart_right_bar != (u16) (-1)) {
		if (video_params->mend_left_bar != (u16) (-1)) {
			endLeft = video_params->mend_left_bar;
		}
		if (video_params->mstart_right_bar != (u16) (-1)) {
			startRight = video_params->mstart_right_bar;
		}
		fc_vertical_bars(dev, endLeft, startRight);
		fc_vertical_bars_valid(dev, 1);
	} else {
		fc_vertical_bars_valid(dev, 0);
	}
	fc_out_pixel_repetition(dev, (video_params->mpixel_repetition_factor + 1) - 1);
}

void fc_packet_sample_flat(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_SAMPFLT_MASK, value);
}

void fc_packet_layout(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_AUDSCONF, FC_AUDSCONF_AUD_PACKET_LAYOUT_MASK, bit);
}

void fc_validity_right(hdmi_tx_dev_t *dev, u8 bit, unsigned channel)
{
	if (channel < 4)
		dev_write_mask(FC_AUDSV, (1 << (4 + channel)), bit);
	else
		pr_err("invalid channel number");
}

void fc_validity_left(hdmi_tx_dev_t *dev, u8 bit, unsigned channel)
{
	if (channel < 4)
		dev_write_mask(FC_AUDSV, (1 << channel), bit);
	else
		pr_err("invalid channel number: %d", channel);
}

void fc_user_right(hdmi_tx_dev_t *dev, u8 bit, unsigned channel)
{
	if (channel < 4)
		dev_write_mask(FC_AUDSU, (1 << (4 + channel)), bit);
	else
		pr_err("invalid channel number: %d", channel);
}

void fc_user_left(hdmi_tx_dev_t *dev, u8 bit, unsigned channel)
{
	if (channel < 4)
		dev_write_mask(FC_AUDSU, (1 << channel), bit);
	else
		pr_err("invalid channel number: %d", channel);
}

void fc_iec_cgms_a(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_AUDSCHNL0, FC_AUDSCHNL0_OIEC_CGMSA_MASK, value);
}

void fc_iec_copyright(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(FC_AUDSCHNL0, FC_AUDSCHNL0_OIEC_COPYRIGHT_MASK, bit);
}

void fc_iec_category_code(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write( FC_AUDSCHNL1, value);
}

void fc_iec_pcm_mode(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_AUDSCHNL2, FC_AUDSCHNL2_OIEC_PCMAUDIOMODE_MASK, value);
}

void fc_iec_source(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_AUDSCHNL2, FC_AUDSCHNL2_OIEC_SOURCENUMBER_MASK, value);
}

void fc_iec_channel_right(hdmi_tx_dev_t *dev, u8 value, unsigned channel)
{
	if (channel == 0)
		dev_write_mask(FC_AUDSCHNL3, FC_AUDSCHNL3_OIEC_CHANNELNUMCR0_MASK, value);
	else if (channel == 1)
		dev_write_mask(FC_AUDSCHNL3, FC_AUDSCHNL3_OIEC_CHANNELNUMCR1_MASK, value);
	else if (channel == 2)
		dev_write_mask(FC_AUDSCHNL4, FC_AUDSCHNL4_OIEC_CHANNELNUMCR2_MASK, value);
	else if (channel == 3)
		dev_write_mask(FC_AUDSCHNL4, FC_AUDSCHNL4_OIEC_CHANNELNUMCR3_MASK, value);
	else
		pr_err("invalid channel number: %d", channel);
}

void fc_iec_channel_left(hdmi_tx_dev_t *dev, u8 value, unsigned channel)
{
	if (channel == 0)
		dev_write_mask(FC_AUDSCHNL5, FC_AUDSCHNL5_OIEC_CHANNELNUMCL0_MASK, value);
	else if (channel == 1)
		dev_write_mask(FC_AUDSCHNL5, FC_AUDSCHNL5_OIEC_CHANNELNUMCL1_MASK, value);
	else if (channel == 2)
		dev_write_mask(FC_AUDSCHNL6, FC_AUDSCHNL6_OIEC_CHANNELNUMCL2_MASK, value);
	else if (channel == 3)
		dev_write_mask(FC_AUDSCHNL6, FC_AUDSCHNL6_OIEC_CHANNELNUMCL3_MASK, value);
	else
		pr_err("invalid channel number: %d", channel);
}

void fc_iec_clock_accuracy(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_AUDSCHNL7, FC_AUDSCHNL7_OIEC_CLKACCURACY_MASK, value);
}

void fc_iec_sampling_freq(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_AUDSCHNL7, FC_AUDSCHNL7_OIEC_SAMPFREQ_MASK, value);
}

void fc_iec_original_sampling_freq(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_AUDSCHNL8, FC_AUDSCHNL8_OIEC_ORIGSAMPFREQ_MASK, value);
}

void fc_iec_word_length(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(FC_AUDSCHNL8, FC_AUDSCHNL8_OIEC_WORDLENGTH_MASK, value);
}

void fc_audio_config(hdmi_tx_dev_t *dev, audio_params_t * audio)
{
	int i = 0;
	u8 data = 0;
	u8 channel_count = audio_channel_count(dev, audio);
	dev->snps_hdmi_ctrl.channel_cnt = channel_count;

	// More than 2 channels => layout 1 else layout 0
	if((channel_count + 1) > 2)
		fc_packet_layout(dev, 1);
	else
		fc_packet_layout(dev, 0);

	// iec validity and user bits (IEC 60958-1)
	for (i = 0; i < 4; i++) {
		/* audio_is_channel_en considers left as 1 channel and
		 * right as another (+1), hence the x2 factor in the following */
		/* validity bit is 0 when reliable, which is !IsChannelEn */
		u8 channel_enable = audio_is_channel_en(dev, audio, (2 * i));
		fc_validity_right(dev, !channel_enable, i);		//write 0xff instead

		channel_enable = audio_is_channel_en(dev, audio, (2 * i) + 1);
		fc_validity_left(dev, !channel_enable, i);		//write 0xff instead

		fc_user_right(dev, 1, i);
		fc_user_left(dev, 1, i);
	}

#if 0
	/* IEC - not needed if non-linear PCM */
	fc_iec_cgms_a(dev, audio->miec_cgms_a);
	fc_iec_copyright(dev, audio->miec_copyright ? 0 : 1);
	fc_iec_category_code(dev, audio->miec_category_code);
	fc_iec_pcm_mode(dev, audio->miec_pcm_mode);
	fc_iec_source(dev, audio->miec_source_number);
#endif

	for (i = 0; i < 4; i++) {	/* 0, 1, 2, 3 */
		fc_iec_channel_left(dev, 2 * i + 1, i);	/* 1, 3, 5, 7 */
		fc_iec_channel_right(dev, 2 * (i + 1), i);	/* 2, 4, 6, 8 */
	}

	fc_iec_clock_accuracy(dev, audio->miec_clock_accuracy);

	data = audio_iec_sampling_freq(dev, audio);
	fc_iec_sampling_freq(dev, data);

	data = audio_iec_original_sampling_freq(dev, audio);
	fc_iec_original_sampling_freq(dev, data);

	data = audio_iec_word_length(dev, audio);
	fc_iec_word_length(dev, data);
}

void fc_audio_mute(hdmi_tx_dev_t * dev)
{
	fc_packet_sample_flat(dev, 0xF);
}

void fc_audio_unmute(hdmi_tx_dev_t *dev)
{
	fc_packet_sample_flat(dev, 0);
}

void fc_channel_count(hdmi_tx_dev_t *dev, u8 no_ofchannels)
{
	pr_debug("noOfChannels:%d\n", no_ofchannels);
	dev_write_mask(FC_AUDICONF0, FC_AUDICONF0_CC_MASK, no_ofchannels);
}

void fc_sample_freq(hdmi_tx_dev_t *dev, u8 sf)
{
	dev_write_mask(FC_AUDICONF1, FC_AUDICONF1_SF_MASK, sf);
}

void fc_allocate_channels(hdmi_tx_dev_t *dev, u8 ca)
{
	pr_debug("allocation channel:%d\n", ca);
	dev_write(FC_AUDICONF2, ca);
}

void fc_level_shift_value(hdmi_tx_dev_t *dev, u8 lsv)
{
	dev_write_mask(FC_AUDICONF3, FC_AUDICONF3_LSV_MASK, lsv);
}

void fc_down_mix_inhibit(hdmi_tx_dev_t *dev, u8 prohibited)
{
	dev_write_mask(FC_AUDICONF3, FC_AUDICONF3_DM_INH_MASK, (prohibited ? 1 : 0));
}

void fc_coding_type(hdmi_tx_dev_t *dev, u8 coding_type)
{
	dev_write_mask(FC_AUDICONF0, FC_AUDICONF0_CT_MASK, coding_type);
}

void fc_sampling_size(hdmi_tx_dev_t *dev, u8 ss)
{
	dev_write_mask(FC_AUDICONF1, FC_AUDICONF1_SS_MASK, ss);
}

void fc_audio_info_config(hdmi_tx_dev_t *dev, audio_params_t * audio)
{
	u8 channel_count = audio_channel_count(dev, audio);
	u32 sampling_freq = audio->msampling_frequency;
	fc_channel_count(dev, channel_count);
	fc_allocate_channels(dev, audio->mchannel_allocation);
	fc_level_shift_value(dev, audio->mLevel_shift_value);
	fc_down_mix_inhibit(dev, audio->mdown_mix_inhibit_flag);

	pr_debug("Audio channel count = %d", channel_count);
	pr_debug("Audio channel allocation = %d", audio->mchannel_allocation);
	pr_debug("Audio level shift = %d", audio->mLevel_shift_value);

	/* Audio InfoFrame sample frequency when OBA or DST */
	if (is_equal(sampling_freq, 32000)) {
		fc_sample_freq(dev, 1);
	}
	else if (is_equal(sampling_freq, 44100)) {
		fc_sample_freq(dev, 2);
	}
	else if (is_equal(sampling_freq, 48000)) {
		fc_sample_freq(dev, 3);
	}
	else if (is_equal(sampling_freq, 88200)) {
		fc_sample_freq(dev, 4);
	}
	else if (is_equal(sampling_freq, 96000)) {
		fc_sample_freq(dev, 5);
	}
	else if (is_equal(sampling_freq, 176400)) {
		fc_sample_freq(dev, 6);
	}
	else if (is_equal(sampling_freq, 192000)) {
		fc_sample_freq(dev, 7);
	}
	else {
		fc_sample_freq(dev, 0);
	}

	fc_coding_type(dev, 1);	/* for HDMI refer to stream header  (0) */

	if(is_equal(audio->msample_size, 16)){
		fc_sampling_size(dev, 1);	/* for HDMI refer to stream header  (0) */
	}
	else if(is_equal(audio->msample_size, 24)){
		fc_sampling_size(dev, 3);	/* for HDMI refer to stream header  (0) */
	}
}

void fc_acp_type(hdmi_tx_dev_t *dev, u8 type)
{
	dev_write(FC_ACP0, type);
}

void fc_acp_type_dependent_fields(hdmi_tx_dev_t *dev, u8 * fields, u8 fields_length)
{
	u8 c = 0;
	if (fields_length > (FC_ACP1 - FC_ACP16 + 1)) {
		fields_length = (FC_ACP1 - FC_ACP16 + 1);
		pr_debug("ACP Fields Truncated");
	}

	for (c = 0; c < fields_length; c++)
		dev_write(FC_ACP1 - c, fields[c]);
}

void csc_interpolation(hdmi_tx_dev_t *dev, u8 value)
{
	/* 2-bit width */
	dev_write_mask(CSC_CFG, CSC_CFG_INTMODE_MASK, value);
}

void csc_decimation(hdmi_tx_dev_t *dev, u8 value)
{
	/* 2-bit width */
	dev_write_mask(CSC_CFG, CSC_CFG_DECMODE_MASK, value);
}

void csc_color_depth(hdmi_tx_dev_t *dev, u8 value)
{
	/* 4-bit width */
	dev_write_mask(CSC_SCALE, CSC_SCALE_CSC_COLOR_DEPTH_MASK, value);
}

void csc_scale_factor(hdmi_tx_dev_t *dev, u8 value)
{
	/* 2-bit width */
	dev_write_mask(CSC_SCALE, CSC_SCALE_CSCSCALE_MASK, value);
}

void csc_coefficient_a1(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_A1_LSB, (u8)(value));
	dev_write_mask(CSC_COEF_A1_MSB, CSC_COEF_A1_MSB_CSC_COEF_A1_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_a2(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_A2_LSB, (u8)(value));
	dev_write_mask(CSC_COEF_A2_MSB, CSC_COEF_A2_MSB_CSC_COEF_A2_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_a3(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_A3_LSB, (u8)(value));
	dev_write_mask(CSC_COEF_A3_MSB, CSC_COEF_A3_MSB_CSC_COEF_A3_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_a4(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_A4_LSB, (u8)(value));
	dev_write_mask(CSC_COEF_A4_MSB, CSC_COEF_A4_MSB_CSC_COEF_A4_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_b1(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_B1_LSB, (u8)(value));
	dev_write_mask(CSC_COEF_B1_MSB, CSC_COEF_B1_MSB_CSC_COEF_B1_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_b2(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_B2_LSB, (u8)(value));
	dev_write_mask(CSC_COEF_B2_MSB, CSC_COEF_B2_MSB_CSC_COEF_B2_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_b3(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_B3_LSB, (u8)(value));
	dev_write_mask(CSC_COEF_B3_MSB, CSC_COEF_B3_MSB_CSC_COEF_B3_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_b4(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_B4_LSB, (u8)(value));
	dev_write_mask(CSC_COEF_B4_MSB, CSC_COEF_B4_MSB_CSC_COEF_B4_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_c1(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_C1_LSB, (u8) (value));
	dev_write_mask(CSC_COEF_C1_MSB, CSC_COEF_C1_MSB_CSC_COEF_C1_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_c2(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_C2_LSB, (u8) (value));
	dev_write_mask(CSC_COEF_C2_MSB, CSC_COEF_C2_MSB_CSC_COEF_C2_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_c3(hdmi_tx_dev_t *dev, u16 value)
{
	/* 15-bit width */
	dev_write(CSC_COEF_C3_LSB, (u8) (value));
	dev_write_mask(CSC_COEF_C3_MSB, CSC_COEF_C3_MSB_CSC_COEF_C3_MSB_MASK, (u8)(value >> 8));
}

void csc_coefficient_c4(hdmi_tx_dev_t *dev, u16 value)
{
	dev_write(CSC_COEF_C4_LSB, (u8) (value));
	dev_write_mask(CSC_COEF_C4_MSB, CSC_COEF_C4_MSB_CSC_COEF_C4_MSB_MASK, (u8)(value >> 8));
}

void csc_config(hdmi_tx_dev_t *dev, video_params_t * video,
		unsigned interpolation, unsigned decimation, unsigned color_depth)
{
	if(!video->mcolorimetry){
		video->mcolorimetry = ITU709;
	}
	video_params_get_csc_a(dev, video);
	csc_interpolation(dev, interpolation);
	csc_decimation(dev, decimation);
	csc_coefficient_a1(dev, video->mcsc_a[0]);
	csc_coefficient_a2(dev, video->mcsc_a[1]);
	csc_coefficient_a3(dev, video->mcsc_a[2]);
	csc_coefficient_a4(dev, video->mcsc_a[3]);
	csc_coefficient_b1(dev, video->mcsc_b[0]);
	csc_coefficient_b2(dev, video->mcsc_b[1]);
	csc_coefficient_b3(dev, video->mcsc_b[2]);
	csc_coefficient_b4(dev, video->mcsc_b[3]);
	csc_coefficient_c1(dev, video->mcsc_c[0]);
	csc_coefficient_c2(dev, video->mcsc_c[1]);
	csc_coefficient_c3(dev, video->mcsc_c[2]);
	csc_coefficient_c4(dev, video->mcsc_c[3]);
	csc_scale_factor(dev, video->mcsc_scale);
	csc_color_depth(dev, color_depth);
}

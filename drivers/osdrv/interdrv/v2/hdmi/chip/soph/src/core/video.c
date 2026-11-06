#include "core/video.h"
#include "core/packets.h"
#include "core/fc.h"
#include "core/main_controller.h"
#include "core/hdmi_reg.h"
#include "bsp/access.h"
#include "edid/edid.h"
#include "core/hdmi_core.h"


int video_initialize(hdmi_tx_dev_t *dev, video_params_t * video, u8 data_enable_polarity)
{
	return TRUE;
}

int video_configure(hdmi_tx_dev_t *dev, video_params_t * video)
{
	int res = 0;
	/* DVI mode does not support pixel repetition */
	if ((video->mhdmi == DVI) && video_params_is_pixel_repetition(dev, video)) {
		pr_err("DVI mode with pixel repetition: video not transmitted");
		return HDMI_ERR_VIDEO_CONFIG_FAILED;
	}

	fc_force_output(dev, 1);

	res = fc_video_config(dev, video);
	if (res != 0)
		return res;

	res = video_video_packetizer(dev, video);
	if (res != 0)
		return res;

	res = video_color_space_converter(dev, video);
	if (res != 0)
		return res;

	res = video_video_sampler(dev, video);
	if (res != 0)
		return res;

	return res;
}

int video_color_space_converter(hdmi_tx_dev_t *dev, video_params_t * video)
{
	unsigned interpolation = 0;
	unsigned decimation = 0;
	unsigned color_depth = 0;

	if (video_params_is_color_space_interpolation(dev, video)) {
		if (video->mcsc_filter > 1) {
			pr_err("invalid chroma interpolation filter: %d", video->mcsc_filter);
			return HDMI_ERR_INTERPOLATION_FILTER_INVALID;
		}
		interpolation = 1 + video->mcsc_filter;
	}
	else if (video_params_is_color_space_decimation(dev, video)) {
		if (video->mcsc_filter > 2) {
			pr_err("invalid chroma decimation filter: %d", video->mcsc_filter);
			return 	HDMI_ERR_DECIMATION_FILTER_INVALID;
		}
		decimation = 1 + video->mcsc_filter;
	}

	if ((video->mcolor_resolution == COLOR_DEPTH_24) || (video->mcolor_resolution == 0))
		color_depth = 4;
	else if (video->mcolor_resolution == COLOR_DEPTH_30)
		color_depth = 5;
	else if (video->mcolor_resolution == COLOR_DEPTH_36)
		color_depth = 6;
	else if (video->mcolor_resolution == COLOR_DEPTH_48)
		color_depth = 7;
	else {
		pr_err("invalid color depth: %d", video->mcolor_resolution);
		return HDMI_ERR_COLOR_DEPTH_INVALID;
	}

	csc_config(dev, video, interpolation, decimation, color_depth);

	return 0;
}

int video_video_packetizer(hdmi_tx_dev_t *dev, video_params_t * video)
{
	unsigned color_depth = 0;
	unsigned remap_size = 0;
	unsigned output_select = 0;

	if ((video->mencodingout == RGB) || (video->mencodingout == YCC444) || (video->mencodingout == YCC420)) {
		if (video->mcolor_resolution == 0)
			output_select = 3;
		else if (video->mcolor_resolution == COLOR_DEPTH_24) {
			color_depth = 0;
			output_select = 3;
		} else if (video->mcolor_resolution == COLOR_DEPTH_30)
			color_depth = 5;
		else if (video->mcolor_resolution == COLOR_DEPTH_36)
			color_depth = 6;
		else if (video->mcolor_resolution == COLOR_DEPTH_48)
			color_depth = 7;
		else {
			pr_err("invalid color depth: %d", video->mcolor_resolution);
			return HDMI_ERR_COLOR_DEPTH_INVALID;
		}
	}
	else if (video->mencodingout == YCC422) {
		if ((video->mcolor_resolution == COLOR_DEPTH_24) || (video->mcolor_resolution == 0))
			remap_size = 0;
		else if (video->mcolor_resolution == COLOR_DEPTH_30)
			remap_size = 1;
		else if (video->mcolor_resolution == COLOR_DEPTH_36)
			remap_size = 2;
		else {
			pr_err("invalid color remap size: %d", video->mcolor_resolution);
			return HDMI_ERR_COLOR_REMAP_SIZE_INVALID;
		}
		output_select = 1;
	}
	else {
		pr_err("invalid output encoding type: %d", video->mencodingout);
		return HDMI_ERR_OUTPUT_ENCODETYPE_INVALID;
	}

	vp_pixel_repetition_factor(dev, video->mpixel_repetition_factor);
	vp_color_depth(dev, color_depth);
	vp_pixel_packing_default_phase(dev, video->mpixel_packing_defaultphase);
	vp_ycc422_remap_size(dev, remap_size);
	vp_output_selector(dev, output_select);
	return 0;
}

int video_video_sampler(hdmi_tx_dev_t *dev, video_params_t * video)
{
	unsigned map_code = 0;

	if (video->mencodingin == RGB || video->mencodingin == YCC444 || video->mencodingin == YCC420) {
		if ((video->mcolor_resolution == COLOR_DEPTH_24) || (video->mcolor_resolution == 0))
			map_code = 1;
		else if (video->mcolor_resolution == COLOR_DEPTH_30)
			map_code = 3;
		else if (video->mcolor_resolution == COLOR_DEPTH_36)
			map_code = 5;
		else if (video->mcolor_resolution == COLOR_DEPTH_48)
			map_code = 7;
		else {
			pr_err("invalid color depth: %d", video->mcolor_resolution);
			return HDMI_ERR_COLOR_DEPTH_INVALID;
		}
		map_code += (video->mencodingin != RGB) ? 8 : 0;

		if(video->mencodingin == YCC444)
			map_code = 0x17;

	} else if (video->mencodingin == YCC422) {
		/* YCC422 mapping is discontinued - only map 1 is supported */
		if (video->mcolor_resolution == COLOR_DEPTH_36)
			map_code = 18;
		else if (video->mcolor_resolution == COLOR_DEPTH_30)
			map_code = 20;
		else if ((video->mcolor_resolution == COLOR_DEPTH_24) || (video->mcolor_resolution == 0))
			map_code = 22;
		else {
			pr_err("invalid color remap size: %d", video->mcolor_resolution);
			return HDMI_ERR_COLOR_REMAP_SIZE_INVALID;
		}
	} else {
		pr_err("invalid input encoding type: %d", video->mencodingin);
		return HDMI_ERR_INVALID_INPUT_ENCODETYPE;
	}

	video_sampler_config(dev, map_code);

	return 0;
}

int video_params_get_cea_vic_code(int hdmi_vic_code)
{
	switch(hdmi_vic_code)
	{
	case 1:
		return 95;
		break;
	case 2:
		return 94;
		break;
	case 3:
		return 93;
		break;
	case 4:
		return 98;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

int video_params_get_hhdmi_vic_code(int cea_code)
{
	switch(cea_code)
	{
	case 95:
		return 1;
		break;
	case 94:
		return 2;
		break;
	case 93:
		return 3;
		break;
	case 98:
		return 4;
		break;
	default:
		return -1;
		break;
	}
	return -1;
}

void video_params_reset(hdmi_tx_dev_t *dev, video_params_t * params)
{
	params->mhdmi = MODE_UNDEFINED;
	params->mencodingout = RGB;
	params->mencodingin = RGB;
	params->mcolor_resolution = COLOR_DEPTH_24;
	params->mpixel_repetition_factor = 0;
	params->mrgb_quantization_range = 0;
	params->mpixel_packing_defaultphase = 0;
	params->mcolorimetry = 0;
	params->mscaninfo = 0;
	params->mactive_format_aspect_ratio = 8;
	params->mnon_uniform_scaling = 0;
	params->mext_colorimetry = ~0;
	params->mitcontent = 0;
	params->mend_top_bar = ~0;
	params->mstart_bottom_bar = ~0;
	params->mend_left_bar = ~0;
	params->mstart_right_bar = ~0;
	params->mcsc_filter = 0;
	params->mhdmi_video_format = UNDEFINED_FORMAT;
	params->m3d_structure = UNDEFINED_3D;
	params->m3d_ext_data = UNDEFINED_EXTDATA_3D;
	params->mhdmi_vic = 0;
	params->mhdmi20 = 0;

#if 0
	params->mdtd.m_code = 0;
	params->mdtd.m_limited_to_ycc420 = 0xFF;
	params->mdtd.m_ycc420 = 0xFF;
	params->mdtd.m_pixel_repetition_input = 0xFF;
	params->mdtd.m_pixel_clock = 0.0;
	params->mdtd.m_interlaced = 0xFF;
	params->mdtd.m_hactive = 0;
	params->mdtd.m_hblanking = 0;
	params->mdtd.m_hborder = 0xFFFF;
	params->mdtd.m_himage_size = 0;
	params->mdtd.m_hsync_offset = 0;
	params->mdtd.m_hsync_pulse_width = 0;
	params->mdtd.m_hsync_polarity = 0xFF;
	params->mdtd.m_vactive = 0;
	params->mdtd.m_vblanking = 0;
	params->mdtd.m_vborder = 0xFFFF;
	params->mdtd.m_vimage_size = 0;
	params->mdtd.m_vsync_offset = 0;
	params->mdtd.m_vsync_pulse_width = 0;
	params->mdtd.m_vsync_polarity = 0xFF;
#endif

}

u16 *video_params_get_csc_a(hdmi_tx_dev_t *dev, video_params_t * params)
{
	video_params_update_csc_coefficients(dev, params);
	return params->mcsc_a;
}

void video_params_set_csc_a(hdmi_tx_dev_t *dev, video_params_t * params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mcsc_a) / sizeof(params->mcsc_a[0]); i++) {
		params->mcsc_a[i] = value[i];
	}
}

u16 *video_params_get_csc_b(hdmi_tx_dev_t *dev, video_params_t * params)
{
	video_params_update_csc_coefficients(dev, params);
	return params->mcsc_b;
}

void video_params_set_csc_b(hdmi_tx_dev_t *dev, video_params_t * params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mcsc_b) / sizeof(params->mcsc_b[0]); i++) {
		params->mcsc_b[i] = value[i];
	}
}

u16 *video_params_get_csc_c(hdmi_tx_dev_t *dev, video_params_t * params)
{
	video_params_update_csc_coefficients(dev, params);
	return params->mcsc_c;
}

void video_params_set_csc_c(hdmi_tx_dev_t *dev, video_params_t * params, u16 value[4])
{
	u16 i = 0;
	for (i = 0; i < sizeof(params->mcsc_c) / sizeof(params->mcsc_c[0]); i++) {
		params->mcsc_c[i] = value[i];
	}
}

void video_params_set_csc_scale(hdmi_tx_dev_t *dev, video_params_t * params, u16 value)
{
	params->mcsc_scale = value;
}

/* [0.01 MHz] */
u32 video_params_get_pixel_clock(hdmi_tx_dev_t *dev, video_params_t * params)
{
	u32 pixelClock = 0;
	pixelClock = params->mdtd.m_pixel_clock;

	pr_debug("Pixel clock %xMHz", pixelClock);

	return pixelClock;
}

/* 0.01 */
u32 video_params_get_ratio_clock(hdmi_tx_dev_t *dev, video_params_t * params)
{
	u32 ratio = 1000;

	if (params->mencodingout != YCC422) {
		if (params->mcolor_resolution == COLOR_DEPTH_24) {
			ratio = 1000;
		} else if (params->mcolor_resolution == COLOR_DEPTH_30) {
			ratio = 1250;
		} else if (params->mcolor_resolution == COLOR_DEPTH_36) {
			ratio = 1500;
		} else if (params->mcolor_resolution == COLOR_DEPTH_48) {
			ratio = 2000;
		}
	}
	return ratio * (params->mpixel_repetition_factor + 1);
}

int video_params_is_color_space_conversion(hdmi_tx_dev_t *dev, video_params_t * params)
{
	return params->mencodingin != params->mencodingout;
}

int video_params_is_color_space_decimation(hdmi_tx_dev_t *dev, video_params_t * params)
{
	return params->mencodingout == YCC422 && (params->mencodingin == RGB
			|| params->mencodingin ==
					YCC444);
}

int video_params_is_color_space_interpolation(hdmi_tx_dev_t *dev, video_params_t * params)
{
	return params->mencodingin == YCC422 && (params->mencodingout == RGB
			|| params->mencodingout ==
					YCC444);
}

int video_params_is_pixel_repetition(hdmi_tx_dev_t *dev, video_params_t * params)
{
	return (params->mpixel_repetition_factor > 0) || (params->mdtd.m_pixel_repetition_input > 0);
}

void video_params_update_csc_coefficients(hdmi_tx_dev_t *dev, video_params_t * params)
{
	u16 i = 0;
	if (!video_params_is_color_space_conversion(dev, params)) {
		for (i = 0; i < 4; i++) {
			params->mcsc_a[i] = 0;
			params->mcsc_b[i] = 0;
			params->mcsc_c[i] = 0;
		}
		params->mcsc_a[0] = 0x2000;
		params->mcsc_b[1] = 0x2000;
		params->mcsc_c[2] = 0x2000;
		params->mcsc_scale = 1;
	} else if (video_params_is_color_space_conversion(dev, params) && params->mencodingout == RGB) {
		if (params->mcolorimetry == ITU601) {
			params->mcsc_a[0] = 0x2000;
			params->mcsc_a[1] = 0x6926;
			params->mcsc_a[2] = 0x74fd;
			params->mcsc_a[3] = 0x010e;

			params->mcsc_b[0] = 0x2000;
			params->mcsc_b[1] = 0x2cdd;
			params->mcsc_b[2] = 0x0000;
			params->mcsc_b[3] = 0x7e9a;

			params->mcsc_c[0] = 0x2000;
			params->mcsc_c[1] = 0x0000;
			params->mcsc_c[2] = 0x38b4;
			params->mcsc_c[3] = 0x7e3b;

			params->mcsc_scale = 1;
		} else if (params->mcolorimetry == ITU709) {
			params->mcsc_a[0] = 0x2000;
			params->mcsc_a[1] = 0x7106;
			params->mcsc_a[2] = 0x7a02;
			params->mcsc_a[3] = 0x00a7;

			params->mcsc_b[0] = 0x2000;
			params->mcsc_b[1] = 0x3264;
			params->mcsc_b[2] = 0x0000;
			params->mcsc_b[3] = 0x7e6d;

			params->mcsc_c[0] = 0x2000;
			params->mcsc_c[1] = 0x0000;
			params->mcsc_c[2] = 0x3b61;
			params->mcsc_c[3] = 0x7e25;

			params->mcsc_scale = 1;
		}
	} else if (video_params_is_color_space_conversion(dev, params) && params->mencodingin == RGB) {
		if (params->mcolorimetry == ITU601) {
			params->mcsc_a[0] = 0x2591;
			params->mcsc_a[1] = 0x1322;
			params->mcsc_a[2] = 0x074b;
			params->mcsc_a[3] = 0x0000;

			params->mcsc_b[0] = 0x6535;
			params->mcsc_b[1] = 0x2000;
			params->mcsc_b[2] = 0x7acc;
			params->mcsc_b[3] = 0x0200;

			params->mcsc_c[0] = 0x6acd;
			params->mcsc_c[1] = 0x7534;
			params->mcsc_c[2] = 0x2000;
			params->mcsc_c[3] = 0x0200;

			params->mcsc_scale = 0;
		} else if (params->mcolorimetry == ITU709) {
			params->mcsc_a[0] = 0x2dc5;
			params->mcsc_a[1] = 0x0d9b;
			params->mcsc_a[2] = 0x049e;
			params->mcsc_a[3] = 0x0000;

			params->mcsc_b[0] = 0x62f0;
			params->mcsc_b[1] = 0x2000;
			params->mcsc_b[2] = 0x7d11;
			params->mcsc_b[3] = 0x0200;

			params->mcsc_c[0] = 0x6756;
			params->mcsc_c[1] = 0x78ab;
			params->mcsc_c[2] = 0x2000;
			params->mcsc_c[3] = 0x0200;

			params->mcsc_scale = 0;
		}
	}
	/* else use user coefficients */
}

void video_params_set_ycc420_support(hdmi_tx_dev_t *dev, dtd_t * params_dtd, short_video_desc_t * params_svd)
{
	params_dtd->m_limited_to_ycc420 = params_svd->m_limited_to_ycc420;
	params_dtd->m_ycc420 = params_svd->m_ycc420;
	pr_debug("set ParamsDtd->limite %d", params_dtd->m_limited_to_ycc420);
	pr_debug("set ParamsDtd->supports %d", params_dtd->m_ycc420);
	pr_debug("set ParamsSvd->limite %d", params_svd->m_limited_to_ycc420);
	pr_debug("set ParamsSvd->supports %d", params_svd->m_ycc420);
}


char * get_encoding_string(encoding_t encoding)
{
	switch (encoding){
		case 	RGB: return "RGB";
		case	YCC444: return "YCbCr-444";
		case	YCC422: return "YCbCr-422";
		case	YCC420: return "YCbCr-420";
		default :break;
	}
	return "Undefined";
}

void hal_video_sampler_internal_data_enable_generator(hdmi_tx_dev_t *dev, u8 bit)
{
	dev_write_mask(TX_INVID0, TX_INVID0_INTERNAL_DE_GENERATOR_MASK, (bit ? 1 : 0));
}

void hal_video_sampler_video_mapping(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(TX_INVID0, TX_INVID0_VIDEO_MAPPING_MASK, value);
}

void hal_video_sampler_stuffing_gy(hdmi_tx_dev_t *dev, u16 value)
{
	dev_write((TX_GYDATA0), (u8) (value >> 0));
	dev_write((TX_GYDATA1), (u8) (value >> 8));
	dev_write_mask(TX_INSTUFFING, TX_INSTUFFING_GYDATA_STUFFING_MASK, 1);
}

void hal_video_sampler_stuffing_rcr(hdmi_tx_dev_t *dev, u16 value)
{

	dev_write((TX_RCRDATA0), (u8) (value >> 0));
	dev_write((TX_RCRDATA1), (u8) (value >> 8));
	dev_write_mask(TX_INSTUFFING, TX_INSTUFFING_RCRDATA_STUFFING_MASK, 1);
}

void hal_video_sampler_stuffing_bcb(hdmi_tx_dev_t *dev, u16 value)
{
	dev_write((TX_BCBDATA0), (u8) (value >> 0));
	dev_write((TX_BCBDATA1), (u8) (value >> 8));
	dev_write_mask(TX_INSTUFFING, TX_INSTUFFING_BCBDATA_STUFFING_MASK, 1);
}

void video_sampler_config(hdmi_tx_dev_t *dev, u8 map_code)
{
	hal_video_sampler_internal_data_enable_generator(dev, 0);
	hal_video_sampler_video_mapping(dev, map_code);
	hal_video_sampler_stuffing_gy(dev, 0);
	hal_video_sampler_stuffing_rcr(dev, 0);
	hal_video_sampler_stuffing_bcb(dev, 0);
}
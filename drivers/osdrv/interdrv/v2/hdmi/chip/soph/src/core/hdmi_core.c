#include "hdmi_core.h"
#include "hdmi_proc.h"
#include "api.h"
#include "hdmi_reg.h"
#include "core/irq.h"
#include "phy/phy.h"
#include "hdcp/hdcp.h"
#include "bsp/i2cm.h"
#include "bsp/access.h"
#include "common/hdmi_debug.h"
#include <uapi/linux/sched/types.h>
#include <linux/sched.h>
#include "common/hdmi_ioctl.h"
#include "disp.h"
#include "dsi_phy.h"
#include <linux/compat.h>

/** @short License information */
MODULE_LICENSE("GPL v2");
/** @short Author information */
MODULE_AUTHOR("yu.yang");
/** @short Device description */
MODULE_DESCRIPTION("HDMI_TX module driver");
/** @short Device version */
MODULE_VERSION("1.0");


static struct list_head devlist_global =  LIST_HEAD_INIT(devlist_global);
static struct mem_alloc *alloc_list;
struct hdmitx_dev *dev_hdmi;
struct hdmi_tx_ctx *ctx;
static bool hdmitx_state = FALSE;
static int hdmitx_event_id = 0;
static bool is_resume = false;
static int scrambling_low_rates = -1;

static struct fasync_struct *hdmi_fasync;
static atomic_t  dev_open_cnt;

#define HDMI_DEV_NAME   "soph-hdmi"
#define HDMI_CLASS_NAME "soph-hdmi"
#define MIN(a, b) (((a) < (b))?(a):(b))
#define MAX(a, b) (((a) > (b))?(a):(b))
#define DISP1 1

u32 hdmi_log_lv = CVI_DBG_DEBUG/*CVI_DBG_INFO*/;
module_param(hdmi_log_lv, int, 0644);

const unsigned dtd_size = 0x12;

void _fill_disp_timing(struct disp_timing *timing, dtd_t *mdtd)
{
	timing->vtotal = mdtd->m_vactive + mdtd->m_vblanking - 1;
	timing->htotal = mdtd->m_hactive + mdtd->m_hblanking - 1;
	timing->vsync_start = 0;
	timing->vsync_end = mdtd->m_vsync_pulse_width - 1;
	timing->vfde_start = timing->vmde_start =
		mdtd->m_vsync_pulse_width +
		(mdtd->m_vblanking - mdtd->m_vsync_pulse_width - mdtd->m_vsync_offset);
	timing->vfde_end = timing->vmde_end =
		timing->vfde_start + mdtd->m_vactive - 1;
	timing->hsync_start = 0;
	timing->hsync_end = mdtd->m_hsync_pulse_width - 1;
	timing->hfde_start = timing->hmde_start =
		mdtd->m_hsync_pulse_width +
		(mdtd->m_hblanking - mdtd->m_hsync_pulse_width - mdtd->m_hsync_offset);
	timing->hfde_end = timing->hmde_end =
		timing->hfde_start + mdtd->m_hactive - 1;
	timing->vsync_pol = !(mdtd->m_vsync_polarity);
	timing->hsync_pol = !(mdtd->m_hsync_polarity);
}

void disp_hdmi_gen(dtd_t *mdtd)
{
	struct disp_timing timing;

	_fill_disp_timing(&timing, mdtd);
	disp_set_timing(DISP1, &timing);
	disp_clk_enable(DISP1, TRUE);
	disp_tgen_enable(DISP1, TRUE);
}

struct hdmi_tx_ctx* get_hdmi_ctx()
{
	return ctx;
}

int sink_capability(hdmi_sink_capability* hdmi_sink_cap)
{
	int i, j = 0, k;
	sink_edid_t * sink_cap = NULL;

	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	sink_cap = ctx->mode.sink_cap;
	if(sink_cap == NULL){
		pr_err("%s:sink_cap is NULL\n", __func__);
		return HDMI_ERR_READ_SINK_FAILED;
	}

	hdmi_sink_cap->is_connected = hdmi_sink_cap->is_sink_power_on
							   = dev_read_mask((PHY_STAT0), PHY_STAT0_HPD_MASK);
	hdmi_sink_cap->support_hdmi = ctx->mode.pvideo.mhdmi;
	hdmi_sink_cap->native_video_format = sink_cap->edid_msvd[0].m_code;

	for(i = 0; (i < 128) && (ctx->mode.sink_capinfo[i].sink_cap_info.m_code != 0); i++){
		hdmi_sink_cap->support_video_format[j].mcode = ctx->mode.sink_capinfo[i].sink_cap_info.m_code;
		hdmi_sink_cap->support_video_format[j].fresh_rate = ctx->mode.sink_capinfo[i].fresh_rate;
		hdmi_sink_cap->support_video_format[j].timing_info.hact = ctx->mode.sink_capinfo[i].sink_cap_info.m_hactive;
		hdmi_sink_cap->support_video_format[j].timing_info.interlace =
					ctx->mode.sink_capinfo[i].sink_cap_info.m_interlaced;

		hdmi_sink_cap->support_video_format[j].timing_info.vact =
					ctx->mode.sink_capinfo[i].sink_cap_info.m_interlaced ?
					ctx->mode.sink_capinfo[i].sink_cap_info.m_vactive * 2 :
					ctx->mode.sink_capinfo[i].sink_cap_info.m_vactive;

		hdmi_sink_cap->support_video_format[j++].timing_info.pixel_clk =
					ctx->mode.sink_capinfo[i].sink_cap_info.m_pixel_clock;
	}

	hdmi_sink_cap->support_xvycc709 = ctx->mode.sink_cap->xv_ycc709;
	hdmi_sink_cap->support_xvycc601 = ctx->mode.sink_cap->xv_ycc601;

	hdmi_sink_cap->support_ycbcr = (sink_cap->edid_mycc444_support || sink_cap->edid_mycc422_support
								  || sink_cap->edid_mycc420_support);
	hdmi_sink_cap->hdcp14_en = ctx->hdmi_tx.snps_hdmi_ctrl.hdcp_on;
	hdmi_sink_cap->hdmi_video_input = (hdmi_video_mode)ctx->mode.pvideo.mencodingin;
	hdmi_sink_cap->hdmi_video_output = (hdmi_video_mode)ctx->mode.pvideo.mencodingout;
	hdmi_sink_cap->version = ctx->mode.edid.version;
	hdmi_sink_cap->revision = ctx->mode.edid.revision;
	hdmi_sink_cap->support_dvi_dual = sink_cap->edid_mhdmivsdb.m_dvi_dual;
	hdmi_sink_cap->support_deepcolor_ycbcr444 = sink_cap->edid_mhdmivsdb.m_deep_color_y444;
	hdmi_sink_cap->support_deep_color_30bit = sink_cap->edid_mhdmivsdb.m_deep_color30;
	hdmi_sink_cap->support_deep_color_36bit = sink_cap->edid_mhdmivsdb.m_deep_color36;
	hdmi_sink_cap->support_deep_color_48bit = sink_cap->edid_mhdmivsdb.m_deep_color48;
	hdmi_sink_cap->support_y420_dc_30bit = sink_cap->edid_mhdmi_forumvsdb.mdc_30bit_420;
	hdmi_sink_cap->support_y420_dc_36bit = sink_cap->edid_mhdmi_forumvsdb.mdc_36bit_420;
	hdmi_sink_cap->support_y420_dc_48bit = sink_cap->edid_mhdmi_forumvsdb.mdc_48bit_420;
	hdmi_sink_cap->support_ai = sink_cap->edid_mhdmivsdb.m_supports_ai;			 // TBD
	hdmi_sink_cap->max_tmds_clk = sink_cap->edid_mhdmi_forumvsdb.m_max_tmds_char_rate;

	hdmi_sink_cap->support_hdmi_2_0 = sink_cap->edid_m20sink;
	hdmi_sink_cap->ycc_quant_selectable = hdmi_sink_cap->rgb_quant_selectable
									   = sink_cap->edid_mvideo_capability_datablock.mquantization_range_selectable;
	hdmi_sink_cap->detailed_timing.detail_timing[0].hact = ctx->mode.pvideo.mdtd.m_hactive;
	hdmi_sink_cap->detailed_timing.detail_timing[0].hbb = ctx->mode.pvideo.mdtd.m_hblanking
			- ctx->mode.pvideo.mdtd.m_hsync_offset - ctx->mode.pvideo.mdtd.m_hsync_pulse_width;
	hdmi_sink_cap->detailed_timing.detail_timing[0].hfb = ctx->mode.pvideo.mdtd.m_hsync_offset;
	hdmi_sink_cap->detailed_timing.detail_timing[0].hpw = ctx->mode.pvideo.mdtd.m_hsync_pulse_width;
	hdmi_sink_cap->detailed_timing.detail_timing[0].vact = ctx->mode.pvideo.mdtd.m_vactive;
	hdmi_sink_cap->detailed_timing.detail_timing[0].vbb = ctx->mode.pvideo.mdtd.m_vblanking
			- ctx->mode.pvideo.mdtd.m_vsync_offset - ctx->mode.pvideo.mdtd.m_vsync_pulse_width;
	hdmi_sink_cap->detailed_timing.detail_timing[0].vfb = ctx->mode.pvideo.mdtd.m_vsync_offset;
	hdmi_sink_cap->detailed_timing.detail_timing[0].vpw = ctx->mode.pvideo.mdtd.m_vsync_pulse_width;
	hdmi_sink_cap->detailed_timing.detail_timing[0].ihs = ctx->mode.pvideo.mdtd.m_hsync_polarity;
	hdmi_sink_cap->detailed_timing.detail_timing[0].ivs = ctx->mode.pvideo.mdtd.m_vsync_polarity;
	hdmi_sink_cap->detailed_timing.detail_timing[0].interlace = ctx->mode.pvideo.mdtd.m_interlaced;
	hdmi_sink_cap->detailed_timing.detail_timing[0].img_width = ctx->mode.pvideo.mdtd.m_hactive;
	hdmi_sink_cap->detailed_timing.detail_timing[0].img_height = ctx->mode.pvideo.mdtd.m_vactive;
	hdmi_sink_cap->detailed_timing.detail_timing[0].aspect_ratio_w = ctx->mode.pvideo.mdtd.m_himage_size;
	hdmi_sink_cap->detailed_timing.detail_timing[0].aspect_ratio_h = ctx->mode.pvideo.mdtd.m_vimage_size;
	hdmi_sink_cap->detailed_timing.detail_timing[0].pixel_clk = ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock;
	hdmi_sink_cap->audio_info[0].audio_chn = sink_cap->edid_msad->m_max_channels;
	hdmi_sink_cap->audio_info_num = 1;

	for(k = 0; ctx->mode.sink_cap->support_sample_rate[k] != 0; k++) {
		hdmi_sink_cap->audio_info[0].support_sample_rate[k] = ctx->mode.sink_cap->support_sample_rate[k];
	}
	hdmi_sink_cap->audio_info[0].support_sample_rate_num = k - 1;
	hdmi_sink_cap->audio_info[0].max_bit_rate = hdmi_sink_cap->audio_info[0].support_sample_rate[0];

	/* choose highest sample size supported by sink */
	if (sink_cap->edid_msad[0].mformat == 1) {
		for(k = 0; ctx->mode.sink_cap->support_bit_depth[k] != 0; k++) {
			hdmi_sink_cap->audio_info[0].support_bit_depth[k] = ctx->mode.sink_cap->support_bit_depth[k];
		}
	} else {
		k = 0;
		hdmi_sink_cap->audio_info[0].support_bit_depth[k++] = 0;
	}
	hdmi_sink_cap->audio_info[0].support_bit_depth_num = k;

#if 0
	hdmi_sink_cap->edid_ex_blk_num = 1;			// TBD
	hdmi_sink_cap->i_latency_fields_present = 0;
	hdmi_sink_cap->latency_fields_present = 0;
	hdmi_sink_cap->hdmi_video_present = 0;
#endif

	hdmi_sink_cap->video_latency = sink_cap->edid_mhdmivsdb.m_video_latency;
	hdmi_sink_cap->audio_latency = sink_cap->edid_mhdmivsdb.m_audio_latency;
	hdmi_sink_cap->interlaced_video_latency = sink_cap->edid_mhdmivsdb.m_interlaced_video_latency;
	hdmi_sink_cap->interlaced_audio_latency = sink_cap->edid_mhdmivsdb.m_interlaced_audio_latency;

	return 0;
}

int edid_tx_supports_cea_code(u32 cea_code){
	int i = 0;

	if(ctx->edid_tx_checks == 0){
		return TRUE;
	}

	if(ctx->tx_sink_cap == NULL){
		return HDMI_ERR_READ_EDID_FAILED;
	}

	for(i = 0; (i < EDID_SVD_ARRAY_SIZE) && (ctx->tx_sink_cap->edid_msvd[i].m_code != 0); i++){
		if(ctx->tx_sink_cap->edid_msvd[i].m_code == cea_code){
			return TRUE;
		}
	}

	for(i = 0; (i < 4) && (ctx->tx_sink_cap->edid_mhdmivsdb.m_hdmi_vic[i] != 0); i++){
		if(ctx->tx_sink_cap->edid_mhdmivsdb.m_hdmi_vic[i] == video_params_get_hhdmi_vic_code(cea_code)){
			return TRUE;
		}
	}
	return FALSE;
}

int edid_parser_cea_ext_reset(hdmi_tx_dev_t *dev, sink_edid_t *edid_ext)
{
	unsigned i = 0;
	edid_ext->edid_m20sink = FALSE;
#if 1
	for (i = 0; i < sizeof(edid_ext->edid_mmonitor_name); i++) {
		edid_ext->edid_mmonitor_name[i] = 0;
	}
	edid_ext->edid_mbasic_audio_support = FALSE;
	edid_ext->edid_munder_scan_support = FALSE;
	edid_ext->edid_mycc422_support = FALSE;
	edid_ext->edid_mycc444_support = FALSE;
	edid_ext->edid_mycc420_support = FALSE;
	edid_ext->edid_mdtd_index = 0;
	edid_ext->edid_msad_index = 0;
	edid_ext->edid_msvd_index = 0;
#endif
	hdmivsdb_reset(dev, &edid_ext->edid_mhdmivsdb);
	hdmiforumvsdb_reset(dev, &edid_ext->edid_mhdmi_forumvsdb);
	monitor_range_limits_reset(dev, &edid_ext->edid_mmonitor_range_limits);
	video_cap_data_block_reset(dev, &edid_ext->edid_mvideo_capability_datablock);
	colorimetry_data_block_reset(dev, &edid_ext->edid_mcolorimetry_datablock);
	speaker_alloc_data_block_reset(dev, &edid_ext->edid_mspeaker_allocation_datablock);
	return TRUE;
}

void edid_parser_update_ycc420(sink_edid_t *edid_ext, u8 ycc420_all, u8 limited_to_ycc420all)
{
	u16 edid_cnt = 0;
	for (edid_cnt = 0;edid_cnt < edid_ext->edid_msvd_index;edid_cnt++) {
		switch (edid_ext->edid_msvd[edid_cnt].m_code){
		case 96:
		case 97:
		case 101:
		case 102:
		case 106:
		case 107:
			ycc420_all == 1 ? edid_ext->edid_msvd[edid_cnt].m_ycc420 = ycc420_all : 0;
			limited_to_ycc420all == 1 ?
					edid_ext->edid_msvd[edid_cnt].m_limited_to_ycc420 = limited_to_ycc420all : 0;
			break;
		default:
			break;
		}
	}
}

int edid_parser_parse_data_block(hdmi_tx_dev_t *dev, u8 * data, sink_edid_t *edid_ext)
{
	u8 c = 0;
	short_audio_desc_t tmp_sad;
	short_video_desc_t tmp_svd;
	u8 tmp_ycc420_all = 0;
	u8 tmp_limited_ycc420all = 0;
	u8 extended_tag = 0;
	u32 ieee_id = 0;
	int svd_nr = 0;
	int i = 0;
	int icnt = 0;
	int edid_cnt = 0;
	u8 tag = bit_field(data[0], 5, 3);
	u8 length = bit_field(data[0], 0, 5);
	tmp_svd.m_limited_to_ycc420 = 0;
	tmp_svd.m_ycc420 = 0;

	switch (tag) {
	case 0x1:		/* Audio Data Block */
		pr_debug("EDID: Audio datablock parsing\n");
		for (c = 1; c < (length + 1); c += 3) {
			sad_parse(dev, &tmp_sad, data + c);
			if (edid_ext->edid_msad_index < (sizeof(edid_ext->edid_msad) / sizeof(short_audio_desc_t))) {
				edid_ext->edid_msad[edid_ext->edid_msad_index++] = tmp_sad;
			} else {
				pr_err("buffer full - SAD ignored\n");
			}
		}
		break;
	case 0x2:		/* Video Data Block */
		pr_debug("EDID: Video datablock parsing\n");
		for (c = 1; c < (length + 1); c++) {
			svd_parse(dev, &tmp_svd, data[c]);
			if (edid_ext->edid_msvd_index < (sizeof(edid_ext->edid_msvd) / sizeof(short_video_desc_t))) {
				edid_ext->edid_msvd[edid_ext->edid_msvd_index++] = tmp_svd;
			} else {
				pr_err("buffer full - SVD ignored\n");
			}
		}
		break;
	case 0x3:		/* Vendor Specific Data Block HDMI or HF */
		pr_debug("EDID: VSDB HDMI and HDMI-F\n ");
		ieee_id = byte_to_dword(0x00, data[3], data[2], data[1]);
		if (ieee_id == 0x000C03) {	/* HDMI */
			if (hdmivsdb_parse(dev, &edid_ext->edid_mhdmivsdb, data) != TRUE) {
				pr_err("HDMI Vendor Specific Data Block corrupt");
				break;
			}
			pr_debug("EDID HDMI VSDB parsed");
		} else {
			if (ieee_id == 0xC45DD8) {	/* HDMI-F */
				pr_debug("Sink is HDMI 2.0 because haves HF-VSDB\n");
				edid_ext->edid_m20sink = TRUE;
				scrambling_low_rates = data[6] & 0x8;
				if (hdmiforumvsdb_parse(dev, &edid_ext->edid_mhdmi_forumvsdb, data) != TRUE) {
					pr_err("HDMI Vendor Specific Data Block corrupt");
					break;
				} else {
#if 0
					if (edid_ext->edid_mhdmi_forumvsdb.m_lts_340mcs_scramble == 1) {
						scrambling_enable(baseAddr, 1);
						pr_debug("Scrambling enable by Sink HF-VSDB");
					} else {
						scrambling_enable(baseAddr, 0);
						pr_debug("Scrambling disabled by Sink HF-VSDB");
					}
#endif
				}
			} else {
				pr_err("Vendor Specific Data Block not parsed ieee_id: 0x%x",
						ieee_id);
			}
		}
		break;
	case 0x4:		/* Speaker Allocation Data Block */
		pr_debug("SAD block parsing");
		if (speaker_alloc_data_block_parse(dev, &edid_ext->edid_mspeaker_allocation_datablock, data) != TRUE) {
			pr_err("Speaker Allocation Data Block corrupt");
		}
		break;
	case 0x7:{
		pr_debug("EDID CEA Extended field 0x07\n");
		extended_tag = data[1];
		switch (extended_tag) {
		case 0x00:	/* Video Capability Data Block */
			pr_debug("Video Capability Data Block\n");
			if (video_cap_data_block_parse(dev, &edid_ext->edid_mvideo_capability_datablock, data) != TRUE) {
				pr_err("Video Capability Data Block corrupt");
			}
			break;
		case 0x05:	/* Colorimetry Data Block */
			pr_debug("Colorimetry Data Block");
			if (colorimetry_data_block_parse(dev, &edid_ext->edid_mcolorimetry_datablock, data) != TRUE) {
				pr_err("Colorimetry Data Block corrupt");
			}
			break;
		case 0x04:	/* HDMI Video Data Block */
			pr_debug("HDMI Video Data Block");
			break;
		case 0x12:	/* HDMI Audio Data Block */
			pr_debug("HDMI Audio Data Block");
			break;
		case 0xe:
			/** If it is a YCC420 VDB then VICs can ONLY be displayed in YCC 4:2:0 */
			pr_debug("YCBCR 4:2:0 Video Data Block\n");
			/** If Sink has YCC Datablocks it is HDMI 2.0 */
			edid_ext->edid_m20sink = TRUE;
			tmp_limited_ycc420all = (bit_field(data[0], 0, 5) == 1 ? 1 : 0);
			edid_parser_update_ycc420(edid_ext, tmp_ycc420_all, tmp_limited_ycc420all);
			for (i = 0; i < length - 1; i++) {
				/** Lenght includes the tag byte*/
				tmp_svd.m_code = data[2 + i];
				tmp_svd.mnative = 0;
				tmp_svd.m_limited_to_ycc420 = 1;

				for (edid_cnt = 0;edid_cnt < edid_ext->edid_msvd_index;edid_cnt++) {
					if (edid_ext->edid_msvd[edid_cnt].m_code == tmp_svd.m_code) {
						edid_ext->edid_msvd[edid_cnt].m_limited_to_ycc420 =	1;
						goto concluded;
					}
				}
				if (edid_ext->edid_msvd_index < (sizeof(edid_ext->edid_msvd) /  sizeof(short_video_desc_t)))
				{
					edid_ext->edid_msvd[edid_ext->edid_msvd_index] = tmp_svd;
					edid_ext->edid_msvd_index++;
				} else {
					pr_err("buffer full - YCC 420 DTD ignored");
				}
				concluded: ;
			}
			break;
		case 0x0f:
			/** If it is a YCC420 CDB then VIC can ALSO be displayed in YCC 4:2:0 */
			edid_ext->edid_m20sink = TRUE;
			pr_debug("YCBCR 4:2:0 Capability Map Data Block");
			svd_nr = 0;
			/* If YCC420 CMDB is bigger than 1, then there is SVD info to parse */
			if(length > 1){
				for (i = 0; i < length - 1; i++) {
					for (icnt = 0; icnt <= 7; icnt++) {
						/** Lenght includes the tag byte*/
						if (bit_field(data[2 + i], icnt, 1)) {
							svd_nr = icnt + i*8;
							edid_ext->edid_msvd[svd_nr].m_ycc420 = 1;
						}
					}
				}
				/* Otherwise, all SVDs present at the Video Data Block support YCC420*/
			}else
			{
				tmp_ycc420_all = (bit_field(data[0], 0, 5) == 1 ? 1 : 0);
				edid_parser_update_ycc420(edid_ext, tmp_ycc420_all, tmp_limited_ycc420all);
			}
#if 0
			pr_debug("data[0] = 0x%x", data[0]);
			pr_debug("data[1] = 0x%x", data[1]);
			pr_debug("data[2] = 0x%x", data[2]);
			pr_debug("data[3] = 0x%x", data[3]);
			pr_debug("data[4] = 0x%x", data[4]);
			pr_debug("data[5] = 0x%x", data[5]);
			pr_debug("data[6] = 0x%x", data[6]);
			pr_debug("data[7] = 0x%x", data[7]);
#endif
			break;
		default:
			pr_debug("Extended Data Block not parsed %d\n",
					extended_tag);
			break;
		}
		break;
	}
	default:
		pr_err("Data Block not parsed %d\n", tag);
		break;
	}

	return length + 1;
}

int _edid_struture_parser(hdmi_tx_dev_t *dev, struct edid * edid, sink_edid_t * sink)
{
	int i;
	char * monitor_name;
	dtd_t* tmp_dtd = NULL;
	u16 hactive, vactive;

	if(edid->header[0] != 0){
		pr_err("Invalid Header\n");
		return HDMI_ERR_READ_EDID_FAILED;
	}

	for (i=0; i < 4; i++) {
		struct detailed_timing * detailed_timing = &(edid->detailed_timings[i]);
		if(detailed_timing->pixel_clock == 0){
			struct detailed_non_pixel * npixel = &(detailed_timing->data.other_data);

			switch (npixel->type){
			case EDID_DETAIL_MONITOR_NAME:
				monitor_name = (char *) &(npixel->data.str.str);
				pr_debug("Monitor name: %s\n", monitor_name);
				break;
			case EDID_DETAIL_MONITOR_RANGE:
				break;

			}
		}
		else { //Detailed Timing Definition
			struct detailed_pixel_timing * ptiming = &(detailed_timing->data.pixel_data);
			hactive = concat_bits(ptiming->hactive_hblank_hi, 4, 4, ptiming->hactive_lo, 0, 8);
			vactive = concat_bits(ptiming->vactive_vblank_hi, 4, 4, ptiming->vactive_lo, 0, 8);
			tmp_dtd = find_dtd(hactive, vactive, detailed_timing->pixel_clock * 10);

			if(tmp_dtd){
				sink->edid_msvd[sink->edid_msvd_index].m_code = tmp_dtd->m_code;
				sink->edid_msvd_index++;
			}
		}
	}
	return TRUE;
}

int _edid_cea_extension_parser(hdmi_tx_dev_t *dev, u8 * buffer, sink_edid_t * edid_ext)
{
	int i = 0;
	int c = 0;
	dtd_t tmp_dtd;
	u8 offset = buffer[2];

	if (buffer[1] < 0x3){
		pr_err("Invalid version for CEA Extension block\n");
		return HDMI_ERR_READ_EDID_FAILED;
	}

	edid_ext->edid_mycc422_support = bit_field(buffer[3],	4, 1) == 1;
	edid_ext->edid_mycc444_support = bit_field(buffer[3],	5, 1) == 1;
	edid_ext->edid_mbasic_audio_support = bit_field(buffer[3], 6, 1) == 1;
	edid_ext->edid_munder_scan_support = bit_field(buffer[3], 7, 1) == 1;
	if (offset != 4) {
		for (i = 4; i < offset; i += edid_parser_parse_data_block(dev, buffer + i, edid_ext)) ;
	}

	/* last is checksum */
	for (i = offset, c = 0; i < (sizeof(buffer) - 1) && c < 6; i += dtd_size, c++) {
		if (dtd_parse(dev, &tmp_dtd, buffer + i) == TRUE) {
			if (edid_ext->edid_mdtd_index < (sizeof(edid_ext->edid_mdtd) / sizeof(dtd_t)) - 1) {
				edid_ext->edid_mdtd[edid_ext->edid_mdtd_index++] = tmp_dtd;
				pr_debug("edid_mdtd code %d\n", edid_ext->edid_mdtd[edid_ext->edid_mdtd_index].m_code);
				pr_debug("edid_mdtd limited to Ycc420? %d\n",
							edid_ext->edid_mdtd[edid_ext->edid_mdtd_index].m_limited_to_ycc420);
				pr_debug("edid_mdtd supports Ycc420? %d\n", edid_ext->edid_mdtd[edid_ext->edid_mdtd_index].m_ycc420);
			} else {
				pr_err("buffer full - DTD ignored\n");
			}
		}
	}

	return TRUE;
}

int edid_parser(hdmi_tx_dev_t *dev, u8 * buffer, sink_edid_t *edid_ext, u16 edid_size)
{
	int ret = 0;

	switch (buffer[0]){
		case 0x00:
			ret = _edid_struture_parser(dev, (struct edid *) buffer, edid_ext);
			break;
		case CEA_EXT:
			ret = _edid_cea_extension_parser(dev, buffer, edid_ext);
			break;
		case VTB_EXT:
		case DI_EXT:
		case LS_EXT:
		case MI_EXT:
		default:
			pr_err("Block 0x%02x not supported\n", buffer[0]);
	}

	if(ret == TRUE)
		return TRUE;

	return FALSE;
}

int edid_read_cap()
{
	dtd_t tmp_dtd;
	int edid_ok = 0;
	int edid_tries = 3;
	int i, j = 0;

	if(!ctx->mode.sink_cap) {
		ctx->mode.sink_cap = (sink_edid_t *) vmalloc(sizeof(sink_edid_t));
	}
	memset(ctx->mode.sink_cap, 0, sizeof(sink_edid_t));
	memset(&ctx->mode.edid, 0, sizeof(struct edid));
	memset(ctx->mode.sink_capinfo, 0, sizeof(ctx->mode.sink_capinfo));
	memset(ctx->mode.edid_ext, 0, sizeof(ctx->mode.edid_ext));

	edid_parser_cea_ext_reset(&ctx->hdmi_tx, ctx->mode.sink_cap);

	do{
		if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
			pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
			return HDMI_ERR_HPD_FAILED;
			break;
		}

		edid_ok = edid_read(&ctx->hdmi_tx, &ctx->mode.edid);

		if(edid_ok) //error case
			continue;

		if(edid_parser(&ctx->hdmi_tx, (u8 *) &ctx->mode.edid, ctx->mode.sink_cap, 128) == FALSE){
			pr_debug("Could not parse EDID");
			ctx->mode.edid_done = 0;
			return HDMI_ERR_READ_EDID_FAILED;
		}
		break;
	}while(edid_tries--);

	if(edid_tries <= 0){
		pr_debug("Could not read EDID");
		ctx->mode.edid_done = 0;
		return HDMI_ERR_READ_EDID_FAILED;
	}

	if(ctx->mode.edid.extensions == 0){
		ctx->mode.edid_done = 1;
	}
	else {
		int edid_ext_cnt = 1;
		while(edid_ext_cnt <= ctx->mode.edid.extensions){
			pr_debug("EDID Extension %d", edid_ext_cnt);
			edid_tries = 3;
			do{
				if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
					pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
					return HDMI_ERR_HPD_FAILED;
					break;
				}

				edid_ok = edid_extension_read(&ctx->hdmi_tx, edid_ext_cnt, &ctx->mode.edid_ext[edid_ext_cnt-1][0]);
				if(edid_ok) //error case
					continue;

				ctx->mode.edid_done = 1;
				if(edid_ext_cnt < 2){ // TODO: add support for EDID parsing w/ Ext blocks > 1
					if(edid_parser(&ctx->hdmi_tx,
							&ctx->mode.edid_ext[edid_ext_cnt-1][0], ctx->mode.sink_cap, 128) == FALSE){
						pr_err("Could not parse EDID EXTENSIONS");
						ctx->mode.edid_done = 0;
						return HDMI_ERR_READ_EDID_FAILED;
					}
				}
				break;
			}while(edid_tries--);
			edid_ext_cnt++;
		}
	}

	if(ctx->mode.sink_cap){
		for(i = 0; (i < 128) && (ctx->mode.sink_cap->edid_msvd[i].m_code != 0); i++){
			ctx->mode.sink_capinfo[j].sink_cap_info.m_code = ctx->mode.sink_cap->edid_msvd[i].m_code;
			dtd_fill(&ctx->hdmi_tx ,&tmp_dtd, ctx->mode.sink_cap->edid_msvd[i].m_code, 0);
			ctx->mode.sink_capinfo[j].fresh_rate = dtd_get_refresh_rate(&tmp_dtd);
			memcpy(&ctx->mode.sink_capinfo[j++].sink_cap_info, &tmp_dtd, sizeof(dtd_t));
		}

		ctx->mode.sink_cap->xv_ycc709
		 = supports_xv_ycc709(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_mcolorimetry_datablock);
		ctx->mode.sink_cap->s_ycc601
		 = supports_s_ycc601(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_mcolorimetry_datablock);
		ctx->mode.sink_cap->xv_ycc601
		 = supports_xv_ycc601(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_mcolorimetry_datablock);
		ctx->mode.sink_cap->adobe_ycc601
		 = supports_adobe_ycc601(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_mcolorimetry_datablock);
		ctx->mode.sink_cap->adobe_rgb
		 = supports_adobe_rgb(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_mcolorimetry_datablock);

		i = 0;
		if(sad_support192k(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
			ctx->mode.sink_cap->support_sample_rate[i++] = 192000;
		if(sad_support176k4(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
			ctx->mode.sink_cap->support_sample_rate[i++] = 176400;
		if(sad_support96k(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
			ctx->mode.sink_cap->support_sample_rate[i++] = 96000;
		if(sad_support88k2(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
			ctx->mode.sink_cap->support_sample_rate[i++] = 88000;
		if(sad_support48k(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
			ctx->mode.sink_cap->support_sample_rate[i++] = 48000;
		if(sad_support44k1(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
			ctx->mode.sink_cap->support_sample_rate[i++] = 44100;
		if(sad_support32k(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
			ctx->mode.sink_cap->support_sample_rate[i++] = 32000;

		if (ctx->mode.sink_cap->edid_msad[0].mformat == 1) {
			i = 0;
			if(sad_support24bit(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
				ctx->mode.sink_cap->support_bit_depth[i++] = 24;
			if(sad_support20bit(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
				ctx->mode.sink_cap->support_bit_depth[i++] = 20;
			if(sad_support16bit(&ctx->hdmi_tx, &ctx->mode.sink_cap->edid_msad[0]))
				ctx->mode.sink_cap->support_bit_depth[i++] = 16;
		}
	}

	return 0;
}

static size_t read_file_into_buffer(const char *filename, int size, char *data)
{
	struct file *filp;
	size_t read_size;

	filp = filp_open(filename, O_RDONLY, 0);
	if (IS_ERR(filp)) {
		pr_err("%s: filp_open error %ld\n", __func__, PTR_ERR(filp));
		return -1;
	}

	read_size = kernel_read(filp, data, size, &filp->f_pos);

	filp_close(filp, NULL);

	return read_size;

}

int hdmitx_force_get_edid(hdmi_edid* edid_raw, char *fileName)
{
	int j;
	u8 *edid_data;
	int ret;

	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	if (fileName) {
		edid_data = kmalloc(256, GFP_KERNEL);
		ret = read_file_into_buffer(fileName, 256, edid_data);
		if (ret < 0) {
			kfree(edid_data);
			return -1;
		} else {
			for(j = 0; j < 256; j++)
				edid_raw->edid[j] = edid_data[j];
		}

		kfree(edid_data);
	} else {
		if (edid_read_cap()) {
			pr_debug("Read Edid error !\n");
			hdmitx_event_id = HDMI_EDID_FAIL;
			kill_fasync(&hdmi_fasync, SIGIO, POLL_IN);
			return stop_handler();
		}

		edid_data = (u8 *)&ctx->mode.edid;
		for(j=0; j<128; j++)
		{
			edid_raw->edid[j] = edid_data[j];
			edid_raw->edid[j+128] = ctx->mode.edid_ext[0][j];
		}
	}

	return 0;
}

int hdmitx_set_attr(hdmi_attr* attr)
{
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}
	if (!attr->video_format) {
		pr_err("Invalid Video Mode\n");
		return HDMI_ERR_SET_ATTR_FAILED;
	}
	if (attr->hdmi_video_input != HDMI_VIDEO_MODE_RGB888 &&
		attr->hdmi_video_input != HDMI_VIDEO_MODE_YCBCR444)
	{
		pr_err("Invalid Video Input\n");
		return HDMI_ERR_SET_ATTR_FAILED;
	}
	if (attr->hdmi_video_output != HDMI_VIDEO_MODE_RGB888 &&
		attr->hdmi_video_output != HDMI_VIDEO_MODE_YCBCR444 &&
		attr->hdmi_video_output != HDMI_VIDEO_MODE_YCBCR422)
	{
		pr_err("Invalid Video Output\n");
		return HDMI_ERR_SET_ATTR_FAILED;
	}
	if(attr->audio_en){
		if (attr->sample_rate < HDMI_SAMPLE_RATE_32K ||
			attr->sample_rate > HDMI_SAMPLE_RATE_BUTT)
		{
			pr_err("Invalid Sample Rate\n");
			return HDMI_ERR_SET_ATTR_FAILED;
		}
	}
	if (attr->bit_depth < HDMI_BIT_DEPTH_16 ||
		attr->bit_depth > HDMI_BIT_DEPTH_24)
	{
		pr_err("Invalid Bit Depth\n");
		return HDMI_ERR_SET_ATTR_FAILED;
	}
	if ((attr->pix_clk > 594000) || (attr->pix_clk <= 0)) {
		pr_err("Invalid Pix_Clk Value\n");
		return HDMI_ERR_SET_ATTR_FAILED;
	}
	/*video*/
	ctx->mode.hdmi_en = ctx->hdmi_tx.snps_hdmi_ctrl.hdmi_on
					  = attr->hdmi_en;
	ctx->mode.pvideo.mdtd.m_code = attr->video_format;
	ctx->mode.pvideo.mcolor_resolution = attr->deep_color_mode;
	ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock = attr->pix_clk;
	ctx->mode.pvideo.mencodingin = (encoding_t)attr->hdmi_video_input;
	ctx->mode.pvideo.mencodingout = (encoding_t)attr->hdmi_video_output;
	ctx->hdmi_tx.snps_hdmi_ctrl.csc_on =
				(ctx->mode.pvideo.mencodingin == ctx->mode.pvideo.mencodingout) ? 0: 1;
	ctx->hdmi_tx.snps_hdmi_ctrl.hdcp_on = attr->hdcp14_en;
	ctx->hdmi_tx.snps_hdmi_ctrl.hdmi_force_output = attr->hdmi_force_output;
	/*audio*/
	ctx->mode.paudio.msample_size = attr->bit_depth;
	ctx->mode.paudio.audio_en = ctx->hdmi_tx.snps_hdmi_ctrl.audio_on
							  = attr->audio_en;
	ctx->mode.paudio.msampling_frequency = attr->sample_rate;
	ctx->mode.paudio.start_addr = attr->audio_start_paddr;
	ctx->mode.paudio.stop_addr = attr->audio_stop_paddr;

	return 0;
}

int hdmitx_get_attr(hdmi_attr* attr)
{
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}
	/*video*/
	attr->hdmi_en = (0x1) & ~dev_read_mask(MC_CLKDIS, MC_CLKDIS_PIXELCLK_DISABLE_MASK);
	attr->hdcp14_en = dev_read_mask(A_HDCPCFG0, A_HDCPCFG0_RXDETECT_MASK);
	attr->video_format = ctx->mode.pvideo.mdtd.m_code;
	attr->deep_color_mode = HDMI_DEEP_COLOR_24BIT;
	attr->pix_clk = ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock;
	attr->hdmi_video_input = (hdmi_video_mode)ctx->mode.pvideo.mencodingin;
	attr->hdmi_video_output = (hdmi_video_mode)ctx->mode.pvideo.mencodingout;
	attr->hdmi_force_output = ctx->hdmi_tx.snps_hdmi_ctrl.hdmi_force_output;
	/*audio*/
	attr->audio_en = dev_read_mask(AHB_DMA_START, 0x1);
	attr->sample_rate = ctx->mode.paudio.msampling_frequency;
	attr->bit_depth = ctx->mode.paudio.msample_size;
	attr->audio_start_paddr = ctx->mode.paudio.start_addr;
	attr->audio_stop_paddr = ctx->mode.paudio.stop_addr;

	return 0;
}

int hdmitx_set_avmute(int enable)
{
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	pr_debug("set avmute:%d\n", enable);
	api_avmute(&ctx->hdmi_tx, enable);
	return 0;
}

int hdmitx_set_audio_mute(int enable)
{
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	pr_debug("set audio mute:%d\n", enable);
	audio_mute(&ctx->hdmi_tx, enable);
	return 0;
}

int hdmitx_init(void)
{
	// mutex_init(&(ctx->mutex));
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	if(!ctx->mode.sink_cap){
		if (edid_read_cap()) {
			pr_debug("Read Edid error !");
			hdmitx_event_id = HDMI_EDID_FAIL;
			kill_fasync(&hdmi_fasync, SIGIO, POLL_IN);
		}
	}
	// Reset video, audio and packet structures
	audio_reset(&ctx->hdmi_tx, &ctx->mode.paudio);
	video_params_reset(&ctx->hdmi_tx, &ctx->mode.pvideo);
	reset_hdcp_params();

	ctx->hdmi_tx.snps_hdmi_ctrl.pixel_repetition = 0;
	phy_initialize(&ctx->hdmi_tx);

	ctx->hdmi_tx.is_init = TRUE;
	ctx->hdmi_tx.is_deinit = FALSE;

	return 0;
}

int hdmitx_start(void)
{
	// mutex_lock(&(ctx->mutex));
	int i = 0;
	int res = 0;

	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	control_interrupt_clear_all(&ctx->hdmi_tx);
	irq_mute(&ctx->hdmi_tx);

	if(!ctx->hdmi_tx.snps_hdmi_ctrl.hdmi_force_output) {
		if(ctx->mode.sink_cap){
			while(ctx->mode.sink_cap->edid_msvd[i].m_code != 0)
			{
				if(ctx->mode.pvideo.mdtd.m_code == ctx->mode.sink_cap->edid_msvd[i++].m_code){
					break;
				} else if (ctx->mode.sink_cap->edid_msvd[i].m_code == 0) {
					if (is_resume) {
						is_resume = false;
						return 0;
					}
					pr_err("m_code %d is not supported by this Sink\n", ctx->mode.pvideo.mdtd.m_code);
					ctx->mode.pvideo.mdtd.m_code = 0;
					ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock = 0;
					irq_hpd_sense_enable(&ctx->hdmi_tx);
					return HDMI_ERR_FEATURE_NO_SUPPORT;
				}
			}
		} else {
			pr_err("Sink_cap is NULL. hdmitx is not properly connected with sink, or hdmitx has been deinit !\n");
			ctx->mode.pvideo.mdtd.m_code = 0;
			ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock = 0;
			irq_hpd_sense_enable(&ctx->hdmi_tx);
			return HDMI_ERR_DEV_NOT_CONNECT;
		}

		dtd_fill(&ctx->hdmi_tx, &ctx->mode.pvideo.mdtd,
			 ctx->mode.pvideo.mdtd.m_code, 0);

		if(ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock != ctx->mode.pvideo.mdtd.m_pixel_clock) {
			pr_err("Pixel Clock %d is not match with m_code %d\n",
					ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock,
					ctx->mode.pvideo.mdtd.m_code);
			ctx->mode.pvideo.mdtd.m_code = 0;
			ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock = 0;
			irq_hpd_sense_enable(&ctx->hdmi_tx);
			return HDMI_ERR_FEATURE_NO_SUPPORT;
		}
		if(ctx->mode.pvideo.mdtd.m_interlaced) {
			pr_err("Interlaced mode is not supported by hdmitx\n");
			return HDMI_ERR_FEATURE_NO_SUPPORT;
		}
	} else {
		dtd_fill(&ctx->hdmi_tx, &ctx->mode.pvideo.mdtd,
				ctx->mode.pvideo.mdtd.m_code, 0);
	}

	print_videoinfo(&(ctx->mode.pvideo));

	if(ctx->mode.sink_cap){
		if (ctx->mode.sink_cap->edid_mhdmivsdb.m_supports_ai == 0){
			packets_stop_send_isrc1(&ctx->hdmi_tx);
			packets_stop_send_isrc2(&ctx->hdmi_tx);
			packets_stop_send_acp(&ctx->hdmi_tx);
		} else {
			// packets_audio_content_protection(&ctx->hdmi_tx,0,NULL,0,1);//Type 0 doesn't need any dependent fields 9.3.3 HDMI 1.4 spec
			// packets_isrc_packets(&ctx->hdmi_tx,0x01,NULL,0,1);//Configuring without packet info in ISRC
		}
	}

	if(ctx->mode.hdmi_en){
		res = api_configure(&ctx->hdmi_tx, &ctx->mode.pvideo, &ctx->mode.paudio, &ctx->mode.phdcp);
		if (res != 0) {
			pr_err("Api configure failed\n");
			stop_handler();
			return res;
		}
	}

	irq_hpd_sense_enable(&ctx->hdmi_tx);

	if (is_resume) {
		is_resume = false;
	}

	// mutex_unlock(&(ctx->mutex));

	return res;
}

int hdmitx_stop(void)
{
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	return stop_handler();
}

irqreturn_t _hdmi_tx_handler(int irq, void *dev_id){
	struct hdmitx_dev *dev = NULL;
	u32 hdcp_irq = 0;

	if(dev_id == NULL)
		return IRQ_NONE;

	dev = dev_id;

	hdcp_irq = dev_read(A_APIINTSTAT);
	dev->decode = dev_read(IH_DECODE);

	if(dev->decode){
		dev_write(IH_MUTE, 0x1);
		return IRQ_WAKE_THREAD;
	}

	if(hdcp_irq != 0){
		dev_write(A_APIINTMSK, 0xff);
	}
	else{
		return IRQ_HANDLED;
	}

	return IRQ_HANDLED;
}

irqreturn_t _hdcp_handler(int irq, void *dev_id){
	struct hdmitx_dev *dev = NULL;

	if(dev_id == NULL)
		return IRQ_NONE;

	dev = dev_id;

	return IRQ_HANDLED;
}

irqreturn_t dwc_hdmi_tx_handler(int irq, void *dev_id){
	u32 decode = 0;
	u32 hdcp_irq = 0;
	u8 intr_stat;
	u8 phy_decode = 0;
	int execute = 0;
	struct hdmitx_dev *dev = NULL;

	dev = dev_id;
	decode = dev->decode;
	hdcp_irq = hdcp_interrupt_status(&ctx->hdmi_tx);
	if(hdcp_irq != 0){
		hdcp_handler();
	}

	if(decode_is_fc_stat0(decode)){
		irq_clear_source(&ctx->hdmi_tx, AUDIO_PACKETS);
	}

	if(decode_is_fc_stat1(decode)){
		irq_clear_source(&ctx->hdmi_tx, OTHER_PACKETS);
	}

	if(decode_is_fc_stat2_vp(decode)){
		// TODO: mask this for now...
		irq_mute_source(&ctx->hdmi_tx, PACKETS_OVERFLOW);
		irq_mute_source(&ctx->hdmi_tx, VIDEO_PACKETIZER);
	}
	if(decode_is_as_stat0(decode)){
		irq_clear_source(&ctx->hdmi_tx, AUDIO_SAMPLER);
	}
	if(decode_is_phy(decode)){

		irq_hpd_sense_enable(&ctx->hdmi_tx);
		pr_debug("%s:PHY interrupt 0x%08x", __func__, decode);

		irq_read_stat(&ctx->hdmi_tx, PHY, &phy_decode);

		if(decode_is_phy_lock(phy_decode)) {
			irq_clear_bit(&ctx->hdmi_tx, PHY, IH_PHY_STAT0_TX_PHY_LOCK_MASK);
		}

		if(decode_is_phy_rx_s0(phy_decode)) {
			pr_debug("Rx sense 0 received");
			phy_rx_s0_detected(&ctx->hdmi_tx);
			irq_clear_bit(&ctx->hdmi_tx, PHY, IH_PHY_STAT0_RX_SENSE_0_MASK);
			execute = 1;
		}

		if(decode_is_phy_rx_s1(phy_decode)) {
			pr_debug("Rx sense 1 received");
			phy_rx_s1_detected(&ctx->hdmi_tx);
			irq_clear_bit(&ctx->hdmi_tx, PHY, IH_PHY_STAT0_RX_SENSE_1_MASK);
			execute = 1;
		}

		if(decode_is_phy_rx_s2(phy_decode)) {
			pr_debug("Rx sense 2 received");
			phy_rx_s2_detected(&ctx->hdmi_tx);
			irq_clear_bit(&ctx->hdmi_tx, PHY, IH_PHY_STAT0_RX_SENSE_2_MASK);
			execute = 1;
		}

		if(decode_is_phy_rx_s3(phy_decode)) {
			pr_debug("Rx sense 3 received");
			phy_rx_s3_detected(&ctx->hdmi_tx);
			irq_clear_bit(&ctx->hdmi_tx, PHY, IH_PHY_STAT0_RX_SENSE_3_MASK);
			execute = 1;
		}

		if(decode_is_phy_hpd(phy_decode)) {
			pr_debug("HPD received");
			phy_hot_plug_detected(&ctx->hdmi_tx);
			irq_clear_bit(&ctx->hdmi_tx, PHY, IH_PHY_STAT0_HPD_MASK);
			execute = 1;
		}

		if(execute){
			hpd_handler();
		}
	}

	if(decode_is_i2c_stat0(decode)){
		u8 state = 0;
		irq_read_stat(&ctx->hdmi_tx, I2C_DDC, &state);

		// I2Cmastererror - I2Cmasterdone
		if(state & 0x3){
			irq_clear_bit(&ctx->hdmi_tx, I2C_DDC, IH_I2CM_STAT0_I2CMASTERDONE_MASK);
			//The I2C communication interrupts must be masked - they will be handled by polling in the eDDC block
			pr_debug("%s:I2C DDC interrupt received 0x%x - mask interrupt", __func__, state);
		}
		// SCDC_READREQ
		else if(state & 0x4){
			irq_clear_bit(&ctx->hdmi_tx, I2C_DDC, IH_I2CM_STAT0_SCDC_READREQ_MASK);
		}
	}

	intr_stat = dev_read(IH_PHY_STAT0);
	dev_write(IH_PHY_STAT0, intr_stat);

	irq_hpd_sense_enable(&ctx->hdmi_tx);
	// mutex_unlock(&(ctx->mutex));

	return IRQ_HANDLED;
}

void hdcp_handler(void){
	int temp = 0;
	hdcp_event_handler(&(ctx->hdmi_tx), &temp);
}

void reset_hdcp_params(void)
{
	hdcp_params_reset(&ctx->hdmi_tx, &ctx->mode.phdcp);
	ctx->hdmi_tx.hdcp.max_devices = ctx->mode.ksv_devices;
	ctx->mode.phdcp.mksvList_buffer = NULL; //ctx->mode.ksv_list_buffer;
	ctx->mode.phdcp.maksv = ctx->mode.dpk_aksv;
	ctx->mode.phdcp.msw_enckey = ctx->mode.sw_enc_key;
	ctx->mode.phdcp.mkeys = ctx->mode.dpk_keys;
	ctx->mode.phdcp.bypass = TRUE;
}

void print_videoinfo(video_params_t *pvideo)
{
	u32 refresh_rate = dtd_get_refresh_rate(&pvideo->mdtd);
	pr_debug("CEA VIC=%d: ", pvideo->mdtd.m_code);
	if(pvideo->mdtd.m_interlaced)
		pr_debug("%dx%di",pvideo->mdtd.m_hactive, pvideo->mdtd.m_vactive * 2);
	else
		pr_debug("%dx%dp", pvideo->mdtd.m_hactive, pvideo->mdtd.m_vactive);
	pr_debug("@ ");
	pr_debug("%u Hz ", refresh_rate);
	pr_debug("%d:%d, ", pvideo->mdtd.m_himage_size, pvideo->mdtd.m_vimage_size);
	pr_debug("%d-bpp ", pvideo->mcolor_resolution);
	pr_debug("%s, ", get_encoding_string(pvideo->mencodingin));
	pr_debug("%s\n", pvideo->mhdmi == HDMI ? "HDMI" : "DVI");
}

int do_reset(void){
	pr_debug("Reset done.");

	irq_mask_all(&ctx->hdmi_tx);
	irq_hpd_sense_enable(&ctx->hdmi_tx);

	return 0;
}

int stop_handler(void){
	pr_debug("stop handler");

	hdcp_rxdetect(&ctx->hdmi_tx, 0);
	hdcp_sw_reset(&ctx->hdmi_tx);
	api_standby(&ctx->hdmi_tx);
	do_reset();

	return 0;
}

int start_handler(void){
	sink_edid_t * sink = NULL;

	// Read sink's EDID
	if (edid_read_cap()) {
		pr_debug("Read Edid error !");
		hdmitx_event_id = HDMI_EDID_FAIL;
		kill_fasync(&hdmi_fasync, SIGIO, POLL_IN);
		stop_handler();
		return -1;
	}

	sink = ctx->mode.sink_cap;
	if(sink && sink->edid_mhdmi_forumvsdb.m_rr_capable){
		pr_debug("Enabling Read Request");
		scdc_enable_rr(&ctx->hdmi_tx, TRUE);
	}

	if(sink){
		pr_debug("Sink is HDMI 2.0: %d\n", sink->edid_m20sink);
		pr_debug("HDMI-Forum VSDB valid: %d\n", sink->edid_mhdmi_forumvsdb.m_valid);
		pr_debug("Sink supports SCDC: %d\n", sink->edid_mhdmi_forumvsdb.m_scdc_Present);
		pr_debug("Sink supports RR: %d\n", sink->edid_mhdmi_forumvsdb.m_rr_capable);
		pr_debug("Sink supports LTE_340Mcs Scrambling: %d\n", sink->edid_mhdmi_forumvsdb.m_lts_340mcs_scramble);
		pr_debug("Number of SVDs parsed %d\n", sink->edid_msvd_index);
		pr_debug("First SVDs parsed VIC %d\n", sink->edid_msvd[0].m_code);
		pr_debug("First SVDs Limited Ycc420 %d\n", sink->edid_msvd[0].m_limited_to_ycc420);
		pr_debug("First SVDs supports Ycc420 %d\n", sink->edid_msvd[0].m_ycc420);
		pr_debug("Second SVDs parsed VIC %d\n", sink->edid_msvd[1].m_code);
		pr_debug("Third SVDs parsed VIC %d\n", sink->edid_msvd[2].m_code);
	}

	if(sink)
		ctx->mode.pvideo.mhdmi = (sink->edid_mhdmivsdb.mvalid == TRUE ? HDMI: DVI);
	else
		ctx->mode.pvideo.mhdmi = DVI;

	pr_debug("Changing video mode to:\n");
	print_videoinfo(&(ctx->mode.pvideo));
	if(sink != NULL) {
		if (sink->edid_mhdmivsdb.m_supports_ai == 0){
			packets_stop_send_isrc1(&ctx->hdmi_tx);
			packets_stop_send_isrc2(&ctx->hdmi_tx);
			packets_stop_send_acp(&ctx->hdmi_tx);
		} else {
			// Type 0 doesn't need any dependent fields 9.3.3 HDMI 1.4 spec
			// packets_audio_content_protection(&ctx->hdmi_tx,0,NULL,0,1);
			// packets_isrc_packets(&ctx->hdmi_tx,0x01,NULL,0,1);//Configuring without packet info in ISRC
		}
	}

	if(!ctx->mode.pvideo.mdtd.m_code || !ctx->hdmi_tx.snps_hdmi_ctrl.pixel_clock){
		return -1;
	}

	if (api_configure(&ctx->hdmi_tx, &ctx->mode.pvideo, &ctx->mode.paudio, &ctx->mode.phdcp)) {
		pr_debug("API configure error");
		stop_handler();
		return -1;
	}

	return 0;
}

void hpd_handler()
{
	int ret = 0;

	pr_debug("%s:%d Hot Plug => %s, Rx Sense => %s", __FUNCTION__, __LINE__,
					phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF",
					phy_rx_sense_state(&ctx->hdmi_tx) ? "ON" : "OFF");
	mdelay(100);

	if (phy_hot_plug_state(&ctx->hdmi_tx) && (!hdmitx_state)) {
		ret = start_handler();
		hdmitx_state = TRUE;
		if (!ret || hdmitx_state)
		{
			hdmitx_event_id = HDMI_HOTPLUG;
			kill_fasync(&hdmi_fasync, SIGIO, POLL_IN);
		}
	} else if ((!phy_hot_plug_state(&ctx->hdmi_tx) || !phy_rx_sense_state(&ctx->hdmi_tx)) && hdmitx_state) {
		ret = stop_handler();
		hdmitx_state = FALSE;
		if (!ret)
		{
			hdmitx_event_id = HDMI_NOPLUG;
			kill_fasync(&hdmi_fasync, SIGIO, POLL_IN);
	 	}
	}
}

int hdmitx_deinit(void)
{
	// mutex_init(&(ctx->mutex));
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	if(ctx->hdmi_tx.is_deinit || !ctx->hdmi_tx.is_init)
		return 0;

	if(ctx->mode.sink_cap != NULL){
		vfree(ctx->mode.sink_cap);
		ctx->mode.sink_cap = NULL;
	}

	irq_mask_all(&ctx->hdmi_tx);
	irq_hpd_sense_enable(&ctx->hdmi_tx);

	memset(&ctx->mode, 0, sizeof(struct hdmi_mode));
	memset(&ctx->hdmi_tx.snps_hdmi_ctrl, 0, sizeof(struct hdmi_tx_ctrl));

	ctx->hdmi_tx.is_init = FALSE;
	ctx->hdmi_tx.is_deinit = TRUE;

	return 0;
}
EXPORT_SYMBOL_GPL(hdmitx_deinit);

int hdmitx_set_infoframe(hdmi_infoframe* info_frame)
{
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}
	if(!info_frame->infoframe_unit.avi_infoframe.timing_mode) {
		pr_err("Invaild Video Mode\n");
		return HDMI_ERR_SET_INFOFRAME_FAILED;
	}
	if (info_frame->infoframe_unit.avi_infoframe.color_space != HDMI_COLOR_SPACE_YCBCR422 &&
		info_frame->infoframe_unit.avi_infoframe.color_space != HDMI_COLOR_SPACE_YCBCR444 &&
		info_frame->infoframe_unit.avi_infoframe.color_space != HDMI_COLOR_SPACE_RGB888)
	{
		pr_err("Invalid Color Space\n");
		return HDMI_ERR_SET_INFOFRAME_FAILED;
	}
	if (info_frame->infoframe_unit.avi_infoframe.colorimetry != HDMI_COMMON_COLORIMETRY_ITU601 &&
		info_frame->infoframe_unit.avi_infoframe.colorimetry != HDMI_COMMON_COLORIMETRY_ITU709 &&
		info_frame->infoframe_unit.avi_infoframe.colorimetry != HDMI_COMMON_COLORIMETRY_NO_DATA)
	{
		pr_err("Invalid Colorimetry\n");
		return HDMI_ERR_SET_INFOFRAME_FAILED;
	}
	if (info_frame->infoframe_unit.avi_infoframe.rgb_quant != HDMI_RGB_QUANT_LIMITED_RANGE &&
		info_frame->infoframe_unit.avi_infoframe.rgb_quant != HDMI_RGB_QUANT_FULL_RANGE &&
		info_frame->infoframe_unit.avi_infoframe.rgb_quant != HDMI_RGB_QUANT_DEFAULT_RANGE)
	{
		pr_err("Invalid Rgb Quant\n");
		return HDMI_ERR_SET_INFOFRAME_FAILED;
	}
	if (info_frame->infoframe_unit.audio_infoframe.chn_cnt < HDMI_AUDIO_CHANEL_CNT_STREAM ||
		info_frame->infoframe_unit.audio_infoframe.chn_cnt > HDMI_AUDIO_CHANEL_CNT_8) {
		pr_err("Invalid Chanel Count\n");
		return HDMI_ERR_SET_INFOFRAME_FAILED;
	}
	if (info_frame->infoframe_unit.audio_infoframe.coding_type != HDMI_AUDIO_CODING_PCM) {
		pr_err("Invalid Coding Type\n");
		return HDMI_ERR_SET_INFOFRAME_FAILED;
	}
	if (info_frame->infoframe_unit.audio_infoframe.sample_size != HDMI_AUDIO_SAMPLE_SIZE_16 &&
		info_frame->infoframe_unit.audio_infoframe.sample_size != HDMI_AUDIO_SAMPLE_SIZE_24)
	{
		pr_err("Invalid Sampling size\n");
		return HDMI_ERR_SET_INFOFRAME_FAILED;
	}
	if (info_frame->infoframe_unit.audio_infoframe.sampling_freq < HDMI_AUDIO_SAMPLE_FREQ_32000 &&
		info_frame->infoframe_unit.audio_infoframe.sampling_freq > HDMI_AUDIO_SAMPLE_FREQ_192000)
	{
		pr_err("Invalid Sampling freq\n");
		return HDMI_ERR_SET_INFOFRAME_FAILED;
	}

	/*video*/
	ctx->hdmi_tx.snps_hdmi_ctrl.pixel_repetition = ctx->mode.pvideo.mpixel_repetition_factor
												 = info_frame->infoframe_unit.avi_infoframe.pixel_repetition;
	ctx->mode.pvideo.mencodingout =  (encoding_t)info_frame->infoframe_unit.avi_infoframe.color_space;
	ctx->mode.pvideo.mdtd.m_code = info_frame->infoframe_unit.avi_infoframe.timing_mode;
	ctx->mode.pvideo.mcolorimetry = info_frame->infoframe_unit.avi_infoframe.colorimetry;
	ctx->mode.pvideo.mrgb_quantization_range = info_frame->infoframe_unit.avi_infoframe.rgb_quant;
	/*audio*/
	ctx->mode.paudio.mchannel_allocation =  info_frame->infoframe_unit.audio_infoframe.chn_alloc;
	ctx->mode.paudio.mcoding_type = (coding_type_t)info_frame->infoframe_unit.audio_infoframe.coding_type;
	ctx->mode.paudio.msample_size = info_frame->infoframe_unit.audio_infoframe.sample_size;
	ctx->mode.paudio.msampling_frequency = info_frame->infoframe_unit.audio_infoframe.sampling_freq;
	return 0;
}

int hdmitx_get_infoframe(hdmi_infoframe* info_frame)
{
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	/*video*/
	info_frame->infoframe_unit.avi_infoframe.pixel_repetition = ctx->hdmi_tx.snps_hdmi_ctrl.pixel_repetition;
	info_frame->infoframe_unit.avi_infoframe.color_space = (hdmi_color_space)ctx->mode.pvideo.mencodingout;
	info_frame->infoframe_unit.avi_infoframe.timing_mode = ctx->mode.pvideo.mdtd.m_code;
	info_frame->infoframe_unit.avi_infoframe.colorimetry = ctx->mode.pvideo.mcolorimetry;
	info_frame->infoframe_unit.avi_infoframe.rgb_quant = ctx->mode.pvideo.mrgb_quantization_range;
	/*audio*/
	info_frame->infoframe_unit.audio_infoframe.chn_alloc = ctx->mode.paudio.mchannel_allocation;
	info_frame->infoframe_unit.audio_infoframe.coding_type = (hdmi_coding_type)ctx->mode.paudio.mcoding_type;
	info_frame->infoframe_unit.audio_infoframe.sample_size = ctx->mode.paudio.msample_size;
	info_frame->infoframe_unit.audio_infoframe.sampling_freq = ctx->mode.paudio.msampling_frequency;
	return 0;
}

int hdmi_proc_edid_cmd(u8 edid, char* file_path)
{
	hdmi_edid edid_data;
	memset(&edid_data, 0, sizeof(edid_data));

	if (edid == 0) {
		hdmitx_force_get_edid(&edid_data, NULL);
	} else if (edid == 1) {
		hdmitx_force_get_edid(&edid_data, file_path);
	} else {
		pr_err("%s: Invalid Param edid(%d) \n", __func__, edid);
		return -1;
	}

	return 0;
}

int hdmi_proc_cbar_cmd(u8 cbar)
{
	if (cbar == 1) {
		disp_set_pattern(DISP1, PAT_TYPE_FULL, PAT_COLOR_BAR, NULL);
	} else if (cbar == 0) {
		disp_set_pattern(DISP1, PAT_TYPE_OFF, PAT_COLOR_WHITE, NULL);
	} else {
		pr_err("%s: Invalid Param cbar(%d) \n", __func__, cbar);
		return -1;
	}

	return 0;
}

int hdmi_proc_mode_cmd(u8 hdmi_mode)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();

	ctx->mode.pvideo.mhdmi = hdmi_mode == 1 ? HDMI: DVI;

	if(hdmitx_start() < 0)
		return -1;

	return 0;
}

int hdmi_proc_ddc_cmd(u32 ddc_rate)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();

	if (ddc_rate > 100000) {
		pr_err("%s: Invalid Param ddc_rate(%d) \n", __func__, ddc_rate);
		return -1;
	}

	ctx->hdmi_tx.i2c.scl_high_ns = I2C_SCL_HIGH_TIME_NS * I2C_CLK / ddc_rate;
	ctx->hdmi_tx.i2c.scl_low_ns = I2C_SCL_LOW_TIME_NS * I2C_CLK / ddc_rate;

	pr_info("%s: ctx->hdmi_tx.i2c.scl_high_ns = %d, ctx->hdmi_tx.i2c.scl_low_ns = %d \n",
			__func__, ctx->hdmi_tx.i2c.scl_high_ns, ctx->hdmi_tx.i2c.scl_low_ns);

	i2cddc_clk_set_divs(&ctx->hdmi_tx);

	return 0;
}

int hdmi_proc_outclrspace_parse(u8 outclrspace)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();

	if(outclrspace == 0){
		ctx->mode.pvideo.mencodingout = (encoding_t)HDMI_VIDEO_MODE_RGB888;
	} else if (outclrspace == 1){
		ctx->mode.pvideo.mencodingout = (encoding_t)HDMI_VIDEO_MODE_YCBCR444;
	} else if (outclrspace == 2){
		ctx->mode.pvideo.mencodingout = (encoding_t)HDMI_VIDEO_MODE_YCBCR422;
	} else {
		pr_err("%s: Invalid Param outclrspace(%d) \n", __func__, outclrspace);
		return -1;
	}
	ctx->hdmi_tx.snps_hdmi_ctrl.csc_on =
			(ctx->mode.pvideo.mencodingin == ctx->mode.pvideo.mencodingout) ? 0: 1;

	if(hdmitx_start() < 0)
		return -1;

	return 0;
}

int hdmi_proc_control_cmd(u8 cmd)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();

	if(cmd == 1){
		if(phy_hot_plug_state(&ctx->hdmi_tx)){
			if (start_handler()) {
				pr_err("%s: start_handler failed \n", __func__);
				return -1;
			}
		} else {
			pr_err("%s: Hot Plug = %s \n",
				__func__, phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		}
	} else if(cmd == 0){
		if (phy_hot_plug_state(&ctx->hdmi_tx)){
			if (stop_handler()) {
				pr_err("%s: stop_handler failed \n", __func__);
				return -1;
			}
		} else
			pr_err("%s:Hot Plug = %s \n",
				__func__, phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
	} else {
		pr_err("%s: Invalid Param cmd(%d) \n", __func__, cmd);
		return -EINVAL;
	}

	return 0;
}

int scdc_support(sink_edid_t *sink_cap)
{
	int ret = 0;
	if (!sink_cap || !sink_cap->edid_mhdmi_forumvsdb.m_valid ||
		!sink_cap->edid_mhdmi_forumvsdb.m_scdc_Present) {
		pr_err("%s: Sink does not support SCDC \n", __func__);
		ret = -1;
	}

	return ret;
}

int scdc_enable_scrambling(struct hdmi_tx_ctx *ctx, u8 enable)
{
	hdmi_tx_dev_t *dev;
	u32 pclk;
	int res = 0;

	if (scdc_support(ctx->mode.sink_cap)) {
		return -1;
	}

	dev = &ctx->hdmi_tx;
	pclk = dev->snps_hdmi_ctrl.pixel_clock;

	if (enable) {
		if (pclk > 340000) {
			res = scrambling(dev, TRUE);
			res = tmds_high_rate(dev, TRUE);
		} else {
			if (scrambling_low_rates) {
				res = scrambling(dev, TRUE);
				res = tmds_high_rate(dev, FALSE);
			}
		}

	} else {
		if (pclk > 340000) {
			res = scrambling(dev, FALSE);
			res = tmds_high_rate(dev, TRUE);
		} else {
			res = scrambling(dev, FALSE);
			res = tmds_high_rate(dev, FALSE);
		}
	}

	return res;
}

int scdc_read_cmd(struct hdmi_tx_ctx *ctx, u32 addr, u32 size)
{
	u8 *data;
	int i;

	if (scdc_support(ctx->mode.sink_cap)) {
		return -1;
	}

	if ((size < 1) || (size + addr > 0xFF)) {
		pr_err("%s: Improper arguments \n", __func__);
		pr_err("%s: Addr Value from 0x01 - 0xFF \n", __func__);
		return -1;
	}

	data = (u8*) vmalloc(sizeof(u8) * size);
	if(!data) {
		pr_err("%s: Failed to allocate buffer \n", __func__);
		return -1;
	}

	memset(data, 0, size);

	if (scdc_read(&ctx->hdmi_tx, (u8)addr, (u8)size, data)) {
		pr_err("%s: SCDC Read failed to address 0x%02x \n", __func__, addr);
		vfree(data);
		return -1;
	}

	for (i = 0; i < size; i++)
		pr_info("SCDC Read[0x%02x] = 0x%02x \n", addr + i, data[i]);

	vfree(data);
	return 0;
}

int scdc_write_cmd(struct hdmi_tx_ctx *ctx, u32 addr, u32 data)
{
	if (scdc_support(ctx->mode.sink_cap)) {
		return -1;
	}

	if ((addr != 2) && (addr != 0x10) && (addr != 0x11) &&
		(addr != 0x20) && (addr != 0x30) && (addr != 0x30) &&
		(addr != 0xC0) && ((addr < 0xDE) || (addr > 0xFF))) {
		pr_err("%s: Improper arguments \n", __func__);
		pr_err("%s: Addr: 0x02, 0x10, 0x11, 0x20, 0x30, 0xC0 and 0xDE to 0xFF \n", __func__);
		return -1;
	}

	if ((data > 0xFF)) {
		pr_err("%s: Improper arguments \n", __func__);
		pr_err("%s: data to write 0x00-0xFF \n", __func__);
		return -1;
	}

	if (scdc_write(&ctx->hdmi_tx, (u8)addr, (u8)data, (u8*)&data)) {
		pr_err("%s: SCDC Write failed to address 0x%02x \n", __func__, addr);
		return -1;
	}

	return 0;
}

int scdc_get_channel_status(struct hdmi_tx_ctx *ctx)
{
	u8 data;
	u8 er_data[7] = {0};

	if (scdc_support(ctx->mode.sink_cap)) {
		return -1;
	}

	data = scdc_read_channel_status(&ctx->hdmi_tx);

	if (scdc_read_err_characters(&ctx->hdmi_tx, er_data)) {
		pr_err("%s : SCDC read err characters Failed \n", __func__);
		return -1;
	}

	pr_info("Status: Clock=[%s] Ln0=[%s], Ln1=[%s], Ln2=[%s] \n",
		data & SCDC_STATUS_FLAG_0_CLK ? "LODCKED": "NOT_LOCKED",
		data & SCDC_STATUS_FLAG_0_CH0 ? "LODCKED": "NOT_LOCKED",
		data & SCDC_STATUS_FLAG_0_CH1 ? "LODCKED": "NOT_LOCKED",
		data & SCDC_STATUS_FLAG_0_CH2 ? "LODCKED": "NOT_LOCKED");

	pr_info("CED: CH0=[0x%02X%02X], CH1=[0x%02X%02X], CH2=[0x%02X%02X]",
			er_data[1] & 0x70, er_data[0],
			er_data[3] & 0x70, er_data[2],
			er_data[5] & 0x70, er_data[4]);
	pr_info("CED: CheckSum = [0x%02x] \n", er_data[6]);

	return 0;
}

int hdmi_proc_scdc_cmd(u8 scdc, u32 addr, u32 data)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();
	if (scdc == 0) {
		if (scdc_enable_scrambling(ctx, FALSE)) {
			pr_err("%s: SCDC Close Failed \n", __func__);
			return -1;
		}
	} else if (scdc == 1) {
		if (scdc_enable_scrambling(ctx, TRUE)) {
			pr_err("%s: SCDC Open Failed \n", __func__);
			return -1;
		}
	} else if(scdc == 2) {
		if (scdc_get_channel_status(ctx)) {
			pr_err("%s: SCDC Get Channel Status Failed \n", __func__);
			return -1;
		}
	} else if(scdc == 3) {
		if (scdc_read_cmd(ctx, addr, data)) {
			pr_err("%s:SCDC Read failed \n", __func__);
			return -1;
		}
	} else if (scdc == 4) {
		if (scdc_write_cmd(ctx, addr, data)) {
			pr_err("%s:SCDC Write failed \n", __func__);
			return -1;
		}
	} else {
		pr_err("%s: Invalid Param scdc(%d) \n", __func__, scdc);
		return -EINVAL;
	}

	return 0;
}

int phy_read_cmd(struct hdmi_tx_ctx *ctx, u32 addr)
{
	u32 data = 0;

	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	if (addr > 0x3b) {
		pr_err("%s: Improper arguments \n", __func__);
		pr_err("%s: addr from 0x00 - 0x3B \n", __func__);
		return -1;
	}

	if (phy_read(&ctx->hdmi_tx, (u16)addr, &data)) {
		pr_err("%s: Phy Read failed to address 0x%02x\n", __func__, addr);
		return -1;
	}

	pr_info("Phy Read[0x%02x] = 0x%02x \n", addr, data);

	return 0;
}

int phy_write_cmd(struct hdmi_tx_ctx *ctx, u16 addr, u32 data)
{
	if (!phy_hot_plug_state(&ctx->hdmi_tx)) {
		pr_err("Hot Plug = %s\n", phy_hot_plug_state(&ctx->hdmi_tx) ? "ON" : "OFF");
		return HDMI_ERR_HPD_FAILED;
	}

	if (addr > 0x3b || data > 0xFFFF) {
		pr_err("%s: Improper arguments \n", __func__);
		return -1;
	}

	if (phy_write(&ctx->hdmi_tx, (u16)addr, data)) {
		pr_err("%s: Phy Write failed to address 0x%02x\n", __func__, addr);
		return -1;
	}

	return 0;
}

int hdmi_proc_phy_cmd(u8 phy, u32 addr, u32 data)
{
	struct hdmi_tx_ctx *ctx = get_hdmi_ctx();

	if (phy == 0) {
		if (phy_read_cmd(ctx, addr)) {
			pr_err("%s:PHY Read failed \n", __func__);
			return -1;
		}

	} else if (phy == 1) {
		if (phy_write_cmd(ctx, addr, data)) {
			pr_err("%s:PHY Write failed \n", __func__);
			return -1;
		}
	} else {
		pr_err("%s: Invalid Param Phy(%d) \n", __func__, phy);
		return -EINVAL;
	}

	return 0;
}

static int hdmitx_drv_fasync(int fd, struct file *file, int on)
{
	if (fasync_helper(fd, file, on, &hdmi_fasync) >= 0)
		return 0;
	else
		return -EIO;
}

int get_current_event_id(u32* event_id)
{
	*event_id = hdmitx_event_id;
	return 0;
}

int hdmitx_open(struct inode *inode, struct file *file)
{
	s32 ret = 0;
	struct hdmitx_dev *hdmi_dev;

	hdmi_dev = container_of(inode->i_cdev, struct hdmitx_dev, cdev);
	file->private_data = hdmi_dev;

	atomic_inc(&dev_open_cnt);

	return ret;
}

int hdmitx_release(struct inode *inode, struct file *file)
{
	s32 ret = 0;

	atomic_dec(&dev_open_cnt);
	if (!atomic_read(&dev_open_cnt)) {
		struct hdmitx_dev *dev_hdmi;
		dev_hdmi = container_of(inode->i_cdev, struct hdmitx_dev, cdev);
	}

	hdmitx_drv_fasync(-1, file, 0);
	hdmitx_event_id = 0;

	return ret;
}

#ifdef CONFIG_COMPAT
static long hdmitx_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

const struct file_operations hdmitx_fops = {
	.owner = THIS_MODULE,
	.open = hdmitx_open,
	.release = hdmitx_release,
	.unlocked_ioctl = hdmitx_ioctl,
	.fasync  = hdmitx_drv_fasync,
#ifdef CONFIG_COMPAT
	.compat_ioctl = hdmitx_compat_ptr_ioctl,
#endif
};

static int hdmitx_register_cdev(struct hdmitx_dev *dev)
{
	struct device *dev_t;
	int err = 0;

	dev->hdmi_class = class_create(THIS_MODULE, HDMI_CLASS_NAME);
	if (IS_ERR(dev->hdmi_class)) {
		dev_err(dev->parent_dev, "create class failed\n");
		return PTR_ERR(dev->hdmi_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&dev->cdev_id, 0, 1, HDMI_DEV_NAME)) < 0) {
		err = -EBUSY;
		dev_err(dev->parent_dev, "allocate chrdev failed\n");
		goto alloc_chrdev_region_err;

	}

	/* initialize the device structure and register the device with the kernel */
	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &hdmitx_fops);
	if ((cdev_add(&dev->cdev, dev->cdev_id, 1)) < 0) {
		err = -EBUSY;
		dev_err(dev->parent_dev, "add chrdev failed\n");
		goto cdev_add_err;
	}

	dev_t = device_create(dev->hdmi_class, dev->parent_dev, dev->cdev_id, NULL, "%s", HDMI_DEV_NAME);
	if (IS_ERR(dev_t)) {
		dev_err(dev->parent_dev, "device create failed error code(%ld)\n", PTR_ERR(dev_t));
		err = PTR_ERR(dev_t);
		goto device_create_err;
	}

	return err;

device_create_err:
	cdev_del(&dev->cdev);
cdev_add_err:
	unregister_chrdev_region(dev->cdev_id, 1);
alloc_chrdev_region_err:
	class_destroy(dev->hdmi_class);
	return err;
}

int register_interrupts(struct hdmitx_dev *dev)
{
	int ret = 0;

	ret = devm_request_threaded_irq(dev->parent_dev, dev->irq[0], _hdmi_tx_handler,
									dwc_hdmi_tx_handler, IRQF_SHARED, "vo_dw_hdmi", dev);
	if (ret){
		pr_err("%s:Could not register dwc_hdmi_tx interrupt\n", __func__);
	}

	return ret;
}

void release_interrupts(struct hdmitx_dev *dev)
{
	int i = 0;
	for(i = 0; i < (sizeof(dev->irq) / sizeof(dev->irq[0])); i++){
		if(dev->irq[i]){
			devm_free_irq(dev->parent_dev, dev->irq[i], dev);
		}
	}
}

void free_all_mem(void)
{
	if(alloc_list != NULL){
		while(alloc_list->instance != 0){
			struct mem_alloc *this;
			this = alloc_list->last;
			// cut this from list
			alloc_list->last = this->prev;
			alloc_list->instance--;
			alloc_list->size -= this->size;
			// free allocated memory
			kfree(this->pointer);
			// free this memory
			pr_debug( KERN_INFO "%s:Freeing: %s\n", __func__, this->info);
			kfree(this);
		}

		kfree(alloc_list);
		alloc_list = NULL;
	}
}

void cvitek_hdmi_clk_set(u32 pclk)
{
	dphy_dsi_set_pll(1, pclk, 4, 24);
}

void hdmitx_create_proc(struct hdmitx_dev * dev)
{
	int ret;
	ret = hdmi_proc_init(dev);
	if(ret){
		pr_err("hdmi proc init failed\n");
		goto err_hdmi_proc;
	}

	ret = hdmi_video_proc_init(dev);
	if(ret){
		pr_err("hdmi video proc init failed\n");
		goto err_hdmi_video_proc;
	}

	ret = hdmi_audio_proc_init(dev);
	if(ret){
		pr_err("hdmi audio proc init failed\n");
		goto err_hdmi_audio_proc;
	}

	ret = hdmi_sink_proc_init(dev);
	if(ret){
		pr_err("hdmi sink proc init failed\n");
		goto err_hdmi_sink_proc;
	}

	return;

err_hdmi_proc:
	hdmi_proc_remove(dev);
	return;
err_hdmi_video_proc:
	hdmi_video_proc_remove(dev);
	return;
err_hdmi_audio_proc:
	hdmi_audio_proc_remove(dev);
	return;
err_hdmi_sink_proc:
	hdmi_sink_proc_remove(dev);
	return;
}

void hdmitx_destroy_proc(struct hdmitx_dev * dev)
{
	hdmi_proc_remove(dev);
	hdmi_video_proc_remove(dev);
	hdmi_audio_proc_remove(dev);
	hdmi_sink_proc_remove(dev);
}

static int hdmi_tx_probe(struct platform_device *pdev)
{
	int error = 0;
	int ret = 0;
	int i = 0;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct resource *mem = NULL;

	void* hdmi_reg_base;
	pr_info("hdmi_tx_probe initiated!");

	dev_hdmi = devm_kzalloc(dev, sizeof(*dev_hdmi), GFP_KERNEL);
	if(!dev_hdmi) {
		pr_err("%s:Could not allocated hdmi_tx_dev\n", __func__);
		return -ENOMEM;
	}

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if(!ctx) {
		pr_err("%s:Could not allocated ctx\n", __func__);
		return -ENOMEM;
	}

	//set clock
	dphy_init(DISP1, VO_DISP_INTF_HDMI);
	cvitek_hdmi_clk_set(10368);

	// Zero the device
	memset(dev_hdmi, 0, sizeof(struct hdmitx_dev));
	memset(ctx, 0, sizeof(struct hdmi_tx_ctx));
	memset(&(ctx->hdmi_tx), 0, sizeof(hdmi_tx_dev_t));
	platform_set_drvdata(pdev, dev_hdmi);

	// Update the device node
	dev_hdmi->parent_dev = dev;

  	pr_info("****************************************\n");
	pr_info("%s:Installing SNPS HDMI module\n", __func__);
	pr_info("****************************************\n");
	pr_info("%s:Device registration\n", __func__);

	dev_hdmi->device_name = "HDMI TX";
	pr_info("%s:Driver's name '%s'\n", __func__, dev_hdmi->device_name);

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(mem == NULL){
		pr_err("%s:Base address of the device is not set.\n"
					 "Refer to device tree.\n", __func__);
		error = -ENXIO;
		goto free_mem;
	}

	hdmi_reg_base = devm_ioremap(dev, mem->start, resource_size(mem));
	if (!hdmi_reg_base) {
		pr_err("%s:failed to map hdmi registers\n", __func__);
		return -ENXIO;
	}

	hdmi_set_reg_base(hdmi_reg_base);
	dev_hdmi->base_address = hdmi_reg_base;
	/* Get IRQ numbers from device*/
	pr_info("%s:Get IRQ numbers\n", __func__);
	while (i < 1) {
		dev_hdmi->irq[i] = platform_get_irq(pdev, i);
		if (dev_hdmi->irq[i] <= 0)
			break;
		pr_info("%s:IRQ number %d.\n", __func__, dev_hdmi->irq[i]);
		i++;
	}

	ret = hdmitx_register_cdev(dev_hdmi);
	if (ret) {
		pr_err("Failed to register hdmi dev, err %d\n", ret);
		return ret;
	}

	// Now that everything is fine, let's add it to device list
	list_add_tail(&dev_hdmi->devlist, &devlist_global);

	irq_mask_all(&ctx->hdmi_tx);

	/*
	 * Configure the PHY RX SENSE and HPD interrupts polarities and clear
	 * any pending interrupt.
	 */
	dev_write(PHY_POL0, PHY_POL0_HPD_MASK |
						PHY_POL0_RX_SENSE_0_MASK |
						PHY_POL0_RX_SENSE_1_MASK |
						PHY_POL0_RX_SENSE_2_MASK |
						PHY_POL0_RX_SENSE_3_MASK );
	/* Enable cable hot plug irq. */
	dev_write(PHY_MASK0, (u8)~(PHY_MASK0_HPD_MASK |
						PHY_MASK0_RX_SENSE_0_MASK |
						PHY_MASK0_RX_SENSE_1_MASK |
						PHY_MASK0_RX_SENSE_2_MASK |
						PHY_MASK0_RX_SENSE_3_MASK ));

	/* Clear and unmute interrupts. */
	dev_write(IH_PHY_STAT0, IH_PHY_STAT0_HPD_MASK |
						IH_PHY_STAT0_RX_SENSE_0_MASK |
						IH_PHY_STAT0_RX_SENSE_1_MASK |
						IH_PHY_STAT0_RX_SENSE_2_MASK |
						IH_PHY_STAT0_RX_SENSE_3_MASK );
	dev_write(IH_MUTE_PHY_STAT0, ~(IH_MUTE_PHY_STAT0_HPD_MASK |
						IH_MUTE_PHY_STAT0_RX_SENSE_0_MASK |
						IH_MUTE_PHY_STAT0_RX_SENSE_1_MASK |
						IH_MUTE_PHY_STAT0_RX_SENSE_2_MASK |
						IH_MUTE_PHY_STAT0_RX_SENSE_3_MASK ));
	irq_unmute(&ctx->hdmi_tx);

	/*
	* Read high and low time from device tree. If not available use
	* the default timing scl clock rate is about 100KHz.
	*/
	if (of_property_read_u32(np, "ddc-i2c-scl-high-time-ns",
		&ctx->hdmi_tx.i2c.scl_high_ns))
		ctx->hdmi_tx.i2c.scl_high_ns = 4500;

	if (of_property_read_u32(np, "ddc-i2c-scl-low-time-ns",
		&ctx->hdmi_tx.i2c.scl_low_ns))
		ctx->hdmi_tx.i2c.scl_low_ns = 5200;

	register_interrupts(dev_hdmi);
	hdmitx_create_proc(dev_hdmi);

	return ret;

free_mem:
	free_all_mem();
	return error;
}

static int hdmi_tx_exit(struct platform_device *pdev){

	struct hdmitx_dev *dev = platform_get_drvdata(pdev);
	struct list_head *list;

	pr_info("**************************************\n");
	pr_info("%s:Removing HDMI module\n", __func__);
	pr_info("**************************************\n");

	disable_irq(dev->irq[0]);
	irq_mute(&ctx->hdmi_tx);

	stop_handler();
	hdmitx_deinit();
	hdmitx_destroy_proc(dev);

	if(ctx->mode.sink_cap != NULL){
		vfree(ctx->mode.sink_cap);
		ctx->mode.sink_cap = NULL;
	}

	while(!list_empty(&devlist_global)){
		list = devlist_global.next;
		list_del(list);
		dev = list_entry(list, struct hdmitx_dev, devlist);
		if(dev == NULL){
			continue;
		}

		pr_info("%s:Release interrupts\n", __func__);
		release_interrupts(dev);

		pr_debug("dev->base_address:%x\n", *(unsigned int *)(dev->base_address));
		devm_iounmap(&pdev->dev, dev->base_address);
		free_all_mem();
	}

	device_destroy(dev->hdmi_class, dev->cdev_id);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->cdev_id, 1);
	class_destroy(dev->hdmi_class);
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

#ifdef CONFIG_PM
static int hdmi_tx_resume(struct platform_device *pdev)
{
	struct hdmitx_dev *dev = platform_get_drvdata(pdev);

	enable_irq(dev->irq[0]);
	is_resume = true;
	hdmitx_start();
	pr_info("hdmi resumed\n");

	return 0;
}

static int hdmi_tx_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct hdmitx_dev *dev = platform_get_drvdata(pdev);

	disable_irq(dev->irq[0]);
	hdmitx_stop();
	pr_info("hdmi suspended\n");

	return 0;
}
#endif

static const struct of_device_id dw_hdmi_tx[] = {
	{ .compatible =	"sophgo,vo_hdmi" },
	{ }
};
MODULE_DEVICE_TABLE(of, dw_hdmi_tx);

static struct platform_driver __refdata dwc_hdmi_tx_pdrv = {
	.remove = hdmi_tx_exit,
	.probe = hdmi_tx_probe,
	.driver = {
		.name = "sophgo,vo_hdmi",
		.owner = THIS_MODULE,
		.of_match_table = dw_hdmi_tx,
	},
#ifdef CONFIG_PM
	.resume = hdmi_tx_resume,
	.suspend = hdmi_tx_suspend,
#endif
};
module_platform_driver(dwc_hdmi_tx_pdrv);

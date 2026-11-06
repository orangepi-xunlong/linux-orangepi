#include "edid/hdmivsdb.h"
#include "util/util.h"

void hdmivsdb_reset(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb)
{
	int i, j = 0;
	vsdb->m_physical_address = 0;
	vsdb->m_supports_ai = FALSE;
	vsdb->m_deep_color30 = FALSE;
	vsdb->m_deep_color36 = FALSE;
	vsdb->m_deep_color48 = FALSE;
	vsdb->m_deep_color_y444 = FALSE;
	vsdb->m_dvi_dual = FALSE;
	vsdb->m_max_tmds_clk = 0;
	vsdb->m_video_latency = 0;
	vsdb->m_audio_latency = 0;
	vsdb->m_interlaced_video_latency = 0;
	vsdb->m_interlaced_audio_latency = 0;
	vsdb->m_id = 0;
	vsdb->m_content_type_support = 0;
	vsdb->m_hdmi_vic_count = 0;
	for (i = 0; i < MAX_HDMI_VIC; i++) {
		vsdb->m_hdmi_vic[i] = 0;
	}
	vsdb->m_3dpresent = FALSE;
	for (i = 0; i < MAX_VIC_WITH_3D; i++) {
		for (j = 0; j < MAX_HDMI_3DSTRUCT; j++) {
			vsdb->m_video3d_struct[i][j] = 0;
		}
	}
	for (i = 0; i < MAX_VIC_WITH_3D; i++) {
		for (j = 0; j < MAX_HDMI_3DSTRUCT; j++) {
			vsdb->m_detail3d[i][j] = ~0;
		}
	}
	vsdb->mvalid = FALSE;
}

int hdmivsdb_parse(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 * data)
{
	u8 block_length = 0;
	unsigned video_info_start = 0;
	unsigned hdmi3d_start = 0;
	unsigned hdmi_vic_len = 0;
	unsigned hdmi3dlen = 0;
	unsigned spanned3d = 0;
	unsigned i = 0;
	unsigned j = 0;

	hdmivsdb_reset(dev, vsdb);
	if (data == 0) {
		return FALSE;
	}
	if (bit_field(data[0], 5, 3) != 0x3) {
		pr_err("Invalid datablock tag");
		return FALSE;
	}
	block_length = bit_field(data[0], 0, 5);
	if (block_length < 5) {
		pr_err("%s Invalid minimum length (%d)", __func__, block_length);
		return FALSE;
	}
	if (byte_to_dword(0x00, data[3], data[2], data[1]) != HDMI_LICENSING_LLC_OUI) {
		pr_err("HDMI IEEE registration identifier not valid");
		return FALSE;
	}
	hdmivsdb_reset(dev, vsdb);
	vsdb->m_id = HDMI_LICENSING_LLC_OUI;
	vsdb->m_physical_address = byte_to_word(data[4], data[5]);
	/* parse extension fields if they exist */
	if (block_length > 5) {
		vsdb->m_supports_ai = bit_field(data[6], 7, 1) == 1;
		vsdb->m_deep_color48 = bit_field(data[6], 6, 1) == 1;
		vsdb->m_deep_color36 = bit_field(data[6], 5, 1) == 1;
		vsdb->m_deep_color30 = bit_field(data[6], 4, 1) == 1;
		vsdb->m_deep_color_y444 = bit_field(data[6], 3, 1) == 1;
		vsdb->m_dvi_dual = bit_field(data[6], 0, 1) == 1;
	} else {
		vsdb->m_supports_ai = FALSE;
		vsdb->m_deep_color48 = FALSE;
		vsdb->m_deep_color36 = FALSE;
		vsdb->m_deep_color30 = FALSE;
		vsdb->m_deep_color_y444 = FALSE;
		vsdb->m_dvi_dual = FALSE;
	}
	vsdb->m_max_tmds_clk = (block_length > 6) ? data[7] : 0;
	vsdb->m_video_latency = 0;
	vsdb->m_audio_latency = 0;
	vsdb->m_interlaced_video_latency = 0;
	vsdb->m_interlaced_audio_latency = 0;
	if (block_length > 7) {
		if (bit_field(data[8], 7, 1) == 1) {
			if (block_length < 10) {
				pr_err("Invalid length - latencies are not valid");
				return FALSE;
			}
			if (bit_field(data[8], 6, 1) == 1) {
				if (block_length < 12) {
					pr_err("Invalid length - Interlaced latencies are not valid");
					return FALSE;
				} else {
					vsdb->m_video_latency = data[9];
					vsdb->m_audio_latency = data[10];
					vsdb->m_interlaced_video_latency = data[11];
					vsdb->m_interlaced_audio_latency = data[12];
					video_info_start = 13;
				}
			} else {
				vsdb->m_video_latency = data[9];
				vsdb->m_audio_latency = data[10];
				vsdb->m_interlaced_video_latency = 0;
				vsdb->m_interlaced_audio_latency = 0;
				video_info_start = 11;
			}
		} else {	/* no latency data */
			vsdb->m_video_latency = 0;
			vsdb->m_audio_latency = 0;
			vsdb->m_interlaced_video_latency = 0;
			vsdb->m_interlaced_audio_latency = 0;
			video_info_start = 9;
		}
		vsdb->m_content_type_support = bit_field(data[8], 0, 4);
	}
	if (bit_field(data[8], 5, 1) == 1) {	/* additional video format capabilities are described */
		vsdb->m_image_size = bit_field(data[video_info_start], 3, 2);
		hdmi_vic_len = bit_field(data[video_info_start + 1], 5, 3);
		hdmi3dlen = bit_field(data[video_info_start + 1], 0, 5);
		for (i = 0; i < hdmi_vic_len; i++) {
			vsdb->m_hdmi_vic[i] = data[video_info_start + 2 + i];
		}
		pr_debug("HDMI VIC Count %d", hdmi_vic_len);
		pr_debug("HDMI 3D Present %d", bit_field(data[video_info_start], 7, 1));
		vsdb->m_hdmi_vic_count = hdmi_vic_len;
		if (bit_field(data[video_info_start], 7, 1) == 1) {	/* 3d present */
			vsdb->m_3dpresent = TRUE;
			vsdb->hdmi3dcount = hdmi3dlen;
			hdmi3d_start = video_info_start + hdmi_vic_len + 2;
			/* 3d multi 00 -> both 3d_structure_all and 3d_mask_15 are NOT present */
			/* 3d mutli 11 -> reserved */
			if (bit_field(data[video_info_start], 5, 2) == 1) {	/* 3d multi 01 */
				/* 3d_structure_all is present but 3d_mask_15 not present */
				for (j = 0; j < 16; j++) {	/* j spans 3d structures */
					if (bit_field(data[hdmi3d_start + (j / 8)], (j % 8), 1) == 1) {
						int struct3d = (j < 8)	? j + 8 : j - 8;
						for (i = 0; i < 16; i++) {	/* go through 2 registers, [videoInfoStart + hdmiVicLen + 1] & [videoInfoStart + hdmiVicLen + 2]  */
							vsdb->m_video3d_struct[i][struct3d] = 1;
						}
					}
				}
				spanned3d = 2;
				/*hdmi3d_start += 2;
				   hdmi3dLen -= 2; */
			} else if (bit_field(data[video_info_start], 5, 2) == 2) {	/* 3d multi 10 */
				/* 3d_structure_all and 3d_mask_15 are present */
				for (j = 0; j < 16; j++) {
					for (i = 0; i < 16; i++) {	/* assign according to mask, through 2 registers, [videoInfoStart + hdmiVicLen + 3] & [videoInfoStart + hdmiVicLen + 4] */
						if (bit_field(data[hdmi3d_start + 2 + (i / 8)], (i % 8), 1) == 1) {	/* go through 2 registers, [videoInfoStart + hdmiVicLen + 1] & [videoInfoStart + hdmiVicLen + 2]  */
							vsdb->m_video3d_struct[(i < 8) ?
							i + 8 : i - 8][(j < 8) ? j + 8 : j - 8] =
							bit_field(data[hdmi3d_start + (j / 8)], (j % 8), 1);
						}
					}
				}
				spanned3d = 4;
			}
			if (hdmi3dlen > spanned3d) {
				hdmi3d_start += spanned3d;
				for (i = 0, j = 0; i < (hdmi3dlen - spanned3d); i++) {
#if 0
					if(i == 0){
						vsdb->hdmi3dfirst = bit_field(data[hdmi3d_start + i + j], 4, 4);
						vsdb->hdmi3dfirststrc = bit_field(data[hdmi3d_start + i + j], 0, 4);
					}
#endif
					pr_debug("3D struct %d - %d",
						bit_field(data[hdmi3d_start + i + j], 4, 4),
						bit_field(data[hdmi3d_start + i + j], 0, 4));
					vsdb->m_video3d_struct[bit_field(data[hdmi3d_start + i + j], 4, 4)]
						[bit_field(data[hdmi3d_start + i + j], 0, 4)] = 1;

					if (bit_field(data[hdmi3d_start + i + j], 0, 4) > 7) {	/* bytes with 3D_Detail_X and Reserved(0) are present only when 3D_Structure_X > b'1000 - side-by-side(half) */
						j++;
						pr_debug("3D_Detail_X struct %d - %d - %d",
							bit_field(data[hdmi3d_start + i + j], 4, 4),
							bit_field(data[hdmi3d_start + i + j], 4, 4),
							bit_field(data[hdmi3d_start + i + j], 4, 4));
						vsdb->m_detail3d[bit_field(data[hdmi3d_start + i + j], 4, 4)]
							[bit_field(data[hdmi3d_start + i + j], 4, 4)] =
							bit_field(data[hdmi3d_start + i + j], 4, 4);
					}
				}
			}
		} else {	/* 3d NOT present */
			vsdb->m_3dpresent = FALSE;
		}
	}
	vsdb->mvalid = TRUE;
	return TRUE;
}

u16 get_index_supported_3dstructs(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 index)
{
	u16 structs3d = 0;
	int i;
	for (i = 0; i < MAX_HDMI_3DSTRUCT; i++) {
		structs3d |= (vsdb->m_video3d_struct[index][i] & 1) << i;
	}
	return structs3d;
}

u16 get_3dstruct_indexes(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 struct3d)
{
	u16 indexes = 0;
	int i;
	for (i = 0; i < MAX_HDMI_3DSTRUCT; i++) {
		indexes |= (vsdb->m_video3d_struct[i][struct3d] & 1) << i;
	}
	return indexes;
}

void hdmiforumvsdb_reset(hdmi_tx_dev_t *dev, hdmiforumvsdb_t * forumvsdb)
{
	forumvsdb->m_valid = FALSE;
	forumvsdb->m_ieee_oui = 0;
	forumvsdb->m_version = 0;
	forumvsdb->m_max_tmds_char_rate = 0;
	forumvsdb->m_scdc_Present = FALSE;
	forumvsdb->m_rr_capable = FALSE;
	forumvsdb->m_lts_340mcs_scramble = FALSE;
	forumvsdb->m_independent_view = FALSE;
	forumvsdb->m_dual_view = FALSE;
	forumvsdb->m_3d_osd_disparity = FALSE;
	forumvsdb->mdc_30bit_420 = FALSE;
	forumvsdb->mdc_36bit_420 = FALSE;
	forumvsdb->mdc_48bit_420 = FALSE;
}

int hdmiforumvsdb_parse(hdmi_tx_dev_t *dev, hdmiforumvsdb_t * forumvsdb, u8 * data)
{
	u16 block_length;
	hdmiforumvsdb_reset(dev, forumvsdb);
	if (data == 0) {
		return FALSE;
	}
	if (bit_field(data[0], 5, 3) != 0x3) {
		pr_err("Invalid datablock tag");
		return FALSE;
	}
	pr_debug("block TAG ok");
	block_length = bit_field(data[0], 0, 5);
	if (block_length < 7) {
		pr_err("Invalid minimum length");
		return FALSE;
	}
	pr_debug("size ok");
	if (byte_to_dword(0x00, data[3], data[2], data[1]) != 0xC45DD8) {
		pr_err("HDMI IEEE registration identifier not valid");
		return FALSE;
	}
	pr_debug("ieeeid TAG ok");
	forumvsdb->m_version = bit_field(data[4], 0, 7);
	forumvsdb->m_max_tmds_char_rate = bit_field(data[5], 0, 7);
	forumvsdb->m_scdc_Present = bit_field(data[6], 7, 1);
	forumvsdb->m_rr_capable = bit_field(data[6], 6, 1);
	forumvsdb->m_lts_340mcs_scramble = bit_field(data[6], 3, 1);
	forumvsdb->m_independent_view = bit_field(data[6], 2, 1);
	forumvsdb->m_dual_view = bit_field(data[6], 1, 1);
	forumvsdb->m_3d_osd_disparity = bit_field(data[6], 0, 1);
	forumvsdb->mdc_48bit_420 = bit_field(data[7], 2, 1);
	forumvsdb->mdc_36bit_420 = bit_field(data[7], 1, 1);
	forumvsdb->mdc_30bit_420 = bit_field(data[7], 0, 1);
	forumvsdb->m_valid = TRUE;

#if 1
	pr_debug("version %d", bit_field(data[4], 0, 7));
	pr_debug("Max_TMDS_Charater_rate %d", bit_field(data[5], 0, 7));
	pr_debug("SCDC_Present %d", bit_field(data[6], 7, 1));
	pr_debug("RR_Capable %d", bit_field(data[6], 6, 1));
	pr_debug("LTE_340Mcsc_scramble %d", bit_field(data[6], 3, 1));
	pr_debug("Independent_View %d", bit_field(data[6], 2, 1));
	pr_debug("Dual_View %d", bit_field(data[6], 1, 1));
	pr_debug("3D_OSD_Disparity %d", bit_field(data[6], 0, 1));
	pr_debug("DC_48bit_420 %d", bit_field(data[7], 2, 1));
	pr_debug("DC_36bit_420 %d", bit_field(data[7], 1, 1));
	pr_debug("DC_30bit_420 %d", bit_field(data[7], 0, 1));
#endif
	return TRUE;
}

#ifndef _HDMIVSDB_H_
#define _HDMIVSDB_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"

#define MAX_HDMI_VIC		16
#define MAX_HDMI_3DSTRUCT	16
#define MAX_VIC_WITH_3D		16

/** For detailed handling of this structure, refer to documentation of the functions */
typedef struct {
	u16 m_physical_address;

	int m_supports_ai;

	int m_deep_color30;

	int m_deep_color36;

	int m_deep_color48;

	int m_deep_color_y444;

	int m_dvi_dual;

	u16 m_max_tmds_clk;

	u16 m_video_latency;

	u16 m_audio_latency;

	u16 m_interlaced_video_latency;

	u16 m_interlaced_audio_latency;

	u32 m_id;

	u8 m_content_type_support;

	u8 m_image_size;

	int m_hdmi_vic_count;

	u8 m_hdmi_vic[MAX_HDMI_VIC];

	int m_3dpresent;

	int hdmi3dcount; //number of 3d entries

	int hdmi3dfirst; //number of 3d entries

	int hdmi3dfirststrc; //number of 3d entries

	int m_video3d_struct[MAX_VIC_WITH_3D][MAX_HDMI_3DSTRUCT];	/* row index is the VIC number */

	int m_detail3d[MAX_VIC_WITH_3D][MAX_HDMI_3DSTRUCT];	/* index is the VIC number */

	int mvalid;

} hdmivsdb_t;

/* HDMI 2.0 HF_VSDB */
typedef struct {
	u32 m_ieee_oui;

	u8 m_valid;

	u8 m_version;

	u8 m_max_tmds_char_rate;

	u8 m_3d_osd_disparity;

	u8 m_dual_view;

	u8 m_independent_view;

	u8 m_lts_340mcs_scramble;

	u8 m_rr_capable;

	u8 m_scdc_Present;

	u8 mdc_30bit_420;

	u8 mdc_36bit_420;

	u8 mdc_48bit_420;

} hdmiforumvsdb_t;

void hdmiforumvsdb_reset(hdmi_tx_dev_t *dev, hdmiforumvsdb_t * forumvsdb);

int hdmiforumvsdb_parse(hdmi_tx_dev_t *dev, hdmiforumvsdb_t * forumvsdb, u8 * data);

void hdmivsdb_reset(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb);

int hdmivsdb_parse(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 * data);

u16 get_index_supported_3dstructs(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 index);

u16 get_3dstruct_indexes(hdmi_tx_dev_t *dev, hdmivsdb_t * vsdb, u8 struct3d);

#endif	/* _HDMIVSDB_H_ */

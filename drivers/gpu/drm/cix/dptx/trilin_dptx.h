/* SPDX-License-Identifier: GPL-2.0*/
//------------------------------------------------------------------------------
//	Trilinear Technologies DisplayPort DRM Driver
//	Copyright (C) 2023 Trilinear Technologies
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, version 2.
//
//	This program is distributed in the hope that it will be useful, but
//	WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//	General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program. If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#ifndef _TRILIN_DPTX_H_
#define _TRILIN_DPTX_H_

#include <drm/drm_crtc.h>
#include <drm/drm_connector.h>
#include <drm/drm_encoder.h>
#include <drm/drm_print.h>
#include <drm/display/drm_dp_helper.h>
#include <drm/display/drm_dp_mst_helper.h>
#include <drm/display/drm_hdmi_helper.h>
#include <drm/drm_print.h>
#include <drm/drm_atomic.h>

struct trilin_dp;
struct trilin_dpsub;
union phy_configure_opts;

#include "trilin_dptx_audio.h"
#include "dptx_infoframe.h"
#include "trilin_drm.h"
#include "trilin_phy.h"
#include "hdcp/cix_hdcp.h"

//------------------------------------------------------------------------------
//  Defines
//------------------------------------------------------------------------------
#define TRILIN_MAX_FREQ 600000
#define DP_REDUCED_BIT_RATE 162000
#define DP_HIGH_BIT_RATE 270000
#define DP_HIGH_BIT_RATE2 540000
#define DP_HIGH_BIT_RATE3 810000
#define DP_MAX_TRAINING_TRIES 5
#define DP_MAX_TRAINING_LOOP 3
#define DP_V1_2 0x12
#define DP_V1_4 0x14

//------------------------------------------------------------------------------
//  Link Rate Defines
//------------------------------------------------------------------------------
#define TRILIN_DPTX_PHY_CLOCK_SELECT_1_62G 0x1
#define TRILIN_DPTX_PHY_CLOCK_SELECT_2_70G 0x3
#define TRILIN_DPTX_PHY_CLOCK_SELECT_5_40G 0x5
#define TRILIN_DPTX_PHY_CLOCK_SELECT_8_10G 0x7

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_INTERRUPT_STATE bits
//------------------------------------------------------------------------------
#define TRILIN_DPTX_INTERRUPT_STATE_HPD BIT(0)
#define TRILIN_DPTX_INTERRUPT_STATE_REQUEST BIT(1)
#define TRILIN_DPTX_INTERRUPT_STATE_REPLY BIT(2)
#define TRILIN_DPTX_INTERRUPT_STATE_REPLY_TIMEOUT BIT(3)

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_AUX_COMMAND bits
//------------------------------------------------------------------------------
#define TRILIN_DPTX_AUX_COMMAND_CMD_SHIFT 8
#define TRILIN_DPTX_AUX_COMMAND_ADDRESS_ONLY BIT(12)
#define TRILIN_DPTX_AUX_COMMAND_BYTES_SHIFT 0

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_AUX_STATUS_REPLY bits
//------------------------------------------------------------------------------
#define TRILIN_DPTX_AUX_STATUS_REPLY_ERROR BIT(3)
#define TRILIN_DPTX_AUX_STATUS_REQUEST_IN_PROGRESS BIT(2)
#define TRILIN_DPTX_AUX_STATUS_REPLY_IN_PROGRESS BIT(1)
#define TRILIN_DPTX_AUX_STATUS_REPLY_RECEIVED BIT(0)

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_AUX_REPLY_CODE bits
//------------------------------------------------------------------------------
#define TRILIN_DPTX_AUX_REPLY_CODE_AUX_ACK (0)
#define TRILIN_DPTX_AUX_REPLY_CODE_AUX_NACK BIT(0)
#define TRILIN_DPTX_AUX_REPLY_CODE_AUX_DEFER BIT(1)
#define TRILIN_DPTX_AUX_REPLY_CODE_I2C_ACK (0)
#define TRILIN_DPTX_AUX_REPLY_CODE_I2C_NACK BIT(2)
#define TRILIN_DPTX_AUX_REPLY_CODE_I2C_DEFER BIT(3)

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_REPLY_DATA_COUNT bits
//------------------------------------------------------------------------------
#define TRILIN_DPTX_REPLY_DATA_COUNT_MASK 0xff

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_MAIN_STREAM_MISC0 bits
//------------------------------------------------------------------------------
#define TRILIN_DPTX_STREAM_MISC0_SYNC_LOCK BIT(0)
#define TRILIN_DPTX_STREAM_MISC0_COMP_FORMAT_RGB (0 << 1)
#define TRILIN_DPTX_STREAM_MISC0_COMP_FORMAT_YCRCB_422 (5 << 1)
#define TRILIN_DPTX_STREAM_MISC0_COMP_FORMAT_YCRCB_444 (6 << 1)
#define TRILIN_DPTX_STREAM_MISC0_COMP_FORMAT_MASK (7 << 1)
#define TRILIN_DPTX_STREAM_MISC0_DYNAMIC_RANGE BIT(3)
#define TRILIN_DPTX_STREAM_MISC0_YCBCR_COLR BIT(4)
#define TRILIN_DPTX_STREAM_MISC0_BPC_6 (0 << 5)
#define TRILIN_DPTX_STREAM_MISC0_BPC_8 (1 << 5)
#define TRILIN_DPTX_STREAM_MISC0_BPC_10 (2 << 5)
#define TRILIN_DPTX_STREAM_MISC0_BPC_12 (3 << 5)
#define TRILIN_DPTX_STREAM_MISC0_BPC_16 (4 << 5)
#define TRILIN_DPTX_STREAM_MISC0_BPC_MASK (7 << 5)

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_MAIN_STREAM_MISC1 bits
//------------------------------------------------------------------------------
#define TRILIN_DPTX_STREAM_MISC1_Y_ONLY_EN BIT(7)

#define TRILIN_DPTX_VSC_COLORIMETRY_EN BIT(6)
#define TRILIN_DPTX_STREAM_OVERRIDE_ENABLE BIT(6)
#define TRILIN_DPTX_STREAM_OVERRIDE_YCbCR_420 (4 << 3)
#define TRILIN_DPTX_STREAM_OVERRIDE_DSC (5 << 3)
#define TRILIN_DPTX_STREAM_OVERRIDE_BPC_6 (0)
#define TRILIN_DPTX_STREAM_OVERRIDE_BPC_8 (1)
#define TRILIN_DPTX_STREAM_OVERRIDE_BPC_10 (2)
#define TRILIN_DPTX_STREAM_OVERRIDE_BPC_12 (3)
#define TRILIN_DPTX_STREAM_OVERRIDE_BPC_16 (4)
#define TRILIN_DPTX_STREAM_OVERRIDE_BPC_MASK (7)

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_MAIN_STREAM_MISC1 bits
//------------------------------------------------------------------------------
#define TRILIN_DPTX_MSA_TRANSFER_UNIT_SIZE_TU_SIZE_DEF 64

//------------------------------------------------------------------------------
//  Register TRILIN_DPTX_MAIN_STREAM_POLARITY shift values
//------------------------------------------------------------------------------
#define TRILIN_DPTX_MAIN_STREAM_POLARITY_HSYNC_SHIFT 0
#define TRILIN_DPTX_MAIN_STREAM_POLARITY_VSYNC_SHIFT 1
#define TRILIN_DPTX_MAIN_STREAM_POLARITY_DATAENABLE_HIGH 0xc

//------------------------------------------------------------------------------
//  Register	TRILIN_DPTX_INTERRUPT_STATE,
//		TRILIN_DPTX_INTERRUPT_CAUSE,
//		TRILIN_DPTX_INTERRUPT_MASK
//bit values
//------------------------------------------------------------------------------
#define TRILIN_DPTX_INTERRUPT_HDCP_TIMER_IRQ BIT(5)
#define TRILIN_DPTX_INTERRUPT_GP_TIMER_IRQ BIT(4)
#define TRILIN_DPTX_INTERRUPT_REPLY_TIMEOUT BIT(3)
#define TRILIN_DPTX_INTERRUPT_REPLY_RECIEVED BIT(2)
#define TRILIN_DPTX_INTERRUPT_HPD_IRQ BIT(1)
#define TRILIN_DPTX_INTERRUPT_HPD_EVENT BIT(0)

#define TRILIN_DPTX_INTERRUPT_MASK_ALL 0x3f
#define TRILIN_DPTX_INTERRUPT_CFG 0x1c

/* PLATFORM define */
#define CIX_PLATFORM_SOC 0
#define CIX_PLATFORM_EMU 1
#define CIX_PLATFORM_FPGA 2
#define CIX_PLATFORM_INVALID -1

#define TRILIN_DPTX_MAX_LANES 4
#define TRILIN_DPTX_AUX_DIVIDER 200

/* DPTX mem resource index */
#define DPTX_MEM_DP_IDX 0
#define DPTX_MEM_DSC_IDX 1
#define DPTX_MEM_DP_PHY_IDX 2
#define DPTX_MEM_DP_RCSU_IDX 3

/* MST define */
#define TRILIN_DPTX_POSSIBLE_CRTCS_MST 0x3
#define TRILIN_DPTX_POSSIBLE_CRTCS_SST 0x1
#define LINLON_KMS_SIZE 2

/*Reserved Register accese CMD*/
#define CIX_SIP_DP_GOP_CTRL (0xc200000f)
#define SKY1_SIP_DP_GOP_GET 0x1
#define SKY1_SIP_DP_GOP_SET 0x2
#define DP_GOP_MASK 0x00020000
#define DP_GOP_SHIFT 17
/**
 * struct trilin_dpsub - Trilinear Technologies DisplayPort Subsystem
 * @drm: The DRM/KMS device
 * @dev: The physical device
 * @apb_clk: The APB clock
 * @disp: The display controller
 * @dp: The DisplayPort controller
 * @dma_align: DMA alignment constraint (must be a power of 2)
 */
struct trilin_dpsub {
	struct drm_device *drm[LINLON_KMS_SIZE];
	struct device *dev;
	struct clk *apb_clk;
	struct clk *vid_clk0;
	struct clk *vid_clk1;
	struct clk *vid_clk2;
	struct clk *vid_clk3;
	struct trilin_dp *dp;
	struct device_link *link;
	unsigned int dma_align;
};

/**
 * struct trilin_dp_link_config - Common link config between source and sink
 * @max_rate: maximum link rate
 * @max_lanes: maximum number of lanes
 */
struct trilin_dp_link_config {
	int max_rate;
	u8 max_lanes;
};

/**
 * struct trilin_dp_mode - Configured mode of DisplayPort
 * @bw_code: code for bandwidth(link rate)
 * @lane_cnt: number of lanes
 * @pclock: pixel clock frequency of current mode
 * @fmt: format identifier string
 */
struct trilin_dp_mode {
	u8 bw_code;
	u8 lane_cnt;
	int pclock;
	int link_rate;
	const char *fmt;
};

/**
 * struct trilin_dp_config - Configuration of DisplayPort from DTS
 * @misc0: misc0 configuration (per DP v1.2 spec)
 * @misc1: misc1 configuration (per DP v1.2 spec)
 * @bpp: bits per pixel
 */
struct trilin_dp_config {
	u8 misc0;
	u8 misc1;
	u8 bpp;
	u8 bpc;
	u8 override;
	enum trilin_dpsub_format format;
	u32 colorspace;
	u8 dynamic_range;
	u8 content_type;
};

/* stream id */
enum trilin_dp_stream_id {
	DP_STREAM_0,
	DP_STREAM_1,
	DP_STREAM_2,
	DP_STREAM_3,
	DP_STREAM_MAX,
};

#define MAX_DP_MST_DRM_ENCODERS DP_STREAM_MAX

struct trilin_encoder {
	struct drm_encoder base;
	struct trilin_connector *connector;
	struct trilin_dp_panel *dp_panel;
	struct trilin_dp *dp;
	struct drm_device *drm;
	void *enc_priv;
	int vcpi;
	int pbn;
	int num_slots;
	int start_slot;
	u32 id;
	bool enable;
};

enum trilin_output_type {
	TRILIN_OUTPUT_UNUSED = 0,
	TRILIN_OUTPUT_ANALOG = 1,
	TRILIN_OUTPUT_DVO = 2,
	TRILIN_OUTPUT_SDVO = 3,
	TRILIN_OUTPUT_LVDS = 4,
	TRILIN_OUTPUT_TVOUT = 5,
	TRILIN_OUTPUT_HDMI = 6,
	TRILIN_OUTPUT_DP = 7,
	TRILIN_OUTPUT_EDP = 8,
	TRILIN_OUTPUT_DSI = 9,
	TRILIN_OUTPUT_DDI = 10,
	TRILIN_OUTPUT_DP_MST = 11,
};


struct trilin_connector {
	struct drm_connector base;
	uint32_t id;
	struct drm_dp_mst_port *port;
	struct drm_device *drm;
	void *con_priv;

	struct trilin_dp *dp;
	struct trilin_dp_panel *dp_panel;
	enum trilin_output_type type;
	struct drm_atomic_state *state;

	struct trilin_dp_config config;
	struct hdmi_drm_infoframe drm_infoframe;
	struct dp_sdp sdp[CIX_MAX_SDP];
	bool hdr_flush;
	/* Variable Refresh Rate state */
	struct {
		bool enable;
		u16 vmin, vmax;
	} vrr;
};

struct trilin_dp_link_caps {
	bool enhanced_framing;
	bool tps3_supported;
	bool tps4_supported;
	bool fast_training;
	bool channel_coding;
	bool ssc;
	bool mst;
	bool vsc_supported;
	bool vscext_supported;
	bool vscext_chaining_supported;
	bool vrr;
	bool arr;
	bool psr_sink_support;
	bool psr2_sink_support;
};

enum trilin_dptx_state {
	DP_STATE_DISCONNECTED = 0,
	DP_STATE_CONFIGURED = BIT(0),
	DP_STATE_INITIALIZED = BIT(1),
	DP_STATE_READY = BIT(2),
	DP_STATE_CONNECTED = BIT(3),
	DP_STATE_CONNECT_NOTIFIED = BIT(4),
	DP_STATE_DISCONNECT_NOTIFIED = BIT(5),
	DP_STATE_ENABLED = BIT(6),
	DP_STATE_SUSPENDED = BIT(7),
	DP_STATE_INIT_TRAIN = BIT(8),
};

struct trilin_dp_panel {
	/* By default, stream_id is assigned to DP_INVALID_STREAM.
	 * Client sets the stream id value using set_stream_id interface.
	 */
	enum trilin_dp_stream_id stream_id;
	/* DRM connector assosiated with this panel */
	struct trilin_connector *connector;
};

struct trilin_dp_mst_ch_slot_info {
	u32 start_slot;
	u32 tot_slots;
};

struct trilin_dp_mst_channel_info {
	struct trilin_dp_mst_ch_slot_info slot_info[DP_STREAM_MAX];
};

struct trilin_dp_mst_private {
	bool mst_initialized;
	struct drm_dp_mst_topology_mgr mst_mgr;
	struct trilin_encoder mst_encoders[MAX_DP_MST_DRM_ENCODERS];
	struct trilin_dp *dp;
	struct mutex mst_lock;
	struct mutex poll_irq_lock;
	bool mst_session_state;
};

struct trilin_mst_panels {
	bool in_use;
	struct trilin_dp_panel panel;
};

struct trilin_dp_mst {
	bool mst_active;
	bool drm_registered;
	struct trilin_dp_mst_private private_info;
	struct trilin_mst_panels mst_panels[MAX_DP_MST_DRM_ENCODERS];
};

struct trilin_dp_psr {
	u8 sink_sync_latency;
	bool link_standby;
	bool enable;
	bool active;
	bool singleframeupdate;
	bool psr2_enabled;
	bool main_link_keep_active;
};

/**
 * struct trilin_dp - Trilinear DisplayPort core
 * @encoder: the drm encoder structure
 * @connector: the drm connector structure
 * @dev: device structure
 * @dpsub: Display subsystem
 * @drm: DRM core
 * @iomem: device I/O memory for register access
 * @reset: reset controller
 * @irq: irq
 * @config: IP core configuration from DTS
 * @aux: aux channel
 * @phy: PHY handles for DP lanes
 * @num_lanes: number of enabled phy lanes
 * @hpd_work: hot plug detection worker
 * @status: connection status
 * @enabled: flag to indicate if the device is enabled
 * @dpcd: DP configuration data from currently connected sink device
 * @link_config: common link configuration between IP core and sink device
 * @mode: current mode between IP core and sink device
 * @train_set: set of training data
 * @support_d3_cmd:  whether dp-device support D3 cmd, yes: support no: not support
 */
struct trilin_dp {
	struct trilin_encoder encoder; //for sst
	struct trilin_connector connector; //for sst
	struct drm_panel *edp_panel;
	struct device *dev;
	struct trilin_dpsub *dpsub;
	struct drm_device **drm;
	void __iomem *dp_iomem;
	void __iomem *phy_iomem;
	void __iomem *rcsu_iomem;
	struct reset_control *reset;
	struct reset_control *phy_reset;
	int irq;

	struct trilin_dp_config config;
	struct drm_dp_desc desc;
	struct drm_dp_aux aux;
	u32 max_rate;
	u32 num_lanes;
	u32 max_streams;
	u32 max_mst_encoders;
	u32 aux_clock_divider;
	u32 delay_after_hpd;
	u32 enabled_by_gop;
	struct delayed_work hpd_event_work;
	struct delayed_work hpd_irq_work;
	enum drm_connector_status status;
	bool enabled;
	bool psr_default_on;
	bool fasttrain_default_on;
	bool mst_default_on;
	u32 cfg_adapter_port;

	u8 dpcd[DP_RECEIVER_CAP_SIZE];
	u8 psr_dpcd[EDP_PSR_RECEIVER_CAP_SIZE];
	u8 downstream_ports[DP_MAX_DOWNSTREAM_PORTS];
	u8 edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE];

	struct trilin_dp_link_config link_config;
	struct trilin_dp_mode mode;
	struct trilin_dp_link_caps caps;
	u8 train_set[TRILIN_DPTX_MAX_LANES];
	bool train_cr_done;
	bool train_ce_done;

	struct trilin_phy_t phy;
	enum trilin_dptx_state state;
	int platform_id;

	struct drm_display_mode *adjusted_mode;
	struct dptx_audio dp_audio;
	struct cix_hdcp hdcp;
	u8 pixel_per_cycle, force_pixel_per_cycle;
	bool plugin;
	bool hpd_multi_func;

	u32 active_stream_cnt;
	u32 link_request;
	u32 prev_sink_count;
	u32 sink_count;
	struct trilin_dp_mst mst;
	struct mutex session_lock;
	u32 tot_dsc_blks_in_use;
	u32 status_update;

	struct trilin_dp_panel dp_panel;
	struct trilin_dp_panel *active_panels[DP_STREAM_MAX];
	struct trilin_dp_mst_channel_info mst_ch_info;
	struct drm_dp_mst_topology_mgr *mst_mgr;

	bool support_d3_cmd;
	struct trilin_dp_psr psr;
	bool edp_panel_ready;
};

//------------------------------------------------------------------------------
// DEBUG
//------------------------------------------------------------------------------
#define DRM_UT_KMS 1
#define DP_DEBUG(fmt, ...)										\
	do {                                                              \
		if (DRM_UT_KMS)                                           \
			drm_dbg_atomic(dp->drm[0], ""fmt, ##__VA_ARGS__);    \
		else                                                      \
			dev_dbg(dp->dev, "[drm:%s][debug]" fmt, __func__, \
				##__VA_ARGS__);                           \
	} while (0)

#define DP_INFO(fmt, ...)	\
	dev_info(dp->dev, "[drm:%s][info]" fmt, __func__, ##__VA_ARGS__)

#define DP_WARN(fmt, ...) \
	dev_warn(dp->dev, "[drm:%s][warn]" fmt, __func__, ##__VA_ARGS__)

#define DP_ERR(fmt, ...) \
	dev_err(dp->dev, "[drm:%s][ERROR]" fmt, __func__, ##__VA_ARGS__)

#define DP_MST_DEBUG(fmt, ...)									\
	do {                                                              \
		if (DRM_UT_KMS)                                           \
			drm_dbg_atomic(dp->drm[0], "MST:" fmt, ##__VA_ARGS__);    \
		else                                                      \
			dev_dbg(dp->dev, "[MST:%s][debug]" fmt, __func__, \
				##__VA_ARGS__);                           \
	} while (0)

#define DP_MST_INFO(fmt, ...) \
	dev_info(dp->dev, "[MST:%s][info]" fmt, __func__, ##__VA_ARGS__)

//------------------------------------------------------------------------------
// Driver interface
//------------------------------------------------------------------------------
void trilin_dp_write(struct trilin_dp *dp, int offset, u32 val);
u32 trilin_dp_read(struct trilin_dp *dp, int offset);
void trilin_phy_write(struct trilin_dp *dp, int offset, u32 val);
u32 trilin_phy_read(struct trilin_dp *dp, int offset);

struct trilin_encoder *encoder_to_trilin(struct drm_encoder *encoder);
struct trilin_connector *connector_to_trilin(struct drm_connector *connector);

int trilin_dp_probe(struct trilin_dpsub *dpsub, struct drm_device *drm);
void trilin_dp_remove(struct trilin_dpsub *dpsub);
int trilin_dp_init_config(struct trilin_dp *dp);
int trilin_dp_handle_connect(struct trilin_dp *dp, bool send_notification);
int trilin_dp_handle_disconnect(struct trilin_dp *dp, bool send_notification);
int trilin_dp_host_init(struct trilin_dp *dp);
int trilin_dp_hdcp_init(struct trilin_dpsub *dpsub);
void trilin_dp_hdcp_uninit(struct trilin_dpsub *dpsub);

bool trilin_dp_get_hpd_state(struct trilin_dp *dp);
int trilin_dp_prepare(struct trilin_dp *dp);
int trilin_dp_enable(struct trilin_dp *dp, struct trilin_dp_panel *dp_panel);
int trilin_dp_post_enable(struct trilin_dp *dp,
			  struct trilin_dp_panel *dp_panel);
int trilin_dp_pre_disable(struct trilin_dp *dp, struct trilin_dp_panel *panel);
int trilin_dp_disable(struct trilin_dp *dp, struct trilin_dp_panel *panel);
int trilin_dp_unprepare(struct trilin_dp *dp);
int trilin_dp_set_stream_info(struct trilin_dp *dp,
			      struct trilin_dp_panel *dp_panel, u32 stream_id,
			      u32 start_slot, u32 num_slots);
int trilin_dp_panel_setup_hdr_sdp(struct trilin_dp *dp,
				  struct trilin_dp_panel *dp_panel);
int trilin_dp_max_rate(int link_rate, u8 lane_num, u8 bpp);
int trilin_dp_mode_configure(struct trilin_dp *dp, int pclock, u8 current_bw,
			     u8 bpp, bool downshift);
void trilin_dp_dump_regs(struct seq_file *m, struct trilin_dp *dp);
void trilin_dp_connector_debugfs_init(struct drm_connector *connector,
				      struct dentry *root);

int trilin_dp_pm_prepare(struct trilin_dp *dp);
int trilin_dp_pm_complete(struct trilin_dp *dp);
int trilin_dp_hpd_config_cb(struct trilin_dp *dp);
int trilin_dp_deinit_config(struct trilin_dp *dp);

void trilin_dp_psr_enable(struct trilin_dp *dp,
	struct trilin_dp_panel *dp_panel);
void trilind_dp_psr_disable(struct trilin_dp *dp,
	struct trilin_dp_panel *dp_panel);
//---------------------------------------------------------
#endif /* _TRILIN_DPTX_H_ */

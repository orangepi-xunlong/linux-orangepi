#ifndef _HDMI_CORE_H_
#define _HDMI_CORE_H_

#include "core/hdmi_includes.h"
#include "bsp/access.h"
#include "hdmitx_dev.h"
#include "edid/edid.h"
#include "util/util.h"
#include "core/audio.h"
#include "core/video.h"
#include "core/packets.h"
#include "core/main_controller.h"
#include "scdc/scdc.h"
#include "identification/identification.h"
#include <linux/hdmi_uapi.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>

#define STRING_ARRAY_SIZE 150

#define IS_EQUAL(buffer, text) \
	((strcmp(buffer, text) == 0) ? 1 : 0)

#define GET(buf, argc) \
	strtoul(buf[argc], NULL, 0)

#define SIZE_ARRAY(array, type) \
	(sizeof(array) / sizeof(type))

#define EDID_DTD_ARRAY_SIZE     (32)
#define EDID_SVD_ARRAY_SIZE     (128)
#define EDID_SAD_ARRAY_SIZE     (128)
#define EDID_MONITOR_NAME_SIZE  (13)

#define HDMI_HOTPLUG    (1)
#define HDMI_NOPLUG     (2)
#define HDMI_EDID_FAIL  (3)

typedef struct _sink_cap_info {
	dtd_t sink_cap_info;
	u32 fresh_rate;
} sink_cap_info;

/**
 * Compliance mode structure
 * This structure contains the important information regarding the expert
 * mode: variables and structures
 */
struct hdmi_compliance_mode {
	int current_mode;
	int re_configure;
	int ycc420_support;
};

/**
 * Demo mode structure
 * This structure contains the important information regarding the expert
 * mode: variables and structures
 */
struct hdmi_demo_mode {
	int current_3D;
	int re_configure;
	int ycc420_support;
	u32	current_mode;
	u32 cea;
	u32 dcm;
	u32 sfr_clk;
};

typedef struct {
	/**
	 * Array to hold all the parsed Detailed Timing Descriptors.
	 */
	dtd_t edid_mdtd[32];

	unsigned int edid_mdtd_index;
	/**
	 * array to hold all the parsed Short Video Descriptors.
	 */
	short_video_desc_t edid_msvd[EDID_SVD_ARRAY_SIZE];
	short_video_desc_t tmp_svd;

	unsigned int edid_msvd_index;
	/**
	 * array to hold all the parsed Short Audio Descriptors.
	 */
	short_audio_desc_t edid_msad[EDID_SAD_ARRAY_SIZE];

	unsigned int edid_msad_index;
	int native_vic;
	/**
	 * A string to hold the Monitor Name parsed from EDID.
	 */
	char edid_mmonitor_name[EDID_MONITOR_NAME_SIZE];
	int edid_mycc444_support;
	int edid_mycc422_support;
	int edid_mycc420_support;
	int edid_mbasic_audio_support;
	int edid_munder_scan_support;
	int xv_ycc709;
	int s_ycc601;
	int xv_ycc601;
	int adobe_ycc601;
	int adobe_rgb;
	/**
	 *  Audio
	 */
	u32 support_sample_rate[10];
	u8  support_bit_depth[10];

	/**
	 *  If Sink is HDMI 2.0
	 */
	int edid_m20sink;

	hdmivsdb_t edid_mhdmivsdb;
	hdmiforumvsdb_t edid_mhdmi_forumvsdb;

	monitor_range_limits_t edid_mmonitor_range_limits;

	video_capability_datablock_t edid_mvideo_capability_datablock;

	colorimetry_datablock_t edid_mcolorimetry_datablock;

	speaker_allocation_datablock_t edid_mspeaker_allocation_datablock;
} sink_edid_t;

struct hdmi_mode {
	bool hdmi_en;

	video_params_t pvideo;
	audio_params_t paudio;
	hdcp_params_t phdcp;
	hdmivsdb_t vsdb;
	hdmiforumvsdb_t forumvsdb;

	u8 ksv_list_buffer[670];
	u8 ksv_devices;
	u8 dpk_aksv[7];
	u8 sw_enc_key[2];
	u8 dpk_keys[560];

	u8 edid_ext[3][128];

	int hpd;
	int edid_done;

	struct edid edid;
	sink_edid_t * sink_cap;
	sink_cap_info sink_capinfo[128];

	struct hdmi_compliance_mode compliance;
	struct hdmi_demo_mode demo;
};

struct hdmi_tx_ctx {
	/** Verbose */
	int verbose;

	/** HDMI TX Driver */
	char hdmi_tx_name[STRING_ARRAY_SIZE];
	/** File descriptor */
	u32 base_address;

	/* Reserved for API internal use only */
	/** HDMI TX API Internals */
	struct device_access dev_access;
	hdmi_tx_dev_t hdmi_tx;

	/** Application mode configurations */
	struct hdmi_mode mode;

	/** Mutex to synchronize calls */
	struct mutex mutex;

	struct edid * tx_edid;
	u8 * tx_edid_ext;
	sink_edid_t * tx_sink_cap;

	int edid_tx_checks;
};

int hdmitx_init(void);

int hdmitx_deinit(void);

int hdmitx_set_phy(int phy, int debug_mode);

int edid_read_cap(void);

int hdmitx_force_get_edid(hdmi_edid* edid_raw, char *fileName);

int sink_capability(hdmi_sink_capability* hdmi_sink_cap);

int hdmitx_set_infoframe(hdmi_infoframe* info_frame);

int hdmitx_get_infoframe(hdmi_infoframe* info_frame);

int hdmitx_start(void);

int hdmitx_stop(void);

int hdmitx_set_attr(hdmi_attr* attr);

int hdmitx_get_attr(hdmi_attr* attr);

int stop_handler(void);

void hpd_handler(void);

void hdcp_handler(void);

void reset_hdcp_params(void);

int hdmitx_set_avmute(int enable);

int hdmitx_set_audio_mute(int enable);

int edid_parser(hdmi_tx_dev_t *dev, u8 * buffer, sink_edid_t *edid_ext, u16 edid_size);

void print_videoinfo(video_params_t *pvideo);

void cvitek_hdmi_clk_set(u32 pclk);

void disp_hdmi_gen(dtd_t *mdtd);

int edid_tx_supports_cea_code(u32 cea_code);

struct hdmi_tx_ctx* get_hdmi_ctx(void);

int send_signal(hdmi_tx_dev_t *dev, int sig);

int get_current_event_id(u32* event_id);

void hdmitx_create_proc(struct hdmitx_dev * dev);

void hdmitx_destroy_proc(struct hdmitx_dev * dev);

int hdmi_proc_edid_cmd(u8 edid, char* file_path);

int hdmi_proc_cbar_cmd(u8 cbar);

int hdmi_proc_mode_cmd(u8 hdmi_mode);

int hdmi_proc_ddc_cmd(u32 ddc_rate);

int hdmi_proc_control_cmd(u8 cmd);

int hdmi_proc_outclrspace_parse(u8 outclrspace);

int hdmi_proc_scdc_cmd(u8 scdc, u32 addr, u32 data);

int hdmi_proc_phy_cmd(u8 phy, u32 addr, u32 data);

#endif /* _HDMI_CORE_H_*/

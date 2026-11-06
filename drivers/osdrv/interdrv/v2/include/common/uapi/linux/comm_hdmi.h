#ifndef _COMM_HDMI_H_
#define _COMM_HDMI_H_
#include <linux/common.h>
#include <linux/comm_video.h>


#define HDMI_VENDOR_USER_DATA_MAX_LEN 22
#define HDMI_MAX_AUDIO_CAPBILITY_CNT 16
#define HDMI_MAX_SAMPLE_RATE_NUM 8
#define HDMI_MAX_BIT_DEPTH_NUM 6
#define HDMI_DETAIL_TIMING_MAX 10
#define HDMI_EDID_RAW_DATA_LEN 512
#define HDMI_HW_PARAM_NUM 4
#define HDMI_MANUFACTURE_NAME_LEN 4
#define HDMI_AUDIO_SPEAKER_BUTT 1

typedef enum _hdmi_video_format {
	HDMI_VIDEO_FORMAT_CEA861_640x480p60     = 1,
	HDMI_VIDEO_FORMAT_CEA861_720x480p60     = 2,
	HDMI_VIDEO_FORMAT_CEA861_1280x720p60    = 4,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080i60   = 5,
	HDMI_VIDEO_FORMAT_CEA861_1440x480i60    = 6,
	HDMI_VIDEO_FORMAT_CEA861_1440x240p60    = 8,
	HDMI_VIDEO_FORMAT_CEA861_2880x480i60    = 10,
	HDMI_VIDEO_FORMAT_CEA861_2880x240p60    = 12,
	HDMI_VIDEO_FORMAT_CEA861_1440x480p60    = 14,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080p60   = 16,
	HDMI_VIDEO_FORMAT_CEA861_720x576p50     = 17,
	HDMI_VIDEO_FORMAT_CEA861_1280x720p50    = 19,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080i50   = 20,
	HDMI_VIDEO_FORMAT_CEA861_1440x576i50    = 21,
	HDMI_VIDEO_FORMAT_CEA861_1440x288p50    = 23,
	HDMI_VIDEO_FORMAT_CEA861_2880x576i50    = 25,
	HDMI_VIDEO_FORMAT_CEA861_2880x288p50    = 27,
	HDMI_VIDEO_FORMAT_CEA861_1440x576p50    = 29,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080p50   = 31,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080p24   = 32,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080p25   = 33,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080p30   = 34,
	HDMI_VIDEO_FORMAT_CEA861_2880x480p60    = 35,
	HDMI_VIDEO_FORMAT_CEA861_2880x576p50    = 37,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080i100  = 40,
	HDMI_VIDEO_FORMAT_CEA861_1280x720p100   = 41,
	HDMI_VIDEO_FORMAT_CEA861_720x576p100    = 42,
	HDMI_VIDEO_FORMAT_CEA861_1440x576i100   = 44,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080i120  = 46,
	HDMI_VIDEO_FORMAT_CEA861_1280x720p120   = 47,
	HDMI_VIDEO_FORMAT_CEA861_720x480p120    = 48,
	HDMI_VIDEO_FORMAT_CEA861_720x480i120    = 50,
	HDMI_VIDEO_FORMAT_CEA861_720x576p200    = 52,
	HDMI_VIDEO_FORMAT_CEA861_720x576i200    = 54,
	HDMI_VIDEO_FORMAT_CEA861_720x480p240    = 56,
	HDMI_VIDEO_FORMAT_CEA861_720x480i240    = 58,
	HDMI_VIDEO_FORMAT_CEA861_1280x720p24    = 60,
	HDMI_VIDEO_FORMAT_CEA861_1280x720p25    = 61,
	HDMI_VIDEO_FORMAT_CEA861_1280x720p30    = 62,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080p120  = 63,
	HDMI_VIDEO_FORMAT_CEA861_1920x1080p100  = 64,
	HDMI_VIDEO_FORMAT_CEA861_1680x720p24    = 79,
	HDMI_VIDEO_FORMAT_CEA861_1680x720p25    = 80,
	HDMI_VIDEO_FORMAT_CEA861_1680x720p30    = 81,
	HDMI_VIDEO_FORMAT_CEA861_1680x720p50    = 82,
	HDMI_VIDEO_FORMAT_CEA861_1680x720p60    = 83,
	HDMI_VIDEO_FORMAT_CEA861_1680x720p100   = 84,
	HDMI_VIDEO_FORMAT_CEA861_1680x720p120   = 85,
	HDMI_VIDEO_FORMAT_CEA861_2560x1080p24   = 86,
	HDMI_VIDEO_FORMAT_CEA861_2560x1080p25   = 87,
	HDMI_VIDEO_FORMAT_CEA861_2560x1080p30   = 88,
	HDMI_VIDEO_FORMAT_CEA861_2560x1080p50   = 89,
	HDMI_VIDEO_FORMAT_CEA861_2560x1080p60   = 90,
	HDMI_VIDEO_FORMAT_CEA861_2560x1080p100  = 91,
	HDMI_VIDEO_FORMAT_CEA861_2560x1080p120  = 92,
	HDMI_VIDEO_FORMAT_CEA861_3840x2160p24   = 93,
	HDMI_VIDEO_FORMAT_CEA861_3840x2160p25   = 94,
	HDMI_VIDEO_FORMAT_CEA861_3840x2160p30   = 95,
	HDMI_VIDEO_FORMAT_CEA861_3840x2160p50   = 96,
	HDMI_VIDEO_FORMAT_CEA861_3840x2160p60   = 97,
	HDMI_VIDEO_FORMAT_CEA861_4096x2160p24   = 98,
	HDMI_VIDEO_FORMAT_CEA861_4096x2160p25   = 99,
	HDMI_VIDEO_FORMAT_CEA861_4096x2160p30   = 100,
	HDMI_VIDEO_FORMAT_CEA861_4096x2160p50   = 101,
	HDMI_VIDEO_FORMAT_CEA861_4096x2160p60   = 102,
	HDMI_VIDEO_FORMAT_CVT_RB_2560X1440p60   = 108,
	HDMI_VIDEO_FORMAT_CUSTOMER_DEFINE,
	HDMI_VIDEO_FORMAT_BUTT,
} hdmi_video_format;


typedef enum _hdmi_errorcode {
	HDMI_ERR_SCDC_FAILED	= -100,
	HDMI_ERR_DEV_POINTER_INVALID,
	HDMI_ERR_ARG_INVALID,
	HDMI_ERR_VIDEO_CONFIG_FAILED,
	HDMI_ERR_VIDEO_ARGS_INVALID,
	HDMI_ERR_COLOR_DEPTH_INVALID,
	HDMI_ERR_COLOR_REMAP_SIZE_INVALID,
	HDMI_ERR_OUTPUT_ENCODETYPE_INVALID,
	HDMI_ERR_INTERPOLATION_FILTER_INVALID,
	HDMI_ERR_DECIMATION_FILTER_INVALID,
	HDMI_ERR_INVALID_INPUT_ENCODETYPE,
	HDMI_ERR_AUDIO_ARGS_INVALID,
	HDMI_ERR_3DSTRUCTURE_NOT_SUPPORT,
	HDMI_ERR_SCRAMBLING_FAILED,
	HDMI_ERR_PHY_NOT_CONFIG,
	HDMI_ERR_PHY_PLL_NOT_LOCK,
	HDMI_ERR_HDCP_NOT_SUPORRT,
	HDMI_ERR_SET_INFOFRAME_FAILED,
	HDMI_ERR_HPD_FAILED,
	HDMI_ERR_NOT_INIT,
	HDMI_ERR_INVALID_PARA,
	HDMI_ERR_NULL_PTR,
	HDMI_ERR_DEV_NOT_OPEN,
	HDMI_ERR_DEV_NOT_CONNECT,
	HDMI_ERR_READ_SINK_FAILED,
	HDMI_ERR_INIT_ALREADY,
	HDMI_ERR_CALLBACK_ALREADY,
	HDMI_ERR_INVALID_CALLBACK,
	HDMI_ERR_FEATURE_NO_SUPPORT,
	HDMI_ERR_READ_EVENT_FAILED,
	HDMI_ERR_NOT_START,
	HDMI_ERR_READ_EDID_FAILED,
	HDMI_ERR_INIT_FAILED,
	HDMI_ERR_CREATE_TESK_FAILED,
	HDMI_ERR_MALLOC_FAILED,
	HDMI_ERR_FREE_FAILED,
	HDMI_ERR_PTHREAD_CREATE_FAILED,
	HDMI_ERR_PTHREAD_JOIN_FAILED,
	HDMI_ERR_STRATEGY_FAILED,
	HDMI_ERR_SET_ATTR_FAILED,
	HDMI_ERR_CALLBACK_NOT_REGISTER,
	HDMI_ERR_UNKNOWN_COMMAND,
	HDMI_ERR_MUTEX_LOCK_FAILED,
} hdmi_errorcode;

typedef enum _hdmi_id {
	HDMI_ID_0 = 0,
	HDMI_ID_1 = 1,
	HDMI_ID_BUTT,
} hdmi_id;

typedef enum _hdmi_event_type {
	HDMI_EVENT_HOTPLUG = 1,
	HDMI_EVENT_NO_PLUG = 2,
	HDMI_EVENT_EDID_FAIL = 3,
	HDMI_EVENT_BUTT,
} hdmi_event_type;

typedef enum _hdmi_video_mode {
	HDMI_VIDEO_MODE_RGB888 = 0,
	HDMI_VIDEO_MODE_YCBCR444,
	HDMI_VIDEO_MODE_YCBCR422,
	HDMI_VIDEO_MODE_YCBCR420,
	HDMI_VIDEO_MODE_BUTT,
} hdmi_video_mode;

typedef enum _hdmi_deep_color {
	HDMI_DEEP_COLOR_24BIT = 24,
	HDMI_DEEP_COLOR_30BIT = 30,
	HDMI_DEEP_COLOR_36BIT = 36,
	HDMI_DEEP_COLOR_BUTT,
} hdmi_deep_color;

typedef void (* hdmi_callback) (hdmi_event_type event, void *private_data);

typedef enum _hdmi_sample_rate {
	HDMI_SAMPLE_RATE_UNKNOWN,
	HDMI_SAMPLE_RATE_32K = 32000,
	HDMI_SAMPLE_RATE_44K = 44100,
	HDMI_SAMPLE_RATE_48K = 48000,
	HDMI_SAMPLE_RATE_88K = 88000,
	HDMI_SAMPLE_RATE_96K = 96000,
	HDMI_SAMPLE_RATE_176K = 176400,
	HDMI_SAMPLE_RATE_192K = 192000,
	HDMI_SAMPLE_RATE_BUTT,
} hdmi_sample_rate;

typedef enum _hdmi_bit_depth {
	HDMI_BIT_DEPTH_UNKNOWN,
	HDMI_BIT_DEPTH_16 = 16,
	HDMI_BIT_DEPTH_24 = 24,
	HDMI_BIT_DEPTH_BUTT,
} hdmi_bit_depth;

typedef enum _hdmi_audio_format_code {
	HDMI_AUDIO_FORMAT_CODE_RESERVED,
	HDMI_AUDIO_FORMAT_CODE_PCM,
	HDMI_AUDIO_FORMAT_CODE_AC3,
	HDMI_AUDIO_FORMAT_CODE_MPEG1,
	HDMI_AUDIO_FORMAT_CODE_MP3,
	HDMI_AUDIO_FORMAT_CODE_MPEG2,
	HDMI_AUDIO_FORMAT_CODE_AAC,
	HDMI_AUDIO_FORMAT_CODE_DTS,
	HDMI_AUDIO_FORMAT_CODE_ATRAC,
	HDMI_AUDIO_FORMAT_CODE_ONE_BIT,
	HDMI_AUDIO_FORMAT_CODE_DDP,
	HDMI_AUDIO_FORMAT_CODE_DTS_HD,
	HDMI_AUDIO_FORMAT_CODE_MAT,
	HDMI_AUDIO_FORMAT_CODE_DST,
	HDMI_AUDIO_FORMAT_CODE_WMA_PRO,
	HDMI_AUDIO_FORMAT_CODE_BUTT,
} hdmi_audio_format_code;

typedef enum _hdmi_color_space {
	HDMI_COLOR_SPACE_RGB888,
	HDMI_COLOR_SPACE_YCBCR422,
	HDMI_COLOR_SPACE_YCBCR444,
	HDMI_COLOR_SPACE_YCBCR420,
	HDMI_COLOR_SPACE_BUTT,
} hdmi_color_space;

typedef enum _hdmi_bar_info {
	HDMI_BAR_INFO_NVALID,
	HDMI_BAR_INFO_V,
	HDMI_BAR_INFO_H,
	HDMI_BAR_INFO_VH,
	HDMI_BAR_INFO_BUTT,
} hdmi_bar_info;

typedef enum _hdmi_scan_info {
	HDMI_SCAN_INFO_NO_DATA,
	HDMI_SCAN_INFO_OVERSCANNED,
	HDMI_SCAN_INFO_UNDERSCANNED,
	HDMI_SCAN_INFO_BUTT,
} hdmi_scan_info;

typedef enum _hdmi_colorimetry {
	HDMI_COMMON_COLORIMETRY_NO_DATA,
	HDMI_COMMON_COLORIMETRY_ITU601,
	HDMI_COMMON_COLORIMETRY_ITU709,
	HDMI_COMMON_COLORIMETRY_BUTT,
} hdmi_colorimetry;

typedef enum _hdmi_ex_colorimetry {
	HDMI_COMMON_COLORIMETRY_XVYCC_601,
	HDMI_COMMON_COLORIMETRY_XVYCC_709,
	HDMI_COMMON_COLORIMETRY_S_YCC_601,
	HDMI_COMMON_COLORIMETRY_ADOBE_YCC_601,
	HDMI_COMMON_COLORIMETRY_ADOBE_RGB,
	HDMI_COMMON_COLORIMETRY_2020_CONST_LUMINOUS,
	HDMI_COMMON_COLORIMETRY_2020_NON_CONST_LUMINOUS,
	HDMI_COMMON_COLORIMETRY_EX_BUTT,
} hdmi_ex_colorimetry;

typedef enum _hdmi_pic_aspect_ratio {
	HDMI_PIC_ASPECT_RATIO_NO_DATA,
	HDMI_PIC_ASPECT_RATIO_4TO3,
	HDMI_PIC_ASPECT_RATIO_16TO9,
	HDMI_PIC_ASPECT_RATIO_64TO27,
	HDMI_PIC_ASPECT_RATIO_256TO135,
	HDMI_PIC_ASPECT_BUTT,
} hdmi_pic_aspect_ratio;

typedef enum _hdmi_active_aspect_ratio {
	HDMI_ACTIVE_ASPECT_RATIO_16TO9_TOP = 2,
	HDMI_ACTIVE_ASPECT_RATIO_14TO9_TOP,
	HDMI_ACTIVE_ASPECT_RATIO_16TO9_BOX_CENTER,
	HDMI_ACTIVE_ASPECT_RATIO_SAME_PIC = 8,
	HDMI_ACTIVE_ASPECT_RATIO_4TO3_CENTER,
	HDMI_ACTIVE_ASPECT_RATIO_16TO9_CENTER,
	HDMI_ACTIVE_ASPECT_RATIO_14TO9_CENTER,
	HDMI_ACTIVE_ASPECT_RATIO_4TO3_14_9 = 13,
	HDMI_ACTIVE_ASPECT_RATIO_16TO9_14_9,
	HDMI_ACTIVE_ASPECT_RATIO_16TO9_4_3,
	HDMI_ACTIVE_ASPECT_RATIO_BUTT,
} hdmi_active_aspect_ratio;

typedef enum _hdmi_pic_scaline {
	HDMI_PICTURE_NON_UNIFORM_SCALING,
	HDMI_PICTURE_SCALING_H,
	HDMI_PICTURE_SCALING_V,
	HDMI_PICTURE_SCALING_HV,
	HDMI_PICTURE_SCALING_BUTT,
} hdmi_pic_scaline;

typedef enum _hdmi_rgb_quant_range {
	HDMI_RGB_QUANT_DEFAULT_RANGE,
	HDMI_RGB_QUANT_LIMITED_RANGE,
	HDMI_RGB_QUANT_FULL_RANGE,
	HDMI_RGB_QUANT_FULL_BUTT,
} hdmi_rgb_quant_range;

typedef enum _hdmi_pixel_repettion {
	HDMI_PIXEL_REPET_NO,
	HDMI_PIXEL_REPET_2_TIMES,
	HDMI_PIXEL_REPET_3_TIMES,
	HDMI_PIXEL_REPET_4_TIMES,
	HDMI_PIXEL_REPET_5_TIMES,
	HDMI_PIXEL_REPET_6_TIMES,
	HDMI_PIXEL_REPET_7_TIMES,
	HDMI_PIXEL_REPET_8_TIMES,
	HDMI_PIXEL_REPET_9_TIMES,
	HDMI_PIXEL_REPET_10_TIMES,
	HDMI_PIXEL_REPET_BUTT,
} hdmi_pixel_repettion;

typedef enum _hdmi_content_type {
	HDMI_CONTNET_GRAPHIC,
	HDMI_CONTNET_PHOTO,
	HDMI_CONTNET_CINEMA,
	HDMI_CONTNET_GAME,
	HDMI_CONTNET_BUTT,
} hdmi_content_type;

typedef enum _hdmi_ycc_quant_range {
	HDMI_YCC_QUANT_LIMITED_RANGE,
	HDMI_YCC_QUANT_FULL_RANGE,
	HDMI_YCC_QUANT_BUTT,
} hdmi_ycc_quant_range;

typedef enum _hdmi_audio_chn_cnt {
	HDMI_AUDIO_CHANEL_CNT_STREAM,
	HDMI_AUDIO_CHANEL_CNT_2,
	HDMI_AUDIO_CHANEL_CNT_3,
	HDMI_AUDIO_CHANEL_CNT_4,
	HDMI_AUDIO_CHANEL_CNT_5,
	HDMI_AUDIO_CHANEL_CNT_6,
	HDMI_AUDIO_CHANEL_CNT_7,
	HDMI_AUDIO_CHANEL_CNT_8,
	HDMI_AUDIO_CHANEL_BUTT,
} hdmi_audio_chn_cnt;

typedef enum _hdmi_coding_type {
	HDMI_AUDIO_CODING_REFER_STREAM_HEAD,
	HDMI_AUDIO_CODING_PCM,
	HDMI_AUDIO_CODING_AC3,
	HDMI_AUDIO_CODING_MPEG1,
	HDMI_AUDIO_CODING_MP3,
	HDMI_AUDIO_CODING_MPEG2,
	HDMI_AUDIO_CODING_AACLC,
	HDMI_AUDIO_CODING_DTS,
	HDMI_AUDIO_CODING_ATRAC,
	HDMI_AUDIO_CODIND_ONE_BIT_AUDIO,
	HDMI_AUDIO_CODING_ENAHNCED_AC3,
	HDMI_AUDIO_CODING_DTS_HD,
	HDMI_AUDIO_CODING_MAT,
	HDMI_AUDIO_CODING_DST,
	HDMI_AUDIO_CODING_WMA_PRO,
	HDMI_AUDIO_CODING_BUTT,
} hdmi_coding_type;

typedef enum _hdmi_audio_sample_size {
	HDMI_AUDIO_SAMPLE_SIZE_STREAM,
	HDMI_AUDIO_SAMPLE_SIZE_16 = 16,
	HDMI_AUDIO_SAMPLE_SIZE_20 = 20,
	HDMI_AUDIO_SAMPLE_SIZE_24 = 24,
	HDMI_AUDIO_SAMPLE_SIZE_BUTT,
} hdmi_audio_sample_size;

typedef enum _hdmi_audio_sample_freq {
	HDMI_AUDIO_SAMPLE_FREQ_STREAM,
	HDMI_AUDIO_SAMPLE_FREQ_32000 = 32000,
	HDMI_AUDIO_SAMPLE_FREQ_44100 = 44100,
	HDMI_AUDIO_SAMPLE_FREQ_48000 = 48000,
	HDMI_AUDIO_SAMPLE_FREQ_88200 = 88200,
	HDMI_AUDIO_SAMPLE_FREQ_96000 = 96000,
	HDMI_AUDIO_SAMPLE_FREQ_176400 = 176400,
	HDMI_AUDIO_SAMPLE_FREQ_192000 = 192000,
	HDMI_AUDIO_SAMPLE_FREQ_BUTT,
} hdmi_audio_sample_freq;

typedef enum _hdmi_level_shift_val {
	HDMI_LEVEL_SHIFT_VALUE_0_DB,
	HDMI_LEVEL_SHIFT_VALUE_1_DB,
	HDMI_LEVEL_SHIFT_VALUE_2_DB,
	HDMI_LEVEL_SHIFT_VALUE_3_DB,
	HDMI_LEVEL_SHIFT_VALUE_4_DB,
	HDMI_LEVEL_SHIFT_VALUE_5_DB,
	HDMI_LEVEL_SHIFT_VALUE_6_DB,
	HDMI_LEVEL_SHIFT_VALUE_7_DB,
	HDMI_LEVEL_SHIFT_VALUE_8_DB,
	HDMI_LEVEL_SHIFT_VALUE_9_DB,
	HDMI_LEVEL_SHIFT_VALUE_10_DB,
	HDMI_LEVEL_SHIFT_VALUE_11_DB,
	HDMI_LEVEL_SHIFT_VALUE_12_DB,
	HDMI_LEVEL_SHIFT_VALUE_13_DB,
	HDMI_LEVEL_SHIFT_VALUE_14_DB,
	HDMI_LEVEL_SHIFT_VALUE_15_DB,
	HDMI_LEVEL_SHIFT_VALUE_BUTT,
} hdmi_level_shift_val;

typedef enum _hdmi_lfe_playback_level {
	HDMI_LFE_PLAYBACK_NO,
	HDMI_LFE_PLAYBACK_0_DB,
	HDMI_LFE_PLAYBACK_10_DB,
	HDMI_LFE_PLAYBACK_BUTT,
} hdmi_lfe_playback_level;

typedef enum _hdmi_infoframe_type {
	INFOFRAME_TYPE_AVI,
	INFOFRAME_TYPE_AUDIO,
	INFOFRAME_TYPE_VENDORSPEC,
	INFOFRAME_TYPE_BUTT,
} hdmi_infoframe_type;

typedef enum _hdmi_force_action {
	HDMI_FORCE_NULL,
	HDMI_FORCE_HDMI,
} hdmi_force_action;

typedef struct _hdmi_callback_func{
	hdmi_callback hdmi_event_callback;
	void *private_data;
} hdmi_callback_func;

typedef struct _hdmi_attr {
	unsigned char hdmi_en;
	hdmi_video_format video_format;
	hdmi_deep_color deep_color_mode;
	unsigned char audio_en;
	unsigned char hdcp14_en;
	hdmi_force_action hdmi_force_output;
	hdmi_video_mode hdmi_video_input;
	hdmi_video_mode hdmi_video_output;
	hdmi_sample_rate sample_rate;
	hdmi_bit_depth bit_depth;
	unsigned char auth_mode_en;
	uint64_t audio_start_paddr;
	uint64_t audio_stop_paddr;
	unsigned char deep_color_adapt_en;
	unsigned int pix_clk;
} hdmi_attr;

typedef struct _hdmi_audio_info {
	hdmi_audio_format_code audio_format_code;
	hdmi_sample_rate support_sample_rate[HDMI_MAX_SAMPLE_RATE_NUM];
	unsigned int support_sample_rate_num;
	unsigned char audio_chn;
	hdmi_bit_depth support_bit_depth[HDMI_MAX_BIT_DEPTH_NUM];
	unsigned int support_bit_depth_num;
	unsigned int max_bit_rate;
} hdmi_audio_info;

typedef struct _hdmi_timing_info {
	unsigned int vfb;
	unsigned int vbb;
	unsigned int vact;
	unsigned int hfb;
	unsigned int hbb;
	unsigned int hact;
	unsigned int vpw;
	unsigned int hpw;
	unsigned char idv;
	unsigned char ihs;
	unsigned char ivs;
	unsigned int img_width;
	unsigned int img_height;
	unsigned int aspect_ratio_w;
	unsigned int aspect_ratio_h;
	unsigned char interlace;
	unsigned int pixel_clk;
} hdmi_timing_info;

typedef struct _hdmi_detail_timing {
	unsigned int detail_timing_num;
	hdmi_timing_info detail_timing[HDMI_DETAIL_TIMING_MAX];
} hdmi_detail_timing;

typedef struct _hdmi_video_info {
	hdmi_video_format mcode;
	hdmi_timing_info timing_info;
	unsigned int fresh_rate;
} hdmi_video_info;

typedef struct _hdmi_sink_capability {
	unsigned char is_connected;
	unsigned char support_hdmi;
	unsigned char is_sink_power_on;
	hdmi_video_format native_video_format;
	hdmi_video_info support_video_format[HDMI_VIDEO_FORMAT_BUTT];
	unsigned char support_ycbcr;
	unsigned char support_xvycc601;
	unsigned char support_xvycc709;
	unsigned char md_bit;
	unsigned int audio_info_num;
	unsigned char hdcp14_en;
	hdmi_video_mode hdmi_video_input;
	hdmi_video_mode hdmi_video_output;
	hdmi_audio_info audio_info[HDMI_MAX_AUDIO_CAPBILITY_CNT];
	unsigned char speaker[HDMI_AUDIO_SPEAKER_BUTT];
	unsigned char manufacture_name[HDMI_MANUFACTURE_NAME_LEN];
	unsigned int pdt_code;
	unsigned int serial_num;
	unsigned int week_of_manufacture;
	unsigned int year_of_manufacture;
	unsigned char version;
	unsigned char revision;
	unsigned char edid_ex_blk_num;
	unsigned char is_phy_addr_valid;
	unsigned char phys_addr_a;
	unsigned char phys_addr_b;
	unsigned char phys_addr_c;
	unsigned char phys_addr_d;
	unsigned char support_dvi_dual;
	unsigned char support_deepcolor_ycbcr444;
	unsigned char support_deep_color_30bit;
	unsigned char support_deep_color_36bit;
	unsigned char support_deep_color_48bit;
	unsigned char support_ai;
	unsigned int max_tmds_clk;
	unsigned char i_latency_fields_present;
	unsigned char latency_fields_present;
	unsigned char hdmi_video_present;
	unsigned char video_latency;
	unsigned char audio_latency;
	unsigned char interlaced_video_latency;
	unsigned char interlaced_audio_latency;
	unsigned char support_y420_dc_30bit;
	unsigned char support_y420_dc_36bit;
	unsigned char support_y420_dc_48bit;
	unsigned char support_hdmi_2_0;
	unsigned char support_y420_format[HDMI_VIDEO_FORMAT_BUTT];
	unsigned char only_support_y420_format[HDMI_VIDEO_FORMAT_BUTT];
	unsigned char ycc_quant_selectable;
	unsigned char rgb_quant_selectable;
	hdmi_detail_timing detailed_timing;
} hdmi_sink_capability;

typedef struct _hdmi_edid{
	unsigned char edid_valid;
	unsigned int edid_len;
	unsigned char edid[256];
} hdmi_edid;

typedef struct _hdmi_avi_infoframe {
	hdmi_video_format timing_mode;
	hdmi_color_space color_space;
	unsigned char active_info_present;
	hdmi_bar_info bar_info;
	hdmi_scan_info scan_info;
	hdmi_colorimetry colorimetry;
	hdmi_ex_colorimetry ex_colorimetry;
	hdmi_pic_aspect_ratio aspect_ratio;
	hdmi_active_aspect_ratio active_aspect_ratio;
	hdmi_pic_scaline pic_scaling;
	hdmi_rgb_quant_range rgb_quant;
	unsigned char is_it_content;
	hdmi_pixel_repettion pixel_repetition;
	hdmi_content_type content_type;
	hdmi_ycc_quant_range ycc_quant;
	unsigned short line_n_end_of_top_bar;
	unsigned short line_n_start_of_bcvi_bar;
	unsigned short pixel_n_end_of_left_bar;
	unsigned short pixel_n_start_of_right_bar;
} hdmi_avi_infoframe;

typedef struct _hdmi_audio_infoframe {
	hdmi_audio_chn_cnt chn_cnt;
	hdmi_coding_type coding_type;
	hdmi_audio_sample_size sample_size;
	hdmi_audio_sample_freq sampling_freq;
	unsigned char chn_alloc;
	hdmi_level_shift_val level_shift;
	hdmi_lfe_playback_level lfe_playback_level;
	unsigned char down_mix_inhibit;
} hdmi_audio_infoframe;

typedef struct _hdmi_vendorspec_infoframe {
	unsigned char data_len;
	unsigned char user_data[HDMI_VENDOR_USER_DATA_MAX_LEN];
} hdmi_vendorspec_infoframe;

typedef struct _hdmi_infoframe_unit {
	hdmi_avi_infoframe avi_infoframe;
	hdmi_audio_infoframe audio_infoframe;
	hdmi_vendorspec_infoframe vendor_spec_infoframe;
} hdmi_infoframe_unit;

typedef struct _hdmi_infoframe {
	hdmi_infoframe_type infoframe_type;
	hdmi_infoframe_unit infoframe_unit;
} hdmi_infoframe;

typedef struct _hdmi_hw_param {
	unsigned int i_de_main_clk;
	unsigned int i_de_main_data;
	unsigned int i_main_clk;
	unsigned int i_main_data;
	unsigned int ft_cap_clk;
	unsigned int ft_cap_data;
} hdmi_hw_param;

typedef struct _hdmi_hw_spec {
	hdmi_hw_param hw_param[HDMI_HW_PARAM_NUM];
} hdmi_hw_spec;

#endif

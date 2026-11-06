#ifndef __U_VO_UAPI_H__
#define __U_VO_UAPI_H__

#include <linux/comm_vo.h>
#ifdef __cplusplus
	extern "C" {
#endif

#define VO_IOC_MAGIC	'o'
#define VO_IOC_BASE	0x20

#define VO_IOC_G_CTRL		_IOWR(VO_IOC_MAGIC, VO_IOC_BASE, struct vo_ext_control)
#define VO_IOC_S_CTRL		_IOWR(VO_IOC_MAGIC, VO_IOC_BASE + 1, struct vo_ext_control)

enum vo_ioctl_cmd {
	//VO_IOC_S_CTRL
	VO_IOCTL_BASE = 0,
	VO_IOCTL_VB_DONE,
	VO_IOCTL_INTR,
	VO_IOCTL_OUT_CSC,
	VO_IOCTL_PATTERN,
	VO_IOCTL_FRAME_BGCOLOR,
	VO_IOCTL_WINDOW_BGCOLOR,
	VO_IOCTL_ONLINE,
	VO_IOCTL_INTF,
	VO_IOCTL_ENABLE_WIN_BGCOLOR,
	VO_IOCTL_SET_ALIGN = 10,
	VO_IOCTL_SET_RGN,
	VO_IOCTL_SET_CUSTOM_CSC,
	VO_IOCTL_SET_CLK,
	VO_IOCTL_GAMMA_LUT_UPDATE,

	//VO_IOC_G_CTRL
	VO_IOCTL_GET_VLAYER_SIZE,
	VO_IOCTL_GET_INTF_TYPE,
	VO_IOCTL_GAMMA_LUT_READ = 20,

	//VO_IOC_S_SELECTION
	VO_IOCTL_SEL_TGT_COMPOSE,
	VO_IOCTL_SEL_TGT_CROP,

	//VO_IOC_S_DV_TIMINGS
	VO_IOCTL_SET_DV_TIMINGS,
	VO_IOCTL_GET_DV_TIMINGS,
	VO_IOCTL_SET_FMT,

	VO_IOCTL_START_STREAMING,
	VO_IOCTL_STOP_STREAMING,
	VO_IOCTL_ENQ_WAITQ,
	VO_IOCTL_SDK_CTRL,
	VO_IOCTL_MAX,
};

enum vo_sdk_ctrl {
	//DEV CTRL
	VO_SDK_SET_PUBATTR,
	VO_SDK_GET_PUBATTR,
	VO_SDK_SET_HDMIPARAM,
	VO_SDK_GET_HDMIPARAM,
	VO_SDK_SET_LVDSPARAM,
	VO_SDK_GET_LVDSPARAM,
	VO_SDK_ENABLE,
	VO_SDK_DISABLE,
	VO_SDK_SUSPEND,
	VO_SDK_RESUME,
	VO_SDK_ISENABLE,
	VO_SDK_GET_PANELSTATUE,
	//LAYER CTRL
	VO_SDK_SET_VIDEOLAYERATTR,
	VO_SDK_GET_VIDEOLAYERATTR,
	VO_SDK_ENABLE_VIDEOLAYER,
	VO_SDK_DISABLE_VIDEOLAYER,
	VO_SDK_SET_DISPLAYBUFLEN,
	VO_SDK_GET_DISPLAYBUFLEN,
	VO_SDK_GET_SCREENFRAME,
	VO_SDK_RELEASE_SCREENFRAME,
	VO_SDK_SET_LAYER_PROC_AMP,
	VO_SDK_GET_LAYER_PROC_AMP,
	VO_SDK_SET_LAYERCSC,
	VO_SDK_GET_LAYERCSC,
	VO_SDK_SET_LAYERTOLERATION,
	VO_SDK_GET_LAYERTOLERATION,
	VO_SDK_SET_LAYERPRRIORITY,
	VO_SDK_GET_LAYERPRRIORITY,
	VO_SDK_BIND_LAYER,
	VO_SDK_UNBIND_LAYER,
	//CHN CTRL
	VO_SDK_SET_CHNATTR,
	VO_SDK_GET_CHNATTR,
	VO_SDK_ENABLE_CHN,
	VO_SDK_DISABLE_CHN,
	VO_SDK_SET_CHNPARAM,
	VO_SDK_GET_CHNPARAM,
	VO_SDK_SET_CHNZOOM,
	VO_SDK_GET_CHNZOOM,
	VO_SDK_SET_CHNBORDER,
	VO_SDK_GET_CHNBORDER,
	VO_SDK_SET_CHNMIRROR,
	VO_SDK_GET_CHNMIRROR,
	VO_SDK_SET_CHNROTATION,
	VO_SDK_GET_CHNROTATION,
	VO_SDK_GET_CHNFRAME,
	VO_SDK_RELEASE_CHNFRAME,
	VO_SDK_SET_CHNFRAMERATE,
	VO_SDK_GET_CHNFRAMERATE,
	VO_SDK_GET_CHNPTS,
	VO_SDK_GET_CHNSTATUS,
	VO_SDK_SET_CHNTHRESHOLD,
	VO_SDK_GET_CHNTHRESHOLD,
	VO_SDK_SHOW_CHN,
	VO_SDK_HIDE_CHN,
	VO_SDK_RESUME_CHN,
	VO_SDK_PAUSE_CHN,
	VO_SDK_STEP_CHN,
	VO_SDK_REFRESH_CHN,
	VO_SDK_SEND_FRAME,
	VO_SDK_CLEAR_CHNBUF,
	//WBC CTRL
	VO_SDK_SET_WBCSRC,
	VO_SDK_GET_WBCSRC,
	VO_SDK_ENABLE_WBC,
	VO_SDK_DISABLE_WBC,
	VO_SDK_SET_WBCATTR,
	VO_SDK_GET_WBCATTR,
	VO_SDK_SET_WBCMODE,
	VO_SDK_GET_WBCMODE,
	VO_SDK_SET_WBCDEPTH,
	VO_SDK_GET_WBCDEPTH,
	VO_SDK_GET_WBCFRAME,
	VO_SDK_RELEASE_WBCFRAME,
	VO_SDK_SET_BTPARAM,
	VO_SDK_GET_BTPARAM,
};

struct vo_ext_control {
	u32 id;
	u32 sdk_id;
	u32 size;
	u32 reserved[1];
	union {
		s32 value;
		s64 value64;
		void *ptr;
	};
};

struct vo_capability {
	u8 driver[16];
	u8 card[32];
	u8 bus_info[32];
	u32 version;
	u32 capabilities;
	u32 vdevice_caps;
	u32 reserved[3];
};

struct vo_bt_timings {
	u32 width;
	u32 height;
	u32 interlaced;
	u32 polarities;
	u64 pixelclock;
	u32 hfrontporch;
	u32 hsync;
	u32 hbackporch;
	u32 vfrontporch;
	u32 vsync;
	u32 vbackporch;
	u32 il_vfrontporch;
	u32 il_vsync;
	u32 il_vbackporch;
	u32 standards;
	u32 flags;
	u32 reservedd[14];
};

struct vo_dv_timings {
	u32 type;
	struct vo_bt_timings bt;
};

struct vo_rect {
	u32 left;
	u32 top;
	u32 width;
	u32 height;
};

//vo sdk layer config
struct vo_video_layer_cfg {
	u8 layer;
};

struct vo_video_layer_bind_cfg {
	u8 dev;
	u8 layer;
};

struct vo_video_layer_attr_cfg {
	u8 layer;
	vo_video_layer_attr_s layer_attr;
};

struct vo_layer_proc_amp_cfg {
	u8 layer;
	s32 proc_amp[PROC_AMP_MAX];
};

struct vo_layer_csc_cfg {
	u8 layer;
	vo_csc_s video_csc;
};

struct vo_layer_toleration_cfg {
	u8 layer;
	u32 toleration;
};

struct vo_layer_priority_cfg {
	u8 layer;
	u32 priority;
};

struct vo_clear_chn_buf_cfg {
	u8 layer;
	u8 chn;
	bool clear;
};

struct vo_snd_frm_cfg {
	u8 layer;
	u8 chn;
	video_frame_info_s video_frame;
	s32 millisec;
};

struct vo_display_buflen_cfg {
	u8 layer;
	u32 buflen;
};

struct vo_chn_attr_cfg {
	u8 layer;
	u8 chn;
	vo_chn_attr_s chn_attr;
};

struct vo_chn_param_cfg {
	u8 layer;
	u8 chn;
	vo_chn_param_s chn_param;
};

struct vo_chn_zoom_cfg {
	u8 layer;
	u8 chn;
	vo_chn_zoom_attr_s chn_zoom_attr;
};

struct vo_chn_border_cfg {
	u8 layer;
	u8 chn;
	vo_chn_border_attr_s chn_border_attr;
};

struct vo_chn_mirror_cfg {
	u8 layer;
	u8 chn;
	vo_chn_mirror_type_e chn_mirror;
};

struct vo_chn_cfg {
	u8 layer;
	u8 chn;
};

struct vo_chn_frame_cfg {
	u8 layer;
	u8 chn;
	video_frame_info_s video_frame;
	s32 millisec;
};

struct vo_chn_frmrate_cfg {
	u8 layer;
	u8 chn;
	u32 frame_rate;
};

struct vo_chn_pts_cfg {
	u8 layer;
	u8 chn;
	u64 chn_pts;
};

struct vo_chn_status_cfg {
	u8 layer;
	u8 chn;
	vo_query_status_s status;
};

struct vo_chn_threshold_cfg {
	u8 layer;
	u8 chn;
	u32 threshold;
};

struct vo_chn_rotation_cfg {
	u8 layer;
	u8 chn;
	rotation_e rotation;
};

struct vo_panel_status_cfg {
	u8 layer;
	u8 chn;
	u32 is_init;
};

struct vo_dev_cfg {
	u8 dev;
	u8 enable;
};

struct vo_pub_attr_cfg {
	u8 dev;
	vo_pub_attr_s pub_attr;
};

struct vo_lvds_param_cfg {
	u8 dev;
	vo_lvds_attr_s lvds_param;
};

struct vo_bt_param_cfg {
	u8 dev;
	vo_bt_attr_s bt_param;
};

struct vo_hdmi_param_cfg {
	u8 dev;
	vo_hdmi_param_s hdmi_param;
};

struct vo_screen_frame {
	u8 layer;
	video_frame_info_s video_frame;
	s32 millisec;
};

struct vo_wbc_src_cfg {
	u8 wbc_dev;
	vo_wbc_src_s wbc_src;
};

struct vo_wbc_cfg {
	u8 wbc_dev;
};

struct vo_wbc_attr_cfg {
	u8 wbc_dev;
	vo_wbc_attr_s wbc_attr;
};

struct vo_wbc_mode_cfg {
	u8 wbc_dev;
	vo_wbc_mode_e wbc_mode;
};

struct vo_wbc_depth_cfg {
	u8 wbc_dev;
	u32 depth;
};

struct vo_wbc_frame_cfg {
	u8 wbc_dev;
	video_frame_info_s video_frame;
	s32 millisec;
};

#ifdef __cplusplus
	}
#endif

#endif /* __U_VO_UAPI_H__ */


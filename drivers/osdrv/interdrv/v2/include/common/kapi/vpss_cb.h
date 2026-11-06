#ifndef __VPSS_CB_H__
#define __VPSS_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <base_ctx.h>
#include <ldc_cb.h>

#define VPSS_STITCH_MAX_CHN 64

struct sc_cfg_cb {
	u8  snr_num;
	u8  is_tile;
	u8  is_left_tile;
	struct vip_line l_in;
	struct vip_line l_out;
	struct vip_line r_in;
	struct vip_line r_out;
	struct mlv_i_s m_lv_i;
	struct timespec64 ts;
};

struct sc_err_handle_cb {
	u8  snr_num;
};

struct vpss_grp_sbm_cfg {
	u16 grp;
	u8 sb_mode;
	u8 sb_size;
	u8 sb_nb;
	u32 ion_size;
	u64 ion_paddr;
};

struct vpss_vc_sbm_flow_cfg {
	u16 vpss_grp;
	u8 vpss_chn;
	u8 ready;
};

struct vpss_grp_mlv_info {
	u16 vpss_grp;
	struct mlv_i_s m_lv_i;
};

struct vpss_window {
	struct vip_rect rect_out; /*outside rectangle*/
	struct vip_rect rect_in; /*inside rectangle*/
	__u32 top_width;
	__u32 bottom_width;
	__u32 left_width;
	__u32 right_width;
	__u8 border_color[3]; /*border color RGB888*/
	__u8 bgcolor[3]; /*background color RGB888*/
	__u8 flip; /*0:normal 1:mirror 2:flip 3:mirror+flip*/
};

struct stitch_chn_cfg {
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[3];
	struct vip_frmsize src_size;
	struct vip_rect rect_crop;
	struct vpss_window window;
};

struct stitch_dst_cfg {
	__u8 color[3]; //background color RGB888
	__u32 pixelformat;
	__u32 bytesperline[2];
	__u64 addr[3];
	struct vip_frmsize dst_size;
};

typedef void (*stitch_job_cb)(void *data);

struct vpss_stitch_cfg {
	__u8 num;
	struct stitch_dst_cfg dst_cfg;
	struct stitch_chn_cfg *chn_cfg;
	stitch_job_cb job_cb;
	void *data;
};

enum vpss_cb_cmd {
	VPSS_CB_VI_ONLINE_TRIGGER,
	VPSS_CB_SET_VIVPSSMODE,
	VPSS_CB_GET_RGN_HDLS,
	VPSS_CB_SET_RGN_HDLS,
	VPSS_CB_SET_RGN_CFG,
	VPSS_CB_SET_RGNEX_CFG,
	VPSS_CB_SET_COVEREX_CFG,
	VPSS_CB_SET_MOSAIC_CFG,
	VPSS_CB_SET_RGN_LUT_CFG,
	VPSS_CB_GET_RGN_OW_ADDR,
	VPSS_CB_GET_CHN_SIZE,
	VPSS_CB_ONLINE_ERR_HANDLE,
	VPSS_CB_SET_LDC_2_VPSS_SBM,
	VPSS_CB_VC_SET_SBM_FLOW,
	VPSS_CB_GET_MLV_INFO,
	VPSS_CB_STITCH,
	VPSS_CB_GDC_OP_DONE = LDC_CB_GDC_OP_DONE,
	VPSS_CB_OVERFLOW_CHECK,
	VPSS_CB_MAX
};

#ifdef __cplusplus
}
#endif

#endif /* __VPSS_CB_H__ */

#ifndef __U_VPSS_CTX_H__
#define __U_VPSS_CTX_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/defines.h>
#include <linux/comm_vpss.h>
#include <linux/comm_gdc.h>
#include <linux/comm_region.h>
#include <base_ctx.h>
#include "base_ctx.h"
#include "vpss_hal.h"

#define VPSS_ONLINE_JOB_NUM 2

enum handler_state {
	HANDLER_STATE_STOP = 0,
	HANDLER_STATE_RUN,
	HANDLER_STATE_SUSPEND,
	HANDLER_STATE_RESUME,
	HANDLER_STATE_MAX,
};

struct vpss_grp_work_status_s {
	u32 recv_cnt;
	u32 frc_recv_cnt;
	u32 lost_cnt;
	u32 start_fail_cnt; //start job fail cnt
	u32 cost_time; // current job cost time in us
	u32 max_cost_time;
	u32 hw_cost_time; // current job Hw cost time in us
	u32 hw_max_cost_time;
};

struct vpss_chn_work_status_s {
	u32 send_ok; // send OK cnt after latest chn enable
	u64 prev_time; // latest time (us)
	u32 frame_num;  //The number of Frame in one second
	u32 real_frame_rate; // chn real time frame rate
	u32 pool_id;
};

struct vpss_chn_cfg {
	u8 is_enabled;
	u8 is_muted;
	u8 is_drop;
	vpss_chn_attr_s chn_attr;
	vpss_crop_info_s crop_info;
	rotation_e rotation;
	u32 blk_size;
	u32 align;
	rgn_handle rgn_handle[RGN_MAX_LAYER_VPSS][RGN_MAX_NUM_VPSS]; //only overlay
	rgn_handle cover_ex_handle[RGN_COVEREX_MAX_NUM];
	rgn_handle mosaic_handle[RGN_MOSAIC_MAX_NUM];
	struct rgn_cfg rgn_cfg[RGN_MAX_LAYER_VPSS];
	struct rgn_coverex_cfg rgn_coverex_cfg;
	struct rgn_mosaic_cfg rgn_mosaic_cfg;
	vpss_scale_coef_e coef;
	vpss_draw_rect_s draw_rect;
	vpss_convert_s convert;
	u32 y_ratio;
	vpss_ldc_attr_s ldc_attr;
	fisheye_attr_s fisheye_attr;
	u32 vb_pool;
	vpss_chn_buf_wrap_s buf_wrap;
	u64 buf_wrap_phy_addr;
	u32 buf_wrap_depth;
	struct vpss_chn_work_status_s chn_work_status;

	// hw cfgs;
	u8 is_cfg_changed;
};

FIFO_HEAD(vpssjobq, vpss_job*);

struct vpss_ctx {
	vpss_grp vpss_grp;
	u8 is_created;
	u8 is_started;
	u8 online_from_isp;
	atomic_t hdl_state;
	struct timespec64 time;
	struct mutex lock;
	vpss_grp_attr_s grp_attr;
	vpss_crop_info_s grp_crop_info;
	u8 chn_num;
	struct vpss_chn_cfg chn_cfgs[VPSS_MAX_CHN_NUM];
	struct crop_size frame_crop;
	s16 offset_top;
	s16 offset_bottom;
	s16 offset_left;
	s16 offset_right;
	struct vpss_grp_work_status_s grp_work_status;
	void *job_buffer;
	struct vpssjobq jobq;
	struct vpss_hw_cfg hw_cfg;
	u8 is_cfg_changed;
	u8 is_copy_upsample;
};

#ifdef __cplusplus
}
#endif

#endif /* __U_VPSS_CTX_H__ */

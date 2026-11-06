/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef _VO_CTX_H_
#define _VO_CTX_H_

#ifdef __cplusplus
	extern "C" {
#endif

#include <linux/comm_region.h>
#include "vbq.h"

struct vo_wbc_ctx {
	bool is_wbc_enable;
	bool is_wbc_src_cfg;
	bool is_wbc_attr_cfg;
	bool is_odma_enable;
	bool is_drop;
	vo_wbc_src_s wbc_src;
	vo_wbc_attr_s wbc_attr;
	vo_wbc_mode_e wbc_mode;
	u32 depth;

	struct vb_jobs_t wbc_jobs;
	struct mutex wbc_lock;
	struct task_struct *thread;
	wait_queue_head_t wq;
	u32 event;

	struct list_head qbuf_list;
	struct list_head dqbuf_list;
	spinlock_t qbuf_lock;
	spinlock_t dqbuf_lock;
	u8 qbuf_num;

	u32 done_cnt;
	u32 frame_num;
	u32 frame_rate;
	u32 odma_fifofull;
};

struct vo_chn_ctx {
	bool is_chn_enable;
	bool hide;
	bool pause;
	bool refresh;
	bool step;
	bool step_trigger;
	bool is_drop;
	vo_chn_attr_s chn_attr;
	vo_chn_zoom_attr_s chn_zoom_attr;
	vo_chn_border_attr_s chn_border_attr;
	vo_chn_param_s chn_param;
	vo_chn_mirror_type_e chn_mirror;
	u32 src_width;
	u32 src_height;
	rotation_e rotation;

	struct vb_jobs_t chn_jobs;
	struct mutex gdc_lock;

	u64 display_pts;
	u64 predone_pts;
	u32 frame_num;
	u32 src_frame_num;
	u32 frame_rate;
	u32 src_frame_rate;
	u32 frame_rate_user_set;
	u32 frame_index;
	u32 threshold;

	struct {
		u64 paddr;
		void *vaddr;
	} mesh;
};

struct vo_layer_ctx {
	vo_layer layer;
	bool is_layer_enable;
	bool is_layer_update;
	bool is_drop;
	vo_video_layer_attr_s layer_attr;
	u32 display_buflen;
	int proc_amp[PROC_AMP_MAX];
	int bind_dev_id;
	struct vo_chn_ctx chn_ctx[VO_MAX_CHN_NUM];
	struct mutex layer_lock;
	struct task_struct *thread;
	wait_queue_head_t wq;
	u32	event;
	spinlock_t list_lock;
	struct list_head list_wait;
	struct list_head list_work;
	struct list_head list_done;
	struct vb_jobs_t layer_jobs;

	u32 frame_num;
	u32 src_frame_num;
	u32 frame_rate;
	u32 src_frame_rate;
	u32 frame_index;
	u32 done_cnt;
	u32 toleration;
	u64 display_pts;
	u64 predone_pts;
	u32 bw_fail;
	u32 vgop_bw_fail;

	struct{
		u32 left;
		u32 top;
		u32 width;
		u32 height;
	} rect_crop;
};

struct vo_overlay_ctx {
	int bind_dev_id;
	u32 priority;
	rgn_handle rgn_handle[RGN_MAX_NUM_VO];
	struct rgn_cfg rgn_cfg;
};

struct vo_dev_ctx {
	bool is_dev_enable;
	vo_pub_attr_s pub_attr;
	int bind_layer_id;
	int bind_overlay_id[VO_MAX_GRAPHIC_LAYER_IN_DEV];
	vo_lvds_attr_s lvds_param;
	vo_bt_attr_s bt_param;
	struct mutex dev_lock;
};

struct vo_ctx {
	bool suspend;
	struct vo_dev_ctx dev_ctx[VO_MAX_DEV_NUM];
	struct vo_layer_ctx layer_ctx[VO_MAX_VIDEO_LAYER_NUM];
	struct vo_overlay_ctx overlay_ctx[VO_MAX_GRAPHIC_LAYER_NUM];
	struct vo_wbc_ctx wbc_ctx[VO_MAX_WBC_NUM];
};

#ifdef __cplusplus
}
#endif

#endif /* __VO_CTX_H__ */

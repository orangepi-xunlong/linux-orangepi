#ifndef __VCODEC_CB_H__
#define __VCODEC_CB_H__

#ifdef __cplusplus
	extern "C" {
#endif

#include <base_ctx.h>

enum VCODEC_CB_CMD {
	VCODEC_CB_SEND_FRM,
	VCODEC_CB_SKIP_FRM,
	VCODEC_CB_SNAP_JPG_FRM,
	VCODEC_CB_OVERFLOW_CHECK,
	VCODEC_CB_MAX
};

struct venc_send_frm_info {
	s32 vpss_grp;
	s32 vpss_chn;
	s32 vpss_chn1;
	struct video_buffer stInFrmBuf;
	struct video_buffer stInFrmBuf1;
	u8 isOnline;
	u32 sb_nb;
};

struct venc_snap_frm_info {
	s32 vpss_grp;
	s32 vpss_chn;
	u32 skip_frm_cnt;
};

#ifdef __cplusplus
}
#endif

#endif /* __VCODEC_CB_H__ */


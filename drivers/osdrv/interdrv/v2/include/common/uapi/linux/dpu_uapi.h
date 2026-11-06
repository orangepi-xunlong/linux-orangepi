/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: dpu_uapi.h
 * Description:
 */

#ifndef _U_DPU_UAPI_H_
#define _U_DPU_UAPI_H_

#include <linux/comm_dpu.h>
#include <linux/comm_sys.h>
#include <linux/defines.h>

#ifdef __cplusplus
extern "C" {
#endif


struct dpu_grp_cfg {
	dpu_grp dpu_grp_id;
};

struct dpu_grp_attr {
	dpu_grp dpu_grp_id;
	dpu_grp_attr_s grp_attr;
};

struct dpu_chn_cfg {
	dpu_grp dpu_grp_id;
	dpu_chn dpu_chn_id;
};

struct dpu_chn_attr {
	dpu_grp dpu_grp_id;
	dpu_chn dpu_chn_id;
	dpu_chn_attr_s chn_attr;
};

struct dpu_set_frame_cfg {
	dpu_grp dpu_grp_id;
	video_frame_info_s src_left_frame;
	video_frame_info_s src_right_frame;
	int millisec;
};

struct dpu_get_frame_cfg {
	dpu_grp dpu_grp_id;
	dpu_chn dpu_chn_id;
	video_frame_info_s video_frame;
	int millisec;
};

struct dpu_release_frame_cfg {
	dpu_grp dpu_grp_id;
	dpu_chn dpu_chn_id;
	video_frame_info_s video_frame;
};

struct dpu_vb_pool_cfg {
	dpu_grp dpu_grp_id;
	dpu_chn dpu_chn_id;
	unsigned int vbpool;
};

struct dpu_snap_cfg {
	dpu_grp dpu_grp_id;
	dpu_chn dpu_chn_id;
	unsigned int frame_cnt;
};


/* Public */
#define DPU_CREATE_GROUP _IOW('U', 0x00, struct dpu_grp_attr)
#define DPU_DESTROY_GROUP _IOW('U', 0x01, struct dpu_grp_cfg)
#define DPU_GET_AVAIL_GROUP _IOWR('U', 0x11, dpu_grp)
#define DPU_START_GROUP _IOW('U', 0x02, struct dpu_grp_cfg)
#define DPU_STOP_GROUP _IOW('u', 0x03, struct dpu_grp_cfg)
#define DPU_SET_GRP_ATTR _IOW('U', 0x05, struct dpu_grp_attr)
#define DPU_GET_GRP_ATTR _IOWR('U', 0x06, struct dpu_grp_attr)

#define DPU_SET_CHN_ATTR _IOW('U', 0x7, struct dpu_chn_attr)
#define DPU_GET_CHN_ATTR _IOWR('U', 0x8, struct dpu_chn_attr)

#define DPU_ENABLE_CHN _IOW('U', 0x9, struct dpu_chn_cfg)
#define DPU_DISABLE_CHN _IOW('U', 0xa, struct dpu_chn_cfg)

#define DPU_GET_FRAME _IOWR('U', 0x0b, struct dpu_get_frame_cfg)
#define DPU_RELEASE_FRAME _IOW('U', 0x0c, struct dpu_release_frame_cfg)
#define DPU_SEND_FRAME _IOW('U', 0x0d, struct dpu_set_frame_cfg)

#define DPU_CHECK_REG_READ _IO('U', 0x0e)
#define DPU_CHECK_REG_WRITE _IO('U', 0x0f)
#define DPU_SEND_CHN_FRAME _IOW('U', 0x10, struct dpu_get_frame_cfg)

#define DPU_GET_SGBM_STATUS _IO('U', 0xa0)
#define DPU_GET_FGS_STATUS _IO('U', 0xa1)
#define DPU_RESET _IO('U', 0xa2)


#ifdef __cplusplus
}
#endif

#endif /* _U_DPU_UAPI_H_ */

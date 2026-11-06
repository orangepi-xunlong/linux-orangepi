/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File name: stitch_uapi.h
 * Description:
 */

#ifndef _U_STITCH_UAPI_H_
#define _U_STITCH_UAPI_H_

#include "comm_stitch.h"

#ifdef __cplusplus
extern "C" {
#endif

struct stitch_src_frm_cfg {
	stitch_src_idx src_id;
	video_frame_info_s video_frame;
	int milli_sec;
};

struct stitch_chn_frm_cfg {
	video_frame_info_s video_frame;
	int milli_sec;
};

struct stitch_vb_pool_cfg {
	void *reserved;
	unsigned int vb_pool;
};

struct stitch_grp_src_frm_cfg {
	stitch_grp grp_id;
	struct stitch_src_frm_cfg cfg;
};

struct stitch_grp_chn_frm_cfg {
	stitch_grp grp_id;
	struct stitch_chn_frm_cfg cfg;
};

struct stitch_grp_vb_pool_cfg {
	stitch_grp grp_id;
	struct stitch_vb_pool_cfg vb_pool_cfg;
};

typedef struct _stitch_grp_src_bd_attr {
	stitch_grp grp_id;
	stitch_src_bd_attr src_bd_attr;
} stitch_grp_src_bd_attr;

typedef struct _stitch_grp_src_attr {
	stitch_grp grp_id;
	stitch_src_attr src_attr;
} stitch_grp_src_attr;

typedef struct _stitch_grp_bld_wgt_attr {
	stitch_grp grp_id;
	stitch_bld_wgt_attr bld_wgt_attr;
} stitch_grp_bld_wgt_attr;

typedef struct _stitch_grp_chn_attr {
	size_s img_size;
	stitch_grp grp_id;
	stitch_chn_attr chn_attr;
} stitch_grp_chn_attr;

typedef struct _stitch_grp_op_attr {
	stitch_grp grp_id;
	stitch_op_attr opt_attr;
} stitch_grp_op_attr;

/* Public */

#define STITCH_INIT          _IO('S',   0x00)
#define STITCH_DEINIT        _IO('S',   0x01)
#define STITCH_SET_SRC_ATTR  _IOW('S',  0x02, stitch_grp_src_attr)
#define STITCH_GET_SRC_ATTR  _IOWR('S', 0x03, stitch_grp_src_attr)
#define STITCH_SET_CHN_ATTR  _IOW('S',  0x04, stitch_grp_chn_attr)
#define STITCH_GET_CHN_ATTR  _IOWR('S', 0x05, stitch_grp_chn_attr)
#define STITCH_SET_OP_ATTR   _IOW('S',  0x06, stitch_grp_op_attr)
#define STITCH_GET_OP_ATTR   _IOWR('S', 0x07, stitch_grp_op_attr)
#define STITCH_SET_WGT_ATTR   _IOW('S',  0x08, stitch_grp_bld_wgt_attr)
#define STITCH_GET_WGT_ATTR   _IOWR('S', 0x09, stitch_grp_bld_wgt_attr)

#define STITCH_SET_REGX      _IOW('S',  0x0a, uint8_t)
#define STITCH_GRP_ENABLE    _IOW('S',   0x0b, stitch_grp)
#define STITCH_GRP_DISABLE   _IOW('S',   0x0c, stitch_grp)
#define STITCH_SEND_SRC_FRM  _IOW('S',  0x0d, struct stitch_grp_src_frm_cfg)
#define STITCH_SEND_CHN_FRM  _IOW('S',  0x0e, struct stitch_grp_chn_frm_cfg)
#define STITCH_GET_CHN_FRM   _IOWR('S', 0x0f, struct stitch_grp_chn_frm_cfg)
#define STITCH_RLS_CHN_FRM   _IOW('S',  0x10, struct stitch_grp_chn_frm_cfg)
#define STITCH_ATTACH_VB_POOL _IOW('S', 0x11, struct stitch_grp_vb_pool_cfg)
#define STITCH_DETACH_VB_POOL _IOW('S', 0x12, struct stitch_grp_vb_pool_cfg)
#define STITCH_DUMP_REGS _IO('S', 0x13)
#define STITCH_RST _IO('S', 0x14)

#define STITCH_SUSPEND _IO('S',0x15)
#define STITCH_RESUME _IO('S',0x16)

#define STITCH_INIT_GRP          _IOW('S',   0x17, stitch_grp)
#define STITCH_DEINIT_GRP        _IOW('S',   0x18, stitch_grp)
#define STITCH_GET_AVAIL_GROUP   _IOWR('S', 0x19, stitch_grp)

/* Internal use */

#ifdef __cplusplus
}
#endif

#endif /* _U_VPSS_UAPI_H_ */

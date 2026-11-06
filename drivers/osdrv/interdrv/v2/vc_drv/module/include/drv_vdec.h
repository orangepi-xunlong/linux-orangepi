/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/drv_vdec.h
 * Description:
 *   Common video decode definitions.
 */

#ifndef __DRV_VDEC_H__
#define __DRV_VDEC_H__

#include <base_ctx.h>
#include <linux/types.h>
#include <linux/common.h>
#include <linux/comm_video.h>
//#include "comm_vb.h"
#include <linux/comm_vdec.h>
#include <vb.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

int drv_vdec_create_chn(vdec_chn VdChn, const vdec_chn_attr_s *pstAttr);
int drv_vdec_destroy_chn(vdec_chn VdChn);

int drv_vdec_get_chn_attr(vdec_chn VdChn, vdec_chn_attr_s *pstAttr);
int drv_vdec_set_chn_attr(vdec_chn VdChn, const vdec_chn_attr_s *pstAttr);

int drv_vdec_start_recv_stream(vdec_chn VdChn);
int drv_vdec_stop_recv_stream(vdec_chn VdChn);

int drv_vdec_query_status(vdec_chn VdChn, vdec_chn_status_s *pstStatus);

int drv_vdec_get_fd(vdec_chn VdChn);
int drv_vdec_close_fd(vdec_chn VdChn);

int drv_vdec_reset_chn(vdec_chn VdChn);

int drv_vdec_set_chn_param(vdec_chn VdChn, const vdec_chn_param_s *pstParam);
int drv_vdec_get_chn_param(vdec_chn VdChn, vdec_chn_param_s *pstParam);

int drv_vdec_set_protocol_param(vdec_chn VdChn,
                  const vdec_prtcl_param_s *pstParam);
int drv_vdec_get_protocol_param(vdec_chn VdChn, vdec_prtcl_param_s *pstParam);

/* s32MilliSec: -1 is block,0 is no block,other positive number is timeout */
int drv_vdec_send_stream(vdec_chn VdChn, const vdec_stream_s *pstStream,
                int s32MilliSec);

int drv_vdec_get_frame(vdec_chn VdChn, video_frame_info_s *pstFrameInfo,
              int s32MilliSec);
int drv_vdec_release_frame(vdec_chn VdChn,
                  const video_frame_info_s *pstFrameInfo);

int drv_vdec_get_userdata(vdec_chn VdChn, vdec_userdata_s *pstUserData,
                 int s32MilliSec);
int drv_vdec_release_userdata(vdec_chn VdChn,
                 const vdec_userdata_s *pstUserData);

int drv_vdec_set_user_pic(vdec_chn VdChn,
                const video_frame_info_s *pstUsrPic);
int drv_vdec_enable_user_pic(vdec_chn VdChn, unsigned char bInstant);
int drv_vdec_DisableUserPic(vdec_chn VdChn);

int drv_vdec_set_display_mode(vdec_chn VdChn,
                video_display_mode_e enDisplayMode);
int drv_vdec_get_display_mode(vdec_chn VdChn,
                video_display_mode_e *penDisplayMode);

int drv_vdec_set_rotation(vdec_chn VdChn, rotation_e enRotation);
int drv_vdec_get_rotation(vdec_chn VdChn, rotation_e *penRotation);

int drv_vdec_attach_vbpool(vdec_chn VdChn, const vdec_chn_pool_s *pstPool);
int drv_vdec_detach_vbpool(vdec_chn VdChn);

int drv_vdec_set_userdata_attr(vdec_chn VdChn,
                 const vdec_user_data_attr_s *pstUserDataAttr);
int drv_vdec_get_userdata_attr(vdec_chn VdChn,
                 vdec_user_data_attr_s *pstUserDataAttr);

int drv_vdec_set_mod_param(const vdec_mod_param_s *pstModParam);
int drv_vdec_get_mod_param(vdec_mod_param_s *pstModParam);
int drv_vdec_frame_buffer_add_user(vdec_chn VdChn, video_frame_info_s *pstFrameInfo);
int drv_vdec_set_stride_align(vdec_chn VdChn, unsigned int align);
int drv_vdec_set_user_pic(vdec_chn VdChn, const video_frame_info_s *usr_pic);
int drv_vdec_enable_user_pic(vdec_chn VdChn, unsigned char instant);
int drv_vdec_disable_user_pic(vdec_chn VdChn);
int drv_vdec_set_display_mode(vdec_chn VdChn, video_display_mode_e display_mode);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef  __DRV_VDEC_H__ */

/*
 * Copyright (C) Cvitek Co., Ltd. 2019-2020. All rights reserved.
 *
 * File Name: include/drv_venc.h
 * Description:
 *   Common video encode definitions.
 */

#ifndef __DRV_VENC_H__
#define __DRV_VENC_H__

#include <base_ctx.h>
#include <linux/comm_video.h>
//#include "comm_vb.h"
#include <linux/comm_venc.h>
#include <linux/common.h>
#include <vb.h>

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

int drv_venc_create_chn(venc_chn VeChn, const venc_chn_attr_s *pstAttr);
int drv_venc_destroy_chn(venc_chn VeChn);
int drv_venc_reset_chn(venc_chn VeChn);

int drv_venc_start_recvframe(venc_chn VeChn,
                const venc_recv_pic_param_s *pstRecvParam);
int drv_venc_stop_recvframe(venc_chn VeChn);

int drv_venc_query_status(venc_chn VeChn, venc_chn_status_s *pstStatus);

int drv_venc_set_chn_attr(venc_chn VeChn, const venc_chn_attr_s *pstChnAttr);
int drv_venc_get_chn_attr(venc_chn VeChn, venc_chn_attr_s *pstChnAttr);

int drv_venc_get_stream(venc_chn VeChn, venc_stream_s *pstStream,
               int S32MilliSec);
int drv_venc_release_stream(venc_chn VeChn, venc_stream_s *pstStream);

int drv_venc_insert_userdata(venc_chn VeChn, unsigned char *pu8Data,
                unsigned int u32Len);

int drv_venc_send_frame(venc_chn VeChn, const video_frame_info_s *pstFrame,
               int s32MilliSec);
int drv_venc_send_frame_ex(venc_chn VeChn, const user_frame_info_s *pstFrame,
                 int s32MilliSec);

int drv_venc_request_idr(venc_chn VeChn, unsigned char bInstant);

int drv_venc_get_fd(venc_chn VeChn);
int drv_venc_close_fd(venc_chn VeChn);

int drv_venc_set_roi_attr(venc_chn VeChn, const venc_roi_attr_s *pstRoiAttr);
int drv_venc_get_roi_attr(venc_chn VeChn, unsigned int u32Index,
                venc_roi_attr_s *pstRoiAttr);

int drv_venc_get_roi_attr_ex(venc_chn VeChn, unsigned int u32Index,
                  venc_roi_attr_ex_s *pstRoiAttrEx);
int drv_venc_set_roi_attr_ex(venc_chn VeChn,
                  const venc_roi_attr_ex_s *pstRoiAttrEx);

int
drv_venc_set_roi_bgframerate(venc_chn VeChn,
               const venc_roibg_frame_rate_s *pstRoiBgFrmRate);
int drv_venc_get_roi_bgframerate(venc_chn VeChn,
                   venc_roibg_frame_rate_s *pstRoiBgFrmRate);

int
drv_venc_set_h264_slicesplit(venc_chn VeChn,
               const venc_h264_slice_split_s *pstSliceSplit);
int drv_venc_get_h264_slicesplit(venc_chn VeChn,
                   venc_h264_slice_split_s *pstSliceSplit);

int
drv_venc_set_h264_intrapred(venc_chn VeChn,
              const venc_h264_intra_pred_s *pstH264IntraPred);
int drv_venc_get_h264_intrapred(venc_chn VeChn,
                  venc_h264_intra_pred_s *pstH264IntraPred);

int drv_venc_set_h264_trans(venc_chn VeChn,
                  const venc_h264_trans_s *pstH264Trans);
int drv_venc_get_h264_trans(venc_chn VeChn, venc_h264_trans_s *pstH264Trans);

int drv_venc_set_h264_entropy(venc_chn VeChn,
                const venc_h264_entropy_s *pstH264EntropyEnc);
int drv_venc_get_h264_entropy(venc_chn VeChn,
                venc_h264_entropy_s *pstH264EntropyEnc);

int drv_venc_set_h264_dblk(venc_chn VeChn,
                 const venc_h264_dblk_s *pstH264Dblk);
int drv_venc_get_h264_dblk(venc_chn VeChn, venc_h264_dblk_s *pstH264Dblk);

int drv_venc_set_h264_vui(venc_chn VeChn, const venc_h264_vui_s *pstH264Vui);
int drv_venc_get_h264_vui(venc_chn VeChn, venc_h264_vui_s *pstH264Vui);

int drv_venc_set_h265_vui(venc_chn VeChn, const venc_h265_vui_s *pstH265Vui);
int drv_venc_get_h265_vui(venc_chn VeChn, venc_h265_vui_s *pstH265Vui);

int drv_venc_set_jpeg_param(venc_chn VeChn,
                  const venc_jpeg_param_s *pstJpegParam);
int drv_venc_get_jpeg_param(venc_chn VeChn, venc_jpeg_param_s *pstJpegParam);

int drv_venc_set_mjpeg_param(venc_chn VeChn,
                   const venc_mjpeg_param_s *pstMjpegParam);
int drv_venc_get_mjpeg_param(venc_chn VeChn,
                   venc_mjpeg_param_s *pstMjpegParam);

int drv_venc_get_rc_param(venc_chn VeChn, venc_rc_param_s *pstRcParam);
int drv_venc_set_rc_param(venc_chn VeChn, const venc_rc_param_s *pstRcParam);

int drv_venc_set_ref_param(venc_chn VeChn,
                 const venc_ref_param_s *pstRefParam);
int drv_venc_get_ref_param(venc_chn VeChn, venc_ref_param_s *pstRefParam);

int
drv_venc_set_jpeg_encode_mode(venc_chn VeChn,
               const venc_jpeg_encode_mode_e enJpegEncodeMode);
int drv_venc_get_jpeg_encode_mode(venc_chn VeChn,
                   venc_jpeg_encode_mode_e *penJpegEncodeMode);

int drv_venc_enable_idr(venc_chn VeChn, unsigned char bEnableIDR);

int drv_venc_get_stream_bufinfo(venc_chn VeChn,
                  venc_stream_buf_info_s *pstStreamBufInfo);

int
drv_venc_set_h265_slicesplit(venc_chn VeChn,
               const venc_h265_slice_split_s *pstSliceSplit);
int drv_venc_get_h265_slicesplit(venc_chn VeChn,
                   venc_h265_slice_split_s *pstSliceSplit);

int drv_venc_set_h265_predunit(venc_chn VeChn,
                 const venc_h265_pu_s *pstPredUnit);
int drv_venc_get_h265_predunit(venc_chn VeChn, venc_h265_pu_s *pstPredUnit);

int drv_venc_set_h265_trans(venc_chn VeChn,
                  const venc_h265_trans_s *pstH265Trans);
int drv_venc_get_h265_trans(venc_chn VeChn, venc_h265_trans_s *pstH265Trans);

int drv_venc_set_h265_entropy(venc_chn VeChn,
                const venc_h265_entropy_s *pstH265Entropy);
int drv_venc_get_h265_entropy(venc_chn VeChn,
                venc_h265_entropy_s *pstH265Entropy);

int drv_venc_set_h265_dblk(venc_chn VeChn,
                 const venc_h265_dblk_s *pstH265Dblk);
int drv_venc_get_h265_dblk(venc_chn VeChn, venc_h265_dblk_s *pstH265Dblk);

int drv_venc_set_h265_sao(venc_chn VeChn, const venc_h265_sao_s *pstH265Sao);
int drv_venc_get_h265_sao(venc_chn VeChn, venc_h265_sao_s *pstH265Sao);

int drv_venc_set_framelost_strategy(venc_chn VeChn,
                      const venc_framelost_s *pstFrmLostParam);
int drv_venc_get_framelost_strategy(venc_chn VeChn,
                      venc_framelost_s *pstFrmLostParam);

int
drv_venc_set_superframe_strategy(venc_chn VeChn,
                   const venc_superframe_cfg_s *pstSuperFrmParam);
int drv_venc_get_superframe_strategy(venc_chn VeChn,
                       venc_superframe_cfg_s *pstSuperFrmParam);

int drv_venc_set_intra_refresh(venc_chn VeChn,
                 const venc_intra_refresh_s *pstIntraRefresh);
int drv_venc_get_intra_refresh(venc_chn VeChn,
                 venc_intra_refresh_s *pstIntraRefresh);

int drv_venc_get_sseregion(venc_chn VeChn, unsigned int u32Index,
                  venc_sse_cfg_s *pstSSECfg);
int drv_venc_set_sseregion(venc_chn VeChn, const venc_sse_cfg_s *pstSSECfg);

int drv_venc_set_chn_param(venc_chn VeChn,
                 const venc_chn_param_s *pstChnParam);
int drv_venc_get_chn_param(venc_chn VeChn, venc_chn_param_s *pstChnParam);

int drv_venc_set_modparam(const venc_param_mod_s *pstModParam);
int drv_venc_get_modparam(venc_param_mod_s *pstModParam);

int
drv_venc_get_foreground_protect(venc_chn VeChn,
                  venc_foreground_protect_s *pstForegroundProtect);
int drv_venc_set_foreground_protect(
    venc_chn VeChn, const venc_foreground_protect_s *pstForegroundProtect);

int drv_venc_set_scenemode(venc_chn VeChn,
                  const venc_scene_mode_e enSceneMode);
int drv_venc_get_scenemode(venc_chn VeChn, venc_scene_mode_e *penSceneMode);

int drv_venc_attach_vbpool(venc_chn VeChn, const venc_chn_pool_s *pstPool);
int drv_venc_detach_vbpool(venc_chn VeChn);

int drv_venc_set_cu_prediction(venc_chn VeChn,
                 const venc_cu_prediction_s *pstCuPrediction);
int drv_venc_get_cu_prediction(venc_chn VeChn,
                 venc_cu_prediction_s *pstCuPrediction);

int drv_venc_set_skipbias(venc_chn VeChn,
                 const venc_skip_bias_s *pstSkipBias);
int drv_venc_get_skipbias(venc_chn VeChn, venc_skip_bias_s *pstSkipBias);

int
drv_venc_set_debreatheffect(venc_chn VeChn,
               const venc_debreatheffect_s *pstDeBreathEffect);
int drv_venc_get_debreatheffect(venc_chn VeChn,
                   venc_debreatheffect_s *pstDeBreathEffect);

int
drv_venc_set_hierarchicalqp(venc_chn VeChn,
               const venc_hierarchical_qp_s *pstHierarchicalQp);
int drv_venc_get_hierarchicalqp(venc_chn VeChn,
                   venc_hierarchical_qp_s *pstHierarchicalQp);

int drv_venc_set_rc_advparam(venc_chn VeChn,
                   const venc_rc_advparam_s *pstRcAdvParam);
int drv_venc_get_rc_advparam(venc_chn VeChn,
                   venc_rc_advparam_s *pstRcAdvParam);

int drv_venc_calc_frame_param(venc_chn VeChn,
                venc_frame_param_s *pstFrameParam);
int drv_venc_set_frame_param(venc_chn VeChn,
                   const venc_frame_param_s *pstFrameParam);
int drv_venc_get_frame_param(venc_chn VeChn,
                   venc_frame_param_s *pstFrameParam);

int drv_venc_set_custommap(venc_chn VeChn,
                   const venc_custom_map_s *pstVeCustomMap);
int drv_venc_get_intialInfo(venc_chn VeChn, venc_initial_info_s *pstVencInitialInfo);
int drv_venc_get_encode_header(venc_chn VeChn, venc_encode_header_s *pstEncodeHeader);

int drv_venc_set_search_window(venc_chn VeChn, const venc_search_window_s *pstVencSearchWindow);
int drv_venc_get_search_window(venc_chn VeChn, venc_search_window_s *pstVencSearchWindow);

int drv_venc_set_extern_buf(venc_chn VeChn, const venc_extern_buf_s *pstExternBuf);

#define DRV_VENC_MASK_ERR		0x1
#define DRV_VENC_MASK_WARN		0x2
#define DRV_VENC_MASK_INFO		0x3
#define DRV_VENC_MASK_DBG		0x4

extern unsigned int VENC_LOG_LV;

#define DRV_VENC_ERR(msg, ...)		\
    do { \
        if (VENC_LOG_LV >= DRV_VENC_MASK_ERR) \
        pr_err("[ERR] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VENC_WARN(msg, ...)		\
    do { \
        if (VENC_LOG_LV >= DRV_VENC_MASK_WARN) \
        pr_warn("[WARN] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VENC_DBG(msg, ...)	\
    do { \
        if (VENC_LOG_LV >= DRV_VENC_MASK_DBG) \
        pr_notice("[CFG] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#define DRV_VENC_INFO(msg, ...)		\
    do { \
        if (VENC_LOG_LV >= DRV_VENC_MASK_INFO) \
        pr_info("[INFO] %s = %d, "msg, __func__, __LINE__, ## __VA_ARGS__); \
    } while (0)


#define DRV_H264_PROFILE_DEFAULT H264E_PROFILE_HIGH
#define DRV_H264_PROFILE_MIN 0
#define DRV_H264_PROFILE_MAX (H264E_PROFILE_BUTT - 1)

#define DRV_H264_ENTROPY_DEFAULT 1
#define DRV_H264_ENTROPY_MIN 0
#define DRV_H264_ENTROPY_MAX 1

#define DRV_INITIAL_DELAY_DEFAULT 1000
#define DRV_INITIAL_DELAY_MIN 10
#define DRV_INITIAL_DELAY_MAX 3000

#define DRV_VARI_FPS_EN_DEFAULT 0
#define DRV_VARI_FPS_EN_MIN 0
#define DRV_VARI_FPS_EN_MAX 1

#define DRV_H26X_GOP_DEFAULT 60
#define DRV_H26X_GOP_MIN 1
#define DRV_H26X_GOP_MAX 3600

#define DRV_H26X_GOP_MODE_DEFAULT VENC_GOPMODE_NORMALP
#define DRV_H26X_GOP_MODE_MIN VENC_GOPMODE_NORMALP
#define DRV_H26X_GOP_MODE_MAX (VENC_GOPMODE_BUTT - 1)

#define DRV_H26X_NORMALP_IP_QP_DELTA_DEFAULT 2
#define DRV_H26X_NORMALP_IP_QP_DELTA_MIN -10
#define DRV_H26X_NORMALP_IP_QP_DELTA_MAX 30

#define DRV_H26X_SMARTP_BG_INTERVAL_DEFAULT (DRV_H26X_GOP_DEFAULT * 2)
#define DRV_H26X_SMARTP_BG_INTERVAL_MIN DRV_H26X_GOP_MIN
#define DRV_H26X_SMARTP_BG_INTERVAL_MAX 65536

#define DRV_H26X_SMARTP_BG_QP_DELTA_DEFAULT 2
#define DRV_H26X_SMARTP_BG_QP_DELTA_MIN -10
#define DRV_H26X_SMARTP_BG_QP_DELTA_MAX 30

#define DRV_H26X_SMARTP_VI_QP_DELTA_DEFAULT 0
#define DRV_H26X_SMARTP_VI_QP_DELTA_MIN -10
#define DRV_H26X_SMARTP_VI_QP_DELTA_MAX 30

#define DRV_H26X_MAXIQP_DEFAULT 51
#define DRV_H26X_MAXIQP_MIN 1
#define DRV_H26X_MAXIQP_MAX 51

#define DRV_H26X_MINIQP_DEFAULT 1
#define DRV_H26X_MINIQP_MIN 1
#define DRV_H26X_MINIQP_MAX 51

#define DRV_H26X_MAXQP_DEFAULT 51
#define DRV_H26X_MAXQP_MIN 0
#define DRV_H26X_MAXQP_MAX 51

#define DRV_H26X_MINQP_DEFAULT 1
#define DRV_H26X_MINQP_MIN 0
#define DRV_H26X_MINQP_MAX 51

#define DRV_H26X_MAX_I_PROP_DEFAULT 100
#define DRV_H26X_MAX_I_PROP_MIN 1
#define DRV_H26X_MAX_I_PROP_MAX 100

#define DRV_H26X_MIN_I_PROP_DEFAULT 1
#define DRV_H26X_MIN_I_PROP_MIN 1
#define DRV_H26X_MIN_I_PROP_MAX 100

#define DRV_H26X_MAXBITRATE_DEFAULT 5000
#define DRV_H26X_MAXBITRATE_MIN 100
#define DRV_H26X_MAXBITRATE_MAX 300000

#define DRV_H26X_CHANGE_POS_DEFAULT 90
#define DRV_H26X_CHANGE_POS_MIN 50
#define DRV_H26X_CHANGE_POS_MAX 100

#define DRV_H26X_MIN_STILL_PERCENT_DEFAULT 10
#define DRV_H26X_MIN_STILL_PERCENT_MIN 1
#define DRV_H26X_MIN_STILL_PERCENT_MAX 100

#define DRV_H26X_MAX_STILL_QP_DEFAULT 1
#define DRV_H26X_MAX_STILL_QP_MIN 0
#define DRV_H26X_MAX_STILL_QP_MAX 51

#define DRV_H26X_MOTION_SENSITIVITY_DEFAULT 100
#define DRV_H26X_MOTION_SENSITIVITY_MIN 1
#define DRV_H26X_MOTION_SENSITIVITY_MAX 1024

#define DRV_H26X_AVBR_FRM_LOST_OPEN_DEFAULT 1
#define DRV_H26X_AVBR_FRM_LOST_OPEN_MIN 0
#define DRV_H26X_AVBR_FRM_LOST_OPEN_MAX 1

#define DRV_H26X_AVBR_FRM_GAP_DEFAULT 1
#define DRV_H26X_AVBR_FRM_GAP_MIN 0
#define DRV_H26X_AVBR_FRM_GAP_MAX 100

#define DRV_H26X_AVBR_PURE_STILL_THR_DEFAULT 4
#define DRV_H26X_AVBR_PURE_STILL_THR_MIN 0
#define DRV_H26X_AVBR_PURE_STILL_THR_MAX 500

#define DRV_H26X_INTRACOST_DEFAULT 0
#define DRV_H26X_INTRACOST_MIN 0
#define DRV_H26X_INTRACOST_MAX 16383

#define DRV_H26X_THRDLV_DEFAULT 2
#define DRV_H26X_THRDLV_MIN 0
#define DRV_H26X_THRDLV_MAX 4

#define DRV_H26X_BG_ENHANCE_EN_DEFAULT 0
#define DRV_H26X_BG_ENHANCE_EN_MIN 0
#define DRV_H26X_BG_ENHANCE_EN_MAX 1

#define DRV_H26X_BG_DELTA_QP_DEFAULT 0
#define DRV_H26X_BG_DELTA_QP_MIN -8
#define DRV_H26X_BG_DELTA_QP_MAX 8

#define DRV_H26X_ROW_QP_DELTA_DEFAULT 1
#define DRV_H26X_ROW_QP_DELTA_MIN 0
#define DRV_H26X_ROW_QP_DELTA_MAX 10

#define DRV_H26X_SUPER_FRM_MODE_DEFAULT 0
#define DRV_H26X_SUPER_FRM_MODE_MIN 0
#define DRV_H26X_SUPER_FRM_MODE_MAX 3

#define DRV_H26X_SUPER_I_BITS_THR_DEFAULT (4 * 8 * 1024 * 1024)
#define DRV_H26X_SUPER_I_BITS_THR_MIN 1000
#define DRV_H26X_SUPER_I_BITS_THR_MAX (10 * 8 * 1024 * 1024)

#define DRV_H26X_SUPER_P_BITS_THR_DEFAULT (4 * 8 * 1024 * 1024)
#define DRV_H26X_SUPER_P_BITS_THR_MIN 1000
#define DRV_H26X_SUPER_P_BITS_THR_MAX (10 * 8 * 1024 * 1024)

#define DRV_H26X_MAX_RE_ENCODE_DEFAULT 0
#define DRV_H26X_MAX_RE_ENCODE_MIN 0
#define DRV_H26X_MAX_RE_ENCODE_MAX 4

#define DRV_H26X_ASPECT_RATIO_INFO_PRESENT_FLAG_DEFAULT 0
#define DRV_H26X_ASPECT_RATIO_INFO_PRESENT_FLAG_MIN 0
#define DRV_H26X_ASPECT_RATIO_INFO_PRESENT_FLAG_MAX 1

#define DRV_H26X_ASPECT_RATIO_IDC_DEFAULT 1
#define DRV_H26X_ASPECT_RATIO_IDC_MIN 0
#define DRV_H26X_ASPECT_RATIO_IDC_MAX 255

#define DRV_H26X_OVERSCAN_INFO_PRESENT_FLAG_DEFAULT 0
#define DRV_H26X_OVERSCAN_INFO_PRESENT_FLAG_MIN 0
#define DRV_H26X_OVERSCAN_INFO_PRESENT_FLAG_MAX 1

#define DRV_H26X_OVERSCAN_APPROPRIATE_FLAG_DEFAULT 0
#define DRV_H26X_OVERSCAN_APPROPRIATE_FLAG_MIN 0
#define DRV_H26X_OVERSCAN_APPROPRIATE_FLAG_MAX 1

#define DRV_H26X_SAR_WIDTH_DEFAULT 1
#define DRV_H26X_SAR_WIDTH_MIN 1
#define DRV_H26X_SAR_WIDTH_MAX 65535

#define DRV_H26X_SAR_HEIGHT_DEFAULT 1
#define DRV_H26X_SAR_HEIGHT_MIN 1
#define DRV_H26X_SAR_HEIGHT_MAX 65535

#define DRV_H26X_TIMING_INFO_PRESENT_FLAG_DEFAULT 0
#define DRV_H26X_TIMING_INFO_PRESENT_FLAG_MIN 0
#define DRV_H26X_TIMING_INFO_PRESENT_FLAG_MAX 1

#define DRV_H264_FIXED_FRAME_RATE_FLAG_DEFAULT 0
#define DRV_H264_FIXED_FRAME_RATE_FLAG_MIN 0
#define DRV_H264_FIXED_FRAME_RATE_FLAG_MAX 1

#define DRV_H26X_NUM_UNITS_IN_TICK_DEFAULT 1
#define DRV_H26X_NUM_UNITS_IN_TICK_MIN 1
#define DRV_H26X_NUM_UNITS_IN_TICK_MAX (UINT_MAX)

#define DRV_H26X_TIME_SCALE_DEFAULT 60
#define DRV_H26X_TIME_SCALE_MIN 1
#define DRV_H26X_TIME_SCALE_MAX (UINT_MAX)

#define DRV_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_DEFAULT 1
#define DRV_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_MIN 0
#define DRV_H265_NUM_TICKS_POC_DIFF_ONE_MINUS1_MAX (UINT_MAX - 1)

#define DRV_H26X_VIDEO_SIGNAL_TYPE_PRESENT_FLAG_DEFAULT 0
#define DRV_H26X_VIDEO_SIGNAL_TYPE_PRESENT_FLAG_MIN 0
#define DRV_H26X_VIDEO_SIGNAL_TYPE_PRESENT_FLAG_MAX 1

#define DRV_H26X_VIDEO_FORMAT_DEFAULT 5
#define DRV_H26X_VIDEO_FORMAT_MIN 0
#define DRV_H264_VIDEO_FORMAT_MAX 7
#define DRV_H265_VIDEO_FORMAT_MAX 5

#define DRV_H26X_VIDEO_FULL_RANGE_FLAG_DEFAULT 0
#define DRV_H26X_VIDEO_FULL_RANGE_FLAG_MIN 0
#define DRV_H26X_VIDEO_FULL_RANGE_FLAG_MAX 1

#define DRV_H26X_COLOUR_DESCRIPTION_PRESENT_FLAG_DEFAULT 0
#define DRV_H26X_COLOUR_DESCRIPTION_PRESENT_FLAG_MIN 0
#define DRV_H26X_COLOUR_DESCRIPTION_PRESENT_FLAG_MAX 1

#define DRV_H26X_COLOUR_PRIMARIES_DEFAULT 2
#define DRV_H26X_COLOUR_PRIMARIES_MIN 0
#define DRV_H26X_COLOUR_PRIMARIES_MAX 255

#define DRV_H26X_TRANSFER_CHARACTERISTICS_DEFAULT 2
#define DRV_H26X_TRANSFER_CHARACTERISTICS_MIN 0
#define DRV_H26X_TRANSFER_CHARACTERISTICS_MAX 255

#define DRV_H26X_MATRIX_COEFFICIENTS_DEFAULT 2
#define DRV_H26X_MATRIX_COEFFICIENTS_MIN 0
#define DRV_H26X_MATRIX_COEFFICIENTS_MAX 255

#define DRV_H26X_BITSTREAM_RESTRICTION_FLAG_DEFAULT 0
#define DRV_H26X_BITSTREAM_RESTRICTION_FLAG_MIN 0
#define DRV_H26X_BITSTREAM_RESTRICTION_FLAG_MAX 1

#define DRV_H26X_TEST_UBR_EN_DEFAULT 0
#define DRV_H26X_TEST_UBR_EN_MIN 0
#define DRV_H26X_TEST_UBR_EN_MAX 1

#define DRV_H26X_FRAME_QP_DEFAULT 38
#define DRV_H26X_FRAME_QP_MIN 0
#define DRV_H26X_FRAME_QP_MAX 51

#define DRV_H26X_FRAME_BITS_DEFAULT (200 * 1000)
#define DRV_H26X_FRAME_BITS_MIN 1000
#define DRV_H26X_FRAME_BITS_MAX 10000000

#define DRV_H26X_ES_BUFFER_QUEUE_DEFAULT 1
#define DRV_H26X_ES_BUFFER_QUEUE_MIN 0
#define DRV_H26X_ES_BUFFER_QUEUE_MAX 1

#define DRV_H26X_ISO_SEND_FRAME_DEFAUL 1
#define DRV_H26X_ISO_SEND_FRAME_MIN 0
#define DRV_H26X_ISO_SEND_FRAME_MAX 1

#define DRV_H26X_SENSOR_EN_DEFAULT      0
#define DRV_H26X_SENSOR_EN_MIN          0
#define DRV_H26X_SENSOR_EN_MAX          1

#define DEF_STAT_TIME -1
#define DEF_GOP 30
#define DEF_IQP 32
#define DEF_PQP 32

#define DEF_VARI_FPS_EN 0
#define DEF_264_GOP 60
#define DEF_264_MAXIQP 51
#define DEF_264_MINIQP 1
#define DEF_264_MAXQP 51
#define DEF_264_MINQP 1
#define DEF_264_MAXBITRATE 5000
#define DEF_26X_CHANGE_POS 90

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __DRV_VENC_H__ */

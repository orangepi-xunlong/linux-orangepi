
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <generated/compile.h>

#include <base_ctx.h>
#include <linux/comm_venc.h>
#include <linux/comm_vdec.h>

#include "drv_venc.h"
#include "drv_vdec.h"
#include "venc.h"
#include "vdec.h"

//#include "vpuconfig.h"
#include "vc_drv_proc.h"

extern venc_context *handle;
extern uint32_t MaxVencChnNum;
#ifdef ENABLE_DEC
extern vdec_context *vdec_handle;
extern uint32_t MaxVdecChnNum;
#endif

proc_debug_config_t tVencDebugConfig = { .u32DbgMask = 0x1, .u32EndFrmIdx = UINT_MAX };
proc_debug_config_t tVdecDebugConfig = { .u32DbgMask = 0x1, .u32EndFrmIdx = UINT_MAX };

static void get_codec_type_str(payload_type_e enType, char *pcCodecType)
{
    switch (enType) {
    case PT_JPEG:
        strcpy(pcCodecType, "JPEG");
        break;
    case PT_MJPEG:
        strcpy(pcCodecType, "MJPEG");
        break;
    case PT_H264:
        strcpy(pcCodecType, "H264");
        break;
    case PT_H265:
        strcpy(pcCodecType, "H265");
        break;
    default:
        strcpy(pcCodecType, "N/A");
        break;
    }
}

static void get_rc_mode_str(venc_rc_mode_e enRcMode, char *pRcMode)
{
    switch (enRcMode) {
    case VENC_RC_MODE_H264CBR:
    case VENC_RC_MODE_H265CBR:
    case VENC_RC_MODE_MJPEGCBR:
        strcpy(pRcMode, "CBR");
        break;
    case VENC_RC_MODE_H264VBR:
    case VENC_RC_MODE_H265VBR:
    case VENC_RC_MODE_MJPEGVBR:
        strcpy(pRcMode, "VBR");
        break;
    case VENC_RC_MODE_H264AVBR:
    case VENC_RC_MODE_H265AVBR:
        strcpy(pRcMode, "AVBR");
        break;
    case VENC_RC_MODE_H264QVBR:
    case VENC_RC_MODE_H265QVBR:
        strcpy(pRcMode, "QVBR");
        break;
    case VENC_RC_MODE_H264FIXQP:
    case VENC_RC_MODE_H265FIXQP:
    case VENC_RC_MODE_MJPEGFIXQP:
        strcpy(pRcMode, "FIXQP");
        break;
    case VENC_RC_MODE_H264QPMAP:
    case VENC_RC_MODE_H265QPMAP:
        strcpy(pRcMode, "QPMAP");
        break;
    case VENC_RC_MODE_H264UBR:
    case VENC_RC_MODE_H265UBR:
        strcpy(pRcMode, "UBR");
        break;
    default:
        strcpy(pRcMode, "N/A");
        break;
    }
}

static void get_gop_mode_str(venc_gop_mode_e enGopMode, char *pcGopMode)
{
    switch (enGopMode) {
    case VENC_GOPMODE_NORMALP:
        strcpy(pcGopMode, "NORMALP");
        break;
    case VENC_GOPMODE_DUALP:
        strcpy(pcGopMode, "DUALP");
        break;
    case VENC_GOPMODE_SMARTP:
        strcpy(pcGopMode, "SMARTP");
        break;
    case VENC_GOPMODE_ADVSMARTP:
        strcpy(pcGopMode, "ADVSMARTP");
        break;
    case VENC_GOPMODE_BIPREDB:
        strcpy(pcGopMode, "BIPREDB");
        break;
    case VENC_GOPMODE_LOWDELAYB:
        strcpy(pcGopMode, "LOWDELAYB");
        break;
    case VENC_GOPMODE_BUTT:
        strcpy(pcGopMode, "BUTT");
        break;
    default:
        strcpy(pcGopMode, "N/A");
        break;
    }
}

static void get_pixel_format_str(pixel_format_e enPixelFormat, char *pcPixelFormat)
{
    switch (enPixelFormat) {
    case PIXEL_FORMAT_YUV_PLANAR_422:
        strcpy(pcPixelFormat, "YUV422");
        break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
        strcpy(pcPixelFormat, "YUV420");
        break;
    case PIXEL_FORMAT_YUV_PLANAR_444:
        strcpy(pcPixelFormat, "YUV444");
        break;
    case PIXEL_FORMAT_NV12:
        strcpy(pcPixelFormat, "NV12");
        break;
    case PIXEL_FORMAT_NV21:
        strcpy(pcPixelFormat, "NV21");
        break;
    default:
        strcpy(pcPixelFormat, "N/A");
        break;
    }
}

static void get_framerate(venc_chn_attr_s *pstChnAttr, unsigned int *pu32SrcFrameRate,
             DRV_FR32 *pfr32DstFrameRate)
{
    switch (pstChnAttr->stRcAttr.enRcMode) {
    case VENC_RC_MODE_H264CBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH264Cbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH264Cbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H265CBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH265Cbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH265Cbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_MJPEGCBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stMjpegCbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stMjpegCbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H264VBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH264Vbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH264Vbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H265VBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH265Vbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH265Vbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_MJPEGVBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stMjpegVbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stMjpegVbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H264FIXQP:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH264FixQp.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH264FixQp.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H265FIXQP:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH265FixQp.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH265FixQp.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_MJPEGFIXQP:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stMjpegFixQp.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stMjpegFixQp.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H264AVBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH264AVbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH264AVbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H265AVBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH265AVbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH265AVbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H264QVBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH264QVbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH264QVbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H265QVBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH265QVbr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH265QVbr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H264QPMAP:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH264QpMap.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH264QpMap.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H265QPMAP:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH265QpMap.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH265QpMap.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H264UBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH264Ubr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH264Ubr.fr32DstFrameRate;
        break;
    case VENC_RC_MODE_H265UBR:
        *pu32SrcFrameRate =
            pstChnAttr->stRcAttr.stH265Ubr.u32SrcFrameRate;
        *pfr32DstFrameRate =
            pstChnAttr->stRcAttr.stH265Ubr.fr32DstFrameRate;
        break;
    default:
        break;
    }
}

static int venc_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Module: [VENC] System Build Time [%s]\n", UTS_VERSION);

    if (handle != NULL) {
        int idx = 0;
        drv_venc_param_mod_s *pVencModParam = &handle->ModParam;

        seq_puts(
            m,
            "-----MODULE PARAM---------------------------------------------\n");
        seq_printf(
            m,
            "VencBufferCache: %u\t FrameBufRecycle: %d\t VencMaxChnNum: %u\n\n",
            pVencModParam->stVencModParam.u32VencBufferCache,
            pVencModParam->stVencModParam.u32FrameBufRecycle,
            MaxVencChnNum);

        for (idx = 0; idx < MaxVencChnNum; idx++) {
            if (handle && handle->chn_handle[idx] != NULL) {
                char cCodecType[6] = { '\0' };
                char cRcMode[6] = { '\0' };
                char cGopMode[10] = { '\0' };
                char cPixelFormat[8] = { '\0' };
                unsigned int u32SrcFrameRate = 0;
                DRV_FR32 fr32DstFrameRate = 0;
                int roiIdx = 0;

                venc_chn_attr_s *pstChnAttr =
                    handle->chn_handle[idx]->pChnAttr;
                venc_chn_param_s *pstChnParam =
                    &handle->chn_handle[idx]->pChnVars->stChnParam;
                venc_chn_status_s *pchnStatus =
                    &handle->chn_handle[idx]->pChnVars->chnStatus;
                venc_stream_s *pstStream =
                    &handle->chn_handle[idx]->pChnVars->stStream;
                vcodec_perf_fps_s *pstFPS =
                    &handle->chn_handle[idx]->pChnVars->stFPS;
                video_frame_s *pstVFrame =
                    &handle->chn_handle[idx]->pChnVars->stFrameInfo.video_frame;

                get_codec_type_str(pstChnAttr->stVencAttr.enType,
                        cCodecType);
                get_rc_mode_str(pstChnAttr->stRcAttr.enRcMode,
                         cRcMode);
                get_gop_mode_str(pstChnAttr->stGopAttr.enGopMode,
                          cGopMode);
                get_pixel_format_str(pstVFrame->pixel_format,
                          cPixelFormat);
                get_framerate(pstChnAttr, &u32SrcFrameRate,
                         &fr32DstFrameRate);

                seq_puts(
                    m,
                    "-----VENC CHN ATTR 1---------------------------------------------\n");
                seq_printf(
                    m,
                    "ID: %d\t Width: %u\t Height: %u\t Type: %s\t RcMode: %s",
                    idx, pstChnAttr->stVencAttr.u32PicWidth,
                    pstChnAttr->stVencAttr.u32PicHeight,
                    cCodecType, cRcMode);
                seq_printf(
                    m,
                    "\t EsBufQueueEn: %d\t bIsoSendFrmEn: %d",
                    pstChnAttr->stVencAttr.bEsBufQueueEn,
                    pstChnAttr->stVencAttr.bIsoSendFrmEn);
                seq_printf(
                    m,
                    "\t ByFrame: %s\t Sequence: %u\t LeftBytes: %u\t LeftFrm: %u",
                    pstChnAttr->stVencAttr.bByFrame ? "Y" :
                                        "N",
                    pstStream->u32Seq,
                    pchnStatus->u32LeftStreamBytes,
                    pchnStatus->u32LeftStreamFrames);
                seq_printf(
                    m,
                    "\t CurPacks: %u\t GopMode: %s\t Prio: %d\n\n",
                    pchnStatus->u32CurPacks, cGopMode,
                    pstChnParam->u32Priority);

                seq_puts(
                    m,
                    "-----VENC CHN ATTR 2-----------------------------------------------\n");
                seq_printf(
                    m,
                    "VeStr: Y\t SrcFr: %u\t TarFr: %u\t Timeref: %u\t PixFmt: %s",
                    u32SrcFrameRate, fr32DstFrameRate,
                    pstVFrame->time_ref, cPixelFormat);
                seq_printf(
                    m,
                    "\t PicAddr: 0x%llx\t WakeUpFrmCnt: %u\n",
                    pstVFrame->phyaddr[0],
                    pstChnParam->u32PollWakeUpFrmCnt);

// TODO: following info should be amended later
#if 0
                seq_puts(m, "-----VENC JPEGE ATTR ----------------------------------------------\n");
                seq_printf(m, "ID: %d\t RcvMode: %d\t MpfCnt: %d\t Mpf0Width: %d\t Mpf0Height: %d",
                    idx, 0, 0, 0, 0);
                seq_printf(m, "\t Mpf1Width: %d\t Mpf1Height: %d\n", 0, 0);

                seq_puts(m, "-----VENC CHN RECEIVE STAT-----------------------------------------\n");
                seq_printf(m, "ID: %d\t Start: %d\t StartEx: %d\t RecvLeft: %d\t EncLeft: %d",
                    idx, 0, 0, 0, 0);
                seq_puts(m, "\t JpegEncodeMode: NA\n");

                seq_puts(m, "-----VENC VPSS QUERY----------------------------------------------\n");
                seq_printf(m, "ID: %d\t Query: %d\t QueryOk: %d\t QueryFR: %d\t Invld: %d",
                    idx, 0, 0, 0, 0);
                seq_printf(m, "\t Full: %d\t VbFail: %d\t QueryFail: %d\t InfoErr: %d\t Stop: %d\n",
                    0, 0, 0, 0, 0);

                seq_puts(m, "-----VENC SEND1---------------------------------------------------\n");
                seq_printf(m, "ID: %d\t VpssSnd: %d\t VInfErr: %d\t OthrSnd: %d\t OInfErr: %d\t Send: %d",
                    idx, 0, 0, 0, 0, 0);
                seq_printf(m, "\t Stop: %d\t Full: %d\t CropErr: %d\t DrectSnd: %d\t SizeErr: %d\n",
                    0, 0, 0, 0, 0);

                seq_puts(m, "-----VENC SEND2--------------------------------------------------\n");
                seq_printf(m, "ID: %d\t SendVgs: %d\t StartOk: %d\t StartFail: %d\t IntOk: %d",
                    idx, 0, 0, 0, 0);
                seq_printf(m, "\t IntFail: %d\t SrcAdd: %d\t SrcSub: %d\t DestAdd: %d\t DestSub: %d\n",
                    0, 0, 0, 0, 0);

                seq_puts(m, "-----VENC PIC QUEUE STATE-----------------------------------------\n");
                seq_printf(m, "ID: %d\t Free: %d\t Busy: %d\t Vgs: %d\t BFrame: %d\n",
                    idx, 0, 0, 0, 0);

                seq_puts(m, "-----VENC DCF/MPF QUEUE STATE-----------------------------------------\n");
                seq_printf(m, "ID: %d\t ThumbFree: %d\t ThumbBusy: %d",
                    idx, 0, 0);
                seq_printf(m, "\t Mpf0Free: %d\t Mpf0Busy: %d\t Mpf1Free: %d\t Mpf1Busy: %d\n",
                    0, 0, 0, 0);

                seq_puts(m, "-----VENC CHNL INFO------------------------------------------------\n");
                seq_printf(m, "ID: %d\t Inq: %d\t InqOk: %d\t Start: %d\t StartOk: %d\t Config: %d",
                    idx, 0, 0, 0, 0, 0);
                seq_printf(m, "\t VencInt: %d\t ChaResLost: %d\t OverLoad: %d\t RingSkip: %d\t RcSkip: %d\n",
                    0, 0, 0, 0, 0);
#endif

                seq_puts(
                    m,
                    "-----VENC CROP INFO------------------------------------------------\n");
                seq_printf(
                    m,
                    "ID: %d\t CropEn: %s\t StartX: %d\t StartY: %d\t Width: %u\t Height: %u\n\n",
                    idx,
                    pstChnParam->stCropCfg.bEnable ? "Y" : "N",
                    pstChnParam->stCropCfg.stRect.x,
                    pstChnParam->stCropCfg.stRect.y,
                    pstChnParam->stCropCfg.stRect.width,
                    pstChnParam->stCropCfg.stRect.height);

                seq_puts(
                    m,
                    "-----ROI INFO-----------------------------------------------------\n");
                for (roiIdx = 0; roiIdx < 8; roiIdx++) {
                    if (handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].bEnable) {
                        seq_printf(
                            m,
                            "ID: %d\t Index: %u\t bRoiEn: %s\t bAbsQp: %s\t Qp: %d",
                            idx,
                            handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].u32Index,
                            handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].bEnable ?
                                      "Y" :
                                      "N",
                            handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].bAbsQp ?
                                      "Y" :
                                      "N",
                            handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].s32Qp);
                        seq_printf(
                            m,
                            "\t Width: %u\t Height: %u\t StartX: %d\t StartY: %d\n\n",
                            handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].stRect.width,
                            handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].stRect.height,
                            handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].stRect.x,
                            handle->chn_handle[idx]->pChnVars->stRoiAttr[roiIdx].stRect.y);
                    }
                }

#if 0
                seq_puts(m, "-----VENC STREAM STATE---------------------------------------------\n");
                seq_printf(m, "ID: %d\t FreeCnt: %d\t BusyCnt: %d\t UserCnt: %d\t UserGet: %d",
                    idx, 0, 0, 0, 0);
                seq_printf(m, "\t UserRls: %d\t GetTimes: %d\t Interval: %d\t FrameRate: %d\n",
                    0, 0, 0, 0);
#endif

                seq_puts(
                    m,
                    "-----VENC PTS STATE------------------------------------------------\n");
                seq_printf(
                    m,
                    "ID: %d\t RcvFirstFrmPts: %llu\t RcvFrmPts: %llu\n\n",
                    idx, 0LL, pstVFrame->pts);

                seq_puts(
                    m,
                    "-----VENC CHN PERFORMANCE------------------------------------------------\n");
                seq_printf(
                    m,
                    "ID: %d\t No.SendFramePerSec: %u\t No.EncFramePerSec: %u\t HwEncTime: %llu us\t MaxHwEncTime: %llu us\n\n",
                    idx, pstFPS->in_fps,
                    pstFPS->out_fps, pstFPS->hw_time, pstFPS->max_hw_time);

            }
        }
    }

    seq_puts(
        m,
        "\n----- Debug Level STATE ----------------------------------------\n");
    seq_printf(
        m,
        "VencDebugMask: 0x%X\t VencStartFrmIdx: %u\t VencEndFrmIdx: %u\t VencDumpPath: %s\t ",
        tVencDebugConfig.u32DbgMask, tVencDebugConfig.u32StartFrmIdx,
        tVencDebugConfig.u32EndFrmIdx, tVencDebugConfig.cDumpPath);
    seq_printf(m, "VencNoDataTimeout: %u\n",
           tVencDebugConfig.u32NoDataTimeout);

    return 0;
}

static int venc_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, venc_proc_show, PDE_DATA(inode));
}

static ssize_t
vcodec_proc_write_helper(const char __user *user_buf, size_t count,
             proc_debug_config_t *ptDebugConfig,
             char *pcProcInputdata, char *pcDbgPrefix,
             char *pcDbgStartFrmPrefix, char *pcDbgEndFrmPrefix,
             char *pcDbgDirPrefix, char *pcNoDataTimeoutPrefix)
{
    uint8_t u8DgbPrefixLen;
    uint8_t u8DgbStartFrmPrefixLen;
    uint8_t u8DgbEndFrmPrefixLen;
    uint8_t u8DgbDirPrefixLen;
    uint8_t u8NoDataTimeoutPrefixLen;

    if (!user_buf) {
        pr_err("no user buf input\n");
        return -EFAULT;
    }

    u8DgbPrefixLen = strlen(pcDbgPrefix);
    u8DgbStartFrmPrefixLen = strlen(pcDbgStartFrmPrefix);
    u8DgbEndFrmPrefixLen = strlen(pcDbgEndFrmPrefix);
    u8DgbDirPrefixLen = strlen(pcDbgDirPrefix);
    if (pcNoDataTimeoutPrefix)
        u8NoDataTimeoutPrefixLen = strlen(pcNoDataTimeoutPrefix);

    if (copy_from_user(pcProcInputdata, user_buf, count)) {
        pr_err("copy_from_user fail\n");
        return -EFAULT;
    }

    if (strncmp(pcProcInputdata, pcDbgPrefix, u8DgbPrefixLen) == 0) {
        if (kstrtouint(pcProcInputdata + u8DgbPrefixLen, 16,
                   &ptDebugConfig->u32DbgMask) != 0) {
            pr_err("fail to set debug level: 0x%s\n",
                   pcProcInputdata + u8DgbPrefixLen);
            return -EFAULT;
        }
    } else if (strncmp(pcProcInputdata, pcDbgStartFrmPrefix,
               u8DgbStartFrmPrefixLen) == 0) {
        if (kstrtouint(pcProcInputdata + u8DgbStartFrmPrefixLen, 10,
                   &ptDebugConfig->u32StartFrmIdx) != 0) {
            pr_err("fail to set start frame index: 0x%s\n",
                   pcProcInputdata + u8DgbStartFrmPrefixLen);
            return -EFAULT;
        }
    } else if (strncmp(pcProcInputdata, pcDbgEndFrmPrefix,
               u8DgbEndFrmPrefixLen) == 0) {
        if (kstrtouint(pcProcInputdata + u8DgbEndFrmPrefixLen, 10,
                   &ptDebugConfig->u32EndFrmIdx) != 0) {
            pr_err("fail to set end frame index: 0x%s\n",
                   pcProcInputdata + u8DgbEndFrmPrefixLen);
            return -EFAULT;
        }
    } else if (strncmp(pcProcInputdata, pcDbgDirPrefix,
               u8DgbDirPrefixLen) == 0) {
        if (strcpy(ptDebugConfig->cDumpPath,
               pcProcInputdata + u8DgbDirPrefixLen) == NULL) {
            pr_err("fail to set debug folder: 0x%s\n",
                   pcProcInputdata + u8DgbDirPrefixLen);
            return -EFAULT;
        }
    } else if (pcNoDataTimeoutPrefix &&
           strncmp(pcProcInputdata, pcNoDataTimeoutPrefix,
               u8NoDataTimeoutPrefixLen) == 0) {
        if (kstrtouint(pcProcInputdata + u8NoDataTimeoutPrefixLen, 10,
                   &ptDebugConfig->u32NoDataTimeout) != 0) {
            pr_err("fail to set no data threshold: 0x%s\n",
                   pcProcInputdata + u8NoDataTimeoutPrefixLen);
            return -EFAULT;
        }
    } else {
        pr_err("can't handle user input %s\n", pcProcInputdata);
        return -EFAULT;
    }

    return count;
}

static ssize_t venc_proc_write(struct file *file, const char __user *user_buf,
                   size_t count, loff_t *ppos)
{
    /* debug_level list, please refer to: comm_venc.h
     * DRV_VENC_MASK_ERR    0x1
     * DRV_VENC_MASK_WARN    0x2
     * DRV_VENC_MASK_INFO    0x4
     * DRV_VENC_MASK_FLOW    0x8
     * DRV_VENC_MASK_DBG    0x10
     * DRV_VENC_MASK_BS        0x100
     * DRV_VENC_MASK_SRC    0x200
     * DRV_VENC_MASK_API    0x400
     * DRV_VENC_MASK_SYNC    0x800
     * DRV_VENC_MASK_PERF    0x1000
     * DRV_VENC_MASK_CFG    0x2000
     * DRV_VENC_MASK_FRC    0x4000
     * DRV_VENC_MASK_BIND    0x8000
     * DRV_VENC_MASK_TRACE    0x10000
     * DRV_VENC_MASK_DUMP_YUV    0x100000
     * DRV_VENC_MASK_DUMP_BS    0x200000
     */
    char cProcInputdata[MAX_PROC_STR_SIZE] = { '\0' };
    char cVencDbgPrefix[] = "venc=0x"; // venc=debug_levle
    char cVencDbgStartFrmPrefix[] = "venc_sfn="; // venc_sfn=frame_idx_begin
    char cVencDbgEndFrmPrefix[] = "venc_efn="; // venc_efn=frame_idx_end
    char cVencDbgDirPrefix[] =
        "venc_dir="; // venc_dir=your_preference_directory
    char cVencDbgNoDataTimeoutPrefix[] =
        "venc_no_inputdata_timeout="; // venc_no_inputdata_timeout=timeout
    proc_debug_config_t *pVencDebugConfig =
        (proc_debug_config_t *)&tVencDebugConfig;

    return vcodec_proc_write_helper(user_buf, count, pVencDebugConfig,
                    cProcInputdata, cVencDbgPrefix,
                    cVencDbgStartFrmPrefix,
                    cVencDbgEndFrmPrefix, cVencDbgDirPrefix,
                    cVencDbgNoDataTimeoutPrefix);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops venc_proc_fops = {
    .proc_open = venc_proc_open,
    .proc_read = seq_read,
    .proc_write = venc_proc_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations venc_proc_fops = {
    .owner = THIS_MODULE,
    .open = venc_proc_open,
    .read = seq_read,
    .write = venc_proc_write,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

int venc_proc_init(struct device *dev)
{
    if (proc_create_data(VENC_PROC_NAME, VIDEO_PROC_PERMS,
                 VIDEO_PROC_PARENT, &venc_proc_fops, dev) == NULL) {
        dev_err(dev, "ERROR! /proc/%s create fail\n", VENC_PROC_NAME);
        remove_proc_entry(VENC_PROC_NAME, NULL);
        return -1;
    }
    return 0;
}

int venc_proc_deinit(void)
{
    remove_proc_entry(VENC_PROC_NAME, NULL);
    return 0;
}

static int h265e_proc_show(struct seq_file *m, void *v)
{
    int idx = 0;
    drv_venc_param_mod_s *pVencModParam;

    seq_printf(m, "Module: [H265E] System Build Time [%s]\n", UTS_VERSION);

    if (handle == NULL)
        return 0;

    pVencModParam = &handle->ModParam;

    seq_puts(
        m,
        "-----MODULE PARAM-------------------------------------------------\n");
    seq_printf(m, "OnePack: %u\t H265eVBSource: %d\t PowerSaveEn: %u",
           pVencModParam->stH265eModParam.u32OneStreamBuffer,
           pVencModParam->stH265eModParam.enH265eVBSource,
           pVencModParam->stH265eModParam.u32H265ePowerSaveEn);
    seq_printf(
        m,
        "\t MiniBufMode: %u\t bQpHstgrmEn: %u\t UserDataMaxLen: %u\n",
        pVencModParam->stH265eModParam.u32H265eMiniBufMode,
        pVencModParam->stH265eModParam.bQpHstgrmEn,
        pVencModParam->stH265eModParam.u32UserDataMaxLen);
    seq_printf(m,
           "SingleEsBuf: %u\t SingleEsBufSize: %u\t RefreshType: %u\n",
           pVencModParam->stH265eModParam.bSingleEsBuf,
           pVencModParam->stH265eModParam.u32SingleEsBufSize,
           pVencModParam->stH265eModParam.enRefreshType);

    for (idx = 0; idx < MaxVencChnNum; idx++) {
        if (handle->chn_handle[idx] != NULL &&
            handle->chn_handle[idx]->pChnAttr->stVencAttr.enType ==
                PT_H265) {
            char cGopMode[10] = { '\0' };
            venc_chn_attr_s *pstChnAttr =
                handle->chn_handle[idx]->pChnAttr;
            venc_chn_param_s *pstChnParam =
                &handle->chn_handle[idx]->pChnVars->stChnParam;
            venc_ref_param_s *pstRefParam =
                &handle->chn_handle[idx]->refParam;

            get_gop_mode_str(pstChnAttr->stGopAttr.enGopMode,
                      cGopMode);

            seq_puts(
                m,
                "-----CHN ATTR-----------------------------------------------------\n");
            seq_printf(
                m,
                "ID: %d\t MaxWidth: %u\t MaxHeight: %u\t Width: %u\t Height: %u",
                idx, pstChnAttr->stVencAttr.u32MaxPicWidth,
                pstChnAttr->stVencAttr.u32MaxPicHeight,
                pstChnAttr->stVencAttr.u32PicWidth,
                pstChnAttr->stVencAttr.u32PicHeight);
            seq_printf(
                m,
                "\t C2GEn: %d\t BufSize: %u\t ByFrame: %d\t GopMode: %s\t MaxStrCnt: %u\n",
                pstChnParam->bColor2Grey,
                pstChnAttr->stVencAttr.u32BufSize,
                pstChnAttr->stVencAttr.bByFrame, cGopMode,
                pstChnParam->u32MaxStrmCnt);

            seq_puts(
                m,
                "-----RefParam INFO---------------------------------------------\n");
            seq_printf(
                m,
                "ID: %d\t EnPred: %s\t Base: %u\t Enhance: %u\t RcnRefShareBuf: %u\n",
                idx, pstRefParam->bEnablePred ? "Y" : "N",
                pstRefParam->u32Base, pstRefParam->u32Enhance,
                pstChnAttr->stVencAttr.stAttrH265e
                    .bRcnRefShareBuf);

            seq_puts(
                m,
                "-----Syntax INFO---------------------------------------------\n");
            seq_printf(m, "ID: %d\t Profile: Main\n", idx);
        }
    }
    return 0;
}

static int h265e_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, h265e_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops h265e_proc_fops = {
    .proc_open = h265e_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations h265e_proc_fops = {
    .owner = THIS_MODULE,
    .open = h265e_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

static int codecinst_proc_show(struct seq_file *m, void *v)
{
    int idx = 0;
    venc_chn_context *pChnHandle;
    venc_enc_ctx *pEncCtx;
    payload_type_e enType;

    seq_printf(m, "Module: [CodecInst] System Build Time [%s]\n", UTS_VERSION);

    if (handle == NULL)
        return 0;

    for (idx = 0; idx < MaxVencChnNum; idx++) {
        pChnHandle = handle->chn_handle[idx];
        if (pChnHandle != NULL) {
            pEncCtx = &pChnHandle->encCtx;
            enType = pChnHandle->pChnAttr->stVencAttr.enType;

            if (pEncCtx->base.ioctl && (enType == PT_H264 || enType == PT_H265)) {
                pEncCtx->base.ioctl(pEncCtx, DRV_H26X_OP_GET_CHN_INFO, (void *)m);
            } else if (pEncCtx->base.ioctl &&
                   (enType == PT_JPEG || enType == PT_MJPEG)) {
                pEncCtx->base.ioctl(pEncCtx, DRV_JPEG_OP_SHOW_CHN_INFO, (void *)m);
            }
        }
    }
    return 0;
}


static int codecinst_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, codecinst_proc_show, PDE_DATA(inode));
}



#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops codecinst_proc_fops = {
    .proc_open = codecinst_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations codecinst_proc_fops = {
    .owner = THIS_MODULE,
    .open = codecinst_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif


int codecinst_proc_init(struct device *dev)
{
    if (proc_create_data(CODEC_PROC_NAME, VIDEO_PROC_PERMS,
                 VIDEO_PROC_PARENT, &codecinst_proc_fops,
                 dev) == NULL) {
        dev_err(dev, "ERROR! /proc/%s create fail\n", H265E_PROC_NAME);
        remove_proc_entry(CODEC_PROC_NAME, NULL);
        return -1;
    }
    return 0;
}

int codecinst_proc_deinit(void)
{
    remove_proc_entry(CODEC_PROC_NAME, NULL);
    return 0;
}

int h265e_proc_init(struct device *dev)
{
    if (proc_create_data(H265E_PROC_NAME, VIDEO_PROC_PERMS,
                 VIDEO_PROC_PARENT, &h265e_proc_fops,
                 dev) == NULL) {
        dev_err(dev, "ERROR! /proc/%s create fail\n", H265E_PROC_NAME);
        remove_proc_entry(H265E_PROC_NAME, NULL);
        return -1;
    }
    return 0;
}

int h265e_proc_deinit(void)
{
    remove_proc_entry(H265E_PROC_NAME, NULL);
    return 0;
}

static int h264e_proc_show(struct seq_file *m, void *v)
{
    static const char *const prifle[] = { "Base", "Main", "High", "Svc-t",
                          "Err" };
    int idx = 0;

    drv_venc_param_mod_s *pVencModParam;
    unsigned int u32Profile;

    seq_printf(m, "Module: [H264E] System Build Time [%s]\n", UTS_VERSION);

    if (handle == NULL)
        return 0;

    pVencModParam = &handle->ModParam;

    seq_puts(
        m,
        "-----MODULE PARAM-------------------------------------------------\n");
    seq_printf(m, "OnePack: %u\t H264eVBSource: %d\t PowerSaveEn: %u",
           pVencModParam->stH264eModParam.u32OneStreamBuffer,
           pVencModParam->stH264eModParam.enH264eVBSource,
           pVencModParam->stH264eModParam.u32H264ePowerSaveEn);
    seq_printf(m,
           "\t MiniBufMode: %u\t QpHstgrmEn: %u\t UserDataMaxLen: %u\n",
           pVencModParam->stH264eModParam.u32H264eMiniBufMode,
           pVencModParam->stH264eModParam.bQpHstgrmEn,
           pVencModParam->stH264eModParam.u32UserDataMaxLen);
    seq_printf(m, "SingleEsBuf: %u\t SingleEsBufSize: %u\n",
           pVencModParam->stH264eModParam.bSingleEsBuf,
           pVencModParam->stH264eModParam.u32SingleEsBufSize);

    for (idx = 0; idx < MaxVencChnNum; idx++) {
        if (handle->chn_handle[idx] != NULL &&
            handle->chn_handle[idx]->pChnAttr->stVencAttr.enType ==
                PT_H264) {
            char cGopMode[10] = { '\0' };
            venc_chn_attr_s *pstChnAttr =
                handle->chn_handle[idx]->pChnAttr;
            venc_chn_param_s *pstChnParam =
                &handle->chn_handle[idx]->pChnVars->stChnParam;
            venc_ref_param_s *pstRefParam =
                &handle->chn_handle[idx]->refParam;

            get_gop_mode_str(pstChnAttr->stGopAttr.enGopMode,
                      cGopMode);

            seq_puts(
                m,
                "-----CHN ATTR-----------------------------------------------------\n");
            seq_printf(
                m,
                "ID: %d\t MaxWidth: %u\t MaxHeight: %u\t Width: %u\t Height: %u",
                idx, pstChnAttr->stVencAttr.u32MaxPicWidth,
                pstChnAttr->stVencAttr.u32MaxPicHeight,
                pstChnAttr->stVencAttr.u32PicWidth,
                pstChnAttr->stVencAttr.u32PicHeight);
            seq_printf(
                m,
                "\t C2GEn: %d\t BufSize: %u\t ByFrame: %d\t GopMode: %s\t MaxStrCnt: %u\n",
                pstChnParam->bColor2Grey,
                pstChnAttr->stVencAttr.u32BufSize,
                pstChnAttr->stVencAttr.bByFrame, cGopMode,
                pstChnParam->u32MaxStrmCnt);

            seq_puts(
                m,
                "-----RefParam INFO---------------------------------------------\n");
            seq_printf(
                m,
                "ID: %d\t EnPred: %s\t Base: %u\t Enhance: %u\t RcnRefShareBuf: %u\n",
                idx, pstRefParam->bEnablePred ? "Y" : "N",
                pstRefParam->u32Base, pstRefParam->u32Enhance,
                pstChnAttr->stVencAttr.stAttrH264e
                    .bRcnRefShareBuf);

            seq_puts(
                m,
                "-----Syntax INFO---------------------------------------------\n");
            u32Profile = pstChnAttr->stVencAttr.u32Profile;
            if (u32Profile >= 4)
                u32Profile = 4;

            seq_printf(m, "ID: %d\t Profile: %s\n", idx,
                   prifle[u32Profile]);
        }
    }
    return 0;
}

static int h264e_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, h264e_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops h264e_proc_fops = {
    .proc_open = h264e_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations h264e_proc_fops = {
    .owner = THIS_MODULE,
    .open = h264e_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

int h264e_proc_init(struct device *dev)
{
    if (proc_create_data(H264E_PROC_NAME, VIDEO_PROC_PERMS,
                 VIDEO_PROC_PARENT, &h264e_proc_fops,
                 dev) == NULL) {
        dev_err(dev, "ERROR! /proc/%s create fail\n", H264E_PROC_NAME);
        remove_proc_entry(H264E_PROC_NAME, NULL);
        return -1;
    }
    return 0;
}

int h264e_proc_deinit(void)
{
    remove_proc_entry(H264E_PROC_NAME, NULL);
    return 0;
}

static int jpege_proc_show(struct seq_file *m, void *v)
{
    int idx = 0;
    drv_venc_param_mod_s *pVencModParam;

    seq_printf(m, "Module: [JPEGE] System Build Time [%s]\n", UTS_VERSION);

    if (handle == NULL)
        return 0;

    pVencModParam = &handle->ModParam;

    seq_puts(
        m,
        "-----MODULE PARAM-------------------------------------------------\n");
    seq_printf(m, "OnePack: %u\t JpegeMiniBufMode: %d",
           pVencModParam->stJpegeModParam.u32OneStreamBuffer,
           pVencModParam->stJpegeModParam.u32JpegeMiniBufMode);
    seq_printf(m, "\t JpegClearStreamBuf: %u\t JpegeDeringMode: %u\n",
           pVencModParam->stJpegeModParam.u32JpegClearStreamBuf, 0);
    seq_printf(m,
           "SingleEsBuf: %u\t SingleEsBufSize: %u\t JpegeFormat: %u\n",
           pVencModParam->stJpegeModParam.bSingleEsBuf,
           pVencModParam->stJpegeModParam.u32SingleEsBufSize,
           pVencModParam->stJpegeModParam.enJpegeFormat);
    seq_printf(
        m,
        "JpegMarkerOrder: %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d\n",
        pVencModParam->stJpegeModParam.JpegMarkerOrder[0],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[1],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[2],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[3],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[4],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[5],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[6],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[7],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[8],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[9],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[10],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[11],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[12],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[13],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[14],
        pVencModParam->stJpegeModParam.JpegMarkerOrder[15]);

    for (idx = 0; idx < MaxVencChnNum; idx++) {
        if (handle->chn_handle[idx] != NULL &&
            (handle->chn_handle[idx]->pChnAttr->stVencAttr.enType ==
                 PT_JPEG ||
             handle->chn_handle[idx]->pChnAttr->stVencAttr.enType ==
                 PT_MJPEG)) {
            char cGopMode[10] = { '\0' };
            char cPicType[8] = { '\0' };
            unsigned int u32Qfactor = 0;

            venc_chn_attr_s *pstChnAttr =
                handle->chn_handle[idx]->pChnAttr;
            venc_chn_param_s *pstChnParam =
                &handle->chn_handle[idx]->pChnVars->stChnParam;
            video_frame_s *pstVFrame =
                &handle->chn_handle[idx]
                     ->pChnVars->stFrameInfo.video_frame;

            get_gop_mode_str(pstChnAttr->stGopAttr.enGopMode,
                      cGopMode);
            get_pixel_format_str(pstVFrame->pixel_format, cPicType);

            if (pstChnAttr->stVencAttr.enType == PT_JPEG) {
                u32Qfactor = handle->chn_handle[idx]
                             ->pChnVars->stJpegParam
                             .u32Qfactor;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_MJPEGFIXQP) {
                u32Qfactor = pstChnAttr->stRcAttr.stMjpegFixQp
                             .u32Qfactor;
            }

            seq_puts(
                m,
                "-----CHN ATTR-----------------------------------------------------\n");
            seq_printf(
                m,
                "ID: %d\t bMjpeg: %s\t PicType: %s\t MaxWidth: %u\t MaxHeight: %u",
                idx,
                pstChnAttr->stVencAttr.enType == PT_MJPEG ?
                          "Y" :
                          "N",
                cPicType, pstChnAttr->stVencAttr.u32MaxPicWidth,
                pstChnAttr->stVencAttr.u32MaxPicHeight);
            seq_printf(
                m,
                "\t Width: %u\t Height: %u\t BufSize: %u\t ByFrm: %d",
                pstChnAttr->stVencAttr.u32PicWidth,
                pstChnAttr->stVencAttr.u32PicHeight,
                pstChnAttr->stVencAttr.u32BufSize,
                pstChnAttr->stVencAttr.bByFrame);
            seq_printf(
                m,
                "\t MCU: %d\t Qfactor: %u\t C2GEn: %d\t DcfEn: %d\n",
                1, u32Qfactor, pstChnParam->bColor2Grey, 0);
        }
    }
    return 0;
}

static int jpege_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, jpege_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops jpege_proc_fops = {
    .proc_open = jpege_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations jpege_proc_fops = {
    .owner = THIS_MODULE,
    .open = jpege_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

int jpege_proc_init(struct device *dev)
{
    if (proc_create_data(JPEGE_PROC_NAME, VIDEO_PROC_PERMS,
                 VIDEO_PROC_PARENT, &jpege_proc_fops,
                 dev) == NULL) {
        dev_err(dev, "ERROR! /proc/%s create fail\n", JPEGE_PROC_NAME);
        remove_proc_entry(JPEGE_PROC_NAME, NULL);
        return -1;
    }
    return 0;
}

int jpege_proc_deinit(void)
{
    remove_proc_entry(JPEGE_PROC_NAME, NULL);
    return 0;
}

static int rc_proc_show(struct seq_file *m, void *v)
{
    int idx = 0;

    seq_printf(m, "Module: [RC] System Build Time [%s]\n", UTS_VERSION);

    if (handle == NULL)
        return 0;

    for (idx = 0; idx < MaxVencChnNum; idx++) {
        if (handle->chn_handle[idx] != NULL) {
            char cCodecType[16] = { '\0' };
            char cRcMode[8] = { '\0' };
            char cQpMapMode[8] = { '\0' };
            char cGopMode[10] = { '\0' };
            unsigned int u32Gop = 0;
            unsigned int u32StatTime = 0;
            unsigned int u32SrcFrameRate = 0;
            DRV_FR32 fr32DstFrameRate = 0;
            unsigned int u32BitRate = 0;
            unsigned int u32IQp = 0;
            unsigned int u32PQp = 0;
            unsigned int u32BQp = 0;
            unsigned int u32Qfactor = 0;
            unsigned int u32MinIprop = 0;
            unsigned int u32MaxIprop = 0;
            unsigned int u32MaxQp = 0;
            unsigned int u32MinQp = 0;
            unsigned int u32MaxIQp = 0;
            unsigned int u32MinIQp = 0;
            unsigned char bQpMapEn = 0;
            unsigned char bVariFpsEn = 0;
            int s32IPQpDelta = 0;
            unsigned int u32SPInterval = 0;
            int s32SPQpDelta = 0;
            unsigned int u32BgInterval = 0;
            int s32BgQpDelta = 0;
            int s32ViQpDelta = 0;
            unsigned int u32BFrmNum = 0;
            int s32BQpDelta = 0;
            int s32MaxReEncodeTimes = 0;
            int s32ChangePos = 0;
            unsigned int u32MaxQfactor = 0;
            unsigned int u32MinQfactor = 0;
            venc_rc_qpmap_mode_e enQpMapMode =
                VENC_RC_QPMAP_MODE_BUTT + 1;
            int s32MinStillPercent = 0;
            unsigned int u32MaxStillQP = 0;
            unsigned int u32MinStillPSNR = 0;
            unsigned int u32MinQpDelta = 0;
            unsigned int u32MotionSensitivity = 0;
            int s32AvbrFrmLostOpen = 0;
            int s32AvbrFrmGap = 0;

            venc_chn_attr_s *pstChnAttr =
                handle->chn_handle[idx]->pChnAttr;
            venc_rc_param_s *pRcParam =
                &handle->chn_handle[idx]->rcParam;
            venc_superframe_cfg_s *pstSuperFrmParam =
                &handle->chn_handle[idx]
                     ->pChnVars->stSuperFrmParam;

            get_codec_type_str(pstChnAttr->stVencAttr.enType,
                    cCodecType);
            get_gop_mode_str(pstChnAttr->stGopAttr.enGopMode,
                      cGopMode);
            get_framerate(pstChnAttr, &u32SrcFrameRate,
                     &fr32DstFrameRate);

            switch (pstChnAttr->stGopAttr.enGopMode) {
            case VENC_GOPMODE_NORMALP:
                s32IPQpDelta = pstChnAttr->stGopAttr.stNormalP
                               .s32IPQpDelta;
                break;
            case VENC_GOPMODE_DUALP:
                u32SPInterval = pstChnAttr->stGopAttr.stDualP
                            .u32SPInterval;
                s32SPQpDelta = pstChnAttr->stGopAttr.stDualP
                               .s32SPQpDelta;
                s32IPQpDelta = pstChnAttr->stGopAttr.stDualP
                               .s32IPQpDelta;
                break;
            case VENC_GOPMODE_SMARTP:
                u32BgInterval = pstChnAttr->stGopAttr.stSmartP
                            .u32BgInterval;
                s32BgQpDelta = pstChnAttr->stGopAttr.stSmartP
                               .s32BgQpDelta;
                s32ViQpDelta = pstChnAttr->stGopAttr.stSmartP
                               .s32ViQpDelta;
                break;
            case VENC_GOPMODE_ADVSMARTP:
                u32BgInterval =
                    pstChnAttr->stGopAttr.stAdvSmartP
                        .u32BgInterval;
                s32BgQpDelta = pstChnAttr->stGopAttr.stAdvSmartP
                               .s32BgQpDelta;
                s32ViQpDelta = pstChnAttr->stGopAttr.stAdvSmartP
                               .s32ViQpDelta;
                break;
            case VENC_GOPMODE_BIPREDB:
                u32BFrmNum = pstChnAttr->stGopAttr.stBipredB
                             .u32BFrmNum;
                s32BQpDelta = pstChnAttr->stGopAttr.stBipredB
                              .s32BQpDelta;
                s32IPQpDelta = pstChnAttr->stGopAttr.stBipredB
                               .s32IPQpDelta;
                break;
            default:
                break;
            }

            if (pstChnAttr->stRcAttr.enRcMode ==
                VENC_RC_MODE_H264CBR) {
                strcpy(cRcMode, "CBR");
                u32Gop = pstChnAttr->stRcAttr.stH264Cbr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH264Cbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH264Cbr
                             .u32BitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stH264Cbr
                             .bVariFpsEn;

                u32MinIprop =
                    pRcParam->stParamH264Cbr.u32MinIprop;
                u32MaxIprop =
                    pRcParam->stParamH264Cbr.u32MaxIprop;
                u32MaxQp = pRcParam->stParamH264Cbr.u32MaxQp;
                u32MinQp = pRcParam->stParamH264Cbr.u32MinQp;
                u32MaxIQp = pRcParam->stParamH264Cbr.u32MaxIQp;
                u32MinIQp = pRcParam->stParamH264Cbr.u32MinIQp;
                s32MaxReEncodeTimes =
                    pRcParam->stParamH264Cbr
                        .s32MaxReEncodeTimes;
                bQpMapEn = pRcParam->stParamH264Cbr.bQpMapEn;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H265CBR) {
                strcpy(cRcMode, "CBR");
                u32Gop = pstChnAttr->stRcAttr.stH265Cbr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH265Cbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH265Cbr
                             .u32BitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stH265Cbr
                             .bVariFpsEn;

                u32MinIprop =
                    pRcParam->stParamH265Cbr.u32MinIprop;
                u32MaxIprop =
                    pRcParam->stParamH265Cbr.u32MaxIprop;
                u32MaxQp = pRcParam->stParamH265Cbr.u32MaxQp;
                u32MinQp = pRcParam->stParamH265Cbr.u32MinQp;
                u32MaxIQp = pRcParam->stParamH265Cbr.u32MaxIQp;
                u32MinIQp = pRcParam->stParamH265Cbr.u32MinIQp;
                s32MaxReEncodeTimes =
                    pRcParam->stParamH265Cbr
                        .s32MaxReEncodeTimes;
                bQpMapEn = pRcParam->stParamH265Cbr.bQpMapEn;
                enQpMapMode =
                    pRcParam->stParamH265Cbr.enQpMapMode;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_MJPEGCBR) {
                strcpy(cRcMode, "CBR");
                u32StatTime = pstChnAttr->stRcAttr.stMjpegCbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stMjpegCbr
                             .u32BitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stMjpegCbr
                             .bVariFpsEn;

                u32MaxQfactor =
                    pRcParam->stParamMjpegCbr.u32MaxQfactor;
                u32MinQfactor =
                    pRcParam->stParamMjpegCbr.u32MinQfactor;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H264VBR) {
                strcpy(cRcMode, "VBR");
                u32Gop = pstChnAttr->stRcAttr.stH264Vbr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH264Vbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH264Vbr
                             .u32MaxBitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stH264Vbr
                             .bVariFpsEn;

                s32ChangePos =
                    pRcParam->stParamH264Vbr.s32ChangePos;
                u32MinIprop =
                    pRcParam->stParamH264Vbr.u32MinIprop;
                u32MaxIprop =
                    pRcParam->stParamH264Vbr.u32MaxIprop;
                u32MaxQp = pRcParam->stParamH264Vbr.u32MaxQp;
                u32MinQp = pRcParam->stParamH264Vbr.u32MinQp;
                u32MaxIQp = pRcParam->stParamH264Vbr.u32MaxIQp;
                u32MinIQp = pRcParam->stParamH264Vbr.u32MinIQp;
                bQpMapEn = pRcParam->stParamH264Vbr.bQpMapEn;
                s32MaxReEncodeTimes =
                    pRcParam->stParamH264Vbr
                        .s32MaxReEncodeTimes;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H265VBR) {
                strcpy(cRcMode, "VBR");
                u32Gop = pstChnAttr->stRcAttr.stH265Vbr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH265Vbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH265Vbr
                             .u32MaxBitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stH265Vbr
                             .bVariFpsEn;

                s32ChangePos =
                    pRcParam->stParamH265Vbr.s32ChangePos;
                u32MinIprop =
                    pRcParam->stParamH265Vbr.u32MinIprop;
                u32MaxIprop =
                    pRcParam->stParamH265Vbr.u32MaxIprop;
                u32MaxQp = pRcParam->stParamH265Vbr.u32MaxQp;
                u32MinQp = pRcParam->stParamH265Vbr.u32MinQp;
                u32MaxIQp = pRcParam->stParamH265Vbr.u32MaxIQp;
                u32MinIQp = pRcParam->stParamH265Vbr.u32MinIQp;
                bQpMapEn = pRcParam->stParamH265Vbr.bQpMapEn;
                s32MaxReEncodeTimes =
                    pRcParam->stParamH265Vbr
                        .s32MaxReEncodeTimes;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_MJPEGVBR) {
                strcpy(cRcMode, "VBR");
                u32StatTime = pstChnAttr->stRcAttr.stMjpegVbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stMjpegVbr
                             .u32MaxBitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stMjpegVbr
                             .bVariFpsEn;

                s32ChangePos =
                    pRcParam->stParamMjpegVbr.s32ChangePos;
                u32MaxQfactor =
                    pRcParam->stParamMjpegVbr.u32MaxQfactor;
                u32MinQfactor =
                    pRcParam->stParamMjpegVbr.u32MinQfactor;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H264FIXQP) {
                strcpy(cRcMode, "FIXQP");
                u32Gop =
                    pstChnAttr->stRcAttr.stH264FixQp.u32Gop;
                u32IQp =
                    pstChnAttr->stRcAttr.stH264FixQp.u32IQp;
                u32PQp =
                    pstChnAttr->stRcAttr.stH264FixQp.u32PQp;
                u32BQp =
                    pstChnAttr->stRcAttr.stH264FixQp.u32BQp;
                bVariFpsEn = pstChnAttr->stRcAttr.stH264FixQp
                             .bVariFpsEn;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H265FIXQP) {
                strcpy(cRcMode, "FIXQP");
                u32Gop =
                    pstChnAttr->stRcAttr.stH265FixQp.u32Gop;
                u32IQp =
                    pstChnAttr->stRcAttr.stH265FixQp.u32IQp;
                u32PQp =
                    pstChnAttr->stRcAttr.stH265FixQp.u32PQp;
                u32BQp =
                    pstChnAttr->stRcAttr.stH265FixQp.u32BQp;
                bVariFpsEn = pstChnAttr->stRcAttr.stH265FixQp
                             .bVariFpsEn;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_MJPEGFIXQP) {
                strcpy(cRcMode, "FIXQP");
                u32Qfactor = pstChnAttr->stRcAttr.stMjpegFixQp
                             .u32Qfactor;
                bVariFpsEn = pstChnAttr->stRcAttr.stMjpegFixQp
                             .bVariFpsEn;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H264AVBR) {
                strcpy(cRcMode, "AVBR");
                u32Gop = pstChnAttr->stRcAttr.stH264AVbr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH264AVbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH264AVbr
                             .u32MaxBitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stH264AVbr
                             .bVariFpsEn;

                s32ChangePos =
                    pRcParam->stParamH264AVbr.s32ChangePos;
                u32MinIprop =
                    pRcParam->stParamH264AVbr.u32MinIprop;
                u32MaxIprop =
                    pRcParam->stParamH264AVbr.u32MaxIprop;
                s32MinStillPercent =
                    pRcParam->stParamH264AVbr
                        .s32MinStillPercent;
                u32MaxStillQP =
                    pRcParam->stParamH264AVbr.u32MaxStillQP;
                u32MinStillPSNR = pRcParam->stParamH264AVbr
                              .u32MinStillPSNR;
                u32MaxQp = pRcParam->stParamH264AVbr.u32MaxQp;
                u32MinQp = pRcParam->stParamH264AVbr.u32MinQp;
                u32MaxIQp = pRcParam->stParamH264AVbr.u32MaxIQp;
                u32MinIQp = pRcParam->stParamH264AVbr.u32MinIQp;
                u32MinQpDelta =
                    pRcParam->stParamH264AVbr.u32MinQpDelta;
                u32MotionSensitivity =
                    pRcParam->stParamH264AVbr
                        .u32MotionSensitivity;
                s32AvbrFrmLostOpen =
                    pRcParam->stParamH264AVbr
                        .s32AvbrFrmLostOpen;
                s32AvbrFrmGap =
                    pRcParam->stParamH264AVbr.s32AvbrFrmGap;
                u32MinStillPSNR = pRcParam->stParamH264AVbr
                              .s32AvbrPureStillThr;
                bQpMapEn = pRcParam->stParamH264AVbr.bQpMapEn;
                s32MaxReEncodeTimes =
                    pRcParam->stParamH264AVbr
                        .s32MaxReEncodeTimes;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H265AVBR) {
                strcpy(cRcMode, "AVBR");
                u32Gop = pstChnAttr->stRcAttr.stH265AVbr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH265AVbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH265AVbr
                             .u32MaxBitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stH265AVbr
                             .bVariFpsEn;

                s32ChangePos =
                    pRcParam->stParamH265AVbr.s32ChangePos;
                u32MinIprop =
                    pRcParam->stParamH265AVbr.u32MinIprop;
                u32MaxIprop =
                    pRcParam->stParamH265AVbr.u32MaxIprop;
                s32MinStillPercent =
                    pRcParam->stParamH265AVbr
                        .s32MinStillPercent;
                u32MaxStillQP =
                    pRcParam->stParamH265AVbr.u32MaxStillQP;
                u32MinStillPSNR = pRcParam->stParamH265AVbr
                              .u32MinStillPSNR;
                u32MaxQp = pRcParam->stParamH265AVbr.u32MaxQp;
                u32MinQp = pRcParam->stParamH265AVbr.u32MinQp;
                u32MaxIQp = pRcParam->stParamH265AVbr.u32MaxIQp;
                u32MinIQp = pRcParam->stParamH265AVbr.u32MinIQp;
                u32MinQpDelta =
                    pRcParam->stParamH265AVbr.u32MinQpDelta;
                u32MotionSensitivity =
                    pRcParam->stParamH265AVbr
                        .u32MotionSensitivity;
                s32AvbrFrmLostOpen =
                    pRcParam->stParamH265AVbr
                        .s32AvbrFrmLostOpen;
                s32AvbrFrmGap =
                    pRcParam->stParamH265AVbr.s32AvbrFrmGap;
                u32MinStillPSNR = pRcParam->stParamH265AVbr
                              .s32AvbrPureStillThr;
                bQpMapEn = pRcParam->stParamH265AVbr.bQpMapEn;
                s32MaxReEncodeTimes =
                    pRcParam->stParamH265AVbr
                        .s32MaxReEncodeTimes;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H264QVBR) {
                strcpy(cRcMode, "QVBR");
                u32Gop = pstChnAttr->stRcAttr.stH264QVbr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH264QVbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH264QVbr
                             .u32TargetBitRate;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H265QVBR) {
                strcpy(cRcMode, "QVBR");
                u32Gop = pstChnAttr->stRcAttr.stH265QVbr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH265QVbr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH265QVbr
                             .u32TargetBitRate;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H264QPMAP) {
                strcpy(cRcMode, "QPMAP");
                u32Gop =
                    pstChnAttr->stRcAttr.stH264QpMap.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH264QpMap
                              .u32StatTime;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H265QPMAP) {
                strcpy(cRcMode, "QPMAP");
                u32Gop =
                    pstChnAttr->stRcAttr.stH265QpMap.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH265QpMap
                              .u32StatTime;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H264UBR) {
                strcpy(cRcMode, "UBR");
                u32Gop = pstChnAttr->stRcAttr.stH264Ubr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH264Ubr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH264Ubr
                             .u32BitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stH264Ubr
                             .bVariFpsEn;

                u32MinIprop =
                    pRcParam->stParamH264Ubr.u32MinIprop;
                u32MaxIprop =
                    pRcParam->stParamH264Ubr.u32MaxIprop;
                u32MaxQp = pRcParam->stParamH264Ubr.u32MaxQp;
                u32MinQp = pRcParam->stParamH264Ubr.u32MinQp;
                u32MaxIQp = pRcParam->stParamH264Ubr.u32MaxIQp;
                u32MinIQp = pRcParam->stParamH264Ubr.u32MinIQp;
                s32MaxReEncodeTimes =
                    pRcParam->stParamH264Ubr
                        .s32MaxReEncodeTimes;
                bQpMapEn = pRcParam->stParamH264Ubr.bQpMapEn;
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                   VENC_RC_MODE_H265UBR) {
                strcpy(cRcMode, "UBR");
                u32Gop = pstChnAttr->stRcAttr.stH265Ubr.u32Gop;
                u32StatTime = pstChnAttr->stRcAttr.stH265Ubr
                              .u32StatTime;
                u32BitRate = pstChnAttr->stRcAttr.stH265Ubr
                             .u32BitRate;
                bVariFpsEn = pstChnAttr->stRcAttr.stH265Ubr
                             .bVariFpsEn;

                u32MinIprop =
                    pRcParam->stParamH265Ubr.u32MinIprop;
                u32MaxIprop =
                    pRcParam->stParamH265Ubr.u32MaxIprop;
                u32MaxQp = pRcParam->stParamH265Ubr.u32MaxQp;
                u32MinQp = pRcParam->stParamH265Ubr.u32MinQp;
                u32MaxIQp = pRcParam->stParamH265Ubr.u32MaxIQp;
                u32MinIQp = pRcParam->stParamH265Ubr.u32MinIQp;
                s32MaxReEncodeTimes =
                    pRcParam->stParamH265Ubr
                        .s32MaxReEncodeTimes;
                bQpMapEn = pRcParam->stParamH265Ubr.bQpMapEn;
                enQpMapMode =
                    pRcParam->stParamH265Ubr.enQpMapMode;
            } else {
                strcpy(cRcMode, "N/A");
            }

            switch (enQpMapMode) {
            case VENC_RC_QPMAP_MODE_MEANQP:
                strcpy(cQpMapMode, "MEANQP");
                break;
            case VENC_RC_QPMAP_MODE_MINQP:
                strcpy(cQpMapMode, "MINQP");
                break;
            case VENC_RC_QPMAP_MODE_MAXQP:
                strcpy(cQpMapMode, "MAXQP");
                break;
            case VENC_RC_QPMAP_MODE_BUTT:
                strcpy(cQpMapMode, "BUTT");
                break;
            default:
                strcpy(cQpMapMode, "N/A");
                break;
            }

            seq_puts(
                m,
                "------BASE PARAMS 1------------------------------------------------------\n");
            seq_printf(
                m,
                "ChnId: %d\t Gop: %u\t StatTm: %u\t ViFr: %u\t TrgFr: %u\t ProType: %s",
                idx, u32Gop, u32StatTime, u32SrcFrameRate,
                fr32DstFrameRate, cCodecType);
            seq_printf(
                m,
                "\t RcMode: %s\t Br(kbps): %u\t FluLev: %d\t IQp: %u\t PQp: %u\t BQp: %u\n",
                cRcMode, u32BitRate, 0, u32IQp, u32PQp, u32BQp);

            seq_puts(
                m,
                "------BASE PARAMS 2------------------------------------------------------\n");
            seq_printf(
                m,
                "ChnId: %d\t MinQp: %u\t MaxQp: %u\t MinIQp: %u\t MaxIQp: %u",
                idx, u32MinQp, u32MaxQp, u32MinIQp, u32MaxIQp);
            seq_printf(
                m,
                "\t EnableIdr: %d\t bQpMapEn: %d\t QpMapMode: %s\n",
                0, bQpMapEn, cQpMapMode);
            seq_printf(m, "u32RowQpDelta: %d\t",
                   pRcParam->u32RowQpDelta);
            seq_printf(
                m,
                "InitialDelay: %d\t VariFpsEn: %d\t ThrdLv: %d\t BgEnhanceEn: %d\t BgDeltaQp: %d\n",
                pRcParam->s32InitialDelay, bVariFpsEn,
                pRcParam->u32ThrdLv, pRcParam->bBgEnhanceEn,
                pRcParam->s32BgDeltaQp);

            seq_puts(
                m,
                "-----GOP MODE ATTR-------------------------------------------------------\n");
            seq_printf(
                m,
                "ChnId: %d\t GopMode: %s\t IpQpDelta: %d\t SPInterval: %u\t SPQpDelta: %d",
                idx, cGopMode, s32IPQpDelta, u32SPInterval,
                s32SPQpDelta);
            seq_printf(
                m,
                "\t BFrmNum: %u\t BQpDelta: %d\t BgInterval: %u\t ViQpDelta: %d\n",
                u32BFrmNum, s32BQpDelta, u32BgInterval,
                s32ViQpDelta);

            if (pstChnAttr->stRcAttr.enRcMode ==
                    VENC_RC_MODE_H264CBR ||
                pstChnAttr->stRcAttr.enRcMode ==
                    VENC_RC_MODE_H265CBR ||
                pstChnAttr->stRcAttr.enRcMode ==
                    VENC_RC_MODE_H264UBR ||
                pstChnAttr->stRcAttr.enRcMode ==
                    VENC_RC_MODE_H265UBR) {
                seq_puts(
                    m,
                    "-----SUPER FRAME PARAM -------------------------------------------\n");
                seq_printf(
                    m,
                    "ChnId: %d\t FrmMode: %d\t IFrmBitsThr: %d\t PFrmBitsThr: %d\n",
                    idx, pstSuperFrmParam->enSuperFrmMode,
                    pstSuperFrmParam->u32SuperIFrmBitsThr,
                    pstSuperFrmParam->u32SuperPFrmBitsThr);
                if (pstChnAttr->stRcAttr.enRcMode ==
                        VENC_RC_MODE_H264CBR ||
                    pstChnAttr->stRcAttr.enRcMode ==
                        VENC_RC_MODE_H265CBR) {
                    seq_puts(
                    m,
                    "-----RUN CBR PARAM -------------------------------------------\n");
                } else {
                    seq_puts(
                    m,
                    "-----RUN UBR PARAM -------------------------------------------\n");
                }

                seq_printf(
                    m,
                    "ChnId: %d\t MinIprop: %u\t MaxIprop: %u\t MaxQp: %u\t MinQp: %u",
                    idx, u32MinIprop, u32MaxIprop, u32MaxQp,
                    u32MinQp);
                seq_printf(
                    m,
                    "\t MaxIQp: %u\t MinIQp: %u\t MaxReEncTimes: %d\n",
                    u32MaxIQp, u32MinIQp,
                    s32MaxReEncodeTimes);
            } else if (pstChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_MJPEGCBR) {
                seq_puts(
                    m,
                    "-----RUN CBR PARAM -------------------------------------------\n");
                seq_printf(
                    m,
                    "\t ChnId: %d\t MaxQfactor: %u\t MinQfactor: %u\n",
                    idx, u32MaxQfactor, u32MinQfactor);
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                       VENC_RC_MODE_H264VBR ||
                   pstChnAttr->stRcAttr.enRcMode ==
                       VENC_RC_MODE_H265VBR) {
                seq_puts(
                    m,
                    "-----RUN VBR PARAM -------------------------------------------\n");
                seq_printf(
                    m,
                    "ChnId: %d\t ChgPs: %d\t MinIprop: %u\t MaxIprop: %u\t MaxQp: %u",
                    idx, s32ChangePos, u32MinIprop,
                    u32MaxIprop, u32MaxQp);
                seq_printf(
                    m,
                    "\t MinQp: %u\t MaxIQp: %u\t MinIQp: %u\t MaxReEncTimes: %d\n",
                    u32MinQp, u32MaxIQp, u32MinIQp,
                    s32MaxReEncodeTimes);
            } else if (pstChnAttr->stRcAttr.enRcMode == VENC_RC_MODE_MJPEGVBR) {
                seq_puts(
                    m,
                    "-----RUN VBR PARAM -------------------------------------------\n");
                seq_printf(
                    m,
                    "\t ChnId: %d\t ChgPs: %d\t MaxQfactor: %u\t MinQfactor: %u\n",
                    idx, s32ChangePos, u32MaxQfactor, u32MinQfactor);
            } else if (pstChnAttr->stRcAttr.enRcMode ==
                       VENC_RC_MODE_H264AVBR ||
                   pstChnAttr->stRcAttr.enRcMode ==
                       VENC_RC_MODE_H265AVBR) {
                seq_puts(
                    m,
                    "-----RUN AVBR PARAM -------------------------------------------\n");
                seq_printf(
                    m,
                    "ChnId: %d\t ChgPs: %d\t MinIprop: %u\t MaxIprop: %u\t MaxQp: %u",
                    idx, s32ChangePos, u32MinIprop,
                    u32MaxIprop, u32MaxQp);
                seq_printf(
                    m,
                    "\t MinQp: %u\t MaxIQp: %u\t MinIQp: %u\t MaxReEncTimes: %d\n",
                    u32MinQp, u32MaxIQp, u32MinIQp,
                    s32MaxReEncodeTimes);
                seq_printf(
                    m,
                    "MinStillPercent: %d\t MaxStillQP: %u\t MinStillPSNR: %u\t MinQpDelta: %u\n",
                    s32MinStillPercent, u32MaxStillQP,
                    u32MinStillPSNR, u32MinQpDelta);
                seq_printf(
                    m,
                    "MotionSensitivity: %u\t AvbrFrmLostOpen: %d\t AvbrFrmGap: %d\t bQpMapEn: %d\n",
                    u32MotionSensitivity,
                    s32AvbrFrmLostOpen, s32AvbrFrmGap,
                    bQpMapEn);
            }
        }
    }
    return 0;
}

static int rc_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, rc_proc_show, PDE_DATA(inode));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops rc_proc_fops = {
    .proc_open = rc_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations rc_proc_fops = {
    .owner = THIS_MODULE,
    .open = rc_proc_open,
    .read = seq_read,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

int rc_proc_init(struct device *dev)
{
    if (proc_create_data(RC_PROC_NAME, VIDEO_PROC_PERMS, VIDEO_PROC_PARENT,
                 &rc_proc_fops, dev) == NULL) {
        dev_err(dev, "ERROR! /proc/%s create fail\n", RC_PROC_NAME);
        remove_proc_entry(RC_PROC_NAME, NULL);
        return -1;
    }
    return 0;
}

int rc_proc_deinit(void)
{
    remove_proc_entry(RC_PROC_NAME, NULL);
    return 0;
}
#ifdef ENABLE_DEC
static int vdec_proc_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Module: [VDEC] System Build Time [%s]\n", UTS_VERSION);

    if (vdec_handle != NULL) {
        int idx;
        vdec_mod_param_s *pVdecModParam = &vdec_handle->g_stModParam;

        seq_printf(
            m,
            "VdecMaxChnNum: %u\t MiniBufMode: %u\t\t enVdecVBSource: %d\t ParallelMode: %u\n",
            MaxVdecChnNum, pVdecModParam->u32MiniBufMode,
            pVdecModParam->enVdecVBSource,
            pVdecModParam->u32ParallelMode);

        seq_printf(
            m,
            "MaxPicWidth: %u\t MaxPicHeight: %u\t MaxSliceNum: %u",
            pVdecModParam->stVideoModParam.u32MaxPicWidth,
            pVdecModParam->stVideoModParam.u32MaxPicHeight,
            pVdecModParam->stVideoModParam.u32MaxSliceNum);
        seq_printf(
            m,
            "\t VdhMsgNum: %u\t\t VdhBinSize: %u\t VdhExtMemLevel: %u",
            pVdecModParam->stVideoModParam.u32VdhMsgNum,
            pVdecModParam->stVideoModParam.u32VdhBinSize,
            pVdecModParam->stVideoModParam.u32VdhExtMemLevel);
        seq_printf(m, "\nMaxJpegeWidth: %u\t MaxJpegeHeight: %u",
               pVdecModParam->stPictureModParam.u32MaxPicWidth,
               pVdecModParam->stPictureModParam.u32MaxPicHeight);
        seq_printf(
            m,
            "\t SupportProgressive: %d\t DynamicAllocate: %d\t CapStrategy: %d\n\n",
            pVdecModParam->stPictureModParam.bSupportProgressive,
            pVdecModParam->stPictureModParam.bDynamicAllocate,
            pVdecModParam->stPictureModParam.enCapStrategy);

        for (idx = 0; idx < MaxVdecChnNum; idx++) {
            if (vdec_handle &&
                vdec_handle->chn_handle[idx] != NULL) {
                char cCodecType[16] = { '\0' };
                char cDecMode[8] = { '\0' };
                char cOutputOrder[8] = { '\0' };
                char cCompressMode[8] = { '\0' };
                char cPixelFormat[8] = { '\0' };

                vdec_chn_context *pChnHandle =
                    vdec_handle->chn_handle[idx];

                vdec_chn_attr_s *pstChnAttr =
                    &pChnHandle->ChnAttr;
                vdec_chn_param_s *pstChnParam =
                    &pChnHandle->ChnParam;

                video_frame_s *pstFrame =
                    &pChnHandle->stVideoFrameInfo.video_frame;

                get_codec_type_str(pstChnAttr->enType, cCodecType);
                get_pixel_format_str(pstFrame->pixel_format,
                          cPixelFormat);

                switch (pstChnParam->stVdecVideoParam.enDecMode) {
                case VIDEO_DEC_MODE_IP:
                    strcpy(cDecMode, "IP");
                    break;
                case VIDEO_DEC_MODE_I:
                    strcpy(cDecMode, "I");
                    break;
                case VIDEO_DEC_MODE_BUTT:
                    strcpy(cDecMode, "BUTT");
                    break;
                case VIDEO_DEC_MODE_IPB:
                default:
                    strcpy(cDecMode, "IPB");
                    break;
                }

                if (pstChnAttr->u8ReorderEnable)
                    strcpy(cOutputOrder, "DISP");
                else
                    strcpy(cOutputOrder, "DEC");

                switch (pstChnParam->stVdecVideoParam.enCompressMode) {
                case COMPRESS_MODE_TILE:
                    strcpy(cCompressMode, "TILE");
                    break;
                case COMPRESS_MODE_LINE:
                    strcpy(cCompressMode, "LINE");
                    break;
                case COMPRESS_MODE_FRAME:
                    strcpy(cCompressMode, "FRAME");
                    break;
                case COMPRESS_MODE_BUTT:
                    strcpy(cCompressMode, "BUTT");
                    break;
                case COMPRESS_MODE_NONE:
                default:
                    strcpy(cCompressMode, "NONE");
                    break;
                }

                seq_puts(
                    m,
                    "----- CHN COMM ATTR & PARAMS --------------------------------------\n");
                seq_printf(
                    m,
                    "ID: %d\t TYPE: %s\t MaxW: %u\t MaxH: %u\t Width: %u\t Height: %u",
                    idx, cCodecType, MAX_DEC_PIC_WIDTH,
                    MAX_DEC_PIC_HEIGHT, pstFrame->width,
                    pstFrame->height);
                seq_printf(
                    m,
                    "\t Stride: %u\t output pixel format: %s\t PTS: %llu\t PA: 0x%llx\n",
                    pstFrame->stride[0], cPixelFormat,
                    pstFrame->pts,
                    pstFrame->phyaddr[0]);

                get_pixel_format_str(pstChnParam->enPixelFormat,
                          cPixelFormat);
                seq_printf(
                    m,
                    "InputMode: %s\t StreamBufSize: %u\t\t FrmBufSize: %u",
                    pstChnAttr->enMode==VIDEO_MODE_STREAM ? "stream" : "frame",
                    pstChnAttr->u32StreamBufSize,
                    pstChnAttr->u32FrameBufSize);
                seq_printf(
                    m,
                    "\t extra FrmBufCnt: %u\t\t TmvBufSize: %u\n",
                    pstChnAttr->u32FrameBufCnt,
                    pstChnAttr->stVdecVideoAttr.u32TmvBufSize);

                seq_printf(
                    m,
                    "ID: %d\t DispNum: %d\t DispMode: %s\t SetUserPic: %s\t EnUserPic: %s",
                    idx, 2, "PLAYBACK", "N", "N");
                seq_printf(
                    m,
                    "\t Rotation: %u\t PicPoolId: %d\t TmvPoolId: %d\t STATE: %s\n\n",
                    (pstChnAttr->enType == PT_JPEG || pstChnAttr->enType == PT_MJPEG) ?
                        pstChnParam->stVdecPictureParam.s32RotAngle : 0,
                    pChnHandle->bHasVbPool ? pChnHandle->vbPool.hPicVbPool : -1, -1, "START");

                seq_puts(
                    m,
                    "----- CHN VIDEO ATTR & PARAMS -------------------------------------\n");
                seq_printf(
                    m,
                    "ID: %d\t VfmwID: %d\t RefNum: %u\t TemporalMvp: %s\t ErrThr: %d",
                    idx,
                    pstChnAttr->enType == PT_H265 ? 0 : 1,
                    pstChnAttr->stVdecVideoAttr.u32RefFrameNum,
                    pstChnAttr->stVdecVideoAttr.bTemporalMvpEnable ?
                              "Y" :
                              "N",
                    pstChnParam->stVdecVideoParam.s32ErrThreshold);
                seq_printf(
                    m,
                    "\t DecMode: %s\t OutPutOrder: %s\t Compress: %s\t VideoFormat: %d",
                    cDecMode, cOutputOrder, cCompressMode,
                    pstChnParam->stVdecVideoParam.enVideoFormat);
                seq_printf(
                    m,
                    "\t MaxVPS: %u\t MaxSPS: %u\t MaxPPS: %u\n\n",
                    0, 0, 0);

                seq_puts(
                    m,
                    "----- CHN PICTURE ATTR & PARAMS---------------------------------\n");
                seq_printf(m, "ID: %d\t Alpha: %u\n\n", idx, pstChnParam->stVdecPictureParam.u32Alpha);

// TODO: following info should be amended later
#if 0
                seq_puts(m, "----- CHN STATE -------------------------------------------------\n");
                seq_printf(m, "ID: %d\t PrtclErr: %u\t StrmUnSP: %u\t StrmError: %u\t RefNumErr: %u",
                    idx, 0, 0, 0, 0);
                seq_printf(m, "\t PicSizeErr: %u\t Fmterror: %u\t PicBufSizeErr: %u",
                    0, 0, 0);
                seq_printf(m, "\t StrSizeOver: %u\t Notify: %u\t UniqueId: %u\t State: %u\n",
                    0, 0, 0, 0);
                seq_printf(m, "ID: %d\t fps: %d\t TimerCnt: %u\t BufFLen: %u\t DataLen: %u",
                    idx, 30, 0, 0, 0);
                seq_printf(m, "\t RdRegionLen: %u\t SCDLeftLen: %u\t WrRegionLen: %u\t ptsBufF: %u",
                    0, 0, 0, 0);
                seq_printf(m, "\t ptsBufU: %u\t StreamEnd: %u\t FrameEnd: %u\n",
                    0, 0, 0);

                seq_puts(m, "----- Detail Stream STATE -----------------------------------------\n");
                seq_printf(m, "ID: %d\t MpiSndNum: %u\t MpiSndLen: %u\t VdecNum: %u\t VdecLen: %u",
                    idx, 0, 0, 0, 0);
                seq_printf(m, "\t FmGetNum: %u\t FmGetLen: %u\t FmRlsNum: %u\t FmRlsLen: %u\n",
                    0, 0, 0, 0);

                seq_printf(m, "ID: %d\t FmLstGet: %d\t FmRlsFail: %d\n", idx, 0, 0);

                seq_puts(m, "----- Detail FrameStore STATE ---------------------------------\n");
                seq_puts(m, "NOT SUPPORTED\n");

                seq_puts(m, "----- Detail UserData STATE ----------------------------------------\n");
                seq_printf(m, "ID: %d\t Enable: %d\t MaxUserDataLen: %u\n",
                    idx, 1, 1024);
                seq_printf(m, "ID: %d\t MpiGet: %d\t MpiGetLen: %u\t MpiRls: %d\t MpiRlsLen: %d",
                    idx, 0, 0, 0, 0);
                seq_printf(m, "\t Discard: %d\t DiscardLen: %d\t GetFromFm: %d", 0, 0, 0);
                seq_printf(m, "\t GetFromFmLen: %d\t UsrFLen: %d\t UsrLen: %d\n", 0, 0, 0);
#endif

                seq_puts(
                    m,
                    "-----VDEC CHN PERFORMANCE------------------------------------------------\n");
                seq_printf(
                    m,
                    "ID: %d\t No.SendStreamPerSec: %u\t No.DecFramePerSec: %u\t HwDecTime: %llu us\n\n",
                    idx, pChnHandle->stFPS.in_fps,
                    pChnHandle->stFPS.out_fps,
                    pChnHandle->stFPS.hw_time);
            }
        }
    }

    seq_puts(
        m,
        "\n----- Debug Level STATE ----------------------------------------\n");
    seq_printf(
        m,
        "VdecDebugMask: 0x%X\t VdecStartFrmIdx: %u\t VdecEndFrmIdx: %u\t VdecDumpPath: %s\n",
        tVdecDebugConfig.u32DbgMask, tVdecDebugConfig.u32StartFrmIdx,
        tVdecDebugConfig.u32EndFrmIdx, tVdecDebugConfig.cDumpPath);
    return 0;
}

static int vdec_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, vdec_proc_show, PDE_DATA(inode));
}

static ssize_t vdec_proc_write(struct file *file, const char __user *user_buf,
                   size_t count, loff_t *ppos)
{
    /* debug_level list, please refer to: comm_vdec.h
     * DRV_VDEC_MASK_ERR    0x1
     * DRV_VDEC_MASK_WARN    0x2
     * DRV_VDEC_MASK_INFO    0x4
     * DRV_VDEC_MASK_FLOW    0x8
     * DRV_VDEC_MASK_DBG    0x10
     * DRV_VDEC_MASK_BS        0x100
     * DRV_VDEC_MASK_SRC    0x200
     * DRV_VDEC_MASK_API    0x400
     * DRV_VDEC_MASK_DISP    0x800
     * DRV_VDEC_MASK_PERF    0x1000
     * DRV_VDEC_MASK_CFG    0x2000
     * DRV_VDEC_MASK_TRACE    0x4000
     * DRV_VDEC_MASK_DUMP_YUV    0x10000
     * DRV_VDEC_MASK_DUMP_BS    0x20000
     */
    char cProcInputdata[MAX_PROC_STR_SIZE] = { '\0' };
    char cVdecDbgPrefix[] = "vdec=0x"; // vdec=debug_levle
    char cVdecDbgStartFrmPrefix[] = "vdec_sfn="; // vdec_sfn=frame_idx_begin
    char cVdecDbgEndFrmPrefix[] = "vdec_efn="; // vdec_efn=frame_idx_end
    char cVdecDbgDirPrefix[] =
        "vdec_dir="; // vdec_dir=your_preference_directory
    proc_debug_config_t *pVdecDebugConfig =
        (proc_debug_config_t *)&tVdecDebugConfig;

    return vcodec_proc_write_helper(user_buf, count, pVdecDebugConfig,
                    cProcInputdata, cVdecDbgPrefix,
                    cVdecDbgStartFrmPrefix,
                    cVdecDbgEndFrmPrefix, cVdecDbgDirPrefix,
                    NULL);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0))
static const struct proc_ops vdec_proc_fops = {
    .proc_open = vdec_proc_open,
    .proc_read = seq_read,
    .proc_write = vdec_proc_write,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};
#else
static const struct file_operations vdec_proc_fops = {
    .owner = THIS_MODULE,
    .open = vdec_proc_open,
    .read = seq_read,
    .write = vdec_proc_write,
    .llseek = seq_lseek,
    .release = single_release,
};
#endif

int vdec_proc_init(struct device *dev)
{
    if (proc_create_data(VDEC_PROC_NAME, VIDEO_PROC_PERMS,
                 VIDEO_PROC_PARENT, &vdec_proc_fops, dev) == NULL) {
        dev_err(dev, "ERROR! /proc/%s create fail\n", VDEC_PROC_NAME);
        remove_proc_entry(VDEC_PROC_NAME, NULL);
        return -1;
    }
    return 0;
}

int vdec_proc_deinit(void)
{
    remove_proc_entry(VDEC_PROC_NAME, NULL);
    return 0;
}
#endif
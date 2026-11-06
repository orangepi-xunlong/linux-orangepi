#include "venc_rc.h"

static void set_pic_qp_by_delta(stRcInfo *pRcInfo, EncOpenParam *pOpenParam, int delta)
{
    EncWave5Param *param = &pOpenParam->EncStdParam.waveParam;
    pRcInfo->picIMaxQp = (param->maxQpI > 0) ? CLIP3(0, 51, param->maxQpI - delta) : 51;
    pRcInfo->picIMinQp = (param->minQpI > 0) ? CLIP3(0, 51, param->minQpI + delta) : 0;
    pRcInfo->picPMaxQp = (param->maxQpP > 0) ? CLIP3(0, 51, param->minQpP - delta) : 51;
    pRcInfo->picPMinQp = (param->minQpP > 0) ? CLIP3(0, 51, param->minQpP + delta) : 0;
}

static void _venc_rc_set_bitrate_param(stRcInfo *pRcInfo, EncOpenParam *pOpenParam)
{
    EncWave5Param *param = &pOpenParam->EncStdParam.waveParam;
    int frameRateDiv, frameRateRes;

    // VBR
    if (pRcInfo->rcMode == RC_MODE_VBR) {
        pRcInfo->convergStateBufThr = pOpenParam->bitRate; // in wave5 series, bitRate is bps
        pRcInfo->targetBitrate = (pOpenParam->bitRate * pOpenParam->changePos) / 100;
        pRcInfo->maximumBitrate = pOpenParam->bitRate;
    } else if (pRcInfo->rcMode == RC_MODE_AVBR) {
        int minPercent = pOpenParam->minStillPercent;
        int motLv = pRcInfo->periodMotionLv;
        int motBitRatio = (((100 - minPercent) * (motLv)) / 255) + minPercent;
        int bitrateByMotLv = (pOpenParam->bitRate * motBitRatio) / 100;

        //bitrateByMotLv = (pRcInfo->avbrLastPeriodBitrate * 0.25) + (bitrateByMotLv * 0.75);
        pRcInfo->avbrLastPeriodBitrate = bitrateByMotLv;
        pRcInfo->convergStateBufThr = bitrateByMotLv;
        pRcInfo->targetBitrate =
            (bitrateByMotLv * pOpenParam->changePos) / 100;
        pRcInfo->maximumBitrate = pOpenParam->bitRate;
        VLOG(INFO, "new targetBitrate = %d\n", pRcInfo->targetBitrate );
    } else {
        pRcInfo->targetBitrate = pOpenParam->bitRate;
    }

    frameRateDiv = (pOpenParam->frameRateInfo >> 16) + 1;
    frameRateRes = pOpenParam->frameRateInfo & 0xFFFF;

    if (pRcInfo->RcEn) {
        // TODO: use soft-floating to replace it
        pRcInfo->picAvgBit = (int)(pRcInfo->targetBitrate *
                frameRateDiv / frameRateRes);
    } else {
        VLOG(INFO, "RcEn = %d\n", pRcInfo->RcEn);
    }

    VLOG(INFO,"targetbitrate = %d, gopSize = %d, maxIprop = %d, framerate = %d/%d \n"
            , pRcInfo->targetBitrate, param->intraPeriod
            , pOpenParam->maxIprop, frameRateRes, frameRateDiv);

    if ((pRcInfo->codec == STD_AVC || pRcInfo->rcMode == RC_MODE_UBR) &&
        pOpenParam->maxIprop > 0 && param->intraPeriod > 1) {
        if (pRcInfo->RcEn) {
            pRcInfo->maxIPicBit =
                (int)((Uint64)pOpenParam->maxIprop * param->intraPeriod *
                        pRcInfo->targetBitrate * frameRateDiv /
                        frameRateRes /
                        (pOpenParam->maxIprop + param->intraPeriod - 1));
        } else {
            VLOG(INFO, "RcEn = %d\n", pRcInfo->RcEn);
        }
        VLOG(INFO, "maxIPicBit = %d\n", pRcInfo->maxIPicBit);
    }
}
static void _venc_rc_set_framerate_param(stRcInfo *pRcInfo, EncOpenParam *pOpenParam)
{
    pRcInfo->framerate = pOpenParam->frameRateInfo;
}

int venc_rc_set_param(stRcInfo *pRcInfo, EncOpenParam *pOpenParam, eRcParam eParam)
{
    int ret = 0;

    switch (eParam) {
    case E_BITRATE:
        _venc_rc_set_bitrate_param(pRcInfo, pOpenParam);
        ret = 1;
        break;
    case E_FRAMERATE:
        _venc_rc_set_framerate_param(pRcInfo, pOpenParam);
        ret = 1;
        break;
    default:
        break;
    }

    return ret;
}

void venc_rc_open(stRcInfo *pRcInfo, EncOpenParam *pOpenParam)
{
    EncWave5Param *param = &pOpenParam->EncStdParam.waveParam;

    pRcInfo->rcEnable = pOpenParam->rcEnable;

    pRcInfo->RcEn = 1;// pOpenParam->RcEn;

    pRcInfo->codec = pOpenParam->bitstreamFormat;

    // params setting
    pRcInfo->rcMode = pOpenParam->hostRcmode;
    pRcInfo->numOfPixel = pOpenParam->picWidth * pOpenParam->picHeight;
    // frame skipping
    pRcInfo->contiSkipNum = (!pOpenParam->frmLostOpen) ? 0 :(pOpenParam->encFrmGaps == 0) ? 65535 : pOpenParam->encFrmGaps;

    pRcInfo->frameSkipBufThr = pOpenParam->frmLostBpsThr;

    // avbr setting
    if (pRcInfo->rcMode == RC_MODE_AVBR) {
        int basePeriod, idx;
        for (basePeriod = AVBR_MAX_BITRATE_WIN; basePeriod > 0; basePeriod--) {
            if (param->intraPeriod % basePeriod == 0) {
                pRcInfo->bitrateChangePeriod = basePeriod;
                break;
            }
        }
        for (idx = 0; idx < AVBR_MAX_BITRATE_WIN; idx++) {
            pRcInfo->picMotionLvWindow[idx] = MOT_LV_DEFAULT;
        }
    }
    pRcInfo->periodMotionLv = MOT_LV_DEFAULT;
    pRcInfo->lastPeriodMotionLv = MOT_LV_DEFAULT;
    pRcInfo->avbrContiSkipNum = (!pOpenParam->avbrFrmLostOpen) ? 0 : (pOpenParam->avbrFrmGaps == 0) ? 65535 : pOpenParam->avbrFrmGaps;

    pRcInfo->maxIPicBit = -1;

    // bitrate
    venc_rc_set_param(pRcInfo, pOpenParam, E_BITRATE);
    venc_rc_set_param(pRcInfo, pOpenParam, E_FRAMERATE);

    // pic min/max qp
    pRcInfo->picMinMaxQpClipRange = 0;
    set_pic_qp_by_delta(pRcInfo, pOpenParam, pRcInfo->picMinMaxQpClipRange);

    // initial value setup
    pRcInfo->frameSkipCnt = 0;
    pRcInfo->bitrateBuffer = pRcInfo->targetBitrate * 400; // initial buffer level
    pRcInfo->ConvergenceState = 0;
    pRcInfo->rcState = STEADY;
    pRcInfo->avbrLastPeriodBitrate = pRcInfo->targetBitrate;
    pRcInfo->avbrChangeBrEn = FALSE;
    pRcInfo->isLastPicI = FALSE;
    pRcInfo->avbrChangeValidCnt = 0;
    pRcInfo->periodMotionLvRaw = 32;
    pRcInfo->s32HrdBufLevel = pRcInfo->targetBitrate * pOpenParam->rcInitDelay / 4;

    // I frame RQ model for max I frame size constraint
    VLOG(INFO, "resolution wxh = %dx%d\n", pOpenParam->picWidth, pOpenParam->picHeight);
    VLOG(INFO, "rcEnable = %d\n", pRcInfo->rcEnable);
    VLOG(INFO, "rc mode = %d\n", pRcInfo->rcMode);
    VLOG(INFO, "targetBitrate = %dk\n", pRcInfo->targetBitrate);
    VLOG(INFO, "frame rate = %d\n", pOpenParam->frameRateInfo);
    VLOG(INFO, "picAvgBit = %d\n", pRcInfo->picAvgBit);
    VLOG(INFO, "convergStateBufThr = %d\n", pRcInfo->convergStateBufThr);
    if (pRcInfo->rcMode == RC_MODE_AVBR) {
        VLOG(INFO, "bitrateChangePeriod = %d\n",
                pRcInfo->bitrateChangePeriod);
        VLOG(INFO, "motionSensitivity = %d\n", pOpenParam->motionSensitivity);
        VLOG(INFO, "minStillPercent = %d\n", pOpenParam->minStillPercent);
        VLOG(INFO, "maxStillQp = %d\n", pOpenParam->maxStillQp);
        VLOG(INFO, "avbrPureStillThr = %d\n", pOpenParam->avbrPureStillThr);
        VLOG(INFO, "avbrContiSkipNum = %d\n",
                pRcInfo->avbrContiSkipNum);
    }
    VLOG(INFO, "bFrmLostOpen = %d\n", pOpenParam->frmLostOpen);
    if (pOpenParam->frmLostOpen) {
        VLOG(INFO, "bitrateBuffer = %d\n", pRcInfo->bitrateBuffer);
        VLOG(INFO, "frameSkipBufThr = %d\n", pRcInfo->frameSkipBufThr);
        VLOG(INFO, "contiSkipNum = %d\n", pRcInfo->contiSkipNum);
    }

    VLOG(INFO, "maxIprop = %d, gopSize = %d, maxIPicBit = %d\n",
            pOpenParam->maxIprop, param->intraPeriod, pRcInfo->maxIPicBit);

    VLOG(INFO, "RcEn = %d\n", pRcInfo->RcEn);
}

void venc_rc_update_frame_skip_setting(stRcInfo *pRcInfo, int frmLostOpen, int encFrmGaps, int frmLostBpsThr)
{
    pRcInfo->contiSkipNum = (!frmLostOpen) ? 0 : (encFrmGaps == 0) ? 65535 : encFrmGaps;
    pRcInfo->frameSkipBufThr = frmLostBpsThr;
}


int venc_rc_chk_frame_skip_by_bitrate(stRcInfo *pRcInfo, int isIFrame)
{
    int isFrameDrop = 0;

    if ((!pRcInfo->rcEnable) || (pRcInfo->contiSkipNum == 0)) {
        return 0;
    }

    if (pRcInfo->bitrateBuffer > pRcInfo->frameSkipBufThr &&
        pRcInfo->frameSkipCnt < pRcInfo->contiSkipNum && !isIFrame) {
        isFrameDrop = 1;
        pRcInfo->frameSkipCnt++;
    } else {
        isFrameDrop = 0;
        pRcInfo->frameSkipCnt = 0;
    }
    // VLOG(INFO, "FrameDrop: %d  %d / %d\n", isFrameDrop,
    //         pRcInfo->bitrateBuffer, pRcInfo->frameSkipBufThr);
    return isFrameDrop;

}
BOOL venc_rc_state_check(stRcInfo *pRcInfo, BOOL detect)
{
    eRcState curState = pRcInfo->rcState;
    BOOL transition = FALSE;
    switch (curState) {
    case UNSTABLE:
        transition |= (!detect);
        pRcInfo->rcState += (!detect);
        break;
    case RECOVERY:
        pRcInfo->rcState = (detect) ? UNSTABLE : STEADY;
        transition = TRUE;
        break;
    default:
        transition |= detect;
        pRcInfo->rcState += detect;
        break;
    }

    return transition;
}

static int calc_avg_motionLv(stRcInfo *pRcInfo)
{
    int winSize = pRcInfo->bitrateChangePeriod;
    // trimmed average
    /*
    int idx = 0, motionWin[AVBR_MAX_BITRATE_WIN];
    for(idx=0; idx<winSize; idx++) {
      motionWin[idx] = pRcInfo->picMotionLvWindow[idx];
    }
    insertion_sort(motionWin, winSize);
    int startIdx = (winSize >> 2);
    int endIdx = startIdx + ((winSize)>>1);
    int accum = 0;
    for(idx=startIdx; idx<endIdx; idx++) {
        accum += motionWin[idx % winSize];
    }
    return CLIP3(0, 255, (accum / (winSize >> 1)));
    */
    int accum = 0, idx = 0;
    for (idx = 0; idx < winSize; idx++) {
        accum += pRcInfo->picMotionLvWindow[idx % winSize];
    }
    return CLIP3(0, 255, (accum / winSize));
}

BOOL venc_rc_avbr_pic_ctrl(stRcInfo *pRcInfo, EncOpenParam *pOpenParam, int frameIdx)
{
    BOOL changeBrEn = FALSE;
    int winSize, coring_offset, coring_in, coring_out, coring_gain;

    if (pRcInfo->rcMode != RC_MODE_AVBR) {
        return FALSE;
    }
    winSize = pRcInfo->bitrateChangePeriod;
    coring_offset = pOpenParam->avbrPureStillThr;
    coring_in = pOpenParam->picMotionLevel;
    coring_out = CLIP3(0, 255, (coring_in - coring_offset));
    // DRV_VC_INFO("pic mot in = %d, mot lv = %d\n", coring_in, coring_out);

    pOpenParam->picMotionLevel = coring_out;
    pRcInfo->picMotionLvWindow[frameIdx % winSize] = pOpenParam->picMotionLevel;
    pRcInfo->avbrChangeBrEn = FALSE;
    coring_gain = pOpenParam->motionSensitivity;
    pRcInfo->periodMotionLvRaw = calc_avg_motionLv(pRcInfo);
    pRcInfo->periodMotionLv = CLIP3(0, 255, (pRcInfo->periodMotionLvRaw * coring_gain) / 10);
    // DRV_VC_INFO("periodMotionLvRaw = %d periodMotionLv = %d\n",
    //         pRcInfo->periodMotionLvRaw, pRcInfo->periodMotionLv);
    pRcInfo->avbrChangeValidCnt = (pRcInfo->avbrChangeValidCnt > 0) ?
                  (pRcInfo->avbrChangeValidCnt - 1) : 0;
    // DRV_VC_INFO("avbrChangeValidCnt = %d\n", pRcInfo->avbrChangeValidCnt);
    if (pRcInfo->avbrChangeValidCnt == 0) {
        int MotionLvLower;
        // quantize motion diff to avoid too frequently bitrate change
        int diffMotLv = (pRcInfo->lastPeriodMotionLv >
                 pRcInfo->periodMotionLv) ?
                          (pRcInfo->lastPeriodMotionLv -
                     pRcInfo->periodMotionLv) >>
                        4 :
                          (pRcInfo->periodMotionLv -
                     pRcInfo->lastPeriodMotionLv) >>
                        4;
        changeBrEn =
            (diffMotLv >= 1) || (pRcInfo->periodMotionLv == 0 &&
                         pRcInfo->lastPeriodMotionLv > 0 &&
                         pRcInfo->lastPeriodMotionLv <= 16);
        MotionLvLower = CLIP3(0, 255, pRcInfo->lastPeriodMotionLv - MOT_LOWER_THR);
        pRcInfo->periodMotionLv = MAX(MotionLvLower, pRcInfo->periodMotionLv);
        if (changeBrEn) {
            pRcInfo->lastPeriodMotionLv = pRcInfo->periodMotionLv;
            pRcInfo->avbrChangeBrEn = TRUE;
            pRcInfo->avbrChangeValidCnt = MAX(
                20, pRcInfo->framerate); //AVBR_MAX_BITRATE_WIN;
        }
        //DRV_VC_INFO("avbr bitrate = %d\n",  pRcInfo->targetBitrate);
    }
    return (changeBrEn) ? TRUE : FALSE;

}

int venc_rc_get_param(stRcInfo *pRcInfo, eRcParam eParam)
{
    int ret = 0;

    switch (eParam) {
    case E_BITRATE:
        ret = (pRcInfo->rcMode == RC_MODE_CBR || pRcInfo->rcMode == RC_MODE_UBR) ?
                 pRcInfo->targetBitrate : pRcInfo->maximumBitrate;
        break;
    case E_FRAMERATE:
        ret = pRcInfo->framerate;
        break;
    default:
        break;
    }

    return ret;

}

BOOL venc_rc_avbr_check_frame_skip(stRcInfo *pRcInfo, EncOpenParam *pOpenParam, int isIFrame)
{
    BOOL isFrameSkip = FALSE;
    BOOL isStillPic = TRUE;
    int idx;

    if ((!pRcInfo->rcEnable) || (pRcInfo->rcMode != RC_MODE_AVBR) ||
        pRcInfo->avbrContiSkipNum == 0) {
        return FALSE;
    }

    for (idx = 0; idx < pRcInfo->bitrateChangePeriod; idx++) {
        if (pRcInfo->picMotionLvWindow[idx] > 0) {
            isStillPic = FALSE;
            break;
        }
    }

    if (isStillPic && pRcInfo->frameSkipCnt < pRcInfo->avbrContiSkipNum &&
        !isIFrame) {
        isFrameSkip = TRUE;
        pRcInfo->frameSkipCnt++;
    } else {
        isFrameSkip = FALSE;
        pRcInfo->frameSkipCnt = 0;
    }
    // CVI_VC_INFO("FrameSkipByStillPic: %d %d %d\n", isFrameSkip, pOpenParam->picMotionLevel, pRcInfo->periodMotionLv);
    return isFrameSkip;
}

int venc_rc_avbr_get_qpdelta(stRcInfo *pRcInfo, EncOpenParam *pOpenParam)
{
    EncWave5Param *param = &pOpenParam->EncStdParam.waveParam;
    int qDelta = 0;
    if ((!pRcInfo->rcEnable) || (pRcInfo->rcMode != RC_MODE_AVBR)) {
        return 0;
    }

    do {
        int stillQp = pOpenParam->maxStillQp;
        int maxQp = (param->maxQpP + param->maxQpI) >> 1;
        int ratio = CLIP3(0, 64, 64 - pRcInfo->periodMotionLvRaw);
        int qDiff = MIN(0, stillQp - maxQp);

        qDelta = (ratio * qDiff) >> 6;

        pRcInfo->qDelta = qDelta;
    } while (0);

    return qDelta;

}



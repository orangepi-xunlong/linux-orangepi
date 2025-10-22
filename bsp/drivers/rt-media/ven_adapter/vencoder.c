/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (C) 2008-2015 Allwinner Technology Co. Ltd.
 * Author: Ning Fang <fangning@allwinnertech.com>
 *         Caoyuan Yang <yangcaoyuan@allwinnertech.com>
 *
 * This software is confidential and proprietary and may be used
 * only as expressly authorized by a licensing agreement from
 * Softwinner Products.
 *
 * The entire notice above must be reproduced on all copies
 * and should not be removed.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
//vencoder.c only adater
//#define LOG_TAG "venc"
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/time.h>
#include "vencoder.h"
#include "../memory/cdc_log.h"

enum CDC_LOG_LEVEL_TYPE CDC_GLOBAL_LOG_LEVEL = LOG_LEVEL_DEBUG;
const char *CDC_LOG_LEVEL_NAME[7] = {
     "",
     "",
     [LOG_LEVEL_VERBOSE] = "VERBOSE",
     [LOG_LEVEL_DEBUG]   = "DEBUG  ",
     [LOG_LEVEL_INFO]    = "INFO   ",
     [LOG_LEVEL_WARNING] = "WARNING",
     [LOG_LEVEL_ERROR]   = "ERROR  ",
};

int VencSetParameter(VideoEncoder *pEncoder, VENC_INDEXTYPE indexType, void *paramData)
{
    return 0;
}
EXPORT_SYMBOL(VencGetValidOutputBufNum);

void VencDestroy(VideoEncoder *pEncoder)
{
    return ;
}
EXPORT_SYMBOL(VencDestroy);

int VencGetParameter(VideoEncoder *pEncoder, VENC_INDEXTYPE indexType, void *paramData)
{
    return 0;
}
EXPORT_SYMBOL(VencGetParameter);

VideoEncoder *VencCreate(VENC_CODEC_TYPE eCodecType)
{
    return NULL;
}
EXPORT_SYMBOL(VencCreate);

int VencInit(VideoEncoder *pEncoder, VencBaseConfig *pConfig)
{
    return 0;
}
EXPORT_SYMBOL(VencInit);

int VencStart(VideoEncoder *pEncoder)
{
    return 0;
}
EXPORT_SYMBOL(VencStart);

int VencPause(VideoEncoder *pEncoder)
{
    return 0;
}
EXPORT_SYMBOL(VencPause);

int VencSetCallbacks(VideoEncoder *pEncoder, VencCbType *pCallbacks, void *pAppData1, void *pAppData2)
{
    return 0;
}
EXPORT_SYMBOL(VencSetCallbacks);

int VencQueueOutputBuf(VideoEncoder *pEncoder, VencOutputBuffer *pBuffer)
{
    return 0;
}
EXPORT_SYMBOL(VencQueueOutputBuf);

int VencDequeueOutputBuf(VideoEncoder *pEncoder, VencOutputBuffer *pBuffer)
{
    return 0;
}
EXPORT_SYMBOL(VencDequeueOutputBuf);

int VencGetValidOutputBufNum(VideoEncoder *pEncoder)
{
    return 0;
}
EXPORT_SYMBOL(VencSetParameter);

int VencQueueInputBuf(VideoEncoder *pEncoder, VENC_IN VencInputBuffer *inputbuffer)
{
    return 0;
}
EXPORT_SYMBOL(VencQueueInputBuf);

int encodeOneFrame(VideoEncoder *pEncoder)
{
    return 0;
}
EXPORT_SYMBOL(encodeOneFrame);

#ifdef __cplusplus
}
#endif /* __cplusplus */


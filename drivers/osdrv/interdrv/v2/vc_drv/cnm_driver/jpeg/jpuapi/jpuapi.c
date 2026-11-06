//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------
#include "jpuapifunc.h"
#include "jpuapi.h"
#include "regdefine.h"
#include "jpulog.h"

static JPUCap   g_JpuAttributes;
extern int jpu_enable_irq(int coreidx);


int JPU_IsBusy(JpgHandle handle)
{
    Uint32 val;
    JpgInst *pJpgInst = (JpgInst *)handle;
    Int32 instRegIndex;

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }
    val = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_STATUS_REG);

    if ((val & (1<<INT_JPU_DONE)) ||
        (val & (1<<INT_JPU_ERROR)))
        return 0;

    return 1;
}

void JPU_ClrStatus(JpgHandle handle, Uint32 val)
{
    JpgInst *pJpgInst = (JpgInst *)handle;
    Int32 instRegIndex;

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }
    if (val != 0)
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_STATUS_REG, val);
}
Uint32 JPU_GetStatus(JpgHandle handle)
{
    JpgInst *pJpgInst = (JpgInst *)handle;
    Int32 instRegIndex;

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }

    return JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_STATUS_REG);
}


Uint32 JPU_IsInit()
{
    jpu_instance_pool_t *pjip;

    JpgEnterLockEx();
    pjip = (jpu_instance_pool_t *)jdi_get_instance_pool();

    if (!pjip) {
        JpgLeaveLockEx();
        return 0;
    }

    JpgLeaveLockEx();
    return 1;
}


Int32 JPU_WaitInterrupt(JpgHandle handle, int timeout)
{
    Uint32 val;
    Int32 instRegIndex;
    Int32 reason = 0;
    JpgInst *pJpgInst = (JpgInst *)handle;

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }

    reason = jdi_wait_interrupt(pJpgInst->coreIndex, timeout, instRegIndex);
    if (reason == -1)
        return -1;

    if (reason & (1<<INT_JPU_DONE) || reason & (1<<INT_JPU_SLICE_DONE)) {
        val = JpuReadReg(pJpgInst->coreIndex, MJPEG_INST_CTRL_STATUS_REG);
        if ((((val & 0xf) >> instRegIndex) & 0x01) == 0) {
            jpu_enable_irq(pJpgInst->coreIndex);
            return -2;
        }
    }

    return reason;
}

int JPU_GetOpenInstanceNum(void)
{
    jpu_instance_pool_t *pjip;

    pjip = (jpu_instance_pool_t *)jdi_get_instance_pool();
    if (!pjip)
        return -1;

    return pjip->jpu_instance_num;
}

JpgRet JPU_Init(void)
{
    jpu_instance_pool_t *pjip;
    Uint32 val = 0;
    int core_idx = 0;

    JpgEnterLockEx();
    //only init once
    if (jdi_init() < 0) {
        JpgLeaveLockEx();
        return JPG_RET_FAILURE;
    }

    if (jdi_get_task_num() > 1) {
        JpgLeaveLockEx();
        return JPG_RET_CALLED_BEFORE;
    }

    pjip = (jpu_instance_pool_t *)jdi_get_instance_pool();
    if (!pjip) {
        JpgLeaveLockEx();
        return JPG_RET_FAILURE;
    }

    //jdi_log(core_idx, JDI_LOG_CMD_INIT, 1, 0);
    for (core_idx = 0; core_idx < MAX_NUM_JPU_CORE; core_idx++) {
        JPU_SWTopReset(core_idx);
        JPU_SWReset(core_idx, NULL);
        JpuWriteRegExt(core_idx,MJPEG_INST_CTRL_START_REG, (1<<0));

        val = JpuReadInstRegExt(core_idx, 0, MJPEG_VERSION_INFO_REG);
        // JPU Capabilities
        g_JpuAttributes.productId    = (val>>24) & 0xf;
        g_JpuAttributes.revisoin     = (val & 0xffffff);
        g_JpuAttributes.support12bit = (val>>28) & 0x01;

        JLOG(INFO, "JPU core:%d, Version API_VERSION=0x%x, HW_REVISION=%d, HW_PRODUCT_ID=0x%x\n"
            , core_idx, API_VERSION, g_JpuAttributes.revisoin, g_JpuAttributes.productId);
    }

    // jdi_log(JDI_LOG_CMD_INIT, 0, 0);
    JpgLeaveLockEx();
    return JPG_RET_SUCCESS;
}

void JPU_DeInit(void)
{
    int core_idx;

    JpgEnterLockEx();
    if (jdi_get_task_num() == 1) {
        for (core_idx = 0; core_idx < MAX_NUM_JPU_CORE; core_idx++) {
            JpuWriteRegExt(core_idx,MJPEG_INST_CTRL_START_REG, 0);
            if (jdi_wait_inst_ctrl_busy(core_idx, JPU_INST_CTRL_TIMEOUT_MS, MJPEG_INST_CTRL_STATUS_REG, INST_CTRL_IDLE) == -1) {// wait for INST_CTRL become IDLE
                JLOG(WARN, "coreidx:%d inst_ctrl_busy fail", core_idx);
            } else {
                JLOG(INFO, "coreidx:%d inst_ctrl_busy ok", core_idx);
            }
        }
    }

    jdi_release();
    JpgLeaveLockEx();
}

JpgRet JPU_GetVersionInfo(Uint32 *apiVersion, Uint32 *hwRevision, Uint32 *hwProductId)
{
    if (JPU_IsInit() == 0) {
        return JPG_RET_NOT_INITIALIZED;
    }

    JpgEnterLock();
    if (apiVersion) {
        *apiVersion = API_VERSION;
    }
    if (hwRevision) {
        *hwRevision = g_JpuAttributes.revisoin;
    }
    if (hwProductId) {
        *hwProductId = g_JpuAttributes.productId;
    }
    JpgLeaveLock();
    return JPG_RET_SUCCESS;
}

JpgRet JPU_GetExtAddr(int core_idx, Uint32 *extAddr)
{
    if (extAddr) {
        *extAddr = jdi_get_extension_address(core_idx);
    }

    return JPG_RET_SUCCESS;
}

void JPU_SetExtAddr(int core_idx, Uint32 extAddr)
{
    JpgEnterLockEx();
    jdi_set_extension_address(core_idx, extAddr);
    JpgLeaveLockEx();
}

void JPU_SWTopReset(int core_idx)
{
    jdi_sw_top_reset(core_idx);
}

JpgRet JPU_DecOpen(JpgDecHandle * pHandle, JpgDecOpenParam * pop)
{
    JpgInst*    pJpgInst;
    JpgDecInfo* pDecInfo;
    JpgRet      ret;
    ret = CheckJpgDecOpenParam(pop);
    if (ret != JPG_RET_SUCCESS) {
        return ret;
    }

    JpgEnterLockEx();
    ret = GetJpgInstance(&pJpgInst);
    if (ret == JPG_RET_FAILURE) {
        *pHandle = 0;
        JpgLeaveLockEx();
        return JPG_RET_FAILURE;
    }
    JpgLeaveLockEx();

    pJpgInst->isDecoder = TRUE;
    *pHandle = pJpgInst;

    pDecInfo = &pJpgInst->JpgInfo->decInfo;
    memset(pDecInfo, 0x00, sizeof(JpgDecInfo));

    pDecInfo->streamWrPtr = pop->bitstreamBuffer;
    pDecInfo->streamRdPtr = pop->bitstreamBuffer;

    pDecInfo->streamBufStartAddr = pop->bitstreamBuffer;
    pDecInfo->streamBufSize     = pop->bitstreamBufferSize;
    pDecInfo->streamBufEndAddr  = pop->bitstreamBuffer + pop->bitstreamBufferSize;
    pDecInfo->pBitStream        = pop->pBitStream;
    pDecInfo->streamEndian      = pop->streamEndian;
    pDecInfo->frameEndian       = pop->frameEndian;
    pDecInfo->chromaInterleave  = pop->chromaInterleave;
    pDecInfo->packedFormat      = pop->packedFormat;
    pDecInfo->roiEnable         = pop->roiEnable;
    pDecInfo->roiWidth          = pop->roiWidth;
    pDecInfo->roiHeight         = pop->roiHeight;
    pDecInfo->roiOffsetX        = pop->roiOffsetX;
    pDecInfo->roiOffsetY        = pop->roiOffsetY;
    pDecInfo->pixelJustification = pop->pixelJustification;
    pDecInfo->intrEnableBit     = pop->intrEnableBit;
    pJpgInst->sliceInstMode     = 0;    /* The decoder does not support the slice-decoding */
    pDecInfo->rotationIndex     = pop->rotation / 90;
    pDecInfo->mirrorIndex       = pop->mirror;

    /* convert output format */
    switch (pop->outputFormat) {
    case FORMAT_400: ret = JPG_RET_INVALID_PARAM; break;
    case FORMAT_420: pDecInfo->ofmt = O_FMT_420;  break;
    case FORMAT_422: pDecInfo->ofmt = O_FMT_422;  break;
    case FORMAT_440: ret = JPG_RET_INVALID_PARAM; break;
    case FORMAT_444: pDecInfo->ofmt = O_FMT_444;  break;
    case FORMAT_MAX: pDecInfo->ofmt = O_FMT_NONE; break;
    default:         ret = JPG_RET_INVALID_PARAM; break;
    }

    pDecInfo->userqMatTab = 0;
    pDecInfo->decIdx = 0;

    return ret;
}

JpgRet JPU_DecClose(JpgDecHandle handle)
{
    JpgRet ret;
    JpgInst * pJpgInst = handle;;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS) {
        return ret;
    }

    JpgEnterLockEx();

    if (GetJpgPendingInstEx(pJpgInst)) {
        JpgLeaveLockEx();
        return JPG_RET_FRAME_NOT_COMPLETE;
    }

    FreeJpgInstance(pJpgInst);
    JpgLeaveLockEx();

    return JPG_RET_SUCCESS;
}

JpgRet JPU_DecGetInitialInfo(JpgDecHandle handle, JpgDecInitialInfo * info)
{
    JpgInst*    pJpgInst = handle;
    JpgDecInfo* pDecInfo;
    JpgRet      ret;
    int         err;

    if (pJpgInst == NULL) {
        return JPG_RET_INVALID_PARAM;
    }

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    if (info == 0) {
        return JPG_RET_INVALID_PARAM;
    }

    pDecInfo = &pJpgInst->JpgInfo->decInfo;

    if (0 >= (err=JpegDecodeHeader(pJpgInst, pDecInfo))) {

        if (err == (Uint32)-4) {
            return JPG_RET_INSUFFICIENT_BITSTREAM_BUFFER;
        }
        /* The value of -2 indicates that the bitstream in the buffer is not enough to decode a header. */
        JLOG(ERR,"JpegDecodeHeader err:%d\n", err);
        return (-2 == err) ? JPG_RET_BIT_EMPTY : JPG_RET_FAILURE;
    }

    if (pDecInfo->jpg12bit == TRUE && g_JpuAttributes.support12bit == FALSE) {
        return JPG_RET_NOT_SUPPORT;
    }

    info->picWidth = pDecInfo->picWidth;
    info->picHeight = pDecInfo->picHeight;
    info->minFrameBufferCount = 1;
    info->sourceFormat = (FrameFormat)pDecInfo->format;
    info->ecsPtr = pDecInfo->ecsPtr;

    JLOG(INFO,"JPU_DecGetInitialInfo sourceFormat:%d\n", info->sourceFormat);

    pDecInfo->initialInfoObtained = 1;
    pDecInfo->minFrameBufferNum = 1;


    if ((pDecInfo->packedFormat == PACKED_FORMAT_444) && (pDecInfo->format != FORMAT_444)) {
        return JPG_RET_INVALID_PARAM;
    }

    if (pDecInfo->roiEnable) {
        if (pDecInfo->format == FORMAT_400) {
            pDecInfo->roiMcuWidth = pDecInfo->roiWidth/8;
        }
        else  {
            pDecInfo->roiMcuWidth = pDecInfo->roiWidth/pDecInfo->mcuWidth;
        }
        pDecInfo->roiMcuHeight = pDecInfo->roiHeight/pDecInfo->mcuHeight;
        pDecInfo->roiMcuOffsetX = pDecInfo->roiOffsetX/pDecInfo->mcuWidth;
        pDecInfo->roiMcuOffsetY = pDecInfo->roiOffsetY/pDecInfo->mcuHeight;

        if ((pDecInfo->roiOffsetX > pDecInfo->alignedWidth)
            || (pDecInfo->roiOffsetY > pDecInfo->alignedHeight)
            || (pDecInfo->roiOffsetX + pDecInfo->roiWidth > pDecInfo->alignedWidth)
            || (pDecInfo->roiOffsetY + pDecInfo->roiHeight > pDecInfo->alignedHeight))
            return JPG_RET_INVALID_PARAM;

        if (pDecInfo->format == FORMAT_400) {
            if (((pDecInfo->roiOffsetX + pDecInfo->roiWidth) < 8) || ((pDecInfo->roiOffsetY + pDecInfo->roiHeight) < pDecInfo->mcuHeight))
                return JPG_RET_INVALID_PARAM;
        }
        else {
            if (((pDecInfo->roiOffsetX + pDecInfo->roiWidth) < pDecInfo->mcuWidth) || ((pDecInfo->roiOffsetY + pDecInfo->roiHeight) < pDecInfo->mcuHeight))
                return JPG_RET_INVALID_PARAM;
        }

        if (pDecInfo->format == FORMAT_400)
            pDecInfo->roiFrameWidth = pDecInfo->roiMcuWidth * 8;
        else
            pDecInfo->roiFrameWidth = pDecInfo->roiMcuWidth * pDecInfo->mcuWidth;
        pDecInfo->roiFrameHeight = pDecInfo->roiMcuHeight*pDecInfo->mcuHeight;
        info->roiFrameWidth   = pDecInfo->roiFrameWidth;
        info->roiFrameHeight  = pDecInfo->roiFrameHeight;
        info->roiFrameOffsetX = pDecInfo->roiMcuOffsetX*pDecInfo->mcuWidth;
        info->roiFrameOffsetY = pDecInfo->roiMcuOffsetY*pDecInfo->mcuHeight;
    }
    info->bitDepth        = pDecInfo->bitDepth;


    return JPG_RET_SUCCESS;
}


JpgRet JPU_DecRegisterFrameBuffer(JpgDecHandle handle, FrameBuffer * bufArray, int num, int stride)
{
    JpgInst * pJpgInst;
    JpgDecInfo * pDecInfo;
    JpgRet ret;


    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    pJpgInst = handle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;


    if (!pDecInfo->initialInfoObtained) {
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }

    if (bufArray == 0) {
        return JPG_RET_INVALID_FRAME_BUFFER;
    }

    if (num < pDecInfo->minFrameBufferNum) {
        return JPG_RET_INSUFFICIENT_FRAME_BUFFERS;
    }

    if ((stride % 8) != 0) {
        return JPG_RET_INVALID_STRIDE;
    }

    pDecInfo->frameBufPool    = bufArray;
    pDecInfo->numFrameBuffers = num;
    pDecInfo->stride          = stride;
    pDecInfo->stride_c        = bufArray[0].strideC;

    return JPG_RET_SUCCESS;
}

JpgRet JPU_DecGetBitstreamBuffer(JpgDecHandle handle,
    PhysicalAddress * prdPtr,
    PhysicalAddress * pwrPtr,
    int * size)
{
    JpgInst * pJpgInst;
    JpgDecInfo * pDecInfo;
    JpgRet ret;
    PhysicalAddress rdPtr;
    PhysicalAddress wrPtr;
    int room;
    Int32 instRegIndex;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    pJpgInst = handle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }

    if (GetJpgPendingInstEx(pJpgInst) == pJpgInst) {
        rdPtr = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_RD_PTR_REG);
    }
    else {
        rdPtr = pDecInfo->streamRdPtr;
    }

    wrPtr = pDecInfo->streamWrPtr;

    if (wrPtr == pDecInfo->streamBufStartAddr) {
        if (pDecInfo->frameOffset == 0) {
            room = (pDecInfo->streamBufEndAddr-pDecInfo->streamBufStartAddr);
        }
        else {
            room = (pDecInfo->frameOffset);
        }
    }
    else {
        room = (pDecInfo->streamBufEndAddr - wrPtr);
    }

    room = ((room>>9)<<9); // multiple of 512

    if (prdPtr) *prdPtr = rdPtr;
    if (pwrPtr) *pwrPtr = wrPtr;
    if (size)   *size = room;

    JLOG(INFO,"JPU_DecGetBitstreamBuffer rdPtr:%llx wrPtr:%llx size:%d\n", rdPtr, wrPtr, room);

    return JPG_RET_SUCCESS;
}


JpgRet JPU_DecUpdateBitstreamBuffer(JpgDecHandle handle, int size)
{
    JpgInst * pJpgInst;
    JpgDecInfo * pDecInfo;
    PhysicalAddress wrPtr;
    PhysicalAddress rdPtr;
    JpgRet ret;
    int        val = 0;
    Uint32 strmCntOfEos;
    Int32 instRegIndex;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    pJpgInst = handle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;
    wrPtr = pDecInfo->streamWrPtr;

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }

    if (size == 0)
    {
        if (GetJpgPendingInstEx(pJpgInst) == pJpgInst) {
            val = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_STRM_CTRL_REG);
            strmCntOfEos = (wrPtr-pDecInfo->streamBufStartAddr) / 256;
            if ((wrPtr-pDecInfo->streamBufStartAddr) % 256) {
                strmCntOfEos = strmCntOfEos + 1;
            }
            val |= (1UL << 31);
            val &= ~(0xffffff);
            val |= (strmCntOfEos&0xffffff);
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_STRM_CTRL_REG, val);
        }
        pDecInfo->streamEndflag = 1;
        JLOG(INFO,"JPU_DecUpdateBitstreamBuffer end \n");
        return JPG_RET_SUCCESS;
    }

    wrPtr = pDecInfo->streamWrPtr;
    wrPtr += size;

    if (wrPtr == pDecInfo->streamBufEndAddr) {
        wrPtr = pDecInfo->streamBufStartAddr;
    }

    pDecInfo->streamWrPtr = wrPtr;

    if (GetJpgPendingInstEx(pJpgInst) == pJpgInst) {
        rdPtr = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_RD_PTR_REG);

        if (rdPtr >= pDecInfo->streamBufEndAddr) {
            JLOG(INFO, "inst=%d !!!!! WRAP-AROUND !!!!!\n", pJpgInst->instIndex);
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_CUR_POS_REG, 0);
        }

        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_WR_PTR_REG, wrPtr);
        if (wrPtr == pDecInfo->streamBufStartAddr) {
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_END_ADDR_REG, pDecInfo->streamBufEndAddr);
        }
        else {
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_END_ADDR_REG, wrPtr);
        }
    }
    else {
        rdPtr = pDecInfo->streamRdPtr;
    }

    pDecInfo->streamRdPtr = rdPtr;


    JLOG(INFO,"JPU_DecUpdateBitstreamBuffer rdPtr:%llx wrPtr:%llx size:%d\n", rdPtr, wrPtr, size);

    return JPG_RET_SUCCESS;
}

JpgRet JPU_SWReset(int core_idx, JpgHandle handle)
{
    Uint32 val;
    int clock_state;
    JpgInst *pJpgInst;

    pJpgInst = ((JpgInst *)handle);
    clock_state = jdi_get_clock_gate(core_idx);

    if (clock_state == 0)
        jdi_set_clock_gate(core_idx, 1);

    if (handle)
        jdi_log(core_idx, JDI_LOG_CMD_RESET, 1, pJpgInst->instIndex);

    val = JpuReadRegExt(core_idx, MJPEG_INST_CTRL_START_REG);
    val |= (1<<2);
    JpuWriteRegExt(core_idx, MJPEG_INST_CTRL_START_REG, val);
    if (jdi_wait_inst_ctrl_busy(core_idx, JPU_INST_CTRL_TIMEOUT_MS, MJPEG_INST_CTRL_STATUS_REG, INST_CTRL_IDLE) == -1) {// wait for INST_CTRL become IDLE
        if (handle)
            jdi_log(core_idx, JDI_LOG_CMD_RESET, 0, pJpgInst->instIndex);
        val &= ~(1<<2);
        JpuWriteRegExt(core_idx, MJPEG_INST_CTRL_START_REG, val);
        if (clock_state == 0)
            jdi_set_clock_gate(core_idx, 0);
        return JPG_RET_INST_CTRL_ERROR;
    }
    val = 0x00;
    JpuWriteRegExt(core_idx, MJPEG_INST_CTRL_START_REG, val);

    if (handle)
        jdi_log(core_idx, JDI_LOG_CMD_RESET, 0, pJpgInst->instIndex);

    if (clock_state == 0)
        jdi_set_clock_gate(core_idx, 0);

    return JPG_RET_SUCCESS;
}
JpgRet JPU_HWReset(int core_idx)
{
    if (jdi_hw_reset(core_idx) < 0 )
        return JPG_RET_FAILURE;

    return JPG_RET_SUCCESS;
}

JpgRet JPU_DecSetRdPtr(JpgDecHandle handle, PhysicalAddress addr, BOOL updateWrPtr)
{
    JpgInst*    pJpgInst;
    JpgDecInfo* pDecInfo;
    JpgRet      ret;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    pJpgInst = (JpgInst*)handle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;

    JpgEnterLock();

    if (GetJpgPendingInstEx(pJpgInst)) {
        JpgLeaveLock();
        return JPG_RET_FRAME_NOT_COMPLETE;
    }
    pDecInfo->streamRdPtr = addr;
    if (updateWrPtr)
        pDecInfo->streamWrPtr = addr;

    pDecInfo->frameOffset = addr - pDecInfo->streamBufStartAddr;
    pDecInfo->consumeByte = 0;

    JpuWriteReg(pJpgInst->coreIndex, MJPEG_BBC_RD_PTR_REG, pDecInfo->streamRdPtr);

    JpgLeaveLock();

    return JPG_RET_SUCCESS;
}

JpgRet JPU_DecSetRdPtrEx(JpgDecHandle handle, PhysicalAddress addr, BOOL updateWrPtr)
{
    JpgInst*    pJpgInst;
    JpgDecInfo* pDecInfo;
    JpgRet      ret;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    pJpgInst = (JpgInst*)handle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;

    JpgEnterLock();

    if (GetJpgPendingInstEx(pJpgInst)) {
        JpgLeaveLock();
        return JPG_RET_FRAME_NOT_COMPLETE;
    }
    pDecInfo->streamRdPtr = addr;
    pDecInfo->streamBufStartAddr = addr;
    if (updateWrPtr)
        pDecInfo->streamWrPtr = addr;

    pDecInfo->frameOffset = 0;
    pDecInfo->consumeByte = 0;

    JpuWriteReg(pJpgInst->coreIndex, MJPEG_BBC_RD_PTR_REG, pDecInfo->streamRdPtr);

    JpgLeaveLock();

    return JPG_RET_SUCCESS;
}

JpgRet JPU_DecStartOneFrame(JpgDecHandle handle, JpgDecParam *param)
{
    JpgInst * pJpgInst;
    JpgDecInfo * pDecInfo;
    JpgRet ret;
    Uint32 val;
    PhysicalAddress rdPtr, wrPtr;
    BOOL    is12Bit     = FALSE;
    BOOL    ppuEnable   = FALSE;
    Int32 instRegIndex;
    BOOL bTableInfoUpdate;
    Uint32 strmCntOfEos;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;


    pJpgInst = handle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;
    is12Bit  = (pDecInfo->bitDepth == 8) ? FALSE : TRUE;

    if (pDecInfo->frameBufPool == 0) { // This means frame buffers have not been registered.
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }

    JpgEnterLock();
    if (GetJpgPendingInstEx(pJpgInst) == pJpgInst) {
        JpgLeaveLock();
        return JPG_RET_FRAME_NOT_COMPLETE;
    }

    if (pDecInfo->frameOffset < 0) {
        SetJpgPendingInstEx(0, pJpgInst->instIndex);
        return JPG_RET_EOS;
    }

    pDecInfo->q_prec0 = 0;
    pDecInfo->q_prec1 = 0;
    pDecInfo->q_prec2 = 0;
    pDecInfo->q_prec3 = 0;

    //check for stream empty case
    if (pDecInfo->streamEndflag == 0) {
        rdPtr = pDecInfo->streamRdPtr;
        wrPtr = pDecInfo->streamWrPtr;
        if (wrPtr == pDecInfo->streamBufStartAddr)
            wrPtr = pDecInfo->streamBufEndAddr;
        if (rdPtr > wrPtr) { // wrap-around case
        }
        else {
            if (wrPtr - rdPtr < JPU_GBU_WRAP_SIZE) {
                JpgLeaveLock();
                return JPG_RET_BIT_EMPTY;
            }
        }
    }

    {
        val = JpegDecodeHeader(pJpgInst, pDecInfo);
        if (val == 0) {
            JpgLeaveLock();
            return JPG_RET_FAILURE;
        }

        if (val == (Uint32)-4) {
            JpgLeaveLock();
            return JPG_RET_INSUFFICIENT_BITSTREAM_BUFFER; // bitstream buffer siz is not enough.
        }
        if (val == (Uint32)-2) {    // wrap around case
            pDecInfo->frameOffset = 0;
            pDecInfo->ecsPtr = 0;
            val = JpegDecodeHeader(pJpgInst, pDecInfo);
            if (val == 0) {
                JpgLeaveLock();
                return JPG_RET_FAILURE;
            }
        }

        if (val == (Uint32)-1) {    //stream empty case
            if (pDecInfo->streamEndflag == 1) {
                SetJpgPendingInstEx(0, pJpgInst->instIndex);
                pDecInfo->frameOffset = -1;
                if (pJpgInst->sliceInstMode == TRUE) {
                    JpgLeaveLock();
                }
                return JPG_RET_EOS;
            }
            JpgLeaveLock();
            return JPG_RET_BIT_EMPTY;
        }
        if (val == (Uint32)-4) {
            JpgLeaveLock();
            return JPG_RET_INSUFFICIENT_BITSTREAM_BUFFER;
        }
    }


    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_INTR_MASK_REG, ((~pDecInfo->intrEnableBit) & 0x3ff));
    /* The registers related to the slice encoding should be clear */
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_SLICE_INFO_REG,    pDecInfo->alignedHeight);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_SLICE_DPB_POS_REG, pDecInfo->alignedHeight);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_SLICE_POS_REG,     0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_SETMB_REG,     0);

    if (pDecInfo->streamRdPtr == pDecInfo->streamBufEndAddr) {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_CUR_POS_REG, 0);
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_TCNT_REG, 0);
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, (MJPEG_GBU_TCNT_REG+4), 0);
    }

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_WR_PTR_REG, pDecInfo->streamWrPtr);
    if (pDecInfo->streamWrPtr == pDecInfo->streamBufStartAddr) {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_END_ADDR_REG, pDecInfo->streamBufEndAddr);
    }
    else {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_END_ADDR_REG, JPU_CEIL(256, pDecInfo->streamWrPtr));
    }

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_BAS_ADDR_REG, pDecInfo->streamBufStartAddr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_TCNT_REG, 0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, (MJPEG_GBU_TCNT_REG+4), 0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_ERRMB_REG, 0);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_CTRL_REG, is12Bit << 31 | pDecInfo->q_prec0 << 30 | pDecInfo->q_prec1 << 29 | pDecInfo->q_prec2 << 28 | pDecInfo->q_prec3 << 27 |
                                                       pDecInfo->huffAcIdx << 13 | pDecInfo->huffDcIdx << 7 | pDecInfo->userHuffTab << 6 |
                                                       (JPU_CHECK_WRITE_RESPONSE_BVALID_SIGNAL << 2) | 0);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_SIZE_REG, (pDecInfo->alignedWidth << 16) | pDecInfo->alignedHeight);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_OP_INFO_REG,  pDecInfo->busReqNum);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_MCU_INFO_REG, (pDecInfo->mcuBlockNum&0x0f) << 17 | (pDecInfo->compNum&0x07) << 14    |
                                                      (pDecInfo->compInfo[0]&0x3f) << 8  | (pDecInfo->compInfo[1]&0x0f) << 4 |
                                                      (pDecInfo->compInfo[2]&0x0f));

    //enable intlv NV12: 10, NV21: 11
    //packedFormat:0 => 4'd0
    //packedFormat:1,2,3,4 => 4, 5, 6, 7,
    //packedFormat:5 => 8
    //packedFormat:6 => 9
    val = (pDecInfo->ofmt << 9) | (pDecInfo->frameEndian << 6) | ((pDecInfo->chromaInterleave==0)?0:(pDecInfo->chromaInterleave==1)?2:3);
    if (pDecInfo->packedFormat == PACKED_FORMAT_NONE) {
        val |= (0<<5) | (0<<4);
    }
    else if (pDecInfo->packedFormat == PACKED_FORMAT_444) {
        val |= (1<<5) | (0<<4) | (0<<2);
    }
    else {
        val |= (0<<5) | (1<<4) | ((pDecInfo->packedFormat-1)<<2);
    }
    val |= (pDecInfo->pixelJustification << 11);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_CONFIG_REG, val);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_RST_INTVAL_REG, pDecInfo->rstIntval);

    if (param) {
        if (param->scaleDownRatioWidth > 0 )
            pDecInfo->iHorScaleMode = param->scaleDownRatioWidth;
        if (param->scaleDownRatioHeight > 0 )
            pDecInfo->iVerScaleMode = param->scaleDownRatioHeight;
    }
    if (pDecInfo->iHorScaleMode | pDecInfo->iVerScaleMode)
        val = ((pDecInfo->iHorScaleMode&0x3)<<2) | ((pDecInfo->iVerScaleMode&0x3)) | 0x10 ;
    else {
        val = 0;
    }
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_SCL_INFO_REG, val);

    bTableInfoUpdate = FALSE;
    if (pDecInfo->userHuffTab) {
        bTableInfoUpdate = TRUE;
    }

    if (bTableInfoUpdate == TRUE) {
        if (is12Bit == TRUE){
            if (!JpgDecHuffTabSetUp_12b(pJpgInst,pDecInfo, instRegIndex)) {
                JpgLeaveLock();
                return JPG_RET_INVALID_PARAM;
            }
        }else{
            if (!JpgDecHuffTabSetUp(pJpgInst, pDecInfo, instRegIndex)) {
                JpgLeaveLock();
                return JPG_RET_INVALID_PARAM;
            }
        }
    }

    bTableInfoUpdate = TRUE; // it always should be TRUE for multi-instance
    if (bTableInfoUpdate == TRUE) {
        if (!JpgDecQMatTabSetUp(pJpgInst, pDecInfo, instRegIndex)) {
            JpgLeaveLock();
            return JPG_RET_INVALID_PARAM;
        }
    }

    if (JpgDecGramSetup(pJpgInst, pDecInfo, instRegIndex, JPU_INST_CTRL_TIMEOUT_MS)) {
        JLOG(ERR, "JpgDecGramSetup fail\n");
        return JPG_RET_FAILURE;
    }

    if (pDecInfo->streamEndflag == 1) {
        val = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_STRM_CTRL_REG);
        if ((val & (1UL << 31)) == 0 ) {

            strmCntOfEos = (pDecInfo->streamWrPtr-pDecInfo->streamBufStartAddr) / 256;
            if ((pDecInfo->streamWrPtr-pDecInfo->streamBufStartAddr) % 256) {
                strmCntOfEos = strmCntOfEos + 1;
            }
            val |= (1UL << 31);
            val &= ~(0xffffff);
            val |= (strmCntOfEos&0xffffff);
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_STRM_CTRL_REG, val);
        }
    }
    else {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_STRM_CTRL_REG, 0);
    }

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_RST_INDEX_REG, 0);    // RST index at the beginning.
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_RST_COUNT_REG, 0);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPCM_DIFF_Y_REG, 0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPCM_DIFF_CB_REG, 0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPCM_DIFF_CR_REG, 0);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_FF_RPTR_REG, pDecInfo->bitPtr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_CTRL_REG, 3);

    ppuEnable = (pDecInfo->rotationIndex > 0) || (pDecInfo->mirrorIndex > 0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_ROT_INFO_REG, (ppuEnable<<4) | (pDecInfo->mirrorIndex<<2) | pDecInfo->rotationIndex);

    val = (pDecInfo->frameIdx % pDecInfo->numFrameBuffers);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_BASE00_REG, pDecInfo->frameBufPool[val].bufY);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_BASE01_REG, pDecInfo->frameBufPool[val].bufCb);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_BASE02_REG, pDecInfo->frameBufPool[val].bufCr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_YSTRIDE_REG, pDecInfo->stride);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_CSTRIDE_REG, pDecInfo->stride_c);

    if (pDecInfo->roiEnable) {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_CLP_INFO_REG, 1);
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_CLP_BASE_REG, pDecInfo->roiOffsetX << 16 | pDecInfo->roiOffsetY);    // pixel unit
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_CLP_SIZE_REG, (pDecInfo->roiFrameWidth << 16) | pDecInfo->roiFrameHeight); // pixel Unit
    }
    else {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_CLP_INFO_REG, 0);
    }

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_STATUS_REG, 0x3ff);

    if (pJpgInst->loggingEnable)
        jdi_log(pJpgInst->coreIndex, JDI_LOG_CMD_PICRUN, 1, instRegIndex);

    jpu_enable_irq(pJpgInst->coreIndex);
    if (pJpgInst->sliceInstMode == TRUE) {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_START_REG, (1<<JPG_DEC_SLICE_ENABLE_START_PIC));
    }
    else {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_START_REG, (1<<JPG_ENABLE_START_PIC));
    }

    pDecInfo->decIdx++;

    SetJpgPendingInstEx(pJpgInst, pJpgInst->instIndex);
    if (pJpgInst->sliceInstMode == TRUE) {
        JpgLeaveLock();
    }
    return JPG_RET_SUCCESS;
}


JpgRet JPU_SetJpgPendingInstEx(JpgInst* pJpgInst, void *data)
{
    JpgRet      ret;

    ret = CheckJpgInstValidity(pJpgInst);
    if (ret != JPG_RET_SUCCESS) {
        return ret;
    }

    SetJpgPendingInstEx(data, pJpgInst->instIndex);

    return JPG_RET_SUCCESS;
}

JpgRet JPU_DecGetOutputInfo(JpgDecHandle handle, JpgDecOutputInfo * info)
{
    JpgInst*    pJpgInst;
    JpgDecInfo* pDecInfo;
    JpgRet      ret;
    Uint32      val = 0;
    Int32       instRegIndex;
    Uint32 reason;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS) {
        return ret;
    }

    if (info == 0) {
        return JPG_RET_INVALID_PARAM;
    }

    pJpgInst = handle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }

    if (pJpgInst->sliceInstMode == TRUE) {
        JpgEnterLock();
    }

    if (pJpgInst != GetJpgPendingInstEx(pJpgInst)) {
        JpgLeaveLock();
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }


    if (pDecInfo->frameOffset < 0)
    {
        info->numOfErrMBs = 0;
        info->decodingSuccess = 1;
        info->indexFrameDisplay = -1;
        SetJpgPendingInstEx(0, pJpgInst->instIndex);

        JpgLeaveLock();
        return JPG_RET_SUCCESS;
    }

    if (pDecInfo->roiEnable) {
        info->decPicWidth = pDecInfo->roiMcuWidth*pDecInfo->mcuWidth;
        info->decPicHeight = pDecInfo->roiMcuHeight*pDecInfo->mcuHeight;
    }
    else {
        info->decPicWidth = pDecInfo->alignedWidth;
        info->decPicHeight = pDecInfo->alignedHeight;
    }

    info->decPicWidth  >>= pDecInfo->iHorScaleMode;
    info->decPicHeight >>= pDecInfo->iVerScaleMode;

    info->indexFrameDisplay = (pDecInfo->frameIdx%pDecInfo->numFrameBuffers);
    info->consumedByte = (JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_TCNT_REG))/8;
    pDecInfo->streamRdPtr = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_RD_PTR_REG);
    pDecInfo->consumeByte = info->consumedByte + pDecInfo->ecsPtr - 16;
    info->bytePosFrameStart = pDecInfo->frameOffset;
    info->ecsPtr = pDecInfo->ecsPtr;
    info->rdPtr  = pDecInfo->streamRdPtr;
    info->wrPtr  = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_WR_PTR_REG);

    pDecInfo->ecsPtr = 0;
    pDecInfo->headerSize = 0;
    pDecInfo->frameIdx++;

    val = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_STATUS_REG);
    reason = val;
    if (reason & (1<<INT_JPU_DONE)) {
        info->decodingSuccess  = 1;
        info->numOfErrMBs      = 0;
    }
    else if (reason & (1<<INT_JPU_ERROR)){
        info->numOfErrMBs = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_ERRMB_REG);
        info->decodingSuccess = 0;
        // + make fsm of instance controller to IDLE
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_START_REG, 0);
        JPU_SWTopReset(pJpgInst->coreIndex);
        JPU_SWReset(pJpgInst->coreIndex, NULL);
        JpuWriteReg(pJpgInst->coreIndex, MJPEG_INST_CTRL_START_REG, (1<<0));
        reason = 0;
    }

    if (reason != 0)
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_STATUS_REG, reason);
    info->frameCycle = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_CYCLE_INFO_REG);
    if (pJpgInst->loggingEnable)
        jdi_log(pJpgInst->coreIndex, JDI_LOG_CMD_PICRUN, 0, instRegIndex);

    JLOG(INFO, "decode jpeg hw Cycles:%d cost_time_us:%d\n", info->frameCycle, info->frameCycle/650);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_START_REG, 0);

    val = JpuReadReg(pJpgInst->coreIndex, MJPEG_INST_CTRL_STATUS_REG);
    val &= ~(1UL<<instRegIndex);
    JpuWriteReg(pJpgInst->coreIndex, MJPEG_INST_CTRL_STATUS_REG, val);

    SetJpgPendingInstEx(0, pJpgInst->instIndex);
    // jpu_enable_irq(pJpgInst->coreIndex);

    JpgLeaveLock();

    return JPG_RET_SUCCESS;
}


JpgRet JPU_DecGiveCommand(
    JpgDecHandle handle,
    JpgCommand cmd,
    void * param)
{
    JpgInst * pJpgInst;
    JpgDecInfo * pDecInfo;
    JpgRet ret;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    pJpgInst = handle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;
    switch (cmd)
    {
    case SET_JPG_SCALE_HOR:
        {
            int scale;
            scale = *(int *)param;
            if ((pDecInfo->alignedWidth >> (scale&0x3)) < 16 ||
                (pDecInfo->alignedHeight >> (scale&0x3)) < 16 ) {
                if (scale) {
                    return JPG_RET_INVALID_PARAM;
                }
            }
            pDecInfo->iHorScaleMode = scale;
            break;
        }
    case SET_JPG_SCALE_VER:
        {
            int scale;
            scale = *(int *)param;
            if ((pDecInfo->alignedWidth >> (scale&0x3)) < 16 ||
                (pDecInfo->alignedHeight >> (scale&0x3)) < 16 ) {
                if (scale) {
                    return JPG_RET_INVALID_PARAM;
                }
            }
            pDecInfo->iVerScaleMode = scale;
            break;
        }
    case ENABLE_LOGGING:
        {
            pJpgInst->loggingEnable = 1;
        }
        break;
    case DISABLE_LOGGING:
        {
            pJpgInst->loggingEnable = 0;
        }
        break;
    default:
        return JPG_RET_INVALID_COMMAND;
    }
    return JPG_RET_SUCCESS;
}

JpgRet JPU_EncOpen(JpgEncHandle * pHandle, JpgEncOpenParam * pop)
{
    JpgInst*    pJpgInst;
    JpgEncInfo* pEncInfo;
    JpgRet      ret;
    Uint32      mcuWidth, mcuHeight;
    Uint32      comp0McuWidth, comp0McuHeight;
    FrameFormat compFormat;

    ret = CheckJpgEncOpenParam(pop, &g_JpuAttributes);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR,"CheckJpgEncOpenParam failed.\n");
        return ret;
    }

    JpgEnterLockEx();
    ret = GetJpgInstance(&pJpgInst);
    if (ret == JPG_RET_FAILURE) {
        JpgLeaveLockEx();
        JLOG(ERR,"GetJpgInstance failed.\n");
        return JPG_RET_FAILURE;
    }
    JpgLeaveLockEx();

    pJpgInst->isDecoder = FALSE;
    *pHandle = pJpgInst;
    pEncInfo = &pJpgInst->JpgInfo->encInfo;
    memset(pEncInfo, 0x00, sizeof(JpgEncInfo));
    pEncInfo->streamRdPtr        = pop->bitstreamBuffer;
    pEncInfo->streamWrPtr        = pop->bitstreamBuffer;
    pEncInfo->sliceHeight        = pop->sliceHeight;
    pEncInfo->intrEnableBit      = pop->intrEnableBit;

    pEncInfo->streamBufStartAddr = pop->bitstreamBuffer;
    pEncInfo->streamBufEndAddr   = pop->bitstreamBuffer + pop->bitstreamBufferSize - (4096);
    pEncInfo->streamEndian       = pop->streamEndian;
    pEncInfo->frameEndian        = pop->frameEndian;
    pEncInfo->chromaInterleave   = pop->chromaInterleave;
    pEncInfo->format             = pop->sourceFormat;
    pEncInfo->picWidth           = pop->picWidth;
    pEncInfo->picHeight          = pop->picHeight;

    // Picture size alignment
    if (pEncInfo->format == FORMAT_420 || pEncInfo->format == FORMAT_422) {
        pEncInfo->alignedWidth = JPU_CEIL(16, pEncInfo->picWidth);
        mcuWidth = 16;
    }
    else {
        pEncInfo->alignedWidth = JPU_CEIL(8, pEncInfo->picWidth);
        if (pEncInfo->format == FORMAT_400)
            mcuWidth = (pEncInfo->picWidth >= 32) ? 32 : 8;
        else
            mcuWidth = 8;
    }

    if (pEncInfo->format == FORMAT_420 || pEncInfo->format == FORMAT_440) {
        pEncInfo->alignedHeight = JPU_CEIL(16, pEncInfo->picHeight);
        mcuHeight = 16;
    }
    else {
        pEncInfo->alignedHeight = JPU_CEIL(8, pEncInfo->picHeight);
        mcuHeight = 8;
    }

    pEncInfo->mcuWidth  = mcuWidth;
    pEncInfo->mcuHeight = mcuHeight;

    comp0McuWidth  = pEncInfo->mcuWidth;
    comp0McuHeight = pEncInfo->mcuHeight;
    if (pop->rotation == 90 || pop->rotation == 270) {
        if (pEncInfo->format == FORMAT_420 || pEncInfo->format == FORMAT_422) {
            comp0McuWidth = 16;
        }
        else  {
            comp0McuWidth = 8;
        }

        if (pEncInfo->format == FORMAT_420 || pEncInfo->format == FORMAT_440) {
            comp0McuHeight = 16;
        }
        else if (pEncInfo->format == FORMAT_400) {
            comp0McuHeight = (pEncInfo->picHeight >= 32) ? 32 : 8;
        }
        else {
            comp0McuHeight = 8;
        }
    }

    if (pop->sliceInstMode == TRUE) {
        Uint32 ppuHeight = ((pop->rotation == 90 || pop->rotation == 270) == TRUE) ? pEncInfo->alignedWidth : pEncInfo->alignedHeight;
        if (pop->sliceHeight % pEncInfo->mcuHeight) {
            JLOG(ERR,"JPU_EncOpen  sliceHeight:%d mcuHeigth:%d.\n",pop->sliceHeight, pEncInfo->mcuHeight);
            return JPG_RET_INVALID_PARAM;
        }

        if (pop->sliceHeight > ppuHeight) {
            JpgLeaveLockEx();
            JLOG(ERR,"JPU_EncOpen  sliceHeight:%d ppuHeight:%d.\n",pop->sliceHeight, ppuHeight);
            return JPG_RET_INVALID_PARAM;
        }

        if (pop->sliceHeight < pEncInfo->mcuHeight) {
            JLOG(ERR,"JPU_EncOpen 2 sliceHeight:%d mcuHeigth:%d.\n",pop->sliceHeight, pEncInfo->mcuHeight);
            return JPG_RET_INVALID_PARAM;
        }
    }

    pJpgInst->sliceInstMode = pop->sliceInstMode;
    pEncInfo->rstIntval     = pop->restartInterval;
    pEncInfo->jpg12bit      = pop->jpg12bit;
    pEncInfo->q_prec0       = pop->q_prec0;
    pEncInfo->q_prec1       = pop->q_prec1;
    pEncInfo->pixelJustification = pop->pixelJustification;

    // huffman table
    // jpg12bit enable use array 0-7
    // jpg12bit disable use array 0-4
    memcpy(pEncInfo->huffVal,  pop->huffVal, sizeof(pEncInfo->huffVal));
    memcpy(pEncInfo->huffBits,  pop->huffBits, sizeof(pEncInfo->huffBits));

    // qmat table
    memcpy(pEncInfo->qMatTab, pop->qMatTab, sizeof(pEncInfo->qMatTab));

    compFormat = pEncInfo->format;
    if (pop->rotation == 90 || pop->rotation == 270) {
        compFormat = (pEncInfo->format==FORMAT_422) ? FORMAT_440 : (pEncInfo->format==FORMAT_440) ? FORMAT_422 : pEncInfo->format;
    }
    pEncInfo->pCInfoTab[0] = sJpuCompInfoTable[compFormat];
    pEncInfo->pCInfoTab[1] = pEncInfo->pCInfoTab[0] + 6;
    pEncInfo->pCInfoTab[2] = pEncInfo->pCInfoTab[1] + 6;
    pEncInfo->pCInfoTab[3] = pEncInfo->pCInfoTab[2] + 6;

    if (pop->packedFormat == PACKED_FORMAT_444 && pEncInfo->format != FORMAT_444) {
        JLOG(ERR,"JPU_EncOpen packedFormat:%d.\n", pop->packedFormat);
        return JPG_RET_INVALID_PARAM;
    }

    pEncInfo->packedFormat = pop->packedFormat;
    if (pEncInfo->format == FORMAT_400) {
        pEncInfo->compInfo[1] = 0;
        pEncInfo->compInfo[2] = 0;
    }
    else {
        pEncInfo->compInfo[1] = 5;
        pEncInfo->compInfo[2] = 5;
    }

    if (pEncInfo->format == FORMAT_400) {
        pEncInfo->compNum = 1;
    }
    else
        pEncInfo->compNum = 3;

    if (pEncInfo->format == FORMAT_420) {
        pEncInfo->mcuBlockNum = 6;
    }
    else if (pEncInfo->format == FORMAT_422) {
        pEncInfo->mcuBlockNum = 4;
    } else if (pEncInfo->format == FORMAT_440) { /* aka YUV440 */
        pEncInfo->mcuBlockNum = 4;
    } else if (pEncInfo->format == FORMAT_444) {
        pEncInfo->mcuBlockNum = 3;
    } else if (pEncInfo->format == FORMAT_400) {
        Uint32 picHeight = (90 == pop->rotation || 270 == pop->rotation) ? pEncInfo->picWidth : pEncInfo->picHeight;
        Uint32 picWidth = (90 == pop->rotation || 270 == pop->rotation) ? pEncInfo->picHeight : pEncInfo->picWidth;
        if (0 < pEncInfo->rstIntval && picHeight == pEncInfo->sliceHeight) {
            pEncInfo->mcuBlockNum = 1;
            comp0McuWidth         = 8;
            comp0McuHeight        = 8;
        }
        else {
            pEncInfo->mcuBlockNum = (picWidth >= 32) ? 4 : 1;
        }
    }
    pEncInfo->compInfo[0] = (comp0McuWidth >> 3) << 3 | (comp0McuHeight >> 3);

    pEncInfo->busReqNum = (pop->jpg12bit == FALSE) ? GetEnc8bitBusReqNum(pEncInfo->packedFormat, pEncInfo->format) :
                                                     GetEnc12bitBusReqNum(pEncInfo->packedFormat, pEncInfo->format);


    pEncInfo->tiledModeEnable = pop->tiledModeEnable;

    pEncInfo->encIdx = 0;
    pEncInfo->encDoneIdx = 0;
    pEncInfo->encSlicePosY = 0;
    pEncInfo->rotationIndex = pop->rotation/90;
    pEncInfo->mirrorIndex   = pop->mirror;

    return JPG_RET_SUCCESS;

}

JpgRet JPU_EncClose(JpgEncHandle handle)
{
    JpgRet ret;
    JpgInst * pJpgInst = handle;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    JpgEnterLockEx();

    if (GetJpgPendingInstEx(pJpgInst)) {
        JpgLeaveLockEx();
        return JPG_RET_FRAME_NOT_COMPLETE;
    }

    FreeJpgInstance(pJpgInst);

    JpgLeaveLockEx();

    return JPG_RET_SUCCESS;
}



JpgRet JPU_EncGetBitstreamBuffer( JpgEncHandle handle,
    PhysicalAddress * prdPtr,
    PhysicalAddress * pwrPtr,
    int * size)
{
    JpgInst * pJpgInst;
    JpgRet ret;
    Int32 instRegIndex;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    if ( prdPtr == 0 || pwrPtr == 0 || size == 0) {
        return JPG_RET_INVALID_PARAM;
    }

    pJpgInst = handle;

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }


    *pwrPtr = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_WR_PTR_REG);
    *prdPtr = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_RD_PTR_REG);

    *size = *pwrPtr - *prdPtr;

    return JPG_RET_SUCCESS;

}

JpgRet JPU_EncUpdateBitstreamBuffer(
    JpgEncHandle handle,
    int size)
{
    JpgInst * pJpgInst;
    JpgEncInfo * pEncInfo;
    JpgRet ret;
    Int32 instRegIndex;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    pJpgInst = handle;

    pEncInfo = &pJpgInst->JpgInfo->encInfo;
    if (GetJpgPendingInstEx(pJpgInst) == pJpgInst) {
        if (pJpgInst->sliceInstMode == TRUE) {
            instRegIndex = pJpgInst->instIndex;
        }
        else {
            instRegIndex = 0;
        }

        pEncInfo->streamWrPtr = JpuReadInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_WR_PTR_REG);
        pEncInfo->streamRdPtr += size;
        if ((pEncInfo->streamWrPtr >= pEncInfo->streamBufEndAddr) || (size == 0)) {    //Full Interrupt case. wrap to the start address
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_CUR_POS_REG, 0);
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_BAS_ADDR_REG, pEncInfo->streamBufStartAddr);
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_EXT_ADDR_REG, pEncInfo->streamBufStartAddr);
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_END_ADDR_REG, pEncInfo->streamBufEndAddr);
            pEncInfo->streamRdPtr = pEncInfo->streamBufStartAddr;
            pEncInfo->streamWrPtr = pEncInfo->streamBufStartAddr;
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_RD_PTR_REG, pEncInfo->streamRdPtr);
            JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_WR_PTR_REG, pEncInfo->streamWrPtr);
        }
    }
    else {
        pEncInfo->streamRdPtr = pEncInfo->streamBufStartAddr;
        pEncInfo->streamWrPtr = pEncInfo->streamBufStartAddr;
    }

    return JPG_RET_SUCCESS;
}


JpgRet JPU_EncStartOneFrame(JpgEncHandle handle, JpgEncParam * param)
{
    JpgInst * pJpgInst;
    JpgEncInfo * pEncInfo;
    FrameBuffer * pBasFrame;
    JpgRet ret;
    Uint32 val;
    Int32 instRegIndex;
    BOOL bTableInfoUpdate;
    Uint32  rotMirEnable = 0;
    Uint32  rotMirMode   = 0;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;

    pJpgInst = handle;
    pEncInfo = &pJpgInst->JpgInfo->encInfo;
    ret = CheckJpgEncParam(handle, param);
    if (ret != JPG_RET_SUCCESS) {
        JLOG(ERR,"JPU_EncStartOneFrame CheckJpgEncParam err ret:%x.\n",ret);
        return ret;
    }

    pBasFrame = param->sourceFrame;

    JpgEnterLock();

    if (GetJpgPendingInstEx(pJpgInst) == pJpgInst) {
        JpgLeaveLock();
        JLOG(ERR,"JPU_EncStartOneFrame GetJpgPendingInstEx err.\n");
        return JPG_RET_FRAME_NOT_COMPLETE;
    }

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_INTR_MASK_REG, ((~pEncInfo->intrEnableBit) & 0x3ff));
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_SLICE_INFO_REG, pEncInfo->sliceHeight);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_SLICE_DPB_POS_REG, pEncInfo->picHeight); // assume that the all of source buffer is available
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_SLICE_POS_REG, pEncInfo->encSlicePosY);
    val = (0 << 16) | (pEncInfo->encSlicePosY / pEncInfo->mcuHeight);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_SETMB_REG,val);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_CLP_INFO_REG, 0);    //off ROI enable due to not supported feature for encoder.

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_BAS_ADDR_REG, pEncInfo->streamBufStartAddr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_END_ADDR_REG, pEncInfo->streamBufEndAddr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_WR_PTR_REG, pEncInfo->streamWrPtr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_RD_PTR_REG, pEncInfo->streamRdPtr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_CUR_POS_REG, 0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_DATA_CNT_REG, JPU_GBU_SIZE / 4);    // 64 * 4 byte == 32 * 8 byte
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_EXT_ADDR_REG, pEncInfo->streamBufStartAddr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_INT_ADDR_REG, 0);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_BAS_ADDR_REG, pEncInfo->streamWrPtr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_EXT_ADDR_REG, pEncInfo->streamWrPtr);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_BPTR_REG, 0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_WPTR_REG, 0);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_BBSR_REG, 0);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_CTRL_REG, 0);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_BBER_REG, ((JPU_GBU_SIZE / 4) * 2) - 1);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_BBIR_REG, JPU_GBU_SIZE / 4);    // 64 * 4 byte == 32 * 8 byte
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_BBHR_REG, JPU_GBU_SIZE / 4);    // 64 * 4 byte == 32 * 8 byte

#define DEFAULT_TDI_TAI_DATA 0x055
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_CTRL_REG, (pEncInfo->jpg12bit<<31) | (pEncInfo->q_prec0<<30) | (pEncInfo->q_prec1<<29) | (pEncInfo->tiledModeEnable<<19) |
                                                      (DEFAULT_TDI_TAI_DATA<<7) | 0x18 | (1<<6) |  (1<<4) | (1<<3) |(JPU_CHECK_WRITE_RESPONSE_BVALID_SIGNAL<<2));
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_SCL_INFO_REG, 0);

    val = 0;
    //PackMode[3:0]: 0(NONE), 8(PACK444), 4,5,6,7(YUYV => VYUY)
    val = (pEncInfo->frameEndian << 6) | ((pEncInfo->chromaInterleave==0)?0:(pEncInfo->chromaInterleave==1)?2:3);
    if (pEncInfo->packedFormat == PACKED_FORMAT_NONE) {
        val |= (0<<5) | (0<<4) | (0<<2);
    }
    else if (pEncInfo->packedFormat == PACKED_FORMAT_444) {
        val |= (1<<5) | (0<<4) | (0<<2);
    }
    else {
        val |= (0<<5) | (1<<4) | ((pEncInfo->packedFormat-1)<<2);
    }
    val |= (pEncInfo->pixelJustification << 11);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_CONFIG_REG, val);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_RST_INTVAL_REG, pEncInfo->rstIntval);
    if (pEncInfo->encSlicePosY == 0) {
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_RST_INDEX_REG, 0);// RST index from 0.
        JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_STRM_CTRL_REG, 0);// clear BBC ctrl status.
    }

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_BBC_CTRL_REG, (pEncInfo->streamEndian << 1) | 1);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_OP_INFO_REG, pEncInfo->busReqNum);


    bTableInfoUpdate = FALSE;
    if (pJpgInst->sliceInstMode == FALSE) {
        bTableInfoUpdate = TRUE;
    }
    else {
        if (pEncInfo->encDoneIdx == 0 && pEncInfo->encSlicePosY == 0) {
            bTableInfoUpdate = TRUE;
        }
    }

    if (bTableInfoUpdate == TRUE) {
        // Load HUFFTab
        if (pEncInfo->jpg12bit){
            if (!JpgEncLoadHuffTab_12b(pJpgInst, instRegIndex)) {
                JpgLeaveLock();
                JLOG(ERR,"JPU_EncStartOneFrame JpgEncLoadHuffTab_12b err.\n");
                return JPG_RET_INVALID_PARAM;
            }
        }else{
            if (!JpgEncLoadHuffTab(pJpgInst, instRegIndex)) {
                JpgLeaveLock();
                JLOG(ERR,"JPU_EncStartOneFrame JpgEncLoadHuffTab err.\n");
                return JPG_RET_INVALID_PARAM;
            }
        }
    }

    bTableInfoUpdate = FALSE;
    if (pEncInfo->encSlicePosY == 0) {
        bTableInfoUpdate = TRUE;
    }

    if (bTableInfoUpdate == TRUE) {
        // Load QMATTab
        if (!JpgEncLoadQMatTab(pJpgInst, instRegIndex)) {
            JpgLeaveLock();
            JLOG(ERR,"JPU_EncStartOneFrame JpgEncLoadQMatTab err.\n");
            return JPG_RET_INVALID_PARAM;
        }
    }

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_SIZE_REG, pEncInfo->alignedWidth<<16 | pEncInfo->alignedHeight);

    if (pEncInfo->rotationIndex || pEncInfo->mirrorIndex) {
        rotMirEnable = 0x10;
        rotMirMode   = (pEncInfo->mirrorIndex << 2) | pEncInfo->rotationIndex;
    }
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_ROT_INFO_REG, rotMirEnable | rotMirMode);

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_MCU_INFO_REG, (pEncInfo->mcuBlockNum&0x0f) << 17 | (pEncInfo->compNum&0x07) << 14    |
                                                      (pEncInfo->compInfo[0]&0x3f) << 8  | (pEncInfo->compInfo[1]&0x0f) << 4 |
                                                      (pEncInfo->compInfo[2]&0x0f));

    //JpgEncGbuResetReg
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_GBU_CTRL_REG, pEncInfo->stuffByteEnable<<3);     // stuffing "FF" data where frame end

    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_BASE00_REG,  pBasFrame->bufY);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_BASE01_REG,  pBasFrame->bufCb);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_BASE02_REG,  pBasFrame->bufCr);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_YSTRIDE_REG, pBasFrame->stride);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_DPB_CSTRIDE_REG, pBasFrame->strideC);


    if (pJpgInst->loggingEnable)
        jdi_log(pJpgInst->coreIndex, JDI_LOG_CMD_PICRUN, 1, instRegIndex);

    jpu_enable_irq(pJpgInst->coreIndex);
    JpuWriteInstReg(pJpgInst->coreIndex, instRegIndex, MJPEG_PIC_START_REG, (1<<JPG_ENABLE_START_PIC));

    pEncInfo->encIdx++;


    SetJpgPendingInstEx(pJpgInst, pJpgInst->instIndex);
    if (pJpgInst->sliceInstMode == TRUE) {
        if (pEncInfo->encDoneIdx == 0) {
            // don't unlock
        }
        else {
            JpgLeaveLock();
        }
    }
    return JPG_RET_SUCCESS;
}


JpgRet JPU_EncGetOutputInfo(
    JpgEncHandle handle,
    JpgEncOutputInfo * info
    )
{
    JpgInst * pJpgInst;
    JpgEncInfo * pEncInfo;
    Uint32 val;
    Uint32 intReason;
    JpgRet ret;
    Int32 instRegIndex;
    Int32 coreIndex;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS) {
        return ret;
    }

    if (info == 0) {
        return JPG_RET_INVALID_PARAM;
    }

    pJpgInst = handle;
    pEncInfo = &pJpgInst->JpgInfo->encInfo;
    coreIndex = pJpgInst->coreIndex;


    if (pJpgInst->sliceInstMode == TRUE) {
        if (pEncInfo->encDoneIdx == 0) {
            // already locked.
        }
        else {
            JpgEnterLock();
        }
    }


    if (pJpgInst != GetJpgPendingInstEx(pJpgInst)) {
        JpgLeaveLock();
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }

    if (pJpgInst->sliceInstMode == TRUE) {
        instRegIndex = pJpgInst->instIndex;
    }
    else {
        instRegIndex = 0;
    }

    info->frameCycle = JpuReadInstRegExt(coreIndex, instRegIndex, MJPEG_CYCLE_INFO_REG);
    intReason = JpuReadInstRegExt(coreIndex, instRegIndex, MJPEG_PIC_STATUS_REG);

    if ((intReason & 0x4) >> 2) {
        JpgLeaveLock();
        return JPG_RET_WRONG_CALL_SEQUENCE;
    }

    info->encodedSliceYPos = JpuReadInstRegExt(coreIndex, instRegIndex, MJPEG_SLICE_POS_REG);
    pEncInfo->encSlicePosY = info->encodedSliceYPos;
    if (intReason & (1<<INT_JPU_DONE))
        pEncInfo->encSlicePosY = 0;

    pEncInfo->streamWrPtr = JpuReadInstRegExt(coreIndex, instRegIndex, MJPEG_BBC_WR_PTR_REG);
    pEncInfo->streamRdPtr = JpuReadInstRegExt(coreIndex, instRegIndex, MJPEG_BBC_RD_PTR_REG);
    info->bitstreamBuffer = pEncInfo->streamRdPtr;
    info->bitstreamSize = pEncInfo->streamWrPtr - pEncInfo->streamRdPtr;
    info->streamWrPtr = pEncInfo->streamWrPtr;
    info->streamRdPtr = pEncInfo->streamRdPtr;

    if (intReason != 0) {
        JpuWriteInstRegExt(coreIndex, instRegIndex, MJPEG_PIC_STATUS_REG, intReason);
        if (intReason & (1<<INT_JPU_SLICE_DONE))
            info->encodeState = ENCODE_STATE_SLICE_DONE;
        if (intReason & (1<<INT_JPU_DONE))
            info->encodeState = ENCODE_STATE_FRAME_DONE;
    }

    if (info->encodeState == ENCODE_STATE_FRAME_DONE) {
        pEncInfo->encDoneIdx ++;
    }
    if (pJpgInst->loggingEnable)
        jdi_log(pJpgInst->coreIndex, JDI_LOG_CMD_PICRUN, 0, instRegIndex);

    JLOG(INFO, "encode jpeg hw Cycles:%d cost_time_us:%d\n", info->frameCycle, info->frameCycle/650);
    JpuWriteInstRegExt(coreIndex, instRegIndex, MJPEG_PIC_START_REG, 0);

    val = JpuReadRegExt(coreIndex, MJPEG_INST_CTRL_STATUS_REG);
    val &= ~(1UL<<instRegIndex);
    JpuWriteRegExt(coreIndex, MJPEG_INST_CTRL_STATUS_REG, val);

    // jpu_enable_irq(coreIndex);
    SetJpgPendingInstEx(0, pJpgInst->instIndex);

    JpgLeaveLock();
    return JPG_RET_SUCCESS;
}


JpgRet JPU_EncGiveCommand(
    JpgEncHandle handle,
    JpgCommand cmd,
    void * param)
{
    JpgInst * pJpgInst;
    JpgEncInfo * pEncInfo;
    JpgRet ret;

    ret = CheckJpgInstValidity(handle);
    if (ret != JPG_RET_SUCCESS)
        return ret;


    pJpgInst = handle;
    pEncInfo = &pJpgInst->JpgInfo->encInfo;
    switch (cmd)
    {
    case ENC_JPG_GET_HEADER:
        {
            if (param == 0) {
                return JPG_RET_INVALID_PARAM;
            }

            if (!JpgEncEncodeHeader(handle, param)) {
                return JPG_RET_INVALID_PARAM;
            }
            break;
        }
    case SET_JPG_USE_STUFFING_BYTE_FF:
        {
            int enable;
            enable = *(int *)param;
            pEncInfo->stuffByteEnable = enable;
            break;
        }
    case SET_JPG_QUALITY_FACTOR:
        {
            Uint32 encQualityPercentage;
            encQualityPercentage = *(Uint32*)param;
            JpgEncSetQualityFactor(handle, encQualityPercentage, TRUE);
            break;
        }
    case GET_JPG_QUALITY_TABLE:
        {
            JpgEncGetQulityTable(handle, (JpgQualityTable *)param);
            break;
        }
    case SET_JPG_QUALITY_TABLE:
        {
            JpgEncSetQulityTable(handle, (JpgQualityTable *)param);
            break;
        }
    case ENABLE_LOGGING:
        {
            pJpgInst->loggingEnable = 1;
        }
        break;
    case DISABLE_LOGGING:
        {
            pJpgInst->loggingEnable = 0;
        }
        break;

    default:
        return JPG_RET_INVALID_COMMAND;
    }
    return JPG_RET_SUCCESS;
}

int JPU_RequestCore(int timeout)
{
    return jdi_request_core(timeout);
}

int JPU_ReleaseCore(int core_idx)
{
    jdi_release_core(core_idx);
    return 0;
}

JpgRet JPU_DecSetParam(JpgDecHandle pHandle, JpgDecOpenParam * pop)
{
    JpgInst*    pJpgInst;
    JpgDecInfo* pDecInfo;
    JpgRet      ret;

    if (!pHandle)
        return JPG_RET_FAILURE;

    ret = CheckJpgDecOpenParam(pop);
    if (ret != JPG_RET_SUCCESS) {
        return ret;
    }

    JpgEnterLock();
    pJpgInst = pHandle;
    pDecInfo = &pJpgInst->JpgInfo->decInfo;

    pDecInfo->streamWrPtr = pop->bitstreamBuffer;
    pDecInfo->streamRdPtr = pop->bitstreamBuffer;

    pDecInfo->streamBufStartAddr = pop->bitstreamBuffer;
    pDecInfo->streamBufSize     = pop->bitstreamBufferSize;
    pDecInfo->streamBufEndAddr  = pop->bitstreamBuffer + pop->bitstreamBufferSize;
    pDecInfo->pBitStream        = pop->pBitStream;
    pDecInfo->streamEndian      = pop->streamEndian;
    pDecInfo->frameEndian       = pop->frameEndian;
    pDecInfo->chromaInterleave  = pop->chromaInterleave;
    pDecInfo->packedFormat      = pop->packedFormat;
    pDecInfo->roiEnable         = pop->roiEnable;
    pDecInfo->roiWidth          = pop->roiWidth;
    pDecInfo->roiHeight         = pop->roiHeight;
    pDecInfo->roiOffsetX        = pop->roiOffsetX;
    pDecInfo->roiOffsetY        = pop->roiOffsetY;
    pDecInfo->pixelJustification = pop->pixelJustification;
    pDecInfo->intrEnableBit     = pop->intrEnableBit;
    pJpgInst->sliceInstMode     = 0;    /* The decoder does not support the slice-decoding */
    pDecInfo->rotationIndex     = pop->rotation / 90;
    pDecInfo->mirrorIndex       = pop->mirror;

    /* convert output format */
    switch (pop->outputFormat) {
    case FORMAT_400: ret = JPG_RET_INVALID_PARAM; break;
    case FORMAT_420: pDecInfo->ofmt = O_FMT_420;  break;
    case FORMAT_422: pDecInfo->ofmt = O_FMT_422;  break;
    case FORMAT_440: ret = JPG_RET_INVALID_PARAM; break;
    case FORMAT_444: pDecInfo->ofmt = O_FMT_444;  break;
    case FORMAT_MAX: pDecInfo->ofmt = O_FMT_NONE; break;
    default:         ret = JPG_RET_INVALID_PARAM; break;
    }

    pDecInfo->userqMatTab = 0;
    pDecInfo->decIdx = 0;

    JpgLeaveLock();

    return ret;
}


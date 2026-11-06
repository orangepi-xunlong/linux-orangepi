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
#include <linux/vmalloc.h>
#include "wave/wave6.h"
#include "wave/wave6_regdefine.h"
#include "wave/wave6_av1_cdf_table.h"

#define BSOPTION_ENABLE_EXPLICIT_END (1<<0)

#define WTL_RIGHT_JUSTIFIED          0
#define WTL_LEFT_JUSTIFIED           1
#define WTL_PIXEL_8BIT               0
#define WTL_PIXEL_16BIT              1
#define WTL_PIXEL_32BIT              3

#define WAVE6_DEC_HEVC_MVCOL_BUF_SIZE(_w, _h)    ((VPU_ALIGN256(_w)/16)*(VPU_ALIGN64(_h)/16)*1*16)
#define WAVE6_DEC_AVC_MVCOL_BUF_SIZE(_w, _h)     ((VPU_ALIGN64(_w)/16)*(VPU_ALIGN16(_h)/16)*5*16)
#define WAVE6_DEC_VP9_MVCOL_BUF_SIZE(_w, _h)     ((VPU_ALIGN64(_w)/64)*(VPU_ALIGN64(_h)/64)*32*16 + (VPU_ALIGN256(_w)/256)*(VPU_ALIGN64(_h)/64)*16*16)
#define WAVE6_DEC_AV1_MVCOL_BUF_SIZE(_w, _h)     (((VPU_ALIGN64(_w)/64*16*16) + (VPU_ALIGN256(_w)/64*8*16))*VPU_ALIGN64(_h)/64 + (VPU_ALIGN128(_w)/64)*(VPU_ALIGN128(_h)/64)*4*128 + 656*16)
#define WAVE6_AV1_DEFAULT_CDF_BUF_SIZE           (48*1024)
#define WAVE6_VP9_SEGMAP_BUF_SIZE(_w, _h)        ((VPU_ALIGN64(_w)/64)*(VPU_ALIGN64(_h)/64)*64)
#define WAVE6_FBC_LUMA_TABLE_SIZE(_w, _h)        (VPU_ALIGN256(_w)*VPU_ALIGN64(_h)/32)
#define WAVE6_FBC_CHROMA_TABLE_SIZE(_w, _h)      (VPU_ALIGN256(_w/2)*VPU_ALIGN64(_h)/32)
#define WAVE6_ENC_AV1_MVCOL_BUF_SIZE             (12*1024)
#define WAVE6_ENC_AVC_MVCOL_BUF_SIZE(_w, _h)     ((VPU_ALIGN512(_w)/512)*(VPU_ALIGN16(_h)/16)*16)
#define WAVE6_ENC_HEVC_MVCOL_BUF_SIZE(_w, _h)    ((VPU_ALIGN64(_w)/64)*(VPU_ALIGN64(_h)/64)*128)
#define WAVE6_ENC_SUBSAMPLED_SIZE(_w, _h)        (VPU_ALIGN16(_w/4)*VPU_ALIGN32(_h/4))

static Uint32 to_hw_endian(Uint32 core_idx, Uint32 endian)
{
    Uint32 temp;

    temp = vdi_convert_endian(core_idx, endian);
    temp = (~temp&VDI_128BIT_ENDIAN_MASK);
    return temp;
}

static void load_av1_cdf_table(Uint32 core_idx, PhysicalAddress buf_addr)
{
    const Uint16* tbl_data = NULL;
    Uint32 tbl_size = AV1_MAX_CDF_WORDS * AV1_CDF_WORDS_SIZE * 2;

    VLOG(INFO, "Start to load av1 default cdf table\n");

    tbl_data = defCdfTbl;
    VpuWriteMem(core_idx, buf_addr, (unsigned char*)tbl_data, tbl_size, VDI_128BIT_LITTLE_ENDIAN);

    VLOG(INFO, "Success to load av1 default cdf table\n");
}

Uint32 Wave6VpuIsInit(Uint32 core_idx)
{
    Uint32 pc;

    pc = (Uint32)VpuReadReg(core_idx, W6_VCPU_CUR_PC);

    return pc;
}

Int32 Wave6VpuIsBusy(Uint32 core_idx)
{
    return VpuReadReg(core_idx, W6_VPU_CMD_BUSY_STATUS);
}

static void W6EnableInterrupt(Uint32 core_idx)
{
    Uint32 regVal = 0;
    // encoder
    regVal  = (1<<INT_WAVE6_ENC_SET_PARAM);
    regVal |= (1<<INT_WAVE6_ENC_PIC);
    regVal |= (1<<INT_WAVE6_BSBUF_FULL);
    // decoder
    regVal |= (1<<INT_WAVE6_INIT_SEQ);
    regVal |= (1<<INT_WAVE6_DEC_PIC);
    regVal |= (1<<INT_WAVE6_BSBUF_EMPTY);
    VpuWriteReg(core_idx, W6_VPU_VINT_ENABLE, regVal);
}

static void W6SendCommand(Uint32 core_idx, CodecInst* inst, Uint32 cmd)
{
    Uint32 index = 0;
    Uint32 codec = 0;

    if (inst) {
        index = inst->instIndex;
        codec = inst->codecMode;
    }

    if (cmd == W6_INIT_VPU || cmd == W6_WAKEUP_VPU) {
        VpuWriteReg(core_idx, W6_VPU_CMD_BUSY_STATUS, 1);
        VpuWriteReg(core_idx, W6_COMMAND, cmd);

        if (inst && inst->loggingEnable)
            vdi_log(core_idx, index, cmd, 1);

        VpuWriteReg(core_idx, W6_VPU_REMAP_CORE_START, 1);
    } else {
        if (cmd == W6_CREATE_INSTANCE)
            VpuWriteReg(core_idx, W6_CMD_INSTANCE_INFO, (codec<<16));
        else
            VpuWriteReg(core_idx, W6_CMD_INSTANCE_INFO, (codec<<16)|(index&0xffff));

        VpuWriteReg(core_idx, W6_VPU_CMD_BUSY_STATUS, 1);
        VpuWriteReg(core_idx, W6_COMMAND, cmd);

        if (inst && inst->loggingEnable)
            vdi_log(core_idx, index, cmd, 1);

        VpuWriteReg(core_idx, W6_VPU_HOST_INT_REQ, 1);
    }
}
static void W6RemapCodeBuffer(Uint32 core_idx)
{
    vpu_buffer_t    vb;
    PhysicalAddress codeBase;
    Uint32          i;
    Uint32          regVal;

    vdi_get_common_memory(core_idx, &vb);

    codeBase = vb.phys_addr;

    for (i = 0; i < WAVE6_MAX_CODE_BUF_SIZE/W_REMAP_MAX_SIZE; i++) {
        regVal = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (0<<16) | (i<<12) | (1<<11) | ((W_REMAP_MAX_SIZE>>12)&0x1ff);
        VpuWriteReg(core_idx, W6_VPU_REMAP_CTRL,  regVal);
        VpuWriteReg(core_idx, W6_VPU_REMAP_VADDR, i*W_REMAP_MAX_SIZE);
        VpuWriteReg(core_idx, W6_VPU_REMAP_PADDR, codeBase + i*W_REMAP_MAX_SIZE);
    }
}
static RetCode SendQuery(Uint32 coreIdx, CodecInst* inst, W6_QUERY_OPT queryOpt)
{
    VpuWriteReg(coreIdx, W6_QUERY_OPTION, queryOpt);
    W6SendCommand(coreIdx, inst, W6_QUERY);

    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1) {
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(coreIdx, W6_RET_SUCCESS) == FALSE) {
        Uint32 regVal = VpuReadReg(coreIdx, W6_RET_FAIL_REASON);
        VLOG(ERR, "W6_RET_FAIL_REASON\n");
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_RESULT_NOT_READY)
            return RETCODE_REPORT_NOT_READY;
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_VLC_BUF_FULL)
            return RETCODE_VLC_BUF_FULL;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            return RETCODE_VPU_BUS_ERROR;
        else
            return RETCODE_QUERY_FAILURE;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuGetDebugInfo(CodecInst* inst, VpuDebugInfo* info)
{
    RetCode ret = RETCODE_SUCCESS;
    VpuAttr* attr = &g_VpuCoreAttributes[inst->coreIdx];
    Int32 coreIdx = inst->coreIdx;
    int i;
    int idx;

    if (!attr->supportCommandQueue)
        return RETCODE_NOT_SUPPORTED_FEATURE;

    ret = SendQuery(coreIdx, inst, W6_QUERY_GET_FW_STATUS);
    if (ret != RETCODE_SUCCESS)
        return RETCODE_QUERY_FAILURE;

    idx = 0;
    for (i = 0; i <= 0x228; i = i + 4) {
        info->regs[idx] = VpuReadReg(coreIdx, 0x300 + i);
        idx++;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuGetVersion(Uint32 core_idx, Uint32* version, Uint32* revision)
{
    VpuAttr* attr = &g_VpuCoreAttributes[core_idx];
    Uint32 val;
    Uint8* str;
    RetCode ret = RETCODE_SUCCESS;

    // If INIT_VPU command finished, W6_RET_CQ_FLAG register is updated by FW.
    attr->supportCommandQueue = VpuReadReg(core_idx, W6_RET_CQ_FLAG);

    if (attr->supportCommandQueue) {
        ret = SendQuery(core_idx, NULL, W6_QUERY_GET_VPU_INFO);
        if (ret != RETCODE_SUCCESS)
            return ret;
    }
    else {
        W6SendCommand(core_idx, NULL, W6_GET_VPU_INFO);
        if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1) {
            ret = RETCODE_VPU_RESPONSE_TIMEOUT;
            return ret;
        }

        if (VpuReadReg(core_idx, W6_RET_SUCCESS) == FALSE) {
            ret = RETCODE_FAILURE;
            return ret;
        }
    }

    val = VpuReadReg(core_idx, W6_RET_PRODUCT_NAME);
    str = (Uint8*)&val;
    attr->productName[0] = str[3];
    attr->productName[1] = str[2];
    attr->productName[2] = str[1];
    attr->productName[3] = str[0];
    attr->productName[4] = 0;

    attr->productVersion  = VpuReadReg(core_idx, W6_RET_PRODUCT_VERSION);
    attr->fwVersion       = VpuReadReg(core_idx, W6_RET_FW_VERSION);
    attr->hwConfigDef0    = VpuReadReg(core_idx, W6_RET_STD_DEF0);
    attr->hwConfigDef1    = VpuReadReg(core_idx, W6_RET_STD_DEF1);
    attr->hwConfigFeature = VpuReadReg(core_idx, W6_RET_CONF_FEATURE);
    attr->hwConfigDate    = VpuReadReg(core_idx, W6_RET_CONF_DATE);
    attr->hwConfigRev     = VpuReadReg(core_idx, W6_RET_CONF_REVISION);
    attr->hwConfigType    = VpuReadReg(core_idx, W6_RET_CONF_TYPE);

    attr->supportDecoders = 0;
    attr->supportEncoders = 0;
    if (attr->productId == PRODUCT_ID_617) {
        attr->supportDecoders  = (((attr->hwConfigDef1>>2)&0x01) << STD_HEVC);
        attr->supportDecoders |= (((attr->hwConfigDef1>>3)&0x01) << STD_AVC);
        attr->supportDecoders |= (((attr->hwConfigDef1>>5)&0x01) << STD_AV1);
        attr->supportDecoders |= (((attr->hwConfigDef1>>6)&0x01) << STD_VP9);
    } else if (attr->productId == PRODUCT_ID_627) {
        attr->supportEncoders  = (((attr->hwConfigDef1>>0)&0x01) << STD_HEVC);
        attr->supportEncoders |= (((attr->hwConfigDef1>>1)&0x01) << STD_AVC);
        attr->supportEncoders |= (((attr->hwConfigDef1>>4)&0x01) << STD_AV1);
    } else if (attr->productId == PRODUCT_ID_637) {
        attr->supportDecoders  = (((attr->hwConfigDef1>>2)&0x01) << STD_HEVC);
        attr->supportDecoders |= (((attr->hwConfigDef1>>3)&0x01) << STD_AVC);
        attr->supportDecoders |= (((attr->hwConfigDef1>>5)&0x01) << STD_AV1);
        attr->supportDecoders |= (((attr->hwConfigDef1>>6)&0x01) << STD_VP9);
        attr->supportEncoders  = (((attr->hwConfigDef1>>0)&0x01) << STD_HEVC);
        attr->supportEncoders |= (((attr->hwConfigDef1>>1)&0x01) << STD_AVC);
        attr->supportEncoders |= (((attr->hwConfigDef1>>4)&0x01) << STD_AV1);
    }

    attr->supportHEVC10bitEnc       = (BOOL)((attr->hwConfigFeature>>3)&0x01);
    attr->supportAVC10bitEnc        = (BOOL)((attr->hwConfigFeature>>11)&0x01);
    attr->supportWTL                = TRUE;
    attr->supportEndianMask         = (Uint32)((1<<VDI_LITTLE_ENDIAN) | (1<<VDI_BIG_ENDIAN) | (1<<VDI_32BIT_LITTLE_ENDIAN) | (1<<VDI_32BIT_BIG_ENDIAN) | (0xffffUL<<16));
    attr->supportBitstreamMode      = (1<<BS_MODE_INTERRUPT) | (1<<BS_MODE_PIC_END);
    attr->maxNumVcores              = MAX_NUM_VCORE;
    attr->supportDualCore           = (BOOL)((attr->hwConfigDef1>>26)&0x01);
    attr->supportNewTimer           = TRUE;
    attr->support2AlignScaler       = TRUE;
    attr->supportTiled2Linear       = FALSE;
    attr->supportMapTypes           = FALSE;
    attr->supportFBCBWOptimization  = FALSE;
    attr->supportBackbone           = FALSE;
    attr->supportVcpuBackbone       = FALSE;
    attr->supportVcoreBackbone      = FALSE;
    attr->supportUserQMatrix        = FALSE;
    attr->supportWeightedPrediction = FALSE;
    attr->supportRDOQ               = FALSE;
    attr->supportAdaptiveRounding   = FALSE;
    attr->supportOCSRAM             = (BOOL)((attr->hwConfigDef1>>31)&0x01);
    attr->framebufferCacheType      = FramebufCacheNone;
    attr->numberOfMemProtectRgns    = 0;
    attr->bitstreamBufferMargin     = 0;

    if (version != NULL)
        *version = 0;
    if (revision != NULL)
        *revision = attr->fwVersion;

    return ret;
}

RetCode Wave6VpuInit(Uint32 core_idx, void* fw, Uint32 fw_size)
{
    vpu_buffer_t vb;
    Uint32       regVal = 0;
    RetCode      ret = RETCODE_SUCCESS;

    vdi_get_common_memory(core_idx, &vb);

    if (WAVE6_MAX_CODE_BUF_SIZE < fw_size*2)
        return RETCODE_INSUFFICIENT_RESOURCE;

    VLOG(INFO, "\nVPU INIT Start!!! codeBase = 0x%x\n", vb.phys_addr);
    VpuWriteMem(core_idx, vb.phys_addr, (unsigned char*)fw, fw_size*2, VDI_128BIT_LITTLE_ENDIAN);
    vdi_set_bit_firmware_to_pm(core_idx, (Uint16*)fw);
    W6RemapCodeBuffer(core_idx);

    /* Interrupt */
    W6EnableInterrupt(core_idx);

    W6SendCommand(core_idx, NULL, W6_INIT_VPU);
    if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1) {
        VLOG(ERR, "VPU init(W6_VPU_REMAP_CORE_START) timeout\n");
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }
    regVal = VpuReadReg(core_idx, W6_RET_SUCCESS);
    if (regVal == 0) {
        Uint32 reasonCode = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
        VLOG(ERR, "VPU init(W6_RET_SUCCESS) failed(%d) REASON CODE(%08x)\n", regVal, reasonCode);
        return RETCODE_FAILURE;
    }

    ret = Wave6VpuGetVersion(core_idx, NULL, NULL);

#ifdef SUPPORT_GDB_IN_API
    ExecuteDebugger();
#endif

    return ret;
}

RetCode Wave6VpuBuildUpDecParam(CodecInst* instance, DecOpenParam* param)
{
    RetCode  ret      = RETCODE_SUCCESS;
    DecInfo* pDecInfo = VPU_HANDLE_TO_DECINFO(instance);
    Uint32   bsEndian = 0, temp = 0;

    pDecInfo->streamRdPtrRegAddr      = W6_RET_DEC_BS_RD_PTR;
    pDecInfo->streamWrPtrRegAddr      = W6_CMD_DEC_INIT_SEQ_BS_WR_PTR;
    pDecInfo->frameDisplayFlagRegAddr = W6_RET_DEC_DISP_IDC;
    pDecInfo->currentPC               = W6_VCPU_CUR_PC;
    pDecInfo->busyFlagAddr            = W6_VPU_CMD_BUSY_STATUS;

    switch (param->bitstreamFormat) {
    case STD_HEVC:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_HEVC;
        break;
    case STD_VP9:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_VP9;
        break;
    case STD_AVC:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_AVC;
        break;
    case STD_AV1:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_AV1;
        break;
    default:
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }

    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_TEMP_BASE, param->instBuffer.tempBufBase);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_TEMP_SIZE, param->instBuffer.tempBufSize);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_SEC_AXI_BASE, param->instBuffer.secAxiBufBase);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_SEC_AXI_SIZE, param->instBuffer.secAxiBufSize);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_WORK_BASE, param->instBuffer.workBufBase);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_WORK_SIZE, param->instBuffer.workBufSize);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_BS_ADDR, pDecInfo->streamBufStartAddr);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_BS_SIZE, pDecInfo->streamBufSize);

    /* NOTE: When endian mode is 0, SDMA reads MSB first */
    bsEndian = to_hw_endian(instance->coreIdx, param->streamEndian);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_BS_PARAM, bsEndian);

    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_ADDR_EXT, param->extAddrVcpu);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_CORE_INFO, (1<<4) | (1<<0));
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_CREATE_INST_PRIORITY, (param->isSecureInst        <<8) |
                                                                    ((param->instPriority&0xFF) <<0));


    W6SendCommand(instance->coreIdx, instance, W6_CREATE_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(instance->coreIdx, W6_RET_SUCCESS) == FALSE) {
        temp = VpuReadReg(instance->coreIdx, W6_RET_FAIL_REASON);
        VLOG(ERR, "FAIL_REASON = 0x%x\n", temp);

        if (temp == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            ret = RETCODE_MEMORY_ACCESS_VIOLATION;
        if (temp == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            ret = RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (temp == WAVE5_SYSERR_BUS_ERROR || temp == WAVE5_SYSERR_DOUBLE_FAULT)
            ret = RETCODE_VPU_BUS_ERROR;
        else
            ret = RETCODE_FAILURE;
    }

    instance->instIndex = VpuReadReg(instance->coreIdx, W6_RET_INSTANCE_ID);

    return ret;
}

RetCode Wave6VpuDecInitSeq(CodecInst* instance)
{
    RetCode  ret = RETCODE_SUCCESS;
    VpuAttr* attr = &g_VpuCoreAttributes[instance->coreIdx];
    DecInfo* info;
    Uint32   cmdOption = INIT_SEQ_NORMAL, bsOption, regVal;

    if (instance == NULL)
        return RETCODE_INVALID_PARAM;

    info = VPU_HANDLE_TO_DECINFO(instance);
    if (info->thumbnailMode)
        cmdOption = INIT_SEQ_W_THUMBNAIL;

    /* Set attributes of bitstream buffer controller */
    bsOption = 0;
    switch (info->openParam.bitstreamMode) {
    case BS_MODE_INTERRUPT:
        if (info->seqInitEscape == TRUE)
            bsOption = BSOPTION_ENABLE_EXPLICIT_END;
        break;
    case BS_MODE_PIC_END:
        bsOption = BSOPTION_ENABLE_EXPLICIT_END;
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_INIT_SEQ_BS_RD_PTR, info->streamRdPtr);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_INIT_SEQ_BS_WR_PTR, info->streamWrPtr);

    if (info->streamEndflag == 1) {
        bsOption = 3;
    }
    if (instance->codecMode == W_AV1_DEC) {
        bsOption |= ((info->openParam.av1Format) << 2);
    }
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_INIT_SEQ_BS_OPTION, (1UL<<31) | bsOption);

    VpuWriteReg(instance->coreIdx, W6_COMMAND_OPTION, cmdOption);

    W6SendCommand(instance->coreIdx, instance, W6_INIT_SEQ);

    if (attr->supportCommandQueue) {
        if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        regVal = VpuReadReg(instance->coreIdx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((regVal >> instance->instIndex) & 0x1);
        regVal = VpuReadReg(instance->coreIdx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty = ((regVal >> instance->instIndex) & 0x1);

        if (VpuReadReg(instance->coreIdx, W6_RET_SUCCESS) == FALSE) {
            regVal = VpuReadReg(instance->coreIdx, W6_RET_FAIL_REASON);
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

            if (regVal == WAVE5_SYSERR_QUEUEING_FAIL)
                ret = RETCODE_QUEUEING_FAILURE;
            else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
                ret = RETCODE_MEMORY_ACCESS_VIOLATION;
            else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
                ret = RETCODE_VPU_RESPONSE_TIMEOUT;
            else if (regVal == WAVE5_SYSERR_VLC_BUF_FULL)
                ret = RETCODE_VLC_BUF_FULL;
            else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
                ret = RETCODE_VPU_BUS_ERROR;
            else
                ret = RETCODE_FAILURE;
        }
    }

    return ret;
}

static void GetDecSequenceResult(CodecInst* instance, DecInitialInfo* info)
{
    DecInfo* pDecInfo = &instance->CodecInfo->decInfo;
    Uint32   regVal, subLayerInfo;
    Uint32   profileCompatibilityFlag;
    Uint32   left, right, top, bottom;
    Uint32   progressiveFlag, interlacedFlag;

    info->rdPtr = VpuReadReg(instance->coreIdx, W6_RET_DEC_BS_RD_PTR);
    pDecInfo->frameDisplayFlag = VpuReadReg(instance->coreIdx, W6_RET_DEC_DISP_IDC);

    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_PIC_SIZE);
    info->picWidth            = ( (regVal >> 16) & 0xffff );
    info->picHeight           = ( regVal & 0xffff );
    info->minFrameBufferCount = VpuReadReg(instance->coreIdx, W6_RET_DEC_NUM_REQUIRED_FB);
    info->frameBufDelay       = VpuReadReg(instance->coreIdx, W6_RET_DEC_NUM_REORDER_DELAY);

    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_CROP_LEFT_RIGHT);
    left   = (regVal >> 16) & 0xffff;
    right  = regVal & 0xffff;
    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_CROP_TOP_BOTTOM);
    top    = (regVal >> 16) & 0xffff;
    bottom = regVal & 0xffff;

    info->picCropRect.left   = left;
    info->picCropRect.right  = info->picWidth - right;
    info->picCropRect.top    = top;
    info->picCropRect.bottom = info->picHeight - bottom;

    info->fRateNumerator   = VpuReadReg(instance->coreIdx, W6_RET_DEC_FRAME_RATE_NR);
    info->fRateDenominator = VpuReadReg(instance->coreIdx, W6_RET_DEC_FRAME_RATE_DR);

    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_COLOR_SAMPLE_INFO);
    info->lumaBitdepth           = (regVal>>0)&0x0f;
    info->chromaBitdepth         = (regVal>>4)&0x0f;
    info->chromaFormatIDC        = (regVal>>8)&0x0f;
    info->aspectRateInfo         = (regVal>>16)&0xff;
    info->isExtSAR               = (info->aspectRateInfo == 255 ? TRUE : FALSE);
    if (info->isExtSAR == TRUE) {
        info->aspectRateInfo     = VpuReadReg(instance->coreIdx, W6_RET_DEC_ASPECT_RATIO);  /* [0:15] - vertical size, [16:31] - horizontal size */
    }
    info->bitRate                = VpuReadReg(instance->coreIdx, W6_RET_DEC_BIT_RATE);

    subLayerInfo = VpuReadReg(instance->coreIdx, W6_RET_DEC_SUB_LAYER_INFO);
    info->maxTemporalLayers = (subLayerInfo>>8)&0x7;

    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_SEQ_PARAM);
    info->level                  = regVal&0xff;
    interlacedFlag               = (regVal>>10)&0x1;
    progressiveFlag              = (regVal>>11)&0x1;
    profileCompatibilityFlag     = (regVal>>12)&0xff;
    info->profile                = (regVal>>24)&0x1f;
    info->tier                   = (regVal>>29)&0x01;

    if (instance->codecMode == W_HEVC_DEC) {
        /* Guessing Profile */
        if (info->profile == 0) {
            if      ((profileCompatibilityFlag&0x06) == 0x06) info->profile = HEVC_PROFILE_MAIN;         /* Main profile */
            else if ((profileCompatibilityFlag&0x04) == 0x04) info->profile = HEVC_PROFILE_MAIN10;       /* Main10 profile */
            else if ((profileCompatibilityFlag&0x08) == 0x08) info->profile = HEVC_PROFILE_STILLPICTURE; /* Main Still Picture profile */
            else                                              info->profile = HEVC_PROFILE_MAIN;         /* For old version HM */
        }

        if (progressiveFlag == 1 && interlacedFlag == 0)
            info->interlace = 0;
        else
            info->interlace = 1;
    }
    else if (instance->codecMode == W_AVC_DEC) {
        info->profile   = (regVal>>24)&0x7f;
        info->interlace = 0;
    }
    else if (instance->codecMode == W_AV1_DEC) {
        info->maxSpatialLayers = (subLayerInfo>>24)&0x3;
    }

    return;
}

RetCode Wave6VpuDecGetSeqInfo(CodecInst* instance, DecInitialInfo* info)
{
    RetCode  ret = RETCODE_SUCCESS;
    VpuAttr* attr = &g_VpuCoreAttributes[instance->coreIdx];
    Uint32   regVal;
    DecInfo* pDecInfo = VPU_HANDLE_TO_DECINFO(instance);

    if (attr->supportCommandQueue) {
        ret = SendQuery(instance->coreIdx, instance, W6_QUERY_GET_RESULT);
        if (ret != RETCODE_SUCCESS)
            return ret;

        regVal = VpuReadReg(instance->coreIdx, W6_CMD_QUEUE_FULL_IDC);
        pDecInfo->instanceQueueFull = ((regVal >> instance->instIndex) & 0x1);
        regVal = VpuReadReg(instance->coreIdx, W6_CMD_QUEUE_EMPTY_IDC);
        pDecInfo->reportQueueEmpty = ((regVal >> instance->instIndex) & 0x1);

        if (VpuReadReg(instance->coreIdx, W6_RET_DEC_DECODING_SUCCESS) == 0) {
            info->seqInitErrReason = VpuReadReg(instance->coreIdx, W6_RET_DEC_ERR_INFO);
            //retCode = RETCODE_FAILURE;
            VLOG(INFO, "W6_RET_DEC_DECODING_SUCCESS\n");
        }
        else {
            info->warnInfo = VpuReadReg(instance->coreIdx, W6_RET_DEC_WARN_INFO);
        }
    }
    else {
        if (VpuReadReg(instance->coreIdx, W6_RET_SUCCESS) != 1) {
            info->seqInitErrReason = VpuReadReg(instance->coreIdx, W6_RET_FAIL_REASON);
            ret = RETCODE_FAILURE;
        }
    }

    GetDecSequenceResult(instance, info);

    return ret;
}

RetCode Wave6VpuDecRegisterFramebuffer(CodecInst* inst, FrameBuffer* fbArr, TiledMapType mapType, Uint32 count)
{
    RetCode      ret = RETCODE_SUCCESS;
    DecInfo*     info = &inst->CodecInfo->decInfo;
    Int32        idx = 0;
    Uint32       i, k;
    Uint32       remain, groupNum;
    Int32        coreIdx;
    Uint32       regVal, cbcrInterleave, nv21, picSize;
    Uint32       endian, yuvFormat = 0;
    Uint32       addrY, addrCb, addrCr;
    Uint32       initPicWidth = 0, initPicHeight = 0;
    Uint32       pixelOrder=1;
    BOOL         bwbFlag = (mapType == LINEAR_FRAME_MAP) ? TRUE : FALSE;

    coreIdx        = inst->coreIdx;
    cbcrInterleave = info->openParam.cbcrInterleave;
    nv21           = info->openParam.nv21;
    initPicWidth   = info->initialInfo.picWidth;
    initPicHeight  = info->initialInfo.picHeight;

    if (mapType >= COMPRESSED_FRAME_MAP) {
        cbcrInterleave = 0;
        nv21           = 0;

        if (inst->codecMode == W_AV1_DEC) {
            if (!info->vbDefCdf.phys_addr)
                return RETCODE_INSUFFICIENT_RESOURCE;

            load_av1_cdf_table(inst->coreIdx, info->vbDefCdf.phys_addr);
        } else if (inst->codecMode == W_VP9_DEC) {
            if (!info->vbSegMap.phys_addr)
                return RETCODE_INSUFFICIENT_RESOURCE;
        }

        for (i = 0; i < count; i++) {
            if (!info->vbFbcYTbl[i].phys_addr)
                return RETCODE_INSUFFICIENT_RESOURCE;
            if (!info->vbFbcCTbl[i].phys_addr)
                return RETCODE_INSUFFICIENT_RESOURCE;
            if (!info->vbMV[i].phys_addr)
                return RETCODE_INSUFFICIENT_RESOURCE;
        }

        picSize = (initPicWidth<<16)|(initPicHeight);
    } else {
        picSize = (initPicWidth<<16)|(initPicHeight);
    }
    endian = vdi_convert_endian(coreIdx, fbArr[0].endian);
    VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_PIC_SIZE, picSize);


    yuvFormat = 0;
    if (mapType == LINEAR_FRAME_MAP) {
        BOOL   justified = WTL_RIGHT_JUSTIFIED;
        Uint32 formatNo  = WTL_PIXEL_8BIT;
        switch (info->wtlFormat) {
        case FORMAT_420_P10_16BIT_MSB:
            justified = WTL_RIGHT_JUSTIFIED;
            formatNo  = WTL_PIXEL_16BIT;
            break;
        case FORMAT_420_P10_16BIT_LSB:
            justified = WTL_LEFT_JUSTIFIED;
            formatNo  = WTL_PIXEL_16BIT;
            break;
        case FORMAT_420_P10_32BIT_MSB:
            justified = WTL_RIGHT_JUSTIFIED;
            formatNo  = WTL_PIXEL_32BIT;
            break;
        case FORMAT_420_P10_32BIT_LSB:
            justified = WTL_LEFT_JUSTIFIED;
            formatNo  = WTL_PIXEL_32BIT;
            break;
        default:
            break;
        }
        yuvFormat = justified<<2 | formatNo;
    }

    regVal =
             (bwbFlag        << 28) |
             (pixelOrder     << 23) | /* PIXEL ORDER in 128bit. first pixel in low address */
             (yuvFormat      << 20) |
             (nv21           << 17) |
             (cbcrInterleave << 16) |
             (fbArr[0].stride);
    VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_PIC_INFO, regVal);
    VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_DEFAULT_CDF, info->vbDefCdf.phys_addr);
    VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_SEGMAP, info->vbSegMap.phys_addr);

    remain   = count;
    groupNum = (VPU_ALIGN16(count)/16) - 1;
    for (i = 0; i <= groupNum; i++) {
        BOOL firstGroup = (i == 0) ? TRUE : FALSE;
        BOOL lastGroup  = (i == groupNum) ? TRUE : FALSE;
        Uint32 setFbNum = (remain >= 16) ? 16 : remain;
        Uint32 startNo  = i*16;
        Uint32 endNo    = startNo + setFbNum - 1;

        regVal = (info->openParam.enableNonRefFbcWrite << 26) |
                 (endian                               << 16) |
                 (lastGroup                            << 4)  |
                 (firstGroup                           << 3);
        VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_OPTION, regVal);

        regVal = (startNo << 24) |
                 (endNo   << 16) |
                 (startNo << 5)  |
                 (endNo   << 0);
        VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_NUM, regVal);

        for (k = 0; k < setFbNum; k++) {
            if (mapType == LINEAR_FRAME_MAP && info->openParam.cbcrOrder == CBCR_ORDER_REVERSED) {
                addrY  = fbArr[idx].bufY;
                addrCb = fbArr[idx].bufCr;
                addrCr = fbArr[idx].bufCb;
            } else {
                addrY  = fbArr[idx].bufY;
                addrCb = fbArr[idx].bufCb;
                addrCr = fbArr[idx].bufCr;
            }
            VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_FBC_Y0 + (k*24), addrY);
            VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_FBC_C0 + (k*24), addrCb);
            APIDPRINT("REGISTER FB[%02d] Y(0x%08x), Cb(0x%08x) ", k, addrY, addrCb);
            if (mapType >= COMPRESSED_FRAME_MAP) {
                VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_FBC_Y_OFFSET0 + (k*24), info->vbFbcYTbl[idx].phys_addr);
                VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_FBC_C_OFFSET0 + (k*24), info->vbFbcCTbl[idx].phys_addr);
                VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_MV_COL0       + (k*24), info->vbMV[idx].phys_addr);
                APIDPRINT("Yo(0x%08x) Co(0x%08x), Mv(0x%08x)\n", info->vbFbcYTbl[idx].phys_addr, info->vbFbcCTbl[idx].phys_addr, info->vbMV[idx].phys_addr);
            } else {
                VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_FBC_Y_OFFSET0 + (k*24), addrCr);
                VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_FBC_C_OFFSET0 + (k*24), 0);
                VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_MV_COL0       + (k*24), 0);
                APIDPRINT("Cr(0x%08x)\n", addrCr);
            }
            idx++;
        }
        remain -= k;

        W6SendCommand(coreIdx, inst, W6_SET_FB);
        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(coreIdx, W6_RET_SUCCESS);
    if (regVal == 0)
        return RETCODE_FAILURE;

    return ret;
}

RetCode Wave6VpuDecUpdateFramebuffer(CodecInst* inst, FrameBuffer* fbcFb, FrameBuffer* linearFb, Int32 mvIndex, Int32 width, Int32 height)
{
    DecInfo* info = &inst->CodecInfo->decInfo;
    Int32    fbcIndex = -1;
    Int32    linearIndex = -1;
    Int32    coreIdx = inst->coreIdx;
    Uint32   stride;
    PhysicalAddress fbcY, fbcC, fbcYTbl, fbcCTbl, mvCol;

    if (!fbcFb)
        return RETCODE_INVALID_PARAM;
    if (!linearFb)
        return RETCODE_INVALID_PARAM;

    if (fbcFb->myIndex >= 0) {
        info->frameBufPool[fbcFb->myIndex] = *fbcFb;

        fbcIndex = fbcFb->myIndex;
        stride  = fbcFb->stride;
        fbcY    = fbcFb->bufY;
        fbcC    = fbcFb->bufCb;
        fbcYTbl = info->vbFbcYTbl[fbcIndex].phys_addr;
        fbcCTbl = info->vbFbcCTbl[fbcIndex].phys_addr;
        mvCol   = 0;
    }
    else if (linearFb->myIndex >= 0) {
        info->frameBufPool[linearFb->myIndex] = *linearFb;

        linearIndex = linearFb->myIndex - info->numFbsForDecoding;
        stride  = linearFb->stride;
        fbcY    = linearFb->bufY;
        fbcC    = linearFb->bufCb;
        fbcYTbl = linearFb->bufCr;
        fbcCTbl = 0;
        mvCol   = 0;
    }
    else if (mvIndex >= 0) {
        stride  = 0;
        fbcY    = 0;
        fbcC    = 0;
        fbcYTbl = 0;
        fbcCTbl = 0;
        mvCol   = info->vbMV[mvIndex].phys_addr;
    }
    else {
        return RETCODE_INVALID_PARAM;
    }

    VpuWriteReg(coreIdx, W6_CMD_DEC_UPDATE_FB_PIC_SIZE, (width<<16) | (height<<0));
    VpuWriteReg(coreIdx, W6_CMD_DEC_UPDATE_FB_STRIDE, stride);
    VpuWriteReg(coreIdx, W6_CMD_DEC_UPDATE_FB_INDICE, ((mvIndex & 0xff)     <<16) |
                                                      ((linearIndex & 0xff) <<8)  |
                                                      ((fbcIndex & 0xff)    <<0));
    VpuWriteReg(coreIdx, W6_CMD_DEC_UPDATE_FB_FBC_Y, fbcY);
    VpuWriteReg(coreIdx, W6_CMD_DEC_UPDATE_FB_FBC_C, fbcC);
    VpuWriteReg(coreIdx, W6_CMD_DEC_UPDATE_FB_FBC_Y_OFFSET, fbcYTbl);
    VpuWriteReg(coreIdx, W6_CMD_DEC_UPDATE_FB_FBC_C_OFFSET, fbcCTbl);
    VpuWriteReg(coreIdx, W6_CMD_DEC_UPDATE_FB_MV_COL, mvCol);

    VpuWriteReg(coreIdx, W6_CMD_DEC_SET_FB_OPTION, 1);
    W6SendCommand(coreIdx, inst, W6_SET_FB);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(coreIdx, W6_RET_SUCCESS) == 0)
        return RETCODE_FAILURE;

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuDecode(CodecInst* instance, DecParam* option)
{
    DecInfo*      info = &instance->CodecInfo->decInfo;
    VpuAttr*      attr = &g_VpuCoreAttributes[instance->coreIdx];
    DecOpenParam* pOpenParam = &info->openParam;
    Uint32        modeOption   = DEC_PIC_NORMAL, bsOption, regVal;
    Int32         forceLatency = -1;

    if (info->thumbnailMode) {
        modeOption = DEC_PIC_W_THUMBNAIL;
    }
    else if (option->skipframeMode) {
        switch (option->skipframeMode) {
        case WAVE_SKIPMODE_NON_IRAP:
            modeOption   = SKIP_NON_IRAP;
            forceLatency = 0;
            break;
        case WAVE_SKIPMODE_NON_REF:
            modeOption = SKIP_NON_REF_PIC;
            break;
        default:
            // skip off
            break;
        }
    }

    // set disable reorder
    if (info->reorderEnable == FALSE) {
        forceLatency = 0;
    }

    /* Set attributes of bitstream buffer controller */
    bsOption = 0;
    regVal = 0;
    switch (pOpenParam->bitstreamMode) {
    case BS_MODE_INTERRUPT:
        bsOption = 0;
        break;
    case BS_MODE_PIC_END:
        bsOption = BSOPTION_ENABLE_EXPLICIT_END;
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_BS_RD_PTR, info->streamRdPtr);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_BS_WR_PTR, info->streamWrPtr);
    if (info->streamEndflag == 1) {
        bsOption = 3;   // (streamEndFlag<<1) | EXPLICIT_END
    }
    if (pOpenParam->bitstreamMode == BS_MODE_PIC_END) {
        bsOption |= (1UL<<31);
    }
    if (instance->codecMode == W_AV1_DEC) {
        bsOption |= ((pOpenParam->av1Format) << 2);
    }
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_BS_OPTION, bsOption);

    /* Secondary AXI */
    regVal = (info->secAxiInfo.u.wave.useIpEnable << 1) |
             (info->secAxiInfo.u.wave.useLfRowEnable <<0);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_USE_SEC_AXI, regVal);

    /* Set attributes of User buffer */
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_USERDATA_MASK, info->userDataEnable);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_USERDATA_BASE, info->userDataBufAddr);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_USERDATA_SIZE, info->userDataBufSize);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_USERDATA_PARAM, VPU_USER_DATA_ENDIAN&VDI_128BIT_ENDIAN_MASK);

    VpuWriteReg(instance->coreIdx, W6_COMMAND_OPTION, ((option->disableFilmGrain<<6) | (option->craAsBlaFlag<<5) | modeOption));
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_TEMPORAL_ID_PLUS1, ((info->targetSpatialId + 1) << 9) | (info->tempIdSelectMode << 8) | (info->targetTempId + 1));
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_SEQ_CHANGE_ENABLE_FLAG, info->seqChangeMask);
    VpuWriteReg(instance->coreIdx, W6_CMD_DEC_PIC_FORCE_FB_LATENCY_PLUS1, forceLatency + 1);
    W6SendCommand(instance->coreIdx, instance, W6_DEC_PIC);
    if (attr->supportCommandQueue) {
        if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        regVal = VpuReadReg(instance->coreIdx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((regVal >> instance->instIndex) & 0x1);
        regVal = VpuReadReg(instance->coreIdx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty = ((regVal >> instance->instIndex) & 0x1);

        if (VpuReadReg(instance->coreIdx, W6_RET_SUCCESS) == FALSE) {
            regVal = VpuReadReg(instance->coreIdx, W6_RET_FAIL_REASON);
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

            if (regVal == WAVE5_SYSERR_QUEUEING_FAIL)
                return RETCODE_QUEUEING_FAILURE;
            else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
                return RETCODE_MEMORY_ACCESS_VIOLATION;
            else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            else if (regVal == WAVE5_SYSERR_VLC_BUF_FULL)
                return RETCODE_VLC_BUF_FULL;
            else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
                return RETCODE_VPU_BUS_ERROR;
            else
                return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuDecGetResult(CodecInst* instance, DecOutputInfo* result)
{
    VpuAttr* attr = &g_VpuCoreAttributes[instance->coreIdx];
    Uint32 regVal, subLayerInfo, index, nalUnitType;
    DecInfo* info = VPU_HANDLE_TO_DECINFO(instance);
    RetCode retCode = RETCODE_SUCCESS;
    vpu_instance_pool_t* instancePool = NULL;

    if (attr->supportCommandQueue) {
        retCode = SendQuery(instance->coreIdx, instance, W6_QUERY_GET_RESULT);
        if (retCode != RETCODE_SUCCESS)
            return retCode;

        regVal = VpuReadReg(instance->coreIdx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((regVal >> instance->instIndex) & 0x1);
        regVal = VpuReadReg(instance->coreIdx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty = ((regVal >> instance->instIndex) & 0x1);

        result->decodingSuccess = VpuReadReg(instance->coreIdx, W6_RET_DEC_DECODING_SUCCESS);

#if 0
        if (!result->decodingSuccess) {
            result->errorReason = VpuReadReg(instance->coreIdx, W6_RET_DEC_ERR_INFO);
            retCode = RETCODE_FAILURE;
            VLOG(INFO, "W6_RET_DEC_DECODING_SUCCESS\n");
        }
        else {
            result->warnInfo = VpuReadReg(instance->coreIdx, W6_RET_DEC_WARN_INFO);
        }
#endif
    }
    else {
        result->decodingSuccess = VpuReadReg(instance->coreIdx, W6_RET_SUCCESS);
        if (result->decodingSuccess == FALSE) {
            result->errorReason = VpuReadReg(instance->coreIdx, W6_RET_FAIL_REASON);
            retCode = RETCODE_FAILURE;
            return retCode;
        }
    }

    result->decOutputExtData.userDataBufAddr = VpuReadReg(instance->coreIdx, W6_RET_DEC_USERDATA_BASE);
    result->decOutputExtData.userDataSize    = VpuReadReg(instance->coreIdx, W6_RET_DEC_USERDATA_SIZE);
    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_USERDATA_IDC);
    if (regVal) {
        result->decOutputExtData.userDataHeader  = regVal;
        result->decOutputExtData.userDataBufFull = (regVal&0x2);
        result->decOutputExtData.userDataNum     = 0;
        for (index = 2; index < 32; index++) {
            if ((regVal>>index) & 0x1)
                result->decOutputExtData.userDataNum++;
        }
    } else {
        result->decOutputExtData.userDataHeader  = 0;
        result->decOutputExtData.userDataNum     = 0;
        result->decOutputExtData.userDataBufFull = 0;
    }

    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_PIC_TYPE);

    nalUnitType     = (regVal & 0x3f0) >> 4;
    result->nalType = nalUnitType;

    if (instance->codecMode == W_VP9_DEC) {
        if      (regVal&0x01) result->picType = PIC_TYPE_I;
        else if (regVal&0x02) result->picType = PIC_TYPE_P;
        else if (regVal&0x04) result->picType = PIC_TYPE_REPEAT;
        else                  result->picType = PIC_TYPE_MAX;
    }
    else if (instance->codecMode == W_HEVC_DEC) {
        if      (regVal&0x04) result->picType = PIC_TYPE_B;
        else if (regVal&0x02) result->picType = PIC_TYPE_P;
        else if (regVal&0x01) result->picType = PIC_TYPE_I;
        else                  result->picType = PIC_TYPE_MAX;
        if ((nalUnitType == 19 || nalUnitType == 20) && (result->picType == PIC_TYPE_I)) {
            /* IDR_W_RADL, IDR_N_LP */
            result->picType = PIC_TYPE_IDR;
        }
    }
    else if (instance->codecMode == W_AVC_DEC) {
        if      (regVal&0x04) result->picType = PIC_TYPE_B;
        else if (regVal&0x02) result->picType = PIC_TYPE_P;
        else if (regVal&0x01) result->picType = PIC_TYPE_I;
        else                  result->picType = PIC_TYPE_MAX;
        if ((nalUnitType == 5) && (result->picType == PIC_TYPE_I)) {
            /* IDR */
            result->picType = PIC_TYPE_IDR;
        }
    }
    else { // AV1_DEC
        switch (regVal & 0x07) {
        case 0: result->picType = PIC_TYPE_KEY;        break;
        case 1: result->picType = PIC_TYPE_INTER;      break;
        case 2: result->picType = PIC_TYPE_AV1_INTRA;  break;
        case 3: result->picType = PIC_TYPE_AV1_SWITCH; break;
        default:
            result->picType = PIC_TYPE_MAX; break;
        }
    }

    result->outputFlag                = (regVal>>31)&0x1;
    result->ctuSize                   = 16<<((regVal>>10)&0x3);

    index                             = VpuReadReg(instance->coreIdx, W6_RET_DEC_DISPLAY_INDEX);
    result->indexFrameDisplay         = index;
    result->indexFrameDisplayForTiled = index;

    index                             = VpuReadReg(instance->coreIdx, W6_RET_DEC_DECODED_INDEX);
    result->indexFrameDecoded         = index;
    result->indexFrameDecodedForTiled = index;

    subLayerInfo = VpuReadReg(instance->coreIdx, W6_RET_DEC_SUB_LAYER_INFO);
    result->temporalId = subLayerInfo&0x7;

    if (instance->codecMode == W_HEVC_DEC) {
        result->decodedPOC = -1;
        result->displayPOC = -1;
        if (result->indexFrameDecoded >= 0 || result->indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
            result->decodedPOC = VpuReadReg(instance->coreIdx, W6_RET_DEC_PIC_POC);
        }
    }
    else if (instance->codecMode == W_AVC_DEC) {
        result->decodedPOC = -1;
        result->displayPOC = -1;
        if (result->indexFrameDecoded >= 0 || result->indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
            result->decodedPOC = VpuReadReg(instance->coreIdx, W6_RET_DEC_PIC_POC);
        }
    }
    else if (instance->codecMode == W_AV1_DEC) {
        result->decodedPOC = -1;
        result->displayPOC = -1;
        if (result->indexFrameDecoded >= 0 || result->indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
            result->decodedPOC = VpuReadReg(instance->coreIdx, W6_RET_DEC_PIC_POC);
        }

        result->av1Info.spatialId = (subLayerInfo>>19)&0x3;
        regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_PIC_PARAM);
        result->av1Info.allowIntraBC            = (regVal>>0)&0x1;
        result->av1Info.allowScreenContentTools = (regVal>>1)&0x1;
    }

    result->sequenceChanged = VpuReadReg(instance->coreIdx, W6_RET_DEC_NOTIFICATION);
    if (result->sequenceChanged & SEQ_CHANGE_INTER_RES_CHANGE)
        result->indexInterFrameDecoded = VpuReadReg(instance->coreIdx, W6_RET_DEC_REALLOC_INDEX);

    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_PIC_SIZE);
    result->decPicWidth  = regVal>>16;
    result->decPicHeight = regVal&0xffff;

    if (result->sequenceChanged) {
        osal_memcpy((void*)&info->newSeqInfo, (void*)&info->initialInfo, sizeof(DecInitialInfo));
        GetDecSequenceResult(instance, &info->newSeqInfo);
    }

    result->numOfErrMBs       = VpuReadReg(instance->coreIdx, W6_RET_DEC_ERR_CTB_NUM)>>16;
    result->numOfTotMBs       = VpuReadReg(instance->coreIdx, W6_RET_DEC_ERR_CTB_NUM)&0xffff;
    result->bytePosFrameStart = VpuReadReg(instance->coreIdx, W6_RET_DEC_AU_START_POS);
    result->bytePosFrameEnd   = VpuReadReg(instance->coreIdx, W6_RET_DEC_AU_END_POS);

    regVal = VpuReadReg(instance->coreIdx, W6_RET_DEC_RECOVERY_POINT);
    result->h265RpSei.recoveryPocCnt = regVal & 0xFFFF;            // [15:0]
    result->h265RpSei.exactMatchFlag = (regVal >> 16)&0x01;        // [16]
    result->h265RpSei.brokenLinkFlag = (regVal >> 17)&0x01;        // [17]
    result->h265RpSei.exist =  (regVal >> 18)&0x01;                // [18]
    if(result->h265RpSei.exist == 0) {
        result->h265RpSei.recoveryPocCnt = 0;
        result->h265RpSei.exactMatchFlag = 0;
        result->h265RpSei.brokenLinkFlag = 0;
    }

    result->cycle.hostCmdStartTick    = VpuReadReg(instance->coreIdx, W6_CMD_CQ_IN_TICK);
    result->cycle.hostCmdEndTick      = VpuReadReg(instance->coreIdx, W6_CMD_RQ_OUT_TICK);
    result->cycle.processingStartTick = VpuReadReg(instance->coreIdx, W6_CMD_HW_RUN_TICK);
    result->cycle.processingEndTick   = VpuReadReg(instance->coreIdx, W6_CMD_HW_DONE_TICK);
    result->cycle.vpuStartTick        = VpuReadReg(instance->coreIdx, W6_CMD_FW_RUN_TICK);
    result->cycle.vpuEndTick          = VpuReadReg(instance->coreIdx, W6_CMD_FW_DONE_TICK);
    result->cycle.frameCycle          = (result->cycle.vpuEndTick - result->cycle.hostCmdStartTick) * info->cyclePerTick;
    result->cycle.processingCycle     = (result->cycle.processingEndTick - result->cycle.processingStartTick) * info->cyclePerTick;
    result->cycle.vpuCycle            = ((result->cycle.vpuEndTick - result->cycle.vpuStartTick) - ( result->cycle.processingEndTick - result->cycle.processingStartTick)) * info->cyclePerTick;
    if (attr->supportCommandQueue) {
        result->cycle.preProcessingStartTick = VpuReadReg(instance->coreIdx, W6_CMD_HW_PRE_RUN_TICK);
        result->cycle.preProcessingEndTick   = VpuReadReg(instance->coreIdx, W6_CMD_HW_PRE_DONE_TICK);
        result->cycle.preVpuStartTick        = VpuReadReg(instance->coreIdx, W6_CMD_FW_PRE_RUN_TICK);
        result->cycle.preVpuEndTick          = VpuReadReg(instance->coreIdx, W6_CMD_FW_PRE_DONE_TICK);

        instancePool = vdi_get_instance_pool(instance->coreIdx);
        if (instancePool) {
            if (info->firstCycleCheck == FALSE) {
                result->cycle.frameCycle = (result->cycle.vpuEndTick - result->cycle.hostCmdStartTick) * info->cyclePerTick;
                info->firstCycleCheck = TRUE;
            }
            else {
                result->cycle.frameCycle = (result->cycle.vpuEndTick - instancePool->lastPerformanceCycles) * info->cyclePerTick;
            }
            instancePool->lastPerformanceCycles = result->cycle.vpuEndTick;
        }
        result->cycle.preProcessingCycle = (result->cycle.preProcessingEndTick - result->cycle.preProcessingStartTick) * info->cyclePerTick;
        result->cycle.preVpuCycle        = ((result->cycle.preProcessingStartTick - result->cycle.preVpuStartTick) + (result->cycle.preVpuEndTick - result->cycle.preProcessingEndTick)) * info->cyclePerTick;
    }


    return RETCODE_SUCCESS;
}

RetCode Wave6VpuReInit(Uint32 core_idx, void* fw, Uint32 fw_size)
{
    vpu_buffer_t    vb;
    PhysicalAddress codeBase;
    PhysicalAddress oldCodeBase;
    Uint32          codeSize;
    RetCode         ret = RETCODE_SUCCESS;

    vdi_get_common_memory(core_idx, &vb);

    codeBase = vb.phys_addr;
    /* ALIGN TO 4KB */
    codeSize = (WAVE6_MAX_CODE_BUF_SIZE & ~0xfff);
    if (codeSize < fw_size*2)
        return RETCODE_INSUFFICIENT_RESOURCE;

    oldCodeBase = VpuReadReg(core_idx, W6_VPU_REMAP_PADDR);
    oldCodeBase = ((PhysicalAddress)PROC_AXI_EXT_BASE<<32) | oldCodeBase;

    if (oldCodeBase != codeBase + ((WAVE6_MAX_CODE_BUF_SIZE/W_REMAP_MAX_SIZE)-1)*W_REMAP_MAX_SIZE) {
        ret = Wave6VpuReset(core_idx);
        if (ret != RETCODE_SUCCESS)
            return ret;

        ret = Wave6VpuInit(core_idx, fw, fw_size);
    } else {

        /* Interrupt */
        W6EnableInterrupt(core_idx);

        ret = Wave6VpuGetVersion(core_idx, NULL, NULL);
    }

    return ret;
}

RetCode Wave6VpuSleepWake(Uint32 core_idx, BOOL is_sleep, const Uint16* code, Uint32 size)
{
    if (is_sleep) {
        // Check the status of VPU. Idle or not.
        if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_BUSY_STATUS) == -1)
            return RETCODE_VPU_STILL_RUNNING;
    } else {
        Uint32 regVal;

        if (Wave6VpuReset(core_idx) == RETCODE_FAILURE)
            return RETCODE_FAILURE;
        W6RemapCodeBuffer(core_idx);


        /* Interrupt */
        W6EnableInterrupt(core_idx);

        W6SendCommand(core_idx, NULL, W6_WAKEUP_VPU);
        if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1) {
            VLOG(ERR, "VPU wakeup(W6_VPU_REMAP_CORE_START) timeout\n");
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        regVal = VpuReadReg(core_idx, W6_RET_SUCCESS);
        if (regVal == 0) {
            Uint32 reasonCode = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
            VLOG(ERR, "VPU wakeup(W6_RET_SUCCESS) failed(%d) REASON CODE(%08x)\n", regVal, reasonCode);
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuReset(Uint32 core_idx)
{
    if (vdi_vpu_reset(core_idx) < 0)
        return RETCODE_FAILURE;

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuSetBusTransaction(Uint32 core_idx, BOOL enable)
{
    RetCode ret = RETCODE_SUCCESS;
    Uint32 val = 0;
    BOOL support_dual_core = FALSE;

    val = VpuReadReg(core_idx, W6_VPU_RET_VPU_CONFIG1);
    if (((val>>26) & 0x1) == 0x01) {
        support_dual_core = TRUE;
    }

    if (enable) {
        if (support_dual_core) {
            vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_CTRL_VCORE0, 0x00);
            vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_CTRL_VCORE1, 0x00);
        } else {
            vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_CTRL, 0x00);
        }
    } else {
        if (support_dual_core) {
            vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_CTRL_VCORE0, 0x808080F7);
            if (vdi_wait_bus_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_BACKBONE_BUS_STATUS_VCORE0) == -1) {
                vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_STATUS_VCORE0, 0x00);
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            }

            vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_CTRL_VCORE1, 0x808080F7);
            if (vdi_wait_bus_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_BACKBONE_BUS_STATUS_VCORE1) == -1) {
                vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_STATUS_VCORE1, 0x00);
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            }
        } else {
            vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_CTRL, 0x808080F7);
            if (vdi_wait_bus_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_BACKBONE_BUS_STATUS) == -1) {
                vdi_fio_write_register(core_idx, W6_BACKBONE_BUS_STATUS, 0x00);
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            }
        }
    }

    return ret;
}

RetCode Wave6VpuDecFiniSeq(CodecInst* instance)
{
    RetCode ret = RETCODE_SUCCESS;

    W6SendCommand(instance->coreIdx, instance, W6_DESTROY_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(instance->coreIdx, W6_RET_SUCCESS) == FALSE) {
        Uint32 regVal;

        regVal = VpuReadReg(instance->coreIdx, W6_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_VPU_STILL_RUNNING)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_QUEUEING_FAIL)
            ret = RETCODE_QUEUEING_FAILURE;
        else if (regVal == WAVE5_SYSERR_VPU_STILL_RUNNING)
            ret = RETCODE_VPU_STILL_RUNNING;
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            ret = RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            ret = RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_VLC_BUF_FULL)
            ret = RETCODE_VLC_BUF_FULL;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            ret = RETCODE_VPU_BUS_ERROR;
        else
            ret = RETCODE_FAILURE;
    }

    return ret;
}

RetCode Wave6VpuDecFlush(CodecInst* instance, FramebufferIndex* framebufferIndexes, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;

    W6SendCommand(instance->coreIdx, instance, W6_FLUSH_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(instance->coreIdx, W6_RET_SUCCESS) == FALSE) {
        Uint32 regVal;

        regVal = VpuReadReg(instance->coreIdx, W6_RET_FAIL_REASON);
        VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_QUEUEING_FAIL)
            return RETCODE_QUEUEING_FAILURE;
        else if (regVal == WAVE5_SYSERR_VPU_STILL_RUNNING)
            return RETCODE_VPU_STILL_RUNNING;
        if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_VLC_BUF_FULL)
            return RETCODE_VLC_BUF_FULL;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            return RETCODE_VPU_BUS_ERROR;
        else
            return RETCODE_FAILURE;
    }

    return ret;

}

RetCode Wave6VpuDecGetAuxBufSize(CodecInst* inst, DecAuxBufferSizeInfo sizeInfo, Uint32* size)
{
    Uint32 width = sizeInfo.width;
    Uint32 height = sizeInfo.height;
    Uint32 bufSize = 0;

    if (sizeInfo.type == AUX_BUF_FBC_Y_TBL) {
        switch (inst->codecMode) {
        case W_HEVC_DEC: bufSize = WAVE6_FBC_LUMA_TABLE_SIZE(width, height); break;
        case W_VP9_DEC:  bufSize = WAVE6_FBC_LUMA_TABLE_SIZE(VPU_ALIGN64(width), VPU_ALIGN64(height)); break;
        case W_AVC_DEC:  bufSize = WAVE6_FBC_LUMA_TABLE_SIZE(width, height); break;
        case W_AV1_DEC:  bufSize = WAVE6_FBC_LUMA_TABLE_SIZE(VPU_ALIGN16(width), VPU_ALIGN8(height)); break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
        bufSize = VPU_ALIGN16(bufSize);
    } else if (sizeInfo.type == AUX_BUF_FBC_C_TBL) {
        switch (inst->codecMode) {
        case W_HEVC_DEC: bufSize = WAVE6_FBC_CHROMA_TABLE_SIZE(width, height); break;
        case W_VP9_DEC:  bufSize = WAVE6_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN64(width), VPU_ALIGN64(height)); break;
        case W_AVC_DEC:  bufSize = WAVE6_FBC_CHROMA_TABLE_SIZE(width, height); break;
        case W_AV1_DEC:  bufSize = WAVE6_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN16(width), VPU_ALIGN8(height)); break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
        bufSize = VPU_ALIGN16(bufSize);
    } else if (sizeInfo.type == AUX_BUF_MV_COL) {
        switch (inst->codecMode) {
        case W_HEVC_DEC: bufSize = WAVE6_DEC_HEVC_MVCOL_BUF_SIZE(width, height); break;
        case W_VP9_DEC:  bufSize = WAVE6_DEC_VP9_MVCOL_BUF_SIZE(width, height); break;
        case W_AVC_DEC:  bufSize = WAVE6_DEC_AVC_MVCOL_BUF_SIZE(width, height); break;
        case W_AV1_DEC:  bufSize = WAVE6_DEC_AV1_MVCOL_BUF_SIZE(width, height); break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
        bufSize = VPU_ALIGN16(bufSize);
    } else if (sizeInfo.type == AUX_BUF_DEF_CDF) {
        if (inst->codecMode == W_AV1_DEC) {
            bufSize = WAVE6_AV1_DEFAULT_CDF_BUF_SIZE;
        } else {
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
    } else if (sizeInfo.type == AUX_BUF_SEG_MAP) {
        if (inst->codecMode == W_VP9_DEC) {
            bufSize = WAVE6_VP9_SEGMAP_BUF_SIZE(width, height)*2;
        } else {
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
    }
    else {
        return RETCODE_INVALID_PARAM;
    }

    *size = bufSize;

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuDecRegisterAuxBuffer(CodecInst* inst, AuxBufferInfo auxBufferInfo)
{
    DecInfo* info = &inst->CodecInfo->decInfo;
    AuxBuffer* auxBufs = auxBufferInfo.bufArray;
    DecAuxBufferSizeInfo sizeInfo;
    RetCode ret = RETCODE_SUCCESS;
    Uint32 expectedSize;
    Uint32 i = 0;

    sizeInfo.width = info->initialInfo.picWidth;
    sizeInfo.height = info->initialInfo.picHeight;
    sizeInfo.type = auxBufferInfo.type;

    ret = Wave6VpuDecGetAuxBufSize(inst, sizeInfo, &expectedSize);
    if (ret != RETCODE_SUCCESS)
        return ret;

    switch (auxBufferInfo.type) {
    case AUX_BUF_FBC_Y_TBL:
        for (i = 0; i < auxBufferInfo.num; i++) {
            if (expectedSize > auxBufs[i].size)
                return RETCODE_INSUFFICIENT_RESOURCE;

            info->vbFbcYTbl[auxBufs[i].index].phys_addr = auxBufs[i].addr;
        }
        break;
    case AUX_BUF_FBC_C_TBL:
        for (i = 0; i < auxBufferInfo.num; i++) {
            if (expectedSize > auxBufs[i].size)
                return RETCODE_INSUFFICIENT_RESOURCE;

            info->vbFbcCTbl[auxBufs[i].index].phys_addr = auxBufs[i].addr;
        }
        break;
    case AUX_BUF_MV_COL:
        for (i = 0; i < auxBufferInfo.num; i++) {
            if (expectedSize > auxBufs[i].size)
                return RETCODE_INSUFFICIENT_RESOURCE;

            info->vbMV[auxBufs[i].index].phys_addr = auxBufs[i].addr;
        }
        break;
    case AUX_BUF_DEF_CDF:
        if (expectedSize > auxBufs[0].size)
            return RETCODE_INSUFFICIENT_RESOURCE;

        info->vbDefCdf.phys_addr = auxBufs[0].addr;
        break;
    case AUX_BUF_SEG_MAP:
        if (expectedSize > auxBufs[0].size)
            return RETCODE_INSUFFICIENT_RESOURCE;

        info->vbSegMap.phys_addr = auxBufs[0].addr;
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

    return ret;
}

RetCode Wave6VpuDecGiveCommand(CodecInst* instance, CodecCommand cmd, void* param)
{
    RetCode ret = RETCODE_SUCCESS;
    DecInfo* info;

    info = &instance->CodecInfo->decInfo;

    if (param == NULL)
        return RETCODE_INVALID_PARAM;

    switch (cmd) {
    case DEC_GET_QUEUE_STATUS:
        {
            QueueStatusInfo* queueInfo = (QueueStatusInfo*)param;
            queueInfo->instanceQueueFull = info->instanceQueueFull;
            queueInfo->reportQueueEmpty  = info->reportQueueEmpty;

            break;
        }
    case SET_SEC_AXI:
        {
            SecAxiUse secAxiUse = *(SecAxiUse *)param;

            info->secAxiInfo.u.wave.useIpEnable    = secAxiUse.u.wave.useIpEnable;
            info->secAxiInfo.u.wave.useLfRowEnable = secAxiUse.u.wave.useLfRowEnable;

            break;
        }
    case DEC_SET_WTL_FRAME_FORMAT:
        {
            FrameBufferFormat frameFormat = *(FrameBufferFormat *)param;

            if ((frameFormat != FORMAT_420)               &&
                (frameFormat != FORMAT_420_P10_16BIT_MSB) &&
                (frameFormat != FORMAT_420_P10_16BIT_LSB) &&
                (frameFormat != FORMAT_420_P10_32BIT_MSB) &&
                (frameFormat != FORMAT_420_P10_32BIT_LSB)) {
                return RETCODE_INVALID_PARAM;
            }

            info->wtlFormat = frameFormat;

            break;
        }
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

Int32 Wave6VpuWaitInterrupt(CodecInst* inst, Int32 time)
{
    Int32 reason = 0;

    reason = vdi_wait_interrupt(inst->coreIdx, inst->instIndex, time);

    return reason;
}

RetCode Wave6VpuClearInterrupt(Uint32 core_idx, Uint32 flags)
{
    Uint32 intr_reason;

    intr_reason = VpuReadReg(core_idx, W6_VPU_VINT_REASON_USR);
    intr_reason &= ~flags;
    VpuWriteReg(core_idx, W6_VPU_VINT_REASON_USR, intr_reason);

    return RETCODE_SUCCESS;
}

/************************************************************************/
/*                       ENCODER functions                              */
/************************************************************************/

struct enc_cmd_set_param_reg {
    Uint32 enable;
    Uint32 src_size;
    Uint32 custom_map_endian;
    Uint32 sps_param;
    Uint32 pps_param;
    Uint32 gop_param;
    Uint32 intra_param;
    Uint32 conf_win_top_bot;
    Uint32 conf_win_left_right;
    Uint32 rdo_param;
    Uint32 slice_param;
    Uint32 intra_refresh;
    Uint32 intra_min_max_qp;
    Uint32 rc_frame_rate;
    Uint32 rc_target_rate;
    Uint32 rc_param;
    Uint32 hvs_param;
    Uint32 rc_max_bitrate;
    Uint32 rc_vbv_buffer_size;
    Uint32 inter_min_max_qp;
    Uint32 rot_param;
    Uint32 num_units_in_tick;
    Uint32 time_scale;
    Uint32 num_ticks_poc_diff_one;
    Uint32 bg_param;
    Uint32 non_vcl_param;
    Uint32 vui_rbsp_addr;
    Uint32 hrd_rbsp_addr;
    Uint32 qround_offset;
    Uint32 quant_param_1;
    Uint32 quant_param_2;
    Uint32 custom_gop_param;
    Uint32 custom_gop_pic_param[MAX_GOP_NUM];
    Uint32 tile_param;
    Uint32 custom_lambda[MAX_CUSTOM_LAMBDA_NUM];
    Uint32 temporal_layer_qp[MAX_NUM_CHANGEABLE_TEMPORAL_LAYER];
    Uint32 sfs_param;
};

struct enc_cmd_set_fb_reg {
    Uint32 option;
    Uint32 pic_info;
    Uint32 pic_size;
    Uint32 num_fb;
    Uint32 fbc_stride;
    Uint32 fbc_y[MAX_GDI_IDX];
    Uint32 fbc_c[MAX_GDI_IDX];
    Uint32 fbc_y_offset[MAX_GDI_IDX];
    Uint32 fbc_c_offset[MAX_GDI_IDX];
    Uint32 mv_col[MAX_GDI_IDX];
    Uint32 sub_sampled[MAX_GDI_IDX];
    Uint32 default_cdf;
};

struct enc_cmd_enc_pic_reg {
    Uint32 bs_start;
    Uint32 bs_size;
    Uint32 bs_option;
    Uint32 use_sec_axi;
    Uint32 report_param;
    Uint32 mv_histo_class0;
    Uint32 mv_histo_class1;
    Uint32 custom_map_param;
    Uint32 custom_map_addr;
    Uint32 src_pic_idx;
    Uint32 src_addr_y;
    Uint32 src_addr_u;
    Uint32 src_addr_v;
    Uint32 src_stride;
    Uint32 src_format;
    Uint32 src_axi_sel;
    Uint32 code_option;
    Uint32 pic_param;
    Uint32 longterm_pic;
    Uint32 prefix_sei_nal_addr;
    Uint32 prefix_sei_info;
    Uint32 suffix_sei_nal_addr;
    Uint32 suffix_sei_info;
};

RetCode Wave6VpuBuildUpEncParam(CodecInst* inst, EncOpenParam* param)
{
    RetCode  ret = RETCODE_SUCCESS;
    EncInfo* info = &inst->CodecInfo->encInfo;
    Uint32   temp = 0;
    Uint32   core_idx = inst->coreIdx;

    if (param->bitstreamFormat == STD_HEVC) {
        inst->codecMode = W_HEVC_ENC;
    } else if (param->bitstreamFormat == STD_AVC) {
        inst->codecMode = W_AVC_ENC;
    } else if (param->bitstreamFormat == STD_AV1) {
        inst->codecMode = W_AV1_ENC;
    } else {
        return RETCODE_INVALID_PARAM;
    }

    info->streamRdPtrRegAddr  = W6_RET_ENC_RD_PTR;
    info->streamWrPtrRegAddr  = W6_RET_ENC_WR_PTR;
    info->currentPC           = W6_VCPU_CUR_PC;
    info->busyFlagAddr        = W6_VPU_CMD_BUSY_STATUS;
    info->streamRdPtr         = param->bitstreamBuffer;
    info->streamWrPtr         = param->bitstreamBuffer;
    info->lineBufIntEn        = param->lineBufIntEn;
    info->streamBufStartAddr  = param->bitstreamBuffer;
    info->streamBufSize       = param->bitstreamBufferSize;
    info->streamBufEndAddr    = param->bitstreamBuffer + param->bitstreamBufferSize;
    info->stride              = 0;
    info->vbFrame.size        = 0;
    info->vbPPU.size          = 0;
    info->frameAllocExt       = 0;
    info->ppuAllocExt         = 0;
    info->initialInfoObtained = 0;
    info->streamEndian        = to_hw_endian(core_idx, param->streamEndian);
    info->sourceEndian        = to_hw_endian(core_idx, param->sourceEndian);
    info->customMapEndian     = to_hw_endian(core_idx, param->EncStdParam.wave6Param.customMapEndian);

    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_TEMP_BASE, param->instBuffer.tempBufBase);
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_TEMP_SIZE, param->instBuffer.tempBufSize);
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_SEC_AXI_BASE, param->instBuffer.secAxiBufBase);
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_SEC_AXI_SIZE, param->instBuffer.secAxiBufSize);
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_AR_TABLE_BASE, param->instBuffer.arTblBufBase);
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_WORK_BASE, param->instBuffer.workBufBase);
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_WORK_SIZE, param->instBuffer.workBufSize);
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_BS_PARAM, info->streamEndian);

    temp = 0;
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_SRC_OPT, temp);

    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_ADDR_EXT, param->extAddrVcpu);
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_CORE_INFO, (1<<4) | (1<<0));
    VpuWriteReg(core_idx, W6_CMD_ENC_CREATE_INST_PRIORITY, (param->isSecureInst        <<8) |
                                                           ((param->instPriority&0xFF) <<0));

    W6SendCommand(core_idx, inst, W6_CREATE_INSTANCE);
    if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(core_idx, W6_RET_SUCCESS) == FALSE) {
        temp = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
        VLOG(ERR, "FAIL_REASON = 0x%x\n", temp);

        if (temp == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            ret = RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (temp == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            ret = RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (temp == WAVE5_SYSERR_BUS_ERROR || temp == WAVE5_SYSERR_DOUBLE_FAULT)
            ret = RETCODE_VPU_BUS_ERROR;
        else
            ret = RETCODE_FAILURE;
    }

    inst->instIndex = VpuReadReg(core_idx, W6_RET_INSTANCE_ID);

    return ret;
}

static void gen_set_param_reg(EncInfo *info, Int32 codec, struct enc_cmd_set_param_reg *reg)
{
    EncOpenParam *open_param = &info->openParam;
    EncWave6Param *param = &open_param->EncStdParam.wave6Param;
    Uint32 rot = 0, i = 0;

    if (info->rotationEnable == TRUE) {
        switch (info->rotationAngle) {
        case 0:   rot |= 0x0; break;
        case 90:  rot |= 0x3; break;
        case 180: rot |= 0x5; break;
        case 270: rot |= 0x7; break;
        }
    }
    if (info->mirrorEnable == TRUE) {
        switch (info->mirrorDirection) {
        case MIRDIR_NONE:    rot |= 0x0;  break;
        case MIRDIR_VER:     rot |= 0x9;  break;
        case MIRDIR_HOR:     rot |= 0x11; break;
        case MIRDIR_HOR_VER: rot |= 0x19; break;
        }
    }

    SetEncCropInfo(codec, &param->confWin, rot, info->encWidth, info->encHeight);

    if (codec == W_HEVC_ENC) {
        reg->sps_param = (param->enScalingList        <<31) |
                         (param->enStillPicture       <<30) |
                         (param->strongIntraSmoothing <<27) |
                         (param->intraTransSkip       <<25) |
                         (param->enSAO                <<24) |
                         (param->enTemporalMVP        <<23) |
                         (param->enLongTerm           <<21) |
                         (param->internalBitDepth     <<14) |
                         (param->tier                 <<12) |
                         (param->level                <<3)  |
                         (param->profile              <<0);
        reg->pps_param = ((param->crQpOffset&0x1F)        <<19) |
                         ((param->cbQpOffset&0x1F)        <<14) |
                         ((param->tcOffsetDiv2&0xF)       <<10) |
                         ((param->betaOffsetDiv2&0xF)     <<6)  |
                         ((!param->enDBK)                 <<5)  | //lint !e514
                         (param->lfCrossSliceBoundaryFlag <<2)  |
                         (param->constrainedIntraPred     <<1);
        reg->intra_param = (param->intraPeriod         <<16) |
                           (param->forcedIdrHeader     <<9)  |
                           (param->qp                  <<3)  |
                           (param->decodingRefreshType <<0);
        reg->rdo_param = (param->enCustomLambda   <<22) |
                         (param->meCenter         <<21) |
                         (param->enQpMap          <<20) |
                         (param->enModeMap        <<19) |
                         (param->enQRoundOffset   <<17) |
                         (param->intraNxN         <<8)  |
                         (0x7                    <<5)  |
                         (param->disableCoefClear <<4)  |
                         (param->enAdaptiveRound  <<3)  |
                         (param->enHvsQp          <<2);
        reg->slice_param = (param->sliceArg  <<3) |
                           (param->sliceMode <<0);
        reg->quant_param_2 = ((param->lambdaDqpInter&0x3F) <<14) |
                             ((param->lambdaDqpIntra&0x3F) <<8);
        reg->non_vcl_param = (open_param->hrdRbspDataSize    <<18) |
                             (open_param->vuiRbspDataSize    <<4)  |
                             (open_param->encodeHrdRbspInVPS <<2)  |
                             (open_param->encodeVuiRbsp      <<1)  |
                             (open_param->encodeAUD          <<0);
        reg->vui_rbsp_addr = open_param->vuiRbspDataAddr;
        reg->hrd_rbsp_addr = open_param->hrdRbspDataAddr;
    } else if (codec == W_AVC_ENC) {
        reg->sps_param = (param->enScalingList    <<31) |
                         (param->enLongTerm       <<21) |
                         (param->internalBitDepth <<14) |
                         (param->level            <<3);
        reg->pps_param = (param->enCABAC                  <<30) |
                         (param->transform8x8             <<29) |
                         ((param->crQpOffset&0x1F)        <<19) |
                         ((param->cbQpOffset&0x1F)        <<14) |
                         ((param->tcOffsetDiv2&0xF)       <<10) |
                         ((param->betaOffsetDiv2&0xF)     <<6)  |
                         ((!param->enDBK)                 <<5)  |  //lint !e514
                         (param->lfCrossSliceBoundaryFlag <<2)  |
                         (param->constrainedIntraPred     <<1);
        reg->intra_param = (param->forcedIdrHeader <<28) |
                           (param->idrPeriod       <<17) |
                           (param->intraPeriod     <<6)  |
                           (param->qp              <<0);
        reg->rdo_param = (param->enCustomLambda   <<22) |
                         (param->meCenter         <<21) |
                         (param->enQpMap          <<20) |
                         (param->enModeMap        <<19) |
                         (param->enQRoundOffset   <<17) |
                         (0x7                    <<5)  |
                         (param->disableCoefClear <<4)  |
                         (param->enAdaptiveRound  <<3)  |
                         (param->enHvsQp          <<2);
        reg->slice_param = (param->sliceArg  <<3) |
                           (param->sliceMode <<0);
        reg->quant_param_2 = ((param->lambdaDqpInter&0x3F) <<14) |
                             ((param->lambdaDqpIntra&0x3F) <<8);
        reg->non_vcl_param = (open_param->hrdRbspDataSize    <<18) |
                             (open_param->vuiRbspDataSize    <<4)  |
                             (open_param->encodeHrdRbspInVPS <<2)  |
                             (open_param->encodeVuiRbsp      <<1)  |
                             (open_param->encodeAUD          <<0);
        reg->vui_rbsp_addr = open_param->vuiRbspDataAddr;
        reg->hrd_rbsp_addr = open_param->hrdRbspDataAddr;
    } else if (codec == W_AV1_ENC) {
        reg->sps_param = (param->enScalingList    <<31) |
                         (param->intraTransSkip   <<25) |
                         (param->enWiener         <<24) |
                         (param->enCdef           <<23) |
                         (param->enLongTerm       <<21) |
                         (param->internalBitDepth <<14) |
                         (param->level            <<3);
        reg->pps_param = (param->lfSharpness <<6) |
                         ((!param->enDBK)    <<5);           //lint !e514
        reg->intra_param = (param->intraPeriod     <<16) |
                           (param->forcedIdrHeader <<9)  |
                           (param->qp              <<3);
        reg->rdo_param = (param->enCustomLambda   <<22) |
                         (param->meCenter         <<21) |
                         (param->enQpMap          <<20) |
                         (param->enModeMap        <<19) |
                         (param->enQRoundOffset   <<17) |
                         (param->intraNxN         <<8)  |
                         (0x7                    <<5)  |
                         (param->disableCoefClear <<4)  |
                         (param->enAdaptiveRound  <<3)  |
                         (param->enHvsQp          <<2);
        reg->quant_param_1 = ((param->uDcQpDelta&0xFF) <<24) |
                             ((param->vDcQpDelta&0xFF) <<16) |
                             ((param->uAcQpDelta&0xFF) <<8)  |
                             ((param->vAcQpDelta&0xFF) <<0);
        reg->quant_param_2 = ((param->lambdaDqpInter&0x3F) <<14) |
                             ((param->lambdaDqpIntra&0x3F) <<8)  |
                             ((param->yDcQpDelta&0xFF)     <<0);
        reg->tile_param = ((param->rowTileNum-1) <<4) |
                          ((param->colTileNum-1) <<0);
    }

    reg->src_size = (info->encHeight <<16) |
                    (info->encWidth  <<0);
    reg->custom_map_endian = info->customMapEndian;
    reg->gop_param = (param->tempLayerCnt            <<16) |
                     (param->tempLayer[3].enChangeQp <<11) |
                     (param->tempLayer[2].enChangeQp <<10) |
                     (param->tempLayer[1].enChangeQp <<9)  |
                     (param->tempLayer[0].enChangeQp <<8)  |
                     (param->gopPreset               <<0);
    reg->intra_refresh = (param->intraRefreshArg  <<16) |
                         (param->intraRefreshMode <<0);
    reg->intra_min_max_qp = (param->maxQpI <<6) |
                            (param->minQpI <<0);
    reg->rc_frame_rate = param->frameRate;
    reg->rc_target_rate = param->encBitrate;
    reg->rc_param = (param->rcUpdateSpeed      <<24) |
                    (param->rcInitialLevel     <<20) |
                    ((param->rcInitialQp&0x3f) <<14) |
                    (param->rcMode             <<13) |
                    (param->picRcMaxDqp        <<7)  |
                    (param->cuLevelRateControl <<1)  |
                    (param->enRateControl      <<0);
    reg->hvs_param = (param->maxDeltaQp     <<12) |
                     (param->hvsQpScaleDiv2 <<0);
    reg->rc_max_bitrate = param->maxBitrate;
    reg->rc_vbv_buffer_size = param->vbvBufferSize;
    reg->inter_min_max_qp = (param->maxQpB <<18) |
                            (param->minQpB <<12) |
                            (param->maxQpP <<6)  |
                            (param->minQpP <<0);
    reg->rot_param = rot;
    reg->conf_win_top_bot = (param->confWin.bottom <<16) |
                            (param->confWin.top <<0);
    reg->conf_win_left_right = (param->confWin.right <<16) |
                               (param->confWin.left  <<0);
    reg->num_units_in_tick = param->numUnitsInTick;
    reg->time_scale = param->timeScale;
    reg->num_ticks_poc_diff_one = param->numTicksPocDiffOne;
    reg->bg_param = ((param->bgDeltaQp&0x3F) <<24) |
                    (param->bgThMeanDiff     <<10) |
                    (param->bgThDiff         <<1)  |
                    (param->enBgDetect       <<0);
    reg->qround_offset = (param->qRoundInter <<13) |
                         (param->qRoundIntra <<2);
    reg->custom_gop_param = param->gopParam.customGopSize;
    for (i = 0; i < param->gopParam.customGopSize; i++) {
        CustomGopPicParam pic_param = param->gopParam.picParam[i];
        reg->custom_gop_pic_param[i] = (pic_param.temporalId      <<26) |
                                       ((pic_param.refPocL1&0x3F) <<20) |
                                       ((pic_param.refPocL0&0x3F) <<14) |
                                       (pic_param.useMultiRefP    <<13) |
                                       (pic_param.picQp           <<7)  |
                                       (pic_param.pocOffset       <<2)  |
                                       (pic_param.picType         <<0);
    }
    for (i = 0; i < MAX_CUSTOM_LAMBDA_NUM; i++) {
        reg->custom_lambda[i] = (param->customLambdaSSD[i] <<7) |
                                (param->customLambdaSAD[i] <<0);
    }
    for (i = 0; i < MAX_NUM_CHANGEABLE_TEMPORAL_LAYER; i++) {
        reg->temporal_layer_qp[i] = (param->tempLayer[i].qpB <<12) |
                                    (param->tempLayer[i].qpP <<6)  |
                                    (param->tempLayer[i].qpI <<0);
    }
    reg->sfs_param = (open_param->subFrameSyncMode   <<1) |
                     (open_param->subFrameSyncEnable <<0);
}

static BOOL update_enc_size(EncInfo* info)
{
    info->encWidth  = info->openParam.picWidth;
    info->encHeight = info->openParam.picHeight;

    return TRUE;
}

RetCode Wave6VpuEncInitSeq(CodecInst* inst)
{
    struct enc_cmd_set_param_reg *reg;
    VpuAttr* attr = &g_VpuCoreAttributes[inst->coreIdx];
    Int32 core_idx = inst->coreIdx;
    Uint32 temp = 0;
    Uint32 i = 0;
    EncInfo* info = &inst->CodecInfo->encInfo;

    reg = vzalloc(sizeof(struct enc_cmd_set_param_reg));
    if (reg == NULL)
        return RETCODE_FAILURE;

    if (update_enc_size(info) == FALSE) {
        vfree(reg);
        return RETCODE_INVALID_PARAM;
    }

    gen_set_param_reg(info, inst->codecMode, reg);

    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_OPTION, W6_SET_PARAM_OPT_COMMON);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_ENABLE, reg->enable);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_SRC_SIZE, reg->src_size);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_CUSTOM_MAP_ENDIAN, reg->custom_map_endian);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_SPS_PARAM, reg->sps_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_PPS_PARAM, reg->pps_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_GOP_PARAM, reg->gop_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_INTRA_PARAM, reg->intra_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_CONF_WIN_TOP_BOT, reg->conf_win_top_bot);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_CONF_WIN_LEFT_RIGHT, reg->conf_win_left_right);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RDO_PARAM, reg->rdo_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_SLICE_PARAM, reg->slice_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_INTRA_REFRESH, reg->intra_refresh);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_INTRA_MIN_MAX_QP, reg->intra_min_max_qp);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_FRAME_RATE, reg->rc_frame_rate);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_TARGET_RATE, reg->rc_target_rate);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_PARAM, reg->rc_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_HVS_PARAM, reg->hvs_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_MAX_BITRATE, reg->rc_max_bitrate);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_VBV_BUFFER_SIZE, reg->rc_vbv_buffer_size);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_INTER_MIN_MAX_QP, reg->inter_min_max_qp);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_ROT_PARAM, reg->rot_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_NUM_UNITS_IN_TICK, reg->num_units_in_tick);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_TIME_SCALE, reg->time_scale);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_NUM_TICKS_POC_DIFF_ONE, reg->num_ticks_poc_diff_one);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_BG_PARAM, reg->bg_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_NON_VCL_PARAM, reg->non_vcl_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_VUI_RBSP_ADDR, reg->vui_rbsp_addr);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_HRD_RBSP_ADDR, reg->hrd_rbsp_addr);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_QROUND_OFFSET, reg->qround_offset);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_QUANT_PARAM_1, reg->quant_param_1);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_QUANT_PARAM_2, reg->quant_param_2);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_CUSTOM_GOP_PARAM, reg->custom_gop_param);
    for (i = 0; i < MAX_GOP_NUM; i++) {
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_CUSTOM_GOP_PIC_PARAM_0 + (i*4), reg->custom_gop_pic_param[i]);
    }
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_TILE_PARAM, reg->tile_param);
    for (i = 0; i < MAX_CUSTOM_LAMBDA_NUM; i++) {
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_CUSTOM_LAMBDA_0 + (i*4), reg->custom_lambda[i]);
    }
    for (i = 0; i < MAX_NUM_CHANGEABLE_TEMPORAL_LAYER; i++) {
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_TEMPORAL_LAYER_0_QP + (i*4), reg->temporal_layer_qp[i]);
    }
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_SFS_PARAM, reg->sfs_param);

    W6SendCommand(core_idx, inst, W6_ENC_SET_PARAM);
    vfree(reg);
    if (attr->supportCommandQueue) {
        if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        temp = VpuReadReg(core_idx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((temp>>inst->instIndex)&0x1);
        temp = VpuReadReg(core_idx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty = ((temp>>inst->instIndex)&0x1);

        if (VpuReadReg(core_idx, W6_RET_SUCCESS) == 0) {
            temp = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
            VLOG(ERR, "FAIL_REASON = 0x%x\n", temp);

            if (temp == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
                return RETCODE_MEMORY_ACCESS_VIOLATION;
            else if (temp == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            else if (temp == WAVE5_SYSERR_QUEUEING_FAIL)
                return RETCODE_QUEUEING_FAILURE;
            else if (temp == WAVE5_SYSERR_BUS_ERROR || temp == WAVE5_SYSERR_DOUBLE_FAULT)
                return RETCODE_VPU_BUS_ERROR;
            else
                return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuEncGetSeqInfo(CodecInst* inst, EncInitialInfo* ret)
{
    Uint32 core_idx = inst->coreIdx;
    RetCode retCode = RETCODE_SUCCESS;
    VpuAttr* attr = &g_VpuCoreAttributes[inst->coreIdx];
    EncInfo* info = &inst->CodecInfo->encInfo;
    Uint32 regVal;

    if (attr->supportCommandQueue) {
        retCode = SendQuery(core_idx, inst, W6_QUERY_GET_RESULT);
        if (retCode != RETCODE_SUCCESS)
            return retCode;

        regVal = VpuReadReg(core_idx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((regVal >> inst->instIndex) & 0x1);
        regVal = VpuReadReg(core_idx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty = ((regVal >> inst->instIndex) & 0x1);

        if (VpuReadReg(core_idx, W6_RET_ENC_ENCODING_SUCCESS) == 0) {
            ret->seqInitErrReason = VpuReadReg(core_idx, W6_RET_ENC_ERR_INFO);
            retCode = RETCODE_FAILURE;
            VLOG(INFO, "W6_RET_ENC_ENCODING_SUCCESS\n");
        }
        else {
            ret->warnInfo = VpuReadReg(core_idx, W6_RET_ENC_WARN_INFO);
        }
    }
    else {
        if (VpuReadReg(core_idx, W6_RET_SUCCESS) != 1) {
            ret->seqInitErrReason = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
            retCode = RETCODE_FAILURE;
            return retCode;
        }
    }

    ret->minFrameBufferCount = VpuReadReg(core_idx, W6_RET_ENC_NUM_REQUIRED_FB);
    ret->minSrcFrameCount    = VpuReadReg(core_idx, W6_RET_ENC_MIN_SRC_BUF_NUM);
    ret->maxLatencyPictures  = VpuReadReg(core_idx, W6_RET_ENC_PIC_MAX_LATENCY_PICTURES);

    return retCode;
}

static void gen_set_fb_reg(EncInfo *info, Int32 codec, struct enc_cmd_set_fb_reg *reg)
{
    EncOpenParam *open_param = &info->openParam;
    Uint32 buf_width = 0;
    Uint32 buf_height = 0;
    Uint32 stride_l = 0;
    Uint32 stride_c = 0;
    Uint32 i = 0;

    if (codec == W_AVC_ENC) {
        buf_width  = VPU_ALIGN16(info->encWidth);
        buf_height = VPU_ALIGN16(info->encHeight);
        if (info->rotationAngle == 90 || info->rotationAngle == 270) {
            buf_width  = VPU_ALIGN16(info->encHeight);
            buf_height = VPU_ALIGN16(info->encWidth);
        }
    } else {
        buf_width  = VPU_ALIGN8(info->encWidth);
        buf_height = VPU_ALIGN8(info->encHeight);
        if ((info->rotationAngle != 0 || info->mirrorDirection != 0) &&
            !(info->rotationAngle == 180 && info->mirrorDirection == MIRDIR_HOR_VER)) {
            buf_width  = VPU_ALIGN32(info->encWidth);
            buf_height = VPU_ALIGN32(info->encHeight);
        }
        if (info->rotationAngle == 90 || info->rotationAngle == 270) {
            buf_width  = VPU_ALIGN32(info->encHeight);
            buf_height = VPU_ALIGN32(info->encWidth);
        }
    }

    if ((info->rotationAngle != 0 || info->mirrorDirection != 0) &&
        !(info->rotationAngle == 180 && info->mirrorDirection == MIRDIR_HOR_VER)) {
        stride_l = VPU_ALIGN64(buf_width + 63);
        stride_c = VPU_ALIGN32(buf_width + 31) / 2;
    } else {
        stride_l = VPU_ALIGN64(info->encWidth + 63);
        stride_c = VPU_ALIGN32(info->encWidth + 31) / 2;
    }

    reg->option = (open_param->enableNonRefFbcWrite <<26) |
                  (info->frameBufPool[0].endian     <<16) |
                  (1                                <<4) |
                  (1                                <<3);
    reg->pic_info = info->stride;
    reg->pic_size = (buf_width  <<16) |
                    (buf_height <<0);
    reg->num_fb = ((info->numFrameBuffers-1) <<16) |
                  ((info->numFrameBuffers-1) <<0);
    reg->fbc_stride = (stride_l <<16) |
                      (stride_c <<0);
    reg->default_cdf = info->vbDefCdf.phys_addr;

    for (i = 0; i < info->numFrameBuffers; i++) {
        reg->fbc_y[i] = info->frameBufPool[i].bufY;
        reg->fbc_c[i] = info->frameBufPool[i].bufCb;
        reg->fbc_y_offset[i] = info->vbFbcYTbl[i].phys_addr;
        reg->fbc_c_offset[i] = info->vbFbcCTbl[i].phys_addr;
        reg->mv_col[i] = info->vbMV[i].phys_addr;
        reg->sub_sampled[i] = info->vbSubSamBuf[i].phys_addr;
    }
}

RetCode Wave6VpuEncRegisterFramebuffer(CodecInst* inst, FrameBuffer* fbArr)
{
    struct enc_cmd_set_fb_reg *reg;
    Int32    idx;
    Uint32   core_idx = inst->coreIdx;
    EncInfo* info = &inst->CodecInfo->encInfo;

    reg  = vzalloc(sizeof(struct enc_cmd_set_fb_reg));
    if (reg == NULL)
        return RETCODE_FAILURE;

    if (inst->codecMode == W_AV1_ENC) {
        if (!info->vbDefCdf.phys_addr)
            return RETCODE_INSUFFICIENT_RESOURCE;

        load_av1_cdf_table(core_idx, info->vbDefCdf.phys_addr);
    }

    for (idx = 0; idx < info->numFrameBuffers; idx++) {
        if (!info->vbFbcYTbl[idx].phys_addr)
            return RETCODE_INSUFFICIENT_RESOURCE;
        if (!info->vbFbcCTbl[idx].phys_addr)
            return RETCODE_INSUFFICIENT_RESOURCE;
        if (!info->vbMV[idx].phys_addr)
            return RETCODE_INSUFFICIENT_RESOURCE;
        if (!info->vbSubSamBuf[idx].phys_addr)
            return RETCODE_INSUFFICIENT_RESOURCE;
    }

    gen_set_fb_reg(info, inst->codecMode, reg);

    VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_OPTION, reg->option);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_PIC_INFO, reg->pic_info);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_PIC_SIZE, reg->pic_size);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_NUM, reg->num_fb);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_FBC_STRIDE, reg->fbc_stride);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_DEFAULT_CDF, reg->default_cdf);
    for (idx = 0; idx < info->numFrameBuffers; idx++) {
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_FBC_Y0 + (idx*24), reg->fbc_y[idx]);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_FBC_C0 + (idx*24), reg->fbc_c[idx]);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_FBC_Y_OFFSET0 + (idx*24), reg->fbc_y_offset[idx]);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_FBC_C_OFFSET0 + (idx*24), reg->fbc_c_offset[idx]);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_MV_COL0 + (idx*24), reg->mv_col[idx]);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_FB_SUB_SAMPLED0 + (idx*24), reg->sub_sampled[idx]);
    }

    vfree(reg);
    W6SendCommand(core_idx, inst, W6_SET_FB);
    if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(core_idx, W6_RET_SUCCESS) == 0)
        return RETCODE_FAILURE;

    return RETCODE_SUCCESS;
}

static void gen_enc_pic_reg(EncInfo info, EncParam opt, struct enc_cmd_enc_pic_reg *reg)
{
    EncOpenParam *open_param = &info.openParam;
    EncWave6Param *param = &open_param->EncStdParam.wave6Param;
    BOOL is_lsb = FALSE;
    BOOL is_10bit = FALSE;
    BOOL is_3p4b = FALSE;
    Uint32 stride_c = 0;
    Uint32 src_format = 0;
    BOOL is_ayuv = FALSE;
    BOOL is_csc_format = FALSE;


    switch (open_param->srcFormat) {
    case FORMAT_420:
    case FORMAT_422:
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
        is_lsb   = FALSE;
        is_3p4b  = FALSE;
        stride_c = (open_param->cbcrInterleave == 1) ? opt.sourceFrame->stride : (opt.sourceFrame->stride/2);
        stride_c = (open_param->srcFormat == FORMAT_422) ? stride_c*2 : stride_c;
        break;
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_YUYV_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
        is_lsb   = FALSE;
        is_10bit = TRUE;
        is_3p4b  = FALSE;
        stride_c = (open_param->cbcrInterleave == 1) ? opt.sourceFrame->stride : (opt.sourceFrame->stride/2);
        stride_c = (open_param->srcFormat == FORMAT_422_P10_16BIT_MSB) ? stride_c*2 : stride_c;
        break;
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
        is_lsb   = TRUE;
        is_10bit = TRUE;
        is_3p4b  = FALSE;
        stride_c = (open_param->cbcrInterleave == 1) ? opt.sourceFrame->stride : (opt.sourceFrame->stride/2);
        stride_c = (open_param->srcFormat == FORMAT_422_P10_16BIT_LSB) ? stride_c*2 : stride_c;
        break;
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_422_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
        is_lsb   = FALSE;
        is_10bit = TRUE;
        is_3p4b  = TRUE;
        stride_c = (open_param->cbcrInterleave == 1) ? opt.sourceFrame->stride : VPU_ALIGN16(opt.sourceFrame->stride/2)*(1<<open_param->cbcrInterleave);
        stride_c = (open_param->srcFormat == FORMAT_422_P10_32BIT_MSB) ? stride_c*2 : stride_c;
        break;
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        is_lsb   = TRUE;
        is_10bit = TRUE;
        is_3p4b  = TRUE;
        stride_c = (open_param->cbcrInterleave == 1) ? opt.sourceFrame->stride : VPU_ALIGN16(opt.sourceFrame->stride/2)*(1<<open_param->cbcrInterleave);
        stride_c = (open_param->srcFormat == FORMAT_422_P10_32BIT_LSB) ? stride_c*2 : stride_c;
        break;
    case FORMAT_RGB_32BIT_PACKED:
        is_ayuv       = FALSE;
        is_csc_format = TRUE;
        break;
    case FORMAT_RGB_P10_32BIT_PACKED:
        is_ayuv       = FALSE;
        is_csc_format = TRUE;
        is_10bit      = TRUE;
        break;
    case FORMAT_YUV444_32BIT_PACKED:
        is_ayuv       = TRUE;
        is_csc_format = TRUE;
        break;
    case FORMAT_YUV444_P10_32BIT_PACKED:
        is_ayuv       = TRUE;
        is_csc_format = TRUE;
        is_10bit      = TRUE;
        break;
    default:
        break;
    }

    src_format = (open_param->nv21 <<2) | (open_param->cbcrInterleave <<1);
    switch (open_param->packedFormat) {
    case PACKED_YUYV:
        src_format = 1;
        break;
    case PACKED_YVYU:
        src_format = 5;
        break;
    case PACKED_UYVY:
        src_format = 9;
        break;
    case PACKED_VYUY:
        src_format = 13;
        break;
    default:
        break;
    }

    reg->bs_start = opt.picStreamBufferAddr;
    reg->bs_size = opt.picStreamBufferSize;
    reg->bs_option = (1U                <<31) |
                     (info.lineBufIntEn <<6);
    reg->use_sec_axi = (info.secAxiInfo.u.wave.useEncRdoEnable <<1) |
                       (info.secAxiInfo.u.wave.useEncLfEnable  <<0);
    reg->report_param = (param->enReportMvHisto <<1);
    reg->mv_histo_class0 = (param->reportMvHistoThreshold0 <<16) |
                           (param->reportMvHistoThreshold1 <<0);
    reg->mv_histo_class1 = (param->reportMvHistoThreshold2 <<16) |
                           (param->reportMvHistoThreshold3 <<0);
    reg->custom_map_param = (opt.customMapOpt.customModeMapEnable <<1) |
                            (opt.customMapOpt.customRoiMapEnable  <<0);
    reg->custom_map_addr = opt.customMapOpt.addrCustomMap;

    if (opt.srcEndFlag)
        reg->src_pic_idx = 0xFFFFFFFF;
    else
        reg->src_pic_idx = opt.srcIdx;

    reg->src_addr_y = opt.sourceFrame->bufY;
    if (open_param->cbcrOrder == CBCR_ORDER_NORMAL) {
        reg->src_addr_u = opt.sourceFrame->bufCb;
        reg->src_addr_v = opt.sourceFrame->bufCr;
    } else {
        reg->src_addr_u = opt.sourceFrame->bufCr;
        reg->src_addr_v = opt.sourceFrame->bufCb;
    }
    reg->src_stride = (opt.sourceFrame->stride <<16) |
                      (stride_c                <<0);
    reg->src_format = (is_ayuv                       <<24) |
                      (opt.updateLast2Bit            <<23) |
                      ((opt.last2BitData&0x3)        <<21) |
                      (is_csc_format                 <<20) |
                      (opt.csc.formatOrder           <<16) |
                      (info.sourceEndian             <<12) |
                      (is_lsb                        <<6)  |
                      (is_3p4b                       <<5)  |
                      (is_10bit                      <<4)  |
                      (src_format                    <<0);
    reg->src_axi_sel = DEFAULT_SRC_AXI;
    if (opt.codeOption.implicitHeaderEncode) {
        reg->code_option = (opt.srcEndFlag           <<10) |
                           (opt.codeOption.encodeEOB <<7)  |
                           (opt.codeOption.encodeEOS <<6)  |
                           (CODEOPT_ENC_VCL)               |
                           (CODEOPT_ENC_HEADER_IMPLICIT);
    } else {
        reg->code_option = (opt.codeOption.encodeEOB            <<7) |
                           (opt.codeOption.encodeEOS            <<6) |
                           (opt.codeOption.encodePPS            <<4) |
                           (opt.codeOption.encodeSPS            <<3) |
                           (opt.codeOption.encodeVPS            <<2) |
                           (opt.codeOption.encodeVCL            <<1) |
                           (opt.codeOption.implicitHeaderEncode <<0);
    }
    reg->pic_param = (opt.forcePicType              <<21) |
                     (opt.forcePicTypeEnable        <<20) |
                     (opt.forcePicQpB               <<14) |
                     (opt.forcePicQpP               <<8)  |
                     (opt.forcePicQpI               <<2)  |
                     (opt.forcePicQpEnable          <<1)  |
                     (opt.skipPicture               <<0);
    reg->longterm_pic = (opt.useLongtermRef         <<1) |
                        (opt.useCurSrcAsLongtermPic <<0);
    reg->prefix_sei_nal_addr = info.prefixSeiNalAddr;
    reg->prefix_sei_info = (info.prefixSeiDataSize  <<16) |
                           (info.prefixSeiNalEnable <<0);
    reg->suffix_sei_nal_addr = info.suffixSeiNalAddr;
    reg->suffix_sei_info = (info.suffixSeiDataSize  <<16) |
                           (info.suffixSeiNalEnable <<0);
}

RetCode Wave6VpuEncode(CodecInst* inst, EncParam* opt)
{
    struct enc_cmd_enc_pic_reg *reg;
    Int32 core_idx = inst->coreIdx;
    VpuAttr* attr = &g_VpuCoreAttributes[inst->coreIdx];
    EncInfo* info = &inst->CodecInfo->encInfo;
    Uint32 regVal;

    reg = vzalloc(sizeof(struct enc_cmd_enc_pic_reg));
    if (reg == NULL)
        return RETCODE_FAILURE;

    info->streamWrPtr        = opt->picStreamBufferAddr;
    info->streamRdPtr        = opt->picStreamBufferAddr;
    info->streamBufStartAddr = opt->picStreamBufferAddr;
    info->streamBufSize      = opt->picStreamBufferSize;
    info->streamBufEndAddr   = opt->picStreamBufferAddr + opt->picStreamBufferSize;

    gen_enc_pic_reg(*info, *opt, reg);

    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_BS_START, reg->bs_start);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_BS_SIZE, reg->bs_size);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_BS_OPTION, reg->bs_option);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_USE_SEC_AXI, reg->use_sec_axi);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_REPORT_PARAM, reg->report_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_MV_HISTO_CLASS0, reg->mv_histo_class0);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_MV_HISTO_CLASS1, reg->mv_histo_class1);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_CUSTOM_MAP_OPTION_PARAM, reg->custom_map_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_CUSTOM_MAP_OPTION_ADDR, reg->custom_map_addr);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SRC_PIC_IDX, reg->src_pic_idx);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SRC_ADDR_Y, reg->src_addr_y);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SRC_ADDR_U, reg->src_addr_u);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SRC_ADDR_V, reg->src_addr_v);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SRC_STRIDE, reg->src_stride);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SRC_FORMAT, reg->src_format);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SRC_AXI_SEL, reg->src_axi_sel);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_CODE_OPTION, reg->code_option);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_PIC_PARAM, reg->pic_param);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_LONGTERM_PIC, reg->longterm_pic);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_PREFIX_SEI_NAL_ADDR, reg->prefix_sei_nal_addr);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_PREFIX_SEI_INFO, reg->prefix_sei_info);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SUFFIX_SEI_NAL_ADDR, reg->suffix_sei_nal_addr);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SUFFIX_SEI_INFO, reg->suffix_sei_info);

    vfree(reg);
    W6SendCommand(core_idx, inst, W6_ENC_PIC);
    if (attr->supportCommandQueue) {
        if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        regVal = VpuReadReg(core_idx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((regVal>>inst->instIndex)&0x1);
        regVal = VpuReadReg(core_idx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty  = ((regVal>>inst->instIndex)&0x1);

        if (VpuReadReg(core_idx, W6_RET_SUCCESS) == FALSE) {
            Uint32 reason = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
            if (reason != WAVE5_SYSERR_QUEUEING_FAIL)
                VLOG(ERR, "FAIL_REASON = 0x%x\n", reason);

            if (reason == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
                return RETCODE_MEMORY_ACCESS_VIOLATION;
            else if (reason == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            else if (reason == WAVE5_SYSERR_QUEUEING_FAIL)
                return RETCODE_QUEUEING_FAILURE;
            else if (reason == WAVE5_SYSERR_BUS_ERROR || reason == WAVE5_SYSERR_DOUBLE_FAULT)
                return RETCODE_VPU_BUS_ERROR;
            else
                return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuEncGetResult(CodecInst* inst, EncOutputInfo* ret)
{
    Uint32 val;
    Uint32 core_idx = inst->coreIdx;
    EncInfo* info = &inst->CodecInfo->encInfo;
    VpuAttr* attr = &g_VpuCoreAttributes[inst->coreIdx];
    RetCode retCode = RETCODE_SUCCESS;
    vpu_instance_pool_t* instancePool = NULL;

    if (attr->supportCommandQueue) {
        retCode = SendQuery(core_idx, inst, W6_QUERY_GET_RESULT);
        if (retCode != RETCODE_SUCCESS)
            return retCode;

        val = VpuReadReg(core_idx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((val>>inst->instIndex)&0x1);
        val = VpuReadReg(core_idx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty  = ((val>>inst->instIndex)&0x1);

        if (VpuReadReg(core_idx, W6_RET_ENC_ENCODING_SUCCESS) == 0) {
            ret->errorReason = VpuReadReg(core_idx, W6_RET_ENC_ERR_INFO);
            retCode = RETCODE_FAILURE;
        } else {
            ret->warnInfo = VpuReadReg(core_idx, W6_RET_ENC_WARN_INFO);
        }
    }
    else {
        val = VpuReadReg(core_idx, W6_RET_SUCCESS);
        if (val == FALSE) {
            ret->errorReason = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
            retCode = RETCODE_FAILURE;
            return retCode;
        }
    }

    ret->encPicCnt       = VpuReadReg(core_idx, W6_RET_ENC_PIC_NUM);
    val= VpuReadReg(core_idx, W6_RET_ENC_PIC_TYPE);
    ret->picType         = val & 0xFFFF;

    ret->encVclNut       = VpuReadReg(core_idx, W6_RET_ENC_VCL_NUT);
    ret->reconFrameIndex = VpuReadReg(core_idx, W6_RET_ENC_PIC_IDX);

    if (ret->reconFrameIndex >= 0)
        ret->reconFrame  = info->frameBufPool[ret->reconFrameIndex];

    ret->nonRefPicFlag   = VpuReadReg(core_idx, W6_RET_ENC_PIC_NON_REF_PIC_FLAG);

    ret->numOfSlices     = VpuReadReg(core_idx, W6_RET_ENC_PIC_SLICE_NUM);
    ret->picSkipped      = VpuReadReg(core_idx, W6_RET_ENC_PIC_SKIP);
    ret->numOfIntra      = VpuReadReg(core_idx, W6_RET_ENC_PIC_NUM_INTRA);
    ret->numOfMerge      = VpuReadReg(core_idx, W6_RET_ENC_PIC_NUM_MERGE);
    ret->numOfSkipBlock  = VpuReadReg(core_idx, W6_RET_ENC_PIC_NUM_SKIP);
    ret->bitstreamWrapAround = 0;    // only support line-buffer mode.

    ret->avgCtuQp        = VpuReadReg(core_idx, W6_RET_ENC_PIC_AVG_CTU_QP);
    ret->encPicByte      = VpuReadReg(core_idx, W6_RET_ENC_PIC_BYTE);

    ret->encGopPicIdx    = VpuReadReg(core_idx, W6_RET_ENC_GOP_PIC_IDX);
    ret->encPicPoc       = VpuReadReg(core_idx, W6_RET_ENC_PIC_POC);
    ret->encSrcIdx       = VpuReadReg(core_idx, W6_RET_ENC_USED_SRC_IDX);

    info->streamWrPtr    = VpuReadReg(core_idx, info->streamWrPtrRegAddr);
    info->streamRdPtr    = VpuReadReg(core_idx, info->streamRdPtrRegAddr);

    ret->picDistortionLow    = VpuReadReg(core_idx, W6_RET_ENC_PIC_DIST_LOW);
    ret->picDistortionHigh   = VpuReadReg(core_idx, W6_RET_ENC_PIC_DIST_HIGH);

    ret->reportMvHisto.cnt0 = VpuReadReg(core_idx, W6_RET_ENC_HISTO_CNT_0);
    ret->reportMvHisto.cnt1 = VpuReadReg(core_idx, W6_RET_ENC_HISTO_CNT_1);
    ret->reportMvHisto.cnt2 = VpuReadReg(core_idx, W6_RET_ENC_HISTO_CNT_2);
    ret->reportMvHisto.cnt3 = VpuReadReg(core_idx, W6_RET_ENC_HISTO_CNT_3);
    ret->reportMvHisto.cnt4 = VpuReadReg(core_idx, W6_RET_ENC_HISTO_CNT_4);

    ret->reportFmeSum.lowerX0  = VpuReadReg(core_idx, W6_RET_ENC_SUM_ME0_X_DIR_LOWER);
    ret->reportFmeSum.higherX0 = VpuReadReg(core_idx, W6_RET_ENC_SUM_ME0_X_DIR_HIGHER);
    ret->reportFmeSum.lowerY0  = VpuReadReg(core_idx, W6_RET_ENC_SUM_ME0_Y_DIR_LOWER);
    ret->reportFmeSum.higherY0 = VpuReadReg(core_idx, W6_RET_ENC_SUM_ME0_Y_DIR_HIGHER);
    ret->reportFmeSum.lowerX1  = VpuReadReg(core_idx, W6_RET_ENC_SUM_ME1_X_DIR_LOWER);
    ret->reportFmeSum.higherX1 = VpuReadReg(core_idx, W6_RET_ENC_SUM_ME1_X_DIR_HIGHER);
    ret->reportFmeSum.lowerY1  = VpuReadReg(core_idx, W6_RET_ENC_SUM_ME1_Y_DIR_LOWER);
    ret->reportFmeSum.higherY1 = VpuReadReg(core_idx, W6_RET_ENC_SUM_ME1_Y_DIR_HIGHER);


    ret->bitstreamBuffer = VpuReadReg(core_idx, info->streamRdPtrRegAddr);
    ret->rdPtr = info->streamRdPtr;
    ret->wrPtr = info->streamWrPtr;

    if (ret->reconFrameIndex == RECON_IDX_FLAG_HEADER_ONLY) //result for header only(no vcl) encoding
        ret->bitstreamSize   = ret->encPicByte;
    else if (ret->reconFrameIndex < 0)
        ret->bitstreamSize   = 0;
    else
        ret->bitstreamSize   = ret->encPicByte;

    ret->cycle.hostCmdStartTick    = VpuReadReg(core_idx, W6_CMD_CQ_IN_TICK);
    ret->cycle.hostCmdEndTick      = VpuReadReg(core_idx, W6_CMD_RQ_OUT_TICK);
    ret->cycle.processingStartTick = VpuReadReg(core_idx, W6_CMD_HW_RUN_TICK);
    ret->cycle.processingEndTick   = VpuReadReg(core_idx, W6_CMD_HW_DONE_TICK);
    ret->cycle.vpuStartTick        = VpuReadReg(core_idx, W6_CMD_FW_RUN_TICK);
    ret->cycle.vpuEndTick          = VpuReadReg(core_idx, W6_CMD_FW_DONE_TICK);
    ret->cycle.frameCycle          = (ret->cycle.vpuEndTick - ret->cycle.hostCmdStartTick) * info->cyclePerTick;
    ret->cycle.processingCycle     = (ret->cycle.processingEndTick - ret->cycle.processingStartTick) * info->cyclePerTick;
    ret->cycle.vpuCycle            = ((ret->cycle.vpuEndTick - ret->cycle.vpuStartTick) - (ret->cycle.processingEndTick - ret->cycle.processingStartTick)) * info->cyclePerTick;
    if (attr->supportCommandQueue) {
        ret->cycle.preProcessingStartTick = VpuReadReg(core_idx, W6_CMD_HW_PRE_RUN_TICK);
        ret->cycle.preProcessingEndTick   = VpuReadReg(core_idx, W6_CMD_HW_PRE_DONE_TICK);
        ret->cycle.preVpuStartTick        = VpuReadReg(core_idx, W6_CMD_FW_PRE_RUN_TICK);
        ret->cycle.preVpuEndTick          = VpuReadReg(core_idx, W6_CMD_FW_PRE_DONE_TICK);

        instancePool = vdi_get_instance_pool(core_idx);
        if (instancePool) {
            if (info->firstCycleCheck == FALSE) {
                ret->cycle.frameCycle = (ret->cycle.vpuEndTick- ret->cycle.hostCmdStartTick) * info->cyclePerTick;
                info->firstCycleCheck = TRUE;
            }
            else {
                ret->cycle.frameCycle = (ret->cycle.vpuEndTick- instancePool->lastPerformanceCycles) * info->cyclePerTick;
            }
            instancePool->lastPerformanceCycles = ret->cycle.vpuEndTick;
        }
        ret->cycle.preProcessingCycle = (ret->cycle.preProcessingEndTick - ret->cycle.preProcessingStartTick) * info->cyclePerTick;
        ret->cycle.preVpuCycle        = ((ret->cycle.preProcessingStartTick - ret->cycle.preVpuStartTick) + (ret->cycle.preVpuEndTick - ret->cycle.preProcessingEndTick)) * info->cyclePerTick;
    }
    return retCode;
}

RetCode Wave6VpuEncGetHeader(CodecInst* inst, EncHeaderParam* param)
{
    Int32 core_idx = inst->coreIdx;
    EncInfo* info = &inst->CodecInfo->encInfo;
    VpuAttr* attr = &g_VpuCoreAttributes[inst->coreIdx];

    info->streamRdPtr        = param->buf;
    info->streamWrPtr        = param->buf;
    info->streamBufStartAddr = param->buf;
    info->streamBufSize      = param->size;
    info->streamBufEndAddr   = param->buf + param->size;

    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_BS_START, param->buf);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_BS_SIZE, param->size);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_CODE_OPTION, param->headerType);
    VpuWriteReg(core_idx, W6_CMD_ENC_PIC_SRC_PIC_IDX, 0);

    W6SendCommand(core_idx, inst, W6_ENC_PIC);
    if (attr->supportCommandQueue) {
        Uint32 regVal;

        if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        regVal = VpuReadReg(core_idx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((regVal >> inst->instIndex) & 0x1);
        regVal = VpuReadReg(core_idx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty = ((regVal >> inst->instIndex) & 0x1);

        if (VpuReadReg(core_idx, W6_RET_SUCCESS) == FALSE) {
            Uint32 reason = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
            if (reason != WAVE5_SYSERR_QUEUEING_FAIL)
                VLOG(ERR, "FAIL_REASON = 0x%x\n", reason);

            if (reason == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
                return RETCODE_MEMORY_ACCESS_VIOLATION;
            else if (reason == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            else if (reason == WAVE5_SYSERR_QUEUEING_FAIL)
                return RETCODE_QUEUEING_FAILURE;
            else if (reason == WAVE5_SYSERR_BUS_ERROR || reason == WAVE5_SYSERR_DOUBLE_FAULT)
                return RETCODE_VPU_BUS_ERROR;
            else
                return RETCODE_FAILURE;
        }
    }
    else {
        Int32 regVal;

        regVal = Wave6VpuWaitInterrupt(inst, VPU_ENC_TIMEOUT);
        if (regVal == INTERRUPT_TIMEOUT_VALUE)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        Wave6VpuClearInterrupt(core_idx, regVal);

        info->streamWrPtr = VpuReadReg(core_idx, info->streamWrPtrRegAddr);
        info->streamRdPtr = VpuReadReg(core_idx, info->streamRdPtrRegAddr);
        param->size = VpuReadReg(core_idx, W6_RET_ENC_PIC_BYTE);

        if (VpuReadReg(inst->coreIdx, W6_RET_SUCCESS) == FALSE) {
            Uint32 reason = VpuReadReg(inst->coreIdx, W6_RET_FAIL_REASON);
            if (reason == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
                return RETCODE_MEMORY_ACCESS_VIOLATION;
            else if (reason == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            else if (reason == WAVE5_SYSERR_BUS_ERROR || reason == WAVE5_SYSERR_DOUBLE_FAULT)
                return RETCODE_VPU_BUS_ERROR;
            else
                return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuEncFiniSeq(CodecInst* inst)
{
    RetCode ret = RETCODE_SUCCESS;
    Uint32 core_idx = inst->coreIdx;
    Uint32 temp = 0;

    W6SendCommand(core_idx, inst, W6_DESTROY_INSTANCE);
    if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(core_idx, W6_RET_SUCCESS) == FALSE) {
        temp = VpuReadReg(core_idx, W6_RET_FAIL_REASON);

        if (temp == WAVE5_SYSERR_QUEUEING_FAIL)
            ret = RETCODE_QUEUEING_FAILURE;
        else if (temp == WAVE5_SYSERR_VPU_STILL_RUNNING)
            ret = RETCODE_VPU_STILL_RUNNING;
        else if (temp == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            ret = RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (temp == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            ret = RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (temp == WAVE5_SYSERR_BUS_ERROR || temp == WAVE5_SYSERR_DOUBLE_FAULT)
            ret = RETCODE_VPU_BUS_ERROR;
        else
            ret = RETCODE_FAILURE;
    }
    return ret;
}

RetCode Wave6VpuEncUpdateBS(CodecInst* inst)
{
    Uint32 core_idx = inst->coreIdx;
    EncInfo* info = &inst->CodecInfo->encInfo;
    Uint32 val = 0;

    VpuWriteReg(core_idx, W6_CMD_ENC_UPDATE_BS_BS_START, info->streamRdPtr);
    VpuWriteReg(core_idx, W6_CMD_ENC_UPDATE_BS_BS_SIZE, info->streamBufSize);

    W6SendCommand(core_idx, inst, W6_UPDATE_BS);
    if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(core_idx, W6_RET_SUCCESS) == 0) {
        val = VpuReadReg(inst->coreIdx, W6_RET_FAIL_REASON);
        if (val == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (val == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (val == WAVE5_SYSERR_BUS_ERROR || val == WAVE5_SYSERR_DOUBLE_FAULT)
            return RETCODE_VPU_BUS_ERROR;
        else
            return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuEncGiveCommand(CodecInst* inst, CodecCommand cmd, void* param)
{
    RetCode ret = RETCODE_SUCCESS;
    EncInfo* info = &inst->CodecInfo->encInfo;

    if (param == NULL)
        return RETCODE_INVALID_PARAM;

    switch (cmd) {
    case ENC_GET_QUEUE_STATUS:
        {
            QueueStatusInfo* queueInfo = (QueueStatusInfo*)param;
            queueInfo->instanceQueueFull = info->instanceQueueFull;
            queueInfo->reportQueueEmpty  = info->reportQueueEmpty;
        }
        break;
    case ENC_SET_SUB_FRAME_SYNC:
        {
            EncSubFrameSyncState* sfs = (EncSubFrameSyncState*)param;
            VpuWriteReg(inst->coreIdx, W6_VPU_SUB_FRAME_SYNC_CTRL, (sfs->ipuCurFrameIndex <<2) |
                                                                   (sfs->ipuNewFrame      <<1) |
                                                                   (sfs->ipuEndOfRow      <<0));
        }
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode Wave6VpuEncGetRdWrPtr(CodecInst* inst, PhysicalAddress *rdPtr, PhysicalAddress *wrPtr)
{
    EncInfo* info = &inst->CodecInfo->encInfo;
    RetCode ret = RETCODE_SUCCESS;

    if (info->encWrPtrSel == GET_ENC_BSBUF_FULL_WRPTR) {
        ret = SendQuery(inst->coreIdx, inst, W6_QUERY_GET_BS_WR_PTR);
        if (ret != RETCODE_SUCCESS)
            return ret;

        *rdPtr = VpuReadReg(inst->coreIdx, W6_RET_ENC_GET_BS_RD_PTR);
        *wrPtr = VpuReadReg(inst->coreIdx, W6_RET_ENC_GET_BS_WR_PTR);
    } else {
        *rdPtr = info->streamRdPtr;
        *wrPtr = info->streamWrPtr;
    }

    return ret;
}

RetCode Wave6VpuEncGetAuxBufSize(CodecInst* inst, EncAuxBufferSizeInfo sizeInfo, Uint32* size)
{
    Uint32 width = 0;
    Uint32 height = 0;
    Uint32 bufSize = 0;

    if (inst->codecMode == W_AVC_ENC) {
        width  = VPU_ALIGN16(sizeInfo.width);
        height = VPU_ALIGN16(sizeInfo.height);
        if (sizeInfo.rotationAngle == 90 || sizeInfo.rotationAngle == 270) {
            width  = VPU_ALIGN16(sizeInfo.height);
            height = VPU_ALIGN16(sizeInfo.width);
        }
    } else {
        width  = VPU_ALIGN8(sizeInfo.width);
        height = VPU_ALIGN8(sizeInfo.height);
        if ((sizeInfo.rotationAngle != 0 || sizeInfo.mirrorDirection != 0) && !(sizeInfo.rotationAngle == 180 && sizeInfo.mirrorDirection == MIRDIR_HOR_VER)) {
            width  = VPU_ALIGN32(sizeInfo.width);
            height = VPU_ALIGN32(sizeInfo.height);
        }
        if (sizeInfo.rotationAngle == 90 || sizeInfo.rotationAngle == 270) {
            width  = VPU_ALIGN32(sizeInfo.height);
            height = VPU_ALIGN32(sizeInfo.width);
        }
    }

    if (sizeInfo.type == AUX_BUF_FBC_Y_TBL) {
        switch (inst->codecMode) {
        case W_HEVC_ENC: bufSize = WAVE6_FBC_LUMA_TABLE_SIZE(width, height); break;
        case W_AVC_ENC:  bufSize = WAVE6_FBC_LUMA_TABLE_SIZE(width, height); break;
        case W_AV1_ENC:  bufSize = WAVE6_FBC_LUMA_TABLE_SIZE(width, height); break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
    } else if (sizeInfo.type == AUX_BUF_FBC_C_TBL) {
        switch (inst->codecMode) {
        case W_HEVC_ENC: bufSize = WAVE6_FBC_CHROMA_TABLE_SIZE(width, height); break;
        case W_AVC_ENC:  bufSize = WAVE6_FBC_CHROMA_TABLE_SIZE(width, height);break;
        case W_AV1_ENC:  bufSize = WAVE6_FBC_CHROMA_TABLE_SIZE(width, height); break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
    } else if (sizeInfo.type == AUX_BUF_MV_COL) {
        switch (inst->codecMode) {
        case W_HEVC_ENC: bufSize = WAVE6_ENC_HEVC_MVCOL_BUF_SIZE(width, height); break;
        case W_AVC_ENC:  bufSize = WAVE6_ENC_AVC_MVCOL_BUF_SIZE(width, height); break;
        case W_AV1_ENC:  bufSize = WAVE6_ENC_AV1_MVCOL_BUF_SIZE; break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
    } else if (sizeInfo.type == AUX_BUF_DEF_CDF) {
        if (inst->codecMode == W_AV1_ENC) {
            bufSize = WAVE6_AV1_DEFAULT_CDF_BUF_SIZE;
        } else {
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
    } else if (sizeInfo.type == AUX_BUF_SUB_SAMPLE) {
        switch (inst->codecMode) {
        case W_HEVC_ENC: bufSize = WAVE6_ENC_SUBSAMPLED_SIZE(width, height); break;
        case W_AVC_ENC:  bufSize = WAVE6_ENC_SUBSAMPLED_SIZE(width, height); break;
        case W_AV1_ENC:  bufSize = WAVE6_ENC_SUBSAMPLED_SIZE(width, height); break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
    } else {
        return RETCODE_INVALID_PARAM;
    }

    *size = bufSize;

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuEncRegisterAuxBuffer(CodecInst* inst, AuxBufferInfo auxBufferInfo)
{
    EncInfo* info = &inst->CodecInfo->encInfo;
    AuxBuffer* auxBufs = auxBufferInfo.bufArray;
    EncAuxBufferSizeInfo sizeInfo;
    RetCode ret = RETCODE_SUCCESS;
    Uint32 expectedSize;
    Uint32 i = 0;

    sizeInfo.width = info->encWidth;
    sizeInfo.height = info->encHeight;
    sizeInfo.type = auxBufferInfo.type;
    sizeInfo.rotationAngle = info->rotationAngle;
    sizeInfo.mirrorDirection = info->mirrorDirection;

    ret = Wave6VpuEncGetAuxBufSize(inst, sizeInfo, &expectedSize);
    if (ret != RETCODE_SUCCESS)
        return ret;

    switch (auxBufferInfo.type) {
    case AUX_BUF_FBC_Y_TBL:
        for (i = 0; i < auxBufferInfo.num; i++) {
            if (expectedSize > auxBufs[i].size)
                return RETCODE_INSUFFICIENT_RESOURCE;

            info->vbFbcYTbl[auxBufs[i].index].phys_addr = auxBufs[i].addr;
        }
        break;
    case AUX_BUF_FBC_C_TBL:
        for (i = 0; i < auxBufferInfo.num; i++) {
            if (expectedSize > auxBufs[i].size)
                return RETCODE_INSUFFICIENT_RESOURCE;

            info->vbFbcCTbl[auxBufs[i].index].phys_addr = auxBufs[i].addr;
        }
        break;
    case AUX_BUF_MV_COL:
        for (i = 0; i < auxBufferInfo.num; i++) {
            if (expectedSize > auxBufs[i].size)
                return RETCODE_INSUFFICIENT_RESOURCE;

            info->vbMV[auxBufs[i].index].phys_addr = auxBufs[i].addr;
        }
        break;
    case AUX_BUF_DEF_CDF:
        if (expectedSize > auxBufs[0].size)
            return RETCODE_INSUFFICIENT_RESOURCE;

        info->vbDefCdf.phys_addr = auxBufs[0].addr;
        break;
    case AUX_BUF_SUB_SAMPLE:
        for (i = 0; i < auxBufferInfo.num; i++) {
            if (expectedSize > auxBufs[i].size)
                return RETCODE_INSUFFICIENT_RESOURCE;

            info->vbSubSamBuf[auxBufs[i].index].phys_addr = auxBufs[i].addr;
        }
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

    return ret;
}

static void gen_set_param_change_reg(EncWave6ChangeParam param, Int32 codec, struct enc_cmd_set_param_reg *reg)
{
    Uint32 i;

    reg->enable = param.enableOption;
    if (codec == W_HEVC_ENC) {
        reg->pps_param = ((param.crQpOffset&0x1F)        <<19) |
                         ((param.cbQpOffset&0x1F)        <<14) |
                         ((param.tcOffsetDiv2&0xF)       <<10) |
                         ((param.betaOffsetDiv2&0xF)     <<6)  |
                         ((!param.enDBK)                 <<5)  | //lint !e514
                         (param.lfCrossSliceBoundaryFlag <<2)  |
                         (param.constrainedIntraPred     <<1);
        reg->intra_param = (param.intraPeriod         <<16) |
                           (param.forcedIdrHeader     <<9)  |
                           (param.qp                  <<3)  |
                           (param.decodingRefreshType <<0);
        reg->rdo_param = (param.enCustomLambda   <<22) |
                         (param.meCenter         <<21) |
                         (param.enQpMap          <<20) |
                         (param.enModeMap        <<19) |
                         (param.enQRoundOffset   <<17) |
                         (param.intraNxN         <<8)  |
                         (0x7                    <<5)  |
                         (param.disableCoefClear <<4)  |
                         (param.enAdaptiveRound  <<3)  |
                         (param.enHvsQp          <<2);
        reg->slice_param = (param.sliceArg  <<3) |
                           (param.sliceMode <<0);
        reg->quant_param_2 = ((param.lambdaDqpInter&0x3F) <<14) |
                             ((param.lambdaDqpIntra&0x3F) <<8);
        reg->non_vcl_param = (param.hrdRbspDataSize    <<18) |
                             (param.vuiRbspDataSize    <<4)  |
                             (param.encodeHrdRbspInVPS <<2)  |
                             (param.encodeVuiRbsp      <<1)  |
                             (param.encodeAUD          <<0);
        reg->vui_rbsp_addr = param.vuiRbspDataAddr;
        reg->hrd_rbsp_addr = param.hrdRbspDataAddr;
    } else if (codec == W_AVC_ENC) {
        reg->pps_param = (param.enCABAC                  <<30) |
                         (param.transform8x8             <<29) |
                         ((param.crQpOffset&0x1F)        <<19) |
                         ((param.cbQpOffset&0x1F)        <<14) |
                         ((param.tcOffsetDiv2&0xF)       <<10) |
                         ((param.betaOffsetDiv2&0xF)     <<6)  |
                         ((!param.enDBK)                 <<5)  |  //lint !e514
                         (param.lfCrossSliceBoundaryFlag <<2)  |
                         (param.constrainedIntraPred     <<1);
        reg->intra_param = (param.forcedIdrHeader <<28) |
                           (param.idrPeriod       <<17) |
                           (param.intraPeriod     <<6)  |
                           (param.qp              <<0);
        reg->rdo_param = (param.enCustomLambda   <<22) |
                         (param.meCenter         <<21) |
                         (param.enQpMap          <<20) |
                         (param.enModeMap        <<19) |
                         (param.enQRoundOffset   <<17) |
                         (0x7                    <<5)  |
                         (param.disableCoefClear <<4)  |
                         (param.enAdaptiveRound  <<3)  |
                         (param.enHvsQp          <<2);
        reg->slice_param = (param.sliceArg  <<3) |
                           (param.sliceMode <<0);
        reg->quant_param_2 = ((param.lambdaDqpInter&0x3F) <<14) |
                             ((param.lambdaDqpIntra&0x3F) <<8);
        reg->non_vcl_param = (param.hrdRbspDataSize    <<18) |
                             (param.vuiRbspDataSize    <<4)  |
                             (param.encodeHrdRbspInVPS <<2)  |
                             (param.encodeVuiRbsp      <<1)  |
                             (param.encodeAUD          <<0);
        reg->vui_rbsp_addr = param.vuiRbspDataAddr;
        reg->hrd_rbsp_addr = param.hrdRbspDataAddr;
    } else if (codec == W_AV1_ENC) {
        reg->pps_param = (param.lfSharpness <<6) |
                         ((!param.enDBK)    <<5);           //lint !e514
        reg->intra_param = (param.intraPeriod     <<16) |
                           (param.forcedIdrHeader <<9)  |
                           (param.qp              <<3);
        reg->rdo_param = (param.enCustomLambda   <<22) |
                         (param.meCenter         <<21) |
                         (param.enQpMap          <<20) |
                         (param.enModeMap        <<19) |
                         (param.enQRoundOffset   <<17) |
                         (param.intraNxN         <<8)  |
                         (0x7                    <<5)  |
                         (param.disableCoefClear <<4)  |
                         (param.enAdaptiveRound  <<3)  |
                         (param.enHvsQp          <<2);
        reg->quant_param_1 = ((param.uDcQpDelta&0xFF) <<24) |
                             ((param.vDcQpDelta&0xFF) <<16) |
                             ((param.uAcQpDelta&0xFF) <<8)  |
                             ((param.vAcQpDelta&0xFF) <<0);
        reg->quant_param_2 = ((param.lambdaDqpInter&0x3F) <<14) |
                             ((param.lambdaDqpIntra&0x3F) <<8)  |
                             ((param.yDcQpDelta&0xFF)     <<0);
    }

    reg->gop_param = (param.tempLayerCnt            <<16) |
                     (param.tempLayer[3].enChangeQp <<11) |
                     (param.tempLayer[2].enChangeQp <<10) |
                     (param.tempLayer[1].enChangeQp <<9)  |
                     (param.tempLayer[0].enChangeQp <<8);
    reg->intra_min_max_qp = (param.maxQpI <<6) |
                            (param.minQpI <<0);
    reg->rc_target_rate = param.encBitrate;
    reg->rc_param = (param.rcUpdateSpeed      <<24) |
                    (param.rcInitialLevel     <<20) |
                    ((param.rcInitialQp&0x3f) <<14) |
                    (param.rcMode             <<13) |
                    (param.picRcMaxDqp        <<7)  |
                    (param.cuLevelRateControl <<1)  |
                    (param.enRateControl      <<0);
    reg->rc_max_bitrate = param.maxBitrate;
    reg->rc_vbv_buffer_size = param.vbvBufferSize;
    reg->inter_min_max_qp = (param.maxQpB <<18) |
                            (param.minQpB <<12) |
                            (param.maxQpP <<6)  |
                            (param.minQpP <<0);
    reg->bg_param = ((param.bgDeltaQp&0x3F) <<24) |
                    (param.bgThMeanDiff     <<10) |
                    (param.bgThDiff         <<1)  |
                    (param.enBgDetect       <<0);
    reg->qround_offset = (param.qRoundInter <<13) |
                         (param.qRoundIntra <<2);
    for (i = 0; i < MAX_NUM_CHANGEABLE_TEMPORAL_LAYER; i++) {
        reg->temporal_layer_qp[i] = (param.tempLayer[i].qpB <<12) |
                                    (param.tempLayer[i].qpP <<6)  |
                                    (param.tempLayer[i].qpI <<0);
    }
}

RetCode Wave6VpuEncParaChange(CodecInst* inst, EncWave6ChangeParam* param)
{
    struct enc_cmd_set_param_reg reg;
    Int32 core_idx = inst->coreIdx;
    EncInfo* info = &inst->CodecInfo->encInfo;
    VpuAttr* attr = &g_VpuCoreAttributes[inst->coreIdx];

    osal_memset(&reg, 0x00, sizeof(struct enc_cmd_set_param_reg));

    gen_set_param_change_reg(*param, inst->codecMode, &reg);

    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_OPTION, W6_SET_PARAM_OPT_CHANGE_PARAM);
    VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_ENABLE, reg.enable);

    if (reg.enable & W6_ENC_CHANGE_PARAM_PPS)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_PPS_PARAM, reg.pps_param);
    if (reg.enable & W6_ENC_CHANGE_PARAM_GOP_PARAM)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_GOP_PARAM, reg.gop_param);
    if (reg.enable & W6_ENC_CHANGE_PARAM_INTRA_PARAM)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_INTRA_PARAM, reg.intra_param);
    if (reg.enable & W6_ENC_CHANGE_PARAM_TEMPORAL_QP_PARAM) {
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_GOP_PARAM, reg.gop_param);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_TEMPORAL_LAYER_0_QP, reg.temporal_layer_qp[0]);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_TEMPORAL_LAYER_1_QP, reg.temporal_layer_qp[1]);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_TEMPORAL_LAYER_2_QP, reg.temporal_layer_qp[2]);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_TEMPORAL_LAYER_3_QP, reg.temporal_layer_qp[3]);
    }
    if (reg.enable & W6_ENC_CHANGE_PARAM_RDO_PARAM)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RDO_PARAM, reg.rdo_param);
    if (reg.enable & W6_ENC_CHANGE_PARAM_SLICE_PARAM)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_SLICE_PARAM, reg.slice_param);
    if (reg.enable & W6_ENC_CHANGE_PARAM_RC_TARGET_RATE)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_TARGET_RATE, reg.rc_target_rate);
    if (reg.enable & W6_ENC_CHANGE_PARAM_RC_PARAM)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_PARAM, reg.rc_param);
    if (reg.enable & W6_ENC_CHANGE_PARAM_MIN_MAX_QP) {
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_INTRA_MIN_MAX_QP, reg.intra_min_max_qp);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_INTER_MIN_MAX_QP, reg.inter_min_max_qp);
    }
    if (reg.enable & W6_ENC_CHANGE_PARAM_RC_MAX_BITRATE)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_MAX_BITRATE, reg.rc_max_bitrate);
    if (reg.enable & W6_ENC_CHANGE_PARAM_RC_VBV_BUFFER_SIZE)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_RC_VBV_BUFFER_SIZE, reg.rc_vbv_buffer_size);
    if (reg.enable & W6_ENC_CHANGE_PARAM_VUI_HRD_PARAM) {
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_NON_VCL_PARAM, reg.non_vcl_param);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_VUI_RBSP_ADDR, reg.vui_rbsp_addr);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_HRD_RBSP_ADDR, reg.hrd_rbsp_addr);
    }
    if (reg.enable & W6_ENC_CHANGE_PARAM_BG_PARAM)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_BG_PARAM, reg.bg_param);
    if (reg.enable & W6_ENC_CHANGE_PARAM_QROUND_OFFSET)
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_QROUND_OFFSET, reg.qround_offset);
    if (reg.enable & W6_ENC_CHANGE_PARAM_QUANT_PARAM) {
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_QUANT_PARAM_1, reg.quant_param_1);
        VpuWriteReg(core_idx, W6_CMD_ENC_SET_PARAM_QUANT_PARAM_2, reg.quant_param_2);
    }

    W6SendCommand(core_idx, inst, W6_ENC_SET_PARAM);
    if (attr->supportCommandQueue) {
        Uint32 regVal;

        if (vdi_wait_vpu_busy(core_idx, __VPU_BUSY_TIMEOUT, W6_VPU_CMD_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        regVal = VpuReadReg(core_idx, W6_CMD_QUEUE_FULL_IDC);
        info->instanceQueueFull = ((regVal>>inst->instIndex)&0x1);
        regVal = VpuReadReg(core_idx, W6_CMD_QUEUE_EMPTY_IDC);
        info->reportQueueEmpty = ((regVal>>inst->instIndex)&0x1);

        if (VpuReadReg(core_idx, W6_RET_SUCCESS) == 0) {
            Uint32 reason = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
            VLOG(ERR, "FAIL_REASON = 0x%x\n", reason);

            if (reason == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
                return RETCODE_MEMORY_ACCESS_VIOLATION;
            else if (reason == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            else if (reason == WAVE5_SYSERR_QUEUEING_FAIL)
                return RETCODE_QUEUEING_FAILURE;
            else if (reason == WAVE5_SYSERR_BUS_ERROR || reason == WAVE5_SYSERR_DOUBLE_FAULT)
                return RETCODE_VPU_BUS_ERROR;
            else
                return RETCODE_FAILURE;
        }
    }
    else {
        Int32 regVal;

        regVal = Wave6VpuWaitInterrupt(inst, VPU_ENC_TIMEOUT);
        if (regVal == INTERRUPT_TIMEOUT_VALUE)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        Wave6VpuClearInterrupt(core_idx, regVal);

        if (VpuReadReg(core_idx, W6_RET_SUCCESS) == 0) {
            Uint32 reason = VpuReadReg(core_idx, W6_RET_FAIL_REASON);
            if (reason == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
                return RETCODE_MEMORY_ACCESS_VIOLATION;
            else if (reason == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            else if (reason == WAVE5_SYSERR_BUS_ERROR || reason == WAVE5_SYSERR_DOUBLE_FAULT)
                return RETCODE_VPU_BUS_ERROR;
            else
                return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

static RetCode check_gop_param(CodStd std, EncWave6Param *param)
{
    Uint32 idx = 0;
    Int32 lowDelay = 0;

    if (param->gopPreset == PRESET_IDX_CUSTOM_GOP) {
        Int32 min = 0;

        lowDelay = 1;
        if (param->gopParam.customGopSize > 1) {
            min = param->gopParam.picParam[0].pocOffset;

            for (idx = 1; idx < param->gopParam.customGopSize; idx++) {
                if (min > param->gopParam.picParam[idx].pocOffset) {
                    lowDelay = 0;
                    break;
                } else {
                    min = param->gopParam.picParam[idx].pocOffset;
                }
            }
        }
    } else if ((param->gopPreset == PRESET_IDX_ALL_I)      ||
               (param->gopPreset == PRESET_IDX_IPP)        ||
               (param->gopPreset == PRESET_IDX_IBBB)       ||
               (param->gopPreset == PRESET_IDX_IPPPP)      ||
               (param->gopPreset == PRESET_IDX_IBBBB)      ||
               (param->gopPreset == PRESET_IDX_IPP_SINGLE) ||
               (param->gopPreset == PRESET_IDX_IPPPP_SINGLE)) {
        lowDelay = 1;
    }

    if (param->gopPreset > PRESET_IDX_IBBBBBBBP_SINGLE) {
        VLOG(ERR, "gopPreset: %d\n", param->gopPreset);
        return RETCODE_FAILURE;
    }
    if (param->gopPreset == PRESET_IDX_CUSTOM_GOP) {
        if (param->gopParam.customGopSize < 1 || param->gopParam.customGopSize > MAX_GOP_NUM) {
            VLOG(ERR, "customGopSize: %d\n", param->gopParam.customGopSize);
            return RETCODE_FAILURE;
        }
        for (idx = 0; idx < param->gopParam.customGopSize; idx++) {
            CustomGopPicParam picParam = param->gopParam.picParam[idx];

            if ((picParam.picType != PIC_TYPE_I) && (picParam.picType != PIC_TYPE_P) && (picParam.picType != PIC_TYPE_B)) {
                VLOG(ERR, "picParam[%d].picType: %d\n", idx, picParam.picType);
                return RETCODE_FAILURE;
            }
            if ((picParam.pocOffset < 1) || (picParam.pocOffset > param->gopParam.customGopSize)) {
                VLOG(ERR, "picParam[%d].pocOffset: %d\n", idx, picParam.pocOffset);
                return RETCODE_FAILURE;
            }
            if ((picParam.useMultiRefP < 0) || (picParam.useMultiRefP > 1)) {
                VLOG(ERR, "picParam[%d].useMultiRefP: %d\n", idx, picParam.useMultiRefP);
                return RETCODE_FAILURE;
            }
            if ((picParam.temporalId < 0) || (picParam.temporalId > 3)) {
                VLOG(ERR, "picParam[%d].temporalId: %d\n", idx, picParam.temporalId);
                return RETCODE_FAILURE;
            }
        }
        if ((std == STD_AVC) && (!lowDelay)) {
            for (idx = 0; idx < param->gopParam.customGopSize; idx++) {
                if (param->gopParam.picParam[idx].temporalId > 0) {
                    VLOG(ERR, "std: %d, picParam[%d].temporalId: %d\n", std, idx, param->gopParam.picParam[idx].temporalId);
                    return RETCODE_FAILURE;
                }
            }
        }
    }

    if (std == STD_HEVC) {
        if (param->decodingRefreshType > 2) {
            VLOG(ERR, "decodingRefreshType: %d\n", param->decodingRefreshType);
            return RETCODE_FAILURE;
        }
    } else {
        if (param->decodingRefreshType != 0) {
            VLOG(ERR, "decodingRefreshType: %d\n", param->decodingRefreshType);
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

static RetCode check_tile_slice_param(CodStd std, Uint32 picWidth, Uint32 picHeight, EncWave6Param *param)
{
    if (std == STD_AV1) {
        if (param->sliceMode != 0) {
            VLOG(ERR, "sliceMode: %d\n", param->sliceMode);
            return RETCODE_FAILURE;
        }
        if (param->sliceArg != 0) {
            VLOG(ERR, "sliceArg: %d\n", param->sliceArg);
            return RETCODE_FAILURE;
        }
        if (param->colTileNum < 1 || param->colTileNum > 2) {
            VLOG(ERR, "colTileNum: %d\n", param->colTileNum);
            return RETCODE_FAILURE;
        }
        if (param->rowTileNum < 1 || param->rowTileNum > 16) {
            VLOG(ERR, "rowTileNum: %d\n", param->rowTileNum);
            return RETCODE_FAILURE;
        }
    } else {
        if (param->sliceMode > 2) {
            VLOG(ERR, "sliceMode: %d\n", param->sliceMode);
            return RETCODE_FAILURE;
        }
        if (param->sliceMode == 1) {
            Uint32 ctuSize = (std == STD_AVC) ? 16 : 64;
            Uint32 mbNum = ((picWidth + ctuSize - 1) / ctuSize) * ((picHeight + ctuSize - 1) / ctuSize);

            if (param->sliceArg < 1 || param->sliceArg > 262143) {
                VLOG(ERR, "sliceArg: %d\n", param->sliceArg);
                return RETCODE_FAILURE;
            }
            if (param->sliceArg > mbNum) {
                VLOG(ERR, "sliceArg: %d, mbNum: %d\n", param->sliceArg, mbNum);
                return RETCODE_FAILURE;
            }
            if ((std == STD_AVC) && (param->sliceArg < 4)) {
                VLOG(ERR, "std: %d, sliceArg: %d\n", std, param->sliceArg);
                return RETCODE_FAILURE;
            }
        }
        if (param->colTileNum != 0) {
            VLOG(ERR, "colTileNum: %d\n", param->colTileNum);
            return RETCODE_FAILURE;
        }
        if (param->rowTileNum != 0) {
            VLOG(ERR, "rowTileNum: %d\n", param->rowTileNum);
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

static RetCode check_rc_param(EncWave6Param *param)
{
    if (param->frameRate < 1 || param->frameRate > 480) {
        VLOG(ERR, "frameRate: %d\n", param->frameRate);
        return RETCODE_FAILURE;
    }
    if (param->encBitrate > 1500000000) {
        VLOG(ERR, "encBitrate: %d\n", param->encBitrate);
        return RETCODE_FAILURE;
    }
    if (param->qp > 51) {
        VLOG(ERR, "qp: %d\n", param->qp);
        return RETCODE_FAILURE;
    }
    if ((param->minQpI > 51) || (param->minQpP > 51) || (param->minQpB > 51)) {
        VLOG(ERR, "minQpI: %d, minQpP: %d, minQpB: %d\n", param->minQpI, param->minQpP, param->minQpB);
        return RETCODE_FAILURE;
    }
    if ((param->maxQpI > 51) || (param->maxQpP > 51) || (param->maxQpB > 51)) {
        VLOG(ERR, "maxQpI: %d, maxQpP: %d, maxQpB: %d\n", param->maxQpI, param->maxQpP, param->maxQpB);
        return RETCODE_FAILURE;
    }
    if (param->minQpI > param->maxQpI) {
        VLOG(ERR, "minQpI: %d, maxQpI: %d\n", param->minQpI, param->maxQpI);
        return RETCODE_FAILURE;
    }
    if (param->minQpP > param->maxQpP) {
        VLOG(ERR, "minQpP: %d, maxQpP: %d\n", param->minQpP, param->maxQpP);
        return RETCODE_FAILURE;
    }
    if (param->minQpB > param->maxQpB) {
        VLOG(ERR, "minQpB: %d, maxQpB: %d\n", param->minQpB, param->maxQpB);
        return RETCODE_FAILURE;
    }
    if (param->rcInitialQp < -1 || param->rcInitialQp > 51) {
        VLOG(ERR, "rcInitialQp: %d\n", param->rcInitialQp);
        return RETCODE_FAILURE;
    }
    if (param->enRateControl != TRUE && param->enRateControl != FALSE) {
        VLOG(ERR, "enRateControl: %d\n", param->enRateControl);
        return RETCODE_FAILURE;
    }
    if (param->rcMode > 1) {
        VLOG(ERR, "rcMode: %d\n", param->rcMode);
        return RETCODE_FAILURE;
    }
    if (param->enRateControl == TRUE) {
        if (param->encBitrate <= param->frameRate) {
            VLOG(ERR, "encBitrate: %d, frameRate: %d\n", param->encBitrate, param->frameRate);
            return RETCODE_FAILURE;
        }
        if (param->rcInitialQp != -1) {
            if (param->rcInitialQp < param->minQpI) {
                VLOG(ERR, "rcInitialQp: %d, minQpI: %d\n", param->rcInitialQp, param->minQpI);
                return RETCODE_FAILURE;
            }
            if (param->rcInitialQp > param->maxQpI) {
                VLOG(ERR, "rcInitialQp: %d, maxQpI: %d\n", param->rcInitialQp, param->maxQpI);
                return RETCODE_FAILURE;
            }
        }
    } else {
        if (param->qp < param->minQpI) {
            VLOG(ERR, "qp: %d, minQpI: %d\n", param->qp, param->minQpI);
            return RETCODE_FAILURE;
        }
        if (param->qp < param->minQpP) {
            VLOG(ERR, "qp: %d, minQpP: %d\n", param->qp, param->minQpP);
            return RETCODE_FAILURE;
        }
        if (param->qp < param->minQpB) {
            VLOG(ERR, "qp: %d, minQpB: %d\n", param->qp, param->minQpB);
            return RETCODE_FAILURE;
        }
        if (param->qp > param->maxQpI) {
            VLOG(ERR, "qp: %d, maxQpI: %d\n", param->qp, param->maxQpI);
            return RETCODE_FAILURE;
        }
        if (param->qp > param->maxQpP) {
            VLOG(ERR, "qp: %d, maxQpP: %d\n", param->qp, param->maxQpP);
            return RETCODE_FAILURE;
        }
        if (param->qp > param->maxQpB) {
            VLOG(ERR, "qp: %d, maxQpB: %d\n", param->qp, param->maxQpB);
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

static RetCode check_intra_param(CodStd std, Uint32 picWidth, Uint32 picHeight, EncWave6Param *param)
{
    Uint32 ctuSize = (std == STD_AVC) ? 16 : 64;
    Uint32 numCtuCol = (picWidth + ctuSize - 1) / ctuSize;
    Uint32 numCtuRow = (picHeight + ctuSize - 1) / ctuSize;

    if (param->intraRefreshMode > 3) {
        VLOG(ERR, "intraRefreshMode: %d\n", param->intraRefreshMode);
        return RETCODE_FAILURE;
    }
    if (param->intraRefreshMode != 0) {
        if (param->intraRefreshArg < 1 || param->intraRefreshArg > 511) {
            VLOG(ERR, "intraRefreshArg: %d\n", param->intraRefreshArg);
            return RETCODE_FAILURE;
        }
    }
    if ((param->intraRefreshMode == 1) && (param->intraRefreshArg > numCtuRow)) {
        VLOG(ERR, "intraRefreshMode: %d, intraRefreshArg: %d\n", param->intraRefreshMode, param->intraRefreshArg);
        return RETCODE_FAILURE;
    }
    if ((param->intraRefreshMode == 2) && (param->intraRefreshArg > numCtuCol)) {
        VLOG(ERR, "intraRefreshMode: %d, intraRefreshArg: %d\n", param->intraRefreshMode, param->intraRefreshArg);
        return RETCODE_FAILURE;
    }
    if (std == STD_AVC) {
        if (param->intraNxN != 0) {
            VLOG(ERR, "intraNxN: %d\n", param->intraNxN);
            return RETCODE_FAILURE;
        }
    } else {
        if ((param->intraNxN > 3) || (param->intraNxN == 1)) {
            VLOG(ERR, "intraNxN: %d\n", param->intraNxN);
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

static RetCode check_custom_param(EncWave6Param *param)
{
    Uint32 idx = 0;

    if (param->enQpMap != TRUE && param->enQpMap != FALSE) {
        VLOG(ERR, "enQpMap: %d\n", param->enQpMap);
        return RETCODE_FAILURE;
    }
    if (param->enModeMap != TRUE && param->enModeMap != FALSE) {
        VLOG(ERR, "enModeMap: %d\n", param->enModeMap);
        return RETCODE_FAILURE;
    }
    if (param->enCustomLambda != TRUE && param->enCustomLambda != FALSE) {
        VLOG(ERR, "enCustomLambda: %d\n", param->enCustomLambda);
        return RETCODE_FAILURE;
    }
    for (idx = 0; idx < MAX_CUSTOM_LAMBDA_NUM; idx++) {
        if (param->customLambdaSSD[idx] > 16383) {
            VLOG(ERR, "customLambdaSSD[%d]: %d\n", idx, param->customLambdaSSD[idx]);
            return RETCODE_FAILURE;
        }
        if (param->customLambdaSAD[idx] > 127) {
            VLOG(ERR, "customLambdaSAD[%d]: %d\n", idx, param->customLambdaSAD[idx]);
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

static RetCode check_conf_win_size_param(Uint32 picWidth, Uint32 picHeight, VpuRect confWin)
{
    if (confWin.left % 2 || confWin.top % 2 || confWin.right % 2 || confWin.bottom % 2) {
        VLOG(ERR, "confWin.left: %d, confWin.top: %d, confWin.right: %d, confWin.bottom: %d\n", confWin.left, confWin.top, confWin.right, confWin.bottom);
        return RETCODE_FAILURE;
    }
    if (confWin.left > 8192 || confWin.top > 8192 || confWin.right > 8192 || confWin.bottom > 8192) {
        VLOG(ERR, "confWin.left: %d, confWin.top: %d, confWin.right: %d, confWin.bottom: %d\n", confWin.left, confWin.top, confWin.right, confWin.bottom);
        return RETCODE_FAILURE;
    }
    if ((confWin.right - confWin.left) > picWidth) {
        VLOG(ERR, "confWin.left: %d, confWin.right: %d, picWidth: %d\n", confWin.left, confWin.right, picWidth);
        return RETCODE_FAILURE;
    }
    if ((confWin.bottom - confWin.top) > picHeight) {
        VLOG(ERR, "confWin.top: %d, confWin.bottom: %d, picHeight: %d\n", confWin.top, confWin.bottom, picHeight);
        return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}

static RetCode check_temporal_layer_param(EncWave6Param *param)
{
    Uint32 idx = 0;

    if ((param->tempLayerCnt < 1) || (param->tempLayerCnt > 4)) {
        VLOG(ERR, "tempLayerCnt: %d\n", param->tempLayerCnt);
        return RETCODE_FAILURE;
    }
    if (param->tempLayerCnt > 1) {
        if (param->gopPreset == PRESET_IDX_CUSTOM_GOP || param->gopPreset == PRESET_IDX_ALL_I) {
            VLOG(ERR, "tempLayerCnt: %d, gopPreset: %d\n", param->tempLayerCnt, param->gopPreset);
            return RETCODE_FAILURE;
        }
    }
    for (idx = 0; idx < MAX_NUM_CHANGEABLE_TEMPORAL_LAYER; idx++) {
        if (param->tempLayer[idx].enChangeQp != TRUE && param->tempLayer[idx].enChangeQp != FALSE) {
            VLOG(ERR, "tempLayer[%d].enChangeQp: %d\n", idx, param->tempLayer[idx].enChangeQp);
            return RETCODE_FAILURE;
        }
        if (param->tempLayer[idx].qpB > 51) {
            VLOG(ERR, "tempLayer[%d].qpB: %d\n", idx, param->tempLayer[idx].qpB);
            return RETCODE_FAILURE;
        }
        if (param->tempLayer[idx].qpP > 51) {
            VLOG(ERR, "tempLayer[%d].qpP: %d\n", idx, param->tempLayer[idx].qpP);
            return RETCODE_FAILURE;
        }
        if (param->tempLayer[idx].qpI > 51) {
            VLOG(ERR, "tempLayer[%d].qpI: %d\n", idx, param->tempLayer[idx].qpI);
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave6VpuEncCheckOpenParam(EncOpenParam open_param)
{
    Int32 core_idx = open_param.coreIdx;
    VpuAttr *attr = &g_VpuCoreAttributes[core_idx];
    EncWave6Param *param = &open_param.EncStdParam.wave6Param;

    if ((attr->supportEncoders&(1<<open_param.bitstreamFormat)) == 0) {
        VLOG(ERR, "not support codec. bitstreamFormat: %d\n", open_param.bitstreamFormat);
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }
    if (open_param.picWidth % 8 || open_param.picHeight % 8) {
        VLOG(ERR, "picWidth: %d | picHeight: %d\n", open_param.picWidth, open_param.picHeight);
        return RETCODE_INVALID_PARAM;
    }
    if (open_param.picWidth < W_MIN_ENC_PIC_WIDTH || open_param.picWidth > W_MAX_ENC_PIC_WIDTH) {
        VLOG(ERR, "picWidth: %d\n", open_param.picWidth);
        return RETCODE_INVALID_PARAM;
    }
    if (open_param.picHeight < W_MIN_ENC_PIC_HEIGHT || open_param.picHeight > W_MAX_ENC_PIC_HEIGHT) {
        VLOG(ERR, "picHeight: %d\n", open_param.picHeight);
        return RETCODE_INVALID_PARAM;
    }

    // packed format & cbcrInterleave & nv12 can't be set at the same time.
    if (open_param.packedFormat == 1 && open_param.cbcrInterleave == 1) {
        VLOG(ERR, "packedFormat: %d, cbcrInterleave: %d\n", open_param.packedFormat, open_param.cbcrInterleave);
        return RETCODE_INVALID_PARAM;
    }
    if (open_param.packedFormat == 1 && open_param.nv21 == 1) {
        VLOG(ERR, "packedFormat: %d, nv21: %d\n", open_param.packedFormat, open_param.nv21);
        return RETCODE_INVALID_PARAM;
    }
    if ((open_param.srcFormat == FORMAT_RGB_32BIT_PACKED) || (open_param.srcFormat == FORMAT_YUV444_32BIT_PACKED) ||
        (open_param.srcFormat == FORMAT_RGB_P10_32BIT_PACKED) || (open_param.srcFormat == FORMAT_YUV444_P10_32BIT_PACKED)) {
        if (open_param.cbcrInterleave == 0) {
            VLOG(ERR, "srcFormat: %d, cbcrInterleave: %d\n", open_param.srcFormat, open_param.cbcrInterleave);
            return RETCODE_INVALID_PARAM;
        }
        if (open_param.nv21 == 1) {
            VLOG(ERR, "srcFormat: %d, nv21: %d\n", open_param.srcFormat, open_param.nv21);
            return RETCODE_INVALID_PARAM;
        }
    }
    if (open_param.subFrameSyncEnable) {
        if ((open_param.subFrameSyncMode != WIRED_BASE_SUB_FRAME_SYNC) &&
            (open_param.subFrameSyncMode != REGISTER_BASE_SUB_FRAME_SYNC)) {
            VLOG(ERR, "subFrameSyncMode: %d\n", open_param.subFrameSyncMode);
            return RETCODE_INVALID_PARAM;
        }
    }

    if (check_gop_param(open_param.bitstreamFormat, param)) {
        VLOG(ERR, "Failed check_gop_param()\n");
        return RETCODE_INVALID_PARAM;
    }
    if (check_tile_slice_param(open_param.bitstreamFormat, open_param.picWidth, open_param.picHeight, param)) {
        VLOG(ERR, "Failed check_tile_slice_param()\n");
        return RETCODE_INVALID_PARAM;
    }
    if (check_rc_param(param)) {
        VLOG(ERR, "Failed check_rc_param()\n");
        return RETCODE_INVALID_PARAM;
    }
    if (check_intra_param(open_param.bitstreamFormat, open_param.picWidth, open_param.picHeight, param)) {
        VLOG(ERR, "Failed check_intra_param()\n");
        return RETCODE_INVALID_PARAM;
    }
    if (check_custom_param(param)) {
        VLOG(ERR, "Failed check_custom_param()\n");
        return RETCODE_INVALID_PARAM;
    }
    if (check_conf_win_size_param(open_param.picWidth, open_param.picHeight, param->confWin)) {
        VLOG(ERR, "Failed check_conf_win_size_param()\n");
        return RETCODE_INVALID_PARAM;
    }
    if (check_temporal_layer_param(param)) {
        VLOG(ERR, "Failed check_temporal_layer_param()\n");
        return RETCODE_INVALID_PARAM;
    }

    if (param->internalBitDepth != 8 && param->internalBitDepth != 10) {
        VLOG(ERR, "internalBitDepth: %d\n", param->internalBitDepth);
        return RETCODE_INVALID_PARAM;
    }
    if (param->intraPeriod > 65535) {
        VLOG(ERR, "intraPeriod: %d\n", param->intraPeriod);
        return RETCODE_INVALID_PARAM;
    }
    if (param->enLongTerm != TRUE && param->enLongTerm != FALSE) {
        VLOG(ERR, "enLongTerm: %d\n", param->enLongTerm);
        return RETCODE_INVALID_PARAM;
    }
    if (param->vbvBufferSize < 10 || param->vbvBufferSize > 100000) {
        VLOG(ERR, "vbvBufferSize: %d\n", param->vbvBufferSize);
        return RETCODE_INVALID_PARAM;
    }
    if (param->cuLevelRateControl != TRUE && param->cuLevelRateControl != FALSE) {
        VLOG(ERR, "cuLevelRateControl: %d\n", param->cuLevelRateControl);
        return RETCODE_INVALID_PARAM;
    }
    if (param->enHvsQp != TRUE && param->enHvsQp != FALSE) {
        VLOG(ERR, "enHvsQp: %d\n", param->enHvsQp);
        return RETCODE_INVALID_PARAM;
    }
    if (param->enHvsQp == TRUE) {
        if (param->hvsQpScaleDiv2 < 1 || param->hvsQpScaleDiv2 > 4) {
            VLOG(ERR, "hvsQpScaleDiv2: %d\n", param->hvsQpScaleDiv2);
            return RETCODE_INVALID_PARAM;
        }
    }
    if (param->maxDeltaQp > 12) {
        VLOG(ERR, "maxDeltaQp: %d\n", param->maxDeltaQp);
        return RETCODE_INVALID_PARAM;
    }
    if (param->rcUpdateSpeed > 255) {
        VLOG(ERR, "rcUpdateSpeed: %d\n", param->rcUpdateSpeed);
        return RETCODE_INVALID_PARAM;
    }
    if (param->maxBitrate > 1500000000) {
        VLOG(ERR, "maxBitrate: %d\n", param->maxBitrate);
        return RETCODE_INVALID_PARAM;
    }
    if (param->rcInitialLevel > 15) {
        VLOG(ERR, "rcInitialLevel: %d\n", param->rcInitialLevel);
        return RETCODE_INVALID_PARAM;
    }
    if (param->picRcMaxDqp > 51) {
        VLOG(ERR, "picRcMaxDqp: %d\n", param->picRcMaxDqp);
        return RETCODE_INVALID_PARAM;
    }
    if (param->enBgDetect != TRUE && param->enBgDetect != FALSE) {
        VLOG(ERR, "enBgDetect: %d\n", param->enBgDetect);
        return RETCODE_INVALID_PARAM;
    }
    if (param->bgThDiff > 255) {
        VLOG(ERR, "bgThDiff: %d\n", param->bgThDiff);
        return RETCODE_INVALID_PARAM;
    }
    if (param->bgThMeanDiff > 255) {
        VLOG(ERR, "bgThMeanDiff: %d\n", param->bgThMeanDiff);
        return RETCODE_INVALID_PARAM;
    }
    if (param->bgDeltaQp < -16 || param->bgDeltaQp > 15) {
        VLOG(ERR, "bgDeltaQp: %d\n", param->bgDeltaQp);
        return RETCODE_INVALID_PARAM;
    }
    if (param->meCenter > 1) {
        VLOG(ERR, "meCenter: %d\n", param->meCenter);
        return RETCODE_INVALID_PARAM;
    }
    if (param->enDBK != TRUE && param->enDBK != FALSE) {
        VLOG(ERR, "enDBK: %d\n", param->enDBK);
        return RETCODE_INVALID_PARAM;
    }
    if (param->enScalingList != TRUE && param->enScalingList != FALSE) {
        VLOG(ERR, "enScalingList: %d\n", param->enScalingList);
        return RETCODE_INVALID_PARAM;
    }
    if (param->enAdaptiveRound != TRUE && param->enAdaptiveRound != FALSE) {
        VLOG(ERR, "enAdaptiveRound: %d\n", param->enAdaptiveRound);
        return RETCODE_INVALID_PARAM;
    }
    if (param->qRoundIntra > 255) {
        VLOG(ERR, "qRoundIntra: %d\n", param->qRoundIntra);
        return RETCODE_INVALID_PARAM;
    }
    if (param->qRoundInter > 255) {
        VLOG(ERR, "qRoundInter: %d\n", param->qRoundInter);
        return RETCODE_INVALID_PARAM;
    }
    if (param->disableCoefClear != TRUE && param->disableCoefClear != FALSE) {
        VLOG(ERR, "disableCoefClear: %d\n", param->disableCoefClear);
        return RETCODE_INVALID_PARAM;
    }
    if (param->lambdaDqpIntra < -32 || param->lambdaDqpIntra > 31) {
        VLOG(ERR, "lambdaDqpIntra: %d\n", param->lambdaDqpIntra);
        return RETCODE_INVALID_PARAM;
    }
    if (param->lambdaDqpInter < -32 || param->lambdaDqpInter > 31) {
        VLOG(ERR, "lambdaDqpInter: %d\n", param->lambdaDqpInter);
        return RETCODE_INVALID_PARAM;
    }
    if (param->enQRoundOffset != TRUE && param->enQRoundOffset != FALSE) {
        VLOG(ERR, "enQRoundOffset: %d\n", param->enQRoundOffset);
        return RETCODE_INVALID_PARAM;
    }
    if (param->forcedIdrHeader > 2) {
        VLOG(ERR, "forcedIdrHeader: %d\n", param->forcedIdrHeader);
        return RETCODE_INVALID_PARAM;
    }
    if (param->numUnitsInTick > 65535) {
        VLOG(ERR, "numUnitsInTick: %d\n", param->numUnitsInTick);
        return RETCODE_INVALID_PARAM;
    }
    if (param->timeScale > 65535) {
        VLOG(ERR, "timeScale: %d\n", param->timeScale);
        return RETCODE_INVALID_PARAM;
    }

    if (open_param.bitstreamFormat == STD_HEVC) {
        if (param->internalBitDepth == 10 && attr->supportHEVC10bitEnc == FALSE) {
            VLOG(ERR, "internalBitDepth: %d, supportHEVC10bitEnc: %d\n", param->internalBitDepth, attr->supportHEVC10bitEnc);
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
        if (param->idrPeriod != 0) {
            VLOG(ERR, "idrPeriod: %d\n", param->idrPeriod);
            return RETCODE_INVALID_PARAM;
        }
        if (param->strongIntraSmoothing != TRUE && param->strongIntraSmoothing != FALSE) {
            VLOG(ERR, "strongIntraSmoothing: %d\n", param->strongIntraSmoothing);
            return RETCODE_INVALID_PARAM;
        }
        if (param->constrainedIntraPred != TRUE && param->constrainedIntraPred != FALSE) {
            VLOG(ERR, "constrainedIntraPred: %d\n", param->constrainedIntraPred);
            return RETCODE_INVALID_PARAM;
        }
        if (param->intraTransSkip != TRUE && param->intraTransSkip != FALSE) {
            VLOG(ERR, "intraTransSkip: %d\n", param->intraTransSkip);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enTemporalMVP != TRUE && param->enTemporalMVP != FALSE) {
            VLOG(ERR, "enTemporalMVP: %d\n", param->enTemporalMVP);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enCABAC != FALSE) {
            VLOG(ERR, "enCABAC: %d\n", param->enCABAC);
            return RETCODE_INVALID_PARAM;
        }
        if (param->transform8x8 != FALSE) {
            VLOG(ERR, "transform8x8: %d\n", param->transform8x8);
            return RETCODE_INVALID_PARAM;
        }
        if (param->lfCrossSliceBoundaryFlag != TRUE && param->lfCrossSliceBoundaryFlag != FALSE) {
            VLOG(ERR, "lfCrossSliceBoundaryFlag: %d\n", param->lfCrossSliceBoundaryFlag);
            return RETCODE_INVALID_PARAM;
        }
        if (param->betaOffsetDiv2 < -6 || param->betaOffsetDiv2 > 6) {
            VLOG(ERR, "betaOffsetDiv2: %d\n", param->betaOffsetDiv2);
            return RETCODE_INVALID_PARAM;
        }
        if (param->tcOffsetDiv2 < -6 || param->tcOffsetDiv2 > 6) {
            VLOG(ERR, "tcOffsetDiv2: %d\n", param->tcOffsetDiv2);
            return RETCODE_INVALID_PARAM;
        }
        if (param->lfSharpness != 0) {
            VLOG(ERR, "lfSharpness: %d\n", param->lfSharpness);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enSAO != TRUE && param->enSAO != FALSE) {
            VLOG(ERR, "enSAO: %d\n", param->enSAO);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enCdef != FALSE) {
            VLOG(ERR, "enCdef: %d\n", param->enCdef);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enWiener != FALSE) {
            VLOG(ERR, "enWiener: %d\n", param->enWiener);
            return RETCODE_INVALID_PARAM;
        }
        if (param->yDcQpDelta != 0) {
            VLOG(ERR, "yDcQpDelta: %d\n", param->yDcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->uDcQpDelta != 0) {
            VLOG(ERR, "uDcQpDelta: %d\n", param->uDcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->vDcQpDelta != 0) {
            VLOG(ERR, "vDcQpDelta: %d\n", param->vDcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->uAcQpDelta != 0) {
            VLOG(ERR, "uAcQpDelta: %d\n", param->uAcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->vAcQpDelta != 0) {
            VLOG(ERR, "vAcQpDelta: %d\n", param->vAcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->cbQpOffset < -12 || param->cbQpOffset > 12) {
            VLOG(ERR, "cbQpOffset: %d\n", param->cbQpOffset);
            return RETCODE_INVALID_PARAM;
        }
        if (param->crQpOffset < -12 || param->crQpOffset > 12) {
            VLOG(ERR, "crQpOffset: %d\n", param->crQpOffset);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enStillPicture != TRUE && param->enStillPicture != FALSE) {
            VLOG(ERR, "enStillPicture: %d\n", param->enStillPicture);
            return RETCODE_INVALID_PARAM;
        }
        if (param->tier > 1) {
            VLOG(ERR, "tier: %d\n", param->tier);
            return RETCODE_INVALID_PARAM;
        }
        if (param->profile > 3) {
            VLOG(ERR, "profile: %d\n", param->profile);
            return RETCODE_INVALID_PARAM;
        }
        if (param->internalBitDepth == 10 && param->profile == HEVC_PROFILE_MAIN) {
            VLOG(ERR, "internalBitDepth: %d, profile: %d\n", param->internalBitDepth, param->profile);
            return RETCODE_INVALID_PARAM;
        }
        if (param->numTicksPocDiffOne < 1 || param->numTicksPocDiffOne > 65535) {
            VLOG(ERR, "numTicksPocDiffOne: %d\n", param->numTicksPocDiffOne);
            return RETCODE_INVALID_PARAM;
        }
    }
    else if (open_param.bitstreamFormat == STD_AVC) {
        if (param->internalBitDepth == 10 && attr->supportAVC10bitEnc == FALSE) {
            VLOG(ERR, "internalBitDepth: %d, supportAVC10bitEnc: %d\n", param->internalBitDepth, attr->supportAVC10bitEnc);
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }
        if (param->idrPeriod > 65535) {
            VLOG(ERR, "idrPeriod: %d\n", param->idrPeriod);
            return RETCODE_INVALID_PARAM;
        }
        if (param->strongIntraSmoothing != FALSE) {
            VLOG(ERR, "strongIntraSmoothing: %d\n", param->strongIntraSmoothing);
            return RETCODE_INVALID_PARAM;
        }
        if (param->constrainedIntraPred != TRUE && param->constrainedIntraPred != FALSE) {
            VLOG(ERR, "constrainedIntraPred: %d\n", param->constrainedIntraPred);
            return RETCODE_INVALID_PARAM;
        }
        if (param->intraTransSkip != FALSE) {
            VLOG(ERR, "intraTransSkip: %d\n", param->intraTransSkip);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enTemporalMVP != FALSE) {
            VLOG(ERR, "enTemporalMVP: %d\n", param->enTemporalMVP);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enCABAC != TRUE && param->enCABAC != FALSE) {
            VLOG(ERR, "enCABAC: %d\n", param->enCABAC);
            return RETCODE_INVALID_PARAM;
        }
        if (param->transform8x8 != TRUE && param->transform8x8 != FALSE) {
            VLOG(ERR, "transform8x8: %d\n", param->transform8x8);
            return RETCODE_INVALID_PARAM;
        }
        if (param->lfCrossSliceBoundaryFlag != TRUE && param->lfCrossSliceBoundaryFlag != FALSE) {
            VLOG(ERR, "lfCrossSliceBoundaryFlag: %d\n", param->lfCrossSliceBoundaryFlag);
            return RETCODE_INVALID_PARAM;
        }
        if (param->betaOffsetDiv2 < -6 || param->betaOffsetDiv2 > 6) {
            VLOG(ERR, "betaOffsetDiv2: %d\n", param->betaOffsetDiv2);
            return RETCODE_INVALID_PARAM;
        }
        if (param->tcOffsetDiv2 < -6 || param->tcOffsetDiv2 > 6) {
            VLOG(ERR, "tcOffsetDiv2: %d\n", param->tcOffsetDiv2);
            return RETCODE_INVALID_PARAM;
        }
        if (param->lfSharpness != 0) {
            VLOG(ERR, "lfSharpness: %d\n", param->lfSharpness);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enSAO != FALSE) {
            VLOG(ERR, "enSAO: %d\n", param->enSAO);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enCdef != FALSE) {
            VLOG(ERR, "enCdef: %d\n", param->enCdef);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enWiener != FALSE) {
            VLOG(ERR, "enWiener: %d\n", param->enWiener);
            return RETCODE_INVALID_PARAM;
        }
        if (param->yDcQpDelta != 0) {
            VLOG(ERR, "yDcQpDelta: %d\n", param->yDcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->uDcQpDelta != 0) {
            VLOG(ERR, "uDcQpDelta: %d\n", param->uDcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->vDcQpDelta != 0) {
            VLOG(ERR, "vDcQpDelta: %d\n", param->vDcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->uAcQpDelta != 0) {
            VLOG(ERR, "uAcQpDelta: %d\n", param->uAcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->vAcQpDelta != 0) {
            VLOG(ERR, "vAcQpDelta: %d\n", param->vAcQpDelta);
            return RETCODE_INVALID_PARAM;
        }
        if (param->cbQpOffset < -12 || param->cbQpOffset > 12) {
            VLOG(ERR, "cbQpOffset: %d\n", param->cbQpOffset);
            return RETCODE_INVALID_PARAM;
        }
        if (param->crQpOffset < -12 || param->crQpOffset > 12) {
            VLOG(ERR, "crQpOffset: %d\n", param->crQpOffset);
            return RETCODE_INVALID_PARAM;
        }
        if (param->enStillPicture != FALSE) {
            VLOG(ERR, "enStillPicture: %d\n", param->enStillPicture);
            return RETCODE_INVALID_PARAM;
        }
        if (param->tier != 0) {
            VLOG(ERR, "tier: %d\n", param->tier);
            return RETCODE_INVALID_PARAM;
        }
        if (param->profile != 0) {
            VLOG(ERR, "profile: %d\n", param->profile);
            return RETCODE_INVALID_PARAM;
        }
        if (param->numTicksPocDiffOne != 0) {
            VLOG(ERR, "numTicksPocDiffOne: %d\n", param->numTicksPocDiffOne);
            return RETCODE_INVALID_PARAM;
        }
    }

    return RETCODE_SUCCESS;
}



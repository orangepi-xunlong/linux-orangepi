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

#include "wave/wave5.h"
#include "wave/wave5_regdefine.h"
#include <linux/dma-mapping.h>
#include "vdi_debug.h"

Uint32 Wave5VpuIsInit(Uint32 coreIdx)
{
    Uint32 pc;

    pc = (Uint32)VpuReadReg(coreIdx, W5_VCPU_CUR_PC);

    return pc;
}

Int32 Wave5VpuIsBusy(Uint32 coreIdx)
{
    return VpuReadReg(coreIdx, W5_VPU_BUSY_STATUS);
}

void Wave5BitIssueCommand(CodecInst* instance, Uint32 cmd)
{
    Uint32 instanceIndex = 0;
    Uint32 codecMode     = 0;
    Uint32 coreIdx;

    if (instance == NULL) {
        return;
    }

    instanceIndex = instance->instIndex;
    codecMode     = instance->codecMode;
    coreIdx = instance->coreIdx;

    VpuWriteReg(coreIdx, W5_CMD_INSTANCE_INFO, (codecMode<<16)|(instanceIndex&0xffff));
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS,   1);
    VpuWriteReg(coreIdx, W5_COMMAND,           cmd);

    if ((instance != NULL && instance->loggingEnable))
        vdi_log(coreIdx, instanceIndex, cmd, 1);

    VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);
    return;
}

static RetCode SendQuery(CodecInst* instance, QUERY_OPT queryOpt)
{
    // Send QUERY cmd
    VpuWriteReg(instance->coreIdx, W5_QUERY_OPTION, queryOpt);
    VpuWriteReg(instance->coreIdx, W5_VPU_BUSY_STATUS, 1);
    Wave5BitIssueCommand(instance, W5_QUERY);

    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, instance->instIndex, W5_QUERY, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE)
        return RETCODE_FAILURE;

    return RETCODE_SUCCESS;

}

static RetCode SetupWave5Properties(Uint32 coreIdx)
{
    VpuAttr*    pAttr = &g_VpuCoreAttributes[coreIdx];
    Uint32      regVal;
    Uint8*      str;
    RetCode     ret = RETCODE_SUCCESS;

    VpuWriteReg(coreIdx, W5_QUERY_OPTION, GET_VPU_INFO);
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W5_COMMAND, W5_QUERY);
    VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(coreIdx, W5_RET_SUCCESS) == FALSE) {
        ret = RETCODE_QUERY_FAILURE;
    }
    else {
        regVal = VpuReadReg(coreIdx, W5_RET_PRODUCT_NAME);
        str    = (Uint8*)&regVal;
        pAttr->productName[0] = str[3];
        pAttr->productName[1] = str[2];
        pAttr->productName[2] = str[1];
        pAttr->productName[3] = str[0];
        pAttr->productName[4] = 0;

        pAttr->productVersion  = VpuReadReg(coreIdx, W5_RET_PRODUCT_VERSION);
        pAttr->fwVersion       = VpuReadReg(coreIdx, W5_RET_FW_VERSION);
        pAttr->customerId      = VpuReadReg(coreIdx, W5_RET_CUSTOMER_ID);
        pAttr->hwConfigDef0    = VpuReadReg(coreIdx, W5_RET_STD_DEF0);
        pAttr->hwConfigDef1    = VpuReadReg(coreIdx, W5_RET_STD_DEF1);
        pAttr->hwConfigFeature = VpuReadReg(coreIdx, W5_RET_CONF_FEATURE);
        pAttr->hwConfigDate    = VpuReadReg(coreIdx, W5_RET_CONF_DATE);
        pAttr->hwConfigRev     = VpuReadReg(coreIdx, W5_RET_CONF_REVISION);
        pAttr->hwConfigType    = VpuReadReg(coreIdx, W5_RET_CONF_TYPE);

        pAttr->supportHEVC10bitEnc = (pAttr->hwConfigFeature>>3)&1;
        if ( pAttr->hwConfigRev > 167455 ) {//20190321
            pAttr->supportAVC10bitEnc =  (pAttr->hwConfigFeature>>11)&1;
        } else {
            pAttr->supportAVC10bitEnc = pAttr->supportHEVC10bitEnc;
        }

        pAttr->supportDecoders = 0;
        pAttr->supportEncoders = 0;
        if (pAttr->productId == PRODUCT_ID_521) {
            pAttr->supportDecoders  = (((pAttr->hwConfigDef1>>3)&0x01) << STD_AVC);
            pAttr->supportDecoders |= (((pAttr->hwConfigDef1>>2)&0x01) << STD_HEVC);
            pAttr->supportEncoders  = (((pAttr->hwConfigDef1>>1)&0x01) << STD_AVC);
            pAttr->supportEncoders |= (((pAttr->hwConfigDef1>>0)&0x01) << STD_HEVC);
            pAttr->supportDualCore = (BOOL)((pAttr->hwConfigDef1 >> 26) & 0x01);
            if (pAttr->supportDualCore || (pAttr->hwConfigRev < 206116)) {
                pAttr->supportDecoders  = (1 << STD_AVC);
                pAttr->supportDecoders |= (1 << STD_HEVC);
                pAttr->supportEncoders  = (1 << STD_AVC);
                pAttr->supportEncoders |= (1 << STD_HEVC);
            }
            pAttr->support2AlignScaler = (BOOL)((pAttr->hwConfigDef0 >> 7) & 0x01);
        }
        if (pAttr->productId == PRODUCT_ID_511) {
            pAttr->supportDecoders  = (1 << STD_HEVC);
            pAttr->supportDecoders |= (1 << STD_AVC);
            pAttr->support2AlignScaler = (BOOL)((pAttr->hwConfigDef0>>23)&0x01);
        }
        if (pAttr->productId == PRODUCT_ID_517) {
            pAttr->supportDecoders  = (((pAttr->hwConfigDef1>>4)&0x01) << STD_AV1);
            pAttr->supportDecoders |= (((pAttr->hwConfigDef1>>3)&0x01) << STD_AVS2);
            pAttr->supportDecoders |= (((pAttr->hwConfigDef1>>2)&0x01) << STD_AVC);
            pAttr->supportDecoders |= (((pAttr->hwConfigDef1>>1)&0x01) << STD_VP9);
            pAttr->supportDecoders |= (((pAttr->hwConfigDef1>>0)&0x01) << STD_HEVC);
            pAttr->support2AlignScaler = (BOOL)((pAttr->hwConfigDef0>>7)&0x01);
        }

        pAttr->supportBackbone           = (BOOL)((pAttr->hwConfigDef0>>16)&0x01);
        pAttr->supportVcpuBackbone       = (BOOL)((pAttr->hwConfigDef0>>28)&0x01);
        pAttr->supportVcoreBackbone      = (BOOL)((pAttr->hwConfigDef0>>22)&0x01);
        pAttr->supportCommandQueue       = TRUE;
        pAttr->supportFBCBWOptimization  = (BOOL)((pAttr->hwConfigDef1>>15)&0x01);
        pAttr->supportNewTimer           = (BOOL)((pAttr->hwConfigDef1>>27)&0x01);
        pAttr->supportWTL                = TRUE;
        pAttr->supportDualCore           = (BOOL)((pAttr->hwConfigDef1>>26)&0x01);
        pAttr->supportUserQMatrix        = (BOOL)((pAttr->hwConfigDef1>>11)&0x01);
        pAttr->supportWeightedPrediction = (BOOL)((pAttr->hwConfigDef1>>10)&0x01);
        pAttr->supportRDOQ               = (BOOL)((pAttr->hwConfigDef1>>9)&0x01);
        pAttr->supportAdaptiveRounding   = (BOOL)((pAttr->hwConfigDef1>>8)&0x01);
        pAttr->supportTiled2Linear       = FALSE;
        pAttr->supportMapTypes           = FALSE;
        pAttr->supportEndianMask         = (Uint32)((1<<VDI_LITTLE_ENDIAN) | (1<<VDI_BIG_ENDIAN) | (1<<VDI_32BIT_LITTLE_ENDIAN) | (1<<VDI_32BIT_BIG_ENDIAN) | (0xffffUL<<16));
        pAttr->supportBitstreamMode      = (1<<BS_MODE_INTERRUPT) | (1<<BS_MODE_PIC_END);
        pAttr->framebufferCacheType      = FramebufCacheNone;
        pAttr->bitstreamBufferMargin     = 0;
        pAttr->maxNumVcores              = MAX_NUM_VCORE;
        pAttr->numberOfMemProtectRgns    = 10;
    }

    return ret;
}

RetCode Wave5VpuGetVersion(Uint32 coreIdx, Uint32* versionInfo, Uint32* revision)
{
    Uint32          regVal;

    VpuWriteReg(coreIdx, W5_QUERY_OPTION, GET_VPU_INFO);
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
    VpuWriteReg(coreIdx, W5_COMMAND, W5_QUERY);
    VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        VLOG(ERR, "Wave5VpuGetVersion timeout\n");
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(coreIdx, W5_RET_SUCCESS) == FALSE) {
        VLOG(ERR, "Wave5VpuGetVersion FALSE\n");
        return RETCODE_QUERY_FAILURE;
    }

    regVal = VpuReadReg(coreIdx, W5_RET_FW_VERSION);
    if (versionInfo != NULL) {
        *versionInfo = 0;
    }
    if (revision != NULL) {
        *revision    = regVal;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    vpu_buffer_t    vb;
    PhysicalAddress codeBase, tempBase;
    Uint32          codeSize, tempSize;
    Uint32          i, regVal, remapSize;
    Uint32          hwOption    = 0;
    RetCode         ret = RETCODE_SUCCESS;
    Uint32          originValue = 0;

    vdi_get_common_memory(coreIdx, &vb);

    codeBase  = vb.phys_addr;
    /* ALIGN TO 4KB */
    codeSize = (WAVE5_MAX_CODE_BUF_SIZE&~0xfff);
    if (codeSize < size*2) {
        return RETCODE_INSUFFICIENT_RESOURCE;
    }

    if (coreIdx == 0) {
        //*(volatile unsigned int *)(VE_TOP_EXT_ADDR) = codeBase>>32;
        unsigned int *reg_addr = ioremap(VE_TOP_EXT_ADDR, 4);
        originValue = readl(reg_addr);
        writel((codeBase>>32) | originValue, reg_addr);
    } else {
        vdi_fio_write_register(coreIdx, 0xFEC0, codeBase>>32);
        vdi_fio_write_register(coreIdx, 0x8EC0, codeBase>>32);
        vdi_fio_write_register(coreIdx, 0x8EC4, codeBase>>32);
    }

    tempBase = vb.phys_addr + WAVE5_TEMPBUF_OFFSET;
    tempSize = WAVE5_TEMPBUF_SIZE;

    VLOG(INFO, "\nVPU INIT Start!!!\n");
    VpuWriteMem(coreIdx, codeBase, (unsigned char*)firmware, size*2, VDI_128BIT_LITTLE_ENDIAN);
    vdi_set_bit_firmware_to_pm(coreIdx, (Uint16*)firmware);

    regVal = 0;
    VpuWriteReg(coreIdx, W5_PO_CONF, regVal);

    /* clear registers */

    for (i=W5_CMD_REG_BASE; i<W5_CMD_REG_END; i+=4)
    {
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
        if (i == W5_SW_UART_STATUS)
            continue;
#endif
        VpuWriteReg(coreIdx, i, 0x00);
    }

    /* remap page size 0*/
    remapSize = (W_REMAP_MAX_SIZE>>12) & 0x1ff;
    regVal = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (0<<16) | (W_REMAP_INDEX0<<12) | (1<<11) | remapSize;
    VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL,  regVal);
    VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR, W_REMAP_INDEX0*W_REMAP_MAX_SIZE);
    VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR, codeBase + W_REMAP_INDEX0*W_REMAP_MAX_SIZE);

    /* remap page size 1*/
    remapSize = (W_REMAP_MAX_SIZE>>12) & 0x1ff;
    regVal = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (0<<16) | (W_REMAP_INDEX1<<12) | (1<<11) | remapSize;
    VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL,  regVal);
    VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR, W_REMAP_INDEX1*W_REMAP_MAX_SIZE);
    VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR, codeBase + W_REMAP_INDEX1*W_REMAP_MAX_SIZE);

    VpuWriteReg(coreIdx, W5_ADDR_CODE_BASE,  codeBase);
    VpuWriteReg(coreIdx, W5_CODE_SIZE,       codeSize);
    VpuWriteReg(coreIdx, W5_CODE_PARAM,      (WAVE_UPPER_PROC_AXI_ID<<4) | 0);
    VpuWriteReg(coreIdx, W5_ADDR_TEMP_BASE,  tempBase);
    VpuWriteReg(coreIdx, W5_TEMP_SIZE,       tempSize);

#ifdef SUPPORT_QOS_PARAM
    vdi_fio_write_register(coreIdx, W5_BACKBONE_QOS_PROC_R_CH_PRIOR, QOS_R_PRIOR);
    vdi_fio_write_register(coreIdx, W5_BACKBONE_QOS_PROC_W_CH_PRIOR, QOS_W_PRIOR);
#endif

    VpuWriteReg(coreIdx, W5_HW_OPTION, hwOption);

    /* Interrupt */
    // encoder
    regVal  = (1<<INT_WAVE5_ENC_SET_PARAM);
    regVal |= (1<<INT_WAVE5_ENC_PIC);
    regVal |= (1<<INT_WAVE5_BSBUF_FULL);
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    regVal |= (1<<INT_WAVE5_ENC_SRC_RELEASE);
#endif
    // decoder
    regVal |= (1<<INT_WAVE5_INIT_SEQ);
    regVal |= (1<<INT_WAVE5_DEC_PIC);
    regVal |= (1<<INT_WAVE5_BSBUF_EMPTY);
    VpuWriteReg(coreIdx, W5_VPU_VINT_ENABLE, regVal);

    regVal = VpuReadReg(coreIdx, W5_VPU_RET_VPU_CONFIG0);
    if (((regVal>>16)&1) == 1) {
        regVal = ((WAVE5_PROC_AXI_ID<<28)  |
                  (WAVE5_PRP_AXI_ID<<24)   |
                  (WAVE5_FBD_Y_AXI_ID<<20) |
                  (WAVE5_FBC_Y_AXI_ID<<16) |
                  (WAVE5_FBD_C_AXI_ID<<12) |
                  (WAVE5_FBC_C_AXI_ID<<8)  |
                  (WAVE5_PRI_AXI_ID<<4)    |
                  (WAVE5_SEC_AXI_ID<<0));
        vdi_fio_write_register(coreIdx, W5_BACKBONE_PROG_AXI_ID, regVal);
    }

    if (vdi_get_sram_memory(coreIdx, &vb) < 0)  // get SRAM base/size
        return RETCODE_INSUFFICIENT_RESOURCE;

    VpuWriteReg(coreIdx, W5_ADDR_SEC_AXI,         vb.phys_addr);
    VpuWriteReg(coreIdx, W5_SEC_AXI_SIZE,         vb.size);
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS,      1);
    VpuWriteReg(coreIdx, W5_COMMAND,              W5_INIT_VPU);
    VpuWriteReg(coreIdx, W5_VPU_REMAP_CORE_START, 1);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        vdi_hw_reset(coreIdx);
        VLOG(INFO, "VPU init(W5_VPU_REMAP_CORE_START) timeout\n");
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
    if (regVal == 0) {
        Uint32 reasonCode = VpuReadReg(coreIdx, W5_RET_FAIL_REASON);
        VLOG(INFO, "VPU init(W5_RET_SUCCESS) failed(%d) REASON CODE(%08x)\n", regVal, reasonCode);
        return RETCODE_FAILURE;
    }

    ret = SetupWave5Properties(coreIdx);
    return ret;
}

RetCode Wave5VpuBuildUpDecParam(CodecInst* instance, DecOpenParam* param)
{
    RetCode      ret      = RETCODE_SUCCESS;
    DecInfo*     pDecInfo = VPU_HANDLE_TO_DECINFO(instance);
    Uint32       bsEndian = 0;
    vpu_buffer_t vb;

    pDecInfo->streamRdPtrRegAddr      = W5_RET_DEC_BS_RD_PTR;
    pDecInfo->streamWrPtrRegAddr      = W5_BS_WR_PTR;
    pDecInfo->frameDisplayFlagRegAddr = W5_RET_DEC_DISP_IDC;
    pDecInfo->currentPC               = W5_VCPU_CUR_PC;
    pDecInfo->busyFlagAddr            = W5_VPU_BUSY_STATUS;

    switch (param->bitstreamFormat) {
    case STD_HEVC:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_HEVC;
        break;
    case STD_VP9:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_VP9;
        break;
    case STD_AVS2:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_AVS2;
        break;
    case STD_AVC:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_AVC;
        break;
    case STD_AV1:
        pDecInfo->seqChangeMask = SEQ_CHANGE_ENABLE_ALL_AV1;
        break;
    default:
        VLOG(ERR, "Invalid bitstreamFormat:%d\n", param->bitstreamFormat);
        return RETCODE_NOT_SUPPORTED_FEATURE;
    }

    if (instance->productId == PRODUCT_ID_517) {
        pDecInfo->vbWork.size = (Uint32)WAVE517_WORKBUF_SIZE;
    }
    else if (instance->productId == PRODUCT_ID_521) {
        pDecInfo->vbWork.size = (Uint32)WAVE521DEC_WORKBUF_SIZE;
    }
    else if (instance->productId == PRODUCT_ID_511) {
        pDecInfo->vbWork.size = (Uint32)WAVE521DEC_WORKBUF_SIZE;
    }

    APIDPRINT("ALLOC MEM - WORK\n");
    if (vdi_allocate_dma_memory(instance->coreIdx, &pDecInfo->vbWork, DEC_WORK, instance->instIndex) < 0) {
        pDecInfo->vbWork.base = 0;
        pDecInfo->vbWork.phys_addr = 0;
        pDecInfo->vbWork.size = 0;
        pDecInfo->vbWork.virt_addr = 0;
        return RETCODE_INSUFFICIENT_RESOURCE;
    }

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_VCORE_INFO, 1);

    vdi_get_common_memory(instance->coreIdx, &vb);
    pDecInfo->vbTemp.phys_addr = vb.phys_addr + WAVE5_TEMPBUF_OFFSET;
    pDecInfo->vbTemp.size      = WAVE5_TEMPBUF_SIZE;

    vdi_get_sram_memory(instance->coreIdx, &vb);
    pDecInfo->secAxiInfo.bufBase = vb.phys_addr;
    pDecInfo->secAxiInfo.bufSize = vb.size;

    vdi_clear_memory(instance->coreIdx, pDecInfo->vbWork.phys_addr, pDecInfo->vbWork.size, 0);

    VpuWriteReg(instance->coreIdx, W5_ADDR_WORK_BASE, pDecInfo->vbWork.phys_addr);
    VpuWriteReg(instance->coreIdx, W5_WORK_SIZE,      pDecInfo->vbWork.size);

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_BS_START_ADDR, pDecInfo->streamBufStartAddr);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_BS_SIZE,       pDecInfo->streamBufSize);

    /* NOTE: When endian mode is 0, SDMA reads MSB first */
    bsEndian = vdi_convert_endian(instance->coreIdx, param->streamEndian);
    bsEndian = (~bsEndian&VDI_128BIT_ENDIAN_MASK);
    VpuWriteReg(instance->coreIdx, W5_CMD_BS_PARAM, bsEndian);

    VpuWriteReg(instance->coreIdx, W5_CMD_NUM_CQ_DEPTH_M1, (param->cmdQueueDepth-1));
    VpuWriteReg(instance->coreIdx, W5_CMD_ERR_CONCEAL, (param->errorConcealUnit<<2) | (param->errorConcealMode));


    Wave5BitIssueCommand(instance, W5_CREATE_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, instance->instIndex, W5_CREATE_INSTANCE, 2);
        vdi_free_dma_memory(instance->coreIdx, &pDecInfo->vbWork, DEC_WORK, instance->instIndex);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {           // FAILED for adding into VCPU QUEUE
        vdi_free_dma_memory(instance->coreIdx, &pDecInfo->vbWork, DEC_WORK, instance->instIndex);
        ret = RETCODE_FAILURE;
    }

    pDecInfo->productCode = g_VpuCoreAttributes[instance->coreIdx].productVersion;

    return ret;
}

RetCode Wave5VpuDecInitSeq(CodecInst* instance)
{
    RetCode     ret = RETCODE_SUCCESS;
    DecInfo*    pDecInfo;
    Uint32      cmdOption = INIT_SEQ_NORMAL, bsOption;
    Uint32      regVal;

    if (instance == NULL)
        return RETCODE_INVALID_PARAM;

    pDecInfo = VPU_HANDLE_TO_DECINFO(instance);
    if (pDecInfo->thumbnailMode)
        cmdOption = INIT_SEQ_W_THUMBNAIL;


    /* Set attributes of bitstream buffer controller */
    bsOption = 0;
    switch (pDecInfo->openParam.bitstreamMode) {
    case BS_MODE_INTERRUPT:
        if(pDecInfo->seqInitEscape == TRUE)
            bsOption = BSOPTION_ENABLE_EXPLICIT_END;
        break;
    case BS_MODE_PIC_END:
        bsOption = BSOPTION_ENABLE_EXPLICIT_END;
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

    VpuWriteReg(instance->coreIdx, W5_BS_RD_PTR, pDecInfo->streamRdPtr);
    VpuWriteReg(instance->coreIdx, W5_BS_WR_PTR, pDecInfo->streamWrPtr);

    if (pDecInfo->streamEndflag == 1) {
        bsOption = 3;
    }
    if (instance->codecMode == W_AV1_DEC) {
        bsOption |= ((pDecInfo->openParam.av1Format) << 2);
    }
    VpuWriteReg(instance->coreIdx, W5_BS_OPTION, (1UL<<31) | bsOption);

    VpuWriteReg(instance->coreIdx, W5_COMMAND_OPTION, cmdOption);


    Wave5BitIssueCommand(instance, W5_INIT_SEQ);

    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, instance->instIndex, W5_INIT_SEQ, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pDecInfo->instanceQueueCount = (regVal>>16)&0xff;
    pDecInfo->reportQueueCount   = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {           // FAILED for adding a command into VCPU QUEUE
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_QUEUEING_FAIL) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_FAIL_REASON);
            VLOG(INFO, "QUEUE_FAIL_REASON = 0x%x\n", regVal);
            ret = RETCODE_QUEUEING_FAILURE;
        }
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

static void GetDecSequenceResult(CodecInst* instance, DecInitialInfo* info)
{
    DecInfo*        pDecInfo = &instance->CodecInfo->decInfo;
    Uint32          regVal, subLayerInfo;
    Uint32          profileCompatibilityFlag;
    Uint32          left, right, top, bottom;
    Uint32          progressiveFlag, interlacedFlag, outputBitDepthMinus8;
    PhysicalAddress retRdPtr;

    if (Wave5VpuDecGetRdPtr(instance, &retRdPtr) == RETCODE_SUCCESS) {
        pDecInfo->streamRdPtr = retRdPtr;
        info->rdPtr           = retRdPtr;
    }
    else {
        info->rdPtr           = pDecInfo->streamRdPtr;
    }

    pDecInfo->frameDisplayFlag = VpuReadReg(instance->coreIdx, W5_RET_DEC_DISP_IDC);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_SIZE);
    info->picWidth            = ( (regVal >> 16) & 0xffff );
    info->picHeight           = ( regVal & 0xffff );
    info->minFrameBufferCount = VpuReadReg(instance->coreIdx, W5_RET_DEC_NUM_REQUIRED_FB);
    info->frameBufDelay       = VpuReadReg(instance->coreIdx, W5_RET_DEC_NUM_REORDER_DELAY);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_CROP_LEFT_RIGHT);
    left   = (regVal >> 16) & 0xffff;
    right  = regVal & 0xffff;
    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_CROP_TOP_BOTTOM);
    top    = (regVal >> 16) & 0xffff;
    bottom = regVal & 0xffff;

    info->picCropRect.left   = left;
    info->picCropRect.right  = info->picWidth - right;
    info->picCropRect.top    = top;
    info->picCropRect.bottom = info->picHeight - bottom;

    info->fRateNumerator   = VpuReadReg(instance->coreIdx, W5_RET_DEC_FRAME_RATE_NR);
    info->fRateDenominator = VpuReadReg(instance->coreIdx, W5_RET_DEC_FRAME_RATE_DR);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_COLOR_SAMPLE_INFO);
    info->lumaBitdepth           = (regVal>>0)&0x0f;
    info->chromaBitdepth         = (regVal>>4)&0x0f;
    info->chromaFormatIDC        = (regVal>>8)&0x0f;
    info->aspectRateInfo         = (regVal>>16)&0xff;
    info->isExtSAR               = (info->aspectRateInfo == 255 ? TRUE : FALSE);
    if (info->isExtSAR == TRUE) {
        info->aspectRateInfo     = VpuReadReg(instance->coreIdx, W5_RET_DEC_ASPECT_RATIO);  /* [0:15] - vertical size, [16:31] - horizontal size */
    }
    info->bitRate                = VpuReadReg(instance->coreIdx, W5_RET_DEC_BIT_RATE);

    subLayerInfo = VpuReadReg(instance->coreIdx, W5_RET_DEC_SUB_LAYER_INFO);
    info->maxTemporalLayers = (subLayerInfo>>8)&0x7;

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_SEQ_PARAM);
    info->level                  = regVal&0xff;
    interlacedFlag               = (regVal>>10)&0x1;
    progressiveFlag              = (regVal>>11)&0x1;
    profileCompatibilityFlag     = (regVal>>12)&0xff;
    info->profile                = (regVal>>24)&0x1f;
    info->tier                   = (regVal>>29)&0x01;
    outputBitDepthMinus8         = (regVal>>30)&0x03;

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
    else if (instance->codecMode == W_AVS2_DEC) {
        if ((info->lumaBitdepth == 10) && (outputBitDepthMinus8 == 2))
            info->outputBitDepth = 10;
        else
            info->outputBitDepth = 8;

        if (progressiveFlag == 1)
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

    info->vlcBufSize       = VpuReadReg(instance->coreIdx, W5_RET_VLC_BUF_SIZE);
    info->paramBufSize     = VpuReadReg(instance->coreIdx, W5_RET_PARAM_BUF_SIZE);
    pDecInfo->vlcBufSize   = info->vlcBufSize;
    pDecInfo->paramBufSize = info->paramBufSize;
    return;
}

RetCode Wave5VpuDecGetSeqInfo(CodecInst* instance, DecInitialInfo* info)
{
    RetCode     ret = RETCODE_SUCCESS;
    Uint32      regVal;
    DecInfo*    pDecInfo;

    pDecInfo = VPU_HANDLE_TO_DECINFO(instance);

    // Send QUERY cmd
    ret = SendQuery(instance, GET_RESULT);
    if (ret != RETCODE_SUCCESS) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(WARN, "FAIL_REASON = 0x%x\n", regVal);

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

    if (instance->loggingEnable)
        vdi_log(instance->coreIdx, instance->instIndex, W5_INIT_SEQ, 0);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pDecInfo->instanceQueueCount = (regVal>>16)&0xff;
    pDecInfo->reportQueueCount   = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODING_SUCCESS) != 1) {
#ifdef SUPPORT_SW_UART
         ret = RETCODE_FAILURE;
#else
        info->seqInitErrReason = VpuReadReg(instance->coreIdx, W5_RET_DEC_ERR_INFO);
        ret = RETCODE_FAILURE;
#endif
    }
    else {
#ifdef SUPPORT_SW_UART
        info->warnInfo = 0;
#else
        info->warnInfo = VpuReadReg(instance->coreIdx, W5_RET_DEC_WARN_INFO);
#endif
    }

    GetDecSequenceResult(instance, info);

    return ret;
}

RetCode Wave5VpuDecRegisterFramebuffer(CodecInst* inst, FrameBuffer* fbArr, TiledMapType mapType, Uint32 count)
{
    RetCode      ret = RETCODE_SUCCESS;
    DecInfo*     pDecInfo = &inst->CodecInfo->decInfo;
    Int32        q, j, i, remain, idx;
    Uint32       mvCount;
    Uint32       k;
    Int32        coreIdx, startNo, endNo;
    Uint32       regVal, cbcrInterleave, nv21, picSize;
    Uint32       endian, yuvFormat = 0;
    Uint32       addrY, addrCb, addrCr;
    Uint32       mvColSize, fbcYTblSize, fbcCTblSize;
    vpu_buffer_t vbBuffer;
    Uint32       colorFormat  = 0;
    Uint32       initPicWidth = 0, initPicHeight = 0;
    Uint32       pixelOrder=1;
    Uint32       bwbFlag = (mapType == LINEAR_FRAME_MAP) ? 1 : 0;


    coreIdx        = inst->coreIdx;
    cbcrInterleave = pDecInfo->openParam.cbcrInterleave;
    nv21           = pDecInfo->openParam.nv21;
    mvColSize      = fbcYTblSize = fbcCTblSize = 0;

    initPicWidth   = pDecInfo->initialInfo.picWidth;
    initPicHeight  = pDecInfo->initialInfo.picHeight;

    if (mapType >= COMPRESSED_FRAME_MAP) {
        cbcrInterleave = 0;
        nv21           = 0;

        switch (inst->codecMode) {
        case W_HEVC_DEC:    mvColSize = WAVE5_DEC_HEVC_MVCOL_BUF_SIZE(initPicWidth, initPicHeight); break;
        case W_VP9_DEC:     mvColSize = WAVE5_DEC_VP9_MVCOL_BUF_SIZE(initPicWidth, initPicHeight); break;
        case W_AVS2_DEC:    mvColSize = WAVE5_DEC_AVS2_MVCOL_BUF_SIZE(initPicWidth, initPicHeight); break;
        case W_AVC_DEC:     mvColSize = WAVE5_DEC_AVC_MVCOL_BUF_SIZE(initPicWidth, initPicHeight); break;
        case W_AV1_DEC:     mvColSize = WAVE5_DEC_AV1_MVCOL_BUF_SIZE(initPicWidth, initPicHeight); break;
        default:
            return RETCODE_NOT_SUPPORTED_FEATURE;
        }

        mvColSize          = VPU_ALIGN16(mvColSize);
        vbBuffer.phys_addr = 0;
        if (inst->codecMode == W_HEVC_DEC || inst->codecMode == W_AVS2_DEC || inst->codecMode == W_VP9_DEC || inst->codecMode == W_AVC_DEC || inst->codecMode == W_AV1_DEC) {
            vbBuffer.size      = ((mvColSize+4095)&~4095)+4096;   /* 4096 is a margin */
            mvCount = count;

            APIDPRINT("ALLOC MEM - MV\n");
            for (k=0  ; k<mvCount ; k++) {
                if ( pDecInfo->vbMV[k].size == 0) {
                    if (vdi_allocate_dma_memory(inst->coreIdx, &vbBuffer, DEC_MV, inst->instIndex) < 0)
                        return RETCODE_INSUFFICIENT_RESOURCE;
                    pDecInfo->vbMV[k] = vbBuffer;
                }
            }
        }

        if (pDecInfo->productCode == WAVE521C_DUAL_CODE) {
            Uint32 bgs_width      = (pDecInfo->initialInfo.lumaBitdepth >8 ? 256 : 512);
            Uint32 ot_bg_width    = 1024;
            Uint32 frm_width      = VPU_CEIL(initPicWidth, 16);
            Uint32 frm_height     = VPU_CEIL(initPicHeight, 16);
            Uint32 comp_frm_width = VPU_CEIL(VPU_CEIL(frm_width , 16) + 16, 16); // valid_width = align(width, 16), comp_frm_width = align(valid_width+pad_x, 16)
            Uint32 ot_frm_width   = VPU_CEIL(comp_frm_width, ot_bg_width);    // 1024 = offset table BG width

            // sizeof_offset_table()
            Uint32 ot_bg_height = 32;
            Uint32 bgs_height  = (1<<14) / bgs_width / (pDecInfo->initialInfo.lumaBitdepth >8 ? 2 : 1);
            Uint32 comp_frm_height = VPU_CEIL(VPU_CEIL(frm_height, 4) + 4, bgs_height);
            Uint32 ot_frm_height = VPU_CEIL(comp_frm_height, ot_bg_height);
            fbcYTblSize = (ot_frm_width/16) * (ot_frm_height/4) *2;
        }
        else {
            switch (inst->codecMode) {
            case W_HEVC_DEC:    fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(initPicWidth, initPicHeight); break;
            case W_VP9_DEC:     fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(VPU_ALIGN64(initPicWidth), VPU_ALIGN64(initPicHeight)); break;
            case W_AVS2_DEC:    fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(initPicWidth, initPicHeight); break;
            case W_AVC_DEC:     fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(initPicWidth, initPicHeight); break;
            case W_AV1_DEC:     fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(VPU_ALIGN16(initPicWidth), VPU_ALIGN8(initPicHeight)); break;
            default:
                return RETCODE_NOT_SUPPORTED_FEATURE;
            }
            fbcYTblSize        = VPU_ALIGN16(fbcYTblSize);
        }

        vbBuffer.phys_addr = 0;
        vbBuffer.size      = ((fbcYTblSize+4095)&~4095)+4096;
        APIDPRINT("ALLOC MEM - FBC Y TBL\n");
        for (k=0  ; k<count ; k++) {
            if (pDecInfo->vbFbcYTbl[k].size == 0) {
                if (vdi_allocate_dma_memory(inst->coreIdx, &vbBuffer, DEC_FBCY_TBL, inst->instIndex) < 0)
                    return RETCODE_INSUFFICIENT_RESOURCE;
                pDecInfo->vbFbcYTbl[k] = vbBuffer;
            }
        }

        if (pDecInfo->productCode == WAVE521C_DUAL_CODE) {
            Uint32 bgs_width       = (pDecInfo->initialInfo.chromaBitdepth >8 ? 256 : 512);
            Uint32 ot_bg_width    = 1024;
            Uint32 frm_width  = VPU_CEIL(initPicWidth, 16);
            Uint32 frm_height = VPU_CEIL(initPicHeight, 16);
            Uint32 comp_frm_width = VPU_CEIL(VPU_CEIL(frm_width/2 , 16) + 16, 16); // valid_width = align(width, 16), comp_frm_width = align(valid_width+pad_x, 16)
            Uint32 ot_frm_width   = VPU_CEIL(comp_frm_width, ot_bg_width);    // 1024 = offset table BG width

            // sizeof_offset_table()
            Uint32 ot_bg_height = 32;
            Uint32 bgs_height  = (1<<14) / bgs_width / (pDecInfo->initialInfo.chromaBitdepth >8 ? 2 : 1);
            Uint32 comp_frm_height = VPU_CEIL(VPU_CEIL(frm_height, 4) + 4, bgs_height);
            Uint32 ot_frm_height = VPU_CEIL(comp_frm_height, ot_bg_height);
            fbcCTblSize = (ot_frm_width/16) * (ot_frm_height/4) *2;
        }
        else {
            switch (inst->codecMode) {
            case W_HEVC_DEC:    fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(initPicWidth, initPicHeight); break;
            case W_VP9_DEC:     fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN64(initPicWidth), VPU_ALIGN64(initPicHeight)); break;
            case W_AVS2_DEC:    fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(initPicWidth, initPicHeight); break;
            case W_AVC_DEC:     fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(initPicWidth, initPicHeight); break;
            case W_AV1_DEC:     fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN16(initPicWidth), VPU_ALIGN8(initPicHeight)); break;
            default:
                return RETCODE_NOT_SUPPORTED_FEATURE;
            }
            fbcCTblSize        = VPU_ALIGN16(fbcCTblSize);
        }

        vbBuffer.phys_addr = 0;
        vbBuffer.size      = ((fbcCTblSize+4095)&~4095)+4096;
        APIDPRINT("ALLOC MEM - FBC C TBL\n");
        for (k=0  ; k<count ; k++) {
            if (pDecInfo->vbFbcCTbl[k].size == 0) {
                if (vdi_allocate_dma_memory(inst->coreIdx, &vbBuffer, DEC_FBCC_TBL, inst->instIndex) < 0)
                    return RETCODE_INSUFFICIENT_RESOURCE;
                pDecInfo->vbFbcCTbl[k] = vbBuffer;
            }
        }
        picSize = (initPicWidth<<16)|(initPicHeight);

        // Allocate TaskBuffer
        if (pDecInfo->openParam.cmdQueueDepth == 1)
            vbBuffer.size       = (Uint32)((pDecInfo->vlcBufSize * 2) + (pDecInfo->paramBufSize * pDecInfo->openParam.cmdQueueDepth));
        else
            vbBuffer.size       = (Uint32)((pDecInfo->vlcBufSize * VLC_BUF_NUM) + (pDecInfo->paramBufSize * pDecInfo->openParam.cmdQueueDepth));
        vbBuffer.phys_addr  = 0;
        if (vdi_allocate_dma_memory(inst->coreIdx, &vbBuffer, DEC_TASK, inst->instIndex) < 0)
            return RETCODE_INSUFFICIENT_RESOURCE;

        pDecInfo->vbTask = vbBuffer;

        VpuWriteReg(coreIdx, W5_CMD_SET_FB_ADDR_TASK_BUF, pDecInfo->vbTask.phys_addr);
        VpuWriteReg(coreIdx, W5_CMD_SET_FB_TASK_BUF_SIZE, vbBuffer.size);
    }
    else
    {
        picSize = (initPicWidth<<16)|(initPicHeight);
    }
    endian = vdi_convert_endian(coreIdx, fbArr[0].endian);
    VpuWriteReg(coreIdx, W5_PIC_SIZE, picSize);

    yuvFormat = 0;
    if (mapType == LINEAR_FRAME_MAP) {
        BOOL   justified = WTL_RIGHT_JUSTIFIED;
        Uint32 formatNo  = WTL_PIXEL_8BIT;
        switch (pDecInfo->wtlFormat) {
        case FORMAT_420_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_MSB:
            justified = WTL_RIGHT_JUSTIFIED;
            formatNo  = WTL_PIXEL_16BIT;
            break;
        case FORMAT_420_P10_16BIT_LSB:
        case FORMAT_422_P10_16BIT_LSB:
            justified = WTL_LEFT_JUSTIFIED;
            formatNo  = WTL_PIXEL_16BIT;
            break;
        case FORMAT_420_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_MSB:
            justified = WTL_RIGHT_JUSTIFIED;
            formatNo  = WTL_PIXEL_32BIT;
            break;
        case FORMAT_420_P10_32BIT_LSB:
        case FORMAT_422_P10_32BIT_LSB:
            justified = WTL_LEFT_JUSTIFIED;
            formatNo  = WTL_PIXEL_32BIT;
            break;
        default:
            break;
        }
        yuvFormat = justified<<2 | formatNo;

        switch (pDecInfo->wtlFormat) {
        case FORMAT_422:
        case FORMAT_422_P10_16BIT_MSB:
        case FORMAT_422_P10_16BIT_LSB:
        case FORMAT_422_P10_32BIT_MSB:
        case FORMAT_422_P10_32BIT_LSB:
            colorFormat = 1;
            break;
        default:
            colorFormat = 0;
            break;
        }
    }

    regVal =
        (bwbFlag        << 28) |
        (pixelOrder     << 23) | /* PIXEL ORDER in 128bit. first pixel in low address */
        (yuvFormat      << 20) |
        (colorFormat    << 19) |
        (nv21           << 17) |
        (cbcrInterleave << 16) |
        (fbArr[0].stride);
    VpuWriteReg(coreIdx, W5_COMMON_PIC_INFO, regVal); //// 0x008012c0

    remain = count;
    q      = (remain+7)/8;
    idx    = 0;
    for (j=0; j<q; j++) {
        regVal = (endian<<16) | (j==q-1)<<4 | ((j==0)<<3);//lint !e514
        regVal |= (pDecInfo->openParam.enableNonRefFbcWrite<<26);
        VpuWriteReg(coreIdx, W5_SFB_OPTION, regVal);
        startNo = j*8;
        endNo   = startNo + (remain>=8 ? 8 : remain) - 1;

        VpuWriteReg(coreIdx, W5_SET_FB_NUM, (startNo<<8)|endNo);

        for (i=0; i<8 && i<remain; i++) {
            if (mapType == LINEAR_FRAME_MAP && pDecInfo->openParam.cbcrOrder == CBCR_ORDER_REVERSED) {
                addrY  = fbArr[i+startNo].bufY;
                addrCb = fbArr[i+startNo].bufCr;
                addrCr = fbArr[i+startNo].bufCb;
            }
            else {
                addrY  = fbArr[i+startNo].bufY;
                addrCb = fbArr[i+startNo].bufCb;
                addrCr = fbArr[i+startNo].bufCr;
            }
            VpuWriteReg(coreIdx, W5_ADDR_LUMA_BASE0 + (i<<4), addrY);
            VpuWriteReg(coreIdx, W5_ADDR_CB_BASE0   + (i<<4), addrCb);
            APIDPRINT("REGISTER FB[%02d] Y(0x%08x), Cb(0x%08x) ", i, addrY, addrCb);
            if (mapType >= COMPRESSED_FRAME_MAP) {
                VpuWriteReg(coreIdx, W5_ADDR_FBC_Y_OFFSET0 + (i<<4), pDecInfo->vbFbcYTbl[idx].phys_addr); /* Luma FBC offset table */
                VpuWriteReg(coreIdx, W5_ADDR_FBC_C_OFFSET0 + (i<<4), pDecInfo->vbFbcCTbl[idx].phys_addr); /* Chroma FBC offset table */
                VpuWriteReg(coreIdx, W5_ADDR_MV_COL0       + (i<<2), pDecInfo->vbMV[idx].phys_addr);
                APIDPRINT("Yo(0x%08x) Co(0x%08x), Mv(0x%08x)\n",pDecInfo->vbFbcYTbl[idx].phys_addr, pDecInfo->vbFbcCTbl[idx].phys_addr, pDecInfo->vbMV[idx].phys_addr);
            }
            else {
                VpuWriteReg(coreIdx, W5_ADDR_CR_BASE0      + (i<<4), addrCr);
                VpuWriteReg(coreIdx, W5_ADDR_FBC_C_OFFSET0 + (i<<4), 0);
                VpuWriteReg(coreIdx, W5_ADDR_MV_COL0       + (i<<2), 0);
                APIDPRINT("Cr(0x%08x)\n", addrCr);
            }
            idx++;
        }
        remain -= i;

        Wave5BitIssueCommand(inst, W5_SET_FB);
        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }

    regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
    if (regVal == 0) {
        return RETCODE_FAILURE;
    }

    return ret;
}

RetCode Wave5VpuDecUpdateFramebuffer(CodecInst* inst, FrameBuffer* fbcFb, FrameBuffer* linearFb, Int32 mvIndex, Int32 picWidth, Int32 picHeight)
{
    RetCode         ret = RETCODE_SUCCESS;
    DecInfo*        pDecInfo = &inst->CodecInfo->decInfo;
    Int8            fbcIndex, linearIndex;
    Uint32          coreIdx, regVal;
    Uint32          mvColSize, fbcYTblSize, fbcCTblSize;
    Uint32          linearStride, fbcStride;
    vpu_buffer_t*   pvbMv = NULL;
    vpu_buffer_t*   pvbFbcYOffset = NULL;
    vpu_buffer_t*   pvbFbcCOffset = NULL;
    PhysicalAddress fbcYoffsetAddr = 0;
    PhysicalAddress fbcCoffsetAddr = 0;
    coreIdx     = inst->coreIdx;
    linearIndex = (linearFb == NULL) ? -1 : linearFb->myIndex - pDecInfo->numFbsForDecoding;
    fbcIndex    = (fbcFb    == NULL) ? -1 : fbcFb->myIndex;
    mvColSize   = fbcYTblSize = fbcCTblSize = 0;

    mvColSize = WAVE5_DEC_VP9_MVCOL_BUF_SIZE(picWidth, picHeight);

    if ((fbcFb != NULL) && (fbcIndex >= 0)) {
        pDecInfo->frameBufPool[fbcIndex] = *fbcFb;
    }
    if ((linearFb != NULL) && (linearIndex >= 0)) {
        pDecInfo->frameBufPool[pDecInfo->numFbsForDecoding + linearIndex] = *linearFb;
    }

    if (mvIndex >= 0) {
        pvbMv = &pDecInfo->vbMV[mvIndex];
        vdi_free_dma_memory(inst->coreIdx, pvbMv, DEC_MV, inst->instIndex);
        pvbMv->size = ((mvColSize+4095)&~4095) + 4096;
        if (vdi_allocate_dma_memory(inst->coreIdx, pvbMv, DEC_MV, inst->instIndex) < 0) {
            return RETCODE_INSUFFICIENT_RESOURCE;
        }
    }

    /* Reallocate FBC offset tables */
    //VP9 Decoded size : 64 aligned.
    fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(VPU_ALIGN64(picWidth), VPU_ALIGN64(picHeight));

    if (fbcIndex >= 0) {
        pvbFbcYOffset = &pDecInfo->vbFbcYTbl[fbcIndex];
        vdi_free_dma_memory(inst->coreIdx, pvbFbcYOffset, DEC_FBCY_TBL, inst->instIndex);
        pvbFbcYOffset->phys_addr = 0;
        pvbFbcYOffset->size      = ((fbcYTblSize+4095)&~4095)+4096;
        if (vdi_allocate_dma_memory(inst->coreIdx, pvbFbcYOffset, DEC_FBCY_TBL, inst->instIndex) < 0) {
            return RETCODE_INSUFFICIENT_RESOURCE;
        }
        fbcYoffsetAddr = pvbFbcYOffset->phys_addr;
    }

    fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(VPU_ALIGN64(picWidth), VPU_ALIGN64(picHeight));

    if (fbcIndex >= 0) {
        pvbFbcCOffset = &pDecInfo->vbFbcCTbl[fbcIndex];
        vdi_free_dma_memory(inst->coreIdx, pvbFbcCOffset, DEC_FBCC_TBL, inst->instIndex);
        pvbFbcCOffset->phys_addr = 0;
        pvbFbcCOffset->size      = ((fbcCTblSize+4095)&~4095)+4096;
        if (vdi_allocate_dma_memory(inst->coreIdx, pvbFbcCOffset, DEC_FBCC_TBL, inst->instIndex) < 0) {
            return RETCODE_INSUFFICIENT_RESOURCE;
        }
        fbcCoffsetAddr = pvbFbcCOffset->phys_addr;
    }

    linearStride = linearFb == NULL ? 0 : linearFb->stride;
    fbcStride    = fbcFb == NULL ? 0 : fbcFb->stride;
    regVal = linearStride<<16 | fbcStride;
    VpuWriteReg(coreIdx, W5_CMD_SET_FB_STRIDE, regVal);

    regVal = (picWidth<<16) | picHeight;
    VpuWriteReg(coreIdx, W5_PIC_SIZE, regVal);

    VLOG(INFO, "fbcIndex(%d), linearIndex(%d), mvIndex(%d)\n", fbcIndex, linearIndex, mvIndex);
    regVal = (mvIndex&0xff) << 16 | (linearIndex&0xff) << 8 | (fbcIndex&0xff);
    VpuWriteReg(coreIdx, W5_CMD_SET_FB_INDEX, regVal);

    VpuWriteReg(coreIdx, W5_ADDR_LUMA_BASE,     linearFb == NULL ? 0 : linearFb->bufY);
    VpuWriteReg(coreIdx, W5_ADDR_CB_BASE,       linearFb == NULL ? 0 : linearFb->bufCb);
    VpuWriteReg(coreIdx, W5_ADDR_CR_BASE,       linearFb == NULL ? 0 : linearFb->bufCr);
    VpuWriteReg(coreIdx, W5_ADDR_MV_COL,        pvbMv == NULL ? 0 : pvbMv->phys_addr);
    VpuWriteReg(coreIdx, W5_ADDR_FBC_Y_BASE,    fbcFb == NULL ? 0 : fbcFb->bufY);
    VpuWriteReg(coreIdx, W5_ADDR_FBC_C_BASE,    fbcFb == NULL ? 0 : fbcFb->bufCb);
    VpuWriteReg(coreIdx, W5_ADDR_FBC_Y_OFFSET,  fbcYoffsetAddr);
    VpuWriteReg(coreIdx, W5_ADDR_FBC_C_OFFSET,  fbcCoffsetAddr);
    VpuWriteReg(coreIdx, W5_SFB_OPTION,         1); /* UPDATE FRAMEBUFFER */

    Wave5BitIssueCommand(inst, W5_SET_FB);

    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
    if (regVal == 0) {
        return RETCODE_FAILURE;
    }

    return ret;
}

RetCode Wave5VpuDecode(CodecInst* instance, DecParam* option)
{
    DecInfo*      pDecInfo     = &instance->CodecInfo->decInfo;
    DecOpenParam* pOpenParam   = &pDecInfo->openParam;
    Uint32        modeOption   = DEC_PIC_NORMAL, bsOption, regVal;
    Int32         forceLatency = -1;

    if (pDecInfo->thumbnailMode) {
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
    if (pDecInfo->reorderEnable == FALSE) {
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

    VpuWriteReg(instance->coreIdx, W5_BS_RD_PTR, pDecInfo->streamRdPtr);
    VpuWriteReg(instance->coreIdx, W5_BS_WR_PTR, pDecInfo->streamWrPtr);
    if (pDecInfo->streamEndflag == 1) {
        bsOption = 3;   // (streamEndFlag<<1) | EXPLICIT_END
    }
    if (pOpenParam->bitstreamMode == BS_MODE_PIC_END) {
        bsOption |= (1UL<<31);
    }
    if (instance->codecMode == W_AV1_DEC) {
        bsOption |= ((pOpenParam->av1Format) << 2);
    }
    VpuWriteReg(instance->coreIdx, W5_BS_OPTION, bsOption);

    /* Secondary AXI */
    regVal = (pDecInfo->secAxiInfo.u.wave.useBitEnable<<0)    |
             (pDecInfo->secAxiInfo.u.wave.useIpEnable<<9)     |
             (pDecInfo->secAxiInfo.u.wave.useLfRowEnable<<15);
    VpuWriteReg(instance->coreIdx, W5_USE_SEC_AXI,  regVal);

    /* Set attributes of User buffer */
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_USERDATA_MASK, pDecInfo->userDataEnable);
    if (pDecInfo->userDataEnable) {
        VpuWriteReg(instance->coreIdx, W5_CMD_DEC_USERDATA_BASE, pDecInfo->userDataBufAddr);
        VpuWriteReg(instance->coreIdx, W5_CMD_DEC_USERDATA_SIZE, pDecInfo->userDataBufSize);
        VpuWriteReg(instance->coreIdx, W5_CMD_DEC_USERDATA_PARAM, VPU_USER_DATA_ENDIAN&VDI_128BIT_ENDIAN_MASK);
    }

    VpuWriteReg(instance->coreIdx, W5_COMMAND_OPTION, ((option->disableFilmGrain<<6) | (option->craAsBlaFlag<<5) | modeOption));
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_TEMPORAL_ID_PLUS1, ((pDecInfo->targetSpatialId+1)<<9) | (pDecInfo->tempIdSelectMode<<8) | (pDecInfo->targetTempId+1));
    VpuWriteReg(instance->coreIdx, W5_CMD_SEQ_CHANGE_ENABLE_FLAG, pDecInfo->seqChangeMask);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_FORCE_FB_LATENCY_PLUS1, forceLatency+1);


    Wave5BitIssueCommand(instance, W5_DEC_PIC);

    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, instance->instIndex, W5_DEC_PIC, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pDecInfo->instanceQueueCount = (regVal>>16)&0xff;
    pDecInfo->reportQueueCount   = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {           // FAILED for adding a command into VCPU QUEUE
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(INFO, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_QUEUEING_FAIL) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_FAIL_REASON);
            //VLOG(INFO, "QUEUE_FAIL_REASON = 0x%x\n", regVal);
            return RETCODE_QUEUEING_FAILURE;
        }
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

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecGetResult(CodecInst* instance, DecOutputInfo* result)
{
    RetCode              ret = RETCODE_SUCCESS;
    Uint32               regVal, subLayerInfo, index, nalUnitType;
    DecInfo*             pDecInfo;
    vpu_instance_pool_t* instancePool = NULL;

    pDecInfo = VPU_HANDLE_TO_DECINFO(instance);

    // Send QUERY cmd
    ret = SendQuery(instance, GET_RESULT);

    if (ret != RETCODE_SUCCESS) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(INFO, "FAIL_REASON = 0x%x\n", regVal);

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

    if (instance->loggingEnable)
        vdi_log(instance->coreIdx, instance->instIndex, W5_DEC_PIC, 0);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pDecInfo->instanceQueueCount = (regVal>>16)&0xff;
    pDecInfo->reportQueueCount   = (regVal & 0xffff);

    result->decodingSuccess = VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODING_SUCCESS);
#ifdef SUPPORT_SW_UART
#else
    if (result->decodingSuccess == FALSE) {
        result->errorReason = VpuReadReg(instance->coreIdx, W5_RET_DEC_ERR_INFO);
    }
    else {
        result->warnInfo = VpuReadReg(instance->coreIdx, W5_RET_DEC_WARN_INFO);
    }
#endif

    result->decOutputExtData.userDataBufAddr = VpuReadReg(instance->coreIdx, W5_RET_DEC_USERDATA_BASE);
    result->decOutputExtData.userDataSize    = VpuReadReg(instance->coreIdx, W5_RET_DEC_USERDATA_SIZE);
    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_USERDATA_IDC);
    if (regVal) {
        result->decOutputExtData.userDataHeader  = regVal;
        result->decOutputExtData.userDataBufFull = (regVal&0x2);
        result->decOutputExtData.userDataNum     = 0;
        for (index = 2; index < 32; index++) {
            if ((regVal>>index) & 0x1)
                result->decOutputExtData.userDataNum++;
        }
    }
    else {
        result->decOutputExtData.userDataHeader  = 0;
        result->decOutputExtData.userDataBufFull = 0;
        result->decOutputExtData.userDataNum     = 0;
    }

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_TYPE);

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
    else if (instance->codecMode == W_AV1_DEC) {
        switch (regVal & 0x07) {
        case 0: result->picType = PIC_TYPE_KEY;        break;
        case 1: result->picType = PIC_TYPE_INTER;      break;
        case 2: result->picType = PIC_TYPE_AV1_INTRA;  break;
        case 3: result->picType = PIC_TYPE_AV1_SWITCH; break;
        default:
            result->picType = PIC_TYPE_MAX; break;
        }
    }
    else {  // AVS2
        switch(regVal&0x07) {
        case 0: result->picType = PIC_TYPE_I;      break;
        case 1: result->picType = PIC_TYPE_P;      break;
        case 2: result->picType = PIC_TYPE_B;      break;
        case 3: result->picType = PIC_TYPE_AVS2_F; break;
        case 4: result->picType = PIC_TYPE_AVS2_S; break;
        case 5: result->picType = PIC_TYPE_AVS2_G; break;
        case 6: result->picType = PIC_TYPE_AVS2_GB;break;
        default:
             result->picType = PIC_TYPE_MAX; break;
        }
    }
    result->outputFlag                = (regVal>>31)&0x1;
    result->ctuSize                   = 16<<((regVal>>10)&0x3);
    index                             = VpuReadReg(instance->coreIdx, W5_RET_DEC_DISPLAY_INDEX);
    result->indexFrameDisplay         = index;
    result->indexFrameDisplayForTiled = index;
    index                             = VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODED_INDEX);
    result->indexFrameDecoded         = index;
    result->indexFrameDecodedForTiled = index;

    subLayerInfo = VpuReadReg(instance->coreIdx, W5_RET_DEC_SUB_LAYER_INFO);
    result->temporalId = subLayerInfo&0x7;

    if (instance->codecMode == W_HEVC_DEC) {
        result->decodedPOC = -1;
        result->displayPOC = -1;
        if (result->indexFrameDecoded >= 0 || result->indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
            result->decodedPOC = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_POC);
        }
    }
    else if (instance->codecMode == W_AVS2_DEC) {
        result->avs2Info.decodedPOI = -1;
        result->avs2Info.displayPOI = -1;
        if (result->indexFrameDecoded >= 0) {
            result->avs2Info.decodedPOI = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_POC);
        }
    }
    else if (instance->codecMode == W_AVC_DEC) {
        result->decodedPOC = -1;
        result->displayPOC = -1;
        if (result->indexFrameDecoded >= 0 || result->indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
            result->decodedPOC = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_POC);
        }
    }
    else if (instance->codecMode == W_AV1_DEC) {
        result->decodedPOC = -1;
        result->displayPOC = -1;
        if (result->indexFrameDecoded >= 0 || result->indexFrameDecoded == DECODED_IDX_FLAG_SKIP) {
            result->decodedPOC = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_POC);
        }

        result->av1Info.spatialId = (subLayerInfo>>19)&0x3;
        regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_PARAM);
        result->av1Info.allowIntraBC            = (regVal>>0)&0x1;
        result->av1Info.allowScreenContentTools = (regVal>>1)&0x1;
    }

    result->sequenceChanged = VpuReadReg(instance->coreIdx, W5_RET_DEC_NOTIFICATION);
    if (result->sequenceChanged & SEQ_CHANGE_INTER_RES_CHANGE)
        result->indexInterFrameDecoded = VpuReadReg(instance->coreIdx, W5_RET_DEC_REALLOC_INDEX);


    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_PIC_SIZE);
    result->decPicWidth  = regVal>>16;
    result->decPicHeight = regVal&0xffff;

    if (result->sequenceChanged) {
        osal_memcpy((void*)&pDecInfo->newSeqInfo, (void*)&pDecInfo->initialInfo, sizeof(DecInitialInfo));
        GetDecSequenceResult(instance, &pDecInfo->newSeqInfo);
    }

    vdi_stat_fps(instance->coreIdx, instance->instIndex, result->picType);

    result->numOfErrMBs       = VpuReadReg(instance->coreIdx, W5_RET_DEC_ERR_CTB_NUM)>>16;
    result->numOfTotMBs       = VpuReadReg(instance->coreIdx, W5_RET_DEC_ERR_CTB_NUM)&0xffff;
    result->bytePosFrameStart = VpuReadReg(instance->coreIdx, W5_RET_DEC_AU_START_POS);
    result->bytePosFrameEnd   = VpuReadReg(instance->coreIdx, W5_RET_DEC_AU_END_POS);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_DEC_RECOVERY_POINT);
    result->h265RpSei.recoveryPocCnt = regVal & 0xFFFF;            // [15:0]
    result->h265RpSei.exactMatchFlag = (regVal >> 16)&0x01;        // [16]
    result->h265RpSei.brokenLinkFlag = (regVal >> 17)&0x01;        // [17]
    result->h265RpSei.exist =  (regVal >> 18)&0x01;                // [18]
    if(result->h265RpSei.exist == 0) {
        result->h265RpSei.recoveryPocCnt = 0;
        result->h265RpSei.exactMatchFlag = 0;
        result->h265RpSei.brokenLinkFlag = 0;
    }

    result->decHostCmdTick     = VpuReadReg(instance->coreIdx, W5_RET_DEC_HOST_CMD_TICK);
    result->decSeekStartTick   = VpuReadReg(instance->coreIdx, W5_RET_DEC_SEEK_START_TICK);
    result->decSeekEndTick     = VpuReadReg(instance->coreIdx, W5_RET_DEC_SEEK_END_TICK);
    result->decParseStartTick  = VpuReadReg(instance->coreIdx, W5_RET_DEC_PARSING_START_TICK);
    result->decParseEndTick    = VpuReadReg(instance->coreIdx, W5_RET_DEC_PARSING_END_TICK);
    result->decDecodeStartTick = VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODING_START_TICK);
    result->decDecodeEndTick   = VpuReadReg(instance->coreIdx, W5_RET_DEC_DECODING_END_TICK);


    instancePool = vdi_get_instance_pool(instance->coreIdx);
    if (instancePool) {
        if (pDecInfo->firstCycleCheck == FALSE) {
            result->frameCycle = (result->decDecodeEndTick - result->decHostCmdTick)*pDecInfo->cyclePerTick;
            pDecInfo->firstCycleCheck = TRUE;
        }
        else {
            if ( result->indexFrameDecodedForTiled != -1 ) {
                result->frameCycle = (result->decDecodeEndTick - instancePool->lastPerformanceCycles)*pDecInfo->cyclePerTick;
                if (instancePool->lastPerformanceCycles < result->decHostCmdTick)
                    result->frameCycle = (result->decDecodeEndTick - result->decHostCmdTick)*pDecInfo->cyclePerTick;
            }
        }
        instancePool->lastPerformanceCycles = result->decDecodeEndTick;
    }
    result->seekCycle    = (result->decSeekEndTick   - result->decSeekStartTick)*pDecInfo->cyclePerTick;
    result->parseCycle   = (result->decParseEndTick  - result->decParseStartTick)*pDecInfo->cyclePerTick;
    result->DecodedCycle = (result->decDecodeEndTick - result->decDecodeStartTick)*pDecInfo->cyclePerTick;

    if (0 == pDecInfo->instanceQueueCount && 0 == pDecInfo->reportQueueCount) {
        // No remaining command. Reset frame cycle.
        pDecInfo->firstCycleCheck = FALSE;
    }
    vdi_stat_hwcycles(instance->coreIdx, result->DecodedCycle);
    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecFlush(CodecInst* instance, FramebufferIndex* framebufferIndexes, Uint32 size)
{
    RetCode ret = RETCODE_SUCCESS;

    Wave5BitIssueCommand(instance, W5_FLUSH_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {
        Uint32 regVal;
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(INFO, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_VPU_STILL_RUNNING)
            return RETCODE_VPU_STILL_RUNNING;
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
    else {
        DecInfo* pDecInfo = VPU_HANDLE_TO_DECINFO(instance);
        pDecInfo->instanceQueueCount = 0;
        pDecInfo->reportQueueCount   = 0;
    }

    return ret;

}

RetCode Wave5VpuReInit(Uint32 coreIdx, void* firmware, Uint32 size)
{
    vpu_buffer_t    vb;
    PhysicalAddress codeBase, tempBase;
    PhysicalAddress oldCodeBase, tempSize;
    Uint32          codeSize;
    Uint32          regVal, remapSize;
    vdi_get_common_memory(coreIdx, &vb);

    codeBase = vb.phys_addr;
    /* ALIGN TO 4KB */
    codeSize = (WAVE5_MAX_CODE_BUF_SIZE&~0xfff);
    if (codeSize < size*2) {
        return RETCODE_INSUFFICIENT_RESOURCE;
    }
    tempBase = vb.phys_addr + WAVE5_TEMPBUF_OFFSET;
    tempSize = WAVE5_TEMPBUF_SIZE;

    oldCodeBase = VpuReadReg(coreIdx, W5_VPU_REMAP_PADDR);
    oldCodeBase = VPU_MapToAddr40Bit(coreIdx, oldCodeBase);

    //if (oldCodeBase != codeBase + W_REMAP_INDEX1*W_REMAP_MAX_SIZE) {
    if (vdi_get_instance_num(coreIdx) == 0) {
        Uint32 hwOption = 0;

        VpuWriteMem(coreIdx, codeBase, (unsigned char*)firmware, size*2, VDI_128BIT_LITTLE_ENDIAN);
        vdi_set_bit_firmware_to_pm(coreIdx, (Uint16*)firmware);

        regVal = 0;
        VpuWriteReg(coreIdx, W5_PO_CONF, regVal);

        Wave5VpuReset(coreIdx, SW_RESET_ON_BOOT);

        /* remap page size 0*/
        remapSize = (W_REMAP_MAX_SIZE>>12) & 0x1ff;
        regVal = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (0<<16) | (W_REMAP_INDEX0<<12) | (1<<11) | remapSize;
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL, regVal);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR, W_REMAP_INDEX0*W_REMAP_MAX_SIZE);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR, codeBase + W_REMAP_INDEX0*W_REMAP_MAX_SIZE);

        /* remap page size 1*/
        remapSize = (W_REMAP_MAX_SIZE>>12) & 0x1ff;
        regVal = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (0<<16) | (W_REMAP_INDEX1<<12) | (1<<11) | remapSize;
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL, regVal);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR, W_REMAP_INDEX1*W_REMAP_MAX_SIZE);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR, codeBase + W_REMAP_INDEX1*W_REMAP_MAX_SIZE);

        VpuWriteReg(coreIdx, W5_ADDR_CODE_BASE,  codeBase);
        VpuWriteReg(coreIdx, W5_CODE_SIZE,       codeSize);
        VpuWriteReg(coreIdx, W5_CODE_PARAM,      (WAVE_UPPER_PROC_AXI_ID<<4) | 0);
        VpuWriteReg(coreIdx, W5_ADDR_TEMP_BASE,  tempBase);
        VpuWriteReg(coreIdx, W5_TEMP_SIZE,       tempSize);

        VpuWriteReg(coreIdx, W5_HW_OPTION, hwOption);

        /* Interrupt */
        // encoder
        regVal  = (1<<INT_WAVE5_ENC_SET_PARAM);
        regVal |= (1<<INT_WAVE5_ENC_PIC);
        regVal |= (1<<INT_WAVE5_BSBUF_FULL);
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
        regVal |= (1<<INT_WAVE5_ENC_SRC_RELEASE);
#endif
        // decoder
        regVal |= (1<<INT_WAVE5_INIT_SEQ);
        regVal |= (1<<INT_WAVE5_DEC_PIC);
        regVal |= (1<<INT_WAVE5_BSBUF_EMPTY);
        VpuWriteReg(coreIdx, W5_VPU_VINT_ENABLE, regVal);

        regVal = VpuReadReg(coreIdx, W5_VPU_RET_VPU_CONFIG0);
        if (((regVal>>16)&1) == 1) {
            regVal = ((WAVE5_PROC_AXI_ID<<28)  |
                      (WAVE5_PRP_AXI_ID<<24)   |
                      (WAVE5_FBD_Y_AXI_ID<<20) |
                      (WAVE5_FBC_Y_AXI_ID<<16) |
                      (WAVE5_FBD_C_AXI_ID<<12) |
                      (WAVE5_FBC_C_AXI_ID<<8)  |
                      (WAVE5_PRI_AXI_ID<<4)    |
                      (WAVE5_SEC_AXI_ID<<0));
            vdi_fio_write_register(coreIdx, W5_BACKBONE_PROG_AXI_ID, regVal);
        }

        if (vdi_get_sram_memory(coreIdx, &vb) < 0)  // get SRAM base/size
            return RETCODE_INSUFFICIENT_RESOURCE;

        VpuWriteReg(coreIdx, W5_ADDR_SEC_AXI,         vb.phys_addr);
        VpuWriteReg(coreIdx, W5_SEC_AXI_SIZE,         vb.size);
        VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS,      1);
        VpuWriteReg(coreIdx, W5_COMMAND,              W5_INIT_VPU);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CORE_START, 1);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            vdi_hw_reset(coreIdx);
            VLOG(INFO, "VPU reinit(W5_VPU_REMAP_CORE_START) timeout\n");
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
        if (regVal == 0) {
            Uint32 reasonCode = VpuReadReg(coreIdx, W5_RET_FAIL_REASON);
            VLOG(INFO, "VPU reinit(W5_RET_SUCCESS) failed(%d) REASON CODE(%08x)\n", regVal, reasonCode);
            return RETCODE_FAILURE;
        }
    }

    SetupWave5Properties(coreIdx);
    return RETCODE_SUCCESS;
}

RetCode Wave5VpuSleepWake(Uint32 coreIdx, int iSleepWake, const Uint16* code, Uint32 size)
{
    Uint32          regVal;
    vpu_buffer_t    vb;
    PhysicalAddress codeBase;
    Uint32          codeSize;
    Uint32          remapSize;
    if(iSleepWake==1)  //saves
    {
        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 1);
        VpuWriteReg(coreIdx, W5_COMMAND, W5_SLEEP_VPU);
        VpuWriteReg(coreIdx, W5_VPU_HOST_INT_REQ, 1);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1)
            return RETCODE_VPU_RESPONSE_TIMEOUT;

        if (VpuReadReg(coreIdx, W5_RET_SUCCESS) == FALSE) {
            Uint32 reason = VpuReadReg(coreIdx, W5_RET_FAIL_REASON);
            APIDPRINT("SLEEP_VPU failed [0x%x]", reason);

            if (reason == WAVE5_SYSERR_VPU_STILL_RUNNING)
                return RETCODE_VPU_STILL_RUNNING;
            else if (reason == WAVE5_SYSERR_BUS_ERROR || reason == WAVE5_SYSERR_DOUBLE_FAULT)
                return RETCODE_VPU_BUS_ERROR;
            else
                return RETCODE_QUERY_FAILURE;
        }
    }
    else //restore
    {
        Uint32 hwOption = 0;

        vdi_get_common_memory(coreIdx, &vb);

        codeBase = vb.phys_addr;
        /* ALIGN TO 4KB */
        codeSize = (WAVE5_MAX_CODE_BUF_SIZE&~0xfff);
        if (codeSize < size*2) {
            return RETCODE_INSUFFICIENT_RESOURCE;
        }

        regVal = 0;
        VpuWriteReg(coreIdx, W5_PO_CONF, regVal);

        /* remap page size 0*/
        remapSize = (W_REMAP_MAX_SIZE>>12) & 0x1ff;
        regVal = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (0<<16) | (W_REMAP_INDEX0<<12) | (1<<11) | remapSize;
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL,  regVal);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR, W_REMAP_INDEX0*W_REMAP_MAX_SIZE);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR, codeBase + W_REMAP_INDEX0*W_REMAP_MAX_SIZE);

        /* remap page size 1*/
        remapSize = (W_REMAP_MAX_SIZE>>12) & 0x1ff;
        regVal = 0x80000000 | (WAVE_UPPER_PROC_AXI_ID<<20) | (0<<16) | (W_REMAP_INDEX1<<12) | (1<<11) | remapSize;
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CTRL,  regVal);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_VADDR, W_REMAP_INDEX1*W_REMAP_MAX_SIZE);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_PADDR, codeBase + W_REMAP_INDEX1*W_REMAP_MAX_SIZE);

        VpuWriteReg(coreIdx, W5_ADDR_CODE_BASE,  codeBase);
        VpuWriteReg(coreIdx, W5_CODE_SIZE,       codeSize);
        VpuWriteReg(coreIdx, W5_CODE_PARAM,      (WAVE_UPPER_PROC_AXI_ID<<4) | 0);

        VpuWriteReg(coreIdx, W5_HW_OPTION, hwOption);

        /* Interrupt */
        // encoder
        regVal  = (1<<INT_WAVE5_ENC_SET_PARAM);
        regVal |= (1<<INT_WAVE5_ENC_PIC);
        regVal |= (1<<INT_WAVE5_BSBUF_FULL);
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
        regVal |= (1<<INT_WAVE5_ENC_SRC_RELEASE);
#endif
        // decoder
        regVal |= (1<<INT_WAVE5_INIT_SEQ);
        regVal |= (1<<INT_WAVE5_DEC_PIC);
        regVal |= (1<<INT_WAVE5_BSBUF_EMPTY);
        VpuWriteReg(coreIdx, W5_VPU_VINT_ENABLE, regVal);

        regVal = VpuReadReg(coreIdx, W5_VPU_RET_VPU_CONFIG0);
        if (((regVal>>16)&1) == 1) {
            regVal = ((WAVE5_PROC_AXI_ID<<28)  |
                      (WAVE5_PRP_AXI_ID<<24)   |
                      (WAVE5_FBD_Y_AXI_ID<<20) |
                      (WAVE5_FBC_Y_AXI_ID<<16) |
                      (WAVE5_FBD_C_AXI_ID<<12) |
                      (WAVE5_FBC_C_AXI_ID<<8)  |
                      (WAVE5_PRI_AXI_ID<<4)    |
                      (WAVE5_SEC_AXI_ID<<0));
            vdi_fio_write_register(coreIdx, W5_BACKBONE_PROG_AXI_ID, regVal);
        }

        VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS,      1);
        VpuWriteReg(coreIdx, W5_COMMAND,              W5_WAKEUP_VPU);
        VpuWriteReg(coreIdx, W5_VPU_REMAP_CORE_START, 1);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            VLOG(ERR, "VPU wakeup(W5_VPU_REMAP_CORE_START) timeout\n");
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
        if (regVal == 0) {
            Uint32 reasonCode = VpuReadReg(coreIdx, W5_RET_FAIL_REASON);
            VLOG(ERR, "VPU wakeup(W5_RET_SUCCESS) failed(%d) REASON CODE(%08x)\n", regVal, reasonCode);
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuReset(Uint32 coreIdx, SWResetMode resetMode)
{
    Uint32   val   = 0;
    RetCode  ret   = RETCODE_SUCCESS;
    VpuAttr* pAttr = &g_VpuCoreAttributes[coreIdx];
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    Uint32 regSwUartStatus;
#endif
    // VPU doesn't send response. Force to set BUSY flag to 0.
    VpuWriteReg(coreIdx, W5_VPU_BUSY_STATUS, 0);

    if (resetMode == SW_RESET_SAFETY) {
        if ((ret=Wave5VpuSleepWake(coreIdx, TRUE, NULL, 0)) != RETCODE_SUCCESS) {
            return ret;
        }
    }

    val = VpuReadReg(coreIdx, W5_VPU_RET_VPU_CONFIG0);
    if (((val>>16) & 0x1) == 0x01) {
        pAttr->supportBackbone = TRUE;
    }
    if (((val>>22) & 0x1) == 0x01) {
        pAttr->supportVcoreBackbone = TRUE;
    }
    if (((val>>28) & 0x1) == 0x01) {
        pAttr->supportVcpuBackbone = TRUE;
    }

    val = VpuReadReg(coreIdx, W5_VPU_RET_VPU_CONFIG1);
    if (((val>>26) & 0x1) == 0x01) {
       pAttr->supportDualCore = TRUE;
    }

    // Waiting for completion of bus transaction
    if (pAttr->supportBackbone == TRUE) {
        if (pAttr->supportDualCore == TRUE) {
            // check CORE0
            vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE0, 0x7);

            if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_BACKBONE_BUS_STATUS_VCORE0) == -1) {
                vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            }

            // check CORE1
            vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE1, 0x7);

            if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_BACKBONE_BUS_STATUS_VCORE1) == -1) {
                vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE1, 0x00);
                return RETCODE_VPU_RESPONSE_TIMEOUT;
            }

        }
        else {
            if (pAttr->supportVcoreBackbone == TRUE) {
                if (pAttr->supportVcpuBackbone == TRUE) {
                    // Step1 : disable request
                    vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCPU, 0xFF);

                    // Step2 : Waiting for completion of bus transaction
                    if (vdi_wait_vcpu_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_BACKBONE_BUS_STATUS_VCPU) == -1) {
                        vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCPU, 0x00);
                        return RETCODE_VPU_RESPONSE_TIMEOUT;
                    }
                }
                // Step1 : disable request
                vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE0, 0x7);

                // Step2 : Waiting for completion of bus transaction
                if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_BACKBONE_BUS_STATUS_VCORE0) == -1) {
                    vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
                    return RETCODE_VPU_RESPONSE_TIMEOUT;
                }
            }
            else {
                // Step1 : disable request
                vdi_fio_write_register(coreIdx, W5_COMBINED_BACKBONE_BUS_CTRL, 0x7);

                // Step2 : Waiting for completion of bus transaction
                if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_COMBINED_BACKBONE_BUS_STATUS) == -1) {
                    vdi_fio_write_register(coreIdx, W5_COMBINED_BACKBONE_BUS_CTRL, 0x00);
                    return RETCODE_VPU_RESPONSE_TIMEOUT;
                }
            }
        }
    }
    else {
        // Step1 : disable request
        vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x100);

        // Step2 : Waiting for completion of bus transaction
        if (vdi_wait_bus_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_GDI_BUS_STATUS) == -1) {
            vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x00);
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }

    switch (resetMode) {
    case SW_RESET_ON_BOOT:
    case SW_RESET_FORCE:
    case SW_RESET_SAFETY:
        val = W5_RST_BLOCK_ALL;
        break;
    default:
        return RETCODE_INVALID_PARAM;
    }

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    regSwUartStatus = VpuReadReg(coreIdx, W5_SW_UART_STATUS);
#endif
    if (val) {
        VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, val);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_RESET_STATUS) == -1) {
            VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, 0);
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
        VpuWriteReg(coreIdx, W5_VPU_RESET_REQ, 0);
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
        VpuWriteReg(coreIdx, W5_SW_UART_STATUS, regSwUartStatus); // enable SW UART.
#endif
    }
    // Step3 : must clear GDI_BUS_CTRL after done SW_RESET
    if (pAttr->supportBackbone == TRUE) {
        if (pAttr->supportDualCore == TRUE) {
            vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
            vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE1, 0x00);
        }
        else {
            if (pAttr->supportVcoreBackbone == TRUE) {
                if (pAttr->supportVcpuBackbone == TRUE) {
                    vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCPU, 0x00);
                }
                vdi_fio_write_register(coreIdx, W5_BACKBONE_BUS_CTRL_VCORE0, 0x00);
            }
            else {
                vdi_fio_write_register(coreIdx, W5_COMBINED_BACKBONE_BUS_CTRL, 0x00);
            }
        }
    }
    else {
        vdi_fio_write_register(coreIdx, W5_GDI_BUS_CTRL, 0x00);
    }
    if (resetMode == SW_RESET_SAFETY || resetMode == SW_RESET_FORCE) {
        ret = Wave5VpuSleepWake(coreIdx, FALSE, NULL, 0);
    }

    return ret;
}

RetCode Wave5VpuDecFiniSeq(CodecInst* instance)
{
    RetCode ret = RETCODE_SUCCESS;

    Wave5BitIssueCommand(instance, W5_DESTROY_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {
        Uint32 regVal;
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL && regVal != WAVE5_SYSERR_VPU_STILL_RUNNING)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_VPU_STILL_RUNNING)
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

RetCode Wave5VpuDecSetBitstreamFlag(CodecInst* instance, BOOL running, BOOL eos, BOOL explictEnd)
{
    DecInfo* pDecInfo = &instance->CodecInfo->decInfo;
    BitStreamMode bsMode = (BitStreamMode)pDecInfo->openParam.bitstreamMode;
    pDecInfo->streamEndflag = (eos == 1) ? TRUE : FALSE;

    if (bsMode == BS_MODE_INTERRUPT) {
        if (pDecInfo->streamEndflag == TRUE) explictEnd = TRUE;

        VpuWriteReg(instance->coreIdx, W5_BS_OPTION, (pDecInfo->streamEndflag<<1) | explictEnd);
        VpuWriteReg(instance->coreIdx, W5_BS_WR_PTR, pDecInfo->streamWrPtr);

        Wave5BitIssueCommand(instance, W5_UPDATE_BS);
        if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }

        if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == 0) {
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}


RetCode Wave5DecClrDispFlag(CodecInst* instance, Uint32 index)
{
    RetCode ret = RETCODE_SUCCESS;
    DecInfo * pDecInfo;
    pDecInfo   = &instance->CodecInfo->decInfo;

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_CLR_DISP_IDC, (1<<index));
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_SET_DISP_IDC, 0);
    ret = SendQuery(instance, UPDATE_DISP_FLAG);

    if (ret != RETCODE_SUCCESS) {
        VLOG(ERR, "Wave5DecClrDispFlag QUERY FAILURE\n");
        return RETCODE_QUERY_FAILURE;
    }

    pDecInfo->frameDisplayFlag = VpuReadReg(instance->coreIdx, pDecInfo->frameDisplayFlagRegAddr);

    return RETCODE_SUCCESS;
}

RetCode Wave5DecSetDispFlag(CodecInst* instance, Uint32 index)
{
    RetCode ret = RETCODE_SUCCESS;

    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_CLR_DISP_IDC, 0);
    VpuWriteReg(instance->coreIdx, W5_CMD_DEC_SET_DISP_IDC, (1<<index));
    ret = SendQuery(instance, UPDATE_DISP_FLAG);

    return ret;
}

Int32 Wave5VpuWaitInterrupt(CodecInst* instance, Int32 timeout, BOOL pending)
{
    Int32 reason = -1;

    // check an interrupt for my instance during timeout
    reason = vdi_wait_interrupt(instance->coreIdx, instance->instIndex, timeout);

    return reason;
}

RetCode Wave5VpuClearInterrupt(Uint32 coreIdx, Uint32 flags)
{
    Uint32 interruptReason;

    interruptReason = VpuReadReg(coreIdx, W5_VPU_VINT_REASON_USR);
    interruptReason &= ~flags;
    VpuWriteReg(coreIdx, W5_VPU_VINT_REASON_USR, interruptReason);

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecGetRdPtr(CodecInst* instance, PhysicalAddress *rdPtr)
{
    RetCode ret = RETCODE_SUCCESS;

    ret = SendQuery(instance, GET_BS_RD_PTR);

    if (ret != RETCODE_SUCCESS)
        return RETCODE_QUERY_FAILURE;

    *rdPtr = VpuReadReg(instance->coreIdx, W5_RET_QUERY_DEC_BS_RD_PTR);
    *rdPtr = VPU_MapToAddr40Bit(instance->coreIdx, *rdPtr);

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecSetRdPtr(CodecInst* instance, PhysicalAddress rdPtr)
{
    RetCode ret = RETCODE_SUCCESS;

    VpuWriteReg(instance->coreIdx, W5_RET_QUERY_DEC_SET_BS_RD_PTR, rdPtr);

    ret = SendQuery(instance, SET_BS_RD_PTR);

    if (ret != RETCODE_SUCCESS)
        return RETCODE_QUERY_FAILURE;

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuGetBwReport(CodecInst* instance, VPUBWData* bwMon)
{
    RetCode ret      = RETCODE_SUCCESS;
    Int32   coreIdx  = instance->coreIdx;
    Uint32  multiNum = (bwMon->bwMode == 0) ? 16 : 1;

    VpuWriteReg(coreIdx, W5_CMD_BW_OPTION, (bwMon->bwMode << 4) | (bwMon->burstLengthIdx));

    ret = SendQuery(instance, GET_BW_REPORT);
    if (ret != RETCODE_SUCCESS) {
        if (VpuReadReg(coreIdx, W5_RET_FAIL_REASON) == WAVE5_SYSERR_RESULT_NOT_READY)
            return RETCODE_REPORT_NOT_READY;
        else
            return RETCODE_QUERY_FAILURE;
    }

    bwMon->prpBwRead   = VpuReadReg(coreIdx, RET_QUERY_BW_PRP_AXI_READ)    * multiNum;
    bwMon->prpBwWrite  = VpuReadReg(coreIdx, RET_QUERY_BW_PRP_AXI_WRITE)   * multiNum;
    bwMon->fbdYRead    = VpuReadReg(coreIdx, RET_QUERY_BW_FBD_Y_AXI_READ)  * multiNum;
    bwMon->fbcYWrite   = VpuReadReg(coreIdx, RET_QUERY_BW_FBC_Y_AXI_WRITE) * multiNum;
    bwMon->fbdCRead    = VpuReadReg(coreIdx, RET_QUERY_BW_FBD_C_AXI_READ)  * multiNum;
    bwMon->fbcCWrite   = VpuReadReg(coreIdx, RET_QUERY_BW_FBC_C_AXI_WRITE) * multiNum;
    bwMon->priBwRead   = VpuReadReg(coreIdx, RET_QUERY_BW_PRI_AXI_READ)    * multiNum;
    bwMon->priBwWrite  = VpuReadReg(coreIdx, RET_QUERY_BW_PRI_AXI_WRITE)   * multiNum;
    bwMon->secBwRead   = VpuReadReg(coreIdx, RET_QUERY_BW_SEC_AXI_READ)    * multiNum;
    bwMon->secBwWrite  = VpuReadReg(coreIdx, RET_QUERY_BW_SEC_AXI_WRITE)   * multiNum;
    bwMon->procBwRead  = VpuReadReg(coreIdx, RET_QUERY_BW_PROC_AXI_READ)   * multiNum;
    bwMon->procBwWrite = VpuReadReg(coreIdx, RET_QUERY_BW_PROC_AXI_WRITE)  * multiNum;
    bwMon->bwbBwWrite  = VpuReadReg(coreIdx, RET_QUERY_BW_BWB_AXI_WRITE)   * multiNum;

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuGetDebugInfo(CodecInst* instance, VpuDebugInfo* info)
{
    RetCode ret = RETCODE_SUCCESS;
    Int32 coreIdx;
    int i;
    int idx;

    coreIdx = instance->coreIdx;

    ret = SendQuery(instance, GET_DEBUG_INFO);
    if (ret != RETCODE_SUCCESS)
        return RETCODE_QUERY_FAILURE;

    idx = 0;
    for (i = 0; i < 0x40*4; i = i + 4) {
        info->regs[idx] = VpuReadReg(coreIdx, 0x100 + i);
        idx++;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuDecGiveCommand(CodecInst* instance, CodecCommand cmd, void* param)
{
    RetCode ret = RETCODE_SUCCESS;
    DecInfo* decInfo;

    decInfo = &instance->CodecInfo->decInfo;

    if (param == NULL)
        return RETCODE_INVALID_PARAM;

    switch (cmd) {
    case DEC_GET_QUEUE_STATUS:
        {
            QueueStatusInfo* queueInfo = (QueueStatusInfo*)param;
            queueInfo->instanceQueueCount = decInfo->instanceQueueCount;
            queueInfo->reportQueueCount = decInfo->reportQueueCount;
            queueInfo->instanceQueueFull = (decInfo->instanceQueueCount == decInfo->openParam.cmdQueueDepth) ? TRUE : FALSE;
            queueInfo->reportQueueEmpty = (decInfo->reportQueueCount == 0) ? TRUE : FALSE;
        }
        break;
    case SET_SEC_AXI:
        {
            SecAxiUse secAxiUse = *(SecAxiUse *)param;

            if (secAxiUse.u.wave.useBitEnable) {
                if (instance->codecMode == W_AV1_DEC) {
                    return RETCODE_INVALID_PARAM;
                }
                if (instance->codecMode == W_AVC_DEC) {
                    if ((decInfo->productCode != WAVE517_CODE) && (decInfo->productCode != WAVE511_CODE) && (decInfo->productCode != WAVE521C_DUAL_CODE)) {
                        return RETCODE_INVALID_PARAM;
                    }
                }
            }

            decInfo->secAxiInfo.u.wave.useIpEnable    = secAxiUse.u.wave.useIpEnable;
            decInfo->secAxiInfo.u.wave.useLfRowEnable = secAxiUse.u.wave.useLfRowEnable;
            decInfo->secAxiInfo.u.wave.useBitEnable   = secAxiUse.u.wave.useBitEnable;
            break;
        }
    case DEC_SET_WTL_FRAME_FORMAT:
        {
                FrameBufferFormat frameFormat = *(FrameBufferFormat *)param;
                if ((frameFormat != FORMAT_420)               &&
                    (frameFormat != FORMAT_422)               &&
                    (frameFormat != FORMAT_420_P10_16BIT_MSB) &&
                    (frameFormat != FORMAT_420_P10_16BIT_LSB) &&
                    (frameFormat != FORMAT_420_P10_32BIT_MSB) &&
                    (frameFormat != FORMAT_420_P10_32BIT_LSB) &&
                    (frameFormat != FORMAT_422_P10_16BIT_MSB) &&
                    (frameFormat != FORMAT_422_P10_16BIT_LSB) &&
                    (frameFormat != FORMAT_422_P10_32BIT_MSB) &&
                    (frameFormat != FORMAT_422_P10_32BIT_LSB)) {
                    return RETCODE_INVALID_PARAM;
                }

                decInfo->wtlFormat = frameFormat;

                break;
        }
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

/************************************************************************/
/*                       ENCODER functions                              */
/************************************************************************/
RetCode Wave5VpuEncUpdateBS(CodecInst* instance)
{
    EncInfo*        pEncInfo;
    Int32           coreIdx;
    Uint32          regVal = 0, bsEndian;
    EncOpenParam*   pOpenParam;

    pEncInfo        = VPU_HANDLE_TO_ENCINFO(instance);
    pOpenParam      = &pEncInfo->openParam;
    coreIdx         = instance->coreIdx;

    regVal = vdi_convert_endian(coreIdx, pOpenParam->streamEndian);
    bsEndian = (~regVal&VDI_128BIT_ENDIAN_MASK);

    VpuWriteReg(coreIdx, W5_CMD_ENC_BS_START_ADDR, pEncInfo->streamRdPtr);
    VpuWriteReg(coreIdx, W5_CMD_ENC_BS_SIZE, pEncInfo->streamBufSize);

    VpuWriteReg(coreIdx, W5_BS_OPTION, (pEncInfo->lineBufIntEn<<6) | bsEndian);

    Wave5BitIssueCommand(instance, W5_UPDATE_BS);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == 0) {
        return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuEncGetRdWrPtr(CodecInst* instance, PhysicalAddress *rdPtr, PhysicalAddress *wrPtr)
{
    EncInfo* pEncInfo = VPU_HANDLE_TO_ENCINFO(instance);
    RetCode ret = RETCODE_SUCCESS;

    VpuWriteReg(instance->coreIdx, W5_CMD_ENC_REASON_SEL, pEncInfo->encWrPtrSel);

    ret = SendQuery(instance, GET_BS_WR_PTR);

    if (ret != RETCODE_SUCCESS)
        return RETCODE_QUERY_FAILURE;

    *rdPtr = VpuReadReg(instance->coreIdx, W5_RET_ENC_RD_PTR);
    *rdPtr = VPU_MapToAddr40Bit(instance->coreIdx, *rdPtr);
    *wrPtr = VpuReadReg(instance->coreIdx, W5_RET_ENC_WR_PTR);
    *wrPtr = VPU_MapToAddr40Bit(instance->coreIdx, *wrPtr);

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuBuildUpEncParam(CodecInst* instance, EncOpenParam* param)
{
    RetCode      ret      = RETCODE_SUCCESS;
    EncInfo*     pEncInfo = VPU_HANDLE_TO_ENCINFO(instance);
    Uint32       regVal   = 0;
    vpu_buffer_t vb;
    Uint32       bsEndian;

    pEncInfo->streamRdPtrRegAddr = W5_RET_ENC_RD_PTR;
    pEncInfo->streamWrPtrRegAddr = W5_RET_ENC_WR_PTR;
    pEncInfo->currentPC          = W5_VCPU_CUR_PC;
    pEncInfo->busyFlagAddr       = W5_VPU_BUSY_STATUS;

    if (param->bitstreamFormat == STD_HEVC) {
        instance->codecMode = W_HEVC_ENC;
    }
    else if (param->bitstreamFormat == STD_AVC) {
        instance->codecMode = W_AVC_ENC;
    }
    else {
        return RETCODE_INVALID_PARAM;
    }


    vdi_get_common_memory(instance->coreIdx, &vb);
    pEncInfo->vbTemp.base      = vb.phys_addr + WAVE5_TEMPBUF_OFFSET;
    pEncInfo->vbTemp.phys_addr = pEncInfo->vbTemp.base;
    pEncInfo->vbTemp.virt_addr = pEncInfo->vbTemp.base;
    pEncInfo->vbTemp.size      = WAVE5_TEMPBUF_SIZE;

    vdi_get_sram_memory(instance->coreIdx, &vb);
    pEncInfo->secAxiInfo.bufBase = vb.phys_addr;
    pEncInfo->secAxiInfo.bufSize = vb.size;

    if (instance->productId == PRODUCT_ID_521)
        pEncInfo->vbWork.size       = WAVE521ENC_WORKBUF_SIZE;

    if (vdi_allocate_dma_memory(instance->coreIdx, &pEncInfo->vbWork, ENC_WORK, instance->instIndex) < 0) {
        pEncInfo->vbWork.base       = 0;
        pEncInfo->vbWork.phys_addr  = 0;
        pEncInfo->vbWork.size       = 0;
        pEncInfo->vbWork.virt_addr  = 0;
        return RETCODE_INSUFFICIENT_RESOURCE;
    }

    vdi_clear_memory(instance->coreIdx, pEncInfo->vbWork.phys_addr, pEncInfo->vbWork.size, 0);

    VpuWriteReg(instance->coreIdx, W5_ADDR_WORK_BASE, pEncInfo->vbWork.phys_addr);
    VpuWriteReg(instance->coreIdx, W5_WORK_SIZE,      pEncInfo->vbWork.size);

    regVal = vdi_convert_endian(instance->coreIdx, param->streamEndian);
    bsEndian = (~regVal&VDI_128BIT_ENDIAN_MASK);

    regVal = (param->lineBufIntEn<<6) | bsEndian;
    VpuWriteReg(instance->coreIdx, W5_CMD_BS_PARAM, regVal);
    VpuWriteReg(instance->coreIdx, W5_CMD_NUM_CQ_DEPTH_M1, (param->cmdQueueDepth-1));

    regVal = 0;
#ifdef SUPPORT_SOURCE_RELEASE_INTERRUPT
    regVal |= (param->srcReleaseIntEnable<<2);
#endif
    if (instance->productId == PRODUCT_ID_521)
        regVal |= (param->subFrameSyncEnable | param->subFrameSyncMode<<1);
    VpuWriteReg(instance->coreIdx, W5_CMD_ENC_SRC_OPTIONS, regVal);

    VpuWriteReg(instance->coreIdx, W5_CMD_ENC_VCORE_INFO, 1);

    Wave5BitIssueCommand(instance, W5_CREATE_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, instance->instIndex, W5_CREATE_INSTANCE, 2);
        vdi_free_dma_memory(instance->coreIdx, &pEncInfo->vbWork, ENC_WORK, instance->instIndex);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {  // FAILED for adding into VCPU QUEUE
        vdi_free_dma_memory(instance->coreIdx, &pEncInfo->vbWork, ENC_WORK, instance->instIndex);
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);
        if (regVal == 2)
            ret = RETCODE_INVALID_SFS_INSTANCE;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            ret = RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            ret = RETCODE_VPU_BUS_ERROR;
        else
            ret = RETCODE_FAILURE;
    }

    pEncInfo->subFrameSyncConfig.subFrameSyncMode = param->subFrameSyncMode;
    pEncInfo->subFrameSyncConfig.subFrameSyncOn   = param->subFrameSyncEnable;
    pEncInfo->prefixSeiDataSize   = 0;
    pEncInfo->prefixSeiNalEnable  = 0;
    pEncInfo->prefixSeiNalAddr    = 0;
    pEncInfo->suffixSeiDataSize   = 0;
    pEncInfo->suffixSeiNalEnable  = 0;
    pEncInfo->suffixSeiNalAddr    = 0;
    pEncInfo->streamRdPtr         = param->bitstreamBuffer;
    pEncInfo->streamWrPtr         = param->bitstreamBuffer;
    pEncInfo->lineBufIntEn        = param->lineBufIntEn;
    pEncInfo->streamBufStartAddr  = param->bitstreamBuffer;
    pEncInfo->streamBufSize       = param->bitstreamBufferSize;
    pEncInfo->streamBufEndAddr    = param->bitstreamBuffer + param->bitstreamBufferSize;
    pEncInfo->stride              = 0;
    pEncInfo->vbFrame.size        = 0;
    pEncInfo->vbPPU.size          = 0;
    pEncInfo->frameAllocExt       = 0;
    pEncInfo->ppuAllocExt         = 0;
    pEncInfo->initialInfoObtained = 0;
    pEncInfo->productCode         = g_VpuCoreAttributes[instance->coreIdx].productVersion;

    return ret;
}

RetCode Wave5VpuEncInitSeq(CodecInst* instance)
{
    Int32           coreIdx;
    Uint32          regVal = 0, rotMirMode, fixedCuSizeMode = 0x7;
    EncInfo*        pEncInfo;
    EncOpenParam*   pOpenParam;
    EncWave5Param*  pParam;
    int             frameRateDiv = 0, frameRateRes = 0, frameRate = 0;

    coreIdx    = instance->coreIdx;
    pEncInfo   = &instance->CodecInfo->encInfo;
    pOpenParam = &pEncInfo->openParam;
    pParam     = &pOpenParam->EncStdParam.waveParam;

    /*==============================================*/
    /*  W5_SET_PARAM_OPT_CUSTOM_GOP                 */
    /*==============================================*/
    /*
    * SET_PARAM + CUSTOM_GOP
    * only when gopPresetIdx == custom_gop, custom_gop related registers should be set
    */
    if (pParam->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
        int i=0, j = 0;

        VpuWriteReg(coreIdx, W5_CMD_ENC_CUSTOM_GOP_PARAM, pParam->gopParam.customGopSize);
        for (i=0 ; i<pParam->gopParam.customGopSize; i++) {
            VpuWriteReg(coreIdx, W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_0 + (i*4), (pParam->gopParam.picParam[i].picType<<0)            |
                                                                            (pParam->gopParam.picParam[i].pocOffset<<2)          |
                                                                            (pParam->gopParam.picParam[i].picQp<<6)              |
                                                                            (pParam->gopParam.picParam[i].useMultiRefP<<13)      |
                                                                            ((pParam->gopParam.picParam[i].refPocL0&0x1F)<<14)   |
                                                                            ((pParam->gopParam.picParam[i].refPocL1&0x1F)<<19)   |
                                                                            (pParam->gopParam.picParam[i].temporalId<<24));
        }

        for (j = i; j < MAX_GOP_NUM; j++) {
            VpuWriteReg(coreIdx, W5_CMD_ENC_CUSTOM_GOP_PIC_PARAM_0 + (j*4), 0);
        }
    }
    if (pParam->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_SET_PARAM_OPTION, W5_SET_PARAM_OPT_CUSTOM_GOP);
        Wave5BitIssueCommand(instance, W5_ENC_SET_PARAM);

        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            if (instance->loggingEnable)
                vdi_log(coreIdx, instance->instIndex, W5_ENC_SET_PARAM, 2);
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }

    /*===================================================================================*/
    /*  W5_SET_PARAM_OPT_COMMON                                                          */
    /*      : the last SET_PARAM command should be called with W5_SET_PARAM_OPT_COMMON   */
    /*===================================================================================*/
    rotMirMode = 0;
    /* CMD_ENC_ROT_MODE :
    *          | hor_mir | ver_mir |   rot_angle     | rot_en |
    *              [4]       [3]         [2:1]           [0]
    */
    if (pEncInfo->rotationEnable == TRUE) {
        switch (pEncInfo->rotationAngle) {
        case 0:   rotMirMode |= 0x0; break;
        case 90:  rotMirMode |= 0x3; break;
        case 180: rotMirMode |= 0x5; break;
        case 270: rotMirMode |= 0x7; break;
        }
    }

    if (pEncInfo->mirrorEnable == TRUE) {
        switch (pEncInfo->mirrorDirection) {
        case MIRDIR_NONE:    rotMirMode |= 0x0;  break;
        case MIRDIR_VER:     rotMirMode |= 0x9;  break;
        case MIRDIR_HOR:     rotMirMode |= 0x11; break;
        case MIRDIR_HOR_VER: rotMirMode |= 0x19; break;
        }
    }

    SetEncCropInfo(instance->codecMode, &pParam->confWin, rotMirMode, pOpenParam->picWidth, pOpenParam->picHeight);

    /* SET_PARAM + COMMON */
    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_SET_PARAM_OPTION,  W5_SET_PARAM_OPT_COMMON);
    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_SRC_SIZE,          VPU_ALIGN8(pOpenParam->picHeight)<<16 | VPU_ALIGN8(pOpenParam->picWidth));
    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MAP_ENDIAN, VDI_LITTLE_ENDIAN);

    if (instance->codecMode == W_HEVC_ENC) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_S2_SEARCH_RANGE, ((pParam->s2SearchRangeYDiv4&0x7f)<<16) | ((pParam->s2SearchRangeXDiv4&0x7f)<<0));
    }

    if (instance->codecMode == W_AVC_ENC) {
        regVal = (pParam->profile<<0)                    |
                 (pParam->level<<3)                      |
                 (pParam->internalBitDepth<<14)          |
                 (pParam->useLongTerm<<21);
        if (pParam->scalingListEnable == 2) {
            regVal |= (1<<22) | (1<<23);    // [23]=USE_DEFAULT_SCALING_LIST
        } else { // 0 or 1
            regVal |= (pParam->scalingListEnable<<22);
        }
    }
    else {  // HEVC enc
        regVal = (pParam->profile<<0)                    |
                 (pParam->level<<3)                      |
                 (pParam->tier<<12)                      |
                 (pParam->internalBitDepth<<14)          |
                 (pParam->useLongTerm<<21)               |
                 (pParam->tmvpEnable<<23)                |
                 (pParam->saoEnable<<24)                 |
                 (pParam->skipIntraTrans<<25)            |
                 (pParam->strongIntraSmoothEnable<<27)   |
                 (pParam->enStillPicture<<30);
        if (pParam->scalingListEnable == 2) {
            regVal |= (1<<22) | (1UL<<31);    // [31]=USE_DEFAULT_SCALING_LIST
        } else {
            regVal |= (pParam->scalingListEnable<<22);
        }
    }
    VLOG(INFO, "SPS_PARAM = 0x%x\n", regVal);
    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_SPS_PARAM,  regVal);
    regVal = (pParam->losslessEnable)                |
             (pParam->constIntraPredFlag<<1)         |
             (pParam->lfCrossSliceBoundaryEnable<<2) |
             (pParam->weightPredEnable<<3)           |
             (pParam->wppEnable<<4)                  |
             (pParam->disableDeblk<<5)               |
             ((pParam->betaOffsetDiv2&0xF)<<6)       |
             ((pParam->tcOffsetDiv2&0xF)<<10)        |
             ((pParam->chromaCbQpOffset&0x1F)<<14)   |
             ((pParam->chromaCrQpOffset&0x1F)<<19)   |
             (pParam->transform8x8Enable<<29)        |
             (pParam->entropyCodingMode<<30);
    VLOG(INFO, "PPS_PARAM = 0x%x\n", regVal);
    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_PPS_PARAM,  regVal);

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_GOP_PARAM,  pParam->gopPresetIdx);

    if (instance->codecMode == W_AVC_ENC) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INTRA_PARAM, (pParam->intraQP<<0) | ((pParam->intraPeriod&0x7ff)<<6) | ((pParam->avcIdrPeriod&0x7ff)<<17) | ((pParam->forcedIdrHeaderEnable&3)<<28));
    }
    else {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INTRA_PARAM, (pParam->decodingRefreshType<<0) | (pParam->intraQP<<3) | (pParam->forcedIdrHeaderEnable<<9) | (pParam->intraPeriod<<16));
    }

    regVal  = (pParam->useRecommendEncParam)     |
              (pParam->rdoSkip<<2)               |
              (pParam->lambdaScalingEnable<<3)   |
              (pParam->coefClearDisable<<4)      |
              (fixedCuSizeMode<<5)               |
              (pParam->intraNxNEnable<<8)        |
              (pParam->maxNumMerge<<18)          |
              (pParam->customMDEnable<<20)       |
              (pParam->customLambdaEnable<<21)   |
              (pParam->monochromeEnable<<22);

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RDO_PARAM, regVal);

    if (instance->codecMode == W_AVC_ENC) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INTRA_REFRESH, pParam->intraMbRefreshArg<<16 | pParam->intraMbRefreshMode);
    }
    else {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INTRA_REFRESH, pParam->intraRefreshArg<<16 | pParam->intraRefreshMode);
    }

    frameRateDiv = (pOpenParam->frameRateInfo >> 16) + 1;
    frameRateRes = pOpenParam->frameRateInfo & 0xFFFF;

    frameRate = frameRateRes / frameRateDiv;

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_FRAME_RATE,  frameRate);
    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_TARGET_RATE, pOpenParam->bitRate);

    if (instance->codecMode == W_AVC_ENC) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_PARAM,   (pOpenParam->rcEnable<<0)           |
                                                        (pParam->mbLevelRcEnable<<1)         |
                                                        (pParam->hvsQPEnable<<2)             |
                                                        (pParam->hvsQpScale<<4)              |
                                                        (pParam->bitAllocMode<<8)            |
                                                        (pParam->roiEnable<<13)              |
                                                        ((pParam->initialRcQp&0x3F)<<14)     |
                                                        (pOpenParam->vbvBufferSize<<20));
    }
    else {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_PARAM,   (pOpenParam->rcEnable<<0)           |
                                                        (pParam->cuLevelRCEnable<<1)         |
                                                        (pParam->hvsQPEnable<<2)             |
                                                        (pParam->hvsQpScale<<4)              |
                                                        (pParam->bitAllocMode<<8)            |
                                                        (pParam->roiEnable<<13)              |
                                                        ((pParam->initialRcQp&0x3F)<<14)     |
                                                        (pOpenParam->vbvBufferSize<<20));
    }

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_WEIGHT_PARAM, pParam->rcWeightBuf<<8 | pParam->rcWeightParam);

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_MIN_MAX_QP, (pParam->minQpI<<0)                   |
                                                       (pParam->maxQpI<<6)                   |
                                                       (pParam->hvsMaxDeltaQp<<12));

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_INTER_MIN_MAX_QP, (pParam->minQpP << 0)   |
                                                             (pParam->maxQpP << 6)   |
                                                             (pParam->minQpB << 12)  |
                                                             (pParam->maxQpB << 18));

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_BIT_RATIO_LAYER_0_3, (pParam->fixedBitRatio[0]<<0)  |
                                                                (pParam->fixedBitRatio[1]<<8)  |
                                                                (pParam->fixedBitRatio[2]<<16) |
                                                                (pParam->fixedBitRatio[3]<<24));

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_BIT_RATIO_LAYER_4_7, (pParam->fixedBitRatio[4]<<0)  |
                                                                (pParam->fixedBitRatio[5]<<8)  |
                                                                (pParam->fixedBitRatio[6]<<16) |
                                                                (pParam->fixedBitRatio[7]<<24));

    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_ROT_PARAM,  rotMirMode);


    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_BG_PARAM, (pParam->bgDetectEnable)       |
                                                  (pParam->bgThrDiff<<1)         |
                                                  (pParam->bgThrMeanDiff<<10)    |
                                                  (pParam->bgLambdaQp<<18)       |
                                                  ((pParam->bgDeltaQp&0x1F)<<24) |
                                                  (instance->codecMode == W_AVC_ENC ? pParam->s2fmeDisable<<29 : 0));


    if (instance->codecMode == W_HEVC_ENC || instance->codecMode == W_AVC_ENC) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_LAMBDA_ADDR, pParam->customLambdaAddr);

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CONF_WIN_TOP_BOT, pParam->confWin.bottom<<16 | pParam->confWin.top);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CONF_WIN_LEFT_RIGHT, pParam->confWin.right<<16 | pParam->confWin.left);

        if (instance->codecMode == W_AVC_ENC) {
            VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INDEPENDENT_SLICE, pParam->avcSliceArg<<16 | pParam->avcSliceMode);
        }
        else {
            VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INDEPENDENT_SLICE, pParam->independSliceModeArg<<16 | pParam->independSliceMode);
        }

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_USER_SCALING_LIST_ADDR, pParam->userScalingListAddr);

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_NUM_UNITS_IN_TICK, pParam->numUnitsInTick);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_TIME_SCALE, pParam->timeScale);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_NUM_TICKS_POC_DIFF_ONE, pParam->numTicksPocDiffOne);
    }

    if (instance->codecMode == W_HEVC_ENC) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_PU04, (pParam->pu04DeltaRate&0xFF)                 |
                                                            ((pParam->pu04IntraPlanarDeltaRate&0xFF)<<8) |
                                                            ((pParam->pu04IntraDcDeltaRate&0xFF)<<16)    |
                                                            ((pParam->pu04IntraAngleDeltaRate&0xFF)<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_PU08, (pParam->pu08DeltaRate&0xFF)                 |
                                                            ((pParam->pu08IntraPlanarDeltaRate&0xFF)<<8) |
                                                            ((pParam->pu08IntraDcDeltaRate&0xFF)<<16)    |
                                                            ((pParam->pu08IntraAngleDeltaRate&0xFF)<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_PU16, (pParam->pu16DeltaRate&0xFF)                 |
                                                            ((pParam->pu16IntraPlanarDeltaRate&0xFF)<<8) |
                                                            ((pParam->pu16IntraDcDeltaRate&0xFF)<<16)    |
                                                            ((pParam->pu16IntraAngleDeltaRate&0xFF)<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_PU32, (pParam->pu32DeltaRate&0xFF)                 |
                                                            ((pParam->pu32IntraPlanarDeltaRate&0xFF)<<8) |
                                                            ((pParam->pu32IntraDcDeltaRate&0xFF)<<16)    |
                                                            ((pParam->pu32IntraAngleDeltaRate&0xFF)<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_CU08, (pParam->cu08IntraDeltaRate&0xFF)        |
                                                            ((pParam->cu08InterDeltaRate&0xFF)<<8)   |
                                                            ((pParam->cu08MergeDeltaRate&0xFF)<<16));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_CU16, (pParam->cu16IntraDeltaRate&0xFF)        |
                                                            ((pParam->cu16InterDeltaRate&0xFF)<<8)   |
                                                            ((pParam->cu16MergeDeltaRate&0xFF)<<16));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_CU32, (pParam->cu32IntraDeltaRate&0xFF)        |
                                                            ((pParam->cu32InterDeltaRate&0xFF)<<8)   |
                                                            ((pParam->cu32MergeDeltaRate&0xFF)<<16));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_DEPENDENT_SLICE, pParam->dependSliceModeArg<<16 | pParam->dependSliceMode);

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_NR_PARAM,   (pParam->nrYEnable<<0)       |
                                                        (pParam->nrCbEnable<<1)      |
                                                        (pParam->nrCrEnable<<2)      |
                                                        (pParam->nrNoiseEstEnable<<3)|
                                                        (pParam->nrNoiseSigmaY<<4)   |
                                                        (pParam->nrNoiseSigmaCb<<12) |
                                                        (pParam->nrNoiseSigmaCr<<20));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_NR_WEIGHT,  (pParam->nrIntraWeightY<<0)  |
                                                        (pParam->nrIntraWeightCb<<5) |
                                                        (pParam->nrIntraWeightCr<<10)|
                                                        (pParam->nrInterWeightY<<15) |
                                                        (pParam->nrInterWeightCb<<20)|
                                                        (pParam->nrInterWeightCr<<25));
    }

    if (pEncInfo->openParam.encodeVuiRbsp || pEncInfo->openParam.encodeHrdRbspInVPS) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_VUI_HRD_PARAM, (pEncInfo->openParam.hrdRbspDataSize<<18)   |
                                                           (pEncInfo->openParam.vuiRbspDataSize<<4)    |
                                                           (pEncInfo->openParam.encodeHrdRbspInVPS<<2) |
                                                           (pEncInfo->openParam.encodeVuiRbsp));
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_VUI_RBSP_ADDR, pEncInfo->openParam.vuiRbspDataAddr);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_HRD_RBSP_ADDR, pEncInfo->openParam.hrdRbspDataAddr);
    }
    else {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_VUI_HRD_PARAM, 0);
    }

    /* host rc param */
#ifdef ENABLE_HOST_RC
    {	/*	W5_CMD_ENC_SEQ_HOST_RC_PARAM_0x34(0x1D0)
                [15: 8] : u32RCIpQpDelta + 10;   /// Range:[-10,10] -> [0,20]
                [ 7: 4] : u32EnableHierarchy;    /// Range:[0,  1]
                [ 3: 0] : enRcMode
                        0 : C&M R/C
                        1 : host_CBR
                        2 : host_VBR
                        3 : host_ABR
                        4 : host_UNKNOWN
                        5 ~ 15 : N/A
        */
        VLOG(INFO, "hostrc mode:%d, EnableHierarchy:%d, RCIpQpDelta:%d\n"
                , pOpenParam->hostRcmode, pOpenParam->enableHierarchy, pOpenParam->rcIpQpDelta);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_HOST_RC_PARAM_0x34,  pOpenParam->hostRcmode                    |
                                                                (pOpenParam->enableHierarchy      << 4)    |
                                                                ((pOpenParam->rcIpQpDelta + 10)   << 8)
        );

	    /*	W5_CMD_ENC_SEQ_HOST_RC_PARAM_0x35(0x1D4)
                [31:24] : N/A
                [23:16] : u32RcLastClip;         /// Range:[0, 12]
                [15: 8] : u32RcLevelClip;        /// Range:[0, 12]
                [ 7: 0] : u32RcPicNormalClip;    /// Range:[0, 12]
        */
        VLOG(INFO, "hostrc lastClip:%d, levelClip:%d, normalClip:%d\n"
                , pOpenParam->rcLastClip, pOpenParam->rcLevelClip, pOpenParam->rcPicNormalClip);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_HOST_RC_PARAM_0x35, pOpenParam->rcPicNormalClip            |
                                                                (pOpenParam->rcLevelClip      << 8 )   |
                                                                (pOpenParam->rcLastClip       << 16)
        );

    	/*	W5_CMD_ENC_SEQ_HOST_RC_PARAM_0x36(0x1D8)
                [31:24] : u32StatTime;           /// Range:[1, 60];
                [23:16] : u32MinIprop;           /// Range:[1, 100]
                [15: 8] : u32MaxIprop;           /// Range:(u32MinIprop, 100]
                [ 7: 0] : s32ChangePos;          /// Range:[50, 100]
        */
        VLOG(INFO, "hostrc stat:%d, minIprop:%d, maxIprop:%d, changPos:%d\n"
                , pOpenParam->statTime, pOpenParam->minIprop
                , pOpenParam->maxIprop, pOpenParam->changePos);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_HOST_RC_PARAM_0x36, pOpenParam->changePos                |
                                                                (pOpenParam->maxIprop      << 8  )   |
                                                                (pOpenParam->minIprop      << 16 )   |
                                                                (pOpenParam->statTime      << 24)
        );

    	/*	W5_CMD_ENC_SEQ_HOST_RC_PARAM_0x37(0x1DC)
                [31:24] : s32MinStillPercent;    /// Range:[5, 100]
                [23:16] : u32MaxStillQP;         /// Range:[u32MinIQp, u32MaxIQp]
                [15: 8] : u32MotionSensitivity;  /// Range:[0, 100]
                [ 7: 0] : s32AvbrPureStillThr;   /// Range:[0, 128]
        */
        VLOG(INFO, "hostrc minstillPercent:%d, maxStillQp:%d, MotionSensitivity:%d, AvbrPureStillThr:%d\n"
                , pOpenParam->minStillPercent, pOpenParam->maxStillQp
                , pOpenParam->motionSensitivity, pOpenParam->avbrPureStillThr);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_HOST_RC_PARAM_0x37, pOpenParam->avbrPureStillThr               |
                                                                (pOpenParam->motionSensitivity   << 8  )   |
                                                                (pOpenParam->maxStillQp          << 16 )   |
                                                                (pOpenParam->minStillPercent     << 24)
        );
    }
#endif /// ENABLE_HOST_RC

    Wave5BitIssueCommand(instance, W5_ENC_SET_PARAM);
    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        if (instance->loggingEnable)
            vdi_log(coreIdx, instance->instIndex, W5_ENC_SET_PARAM, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    if (VpuReadReg(coreIdx, W5_RET_SUCCESS) == 0) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_QUEUEING_FAIL) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_FAIL_REASON);
            VLOG(ERR, "QUEUE_FAIL_REASON = 0x%x\n", regVal);
            return RETCODE_QUEUEING_FAILURE;
        }
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            return RETCODE_VPU_BUS_ERROR;
        else
            return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuEncGetSeqInfo(CodecInst* instance, EncInitialInfo* info)
{
    RetCode     ret = RETCODE_SUCCESS;
    Uint32      regVal;
    EncInfo*    pEncInfo;

    pEncInfo = VPU_HANDLE_TO_ENCINFO(instance);

    // Send QUERY cmd
    ret = SendQuery(instance, GET_RESULT);
    if (ret != RETCODE_SUCCESS) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_RESULT_NOT_READY)
            return RETCODE_REPORT_NOT_READY;
        else if(regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            return RETCODE_VPU_BUS_ERROR;
        else
            return RETCODE_QUERY_FAILURE;
    }

    if (instance->loggingEnable)
        vdi_log(instance->coreIdx, instance->instIndex, W5_INIT_SEQ, 0);

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pEncInfo->instanceQueueCount = (regVal>>16)&0xff;
    pEncInfo->reportQueueCount   = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_ENC_ENCODING_SUCCESS) != 1) {
        info->seqInitErrReason = VpuReadReg(instance->coreIdx, W5_RET_ENC_ERR_INFO);
        ret = RETCODE_FAILURE;
    }
    else {
        info->warnInfo = VpuReadReg(instance->coreIdx, W5_RET_ENC_WARN_INFO);
    }

    info->minFrameBufferCount   = VpuReadReg(instance->coreIdx, W5_RET_ENC_NUM_REQUIRED_FB);
    info->minSrcFrameCount      = VpuReadReg(instance->coreIdx, W5_RET_ENC_MIN_SRC_BUF_NUM);
    info->maxLatencyPictures    = VpuReadReg(instance->coreIdx, W5_RET_ENC_PIC_MAX_LATENCY_PICTURES);
    info->vlcBufSize            = VpuReadReg(instance->coreIdx, W5_RET_VLC_BUF_SIZE);
    info->paramBufSize          = VpuReadReg(instance->coreIdx, W5_RET_PARAM_BUF_SIZE);
    pEncInfo->vlcBufSize        = info->vlcBufSize;
    pEncInfo->paramBufSize      = info->paramBufSize;

    return ret;
}

RetCode Wave5VpuEncRegisterFramebuffer(CodecInst* inst, FrameBuffer* fbArr, Uint32 count)
{
    RetCode       ret = RETCODE_SUCCESS;
    Int32         q, j, i, remain, idx, coreIdx, startNo, endNo, stride;
    Uint32        regVal=0, picSize=0, mvColSize, fbcYTblSize, fbcCTblSize, subSampledSize=0;
    Uint32        endian, nv21=0, cbcrInterleave = 0, lumaStride, chromaStride, bufHeight = 0, bufWidth = 0;
    vpu_buffer_t  vbMV = {0,};
    vpu_buffer_t  vbFbcYTbl = {0,};
    vpu_buffer_t  vbFbcCTbl = {0,};
    vpu_buffer_t  vbSubSamBuf = {0,};
    vpu_buffer_t  vbTask = {0,};
    EncOpenParam* pOpenParam;
    EncInfo*      pEncInfo = &inst->CodecInfo->encInfo;

    pOpenParam     = &pEncInfo->openParam;
    coreIdx        = inst->coreIdx;
    mvColSize      = fbcYTblSize = fbcCTblSize = 0;
    stride         = pEncInfo->stride;

    if (inst->codecMode == W_AVC_ENC) {
        bufWidth  = VPU_ALIGN16(pOpenParam->picWidth);
        bufHeight = VPU_ALIGN16(pOpenParam->picHeight);

        if ((pEncInfo->rotationAngle != 0 || pEncInfo->mirrorDirection != 0) && !(pEncInfo->rotationAngle == 180 && pEncInfo->mirrorDirection == MIRDIR_HOR_VER)) {
            bufWidth  = VPU_ALIGN16(pOpenParam->picWidth);
            bufHeight = VPU_ALIGN16(pOpenParam->picHeight);
        }

        if (pEncInfo->rotationAngle == 90 || pEncInfo->rotationAngle == 270) {
            bufWidth  = VPU_ALIGN16(pOpenParam->picHeight);
            bufHeight = VPU_ALIGN16(pOpenParam->picWidth);
        }
    }
    else {
        bufWidth  = VPU_ALIGN8(pOpenParam->picWidth);
        bufHeight = VPU_ALIGN8(pOpenParam->picHeight);

        if ((pEncInfo->rotationAngle != 0 || pEncInfo->mirrorDirection != 0) && !(pEncInfo->rotationAngle == 180 && pEncInfo->mirrorDirection == MIRDIR_HOR_VER)) {
            bufWidth  = VPU_ALIGN32(pOpenParam->picWidth);
            bufHeight = VPU_ALIGN32(pOpenParam->picHeight);
        }

        if (pEncInfo->rotationAngle == 90 || pEncInfo->rotationAngle == 270) {
            bufWidth  = VPU_ALIGN32(pOpenParam->picHeight);
            bufHeight = VPU_ALIGN32(pOpenParam->picWidth);
        }
    }

    picSize = (bufWidth<<16) | bufHeight;

    if (inst->codecMode == W_HEVC_ENC) {
        mvColSize      = WAVE5_ENC_HEVC_MVCOL_BUF_SIZE(bufWidth, bufHeight);
        mvColSize      = VPU_ALIGN16(mvColSize);
    }
    else if (inst->codecMode == W_AVC_ENC) {
        mvColSize      = WAVE5_ENC_AVC_MVCOL_BUF_SIZE(bufWidth, bufHeight);
    }

    vbMV.phys_addr = 0;
    vbMV.size      = ((mvColSize+4095)&~4095)+4096;   /* 4096 is a margin */
    for (idx = 0; idx < count; idx++) {
        if (vdi_allocate_dma_memory(inst->coreIdx, &vbMV, ENC_MV, inst->instIndex) < 0)
            return RETCODE_INSUFFICIENT_RESOURCE;

        pEncInfo->vbMV[idx] = vbMV;
    }

    if (pEncInfo->productCode == WAVE521C_DUAL_CODE) {
        Uint32 bgs_width, ot_bg_width, comp_frm_width, ot_frm_width, ot_bg_height, bgs_height, comp_frm_height, ot_frm_height;
        Uint32 frm_width, frm_height;
        Uint32 dualWidth  = bufWidth;
        Uint32 dualHeight = bufHeight;
        bgs_width = (pOpenParam->EncStdParam.waveParam.internalBitDepth >8 ? 256 : 512);

        if (inst->codecMode == W_AVC_ENC)
            ot_bg_width = 1024;
        else // if (inst->codecMode == W_HEVC_ENC)
            ot_bg_width = 512;

        frm_width      = VPU_CEIL(dualWidth, 16);
        frm_height     = VPU_CEIL(dualHeight, 16);
        comp_frm_width = VPU_CEIL(VPU_CEIL(frm_width , 16) + 16, 16); // valid_width = align(width, 16), comp_frm_width = align(valid_width+pad_x, 16)
        ot_frm_width   = VPU_CEIL(comp_frm_width, ot_bg_width);       // 1024 = offset table BG width

        // sizeof_offset_table()
        ot_bg_height    = 32;
        bgs_height      = (1<<14) / bgs_width / (pOpenParam->EncStdParam.waveParam.internalBitDepth>8 ? 2 : 1);
        comp_frm_height = VPU_CEIL(VPU_CEIL(frm_height, 4) + 4, bgs_height);
        ot_frm_height   = VPU_CEIL(comp_frm_height, ot_bg_height);
        fbcYTblSize     = (ot_frm_width/16) * (ot_frm_height/4) *2;
    }
    else {
        fbcYTblSize = WAVE5_FBC_LUMA_TABLE_SIZE(bufWidth, bufHeight);
        fbcYTblSize = VPU_ALIGN16(fbcYTblSize);
    }

    vbFbcYTbl.phys_addr = 0;
    vbFbcYTbl.size      = ((fbcYTblSize+4095)&~4095)+4096;   /* 4096 is a margin */
    for (idx = 0; idx < count; idx++) {
        if (vdi_allocate_dma_memory(inst->coreIdx, &vbFbcYTbl, ENC_FBCY_TBL, inst->instIndex) < 0)
            return RETCODE_INSUFFICIENT_RESOURCE;

        pEncInfo->vbFbcYTbl[idx] = vbFbcYTbl;
    }

    if (pEncInfo->productCode == WAVE521C_DUAL_CODE) {
        Uint32 bgs_width, ot_bg_width, comp_frm_width, ot_frm_width, ot_bg_height, bgs_height, comp_frm_height, ot_frm_height;
        Uint32 frm_width, frm_height;
        Uint32 dualWidth  = bufWidth;
        Uint32 dualHeight = bufHeight;
        bgs_width = (pOpenParam->EncStdParam.waveParam.internalBitDepth >8 ? 256 : 512);

        if (inst->codecMode == W_AVC_ENC)
            ot_bg_width = 1024;
        else // if (inst->codecMode == W_HEVC_ENC)
            ot_bg_width = 512;

        frm_width      = VPU_CEIL(dualWidth, 16);
        frm_height     = VPU_CEIL(dualHeight, 16);
        comp_frm_width = VPU_CEIL(VPU_CEIL(frm_width/2 , 16) + 16, 16); // valid_width = align(width, 16), comp_frm_width = align(valid_width+pad_x, 16)
        ot_frm_width   = VPU_CEIL(comp_frm_width, ot_bg_width);    // 1024 = offset table BG width

        // sizeof_offset_table()
        ot_bg_height    = 32;
        bgs_height      = (1<<14) / bgs_width / (pOpenParam->EncStdParam.waveParam.internalBitDepth>8 ? 2 : 1);
        comp_frm_height = VPU_CEIL(VPU_CEIL(frm_height, 4) + 4, bgs_height);
        ot_frm_height   = VPU_CEIL(comp_frm_height, ot_bg_height);
        fbcCTblSize     = (ot_frm_width/16) * (ot_frm_height/4) *2;
    }
    else {
        fbcCTblSize = WAVE5_FBC_CHROMA_TABLE_SIZE(bufWidth, bufHeight);
        fbcCTblSize = VPU_ALIGN16(fbcCTblSize);
    }

    vbFbcCTbl.phys_addr = 0;
    vbFbcCTbl.size      = ((fbcCTblSize+4095)&~4095)+4096;   /* 4096 is a margin */
    for (idx = 0; idx < count; idx++) {
        if (vdi_allocate_dma_memory(inst->coreIdx, &vbFbcCTbl, ENC_FBCC_TBL, inst->instIndex) < 0)
            return RETCODE_INSUFFICIENT_RESOURCE;

        pEncInfo->vbFbcCTbl[idx] = vbFbcCTbl;
    }

    if (pOpenParam->bitstreamFormat == STD_AVC) {
        subSampledSize = WAVE5_SUBSAMPLED_ONE_SIZE_AVC(bufWidth, bufHeight);
    }
    else {
        subSampledSize = WAVE5_SUBSAMPLED_ONE_SIZE(bufWidth, bufHeight);
    }
    vbSubSamBuf.size      = ((subSampledSize*count+4095)&~4095)+4096;
    vbSubSamBuf.phys_addr = 0;
    if (vdi_allocate_dma_memory(coreIdx, &vbSubSamBuf, ENC_SUBSAMBUF, inst->instIndex) < 0)
        return RETCODE_INSUFFICIENT_RESOURCE;

    pEncInfo->vbSubSamBuf[0]   = vbSubSamBuf;

    if (pOpenParam->cmdQueueDepth == 1)
        vbTask.size      = (Uint32)((pEncInfo->vlcBufSize * 2) + (pEncInfo->paramBufSize * pOpenParam->cmdQueueDepth));
    else
        vbTask.size      = (Uint32)((pEncInfo->vlcBufSize * VLC_BUF_NUM) + (pEncInfo->paramBufSize * pOpenParam->cmdQueueDepth));
    vbTask.phys_addr = 0;
    if (pEncInfo->vbTask.size == 0) {
        if (vdi_allocate_dma_memory(coreIdx, &vbTask, ENC_TASK, inst->instIndex) < 0)
            return RETCODE_INSUFFICIENT_RESOURCE;

        pEncInfo->vbTask = vbTask;

        VpuWriteReg(coreIdx, W5_CMD_SET_FB_ADDR_TASK_BUF, pEncInfo->vbTask.phys_addr);
        VpuWriteReg(coreIdx, W5_CMD_SET_FB_TASK_BUF_SIZE, vbTask.size);
    }

    VpuWriteReg(coreIdx, W5_ADDR_SUB_SAMPLED_FB_BASE, vbSubSamBuf.phys_addr);     // set sub-sampled buffer base addr
    VpuWriteReg(coreIdx, W5_SUB_SAMPLED_ONE_FB_SIZE, subSampledSize);             // set sub-sampled buffer size for one frame

    endian = vdi_convert_endian(coreIdx, fbArr[0].endian);

    VpuWriteReg(coreIdx, W5_PIC_SIZE, picSize);


    // set stride of Luma/Chroma for compressed buffer
    if ((pEncInfo->rotationAngle != 0 || pEncInfo->mirrorDirection != 0) && !(pEncInfo->rotationAngle == 180 && pEncInfo->mirrorDirection == MIRDIR_HOR_VER)){
        lumaStride   = VPU_ALIGN16(bufWidth)*(pOpenParam->EncStdParam.waveParam.internalBitDepth >8 ? 5 : 4);
        lumaStride   = VPU_ALIGN32(lumaStride);
        chromaStride = VPU_ALIGN16(bufWidth/2)*(pOpenParam->EncStdParam.waveParam.internalBitDepth >8 ? 5 : 4);
        chromaStride = VPU_ALIGN32(chromaStride);
    }
    else {
        lumaStride   = VPU_ALIGN16(pOpenParam->picWidth)*(pOpenParam->EncStdParam.waveParam.internalBitDepth >8 ? 5 : 4);
        lumaStride   = VPU_ALIGN32(lumaStride);
        chromaStride = VPU_ALIGN16(pOpenParam->picWidth/2)*(pOpenParam->EncStdParam.waveParam.internalBitDepth >8 ? 5 : 4);
        chromaStride = VPU_ALIGN32(chromaStride);
    }

    VpuWriteReg(coreIdx, W5_FBC_STRIDE, lumaStride<<16 | chromaStride);

    cbcrInterleave = pOpenParam->cbcrInterleave;
    regVal = (nv21           << 29) |
             (cbcrInterleave << 16) |
             (stride);

    VpuWriteReg(coreIdx, W5_COMMON_PIC_INFO, regVal);

    if (pOpenParam->EncStdParam.waveParam.enReportNalSize)
        VpuWriteReg(coreIdx, W5_ADDR_NAL_SIZE_REPORT_BASE, pOpenParam->EncStdParam.waveParam.reportNalSizeAddr);

    // todo mark: remap feature
    // if (pEncInfo->addrRemapEn) {
    //     InitAddrRemapFb(inst);
    // }

    remain = count;
    q      = (remain+7)/8;
    idx    = 0;
    for (j=0; j<q; j++) {
        regVal = (endian<<16) | (j==q-1)<<4 | ((j==0)<<3);//lint !e514
        regVal |= (pOpenParam->enableNonRefFbcWrite<< 26);
        VpuWriteReg(coreIdx, W5_SFB_OPTION, regVal);
        startNo = j*8;
        endNo   = startNo + (remain>=8 ? 8 : remain) - 1;

        VpuWriteReg(coreIdx, W5_SET_FB_NUM, (startNo<<8)|endNo);

        for (i=0; i<8 && i<remain; i++) {
            VpuWriteReg(coreIdx, W5_ADDR_LUMA_BASE0  + (i<<4), fbArr[i+startNo].bufY);
            VpuWriteReg(coreIdx, W5_ADDR_CB_BASE0    + (i<<4), fbArr[i+startNo].bufCb);
            APIDPRINT("REGISTER FB[%02d] Y(0x%08x), Cb(0x%08x) ", i, fbArr[i+startNo].bufY, fbArr[i+startNo].bufCb);
            VpuWriteReg(coreIdx, W5_ADDR_FBC_Y_OFFSET0 + (i<<4), pEncInfo->vbFbcYTbl[i].phys_addr); /* Luma FBC offset table */
            VpuWriteReg(coreIdx, W5_ADDR_FBC_C_OFFSET0 + (i<<4), pEncInfo->vbFbcCTbl[i].phys_addr); /* Chroma FBC offset table */

            VpuWriteReg(coreIdx, W5_ADDR_MV_COL0  + (i<<2), pEncInfo->vbMV[i].phys_addr);
            APIDPRINT("Yo(0x%08x) Co(0x%08x), Mv(0x%08x)\n",pEncInfo->vbFbcYTbl[i], pEncInfo->vbFbcCTbl[i], pEncInfo->vbMV[i]);
            idx++;
        }
        remain -= i;

        Wave5BitIssueCommand(inst, W5_SET_FB);
        if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
    }

    regVal = VpuReadReg(coreIdx, W5_RET_SUCCESS);
    if (regVal == 0) {
        return RETCODE_FAILURE;
    }

    return ret;
}

RetCode Wave5VpuEncode(CodecInst* instance, EncParam* option)
{
    Int32           coreIdx, srcFrameFormat;
    Uint32          regVal = 0, bsEndian;
    Uint32          srcStrideC = 0;
    EncInfo*        pEncInfo;
    FrameBuffer*    pSrcFrame;
    EncOpenParam*   pOpenParam;
    BOOL            justified = WTL_RIGHT_JUSTIFIED;
    Uint32          formatNo  = WTL_PIXEL_8BIT;

    coreIdx     = instance->coreIdx;
    pEncInfo    = VPU_HANDLE_TO_ENCINFO(instance);
    pOpenParam  = &pEncInfo->openParam;
    pSrcFrame   = option->sourceFrame;


    VpuWriteReg(coreIdx, W5_CMD_ENC_BS_START_ADDR, option->picStreamBufferAddr);
    VpuWriteReg(coreIdx, W5_CMD_ENC_BS_SIZE, option->picStreamBufferSize);
    pEncInfo->streamBufStartAddr = option->picStreamBufferAddr;
    pEncInfo->streamBufSize = option->picStreamBufferSize;
    pEncInfo->streamBufEndAddr = option->picStreamBufferAddr + option->picStreamBufferSize;

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_AXI_SEL, DEFAULT_SRC_AXI);
    /* Secondary AXI */
    regVal  = (pEncInfo->secAxiInfo.u.wave.useEncRdoEnable << 11) | (pEncInfo->secAxiInfo.u.wave.useEncLfEnable << 15);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_USE_SEC_AXI, regVal);

    regVal  = 0;
    regVal |= (pOpenParam->EncStdParam.waveParam.enReportNalSize<<2);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_REPORT_PARAM, regVal);

    if (option->codeOption.implicitHeaderEncode == 1) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_CODE_OPTION, CODEOPT_ENC_HEADER_IMPLICIT | CODEOPT_ENC_VCL | // implicitly encode a header(headers) for generating bitstream. (to encode a header only, use ENC_PUT_VIDEO_HEADER for GiveCommand)
                                                        (option->codeOption.encodeAUD<<5)              |
                                                        (option->codeOption.encodeEOS<<6)              |
                                                        (option->codeOption.encodeEOB<<7));
    }
    else {
        VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_CODE_OPTION, (option->codeOption.implicitHeaderEncode<<0)   |
                                                         (option->codeOption.encodeVCL<<1)              |
                                                         (option->codeOption.encodeVPS<<2)              |
                                                         (option->codeOption.encodeSPS<<3)              |
                                                         (option->codeOption.encodePPS<<4)              |
                                                         (option->codeOption.encodeAUD<<5)              |
                                                         (option->codeOption.encodeEOS<<6)              |
                                                         (option->codeOption.encodeEOB<<7));
    }

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_PIC_PARAM,  (option->skipPicture<<0)         |
                                                    (option->forcePicQpEnable<<1)    |
                                                    (option->forcePicQpI<<2)         |
                                                    (option->forcePicQpP<<8)         |
                                                    (option->forcePicQpB<<14)        |
                                                    (option->forcePicTypeEnable<<20) |
                                                    (option->forcePicType<<21)       |
                                                    (option->forceAllCtuCoefDropEnable<<24));

    if (option->srcEndFlag == 1)
        VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_PIC_IDX, 0xFFFFFFFF);               // no more source image.
    else
        VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_PIC_IDX, option->srcIdx);

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_ADDR_Y, pSrcFrame->bufY);
    if (pOpenParam->cbcrOrder == CBCR_ORDER_NORMAL) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_ADDR_U, pSrcFrame->bufCb);
        VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_ADDR_V, pSrcFrame->bufCr);
    }
    else {
        VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_ADDR_U, pSrcFrame->bufCr);
        VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_ADDR_V, pSrcFrame->bufCb);
    }


    switch (pOpenParam->srcFormat) {
    case FORMAT_420:
    case FORMAT_422:
    case FORMAT_YUYV:
    case FORMAT_YVYU:
    case FORMAT_UYVY:
    case FORMAT_VYUY:
        justified  = WTL_LEFT_JUSTIFIED;
        formatNo   = WTL_PIXEL_8BIT;
        srcStrideC = (pSrcFrame->cbcrInterleave == 1) ? pSrcFrame->stride : (pSrcFrame->stride/2);
        srcStrideC = (pOpenParam->srcFormat == FORMAT_422) ? srcStrideC*2 : srcStrideC;
        break;
    case FORMAT_420_P10_16BIT_MSB:
    case FORMAT_422_P10_16BIT_MSB:
    case FORMAT_YUYV_P10_16BIT_MSB:
    case FORMAT_YVYU_P10_16BIT_MSB:
    case FORMAT_UYVY_P10_16BIT_MSB:
    case FORMAT_VYUY_P10_16BIT_MSB:
        justified  = WTL_RIGHT_JUSTIFIED;
        formatNo   = WTL_PIXEL_16BIT;
        srcStrideC = (pSrcFrame->cbcrInterleave == 1) ? pSrcFrame->stride : (pSrcFrame->stride/2);
        srcStrideC = (pOpenParam->srcFormat == FORMAT_422_P10_16BIT_MSB) ? srcStrideC*2 : srcStrideC;
        break;
    case FORMAT_420_P10_16BIT_LSB:
    case FORMAT_422_P10_16BIT_LSB:
    case FORMAT_YUYV_P10_16BIT_LSB:
    case FORMAT_YVYU_P10_16BIT_LSB:
    case FORMAT_UYVY_P10_16BIT_LSB:
    case FORMAT_VYUY_P10_16BIT_LSB:
        justified  = WTL_LEFT_JUSTIFIED;
        formatNo   = WTL_PIXEL_16BIT;
        srcStrideC = (pSrcFrame->cbcrInterleave == 1) ? pSrcFrame->stride : (pSrcFrame->stride/2);
        srcStrideC = (pOpenParam->srcFormat == FORMAT_422_P10_16BIT_LSB) ? srcStrideC*2 : srcStrideC;
        break;
    case FORMAT_420_P10_32BIT_MSB:
    case FORMAT_422_P10_32BIT_MSB:
    case FORMAT_YUYV_P10_32BIT_MSB:
    case FORMAT_YVYU_P10_32BIT_MSB:
    case FORMAT_UYVY_P10_32BIT_MSB:
    case FORMAT_VYUY_P10_32BIT_MSB:
        justified  = WTL_RIGHT_JUSTIFIED;
        formatNo   = WTL_PIXEL_32BIT;
        srcStrideC = (pSrcFrame->cbcrInterleave == 1) ? pSrcFrame->stride : VPU_ALIGN16(pSrcFrame->stride/2)*(1<<pSrcFrame->cbcrInterleave);
        srcStrideC = (pOpenParam->srcFormat == FORMAT_422_P10_32BIT_MSB) ? srcStrideC*2 : srcStrideC;
        break;
    case FORMAT_420_P10_32BIT_LSB:
    case FORMAT_422_P10_32BIT_LSB:
    case FORMAT_YUYV_P10_32BIT_LSB:
    case FORMAT_YVYU_P10_32BIT_LSB:
    case FORMAT_UYVY_P10_32BIT_LSB:
    case FORMAT_VYUY_P10_32BIT_LSB:
        justified  = WTL_LEFT_JUSTIFIED;
        formatNo   = WTL_PIXEL_32BIT;
        srcStrideC = (pSrcFrame->cbcrInterleave == 1) ? pSrcFrame->stride : VPU_ALIGN16(pSrcFrame->stride/2)*(1<<pSrcFrame->cbcrInterleave);
        srcStrideC = (pOpenParam->srcFormat == FORMAT_422_P10_32BIT_LSB) ? srcStrideC*2 : srcStrideC;
        break;
    default:
        return RETCODE_FAILURE;
    }

    srcFrameFormat = (pOpenParam->cbcrInterleave<<1) | (pOpenParam->nv21);
    switch (pOpenParam->packedFormat) {
    case PACKED_YUYV: srcFrameFormat = 4; break;
    case PACKED_YVYU: srcFrameFormat = 5; break;
    case PACKED_UYVY: srcFrameFormat = 6; break;
    case PACKED_VYUY: srcFrameFormat = 7; break;
    default: break;
    }

    regVal = vdi_convert_endian(coreIdx, pOpenParam->sourceEndian);
    bsEndian = (~regVal&VDI_128BIT_ENDIAN_MASK);

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_STRIDE, (pSrcFrame->stride<<16) | srcStrideC);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_FORMAT, (srcFrameFormat<<0) |
                                                    (formatNo<<3)       |
                                                    (justified<<5)      |
                                                    (bsEndian<<6));

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_CUSTOM_MAP_OPTION_ADDR, option->customMapOpt.addrCustomMap);

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_CUSTOM_MAP_OPTION_PARAM,  (option->customMapOpt.customRoiMapEnable << 0)    |
                                                                  (option->customMapOpt.roiAvgQp << 1)              |
                                                                  (option->customMapOpt.customLambdaMapEnable<< 8)  |
                                                                  (option->customMapOpt.customModeMapEnable<< 9)    |
                                                                  (option->customMapOpt.customCoefDropEnable<< 10));

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_LONGTERM_PIC, (option->useCurSrcAsLongtermPic<<0) | (option->useLongtermRef<<1));

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_WP_PIXEL_SIGMA_Y,  option->wpPixSigmaY);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_WP_PIXEL_SIGMA_C, (option->wpPixSigmaCr<<16) | option->wpPixSigmaCb);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_WP_PIXEL_MEAN_Y, option->wpPixMeanY);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_WP_PIXEL_MEAN_C, (option->wpPixMeanCr<<16) | (option->wpPixMeanCb));

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_S2_SEARCH_CENTER, (option->s2SearchCenterEnable<<16) | ((option->s2SearchCenterYDiv4&0xff)<<8) | (option->s2SearchCenterXDiv4&0xff));


    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_PREFIX_SEI_INFO, ((pEncInfo->prefixSeiDataSize << 16) | (pEncInfo->prefixSeiNalEnable&0x01)));
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_PREFIX_SEI_NAL_ADDR, pEncInfo->prefixSeiNalAddr);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SUFFIX_SEI_INFO, ((pEncInfo->suffixSeiDataSize << 16) | (pEncInfo->suffixSeiNalEnable&0x01)));
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SUFFIX_SEI_NAL_ADDR, pEncInfo->suffixSeiNalAddr);

#ifdef ENABLE_HOST_RC /// ML
    /*	W5_CMD_ENC_PIC_MOTION_LEVEL (0x19c)
            [31: 0] : motion level
    */
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_MOTION_LEVEL, option->picMotionLevel);
#endif /// ENABLE_HOST_RC

    Wave5BitIssueCommand(instance, W5_ENC_PIC);

    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, instance->instIndex, W5_ENC_PIC, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pEncInfo->instanceQueueCount = (regVal>>16)&0xff;
    pEncInfo->reportQueueCount   = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {           // FAILED for adding a command into VCPU QUEUE
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(INFO, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_QUEUEING_FAIL) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_FAIL_REASON);
            VLOG(INFO, "QUEUE_FAIL_REASON = 0x%x\n", regVal);
            return RETCODE_QUEUEING_FAILURE;
        }
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            return RETCODE_VPU_BUS_ERROR;
        else
            return RETCODE_FAILURE;
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuEncGetResult(CodecInst* instance, EncOutputInfo* result)
{
    RetCode              ret = RETCODE_SUCCESS;
    Uint32               encodingSuccess;
    Uint32               regVal;
    Int32                coreIdx;
    EncInfo*             pEncInfo = VPU_HANDLE_TO_ENCINFO(instance);
    vpu_instance_pool_t* instancePool = NULL;

    coreIdx = instance->coreIdx;

    ret = SendQuery(instance, GET_RESULT);
    if (ret != RETCODE_SUCCESS) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(INFO, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_RESULT_NOT_READY)
            return RETCODE_REPORT_NOT_READY;
        else if(regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW)
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT)
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            return RETCODE_VPU_BUS_ERROR;
        else
            return RETCODE_QUERY_FAILURE;
    }
    if (instance->loggingEnable)
        vdi_log(coreIdx, instance->instIndex, W5_ENC_PIC, 0);

    regVal = VpuReadReg(coreIdx, W5_RET_QUEUE_STATUS);

    pEncInfo->instanceQueueCount = (regVal>>16)&0xff;
    pEncInfo->reportQueueCount   = (regVal & 0xffff);

    encodingSuccess = VpuReadReg(coreIdx, W5_RET_ENC_ENCODING_SUCCESS);
    if (encodingSuccess == FALSE) {
        result->errorReason = VpuReadReg(coreIdx, W5_RET_ENC_ERR_INFO);
        if (result->errorReason == WAVE5_SYSERR_VLC_BUF_FULL) {
            return RETCODE_VLC_BUF_FULL;
        }
        return RETCODE_FAILURE;
    } else {
        result->warnInfo = VpuReadReg(instance->coreIdx, W5_RET_ENC_WARN_INFO);
    }

    result->encPicCnt       = VpuReadReg(coreIdx, W5_RET_ENC_PIC_NUM);
    regVal= VpuReadReg(coreIdx, W5_RET_ENC_PIC_TYPE);
    result->picType         = regVal & 0xFFFF;

    result->encVclNut       = VpuReadReg(coreIdx, W5_RET_ENC_VCL_NUT);
    result->reconFrameIndex = VpuReadReg(coreIdx, W5_RET_ENC_PIC_IDX);

    if (result->reconFrameIndex >= 0)
        result->reconFrame  = pEncInfo->frameBufPool[result->reconFrameIndex];

    if (pEncInfo->openParam.EncStdParam.waveParam.enReportNalSize == TRUE)
        result->reportNalSizeAddr = VpuReadReg(coreIdx, W5_RET_ENC_NAL_SIZE_REPORT_BASE);
    result->numOfSlices     = VpuReadReg(coreIdx, W5_RET_ENC_PIC_SLICE_NUM);
    result->picSkipped      = VpuReadReg(coreIdx, W5_RET_ENC_PIC_SKIP);
    result->numOfIntra      = VpuReadReg(coreIdx, W5_RET_ENC_PIC_NUM_INTRA);
    result->numOfMerge      = VpuReadReg(coreIdx, W5_RET_ENC_PIC_NUM_MERGE);
    result->numOfSkipBlock  = VpuReadReg(coreIdx, W5_RET_ENC_PIC_NUM_SKIP);
    result->bitstreamWrapAround = 0;    // only support line-buffer mode.

    result->avgCtuQp        = VpuReadReg(coreIdx, W5_RET_ENC_PIC_AVG_CTU_QP);
    result->encPicByte      = VpuReadReg(coreIdx, W5_RET_ENC_PIC_BYTE);

    result->encGopPicIdx    = VpuReadReg(coreIdx, W5_RET_ENC_GOP_PIC_IDX);
    result->encPicPoc       = VpuReadReg(coreIdx, W5_RET_ENC_PIC_POC);
    result->encSrcIdx       = VpuReadReg(coreIdx, W5_RET_ENC_USED_SRC_IDX);
    result->releaseSrcFlag  = VpuReadReg(coreIdx, W5_RET_ENC_SRC_BUF_FLAG);
    pEncInfo->streamWrPtr   = VpuReadReg(coreIdx, pEncInfo->streamWrPtrRegAddr);
    pEncInfo->streamRdPtr   = VpuReadReg(coreIdx, pEncInfo->streamRdPtrRegAddr);

    result->picDistortionLow    = VpuReadReg(coreIdx, W5_RET_ENC_PIC_DIST_LOW);
    result->picDistortionHigh   = VpuReadReg(coreIdx, W5_RET_ENC_PIC_DIST_HIGH);

    result->bitstreamBuffer = VpuReadReg(coreIdx, pEncInfo->streamRdPtrRegAddr);
    result->bitstreamBuffer = VPU_MapToAddr40Bit(coreIdx, result->bitstreamBuffer);
    result->rdPtr = VPU_MapToAddr40Bit(coreIdx, result->rdPtr);
    result->wrPtr = VPU_MapToAddr40Bit(coreIdx, result->wrPtr);

    if (result->reconFrameIndex == RECON_IDX_FLAG_HEADER_ONLY) //result for header only(no vcl) encoding
        result->bitstreamSize   = result->encPicByte;
    else if (result->reconFrameIndex < 0)
        result->bitstreamSize   = 0;
    else
        result->bitstreamSize   = result->encPicByte;

    vdi_stat_fps(coreIdx, instance->instIndex, result->picType);

    result->encHostCmdTick             = VpuReadReg(coreIdx, W5_RET_ENC_HOST_CMD_TICK);
    result->encPrepareStartTick        = VpuReadReg(coreIdx, W5_RET_ENC_PREPARE_START_TICK);
    result->encPrepareEndTick          = VpuReadReg(coreIdx, W5_RET_ENC_PREPARE_END_TICK);
    result->encProcessingStartTick     = VpuReadReg(coreIdx, W5_RET_ENC_PROCESSING_START_TICK);
    result->encProcessingEndTick       = VpuReadReg(coreIdx, W5_RET_ENC_PROCESSING_END_TICK);
    result->encEncodeStartTick         = VpuReadReg(coreIdx, W5_RET_ENC_ENCODING_START_TICK);
    result->encEncodeEndTick           = VpuReadReg(coreIdx, W5_RET_ENC_ENCODING_END_TICK);

    instancePool = vdi_get_instance_pool(instance->coreIdx);
    if (instancePool) {
        if ( pEncInfo->firstCycleCheck == FALSE ) {
            result->frameCycle   = (result->encEncodeEndTick - result->encHostCmdTick) * pEncInfo->cyclePerTick;
            pEncInfo->firstCycleCheck = TRUE;
        }
        else {
            result->frameCycle   = (result->encEncodeEndTick - instancePool->lastPerformanceCycles) * pEncInfo->cyclePerTick;
            if (instancePool->lastPerformanceCycles < result->encHostCmdTick)
                result->frameCycle   = (result->encEncodeEndTick - result->encHostCmdTick) * pEncInfo->cyclePerTick;
        }
        instancePool->lastPerformanceCycles = result->encEncodeEndTick;
    }
    result->prepareCycle = (result->encPrepareEndTick    - result->encPrepareStartTick) * pEncInfo->cyclePerTick;
    result->processing   = (result->encProcessingEndTick - result->encProcessingStartTick) * pEncInfo->cyclePerTick;
    result->EncodedCycle = (result->encEncodeEndTick     - result->encEncodeStartTick) * pEncInfo->cyclePerTick;
    vdi_stat_hwcycles(coreIdx, result->processing);
    return RETCODE_SUCCESS;
}

RetCode Wave5VpuEncGetHeader(EncHandle instance, EncHeaderParam * encHeaderParam)
{
    Int32 coreIdx;
    Uint32 regVal = 0;
    EncInfo* pEncInfo = VPU_HANDLE_TO_ENCINFO(instance);

    coreIdx = instance->coreIdx;


    VpuWriteReg(coreIdx, W5_CMD_ENC_BS_START_ADDR, encHeaderParam->buf);
    VpuWriteReg(coreIdx, W5_CMD_ENC_BS_SIZE, encHeaderParam->size);
    pEncInfo->streamRdPtr = VPU_MapToAddr40Bit(coreIdx, encHeaderParam->buf);
    pEncInfo->streamWrPtr = VPU_MapToAddr40Bit(coreIdx, encHeaderParam->buf);
    pEncInfo->streamBufStartAddr = VPU_MapToAddr40Bit(coreIdx, encHeaderParam->buf);
    pEncInfo->streamBufSize = encHeaderParam->size;
    pEncInfo->streamBufEndAddr = VPU_MapToAddr40Bit(coreIdx, encHeaderParam->buf + encHeaderParam->size);

    /* Secondary AXI */
    regVal  = (pEncInfo->secAxiInfo.u.wave.useEncRdoEnable << 11) | (pEncInfo->secAxiInfo.u.wave.useEncLfEnable << 15);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_USE_SEC_AXI, regVal);

    regVal = (pEncInfo->openParam.EncStdParam.waveParam.enReportNalSize<<2);
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_REPORT_PARAM, regVal);

    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_CODE_OPTION, (encHeaderParam->headerType) | (encHeaderParam->encodeAUD<<5));
    VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_SRC_PIC_IDX, 0);

#ifdef ENABLE_HOST_RC
	VpuWriteReg(coreIdx, W5_CMD_ENC_PIC_MOTION_LEVEL, -1); // MOTION_LEVEL in Wave5VpuEncGetHeader() is not valid.
#endif /// ENABLE_HOST_RC

    Wave5BitIssueCommand(instance, W5_ENC_PIC);

    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {   // Check QUEUE_DONE
        if (instance->loggingEnable)
            vdi_log(instance->coreIdx, instance->instIndex, W5_ENC_PIC, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_STATUS);

    pEncInfo->instanceQueueCount = (regVal>>16)&0xff;
    pEncInfo->reportQueueCount   = (regVal & 0xffff);

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {           // FAILED for adding a command into VCPU QUEUE
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_QUEUEING_FAIL) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_FAIL_REASON);
            VLOG(INFO, "QUEUE_FAIL_REASON = 0x%x\n", regVal);
            return RETCODE_QUEUEING_FAILURE;
        }
        else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW) {
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        }
        else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        }
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT) {
            return RETCODE_VPU_BUS_ERROR;
        }
        else {
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuEncFiniSeq(CodecInst*  instance )
{
    RetCode ret = RETCODE_SUCCESS;

    Wave5BitIssueCommand(instance, W5_DESTROY_INSTANCE);
    if (vdi_wait_vpu_busy(instance->coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1)
        return RETCODE_VPU_RESPONSE_TIMEOUT;

    if (VpuReadReg(instance->coreIdx, W5_RET_SUCCESS) == FALSE) {
        Uint32 regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal == WAVE5_SYSERR_VPU_STILL_RUNNING)
            ret = RETCODE_VPU_STILL_RUNNING;
        else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT)
            ret = RETCODE_VPU_BUS_ERROR;
        else
            ret = RETCODE_FAILURE;
    }
    return ret;
}

RetCode Wave5VpuEncGiveCommand(CodecInst* instance, CodecCommand cmd, void* param)
{
    RetCode  ret      = RETCODE_SUCCESS;
    EncInfo* pEncInfo = VPU_HANDLE_TO_ENCINFO(instance);
    EncOpenParam* pOpenParam;

    if (param == NULL)
        return RETCODE_INVALID_PARAM;

    pOpenParam = &pEncInfo->openParam;
    switch (cmd) {
    case ENC_GET_QUEUE_STATUS:
        {
            QueueStatusInfo* queueInfo = (QueueStatusInfo*)param;
            queueInfo->instanceQueueCount = pEncInfo->instanceQueueCount;
            queueInfo->reportQueueCount   = pEncInfo->reportQueueCount;
            queueInfo->instanceQueueFull  = (pEncInfo->instanceQueueCount == pOpenParam->cmdQueueDepth) ? TRUE : FALSE;
            queueInfo->reportQueueEmpty   = (pEncInfo->reportQueueCount == 0) ? TRUE : FALSE;
        }
        break;
    case ENC_SET_SUB_FRAME_SYNC:
        {
            EncSubFrameSyncState* pSubFrameSyncState = (EncSubFrameSyncState*)param;
            if (instance->productId != PRODUCT_ID_521)
                return RETCODE_INVALID_COMMAND;

            VpuWriteReg(instance->coreIdx, W5_ENC_PIC_SUB_FRAME_SYNC_IF, (pSubFrameSyncState->ipuCurFrameIndex<<2) | ((pSubFrameSyncState->ipuNewFrame&0x01)<<1) | (pSubFrameSyncState->ipuEndOfRow&0x01));
        }
        break;
    default:
        ret = RETCODE_NOT_SUPPORTED_FEATURE;
        break;
    }

    return ret;
}

RetCode Wave5VpuEncParaChange(EncHandle instance, EncChangeParam* param)
{
    Int32 coreIdx;
    Uint32 regVal = 0;
    EncInfo* pEncInfo;
    EncWave5Param *Wave5param;
    coreIdx  = instance->coreIdx;
    pEncInfo = &instance->CodecInfo->encInfo;
    Wave5param = &pEncInfo->openParam.EncStdParam.waveParam;


    /* SET_PARAM + COMMON */
    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_SET_PARAM_OPTION, W5_SET_PARAM_OPT_CHANGE_PARAM);
    VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_SET_PARAM_ENABLE, param->enable_option);

    if (param->enable_option & W5_ENC_CHANGE_PARAM_PPS) {
        regVal = (param->constIntraPredFlag<<1)         |
                 (param->lfCrossSliceBoundaryEnable<<2) |
                 (param->weightPredEnable<<3)           |
                 (param->disableDeblk<<5)               |
                 ((param->betaOffsetDiv2&0xF)<<6)       |
                 ((param->tcOffsetDiv2&0xF)<<10)        |
                 ((param->chromaCbQpOffset&0x1F)<<14)   |
                 ((param->chromaCrQpOffset&0x1F)<<19)   |
                 (param->transform8x8Enable<<29)        |
                 (param->entropyCodingMode<<30);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_PPS_PARAM, regVal);
    }
    // restore old period
    if (param->intraPeriod == -1 || param->avcIdrPeriod == -1) {
        param->intraPeriod = Wave5param->intraPeriod;
        param->avcIdrPeriod = Wave5param->avcIdrPeriod;
    }

    VLOG(INFO, "intraPeriod:%d avcIdrPeriod:%d\n", param->intraPeriod, param->avcIdrPeriod);

    if (param->enable_option & W5_ENC_CHANGE_PARAM_INTRA_PARAM) {
        if (instance->codecMode == W_AVC_ENC) {
            VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INTRA_PARAM, (param->intraQP<<0) | ( (param->intraPeriod&0x7ff)<<6) | ( (param->avcIdrPeriod&0x7ff)<<17) | ( (param->forcedIdrHeaderEnable&3)<<28));
        }
        else {
            VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INTRA_PARAM, (param->intraQP<<3) | (param->forcedIdrHeaderEnable<<9) | (param->intraPeriod<<16));
        }
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_RC_FRAME_RATE) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_FRAME_RATE, param->frameRate);
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_RC_TARGET_RATE) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_TARGET_RATE, param->bitRate);
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_RC) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_PARAM,   (param->hvsQPEnable<<2)             |
                                                        (param->hvsQpScale<<4)              |
                                                        (param->vbvBufferSize<<20));
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_RC_MIN_MAX_QP) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_MIN_MAX_QP, (param->minQpI<<0)                   |
                                                           (param->maxQpI<<6)                   |
                                                           (param->hvsMaxDeltaQp<<12));
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_RC_INTER_MIN_MAX_QP) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_INTER_MIN_MAX_QP, (param->minQpP<<0) |
                                                                 (param->maxQpP<<6) |
                                                                 (param->minQpB<<12) |
                                                                 (param->maxQpB<<18));
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_RC_BIT_RATIO_LAYER) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_BIT_RATIO_LAYER_0_3, (param->fixedBitRatio[0]<<0)  |
                                                                    (param->fixedBitRatio[1]<<8)  |
                                                                    (param->fixedBitRatio[2]<<16) |
                                                                    (param->fixedBitRatio[3]<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_BIT_RATIO_LAYER_4_7, (param->fixedBitRatio[4]<<0)  |
                                                                    (param->fixedBitRatio[5]<<8)  |
                                                                    (param->fixedBitRatio[6]<<16) |
                                                                    (param->fixedBitRatio[7]<<24));
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_VUI_HRD_PARAM) {
        regVal = (param->hrdRbspDataSize<<18)   |
                 (param->vuiRbspDataSize<<4)    |
                 (param->encodeHrdRbspInVPS<<2) |
                 (param->encodeVuiRbsp);

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_VUI_HRD_PARAM, regVal);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_VUI_RBSP_ADDR, param->vuiRbspDataAddr);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_HRD_RBSP_ADDR, param->hrdRbspDataAddr);
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_RC_WEIGHT) {
        regVal  = (param->rcWeightBuf<<8 | param->rcWeightParam);
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RC_WEIGHT_PARAM, regVal);
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_RDO) {
        regVal  = (param->rdoSkip<<2)               |
                  (param->lambdaScalingEnable<<3)   |
                  (param->coefClearDisable<<4)      |
                  (param->intraNxNEnable<<8)        |
                  (param->maxNumMerge<<18)          |
                  (param->customMDEnable<<20)       |
                  (param->customLambdaEnable<<21);

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_RDO_PARAM, regVal);
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_INDEPEND_SLICE) {
        if (instance->codecMode == W_HEVC_ENC ) {
            VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INDEPENDENT_SLICE, param->independSliceModeArg<<16 | param->independSliceMode);
        }
        else if (instance->codecMode == W_AVC_ENC) {
            VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_INDEPENDENT_SLICE, param->avcSliceArg<<16 | param->avcSliceMode);
        }
    }

    if (instance->codecMode == W_HEVC_ENC && param->enable_option & W5_ENC_CHANGE_PARAM_DEPEND_SLICE) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_DEPENDENT_SLICE, param->dependSliceModeArg<<16 | param->dependSliceMode);
    }

    if (instance->codecMode == W_HEVC_ENC && param->enable_option & W5_ENC_CHANGE_PARAM_NR) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_NR_PARAM,   (param->nrYEnable<<0)       |
                                                        (param->nrCbEnable<<1)      |
                                                        (param->nrCrEnable<<2)      |
                                                        (param->nrNoiseEstEnable<<3)|
                                                        (param->nrNoiseSigmaY<<4)   |
                                                        (param->nrNoiseSigmaCb<<12) |
                                                        (param->nrNoiseSigmaCr<<20));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_NR_WEIGHT,  (param->nrIntraWeightY<<0)  |
                                                        (param->nrIntraWeightCb<<5) |
                                                        (param->nrIntraWeightCr<<10)|
                                                        (param->nrInterWeightY<<15) |
                                                        (param->nrInterWeightCb<<20)|
                                                        (param->nrInterWeightCr<<25));
    }

    if (param->enable_option & W5_ENC_CHANGE_PARAM_BG) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_BG_PARAM, (param->bgThrDiff<<1)         |
                                                      (param->bgThrMeanDiff<<10)    |
                                                      (param->bgLambdaQp<<18)       |
                                                      ((param->bgDeltaQp&0x1F)<<24) |
                                                      (instance->codecMode == W_AVC_ENC ? param->s2fmeDisable<<29 : 0));
    }
    if (instance->codecMode == W_HEVC_ENC && param->enable_option & W5_ENC_CHANGE_PARAM_CUSTOM_MD) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_PU04, (param->pu04DeltaRate&0xFF)                 |
                                                            ((param->pu04IntraPlanarDeltaRate&0xFF)<<8) |
                                                            ((param->pu04IntraDcDeltaRate&0xFF)<<16)    |
                                                            ((param->pu04IntraAngleDeltaRate&0xFF)<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_PU08, (param->pu08DeltaRate&0xFF)                 |
                                                            ((param->pu08IntraPlanarDeltaRate&0xFF)<<8) |
                                                            ((param->pu08IntraDcDeltaRate&0xFF)<<16)    |
                                                            ((param->pu08IntraAngleDeltaRate&0xFF)<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_PU16, (param->pu16DeltaRate&0xFF)                 |
                                                            ((param->pu16IntraPlanarDeltaRate&0xFF)<<8) |
                                                            ((param->pu16IntraDcDeltaRate&0xFF)<<16)    |
                                                            ((param->pu16IntraAngleDeltaRate&0xFF)<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_PU32, (param->pu32DeltaRate&0xFF)                 |
                                                            ((param->pu32IntraPlanarDeltaRate&0xFF)<<8) |
                                                            ((param->pu32IntraDcDeltaRate&0xFF)<<16)    |
                                                            ((param->pu32IntraAngleDeltaRate&0xFF)<<24));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_CU08, (param->cu08IntraDeltaRate&0xFF)        |
                                                            ((param->cu08InterDeltaRate&0xFF)<<8)   |
                                                            ((param->cu08MergeDeltaRate&0xFF)<<16));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_CU16, (param->cu16IntraDeltaRate&0xFF)        |
                                                            ((param->cu16InterDeltaRate&0xFF)<<8)   |
                                                            ((param->cu16MergeDeltaRate&0xFF)<<16));

        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_MD_CU32, (param->cu32IntraDeltaRate&0xFF)        |
                                                            ((param->cu32InterDeltaRate&0xFF)<<8)   |
                                                            ((param->cu32MergeDeltaRate&0xFF)<<16));
    }

    if (instance->codecMode == W_HEVC_ENC && param->enable_option & W5_ENC_CHANGE_PARAM_CUSTOM_LAMBDA) {
        VpuWriteReg(coreIdx, W5_CMD_ENC_SEQ_CUSTOM_LAMBDA_ADDR, param->customLambdaAddr);
    }


    Wave5BitIssueCommand(instance, W5_ENC_SET_PARAM);

    if (vdi_wait_vpu_busy(coreIdx, __VPU_BUSY_TIMEOUT, W5_VPU_BUSY_STATUS) == -1) {
        if (instance->loggingEnable)
            vdi_log(coreIdx, instance->instIndex, W5_ENC_SET_PARAM, 2);
        return RETCODE_VPU_RESPONSE_TIMEOUT;
    }

    regVal = VpuReadReg(coreIdx, W5_RET_QUEUE_STATUS);

    pEncInfo->instanceQueueCount = (regVal>>16) & 0xFF;
    pEncInfo->reportQueueCount   = (regVal & 0xFFFF);

    if (VpuReadReg(coreIdx, W5_RET_SUCCESS) == 0) {
        regVal = VpuReadReg(instance->coreIdx, W5_RET_FAIL_REASON);
        if (regVal != WAVE5_SYSERR_QUEUEING_FAIL)
            VLOG(ERR, "FAIL_REASON = 0x%x\n", regVal);

        if (regVal == WAVE5_SYSERR_QUEUEING_FAIL) {
            regVal = VpuReadReg(instance->coreIdx, W5_RET_QUEUE_FAIL_REASON);
            VLOG(INFO, "QUEUE_FAIL_REASON = 0x%x\n", regVal);
            return RETCODE_QUEUEING_FAILURE;
        } else if (regVal == WAVE5_SYSERR_ACCESS_VIOLATION_HW) {
            return RETCODE_MEMORY_ACCESS_VIOLATION;
        } else if (regVal == WAVE5_SYSERR_WATCHDOG_TIMEOUT) {
            return RETCODE_VPU_RESPONSE_TIMEOUT;
        } else if (regVal == WAVE5_SYSERR_BUS_ERROR || regVal == WAVE5_SYSERR_DOUBLE_FAULT) {
            return RETCODE_VPU_BUS_ERROR;
        } else {
            return RETCODE_FAILURE;
        }
    }

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuEncGetSrcBufFlag(CodecInst* instance, Uint32* flag) {

    RetCode ret = RETCODE_SUCCESS;

    ret = SendQuery(instance, GET_SRC_BUF_FLAG);

    if (ret != RETCODE_SUCCESS)
        return RETCODE_QUERY_FAILURE;

    *flag = VpuReadReg(instance->coreIdx, W5_RET_ENC_SRC_BUF_FLAG);

    return RETCODE_SUCCESS;
}

RetCode Wave5VpuEncCheckCommonParamValid(EncOpenParam* pop)
{
    RetCode ret = RETCODE_SUCCESS;
    Int32   low_delay = 0, i = 0;
    EncWave5Param* param = &pop->EncStdParam.waveParam;

    // check low-delay gop structure
    if(param->gopPresetIdx == PRESET_IDX_CUSTOM_GOP)  // common gop
    {
        Int32 minVal = 0;

        low_delay = 1;
        if(param->gopParam.customGopSize > 1)
        {
            minVal = param->gopParam.picParam[0].pocOffset;

            for(i = 1; i < param->gopParam.customGopSize; i++)
            {
                if(minVal > param->gopParam.picParam[i].pocOffset)
                {
                    low_delay = 0;
                    break;
                }
                else
                    minVal = param->gopParam.picParam[i].pocOffset;
            }
        }
    }
    else if(param->gopPresetIdx == PRESET_IDX_ALL_I ||
            param->gopPresetIdx == PRESET_IDX_IPP   ||
            param->gopPresetIdx == PRESET_IDX_IBBB  ||
            param->gopPresetIdx == PRESET_IDX_IPPPP ||
            param->gopPresetIdx == PRESET_IDX_IBBBB ||
            param->gopPresetIdx == PRESET_IDX_IPP_SINGLE ) // low-delay case (IPPP, IBBB)
        low_delay = 1;

    if (pop->bitstreamFormat == STD_HEVC) {
        if (low_delay && param->decodingRefreshType == 1) {
            VLOG(WARN,"CFG WARN : DecodingRefreshType (CRA) is supported in case of a low delay GOP.\n");
            VLOG(WARN,"RECOMMEND CONFIG PARAMETER : Decoding refresh type = IDR\n");
            param->decodingRefreshType = 2;
        }
    }

    if (param->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
        for(i = 0; i < param->gopParam.customGopSize; i++)
        {
            if (param->gopParam.picParam[i].temporalId >= MAX_NUM_TEMPORAL_LAYER )
            {
                VLOG(ERR,"CFG FAIL : temporalId %d exceeds MAX_NUM_TEMPORAL_LAYER\n", param->gopParam.picParam[i].temporalId);
                VLOG(ERR,"RECOMMEND CONFIG PARAMETER : Adjust temporal ID under MAX_NUM_TEMPORAL_LAYER(7) in GOP structure\n");
                ret = RETCODE_FAILURE;
            }

            if (param->gopParam.picParam[i].temporalId < 0)
            {
                VLOG(ERR,"CFG FAIL : Must be %d-th temporal_id >= 0\n",param->gopParam.picParam[i].temporalId);
                VLOG(ERR,"RECOMMEND CONFIG PARAMETER : Adjust temporal layer above '0' in GOP structure\n");
                ret = RETCODE_FAILURE;
            }
        }
    }

    // Slice
    {
        if ( param->wppEnable == 1 && param->independSliceMode == 1) {
            int num_ctb_in_width = VPU_ALIGN64(pop->picWidth)>>6;
            if (param->independSliceModeArg % num_ctb_in_width) {
                VLOG(ERR, "CFG FAIL : If WaveFrontSynchro(WPP) '1', the number of IndeSliceArg(ctb_num) must be multiple of num_ctb_in_width\n");
                VLOG(ERR, "RECOMMEND CONFIG PARAMETER : IndeSliceArg = num_ctb_in_width * a\n");
                ret = RETCODE_FAILURE;
            }
        }

        // multi-slice & wpp
        if(param->wppEnable == 1 && param->dependSliceMode != 0) {
            VLOG(ERR,"CFG FAIL : If WaveFrontSynchro(WPP) '1', the option of multi-slice must be '0'\n");
            VLOG(ERR,"RECOMMEND CONFIG PARAMETER : independSliceMode = 0, dependSliceMode = 0\n");
            ret = RETCODE_FAILURE;
        }

        if(param->independSliceMode == 0 && param->dependSliceMode != 0)
        {
            VLOG(ERR,"CFG FAIL : If independSliceMode is '0', dependSliceMode must be '0'\n");
            VLOG(ERR,"RECOMMEND CONFIG PARAMETER : independSliceMode = 1, independSliceModeArg = TotalCtuNum\n");
            ret = RETCODE_FAILURE;
        }
        else if((param->independSliceMode == 1) && (param->dependSliceMode == 1))
        {
            if(param->independSliceModeArg < param->dependSliceModeArg)
            {
                VLOG(ERR,"CFG FAIL : If independSliceMode & dependSliceMode is both '1' (multi-slice with ctu count), must be independSliceModeArg >= dependSliceModeArg\n");
                VLOG(ERR,"RECOMMEND CONFIG PARAMETER : dependSliceMode = 0\n");
                ret = RETCODE_FAILURE;
            }
        }

        if (param->independSliceMode != 0)
        {
            if (param->independSliceModeArg > 65535)
            {
                VLOG(ERR,"CFG FAIL : If independSliceMode is not 0, must be independSliceModeArg <= 0xFFFF\n");
                ret = RETCODE_FAILURE;
            }
        }

        if (param->dependSliceMode != 0)
        {
            if (param->dependSliceModeArg > 65535)
            {
                VLOG(ERR,"CFG FAIL : If dependSliceMode is not 0, must be dependSliceModeArg <= 0xFFFF\n");
                ret = RETCODE_FAILURE;
            }
        }
    }

    if (param->confWin.top % 2) {
        VLOG(ERR, "CFG FAIL : conf_win_top : %d value is not available. The value should be equal to multiple of 2.\n", param->confWin.top);
        ret = RETCODE_FAILURE;
    }

    if (param->confWin.bottom % 2) {
        VLOG(ERR, "CFG FAIL : conf_win_bot : %d value is not available. The value should be equal to multiple of 2.\n", param->confWin.bottom);
        ret = RETCODE_FAILURE;
    }

    if (param->confWin.left % 2) {
        VLOG(ERR, "CFG FAIL : conf_win_left : %d value is not available. The value should be equal to multiple of 2.\n", param->confWin.left);
        ret = RETCODE_FAILURE;
    }

    if (param->confWin.right % 2) {
        VLOG(ERR, "CFG FAIL : conf_win_right : %d value is not available. The value should be equal to multiple of 2.\n", param->confWin.right);
        ret = RETCODE_FAILURE;
    }

    if (param->losslessEnable && (param->nrYEnable || param->nrCbEnable || param->nrCrEnable)) {
        VLOG(ERR, "CFG FAIL : LosslessCoding and NoiseReduction (EnNrY, EnNrCb, and EnNrCr) cannot be used simultaneously.\n");
        ret = RETCODE_FAILURE;
    }

    if (param->losslessEnable && param->bgDetectEnable) {
        VLOG(ERR, "CFG FAIL : LosslessCoding and BgDetect cannot be used simultaneously.\n");
        ret = RETCODE_FAILURE;
    }

    if (param->losslessEnable && pop->rcEnable) {
        VLOG(ERR, "CFG FAIL : osslessCoding and RateControl cannot be used simultaneously.\n");
        ret = RETCODE_FAILURE;
    }

    if (param->losslessEnable && param->roiEnable) {
        VLOG(ERR, "CFG FAIL : LosslessCoding and Roi cannot be used simultaneously.\n");
        ret = RETCODE_FAILURE;
    }

    if (param->losslessEnable && !param->skipIntraTrans) {
        VLOG(ERR, "CFG FAIL : IntraTransSkip must be enabled when LosslessCoding is enabled.\n");
        ret = RETCODE_FAILURE;
    }

    // Intra refresh
    {
        Int32 num_ctu_row = (pop->picHeight + 64 - 1) / 64;
        Int32 num_ctu_col = (pop->picWidth + 64 - 1) / 64;

        if(param->intraRefreshMode && param->intraRefreshArg <= 0)
        {
            VLOG(ERR, "CFG FAIL : IntraCtuRefreshArg must be greater then 0 when IntraCtuRefreshMode is enabled.\n");
            ret = RETCODE_FAILURE;
        }
        if(param->intraRefreshMode == 1 && param->intraRefreshArg > num_ctu_row)
        {
            VLOG(ERR, "CFG FAIL : IntraCtuRefreshArg must be less then the number of CTUs in a row when IntraCtuRefreshMode is equal to 1.\n");
            ret = RETCODE_FAILURE;
        }
        if(param->intraRefreshMode == 2 && param->intraRefreshArg > num_ctu_col)
        {
            VLOG(ERR, "CFG FAIL : IntraCtuRefreshArg must be less then the number of CTUs in a column when IntraCtuRefreshMode is equal to 2.\n");
            ret = RETCODE_FAILURE;
        }
        if(param->intraRefreshMode == 3 && param->intraRefreshArg > num_ctu_row*num_ctu_col)
        {
            VLOG(ERR, "CFG FAIL : IntraCtuRefreshArg must be less then the number of CTUs in a picture when IntraCtuRefreshMode is equal to 3.\n");
            ret = RETCODE_FAILURE;
        }
        if(param->intraRefreshMode == 4 && param->intraRefreshArg > num_ctu_row*num_ctu_col)
        {
            VLOG(ERR, "CFG FAIL : IntraCtuRefreshArg must be less then the number of CTUs in a picture when IntraCtuRefreshMode is equal to 4.\n");
            ret = RETCODE_FAILURE;
        }
        if(param->intraRefreshMode == 4 && param->losslessEnable)
        {
            VLOG(ERR, "CFG FAIL : LosslessCoding and IntraCtuRefreshMode (4) cannot be used simultaneously.\n");
            ret = RETCODE_FAILURE;
        }
        if(param->intraRefreshMode == 4 && param->roiEnable)
        {
            VLOG(ERR, "CFG FAIL : Roi and IntraCtuRefreshMode (4) cannot be used simultaneously.\n");
            ret = RETCODE_FAILURE;
        }
    }
    return ret;
}

RetCode Wave5VpuEncCheckRcParamValid(EncOpenParam* pop)
{
    RetCode       ret = RETCODE_SUCCESS;
    EncWave5Param* param = &pop->EncStdParam.waveParam;
    int           frameRateDiv = 0, frameRateRes = 0, frameRate = 0;

    if(pop->rcEnable == 1)
    {
        if((param->minQpI > param->maxQpI) || (param->minQpP > param->maxQpP) || (param->minQpB > param->maxQpB))
        {
            VLOG(ERR,"CFG FAIL : Not allowed MinQP > MaxQP\n");
            VLOG(ERR,"RECOMMEND CONFIG PARAMETER : MinQP = MaxQP\n");
            ret = RETCODE_FAILURE;
        }

        frameRateDiv = (pop->frameRateInfo >> 16) + 1;
        frameRateRes = pop->frameRateInfo & 0xFFFF;
        frameRate = frameRateRes / frameRateDiv;

        if(pop->bitRate <= frameRate)
        {
            VLOG(ERR,"CFG FAIL : Not allowed EncBitRate <= FrameRate\n");
            VLOG(ERR,"RECOMMEND CONFIG PARAMETER : EncBitRate = FrameRate * 10000\n");
            ret = RETCODE_FAILURE;
        }
    }

    return ret;
}

RetCode Wave5VpuEncCheckCustomGopParamValid(EncOpenParam* pop)
{
    RetCode       ret = RETCODE_SUCCESS;
    CustomGopParam* gopParam;
    CustomGopPicParam* gopPicParam;

    CustomGopPicParam new_gop[MAX_GOP_NUM*2 +1];
    Int32   curr_poc, i, ei, gi, gop_size;
    Int32   enc_tid[MAX_GOP_NUM*2 +1];

    gopParam    = &(pop->EncStdParam.waveParam.gopParam);
    gop_size    = gopParam->customGopSize;

    new_gop[0].pocOffset    = 0;
    new_gop[0].temporalId   = 0;
    new_gop[0].picType      = PIC_TYPE_I;
    new_gop[0].useMultiRefP = 0;
    enc_tid[0]              = 0;

    for(i = 0; i < gop_size * 2; i++)
    {
        ei = i % gop_size;
        gi = i / gop_size;
        gopPicParam = &gopParam->picParam[ei];

        curr_poc                    = gi * gop_size + gopPicParam->pocOffset;
        new_gop[i + 1].pocOffset    = curr_poc;
        new_gop[i + 1].temporalId   = gopPicParam->temporalId;
        new_gop[i + 1].picType      = gopPicParam->picType;
        new_gop[i + 1].refPocL0     = gopPicParam->refPocL0 + gi * gop_size;
        new_gop[i + 1].refPocL1     = gopPicParam->refPocL1 + gi * gop_size;
        new_gop[i + 1].useMultiRefP = gopPicParam->useMultiRefP;
        enc_tid[i + 1] = -1;
    }

    for(i = 0; i < gop_size; i++)
    {
        gopPicParam = &gopParam->picParam[i];

        if(gopPicParam->pocOffset <= 0)
        {
            VLOG(ERR, "CFG FAIL : the POC of the %d-th picture must be greater then -1\n", i+1);
            ret = RETCODE_FAILURE;
        }
        if(gopPicParam->pocOffset > gop_size)
        {
            VLOG(ERR, "CFG FAIL : the POC of the %d-th picture must be less then GopSize + 1\n", i+1);
            ret = RETCODE_FAILURE;
        }
        if(gopPicParam->temporalId < 0)
        {
            VLOG(ERR, "CFG FAIL : the temporal_id of the %d-th picture must be greater than -1\n", i+1);
            ret = RETCODE_FAILURE;
        }
    }

    for(ei = 1; ei < gop_size * 2 + 1; ei++)
    {
        CustomGopPicParam* cur_pic = &new_gop[ei];
        if(ei <= gop_size)
        {
            enc_tid[cur_pic->pocOffset] = cur_pic->temporalId;
            continue;
        }

        if(new_gop[ei].picType != PIC_TYPE_I)
        {
            Int32 ref_poc = cur_pic->refPocL0;
            if(enc_tid[ref_poc] < 0) // reference picture is not encoded yet
            {
                VLOG(ERR, "CFG FAIL : the 1st reference picture cannot be used as the reference of the picture (POC %d) in encoding order\n", cur_pic->pocOffset - gop_size);
                ret = RETCODE_FAILURE;
            }
            if(enc_tid[ref_poc] > cur_pic->temporalId)
            {
                VLOG(ERR, "CFG FAIL : the temporal_id of the picture (POC %d) is wrong\n", cur_pic->pocOffset - gop_size);
                ret = RETCODE_FAILURE;
            }
            if(ref_poc >= cur_pic->pocOffset)
            {
                VLOG(ERR, "CFG FAIL : the POC of the 1st reference picture of %d-th picture is wrong\n", cur_pic->pocOffset - gop_size);
                ret = RETCODE_FAILURE;
            }
        }
        if(new_gop[ei].picType != PIC_TYPE_P)
        {
            Int32 ref_poc = cur_pic->refPocL1;
            if(enc_tid[ref_poc] < 0) // reference picture is not encoded yet
            {
                VLOG(ERR, "CFG FAIL : the 2nd reference picture cannot be used as the reference of the picture (POC %d) in encoding order\n", cur_pic->pocOffset - gop_size);
                ret = RETCODE_FAILURE;
            }
            if(enc_tid[ref_poc] > cur_pic->temporalId)
            {
                VLOG(ERR, "CFG FAIL : the temporal_id of %d-th picture is wrong\n", cur_pic->pocOffset - gop_size);
                ret = RETCODE_FAILURE;
            }
            if(new_gop[ei].picType == PIC_TYPE_P && new_gop[ei].useMultiRefP > 0)
            {
                if(ref_poc >= cur_pic->pocOffset)
                {
                    VLOG(ERR, "CFG FAIL : the POC of the 2nd reference picture of %d-th picture is wrong\n", cur_pic->pocOffset - gop_size);
                    ret = RETCODE_FAILURE;
                }
            }
            else // HOST_PIC_TYPE_B
            {
                if(ref_poc == cur_pic->pocOffset)
                {
                    VLOG(ERR, "CFG FAIL : the POC of the 2nd reference picture of %d-th picture is wrong\n", cur_pic->pocOffset - gop_size);
                    ret = RETCODE_FAILURE;
                }
            }
        }
        curr_poc = cur_pic->pocOffset;
        enc_tid[curr_poc] = cur_pic->temporalId;
    }
    return ret;
}

RetCode Wave5VpuEncCheckOpenParam(EncOpenParam* pop)
{
    Int32 coreIdx = pop->coreIdx;
    VpuAttr* pAttr = &g_VpuCoreAttributes[coreIdx];
    EncWave5Param* param = &pop->EncStdParam.waveParam;

    if (pop->bitstreamFormat == STD_AVC && pop->EncStdParam.waveParam.internalBitDepth == 10 && pAttr->supportAVC10bitEnc != TRUE)
        return RETCODE_NOT_SUPPORTED_FEATURE;
    if (pop->bitstreamFormat == STD_HEVC && pop->EncStdParam.waveParam.internalBitDepth == 10 && pAttr->supportHEVC10bitEnc != TRUE)
        return RETCODE_NOT_SUPPORTED_FEATURE;


    if (pop->ringBufferEnable == TRUE) {
        if (pop->bitstreamBuffer % 8)
            return RETCODE_INVALID_PARAM;
        if (pop->bitstreamBuffer % 16)
            return RETCODE_INVALID_PARAM;
        if (pop->bitstreamBufferSize < (1024*64))
            return RETCODE_INVALID_PARAM;
        if (pop->bitstreamBufferSize % 1024 || pop->bitstreamBufferSize < 1024)
            return RETCODE_INVALID_PARAM;
    }

    if ((pAttr->supportEncoders&(1<<pop->bitstreamFormat)) == 0)
        return RETCODE_NOT_SUPPORTED_FEATURE;
    if (pop->frameRateInfo == 0)
        return RETCODE_INVALID_PARAM;

    if (pop->bitRate > 700000000 || pop->bitRate < 0)
        return RETCODE_INVALID_PARAM;

    if (pop->picWidth < W_MIN_ENC_PIC_WIDTH || pop->picWidth > W_MAX_ENC_PIC_WIDTH)
        return RETCODE_INVALID_PARAM;

    if (pop->picHeight < W_MIN_ENC_PIC_HEIGHT || pop->picHeight > W_MAX_ENC_PIC_HEIGHT)
        return RETCODE_INVALID_PARAM;

    if (param->profile!=0) {
        if (pop->bitstreamFormat == STD_HEVC) { // only for HEVC condition
            if (param->profile != HEVC_PROFILE_MAIN && param->profile != HEVC_PROFILE_MAIN10 && param->profile != HEVC_PROFILE_STILLPICTURE)
                return RETCODE_INVALID_PARAM;
            if (param->internalBitDepth > 8 && param->profile == HEVC_PROFILE_MAIN)
                return RETCODE_INVALID_PARAM;
        } else if (pop->bitstreamFormat == STD_AVC) {
            if ( (param->internalBitDepth > 8 && param->profile != H264_PROFILE_HIGH10))
                return RETCODE_INVALID_PARAM;
        }
    }

    if (param->internalBitDepth != 8 && param->internalBitDepth != 10)
        return RETCODE_INVALID_PARAM;

    if (param->decodingRefreshType < 0 || param->decodingRefreshType > 2)
        return RETCODE_INVALID_PARAM;

    if (param->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
        if ( param->gopParam.customGopSize < 1 || param->gopParam.customGopSize > MAX_GOP_NUM)
            return RETCODE_INVALID_PARAM;
    }

    if (pop->bitstreamFormat == STD_AVC) {
        if (param->customLambdaEnable == 1)
            return RETCODE_INVALID_PARAM;
    }
    if (param->constIntraPredFlag != 1 && param->constIntraPredFlag != 0)
        return RETCODE_INVALID_PARAM;

    if (param->intraRefreshMode < 0 || param->intraRefreshMode > 4)
        return RETCODE_INVALID_PARAM;

    if (pop->bitstreamFormat == STD_HEVC) {
        if (param->independSliceMode < 0 || param->independSliceMode > 1)
            return RETCODE_INVALID_PARAM;

        if (param->independSliceMode != 0) {
            if (param->dependSliceMode < 0 || param->dependSliceMode > 2)
                return RETCODE_INVALID_PARAM;
        }
    }

    if (param->useRecommendEncParam < 0 && param->useRecommendEncParam > 3)
        return RETCODE_INVALID_PARAM;

    if (param->useRecommendEncParam == 0 || param->useRecommendEncParam == 2 || param->useRecommendEncParam == 3) {
        if (param->intraNxNEnable != 1 && param->intraNxNEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->skipIntraTrans != 1 && param->skipIntraTrans != 0)
            return RETCODE_INVALID_PARAM;

        if (param->scalingListEnable != 2 && param->scalingListEnable != 1 && param->scalingListEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->tmvpEnable != 1 && param->tmvpEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->wppEnable != 1 && param->wppEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->useRecommendEncParam != 3) {     // in FAST mode (recommendEncParam==3), maxNumMerge value will be decided in FW
            if (param->maxNumMerge < 0 || param->maxNumMerge > 3)
                return RETCODE_INVALID_PARAM;
        }

        if (param->disableDeblk != 1 && param->disableDeblk != 0)
            return RETCODE_INVALID_PARAM;

        if (param->disableDeblk == 0 || param->saoEnable != 0) {
            if (param->lfCrossSliceBoundaryEnable != 1 && param->lfCrossSliceBoundaryEnable != 0)
                return RETCODE_INVALID_PARAM;
        }

        if (param->disableDeblk == 0) {
            if (param->betaOffsetDiv2 < -6 || param->betaOffsetDiv2 > 6)
                return RETCODE_INVALID_PARAM;

            if (param->tcOffsetDiv2 < -6 || param->tcOffsetDiv2 > 6)
                return RETCODE_INVALID_PARAM;
        }
    }

    if (param->losslessEnable != 1 && param->losslessEnable != 0)
        return RETCODE_INVALID_PARAM;

    if (param->intraQP < 0 || param->intraQP > 63)
        return RETCODE_INVALID_PARAM;

    if (pop->rcEnable != 1 && pop->rcEnable != 0)
        return RETCODE_INVALID_PARAM;

    if (pop->rcEnable == 1) {
        if (param->minQpI < 0 || param->minQpI > 63)
            return RETCODE_INVALID_PARAM;
        if (param->maxQpI < 0 || param->maxQpI > 63)
            return RETCODE_INVALID_PARAM;

        if (param->minQpP < 0 || param->minQpP > 63)
            return RETCODE_INVALID_PARAM;
        if (param->maxQpP < 0 || param->maxQpP > 63)
            return RETCODE_INVALID_PARAM;

        if (param->minQpB < 0 || param->minQpB > 63)
            return RETCODE_INVALID_PARAM;
        if (param->maxQpB < 0 || param->maxQpB > 63)
            return RETCODE_INVALID_PARAM;

        if (param->cuLevelRCEnable != 1 && param->cuLevelRCEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->hvsQPEnable != 1 && param->hvsQPEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->hvsQPEnable) {
            if (param->hvsMaxDeltaQp < 0 || param->hvsMaxDeltaQp > 51)
                return RETCODE_INVALID_PARAM;
        }

        if (param->bitAllocMode < 0 && param->bitAllocMode > 2)
            return RETCODE_INVALID_PARAM;

        if (pop->vbvBufferSize < 10 || pop->vbvBufferSize > 3000 )
            return RETCODE_INVALID_PARAM;
    }

    // packed format & cbcrInterleave & nv12 can't be set at the same time.
    if (pop->packedFormat == 1 && pop->cbcrInterleave == 1)
        return RETCODE_INVALID_PARAM;

    if (pop->packedFormat == 1 && pop->nv21 == 1)
        return RETCODE_INVALID_PARAM;

    // check valid for common param
    if (Wave5VpuEncCheckCommonParamValid(pop) == RETCODE_FAILURE)
        return RETCODE_INVALID_PARAM;

    // check valid for RC param
    if (Wave5VpuEncCheckRcParamValid(pop) == RETCODE_FAILURE)
        return RETCODE_INVALID_PARAM;

    if (param->gopPresetIdx == PRESET_IDX_CUSTOM_GOP) {
        if (Wave5VpuEncCheckCustomGopParamValid(pop) == RETCODE_FAILURE)
            return RETCODE_INVALID_COMMAND;
    }

    if (param->chromaCbQpOffset < -12 || param->chromaCbQpOffset > 12)
        return RETCODE_INVALID_PARAM;

    if (param->chromaCrQpOffset < -12 || param->chromaCrQpOffset > 12)
        return RETCODE_INVALID_PARAM;

    if (param->intraRefreshMode == 3 && param-> intraRefreshArg == 0)
        return RETCODE_INVALID_PARAM;

    if (pop->bitstreamFormat == STD_HEVC) {
        if (param->nrYEnable != 1 && param->nrYEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->nrCbEnable != 1 && param->nrCbEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->nrCrEnable != 1 && param->nrCrEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->nrNoiseEstEnable != 1 && param->nrNoiseEstEnable != 0)
            return RETCODE_INVALID_PARAM;

        if (param->nrNoiseSigmaY > 255)
            return RETCODE_INVALID_PARAM;

        if (param->nrNoiseSigmaCb > 255)
            return RETCODE_INVALID_PARAM;

        if (param->nrNoiseSigmaCr > 255)
            return RETCODE_INVALID_PARAM;

        if (param->nrIntraWeightY > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrIntraWeightCb > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrIntraWeightCr > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrInterWeightY > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrInterWeightCb > 31)
            return RETCODE_INVALID_PARAM;

        if (param->nrInterWeightCr > 31)
            return RETCODE_INVALID_PARAM;

        if((param->nrYEnable == 1 || param->nrCbEnable == 1 || param->nrCrEnable == 1) && (param->losslessEnable == 1))
            return RETCODE_INVALID_PARAM;
    }

    return RETCODE_SUCCESS;
}



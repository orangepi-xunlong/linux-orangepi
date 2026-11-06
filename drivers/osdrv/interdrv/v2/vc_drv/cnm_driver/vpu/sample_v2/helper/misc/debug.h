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

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <linux/types.h>
#include "config.h"
#include "main_helper.h"

#define W6_FIO_VCORE_OFFSET                 0x4000
#define W6_ENTROPY_BASE                     0x4000
#define W6_ENT_DEBUG_INFO_0                 (W6_ENTROPY_BASE + 0x10)
#define W6_ENT_DEBUG_INFO_1                 (W6_ENTROPY_BASE + 0x14)

#define W6_VCE_ENT_BSU_BASE                 0x4700
#define W6_VCE_ENT_BSU_0x7                  0x4700
#define W6_VCE_ENT_BSU_0x8                  0x4800
#define W6_VCE_ENT_BSU_0x9                  0x4900
#define W6_VCE_ENT_DEC_COMMON               0x4A00
#define W6_VCE_ENT_DEC_AVC_HEVC             0x4B00
#define W6_VCE_ENT_DEC_AV1                  0x4C00
#define W6_VCE_ENT_DEC_AV1_MVP              0x4D00
#define W6_VCE_ENT_DEC_VP9                  0x4E00

#define W6_VCE_VCTRL_BASE                   0x5000
#define W6_VCE_SRC_BASE                     0x5100
#define W6_VCE_CME_BASE                     0x5200
#define W6_VCE_RME_FME_IMD_BASE             0x5300
#define W6_VCE_RHU                          0x5400
#define W6_VCE_REC                          0x5500
#define W6_VCE_DEC_CORE_BASE_0              0x5800
#define W6_VCE_DEC_CORE_BASE_1              0x5900
#define W6_VCE_LF_BASE                      0x5C00
#define W6_VCE_MCS_BASE                     0x2000
#define W6_VCE_RDO_BASE                     0x5600

#define W6_VCE_CTL_STATUS                   (W6_VCE_VCTRL_BASE + 0x24)
#define W6_VCE_CTL_ERR_ERROR                (W6_VCE_VCTRL_BASE + 0x60)
#define W6_VCE_CTL_ERR_WARNING              (W6_VCE_VCTRL_BASE + 0x64)

#define W6_VCE_DEC_CORE_DEBUG               (W6_VCE_DEC_CORE_BASE_0 + 0x08)
#define W6_VCE_DEC_TQ_DEBUG0                (W6_VCE_DEC_CORE_BASE_0 + 0x50)
#define W6_VCE_DEC_TQ_DEBUG1                (W6_VCE_DEC_CORE_BASE_0 + 0x54)
#define W6_VCE_DEC_IP_DEBUG                 (W6_VCE_DEC_CORE_BASE_0 + 0x78)
#define W6_VCE_DEC_REC_DEBUG0               (W6_VCE_DEC_CORE_BASE_0 + 0x7C)
#define W6_VCE_DEC_REC_DEBUG1               (W6_VCE_DEC_CORE_BASE_0 + 0x80)
#define W6_VCE_DEC_MC_REF_DEBUG             (W6_VCE_DEC_CORE_BASE_1 + 0xEC)
#define W6_VCE_DEC_MC_INTP_DEBUG            (W6_VCE_DEC_CORE_BASE_1 + 0xF0)

#define W6_VCE_LF_DEBUG                     (W6_VCE_LF_BASE + 0xF8)

#define W6_VCE_ENT_BSU_STA_ADDR             (W6_VCE_ENT_BSU_BASE + 0x00)
#define W6_VCE_ENT_BSU_END_ADDR             (W6_VCE_ENT_BSU_BASE + 0x04)
#define W6_VCE_ENT_BSU_CURR_POS             (W6_VCE_ENT_BSU_BASE + 0x08)
#define W6_VCE_ENT_BSU_NAL_STATUS           (W6_VCE_ENT_BSU_BASE + 0x4C)
#define W6_VCE_ENT_BSU_RDMA                 (W6_VCE_ENT_BSU_BASE + 0x60)
#define W6_VCE_ENT_BSU_RBSP_STATUS          (W6_VCE_ENT_BSU_BASE + 0xA0)
#define W6_VCE_ENT_BSU_ACCESS_STATUS        (W6_VCE_ENT_BSU_BASE + 0xA4)
#define W6_VCE_ENT_BSU_CUR_VALID            (W6_VCE_ENT_BSU_BASE + 0xA8)
#define W6_VCE_ENT_BSU_CPB_RPTR             (W6_VCE_ENT_BSU_BASE + 0xF0)
#define W6_VCE_ENT_BSU_CPB_WPTR             (W6_VCE_ENT_BSU_BASE + 0xF4)

#define W6_VCE_ENT_DEC_TILE_FIRST_CTU_POS   (W6_VCE_ENT_DEC_COMMON + 0x08)
#define W6_VCE_ENT_DEC_TILE_LAST_CTU_POS    (W6_VCE_ENT_DEC_COMMON + 0x0C)
#define W6_VCE_ENT_DEC_CURR_CTU_POS         (W6_VCE_ENT_DEC_COMMON + 0x10)
#define W6_VCE_ENT_DEC_ERR_INFO             (W6_VCE_ENT_DEC_COMMON + 0x88)
#define W6_VCE_ENT_DEC_DEBUG_INFO           (W6_VCE_ENT_DEC_COMMON + 0x8C)

#define W6_VCE_ENT_DEC_DEBUG_TMPL_STATE     (W6_VCE_ENT_DEC_AVC_HEVC + 0xF0)

#define W6_MCS_MASTER_INFO                  (W6_VCE_MCS_BASE + 0x28)
#define W6_MCS_SLAVE_INFO                   (W6_VCE_MCS_BASE + 0x2C)

#define W6_MCS_MASTER_POS                   (W6_VCE_MCS_BASE + 0x30)
#define W6_MCS_MASTER_STA                   (W6_VCE_MCS_BASE + 0x34)
#define W6_MCS_SLAVE_POS                    (W6_VCE_MCS_BASE + 0x38)
#define W6_MCS_SLAVE_STA                    (W6_VCE_MCS_BASE + 0x3C)

#define W6_RDO_DEBUG_MODE                   (W6_VCE_RDO_BASE + 0xE0)
#define W6_RDO_DEBUG_TOP                    (W6_VCE_RDO_BASE + 0xE4)
#define W6_RDO_DEBUG_CTRL                   (W6_VCE_RDO_BASE + 0xE8)
#define W6_RDO_DEBUG_POS                    (W6_VCE_RDO_BASE + 0xEC)
#define W6_RDO_DEBUG_CTU                    (W6_VCE_RDO_BASE + 0xF0)
#define W6_RDO_DEBUG_CU                     (W6_VCE_RDO_BASE + 0xF4)

#define W6_CME_STATUS_0                     (W6_VCE_CME_BASE + 0x70)
#define W6_CME_STATUS_1                     (W6_VCE_CME_BASE + 0x74)
#define W6_CME_DEBUG_0	                    (W6_VCE_CME_BASE + 0x78)
#define W6_CME_DEBUG_1	                    (W6_VCE_CME_BASE + 0x7C)
#define W6_CME_DEBUG_2	                    (W6_VCE_CME_BASE + 0x80)
#define W6_CME_DEBUG_3	                    (W6_VCE_CME_BASE + 0x84)


enum {
    CNM_ERROR_NONE          = 0x000,
    CNM_ERROR_FAILURE       = 0x001,
    CNM_ERROR_HANGUP        = 0x002,
    CNM_ERROR_MISMATCH      = 0x004,
    CNM_ERROR_SEQ_INIT      = 0x008,
    CNM_ERROR_GET_SEQ_INFO  = 0x010,
};

enum {
    CNMQC_ENV_NONE,
    CNMQC_ENV_GDBSERVER,            /*!<< It executes gdb server in order to debug F/W on the C&M FPGA env. */
    CNMQC_ENV_MAX,
};

typedef enum {
    DEBUG_CODEC_NONE,
    DEBUG_CODEC_DEC,
    DEBUG_CODEC_ENC
} DEBUG_CODEC_TYPE;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void CNMErrorSet(Uint32 val);
Uint32 CNMErrorGet(void);

extern void InitializeDebugEnv(Uint32 options);
extern void ReleaseDebugEnv(void);
extern void ExecuteDebugger(void);

extern void ChekcAndPrintDebugInfo(VpuHandle handle, BOOL isEnc, RetCode result);

extern void PrintDecVpuStatus(
    DecHandle   handle
    );

extern void PrintEncVpuStatus(
    EncHandle   handle
    );

extern void PrintMemoryAccessViolationReason(
    Uint32          core_idx,
    void            *outp
    );

#define VCORE_DBG_ADDR(__vCoreIdx)      0x8000+(0x1000*__vCoreIdx) + 0x300
#define VCORE_DBG_DATA(__vCoreIdx)      0x8000+(0x1000*__vCoreIdx) + 0x304
#define VCORE_DBG_READY(__vCoreIdx)     0x8000+(0x1000*__vCoreIdx) + 0x308

extern void WriteRegVCE(
    Uint32   core_idx,
    Uint32   vce_core_idx,
    Uint32   vce_addr,
    Uint32   udata
    );

extern Uint32 ReadRegVCE(
    Uint32 core_idx,
    Uint32 vce_core_idx,
    Uint32 vce_addr
    );

extern char dumpTime[MAX_FILE_PATH];
#define HEXDUMP_COLS 16
extern void DisplayHex(void *mem, Uint32 len, char* name);


extern RetCode PrintVpuProductInfo(
    Uint32      core_idx,
    VpuAttr*    productInfo
    );


extern Int32 HandleDecInitSequenceError(
    DecHandle       handle,
    Uint32          productId,
    DecOpenParam*   openParam,
    DecInitialInfo* seqInfo,
    RetCode         apiErrorCode
    );

extern void HandleDecoderError(
    DecHandle       handle,
    Uint32          frameIdx,
    DecOutputInfo*  outputInfo
    );

extern void DumpMemory(const char* path, Uint32 coreIdx, PhysicalAddress addr, Uint32 size, EndianMode endian);
extern void DumpCodeBuffer(const char* path);
extern void DumpBitstreamBuffer(Uint32 coreIdx, PhysicalAddress addr, Uint32 size, EndianMode endian, const char* prefix);
extern void DumpColMvBuffers(Uint32 coreIdx, const DecInfo* pDecInfo);

extern void HandleEncoderError(
    EncHandle       handle,
    Uint32          encPicCnt,
    EncOutputInfo*  outputInfo
    );
extern Uint32 SetEncoderTimeout(
    int width,
    int height
    );
extern Uint32 SetWave6EncoderTimeout(
    int width,
    int height
    );
extern Uint32 SetWave5EncoderTimeout(
    int width,
    int height
    );

extern void print_busy_timeout_status(
    Uint32 coreIdx,
    Uint32 product_code,
    Uint32 pc
    );

extern void wave5xx_vcpu_status (
    unsigned long coreIdx
    );

extern void wave6xx_vcpu_status(
    unsigned long coreIdx,
    DEBUG_CODEC_TYPE codec_type
    );
extern void wave6xx_vcore_status(
    unsigned long coreIdx,
    DEBUG_CODEC_TYPE codec_type
    );
extern void vdi_print_vpu_status(
    unsigned long coreIdx
    );

extern void wave5xx_bpu_status(
    Uint32 coreIdx
    );

extern void vdi_print_vcore_status(
    Uint32 coreIdx
    );

extern void vdi_print_vpu_status_dec(
    unsigned long coreIdx
    );

extern void vdi_print_vpu_status_enc(
    unsigned long coreIdx
    );



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _DEBUG_H_ */


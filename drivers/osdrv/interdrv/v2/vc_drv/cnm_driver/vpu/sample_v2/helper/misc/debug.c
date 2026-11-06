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
#include "config.h"
#include "wave/wave5_regdefine.h"
#include "wave/wave6_regdefine.h"
#include "coda9/coda9_regdefine.h"
#include "main_helper.h"
#include "misc/debug.h"
#include "vpuconfig.h"

Uint32 CnmErrorStatus = 0;


enum { False, True };

void ExecuteDebugger(void)
{
    VLOG(INFO, "Starting the debugger....\n");
}

void InitializeDebugEnv(Uint32  option)
{
    switch(option) {
    case CNMQC_ENV_GDBSERVER: ExecuteDebugger(); break;
    default: break;
    }
}

void ReleaseDebugEnv(void)
{
}

Int32 checkLineFeedInHelp(
    struct OptionExt *opt
    )
{
    int i;

    for (i=0;i<MAX_GETOPT_OPTIONS;i++) {
        if (opt[i].name==NULL)
            break;
        if (!osal_strstr(opt[i].help, "\n")) {
            VLOG(INFO, "(%s) doesn't have \\n in options struct in main function. please add \\n\n", opt[i].help);
            return FALSE;
        }
    }
    return TRUE;
}

RetCode PrintVpuProductInfo(
    Uint32   coreIdx,
    VpuAttr* productInfo
)
{
    BOOL verbose = FALSE;
    RetCode ret = RETCODE_SUCCESS;

    if ((ret = VPU_GetProductInfo(coreIdx, productInfo)) != RETCODE_SUCCESS) {
        return ret;
    }

    VLOG(INFO, "VPU coreNum : [%d]\n", coreIdx);
    VLOG(INFO, "Firmware    : CustomerCode: %04x | version : rev.%d(%x)\n", productInfo->customerId, productInfo->fwVersion, productInfo->fwVersion);
    VLOG(INFO, "API         : %d.%d.%d\n", API_VERSION_MAJOR, API_VERSION_MINOR, API_VERSION_PATCH);
    VLOG(INFO, "productId   : %08x\n", productInfo->productId);
    VLOG(INFO, "productName : %s%4x\n", productInfo->productName, productInfo->productVersion);
    VLOG(INFO, "configDate  : %d\n", productInfo->hwConfigDate);
    VLOG(INFO, "HW version  : r%d\n", productInfo->hwConfigRev);

    if ( verbose == TRUE ) {
        if (PRODUCT_ID_W6_SERIES(productInfo->productId)) {
            Uint32 stdDef1          = productInfo->hwConfigDef1; //conf_vpu_config1
            Uint32 confFeature      = productInfo->hwConfigFeature; //conf_codec_std

            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef1           : %08x\n", stdDef1);
            VLOG(INFO, "VCORE ID 0        : %d\n", (stdDef1>>16)&1);
            VLOG(INFO, "CODEC AV1  DEC    : %d\n", (stdDef1>>5)&1);
            VLOG(INFO, "CODEC AV1  ENC    : %d\n", (stdDef1>>4)&1);
            VLOG(INFO, "CODEC AVC  DEC    : %d\n", (stdDef1>>3)&1);
            VLOG(INFO, "CODEC HEVC DEC    : %d\n", (stdDef1>>2)&1);
            VLOG(INFO, "CODEC AVC  ENC    : %d\n", (stdDef1>>1)&1);
            VLOG(INFO, "CODEC HEVC ENC    : %d\n", (stdDef1>>0)&1);

            VLOG(INFO, "==========================\n");
            VLOG(INFO, "confFeature       : %08x\n", confFeature);
            VLOG(INFO, "AV1  ENC          : %d\n", (confFeature>>13)&1);
            VLOG(INFO, "AV1  DEC          : %d\n", (confFeature>>12)&1);
            VLOG(INFO, "AVC  ENC MAIN10   : %d\n", (confFeature>>11)&1);
            VLOG(INFO, "AVC  ENC MAIN     : %d\n", (confFeature>>10)&1);
            VLOG(INFO, "AVC  DEC MAIN10   : %d\n", (confFeature>>9)&1);
            VLOG(INFO, "AVC  DEC MAIN     : %d\n", (confFeature>>8)&1);
            VLOG(INFO, "VP9  ENC Profile2 : %d\n", (confFeature>>7)&1);
            VLOG(INFO, "VP9  ENC Profile0 : %d\n", (confFeature>>6)&1);
            VLOG(INFO, "VP9  DEC Profile2 : %d\n", (confFeature>>5)&1);
            VLOG(INFO, "VP9  DEC Profile0 : %d\n", (confFeature>>4)&1);
            VLOG(INFO, "HEVC ENC MAIN10   : %d\n", (confFeature>>3)&1);
            VLOG(INFO, "HEVC ENC MAIN     : %d\n", (confFeature>>2)&1);
            VLOG(INFO, "HEVC DEC MAIN10   : %d\n", (confFeature>>1)&1);
            VLOG(INFO, "HEVC DEC MAIN     : %d\n", (confFeature>>0)&1);
            VLOG(INFO, "==========================\n");
        }
        if (PRODUCT_ID_W5_SERIES(productInfo->productId))
        {
            Uint32 stdDef0          = productInfo->hwConfigDef0;
            Uint32 stdDef1          = productInfo->hwConfigDef1;
            Uint32 confFeature      = productInfo->hwConfigFeature;
            BOOL supportDownScaler  = FALSE;
            BOOL supportAfbce       = FALSE;
            char ch_ox[2]           = {'X', 'O'};

            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef0           : %08x\n", stdDef0);
            /* checking ONE AXI BIT FILE */
            VLOG(INFO, "MAP CONVERTER REG : %d\n", (stdDef0>>31)&1);
            VLOG(INFO, "MAP CONVERTER SIG : %d\n", (stdDef0>>30)&1);
            VLOG(INFO, "PVRIC FBC EN      : %d\n", (stdDef0>>27)&1);
            VLOG(INFO, "PVRIC FBC ID      : %d\n", (stdDef0>>24)&7);
            VLOG(INFO, "SCALER 2ALIGNED   : %d\n", (stdDef0>>23)&1);
            VLOG(INFO, "VCORE BACKBONE    : %d\n", (stdDef0>>22)&1);
            VLOG(INFO, "STD SWITCH EN     : %d\n", (stdDef0>>21)&1);
            VLOG(INFO, "BG_DETECT         : %d\n", (stdDef0>>20)&1);
            VLOG(INFO, "3D NR             : %d\n", (stdDef0>>19)&1);
            VLOG(INFO, "ONE-PORT AXI      : %d\n", (stdDef0>>18)&1);
            VLOG(INFO, "2nd AXI           : %d\n", (stdDef0>>17)&1);
            VLOG(INFO, "GDI               : %d\n", !((stdDef0>>16)&1));//no-gdi
            VLOG(INFO, "AFBC              : %d\n", (stdDef0>>15)&1);
            VLOG(INFO, "AFBC VERSION      : %d\n", (stdDef0>>12)&7);
            VLOG(INFO, "FBC               : %d\n", (stdDef0>>11)&1);
            VLOG(INFO, "FBC  VERSION      : %d\n", (stdDef0>>8)&7);
            VLOG(INFO, "SCALER            : %d\n", (stdDef0>>7)&1);
            VLOG(INFO, "SCALER VERSION    : %d\n", (stdDef0>>4)&7);
            VLOG(INFO, "BWB               : %d\n", (stdDef0>>2)&1);
            VLOG(INFO, "==========================\n");
            VLOG(INFO, "stdDef1           : %08x\n", stdDef1);
            VLOG(INFO, "REF RINGBUFFER    : %d\n", (stdDef1>>31)&1);
            VLOG(INFO, "SKIP VLC EN       : %d\n", (stdDef1>>30)&1);
            VLOG(INFO, "CyclePerTick      : %d\n", (stdDef1>>27)&1); //0:32768, 1:256
            VLOG(INFO, "MULTI CORE EN     : %d\n", (stdDef1>>26)&1);
            VLOG(INFO, "GCU EN            : %d\n", (stdDef1>>25)&1);
            VLOG(INFO, "CU REPORT         : %d\n", (stdDef1>>24)&1);
            VLOG(INFO, "RDO REPORT        : %d\n", (stdDef1>>23)&1);
            VLOG(INFO, "MV HISTOGRAM      : %d\n", (stdDef1>>22)&1);
            VLOG(INFO, "INPUT RINGBUFFER  : %d\n", (stdDef1>>21)&1);
            VLOG(INFO, "ENT OPT ENABLED   : %d\n", (stdDef1>>20)&1);
            VLOG(INFO, "VCORE ID 3        : %d\n", (stdDef1>>19)&1);
            VLOG(INFO, "VCORE ID 2        : %d\n", (stdDef1>>18)&1);
            VLOG(INFO, "VCORE ID 1        : %d\n", (stdDef1>>17)&1);
            VLOG(INFO, "VCORE ID 0        : %d\n", (stdDef1>>16)&1);
            VLOG(INFO, "BW OPT            : %d\n", (stdDef1>>15)&1);

            VLOG(INFO, "CODEC STD AV1     : %d\n", (stdDef1>>4)&1);
            VLOG(INFO, "CODEC STD AVS2    : %d\n", (stdDef1>>3)&1);
            VLOG(INFO, "CODEC STD AVC     : %d\n", (stdDef1>>2)&1);
            VLOG(INFO, "CODEC STD VP9     : %d\n", (stdDef1>>1)&1);
            VLOG(INFO, "CODEC STD HEVC    : %d\n", (stdDef1>>0)&1);

            VLOG(INFO, "==========================\n");
            VLOG(INFO, "confFeature       : %08x\n", confFeature);
            if ( productInfo->hwConfigRev > 167455 ) {//20190321
                VLOG(INFO, "AVC  ENC MAIN10   : %d\n", (confFeature>>11)&1);
                VLOG(INFO, "AVC  ENC MAIN     : %d\n", (confFeature>>10)&1);
                VLOG(INFO, "AVC  DEC MAIN10   : %d\n", (confFeature>>9)&1);
                VLOG(INFO, "AVC  DEC MAIN     : %d\n", (confFeature>>8)&1);
            }
            else {
                VLOG(INFO, "AVC  ENC          : %d\n", (confFeature>>9)&1);
                VLOG(INFO, "AVC  DEC          : %d\n", (confFeature>>8)&1);
            }
            VLOG(INFO, "AV1  ENC PROF     : %d\n", (confFeature>>14)&1);
            VLOG(INFO, "AV1  DEC HIGH     : %d\n", (confFeature>>13)&1);
            VLOG(INFO, "AV1  DEC MAIN     : %d\n", (confFeature>>12)&1);

            VLOG(INFO, "AVC  ENC MAIN10   : %d\n", (confFeature>>11)&1);
            VLOG(INFO, "AVC  ENC MAIN     : %d\n", (confFeature>>10)&1);
            VLOG(INFO, "AVC  DEC MAIN10   : %d\n", (confFeature>>9)&1);
            VLOG(INFO, "AVC  DEC MAIN     : %d\n", (confFeature>>8)&1);

            VLOG(INFO, "VP9  ENC Profile2 : %d\n", (confFeature>>7)&1);
            VLOG(INFO, "VP9  ENC Profile0 : %d\n", (confFeature>>6)&1);
            VLOG(INFO, "VP9  DEC Profile2 : %d\n", (confFeature>>5)&1);
            VLOG(INFO, "VP9  DEC Profile0 : %d\n", (confFeature>>4)&1);
            VLOG(INFO, "HEVC ENC MAIN10   : %d\n", (confFeature>>3)&1);
            VLOG(INFO, "HEVC ENC MAIN     : %d\n", (confFeature>>2)&1);
            VLOG(INFO, "HEVC DEC MAIN10   : %d\n", (confFeature>>1)&1);
            VLOG(INFO, "HEVC DEC MAIN     : %d\n", (confFeature>>0)&1);
            VLOG(INFO, "==========================\n");

            supportDownScaler = (BOOL)((stdDef0>>7)&1);
            supportAfbce      = (BOOL)((stdDef0>>15)&1);

            VLOG (INFO, "------------------------------------\n");
            VLOG (INFO, "VPU CONF| SCALER | AFBCE  |\n");
            VLOG (INFO, "        |   %c    |    %c   |\n", ch_ox[supportDownScaler], ch_ox[supportAfbce]);
            VLOG (INFO, "------------------------------------\n");
        }
    }
    return ret;
}

#define FIO_DBG_IRB_ADDR    0x8074
#define FIO_DBG_IRB_DATA    0x8078
#define FIO_DBG_IRB_STATUS  0x807C
void vdi_irb_write_register(
    unsigned long coreIdx,
    unsigned int  vcore_idx,
    unsigned int  irb_addr,
    unsigned int  irb_data)
{
    vdi_fio_write_register(coreIdx, FIO_DBG_IRB_DATA + 0x1000*vcore_idx, irb_data);
    vdi_fio_write_register(coreIdx, FIO_DBG_IRB_ADDR + 0x1000*vcore_idx, irb_addr);
}

unsigned int vdi_irb_read_register(
    unsigned long coreIdx,
    unsigned int  vcore_idx,
    unsigned int  irb_addr
    )
{
    unsigned int irb_rdata = 0;

    unsigned int irb_rd_cmd = 0;

    irb_rd_cmd = (1<<20)| (1<<16) | irb_addr; // {dbgMode, Read, Addr}
    vdi_fio_write_register(coreIdx, FIO_DBG_IRB_ADDR + 0x1000*vcore_idx, irb_rd_cmd);
    while((vdi_fio_read_register(coreIdx, FIO_DBG_IRB_STATUS + 0x1000*vcore_idx) & 0x1) == 0);

    irb_rdata = vdi_fio_read_register(coreIdx, FIO_DBG_IRB_DATA + 0x1000*vcore_idx);
    return irb_rdata;
}

char dumpTime[MAX_FILE_PATH];

/******************************************************************************
* help function                                                               *
******************************************************************************/
Uint32 ReadRegVCE(
    Uint32 coreIdx,
    Uint32 vce_core_idx,
    Uint32 vce_addr
    )
{//lint !e18
    int     vcpu_reg_addr;
    int     udata;
    int     vce_core_base = 0x8000 + 0x1000*vce_core_idx;

    SetClockGate(coreIdx, 1);
    vdi_fio_write_register(coreIdx, VCORE_DBG_READY(vce_core_idx), 0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(coreIdx, VCORE_DBG_ADDR(vce_core_idx),vcpu_reg_addr + vce_core_base);

    if (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 1)
        udata= vdi_fio_read_register(0, VCORE_DBG_DATA(vce_core_idx));
    else {
        VLOG(INFO, "failed to read VCE register: %d, 0x%04x\n", vce_core_idx, vce_addr);
        udata = -2;//-1 can be a valid value
    }

    SetClockGate(coreIdx, 0);
    return udata;
}

void WriteRegVCE(
    Uint32   coreIdx,
    Uint32   vce_core_idx,
    Uint32   vce_addr,
    Uint32   udata
    )
{
    int vcpu_reg_addr;

    SetClockGate(coreIdx, 1);

    vdi_fio_write_register(coreIdx, VCORE_DBG_READY(vce_core_idx),0);

    vcpu_reg_addr = vce_addr >> 2;

    vdi_fio_write_register(coreIdx, VCORE_DBG_DATA(vce_core_idx),udata);
    vdi_fio_write_register(coreIdx, VCORE_DBG_ADDR(vce_core_idx),(vcpu_reg_addr) & 0x00007FFF);

    while (vdi_fio_read_register(0, VCORE_DBG_READY(vce_core_idx)) == 0xffffffff) {
        VLOG(ERR, "failed to write VCE register: 0x%04x\n", vce_addr);
    }
    SetClockGate(coreIdx, 0);
}

#define VCE_DEC_CHECK_SUM0         0x110
#define VCE_DEC_CHECK_SUM1         0x114
#define VCE_DEC_CHECK_SUM2         0x118
#define VCE_DEC_CHECK_SUM3         0x11C
#define VCE_DEC_CHECK_SUM4         0x120
#define VCE_DEC_CHECK_SUM5         0x124
#define VCE_DEC_CHECK_SUM6         0x128
#define VCE_DEC_CHECK_SUM7         0x12C
#define VCE_DEC_CHECK_SUM8         0x130
#define VCE_DEC_CHECK_SUM9         0x134
#define VCE_DEC_CHECK_SUM10        0x138
#define VCE_DEC_CHECK_SUM11        0x13C

#define READ_BIT(val,high,low) ((((high)==31) && ((low) == 0)) ?  (val) : (((val)>>(low)) & (((1<< ((high)-(low)+1))-1))))


void DisplayVceEncDebugCommon521(int coreIdx, int vcore_idx, int set_mode, int debug0, int debug1, int debug2)
{
    int reg_val;
    VLOG(INFO, "---------------Common Debug INFO-----------------\n");

    WriteRegVCE(coreIdx, vcore_idx, set_mode,0 );

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug0);
    VLOG(INFO,"\t- subblok_done      :  0x%x\n", READ_BIT(reg_val,30,23));
    VLOG(INFO,"\t- pipe_on[4]        :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(INFO,"\t- cur_s2ime         :  0x%x\n", READ_BIT(reg_val,19,16));
    VLOG(INFO,"\t- cur_pipe          :  0x%x\n", READ_BIT(reg_val,15,12));
    VLOG(INFO,"\t- pipe_on[3:0]      :  0x%x\n", READ_BIT(reg_val,11, 8));
    VLOG(INFO,"\t- i_grdma_debug_reg :  0x%x\n", READ_BIT(reg_val, 5, 3));
    VLOG(INFO,"\t- cur_ar_tbl_w_fsm  :  0x%x\n", READ_BIT(reg_val, 2, 0));

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug1);
    VLOG(INFO,"\t- i_avc_rdo_debug :  0x%x\n", READ_BIT(reg_val,31,31));
    VLOG(INFO,"\t- curbuf_prp      :  0x%x\n", READ_BIT(reg_val,28,25));
    VLOG(INFO,"\t- curbuf_s2       :  0x%x\n", READ_BIT(reg_val,24,21));
    VLOG(INFO,"\t- curbuf_s0       :  0x%x\n", READ_BIT(reg_val,20,17));
    VLOG(INFO,"\t- cur_s2ime_sel   :  0x%x\n", READ_BIT(reg_val,16,16));
    VLOG(INFO,"\t- cur_mvp         :  0x%x\n", READ_BIT(reg_val,15,14));
    VLOG(INFO,"\t- cmd_ready       :  0x%x\n", READ_BIT(reg_val,13,13));
    VLOG(INFO,"\t- rc_ready        :  0x%x\n", READ_BIT(reg_val,12,12));
    VLOG(INFO,"\t- pipe_cmd_cnt    :  0x%x\n", READ_BIT(reg_val,11, 9));
    VLOG(INFO,"\t- subblok_done    :  LF_PARAM 0x%x SFU 0x%x LF 0x%x RDO 0x%x IMD 0x%x FME 0x%x IME 0x%x\n",
        READ_BIT(reg_val, 6, 6), READ_BIT(reg_val, 5, 5), READ_BIT(reg_val, 4, 4), READ_BIT(reg_val, 3, 3),
        READ_BIT(reg_val, 2, 2), READ_BIT(reg_val, 1, 1), READ_BIT(reg_val, 0, 0));

    reg_val = ReadRegVCE(coreIdx, vcore_idx, debug2);
    //VLOG(INFO,"\t- reserved          :  0x%x\n", READ_BIT(reg_val,31, 23));
    VLOG(INFO,"\t- cur_prp_dma_state :  0x%x\n", READ_BIT(reg_val,22, 20));
    VLOG(INFO,"\t- cur_prp_state     :  0x%x\n", READ_BIT(reg_val,19, 18));
    VLOG(INFO,"\t- main_ctu_xpos     :  0x%x\n", READ_BIT(reg_val,17,  9));
    VLOG(INFO,"\t- main_ctu_ypos     :  0x%x(HEVC:*32, AVC:*16)\n", READ_BIT(reg_val, 8,  0));

    reg_val = ReadRegVCE(coreIdx, vcore_idx, 0x0ae8);
    VLOG(INFO,"\t- sub_frame_sync_ypos_valid :  0x%x\n", READ_BIT(reg_val,0,0));
    VLOG(INFO,"\t- sub_frame_sync_ypos       :  0x%x\n", READ_BIT(reg_val,13,1));
}

void DisplayVceEncDebugMode(int core_idx, int vcore_idx, int set_mode, int* debug)
{
    int reg_val;
    int i;
    VLOG(INFO,"----------- MODE 2 : ----------\n");

    WriteRegVCE(core_idx,vcore_idx, set_mode, 2);

    reg_val = ReadRegVCE(core_idx, vcore_idx, debug[7]);
    VLOG(INFO,"\t- s2fme_info_full    :  0x%x\n", READ_BIT(reg_val,26,26));
    VLOG(INFO,"\t- ime_cmd_ref_full   :  0x%x\n", READ_BIT(reg_val,25,25));
    VLOG(INFO,"\t- ime_cmd_ctb_full   :  0x%x\n", READ_BIT(reg_val,24,24));
    VLOG(INFO,"\t- ime_load_info_full :  0x%x\n", READ_BIT(reg_val,23,23));
    VLOG(INFO,"\t- mvp_nb_info_full   :  0x%x\n", READ_BIT(reg_val,22,22));
    VLOG(INFO,"\t- ime_final_mv_full  :  0x%x\n", READ_BIT(reg_val,21,21));
    VLOG(INFO,"\t- ime_mv_full        :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(INFO,"\t- cur_fme_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val,19,16));
    VLOG(INFO,"\t- cur_s2me_fsm[3:0]  :  0x%x\n", READ_BIT(reg_val,15,12));
    VLOG(INFO,"\t- cur_s2mvp_fsm[3:0] :  0x%x\n", READ_BIT(reg_val,11, 8));
    VLOG(INFO,"\t- cur_ime_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val, 7, 4));
    VLOG(INFO,"\t- cur_sam_fsm[3:0]   :  0x%x\n", READ_BIT(reg_val, 3, 0));

    VLOG(INFO,"----------- MODE 6 : ----------\n");
    WriteRegVCE(core_idx,vcore_idx, set_mode, 6);
    for ( i = 3; i < 10 ; i++ )
    {
        reg_val = ReadRegVCE(core_idx, vcore_idx, debug[i]);
        VLOG(INFO,"\t- mode 6, %08x = %08x\n", debug[i], reg_val);
    }

    VLOG(INFO,"----------- MODE 7 : ----------\n");
    WriteRegVCE(core_idx,vcore_idx, set_mode, 7);
    for ( i = 3; i < 10 ; i++ )
    {
        reg_val = ReadRegVCE(core_idx, vcore_idx, debug[i]);
        VLOG(INFO,"\t- mode 7, %08x = %08x\n", debug[i], reg_val);
    }
}

void DisplayVceEncDebugModeAll(int core_idx, int vcore_idx)
{
    int ii;
    int iMode, iIndex;
    int reg_val;

    int REG_DEBUG[12] = {0, };

    REG_DEBUG[ 0] = 0x0ba0;//MODE SEL //parameter VCE_ENC_DEBUG0            = 9'h1A0;
    REG_DEBUG[ 1] = 0x0ba4;
    REG_DEBUG[ 2] = 0x0ba8;
    REG_DEBUG[ 3] = 0x0bac;
    REG_DEBUG[ 4] = 0x0bb0;
    REG_DEBUG[ 5] = 0x0bb4;
    REG_DEBUG[ 6] = 0x0bb8;
    REG_DEBUG[ 7] = 0x0bbc;
    REG_DEBUG[ 8] = 0x0bc0;
    REG_DEBUG[ 9] = 0x0bc4;
    REG_DEBUG[10] = 0x0bc8;
    REG_DEBUG[11] = 0x0bcc;

    for( ii=0; ii<2; ii++ )
    {
        for( iMode = 0; iMode < 8; iMode++ )
        {
            WriteRegVCE(core_idx, vcore_idx, REG_DEBUG[0], iMode);
            VLOG(INFO,"-----------  VCE Scan with mode:%d  ----------.\n", iMode );

            for( iIndex = 0; iIndex < 12; iIndex++ )
            {
                reg_val = ReadRegVCE(core_idx, vcore_idx, REG_DEBUG[iIndex]);
                VLOG(INFO,"\t- debug[%2d] : 0x%x\n", iIndex, reg_val );
            }
        }
    }

    ///---- read backbone reg
    ///-- ADR_EMPTY_FLAG
    reg_val = vdi_fio_read_register(core_idx, 0xFE14);
    VLOG(INFO,"ADR_EMPTY_FLAG = 0x%x   \n",reg_val);
    VLOG(INFO,"ADR_EMPTY_FLAG = 0x%x   \n",reg_val);
}


#define VCE_BUSY                   0xA04
#define VCE_LF_PARAM               0xA6c
#define VCE_BIN_WDMA_CUR_ADDR      0xB1C
#define VCE_BIN_PIC_PARAM          0xB20
#define VCE_BIN_WDMA_BASE          0xB24
#define VCE_BIN_WDMA_END           0xB28
void DisplayVceEncReadVCE(int coreIdx, int vcore_idx)
{
    int reg_val;

    VLOG(INFO, "---------------DisplayVceEncReadVCE-----------------\n");
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BUSY);
    VLOG(INFO,"\t- VCE_BUSY                 :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_LF_PARAM);
    VLOG(INFO,"\t- VCE_LF_PARAM             :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_CUR_ADDR);
    VLOG(INFO,"\t- VCE_BIN_WDMA_CUR_ADDR    :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_PIC_PARAM);
    VLOG(INFO,"\t- VCE_BIN_PIC_PARAM        :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_BASE);
    VLOG(INFO,"\t- VCE_BIN_WDMA_BASE        :  0x%x\n", reg_val);
    reg_val = ReadRegVCE(coreIdx, vcore_idx, VCE_BIN_WDMA_END);
    VLOG(INFO,"\t- VCE_BIN_WDMA_END         :  0x%x\n", reg_val);
}


void PrintWave5xxDecSppStatus(
    Uint32 coreIdx
    )
{
    Uint32  regVal;
    //DECODER SDMA INFO
    VLOG(WARN,"[+] SDMA REG Dump\n");
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5000);
    VLOG(WARN,"C_SDMA_LOAD_CMD      = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5004);
    VLOG(WARN,"C_SDMA_AUTO_MODE     = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5008);
    VLOG(WARN,"C_SDMA_START_ADDR    = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x500C);
    VLOG(WARN,"C_SDMA_END_ADDR      = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5010);
    VLOG(WARN,"C_SDMA_ENDIAN        = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5014);
    VLOG(WARN,"C_SDMA_IRQ_CLEAR     = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5018);
    VLOG(WARN,"C_SDMA_BUSY          = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x501C);
    VLOG(WARN,"C_SDMA_LAST_ADDR     = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5020);
    VLOG(WARN,"C_SDMA_SC_BASE_ADDR  = 0x08%x\n",regVal);
    VLOG(WARN,"[-] SMDA REG Dump\n");

    VLOG(WARN,"[+] SHU REG Dump\n");
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5400);
    VLOG(WARN,"C_SHU_INIT           = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5404);
    VLOG(WARN,"C_SHU_SEEK_NXT_NAL   = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x540C);
    VLOG(WARN,"C_SHU_RD_NAL_ADDR    = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x540C);
    VLOG(WARN,"C_SHU_STATUS         = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5478);
    VLOG(WARN,"C_SHU_REMAIN_BYTE    = 0x%08x\n",regVal);
    VLOG(WARN,"[-] SHU REG Dump\n");

    VLOG(WARN,"[+] GBU REG Dump\n");
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5800);
    VLOG(WARN,"C_GBU_INIT           = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5804);
    VLOG(WARN,"GBU_STATUS           = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5808);
    VLOG(WARN,"GBU_TCNT             = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x580C);
    VLOG(WARN,"GBU_NCNT             = 0x%08x\n",regVal);
    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x5478);
    VLOG(WARN,"GBU_REMAIN_BIT       = 0x%08x\n",regVal);
    VLOG(WARN,"[-] GBU REG Dump\n");
}


void PrintWave5xxDecPrescanStatus(
    Uint32 coreIdx
    )
{
    Uint32  regVal;
    //DECODER SDMA INFO
    VLOG(WARN,"[+] PRESCAN REG Dump\n");

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x61a0);
    VLOG(WARN,"V_PRESCAN_CQ_BS_START_ADDR   = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x61a4);
    VLOG(WARN,"V_PRESCAN_CQ_BS_END_ADDR     = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x61ac);
    VLOG(WARN,"V_PRESCAN_CQ_DEC_CODEC_STD   = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x6200);
    VLOG(WARN,"V_PRESCAN_AVC_SEQ_PARAM      = 0x%08x\n",regVal);

    regVal = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x6204);
    VLOG(WARN,"V_PRESCAN_AVC_PIC_PARAM     = 0x%08x\n",regVal);

    VLOG(WARN,"[-] PRESCAN REG Dump\n");
}



void PrintDecVpuStatus(DecHandle handle)
{
    Int32       coreIdx;
    Uint32      product_code = 0, product_id =0;
    BOOL        running = FALSE;

    if (NULL == handle) {
        VLOG(ERR, "<%s:%d> Handle is NULL\n", __FUNCTION__, __LINE__);
        return;
    }
    coreIdx = VPU_HANDLE_CORE_INDEX(handle);
    product_id = VPU_HANDLE_PRODUCT_ID(handle);

    if (TRUE == PRODUCT_ID_CODA_SERIES(product_id)) {
        running = (BOOL)(GetPendingInst(coreIdx) == handle);
        if (running == FALSE)
            SetClockGate(coreIdx, TRUE);
    } else {
        SetClockGate(coreIdx, TRUE);
    }

    vdi_print_vpu_status(coreIdx);

    if (PRODUCT_ID_W_SERIES(handle->productId)) {
        product_code = VpuReadReg(coreIdx, VPU_PRODUCT_CODE_REGISTER);
        if ((WAVE517_CODE == product_code) || (WAVE537_CODE == product_code))
        {
            vdi_print_vpu_status_dec(coreIdx);
        }
    }

    if (TRUE == PRODUCT_ID_CODA_SERIES(handle->productId)) {
        if (running == FALSE)
            SetClockGate(coreIdx, FALSE);
    } else {
         SetClockGate(coreIdx, FALSE);
    }
}

void PrintEncVpuStatus(EncHandle   handle)
{
    Int32       coreIdx;
    coreIdx   = handle->coreIdx;

    SetClockGate(coreIdx, 1);
    vdi_print_vpu_status(coreIdx);
    vdi_print_vpu_status_enc(coreIdx);
    SetClockGate(coreIdx, 0);
}


void HandleEncoderError(
    EncHandle       handle,
    Uint32          encPicCnt,
    EncOutputInfo*  outputInfo
    )
{
/*lint -save -e527 */
    Uint32 productCode;
    Uint32 coreIdx = VPU_HANDLE_CORE_INDEX(handle);

    productCode = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);
    /* Dump VCPU status registers */
    if (PRODUCT_CODE_W6_SERIES(productCode)) {
        wave6xx_vcpu_status(coreIdx, DEBUG_CODEC_ENC);
        wave6xx_vcore_status(coreIdx, DEBUG_CODEC_ENC);
    } else if (PRODUCT_CODE_W5_SERIES(productCode)) {
        PrintEncVpuStatus(handle);
    }
    /*lint -restore */
}

void DumpMemory(const char* path, Uint32 coreIdx, PhysicalAddress addr, Uint32 size, EndianMode endian)
{
    osal_file_t   ofp;
    Uint8*  buffer;

    VLOG(INFO,"DUMP MEMORY ADDR(0x%08x) SIZE(%d) FILE(%s)\n", addr, size, path);
    if (NULL == (ofp=osal_fopen(path, "wb"))) {
        VLOG(ERR,"[FAIL]\n");
        return;
    }

    if (NULL == (buffer=(Uint8*)osal_malloc(size))) {
        VLOG(ERR, "<%s:%d> Failed to allocate memory(%d)\n", __FUNCTION__, __LINE__, size);
        return;
    }

    VpuReadMem(coreIdx, addr, buffer, size, endian);
    osal_fwrite(buffer, 1, size, ofp);
    osal_free(buffer);
    osal_fclose(ofp);
}

void DumpCodeBuffer(const char* path)
{
    Uint8*          buffer;
    vpu_buffer_t    vb;
    PhysicalAddress addr;
    osal_file_t     ofp;

    buffer = (Uint8*)osal_malloc(WAVE5_MAX_CODE_BUF_SIZE);
    if ((ofp=osal_fopen(path, "wb")) == NULL) {
        VLOG(ERR,"DUMP CODE AREA into %s FAILED", path);
    }
    else {
        vdi_get_common_memory(0, &vb);

        addr   = vb.phys_addr;
        VpuReadMem(0, addr, buffer, WAVE5_MAX_CODE_BUF_SIZE, VDI_128BIT_LITTLE_ENDIAN);
        osal_fwrite(buffer, 1, WAVE5_MAX_CODE_BUF_SIZE, ofp);
        osal_fclose(ofp);
        VLOG(ERR,"DUMP CODE AREA into %s OK", path);
    }
    osal_free(buffer);
}

void DumpBitstreamBuffer(Uint32 coreIdx, PhysicalAddress addr, Uint32 size, EndianMode endian, const char* prefix)
{
    char    path[1024];
    osal_file_t   ofp;

    osal_sprintf(path, "./%s_dump_bitstream_buffer.bin", prefix);
    if ((ofp=osal_fopen(path, "wb")) == NULL) {
        VLOG(ERR,"DUMP BITSTREAMBUFFER into %s [FAIL]", path);
    }
    else {
        if (size > 0) {
            Uint8* buffer = (Uint8*)osal_malloc(size);
            if (NULL == buffer) {
                VLOG(ERR, "<%s:%d> Failed to allocate memory(%d)\n", __FUNCTION__, __LINE__, size);
                return;
            }
            VpuReadMem(coreIdx, addr, buffer, size, endian);
            osal_fwrite(buffer, 1, size, ofp);
            osal_free(buffer);
        }
        else {
            VLOG(ERR,">> NO BITSTREAM BUFFER\n");
        }
        osal_fclose(ofp);
        VLOG(INFO,"DUMP BITSTREAMBUFFER into %s [OK]\n", path);
    }
}

void DumpColMvBuffers(Uint32 coreIdx, const DecInfo* pDecInfo)
{
    char    path[MAX_FILE_PATH];
    osal_file_t   ofp;
    Uint32  idx = 0;
    Uint32  mvCnt = pDecInfo->numFbsForDecoding;
    Uint32  mvSize = pDecInfo->vbMV[0].size;
    Uint8*  buffer;

    buffer = (Uint8*)osal_malloc(mvSize);
    osal_memset(buffer, 0x00, mvSize);

    if (NULL == buffer) {
        VLOG(ERR,"%s:%d MEM Alloc Failure", __FUNCTION__, __LINE__);
        return;
    }

    for (idx = 0; idx < mvCnt; idx++) {
        if (0 < mvSize) {
            osal_sprintf(path, "Dump_CovMV_buffer_%d.bin", idx);

            if ((ofp=osal_fopen(path, "wb")) == NULL) {
                VLOG(ERR,"DUMP MVCOL BUFFER into %s [FAIL]", path);
                return ;
            }
            VpuReadMem(coreIdx, pDecInfo->vbMV[idx].phys_addr, buffer, mvSize, VDI_128BIT_LITTLE_ENDIAN);
            osal_fwrite(buffer, 1, mvSize, ofp);
            osal_fclose(ofp);
        }
    }
    osal_free(buffer);
}


void HandleDecoderError(
    DecHandle       handle,
    Uint32          frameIdx,
    DecOutputInfo*  outputInfo
    )
{
    UNREFERENCED_PARAMETER(handle);
    UNREFERENCED_PARAMETER(outputInfo);
}

void PrintMemoryAccessViolationReason(
    Uint32          coreIdx,
    void            *outp
    )
{
    UNREFERENCED_PARAMETER(coreIdx);
    UNREFERENCED_PARAMETER(outp);
}

/**
* \brief           Handle error cases depending on product
* \return  -1      SEQUENCE ERROR
*/
Int32 HandleDecInitSequenceError(DecHandle handle, Uint32 productId, DecOpenParam* openParam, DecInitialInfo* seqInfo, RetCode apiErrorCode)
{
    if (apiErrorCode == RETCODE_MEMORY_ACCESS_VIOLATION) {
        PrintMemoryAccessViolationReason(handle->coreIdx, NULL);
        return -1;
    }

    if (PRODUCT_ID_W_SERIES(productId)) {
        if (seqInfo->seqInitErrReason == HEVC_ETCERR_INIT_SEQ_SPS_NOT_FOUND) {
            return -2;
        } else {
            if (seqInfo->seqInitErrReason == HEVC_SPECERR_OVER_PICTURE_WIDTH_SIZE) {
                VLOG(ERR, "Not supported picture width: MAX_SIZE(8192): %d\n", seqInfo->picWidth);
            }
            if (seqInfo->seqInitErrReason == HEVC_SPECERR_OVER_PICTURE_HEIGHT_SIZE) {
                VLOG(ERR, "Not supported picture height: MAX_SIZE(8192): %d\n", seqInfo->picHeight);
            }
            if (seqInfo->seqInitErrReason == HEVC_SPECERR_OVER_CHROMA_FORMAT) {
                VLOG(ERR, "Not supported chroma idc: %d\n", seqInfo->chromaFormatIDC);
            }
            if (seqInfo->seqInitErrReason == HEVC_SPECERR_OVER_BIT_DEPTH) {
                VLOG(ERR, "Not supported Luma or Chroma bitdepth: L(%d), C(%d)\n", seqInfo->lumaBitdepth, seqInfo->chromaBitdepth);
            }
            if (seqInfo->warnInfo == HEVC_SPECWARN_OVER_PROFILE) {
                VLOG(INFO, "SPEC over profile: %d\n", seqInfo->profile);
            }
            if (seqInfo->warnInfo == HEVC_ETCWARN_INIT_SEQ_VCL_NOT_FOUND) {
                VLOG(INFO, "VCL Not found : RD_PTR(0x%08x), WR_PTR(0x%08x)\n", seqInfo->rdPtr, seqInfo->wrPtr);
            }
            return -1;
        }
    }
    else {
        if (openParam->bitstreamMode == BS_MODE_PIC_END && (seqInfo->seqInitErrReason&(1UL<<31))) {
            VLOG(ERR, "SEQUENCE HEADER NOT FOUND\n");
            return -1;
        }
        else {
            return -1;
        }
    }
}

void print_busy_timeout_status(Uint32 coreIdx, Uint32 product_code, Uint32 pc)
{
    if (PRODUCT_CODE_W_SERIES(product_code)) {
        vdi_print_vpu_status(coreIdx);
        if (product_code == 0) //0:enc core; 1,2:dec core
            vdi_print_vpu_status_enc(coreIdx);
        else
            vdi_print_vpu_status_dec(coreIdx);
    } else {
        Uint32 idx;
        for (idx=0; idx<20; idx++) {
            VLOG(ERR, "[VDI] vdi_wait_vpu_busy timeout, PC=0x%lx\n", vdi_read_register(coreIdx, pc));
            {
                Uint32 vcpu_reg[31]= {0,};
                int i;
                // --------- VCPU register Dump
                VLOG(INFO, "[+] VCPU REG Dump\n");
                for (i = 0; i < 25; i++)
                {
                    vdi_write_register(coreIdx, 0x14, (1 << 9) | (i & 0xff));
                    vcpu_reg[i] = vdi_read_register(coreIdx, 0x1c);

                    if (i < 16)
                    {
                        VLOG(INFO, "0x%08x\t", vcpu_reg[i]);
                        if ((i % 4) == 3)
                            VLOG(INFO, "\n");
                    }
                    else
                    {
                        switch (i)
                        {
                        case 16:
                            VLOG(INFO, "CR0: 0x%08x\t", vcpu_reg[i]);
                            break;
                        case 17:
                            VLOG(INFO, "CR1: 0x%08x\n", vcpu_reg[i]);
                            break;
                        case 18:
                            VLOG(INFO, "ML:  0x%08x\t", vcpu_reg[i]);
                            break;
                        case 19:
                            VLOG(INFO, "MH:  0x%08x\n", vcpu_reg[i]);
                            break;
                        case 21:
                            VLOG(INFO, "LR:  0x%08x\n", vcpu_reg[i]);
                            break;
                        case 22:
                            VLOG(INFO, "PC:  0x%08x\n", vcpu_reg[i]);
                            break;
                        case 23:
                            VLOG(INFO, "SR:  0x%08x\n", vcpu_reg[i]);
                            break;
                        case 24:
                            VLOG(INFO, "SSP: 0x%08x\n", vcpu_reg[i]);
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
    }
}

void wave5xx_vcore_status(
    Uint32 coreIdx
    )
{
    Uint32 i;
    Uint32 temp;

    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                            VCORE BPU STATUS                        -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");

    VLOG(WARN,"[+] BPU REG Dump\n");
    for (i=0;i < 20; i++) {
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x18));
        VLOG(WARN,"BITPC = 0x%08x\n", temp);
    }

/*
    VLOG(WARN, "r0 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x0) );
    VLOG(WARN, "r1 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x1) );
    VLOG(WARN, "r2 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x2) );
    VLOG(WARN, "r3 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x3) );

    VLOG(WARN, "r4 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x4) );
    VLOG(WARN, "r5 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x5) );
    VLOG(WARN, "r6 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x6) );
    VLOG(WARN, "r7 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x7) );

    VLOG(WARN, "stack0 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x10) );
    VLOG(WARN, "stack1 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x11) );
    VLOG(WARN, "stack2 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x12) );
    VLOG(WARN, "stack3 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x13) );

    VLOG(WARN, "stack4 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x14) );
    VLOG(WARN, "stack5 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x15) );
    VLOG(WARN, "stack6 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x16) );
    VLOG(WARN, "stack7 : 0x%08x \n", vdi_irb_read_register(coreIdx, 0, 0x17) );
*/

    VLOG(WARN,"[+] BPU Debug message REG Dump\n");
    VLOG(WARN,"[MSG_0:0x%08x], [MSG_1:0x%08x],[MSG_2:0x%08x],[MSG_3:0x%08x],[MSG_4:0x%08x],[MSG_5:0x%08x] \n",
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1A8),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1AC),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B0),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B4),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B8),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1BC));

    VLOG(WARN,"[-] BPU Debug message REG Dump\n");
    VLOG(WARN,"[+] BPU interface REG Dump\n");
    for(i = 0x8000; i < 0x80FC; i += 16) {
        VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(WARN,"[-] BPU interfrace REG Dump\n");



    VLOG(WARN,"[+] MIB REG Dump\n");
    temp  = vdi_irb_read_register(coreIdx, 0, 0x110);
    VLOG(WARN,"MIB_EXTADDR        : 0x%08x , External base address \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x111);
    VLOG(WARN,"MIB_INT_ADDR       : 0x%08x , Internal base address (MIBMEM) \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x112);
    VLOG(WARN,"MIB_DATA_CNT       : 0x%08x , Length (8-byte unit) \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x113);
    VLOG(WARN,"MIB_COMMAND        : 0x%08x , COMMAND[load, save] \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x114);
    VLOG(WARN,"MIB_BUSY           : 0x%08x , Busy status \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x116);
    VLOG(WARN,"MIB_WREQ           : 0x%08x , Write response done \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x117);
    VLOG(WARN,"MIB_BUSID          : 0x%08x , GDI bus ID for core \n", temp);
    VLOG(WARN,"[-] MIB REG Dump\n");

    VLOG(WARN,"[+] RDMA REG Dump\n");
    temp  = vdi_irb_read_register(coreIdx, 0, 0x120);
    VLOG(WARN,"RDMA_WR_SEL          : 0x%08x , [0] : selection flag for writing register, 0 - for GBIN0, 1- for GBIN1 \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x121);
    VLOG(WARN,"RDMA_RD_SEL          : 0x%08x , [0] : selection flag for reading register,  \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x122);
    VLOG(WARN,"RDMA_INIT            : 0x%08x , (WO) 1 - init RDMA, (RO) 1 - init_busy during RDMA initialize  \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x123);
    VLOG(WARN,"RDMA_LOAD_CMD        : 0x%08x , [0] auto_mode,[1] manual_mode  \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x125);
    VLOG(WARN,"RDMA_BASE_ADDR       : 0x%08x , Base address after init, should be 16byte align \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x126);
    VLOG(WARN,"RDMA_END_ADDR        : 0x%08x , RDMA end address, if current >= rdma end addr, empty intterupt is occrured \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x127);
    VLOG(WARN,"RDMA_ENDIAN          : 0x%08x , ENDIAN setting for RDMA \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x128);
    VLOG(WARN,"RDMA_CUR_ADDR        : 0x%08x , RDMA current addr, after loading, current addr is increased with load Bytes \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x129);
    VLOG(WARN,"RDMA_STATUS          : 0x%08x , [0] if 1, RMDA busy [30:28] load command count [31] if 1, bin_rmda_empty \n", temp);
    temp  = vdi_irb_read_register(coreIdx, 0, 0x12A);
    VLOG(WARN,"RDMA_DBG_INFO        : 0x%08x , RDMA debug info \n", temp);
    VLOG(WARN,"[+] RDMA REG Dump\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                            VCORE STATUS                              -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    // --------- VCE register Dump
    VLOG(WARN,"[+] VCE REG Dump Core0\n");
    for (i=0x000; i<0x1fc; i+=16) {
        VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
            ReadRegVCE(coreIdx, 0, (i+0x00)),
            ReadRegVCE(coreIdx, 0, (i+0x04)),
            ReadRegVCE(coreIdx, 0, (i+0x08)),
            ReadRegVCE(coreIdx, 0, (i+0x0c)));
    }
    VLOG(WARN,"[-] VCE REG Dump\n");
}


void wave5xx_PP_status(
    Uint32 coreIdx
    )
{
    Uint32 temp;

    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            PP STATUS                               -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");

    VLOG(INFO,"[+] PP COMMAND REG Dump\n");
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x0);
    VLOG(INFO,"PP_CMD               : 0x%08x , [0] pic_init_c, [1] soft_reset_c \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x4);
    VLOG(INFO,"PP_CTB_SIZE          : 0x%08x , 0:CTB16, 1:CTB32, 2:CTB64, 3:CTB128 \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x48);
    VLOG(INFO,"PP_LF_CRTL           : 0x%08x , [0] bwb_chroma_out_disable \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x4C);
    VLOG(INFO,"PP_BWB_OUT_FORMAT    : 0x%08x , \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x50);
    VLOG(INFO,"PP_BWB_STRIDE        : 0x%08x , [15: 0] bwb_chr_stride [31:16] bwb_lum_stride \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x54);
    VLOG(INFO,"PP_BWB_SIZE          : 0x%08x , [15: 0] bwb_pic_height [31:16] bwb_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x80);
    VLOG(INFO,"PP_STATUS            : 0x%08x \n ", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x84);
    VLOG(INFO,"PP_BWB_DEBUG0        : 0x%08x \n ", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x88);
    VLOG(INFO,"PP_BWB_DEBUG1        : 0x%08x \n ", temp);
    VLOG(INFO,"[-] PP COMMAND REG Dump\n");

    VLOG(INFO,"[+] PP SCALER REG Dump\n");
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x14);
    VLOG(INFO,"PP_SCL_PARAM         : 0x%08x , 0:scaler enable \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x18);
    VLOG(INFO,"PP_SCL_IN_DISP_SIZE  : 0x%08x , [15: 0] scl_in_disp_pic_height, [31:16] scl_in_disp_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x1C);
    VLOG(INFO,"PP_SCL_IN_SIZE       : 0x%08x , [15: 0] scl_in_pic_height, [31:16] scl_in_disp_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x20);
    VLOG(INFO,"PP_SCL_OUT_SIZE      : 0x%08x , [15: 0] scl_out_pic_height, [31:16] scl_out_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x20);
    VLOG(INFO,"PP_SCL_OUT_SIZE      : 0x%08x , [15: 0] scl_out_pic_height, [31:16] scl_out_pic_width \n", temp);
    VLOG(INFO,"[-] PP SCALER REG Dump\n");

    VLOG(INFO,"[+] PP IFBC REG Dump\n");
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x58);
    VLOG(INFO,"IFBC_NO_MORE_REQ     : 0x%08x , [0] ifbc_no_more_req [31] soft_reset_ready  \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x5C);
    VLOG(INFO,"IFBC_PICSIZE         : 0x%08x , [15: 0] ifbc_pic_height [31:16]ifbc_pic_width \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x60);
    VLOG(INFO,"IFBC_BYTE_FORMAT     : 0x%08x , [1:0] ifbc_byte_format \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x64);
    VLOG(INFO,"IFBC_RUN_MODE        : 0x%08x , [0] ifbc_enable [1] pp_out_disable \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x68);
    VLOG(INFO,"IFBC_Y_BASE          : 0x%08x , [25:0] base_ifbc_y \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x6C);
    VLOG(INFO,"IFBC_UV_BASE         : 0x%08x , [25:0] base_ifbc_uv \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x70);
    VLOG(INFO,"IFBC_TILE_INFO       : 0x%08x \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x74);
    VLOG(INFO,"IFBC_PIC_WIDTH_MOD96 : 0x%08x \n", temp);
    temp  = vdi_fio_read_register(coreIdx, W5_REG_BASE + 0xD000 + 0x78);
    VLOG(INFO,"IFBC_TILE_STRIDE     : 0x%08x \n", temp);
    VLOG(INFO,"[-] PP IFBC REG Dump\n");
}


void wave5xx_mismatch_vcore_status(
    Uint32 coreIdx
    )
{
    Uint32 i;
    Uint32 temp;

    VLOG(INFO,"-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCORE XXX STATUS                        -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");

    VLOG(INFO,"[+] BPU REG Dump\n");
    for (i=0;i < 20; i++) {
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x18));
        VLOG(INFO,"BITPC = 0x%08x\n", temp);
    }
    VLOG(INFO,"[-] VCE REG Dump\n");
}

void wave5xx_bpu_status(
    Uint32 coreIdx
    )
{
    Uint32 i;
    Uint32 temp;

    VLOG(INFO,"[+] BPU REG Dump\n");
    for (i=0;i < 20; i++) {
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x18));
        VLOG(INFO,"BITPC = 0x%08x\n", temp);
    }
    for (i=0; i < 44; i+=4) {
        //0x40 ~ 0x68, DEBUG_BPU_INFO_0 ~ DEBUG_BPU_INFO_A
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x40 + i));
        VLOG(INFO,"DEBUG_BPU_INFO_%x = 0x%08x\n", (i/4), temp);
    }

    temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x30));
    VLOG(INFO,"BIT_BUSY Core0=0x%08x \n", temp);

    for (i=0; i < 8; i += 4 ) {
        temp = vdi_fio_read_register(coreIdx, (W5_REG_BASE + 0x8000 + 0x80 + i));
        VLOG(INFO,"stack[%d] Core0=0x%08x\n", temp);
    }



    for(i = 0x8000; i < 0x81FC; i += 16) {
        VLOG(INFO,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }

    VLOG(INFO,"[-] BPU REG Dump\n");

    VLOG(INFO,"[+] MIB REG Dump\n");
    for (i=0x110 ; i < 0x118 ; i++) {
        temp  = vdi_irb_read_register(coreIdx, 0, i);
        VLOG(INFO,"MIB 0x%08x Core0=0x%08x\n", i, temp);
    }
    VLOG(INFO,"[-] MIB REG Dump\n");


    VLOG(INFO,"[-] BPU MSG REG Dump\n");

    VLOG(INFO,"[MSG_0:0x%08x], [MSG_1:0x%08x],[MSG_2:0x%08x],[MSG_3:0x%08x],[MSG_4:0x%08x],[MSG_5:0x%08x] \n",
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1A8),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1AC),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B0),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B4),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1B8),
        vdi_fio_read_register(coreIdx, W5_REG_BASE + 0x8000 + 0x1BC));

    VLOG(INFO,"[-] BPU MSG REG Dump\n");

}


void vdi_print_vpu_status_enc(unsigned long coreIdx)
{
    int       vce_enc_debug[12] = {0, };
    int       set_mode;
    int       vcore_num;
    int i;

    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                           Encoder only                                                         -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"BS_OPT: 0x%08x\n", VpuReadReg(coreIdx, W5_BS_OPTION));

    VLOG(WARN,"[+] VCPU DMA Dump\n");
    for(i = 0x2000; i < 0x2018; i += 16) {
        VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(WARN,"[-] VCPU DMA Dump\n");

    VLOG(WARN,"[+] VCPU HOST REG Dump\n");
    for(i = 0x3000; i < 0x30fc; i += 16) {
        VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(WARN,"[-] VCPU HOST REG Dump\n");

    VLOG(WARN,"[+] VCPU ENT ENC REG Dump\n");
    for(i = 0x6800; i < 0x7000; i += 16) {
        VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(WARN,"[-] VCPU ENT ENC REG Dump\n");

    VLOG(WARN,"[+] VCPU HOST MEM Dump\n");
    for(i = 0x7000; i < 0x70fc; i += 16) {
        VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
            vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
    }
    VLOG(WARN,"[-] VCPU SPP Dump\n");

    VLOG(WARN,"vce run flag = %d\n", VpuReadReg(coreIdx, 0x1E8));


    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                            VCE DUMP(ENC)                           -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    vce_enc_debug[0] = 0x0ba0;//MODE SEL //parameter VCE_ENC_DEBUG0            = 9'h1A0;
    vce_enc_debug[1] = 0x0ba4;
    vce_enc_debug[2] = 0x0ba8;
    vce_enc_debug[3] = 0x0bac;
    vce_enc_debug[4] = 0x0bb0;
    vce_enc_debug[5] = 0x0bb4;
    vce_enc_debug[6] = 0x0bb8;
    vce_enc_debug[7] = 0x0bbc;
    vce_enc_debug[8] = 0x0bc0;
    vce_enc_debug[9] = 0x0bc4;
    set_mode              = 0x0ba0;
    vcore_num            = 1;

#if 0
    for (vcore_idx = 0; vcore_idx < vcore_num ; vcore_idx++) {
        VLOG(WARN,"==========================================\n");
        VLOG(WARN,"[+] VCE REG Dump VCORE_IDX : %d\n",vcore_idx);
        VLOG(WARN,"==========================================\n");
        DisplayVceEncReadVCE             (coreIdx, vcore_idx);
        DisplayVceEncDebugCommon521      (coreIdx, vcore_idx, set_mode, vce_enc_debug[0], vce_enc_debug[1], vce_enc_debug[2]);
        DisplayVceEncDebugMode          (coreIdx, vcore_idx, set_mode, vce_enc_debug);
        DisplayVceEncDebugModeAll       (coreIdx, vcore_idx);
    }
#endif
}

void vdi_print_vpu_status_dec(unsigned long coreIdx)
{
    Uint32 i;
    Int32 product_id = VPU_GetProductId(coreIdx);

    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                           Decoder only                             -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");

    /// -- VCPU ENTROPY PERI DECODE Common
    VLOG(WARN,"[+] VCPU ENT DEC REG Dump\n");
    if (PRODUCT_ID_W_SERIES(product_id)) {
        if (PRODUCT_ID_517 == product_id) {
            Uint32 reg_val[4] = {0,};
            for(i = 0x6000; i < 0x6800; i += 16) {
                reg_val[0] = vdi_fio_read_register(coreIdx, (W5_REG_BASE + i));
                reg_val[1] = vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4));
                if (0x61F0 != i) {
                    reg_val[2] = vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8));
                    reg_val[3] = vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12));
                } else {
                    reg_val[2] = 0; //0x61F8
                    reg_val[3] = 0; //0x61FC
                }
                if (i == 0x6020 || i == 0x6030 || i == 0x6040) {
                    VLOG(WARN,"-HEVC_INFO start-\n");
                    VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
                        reg_val[0],
                        reg_val[1],
                        reg_val[2],
                        reg_val[3]);
                    VLOG(WARN,"-HEVC_INFO DONE-\n");
                } else if (i == 0x6220) {
                    VLOG(WARN,"-AVC_INFO start-\n");
                    VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
                        reg_val[0],
                        reg_val[1],
                        reg_val[2],
                        reg_val[3]);
                    VLOG(WARN,"-AVC_INFO DONE-\n");

                } else {
                    VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
                        reg_val[0],
                        reg_val[1],
                        reg_val[2],
                        reg_val[3]);
                }
            }
        } else {
            for(i = 0x6000; i < 0x6800; i += 16) {
                VLOG(WARN,"0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", (W5_REG_BASE + i),
                    vdi_fio_read_register(coreIdx, (W5_REG_BASE + i)),
                    vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 4 )),
                    vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 8 )),
                    vdi_fio_read_register(coreIdx, (W5_REG_BASE + i + 12)));
            }
        }
    }
    VLOG(WARN,"[-] VCPU ENT DEC REG Dump\n");
}


void wave6xx_vcpu_status(unsigned long coreIdx, DEBUG_CODEC_TYPE codec_type)
{
    Uint32 i;
    Uint32 vcpu_reg[31]= {0,};
    Uint32 regVal;
    VLOG(INFO, "-------------------------------------------------------------------------------\n");
    VLOG(INFO,"------                            VCPU CORE STATUS                        -----\n");
    VLOG(INFO,"-------------------------------------------------------------------------------\n");

    regVal = VpuReadReg(coreIdx, W6_CMD_QUEUE_FULL_IDC);
    VLOG(TRACE, "QUE_FULL_IDC=0x%x \n", regVal);
    regVal = VpuReadReg(coreIdx, W6_CMD_QUEUE_FULL_IDC + 4);
    VLOG(TRACE, "RET_EMPTY_IDC=0x%x \n", regVal);
    VLOG(TRACE, "W6_VPU_SUB_FRAME_SYNC_CTRL=%x\n", VpuReadReg(coreIdx, W6_VPU_SUB_FRAME_SYNC_CTRL));
    for (i = 0; i < 10; i++) {
        VLOG(INFO, "PC = %x\n", VpuReadReg(coreIdx, W5_VCPU_CUR_PC));
    }
    // --------- VCPU register Dump
    VLOG(INFO,"[+] VCPU REG Dump\n");
    for (i = 0; i < 25; i++) {
        VpuWriteReg (coreIdx, W6_VPU_PDBG_IDX_REG, (1<<9) | (i & 0xff));
        vcpu_reg[i] = VpuReadReg (coreIdx, W6_VPU_PDBG_RDATA_REG);

        if (i < 16) {
            VLOG(INFO,"0x%08x\t",  vcpu_reg[i]);
            if ((i % 4) == 3) VLOG(INFO,"\n");
        }
        else {
            switch (i) {
            case 16: VLOG(INFO,"CR0: 0x%08x\t", vcpu_reg[i]); break;
            case 17: VLOG(INFO,"CR1: 0x%08x\n", vcpu_reg[i]); break;
            case 18: VLOG(INFO,"ML:  0x%08x\t", vcpu_reg[i]); break;
            case 19: VLOG(INFO,"MH:  0x%08x\n", vcpu_reg[i]); break;
            case 21: VLOG(INFO,"LR:  0x%08x\n", vcpu_reg[i]); break;
            case 22: VLOG(INFO,"PC:  0x%08x\n", vcpu_reg[i]); break;
            case 23: VLOG(INFO,"SR:  0x%08x\n", vcpu_reg[i]); break;
            case 24: VLOG(INFO,"SSP: 0x%08x\n", vcpu_reg[i]); break;
            default: break;
            }
        }
    }
    regVal = VpuReadReg(coreIdx, 0x010C);
    VLOG(INFO, "VCPU Backbone busy flag: 0x%08x\n", regVal);
    VLOG(INFO,"[-] VCPU REG Dump\n");
}

void wave6xx_vcore_status(unsigned long coreIdx, DEBUG_CODEC_TYPE codec_type)
{
    Uint32 reg_val;
    Uint32 reg_offset = 0;


    reg_val = vdi_fio_read_register(coreIdx, 0x620C);
    VLOG(INFO, "Core0 Backbone busy flag: 0x%08x\n", reg_val);
    reg_val = vdi_fio_read_register(coreIdx, 0xA20C);
    VLOG(INFO, "Core1 Backbone busy flag: 0x%08x\n", reg_val);

    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_CTL_STATUS + reg_offset);
    VLOG(INFO,"- VCE_CTL_STATUS = %08x\n", reg_val);
    VLOG(INFO,"\t- block_busy_SRC        :  0x%x\n", READ_BIT(reg_val,31,31));
    VLOG(INFO,"\t- block_busy_CME        :  0x%x\n", READ_BIT(reg_val,30,30));
    VLOG(INFO,"\t- block_busy_RHU_L0_Y   :  0x%x\n", READ_BIT(reg_val,29,29));
    VLOG(INFO,"\t- block_busy_RHU_L0_C   :  0x%x\n", READ_BIT(reg_val,28,28));
    VLOG(INFO,"\t- block_busy_RHU_L1_Y   :  0x%x\n", READ_BIT(reg_val,27,27));
    VLOG(INFO,"\t- block_busy_RHU_L1_C   :  0x%x\n", READ_BIT(reg_val,26,26));
    VLOG(INFO,"\t- block_busy_RME        :  0x%x\n", READ_BIT(reg_val,25,25));
    VLOG(INFO,"\t- block_busy_FME        :  0x%x\n", READ_BIT(reg_val,24,24));
    VLOG(INFO,"\t- block_busy_IMD        :  0x%x\n", READ_BIT(reg_val,23,23));
    VLOG(INFO,"\t- block_busy_RDO        :  0x%x\n", READ_BIT(reg_val,22,22));
    VLOG(INFO,"\t- block_busy_REC        :  0x%x\n", READ_BIT(reg_val,21,21));
    VLOG(INFO,"\t- block_busy_LF_PARA0   :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(INFO,"\t- block_busy_LF_PARA1   :  0x%x\n", READ_BIT(reg_val,19,19));
    VLOG(INFO,"\t- block_busy_LF_DBK     :  0x%x\n", READ_BIT(reg_val,18,18));
    VLOG(INFO,"\t- block_busy_LF_SAOCDEF :  0x%x\n", READ_BIT(reg_val,17,17));
    VLOG(INFO,"\t- block_busy_LF_SR_LR   :  0x%x\n", READ_BIT(reg_val,16,16));
    VLOG(INFO,"\t- block_busy_LF_FBC     :  0x%x\n", READ_BIT(reg_val,15,15));
    VLOG(INFO,"\t- block_busy_LF_LOADER  :  0x%x\n", READ_BIT(reg_val,14,14));
    VLOG(INFO,"\t- block_busy_ENT        :  0x%x\n", READ_BIT(reg_val,13,13));
    VLOG(INFO,"\t- block_busy_DEC_TQ     :  0x%x\n", READ_BIT(reg_val,12,12));
    VLOG(INFO,"\t- block_busy_DEC_IP     :  0x%x\n", READ_BIT(reg_val,11,11));
    VLOG(INFO,"\t- block_busy_DEC_MC_REF :  0x%x\n", READ_BIT(reg_val,10,10));
    VLOG(INFO,"\t- block_busy_DEC_MC_INTP:  0x%x\n", READ_BIT(reg_val, 9, 9));
    VLOG(INFO,"\t- block_busy_DEC_REC    :  0x%x\n", READ_BIT(reg_val, 8, 8));
    VLOG(INFO,"\t- warning_flag          :  0x%x\n", READ_BIT(reg_val, 7, 7));
    VLOG(INFO,"\t- error_flag            :  0x%x\n", READ_BIT(reg_val, 6, 6));
    VLOG(INFO,"\t- pic_busy              :  0x%x\n", READ_BIT(reg_val, 5, 5));
    VLOG(INFO,"\t- tile_busy             :  0x%x\n", READ_BIT(reg_val, 4, 4));
    VLOG(INFO,"\t- reserved              :  0x%x\n", READ_BIT(reg_val, 3, 3));
    VLOG(INFO,"\t- slice_done            :  0x%x\n", READ_BIT(reg_val, 2, 2));
    VLOG(INFO,"\t- tile_done             :  0x%x\n", READ_BIT(reg_val, 1, 1));
    VLOG(INFO,"\t- pic_done              :  0x%x\n", READ_BIT(reg_val, 0, 0));

    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_CTL_ERR_ERROR + reg_offset);
    VLOG(INFO,"- VCE_CTL_ERR_ERROR   = %08x\n", reg_val);
    VLOG(INFO,"\t- block_error_SRC        :  0x%x\n", READ_BIT(reg_val,31,31));
    VLOG(INFO,"\t- block_error_CME        :  0x%x\n", READ_BIT(reg_val,30,30));
    VLOG(INFO,"\t- block_error_RHU        :  0x%x\n", READ_BIT(reg_val,29,29));
    VLOG(INFO,"\t- block_error_RME        :  0x%x\n", READ_BIT(reg_val,28,28));
    VLOG(INFO,"\t- block_error_FME        :  0x%x\n", READ_BIT(reg_val,27,27));
    VLOG(INFO,"\t- block_error_IMD        :  0x%x\n", READ_BIT(reg_val,26,26));
    VLOG(INFO,"\t- block_error_RDO        :  0x%x\n", READ_BIT(reg_val,25,25));
    VLOG(INFO,"\t- block_error_REC        :  0x%x\n", READ_BIT(reg_val,24,24));
    VLOG(INFO,"\t- block_error_LF_PARA0   :  0x%x\n", READ_BIT(reg_val,23,23));
    VLOG(INFO,"\t- block_error_LF_PARA1   :  0x%x\n", READ_BIT(reg_val,22,22));
    VLOG(INFO,"\t- block_error_LF_DBK     :  0x%x\n", READ_BIT(reg_val,21,21));
    VLOG(INFO,"\t- block_error_LF_SAOCDEF :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(INFO,"\t- block_error_LF_SR_LR   :  0x%x\n", READ_BIT(reg_val,19,19));
    VLOG(INFO,"\t- block_error_LF_FBC     :  0x%x\n", READ_BIT(reg_val,18,18));
    VLOG(INFO,"\t- block_error_LF_LOADER  :  0x%x\n", READ_BIT(reg_val,17,17));
    VLOG(INFO,"\t- block_error_ENT        :  0x%x\n", READ_BIT(reg_val,16,16));
    VLOG(INFO,"\t- block_error_DEC_TQ     :  0x%x\n", READ_BIT(reg_val,15,15));
    VLOG(INFO,"\t- block_error_DEC_IP     :  0x%x\n", READ_BIT(reg_val,14,14));
    VLOG(INFO,"\t- block_error_DEC_MC_REF :  0x%x\n", READ_BIT(reg_val,13,13));
    VLOG(INFO,"\t- block_error_DEC_MC_INTP:  0x%x\n", READ_BIT(reg_val,12,12));
    VLOG(INFO,"\t- block_error_DEC_REC    :  0x%x\n", READ_BIT(reg_val,11,11));
    VLOG(INFO,"\t- reserved               :  0x%x\n", READ_BIT(reg_val,10, 0));

    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_CTL_ERR_WARNING + reg_offset);
    VLOG(INFO,"- VCE_CTL_ERR_WARNING = %08x\n", reg_val);
    VLOG(INFO,"\t- block_warning_SRC        :  0x%x\n", READ_BIT(reg_val,31,31));
    VLOG(INFO,"\t- block_warning_CME        :  0x%x\n", READ_BIT(reg_val,30,30));
    VLOG(INFO,"\t- block_warning_RHU        :  0x%x\n", READ_BIT(reg_val,29,29));
    VLOG(INFO,"\t- block_warning_RME        :  0x%x\n", READ_BIT(reg_val,28,28));
    VLOG(INFO,"\t- block_warning_FME        :  0x%x\n", READ_BIT(reg_val,27,27));
    VLOG(INFO,"\t- block_warning_IMD        :  0x%x\n", READ_BIT(reg_val,26,26));
    VLOG(INFO,"\t- block_warning_RDO        :  0x%x\n", READ_BIT(reg_val,25,25));
    VLOG(INFO,"\t- block_warning_REC        :  0x%x\n", READ_BIT(reg_val,24,24));
    VLOG(INFO,"\t- block_warning_LF_PARA0   :  0x%x\n", READ_BIT(reg_val,23,23));
    VLOG(INFO,"\t- block_warning_LF_PARA1   :  0x%x\n", READ_BIT(reg_val,22,22));
    VLOG(INFO,"\t- block_warning_LF_DBK     :  0x%x\n", READ_BIT(reg_val,21,21));
    VLOG(INFO,"\t- block_warning_LF_SAOCDEF :  0x%x\n", READ_BIT(reg_val,20,20));
    VLOG(INFO,"\t- block_warning_LF_SR_LR   :  0x%x\n", READ_BIT(reg_val,19,19));
    VLOG(INFO,"\t- block_warning_LF_FBC     :  0x%x\n", READ_BIT(reg_val,18,18));
    VLOG(INFO,"\t- block_warning_LF_LOADER  :  0x%x\n", READ_BIT(reg_val,17,17));
    VLOG(INFO,"\t- block_warning_ENT        :  0x%x\n", READ_BIT(reg_val,16,16));
    VLOG(INFO,"\t- block_warning_DEC_TQ     :  0x%x\n", READ_BIT(reg_val,15,15));
    VLOG(INFO,"\t- block_warning_DEC_IP     :  0x%x\n", READ_BIT(reg_val,14,14));
    VLOG(INFO,"\t- block_warning_DEC_MC_REF :  0x%x\n", READ_BIT(reg_val,13,13));
    VLOG(INFO,"\t- block_warning_DEC_MC_INTP:  0x%x\n", READ_BIT(reg_val,12,12));
    VLOG(INFO,"\t- block_warning_DEC_REC    :  0x%x\n", READ_BIT(reg_val,11,11));
    VLOG(INFO,"\t- reserved                 :  0x%x\n", READ_BIT(reg_val,10, 0));


    if(codec_type == DEBUG_CODEC_ENC) {
        VLOG(INFO, "- W6_RDO_DEBUG_TOP %08x\n", W6_RDO_DEBUG_TOP + reg_offset);
        reg_val = vdi_fio_read_register(coreIdx, W6_RDO_DEBUG_TOP + reg_offset);    VLOG(INFO,"- W6_RDO_DEBUG_TOP = %08x\n", reg_val);
        VLOG(INFO,"\t- w_debug_info_coe_buf[3:0]    :  0x%x\n", READ_BIT(reg_val,27,24));
        VLOG(INFO,"\t- 1'b0                         :  0x%x\n", READ_BIT(reg_val,23,23));
        VLOG(INFO,"\t- w_debug_info_ar[2:0]         :  0x%x\n", READ_BIT(reg_val,22,20));
        VLOG(INFO,"\t- o_rdo_sub_ctu_param_ready    :  0x%x\n", READ_BIT(reg_val,19,19));
        VLOG(INFO,"\t- i_lf_sub_ctu_param_ready     :  0x%x\n", READ_BIT(reg_val,18,18));
        VLOG(INFO,"\t- i_dbk_param_wr_block_ready   :  0x%x\n", READ_BIT(reg_val,17,17));
        VLOG(INFO,"\t- o_imd_block_ready            :  0x%x\n", READ_BIT(reg_val,16,16));
        VLOG(INFO,"\t- i_imd_nb_up_ready            :  0x%x\n", READ_BIT(reg_val,15,15));
        VLOG(INFO,"\t- o_fme_block_ready            :  0x%x\n", READ_BIT(reg_val,14,14));
        VLOG(INFO,"\t- i_rhu_ref_c_rd_block_ready_0 :  0x%x\n", READ_BIT(reg_val,13,13));
        VLOG(INFO,"\t- i_rhu_ref_c_rd_block_ready_1 :  0x%x\n", READ_BIT(reg_val,12,12));
        VLOG(INFO,"\t- i_rdo_ent_mode_wr_block_ready:  0x%x\n", READ_BIT(reg_val,11,11));
        VLOG(INFO,"\t- i_rdo_ent_tc_wr_block_ready  :  0x%x\n", READ_BIT(reg_val,10,10));
        VLOG(INFO,"\t- i_rdo_s2me_mv_wr_block_ready :  0x%x\n", READ_BIT(reg_val, 9, 9));
        VLOG(INFO,"\t- o_rec_rd_block_ready         :  0x%x\n", READ_BIT(reg_val, 8, 8));
        VLOG(INFO,"\t- rdo_rec_wr_block_ready       :  0x%x\n", READ_BIT(reg_val, 7, 7));
        VLOG(INFO,"\t- rdo_src_rd_block_ready0      :  0x%x\n", READ_BIT(reg_val, 6, 6));
        VLOG(INFO,"\t- rdo_src_rd_block_ready1      :  0x%x\n", READ_BIT(reg_val, 5, 5));
        VLOG(INFO,"\t- rdo_ref_y_rd_block_ready0    :  0x%x\n", READ_BIT(reg_val, 4, 4));
        VLOG(INFO,"\t- rdo_ref_y_rd_block_ready1    :  0x%x\n", READ_BIT(reg_val, 3, 3));
        VLOG(INFO,"\t- w_sub_ctu_coe_wr_ready       :  0x%x\n", READ_BIT(reg_val, 2, 2));
        VLOG(INFO,"\t- w_ar_rdma_ready              :  0x%x\n", READ_BIT(reg_val, 1, 1));
        VLOG(INFO,"\t- w_sub_ctu_lf_para_ready      :  0x%x\n", READ_BIT(reg_val, 0, 0));
        VLOG(INFO, "- W6_RDO_DEBUG_CTRL %08x\n", W6_RDO_DEBUG_CTRL+ reg_offset);
        reg_val = vdi_fio_read_register(coreIdx, W6_RDO_DEBUG_CTRL+ reg_offset);    VLOG(INFO,"- W6_RDO_DEBUG_CTRL= %08x\n", reg_val);
        VLOG(INFO,"\t- 4'b0                        :  0x%x\n", READ_BIT(reg_val,31,28));
        VLOG(INFO,"\t- 1'b0, fsm_imd_cur[2:0]      :  0x%x\n", READ_BIT(reg_val,27,24));
        VLOG(INFO,"\t- 1'b0, fsm_cur[2:0]          :  0x%x\n", READ_BIT(reg_val,23,20));
        VLOG(INFO,"\t- imd_avail_ready             :  0x%x\n", READ_BIT(reg_val,19,19));
        VLOG(INFO,"\t- r_imd_done                  :  0x%x\n", READ_BIT(reg_val,18,18));
        VLOG(INFO,"\t- pre_avail_ready             :  0x%x\n", READ_BIT(reg_val,17,17));
        VLOG(INFO,"\t- w_rdo_slc_avail_empty       :  0x%x\n", READ_BIT(reg_val,16,16));
        VLOG(INFO,"\t- o_slc_avail_full            :  0x%x\n", READ_BIT(reg_val,15,15));
        VLOG(INFO,"\t- i_slc_avail_full            :  0x%x\n", READ_BIT(reg_val,14,14));
        VLOG(INFO,"\t- w_fme_center_l0_fifo_empty  :  0x%x\n", READ_BIT(reg_val,13,13));
        VLOG(INFO,"\t- w_fme_center_l1_fifo_empty  :  0x%x\n", READ_BIT(reg_val,12,12));
        VLOG(INFO,"\t- w_sub_ctu_param_fifo_empty  :  0x%x\n", READ_BIT(reg_val,11,11));
        VLOG(INFO,"\t- rec_buf_full                :  0x%x\n", READ_BIT(reg_val,10,10));
        VLOG(INFO,"\t- rec_buf_ready               :  0x%x\n", READ_BIT(reg_val, 9, 9));
        VLOG(INFO,"\t- r_fme_done                  :  0x%x\n", READ_BIT(reg_val, 8, 8));
        VLOG(INFO,"\t- r_qp_done                   :  0x%x\n", READ_BIT(reg_val, 7, 7));
        VLOG(INFO,"\t- r_done                      :  0x%x\n", READ_BIT(reg_val, 6, 6));
        VLOG(INFO,"\t- r_md_done                   :  0x%x\n", READ_BIT(reg_val, 5, 5));
        VLOG(INFO,"\t- r_lsc_done                  :  0x%x\n", READ_BIT(reg_val, 4, 4));
        VLOG(INFO,"\t- r_load_done                 :  0x%x\n", READ_BIT(reg_val, 3, 3));
        VLOG(INFO,"\t- r_pre_pipeline              :  0x%x\n", READ_BIT(reg_val, 2, 2));
        VLOG(INFO,"\t- r_rdo_pipeline              :  0x%x\n", READ_BIT(reg_val, 1, 1));
        VLOG(INFO,"\t- r_imd_pipeline              :  0x%x\n", READ_BIT(reg_val, 0, 0));
        reg_val = vdi_fio_read_register(coreIdx, W6_RDO_DEBUG_POS + reg_offset);    VLOG(INFO,"- W6_RDO_DEBUG_POS = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_RDO_DEBUG_CTU + reg_offset);    VLOG(INFO,"- W6_RDO_DEBUG_CTU = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_RDO_DEBUG_CU  + reg_offset);    VLOG(INFO,"- W6_RDO_DEBUG_CU  = %08x\n", reg_val);


        reg_val = vdi_fio_read_register(coreIdx, W6_CME_STATUS_0+ reg_offset);    VLOG(INFO,"- W6_CME_STATUS_0 = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_CME_STATUS_1+ reg_offset);    VLOG(INFO,"- W6_CME_STATUS_1 = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_CME_DEBUG_0    + reg_offset);    VLOG(INFO,"- W6_CME_DEBUG_0  = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_CME_DEBUG_1    + reg_offset);    VLOG(INFO,"- W6_CME_DEBUG_1  = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_CME_DEBUG_2    + reg_offset);    VLOG(INFO,"- W6_CME_DEBUG_2  = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_CME_DEBUG_3    + reg_offset);    VLOG(INFO,"- W6_CME_DEBUG_3  = %08x\n", reg_val);

        reg_val = vdi_fio_read_register(coreIdx, W6_ENT_DEBUG_INFO_0 + reg_offset);
        VLOG(INFO,"- ENT_ENC_DEBUG_INFO_0 = %08x\n", reg_val);
        VLOG(INFO,"\t- lf_wiener_rd_empty   :  0x%x\n", READ_BIT(reg_val,24,24));
        VLOG(INFO,"\t- lf_cdef_idx_rd_empty :  0x%x\n", READ_BIT(reg_val,23,23));
        VLOG(INFO,"\t- ent_tc_rd_ready      :  0x%x\n", READ_BIT(reg_val,22,22));
        VLOG(INFO,"\t- ent_mode_rd_ready    :  0x%x\n", READ_BIT(reg_val,21,21));
        VLOG(INFO,"\t- slice_done           :  0x%x\n", READ_BIT(reg_val,20,20));
        VLOG(INFO,"\t- tile_done            :  0x%x\n", READ_BIT(reg_val,19,19));
        VLOG(INFO,"\t- pic_done             :  0x%x\n", READ_BIT(reg_val,18,18));
        VLOG(INFO,"\t- sctu_pos_y/mb_pos_y  :  0x%x(AVC:*16, HEVC,AV1:*32)\n", READ_BIT(reg_val,17, 9));
        VLOG(INFO,"\t- sctu_pos_x/mb_pos_x  :  0x%x(AVC:*16, HEVC,AV1:*32)\n", READ_BIT(reg_val, 8, 0));

        reg_val = vdi_fio_read_register(coreIdx, W6_ENT_DEBUG_INFO_1 + reg_offset);
        VLOG(INFO,"- ENT_ENC_DEBUG_INFO_1 = %08x\n", reg_val);
        VLOG(INFO,"\t- pbu_full             :  0x%x\n", READ_BIT(reg_val, 2, 2));
        VLOG(INFO,"\t- nal_mem_full         :  0x%x\n", READ_BIT(reg_val, 1, 1));
        VLOG(INFO,"\t- ent_busy             :  0x%x\n", READ_BIT(reg_val, 0, 0));
    }

    if(codec_type == DEBUG_CODEC_DEC) {
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_DEC_TQ_DEBUG0 + reg_offset);
        VLOG(INFO,"- VCE_DEC_TQ_DEBUG = %08x\n", reg_val);
        VLOG(INFO,"\t- fsm_pipe_curr            :  0x%x\n", READ_BIT(reg_val,30,29));
        VLOG(INFO,"\t- fsm_qm_curr              :  0x%x\n", READ_BIT(reg_val,28,26));
        VLOG(INFO,"\t- fsm_stage0_curr          :  0x%x\n", READ_BIT(reg_val,25,23));
        VLOG(INFO,"\t- fsm_stage1_curr          :  0x%x\n", READ_BIT(reg_val,22,20));
        VLOG(INFO,"\t- fsm_stage2_curr          :  0x%x\n", READ_BIT(reg_val,19,17));
        VLOG(INFO,"\t- r_tb_run_cnt_in_b64      :  0x%x\n", READ_BIT(reg_val,16, 8));
        VLOG(INFO,"\t- r_pipe_run_index         :  0x%x\n", READ_BIT(reg_val, 7, 0));
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_DEC_TQ_DEBUG1 + reg_offset);
        VLOG(INFO,"\t- tq_ctu_xpos              :  0x%x\n", READ_BIT(reg_val,19,10));
        VLOG(INFO,"\t- tq_ctu_ypos              :  0x%x\n", READ_BIT(reg_val, 9, 0));

        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_DEC_IP_DEBUG + reg_offset);
        VLOG(INFO,"- VCE_DEC_IP_DEBUG = %08x\n", reg_val);
        VLOG(INFO,"\t- o_ent_cu_wr_ready        :  0x%x\n", READ_BIT(reg_val,21,21));
        VLOG(INFO,"\t- o_ip_cu_ready            :  0x%x\n", READ_BIT(reg_val,20,20));
        VLOG(INFO,"\t- w_wdma_idle              :  0x%x\n", READ_BIT(reg_val,19,19));
        VLOG(INFO,"\t- w_rdma_idle              :  0x%x\n", READ_BIT(reg_val,18,18));
        VLOG(INFO,"\t- w_ctu_xpos               :  0x%x\n", READ_BIT(reg_val,17, 9));
        VLOG(INFO,"\t- w_ctu_ypos               :  0x%x\n", READ_BIT(reg_val, 8, 0));
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_DEC_CORE_DEBUG + reg_offset);
        VLOG(INFO,"\t- w_1st ctu_xpos           :  0x%x\n", READ_BIT(reg_val,31,16));
        VLOG(INFO,"\t- w_1st ctu_ypos           :  0x%x\n", READ_BIT(reg_val,15, 0));

        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_DEC_REC_DEBUG0 + reg_offset);
        VLOG(INFO,"- VCE_DEC_REC_DEBUG = %08x\n", reg_val);
        VLOG(INFO,"\t- main fsm                 :  0x%x\n", READ_BIT(reg_val,31,28));
        VLOG(INFO,"\t- resi rd ready            :  0x%x\n", READ_BIT(reg_val,27,27));
        VLOG(INFO,"\t- rec wr ready             :  0x%x\n", READ_BIT(reg_val,26,26));
        VLOG(INFO,"\t- resi bufr state          :  0x%x\n", READ_BIT(reg_val,24,23));
        VLOG(INFO,"\t- resi ch state            :  0x%x\n", READ_BIT(reg_val,22,20));
        VLOG(INFO,"\t- resi buf cnt             :  0x%x\n", READ_BIT(reg_val,19,18));
        VLOG(INFO,"\t- resi buf cucnt           :  0x%x\n", READ_BIT(reg_val,17, 8));
        VLOG(INFO,"\t- mc buf y state           :  0x%x\n", READ_BIT(reg_val, 7, 4));
        VLOG(INFO,"\t- mc buf c state           :  0x%x\n", READ_BIT(reg_val, 3, 0));
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_DEC_REC_DEBUG1 + reg_offset);
        VLOG(INFO,"\t- rec_ctu_xpos             :  0x%x\n", READ_BIT(reg_val,19,10));
        VLOG(INFO,"\t- rec_ctu_ypos             :  0x%x\n", READ_BIT(reg_val, 9, 0));

        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_DEC_MC_REF_DEBUG + reg_offset);
        VLOG(INFO,"- VCE_DEC_MC_REF_DEBUG = %08x\n", reg_val);
        VLOG(INFO,"\t- cu_mem_ref_rd_ready      :  0x%x\n", READ_BIT(reg_val,31,31));
        VLOG(INFO,"\t- cu_mem_intp_wr_ready     :  0x%x\n", READ_BIT(reg_val,30,30));
        VLOG(INFO,"\t- cu_run_ready             :  0x%x\n", READ_BIT(reg_val,29,29));
        VLOG(INFO,"\t- done_dma                 :  0x%x\n", READ_BIT(reg_val,28,28));
        VLOG(INFO,"\t- cur_cu_state             :  0x%x\n", READ_BIT(reg_val,25,24));
        VLOG(INFO,"\t- cur_warp_mem_state       :  0x%x\n", READ_BIT(reg_val,21,20));
        VLOG(INFO,"\t- cur_cu_mem_state         :  0x%x\n", READ_BIT(reg_val,18,16));
        VLOG(INFO,"\t- cur_state                :  0x%x\n", READ_BIT(reg_val, 5, 3));
        VLOG(INFO,"\t- req_info_full            :  0x%x\n", READ_BIT(reg_val, 2, 2));
        VLOG(INFO,"\t- gdi_req_fulll            :  0x%x\n", READ_BIT(reg_val, 1, 1));
        VLOG(INFO,"\t- fbc_data_avail           :  0x%x\n", READ_BIT(reg_val, 0, 0));

        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_DEC_MC_INTP_DEBUG + reg_offset);
        VLOG(INFO,"- VCE_DEC_MC_INTP_DEBUG = %08x\n", reg_val);
        VLOG(INFO,"\t- cu_mem_intp_rd_ready     :  0x%x\n", READ_BIT(reg_val,25,25));
        VLOG(INFO,"\t- cu_run_ready             :  0x%x\n", READ_BIT(reg_val,24,24));
        VLOG(INFO,"\t- cur_cu_state             :  0x%x\n", READ_BIT(reg_val,21,20));
        VLOG(INFO,"\t- cur_cu_mem_state         :  0x%x\n", READ_BIT(reg_val,18,16));
        VLOG(INFO,"\t- cur_state                :  0x%x\n", READ_BIT(reg_val, 7, 4));
        VLOG(INFO,"\t- pmbuf_empty              :  0x%x\n", READ_BIT(reg_val, 3, 3));
        VLOG(INFO,"\t- sync_buf_ready           :  0x%x\n", READ_BIT(reg_val, 2, 2));
        VLOG(INFO,"\t- sync_info_ready          :  0x%x\n", READ_BIT(reg_val, 1, 1));
        VLOG(INFO,"\t- req_info_empty           :  0x%x\n", READ_BIT(reg_val, 0, 0));

        //ENT_BSU
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_BSU_STA_ADDR + reg_offset);
        VLOG(INFO,"- VCE_ENT_BSU_CPB_STA_ADDR = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_BSU_END_ADDR + reg_offset);
        VLOG(INFO,"- VCE_ENT_BSU_CPB_END_ADDR = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_BSU_CPB_RPTR + reg_offset);
        VLOG(INFO,"- VCE_ENT_BSU_CPB_RPTR     = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_BSU_CPB_WPTR + reg_offset);
        VLOG(INFO,"- VCE_ENT_BSU_CPB_WPTR     = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_BSU_RDMA     + reg_offset);
        VLOG(INFO,"- VCE_ENT_BSU_RDMA raxi    = %08x\n", READ_BIT(reg_val, 15,0));
        VLOG(INFO,"\t- raxi_cs                :  0x%x (0:RAXI_IDLE, 1,2:RAXI_INIT, 3:RAXI_READY(WAIT_BUF16), 4:WAIT_AXI_ARREADY), 5: ETC \n", READ_BIT(reg_val, 7, 4));
        VLOG(INFO,"\t- stop_done              :  0x%x\n", READ_BIT(reg_val, 3, 3));
        VLOG(INFO,"\t- rdata_busy             :  0x%x(0:All dma data_done , 1:DMA run) \n", READ_BIT(reg_val, 2, 2));
        VLOG(INFO,"\t- raddr_busy             :  0x%x(0:All dma req done  , 1:DMA run) \n", READ_BIT(reg_val, 1, 1));
        VLOG(INFO,"\t- raxi_busy              :  0x%x(0:All dma done      , 1:DAM run) \n", READ_BIT(reg_val, 0, 0));
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_BSU_ACCESS_STATUS + reg_offset);
        VLOG(INFO,"- VCE_ENT_BSU_ACCESS       = %08x\n", reg_val);
        VLOG(INFO,"\t- empty_space            :  0x%x\n", READ_BIT(reg_val,23, 16));
        VLOG(INFO,"\t- valid_space            :  0x%x\n", READ_BIT(reg_val,15, 8));
        VLOG(INFO,"\t- find_start_code_fsm    :  0x%x(0:SC_IDLE, 1:SC_RUN, 2:SC_FIND, 3:SC_EOB)\n", READ_BIT(reg_val, 5, 4));
        VLOG(INFO,"\t- valid_d1_d0            :  0x%x\n", READ_BIT(reg_val, 0, 0));

        //ENT_DEC
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_DEC_TILE_FIRST_CTU_POS + reg_offset);
        VLOG(INFO,"- VCE_ENT_DEC\n");
        VLOG(INFO,"\t- tile_start_pos_x       :  0x%x\n", READ_BIT(reg_val,31,16));
        VLOG(INFO,"\t- tile_start_pos_y       :  0x%x\n", READ_BIT(reg_val,15, 0));
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_DEC_TILE_LAST_CTU_POS + reg_offset);
        VLOG(INFO,"\t- tile_end_pos_x         :  0x%x\n", READ_BIT(reg_val,31,16));
        VLOG(INFO,"\t- tile_end_pos_y         :  0x%x\n", READ_BIT(reg_val,15, 0));
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_DEC_CURR_CTU_POS + reg_offset);
        VLOG(INFO,"\t- curr_ctu_pos_x         :  0x%x\n", READ_BIT(reg_val,31,16));
        VLOG(INFO,"\t- curr_ctu_pos_y         :  0x%x\n", READ_BIT(reg_val,15, 0));
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_DEC_ERR_INFO + reg_offset);
        VLOG(INFO,"- VCE_ENT_DEC_ERR_INFO     = %08x\n", reg_val);
        VLOG(INFO,"\t- decoded_CTU_counts     :  0x%x\n", READ_BIT(reg_val,31, 12));
        VLOG(INFO,"\t- error_reason           :  0x%x\n", READ_BIT(reg_val,11,  4));
        VLOG(INFO,"\t- mvd_qpd_warn (AVC)     :  0x%x\n", READ_BIT(reg_val, 1,  1));
        VLOG(INFO,"\t- decoding success       :  0x%x\n", READ_BIT(reg_val, 0,  0));

        if (READ_BIT(reg_val, 0, 0) == 0) { // err_asserted check
            VLOG(INFO,"\t- AVC error_reason\n");
            VLOG(INFO,"\t- 4'b0000 : mvd out of range \n");
            VLOG(INFO,"\t- 4'b0001 : mb_qp_delta out of range ( -33 <   mb_qp_delta < 33) \n");
            VLOG(INFO,"\t- 4'b0010 : coeff out of range \n");
            VLOG(INFO,"\t- 4'b0011 : mv out of range \n");
            VLOG(INFO,"\t- 4'b0100 : mb_skip_run out of range \n");
            VLOG(INFO,"\t- 4'b0101 : mb_type out of range \n");
            VLOG(INFO,"\t- 4'b0110 : sub_mb_type out of range \n");
            VLOG(INFO,"\t- 4'b0111 : coded_block_pattern out of range \n");
            VLOG(INFO,"\t- 4'b1000 : intra_chroma_pred_mode out of range \n");
            VLOG(INFO,"\t- 4'b1001 : ref_idx out of range \n");
            VLOG(INFO,"\t- 4'b1010 : coeff_token Error \n");
            VLOG(INFO,"\t- 4'b1011 : Total Zero Error \n");
            VLOG(INFO,"\t- 4'b1100 : run_before Error \n");
            VLOG(INFO,"\t- 4'b1101 : over_cs \n");
            VLOG(INFO,"\t- 4'b1110 : EOS Error \n");
            VLOG(INFO,"\t- 4'b1111 : reserved \n");
            VLOG(INFO,"\t- HEVC error_reason\n");
            VLOG(INFO,"\t- 4'b0000 : mvd out of range \n");
            VLOG(INFO,"\t- 4'b0001 : CuQpDeltaVal out of range \n");
            VLOG(INFO,"\t- 4'b0010 : coeff_level_remaining out of range \n");
            VLOG(INFO,"\t- 4'b0011 : pcm_err \n");
            VLOG(INFO,"\t- 4'b0100 : over_cs \n");
            VLOG(INFO,"\t- 4'b1000 : eos_err \n");
            VLOG(INFO,"\t- 4'b1100 : eos_err_one \n");
            VLOG(INFO,"\t- AV1 error_reason\n");
            VLOG(INFO,"\t- 5'b00001 : is_mv_valid error (need to '1') \n");
            VLOG(INFO,"\t- 5'b00010 : golomb_length_bit range error (need to value < 21) \n");
            VLOG(INFO,"\t- 5'b00100 : symbolMaxBits range error (need to value > -14) \n");
            VLOG(INFO,"\t- 5'b01000 : trailing padding bit pattern error (need to All zeros) \n");
            VLOG(INFO,"\t- 5'b10000 : trailing bit pattern error (need to 15'b100_0000_0000_0000) \n");
            VLOG(INFO,"\t- VP9 error_reason\n");
            VLOG(INFO,"\t- 2'b00 : TILE_MARKER \n");
            VLOG(INFO,"\t- 2'b01 : MV_RANGE_OVER \n");
            VLOG(INFO,"\t- 2'b10 : reserved \n");
            VLOG(INFO,"\t- 2'b11 : OVER_CONSUME \n");
        }

        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_DEC_DEBUG_INFO + reg_offset);
        VLOG(INFO,"- VCE_ENT_DEC_DEBUG_INFO   = %08x\n", reg_val);
        reg_val = vdi_fio_read_register(coreIdx, W6_VCE_ENT_DEC_DEBUG_TMPL_STATE + reg_offset);
        VLOG(INFO,"- VCE_ENT_DEC_DEBUG_TMPL_STATE   = %08x\n", reg_val);
        VLOG(INFO,"\t- save_cmd_empty         :  0x%x\n", READ_BIT(reg_val,14, 14));
        VLOG(INFO,"\t- load_cmd_empty         :  0x%x\n", READ_BIT(reg_val,13, 13));
        VLOG(INFO,"\t- mv_not_ready           :  0x%x\n", READ_BIT(reg_val,12, 12));
        VLOG(INFO,"\t- wait_init_cond_wr      :  0x%x\n", READ_BIT(reg_val,11, 11));
        VLOG(INFO,"\t- cur_sync               :  0x%x\n", READ_BIT(reg_val,10,  7));
        VLOG(INFO,"\t- r_tmpl_load_cs         :  0x%x\n", READ_BIT(reg_val, 6,  4));
        VLOG(INFO,"\t- r_tmpl_save_cs         :  0x%x\n", READ_BIT(reg_val, 2,  0));
    }

    // LF FBC IN LOADER
    VLOG(INFO,"- VCE_LF_FBC_IN_LOADER_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 0);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- BlockPosX                :  0x%x\n", READ_BIT(reg_val,31,16));
    VLOG(INFO,"\t- BlockPosY                :  0x%x\n", READ_BIT(reg_val,15, 0));
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 1);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- sync_state               :  0x%x\n", READ_BIT(reg_val,11, 8));
    VLOG(INFO,"\t- main_state               :  0x%x\n", READ_BIT(reg_val, 7, 5));
    VLOG(INFO,"\t- rec_buf_ready            :  0x%x\n", READ_BIT(reg_val, 4, 4));
    VLOG(INFO,"\t- dbk_out_buf_ready        :  0x%x\n", READ_BIT(reg_val, 3, 3));
    VLOG(INFO,"\t- sao_out_buf_ready        :  0x%x\n", READ_BIT(reg_val, 2, 2));
    VLOG(INFO,"\t- n_order_fifo_ready       :  0x%x\n", READ_BIT(reg_val, 1, 1));
    VLOG(INFO,"\t- out_fbc_ready            :  0x%x\n", READ_BIT(reg_val, 0, 0));

    // LF DBK
    VLOG(INFO,"- VCE_DBK_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 2);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- BlockPosX                :  0x%x\n", READ_BIT(reg_val,31,16));
    VLOG(INFO,"\t- BlockPosY                :  0x%x\n", READ_BIT(reg_val,15, 0));
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 3);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- SubBlkIdx                :  0x%x\n", READ_BIT(reg_val,12, 9));
    VLOG(INFO,"\t- sync_state               :  0x%x\n", READ_BIT(reg_val, 8, 6));
    VLOG(INFO,"\t- col_param_ready          :  0x%x\n", READ_BIT(reg_val, 5, 5));
    VLOG(INFO,"\t- dbk_param_buf_ready      :  0x%x\n", READ_BIT(reg_val, 4, 4));
    VLOG(INFO,"\t- rec_buf_ready            :  0x%x\n", READ_BIT(reg_val, 3, 3));
    VLOG(INFO,"\t- in_n_order_fifo_ready    :  0x%x\n", READ_BIT(reg_val, 2, 2));
    VLOG(INFO,"\t- dbk_out_buf_ready        :  0x%x\n", READ_BIT(reg_val, 1, 1));
    VLOG(INFO,"\t- out_n_order_fifo_ready   :  0x%x\n", READ_BIT(reg_val, 0, 0));

    // LF SAO CDEF
    VLOG(INFO,"- VCE_SAO_CDEF_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 4);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- BlockPosX                :  0x%x\n", READ_BIT(reg_val,31,16));
    VLOG(INFO,"\t- BlockPosY                :  0x%x\n", READ_BIT(reg_val,15, 0));
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 5);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- SubBlkIdx                :  0x%x\n", READ_BIT(reg_val,12, 9));
    VLOG(INFO,"\t- sync_state               :  0x%x\n", READ_BIT(reg_val, 8, 6));
    VLOG(INFO,"\t- slice_avail_fifo_empty   :  0x%x\n", READ_BIT(reg_val, 5, 5));
    VLOG(INFO,"\t- sao_cdef_param_ready     :  0x%x\n", READ_BIT(reg_val, 4, 4));
    VLOG(INFO,"\t- dbk_buf_ready            :  0x%x\n", READ_BIT(reg_val, 3, 3));
    VLOG(INFO,"\t- n_order_fifo_ready       :  0x%x\n", READ_BIT(reg_val, 2, 2));
    VLOG(INFO,"\t- cdef_out_buf_ready       :  0x%x\n", READ_BIT(reg_val, 1, 1));
    VLOG(INFO,"\t- reserved                 :  0x%x\n", READ_BIT(reg_val, 0, 0));

    // LF SR_LR
    VLOG(INFO,"- VCE_SR_LR_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 6);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- BlockPosX                :  0x%x\n", READ_BIT(reg_val,31,16));
    VLOG(INFO,"\t- BlockPosY                :  0x%x\n", READ_BIT(reg_val,15, 0));
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 7);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- sync_state               :  0x%x\n", READ_BIT(reg_val, 6, 3));
    VLOG(INFO,"\t- cdef_buf_ready           :  0x%x\n", READ_BIT(reg_val, 2, 2));
    VLOG(INFO,"\t- lr_param_fifo_empty      :  0x%x\n", READ_BIT(reg_val, 1, 1));
    VLOG(INFO,"\t- output_ready             :  0x%x\n", READ_BIT(reg_val, 0, 0));

    // LF FBC IN BUFIF
    VLOG(INFO,"- VCE_FBC_IN_BUFIF_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 8);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- cmd_empty                :  0x%x\n", READ_BIT(reg_val,18,18));
    VLOG(INFO,"\t- Y_X_0_fill               :  %01x%01x%01x%01x\n", READ_BIT(reg_val,17,17), READ_BIT(reg_val,16,16), READ_BIT(reg_val,15,15), READ_BIT(reg_val,14,14));
    VLOG(INFO,"\t- Y_X_1_fill               :  %01x%01x%01x%01x\n", READ_BIT(reg_val,13,13), READ_BIT(reg_val,12,12), READ_BIT(reg_val,11,11), READ_BIT(reg_val,10,10));
    VLOG(INFO,"\t- C_X_0_fill               :  %01x%01x%01x%01x\n", READ_BIT(reg_val, 9, 9), READ_BIT(reg_val, 8, 8), READ_BIT(reg_val, 7, 7), READ_BIT(reg_val, 6, 6));
    VLOG(INFO,"\t- C_X_1_fill               :  %01x%01x%01x%01x\n", READ_BIT(reg_val, 5, 5), READ_BIT(reg_val, 4, 4), READ_BIT(reg_val, 3, 3), READ_BIT(reg_val, 2, 2));
    VLOG(INFO,"\t- out_fbc_req              :  0x%x\n", READ_BIT(reg_val, 1, 1));
    VLOG(INFO,"\t- out_pp_req               :  0x%x\n", READ_BIT(reg_val, 0, 0));

    // LF PARGEN STEP0
    VLOG(INFO,"- VCE_PARAGEN_STEP0_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 9);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- BlockPosX                :  0x%x\n", READ_BIT(reg_val,31,16));
    VLOG(INFO,"\t- BlockPosY                :  0x%x\n", READ_BIT(reg_val,15, 0));
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset,10);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- sync_state               :  0x%x\n", READ_BIT(reg_val,16,15));
    VLOG(INFO,"\t- rec_buf_ready            :  0x%x\n", READ_BIT(reg_val,14,14));
    VLOG(INFO,"\t- src_buf_ready            :  0x%x\n", READ_BIT(reg_val,13,13));
    VLOG(INFO,"\t- param_fifo_ready         :  0x%x\n", READ_BIT(reg_val,12,12));
    VLOG(INFO,"\t- in_slice_avail_empty     :  0x%x\n", READ_BIT(reg_val,11,11));
    VLOG(INFO,"\t- out_slice_avail_full     :  0x%x\n", READ_BIT(reg_val,10,10));
    VLOG(INFO,"\t- pipe_on_2                :  0x%x\n", READ_BIT(reg_val, 9, 9));
    VLOG(INFO,"\t- pipe_on_1                :  0x%x\n", READ_BIT(reg_val, 8, 8));
    VLOG(INFO,"\t- pip3_on_0                :  0x%x\n", READ_BIT(reg_val, 7, 7));
    VLOG(INFO,"\t- pipe2_cur_fsm            :  0x%x\n", READ_BIT(reg_val, 6, 5));
    VLOG(INFO,"\t- pipe2_wh_fsm             :  0x%x\n", READ_BIT(reg_val, 4, 2));
    VLOG(INFO,"\t- pipe2_sao_cmp_fsm        :  0x%x\n", READ_BIT(reg_val, 1, 0));

    // LF PARAGEN STEP1
    VLOG(INFO,"- VCE_PARAGEN_STEP1_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 11);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- BlockPosX                :  0x%x\n", READ_BIT(reg_val,31,16));
    VLOG(INFO,"\t- BlockPosY                :  0x%x\n", READ_BIT(reg_val,15, 0));
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 12);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- sync_state               :  0x%x\n", READ_BIT(reg_val, 6, 5));
    VLOG(INFO,"\t- cdef_buf_ready           :  0x%x\n", READ_BIT(reg_val, 4, 4));
    VLOG(INFO,"\t- src_buf_ready            :  0x%x\n", READ_BIT(reg_val, 3, 3));
    VLOG(INFO,"\t- lambda_fifo_empty        :  0x%x\n", READ_BIT(reg_val, 2, 2));
    VLOG(INFO,"\t- out_ent_fifo_full        :  0x%x\n", READ_BIT(reg_val, 1, 1));
    VLOG(INFO,"\t- out_sr_lr_fifo_full      :  0x%x\n", READ_BIT(reg_val, 0, 0));

    // LF DBK TILE DMA
    VLOG(INFO,"- VCE_DBK_TILE_DMA_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 13);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- WREQ_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 14);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- WDAT_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 15);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- RREQ_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 16);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- RDAT_CNT                 :  0x%x\n", reg_val);

    // LF SR_LR TILE DMA
    VLOG(INFO,"- VCE_SR_LR_TILE_DMA_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 17);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- WREQ_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 18);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- WDAT_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 19);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- RREQ_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 20);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- RDAT_CNT                 :  0x%x\n", reg_val);

    // LF FBC_IN_BUFIF TILE DMA
    VLOG(INFO,"- VCE_FBC_IN_BUFIF_TILE_DMA_DEBUG\n");
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 21);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- WREQ_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 22);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- WDAT_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 23);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- RREQ_CNT                 :  0x%x\n", reg_val);
    vdi_fio_write_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset, 24);
    reg_val = vdi_fio_read_register(coreIdx, W6_VCE_LF_DEBUG + reg_offset);
    VLOG(INFO,"\t- RDAT_CNT                 :  0x%x\n", reg_val);


}

void wave5xx_vcpu_status(unsigned long coreIdx)
{
#ifdef MCTS_RC_TEST
    return;
#endif //MCTS_RC_TEST
    Uint32 vcpu_reg[31]= {0,};
    Uint32 i;
    Uint32 que_status;
    Uint32 stage_1;
    Uint32 stage_2;
    Uint32 stage_3;
    Uint32 instance_done;

    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                            Scheduler STATUS                        -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    {
        que_status = vdi_read_register(coreIdx,  0x1e0);
        VLOG(WARN, "COMMAND_QUE_CNT : %d,    REPORT_QUE_CNT : %d \n", que_status >> 16, que_status & 0xffff);
        stage_1 = vdi_read_register(coreIdx, 0x1f0);
        VLOG(WARN, "PARSING_INSTANCE     : 0x%08x \n", stage_1);
        stage_2 = vdi_read_register(coreIdx, 0x1f4);
        VLOG(WARN, "DECODING_INSTANCE     : 0x%08x \n", stage_2);
        stage_3 = vdi_read_register(coreIdx, 0x1f8);
        VLOG(WARN, "ENCODING_INSTANCE     : 0x%08x \n", stage_3);
        instance_done = vdi_read_register(coreIdx, 0x1fC);
        VLOG(WARN, "QUEUED_COMMAND_DONE : 0x%08x \n", instance_done);
    }

    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                            VCPU CORE STATUS                        -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
   // --------- VCPU register Dump
    VLOG(WARN,"[+] VCPU REG Dump\n");
    for (i = 0; i < 25; i++) {
        VpuWriteReg (coreIdx, 0x14, (1<<9) | (i & 0xff));
        vcpu_reg[i] = VpuReadReg (coreIdx, 0x1c);

        if (i < 16) {
            VLOG(WARN,"0x%08x\t",  vcpu_reg[i]);
            if ((i % 4) == 3) VLOG(INFO,"\n");
        }
        else {
            switch (i) {
            case 16: VLOG(WARN,"CR0: 0x%08x\t", vcpu_reg[i]); break;
            case 17: VLOG(WARN,"CR1: 0x%08x\n", vcpu_reg[i]); break;
            case 18: VLOG(WARN,"ML:  0x%08x\t", vcpu_reg[i]); break;
            case 19: VLOG(WARN,"MH:  0x%08x\n", vcpu_reg[i]); break;
            case 21: VLOG(WARN,"LR:  0x%08x\n", vcpu_reg[i]); break;
            case 22: VLOG(WARN,"PC:  0x%08x\n", vcpu_reg[i]); break;
            case 23: VLOG(WARN,"SR:  0x%08x\n", vcpu_reg[i]); break;
            case 24: VLOG(WARN,"SSP: 0x%08x\n", vcpu_reg[i]); break;
            default: break;
            }
        }
    }

    for ( i = 0 ; i < 20 ; i++) {
        VLOG(WARN, "LR=0x%x, PC=0x%x\n", vdi_read_register(coreIdx, W5_VCPU_CUR_LR), vdi_read_register(coreIdx, W5_VCPU_CUR_PC));
    }
    VLOG(WARN, "VCPU_BUSY_STATUS    : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x70) );
    VLOG(WARN, "VCPU_HALT_STATUS    : 0x%08x , [3:0] VPU bus transaction [4] halt status \n", vdi_read_register(coreIdx, 0x74) );
    VLOG(WARN,"[-] VCPU CORE REG Dump\n");
    VLOG(WARN, "TEMP VPU_SUB_FRM_SYNC_IF          : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x300) );
    VLOG(WARN, "TEMP VPU_SUB_FRM_SYNC             : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x304) );
    VLOG(WARN, "TEMP VPU_SUB_FRM_SYNC_IDX         : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x308) );
    VLOG(WARN, "TEMP VPU_SUB_FRM_SYNC_CLR         : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x30C) );
    VLOG(WARN, "TEMP VPU_SUB_FRM_SYNC_CNT         : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x310) );
    VLOG(WARN, "TEMP VPU_SUB_FRM_SYNC_ENC_FIDX    : 0x%08x , VPU command re-enterance check \n", vdi_read_register(coreIdx, 0x314) );

    VLOG(WARN,"[-] VCPU REG Dump\n");
    /// -- VCPU PERI DECODE Common
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                            VCPU PERI(SPP)                          -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    PrintWave5xxDecSppStatus(coreIdx);
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    VLOG(WARN,"------                            VCPU PERI(PRESCAN)                      -----\n");
    VLOG(WARN,"-------------------------------------------------------------------------------\n");
    PrintWave5xxDecPrescanStatus(coreIdx);
}

void vdi_print_vpu_status(unsigned long coreIdx)
{
    Uint32 productCode;

    productCode = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);

    if (PRODUCT_CODE_W6_SERIES(productCode)) {
        wave6xx_vcpu_status(coreIdx, DEBUG_CODEC_NONE);
        wave6xx_vcore_status(coreIdx, DEBUG_CODEC_NONE);
    } else if (PRODUCT_CODE_W5_SERIES(productCode)) {
        wave5xx_vcpu_status(coreIdx);
        wave5xx_vcore_status(coreIdx);
        VLOG(INFO,"-------------------------------------------------------------------------------\n");
    } else if (PRODUCT_CODE_CODA_SERIES(productCode)) {
    } else {
        VLOG(ERR, "Unknown product id : %08x\n", productCode);
    }
}

typedef struct {
    Uint32 active_cmd_pre_info;
    Uint32 active_cmd_vcore_core0_info;
    Uint32 active_cmd_vcore_core1_info;
    Uint32 cmd_que_info_core_id_0;
    Uint32 cmd_que_info_core_id_1;
    Uint32 cmd_que_info_inst_id_0;
    Uint32 cmd_que_info_inst_id_1;
    Uint32 cmd_que_info_inst_id_2;
    Uint32 cmd_que_info_inst_id_3;
    Uint32 cmd_que_info_inst_id_4;
    Uint32 cmd_que_info_inst_id_5;
    Uint32 cmd_que_info_inst_id_6;
    Uint32 cmd_que_info_inst_id_7;
    Uint32 cmd_que_info_cmd_idx_0;
    Uint32 cmd_que_info_cmd_idx_1;
    Uint32 cmd_que_info_cmd_idx_2;
    Uint32 cmd_que_info_cmd_idx_3;
    Uint32 cmd_que_info_cmd_idx_4;
    Uint32 cmd_que_info_cmd_idx_5;
    Uint32 cmd_que_info_cmd_idx_6;
    Uint32 cmd_que_info_cmd_idx_7;
    Uint32 cmd_que_info_cmd_idx_8;
    Uint32 cmd_que_info_cmd_idx_9;
    Uint32 cmd_que_info_cmd_idx_a;
    Uint32 cmd_que_info_cmd_idx_b;
    Uint32 cmd_que_info_cmd_idx_c;
    Uint32 cmd_que_info_cmd_idx_d;
    Uint32 cmd_que_info_cmd_idx_e;
    Uint32 cmd_que_info_cmd_idx_f;
    Uint32 cmd_que_pre_head_tail;
    Uint32 vcore0_cmd_que_info_core_id_0;
    Uint32 vcore0_cmd_que_info_core_id_1;
    Uint32 vcore0_cmd_que_info_inst_id_0;
    Uint32 vcore0_cmd_que_info_inst_id_1;
    Uint32 vcore0_cmd_que_info_inst_id_2;
    Uint32 vcore0_cmd_que_info_inst_id_3;
    Uint32 vcore0_cmd_que_info_inst_id_4;
    Uint32 vcore0_cmd_que_info_inst_id_5;
    Uint32 vcore0_cmd_que_info_inst_id_6;
    Uint32 vcore0_cmd_que_info_inst_id_7;
    Uint32 vcore0_cmd_que_info_cmd_idx_0;
    Uint32 vcore0_cmd_que_info_cmd_idx_1;
    Uint32 vcore0_cmd_que_info_cmd_idx_2;
    Uint32 vcore0_cmd_que_info_cmd_idx_3;
    Uint32 vcore0_cmd_que_info_cmd_idx_4;
    Uint32 vcore0_cmd_que_info_cmd_idx_5;
    Uint32 vcore0_cmd_que_info_cmd_idx_6;
    Uint32 vcore0_cmd_que_info_cmd_idx_7;
    Uint32 vcore0_cmd_que_info_cmd_idx_8;
    Uint32 vcore0_cmd_que_info_cmd_idx_9;
    Uint32 vcore0_cmd_que_info_cmd_idx_a;
    Uint32 vcore0_cmd_que_info_cmd_idx_b;
    Uint32 vcore0_cmd_que_info_cmd_idx_c;
    Uint32 vcore0_cmd_que_info_cmd_idx_d;
    Uint32 vcore0_cmd_que_info_cmd_idx_e;
    Uint32 vcore0_cmd_que_info_cmd_idx_f;
    Uint32 vcore0_cmd_que_head_tail;
    Uint32 vcore1_cmd_que_info_core_id_0;
    Uint32 vcore1_cmd_que_info_core_id_1;
    Uint32 vcore1_cmd_que_info_inst_id_0;
    Uint32 vcore1_cmd_que_info_inst_id_1;
    Uint32 vcore1_cmd_que_info_inst_id_2;
    Uint32 vcore1_cmd_que_info_inst_id_3;
    Uint32 vcore1_cmd_que_info_inst_id_4;
    Uint32 vcore1_cmd_que_info_inst_id_5;
    Uint32 vcore1_cmd_que_info_inst_id_6;
    Uint32 vcore1_cmd_que_info_inst_id_7;
    Uint32 vcore1_cmd_que_info_cmd_idx_0;
    Uint32 vcore1_cmd_que_info_cmd_idx_1;
    Uint32 vcore1_cmd_que_info_cmd_idx_2;
    Uint32 vcore1_cmd_que_info_cmd_idx_3;
    Uint32 vcore1_cmd_que_info_cmd_idx_4;
    Uint32 vcore1_cmd_que_info_cmd_idx_5;
    Uint32 vcore1_cmd_que_info_cmd_idx_6;
    Uint32 vcore1_cmd_que_info_cmd_idx_7;
    Uint32 vcore1_cmd_que_info_cmd_idx_8;
    Uint32 vcore1_cmd_que_info_cmd_idx_9;
    Uint32 vcore1_cmd_que_info_cmd_idx_a;
    Uint32 vcore1_cmd_que_info_cmd_idx_b;
    Uint32 vcore1_cmd_que_info_cmd_idx_c;
    Uint32 vcore1_cmd_que_info_cmd_idx_d;
    Uint32 vcore1_cmd_que_info_cmd_idx_e;
    Uint32 vcore1_cmd_que_info_cmd_idx_f;
    Uint32 vcore1_cmd_que_head_tail;
    Uint32 reserved;
    Uint32 prev_run_vcore_core_id;
    Uint32 prev_run_vcore_cmd_idx_inst_0123;
    Uint32 prev_run_vcore_cmd_idx_inst_4567;
    Uint32 prev_run_vcore_cmd_idx_inst_89ab;
    Uint32 prev_run_vcore_cmd_idx_inst_cdef;
    Uint32 inst_0_cmd_que_cmd_idx;
    Uint32 inst_1_cmd_que_cmd_idx;
    Uint32 inst_2_cmd_que_cmd_idx;
    Uint32 inst_3_cmd_que_cmd_idx;
    Uint32 inst_4_cmd_que_cmd_idx;
    Uint32 inst_5_cmd_que_cmd_idx;
    Uint32 inst_6_cmd_que_cmd_idx;
    Uint32 inst_7_cmd_que_cmd_idx;
    Uint32 inst_8_cmd_que_cmd_idx;
    Uint32 inst_9_cmd_que_cmd_idx;
    Uint32 inst_a_cmd_que_cmd_idx;
    Uint32 inst_b_cmd_que_cmd_idx;
    Uint32 inst_c_cmd_que_cmd_idx;
    Uint32 inst_d_cmd_que_cmd_idx;
    Uint32 inst_e_cmd_que_cmd_idx;
    Uint32 inst_f_cmd_que_cmd_idx;
    Uint32 inst_0_7_cmd_que_core_id;
    Uint32 inst_8_f_cmd_que_core_id;
    Uint32 inst_0_3_cmd_que_cnt_tail;
    Uint32 inst_4_7_cmd_que_cnt_tail;
    Uint32 inst_9_b_cmd_que_cnt_tail;
    Uint32 inst_c_f_cmd_que_cnt_tail;
    Uint32 inst_cmd_que_next_alloc_core_id;
    Uint32 inst_0_3_cmd_que_next_rel_cmd_idx;
    Uint32 inst_4_7_cmd_que_next_rel_cmd_idx;
    Uint32 inst_8_b_cmd_que_next_rel_cmd_idx;
    Uint32 inst_c_f_cmd_que_next_rel_cmd_idx;
    Uint32 inst_0_wait_cmd_que_cmd_idx;
    Uint32 inst_1_wait_cmd_que_cmd_idx;
    Uint32 inst_2_wait_cmd_que_cmd_idx;
    Uint32 inst_3_wait_cmd_que_cmd_idx;
    Uint32 inst_4_wait_cmd_que_cmd_idx;
    Uint32 inst_5_wait_cmd_que_cmd_idx;
    Uint32 inst_6_wait_cmd_que_cmd_idx;
    Uint32 inst_7_wait_cmd_que_cmd_idx;
    Uint32 inst_8_wait_cmd_que_cmd_idx;
    Uint32 inst_9_wait_cmd_que_cmd_idx;
    Uint32 inst_a_wait_cmd_que_cmd_idx;
    Uint32 inst_b_wait_cmd_que_cmd_idx;
    Uint32 inst_c_wait_cmd_que_cmd_idx;
    Uint32 inst_d_wait_cmd_que_cmd_idx;
    Uint32 inst_e_wait_cmd_que_cmd_idx;
    Uint32 inst_f_wait_cmd_que_cmd_idx;
    Uint32 inst_0_7_wait_cmd_que_core_id;
    Uint32 inst_8_f_wait_cmd_que_core_id;
    Uint32 inst_0_3_report_queue_cnt_tail;
    Uint32 inst_4_7_report_queue_cnt_tail;
    Uint32 inst_8_b_report_queue_cnt_tail;
    Uint32 inst_c_f_report_queue_cnt_tail;
} FW_STATUS;

#define READ_BIT(val,high,low) ((((high)==31) && ((low) == 0)) ?  (val) : (((val)>>(low)) & (((1<< ((high)-(low)+1))-1))))
void ChekcAndPrintDebugInfo(VpuHandle handle, BOOL bEncoder, RetCode result)
{
    RetCode cmd_ret;
    int productId;
    VpuDebugInfo *debugInfo;
    VLOG(INFO, "+%s core=%d, inst=%d, bEncoder=%d, result=%d\n", __FUNCTION__, handle->coreIdx, handle->instIndex, bEncoder, result);

    debugInfo = vmalloc(sizeof(VpuDebugInfo));
    if (debugInfo == NULL)
        return;

    productId = VPU_GetProductId(handle->coreIdx);

    if (PRODUCT_ID_W6_SERIES(productId)) {
        FW_STATUS fw_status;
        Uint32 *reg_base;
        Uint32 i, j, k;

        if (bEncoder == TRUE)
            cmd_ret = VPU_EncGiveCommand(handle, GET_DEBUG_INFORM, debugInfo);
        else
            cmd_ret = VPU_DecGiveCommand(handle, GET_DEBUG_INFORM, debugInfo);
        if (cmd_ret != RETCODE_SUCCESS) {
            VLOG(ERR, "-%s GET_DEBUG_INFOM command fail \n", __FUNCTION__);
            vfree(debugInfo);
            return;
        }

        osal_memcpy(&fw_status, debugInfo->regs, sizeof(FW_STATUS));

        VLOG(INFO, "-------------------------------------------------------------------------------\n");
        VLOG(INFO, "------                                    FW STATUS                        ----\n");
        VLOG(INFO, "-------------------------------------------------------------------------------\n");

        VLOG(INFO, "[+] ACTIVE_CMD_INFO\n");

        i = 0;

        VLOG(INFO, "\t- STAGE_PRE      : inst_id %3d core_id %3d cmd_idx %3d\n", READ_BIT(fw_status.active_cmd_pre_info, 19, 16),
            READ_BIT(fw_status.active_cmd_pre_info, 20, 20),
            READ_BIT(fw_status.active_cmd_pre_info, 15, 0));


        VLOG(INFO, "\t- STAGE_VCORE[0] : inst_id %3d core_id %3d cmd_idx %3d\n", READ_BIT(fw_status.active_cmd_vcore_core0_info, 19, 16),
            READ_BIT(fw_status.active_cmd_vcore_core0_info, 20, 20),
            READ_BIT(fw_status.active_cmd_vcore_core0_info, 15, 0));

        VLOG(INFO, "\t- STAGE_VCORE[1] : inst_id %3d core_id %3d cmd_idx %3d\n", READ_BIT(fw_status.active_cmd_vcore_core1_info, 19, 16),
            READ_BIT(fw_status.active_cmd_vcore_core1_info, 20, 20),
            READ_BIT(fw_status.active_cmd_vcore_core1_info, 15, 0));
        VLOG(INFO, "[-] ACTIVE_CMD_INFO\n");

        VLOG(INFO, "[+] CMD_QUE_INFO\n");
        {
            Uint32 inst_id[64];
            Uint32 core_id[64];
            Uint32 cmd_idx[64];

            reg_base = &fw_status.cmd_que_info_inst_id_0;
            i = 0;
            for (k = 0; k < 8; k++)
            {
                for (j = 0; i < (8 * (k + 1)); i++, j++)
                    inst_id[i] = (((*reg_base) >> (4 * j)) & 0xF);

                reg_base++;
            }

            reg_base = &fw_status.cmd_que_info_core_id_0;
            i = 0;
            for (k = 0; k < 2; k++)
            {
                for (j = 0; i < (32 * (k + 1)); i++, j++)
                    core_id[i] = (((*reg_base) >> j) & 0x1);

                reg_base++;
            }

            reg_base = &fw_status.cmd_que_info_cmd_idx_0;
            i = 0;
            for (k = 0; k < 16; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                    cmd_idx[i] = (((*reg_base) >> (8 * j)) & 0xFF);

                reg_base++;
            }


            VLOG(INFO, "\t- tail %3d head %3d\n", READ_BIT(fw_status.cmd_que_pre_head_tail, 5, 0),
                READ_BIT(fw_status.cmd_que_pre_head_tail, 11, 6));
            for (i = 0; i < 64; i++)
                VLOG(INFO, "\t\t- [cmd_que_idx %3d] inst_id %3d core_id %3d cmd_idx %3d\n", i, inst_id[i], core_id[i], cmd_idx[i]);
        }
        VLOG(INFO, "[-] CMD_QUE_INFO\n");

        VLOG(INFO, "[+] VCORE_0_CMD_QUE_INFO\n");
        {
            Uint32 inst_id[64];
            Uint32 core_id[64];
            Uint32 cmd_idx[64];

            reg_base = &fw_status.vcore0_cmd_que_info_inst_id_0;
            i = 0;
            for (k = 0; k < 8; k++)
            {
                for (j = 0; i < (8 * (k + 1)); i++, j++)
                    inst_id[i] = (((*reg_base) >> (4 * j)) & 0xF);

                reg_base++;
            }

            reg_base = &fw_status.vcore0_cmd_que_info_core_id_0;
            i = 0;
            for (k = 0; k < 2; k++)
            {
                for (j = 0; i < (32 * (k + 1)); i++, j++)
                    core_id[i] = (((*reg_base) >> j) & 0x1);

                reg_base++;
            }

            reg_base = &fw_status.vcore0_cmd_que_info_cmd_idx_0;
            i = 0;
            for (k = 0; k < 16; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                    cmd_idx[i] = (((*reg_base) >> (8 * j)) & 0xFF);

                reg_base++;
            }


            VLOG(INFO, "\t- tail %3d head %3d\n", READ_BIT(fw_status.vcore0_cmd_que_head_tail, 5, 0),
                READ_BIT(fw_status.vcore0_cmd_que_head_tail, 11, 6));
            for (i = 0; i < 64; i++)
                VLOG(INFO, "\t\t- [vcore_cmd_que_idx %3d] inst_id %3d core_id %3d cmd_idx %3d\n", i, inst_id[i], core_id[i], cmd_idx[i]);
        }
        VLOG(INFO, "[-] VCORE_0_CMD_QUE_INFO\n");

        VLOG(INFO, "[+] VCORE_1_CMD_QUE_INFO\n");
        {
            Uint32 inst_id[64];
            Uint32 core_id[64];
            Uint32 cmd_idx[64];

            reg_base = &fw_status.vcore1_cmd_que_info_inst_id_0;
            i = 0;
            for (k = 0; k < 8; k++)
            {
                for (j = 0; i < (8 * (k + 1)); i++, j++)
                    inst_id[i] = (((*reg_base) >> (4 * j)) & 0xF);

                reg_base++;
            }

            reg_base = &fw_status.vcore1_cmd_que_info_core_id_0;
            i = 0;
            for (k = 0; k < 2; k++)
            {
                for (j = 0; i < (32 * (k + 1)); i++, j++)
                    core_id[i] = (((*reg_base) >> j) & 0x1);

                reg_base++;
            }

            reg_base = &fw_status.vcore1_cmd_que_info_cmd_idx_0;
            i = 0;
            for (k = 0; k < 16; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                    cmd_idx[i] = (((*reg_base) >> (8 * j)) & 0xFF);

                reg_base++;
            }


            VLOG(INFO, "\t- tail %3d head %3d\n", READ_BIT(fw_status.vcore1_cmd_que_head_tail, 5, 0),
                READ_BIT(fw_status.vcore1_cmd_que_head_tail, 11, 6));
            for (i = 0; i < 64; i++)
                VLOG(INFO, "\t\t- [vcore_cmd_que_idx %3d] inst_id %3d core_id %3d cmd_idx %3d\n", i, inst_id[i], core_id[i], cmd_idx[i]);
        }
        VLOG(INFO, "[-] VCORE_1_CMD_QUE_INFO\n");

        VLOG(INFO, "[+] PREV_RUN_VCORE_CMD_INFO\n");
        {
            Uint32 core_id[16];
            Uint32 cmd_idx[16];


            reg_base = &fw_status.prev_run_vcore_core_id;
            i = 0;
            for (k = 0; k < 1; k++)
            {
                for (j = 0; i < (16 * (k + 1)); i++, j++)
                    core_id[i] = (((*reg_base) >> j) & 0x1);

                reg_base++;
            }

            reg_base = &fw_status.prev_run_vcore_cmd_idx_inst_0123;
            i = 0;
            for (k = 0; k < 4; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                    cmd_idx[i] = (((*reg_base) >> (8 * j)) & 0xFF);

                reg_base++;
            }

            for (i = 0; i < 16; i++)
                VLOG(INFO, "\t-  [inst_id %3d] prev_run_core_id %3d prev_run_core_cmd_idx %3d\n", i, core_id[i], cmd_idx[i]);
        }
        VLOG(INFO, "[-] PREV_RUN_VCORE_CMD_INFO\n");

        VLOG(INFO, "[+] INSTANCE_CMD_QUE_INFO\n");
        {
            Uint32 core_id[64];
            Uint32 cmd_idx[64];
            Uint32 cnt[16];
            Uint32 tail[16];
            Uint32 next_alloc_core_id[16];

            reg_base = &fw_status.inst_0_7_cmd_que_core_id;
            i = 0;
            for (k = 0; k < 2; k++)
            {
                for (j = 0; i < (32 * (k + 1)); i++, j++)
                    core_id[i] = (((*reg_base) >> j) & 0x1);

                reg_base++;
            }

            reg_base = &fw_status.inst_0_cmd_que_cmd_idx;
            i = 0;
            for (k = 0; k < 16; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                    cmd_idx[i] = (((*reg_base) >> (8 * j)) & 0xFF);

                reg_base++;
            }

            reg_base = &fw_status.inst_0_3_cmd_que_cnt_tail;
            i = 0;
            for (k = 0; k < 4; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                {
                    cnt[i] = (((((*reg_base) >> (8 * j)) & 0xFF) >> 4) & 0xF);
                    tail[i] = (((((*reg_base) >> (8 * j)) & 0xFF) >> 0) & 0xF);
                }

                reg_base++;
            }

            reg_base = &fw_status.inst_cmd_que_next_alloc_core_id;
            i = 0;
            for (k = 0; k < 1; k++)
            {
                for (j = 0; i < (16 * (k + 1)); i++, j++)
                    next_alloc_core_id[i] = (((*reg_base) >> j) & 0x1);

                reg_base++;
            }

            for (i = 0; i <= 60; i += 4)
            {
                if ((i % 4) == 0)
                    VLOG(INFO, "\t- [inst_id %3d] tail %3d cnt %3d next_alloc_core_id %3d\n", i / 4, tail[i / 4], cnt[i / 4], next_alloc_core_id[i / 4]);

                VLOG(INFO, "\t\t- [inst_id %3d cq_idx %3d] core_id %3d cmd_idx %3d\n", i / 4, (i % 4) + 0, core_id[i + 0], cmd_idx[i + 0]);
                VLOG(INFO, "\t\t- [inst_id %3d cq_idx %3d] core_id %3d cmd_idx %3d\n", i / 4, (i % 4) + 1, core_id[i + 1], cmd_idx[i + 1]);
                VLOG(INFO, "\t\t- [inst_id %3d cq_idx %3d] core_id %3d cmd_idx %3d\n", i / 4, (i % 4) + 2, core_id[i + 2], cmd_idx[i + 2]);
                VLOG(INFO, "\t\t- [inst_id %3d cq_idx %3d] core_id %3d cmd_idx %3d\n", i / 4, (i % 4) + 3, core_id[i + 3], cmd_idx[i + 3]);
            }
        }
        VLOG(INFO, "[-] INSTANCE_CMD_QUE_INFO\n");

        VLOG(INFO, "[+] WATI_CMD_QUE_INFO\n");
        {
            int next_release_cmd[16];
            int core_id[64];
            int cmd_idx[64];

            reg_base = &fw_status.inst_0_3_cmd_que_next_rel_cmd_idx;
            i = 0;
            for (k = 0; k < 4; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                    next_release_cmd[i] = (((*reg_base) >> (8 * j)) & 0xFF);

                reg_base++;
            }

            reg_base = &fw_status.inst_0_7_wait_cmd_que_core_id;
            i = 0;
            for (k = 0; k < 2; k++)
            {
                for (j = 0; i < (32 * (k + 1)); i++, j++)
                    core_id[i] = (((*reg_base) >> j) & 0x1);

                reg_base++;
            }

            reg_base = &fw_status.inst_0_wait_cmd_que_cmd_idx;
            i = 0;
            for (k = 0; k < 16; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                    cmd_idx[i] = (((*reg_base) >> (8 * j)) & 0xFF);

                reg_base++;
            }

            for (i = 0; i <= 60; i += 4)
            {
                if ((i % 4) == 0)
                    VLOG(INFO, "\t- [inst_id %3d] next_release_cmd %3d\n", i / 4, next_release_cmd[i / 4]);

                VLOG(INFO, "\t\t- [inst_id %3d cq_idx %3d] core_id %3d cmd_idx %3d\n", i / 4, (i % 4) + 0, core_id[i + 0], cmd_idx[i + 0]);
                VLOG(INFO, "\t\t- [inst_id %3d cq_idx %3d] core_id %3d cmd_idx %3d\n", i / 4, (i % 4) + 1, core_id[i + 1], cmd_idx[i + 1]);
                VLOG(INFO, "\t\t- [inst_id %3d cq_idx %3d] core_id %3d cmd_idx %3d\n", i / 4, (i % 4) + 2, core_id[i + 2], cmd_idx[i + 2]);
                VLOG(INFO, "\t\t- [inst_id %3d cq_idx %3d] core_id %3d cmd_idx %3d\n", i / 4, (i % 4) + 3, core_id[i + 3], cmd_idx[i + 3]);
            }
        }
        VLOG(INFO, "[-] WATI_CMD_QUE_INFO\n");

        VLOG(INFO, "[+] REPORT_QUEUE_INFO\n");
        {
            int tail[16];
            int cnt[16];

            reg_base = &fw_status.inst_0_3_report_queue_cnt_tail;
            i = 0;
            for (k = 0; k < 4; k++)
            {
                for (j = 0; i < (4 * (k + 1)); i++, j++)
                {
                    cnt[i] = (((((*reg_base) >> (8 * j)) & 0xFF) >> 4) & 0xF);
                    tail[i] = (((((*reg_base) >> (8 * j)) & 0xFF) >> 0) & 0xF);
                }

                reg_base++;
            }

            for (i = 0; i < 16; i++)
                VLOG(INFO, "\t- [inst_id %3d] tail %3d cnt %3d\n", i, tail[i], cnt[i]);
        }
        VLOG(INFO, "[-] REPORT_QUEUE_INFO\n");
    }
    else
    {
        vdi_log(handle->coreIdx, handle->instIndex, 0, 1);
        if (bEncoder == TRUE)
            cmd_ret = VPU_EncGiveCommand(handle, GET_DEBUG_INFORM, debugInfo);
        else
            cmd_ret = VPU_DecGiveCommand(handle, GET_DEBUG_INFORM, debugInfo);
        if (cmd_ret != RETCODE_SUCCESS) {
            VLOG(ERR, "-%s GET_DEBUG_INFOM command fail \n", __FUNCTION__);
            vfree(debugInfo);
            return;
        }

        VLOG(TRACE, "++ interrupt flags \n");
        VLOG(TRACE, "     pend_intrpt_idc = 0x%x \n", (debugInfo->regs[0x0c]>>16)&0xffff);
        VLOG(TRACE, "     multi_int_reason = 0x%x \n", debugInfo->regs[0x0c]&0xffff);
        VLOG(TRACE, "     last interrupt reason = 0x%x \n", debugInfo->regs[0x0d]);
        VLOG(TRACE, "-- interrupt flags \n\n");

        VLOG(TRACE, "++ available core flags \n");
        VLOG(TRACE, "     STAGE_SEEK      core_avail_idc = %d \n", ((debugInfo->regs[0x0e] >> 0)&0x0f));
        VLOG(TRACE, "     STAGE_PARSING   core_avail_idc = %d \n", ((debugInfo->regs[0x0e] >> 4)&0x0f));
        VLOG(TRACE, "     STAGE_DEC/ENC   core_avail_idc = %d \n", ((debugInfo->regs[0x0e] >> 8)&0x0f));
        VLOG(TRACE, "     STAGE_PACKAGING core_avail_idc = %d \n", ((debugInfo->regs[0x0e] >> 12)&0x0f));
        VLOG(TRACE, "-- avaiable core flags \n\n");

        VLOG(TRACE, "++ the number of the allocated queue and commands in report queue\n");
        VLOG(TRACE, "     inst0={que_cnt=%d, rq_cnt=%d} inst1={que_cnt=%d, rq_cnt=%d} inst2={que_cnt=%d, rq_cnt=%d} inst3={que_cnt=%d, rq_cnt=%d}\n",
            ((((debugInfo->regs[0x06] >> 0)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x06] >> 0)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x06] >> 8)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x06] >> 8)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x06] >> 16)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x06] >> 16)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x06] >> 24)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x06] >> 24)&0xff)>>0) & 0xf));
        VLOG(TRACE, "     inst4={que_cnt=%d, rq_cnt=%d} inst5={que_cnt=%d, rq_cnt=%d} inst6={que_cnt=%d, rq_cnt=%d} inst7={que_cnt=%d, rq_cnt=%d}\n",
            ((((debugInfo->regs[0x07] >> 0)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x07] >> 0)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x07] >> 8)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x07] >> 8)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x07] >> 16)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x07] >> 16)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x07] >> 24)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x07] >> 24)&0xff)>>0) & 0xf));
        VLOG(TRACE, "     inst7={que_cnt=%d, rq_cnt=%d} inst9={que_cnt=%d, rq_cnt=%d} inst10={que_cnt=%d, rq_cnt=%d} inst11={que_cnt=%d, rq_cnt=%d}\n",
            ((((debugInfo->regs[0x08] >> 0)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x08] >> 0)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x08] >> 8)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x08] >> 8)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x08] >> 16)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x08] >> 16)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x08] >> 24)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x08] >> 24)&0xff)>>0) & 0xf));
        VLOG(TRACE, "     inst12={que_cnt=%d, rq_cnt=%d} inst13={que_cnt=%d, rq_cnt=%d} inst14={que_cnt=%d, rq_cnt=%d} inst15={que_cnt=%d, rq_cnt=%d}\n",
            ((((debugInfo->regs[0x09] >> 0)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x09] >> 0)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x09] >> 8)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x09] >> 8)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x09] >> 16)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x09] >> 16)&0xff)>>0) & 0xf),
            ((((debugInfo->regs[0x09] >> 24)&0xff)>>4) & 0xf), ((((debugInfo->regs[0x09] >> 24)&0xff)>>0) & 0xf));
        VLOG(TRACE, "-- the number of the allocated queue and commands in report queue\n\n");


        VLOG(TRACE, "++ status of instance handle\n");
        VLOG(TRACE, "     inst0 status=%d, inst1 status=%d, inst2 status=%d, inst3 status=%d \n",
            ((debugInfo->regs[0x0A] >> 0)&0xf), ((debugInfo->regs[0x0A] >> 4)&0xf), ((debugInfo->regs[0x0A] >> 8)&0xf), ((debugInfo->regs[0x0A] >> 12)&0xf));
        VLOG(TRACE, "     inst4 status=%d, inst5 status=%d, inst6 status=%d, inst7 status=%d \n",
            ((debugInfo->regs[0x0A] >> 16)&0xf), ((debugInfo->regs[0x0A] >> 20)&0xf), ((debugInfo->regs[0x0A] >> 24)&0xf), ((debugInfo->regs[0x0A] >> 28)&0xf));

        VLOG(TRACE, "     inst8 status=%d, inst9 status=%d, inst10 status=%d, inst11 status=%d \n",
            ((debugInfo->regs[0x0B] >> 0)&0xf), ((debugInfo->regs[0x0B] >> 4)&0xf), ((debugInfo->regs[0x0B] >> 8)&0xf), ((debugInfo->regs[0x0B] >> 12)&0xf));

        VLOG(TRACE, "     inst12 status=%d, inst13 status=%d, inst14 status=%d, inst15 status=%d \n",
            ((debugInfo->regs[0x0B] >> 16)&0xf), ((debugInfo->regs[0x0B] >> 20)&0xf), ((debugInfo->regs[0x0B] >> 24)&0xf), ((debugInfo->regs[0x0B] >> 28)&0xf));
        VLOG(TRACE, "-- status of instance handle\n\n");

        VLOG(TRACE, "++ active command in each stage\n");
        VLOG(TRACE, "     STAGE_SEEK, CMD_INST_ID = %d \n", (debugInfo->regs[0x10]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_QUE_CNT = %d \n", (debugInfo->regs[0x10]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_IDX = %d \n", (debugInfo->regs[0x11]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_BUF_IDX = %d \n", (debugInfo->regs[0x11]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_QUEUE_STATUS = %d \n", (debugInfo->regs[0x12]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_STATUS = %d \n", (debugInfo->regs[0x12]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_CMD = %d \n", (debugInfo->regs[0x13]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_USE_TASKBUF = %d \n", (debugInfo->regs[0x13]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_SCHED_STATUS = %d \n", (debugInfo->regs[0x14]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_SEEK, CMD_MAX_QUEUE_DEPTH = %d \n", (debugInfo->regs[0x14]>>0)&0xffff);


        VLOG(TRACE, "     STAGE_PARSING, CMD_INST_ID = %d \n", (debugInfo->regs[0x15]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_QUE_CNT = %d \n", (debugInfo->regs[0x15]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_IDX = %d \n", (debugInfo->regs[0x16]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_BUF_IDX = %d \n", (debugInfo->regs[0x16]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_QUEUE_STATUS = %d \n", (debugInfo->regs[0x17]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_STATUS = %d \n", (debugInfo->regs[0x17]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_CMD = %d \n", (debugInfo->regs[0x18]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_USE_TASKBUF = %d \n", (debugInfo->regs[0x18]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_SCHED_STATUS = %d \n", (debugInfo->regs[0x19]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PARSING, CMD_MAX_QUEUE_DEPTH = %d \n", (debugInfo->regs[0x19]>>0)&0xffff);


        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_INST_ID = %d \n", (debugInfo->regs[0x1a]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_QUE_CNT = %d \n", (debugInfo->regs[0x1a]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_IDX = %d \n", (debugInfo->regs[0x1b]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_BUF_IDX = %d \n", (debugInfo->regs[0x1b]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_QUEUE_STATUS = %d \n", (debugInfo->regs[0x1c]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_STATUS = %d \n", (debugInfo->regs[0x1c]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_CMD = %d \n", (debugInfo->regs[0x1d]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_USE_TASKBUF = %d \n", (debugInfo->regs[0x1d]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_SCHED_STATUS = %d \n", (debugInfo->regs[0x1e]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_DEC/ENCODING, CMD_MAX_QUEUE_DEPTH = %d \n", (debugInfo->regs[0x1e]>>0)&0xffff);

        VLOG(TRACE, "     STAGE_PACKAGING, CMD_INST_ID = %d \n", (debugInfo->regs[0x1f]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_QUE_CNT = %d \n", (debugInfo->regs[0x1f]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_IDX = %d \n", (debugInfo->regs[0x20]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_BUF_IDX = %d \n", (debugInfo->regs[0x20]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_QUEUE_STATUS = %d \n", (debugInfo->regs[0x21]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_STATUS = %d \n", (debugInfo->regs[0x21]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_CMD = %d \n", (debugInfo->regs[0x22]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_USE_TASKBUF = %d \n", (debugInfo->regs[0x22]>>0)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_SCHED_STATUS = %d \n", (debugInfo->regs[0x23]>>16)&0xffff);
        VLOG(TRACE, "     STAGE_PACKAGING, CMD_MAX_QUEUE_DEPTH = %d \n", (debugInfo->regs[0x23]>>0)&0xffff);
        VLOG(TRACE, "-- active command in each stage\n\n");


        VLOG(TRACE, "++ queued commands in each stage\n");
        VLOG(TRACE, "     STAGE_SEEK      inst0->cmd_cnt=%d, inst1->cmt_cmd=%d, inst2->cmd_cnt=%d, inst3=>cmd_cnt=%d inst4=>cmd_cnt=%d, inst5=>cmd_cnt=%d, inst6=>cmd_cnt=%d, inst7=>cmd_cnt=%d \n",
            ((debugInfo->regs[0x30] >> 0) & 0xf), ((debugInfo->regs[0x30] >> 4) & 0xf),
            ((debugInfo->regs[0x30] >> 8) & 0xf), ((debugInfo->regs[0x30] >> 12) & 0xf),
            ((debugInfo->regs[0x30] >> 16) & 0xf), ((debugInfo->regs[0x30] >> 20) & 0xf),
            ((debugInfo->regs[0x30] >> 24) & 0xf), ((debugInfo->regs[0x30] >> 28) & 0xf));
        VLOG(TRACE, "     STAGE_SEEK      inst8->cmd_cnt=%d, inst9->cmt_cmd=%d, inst10->cmd_cnt=%d, inst11=>cmd_cnt=%d inst12=>cmd_cnt=%d, inst13=>cmd_cnt=%d, inst14=>cmd_cnt=%d, inst15=>cmd_cnt=%d \n",
            ((debugInfo->regs[0x31] >> 0) & 0xf), ((debugInfo->regs[0x31] >> 4) & 0xf),
            ((debugInfo->regs[0x31] >> 8) & 0xf), ((debugInfo->regs[0x31] >> 12) & 0xf),
            ((debugInfo->regs[0x31] >> 16) & 0xf), ((debugInfo->regs[0x31] >> 20) & 0xf),
            ((debugInfo->regs[0x31] >> 24) & 0xf), ((debugInfo->regs[0x31] >> 28) & 0xf));
        VLOG(TRACE, "     STAGE_PARSING      inst0->cmd_cnt=%d, inst1->cmt_cmd=%d, inst2->cmd_cnt=%d, inst3=>cmd_cnt=%d inst4=>cmd_cnt=%d, inst5=>cmd_cnt=%d, inst6=>cmd_cnt=%d, inst7=>cmd_cnt=%d \n",
            ((debugInfo->regs[0x32] >> 0) & 0xf), ((debugInfo->regs[0x32] >> 4) & 0xf),
            ((debugInfo->regs[0x32] >> 8) & 0xf), ((debugInfo->regs[0x32] >> 12) & 0xf),
            ((debugInfo->regs[0x32] >> 16) & 0xf), ((debugInfo->regs[0x32] >> 20) & 0xf),
            ((debugInfo->regs[0x32] >> 24) & 0xf), ((debugInfo->regs[0x32] >> 28) & 0xf));
        VLOG(TRACE, "     STAGE_PARSING      inst8->cmd_cnt=%d, inst9->cmt_cmd=%d, inst10->cmd_cnt=%d, inst11=>cmd_cnt=%d inst12=>cmd_cnt=%d, inst13=>cmd_cnt=%d, inst14=>cmd_cnt=%d, inst15=>cmd_cnt=%d \n",
            ((debugInfo->regs[0x33] >> 0) & 0xf), ((debugInfo->regs[0x33] >> 4) & 0xf),
            ((debugInfo->regs[0x33] >> 8) & 0xf), ((debugInfo->regs[0x33] >> 12) & 0xf),
            ((debugInfo->regs[0x33] >> 16) & 0xf), ((debugInfo->regs[0x33] >> 20) & 0xf),
            ((debugInfo->regs[0x33] >> 24) & 0xf), ((debugInfo->regs[0x33] >> 28) & 0xf));
        VLOG(TRACE, "     STAGE_DEC/ENCODING      inst0->cmd_cnt=%d, inst1->cmt_cmd=%d, inst2->cmd_cnt=%d, inst3=>cmd_cnt=%d inst4=>cmd_cnt=%d, inst5=>cmd_cnt=%d, inst6=>cmd_cnt=%d, inst7=>cmd_cnt=%d \n",
            ((debugInfo->regs[0x34] >> 0) & 0xf), ((debugInfo->regs[0x34] >> 4) & 0xf),
            ((debugInfo->regs[0x34] >> 8) & 0xf), ((debugInfo->regs[0x34] >> 12) & 0xf),
            ((debugInfo->regs[0x34] >> 16) & 0xf), ((debugInfo->regs[0x34] >> 20) & 0xf),
            ((debugInfo->regs[0x34] >> 24) & 0xf), ((debugInfo->regs[0x34] >> 28) & 0xf));
        VLOG(TRACE, "     STAGE_DEC/ENCODING      inst8->cmd_cnt=%d, inst9->cmt_cmd=%d, inst10->cmd_cnt=%d, inst11=>cmd_cnt=%d inst12=>cmd_cnt=%d, inst13=>cmd_cnt=%d, inst14=>cmd_cnt=%d, inst15=>cmd_cnt=%d \n",
            ((debugInfo->regs[0x35] >> 0) & 0xf), ((debugInfo->regs[0x35] >> 4) & 0xf),
            ((debugInfo->regs[0x35] >> 8) & 0xf), ((debugInfo->regs[0x35] >> 12) & 0xf),
            ((debugInfo->regs[0x35] >> 16) & 0xf), ((debugInfo->regs[0x35] >> 20) & 0xf),
            ((debugInfo->regs[0x35] >> 24) & 0xf), ((debugInfo->regs[0x35] >> 28) & 0xf));
        VLOG(TRACE, "     STAGE_PACKAGING      inst0->cmd_cnt=%d, inst1->cmt_cmd=%d, inst2->cmd_cnt=%d, inst3=>cmd_cnt=%d inst4=>cmd_cnt=%d, inst5=>cmd_cnt=%d, inst6=>cmd_cnt=%d, inst7=>cmd_cnt=%d \n",
            ((debugInfo->regs[0x36] >> 0) & 0xf), ((debugInfo->regs[0x36] >> 4) & 0xf),
            ((debugInfo->regs[0x36] >> 8) & 0xf), ((debugInfo->regs[0x36] >> 12) & 0xf),
            ((debugInfo->regs[0x36] >> 16) & 0xf), ((debugInfo->regs[0x36] >> 20) & 0xf),
            ((debugInfo->regs[0x36] >> 24) & 0xf), ((debugInfo->regs[0x36] >> 28) & 0xf));
        VLOG(TRACE, "     STAGE_PACKAGING      inst8->cmd_cnt=%d, inst9->cmt_cmd=%d, inst10->cmd_cnt=%d, inst11=>cmd_cnt=%d inst12=>cmd_cnt=%d, inst13=>cmd_cnt=%d, inst14=>cmd_cnt=%d, inst15=>cmd_cnt=%d \n",
            ((debugInfo->regs[0x37] >> 0) & 0xf), ((debugInfo->regs[0x37] >> 4) & 0xf),
            ((debugInfo->regs[0x37] >> 8) & 0xf), ((debugInfo->regs[0x37] >> 12) & 0xf),
            ((debugInfo->regs[0x37] >> 16) & 0xf), ((debugInfo->regs[0x37] >> 20) & 0xf),
            ((debugInfo->regs[0x37] >> 24) & 0xf), ((debugInfo->regs[0x37] >> 28) & 0xf));
        VLOG(TRACE, "-- queued commands in each stage\n\n");

        VLOG(TRACE, "-- vp9 vcore wtl status \n\n");
        VLOG(TRACE, "DEBUG_SCHED_VCORE_STATUS %08x \n", (debugInfo->regs[0x38]));
        VLOG(TRACE, "DEBUG_FBC_AVAIL          %08x \n", (debugInfo->regs[0x39]));
        VLOG(TRACE, "DEBUG_WTL_AVAIL          %08x \n", (debugInfo->regs[0x3A]));
        VLOG(TRACE, "-- vp9 vcore wtl status \n\n");
        VLOG(INFO, "-%s core=%d, inst=%d, bEncoder=%d\n", __FUNCTION__, handle->coreIdx, handle->instIndex, bEncoder);
    }

    vfree(debugInfo);
}


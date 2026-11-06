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

#include "vdi_debug.h"
#include "vdi_osal.h"
#include "vpuapifunc.h"
#include "coda9/coda9_regdefine.h"
#include "wave/wave5_regdefine.h"
#include "wave/wave6_regdefine.h"

extern void vpu_update_stat_cycles(int coreIdx, int hwCycles);

static Uint32 vdi_core_stat_fps[MAX_NUM_VPU_CORE] = {0};
static Uint32 vdi_core_stat_fps_non_intra[MAX_NUM_VPU_CORE] = {0};
static Uint64 vdi_core_stat_lastts[MAX_NUM_VPU_CORE] = {0};
static Uint32 vdi_inst_stat_fps[MAX_NUM_VPU_CORE][MAX_NUM_INSTANCE] = {0};

static int read_pinfo_buffer(int coreIdx, int addr)
{
    int ack;
    int rdata;
#define VDI_LOG_GDI_PINFO_ADDR  (0x1068)
#define VDI_LOG_GDI_PINFO_REQ   (0x1060)
#define VDI_LOG_GDI_PINFO_ACK   (0x1064)
#define VDI_LOG_GDI_PINFO_DATA  (0x106c)
    //------------------------------------------
    // read pinfo - indirect read
    // 1. set read addr     (GDI_PINFO_ADDR)
    // 2. send req          (GDI_PINFO_REQ)
    // 3. wait until ack==1 (GDI_PINFO_ACK)
    // 4. read data         (GDI_PINFO_DATA)
    //------------------------------------------
    vdi_write_register(coreIdx, VDI_LOG_GDI_PINFO_ADDR, addr);
    vdi_write_register(coreIdx, VDI_LOG_GDI_PINFO_REQ, 1);

    ack = 0;
    while (ack == 0)
    {
        ack = vdi_read_register(coreIdx, VDI_LOG_GDI_PINFO_ACK);
    }

    rdata = vdi_read_register(coreIdx, VDI_LOG_GDI_PINFO_DATA);

    //VLOG(INFO, "[READ PINFO] ADDR[%x], DATA[%x]", addr, rdata);
    return rdata;
}

static void printf_gdi_info(int coreIdx, int num, int reset)
{
    int i;
    int bus_info_addr;
    int tmp;
    int val;
    int productId = 0;
#define VDI_PRODUCT_ID_980 (0)
#define VDI_PRODUCT_ID_960 (1)

    val = vdi_read_register(coreIdx, VPU_PRODUCT_CODE_REGISTER);
    if ((val&0xff00) == 0x3200) val = 0x3200;

    if (PRODUCT_CODE_W_SERIES(val)) {
        return;
    }
    else if (PRODUCT_CODE_CODA_SERIES(val)) {
        if (val == CODA960_CODE || val == BODA950_CODE)
            productId = VDI_PRODUCT_ID_960;
        else if (val == CODA980_CODE)
            productId = VDI_PRODUCT_ID_980;
        else
            return;
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", val);
        return;
    }

    if (productId == VDI_PRODUCT_ID_980)
        VLOG(INFO, "\n**GDI information for GDI_20\n");
    else
        VLOG(INFO, "\n**GDI information for GDI_10\n");

    for (i=0; i < num; i++)
    {

#define VDI_LOG_GDI_INFO_CONTROL 0x1400
        if (productId == VDI_PRODUCT_ID_980)
            bus_info_addr = VDI_LOG_GDI_INFO_CONTROL + i*(0x20);
        else
            bus_info_addr = VDI_LOG_GDI_INFO_CONTROL + i*0x14;
        if (reset)
        {
            vdi_write_register(coreIdx, bus_info_addr, 0x00);
            bus_info_addr += 4;
            vdi_write_register(coreIdx, bus_info_addr, 0x00);
            bus_info_addr += 4;
            vdi_write_register(coreIdx, bus_info_addr, 0x00);
            bus_info_addr += 4;
            vdi_write_register(coreIdx, bus_info_addr, 0x00);
            bus_info_addr += 4;
            vdi_write_register(coreIdx, bus_info_addr, 0x00);

            if (productId == VDI_PRODUCT_ID_980)
            {
                bus_info_addr += 4;
                vdi_write_register(coreIdx, bus_info_addr, 0x00);

                bus_info_addr += 4;
                vdi_write_register(coreIdx, bus_info_addr, 0x00);

                bus_info_addr += 4;
                vdi_write_register(coreIdx, bus_info_addr, 0x00);
            }

        }
        else
        {
            VLOG(INFO, "index = %02d", i);

            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);    //TiledEn<<20 ,GdiFormat<<17,IntlvCbCr,<<16 GdiYuvBufStride
            VLOG(INFO, " control = 0x%08x", tmp);

            bus_info_addr += 4;
            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
            VLOG(INFO, " pic_size = 0x%08x", tmp);

            bus_info_addr += 4;
            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
            VLOG(INFO, " y-top = 0x%08x", tmp);

            bus_info_addr += 4;
            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
            VLOG(INFO, " cb-top = 0x%08x", tmp);

            bus_info_addr += 4;
            tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
            VLOG(INFO, " cr-top = 0x%08x", tmp);
            if (productId == VDI_PRODUCT_ID_980)
            {
                bus_info_addr += 4;
                tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
                VLOG(INFO, " y-bot = 0x%08x", tmp);

                bus_info_addr += 4;
                tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
                VLOG(INFO, " cb-bot = 0x%08x", tmp);

                bus_info_addr += 4;
                tmp = read_pinfo_buffer(coreIdx, bus_info_addr);
                VLOG(INFO, " cr-bot = 0x%08x", tmp);
            }
            VLOG(INFO, "\n");
        }
    }
}

static void make_log(unsigned long instIdx, const char *str, int step)
{ //lint !e578
    if (step == 1)
        VLOG(INFO, "\n**%s start(%d)\n", str, instIdx);
    else if (step == 2)    //
        VLOG(INFO, "\n**%s timeout(%d)\n", str, instIdx);
    else
        VLOG(INFO, "\n**%s end(%d)\n", str, instIdx);
}

void vdi_log(unsigned long coreIdx, unsigned long instIdx, int cmd, int step)
{ //lint !e578
    int i;
    int productId;
    int start_addr = 0x200;
    int end_addr = 0x600;

    if (coreIdx >= MAX_NUM_VPU_CORE)
        return ;

    productId = VPU_GetProductId(coreIdx);

    if (PRODUCT_ID_W6_SERIES(productId))
    {
        switch(cmd)
        {
        case W6_INIT_VPU:
            make_log(instIdx, "W6_INIT_VPU", step);
            break;
        case W6_WAKEUP_VPU:
            make_log(instIdx, "W6_WAKEUP_VPU", step);
            break;
        case W6_CREATE_INSTANCE:
            make_log(instIdx, "W6_CREATE_INSTANCE", step);
            break;
        case W6_FLUSH_INSTANCE:
            make_log(instIdx, "W6_FLUSH_INSTANCE", step);
            break;
        case W6_DESTROY_INSTANCE:
            make_log(instIdx, "W6_DESTROY_INSTANCE", step);
            break;
        case W6_INIT_SEQ:
            make_log(instIdx, "W6_INIT_SEQ", step);
            break;
        case W6_SET_FB:
            make_log(instIdx, "W6_SET_FB", step);
            break;
        case W6_DEC_PIC: //W6_ENC_PIC for ENC
            make_log(instIdx, "W6_DEC_PIC(W6_ENC_PIC)", step);
            break;
        case W6_ENC_SET_PARAM:
            make_log(instIdx, "W6_ENC_SET_PARAM", step);
            break;
        case W6_GET_VPU_INFO: //W6_QUERY for DUAL
            make_log(instIdx, "W6_GET_VPU_INFO(W6_QUERY)", step);
            break;
        default:
            make_log(instIdx, "ANY_CMD", step);
            break;
        }
    }
    else if (PRODUCT_ID_W5_SERIES(productId))
    {
        switch(cmd)
        {
        case W5_INIT_VPU:
            make_log(instIdx, "INIT_VPU", step);
            break;
        case W5_ENC_SET_PARAM:
            make_log(instIdx, "ENC_SET_PARAM", step);
            break;
        case W5_INIT_SEQ:
            make_log(instIdx, "DEC INIT_SEQ", step);
            break;
        case W5_DESTROY_INSTANCE:
            make_log(instIdx, "DESTROY_INSTANCE", step);
            break;
        case W5_DEC_PIC://ENC_PIC for ENC
            make_log(instIdx, "DEC_PIC(ENC_PIC)", step);
            break;
        case W5_SET_FB:
            make_log(instIdx, "SET_FRAMEBUF", step);
            break;
        case W5_FLUSH_INSTANCE:
            make_log(instIdx, "FLUSH INSTANCE", step);
            break;
        case W5_QUERY:
            make_log(instIdx, "QUERY", step);
            break;
        case W5_SLEEP_VPU:
            make_log(instIdx, "SLEEP_VPU", step);
            break;
        case W5_WAKEUP_VPU:
            make_log(instIdx, "WAKEUP_VPU", step);
            break;
        case W5_UPDATE_BS:
            make_log(instIdx, "UPDATE_BS", step);
            break;
        case W5_CREATE_INSTANCE:
            make_log(instIdx, "CREATE_INSTANCE", step);
            break;
        default:
            make_log(instIdx, "ANY_CMD", step);
            break;
        }
    }
    else if (PRODUCT_ID_CODA_SERIES(productId))
    {
        switch(cmd)
        {
        case ENC_SEQ_INIT://DEC_SEQ_INNT
            make_log(instIdx, "SEQ_INIT", step);
            break;
        case ENC_SEQ_END://DEC_SEQ_END
            make_log(instIdx, "SEQ_END", step);
            break;
        case PIC_RUN:
            make_log(instIdx, "PIC_RUN", step);
            break;
        case SET_FRAME_BUF:
            make_log(instIdx, "SET_FRAME_BUF", step);
            break;
        case ENCODE_HEADER:
            make_log(instIdx, "ENCODE_HEADER", step);
            break;
        case ENC_CHANGE_PARAMETER:
            make_log(instIdx, "ENC_CHANGE_PARAMETER", step);
            break;
        case DEC_BUF_FLUSH:
            make_log(instIdx, "DEC_BUF_FLUSH", step);
            break;
        case FIRMWARE_GET:
            make_log(instIdx, "FIRMWARE_GET", step);
            break;
        case ENC_PARA_SET:
            make_log(instIdx, "ENC_PARA_SET", step);
            break;
        case DEC_PARA_SET:
            make_log(instIdx, "DEC_PARA_SET", step);
            break;
        default:
            make_log(instIdx, "ANY_CMD", step);
            break;
        }
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productId);
        return;
    }

    if (PRODUCT_ID_W6_SERIES(productId)) {
        start_addr = 0x200;
        end_addr = 0x600;
    }
    else {
        start_addr = 0x0;
        end_addr = 0x200;
    }

    for (i=start_addr; i<end_addr; i=i+16) { // host IF register 0x100 ~ 0x200
        VLOG(INFO, "0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
            vdi_read_register(coreIdx, i), vdi_read_register(coreIdx, i+4),
            vdi_read_register(coreIdx, i+8), vdi_read_register(coreIdx, i+0xc));
    }
    for (i=P_MBC_PIC_SUB_FRAME_CMD; i<P_MBC_ENC_SUB_FRAME_SYNC; i=i+16) { // host IF register 0x100 ~ 0x200
        VLOG(INFO, "0x%04xh: 0x%08x 0x%08x 0x%08x 0x%08x\n", i,
            vdi_read_register(coreIdx, i), vdi_read_register(coreIdx, i+4),
            vdi_read_register(coreIdx, i+8), vdi_read_register(coreIdx, i+0xc));
    }
    if (PRODUCT_ID_W_SERIES(productId))
    {
//         if (cmd == W5_INIT_VPU || cmd == W5_CREATE_INSTANCE)
//         {
// #ifdef SUPPORT_V4L2_DRIVER
// #else
//             vdi_print_vpu_status(coreIdx);
// #endif
//         }
    }
    else if (PRODUCT_ID_CODA_SERIES(productId))
    {
        if (cmd == PIC_RUN && step== 0)
        {
            printf_gdi_info(coreIdx, 32, 0);

#define VDI_LOG_MBC_BUSY 0x0440
#define VDI_LOG_MC_BASE	 0x0C00
#define VDI_LOG_MC_BUSY	 0x0C04
#define VDI_LOG_GDI_BUS_STATUS (0x10F4)
#define VDI_LOG_ROT_SRC_IDX	 (0x400 + 0x10C)
#define VDI_LOG_ROT_DST_IDX	 (0x400 + 0x110)

            VLOG(INFO, "MBC_BUSY = %x\n", vdi_read_register(coreIdx, VDI_LOG_MBC_BUSY));
            VLOG(INFO, "MC_BUSY = %x\n", vdi_read_register(coreIdx, VDI_LOG_MC_BUSY));
            VLOG(INFO, "MC_MB_XY_DONE=(y:%d, x:%d)\n", (vdi_read_register(coreIdx, VDI_LOG_MC_BASE) >> 20) & 0x3F, (vdi_read_register(coreIdx, VDI_LOG_MC_BASE) >> 26) & 0x3F);
            VLOG(INFO, "GDI_BUS_STATUS = %x\n", vdi_read_register(coreIdx, VDI_LOG_GDI_BUS_STATUS));

            VLOG(INFO, "ROT_SRC_IDX = %x\n", vdi_read_register(coreIdx, VDI_LOG_ROT_SRC_IDX));
            VLOG(INFO, "ROT_DST_IDX = %x\n", vdi_read_register(coreIdx, VDI_LOG_ROT_DST_IDX));

            VLOG(INFO, "P_MC_PIC_INDEX_0 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x200));
            VLOG(INFO, "P_MC_PIC_INDEX_1 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x20c));
            VLOG(INFO, "P_MC_PIC_INDEX_2 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x218));
            VLOG(INFO, "P_MC_PIC_INDEX_3 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x230));
            VLOG(INFO, "P_MC_PIC_INDEX_3 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x23C));
            VLOG(INFO, "P_MC_PIC_INDEX_4 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x248));
            VLOG(INFO, "P_MC_PIC_INDEX_5 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x254));
            VLOG(INFO, "P_MC_PIC_INDEX_6 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x260));
            VLOG(INFO, "P_MC_PIC_INDEX_7 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x26C));
            VLOG(INFO, "P_MC_PIC_INDEX_8 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x278));
            VLOG(INFO, "P_MC_PIC_INDEX_9 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x284));
            VLOG(INFO, "P_MC_PIC_INDEX_a = %x\n", vdi_read_register(coreIdx, MC_BASE+0x290));
            VLOG(INFO, "P_MC_PIC_INDEX_b = %x\n", vdi_read_register(coreIdx, MC_BASE+0x29C));
            VLOG(INFO, "P_MC_PIC_INDEX_c = %x\n", vdi_read_register(coreIdx, MC_BASE+0x2A8));
            VLOG(INFO, "P_MC_PIC_INDEX_d = %x\n", vdi_read_register(coreIdx, MC_BASE+0x2B4));

            VLOG(INFO, "P_MC_PICIDX_0 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x028));
            VLOG(INFO, "P_MC_PICIDX_1 = %x\n", vdi_read_register(coreIdx, MC_BASE+0x02C));
        }
    }
    else {
        VLOG(ERR, "Unknown product id : %08x\n", productId);
        return;
    }
}


void vdi_stat_fps(int coreIdx, int instIndex, int picType)
{
    Uint64 currentTs = osal_gettime();
    int i = 0;

    if (picType != PIC_TYPE_MAX) {
        vdi_core_stat_fps[coreIdx] += 1;
    }

    if (picType == PIC_TYPE_P || picType == PIC_TYPE_B) {
        vdi_core_stat_fps_non_intra[coreIdx] += 1;
    }

    vdi_inst_stat_fps[coreIdx][instIndex] += 1;

    if (vdi_core_stat_lastts[coreIdx] == 0) {
        vdi_core_stat_lastts[coreIdx] = currentTs;
    }

    if (currentTs >= vdi_core_stat_lastts[coreIdx] + 1000 ) {
        VLOG(INFO, "vdi core:%d fps:%d non-intra:%d \n"
            , coreIdx, vdi_core_stat_fps[coreIdx], vdi_core_stat_fps_non_intra[coreIdx]);

        VLOG(INFO, "vdi inst[%d][%d][%d][%d][%d][%d][%d][%d][%d][%d] \n",
            vdi_inst_stat_fps[coreIdx][0],
            vdi_inst_stat_fps[coreIdx][1],
            vdi_inst_stat_fps[coreIdx][2],
            vdi_inst_stat_fps[coreIdx][3],
            vdi_inst_stat_fps[coreIdx][4],
            vdi_inst_stat_fps[coreIdx][5],
            vdi_inst_stat_fps[coreIdx][6],
            vdi_inst_stat_fps[coreIdx][7],
            vdi_inst_stat_fps[coreIdx][8],
            vdi_inst_stat_fps[coreIdx][9]);
        vdi_core_stat_lastts[coreIdx] = currentTs;
        vdi_core_stat_fps[coreIdx] = 0;
        vdi_core_stat_fps_non_intra[coreIdx] = 0;
        for ( i = 0; i<10; ++i) {
            vdi_inst_stat_fps[coreIdx][i] = 0;
        }
    }
}

void vdi_stat_hwcycles(int coreIdx, int hwCycles)
{
    vpu_update_stat_cycles(coreIdx, hwCycles);
}
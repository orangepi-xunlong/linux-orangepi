/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2023 Vivante Corporation
*
*    Permission is hereby granted, free of charge, to any person obtaining a
*    copy of this software and associated documentation files (the "Software"),
*    to deal in the Software without restriction, including without limitation
*    the rights to use, copy, modify, merge, publish, distribute, sublicense,
*    and/or sell copies of the Software, and to permit persons to whom the
*    Software is furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
*    DEALINGS IN THE SOFTWARE.
*
*****************************************************************************
*
*    The GPL License (GPL)
*
*    Copyright (C) 2017 - 2023 Vivante Corporation
*
*    This program is free software; you can redistribute it and/or
*    modify it under the terms of the GNU General Public License
*    as published by the Free Software Foundation; either version 2
*    of the License, or (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*****************************************************************************
*
*    Note: This software is released under dual MIT and GPL licenses. A
*    recipient may use this file under the terms of either the MIT license or
*    GPL License. If you wish to use only one license not the other, you can
*    indicate your decision by deleting one of the above license notices in your
*    version of this file.
*
*****************************************************************************/

#include <gc_vip_hardware.h>
#include <gc_vip_kernel.h>
#include <gc_vip_kernel_port.h>
#include <gc_vip_video_memory.h>
#include <gc_vip_kernel_drv.h>
#include <gc_vip_kernel_debug.h>
#include <gc_vip_kernel_pm.h>
#include <gc_vip_version.h>
#include <gc_vip_kernel_drv.h>
#include <gc_vip_kernel_command.h>

#define ENABLE_DUMP_AHB_REGISTER       0
#define ENABLE_DUMP_MMU_MAPPING        0

#if vpmdENABLE_HANG_DUMP
typedef struct _gcsiDEBUG_REGISTERS
{
    vip_char_t     *module;
    vip_uint32_t    index;
    vip_uint32_t    shift;
    vip_uint32_t    data;
    vip_uint32_t    count;
    vip_uint32_t    pipeMask;
    vip_uint32_t    selectStart;
    vip_bool_e      avail;
    vip_bool_e      inCluster;
}
gcsiDEBUG_REGISTERS;

typedef struct _gcOther_REGISTERS
{
    vip_uint32_t   address;
    vip_char_t     *name;
}
gcOther_REGISTERS;

static vip_status_e verify_dma(
    vip_uint32_t *address1,
    vip_uint32_t *address2,
    vip_uint32_t *state1,
    vip_uint32_t *state2,
    gckvip_hardware_t *hardware
    )
{
    vip_status_e error = VIP_SUCCESS;
    vip_int32_t i;

    do {
        gckvip_read_register(hardware, 0x00660, state1);
        gckvip_read_register(hardware, 0x00660, state1);
        gckvip_read_register(hardware, 0x00664, address1);
        gckvip_read_register(hardware, 0x00664, address1);
        for (i = 0; i < 500; i += 1) {
            gckvip_read_register(hardware, 0x00660, state2);
            gckvip_read_register(hardware, 0x00660, state2);
            gckvip_read_register(hardware, 0x00664, address2);
            gckvip_read_register(hardware, 0x00664, address2);
            if (*address1 != *address2) {
                break;
            }
            if (*state1 != *state2) {
                break;
            }
        }
    } while (0);

    return error;
}

static vip_status_e dump_fe_stack(
    gckvip_hardware_t *hardware
    )
{
    vip_int32_t i;
    vip_int32_t j;
    vip_uint32_t stack[32][2];
    vip_uint32_t link[32];

    typedef struct _gcsFE_STACK
    {
        vip_char_t  *   name;
        vip_int32_t     count;
        vip_uint32_t    highSelect;
        vip_uint32_t    lowSelect;
        vip_uint32_t    linkSelect;
        vip_uint32_t    clear;
        vip_uint32_t    next;
    }
    gcsFE_STACK;

    static gcsFE_STACK _feStacks[] =
    {
        { "PRE_STACK", 32, 0x1A, 0x9A, 0x00, 0x1B, 0x1E },
        { "CMD_STACK", 32, 0x1C, 0x9C, 0x1E, 0x1D, 0x1E },
    };

    for (i = 0; i < 2; i++) {
        gckvip_write_register(hardware, 0x00470, _feStacks[i].clear);

        for (j = 0; j < _feStacks[i].count; j++) {
            gckvip_write_register(hardware, 0x00470, _feStacks[i].highSelect);

            gckvip_read_register(hardware, 0x00450, &stack[j][0]);

            gckvip_write_register(hardware, 0x00470, _feStacks[i].lowSelect);

            gckvip_read_register(hardware, 0x00450, &stack[j][1]);

            gckvip_write_register(hardware, 0x00470, _feStacks[i].next);

            if (_feStacks[i].linkSelect) {
                gckvip_write_register(hardware, 0x00470, _feStacks[i].linkSelect);

                gckvip_read_register(hardware, 0x00450, &link[j]);
            }
        }

        PRINTK("   %s:\n", _feStacks[i].name);

        for (j = 31; j >= 3; j -= 4) {
            PRINTK("     %08X %08X %08X %08X %08X %08X %08X %08X\n",
                            stack[j][0], stack[j][1], stack[j - 1][0], stack[j - 1][1],
                            stack[j - 2][0], stack[j - 2][1], stack[j - 3][0],
                            stack[j - 3][1]);
        }

        if (_feStacks[i].linkSelect) {
            PRINTK("   LINK_STACK:\n");

            for (j = 31; j >= 3; j -= 4) {
                PRINTK("     %08X %08X %08X %08X %08X %08X %08X %08X\n",
                                link[j], link[j], link[j - 1], link[j - 1],
                                link[j - 2], link[j - 2], link[j - 3], link[j - 3]);
            }
        }

    }

    return VIP_SUCCESS;
}

static vip_status_e _dump_debug_registers(
    gckvip_hardware_t *hardware,
    IN gcsiDEBUG_REGISTERS *descriptor
    )
{
    /* If this value is changed, print formats need to be changed too. */
    #define REG_PER_LINE 8
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t select;
    vip_uint32_t i, j, pipe;
    vip_uint32_t datas[REG_PER_LINE];
    vip_uint32_t oldControl, control;

    /* Record control. */
    gckvip_read_register(hardware, 0x0, &oldControl);

    for (pipe = 0; pipe < 4; pipe++) {
        if (!descriptor->avail) {
            continue;
        }
        if (!(descriptor->pipeMask & (1 << pipe))) {
            continue;
        }

        PRINTK("     %s[%d] debug registers:\n", descriptor->module, pipe);

        /* Switch pipe. */
        gckvip_read_register(hardware, 0x0, &control);
        control &= ~(0xF << 20);
        control |= (pipe << 20);
        gckvip_write_register(hardware, 0x0, control);

        for (i = 0; i < descriptor->count; i += REG_PER_LINE) {
            /* Select of first one in the group. */
            select = i + descriptor->selectStart;

            /* Read a group of registers. */
            for (j = 0; j < REG_PER_LINE; j++) {
                /* Shift select to right position. */
                gckvip_write_register(hardware, descriptor->index, (select + j) << descriptor->shift);
                gckvip_read_register(hardware, descriptor->data, &datas[j]);
            }

            PRINTK("     [%02X] %08X %08X %08X %08X %08X %08X %08X %08X\n",
                            select, datas[0], datas[1], datas[2], datas[3], datas[4],
                            datas[5], datas[6], datas[7]);
        }
    }

    /* Restore control. */
    gckvip_write_register(hardware, 0x0, oldControl);
    return status;
}

#if vpmdENABLE_MMU
#define _read_page_entry(page_entry)  *(vip_uint32_t*)(page_entry)

static vip_status_e gckvip_show_exception(
    gckvip_context_t *context,
    vip_uint32_t address,
    vip_uint32_t index
    )
{
    vip_uint32_t stlb_shift = 0;
    vip_uint32_t stlb_mask = 0;
    vip_uint32_t pgoff_mask = 0;
    vip_uint32_t mtlb = 0;
    vip_uint32_t stlb = 0;
    vip_uint32_t offset = 0;
    vip_uint32_t *stlb_logical = VIP_NULL;
    vip_uint32_t stlb_physical = 0;
    vip_uint8_t page_type = 0;
    volatile vip_uint8_t *bitmap = VIP_NULL;
    vip_uint32_t page_index = 0;

    mtlb = (address & gcdMMU_MTLB_MASK) >> gcdMMU_MTLB_SHIFT;
    if (mtlb >= gcdMMU_1M_MTLB_COUNT) {
        page_type = GCVIP_MMU_4K_PAGE;
    }
    else {
        page_type = GCVIP_MMU_1M_PAGE;
    }

    if (GCVIP_MMU_1M_PAGE == page_type) {
        pgoff_mask = gcdMMU_OFFSET_1M_MASK;
        stlb_shift = gcdMMU_STLB_1M_SHIFT;
        stlb_mask  = gcdMMU_STLB_1M_MASK;
    }
    else {
        pgoff_mask = gcdMMU_OFFSET_4K_MASK;
        stlb_shift = gcdMMU_STLB_4K_SHIFT;
        stlb_mask  = gcdMMU_STLB_4K_MASK;
    }

    stlb = (address & stlb_mask) >> stlb_shift;
    offset =  address & pgoff_mask;

    PRINTK("  MMU%d: exception address = 0x%08X\n", index, address);
    PRINTK("    page type = %s\n", (page_type == GCVIP_MMU_4K_PAGE)?"4K":"1M");
    PRINTK("    MTLB entry index = %d\n", mtlb);
    PRINTK("    STLB entry index = %d\n", stlb);
    PRINTK("    Offset = 0x%08X\n", offset);

    PRINTK("    1M page max num = %d\n", context->max_1Mpage);
    PRINTK("    1M page max MTLB index = %d\n", context->max_1Mpage / gcdMMU_STLB_1M_ENTRY_NUM);
    PRINTK("    1M page max STLB index = %d\n", context->max_1Mpage % gcdMMU_STLB_1M_ENTRY_NUM);
    PRINTK("    4K page max num = %d\n", context->max_4Kpage);
    PRINTK("    4K page MTLB base index = %d\n", gcdMMU_1M_MTLB_COUNT);
    PRINTK("    4K page max MTLB index = %d\n",
        context->max_4Kpage / gcdMMU_STLB_4K_ENTRY_NUM + gcdMMU_1M_MTLB_COUNT);
    PRINTK("    4K page max STLB index = %d\n", context->max_4Kpage % gcdMMU_STLB_4K_ENTRY_NUM);

    /* get page entry*/
    if (GCVIP_MMU_1M_PAGE == page_type) {
        stlb_logical = (vip_uint32_t*)((vip_uint8_t*)context->ptr_STLB_1M->logical + \
                       (mtlb * gcdMMU_STLB_1M_SIZE));
        stlb_physical = context->ptr_STLB_1M->physical +  (mtlb * gcdMMU_STLB_1M_SIZE);
        bitmap = context->bitmap_1M;
        page_index = (mtlb * gcdMMU_STLB_1M_ENTRY_NUM) + stlb * sizeof(vip_uint8_t);
    }
    else if (GCVIP_MMU_4K_PAGE == page_type) {
        stlb_logical = (vip_uint32_t*)((vip_uint8_t*)context->ptr_STLB_4K->logical +  \
                       ((mtlb - gcdMMU_1M_MTLB_COUNT) * gcdMMU_STLB_4K_SIZE));
        stlb_physical = context->ptr_STLB_4K->physical + (mtlb - gcdMMU_1M_MTLB_COUNT) * gcdMMU_STLB_4K_SIZE;
        bitmap = context->bitmap_4K;
        page_index = ((mtlb - gcdMMU_1M_MTLB_COUNT) * gcdMMU_STLB_4K_ENTRY_NUM) \
                        + stlb * sizeof(vip_uint8_t);
    }

    PRINTK("    Page table entry bitmap = 0x%X\n", \
        (bitmap[page_index / (8 * sizeof(vip_uint8_t))] & \
        (1 << (page_index % (8 * sizeof(vip_uint8_t))))) >> (page_index % (8 * sizeof(vip_uint8_t))));
    PRINTK("    Page table entry address = 0x%08X\n",
                    stlb_physical + stlb * sizeof(vip_uint32_t));
    PRINTK("    Page table entry value = 0x%08X\n",
                     _read_page_entry(stlb_logical + stlb));

    return VIP_SUCCESS;
}

static vip_status_e gckvip_dump_mtlb(
    gckvip_context_t *context
    )
{
    PRINTK("\n*****DUMP MMU entry******\n");
    gckvip_dump_data(context->MMU_entry.logical, context->MMU_entry.physical,
                    (context->MMU_entry.size > 64) ? 64 : context->MMU_entry.size);

    PRINTK("*****DUMP MTLB******\n");
    gckvip_dump_data(context->MTLB.logical, context->MTLB.physical, context->MTLB.size);

    return VIP_SUCCESS;
}

static vip_status_e gckvip_dump_stlb(
    gckvip_context_t *context
    )
{
    vip_int32_t i = 0;
    vip_uint32_t *logical = VIP_NULL;
    vip_int32_t unit = 0;
    vip_uint32_t dump_size = 0;
    vip_uint32_t *tmp = VIP_NULL;
    vip_int32_t total_num = 0;

    PRINTK("\n*****DUMP STLB Start******\n");
    PRINTK("dump 1Mbytes STLB:\n");
    PRINTK("1Mbytes STLB physical base=0x%08x, size=0x%x\n",
                    context->ptr_STLB_1M->physical, context->ptr_STLB_1M->size);

    logical = (vip_uint32_t*)context->ptr_STLB_1M->logical;
    unit = gcdMMU_1M_MTLB_COUNT * gcdMMU_STLB_1M_ENTRY_NUM;
    if (logical != VIP_NULL) {
        vip_uint32_t *data = (vip_uint32_t*)logical;
        for (i = unit - 1; i >= 0; i--) {
            /* 0 bit indicate whether present or not. */
            if (data[i] & (1 << 0)) {
                break;
            }
        }

        total_num = i + 1;
        for (i = 0; i < total_num; i += 8) {
            tmp = (vip_uint32_t*)context->ptr_STLB_1M->logical + i;
            if ((total_num - i) >= 8) {
                dump_size = 8 * sizeof(vip_uint32_t);
            }
            else {
                dump_size = (total_num - i) * sizeof(vip_uint32_t);
            }
            gckvip_dump_data(tmp,
                             context->ptr_STLB_1M->physical + sizeof(vip_uint32_t) * i,
                             dump_size);
        }
    }

    PRINTK("dump 4Kbytes STLB:\n");
    PRINTK("4Kbytes STLB physical base=0x%08x, size=0x%x\n",
                    context->ptr_STLB_4K->physical, context->ptr_STLB_4K->size);

    unit = gcdMMU_4K_MTLB_COUNT * gcdMMU_STLB_4K_ENTRY_NUM;
    logical = (vip_uint32_t*)context->ptr_STLB_4K->logical;
    if (logical != VIP_NULL) {
        vip_uint32_t *data = (vip_uint32_t*)logical;
        for (i = unit - 1; i >= 0; i--) {
            /* 0 bit indicate whether present or not. */
            if (data[i] & (1 << 0)) {
                break;
            }
        }

        total_num = i + 1;
        for (i = 0; i < total_num; i += 8) {
            tmp = (vip_uint32_t*)context->ptr_STLB_4K->logical + i;
            if ((total_num - i) >= 8) {
                dump_size = 8 * sizeof(vip_uint32_t);
            }
            else {
                dump_size = (total_num - i) * sizeof(vip_uint32_t);
            }
            gckvip_dump_data(tmp,
                             context->ptr_STLB_4K->physical + sizeof(vip_uint32_t) * i,
                             dump_size);
        }
    }

    PRINTK("*****DUMP STLB Done******\n");

    return VIP_SUCCESS;
}

static vip_status_e gckvip_dump_mmu_info(
    gckvip_context_t *context,
    gckvip_hardware_t *hardware
    )
{
    vip_uint32_t mmu_status = 0;
    vip_uint32_t mmu_temp = 0;
    vip_uint32_t address = 0;
    vip_uint32_t mmu = 0;
    vip_uint32_t i = 0;
    vip_uint32_t axi_sram_size = 0;

    gckvip_read_register(hardware, 0x00384, &mmu_status);
    PRINTK("   MMU status = 0x%08X\n", mmu_status);

    if (mmu_status > 0) {
        PRINTK("\n ***********************\n");
        PRINTK("***   MMU STATUS DUMP   ***\n");
        PRINTK("\n ***********************\n");
        mmu_temp = mmu_status;
    }

    for (i = 0; i < context->device_count; i++) {
        axi_sram_size += context->axi_sram_size[i];
    }

    for (i = 0; i < 4; i++) {
        mmu = mmu_status & 0xF;
        mmu_status >>= 4;

        if (mmu == 0) {
            continue;
        }

        switch (mmu) {
        case 1:
              PRINTK("  MMU%d: slave not present\n", i);
              break;

        case 2:
              PRINTK("  MMU%d: page not present\n", i);
              break;

        case 3:
              PRINTK("  MMU%d: write violation\n", i);
              break;

        case 4:
              PRINTK("  MMU%d: out of bound", i);
              break;

        case 5:
              PRINTK("  MMU%d: read security violation", i);
              break;

        case 6:
              PRINTK("  MMU%d: write security violation", i);
              break;

        default:
              PRINTK("  MMU%d: unknown state\n", i);
        }

        gckvip_read_register(hardware, 0x00380, &address);

        if ((address >= context->axi_sram_virtual) &&
            (address < (context->axi_sram_virtual + axi_sram_size))) {
            PRINTK("  MMU%d: exception address = 0x%08X, mapped AXI SRAM.\n",
                            i, address);
        }
        else if ((address >= context->vip_sram_address) &&
                 (address < (context->vip_sram_address + context->vip_sram_size[0]))) {
            PRINTK("  MMU%d: exception address = 0x%08X, mapped VIP SRAM.\n",
                            i, address);
        }
        #if vpmdENABLE_VIDEO_MEMORY_HEAP
        else if ((address >= context->heap_virtual) &&
                 (address < (context->heap_virtual + context->heap_size))) {
            PRINTK("  MMU%d: exception address = 0x%08X, mapped video memory heap.\n",
                            i, address);
        }
        #endif
        else {
            PRINTK("  MMU%d: exception address = 0x%08X, "
                            "it is not mapped or dynamic memory.\n",  i, address);
        }

        gckvip_show_exception(context, address, i);

        if (mmu_temp > 0) {
            gckvip_dump_mtlb(context);
            gckvip_dump_stlb(context);
        }
    }

    return VIP_SUCCESS;
}
#endif

/*
    @ brief
    Dump debug register values. So far just for IDLE, FE, PE, and SH.
*/
vip_status_e gckvip_dump_states(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_uint32_t idleState = 0;
    vip_uint32_t dmaaddress1 = 0, dmaaddress2 = 0;
    vip_uint32_t dmastate1 = 0, dmastate2 = 0, dmaReqState = 0;
    vip_uint32_t cmdState = 0, cmdDmaState = 0, cmdFetState = 0, calState = 0;
    vip_uint32_t axiStatus = 0, dmaLo = 0, dmaHi = 0, veReqState = 0;
    vip_uint32_t control = 0, oldControl = 0, pipe = 0, mmuData = 0;
    vip_uint32_t ack_value = 0;
    vip_uint32_t hiControl = 0, clk_gate = 0;
    static vip_char_t *_cmdState[] =
    {
        "PAR_IDLE_ST", "PAR_DEC_ST", "PAR_ADR0_ST", "PAR_LOAD0_ST",
        "PAR_ADR1_ST", "PAR_LOAD1_ST", "PAR_3DADR_ST", "PAR_3DCMD_ST",
        "PAR_3DCNTL_ST", "PAR_3DIDXCNTL_ST", "PAR_INITREQDMA_ST", "PAR_DRAWIDX_ST",
        "PAR_DRAW_ST", "PAR_2DRECT0_ST", "PAR_2DRECT1_ST", "PAR_2DDATA0_ST",
        "PAR_2DDATA1_ST", "PAR_WAITFIFO_ST", "PAR_WAIT_ST", "PAR_LINK_ST",
        "PAR_END_ST", "PAR_STALL_ST", "INVALID_PAR_ST", "INVALID_PAR_ST",
        "INVALID_PAR_ST", "INVALID_PAR_ST", "INVALID_PAR_ST", "INVALID_PAR_ST",
        "INVALID_PAR_ST", "INVALID_PAR_ST", "INVALID_PAR_ST", "INVALID_PAR_ST"
    };
    static vip_char_t * _cmdDmaState[] =
    {
        "CMD_IDLE_ST", "CMD_START_ST", "CMD_REQ_ST", "CMD_END_ST"
    };
    static vip_char_t * _cmdFetState[] =
    {
        "FET_IDLE_ST", "FET_RAMVALID_ST", "FET_VALID_ST", "INVALID_FET_ST"
    };
    static vip_char_t * _reqDmaState[] =
    {
        "REQ_IDLE_ST", "REQ_WAITIDX_ST", "REQ_CAL_ST", "INVALID_REQ_ST"
    };
    static vip_char_t * _calState[] =
    {
        "CAL_IDLE_ST", "CAL_LDADR_ST", "CAL_IDXCALC_ST", "INVALID_CAL_ST"
    };
    static vip_char_t * _veReqState[] =
    {
        "VER_IDLE_ST", "VER_CKCACHE_ST", "VER_MISS_ST", "INVALID_VER_ST"
    };
    /* All module names here. */
    const char *module_name[] = {
        "FE",        "NULL",      "PE",        "SH",        "NULL",
        "NULL",      "NULL",      "NULL",      "NULL",      "NULL",
        "NULL",      "NULL",      "NULL",      "FE BLT",    "MC",
        "NULL",      "NULL",      "NULL",      "NN",        "TP",
        "Scaler",    "MMU"
    };

    /* must keep order correctly for _dbgRegs, we need ajust some value base on the index
    0x474, 24, 0x44C:
    NN ==> 0x00 - 0x80
    TP => 1000_0000 - 1001_1111  [0x80 - 0x9F] size is 0x20
        0x80: layer_i, total commit TP operations.
        0x84: TP total busy cycle
    other(MC/USC) =>  1100_00000 - 1101_1111  [0xC0 - 0xDF]
    NN => 1110_0000 -> 1111_1111.

    SH[0] debug registers: [30] FFFFEFFE, bit0 is 0 indicate NN or TP not idle.
    */
    static gcsiDEBUG_REGISTERS _dbgRegs[] =
    {
        { "RA",  0x474, 16, 0x448, 256, 0x1, 0x00, vip_true_e,  vip_true_e  },
        /*{ "TX",  0x474, 24, 0x44C, 256, 0x2, 0x00, vip_true_e,  vip_true_e  },*/
        { "FE",  0x470,  0, 0x450, 256, 0x1, 0x00, vip_true_e,  vip_false_e },
        { "PE",  0x470, 16, 0x454, 256, 0x3, 0x00, vip_true_e,  vip_true_e  },
        /*{ "DE",  0x470,  8, 0x458, 256, 0x1, 0x00, vip_true_e,  vip_false_e },*/
        { "SH",  0x470, 24, 0x45C, 256, 0x1, 0x00, vip_true_e,  vip_true_e  },
        /*{ "PA",  0x474,  0, 0x460, 256, 0x1, 0x00, vip_true_e,  vip_true_e  },*/
        { "SE",  0x474,  8, 0x464, 256, 0x1, 0x00, vip_true_e,  vip_true_e  },
        { "MC",  0x478,  0, 0x468, 256, 0x3, 0x00, vip_true_e,  vip_true_e  },
        { "HI",  0x478,  8, 0x46C, 256, 0x1, 0x00, vip_true_e,  vip_false_e },
        { "TP",  0x474, 24, 0x44C,  32, 0x2, 0x80, vip_true_e, vip_true_e  },
        /*{ "TFB", 0x474, 24, 0x44C,  32, 0x2, 0xA0, vip_true_e, vip_true_e  },*/
        { "USC", 0x474, 24, 0x44C,  64, 0x2, 0xC0, vip_true_e, vip_true_e  },
        /*{ "L2",  0x478,  0, 0x564, 256, 0x1, 0x00, vip_true_e,  vip_false_e },*/
        /*{ "BLT", 0x478, 24, 0x1A4, 256, 0x1, 0x00, vip_true_e, vip_true_e  },*/
        { "NN",  0x474, 24, 0x44C, 256, 0x2, 0x00, vip_true_e, vip_true_e  },
    };

    static gcOther_REGISTERS _otherRegs[] =
    {
        {0x00044, "TotalWrites"},
        {0x0004C, "TotalWriteBursts"}, {0x00050, "TotalWriteReqs"},
        {0x00054, "TotalWriteLasts"},
        {0x00040, "TotalReads"},
        {0x00058, "TotalReadBursts"}, {0x0005C, "TotalReadReqs"},
        {0x00060, "TotalReadLasts"},
        {0x0043C, "OT_Rread0"}, {0x00440, "OT_Read1"},
        {0x00444, "OT_Writes"}, {0x00414, "MEM"}, {0x00100, "Power"},
    };
    vip_uint32_t i = 0, core_index = 0;
    vip_uint32_t n_modules = sizeof(module_name) / sizeof(const char*);

    PRINTK("   ***********************************\n");
    PRINTK("   ******** VIPLite hang dump ********\n");
    PRINTK("   chip ver1     = 0x%08X\n", context->chip_ver1);
    PRINTK("   chip ver2     = 0x%08X\n", context->chip_ver2);
    PRINTK("   chip date     = 0x%08X\n", context->chip_date);
    PRINTK("   chip cid      = 0x%08X\n", context->chip_cid);
    PRINTK("   ***********************************\n");
    if (VIP_NULL == device) {
        PRINTK_E("fail device object is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    for (core_index = 0; core_index < device->hardware_count; core_index++) {
        hardware = &device->hardware[core_index];
        PRINTK("*********Hardware %d start***************\n", hardware->core_id);
        /* Verify whether DMA is running. */
        gcOnError(verify_dma(&dmaaddress1, &dmaaddress2, &dmastate1, &dmastate2, hardware));

        cmdState    = dmastate2         & 0x1F;
        cmdDmaState = (dmastate2 >> 8)  & 0x03;
        cmdFetState = (dmastate2 >> 10) & 0x03;
        dmaReqState = (dmastate2 >> 12) & 0x03;
        calState    = (dmastate2 >> 14) & 0x03;
        veReqState  = (dmastate2 >> 16) & 0x03;

        /* Get debug value. */
        gckvip_read_register(hardware, 0x00004, &idleState);
        gckvip_read_register(hardware, 0x00000, &hiControl);
        gckvip_read_register(hardware, 0x0000C, &axiStatus);
        gckvip_read_register(hardware, 0x00668, &dmaLo);
        gckvip_read_register(hardware, 0x00668, &dmaLo);
        gckvip_read_register(hardware, 0x0066C, &dmaHi);
        gckvip_read_register(hardware, 0x0066C, &dmaHi);

        PRINTK("   axi     = 0x%08X\n", axiStatus);
        if ((axiStatus >> 8) & 0x1) {
            PRINTK("     Detect write error ID=%d\n", axiStatus & 0xF);
        }
        if ((axiStatus >> 9) & 0x1) {
            PRINTK("     Detect read error ID=%d\n", (axiStatus >> 4) & 0xF);
        }
        if ((idleState & 0x80000000) != 0) {
            PRINTK("   AXI low power mode\n");
        }
        PRINTK("   idle    = 0x%08X\n", idleState);
        for (i = 0; i < n_modules; i++) {
            if ((idleState & 0x01) == 0) {
                PRINTK("     %s: not idle.\n", module_name[i]);
            }
            idleState >>= 1;
        }
        PRINTK("   HI_CLOCK_CONTROL = 0x%08X\n", hiControl);

        if ((dmaaddress1 == dmaaddress2) && (dmastate1 == dmastate2)) {
            PRINTK("   DMA appears to be stuck at this address: 0x%08X \n",
                            dmaaddress1);
        }
        else {
            if (dmaaddress1 == dmaaddress2) {
                PRINTK("   DMA address is constant, but state is changing:\n");
                PRINTK("      0x%08X\n", dmastate1);
                PRINTK("      0x%08X\n", dmastate2);
            }
            else {
                PRINTK("   DMA is running, known addresses are:\n");
                PRINTK("     0x%08X\n", dmaaddress1);
                PRINTK("     0x%08X\n", dmaaddress2);
            }
        }

        PRINTK("   dmaLow   = 0x%08X\n", dmaLo);
        PRINTK("   dmaHigh  = 0x%08X\n", dmaHi);
        PRINTK("   dmaState = 0x%08X\n", dmastate2);

        PRINTK("     command state       = %d (%s)\n", cmdState, _cmdState[cmdState]);
        PRINTK("     command DMA state   = %d (%s)\n", cmdDmaState, _cmdDmaState[cmdDmaState]);
        PRINTK("     command fetch state = %d (%s)\n", cmdFetState, _cmdFetState[cmdFetState]);
        PRINTK("     DMA request state   = %d (%s)\n", dmaReqState, _reqDmaState[dmaReqState]);
        PRINTK("     cal state           = %d (%s)\n", calState, _calState[calState]);
        PRINTK("     VE request state    = %d (%s)\n", veReqState, _veReqState[veReqState]);

        /* read interrupt value */
        gckvip_read_register(hardware, 0x00010, &ack_value);
        PRINTK("   interrupt ACK value=0x%x\n", ack_value);
        PRINTK("   irq value=0x%x\n", *hardware->irq_value);

        /* Record control. */
        gckvip_read_register(hardware, 0x0, &oldControl);

        /* Switch pipe. */
        gckvip_read_register(hardware, 0x0, &control);
        control &= ~(0xF << 20);
        control |= (pipe << 20);
        gckvip_write_register(hardware, 0x0, control);

        PRINTK("   Debug registers:\n");
        /* disable NN/TP clock gating */
        gckvip_read_register(hardware, 0x00178, &clk_gate);
        clk_gate = SETBIT(clk_gate, 0, 1);
        clk_gate = SETBIT(clk_gate, 7, 1);
        gckvip_write_register(hardware, 0x00178, clk_gate);
        for (i = 0; i < sizeof(_dbgRegs) / sizeof(_dbgRegs[0]); i += 1) {
            _dump_debug_registers(hardware, &_dbgRegs[i]);
        }
        clk_gate = SETBIT(clk_gate, 0, 0);
        clk_gate = SETBIT(clk_gate, 7, 0);
        gckvip_write_register(hardware, 0x00178, clk_gate);

        PRINTK("   other registers:\n");
        for (i = 0; i < (sizeof(_otherRegs) / sizeof((_otherRegs)[0])); i += 1) {
            vip_uint32_t read = 0;
            gckvip_read_register(hardware, _otherRegs[i].address, &read);
            PRINTK("      [0x%04X]=0x%08X %s\n", _otherRegs[i].address, read, _otherRegs[i].name);
        }

        /* dump MMU info */
        gckvip_read_register(hardware, 0x00388, &mmuData);
        if ((mmuData & 0x01) == 0x00) {
            PRINTK("   MMU has been disabled\n");
        }
        #if vpmdENABLE_MMU
        else {
            PRINTK("   MMU enabled\n");
            gckvip_dump_mmu_info(context, hardware);
        }
        #endif

        /* Restore control. */
        gckvip_write_register(hardware, 0x0, oldControl);

        PRINTK("   dump FE stack:\n");
        dump_fe_stack(hardware);

        PRINTK("*********Hardware %d end***************\n", hardware->core_id);
    }

    if (device != VIP_NULL) {
        PRINTK("**************************\n");
        PRINTK("*****   SW COUNTERS  *****\n");
        PRINTK("**************************\n");
        PRINTK("    APP Submit Count = 0x%08X\n", device->user_submit_count);
        PRINTK("    HW  Submit Count = 0x%08X\n", device->hw_submit_count);
        PRINTK("    Execute Addr  = 0x%08X\n", device->curr_command.cmd_physical);
        PRINTK("    End     Addr  = 0x%08X\n",
               device->curr_command.cmd_physical + device->curr_command.cmd_size);
        PRINTK("    Execute Cmd Size = 0x%08X\n", device->curr_command.cmd_size);
    }

onError:
    return status;
}

#if vpmdENABLE_WAIT_LINK_LOOP
vip_status_e gckvip_dump_waitlink(
    gckvip_device_t *device
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t *data = VIP_NULL;
    vip_uint32_t physical = 0;
    gckvip_hardware_t *hardware = VIP_NULL;
    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        data = hardware->waitlinkbuf_logical;
        physical = hardware->waitlinkbuf_physical;
        PRINTK("   Hardware%d WAIT LINK BUF DUMP\n", hardware->core_id);
        PRINTK("%08X : %08X %08X %08X %08X\n", physical, data[0], data[1], data[2], data[3]);
    }
    return VIP_SUCCESS;
}
#endif

vip_status_e gckvip_dump_data(
    vip_uint32_t *data,
    vip_uint32_t physical,
    vip_uint32_t size
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t line = size / 32;
    vip_uint32_t left = size % 32;

    for (i = 0; i < line; i++) {
        PRINTK("%08X : %08X %08X %08X %08X %08X %08X %08X %08X\n",
                  physical, data[0], data[1], data[2], data[3],
                  data[4], data[5], data[6], data[7]);
        data += 8;
        physical += 8 * 4;
    }

    switch(left) {
    case 30:
      PRINTK("%08X : %08X %08X %08X %08X %08X %08X %08X %08X\n",
              physical, data[0], data[1], data[2], data[3],
              data[4], data[5], data[6], data[7]);
        break;
    case 28:
    case 26:
      PRINTK("%08X : %08X %08X %08X %08X %08X %08X %08X\n",
                physical, data[0], data[1], data[2], data[3],
                data[4], data[5], data[6]);
      break;
    case 24:
    case 22:
      PRINTK("%08X : %08X %08X %08X %08X %08X %08X\n",
                physical, data[0], data[1], data[2], data[3],
                data[4], data[5]);
      break;
    case 20:
    case 18:
      PRINTK("%08X : %08X %08X %08X %08X %08X\n",
                physical, data[0], data[1], data[2], data[3],
                data[4]);
      break;
    case 16:
    case 14:
      PRINTK("%08X : %08X %08X %08X %08X\n", physical, data[0],
                data[1], data[2], data[3]);
      break;
    case 12:
    case 10:
      PRINTK("%08X : %08X %08X %08X\n", physical, data[0],
                data[1], data[2]);
      break;
    case 8:
    case 6:
      PRINTK("%08X : %08X %08X\n", physical, data[0], data[1]);
      break;
    case 4:
    case 2:
      PRINTK("%08X : %08X\n", physical, data[0]);
      break;
    default:
      break;
    }

    return VIP_SUCCESS;
}

#if ENABLE_DUMP_MMU_MAPPING
vip_status_e gckvip_dump_MMU_mapping(void)
{
#define DATA_PER_LINE 4
#define SIZE_PER_MSG 26
#define BUFFER_SIZE 256
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t first_free_pos = 0;
    vip_uint32_t iter = 0;
    vip_uint32_t phy_iter = 0;
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    vip_bool_e heap_not_profile = vip_true_e;
#endif
    gckvip_video_mem_handle_t *pointer = VIP_NULL;
    gckvip_database_table_t *table = VIP_NULL;
    gckvip_context_t *context = gckvip_get_context();
    vip_int32_t left_size = BUFFER_SIZE - 1;
    char buffer[BUFFER_SIZE] = {'\0'};
    char* p = buffer;

    if (0 == context->initialize) {
        PRINTK_E("vip not working, fail to dump mmu mapping.\n");
        return status;
    }

    status = gckvip_os_lock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("lock memory mutex faild, status = %d.\n", status);
        return status;
    }
    PRINTK("\n**************************\n");
    PRINTK("dump video memory start...\n");

    first_free_pos = gckvip_database_get_freepos(&context->videomem_database);
    for (iter = 0; iter < first_free_pos; iter++) {
        gckvip_database_get_table(&context->videomem_database, iter + DATABASE_MAGIC_DATA, &table);
        if (table->handle != VIP_NULL) {
            pointer = (gckvip_video_mem_handle_t*)table->handle;
        #if vpmdENABLE_VIDEO_MEMORY_HEAP
            if (GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP == pointer->memory_type) {
                if (heap_not_profile) {
                    PRINTK("@{0x%08X, 0x%08X\n", context->heap_virtual,
                                                 context->heap_size);
                    PRINTK("  [0x%08X, 0x%08X]\n", context->heap_physical,
                                                   context->heap_size);
                    PRINTK("}\n");
                    heap_not_profile = vip_false_e;
                }
            }
            else
        #endif
            {
                PRINTK("@{0x%08X, 0x%08X\n", pointer->node.dyn_node.vip_virtual_address,
                                             pointer->node.dyn_node.size);
                for (phy_iter = 0; phy_iter < pointer->node.dyn_node.physical_num; phy_iter++) {
                    gckvip_os_snprint(p, left_size, "  [0x%08X, 0x%08X]",
                                           pointer->node.dyn_node.physical_table[phy_iter],
                                           pointer->node.dyn_node.size_table[phy_iter]);
                    p += SIZE_PER_MSG;
                    left_size -= SIZE_PER_MSG;
                    if ((pointer->node.dyn_node.physical_num - 1 == phy_iter) ||
                        (DATA_PER_LINE - 1 == phy_iter % DATA_PER_LINE))
                    {
                        gckvip_os_snprint(p, left_size, "\n");
                        PRINTK(buffer);
                        p = buffer;
                        left_size = BUFFER_SIZE - 1;
                    }
                }
                PRINTK("}\n");
            }
        }
    }

    PRINTK("dump video memory end...\n");
    PRINTK("**************************\n\n");
    status = gckvip_os_unlock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("unlock memory mutex faild, status = %d.\n", status);
        return status;
    }

    return status;
}
#endif

#if ENABLE_DUMP_AHB_REGISTER
static vip_uint32_t gckvip_read_reg(
    gckvip_hardware_t *hardware,
    vip_uint32_t address
    )
{
    vip_uint32_t data = 0;
    gckvip_read_register(hardware, address, &data);
    return data;
}

vip_status_e gckvip_dump_AHB_register(
    gckvip_device_t *device
    )
{
    vip_uint32_t reg_size = 0x800;
    vip_uint32_t line =  reg_size / 32;
    vip_uint32_t left =  reg_size % 32;
    vip_uint32_t physical = 0;
    vip_uint32_t i = 0;
    vip_uint32_t index = 0;

    for (index = 0; index < device->hardware_count; index++) {
        gckvip_hardware_t *hardware = &device->hardware[index];
        PRINTK("**************************\n");
        PRINTK("   AHB REGISTER DUMP\n");
        PRINTK("**************************\n");
        PRINTK("core id=%d, AHB register size: %d bytes\n", hardware->core_id, reg_size);

        for (i = 0; i < line; i++)
        {
            PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                            physical, physical + 28, gckvip_read_reg(hardware, physical),
                            gckvip_read_reg(hardware, physical + 1*4),
                            gckvip_read_reg(hardware, physical + 2*4),
                            gckvip_read_reg(hardware, physical + 3*4),
                            gckvip_read_reg(hardware, physical + 4*4),
                            gckvip_read_reg(hardware, physical + 5*4),
                            gckvip_read_reg(hardware, physical + 6*4),
                            gckvip_read_reg(hardware, physical + 7*4));
            physical += 32;
        }

        switch(left) {
        case 28:
          PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X %08X %08X %08X\n",
                        physical, physical + 24, gckvip_read_reg(hardware, physical),
                        gckvip_read_reg(hardware, physical + 1*4),
                        gckvip_read_reg(hardware, physical + 2*4),
                        gckvip_read_reg(hardware, physical + 3*4),
                        gckvip_read_reg(hardware, physical + 4*4),
                        gckvip_read_reg(hardware, physical + 5*4),
                        gckvip_read_reg(hardware, physical + 6*4)
                        );
          break;
        case 24:
          PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X %08X %08X\n",
                        physical, physical + 20, gckvip_read_reg(hardware, physical),
                        gckvip_read_reg(hardware, physical + 1*4),
                        gckvip_read_reg(hardware, physical + 2*4),
                        gckvip_read_reg(hardware, physical + 3*4),
                        gckvip_read_reg(hardware, physical + 4*4),
                        gckvip_read_reg(hardware, physical + 5*4)
                        );
          break;
        case 20:
          PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X %08X\n",
                        physical, physical + 16, gckvip_read_reg(hardware, physical),
                        gckvip_read_reg(hardware, physical + 1*4),
                        gckvip_read_reg(hardware, physical + 2*4),
                        gckvip_read_reg(hardware, physical + 3*4),
                        gckvip_read_reg(hardware, physical + 4*4));
          break;
        case 16:
          PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X\n",
                         physical, physical + 12, gckvip_read_reg(hardware, physical),
                         gckvip_read_reg(hardware, physical + 1*4),
                         gckvip_read_reg(hardware, physical + 2*4),
                         gckvip_read_reg(hardware, physical + 3*4));
          break;
        case 12:
          PRINTK("address: %08X - %08X, value: %08X %08X %08X\n",
                          physical, physical + 8, gckvip_read_reg(hardware, physical),
                          gckvip_read_reg(hardware, physical + 1*4),
                          gckvip_read_reg(hardware, physical + 2*4));
          break;
        case 8:
          PRINTK("address: %08X - %08X, value: %08X %08X\n",
                          physical, physical + 4, gckvip_read_reg(hardware, physical),
                          gckvip_read_reg(hardware, physical + 4)
                          );
          break;
        case 4:
          PRINTK("address: %08X, value: %08X\n", physical,
                           gckvip_read_reg(hardware, physical));
          break;
        default:
          break;
        }
    }
    return VIP_SUCCESS;
}
#endif

vip_status_e gckvip_dump_command(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_uint32_t dmaAddr = 0xFFFFF;
    vip_uint32_t dmaLo = 0;
    vip_uint32_t dmaHi = 0;
    gckvip_submit_t *submit = &device->curr_command;
    gckvip_submit_t *pre_submit = &device->pre_command;
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_uint32_t i = 0;

    if (VIP_NULL != submit->cmd_logical) {
        PRINTK("**************************\n");
        PRINTK("   COMMAND BUF DUMP\n");
        PRINTK("**************************\n");
        if (submit->cmd_physical == submit->last_cmd_physical) {
            PRINTK("command total size=0x%xbytes, physical=0x%08x, logical=0x%"PRPx"\n",
                submit->cmd_size, submit->cmd_physical, submit->cmd_logical);
        }
        else {
            PRINTK("first command total size=0x%xbytes, physical=0x%08x, logical=0x%"PRPx"\n",
                submit->cmd_size, submit->cmd_physical, submit->cmd_logical);
            PRINTK("last command total size=0x%xbytes, physical=0x%08x, logical=0x%"PRPx"\n",
                submit->last_cmd_size, submit->last_cmd_physical, submit->last_cmd_logical);
        }
        for (i = 0; i < device->hardware_count; i++) {
            hardware = &device->hardware[i];
            gckvip_read_register(hardware, 0x00668, &dmaLo);
            gckvip_read_register(hardware, 0x0066C, &dmaHi);
            gckvip_read_register(hardware, 0x00664, &dmaAddr);
            PRINTK("Hardware%d DMA Address = 0x%08X\n", hardware->core_id, dmaAddr);
            PRINTK("   dmaLow   = 0x%08X\n", dmaLo);
            PRINTK("   dmaHigh  = 0x%08X\n", dmaHi);
        }
        gckvip_dump_data((vip_uint32_t*)submit->cmd_logical, submit->cmd_physical, submit->cmd_size);
        if (submit->cmd_physical != submit->last_cmd_physical) {
            gckvip_dump_data((vip_uint32_t*)submit->last_cmd_logical,
                submit->last_cmd_physical, submit->last_cmd_size);
        }
    }
    else {
        PRINTK("can't dump curr command buffer, it is NULL\n");
    }

    if (VIP_NULL != pre_submit->cmd_logical) {
        if (pre_submit->cmd_physical == pre_submit->last_cmd_physical) {
            PRINTK("pre commit command total size=0x%xbytes, physical=0x%08x, logical=0x%"PRPx"\n",
                pre_submit->cmd_size, pre_submit->cmd_physical, pre_submit->cmd_logical);
        }
        else {
            PRINTK("pre commit first command total size=0x%xbytes, physical=0x%08x, logical=0x%"PRPx"\n",
                pre_submit->cmd_size, pre_submit->cmd_physical, pre_submit->cmd_logical);
            PRINTK("pre commit last command total size=0x%xbytes, physical=0x%08x, logical=0x%"PRPx"\n",
                pre_submit->last_cmd_size, pre_submit->last_cmd_physical, pre_submit->last_cmd_logical);
        }
        gckvip_dump_data((vip_uint32_t*)pre_submit->cmd_logical, pre_submit->cmd_physical, pre_submit->cmd_size);
        if (pre_submit->cmd_physical != pre_submit->last_cmd_physical) {
            gckvip_dump_data((vip_uint32_t*)pre_submit->last_cmd_logical,
                pre_submit->last_cmd_physical, pre_submit->last_cmd_size);
        }
    }
    else {
        PRINTK("can't dump pre command buffer, it is NULL\n");
    }

    return VIP_SUCCESS;
}

static vip_status_e dump_video_memory_layout(
    gckvip_context_t *context,
    gckvip_video_mem_handle_t *handle,
    vip_uint32_t index
    )
{
    vip_status_e status = VIP_SUCCESS;
    phy_address_t virtual_addr = 0;
    vip_uint32_t physical_num = 0, size = 0;

    vip_bool_e is_heap_alloc = (handle->memory_type == GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP) ? vip_true_e : vip_false_e;

    if (!is_heap_alloc) {
        virtual_addr = handle->node.dyn_node.vip_virtual_address;
        physical_num = handle->node.dyn_node.physical_num;
        size = handle->node.dyn_node.size;
        PRINTK("[%d, 0x%08x, 0x%08x, phy_n=0x%x]\n", index, virtual_addr, size, physical_num);
    }
    #if vpmdENABLE_VIDEO_MEMORY_HEAP
    else {
        phy_address_t physical = 0;
        size = handle->node.heap_node.size;
        physical = handle->node.heap_node.physical;
        if (handle->node.heap_node.mem_flag & GCVIP_MEM_FLAG_NO_MMU_PAGE) {
            virtual_addr  = physical;
        }
        else {
            status = gckvip_mem_get_vipvirtual(context, (void*)handle, physical, (phy_address_t*)&virtual_addr);
            if (status != VIP_SUCCESS) {
                PRINTK("get vipvirtual fail, not dump video memory allocate info.\n");
                return status;
            }
        }
        PRINTK("[%d, 0x%08x, 0x%08x, 0x%"PRIx64"]\n", index, virtual_addr, size, physical);
    }
    #endif

    return status;
}

static vip_status_e gckvip_dump_video_memory(
    gckvip_context_t *context)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t iter = 0, first_free_pos = 0;
    gckvip_database_table_t *table = VIP_NULL;
    gckvip_video_mem_handle_t *handle = VIP_NULL;

    status = gckvip_os_lock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("lock memory mutex faild, status = %d.\n", status);
        return status;
    }

    first_free_pos = gckvip_database_get_freepos(&context->videomem_database);

    #if vpmdENABLE_VIDEO_MEMORY_HEAP
    PRINTK("Video Memory Allocate Layout:\n");
    PRINTK("heap_virtual_base = 0x%08x, heap_size = 0x%x, heap_physical_base = 0x%" PRIx64", total_handle_count=%d\n",
            context->heap_virtual, context->heap_size, context->heap_physical, first_free_pos - 1);
    #else
    PRINTK("Video Memory Allocate Layout: total_handle_count = %d\n", first_free_pos - 1);
    #endif

    PRINTK("[index  virtual_addr  size  physical/physical_num]\n");
    for (iter = 0; iter < first_free_pos; iter++) {
        gckvip_database_get_table(&context->videomem_database, iter + DATABASE_MAGIC_DATA, &table);
        if (table->handle != VIP_NULL) {
            handle = (gckvip_video_mem_handle_t*)table->handle;
            dump_video_memory_layout(context, handle, iter);
        }
    }
    status = gckvip_os_unlock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("unlock memory mutex faild, status = %d.\n", status);
        return status;
    }

    return status;
}

vip_status_e gckvip_hang_dump(
    gckvip_device_t *device
    )
{
    gckvip_context_t *context = gckvip_get_context();

    PRINTK_E("start device%d hang dump dump_finish=%d\n", device->device_id, device->dump_finish);
    if (device->dump_finish == vip_false_e) {
        gckvip_dump_states(context, device);
        gckvip_dump_command(context, device);
    #if vpmdENABLE_WAIT_LINK_LOOP
        gckvip_dump_waitlink(device);
    #endif
    #if ENABLE_DUMP_MMU_MAPPING
        gckvip_dump_MMU_mapping();
    #endif

    #if ENABLE_DUMP_AHB_REGISTER
        gckvip_dump_AHB_register(device);
    #endif
        gckvip_dump_video_memory(context);
    #if vpmdREGISTER_NETWORK
        gckvip_dump_network_info(device);
    #endif
        device->dump_finish = vip_true_e;
    }

    return VIP_SUCCESS;
}
#endif

#if vpmdREGISTER_NETWORK && (!vpmdENABLE_CNN_PROFILING)
void gckvip_prepare_profiling(
    gckvip_submit_t *submit,
    gckvip_device_t *device
    )
{
    vip_uint32_t i = 0;
    gckvip_hardware_t *hardware;
    /* reset memory counter when vip idle */
    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        gckvip_write_register(hardware, 0x0003C, 1);
        gckvip_write_register(hardware, 0x0003C, 0);
    }
}

vip_status_e gckvip_start_profiling(
    gckvip_submit_t *submit,
    gckvip_device_t *device
    )
{
    gckvip_context_t *context = gckvip_get_context();
    gckvip_hardware_t *hardware;
    vip_uint32_t i = 0;
    gckvip_segment_info_t *info = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;
    gckvip_hashmap_get_by_handle(&context->segment_info, submit->cmd_handle, &i, (void**)&info);
    if (0 < i) {
        gckvip_os_zero_memory((void*)&info->cur_data, sizeof(gckvip_profiling_data_t));
        info->cur_data.time = gckvip_os_get_time();
        gckvip_hashmap_unuse(&context->segment_info, i, vip_false_e);

        for (i = 0; i < device->hardware_count; i++) {
            hardware = &device->hardware[i];
            gckvip_write_register(hardware, 0x00438, 0);
            gckvip_write_register(hardware, 0x00078, 0);
            gckvip_write_register(hardware, 0x0007C, 0);
        }
    }
    else {
        PRINTK_D("failed to start profiling can't find handle=0x%"PRPx"\n", submit->cmd_handle);
    }

    return status;
}

vip_status_e gckvip_end_profiling(
    gckvip_context_t *context,
    gckvip_device_t *device,
    void *cmd_handle
    )
{
    vip_uint32_t index = 0, total_cycle = 0, total_idle_cycle = 0;
    gckvip_segment_info_t *info = VIP_NULL;
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    GCKVIP_CHECK_USER_PM_STATUS();
    gckvip_hashmap_get_by_handle(&context->segment_info, cmd_handle, &index, (void**)&info);
    if (0 < index) {
        vip_uint64_t cur_tmp_time = 0, avg_tmp_time = 0;
        vip_uint64_t time = gckvip_os_get_time();
        vip_uint32_t submit_count = info->submit_count < NUM_OF_AVG_TIME ? info->submit_count : NUM_OF_AVG_TIME;
        if (0 < submit_count) {
            /* multi-vip only need read core0 cycle count */
            hardware = &device->hardware[0];
            gckvip_read_register(hardware, 0x00078, &total_cycle);
            gckvip_read_register(hardware, 0x0007C, &total_idle_cycle);
            if (time > info->cur_data.time) {
                info->cur_data.time = time - info->cur_data.time;
            }
            else {
                vip_uint64_t tmp = ~0;
                info->cur_data.time = tmp - info->cur_data.time + time;
            }

            info->cur_data.total_cycle = total_cycle;
            info->cur_data.total_idle_cycle = total_idle_cycle;


            info->avg_data.total_cycle = info->avg_data.total_cycle +
                                         info->cur_data.total_cycle / submit_count -
                                         info->avg_data.total_cycle / submit_count;
            info->avg_data.total_idle_cycle = info->avg_data.total_idle_cycle +
                                              info->cur_data.total_idle_cycle / submit_count -
                                              info->avg_data.total_idle_cycle / submit_count;
            GCKVIP_DO_DIV64(info->cur_data.time, submit_count, cur_tmp_time);
            GCKVIP_DO_DIV64(info->avg_data.time, submit_count, avg_tmp_time);
            info->avg_data.time = info->avg_data.time + cur_tmp_time - avg_tmp_time;
        }
        gckvip_hashmap_unuse(&context->segment_info, index, vip_false_e);
    }
    else {
        PRINTK_D("end profiling can't find handle=0x%"PRPx"\n", cmd_handle);
    }

    return status;
}

vip_status_e gckvip_query_profiling(
    gckvip_context_t *context,
    gckvip_device_t *device,
    void *mem_handle,
    gckvip_query_profiling_t *profiling
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_segment_info_t* info = VIP_NULL;
    vip_uint32_t index = 0;

    gckvip_hashmap_get_by_handle(&context->segment_info, mem_handle, &index, (void**)&info);
    if (index > 0) {
        profiling->total_cycle = info->cur_data.total_cycle;
        profiling->total_idle_cycle = info->cur_data.total_idle_cycle;
        profiling->inference_time = (vip_uint32_t)info->cur_data.time;
        PRINTK_D("handle=0x%"PRPx", inference_time=%dus, total_cycle=%d, idle_cycle=%d\n",
                 mem_handle, profiling->inference_time, profiling->total_cycle,
                 profiling->total_idle_cycle);
        gckvip_hashmap_unuse(&context->segment_info, index, vip_false_e);
    }
    else {
        PRINTK_D("can't find this handle=0x%"PRPx" in profiling queue\n", mem_handle);
        profiling->inference_time = 0;
        profiling->total_cycle = 0;
        profiling->total_idle_cycle = 0;
    }

    return status;
}
#endif

#if (vpmdENABLE_DEBUG_LOG > 3) || vpmdENABLE_DEBUGFS
static vip_status_e check_clock_begin(
    gckvip_hardware_t *hardware,
    vip_uint64_t *start_MC,
    vip_uint64_t *start_SH
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t mc_start, sh_start;

    mc_start = gckvip_os_get_time();
    gckvip_write_register(hardware, 0x00438, 0);
    *start_MC = mc_start;

    gckvip_write_register(hardware, 0x00470, 0xFF << 24);
    sh_start = gckvip_os_get_time();
    gckvip_write_register(hardware, 0x00470, 0x4 << 24);
    *start_SH = sh_start;

    return status;
}

static vip_status_e check_clock_end(
    gckvip_hardware_t *hardware,
    vip_uint64_t *end_MC,
    vip_uint64_t *end_SH,
    vip_uint64_t *cycles_MC,
    vip_uint64_t *cycles_SH
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t mc_end = 0, sh_end = 0;
    vip_uint32_t mc_cycles = 0, sh_cycles = 0;

    mc_end = gckvip_os_get_time();
    gckvip_read_register(hardware, 0x00438, &mc_cycles);
    if (mc_cycles == 0) {
        status = VIP_ERROR_IO;
        return status;
    }

    sh_end = gckvip_os_get_time();
    gckvip_read_register(hardware, 0x0045C, &sh_cycles);
    if (sh_cycles == 0) {
        sh_cycles = mc_cycles;
    }

    *end_MC = mc_end;
    *end_SH = sh_end;
    *cycles_MC = (vip_uint64_t)mc_cycles;
    *cycles_SH = (vip_uint64_t)sh_cycles;

    return status;
}

/*
@ brief
    get frequency of MC and SH.
*/
vip_status_e gckvip_get_clock(
    gckvip_hardware_t *hardware,
    vip_uint64_t *clk_MC,
    vip_uint64_t *clk_SH
    )
{
    vip_uint64_t mc_start = 0, sh_start = 0, mc_end = 0, sh_end = 0;
    vip_uint64_t mc_cycles = 0, sh_cycles = 0;
    vip_uint64_t mc_freq = 0, sh_freq = 0;
    vip_uint32_t data = 1000000U << 12;
    vip_uint32_t diff = 0, div = 0;
    vip_status_e status = VIP_SUCCESS;

    status = check_clock_begin(hardware, &mc_start, &sh_start);
    if (status != VIP_SUCCESS) {
        return status;
    }

    gckvip_os_delay(50);

    status = check_clock_end(hardware, &mc_end, &sh_end, &mc_cycles, &sh_cycles);
    if (status != VIP_SUCCESS) {
        return status;
    }

    diff = (vip_uint32_t)(mc_end - mc_start);
    div = data / diff;
    mc_freq = (mc_cycles * (vip_uint64_t)div) >> 12;
    *clk_MC = mc_freq;

    diff = (vip_uint32_t)(sh_end - sh_start);
    div = data / diff;
    sh_freq = (sh_cycles * (vip_uint64_t)div) >> 12;
    *clk_SH = sh_freq;

    return status;
}

/*
@brief report master clock and PPU clock
*/
void gckvip_report_clock(void)
{
    vip_uint64_t mc_freq = 0, sh_freq = 0;
    gckvip_context_t *context = gckvip_get_context();
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint32_t j = 0;

    PRINTK_I("VIP Frequency:\n");
    for (i = 0; i < context->device_count; i++) {
        gckvip_device_t *device = &context->device[i];
        #if vpmdENABLE_CLOCK_SUSPEND
        gckvip_os_inc_atomic(device->disable_clock_suspend);
        gckvip_pm_enable_clock(device);
        #endif
        gckvip_pm_set_frequency(device, 100);
        for (j = 0; j < device->hardware_count; j++) {
            hardware = &device->hardware[j];
            if (VIP_SUCCESS == gckvip_get_clock(hardware, &mc_freq, &sh_freq)) {
                PRINTK_I("core[%d]   Core Frequency=%" PRId64" HZ\n", hardware->core_id, mc_freq);
                PRINTK_I("core[%d]   PPU  Frequency=%" PRId64" HZ\n", hardware->core_id, sh_freq);
            }
            else {
                PRINTK_I("unable to get VIP frequency.\n");
            }
        }
        #if vpmdENABLE_CLOCK_SUSPEND
        gckvip_os_dec_atomic(device->disable_clock_suspend);
        #endif
    }
}

vip_status_e gckvip_dump_options(void)
{
    PRINTK_I("VIPLite driver software version %d.%d.%d.%s\n",
                VERSION_MAJOR, VERSION_MINOR, VERSION_SUB_MINOR, VERSION_PATCH);
    PRINTK_I("vpmdENABLE_RECOVERY=%d\n", vpmdENABLE_RECOVERY);
    PRINTK_I("vpmdHARDWARE_TIMEOUT=%d\n", vpmdHARDWARE_TIMEOUT);
    PRINTK_I("vpmdENABLE_HANG_DUMP=%d\n", vpmdENABLE_HANG_DUMP);
    PRINTK_I("vpmdENABLE_USE_AGENT_TRIGGER=%d\n", vpmdENABLE_USE_AGENT_TRIGGER);
    PRINTK_I("vpmdENABLE_CAPTURE=%d\n", vpmdENABLE_CAPTURE);
    PRINTK_I("vpmdENABLE_CAPTURE_IN_KERNEL=%d\n", vpmdENABLE_CAPTURE_IN_KERNEL);
    PRINTK_I("vpmdCREATE_NETWORK_FROM_FLASH=%d\n", vpmdCREATE_NETWORK_FROM_FLASH);
    PRINTK_I("vpmdENABLE_WAIT_LINK_LOOP=%d\n", vpmdENABLE_WAIT_LINK_LOOP);
    PRINTK_I("vpmdENABLE_FLUSH_CPU_CACHE=%d, LINE_SIZE=%d\n",
                vpmdENABLE_FLUSH_CPU_CACHE, vpmdCPU_CACHE_LINE_SIZE);
    PRINTK_I("vpmdENABLE_VIDEO_MEMORY_CACHE=%d\n", vpmdENABLE_VIDEO_MEMORY_CACHE);
    PRINTK_I("vpmdENABLE_LAYER_DUMP=%d\n", vpmdENABLE_LAYER_DUMP);
    PRINTK_I("vpmdENABLE_CNN_PROFILING=%d\n", vpmdENABLE_CNN_PROFILING);
    PRINTK_I("vpmdENABLE_BW_PROFILING=%d\n", vpmdENABLE_BW_PROFILING);
    PRINTK_I("vpmdENABLE_MEMORY_PROFILING=%d\n", vpmdENABLE_MEMORY_PROFILING);
    PRINTK_I("vpmdENABLE_DEBUG_LOG=%d\n", vpmdENABLE_DEBUG_LOG);
    PRINTK_I("vpmdENABLE_SUPPORT_CPU_LAYER=%d\n", vpmdENABLE_SUPPORT_CPU_LAYER);
    PRINTK_I("vpmdENABLE_SYS_MEMORY_HEAP=%d\n", vpmdENABLE_SYS_MEMORY_HEAP);
    PRINTK_I("vpmdENABLE_VIDEO_MEMORY_HEAP=%d\n", vpmdENABLE_VIDEO_MEMORY_HEAP);
    PRINTK_I("vpmdENABLE_MMU=%d\n", vpmdENABLE_MMU);
#if vpmdENABLE_MMU && vpmdENABLE_VIDEO_MEMORY_HEAP
    PRINTK_I("VIDEO_MEMORY_HEAP_READ_ONLY=%d\n", VIDEO_MEMORY_HEAP_READ_ONLY);
#endif
    PRINTK_I("vpmdPOWER_OFF_TIMEOUT=%d\n", vpmdPOWER_OFF_TIMEOUT);
    PRINTK_I("vpmdENABLE_SUSPEND_RESUME=%d\n", vpmdENABLE_SUSPEND_RESUME);
    PRINTK_I("vpmdENABLE_USER_CONTROL_POWER=%d\n", vpmdENABLE_USER_CONTROL_POWER);
    PRINTK_I("vpmdENABLE_CANCELATION=%d\n", vpmdENABLE_CANCELATION);
    PRINTK_I("vpmdENABLE_DEBUGFS=%d\n", vpmdENABLE_DEBUGFS);
    PRINTK_I("vpmdENABLE_CREATE_BUF_FD=%d\n", vpmdENABLE_CREATE_BUF_FD);
    PRINTK_I("vpmdENABLE_GROUP_MODE=%d\n", vpmdENABLE_GROUP_MODE);
    PRINTK_I("vpmdENABLE_NODE=%d\n", vpmdENABLE_NODE);
    PRINTK_I("vpmdENABLE_CHANGE_PPU_PARAM=%d\n", vpmdENABLE_CHANGE_PPU_PARAM);
    PRINTK_I("vpmdENABLE_FUNC_TRACE=%d\n", vpmdENABLE_FUNC_TRACE);
    PRINTK_I("vpmdENABLE_REDUCE_MEMORY=%d\n", vpmdENABLE_REDUCE_MEMORY);
    PRINTK_I("vpmdENABLE_RESERVE_PHYSICAL=%d\n", vpmdENABLE_RESERVE_PHYSICAL);
    PRINTK_I("vpmdENABLE_POWER_MANAGEMENT=%d\n", vpmdENABLE_POWER_MANAGEMENT);
    PRINTK_I("vpmdENABLE_CLOCK_SUSPEND=%d\n", vpmdENABLE_CLOCK_SUSPEND);
    PRINTK_I("vpmdDUMP_NBG_RESOURCE=%d\n", vpmdDUMP_NBG_RESOURCE);
    PRINTK_I("vpmdENABLE_POLLING=%d\n", vpmdENABLE_POLLING);

    return VIP_SUCCESS;
}
#endif

vip_status_e gckvip_check_irq_value(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    gckvip_hardware_t *hardware = &device->hardware[0];
    vip_uint32_t irq_value= *hardware->irq_value;
    vip_uint32_t try_count = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_bool_e wait_done = vip_false_e;

    while (vip_false_e == wait_done) {
        PRINTK_D("irq check try count=%d, wait irq value=0x%x\n", try_count, irq_value);
        if (irq_value & 0x80000000) {
            PRINTK_I("AXI BUS ERROR...\n");
        }
        else if (irq_value & 0x40000000) {
            vip_uint32_t mmu_status = 0;
            gckvip_read_register(hardware, 0x00388, &mmu_status);
            if (mmu_status & 0x01) {
                PRINTK_I("MMU exception...\n");
            }
            else {
                PRINTK_I("MMU exception, then MMU is disabled\n");
            }
        }
        else if (irq_value & 0x10000000) {
            status = VIP_ERROR_CANCELED;
            PRINTK_I("Execute canceled, irq_value = 0x%x.\n", irq_value);
            wait_done = vip_true_e;
        }
        else {
            wait_done = vip_true_e;
        }

        if (vip_false_e == wait_done) {
            if (try_count > MAX_RECOVERY_TIMES) {
                PRINTK_E("fail to wait hardware irq value=0x%x.\n", irq_value);
                #if vpmdENABLE_HANG_DUMP && vpmdENABLE_MMU && (vpmdENABLE_DEBUG_LOG > 2)
                if (irq_value & 0x40000000) {
                    gckvip_dump_mmu_info(context, hardware);
                }
                #endif
                gcGoOnError(VIP_ERROR_TIMEOUT);
            }

            status = gckvip_os_wait_interrupt(hardware->irq_queue, hardware->irq_value,
                                              vpmdHARDWARE_TIMEOUT, 0xFFFFFFFF);
            if (VIP_ERROR_TIMEOUT == status) {
                status = gcvip_cmd_timeout_handle(context, device);
                gcGoOnError(status);
            }
            try_count++;
            irq_value= *hardware->irq_value;
        }
    }

onError:
    return status;
}

vip_status_e gckvip_register_dump_wrapper(
    vip_status_e(*func)(gckvip_hardware_t* hardware, ...),
    ...
    )
{
    va_list arg_list;
    vip_status_e status = VIP_SUCCESS;

    /* variables at most */
    gckvip_hardware_t* hardware;
    void* var2, * var3;
    va_start(arg_list, func);
    hardware = va_arg(arg_list, gckvip_hardware_t*);
    var2 = va_arg(arg_list, void*);
    var3 = va_arg(arg_list, void*);
    va_end(arg_list);

    if (func == (void*)gckvip_read_register) {
        vip_uint32_t address = GCKVIPPTR_TO_UINT32(var2);
        vip_uint32_t* data = (vip_uint32_t*)var3;

        status = gckvip_read_register(hardware, address, data);

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        gckvip_register_dump_action(GCKVIP_REGISTER_DUMP_READ, hardware->core_id, address, 0, *data);
        #endif
    }
    else if (func == (void*)gckvip_write_register) {
        vip_uint32_t address = GCKVIPPTR_TO_UINT32(var2);
        vip_uint32_t data = GCKVIPPTR_TO_UINT32(var3);

        status = gckvip_write_register(hardware, address, data);

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        gckvip_register_dump_action(GCKVIP_REGISTER_DUMP_WRITE, hardware->core_id, address, 0, data);
        #endif
    }
    else {
        PRINTK_E("unknown function to be wrapped in register dump\n");
        status = VIP_ERROR_NOT_SUPPORTED;
    }

    return status;
}

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
vip_status_e gckvip_register_dump_init(
    gckvip_context_t* context
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t flag = GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS |
                        GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL |
                        GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE |
                        GCVIP_VIDEO_MEM_ALLOC_MMU_PAGE_READ;
    phy_address_t physical_temp = 0;

    /* each vip core may have about 80 actions and each action need 2 U32 space */
    context->register_dump_buffer.size = context->core_count * sizeof(vip_uint32_t) * 80 * 2;
    status = gckvip_mem_allocate_videomemory(context, context->register_dump_buffer.size,
                                             &context->register_dump_buffer.handle,
                                             (void**)&context->register_dump_buffer.logical,
                                             &physical_temp,
                                             gcdVIP_MEMORY_ALIGN_SIZE,
                                             flag);
    /* cpu's physical address */
    context->register_dump_buffer.physical = physical_temp;
    context->register_dump_count = 0;
    PRINTK_D("init register capture done\n");

    return status;
}

vip_status_e gckvip_register_dump_uninit(
    gckvip_context_t* context
    )
{
    if (context->register_dump_buffer.handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, context->register_dump_buffer.handle);
        context->register_dump_buffer.handle = VIP_NULL;
    }

    return VIP_SUCCESS;
}

vip_status_e gckvip_register_dump_action(
    gckvip_register_dump_type_e type,
    vip_uint32_t core_id,
    vip_uint32_t address,
    vip_uint32_t expect,
    vip_uint32_t data
    )
{
    gckvip_context_t* context = gckvip_get_context();
    vip_uint32_t* logical = context->register_dump_buffer.logical;
    vip_uint32_t* count = &context->register_dump_count;

    if (context->register_dump_buffer.size > (*count + 3) * sizeof(vip_uint32_t)) {
        logical[(*count)++] = (address << DUMP_REGISTER_ADDRESS_START) |
            (core_id << DUMP_REGISTER_CORE_ID_START) |
            (type << DUMP_REGISTER_TYPE_START);
        logical[(*count)++] = data;
        if (GCKVIP_REGISTER_DUMP_WAIT == type) {
            logical[(*count)++] = expect;
        }
    }

    return VIP_SUCCESS;
}
#endif

#if vpmdREGISTER_NETWORK
vip_status_e gckvip_register_network_info(
    gckvip_register_network_t *reg
    )
{
    gckvip_context_t *context = gckvip_get_context();
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    gckvip_network_info_t *info = VIP_NULL;
    void *network_handle = VIP_NULL;

    if (0 == reg->segment_count) {
        PRINTK_E("network should have one segment at least\n");
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }
    status = gckvip_database_get_handle(&context->videomem_database, reg->network_handle, &network_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to convert network id, handle=0x%x, status=%d.\n", reg->network_handle, status);
        gcGoOnError(status);
    }
    if (VIP_NULL == network_handle) {
        PRINTK_E("network handle is NULL\n");
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

Expand:
    if (vip_true_e == gckvip_hashmap_full(&context->network_info)) {
        vip_bool_e expand = vip_false_e;
        status = gckvip_hashmap_expand(&context->network_info, &expand);
        if (VIP_SUCCESS != status) {
            PRINTK_E("network information hashmap is full\n");
            gcGoOnError(VIP_ERROR_FAILURE);
        }
    }
    status = gckvip_hashmap_insert(&context->network_info, network_handle, &i);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        goto Expand;
    }
    gckvip_hashmap_get_by_index(&context->network_info, i, (void**)&info);
    if (VIP_NULL == info) {
        PRINTK_E("fail to get ntework info by index=%d network 0x%"PRPx"\n", i, network_handle);
        gcGoOnError(VIP_ERROR_FAILURE);
    }
    gckvip_os_zero_memory((void*)info, sizeof(gckvip_network_info_t));
    info->segment_count = reg->segment_count;
    gckvip_os_snprint(info->network_name, NETWORK_NAME_SIZE, reg->network_name);
    status = gckvip_os_allocate_memory(sizeof(void*) * info->segment_count, (void**)&info->segment_handle);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to register network, allocate fail\n");
        gckvip_hashmap_remove(&context->network_info, network_handle);
        gckvip_hashmap_unuse(&context->network_info, i, vip_false_e);
        gcGoOnError(status);
    }
    gckvip_os_zero_memory(info->segment_handle, sizeof(void*) * info->segment_count);
    gckvip_hashmap_unuse(&context->network_info, i, vip_false_e);
    PRINTK_D("register network 0x%"PRPx"\n", network_handle);
onError:
    return status;
}

vip_status_e gckvip_register_segment_info(
    gckvip_register_segment_t *reg
    )
{
    gckvip_context_t *context = gckvip_get_context();
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    gckvip_segment_info_t *seg_info = VIP_NULL;
    gckvip_network_info_t *net_info = VIP_NULL;
    void *segment_handle = VIP_NULL;
    void *network_handle = VIP_NULL;
    status = gckvip_database_get_handle(&context->videomem_database, reg->network_handle, &network_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to convert network id, handle=0x%x, status=%d.\n", reg->network_handle, status);
        gcGoOnError(status);
    }
    if (VIP_NULL == network_handle) {
        PRINTK_E("network handle is NULL\n");
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    status = gckvip_database_get_handle(&context->videomem_database, reg->segment_handle, &segment_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to convert segment id, handle=0x%x, status=%d.\n", reg->segment_handle, status);
        gcGoOnError(status);
    }
    if (VIP_NULL == segment_handle) {
        PRINTK_E("segment handle is NULL\n");
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    gcOnError(gckvip_hashmap_get_by_handle(&context->network_info, network_handle, &i, (void**)&net_info));
    if (reg->segment_index >= net_info->segment_count) {
        gckvip_hashmap_unuse(&context->network_info, i, vip_false_e);
        PRINTK_E("segment index %d is out of range [0, %d)\n", reg->segment_index, net_info->segment_count);
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }
    net_info->segment_handle[reg->segment_index] = segment_handle;
    gckvip_hashmap_unuse(&context->network_info, i, vip_false_e);
Expand:
    if (vip_true_e == gckvip_hashmap_full(&context->segment_info)) {
        vip_bool_e expand = vip_false_e;
        status = gckvip_hashmap_expand(&context->segment_info, &expand);
        if (VIP_SUCCESS != status) {
            PRINTK_E("segment information hashmap is full\n");
            gcGoOnError(VIP_ERROR_FAILURE);
        }
    }
    status = gckvip_hashmap_insert(&context->segment_info, segment_handle, &i);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        goto Expand;
    }
    gckvip_hashmap_get_by_index(&context->segment_info, i, (void**)&seg_info);
    if (VIP_NULL == seg_info) {
        PRINTK_E("fail to get segment info by index=%d segment 0x%"PRPx" of network 0x%"PRPx"\n",
                 i, segment_handle, network_handle);
        gcGoOnError(VIP_ERROR_FAILURE);
    }
    gckvip_os_zero_memory((void*)seg_info, sizeof(gckvip_segment_info_t));
    seg_info->network_handle = network_handle;
    seg_info->operation_start = reg->operation_start;
    seg_info->operation_end = reg->operation_end;
    seg_info->segment_index = reg->segment_index;
    gckvip_hashmap_unuse(&context->segment_info, i, vip_false_e);
    PRINTK_D("register segment 0x%"PRPx" of network 0x%"PRPx"\n", segment_handle, network_handle);
onError:
    return status;
}

vip_status_e gckvip_unregister_network_info(
    gckvip_unregister_t *unreg
    )
{
    gckvip_context_t *context = gckvip_get_context();
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    gckvip_network_info_t *net_info = VIP_NULL;
    void *handle = VIP_NULL;
    gcOnError(gckvip_database_get_handle(&context->videomem_database, unreg->handle, &handle));
    gcOnError(gckvip_hashmap_get_by_handle(&context->network_info, handle, &i, (void**)&net_info));
    if (VIP_NULL != net_info->segment_handle) {
        gckvip_os_free_memory(net_info->segment_handle);
        net_info->segment_handle = VIP_NULL;
    }
    gckvip_hashmap_remove(&context->network_info, handle);
    gckvip_hashmap_unuse(&context->network_info, i, vip_false_e);
    PRINTK_D("unregister network 0x%"PRPx"\n", handle);
onError:
    return status;
}

vip_status_e gckvip_unregister_segment_info(
    gckvip_unregister_t *unreg
    )
{
    gckvip_context_t *context = gckvip_get_context();
    vip_status_e status = VIP_SUCCESS;
    void *handle = VIP_NULL;
    gcOnError(gckvip_database_get_handle(&context->videomem_database, unreg->handle, &handle));
    gcOnError(gckvip_hashmap_remove(&context->segment_info, handle));
    PRINTK_D("unregister segment 0x%"PRPx"\n", handle);
onError:
    return status;
}

/*
check the command has been skip by debugFS
*/
vip_bool_e gckvip_check_segment_skip(
    void *cmd_handle
    )
{
    gckvip_context_t* context = gckvip_get_context();
    gckvip_segment_info_t* info = VIP_NULL;
    vip_uint32_t i = 0;
    vip_bool_e skip = vip_false_e;
    if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->segment_info, cmd_handle, &i, (void**)&info)) {
        if (info->skip) {
            skip = vip_true_e;
        }
        gckvip_hashmap_unuse(&context->segment_info, i, vip_false_e);
    }
    return skip;
}

void gckvip_start_segment(
    gckvip_device_t *device,
    void *cmd_handle
    )
{
    gckvip_context_t* context = gckvip_get_context();
    gckvip_segment_info_t* info = VIP_NULL;
    vip_uint32_t i = 0;
    if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->segment_info, cmd_handle, &i, (void**)&info)) {
        info->submit_count++;
        gckvip_hashmap_unuse(&context->segment_info, i, vip_false_e);
    }
    /* array is full, overwrite oldest record */
    if ((device->pre_handle_end_index + 1) % PREVIOUS_NET_COUNT == device->pre_handle_start_index) {
        device->pre_handle_start_index = (device->pre_handle_start_index + 1) % PREVIOUS_NET_COUNT;
    }

    device->previous_handles[device->pre_handle_end_index] = cmd_handle;
    device->pre_handle_end_index = (device->pre_handle_end_index + 1) % PREVIOUS_NET_COUNT;
}

vip_status_e gckvip_dump_network_info(
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_context_t* context = gckvip_get_context();
    vip_uint32_t index = device->pre_handle_start_index;
    vip_uint32_t count = 0;
    gckvip_network_info_t* net_info = VIP_NULL;
    gckvip_segment_info_t* seg_info = VIP_NULL;
    vip_uint32_t i = 0;
    void *handle = VIP_NULL;
    #if vpmdENABLE_HANG_DUMP
    handle = device->curr_command.cmd_handle;
    #endif
    if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->segment_info, handle, &i, (void**)&seg_info)) {
        PRINTK("******************** dump hang inference network start ********************\n");
        if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->network_info,
                seg_info->network_handle, &i, (void**)&net_info)) {
            if (1 < net_info->segment_count) {
                PRINTK("[%02d] 0x%"PRPx" %s(%d) inference count:%d\n", count, seg_info->network_handle,
                    net_info->network_name, seg_info->segment_index, seg_info->submit_count);
            }
            else {
                PRINTK("[%02d] 0x%"PRPx" %s inference count:%d\n", count, seg_info->network_handle,
                    net_info->network_name, seg_info->submit_count);
            }
        }
        else {
            PRINTK("[%02d] 0x%"PRPx"\n", count, seg_info->network_handle);
        }
        PRINTK("******************** dump hang inference network end ********************\n");
    }
    PRINTK("******************** dump previous inference network start ********************\n");
    while (index != device->pre_handle_end_index) {
        handle = device->previous_handles[index];
        if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->segment_info, handle, &i, (void**)&seg_info)) {
            if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->network_info,
                seg_info->network_handle, &i, (void**)&net_info)) {
                if (1 < net_info->segment_count) {
                    PRINTK("[%02d] 0x%"PRPx" %s(%d) inference count:%d\n", count, seg_info->network_handle,
                        net_info->network_name, seg_info->segment_index, seg_info->submit_count);
                }
                else {
                    PRINTK("[%02d] 0x%"PRPx" %s inference count:%d\n", count, seg_info->network_handle,
                        net_info->network_name, seg_info->submit_count);
                }
            }
            else {
                PRINTK("[%02d] 0x%"PRPx"\n", count, seg_info->network_handle);
            }
        }
        else {
            PRINTK("[%02d] 0x%"PRPx"\n", count, handle);
        }

        index = (index + 1) % PREVIOUS_NET_COUNT;
        count++;
    }
    PRINTK("******************** dump previous inference network end **********************\n");
    return status;
}
#endif

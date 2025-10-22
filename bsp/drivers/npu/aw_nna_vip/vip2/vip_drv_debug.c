/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2024 Vivante Corporation
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
*    Copyright (C) 2017 - 2024 Vivante Corporation
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

#include <vip_drv_debug.h>
#include <vip_lite_version.h>
#include <vip_drv_task_common.h>
#include <vip_drv_mmu.h>
#include <vip_drv_mem_heap.h>
#include <vip_drv_device_driver.h>
#include <vip_drv_context.h>
#include <vip_drv_task_descriptor.h>
#include <vip_drv_task_debug.h>
#include <vip_drv_mem_allocator.h>


#define ENABLE_DUMP_AHB_REGISTER       0

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
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e error = VIP_SUCCESS;
    vip_int32_t i;

    do {
        vipdrv_read_register(hardware, 0x00660, state1);
        vipdrv_read_register(hardware, 0x00660, state1);
        vipdrv_read_register(hardware, 0x00664, address1);
        vipdrv_read_register(hardware, 0x00664, address1);
        for (i = 0; i < 500; i += 1) {
            vipdrv_read_register(hardware, 0x00660, state2);
            vipdrv_read_register(hardware, 0x00660, state2);
            vipdrv_read_register(hardware, 0x00664, address2);
            vipdrv_read_register(hardware, 0x00664, address2);
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
    vipdrv_hardware_t *hardware
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
        vipdrv_write_register(hardware, 0x00470, _feStacks[i].clear);

        for (j = 0; j < _feStacks[i].count; j++) {
            vipdrv_write_register(hardware, 0x00470, _feStacks[i].highSelect);

            vipdrv_read_register(hardware, 0x00450, &stack[j][0]);

            vipdrv_write_register(hardware, 0x00470, _feStacks[i].lowSelect);

            vipdrv_read_register(hardware, 0x00450, &stack[j][1]);

            vipdrv_write_register(hardware, 0x00470, _feStacks[i].next);

            if (_feStacks[i].linkSelect) {
                vipdrv_write_register(hardware, 0x00470, _feStacks[i].linkSelect);

                vipdrv_read_register(hardware, 0x00450, &link[j]);
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
    vipdrv_hardware_t *hardware,
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
    vipdrv_read_register(hardware, 0x0, &oldControl);

    for (pipe = 0; pipe < 4; pipe++) {
        if (!descriptor->avail) {
            continue;
        }
        if (!(descriptor->pipeMask & (1 << pipe))) {
            continue;
        }

        PRINTK("     %s[%d] debug registers:\n", descriptor->module, pipe);

        /* Switch pipe. */
        vipdrv_read_register(hardware, 0x0, &control);
        control &= ~(0xF << 20);
        control |= (pipe << 20);
        vipdrv_write_register(hardware, 0x0, control);

        for (i = 0; i < descriptor->count; i += REG_PER_LINE) {
            /* Select of first one in the group. */
            select = i + descriptor->selectStart;

            /* Read a group of registers. */
            for (j = 0; j < REG_PER_LINE; j++) {
                /* Shift select to right position. */
                vipdrv_write_register(hardware, descriptor->index, (select + j) << descriptor->shift);
                vipdrv_read_register(hardware, descriptor->data, &datas[j]);
            }

            PRINTK("     [%02X] %08X %08X %08X %08X %08X %08X %08X %08X\n",
                            select, datas[0], datas[1], datas[2], datas[3], datas[4],
                            datas[5], datas[6], datas[7]);
        }
    }

    /* Restore control. */
    vipdrv_write_register(hardware, 0x0, oldControl);
    return status;
}

#if vpmdENABLE_MMU
#define _read_page_entry(page_entry)  *(vip_uint32_t*)(page_entry)

static vip_status_e vipdrv_show_exception(
    vipdrv_mmu_info_t* mmu_info,
    vip_virtual_t address,
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
    vip_physical_t stlb_physical = 0;
    vip_uint8_t page_type = 0;
    volatile vip_uint8_t *bitmap = VIP_NULL;
    vipdrv_MTLB_info_t *mtlb_info = VIP_NULL;

    mtlb = (address & vipdMMU_MTLB_MASK) >> vipdMMU_MTLB_SHIFT;
    if (mtlb >= mmu_info->MTLB_cnt) {
        return VIP_SUCCESS;
    }
    mtlb_info = &mmu_info->MTLB_info[mtlb];
    page_type = mtlb_info->type;

    if (VIP_SUCCESS != vipdrv_mmu_page_macro(page_type, VIP_NULL, VIP_NULL,
        &stlb_shift, &stlb_mask, &pgoff_mask, VIP_NULL)) {
        return VIP_SUCCESS;
    }

    stlb = (address & stlb_mask) >> stlb_shift;
    offset =  address & pgoff_mask;

    PRINTK("  MMU%d: exception address = 0x%08X\n", index, address);
    PRINTK("    page type = %s\n", vip_drv_mmu_page_str(page_type));
    PRINTK("    MTLB entry index = %d\n", mtlb);
    PRINTK("    STLB entry index = %d\n", stlb);
    PRINTK("    Offset = 0x%08X\n", offset);

    stlb_logical = (vip_uint32_t*)mtlb_info->STLB->logical;
    stlb_physical = mtlb_info->STLB->physical;
    bitmap = mtlb_info->bitmap;

    PRINTK("    Page table entry bitmap = 0x%X\n", \
        (bitmap[stlb / (8 * sizeof(vip_uint8_t))] & \
        (1 << (stlb % (8 * sizeof(vip_uint8_t))))) >> (stlb % (8 * sizeof(vip_uint8_t))));
    PRINTK("    Page table entry address = 0x%016llX\n",
                    stlb_physical + stlb * sizeof(vip_uint32_t));
    PRINTK("    Page table entry value = 0x%08X\n",
                     _read_page_entry(stlb_logical + stlb));

    return VIP_SUCCESS;
}

static vip_status_e vipdrv_dump_tlb(
    vipdrv_mmu_info_t* mmu_info
    )
{
    vip_uint32_t i = 0;
    PRINTK("*****DUMP MMU Translation Table Start*****\n");
    PRINTK("*****DUMP MTLB******\n");
    vipdrv_dump_data(mmu_info->MTLB.logical, mmu_info->MTLB.physical, mmu_info->MTLB.size);

    PRINTK("*****DUMP STLB******\n");
    for (i = 0; i < mmu_info->MTLB_cnt; i++) {
        vipdrv_MTLB_info_t *MTLB_info = &mmu_info->MTLB_info[i];
        vip_uint32_t size = 0;
        if (VIPDRV_MMU_NONE_PAGE == MTLB_info->type) {
            continue;
        }
        PRINTK("STLB index %d type %s\n", i, vip_drv_mmu_page_str(MTLB_info->type));
        if (VIP_SUCCESS != vipdrv_mmu_page_macro(MTLB_info->type, &size, VIP_NULL,
            VIP_NULL, VIP_NULL, VIP_NULL, VIP_NULL)) {
            continue;
        }
        size *= sizeof(vip_uint32_t);
        vipdrv_dump_data((vip_uint32_t*)MTLB_info->STLB->logical, (vip_uint32_t)MTLB_info->STLB->physical, size);
    }

    PRINTK("*****DUMP MMU Translation Table End*****\n");
    return VIP_SUCCESS;
}

static vip_status_e vipdrv_dump_mmu_info(
    vipdrv_hardware_t *hardware
    )
{
    vipdrv_context_t *context = hardware->device->context;
    vipdrv_mmu_info_t* mmu_info = VIP_NULL;
    vip_uint32_t mmu_status = 0;
    vip_uint32_t mmu_temp = 0;
    vip_virtual_t address = 0;
    vip_uint32_t tmp = 0;
    vip_uint32_t mmu = 0;
    vip_uint32_t i = 0;
    vip_uint32_t axi_sram_size = 0;

    if (VIP_SUCCESS != vipdrv_mmu_page_get(hardware->cur_mmu_id, &mmu_info)) {
        PRINTK("Did not find mmu page=0x%llX\n", hardware->cur_mmu_id);
        return VIP_SUCCESS;
    }

    vipdrv_read_register(hardware, 0x00384, &mmu_status);
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

        vipdrv_read_register(hardware, 0x00380, &tmp);
        address = tmp;
        #if vpmdENABLE_40BITVA
        vipdrv_read_register(hardware, 0x003B8, &tmp);
        address |= (vip_virtual_t)(tmp & 0xff) << 32;
        #endif

        if ((address >= context->axi_sram_address) &&
            (address < (context->axi_sram_address + axi_sram_size))) {
            PRINTK("  MMU%d: exception address = 0x%X, mapped AXI SRAM.\n",  i, address);
        }
        else if ((address >= context->vip_sram_address) &&
                 (address < (context->vip_sram_address + context->vip_sram_size[0]))) {
            PRINTK("  MMU%d: exception address = 0x%X, mapped VIP SRAM.\n", i, address);
        }
        else {
            #if vpmdENABLE_VIDEO_MEMORY_HEAP
            vip_uint32_t heap_index = 0;
            vipdrv_alloc_mgt_heap_t* heap = VIP_NULL;
            vipdrv_alloc_mgt_t* cur = (vipdrv_alloc_mgt_t*)context->alloc_list_head;
            while (cur) {
                if (cur->allocator.alctor_type & VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP) {
                    heap = (vipdrv_alloc_mgt_heap_t*)cur;
                    if (address >= heap->virtual && address < (heap->virtual + heap->total_size)) {
                        PRINTK("  MMU%d: exception address = 0x%X, mapped video memory heap %d.\n",
                            i, address, heap_index);
                        break;
                    }
                }
                cur = (vipdrv_alloc_mgt_t*)cur->next;
            }
            if (VIP_NULL == cur)
            #endif
            {
                PRINTK("  MMU%d: exception address = 0x%X, "
                    "it is not mapped or dynamic memory.\n", i, address);
            }
        }

        vipdrv_show_exception(mmu_info, address, i);

        if (mmu_temp > 0) {
            if (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_MMU_PDMODE)) {
                PRINTK("*****DUMP MMU entry******\n");
                vipdrv_dump_data(context->MMU_entry.logical, context->MMU_entry.physical,
                    (context->MMU_entry.size > 64) ? 64 : context->MMU_entry.size);
            }
            vipdrv_dump_tlb(mmu_info);
        }
    }
    vipdrv_mmu_page_put(hardware->cur_mmu_id);
    return VIP_SUCCESS;
}
#endif

/*
    @ brief
    Dump debug register values. So far just for IDLE, FE, PE, and SH.
*/
vip_status_e vipdrv_dump_states(
    vipdrv_task_t *task
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t idleState = 0;
    vip_uint32_t dmaaddress1 = 0, dmaaddress2 = 0;
    vip_uint32_t dmastate1 = 0, dmastate2 = 0, dmaReqState = 0;
    vip_uint32_t cmdState = 0, cmdDmaState = 0, cmdFetState = 0, calState = 0;
    vip_uint32_t axiStatus = 0, dmaLo = 0, dmaHi = 0, veReqState = 0;
    vip_uint32_t control = 0, oldControl = 0, pipe = 0, mmuData = 0;
    vip_uint32_t ack_value = 0;
    vip_uint32_t hiControl = 0, clk_gate = 0;
    vipdrv_device_t *device = task->device;
    vipdrv_context_t *context = VIP_NULL;
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
    vip_uint32_t i = 0;
    vip_uint32_t n_modules = sizeof(module_name) / sizeof(const char*);

    if (VIP_NULL == device) {
        PRINTK_E("fail device object is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    context = device->context;
    PRINTK("   ***********************************\n");
    PRINTK("   ******** VIPLite hang dump ********\n");
    PRINTK("   chip ver1     = 0x%08X\n", context->chip_ver1);
    PRINTK("   chip ver2     = 0x%08X\n", context->chip_ver2);
    PRINTK("   chip date     = 0x%08X\n", context->chip_date);
    PRINTK("   chip cid      = 0x%08X\n", context->chip_cid);
    PRINTK("   VIPLite       = %d.%d.%d.%s\n",
                VERSION_MAJOR, VERSION_MINOR, VERSION_SUB_MINOR, VERSION_PATCH);
    PRINTK("   ***********************************\n");

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    PRINTK("*******Hardware index=%d, id=%d start*******\n", hw->core_index, hw->core_id);
    /* Verify whether DMA is running. */
    vipOnError(verify_dma(&dmaaddress1, &dmaaddress2, &dmastate1, &dmastate2, hw));

    cmdState    = dmastate2         & 0x1F;
    cmdDmaState = (dmastate2 >> 8)  & 0x03;
    cmdFetState = (dmastate2 >> 10) & 0x03;
    dmaReqState = (dmastate2 >> 12) & 0x03;
    calState    = (dmastate2 >> 14) & 0x03;
    veReqState  = (dmastate2 >> 16) & 0x03;

    /* Get debug value. */
    vipdrv_read_register(hw, 0x00004, &idleState);
    vipdrv_read_register(hw, 0x00000, &hiControl);
    vipdrv_read_register(hw, 0x0000C, &axiStatus);
    vipdrv_read_register(hw, 0x00668, &dmaLo);
    vipdrv_read_register(hw, 0x00668, &dmaLo);
    vipdrv_read_register(hw, 0x0066C, &dmaHi);
    vipdrv_read_register(hw, 0x0066C, &dmaHi);

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
    vipdrv_read_register(hw, 0x00010, &ack_value);
    PRINTK("   interrupt ACK value=0x%x\n", ack_value);
    /* wait task thread doesn't handle this irq if the 'irq flag' is not 0 */
    PRINTK("   irq value=0x%x, error irq flag=0x%x\n", *hw->irq_value | hw->irq_local_value,
            (*hw->irq_value | hw->irq_local_value) & 0xFFFF0000);

    /* Record control. */
    vipdrv_read_register(hw, 0x0, &oldControl);

    /* Switch pipe. */
    vipdrv_read_register(hw, 0x0, &control);
    control &= ~(0xF << 20);
    control |= (pipe << 20);
    vipdrv_write_register(hw, 0x0, control);

    PRINTK("   Debug registers:\n");
    /* disable NN/TP clock gating */
    vipdrv_read_register(hw, 0x00178, &clk_gate);
    clk_gate = SETBIT(clk_gate, 0, 1);
    clk_gate = SETBIT(clk_gate, 7, 1);
    vipdrv_write_register(hw, 0x00178, clk_gate);
    for (i = 0; i < sizeof(_dbgRegs) / sizeof(_dbgRegs[0]); i += 1) {
        _dump_debug_registers(hw, &_dbgRegs[i]);
    }
    clk_gate = SETBIT(clk_gate, 0, 0);
    clk_gate = SETBIT(clk_gate, 7, 0);
    vipdrv_write_register(hw, 0x00178, clk_gate);

    PRINTK("   other registers:\n");
    for (i = 0; i < (sizeof(_otherRegs) / sizeof((_otherRegs)[0])); i += 1) {
        vip_uint32_t read = 0;
        vipdrv_read_register(hw, _otherRegs[i].address, &read);
        PRINTK("      [0x%04X]=0x%08X %s\n", _otherRegs[i].address, read, _otherRegs[i].name);
    }

    /* dump MMU info */
    vipdrv_read_register(hw, 0x00388, &mmuData);
    if ((mmuData & 0x01) == 0x00) {
        PRINTK("   MMU has been disabled\n");
    }
    #if vpmdENABLE_MMU
    else {
        PRINTK("   MMU enabled\n");
        vipdrv_dump_mmu_info(hw);
    }
    #endif

    /* Restore control. */
    vipdrv_write_register(hw, 0x0, oldControl);

    PRINTK("   dump FE stack:\n");
    dump_fe_stack(hw);

    PRINTK("**************************\n");
    PRINTK("*****   SW COUNTERS  *****\n");
    PRINTK("**************************\n");
    PRINTK("    APP Submit Count = 0x%08X\n", hw->user_submit_count);
    PRINTK("    HW  Submit Count = 0x%08X\n", hw->hw_submit_count);

    PRINTK("*******Hardware index=%d, id=%d end*******\n", hw->core_index, hw->core_id);
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
onError:
    return status;
}

#if vpmdTASK_PARALLEL_ASYNC
vip_status_e vipdrv_dump_waitlink_table(
    vipdrv_task_t *task
    )
{
    vip_uint32_t i = 0, k = 0;
    char buffer[512] = {'\0'};
    vip_uint32_t left_size = 511;

    PRINTK("------dump wait link table start------\n");
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    vip_uint8_t* logical = (vip_uint8_t*)hw->wl_memory.logical;
    vip_uint32_t physical = (vip_uint32_t)hw->wl_memory.physical;
    if (0 == hw->wl_memory.mem_id) {
        PRINTK("core %d no wl table\n", hw->core_id);
    }
    PRINTK("core %d wl count=%d table:\n", hw->core_id, WL_TABLE_COUNT);
    PRINTK("current wl location: start=%d, end=%d\n", hw->wl_start_index, hw->wl_end_index);
    for (i = 0; i < WL_TABLE_COUNT; i++) {
        vip_uint32_t* data = (vip_uint32_t*)logical;
        char* p = buffer;
        left_size = 511;
        vipdrv_os_snprint(p, left_size, "%08X : ", physical);
        p += 11;
        left_size -= 11;
        for (k = 0; k < (WAIT_LINK_SIZE / sizeof(vip_uint32_t)); k++) {
            vipdrv_os_snprint(p, left_size, "%08X ", data[k]);
            p += 9;
            left_size -= 9;
        }
        vipdrv_os_snprint(p, left_size, "\n");
        PRINTK(buffer);
        logical += WAIT_LINK_SIZE;
        physical += WAIT_LINK_SIZE;
    }
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
    PRINTK("------dump wait link table end------\n");
    return VIP_SUCCESS;
}
#endif

vip_status_e vipdrv_dump_data(
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

#if ENABLE_DUMP_AHB_REGISTER
static vip_uint32_t vipdrv_read_reg(
    vipdrv_hardware_t *hardware,
    vip_uint32_t address
    )
{
    vip_uint32_t data = 0;
    vipdrv_read_register(hardware, address, &data);
    return data;
}

vip_status_e vipdrv_dump_AHB_register(
    vipdrv_task_t *task
    )
{
    vip_uint32_t reg_size = 0x800;
    vip_uint32_t line =  reg_size / 32;
    vip_uint32_t left =  reg_size % 32;
    vip_uint32_t physical = 0;
    vip_uint32_t i = 0;
    vipdrv_device_t *device = task->device;

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    PRINTK("**************************\n");
    PRINTK("   AHB REGISTER DUMP\n");
    PRINTK("**************************\n");
    PRINTK("core id=%d, AHB register size: %d bytes\n", hw->core_id, reg_size);

    for (i = 0; i < line; i++) {
        PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X %08X %08X %08X %08X\n",
                        physical, physical + 28, vipdrv_read_reg(hw, physical),
                        vipdrv_read_reg(hw, physical + 1*4),
                        vipdrv_read_reg(hw, physical + 2*4),
                        vipdrv_read_reg(hw, physical + 3*4),
                        vipdrv_read_reg(hw, physical + 4*4),
                        vipdrv_read_reg(hw, physical + 5*4),
                        vipdrv_read_reg(hw, physical + 6*4),
                        vipdrv_read_reg(hw, physical + 7*4));
        physical += 32;
    }

    switch(left) {
    case 28:
      PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X %08X %08X %08X\n",
                    physical, physical + 24, vipdrv_read_reg(hw, physical),
                    vipdrv_read_reg(hw, physical + 1*4),
                    vipdrv_read_reg(hw, physical + 2*4),
                    vipdrv_read_reg(hw, physical + 3*4),
                    vipdrv_read_reg(hw, physical + 4*4),
                    vipdrv_read_reg(hw, physical + 5*4),
                    vipdrv_read_reg(hw, physical + 6*4)
                    );
      break;
    case 24:
      PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X %08X %08X\n",
                    physical, physical + 20, vipdrv_read_reg(hw, physical),
                    vipdrv_read_reg(hw, physical + 1*4),
                    vipdrv_read_reg(hw, physical + 2*4),
                    vipdrv_read_reg(hw, physical + 3*4),
                    vipdrv_read_reg(hw, physical + 4*4),
                    vipdrv_read_reg(hw, physical + 5*4)
                    );
      break;
    case 20:
      PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X %08X\n",
                    physical, physical + 16, vipdrv_read_reg(hw, physical),
                    vipdrv_read_reg(hw, physical + 1*4),
                    vipdrv_read_reg(hw, physical + 2*4),
                    vipdrv_read_reg(hw, physical + 3*4),
                    vipdrv_read_reg(hw, physical + 4*4));
      break;
    case 16:
      PRINTK("address: %08X - %08X, value: %08X %08X %08X %08X\n",
                     physical, physical + 12, vipdrv_read_reg(hw, physical),
                     vipdrv_read_reg(hw, physical + 1*4),
                     vipdrv_read_reg(hw, physical + 2*4),
                     vipdrv_read_reg(hw, physical + 3*4));
      break;
    case 12:
      PRINTK("address: %08X - %08X, value: %08X %08X %08X\n",
                      physical, physical + 8, vipdrv_read_reg(hw, physical),
                      vipdrv_read_reg(hw, physical + 1*4),
                      vipdrv_read_reg(hw, physical + 2*4));
      break;
    case 8:
      PRINTK("address: %08X - %08X, value: %08X %08X\n",
                      physical, physical + 4, vipdrv_read_reg(hw, physical),
                      vipdrv_read_reg(hw, physical + 4)
                      );
      break;
    case 4:
      PRINTK("address: %08X, value: %08X\n", physical,
                       vipdrv_read_reg(hw, physical));
      break;
    default:
      break;
    }
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
    return VIP_SUCCESS;
}
#endif

vip_status_e vipdrv_dump_video_memory(
    vipdrv_task_t *task
    )
{
#define DATA_PER_LINE 4
#define SIZE_PER_MSG 30
#define BUFFER_SIZE 256
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vip_uint32_t phy_iter = 0;
    vipdrv_video_mem_handle_t *pointer = VIP_NULL;
    vipdrv_context_t *context = task->device->context;
    vip_int32_t left_size = BUFFER_SIZE - 1;
    char buffer[BUFFER_SIZE] = {'\0'};
    char* p = buffer;
    vipdrv_mem_control_block_t* mcb = VIP_NULL;

    if (0 == context->initialize) {
        PRINTK_E("vip not working, fail to dump mmu mapping.\n");
        return status;
    }

    PRINTK("\n**************************\n");
    PRINTK("dump video memory start...\n");
    #if vpmdENABLE_VIDEO_MEMORY_HEAP
    {
        vip_uint32_t heap_index = 0;
        vipdrv_alloc_mgt_heap_t* heap = VIP_NULL;
        vipdrv_alloc_mgt_t* cur = (vipdrv_alloc_mgt_t*)context->alloc_list_head;
        while (cur) {
            if (cur->allocator.alctor_type & VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP) {
                heap = (vipdrv_alloc_mgt_heap_t*)cur;
                PRINTK("heap %d: virtual_base = 0x%08x, size = 0x%x, physical_base = 0x%" PRIx64"\n",
                    heap_index, heap->virtual, heap->total_size, heap->physical);
                heap_index++;
            }
            cur = (vipdrv_alloc_mgt_t*)cur->next;
        }
    }
    #endif

    for (i = 1; i < vipdrv_database_free_pos(&context->mem_database); i++) {
        vipdrv_database_use(&context->mem_database, i, (void**)&mcb);
        if (VIP_NULL != mcb) {
            if (VIP_NULL != mcb->handle) {
                pointer = mcb->handle;
                PRINTK("@{0x%08X, 0x%010llX\n", pointer->vip_address, pointer->size);
                for (phy_iter = 0; phy_iter < pointer->physical_num; phy_iter++) {
                    vipdrv_os_snprint(p, left_size, "  [0x%010llX, 0x%010llX]",
                        pointer->physical_table[phy_iter],
                        pointer->size_table[phy_iter]);
                    p += SIZE_PER_MSG;
                    left_size -= SIZE_PER_MSG;
                    if ((pointer->physical_num - 1 == phy_iter) ||
                        (DATA_PER_LINE - 1 == phy_iter % DATA_PER_LINE)) {
                        vipdrv_os_snprint(p, left_size, "\n");
                        PRINTK(buffer);
                        p = buffer;
                        left_size = BUFFER_SIZE - 1;
                    }
                }
                PRINTK("} %s, pid=0x%x, flag=0x%x\n", pointer->allocator->name, pointer->pid, pointer->mem_flag);
            }
            vipdrv_database_unuse(&context->mem_database, i);
        }
    }
    PRINTK("dump video memory end...\n");
    PRINTK("**************************\n\n");

    return status;
}

vip_status_e vipdrv_hang_dump(
    vipdrv_task_t *task
    )
{
    vipdrv_dump_options();
    VIPDRV_SHOW_TASK_INFO_IMPL(task, "hang dump");
    vipdrv_dump_states(task);

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    vipdrv_dump_PC_value(hw);
    VIPDRV_LOOP_HARDWARE_IN_TASK_END

    vipdrv_task_desc_dump(task);
    vipdrv_task_tcb_dump(task);
#if vpmdTASK_DISPATCH_DEBUG
    vipdrv_task_dispatch_profile(task->device, VIPDRV_TASK_DIS_PROFILE_SHOW);
#endif

#if vpmdTASK_PARALLEL_ASYNC
    vipdrv_dump_waitlink_table(task);
#endif

#if ENABLE_DUMP_AHB_REGISTER
    vipdrv_dump_AHB_register(task);
#endif
    vipdrv_dump_video_memory(task);
    vipdrv_task_history_dump(task);

    return VIP_SUCCESS;
}

vip_status_e vipdrv_dump_PC_value(
    vipdrv_hardware_t *hardware
    )
{
    vip_uint32_t dmaAddr = 0xFFFFF, dmaLo = 0, dmaHi = 0;
    vipdrv_read_register(hardware, 0x00668, &dmaLo);
    vipdrv_read_register(hardware, 0x0066C, &dmaHi);
    vipdrv_read_register(hardware, 0x00664, &dmaAddr);
    PRINTK("dev%d hw%d DMA Address = 0x%08X\n", hardware->device->device_index,
           hardware->core_index, dmaAddr);
    PRINTK("   dmaLow   = 0x%08X\n", dmaLo);
    PRINTK("   dmaHigh  = 0x%08X\n", dmaHi);
    return VIP_SUCCESS;
}
#endif

#if (vpmdENABLE_DEBUG_LOG > 3) || vpmdENABLE_DEBUGFS
static vip_status_e check_clock_begin(
    vipdrv_hardware_t *hardware,
    vip_uint64_t *start_MC,
    vip_uint64_t *start_SH
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t mc_start, sh_start;

    mc_start = vipdrv_os_get_time();
    vipdrv_write_register(hardware, 0x00438, 0);
    *start_MC = mc_start;

    vipdrv_write_register(hardware, 0x00470, 0xFF << 24);
    sh_start = vipdrv_os_get_time();
    vipdrv_write_register(hardware, 0x00470, 0x4 << 24);
    *start_SH = sh_start;

    return status;
}

static vip_status_e check_clock_end(
    vipdrv_hardware_t *hardware,
    vip_uint64_t *end_MC,
    vip_uint64_t *end_SH,
    vip_uint64_t *cycles_MC,
    vip_uint64_t *cycles_SH
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t mc_end = 0, sh_end = 0;
    vip_uint32_t mc_cycles = 0, sh_cycles = 0;

    mc_end = vipdrv_os_get_time();
    vipdrv_read_register(hardware, 0x00438, &mc_cycles);
    if (mc_cycles == 0) {
        status = VIP_ERROR_IO;
        return status;
    }

    sh_end = vipdrv_os_get_time();
    vipdrv_read_register(hardware, 0x0045C, &sh_cycles);
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
vip_status_e vipdrv_get_clock(
    vipdrv_hardware_t *hardware,
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

    vipdrv_os_delay(50);

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
void vipdrv_report_clock(void)
{
    vip_uint64_t mc_freq = 0, sh_freq = 0;

    PRINTK_I("VIP Frequency:\n");
    VIPDRV_LOOP_DEVICE_START
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    #if vpmdENABLE_CLOCK_SUSPEND
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
    #endif
    #if vpmdENABLE_POWER_MANAGEMENT
    vipdrv_pm_set_frequency(hw, 100);
    #endif
    if (VIP_SUCCESS == vipdrv_get_clock(hw, &mc_freq, &sh_freq)) {
        PRINTK_I("dev%d hw%d Core Frequency=%" PRId64" HZ\n", device->device_index, hw->core_index, mc_freq);
        PRINTK_I("dev%d hw%d PPU  Frequency=%" PRId64" HZ\n", device->device_index,hw->core_index, sh_freq);
    }
    else {
        PRINTK_I("unable to get VIP frequency.\n");
    }
    #if vpmdENABLE_CLOCK_SUSPEND
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
    #endif
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    VIPDRV_LOOP_DEVICE_END
}
#endif

vip_status_e vipdrv_dump_options(void)
{
    PRINTK("VIPLite driver software version %d.%d.%d.%s\n",
                VERSION_MAJOR, VERSION_MINOR, VERSION_SUB_MINOR, VERSION_PATCH);
    PRINTK("vpmdENABLE_RECOVERY=%d\n", vpmdENABLE_RECOVERY);
    PRINTK("vpmdHARDWARE_TIMEOUT=%d\n", vpmdHARDWARE_TIMEOUT);
    PRINTK("vpmdENABLE_HANG_DUMP=%d\n", vpmdENABLE_HANG_DUMP);
    PRINTK("vpmdENABLE_CAPTURE=%d\n", vpmdENABLE_CAPTURE);
    PRINTK("vpmdENABLE_AHB_LOG=%d\n", vpmdENABLE_AHB_LOG);
    PRINTK("vpmdENABLE_FLUSH_CPU_CACHE=%d, LINE_SIZE=%d\n",
                vpmdENABLE_FLUSH_CPU_CACHE, vpmdCPU_CACHE_LINE_SIZE);
    PRINTK("vpmdENABLE_VIDEO_MEMORY_CACHE=%d\n", vpmdENABLE_VIDEO_MEMORY_CACHE);
    PRINTK("vpmdENABLE_LAYER_DUMP=%d\n", vpmdENABLE_LAYER_DUMP);
    PRINTK("vpmdENABLE_CNN_PROFILING=%d\n", vpmdENABLE_CNN_PROFILING);
    PRINTK("vpmdENABLE_BW_PROFILING=%d\n", vpmdENABLE_BW_PROFILING);
    PRINTK("vpmdENABLE_MEMORY_PROFILING=%d\n", vpmdENABLE_MEMORY_PROFILING);
    PRINTK("vpmdENABLE_DEBUG_LOG=%d\n", vpmdENABLE_DEBUG_LOG);
    PRINTK("vpmdENABLE_SUPPORT_CPU_LAYER=%d\n", vpmdENABLE_SUPPORT_CPU_LAYER);
    PRINTK("vpmdENABLE_SYS_MEMORY_HEAP=%d\n", vpmdENABLE_SYS_MEMORY_HEAP);
    PRINTK("vpmdENABLE_VIDEO_MEMORY_HEAP=%d\n", vpmdENABLE_VIDEO_MEMORY_HEAP);
    PRINTK("vpmdENABLE_MULTIPLE_TASK=%d, DISPATCH_DEBUG=%d\n", vpmdENABLE_MULTIPLE_TASK, vpmdTASK_DISPATCH_DEBUG);
    PRINTK("vpmdENABLE_MMU=%d\n", vpmdENABLE_MMU);
#if vpmdENABLE_MMU && vpmdENABLE_VIDEO_MEMORY_HEAP
    PRINTK("VIDEO_MEMORY_HEAP_READ_ONLY=%d\n", VIDEO_MEMORY_HEAP_READ_ONLY);
#endif
    PRINTK("vpmdPOWER_OFF_TIMEOUT=%d\n", vpmdPOWER_OFF_TIMEOUT);
    PRINTK("vpmdENABLE_SUSPEND_RESUME=%d\n", vpmdENABLE_SUSPEND_RESUME);
    PRINTK("vpmdENABLE_USER_CONTROL_POWER=%d\n", vpmdENABLE_USER_CONTROL_POWER);
    PRINTK("vpmdENABLE_CANCELATION=%d\n", vpmdENABLE_CANCELATION);
    PRINTK("vpmdENABLE_DEBUGFS=%d\n", vpmdENABLE_DEBUGFS);
    PRINTK("vpmdENABLE_CREATE_BUF_FD=%d\n", vpmdENABLE_CREATE_BUF_FD);
    PRINTK("vpmdENABLE_GROUP_MODE=%d\n", vpmdENABLE_GROUP_MODE);
    PRINTK("vpmdENABLE_NODE=%d\n", vpmdENABLE_NODE);
    PRINTK("vpmdENABLE_CHANGE_PPU_PARAM=%d\n", vpmdENABLE_CHANGE_PPU_PARAM);
    PRINTK("vpmdENABLE_FUNC_TRACE=%d\n", vpmdENABLE_FUNC_TRACE);
    PRINTK("vpmdENABLE_REDUCE_MEMORY=%d\n", vpmdENABLE_REDUCE_MEMORY);
    PRINTK("vpmdENABLE_RESERVE_PHYSICAL=%d\n", vpmdENABLE_RESERVE_PHYSICAL);
    PRINTK("vpmdENABLE_POWER_MANAGEMENT=%d\n", vpmdENABLE_POWER_MANAGEMENT);
    PRINTK("vpmdENABLE_CLOCK_SUSPEND=%d\n", vpmdENABLE_CLOCK_SUSPEND);
    PRINTK("vpmdDUMP_NBG_RESOURCE=%d\n", vpmdDUMP_NBG_RESOURCE);
    PRINTK("vpmdENABLE_POLLING=%d\n", vpmdENABLE_POLLING);
    PRINTK("vpmdPRELOAD_COEFF=%d\n", vpmdPRELOAD_COEFF);
    PRINTK("vpmdHARDWARE_BYPASS_MODE=%d\n", vpmdHARDWARE_BYPASS_MODE);
    PRINTK("vpmdENABLE_CHECKSUM=%d\n", vpmdENABLE_CHECKSUM);
    PRINTK("vpmdENABLE_APP_PROFILING=%d\n", vpmdENABLE_APP_PROFILING);
    PRINTK("vpmdENABLE_PREEMPTION=%d\n", vpmdENABLE_PREEMPTION);
    PRINTK("vpmdDYNAMIC_CHANGE_MEM_POOL=%d\n", vpmdDYNAMIC_CHANGE_MEM_POOL);
    PRINTK("vpmdENABLE_STALL_FE=%d\n", vpmdENABLE_STALL_FE);

    return VIP_SUCCESS;
}

vip_status_e vipdrv_register_dump_wrapper(
    vip_status_e(*func)(vipdrv_hardware_t* hardware, ...),
    ...
    )
{
    va_list arg_list;
    vip_status_e status = VIP_SUCCESS;

    /* variables at most */
    vipdrv_hardware_t* hardware;
    void* var2, * var3;
    va_start(arg_list, func);
    hardware = va_arg(arg_list, vipdrv_hardware_t*);
    var2 = va_arg(arg_list, void*);
    var3 = va_arg(arg_list, void*);
    va_end(arg_list);

    if (func == (void*)vipdrv_read_register) {
        vip_uint32_t address = VIPDRV_PTR_TO_UINT32(var2);
        vip_uint32_t* data = (vip_uint32_t*)var3;

        status = vipdrv_read_register(hardware, address, data);

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        vipdrv_register_dump_action(VIPDRV_REGISTER_DUMP_READ, hardware, address, 0, *data);
        #endif
    }
    else if (func == (void*)vipdrv_write_register) {
        vip_uint32_t address = VIPDRV_PTR_TO_UINT32(var2);
        vip_uint32_t data = VIPDRV_PTR_TO_UINT32(var3);

        status = vipdrv_write_register(hardware, address, data);

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        vipdrv_register_dump_action(VIPDRV_REGISTER_DUMP_WRITE, hardware, address, 0, data);
        #endif
    }
    else {
        PRINTK_E("unknown function to be wrapped in register dump\n");
        status = VIP_ERROR_NOT_SUPPORTED;
    }

    return status;
}

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
vip_status_e vipdrv_register_dump_init(
    vipdrv_context_t* context
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t flag = VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS |
                        VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL |
                        VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE;
    vip_physical_t physical_temp = 0;

    /* each vip core may have about 80 actions and each action need 2 U32 space */
    context->register_dump_buffer.size = context->core_count * sizeof(vip_uint32_t) * 80 * 2;
    status = vipdrv_mem_allocate_videomemory(context->register_dump_buffer.size,
                                             &context->register_dump_buffer.mem_id,
                                             (void**)&context->register_dump_buffer.logical,
                                             &physical_temp,
                                             vipdMEMORY_ALIGN_SIZE,
                                             flag, 0, 0, 0);
    /* cpu's physical address */
    context->register_dump_buffer.physical = physical_temp;
    context->register_dump_count = 0;

    PRINTK_D("init register capture done\n");

    return status;
}

vip_status_e vipdrv_register_dump_uninit(
    vipdrv_context_t* context
    )
{
    if (0 != context->register_dump_buffer.mem_id) {
        vipdrv_mem_free_videomemory(context->register_dump_buffer.mem_id);
        context->register_dump_buffer.mem_id = 0;
    }

    return VIP_SUCCESS;
}

vip_status_e vipdrv_register_dump_action(
    vipdrv_register_dump_type_e type,
    vipdrv_hardware_t* hardware,
    vip_uint32_t address,
    vip_uint32_t expect,
    vip_uint32_t data
    )
{
    vipdrv_context_t* context = hardware->device->context;
    vip_uint32_t core_id = hardware->core_id;
    vip_uint32_t* logical = context->register_dump_buffer.logical;
    vip_uint32_t* count = &context->register_dump_count;

    if (context->register_dump_buffer.size > (*count + 3) * sizeof(vip_uint32_t)) {
        logical[(*count)++] = (address << DUMP_REGISTER_ADDRESS_START) |
            (core_id << DUMP_REGISTER_CORE_ID_START) |
            (type << DUMP_REGISTER_TYPE_START);
        logical[(*count)++] = data;
        if (VIPDRV_REGISTER_DUMP_WAIT == type) {
            logical[(*count)++] = expect;
        }
    }

    return VIP_SUCCESS;
}
#endif


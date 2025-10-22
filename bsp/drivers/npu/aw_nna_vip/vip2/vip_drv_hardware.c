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

#include <vip_drv_hardware.h>
#include <vip_drv_device_driver.h>
#include <vip_drv_context.h>
#include <vip_drv_debug.h>
#include <vip_drv_mmu.h>

/*
@brief list all support bypass reorder.
*/
vip_bool_e vipdrv_hw_bypass_reorder(void)
{
    vip_bool_e support = vip_false_e;
    vipdrv_context_t *context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vip_uint32_t chip_cid = context->chip_cid;

    if (0x1000000A == chip_cid) {
        return vip_true_e;
    }
    if (0xDC != chip_cid) {
        vip_uint32_t i = 0;
        vip_uint32_t axi_sram_total = 0;
        for (i = 0; i < context->device_count; i++) {
            axi_sram_total += context->axi_sram_size[i];
        }
        if (0 == axi_sram_total) {
            support = vip_true_e;
        }
    }
    return support;
}

/*
@brief list all devices for don't need software reset.
*/
static vip_bool_e vipdrv_hw_bypass_reset(void)
{
    vip_uint32_t *chip_cid = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_CID);
    vip_bool_e need = vip_true_e;

    if (0xA4 == *chip_cid) {
        need = vip_false_e;
    }

    return need;
}

/*
@brief software resrt hardware.
*/
static vip_status_e reset_vip(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t control = 0, idle = 0;
    vip_uint32_t count = 0;
    vip_uint32_t try_count = 0;
    vip_uint32_t value = 0;

    vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware,
        0x00000, 0x00070900));
    while ((count < 2) && (try_count < 10)) {
        /* Disable clock gating. */
        vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware,
                    0x00104, 0x00000000));

        /* Disable pulse-eater and set clock */
        control = SETBIT(0x01590880, 16, 0);
        control = SETBIT(control, 17, 1);
        control = SETBIT(control, 0, 1);
        vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x0010C, control));
        control = SETBIT(control, 0, 0);
        vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x0010C, control));

        /* FSCALE_CMD_LOAD */
        vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware,
                    0x00000, 0x00070900 | 0x200));
        vipdrv_os_udelay(5);
        vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware,
                    0x00000, 0x00070900));

        /* Wait for clock being stable. */
        #if vpmdFPGA_BUILD
        vipdrv_os_delay(10);
        #else
        vipdrv_os_udelay(20);
        #endif

        /* Isolate the VIP. */
        control = 0x00070900 | 0x80000;
        vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00000, control));

        vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x003A8, 0x1));
        /* Reset VIP, minimum 32 cycles request for slowest clock */
        #if vpmdFPGA_BUILD
        vipdrv_os_delay(5);
        #else
        vipdrv_os_udelay(20);
        #endif
        vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x003A8, 0x0));
        /* de-Reset VIP, minimum 128cycles request */
        #if vpmdFPGA_BUILD
        vipdrv_os_delay(10);
        #else
        vipdrv_os_udelay(40);
        #endif

        vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00000, SETBIT(control, 12, 0));
        /* Reset VIP isolation. */
        control &= ~0x80000;
        vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00000, control);

        vipdrv_os_udelay(50);
        /* Read idle register. */
        vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00004, &idle);
        if ((idle & 0x1) == 0) {    /* if either FE or MC is not idle, try again. */
            PRINTK_I("reset vip read idle=0x%08X\n", idle);
            try_count++;
            continue;
        }

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        vipdrv_register_dump_action(VIPDRV_REGISTER_DUMP_WAIT, hardware, 0x00004, 0x1, idle);
        #endif
        #if vpmdENABLE_AHB_LOG
        PRINTK("@[register.wait %u 0x%05X 0x%08X 0x%08X]\n", hardware->core_id, 0x00004, 0x1, idle);
        #endif

        /* Read reset register. */
        vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00000, &control);
        if ((control & 0x30000) != 0x30000) {
            PRINTK_I("reset vip control=0x%08X\n", control);
            try_count++;
            continue;
        }

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        vipdrv_register_dump_action(VIPDRV_REGISTER_DUMP_WAIT, hardware,
            0x00000, 0x30000, (control & 0x30000));
        #endif
        #if vpmdENABLE_AHB_LOG
        PRINTK("@[register.wait %u 0x%05X 0x%08X 0x%08X]\n", hardware->core_id, 0x00000, 0x30000, control);
        #endif

        vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00388, &control);
        if (control & 0x01) {
            PRINTK_I("mmu control=0x%08X\n", control);
            try_count++;
            continue;
        }

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        vipdrv_register_dump_action(VIPDRV_REGISTER_DUMP_WAIT, hardware,
            0x00388, 0x001, (control & 0x01));
        #endif
        #if vpmdENABLE_AHB_LOG
        PRINTK("@[register.wait %u 0x%05X 0x%08X 0x%08X]\n", hardware->core_id, 0x00388, 0x001, control);
        #endif

        count++;
    }

    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x0055C, 0x00FFFFFF);
    vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00090, &value);
    value &= 0xFFFFFFBF;
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00090, value);

    /* make sure all modules is idle after hw reset */
    vipdrv_read_register(hardware, 0x00004, &value);
    if(value != EXPECT_IDLE_VALUE_ALL_MODULE) {
        vip_uint32_t cid = 0;
        status = VIP_ERROR_FAILURE;
        vipdrv_read_register(hardware, 0x00030, &cid);
        PRINTK_E("fail to reset hardware, idleState=0x%08X, cid=0x%x\n", value, cid);
    }

    return status;
onError:
    PRINTK_E("fail to reset hardware%d\n", hardware->core_id);
    return status;
}

#if vpmdENABLE_MMU
/*
@brief register to set up MMU
*/
vip_status_e vipdrv_hw_pdmode_setup_mmu(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_mmu_info_t *mmu_info = VIP_NULL;
    vip_physical_t physical_tmp = 0;
    vip_uint32_t pd_entry_value = 0;
    status = vipdrv_mmu_page_get(hardware->cur_mmu_id, &mmu_info);
    if (VIP_SUCCESS != status) {
        PRINTK_E("Can not find mmu page when setup mmu by pdmode\n");
        return status;
    }

    physical_tmp = mmu_info->MTLB.physical >> 12;

#if vpmdENABLE_40BITVA
    pd_entry_value = ((physical_tmp << 4) | 0x0);
#else
    pd_entry_value = ((physical_tmp << 4) | 0x1);
#endif
    status = vipdrv_write_register(hardware, 0x003B4, pd_entry_value);
    if (status != VIP_SUCCESS) {
        PRINTK_E("write mmu pdentry register fail.\n");
        vipOnError(status);
    }

    status = vipdrv_write_register(hardware, 0x00388, 0x21);
    if (status != VIP_SUCCESS) {
        PRINTK_E("write mmu pdentry register bit[5] and bit[0] fail.\n");
        vipOnError(status);
    }

onError:
    vipdrv_mmu_page_put(hardware->cur_mmu_id);
    return status;
}

/*
@brief commmit a commands to setup MMU buffer mode.
*/
static vip_status_e mmu_commandmodel_setup(
    vipdrv_hardware_t *hardware
    )
{
    #define STATES_SIZE   8
    vip_status_e status = VIP_SUCCESS;
    vipdrv_video_memory_t* MMU_entry = &hardware->device->context->MMU_entry;
    vip_uint32_t mem_id = 0;
    vip_physical_t physical = 0;
    vip_uint32_t *logical = VIP_NULL;
    vip_uint32_t size = STATES_SIZE * sizeof(vip_uint32_t);
    vip_uint32_t buffer_size = size;
    vip_uint32_t idleState = 0;
    vip_int32_t try_count = 0;
    vip_uint32_t count = 0;
    vip_physical_t tmp = 0;
    vip_uint32_t page_table_l = 0;
    vip_uint32_t page_table_h = 0;

    vip_uint32_t alloc_flag = VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS |
                              VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR |
                              VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL |
                              VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE;
#if !vpmdENABLE_VIDEO_MEMORY_CACHE
    alloc_flag |= VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE;
#endif
    /* make 32-bits system happy */
    tmp = MMU_entry->physical >> 16;
    page_table_h = (vip_uint32_t)((tmp >> 16) & 0xFFFFFFFF);
    page_table_l = (vip_uint32_t)(MMU_entry->physical & 0xFFFFFFFF);

    vipdrv_write_register(hardware, 0x0038C, page_table_l);
    vipdrv_write_register(hardware, 0x00390, page_table_h);
    vipdrv_write_register(hardware, 0x00394, 1);

    buffer_size += APPEND_COMMAND_SIZE;
    buffer_size = VIP_ALIGN(buffer_size, vpmdCPU_CACHE_LINE_SIZE);
    vipOnError(vipdrv_mem_allocate_videomemory(buffer_size, &mem_id, (void**)&logical, &physical,
        vipdMEMORY_ALIGN_SIZE, alloc_flag, 0, (vip_uint64_t)1 << hardware->device->device_index, 0));
    if (physical & 0xFFFFFFFF00000000ULL) {
        PRINTK_E("MMU initialize buffer only support 32bit physical address\n");
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    logical[count++] = 0x0801006b; 
    logical[count++] = 0xfffe0000;
    logical[count++] = 0x08010e12;
    logical[count++] = 0x00490000;
    logical[count++] = 0x08010e02;
    logical[count++] = 0x00000701;
    logical[count++] = 0x48000000;
    logical[count++] = 0x00000701;
    /* Submit commands. */
    logical[count++] = (2 << 27);
    logical[count++] = 0;
    vipdrv_os_memorybarrier();

    size = count * sizeof(vip_uint32_t);
    PRINTK_D("mmu command size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64"\n",
              size, logical, physical);
    if (size > buffer_size) {
        PRINTK_E("fail out of memory buffer size=%d, size=%d\n", buffer_size, size);
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(mem_id, VIPDRV_CACHE_FLUSH);
#endif
    vipdrv_hw_trigger(hardware, (vip_uint32_t)physical);
    /* Wait until MMU configure finishes. */
    vipdrv_read_register(hardware, 0x00004, &idleState);
    while (((idleState & 0x01) != 0x01) && (try_count < 100)) {
        try_count++;
#if (defined(_WIN32) || defined (LINUXEMULATOR)) || vpmdFPGA_BUILD
        vipdrv_os_delay(10);
#else
        vipdrv_os_udelay(100);
#endif
        vipdrv_read_register(hardware, 0x00004, &idleState);
    }

    if (try_count >= 100) {
        #if vpmdENABLE_HANG_DUMP
        vipdrv_dump_PC_value(hardware);
        PRINTK("COMMAND BUF DUMP\n");
        vipdrv_dump_data(logical, physical, size);
        #endif
        PRINTK_E("fail to setup MMU, idleState=0x%x\n", idleState);
        vipGoOnError(VIP_ERROR_TIMEOUT);
    }

    PRINTK_D("mmu setup check vip idle status=0x8%x, try_count=%d\n", idleState, try_count);
#if vpmdFPGA_BUILD
    vipdrv_os_delay(20);
#else
    vipdrv_os_udelay(10);
#endif

    /* enable MMU */
    status = vipdrv_write_register(hardware, 0x00388, 0x01);
    if (status != VIP_SUCCESS) {
        PRINTK_E("mmu enable fail.\n");
        vipGoOnError(status);
    }

onError:
    if (0 != mem_id) {
        vipdrv_mem_free_videomemory(mem_id);
    }

    return status;
}

/*
@brief enable MMU.
*/
static vip_status_e vipdrv_hw_enable_mmu(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_bool_e support_pdmode = vip_false_e;

    support_pdmode = vipdrv_context_support(VIP_HW_FEATURE_MMU_PDMODE);

    if (vip_true_e == support_pdmode) {
        status = vipdrv_hw_pdmode_setup_mmu(hardware);
        if (status != VIP_SUCCESS) {
            PRINTK_E("core%d mmu enbale fail for pdmode\n", hardware->core_id);
            vipGoOnError(status);
        }
    }
    else {
        status = mmu_commandmodel_setup(hardware);
        if (status != VIP_SUCCESS) {
            PRINTK_E("core%d mmu enable fail for command mode\n", hardware->core_id);
            vipGoOnError(status);
        }
    }

    PRINTK_D("core%d mmu enable done mode=%s\n", hardware->core_id, support_pdmode ? "PD" : "CMD");

onError:
    return status;
}
#endif

/*
@brief initialzie vip hardware register.
*/
static vip_status_e init_vip(
    vipdrv_hardware_t *hardware
    )
{
    /* Limits the number of outstanding read requests, max value is 64. 0 is not limits */
#define MAX_OUTSTANDING_NUM     0
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t data = 0xcdcdcdcd;
    vip_uint32_t idle = 0;

    vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware,
                             0x00000, 0x00070900));

    data = SETBIT(0x00070900, 11, 0);
    vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware,
                             0x00000, data));
    data = SETBIT(0, 1, 1) | SETBIT(0, 11, 0);
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x003A8, data);

    /* Reset memory counters. */
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x0003C, ~0U);
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x0003C, 0);

    /* Enable model level clock gating and set gating counter */
    data = SETBIT(0x00140020, 0, 1);
    if (vipdrv_context_support(VIP_HW_FEATURE_SH_CONFORMANCE_BRUTEFORCE) == 0) {
        data = SETBIT(data, 10, 1);
    }
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00100, data);

    /* 2. enable MMU */
#if vpmdENABLE_MMU
    status = vipdrv_hw_enable_mmu(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to enable MMU, status=%d.\n", status);
        vipGoOnError(status);
    }
#endif

    /* Enable all model level clock gating */
    data = 0x00000000;
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00104, data);

    vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x0010C, &data);
    data = SETBIT(data, 16, 0);
    data = SETBIT(data, 17, 1);
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x0010C, data);

    if (vipdrv_hw_bypass_reorder()) {
        vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00090, &data);
        data |= SETBIT(data, 6, 1);
        vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00090, data);
    }

    vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00414, &data);
    data = SETBIT(data, 0, MAX_OUTSTANDING_NUM);
    vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00414, data);

    vipdrv_register_dump_wrapper((void*)vipdrv_read_register, hardware, 0x00004, &idle);
    PRINTK_D("dev%d hw%d after init idle=0x%08x\n", hardware->device->device_index, hardware->core_index, idle);

onError:
    return status;
}

/*
@brief initialzie interrupt of NPU hardware
enable interrupt
*/
#if !vpmdENABLE_POLLING
static vip_status_e init_interrupt(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    /* Enable all Interrupt EVENTs. */
    vipOnError(vipdrv_register_dump_wrapper((void*)vipdrv_write_register, hardware, 0x00014, 0xFFFFFFFF));

#if vpmdENABLE_FUSA
    status = vipdrv_fusa_init_interrupt(hardware);
#endif

onError:
    return status;
}
#endif

vip_uint32_t cmd_link(
    vip_uint32_t *command,
    vip_uint32_t address
    )
{
    command[1] = address;
    vipdrv_os_memorybarrier();
    command[0] = ((8 << 27) | (vipdMAX_CMD_SIZE >> 3));

    return LINK_SIZE;
}

#if vpmdTASK_PARALLEL_ASYNC || vpmdTASK_SCHEDULE
vip_uint32_t cmd_chip_enable(
    vip_uint32_t *command,
    vip_uint32_t core_id,
    vip_bool_e   is_enable_all
    )
{
    if (is_enable_all == vip_true_e) {
        command[0] = 0x6800FFFF;
        command[1] = 0;
    }
    else {
        command[0] = 0x68000000 | (1 << core_id);
        command[1] = 0;
    }

    return CHIP_ENABLE_SIZE;
}

vip_uint32_t cmd_event(
    vip_uint32_t *command,
    vip_uint32_t block,
    vip_uint32_t id
    )
{
    command[0] = ((1 << 27) | (1 << 16) | (0x0E01));
    command[1] = ((id & 0x0000001F) | (block));
    return EVENT_SIZE;
}
#endif

#if vpmdTASK_PARALLEL_ASYNC
vip_uint32_t cmd_wait(
    vip_uint32_t *command,
    vip_uint32_t time
    )
{
    if (0 == time) {
        /* nop cmd */
        command[0] = (3 << 27);
    }
    else {
        command[0] = ((7 << 27) | (time));
    }
    command[1] = 0;

    return WAIT_SIZE;
}

static vip_uint32_t cmd_wait_link(
    vip_uint32_t *command,
    vip_uint32_t time,
    vip_uint32_t address
    )
{
    command[0] = ((7 << 27) | (time));
    command[1] = 0;
    vipdrv_os_memorybarrier();
    command[3] = address;
    vipdrv_os_memorybarrier();
    command[2] = ((8 << 27) | (vipdMAX_CMD_SIZE >> 3));

    return WAIT_LINK_SIZE;
}

vip_uint32_t cmd_append_irq(
    vip_uint8_t* command,
    vip_uint32_t core_id,
    vip_uint32_t event_id
    )
{
    vip_uint8_t *logical = command;

    /* enable core id */
    logical += cmd_chip_enable((vip_uint32_t*)logical, core_id, vip_false_e);
    logical += cmd_event((vip_uint32_t*)logical, (1 << 6), event_id);
    /* enable all cores */
    logical += cmd_chip_enable((vip_uint32_t*)logical, 0, vip_true_e);

    return (logical - command);
}

/*
@ brief
    trigger hardware, NPU start to run.
    write wait/link physical base address
    to command control register for starting vip.
*/
vip_status_e vipdrv_hw_trigger_wl(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (0 == hardware->wl_memory.mem_id) {
        return status;
    }

    cmd_wait((vip_uint32_t*)(hardware->wl_table[hardware->wl_end_index].logical), WAIT_TIME_EXE);
    vipOnError(vipdrv_hw_trigger(hardware, hardware->wl_table[hardware->wl_start_index].physical));
    PRINTK_D("hardware %d wait-link, setup FE done..\n", hardware->core_id);
onError:
    return status;
}

vip_status_e vipdrv_hw_stop_wl(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t* logical = VIP_NULL;

    if (0 == hardware->wl_memory.mem_id) {
        return status;
    }

    logical = (vip_uint32_t*)(hardware->wl_table[hardware->wl_end_index].logical);
    /* End the "wait-link" loop */
    logical[0] = 0x10000000;
    logical[1] = 0x0;

    status = vipdrv_hw_wait_idle(hardware, 100, vip_true_e, EXPECT_IDLE_VALUE_ALL_MODULE);
    return status;
}

vip_status_e vipdrv_hw_wl_table_init(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vip_uint8_t* logical = hardware->wl_memory.logical;
    vip_uint32_t physical = hardware->wl_memory.physical;

    hardware->wl_memory.size = WL_TABLE_COUNT * WAIT_LINK_SIZE;
    status = vipdrv_mem_allocate_videomemory(hardware->wl_memory.size, &hardware->wl_memory.mem_id,
        (void**)&hardware->wl_memory.logical, &hardware->wl_memory.physical,
        vipdMEMORY_ALIGN_SIZE, VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL | VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE |
        VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR | VIPDRV_VIDEO_MEM_ALLOC_NPU_READ_ONLY,
        0, 1 << hardware->device->device_index, 0);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate video memory wait-link table, status=%d.\n", status);
        return status;
    }
    PRINTK_D("core%d wait-link table memory physical=0x%08x, size=0x%x, table_count=%d\n",
              hardware->core_id, hardware->wl_memory.physical, hardware->wl_memory.size, WL_TABLE_COUNT);

    logical = hardware->wl_memory.logical;
    physical = hardware->wl_memory.physical;
    for (i = 0; i < WL_TABLE_COUNT; i++) {
        hardware->wl_table[i].logical = logical;
        hardware->wl_table[i].physical = physical;
        hardware->wl_table[i].tcb_index = 0;
        cmd_wait_link((vip_uint32_t*)logical, WAIT_TIME_EXE, physical);
        logical += WAIT_LINK_SIZE;
        physical += WAIT_LINK_SIZE;
    }

    return status;
}

vip_status_e vipdrv_hw_wl_table_uninit(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (0 != hardware->wl_memory.mem_id) {
        vipdrv_mem_free_videomemory(hardware->wl_memory.mem_id);
        hardware->wl_memory.mem_id = 0;
    }

    return status;
}
#endif

/*
@brief
    do initialze hardware
*/
vip_status_e vipdrv_hw_init(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t mmu_status = 0;
    PRINTK_D("hardware initialize core%d..\n", hardware->core_id);

    if ((0x00 == hardware->device->context->chip_cid) ||
        (0x00 == hardware->device->context->chip_ver2)) {
        PRINTK_E("fail to check pid=0x%x\n", hardware->device->context->chip_cid);
        return VIP_ERROR_FAILURE;
    }

    /* 1. Init VIP. */
    status = init_vip(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to init core%d, status=%d.\n", hardware->core_id, status);
        vipGoOnError(status);
    }

    /* 2. irq init. */
#if !vpmdENABLE_POLLING
    status = init_interrupt(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to init irq core%d, status=%d.\n", hardware->core_id, status);
        vipGoOnError(status);
    }
#endif

    vipdrv_read_register(hardware, 0x00388, &mmu_status);
    if ((mmu_status & 0x01) == 0x00) {
        PRINTK_D("   MMU is disabled\n");
    }
    else {
        PRINTK_D("   MMU is enabled\n");
    }

    vipdrv_os_udelay(10);

    status = vipdrv_hw_submit_initcmd(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize core%d states, status=%d.\n", hardware->core_id, status);
        vipGoOnError(status);
    }
    PRINTK_D("hardware initialize done core=%d\n", hardware->core_id);

onError:
    return status;
}

/* shut down VIP hardware */
vip_status_e vipdrv_hw_destroy(void)
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdTASK_PARALLEL_ASYNC
    VIPDRV_LOOP_HARDWARE_START
    vipdrv_hw_wl_table_uninit(hw);
    VIPDRV_LOOP_HARDWARE_END
#endif

    return status;
}

/*
@brief To run any initial commands once in the beginning.
*/
vip_status_e vipdrv_hw_submit_initcmd(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t trigger_physical = 0;
    vip_uint32_t idleState = 0;
    vip_int32_t try_count = 0;
    vipdrv_device_t *device = hardware->device;

    if (VIP_NULL == hardware->device->init_command.mem_id) {
        PRINTK_E("init command memory is NULL\n");
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    PRINTK_D("submit initialize commands size=0x%x, mem_id=0x%x\n",
             device->init_command.size, device->init_command.mem_id);
    vipOnError(vipdrv_mmu_page_switch(hardware, 0));
    trigger_physical = device->init_command.physical;
#if vpmdENABLE_MMU
    if (vip_true_e == hardware->flush_mmu) {
#if vpmdTASK_PARALLEL_ASYNC
        vipdrv_mmu_flush_t* MMU_flush = &hardware->MMU_flush_table[0];
#else
        vipdrv_mmu_flush_t* MMU_flush = &hardware->MMU_flush;
#endif
        vip_uint8_t* flush_logical_end = MMU_flush->logical + FLUSH_MMU_STATES_SIZE;
        /* link flush mmu states and command buffer */
        cmd_link((vip_uint32_t*)flush_logical_end, trigger_physical);
        trigger_physical = MMU_flush->virtual;
        hardware->flush_mmu = vip_false_e;
    }
#endif
    vipOnError(vipdrv_hw_trigger(hardware, trigger_physical));

    try_count = 0;
    /* Wait until init cmd finishes. */
    vipdrv_read_register(hardware, 0x00004, &idleState);
    while (((idleState & 0x01) != 0x01) && (try_count < 100)) {
        try_count++;
        #if (defined(_WIN32) || defined (LINUXEMULATOR)) || vpmdFPGA_BUILD
        vipdrv_os_delay(10);
        #else
        vipdrv_os_udelay(100);
        #endif
        vipdrv_read_register(hardware, 0x00004, &idleState);
    }

    if (try_count >= 100) {
        #if vpmdENABLE_HANG_DUMP
        vipdrv_dump_PC_value(hardware);
        PRINTK("COMMAND BUF DUMP\n");
        vipdrv_dump_data((vip_uint32_t*)device->init_command.logical,
                        device->init_command.physical, device->init_command.size);
        #endif
        PRINTK_E("fail to submit init command idleState=0x%x\n", idleState);
        vipGoOnError(VIP_ERROR_TIMEOUT);
    }
    hardware->irq_local_value = 0;
    *hardware->irq_value &= IRQ_VALUE_FLAG;
#if !vpmdENABLE_POLLING
    vipdrv_os_set_atomic(*hardware->irq_count, 0);
#endif
    vipdrv_os_zero_memory(hardware->previous_task_id, sizeof(vip_uint32_t) * TASK_HISTORY_RECORD_COUNT);
    vipdrv_os_zero_memory(hardware->previous_subtask_index, sizeof(vip_uint32_t) * TASK_HISTORY_RECORD_COUNT);
    hardware->pre_start_index = 0;
    hardware->pre_end_index = 0;
    PRINTK_D("initialize commands done\n");
#if vpmdFPGA_BUILD
    vipdrv_os_delay(20);
#else
    vipdrv_os_udelay(10);
#endif

#if vpmdTASK_PARALLEL_ASYNC && (!vpmdENABLE_POWER_MANAGEMENT)
    vipOnError(vipdrv_hw_trigger_wl(hardware));
#endif

    return status;
onError:
    PRINTK_E("fail to submit initialize command\n");
    return status;
}

/*
@ brief
    Do a software HW reset.
*/
vip_status_e vipdrv_hw_reset(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_bool_e soft_reset = vipdrv_hw_bypass_reset();
    if (soft_reset == vip_false_e) {
        return status;
    }

    status |= reset_vip(hardware);

    return status;
}

vip_status_e vipdrv_hw_trigger(
    vipdrv_hardware_t *hardware,
    vip_uint32_t physical
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipOnError(vipdrv_write_register(hardware, 0x00654, physical));
    vipdrv_os_memorybarrier();
    vipOnError(vipdrv_write_register(hardware, 0x003A4, 0x00010000 | (vipdMAX_CMD_SIZE >> 3)));

onError:
    return status;
}

/*
@ brief query hardware status, runnign or idle.
*/
vip_status_e vipdrv_hw_idle(
    vipdrv_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (vip_false_e == vipdrv_os_get_atomic(hardware->idle)) {
        #if vpmdENABLE_POWER_MANAGEMENT
        status = vipdrv_pm_hw_idle(hardware);
        #else
        vipdrv_os_set_atomic(hardware->idle, vip_true_e);
        #endif
    }
    else {
        PRINTK_D("vip %d in idle status, don't need going to idle again\n", hardware->core_id);
    }

    return status;
}

vip_status_e vipdrv_hw_read_info(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t *device = vipdrv_get_device(0);
    vipdrv_hardware_t *hardware = VIP_NULL;
    vipdrv_context_t *context = VIP_NULL;

    vipIsNULL(device);
    context = device->context;
    hardware = vipdrv_get_hardware(device, 0);
    vipIsNULL(hardware);

    vipdrv_write_register(hardware, 0x00000, 0x00070900);
    vipdrv_read_register(hardware, 0x00020, &context->chip_ver1);
    vipdrv_read_register(hardware, 0x00024, &context->chip_ver2);
    vipdrv_read_register(hardware, 0x00030, &context->chip_cid);
    vipdrv_read_register(hardware, 0x00028, &context->chip_date);
    PRINTK_D("ver1=0x%04x, ver2=0x%04x, cid=0x%04x, date=0x%08x\n",
              context->chip_ver1, context->chip_ver2, context->chip_cid,
              context->chip_date);

onError:
    return status;
}

/*
@brief  read a hardware register.
@param  address, the address of hardware register.
@param  data, read data from register.
*/
vip_status_e vipdrv_read_register(
    vipdrv_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    if(VIP_NULL == hardware->reg_base) {
        PRINTK_E("core%d, fail to read register address=0x%05x, reg is NULL\n",
                    address, hardware->core_id);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
#if vpmdENABLE_MULTIPLE_TASK
    if (hardware->reg_mutex != VIP_NULL) {
        status = vipdrv_os_lock_mutex(hardware->reg_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to lock video register mutex write register\n");
            return VIP_ERROR_FAILURE;
        }
    }
#endif

    vipOnError(vipdrv_os_read_reg(hardware->reg_base, address, data));

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (hardware->reg_mutex != VIP_NULL) {
        if (VIP_SUCCESS != vipdrv_os_unlock_mutex(hardware->reg_mutex)) {
            PRINTK_E("fail to unlock video register mutex write register\n");
        }
    }
#endif
    return status;
}

/*
@brief  write a hardware register.
@param  address, the address of hardware register.
@param  data, write data to register.
*/
vip_status_e vipdrv_write_register_ext(
    vipdrv_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t data
    )
{
    vip_status_e status = VIP_SUCCESS;
    if(VIP_NULL == hardware->reg_base) {
        PRINTK_E("core%d, fail to write register address=0x%05x, reg is NULL\n",
                  address, hardware->core_id);
        return VIP_ERROR_OUT_OF_MEMORY;
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (hardware->reg_mutex != VIP_NULL) {
        status = vipdrv_os_lock_mutex(hardware->reg_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to lock video register mutex write register\n");
            return VIP_ERROR_FAILURE;
        }
    }
#endif

    vipOnError(vipdrv_os_write_reg(hardware->reg_base, address, data));

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (hardware->reg_mutex != VIP_NULL) {
        if (VIP_SUCCESS != vipdrv_os_unlock_mutex(hardware->reg_mutex)) {
            PRINTK_E("fail to unlock video register mutex write register\n");
        }
    }
#endif
    return status;
}

/*
@brief  write a hardware register.
@param  address, the address of hardware register.
@param  data, write data to register.
*/
vip_status_e vipdrv_write_register(
    vipdrv_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t data
    )
{
    vip_status_e status = VIP_SUCCESS;
    status = vipdrv_write_register_ext(hardware, address, data);
    return status;
}

/*
@brief waiting for hardware going to idle.
@param timeout the number of milliseconds this waiting.
@param is_fast true indicate that need udelay for poll hardware idle or not.
*/
vip_status_e vipdrv_hw_wait_idle(
    vipdrv_hardware_t *hardware,
    vip_uint32_t timeout,
    vip_uint8_t is_fast,
    vip_uint32_t expect_idle_value
    )
{
#define POLL_SPACE    5   /* 5ms */
#define POLL_SPACE_US 50 /* 50us */
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t count = 0;
    vip_uint32_t idle_reg = 0;
    vip_bool_e idle = vip_false_e;

    if (is_fast) {
        count = (vip_int32_t)(timeout * 1000) / POLL_SPACE_US;
    }
    else {
        count = (vip_int32_t)timeout / POLL_SPACE;
    }

    vipdrv_read_register(hardware, 0x00004, &idle_reg);
    idle = (idle_reg & expect_idle_value) == expect_idle_value ? vip_true_e : vip_false_e;
    while ((count >= 0) && (vip_false_e == idle)) {
        if (is_fast) {
            vipdrv_os_udelay(POLL_SPACE_US);
        }
        else {
            vipdrv_os_delay(POLL_SPACE);
        }
        vipdrv_read_register(hardware, 0x00004, &idle_reg);
        idle = (idle_reg & expect_idle_value) == expect_idle_value ? vip_true_e : vip_false_e;
        count--;
    }

    PRINTK_D("dev%d hw%d wait idle, wait time=%dms, fast=%d, reg=0x%x, is_idle=%s\n",
    hardware->device->device_index, hardware->core_index, timeout, is_fast, idle_reg, vip_true_e == idle ? "Y" : "N");

    if (vip_false_e == idle) {
        const char *module_name[] = {
            "FE",        "NULL",      "NULL",      "SH",        "NULL",
            "NULL",      "NULL",      "NULL",      "NULL",      "NULL",
            "NULL",      "NULL",      "NULL",      "FE BLT",    "MC",
            "NULL",      "NULL",      "NULL",      "NN",        "TP",
            "Scaler"
        };
        vip_uint32_t n_modules = sizeof(module_name) / sizeof(const char*);
        vip_uint32_t i = 0;

        for (i = 0; i < n_modules; i++) {
            if ((idle_reg & 0x01) == 0) {
                if (14 == i) {
                    if (hardware->device->context->chip_cid != 0xba) {
                        PRINTK_E("wait dev%d hw%d idle, %s not idle.\n",
                            hardware->device->device_index, hardware->core_index, module_name[i]);
                        status = VIP_ERROR_FAILURE;
                    }
                }
                else {
                    PRINTK_E("wait dev%d hw%d idle, %s not idle.\n",
                        hardware->device->device_index, hardware->core_index, module_name[i]);
                    status = VIP_ERROR_FAILURE;
                }
            }
            idle_reg >>= 1;
        }
    }

    return status;
#undef POLL_SPACE
#undef POLL_SPACE_US
}

#if vpmdENABLE_RECOVERY || vpmdENABLE_CANCELATION
vip_status_e vipdrv_hw_cancel(
    vipdrv_device_t *device,
    vip_uint8_t* cancel_mask
    )
{
#define CHECK_CANCEL_READY_COUNT    200
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t data = 0, orgi_data = 0;
    vip_uint32_t cancel_status = 0;
    vip_int32_t try_count = CHECK_CANCEL_READY_COUNT;
    vip_int32_t i = 0;

    /* 1. enable cancel */
    for (i = 0; i < (vip_int32_t)device->hardware_count; i++) {
        if (cancel_mask[i]) {
            vipdrv_hardware_t* hardware = &device->hardware[i];
            VIPDRV_CHECK_NULL(hardware);
            vipOnError(vipdrv_read_register(hardware, 0x003A8, &orgi_data));
            data = ((((vip_uint32_t) (orgi_data)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 3:3) - (0 ?
 3:3) + 1))) - 1)))))) << (0 ?
 3:3))) | (((vip_uint32_t) ((vip_uint32_t) (1) & ((vip_uint32_t) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 3:3) - (0 ? 3:3) + 1))) - 1)))))) << (0 ? 3:3)));
            vipOnError(vipdrv_write_register(hardware, 0x003A8, data));
        }
    }
#if vpmdFPGA_BUILD
    vipdrv_os_delay(10);
#else
    vipdrv_os_udelay(100);
#endif

    /* 2. release cancel */
    for (i = (vip_int32_t)device->hardware_count - 1; i >= 0 ; i--) {
        if (cancel_mask[i]) {
            vipdrv_hardware_t* hardware = &device->hardware[i];
            VIPDRV_CHECK_NULL(hardware);
            vipOnError(vipdrv_read_register(hardware, 0x003A8, &orgi_data));
            data = ((((vip_uint32_t) (orgi_data)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 3:3) - (0 ?
 3:3) + 1))) - 1)))))) << (0 ?
 3:3))) | (((vip_uint32_t) ((vip_uint32_t) (0) & ((vip_uint32_t) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 3:3) - (0 ? 3:3) + 1))) - 1)))))) << (0 ? 3:3)));
            vipOnError(vipdrv_write_register(hardware, 0x003A8, data));

            do {
                #if vpmdFPGA_BUILD
                vipdrv_os_delay(10);
                #else
                vipdrv_os_udelay(100);
                #endif
                vipOnError(vipdrv_read_register(hardware, 0x003A8, &data));
                data = (((((vip_uint32_t) (data)) >> (0 ? 4:4)) & ((vip_uint32_t) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~0 & ((1 << (((1 ? 4:4) - (0 ? 4:4) + 1))) - 1))))) );
                try_count--;
            } while ((!data) && (try_count > 0));

            if (0 == try_count) {
                PRINTK_E("fail wait core%d cancel done, CANCEL_COMPL=%d, JOB_STATUS=%d, check_count=%d\n",
                          i, cancel_status, data, try_count);
                vipGoOnError(VIP_ERROR_FAILURE);
            }
            else {
				PRINTK("hardware %d cancel done...\n", i);
            }
        }
    }
onError:
    return status;
#undef CHECK_CANCEL_READY_COUNT
}
#endif

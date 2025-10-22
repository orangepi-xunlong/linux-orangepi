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
#include <gc_vip_kernel_port.h>
#include <gc_vip_kernel_drv.h>
#include <gc_vip_video_memory.h>
#include <gc_vip_common.h>
#include <gc_vip_kernel_pm.h>
#include <gc_vip_kernel_debug.h>
#include <gc_vip_kernel_command.h>

/*
@brief list all pid for mmu support pdmode.
*/
vip_bool_e gckvip_hw_support_pdmode(
    gckvip_context_t *context
    )
{
    vip_bool_e support = vip_false_e;

    if (0x10000018 == context->chip_cid) {
        support = vip_true_e;
    }

    return support;
}

vip_bool_e gckvip_hw_support_cancel(
    gckvip_context_t *context
    )
{
    vip_bool_e support = vip_false_e;
    if (0x10000020 == context->chip_cid ||
        0x1000001b == context->chip_cid) {
        support = vip_true_e;
    }

    return support;
}

/*
list not support clock gating project.
*/
vip_bool_e gckvip_hw_support_clkgating(
    gckvip_context_t *context
    )
{
    vip_bool_e support = vip_true_e;
#if vpmdFPGA_BUILD
    support = vip_false_e;
#endif
    return support;
}

/*
@brief list all support bypass reorder.
*/
vip_bool_e gckvip_hw_bypass_reorder(
    gckvip_context_t *context
    )
{
    vip_bool_e support = vip_false_e;
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
static vip_bool_e gckvip_hw_bypass_reset(
    gckvip_context_t *context
    )
{
    vip_bool_e need = vip_true_e;
    vip_uint32_t chip_cid = context->chip_cid;

    if (0xA4 == chip_cid) {
        need = vip_false_e;
    }

    return need;
}

/*
@brief software resrt hardware.
*/
static vip_status_e reset_vip(
    gckvip_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t control = 0, idle = 0;
    vip_uint32_t count = 0;
    vip_uint32_t try_count = 0;
    vip_uint32_t value = 0;

    gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware,
        0x00000, 0x00070900));
    while ((count < 2) && (try_count < 10)) {
        /* Disable clock gating. */
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware,
            0x00104, 0x00000000));

        /* Disable pulse-eater and set clock */
        control = 0x01590880 | 0x20000;
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x0010C, control));
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware,
                                                0x0010C, (control | 0x1)));
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x0010C, control));
        /* FSCALE_CMD_LOAD */
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware,
            0x00000, 0x00070900 | 0x200));
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware,
            0x00000, 0x00070900));

        /* Wait for clock being stable. */
        #if vpmdFPGA_BUILD
        gckvip_os_delay(10);
        #else
        gckvip_os_udelay(100);
        #endif

        /* Isolate the VIP. */
        control = 0x00070900 | 0x80000;
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00000, control));
        /* Reset VIP in TA. */
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x003A8, 0x1));

        /* Wait for reset. */
        #if vpmdFPGA_BUILD
        gckvip_os_delay(10);
        #else
        gckvip_os_udelay(100);
        #endif

        /* Reset soft reset bit. */
        gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x003A8, 0x0));
        gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00000, SETBIT(control, 12, 0));

        /* Reset VIP isolation. */
        control &= ~0x80000;
        gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00000, control);

        gckvip_os_udelay(50);
        /* Read idle register. */
        gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00004, &idle);
        if ((idle & 0x1) == 0) {    /* if either FE or MC is not idle, try again. */
            PRINTK_I("reset vip read idle=0x%08X\n", idle);
            try_count++;
            continue;
        }

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        gckvip_register_dump_action(GCKVIP_REGISTER_DUMP_WAIT, hardware->core_id, 0x00004, 0x1, idle);
        #endif
        #if vpmdENABLE_CAPTURE_IN_KERNEL
        PRINTK("@[register.wait %u 0x%05X 0x%08X 0x%08X]\n", hardware->core_id, 0x00004, 0x1, idle);
        #endif

        /* Read reset register. */
        gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00000, &control);
        if ((control & 0x30000) != 0x30000) {
            PRINTK_I("reset vip control=0x%08X\n", control);
            try_count++;
            continue;
        }

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        gckvip_register_dump_action(GCKVIP_REGISTER_DUMP_WAIT, hardware->core_id,
            0x00000, 0x30000, (control & 0x30000));
        #endif
        #if vpmdENABLE_CAPTURE_IN_KERNEL
        PRINTK("@[register.wait %u 0x%05X 0x%08X 0x%08X]\n", hardware->core_id, 0x00000, 0x30000, control);
        #endif

        gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00388, &control);
        if (control & 0x01) {
            PRINTK_I("mmu control=0x%08X\n", control);
            try_count++;
            continue;
        }

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        gckvip_register_dump_action(GCKVIP_REGISTER_DUMP_WAIT, hardware->core_id,
            0x00388, 0x001, (control & 0x01));
        #endif
        #if vpmdENABLE_CAPTURE_IN_KERNEL
        PRINTK("@[register.wait %u 0x%05X 0x%08X 0x%08X]\n", hardware->core_id, 0x00388, 0x001, control);
        #endif

        count++;
    }

    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x0055C, 0x00FFFFFF);
    gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00090, &value);
    value &= 0xFFFFFFBF;
    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00090, value);

    return status;
onError:
    PRINTK_E("fail to reset hardware%d\n", hardware->core_id);
    return status;
}

/*
@brief initialzie vip hardware register.
*/
static vip_status_e init_vip(
    gckvip_context_t *context,
    gckvip_hardware_t *hardware
    )
{
    /* Limits the number of outstanding read requests, max value is 64. 0 is not limits */
#define MAX_OUTSTANDING_NUM     0
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t data = 0xcdcdcdcd;
    gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware,
                             0x00000, 0x00070900));
    data = SETBIT(0x00070900, 11, 0);
    gcOnError(gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware,
                             0x00000, data));
    data = SETBIT(0, 1, 1) | SETBIT(0, 11, 0);
    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x003A8, data);

    /* Reset memory counters. */
    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x0003C, ~0U);
    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x0003C, 0);

    /* Enable model level clock gating. */
    gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00100, &data);
    data = SETBIT(data, 0, 1);
    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00100, data);

    /* 2. enable MMU */
#if vpmdENABLE_MMU
    status = gckvip_mmu_enable(context, hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to enable MMU, status=%d.\n", status);
        gcGoOnError(status);
    }
#endif

    gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00104, &data);
    if (vip_false_e == gckvip_hw_support_clkgating(context)) {
        data = SETBIT(data, 3, 1);
    }
    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00104, data);

    gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x0010C, &data);
    data = SETBIT(data, 16, 0);
    data = SETBIT(data, 17, 1);
    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x0010C, data);

    if (gckvip_hw_bypass_reorder(context)) {
        gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00090, &data);
        data |= SETBIT(data, 6, 1);
        gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00090, data);
    }

    gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00414, &data);
    data = SETBIT(data, 0, MAX_OUTSTANDING_NUM);
    gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00414, data);

onError:
    return status;
}

#if vpmdENABLE_WAIT_LINK_LOOP
/*
@brief generate a wait-link command for command buffer.
*/
static vip_uint32_t *cmd_wait_link(
    vip_uint32_t *command,
    vip_uint32_t time,
    vip_uint32_t bytes,
    vip_uint32_t address
    )
{
    command[0] = ((7 << 27) | (time));
    command[1] = 0;

    gckvip_os_memorybarrier();

    command[2] = ((8 << 27) | ((bytes + 7) >> 3));
    gckvip_os_memorybarrier();
    command[3] = address;

    /* Return the link command location, for later to modify. */
    return command;
}
#endif

/*
@brief Link to a new command buffer address.
*/
#if vpmdENABLE_MMU || vpmdENABLE_WAIT_LINK_LOOP
static vip_uint32_t *cmd_update_link(
    vip_uint32_t *command,
    vip_uint32_t bytes,
    vip_uint32_t address
    )
{
    command[1] = address;
    /*
      you need to make sure that Link[1] is written to memory before Link[0].
      So, call memory barrier in here.
    */
    gckvip_os_memorybarrier();
    command[0] = ((8 << 27) | ((bytes + 7) >> 3));

    return &command[0];
}
#endif

/*
@brief
    do initialze hardware
*/
vip_status_e gckvip_hw_init(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t mmu_status = 0;
    vip_uint32_t i = 0;

    for (i = 0; i < device->hardware_count; i++) {
        gckvip_hardware_t *hardware = &device->hardware[i];

        PRINTK_D("hardware initialize core%d..\n", hardware->core_id);

        /* 1. Init VIP. */
        status = init_vip(context, hardware);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to init core%d, status=%d.\n", hardware->core_id, status);
            gcGoOnError(status);
        }

        /* 2. Enable all Interrupt EVENTs. */
        gckvip_register_dump_wrapper((void*)gckvip_write_register, hardware, 0x00014, 0xFFFFFFFF);

        gckvip_os_memorybarrier();

        gckvip_register_dump_wrapper((void*)gckvip_read_register, hardware, 0x00388, &mmu_status);
        if ((mmu_status & 0x01) == 0x00) {
            PRINTK_D("   MMU is disabled\n");
        }
        else {
            PRINTK_D("   MMU is enabled\n");
        }

        gckvip_os_udelay(10);

        /* Start the "wait-link" loop for command queue. */
    #if vpmdENABLE_WAIT_LINK_LOOP
        {
        if (VIP_NULL == hardware->waitlinkbuf_handle) {
            phy_address_t physical_tmp = 0;
            vip_uint32_t alloc_flag = GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS |
                                      GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL |
                                      GCVIP_VIDEO_MEM_ALLOC_MMU_PAGE_READ;
            #if !vpmdENABLE_VIDEO_MEMORY_CACHE
            alloc_flag |= GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE;
            #endif
            hardware->waitlinkbuf_size = WAIT_LINK_SIZE;
            status = gckvip_mem_allocate_videomemory(context, hardware->waitlinkbuf_size,
                                                     (void**)&hardware->waitlinkbuf_handle,
                                                     (void **)&hardware->waitlinkbuf_logical,
                                                     &physical_tmp,
                                                     gcdVIP_MEMORY_ALIGN_SIZE,
                                                     alloc_flag);
            if (status != VIP_SUCCESS) {
                PRINTK_E("fail to allocate video memory for wait/link buffer, status=%d.\n", status);
                gcGoOnError(status);
            }
            hardware->waitlinkbuf_physical = (vip_uint32_t)physical_tmp;
        }

        gckvip_hw_trigger(context, hardware);
        hardware->waitlink_logical = hardware->waitlinkbuf_logical;
        PRINTK_D("used wait-link trigger hardware core%d...\n", hardware->core_id);
        gckvip_os_udelay(100);
        }
    #endif
    }

    status = gckvip_cmd_submit_init(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize core%d states, status=%d.\n", device->device_id, status);
        gcGoOnError(status);
    }
#if vpmdENABLE_RECOVERY
    device->recovery_times = MAX_RECOVERY_TIMES;
#endif
    PRINTK_D("hardware initialize done core_count=%d\n", device->hardware_count);

onError:
    return status;
}

/* shut down VIP hardware */
vip_status_e gckvip_hw_destroy(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_WAIT_LINK_LOOP
    GCKVIP_LOOP_HARDWARE_START
    if (hw->waitlinkbuf_handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, hw->waitlinkbuf_handle);
        hw->waitlinkbuf_handle = VIP_NULL;
    }
    GCKVIP_LOOP_HARDWARE_END
#endif

    return status;
}

/*
@ brief
    Do a software HW reset.
*/
vip_status_e gckvip_hw_reset(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vip_bool_e soft_reset = gckvip_hw_bypass_reset(context);
    if (soft_reset == vip_false_e) {
        return status;
    }

    for (i = 0; i < device->hardware_count; i++) {
        gckvip_hardware_t *hardware = &device->hardware[i];
        status |= reset_vip(hardware);
    }
    return status;
}

/*
@ brief
    Insert flush mmu cache load states
*/
#if vpmdENABLE_MMU
static vip_status_e gckvip_hw_flush_mmu(
    gckvip_context_t *context,
    gckvip_submit_t *submit,
    gckvip_hardware_t *hardware
    )
{
    vip_uint32_t physical = submit->cmd_physical;
    vip_uint8_t *flush_logical_end = VIP_NULL;
    vip_uint32_t size = 0;
    vip_status_e status = VIP_SUCCESS;

    context->ptr_MMU_flush = &hardware->MMU_flush;
    flush_logical_end = (vip_uint8_t*)context->ptr_MMU_flush->logical + FLUSH_MMU_STATES_SIZE;

    if (submit->cmd_size > gcdVIP_MAX_CMD_SIZE) {
        size = gcdVIP_MAX_CMD_SIZE;
    }
    else {
        #if vpmdENABLE_WAIT_LINK_LOOP
        size = submit->cmd_size + WAIT_LINK_SIZE + EVENT_SIZE;
        #else
        size = submit->cmd_size + END_SIZE + EVENT_SIZE;
        #endif
    }

    /* link flush mmu states and command buffer */
    cmd_update_link((vip_uint32_t*)flush_logical_end, size, physical);
    #if vpmdENABLE_VIDEO_MEMORY_CACHE
    status = gckvip_mem_flush_cache(context->ptr_MMU_flush->handle, context->ptr_MMU_flush->physical,
                           context->ptr_MMU_flush->logical,
                           GCVIP_ALIGN(context->ptr_MMU_flush->size, vpmdCPU_CACHE_LINE_SIZE),
                           GCKVIP_CACHE_FLUSH);
    #endif

    return status;
}
#endif

#if vpmdENABLE_WAIT_LINK_LOOP
/*
@ brief
    trigger hardware, NPU start to run.
    write wait/link physical base address
    to command control register for starting vip.
*/
vip_status_e gckvip_hw_trigger(
    gckvip_context_t *context,
    gckvip_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    cmd_wait_link(hardware->waitlinkbuf_logical, WAIT_TIME_EXE,
                  hardware->waitlinkbuf_size, hardware->waitlinkbuf_physical);

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    gcOnError(gckvip_mem_flush_cache(hardware->waitlinkbuf_handle, hardware->waitlinkbuf_physical,
                           hardware->waitlinkbuf_logical,
                           GCVIP_ALIGN(hardware->waitlinkbuf_size, vpmdCPU_CACHE_LINE_SIZE),
                           GCKVIP_CACHE_FLUSH));
#endif

    gcOnError(gckvip_write_register(hardware, 0x00654, hardware->waitlinkbuf_physical));
    /* Make sure writing to command buffer and previous AHB register is done. */
    gckvip_os_memorybarrier();
    gcOnError(gckvip_write_register(hardware, 0x003A4,
              0x00010000 | ((hardware->waitlinkbuf_size + 7) >> 3)));

    PRINTK_D("wait-link, setup FE done..\n");
onError:
    return status;
}

/*
@ brief
    submit a command to hardware.
*/
vip_status_e gckvip_hw_submit(
    gckvip_context_t *context,
    gckvip_device_t *device,
    gckvip_submit_t *submit
    )
{
    vip_uint32_t *ptr = VIP_NULL;
    vip_uint32_t trigger_physical = submit->cmd_physical;
    vip_uint32_t trigger_size = 0;
    vip_uint32_t last_cmd_size = submit->last_cmd_size;
    vip_uint32_t cmd_size = submit->cmd_size;
    vip_uint32_t i = 0;
    vip_status_e status = VIP_SUCCESS;

    if ((VIP_NULL == submit->cmd_logical) || (VIP_NULL == submit->last_cmd_logical)) {
        PRINTK_E("failed to submit hardware command, logical=0x%"PRPx", last_logical=0x%"PRPx"\n",
                 submit->cmd_logical, submit->last_cmd_logical);
        return VIP_ERROR_FAILURE;
    }

    if (0 == submit->cmd_size) {
        PRINTK_E("failed to submit command size=0x%x\n", cmd_size);
        return VIP_ERROR_FAILURE;
    }

    gcOnError(gckvip_pm_hw_submit(context, device));

    /* Append the wait-link commands to the command buffer. */
    ptr = cmd_wait_link((vip_uint32_t*)(submit->last_cmd_logical + last_cmd_size),
                        WAIT_TIME_EXE, WAIT_LINK_SIZE, submit->last_cmd_physical + last_cmd_size);
#if vpmdENABLE_VIDEO_MEMORY_CACHE
    {
    vip_uint32_t align_size = GCVIP_ALIGN_BASE(last_cmd_size, vpmdCPU_CACHE_LINE_SIZE);
    gckvip_mem_flush_cache(submit->last_cmd_handle, submit->last_cmd_physical + align_size,
                           submit->last_cmd_logical + align_size, vpmdCPU_CACHE_LINE_SIZE,
                           GCKVIP_CACHE_FLUSH);
    }
#endif
    last_cmd_size += WAIT_LINK_SIZE;

    if (submit->cmd_logical == submit->last_cmd_logical) {
        cmd_size = last_cmd_size;
    }

    if (cmd_size >= (gcdVIP_MAX_CMD_SIZE - 7)) {
        trigger_size = gcdVIP_MAX_CMD_SIZE;
    }
    else {
        trigger_size = cmd_size + 7;
    }

    for (i = 0; i < device->hardware_count; i++) {
        gckvip_hardware_t *hardware = &device->hardware[i];

        #if vpmdENABLE_MMU
        if (1 == gckvip_os_get_atomic(hardware->flush_mmu)) {
            gckvip_hw_flush_mmu(context, submit, hardware);
            context->ptr_MMU_flush = &hardware->MMU_flush;
            trigger_physical = context->ptr_MMU_flush->physical;
            trigger_size = context->ptr_MMU_flush->size;
            gckvip_os_set_atomic(hardware->flush_mmu, 0);
        }
        #endif

        *hardware->irq_value = 0;

        /* Update the link command to wait-link buffer. */
        #if !(vpmdHARDWARE_BYPASS_MODE & 0x100)
        cmd_update_link(hardware->waitlink_logical, trigger_size, trigger_physical);
        #else
        HARDWARE_BYPASS_SUBMIT()
        #endif

        #if vpmdENABLE_VIDEO_MEMORY_CACHE
        gckvip_mem_flush_cache(hardware->waitlinkbuf_handle , hardware->waitlinkbuf_physical,
                               hardware->waitlinkbuf_logical,
                               GCVIP_ALIGN(hardware->waitlinkbuf_size, vpmdCPU_CACHE_LINE_SIZE),
                               GCKVIP_CACHE_FLUSH);
        #endif

        hardware->waitlink_logical = ptr;
        hardware->waitlink_handle = submit->last_cmd_handle;
        hardware->waitlink_physical = submit->last_cmd_physical + ((vip_uint8_t*)ptr - submit->last_cmd_logical);
    }

#if vpmdENABLE_HANG_DUMP || vpmdENABLE_WAIT_LINK_LOOP
    device->pre_command.cmd_handle = device->curr_command.cmd_handle;
    device->pre_command.cmd_logical = device->curr_command.cmd_logical;
    device->pre_command.cmd_physical = device->curr_command.cmd_physical;
    device->pre_command.cmd_size = device->curr_command.cmd_size;
    device->pre_command.last_cmd_logical = device->curr_command.last_cmd_logical;
    device->pre_command.last_cmd_physical = device->curr_command.last_cmd_physical;
    device->pre_command.last_cmd_size = device->curr_command.last_cmd_size;

    device->curr_command.cmd_handle = submit->cmd_handle;
    device->curr_command.cmd_logical = submit->cmd_logical;
    device->curr_command.cmd_physical = submit->cmd_physical;
    device->curr_command.cmd_size = cmd_size;
    device->curr_command.last_cmd_logical = submit->last_cmd_logical;
    device->curr_command.last_cmd_physical = submit->last_cmd_physical;
    device->curr_command.last_cmd_size = submit->last_cmd_size;
#endif
    /* only for linux emulator signal sync */
#if defined (LINUXEMULATOR)
    gckvip_os_delay(500);
#endif

onError:
    return status;
}

/*
@ brief
    hardware go back to WL.
*/
vip_status_e gckvip_hw_idle(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_uint32_t i = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t delay_time = 1; /* 1us */
    vip_int32_t try_times = 2000 / delay_time; /* 2ms/1us */

    if (vip_false_e == gckvip_os_get_atomic(device->device_idle)) {
        for (i = 0; i < device->hardware_count; i++) {
            gckvip_hardware_t *hardware = &device->hardware[i];
            /* switch to the kernel's WL command. Ensure the kernel WL is a closed loop. */
            cmd_wait_link(hardware->waitlinkbuf_logical, WAIT_TIME_EXE,
                          hardware->waitlinkbuf_size, hardware->waitlinkbuf_physical);

            #if vpmdENABLE_VIDEO_MEMORY_CACHE
            gcOnError(gckvip_mem_flush_cache(hardware->waitlinkbuf_handle, hardware->waitlinkbuf_physical,
                                   hardware->waitlinkbuf_logical,
                                   GCVIP_ALIGN(hardware->waitlinkbuf_size, vpmdCPU_CACHE_LINE_SIZE),
                                   GCKVIP_CACHE_FLUSH));
            #endif

            /* Update the command link to the kernel WL command. */
            cmd_update_link(hardware->waitlink_logical, hardware->waitlinkbuf_size, hardware->waitlinkbuf_physical);
            #if vpmdENABLE_VIDEO_MEMORY_CACHE
            {
            vip_uint32_t flush_physical = GCVIP_ALIGN_BASE(hardware->waitlink_physical, vpmdCPU_CACHE_LINE_SIZE);
            vip_uint32_t diff = hardware->waitlink_physical - flush_physical;
            gcOnError(gckvip_mem_flush_cache(hardware->waitlink_handle , flush_physical,
                                   (vip_uint8_t*)hardware->waitlink_logical - diff,
                                   vpmdCPU_CACHE_LINE_SIZE, GCKVIP_CACHE_FLUSH));
            }
            #endif
            /* Wait the command has been linked to the kernel WL command. */
            while (try_times--) {
                vip_uint32_t cur_address = 0;
                gckvip_read_register(hardware, 0x00664, &cur_address);
                if (cur_address < hardware->waitlink_physical ||
                    cur_address >= hardware->waitlink_physical + WAIT_LINK_SIZE) {
                    break;
                }
                gckvip_os_udelay(delay_time);
            }

            hardware->waitlink_logical = hardware->waitlinkbuf_logical;
            hardware->waitlink_handle = hardware->waitlinkbuf_handle;
            hardware->waitlink_physical = hardware->waitlink_physical;
        }
        gcOnError(gckvip_pm_hw_idle(context, device));
    }
    else {
        PRINTK_D("vip in idle status, don't need going to idle again\n");
    }

onError:
    return status;
}
#else
vip_status_e gckvip_hw_submit(
    gckvip_context_t *context,
    gckvip_device_t *device,
    gckvip_submit_t *submit
    )
{
    vip_uint32_t trigger_size = 0, trigger_physical = 0, cmd_size = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;

    if ((VIP_NULL == context) || (VIP_NULL == device) || (VIP_NULL == submit)) {
        PRINTK_E("param is NUll, failed to hw submit.\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    trigger_physical = submit->cmd_physical;
    cmd_size = submit->cmd_size;

    if ((VIP_NULL == submit->cmd_logical) || (VIP_NULL == submit->last_cmd_logical)) {
        PRINTK_E("failed to submit hardware command, logical=0x%"PRPx", last_logical=0x%"PRPx"\n",
                 submit->cmd_logical, submit->last_cmd_logical);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (0 == submit->cmd_size) {
        PRINTK_E("failed to submit command size=0x%x\n", cmd_size);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    gcOnError(gckvip_pm_hw_submit(context, device))

    if (cmd_size >= (gcdVIP_MAX_CMD_SIZE - 7)) {
        trigger_size = gcdVIP_MAX_CMD_SIZE;
    }
    else {
        trigger_size = cmd_size + 7;
    }

    for (i = 0; i < device->hardware_count; i++) {
        gckvip_hardware_t *hardware = &device->hardware[i];
        #if vpmdENABLE_MMU
        if (1 == gckvip_os_get_atomic(hardware->flush_mmu)) {
            gckvip_hw_flush_mmu(context, submit, hardware);
            context->ptr_MMU_flush = &hardware->MMU_flush;
            trigger_physical = context->ptr_MMU_flush->physical;
            trigger_size = context->ptr_MMU_flush->size + 7;
            gckvip_os_set_atomic(hardware->flush_mmu, 0);
        }
        #endif
        *hardware->irq_value = 0;
        #if !(vpmdHARDWARE_BYPASS_MODE & 0x100)
        gcOnError(gckvip_write_register(hardware, 0x00654, trigger_physical));
        gckvip_os_memorybarrier();
        gcOnError(gckvip_write_register(hardware, 0x003A4, 0x00010000 | (trigger_size >> 3)));
        #else
        HARDWARE_BYPASS_SUBMIT()
        #endif
    }

#if vpmdENABLE_HANG_DUMP || vpmdENABLE_WAIT_LINK_LOOP
    device->pre_command.cmd_handle = device->curr_command.cmd_handle;
    device->pre_command.cmd_logical = device->curr_command.cmd_logical;
    device->pre_command.cmd_physical = device->curr_command.cmd_physical;
    device->pre_command.cmd_size = device->curr_command.cmd_size;
    device->pre_command.last_cmd_logical = device->curr_command.last_cmd_logical;
    device->pre_command.last_cmd_physical = device->curr_command.last_cmd_physical;
    device->pre_command.last_cmd_size = device->curr_command.last_cmd_size;

    device->curr_command.cmd_handle = submit->cmd_handle;
    device->curr_command.cmd_logical = submit->cmd_logical;
    device->curr_command.cmd_physical = submit->cmd_physical;
    device->curr_command.cmd_size = cmd_size;
    device->curr_command.last_cmd_logical = submit->last_cmd_logical;
    device->curr_command.last_cmd_physical = submit->last_cmd_physical;
    device->curr_command.last_cmd_size = submit->last_cmd_size;
#endif
    /* only for linux emulator signal sync, not used on FPGA and silicon */
#if defined (LINUXEMULATOR)
    gckvip_os_delay(500);
#endif

onError:
    return status;
}

/*
@ brief query hardware status, runnign or idle.
*/
vip_status_e gckvip_hw_idle(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (vip_false_e == gckvip_os_get_atomic(device->device_idle)) {
        status = gckvip_pm_hw_idle(context, device);
    }
    else {
        PRINTK_D("vip in idle status, don't need going to idle again\n");
    }

    return status;
}
#endif

vip_status_e gckvip_hw_read_info(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = &context->device[0];
    gckvip_hardware_t *hardware = &device->hardware[0];

    gckvip_write_register(hardware, 0x00000, 0x00070900);
    gckvip_read_register(hardware, 0x00020, &context->chip_ver1);
    gckvip_read_register(hardware, 0x00024, &context->chip_ver2);
    gckvip_read_register(hardware, 0x00030, &context->chip_cid);
    gckvip_read_register(hardware, 0x00028, &context->chip_date);
    PRINTK_D("ver1=0x%04x, ver2=0x%04x, cid=0x%04x, date=0x%08x\n",
              context->chip_ver1, context->chip_ver2, context->chip_cid,
              context->chip_date);

    return status;
}

/*
@brief  read a hardware register.
@param  address, the address of hardware register.
@param  data, read data from register.
*/
vip_status_e gckvip_read_register(
    gckvip_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    if(VIP_NULL == hardware->reg_base) {
        PRINTK_E("core%d, failed to read register address=0x%05x, reg is NULL\n",
                    address, hardware->core_id);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
#if vpmdENABLE_MULTIPLE_TASK
    if (hardware->reg_mutex != VIP_NULL) {
        status = gckvip_os_lock_mutex(hardware->reg_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to lock video register mutex write register\n");
            return VIP_ERROR_FAILURE;
        }
    }
#endif

    gcOnError(gckvip_os_read_reg(hardware->reg_base, address, data));

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (hardware->reg_mutex != VIP_NULL) {
        if (VIP_SUCCESS != gckvip_os_unlock_mutex(hardware->reg_mutex)) {
            PRINTK_E("failed to unlock video register mutex write register\n");
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
vip_status_e gckvip_write_register_ext(
    gckvip_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t data
    )
{
    vip_status_e status = VIP_SUCCESS;
    if(VIP_NULL == hardware->reg_base) {
        PRINTK_E("core%d, failed to write register address=0x%05x, reg is NULL\n",
                  address, hardware->core_id);
        return VIP_ERROR_OUT_OF_MEMORY;
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (hardware->reg_mutex != VIP_NULL) {
        status = gckvip_os_lock_mutex(hardware->reg_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to lock video register mutex write register\n");
            return VIP_ERROR_FAILURE;
        }
    }
#endif

    gcOnError(gckvip_os_write_reg(hardware->reg_base, address, data));

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (hardware->reg_mutex != VIP_NULL) {
        if (VIP_SUCCESS != gckvip_os_unlock_mutex(hardware->reg_mutex)) {
            PRINTK_E("failed to unlock video register mutex write register\n");
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
vip_status_e gckvip_write_register(
    gckvip_hardware_t *hardware,
    vip_uint32_t address,
    vip_uint32_t data
    )
{
    vip_status_e status = VIP_SUCCESS;
    status = gckvip_write_register_ext(hardware, address, data);
    return status;
}

/*
@brief query hardware going to idle or not.
@param timeout the number of milliseconds this waiting.
@param is_fast true indicate that need udelay for poll hardware idle or not.
*/
vip_bool_e gckvip_hw_query_idle(
    gckvip_hardware_t *hardware,
    vip_uint32_t timeout,
    vip_uint8_t is_fast
    )
{
    #define POLL_SPACE    5   /* 5ms */
    #define POLL_SPACE_US 50 /* 50us */
    vip_int32_t count = 0;
    vip_bool_e idle = vip_false_e;
    vip_uint32_t idle_reg = 0;
    vip_uint32_t expect_idle_value = 0x7FFFFFFF;

    if (is_fast) {
        count = (vip_int32_t)(timeout * 1000) / POLL_SPACE_US;
    }
    else {
        count = (vip_int32_t)timeout / POLL_SPACE;
    }

    gckvip_read_register(hardware, 0x00004, &idle_reg);
    while ((count > 0) && ((idle_reg & expect_idle_value) != expect_idle_value)) {
        if (is_fast) {
            gckvip_os_udelay(POLL_SPACE_US);
        }
        else {
            gckvip_os_delay(POLL_SPACE);
        }
        gckvip_read_register(hardware, 0x00004, &idle_reg);
        count--;
    }
    PRINTK_D("query hw%d idle, wait time=%dms, fast=%d, reg=0x%x, is_idle=%s\n",
             hardware->core_id, timeout, is_fast, idle_reg, idle_reg == expect_idle_value ? "Y" : "N");

    if (expect_idle_value == idle_reg) {
        idle = vip_true_e;
    }

    return idle;
}

#if vpmdENABLE_POLLING
/*
@brief poll to waiting hardware going to idle.
@param timeout the number of milliseconds this waiting.
*/
vip_status_e gckvip_hw_wait_poll(
    gckvip_hardware_t *hardware,
    gckvip_device_t   *device,
    vip_uint32_t timeout
    )
{
    #define POLL_SPACE   5  /* 5ms */
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t count = (vip_int32_t)timeout / POLL_SPACE;
    vip_uint32_t idle_reg = 0;

    PRINTK_D("polling hardware count=%d...\n", count);
    if (vip_true_e == gckvip_os_get_atomic(device->device_idle)) {
        PRINTK_I("wait poll, hardware is in idle status\n");
        return VIP_SUCCESS;
    }

#if vpmdENABLE_WAIT_LINK_LOOP
    gckvip_read_register(hardware, 0x00004, &idle_reg);
    while ((count > 0) && ((idle_reg & 0xFFFFE) != 0xFFFFE)) {
        gckvip_os_delay(POLL_SPACE);
        gckvip_read_register(hardware, 0x00004, &idle_reg);
        count--;
    }

    if (count > 0) {
        vip_uint32_t address = 0;
        vip_uint32_t wl_address = 0, max_address = 0;
        count = (vip_int32_t)timeout / POLL_SPACE;
        gckvip_read_register(hardware, 0x00664, &address);
        wl_address = device->curr_command.cmd_physical + device->curr_command.cmd_size - WAIT_LINK_SIZE;
        max_address = device->curr_command.cmd_physical + device->curr_command.cmd_size;
        PRINTK_E("base=0x%08x, size=0x%08x, wl addr=0x%08x, max addr=0x%08x, address=0x%08x\n",
            device->curr_command.cmd_physical, device->curr_command.cmd_size , wl_address, max_address, address);
        while (count > 0) {
            /* check loop on command wait-link cmds */
            if ((address < max_address) && (address >= wl_address)) {
                break;
            }
            gckvip_os_delay(POLL_SPACE);
            gckvip_read_register(hardware, 0x00664, &address);
            count--;
        }

        if (count > 0) {
            status = VIP_SUCCESS;
        }
        else {
            PRINTK_E("polling read idle status=0x%x, address=0x%08x, wait-link=0x%08x, size=0x%x\n",
                     idle_reg, address, hardware->waitlinkbuf_physical, hardware->waitlinkbuf_size);
            status = VIP_ERROR_TIMEOUT;
        }
    }
    else {
        PRINTK_E("polling time=%d, wait-link read idle status=0x%x\n", timeout, idle_reg);
        status = VIP_ERROR_TIMEOUT;
    }
#else
    gckvip_read_register(hardware, 0x00004, &idle_reg);
    while ((count > 0) && ((idle_reg & 0xFFFFF) != 0xFFFFF)) {
        gckvip_os_delay(POLL_SPACE);
        gckvip_read_register(hardware, 0x00004, &idle_reg);
        count--;
    }

    if (count > 0) {
        status = VIP_SUCCESS;
    }
    else {
        PRINTK_E("polling time=%d, idle status=0x%x\n", timeout, idle_reg);
        status = VIP_ERROR_TIMEOUT;
    }
#endif
    return status;
}
#endif

/*
@brief waiting for hardware going to idle.
@param context, the contect of kernel space.
@param timeout the number of milliseconds this waiting.
@param is_fast true indicate that need udelay for poll hardware idle or not.
*/
vip_status_e gckvip_hw_wait_idle(
    gckvip_context_t *context,
    gckvip_hardware_t *hardware,
    vip_uint32_t timeout,
    vip_uint8_t is_fast
    )
{
    #define POLL_SPACE    5   /* 5ms */
    #define POLL_SPACE_US 50 /* 50us */
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t count = 0;
    vip_uint32_t idle_reg = 0;

    if (is_fast) {
        count = (vip_int32_t)(timeout * 1000) / POLL_SPACE_US;
    }
    else {
        count = (vip_int32_t)timeout / POLL_SPACE;
    }
#if vpmdENABLE_WAIT_LINK_LOOP
    /* wait function module idle */
    gckvip_read_register(hardware, 0x00004, &idle_reg);
    while ((count > 0) && ((idle_reg & 0xFFFFE) != 0xFFFFE)) {
        if (is_fast) {
            gckvip_os_udelay(POLL_SPACE_US);
        }
        else {
            gckvip_os_delay(POLL_SPACE);
        }
        gckvip_read_register(hardware, 0x00004, &idle_reg);
        count--;
    }

    if (count <= 0) {
        PRINTK_E("hw wait idle, function module not idle=0x%x\n", idle_reg);
    }

    if ((hardware->waitlink_logical != VIP_NULL) && (hardware->waitlinkbuf_logical != VIP_NULL)) {
        /* wait FE idle, modify WAIT cmd to END cmd for ending FE */
        if (hardware->waitlink_logical != hardware->waitlinkbuf_logical) {
            hardware->waitlink_logical[1] = 0;
            hardware->waitlinkbuf_logical[1] = 0;
            gckvip_os_memorybarrier();
            hardware->waitlink_logical[0] = (2 << 27);
            hardware->waitlinkbuf_logical[0] = (2 << 27);
        }
        else {
            hardware->waitlink_logical[1] = 0;
            gckvip_os_memorybarrier();
            hardware->waitlink_logical[0] = (2 << 27);
        }
    }

    gckvip_os_udelay(200); /* waiting idle */
    gckvip_read_register(hardware, 0x00004, &idle_reg);
#else
    gckvip_read_register(hardware, 0x00004, &idle_reg);
    while ((count > 0) && ((idle_reg & 0xFFFFF) != 0xFFFFF)) {
        if (is_fast) {
            gckvip_os_udelay(POLL_SPACE_US);
        }
        else {
            gckvip_os_delay(POLL_SPACE);
        }
        gckvip_read_register(hardware, 0x00004, &idle_reg);
        count--;
    }
#endif

    PRINTK_D("wait hw%d idle, reg=0x%x, is_idle=%s\n",
            hardware->core_id, idle_reg, idle_reg == 0x7FFFFFFF ? "Y" : "N");

    if ((idle_reg & 0x7FFFFFFF) != 0x7FFFFFFF) {
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
                    if (context->chip_cid != 0xba) {
                        PRINTK_E("wait hw%d idle, %s not idle.\n", hardware->core_id, module_name[i]);
                        status = VIP_ERROR_FAILURE;
                    }
                }
                else {
                    PRINTK_E("wait hw%d idle, %s not idle.\n", hardware->core_id, module_name[i]);
                    status = VIP_ERROR_FAILURE;
                }
            }
            idle_reg >>= 1;
        }
    }

    return status;
}

/*
@brief recover hardware when hang occur.
@param context, the contect of kernel space.
@param device, recover device.
*/
#if vpmdENABLE_RECOVERY
vip_status_e gckvip_hw_recover(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;

    PRINTK_I("error: wait interrupt timeout, times=%d\n", device->recovery_times);
    if (device->recovery_times < 0) {
        PRINTK_E("fail to recovery.....\n");
        gcGoOnError(VIP_ERROR_TIMEOUT);
    }
    device->recovery_times--;

    /* power off and then on, clock off, on, HW reset.. */
    for (i = 0; i < device->hardware_count; i++) {
        gckvip_hardware_t *hardware = &device->hardware[i];
        gckvip_drv_set_power_clk(hardware->core_id, GCKVIP_POWER_OFF);
        gckvip_drv_set_power_clk(hardware->core_id, GCKVIP_POWER_ON);
    }

    /* recovery hardware */
    status = gckvip_hw_reset(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to reset hardware, status=%d.\n",status);
        gcGoOnError(status);
    }

    status = gckvip_hw_init(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failedto initialize hardware, status=%d.\n", status);
        gcGoOnError(status);
    }
    else {
        PRINTK_E("device%d, sys_time=0x%"PRIx64", hardware recovery successfully.....\n",
                  device->device_id, gckvip_os_get_time());
        device->hw_submit_count = 0;
        gckvip_hw_idle(context, device);
        #if vpmdENABLE_HANG_DUMP
        device->dump_finish = vip_false_e;
        #endif
        gcOnError(VIP_SUCCESS);
    }

onError:
    return status;
}
#endif

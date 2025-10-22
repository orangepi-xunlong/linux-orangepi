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

#include <gc_vip_kernel_pm.h>
#include <gc_vip_hardware.h>
#include <gc_vip_kernel.h>
#include <gc_vip_kernel_drv.h>
#include <gc_vip_kernel_allocator.h>
#include <gc_vip_kernel_mmu.h>
#include <gc_vip_video_memory.h>
#include <gc_vip_kernel_debug.h>
#include <gc_vip_kernel_command.h>

vip_status_e gckvip_cmd_submit_init(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_submit_t submission = {0};
    if (VIP_NULL == device->init_command.handle) {
        PRINTK_E("init command memory is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    /* Submit commands. */
    submission.cmd_logical = (vip_uint8_t *)device->init_command.logical;
    submission.cmd_physical = (vip_uint32_t)device->init_command.physical;
    submission.cmd_handle = device->init_command.handle;
    submission.cmd_size = device->init_command.size;
    submission.last_cmd_logical = (vip_uint8_t *)device->init_command.logical;
    submission.last_cmd_physical = (vip_uint32_t)device->init_command.physical;
    submission.last_cmd_handle = device->init_command.handle;
    submission.last_cmd_size = device->init_command.size;
    submission.device_id = device->device_id;
    PRINTK_D("submit initialize commands size=0x%x, hand=0x%"PRPx"\n",
              device->init_command.size, device->init_command.handle);
#if vpmdREGISTER_NETWORK && (!vpmdENABLE_CNN_PROFILING)
    gckvip_prepare_profiling(&submission, device);
#endif
    gcOnError(gckvip_hw_submit(context, device, &submission));
#if vpmdENABLE_DEBUGFS
    gckvip_debug_set_profile_data(gckvip_os_get_time(), PROFILE_SUBMIT);
#endif

    /* Wait to be done. */
    {
    gckvip_wait_cmd_t wait_data;
    wait_data.mask = 0xFFFFFFFF;
    wait_data.time_out = 100; /* 100ms timeout for initialize states */
    wait_data.cmd_handle = device->init_command.handle;
#if vpmdENABLE_CLOCK_SUSPEND
    gcOnError(gckvip_os_inc_atomic(device->disable_clock_suspend));
#endif
#if vpmdPOWER_OFF_TIMEOUT && !vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(device->dp_management.enable_timer, vip_false_e);
#elif vpmdPOWER_OFF_TIMEOUT && vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(context->sp_management.enable_timer, vip_false_e);
#endif
    gcOnError(gckvip_cmd_wait(context, (void *)&wait_data, device));
#if vpmdPOWER_OFF_TIMEOUT && !vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(device->dp_management.enable_timer, vip_true_e);
#elif vpmdPOWER_OFF_TIMEOUT && vpmdONE_POWER_DOMAIN
    gckvip_os_set_atomic(context->sp_management.enable_timer, vip_true_e);
#endif
#if vpmdENABLE_CLOCK_SUSPEND
    gcOnError(gckvip_os_dec_atomic(device->disable_clock_suspend));
#endif
    }
    PRINTK_D("initialize commands done\n");

    return status;
onError:
    PRINTK_E("fail to submit initialize command\n");
    return status;
}

/*
@brief To run any initial commands once in the beginning.
*/
vip_status_e gckvip_cmd_prepare_init(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_uint32_t cmd_size = sizeof(vip_uint32_t) * (20 + 18);
    vip_uint32_t *logical = VIP_NULL;
    vip_uint32_t buf_size = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t count = 0;
    vip_uint32_t reg_data = 0;
    vip_uint32_t alloc_flag = GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL |
                              GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS |
                              GCVIP_VIDEO_MEM_ALLOC_MMU_PAGE_READ;
    phy_address_t physical_tmp = 0;
    vip_uint32_t axi_sram_size = 0;
    vip_uint32_t i = 0;
#if !vpmdENABLE_VIDEO_MEMORY_CACHE
    alloc_flag |= GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE;
#endif
    PRINTK_I("start prepare init commands\n");

    for (i = 0; i < context->device_count; i++) {
        axi_sram_size += context->axi_sram_size[i];
    }

    if (context->vip_sram_size > 0) {
        cmd_size += sizeof(vip_uint32_t) * 2;
    }
    if (axi_sram_size > 0) {
        cmd_size += sizeof(vip_uint32_t) * 4;
    }

    buf_size = cmd_size + APPEND_COMMAND_SIZE;
    if (1 < device->hardware_count) {
        buf_size += 2 * CHIP_ENABLE_SIZE;
    }
    buf_size = GCVIP_ALIGN(buf_size, vpmdCPU_CACHE_LINE_SIZE);
    gcOnError(gckvip_mem_allocate_videomemory(context, buf_size, &device->init_command.handle,
                                              (void **)&logical, &physical_tmp,
                                              gcdVIP_MEMORY_ALIGN_SIZE, alloc_flag));
    gckvip_os_zero_memory(logical, buf_size);
    device->init_command.logical = (void*)logical;
    device->init_command.physical = (vip_uint32_t)physical_tmp;

    logical[count++] = 0x0801028A;
    logical[count++] = 0x00000011;
    logical[count++] = 0x08010E13;
    logical[count++] = 0x00000002;
    logical[count++] = 0x08010E12;/* nn/tp debug register */
    reg_data = SETBIT(0, 16, 1) | SETBIT(0, 27, 1) | SETBIT(0, 28, 1);
    logical[count++] = reg_data;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBIT(0, 24, 3) | 0x0;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBIT(0, 24, 3) | 0x1;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBIT(0, 24, 3) | 0x2;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBIT(0, 24, 3) | 0x3;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBIT(0, 24, 3) | 0x4;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBIT(0, 24, 3) | 0x5;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBIT(0, 24, 3) | 0x6;
    logical[count++] = 0x08010E55;
    logical[count++] = SETBIT(0, 16, 1) | SETBIT(0, 24, 3) | 0x7;
    logical[count++] = 0x08010E21;
    logical[count++] = 0x00020000;
    logical[count++] = 0x08010e02;
    logical[count++] = 0x00000701;
    logical[count++] = 0x48000000;
    logical[count++] = 0x00000701;
    logical[count++] = 0x0801022c;
    logical[count++] = 0x0000001f;
    if (context->vip_sram_size > 0) {
        logical[count++] = 0x08010e4e;
        logical[count++] = context->vip_sram_address;
    }
    if (axi_sram_size > 0) {
        logical[count++] = 0x08010e4f;
        logical[count++] = context->axi_sram_physical;
        logical[count++] = 0x08010e50;
        logical[count++] = context->axi_sram_physical + axi_sram_size;
    }
    logical[count++] = 0x08010e02;
    logical[count++] = 0x00000701;
    logical[count++] = 0x48000000;
    logical[count++] = 0x00000701;

    if (1 < device->hardware_count) {
        logical[count++] = 0x68000000 | (1 << device->hardware[0].core_id);
        logical[count++] = 0;
    }
#if !vpmdENABLE_POLLING
    logical[count++] = 0x08010E01;
    logical[count++] = (((1 << 6)) | (IRQ_EVENT_ID & 0x0000001D));
#endif
    if (1 < device->hardware_count) {
        logical[count++] = 0x6800FFFF;
        logical[count++] = 0;
    }

#if !vpmdENABLE_WAIT_LINK_LOOP
    logical[count++] = 0x10000000;
    logical[count++] = 0;
#endif
    device->init_command_size = count * sizeof(vip_uint32_t);

    if ((count * sizeof(vip_uint32_t)) > buf_size) {
        PRINTK_E("failed to init commands, size is overflow\n");
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    cmd_size = count * sizeof(vip_uint32_t);
    device->init_command.size = cmd_size;

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    gckvip_mem_flush_cache(device->init_command.handle, device->init_command.physical,
                           device->init_command.logical, buf_size, GCKVIP_CACHE_FLUSH);
#endif
    gcOnError(gckvip_database_get_id(&context->videomem_database, device->init_command.handle,
                                     gckvip_os_get_pid(), &device->init_command.handle_id));
onError:
    return status;
}

vip_status_e gcvip_cmd_timeout_handle(
    gckvip_context_t *context,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_HANG_DUMP
    gckvip_hang_dump(device);
#endif

#if vpmdENABLE_RECOVERY
    status = gckvip_hw_recover(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to recover hardware, status=%d.\n", status);
        gcGoOnError(status);
    }
    PRINTK_I("dev%d hardware recovery done, return %d for app use.\n", device->device_id, status);
    gcGoOnError(VIP_ERROR_RECOVERY);
#else
    PRINTK_E("dev%d, sys_time=0x%"PRIx64", error hardware hang wait irq.\n",
              device->device_id, gckvip_os_get_time());
    gcGoOnError(VIP_ERROR_TIMEOUT);
#endif

onError:
    return status;
}

/*
@brief wait hardware interrupt return.
*/
vip_status_e gckvip_cmd_wait(
    gckvip_context_t *context,
    gckvip_wait_cmd_t *info,
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t timeout = info->time_out;
    gckvip_hardware_t *hardware = VIP_NULL;
    if (vip_false_e == gckvip_os_get_atomic(device->device_idle)) {
        PRINTK_I("dev%d wait mem_handle=0x%"PRPx" start\n", device->device_id, info->cmd_handle);
        hardware = &device->hardware[0];
        #if vpmdENABLE_POLLING
        /* poll hardware idle, only need first core? */
        status = gckvip_hw_wait_poll(hardware, device, timeout);
        #else
        /* wait interrupt on first core */
        if ((VIP_NULL == hardware->irq_queue) || (VIP_NULL == hardware->irq_value)) {
            PRINTK_E("fail to wait interrupt. int queue or int falg is NULL\n");
            return VIP_ERROR_OUT_OF_MEMORY;
        }
        status = gckvip_os_wait_interrupt(hardware->irq_queue, hardware->irq_value,
                                          timeout, info->mask);
        #endif
        if (status == VIP_ERROR_TIMEOUT) {
            status = gcvip_cmd_timeout_handle(context, device);
            gcGoOnError(status);
        }
        else {
            status = gckvip_check_irq_value(context, device);
            if (status == VIP_ERROR_TIMEOUT) {
                status = gcvip_cmd_timeout_handle(context, device);
                gcGoOnError(status);
            }
            else if ((status != VIP_SUCCESS) && (status != VIP_ERROR_RECOVERY) && (status != VIP_ERROR_CANCELED)) {
                PRINTK_E("fail to dev%d check irq value status=%d\n", device->device_id, status);
                gcGoOnError(status);
            }
            *hardware->irq_value = 0;
        }

        #if vpmdREGISTER_NETWORK && (!vpmdENABLE_CNN_PROFILING)
        gckvip_end_profiling(context, device, info->cmd_handle);
        #endif
        #if vpmdENABLE_DEBUGFS
        gckvip_debug_set_profile_data(gckvip_os_get_time(), PROFILE_WAIT);
        #endif

        /* Then link back to the WL commands for letting vip going to idle. */
        if (VIP_SUCCESS != gckvip_hw_idle(context, device)) {
            PRINTK_E("fail dev%d to hw idle.\n", device->device_id);
            gcGoOnError(VIP_ERROR_FAILURE)
        }

        PRINTK_I("dev%d wait mem_handle=0x%"PRPx" done\n", device->device_id, info->cmd_handle);
    }

    return status;
onError:
    PRINTK_E("failed to wait command complete status=%d\n", status);
    return status;
}

/*
@brief commit commands to hardware.
*/
vip_status_e gckvip_cmd_submit(
    gckvip_context_t *context,
    void *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = VIP_NULL;
    gckvip_submit_t *submit = VIP_NULL;

    /* defined(__linux__) is just for viplite run on Linux.
       user space logical need to be switched kernel space logical address
    */
#if defined(__linux__)
    gckvip_submit_t submitTmp = {0};
    gckvip_submit_t *tmp = (gckvip_submit_t *)data;
    gcOnError(gckvip_mem_get_kernellogical(context, tmp->cmd_handle, tmp->cmd_physical,
                                vip_true_e, (void**)&submitTmp.cmd_logical));
    submitTmp.cmd_physical = tmp->cmd_physical;
    submitTmp.cmd_handle = tmp->cmd_handle;
    submitTmp.cmd_size = tmp->cmd_size;
    gcOnError(gckvip_mem_get_kernellogical(context, tmp->last_cmd_handle,
                                 tmp->last_cmd_physical,
                                 vip_true_e, (void**)&submitTmp.last_cmd_logical));
    submitTmp.last_cmd_physical = tmp->last_cmd_physical;
    submitTmp.last_cmd_handle = tmp->last_cmd_handle;
    submitTmp.last_cmd_size = tmp->last_cmd_size;
    submitTmp.device_id = tmp->device_id;
    submit = &submitTmp;
#else
    submit = (gckvip_submit_t *)data;
#endif
    if (VIP_NULL == submit->cmd_handle) {
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    device = &context->device[submit->device_id];
#if vpmdENABLE_RECOVERY
    if (0 > device->recovery_times) {
        PRINTK_E("device %d recover fail last time\n", device->device_id);
        gcGoOnError(VIP_ERROR_FAILURE);
    }
#endif

#if vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
    status = gckvip_pm_check_power(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to check power in submit status=%d\n", status);
        gcGoOnError(status);
    }
#endif

    PRINTK_D("dev%d submit command size=0x%x, physical=0x%08x, last cmd size=0x%x, physical=0x%08x\n",
              submit->device_id, submit->cmd_size, submit->cmd_physical, submit->last_cmd_size,
              submit->last_cmd_physical);

    GCKVIP_CHECK_USER_PM_STATUS();
#if vpmdREGISTER_NETWORK && (!vpmdENABLE_CNN_PROFILING)
    gckvip_prepare_profiling(submit, device);
#endif
    gcOnError(gckvip_hw_submit(context, device, submit));
    device->hw_submit_count++;

#if vpmdENABLE_DEBUGFS
    gckvip_debug_set_profile_data(gckvip_os_get_time(), PROFILE_SUBMIT);
#endif

#if vpmdREGISTER_NETWORK
#if !vpmdENABLE_CNN_PROFILING
    gckvip_start_profiling(submit, device);
#endif
    gckvip_start_segment(device, submit->cmd_handle);
#endif

    return status;
onError:
    PRINTK_E("fail to submit cmds to hardware status=%d\n", status);
    return status;
}

#if vpmdENABLE_MULTIPLE_TASK
vip_status_e gckvip_cmd_thread_handle(
    gckvip_context_t *context,
    gckvip_queue_t *queue
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_queue_data_t *queue_data = VIP_NULL;
    gckvip_task_slot_t *m_task = VIP_NULL;
    vip_uint32_t device_id = queue->device_id;
    gckvip_device_t *device = &context->device[device_id];
#if vpmdONE_POWER_DOMAIN
    gckvip_pm_t* pm = &context->sp_management;
#else
    gckvip_pm_t* pm = &device->dp_management;
#endif
    gckvip_wait_cmd_t wait_data;
    wait_data.mask = 0xFFFFFFFF;

    gckvip_os_set_atomic(device->mt_thread_running, vip_true_e);
    PRINTK_I("multi-thread dev%d process thread start\n", device_id);
    gckvip_os_set_signal(device->mt_thread_signal, vip_false_e);

    while (gckvip_os_get_atomic(device->mt_thread_running)) {
        if (gckvip_queue_read(queue, &queue_data)) {
            if (VIP_NULL == queue_data) {
                PRINTK_E("multi-task dev%d read queue data is NULL\n", device_id);
                continue;
            }

            gckvip_hashmap_get_by_index(&device->mt_hashmap, queue_data->v1, (void**)&m_task);
            if (VIP_NULL == m_task) {
                PRINTK_E("multi-task dev%d get task%d by index fail\n", device_id, queue_data->v1);
                continue;
            }
            PRINTK_I("multi-task dev%d task slot index=%d, start submit command mem_handle=0x%"PRPx"\n",
                     device_id, queue_data->v1, m_task->submit_handle.cmd_handle);
            /* make sure other thread can not change power state during submit */
            gckvip_lock_recursive_mutex(&pm->mutex);
            gckvip_os_lock_mutex(m_task->cancel_mutex);
            if (m_task->status != GCKVIP_TASK_WITH_DATA) {
                gckvip_os_unlock_mutex(m_task->cancel_mutex);
                gckvip_unlock_recursive_mutex(&pm->mutex);
                gckvip_hashmap_unuse(&device->mt_hashmap, queue_data->v1, vip_true_e);
                PRINTK_I("multi-task dev%d task status=0x%x not need submit\n", device_id, m_task->status);
                continue;
            }
            /* submit command buffer */
            status = gckvip_cmd_submit(context, (void*)&m_task->submit_handle);
            m_task->status = GCKVIP_TASK_INFER_START;
            m_task->inference_running = vip_true_e;
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_unlock_recursive_mutex(&pm->mutex);
            if (VIP_SUCCESS == status) {
                wait_data.cmd_handle = m_task->submit_handle.cmd_handle;
                wait_data.time_out = m_task->submit_handle.time_out;
                status = gckvip_cmd_wait(context, &wait_data, device);
                if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
                    PRINTK_E("fail dev%d to wait interrupt in multi-task thread, status=%d, mem_handle=0x%"PRPx"\n",
                             device_id, status, wait_data.cmd_handle);
                }
                PRINTK_I("multi-task dev%d wait task done mem_handle=0x%"PRPx"\n", device_id, wait_data.cmd_handle);
            }
            else {
                PRINTK_E("fail to dev%d submit command mem_handle=0x%"PRPx", physical=0x%08x, size=0x%x, status=%d\n",
                          device_id, m_task->submit_handle.cmd_handle, m_task->submit_handle.cmd_physical,
                          m_task->submit_handle.cmd_size, status);
            }
            m_task->inference_running = vip_false_e;
            m_task->queue_data->v2 = status;
            gckvip_os_lock_mutex(m_task->cancel_mutex);
            /* wait do cancel end */
            m_task->status = GCKVIP_TASK_INFER_END;
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_hashmap_unuse(&device->mt_hashmap, queue_data->v1, vip_true_e);
        }
    }

    /* wake up wait the thread exit task */
    gckvip_os_set_signal(device->mt_thread_signal, vip_true_e);
    PRINTK_I("multi-task thread exit\n");

    return status;
}
#endif

vip_status_e gckvip_cmd_do_wait(
    gckvip_context_t *context,
    gckvip_wait_t *info
    )
{
    vip_status_e status = VIP_SUCCESS;
    void *mem_handle = VIP_NULL;
    gckvip_device_t *device = VIP_NULL;
    vip_uint32_t handle_id = info->handle;
    device = &context->device[info->device_id];

    gcOnError(gckvip_database_get_handle(&context->videomem_database, handle_id, &mem_handle));
    PRINTK_I("user start do wait device%d, mem_handle=0x%"PRPx", timeout=%dms\n",
             info->device_id, mem_handle, info->time_out);

#if vpmdREGISTER_NETWORK
    if (vip_true_e == gckvip_check_segment_skip(mem_handle)) {
        goto Skip;
    }
#endif

#if vpmdENABLE_MULTIPLE_TASK
    {
    gckvip_task_slot_t *m_task = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint32_t index = 0;

    if (vip_false_e == gckvip_os_get_atomic(device->mt_thread_running)) {
        PRINTK_D("bypass wait irq, mt_thread=%d, device_idle=%s, mem_handle=0x%"PRPx"\n",
                  gckvip_os_get_atomic(device->mt_thread_running),
                  (gckvip_os_get_atomic(device->device_idle)) ? "true" : "false", mem_handle);
        return VIP_ERROR_FAILURE;
    }

    gckvip_hashmap_get_by_handle(&device->mt_hashmap, mem_handle, &index, (void**)&m_task);
    if (0 < index) {
        vip_uint32_t wait_time = info->time_out + WAIT_SIGNAL_TIMEOUT; /* user space wait more time than kernel */
        vip_uint32_t cur_wait_time = 0;

        gckvip_os_lock_mutex(m_task->cancel_mutex);
        if (VIP_ERROR_CANCELED == m_task->queue_data->v2) {
            /* the m_task has been canceled.
              1. don't need waiting for task inference done.
              2. don't need clean up mem_handle.
            */
            PRINTK_I("user end do wait device%d, mem_handle=0x%"PRPx", task_status=0x%x\n",
                    info->device_id, mem_handle, m_task->status);
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_hashmap_remove(&device->mt_hashmap, mem_handle);
            gckvip_hashmap_unuse(&device->mt_hashmap, index, vip_false_e);
            return VIP_ERROR_CANCELED;
        }
        gckvip_os_unlock_mutex(m_task->cancel_mutex);

        i = 0;
        while (0 < wait_time) {
            if (wait_time > WAIT_SIGNAL_TIMEOUT) {
                cur_wait_time = WAIT_SIGNAL_TIMEOUT;
                wait_time -= WAIT_SIGNAL_TIMEOUT;
            }
            else {
                cur_wait_time = wait_time;
                wait_time = 0;
            }
            status = gckvip_os_wait_signal(m_task->wait_signal, cur_wait_time);
            if (VIP_SUCCESS == status) {
                break;
            }
            else {
                PRINTK_D("wait task timeout loop index=%d\n", i);
            }
            i++;
            cur_wait_time = 0;
        }
        gckvip_os_set_signal(m_task->wait_signal, vip_false_e);
        if ((status != VIP_SUCCESS) && (0 == cur_wait_time) && (GCKVIP_TASK_INFER_START == m_task->status)) {
        #if (vpmdENABLE_DEBUG_LOG >= 4)
            gckvip_task_slot_t* temp_task = VIP_NULL;
            PRINTK("dump task in submit_mt:\n");
            for (i = 1; i < gckvip_hashmap_free_pos(&device->mt_hashmap); i++) {
                gckvip_hashmap_get_by_index(&device->mt_hashmap, i, (void**)&temp_task);
                if (VIP_NULL == temp_task) {
                    continue;
                }
                if (VIP_NULL != temp_task->submit_handle.cmd_handle) {
                    PRINTK("wait signal=0x%"PRPx", mem_handle=0x%"PRPx"\n", temp_task->wait_signal,
                            temp_task->submit_handle.cmd_handle);
                }
                gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
            }
        #endif
            PRINTK_E("do wait signal mem_id=0x%x, mem_handle=0x%"PRPx", status=%d\n",
                     handle_id, mem_handle, status);
            m_task->submit_handle.cmd_handle = VIP_NULL;
            m_task->status = GCKVIP_TASK_EMPTY;
            gckvip_hashmap_remove(&device->mt_hashmap, mem_handle);
            gckvip_hashmap_unuse(&device->mt_hashmap, index, vip_false_e);
            gcGoOnError(status);
        }
        m_task->submit_handle.cmd_handle = VIP_NULL;
        m_task->status = GCKVIP_TASK_EMPTY;
        status = m_task->queue_data->v2;
        gckvip_hashmap_remove(&device->mt_hashmap, mem_handle);
        gckvip_hashmap_unuse(&device->mt_hashmap, index, vip_false_e);
        if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
            PRINTK_E("do wait mt thread fail mem_id=0x%x, mem_handle=0x%"PRPx", status=%d\n",
                      handle_id, mem_handle, status);
            gcGoOnError(status);
        }
    }
    else {
        PRINTK_D("bypass wait irq, device_idle=%s, mem_handle=0x%"PRPx"\n",
                  (gckvip_os_get_atomic(device->device_idle)) ? "true" : "false", mem_handle);
        #if vpmdENABLE_CANCELATION
        status = VIP_ERROR_CANCELED;
        #endif
    }
    }
#else
    {
    gckvip_wait_cmd_t wait;
    gckvip_task_slot_t *m_task = &device->task_slot;

    gckvip_os_lock_mutex(m_task->cancel_mutex);
    if (GCKVIP_TASK_CANCELED == m_task->status) {
        PRINTK_E("submit is canceled.\n");
        gckvip_os_unlock_mutex(m_task->cancel_mutex);
        gcGoOnError(VIP_ERROR_CANCELED);
    }
    m_task->status = GCKVIP_TASK_USER_WAIT;
    gckvip_os_unlock_mutex(m_task->cancel_mutex);

    wait.mask = info->mask;
    wait.time_out = info->time_out;
    wait.cmd_handle = mem_handle;
    GCKVIP_CHECK_USER_PM_STATUS();
    status = gckvip_cmd_wait(context, &wait, device);
    if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
        PRINTK_E("fail to wait task done, status=%d, mem_handle=0x%"PRPx"\n", status, wait.cmd_handle);
    }
    m_task->status = GCKVIP_TASK_INFER_END;
    }
#endif

#if vpmdREGISTER_NETWORK
Skip:
#endif
    GCKVIP_CHECK_USER_PM_STATUS();

    PRINTK_I("user end do wait device%d, mem_handle=0x%"PRPx"\n", info->device_id, mem_handle);
onError:
    return status;
}

vip_status_e gckvip_cmd_do_submit(
    gckvip_context_t *context,
    gckvip_commit_t *commit
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = VIP_NULL;
    void *cmd_handle = VIP_NULL;
    device = &context->device[commit->device_id];

    GCKVIP_CHECK_USER_PM_STATUS();

    status = gckvip_database_get_handle(&context->videomem_database, commit->cmd_handle, &cmd_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail convert command buffer video memory id, handle=0x%x, status=%d.\n", commit->cmd_handle, status);
        gcGoOnError(status);
    }
    if (VIP_NULL == cmd_handle) {
        PRINTK_E("fail cmd handle is NULL in submit\n");
        gcGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }
    PRINTK_I("user start do submit device%d, cmd size=0x%x, id=0x%x, handle=0x%"PRPx", physical=0x%08x,timeout=%dms\n",
           commit->device_id, commit->cmd_size, commit->cmd_handle, cmd_handle,
           commit->cmd_physical, commit->time_out);

#if vpmdREGISTER_NETWORK
    if (vip_true_e == gckvip_check_segment_skip(cmd_handle)) {
        goto Skip;
    }
#endif

#if vpmdENABLE_MULTIPLE_TASK
    if (vip_true_e == gckvip_os_get_atomic(device->mt_thread_running)) {
        gckvip_task_slot_t *m_task = VIP_NULL;
        vip_bool_e ret = vip_true_e;
        vip_uint32_t i = 0;

        /* delivery the command buffer to multi thread processing */
        /* get a empty multi thread object */
Expand:
        if (vip_true_e == gckvip_hashmap_full(&device->mt_hashmap)) {
            vip_uint32_t capacity = 0;
            vip_bool_e expand = vip_false_e;
            status = gckvip_hashmap_expand(&device->mt_hashmap, &expand);
            if (VIP_SUCCESS != status) {
                PRINTK_E("multi thread is full, not support so many threads\n");
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            if (vip_false_e == expand) {
                goto Expand;
            }
            capacity = gckvip_hashmap_capacity(&device->mt_hashmap);
            for (i = capacity - HASH_MAP_CAPABILITY; i < capacity; i++) {
                gckvip_hashmap_get_by_index(&device->mt_hashmap, i, (void**)&m_task);
                if (VIP_NULL == m_task) {
                    PRINTK_E("fail expend mt hashmap i=%d\n", i);
                    continue;
                }
                status = gckvip_os_create_signal((gckvip_signal*)&m_task->wait_signal);
                if (status != VIP_SUCCESS) {
                    PRINTK_E("failed to create a signal wait signal\n");
                    gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
                    gcGoOnError(VIP_ERROR_IO);
                }
                gckvip_os_set_signal(m_task->wait_signal, vip_false_e);
                status = gckvip_os_create_mutex((gckvip_mutex*)&m_task->cancel_mutex);
                if (status != VIP_SUCCESS) {
                    PRINTK_E("failed to create mutex for cancel task.\n");
                    gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
                    gcGoOnError(VIP_ERROR_IO);
                }
                gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
            }
        }

        /* get an index from task_slot and put the cmd_handle into the task_slot*/
        status = gckvip_hashmap_insert(&device->mt_hashmap, cmd_handle, &i);
        if (VIP_SUCCESS != status) {
            if (VIP_ERROR_OUT_OF_RESOURCE == status) {
                /* multi-thread get the task slot at the same time; need expand */
                goto Expand;
            }
            else {
                PRINTK_E("The duplicate commands 0x%"PRPx" can't be submitted\n", cmd_handle);
                gcGoOnError(status);
            }
        }
        gckvip_hashmap_get_by_index(&device->mt_hashmap, i, (void**)&m_task);
        if (VIP_NULL == m_task) {
            PRINTK_E("fail to get task %d status=%d\n", i);
            gcGoOnError(status);
        }
        if (VIP_NULL == m_task->queue_data) {
            status = gckvip_os_allocate_memory(sizeof(gckvip_queue_data_t), (void**)&m_task->queue_data);
            if (VIP_SUCCESS != status) {
                PRINTK_E("fail to submit, allocate queue_data fail\n");
                gckvip_hashmap_remove(&device->mt_hashmap, m_task->submit_handle.cmd_handle);
                gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            gckvip_os_zero_memory((void*)m_task->queue_data, sizeof(gckvip_queue_data_t));
        }

        gckvip_os_lock_mutex(m_task->cancel_mutex);
        if (vip_true_e == gckvip_hashmap_check_remove(&device->mt_hashmap, i)) {
            PRINTK_E("task has been cancelled during submitting\n");
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
            gckvip_os_free_memory(m_task->queue_data);
            m_task->queue_data = VIP_NULL;
            gcGoOnError(VIP_ERROR_CANCELED);
        }
        m_task->status = GCKVIP_TASK_WITH_DATA;
        m_task->submit_handle.cmd_handle = cmd_handle;
        m_task->submit_handle.cmd_logical = GCKVIPUINT64_TO_PTR(commit->cmd_logical);
        m_task->submit_handle.cmd_physical = commit->cmd_physical;
        m_task->submit_handle.cmd_size = commit->cmd_size;
        m_task->submit_handle.pid = gckvip_os_get_pid();
        m_task->submit_handle.time_out = commit->time_out;
        status = gckvip_database_get_handle(&context->videomem_database, commit->last_cmd_handle,
                                            (vip_ptr *)&m_task->submit_handle.last_cmd_handle);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to convert command buffer video memory id, handle=0x%x\n",
                     commit->last_cmd_handle);
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_hashmap_remove(&device->mt_hashmap, cmd_handle);
            gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
            gckvip_os_free_memory(m_task->queue_data);
            m_task->queue_data = VIP_NULL;
            gcGoOnError(status);
        }
        m_task->submit_handle.last_cmd_logical = GCKVIPUINT64_TO_PTR(commit->last_cmd_logical);
        m_task->submit_handle.last_cmd_physical = commit->last_cmd_physical;
        m_task->submit_handle.last_cmd_size = commit->last_cmd_size;
        m_task->submit_handle.device_id = commit->device_id;
    #if vpmdENABLE_PREEMPTION
        m_task->queue_data->priority = commit->priority;
    #endif
        m_task->queue_data->v1 = i;
        m_task->queue_data->v2 = VIP_ERROR_FAILURE;
        PRINTK_D("user put mem_handle=0x%"PRPx" into task slot index=%d, status=0x%x\n",
                  cmd_handle, i, m_task->status);

        gckvip_os_set_signal(m_task->wait_signal, vip_false_e);
        ret = gckvip_queue_write(&device->mt_input_queue, m_task->queue_data);
        gckvip_os_unlock_mutex(m_task->cancel_mutex);
        if (!ret) {
            PRINTK_E("failed to write task into queue\n");
            m_task->status = GCKVIP_TASK_EMPTY;
            gckvip_hashmap_remove(&device->mt_hashmap, m_task->submit_handle.cmd_handle);
            gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
            gckvip_os_free_memory(m_task->queue_data);
            m_task->queue_data = VIP_NULL;
            gcGoOnError(VIP_ERROR_FAILURE);
        }
        gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
    }
    else {
        PRINTK_E("mutli task thread is not running\n");
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }
#else
    {
        gckvip_task_slot_t *m_task = &device->task_slot;
        #if vpmdONE_POWER_DOMAIN
        gckvip_pm_t* pm = &context->sp_management;
        #else
        gckvip_pm_t* pm = &device->dp_management;
        #endif
        gckvip_os_lock_mutex(m_task->cancel_mutex);
        m_task->status = GCKVIP_TASK_WITH_DATA;
        m_task->submit_handle.cmd_handle = cmd_handle;
        gckvip_os_unlock_mutex(m_task->cancel_mutex);
        m_task->submit_handle.cmd_logical = GCKVIPUINT64_TO_PTR(commit->cmd_logical);
        m_task->submit_handle.cmd_physical = commit->cmd_physical;
        m_task->submit_handle.cmd_size = commit->cmd_size;
        gcOnError(gckvip_database_get_handle(&context->videomem_database,
                                             commit->last_cmd_handle,
                                             &m_task->submit_handle.last_cmd_handle));
        m_task->submit_handle.last_cmd_logical = GCKVIPUINT64_TO_PTR(commit->last_cmd_logical);
        m_task->submit_handle.last_cmd_physical = commit->last_cmd_physical;
        m_task->submit_handle.last_cmd_size = commit->last_cmd_size;
        m_task->submit_handle.device_id = commit->device_id;

        /* make sure other thread can not change power state during submit */
        gckvip_lock_recursive_mutex(&pm->mutex);
        gckvip_os_lock_mutex(m_task->cancel_mutex);
        if (GCKVIP_TASK_CANCELED == m_task->status) {
            PRINTK_E("submit is canceled, not submit.\n");
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_unlock_recursive_mutex(&pm->mutex);
            gcGoOnError(VIP_ERROR_CANCELED);
        }
        status = gckvip_cmd_submit(context, &m_task->submit_handle);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to submit cmd.\n");
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_unlock_recursive_mutex(&pm->mutex);
            gcGoOnError(status);
        }
        m_task->status = GCKVIP_TASK_INFER_START;
        gckvip_os_unlock_mutex(m_task->cancel_mutex);
        gckvip_unlock_recursive_mutex(&pm->mutex);
    }
#endif

#if vpmdREGISTER_NETWORK
Skip:
#endif
    device->user_submit_count++;
    PRINTK_I("user end do submit device%d\n", device->device_id);

    return status;
onError:
    PRINTK_E("fail user to do command submit status=%d\n", status);

    return status;
}

#if vpmdENABLE_CANCELATION
vip_status_e gckvip_hardware_do_cancel(
    gckvip_task_slot_t *m_task,
    gckvip_device_t *device
    )
{
 #define TOTAL_WAIT_COUNT           800
    vip_status_e status = VIP_SUCCESS;
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_uint32_t i = 0, data = 0, orgi_data[gcdMAX_CORE] = {0};
    gckvip_context_t *context = gckvip_get_context();
    vip_int32_t try_count = 0;
    vip_int32_t wait_wakeup_done = TOTAL_WAIT_COUNT;

    PRINTK_I("start do hardware cancel device_id=%d, mem_handle=0x%"PRPx"\n",
              device->device_id, m_task->submit_handle.cmd_handle);
    /* enable cancel */
    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        gcOnError(gckvip_read_register(hardware, 0x003A8, &orgi_data[i]));
        data = ((((vip_uint32_t) (orgi_data[i])) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 3:3) - (0 ?
 3:3) + 1))) - 1)))))) << (0 ?
 3:3))) | (((vip_uint32_t) ((vip_uint32_t) (1) & ((vip_uint32_t) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 3:3) - (0 ? 3:3) + 1))) - 1)))))) << (0 ? 3:3)));
        gcOnError(gckvip_write_register(hardware, 0x003A8, data));
    }
    gckvip_os_udelay(10);
    /* check cancelled */
    for (i = 0; i < device->hardware_count; i++) {
        try_count = TOTAL_WAIT_COUNT;
        hardware = &device->hardware[i];
        /* set job cancel bit to 0 */
        orgi_data[i] = ((((vip_uint32_t) (orgi_data[i])) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 3:3) - (0 ?
 3:3) + 1))) - 1)))))) << (0 ?
 3:3))) | (((vip_uint32_t) ((vip_uint32_t) (0) & ((vip_uint32_t) ((((1 ?
 3:3) - (0 ?
 3:3) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 3:3) - (0 ? 3:3) + 1))) - 1)))))) << (0 ? 3:3)));
        gcOnError(gckvip_write_register(hardware, 0x003A8, orgi_data[i]));
        do {
            gckvip_os_udelay(50);
            gcOnError(gckvip_read_register(hardware, 0x003A8, &data));
            data = (((((vip_uint32_t) (data)) >> (0 ? 4:4)) & ((vip_uint32_t) ((((1 ? 4:4) - (0 ? 4:4) + 1) == 32) ? ~0 : (~0 & ((1 << (((1 ? 4:4) - (0 ? 4:4) + 1))) - 1))))) );
            try_count--;
        } while ((!data) && (try_count > 0));
    }

    if (0 == data) {
        PRINTK_E("fail to cancel hardware%d, cancel_status=%d\n", i, data);
        gcGoOnError(VIP_ERROR_FAILURE);
    }
    gckvip_os_udelay(10);

    /* deal wakeup interrupt for sync mode*/
#if vpmdENABLE_MULTIPLE_TASK
    if (vip_true_e == m_task->inference_running) {
        hardware = &device->hardware[0];
        *hardware->irq_value = 0x10000000;
        gckvip_os_wakeup_interrupt(hardware->irq_queue);
        while(wait_wakeup_done > 0) {
            gckvip_os_udelay(200);
            wait_wakeup_done--;
            if (vip_false_e == m_task->inference_running) {
                try_count = wait_wakeup_done;
                wait_wakeup_done = 0;
            }
        }
        if (vip_true_e == m_task->inference_running) {
            PRINTK_E("fail cancel wait irq done\n");
        }
    }
#else
    if (GCKVIP_TASK_USER_WAIT == m_task->status) {
        hardware = &device->hardware[0];
        *hardware->irq_value = 0x10000000;
        gckvip_os_wakeup_interrupt(hardware->irq_queue);
        while(wait_wakeup_done > 0) {
            gckvip_os_udelay(200);
            wait_wakeup_done--;
            if (GCKVIP_TASK_INFER_END == m_task->status) {
                try_count = wait_wakeup_done;
                wait_wakeup_done = 0;
            }
        }
        if (GCKVIP_TASK_USER_WAIT == m_task->status) {
            PRINTK_E("fail cancel wait irq done\n");
        }
    }
    else {
        m_task->status = GCKVIP_TASK_CANCELED;
    }
#endif
    PRINTK_I("cancel wait irq done, task status=%d, try_count=%d\n",
              m_task->status, TOTAL_WAIT_COUNT-try_count);

    /* start wait-link loop command */
#if vpmdENABLE_WAIT_LINK_LOOP
    for (i = 0; i < device->hardware_count; i++) {
        hardware = &device->hardware[i];
        gckvip_hw_trigger(context, hardware);
        hardware->waitlink_logical = hardware->waitlinkbuf_logical;
    }
#endif
    /* remap vip-sram*/
    gckvip_cmd_submit_init(context, device);

onError:
    return status;
}

vip_status_e gckvip_cmd_do_cancel(
    gckvip_context_t *context,
    gckvip_cancel_t *info
    )
{
    vip_status_e status = VIP_SUCCESS;
    void *mem_handle = VIP_NULL;
    gckvip_device_t *device = VIP_NULL;
    vip_uint32_t handle_id = info->handle;
    vip_bool_e hw_cancel = info->hw_cancel;
    device = &context->device[info->device_id];

    PRINTK_I("user start do cancel on device%d, hw_cancel=%d.\n", info->device_id, hw_cancel);

    if (vip_false_e == hw_cancel) {
        vip_uint32_t i = 0;
        vip_uint32_t wait_time = 300; /* 300ms */
        #if vpmdFPGA_BUILD
        wait_time = 3000;
        #endif
        /* wait hw idle */
        if (vip_false_e == gckvip_os_get_atomic(device->device_idle)) {
            for (i = 0; i < device->hardware_count; i++) {
                gckvip_hw_wait_idle(context, &device->hardware[i], wait_time, vip_true_e);
            }
        }
        PRINTK_I("user end do cancel on device%d, hw_cancel=%d.\n", info->device_id, hw_cancel);
        return status;
    }
    gcOnError(gckvip_database_get_handle(&context->videomem_database, handle_id, &mem_handle));
    PRINTK_I("hw cancel mem_handle=0x%"PRPx"\n", mem_handle);

#if vpmdENABLE_MULTIPLE_TASK
    {
        gckvip_task_slot_t *m_task = VIP_NULL;
        vip_uint32_t i = 0;
        vip_bool_e wake_up_user = vip_false_e;

        if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&device->mt_hashmap, mem_handle, &i, (void**)&m_task)) {
            gckvip_os_lock_mutex(m_task->cancel_mutex);
            m_task->queue_data->v2 = VIP_ERROR_CANCELED;
            if (m_task->status == GCKVIP_TASK_WITH_DATA) {
                if (VIP_SUCCESS == gckvip_queue_clean(&device->mt_input_queue, (void*)&m_task->queue_data->v1)) {
                    /* this task remove from queue */
                    wake_up_user = vip_true_e;
                }
                PRINTK_D("with data cancel task_slot index=%d, task_status=0x%x\n", i, m_task->status);
            }
            else if (m_task->status == GCKVIP_TASK_INFER_START) {
                status = gckvip_hardware_do_cancel(m_task, device);
                if (VIP_SUCCESS != status) {
                    gckvip_os_unlock_mutex(m_task->cancel_mutex);
                    PRINTK_E("hardware cancel job failed status=%d.\n", status);
                    gckvip_hashmap_remove(&device->mt_hashmap, mem_handle);
                    gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
                    gcGoOnError(status);
                }
                PRINTK_D("inference start cancel task_slot index=%d, task_status=0x%x\n", i, m_task->status);
            }
            else if (m_task->status == GCKVIP_TASK_INFER_END) {
                PRINTK_D("inference end cancel task_slot index=%d, task_status=0x%x\n", i, m_task->status);
            }

            m_task->status = GCKVIP_TASK_CANCELED;
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
            gckvip_hashmap_remove(&device->mt_hashmap, mem_handle);
            gckvip_hashmap_unuse(&device->mt_hashmap, i, wake_up_user);
        }
        else {
            PRINTK_D("bypass cancel, the task not in task_slot mem_handle=0x%"PRPx"\n", mem_handle);
        }
    }
#else
    {
        gckvip_task_slot_t *m_task = &device->task_slot;

        if (GCKVIPPTR_TO_UINT32(mem_handle) == GCKVIPPTR_TO_UINT32(m_task->submit_handle.cmd_handle)) {
            gckvip_os_lock_mutex(m_task->cancel_mutex);
            if (m_task->status < GCKVIP_TASK_INFER_START) {
                m_task->status = GCKVIP_TASK_CANCELED;
                PRINTK_I("mem_handle=0x%"PRPx" is marked cancel in queue, not be executed.\n", mem_handle);
            }
            else if (m_task->status == GCKVIP_TASK_INFER_START ||
                     m_task->status == GCKVIP_TASK_USER_WAIT) {
                if (VIP_SUCCESS != gckvip_hardware_do_cancel(m_task, device)) {
                    gckvip_os_unlock_mutex(m_task->cancel_mutex);
                    PRINTK_E("hardware cancel job failed.\n");
                    gcGoOnError(VIP_ERROR_FAILURE);
                }
            }
            else if (m_task->status == GCKVIP_TASK_INFER_END) {
                PRINTK_I("mem_handle=0x%"PRPx" already forward done\n", mem_handle);
            }
            gckvip_os_unlock_mutex(m_task->cancel_mutex);
        }
        else {
            PRINTK_D("bypass cancel, mem_handle=0x%"PRPx"\n", mem_handle);
        }
    }
#endif

    PRINTK_I("user end do cancel device%d, mem_handle=0x%"PRPx"\n", info->device_id, mem_handle);
onError:
    return status;
}
#endif


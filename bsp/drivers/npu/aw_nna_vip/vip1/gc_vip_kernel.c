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
#include <gc_vip_version.h>

/*
INIT_TASK_NUM is the initial number of task supported.  32 is default value when multi-task is enabled.
*/
#if vpmdENABLE_MULTIPLE_TASK > 0
#define INIT_TASK_NUM          32
#else
#define INIT_TASK_NUM          1
#endif

/* Kernel context object. */
gckvip_context_t kContext = {0};
#define GET_CONTEXTK() gckvip_context_t *context = &kContext

#define GCKVIP_CHECK_INIT()            \
{                                      \
    if (context->initialize <= 0) {    \
        PRINTK_E("fail driver not initialize.\n");   \
        return VIP_ERROR_FAILURE;      \
    }                                  \
}

#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
static DEFINE_MUTEX(kinit_mutex);
#endif
#endif

/*
@brief Get kernel context object.
*/
gckvip_context_t* gckvip_get_context(void)
{
    GET_CONTEXTK();
    return context;
}

/*
@brief Get hardware object from core id.
*/
gckvip_hardware_t* gckvip_get_hardware(
    vip_uint32_t core_id
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t j = 0;
    gckvip_hardware_t *hardware = VIP_NULL;
    GET_CONTEXTK();

    for (i = 0; i < context->device_count; i++) {
        gckvip_device_t *device = &context->device[i];
        for (j = 0; j < device->hardware_count; j++) {
            hardware = &device->hardware[j];
            if (core_id == hardware->core_id) {
                return hardware;
            }
        }
    }

    if (i >= context->device_count) {
        PRINTK_E("failed to get core%d hardware object\n", core_id);
    }

    return hardware;
}

/*
@brief Get device object from core id.
*/
gckvip_device_t* gckvip_get_device(
    vip_uint32_t core_id
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t j = 0;
    gckvip_device_t *device = VIP_NULL;
    GET_CONTEXTK();

    for (i = 0; i < context->device_count; i++) {
        device = &context->device[i];
        for (j = 0; j < device->hardware_count; j++) {
            gckvip_hardware_t *hardware = &device->hardware[j];
            if (core_id == hardware->core_id) {
                return device;
            }
        }
    }

    if (i >= context->device_count) {
        PRINTK_E("failed to get core%d device object\n", core_id);
    }

    return VIP_NULL;
}

#if vpmdENABLE_MULTIPLE_TASK
static vip_int32_t gckvip_multi_thread_handle(
    vip_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t ret = 0;
    GET_CONTEXTK();

    status = gckvip_cmd_thread_handle(context, (gckvip_queue_t *)param);
    if (status != VIP_SUCCESS) {
        ret = -1;
    }
    return ret;
}
#endif

static vip_status_e gckvip_device_init(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_hardware_info_t info = {0};
    vip_uint32_t i = 0;
    vip_uint32_t core_cnt = 0;

    status = gckvip_drv_get_hardware_info(&info);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to get hardware information, status=%d.\n", status);
        gcGoOnError(status);
    }

    context->device_count = 0;
    context->core_count = info.core_count;
    context->max_core_count = info.max_core_count;
    /* when mmu enabled, axi_sram_virtual will be converted to VIP virtual in mmu.c */
    context->axi_sram_virtual = info.axi_sram_base;
    context->axi_sram_physical = info.axi_sram_base;
    context->vip_sram_address = info.vip_sram_base;

    PRINTK_D("axi sram address=0x%08x, vipsram address=0x%08x\n",
              context->axi_sram_virtual, context->vip_sram_address);

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    context->heap_user = info.cpu_virtual;
    context->heap_kernel = info.cpu_virtual_kernel;
    context->heap_physical = info.vip_physical;
    context->heap_size = info.vip_memsize;
#endif
#if vpmdENABLE_SYS_MEMORY_HEAP
    context->sys_heap_size = info.sys_heap_size;
#endif
#if vpmdENABLE_RESERVE_PHYSICAL
    context->reserve_phy_base = info.reserve_phy_base;
    context->reserve_phy_size = info.reserve_phy_size;
#endif
    if (context->core_count > gcdMAX_CORE) {
        PRINTK_E("fail to device init, core count %d is more than max %d\n",
                  context->core_count, gcdMAX_CORE);
        gcGoOnError(VIP_ERROR_IO);
    }

    /* caculate device count */
    for (i = 0; i < gcdMAX_CORE; i++) {
        if (info.device_core_number[i] != 0) {
            context->device_count += 1;
            context->axi_sram_size[i] = info.axi_sram_size[i];
            context->vip_sram_size[i] = info.vip_sram_size[i];
            PRINTK_D("core %d axi sram size=0x%08x, vip sram size=0x%x\n",
                i, context->axi_sram_size[i], context->vip_sram_size[i]);
            if (0 != context->axi_sram_size[i] % gcdVIP_MEMORY_ALIGN_SIZE) {
                PRINTK_E("core %d axi sram size=0x%08x is not %d align\n",
                    i, context->axi_sram_size[i], gcdVIP_MEMORY_ALIGN_SIZE);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
        }
        else {
            break;
        }
    }
    PRINTK_D("max_core_count=%d, total core count=%d, total device=%d\n",
             context->max_core_count, context->core_count, context->device_count);

    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_device_t) * context->device_count,
                                       (void**)&context->device));
    gckvip_os_zero_memory(context->device, sizeof(gckvip_device_t) * context->device_count);

    for (i = 0; i < context->device_count; i++) {
        vip_uint32_t j = 0;
        gckvip_device_t *device = &context->device[i];

        device->device_id = i;
        device->hardware_count = info.device_core_number[i];
        status = gckvip_os_create_atomic(&device->core_fscale_percent);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to create atomic for device=%d core frequency\n", i);
            gcGoOnError(VIP_ERROR_IO);
        }
        status = gckvip_os_create_atomic(&device->last_fscale_percent);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to create atomic for device=%d last core frequency\n", i);
            gcGoOnError(VIP_ERROR_IO);
        }

        if ((info.core_fscale_percent > 0) && (info.core_fscale_percent <= 100)) {
            gckvip_os_set_atomic(device->core_fscale_percent, info.core_fscale_percent);
        }
        else {
            /* default full clock freq */
            gckvip_os_set_atomic(device->core_fscale_percent, 100);
        }
        #if vpmdENABLE_RECOVERY
        device->recovery_times = MAX_RECOVERY_TIMES;
        #endif
        #if vpmdENABLE_HANG_DUMP
        device->dump_finish = vip_false_e;
        #endif
        PRINTK_D("device_id=%d, this device core count=%d, core_fscale_percent=%d\n",
                 i, device->hardware_count, device->core_fscale_percent);
        gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_hardware_t) * device->hardware_count,
                                           (void**)&device->hardware));
        gckvip_os_zero_memory(device->hardware, sizeof(gckvip_hardware_t) * device->hardware_count);
        for (j = 0; j < device->hardware_count; j++) {
            gckvip_hardware_t *hardware = &device->hardware[j];
            hardware->core_id = core_cnt;
            hardware->reg_base = info.vip_reg[core_cnt];
            hardware->irq_queue = info.irq_queue[core_cnt];
            hardware->irq_value = info.irq_value[core_cnt];
            #if vpmdENABLE_MULTIPLE_TASK
            status = gckvip_os_create_mutex(&hardware->reg_mutex);
            if (status != VIP_SUCCESS) {
                PRINTK_E("failed to create mutex for register memory\n");
                gcGoOnError(VIP_ERROR_IO);
            }
            #endif
            core_cnt++;
            if (core_cnt > context->core_count) {
                PRINTK_E("fail to device init, core count=%d overflow\n", core_cnt);
                gcOnError(VIP_ERROR_FAILURE);
            }
            PRINTK_D("  core_id=%d\n", hardware->core_id);
        }

        #if vpmdREGISTER_NETWORK
        gcOnError(gckvip_os_allocate_memory(sizeof(void*) * PREVIOUS_NET_COUNT,
            (void**)&device->previous_handles));
        gckvip_os_zero_memory(device->previous_handles, sizeof(void*) * PREVIOUS_NET_COUNT);
        #endif
    }

onError:
    return status;
}

static vip_status_e gckvip_device_uninit(
    gckvip_context_t *context
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t j = 0;

    for (i = 0; i < context->device_count; i++) {
        gckvip_device_t *device = &context->device[i];
        if (device->core_fscale_percent != VIP_NULL) {
            gckvip_os_destroy_atomic(device->core_fscale_percent);
            device->core_fscale_percent = VIP_NULL;
        }
        if (device->last_fscale_percent != VIP_NULL) {
            gckvip_os_destroy_atomic(device->last_fscale_percent);
            device->last_fscale_percent = VIP_NULL;
        }
        if (VIP_NULL == device->hardware) {
            PRINTK_E("fail to uninit device%d hardware\n", i);
            continue;
        }
        for (j = 0; j < device->hardware_count; j++) {
            gckvip_hardware_t *hardware = &device->hardware[j];
            if (hardware != VIP_NULL) {
                #if vpmdENABLE_MULTIPLE_TASK
                if (hardware->reg_mutex != VIP_NULL) {
                    gckvip_os_destroy_mutex(hardware->reg_mutex);
                }
                #endif
            }
        }
        gckvip_os_free_memory(device->hardware);
        device->hardware = VIP_NULL;
        #if vpmdREGISTER_NETWORK
        if (VIP_NULL != device->previous_handles) {
            gckvip_os_free_memory(device->previous_handles);
            device->previous_handles = VIP_NULL;
        }
        #endif
    }
    if (context->device != VIP_NULL) {
        gckvip_os_zero_memory(context->device, sizeof(gckvip_device_t));
        gckvip_os_free_memory(context->device);
        context->device = VIP_NULL;
    }

    return VIP_SUCCESS;
}

#if vpmdENABLE_MULTIPLE_TASK
static vip_status_e wake_up_user_wait(
    void* element
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_task_slot_t* m_task = (gckvip_task_slot_t*)element;
    gckvip_os_set_signal(m_task->wait_signal, vip_true_e);
    return status;
}
#endif

static vip_status_e gckvip_context_init(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t index = 0;

#if vpmdENABLE_DEBUG_LOG > 3
    gckvip_dump_options();
#endif
    gcOnError(gckvip_hashmap_init(&context->process_id, INIT_TASK_NUM, 0, "process ID", VIP_NULL));
    gckvip_hashmap_insert(&context->process_id, GCKVIPUINT64_TO_PTR(gckvip_os_get_pid()), &index);
    status = gckvip_os_create_atomic(&context->init_done);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for init done flag\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(context->init_done, vip_false_e);

    status = gckvip_device_init(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to device init status=%d\n", status);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_create_mutex(&context->flush_cache_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create mutex for flush cache mutex\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    GCKVIP_LOOP_DEVICE_START
    vip_char_t name[256];
    gckvip_os_snprint(name, 255, "device_thread_%d", dev_index);
    device->device_id = dev_index;
    status = gckvip_os_create_signal(&device->mt_thread_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create a signal multiple thread\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    status = gckvip_queue_initialize(&device->mt_input_queue, device->device_id);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create input queue for supporting multi-thread\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    status = gckvip_os_create_atomic(&device->mt_thread_running);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for mt thread running\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    status = gckvip_os_create_thread((gckvip_thread_func)gckvip_multi_thread_handle,
                                    (vip_ptr)&device->mt_input_queue, name,
                                    &device->mt_thread_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create a thread for supporting multi-thread\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    GCKVIP_LOOP_DEVICE_END
    {
    vip_uint32_t i = 0, j = 0;
    for (j = 0; j < context->device_count; j++) {
        gckvip_device_t *device = &context->device[j];
        gcOnError(gckvip_hashmap_init(&device->mt_hashmap, INIT_TASK_NUM,
            sizeof(gckvip_task_slot_t), "task slot", (void*)wake_up_user_wait));
        PRINTK_D("device%d, mt_hashmap=0x%"PRPx"\n", j, &device->mt_hashmap);
        for (i = 1; i < gckvip_hashmap_capacity(&device->mt_hashmap); i++) {
            gckvip_task_slot_t* m_task = VIP_NULL;
            gcOnError(gckvip_hashmap_get_by_index(&device->mt_hashmap, i, (void**)&m_task));
            status = gckvip_os_create_signal((gckvip_signal*)&m_task->wait_signal);
            if (status != VIP_SUCCESS) {
                PRINTK_E("failed to create a signal wait_signal\n");
                gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
                gcGoOnError(VIP_ERROR_IO);
            }
            gckvip_os_set_signal(m_task->wait_signal, vip_false_e);
            status = gckvip_os_create_mutex((gckvip_mutex *)&m_task->cancel_mutex);
            if (status != VIP_SUCCESS) {
                PRINTK_E("failed to create mutex for cancel task.\n");
                gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
                gcGoOnError(VIP_ERROR_IO);
            }
            m_task->status = GCKVIP_TASK_EMPTY;
            gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
        }
    }
    }
    PRINTK_D("multi-task is enabled...\n");
#else
    GCKVIP_LOOP_DEVICE_START
    status = gckvip_os_create_mutex((gckvip_mutex *)&device->task_slot.cancel_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create mutex for cancel task.\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    GCKVIP_LOOP_DEVICE_END
#endif

    gcOnError(gckvip_hashmap_init(&context->wrap_hashmap, INIT_WRAP_USER_MEMORY_NUM, 0, "memory wrap", VIP_NULL));
#if vpmdREGISTER_NETWORK
    gcOnError(gckvip_hashmap_init(&context->network_info, HASH_MAP_CAPABILITY,
        sizeof(gckvip_network_info_t), "network information", VIP_NULL));
    gcOnError(gckvip_hashmap_init(&context->segment_info, HASH_MAP_CAPABILITY,
        sizeof(gckvip_segment_info_t), "segment information", VIP_NULL));
#endif

    GCKVIP_LOOP_DEVICE_START
    status = gckvip_os_create_atomic(&device->device_idle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for vip idle\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(device->device_idle, vip_true_e);

#if vpmdENABLE_CLOCK_SUSPEND
    status = gckvip_os_create_atomic(&device->disable_clock_suspend);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for disable clock suspend\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(device->disable_clock_suspend, 0);
    status = gckvip_os_create_atomic(&device->clock_suspended);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for flag clock suspended\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_set_atomic(device->clock_suspended, vip_false_e);
#endif
#if !vpmdONE_POWER_DOMAIN
    status = gckvip_os_create_atomic(&device->dp_management.status);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for power status\n");
        gcGoOnError(VIP_ERROR_IO);
    }
#endif
    GCKVIP_LOOP_DEVICE_END

#if vpmdONE_POWER_DOMAIN
    status = gckvip_os_create_atomic(&context->sp_management.status);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for power status\n");
        gcGoOnError(VIP_ERROR_IO);
    }
#endif

    /* 5. initialize video memory data base */
    status = gckvip_database_init(&context->videomem_database);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize video memory database\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    status = gckvip_os_create_mutex(&context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create mutex for video memory\n");
        gcGoOnError(VIP_ERROR_IO);
    }
#if vpmdENABLE_DEBUGFS
    status = gckvip_debug_set_profile_data(gckvip_os_get_time(), PROFILE_INIT);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail record cur init time\n");
    }
#endif

onError:
    return status;
}

static vip_status_e gckvip_context_uninit(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_MULTIPLE_TASK
    vip_uint32_t i = 0;
    if (context->flush_cache_mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(context->flush_cache_mutex);
    }
    GCKVIP_LOOP_DEVICE_START
    if (device->mt_thread_handle != VIP_NULL) {
        gckvip_os_destroy_thread(device->mt_thread_handle);
    }
    gckvip_queue_destroy(&device->mt_input_queue);
    if (device->mt_thread_signal != VIP_NULL) {
        gckvip_os_destroy_signal(device->mt_thread_signal);
    }

    for (i = 1; i < gckvip_hashmap_capacity(&device->mt_hashmap); i++) {
        gckvip_task_slot_t* m_task = VIP_NULL;
        gckvip_hashmap_get_by_index(&device->mt_hashmap, i, (void**)&m_task);
        if (VIP_NULL == m_task) {
            continue;
        }
        gckvip_os_destroy_signal(m_task->wait_signal);
        if (VIP_NULL != m_task->queue_data) {
            gckvip_os_free_memory((void*)m_task->queue_data);
            m_task->queue_data = VIP_NULL;
        }
        if (m_task->cancel_mutex != VIP_NULL) {
            gckvip_os_destroy_mutex(m_task->cancel_mutex);
            m_task->cancel_mutex = VIP_NULL;
        }
        gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
    }
    gckvip_hashmap_destroy(&device->mt_hashmap);
    if (device->mt_thread_running != VIP_NULL) {
        gckvip_os_destroy_atomic(device->mt_thread_running);
        device->mt_thread_running = VIP_NULL;
    }
    GCKVIP_LOOP_DEVICE_END
#else
    GCKVIP_LOOP_DEVICE_START
    if (device->task_slot.cancel_mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(device->task_slot.cancel_mutex);
        device->task_slot.cancel_mutex = VIP_NULL;
    }
    GCKVIP_LOOP_DEVICE_END
#endif

    gckvip_hashmap_destroy(&context->wrap_hashmap);
#if vpmdREGISTER_NETWORK
    gckvip_hashmap_destroy(&context->network_info);
    gckvip_hashmap_destroy(&context->segment_info);
#endif
    GCKVIP_LOOP_DEVICE_START
    if (device->device_idle != VIP_NULL) {
        gckvip_os_destroy_atomic(device->device_idle);
        device->device_idle = VIP_NULL;
    }

#if vpmdENABLE_CLOCK_SUSPEND
    if (device->disable_clock_suspend != VIP_NULL) {
        gckvip_os_destroy_atomic(device->disable_clock_suspend);
        device->disable_clock_suspend = VIP_NULL;
    }
    if (device->clock_suspended != VIP_NULL) {
        gckvip_os_destroy_atomic(device->clock_suspended);
        device->clock_suspended = VIP_NULL;
    }
#endif
#if !vpmdONE_POWER_DOMAIN
    if (device->dp_management.status != VIP_NULL) {
        gckvip_os_destroy_atomic(device->dp_management.status);
    }
#endif
    GCKVIP_LOOP_DEVICE_END
#if vpmdONE_POWER_DOMAIN
    if (context->sp_management.status != VIP_NULL) {
        gckvip_os_destroy_atomic(context->sp_management.status);
    }
#endif
    if (context->init_done != VIP_NULL) {
        gckvip_os_destroy_atomic(context->init_done);
    }

    if (context->memory_mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(context->memory_mutex);
    }
    gckvip_hashmap_destroy(&context->process_id);
    gckvip_database_uninit(&context->videomem_database);
    gckvip_device_uninit(context);

    return status;
}

/*
@brief initialize hardware and all resource.
*/
static vip_status_e do_init(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_initialize_t *init_data = (gckvip_initialize_t *)data;
    vip_uint32_t pid = gckvip_os_get_pid();
    gckvip_pm_t *pm = VIP_NULL;
    GET_CONTEXTK();

    if (init_data->version != ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_SUB_MINOR))) {
        PRINTK_E("lib version mismatch, vipuser.so=0x%08x, vipcore.ko=0x%08x.\n",
            init_data->version, (VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_SUB_MINOR));
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
    mutex_lock(&kinit_mutex);
#endif
    if (VIP_NULL == context->initialize_mutex) {
        PRINTK_D("start create initialize mutex ...\n");
        gckvip_os_zero_memory(context, sizeof(gckvip_context_t));
        status = gckvip_os_create_mutex(&context->initialize_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to create mutex for initialize\n");
            gcGoOnError(VIP_ERROR_IO);
        }
    }
    gckvip_os_lock_mutex(context->initialize_mutex);

    if (context->initialize >= 1) {
        vip_uint32_t index = 0;
        /* Mark initialized. */
        if (vip_true_e == gckvip_hashmap_full(&context->process_id)) {
            vip_bool_e expand = vip_false_e;
            status = gckvip_hashmap_expand(&context->process_id, &expand);
            if (VIP_SUCCESS != status) {
                PRINTK_E("failed to initialize, task id is full.\n");
                gcGoOnError(VIP_ERROR_IO);
            }
        }
        gckvip_hashmap_insert(&context->process_id, GCKVIPUINT64_TO_PTR(pid), &index);
        context->initialize++;
        PRINTK_D("vipcore has been initialized num=%d, pid=%x\n", context->initialize, pid);

        #if (vpmdENABLE_DEBUG_LOG >= 4)
        {
            vip_uint32_t i = 0;
            void* handle = VIP_NULL;
            PRINTK("pids: \n");
            for (i = 1; i < gckvip_hashmap_free_pos(&context->process_id); i++) {
                gckvip_hashmap_get_by_index(&context->process_id, i, &handle);
                if (VIP_NULL != handle) {
                    PRINTK("index=%d, pid=%x\n", i, handle);
                    gckvip_hashmap_unuse(&context->process_id, i, vip_false_e);
                }
            }
        }
        #endif

        gckvip_os_unlock_mutex(context->initialize_mutex);
        #if defined(LINUX)
        mutex_unlock(&kinit_mutex);
        #endif

        return VIP_SUCCESS;
    }
#else
    if (context->initialize >= 1) {
        PRINTK_E("vipcore has been initialized, pid=%x\n", pid);
        status = VIP_ERROR_NOT_SUPPORTED;
        gcGoOnError(status);
    }
    gckvip_os_zero_memory(context, sizeof(gckvip_context_t));
#endif
    /* set default value of capture function */
    context->func_config.enable_capture = vip_true_e;
    context->func_config.enable_cnn_profile = vip_true_e;
    context->func_config.enable_dump_nbg = vip_true_e;

    /* 1. Initialize OS layer, driver layer and power on hardware. */
    status = gckvip_os_init(init_data->video_mem_size);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to os initialize, status=%d.\n", status);
        gcGoOnError(status);
    }

    /* 2. init resource resource */
    status = gckvip_context_init(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize context resource, status=%d.\n", status);
        gcGoOnError(status);
    }

    /* 3. Initialize power managment */
    status = gckvip_pm_init(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to os initialize power management, status=%d.\n", status);
        gcGoOnError(status);
    }

    /* 4. Initialize video memory heap */
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    status = gckvip_mem_heap_init(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize video memory heap\n");
        gcGoOnError(VIP_ERROR_IO);
    }
#endif
#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
    gcOnError(gckvip_register_dump_init(context));
#endif

    /* Read hardware information */
    gckvip_hw_read_info(context);

    /* 5. Reset hardware. */
    GCKVIP_LOOP_DEVICE_START
    status = gckvip_hw_reset(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to software reset hardware, status=%d.\n", status);
        gcGoOnError(status);
    }
    GCKVIP_LOOP_DEVICE_END

    /* 6. Init MMU page table */
#if vpmdENABLE_MMU
    status = gckvip_mmu_init(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize MMU, status=%d.\n", status);
        gcGoOnError(status);
    }
#endif

    /* 7. Init hardware */
    GCKVIP_LOOP_DEVICE_START
    status = gckvip_cmd_prepare_init(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to prepare init command\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    status = gckvip_hw_init(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to hardware initialize, status=%d.\n", status);
        gcGoOnError(status);
    }
#if vpmdONE_POWER_DOMAIN
    pm = &context->sp_management;
#else
    pm = &device->dp_management;
#endif
    gckvip_os_set_atomic(pm->status, GCKVIP_POWER_IDLE);
#if vpmdPOWER_OFF_TIMEOUT
    gckvip_os_stop_timer(pm->timer);
    gckvip_os_start_timer(pm->timer, 100 * 1000);
#endif
    GCKVIP_LOOP_DEVICE_END

#if vpmdENABLE_DEBUG_LOG > 3
    gckvip_report_clock();
#endif

#if !vpmdENABLE_WAIT_LINK_LOOP
    /* entry low power */
    GCKVIP_LOOP_DEVICE_START
    #if vpmdENABLE_CLOCK_SUSPEND
    gckvip_pm_disable_clock(device);
    #else
    gckvip_pm_set_frequency(device, 1);
    #endif
    GCKVIP_LOOP_DEVICE_END
#endif
    context->initialize = 1;
    PRINTK_D("do init done initialize num=%d pid=0x%x.\n", context->initialize, pid);

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_unlock_mutex(context->initialize_mutex);
    }
    else {
        PRINTK_E("error initialize mutex is NULL in do init\n");
    }
#if defined(LINUX)
    mutex_unlock(&kinit_mutex);
#endif
#endif

    return status;
}

static vip_status_e gckvip_wait_device_idle(
    gckvip_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    GET_CONTEXTK();
#if vpmdONE_POWER_DOMAIN
    if (GCKVIP_POWER_OFF != gckvip_os_get_atomic(context->sp_management.status)) {
#else
    if (GCKVIP_POWER_OFF != gckvip_os_get_atomic(device->dp_management.status)) {
#endif
        if (vip_false_e == gckvip_os_get_atomic(device->device_idle)) {
        #if vpmdENABLE_CLOCK_SUSPEND
            gckvip_pm_enable_clock(device);
        #endif
            for (i = 0; i < device->hardware_count; i++) {
                gckvip_hardware_t *hardware = &device->hardware[i];
                /* hardware is idle status, power off hardware immediately */
                status |= gckvip_hw_wait_idle(context, hardware, 1000, vip_false_e);
            }
        }
        if (status != VIP_SUCCESS) {
            PRINTK_E("error, VIP not going to idle.\n");
        }
    }
    return status;
}

/*
@brief destroy resource.
*/
static vip_status_e do_destroy(void)
{
    vip_status_e status = VIP_SUCCESS;
    GET_CONTEXTK();

#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
    mutex_lock(&kinit_mutex);
#endif
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_lock_mutex(context->initialize_mutex);
    }
    else {
        PRINTK_E("error initialize mutex is NULL in do destory\n");
    }
#endif

    /* Mark uninitialized. */
#if vpmdENABLE_MULTIPLE_TASK
    if (VIP_SUCCESS == gckvip_hashmap_remove(&context->process_id, GCKVIPUINT64_TO_PTR(gckvip_os_get_pid()))) {
        context->initialize--;
    }
    else {
        PRINTK_E("destroy can't found process_id=0x%x. pids:\n", gckvip_os_get_pid());
    }
    if (context->initialize <= 0) {
#else
    if (context->initialize > 0) {
        context->initialize = 0;
        gckvip_hashmap_remove(&context->process_id, GCKVIPUINT64_TO_PTR(gckvip_os_get_pid()));
#endif
        PRINTK_D("start to destroy vip pid=0x%x..\n", gckvip_os_get_pid());
    #if vpmdENABLE_CLOCK_SUSPEND
        GCKVIP_LOOP_DEVICE_START
        gckvip_os_set_atomic(device->disable_clock_suspend, 0xDEADBEEF);
        GCKVIP_LOOP_DEVICE_END
    #endif

    #if vpmdENABLE_MULTIPLE_TASK
        GCKVIP_LOOP_DEVICE_START
        gckvip_os_set_atomic(device->mt_thread_running, vip_false_e);
        device->mt_input_queue.queue_stop = vip_true_e;
        /* exit multi-thread process */
        gckvip_os_set_signal(device->mt_input_queue.read_signal, vip_true_e);
        gckvip_os_wait_signal(device->mt_thread_signal, WAIT_SIGNAL_TIMEOUT);
        GCKVIP_LOOP_DEVICE_END
    #endif

        /* wait hardware idle */
        GCKVIP_LOOP_DEVICE_START
        if (device->init_command.handle != VIP_NULL) {
            gckvip_database_put_id(&context->videomem_database, device->init_command.handle_id);
            gckvip_mem_free_videomemory(context, device->init_command.handle);
            device->init_command.handle = VIP_NULL;
        }
    #if vpmdPOWER_OFF_TIMEOUT
        /* power thread exit */
        #if !vpmdONE_POWER_DOMAIN
        gckvip_os_set_atomic(device->dp_management.thread_running, vip_false_e);
        gckvip_os_set_signal(device->dp_management.signal, vip_true_e);
        #endif
    #endif
        status = gckvip_wait_device_idle(device);
        if (status != VIP_SUCCESS) {
            PRINTK_E("device%d not going to idle\n", dev_index);
        }
        GCKVIP_LOOP_DEVICE_END

    #if vpmdPOWER_OFF_TIMEOUT
        /* power thread exit */
        #if vpmdONE_POWER_DOMAIN
        gckvip_os_set_atomic(context->sp_management.thread_running, vip_false_e);
        gckvip_os_set_signal(context->sp_management.signal, vip_true_e);
        #endif
    #endif

        /* has free video memory in destroy function, should be call before mmu uninit */
        status = gckvip_hw_destroy(context);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to destroy hardware\n");
        }

    #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        gckvip_register_dump_uninit(context);
    #endif

    #if vpmdENABLE_MMU
        status = gckvip_mmu_uninit(context);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to uninit mmu\n");
        }
    #endif

    #if vpmdENABLE_VIDEO_MEMORY_HEAP
        status = gckvip_mem_heap_destroy(context);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to destroy video memory heap\n");
        }
    #endif

        /* uninitialize power management*/
        gckvip_pm_uninit(context);

        gckvip_context_uninit(context);

        status = gckvip_os_close();
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to destroy os\n");
        }

    #if vpmdENABLE_MULTIPLE_TASK
        /* unlock and destroy initialize mutex last */
        if (context->initialize_mutex != VIP_NULL) {
            gckvip_os_unlock_mutex(context->initialize_mutex);
            PRINTK_D("destroy initialize mutex\n");
            gckvip_os_destroy_mutex(context->initialize_mutex);
            context->initialize_mutex = VIP_NULL;
        }
    #endif
        gckvip_os_zero_memory(context, sizeof(gckvip_context_t));
        PRINTK_I("end to destroy vip..\n");
    }
#if vpmdENABLE_MULTIPLE_TASK
    else {
        if (context->initialize_mutex != VIP_NULL) {
            gckvip_os_unlock_mutex(context->initialize_mutex);
        }
        else {
            PRINTK_E("error initialize mutex is NULL in do destory end\n");
        }
    }
#if defined(LINUX)
    mutex_unlock(&kinit_mutex);
#endif
#endif

    return status;
}

/*
@brief Read a register.
*/
static vip_status_e do_read_register(void *data)
{
    gckvip_reg_t *reg_data = (gckvip_reg_t *)data;
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();
    if (VIP_NULL == data) {
        PRINTK_E("failed to read register, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }
#if vpmdPOWER_OFF_TIMEOUT
    {
    gckvip_device_t* device = gckvip_get_device(reg_data->core);
    status = gckvip_pm_check_power(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to check power in read reg status=%d\n", status);
        gcGoOnError(status);
    }
    }
#endif
    hardware = gckvip_get_hardware(reg_data->core);
#if vpmdENABLE_CLOCK_SUSPEND
    status = gckvip_pm_enable_hardware_clock(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to enable hardare%d clock\n", hardware->core_id);
        gcGoOnError(status);
    }
#endif
    status = gckvip_read_register(hardware, reg_data->reg, &reg_data->data);
#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_pm_disable_hardware_clock(hardware);
#endif
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to read hardware register status=%d\n", status);
        gcGoOnError(status);
    }
onError:
    return status;
}

/*
@brief Write a register.
*/
static vip_status_e do_write_register(void *data)
{
    gckvip_reg_t *reg_data = (gckvip_reg_t *)data;
    gckvip_hardware_t *hardware = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();
    if (VIP_NULL == data) {
        PRINTK_E("failed to write register, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }
#if vpmdPOWER_OFF_TIMEOUT
    {
    gckvip_device_t* device = gckvip_get_device(reg_data->core);
    status = gckvip_pm_check_power(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to check power in write reg status=%d\n", status);
        gcGoOnError(status);
    }
    }
#endif
    hardware = gckvip_get_hardware(reg_data->core);
#if vpmdENABLE_CLOCK_SUSPEND
    status = gckvip_pm_enable_hardware_clock(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to enable hardare%d clock\n", hardware->core_id);
        gcGoOnError(status);
    }
#endif
    status = gckvip_write_register(hardware, reg_data->reg, reg_data->data);
#if vpmdENABLE_CLOCK_SUSPEND
    gckvip_pm_disable_hardware_clock(hardware);
#endif
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to write hardware register\n");
        gcGoOnError(status);
    }
onError:
    return status;
}

/*
@brief free video memory
when MMU disable, allocate video memory from heap. And return physcical address.
when MM enable, allocate video memory from heap and system. return VIP's virtual address.
*/
static vip_status_e do_allocation_videomem(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_allocation_t *allocation = (gckvip_allocation_t *)data;
    vip_uint32_t alloc_flag = allocation->alloc_flag;
    gckvip_video_memory_t memory = {0};
    phy_address_t physical_tmp = 0;
    vip_uint32_t handle_id = 0;
    void *logical = VIP_NULL;
    vip_bool_e vip_virtual = (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE) ? vip_false_e : vip_true_e;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    status = gckvip_mem_allocate_videomemory(context, allocation->size, &memory.handle,
                                             &memory.logical, &physical_tmp,
                                             allocation->align, alloc_flag);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate video memory in allocation videomem, size=0x%x, status=%d\n",
                  allocation->size, status);
        gcGoOnError(status);
    }
    memory.physical = physical_tmp;

    /* the user logical address will be converted in user space for multiple-process */
    if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MAP_USER) {
        status |= gckvip_mem_get_userlogical(context, memory.handle, memory.physical, vip_virtual, &logical);
    }
    status |= gckvip_database_get_id(&context->videomem_database, memory.handle,
                                     gckvip_os_get_pid(), &handle_id);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to get handle id for allocate video memory, size=%d\n", allocation->size);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    allocation->logical = GCKVIPPTR_TO_UINT64(logical);
    allocation->handle = handle_id;
    allocation->physical = memory.physical;

    PRINTK_D("allocate video memory size=0x%x, align=0x%x, type=0x%x, handle=0x%x\n",
              allocation->size, allocation->align, allocation->alloc_flag, handle_id);

    return status;

onError:
    allocation->logical = 0;
    allocation->physical = 0;
    allocation->handle = 0;

    return status;
}

/*
@brief free video memory
*/
static vip_status_e do_free_videomem(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_deallocation_t *allocation = (gckvip_deallocation_t *)data;
    void *handle = VIP_NULL;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    gcOnError(gckvip_database_get_handle(&context->videomem_database, allocation->handle, &handle));

    PRINTK_D("free video memory handle=0x%x, 0x%"PRPx"\n", allocation->handle, handle);

    status = gckvip_mem_free_videomemory(context, handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to free video memory handle=0x%x, 0x%"PRPx"\n", allocation->handle, handle);
        gcGoOnError(status);
    }

    gcOnError(gckvip_database_put_id(&context->videomem_database, allocation->handle));

onError:
    return status;
}

/*
@brief mapping user space logical address.
*/
static vip_status_e do_map_user_logical(void *data)
{
    gckvip_map_user_logical_t *map = (gckvip_map_user_logical_t*)data;
    vip_status_e status = VIP_SUCCESS;
    void *handle = VIP_NULL;
    void *logical = VIP_NULL;
    GET_CONTEXTK();

    gcOnError(gckvip_database_get_handle(&context->videomem_database, map->handle, &handle));
    gcOnError(gckvip_mem_map_userlogical(context, handle, map->physical, map->vip_virtual, &logical));
    map->logical = GCKVIPPTR_TO_UINT64(logical);

    PRINTK_I("map user logical=0x%"PRIx64", physical=0x%08x, handle=0x%"PRPx"\n",
              map->logical, map->physical, handle);

    return status;
onError:
    PRINTK_E("fail to map handle=0x%x user logical\n", map->handle);
    return status;
}

/*
@brief unmapping user space logical address.
*/
static vip_status_e do_unmap_user_logical(void *data)
{
    gckvip_unmap_user_logical_t *unmap = (gckvip_unmap_user_logical_t*)data;
    vip_status_e status = VIP_SUCCESS;
    void *handle = VIP_NULL;
    GET_CONTEXTK();

    gcOnError(gckvip_database_get_handle(&context->videomem_database, unmap->handle, &handle));
    gcOnError(gckvip_mem_unmap_userlogical(context, handle));

    return status;
onError:
    PRINTK_E("fail to unmap handle=0x%x user logical\n", unmap->handle);
    return status;
}

/*
@brief software rest hardware.
*/
static vip_status_e do_reset_vip(void)
{
    vip_status_e status = VIP_SUCCESS;
    GET_CONTEXTK();

    GCKVIP_LOOP_DEVICE_START
    status = gckvip_hw_reset(context, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to reset vip\n");
        return VIP_ERROR_NOT_SUPPORTED;
    }
    GCKVIP_LOOP_DEVICE_END

    return status;
}

static vip_status_e do_wait(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_wait_t *info = (gckvip_wait_t *)data;
    GET_CONTEXTK();
    gcIsNULL(info);
    GCKVIP_CHECK_DEV(info->device_id);
    GCKVIP_CHECK_INIT();

    status = gckvip_cmd_do_wait(context, info);
    if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
        PRINTK_E("fail to do cmd wait, status=%d.\n", status);
    }

onError:
    return status;
}

#if vpmdENABLE_CANCELATION
static vip_status_e do_cancel(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_cancel_t *info = (gckvip_cancel_t *)data;
    GET_CONTEXTK();
    gcIsNULL(info);
    GCKVIP_CHECK_DEV(info->device_id);
    GCKVIP_CHECK_INIT();

    status = gckvip_cmd_do_cancel(context, info);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to do cmd cancel, status=%d.\n", status);
    }

onError:
    return status;
}
#endif

static vip_status_e do_submit(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_commit_t *commit = (gckvip_commit_t *)data;
    GET_CONTEXTK();
    gcIsNULL(commit);
    GCKVIP_CHECK_DEV(commit->device_id);
    GCKVIP_CHECK_INIT();

    status = gckvip_cmd_do_submit(context, commit);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to do cmd submit handle=0x%x, status=%d.\n", commit->cmd_handle, status);
        gcGoOnError(status);
    }

onError:
    return status;
}

/*
@brief query wait-link address info for agent driver used.
*/
#if vpmdENABLE_USE_AGENT_TRIGGER
static vip_status_e do_query_address_info(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_query_address_info_t *info = (gckvip_query_address_info_t*)data;

#if vpmdENABLE_WAIT_LINK_LOOP
    void *logical = VIP_NULL;
    GET_CONTEXTK();
    gckvip_hardware_t *hardware = gckvip_get_hardware(0);

    /* get user space logical address of wait/link buffer */
    gckvip_mem_get_userlogical(context, hardware->waitlinkbuf_handle,
                               hardware->waitlinkbuf_physical, (void**)&logical);
    info->waitlink_logical = GCKVIPPTR_TO_UINT64(logical);
    info->waitlink_physical = hardware->waitlinkbuf_physical;
    info->waitlink_size = WAIT_LINK_SIZE;

    PRINTK_D("wait link buffer physical=0x%08X,  user space logical=0x%"PRIx64"\n",
              info->waitlink_physical, info->waitlink_logical);
#else
    info->waitlink_logical = 0;
    info->waitlink_size = 0;
    info->waitlink_physical = 0;
#endif

    return status;
}
#endif

/*
@brief query hardware database
*/
static vip_status_e do_query_database(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t iter0 = 0, iter1 = 0;
    gckvip_query_database_t *database = (gckvip_query_database_t*)data;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_lock_mutex(context->initialize_mutex);
    }
#endif

    if (context->initialize > 0) {
        database->axi_sram_base = context->axi_sram_virtual;
        database->vip_sram_base = context->vip_sram_address;
        database->func_config.enable_capture = context->func_config.enable_capture;
        database->func_config.enable_cnn_profile = context->func_config.enable_cnn_profile;
        database->func_config.enable_dump_nbg = context->func_config.enable_dump_nbg;
        database->hw_cid = context->chip_cid;
        database->core_count = context->core_count;
        database->max_core_count = context->max_core_count;
        database->device_count = context->device_count;
    #if vpmdENABLE_VIDEO_MEMORY_HEAP
        #if vpmdENABLE_MMU
        database->video_heap_vir_base = context->heap_virtual;
        database->video_heap_phy_base = context->heap_physical;
        #else
        database->video_heap_vir_base = (vip_uint32_t)context->heap_physical;
        database->video_heap_phy_base = context->heap_physical;
        #endif
        database->video_heap_size = context->heap_size;
    #endif
    #if vpmdENABLE_SYS_MEMORY_HEAP
        database->sys_heap_size = context->sys_heap_size;
    #endif

        for (iter0 = 0; iter0 < context->device_count; iter0++) {
            gckvip_device_t *device = &context->device[iter0];
            database->device_core_number[iter0] = device->hardware_count;
            database->axi_sram_size[iter0] = context->axi_sram_size[iter0];
            database->vip_sram_size[iter0] = context->vip_sram_size[iter0];
            for (iter1 = 0; iter1 < device->hardware_count; iter1++) {
                gckvip_hardware_t *hardware = &device->hardware[iter1];
                database->device_core_index[iter0][iter1] = hardware->core_id;
            }
        }
    }
    else {
        PRINTK_E("failed to query databse, please call do_init first\n");
        status = VIP_ERROR_IO;
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_unlock_mutex(context->initialize_mutex);
    }
#endif

    return status;
}

/*
@brief query the kernel space driver status.
*/
static vip_status_e do_query_driver_status(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = VIP_NULL;
    gckvip_query_driver_status_t *drv_status = (gckvip_query_driver_status_t*)data;
    vip_uint32_t device_id = drv_status->device_id;
    GET_CONTEXTK();
    GCKVIP_CHECK_DEV(device_id);
    GCKVIP_CHECK_INIT();

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_lock_mutex(context->initialize_mutex);
    }
#endif
    device = &context->device[device_id];
    drv_status->commit_count = device->user_submit_count;

    /* for saving initialize command into capture file */
    drv_status->init_cmd_physical = device->init_command.physical;
    drv_status->init_cmd_size = device->init_command_size;
    drv_status->init_cmd_handle = device->init_command.handle_id;

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_unlock_mutex(context->initialize_mutex);
    }
#endif

onError:
    return status;
}

/*
@brief flush CPU cache for video memory heap.
*/
#if vpmdENABLE_FLUSH_CPU_CACHE
static vip_status_e do_flush_cache(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    void *handle = VIP_NULL;
    gckvip_operate_cache_t *operate = (gckvip_operate_cache_t*)data;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    if (VIP_NULL == operate) {
        PRINTK_E("failed to flush cache, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    gcOnError(gckvip_database_get_handle(&context->videomem_database, operate->handle, &handle));
    gcOnError(gckvip_mem_flush_cache(handle, operate->physical, GCKVIPUINT64_TO_PTR(operate->logical),
                                     operate->size, operate->type));
    return status;

onError:
    PRINTK_E("failed to do flush cache. handle=0x%x physical=0x%x, size=0x%x, type=%d\n",
              operate->handle, operate->physical, operate->size, operate->type);
    return status;
}
#endif

static vip_status_e do_wrap_user_physical(void *data)
{
    vip_status_e status = VIP_SUCCESS;

    gckvip_wrap_user_physical_t *memory = (gckvip_wrap_user_physical_t*)data;
    void *handle = VIP_NULL;
    vip_uint32_t handle_id = 0;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    memory->handle = 0;
    status = gckvip_mem_wrap_userphysical(context, memory->physical_table,
                                          memory->size_table, memory->physical_num,
                                          memory->alloc_flag,
                                          memory->memory_type, &memory->virtual_addr,
                                          (void**)&memory->logical, &handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to wrap user physical, status=%d\n", status);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    gcOnError(gckvip_database_get_id(&context->videomem_database, handle, gckvip_os_get_pid(), &handle_id));
    PRINTK_D("wrap user physical, physical[0]=0x%"PRIx64", physical number=%d, virtual=0x%08x, handle=0x%x\n",
              memory->physical_table[0], memory->physical_num,  memory->virtual_addr, handle_id);

    memory->handle = handle_id;

onError:
    return status;
}

/*
@brief  un-wrap user memory
*/
static vip_status_e do_unwrap_user_physical(void *data)
{
    vip_status_e status = VIP_SUCCESS;

    gckvip_unwrap_user_memory_t *memory = (gckvip_unwrap_user_memory_t*)data;
    vip_uint32_t handle_id = memory->handle;
    void *handle = 0;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    gcOnError(gckvip_database_get_handle(&context->videomem_database, handle_id, &handle));

    status = gckvip_mem_unwrap_userphysical(context, handle, memory->virtual_addr);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to unwrap user memory, status=%d\n", status);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    gcOnError(gckvip_database_put_id(&context->videomem_database, handle_id));

onError:
    return status;
}

/*
@brief, wrap user memory to VIP. return VIP's virtual address and video memory handle to use space.
*/
static vip_status_e do_wrap_user_memory(void *data)
{
    vip_status_e status = VIP_SUCCESS;

    gckvip_wrap_user_memory_t *memory = (gckvip_wrap_user_memory_t*)data;
    void *handle = VIP_NULL;
    vip_uint32_t handle_id = 0;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    memory->handle = 0;
    status = gckvip_mem_wrap_usermemory(context, GCKVIPUINT64_TO_PTR(memory->logical), memory->size,
                                          memory->memory_type, &memory->virtual_addr, &handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to wrap user memory, status=%d\n", status);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    gcOnError(gckvip_database_get_id(&context->videomem_database, handle, gckvip_os_get_pid(), &handle_id));
    PRINTK_D("wrap user memory, logical=0x%"PRIx64", size=0x%08x, virtual=0x%08x, handle=0x%x\n",
              memory->logical, memory->size, memory->virtual_addr, handle_id);

    memory->handle = handle_id;

onError:
    return status;
}

/*
@brief  un-wrap user memory
*/
static vip_status_e do_unwrap_user_memory(void *data)
{
    vip_status_e status = VIP_SUCCESS;

    gckvip_unwrap_user_memory_t *memory = (gckvip_unwrap_user_memory_t*)data;
    vip_uint32_t handle_id = memory->handle;
    void *handle = 0;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    gcOnError(gckvip_database_get_handle(&context->videomem_database, handle_id, &handle));

    status = gckvip_mem_unwrap_usermemory(context, handle, memory->virtual_addr);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to unwrap user memory, status=%d\n", status);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    gcOnError(gckvip_database_put_id(&context->videomem_database, handle_id));

onError:
    return status;
}

#if vpmdENABLE_CREATE_BUF_FD
/*
@brief, from fd wrap user memory to VIP.
   return VIP's virtual address and video memory handle to use space.
*/
static vip_status_e do_wrap_user_fd(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_wrap_user_fd_t *memory = (gckvip_wrap_user_fd_t*)data;
    void *handle = VIP_NULL;
    vip_uint8_t *logical = VIP_NULL;
    vip_uint32_t handle_id = 0;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    memory->handle = 0;
    status = gckvip_mem_wrap_userfd(context, memory->fd, memory->size, memory->memory_type,
                                    &memory->virtual_addr, &logical, &handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to wrap user fd, status=%d\n", status);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    gcOnError(gckvip_database_get_id(&context->videomem_database, handle, gckvip_os_get_pid(), &handle_id));

    PRINTK_D("wrap user fd=%d, size=0x%08x, virtual=0x%08x, handle=0x%x\n",
              memory->fd, memory->size, memory->virtual_addr, handle_id);

    memory->handle = handle_id;
    memory->logical = GCKVIPPTR_TO_UINT64(logical);

onError:
    return status;
}

/*
@brief  un-wrap user memory fd
*/
static vip_status_e do_unwrap_user_fd(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_unwrap_user_memory_t *memory = (gckvip_unwrap_user_memory_t*)data;
    vip_uint32_t handle_id = memory->handle;
    void *handle = 0;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    gcOnError(gckvip_database_get_handle(&context->videomem_database, handle_id, &handle));

    status = gckvip_mem_unwrap_userfd(context, handle, memory->virtual_addr);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to unwrap user fd, status=%d\n", status);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    gcOnError(gckvip_database_put_id(&context->videomem_database, handle_id));

onError:
    return status;
}
#endif

/*
@brief query hardware information, hardware status.
*/
#if vpmdENABLE_POWER_MANAGEMENT
static vip_status_e do_query_power_info(
    void *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();

    status = gckvip_pm_query_info(context, data);
    if (status != VIP_SUCCESS) {
        PRINTK_E("query power status fail.\n");
    }

    return status;
}
#endif

#if vpmdENABLE_APP_PROFILING
static vip_status_e do_query_profiling(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_context_t *context = gckvip_get_context();
    gckvip_query_profiling_t *profiling = (gckvip_query_profiling_t*)data;
    vip_uint32_t device_id = profiling->device_id;
    void *mem_handle = VIP_NULL;
    GCKVIP_CHECK_INIT();
    gcIsNULL(data);

    gcOnError(gckvip_database_get_handle(&context->videomem_database, profiling->cmd_handle, &mem_handle));

    status = gckvip_query_profiling(context, &context->device[device_id], mem_handle, profiling);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to query profiling\n");
    }

onError:
    return status;
}
#endif

#if vpmdENABLE_CAPTURE_MMU
static vip_status_e do_query_mmu_info(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_query_mmu_info_t *info = (gckvip_query_mmu_info_t*)data;
    GET_CONTEXTK();
    gcIsNULL(data);

    if ((VIP_NULL == context->MMU_entry.handle) || (VIP_NULL == context->MTLB.handle) ||
        (VIP_NULL == context->ptr_STLB_1M->handle) ||
        (VIP_NULL == context->ptr_STLB_4K->handle)) {
        PRINTK_E("fail to get mmu info, page table is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    info->entry_physical = context->MMU_entry.physical;
    info->entry_size = context->MMU_entry.size;
    info->entry_handle = context->MMU_entry.handle_id;

    info->mtlb_physical = context->MTLB.physical;
    info->mtlb_size = context->MTLB.size;
    info->mtlb_handle = context->MTLB.handle_id;

    info->stlb_1m_physical = context->ptr_STLB_1M->physical;
    info->stlb_1m_size = context->STLB_1M.size;
    info->stlb_1m_handle = context->STLB_1M.handle_id;

    info->stlb_4k_physical = context->ptr_STLB_4K->physical;
    info->stlb_4k_size = context->STLB_4K.size;
    info->stlb_4k_handle = context->STLB_4K.handle_id;

    PRINTK_I("entry size=0x%x, mtlb size=0x%x, stlb_1M size=0x%x, stlb_4K size=0x%x\n",
            context->MMU_entry.size, context->MTLB.size, context->STLB_1M.size,
            context->STLB_4K.size);

    return status;
onError:
    PRINTK_E("fail to get mmu info\n");
    return status;
}
#endif

/*
@brief Applications more control over power management for VIP cores.
       such as setting core frquency, and power on/off. stop/re-start VIP core.
*/
#if vpmdENABLE_USER_CONTROL_POWER || vpmdPOWER_OFF_TIMEOUT
static vip_status_e do_power_management(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_device_t *device = VIP_NULL;
    gckvip_power_management_t *power = (gckvip_power_management_t*)data;
    GET_CONTEXTK();
    GCKVIP_CHECK_DEV(power->device_id);
    gcIsNULL(power);
    GCKVIP_CHECK_INIT();

    device = &context->device[power->device_id];
    status = gckvip_pm_power_management(power, device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to set power management device%d.\n", power->device_id);
    }
onError:
    return status;
}
#endif

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
static vip_status_e do_query_register_dump(void* data)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t handle_id = 0;
    gckvip_query_register_dump_t* info = (gckvip_query_register_dump_t*)data;
    GET_CONTEXTK();
    gcIsNULL(info);

    status |= gckvip_database_get_id(&context->videomem_database, context->register_dump_buffer.handle,
        gckvip_os_get_pid(), &handle_id);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to get register dump information\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    info->physical = context->register_dump_buffer.physical;
    info->count = context->register_dump_count;
    info->handle = handle_id;

    PRINTK_D("register dump in kernel: handle=0x%x, physical=0x%x, count=0x%x.\n",
        info->handle, info->physical, info->count);

onError:
    return status;
}
#endif

#if vpmdREGISTER_NETWORK
static vip_status_e do_register_network_info(void* data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_register_network_t *reg = (gckvip_register_network_t*)data;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();
    gcIsNULL(reg);

    status = gckvip_register_network_info(reg);
onError:
    return status;
}

static vip_status_e do_register_segment_info(void* data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_register_segment_t *reg = (gckvip_register_segment_t*)data;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();
    gcIsNULL(reg);

    status = gckvip_register_segment_info(reg);
onError:
    return status;
}

static vip_status_e do_unregister_network_info(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_unregister_t *unreg = (gckvip_unregister_t*)data;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();
    gcIsNULL(unreg);

    status = gckvip_unregister_network_info(unreg);

onError:
    return status;
}

static vip_status_e do_unregister_segment_info(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_unregister_t *unreg = (gckvip_unregister_t*)data;
    GET_CONTEXTK();
    GCKVIP_CHECK_INIT();
    gcIsNULL(unreg);

    status = gckvip_unregister_segment_info(unreg);

onError:
    return status;
}
#endif

/*
@brief Destroy kernel space resource.
       This function only use on Linux/Andorid system when used ctrl+c to exit application or
       application crash happend.
       When the appliction exits abnormally, gckvip_destroy() used to free all kenerl space resource.
*/
vip_status_e gckvip_destroy(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t process_id = gckvip_os_get_pid();
    vip_bool_e need_destroy = vip_false_e;
    vip_bool_e skip_this_handle = vip_false_e;
    vip_uint32_t i = 0;
    vip_uint32_t first_free_pos = 0;
    gckvip_database_table_t *table = VIP_NULL;
    GET_CONTEXTK();

    /* destroy resource */
#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
    mutex_lock(&kinit_mutex);
#endif
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_lock_mutex(context->initialize_mutex);
    }
#endif
    if (context->initialize > 0) {
        gckvip_hashmap_get_by_handle(&context->process_id, GCKVIPUINT64_TO_PTR(process_id), &i, VIP_NULL);
        if (0 == i) {
            need_destroy = vip_false_e;
        }
        else {
            need_destroy = vip_true_e;
            gckvip_hashmap_unuse(&context->process_id, i, vip_false_e);
        }
    }
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_unlock_mutex(context->initialize_mutex);
    }
#if defined(LINUX)
    mutex_unlock(&kinit_mutex);
#endif
#endif

    /* free all video memory which created by this porcess */
    if (vip_true_e == need_destroy) {
        gckvip_os_delay(60);
        GCKVIP_LOOP_DEVICE_START
        #if vpmdENABLE_MULTIPLE_TASK
        vip_uint32_t del_list[gcdMAX_CORE] = { 0 };
        gckvip_task_slot_t* m_task = VIP_NULL;
        #if vpmdENABLE_CANCELATION
        vip_bool_e support_cancel = gckvip_hw_support_cancel(context);
        #endif
        for (i = gckvip_hashmap_free_pos(&device->mt_hashmap) - 1; i > 0; i--) {
            status = gckvip_hashmap_get_by_index(&device->mt_hashmap, i, (void**)&m_task);
            if (status != VIP_NULL) {
                continue;
            }
            if (process_id == m_task->submit_handle.pid && GCKVIP_TASK_EMPTY != m_task->status) {
                if (GCKVIP_TASK_INFER_START == m_task->status) {
                    /* in inference, handle later */
                    vip_uint32_t index = m_task->submit_handle.device_id;
                    if (0 != del_list[index]) {
                        gckvip_task_slot_t* temp_task = VIP_NULL;
                        gckvip_hashmap_get_by_index(&device->mt_hashmap, del_list[index], (void**)&temp_task);
                        if (temp_task != VIP_NULL) {
                            gckvip_hashmap_remove(&device->mt_hashmap, temp_task->submit_handle.cmd_handle);
                        }
                        gckvip_hashmap_unuse(&device->mt_hashmap, del_list[index], vip_false_e);
                    }
                    del_list[index] = i;
                }
                else {
                    /* del directly */
                    m_task->status = GCKVIP_TASK_EMPTY;
                    if (VIP_NULL != m_task->queue_data) {
                        m_task->queue_data->v2 = VIP_ERROR_CANCELED;
                        gckvip_hashmap_remove(&device->mt_hashmap, m_task->submit_handle.cmd_handle);
                    }
                }
            }
            gckvip_hashmap_unuse(&device->mt_hashmap, i, vip_false_e);
        }
        for (i = 0; i < gcdMAX_CORE; i++) {
            if (0 != del_list[i]) {
                gckvip_hashmap_get_by_index(&device->mt_hashmap, del_list[i], (void**)&m_task);
                if (VIP_NULL == m_task) {
                    continue;
                }
                if (GCKVIP_TASK_INFER_START == m_task->status) {
                    /* still in inference */
                    #if vpmdENABLE_CANCELATION
                    if (vip_true_e == support_cancel) {
                        status = gckvip_hardware_do_cancel(m_task, device);
                        if (status != VIP_SUCCESS) {
                            PRINTK_E("hardware %d cancel failed during destroy\n", m_task->submit_handle.device_id);
                        }
                    }
                    else
                    #endif
                    {
                        status = gckvip_wait_device_idle(device);
                        if (status != VIP_SUCCESS) {
                            PRINTK_E("hardware %d not idle during destroy\n", m_task->submit_handle.device_id);
                        }
                    }
                }
                m_task->status = GCKVIP_TASK_EMPTY;
                gckvip_hashmap_remove(&device->mt_hashmap, m_task->submit_handle.cmd_handle);
                gckvip_hashmap_unuse(&device->mt_hashmap, del_list[i], vip_false_e);
            }
        }
        #else
        status = gckvip_wait_device_idle(device);
        if (status != VIP_SUCCESS) {
            PRINTK_E("device%d not going to idle\n", dev_index);
        }
        #endif
        GCKVIP_LOOP_DEVICE_END
        first_free_pos = gckvip_database_get_freepos(&context->videomem_database);
        for (i = 0; i < first_free_pos; i++) {
            skip_this_handle = vip_false_e;
            gckvip_database_get_table(&context->videomem_database, i + DATABASE_MAGIC_DATA, &table);
            if ((table->process_id == process_id) && (table->handle != VIP_NULL)) {
                gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t*)table->handle;

                #if vpmdENABLE_CAPTURE_MMU
                if ((ptr == context->MMU_entry.handle) || (ptr == context->STLB_4K.handle) ||
                    (ptr == context->STLB_1M.handle) || (ptr == context->MTLB.handle)) {
                        continue;
                }
                #endif

                GCKVIP_LOOP_DEVICE_START
                if (device->init_command.handle != VIP_NULL) {
                    if (ptr == device->init_command.handle) {
                        skip_this_handle = vip_true_e;
                    }
                    continue;
                }
                GCKVIP_LOOP_DEVICE_END
                if (vip_true_e == skip_this_handle) {
                    continue;
                }

                PRINTK_I("force free video memory handle=0x%"PRPx"\n", table->handle);
                #if vpmdREGISTER_NETWORK
                {
                    gckvip_network_info_t *info = VIP_NULL;
                    vip_uint32_t j = 0, k = 0;
                    if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->network_info, table->handle,
                                                                    &j, (void**)&info)) {
                        if (VIP_NULL != info->segment_handle) {
                            for (k = 0; k < info->segment_count; k++) {
                                gckvip_hashmap_remove(&context->segment_info, info->segment_handle[k]);
                            }
                            gckvip_os_free_memory(info->segment_handle);
                            info->segment_handle = VIP_NULL;
                        }
                        gckvip_hashmap_remove(&context->network_info, table->handle);
                        gckvip_hashmap_unuse(&context->network_info, j, vip_false_e);
                    }
                }
                #endif
                switch (ptr->memory_type) {
                    case GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC:
                    {
                        gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
                        node->mem_flag &= ~GCVIP_MEM_FLAG_MAP_USER;
                        gckvip_mem_free_videomemory(context, table->handle);
                    }
                    break;

                    case GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP:
                    {
                        gckvip_mem_free_videomemory(context, table->handle);
                    }
                    break;

                    case GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_LOGICAL:
                    {
                        gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
                        node->mem_flag &= ~GCVIP_MEM_FLAG_MAP_USER;
                        gckvip_mem_unwrap_usermemory(context, table->handle, 0);
                    }
                    break;

                    case GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL:
                    {
                        gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
                        node->mem_flag &= ~GCVIP_MEM_FLAG_MAP_USER;
                        gckvip_mem_unwrap_userphysical(context, table->handle, 0);
                    }
                    break;

                    default:
                        break;
                }
                gckvip_database_put_id(&context->videomem_database, i + DATABASE_MAGIC_DATA);
            }
        }
        PRINTK_I("start to destroy resource, pid=%x, tid=%x\n", process_id, gckvip_os_get_tid());
        do_destroy();
    }
    else {
        PRINTK_I("vip destroyed or other tasks running, initialize=%d\n", context->initialize);
    }

    return status;
}

/*
@brief  initialize driver
*/
vip_status_e gckvip_init(void)
{
    gckvip_initialize_t init_data;
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    gckvip_hardware_info_t info = {0};
    gckvip_drv_get_hardware_info(&info);
    init_data.video_mem_size = info.vip_memsize;
#endif

    status = do_init(&init_data);
    return status;
}

/*
@brief  dispatch kernel command
@param  command, command type.
@param  data, the data for command type.
*/
vip_status_e gckvip_kernel_call(
    gckvip_command_id_e command,
    void *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    switch (command)
    {
    case KERNEL_CMD_INIT:           /* VIP initialization command. */
        status = do_init(data);
        break;

    case KERNEL_CMD_DESTROY:        /* VIP terminate command. */
        status = do_destroy();
        break;

    case KERNEL_CMD_READ_REG:       /* VIP register read command. */
        status = do_read_register(data);
        break;

    case KERNEL_CMD_WRITE_REG:      /* VIP register write command. */
        status = do_write_register(data);
        break;

    case KERNEL_CMD_WAIT:           /* VIP interrupt wait command. */
        status = do_wait(data);
        break;

    case KERNEL_CMD_SUBMIT:         /* VIP command buffer submit command. */
        status = do_submit(data);
        break;

    case KERNEL_CMD_ALLOCATION:     /* VIP buffer allocation command. */
        status = do_allocation_videomem(data);
        break;

    case KERNEL_CMD_DEALLOCATION:      /* VIP memory reset command. */
        status = do_free_videomem(data);
        break;

    case KERNEL_CMD_MAP_USER_LOGICAL:
        status = do_map_user_logical(data);
        break;
    case KERNEL_CMD_UNMAP_USER_LOGICAL:
        status = do_unmap_user_logical(data);
        break;

    case KERNEL_CMD_RESET_VIP:      /* VIP HW software reset command. */
        status = do_reset_vip();
        break;

    case KERNEL_CMD_QUERY_DATABASE: /* query vip sram  and axi sram info */
        status= do_query_database(data);
        break;

    case KERNEL_CMD_QUERY_DRIVER_STATUS:
        status= do_query_driver_status(data);
        break;
    case KERNEL_CMD_WRAP_USER_PHYSICAL:  /* wrap user physical memory to VIP */
        status = do_wrap_user_physical(data);
        break;

    case KERNEL_CMD_UNWRAP_USER_PHYSICAL: /* un-warp physical memory */
        status = do_unwrap_user_physical(data);
        break;

    case KERNEL_CMD_WRAP_USER_MEMORY: /* wrap user logical address memory to VIP virtual address */
        status = do_wrap_user_memory(data);
        break;

    case KERNEL_CMD_UNWRAP_USER_MEMORY: /* un-warp user memory to VIP virtual */
        status = do_unwrap_user_memory(data);
        break;

#if vpmdENABLE_CANCELATION
    case KERNEL_CMD_CANCEL:           /* VIP cancel command. */
        status = do_cancel(data);
        break;
#endif

#if vpmdENABLE_FLUSH_CPU_CACHE
    case KERNEL_CMD_OPERATE_CACHE:    /* flush/clean/invalid cpu cache */
        status = do_flush_cache(data);
        break;
#endif

#if vpmdENABLE_USE_AGENT_TRIGGER
    case KERNEL_CMD_QUERY_ADDRESS_INFO: /* query viplite's wait-link buffer address information */
        status = do_query_address_info(data);
        break;
#endif

#if vpmdENABLE_CREATE_BUF_FD
    case KERNEL_CMD_WRAP_USER_FD:
        status = do_wrap_user_fd(data);
        break;
    case KERNEL_CMD_UNWRAP_USER_FD:
        status = do_unwrap_user_fd(data);
        break;
#endif

#if vpmdENABLE_POWER_MANAGEMENT
    case KERNEL_CMD_QUERY_POWER_INFO: /* query hardware information */
        status = do_query_power_info(data);
        break;
#endif

#if vpmdENABLE_APP_PROFILING
    case KERNEL_CMD_QUERY_PROFILING:
        status = do_query_profiling(data);
        break;
#endif

#if vpmdENABLE_USER_CONTROL_POWER || vpmdPOWER_OFF_TIMEOUT
    case KERNEL_CMD_POWER_MANAGEMENT:
        status = do_power_management(data);
        break;
#endif

#if vpmdENABLE_CAPTURE_MMU
    case KERNEL_CMD_QUERY_MMU_INFO:
        status = do_query_mmu_info(data);
        break;
#endif
#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
    case KERNEL_CMD_QUERY_REGISTER_DUMP:
        status = do_query_register_dump(data);
        break;
#endif
#if vpmdREGISTER_NETWORK
    case KERNEL_CMD_REGISTER_NETWORK_INFO:
        status = do_register_network_info(data);
        break;
    case KERNEL_CMD_REGISTER_SEGMENT_INFO:
        status = do_register_segment_info(data);
        break;
    case KERNEL_CMD_UNREGISTER_NETWORK_INFO:
        status = do_unregister_network_info(data);
        break;
    case KERNEL_CMD_UNREGISTER_SEGMENT_INFO:
        status = do_unregister_segment_info(data);
        break;
#endif
    default:
        status = VIP_ERROR_INVALID_ARGUMENTS;
        PRINTK_E("kernel doesn't support this command=%s\n", ioctl_cmd_string[command]);
        break;
    };
    return status;
}

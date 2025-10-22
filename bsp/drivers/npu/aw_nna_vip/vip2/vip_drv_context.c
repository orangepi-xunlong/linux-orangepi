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

#include <vip_drv_context.h>
#include <vip_drv_device_driver.h>
#include <vip_drv_debug.h>
#include <vip_drv_task_descriptor.h>
#include <vip_feature_database.h>

/* vip drv context object. */
vipdrv_context_t vipDrvContext = {0};

/*
@brief Get vip-drv context object.
*/
void* vipdrv_get_context(
    vipdrv_context_property_e prop
    )
{
    vipdrv_context_t* context = &vipDrvContext;
    switch (prop)
    {
    case VIPDRV_CONTEXT_PROP_ALL:
        return (void*)context;
    case VIPDRV_CONTEXT_PROP_CID:
        return (void*)&context->chip_cid;
    case VIPDRV_CONTEXT_PROP_TASK_DESC:
        return (void*)&context->tskDesc_database;
#if vpmdTASK_QUEUE_USED
    case VIPDRV_CONTEXT_PROP_TCB:
        return (void*)&context->tskTCB_hashmap;
#endif
    case VIPDRV_CONTEXT_PROP_INITED:
        return (void*)&context->initialize;
    case VIPDRV_CONTEXT_PROP_DEVICE_CNT:
        return (void*)&context->device_count;
    case VIPDRV_CONTEXT_PROP_HARDWARE_CNT:
        return (void*)&context->core_count;
    default:
        break;
    }

    return VIP_NULL;
}

static vip_status_e vipdrv_device_init(
    vipdrv_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_hardware_info_t info = {0};
    vipdrv_platform_memory_config_t mem_config = {0};
    vip_uint32_t i = 0;
    vip_uint32_t core_cnt = 0;

    status = vipdrv_drv_get_hardware_info(&info);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to get hardware information, status=%d.\n", status);
        vipGoOnError(status);
    }

    status = vipdrv_drv_get_memory_config(&mem_config);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to get memory config, status=%d.\n", status);
        vipGoOnError(status);
    }

    context->device_count = 0;
    context->core_count = info.core_count;
    context->max_core_count = info.max_core_count;
#if !vpmdENABLE_MMU
    context->axi_sram_address = (vip_virtual_t)info.axi_sram_base;
#endif
    context->axi_sram_physical = info.axi_sram_base;
    context->vip_sram_address = info.vip_sram_base;

    PRINTK_D("axi sram address=0x%x, vipsram address=0x%08x\n",
              context->axi_sram_physical, context->vip_sram_address);
#if vpmdENABLE_SYS_MEMORY_HEAP
    context->sys_heap_size = mem_config.sys_heap_size;
#endif
#if vpmdENABLE_RESERVE_PHYSICAL
    context->reserve_phy_base = mem_config.reserve_phy_base;
    context->reserve_phy_size = mem_config.reserve_phy_size;
#endif
    if (context->core_count > vipdMAX_CORE) {
        PRINTK_E("fail to device init, core count %d is more than max %d\n",
                  context->core_count, vipdMAX_CORE);
        vipGoOnError(VIP_ERROR_IO);
    }

    /* caculate device count */
    for (i = 0; i < vipdMAX_CORE; i++) {
        if (info.device_core_number[i] != 0) {
            context->device_count += 1;
            context->axi_sram_size[i] = info.axi_sram_size[i];
            PRINTK_D("dev%d axi sram size=0x%08x\n", i, context->axi_sram_size[i]);
            if (0 != context->axi_sram_size[i] % vipdMEMORY_ALIGN_SIZE) {
                PRINTK_E("dev%d axi sram size=0x%08x is not %d align\n",
                    i, context->axi_sram_size[i], vipdMEMORY_ALIGN_SIZE);
                vipGoOnError(VIP_ERROR_FAILURE);
            }
        }
        else {
            break;
        }
    }
    PRINTK_D("max_core_count=%d, total core count=%d, total device=%d\n",
             context->max_core_count, context->core_count, context->device_count);
    if (0 == context->device_count) {
        PRINTK_E("fail to device init dev cnt=%d\n", context->device_count);
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    else if (context->device_count > 64) {
        PRINTK_E("fail max support 64 device for video memory mask\n", context->device_count);
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_device_t) * context->device_count,
                                       (void**)&context->device));
    vipdrv_os_zero_memory(context->device, sizeof(vipdrv_device_t) * context->device_count);

    for (i = 0; i < context->device_count; i++) {
        vipdrv_device_t *device = &context->device[i];
        device->device_index = i;
        device->context = context;
        device->hardware_count = info.device_core_number[i];
        PRINTK_D("device_index=%d, this device core count=%d\n",
                 i, device->hardware_count);
        vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_hardware_t) * device->hardware_count,
                                           (void**)&device->hardware));
        vipdrv_os_zero_memory(device->hardware, sizeof(vipdrv_hardware_t) * device->hardware_count);

        VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
        #if vpmdTASK_PARALLEL_ASYNC
        if (0 == hw_index) {
            device->irq_queue = info.irq_queue[core_cnt];
            device->irq_value = info.irq_value[core_cnt];
        }
        #endif
        #if vpmdENABLE_RECOVERY
        hw->recovery_times = MAX_RECOVERY_TIMES;
        #endif
        hw->core_id = core_cnt;
        hw->core_index = hw_index;
        hw->device = device;
        hw->reg_base = info.vip_reg[core_cnt];
        hw->init_done = vip_false_e;
        VIPDRV_CHECK_NULL(hw->reg_base);
        hw->irq_value = info.irq_value[core_cnt];
        *hw->irq_value = 0;
        VIPDRV_CHECK_NULL(hw->irq_value);
        #if !vpmdENABLE_POLLING
        hw->irq_queue = info.irq_queue[core_cnt];
        VIPDRV_CHECK_NULL(hw->irq_queue);
        hw->irq_count = info.irq_count[core_cnt];
        VIPDRV_CHECK_NULL(hw->irq_count);
        #endif
        #if vpmdENABLE_MULTIPLE_TASK
        status = vipdrv_os_create_mutex(&hw->reg_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to create mutex for register memory\n");
            vipGoOnError(VIP_ERROR_IO);
        }
        #endif
        #if vpmdENABLE_POWER_MANAGEMENT
        if (0 == info.core_fscale_percent || 100 < info.core_fscale_percent) {
            info.core_fscale_percent = 100;
        }
        hw->core_fscale_percent = info.core_fscale_percent;
        #endif
        core_cnt++;
        if (core_cnt > context->core_count) {
            PRINTK_E("fail to device init, core count=%d overflow\n", core_cnt);
            vipOnError(VIP_ERROR_FAILURE);
        }
        PRINTK_D("  dev_id=%d, core_id=%d, core_index=%d, core_fscale_percent=%d\n",
                 i, hw->core_id, hw->core_index, info.core_fscale_percent);

        vipOnError(vipdrv_os_allocate_memory(sizeof(vip_uint32_t) * TASK_HISTORY_RECORD_COUNT,
            (void**)&hw->previous_task_id));
        vipdrv_os_zero_memory(hw->previous_task_id, sizeof(vip_uint32_t) * TASK_HISTORY_RECORD_COUNT);
        vipOnError(vipdrv_os_allocate_memory(sizeof(vip_uint32_t) * TASK_HISTORY_RECORD_COUNT,
            (void**)&hw->previous_subtask_index));
        vipdrv_os_zero_memory(hw->previous_subtask_index, sizeof(vip_uint32_t) * TASK_HISTORY_RECORD_COUNT);
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    }

onError:
    return status;
}

static vip_status_e vipdrv_device_uninit(
    vipdrv_context_t* context
    )
{
    VIPDRV_LOOP_DEVICE_START
    if (VIP_NULL == device->hardware) {
        PRINTK_E("fail to uninit device%d hardware\n", dev_index);
        continue;
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    if (hw != VIP_NULL) {
        #if vpmdENABLE_MULTIPLE_TASK
        if (hw->reg_mutex != VIP_NULL) {
            vipdrv_os_destroy_mutex(hw->reg_mutex);
        }
        #endif
        if (VIP_NULL != hw->previous_task_id) {
            vipdrv_os_free_memory(hw->previous_task_id);
            hw->previous_task_id = VIP_NULL;
        }
        if (VIP_NULL != hw->previous_subtask_index) {
            vipdrv_os_free_memory(hw->previous_subtask_index);
            hw->previous_subtask_index = VIP_NULL;
        }
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    vipdrv_os_free_memory(device->hardware);
    device->hardware = VIP_NULL;
    VIPDRV_LOOP_DEVICE_END
    if (context->device != VIP_NULL) {
        vipdrv_os_free_memory(context->device);
        context->device = VIP_NULL;
    }
    return VIP_SUCCESS;
}

vip_status_e vipdrv_context_init(
    vipdrv_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t index = 0;
    vip_uint32_t i = 0;

#if vpmdENABLE_DEBUG_LOG > 3
    vipdrv_dump_options();
#endif

    vipOnError(vipdrv_hashmap_init(&context->process_id, 12, 0, "process-ID", VIP_NULL, VIP_NULL, VIP_NULL));
    vipdrv_hashmap_insert(&context->process_id, (vip_uint64_t)vipdrv_os_get_pid(), &index);
    status = vipdrv_device_init(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to device init status=%d\n", status);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    vipOnError(vipdrv_task_desc_init());

    vipOnError(vipdrv_task_tcb_init());
    PRINTK_D("tskTCB init done...\n");

    vipOnError(vipdrv_task_init());

    VIPDRV_LOOP_DEVICE_START
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    status = vipdrv_os_create_atomic(&hw->idle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for vip idle\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_set_atomic(hw->idle, vip_true_e);
    #if vpmdENABLE_POWER_MANAGEMENT
    hw->disable_clock_suspend = 0;
    #endif
    #if vpmdENABLE_CLOCK_SUSPEND
    hw->clock_suspended = vip_false_e;
    #endif
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    VIPDRV_LOOP_DEVICE_END

#if vpmdONE_POWER_DOMAIN
    status = vipdrv_os_create_mutex(&context->power_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create mutex for power on/off\n");
        vipGoOnError(VIP_ERROR_IO);
    }
#endif

    /* Read feature database */
    vipOnError(vipdrv_context_select_featureDB(context));

    /* init vip-sram size from feature database */
    for (i = 0; i < context->core_count; i++) {
        context->vip_sram_size[i] = context->feature_db->vip_sram_size;
        PRINTK_D("hw%d vip sram size=0x%x\n", i, context->vip_sram_size[i]);
    }

    return status;
onError:
    PRINTK_E("fail to context init status=%d\n", status);
    return status;
}

vip_status_e vipdrv_context_uninit(
    vipdrv_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;

    vipdrv_task_uninit();

    vipdrv_task_tcb_uninit();

    vipdrv_task_desc_uninit();
    VIPDRV_LOOP_DEVICE_START
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    if (hw->idle != VIP_NULL) {
        vipdrv_os_destroy_atomic(hw->idle);
        hw->idle = VIP_NULL;
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    VIPDRV_LOOP_DEVICE_END
#if vpmdONE_POWER_DOMAIN
    if (context->power_mutex != VIP_NULL) {
        vipdrv_os_destroy_mutex(context->power_mutex);
    }
#endif
    vipdrv_hashmap_destroy(&context->process_id);
    vipdrv_device_uninit(context);

    return status;
}

vip_status_e vipdrv_context_select_featureDB(
    vipdrv_context_t* context
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;

    /* Read hardware information */
    vipdrv_hw_read_info();

    for (i = 0; i < sizeof(vip_chip_info) / sizeof(vip_hw_feature_db_t); i++) {
        if (context->chip_cid == vip_chip_info[i].pid) {
            context->feature_db = &vip_chip_info[i];
            break;
        }
    }

    if (VIP_NULL == context->feature_db) {
        status = VIP_ERROR_FAILURE;
        PRINTK_E("did not find feature database for 0x%x\n", context->chip_cid);
    }

    return status;
}

vip_bool_e vipdrv_context_support(
    vip_hardware_feature_e feature
    )
{
    vip_bool_e support = vip_false_e;
    vipdrv_context_t* context = &vipDrvContext;

    if (VIP_NULL == context->feature_db) {
        return vip_false_e;
    }

    switch (feature)
    {
    case VIP_HW_FEATURE_JOB_CANCEL:
        #if !(defined(_WIN32) || defined(LINUXEMULATOR))
        if (context->feature_db->job_cancel) {
            support = vip_true_e;
        }
        #endif
        break;
    case VIP_HW_FEATURE_MMU_PDMODE:
        #if !(defined(_WIN32) || defined(LINUXEMULATOR))
        if (context->feature_db->mmu_pd_mode) {
            support = vip_true_e;
        }
        #endif
        break;
    case VIP_HW_FEATURE_FREQ_SCALE:
        #if vpmdENABLE_USER_CONTROL_POWER
        support = vip_true_e;
        #else
        support = vip_false_e;
        #endif
        break;
    case VIP_HW_FEATURE_NN_ERROR_DETECT:
        if (context->feature_db->nn_error_detect) {
            support = vip_true_e;
        }
        break;
    case VIP_HW_FEATURE_OCB_COUNTER:
        if (context->feature_db->ocb_counter) {
            support = vip_true_e;
        }
        break;
    case VIP_HW_FEATURE_SH_CONFORMANCE_BRUTEFORCE:
        if (context->feature_db->sh_conformance) {
            support = vip_true_e;
        }
        break;
    case VIP_HW_FEATURE_MMU:
        if (!context->feature_db->remove_mmu) {
            support = vip_true_e;
        }
        break;
    case VIP_HW_FEATURE_MMU_40VA:
        if (40 == context->feature_db->va_bits) {
            support = vip_true_e;
        }
        break;
    case VIP_HW_FEATURE_NN_XYDP0:
        if (context->feature_db->nn_xydp0) {
            support = vip_true_e;
        }
        break;
    default:
        break;
    }

    return support;
}

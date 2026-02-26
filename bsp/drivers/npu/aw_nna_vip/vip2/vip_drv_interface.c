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

#include <vip_drv_interface.h>
#include <vip_lite_version.h>
#include <vip_drv_context.h>
#include <vip_drv_os_port.h>
#include <vip_drv_device_driver.h>
#include <vip_drv_debug.h>
#include <vip_drv_task_descriptor.h>
#include <vip_drv_mmu.h>

#define VIPDRV_CHECK_INIT()                                                    \
{                                                                              \
    if (*(vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED) <= 0) { \
        PRINTK_E("fail driver not initialize.\n");                             \
        return VIP_ERROR_FAILURE;                                              \
    }                                                                          \
}

#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
static DEFINE_MUTEX(kinit_mutex);
#endif
#endif

/*
@brief initialize hardware and all resource.
*/
static vip_status_e do_init(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_initialize_t *init_data = (vipdrv_initialize_t *)data;
    vip_uint32_t pid = vipdrv_os_get_pid();
    vipdrv_context_t *context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

    if (init_data->version != ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_SUB_MINOR))) {
        PRINTK_E("lib version mismatch, VIPHal.so=0x%08x, vipcore.ko=0x%08x.\n",
            init_data->version, (VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_SUB_MINOR));
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
    mutex_lock(&kinit_mutex);
#endif
    if (VIP_NULL == context->initialize_mutex) {
        PRINTK_D("start create initialize mutex ...\n");
        vipdrv_os_zero_memory(context, sizeof(vipdrv_context_t));
        status = vipdrv_os_create_mutex(&context->initialize_mutex);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to create mutex for initialize\n");
            vipGoOnError(VIP_ERROR_IO);
        }
    }
    vipdrv_os_lock_mutex(context->initialize_mutex);

    if (context->initialize >= 1) {
        vip_uint32_t index = 0;
        /* Mark initialized. */
        if (vip_true_e == vipdrv_hashmap_full(&context->process_id)) {
            vip_bool_e expand = vip_false_e;
            status = vipdrv_hashmap_expand(&context->process_id, &expand);
            if (VIP_SUCCESS != status) {
                PRINTK_E("fail to initialize, task id is full.\n");
                vipGoOnError(VIP_ERROR_IO);
            }
        }
        vipdrv_hashmap_insert(&context->process_id, (vip_uint64_t)pid, &index);
        context->initialize++;
        PRINTK_D("vipcore has been initialized num=%d, pid=%x\n", context->initialize, pid);

        #if (vpmdENABLE_DEBUG_LOG >= 4)
        {
            vip_uint32_t i = 0;
            void* handle = VIP_NULL;
            PRINTK("pids: \n");
            for (i = 1; i < vipdrv_hashmap_free_pos(&context->process_id); i++) {
                vipdrv_hashmap_get_by_index(&context->process_id, i, &handle);
                if (VIP_NULL != handle) {
                    PRINTK("index=%d, pid=%x\n", i, handle);
                    vipdrv_hashmap_unuse(&context->process_id, i, vip_false_e);
                }
            }
        }
        #endif

        vipdrv_os_unlock_mutex(context->initialize_mutex);
        #if defined(LINUX)
        mutex_unlock(&kinit_mutex);
        #endif

        return VIP_SUCCESS;
    }
#else
    if (context->initialize >= 1) {
        PRINTK_E("vipcore has been initialized, pid=%x\n", pid);
        status = VIP_ERROR_NOT_SUPPORTED;
        vipGoOnError(status);
    }
    vipdrv_os_zero_memory(context, sizeof(vipdrv_context_t));
#endif
    /* set default value of capture function */
    context->func_config.enable_capture = vip_true_e;
    context->func_config.enable_cnn_profile = vip_true_e;
    context->func_config.enable_dump_nbg = vip_true_e;

    /* 1. Initialize OS layer, driver layer and power on hardware. */
    status = vipdrv_os_init(init_data->video_mem_size);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to os initialize, status=%d.\n", status);
        vipGoOnError(status);
    }

    /* 2. Init resource resource */
    status = vipdrv_context_init(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize context resource, status=%d.\n", status);
        vipGoOnError(status);
    }

    /* 3. Init video memory */
    status = vipdrv_video_memory_init();
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize video memory, status=%d.\n", status);
        vipGoOnError(status);
    }

#if vpmdENABLE_POWER_MANAGEMENT
    /* 4. Initialize power managment */
    status = vipdrv_pm_init();
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to os initialize power management, status=%d.\n", status);
        vipGoOnError(status);
    }
#endif

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
    vipOnError(vipdrv_register_dump_init(context));
#endif

    /* 5. Reset hardware. */
    VIPDRV_LOOP_DEVICE_START
    status = vipdrv_device_hw_reset(device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to reset device %d\n", device->device_index);
        vipGoOnError(VIP_ERROR_IO);
    }
    VIPDRV_LOOP_DEVICE_END

    /* 6. Init hardware */
    VIPDRV_LOOP_DEVICE_START
    status = vipdrv_device_prepare_initcmd(device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to prepare init command\n");
        vipGoOnError(VIP_ERROR_IO);
    }

#if vpmdENABLE_AW_DEVFREQ
	vipdrv_set_internal_freq();
#endif

#if vpmdTASK_PARALLEL_ASYNC
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    vipdrv_hw_wl_table_init(hw);
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
#endif

    status = vipdrv_device_hw_init(device);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to init hardware device %d\n", device->device_index);
        vipGoOnError(VIP_ERROR_IO);
    }
    VIPDRV_LOOP_DEVICE_END

#if vpmdENABLE_DEBUG_LOG > 3
    vipdrv_report_clock();
#endif
    context->initialize = 1;
#if vpmdONE_POWER_DOMAIN
    context->power_hardware_cnt = context->core_count;
#endif
    PRINTK_D("do init done initialize num=%d pid=0x%x.\n", context->initialize, pid);

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_unlock_mutex(context->initialize_mutex);
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

/*
@brief destroy resource.
*/
static vip_status_e do_destroy(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
    mutex_lock(&kinit_mutex);
#endif
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_lock_mutex(context->initialize_mutex);
    }
    else {
        PRINTK_E("error initialize mutex is NULL in do destroy\n");
    }
#endif

    /* Mark uninitialized. */
#if vpmdENABLE_MULTIPLE_TASK
    if (VIP_SUCCESS == vipdrv_hashmap_remove(&context->process_id,
        (vip_uint64_t)vipdrv_os_get_pid(), vip_false_e)) {
        #if vpmdENABLE_MMU
        vipdrv_mmu_page_uninit(gen_mmu_id(vipdrv_os_get_pid(), 0));
        #endif
        context->initialize--;
    }
    else {
        PRINTK_E("destroy can't found process_id=0x%x. pids:\n", vipdrv_os_get_pid());
    }
    if (context->initialize <= 0) {
#else
    if (context->initialize > 0) {
        context->initialize = 0;
        vipdrv_hashmap_remove(&context->process_id, (vip_uint64_t)vipdrv_os_get_pid(), vip_false_e);
        #if vpmdENABLE_MMU
        vipdrv_mmu_page_uninit(gen_mmu_id(vipdrv_os_get_pid(), 0));
        #endif
#endif
        PRINTK_D("start to destroy vip pid=0x%x..\n", vipdrv_os_get_pid());
        #if vpmdENABLE_POWER_MANAGEMENT
        vipdrv_pm_pre_uninit();
        #elif vpmdTASK_PARALLEL_ASYNC
        VIPDRV_LOOP_HARDWARE_START
        vipdrv_hw_stop_wl(hw);
        VIPDRV_LOOP_HARDWARE_END
        #endif
        vipdrv_task_pre_uninit();

        /* wait hardware idle */
        VIPDRV_LOOP_DEVICE_START
        if (0 != device->init_command.mem_id) {
            vipdrv_mem_free_videomemory(device->init_command.mem_id);
            device->init_command.mem_id = 0;
        }
        status = vipdrv_device_wait_idle(device, 1000, vip_false_e);
        if (status != VIP_SUCCESS) {
            PRINTK_E("device%d not going to idle\n", dev_index);
        }
        VIPDRV_LOOP_DEVICE_END

        /* has free video memory in destroy function, should be call before mmu uninit */
        status = vipdrv_hw_destroy();
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to destroy hardware\n");
        }

        #if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
        vipdrv_register_dump_uninit(context);
        #endif
        #if vpmdENABLE_POWER_MANAGEMENT
        /* uninitialize power management*/
        vipdrv_pm_uninit();
        #endif

        status = vipdrv_video_memory_uninit();
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to uninit video memory, status=%d.\n", status);
        }

        vipdrv_context_uninit(context);

        status = vipdrv_os_close();
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to destroy os\n");
        }

    #if vpmdENABLE_MULTIPLE_TASK
        /* unlock and destroy initialize mutex last */
        if (context->initialize_mutex != VIP_NULL) {
            vipdrv_os_unlock_mutex(context->initialize_mutex);
            PRINTK_D("destroy initialize mutex\n");
            vipdrv_os_destroy_mutex(context->initialize_mutex);
            context->initialize_mutex = VIP_NULL;
        }
    #endif
        vipdrv_os_zero_memory(context, sizeof(vipdrv_context_t));
        PRINTK_D("end to destroy vip..\n");
    }
#if vpmdENABLE_MULTIPLE_TASK
    else {
        if (context->initialize_mutex != VIP_NULL) {
            vipdrv_os_unlock_mutex(context->initialize_mutex);
        }
        else {
            PRINTK_E("error initialize mutex is NULL in do destroy end\n");
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
    vipdrv_reg_t *reg_data = (vipdrv_reg_t *)data;
    vipdrv_device_t *device = VIP_NULL;
    vipdrv_hardware_t *hardware = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    if (VIP_NULL == data) {
        PRINTK_E("fail to read register, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    device = vipdrv_get_device(reg_data->dev_index);
    vipIsNULL(device);

    hardware = vipdrv_get_hardware(device, reg_data->core_index);
    vipIsNULL(hardware);

#if vpmdPOWER_OFF_TIMEOUT
    status = vipdrv_pm_check_power(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to check power in read reg status=%d\n", status);
        vipGoOnError(status);
    }
#endif
#if vpmdENABLE_CLOCK_SUSPEND
    status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to enable hardare%d clock\n", hardware->core_id);
        vipGoOnError(status);
    }
#endif
    status = vipdrv_read_register(hardware, reg_data->reg, &reg_data->data);
#if vpmdENABLE_CLOCK_SUSPEND
    status |= vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
#endif
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to read hardware register status=%d\n", status);
        vipGoOnError(status);
    }
onError:
    return status;
}

/*
@brief Write a register.
*/
static vip_status_e do_write_register(void *data)
{
    vipdrv_reg_t *reg_data = (vipdrv_reg_t *)data;
    vipdrv_device_t *device = VIP_NULL;
    vipdrv_hardware_t *hardware = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;

    if (VIP_NULL == data) {
        PRINTK_E("fail to write register, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    device = vipdrv_get_device(reg_data->dev_index);
    vipIsNULL(device);

    hardware = vipdrv_get_hardware(device, reg_data->core_index);
    vipIsNULL(hardware);

#if vpmdPOWER_OFF_TIMEOUT
    status = vipdrv_pm_check_power(hardware);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to check power in write reg status=%d\n", status);
        vipGoOnError(status);
    }
#endif
#if vpmdENABLE_CLOCK_SUSPEND
    status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to enable hardare%d clock\n", hardware->core_id);
        vipGoOnError(status);
    }
#endif
    status = vipdrv_write_register(hardware, reg_data->reg, reg_data->data);
#if vpmdENABLE_CLOCK_SUSPEND
    status |= vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
#endif
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to write hardware register\n");
        vipGoOnError(status);
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
    vipdrv_allocation_t *allocation = (vipdrv_allocation_t *)data;
    vip_uint32_t alloc_flag = allocation->alloc_flag;
    vip_uint32_t mem_id = 0;
    void* user_logical = VIP_NULL;
    vip_physical_t physical = allocation->physical;
    VIPDRV_CHECK_INIT();

    status = vipdrv_mem_allocate_videomemory(allocation->size, &mem_id, &user_logical, &physical,
        allocation->align, alloc_flag, gen_mmu_id(vipdrv_os_get_pid(), 0),
        allocation->device_vis, allocation->specified);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate video memory, size=0x%x, status=%d\n",
                  allocation->size, status);
        vipGoOnError(status);
    }

    if (VIPDRV_VIDEO_MEM_ALLOC_MAP_USER & alloc_flag) {
        allocation->logical = VIPDRV_PTR_TO_UINT64(user_logical);
    }
    else {
        allocation->logical = VIP_NULL;
    }
    allocation->mem_id = mem_id;
    allocation->physical = physical;
    PRINTK_D("allocate video memory size=0x%x, align=0x%x, type=0x%x, mem_id=0x%x\n",
              allocation->size, allocation->align, allocation->alloc_flag, mem_id);

    return status;
onError:
    allocation->logical = 0;
    allocation->physical = 0;
    allocation->mem_id = 0;

    return status;
}

/*
@brief free video memory
*/
static vip_status_e do_free_videomem(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_deallocation_t *allocation = (vipdrv_deallocation_t *)data;
    VIPDRV_CHECK_INIT();

    status = vipdrv_mem_free_videomemory(allocation->mem_id);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to free video memory mem_id=0x%x\n", allocation->mem_id);
        vipGoOnError(status);
    }

onError:
    return status;
}

/*
@brief mapping user space logical address.
*/
static vip_status_e do_map_user_logical(void *data)
{
    vipdrv_map_user_logical_t *map = (vipdrv_map_user_logical_t*)data;
    vip_status_e status = VIP_SUCCESS;
    void *logical = VIP_NULL;
    VIPDRV_CHECK_INIT();

    vipOnError(vipdrv_mem_map_userlogical(map->mem_id, &logical));
    map->logical = VIPDRV_PTR_TO_UINT64(logical);

    PRINTK_I("map user logical=0x%"PRIx64", mem_id=0x%x\n", map->logical, map->mem_id);
    return status;
onError:
    PRINTK_E("fail to map mem_id=0x%x user logical\n", map->mem_id);
    return status;
}

/*
@brief unmapping user space logical address.
*/
static vip_status_e do_unmap_user_logical(void *data)
{
    vipdrv_unmap_user_logical_t *unmap = (vipdrv_unmap_user_logical_t*)data;
    vip_status_e status = VIP_SUCCESS;
    VIPDRV_CHECK_INIT();

    vipOnError(vipdrv_mem_unmap_userlogical(unmap->mem_id));

    return status;
onError:
    PRINTK_E("fail to unmap user logical mem_id=0x%x\n", unmap->mem_id);
    return status;
}

static vip_status_e do_wait_task(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_wait_task_param_t *wait_para = (vipdrv_wait_task_param_t *)data;
    vipIsNULL(wait_para);
    VIPDRV_CHECK_INIT();

    status = vipdrv_task_desc_wait(wait_para);
    if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
        PRINTK_E("fail to do cmd wait, status=%d.\n", status);
    }

onError:
    return status;
}

#if vpmdENABLE_CANCELATION
static vip_status_e do_cancel_task(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_cancel_task_param_t *cancel = (vipdrv_cancel_task_param_t*)data;
    vipIsNULL(cancel);
    VIPDRV_CHECK_INIT();

    status = vipdrv_task_tcb_cancel(cancel);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to do cmd cancel, status=%d.\n", status);
    }

onError:
    return status;
}
#endif

static vip_status_e do_submit_task(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_submit_task_param_t *sbmt_para = (vipdrv_submit_task_param_t*)data;
    vipIsNULL(sbmt_para);
    VIPDRV_CHECK_INIT();

    status = vipdrv_task_desc_submit(sbmt_para);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to do cmd submit status=%d.\n", status);
        vipGoOnError(status);
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
    vipdrv_query_address_info_t *info = (vipdrv_query_address_info_t*)data;

    info->waitlink_logical = 0;
    info->waitlink_size = 0;
    info->waitlink_physical = 0;

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
    vipdrv_query_database_t *database = (vipdrv_query_database_t*)data;
    vipdrv_context_t *context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    VIPDRV_CHECK_INIT();

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_lock_mutex(context->initialize_mutex);
    }
#endif

    if (context->initialize > 0) {
        database->axi_sram_base = context->axi_sram_address;
        database->vip_sram_base = context->vip_sram_address;
        database->func_config.enable_capture = context->func_config.enable_capture;
        database->func_config.enable_cnn_profile = context->func_config.enable_cnn_profile;
        database->func_config.enable_dump_nbg = context->func_config.enable_dump_nbg;
        database->hw_cid = context->chip_cid;
        database->core_count = context->core_count;
        database->max_core_count = context->max_core_count;
        database->device_count = context->device_count;
    #if vpmdENABLE_SYS_MEMORY_HEAP
        database->sys_heap_size = context->sys_heap_size;
    #endif

        for (iter0 = 0; iter0 < context->device_count; iter0++) {
            vipdrv_device_t *device = &context->device[iter0];
            database->device_core_number[iter0] = device->hardware_count;
            database->axi_sram_size[iter0] = context->axi_sram_size[iter0];
            database->vip_sram_size[iter0] = context->vip_sram_size[iter0];
            for (iter1 = 0; iter1 < device->hardware_count; iter1++) {
                vipdrv_hardware_t *hardware = &device->hardware[iter1];
                database->device_core_id[iter0][iter1] = hardware->core_id;
            }
        }
    }
    else {
        PRINTK_E("fail to query databse, please call do init first\n");
        status = VIP_ERROR_IO;
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_unlock_mutex(context->initialize_mutex);
    }
#endif

    return status;
}

/*
@brief query the vip-drv status.
*/
static vip_status_e do_query_driver_status(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t *device = VIP_NULL;
    vipdrv_query_driver_status_t *drv_status = (vipdrv_query_driver_status_t*)data;
#if vpmdENABLE_MULTIPLE_TASK
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
#endif
    VIPDRV_CHECK_INIT();
    vipIsNULL(drv_status);

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_lock_mutex(context->initialize_mutex);
    }
#endif
    device = vipdrv_get_device(drv_status->device_index);
    vipIsNULL(device);
    drv_status->commit_count = device->hardware[0].user_submit_count;

    /* for saving initialize command into capture file */
    drv_status->init_cmd_physical = device->init_command.physical;
    drv_status->init_cmd_size = device->init_command_size;
    drv_status->init_cmd_handle = device->init_command.mem_id;

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_unlock_mutex(context->initialize_mutex);
    }
#endif
    return status;
}

/*
@brief flush CPU cache for video memory heap.
*/
#if vpmdENABLE_FLUSH_CPU_CACHE
static vip_status_e do_flush_cache(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_operate_cache_t *operate = (vipdrv_operate_cache_t*)data;
    VIPDRV_CHECK_INIT();

    if (VIP_NULL == operate) {
        PRINTK_E("fail to flush cache, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    vipOnError(vipdrv_mem_flush_cache(operate->mem_id, operate->type));
    return status;

onError:
    PRINTK_E("fail to do flush cache. mem_id=0x%x, type=%d\n", operate->mem_id, operate->type);
    return status;
}
#endif

static vip_status_e do_wrap_user_physical(void *data)
{
    vip_status_e status = VIP_SUCCESS;

    vipdrv_wrap_user_physical_t *memory = (vipdrv_wrap_user_physical_t*)data;
    vip_uint32_t mem_id = 0;
    VIPDRV_CHECK_INIT();

    memory->mem_id = 0;
    status = vipdrv_mem_wrap_userphysical(memory->physical_table,
                                          memory->size_table, memory->physical_num,
                                          memory->alloc_flag, memory->memory_type,
                                          memory->device_vis, &memory->virtual_addr,
                                          (void**)&memory->logical, &mem_id);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to wrap user physical, status=%d\n", status);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    PRINTK_D("wrap user physical, physical[0]=0x%"PRIx64", physical number=%d, "
             "visibility=0x%"PRIx64", virtual=0x%08x, mem_id=0x%x\n",
             memory->physical_table[0], memory->physical_num, memory->virtual_addr, mem_id);

    memory->mem_id = mem_id;
onError:
    return status;
}

/*
@brief, wrap user memory to VIP. return VIP's virtual address and video memory handle to use space.
*/
static vip_status_e do_wrap_user_memory(void *data)
{
    vip_status_e status = VIP_SUCCESS;

    vipdrv_wrap_user_memory_t *memory = (vipdrv_wrap_user_memory_t*)data;
    vip_uint32_t mem_id = 0;
    VIPDRV_CHECK_INIT();

    memory->mem_id = 0;
    status = vipdrv_mem_wrap_usermemory(VIPDRV_UINT64_TO_PTR(memory->logical), memory->size,
        memory->memory_type, memory->device_vis, &memory->virtual_addr, &mem_id);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to wrap user memory, status=%d\n", status);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    PRINTK_D("wrap user memory, logical=0x%"PRIx64", visibility=0x%"PRIx64", size=0x%08x, "
             "virtual=0x%08x, mem_id=0x%x\n",
             memory->logical, memory->size, memory->virtual_addr, mem_id);

    memory->mem_id = mem_id;

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
    vipdrv_wrap_user_fd_t *memory = (vipdrv_wrap_user_fd_t*)data;
    vip_uint8_t *logical = VIP_NULL;
    vip_uint32_t mem_id = 0;
    VIPDRV_CHECK_INIT();

    memory->mem_id = 0;
    status = vipdrv_mem_wrap_userfd(memory->fd, memory->size, memory->memory_type,
        memory->device_vis, &memory->virtual_addr, &logical, &mem_id);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to wrap user fd, status=%d\n", status);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    PRINTK_D("wrap user fd=%d, size=0x%08x, visibility=0x%"PRIx64", virtual=0x%08x, mem_id=0x%x\n",
              memory->fd, memory->size, memory->device_vis, memory->virtual_addr, mem_id);

    memory->mem_id = mem_id;
    memory->logical = VIPDRV_PTR_TO_UINT64(logical);

onError:
    return status;
}
#endif

#if vpmdENABLE_TASK_PROFILE
static vip_status_e do_query_profiling(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_query_profiling_t *profiling = (vipdrv_query_profiling_t*)data;
    VIPDRV_CHECK_INIT();
    vipIsNULL(data);

    status = vipdrv_task_profile_query(profiling);
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
    vipdrv_query_mmu_info_t *info = (vipdrv_query_mmu_info_t*)data;

    vipIsNULL(data);
    vipOnError(vipdrv_mmu_query_mmu_info(info));

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
static vip_status_e do_power_management(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t *device = VIP_NULL;
    vipdrv_power_management_t *power = (vipdrv_power_management_t*)data;
    vipIsNULL(power);

    device = vipdrv_get_device(power->device_index);
    vipIsNULL(device);

#if vpmdENABLE_POWER_MANAGEMENT
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    status |= vipdrv_pm_power_management(hw, power);
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to set power management device%d.\n", power->device_index);
    }
#else
    status = VIP_ERROR_NOT_SUPPORTED;
    PRINTK_E("power management is disabled.\n");
#endif
onError:
    return status;
}

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
static vip_status_e do_query_register_dump(void* data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_query_register_dump_t* info = (vipdrv_query_register_dump_t*)data;
    vipdrv_context_t *context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipIsNULL(info);

    info->physical = context->register_dump_buffer.physical;
    info->count = context->register_dump_count;
    info->mem_id = context->register_dump_buffer.mem_id;

    PRINTK_D("register dump in vip-drv: mem_id=0x%x, physical=0x%x, count=0x%x.\n",
        info->mem_id, info->physical, info->count);

onError:
    return status;
}
#endif

static vip_status_e do_create_task(void* data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_create_task_param_t *pram = (vipdrv_create_task_param_t*)data;
    VIPDRV_CHECK_INIT();
    vipIsNULL(pram);

    status = vipdrv_task_desc_create(pram);
onError:
    return status;
}

static vip_status_e do_set_task_property(void* data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_set_task_property_t *task_property = (vipdrv_set_task_property_t*)data;
    VIPDRV_CHECK_INIT();
    vipIsNULL(task_property);

    status = vipdrv_task_desc_set_property(task_property);
onError:
    return status;
}

static vip_status_e do_destroy_task(void *data)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_destroy_task_param_t *param = (vipdrv_destroy_task_param_t*)data;
    VIPDRV_CHECK_INIT();
    vipIsNULL(param);

    status = vipdrv_task_desc_destroy(param);

onError:
    return status;
}

/*
@brief Destroy vip-drv space resource.
       This function only use on Linux/Andorid system when used ctrl+c to exit application or
       application crash happend.
       When the appliction exits abnormally, vipdrv_destroy() used to free all kenerl space resource.
*/
vip_status_e vipdrv_destroy(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t process_id = vipdrv_os_get_pid();
    vip_bool_e need_destroy = vip_false_e;
    vip_uint32_t i = 0;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

    /* destroy resource */
#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
    mutex_lock(&kinit_mutex);
#endif
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_lock_mutex(context->initialize_mutex);
    }
#endif
    if (context->initialize > 0) {
        vipdrv_hashmap_get_by_handle(&context->process_id, (vip_uint64_t)process_id, &i, VIP_NULL);
        if (0 == i) {
            need_destroy = vip_false_e;
        }
        else {
            need_destroy = vip_true_e;
            vipdrv_hashmap_unuse(&context->process_id, i, vip_false_e);
        }
    }
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_unlock_mutex(context->initialize_mutex);
    }
#if defined(LINUX)
    mutex_unlock(&kinit_mutex);
#endif
#endif

    if (vip_true_e == need_destroy) {
        vipdrv_os_delay(60);

        /* force destroy all related tasks */
        vipdrv_task_desc_force_destroy(process_id);

        /* free all video memory which created by this porcess */
        vipdrv_mem_force_free_videomemory(process_id);

        PRINTK_I("start to destroy resource, pid=%x, tid=%x\n", process_id, vipdrv_os_get_tid());
        do_destroy();
    }
    else {
        PRINTK_I("vip destroyed or other tasks running, initialize=%d\n", context->initialize);
    }

    return status;
}

#if vpmdENABLE_AW_DEVFREQ
vip_status_e vipdrv_set_internal_core_freq(vip_uint32_t fscale)
{
	vip_status_e status = VIP_SUCCESS;
	vip_uint32_t dev_index = 0;
	vipdrv_power_management_t power;

	vipdrv_context_t *context = (vipdrv_context_t *)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

	power.property = VIPDRV_POWER_PROPERTY_SET_FREQUENCY;
	power.fscale_percent = fscale;

#if vpmdENABLE_MULTIPLE_TASK
#if defined(LINUX)
	mutex_lock(&kinit_mutex);
#endif
	if (context->initialize_mutex != VIP_NULL) {
		vipdrv_os_lock_mutex(context->initialize_mutex);
	}
#endif
	if (context->initialize > 0) {
		for (dev_index = 0; dev_index < context->device_count; dev_index++) {
			power.device_index = dev_index;
			do_power_management(&power);
		}
	}
#if vpmdENABLE_MULTIPLE_TASK
	if (context->initialize_mutex != VIP_NULL) {
		vipdrv_os_unlock_mutex(context->initialize_mutex);
	}
#if defined(LINUX)
	mutex_unlock(&kinit_mutex);
#endif
#endif

	return status;
}
#endif

/*
@brief  initialize driver
*/
vip_status_e vipdrv_init(
    vipdrv_initialize_t *init_data
    )
{
    vip_status_e status = VIP_SUCCESS;
    status = do_init((void*)init_data);
    return status;
}

/*
@brief  dispatch vip_drv command
@param  command, command type.
@param  data, the data for command type.
*/
vip_status_e vipdrv_interface_call(
    vipdrv_intrface_e command,
    void *data
    )
{
    vip_status_e status = VIP_SUCCESS;
    switch (command)
    {
    case VIPDRV_INTRFACE_INIT:           /* VIP initialization command. */
        status = do_init(data);
        break;

    case VIPDRV_INTRFACE_DESTROY:        /* VIP terminate command. */
        status = do_destroy();
        break;

    case VIPDRV_INTRFACE_READ_REG:       /* VIP register read command. */
        status = do_read_register(data);
        break;

    case VIPDRV_INTRFACE_WRITE_REG:      /* VIP register write command. */
        status = do_write_register(data);
        break;

    case VIPDRV_INTRFACE_WAIT_TASK:     /* VIP interrupt wait task. */
        status = do_wait_task(data);
        break;

    case VIPDRV_INTRFACE_SUBMIT_TASK:    /* VIP command buffer submit task. */
        status = do_submit_task(data);
        break;

    case VIPDRV_INTRFACE_ALLOCATION:     /* VIP buffer allocation command. */
        status = do_allocation_videomem(data);
        break;

    case VIPDRV_INTRFACE_DEALLOCATION:      /* VIP memory reset command. */
        status = do_free_videomem(data);
        break;

    case VIPDRV_INTRFACE_MAP_USER_LOGICAL:
        status = do_map_user_logical(data);
        break;
    case VIPDRV_INTRFACE_UNMAP_USER_LOGICAL:
        status = do_unmap_user_logical(data);
        break;

    case VIPDRV_INTRFACE_QUERY_DATABASE: /* query vip sram  and axi sram info */
        status= do_query_database(data);
        break;

    case VIPDRV_INTRFACE_QUERY_DRIVER_STATUS:
        status= do_query_driver_status(data);
        break;

    case VIPDRV_INTRFACE_WRAP_USER_PHYSICAL:  /* wrap user physical memory to VIP */
        status = do_wrap_user_physical(data);
        break;

    case VIPDRV_INTRFACE_UNWRAP_USER_PHYSICAL: /* un-warp physical memory */
        status = do_free_videomem(data);
        break;

    case VIPDRV_INTRFACE_WRAP_USER_MEMORY: /* wrap user logical address memory to VIP virtual address */
        status = do_wrap_user_memory(data);
        break;

    case VIPDRV_INTRFACE_UNWRAP_USER_MEMORY: /* un-warp user memory to VIP virtual */
        status = do_free_videomem(data);
        break;

#if vpmdENABLE_CANCELATION
    case VIPDRV_INTRFACE_CANCEL_TASK:           /* VIP cancel task. */
        status = do_cancel_task(data);
        break;
#endif

#if vpmdENABLE_FLUSH_CPU_CACHE
    case VIPDRV_INTRFACE_OPERATE_CACHE:    /* flush/clean/invalid cpu cache */
        status = do_flush_cache(data);
        break;
#else
	case VIPDRV_INTRFACE_OPERATE_CACHE:
		break;
#endif

#if vpmdENABLE_USE_AGENT_TRIGGER
    case VIPDRV_INTRFACE_QUERY_ADDRESS_INFO: /* query viplite's wait-link buffer address information */
        status = do_query_address_info(data);
        break;
#endif

#if vpmdENABLE_CREATE_BUF_FD
    case VIPDRV_INTRFACE_WRAP_USER_FD:
        status = do_wrap_user_fd(data);
        break;
    case VIPDRV_INTRFACE_UNWRAP_USER_FD:
        status = do_free_videomem(data);
        break;
#endif

#if vpmdENABLE_TASK_PROFILE
    case VIPDRV_INTRFACE_QUERY_PROFILING:
        status = do_query_profiling(data);
        break;
#endif

    case VIPDRV_INTRFACE_POWER_MANAGEMENT:
        status = do_power_management(data);
        break;

#if vpmdENABLE_CAPTURE_MMU
    case VIPDRV_INTRFACE_QUERY_MMU_INFO:
        status = do_query_mmu_info(data);
        break;
#endif
#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
    case VIPDRV_INTRFACE_QUERY_REGISTER_DUMP:
        status = do_query_register_dump(data);
        break;
#endif
    case VIPDRV_INTRFACE_CREATE_TASK:
        status = do_create_task(data);
        break;
    case VIPDRV_INTRFACE_SET_TASK_PROPERTY:
        status = do_set_task_property(data);
        break;
    case VIPDRV_INTRFACE_DESTROY_TASK:
        status = do_destroy_task(data);
        break;
    default:
        status = VIP_ERROR_INVALID_ARGUMENTS;
        PRINTK_E("vip-drv doesn't support this command=%s\n", ioctl_cmd_string[command]);
        break;
    };
    return status;
}

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

#include <vip_drv_task_common.h>
#if vpmdTASK_PARALLEL_ASYNC
#include <vip_drv_context.h>
#include <vip_drv_device_driver.h>
#include <vip_drv_debug.h>
#include <vip_drv_task_descriptor.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_TASK


static vip_status_e query_hardware_dispatch(
    vipdrv_device_t* device,
    vip_uint16_t* flag
    )
{
    vip_uint16_t mask = 0;

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    if ((hw->wl_end_index + 1) % WL_TABLE_COUNT != hw->wl_start_index) {
        /* wait-link table is not full */
        mask |= (1 << hw->core_id);
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

    *flag = mask;
    PRINTK_D("query device %d dispatch 0x%x\n", device->device_index, mask);

    return VIP_SUCCESS;
}

static vip_status_e set_hardware_dispatch(
    vipdrv_task_control_block_t *tskTCB
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_task_t* task = &tskTCB->task;

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    hw->wl_table[hw->wl_end_index].tcb_index = tskTCB->queue_data->v1;
    hw->wl_end_index = (hw->wl_end_index + 1) % WL_TABLE_COUNT;
    VIPDRV_LOOP_HARDWARE_IN_TASK_END

    return status;
}

static vip_int32_t vipdrv_task_submit_thread_handle(
    vip_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t* device = (vipdrv_device_t*)param;
    vipdrv_queue_t* queue = &device->tskTCB_queue;
    vipdrv_queue_data_t* queue_data = VIP_NULL;
    vipdrv_task_control_block_t* tskTCB = VIP_NULL;
    vip_uint32_t device_index = device->device_index;
    vip_uint16_t flag = 0;
    vipdrv_hashmap_t* tskTCB_hashmap = &device->context->tskTCB_hashmap;

    PRINTK_I("submit-daemon thread start\n");
    vipdrv_os_set_signal(device->submit_thread_exit_signal, vip_false_e);

    while (vip_true_e == device->tskTCB_thread_running) {
        PRINTK_I("submit-daemon thread wait submit signal\n");
        vipdrv_os_wait_signal(device->tskTCB_submit_signal, vpmdINFINITE);
        vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_false_e);
        PRINTK_I("submit-daemon thread start read a task\n");
        while (VIP_SUCCESS == query_hardware_dispatch(device, &flag) &&
            vip_true_e == vipdrv_queue_read(queue, &queue_data, (void*)&flag)) {
            TASK_COUNTER_HOOK(device, read_queue);
            if (VIP_NULL == queue_data) {
                PRINTK_E("multi-task dev%d read queue data is NULL\n", device_index);
                continue;
            }
            vipdrv_os_wait_signal(device->tskTCB_recover_signal, vpmdINFINITE);
            vipdrv_os_inc_atomic(device->disable_recover);
            TASK_COUNTER_HOOK(device, recover_done);

            vipdrv_hashmap_get_by_index(tskTCB_hashmap, queue_data->v1, (void**)&tskTCB);
            if (VIP_NULL == tskTCB) {
                PRINTK_E("submit-daemon thread tskTCB is NULL v1=%d\n", queue_data->v1);
                continue;
            }

            #if vpmdENABLE_POWER_MANAGEMENT
            vipdrv_pm_lock_device_hw(device, tskTCB->task.core_index, tskTCB->task.core_cnt);
            #endif
            vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
            if (VIPDRV_TASK_READY != tskTCB->status) {
                vipdrv_os_dec_atomic(device->disable_recover);
                PRINTK_I("multi-task dev%d task status=0x%x not need submit\n", device_index, tskTCB->status);
                vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                #if vpmdENABLE_POWER_MANAGEMENT
                vipdrv_pm_unlock_device_hw(device, tskTCB->task.core_index, tskTCB->task.core_cnt);
                #endif
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tskTCB->queue_data->v1, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, tskTCB->queue_data->v1, vip_true_e);
                continue;
            }
            PRINTK_I("submit-daemon thread submit command task_id=0x%x\n",
                     tskTCB->task.task_id);
            /* wake up the thread of first core to wait for task inference done */
            status = vipdrv_task_submit(&tskTCB->task);
            tskTCB->status = VIPDRV_TASK_INFER_START;
            TASK_COUNTER_HOOK(device, submit_hw);
            /* set hardwares are distribute state */
            set_hardware_dispatch(tskTCB);
            vipdrv_os_dec_atomic(device->disable_recover);
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            #if vpmdENABLE_POWER_MANAGEMENT
            vipdrv_pm_unlock_device_hw(device, tskTCB->task.core_index, tskTCB->task.core_cnt);
            #endif
            vipdrv_hashmap_unuse(tskTCB_hashmap, queue_data->v1, vip_false_e);
            *device->irq_value |= IRQ_VALUE_FLAG;
            vipdrv_os_wakeup_interrupt(device->irq_queue);
        }
    }

    /* wake up wait the thread exit task */
    vipdrv_os_set_signal(device->submit_thread_exit_signal, vip_true_e);
    PRINTK_I("submit-daemon thread exit\n");

    return status;
}

static vip_int32_t vipdrv_task_wait_thread_handle(
    vip_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_status_e recover_status = VIP_SUCCESS;
    vipdrv_device_t* device = (vipdrv_device_t*)param;
    vipdrv_task_control_block_t* tskTCB = VIP_NULL;
    vipdrv_task_control_block_t* temp_task = VIP_NULL;
    vip_bool_e re_proflie = vip_false_e;
    vip_uint8_t recover_flag[vipdMAX_CORE] = { 0 };
    vip_uint32_t* cancel_task = VIP_NULL;
    vip_uint32_t cancel_index = 0;
    vip_uint32_t i = 0;
    vip_uint32_t timeout = ~0;
    vipdrv_context_t* context = device->context;
    vipdrv_hashmap_t* tskTCB_hashmap = &context->tskTCB_hashmap;

    PRINTK_I("wait-daemon thread start\n");
    vipdrv_os_set_signal(device->wait_thread_exit_signal, vip_false_e);

    while (vip_true_e == device->tskTCB_thread_running) {
        PRINTK_D("wait-daemon thread start wait irq signal\n");
        status = vipdrv_os_wait_interrupt(device->irq_queue, device->irq_value, timeout, IRQ_VALUE_FLAG);
        if (vip_false_e == device->tskTCB_thread_running) {
            break;
        }
        *device->irq_value &= (~IRQ_VALUE_FLAG);
        timeout = ~0;

        VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
        re_proflie = vip_false_e;
        while (1) {
            if (hw->wl_start_index == hw->wl_end_index) {
                #if vpmdENABLE_POWER_MANAGEMENT
                vipdrv_pm_lock_hardware(hw);
                if (hw->wl_start_index == hw->wl_end_index) {
                #endif
                    /* hardware is idle */
                    vipdrv_hw_idle(hw);
                #if vpmdENABLE_POWER_MANAGEMENT
                }
                vipdrv_pm_unlock_hardware(hw);
                #endif
                break;
            }
            vipdrv_hashmap_get_by_index(tskTCB_hashmap,
                hw->wl_table[hw->wl_start_index].tcb_index, (void**)&tskTCB);
            if (VIP_NULL == tskTCB) {
                PRINTK_E("wait-daemon thread submit is null\n");
                break;
            }
            tskTCB->hw_ready_mask |= 1 << hw->core_id;
            if (vip_true_e == re_proflie) {
                vipdrv_task_profile_start(&tskTCB->task);
            }

            if (hw_index == tskTCB->task.core_index) {
                /* interrupt will send to the first core */
                if (0 < vipdrv_os_get_atomic(*hw->irq_count)) {
                    vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
                    status = vipdrv_task_check_irq_value(hw);
                    PRINTK_D("wait-daemon thread  wait done task_id=0x%x\n",
                              tskTCB->task.task_id);
                    if (VIP_SUCCESS != status) {
                        vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_false_e);
                        vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                        goto Exception;
                    }
                    vipdrv_task_profile_end(&tskTCB->task);
                    vipdrv_os_dec_atomic(*hw->irq_count);
                    tskTCB->queue_data->v2 = status;
                    tskTCB->status = VIPDRV_TASK_INFER_END;
                    TASK_COUNTER_HOOK(hw, wait_hw);
                    vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                }
            }

            if (VIPDRV_TASK_INFER_END == tskTCB->status) {
                if (hw_index == tskTCB->task.core_index + tskTCB->task.core_cnt - 1) {
                    /* last core wake up user wait */
                    vipdrv_hashmap_unuse(tskTCB_hashmap, tskTCB->queue_data->v1, vip_true_e);
                }
                else {
                    vipdrv_hashmap_unuse(tskTCB_hashmap, tskTCB->queue_data->v1, vip_false_e);
                }
                hw->wl_start_index = (hw->wl_start_index + 1) % WL_TABLE_COUNT;
                /* wake up submit daemon thread */
                vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_true_e);
                re_proflie = vip_true_e; /* will link to next task */
            }
            else {
                /* check resource is enough */
                if (tskTCB->queue_data->res_mask == (tskTCB->hw_ready_mask & tskTCB->queue_data->res_mask)) {
                    vipdrv_task_descriptor_t *tsk_desc = VIP_NULL;
                    vip_uint64_t cur_time = vipdrv_os_get_time();
                    vip_uint64_t start_time = cur_time;
                    vip_uint32_t index = vipdrv_task_desc_id2index(tskTCB->task.task_id);
                    vip_uint32_t rest_time = 0;
                    if (VIP_SUCCESS == vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
                        start_time = tsk_desc->subtasks[tskTCB->task.subtask_index].cur_time;
                        vipdrv_database_unuse(&context->tskDesc_database, index);
                    }
                    VIPDRV_SUB(cur_time, start_time, cur_time, vip_uint64_t);
                    /* check timeout */
                    if (cur_time > (vip_uint64_t)tskTCB->time_out * 1000) {
                        vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_false_e);
                        PRINTK("timeout cur_time=%d,  task setting time_out=%lld\n",
                                cur_time, (vip_uint64_t)tskTCB->time_out * 1000);
                        status = VIP_ERROR_TIMEOUT;
                        goto Exception;
                    }
                    rest_time = tskTCB->time_out - (vip_uint32_t)(cur_time / 1000);
                    timeout = rest_time < timeout ? rest_time : timeout;
                    PRINTK_D("wait-daemon thread task=0x%x, timeout=%dms, rest_time=0x%"PRIx64", " \
                             "cur_time=0x%"PRIx64", start_time=0x%"PRIx64"\n",
                             tskTCB->task.task_id, tskTCB->time_out,
                             rest_time, cur_time, start_time);
                }
                /* wait current task */
                vipdrv_hashmap_unuse(tskTCB_hashmap, tskTCB->queue_data->v1, vip_false_e);
                break;
            }
        }
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

        continue;
    Exception:
        PRINTK_I("wait-daemon start exception handle status=%d\n", status);
        #if vpmdENABLE_HANG_DUMP
        if (VIP_ERROR_TIMEOUT == status) {
            vipdrv_hang_dump(&tskTCB->task);
        }
        #endif
        vipdrv_os_zero_memory(recover_flag, sizeof(vip_uint8_t) * vipdMAX_CORE);
        vipdrv_os_allocate_memory(sizeof(vip_uint32_t) * WL_TABLE_COUNT * context->core_count, (void**)&cancel_task);
        if (VIP_NULL == cancel_task) {
            PRINTK_E("fail to alloc memory for cancel_task\n");
            continue;
        }
        vipdrv_os_zero_memory(cancel_task, sizeof(vip_uint32_t)* WL_TABLE_COUNT* context->core_count);
        cancel_index = 0;

        /* mark which hw need recover */
        recover_flag[tskTCB->task.core_index] = 1;
        while (vipdrv_os_get_atomic(device->disable_recover)) {
            vipdrv_os_udelay(10);
        }
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
        if ((*hw->irq_value | hw->irq_local_value) & IRQ_VALUE_CANCEL) {
            recover_flag[i] = 1;
        }
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

        /* handle all related task instead of only current task */
        while (1) {
            vip_bool_e goon = vip_false_e;
            VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
            if (1 == recover_flag[i]) {
                vip_uint32_t index = 0;
                index = hw->wl_start_index;
                goon = vip_true_e;

                while (index != hw->wl_end_index) {
                    vipdrv_hashmap_get_by_index(tskTCB_hashmap,
                        hw->wl_table[index].tcb_index, (void**)&temp_task);
                    if (VIP_NULL != temp_task) {
                        if (VIPDRV_TASK_INFER_START == temp_task->status) {
                            vip_uint32_t i = 0;
                            for (i = 0; i < temp_task->task.core_cnt; i++) {
                                recover_flag[temp_task->task.core_index + i] =
                                    recover_flag[temp_task->task.core_index + i] > 1 ?
                                    recover_flag[temp_task->task.core_index + i] : 1;
                            }

                            temp_task->status = VIPDRV_TASK_CANCELED;
                            temp_task->queue_data->v2 = VIP_ERROR_CANCELED;
                            if (temp_task != tskTCB) {
                                cancel_task[cancel_index++] = temp_task->queue_data->v1;
                            }
                        }
                        vipdrv_hashmap_unuse(tskTCB_hashmap, temp_task->queue_data->v1, vip_false_e);
                    }
                    index = (index + 1) % WL_TABLE_COUNT;
                }
                recover_flag[i] = 2;
            }
            VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
            if (vip_false_e == goon) {
                break;
            }
        }

        #if (!vpmdENABLE_RECOVERY) && vpmdENABLE_CANCELATION
        /* recover hardware */
        if (VIP_ERROR_TIMEOUT == status) {
            tskTCB->queue_data->v2 = VIP_ERROR_TIMEOUT;
        }
        else
        #endif
        #if vpmdENABLE_RECOVERY || vpmdENABLE_CANCELATION
        {
            recover_status = vipdrv_task_recover(device, recover_flag);
            if (VIP_SUCCESS == recover_status) {
                if (VIP_ERROR_TIMEOUT == status) {
                    tskTCB->queue_data->v2 = VIP_ERROR_RECOVERY;
                }
                else {
                    tskTCB->queue_data->v2 = VIP_ERROR_CANCELED;
                }
            }

            for (i = 0; i < device->hardware_count; i++) {
                if (recover_flag[i]) {
                    vipdrv_hardware_t* hardware = &device->hardware[i];
                    VIPDRV_CHECK_NULL(hardware);
                    PRINTK("hardware %d recovery done.\n", hardware->core_id);
                    hardware->hw_submit_count = 0;
                    hardware->user_submit_count = 0;
                    hardware->wl_start_index = 0;
                    hardware->wl_end_index = 0;
                }
            }
        }
        #endif
        vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tskTCB->queue_data->v1, vip_false_e);
        vipdrv_hashmap_unuse(tskTCB_hashmap, tskTCB->queue_data->v1, vip_true_e);
        vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_true_e);
        vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_true_e);

        cancel_index = 0;
        while (cancel_task[cancel_index]) {
            vipdrv_hashmap_get_by_index(tskTCB_hashmap, cancel_task[cancel_index], (void**)&temp_task);
            if (VIP_NULL != temp_task) {
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, temp_task->queue_data->v1, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, temp_task->queue_data->v1, vip_true_e);
            }
            cancel_index++;
        }
        vipdrv_os_free_memory(cancel_task);
        cancel_task = VIP_NULL;
    }

    /* wake up wait the thread exit task */
    vipdrv_os_set_signal(device->wait_thread_exit_signal, vip_true_e);
    PRINTK_I("wait-daemon thread exit\n");
    return status;
}

vip_status_e vipdrv_task_pre_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;

    VIPDRV_LOOP_DEVICE_START
    device->tskTCB_thread_running = vip_false_e;
    device->tskTCB_queue.queue_stop = vip_true_e;
    /* exit multi-thread process */
    vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_true_e);
    vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_true_e);
    vipdrv_os_set_atomic(device->disable_recover, 0);
    *device->irq_value |= IRQ_VALUE_FLAG;
    vipdrv_os_wakeup_interrupt(device->irq_queue);
    vipdrv_os_wait_signal(device->wait_thread_exit_signal, WAIT_SIGNAL_TIMEOUT);
    vipdrv_os_wait_signal(device->submit_thread_exit_signal, WAIT_SIGNAL_TIMEOUT);
    VIPDRV_LOOP_DEVICE_END

    return status;
}

vip_status_e vipdrv_task_init(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_char_t name[256] = { 0 };

    VIPDRV_LOOP_DEVICE_START
    status = vipdrv_os_create_signal(&device->tskTCB_submit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a signal submit task\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_false_e);
    status = vipdrv_os_create_signal(&device->submit_thread_exit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a signal for submit thread\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    status = vipdrv_os_create_signal(&device->tskTCB_recover_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a signal for recover\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_true_e);

    status = vipdrv_os_create_atomic(&device->disable_recover);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for disable recover\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_set_atomic(device->disable_recover, 0);

    status = vipdrv_os_create_signal(&device->wait_thread_exit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a signal for exit thread\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    device->tskTCB_thread_running = vip_true_e;

    vipdrv_os_snprint(name, 255, "device_submit_thread_%d", device->device_index);
    status = vipdrv_os_create_thread((vipdrv_thread_func)vipdrv_task_submit_thread_handle,
        (vip_ptr)device, name, &device->submit_thread_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a thread for supporting multi-thread\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    vipdrv_os_snprint(name, 255, "device_wait_thread_%d", device->device_index);
    status = vipdrv_os_create_thread((vipdrv_thread_func)vipdrv_task_wait_thread_handle,
        (vip_ptr)device, name, &device->wait_thread_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create a thread for supporting multi-thread\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    VIPDRV_LOOP_DEVICE_END

onError:
    return status;
}

vip_status_e vipdrv_task_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;
    VIPDRV_LOOP_DEVICE_START
    if (device->submit_thread_handle != VIP_NULL) {
        vipdrv_os_destroy_thread(device->submit_thread_handle);
    }
    if (device->submit_thread_exit_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(device->submit_thread_exit_signal);
    }
    if (device->wait_thread_handle != VIP_NULL) {
        vipdrv_os_destroy_thread(device->wait_thread_handle);
    }
    if (device->wait_thread_exit_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(device->wait_thread_exit_signal);
    }
    if (device->tskTCB_recover_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(device->tskTCB_recover_signal);
    }
    if (device->disable_recover != VIP_NULL) {
        vipdrv_os_destroy_atomic(device->disable_recover);
        device->disable_recover = VIP_NULL;
    }
    if (device->tskTCB_submit_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(device->tskTCB_submit_signal);
    }
    VIPDRV_LOOP_DEVICE_END

    return status;
}

#if vpmdTASK_DISPATCH_DEBUG
vip_status_e vipdrv_task_dispatch_profile(
    vipdrv_device_t* device,
    vipdrv_task_dispatch_profile_e type
    )
{
    vip_status_e status = VIP_SUCCESS;
    switch (type)
    {
    case VIPDRV_TASK_DIS_PROFILE_SHOW:
    {
        PRINTK("DEVICE %d TASK DISPATCH PROFILE START\n", device->device_index);
        PRINTK("device read from task quque : %lld\n", device->read_queue_cnt);
        PRINTK("device submit to HW         : %lld\n", device->submit_hw_cnt);
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
        PRINTK("core %d thread wait HW done  : %lld\n", hw->core_index, hw->wait_hw_cnt);
        PRINTK("core %d wait-link start      : %lld\n", hw->core_index, hw->wl_start_index);
        PRINTK("core %d wait-link end        : %lld\n", hw->core_index, hw->wl_end_index);
        PRINTK("core %d current irq          : %lld\n", hw->core_index, vipdrv_os_get_atomic(*hw->irq_count));
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
        PRINTK("DEVICE %d TASK DISPATCH PROFILE END\n", device->device_index);
    }
    break;

    case VIPDRV_TASK_DIS_PROFILE_RESET:
    {
        device->read_queue_cnt = 0;
        device->submit_hw_cnt = 0;
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
        hw->wait_hw_cnt = 0;
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    }
    break;

    default:
        break;
    }

    return status;
}
#endif
#endif

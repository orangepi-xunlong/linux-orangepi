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
#if vpmdTASK_PARALLEL_SYNC
#include <vip_drv_device.h>
#include <vip_drv_context.h>

#define LOGTAG  "tm-para-sync"

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_TASK

static vip_status_e query_hardware_dispatch(
    vipdrv_device_t* device,
    vip_uint16_t* flag
    )
{
    vip_uint16_t mask = 0;

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    if (vip_false_e == hw->dispatch) {
        mask |= (1 << hw->core_id);
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

    *flag = mask;
    PRINTK_D("%s query device %d dispatch 0x%x\n", LOGTAG, device->device_index, mask);

    return VIP_SUCCESS;
}

static vip_status_e set_hardware_dispatch(
    vipdrv_task_control_block_t *tskTCB,
    vip_bool_e value
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_task_t* task = &tskTCB->task;

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    hw->dispatch = value;
    VIPDRV_LOOP_HARDWARE_IN_TASK_END

    return status;
}

static vip_int32_t vipdrv_task_daemon_thread_handle(
    vip_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_queue_data_t* queue_data = VIP_NULL;
    vipdrv_task_control_block_t* tskTCB = VIP_NULL;
    vipdrv_device_t* device = (vipdrv_device_t*)param;
    vipdrv_queue_t* queue = &device->tskTCB_queue;
    vip_uint32_t device_index = device->device_index;
    vipdrv_hardware_t* hardware = VIP_NULL;
    vip_uint16_t flag = 0;
    vipdrv_hashmap_t* tskTCB_hashmap = &device->context->tskTCB_hashmap;

    PRINTK_I("%s dev%d daemon-thread process thread start\n", LOGTAG, device_index);
    vipdrv_os_set_signal(device->submit_thread_exit_signal, vip_false_e);

    while (device->tskTCB_thread_running) {
        vipdrv_os_wait_signal(device->tskTCB_submit_signal, vpmdINFINITE);
        vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_false_e);
        while (VIP_SUCCESS == query_hardware_dispatch(device, &flag) &&
            vip_true_e == vipdrv_queue_read(queue, &queue_data, (void*)&flag)) {
            TASK_COUNTER_HOOK(device, read_queue);
            if (VIP_NULL == queue_data) {
                PRINTK_E("%s dev%d read queue data is NULL\n", LOGTAG, device_index);
                continue;
            }
            vipdrv_hashmap_get_by_index(tskTCB_hashmap, queue_data->v1, (void**)&tskTCB);
            if (VIP_NULL == tskTCB) {
                PRINTK_E("%s dev%d get task%d by index fail\n", LOGTAG, device_index, queue_data->v1);
                continue;
            }
            vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
            if (VIPDRV_TASK_READY != tskTCB->status) {
                PRINTK_I("%s dev%d task status=0x%x not need submit\n", LOGTAG, device_index, tskTCB->status);
                vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tskTCB->queue_data->v1, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, tskTCB->queue_data->v1, vip_true_e);
                continue;
            }
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            PRINTK_I("%s submit command task_id=0x%x\n", LOGTAG, tskTCB->task.task_id);
            /* set hardwares are distribute state */
            set_hardware_dispatch(tskTCB, vip_true_e);

            /* wake up the thread of first core to submit and waiting for task inference done */
            hardware = &device->hardware[tskTCB->task.core_index];
            hardware->tcb_index = queue_data->v1;
            PRINTK_I("%s wake up core%d thread\n", LOGTAG, hardware->core_id);
            vipdrv_os_set_signal(hardware->core_thread_run_signal, vip_true_e);
            TASK_COUNTER_HOOK(device, send_notify);
        }
    }

    /* wake up wait the thread exit task */
    vipdrv_os_set_signal(device->submit_thread_exit_signal, vip_true_e);
    PRINTK_I("%s daemon thread exit\n", LOGTAG);

    return status;
}

static vip_int32_t vipdrv_task_core_thread_handle(
    vip_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_wait_param_t wait_param = { 0 };
    vipdrv_task_control_block_t *tskTCB = VIP_NULL;
    vipdrv_task_t *task = VIP_NULL;
    vipdrv_hardware_t* hardware = (vipdrv_hardware_t*)param;
    vipdrv_device_t* device = hardware->device;
    vipdrv_hashmap_t* tskTCB_hashmap = &device->context->tskTCB_hashmap;
    wait_param.mask = IRQ_VALUE_FLAG;
    vipdrv_os_set_signal(hardware->core_thread_exit_signal, vip_false_e);

    while (device->tskTCB_thread_running) {
        vipdrv_os_wait_signal(hardware->core_thread_run_signal, vpmdINFINITE);
        vipdrv_os_set_signal(hardware->core_thread_run_signal, vip_false_e);
        TASK_COUNTER_HOOK(hardware, get_notify);
        if (vip_false_e == device->tskTCB_thread_running) {
            continue;
        }
        vipdrv_hashmap_get_by_index(tskTCB_hashmap, hardware->tcb_index, (void**)&tskTCB);
        if (VIP_NULL == tskTCB) {
            vipdrv_hashmap_unuse(tskTCB_hashmap, hardware->tcb_index, vip_false_e);
            PRINTK_E("fail %s core=%d-thread submit is null\n", LOGTAG, hardware->core_id);
            continue;
        }
        /* unuse tskTCB that use in vipdrv_task_submit_thread_handle  */
        vipdrv_hashmap_unuse(tskTCB_hashmap, hardware->tcb_index, vip_false_e);
        task = &tskTCB->task;
        #if vpmdENABLE_POWER_MANAGEMENT
        /* make sure other thread can not change power state during submit */
        vipdrv_pm_lock_device_hw(device, task->core_index, task->core_cnt);
        #endif
        vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
        if (VIPDRV_TASK_READY == tskTCB->status) {
            PRINTK_I("%s core%d-thread start submit command, task_id=0x%x\n",
                     LOGTAG, hardware->core_id, task->task_id);
            status = vipdrv_task_submit(task);
            TASK_COUNTER_HOOK(hardware, submit_hw);
            tskTCB->status = VIPDRV_TASK_INFER_START;
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            #if vpmdENABLE_POWER_MANAGEMENT
            vipdrv_pm_unlock_device_hw(device, task->core_index, task->core_cnt);
            #endif
            if (VIP_SUCCESS == status) {
                wait_param.time_out = tskTCB->time_out;
                status = vipdrv_task_wait(task, &wait_param);
                TASK_COUNTER_HOOK(hardware, wait_hw);
                if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
                    PRINTK_E("fail %s to wait interrupt in core thread, status=%d, task_id=0x%x\n",
                            LOGTAG, status, task->task_id);
                }
                PRINTK_I("%s core%d-thread wait task done, task_id=0x%x\n",
                        LOGTAG, hardware->core_id, task->task_id);
            }
            else {
                PRINTK_E("fail %s to submit task_id=0x%x, status=%d\n",
                    LOGTAG, task->task_id, status);
            }
            hardware->tcb_index = 0;
        }
        else {
            vip_bool_e wake_up_user = (VIP_ERROR_CANCELED == tskTCB->queue_data->v2);
            hardware->tcb_index = 0;
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            #if vpmdENABLE_POWER_MANAGEMENT
            vipdrv_pm_unlock_device_hw(device, task->core_index, task->core_cnt);
            #endif
            PRINTK_I("%s core%d-thread task is canceled=%d or status=%d not READY, not need submit\n",
                     LOGTAG, hardware->core_id, tskTCB->queue_data->v2, tskTCB->status);
            vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tskTCB->queue_data->v1, vip_false_e);
            /* Setting hardwares are not in distribution state */
            set_hardware_dispatch(tskTCB, vip_false_e);
            vipdrv_hashmap_unuse(tskTCB_hashmap, tskTCB->queue_data->v1, wake_up_user);
            /* wake up mp daemon thread */
            vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_true_e);
            PRINTK_I("%s core%d-thread end submit command\n", LOGTAG, hardware->core_id);
            continue;
        }
        tskTCB->queue_data->v2 = status;
        vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
        /* wait do cancel end */
        tskTCB->status = VIPDRV_TASK_INFER_END;
        hardware->irq_local_value = 0;
        *hardware->irq_value &= ~IRQ_VALUE_FLAG;
        vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_true_e);
        vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
        set_hardware_dispatch(tskTCB, vip_false_e);
        vipdrv_hashmap_unuse(tskTCB_hashmap, tskTCB->queue_data->v1, vip_true_e);

        /* wake up mp daemon thread */
        vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_true_e);
        PRINTK_I("%s core%d-thread end submit command\n", LOGTAG, hardware->core_id);
    }

    /* core thread exit */
    vipdrv_os_set_signal(hardware->core_thread_exit_signal, vip_true_e);
    return status;
}

vip_status_e vipdrv_task_init(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_char_t name[256] = { 0 };

    VIPDRV_LOOP_DEVICE_START
    status = vipdrv_os_create_signal(&device->tskTCB_submit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create tskTCB submit signal\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_false_e);

    status = vipdrv_os_create_signal(&device->tskTCB_recover_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create tskTCB recover signal\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_true_e);

    status = vipdrv_os_create_signal(&device->submit_thread_exit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create submit thread exit signal\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }

    device->tskTCB_thread_running = vip_true_e;

    vipdrv_os_snprint(name, 255, "device%d_daemon_thrd", device->device_index);
    status = vipdrv_os_create_thread((vipdrv_thread_func)vipdrv_task_daemon_thread_handle,
        (vip_ptr)device, name, &device->submit_thread_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create daemon thread\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    status = vipdrv_os_create_signal(&hw->core_thread_exit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create core thread exit signal\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }

    status = vipdrv_os_create_signal(&hw->core_thread_run_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create core thread run signal\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_set_signal(hw->core_thread_run_signal, vip_false_e);

    vipdrv_os_snprint(name, 255, "core%d_wait_thrd", hw->core_id);
    status = vipdrv_os_create_thread((vipdrv_thread_func)vipdrv_task_core_thread_handle,
        (vip_ptr)hw, name,
        &hw->wait_thread_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create core%s thread\n", LOGTAG, hw->core_id);
        vipGoOnError(VIP_ERROR_IO);
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    VIPDRV_LOOP_DEVICE_END

onError:
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

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    vipdrv_os_set_signal(hw->core_thread_run_signal, vip_true_e);
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    vipdrv_os_wait_signal(hw->core_thread_exit_signal, WAIT_SIGNAL_TIMEOUT);
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

    vipdrv_os_wait_signal(device->submit_thread_exit_signal, WAIT_SIGNAL_TIMEOUT);
    VIPDRV_LOOP_DEVICE_END

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
    if (device->tskTCB_submit_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(device->tskTCB_submit_signal);
    }
    if (device->tskTCB_recover_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(device->tskTCB_recover_signal);
    }

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    if (hw->core_thread_exit_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(hw->core_thread_exit_signal);
    }

    if (hw->core_thread_run_signal != VIP_NULL) {
        vipdrv_os_destroy_signal(hw->core_thread_run_signal);
    }

    if (hw->wait_thread_handle != VIP_NULL) {
        vipdrv_os_destroy_thread(hw->wait_thread_handle);
    }
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
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
        PRINTK("device notify core thread   : %lld\n", device->send_notify_cnt);
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
        PRINTK("core %d thread get notice    : %lld\n", hw->core_index, hw->get_notify_cnt);
        PRINTK("core %d thread submit to HW  : %lld\n", hw->core_index, hw->submit_hw_cnt);
        PRINTK("core %d thread wait HW done  : %lld\n", hw->core_index, hw->wait_hw_cnt);
        PRINTK("core %d hardware dispatch    : %lld\n", hw->core_index, hw->dispatch);
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
        PRINTK("DEVICE %d TASK DISPATCH PROFILE END\n", device->device_index);
    }
    break;

    case VIPDRV_TASK_DIS_PROFILE_RESET:
    {
        device->read_queue_cnt = 0;
        device->send_notify_cnt = 0;
        VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
        hw->get_notify_cnt = 0;
        hw->submit_hw_cnt = 0;
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

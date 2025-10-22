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
#if vpmdTASK_SERIAL_SYNC
#include <vip_drv_context.h>

#define LOGTAG  "tm-serial-sync"
#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_TASK


static vip_status_e query_hardware_dispatch(
    vipdrv_device_t* device,
    vip_uint16_t* flag
    )
{
    vip_uint16_t mask = 0;

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    mask |= (1 << hw->core_id);
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

    *flag = mask;
    PRINTK_D("%s query device %d dispatch 0x%x\n", LOGTAG, device->device_index, mask);

    return VIP_SUCCESS;
}

static vip_int32_t vipdrv_task_thread_handle(
    vip_ptr param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_queue_data_t *queue_data = VIP_NULL;
    vipdrv_task_control_block_t *tskTCB = VIP_NULL;
    vipdrv_task_t *task = VIP_NULL;
    vipdrv_device_t* device = (vipdrv_device_t*)param;
    vipdrv_queue_t* queue = &device->tskTCB_queue;
    vip_uint32_t device_index = device->device_index;
    vipdrv_wait_param_t wait_param = { 0 };
    vip_uint16_t flag = 0;
    vipdrv_hardware_t* hardware = VIP_NULL;
    vipdrv_hashmap_t* tskTCB_hashmap = &device->context->tskTCB_hashmap;
    wait_param.mask = IRQ_VALUE_FLAG;

    PRINTK_I("%s dev%d process thread start\n", LOGTAG, device_index);
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
            task = &tskTCB->task;
            PRINTK_I("%s dev%d TCB index=%d, start submit task_id=0x%x\n",
                     LOGTAG, device_index, queue_data->v1, task->task_id);

            #if vpmdENABLE_POWER_MANAGEMENT
            /* make sure other thread can not change power state during submit */
            vipdrv_pm_lock_device_hw(device, task->core_index, task->core_cnt);
            #endif
            vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
            if (VIPDRV_TASK_READY != tskTCB->status) {
                PRINTK_I("%s dev%d task status=0x%x not need submit\n", LOGTAG, device_index, tskTCB->status);
                vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                #if vpmdENABLE_POWER_MANAGEMENT
                vipdrv_pm_unlock_device_hw(device, task->core_index, task->core_cnt);
                #endif
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, queue_data->v1, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, queue_data->v1, vip_true_e);
                continue;
            }

            /* submit command buffer */
            status = vipdrv_task_submit(task);
            TASK_COUNTER_HOOK(device, submit_hw);
            tskTCB->status = VIPDRV_TASK_INFER_START;
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            #if vpmdENABLE_POWER_MANAGEMENT
            vipdrv_pm_unlock_device_hw(device, task->core_index, task->core_cnt);
            #endif

            if (VIP_SUCCESS == status) {
                wait_param.time_out = tskTCB->time_out;
                status = vipdrv_task_wait(task, &wait_param);
                TASK_COUNTER_HOOK(device, wait_hw);
                if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
                    PRINTK_E("fail %s dev%d to wait interrupt, status=%d, task_id=0x%x\n",
                             LOGTAG, device_index, status, task->task_id);
                }
                PRINTK_I("%s dev%d wait task done task_id=0x%x\n",
                        LOGTAG, device_index, task->task_id);
            }
            else {
                PRINTK_E("fail %s to dev%d submit task_id=0x%x, status=%d\n",
                         LOGTAG, device_index, task->task_id, status);
            }
            tskTCB->queue_data->v2 = status;
            vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
            /* wait do cancel end */
            tskTCB->status = VIPDRV_TASK_INFER_END;
            hardware = &device->hardware[task->core_index];
            hardware->irq_local_value = 0;
            *hardware->irq_value &= ~IRQ_VALUE_FLAG;
            vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_true_e);
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            vipdrv_hashmap_unuse(tskTCB_hashmap, queue_data->v1, vip_true_e);
        }
    }

    /* wake up wait the thread exit task */
    vipdrv_os_set_signal(device->submit_thread_exit_signal, vip_true_e);
    PRINTK_I("%s thread exit\n", LOGTAG);

    return status;
}

vip_status_e vipdrv_task_init(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_char_t name[256] = "submit_thrd";

    VIPDRV_LOOP_DEVICE_START
    status = vipdrv_os_create_signal(&device->tskTCB_submit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create tskTCB submit signal\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }

    status = vipdrv_os_create_signal(&device->tskTCB_recover_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create tskTCB recover signal\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_true_e);

    vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_false_e);
    status = vipdrv_os_create_signal(&device->submit_thread_exit_signal);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create submit thread exit signal\n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }

    device->tskTCB_thread_running = vip_true_e;

    vipdrv_os_snprint(name, 255, "device%d_submit_thrd", device->device_index);
    status = vipdrv_os_create_thread((vipdrv_thread_func)vipdrv_task_thread_handle,
        (vip_ptr)device, name, &device->submit_thread_handle);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail %s to create submit thread \n", LOGTAG);
        vipGoOnError(VIP_ERROR_IO);
    }
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
        PRINTK("device wait HW done         : %lld\n", device->wait_hw_cnt);
        PRINTK("DEVICE %d TASK DISPATCH PROFILE END\n", device->device_index);
    }
    break;

    case VIPDRV_TASK_DIS_PROFILE_RESET:
    {
        device->read_queue_cnt = 0;
        device->submit_hw_cnt = 0;
        device->wait_hw_cnt = 0;
    }
    break;

    default:
        break;
    }

    return status;
}
#endif
#endif

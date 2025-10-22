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
#include <vip_drv_device_driver.h>
#include <vip_drv_debug.h>
#include <vip_drv_context.h>
#include <vip_drv_task_descriptor.h>
#include <vip_drv_mmu.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_TASK

/*
    INIT_TASK_QUEUE_DEPTH is the initial number of task supported in task queue.
    32 is default value when task queue is enabled.
*/
#define INIT_TASK_QUEUE_DEPTH          32

#if vpmdTASK_QUEUE_USED
static vip_status_e wake_up_user_wait(
    void* element
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_task_control_block_t* tskTCB = (vipdrv_task_control_block_t*)element;
    vipdrv_os_set_signal(tskTCB->wait_signal, vip_true_e);
    return status;
}

static vip_status_e free_TCB_resource(
    void* element
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_task_control_block_t* tskTCB = (vipdrv_task_control_block_t*)element;
    if (VIP_NULL != tskTCB->wait_signal) {
        vipdrv_os_destroy_signal(tskTCB->wait_signal);
        tskTCB->wait_signal = VIP_NULL;
    }
    if (VIP_NULL != tskTCB->queue_data) {
        vipdrv_os_free_memory((void*)tskTCB->queue_data);
        tskTCB->queue_data = VIP_NULL;
    }
    if (VIP_NULL != tskTCB->cancel_mutex) {
        vipdrv_os_destroy_mutex(tskTCB->cancel_mutex);
        tskTCB->cancel_mutex = VIP_NULL;
    }
    return status;
}

vip_status_e vipdrv_task_tcb_init(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_hashmap_t* tskTCB_hashmap = (vipdrv_hashmap_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TCB);

    VIPDRV_LOOP_DEVICE_START
    /* eg. 4cores need 10 queues -> 0, 1, 2, 3, 01, 12, 23, 012, 123, 0123 */
    status = vipdrv_queue_initialize(&device->tskTCB_queue, device->device_index,
        (device->hardware_count + 1) * device->hardware_count / 2);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create input queue for supporting multi-thread\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    VIPDRV_LOOP_DEVICE_END
    vipOnError(vipdrv_hashmap_init(tskTCB_hashmap, INIT_TASK_QUEUE_DEPTH,
        sizeof(vipdrv_task_control_block_t), "tskTCB", (void*)wake_up_user_wait, VIP_NULL, (void*)free_TCB_resource));
    PRINTK_D("tskTCB_hashmap=0x%"PRPx"\n", tskTCB_hashmap);
onError:
    return status;
}

vip_status_e vipdrv_task_tcb_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_hashmap_t* tskTCB_hashmap = (vipdrv_hashmap_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TCB);
    vipdrv_hashmap_destroy(tskTCB_hashmap);
    VIPDRV_LOOP_DEVICE_START
    vipdrv_queue_destroy(&device->tskTCB_queue);
    VIPDRV_LOOP_DEVICE_END

    return status;
}

vip_status_e vipdrv_task_tcb_submit(
    vipdrv_device_t *device,
    vipdrv_submit_task_param_t *sbmt_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t task_id = sbmt_para->task_id + sbmt_para->subtask_index;
    vipdrv_hashmap_t* tskTCB_hashmap = VIP_NULL;

    if (vip_true_e == device->tskTCB_thread_running) {
        vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
        vipdrv_task_control_block_t *tskTCB = VIP_NULL;
        vip_bool_e ret = vip_true_e;
        vip_uint32_t tcb_index = 0;
        vip_uint32_t tsk_desc_index = vipdrv_task_desc_id2index(task_id);
        vip_uint32_t queue_id = 0;
        tskTCB_hashmap = &device->context->tskTCB_hashmap;

        /* delivery the command buffer to multi thread processing */
        /* get a empty multi thread object */
Expand:
        if (vip_true_e == vipdrv_hashmap_full(tskTCB_hashmap)) {
            vip_bool_e expand = vip_false_e;
            status = vipdrv_hashmap_expand(tskTCB_hashmap, &expand);
            if (VIP_SUCCESS != status) {
                PRINTK_E("multi thread is full, not support so many threads\n");
                vipGoOnError(VIP_ERROR_FAILURE);
            }
        }

        /* get an index from task TCB hashmap and put the cmd_handle into the hashmap */
        status = vipdrv_hashmap_insert(tskTCB_hashmap, (vip_uint64_t)task_id, &tcb_index);
        if (VIP_SUCCESS != status) {
            if (VIP_ERROR_OUT_OF_RESOURCE == status) {
                /* multi-thread get the task control block at the same time; need expand */
                goto Expand;
            }
            else {
                PRINTK_E("The duplicate commands 0x%x can't be submitted\n", task_id);
                vipGoOnError(status);
            }
        }
        status = vipdrv_hashmap_get_by_index(tskTCB_hashmap, tcb_index, (void**)&tskTCB);
        if (VIP_NULL == tskTCB) {
            PRINTK_E("fail to get task %d status=%d\n", tcb_index);
            vipGoOnError(status);
        }

        if (VIP_NULL == tskTCB->wait_signal) {
            status = vipdrv_os_create_signal((vipdrv_signal*)&tskTCB->wait_signal);
            if (status != VIP_SUCCESS) {
                PRINTK_E("fail to create a signal wait signal\n");
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tcb_index, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
                vipGoOnError(VIP_ERROR_IO);
            }
            vipdrv_os_set_signal(tskTCB->wait_signal, vip_false_e);
        }

        if (VIP_NULL == tskTCB->cancel_mutex) {
            status = vipdrv_os_create_mutex((vipdrv_mutex*)&tskTCB->cancel_mutex);
            if (status != VIP_SUCCESS) {
                PRINTK_E("fail to create mutex for cancel task.\n");
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tcb_index, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
                vipGoOnError(VIP_ERROR_IO);
            }
        }

        if (VIP_NULL == tskTCB->queue_data) {
            status = vipdrv_os_allocate_memory(sizeof(vipdrv_queue_data_t), (void**)&tskTCB->queue_data);
            if (VIP_SUCCESS != status) {
                PRINTK_E("fail to submit, allocate queue_data fail\n");
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tcb_index, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
                vipGoOnError(VIP_ERROR_IO);
            }
            vipdrv_os_zero_memory((void*)tskTCB->queue_data, sizeof(vipdrv_queue_data_t));
        }

        vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
        if (vip_true_e == vipdrv_hashmap_check_remove(tskTCB_hashmap, tcb_index)) {
            PRINTK_E("task has been cancelled during submitting\n");
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            vipGoOnError(VIP_ERROR_CANCELED);
        }

        tskTCB->status = VIPDRV_TASK_READY;
        tskTCB->task.task_id = task_id;
        tskTCB->task.subtask_index = sbmt_para->subtask_index;
        tskTCB->task.subtask_count = sbmt_para->subtask_cnt;
        tskTCB->time_out = sbmt_para->time_out;
        tskTCB->task.device = device;
        tskTCB->task.core_index = sbmt_para->core_index;
        tskTCB->task.core_cnt = sbmt_para->core_cnt;
    #if vpmdENABLE_PREEMPTION
        tskTCB->queue_data->priority = sbmt_para->priority;
    #endif
        tskTCB->queue_data->v1 = tcb_index;
        tskTCB->queue_data->v2 = VIP_ERROR_FAILURE;
        tskTCB->queue_data->res_mask = 0;
        VIPDRV_LOOP_HARDWARE_IN_TASK_START(&tskTCB->task)
        tskTCB->queue_data->res_mask |= (1 << hw->core_id);
        VIPDRV_LOOP_HARDWARE_IN_TASK_END
        if (VIP_SUCCESS == vipdrv_database_use(&device->context->tskDesc_database,
            tsk_desc_index, (void**)&tsk_desc)) {
            tskTCB->queue_data->time = tsk_desc->subtasks[sbmt_para->subtask_index].avg_time;
            vipdrv_database_unuse(&device->context->tskDesc_database, tsk_desc_index);
        }
        PRINTK_D("user put task_id=0x%x into TCB index=%d, status=0x%x\n",
            task_id, tcb_index, tskTCB->status);

        vipdrv_os_set_signal(tskTCB->wait_signal, vip_false_e);
        VIPDRV_SHOW_DEV_CORE_INFO(device->device_index, sbmt_para->core_cnt,
                                  sbmt_para->core_index, "submit to queue");
        /* assume there are 4 cores, so there are 10 kinds of tasks.
           0    1    2    3
           01   12   23
           012  123
           0123
           if current task use core 1&2, the queue index is 5. */
        queue_id = (2 * device->hardware_count - sbmt_para->core_cnt + 2) *
                   (sbmt_para->core_cnt - 1) / 2 + sbmt_para->core_index;
        ret = vipdrv_queue_write(&device->tskTCB_queue, tskTCB->queue_data, &queue_id);
        vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
        if (!ret) {
            PRINTK_E("fail to write task into queue\n");
            tskTCB->status = VIPDRV_TASK_EMPTY;
            vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tcb_index, vip_false_e);
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            vipGoOnError(VIP_ERROR_FAILURE);
        }
        vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
        vipdrv_os_set_signal(device->tskTCB_submit_signal, vip_true_e);
    }
    else {
        PRINTK_E("multi task thread is not running\n");
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

onError:
    return status;
}

vip_status_e vipdrv_task_tcb_wait(
    vipdrv_wait_task_param_t* wait_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_task_control_block_t *tskTCB = VIP_NULL;
    vip_uint32_t tcb_index = 0;
    vip_uint32_t task_id = wait_para->task_id + wait_para->subtask_index;
    vipdrv_hashmap_t* tskTCB_hashmap = (vipdrv_hashmap_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TCB);

    vipdrv_hashmap_get_by_handle(tskTCB_hashmap, (vip_uint64_t)task_id, &tcb_index, (void**)&tskTCB);
    if (0 < tcb_index) {
        vipdrv_device_t* device = tskTCB->task.device;
        vip_uint32_t i = 0;

        if (vip_false_e == device->tskTCB_thread_running) {
            PRINTK_D("bypass wait irq, mt_thread=%d, device_idle=%s, task_id=0x%x\n",
                device->tskTCB_thread_running,
                (vipdrv_os_get_atomic(device->hardware[0].idle)) ? "true" : "false", task_id);
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            return VIP_ERROR_FAILURE;
        }

        vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
        if (VIP_ERROR_CANCELED == tskTCB->queue_data->v2) {
            /* the tskTCB has been canceled.
              1. don't need waiting for task inference done.
              2. don't need clean up mem_handle.
            */
            PRINTK_I("user end do wait device%d, task_id=0x%x, task_status=0x%x\n",
                      device->device_index, task_id, tskTCB->status);
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tcb_index, vip_false_e);
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            return VIP_ERROR_CANCELED;
        }
        vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);

        i = 0;
        while (1) {
            status = vipdrv_os_wait_signal(tskTCB->wait_signal, WAIT_SIGNAL_TIMEOUT);
            if (VIP_SUCCESS == status) {
                break;
            }
            else {
                PRINTK_D("wait task timeout loop index=%d\n", i);
            }
            i++;
        }
        vipdrv_os_set_signal(tskTCB->wait_signal, vip_false_e);
        if ((status != VIP_SUCCESS) && (VIPDRV_TASK_INFER_START == tskTCB->status)) {
            PRINTK_E("do wait signal task_id=0x%x, status=%d\n", task_id, status);
            tskTCB->task.subtask_count = 0;
            tskTCB->status = VIPDRV_TASK_EMPTY;
            vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tcb_index, vip_false_e);
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            vipGoOnError(status);
        }
        tskTCB->task.subtask_count = 0;
        tskTCB->status = VIPDRV_TASK_EMPTY;
        status = tskTCB->queue_data->v2;
        vipdrv_hashmap_remove_by_index(tskTCB_hashmap, tcb_index, vip_false_e);
        vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
        if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
            PRINTK_E("do wait mt thread fail, task_id=0x%x, status=%d\n", task_id, status);
            vipGoOnError(status);
        }
    }
    else {
        PRINTK_D("bypass wait irq, task_id=0x%x\n", task_id);
        status = VIP_ERROR_CANCELED;
    }

onError:
    return status;
}

#if vpmdENABLE_CANCELATION
vip_status_e vipdrv_task_tcb_cancel(
    vipdrv_cancel_task_param_t *cancel
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t task_id = cancel->task_id + cancel->subtask_index;
    vip_bool_e hw_cancel = vipdrv_context_support(VIP_HW_FEATURE_JOB_CANCEL);
    vipdrv_task_control_block_t* tskTCB = VIP_NULL;
    vip_uint32_t i = 0;
    vipdrv_hashmap_t* tskTCB_hashmap = (vipdrv_hashmap_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TCB);

    PRINTK_I("user start do cancel task_id=0x%x\n", task_id);
    if (VIP_SUCCESS == vipdrv_hashmap_get_by_handle(tskTCB_hashmap,
        (vip_uint64_t)task_id, &i, (void**)&tskTCB)) {
        vipdrv_device_t* device = tskTCB->task.device;
        #if vpmdTASK_PARALLEL_ASYNC
        vipdrv_os_wait_signal(device->tskTCB_recover_signal, vpmdINFINITE);
        vipdrv_os_inc_atomic(device->disable_recover);
        #endif

        vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
        if (tskTCB->status == VIPDRV_TASK_READY) {
            tskTCB->queue_data->v2 = VIP_ERROR_CANCELED;
            tskTCB->status = VIPDRV_TASK_CANCELED;
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            if (VIP_SUCCESS == vipdrv_queue_clean(&device->tskTCB_queue, VIPDRV_QUEUE_CLEAN_ONE,
                                                  (void*)&tskTCB->queue_data->v1)) {
                /* clean up mem_handle */
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, i, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, i, vip_true_e);
            }
            else {
                vipdrv_hashmap_unuse(tskTCB_hashmap, i, vip_false_e);
            }
            PRINTK_I("task_id=0x%x is marked cancel in queue, not be executed.\n", task_id);
        }
        else if (tskTCB->status == VIPDRV_TASK_INFER_START) {
            vipdrv_hardware_t *hardware = &device->hardware[tskTCB->task.core_index];
            VIPDRV_CHECK_NULL(hardware);

            if (vip_false_e == hw_cancel) {
                vip_uint32_t wait_time = 300; /* 300ms */
                #if vpmdFPGA_BUILD
                wait_time = 3000;
                #endif
                /* wait hw idle */
                vipdrv_device_wait_idle(device, wait_time, vip_true_e);
                PRINTK_I("user end do cancel on device%d, hw_cancel=%d.\n", device->device_index, hw_cancel);
                vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                vipdrv_hashmap_unuse(tskTCB_hashmap, i, vip_false_e);
            }
            else {
                vipdrv_os_set_signal(device->tskTCB_recover_signal, vip_false_e);
                hardware->irq_local_value = IRQ_VALUE_CANCEL;
                #if vpmdTASK_PARALLEL_ASYNC
                *device->irq_value |= IRQ_VALUE_FLAG;
                #else
                *hardware->irq_value |= IRQ_VALUE_FLAG;
                #endif
                #if !vpmdENABLE_POLLING
                vipdrv_os_inc_atomic(*hardware->irq_count);
                #if vpmdTASK_PARALLEL_ASYNC
                vipdrv_os_wakeup_interrupt(device->irq_queue);
                #else
                vipdrv_os_wakeup_interrupt(hardware->irq_queue);
                #endif
                #endif
                vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                /* wait recover done */

                #if vpmdTASK_PARALLEL_ASYNC
                vipdrv_os_dec_atomic(device->disable_recover);
                #endif
                vipdrv_os_wait_signal(device->tskTCB_recover_signal, vpmdINFINITE);
                if (VIP_ERROR_CANCELED != tskTCB->queue_data->v2) {
                    status = tskTCB->queue_data->v2;
                    PRINTK_E("fail to cancel task_id=0x%x, status=%d\n", task_id, status);
                }
                vipdrv_hashmap_remove_by_index(tskTCB_hashmap, i, vip_false_e);
                vipdrv_hashmap_unuse(tskTCB_hashmap, i, vip_false_e);
                PRINTK_I("task_id=0x%x is cancelled during executing.\n", task_id);
                vipGoOnError(status);
            }
        }
        else {
            vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
            vipdrv_hashmap_remove_by_index(tskTCB_hashmap, i, vip_false_e);
            vipdrv_hashmap_unuse(tskTCB_hashmap, i, vip_false_e);
            PRINTK_I("task_id=0x%x already forward done\n", task_id);
        }
        #if vpmdTASK_PARALLEL_ASYNC
        vipdrv_os_dec_atomic(device->disable_recover);
        #endif
    }
    else {
        PRINTK_E("bypass cancel, can not find cancel task_id=0x%x\n", task_id);
    }
onError:
    PRINTK_I("user end do cancel task_id=0x%x, status=%d\n", task_id, status);
    return status;
}
#endif
#endif

#if vpmdENABLE_RECOVERY || vpmdENABLE_CANCELATION
vip_status_e vipdrv_task_recover(
    vipdrv_device_t* device,
    vip_uint8_t* recover_mask
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;

    /* cancel hardware */
    if (vip_true_e == vipdrv_context_support(VIP_HW_FEATURE_JOB_CANCEL)) {
        status = vipdrv_hw_cancel(device, recover_mask);
        if (VIP_SUCCESS != status) {
            PRINTK_E("fail to cancel hardware %d, status=%d.\n", status);
            vipGoOnError(status);
        }

        for (i = 0; i < device->hardware_count; i++) {
            if (recover_mask[i]) {
                vipdrv_hardware_t* hardware = &device->hardware[i];
                VIPDRV_CHECK_NULL(hardware);
                #if vpmdENABLE_POWER_MANAGEMENT
                vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_END, VIP_NULL);
                vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
                #endif
                status = vipdrv_hw_submit_initcmd(hardware);
                #if vpmdENABLE_POWER_MANAGEMENT
                vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
                vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_RUN, VIP_NULL);
                #endif
                if (status != VIP_SUCCESS) {
                    PRINTK_E("fail to submit init cmd, status=%d.\n", status);
                    vipGoOnError(status);
                }
            }
        }
    }
    #if vpmdENABLE_POWER_MANAGEMENT
    else {
        /* no need to do reset after hw cancel */
        for (i = 0; i < device->hardware_count; i++) {
            if (recover_mask[i]) {
                vipdrv_hardware_t* hardware = &device->hardware[i];
                VIPDRV_CHECK_NULL(hardware);
                /* power off and then on, clock off, on, HW reset.. */
                vipdrv_drv_set_power_clk(hardware->core_id, VIPDRV_POWER_OFF);
                vipdrv_drv_set_power_clk(hardware->core_id, VIPDRV_POWER_ON);

                status = vipdrv_hw_reset(hardware);
                if (status != VIP_SUCCESS) {
                    PRINTK_E("fail to reset hardware, status=%d.\n", status);
                    vipGoOnError(status);
                }
                vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_END, VIP_NULL);
                vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
                status = vipdrv_hw_init(hardware);
                vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
                if (status != VIP_SUCCESS) {
                    PRINTK_E("fail to initialize hardware, status=%d.\n", status);
                    vipGoOnError(status);
                }
            }
        }
    }
    #endif

onError:
    return status;
}
#endif

/*
@ brief
    Insert flush mmu cache load states
*/
#if vpmdENABLE_MMU
static vip_status_e vipdrv_task_flush_mmu(
    vipdrv_task_t* task,
    vip_uint32_t* physical
    )
{
    vipdrv_hardware_t* hardware = &task->device->hardware[task->core_index];
    vip_uint8_t *flush_logical_end = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;
#if vpmdTASK_PARALLEL_ASYNC
    vipdrv_mmu_flush_t* MMU_flush = &hardware->MMU_flush_table[hardware->wl_end_index];
#else
    vipdrv_mmu_flush_t* MMU_flush = &hardware->MMU_flush;
#endif
    flush_logical_end = (vip_uint8_t*)MMU_flush->logical + FLUSH_MMU_STATES_SIZE;
    /* link flush mmu states and command buffer */
    cmd_link((vip_uint32_t*)flush_logical_end, *physical);
    *physical = MMU_flush->virtual;

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(task->device->context->mmu_flush_mem.mem_id, VIPDRV_CACHE_FLUSH);
#endif

    return status;
}
#endif

vip_status_e vipdrv_task_check_error(
    vipdrv_hardware_t* hardware
    )
{
    vip_status_e status = VIP_SUCCESS;

    /* check hw nan or inf error */
    if (0) {
        vip_uint32_t error_value = 0, data = 0;
        vipdrv_read_register(hardware, 0x00348, &error_value);
        if ((((((vip_uint32_t) (error_value)) >> (0 ? 1:1)) & ((vip_uint32_t) ((((1 ? 1:1) - (0 ? 1:1) + 1) == 32) ? ~0 : (~0 & ((1 << (((1 ? 1:1) - (0 ? 1:1) + 1))) - 1))))) )) {
            status = VIP_ERROR_NAN_INF;
        }
        data = ((((vip_uint32_t) (error_value)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 1:1) - (0 ?
 1:1) + 1))) - 1)))))) << (0 ?
 1:1))) | (((vip_uint32_t) ((vip_uint32_t) (1) & ((vip_uint32_t) ((((1 ?
 1:1) - (0 ?
 1:1) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 1:1) - (0 ? 1:1) + 1))) - 1)))))) << (0 ? 1:1)));
        vipdrv_write_register(hardware, 0x00348, data);
    }

    return status;
}

#if !vpmdENABLE_POLLING
vip_status_e vipdrv_task_check_irq_value(
    vipdrv_hardware_t* hardware
    )
{
    vip_uint32_t irq_value = *hardware->irq_value | hardware->irq_local_value;
    vip_status_e status = VIP_SUCCESS;
    VIPDRV_CHECK_NULL(hardware);

    PRINTK_D("dev%d hw%d, irq value=0x%x, irq_count=%d\n",
        hardware->device->device_index, hardware->core_index, irq_value,
        vipdrv_os_get_atomic(*hardware->irq_count));
    if (0 == irq_value) {
        PRINTK_E("dev%d hw%d, fail irq_value is 0\n", hardware->device->device_index, hardware->core_index);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (irq_value & IRQ_VALUE_CANCEL) {
        status = VIP_ERROR_CANCELED;
        PRINTK_I("dev%d hw%d, Execute canceled, irq_value=0x%x.\n",
            hardware->device->device_index, hardware->core_index, irq_value);
    }

    if (irq_value & IRQ_VALUE_AXI) {
        PRINTK_I("dev%d hw%d, AXI BUS ERROR...\n", hardware->device->device_index, hardware->core_index);
        status = VIP_ERROR_TIMEOUT; /* need recover */
    }

    if (irq_value & IRQ_VALUE_MMU) {
        vip_uint32_t mmu_status = 0;
        vipdrv_read_register(hardware, 0x00388, &mmu_status);
        if (mmu_status & 0x01) {
            PRINTK_I("MMU exception...\n");
        }
        else {
            PRINTK_I("MMU exception, then MMU is disabled\n");
        }
        status = VIP_ERROR_TIMEOUT; /* need recover */
    }

    /* error check */
    status = vipdrv_task_check_error(hardware);

#if vpmdENABLE_FUSA
    if (irq_value & IRQ_VALUE_ERROR_FUSA) {
        vipdrv_fusa_check_irq_value(hardware);
        status = VIP_ERROR_FUSA;
    }
#endif

    return status;
}
#endif

static vip_status_e vipdrv_task_get_cmd_info(
    vipdrv_task_t* task,
    vipdrv_task_cmd_type_e type,
    vip_uint32_t subtask_index,
    vip_uint32_t* mem_id,
    vip_uint8_t** logical,
    vip_uint32_t* physical,
    vip_uint32_t* used_size,
    vip_uint32_t* pid
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vipdrv_context_t* context = task->device->context;
    vip_uint32_t index = vipdrv_task_desc_id2index(task->task_id);
    vip_address_t phy_tmp = 0;

    if (VIP_SUCCESS != vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
        PRINTK_E("fail to get tsk desc task_id=0x%x, index=0x%x\n", task->task_id, index);
        return VIP_ERROR_FAILURE;
    }
    switch (type) {
    case VIPDRV_TASK_CMD_SUBTASK:
    {
        if (subtask_index >= tsk_desc->subtask_count) {
            status = VIP_ERROR_OUT_OF_MEMORY;
            PRINTK_E("subtask index=%d out of range\n", subtask_index);
        }
        else {
            vipOnError(vipdrv_mem_get_info(context, tsk_desc->subtasks[subtask_index].cmd.mem_id,
                logical, &phy_tmp, VIP_NULL, pid));
            if (VIP_NULL != physical) {
                *physical = (vip_uint32_t)phy_tmp;
            }
            if (VIP_NULL != mem_id) {
                *mem_id = tsk_desc->subtasks[subtask_index].cmd.mem_id;
            }
            if (VIP_NULL != used_size) {
                *used_size = tsk_desc->subtasks[subtask_index].cmd.size;
            }
        }
    }
    break;

    case VIPDRV_TASK_CMD_INIT:
    {
        if (0 != tsk_desc->init_cmd.mem_id) {
            vipOnError(vipdrv_mem_get_info(context, tsk_desc->init_cmd.mem_id, logical, &phy_tmp, VIP_NULL, pid));
            if (VIP_NULL != physical) {
                *physical = (vip_uint32_t)phy_tmp;
            }
            if (VIP_NULL != mem_id) {
                *mem_id = tsk_desc->init_cmd.mem_id;
            }
            if (VIP_NULL != used_size) {
                *used_size = tsk_desc->init_cmd.size;
            }
        }
        else {
            vipGoOnError(VIP_ERROR_FAILURE);
        }
    }
    break;

    case VIPDRV_TASK_CMD_PRELOAD:
    {
        if (0 != tsk_desc->preload_cmd.mem_id) {
            vipOnError(vipdrv_mem_get_info(context, tsk_desc->preload_cmd.mem_id, logical, &phy_tmp, VIP_NULL, pid));
            if (VIP_NULL != physical) {
                *physical = (vip_uint32_t)phy_tmp;
            }
            if (VIP_NULL != mem_id) {
                *mem_id = tsk_desc->preload_cmd.mem_id;
            }
            if (VIP_NULL != used_size) {
                *used_size = tsk_desc->preload_cmd.size;
            }
        }
        else {
            vipGoOnError(VIP_ERROR_FAILURE);
        }
    }
    break;

    default:
        PRINTK_E("not implement this type=%d\n", type);
        status = VIP_ERROR_NOT_SUPPORTED;
        break;
    }
onError:
    vipdrv_database_unuse(&context->tskDesc_database, index);
    return status;
}

/*
submit the task to hardware
 command order:
 flush cache -> task init cmd -> preload cmd -> subtask_0 cmd -> subtask_n cmd
*/
static vip_status_e vipdrv_task_submit_bottom_half(
    vipdrv_task_t* task
    )
{
    vip_uint32_t trigger_physical = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_bool_e init_done = vip_true_e;
    vip_uint32_t pid = 0;
#if vpmdPRELOAD_COEFF > 1
    vip_bool_e need_preload = vip_false_e;
#endif
#if vpmdTASK_PARALLEL_ASYNC
    vip_uint32_t mem_id = 0;
    vip_uint8_t* buffer = VIP_NULL;
    vip_uint32_t size = 0;
#endif

#if vpmdPRELOAD_COEFF > 1
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    vip_uint32_t last_task_id = hw->previous_task_id[(hw->pre_end_index +
        TASK_HISTORY_RECORD_COUNT - 1) % TASK_HISTORY_RECORD_COUNT];
    if (vipdrv_task_desc_id2index(last_task_id) != vipdrv_task_desc_id2index(task->task_id)) {
        need_preload = vip_true_e;
    }
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
#endif

    vipOnError(vipdrv_task_get_cmd_info(task, VIPDRV_TASK_CMD_SUBTASK, task->subtask_index,
        VIP_NULL, VIP_NULL, &trigger_physical, VIP_NULL, &pid));

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    vipOnError(vipdrv_mmu_page_switch(hw, gen_mmu_id(pid,
        vipdrv_task_desc_index2id(vipdrv_task_desc_id2index(task->task_id)))));
    VIPDRV_LOOP_HARDWARE_IN_TASK_END

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    if (vip_false_e == hw->init_done) {
        init_done = vip_false_e;
    }
#if vpmdENABLE_POWER_MANAGEMENT
    vipOnError(vipdrv_pm_hw_submit(hw));
#else
    vipdrv_os_set_atomic(hw->idle, vip_false_e);
#endif
    VIPDRV_LOOP_HARDWARE_IN_TASK_END

#if vpmdPRELOAD_COEFF > 1
    if (vip_true_e == need_preload) {
        vip_uint8_t* logical_tmp = VIP_NULL;
        vip_uint32_t used_size_tmp = 0;
        vip_uint32_t physical_tmp = 0;
        vip_uint32_t handle_tmp = 0;
        if (VIP_SUCCESS == vipdrv_task_get_cmd_info(task, VIPDRV_TASK_CMD_PRELOAD, 0,
            &handle_tmp, &logical_tmp, &physical_tmp, &used_size_tmp, VIP_NULL)) {
            cmd_link((vip_uint32_t*)(logical_tmp + used_size_tmp), trigger_physical);
            #if vpmdENABLE_VIDEO_MEMORY_CACHE
            vipdrv_mem_flush_cache(handle_tmp, VIPDRV_CACHE_FLUSH);
            #endif
            trigger_physical = physical_tmp;
            PRINTK_I("preload physical=0x%x, size=0x%x\n", physical_tmp, used_size_tmp);
        }
    }
#endif

    if (vip_false_e == init_done) {
        vip_uint8_t* logical_tmp = VIP_NULL;
        vip_uint32_t used_size_tmp = 0;
        vip_uint32_t physical_tmp = 0;
        vip_uint32_t handle_tmp = 0;
        if (VIP_SUCCESS == vipdrv_task_get_cmd_info(task, VIPDRV_TASK_CMD_INIT, 0,
            &handle_tmp, &logical_tmp, &physical_tmp, &used_size_tmp, VIP_NULL)) {
            cmd_link((vip_uint32_t*)(logical_tmp + used_size_tmp), trigger_physical);
            #if vpmdENABLE_VIDEO_MEMORY_CACHE
            vipdrv_mem_flush_cache(handle_tmp, VIPDRV_CACHE_FLUSH);
            #endif
            trigger_physical = physical_tmp;

            PRINTK_I("init cmd physical=0x%x, size=0x%x\n", physical_tmp, used_size_tmp);
            #if vpmdTASK_CMD_DUMP
            PRINTK("task-init-cmd-running\n");
            #endif
        }
        VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
        hw->init_done = vip_true_e;
        VIPDRV_LOOP_HARDWARE_IN_TASK_END
    }

#if vpmdTASK_PARALLEL_ASYNC
    vipOnError(vipdrv_task_get_cmd_info(task, VIPDRV_TASK_CMD_SUBTASK, task->subtask_index + task->subtask_count - 1,
        &mem_id, &buffer, VIP_NULL, &size, VIP_NULL));
    buffer += size + SEMAPHORE_STALL;
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    vip_uint32_t start_index = 0;
    vip_uint32_t end_index = 0;

    start_index = hw->wl_end_index;
    end_index = (hw->wl_end_index + 1) % WL_TABLE_COUNT;

    cmd_wait((vip_uint32_t*)(hw->wl_table[end_index].logical), WAIT_TIME_EXE);
    if (0 == hw_index) {
        /* enable irq */
        buffer += cmd_append_irq(buffer, hw->core_id, start_index);
    }
    buffer += cmd_chip_enable((vip_uint32_t*)buffer, hw->core_id, vip_false_e);
    buffer += cmd_link((vip_uint32_t*)buffer, hw->wl_table[end_index].physical);
#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(mem_id, VIPDRV_CACHE_FLUSH);
#endif
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
#endif

#if vpmdENABLE_MMU
    {
        vipdrv_hardware_t* hardware = &task->device->hardware[task->core_index];
        if (vip_true_e == hardware->flush_mmu) {
            vipdrv_task_flush_mmu(task, &trigger_physical);
            hardware->flush_mmu = vip_false_e;
        }
    }
#endif

    VIPDRV_SHOW_TASK_INFO(task, "submit to hw");
#if vpmdENABLE_TASK_PROFILE && (!vpmdTASK_PARALLEL_ASYNC)
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    vipdrv_hw_profile_reset(hw);
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
#endif
    vipdrv_task_profile_start(task);

    /* submit to hardware */
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
#if !(vpmdHARDWARE_BYPASS_MODE & 0x100)
#if vpmdTASK_PARALLEL_ASYNC
    vip_uint32_t start_index = hw->wl_end_index;
    cmd_link((vip_uint32_t*)(hw->wl_table[start_index].logical), trigger_physical);
    PRINTK_D("submit task_id=0x%x, link to core%d wl index=0x%x\n",
             task->task_id, hw->core_id, start_index);
#else
    vipOnError(vipdrv_hw_trigger(hw, trigger_physical));
#endif
#else
    if (0 == hw_index) {
        HARDWARE_BYPASS_SUBMIT()
    }
#endif
    VIPDRV_LOOP_HARDWARE_IN_TASK_END

    /* only for linux emulator signal sync, not used on FPGA and silicon */
#if defined (LINUXEMULATOR)
    vipdrv_os_delay(500);
#endif

    return status;
onError:
    PRINTK_E("fail task submit bottom half dev%d task_id=0x%x\n",
                task->device->device_index, task->task_id);
    return status;
}

vip_status_e vipdrv_task_submit(
    vipdrv_task_t* task
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t *device = VIP_NULL;
    if (0 == task->task_id) {
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    device = task->device;
#if vpmdENABLE_RECOVERY
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    if (0 > hw->recovery_times) {
        PRINTK_E("device %d recover fail last time\n", device->device_index);
        vipGoOnError(VIP_ERROR_FAILURE);
    }
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
#endif

#if vpmdENABLE_USER_CONTROL_POWER
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    VIPDRV_CHECK_USER_PM_STATUS();
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
#endif

#if vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    status = vipdrv_pm_check_power(hw);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to check power in submit status=%d\n", status);
        vipGoOnError(status);
    }
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
#endif

    PRINTK_D("dev%d submit task_id=0x%x, subtask_index[%d ~ %d]\n", task->device->device_index,
            task->task_id, task->subtask_index, task->subtask_index + task->subtask_count);

    vipOnError(vipdrv_task_submit_bottom_half(task));

    vipdrv_task_history_record(task);

    return status;
onError:
    PRINTK_E("fail to submit cmds to hardware status=%d\n", status);
    return status;
}

#if vpmdENABLE_POLLING
/*
@brief poll to waiting hardware going to idle.
@param timeout the number of milliseconds this waiting.
*/
vip_status_e vipdrv_task_wait_poll(
    vipdrv_task_t* task,
    vip_uint32_t timeout
    )
{
    #define POLL_SPACE   5  /* 5ms */
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t count = 0;
    vip_uint32_t idle_reg = 0;
    vip_int32_t try_wait_cnt = 3;

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    count = (vip_int32_t)timeout / POLL_SPACE;
    PRINTK_D("dev%d hw%d total polling count=%d...\n", dev->device_index, hw->core_index, count);
    if (vip_true_e == vipdrv_os_get_atomic(hw->idle)) {
        PRINTK_I("dev%d hw%d  wait poll is in idle status\n", dev->device_index, hw->core_index);
        continue;
    }

    while (try_wait_cnt > 0) {
        vipdrv_read_register(hw, 0x00004, &idle_reg);
        while (((idle_reg & EXPECT_IDLE_VALUE_ALL_MODULE) != EXPECT_IDLE_VALUE_ALL_MODULE) &&
               (0 == hw->irq_local_value) && (count > 0)) {
            vipdrv_os_delay(POLL_SPACE);
            vipdrv_read_register(hw, 0x00004, &idle_reg);
            count--;

            vipOnError(vipdrv_task_check_error(hw));

            #if vpmdENABLE_FUSA
            vipOnError(vipdrv_fusa_polling(hw));
            #endif
        }

        if (0 == count) {
            PRINTK_E("dev%d hw%d polling time=%d, idle status=0x%x\n",
                    dev->device_index, hw->core_index, timeout, idle_reg);
            break;
        }

        try_wait_cnt--;
    }

    if (hw->irq_local_value) {
        PRINTK_E("dev%d hw%d irq local value of hardware=0x%x\n",
                dev->device_index, hw->core_index, hw->irq_local_value);
        break;
    }

    if (try_wait_cnt > 0) {
        vipGoOnError(VIP_ERROR_TIMEOUT);
        break;
    }

    VIPDRV_LOOP_HARDWARE_IN_TASK_END

onError:
    return status;
}
#endif

#if vpmdTASK_SERIAL_SYNC || vpmdTASK_PARALLEL_SYNC
vip_status_e vipdrv_task_wait(
    vipdrv_task_t *task,
    vipdrv_wait_param_t *wait_param
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t timeout = wait_param->time_out;
    vipdrv_device_t* device = VIP_NULL;
    vipdrv_hardware_t* hardware = VIP_NULL;
    if (0 == task->task_id) {
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    device = task->device;
    hardware = &device->hardware[task->core_index];

    if (vip_false_e == vipdrv_os_get_atomic(hardware->idle)) {
        PRINTK_I("dev%d wait task_id=0x%x start\n", device->device_index, task->task_id);
        #if vpmdENABLE_POLLING
        status = vipdrv_task_wait_poll(task, timeout);
        if (status == VIP_ERROR_TIMEOUT) {
            VIPDRV_SHOW_TASK_INFO(task, "wait task irq");
            PRINTK_E("fail dev%d wait task_id=0x%x timeout\n", device->device_index, task->task_id);
            goto Exception;
        }
        else {
            vipOnError(status);
        }
        #else
        /* wait interrupt on first core */
        if ((VIP_NULL == hardware->irq_queue) || (VIP_NULL == hardware->irq_value)) {
            PRINTK_E("fail to wait interrupt. int queue or int falg is NULL\n");
            return VIP_ERROR_OUT_OF_MEMORY;
        }
        status = vipdrv_os_wait_interrupt(hardware->irq_queue, hardware->irq_value,
                                          timeout, wait_param->mask);
        *hardware->irq_value &= (~IRQ_VALUE_FLAG);
        vipdrv_os_dec_atomic(*hardware->irq_count);
        if (status == VIP_ERROR_TIMEOUT) {
            VIPDRV_SHOW_TASK_INFO(task, "wait task irq");
            PRINTK_E("fail dev%d wait task_id=0x%x timeout\n", device->device_index, task->task_id);
            goto Exception;
        }
        else {
            status = vipdrv_task_check_irq_value(hardware);
            if (status == VIP_ERROR_TIMEOUT) {
                PRINTK_E("fail check task irq value=0x%x\n", *hardware->irq_value);
                goto Exception;
            }
            else if (status == VIP_ERROR_CANCELED) {
                goto Exception;
            }
            else if ((status != VIP_SUCCESS) && (status != VIP_ERROR_RECOVERY)) {
                PRINTK_E("fail to dev%d check irq value status=%d\n", device->device_index, status);
                vipGoOnError(status);
            }
        }
        #endif

        vipdrv_task_profile_end(task);

        /* Then link back to the WL commands for letting vip going to idle. */
        VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
        if (VIP_SUCCESS != vipdrv_hw_idle(hw)) {
            PRINTK_E("fail hw%d idle.\n", hw->core_id);
            vipGoOnError(VIP_ERROR_FAILURE)
        }
        #if vpmdENABLE_RECOVERY
        hw->recovery_times = MAX_RECOVERY_TIMES;
        #endif
        VIPDRV_LOOP_HARDWARE_IN_TASK_END

        PRINTK_I("dev%d wait task_id=0x%x done\n", device->device_index, task->task_id);
    }
    return status;

Exception:
    VIPDRV_SHOW_TASK_INFO_IMPL(task, "wait task exception");
#if vpmdENABLE_HANG_DUMP
    if (VIP_ERROR_TIMEOUT == status) {
        vipdrv_hang_dump(task);
    }
#endif

#if (!vpmdENABLE_RECOVERY) && vpmdENABLE_CANCELATION
    if (VIP_ERROR_TIMEOUT == status) {
        PRINTK_E("dev%d, sys_time=0x%"PRIx64", error wait irq hardware hang.\n",
            device->device_index, vipdrv_os_get_time());
    }
    else
#endif
#if vpmdENABLE_RECOVERY || vpmdENABLE_CANCELATION
    {
        vip_uint8_t recover_flag[vipdMAX_CORE] = { 0 };
        vip_uint32_t i = 0;
        vip_status_e recover_status = VIP_SUCCESS;
        for (i = 0; i < task->core_cnt; i++) {
            recover_flag[task->core_index + i] = 1;
        }
        recover_status  = vipdrv_task_recover(device, recover_flag);
        if (VIP_SUCCESS == recover_status) {
            if (VIP_ERROR_TIMEOUT == status) {
                status = VIP_ERROR_RECOVERY;
            }
            else {
                status = VIP_ERROR_CANCELED;
            }
        }

        for (i = 0; i < task->core_cnt; i++) {
            hardware = &device->hardware[i];
            VIPDRV_CHECK_NULL(hardware);
            hardware->hw_submit_count = 0;
            hardware->user_submit_count = 0;
            #if vpmdENABLE_RECOVERY
            if (VIP_ERROR_TIMEOUT == status) {
                hardware->recovery_times--;
            }
            #endif
            PRINTK("dev%d hw%d recovery done.\n", hardware->device->device_index, hardware->core_index);
        }
    }
#endif
onError:
    PRINTK_E("fail to wait task_id=0x%x complete status=%d\n", task->task_id, status);
    return status;
}
#endif

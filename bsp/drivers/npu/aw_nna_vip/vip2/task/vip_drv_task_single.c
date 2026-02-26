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
#if vpmdTASK_SINGLE
#include <vip_drv_context.h>
#include <vip_drv_device_driver.h>
#include <vip_drv_debug.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_TASK


static vip_status_e vipdrv_cmd_timeout_handle(
    vipdrv_task_t *task
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t *device = task->device;
#if vpmdENABLE_RECOVERY
    vip_uint8_t recover_flag[vipdMAX_CORE] = { 0 };
    vip_uint32_t i = 0;
#endif
#if vpmdENABLE_HANG_DUMP
    vipdrv_hang_dump(task);
#endif

#if vpmdENABLE_RECOVERY
    for (i = 0; i < task->core_cnt; i++) {
        vipdrv_hardware_t* hardware = &device->hardware[i];
        recover_flag[task->core_index + i] = 1;
        hardware->recovery_times--;
    }
    status = vipdrv_task_recover(device, recover_flag);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to recover\n");
        vipGoOnError(status);
    }
    else {
        vipGoOnError(VIP_ERROR_RECOVERY);
    }
    PRINTK_I("dev%d hardware recovery done, return %d for app use.\n", device->device_index, status);
#else
    PRINTK_E("dev%d, sys_time=0x%"PRIx64", error hardware hang wait irq.\n",
              device->device_index, vipdrv_os_get_time());
    vipGoOnError(VIP_ERROR_TIMEOUT);
#endif

onError:
    return status;
}

/*
@brief wait hardware interrupt return.
*/
vip_status_e vipdrv_task_wait(
    vipdrv_task_t *task,
    vipdrv_wait_param_t *wait_parm
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t timeout = wait_parm->time_out;
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
            status = vipdrv_cmd_timeout_handle(task);
        }
        vipGoOnError(status);
        #else
        /* wait interrupt on first core */
        if ((VIP_NULL == hardware->irq_queue) || (VIP_NULL == hardware->irq_value)) {
            PRINTK_E("fail to wait interrupt. int queue or int falg is NULL\n");
            return VIP_ERROR_OUT_OF_MEMORY;
        }
        status = vipdrv_os_wait_interrupt(hardware->irq_queue, hardware->irq_value,
                                          timeout, wait_parm->mask);
        *hardware->irq_value &= (~IRQ_VALUE_FLAG);
        vipdrv_os_dec_atomic(*hardware->irq_count);

        if (status == VIP_ERROR_TIMEOUT) {
            status = vipdrv_cmd_timeout_handle(task);
            vipGoOnError(status);
        }
        else {
            status = vipdrv_task_check_irq_value(hardware);
            if (status == VIP_ERROR_TIMEOUT) {
                status = vipdrv_cmd_timeout_handle(task);
                vipGoOnError(status);
            }
            else if ((status != VIP_SUCCESS) && (status != VIP_ERROR_RECOVERY) && (status != VIP_ERROR_CANCELED)) {
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
onError:
    PRINTK_E("fail to wait command complete status=%d\n", status);
    return status;
}

static vip_status_e vipdrv_task_do_cancel(
    vipdrv_task_control_block_t *tskTCB,
    vipdrv_device_t *device
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint8_t recover_flag[vipdMAX_CORE] = { 0 };
    vipdrv_task_t* task = &tskTCB->task;
    vip_uint32_t i = 0;
    vipdrv_hardware_t* hardware = VIP_NULL;

    for (i = 0; i < task->core_cnt; i++) {
        recover_flag[task->core_index + i] = 1;
    }
    status = vipdrv_task_recover(device, recover_flag);
    tskTCB->status = VIPDRV_TASK_CANCELED;
    hardware = &device->hardware[task->core_index];
    hardware->irq_local_value = IRQ_VALUE_CANCEL;
    *hardware->irq_value |= IRQ_VALUE_FLAG;
    #if !vpmdENABLE_POLLING
    vipdrv_os_inc_atomic(*hardware->irq_count);
    vipdrv_os_wakeup_interrupt(hardware->irq_queue);
    #endif
    vipdrv_os_udelay(200);
    *hardware->irq_value &= ~IRQ_VALUE_FLAG;

    PRINTK_I("cancel wait irq done, task status=%d\n", tskTCB->status);
    return status;
}

vip_status_e vipdrv_task_init(void)
{
    vip_status_e status = VIP_SUCCESS;

    VIPDRV_LOOP_DEVICE_START
    status = vipdrv_os_create_mutex((vipdrv_mutex*)&device->tskTCB.cancel_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create mutex for cancel task.\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    VIPDRV_LOOP_DEVICE_END

onError:
    return status;
}

vip_status_e vipdrv_task_pre_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;

    return status;
}

vip_status_e vipdrv_task_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;
    VIPDRV_LOOP_DEVICE_START
    if (VIP_NULL != device->tskTCB.cancel_mutex) {
        vipdrv_os_destroy_mutex(device->tskTCB.cancel_mutex);
        device->tskTCB.cancel_mutex = VIP_NULL;
    }
    VIPDRV_LOOP_DEVICE_END

    return status;
}

vip_status_e vipdrv_task_tcb_init(void)
{
    return VIP_SUCCESS;
}

vip_status_e vipdrv_task_tcb_uninit(void)
{
    return VIP_SUCCESS;
}

vip_status_e vipdrv_task_tcb_submit(
    vipdrv_device_t *dev,
    vipdrv_submit_task_param_t *sbmt_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t task_id = sbmt_para->task_id + sbmt_para->subtask_index;
    vipdrv_task_control_block_t* tskTCB = &dev->tskTCB;

    VIPDRV_LOOP_DEVICE_START
    vipdrv_os_lock_mutex(device->tskTCB.cancel_mutex);
    if (task_id == device->tskTCB.task.task_id &&
        VIPDRV_TASK_INFER_START == tskTCB->status) {
        PRINTK("pls wait task=0x%x has done on device=%d\n", tskTCB->task.task_id, device->device_index);
        vipdrv_os_unlock_mutex(device->tskTCB.cancel_mutex);
        vipGoOnError(status);
    }
    vipdrv_os_unlock_mutex(device->tskTCB.cancel_mutex);
    VIPDRV_LOOP_DEVICE_END

    vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
    if (VIPDRV_TASK_INFER_START == tskTCB->status) {
        PRINTK("pls submit when last task=0x%x has done\n", tskTCB->task.task_id);
        vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
        vipGoOnError(status);
    }
    tskTCB->status = VIPDRV_TASK_READY;
    tskTCB->task.subtask_count = sbmt_para->subtask_cnt;
    tskTCB->task.subtask_index = sbmt_para->subtask_index;
    tskTCB->task.task_id = task_id;
    tskTCB->task.device = dev;
    tskTCB->task.core_index = sbmt_para->core_index;
    tskTCB->task.core_cnt = sbmt_para->core_cnt;
    tskTCB->time_out = sbmt_para->time_out;
    vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);

#if vpmdENABLE_POWER_MANAGEMENT
    /* make sure other thread can not change power state during submit */
    vipdrv_pm_lock_device_hw(dev, sbmt_para->core_index, sbmt_para->core_cnt);
#endif
    vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
    if (VIPDRV_TASK_CANCELED == tskTCB->status) {
        PRINTK_E("submit is canceled, not submit.\n");
        vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
        #if vpmdENABLE_POWER_MANAGEMENT
        vipdrv_pm_unlock_device_hw(dev, sbmt_para->core_index, sbmt_para->core_cnt);
        #endif
        vipGoOnError(VIP_ERROR_CANCELED);
    }
    status = vipdrv_task_submit(&tskTCB->task);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to submit cmd in single TM.\n");
        vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
        #if vpmdENABLE_POWER_MANAGEMENT
        vipdrv_pm_unlock_device_hw(dev, sbmt_para->core_index, sbmt_para->core_cnt);
        #endif
        vipGoOnError(status);
    }
    tskTCB->status = VIPDRV_TASK_INFER_START;
    vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
#if vpmdENABLE_POWER_MANAGEMENT
    vipdrv_pm_unlock_device_hw(dev, sbmt_para->core_index, sbmt_para->core_cnt);
#endif

onError:
    return status;
}

vip_status_e vipdrv_task_tcb_wait(
    vipdrv_wait_task_param_t* wait_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_wait_param_t wait_param = { 0 };
    vipdrv_hardware_t* hardware = VIP_NULL;
    vipdrv_task_control_block_t* tskTCB = VIP_NULL;

    VIPDRV_LOOP_DEVICE_START
    vipdrv_os_lock_mutex(device->tskTCB.cancel_mutex);
    if (wait_para->task_id + wait_para->subtask_index == device->tskTCB.task.task_id) {
        tskTCB = &device->tskTCB;
        break;
    }
    vipdrv_os_unlock_mutex(device->tskTCB.cancel_mutex);
    VIPDRV_LOOP_DEVICE_END
    if (VIP_NULL == tskTCB) {
        PRINTK_E("fail to wait TCB is null, wait task_id=0x%x, subtask_index=%d\n",
                  wait_para->task_id, wait_para->subtask_index);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    if (VIPDRV_TASK_CANCELED == tskTCB->status) {
        PRINTK_E("submit is canceled.\n");
        vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
        vipGoOnError(VIP_ERROR_CANCELED);
    }
    vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);

#if vpmdENABLE_USER_CONTROL_POWER
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(&tskTCB->task)
    VIPDRV_CHECK_USER_PM_STATUS();
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
#endif

    wait_param.mask = IRQ_VALUE_FLAG;
    wait_param.time_out = tskTCB->time_out;
    status = vipdrv_task_wait(&tskTCB->task, &wait_param);
    vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
    if ((status != VIP_SUCCESS) && (status != VIP_ERROR_CANCELED)) {
        PRINTK_E("fail to wait task done, status=%d, task_id=0x%x\n",
            status, tskTCB->task.task_id);
    }
    tskTCB->status = VIPDRV_TASK_INFER_END;
    hardware = &tskTCB->task.device->hardware[tskTCB->task.core_index];
    hardware->irq_local_value = 0;
    *hardware->irq_value &= ~IRQ_VALUE_FLAG;
    vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);

onError:
    return status;
}

#if vpmdENABLE_CANCELATION
vip_status_e vipdrv_task_tcb_cancel(
    vipdrv_cancel_task_param_t *cancel
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t *context = VIP_NULL;
    vipdrv_device_t *device = VIP_NULL;
    vip_uint32_t task_id = cancel->task_id + cancel->subtask_index;
    vip_bool_e hw_cancel = vipdrv_context_support(VIP_HW_FEATURE_JOB_CANCEL);
    vipdrv_task_control_block_t* tskTCB = VIP_NULL;

    VIPDRV_LOOP_DEVICE_START
    vipdrv_os_lock_mutex(device->tskTCB.cancel_mutex);
    if (task_id == device->tskTCB.task.task_id) {
        tskTCB = &device->tskTCB;
        vipdrv_os_unlock_mutex(device->tskTCB.cancel_mutex);
        break;
    }
    vipdrv_os_unlock_mutex(device->tskTCB.cancel_mutex);
    VIPDRV_LOOP_DEVICE_END
    if (VIP_NULL == tskTCB) {
        PRINTK_E("fail to cancel TCB is null, wait task_id=0x%x, subtask_index=%d\n",
                  cancel->task_id, cancel->subtask_index);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    device = tskTCB->task.device;
    vipIsNULL(device);
    context = device->context;

    PRINTK_I("user start do cancel on device%d, hw_cancel=%d.\n", device->device_index, hw_cancel);
    if (vip_false_e == hw_cancel) {
        vip_uint32_t wait_time = 300; /* 300ms */
        #if vpmdFPGA_BUILD
        wait_time = 3000;
        #endif
        /* wait hw idle */
        vipdrv_device_wait_idle(device, wait_time, vip_true_e);
        PRINTK_I("user end do cancel on device%d, hw_cancel=%d.\n", device->device_index, hw_cancel);
        return status;
    }

    vipdrv_os_lock_mutex(tskTCB->cancel_mutex);
    if (task_id == tskTCB->task.task_id) {
        if (tskTCB->status < VIPDRV_TASK_INFER_START) {
            tskTCB->status = VIPDRV_TASK_CANCELED;
            PRINTK_I("task_id=0x%x is marked cancel in queue, not be executed.\n", task_id);
        }
        else if (tskTCB->status == VIPDRV_TASK_INFER_START) {
            if (VIP_SUCCESS != vipdrv_task_do_cancel(tskTCB, device)) {
                vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
                PRINTK_E("hardware cancel job failed.\n");
                vipGoOnError(VIP_ERROR_FAILURE);
            }
        }
        else if (tskTCB->status == VIPDRV_TASK_INFER_END) {
            PRINTK_I("task_id=0x%x already forward done\n", task_id);
        }
    }
    else {
        PRINTK_D("bypass cancel, task_id=0x%x\n", task_id);
    }
    vipdrv_os_unlock_mutex(tskTCB->cancel_mutex);
    PRINTK_I("user end do cancel device%d, task_id=0x%x\n", device->device_index, task_id);
onError:
    return status;
}
#endif

#endif

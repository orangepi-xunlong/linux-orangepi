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
#ifndef _VIP_DRV_TASK_COMMON_H_
#define _VIP_DRV_TASK_COMMON_H_
#include <vip_drv_type.h>
#include <vip_drv_util.h>
#include <vip_drv_share.h>

#define VIPDRV_SHOW_TASK_INFO_IMPL(tsk, string)                        \
{                                                                      \
vip_char_t info_t[256];                                                \
vip_char_t *tmp = info_t;                                              \
vip_uint32_t itr = 0;                                                  \
vip_uint32_t dev_idx = tsk->device->device_index;                      \
vip_uint32_t core_idx = tsk->core_index;                               \
vip_uint32_t core_cnt = tsk->core_cnt;                                 \
vipdrv_os_snprint(tmp, 255, "%s, dev_idx=%d, core_count=%d, core_id[", \
                   string, dev_idx, core_cnt);                         \
tmp += 34 + sizeof(string);                                            \
for (itr = 0; itr < core_cnt; itr++) {                                 \
    vipdrv_os_snprint(tmp, 255, "%02d ", (core_idx + itr));            \
    tmp += 3;                                                          \
}                                                                      \
vipdrv_os_snprint(tmp, 255, "], ");                                    \
tmp += 3;                                                              \
vipdrv_os_snprint(tmp, 255, "tsk_id=0x%x sbtsk_idx=%d, sbtsk_cnt=%d\n",\
        tsk->task_id, tsk->subtask_index, tsk->subtask_count);         \
PRINTK("%s", info_t);                                                  \
}

#if (vpmdENABLE_DEBUG_LOG >= 4)
#define VIPDRV_SHOW_TASK_INFO(tsk, string)   VIPDRV_SHOW_TASK_INFO_IMPL(tsk, string)
#else
#define VIPDRV_SHOW_TASK_INFO(tsk, string)
#endif

#define DEFAULT_INFER_TIME 100000 /* us */

#if vpmdTASK_DISPATCH_DEBUG
#define TASK_COUNTER_HOOK(object, counter) object->counter##_cnt++;
#else
#define TASK_COUNTER_HOOK(object, counter)
#endif

typedef enum _vipdrv_task_status_e
{
    VIPDRV_TASK_NONE           = 0,
    VIPDRV_TASK_EMPTY          = 1,
    /* task has been submit to queue */
    VIPDRV_TASK_READY          = 2,
    /* task has been submit to HW */
    VIPDRV_TASK_INFER_START    = 3,
    /* wait irq return, inference done */
    VIPDRV_TASK_INFER_END      = 4,
    /* the task has been canceled */
    VIPDRV_TASK_CANCELED       = 5,
    VIPDRV_TASK_MAX,
} vipdrv_task_status_e;

struct _vipdrv_task
{
    vip_uint32_t      task_id;
    vip_uint32_t      subtask_index;
    vip_uint32_t      subtask_count;
    /* this task runs on which device */
    vipdrv_device_t*  device;
    /* the start core index of this device used to run task */
    vip_uint32_t      core_index;
    /* the number of cores used by this task */
    vip_uint32_t      core_cnt;
};

struct _vipdrv_task_control_block
{
    vipdrv_task_t                 task;
    volatile vipdrv_task_status_e status;
    vipdrv_mutex                  cancel_mutex;

    /* for multi-task enabled */
#if vpmdTASK_QUEUE_USED
#if vpmdTASK_PARALLEL_ASYNC
    volatile vip_uint8_t hw_ready_mask;
#endif
    vipdrv_queue_data_t  *queue_data;
    vipdrv_signal        wait_signal;
#endif
    vip_uint32_t         time_out;
};

typedef struct _vipdrv_wait_param
{
    vip_uint32_t    mask;
    vip_uint32_t    time_out;
} vipdrv_wait_param_t;

vip_status_e vipdrv_task_init(void);

vip_status_e vipdrv_task_pre_uninit(void);

vip_status_e vipdrv_task_uninit(void);

vip_status_e vipdrv_task_tcb_init(void);

vip_status_e vipdrv_task_tcb_uninit(void);

vip_status_e vipdrv_task_tcb_submit(
    vipdrv_device_t *device,
    vipdrv_submit_task_param_t *sbmt_para
    );

vip_status_e vipdrv_task_tcb_wait(
    vipdrv_wait_task_param_t* wait_para
    );

#if vpmdENABLE_CANCELATION
vip_status_e vipdrv_task_tcb_cancel(
    vipdrv_cancel_task_param_t *cancel
    );
#endif

#if !vpmdENABLE_POLLING
vip_status_e vipdrv_task_check_irq_value(
    vipdrv_hardware_t* hardware
    );
#endif

#if vpmdENABLE_RECOVERY || vpmdENABLE_CANCELATION
vip_status_e vipdrv_task_recover(
    vipdrv_device_t* device,
    vip_uint8_t* recover_mask
    );
#endif

vip_status_e vipdrv_task_submit(
    vipdrv_task_t *task
    );

#if !vpmdTASK_PARALLEL_ASYNC
vip_status_e vipdrv_task_wait(
    vipdrv_task_t *task,
    vipdrv_wait_param_t *wait_param
    );
#endif

#if vpmdENABLE_POLLING
vip_status_e vipdrv_task_wait_poll(
    vipdrv_task_t* task,
    vip_uint32_t timeout
    );
#endif

#if vpmdTASK_DISPATCH_DEBUG
typedef enum _vipdrv_task_dispatch_profile_e
{
    VIPDRV_TASK_DIS_PROFILE_NONE  = 0,
    VIPDRV_TASK_DIS_PROFILE_SHOW  = 1,
    VIPDRV_TASK_DIS_PROFILE_RESET = 2,
    VIPDRV_TASK_DIS_PROFILE_MAX,
} vipdrv_task_dispatch_profile_e;

vip_status_e vipdrv_task_dispatch_profile(
    vipdrv_device_t* device,
    vipdrv_task_dispatch_profile_e type
    );
#endif
#endif

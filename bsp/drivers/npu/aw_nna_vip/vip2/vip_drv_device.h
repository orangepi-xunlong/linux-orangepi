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

#ifndef __VIP_DRV_DEVICE_H__
#define __VIP_DRV_DEVICE_H__

#include <vip_drv_type.h>
#include <vip_drv_hardware.h>
#include <vip_drv_task_common.h>

/*
@brief define loop fun for getting each vip core in device
*/
#define VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)                      \
{  vip_uint32_t hw_index = 0;                                             \
   for (hw_index = 0 ; hw_index < (device)->hardware_count; hw_index++) { \
       vipdrv_hardware_t *hw = &(device)->hardware[hw_index];             \
       if (hw != VIP_NULL) {                                              \

#define VIPDRV_LOOP_HARDWARE_IN_DEVICE_END }}}

/*
@brief define loop fun for getting each vip core in device from M_index to N_index
*/
#define VIPDRV_LOOP_HARDWARE_IN_DEVICE_MtoN_START(device, start, cnt)   \
{  vip_uint32_t hw_index = 0;                                           \
   for (hw_index = 0 ; hw_index < (cnt); hw_index++) {                  \
       vipdrv_hardware_t *hw = &(device)->hardware[(start) + hw_index]; \
       if (hw != VIP_NULL) {                                            \

#define VIPDRV_LOOP_HARDWARE_IN_DEVICE_MtoN_END }}}

/*
@brief define loop fun for getting each vip hardware in task
*/
#define VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)                                \
{  vip_uint32_t hw_index = 0;                                                   \
   vipdrv_device_t *dev = (task)->device;                                       \
   for (hw_index = 0 ; hw_index < (task)->core_cnt; hw_index++) {               \
       vipdrv_hardware_t *hw = &(dev->hardware[(task)->core_index + hw_index]); \
       if (hw != VIP_NULL) {                                                    \

#define VIPDRV_LOOP_HARDWARE_IN_TASK_END }}}

struct _vipdrv_device
{
    vip_uint32_t              device_index;
    /* the number of VIP cored in this device */
    vip_uint32_t              hardware_count;
    vipdrv_hardware_t        *hardware;
    /* belong to which context */
    vipdrv_context_t         *context;

    /* init command in vip-drv */
    vipdrv_video_memory_t     init_command;
    vip_uint32_t              init_command_size;
#if vpmdTASK_QUEUE_USED
    vipdrv_queue_t            tskTCB_queue;
    /* atomic object for reading TCB from tskTCB_queue thread is running */
    volatile vip_bool_e       tskTCB_thread_running;
    /* a signal for sending to read TCB from tskTCB_queue thread */
    vipdrv_signal             tskTCB_submit_signal;
    /* a signal for job cancellation the taskTCB */
    vipdrv_signal             tskTCB_recover_signal;

    /* thread handle object for reading tskTCB from tskTCB_queue and send task to next stage */
    void                     *submit_thread_handle;
    /* signal for destroy task submitting thread */
    vipdrv_signal             submit_thread_exit_signal;
#if vpmdTASK_PARALLEL_ASYNC
    void                     *wait_thread_handle; /* thread object. task waiting thread process handle */
    vipdrv_signal             wait_thread_exit_signal; /* signal for destroy task waiting thread */
    vipdrv_atomic             disable_recover; /* indicate how many thread not allow recover yet */
    void                     *irq_queue;
    volatile vip_uint32_t    *irq_value;
#endif
#else
    vipdrv_task_control_block_t tskTCB;
#endif
#if vpmdTASK_DISPATCH_DEBUG
#if vpmdTASK_PARALLEL_SYNC
    volatile vip_uint32_t     read_queue_cnt;
    volatile vip_uint32_t     send_notify_cnt;
#elif vpmdTASK_SERIAL_SYNC
    volatile vip_uint32_t     read_queue_cnt;
    volatile vip_uint32_t     submit_hw_cnt;
    volatile vip_uint32_t     wait_hw_cnt;
#elif vpmdTASK_PARALLEL_ASYNC
    volatile vip_uint32_t     read_queue_cnt;
    volatile vip_uint32_t     recover_done_cnt;
    volatile vip_uint32_t     submit_hw_cnt;
#endif
#endif
};

/*
@brief Get hardware object from device.
*/
vipdrv_hardware_t* vipdrv_get_hardware(
    vipdrv_device_t* device,
    vip_uint32_t core_index
    );

vipdrv_device_t* vipdrv_get_device(
    vip_uint32_t device_index
    );

vip_status_e vipdrv_device_hw_reset(
    vipdrv_device_t *device
    );

vip_status_e vipdrv_device_hw_init(
    vipdrv_device_t *device
    );

/*
@brief To run any initial commands once in the beginning.
*/
vip_status_e vipdrv_device_prepare_initcmd(
    vipdrv_device_t *device
    );

vip_status_e vipdrv_device_wait_idle(
    vipdrv_device_t *device,
    vip_uint32_t timeout,
    vip_uint8_t is_fast
    );
#endif

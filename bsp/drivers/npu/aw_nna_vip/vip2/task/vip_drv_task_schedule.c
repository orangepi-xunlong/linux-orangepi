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

#include <vip_drv_task_schedule.h>
#if vpmdTASK_SCHEDULE
#include <vip_drv_context.h>
#include <vip_drv_task_descriptor.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_TASK


#if (4 <= vpmdENABLE_DEBUG_LOG)
static vip_status_e task_schedule_dump(
    vipdrv_device_t *device,
    vipdrv_submit_task_param_t *sbmt_para,
    vipdrv_core_load_t* loads,
    vip_uint32_t schedule_core
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t core_cnt = sbmt_para->core_cnt;
    vip_uint32_t core_index = sbmt_para->core_index;
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vipdrv_subtask_info_t* sub_task = VIP_NULL;
    vipdrv_context_t* context = device->context;
    vip_uint32_t task_index = vipdrv_task_desc_id2index(sbmt_para->task_id);
    vip_uint32_t i = 0;

    PRINTK("device_%d task_id=0x%x schedule info:\n", device->device_index, sbmt_para->task_id);
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    PRINTK("core_%d, task count %d, estimated time %lluus\n",
           hw_index, loads[hw_index].task_count, loads[hw_index].estimated_time);
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

    PRINTK("task schedule from sbmt core[%d~%d] to core[%d~%d]\n",
        core_index, core_index + core_cnt - 1, schedule_core, schedule_core + core_cnt - 1);

    status = vipdrv_database_use(&context->tskDesc_database, task_index, (void**)&tsk_desc);
    if (VIP_SUCCESS == status) {
        for (i = 0; i < sbmt_para->subtask_cnt; i++) {
            sub_task = &tsk_desc->subtasks[i + sbmt_para->subtask_index];
            PRINTK("task schedule subtsk%d last core from %d to %d\n", i + sbmt_para->subtask_index,
                    sub_task->core_index, schedule_core);
        }
        vipdrv_database_unuse(&context->tskDesc_database, task_index);
    }

    return status;
}
#endif

static vip_status_e hardware_multivip_sync(
    vip_uint32_t *buffer,
    vip_uint32_t *size,
    vip_uint32_t core_index,
    vip_uint32_t core_count
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t *memory = buffer;
    vip_int32_t i = 0;
    vip_uint32_t num = 0;

    if (VIP_NULL == buffer) {
        PRINTK_E("fail to get multivip sync cmds, buffer is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (1 >= core_count) {
        if (size != VIP_NULL) {
            *size = 0;
        }
        return VIP_SUCCESS;
    }

    *memory = 0x08010e02; memory++;
    *memory = 0x30000701; memory++;
    *memory = 0x48000000; memory++;
    *memory = 0x30000701; memory++;

    for (i = 0; i < (vip_int32_t)(core_count - 1); i++) {
        do {    do {    *memory++        = ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 31:27) - (0 ?
 31:27) + 1))) - 1)))))) << (0 ?
 31:27))) | (((vip_uint32_t) (0x01 & ((vip_uint32_t) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 31:27) - (0 ?
 31:27) + 1))) - 1)))))) << (0 ?
 31:27)))        | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 26:26) - (0 ?
 26:26) + 1))) - 1)))))) << (0 ?
 26:26))) | (((vip_uint32_t) ((vip_uint32_t) (vip_false_e) & ((vip_uint32_t) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 26:26) - (0 ?
 26:26) + 1))) - 1)))))) << (0 ?
 26:26)))        | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 25:16) - (0 ?
 25:16) + 1))) - 1)))))) << (0 ?
 25:16))) | (((vip_uint32_t) ((vip_uint32_t) (1) & ((vip_uint32_t) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 25:16) - (0 ?
 25:16) + 1))) - 1)))))) << (0 ?
 25:16)))        | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 15:0) - (0 ?
 15:0) + 1))) - 1)))))) << (0 ?
 15:0))) | (((vip_uint32_t) ((vip_uint32_t) (0x0E02) & ((vip_uint32_t) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 15:0) - (0 ? 15:0) + 1))) - 1)))))) << (0 ? 15:0)));}while(0);
    do {vip_uint32_t __temp_data32__;
__temp_data32__ = ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 4:0) - (0 ?
 4:0) + 1))) - 1)))))) << (0 ?
 4:0))) | (((vip_uint32_t) (0x01 & ((vip_uint32_t) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 4:0) - (0 ?
 4:0) + 1))) - 1)))))) << (0 ?
 4:0))) | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 23:20) - (0 ?
 23:20) + 1))) - 1)))))) << (0 ?
 23:20))) | (((vip_uint32_t) ((vip_uint32_t) (core_index + i) & ((vip_uint32_t) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 23:20) - (0 ?
 23:20) + 1))) - 1)))))) << (0 ?
 23:20))) | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 19:16) - (0 ?
 19:16) + 1))) - 1)))))) << (0 ?
 19:16))) | (((vip_uint32_t) ((vip_uint32_t) (0) & ((vip_uint32_t) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 19:16) - (0 ?
 19:16) + 1))) - 1)))))) << (0 ?
 19:16))) | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 12:8) - (0 ?
 12:8) + 1))) - 1)))))) << (0 ?
 12:8))) | (((vip_uint32_t) (0x0F & ((vip_uint32_t) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 12:8) - (0 ?
 12:8) + 1))) - 1)))))) << (0 ?
 12:8))) | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 27:24) - (0 ?
 27:24) + 1))) - 1)))))) << (0 ?
 27:24))) | (((vip_uint32_t) ((vip_uint32_t) (core_index + i + 1) & ((vip_uint32_t) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 27:24) - (0 ? 27:24) + 1))) - 1)))))) << (0 ? 27:24))) ;*memory++ = __temp_data32__;
} while(0);
}while(0);


        *memory++ = ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 31:27) - (0 ?
 31:27) + 1))) - 1)))))) << (0 ?
 31:27))) | (((vip_uint32_t) (0x09 & ((vip_uint32_t) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 31:27) - (0 ? 31:27) + 1))) - 1)))))) << (0 ? 31:27)));

        *memory++ = ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 4:0) - (0 ?
 4:0) + 1))) - 1)))))) << (0 ?
 4:0))) | (((vip_uint32_t) (0x01 & ((vip_uint32_t) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 4:0) - (0 ? 4:0) + 1))) - 1)))))) << (0 ? 4:0)))
            | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 23:20) - (0 ?
 23:20) + 1))) - 1)))))) << (0 ?
 23:20))) | (((vip_uint32_t) ((vip_uint32_t) (core_index + i) & ((vip_uint32_t) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 23:20) - (0 ? 23:20) + 1))) - 1)))))) << (0 ? 23:20)))
            | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 19:16) - (0 ?
 19:16) + 1))) - 1)))))) << (0 ?
 19:16))) | (((vip_uint32_t) ((vip_uint32_t) (0) & ((vip_uint32_t) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 19:16) - (0 ? 19:16) + 1))) - 1)))))) << (0 ? 19:16)))
            | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 12:8) - (0 ?
 12:8) + 1))) - 1)))))) << (0 ?
 12:8))) | (((vip_uint32_t) (0x0F & ((vip_uint32_t) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 12:8) - (0 ? 12:8) + 1))) - 1)))))) << (0 ? 12:8)))
            | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 27:24) - (0 ?
 27:24) + 1))) - 1)))))) << (0 ?
 27:24))) | (((vip_uint32_t) ((vip_uint32_t) (core_index + i + 1) & ((vip_uint32_t) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 27:24) - (0 ? 27:24) + 1))) - 1)))))) << (0 ? 27:24)));
    }

    for (i = (vip_int32_t)(core_count - 1); i > 0; i--) {
        do {    do {    *memory++        = ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 31:27) - (0 ?
 31:27) + 1))) - 1)))))) << (0 ?
 31:27))) | (((vip_uint32_t) (0x01 & ((vip_uint32_t) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 31:27) - (0 ?
 31:27) + 1))) - 1)))))) << (0 ?
 31:27)))        | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 26:26) - (0 ?
 26:26) + 1))) - 1)))))) << (0 ?
 26:26))) | (((vip_uint32_t) ((vip_uint32_t) (vip_false_e) & ((vip_uint32_t) ((((1 ?
 26:26) - (0 ?
 26:26) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 26:26) - (0 ?
 26:26) + 1))) - 1)))))) << (0 ?
 26:26)))        | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 25:16) - (0 ?
 25:16) + 1))) - 1)))))) << (0 ?
 25:16))) | (((vip_uint32_t) ((vip_uint32_t) (1) & ((vip_uint32_t) ((((1 ?
 25:16) - (0 ?
 25:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 25:16) - (0 ?
 25:16) + 1))) - 1)))))) << (0 ?
 25:16)))        | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 15:0) - (0 ?
 15:0) + 1))) - 1)))))) << (0 ?
 15:0))) | (((vip_uint32_t) ((vip_uint32_t) (0x0E02) & ((vip_uint32_t) ((((1 ?
 15:0) - (0 ?
 15:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 15:0) - (0 ? 15:0) + 1))) - 1)))))) << (0 ? 15:0)));}while(0);
    do {vip_uint32_t __temp_data32__;
__temp_data32__ = ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 4:0) - (0 ?
 4:0) + 1))) - 1)))))) << (0 ?
 4:0))) | (((vip_uint32_t) (0x01 & ((vip_uint32_t) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 4:0) - (0 ?
 4:0) + 1))) - 1)))))) << (0 ?
 4:0))) | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 23:20) - (0 ?
 23:20) + 1))) - 1)))))) << (0 ?
 23:20))) | (((vip_uint32_t) ((vip_uint32_t) (core_index + i) & ((vip_uint32_t) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 23:20) - (0 ?
 23:20) + 1))) - 1)))))) << (0 ?
 23:20))) | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 19:16) - (0 ?
 19:16) + 1))) - 1)))))) << (0 ?
 19:16))) | (((vip_uint32_t) ((vip_uint32_t) (0) & ((vip_uint32_t) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 19:16) - (0 ?
 19:16) + 1))) - 1)))))) << (0 ?
 19:16))) | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 12:8) - (0 ?
 12:8) + 1))) - 1)))))) << (0 ?
 12:8))) | (((vip_uint32_t) (0x0F & ((vip_uint32_t) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 12:8) - (0 ?
 12:8) + 1))) - 1)))))) << (0 ?
 12:8))) | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 27:24) - (0 ?
 27:24) + 1))) - 1)))))) << (0 ?
 27:24))) | (((vip_uint32_t) ((vip_uint32_t) (core_index + i - 1) & ((vip_uint32_t) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 27:24) - (0 ? 27:24) + 1))) - 1)))))) << (0 ? 27:24))) ;*memory++ = __temp_data32__;
} while(0);
}while(0);

        *memory++ = ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 31:27) - (0 ?
 31:27) + 1))) - 1)))))) << (0 ?
 31:27))) | (((vip_uint32_t) (0x09 & ((vip_uint32_t) ((((1 ?
 31:27) - (0 ?
 31:27) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 31:27) - (0 ? 31:27) + 1))) - 1)))))) << (0 ? 31:27)));

        *memory++ = ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 4:0) - (0 ?
 4:0) + 1))) - 1)))))) << (0 ?
 4:0))) | (((vip_uint32_t) (0x01 & ((vip_uint32_t) ((((1 ?
 4:0) - (0 ?
 4:0) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 4:0) - (0 ? 4:0) + 1))) - 1)))))) << (0 ? 4:0)))
            | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 23:20) - (0 ?
 23:20) + 1))) - 1)))))) << (0 ?
 23:20))) | (((vip_uint32_t) ((vip_uint32_t) (core_index + i) & ((vip_uint32_t) ((((1 ?
 23:20) - (0 ?
 23:20) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 23:20) - (0 ? 23:20) + 1))) - 1)))))) << (0 ? 23:20)))
            | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 19:16) - (0 ?
 19:16) + 1))) - 1)))))) << (0 ?
 19:16))) | (((vip_uint32_t) ((vip_uint32_t) (0) & ((vip_uint32_t) ((((1 ?
 19:16) - (0 ?
 19:16) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 19:16) - (0 ? 19:16) + 1))) - 1)))))) << (0 ? 19:16)))
            | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 12:8) - (0 ?
 12:8) + 1))) - 1)))))) << (0 ?
 12:8))) | (((vip_uint32_t) (0x0F & ((vip_uint32_t) ((((1 ?
 12:8) - (0 ?
 12:8) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ? 12:8) - (0 ? 12:8) + 1))) - 1)))))) << (0 ? 12:8)))
            | ((((vip_uint32_t) (0)) & ~(((vip_uint32_t) (((vip_uint32_t) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 27:24) - (0 ?
 27:24) + 1))) - 1)))))) << (0 ?
 27:24))) | (((vip_uint32_t) ((vip_uint32_t) (core_index + i - 1) & ((vip_uint32_t) ((((1 ?
 27:24) - (0 ?
 27:24) + 1) == 32) ?
 ~0 : (~0 & ((1 << (((1 ?
 27:24) - (0 ? 27:24) + 1))) - 1)))))) << (0 ? 27:24)));
    }

    num = (vip_uint32_t)((vip_uint8_t*)memory - (vip_uint8_t*)buffer);
    if (size != VIP_NULL) {
        *size = num;
    }

    return status;
}

static vip_status_e append_cmds(
    vip_uint32_t *logical,
    vip_bool_e with_event,
    vip_bool_e is_multi,
    vip_uint32_t core_id
    )
{
    /* semphore stall*/
    *logical++ = 0x08010e02;
    *logical++ = 0x30000701;
    *logical++ = 0x48000000;
    *logical++ = 0x30000701;
    *logical++ = 0x08010E03;
    *logical++ = 0x00000C23;

#if !vpmdTASK_PARALLEL_ASYNC
    /* enable core id */
    if (is_multi && with_event) {
        logical += cmd_chip_enable(logical, core_id, vip_false_e) / sizeof(vip_uint32_t);
    }

   /* Append the PE event command. */
    if (with_event) {
        logical += cmd_event(logical, (1 << 6), IRQ_EVENT_ID) / sizeof(vip_uint32_t);
    }

    /* enable all cores */
    if (is_multi && with_event) {
        logical += cmd_chip_enable(logical, core_id, vip_true_e) / sizeof(vip_uint32_t);
    }

    /* end */
    *logical++ = 0x10000000;
    *logical++ = 0x0;
#endif
    return VIP_SUCCESS;
}

static vip_status_e task_add_load_in_queue(
    vipdrv_device_t* device,
    vipdrv_core_load_t* loads
    )
{
    vip_uint32_t core_count = 0;
    vip_uint32_t start_core = 0;
    vip_uint32_t i = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t index = 0;
    vipdrv_queue_property_t* property = VIP_NULL;
    for (core_count = 1; core_count <= device->hardware_count; core_count++) {
        for (start_core = 0; start_core <= device->hardware_count - core_count; start_core++) {
            /* assume there are 4 cores, so there are 10 kinds of tasks.
               0    1    2    3
               01   12   23
               012  123
               0123
               if current task use core 1&2, the "index" is 5. */
            vipOnError(vipdrv_queue_query_property(&device->tskTCB_queue, (void*)(&index), &property));
            for (i = start_core; i < start_core + core_count; i++) {
                loads[i].estimated_time += property->estimated_time;
                loads[i].task_count += property->task_count;
            }
            index++;
        }
    }

onError:
    return status;
}

static vip_status_e task_add_loads_on_hardware(
    vipdrv_device_t* device,
    vipdrv_core_load_t* loads
    )
{
    vipdrv_task_control_block_t* tskTCB = VIP_NULL;
    vipdrv_hashmap_t* tskTCB_hashmap = &device->context->tskTCB_hashmap;
    vip_uint32_t index = 0;
    vipdrv_context_t* context = device->context;
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vip_uint32_t tcb_index = 0;

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    vip_uint32_t i = 0;
#if vpmdTASK_PARALLEL_ASYNC
    i = hw->wl_end_index; /* ignore subsequent tasks */
    while (i != hw->wl_start_index)
    {
        i = (i + WL_TABLE_COUNT - 1) % WL_TABLE_COUNT;
        tcb_index = hw->wl_table[i].tcb_index;
#elif vpmdTASK_PARALLEL_SYNC
    {
        tcb_index = hw->tcb_index;
#endif
        if (0 == tcb_index) {
            goto NEXT_HW;
        }
        vipdrv_hashmap_get_by_index(tskTCB_hashmap, tcb_index, (void**)&tskTCB);
        if (VIP_NULL == tskTCB) {
            goto NEXT_HW;
        }
        index = vipdrv_task_desc_id2index(tskTCB->task.task_id);
        if (VIPDRV_TASK_READY != tskTCB->status && VIPDRV_TASK_INFER_START != tskTCB->status) {
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            /* if one task is end or cancel, front tasks has done */
            goto NEXT_HW;
        }
        if (VIP_SUCCESS != vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            goto NEXT_HW;
        }

        if (VIPDRV_TASK_READY == tskTCB->status) {
            #if vpmdTASK_PARALLEL_ASYNC
            loads[hw_index].estimated_time += tsk_desc->subtasks[tskTCB->task.subtask_index].avg_time;
            #else
            for (i = 0; i < tskTCB->task.core_cnt; i++) {
                loads[tskTCB->task.core_index + i].estimated_time +=
                    tsk_desc->subtasks[tskTCB->task.subtask_index].avg_time;
            }
            #endif
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            vipdrv_database_unuse(&context->tskDesc_database, index);
            /* only one task can be VIPDRV_TASK_READY */
            goto NEXT_HW;
        }
        else if (VIPDRV_TASK_INFER_START == tskTCB->status) {
            vip_uint64_t time = vipdrv_os_get_time();
            VIPDRV_SUB(time, tsk_desc->subtasks[tskTCB->task.subtask_index].cur_time, time, vip_uint64_t);
            if (time < tsk_desc->subtasks[tskTCB->task.subtask_index].avg_time) {
                time = tsk_desc->subtasks[tskTCB->task.subtask_index].avg_time - time;
                #if vpmdTASK_PARALLEL_ASYNC
                loads[hw_index].estimated_time += time;
                #else
                for (i = 0; i < tskTCB->task.core_cnt; i++) {
                    loads[tskTCB->task.core_index + i].estimated_time += time;
                }
                #endif
            }
        }
        else {
            vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
            vipdrv_database_unuse(&context->tskDesc_database, index);
            /* if one task is end or cancel, front tasks has done */
            goto NEXT_HW;
        }
        vipdrv_hashmap_unuse(tskTCB_hashmap, tcb_index, vip_false_e);
        vipdrv_database_unuse(&context->tskDesc_database, index);
    }
NEXT_HW:;
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END

    return VIP_SUCCESS;
}

static vip_uint32_t task_schedule(
    vipdrv_device_t* device,
    vipdrv_core_load_t* loads,
    vip_uint32_t core_cnt
    )
{
    vip_uint32_t schedule_core = 0;
    vip_uint32_t i = 0;
    vip_uint32_t left = 0, right = 0;
    vip_uint32_t window[vipdMAX_CORE] = { 0 };
    vip_uint64_t min_time = -1;
    for (i = 0; i < device->hardware_count; i++) {
        if (left != right && i - window[left] >= core_cnt) {
            left++;
        }

        while (left != right && loads[window[right - 1]].estimated_time <= loads[i].estimated_time) {
            right--;
        }

        window[right++] = i;

        if (i >= core_cnt - 1) {
            if (min_time > loads[window[left]].estimated_time) {
                min_time = loads[window[left]].estimated_time;
                schedule_core = i + 1 - core_cnt;
            }
        }
    }

    return schedule_core;
}

static vip_status_e task_patch_command(
    vipdrv_device_t* device,
    vipdrv_submit_task_param_t* sbmt_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint8_t* logical = VIP_NULL;
    vip_bool_e with_event = (0 == vpmdENABLE_POLLING) ? vip_true_e : vip_false_e;
    vip_uint32_t task_index = vipdrv_task_desc_id2index(sbmt_para->task_id);
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vipdrv_context_t* context = device->context;
    vip_uint32_t i = 0, j = 0;
    vip_uint8_t need_append = vip_false_e;
    vipdrv_subtask_info_t* sub_task = VIP_NULL;
    vip_uint32_t core_id = device->hardware[sbmt_para->core_index].core_id;
    status = vipdrv_database_use(&context->tskDesc_database, task_index, (void**)&tsk_desc);
    if (VIP_SUCCESS != status) {
        return status;
    }

    /* patch for multi-core */
    if (1 < sbmt_para->core_cnt) {
        for (i = 0; i < sbmt_para->subtask_cnt; i++) {
            sub_task = &tsk_desc->subtasks[i + sbmt_para->subtask_index];
            if (sub_task->core_index == sbmt_para->core_index) {
                continue;
            }
            PRINTK_D("task schedule patch cmd task_id=0x%x %d core from %d to %d\n",
                 sbmt_para->task_id, sbmt_para->subtask_index, sub_task->core_index,
                 sbmt_para->core_index);
            sub_task->core_index = sbmt_para->core_index;
            need_append = vip_true_e;
            vipOnError(vipdrv_mem_get_info(context, sub_task->cmd.mem_id,
                                           &logical, VIP_NULL, VIP_NULL, VIP_NULL));
            for (j = 0; j < sub_task->patch_cnt; j++) {
                vipdrv_subtask_patch_info_t* patch_info = &sub_task->patch_info[j];
                switch (patch_info->type) {
                case VIPDRV_TASK_PATCH_SYNC:
                {
                    vip_uint32_t size_tmp = 0;
                    hardware_multivip_sync((vip_uint32_t*)(logical + patch_info->offset),
                        &size_tmp, core_id + patch_info->core_index, patch_info->core_count);
                    if (size_tmp != patch_info->size) {
                        PRINTK_E("sync cmd size is not same %u != %u\n", size_tmp, patch_info->size);
                    }
                }
                break;

                case VIPDRV_TASK_PATCH_CHIP:
                {
                    vip_uint32_t* command = (vip_uint32_t*)(logical + patch_info->offset);
                    command[0] = 0x68000000 | (1 << (core_id + patch_info->core_index));
                }
                break;

                default:
                    break;
                }
            }
            #if vpmdENABLE_VIDEO_MEMORY_CACHE
            if (i != sbmt_para->subtask_cnt - 1) {
                vipdrv_mem_flush_cache(sub_task->cmd.mem_id, VIPDRV_CACHE_FLUSH);
            }
            #endif
        }
    }

    /* append command */
    sub_task = &tsk_desc->subtasks[sbmt_para->subtask_index + sbmt_para->subtask_cnt - 1];
    if ((vip_true_e == need_append) || (sub_task->core_index != sbmt_para->core_index)) {
        vipOnError(vipdrv_mem_get_info(context, sub_task->cmd.mem_id,
                            &logical, VIP_NULL, VIP_NULL, VIP_NULL));
        append_cmds((vip_uint32_t*)(logical + sub_task->cmd.size), with_event,
                       1 < sbmt_para->core_cnt ? vip_true_e : vip_false_e, core_id);
        #if vpmdENABLE_VIDEO_MEMORY_CACHE
        vipdrv_mem_flush_cache(sub_task->cmd.mem_id, VIPDRV_CACHE_FLUSH);
        #endif
        PRINTK_D("task schedule append cmd task_id=0x%x %d core from %d to %d\n",
             sbmt_para->task_id, sbmt_para->subtask_index, sub_task->core_index,
             sbmt_para->core_index);
    }

onError:
    vipdrv_database_unuse(&context->tskDesc_database, task_index);
    return status;
}

vip_status_e vipdrv_task_schedule(
    vipdrv_device_t *device,
    vipdrv_submit_task_param_t *sbmt_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_core_load_t loads[vipdMAX_CORE];
    vip_uint32_t schedule_core = 0;
    vipdrv_os_zero_memory(loads, sizeof(vipdrv_core_load_t) * vipdMAX_CORE);

    if (0 == sbmt_para->need_schedule) {
        return status;
    }

    vipOnError(task_add_load_in_queue(device, loads));
    vipOnError(task_add_loads_on_hardware(device, loads));
    schedule_core = task_schedule(device, loads, sbmt_para->core_cnt);
#if (4 <= vpmdENABLE_DEBUG_LOG)
    task_schedule_dump(device, sbmt_para, loads, schedule_core);
#endif
    sbmt_para->core_index = schedule_core;
    vipOnError(task_patch_command(device, sbmt_para));

    return status;
onError:
    PRINTK_E("fail to task schedule status=%d\n", status);
    return status;
}
#else
vip_status_e vipdrv_task_schedule(
    vipdrv_device_t *device,
    vipdrv_submit_task_param_t *sbmt_para
    )
{
    return VIP_SUCCESS;
}
#endif

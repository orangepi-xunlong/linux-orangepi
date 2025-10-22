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
#include <vip_drv_task_debug.h>
#include <vip_drv_task_descriptor.h>
#include <vip_drv_debug.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_TASK


#if vpmdENABLE_HANG_DUMP
vip_status_e vipdrv_task_tcb_dump(
    vipdrv_task_t *task
    )
{
    vipdrv_context_t* context = task->device->context;
    vip_uint32_t index = vipdrv_task_desc_id2index(task->task_id);
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint8_t* logical = VIP_NULL;
    vip_address_t physical = 0;
    vip_size_t size = 0;

    if (VIP_SUCCESS == vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
        PRINTK("**************************\n");
        PRINTK("   COMMAND BUF DUMP\n");
        PRINTK("**************************\n");

        if (0 != tsk_desc->init_cmd.mem_id) {
            vipdrv_mem_get_info(context, tsk_desc->init_cmd.mem_id, &logical, &physical, &size, VIP_NULL);
            PRINTK("init command total size=0x%"PRIx64", physical=0x%"PRIx64", logical=0x%"PRPx"\n",
                    size, physical, logical);
        }

        if (0 != tsk_desc->preload_cmd.mem_id) {
            vipdrv_mem_get_info(context, tsk_desc->preload_cmd.mem_id, &logical, &physical, &size, VIP_NULL);
            PRINTK("preload command total size=0x%"PRIx64", physical=0x%"PRIx64", logical=0x%"PRPx"\n",
                    size, physical, logical);
        }

        for (i = 0; i < task->subtask_count; i++) {
            vip_uint32_t mem_id = tsk_desc->subtasks[task->subtask_index + i].cmd.mem_id;
            if (VIP_SUCCESS == vipdrv_mem_get_info(task->device->context, mem_id,
                &logical, &physical, &size, VIP_NULL)) {
                PRINTK("command %d total size=0x%"PRIx64", physical=0x%"PRIx64", logical=0x%"PRPx"\n",
                    i, size, physical, logical);
                vipdrv_dump_data((vip_uint32_t*)logical, (vip_uint32_t)physical, size);
            }
            else {
                PRINTK("fail to get subtask_%d memory\n", i);
            }
        }

        vipdrv_database_unuse(&context->tskDesc_database, index);
    }
    else {
        PRINTK("fail to get tsk desc index=0x%x\n", index);
    }

    return VIP_SUCCESS;
}

vip_status_e vipdrv_task_desc_dump(
    vipdrv_task_t *task
    )
{
    vip_uint32_t index = vipdrv_task_desc_id2index(task->task_id);
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vipdrv_context_t* context = task->device->context;

    if (VIP_SUCCESS == vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
        PRINTK("**********dump task desc start***********\n");

        PRINTK("task_id=0x%x, name=%s, subtask_count=%d, skip=%d, pid=0x%x\n",
            task->task_id, tsk_desc->task_name, tsk_desc->subtask_count, tsk_desc->skip, tsk_desc->pid);
        if (tsk_desc->preload_cmd.mem_id != 0) {
            PRINTK("    preload memid=%d, size=%d\n", tsk_desc->preload_cmd.mem_id, tsk_desc->preload_cmd.size);
        }
        if (tsk_desc->init_cmd.mem_id != 0) {
            PRINTK("    init cmd memid=%d, size=%d\n", tsk_desc->init_cmd.mem_id, tsk_desc->init_cmd.size);
        }

        PRINTK("**********dump task desc end***********\n");
        vipdrv_database_unuse(&context->tskDesc_database, index);
    }

    return VIP_SUCCESS;
}
#endif

vip_status_e vipdrv_task_submit_parameter_dump(
    vipdrv_submit_task_param_t *sbmt_para
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t index = vipdrv_task_desc_id2index(sbmt_para->task_id);
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;

    PRINTK("**********dump submit task param start***********\n");
    PRINTK("task_id=0x%x, timeout=%dms\n", sbmt_para->task_id, sbmt_para->time_out);
    VIPDRV_SHOW_DEV_CORE_INFO(sbmt_para->device_index, sbmt_para->core_cnt,
                              sbmt_para->core_index, "submit task param");

    if (VIP_SUCCESS == vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
        PRINTK("pid=0x%x, task name=%s\n", tsk_desc->pid, tsk_desc->task_name);
        PRINTK("subtsk_idex=%d, subtsk_cnt=%d, subtask list:\n",
                  sbmt_para->subtask_index, sbmt_para->subtask_cnt);
        for (i = 0; i < sbmt_para->subtask_cnt; i++) {
            vip_uint32_t mem_id = tsk_desc->subtasks[sbmt_para->subtask_index + i].cmd.mem_id;
            vip_uint8_t* logical = VIP_NULL;
            vip_address_t physical = 0;
            vip_size_t size = 0;
            if (VIP_SUCCESS == vipdrv_mem_get_info(context, mem_id, &logical, &physical, &size, VIP_NULL)) {
                PRINTK("subtask[%d] mem_id=0x%x, logical=0x%"PRPx", physical=0x%"PRIx64", size=0x%x\n",
                    i, mem_id, logical, physical, size);
            }
        }
        PRINTK("**********dump submit task param end, maxcnt=%d***********\n", tsk_desc->subtask_count);
        vipdrv_database_unuse(&context->tskDesc_database, index);
    }

    return VIP_SUCCESS;
}

void vipdrv_task_history_record(
    vipdrv_task_t *task
    )
{
    vipdrv_context_t* context = task->device->context;
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vip_uint32_t index = vipdrv_task_desc_id2index(task->task_id);
    if (VIP_SUCCESS == vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
        tsk_desc->subtasks[task->subtask_index].submit_count++;
        vipdrv_database_unuse(&context->tskDesc_database, index);
    }

    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    /* array is full, overwrite oldest record */
    if ((hw->pre_end_index + 1) % TASK_HISTORY_RECORD_COUNT == hw->pre_start_index) {
        hw->pre_start_index = (hw->pre_start_index + 1) % TASK_HISTORY_RECORD_COUNT;
    }
    hw->previous_task_id[hw->pre_end_index] = task->task_id;
    hw->previous_subtask_index[hw->pre_end_index] = task->subtask_index;
    hw->pre_end_index = (hw->pre_end_index + 1) % TASK_HISTORY_RECORD_COUNT;
    hw->hw_submit_count++;
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
}

vip_status_e vipdrv_task_history_dump(
    vipdrv_task_t *task
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t *device = task->device;
    vipdrv_context_t *context = device->context;
    vip_uint32_t index = 0;
    vip_uint32_t i = vipdrv_task_desc_id2index(task->task_id);
    vip_uint32_t count = 0;
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;

    PRINTK("******************** dump previous inference network start ********************\n");
    VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
    count = 0;
    index = hw->pre_start_index;
    PRINTK("index    task-id     task-name   subtsk-index   infer-cnt\n");
    while (index != hw->pre_end_index) {
        vip_uint32_t task_id = hw->previous_task_id[index];
        vip_uint32_t subtask_index = hw->previous_subtask_index[index];
        i = vipdrv_task_desc_id2index(task_id);
        if (VIP_SUCCESS == vipdrv_database_use(&context->tskDesc_database, i, (void**)&tsk_desc)) {
            PRINTK("[%02d] 0x%x %s %d inference count:%d\n", count, task_id,
                tsk_desc->task_name, subtask_index, tsk_desc->subtasks[subtask_index].submit_count);
            vipdrv_database_unuse(&context->tskDesc_database, i);
        }
        index = (index + 1) % TASK_HISTORY_RECORD_COUNT;
        count++;
    }
    VIPDRV_LOOP_HARDWARE_IN_TASK_END
    PRINTK("******************** dump previous inference network end **********************\n");
    return status;
}

vip_status_e vipdrv_task_profile_start(
    vipdrv_task_t *task
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t* device = task->device;
    vipdrv_context_t* context = device->context;
    vip_uint32_t index = vipdrv_task_desc_id2index(task->task_id);
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;

#if vpmdENABLE_DEBUGFS
    vipdrv_debug_core_loading_profile_set(task, PROFILE_START);
#endif
    if (VIP_SUCCESS == vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
        vipdrv_subtask_info_t* info = &tsk_desc->subtasks[task->subtask_index];
        #if vpmdENABLE_TASK_PROFILE
        vipdrv_hardware_t* hardware = VIP_NULL;
        vip_uint32_t i = 0;
        #if vpmdENABLE_APP_PROFILING
        vipdrv_task_profile_cycle_t* cur_cycle = VIP_NULL;
        #endif
        #if vpmdENABLE_BW_PROFILING
        vipdrv_task_profile_bw_t* cur_bw = VIP_NULL;
        #endif
        for (i = 0; i < task->core_cnt; i++) {
            hardware = &device->hardware[task->core_index + i];
            VIPDRV_CHECK_NULL(hardware);
            #if vpmdENABLE_APP_PROFILING
            cur_cycle = &info->cur_cycle[i];
            vipdrv_read_register(hardware, 0x00078, &cur_cycle->total_cycle);
            vipdrv_read_register(hardware, 0x0007C, &cur_cycle->total_idle_cycle);
            #endif
            #if vpmdENABLE_BW_PROFILING
            cur_bw = &info->cur_bw[i];
            vipdrv_read_register(hardware, 0x00040, &cur_bw->total_read);
            vipdrv_read_register(hardware, 0x00044, &cur_bw->total_write);
            if (vipdrv_context_support(VIP_HW_FEATURE_OCB_COUNTER)) {
                vipdrv_read_register(hardware, 0x005C0, &cur_bw->total_read_ocb);
                vipdrv_read_register(hardware, 0x005D0, &cur_bw->total_write_ocb);
            }
            #endif
        }
        #endif
        info->cur_time = vipdrv_os_get_time();
        vipdrv_database_unuse(&context->tskDesc_database, index);
    }
    else {
        PRINTK_D("fail to profile time start task_id=0x%x\n", task->task_id);
    }

    return status;
}

/* record task profile data into first subtask which used by the TCB */
vip_status_e vipdrv_task_profile_end(
    vipdrv_task_t *task
    )
{
    vipdrv_task_descriptor_t *tsk_desc = VIP_NULL;
    vip_status_e status = VIP_SUCCESS;
    vipdrv_device_t *device = task->device;
    vipdrv_context_t *context = device->context;
    vip_uint32_t index = vipdrv_task_desc_id2index(task->task_id);
    vip_uint64_t time = vipdrv_os_get_time();
    vip_uint64_t cur_tmp_time = 0, avg_tmp_time = 0;

#if vpmdENABLE_DEBUGFS
    vipdrv_debug_core_loading_profile_set(task, PROFILE_END);
#endif
    if (VIP_SUCCESS == vipdrv_database_use(&context->tskDesc_database, index, (void**)&tsk_desc)) {
        vipdrv_subtask_info_t *subtask_info = &tsk_desc->subtasks[task->subtask_index];
        vip_uint32_t submit_count = subtask_info->submit_count < NUM_OF_AVG_TIME ?
            subtask_info->submit_count : NUM_OF_AVG_TIME;
        #if vpmdENABLE_TASK_PROFILE
        vipdrv_hardware_t* hardware = VIP_NULL;
        vip_uint32_t i = 0;
        #if vpmdENABLE_APP_PROFILING
        vip_uint32_t total_cycle = 0;
        vip_uint32_t total_idle_cycle = 0;
        vipdrv_task_profile_cycle_t *cur_cycle = VIP_NULL;
        vipdrv_task_profile_cycle_t *avg_cycle = VIP_NULL;
        #endif
        #if vpmdENABLE_BW_PROFILING
        vipdrv_task_profile_bw_t* cur_bw = VIP_NULL;
        vip_uint32_t total_read = 0, total_write = 0;
        vip_uint32_t total_read_ocb = 0, total_write_ocb = 0;
        #endif
        if (0 < submit_count) {
            for (i = 0; i < task->core_cnt; i++) {
                hardware = &device->hardware[task->core_index + i];
                VIPDRV_CHECK_NULL(hardware);
                #if vpmdENABLE_APP_PROFILING
                cur_cycle = &subtask_info->cur_cycle[i];
                avg_cycle = &subtask_info->avg_cycle[i];
				vipdrv_read_register(hardware, 0x0007C, &total_idle_cycle);
                vipdrv_read_register(hardware, 0x00078, &total_cycle);
                VIPDRV_SUB(total_cycle, cur_cycle->total_cycle, cur_cycle->total_cycle, vip_uint32_t);
                VIPDRV_SUB(total_idle_cycle, cur_cycle->total_idle_cycle, cur_cycle->total_idle_cycle, vip_uint32_t);
                avg_cycle->total_cycle = avg_cycle->total_cycle +
                                         cur_cycle->total_cycle / submit_count -
                                         avg_cycle->total_cycle / submit_count;
                avg_cycle->total_idle_cycle = avg_cycle->total_idle_cycle +
                                          cur_cycle->total_idle_cycle / submit_count -
                                          avg_cycle->total_idle_cycle / submit_count;
                PRINTK_D("core_index=%d total_cycle=%u, idle_cycle=%u\n",
                         i, cur_cycle->total_cycle, cur_cycle->total_idle_cycle);
                #endif
                #if vpmdENABLE_BW_PROFILING
                cur_bw = &subtask_info->cur_bw[i];
                vipdrv_read_register(hardware, 0x00040, &total_read);
                vipdrv_read_register(hardware, 0x00044, &total_write);
                VIPDRV_SUB(total_read, cur_bw->total_read, cur_bw->total_read, vip_uint32_t);
                VIPDRV_SUB(total_write, cur_bw->total_write, cur_bw->total_write, vip_uint32_t);
                if (vipdrv_context_support(VIP_HW_FEATURE_OCB_COUNTER)) {
                    vipdrv_read_register(hardware, 0x005C0, &total_read_ocb);
                    vipdrv_read_register(hardware, 0x005D0, &total_write_ocb);
                    VIPDRV_SUB(total_read_ocb, cur_bw->total_read_ocb, cur_bw->total_read_ocb, vip_uint32_t);
                    VIPDRV_SUB(total_write_ocb, cur_bw->total_write_ocb, cur_bw->total_write_ocb, vip_uint32_t);
                }
                PRINTK_D("core_index=%d total_read=%u, total_write=%u, read_ocb=%u, write_ocb=%u\n",
                     i, cur_bw->total_read, cur_bw->total_write, cur_bw->total_read_ocb,
                     cur_bw->total_write_ocb);
                #endif
            }
        }
        #endif
        VIPDRV_SUB(time, subtask_info->cur_time, subtask_info->cur_time, vip_uint64_t);
        VIPDRV_DO_DIV64(subtask_info->cur_time, submit_count, cur_tmp_time);
        VIPDRV_DO_DIV64(subtask_info->avg_time, submit_count, avg_tmp_time);
        if (1 == submit_count) {
            subtask_info->avg_time = cur_tmp_time;
        }
        else {
            subtask_info->avg_time = subtask_info->avg_time + cur_tmp_time - avg_tmp_time;
        }
        PRINTK_D("task_id=0x%x, subtsk_index=%d, inference_time=%dus\n",
            task->task_id, task->subtask_index, subtask_info->cur_time);
        vipdrv_database_unuse(&context->tskDesc_database, index);
    }
    else {
        PRINTK_D("end profiling can't find task_id=0x%x\n", task->task_id);
    }

    return status;
}

#if vpmdENABLE_TASK_PROFILE
void vipdrv_hw_profile_reset(
    vipdrv_hardware_t *hardware
    )
{
    /* clean counter */
    vipdrv_write_register(hardware, 0x0003C, 1);
    vipdrv_write_register(hardware, 0x0003C, 0);

#if vpmdENABLE_APP_PROFILING
    vipdrv_write_register(hardware, 0x00078, 0);
    vipdrv_write_register(hardware, 0x0007C, 0);
#endif
}

vip_status_e vipdrv_task_profile_query(
    vipdrv_query_profiling_t *profiling
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint32_t index = vipdrv_task_desc_id2index(profiling->task_id);
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);

    PRINTK_D("query profile task_id=0x%x, subtsk_index=%d, index=0x%x\n",
             profiling->task_id, profiling->subtask_index, index);

    status = vipdrv_database_use(tskDesc_database, index, (void**)&tsk_desc);
    if (status == VIP_SUCCESS) {
        vipdrv_subtask_info_t* info = &tsk_desc->subtasks[profiling->subtask_index];

        profiling->inference_time = (vip_uint32_t)info->cur_time;

        for (i = 0; i < vipdMAX_CORE; i++) {
            #if vpmdENABLE_APP_PROFILING
            profiling->total_cycle[i] = info->cur_cycle[i].total_cycle;
            profiling->total_idle_cycle[i] = info->cur_cycle[i].total_idle_cycle;
            #endif
            #if vpmdENABLE_BW_PROFILING
            profiling->total_read[i] = info->cur_bw[i].total_read;
            profiling->total_write[i] = info->cur_bw[i].total_write;
            profiling->total_read_ocb[i] = info->cur_bw[i].total_read_ocb;
            profiling->total_write_ocb[i] = info->cur_bw[i].total_write_ocb;
            #endif
        }

        vipdrv_database_unuse(tskDesc_database, index);
    }
    else {
        PRINTK_D("can't find this task_id=0x%x tskDesc\n", profiling->task_id);
        profiling->inference_time = 0;
    }

    return status;
}
#endif


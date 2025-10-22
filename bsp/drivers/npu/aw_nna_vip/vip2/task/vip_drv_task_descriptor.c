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

#include <vip_drv_task_descriptor.h>
#include <vip_drv_context.h>
#include <vip_drv_device_driver.h>
#include <vip_drv_task_common.h>
#include <vip_drv_debug.h>
#include <vip_drv_task_schedule.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_TASK


static vip_status_e free_task_descriptor(
    void* element
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_task_descriptor_t* tsk_desc = (vipdrv_task_descriptor_t*)element;
    if (VIP_NULL != tsk_desc->subtasks) {
        #if vpmdTASK_SCHEDULE
        vip_uint32_t i = 0;
        for (i = 0; i < tsk_desc->subtask_count; i++) {
            if (VIP_NULL != tsk_desc->subtasks[i].patch_info) {
                vipdrv_os_free_memory(tsk_desc->subtasks[i].patch_info);
                tsk_desc->subtasks[i].patch_info = VIP_NULL;
            }
        }
        #endif
        vipdrv_os_free_memory(tsk_desc->subtasks);
        tsk_desc->subtasks = VIP_NULL;
    }
    tsk_desc->pid = 0;
    tsk_desc->subtask_count = 0;

    return status;
}

vip_status_e vipdrv_task_desc_init(void) {
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    return vipdrv_database_init(tskDesc_database, DATABASE_CAPABILITY,
        sizeof(vipdrv_task_descriptor_t), "task-desc", free_task_descriptor);
}

vip_status_e vipdrv_task_desc_uninit(void) {
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    vipdrv_database_destroy(tskDesc_database);
    return VIP_SUCCESS;
}

static vip_bool_e task_descriptor_check_skip(
    vip_uint32_t task_id
    )
{
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vip_uint32_t index = vipdrv_task_desc_id2index(task_id);
    vip_bool_e skip = vip_true_e;
    if (VIP_SUCCESS == vipdrv_database_use(tskDesc_database, index, (void**)&tsk_desc)) {
        if (tsk_desc->pid == vipdrv_os_get_pid() && /* each process submit own task */
            !tsk_desc->skip) {                      /* set by debugfs */
            skip = vip_false_e;
        }
        vipdrv_database_unuse(tskDesc_database, index);
    }
    return skip;
}

static vip_status_e vipdrv_task_desc_submit_param_check(
    vipdrv_device_t *device,
    vipdrv_submit_task_param_t *sbmt_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t task_id = sbmt_para->task_id;
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vip_uint32_t index = vipdrv_task_desc_id2index(task_id);

    if (sbmt_para->core_index + sbmt_para->core_cnt > device->hardware_count) {
        PRINTK_E("device core cnt=%d, but commit need core [%d, %d)\n", device->hardware_count,
            sbmt_para->core_index, sbmt_para->core_index + sbmt_para->core_cnt);
        vipGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    status = vipdrv_database_use(tskDesc_database, index, (void**)&tsk_desc);
    if (VIP_SUCCESS == status) {
        /* each process submit own task */
        if (tsk_desc->pid != vipdrv_os_get_pid()) {
            PRINTK_E("fail tsk_desc pid=0x%x not match cur pid\n", tsk_desc->pid);
            vipdrv_database_unuse(tskDesc_database, index);
            vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
        if ((sbmt_para->subtask_index + sbmt_para->subtask_cnt) > tsk_desc->subtask_count) {
            vipdrv_database_unuse(tskDesc_database, index);
            vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
        vipdrv_database_unuse(tskDesc_database, index);
    }
    else {
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    return status;
onError:
    PRINTK_E("fail check task submit param\n");
    vipdrv_task_submit_parameter_dump(sbmt_para);
    return status;
}

vip_status_e vipdrv_task_desc_submit(
    vipdrv_submit_task_param_t *sbmt_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t *context = VIP_NULL;
    vipdrv_device_t *device = VIP_NULL;

    device = vipdrv_get_device(sbmt_para->device_index);
    vipIsNULL(device);
    context = device->context;

#if (vpmdENABLE_DEBUG_LOG >= 4) && (VIPDRV_LOG_ZONE_ENABLED & VIPDRV_LOG_ZONE_TASK)
    vipdrv_task_submit_parameter_dump(sbmt_para);
#endif

    vipOnError(vipdrv_task_desc_submit_param_check(device, sbmt_para));
    vipOnError(vipdrv_task_schedule(device, sbmt_para));

#if vpmdENABLE_USER_CONTROL_POWER
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_MtoN_START(device, sbmt_para->core_index, sbmt_para->core_cnt)
    VIPDRV_CHECK_USER_PM_STATUS();
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_MtoN_END
#endif

    if (vip_true_e == task_descriptor_check_skip(sbmt_para->task_id)) {
        PRINTK_I("user do submit skip task_id=0x%x\n", sbmt_para->task_id);
    }
    else {
        vipOnError(vipdrv_task_tcb_submit(device, sbmt_para));
    }

    VIPDRV_LOOP_HARDWARE_IN_DEVICE_MtoN_START(device, sbmt_para->core_index, sbmt_para->core_cnt)
    hw->user_submit_count++;
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_MtoN_END

    PRINTK_I("user end do submit task_id=0x%x device%d\n", sbmt_para->task_id, device->device_index);
    return status;
onError:
    PRINTK_E("fail submit task desc task_id=0x%x status=%d\n", sbmt_para->task_id, status);
    return status;
}

vip_status_e vipdrv_task_desc_wait(
    vipdrv_wait_task_param_t *wait_para
    )
{
    vip_status_e status = VIP_SUCCESS;
    PRINTK_I("user start do wait task_id=0x%x, subtsk_idex=%d\n",
            wait_para->task_id, wait_para->subtask_index);

    if (vip_true_e == task_descriptor_check_skip(wait_para->task_id)) {
        PRINTK_I("user do wait skip task_id=0x%x subtsk_idex=%d\n",
                wait_para->task_id, wait_para->subtask_index);
    }
    else {
        vipOnError(vipdrv_task_tcb_wait(wait_para));
    }
    PRINTK_I("user end do wait task_id=0x%x subtsk_idex=%d\n",
             wait_para->task_id, wait_para->subtask_index);
onError:
    return status;
}

/* the index of task descriptor database to task_id */
vip_uint32_t vipdrv_task_desc_index2id(
    vip_uint32_t index
    )
{
    vip_uint32_t task_id = TASK_MAGIC_DATA + (vip_uint32_t)(index << TASK_INDEX_SHIFT);

    return task_id;
}

/* covert task_id to index of task descriptor database */
vip_uint32_t vipdrv_task_desc_id2index(
    vip_uint32_t task_id
    )
{
    vip_uint32_t index = -1;
    if (task_id >= TASK_MAGIC_DATA) {
        index = (task_id - TASK_MAGIC_DATA) >> TASK_INDEX_SHIFT;
    }

    return index;
}

vip_status_e vipdrv_task_desc_create(
    vipdrv_create_task_param_t *tsk_param
    )
{
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0, j = 0;
    vipdrv_task_descriptor_t *tsk_desc = VIP_NULL;

    if (0 == tsk_param->subtask_count) {
        PRINTK_E("fail task should have one sub-task at least\n");
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }
    if (tsk_param->subtask_count > (1 << TASK_INDEX_SHIFT)) {
        PRINTK_E("fail max support subtsk cnt=%d, cur cnt=%d\n", 1 << TASK_INDEX_SHIFT, tsk_param->subtask_count);
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

Expand:
    if (vip_true_e == vipdrv_database_full(tskDesc_database)) {
        vip_bool_e expand = vip_false_e;
        status = vipdrv_database_expand(tskDesc_database, &expand);
        if (VIP_SUCCESS != status) {
            PRINTK_E("task information hashmap is full\n");
            vipGoOnError(VIP_ERROR_FAILURE);
        }
    }
    status = vipdrv_database_insert(tskDesc_database, &i);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        goto Expand;
    }
    tsk_param->task_id = vipdrv_task_desc_index2id(i);
    vipOnError(vipdrv_database_use(tskDesc_database, i, (void**)&tsk_desc));
    vipdrv_os_zero_memory((void*)tsk_desc, sizeof(vipdrv_task_descriptor_t));
    tsk_desc->subtask_count = tsk_param->subtask_count;
    tsk_desc->pid = vipdrv_os_get_pid();
    vipdrv_os_snprint(tsk_desc->task_name, TASK_NAME_LENGTH, tsk_param->task_name);
    status = vipdrv_os_allocate_memory(sizeof(vipdrv_subtask_info_t) * tsk_desc->subtask_count,
                                         (void**)&tsk_desc->subtasks);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to create task, allocate fail\n");
        vipdrv_database_remove(tskDesc_database, i, vip_false_e);
        vipdrv_database_unuse(tskDesc_database, i);
        vipGoOnError(status);
    }
    vipdrv_os_zero_memory(tsk_desc->subtasks, sizeof(vipdrv_subtask_info_t) * tsk_desc->subtask_count);
    for (j = 0; j < tsk_desc->subtask_count; j++) {
        tsk_desc->subtasks[j].avg_time = DEFAULT_INFER_TIME;
    }
    PRINTK_D("create task name=%s, pid=0x%x, task_id=0x%x, max subtask cnt=%d\n",
        tsk_desc->task_name, tsk_desc->pid, tsk_param->task_id, tsk_param->subtask_count);
    vipdrv_database_unuse(tskDesc_database, i);

    return status;
onError:
    PRINTK_E("fail to create task desc property name=%s status=%d\n", tsk_param->task_name, status);
    return status;
}

vip_status_e vipdrv_task_desc_set_property(
    vipdrv_set_task_property_t *task_property
    )
{
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t index = vipdrv_task_desc_id2index(task_property->task_id);
    vipdrv_task_descriptor_t *tsk_desc = VIP_NULL;

    vipOnError(vipdrv_database_use(tskDesc_database, index, (void**)&tsk_desc));
    if ((VIPDRV_TASK_CMD_SUBTASK == task_property->type) && (task_property->index >= tsk_desc->subtask_count)) {
        PRINTK_E("fail set task_id=0x%x, subtask index=%d is out of range [0, %d)\n",
            task_property->task_id, task_property->index, tsk_desc->subtask_count);
        vipdrv_database_unuse(tskDesc_database, index);
        vipGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
    }

    switch (task_property->type) {
    case VIPDRV_TASK_CMD_SUBTASK:
    {
        switch (task_property->patch_type) {
        case VIPDRV_TASK_PATCH_NONE:
        {
            vipdrv_subtask_info_t* subtask = &tsk_desc->subtasks[task_property->index];
            #if vpmdTASK_SCHEDULE
            vip_uint32_t patch_cnt = task_property->param.subtask_info.patch_count;
            vip_uint32_t mem_size = sizeof(vipdrv_subtask_patch_info_t) * patch_cnt;
            if (0 != patch_cnt && patch_cnt != subtask->patch_cnt) {
                if (VIP_NULL != subtask->patch_info) {
                    vipdrv_os_free_memory(subtask->patch_info);
                    subtask->patch_info = VIP_NULL;
                    subtask->patch_cnt = 0;
                }

                status = vipdrv_os_allocate_memory(mem_size, (void**)&subtask->patch_info);
                if (VIP_SUCCESS != status) {
                    PRINTK_E("fail to create patch info for subtask, patch_cnt=%u\n", patch_cnt);
                    vipdrv_database_unuse(tskDesc_database, index);
                    vipGoOnError(VIP_ERROR_OUT_OF_RESOURCE);
                }
                vipdrv_os_zero_memory(subtask->patch_info, mem_size);
                subtask->patch_cnt = patch_cnt;
            }
            subtask->core_index = task_property->param.subtask_info.core_index;
            #endif

            subtask->cmd.mem_id = task_property->param.subtask_info.subtask_id;
            subtask->cmd.size = task_property->param.subtask_info.size;
        }
        break;

        #if vpmdTASK_SCHEDULE
        case VIPDRV_TASK_PATCH_SYNC:
        case VIPDRV_TASK_PATCH_CHIP:
        {
            vipdrv_subtask_info_t* subtask = &tsk_desc->subtasks[task_property->index];
            vip_uint32_t patch_index = task_property->param.patch_info.patch_index;
            if (patch_index < subtask->patch_cnt) {
                subtask->patch_info[patch_index].core_index = task_property->param.patch_info.core_index;
                subtask->patch_info[patch_index].core_count = task_property->param.patch_info.core_count;
                subtask->patch_info[patch_index].offset = task_property->param.patch_info.offset;
                subtask->patch_info[patch_index].size = task_property->param.patch_info.size;
                subtask->patch_info[patch_index].type = task_property->patch_type;
            }
        }
        break;

        case VIPDRV_TASK_PATCH_DUP:
        {
            vip_uint32_t dup_index = vipdrv_task_desc_id2index(task_property->param.dup_info.task_id);
            vipdrv_task_descriptor_t* tsk_desc_dup = VIP_NULL;
            if (VIP_SUCCESS == vipdrv_database_use(tskDesc_database, dup_index, (void**)&tsk_desc_dup)) {
                if ((VIPDRV_TASK_CMD_SUBTASK == task_property->param.dup_info.type) &&
                    (task_property->param.dup_info.index < tsk_desc_dup->subtask_count) &&
                    (tsk_desc_dup->subtasks[task_property->param.dup_info.index].patch_cnt ==
                        tsk_desc->subtasks[task_property->index].patch_cnt)) {
                    vipdrv_os_copy_memory(tsk_desc->subtasks[task_property->index].patch_info,
                        tsk_desc_dup->subtasks[task_property->param.dup_info.index].patch_info,
                        sizeof(vipdrv_subtask_patch_info_t) * tsk_desc->subtasks[task_property->index].patch_cnt);
                }
                vipdrv_database_unuse(tskDesc_database, dup_index);
            }
        }
        break;
        #endif

        default:
            PRINTK_E("task_id=0x%x, not implement this patch type=%d\n",
                task_property->task_id, task_property->patch_type);
            break;
        }

    }
    break;

    case VIPDRV_TASK_CMD_INIT:
    {
        tsk_desc->init_cmd.mem_id = task_property->param.subtask_info.subtask_id;
        tsk_desc->init_cmd.size = task_property->param.subtask_info.size;
    }
    break;

    case VIPDRV_TASK_CMD_PRELOAD:
    {
        tsk_desc->preload_cmd.mem_id = task_property->param.subtask_info.subtask_id;
        tsk_desc->preload_cmd.size = task_property->param.subtask_info.size;
    }
    break;

    default:
        PRINTK_E("task_id=0x%x, create subtask not implement this type=%d\n",
                 task_property->task_id, task_property->type);
        break;
    }
    vipdrv_database_unuse(tskDesc_database, index);

    return status;
onError:
    PRINTK_E("fail to set task desc index=%d property status=%d\n", index, status);
    return status;
}

static vip_status_e destroy_task(
    vip_uint32_t index
    )
{
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    vipdrv_database_remove(tskDesc_database, index, vip_false_e);
    PRINTK_D("destroy task index=0x%x\n", vipdrv_task_desc_index2id(index));
    return VIP_SUCCESS;
}

vip_status_e vipdrv_task_desc_destroy(
    vipdrv_destroy_task_param_t *param
    )
{
    vip_uint32_t index = vipdrv_task_desc_id2index(param->task_id);
    return destroy_task(index);
}

vip_status_e vipdrv_task_desc_force_destroy(
    vip_uint32_t pid
    )
{
    vip_uint32_t i = 0, j = 0;
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
#if vpmdTASK_QUEUE_USED
    vipdrv_hashmap_t* tskTCB_hashmap = (vipdrv_hashmap_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TCB);
#endif
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;

    for (i = 1; i < vipdrv_database_free_pos(tskDesc_database); i++) {
        if (VIP_SUCCESS == vipdrv_database_use(tskDesc_database, i, (void**)&tsk_desc)) {
            if (tsk_desc->pid == pid) {
                vip_bool_e support_cancel = vipdrv_context_support(VIP_HW_FEATURE_JOB_CANCEL);
                for (j = 0; j < tsk_desc->subtask_count; j++) {
                    vip_uint32_t task_id = j + vipdrv_task_desc_index2id(i);
                    vipdrv_task_control_block_t* tskTCB = VIP_NULL;

                    #if vpmdTASK_QUEUE_USED
                    vip_uint32_t index = 0;
                    if (VIP_SUCCESS != vipdrv_hashmap_get_by_handle(tskTCB_hashmap,
                        (vip_uint64_t)task_id, &index, (void**)&tskTCB)) {
                        continue;
                    }
                    #else
                    vip_bool_e valid = vip_false_e;
                    VIPDRV_LOOP_DEVICE_START
                    tskTCB = &device->tskTCB;
                    if (task_id == tskTCB->task.task_id) {
                        valid = vip_true_e;
                        break;
                    }
                    VIPDRV_LOOP_DEVICE_END
                    if (vip_false_e == valid) {
                        continue;
                    }
                    #endif

                    if (VIPDRV_TASK_INFER_START == tskTCB->status) {
                        /* still in inference */
                        if (vip_true_e == support_cancel) {
                            vip_uint8_t cancel_flag[vipdMAX_CORE] = { 0 };
                            vip_uint32_t core_index = 0;
                            for (core_index = 0; core_index < tskTCB->task.core_cnt; core_index++) {
                                cancel_flag[core_index + tskTCB->task.core_index] = 1;
                            }
                            if (VIP_SUCCESS != vipdrv_hw_cancel(tskTCB->task.device, cancel_flag)) {
                                PRINTK_E("dev%d cancel failed during destroy\n",
                                    tskTCB->task.device->device_index);
                            }
                        }
                        else {
                            vipdrv_task_t* task = &tskTCB->task;
                            VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
                            if (VIP_SUCCESS != vipdrv_hw_wait_idle(hw, 1000, vip_false_e,
                                EXPECT_IDLE_VALUE_ALL_MODULE)) {
                                PRINTK_E("hw%d not idle during destroy\n", hw->core_id);
                            }
                            VIPDRV_LOOP_HARDWARE_IN_TASK_END
                        }
                    }

                    #if vpmdTASK_QUEUE_USED
                    tskTCB->status = VIPDRV_TASK_EMPTY;
                    if (VIP_NULL != tskTCB->queue_data) {
                        tskTCB->queue_data->v2 = VIP_ERROR_CANCELED;
                    }
                    vipdrv_hashmap_remove_by_index(tskTCB_hashmap, index, vip_false_e);
                    vipdrv_hashmap_unuse(tskTCB_hashmap, index, vip_false_e);
                    #endif
                }

                destroy_task(i);
            }
            vipdrv_database_unuse(tskDesc_database, i);
        }
    }

    return VIP_SUCCESS;
}

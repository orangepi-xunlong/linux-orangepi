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
#ifndef _VIP_DRV_TASK_DESCRIPTOR_H
#define _VIP_DRV_TASK_DESCRIPTOR_H
#include <vip_drv_type.h>
#include <vip_drv_share.h>

#define TASK_MAGIC_DATA     0x80000000
#define TASK_INDEX_SHIFT    18

typedef struct _vipdrv_task_profile_bw
{
    vip_uint32_t            total_read;
    vip_uint32_t            total_write;
    vip_uint32_t            total_read_ocb;
    vip_uint32_t            total_write_ocb;
} vipdrv_task_profile_bw_t;

typedef struct _vipdrv_task_profile_cycle
{
    vip_uint32_t            total_cycle;
    vip_uint32_t            total_idle_cycle;
} vipdrv_task_profile_cycle_t;

typedef struct _vipdrv_subtask_cmd_info
{
    /* 'subtask_id'. the id of video memory for the subtask commands. */
    vip_uint32_t            mem_id;
    /* the real size of commands for subtask */
    vip_uint32_t            size;
} vipdrv_subtask_cmd_info_t;

typedef struct _vipdrv_subtask_patch_info
{
    vipdrv_task_patch_type_e type;
    /* the offset in command */
    vip_uint32_t             offset;
    /* the size of command will be patched */
    vip_uint32_t             size;
    /* the index of core in all used cores */
    vip_uint32_t             core_index;
    /* the core count of this patch uses */
    vip_uint32_t             core_count;
} vipdrv_subtask_patch_info_t;

typedef struct _vipdrv_subtask_info
{
    vipdrv_subtask_cmd_info_t     cmd;
    vip_uint32_t                  submit_count;

#if vpmdENABLE_TASK_PROFILE
#if vpmdENABLE_APP_PROFILING
    vipdrv_task_profile_cycle_t   cur_cycle[vipdMAX_CORE];
    vipdrv_task_profile_cycle_t   avg_cycle[vipdMAX_CORE];
#endif
#if vpmdENABLE_BW_PROFILING
    vipdrv_task_profile_bw_t      cur_bw[vipdMAX_CORE];
#endif
#endif

    /* the timeconsumed inference this subtask  */
    vip_uint64_t                  cur_time;
    vip_uint64_t                  avg_time;
#if vpmdTASK_SCHEDULE
    /* the index of core for the subtask last runs on */
    vip_uint32_t                  core_index;
    vip_uint32_t                  patch_cnt;
    vipdrv_subtask_patch_info_t*  patch_info;
#endif
} vipdrv_subtask_info_t;

typedef struct _vipdrv_task_descriptor
{
    vip_char_t                task_name[TASK_NAME_LENGTH];
    /* the command buffer of preload coeff to SRAM */
    vipdrv_subtask_cmd_info_t preload_cmd;
    vipdrv_subtask_cmd_info_t init_cmd;
    /* the maximal subtask count of this task desc */
    vip_uint32_t              subtask_count;
    vipdrv_subtask_info_t*    subtasks;
    vip_uint8_t               skip;
    /* The CPU process id which the task belongs */
    vip_uint32_t              pid;
} vipdrv_task_descriptor_t;

vip_status_e vipdrv_task_desc_init(void);

vip_status_e vipdrv_task_desc_uninit(void);

vip_status_e vipdrv_task_desc_submit(
    vipdrv_submit_task_param_t *sbmt_para
    );

vip_status_e vipdrv_task_desc_wait(
    vipdrv_wait_task_param_t *wait_para
    );

vip_uint32_t vipdrv_task_desc_index2id(
    vip_uint32_t index
    );

vip_uint32_t vipdrv_task_desc_id2index(
    vip_uint32_t task_id
    );

vip_status_e vipdrv_task_desc_create(
    vipdrv_create_task_param_t *tsk_param
    );

vip_status_e vipdrv_task_desc_destroy(
    vipdrv_destroy_task_param_t *param
    );

vip_status_e vipdrv_task_desc_set_property(
    vipdrv_set_task_property_t *task_property
    );

vip_status_e vipdrv_task_desc_force_destroy(
    vip_uint32_t pid
    );
#endif

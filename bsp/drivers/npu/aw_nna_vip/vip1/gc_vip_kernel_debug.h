/****************************************************************************
*
*    The MIT License (MIT)
*
*    Copyright (c) 2017 - 2023 Vivante Corporation
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
*    Copyright (C) 2017 - 2023 Vivante Corporation
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

#ifndef _GC_VIP_KERNEL_DEBUG_H
#define _GC_VIP_KERNEL_DEBUG_H

#include <gc_vip_common.h>
#include <vip_lite.h>
#include <gc_vip_kernel.h>

#if vpmdREGISTER_NETWORK
#define PREVIOUS_NET_COUNT 20
#define NUM_OF_AVG_TIME 100
typedef struct _gckvip_profiling_data
{
    vip_uint32_t            total_cycle;
    vip_uint32_t            total_idle_cycle;
    vip_uint64_t            time;
} gckvip_profiling_data_t;
typedef struct _gckvip_network_info
{
    vip_char_t              network_name[NETWORK_NAME_SIZE];
    vip_uint32_t            segment_count;
    void                    **segment_handle;
} gckvip_network_info_t;

typedef struct _gckvip_segment_info
{
    void                    *network_handle;
    vip_uint32_t            segment_index;
    vip_uint32_t            operation_start;
    vip_uint32_t            operation_end;
    vip_uint32_t            submit_count;
    vip_uint8_t             skip;
#if !vpmdENABLE_CNN_PROFILING
    gckvip_profiling_data_t cur_data;
    gckvip_profiling_data_t avg_data;
#endif
} gckvip_segment_info_t;
#endif

#if vpmdENABLE_HANG_DUMP
vip_status_e gckvip_dump_data(
    vip_uint32_t *data,
    vip_uint32_t physical,
    vip_uint32_t size
    );

vip_status_e gckvip_dump_states(
    gckvip_context_t *context,
    gckvip_device_t *device
    );

vip_status_e gckvip_dump_command(
    gckvip_context_t *context,
    gckvip_device_t *device
    );

vip_status_e gckvip_hang_dump(
    gckvip_device_t *device
    );

#if vpmdENABLE_WAIT_LINK_LOOP
vip_status_e gckvip_dump_waitlink(
    gckvip_device_t *device
    );
#endif
#endif

#if vpmdREGISTER_NETWORK && (!vpmdENABLE_CNN_PROFILING)
void gckvip_prepare_profiling(
    gckvip_submit_t *submit,
    gckvip_device_t *device
    );

vip_status_e gckvip_start_profiling(
    gckvip_submit_t *submit,
    gckvip_device_t *device
    );

vip_status_e gckvip_end_profiling(
    gckvip_context_t *context,
    gckvip_device_t *device,
    void *cmd_handle
    );

vip_status_e gckvip_query_profiling(
    gckvip_context_t *context,
    gckvip_device_t *device,
    void *mem_handle,
    gckvip_query_profiling_t *profiling
    );
#endif

#if (vpmdENABLE_DEBUG_LOG > 3) || vpmdENABLE_DEBUGFS
void gckvip_report_clock(void);

vip_status_e gckvip_dump_options(void);

vip_status_e gckvip_get_clock(
    gckvip_hardware_t *hardware,
    vip_uint64_t *clk_MC,
    vip_uint64_t *clk_SH
    );
#endif

vip_status_e gckvip_check_irq_value(
    gckvip_context_t *context,
    gckvip_device_t *device
    );

vip_status_e gckvip_register_dump_wrapper(
    vip_status_e(*func)(gckvip_hardware_t* hardware, ...),
    ...
    );

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
vip_status_e gckvip_register_dump_init(
    gckvip_context_t* context
    );

vip_status_e gckvip_register_dump_uninit(
    gckvip_context_t* context
    );

vip_status_e gckvip_register_dump_action(
    gckvip_register_dump_type_e type,
    vip_uint32_t core_id,
    vip_uint32_t address,
    vip_uint32_t expect,
    vip_uint32_t data
    );
#endif

#if vpmdREGISTER_NETWORK
vip_status_e gckvip_register_network_info(
    gckvip_register_network_t *reg
    );

vip_status_e gckvip_register_segment_info(
    gckvip_register_segment_t *reg
    );

vip_status_e gckvip_unregister_network_info(
    gckvip_unregister_t *unreg
    );

vip_status_e gckvip_unregister_segment_info(
    gckvip_unregister_t *unreg
    );

vip_bool_e gckvip_check_segment_skip(
    void *cmd_handle
    );

void gckvip_start_segment(
    gckvip_device_t *device,
    void *cmd_handle
    );

vip_status_e gckvip_dump_network_info(
    gckvip_device_t *device
    );
#endif
#endif

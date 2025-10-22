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

#ifndef _VIP_DRV_DEBUG_H
#define _VIP_DRV_DEBUG_H

#include <vip_drv_type.h>
#include <vip_drv_share.h>
#include <vip_drv_task_debug.h>


#if vpmdENABLE_HANG_DUMP
vip_status_e vipdrv_dump_data(
    vip_uint32_t *data,
    vip_uint32_t physical,
    vip_uint32_t size
    );

vip_status_e vipdrv_dump_states(
    vipdrv_task_t *task
    );

vip_status_e vipdrv_hang_dump(
    vipdrv_task_t *task
    );

#if vpmdTASK_PARALLEL_ASYNC
vip_status_e vipdrv_dump_waitlink_table(
    vipdrv_task_t *task
    );
#endif

vip_status_e vipdrv_dump_PC_value(
    vipdrv_hardware_t *hardware
    );
#endif

#if (vpmdENABLE_DEBUG_LOG > 3) || vpmdENABLE_DEBUGFS
void vipdrv_report_clock(void);

vip_status_e vipdrv_get_clock(
    vipdrv_hardware_t *hardware,
    vip_uint64_t *clk_MC,
    vip_uint64_t *clk_SH
    );
#endif

vip_status_e vipdrv_dump_options(void);

vip_status_e vipdrv_register_dump_wrapper(
    vip_status_e(*func)(vipdrv_hardware_t* hardware, ...),
    ...
    );

#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
vip_status_e vipdrv_register_dump_init(
    vipdrv_context_t* context
    );

vip_status_e vipdrv_register_dump_uninit(
    vipdrv_context_t* context
    );

vip_status_e vipdrv_register_dump_action(
    vipdrv_register_dump_type_e type,
    vipdrv_hardware_t* hardware,
    vip_uint32_t address,
    vip_uint32_t expect,
    vip_uint32_t data
    );
#endif

#endif

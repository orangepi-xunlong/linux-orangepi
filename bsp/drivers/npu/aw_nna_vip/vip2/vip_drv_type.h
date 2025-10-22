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

#ifndef _VIP_DRV_TYPE_H
#define _VIP_DRV_TYPE_H

#include <vip_lite_config.h>


/* vip-drv log zone defines */
#define VIPDRV_LOG_ZONE_NONE             (0 << 0)
#define VIPDRV_LOG_ZONE_TASK             (1 << 1)
#define VIPDRV_LOG_ZONE_POWER            (1 << 2)
#define VIPDRV_LOG_ZONE_VIDEO_MEMORY     (1 << 3)
#define VIPDRV_LOG_ZONE_MMU              (1 << 4)
#define VIPDRV_LOG_ZONE_OTHERS           (1 << 31)
#define VIPDRV_LOG_ZONE_ALL              0xffffffff


/* Change this define to control which modules log can be show out. default shows out all logs */
#define VIPDRV_LOG_ZONE_ENABLED          VIPDRV_LOG_ZONE_ALL


/* Default log zone module, don't change the define.
The VIPDRV_LOG_ZONE will be re-define in module source code */
#define VIPDRV_LOG_ZONE                  VIPDRV_LOG_ZONE_OTHERS

/*
    Declaration of data type in vip-drv
*/
typedef struct _vipdrv_video_mem_handle    vipdrv_video_mem_handle_t;
typedef struct _vipdrv_allocator           vipdrv_allocator_t;
typedef void*                              vipdrv_alloc_param_ptr;

typedef struct _vipdrv_video_memory        vipdrv_video_memory_t;
typedef struct _vipdrv_mmu_info            vipdrv_mmu_info_t;

typedef struct _vipdrv_device              vipdrv_device_t;
typedef struct _vipdrv_hardware            vipdrv_hardware_t;
typedef struct _vipdrv_context             vipdrv_context_t;

typedef struct _vipdrv_task                vipdrv_task_t;
typedef struct _vipdrv_task_control_block  vipdrv_task_control_block_t;

typedef struct _vip_hw_feature_db          vip_hw_feature_db_t;
#endif

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

#ifndef __VIP_DRV_HEAP_H__
#define __VIP_DRV_HEAP_H__

#include <vip_lite_config.h>

#if vpmdENABLE_VIDEO_MEMORY_HEAP
#include <vip_drv_type.h>
#include <vip_drv_os_port.h>
#include <vip_drv_mem_allocator.h>
/* allocate node struce for dynamic malloc */
#ifndef vpmdNODE_MEMORY_IN_HEAP
#define vpmdNODE_MEMORY_IN_HEAP              1
#endif

typedef struct _vipdrv_list_head {
    struct _vipdrv_list_head *next;
    struct _vipdrv_list_head *prev;
} vipdrv_list_head_t;

#define VIPDRV_INIT_LIST_HEAD(entry) \
        (entry)->next = (entry);\
        (entry)->prev = (entry);

typedef struct _vipdrv_heap_node {
    vipdrv_list_head_t list;
    vip_uint64_t    offset;
    vip_uint64_t    size;
    vip_uint32_t    status;
} vipdrv_heap_node_t;

/* inherit from vipdrv_alloc_mgt_t */
typedef struct _vipdrv_alloc_mgt_heap {
    vipdrv_alloc_mgt_t allocator_mgt;
    /* extra params in derived class */
    /* vip physical base address */
    vip_physical_t     physical;
    /* The virtual base address of VIP when MMU enabled */
    vip_virtual_t      virtual;
    /* video memory size */
    vip_uint64_t       total_size;
    /* the size of free memory in this heap */
    vip_uint64_t       free_bytes;
    vipdrv_list_head_t list;
    vipdrv_list_head_t idle_list;
    vip_uint32_t       node_capacity;
    vipdrv_heap_node_t *nodes;
    vip_bool_e         mmu_map;
#if vpmdNODE_MEMORY_IN_HEAP
    void               *node_handle;
#endif
#if vpmdENABLE_MULTIPLE_TASK
    vipdrv_mutex       mutex;
#endif

    /* a counter for heap node new and delete */
    vip_uint32_t       node_new_cnt;
    vip_uint32_t       node_del_cnt;
} vipdrv_alloc_mgt_heap_t;

vip_status_e vipdrv_alloc_mgt_heap_init(
    vipdrv_alloc_mgt_heap_t *heap,
    vipdrv_videomem_heap_info_t *heap_info
    );

vip_status_e vipdrv_heap_dump(
    vipdrv_alloc_mgt_heap_t *heap
    );

#endif
#endif

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

#ifndef _VIP_KERNEL_ALLOCATOR_H
#define _VIP_KERNEL_ALLOCATOR_H

#include <vip_drv_type.h>
#include <vip_drv_video_memory.h>
#define DEBUG_ALLOCATOR         0

/* Fill a vipdrv_allocator_desc_t structure. */
#define VIPDRV_DEFINE_ALLOCATOR_DESC(Name, Init, Type)    \
{                                                   \
    .name = Name,                                   \
    .init = Init,                                   \
    .allocator_type = Type,                         \
}

typedef enum _vipdrv_allocator_type
{
    VIPDRV_ALLOCATOR_TYPE_NONE               = 0x000,
    /* indicate this memory is allocated from video memory heap */
    VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP          = 0x001,
    /* indicate this memory is allocated from dynamic memory */
    VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC          = 0x002,
    /* indicate this memory is created from user logical */
    VIPDRV_ALLOCATOR_TYPE_WRAP_USER_LOGICAL  = 0x004,
    /* indicate this memory is created from user physical */
    VIPDRV_ALLOCATOR_TYPE_WRAP_USER_PHYSICAL = 0x008,
    /* dma-buf on Linux */
    VIPDRV_ALLOCATOR_TYPE_WRAP_DMA_BUF       = 0x010,
    /* dynamic allocator, add by customer. NOT implement yet */
    VIPDRV_ALLOCATOR_TYPE_CUSTOMER           = 0x020,
} vipdrv_allocator_type_e;


/* kinds of params of callback */
typedef struct _vipdrv_allocator_alloc_param
{
    vip_size_t    size;
    vip_uint8_t   specified;
    vip_uint8_t   virtual;
    vip_uint32_t  align;
    vip_uint32_t  mem_flag;
    vip_address_t address;
} vipdrv_allocator_alloc_param_t;

typedef struct _vipdrv_allocator_wrapphy_param
{
    vip_uint32_t   phy_num;
    vip_address_t *phy_table;
    vip_size_t    *size_table;
} vipdrv_allocator_wrapphy_param_t;

typedef struct _vipdrv_allocator_wrapmem_param
{
    vip_size_t     size;
    void           *user_logical;
} vipdrv_allocator_wrapmem_param_t;

typedef struct _vipdrv_allocator_wrapfd_param
{
    vip_size_t     size;
    vip_uint32_t   fd;
} vipdrv_allocator_wrapfd_param_t;

typedef struct _vipdrv_allocator_callback
{
    vip_status_e(*allocate)(vipdrv_video_mem_handle_t*, vipdrv_alloc_param_ptr);
    vip_status_e(*free)(vipdrv_video_mem_handle_t*);
    vip_status_e(*map_user_logical)(vipdrv_video_mem_handle_t*);
    vip_status_e(*unmap_user_logical)(vipdrv_video_mem_handle_t*);
    vip_status_e(*map_kernel_logical)(vipdrv_video_mem_handle_t*);
    vip_status_e(*unmap_kernel_logical)(vipdrv_video_mem_handle_t*);
    vip_status_e(*flush_cache)(vipdrv_video_mem_handle_t*, vip_uint8_t);
    vip_physical_t(*phy_vip2cpu)(vip_physical_t);
    vip_physical_t(*phy_cpu2vip)(vip_physical_t);
    /* destructor of allocator or allocator management */
    vip_status_e(*uninit)(vipdrv_allocator_t*);
} vipdrv_allocator_callback_t;

/* base class */
struct _vipdrv_allocator
{
    vipdrv_allocator_callback_t callback;
    vipdrv_allocator_type_e     alctor_type;
    vipdrv_mem_flag_e           mem_flag;
    const vip_char_t            *name;
    vip_uint64_t                device_vis;  /* indicate the visibility for each device */
};

/* inherit from vipdrv_allocator_t */
typedef struct _vipdrv_alloc_mgt
{
    vipdrv_allocator_t          allocator;
    struct _vipdrv_allocator    *next;
    /* extra params in derived class */
    vipdrv_allocator_callback_t base_callback;
} vipdrv_alloc_mgt_t;

/* inherit from vipdrv_alloc_mgt_t */
typedef struct _vipdrv_alloc_mgt_dyn
{
    vipdrv_alloc_mgt_t          allocator_mgt;
} vipdrv_alloc_mgt_dyn_t;

typedef struct _vipdrv_allocator_desc {
    /* name of a allocator. */
    vip_char_t* name;
    /* function to init a allocator. */
    vip_status_e(*init)(vipdrv_allocator_t* allocator, vip_char_t* name, vipdrv_allocator_type_e type);
    vipdrv_allocator_type_e allocator_type;
} vipdrv_allocator_desc_t;

vip_status_e vipdrv_memory_overlap_check(
    vip_address_t physical_a,
    vip_size_t size_a,
    vip_address_t physical_b,
    vip_size_t size_b
    );

vip_status_e vipdrv_allocator_init(void);

vip_status_e vipdrv_allocator_uninit(void);

vip_status_e vipdrv_allocator_get(
    vipdrv_allocator_type_e alctor_type,
    vipdrv_mem_flag_e mem_flag,
    vip_uint64_t device_vis,
    vipdrv_allocator_t** allocator
    );

vip_status_e vipdrv_allocator_dump(void);

#endif

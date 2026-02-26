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

#ifndef _VIP_DRV_MEM_ALLOCATOR_ARRAY_H
#define _VIP_DRV_MEM_ALLOCATOR_ARRAY_H

#include <vip_drv_type.h>
#include <vip_drv_mem_allocator.h>


#if vpmdENABLE_VIDEO_MEMORY_HEAP
extern vip_status_e allocator_init_heap_reserved(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
extern vip_status_e allocator_init_heap_exclusive(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
#endif
#if vpmdENABLE_MMU
extern vip_status_e allocator_init_dyn_segment_continue(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
extern vip_status_e allocator_init_dyn_gfp(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
#endif
extern vip_status_e allocator_init_dyn_continue(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
#if defined (USE_LINUX_CMA)
extern vip_status_e allocator_init_dyn_cma(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
#endif
#if defined (USE_LINUX_MEM_ION)
vip_status_e allocator_init_dyn_ion(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
#endif
extern vip_status_e allocator_init_wrap_user_physical(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
extern vip_status_e allocator_init_wrap_user_memory(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );
extern vip_status_e allocator_init_wrap_dma_buf(
    vipdrv_allocator_t* allocator,
    vip_char_t* name,
    vipdrv_allocator_type_e type
    );


static vipdrv_allocator_desc_t video_mem_allocator_array[] = {
#if vpmdENABLE_VIDEO_MEMORY_DYNAMIC
    #if vpmdENABLE_MMU
    VIPDRV_DEFINE_ALLOCATOR_DESC("dyn-gfp", allocator_init_dyn_gfp, VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC),
    VIPDRV_DEFINE_ALLOCATOR_DESC("dyn-64K-continue", allocator_init_dyn_segment_continue,
                                 VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC),
    VIPDRV_DEFINE_ALLOCATOR_DESC("dyn-1M-continue", allocator_init_dyn_segment_continue,
                                 VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC),
    VIPDRV_DEFINE_ALLOCATOR_DESC("dyn-16M-continue", allocator_init_dyn_segment_continue,
                                 VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC),
    #endif
    VIPDRV_DEFINE_ALLOCATOR_DESC("dyn-continue", allocator_init_dyn_continue,
                                 VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC),
    #if defined (USE_LINUX_CMA)
    VIPDRV_DEFINE_ALLOCATOR_DESC("dyn-cma", allocator_init_dyn_cma, VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC),
    #endif
    #if defined (USE_LINUX_MEM_ION)
    VIPDRV_DEFINE_ALLOCATOR_DESC("dyn-ion", allocator_init_dyn_ion, VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC),
    #endif
#endif

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    VIPDRV_DEFINE_ALLOCATOR_DESC("heap-reserved", allocator_init_heap_reserved,
                                 VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP),
    VIPDRV_DEFINE_ALLOCATOR_DESC("heap-exclusive", allocator_init_heap_exclusive,
                                 VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP),
#endif
    VIPDRV_DEFINE_ALLOCATOR_DESC("wrap-user-phy", allocator_init_wrap_user_physical,
                                 VIPDRV_ALLOCATOR_TYPE_WRAP_USER_PHYSICAL),
    VIPDRV_DEFINE_ALLOCATOR_DESC("wrap-user-mem", allocator_init_wrap_user_memory,
                                 VIPDRV_ALLOCATOR_TYPE_WRAP_USER_LOGICAL),
    VIPDRV_DEFINE_ALLOCATOR_DESC("wrap-dma-buf", allocator_init_wrap_dma_buf,
                                 VIPDRV_ALLOCATOR_TYPE_WRAP_DMA_BUF),
};
#endif

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

#ifndef _VIP_KERNEL_ALLOCATOR_H
#define _VIP_KERNEL_ALLOCATOR_H

#include <linux/dma-buf.h>
#include <gc_vip_common.h>
#include <gc_vip_kernel_port.h>
#include <gc_vip_video_memory.h>

typedef struct _gckvip_dma_buf_info{
    struct dma_buf *dma_buf;
    struct dma_buf_attachment *dma_attachment;
    struct sg_table *sgt;
} gckvip_dma_buf_info_t;

#if vpmdENABLE_MMU
/*
@brief get user space logical address.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
@param, virtual_addr, the VIP's virtual address.
@param, logical, use space logical address.
@param, video memory type see gckvip_video_memory_type_e
*/
vip_status_e gckvip_allocator_get_userlogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t virtual_addr,
    void **logical,
    vip_uint8_t memory_type
    );

/*
@brief get kernel space logical address.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
@param, virtual_addr, the VIP's virtual address.
@param, logical, kernel space logical address.
@param, video memory type see gckvip_video_memory_type_e
*/
vip_status_e gckvip_allocator_get_kernellogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t virtual_addr,
    void **logical,
    vip_uint8_t memory_type
    );

/*
@brief get VIP physical address.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
@param IN vip_virtual, the vitual address of VIP.
@param, physical, VIP physical address.
@param, video memory type see gckvip_video_memory_type_e
*/
vip_status_e gckvip_allocator_get_vipphysical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t vip_virtual,
    phy_address_t *physical,
    vip_uint8_t memory_type
    );
#endif

/*
@brief convert user's memory to CPU physical. And map to VIP virtual address.
@param, context, the contect of kernel space.
@param, logical, the logical address(handle) should be wraped.
@param, memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_wrap_usermemory(
    gckvip_context_t *context,
    vip_ptr logical,
    vip_uint32_t memory_type,
    gckvip_dyn_allocate_node_t *node
    );

/*
@brief un-wrap user memory(handle).
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_unwrap_usermemory(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    );

/*
@brief convert cpu's physical to vip physical.
@param IN context, the contect of kernel space.
@param IN physical_table Physical address table. should be wraped for VIP hardware.
@param IN size_table The size of physical memory for each physical_table element.
@param IN physical_num The number of physical table element.
@param, node, dynamic allocate node info.
@param IN alloc_flag the flag of this video memroy. see gckvip_video_mem_alloc_flag_e.
*/
vip_status_e gckvip_allocator_wrap_userphysical(
    gckvip_context_t *context,
    vip_address_t *physical_table,
    vip_uint32_t *size_table,
    vip_uint32_t physical_num,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t alloc_flag
    );

/*
@brief un-wrap user physical.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_unwrap_userphysical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    );

#if vpmdENABLE_CREATE_BUF_FD
/*
@brief convert user's memory to CPU physical. And map to VIP virtual address.
@param, context, the contect of kernel space.
@param, fd, the user memory file descriptor.
@param, memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_wrap_userfd(
    gckvip_context_t *context,
    vip_uint32_t fd,
    vip_uint32_t memory_type,
    gckvip_dyn_allocate_node_t *node
    );

/*@brief un-wrap user fd(file descriptor).
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_unwrap_userfd(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    );
#endif

/*
@brief Flush CPU cache for dynamic alloc video memory and wrap user memory.
@param context, the contect of kernel space.
@param node, dynamic allocate node info.
@param physical, the physical address should be flush CPU cache.
@param logical, the logical address should be flush.
@param size, the size of the memory should be flush.
@param type The type of operate cache.
*/
#if vpmdENABLE_FLUSH_CPU_CACHE
vip_status_e gckvip_alloctor_flush_cache(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    phy_address_t physical,
    void *logical,
    vip_uint32_t size,
    gckvip_cache_type_e type
    );
#endif

/*
@brief mapping user space logical address.
@param, context, the contect of kernel space.
@param, node, dynamic allocate node info.
@param, logical, use space logical address mapped.
*/
vip_status_e gckvip_allocator_map_userlogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    void **logical
    );

vip_status_e gckvip_allocator_unmap_userlogical(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    );

/*
@brief allocate memory from system. get the physical address table each page. get size table of eache page
@param context, the contect of kernel space.
@param node, dynamic allocate noe.
@param align, the size of alignment for this video memory.
@param flag the flag of this video memroy. see gckvip_video_mem_alloc_flag_e.
*/
vip_status_e gckvip_allocator_dyn_alloc(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    );

/*
@brief free a dynamic allocate memory.
@param context, the contect of kernel space.
@param node, dynamic allocate node info.
*/
vip_status_e gckvip_allocator_dyn_free(
    gckvip_context_t *context,
    gckvip_dyn_allocate_node_t *node
    );

#endif

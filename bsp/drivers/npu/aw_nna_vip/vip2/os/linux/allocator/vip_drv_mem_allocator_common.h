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

#ifndef _VIP_DRV_MEM_ALLOCATOR_COMMON_H
#define _VIP_DRV_MEM_ALLOCATOR_COMMON_H

#include <linux/dma-buf.h>
#include <vip_lite_config.h>
#include <vip_drv_os_port.h>
#include <vip_drv_video_memory.h>


#define USE_MEM_WRITE_COMOBINE      1

#if defined(USE_LINUX_PCIE_DEVICE) || defined(USE_LINUX_PLATFORM_DEVICE)
#define FLUSH_CACHE_HOOK            1
#else
#define FLUSH_CACHE_HOOK            0
#endif

typedef struct _vipdrv_dma_buf_info{
    struct dma_buf *dma_buf;
    struct dma_buf_attachment *dma_attachment;
    struct sg_table *sgt;
} vipdrv_dma_buf_info_t;


vip_status_e flush_cache_init_pages(
    void **flush_handle,
    struct page **pages,
    vip_uint32_t page_count,
    vip_uint32_t offset,
    vip_size_t size
    );

vip_status_e flush_cache_destroy_pages(
    void *flush_handle
    );

vip_status_e flush_cache_pages(
    void* flush_handle,
    vip_uint8_t type
    );

vip_status_e flush_cache_init_continue(
    void **flush_handle,
    struct page *contiguous_pages,
    vip_uint32_t page_count,
    vip_uint32_t offset
    );

vip_status_e flush_cache_destroy_continue(
    void* flush_handle,
    vip_uint32_t page_count
    );

vip_status_e flush_cache_continue(
    void* flush_handle,
    vip_size_t size,
    vip_uint8_t type
    );

vip_status_e vipdrv_map_kernel_page(
    size_t page_count,
    struct page **pages,
    void **logical,
    vip_bool_e no_cache
    );

vip_status_e vipdrv_unmap_kernel_page(
    void *logical
    );

vip_status_e vipdrv_map_user(
    size_t page_count,
    struct page **pages,
    void **logical,
    vip_bool_e no_cache
    );

vip_status_e vipdrv_unmap_user(
    size_t page_count,
    void *logical
    );

vip_status_e vipdrv_map_user_dma_map(
    vip_uint32_t page_count,
    vip_physical_t cpu_physical,
    vip_uint8_t* kerl_logical,
    vip_size_t size,
    void **logical
    );

vip_status_e vipdrv_map_user_dma_buf(
    vip_uint32_t page_count,
    struct dma_buf *dma_buffer,
    unsigned long pgoff,
    void **logical
    );

vip_status_e vipdrv_import_pfn_map(
    vipdrv_video_mem_handle_t *handle,
    unsigned long long memory,
    vip_uint32_t page_count,
    vip_size_t size,
    vip_uint32_t offset
    );

vip_status_e vipdrv_import_page_map(
    vipdrv_video_mem_handle_t *handle,
    unsigned long long memory,
    vip_uint32_t page_count,
    vip_size_t size,
    vip_uint32_t offset,
    unsigned long flags
    );

vip_status_e vipdrv_fill_pages(
    vipdrv_video_mem_handle_t *ptr,
    struct page **pages
    );

vip_status_e vipdrv_get_page_count(
    vipdrv_video_mem_handle_t *ptr,
    vip_uint32_t* page_count
    );
#endif

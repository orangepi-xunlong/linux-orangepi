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

#ifndef _GC_VIP_VIDEO_MEMORY_H
#define _GC_VIP_VIDEO_MEMORY_H

#include <gc_vip_kernel.h>
#include <gc_vip_kernel_mmu.h>

typedef enum _gckvip_mem_flag
{
    GCVIP_MEM_FLAG_NONE               = 0x0000,
    /* Physical contiguous. */
    GCVIP_MEM_FLAG_CONTIGUOUS         = 0x0001,
    /* Physical non contiguous. */
    GCVIP_MEM_FLAG_NON_CONTIGUOUS     = 0x0002,
    /* Need 32bit address. */
    GCVIP_MEM_FLAG_4GB_ADDR           = 0x0004,
    /* without mmaped VIP's MMU page */
    GCVIP_MEM_FLAG_NO_MMU_PAGE        = 0x0008,
    /* user memory PFN map */
    GCVIP_MEM_FLAG_USER_MEMORY_PFN    = 0x0010,
    /* user memory page map */
    GCVIP_MEM_FLAG_USER_MEMORY_PAGE   = 0x0020,
    /* need map user space logical address */
    GCVIP_MEM_FLAG_MAP_USER           = 0x0040,
    /* allocate physical memory 1M byte contiguous */
    GCVIP_MEM_FLAG_1M_CONTIGUOUS      = 0x0080,
    /* allocate memory for CMA */
    GCVIP_MEM_FLAG_CMA                = 0x00100,
    GCVIP_MEM_FLAG_NONE_CACHE         = 0x00200,
    GCVIP_MEM_FLAG_USER_MEMORY_FD     = 0x00400,
    GCVIP_MEM_FLAG_MAP_KERNEL_LOGICAL = 0x00800,
    GCVIP_MEM_FLAG_USER_MEMORY_PHYSICAL = 0x001000,
    GCVIP_MEM_FLAG_MAX,
} gckvip_mem_flag_e;

typedef enum _gckvip_video_memory_type
{
    GCVIP_VIDEO_MEMORY_TYPE_NONE               = 0x000,
    /* indicate this memory is allocated from video memory heap */
    GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP          = 0x001,
    /* indicate this memory is allocated from dynamic memory */
    GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC          = 0x002,
    /* indicate this memory is created from user logical */
    GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_LOGICAL  = 0x004,
    /* indicate this memory is created from user physical */
    GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL = 0x008,

    /* indicate this memory is normal memory */
    GCVIP_VIDEO_MEMORY_TYPE_NORMAL             = 0x010,
    /* indicate this memory is secure memory */
    GCVIP_VIDEO_MEMORY_TYPE_SECURE             = 0x020,

    /* dma-buf on Linux */
    GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF       = 0x040,

    GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER = GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_LOGICAL |
                                        GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL |
                                        GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF,
} gckvip_video_memory_type_e;

typedef struct _gckvip_dyn_allocate_node
{
    /* a handle of allocate */
    void *alloc_handle;

    /* VIP MMU page table information */
#if vpmdENABLE_MMU
    gckvip_mmu_page_info_t mmu_page_info;
#endif
    /***allocator output physical_table, physical_num, size_table in gc_vip_kernel_allocator.c****/
    /* the vip phyiscal address of each page(segment) */
    phy_address_t *physical_table;
    /* the number of physical_table element */
    vip_uint32_t physical_num;
    /* the size of each page table element */
    vip_uint32_t *size_table;

    /* the number of page, May Linux only? */
    vip_uint32_t num_pages;
    /* user space logical address. Linux only? */
    vip_uint8_t *user_logical;
    /* kernel space logical address. Linux only?
      shoule be equal to user_logical and physical in RTOS?
    */
    vip_uint8_t *kerl_logical;
    /* VIP's virtual address */
    vip_uint32_t vip_virtual_address;

    /*  the total size of this video memory */
    vip_uint32_t size;

    /*  a handle for flush CPU's cache */
    void *flush_cache_handle;

    /* video memory flag, see gckvip_mem_flag_e */
    vip_uint32_t mem_flag;
    vip_uint8_t *is_exact_page;
    /* the flag of allocation see gckvip_video_mem_alloc_flag_e */
    vip_uint32_t alloc_flag;
} gckvip_dyn_allocate_node_t;

#if vpmdENABLE_VIDEO_MEMORY_HEAP
typedef struct _gckvip_dyn_heap_node
{
    /* a handle of allocate */
    gckvip_heap_node_t *alloc_handle;
    /* video memory flag, see gckvip_mem_flag_e */
    vip_uint32_t mem_flag;
    /* the actually allocated size */
    vip_uint32_t size;
    /* the allocated physical addr from heap node */
    phy_address_t physical;
} gckvip_dyn_heap_node_t;
#endif

typedef struct _gckvip_video_mem_handle
{
    /* gckvip_video_memory_type_e */
    vip_uint8_t memory_type;

    union {
#if vpmdENABLE_VIDEO_MEMORY_HEAP
        gckvip_dyn_heap_node_t heap_node;
#endif
        gckvip_dyn_allocate_node_t dyn_node;
    } node;

} gckvip_video_mem_handle_t;

/************** function defines *****************/
vip_status_e gckvip_mem_heap_init(
    gckvip_context_t *context
    );

vip_status_e gckvip_mem_heap_destroy(
    gckvip_context_t *context
    );

vip_status_e gckvip_mem_get_kernellogical(
    gckvip_context_t *context,
    void *handle,
    vip_uint32_t physical,
    vip_bool_e vip_virtual,
    void **logical
    );

vip_status_e gckvip_mem_get_userlogical(
    gckvip_context_t *context,
    void *handle,
    phy_address_t physical,
    vip_bool_e vip_virtual,
    void **logical
    );

vip_status_e gckvip_mem_get_vipvirtual(
    gckvip_context_t *context,
    void *handle,
    phy_address_t physical,
    phy_address_t *virtual_addr
    );

/*
@brief Map user space logical address.
@param IN context, the contect of kernel space.
@param IN handle, the handle of video memory.
@param IN physical, the physical address or VIP's virtual of video.
@param IN vip_virtual, indicate the physical is VIP's virtual or not.
@param OUT logical, user space logical address mapped.
*/
vip_status_e gckvip_mem_map_userlogical(
    gckvip_context_t *context,
    void *handle,
    vip_uint64_t physical,
    vip_bool_e vip_virtual,
    void** logical
    );

/*
@brief Map user space logical address.
@param IN context, the contect of kernel space.
@param IN handle, the handle of video memory.
*/
vip_status_e gckvip_mem_unmap_userlogical(
    gckvip_context_t *context,
    void *handle
    );

/*
@brief allocate a video memory from heap or system(MMU enabled).
@param context, the contect of kernel space.
@param size, the size of video memory be allocated.
@param handle, the handle of video memory.
@param memory, kernel space logical address.
@param physical,
    disable MMU, physical is CPU's physical address.
    enable MMU, physical is VIP's virtual address.
@param align the size of video memory alignment.
@flag the flag of this video memroy. see gckvip_video_mem_alloc_flag_e.
*/
vip_status_e gckvip_mem_allocate_videomemory(
    gckvip_context_t *context,
    vip_uint32_t size,
    void **handle,
    void **memory,
    phy_address_t *physical,
    vip_uint32_t align,
    vip_uint32_t alloc_flag
    );

vip_status_e gckvip_mem_free_videomemory(
    gckvip_context_t *context,
    void *handle
    );


#if vpmdENABLE_FLUSH_CPU_CACHE
/*
@brief flush video memory CPU cache
@param handle the video memory handle
@param physical flush memory start physical address. The physical is VIP physical address.
@param logical flush memory start logical address.
@param size the size of memory be flushed.
@param type The type of operate cache. see gckvip_cache_type_e.
*/
vip_status_e gckvip_mem_flush_cache_ext(
    void *handle,
    phy_address_t physical,
    void *logical,
    vip_uint32_t size,
    gckvip_cache_type_e type
    );
/*
@brief flush video memory CPU cache
@param handle the video memory handle
@param physical flush memory start physical address. The physical is VIP virtual address.
@param logical flush memory start logical addrss. kernel logical or user logical.
@param size the size of memory be flushed.
@param type The type of operate cache. see gckvip_cache_type_e.
*/
vip_status_e gckvip_mem_flush_cache(
    void *handle,
    vip_uint32_t physical,
    void *logical,
    vip_uint32_t size,
    gckvip_cache_type_e type
    );
#endif

/*
@brief wrap user memory to VIP virtual address.
@param IN context, the contect of kernel space.
@param IN physical_table Physical address table. should be wraped for VIP hardware.
@param IN size_table The size of physical memory for each physical_table element.
@param IN physical_num The number of physical table element.
@param IN alloc_flag the flag of this video memroy. see gckvip_video_mem_alloc_flag_e.
@param IN memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param OUT virtual_addr, VIP virtual address.
@param OUT handle, the handle of video memory.
*/
vip_status_e gckvip_mem_wrap_userphysical(
    gckvip_context_t *context,
    vip_address_t *physical_table,
    vip_uint32_t *size_table,
    vip_uint32_t physical_num,
    vip_uint32_t alloc_flag,
    vip_uint32_t memory_type,
    vip_uint32_t *virtual_addr,
    void **logical,
    void **handle
    );

/*
@brief un-wrap user memory to VIP virtual address.
@param context, the contect of kernel space.
@param handle, the handle of video memory.
@param virtual_addr, VIP virtual address.
*/
vip_status_e gckvip_mem_unwrap_userphysical(
    gckvip_context_t *context,
    void *handle,
    vip_uint32_t virtual_addr
    );

#if vpmdENABLE_MMU
/*
@brief Get VIP accessible physical address corresponding to the logical address.
@param context, the contect of kernel space.
@param handle, the handle of video memory.
@param IN vip_virtual, the vitual address of VIP.
@param OUT physical, the CPU's physical address corresponding to logical.
*/
vip_status_e gckvip_mem_get_vipphysical(
    gckvip_context_t *context,
    void *handle,
    vip_uint32_t vip_virtual,
    phy_address_t *physical
    );
#endif

/*
@brief wrap user memory to VIP virtual address.
@param IN context, the contect of kernel space.
@param IN logical, the user space logical address(handle).
@param IN size, the size of the memory.
@param IN memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param OUT virtual_addr, VIP virtual address.
@param OUT handle, the handle of video memory.
*/
vip_status_e gckvip_mem_wrap_usermemory(
    gckvip_context_t *context,
    vip_ptr logical,
    vip_uint32_t size,
    vip_uint32_t memory_type,
    vip_uint32_t *virtual_addr,
    void **handle
    );

vip_status_e gckvip_mem_unwrap_usermemory(
    gckvip_context_t *context,
    void *handle,
    vip_uint32_t virtual_addr
    );

#if vpmdENABLE_CREATE_BUF_FD
vip_status_e gckvip_mem_wrap_userfd(
    gckvip_context_t *context,
    vip_uint32_t fd,
    vip_uint32_t size,
    vip_uint32_t memory_type,
    vip_uint32_t *virtual_addr,
    vip_uint8_t **logical,
    void **handle
    );

vip_status_e gckvip_mem_unwrap_userfd(
    gckvip_context_t *context,
    void *handle,
    vip_uint32_t virtual_addr
    );
#endif

#endif

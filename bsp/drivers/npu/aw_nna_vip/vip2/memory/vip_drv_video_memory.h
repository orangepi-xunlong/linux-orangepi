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

#ifndef _VIP_DRV_VIDEO_MEMORY_H_
#define _VIP_DRV_VIDEO_MEMORY_H_

#include <vip_drv_type.h>
#include <vip_drv_share.h>

typedef struct _vipdrv_mem_control_block
{
    vipdrv_video_mem_handle_t* handle;
    vip_uint32_t               process_id;
    vip_uint32_t               task_id;
} vipdrv_mem_control_block_t;

typedef enum _vipdrv_mem_flag
{
    VIPDRV_MEM_FLAG_NONE                 = 0x0000,
    VIPDRV_MEM_FLAG_64K_CONTIGUOUS       = 0x0001, /* physical memory 64K byte contiguous */
    VIPDRV_MEM_FLAG_1M_CONTIGUOUS        = 0x0003, /* physical memory 1M byte contiguous */
    VIPDRV_MEM_FLAG_16M_CONTIGUOUS       = 0x0007, /* physical memory 16M byte contiguous */
    VIPDRV_MEM_FLAG_CONTIGUOUS           = 0x000f, /* physical memory contiguous. */
    VIPDRV_MEM_FLAG_4GB_ADDR_PA          = 0x0010, /* 32bit address. limit physical */
    /* following flag is extra action instead of property of memory */
    VIPDRV_MEM_FLAG_4GB_ADDR_VA          = 0x0020, /* 32bit address. limit virtual */
    VIPDRV_MEM_FLAG_NO_MMU_PAGE          = 0x0040, /* without mmaped VIP's MMU page */
    VIPDRV_MEM_FLAG_MMU_PAGE_WRITEABLE   = 0x0080, /* MMU page is mapped and writeable */
    VIPDRV_MEM_FLAG_MAP_USER_LOGICAL     = 0x0100, /* mapped user space logical address */
    VIPDRV_MEM_FLAG_MAP_KERNEL_LOGICAL   = 0x0200, /* mapped kernel space logical address */
    VIPDRV_MEM_FLAG_NONE_CACHE           = 0x0400, /* allocate none-cache memory */
    VIPDRV_MEM_FLAG_MMU_PAGE_MEM         = 0x0800, /* memory for mmu page */
    /* following flag is for wrapping memory from user */
    VIPDRV_MEM_FLAG_USER_MEMORY_PFN      = 0x1000, /* user memory PFN map */
    VIPDRV_MEM_FLAG_USER_MEMORY_PAGE     = 0x2000, /* user memory page map */
} vipdrv_mem_flag_e;

struct _vipdrv_video_memory
{
    vip_uint32_t     mem_id;
    vip_address_t    physical;
    /* the kernel space cpu logical address */
    void            *logical;
    /* the size of this memory */
    vip_uint32_t    size;
};

struct _vipdrv_video_mem_handle
{
    /* vipdrv_allocator_type_e */
    vip_uint8_t allocator_type;
    /* video memory flag, see vipdrv_mem_flag_e */
    vip_uint32_t mem_flag;
    /* the total size of this video memory */
    vip_size_t size;
    /* the size of align of this video memory */
    vip_uint32_t align;
    /* which process uses current memory */
    vip_uint32_t pid;
    /* user space logical address. Linux only, shoule be equal to kernel_logical and physical in RTOS?.
       fill user_logical via map_user_logical() callback in allocator */
    vip_uint8_t* user_logical;
    /* kernel space logical address. Linux only, shoule be equal to user_logical and physical in RTOS?
       fill kerl_logical via map_kernel_logical() callback in allocator */
    vip_uint8_t* kerl_logical;
    /* VIP's virtual address if MMU is enabled
       VIP's physical address if MMU is disabled
    */
    vip_address_t vip_address;
    /* a handle for flush CPU cache, used the handle to flush CPU cache */
    void* flush_handle;
    /* a handle of allocate, create allocate() callback in allocator if need */
    void* alloc_handle;

    /***allocators fill physical_table, physical_num, size_table in gc_vip_kernel_allocator_xxx.c****/
    /* the number of physical_table element */
    vip_uint32_t physical_num;
    /* the VIP's phyiscal address of each page(segment) */
    vip_physical_t* physical_table;
    /* the size of each page table element */
    vip_size_t* size_table;
    /* which allocator used for this memory alloc */
    vipdrv_allocator_t* allocator;
    /* VIP MMU page table handle, created in map VIP's MMU */
#if vpmdENABLE_MMU
    void* mmu_handle;
#endif
};


/************** function defines *****************/
vip_status_e vipdrv_mem_map_userlogical(
    vip_uint32_t mem_id,
    void **logical
    );

/*
@brief Map user space logical address.
@param IN mem_id, the id of video memory.
*/
vip_status_e vipdrv_mem_unmap_userlogical(
    vip_uint32_t mem_id
    );

/*
@brief allocate a video memory from heap(reserved memory) or system(MMU enabled).
@param size, the size of video memory be allocated.
@param mem_id, the id of video memory. The mem_id is a unique identifier of the video memory.
             Use the id to manage the video memory, map logical, fluch cache, free and so on.
@param logical, The CPU's logical address.
       1. user space logical address.
          a. alloc_flag with VIPDRV_VIDEO_MEM_ALLOC_MAP_USER
          b. alloc_flag with VIPDRV_VIDEO_MEM_ALLOC_MAP_USER & VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL
       2. kernel space logical address.
          a. alloc_flag with VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL
@param physical, The VIP's address.
       1. physical is VIP's physical address.
          a. MMU is disabled.
          b. alloc_flag with VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE.
       2. physical is VIP's virtual address.
          a. MMU is enabled
@param alloc_flag the flag of this video memroy. see vipdrv_video_mem_alloc_flag_e.
@param align, the size of video memory alignment.
@param mmu_id, the id of mmu page.
@param device_vis, a bitmap to indicate which device could access this memory.
@param specified, specify the physical address.
*/
vip_status_e vipdrv_mem_allocate_videomemory(
    vip_size_t size,
    vip_uint32_t *mem_id,
    void **logical,
    vip_physical_t *physical,
    vip_uint32_t align,
    vip_uint32_t alloc_flag,
    vip_uint64_t mmu_id,
    vip_uint64_t device_vis,
    vip_uint8_t specified
    );

/*
@brief free video memory.
@mem_id the id of video memory.
*/
vip_status_e vipdrv_mem_free_videomemory(
    vip_uint32_t mem_id
    );

/*
force free all video memory allocated by process_id
*/
vip_status_e vipdrv_mem_force_free_videomemory(
    vip_uint32_t process_id
    );

#if vpmdENABLE_FLUSH_CPU_CACHE
/*
@brief flush video memory CPU cache
@param mem_id the id of video memory
@param type The type of operate cache. see vipdrv_cache_type_e.
*/
vip_status_e vipdrv_mem_flush_cache(
    vip_uint32_t mem_id,
    vipdrv_cache_type_e type
    );
#endif

/*
@brief wrap user memory to VIP virtual address.
@param IN physical_table Physical address table. should be wraped for VIP hardware.
@param IN size_table The size of physical memory for each physical_table element.
@param IN physical_num The number of physical table element.
@param IN alloc_flag the flag of this video memroy. see vipdrv_video_mem_alloc_flag_e.
@param IN memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param IN device_vis A bitmap to indicate which device could access this memory.
@param OUT virtual_addr, VIP virtual address.
@param mem_id, the id of video memory. The mem_id is a unique identifier of the video memory.
               Use the id to manage the video memory, map logical, fluch cache, free and so on.
*/
vip_status_e vipdrv_mem_wrap_userphysical(
    vip_address_t *physical_table,
    vip_size_t *size_table,
    vip_uint32_t physical_num,
    vip_uint32_t alloc_flag,
    vip_uint32_t memory_type,
    vip_uint64_t device_vis,
    vip_address_t *virtual_addr,
    void **logical,
    vip_uint32_t *mem_id
    );

/*
@brief wrap user memory to VIP virtual address.
@param IN logical, the user space logical address(handle).
@param IN size, the size of the memory.
@param IN memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param IN device_vis A bitmap to indicate which device could access this memory.
@param OUT virtual_addr, VIP virtual address.
@param mem_id, the id of video memory. The mem_id is a unique identifier of the video memory.
             Use the id to manage the video memory, map logical, fluch cache, free and so on.
*/
vip_status_e vipdrv_mem_wrap_usermemory(
    vip_ptr logical,
    vip_size_t size,
    vip_uint32_t memory_type,
    vip_uint64_t device_vis,
    vip_address_t *virtual_addr,
    vip_uint32_t *mem_id
    );

#if vpmdENABLE_CREATE_BUF_FD
/*
@brief un-wrap user memory from fd to VIP virtual address.
@param fd the user memory file descriptor.
@param size, the size of of user memory should be unwraped.
@param memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
       only support DMA_BUF now.
@param IN device_vis A bitmap to indicate which device could access this memory.
@param virtual_addr, VIP virtual address.
@param mem_id, the id of video memory.
*/
vip_status_e vipdrv_mem_wrap_userfd(
    vip_uint32_t fd,
    vip_size_t size,
    vip_uint32_t memory_type,
    vip_uint64_t device_vis,
    vip_address_t *virtual_addr,
    vip_uint8_t **logical,
    vip_uint32_t *mem_id
    );
#endif

vip_status_e vipdrv_video_memory_init(void);

vip_status_e vipdrv_video_memory_uninit(void);

/*
input mem_id to get memory cpu kernel logical, vip physical and size
*/
vip_status_e vipdrv_mem_get_info(
    vipdrv_context_t* context,
    vip_uint32_t mem_id,
    vip_uint8_t** logcial,
    vip_address_t* physical,
    vip_size_t* size,
    vip_uint32_t* pid
    );
#endif

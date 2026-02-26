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
#include <vip_drv_video_memory.h>
#include <vip_drv_os_port.h>
#include <vip_drv_mmu.h>
#include <vip_drv_mem_heap.h>
#include <vip_drv_debug.h>
#include <vip_drv_context.h>
#include <vip_drv_mem_allocator.h>
#include <vip_drv_device_driver.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_VIDEO_MEMORY


#define MEMORY_MAGIC_DATA          0x15000000

#define INIT_MEM_DATABASE_NUM            512


#define CHECK_VIDEO_MEM_ID_VALID(mem_id)            \
if (mem_id <= MEMORY_MAGIC_DATA) {                  \
    VIPDRV_DUMP_STACK();                            \
    PRINTK_E("fail invalid mem id=0x%x\n", mem_id); \
    return VIP_ERROR_FAILURE;                       \
}

#define ENSURE_MEM_DATABASE_CAPACITY() \
    if (vip_true_e == vipdrv_database_full(&context->mem_database)) { \
        vip_bool_e expand = vip_false_e; \
        status = vipdrv_database_expand(&context->mem_database, &expand); \
        if (VIP_SUCCESS != status) { \
            PRINTK_E("failed, wrap handle overflow\n"); \
            vipGoOnError(VIP_ERROR_IO); \
        } \
    }

#if vpmdENABLE_MMU
#define VIDEO_MEMORY_PHYSICAL_CHECK(physical) \
if ((physical) & 0xffffff0000000000) { \
    PRINTK_E("fail mmu enabled video memory physical=0x%"PRIx64"\n", physical); \
    vipGoOnError(VIP_ERROR_OUT_OF_MEMORY); \
}
#else
#define VIDEO_MEMORY_PHYSICAL_CHECK(physical) \
if ((physical) >= 0xffffffff) { \
    PRINTK_E("fail mmu disabled video memory physical=0x%"PRIx64"\n", physical); \
    vipGoOnError(VIP_ERROR_OUT_OF_MEMORY); \
}
#endif

#if (vpmdENABLE_DEBUG_LOG >= 4)
#define VIDEO_MEMORY_ALLOCATOR_TRANSFER_LOG(last_allocator, allocator)      \
else {                                                                      \
    if (last_allocator != VIP_NULL) {                                       \
        PRINTK_D("video memory allocator transfer %s -> %s\n",              \
                  last_allocator->name, allocator->name);                   \
    }                                                                       \
    else {                                                                  \
        PRINTK_D("video memory allocator transfer %s\n", allocator->name);  \
    }                                                                       \
    last_allocator = allocator;                                             \
}
#else
#define VIDEO_MEMORY_ALLOCATOR_TRANSFER_LOG(last_allocator, allocator)
#endif


#if vpmdENABLE_MMU
/*
@brief Get VIP'a virtual address corresponding to the VIP's physical address.
@param handle, the handle of video memory.
@param IN physical_start, the start address of VIP physical.
@param IN physical_end, the end address of VIP physical.
@param OUT virtual_addr, the VIP's virtual address.
*/
static vip_status_e vipdrv_mem_get_vipvirtual(
    vipdrv_context_t *context,
    vip_physical_t physical_start,
    vip_physical_t physical_end,
    vip_virtual_t *virtual_addr
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t axi_sram_size = 0;
    vip_uint32_t i = 0;

    for (i = 0; i < context->device_count; i++) {
        axi_sram_size += context->axi_sram_size[i];
    }

    *virtual_addr = 0;

    /*
    check the VIP physical <physical_start ~~ physical_end> in maped MMU.
    If memory in AXI SRAM, heap or reserve physical memory, vip virtual memory has been mapped.
    */

#if vpmdENABLE_RESERVE_PHYSICAL
    if (physical_start >= context->reserve_phy_base &&
        physical_start < context->reserve_phy_base + context->reserve_phy_size) {
        if (physical_end <= context->reserve_phy_base + context->reserve_phy_size) {
            vip_uint32_t offset = (vip_uint32_t)(physical_start - context->reserve_phy_base);
            *virtual_addr = context->reserve_virtual + offset;
        }
        else {
            PRINTK_E("physical (0x%"PRIx64" ~ 0x%"PRIx64") overlap with reserve physical (0x%"PRIx64" ~ 0x%"PRIx64")",
                physical_start, physical_end, context->reserve_phy_base,
                context->reserve_phy_base + context->reserve_phy_size);
            status = VIP_ERROR_FAILURE;
        }
        goto onError;
    }
#endif

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    {
    vipdrv_alloc_mgt_t* cur = (vipdrv_alloc_mgt_t*)context->alloc_list_head;
    while (cur) {
        if (cur->allocator.alctor_type & VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP) {
            vipdrv_alloc_mgt_heap_t* heap = (vipdrv_alloc_mgt_heap_t*)cur;
            if (physical_start >= heap->physical &&
                physical_start < heap->physical + heap->total_size) {
                if (physical_end <= heap->physical + heap->total_size) {
                    vip_uint32_t offset = (vip_uint32_t)(physical_start - heap->physical);
                    *virtual_addr = heap->virtual + offset;
                }
                else {
                    PRINTK_E("physical (0x%"PRIx64" ~ 0x%"PRIx64") overlap with heap (0x%"PRIx64" ~ 0x%"PRIx64")",
                        physical_start, physical_end, heap->physical, heap->physical + heap->total_size);
                    status = VIP_ERROR_FAILURE;
                }
                goto onError;
            }
        }
        cur = (vipdrv_alloc_mgt_t*)cur->next;
    }
    }
#endif

    if (physical_start >= context->axi_sram_physical &&
        physical_start < context->axi_sram_physical + axi_sram_size) {
        if (physical_end <= context->axi_sram_physical + axi_sram_size) {
            vip_uint32_t offset = (vip_uint32_t)(physical_start - context->axi_sram_physical);
            *virtual_addr = context->axi_sram_address + offset;
        }
        else {
            PRINTK_E("physical (0x%"PRIx64" ~ 0x%"PRIx64") overlap with axi sram (0x%"PRIx64" ~ 0x%"PRIx64")",
                physical_start, physical_end, context->axi_sram_physical,
                context->axi_sram_physical + axi_sram_size);
            status = VIP_ERROR_FAILURE;
        }
        goto onError;
    }

onError:
    return status;
}
#endif

/* the index of memory database to mem_id */
static vip_uint32_t vipdrv_mem_index2id(
    vip_uint32_t index
    )
{
    vip_uint32_t mem_id = MEMORY_MAGIC_DATA + index;

    return mem_id;
}

/* covert mem_id to index of memory database */
static vip_uint32_t vipdrv_mem_id2index(
    vip_uint32_t mem_id
    )
{
    vip_uint32_t index = -1;
    if (mem_id >= MEMORY_MAGIC_DATA) {
        index = mem_id - MEMORY_MAGIC_DATA;
    }

    return index;
}


static vipdrv_mem_flag_e vipdrv_flag_convert(
    vipdrv_video_mem_alloc_flag_e alloc_flag
    )
{
    vipdrv_mem_flag_e flag = VIPDRV_MEM_FLAG_NONE;

    if (VIPDRV_VIDEO_MEM_ALLOC_MMU_PAGE_MEM & alloc_flag) {
        flag |= VIPDRV_MEM_FLAG_MMU_PAGE_MEM;
    }

    if (VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS & alloc_flag) {
        flag |= VIPDRV_MEM_FLAG_CONTIGUOUS;
    }
    else if (VIPDRV_VIDEO_MEM_ALLOC_16M_CONTIGUOUS & alloc_flag) {
        flag |= VIPDRV_MEM_FLAG_16M_CONTIGUOUS;
    }
    else if (VIPDRV_VIDEO_MEM_ALLOC_1M_CONTIGUOUS & alloc_flag) {
        flag |= VIPDRV_MEM_FLAG_1M_CONTIGUOUS;
    }
    else if (VIPDRV_VIDEO_MEM_ALLOC_64K_CONTIGUOUS & alloc_flag) {
        flag |= VIPDRV_MEM_FLAG_64K_CONTIGUOUS;
    }

    if ((VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR & alloc_flag) &&
        VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE & alloc_flag) {
        flag |= VIPDRV_MEM_FLAG_4GB_ADDR_PA;
    }

    return flag;
}

static vipdrv_mem_flag_e vipdrv_flag_strip(
    vipdrv_mem_flag_e flag
    )
{
    if (VIPDRV_MEM_FLAG_MMU_PAGE_MEM & flag) {
        flag &= ~VIPDRV_MEM_FLAG_MMU_PAGE_MEM;
    }

    if (VIPDRV_MEM_FLAG_16M_CONTIGUOUS == (VIPDRV_MEM_FLAG_16M_CONTIGUOUS & flag)) {
        flag &= ~VIPDRV_MEM_FLAG_16M_CONTIGUOUS;
        flag |= VIPDRV_MEM_FLAG_1M_CONTIGUOUS;
    }
    else if (VIPDRV_MEM_FLAG_1M_CONTIGUOUS == (VIPDRV_MEM_FLAG_1M_CONTIGUOUS & flag)) {
        flag &= ~VIPDRV_MEM_FLAG_1M_CONTIGUOUS;
        flag |= VIPDRV_MEM_FLAG_64K_CONTIGUOUS;
    }
    else if (VIPDRV_MEM_FLAG_64K_CONTIGUOUS == (VIPDRV_MEM_FLAG_64K_CONTIGUOUS & flag)) {
        flag &= ~VIPDRV_MEM_FLAG_64K_CONTIGUOUS;
    }

    return flag;
}

/*
@brief wrap user memory to VIP virtual address.
@param IN physical_table Physical address table. should be wraped for VIP hardware.
@param IN size_table The size of physical memory for each physical_table element.
@param IN physical_num The number of physical table element.
@param IN alloc_flag the flag of this video memroy. see vipdrv_video_mem_alloc_flag_e.
@param IN memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param IN device_vis A bitmap to indicate which device could access this memory.
@param OUT logical, The user space logical address.
@param OUT virtual_addr, VIP virtual address.
@param OUT mem_id, the id of video memory.
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
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_video_mem_handle_t *ptr = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint32_t pos = 0;
    vip_uint32_t contiguous = vip_true_e;
    vipdrv_allocator_t* allocator = VIP_NULL;
    vipdrv_allocator_wrapphy_param_t param = { 0 };
    vipdrv_mem_control_block_t* mcb = VIP_NULL;

    if ((VIP_NULL == physical_table) || (VIP_NULL == size_table)) {
        PRINTK_E("fail to wrap user physical, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (physical_num < 1) {
        PRINTK_E("fail to wrap user physical, physical num is %d\n", physical_num);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

#if !vpmdENABLE_MMU
    if (physical_num != 1) {
        PRINTK_E("fail to create buffer from physical mmu is disabled, physical number=%d\n",
                    physical_num);
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    if (physical_table[0] > 0xFFFFFFFF) {
        PRINTK_E("fail to create buffer from physical mmu is disabled, physical address=0x%"PRIx64"\n",
                    physical_table[0]);
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
#endif

    PRINTK_D("user physical, phy_table[0]=0x%"PRIx64", size_table[0]=0x%"PRIx64", physical_num=%d, memory_type=%d\n",
              physical_table[0], size_table[0], physical_num, memory_type);

    vipdrv_os_allocate_memory(sizeof(vipdrv_video_mem_handle_t), (void**)&ptr);
    if (VIP_NULL == ptr) {
        PRINTK_E("fail to allocate memory for wrap physical video memory handle\n");
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    vipdrv_os_zero_memory(ptr, sizeof(vipdrv_video_mem_handle_t));
    ENSURE_MEM_DATABASE_CAPACITY();
    status = vipdrv_database_insert(&context->mem_database, &pos);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        PRINTK_E("fail to wrap user physical, hashmap is full\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    param.phy_num = physical_num;
    param.phy_table = physical_table;
    param.size_table = size_table;

#if 0/* #if vpmdENABLE_FLUSH_CPU_CACHE; map_dma_sg for AXI SRAM crash */
    if (VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE & alloc_flag) {
        ptr->mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;
    }
#else
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;
#endif

    vipOnError(vipdrv_allocator_get(VIPDRV_ALLOCATOR_TYPE_WRAP_USER_PHYSICAL, 0, device_vis, &allocator));
    ptr->allocator = allocator;
    vipOnError(allocator->callback.allocate(ptr, &param));

    /* check physical table are contiguous ? */
    for (i = 0; i < physical_num - 1; i++) {
        if ((physical_table[i] + size_table[i]) != physical_table[i + 1]) {
            contiguous = vip_false_e;
            break;
        }
    }
#if !vpmdENABLE_MMU
    /* check vip-sram not overlap with user wrap physical address*/
    if ((vip_true_e == contiguous) && (context->vip_sram_size > 0)) {
        if((ptr->physical_table[0] + ptr->size > context->vip_sram_address)
                && (context->vip_sram_address >= ptr->physical_table[0])) {
            PRINTK_E("user wrap physical address 0x%x, size 0x%x, overlap vip sram address 0x%x",
                ptr->physical_table[0], ptr->size, context->vip_sram_address);
            vipGoOnError(VIP_ERROR_FAILURE);
        }
    }
#endif

    if (VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL & alloc_flag) {
        vipOnError(allocator->callback.map_kernel_logical(ptr));
    }

    if (VIPDRV_VIDEO_MEM_ALLOC_MAP_USER & alloc_flag) {
        vipOnError(allocator->callback.map_user_logical(ptr));
        PRINTK_D("wrap user physical, user logical=0x%"PRIx64"\n", ptr->user_logical);
    }

    #if vpmdENABLE_MMU
    if (vip_true_e == contiguous) {
        vip_uint64_t mmu_id = gen_mmu_id(vipdrv_os_get_pid(), 0);
        vip_size_t temp_size[2] = {ptr->size, 0};
        vip_virtual_t virtual_tmp = 0;
        vip_physical_t physical_base = ptr->physical_table[0];
        vip_physical_t physical_end = physical_base + ptr->size;
        vipOnError(vipdrv_mem_get_vipvirtual(context, physical_base, physical_end, &virtual_tmp));
        if (0 == virtual_tmp) {
            /* get page base address */
            vipOnError(vipdrv_mmu_map(&ptr->mmu_handle, ptr->physical_table,
                                     1, temp_size, &virtual_tmp, mmu_id, 1, 0));
            ptr->vip_address = virtual_tmp;
            PRINTK_I("user physical contiguous handle=0x%"PRPx", base physical=0x%"PRIx64","
                     " virtual=0x%08x, size=0x%"PRIx64"\n",
                     ptr, ptr->physical_table[0], ptr->vip_address, ptr->size);
        }
        else {
            ptr->vip_address = virtual_tmp;
            PRINTK_I("user handle=0x%"PRPx", base physical=0x%"PRIx64", virtual=0x%08x, size=0x%"PRIx64"\n",
                     ptr, ptr->physical_table[0], ptr->vip_address, ptr->size);
        }
    }
    else {
        vip_uint64_t mmu_id = gen_mmu_id(vipdrv_os_get_pid(), 0);
        vip_virtual_t virtual_tmp = 0;
        /* get page base address */
        vipOnError(vipdrv_mmu_map(&ptr->mmu_handle, ptr->physical_table,
            physical_num, ptr->size_table, &virtual_tmp, mmu_id, 1, 0));
        ptr->vip_address = virtual_tmp;
        PRINTK_I("user physical handle=0x%"PRPx", base physical=0x%"PRIx64", physical_num=%d,"
                 " virtual=0x%08x, size=0x%"PRIx64"\n",
                  ptr, ptr->physical_table[0], physical_num, ptr->vip_address, ptr->size);
    }
    #else
    if (vip_false_e == contiguous) {
        PRINTK_E("wrap physical fail. physical no continue when MMU is disabled\n");
        vipGoOnError(VIP_ERROR_FAILURE);
    }
    ptr->vip_address = ptr->physical_table[0];
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NO_MMU_PAGE;
    #endif

    if (virtual_addr != VIP_NULL) {
        *virtual_addr = ptr->vip_address;
    }
    else {
        PRINTK_E("fail to wrap physical, virtual pointer is NULL\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    *logical = (void*)ptr->user_logical;
    *mem_id = vipdrv_mem_index2id(pos);

    status = vipdrv_database_use(&context->mem_database, pos, (void**)&mcb);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to wrap phy get mem databse index=%d\n", pos);
        vipGoOnError(status);
    }
    mcb->handle = ptr;
    mcb->process_id = vipdrv_os_get_pid();
    vipdrv_database_unuse(&context->mem_database, pos);

#if vpmdENABLE_DEBUGFS
    vipdrv_debug_videomemory_profile_alloc(ptr->size);
#endif

    return status;
onError:
    vipdrv_allocator_dump();
    *mem_id = 0;
    if (VIP_NULL != allocator) {
        allocator->callback.free(ptr);
    }
    else {
        PRINTK_E("fail to get wrap physical allocator is NULL\n");
    }
    vipdrv_database_remove(&context->mem_database, pos, vip_false_e);
    if (ptr != VIP_NULL) {
        vipdrv_os_free_memory(ptr);
    }
    PRINTK_E("fail to mem wrap user phsical staus=%d\n", status);
    VIPDRV_DUMP_STACK();
    return status;
}

/*
@brief wrap user memory to VIP virtual address.
@param IN logical, the user space logical address(handle).
@param IN size, the size of the memory.
@param IN memory_type The type of this VIP buffer memory.
          see vip_buffer_memory_type_e | vipdrv_video_mem_alloc_flag_e
@param IN device_vis A bitmap to indicate which device could access this memory.
@param OUT virtual_addr, VIP virtual address.
@param OUT mem_id, the id of video memory.
*/
vip_status_e vipdrv_mem_wrap_usermemory(
    vip_ptr logical,
    vip_size_t size,
    vip_uint32_t memory_type,
    vip_uint64_t device_vis,
    vip_address_t *virtual_addr,
    vip_uint32_t *mem_id
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_video_mem_handle_t *ptr = VIP_NULL;
    vip_uint32_t pos = 0;
    vipdrv_allocator_t* allocator = VIP_NULL;
    vipdrv_allocator_wrapmem_param_t param = { 0 };
    vipdrv_mem_control_block_t* mcb = VIP_NULL;

    if (VIP_NULL == logical) {
        PRINTK_E("fail to wrap user memory, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    vipdrv_os_allocate_memory(sizeof(vipdrv_video_mem_handle_t), (void**)&ptr);
    if (VIP_NULL == ptr) {
        PRINTK_E("fail to allocate memory for video memory ptr\n");
        return VIP_ERROR_OUT_OF_MEMORY;
    }
    vipdrv_os_zero_memory(ptr, sizeof(vipdrv_video_mem_handle_t));

    ENSURE_MEM_DATABASE_CAPACITY();
    status = vipdrv_database_insert(&context->mem_database, &pos);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        PRINTK_E("fail to wrap memory, hashmap is full\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    param.size = size;
    param.user_logical = logical;

    vipOnError(vipdrv_allocator_get(VIPDRV_ALLOCATOR_TYPE_WRAP_USER_LOGICAL, 0, device_vis, &allocator));
    ptr->allocator = allocator;
    vipOnError(allocator->callback.allocate(ptr, &param));

    #if vpmdENABLE_MMU
    {
        vip_uint64_t mmu_id = gen_mmu_id(vipdrv_os_get_pid(), 0);
        /* mapping VIP MMU page table */
        vip_physical_t physical_base = 0;
        vip_virtual_t virtual_tmp = 0;
        if (0 == ptr->physical_num) {
            PRINTK_E("fail map user memory, physical num is 0\n");
            vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
        physical_base = ptr->physical_table[0];

        if (1 == ptr->physical_num) {
            vip_physical_t physical_end = physical_base + ptr->size;
            vipOnError(vipdrv_mem_get_vipvirtual(context, physical_base, physical_end, &virtual_tmp));
        }

        if (0 == virtual_tmp) {
            vip_uint8_t writable = 0;
            if (0x00 == (VIPDRV_VIDEO_MEM_ALLOC_NPU_READ_ONLY & memory_type)) {
                ptr->mem_flag |= VIPDRV_MEM_FLAG_MMU_PAGE_WRITEABLE;
                writable = 1;
            }
            vipOnError(vipdrv_mmu_map(&ptr->mmu_handle, ptr->physical_table,
                                     ptr->physical_num, ptr->size_table,
                                     &virtual_tmp, mmu_id, writable, 0));
            ptr->vip_address = virtual_tmp;
        }
        else {
            ptr->vip_address = virtual_tmp;
            PRINTK_D("user handle=0x%"PRPx", base physical=0x%"PRIx64", virtual=0x%08x, size=0x%"PRIx64"\n",
                ptr, ptr->physical_table[0], ptr->vip_address, ptr->size);
        }
    }
    #else
    VIDEO_MEMORY_PHYSICAL_CHECK(ptr->physical_table[0]);
    ptr->vip_address = ptr->physical_table[0];
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NO_MMU_PAGE;
    #endif

    if (virtual_addr != VIP_NULL) {
        *virtual_addr = ptr->vip_address;
    }
    else {
        PRINTK_E("fail to wrap memory, virtual pointer is NULL\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    PRINTK_I("wrap user memory handle=0x%"PRPx", logical=0x%"PRPx", physial base=0x%"PRIx64", "
             "virtual=0x%"PRIx64", size=0x%"PRIx64", allocator_type=0x%x\n", ptr, logical,
        ptr->physical_table[0], ptr->vip_address, ptr->size, ptr->allocator_type);
    *mem_id = vipdrv_mem_index2id(pos);

    status = vipdrv_database_use(&context->mem_database, pos, (void**)&mcb);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to unwrap phy get mem databse index=%d\n", pos);
        vipGoOnError(status);
    }
    mcb->handle = ptr;
    mcb->process_id = vipdrv_os_get_pid();
    vipdrv_database_unuse(&context->mem_database, pos);

#if vpmdENABLE_DEBUGFS
    vipdrv_debug_videomemory_profile_alloc(ptr->size);
#endif

    return status;

onError:
    vipdrv_allocator_dump();
    *mem_id = 0;
    if (VIP_NULL != allocator) {
        allocator->callback.free(ptr);
    }
    else {
        PRINTK_E("fail to get wrap user memory allocator\n");
    }
    vipdrv_database_remove(&context->mem_database, pos, vip_false_e);
    if (ptr != VIP_NULL) {
        vipdrv_os_free_memory((void*)ptr);
    }
    VIPDRV_DUMP_STACK();
    return status;
}

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
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_video_mem_handle_t *ptr = VIP_NULL;
    vip_uint32_t pos = 0;
    vipdrv_allocator_t* allocator = VIP_NULL;
    vipdrv_allocator_wrapfd_param_t param = { 0 };
    vipdrv_mem_control_block_t* mcb = VIP_NULL;

    if (VIP_NULL == virtual_addr) {
        PRINTK_E("fail to wrap fd param is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    vipdrv_os_allocate_memory(sizeof(vipdrv_video_mem_handle_t), (void**)&ptr);
    if (VIP_NULL == ptr) {
        PRINTK_E("fail to allocate memory for video memory ptr\n");
        return VIP_ERROR_OUT_OF_MEMORY;
    }
    vipdrv_os_zero_memory(ptr, sizeof(vipdrv_video_mem_handle_t));

    ENSURE_MEM_DATABASE_CAPACITY();
    status = vipdrv_database_insert(&context->mem_database, &pos);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        PRINTK_E("fail to wrap fd, hashmap is full\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    param.size = size;
    param.fd = fd;

    vipOnError(vipdrv_allocator_get(VIPDRV_ALLOCATOR_TYPE_WRAP_DMA_BUF, 0, device_vis, &allocator));
    ptr->allocator = allocator;
    vipOnError(allocator->callback.allocate(ptr, &param));

#if vpmdENABLE_MMU
    {
        vip_uint64_t mmu_id = gen_mmu_id(vipdrv_os_get_pid(), 0);
        /* mapping VIP MMU page table */
        vip_physical_t physical_base = ptr->physical_table[0];
        vip_virtual_t virtual_tmp = 0;

        if (1 == ptr->physical_num) {
            vip_physical_t physical_end = physical_base + ptr->size;
            vipOnError(vipdrv_mem_get_vipvirtual(context, physical_base, physical_end, &virtual_tmp));
        }

        if (0 == virtual_tmp) {
            vipOnError(vipdrv_mmu_map(&ptr->mmu_handle, ptr->physical_table,
                                     ptr->physical_num, ptr->size_table,
                                     &virtual_tmp, mmu_id, 1, 0));
            ptr->vip_address = virtual_tmp;
            *virtual_addr = ptr->vip_address;
        }
        else {
            ptr->vip_address = virtual_tmp;
            PRINTK_D("user handle=0x%"PRPx", base physical=0x%"PRIx64", virtual=0x%"PRIx64", size=0x%"PRIx64"\n",
                ptr, ptr->physical_table[0], ptr->vip_address, ptr->size);
        }
    }
#else
    if (ptr->physical_num != 1) {
        PRINTK_E("fail to wrap user fd memory, physical not contiguous and mmu disabled\n");
        vipGoOnError(VIP_ERROR_FAILURE);
    }
    ptr->vip_address = ptr->physical_table[0];
    *virtual_addr = ptr->physical_table[0];
    ptr->mem_flag |= VIPDRV_MEM_FLAG_NO_MMU_PAGE;
#endif

    PRINTK_I("wrap user fd=%d, physial base=0x%"PRIx64", "
             "virtual=0x%"PRIx64", size=0x%"PRIx64", allocator_type=0x%x\n", fd, \
             ptr->physical_table[0], ptr->vip_address, ptr->size, ptr->allocator_type);

    *mem_id = vipdrv_mem_index2id(pos);
    *logical = ptr->user_logical;

    status = vipdrv_database_use(&context->mem_database, pos, (void**)&mcb);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to wrap fd get mem databse index=%d\n", pos);
        vipGoOnError(status);
    }
    mcb->handle = ptr;
    mcb->process_id = vipdrv_os_get_pid();
    vipdrv_database_unuse(&context->mem_database, pos);

#if vpmdENABLE_DEBUGFS
    vipdrv_debug_videomemory_profile_alloc(ptr->size);
#endif

    return status;
onError:
    vipdrv_allocator_dump();
    *mem_id = 0;
    if (VIP_NULL != allocator) {
        allocator->callback.free(ptr);
    }
    else {
        PRINTK_E("fail to get wrap user fd allocator\n");
    }
    vipdrv_database_remove(&context->mem_database, pos, vip_false_e);
    if ((ptr != VIP_NULL) && (ptr->physical_table != VIP_NULL)) {
        vipdrv_os_free_memory(ptr->physical_table);
        ptr->physical_table = VIP_NULL;
    }
    if (ptr != VIP_NULL) {
        vipdrv_os_free_memory(ptr);
    }
    VIPDRV_DUMP_STACK();
    return status;
}
#endif

/*
@brief Map user space logical address.
@param IN mem_id, the id of video memory.
@param OUT logical, user space logical address mapped.
*/
vip_status_e vipdrv_mem_map_userlogical(
    vip_uint32_t mem_id,
    void** logical
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t pos = 0;
    vipdrv_mem_control_block_t* mcb = VIP_NULL;
    vipdrv_video_mem_handle_t* ptr = VIP_NULL;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

    CHECK_VIDEO_MEM_ID_VALID(mem_id);
    pos = vipdrv_mem_id2index(mem_id);

    status = vipdrv_database_use(&context->mem_database, pos, (void**)&mcb);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to map user logical, wrong handle id=%d\n", pos);
        return VIP_ERROR_FAILURE;
    }
    ptr = mcb->handle;
    if (VIP_NULL == logical) {
        PRINTK_E("fail to map user logical, logical is NULL\n");
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    if (VIP_NULL == ptr->user_logical || ptr->pid != vipdrv_os_get_pid()) {
        vipdrv_allocator_t* allocator = ptr->allocator;
        vipOnError(allocator->callback.map_user_logical(ptr));
    }

    *logical = (void*)(ptr->user_logical);
    vipdrv_database_unuse(&context->mem_database, pos);
    return status;
onError:
    vipdrv_database_unuse(&context->mem_database, pos);
    VIPDRV_DUMP_STACK();
    return status;
}

/*
@brief Map user space logical address.
@param IN mem_id, the id of video memory.
*/
vip_status_e vipdrv_mem_unmap_userlogical(
    vip_uint32_t mem_id
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t pos = 0;
    vipdrv_mem_control_block_t* mcb = VIP_NULL;
    vipdrv_video_mem_handle_t* ptr = VIP_NULL;
    vipdrv_allocator_t* allocator = VIP_NULL;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

    CHECK_VIDEO_MEM_ID_VALID(mem_id);
    pos = vipdrv_mem_id2index(mem_id);

    status = vipdrv_database_use(&context->mem_database, pos, (void**)&mcb);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to unmap user logical, wrong handle id=%d\n", pos);
        return VIP_ERROR_FAILURE;
    }
    ptr = mcb->handle;
    allocator = ptr->allocator;
    if (VIP_NULL != ptr->user_logical && ptr->pid == vipdrv_os_get_pid()) {
        status = allocator->callback.unmap_user_logical(ptr);
    }
    vipdrv_database_unuse(&context->mem_database, pos);
    return status;
}

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
@param physical,
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
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_video_mem_handle_t *ptr = VIP_NULL;
    vip_physical_t phy_addr = 0;
    vip_uint32_t pos = 0;
#if vpmdENABLE_MMU
    vip_uint32_t i = 0;
#endif
#if (vpmdENABLE_DEBUG_LOG >= 4)
    vipdrv_allocator_t* last_allocator = VIP_NULL;
#endif
    vipdrv_mem_flag_e flag = VIPDRV_MEM_FLAG_NONE;
    vipdrv_allocator_t* allocator = VIP_NULL;
    vipdrv_allocator_alloc_param_t alloc_param = { 0 };
    vipdrv_mem_control_block_t* mcb = VIP_NULL;
    vip_uint32_t pid = mmu_id >> 32;
#if !vpmdENABLE_40BITVA
    alloc_flag |= VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR;
#endif
#if !vpmdENABLE_MMU
    alloc_flag |= VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE;
#endif

    if ((VIP_NULL == logical) || (VIP_NULL == context) ||
        (VIP_NULL == physical) || (VIP_NULL == mem_id)) {
        PRINTK_E("fail to allocate memory, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    if (0 == align) {
        PRINTK_E("fail to allocate memory, align=0\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    if (specified) {
        if (0 != (*physical & (align - 1))) {
            PRINTK_E("fail to allocate memory, specified physical 0x%"PRIx64" not align to %u\n",
                *physical, align);
            return VIP_ERROR_INVALID_ARGUMENTS;
        }
    }

    PRINTK_D("video memory alloc_flag=0x%x, align=0x%x, size=0x%"PRIx64"\n", alloc_flag, align, size);
    status = vipdrv_os_allocate_memory(sizeof(vipdrv_video_mem_handle_t), (void**)&ptr);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to alloc ptr for video memory\n");
        return VIP_ERROR_OUT_OF_MEMORY;
    }
    vipdrv_os_zero_memory(ptr, sizeof(vipdrv_video_mem_handle_t));
    ENSURE_MEM_DATABASE_CAPACITY();
    status = vipdrv_database_insert(&context->mem_database, &pos);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        PRINTK_E("fail to wrap user physical, hashmap is full\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    alloc_param.size = size;
    alloc_param.align = align;
    if (specified) {
        alloc_param.specified = 1;
        #if vpmdENABLE_MMU
        if (0x00 == (VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE & alloc_flag)) {
            alloc_param.virtual = 1;
        }
        else
        #endif
        {
            alloc_param.virtual = 0;
        }
        alloc_param.address = *physical;
    }
    else {
        alloc_param.specified = 0;
    }
    flag = vipdrv_flag_convert(alloc_flag);

allocMem:
    alloc_param.mem_flag = flag;
#if vpmdENABLE_VIDEO_MEMORY_CACHE
    if (VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE & alloc_flag) {
        alloc_param.mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;
    }
#else
    alloc_param.mem_flag |= VIPDRV_MEM_FLAG_NONE_CACHE;
#endif
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    if (0x00 == (VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE & alloc_flag)) {
        if (0x00 == (VIPDRV_VIDEO_MEM_ALLOC_NPU_READ_ONLY & alloc_flag)) {
            flag |= VIPDRV_MEM_FLAG_MMU_PAGE_WRITEABLE;
        }
        if (VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR & alloc_flag) {
            flag |= VIPDRV_MEM_FLAG_4GB_ADDR_VA;
        }
    }
    status = VIP_ERROR_FAILURE;
    allocator = VIP_NULL;
    while (VIP_SUCCESS == vipdrv_allocator_get(VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP, flag, device_vis, &allocator)) {
        ptr->allocator = allocator;
        status = allocator->callback.allocate(ptr, &alloc_param);
        if (VIP_SUCCESS == status) {
            phy_addr = ptr->physical_table[0];
            break;
        }
        VIDEO_MEMORY_ALLOCATOR_TRANSFER_LOG(last_allocator, allocator);
    }
    flag &= ~VIPDRV_MEM_FLAG_MMU_PAGE_WRITEABLE;
    flag &= ~VIPDRV_MEM_FLAG_4GB_ADDR_VA;
    if (VIP_SUCCESS != status)
#endif
    {
        if (specified && (!alloc_param.virtual)) {
            PRINTK_E("must with mmu when allocate from dync with specified address\n");
            vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
        status = VIP_ERROR_FAILURE;
        allocator = VIP_NULL;
        while (VIP_SUCCESS == vipdrv_allocator_get(VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC, flag, device_vis, &allocator)) {
            ptr->allocator = allocator;
            status = allocator->callback.allocate(ptr, &alloc_param);
            if (VIP_SUCCESS == status) {
                break;
            }
            VIDEO_MEMORY_ALLOCATOR_TRANSFER_LOG(last_allocator, allocator);
        }
        if (VIP_SUCCESS != status) {
            vipdrv_mem_flag_e new_flag = VIPDRV_MEM_FLAG_NONE;
            new_flag = vipdrv_flag_strip(flag);
            if (new_flag != flag) {
                PRINTK_D("strip alloc flag 0x%x -> 0x%x allocate video memory again\n", flag, new_flag);
                flag = new_flag;
                goto allocMem;
            }
            else {
                PRINTK_E("fail to alloc video mem allocator name=%s\n",
                    VIP_NULL != allocator ? allocator->name : "None");
                goto onError;
            }
        }
        #if vpmdENABLE_MMU
        /* check base address is aligned and overflow */
        if ((ptr->physical_num > 0) && (ptr->physical_table != VIP_NULL)) {
            for (i = 0; i < ptr->physical_num; i++) {
                if ((ptr->physical_table[i] & (align - 1)) != 0) {
                    PRINTK_E("dyn allocate vido memory physical not align to %d, page_index=%d, "
                             "physical=0x%"PRIx64"\n", align, i, ptr->physical_table[i]);
                    vipGoOnError(VIP_ERROR_FAILURE);
                }
                VIDEO_MEMORY_PHYSICAL_CHECK(ptr->physical_table[i]);
                #if ENABLE_MMU_MAPPING_LOG
                PRINTK("index=%d, physical=0x%"PRIx64", size=0x%"PRIx64"\n", i,
                    ptr->physical_table[i], ptr->size_table[i]);
                #endif
            }
        }
        else {
            PRINTK_E("physical num is 0 or physical table is NULL\n");
            vipGoOnError(VIP_ERROR_FAILURE);
        }
        #else
        /* check base address is aligned */
        if ((1 == ptr->physical_num) && (ptr->physical_table != VIP_NULL)) {
            if ((ptr->physical_table[0] & (align - 1)) != 0) {
                PRINTK_E("dyn allocate vido memory physical not align to 0x%x, physical=0x%"PRIx64"\n",
                    align, ptr->physical_table[0]);
                vipGoOnError(VIP_ERROR_FAILURE);
            }
            VIDEO_MEMORY_PHYSICAL_CHECK(ptr->physical_table[0]);
        }
        else {
            PRINTK_E("physical num is larger than 1, is %d or physical table is NULL\n",
                ptr->physical_num);
            vipGoOnError(VIP_ERROR_FAILURE);
        }
        /* check vip-sram not overlap with video memory heap */
        if (context->vip_sram_size > 0) {
            if ((context->vip_sram_address < (ptr->physical_table[0] + ptr->size)) &&
                (context->vip_sram_address >= ptr->physical_table[0])) {
                PRINTK_E("fail vip-sram address overlap with video memory heap, vip-sram=0x%08x, "
                 "vido_phy=0x%"PRIx64", size=0x%"PRIx64"\n", context->vip_sram_address,
                    ptr->physical_table[0], ptr->size);
                vipGoOnError(VIP_ERROR_FAILURE);
            }
        }
        #endif
        phy_addr = ptr->physical_table[0];
    }
    if (0 == phy_addr) {
        PRINTK_E("fail physical_table[0] address is 0\n");
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    if (VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL & alloc_flag) {
        vipOnError(allocator->callback.map_kernel_logical(ptr));
        *logical = (void*)ptr->kerl_logical;
    }

    if (VIPDRV_VIDEO_MEM_ALLOC_MAP_USER & alloc_flag) {
        vipOnError(allocator->callback.map_user_logical(ptr));
        *logical = (void*)ptr->user_logical;
    }
#if vpmdENABLE_MMU
    if (0x00 == (VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE & alloc_flag)) {
        if (ptr->allocator_type & VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC) {
            /* mapped VIP virtual */
            vip_uint8_t writable = 1;
            vip_virtual_t virtual_tmp = 0;
            if (0x00 == (VIPDRV_VIDEO_MEM_ALLOC_NPU_READ_ONLY & alloc_flag)) {
                ptr->mem_flag |= VIPDRV_MEM_FLAG_MMU_PAGE_WRITEABLE;
            }
            else {
                writable = 0;
            }
            if (specified) {
                virtual_tmp = alloc_param.address;
            }
            vipOnError(vipdrv_mmu_map(&ptr->mmu_handle, ptr->physical_table,
                ptr->physical_num, ptr->size_table, &virtual_tmp, mmu_id, writable,
                (VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR & alloc_flag) ? 1 : 0));
            ptr->vip_address = virtual_tmp;
        }
        *physical = ptr->vip_address;
    }
    else
#endif
    {
        if (ptr->allocator_type & VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC) {
            if (ptr->physical_num != 1) {
                PRINTK_E("fail to allocate, NO MMU page alloc flag  and contiguous,"
                    "physical_num must is  1, then %d\n",
                    ptr->physical_num);
                vipGoOnError(VIP_ERROR_FAILURE);
            }
            ptr->mem_flag |= VIPDRV_MEM_FLAG_NO_MMU_PAGE;
            PRINTK_D("dyn allocate video memory contiguous and no mmu page, physical=0x%x\n", phy_addr);
        }
        ptr->vip_address = phy_addr;
        *physical = phy_addr;
    }
    *mem_id = vipdrv_mem_index2id(pos);

    PRINTK_I("video memory allocator=%s, physical=0x%"PRIx64", virtual=0x%"PRIx64", %s logical=0x%"PRPx", "
             "mem_id=0x%x, size=0x%"PRIx64", align=0x%x, allo_flag=0x%x\n",
             allocator->name, phy_addr, *physical, (VIPDRV_VIDEO_MEM_ALLOC_MAP_USER & alloc_flag) ? "user" : \
             (VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL & alloc_flag)?  "kernel" : "None", *logical,
             *mem_id, size, align, alloc_flag);

    if ((alloc_flag & VIPDRV_VIDEO_MEM_ALLOC_4GB_ADDR) && (*physical & 0xFFFFFFFF00000000ULL)) {
        PRINTK_E("fail to allocate, physical=0x%"PRIx64" is larger than 4G, alloc_flag=0x%x\n",
                 *physical, alloc_flag);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    if ((*physical & (align - 1)) != 0) {
        PRINTK_E("fail to allocate, physical=0x%"PRIx64", not align=0x%x\n", *physical, align);
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    status = vipdrv_database_use(&context->mem_database, pos, (void**)&mcb);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to alloc vdo mem get mem databse index=%d\n", pos);
        vipGoOnError(status);
    }
    mcb->handle = ptr;
    mcb->process_id = pid;
    vipdrv_database_unuse(&context->mem_database, pos);

#if vpmdENABLE_DEBUGFS
    vipdrv_debug_videomemory_profile_alloc(ptr->size);
#endif

    return status;

onError:
    vipdrv_allocator_dump();
    *mem_id = 0;
    *logical = VIP_NULL;
    *physical = 0;
    PRINTK_E("fail to allocate video memory from %s, size=0x%"PRIx64", alloc_flag=0x%x status=%d\n",
        ptr->allocator_type & VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC ? "Dynamic Allocate" : "Video MEM heap",
        size, alloc_flag, status);
    if (VIP_NULL != allocator) {
        allocator->callback.free(ptr);
    }
    vipdrv_database_remove(&context->mem_database, pos, vip_false_e);
    vipdrv_os_free_memory(ptr);
    if (allocator != VIP_NULL) {
        PRINTK_E("fail allocate name=%s\n", allocator->name);
    }
    VIPDRV_DUMP_STACK();
    return status;
}

/*
@brief free video memory.
@mem_id the id of video memory.
*/
vip_status_e vipdrv_mem_free_videomemory(
    vip_uint32_t mem_id
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vip_uint32_t pos = 0;

    CHECK_VIDEO_MEM_ID_VALID(mem_id);
    pos = vipdrv_mem_id2index(mem_id);

    vipdrv_database_remove(&context->mem_database, pos, vip_false_e);
    PRINTK_I("free video memory mem_id=0x%x\n", mem_id);
    return status;
}

/*
force free all video memory allocated by process_id
*/
vip_status_e vipdrv_mem_force_free_videomemory(
    vip_uint32_t process_id
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vipdrv_mem_control_block_t *mcb = VIP_NULL;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

    for (i = 1; i < vipdrv_database_free_pos(&context->mem_database); i++) {
        vipdrv_database_use(&context->mem_database, i, (void**)&mcb);
        if (VIP_NULL != mcb) {
            if ((mcb->process_id == process_id) && (mcb->handle != VIP_NULL)) {
                vipdrv_video_mem_handle_t* ptr = mcb->handle;
                vip_uint32_t mem_id = vipdrv_mem_index2id(i);
                PRINTK_I("force free video memory handle=0x%"PRPx"\n", mcb->handle);
                /* process is exit abnormally, can not release user logical */
                ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_USER_LOGICAL;
                ptr->user_logical = VIP_NULL;
                vipdrv_mem_free_videomemory(mem_id);
            }
            vipdrv_database_unuse(&context->mem_database, i);
        }
    }

    return status;
}


#if vpmdENABLE_FLUSH_CPU_CACHE
/*
@brief flush video memory CPU cache
@param mem_id theid of video memory be flush CPU cache.
@param type The type of operate cache. see vipdrv_cache_type_e.
*/
vip_status_e vipdrv_mem_flush_cache(
    vip_uint32_t mem_id,
    vipdrv_cache_type_e type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_mem_control_block_t* mcb = VIP_NULL;
    vipdrv_video_mem_handle_t *ptr = VIP_NULL;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_allocator_t* allocator = VIP_NULL;
    vip_uint32_t pos = 0;

    CHECK_VIDEO_MEM_ID_VALID(mem_id);
    pos = vipdrv_mem_id2index(mem_id);

    status = vipdrv_database_use(&context->mem_database, pos, (void**)&mcb);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to flush video memory cache, wrong handle id=%d\n", pos);
        return VIP_ERROR_FAILURE;
    }
    ptr = mcb->handle;
    if (VIP_NULL == ptr) {
        PRINTK_E("fail to flush video memory cache, handle is NULL\n");
        vipdrv_database_unuse(&context->mem_database, pos);
        return VIP_ERROR_FAILURE;
    }

    allocator = ptr->allocator;
    vipOnError(allocator->callback.flush_cache(ptr, type));
    PRINTK_D("flush cache mem_id=0x%x, type=%s\n", mem_id, (type==VIPDRV_CACHE_FLUSH) ? "FLUSH" : "INVALID");

    vipdrv_database_unuse(&context->mem_database, pos);

    return status;
onError:
    vipdrv_database_unuse(&context->mem_database, pos);
    PRINTK_E("fail to vipdrv flush cache, mem_id=0x%x\n", mem_id);
    return status;
}
#endif

static vip_status_e free_video_memory_handle(
    void* element
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_mem_control_block_t* mcb = (vipdrv_mem_control_block_t*)element;
    vipdrv_video_mem_handle_t* ptr = ptr = mcb->handle;
    vipdrv_allocator_t* allocator = VIP_NULL;

    if (VIP_NULL == ptr) {
        return status;
    }
    allocator = ptr->allocator;

#if vpmdENABLE_DEBUGFS
    vipdrv_debug_videomemory_profile_free(ptr->size);
#endif

    if (VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP & ptr->allocator_type) {
        #if vpmdENABLE_VIDEO_MEMORY_HEAP
        allocator->callback.free(ptr);
        #else
        PRINTK_E("fail to free memory, video memory heap has been disable,"
                 "vpmdENABLE_VIDEO_MEMORY_HEAP=0\n");
        #endif
    }
    else {
        /* unmap mmu */
        #if vpmdENABLE_MMU
        if (0x00 == (ptr->mem_flag & VIPDRV_MEM_FLAG_NO_MMU_PAGE)) {
            vipdrv_mmu_unmap(ptr->mmu_handle, gen_mmu_id(mcb->process_id, mcb->task_id));
        }
        #endif
        /* free pages */
        allocator->callback.free(ptr);
    }

    if (VIP_NULL != mcb->handle) {
        vipdrv_os_free_memory(mcb->handle);
        mcb->handle = VIP_NULL;
    }

    return status;
}

vip_status_e vipdrv_video_memory_init(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

    vipOnError(vipdrv_database_init(&context->mem_database, INIT_MEM_DATABASE_NUM,
        sizeof(vipdrv_mem_control_block_t), "memory-mgt", free_video_memory_handle));

    /* 1. Init video memory allocator */
    status = vipdrv_allocator_init();
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize allocator\n");
        vipGoOnError(VIP_ERROR_IO);
    }

    /* 2. Init MMU page table */
#if vpmdENABLE_MMU
    status = vipdrv_mmu_init();
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to initialize MMU, status=%d.\n", status);
        vipGoOnError(status);
    }
#endif

onError:
    return status;
}

vip_status_e vipdrv_video_memory_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_mem_control_block_t* mcb = VIP_NULL;

    /* avoid memory leak if any video memory is not free */
    for (i = 1; i < vipdrv_database_free_pos(&context->mem_database); i++) {
        vipdrv_database_use(&context->mem_database, i, (void**)&mcb);
        if (VIP_NULL != mcb) {
            if ((0 != mcb->process_id) && (VIP_NULL != mcb->handle)) {
                vipdrv_video_mem_handle_t* ptr = mcb->handle;
                PRINTK_E("free video memory before destroy handle=0x%"PRPx"\n", mcb->handle);
                ptr->mem_flag &= ~VIPDRV_MEM_FLAG_MAP_USER_LOGICAL;
                ptr->user_logical = VIP_NULL;
                vipdrv_database_remove(&context->mem_database, i, vip_true_e);
                continue;
            }
            vipdrv_database_unuse(&context->mem_database, i);
        }
    }

#if vpmdENABLE_MMU
    vipdrv_mmu_uninit();
#endif
    vipdrv_allocator_uninit();
    vipdrv_database_destroy(&context->mem_database);

    return status;
}

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
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t pos = 0;
    vipdrv_mem_control_block_t* mcb = VIP_NULL;
    vipdrv_video_mem_handle_t* ptr = VIP_NULL;

    CHECK_VIDEO_MEM_ID_VALID(mem_id);
    pos = vipdrv_mem_id2index(mem_id);

    status = vipdrv_database_use(&context->mem_database, pos, (void**)&mcb);
    if (VIP_SUCCESS != status) {
        PRINTK_E("fail to get kernel logical, wrong handle id=%d\n", pos);
        return VIP_ERROR_FAILURE;
    }
    ptr = mcb->handle;
    if (VIP_NULL != logcial) {
        *logcial = ptr->kerl_logical;
    }
    if (VIP_NULL != physical) {
        if (0x00 == (ptr->mem_flag & VIPDRV_MEM_FLAG_NO_MMU_PAGE)) {
            *physical = ptr->vip_address;
        }
        else {
            *physical = ptr->physical_table[0];
        }
    }
    if (VIP_NULL != size) {
        *size = ptr->size;
    }
    if (VIP_NULL != pid) {
        *pid = mcb->process_id;
    }
    vipdrv_database_unuse(&context->mem_database, pos);

    return status;
}

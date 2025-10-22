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
#include <gc_vip_hardware.h>
#include <gc_vip_kernel.h>
#include <gc_vip_kernel_port.h>
#include <gc_vip_kernel_drv.h>
#include <gc_vip_video_memory.h>
#include <gc_vip_kernel_allocator.h>


#define ENSURE_WRAP_HANDLE_CAPACITY() \
    if (vip_true_e == gckvip_hashmap_full(&context->wrap_hashmap)) { \
        vip_bool_e expand = vip_false_e; \
        status = gckvip_hashmap_expand(&context->wrap_hashmap, &expand); \
        if (VIP_SUCCESS != status) { \
            PRINTK_E("failed, wrap handle overflow\n"); \
            gcGoOnError(VIP_ERROR_IO); \
        } \
    }

#if vpmdENABLE_MMU
#define VIDEO_MEMORY_PHYSICAL_CHECK(physical) \
if ((physical) & 0xffffff0000000000) { \
    PRINTK_E("fail mmu enabled video memory physical=0x%"PRIx64"\n", physical); \
    gcGoOnError(VIP_ERROR_OUT_OF_MEMORY); \
}
#else
#define VIDEO_MEMORY_PHYSICAL_CHECK(physical) \
if ((physical) >= 0xffffffff) { \
    PRINTK_E("fail mmu disabled video memory physical=0x%"PRIx64"\n", physical); \
    gcGoOnError(VIP_ERROR_OUT_OF_MEMORY); \
}
#endif

#if vpmdENABLE_MMU
#define GCKVIP_CHECK_MAP_MMU(node, pysical_base, virtual_tmp)  \
if (GCVIP_MMU_1M_PAGE == node->mmu_page_info.page_type) {   \
    if ((virtual_tmp & gcdMMU_PAGE_1M_MASK) != (pysical_base & gcdMMU_PAGE_1M_MASK)) {  \
        PRINTK_E("fail to map mmu, physical_base=0x%"PRIx64", virutal=0x%x\n",  \
                  pysical_base, virtual_tmp);   \
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);   \
    }   \
}   \
else if (GCVIP_MMU_4K_PAGE == node->mmu_page_info.page_type) {  \
    if ((virtual_tmp & gcdMMU_PAGE_4K_MASK) != (pysical_base & gcdMMU_PAGE_4K_MASK)) {  \
        PRINTK_E("fail to map mmu, physical_base=0x%"PRIx64", virutal=0x%x\n",  \
                  pysical_base, virtual_tmp);   \
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);   \
    }   \
}   \
else {  \
    PRINTK_E("not support mmu page type=%d\n", node->mmu_page_info.page_type);  \
    gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);   \
}

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
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;

    if (VIP_NULL == handle) {
        PRINTK_E("failed to get cpu physical, handle is NULL\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    ptr = (gckvip_video_mem_handle_t *)handle;

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    if (GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP == ptr->memory_type) {
        vip_int32_t offset = vip_virtual - context->heap_virtual;
        if ((offset >= 0) && ((vip_uint32_t)offset < context->heap_size)) {
            /* logical is kernel space address */
            *physical = (phy_address_t)(context->heap_physical + offset);
        }
        else {
            PRINTK_E("fail to get vip physical in heap\n");
            gcGoOnError(VIP_ERROR_FAILURE);
        }
    }
    else
#endif
    {
        /* is dynamic alloc memory */
        if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
            (GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER & ptr->memory_type)) {
            gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
            gcOnError(gckvip_allocator_get_vipphysical(context, node, vip_virtual,
                                                       physical, ptr->memory_type));
        }
        else {
            PRINTK_E("faied to get cpu physical\n");
            gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
        }
    }
    return status;

onError:
    GCKVIP_DUMP_STACK();
    return status;
}
#endif

#if vpmdENABLE_MMU
static vip_status_e gckvip_get_virtual(
    gckvip_context_t *context,
    phy_address_t physical_start,
    phy_address_t physical_end,
    vip_uint32_t *virtual_addr
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t axi_sram_size = 0;
    vip_uint32_t i = 0;
    for (i = 0; i < context->device_count; i++) {
        axi_sram_size += context->axi_sram_size[i];
    }
    /* if memory in AXI SRAM, heap or reserve memory, vip virtual memory has been mapped */
    *virtual_addr = 0;

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
    if (physical_start >= context->heap_physical &&
        physical_start < context->heap_physical + context->heap_size) {
        if (physical_end <= context->heap_physical + context->heap_size) {
            vip_uint32_t offset = (vip_uint32_t)(physical_start - context->heap_physical);
            *virtual_addr = context->heap_virtual + offset;
        }
        else {
            PRINTK_E("physical (0x%"PRIx64" ~ 0x%"PRIx64") overlap with heap physical (0x%"PRIx64" ~ 0x%"PRIx64")",
                physical_start, physical_end, context->heap_physical,
                context->heap_physical + context->heap_size);
            status = VIP_ERROR_FAILURE;
        }
        goto onError;
    }
#endif

    if (physical_start >= context->axi_sram_physical &&
        physical_start < context->axi_sram_physical + axi_sram_size) {
        if (physical_end <= context->axi_sram_physical + axi_sram_size) {
            vip_uint32_t offset = (vip_uint32_t)(physical_start - context->axi_sram_physical);
            *virtual_addr = context->axi_sram_virtual + offset;
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
/*
@brief wrap user memory to VIP virtual address.
@param IN context, the contect of kernel space.
@param IN physical_table Physical address table. should be wraped for VIP hardware.
@param IN size_table The size of physical memory for each physical_table element.
@param IN physical_num The number of physical table element.
@param IN alloc_flag the flag of this video memroy. see gckvip_video_mem_alloc_flag_e.
@param IN memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
@param OUT logical, The user space logical address.
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
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;
    gckvip_dyn_allocate_node_t *node = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint32_t pos = 0;
    vip_uint32_t total_size = 0;
    vip_uint32_t contiguous = vip_true_e;

    if ((VIP_NULL == physical_table) || (VIP_NULL == size_table)) {
        PRINTK_E("failed to wrap user physical, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (physical_num < 1) {
        PRINTK_E("failed to wrap user physical, physical num is %d\n", physical_num);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

#if !vpmdENABLE_MMU
    if (physical_num != 1) {
        PRINTK_E("failed to create buffer from physical mmu is disabled, physical number=%d\n",
                    physical_num);
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    if (physical_table[0] > 0xFFFFFFFF) {
        PRINTK_E("failed to create buffer from physical mmu is disabled, physical address=0x%"PRIx64"\n",
                    physical_table[0]);
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
#endif

    PRINTK_D("user physical, phy_table[0]=0x%"PRIx64", size_table[0]=0x%x, physical_num=%d, memory_type=%d\n",
              physical_table[0], size_table[0], physical_num, memory_type);

    gckvip_os_allocate_memory(sizeof(gckvip_video_mem_handle_t), (void**)&ptr);
    if (VIP_NULL == ptr) {
        PRINTK_E("failed to allocate memory for wrap physical video memory handle\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    ENSURE_WRAP_HANDLE_CAPACITY();
    status = gckvip_hashmap_insert(&context->wrap_hashmap, ptr, &pos);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        PRINTK_E("failed to wrap user physical, hashmap is full\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    node = &ptr->node.dyn_node;

    gckvip_os_zero_memory(node, sizeof(gckvip_dyn_allocate_node_t));

    for (i = 0; i < physical_num; i++) {
        total_size += size_table[i];
    }
    node->size = total_size;

    /* check physical table are contiguous ? */
    for (i = 0; i < physical_num - 1; i++) {
        if ((physical_table[i] + size_table[i]) != physical_table[i + 1]) {
            contiguous = vip_false_e;
            break;
        }
    }

    status = gckvip_allocator_wrap_userphysical(context, physical_table, size_table, physical_num, node, alloc_flag);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to allocate wrap user physical status=%d\n", status);
        gcGoOnError(status);
    }
    #if !vpmdENABLE_MMU
    /* check vip-sram not overlap with user wrap physical address*/
    if ((vip_true_e == contiguous) && (context->vip_sram_size > 0)) {
        if((node->physical_table[0] + node->size > context->vip_sram_address)
                && (context->vip_sram_address >= node->physical_table[0])) {
            PRINTK_E("user wrap physical address 0x%x, size 0x%x, overlap vip sram address 0x%x",
                    node->physical_table[0], node->size, context->vip_sram_address);
            gcGoOnError(VIP_ERROR_FAILURE);
        }
    }
    #endif

#if vpmdENABLE_MMU
    if (vip_true_e == contiguous) {
        vip_uint32_t temp_size[2] = {total_size, 0};
        vip_uint32_t virtual_tmp = 0;
        phy_address_t physical_base = node->physical_table[0];
        phy_address_t physical_end = physical_base + total_size;
        gcOnError(gckvip_get_virtual(context, physical_base, physical_end, &virtual_tmp));
        if (0 == virtual_tmp) {
            /* get page base address */
            gcOnError(gckvip_mmu_map(context, &node->mmu_page_info, node->physical_table,
                                     1, temp_size, GCVIP_MMU_1M_PAGE, &virtual_tmp, 1));
            GCKVIP_CHECK_MAP_MMU(node, physical_base, virtual_tmp);
            node->vip_virtual_address = virtual_tmp;
            PRINTK_D("user physical contiguous handle=0x%"PRPx", base physical=0x%"PRIx64","
                     " virtual=0x%08x, size=0x%x\n",
                     ptr, node->physical_table[0], node->vip_virtual_address, node->size);
        }
        else {
            node->vip_virtual_address = virtual_tmp;
            node->mmu_page_info.page_type = GCVIP_MMU_NONE_PAGE;
            PRINTK_D("user handle=0x%"PRPx", base physical=0x%"PRIx64", virtual=0x%08x, size=0x%x\n",
                     ptr, node->physical_table[0], node->vip_virtual_address, node->size);
        }
    }
    else {
        vip_uint32_t virtual_tmp = 0;
        phy_address_t physical_base = node->physical_table[0];

        /* get page base address */
        gcOnError(gckvip_mmu_map(context, &node->mmu_page_info, node->physical_table,
                                 physical_num, size_table, GCVIP_MMU_4K_PAGE, &virtual_tmp, 1));
        GCKVIP_CHECK_MAP_MMU(node, physical_base, virtual_tmp);
        node->vip_virtual_address = virtual_tmp;
        PRINTK_D("user physical handle=0x%"PRPx", base physical=0x%"PRIx64", physical_num=%d,"
                 " virtual=0x%08x, size=0x%x\n",
                  ptr, node->physical_table[0], physical_num, node->vip_virtual_address, node->size);
    }
#endif
    ptr->memory_type |= GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL;

    if (virtual_addr != VIP_NULL) {
        #if vpmdENABLE_MMU
        *virtual_addr = node->vip_virtual_address;
        #else
        if (vip_true_e == contiguous) {
            *virtual_addr = node->physical_table[0];
        }
        #endif
    }
    else {
        PRINTK_E("failed to wrap physical, virtual pointer is NULL\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    *logical = (void*)node->user_logical;

    *handle = (void*)ptr;

    return status;

onError:
    gckvip_hashmap_remove(&context->wrap_hashmap, ptr);
    if (ptr != VIP_NULL) {
        gckvip_os_free_memory(ptr);
        *handle = VIP_NULL;
    }
    PRINTK_E("fail to mem wrap user phsical\n");
    GCKVIP_DUMP_STACK();
    return status;
}

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
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;
    gckvip_dyn_allocate_node_t *node = VIP_NULL;

    if (VIP_NULL == handle) {
        PRINTK_E("failed to unwrap user physical, handle is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    PRINTK_D("unwrap user physical, handle=0x%"PRPx", virtual=0x%x\n", handle, virtual_addr);

    status = gckvip_hashmap_remove(&context->wrap_hashmap, handle);
    if (VIP_SUCCESS != status) {
        PRINTK_E("failed to unwrap user physical, not found warp handle\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    ptr = (gckvip_video_mem_handle_t*)handle;
    node = &ptr->node.dyn_node;

    status = gckvip_allocator_unwrap_userphysical(context, node);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to allocate unwrap user physical\n");
        gcGoOnError(status);
    }

#if vpmdENABLE_MMU
    if (node->mmu_page_info.page_type != GCVIP_MMU_NONE_PAGE) {
        /* free mmu page table */
        gckvip_mmu_unmap(context, &ptr->node.dyn_node.mmu_page_info);
    }
#endif
    gckvip_os_free_memory(ptr);
    return status;

onError:
    PRINTK_E("failed to un-wrap user physical memroy.\n");
    GCKVIP_DUMP_STACK();
    return status;
}

/*
@brief wrap user memory to VIP virtual address.
@param IN context, the contect of kernel space.
@param IN logical, the user space logical address(handle).
@param IN size, the size of the memory.
@param IN  memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
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
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;
    gckvip_dyn_allocate_node_t *node = VIP_NULL;
    vip_uint32_t pos = 0;

    if (VIP_NULL == logical) {
        PRINTK_E("failed to wrap user memory, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    gckvip_os_allocate_memory(sizeof(gckvip_video_mem_handle_t), (void**)&ptr);
    if (VIP_NULL == ptr) {
        PRINTK_E("failed to allocate memory for video memory ptr\n");
        return VIP_ERROR_FAILURE;
    }

    node = &ptr->node.dyn_node;
    gckvip_os_zero_memory(node, sizeof(gckvip_dyn_allocate_node_t));
    node->size = size;

    ENSURE_WRAP_HANDLE_CAPACITY();
    status = gckvip_hashmap_insert(&context->wrap_hashmap, ptr, &pos);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        PRINTK_E("failed to wrap memory, hashmap is full\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    if (logical != VIP_NULL) {
        /* get physical_table/size_table/physical_num */
        gcOnError(gckvip_allocator_wrap_usermemory(context, logical, memory_type, node));

        #if vpmdENABLE_MMU
        {
            /* mapping VIP MMU page table */
            vip_uint8_t page_type = GCVIP_MMU_4K_PAGE;
            phy_address_t physical_base = 0;
            vip_uint32_t virtual_tmp = 0;
            if (0 == node->physical_num) {
                PRINTK_E("fail map user memory, physical num is 0\n");
                gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }
            physical_base = node->physical_table[0];

            if (1 == node->physical_num) {
                phy_address_t physical_end = physical_base + node->size;
                gcOnError(gckvip_get_virtual(context, physical_base, physical_end, &virtual_tmp));
            }

            if (0 == virtual_tmp) {
                if ((1 == node->physical_num) && (node->size_table[0] >= gcdMMU_PAGE_1M_SIZE)) {
                    page_type = GCVIP_MMU_1M_PAGE;
                }
                gcOnError(gckvip_mmu_map(context, &node->mmu_page_info, node->physical_table,
                    node->physical_num, node->size_table, page_type,
                    &node->vip_virtual_address, 1));
                GCKVIP_CHECK_MAP_MMU(node, physical_base, node->vip_virtual_address);
            }
            else {
                node->vip_virtual_address = virtual_tmp;
                node->mmu_page_info.page_type = GCVIP_MMU_NONE_PAGE;
                PRINTK_D("user handle=0x%"PRPx", base physical=0x%"PRIx64", virtual=0x%08x, size=0x%x\n",
                    ptr, node->physical_table[0], node->vip_virtual_address, node->size);
            }
        }
        #else
        VIDEO_MEMORY_PHYSICAL_CHECK(node->physical_table[0]);
        node->vip_virtual_address = (vip_uint32_t)node->physical_table[0];
        #endif

        ptr->memory_type |= GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_LOGICAL;
    }
    else {
        PRINTK_E("failed to wrap user memory, logical is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    if (virtual_addr != VIP_NULL) {
        *virtual_addr = node->vip_virtual_address;
    }
    else {
        PRINTK_E("failed to wrap memory, virtual pointer is NULL\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    PRINTK_D("wrap user memory handle=0x%"PRPx", logical=0x%"PRPx", physial base=0x%"PRIx64", "
             "virtual=0x%08x, size=0x%x, mem_type=0x%x\n", ptr, logical,
             node->physical_table[0], node->vip_virtual_address, node->size, ptr->memory_type);

    *handle = (void*)ptr;

    return status;

onError:
    gckvip_hashmap_remove(&context->wrap_hashmap, ptr);
    if (node->physical_table != VIP_NULL) {
        gckvip_os_free_memory(node->physical_table);
        node->physical_table = VIP_NULL;
    }
    if (ptr != VIP_NULL) {
        gckvip_os_free_memory((void*)ptr);
        *handle = VIP_NULL;
    }
    GCKVIP_DUMP_STACK();
    return status;
}

/*
@brief un-wrap user memory to VIP virtual address.
@param context, the contect of kernel space.
@param handle, the handle of video memory.
@param virtual_addr, VIP virtual address.
*/
vip_status_e gckvip_mem_unwrap_usermemory(
    gckvip_context_t *context,
    void *handle,
    vip_uint32_t virtual_addr
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;
    gckvip_dyn_allocate_node_t *node = VIP_NULL;

    if (VIP_NULL == handle) {
        PRINTK_E("failed to unwrap user memory, handle is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    PRINTK_D("unwrap user memory, handle=0x%"PRPx", virtual=0x%x\n", handle, virtual_addr);

    status = gckvip_hashmap_remove(&context->wrap_hashmap, handle);
    if (VIP_SUCCESS != status) {
        PRINTK_E("failed to unwrap memory, not found warp handle\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    ptr = (gckvip_video_mem_handle_t*)handle;
    node = &ptr->node.dyn_node;
#if vpmdENABLE_MMU
    /* free mmu page table */
    gckvip_mmu_unmap(context, &ptr->node.dyn_node.mmu_page_info);
#endif
    /* unwrap user memory */
    if (GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_LOGICAL & ptr->memory_type) {
        gckvip_allocator_unwrap_usermemory(context, node);
    }

    gckvip_os_free_memory(ptr);

    return status;

onError:
    PRINTK_E("failed to un-wrap memroy.\n");
    GCKVIP_DUMP_STACK();
    return status;
}

#if vpmdENABLE_CREATE_BUF_FD
/*
@brief un-wrap user memory from fd to VIP virtual address.
@param context, the contect of kernel space.
@param fd the user memory file descriptor.
@param size, the size of of user memory should be unwraped.
@param memory_type The type of this VIP buffer memory. see vip_buffer_memory_type_e.
       only support DMA_BUF now.
@param virtual_addr, VIP virtual address.
@param handle, the handle of video memory.
*/
vip_status_e gckvip_mem_wrap_userfd(
    gckvip_context_t *context,
    vip_uint32_t fd,
    vip_uint32_t size,
    vip_uint32_t memory_type,
    vip_uint32_t *virtual_addr,
    vip_uint8_t **logical,
    void **handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;
    gckvip_dyn_allocate_node_t *node = VIP_NULL;
    vip_uint32_t pos = 0;

    gcIsNULL(virtual_addr);
    gckvip_os_allocate_memory(sizeof(gckvip_video_mem_handle_t), (void**)&ptr);
    if (VIP_NULL == ptr) {
        PRINTK_E("failed to allocate memory for video memory ptr\n");
        return VIP_ERROR_FAILURE;
    }

    node = &ptr->node.dyn_node;
    gckvip_os_zero_memory(node, sizeof(gckvip_dyn_allocate_node_t));
    node->size = size;

    ENSURE_WRAP_HANDLE_CAPACITY();
    status = gckvip_hashmap_insert(&context->wrap_hashmap, ptr, &pos);
    if (VIP_ERROR_OUT_OF_RESOURCE == status) {
        PRINTK_E("failed to wrap fd, hashmap is full\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    gcOnError(gckvip_allocator_wrap_userfd(context, fd, memory_type, node));

#if vpmdENABLE_MMU
    {
        /* mapping VIP MMU page table */
        vip_uint8_t page_type = GCVIP_MMU_4K_PAGE;
        phy_address_t physical_base = node->physical_table[0];
        vip_uint32_t virtual_tmp = 0;

        if (1 == node->physical_num) {
            phy_address_t physical_end = physical_base + node->size;
            gcOnError(gckvip_get_virtual(context, physical_base, physical_end, &virtual_tmp));
        }

        if (0 == virtual_tmp) {
            if ((1 == node->physical_num) && (node->size_table[0] >= gcdMMU_PAGE_1M_SIZE)) {
                page_type = GCVIP_MMU_1M_PAGE;
            }
            gcOnError(gckvip_mmu_map(context, &node->mmu_page_info, node->physical_table,
                node->physical_num, node->size_table, page_type,
                &node->vip_virtual_address, 1));
            GCKVIP_CHECK_MAP_MMU(node, physical_base, node->vip_virtual_address);
            *virtual_addr = node->vip_virtual_address;
        }
        else {
            node->vip_virtual_address = virtual_tmp;
            node->mmu_page_info.page_type = GCVIP_MMU_NONE_PAGE;
            PRINTK_D("user handle=0x%"PRPx", base physical=0x%"PRIx64", virtual=0x%08x, size=0x%x\n",
                ptr, node->physical_table[0], node->vip_virtual_address, node->size);
        }
    }
#else
    if (node->physical_num != 1) {
        PRINTK_E("failed to wrap user fd memory, physical not contiguous and mmu disabled\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }
    *virtual_addr = node->physical_table[0];
    node->mem_flag |= GCVIP_MEM_FLAG_NO_MMU_PAGE;
#endif

    ptr->memory_type |= GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF;

    PRINTK_D("wrap user fd=%d, physial base=0x%"PRIx64", "
             "virtual=0x%08x, size=0x%x, mem_type=0x%x\n", fd, \
             node->physical_table[0], node->vip_virtual_address, node->size, ptr->memory_type);

    *handle = (void*)ptr;
    *logical = node->user_logical;

    return status;

onError:
    gckvip_hashmap_remove(&context->wrap_hashmap, ptr);
    if ((node != VIP_NULL) && (node->physical_table != VIP_NULL)) {
        gckvip_os_free_memory(node->physical_table);
        node->physical_table = VIP_NULL;
    }
    if (ptr != VIP_NULL) {
        gckvip_os_free_memory(ptr);
        *handle = VIP_NULL;
    }
    GCKVIP_DUMP_STACK();
    return status;
}

/*
@brief un-wrap user memory to VIP virtual address.
@param context, the contect of kernel space.
@param handle, the handle of video memory.
@param virtual_addr, VIP virtual address.
*/
vip_status_e gckvip_mem_unwrap_userfd(
    gckvip_context_t *context,
    void *handle,
    vip_uint32_t virtual_addr
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;
    gckvip_dyn_allocate_node_t *node = VIP_NULL;

    if (VIP_NULL == handle) {
        PRINTK_E("failed to unwrap user fd, handle is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    PRINTK_D("unwrap user fd, handle=0x%"PRPx", virtual=0x%x\n", handle, virtual_addr);
    status = gckvip_hashmap_remove(&context->wrap_hashmap, handle);
    if (VIP_SUCCESS != status) {
        PRINTK_E("failed to unwrap fd, not found warp handle\n");
        gcGoOnError(VIP_ERROR_IO);
    }

    ptr = (gckvip_video_mem_handle_t*)handle;
    node = &ptr->node.dyn_node;

    /* free mmu page table */
#if vpmdENABLE_MMU
    if (0x00 == (node->mem_flag & GCVIP_MEM_FLAG_NO_MMU_PAGE)) {
        gckvip_mmu_unmap(context, &ptr->node.dyn_node.mmu_page_info);
    }
#endif

    /* unwrap user memory */
    if (GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF & ptr->memory_type) {
        gckvip_allocator_unwrap_userfd(context, node);
    }

    gckvip_os_free_memory(ptr);
    return status;

onError:
    PRINTK_E("failed to un-wrap user fd\n");
    GCKVIP_DUMP_STACK();
    return status;
}
#endif

/*
@brief initialize video memory heap.
@param context, the contect of kernel space.
*/
#if vpmdENABLE_VIDEO_MEMORY_HEAP
vip_status_e gckvip_mem_heap_init(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    phy_address_t physical = 0;
    phy_address_t align_physical = 0;
    void *mem = VIP_NULL;
    vip_uint32_t size = 0;
    vip_int32_t diff = 0;
    vip_uint8_t *logical = VIP_NULL;

    PRINTK_D("video memory heap base physical=0x%"PRIx64", user logical=0x%"PRPx", "
             "kernel logical=0x%"PRPx", size=0x%x bytes\n",
             context->heap_physical, context->heap_user, context->heap_kernel, context->heap_size);

    VIDEO_MEMORY_PHYSICAL_CHECK(context->heap_physical + context->heap_size);

    context->heap_virtual = (vip_uint32_t)context->heap_physical; /* MMU is disabled */
    mem = context->heap_kernel;
    physical = context->heap_physical;
    size = context->heap_size;
    align_physical = GCVIP_ALIGN(physical, gcdVIP_MEMORY_ALIGN_SIZE);
    diff = (vip_uint32_t)(align_physical - physical);
    logical = (vip_uint8_t*)mem;

    if (diff > 0) {
        physical = align_physical;
        logical = (vip_uint8_t*)mem + diff;
        size -= diff;
        PRINTK_E("video memory auto alignment 256bytes, org_logical=0x%"PRPx","
                 "org_physical=0x%"PRIx64," alig_log=0x%"PRPx", align_physical=0x%"PRIx64"\n",
                 mem, context->heap_physical, logical, physical);
    }

    status = gckvip_heap_construct(&context->video_mem_heap, size, logical, physical);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to construct video memory heap\n");
        gcGoOnError(VIP_ERROR_IO);
    }

onError:
    return status;
}

/*
@brief destory video memory heap.
@param context, the contect of kernel space.
*/
vip_status_e gckvip_mem_heap_destroy(
    gckvip_context_t *context
    )
{
    PRINTK_D("video memory heap destroy....\n");
    gckvip_heap_destroy(&context->video_mem_heap);

    return VIP_SUCCESS;
}
#endif

/*
@brief Get vip accessible kernel space logical address corresponding to the physical address.
@param context, the contect of kernel space.
@param handle, the handle of video memory.
@param IN physical, the physical address or VIP's virtual of video.
@param IN vip_virtual, indicate the physical is VIP's virtual or not.
@param OUT logical, kerenl space logical address.
*/
vip_status_e gckvip_mem_get_kernellogical(
    gckvip_context_t *context,
    void *handle,
    vip_uint32_t physical,
    vip_bool_e vip_virtual,
    void** logical
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    vip_int64_t offset = -1;
#endif
    if (VIP_NULL == handle){
        PRINTK_E("failed to get kernel logical, hanlde is NULL, physical=0x%08x, virtual=0x%08x\n",
                 physical, vip_virtual);
        gcGoOnError(VIP_ERROR_IO);
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
#if vpmdENABLE_MMU
    if (vip_false_e == vip_virtual) {
    #endif
        if (((phy_address_t)physical >= context->heap_physical) &&
            ((phy_address_t)physical < (context->heap_physical + context->heap_size))) {
            offset = (phy_address_t)physical - context->heap_physical;
        }
    #if vpmdENABLE_MMU
    }
    else {
        if ((physical >= context->heap_virtual) &&
                 (physical < (context->heap_virtual + context->heap_size))){
            offset = physical - context->heap_virtual;
        }
    }
    #endif
    if ((offset >= 0) && ((vip_uint32_t)offset < context->heap_size)) {
        *logical = (void*)((vip_uint8_t *)context->heap_kernel + offset);
    }
    else
#endif
    {
        gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t *)handle;
        #if vpmdENABLE_MMU
        if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
            (GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER & ptr->memory_type)) {
            gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
            gcOnError(gckvip_allocator_get_kernellogical(context, node, physical,
                                                         logical, ptr->memory_type));
        }
        else {
            PRINTK_E("failed to get kernel logical address, physical=0x%x, memory_type=0x%x\n",
                      physical, ptr->memory_type);
            status = VIP_ERROR_OUT_OF_MEMORY;
            GCKVIP_DUMP_STACK();
        }
        #else
        if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
            (GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF & ptr->memory_type)) {
            gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
            vip_int64_t offset_tmp = -1;
            if (((phy_address_t)physical >= node->physical_table[0]) &&
                ((phy_address_t)physical < (node->physical_table[0] + node->size))) {
                offset_tmp = (phy_address_t)physical - node->physical_table[0];
            }

            if ((offset_tmp >= 0) && ((vip_uint32_t)offset_tmp < node->size)) {
                *logical = (void*)((vip_uint8_t *)node->kerl_logical + offset_tmp);
            }
        }
        #endif
    }

onError:
    return status;
}

/*
@brief Map user space logical address.
@param IN context, the contect of kernel space.
@param IN handle, the handle of video memory.
@param IN physical, physical address or virtual address.
@param IN vip_virtual, indicate the physical is VIP's virtual or not.
@param OUT logical, user space logical address mapped.
*/
vip_status_e gckvip_mem_map_userlogical(
    gckvip_context_t *context,
    void *handle,
    vip_uint64_t physical,
    vip_bool_e vip_virtual,
    void** logical
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t *)handle;
    if (VIP_NULL == handle) {
        PRINTK_E("failed to get user logical, handle is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    if (VIP_NULL == logical) {
        PRINTK_E("failed to get user logical, logical is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    if (GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP == ptr->memory_type) {
        vip_int64_t offset = -1;
        #if vpmdENABLE_MMU
        if (vip_false_e == vip_virtual) {
        #endif
            if ((physical >= context->heap_physical) &&
                (physical < (context->heap_physical + context->heap_size))) {
                offset = physical - context->heap_physical;
            }
        #if vpmdENABLE_MMU
        }
        else {
            if ((physical >= context->heap_virtual) &&
                (physical < (context->heap_virtual + context->heap_size))) {
                offset = physical - context->heap_virtual;
            }
        }
        #endif
        if ((offset >= 0) && ((vip_uint32_t)offset < context->heap_size)) {
            *logical = (void*)((vip_uint8_t *)context->heap_user + offset);
        }
        else {
            PRINTK_E("fail to map heap userlogical, heap physical base=0x%"PRIx64", "
                "virtual=0x%08x, size=0x%x, cur physical=0x%"PRIx64", is_virtual=%s\n",
                 context->heap_physical, context->heap_virtual, context->heap_size, physical,
                 vip_virtual? " ture" : "false");
            gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
        }
    }
    else
#endif
    {
        if (GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL & ptr->memory_type) {
        #if defined(LINUX)
            gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
            gcOnError(gckvip_allocator_map_userlogical(context, node, logical));
        #else
            PRINTK_W("not support wrap user phyiscal user logical\n");
            *logical = VIP_NULL;
        #endif
            return VIP_SUCCESS;
        }
        else if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
            (GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF & ptr->memory_type)) {
        #if defined(LINUX)
            gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
            gcOnError(gckvip_allocator_map_userlogical(context, node, logical));
        #else
            gckvip_mem_get_userlogical(context, handle, physical, vip_virtual, logical);
        #endif
        }
        else {
            PRINTK_E("faied to get map logical address, physical=0x%x\n", physical);
            gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
        }
    }

    return status;
onError:
    GCKVIP_DUMP_STACK();
    return status;
}

/*
@brief Map user space logical address.
@param IN context, the contect of kernel space.
@param IN handle, the handle of video memory.
*/
vip_status_e gckvip_mem_unmap_userlogical(
    gckvip_context_t *context,
    void *handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t *)handle;
    if (VIP_NULL == handle) {
        PRINTK_E("failed to get user logical, handle is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
        (GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF & ptr->memory_type)) {
    #if defined(LINUX)
        gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
        gcGoOnError(gckvip_allocator_unmap_userlogical(context, node));
    #endif
    }

onError:
    return status;
}

/*
@brief Get vip accessible user space logical address corresponding to the physical address.
@param context, the contect of kernel space.
@param handle, the handle of video memory.
@param IN physical, the physical address of video memory.
@param IN vip_virtual, indicate the physical is VIP's virtual or not.
@param OUT logical, the user space logical.
*/
vip_status_e gckvip_mem_get_userlogical(
    gckvip_context_t *context,
    void *handle,
    phy_address_t physical,
    vip_bool_e vip_virtual,
    void** logical
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    vip_int64_t offset = -1;
#endif

    if (VIP_NULL == handle) {
        PRINTK_E("failed to get user logical, handle is NULL\n");
        gcGoOnError(VIP_ERROR_IO);
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    #if vpmdENABLE_MMU
    if (vip_false_e == vip_virtual) {
    #endif
        if ((physical >= context->heap_physical) &&
            (physical < (context->heap_physical + context->heap_size))) {
            offset = physical - context->heap_physical;
        }
    #if vpmdENABLE_MMU
    }
    else {
        if ((physical >= context->heap_virtual) &&
            (physical < (context->heap_virtual + context->heap_size))) {
            offset = physical - context->heap_virtual;
        }
    }
    #endif
    if ((offset >= 0) && ((vip_uint32_t)offset < context->heap_size)) {
        *logical = (void*)((vip_uint8_t *)context->heap_user + offset);
    }
    else
#endif
    {
        gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t *)handle;
        #if vpmdENABLE_MMU
        if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
            (GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER & ptr->memory_type)) {
            gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
            gcOnError(gckvip_allocator_get_userlogical(context, node, physical,
                                                       logical, ptr->memory_type));
        }
        else {
            PRINTK_E("failed to get user logical address, physical=0x%x\n", physical);
            gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
        }
        #else
        if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
            (GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF & ptr->memory_type)) {
            gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
            vip_int64_t offset_tmp = -1;
            if ((physical >= node->physical_table[0]) &&
                (physical < (node->physical_table[0] + node->size))) {
                offset_tmp = physical - node->physical_table[0];
            }
            if ((offset_tmp >= 0) && ((vip_uint32_t)offset_tmp < node->size)) {
                *logical = (void*)((vip_uint8_t *)node->user_logical + offset_tmp);
            }
        }
        #endif
    }
    return status;

onError:
    GCKVIP_DUMP_STACK();
    return status;
}

/*
@brief Get vip virtual address. vip virtual is equal to physical when MMU is disabled.
@param context, the contect of kernel space.
@param handle, the handle of video memory.
@param IN physical, the physical address of video memory.
@param OUT virtual, the virtual address of VIP.
*/
vip_status_e gckvip_mem_get_vipvirtual(
    gckvip_context_t *context,
    void *handle,
    phy_address_t physical,
    phy_address_t *virtual_addr
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (VIP_NULL == virtual_addr) {
        PRINTK_E("virtual is MULL, failed\n");
        gcGoOnError(VIP_ERROR_IO);
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    if ((physical >= context->heap_physical) &&
        (physical < (context->heap_physical + context->heap_size))) {
        /* is reserve memory */
        #if vpmdENABLE_MMU
        vip_int64_t offset = -1;
        offset = (vip_int64_t)(physical - context->heap_physical);
        if ((offset >= 0) && ((vip_uint32_t)offset < context->heap_size)) {
            *virtual_addr = context->heap_virtual + (vip_uint32_t)offset;
        }
        #else
        *virtual_addr = physical;
        #endif
    }
    else
#endif
    {
        #if vpmdENABLE_MMU
        gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t *)handle;
        if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
            (GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER & ptr->memory_type)) {
            *virtual_addr = (vip_uint32_t)physical;
        }
        else {
            PRINTK_E("faied to get virtual address, memory_type=0x%x, physical=0x%"PRIx64"\n",
                      ptr->memory_type, physical);
            GCKVIP_DUMP_STACK();
            gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
        }
        #else
        *virtual_addr = physical;
        #endif
    }

onError:
    return status;
}

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
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint8_t heap_alloc = 0;
    vip_uint32_t physical_tmp = 0;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;
    phy_address_t phy_addr = 0;

    if ((VIP_NULL == memory) || (VIP_NULL == context) ||
        (VIP_NULL == physical) || (VIP_NULL == handle)) {
        PRINTK_E("failed to allocate memory, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    if (0 == align) {
        PRINTK_E("failed to allocate memory, align=0\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    PRINTK_D("video memory alloc_flag=0x%x, align=0x%x, size=0x%x\n", alloc_flag, align, size);

    status = gckvip_os_lock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock video memory mutex\n");
        gcOnError(VIP_ERROR_FAILURE);
    }

#if vpmdENABLE_MMU
    #if vpmdENABLE_VIDEO_MEMORY_HEAP
    if ((gckvip_heap_capability(&context->video_mem_heap) & alloc_flag) == alloc_flag) {
        gckvip_heap_node_t *node = (gckvip_heap_node_t *)gckvip_heap_alloc(&context->video_mem_heap,
                                                                           size, memory, &phy_addr,
                                                                           align);
        if (node != VIP_NULL) {
            gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_video_mem_handle_t), (void**)&ptr));
            ptr->memory_type = GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP;
            ptr->node.heap_node.alloc_handle = node;
            ptr->node.heap_node.size= size;
            ptr->node.heap_node.physical = phy_addr;
            *handle = (void*)ptr;
            heap_alloc = 1;
        }
    }
    #endif
    if (!heap_alloc) {
        gckvip_dyn_allocate_node_t *node = VIP_NULL;
        gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_video_mem_handle_t), (void**)&ptr));
        gckvip_os_zero_memory(&ptr->node.dyn_node, sizeof(gckvip_dyn_allocate_node_t));
        ptr->memory_type = GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC;
        ptr->node.dyn_node.size = GCVIP_ALIGN(size, align);

        node = &ptr->node.dyn_node;

        /* get physical table and size table */
        gcOnError(gckvip_allocator_dyn_alloc(context, node, align, alloc_flag));

        /* check base address is aligned and overflow */
        if ((node->physical_num > 0) && (node->physical_table != VIP_NULL)) {
            vip_uint32_t i = 0;
            for (i = 0; i < node->physical_num; i++) {
                if ((node->physical_table[i] & (align - 1)) != 0) {
                    PRINTK_E("dyn allocate vido memory physical not align to %d, page_index=%d, "
                             "physical=0x%"PRIx64"\n", align, i, node->physical_table[i]);
                    gcGoOnError(VIP_ERROR_FAILURE);
                }
                VIDEO_MEMORY_PHYSICAL_CHECK(node->physical_table[i]);
                if ((alloc_flag & GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR) &&
                    (node->physical_table[i] & 0xFFFFFFFF00000000ULL)) {
                    PRINTK_E("failed to allocate, physical=0x%"PRIx64" is larger than 4G, alloc_flag=0x%x\n",
                             node->physical_table[i], alloc_flag);
                    gcGoOnError(VIP_ERROR_FAILURE);
                }
                #if ENABLE_MMU_MAPPING_LOG
                PRINTK("index=%d, physical=0x%"PRIx64", size=0x%x\n", i, node->physical_table[i], node->size_table[i]);
                #endif
            }
        }
        else {
            PRINTK_E("physical num is 0 or physical table is NULL\n");
            gcGoOnError(VIP_ERROR_FAILURE);
        }

        if (0x00 == (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE)) {
            /* mapped VIP virtual */
            vip_uint8_t page_type = 0;
            phy_address_t physical_base = 0;
            vip_uint8_t writable = 1;
            if ((alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MMU_PAGE_READ) &&
                !(alloc_flag & GCVIP_VIDEO_MEM_ALLOC_MMU_PAGE_WRITE)) {
                writable = 0;
            }
            if ((node->mem_flag & GCVIP_MEM_FLAG_1M_CONTIGUOUS) ||
                (node->mem_flag & GCVIP_MEM_FLAG_CONTIGUOUS) ||
                (node->mem_flag & GCVIP_MEM_FLAG_CMA)) {
                vip_uint32_t total_size = 0;
                vip_uint32_t k = 0;
                for (k = 0; k < node->physical_num; k++) {
                    total_size += node->size_table[k];
                }
                if (total_size >= gcdMMU_PAGE_1M_SIZE) {
                    page_type = GCVIP_MMU_1M_PAGE;
                }
                else {
                    page_type = GCVIP_MMU_4K_PAGE;
                }
            }
            else {
                page_type = GCVIP_MMU_4K_PAGE;
            }
            physical_base = node->physical_table[0];
            gcOnError(gckvip_mmu_map(context, &node->mmu_page_info, node->physical_table,
                                            node->physical_num, node->size_table, page_type,
                                            &node->vip_virtual_address, writable));
            GCKVIP_CHECK_MAP_MMU(node, physical_base, node->vip_virtual_address);
        }
        else {
            node->mem_flag |= GCVIP_MEM_FLAG_NO_MMU_PAGE;
            if (0x00 == (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS)) {
                PRINTK_E("failed to allocate, NO MMU page alloc flag only support contiguous alloc flag\n");
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            if (node->physical_num != 1) {
                PRINTK_E("failed to allocate, NO MMU page alloc flag  and contiguous,"
                         "physical_num must is  1, then %d\n",
                         node->physical_num);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            node->vip_virtual_address = (vip_uint32_t)node->physical_table[0];
            PRINTK_D("dyn allocate video memory contiguous and no mmu page, physical=0x%x\n",
                      node->vip_virtual_address);
        }

        phy_addr = node->physical_table[0];
        physical_tmp = ptr->node.dyn_node.vip_virtual_address;
        *memory = (void*)ptr->node.dyn_node.kerl_logical;
        *handle = (void*)ptr;
    }
#else
    #if vpmdENABLE_VIDEO_MEMORY_HEAP
    if ((gckvip_heap_capability(&context->video_mem_heap) & alloc_flag) == alloc_flag) {
        gckvip_heap_node_t *node = (gckvip_heap_node_t *)gckvip_heap_alloc(&context->video_mem_heap,
                                                                           size, memory, &phy_addr, align);
        if (node != VIP_NULL) {
            gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_video_mem_handle_t), (void**)&ptr));
            ptr->memory_type = GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP;
            ptr->node.heap_node.alloc_handle = node;
            ptr->node.heap_node.size= size;
            ptr->node.heap_node.physical = phy_addr;
            *handle = (void*)ptr;
            heap_alloc = 1;
        }
    }
    #endif
    if (!heap_alloc)
    {
        gckvip_dyn_allocate_node_t *node = VIP_NULL;
        gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_video_mem_handle_t), (void**)&ptr));
        gckvip_os_zero_memory(&ptr->node.dyn_node, sizeof(gckvip_dyn_allocate_node_t));
        ptr->memory_type = GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC;
        ptr->node.dyn_node.size = GCVIP_ALIGN(size, align);

        node = &ptr->node.dyn_node;

        /* get physical address */
        gcOnError(gckvip_allocator_dyn_alloc(context, node, align, alloc_flag));

        /* check base address is aligned */
        if ((1 == node->physical_num) && (node->physical_table != VIP_NULL)) {
            if ((node->physical_table[0] & (align - 1)) != 0) {
                PRINTK_E("dyn allocate vido memory physical not align to 0x%x, physical=0x%"PRIx64"\n",
                          align, node->physical_table[0]);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
            VIDEO_MEMORY_PHYSICAL_CHECK(node->physical_table[0]);
        }
        else {
            PRINTK_E("physical num is larger than 1, is %d or physical table is NULL\n",
                     node->physical_num);
            gcGoOnError(VIP_ERROR_FAILURE);
        }

        phy_addr = node->physical_table[0];
        physical_tmp = (vip_uint32_t)node->physical_table[0];
        *memory = (void *)ptr->node.dyn_node.kerl_logical;
        *handle = (void*)ptr;
        #if !vpmdENABLE_MMU
        /* check vip-sram not overlap with video memory heap */
        if (context->vip_sram_size > 0) {
            vip_uint32_t tmp_size = GCVIP_ALIGN(size, align);
            if ((context->vip_sram_address < (physical_tmp + tmp_size)) &&
                (context->vip_sram_address >= physical_tmp)) {
                PRINTK_E("fail vip-sram address overlap with video memory heap, vip-sram=0x%08x, "
                         "vido_phy=0x%"PRIx64", size=0x%x\n", context->vip_sram_address, *physical, tmp_size);
                gcGoOnError(VIP_ERROR_FAILURE);
            }
        }
        #endif
    }

    if (phy_addr & 0xFFFFFFFF00000000ULL) {
        PRINTK_E("failed to allocate memory, MMU disabled, physical=0x%"PRIx64"\n", phy_addr);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
#endif

    if (*handle == VIP_NULL) {
        PRINTK_E("failed to allocate video memory from heap, size=0x%x\n", size);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    else {
        phy_address_t virtual_addr = 0;
        if (heap_alloc) {
            /* phy_addr is physical address in DDR, virtual is VIP's virtual */
            if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE) {
                #if vpmdENABLE_VIDEO_MEMORY_HEAP
                ptr->node.heap_node.mem_flag |= GCVIP_MEM_FLAG_NO_MMU_PAGE;
                #endif
                virtual_addr = phy_addr;
            }
            else {
                gcOnError(gckvip_mem_get_vipvirtual(context, *handle, phy_addr, &virtual_addr));
            }
        }
        else {
            virtual_addr = physical_tmp;
        }

        if (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE) {
            *physical = phy_addr;
        }
        else {
            *physical = virtual_addr;
        }

        #if (vpmdENABLE_DEBUG_LOG >= 4)
        {
        vip_uint32_t *logical = VIP_NULL;
        vip_bool_e vip_virtual = (alloc_flag & GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE) ? vip_false_e : vip_true_e;
        gckvip_mem_get_userlogical(context, *handle, virtual_addr, vip_virtual, (void**)&logical);
        PRINTK_D("video memory %s, physical=0x%"PRIx64", virtual=0x%"PRIx64", kernel logical=0x%"PRPx", "
                 "user logical=0x%"PRPx", handle=0x%"PRPx", size=0x%x, align=0x%x, allo_flag=0x%x\n",
                 heap_alloc ? "heap_alloc" : "dyn_alloc", phy_addr, *physical,
                 *memory, logical, *handle, size, align, alloc_flag);
        }
        #endif

        status = VIP_SUCCESS;
    }

#if vpmdENABLE_DEBUGFS
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    if (heap_alloc) {
        gckvip_debug_videomemory_profile_alloc(ptr->node.heap_node.alloc_handle->size);
    }
    else
#endif
    {
        gckvip_debug_videomemory_profile_alloc(ptr->node.dyn_node.size);
    }
#endif

    if ((alloc_flag & GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR) && (phy_addr & 0xFFFFFFFF00000000ULL)) {
        PRINTK_E("failed to allocate, physical=0x%"PRIx64" is larger than 4G, alloc_flag=0x%x\n",
                 phy_addr, alloc_flag);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    if ((*physical & (align - 1)) != 0) {
        PRINTK_E("failed to allocate, physical=0x%08x, not align=0x%x\n", *physical, align);
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    status = gckvip_os_unlock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to unlock video memory mutex\n");
    }

    return status;

onError:
    if (ptr != VIP_NULL) {
        gckvip_os_free_memory(ptr);
    }
    *handle = VIP_NULL;
    *memory = VIP_NULL;
    *physical = 0;
    gckvip_os_unlock_mutex(context->memory_mutex);
    PRINTK_E("failed to allocate video memory from %s, status=%d\n",
             heap_alloc ? "Video MEM heap" : "Dynamic Allocate", status);
    GCKVIP_DUMP_STACK();
    return status;
}

/*
@brief free video memory.
@param context, the contect of kernel space.
@handle the handle of video memory.
*/
vip_status_e gckvip_mem_free_videomemory(
    gckvip_context_t *context,
    void *handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t*)handle;

    status = gckvip_os_lock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock video memory mutex\n");
        return VIP_ERROR_FAILURE;
    }

    PRINTK_D("free video memory handle=0x%"PRPx"\n", handle);

#if vpmdENABLE_DEBUGFS
    gckvip_debug_videomemory_profile_free(handle);
#endif

    if (ptr != VIP_NULL) {
        if (GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP & ptr->memory_type) {
            #if vpmdENABLE_VIDEO_MEMORY_HEAP
            gcOnError(gckvip_heap_free(&context->video_mem_heap, ptr->node.heap_node.alloc_handle));
            #else
            PRINTK_E("fail to free memory, video memory heap has been disable,"
                     "vpmdENABLE_VIDEO_MEMORY_HEAP=0\n");
            #endif
        }
        else if (GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) {
            gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
            /* unmap mmu */
            #if vpmdENABLE_MMU
            if (0x00 == (node->mem_flag & GCVIP_MEM_FLAG_NO_MMU_PAGE)) {
                gcOnError(gckvip_mmu_unmap(context, &node->mmu_page_info));
            }
            #endif
            /* free pages */
            gcOnError(gckvip_allocator_dyn_free(context, node));
        }
        else {
            PRINTK_E("fail to free video memory, error memory type=0x%x\n",ptr->memory_type);
            gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
        }
        gcOnError(gckvip_os_free_memory(ptr));
    }
    else {
        PRINTK_E("mem free video memory parameter is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    gckvip_os_unlock_mutex(context->memory_mutex);

    return status;

onError:
    gckvip_os_unlock_mutex(context->memory_mutex);
    PRINTK_E("fail to free video memory, handle=0x%"PRPx"\n", handle);
    GCKVIP_DUMP_STACK();
    return status;
}

#if vpmdENABLE_FLUSH_CPU_CACHE
/*
@brief flush video memory CPU cache
@param handle the video memory handle
@param physical flush memory start physical address. The physical is VIP's physical address.
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
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_video_mem_handle_t *ptr = VIP_NULL;
    gckvip_context_t *context = gckvip_get_context();

    if (VIP_NULL == handle) {
        PRINTK_E("failed to flush video memory cache, handle is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    ptr = (gckvip_video_mem_handle_t*)handle;

#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_lock_mutex(context->flush_cache_mutex);
#endif
    if ((GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER & ptr->memory_type) ||
        (GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type)) {
        /* flush dynamic video memory CPU cache and  wrape application memory cache */
        gckvip_dyn_allocate_node_t *node = &ptr->node.dyn_node;
        gcOnError(gckvip_alloctor_flush_cache(context, node, physical, logical, size, type));
    }
    else if (GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP & ptr->memory_type) {
        gcOnError(gckvip_os_flush_cache(physical, logical, size, type));
    }
    else {
        PRINTK_E("flush cache, no the memory type=0x%x, handle=0x%"PRPx"\n", ptr->memory_type, handle);
    }
    PRINTK_D("flush cache handle=0x%"PRPx", physical=0x%"PRIx64", logical=0x%"PRPx", type=%s, size=0x%x\n",
              handle, physical, logical, (type==GCKVIP_CACHE_FLUSH) ? "FLUSH" : "INVALID", size);
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_unlock_mutex(context->flush_cache_mutex);
#endif

    return status;

onError:
#if vpmdENABLE_MULTIPLE_TASK
    gckvip_os_unlock_mutex(context->flush_cache_mutex);
#endif
    PRINTK_E("failed to gckvip flush cache, handle=0x%"PRPx", physical=0x%x, size=0x%x\n",
             handle, physical, size);
    return status;
}

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
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_MMU
    gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t*)handle;
    gckvip_context_t *context = gckvip_get_context();
    vip_uint32_t vip_virtual = physical;
    phy_address_t vip_physical = 0;
    vip_uint8_t need_covert = 0;

    if (VIP_NULL == handle) {
        PRINTK_E("failed to flush video memory cache, handle is NULL\n");
        return VIP_ERROR_FAILURE;
    }
    if ((GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC & ptr->memory_type) ||
            (GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER & ptr->memory_type)) {
        if (0x00 == (ptr->node.dyn_node.mem_flag & GCVIP_MEM_FLAG_NO_MMU_PAGE)) {
            need_covert = 1;
        }
    }
    #if vpmdENABLE_VIDEO_MEMORY_HEAP
    else if (GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP & ptr->memory_type) {
            need_covert = 1;
    }
    #endif
    if (need_covert) {
        /* convert vip virtual to vip physical */
        gcOnError(gckvip_mem_get_vipphysical(context, handle, vip_virtual, &vip_physical));
    }
    else {
        PRINTK_E("faied to flush cache, physical = 0x%x, vip_physical = 0x%x, need_covert = %d.\n",
                physical, vip_physical, need_covert);
        gcGoOnError(VIP_ERROR_FAILURE)
    }

    status = gckvip_mem_flush_cache_ext(handle, vip_physical, logical, size, type);

onError:
#else
    status = gckvip_mem_flush_cache_ext(handle, (phy_address_t)physical, logical, size, type);
#endif

    return status;
}
#endif

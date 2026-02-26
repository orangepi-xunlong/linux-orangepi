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
#include <gc_vip_common.h>
#if vpmdENABLE_MMU
#include <gc_vip_kernel.h>
#include <gc_vip_hardware.h>
#include <gc_vip_kernel_mmu.h>
#include <gc_vip_video_memory.h>
#include <gc_vip_kernel_debug.h>
#include <gc_vip_kernel_port.h>
#include <gc_vip_kernel_drv.h>

#define FREE_STLB_ENTRY_VALUE       0x2
#define DEFAULT_STLB_ENTRY_VALUE    0x5
#define RESERVED_1M_PAGE_NUM        1
#define RESERVED_4K_PAGE_NUM        1
#define UINT8_BIT_COUNT             (8 * sizeof(vip_uint8_t))


#define _write_page_entry(page_entry, entry_value) \
        *(vip_uint32_t*)(page_entry) = (vip_uint32_t)(entry_value)

#define _read_page_entry(page_entry)  *(vip_uint32_t*)(page_entry)

#define MMU_PAGE_ALWAYS_WRITABLE 0

typedef struct _gckvip_mmu_table_entry
{
    vip_uint32_t low;
    vip_uint32_t high;
} gckvip_mmu_table_entry_t;

static vip_uint32_t mmu_set_page(
    vip_uint32_t page_address,
    vip_uint32_t page_address_ext,
    vip_uint8_t writable
    )
{
    vip_uint32_t l_align_address = page_address & ~((1 << 12) - 1);
    vip_uint32_t h_align_address = page_address_ext & 0xFF;
    vip_uint32_t entry = l_align_address
                        /* AddressExt */
                        | (h_align_address << 4)
                        /* Writable */
                        #if MMU_PAGE_ALWAYS_WRITABLE
                        | (1 << 2)
                        #else
                        | ((writable & 0x1) << 2)
                        #endif
                        /* Enable exception */
                        | (1 << 1)
                        /* Present */
                        | (1 << 0);
    return entry;
}

/*
@brief allocate MMU pages.
@param context, the contect of kernel space.
@param num_mtlb, the number of MTLB.
@param, num_stlb, the number of STLB.
@param, page_type, the type of this page. GCVIP_MMU_1M_PAGE / GCVIP_MMU_4K_PAGE ...
@param, info, page information
*/
static vip_status_e mmu_get_page_info(
    gckvip_context_t *context,
    vip_int32_t num_mtlb,
    vip_int32_t num_stlb,
    vip_uint8_t page_type,
    gckvip_mmu_page_info_t *info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t page_start = 0, page_end = 0, i = 0;
    vip_uint32_t count = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t mtlb_count = 0;
    vip_uint32_t default_value = DEFAULT_STLB_ENTRY_VALUE;
    volatile vip_uint8_t *bitmap = VIP_NULL;
    volatile vip_uint32_t *stlb_logical = VIP_NULL;

    if ((0 > num_mtlb) || (0 > num_stlb) || (VIP_NULL == info)) {
        PRINTK_E("failed to get stlb, parameter is 0 or tlb < 0\n");
        return VIP_ERROR_IO;
    }

    if (GCVIP_MMU_1M_PAGE == page_type) {
        mtlb_count = gcdMMU_1M_MTLB_COUNT;
        stlb_entry_num = gcdMMU_STLB_1M_ENTRY_NUM;
        bitmap = context->bitmap_1M;
        stlb_logical = (volatile vip_uint32_t*)context->ptr_STLB_1M->logical;
    }
    else if (GCVIP_MMU_4K_PAGE == page_type) {
        mtlb_count = gcdMMU_4K_MTLB_COUNT;
        stlb_entry_num = gcdMMU_STLB_4K_ENTRY_NUM;
        bitmap = context->bitmap_4K;
        stlb_logical = (volatile vip_uint32_t*)context->ptr_STLB_4K->logical;
        if ((default_value & 0x5) == 0x5) {
            vip_uint32_t physical_low = 0;
            vip_uint32_t physical_high = 0;
            phy_address_t physical = context->default_mem.physical;
            if ((0 == (physical & (gcdMMU_PAGE_4K_SIZE - 1))) &&
                (gcdMMU_PAGE_4K_SIZE == context->default_mem.size)) {
                physical_low = (vip_uint32_t)(physical & 0xFFFFFFFF);
                if (physical & 0xFFFFFFFF00000000ULL) {
                    phy_address_t phy_tmp = physical >> 16;
                    physical_high = (vip_uint32_t)(phy_tmp >> 16);
                    if (physical_high > 0xFF) {
                        physical_high = physical_low = 0;
                    }
                }
                default_value = mmu_set_page(physical_low, physical_high, 1);
            }
        }
    }
    else {
        PRINTK_E("free page info, page type=%d error\n", page_type);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    for (i = 0; i < mtlb_count * stlb_entry_num; i++) {
        if ((1 << (i % UINT8_BIT_COUNT)) & bitmap[i / UINT8_BIT_COUNT]) {
            count = 0;
        }
        else {
            if (0 == count) {
                page_start = i;
            }
            count++;
            if (count == (vip_uint32_t)num_stlb) {
                page_end = i;
                break;
            }
        }
    }
    if (count < (vip_uint32_t)num_stlb) {
        PRINTK_E("failed to get stlb, without enough space\n");
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    for (i = page_start; i <= page_end; i++) {
        /* set default value */
        _write_page_entry(stlb_logical + i, default_value);
        /* flag page used */
        bitmap[i / UINT8_BIT_COUNT] |= (1 << (i % UINT8_BIT_COUNT));
    }

    info->mtlb_start = page_start/stlb_entry_num;
    info->stlb_start = page_start%stlb_entry_num;
    info->mtlb_end = page_end/stlb_entry_num;
    info->stlb_end = page_end%stlb_entry_num;
    if (GCVIP_MMU_4K_PAGE == page_type) {
        info->mtlb_start += gcdMMU_1M_MTLB_COUNT;
        info->mtlb_end += gcdMMU_1M_MTLB_COUNT;
    }

    PRINTK_D("get page type=%s, mtlb_num=%d, stlb_num=%d, mtlb: %d ~ %d, stlb: %d ~ %d\n",
             (page_type == GCVIP_MMU_1M_PAGE) ? "1M" : "4K", num_mtlb, num_stlb,
             info->mtlb_start, info->mtlb_end, info->stlb_start, info->stlb_end);

    if (GCVIP_MMU_1M_PAGE == page_type) {
        if (page_end > context->max_1Mpage) {
            context->max_1Mpage = page_end;
        }
    }
    else if (GCVIP_MMU_4K_PAGE == page_type) {
        if (page_end > context->max_4Kpage) {
            context->max_4Kpage = page_end;
        }
    }

onError:
    return status;
}

/*
@brief free MMU page info.
@param context, the contect of kernel space.
@param, page_type, the type of this page. GCVIP_MMU_1M_PAGE / GCVIP_MMU_4K_PAGE ...
@param, page_info, page information
*/
static vip_status_e mmu_free_page_info(
    gckvip_context_t *context,
    vip_uint8_t page_type,
    gckvip_mmu_page_info_t *page_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t page_start = 0, page_end = 0, i = 0;
    volatile vip_uint32_t *stlb_logical = VIP_NULL;
    volatile vip_uint8_t *bitmap = VIP_NULL;

    if (page_info == VIP_NULL) {
        PRINTK_E("failed to free page info\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (GCVIP_MMU_1M_PAGE == page_type) {
        bitmap = context->bitmap_1M;
        stlb_logical = (volatile vip_uint32_t*)context->ptr_STLB_1M->logical;
        page_start = page_info->mtlb_start * gcdMMU_STLB_1M_ENTRY_NUM + page_info->stlb_start;
        page_end = page_info->mtlb_end * gcdMMU_STLB_1M_ENTRY_NUM + page_info->stlb_end;
    }
    else if (GCVIP_MMU_4K_PAGE == page_type) {
        bitmap = context->bitmap_4K;
        stlb_logical = (volatile vip_uint32_t*)context->ptr_STLB_4K->logical;
        page_start = (page_info->mtlb_start - gcdMMU_1M_MTLB_COUNT) * gcdMMU_STLB_4K_ENTRY_NUM \
                   + page_info->stlb_start;
        page_end = (page_info->mtlb_end - gcdMMU_1M_MTLB_COUNT) * gcdMMU_STLB_4K_ENTRY_NUM \
                 + page_info->stlb_end;
    }
    else {
        PRINTK_E("free page info, page type=%d error\n", page_type);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    for (i = page_start; i <= page_end; i++) {
        /* set default value */
        _write_page_entry(stlb_logical + i, FREE_STLB_ENTRY_VALUE);
        /* flag page used */
        bitmap[i / UINT8_BIT_COUNT] &= ~(1 << (i % UINT8_BIT_COUNT));
    }

    PRINTK_D("free page mtlb: %d ~ %d, stlb : %d ~ %d, page type=%s\n",
             page_info->mtlb_start, page_info->mtlb_end, page_info->stlb_start,
             page_info->stlb_end, (page_type == GCVIP_MMU_1M_PAGE) ? "1M" : "4K");

onError:
    return status;
}

/*
@brief initialize all MTLB bits.
@param context, the contect of kernel space.
*/
static vip_status_e mmu_fill_mtlb(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vip_uint32_t stlb_physical = 0;
    volatile vip_uint32_t *mtlb_logical = (volatile vip_uint32_t*)context->MTLB.logical;
    vip_uint32_t mtlb_value = 0;

    /* 0 ~~ 1m_mtlb_num MTLB for 1Mbytes page */
    stlb_physical = context->ptr_STLB_1M->physical;
    for (i = 0; i < gcdMMU_1M_MTLB_COUNT; i++) {
        mtlb_value = stlb_physical
                        /* 1MB page size */
                        | (1 << 3)
                        /* Ignore exception */
                        | (0 << 1)
                        /* Present */
                        | (1 << 0);

        _write_page_entry(mtlb_logical + i, mtlb_value);
        stlb_physical += gcdMMU_STLB_1M_SIZE;
    }

    /* 1m_mtlb_num ~~  (gcdMMU_1M_MTLB_COUNT + gcdMMU_4K_MTLB_COUNT) for 4Kbytes page */
    stlb_physical = context->ptr_STLB_4K->physical;
    for (i = gcdMMU_1M_MTLB_COUNT; i < (gcdMMU_1M_MTLB_COUNT + gcdMMU_4K_MTLB_COUNT); i++) {
        mtlb_value = stlb_physical
                        /* 4KB page size */
                        | (0 << 2)
                        /*Ignore exception */
                        | (0 << 1)
                        /* Present */
                        | (1 << 0);

        _write_page_entry(mtlb_logical + i, mtlb_value);
        stlb_physical += gcdMMU_STLB_4K_SIZE;
    }

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    gckvip_mem_flush_cache_ext(context->MTLB.handle, context->MTLB.physical,
                              context->MTLB.logical, context->MTLB.size, GCKVIP_CACHE_FLUSH);
#endif

    return status;
}

/*
@brief fill stlb.
@param context, the contect of kernel space.
@param info mmu page information.
@param page_type the type of page should be mapped. GCVIP_MMU_1M_PAGE/GCVIP_MMU_4K_PAGE
@param writable  1: indicate that the memory is NPU writable. 0: indicate that is read only memory.
*/
static vip_status_e mmu_fill_stlb(
    gckvip_context_t *context,
    gckvip_mmu_page_info_t *info,
    vip_uint8_t page_type,
    phy_address_t physical,
    vip_uint8_t writable
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    volatile vip_uint32_t *stlb_logical = VIP_NULL;
    vip_uint32_t s_start = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t max_mtlb_num = 0;
    phy_address_t physical_align = 0;
    vip_uint32_t physical_low = 0;
    vip_uint32_t physical_high = 0;
    vip_uint32_t page_size = 0;
    vip_uint32_t phy_start = 0;

    if ((VIP_NULL == context) || (VIP_NULL == info)) {
        PRINTK_E("failed to fill stlb, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    switch (page_type) {
    case GCVIP_MMU_1M_PAGE:
    {
        stlb_entry_num = gcdMMU_STLB_1M_ENTRY_NUM;
        max_mtlb_num = gcdMMU_1M_MTLB_COUNT;
        physical_align = physical & ~gcdMMU_PAGE_1M_MASK;
        page_size = gcdMMU_PAGE_1M_SIZE;
    }
    break;

    case GCVIP_MMU_4K_PAGE:
    {
        stlb_entry_num = gcdMMU_STLB_4K_ENTRY_NUM;
        max_mtlb_num = gcdMMU_1M_MTLB_COUNT + gcdMMU_4K_MTLB_COUNT;
        physical_align = physical & ~gcdMMU_PAGE_4K_MASK;
        page_size = gcdMMU_PAGE_4K_SIZE;
    }
    break;

    default:
        PRINTK_E("failed to fill stlb, not support this page type=%d\n", page_type);
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    physical_low = (vip_uint32_t)(physical_align & 0xFFFFFFFF);
    if (physical_align & 0xFFFFFFFF00000000ULL) {
        /* make 32-bits system happy */
        phy_address_t phy_tmp = physical_align >> 16;
        physical_high = (vip_uint32_t)(phy_tmp >> 16);
        if (physical_high > 0xFF) {
            PRINTK_E("fail to fill stlb physical=0x%"PRIx64", high=0x%x, low=0x%x\n",
                      physical, physical_high, physical_low);
            gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
    }
    else {
        physical_high = 0;
    }
    phy_start = physical_low;
    for (i = info->mtlb_start; i <= info->mtlb_end; i++) {
        vip_uint32_t s_last = (i == info->mtlb_end) ? info->stlb_end : (stlb_entry_num - 1);

        if (i >= max_mtlb_num) {
            PRINTK_E("MMU MTLB size is greater than max mtlb num %d > %d, page type=%d\n",
                      i, max_mtlb_num, page_type);
            gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }

        if (i > info->mtlb_start) {
            s_start = 0;
        }
        else {
            s_start = info->stlb_start;
        }

        if (GCVIP_MMU_1M_PAGE == page_type) {
            stlb_logical = (volatile vip_uint32_t*)((vip_uint8_t*)context->ptr_STLB_1M->logical + \
                           (i * gcdMMU_STLB_1M_SIZE));
        }
        else if (GCVIP_MMU_4K_PAGE == page_type) {
            stlb_logical = (volatile vip_uint32_t*)((vip_uint8_t*)context->ptr_STLB_4K->logical + \
                           ((i - gcdMMU_1M_MTLB_COUNT) * gcdMMU_STLB_4K_SIZE));
        }

        while (s_start <= s_last) {
            vip_uint32_t entry_value = 0;
            entry_value = mmu_set_page(phy_start, physical_high, writable);
            _write_page_entry(stlb_logical + s_start, entry_value);
            #if 0
            if (GCVIP_MMU_1M_PAGE == page_type) {
                PRINTK_D("mtlb: %d, stlb: %d, phy_h=0x%x, phy_l=0x%x, size=0x%x\n",
                          i, s_start, physical_high, phy_start, page_size);
            }
            #endif
            phy_start += page_size;
            s_start++;
        }
    }

    return status;

onError:
    GCKVIP_DUMP_STACK();
    return status;
}

/*
@brief mmap axi sram.
@param context, the contect of kernel space.
*/
static vip_status_e mmu_map_axi_sram(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_mmu_page_info_t info;
    vip_int32_t num_stlb = 0;
    vip_int32_t num_mtlb = 0;
    vip_uint32_t axi_address = context->axi_sram_physical;
    vip_uint32_t phy_start = axi_address & ~gcdMMU_PAGE_1M_MASK;
    vip_uint32_t offset = axi_address - phy_start;
    vip_uint32_t axi_sram_size = 0;
    vip_uint32_t i = 0;

    for (i = 0; i < context->device_count; i++) {
        axi_sram_size += context->axi_sram_size[i];
    }

    num_stlb = (axi_sram_size + gcdMMU_PAGE_1M_SIZE - 1) / gcdMMU_PAGE_1M_SIZE;
    num_mtlb = (axi_sram_size + (1 << gcdMMU_MTLB_SHIFT) - 1) / (1 << gcdMMU_MTLB_SHIFT);
    if (offset > 0) {
        num_stlb += 1; /* offset in page */
    }

    if (num_stlb > 0) {
        PRINTK_D("mapping AIX-SRAM size=0x%x..\n", axi_sram_size);
        gcOnError(mmu_get_page_info(context, num_mtlb, num_stlb, GCVIP_MMU_1M_PAGE, &info));

        context->axi_sram_virtual = (info.mtlb_start << gcdMMU_MTLB_SHIFT) |
                                    (info.stlb_start << gcdMMU_STLB_1M_SHIFT) |
                                    (axi_address & gcdMMU_PAGE_1M_MASK);

        PRINTK_D("axi sram size=0x%xbytes, mtlb: %d  ~ %d, stlb: %d ~ %d, "
                 "physical=0x%08x to virtual=0x%08x\n",
                 axi_sram_size, info.mtlb_start, info.mtlb_end, info.stlb_start,
                 info.stlb_end, axi_address, context->axi_sram_virtual);

        status = mmu_fill_stlb(context, &info, GCVIP_MMU_1M_PAGE, phy_start, 1);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to fill stlb in map AXI SRAM, status=%d.\n", status);
        }
    }

    return status;

onError:
    PRINTK_E("failed to map AXI-SRAM status=%d.\n", status);
    return status;
}

/*
@brief mmap vip sram.
@param context, the contect of kernel space.
*/
static vip_status_e mmu_map_vip_sram(
    gckvip_context_t *context
    )
{
    #define MAP_OFFSET_M      4
    vip_status_e status = VIP_SUCCESS;
    gckvip_mmu_page_info_t info;
    vip_int32_t num_stlb = 0;
    vip_int32_t num_mtlb = 0;
    vip_uint32_t i = 0;
    vip_uint32_t vip_sram_size = 0;

    for (i = 0; i < context->device_count; i++) {
        vip_sram_size = context->vip_sram_size[i] > vip_sram_size ?
                        context->vip_sram_size[i] : vip_sram_size;
    }

    num_stlb = (vip_sram_size + gcdMMU_PAGE_1M_SIZE - 1) / gcdMMU_PAGE_1M_SIZE;
    num_mtlb = (vip_sram_size + (1 << gcdMMU_MTLB_SHIFT) - 1) / (1 << gcdMMU_MTLB_SHIFT);

    if (vip_sram_size <= (12 << 20)) {
        num_stlb = 16;
        num_mtlb = 1;
    }
    else {
        /* align to a mtlb block */
        num_stlb = GCVIP_ALIGN((num_stlb + MAP_OFFSET_M), 16);
    }

    if (num_stlb > 0) {
        PRINTK_D("mapping VIP-SRAM size=0x%x..\n", vip_sram_size);
        gcOnError(mmu_get_page_info(context, num_mtlb, num_stlb, GCVIP_MMU_1M_PAGE, &info));

        context->vip_sram_address = (info.mtlb_start << gcdMMU_MTLB_SHIFT) |
                                    ((info.stlb_start + MAP_OFFSET_M) << gcdMMU_STLB_1M_SHIFT);

        PRINTK_D("vip sram size=0x%xbytes, mtlb : %d ~ %d, stlb : %d ~ %d, virtual=0x%08x\n",
                  vip_sram_size, info.mtlb_start, info.mtlb_end,
                  info.stlb_start, info.stlb_end, context->vip_sram_address);
    }

    return status;

onError:
    PRINTK_E("failed to map VIP-SRAM status=%d.\n", status);
    return status;
}

#if vpmdENABLE_RESERVE_PHYSICAL
static vip_status_e mmu_map_reserve_physical(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    phy_address_t reserve_phy_base = context->reserve_phy_base;
    vip_uint32_t reserve_size = context->reserve_phy_size;
    phy_address_t phy_start = reserve_phy_base & ~gcdMMU_PAGE_1M_MASK;
    vip_uint32_t m_start = 0;
    vip_uint32_t s_start = 0;
    vip_int32_t num_mtlb = 0;
    vip_int32_t num_stlb = 0;
    gckvip_mmu_page_info_t info;
    gckvip_mmu_page_info_t info_tmp;
    vip_uint32_t offset = (vip_uint32_t)(reserve_phy_base - phy_start);

    if (reserve_size > gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE) {
        PRINTK_E("reserve physical size %d is bigger than 1M page size %d\n",
                 reserve_size, gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE);
        return VIP_ERROR_IO;
    }

    /* 16Mbytes per-MTLB, 1Mbytes per-STLB */
    num_stlb = (reserve_size + gcdMMU_PAGE_1M_SIZE - 1) / gcdMMU_PAGE_1M_SIZE;
    num_mtlb = (reserve_size + (1 << gcdMMU_MTLB_SHIFT) - 1) / (1 << gcdMMU_MTLB_SHIFT);
    if (offset > 0) {
        num_stlb += 1; /* offset in page */
    }

    if (num_stlb > 0) {
        PRINTK_D("mapping reserve physical..\n");
        num_stlb += RESERVED_1M_PAGE_NUM;
        gcOnError(mmu_get_page_info(context, num_mtlb, num_stlb, GCVIP_MMU_1M_PAGE, &info));

        m_start = info.mtlb_start;
        s_start = info.stlb_start;
        if ((s_start + RESERVED_1M_PAGE_NUM) >= gcdMMU_STLB_1M_ENTRY_NUM) {
            s_start = 0;
            m_start += 1;
        }
        else {
            s_start += RESERVED_1M_PAGE_NUM;
        }

        context->reserve_virtual = (m_start << gcdMMU_MTLB_SHIFT) |
                                   (s_start << gcdMMU_STLB_1M_SHIFT) |
                                   (reserve_phy_base & gcdMMU_PAGE_1M_MASK);

        info_tmp.mtlb_start = m_start;
        info_tmp.mtlb_end = info.mtlb_end;
        info_tmp.stlb_start = s_start;
        info_tmp.stlb_end = info.stlb_end;
        info_tmp.page_type = GCVIP_MMU_1M_PAGE;
        PRINTK_D("reserve physical=0x%"PRIx64", vip virtual=0x%08x, "
                 "size=0x%x, mtlb : %d ~ %d, stlb : %d ~ %d\n",
                reserve_phy_base, context->reserve_virtual, reserve_size, m_start,
                info_tmp.mtlb_end, s_start, info_tmp.stlb_end);

        status = mmu_fill_stlb(context, &info_tmp, GCVIP_MMU_1M_PAGE, reserve_phy_base, 1);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to fill stlb in map reserve physical, status=%d.\n", status);
        }
    }

    return status;
onError:
    PRINTK_E("failed to map reserve physical status=%d.\n", status);
    return status;
}
#endif

/*
@brief mmap reserve memory heap.
@param context, the contect of kernel space.
*/
#if vpmdENABLE_VIDEO_MEMORY_HEAP
static vip_status_e mmu_map_memory_heap(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    phy_address_t heap_phy_base = context->heap_physical;
    vip_uint32_t heap_size = context->heap_size;
    phy_address_t phy_start = heap_phy_base & ~gcdMMU_PAGE_1M_MASK;
    vip_uint32_t m_start = 0;
    vip_uint32_t s_start = 0;
    vip_int32_t num_mtlb = 0;
    vip_int32_t num_stlb = 0;
    gckvip_mmu_page_info_t info;
    gckvip_mmu_page_info_t info_tmp;
    vip_uint32_t offset = (vip_uint32_t)(heap_phy_base - phy_start);

    if (heap_size > gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE) {
        PRINTK_E("video memory heap size %d is bigger than 1M page size %d\n",
                  heap_size, gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE);
        return VIP_ERROR_IO;
    }

    /* 16Mbytes per-MTLB, 1Mbytes per-STLB */
    num_stlb = (heap_size + gcdMMU_PAGE_1M_SIZE - 1) / gcdMMU_PAGE_1M_SIZE;
    num_mtlb = (heap_size + (1 << gcdMMU_MTLB_SHIFT) - 1) / (1 << gcdMMU_MTLB_SHIFT);
    if (offset > 0) {
        num_stlb += 1; /* offset in page */
    }

    if (num_stlb > 0) {
        PRINTK_D("mapping video memory heap..\n");
        num_stlb += RESERVED_1M_PAGE_NUM;
        gcOnError(mmu_get_page_info(context, num_mtlb, num_stlb, GCVIP_MMU_1M_PAGE, &info));

        m_start = info.mtlb_start;
        s_start = info.stlb_start;
        if ((s_start + RESERVED_1M_PAGE_NUM) >= gcdMMU_STLB_1M_ENTRY_NUM) {
            s_start = 0;
            m_start += 1;
        }
        else {
            s_start += RESERVED_1M_PAGE_NUM;
        }

        context->heap_virtual =  (m_start << gcdMMU_MTLB_SHIFT) |
                                 (s_start << gcdMMU_STLB_1M_SHIFT) |
                                 (heap_phy_base & gcdMMU_PAGE_1M_MASK);

        info_tmp.mtlb_start = m_start;
        info_tmp.mtlb_end = info.mtlb_end;
        info_tmp.stlb_start = s_start;
        info_tmp.stlb_end = info.stlb_end;
        info_tmp.page_type = GCVIP_MMU_1M_PAGE;
        PRINTK_D("video memory heap physical=0x%"PRIx64", vip virtual=0x%08x, "
                 "size=0x%x, mtlb : %d ~ %d, stlb : %d ~ %d\n",
                heap_phy_base, context->heap_virtual, heap_size, m_start,
                info_tmp.mtlb_end, s_start, info_tmp.stlb_end);

        status = mmu_fill_stlb(context, &info_tmp, GCVIP_MMU_1M_PAGE, heap_phy_base, !VIDEO_MEMORY_HEAP_READ_ONLY);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to fill stlb in map video memory heap\n");
        }
    }

    return status;
onError:
    PRINTK_E("failed to map video memory heap status=%d.\n", status);
    return status;
}
#endif


/*
@brief register to set up MMU
@param context, the contect of kernel space.
*/
static vip_status_e gckvip_mmu_pdmode_setup(
    gckvip_context_t *context,
    gckvip_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t mtlb_low = 0, mtlb_high = 0;

    mtlb_low = (vip_uint32_t)(context->MTLB.physical & 0xFFFFFFFF);
    mtlb_high = (vip_uint32_t)(context->MTLB.physical >> 32);

    status = gckvip_write_register(hardware, 0x003B4, ((((mtlb_high << 20) | (mtlb_low >> 12)) << 4) | 1));
    if (status != VIP_SUCCESS) {
        PRINTK_E("write mmu pdentry register fail.\n");
        return status;
    }

    status = gckvip_write_register(hardware, 0x00388, 0x21);
    if (status != VIP_SUCCESS) {
        PRINTK_E("write mmu pdentry register bit[5] and bit[0] fail.\n");
        return status;
    }

    return status;
}

/*
@brief commmit a commands to setup MMU buffer mode.
@param context, the contect of kernel space.
*/
static vip_status_e gckvip_mmu_commandmodel_setup(
    gckvip_context_t *context,
    gckvip_hardware_t *hardware
    )
{
    #define STATES_SIZE   8
    vip_status_e status = VIP_SUCCESS;
    void *handle = VIP_NULL;
    phy_address_t physical = 0;
    volatile vip_uint32_t *logical = VIP_NULL;
    vip_uint32_t size = STATES_SIZE * sizeof(vip_uint32_t);
    vip_uint32_t buffer_size = size;
    vip_uint32_t idleState = 0;
    vip_int32_t try_count = 0;
    vip_uint32_t count = 0;
    phy_address_t tmp = 0;
    vip_uint32_t page_table_l = 0;
    vip_uint32_t page_table_h = 0;

    vip_uint32_t alloc_flag = GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS |
                              GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR |
                              GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL |
                              GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE;
#if !vpmdENABLE_VIDEO_MEMORY_CACHE
    alloc_flag |= GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE;
#endif
    /* make 32-bits system happy */
    tmp = context->MMU_entry.physical >> 16;
    page_table_h = (vip_uint32_t)((tmp >> 16) & 0xFFFFFFFF);
    page_table_l = (vip_uint32_t)(context->MMU_entry.physical & 0xFFFFFFFF);

    gckvip_write_register(hardware, 0x0038C, page_table_l);
    gckvip_write_register(hardware, 0x00390, page_table_h);
    gckvip_write_register(hardware, 0x00394, 1);

    buffer_size += APPEND_COMMAND_SIZE;
    buffer_size = GCVIP_ALIGN(buffer_size, vpmdCPU_CACHE_LINE_SIZE);
    gcOnError(gckvip_mem_allocate_videomemory(context, buffer_size,
                                              &handle, (void **)&logical, &physical,
                                              gcdVIP_MEMORY_ALIGN_SIZE, alloc_flag));
    if (physical & 0xFFFFFFFF00000000ULL) {
        PRINTK_E("MMU initialize buffer only support 32bit physical address\n");
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    logical[count++] = 0x0801006b;
    logical[count++] = 0xfffe0000;
    logical[count++] = 0x08010e12;
    logical[count++] = 0x00490000;
    logical[count++] = 0x08010e02;
    logical[count++] = 0x00000701;
    logical[count++] = 0x48000000;
    logical[count++] = 0x00000701;
    /* Submit commands. */
    logical[count++] = (2 << 27);
    logical[count++] = 0;
    gckvip_os_memorybarrier();

    size = count * sizeof(vip_uint32_t);
    PRINTK_D("mmu command size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64"\n",
              size, logical, physical);
    if (size > buffer_size) {
        PRINTK_E("fail out of memory buffer size=%d, size=%d\n", buffer_size, size);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    gckvip_mem_flush_cache_ext(handle, physical, (void *)logical,
                               GCVIP_ALIGN(size, vpmdCPU_CACHE_LINE_SIZE), GCKVIP_CACHE_FLUSH);
#endif

    gckvip_write_register(hardware, 0x00654, (vip_uint32_t)physical);
    gckvip_os_memorybarrier();
    gckvip_write_register(hardware, 0x003A4, 0x00010000 | ((size + 7) >> 3));

    /* Wait until MMU configure finishes. */
    gckvip_read_register(hardware, 0x00004, &idleState);
    while (((idleState & 0x01) != 0x01) && (try_count < 100)) {
        try_count++;
#if (defined(_WIN32) || defined (LINUXEMULATOR)) || vpmdFPGA_BUILD
        gckvip_os_delay(10);
#else
        gckvip_os_udelay(100);
#endif
        gckvip_read_register(hardware, 0x00004, &idleState);
    }

    if (try_count >= 100) {
        #if vpmdENABLE_HANG_DUMP
        vip_uint32_t dmaAddr = 0xFFFFF, dmaLo = 0, dmaHi = 0;
        gckvip_read_register(hardware, 0x00668, &dmaLo);
        gckvip_read_register(hardware, 0x0066C, &dmaHi);
        gckvip_read_register(hardware, 0x00664, &dmaAddr);
        PRINTK("DMA Address = 0x%08X\n", dmaAddr);
        PRINTK("   dmaLow   = 0x%08X\n", dmaLo);
        PRINTK("   dmaHigh  = 0x%08X\n", dmaHi);
        PRINTK("COMMAND BUF DUMP\n");
        gckvip_dump_data((vip_uint32_t*)logical, physical, size);
        #endif
        PRINTK_E("failed to setup MMU, idleState=0x%x\n", idleState);
        gcGoOnError(VIP_ERROR_TIMEOUT);
    }

    PRINTK_D("mmu setup check vip idle status=0x8%x, try_count=%d\n", idleState, try_count);
#if vpmdFPGA_BUILD
    gckvip_os_delay(20);
#else
    gckvip_os_udelay(10);
#endif

    /* enable MMU */
    status = gckvip_write_register(hardware, 0x00388, 0x01);
    if (status != VIP_SUCCESS) {
        PRINTK_E("mmu enable fail.\n");
        gcGoOnError(status);
    }

onError:
    if (handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, handle);
    }

    return status;
}

/*
@brief initialize MMU table and allocate MMU, map vip-sram,
       axi-sram, reseverd memory if you system have this.
@param context, the contect of kernel space.
*/
vip_status_e gckvip_mmu_init(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_mmu_table_entry_t *entry = VIP_NULL;
    gckvip_hardware_t *hardware = VIP_NULL;
    phy_address_t physical_tmp = 0;
    vip_uint32_t config = 0;
    vip_uint32_t ext = 0;
    vip_uint32_t flush_mmu_flag = GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS |
                                  GCVIP_VIDEO_MEM_ALLOC_MAP_KERNEL_LOGICAL;
    vip_uint32_t alloc_flag = flush_mmu_flag | GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE;
    vip_uint32_t* stlb_ptr = VIP_NULL;
    vip_uint32_t i = 0;
    vip_uint32_t heap_cap = GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR;
    vip_uint32_t mtlb_align = (vip_false_e == gckvip_hw_support_pdmode(context)) ? 1024 : 4096;
    vip_uint64_t total_videmem_size = 0;

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    heap_cap = gckvip_heap_capability(&context->video_mem_heap);
#endif
    if (heap_cap & GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR) {
        alloc_flag |= GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR;
        flush_mmu_flag |= GCVIP_VIDEO_MEM_ALLOC_4GB_ADDR;
    }
#if !vpmdENABLE_VIDEO_MEMORY_CACHE
    alloc_flag |= GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE;
    flush_mmu_flag |= GCVIP_VIDEO_MEM_ALLOC_NONE_CACHE;
#endif
    total_videmem_size = gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE;
    total_videmem_size += gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE;
    if (total_videmem_size > 0xFFFFFFFF) {
        PRINTK_E("fail MMU page total size overflow, 1M=0x%x, 4K=0x%x\n",
                 gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE, gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    PRINTK_D("1M page size=0x%08x, 4K page size=0x%08x\n",
              gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE, gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE);
    PRINTK_D("1M MTLB count=%d, 4K MTLB count=%d, 1M entry count pre-MTLB=%d,"
             "4K STLB entry count pre-MTLB=%d\n",
              gcdMMU_1M_MTLB_COUNT, gcdMMU_4K_MTLB_COUNT,
              gcdMMU_STLB_1M_ENTRY_NUM, gcdMMU_STLB_4K_ENTRY_NUM);
    PRINTK_D("1M page memory size=%dMbytes, 4K page memory size=%dMbyte, one mtlb size=%dMbytes\n",
              gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE / 1024 / 1024,
              gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE / 1024 / 1024,
              gcdMMU_1K_MODE_ONE_MTLB_MEMORY / 1024 / 1024);
    if ((gcdMMU_1M_MTLB_COUNT + gcdMMU_4K_MTLB_COUNT) > gcdMMU_MTLB_ENTRY_NUM) {
        PRINTK_E("failed init mmu more than max MTLB count\n");
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    GCKVIP_LOOP_HARDWARE_START
    status = gckvip_os_create_atomic(&hw->flush_mmu);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create atomic for flush mmu\n");
        gcGoOnError(VIP_ERROR_IO);
    }
    GCKVIP_LOOP_HARDWARE_END

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_create_mutex(&context->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to create mutex for mmu\n");
        gcGoOnError(VIP_ERROR_IO);
    }
#endif

    /* allocate memory for MTLB, STLB, MMU_Entry */
    context->MTLB.size = (gcdMMU_1M_MTLB_COUNT + gcdMMU_4K_MTLB_COUNT) * sizeof(vip_uint32_t);
    context->MTLB.size = GCVIP_ALIGN(context->MTLB.size, vpmdCPU_CACHE_LINE_SIZE);
    gcOnError(gckvip_mem_allocate_videomemory(context, context->MTLB.size,
                                              &context->MTLB.handle,
                                              (void **)&context->MTLB.logical,
                                              &context->MTLB.physical,
                                              mtlb_align,
                                              alloc_flag));
    PRINTK_D("MTLB size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64", mtlb_align=%d\n",
            context->MTLB.size, context->MTLB.logical, context->MTLB.physical, mtlb_align);
    if ((context->MTLB.physical & (mtlb_align - 1)) > 0) {
        PRINTK_E("MTLB physical not align to %d\n", mtlb_align);
        gcGoOnError(VIP_ERROR_IO);
    }
    gckvip_os_zero_memory(context->MTLB.logical, context->MTLB.size);

    context->STLB_1M.size = gcdMMU_1M_MTLB_COUNT * gcdMMU_STLB_1M_SIZE;
    context->STLB_1M.size = GCVIP_ALIGN(context->STLB_1M.size, vpmdCPU_CACHE_LINE_SIZE);
    gcOnError(gckvip_mem_allocate_videomemory(context, context->STLB_1M.size,
                                              &context->STLB_1M.handle,
                                              (void **)&context->STLB_1M.logical,
                                              &context->STLB_1M.physical,
                                              gcdVIP_MEMORY_ALIGN_SIZE,
                                              alloc_flag));
    PRINTK_D("STLB 1M size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64"\n",
            context->STLB_1M.size, context->STLB_1M.logical, context->STLB_1M.physical);

    stlb_ptr = (vip_uint32_t*)context->STLB_1M.logical;
    for (i = 0; i < context->STLB_1M.size/sizeof(vip_uint32_t); i++) {
        *stlb_ptr = FREE_STLB_ENTRY_VALUE;
        stlb_ptr++;
    }

    gcOnError(gckvip_os_allocate_memory(gcdMMU_STLB_1M_ENTRY_NUM * gcdMMU_1M_MTLB_COUNT / UINT8_BIT_COUNT,
                                        (void **)&context->bitmap_1M));
    gckvip_os_zero_memory((void*)context->bitmap_1M, \
                          gcdMMU_STLB_1M_ENTRY_NUM * gcdMMU_1M_MTLB_COUNT / UINT8_BIT_COUNT);

    context->STLB_4K.size = gcdMMU_4K_MTLB_COUNT * gcdMMU_STLB_4K_SIZE;
    context->STLB_4K.size = GCVIP_ALIGN(context->STLB_4K.size, vpmdCPU_CACHE_LINE_SIZE);
    gcOnError(gckvip_mem_allocate_videomemory(context, context->STLB_4K.size,
                                              &context->STLB_4K.handle,
                                              (void **)&context->STLB_4K.logical,
                                              &context->STLB_4K.physical,
                                              gcdVIP_MEMORY_ALIGN_SIZE,
                                              alloc_flag));
    PRINTK_D("STLB 4K size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64"\n",
            context->STLB_4K.size, context->STLB_4K.logical, context->STLB_4K.physical);

    stlb_ptr = (vip_uint32_t*)context->STLB_4K.logical;
    for (i = 0; i < context->STLB_4K.size/sizeof(vip_uint32_t); i++) {
        *stlb_ptr = FREE_STLB_ENTRY_VALUE;
        stlb_ptr++;
    }

    gcOnError(gckvip_os_allocate_memory(gcdMMU_4K_MTLB_COUNT * gcdMMU_STLB_4K_ENTRY_NUM / UINT8_BIT_COUNT,
                                        (void **)&context->bitmap_4K));
    gckvip_os_zero_memory((void*)context->bitmap_4K, \
                          gcdMMU_4K_MTLB_COUNT * gcdMMU_STLB_4K_ENTRY_NUM / UINT8_BIT_COUNT);

    context->MMU_entry.size = 1024;
    context->MMU_entry.size = GCVIP_ALIGN(context->MMU_entry.size, vpmdCPU_CACHE_LINE_SIZE);
    gcOnError(gckvip_mem_allocate_videomemory(context, context->MMU_entry.size,
                                              &context->MMU_entry.handle,
                                              (void **)&context->MMU_entry.logical,
                                              &context->MMU_entry.physical,
                                              gcdVIP_MEMORY_ALIGN_SIZE,
                                              alloc_flag));
    PRINTK_D("MMU entry size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64"\n",
            context->MMU_entry.size, context->MMU_entry.logical, context->MMU_entry.physical);
    gckvip_os_zero_memory(context->MMU_entry.logical, context->MMU_entry.size);

    if (((context->MTLB.physical & 0xFF00000000) != (context->STLB_1M.physical & 0xFF00000000)) ||
        ((context->MTLB.physical & 0xFF00000000) != (context->STLB_4K.physical & 0xFF00000000))) {
        PRINTK_E("fail MTLB physical=0x%"PRIx64", STLB_1M physical=0x%"PRIx64", STLB_4K physical=0x%"PRIx64"\n",
                    context->MTLB.physical, context->STLB_1M.physical, context->STLB_4K.physical);
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    config = (vip_uint32_t)(context->MTLB.physical & 0xFFFFFFFF);
    /* make 32-bits system happy */
    physical_tmp = context->MTLB.physical >> 16;
    ext = (vip_uint32_t)((physical_tmp >> 16) & 0xFFFFFFFF);

    /* more than 40bit physical address */
    if (ext & 0xFFFFFF00) {
        PRINTK_E("fail to init mmu, more than 40bits physical address\n");
        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

    config |= 1; /* 1K mode */

    entry = (gckvip_mmu_table_entry_t*)context->MMU_entry.logical;
    entry->low = config;
    entry->high = ext;

    /* re-location secure mode page table */
    hardware = gckvip_get_hardware(0);
    context->ptr_STLB_1M = &context->STLB_1M;
    context->ptr_STLB_4K = &context->STLB_4K;
    context->ptr_MMU_flush = &hardware->MMU_flush;

    mmu_fill_mtlb(context);

    status = mmu_map_vip_sram(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to map vip sram, status=%d.\n", status);
        gcGoOnError(status);
    }

    status = mmu_map_axi_sram(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to map axi sram, status=%d.\n", status);
        gcGoOnError(status);
    }

#if vpmdENABLE_RESERVE_PHYSICAL
    if (context->reserve_phy_size > 0) {
        status = mmu_map_reserve_physical(context);
        if (status != VIP_SUCCESS) {
            PRINTK_E("failed to map reserved physical memory, size=0x%x\n", context->reserve_phy_size);
            gcGoOnError(VIP_ERROR_IO);
        }
    }
#endif

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    status = mmu_map_memory_heap(context);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to map video memory heap status=%d\n", status);
        gcGoOnError(VIP_ERROR_IO);
    }
#endif

    /* fill flush mmu cache load states */
    GCKVIP_LOOP_HARDWARE_START
    hw->MMU_flush.size = FLUSH_MMU_STATES_SIZE + LINK_SIZE;
    gcOnError(gckvip_mem_allocate_videomemory(context, hw->MMU_flush.size,
                                              &hw->MMU_flush.handle,
                                              (void **)&hw->MMU_flush.logical,
                                              &physical_tmp,
                                              gcdVIP_MEMORY_ALIGN_SIZE,
                                              flush_mmu_flag | GCVIP_VIDEO_MEM_ALLOC_MMU_PAGE_READ));
    PRINTK_D("MMU flush size=%dbytes, logical=0x%"PRPx", vip virtual=0x%08x\n",
             hw->MMU_flush.size, hw->MMU_flush.logical, hw->MMU_flush.physical);
    hw->MMU_flush.physical = (vip_uint32_t)physical_tmp;
    gckvip_os_zero_memory(hw->MMU_flush.logical, hw->MMU_flush.size);

    {
    volatile vip_uint32_t *data = (volatile vip_uint32_t*)hw->MMU_flush.logical;
    vip_uint32_t count = 0;
    data[count++] = 0x08010061;
    data[count++] = 0xffffff7f;
    data[count++] = 0x08010e02;
    data[count++] = 0x30000701;
    data[count++] = 0x48000000;
    data[count++] = 0x30000701;
    if (count * sizeof(vip_uint32_t) > FLUSH_MMU_STATES_SIZE) {
       PRINTK_E("flush mmu cache memory overflow %d > %d\n",
                 count * sizeof(vip_uint32_t), FLUSH_MMU_STATES_SIZE);
       gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
    data[count++] = 0;
    data[count++] = 0;
    }
    GCKVIP_LOOP_HARDWARE_END

    context->default_mem.size = gcdMMU_PAGE_4K_SIZE;
    gcOnError(gckvip_mem_allocate_videomemory(context, context->default_mem.size,
                                   &context->default_mem.handle,
                                   (void **)&context->default_mem.logical,
                                   &context->default_mem.physical, gcdMMU_PAGE_4K_SIZE,
                                   GCVIP_VIDEO_MEM_ALLOC_CONTIGUOUS |
                                   GCVIP_VIDEO_MEM_ALLOC_NO_MMU_PAGE));
#if vpmdENABLE_VIDEO_MEMORY_CACHE
    gckvip_mem_flush_cache_ext(context->MTLB.handle, context->MTLB.physical,
                          context->MTLB.logical, context->MTLB.size, GCKVIP_CACHE_FLUSH);
    gckvip_mem_flush_cache_ext(context->STLB_1M.handle, context->STLB_1M.physical,
                          context->STLB_1M.logical, context->STLB_1M.size, GCKVIP_CACHE_FLUSH);
    gckvip_mem_flush_cache_ext(context->STLB_4K.handle, context->STLB_4K.physical,
                           context->STLB_4K.logical, context->STLB_4K.size, GCKVIP_CACHE_FLUSH);
    gckvip_mem_flush_cache_ext(context->MMU_entry.handle, context->MMU_entry.physical,
                          context->MMU_entry.logical, context->MMU_entry.size, GCKVIP_CACHE_FLUSH);
    GCKVIP_LOOP_HARDWARE_START
    vip_uint32_t flush_size = GCVIP_ALIGN(hw->MMU_flush.size, vpmdCPU_CACHE_LINE_SIZE);
    gckvip_mem_flush_cache(hw->MMU_flush.handle, hw->MMU_flush.physical,
                           hw->MMU_flush.logical, flush_size, GCKVIP_CACHE_FLUSH);
    GCKVIP_LOOP_HARDWARE_END
#endif

    GCKVIP_LOOP_HARDWARE_START
    gckvip_os_set_atomic(hw->flush_mmu, 1);
    GCKVIP_LOOP_HARDWARE_END

#if vpmdENABLE_CAPTURE_MMU
    gcOnError(gckvip_database_get_id(&context->videomem_database, context->MMU_entry.handle,
                                     gckvip_os_get_pid(), &context->MMU_entry.handle_id));
    gcOnError(gckvip_database_get_id(&context->videomem_database, context->MTLB.handle,
                                     gckvip_os_get_pid(), &context->MTLB.handle_id));
    gcOnError(gckvip_database_get_id(&context->videomem_database, context->STLB_1M.handle,
                                     gckvip_os_get_pid(), &context->STLB_1M.handle_id));
    gcOnError(gckvip_database_get_id(&context->videomem_database, context->STLB_4K.handle,
                                     gckvip_os_get_pid(), &context->STLB_4K.handle_id));
#endif
    return status;

onError:
    GCKVIP_LOOP_HARDWARE_START
    if (hw->flush_mmu != VIP_NULL) {
        gckvip_os_destroy_atomic(hw->flush_mmu);
        hw->flush_mmu = VIP_NULL;
    }
    GCKVIP_LOOP_HARDWARE_END
    if (context->bitmap_4K != VIP_NULL) {
        gckvip_os_free_memory((void*)context->bitmap_4K);
        context->bitmap_4K = VIP_NULL;
    }
    if (context->bitmap_1M != VIP_NULL) {
        gckvip_os_free_memory((void*)context->bitmap_1M);
        context->bitmap_1M = VIP_NULL;
    }
    return status;
}

/*
@brief un-initialize MMU.
@param context, the contect of kernel space.
*/
vip_status_e gckvip_mmu_uninit(
    gckvip_context_t *context
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_CAPTURE_MMU
    gckvip_database_put_id(&context->videomem_database, context->MMU_entry.handle_id);
    gckvip_database_put_id(&context->videomem_database, context->MTLB.handle_id);
    gckvip_database_put_id(&context->videomem_database, context->STLB_1M.handle_id);
    gckvip_database_put_id(&context->videomem_database, context->STLB_4K.handle_id);
#endif

    GCKVIP_LOOP_HARDWARE_START
    if (hw->flush_mmu != VIP_NULL) {
        gckvip_os_destroy_atomic(hw->flush_mmu);
        hw->flush_mmu = VIP_NULL;
    }
    if (hw->MMU_flush.handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, hw->MMU_flush.handle);
        hw->MMU_flush.handle = VIP_NULL;
    }
    GCKVIP_LOOP_HARDWARE_END

    if (context->bitmap_1M != VIP_NULL) {
        gckvip_os_free_memory((void*)context->bitmap_1M);
        context->bitmap_1M = VIP_NULL;
    }
    if (context->bitmap_4K != VIP_NULL) {
        gckvip_os_free_memory((void*)context->bitmap_4K);
        context->bitmap_4K = VIP_NULL;
    }
    if (context->MTLB.handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, context->MTLB.handle);
        context->MTLB.handle = VIP_NULL;
    }
    if (context->STLB_1M.handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, context->STLB_1M.handle);
        context->STLB_1M.handle = VIP_NULL;
    }
    if (context->STLB_4K.handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, context->STLB_4K.handle);
        context->STLB_4K.handle = VIP_NULL;
    }
    if (context->MMU_entry.handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, context->MMU_entry.handle);
        context->MMU_entry.handle = VIP_NULL;
    }
    if (context->default_mem.handle != VIP_NULL) {
        gckvip_mem_free_videomemory(context, context->default_mem.handle);
        context->default_mem.handle = VIP_NULL;
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (context->mmu_mutex != VIP_NULL) {
        gckvip_os_destroy_mutex(context->mmu_mutex);
        context->mmu_mutex = VIP_NULL;
    }
#endif

    return status;
}

/*
@brief enable MMU.
@param context, the context of kernel space.
*/
vip_status_e gckvip_mmu_enable(
    gckvip_context_t *context,
    gckvip_hardware_t *hardware
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_bool_e support_pdmode = vip_false_e;

    support_pdmode = gckvip_hw_support_pdmode(context);

    if (vip_true_e == support_pdmode) {
        status = gckvip_mmu_pdmode_setup(context, hardware);
        if (status != VIP_SUCCESS) {
            PRINTK_E("core%d mmu enbale fail for pdmode\n", hardware->core_id);
            gcGoOnError(status);
        }
    }
    else {
        status = gckvip_mmu_commandmodel_setup(context, hardware);
        if (status != VIP_SUCCESS) {
            PRINTK_E("core%d mmu enable fail for command mode\n", hardware->core_id);
            gcGoOnError(status);
        }
    }

    PRINTK_D("core%d mmu enable done mode=%s..\n", hardware->core_id, support_pdmode ? "PD" : "CMD");

onError:
    return status;
}

/*
@brief map dynamic memory to VIP virtual for all pages are 4Kbytes or 1Mbytes.
       only can handle all pages are 4Kbyes or 1Mbytes.
@param context, the contect of kernel space.
@param OUT page_info, MMU page information for this memory.
@param IN physical_table, physical address table that need to be mapped.
@param IN physical_num, the number of physical address.
@param IN size_table, the size of memory in physical_table.
@param IN page type, GCVIP_MMU_1M_PAGE or GCVIP_MMU_4K_PAGE MMU page.
@param OUT virtual_addr, VIP virtual address.
*/
static vip_status_e gckvip_mem_map_page(
    gckvip_context_t *context,
    gckvip_mmu_page_info_t *page_info,
    phy_address_t *physical_table,
    vip_uint32_t physical_num,
    vip_uint32_t *size_table,
    vip_uint8_t page_type,
    vip_uint32_t *virtual_addr,
    vip_uint8_t writable
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vip_int32_t num_stlb = 0;
    vip_int32_t num_mtlb = 0;
    vip_uint32_t count = 0;
    vip_uint32_t page_size = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t max_mtlb_num = 0;
    vip_uint32_t stlb_shift = 0;
    vip_uint32_t page_mask = 0;
    vip_uint32_t total_size = 0;
    vip_uint32_t offset = 0;
    vip_uint32_t stlb_start = 0;
    vip_uint32_t mtlb_start = 0;
    vip_uint8_t insert_front = vip_true_e;
    vip_uint32_t total_videmem_size = 0;
    gckvip_mmu_page_info_t info;
    gckvip_mmu_page_info_t info_tmp;

    if ((virtual_addr == VIP_NULL) || (physical_table == VIP_NULL) ||
        (page_info == VIP_NULL) || (size_table == VIP_NULL)) {
        PRINTK_E("map dyn page parameter is NULL\n");
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }
    total_videmem_size = gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE;
    total_videmem_size += gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE;

    PRINTK_D("mmu map page physical_num=%d, page_type=%s\n",
              physical_num, (page_type == GCVIP_MMU_1M_PAGE) ? "1M" : "4K");

    for (i = 0; i < physical_num; i++) {
        total_size += size_table[i];
    }

    switch (page_type) {
    case GCVIP_MMU_1M_PAGE:
    {
        stlb_entry_num = gcdMMU_STLB_1M_ENTRY_NUM;
        max_mtlb_num = gcdMMU_1M_MTLB_COUNT;
        page_size = gcdMMU_PAGE_1M_SIZE;
        stlb_shift = gcdMMU_STLB_1M_SHIFT;
        page_mask = gcdMMU_PAGE_1M_MASK;
    }
    break;

    case GCVIP_MMU_4K_PAGE:
    {
        stlb_entry_num = gcdMMU_STLB_4K_ENTRY_NUM;
        max_mtlb_num = gcdMMU_1M_MTLB_COUNT + gcdMMU_4K_MTLB_COUNT;
        page_size = gcdMMU_PAGE_4K_SIZE;
        stlb_shift = gcdMMU_STLB_4K_SHIFT;
        page_mask = gcdMMU_PAGE_4K_MASK;
    }
    break;

    default:
        PRINTK_E("failed to fill stlb, not support this page type=%d\n", page_type);
        gcGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    /* check pages */
    for (i = 1; i < physical_num; i++) {
        if (size_table[i] != page_size) {
            PRINTK_E("map dyn page function not support this page size. index=%d, size=0x%x\n",
                      i, size_table[i]);
            gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
        if ((physical_table[i] & (page_size - 1)) != 0) {
            PRINTK_E("map dyn page function not support address align. index=%d, "
                     "address=0x%"PRIx64"\n", i, physical_table[i]);
            gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
    }

    offset = (vip_uint32_t)physical_table[0] & page_mask;
    total_size += offset;

    if (offset >= gcdMMU_PAGE_4K_SIZE) {
        insert_front = vip_false_e;
    }

    if (physical_num > 1) {
        num_stlb = physical_num;
    }
    else {
        num_stlb = (total_size + page_size - 1) / page_size;
        PRINTK_D("mmu map page physical=0x%"PRIx64", size=0x%xbytes, total_size=0x%x, offset=0x%x\n",
                  physical_table[0], size_table[0], total_size, offset);
    }
    num_mtlb = (total_size + (1 << gcdMMU_MTLB_SHIFT) - 1) / (1 << gcdMMU_MTLB_SHIFT);

    if ((num_mtlb > 0) && (num_stlb > 0)) {
        /* reserved page at the front page-section */
        if (GCVIP_MMU_1M_PAGE == page_type) {
            if (vip_true_e == insert_front) {
                num_stlb += RESERVED_1M_PAGE_NUM * 2;
            }
            else {
                /* only insert end */
                num_stlb += RESERVED_1M_PAGE_NUM;
            }
        }
        else if (GCVIP_MMU_4K_PAGE == page_type) {
            num_stlb += RESERVED_4K_PAGE_NUM * 2;
        }
        gcOnError(mmu_get_page_info(context, num_mtlb, num_stlb, page_type, &info));

        if (info.mtlb_end >= max_mtlb_num) {
            PRINTK_E("MMU MTLB size %d is greater than max mtlb size %d, page type=%d\n",
                      info.mtlb_end, max_mtlb_num, page_type);
            gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }

        mtlb_start = info.mtlb_start;
        stlb_start = info.stlb_start;

        if (GCVIP_MMU_1M_PAGE == page_type) {
            if (vip_true_e == insert_front) {
                if ((info.stlb_start + RESERVED_1M_PAGE_NUM) >= gcdMMU_STLB_1M_ENTRY_NUM) {
                    stlb_start = 0;
                    mtlb_start += 1;
                }
                else {
                    stlb_start += RESERVED_1M_PAGE_NUM;
                }
            }
        }
        else if (GCVIP_MMU_4K_PAGE == page_type) {
            if ((info.stlb_start + RESERVED_4K_PAGE_NUM) >= gcdMMU_STLB_4K_ENTRY_NUM) {
                stlb_start = 0;
                mtlb_start += 1;
            }
            else {
                stlb_start += RESERVED_4K_PAGE_NUM;
            }
        }

        *virtual_addr = (mtlb_start << gcdMMU_MTLB_SHIFT)  |
                   (stlb_start << stlb_shift)         |
                   ((vip_uint32_t)physical_table[0] & page_mask);

        #if ENABLE_MMU_MAPPING_LOG
        PRINTK("virtual=0x%08x, mtlb_start=%d, stlb_start=%d, physstart=0x%"PRIx64", offset=0x%x\n",
                *virtual_addr, mtlb_start, stlb_start, physical_table[0],
                (vip_uint32_t)physical_table[0] & page_mask);
        #endif

        if ((1 == physical_num) && (GCVIP_MMU_1M_PAGE == page_type)) {
            info_tmp.mtlb_start = mtlb_start;
            info_tmp.mtlb_end = info.mtlb_end;
            info_tmp.stlb_start = stlb_start;
            info_tmp.stlb_end = info.stlb_end;
            #if ENABLE_MMU_MAPPING_LOG
            PRINTK("map type=%s, mtlb %d~%d, stlb %d~%d, physical=0x%"PRIx64", size=0x%x\n",
                    (page_type == GCVIP_MMU_1M_PAGE) ? "1M byte" : "4K byte",
                    mtlb_start, info_tmp.mtlb_end, stlb_start, info_tmp.stlb_end, physical_table[0], size_table[0]);
            #endif
            status = mmu_fill_stlb(context, &info_tmp, page_type, physical_table[0], writable);
            if (status != VIP_SUCCESS) {
                PRINTK_E("failed to fill stlb... physical_num=%d\n", physical_num);
                gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
            }
        }
        else if ((1 == physical_num) && (GCVIP_MMU_4K_PAGE == page_type) && (size_table[0] > gcdMMU_PAGE_4K_SIZE)) {
            PRINTK_E("fail not support this config in fill mmu\n");
            gcGoOnError(VIP_ERROR_FAILURE);
        }
        else {
            for (i = mtlb_start; i <= info.mtlb_end; i++) {
                vip_uint32_t s_start = 0;
                vip_uint32_t s_last = (i == info.mtlb_end) ? info.stlb_end : (stlb_entry_num - 1);

                if (i > mtlb_start) {
                    s_start = 0;
                }
                else {
                    s_start = stlb_start; /* first stlb page */
                }

                while (s_start <= s_last) {
                    info_tmp.mtlb_start = i;
                    info_tmp.mtlb_end = i;
                    info_tmp.stlb_start = s_start;
                    info_tmp.stlb_end = s_start;

                    #if ENABLE_MMU_MAPPING_LOG
                    PRINTK("map type=%s, index=%d, mtlb=%d, stlb=%d, physical=0x%"PRIx64"\n",
                            (page_type == GCVIP_MMU_1M_PAGE) ? "1M byte" : "4K byte",
                            count, info_tmp.mtlb_start, info_tmp.stlb_start, physical_table[count]);
                    #endif

                    status = mmu_fill_stlb(context, &info_tmp, page_type, physical_table[count], writable);
                    if (status != VIP_SUCCESS) {
                        PRINTK_E("failed to fill stlb... physical_index=%d, physical_num=%d\n",
                                 count, physical_num);
                        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
                    }
                    count++;
                    s_start++;
                    if (count >= physical_num) {
                        break;
                    }
                }
                if (count >= physical_num) {
                    break;
                }
            }
        }
    }
    else {
        PRINTK_E("failed to map dyn page type %d, mtlb num=%d, stlb num=%d\n",
                  page_type, num_mtlb, num_stlb);
        gcGoOnError(VIP_ERROR_IO);
    }

    page_info->mtlb_start = info.mtlb_start;
    page_info->stlb_start= info.stlb_start;
    page_info->mtlb_end = info.mtlb_end;
    page_info->stlb_end = info.stlb_end;
    page_info->page_type = page_type;

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    if (GCVIP_MMU_1M_PAGE == page_type) {
        gckvip_mem_flush_cache_ext(context->ptr_STLB_1M->handle, context->ptr_STLB_1M->physical,
                                   context->ptr_STLB_1M->logical, context->ptr_STLB_1M->size,
                                   GCKVIP_CACHE_FLUSH);
    }
    else if (GCVIP_MMU_4K_PAGE == page_type) {
        gckvip_mem_flush_cache_ext(context->ptr_STLB_4K->handle, context->ptr_STLB_4K->physical,
                                   context->ptr_STLB_4K->logical, context->ptr_STLB_4K->size,
                                   GCKVIP_CACHE_FLUSH);
    }
#endif

    PRINTK_D("mmu map page, page count=%d, page type=%s, mtlb: %d ~ %d, stlb: %d ~ %d, "
             "virtual=0x%08x\n",
             physical_num, (page_info->page_type == GCVIP_MMU_1M_PAGE) ? "1M" : "4K",
             page_info->mtlb_start, page_info->mtlb_end,
             page_info->stlb_start,page_info->stlb_end, *virtual_addr);

    /* sanity check */
    if ((*virtual_addr + total_size) > total_videmem_size) {
        PRINTK_E("fail to map mmu, virtual overflow, virtual_addr=0x%08x, total_size=0x%x\n",
                 *virtual_addr, total_size);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    return status;

onError:
    PRINTK_E("failed to get vip virtual in mapping dynamic page, status=%d\n", status);
    return status;
}

/*
@brief map dynamic memory to VIP virtual for all pages are 4Kbytes
@param context, the contect of kernel space.
@param OUT page_info, MMU page information for this memory.
@param IN physical_table, physical address table that need to be mapped.
@param IN physical_num, the number of physical address.
@param IN size_table, the size of memory in physical_table.
@param IN page type, GCVIP_MMU_4K_PAGE MMU page.
@param OUT virtual_addr, VIP virtual address.
*/
static vip_status_e gckvip_mmu_map_memory(
    gckvip_context_t *context,
    gckvip_mmu_page_info_t *page_info,
    phy_address_t *physical_table,
    vip_uint32_t physical_num,
    vip_uint32_t *size_table,
    vip_uint8_t page_type,
    vip_uint32_t *virtual_addr,
    vip_uint8_t writable
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t index = 0;
    vip_int32_t num_stlb = 0;
    vip_int32_t num_mtlb = 0;
    vip_uint32_t i = 0;
    vip_uint32_t total_size = 0;
    vip_uint32_t offset = 0;
    vip_uint32_t max_mtlb_num = gcdMMU_1M_MTLB_COUNT + gcdMMU_4K_MTLB_COUNT;
    gckvip_mmu_page_info_t info;
    vip_uint32_t mtlb_start = 0;
    vip_uint32_t stlb_start = 0;
    vip_uint32_t total_videmem_size = 0;
    phy_address_t physical_align = 0;

    PRINTK_D("mmu map memory physical_num=%d, page_type=%s\n",
              physical_num, (page_type == GCVIP_MMU_1M_PAGE) ? "1M" : "4K");
    total_videmem_size = gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE;
    total_videmem_size += gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE;

    /* check physical base address and size should alignment to 4K bytes */
    for (index = 1; index < physical_num; index++) {
        if ((physical_table[index] & (gcdMMU_PAGE_4K_SIZE - 1)) != 0) {
            PRINTK_E("failed to mmu page table, physical index=%d base address not alignment to 4K bytes\n", index);
            gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
        if (index != (physical_num - 1)) {
            if (size_table[index] % gcdMMU_PAGE_4K_SIZE) {
                PRINTK_E("failed to mmu page table, size index=%d not alignment to 4K bytes\n", index);
                gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }
        }
    }
    if (physical_num > 1) {
        vip_uint32_t tmp = (vip_uint32_t)physical_table[0] & gcdMMU_PAGE_4K_MASK;
        if ((tmp + size_table[0]) % gcdMMU_PAGE_4K_SIZE) {
            PRINTK_E("failed physical_num=%d, physical_table[0]=0x%"PRIx64", size=0x%x\n",
                     physical_num, physical_table[0], size_table[0]);
            gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }
    }

    for (i = 0; i < physical_num; i++) {
        total_size += size_table[i];
    }

    offset = (vip_uint32_t)physical_table[0] & gcdMMU_PAGE_4K_MASK;
    total_size += offset;

    if (1 == physical_num) {
        PRINTK_D("mmu map memory physical=0x%"PRIx64", size=0x%xbytes, total_size=0x%x, offset=0x%x\n",
                  physical_table[0], size_table[0], total_size, offset);
    }

    num_stlb = (total_size + gcdMMU_PAGE_4K_SIZE - 1) / gcdMMU_PAGE_4K_SIZE;
    num_mtlb = (total_size+ (1 << gcdMMU_MTLB_SHIFT) - 1) / (1 << gcdMMU_MTLB_SHIFT);

    if ((num_mtlb > 0) && (num_stlb > 0)) {
        vip_uint32_t *stlb_logical = VIP_NULL;
        vip_uint32_t physical_low = 0;
        vip_uint32_t physical_high = 0;
        /* reserved page at the front and end page-section */
        num_stlb += RESERVED_4K_PAGE_NUM * 2;
        gcOnError(mmu_get_page_info(context, num_mtlb, num_stlb, page_type, &info));

        if (info.mtlb_end >= max_mtlb_num) {
            PRINTK_E("MMU MTLB size %d is greater than max mtlb size %d, page type=%d\n",
                      info.mtlb_end, max_mtlb_num, page_type);
            gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }

        mtlb_start = info.mtlb_start;
        stlb_start = info.stlb_start;

        if ((info.stlb_start + RESERVED_4K_PAGE_NUM) >= gcdMMU_STLB_4K_ENTRY_NUM) {
            stlb_start = 0;
            mtlb_start += 1;
        }
        else {
            stlb_start += RESERVED_4K_PAGE_NUM;
        }

        *virtual_addr = (mtlb_start << gcdMMU_MTLB_SHIFT)     |
                   (stlb_start << gcdMMU_STLB_4K_SHIFT)  |
                   ((vip_uint32_t)physical_table[0] & gcdMMU_PAGE_4K_MASK);

        #if ENABLE_MMU_MAPPING_LOG
        PRINTK("virtual=0x%08x, mtlb_start=%d, stlb_start=%d, phy_start=0x%"PRIx64", offset=0x%x\n",
                *virtual_addr, mtlb_start, stlb_start, physical_table[0],
                (vip_uint32_t)physical_table[0] & gcdMMU_PAGE_4K_MASK);
        #endif

        for (i = 0; i < physical_num; i++) {
            vip_uint32_t entry_value = 0;
            phy_address_t physical = physical_table[i] & ~gcdMMU_PAGE_4K_MASK;
            phy_address_t physical_end = physical + GCVIP_ALIGN((size_table[i] + offset), gcdMMU_PAGE_4K_SIZE);
            while (physical < physical_end) {
                physical_align = physical & ~gcdMMU_PAGE_4K_MASK;
                physical_low = (vip_uint32_t)(physical_align & 0xFFFFFFFF);
                if (physical & 0xFFFFFFFF00000000ULL) {
                    /* make 32-bits system happy */
                    phy_address_t phy_tmp = physical_align >> 16;
                    physical_high = (vip_uint32_t)(phy_tmp >> 16);
                    if (physical_high > 0xFF) {
                        PRINTK_E("fail to fill mem stlb physical=0x%"PRIx64", high=0x%x, low=0x%x\n",
                                  physical, physical_high, physical_low);
                        gcGoOnError(VIP_ERROR_NOT_SUPPORTED);
                    }
                }
                else {
                    physical_high = 0;
                }
                entry_value = mmu_set_page(physical_low, physical_high, writable);
                stlb_logical = (vip_uint32_t*)((vip_uint8_t*)context->ptr_STLB_4K->logical + \
                               ((mtlb_start - gcdMMU_1M_MTLB_COUNT) * gcdMMU_STLB_4K_SIZE));
                _write_page_entry(stlb_logical + stlb_start, entry_value);

                #if ENABLE_MMU_MAPPING_LOG
                PRINTK("map index=%d, mtlb=%d, stlb=%d, physical=0x%"PRIx64"\n",
                          i, mtlb_start, stlb_start, physical);
                #endif

                physical += gcdMMU_PAGE_4K_SIZE;
                stlb_start++;

                if (stlb_start >= gcdMMU_STLB_4K_ENTRY_NUM) {
                    mtlb_start++;
                    stlb_start = 0;
                }
            }
        }
    }
    else {
        PRINTK_E("fail to map mmu dyn memory, mtlb_num=%d, stlb_num=%d\n", num_mtlb, num_stlb);
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    page_info->mtlb_start = info.mtlb_start;
    page_info->stlb_start= info.stlb_start;
    page_info->mtlb_end = info.mtlb_end;
    page_info->stlb_end = info.stlb_end;
    page_info->page_type = GCVIP_MMU_4K_PAGE;

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    gckvip_mem_flush_cache_ext(context->ptr_STLB_4K->handle, context->ptr_STLB_4K->physical,
                               context->ptr_STLB_4K->logical, context->ptr_STLB_4K->size,
                               GCKVIP_CACHE_FLUSH);
#endif
    PRINTK_D("mmu map memory, physical count =%d, mtlb: %d ~ %d, stlb: %d ~ %d, virtual=0x%08x\n",
             physical_num, page_info->mtlb_start, page_info->mtlb_end,
             page_info->stlb_start, page_info->stlb_end, *virtual_addr);
    /* sanity check */
    if ((*virtual_addr + total_size) > total_videmem_size) {
        PRINTK_E("fail to map mmu, virtual overflow, virtual_addr=0x%08x, total_size=0x%x\n",
                 *virtual_addr, total_size);
        gcGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    return status;

onError:
    PRINTK_E("failed to get vip virtual in mapping dynamic memory, status=%d\n", status);
    return status;
}

/*
@brief map dynamic memory to VIP virtual.
@param context, the contect of kernel space.
@param OUT page_info, MMU page information for this memory.
@param IN physical_table, physical address table that need to be mapped.
@param IN physical_num, the number of physical address.
@param IN size_table, the size of memory in physical_table.
@param IN page_type, vip mmu page table size, 1M or 4K byte.
@param OUT virtual_addr, VIP virtual address.
*/
vip_status_e gckvip_mmu_map(
    gckvip_context_t *context,
    gckvip_mmu_page_info_t *page_info,
    phy_address_t *physical_table,
    vip_uint32_t physical_num,
    vip_uint32_t *size_table,
    vip_uint32_t page_type,
    vip_uint32_t *virtual_addr,
    vip_uint8_t writable
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t index = 0;
    vip_bool_e all_4K = vip_true_e;
    vip_bool_e all_1M = vip_true_e;
    vip_bool_e mapped = vip_false_e;

    if ((virtual_addr == VIP_NULL) || (page_info == VIP_NULL) || (physical_table == VIP_NULL)) {
        PRINTK_E("map dyn memory parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if ((VIP_NULL == context->MTLB.handle) || (VIP_NULL == context->ptr_STLB_4K->handle)) {
        PRINTK_E("mmu page table is NULL MTLB=0x%"PRPx", STLB=0x%"PRPx"\n",
                  context->MTLB.handle, context->ptr_STLB_4K->handle);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

#if 0
    PRINTK("mmu dump physical table, physical_num=%d\n", physical_num);
    for (index = 0; index < physical_num; index++) {
        PRINTK("%d, physical=0x%"PRIx64", size=0x%x\n", index, physical_table[index], size_table[index]);
    }
#endif

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_lock_mutex(context->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mmu mutex\n");
        return VIP_ERROR_FAILURE;
    }
#endif

    /* The size of each item in size_table[] is the same or divisible,
       call gckvip_mem_map_page() function
    */
    if (physical_num > 2) {
        for (index = 0; index < physical_num; index++) {
            if (size_table[index] > gcdMMU_PAGE_4K_SIZE) {
                all_4K = vip_false_e;
            }
            else {
                if ((gcdMMU_PAGE_4K_SIZE == size_table[index]) && all_4K) {
                    all_4K = vip_true_e;
                }
                else {
                    if ((index != (physical_num - 1)) && (0 != index)) {
                        /* not check the last one physical table */
                        all_4K = vip_false_e;
                    }
                }
            }
            if (size_table[index] > gcdMMU_PAGE_1M_SIZE) {
                all_1M = vip_false_e;
            }
            else {
                if ((gcdMMU_PAGE_1M_SIZE == size_table[index]) && all_1M) {
                    all_1M = vip_true_e;
                }
                else {
                    if ((index != (physical_num - 1)) && (0 != index)) {
                        /* not last one physical table */
                        all_1M = vip_false_e;
                    }
                }
            }
            if ((all_4K) && (size_table[physical_num - 1] > gcdMMU_PAGE_4K_SIZE)) {
                all_4K = vip_false_e;
            }
            if ((all_1M) && (size_table[physical_num - 1] > gcdMMU_PAGE_1M_SIZE)) {
                all_1M = vip_false_e;
            }
        }
    }
    else {
        all_1M = all_4K = vip_false_e;
    }

    if (1 == physical_num) {
        if (size_table[0] > (gcdMMU_PAGE_1M_SIZE / 2)) {
            /* only one physical table should be mapped. we use 1M MMU page table for it */
            gcOnError(gckvip_mem_map_page(context, page_info, physical_table, 1, size_table,
                                          GCVIP_MMU_1M_PAGE, virtual_addr, writable));
        }
        else {
            gcOnError(gckvip_mmu_map_memory(context, page_info, physical_table, 1, size_table,
                                            GCVIP_MMU_4K_PAGE, virtual_addr, writable));
        }
        mapped = vip_true_e;
    }
    else if (all_1M && (GCVIP_MMU_1M_PAGE == page_type)) {
        /* check physical base address should alignment to 1M byte */
        for (index = 1; index < physical_num; index++) {
            if ((physical_table[index] & (gcdMMU_PAGE_1M_SIZE - 1)) != 0) {
                PRINTK_I("physical index=%d base address 0x%"PRIx64" not alignment to 1M bytes\n",
                            index, physical_table[index]);
                all_1M = vip_false_e;
                break;
            }
        }
        if (all_1M) {
            /* all pages are 1M byte */
            gcOnError(gckvip_mem_map_page(context, page_info, physical_table, physical_num,
                                          size_table, GCVIP_MMU_1M_PAGE, virtual_addr, writable));
            mapped = vip_true_e;
        }
    }
    else if (all_4K) {
        /* check physical base address should alignment to 4K bytes */
        for (index = 1; index < physical_num; index++) {
            if ((physical_table[index] & (gcdMMU_PAGE_4K_SIZE - 1)) != 0) {
                PRINTK_I("physical index=%d base address 0x%"PRIx64" not alignment to 4K bytes\n",
                           index, physical_table[index]);
                all_4K = vip_false_e;
                break;
            }
        }
        if (all_4K) {
            /* for Linux 4k page size ?, all pages are 4K bytes */
            gcOnError(gckvip_mem_map_page(context, page_info, physical_table, physical_num,
                                          size_table, GCVIP_MMU_4K_PAGE, virtual_addr, writable));
            mapped = vip_true_e;
        }
    }
    PRINTK_D("mmu map physical_num=%d, all_4K=%d, all_1M=%d, page_type=%s\n",
            physical_num, all_4K, all_1M, (page_type == GCVIP_MMU_1M_PAGE) ? "1M" : "4K");

    if (vip_false_e == mapped) {
        gcOnError(gckvip_mmu_map_memory(context, page_info, physical_table, physical_num,
                                        size_table, GCVIP_MMU_4K_PAGE, virtual_addr, writable));
    }

    GCKVIP_LOOP_HARDWARE_START
    gckvip_os_set_atomic(hw->flush_mmu, 1);
    GCKVIP_LOOP_HARDWARE_END

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (VIP_SUCCESS != gckvip_os_unlock_mutex(context->mmu_mutex)) {
        PRINTK_E("failed to unlock mmu mutex\n");
    }
#endif
    return status;
}

/*
@brief un-map MMU page and free resource for dynamic allocate memory.
@param context, the contect of kernel space.
@param page_info, the mmu page table info.
*/
vip_status_e gckvip_mmu_unmap(
    gckvip_context_t *context,
    gckvip_mmu_page_info_t *page_info
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (VIP_NULL == page_info) {
        PRINTK_E("failed to unmap page table mmory, page info is NULL\n");
        return VIP_ERROR_FAILURE;
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_lock_mutex(context->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mmu mutex\n");
        return VIP_ERROR_FAILURE;
    }
#endif

    gcOnError(mmu_free_page_info(context, page_info->page_type, page_info));

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    if (GCVIP_MMU_1M_PAGE == page_info->page_type) {
        gckvip_mem_flush_cache_ext(context->ptr_STLB_1M->handle, context->ptr_STLB_1M->physical,
                                   context->ptr_STLB_1M->logical, context->ptr_STLB_1M->size,
                                   GCKVIP_CACHE_FLUSH);
    }
    else if (GCVIP_MMU_4K_PAGE == page_info->page_type) {
        gckvip_mem_flush_cache_ext(context->ptr_STLB_4K->handle, context->ptr_STLB_4K->physical,
                                   context->ptr_STLB_4K->logical, context->ptr_STLB_4K->size,
                                   GCKVIP_CACHE_FLUSH);
    }
#endif

    GCKVIP_LOOP_HARDWARE_START
    gckvip_os_set_atomic(hw->flush_mmu, 1);
    GCKVIP_LOOP_HARDWARE_END

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (VIP_SUCCESS != gckvip_os_unlock_mutex(context->mmu_mutex)) {
        PRINTK_E("failed to unlock mmu mutex\n");
    }
#endif
    return status;
}
#endif


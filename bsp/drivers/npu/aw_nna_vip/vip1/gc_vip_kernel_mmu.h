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

#ifndef _GC_VIP_KERNEL_MMU_H
#define _GC_VIP_KERNEL_MMU_H
#include <gc_vip_kernel.h>

#if vpmdENABLE_MMU

#define ENABLE_MMU_MAPPING_LOG      0

enum
{
    GCVIP_MMU_NONE_PAGE = 0,
    GCVIP_MMU_1M_PAGE   = 1,
    GCVIP_MMU_4K_PAGE   = 2,
};

#define gcdMMU_MTLB_SHIFT           24 /* 16Mbytes per-MTLB */
#define gcdMMU_STLB_4K_SHIFT        12
#define gcdMMU_STLB_64K_SHIFT       16
#define gcdMMU_STLB_1M_SHIFT        20
#define gcdMMU_STLB_16M_SHIFT       24

#define gcdMMU_MTLB_BITS            (32 - gcdMMU_MTLB_SHIFT)
#define gcdMMU_PAGE_4K_BITS         gcdMMU_STLB_4K_SHIFT
#define gcdMMU_STLB_4K_BITS         (32 - gcdMMU_MTLB_BITS - gcdMMU_PAGE_4K_BITS)
#define gcdMMU_PAGE_64K_BITS        gcdMMU_STLB_64K_SHIFT
#define gcdMMU_STLB_64K_BITS        (32 - gcdMMU_MTLB_BITS - gcdMMU_PAGE_64K_BITS)
#define gcdMMU_PAGE_1M_BITS         gcdMMU_STLB_1M_SHIFT
#define gcdMMU_STLB_1M_BITS         (32 - gcdMMU_MTLB_BITS - gcdMMU_PAGE_1M_BITS)
#define gcdMMU_PAGE_16M_BITS        gcdMMU_STLB_16M_SHIFT
#define gcdMMU_STLB_16M_BITS        4

#define gcdMMU_MTLB_ENTRY_NUM       (1 << gcdMMU_MTLB_BITS)
#define gcdMMU_MTLB_SIZE            (gcdMMU_MTLB_ENTRY_NUM << 2)
#define gcdMMU_STLB_4K_ENTRY_NUM    (1 << gcdMMU_STLB_4K_BITS)
#define gcdMMU_STLB_4K_SIZE         (gcdMMU_STLB_4K_ENTRY_NUM << 2)
#define gcdMMU_PAGE_4K_SIZE         (1 << gcdMMU_STLB_4K_SHIFT)
#define gcdMMU_STLB_64K_ENTRY_NUM   (1 << gcdMMU_STLB_64K_BITS)
#define gcdMMU_STLB_64K_SIZE        (gcdMMU_STLB_64K_ENTRY_NUM << 2)
#define gcdMMU_PAGE_64K_SIZE        (1 << gcdMMU_STLB_64K_SHIFT)
#define gcdMMU_STLB_1M_ENTRY_NUM    (1 << gcdMMU_STLB_1M_BITS)
#define gcdMMU_STLB_1M_SIZE         (gcdMMU_STLB_1M_ENTRY_NUM << 2)
#define gcdMMU_PAGE_1M_SIZE         (1 << gcdMMU_STLB_1M_SHIFT)
#define gcdMMU_STLB_16M_ENTRY_NUM   (1 << gcdMMU_STLB_16M_BITS)
#define gcdMMU_STLB_16M_SIZE        (gcdMMU_STLB_16M_ENTRY_NUM << 2)
#define gcdMMU_PAGE_16M_SIZE        (1 << gcdMMU_STLB_16M_SHIFT)

#define gcdMMU_MTLB_MASK            (~((1U << gcdMMU_MTLB_SHIFT)-1))
#define gcdMMU_STLB_4K_MASK         ((~0U << gcdMMU_STLB_4K_SHIFT) ^ gcdMMU_MTLB_MASK)
#define gcdMMU_PAGE_4K_MASK         (gcdMMU_PAGE_4K_SIZE - 1)
#define gcdMMU_STLB_64K_MASK        ((~((1U << gcdMMU_STLB_64K_SHIFT)-1)) ^ gcdMMU_MTLB_MASK)
#define gcdMMU_PAGE_64K_MASK        (gcdMMU_PAGE_64K_SIZE - 1)
#define gcdMMU_STLB_1M_MASK         ((~((1U << gcdMMU_STLB_1M_SHIFT)-1)) ^ gcdMMU_MTLB_MASK)
#define gcdMMU_PAGE_1M_MASK         (gcdMMU_PAGE_1M_SIZE - 1)
#define gcdMMU_STLB_16M_MASK        0x0F000000
#define gcdMMU_PAGE_16M_MASK        (gcdMMU_PAGE_16M_SIZE - 1)

/* Page offset definitions. */
#define gcdMMU_OFFSET_4K_BITS       (32 - gcdMMU_MTLB_BITS - gcdMMU_STLB_4K_BITS)
#define gcdMMU_OFFSET_4K_MASK       ((1U << gcdMMU_OFFSET_4K_BITS) - 1)
#define gcdMMU_OFFSET_64K_BITS      (32 - gcdMMU_MTLB_BITS - gcdMMU_STLB_64K_BITS)
#define gcdMMU_OFFSET_64K_MASK      ((1U << gcdMMU_OFFSET_64K_BITS) - 1)
#define gcdMMU_OFFSET_1M_BITS       (32 - gcdMMU_MTLB_BITS - gcdMMU_STLB_1M_BITS)
#define gcdMMU_OFFSET_1M_MASK       ((1U << gcdMMU_OFFSET_1M_BITS) - 1)
#define gcdMMU_OFFSET_16M_BITS      (32 - gcdMMU_MTLB_BITS)
#define gcdMMU_OFFSET_16M_MASK      ((1U << gcdMMU_OFFSET_16M_BITS) - 1)

#define gcdMMU_MTLB_ENTRY_HINTS_BITS 6
#define gcdMMU_MTLB_ENTRY_STLB_MASK  (~((1U << gcdMMU_MTLB_ENTRY_HINTS_BITS) - 1))

#define gcdMMU_MTLB_PRESENT         0x00000001
#define gcdMMU_MTLB_EXCEPTION       0x00000002
#define gcdMMU_MTLB_4K_PAGE         (0 << 2)
#define gcdMMU_MTLB_64K_PAGE        (1 << 2)
#define gcdMMU_MTLB_1M_PAGE         (2 << 2)
#define gcdMMU_MTBL_16M_PAGE        (3 << 2)

#define gcdMMU_STLB_PRESENT         0x00000001
#define gcdMMU_STLB_EXCEPTION       0x00000002
#define gcdMMU_STBL_WRITEABLE       0x00000004

/* You can configure total video memory size and 1M page size for decreasing MMU tale size */

/* maximum 4K page size */
#define gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE     0x60000000 /* 1.5GB */

/* maximum reserved memory size */
#define gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE     0x60000000 /* 1.5GB */

/* maximum video memory size */
#define gcdMMU_TOTAL_VIDEO_MEMORY            (gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE + gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE)


#define gcdMMU_1K_MODE_ONE_MTLB_MEMORY       (0x1000000)  /* 16M bytes */
#define gcdMMU_1M_MTLB_COUNT        (gcdMMU_1M_PAGE_VIDEO_MEMORY_SIZE / gcdMMU_1K_MODE_ONE_MTLB_MEMORY)
#define gcdMMU_4K_MTLB_COUNT        (gcdMMU_4K_PAGE_VIDEO_MEMORY_SIZE / gcdMMU_1K_MODE_ONE_MTLB_MEMORY)

typedef struct _gckvip_mmu_page_info
{
    vip_uint32_t mtlb_start;
    vip_uint32_t mtlb_end;
    vip_uint32_t stlb_start;
    vip_uint32_t stlb_end;
    vip_uint32_t page_type;
} gckvip_mmu_page_info_t;

vip_status_e gckvip_mmu_map(
    gckvip_context_t *context,
    gckvip_mmu_page_info_t *page_info,
    phy_address_t *physical_table,
    vip_uint32_t num_page,
    vip_uint32_t *size_table,
    vip_uint32_t page_type,
    vip_uint32_t *virtual,
    vip_uint8_t writable
    );

vip_status_e gckvip_mmu_unmap(
    gckvip_context_t *context,
    gckvip_mmu_page_info_t *page_info
    );

vip_status_e gckvip_mmu_init(
    gckvip_context_t *context
    );

vip_status_e gckvip_mmu_uninit(
    gckvip_context_t *context
    );

vip_status_e gckvip_mmu_enable(
    gckvip_context_t *context,
    gckvip_hardware_t *hardware
    );

#endif
#endif

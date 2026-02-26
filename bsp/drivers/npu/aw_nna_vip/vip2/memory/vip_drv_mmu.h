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

#ifndef _VIP_DRV_MMU_H
#define _VIP_DRV_MMU_H

#include <vip_drv_type.h>
#if vpmdENABLE_MMU
#include <vip_drv_os_port.h>
#include <vip_drv_video_memory.h>
#define ENABLE_MMU_MAPPING_LOG       0

typedef enum _vipdrv_mmu_page_type
{
    VIPDRV_MMU_NONE_PAGE = 0,
    VIPDRV_MMU_16M_PAGE  = 1,
    VIPDRV_MMU_1M_PAGE   = 2,
    VIPDRV_MMU_64K_PAGE  = 3,
    VIPDRV_MMU_4K_PAGE   = 4,
    VIPDRV_MMU_PAGE_MAX  = 5,
} vipdrv_mmu_page_type_e;

#if vpmdENABLE_40BITVA
/* You can configure total size */
#define vipdMMU_TOTAL_VIDEO_MEMORY   0x800000000 /* 32G default value, can set up to 1TB */

/* 4k mode */
#define vipdMMU_VA_BITS              40
#define vipdMMU_MTLB_SHIFT           30 /* 1024 Mbytes per-MTLB */
#define vipdMMU_PER_MTLB_MEMORY      0x40000000  /* 1024M bytes */
#define vipdMMU_MTLB_COUNT           (0x10000000000 < (vipdMMU_TOTAL_VIDEO_MEMORY + vipdMMU_PER_MTLB_MEMORY - 1) ? \
                                     0x10000000000 : (vipdMMU_TOTAL_VIDEO_MEMORY + vipdMMU_PER_MTLB_MEMORY - 1)) / \
                                     vipdMMU_PER_MTLB_MEMORY
#else
/* You can configure total size */
#define vipdMMU_TOTAL_VIDEO_MEMORY   0xFF000000 /* up to 4GB */

/* 1k mode */
#define vipdMMU_VA_BITS              32
#define vipdMMU_MTLB_SHIFT           24 /* 16Mbytes per-MTLB */
#define vipdMMU_PER_MTLB_MEMORY      0x1000000  /* 16M bytes */
#define vipdMMU_MTLB_COUNT           (0x100000000 < (vipdMMU_TOTAL_VIDEO_MEMORY + vipdMMU_PER_MTLB_MEMORY - 1) ? \
                                     0x100000000 : (vipdMMU_TOTAL_VIDEO_MEMORY + vipdMMU_PER_MTLB_MEMORY - 1)) / \
                                     vipdMMU_PER_MTLB_MEMORY
#endif

#define vipdMMU_STLB_4K_SHIFT        12
#define vipdMMU_STLB_64K_SHIFT       16
#define vipdMMU_STLB_1M_SHIFT        20
#define vipdMMU_STLB_16M_SHIFT       24

#define vipdMMU_MTLB_BITS            (vipdMMU_VA_BITS - vipdMMU_MTLB_SHIFT)
#define vipdMMU_PAGE_4K_BITS         vipdMMU_STLB_4K_SHIFT
#define vipdMMU_STLB_4K_BITS         (vipdMMU_VA_BITS - vipdMMU_MTLB_BITS - vipdMMU_PAGE_4K_BITS)
#define vipdMMU_PAGE_64K_BITS        vipdMMU_STLB_64K_SHIFT
#define vipdMMU_STLB_64K_BITS        (vipdMMU_VA_BITS - vipdMMU_MTLB_BITS - vipdMMU_PAGE_64K_BITS)
#define vipdMMU_PAGE_1M_BITS         vipdMMU_STLB_1M_SHIFT
#define vipdMMU_STLB_1M_BITS         (vipdMMU_VA_BITS - vipdMMU_MTLB_BITS - vipdMMU_PAGE_1M_BITS)
#define vipdMMU_PAGE_16M_BITS        vipdMMU_STLB_16M_SHIFT
#define vipdMMU_STLB_16M_BITS        (vipdMMU_VA_BITS - vipdMMU_MTLB_BITS - vipdMMU_PAGE_16M_BITS)

#define vipdMMU_MTLB_ENTRY_NUM       ((vip_virtual_t)1 << vipdMMU_MTLB_BITS)
#define vipdMMU_MTLB_SIZE            (vipdMMU_MTLB_ENTRY_NUM << 2)
#define vipdMMU_STLB_4K_ENTRY_NUM    ((vip_virtual_t)1 << vipdMMU_STLB_4K_BITS)
#define vipdMMU_STLB_4K_SIZE         (vipdMMU_STLB_4K_ENTRY_NUM << 2)
#define vipdMMU_PAGE_4K_SIZE         ((vip_virtual_t)1 << vipdMMU_STLB_4K_SHIFT)
#define vipdMMU_STLB_64K_ENTRY_NUM   ((vip_virtual_t)1 << vipdMMU_STLB_64K_BITS)
#define vipdMMU_STLB_64K_SIZE        (vipdMMU_STLB_64K_ENTRY_NUM << 2)
#define vipdMMU_PAGE_64K_SIZE        ((vip_virtual_t)1 << vipdMMU_STLB_64K_SHIFT)
#define vipdMMU_STLB_1M_ENTRY_NUM    ((vip_virtual_t)1 << vipdMMU_STLB_1M_BITS)
#define vipdMMU_STLB_1M_SIZE         (vipdMMU_STLB_1M_ENTRY_NUM << 2)
#define vipdMMU_PAGE_1M_SIZE         ((vip_virtual_t)1 << vipdMMU_STLB_1M_SHIFT)
#define vipdMMU_STLB_16M_ENTRY_NUM   ((vip_virtual_t)1 << vipdMMU_STLB_16M_BITS)
#define vipdMMU_STLB_16M_SIZE        (vipdMMU_STLB_16M_ENTRY_NUM << 2)
#define vipdMMU_PAGE_16M_SIZE        ((vip_virtual_t)1 << vipdMMU_STLB_16M_SHIFT)

#define vipdMMU_MTLB_MASK            (~(((vip_virtual_t)1 << vipdMMU_MTLB_SHIFT)-1))
#define vipdMMU_STLB_4K_MASK         ((~(((vip_virtual_t)1 << vipdMMU_STLB_4K_SHIFT)-1)) ^ vipdMMU_MTLB_MASK)
#define vipdMMU_PAGE_4K_MASK         (vipdMMU_PAGE_4K_SIZE - 1)
#define vipdMMU_STLB_64K_MASK        ((~(((vip_virtual_t)1 << vipdMMU_STLB_64K_SHIFT)-1)) ^ vipdMMU_MTLB_MASK)
#define vipdMMU_PAGE_64K_MASK        (vipdMMU_PAGE_64K_SIZE - 1)
#define vipdMMU_STLB_1M_MASK         ((~(((vip_virtual_t)1 << vipdMMU_STLB_1M_SHIFT)-1)) ^ vipdMMU_MTLB_MASK)
#define vipdMMU_PAGE_1M_MASK         (vipdMMU_PAGE_1M_SIZE - 1)
#define vipdMMU_STLB_16M_MASK        ((~(((vip_virtual_t)1 << vipdMMU_STLB_16M_SHIFT)-1)) ^ vipdMMU_MTLB_MASK)
#define vipdMMU_PAGE_16M_MASK        (vipdMMU_PAGE_16M_SIZE - 1)

#define vipdMMU_MTLB_ENTRY_HINTS_BITS 6
#define vipdMMU_MTLB_ENTRY_STLB_MASK  (~(((vip_virtual_t)1 << vipdMMU_MTLB_ENTRY_HINTS_BITS) - 1))

#define vipdMMU_MTLB_PRESENT         0x00000001
#define vipdMMU_MTLB_EXCEPTION       0x00000002
#define vipdMMU_MTLB_4K_PAGE         (0 << 2)
#define vipdMMU_MTLB_64K_PAGE        (1 << 2)
#define vipdMMU_MTLB_1M_PAGE         (2 << 2)
#define vipdMMU_MTLB_16M_PAGE        (3 << 2)

#define vipdMMU_STLB_PRESENT         0x00000001
#define vipdMMU_STLB_EXCEPTION       0x00000002
#define vipdMMU_STBL_WRITEABLE       0x00000004

#define vipdMMU_TOTAL_SIZE           vipdMMU_MTLB_COUNT * vipdMMU_PER_MTLB_MEMORY

typedef struct _vipdrv_STLB_info {
    struct _vipdrv_STLB_info *next;
    vip_physical_t            physical;
    vip_uint8_t              *logical;
} vipdrv_STLB_info_t;

typedef struct _vipdrv_MTLB_info
{
    vip_uint8_t            *bitmap;
    vipdrv_mmu_page_type_e type;
    vipdrv_STLB_info_t     *STLB;
    vip_uint32_t           used_cnt;
} vipdrv_MTLB_info_t;

struct _vipdrv_mmu_info
{
    vipdrv_video_memory_t  MTLB;         /* the video memory of MTLB itself, for all MTLB */
    vip_uint32_t           MTLB_cnt;     /* the total count of MTLB */
    vipdrv_MTLB_info_t     *MTLB_info;   /* array of MTLB information. each item indicate one MTLB */
    vip_uint32_t           *STLB_memory; /* array of video memory ID. each item indicates one block of video memory,
                                            which is used for STLB itself */
    void                   **STLB_info;  /* array of STLB information. each item indicate one block of memory,
                                            which indicate information of all STLB of one MTLB */
    vipdrv_mutex           mmu_mutex;    /* mutex for mmu page table */
    vipdrv_STLB_info_t     free_list[VIPDRV_MMU_PAGE_MAX];
};

vip_status_e vipdrv_mmu_map(
    void **mmu_handle,
    vip_physical_t *physical_table,
    vip_uint32_t num_page,
    vip_size_t *size_table,
    vip_virtual_t *virtual,
    vip_uint64_t mmu_id,
    vip_uint8_t writable,
    vip_uint8_t addr_4G
    );

vip_status_e vipdrv_mmu_unmap(
    void *mmu_handle,
    vip_uint64_t mmu_id
    );

vip_status_e vipdrv_mmu_init(void);

vip_status_e vipdrv_mmu_uninit(void);

#if vpmdENABLE_CAPTURE_MMU
vip_status_e vipdrv_mmu_query_mmu_info(
    vipdrv_query_mmu_info_t* info
    );
#endif

vip_status_e vipdrv_mmu_page_get(
    vip_uint64_t mmu_id,
    vipdrv_mmu_info_t **mmu_info
    );

vip_status_e vipdrv_mmu_page_put(
    vip_uint64_t mmu_id
    );

vip_status_e vipdrv_mmu_page_uninit(
    vip_uint64_t mmu_id
    );

vip_char_t* vip_drv_mmu_page_str(
    vipdrv_mmu_page_type_e type
    );

vip_status_e vipdrv_mmu_page_macro(
    vipdrv_mmu_page_type_e type,
    vip_uint32_t* mtlb_value,
    vip_uint32_t* stlb_entry_num,
    vip_uint32_t* stlb_shift,
    vip_uint32_t* stlb_mask,
    vip_uint32_t* page_mask,
    vip_uint32_t* page_size
    );

vip_status_e vipdrv_mmu_split_heap(
    vipdrv_videomem_heap_info_t *heap_info,
    vipdrv_videomem_heap_info_t *split_info
    );
#endif

vip_status_e vipdrv_mmu_page_switch(
    vipdrv_hardware_t* hardware,
    vip_uint64_t mmu_id
    );

vip_uint64_t gen_mmu_id(
    vip_uint32_t pid,
    vip_uint32_t task_id
    );
#endif

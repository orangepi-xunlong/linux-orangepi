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
#include <vip_drv_mmu.h>
#if vpmdENABLE_MMU
#include <vip_drv_context.h>
#include <vip_drv_mem_heap.h>
#include <vip_drv_device_driver.h>

#undef VIPDRV_LOG_ZONE
#define VIPDRV_LOG_ZONE  VIPDRV_LOG_ZONE_MMU


#define MMU_PAGE_ALWAYS_WRITABLE    0
#define FREE_STLB_ENTRY_VALUE       0x2
#define DEFAULT_STLB_ENTRY_VALUE    0x5
#define RESERVED_PAGE_NUM           1
#define UINT8_BIT_COUNT             (8 * sizeof(vip_uint8_t))
#define STLB_INFO_CNT               16


#define _write_page_entry(page_entry, entry_value) \
        *(vip_uint32_t*)(page_entry) = (vip_uint32_t)(entry_value)

#define _read_page_entry(page_entry)  *(vip_uint32_t*)(page_entry)

typedef struct _vipdrv_mmu_page_info
{
    vip_uint32_t mtlb_start;
    vip_uint32_t mtlb_end;
    vip_uint32_t stlb_start;
    vip_uint32_t stlb_end;
    vip_uint32_t page_type;
} vipdrv_mmu_page_info_t;

typedef struct _vipdrv_mmu_table_entry
{
    vip_uint32_t low;
    vip_uint32_t high;
} vipdrv_mmu_table_entry_t;

static vip_status_e ensure_STLB_capacity(
    vipdrv_mmu_info_t* mmu_info,
    vipdrv_mmu_page_type_e type
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t stlb_size = 0;
    vip_uint32_t *device_cnt = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_DEVICE_CNT);
    vip_uint64_t device_vis = (~(vip_uint64_t)1) >> (64 - *device_cnt);

    vipOnError(vipdrv_mmu_page_macro(type, VIP_NULL, &stlb_size, VIP_NULL, VIP_NULL, VIP_NULL, VIP_NULL));
    stlb_size *= sizeof(vip_uint32_t);

    if (VIP_NULL == mmu_info->free_list[type].next) {
        vipdrv_STLB_info_t* STLB_info = VIP_NULL;
        vipdrv_STLB_info_t* new_STLB_info = VIP_NULL;
        vip_uint32_t i = 0;
        vip_uint8_t* logical = VIP_NULL;
        vip_physical_t physical = 0;
        if (VIPDRV_MMU_4K_PAGE == type) {
            vipdrv_video_memory_t STLB_memory = { 0 };
            vip_uint32_t flag = VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS |
                                VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL |
                                VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE |
                                VIPDRV_VIDEO_MEM_ALLOC_MMU_PAGE_MEM;
            #if !vpmdENABLE_VIDEO_MEMORY_CACHE
            flag |= VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE;
            #endif

            STLB_memory.size = stlb_size * STLB_INFO_CNT;
            vipOnError(vipdrv_mem_allocate_videomemory(STLB_memory.size, &STLB_memory.mem_id,
                (void**)&STLB_memory.logical, &STLB_memory.physical,
                vipdMEMORY_ALIGN_SIZE, flag, 0, device_vis, 0));
            vipdrv_os_zero_memory(STLB_memory.logical, STLB_memory.size);

            logical = STLB_memory.logical;
            physical = STLB_memory.physical;

            if ((mmu_info->MTLB.physical & 0xFF00000000) != (physical & 0xFF00000000)) {
                PRINTK_E("fail MTLB physical=0x%"PRIx64", STLB physical=0x%"PRIx64"\n",
                    mmu_info->MTLB.physical, physical);
                vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }

            if (VIP_NULL == mmu_info->STLB_memory) {
                status = vipdrv_os_allocate_memory(sizeof(vip_uint32_t) * 2, (void**)&mmu_info->STLB_memory);
                if (VIP_SUCCESS != status) {
                    PRINTK_E("fail to allocate memory for STLB memory array\n");
                    vipOnError(status);
                }
                vipdrv_os_zero_memory(mmu_info->STLB_memory, sizeof(vip_uint32_t) * 2);

                mmu_info->STLB_memory[0] = 2;
                mmu_info->STLB_memory[1] = STLB_memory.mem_id;
            }
            else {
                vip_uint32_t* temp = VIP_NULL;
                vip_uint32_t size = mmu_info->STLB_memory[0];
                vipOnError(vipdrv_os_allocate_memory(sizeof(vip_uint32_t) * (size + 1), (void**)&temp));
                vipdrv_os_zero_memory(temp, sizeof(vip_uint32_t) * (size + 1));

                vipdrv_os_copy_memory(temp, mmu_info->STLB_memory, size * sizeof(vip_uint32_t));
                vipdrv_os_free_memory(mmu_info->STLB_memory);
                mmu_info->STLB_memory = temp;
                mmu_info->STLB_memory[0] = size + 1;
                mmu_info->STLB_memory[size] = STLB_memory.mem_id;
            }
        }
        else {
            vipOnError(ensure_STLB_capacity(mmu_info, type + 1));
            STLB_info = mmu_info->free_list[type + 1].next;
            mmu_info->free_list[type + 1].next = STLB_info->next;
            STLB_info->next = VIP_NULL;

            logical = STLB_info->logical;
            physical = STLB_info->physical;
        }

        status = vipdrv_os_allocate_memory(sizeof(vipdrv_STLB_info_t) * STLB_INFO_CNT,
            (void**)&new_STLB_info);
        if (VIP_SUCCESS != status) {
            PRINTK_E("fail to allocate memory for STLB info array\n");
            vipOnError(status);
        }
        vipdrv_os_zero_memory(new_STLB_info, sizeof(vipdrv_STLB_info_t) * STLB_INFO_CNT);
        if (VIP_NULL == mmu_info->STLB_info) {
            vipOnError(vipdrv_os_allocate_memory(sizeof(void*) * 2, (void**)&mmu_info->STLB_info));
            vipdrv_os_zero_memory(mmu_info->STLB_info, sizeof(void*) * 2);

            mmu_info->STLB_info[0] = VIPDRV_UINT64_TO_PTR(2);
            mmu_info->STLB_info[1] = (void*)new_STLB_info;
        }
        else {
            void** temp = VIP_NULL;
            vip_uint32_t size = VIPDRV_PTR_TO_UINT32(mmu_info->STLB_info[0]);
            vipOnError(vipdrv_os_allocate_memory(sizeof(void*) * (size + 1), (void**)&temp));
            vipdrv_os_zero_memory(temp, sizeof(void*) * (size + 1));

            vipdrv_os_copy_memory(temp, mmu_info->STLB_info, size * sizeof(void*));
            vipdrv_os_free_memory(mmu_info->STLB_info);
            mmu_info->STLB_info = temp;
            mmu_info->STLB_info[0] = VIPDRV_UINT64_TO_PTR(size + 1);
            mmu_info->STLB_info[size] = (void*)new_STLB_info;
        }

        STLB_info = &mmu_info->free_list[type];
        for (i = 0; i < STLB_INFO_CNT; i++) {
            new_STLB_info[i].logical = logical + i * stlb_size;
            new_STLB_info[i].physical = physical + i * stlb_size;
            STLB_info->next = &new_STLB_info[i];
            STLB_info = &new_STLB_info[i];
        }
        STLB_info->next = VIP_NULL;
    }

onError:
    return status;
}

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

vip_char_t* vip_drv_mmu_page_str(
    vipdrv_mmu_page_type_e type
    )
{
    switch (type) {
    case VIPDRV_MMU_16M_PAGE: return "16M";
    case VIPDRV_MMU_1M_PAGE: return "1M";
    case VIPDRV_MMU_64K_PAGE: return "64K";
    case VIPDRV_MMU_4K_PAGE: return "4K";
    default:
        return "None";
    }
}

/*
@brief allocate MMU pages.
@param mmu_info, the mmu information.
@param, page_info, page information
@param, num_stlb, the number of STLB.
@param, specified, 0: any continue num_stlb, return mtlb/stlb start/end index
                   1: specify mtlb/stlb start index, try to get continue num_stlb, return mtlb/stlb end index
*/
static vip_status_e mmu_alloc_page_info(
    vipdrv_mmu_info_t *mmu_info,
    vipdrv_mmu_page_info_t *page_info,
    vip_uint32_t num_stlb,
    vip_bool_e specified,
    vip_bool_e addr_over_4G
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t count = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t default_value = DEFAULT_STLB_ENTRY_VALUE;
    vip_uint32_t mtlb_value = 0;
    vip_uint32_t page_type = 0;
    vip_uint32_t i = 0, j = 0;
    vip_uint32_t mtlb_index_4G = 0x100000000 / vipdMMU_PER_MTLB_MEMORY;

#if !vpmdENABLE_40BITVA
    addr_over_4G = vip_false_e;
#endif

    if ((0 > num_stlb) || (VIP_NULL == page_info)) {
        PRINTK_E("fail to get stlb, parameter is 0 or tlb < 0\n");
        return VIP_ERROR_IO;
    }
    page_type = page_info->page_type;

    vipOnError(vipdrv_mmu_page_macro(page_type, &mtlb_value, &stlb_entry_num, VIP_NULL, VIP_NULL, VIP_NULL, VIP_NULL));

    mtlb_value = (mtlb_value << 2) /* page size */
               | (0 << 1)          /* Ignore exception */
               | (1 << 0);         /* Present */

    if (VIPDRV_MMU_4K_PAGE == page_type) {
        if ((default_value & 0x5) == 0x5) {
            vip_uint32_t physical_low = 0;
            vip_uint32_t physical_high = 0;
            vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
            vip_physical_t physical = context->default_mem.physical;
            if ((0 == (physical & (vipdMMU_PAGE_4K_SIZE - 1))) &&
                (vipdMMU_PAGE_4K_SIZE == context->default_mem.size)) {
                physical_low = (vip_uint32_t)(physical & 0xFFFFFFFF);
                if (physical & 0xFFFFFFFF00000000ULL) {
                    vip_physical_t phy_tmp = physical >> 16;
                    physical_high = (vip_uint32_t)(phy_tmp >> 16);
                    if (physical_high > 0xFF) {
                        physical_high = physical_low = 0;
                    }
                }
                default_value = mmu_set_page(physical_low, physical_high, 1);
            }
        }
    }

    if (vip_false_e == specified) {
        i = (vip_true_e == addr_over_4G) ? mtlb_index_4G : 0;

        for (; i < mmu_info->MTLB_cnt; i++) {
            vipdrv_MTLB_info_t* MTLB_info = &mmu_info->MTLB_info[i];
            if (mtlb_index_4G == i) {
                /* do not across 4G */
                count = 0;
            }

            if (VIPDRV_MMU_NONE_PAGE == MTLB_info->type) {
                if (0 == count) {
                    page_info->mtlb_start = i;
                    page_info->stlb_start = 0;
                }

                if (count + stlb_entry_num >= num_stlb) {
                    page_info->mtlb_end = i;
                    page_info->stlb_end = num_stlb - count - 1;
                    count = num_stlb;
                    break;
                }
                else {
                    count += stlb_entry_num;
                    continue;
                }
            }

            if (page_type == MTLB_info->type) {
                for (j = 0; j < stlb_entry_num; j++) {
                    if ((1 << (j % UINT8_BIT_COUNT)) & MTLB_info->bitmap[j / UINT8_BIT_COUNT]) {
                        /* reset */
                        count = 0;
                    }
                    else {
                        if (0 == count) {
                            page_info->mtlb_start = i;
                            page_info->stlb_start = j;
                        }
                        count++;
                        if (count == num_stlb) {
                            page_info->mtlb_end = i;
                            page_info->stlb_end = j;
                            break;
                        }
                    }
                }
            }
            else {
                count = 0;
            }

            if (count == num_stlb) {
                break;
            }
        }
    }
    else {
        vip_uint32_t stlb_index = page_info->stlb_start;
        vip_uint32_t mtlb_index = page_info->mtlb_start;

        while (mtlb_index < mmu_info->MTLB_cnt) {
            vipdrv_MTLB_info_t* MTLB_info = &mmu_info->MTLB_info[mtlb_index];
            if (VIPDRV_MMU_NONE_PAGE == MTLB_info->type) {
                vip_uint32_t temp = (num_stlb - count) < (stlb_entry_num - stlb_index) ?
                                    (num_stlb - count) : (stlb_entry_num - stlb_index);
                stlb_index += temp;
                count += temp;
            }
            else {
                if (page_type == MTLB_info->type) {
                    if ((1 << (stlb_index % UINT8_BIT_COUNT)) & MTLB_info->bitmap[stlb_index / UINT8_BIT_COUNT]) {
                        break;
                    }
                    stlb_index += 1;
                    count += 1;
                }
                else {
                    break;
                }
            }

            if (count == num_stlb) {
                page_info->stlb_end = stlb_index - 1;
                page_info->mtlb_end = mtlb_index;
                break;
            }

            if (stlb_index >= stlb_entry_num) {
                stlb_index = 0;
                mtlb_index += 1;
                if (0x100000000 / vipdMMU_PER_MTLB_MEMORY == mtlb_index) {
                    /* do not across 4G */
                    count = 0;
                    break;
                }
            }
        }
    }

    if (count < num_stlb) {
        PRINTK_E("fail to get stlb, without enough space\n");
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    for (i = page_info->mtlb_start; i <= page_info->mtlb_end; i++) {
        vipdrv_MTLB_info_t* MTLB_info = &mmu_info->MTLB_info[i];
        vip_uint32_t start = 0, end = stlb_entry_num - 1;
        if (VIPDRV_MMU_NONE_PAGE == MTLB_info->type) {
            vip_uint32_t size = stlb_entry_num * sizeof(vip_uint32_t);
            MTLB_info->type = page_type;
            status = ensure_STLB_capacity(mmu_info, MTLB_info->type);
            if (VIP_SUCCESS != status) {
                MTLB_info->type = VIPDRV_MMU_NONE_PAGE;
                vipOnError(status);
            }
            status = vipdrv_os_allocate_memory(stlb_entry_num / UINT8_BIT_COUNT,
                (void**)&MTLB_info->bitmap);
            if (VIP_SUCCESS != status) {
                PRINTK_E("fail to allocate memory for bitmap of STLB\n");
                MTLB_info->type = VIPDRV_MMU_NONE_PAGE;
                vipOnError(status);
            }
            vipdrv_os_zero_memory(MTLB_info->bitmap, stlb_entry_num / UINT8_BIT_COUNT);

            MTLB_info->STLB = mmu_info->free_list[MTLB_info->type].next;
            mmu_info->free_list[MTLB_info->type].next = MTLB_info->STLB->next;
            MTLB_info->STLB->next = VIP_NULL;
            vipdrv_os_zero_memory(MTLB_info->STLB->logical, size);
            ((vip_uint32_t*)mmu_info->MTLB.logical)[i] = mtlb_value | MTLB_info->STLB->physical;
        }

        if (i == page_info->mtlb_start) {
            start = page_info->stlb_start;
        }
        if (i == page_info->mtlb_end) {
            end = page_info->stlb_end;
        }

        MTLB_info->used_cnt += end - start + 1;
        for (j = start; j <= end; j++) {
            vip_uint32_t* stlb_logical = (vip_uint32_t*)MTLB_info->STLB->logical;
            /* set default value */
            _write_page_entry(stlb_logical + j, default_value);
            /* flag page used */
            MTLB_info->bitmap[j / UINT8_BIT_COUNT] |= (1 << (j % UINT8_BIT_COUNT));
        }
    }

    PRINTK_D("get page type=%s, stlb_num=%d, mtlb: %d ~ %d, stlb: %d ~ %d\n",
        vip_drv_mmu_page_str(page_type), num_stlb, page_info->mtlb_start,
        page_info->mtlb_end, page_info->stlb_start, page_info->stlb_end);

onError:
    return status;
}

/*
@brief free MMU page info.
@param mmu_info, the mmu information.
@param, page_info, page information
*/
static vip_status_e mmu_free_page_info(
    vipdrv_mmu_info_t* mmu_info,
    vipdrv_mmu_page_info_t *page_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0, j = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t page_type = 0;

    if (page_info == VIP_NULL) {
        PRINTK_E("fail to free page info\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    page_type = page_info->page_type;

    vipOnError(vipdrv_mmu_page_macro(page_type, VIP_NULL, &stlb_entry_num, VIP_NULL, VIP_NULL, VIP_NULL, VIP_NULL));

    for (i = page_info->mtlb_start; i <= page_info->mtlb_end; i++) {
        vipdrv_MTLB_info_t* MTLB_info = &mmu_info->MTLB_info[i];
        vip_uint32_t start = 0, end = stlb_entry_num - 1;
        if (i == page_info->mtlb_start) {
            start = page_info->stlb_start;
        }
        if (i == page_info->mtlb_end) {
            end = page_info->stlb_end;
        }

        MTLB_info->used_cnt -= end - start + 1;
        for (j = start; j <= end; j++) {
            vip_uint32_t* stlb_logical = (vip_uint32_t*)MTLB_info->STLB->logical;
            /* set default value */
            _write_page_entry(stlb_logical + j, FREE_STLB_ENTRY_VALUE);
            /* flag page used */
            MTLB_info->bitmap[j / UINT8_BIT_COUNT] &= ~(1 << (j % UINT8_BIT_COUNT));
        }

        if (0 == MTLB_info->used_cnt) {
            MTLB_info->STLB->next = mmu_info->free_list[MTLB_info->type].next;
            mmu_info->free_list[MTLB_info->type].next = MTLB_info->STLB;
            MTLB_info->STLB = VIP_NULL;
            vipdrv_os_free_memory(MTLB_info->bitmap);
            MTLB_info->bitmap = VIP_NULL;
            MTLB_info->type = VIPDRV_MMU_NONE_PAGE;
        }
    }

    PRINTK_D("free page mtlb: %d ~ %d, stlb : %d ~ %d, page type=%s\n",
             page_info->mtlb_start, page_info->mtlb_end, page_info->stlb_start,
             page_info->stlb_end, vip_drv_mmu_page_str(page_type));

onError:
    return status;
}

/*
@brief fill stlb.
@param mmu_info, the mmu information.
@param info mmu page information.
@param physical_num physical count.
@param physical_table physical base address of each physical.
@param size_table size of each physical.
@param writable  1: indicate that the memory is NPU writable. 0: indicate that is read only memory.
*/
static vip_status_e mmu_fill_stlb(
    vipdrv_mmu_info_t* mmu_info,
    vipdrv_mmu_page_info_t *info,
    vip_uint32_t physical_num,
    vip_physical_t* physical_table,
    vip_size_t* size_table,
    vip_uint8_t writable
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t page_size = 0;
    vip_uint32_t page_mask = 0;
    vip_uint32_t page_type = info->page_type;
    vip_uint32_t mtlb_start = info->mtlb_start;
    vip_uint32_t stlb_start = info->stlb_start;

    if (VIP_NULL == mmu_info) {
        PRINTK_E("fail to fill stlb, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    vipOnError(vipdrv_mmu_page_macro(page_type, VIP_NULL, &stlb_entry_num,
        VIP_NULL, VIP_NULL, &page_mask, &page_size));

    for (i = 0; i < physical_num; i++) {
        vip_uint32_t entry_value = 0;
        vip_physical_t physical = physical_table[i] & ~(vip_address_t)page_mask;
        vip_physical_t physical_end = physical_table[i] + size_table[i];
        vip_physical_t physical_low = 0;
        vip_physical_t physical_high = 0;
        vip_uint32_t* stlb_logical = VIP_NULL;
        while (physical < physical_end) {
            physical_low = (vip_uint32_t)(physical & 0xFFFFFFFF);
            if (physical & 0xFFFFFFFF00000000ULL) {
                /* make 32-bits system happy */
                vip_physical_t phy_tmp = physical >> 16;
                physical_high = (vip_uint32_t)(phy_tmp >> 16);
                if (physical_high > 0xFF) {
                    PRINTK_E("fail to fill mem stlb physical=0x%"PRIx64", high=0x%x, low=0x%x\n",
                              physical, physical_high, physical_low);
                    vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
                }
            }
            else {
                physical_high = 0;
            }
            entry_value = mmu_set_page(physical_low, physical_high, writable);
            stlb_logical = (vip_uint32_t*)mmu_info->MTLB_info[mtlb_start].STLB->logical;
            _write_page_entry(stlb_logical + stlb_start, entry_value);

            #if ENABLE_MMU_MAPPING_LOG
            PRINTK("map index=%d, mtlb=%d, stlb=%d, physical=0x%"PRIx64"\n",
                      i, mtlb_start, stlb_start, physical);
            #endif

            physical += page_size;
            stlb_start++;

            if (stlb_start >= stlb_entry_num) {
                mtlb_start++;
                stlb_start = 0;
            }
        }
    }

    return status;
onError:
    VIPDRV_DUMP_STACK();
    return status;
}

/*
@brief mmap axi sram.
*/
static vip_status_e mmu_map_axi_sram(
    vipdrv_mmu_info_t* mmu_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_mmu_page_info_t info = { 0 };
    vip_int32_t num_stlb = 0;
    vip_address_t axi_physical = context->axi_sram_physical;
    vip_uint32_t offset = 0;
    vip_size_t axi_sram_size = 0;
    vip_uint32_t i = 0;
    vip_uint32_t stlb_shift = 0;
    vip_uint32_t stlb_mask = 0;
    vip_uint32_t page_mask = 0;
    vip_uint32_t page_size = 0;
#if vpmdENABLE_40BITVA
    vip_uint32_t page_type = VIPDRV_MMU_16M_PAGE;
#else
    vip_uint32_t page_type = VIPDRV_MMU_1M_PAGE;
#endif
    vipdrv_mmu_page_macro(page_type, VIP_NULL, VIP_NULL, &stlb_shift, &stlb_mask, &page_mask, &page_size);
    offset = (vip_uint32_t)(axi_physical & page_mask);

    for (i = 0; i < context->device_count; i++) {
        axi_sram_size += context->axi_sram_size[i];
    }

    num_stlb = (axi_sram_size + offset + page_size - 1) / page_size;

    if (num_stlb > 0) {
        PRINTK_D("mapping AXI-SRAM size=0x%x..\n", axi_sram_size);
        info.page_type = page_type;
        if (0 == context->axi_sram_address) {
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_false_e, vip_false_e));
            context->axi_sram_address = ((vip_virtual_t)info.mtlb_start << vipdMMU_MTLB_SHIFT) |
                                        ((vip_virtual_t)info.stlb_start << stlb_shift) |
                                        offset;
        }
        else {
            info.mtlb_start = ((vip_virtual_t)context->axi_sram_address & vipdMMU_MTLB_MASK) >> \
                               vipdMMU_MTLB_SHIFT;
            info.stlb_start = ((vip_virtual_t)context->axi_sram_address & stlb_mask) >>  \
                               stlb_shift;
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_true_e, vip_false_e));
        }

        PRINTK_D("axi sram size=0x%xbytes, mtlb: %d  ~ %d, stlb: %d ~ %d, "
                 "physical=0x%08x to virtual=0x%"PRIx64"\n",
                 axi_sram_size, info.mtlb_start, info.mtlb_end, info.stlb_start,
                 info.stlb_end, axi_physical, context->axi_sram_address);

        status = mmu_fill_stlb(mmu_info, &info, 1, &axi_physical, &axi_sram_size, 1);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to fill stlb in map AXI SRAM, status=%d.\n", status);
        }
    }

    return status;

onError:
    PRINTK_E("fail to map AXI-SRAM status=%d.\n", status);
    return status;
}

/*
@brief mmap vip sram.
*/
static vip_status_e mmu_map_vip_sram(
    vipdrv_mmu_info_t* mmu_info
    )
{
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vip_status_e status = VIP_SUCCESS;
    vipdrv_mmu_page_info_t info = { 0 };
    vip_int32_t num_stlb = 0;
    vip_uint32_t i = 0;
    vip_uint32_t vip_sram_size = 0;
    vip_uint32_t stlb_shift = 0;
    vip_uint32_t stlb_mask = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t page_size = 0;
#if vpmdENABLE_40BITVA
    vip_uint32_t page_type = VIPDRV_MMU_16M_PAGE;
#else
    vip_uint32_t page_type = VIPDRV_MMU_1M_PAGE;
#endif
    vipdrv_mmu_page_macro(page_type, VIP_NULL, &stlb_entry_num, &stlb_shift, &stlb_mask, VIP_NULL, &page_size);

    for (i = 0; i < context->device_count; i++) {
        vip_sram_size = context->vip_sram_size[i] > vip_sram_size ?
                        context->vip_sram_size[i] : vip_sram_size;
    }

    num_stlb = (vip_sram_size + page_size - 1) / page_size;
    if (num_stlb > 0) {
        PRINTK_D("mapping VIP-SRAM size=0x%x..\n", vip_sram_size);
        info.page_type = page_type;
        context->vip_sram_address = 0x400000;

        if (0 == context->vip_sram_address) {
            #define MAP_OFFSET_M      4
            num_stlb = num_stlb + MAP_OFFSET_M + RESERVED_PAGE_NUM;
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_false_e, vip_false_e));
            context->vip_sram_address = ((vip_virtual_t)info.mtlb_start << vipdMMU_MTLB_SHIFT) |
                                        (((vip_virtual_t)info.stlb_start + MAP_OFFSET_M) << stlb_shift);
            #undef MAP_OFFSET_M
        }
        else {
            info.mtlb_start = (context->vip_sram_address & vipdMMU_MTLB_MASK) >> vipdMMU_MTLB_SHIFT;
            info.stlb_start = (context->vip_sram_address & stlb_mask) >> stlb_shift;
            /* align to a mtlb block */
            num_stlb = stlb_entry_num * info.mtlb_start + info.stlb_start + num_stlb + RESERVED_PAGE_NUM;
            info.mtlb_start = 0;
            info.stlb_start = 0;
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_true_e, vip_false_e));
        }

        PRINTK_D("vip sram size=0x%xbytes, mtlb : %d ~ %d, stlb : %d ~ %d, virtual=0x%08x\n",
                  vip_sram_size, info.mtlb_start, info.mtlb_end,
                  info.stlb_start, info.stlb_end, context->vip_sram_address);
    }

    return status;

onError:
    PRINTK_E("fail to map VIP-SRAM status=%d.\n", status);
    return status;
}

#if vpmdENABLE_RESERVE_PHYSICAL
static vip_status_e mmu_map_reserve_physical(
    vipdrv_mmu_info_t* mmu_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vip_physical_t reserve_phy_base = context->reserve_phy_base;
    vip_size_t reserve_size = context->reserve_phy_size;
    vip_uint32_t m_start = 0;
    vip_uint32_t s_start = 0;
    vip_int32_t num_stlb = 0;
    vipdrv_mmu_page_info_t info = { 0 };
    vip_uint32_t offset = 0;
    vip_uint32_t stlb_shift = 0;
    vip_uint32_t stlb_mask = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t page_size = 0;
    vip_uint32_t page_mask = 0;
#if vpmdENABLE_40BITVA
    vip_uint32_t page_type = VIPDRV_MMU_16M_PAGE;
#else
    vip_uint32_t page_type = VIPDRV_MMU_1M_PAGE;
#endif
    vipdrv_mmu_page_macro(page_type, VIP_NULL, &stlb_entry_num, &stlb_shift, &stlb_mask, &page_mask, &page_size);

    if (0 == reserve_size) {
        return status;
    }

    if (reserve_size > vipdMMU_TOTAL_SIZE) {
        PRINTK_E("reserve physical size %d is bigger than video memory size %d\n",
                 reserve_size, vipdMMU_TOTAL_SIZE);
        return VIP_ERROR_IO;
    }

    offset = (vip_uint32_t)(reserve_phy_base & page_mask);
    /* 16Mbytes per-MTLB, 1Mbytes per-STLB */
    num_stlb = (reserve_size + offset + page_size - 1) / page_size;

    if (num_stlb > 0) {
        PRINTK_D("mapping reserve physical..\n");
        info.page_type = page_type;
        num_stlb += RESERVED_PAGE_NUM;
        if (0 == context->reserve_virtual) {
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_false_e, vip_false_e));

            m_start = info.mtlb_start;
            s_start = info.stlb_start;
            if ((s_start + RESERVED_PAGE_NUM) >= stlb_entry_num) {
                s_start = s_start + RESERVED_PAGE_NUM - stlb_entry_num;
                m_start += 1;
            }
            else {
                s_start += RESERVED_PAGE_NUM;
            }

            context->reserve_virtual = ((vip_virtual_t)m_start << vipdMMU_MTLB_SHIFT) |
                                       ((vip_virtual_t)s_start << stlb_shift) |
                                       offset;
        }
        else {
            m_start = (context->reserve_virtual & vipdMMU_MTLB_MASK) >> vipdMMU_MTLB_SHIFT;
            s_start = (context->reserve_virtual & stlb_mask) >> stlb_shift;
            if (s_start >= RESERVED_PAGE_NUM) {
                info.mtlb_start = m_start;
                info.stlb_start = s_start - RESERVED_PAGE_NUM;
            }
            else {
                info.mtlb_start = m_start - 1;
                info.stlb_start = s_start + stlb_entry_num - RESERVED_PAGE_NUM;
            }
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_true_e, vip_false_e));
        }

        info.mtlb_start = m_start;
        info.mtlb_end = info.mtlb_end;
        info.stlb_start = s_start;
        info.stlb_end = info.stlb_end;
        info.page_type = page_type;
        PRINTK_D("reserve physical=0x%"PRIx64", vip virtual=0x%08x, "
                 "size=0x%x, mtlb : %d ~ %d, stlb : %d ~ %d\n",
                reserve_phy_base, context->reserve_virtual, reserve_size, m_start,
                info.mtlb_end, s_start, info.stlb_end);

        status = mmu_fill_stlb(mmu_info, &info, 1, &reserve_phy_base, &reserve_size, 1);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to fill stlb in map reserve physical, status=%d.\n", status);
        }
    }

    return status;
onError:
    PRINTK_E("fail to map reserve physical size=0x%x, status=%d.\n", reserve_size, status);
    return status;
}
#endif

/*
@brief mmap reserve memory heap.
*/
#if vpmdENABLE_VIDEO_MEMORY_HEAP
static vip_status_e mmu_map_memory_heap(
    vipdrv_mmu_info_t* mmu_info,
    vipdrv_alloc_mgt_heap_t* heap
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_physical_t heap_phy_base = heap->physical;
    vip_size_t heap_size = heap->total_size;
    vip_uint32_t m_start = 0;
    vip_uint32_t s_start = 0;
    vip_int32_t num_stlb = 0;
    vipdrv_mmu_page_info_t info = { 0 };
    vip_uint32_t offset = 0;
    vip_uint32_t stlb_shift = 0;
    vip_uint32_t stlb_mask = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t page_size = 0;
    vip_uint32_t page_mask = 0;
#if vpmdENABLE_40BITVA
    vip_uint32_t page_type = VIPDRV_MMU_16M_PAGE;
#else
    vip_uint32_t page_type = VIPDRV_MMU_1M_PAGE;
#endif
    vipdrv_mmu_page_macro(page_type, VIP_NULL, &stlb_entry_num, &stlb_shift, &stlb_mask, &page_mask, &page_size);

    if (heap_size > vipdMMU_TOTAL_SIZE) {
        PRINTK_E("video memory heap size %d is bigger than video memory size %d\n",
                  heap_size, vipdMMU_TOTAL_SIZE);
        return VIP_ERROR_IO;
    }

    offset = (vip_uint32_t)(heap_phy_base & page_mask);
    /* 16Mbytes per-MTLB, 1Mbytes per-STLB */
    num_stlb = (heap_size + offset + page_size - 1) / page_size;

    if (num_stlb > 0) {
        PRINTK_D("mapping video memory heap..\n");
        num_stlb += RESERVED_PAGE_NUM;
        info.page_type = page_type;
        if (vip_false_e == heap->mmu_map) {
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_false_e, vip_false_e));

            m_start = info.mtlb_start;
            s_start = info.stlb_start;
            if ((s_start + RESERVED_PAGE_NUM) >= stlb_entry_num) {
                s_start = s_start + RESERVED_PAGE_NUM - stlb_entry_num;
                m_start += 1;
            }
            else {
                s_start += RESERVED_PAGE_NUM;
            }

            heap->virtual = ((vip_virtual_t)m_start << vipdMMU_MTLB_SHIFT) |
                            ((vip_virtual_t)s_start << stlb_shift) |
                            offset;
            heap->mmu_map = vip_true_e;
        }
        else {
            m_start = (heap->virtual & vipdMMU_MTLB_MASK) >> vipdMMU_MTLB_SHIFT;
            s_start = (heap->virtual & stlb_mask) >> stlb_shift;
            if (s_start >= RESERVED_PAGE_NUM) {
                info.mtlb_start = m_start;
                info.stlb_start = s_start - RESERVED_PAGE_NUM;
            }
            else {
                info.mtlb_start = m_start - 1;
                info.stlb_start = s_start + stlb_entry_num - RESERVED_PAGE_NUM;
            }
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_true_e, vip_false_e));
        }

        info.mtlb_start = m_start;
        info.mtlb_end = info.mtlb_end;
        info.stlb_start = s_start;
        info.stlb_end = info.stlb_end;
        info.page_type = page_type;
        PRINTK_D("video memory heap physical=0x%"PRIx64", vip virtual=0x%08x, "
                 "size=0x%"PRIx64", mtlb : %d ~ %d, stlb : %d ~ %d\n",
                heap_phy_base, heap->virtual, heap_size, m_start,
                info.mtlb_end, s_start, info.stlb_end);

        status = mmu_fill_stlb(mmu_info, &info, 1, &heap_phy_base, &heap_size, !VIDEO_MEMORY_HEAP_READ_ONLY);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to fill stlb in map video memory heap\n");
        }

        if ((heap->virtual + heap->total_size) > 0x100000000) {
            heap->allocator_mgt.allocator.mem_flag &= ~VIPDRV_MEM_FLAG_4GB_ADDR_VA;
        }
        else {
            heap->allocator_mgt.allocator.mem_flag |= VIPDRV_MEM_FLAG_4GB_ADDR_VA;
        }
    }

    return status;
onError:
    PRINTK_E("fail to map video memory heap status=%d.\n", status);
    return status;
}
#endif

static vip_status_e mmu_map_mmu_flush(
    vipdrv_mmu_info_t* mmu_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_mmu_page_info_t info = { 0 };
    vip_int32_t num_stlb = 0;
    vip_physical_t flush_address = context->mmu_flush_mem.physical;
    vip_uint32_t offset = 0;
    vip_size_t size = context->mmu_flush_mem.size;
    vip_uint32_t stlb_shift = 0;
    vip_uint32_t stlb_mask = 0;
    vip_uint32_t page_size = 0;
    vip_uint32_t page_mask = 0;
    vipdrv_mmu_page_macro(VIPDRV_MMU_4K_PAGE, VIP_NULL, VIP_NULL, &stlb_shift, &stlb_mask, &page_mask, &page_size);

    offset = (vip_uint32_t)(flush_address & page_mask);
    num_stlb = (size + offset + page_size - 1) / page_size;

    if (num_stlb > 0) {
        info.page_type = VIPDRV_MMU_4K_PAGE;
        if (0 == context->mmu_flush_virtual) {
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_false_e, vip_false_e));

            context->mmu_flush_virtual = ((vip_virtual_t)info.mtlb_start << vipdMMU_MTLB_SHIFT) |
                                         ((vip_virtual_t)info.stlb_start << stlb_shift) |
                                         offset;
        }
        else {
            info.mtlb_start = (context->mmu_flush_virtual & vipdMMU_MTLB_MASK) >> vipdMMU_MTLB_SHIFT;
            info.stlb_start = (context->mmu_flush_virtual & stlb_mask) >> stlb_shift;
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_true_e, vip_false_e));
        }

        status = mmu_fill_stlb(mmu_info, &info, 1, &flush_address, &size, 0);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to fill stlb in map MMU flush, status=%d.\n", status);
        }
    }

    return status;
onError:
    PRINTK_E("fail to map AXI-SRAM status=%d.\n", status);
    return status;
}

static vip_status_e free_mmu_info(
    vipdrv_mmu_info_t* mmu_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t i = 0;
    vipIsNULL(mmu_info);

    if (0 != mmu_info->MTLB.mem_id) {
        vipdrv_mem_free_videomemory(mmu_info->MTLB.mem_id);
        mmu_info->MTLB.mem_id = 0;
    }

    if (VIP_NULL != mmu_info->MTLB_info) {
        for (i = 0; i < mmu_info->MTLB_cnt; i++) {
            vipdrv_MTLB_info_t* MTLB_info = &mmu_info->MTLB_info[i];
            if (VIP_NULL != MTLB_info->bitmap) {
                vipdrv_os_free_memory(MTLB_info->bitmap);
                MTLB_info->bitmap = VIP_NULL;
            }
        }

        vipdrv_os_free_memory(mmu_info->MTLB_info);
        mmu_info->MTLB_info = VIP_NULL;
    }

    if (VIP_NULL != mmu_info->STLB_info) {
        vip_uint32_t size = VIPDRV_PTR_TO_UINT32(mmu_info->STLB_info[0]);
        for (i = 1; i < size; i++) {
            vipdrv_os_free_memory(mmu_info->STLB_info[i]);
            mmu_info->STLB_info[i] = VIP_NULL;
        }

        vipdrv_os_free_memory(mmu_info->STLB_info);
        mmu_info->STLB_info = VIP_NULL;
    }

    if (VIP_NULL != mmu_info->STLB_memory) {
        vip_uint32_t size = mmu_info->STLB_memory[0];
        for (i = 1; i < size; i++) {
            vipdrv_mem_free_videomemory(mmu_info->STLB_memory[i]);
            mmu_info->STLB_memory[i] = 0;
        }

        vipdrv_os_free_memory(mmu_info->STLB_memory);
        mmu_info->STLB_memory = VIP_NULL;
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (mmu_info->mmu_mutex != VIP_NULL) {
        vipdrv_os_destroy_mutex(mmu_info->mmu_mutex);
        mmu_info->mmu_mutex = VIP_NULL;
    }
#endif

onError:
    return status;
}

/*
@brief initialize MMU table and allocate MMU, map vip-sram,
       axi-sram, reseverd memory if you system have this.
*/
vip_status_e vipdrv_mmu_init(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_mmu_table_entry_t *entry = VIP_NULL;
    vip_physical_t physical_tmp = 0;
    vip_uint32_t config = 0;
    vip_uint32_t ext = 0;
    vip_uint32_t alloc_flag = VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS |
                              VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL |
                              VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE |
                              VIPDRV_VIDEO_MEM_ALLOC_MMU_PAGE_MEM;
    vipdrv_mmu_info_t* mmu_info = VIP_NULL;
    vip_uint32_t offset = 0;
    vip_uint64_t device_vis = (~(vip_uint64_t)1) >> (64 - context->device_count);

    if (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_MMU)) {
        PRINTK_E("do not support MMU\n");
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

#if vpmdENABLE_40BITVA
    if (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_MMU_40VA)) {
        PRINTK_E("do not support MMU 40bit VA\n");
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }
#endif

#if !vpmdENABLE_VIDEO_MEMORY_CACHE
    alloc_flag |= VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE;
#endif
    PRINTK_D("MTLB count=%d, 1M entry count pre-MTLB=%d, 4K STLB entry count pre-MTLB=%d\n",
        vipdMMU_MTLB_COUNT, vipdMMU_STLB_1M_ENTRY_NUM, vipdMMU_STLB_4K_ENTRY_NUM);
    PRINTK_D("STLB memory size=%dMbytes, one mtlb size=%dMbytes\n",
        vipdMMU_TOTAL_SIZE / 1024 / 1024, vipdMMU_PER_MTLB_MEMORY / 1024 / 1024);
    if (vipdMMU_MTLB_COUNT > vipdMMU_MTLB_ENTRY_NUM) {
        PRINTK_E("failed init mmu more than max MTLB count\n");
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }

#if vpmdMMU_SINGLE_PAGE
    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_mmu_info_t), (void**)&context->mmu_info));
    vipdrv_os_zero_memory((void*)context->mmu_info, sizeof(vipdrv_mmu_info_t));
#else
    vipOnError(vipdrv_hashmap_init(&context->mmu_hashmap, HASH_MAP_CAPABILITY,
        sizeof(vipdrv_mmu_info_t), "MMU-page", VIP_NULL, (void*)free_mmu_info, (void*)free_mmu_info));
#endif

#if vpmdTASK_PARALLEL_ASYNC
    context->mmu_flush_mem.size = (FLUSH_MMU_STATES_SIZE + LINK_SIZE) * context->core_count * WL_TABLE_COUNT;
#else
    context->mmu_flush_mem.size = (FLUSH_MMU_STATES_SIZE + LINK_SIZE) * context->core_count;
#endif
    vipOnError(vipdrv_mem_allocate_videomemory(context->mmu_flush_mem.size,
        &context->mmu_flush_mem.mem_id,
        (void**)&context->mmu_flush_mem.logical,
        &context->mmu_flush_mem.physical, vipdMMU_PAGE_4K_SIZE,
        VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS |
        VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE |
        VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL |
        VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE, 0, device_vis, 0));
    PRINTK_D("MMU flush memory size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64"\n",
        context->mmu_flush_mem.size, context->mmu_flush_mem.logical, context->mmu_flush_mem.physical);
    vipdrv_os_zero_memory(context->mmu_flush_mem.logical, context->mmu_flush_mem.size);

    context->default_mem.size = vipdMMU_PAGE_4K_SIZE;
    vipOnError(vipdrv_mem_allocate_videomemory(context->default_mem.size,
        &context->default_mem.mem_id,
        (void**)&context->default_mem.logical,
        &context->default_mem.physical, vipdMMU_PAGE_4K_SIZE,
        VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS |
        VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE, 0, device_vis, 0));

    vipOnError(vipdrv_mmu_page_get(0, &mmu_info));

    /* fill flush mmu cache load states */
    VIPDRV_LOOP_HARDWARE_START
    volatile vip_uint32_t* data = VIP_NULL;
    vip_uint32_t count = 0;
    vipdrv_mmu_flush_t* MMU_flush = VIP_NULL;

#if vpmdTASK_PARALLEL_ASYNC
    vip_uint32_t i = 0;
    for (i = 0; i < WL_TABLE_COUNT; i++)
    {
        MMU_flush = &hw->MMU_flush_table[i];
#else
    {
        MMU_flush = &hw->MMU_flush;
#endif

        MMU_flush->logical = (vip_uint8_t*)context->mmu_flush_mem.logical + offset;
        MMU_flush->virtual = context->mmu_flush_virtual + offset;
        PRINTK_D("MMU flush logical=0x%"PRPx", vip virtual=0x%"PRIx64"\n",
            MMU_flush->logical, MMU_flush->virtual);

        data = (volatile vip_uint32_t*)MMU_flush->logical;
        count = 0;
        data[count++] = 0x08010061;
        data[count++] = 0xffffff7f;
        data[count++] = 0x08010e02;
        data[count++] = 0x30000701;
        data[count++] = 0x48000000;
        data[count++] = 0x30000701;
        if (count * sizeof(vip_uint32_t) > FLUSH_MMU_STATES_SIZE) {
            PRINTK_E("flush mmu cache memory overflow %d > %d\n",
                count * sizeof(vip_uint32_t), FLUSH_MMU_STATES_SIZE);
            vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
        }
        data[count++] = 0;
        data[count++] = 0;
        offset += FLUSH_MMU_STATES_SIZE + LINK_SIZE;
    }
    VIPDRV_LOOP_HARDWARE_END

    if (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_MMU_PDMODE)) {
        context->MMU_entry.size = 1024;
        context->MMU_entry.size = VIP_ALIGN(context->MMU_entry.size, vpmdCPU_CACHE_LINE_SIZE);
        vipOnError(vipdrv_mem_allocate_videomemory(context->MMU_entry.size,
            &context->MMU_entry.mem_id,
            (void**)&context->MMU_entry.logical,
            &context->MMU_entry.physical,
            vipdMEMORY_ALIGN_SIZE,
            alloc_flag, 0, device_vis, 0));
        PRINTK_D("MMU entry size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64"\n",
            context->MMU_entry.size, context->MMU_entry.logical, context->MMU_entry.physical);
        vipdrv_os_zero_memory(context->MMU_entry.logical, context->MMU_entry.size);

        config = (vip_uint32_t)(mmu_info->MTLB.physical & 0xFFFFFFFF);
        /* make 32-bits system happy */
        physical_tmp = mmu_info->MTLB.physical >> 16;
        ext = (vip_uint32_t)((physical_tmp >> 16) & 0xFFFFFFFF);

        /* more than 40bit physical address */
        if (ext & 0xFFFFFF00) {
            PRINTK_E("fail to init mmu, more than 40bits physical address\n");
            vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
        }

        #if vpmdENABLE_40BITVA
        config |= 0; /* 4K mode */
        #else
        config |= 1; /* 1K mode */
        #endif
        entry = (vipdrv_mmu_table_entry_t*)context->MMU_entry.logical;
        entry->low = config;
        entry->high = ext;
    }

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(mmu_info->MTLB.mem_id, VIPDRV_CACHE_FLUSH);
	if (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_MMU_PDMODE)) {
    	vipdrv_mem_flush_cache(context->MMU_entry.mem_id, VIPDRV_CACHE_FLUSH);
	}

    if (VIP_NULL != mmu_info->STLB_memory) {
        vip_uint32_t i = 0;
        vip_uint32_t stlb_mem_cnt = mmu_info->STLB_memory[0] - 1;
        for (i = 0; i < stlb_mem_cnt; i++) {
            vipdrv_mem_flush_cache(mmu_info->STLB_memory[i + 1], VIPDRV_CACHE_FLUSH);
        }
    }
#endif

    vipdrv_mmu_page_put(0);

    VIPDRV_LOOP_HARDWARE_START
    hw->flush_mmu = vip_true_e;
    VIPDRV_LOOP_HARDWARE_END

    return status;

onError:
    return status;
}

/*
@brief un-initialize MMU.
*/
vip_status_e vipdrv_mmu_uninit(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

    if (0 != context->mmu_flush_mem.mem_id) {
        vipdrv_mem_free_videomemory(context->mmu_flush_mem.mem_id);
        context->mmu_flush_mem.mem_id = 0;
    }
    if (0 != context->MMU_entry.mem_id) {
        vipdrv_mem_free_videomemory(context->MMU_entry.mem_id);
        context->MMU_entry.mem_id = 0;
    }
    if (0 != context->default_mem.mem_id) {
        vipdrv_mem_free_videomemory(context->default_mem.mem_id);
        context->default_mem.mem_id = 0;
    }
    vipdrv_mmu_page_uninit(0);

#if vpmdMMU_SINGLE_PAGE
    vipdrv_os_free_memory((void*)context->mmu_info);
#else
    vipdrv_hashmap_destroy(&context->mmu_hashmap);
#endif

    return status;
}

/*
@brief map dynamic memory to VIP virtual for all pages are 4Kbytes
@param mmu_info, the mmu information.
@param OUT page_info, MMU page information for this memory.
@param IN physical_table, physical address table that need to be mapped.
@param IN physical_num, the number of physical address.
@param IN size_table, the size of memory in physical_table.
@param IN page type, MMU page type.
@param OUT virtual_addr, VIP virtual address.
*/
static vip_status_e vipdrv_mmu_map_memory(
    vipdrv_mmu_info_t* mmu_info,
    vipdrv_mmu_page_info_t *page_info,
    vip_physical_t *physical_table,
    vip_uint32_t physical_num,
    vip_size_t *size_table,
    vip_uint8_t page_type,
    vip_virtual_t *virtual_addr,
    vip_uint8_t writable,
    vip_uint8_t addr_4G
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t num_stlb = 0;
    vip_uint32_t i = 0;
    vip_size_t total_size = 0;
    vip_uint32_t head_margin = 0;
    vip_size_t tail_margin = 0;
    vipdrv_mmu_page_info_t info = { 0 };
    vip_uint32_t mtlb_start = 0;
    vip_uint32_t stlb_start = 0;
    vip_uint32_t page_size = 0;
    vip_uint32_t stlb_entry_num = 0;
    vip_uint32_t stlb_shift = 0;
    vip_uint32_t page_mask = 0;
    vip_uint32_t stlb_mask = 0;

    vipOnError(vipdrv_mmu_page_macro(page_type, VIP_NULL, &stlb_entry_num,
        &stlb_shift, &stlb_mask, &page_mask, &page_size));

    head_margin = (vip_uint32_t)physical_table[0] & page_mask;
    total_size += head_margin;
    total_size += size_table[0];
    for (i = 1; i < physical_num; i++) {
        total_size += size_table[i];
        if (i == physical_num - 1) {
            tail_margin = VIP_ALIGN(size_table[i], page_size) - size_table[i];
            total_size += tail_margin;
        }
    }

    if (1 == physical_num) {
        PRINTK_D("mmu map memory physical=0x%"PRIx64", size=0x%"PRIx64"bytes, total_size=0x%llx, offset=0x%x\n",
                  physical_table[0], size_table[0], total_size, head_margin);
        tail_margin = VIP_ALIGN(physical_table[0] + size_table[0], page_size) -
            physical_table[0] - size_table[0];
        total_size += tail_margin;
    }

    if (total_size & page_mask) {
        PRINTK_E("fail to mmu page table, size not alignment to 0x%x bytes\n", page_size);
        vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
    }
    num_stlb = total_size / page_size;

    if (num_stlb > 0) {
        /* reserved page at the front and end page-section */
        if (head_margin < vipdMMU_PAGE_4K_SIZE) {
            num_stlb += RESERVED_PAGE_NUM;
        }
        if (tail_margin < vipdMMU_PAGE_4K_SIZE) {
            num_stlb += RESERVED_PAGE_NUM;
        }
        info.page_type = page_type;
        if (0 == *virtual_addr) {
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_false_e,
                (addr_4G ? vip_false_e : vip_true_e)));

            if (info.mtlb_end >= vipdMMU_MTLB_COUNT) {
                PRINTK_E("MMU MTLB size %d is greater than max mtlb size %d, page type=%d\n",
                    info.mtlb_end, vipdMMU_MTLB_COUNT, page_type);
                vipGoOnError(VIP_ERROR_NOT_SUPPORTED);
            }

            mtlb_start = info.mtlb_start;
            stlb_start = info.stlb_start;

            if (head_margin < vipdMMU_PAGE_4K_SIZE) {
                if ((info.stlb_start + RESERVED_PAGE_NUM) >= stlb_entry_num) {
                    stlb_start = info.stlb_start + RESERVED_PAGE_NUM - stlb_entry_num;
                    mtlb_start += 1;
                }
                else {
                    stlb_start += RESERVED_PAGE_NUM;
                }
            }

            *virtual_addr = ((vip_virtual_t)mtlb_start << vipdMMU_MTLB_SHIFT) |
                            ((vip_virtual_t)stlb_start << stlb_shift) |
                            ((vip_uint32_t)physical_table[0] & page_mask);
        }
        else {
            mtlb_start = (*virtual_addr & vipdMMU_MTLB_MASK) >> vipdMMU_MTLB_SHIFT;
            stlb_start = (*virtual_addr & stlb_mask) >> stlb_shift;
            if (head_margin < vipdMMU_PAGE_4K_SIZE) {
                if (stlb_start >= RESERVED_PAGE_NUM) {
                    info.mtlb_start = mtlb_start;
                    info.stlb_start = stlb_start - RESERVED_PAGE_NUM;
                }
                else {
                    info.mtlb_start = mtlb_start - 1;
                    info.stlb_start = stlb_start + stlb_entry_num - RESERVED_PAGE_NUM;
                }
            }
            else {
                info.mtlb_start = mtlb_start;
                info.stlb_start = stlb_start;
            }
            vipOnError(mmu_alloc_page_info(mmu_info, &info, num_stlb, vip_true_e, vip_false_e));
        }

        page_info->mtlb_start = info.mtlb_start;
        page_info->stlb_start = info.stlb_start;
        page_info->mtlb_end = info.mtlb_end;
        page_info->stlb_end = info.stlb_end;
        page_info->page_type = info.page_type;

        #if ENABLE_MMU_MAPPING_LOG
        PRINTK("virtual=0x%"PRIx64", mtlb_start=%d, stlb_start=%d, phy_start=0x%"PRIx64", offset=0x%x\n",
                *virtual_addr, mtlb_start, stlb_start, physical_table[0],
                (vip_uint32_t)physical_table[0] & page_mask);
        #endif

        info.mtlb_start = mtlb_start;
        info.stlb_start = stlb_start;
        vipOnError(mmu_fill_stlb(mmu_info, &info, physical_num, physical_table, size_table, writable));
    }
    else {
        PRINTK_E("fail to map mmu dyn memory, stlb_num=%d\n", num_stlb);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    PRINTK_D("mmu map memory, physical count =%d, mtlb: %d ~ %d, stlb: %d ~ %d, virtual=0x%"PRIx64"\n",
             physical_num, page_info->mtlb_start, page_info->mtlb_end,
             page_info->stlb_start, page_info->stlb_end, *virtual_addr);
    /* sanity check */
    if ((*virtual_addr + total_size) > vipdMMU_TOTAL_SIZE) {
        PRINTK_E("fail to map mmu, virtual overflow, virtual_addr=0x%"PRIx64", total_size=0x%x\n",
                 *virtual_addr, total_size);
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }

    return status;

onError:
    PRINTK_E("fail to get vip virtual in mapping dynamic memory, status=%d\n", status);
    return status;
}

static vip_uint64_t adjust_mmu_id(
    vip_uint64_t mmu_id
    )
{
#if vpmdMMU_SINGLE_PAGE
    mmu_id = 0;
#elif vpmdMMU_PER_PROCESS
    mmu_id &= 0xffffffff00000000;
#endif

    if (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_MMU_PDMODE)) {
        mmu_id = 0;
    }

    return mmu_id;
}

/*
@brief map dynamic memory to VIP virtual.
@param OUT mmu_handle, MMU page information for this memory. an Anonymous pointer for MMU.
@param IN physical_table, physical address table that need to be mapped.
@param IN physical_num, the number of physical address.
@param IN size_table, the size of memory in physical_table.
@param IN page_type, vip mmu page table size, 1M or 4K byte.
@param OUT virtual_addr, VIP virtual address.
@param IN writable, if set to 1, indicate mapping writable MMU page.
*/
vip_status_e vipdrv_mmu_map(
    void **mmu_handle,
    vip_physical_t *physical_table,
    vip_uint32_t physical_num,
    vip_size_t *size_table,
    vip_virtual_t *virtual_addr,
    vip_uint64_t mmu_id,
    vip_uint8_t writable,
    vip_uint8_t addr_4G
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_mmu_info_t* mmu_info = VIP_NULL;
    vipdrv_mmu_page_info_t *page_info = VIP_NULL;
#if vpmdENABLE_40BITVA
    vipdrv_mmu_page_type_e page_types[] = { VIPDRV_MMU_16M_PAGE, VIPDRV_MMU_1M_PAGE,
        VIPDRV_MMU_64K_PAGE, VIPDRV_MMU_4K_PAGE };
#else
    vipdrv_mmu_page_type_e page_types[] = { VIPDRV_MMU_1M_PAGE, VIPDRV_MMU_64K_PAGE, VIPDRV_MMU_4K_PAGE };
#endif
    vipdrv_mmu_page_type_e page_type = VIPDRV_MMU_NONE_PAGE;
    vip_uint32_t i = 0;
    vip_uint32_t page_size = 0;
    vip_uint32_t page_mask = 0;

    if ((virtual_addr == VIP_NULL) || (mmu_handle == VIP_NULL) || (physical_table == VIP_NULL)) {
        PRINTK_E("map dyn memory parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (VIP_SUCCESS != vipdrv_mmu_page_get(mmu_id, &mmu_info)) {
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (0 == mmu_info->MTLB.mem_id) {
        PRINTK_E("mmu page table is NULL MTLB=0x%x, STLB=0x%x\n",
            mmu_info->MTLB.mem_id, 0);
        vipdrv_mmu_page_put(mmu_id);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = vipdrv_os_lock_mutex(mmu_info->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock mmu mutex\n");
        vipdrv_mmu_page_put(mmu_id);
        return VIP_ERROR_FAILURE;
    }
#endif

    vipOnError(vipdrv_os_allocate_memory(sizeof(vipdrv_mmu_page_info_t), (void**)&page_info));
    vipdrv_os_zero_memory(page_info, sizeof(vipdrv_mmu_page_info_t));
    *mmu_handle = (void*)page_info;

    for (i = 0; i < sizeof(page_types) / sizeof(vipdrv_mmu_page_type_e); i++) {
        vipOnError(vipdrv_mmu_page_macro(page_types[i], VIP_NULL, VIP_NULL,
            VIP_NULL, VIP_NULL, &page_mask, &page_size));
        if (1 == physical_num) {
            if (VIPDRV_MMU_4K_PAGE == page_types[i] || size_table[0] > (page_size / 2)) {
                /* find */
                page_type = page_types[i];
                break;
            }
        }
        else {
            vip_uint32_t index = 0;
            if ((physical_table[0] + size_table[0]) & page_mask) {
                continue;
            }

            for (index = 1; index < physical_num; index++) {
                if (physical_table[index] & page_mask) {
                    break;
                }
                if (index != physical_num - 1) {
                    if (size_table[index] % page_size) {
                        break;
                    }
                }
            }

            if (index == physical_num) {
                /* find */
                page_type = page_types[i];
                break;
            }
        }
    }

    if (VIPDRV_MMU_NONE_PAGE == page_type) {
        PRINTK_E("mmu map failed, no suitable page type\n");
        PRINTK("mmu dump physical table, physical_num=%d\n", physical_num);
        for (i = 0; i < physical_num; i++) {
            PRINTK("%d, physical=0x%"PRIx64", size=0x%"PRIx64"\n", i, physical_table[i], size_table[i]);
        }
        vipGoOnError(VIP_ERROR_INVALID_ARGUMENTS);
    }

    PRINTK_D("mmu map physical_num=%d, page_type=%s\n", physical_num, vip_drv_mmu_page_str(page_type));

    vipOnError(vipdrv_mmu_map_memory(mmu_info, page_info, physical_table, physical_num,
        size_table, page_type, virtual_addr, writable, addr_4G));

    if ((*virtual_addr & page_mask) != (physical_table[0] & page_mask)) {
        PRINTK_E("fail to map mmu, physical_base=0x%"PRIx64", virutal=0x%x\n",
            physical_table[0], *virtual_addr);
        vipGoOnError(VIP_ERROR_OUT_OF_MEMORY);
    }
#if vpmdENABLE_MULTIPLE_TASK
    if (VIP_SUCCESS != vipdrv_os_unlock_mutex(mmu_info->mmu_mutex)) {
        PRINTK_E("fail to unlock mmu mutex\n");
    }
#endif

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(mmu_info->MTLB.mem_id, VIPDRV_CACHE_FLUSH);
    if (VIP_NULL != mmu_info->STLB_memory) {
        vip_uint32_t stlb_mem_cnt = mmu_info->STLB_memory[0] - 1;
        for (i = 0; i < stlb_mem_cnt; i++) {
            vipdrv_mem_flush_cache(mmu_info->STLB_memory[i + 1], VIPDRV_CACHE_FLUSH);
        }
    }
#endif

    vipdrv_mmu_page_put(mmu_id);

    VIPDRV_LOOP_HARDWARE_START
    if (adjust_mmu_id(mmu_id) == adjust_mmu_id(hw->cur_mmu_id)) {
        hw->flush_mmu = vip_true_e;
    }
    VIPDRV_LOOP_HARDWARE_END

    return status;
onError:
#if vpmdENABLE_MULTIPLE_TASK
    vipdrv_os_unlock_mutex(mmu_info->mmu_mutex);
#endif
    vipdrv_mmu_page_put(mmu_id);
    PRINTK_E("fail to map mmu\n");
    return status;
}

/*
@brief un-map MMU page and free resource for dynamic allocate memory.
@param mmu_handle, MMU page information for this memory. a Anonymous pointer for MMU.
*/
vip_status_e vipdrv_mmu_unmap(
    void *mmu_handle,
    vip_uint64_t mmu_id
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_mmu_info_t* mmu_info = VIP_NULL;
    vipdrv_mmu_page_info_t *page_info = (vipdrv_mmu_page_info_t*)mmu_handle;

    if (VIP_NULL == page_info) {
        PRINTK_E("fail to unmap page table mmory, page info is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    if (VIP_SUCCESS != vipdrv_mmu_page_get(mmu_id, &mmu_info)) {
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = vipdrv_os_lock_mutex(mmu_info->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock mmu mutex\n");
        vipdrv_mmu_page_put(mmu_id);
        return VIP_ERROR_FAILURE;
    }
#endif

    vipOnError(mmu_free_page_info(mmu_info, page_info));

#if vpmdENABLE_MULTIPLE_TASK
    if (VIP_SUCCESS != vipdrv_os_unlock_mutex(mmu_info->mmu_mutex)) {
        PRINTK_E("fail to unlock mmu mutex\n");
    }
#endif

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(mmu_info->MTLB.mem_id, VIPDRV_CACHE_FLUSH);
    if (VIP_NULL != mmu_info->STLB_memory) {
        vip_uint32_t i = 0;
        vip_uint32_t stlb_mem_cnt = mmu_info->STLB_memory[0] - 1;
        for (i = 0; i < stlb_mem_cnt; i++) {
            vipdrv_mem_flush_cache(mmu_info->STLB_memory[i + 1], VIPDRV_CACHE_FLUSH);
        }
    }
#endif

    VIPDRV_LOOP_HARDWARE_START
    if (adjust_mmu_id(mmu_id) == adjust_mmu_id(hw->cur_mmu_id)) {
        hw->flush_mmu = vip_true_e;
    }
    VIPDRV_LOOP_HARDWARE_END

    vipdrv_os_free_memory(page_info);
    return status;
onError:
#if vpmdENABLE_MULTIPLE_TASK
    vipdrv_os_unlock_mutex(mmu_info->mmu_mutex);
#endif
    vipdrv_os_free_memory(page_info);
    PRINTK_E("fail to unmap mmu\n");
    return status;
}

#if vpmdENABLE_CAPTURE_MMU
vip_status_e vipdrv_mmu_query_mmu_info(
    vipdrv_query_mmu_info_t* info
    )
{
    vip_uint32_t i = 0;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vipdrv_mmu_info_t* mmu_info = VIP_NULL;
    vip_uint32_t stlb_mem_cnt = 0;

    vipdrv_mmu_page_get(gen_mmu_id(vipdrv_os_get_pid(), 0), &mmu_info);

    if (0 == mmu_info->MTLB.mem_id) {
        PRINTK_E("fail to get mmu info, page table is NULL\n");
        vipdrv_mmu_page_put(gen_mmu_id(vipdrv_os_get_pid(), 0));
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    info->entry_physical = context->MMU_entry.physical;
    info->entry_size = context->MMU_entry.size;
    info->entry_mem_id = context->MMU_entry.mem_id;

    info->mtlb_physical = mmu_info->MTLB.physical;
    info->mtlb_size = mmu_info->MTLB.size;
    info->mtlb_mem_id = mmu_info->MTLB.mem_id;

    PRINTK_I("entry size=0x%x, mtlb size=0x%x\n", context->MMU_entry.size, mmu_info->MTLB.size);
    stlb_mem_cnt = mmu_info->STLB_memory[0] - 1;
    stlb_mem_cnt = stlb_mem_cnt <= MAX_STLB_MEM_CNT ? stlb_mem_cnt : MAX_STLB_MEM_CNT;
    for (i = 0; i < stlb_mem_cnt; i++) {
        info->stlb_mem_id[i] = mmu_info->STLB_memory[i + 1];
        vipdrv_mem_get_info(context, info->stlb_mem_id[i], VIP_NULL,
            &info->stlb_physical[i], &info->stlb_size[i], VIP_NULL);
        PRINTK_I("stlb %d size=0x%x\n", i, info->stlb_size[i]);
    }

    vipdrv_mmu_page_put(gen_mmu_id(vipdrv_os_get_pid(), 0));
    return VIP_SUCCESS;
}
#endif

#if vpmdENABLE_VIDEO_MEMORY_HEAP
/* try to map the heaps that follow first */
static vip_status_e vipdrv_mmu_map_heap_list(
    vipdrv_mmu_info_t* mmu_info,
    vipdrv_alloc_mgt_t* alloc_mgt
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_alloc_mgt_heap_t* heap = (vipdrv_alloc_mgt_heap_t*)alloc_mgt;

    if (VIP_NULL == alloc_mgt) {
        return status;
    }
    if (!(alloc_mgt->allocator.alctor_type & VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP)) {
        return status;
    }

    vipOnError(vipdrv_mmu_map_heap_list(mmu_info, (vipdrv_alloc_mgt_t*)alloc_mgt->next));
    if (0x0 == (heap->allocator_mgt.allocator.mem_flag & VIPDRV_MEM_FLAG_MMU_PAGE_MEM)) {
        status = mmu_map_memory_heap(mmu_info, heap);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to map video memory heap, status=%d\n", status);
            vipGoOnError(VIP_ERROR_IO);
        }
    }

onError:
    return status;
}
#endif

static vip_status_e vipdrv_mmu_page_init(
    vipdrv_mmu_info_t* mmu_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t alloc_flag = VIPDRV_VIDEO_MEM_ALLOC_CONTIGUOUS |
                              VIPDRV_VIDEO_MEM_ALLOC_MAP_KERNEL |
                              #if !vpmdENABLE_VIDEO_MEMORY_CACHE
                              VIPDRV_VIDEO_MEM_ALLOC_NONE_CACHE |
                              #endif
                              VIPDRV_VIDEO_MEM_ALLOC_NO_MMU_PAGE |
                              VIPDRV_VIDEO_MEM_ALLOC_MMU_PAGE_MEM;
    vip_uint32_t mtlb_align = (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_MMU_PDMODE)) ? 1024 : 4096;
    vip_uint32_t *device_cnt = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_DEVICE_CNT);
    vip_uint64_t device_vis = (~(vip_uint64_t)1) >> (64 - *device_cnt);

    if (0 != mmu_info->MTLB.mem_id) {
        return VIP_SUCCESS;
    }

#if vpmdENABLE_MULTIPLE_TASK
    status = vipdrv_os_create_mutex(&mmu_info->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to create mutex for mmu\n");
        vipGoOnError(VIP_ERROR_IO);
    }
    status = vipdrv_os_lock_mutex(mmu_info->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock mmu mutex during init\n");
        vipGoOnError(VIP_ERROR_IO);
    }
#endif

    /* allocate memory for MTLB, STLB */
    mmu_info->MTLB_cnt = vipdMMU_MTLB_COUNT;
    vipOnError(vipdrv_os_allocate_memory(mmu_info->MTLB_cnt * sizeof(vipdrv_MTLB_info_t),
        (void**)&mmu_info->MTLB_info));
    vipdrv_os_zero_memory((void*)(mmu_info->MTLB_info), mmu_info->MTLB_cnt * sizeof(vipdrv_MTLB_info_t));
    mmu_info->MTLB.size = vipdMMU_MTLB_COUNT * sizeof(vip_uint32_t);
    mmu_info->MTLB.size = VIP_ALIGN(mmu_info->MTLB.size, vpmdCPU_CACHE_LINE_SIZE);
    vipOnError(vipdrv_mem_allocate_videomemory(mmu_info->MTLB.size,
                                              &mmu_info->MTLB.mem_id,
                                              (void **)&mmu_info->MTLB.logical,
                                              &mmu_info->MTLB.physical,
                                              mtlb_align,
                                              alloc_flag, 0, device_vis, 0));
    PRINTK_D("MTLB size=%dbytes, logical=0x%"PRPx", physical=0x%"PRIx64", mtlb_align=%d\n",
        mmu_info->MTLB.size, mmu_info->MTLB.logical, mmu_info->MTLB.physical, mtlb_align);
    if ((mmu_info->MTLB.physical & (mtlb_align - 1)) > 0) {
        PRINTK_E("MTLB physical not align to %d\n", mtlb_align);
        vipGoOnError(VIP_ERROR_IO);
    }
    vipdrv_os_zero_memory(mmu_info->MTLB.logical, mmu_info->MTLB.size);

    status = mmu_map_vip_sram(mmu_info);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to map vip sram, status=%d.\n", status);
        vipGoOnError(status);
    }

    status = mmu_map_axi_sram(mmu_info);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to map axi sram, status=%d.\n", status);
        vipGoOnError(status);
    }

#if vpmdENABLE_RESERVE_PHYSICAL
    vipOnError(mmu_map_reserve_physical(mmu_info));
#endif

    status = mmu_map_mmu_flush(mmu_info);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to map mmu flush, status=%d.\n", status);
        vipGoOnError(status);
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    {
        vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
        vipOnError(vipdrv_mmu_map_heap_list(mmu_info, (vipdrv_alloc_mgt_t*)context->alloc_list_head));
    }
#endif

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(mmu_info->MTLB.mem_id, VIPDRV_CACHE_FLUSH);
    if (VIP_NULL != mmu_info->STLB_memory) {
        vip_uint32_t i = 0;
        vip_uint32_t stlb_mem_cnt = mmu_info->STLB_memory[0] - 1;
        for (i = 0; i < stlb_mem_cnt; i++) {
            vipdrv_mem_flush_cache(mmu_info->STLB_memory[i + 1], VIPDRV_CACHE_FLUSH);
        }
    }
#endif

onError:
#if vpmdENABLE_MULTIPLE_TASK
    if (VIP_NULL != mmu_info->mmu_mutex) {
        vipdrv_os_unlock_mutex(mmu_info->mmu_mutex);
    }
#endif
    return status;
}

vip_status_e vipdrv_mmu_page_get(
    vip_uint64_t mmu_id,
    vipdrv_mmu_info_t **mmu_info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);

#if vpmdMMU_SINGLE_PAGE
    *mmu_info = context->mmu_info;
    vipOnError(vipdrv_mmu_page_init(*mmu_info));
#else
    vip_uint32_t index = 0;
    mmu_id = adjust_mmu_id(mmu_id);
Expand:
    if (vip_true_e == vipdrv_hashmap_full(&context->mmu_hashmap)) {
        vip_bool_e expand = vip_false_e;
        status = vipdrv_hashmap_expand(&context->mmu_hashmap, &expand);
        if (VIP_SUCCESS != status) {
            PRINTK_E("mmu page is full, not support so many pages\n");
            vipGoOnError(VIP_ERROR_FAILURE);
        }
    }
    status = vipdrv_hashmap_insert(&context->mmu_hashmap, mmu_id, &index);
    if (VIP_SUCCESS == status || VIP_ERROR_INVALID_ARGUMENTS == status) {
        vipdrv_hashmap_get_by_index(&context->mmu_hashmap, index, (void**)mmu_info);
    }

    if (VIP_SUCCESS == status) {
        PRINTK_D("mmu page init mmu_id=0x%"PRIx64", index=%d\n", mmu_id, index);
        status = vipdrv_mmu_page_init(*mmu_info);
        if (VIP_SUCCESS != status) {
            vipdrv_hashmap_unuse(&context->mmu_hashmap, index, vip_false_e);
            vipGoOnError(status);
        }
    }
    else {
        if (VIP_ERROR_OUT_OF_RESOURCE == status) {
            goto Expand;
        }
        else if (VIP_ERROR_INVALID_ARGUMENTS == status) {
            volatile vip_uint32_t* mem_id = &(*mmu_info)->MTLB.mem_id;
            /* wait current mmu page has been created */
            while (0 == *mem_id) {
                vipdrv_os_udelay(10);
            }
            vipdrv_os_lock_mutex((*mmu_info)->mmu_mutex);
            vipdrv_os_unlock_mutex((*mmu_info)->mmu_mutex);
            status = VIP_SUCCESS;
        }
        else {
            vipGoOnError(status);
        }
    }
#endif

onError:
    return status;
}

vip_status_e vipdrv_mmu_page_put(
    vip_uint64_t mmu_id
    )
{
#if vpmdMMU_SINGLE_PAGE
    return VIP_SUCCESS;
#else
    vip_uint32_t index = 0;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    mmu_id = adjust_mmu_id(mmu_id);
    if (VIP_ERROR_INVALID_ARGUMENTS != vipdrv_hashmap_insert(&context->mmu_hashmap, mmu_id, &index)) {
        return VIP_SUCCESS;
    }
    vipdrv_hashmap_unuse(&context->mmu_hashmap, index, vip_false_e);
    return VIP_SUCCESS;
#endif
}

vip_status_e vipdrv_mmu_page_uninit(
    vip_uint64_t mmu_id
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t new_mmu_id = adjust_mmu_id(mmu_id);

    if (new_mmu_id == mmu_id) {
        vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
        /* free mmu info by its creator */
        #if vpmdMMU_SINGLE_PAGE
        free_mmu_info(context->mmu_info);
        #else
        vipdrv_hashmap_remove(&context->mmu_hashmap, mmu_id, vip_false_e);
        #endif
    }
    return status;
}

vip_status_e vipdrv_mmu_page_macro(
    vipdrv_mmu_page_type_e type,
    vip_uint32_t* mtlb_value,
    vip_uint32_t* stlb_entry_num,
    vip_uint32_t* stlb_shift,
    vip_uint32_t* stlb_mask,
    vip_uint32_t* page_mask,
    vip_uint32_t* page_size
    )
{
    vip_uint32_t _mtlb_value = 0;
    vip_uint32_t _stlb_entry_num = 0;
    vip_uint32_t _stlb_shift = 0;
    vip_uint32_t _stlb_mask = 0;
    vip_uint32_t _page_mask = 0;
    vip_uint32_t _page_size = 0;

    switch (type) {
    case VIPDRV_MMU_16M_PAGE:
    {
        _mtlb_value = 3;
        _stlb_entry_num = vipdMMU_STLB_16M_ENTRY_NUM;
        _page_size = vipdMMU_PAGE_16M_SIZE;
        _stlb_shift = vipdMMU_STLB_16M_SHIFT;
        _page_mask = vipdMMU_PAGE_16M_MASK;
        _stlb_mask = vipdMMU_STLB_16M_MASK;
    }
    break;

    case VIPDRV_MMU_1M_PAGE:
    {
        _mtlb_value = 2;
        _stlb_entry_num = vipdMMU_STLB_1M_ENTRY_NUM;
        _page_size = vipdMMU_PAGE_1M_SIZE;
        _stlb_shift = vipdMMU_STLB_1M_SHIFT;
        _page_mask = vipdMMU_PAGE_1M_MASK;
        _stlb_mask = vipdMMU_STLB_1M_MASK;
    }
    break;

    case VIPDRV_MMU_64K_PAGE:
    {
        _mtlb_value = 1;
        _stlb_entry_num = vipdMMU_STLB_64K_ENTRY_NUM;
        _page_size = vipdMMU_PAGE_64K_SIZE;
        _stlb_shift = vipdMMU_STLB_64K_SHIFT;
        _page_mask = vipdMMU_PAGE_64K_MASK;
        _stlb_mask = vipdMMU_STLB_64K_MASK;
    }
    break;

    case VIPDRV_MMU_4K_PAGE:
    {
        _mtlb_value = 0;
        _stlb_entry_num = vipdMMU_STLB_4K_ENTRY_NUM;
        _page_size = vipdMMU_PAGE_4K_SIZE;
        _stlb_shift = vipdMMU_STLB_4K_SHIFT;
        _page_mask = vipdMMU_PAGE_4K_MASK;
        _stlb_mask = vipdMMU_STLB_4K_MASK;
    }
    break;

    default:
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    if (VIP_NULL != mtlb_value) {
        *mtlb_value = _mtlb_value;
    }

    if (VIP_NULL != stlb_entry_num) {
        *stlb_entry_num = _stlb_entry_num;
    }

    if (VIP_NULL != page_size) {
        *page_size = _page_size;
    }

    if (VIP_NULL != stlb_shift) {
        *stlb_shift = _stlb_shift;
    }

    if (VIP_NULL != page_mask) {
        *page_mask = _page_mask;
    }

    if (VIP_NULL != stlb_mask) {
        *stlb_mask = _stlb_mask;
    }


    return VIP_SUCCESS;
}

vip_status_e vipdrv_mmu_split_heap(
    vipdrv_videomem_heap_info_t *heap_info,
    vipdrv_videomem_heap_info_t *split_info
    )
{
    vip_size_t size = sizeof(vip_uint32_t) * vipdMMU_STLB_4K_ENTRY_NUM * STLB_INFO_CNT;
    size = (((vipdMMU_TOTAL_VIDEO_MEMORY >> 10) + size - 1) / size) * size;

    if (heap_info->vip_memsize < size) {
        return VIP_ERROR_OUT_OF_MEMORY;
    }

    vipdrv_os_copy_memory(split_info, heap_info, sizeof(vipdrv_videomem_heap_info_t));
#if vpmdENABLE_40BITVA
    if (heap_info->vip_memsize <= size + vipdMMU_PAGE_16M_SIZE) {
#else
    if (heap_info->vip_memsize <= size + vipdMMU_PAGE_1M_SIZE) {
#endif
        heap_info->vip_memsize = 0;
    }
    else {
        split_info->vip_memsize = size;
        heap_info->vip_physical += size;
        heap_info->vip_memsize = heap_info->vip_memsize - size;
    }

    return VIP_SUCCESS;
}
#endif

vip_status_e vipdrv_mmu_page_switch(
    vipdrv_hardware_t* hardware,
    vip_uint64_t mmu_id
    )
{
#if vpmdMMU_PER_PROCESS || vpmdMMU_PER_VIP_TASK
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t physical = hardware->MMU_flush.virtual;
    vip_uint32_t* logical = (vip_uint32_t*)(hardware->MMU_flush.logical + FLUSH_MMU_STATES_SIZE);
    vip_uint32_t idleState = 0;
    vip_uint32_t try_count = 0;
    vip_uint64_t ad_mmu_id = adjust_mmu_id(mmu_id);
    vip_uint64_t ad_cur_mmu_id = adjust_mmu_id(hardware->cur_mmu_id);

    if (ad_mmu_id == ad_cur_mmu_id) {
        return VIP_SUCCESS;
    }
    PRINTK_D("switch page table mmu_id from 0x%"PRIx64" to 0x%"PRIx64"\n", ad_mmu_id, ad_cur_mmu_id);

    hardware->cur_mmu_id = mmu_id;
    hardware->flush_mmu = vip_false_e;

    /* mmu flush based on current mmu page */
    logical[0] = 0x10000000; /* end */
    logical[1] = 0x0;

#if vpmdENABLE_VIDEO_MEMORY_CACHE
    vipdrv_mem_flush_cache(hardware->device->context->mmu_flush_mem.mem_id, VIPDRV_CACHE_FLUSH);
#endif

    vipdrv_hw_trigger(hardware, (vip_uint32_t)physical);

    /* wait mmu flush cmd finish */
    vipdrv_read_register(hardware, 0x00004, &idleState);
    while (((idleState & 0x01) != 0x01) && (try_count < 100)) {
        try_count++;
#if (defined(_WIN32) || defined (LINUXEMULATOR)) || vpmdFPGA_BUILD
        vipdrv_os_delay(10);
#else
        vipdrv_os_udelay(100);
#endif
        vipdrv_read_register(hardware, 0x00004, &idleState);
    }

    if (try_count >= 100) {
        PRINTK_E("fail to flush MMU, idleState=0x%x\n", idleState);
        vipGoOnError(VIP_ERROR_TIMEOUT);
    }

    /* switch mmu page */
    vipOnError(vipdrv_hw_pdmode_setup_mmu(hardware));
onError:
    return status;
#else
    return VIP_SUCCESS;
#endif
}

vip_uint64_t gen_mmu_id(
    vip_uint32_t pid,
    vip_uint32_t task_id
    )
{
    return ((vip_uint64_t)pid << 32) | task_id;
}

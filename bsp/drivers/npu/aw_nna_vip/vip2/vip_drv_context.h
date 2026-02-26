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

#ifndef __VIP_DRV_CONTEXT_H__
#define __VIP_DRV_CONTEXT_H__

#include <vip_drv_type.h>
#include <vip_drv_hardware.h>
#include <vip_drv_device.h>

#define VIPDRV_CHECK_NULL(object)                                            \
if (VIP_NULL == object) {                                                    \
    PRINTK_E("%s[%d] %s object is NULL\n", __FUNCTION__, __LINE__, #object); \
    return VIP_ERROR_INVALID_ARGUMENTS;                                      \
}

#define VIPDRV_CHECK_CORE(core)                                                                  \
{                                                                                                \
    vip_uint32_t *c_cnt = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_HARDWARE_CNT);   \
    if ((core) > *cnt) {                                                                         \
        PRINTK_E("failed core=%d is larger than core count=%d\n", core, *cnt);                   \
        VIPDRV_DUMP_STACK();                                                                     \
        vipGoOnError(VIP_ERROR_FAILURE);                                                         \
    }                                                                                            \
}

#define VIPDRV_CHECK_DEV(dev)                                                                    \
{                                                                                                \
    vip_uint32_t *d_cnt = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_DEVICE_CNT);     \
    if ((dev) >= *cnt) {                                                                         \
        PRINTK_E("failed device id =%d is larger than device count=%d\n",                        \
                  dev, *cnt);                                                                    \
        VIPDRV_DUMP_STACK();                                                                     \
        vipGoOnError(VIP_ERROR_FAILURE);                                                         \
    }                                                                                            \
}

#if (vpmdENABLE_DEBUG_LOG >= 4)
#define VIPDRV_SHOW_DEV_CORE_INFO(dev_idx, core_count, core_idx, string)   \
{                                                                          \
    vip_char_t info_t[256];                                                \
    vip_char_t *tmp = info_t;                                              \
    vip_uint32_t itr = 0;                                                  \
    vipdrv_os_snprint(tmp, 255, "%s, dev_idx=%d, core_count=%d, core_id[", \
                       string, dev_idx, core_count);                       \
    tmp += 34 + sizeof(string);                                            \
    for (itr = 0; itr < core_count; itr++) {                               \
        vipdrv_os_snprint(tmp, 255, "%02d ", (core_idx + itr));            \
        tmp += 3;                                                          \
    }                                                                      \
    vipdrv_os_snprint(tmp, 255, "]\n");                                    \
    PRINTK("%s", info_t);                                                  \
}
#else
#define VIPDRV_SHOW_DEV_CORE_INFO(dev_idx, core_count, core_idx, string)
#endif

/*
@brief define loop fun for each vip device
*/
#define VIPDRV_LOOP_DEVICE_START                                                                \
{  vip_uint32_t dev_index = 0;                                                                  \
   vipdrv_context_t *kcontext = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL); \
   for (dev_index = 0 ; dev_index < kcontext->device_count; dev_index++) {                      \
        vipdrv_device_t *device = &kcontext->device[dev_index];                                 \

#define VIPDRV_LOOP_DEVICE_END }}

/*
@brief define loop fun for each vip core
*/
#define VIPDRV_LOOP_HARDWARE_START                                                              \
{  vip_uint32_t dev_index = 0, hw_index = 0;                                                    \
   vipdrv_context_t *kcontext = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL); \
   for (dev_index = 0 ; dev_index < kcontext->device_count; dev_index++) {                      \
        vipdrv_device_t *dev = &kcontext->device[dev_index];                                    \
        for (hw_index = 0 ; hw_index < dev->hardware_count; hw_index++) {                       \
            vipdrv_hardware_t *hw = &dev->hardware[hw_index];                                   \
            if (hw != VIP_NULL) {                                                               \

#define VIPDRV_LOOP_HARDWARE_END }}}}

typedef enum _vipdrv_context_property_e
{
    VIPDRV_CONTEXT_PROP_ALL            = 0,
    VIPDRV_CONTEXT_PROP_CID            = 1,
    VIPDRV_CONTEXT_PROP_TASK_DESC      = 2,
    VIPDRV_CONTEXT_PROP_TCB            = 3,
    VIPDRV_CONTEXT_PROP_INITED         = 4,
    VIPDRV_CONTEXT_PROP_DEVICE_CNT     = 5,
    VIPDRV_CONTEXT_PROP_HARDWARE_CNT   = 6,
    VIPDRV_CONTEXT_PROP_MAX
} vipdrv_context_property_e;

/* vip-drv global context. */
struct _vipdrv_context
{
    volatile vip_int32_t  initialize;
    vipdrv_hashmap_t      process_id;
    /* the max number of vip core */
    vip_uint32_t          max_core_count;
    /* the real number of vip core */
    vip_uint32_t          core_count;
    /* the logical vip device number */
    vip_uint32_t          device_count;
    vipdrv_device_t       *device;

    vip_uint32_t    chip_ver1;
    vip_uint32_t    chip_ver2;
    vip_uint32_t    chip_cid;
    vip_uint32_t    chip_date;
    vip_hw_feature_db_t *feature_db;

#if vpmdENABLE_SYS_MEMORY_HEAP
    /* the size of system heap memory */
    vip_uint32_t    sys_heap_size;
#endif
#if vpmdENABLE_RESERVE_PHYSICAL
    vip_physical_t   reserve_phy_base;
    vip_uint32_t    reserve_phy_size;
    vip_virtual_t   reserve_virtual;
#endif
#if vpmdENABLE_MMU
    vipdrv_video_memory_t MMU_entry;
    vipdrv_video_memory_t default_mem;
    vipdrv_video_memory_t mmu_flush_mem;
    vip_virtual_t         mmu_flush_virtual;
#if vpmdMMU_SINGLE_PAGE
    vipdrv_mmu_info_t     *mmu_info;
#else
    vipdrv_hashmap_t      mmu_hashmap;
#endif
#endif
    /* the size of vip-sram for per-core */
    vip_uint32_t    vip_sram_size[vipdMAX_CORE];
    /* the remap address or VIP's virtual address of VIP SRAM */
    vip_virtual_t   vip_sram_address;
    /* the size of axi sram for per-device */
    vip_uint32_t    axi_sram_size[vipdMAX_CORE];
    /* MMU enabled: the VIP's virtual address of AXI-SRAM
       MMU disabled: the VIP's physical address of AXI-SRAM */
    vip_virtual_t   axi_sram_address;
    /* the NPU physical address of AXI SRAM */
    vip_physical_t  axi_sram_physical;

#if vpmdENABLE_MULTIPLE_TASK
    /* mutex for initialize vip-drv */
    vipdrv_mutex          initialize_mutex;
#endif
    vipdrv_database_t     mem_database;

    vipdrv_feature_config_t   func_config;
#if vpmdENABLE_CAPTURE || (vpmdENABLE_HANG_DUMP > 1)
    vipdrv_video_memory_t     register_dump_buffer;
    vip_uint32_t              register_dump_count;
#endif
    vipdrv_database_t     tskDesc_database;
#if vpmdTASK_QUEUE_USED
    vipdrv_hashmap_t      tskTCB_hashmap;
#endif
#if vpmdONE_POWER_DOMAIN
    vipdrv_mutex          power_mutex;
    volatile vip_uint32_t power_hardware_cnt;
#endif
    /* video memory allocator instance list */
    vipdrv_allocator_t    *alloc_list_head;
};

/*
@brief Get vip-drv context object.
*/
void* vipdrv_get_context(
    vipdrv_context_property_e prop
    );

vip_status_e vipdrv_context_init(
    vipdrv_context_t *context
    );

vip_status_e vipdrv_context_uninit(
    vipdrv_context_t *context
    );

vip_status_e vipdrv_context_select_featureDB(
    vipdrv_context_t* context
    );

vip_bool_e vipdrv_context_support(
    vip_hardware_feature_e feature
    );
#endif

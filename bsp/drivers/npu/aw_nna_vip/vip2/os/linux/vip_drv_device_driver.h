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

#ifndef __VIP_DRV_DEVICE_DRIVER_H__
#define __VIP_DRV_DEVICE_DRIVER_H__

#include <linux/pagemap.h>
#include <linux/seq_file.h>
#include <linux/mman.h>
#include <asm/atomic.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include <linux/irqflags.h>
#include <linux/vmalloc.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/interrupt.h>
#include <linux/vmalloc.h>
#include <linux/dma-mapping.h>
#include <linux/kthread.h>
#include <linux/pagemap.h>
#include <linux/mman.h>
#ifdef CONFIG_COMPAT
#include <asm/compat.h>
#endif
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/dma-mapping.h>
#include <linux/math64.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0))
#include <linux/stdarg.h>
#else
#include <stdarg.h>
#endif
#include <linux/time.h>
#include <linux/mm.h>
#include <linux/debugfs.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
#include <linux/cma.h>
#endif
#if defined (USE_LINUX_PCIE_DEVICE)
#include <linux/pci.h>
#endif
#include <vip_lite_config.h>
#if vpmdENABLE_DEBUGFS
#include <vip_drv_os_debug.h>
#endif
#include <vip_drv_hardware.h>
#include <vip_drv_os_port.h>


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,8,0)
#define current_mm_mmap_sem current->mm->mmap_lock
#else
#define current_mm_mmap_sem current->mm->mmap_sem
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION (3,7,0)
#define vipdVM_FLAGS (VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP)
#else
#define vipdVM_FLAGS (VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_RESERVED)
#endif

typedef uintptr_t  vipdrv_uintptr_t;

#define GetPageCount(size)     (((size) + PAGE_SIZE - 1) >> PAGE_SHIFT)

/*
  video memory heap base address alignment size
  4K bytes. request for Linux system page mapping.
*/
#define gcdVIP_MEMORY_ALIGN_BASE_ADDRESS  4096

/* the maximum number of heap can be managed by this driver */
#define VIDEO_MEMORY_HEAP_MAX_CNT      4

/* exclusive allocator
 heap-exclusive memory should be:
    1. Device memory on PCIE card.
    2. Reserved in DTS, and reserved by Linux System.
 This memory has not manage by Linux system.
*/
#define ALLOCATOR_NAME_HEAP_EXCLUSIVE     "heap-exclusive"

/* reserved allocator
 reserved memory should be host memory, not used by system and reserved for NPU
 This memory has been manage by Linux system.
*/
#define ALLOCATOR_NAME_HEAP_RESERVED      "heap-reserved"


typedef struct _vipdrv_driver_t {
    struct device *device;
    volatile void *vip_reg[vipdMAX_CORE];     /* Mapped cpu virtual address for AHB register memory base */
    vip_uint8_t    reg_drv_map[vipdMAX_CORE];
    vip_physical_t vip_reg_phy[vipdMAX_CORE];  /* the AHB register CPU physical base address */
    vip_uint32_t   vip_reg_size[vipdMAX_CORE]; /* the size of AHB register memory */
    vip_uint32_t   vip_bars[vipdMAX_CORE];
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    /* vido memory heap */
    vip_uint32_t       vip_mem_cnt;
    /* CPU physical adddress for video memory heap */
    vip_physical_t      cpu_physical[VIDEO_MEMORY_HEAP_MAX_CNT + 1];
    struct page        *pages[VIDEO_MEMORY_HEAP_MAX_CNT + 1];
    vipdrv_videomem_heap_info_t heap_infos[VIDEO_MEMORY_HEAP_MAX_CNT + 1];
#endif
    vip_uint32_t        sys_heap_size; /* the size of system heap size */

    vip_uint32_t        axi_sram_size[vipdMAX_CORE]; /* the size of AXI SRAM */
    vip_physical_t      axi_sram_base; /* the base address of AXI SRAM */
    vip_uint32_t        vip_sram_base; /* the base address of VIP SRAM */

    vip_int32_t           irq_registed[vipdMAX_CORE];
    vip_uint8_t           irq_map[vipdMAX_CORE]; /* map core_id to irq_queue index */
    wait_queue_head_t     irq_queue[vipdMAX_CORE];
    vipdrv_atomic         irq_count[vipdMAX_CORE];
    vip_int32_t           registered;
    vip_int32_t           major;
    struct class          *class;
    vip_int32_t           created;
    volatile vip_uint32_t irq_value[vipdMAX_CORE];
#if defined (USE_LINUX_PCIE_DEVICE)
    vip_uint32_t          pci_bars[vipdMAX_CORE];
    vip_uint32_t          reg_offset[vipdMAX_CORE];
    vip_uint64_t          pci_bar_size[vipdMAX_CORE];
    vip_physical_t         pci_bar_base[vipdMAX_CORE];
    volatile void         *pci_vip_reg[vipdMAX_CORE];
    vip_uint64_t          dma_mask;
    struct pci_dev        *pdev;
    struct pci_driver     *pdrv;
#elif defined (USE_LINUX_PLATFORM_DEVICE)
    struct platform_device *pdev;
    struct platform_driver *pdrv;
#endif

    vip_uint32_t     core_fscale_percent;

    vip_uint32_t           irq_line[vipdMAX_CORE]; /* IRQ Line number */

    volatile vip_uint32_t  power_status[vipdMAX_CORE];
    vip_uint32_t           max_core_count;
    /* real core count */
    vip_uint32_t           core_count;
    /* real device count */
    vip_uint32_t           device_count;
    /* the core numbers for each logical device */
    vip_uint32_t           device_core_number[vipdMAX_CORE];
#if vpmdENABLE_RESERVE_PHYSICAL
    vip_physical_t          reserve_physical_base;
    vip_uint32_t           reserve_physical_size;
#endif
    /* dyn set log level for debug */
    vip_uint32_t     log_level;
    /* dyn set enable capture is enable or disable */
    vipdrv_feature_config_t func_config;
    vip_uint32_t     open_device_count;

    vip_uint32_t            device_cnt;
} vipdrv_driver_t;


typedef struct _vipdrv_command_data {
    union _u {
        vipdrv_initialize_t init;
        vipdrv_allocation_t allocate;
        vipdrv_wait_task_param_t wait;
        vipdrv_cancel_task_param_t cancel;
        vipdrv_submit_task_param_t task;
        vipdrv_operate_cache_t flush;
        vipdrv_reg_t reg;
        vipdrv_query_address_info_t info;
        vipdrv_power_management_t power_manage;
        vipdrv_wrap_user_memory_t wrap_memory;
        vipdrv_query_database_t database;
        vipdrv_deallocation_t unalloc;
        vipdrv_query_profiling_t profiling;
        vipdrv_query_register_dump_t register_dump;
        vipdrv_create_task_param_t create_task;
        vipdrv_set_task_property_t set_prop;
        vipdrv_destroy_task_param_t destroy_task;
    } u;
} vipdrv_command_data;

/*
@brief get kdriver object.
*/
vipdrv_driver_t* vipdrv_get_kdriver(void);

/*
@brief do some initialize in this function.
@param, vip_memsizem, the size of video memory heap.
*/
vip_status_e vipdrv_drv_init(
    vip_uint32_t video_memsize
    );

/*
@brief do some un-initialize in this function.
*/
vip_status_e vipdrv_drv_exit(void);

/*
 Get the video memory config information
*/
vip_status_e vipdrv_drv_get_memory_config(
    vipdrv_platform_memory_config_t *memory_info
    );

/*
 Get the video memory config information
 1. the size of system memory used by vip_hal.
 2. reserve_physical is a video memory, used by vip_create_network_from_physical() api in nbg_linker.
 3. heap info is used for video memory heap basic information.
*/
vip_status_e vipdrv_drv_get_hardware_info(
    vipdrv_hardware_info_t *info
    );

/*
@brief Set power on/off and clock on/off
@param state, power status. refer to vipdrv_power_status_e.
*/
vip_status_e vipdrv_drv_set_power_clk(
    vip_uint32_t core,
    vip_uint32_t state
    );

/***********Platform Functions Start***************************/
/*
@brief convert CPU physical to VIP physical.
@param cpu_physical. the physical address of CPU domain.
*/
vip_physical_t vipdrv_drv_get_vipphysical(
    vip_physical_t cpu_physical
    );

/*
@brief convert VIP physical to CPU physical.
@param vip_physical. the physical address of VIP domain.
*/
vip_physical_t vipdrv_drv_get_cpuphysical(
    vip_physical_t vip_physical
    );

/*
@brief 1. platfrom(vendor) initialize. prepare environmnet for running VIP hardware.
       2. initialzie linux platform device.
       3. this function should be called before probe device/driver.
          so many variables not initialize in kdriver. such as kdriver->pdev.
@param pdrv vip drvier device object.
*/
vip_int32_t vipdrv_drv_platform_init(
    vipdrv_driver_t *kdriver
    );

/*
@brief 1. platfrom(vendor) un-initialize.
       2. uninitialzie linux platform device.
@param pdrv vip drvier device object.
*/
vip_int32_t vipdrv_drv_platform_uninit(
    vipdrv_driver_t *kdriver
    );

/*
@brief adjust parameters of vip devices. such as irq, SRAM, video memory heap.
        You can overwrite insmod command line in vipdrv_drv_adjust_param() function.
        Verisilicon don't overwrite insmod command line parameters.
        this function called after probe device/driver, so kdriver->pdev can be used.
@param kdriver, vip device object.
*/
vip_int32_t vipdrv_drv_adjust_param(
    vipdrv_driver_t *kdriver
    );

/*
@brief release resources created in vipdrv_drv_adjust_param if neeed.
        this function called before remove device/driver, so kdriver->pdev can be used.
@param kdriver, vip device object.
*/
vip_int32_t vipdrv_drv_unadjust_param(
    vipdrv_driver_t *kdriver
    );

/*
@brief custome device initialize. power on and rise up clock.
@param kdriver, vip device object.
*/
vip_int32_t vipdrv_drv_device_uninit(
    vipdrv_driver_t *kdriver,
    vip_uint32_t core
    );

/*
@brief custome device un-initialize. power off.
@param kdriver, vip device object.
*/
vip_int32_t vipdrv_drv_device_init(
    vipdrv_driver_t *kdriver,
    vip_uint32_t core
    );
/***********Platform Functions End***************************/

vip_int32_t vipdrv_drv_get_supported_frequency(struct device *dev, u64 *max_freq, u32 *npu_vol, u64 arr[], u32 size);
vip_int32_t vipdrv_drv_update_clk_freq(const u64 freq);
vip_int32_t vipdrv_drv_update_vol_regul(const int vol);
vip_uint64_t vipdrv_drv_get_clk_freq(void);
vip_uint32_t vipdrv_drv_get_vol_regul(void);

#if vpmdENABLE_AW_DEVFREQ
void vipdrv_set_internal_freq(void);
#endif

#endif /* __VIP_DRV_DEVICE_DRIVER_H__ */

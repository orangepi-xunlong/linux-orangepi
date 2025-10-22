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

#ifndef __GC_VIP_KERNEL_DRV_H__
#define __GC_VIP_KERNEL_DRV_H__

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
#include <asm/atomic.h>
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
#include <gc_vip_common.h>
#if vpmdENABLE_DEBUGFS
#include <gc_vip_kernel_os_debug.h>
#endif
#include <gc_vip_hardware.h>
#include <gc_vip_kernel_port.h>

typedef uintptr_t  gckvip_uintptr_t;

#define GetPageCount(size)     (((size) + PAGE_SIZE - 1) >> PAGE_SHIFT)

/*
  video memory heap base address alignment size
  4K bytes. request for Linux system page mapping.
*/
#define gcdVIP_MEMORY_ALIGN_BASE_ADDRESS  4096

typedef enum _gckvip_reserved_mem_type
{
    /* the memory is reserved at boot time, and not mapping in system. memory reserved in DTS? */
    GCKVIP_RESERVED_MEM_BOOT_TIME     = 0,
    /* the memory is reserved at run time, and mapped in system. */
    GCKVIP_RESERVED_MEM_RUN_TIME      = 1,
    GCKVIP_RESERVED_MEM_MAX,
} gckvip_reserved_mem_type_e;

typedef struct _gckvip_driver_t {
    struct device *device;
    volatile void *vip_reg[gcdMAX_CORE];     /* Mapped cpu virtual address for AHB register memory base */
    vip_uint8_t    reg_drv_map[gcdMAX_CORE];
    vip_uint32_t vip_reg_phy[gcdMAX_CORE];  /* the AHB register physical base address */
    vip_uint32_t vip_reg_size[gcdMAX_CORE]; /* the size of AHB register memory */
    vip_uint32_t vip_bars[gcdMAX_CORE];

    /* vido memory heap */
    void         *cpu_virtual; /* Mapped cpu virtual address for video memory heap in user space */
    void         *cpu_virtual_kernel;/* Mapped cpu virtual address for video memory heap in kernel space */
    phy_address_t cpu_physical; /* CPU physical adddress for video memory heap */
    phy_address_t vip_physical; /* VIP physical address for video memory heap */
    vip_uint32_t vip_memsize;   /* the size of reserved memory */
    vip_uint8_t  reserved_mem_type; /* see gckvip_reserved_mem_type_e */

    vip_uint32_t sys_heap_size; /* the size of system heap size */

    vip_uint32_t axi_sram_size[gcdMAX_CORE]; /* the size of AXI SRAM */
    vip_uint32_t axi_sram_base; /* the base address of AXI SRAM */
    vip_uint32_t vip_sram_size[gcdMAX_CORE]; /* the size of VIP SRAM */
    vip_uint32_t vip_sram_base; /* the base address of VIP SRAM */

    vip_int32_t           irq_registed[gcdMAX_CORE];
    wait_queue_head_t     irq_queue[gcdMAX_CORE];
    vip_int32_t           registered;
    vip_int32_t           major;
    struct class          *class;
    vip_int32_t           created;
    volatile vip_uint32_t irq_value[gcdMAX_CORE];
#if defined (USE_LINUX_PCIE_DEVICE)
    vip_uint32_t          pci_bars[gcdMAX_CORE];
    vip_uint32_t          reg_offset[gcdMAX_CORE];
    vip_uint32_t          pci_bar_size[gcdMAX_CORE];
    phy_address_t         pci_bar_base[gcdMAX_CORE];
    volatile void         *pci_vip_reg[gcdMAX_CORE];
    vip_uint64_t          dma_mask;
    struct pci_dev        *pdev;
    struct pci_driver     *pdrv;
#elif defined (USE_LINUX_PLATFORM_DEVICE)
    struct platform_device *pdev;
    struct platform_driver *pdrv;
#endif

    vip_uint32_t     core_fscale_percent;

#if vpmdENABLE_DEBUGFS
    struct dentry    *debugfs_parent;
    gckvip_profile_t profile_data;
#endif
    vip_uint32_t           irq_line[gcdMAX_CORE]; /* IRQ Line number */
    struct page            *pages;
    vip_bool_e             is_exact_page;

    volatile vip_uint32_t  power_status[gcdMAX_CORE];
    vip_uint32_t           max_core_count;
    vip_uint32_t           core_count;
    /* the core numbers for each logical device */
    vip_uint32_t           device_core_number[gcdMAX_CORE];
#if vpmdENABLE_RESERVE_PHYSICAL
    phy_address_t          reserve_physical_base;
    vip_uint32_t           reserve_physical_size;
#endif
    /* dyn set log level for debug */
    vip_uint32_t     log_level;
    /* dyn set enable capture is enable or disable */
    gckvip_feature_config_t func_config;
    vip_uint32_t     open_device_count;
} gckvip_driver_t;


typedef struct _gckvip_command_data {
    union _u {
        gckvip_initialize_t init;
        gckvip_allocation_t allocate;
        gckvip_wait_t wait;
        gckvip_cancel_t cancel;
        gckvip_commit_t commit;
        gckvip_operate_cache_t flush;
        gckvip_reg_t reg;
        gckvip_query_address_info_t info;
        gckvip_query_power_info_t power;
        gckvip_set_work_mode_t mode;
        gckvip_power_management_t power_manage;
        gckvip_wrap_user_memory_t wrap_memory;
        gckvip_unwrap_user_memory_t unwrap;
        gckvip_query_database_t database;
        gckvip_deallocation_t unalloc;
        gckvip_query_profiling_t profiling;
        gckvip_query_register_dump_t register_dump;
    } u;
} gckvip_command_data;

/*
@brief get kdriver object.
*/
gckvip_driver_t* gckvip_get_kdriver(void);

/*
@brief do some initialize in this function.
@param, vip_memsizem, the size of video memory heap.
*/
vip_status_e gckvip_drv_init(
    vip_uint32_t video_memsize
    );

/*
@brief do some un-initialize in this function.
*/
vip_status_e gckvip_drv_exit(void);

/*
@brief Get hardware basic information.
*/
vip_status_e gckvip_drv_get_hardware_info(
    gckvip_hardware_info_t *info
    );

/*
@brief Set power on/off and clock on/off
@param state, power status. refer to gckvip_power_status_e.
*/
vip_status_e gckvip_drv_set_power_clk(
    vip_uint32_t core,
    vip_uint32_t state
    );

/***********Platform Functions Start***************************/
/*
@brief convert CPU physical to VIP physical.
@param cpu_physical. the physical address of CPU domain.
*/
vip_address_t gckvip_drv_get_vipphysical(
    vip_address_t cpu_physical
    );

/*
@brief convert VIP physical to CPU physical.
@param vip_physical. the physical address of VIP domain.
*/
vip_address_t gckvip_drv_get_cpuphysical(
    vip_address_t vip_physical
    );

/*
@brief 1. platfrom(vendor) initialize. prepare environmnet for running VIP hardware.
       2. initialzie linux platform device.
       3. this function should be called before probe device/driver.
          so many variables not initialize in kdriver. such as kdriver->pdev.
@param pdrv vip drvier device object.
*/
vip_int32_t gckvip_drv_platform_init(
    gckvip_driver_t *kdriver
    );

/*
@brief 1. platfrom(vendor) un-initialize.
       2. uninitialzie linux platform device.
@param pdrv vip drvier device object.
*/
vip_int32_t gckvip_drv_platform_uninit(
    gckvip_driver_t *kdriver
    );

/*
@brief adjust parameters of vip devices. such as irq, SRAM, video memory heap.
        You can overwrite insmod command line in gckvip_drv_adjust_param() function.
        Verisilicon don't overwrite insmod command line parameters.
        this function called after probe device/driver, so kdriver->pdev can be used.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_adjust_param(
    gckvip_driver_t *kdriver
    );

/*
@brief release resources created in gckvip_drv_adjust_param if neeed.
        this function called before remove device/driver, so kdriver->pdev can be used.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_unadjust_param(
    gckvip_driver_t *kdriver
    );

/*
@brief custome device initialize. power on and rise up clock.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_device_uninit(
    gckvip_driver_t *kdriver,
    vip_uint32_t core
    );

/*
@brief custome device un-initialize. power off.
@param kdriver, vip device object.
*/
vip_int32_t gckvip_drv_device_init(
    gckvip_driver_t *kdriver,
    vip_uint32_t core
    );
/***********Platform Functions End***************************/

#endif /* __GC_VIP_KERNEL_DRV_H__ */

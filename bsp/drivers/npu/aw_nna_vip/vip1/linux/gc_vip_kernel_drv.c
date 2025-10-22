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
#include <gc_vip_kernel_pm.h>
#include <gc_vip_kernel_debug.h>
#include <gc_vip_kernel_drv.h>
#include <gc_vip_kernel_os.h>
#include <gc_vip_version.h>

#ifndef DEVICE_NAME
#define DEVICE_NAME              "vipcore"
#endif

#ifndef CLASS_NAME
#define CLASS_NAME               "vipcore_class"
#endif

/*
  Linux insmod parameters.
  These parameter need to be configured according to your chip and system enviroment
*/

static vip_uint32_t major = 199;
module_param(major, uint, 0644);
MODULE_PARM_DESC(major, "major device number for VIP device");

static vip_uint32_t pciBars[gcdMAX_CORE] = {[0 ... gcdMAX_CORE -1] = 0};
module_param_array(pciBars, uint, NULL, 0644);
MODULE_PARM_DESC(pciBars, " the index of each pcie bars for vip multi-core");

static vip_uint32_t registerMemBase[gcdMAX_CORE] = {[0 ... gcdMAX_CORE -1] = 0};
module_param_array(registerMemBase, uint, NULL, 0644);
MODULE_PARM_DESC(registerMemBase, "AHB register base physical address of vip multi-core");

static vip_uint32_t registerMemSize[gcdMAX_CORE] = {[0 ... gcdMAX_CORE -1] = 0};
module_param_array(registerMemSize, uint, NULL, 0644);
MODULE_PARM_DESC(registerMemSize, "AHB register memory size of vip multi-core");

static vip_uint32_t registerOffset[gcdMAX_CORE] = {[0 ... gcdMAX_CORE -1] = 0};
module_param_array(registerOffset, uint, NULL, 0644);
MODULE_PARM_DESC(registerOffset, "AHB register offsets in corresponding BAR space, only for PCIE device");

static vip_uint32_t irqLine[gcdMAX_CORE] = {[0 ... gcdMAX_CORE -1] = 0};
module_param_array(irqLine, uint, NULL, 0644);
MODULE_PARM_DESC(irqLine, "irq line number of vip multi-core");

static vip_uint32_t contiguousSize = 0;
module_param(contiguousSize, uint, 0644);
MODULE_PARM_DESC(contiguousSize, "defalut video memory heap size");

static ulong contiguousBase = 0;
module_param(contiguousBase, ulong, 0644);
MODULE_PARM_DESC(contiguousBase, "the base pyhsical address of video memory heap");

/*
   After reserved this contiguous phyiscal, driver fill mmu page table in vip_init stages,
   when application call create_buffer_form_physical don't fill mmu page table again.
   so speed up API calls. This feature is avaliable when set vpmdENABLE_RESERVE_PHYSICAL to 1.
*/
static vip_uint32_t reservePhysicalSize = 0;
module_param(reservePhysicalSize, uint, 0644);
MODULE_PARM_DESC(reservePhysicalSize, "the size of contiguous physical memory, "
                    "it is only used for <create_buffer_form_physical> API");

static ulong reservePhysicalBase = 0;
module_param(reservePhysicalBase, ulong, 0644);
MODULE_PARM_DESC(reservePhysicalBase, "the base address of contiguous physical memory, "
                    "it is only used for <create_buffer_form_physical> API");

static vip_int32_t verbose = 1;
module_param(verbose, int, S_IRUGO);
MODULE_PARM_DESC(verbose, "ebable/disable log print");

static vip_int32_t drvType = 0;
module_param(drvType, int, 0644);
MODULE_PARM_DESC(drvType, "0 - Char Driver (Default), 1 - Misc Driver");

static vip_uint32_t AXISramSize[gcdMAX_CORE] = {[0 ... gcdMAX_CORE -1] = 0};
module_param_array(AXISramSize, uint, NULL, 0644);
MODULE_PARM_DESC(AXISramSize, "the size of AXI SRAM");

static vip_uint32_t AXISramBaseAddress = 0;
module_param(AXISramBaseAddress, uint, 0644);
MODULE_PARM_DESC(AXISramBaseAddress, "the physical base address of AXI SRAM");

static vip_uint32_t VIPSramSize[gcdMAX_CORE] = {[0 ... gcdMAX_CORE -1] = 0};
module_param_array(VIPSramSize, uint, NULL, 0644);
MODULE_PARM_DESC(VIPSramSize, "the size of VIP SRAM");

static vip_uint32_t VIPSramBaseAddress = 0;
module_param(VIPSramBaseAddress, uint, 0644);
MODULE_PARM_DESC(VIPSramBaseAddress, "the base address of VIP SRAM");


#define MIN_VIDEO_MEMORY_SIZE       (4 * 1024)
#define USE_MEM_WRITE_COMOBINE       1

#if vpmdENABLE_VIDEO_MEMORY_CACHE
#undef USE_MEM_WRITE_COMOBINE
#define USE_MEM_WRITE_COMOBINE        0
#endif

#define gcdVIP_IRQ_SHARED   1

#if LINUX_VERSION_CODE >= KERNEL_VERSION (4, 1, 0)
#if gcdVIP_IRQ_SHARED
#define gcdVIP_IRQ_FLAG   (IRQF_SHARED | IRQF_TRIGGER_HIGH)
#else
#define gcdVIP_IRQ_FLAG   (IRQF_TRIGGER_HIGH)
#endif
#else
#if gcdVIP_IRQ_SHARED
#define gcdVIP_IRQ_FLAG   (IRQF_DISABLED | IRQF_SHARED | IRQF_TRIGGER_HIGH)
#else
#define gcdVIP_IRQ_FLAG   (IRQF_DISABLED | IRQF_TRIGGER_HIGH)
#endif
#endif

/* default disable internel clock only used for the SOC that power is always online */
#define DEFAULT_DISABLE_INTERNEL_CLOCK          0

#define DEFAULT_ENABLE_LOG                  1
#define DEFAULT_ENABLE_CAPTURE              1
#define DEFAULT_ENABLE_CNN_PROFILE          1
#define DEFAULT_ENABLE_DUMP_NBG             1

/*
@brief define loop fun for each vip core
*/
#define LOOP_CORE_START                                  \
{  vip_uint32_t core = 0;                                \
   gckvip_driver_t *kdriver = gckvip_get_kdriver();      \
   for (core = 0 ; core < kdriver->core_count; core++) {

#define LOOP_CORE_END }}


static DEFINE_MUTEX(set_power_mutex);
static DEFINE_MUTEX(open_mutex);

/* Global variable for vip device */
gckvip_driver_t *kdriver = VIP_NULL;

gckvip_driver_t* gckvip_get_kdriver(void)
{
    return kdriver;
}

#if vpmdENABLE_VIDEO_MEMORY_HEAP
static vip_int32_t map_kernel_logical_page(
    phy_address_t physical,
    unsigned long bytes,
    void **logical
    )
{
    int ret = 0;
    unsigned long pfn = __phys_to_pfn(physical);
    vip_uint32_t offset = physical - (physical & PAGE_MASK);
    struct page **pages;
    struct page *page;
    size_t num_pages = 0;
    size_t i = 0;
    pgprot_t pgprot;
    void *ptr = VIP_NULL;
    vip_uint8_t *prt_tmp = VIP_NULL;

    num_pages = GetPageCount(PAGE_ALIGN(bytes));
    gckvip_os_allocate_memory(sizeof(struct page *) * num_pages, (void**)&pages);
    if (!pages) {
        PRINTK("vipcore, fail to malloc for pages\n");
        return -1;
    }

    if (!pfn_valid(pfn)) {
        PRINTK("vipcore, unable to convert %pa to pfn\n", &physical);
        ret = -1;
        goto onError;
    }

    page = pfn_to_page(pfn);

    for (i = 0; i < num_pages; i++) {
        pages[i] = nth_page(page, i);
    }

#if USE_MEM_WRITE_COMOBINE
    pgprot = pgprot_writecombine(PAGE_KERNEL);
    PRINTK_D("map kernel logical writecombine..\n");
#elif vpmdENABLE_VIDEO_MEMORY_CACHE
    pgprot = PAGE_KERNEL;
    PRINTK_D("map kernel logical with cache..\n");
#else
    pgprot = pgprot_noncached(PAGE_KERNEL);
    PRINTK_D("map kernel logical none-cache..\n");
#endif

    ptr = vmap(pages, num_pages, 0, pgprot);
    if (ptr == NULL) {
        PRINTK("vipcore, fail to vmap physical to kernel space\n");
        ret = -1;
        goto onError;
    }

    prt_tmp = (vip_uint8_t*)ptr;
    *logical = (void*)(prt_tmp + offset);
    PRINTK_I("map logical kernel vmap..kernel logical address=0x%"PRPx",\n", *logical);

    gckvip_os_free_memory(pages);
    return ret;

onError:
    PRINTK_E("fail to map kernel logical page\n");
    if (pages != NULL) {
        gckvip_os_free_memory(pages);
    }
    return ret;
}

static vip_int32_t map_kernel_logical(
    gckvip_driver_t *kdriver,
    phy_address_t physical,
    unsigned long bytes,
    void **logical
    )
{
    int ret = 0;
    PRINTK_D("physical=0x%"PRIx64", size=0x%x\n", physical, bytes);
#if defined (USE_LINUX_RESERVE_MEM)
    if (GCKVIP_RESERVED_MEM_BOOT_TIME == kdriver->reserved_mem_type) {
        void *vaddr = VIP_NULL;
        #if USE_MEM_WRITE_COMOBINE
        vaddr =  ioremap_wc(physical, bytes);
        #elif vpmdENABLE_VIDEO_MEMORY_CACHE
        vaddr = ioremap_cache(physical, bytes);
        PRINTK_D("map kernel logical with cache...\n");
        #else
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
        vaddr = ioremap(physical, bytes);
        PRINTK_D("map kernel logical ioremap..\n");
        #else
        vaddr = ioremap_nocache(physical, bytes);
        PRINTK_D("map kernel logical none-cache physical=0x%llx, szie=0x%x..\n", physical, bytes);
        #endif
        #endif
        if (!vaddr) {
            PRINTK("vipcore, fail to map physical to kernel space\n");
            ret = -1;
        }
        PRINTK_I("map logical kernel ioremap..kernel logical address=0x%"PRPx"\n", vaddr);
        *logical = vaddr;
    }
    else {
        ret = map_kernel_logical_page(physical, bytes, logical);
    }
#else
    ret = map_kernel_logical_page(physical, bytes, logical);
    {
        dma_addr_t *dma_addr = VIP_NULL;
        vip_uint32_t num_pages = GetPageCount(PAGE_ALIGN(bytes));
        gckvip_os_allocate_memory(sizeof(dma_addr_t), (void**)&dma_addr);
        if (VIP_NULL == dma_addr) {
            PRINTK_E("fail to flush cache for reserved memory page\n");
            ret = -1;
            goto error;
        }
        *dma_addr = dma_map_page(kdriver->device, kdriver->pages, 0,
                                 num_pages * PAGE_SIZE, DMA_BIDIRECTIONAL);
        if (dma_mapping_error(kdriver->device, *dma_addr)) {
            PRINTK_E("dma map page failed.\n");
            return -1;
        }
        dma_sync_single_for_device(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_TO_DEVICE);
        dma_sync_single_for_cpu(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
        dma_unmap_page(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
        gckvip_os_free_memory((void*)dma_addr);
    }
error:
#endif
    return ret;
}

static vip_status_e unmap_kernel_logical(
    gckvip_driver_t *kdriver
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (kdriver != VIP_NULL) {
        if (kdriver->cpu_virtual_kernel != VIP_NULL) {
        #if defined(USE_LINUX_RESERVE_MEM)
            if (GCKVIP_RESERVED_MEM_BOOT_TIME == kdriver->reserved_mem_type) {
                iounmap((void *)kdriver->cpu_virtual_kernel);
            }
            else {
                vunmap((void *)kdriver->cpu_virtual_kernel);
            }
        #else
            vunmap((void *)kdriver->cpu_virtual_kernel);
        #endif
            kdriver->cpu_virtual_kernel = VIP_NULL;
        }
    }
    else {
        PRINTK_E("vipcore, vip device is NULL, in unmap kernel logical\n");
    }

    PRINTK_I("vipcore, unmap logical kernel address done\n");

    return status;
}
#endif

static vip_int32_t drv_open(struct inode * inode, struct file * file)
{
    vip_int32_t ret = 0;

    mutex_lock(&open_mutex);
    kdriver->open_device_count++;

    file->private_data = (void*)kdriver;
    PRINTK_D("drv open process pid=%x, tid=%x\n", gckvip_os_get_pid(), gckvip_os_get_tid());

#if !vpmdENABLE_MULTIPLE_TASK
    if (kdriver->open_device_count > 1) {
        ret = -1;
        kdriver->open_device_count--;
        PRINTK_E("fail to open device, mult-task feature is disbale, please set vpmdENABLE_MULTIPLE_TASK to 1\n");
    }
#endif
    mutex_unlock(&open_mutex);

    return ret;
}

static vip_int32_t drv_release(struct inode * inode, struct file * file)
{
    mutex_lock(&open_mutex);
    kdriver->open_device_count--;

    PRINTK_D("drv release process pid=%x, tid=%x\n", gckvip_os_get_pid(), gckvip_os_get_tid());

    gckvip_destroy();
    mutex_unlock(&open_mutex);

    return 0;
}

static long drv_ioctl(struct file *file, unsigned int ioctl_code, unsigned long arg)
{
    struct ioctl_data arguments;
    void *data = VIP_NULL;
    void *heap_data = VIP_NULL;
    gckvip_command_data stack_data;

    if (ioctl_code != GCKVIP_IOCTL) {
        PRINTK("vipcore, error ioctl code is %d\n", ioctl_code);
        return -1;
    }
    if ((void *)arg == VIP_NULL) {
        PRINTK("vipcore, error arg is NULL\n");
        return -1;
    }
    if (copy_from_user(&arguments, (void *)arg, sizeof(arguments)) != 0) {
        PRINTK("vipcore, fail to copy arguments\n");
        return -1;
    }
    if (arguments.bytes > 0) {
        if (arguments.bytes >= sizeof(gckvip_command_data)) {
            gckvip_os_allocate_memory(arguments.bytes, (void **)&heap_data);
            if (VIP_NULL == heap_data) {
                PRINTK("vipcore, fail to malloc memory for ioctl data");
                return -1;
            }
            data = heap_data;
        }
        else {
            data = &stack_data;
        }

        if (copy_from_user(data, (void*)(gckvip_uintptr_t)(arguments.buffer), arguments.bytes) != 0) {
            PRINTK("vipcore, %s fail to copy arguments buffer, size: %d\n", __func__, arguments.bytes);
            goto error;
        }
    }

    /* Execute kernel command. */
    arguments.error = gckvip_kernel_call(arguments.command, data);
    if (arguments.error != VIP_SUCCESS) {
        if ((arguments.error != VIP_ERROR_POWER_OFF) && (arguments.error != VIP_ERROR_CANCELED)) {
            PRINTK("vipocre, failed to kernel call, command[%d]: %s, status=%d\n",
                    arguments.command, ioctl_cmd_string[arguments.command], arguments.error);
        }
        if (copy_to_user((void *)arg, &arguments, sizeof(arguments)) != 0) {
            PRINTK("vipcore, fail to copy data to user arguments\n");
        }
        goto error;
    }

    if (arguments.bytes > 0) {
        if (copy_to_user((void*)(gckvip_uintptr_t)arguments.buffer, data, arguments.bytes) != 0) {
            PRINTK("vipcore, %s fail to copy data to user arguments buffer, size: %d\n", __func__,
                    arguments.bytes);
            goto error;
        }
    }

    if (copy_to_user((void *)arg, &arguments, sizeof(arguments)) != 0) {
        PRINTK("vipcore, fail to copy data to user arguments\n");
        goto error;
    }

    if (heap_data != VIP_NULL) {
        gckvip_os_free_memory(heap_data);
    }
    return 0;

error:
    if ((arguments.error != VIP_ERROR_POWER_OFF) && (arguments.error != VIP_ERROR_CANCELED)) {
        PRINTK("vipocre, failed to ioctl, command[%d]: %s\n",
               arguments.command, ioctl_cmd_string[arguments.command]);
    }
    if (heap_data != VIP_NULL) {
        gckvip_os_free_memory(heap_data);
    }
    return -1;
}

static ssize_t drv_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
    gckvip_driver_t *device = (gckvip_driver_t *)file->private_data;

    if (length != 4) {
        return 0;
    }

    /* return video memory heap size */
    if (copy_to_user((void __user *) buffer, (const void *) &device->vip_memsize,
         sizeof(device->vip_memsize)) != 0) {
        return 0;
    }

    return 4;
}

static int drv_mmap(struct file *file, struct vm_area_struct *vma)
{
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    unsigned long size;
    gckvip_driver_t *device = (gckvip_driver_t *)file->private_data;

    size = vma->vm_end - vma->vm_start;
    if (size > device->vip_memsize) {
        size = device->vip_memsize;
    }

    if (size < MIN_VIDEO_MEMORY_SIZE) {
        PRINTK_E("video memory heap size=%d is less than min size=0x%x\n",
                 size, MIN_VIDEO_MEMORY_SIZE);
        return -1;
    }

    /* Make this mapping non-cached. */
#if USE_MEM_WRITE_COMOBINE
    vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#elif !vpmdENABLE_VIDEO_MEMORY_CACHE
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    PRINTK_D("mmap none-cache....\n");
#endif
    vma->vm_flags |= (VM_IO | VM_DONTCOPY | VM_DONTEXPAND | VM_DONTDUMP);

    /* map kernel memory to user space.. */
    if (remap_pfn_range(vma, vma->vm_start, device->cpu_physical >> PAGE_SHIFT,
                        size, vma->vm_page_prot) < 0) {
        PRINTK_E("vipcore, remap_pfn_range failed\n");
        return -1;
    }
    PRINTK_D("vipcore, remap pfn range to user space\n");

    device->cpu_virtual = (void *)vma->vm_start;

    PRINTK_D("vipcore, user logical=0x%"PRPx", pyhsical address=0x%"PRIx64", size=0x%x, %s map\n",
            device->cpu_virtual, kdriver->cpu_physical, kdriver->vip_memsize,
            (vma->vm_flags & VM_PFNMAP) ? "PFN" : "PAGE");
#endif

    return 0;
}

static struct file_operations file_operations =
{
    .owner          = THIS_MODULE,
    .open           = drv_open,
    .release        = drv_release,
    .read           = drv_read,
    .unlocked_ioctl = drv_ioctl,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,8,18)
#ifdef HAVE_COMPAT_IOCTL
    .compat_ioctl   = drv_ioctl,
#endif
#else
    .compat_ioctl   = drv_ioctl,
#endif
    .mmap           = drv_mmap,
};

static struct miscdevice vipmisc_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = DEVICE_NAME,
    .fops  = &file_operations,
    .mode  = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
};

#if !vpmdENABLE_POLLING
/*
@brief Iinterrupte handle function.
*/
static irqreturn_t drv_irq_handler(
    int irq,
    void *data
    )
{
    uint32_t value = 0;
    vip_uint32_t core = GCKVIPPTR_TO_UINT32(data) - 1;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    if (VIP_NULL == kdriver->vip_reg[core]) {
        return IRQ_NONE;
    }
    /* Read interrupt status. */
    value = readl((uint8_t *)kdriver->vip_reg[core] + 0x00010);
    if (value != 0x00) {
        /* Combine with current interrupt flags. */
        kdriver->irq_value[core] = value;

        /* Wake up any waiters. */
        gckvip_os_wakeup_interrupt(&kdriver->irq_queue[core]);
        /* We handled the IRQ. */
        return IRQ_HANDLED;
    }

    /* Not our IRQ. */
    return IRQ_NONE;
}
#endif

/*  If VIP memory(video memory) has different address mapping view for CPU and VIP, need to
 *  add tranlation here. Default we assume VIP physical equals to CPU physical.
 */

#if (vpmdENABLE_DEBUG_LOG > 1)
static vip_int32_t  drv_show_params(
    gckvip_driver_t *kdriver
    )
{
    #define NAME_HEADER_SIZE  20
    vip_char_t info[256];
    vip_char_t *tmp = info;
    if (verbose) {
        PRINTK("=======vipcore parameter=====\n");
        gckvip_os_memcopy(tmp, "registerMemBase     ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        gckvip_os_snprint(tmp, 255, "0x%08X, ", kdriver->vip_reg_phy[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);

        tmp = info;
        gckvip_os_memcopy(tmp, "registerMemSize     ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        gckvip_os_snprint(tmp, 255, "0x%08X, ", kdriver->vip_reg_size[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);

        tmp = info;
        gckvip_os_memcopy(tmp, "irqLine             ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        gckvip_os_snprint(tmp, 255, "0x%08X, ", kdriver->irq_line[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);

        #if defined (USE_LINUX_PCIE_DEVICE)
        tmp = info;
        gckvip_os_memcopy(tmp, "pciBars             ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        gckvip_os_snprint(tmp, 255, "0x%08X, ", kdriver->pci_bars[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);
        tmp = info;
        gckvip_os_memcopy(tmp, "registerOffset      ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        gckvip_os_snprint(tmp, 255, "0x%08X, ", kdriver->reg_offset[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);
        #endif
        #if vpmdENABLE_VIDEO_MEMORY_HEAP
        PRINTK("contiguousSize      0x%08X\n", kdriver->vip_memsize);
        PRINTK("contiguousBase      0x%"PRIx64"\n", kdriver->cpu_physical);
        PRINTK("vipContiguousBase   0x%"PRIx64"\n", kdriver->vip_physical);
        #endif
        PRINTK("drvType             0x%08X\n", drvType);
        PRINTK("AXISramBaseAddress  0x%08X\n", kdriver->axi_sram_base);
        tmp = info;
        gckvip_os_memcopy(tmp, "AXISramSize         ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        gckvip_os_snprint(tmp, 255, "0x%08X, ", kdriver->axi_sram_size[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);
        PRINTK("VIPSramBaseAddress  0x%08X\n", kdriver->vip_sram_base);
        tmp = info;
        gckvip_os_memcopy(tmp, "VIPSramSize         ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        gckvip_os_snprint(tmp, 255, "0x%08X, ", kdriver->vip_sram_size[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);
        #if vpmdENABLE_SYS_MEMORY_HEAP
        PRINTK("sysHeapSize         0x%08X\n", kdriver->sys_heap_size);
        #endif
        #if vpmdENABLE_RESERVE_PHYSICAL
        PRINTK("reservePhysicalBase 0x%"PRIx64"\n", kdriver->reserve_physical_base);
        PRINTK("reservePhysicalSize 0x%08X\n", kdriver->reserve_physical_size);
        #endif
        PRINTK("===============================\n");
    }

    return 0;
}
#endif

static vip_int32_t drv_check_params(
    gckvip_driver_t *kdriver
    )
{
    vip_int32_t ret = 0;
    vip_uint32_t i = 0;
    vip_uint32_t total_core = 0;
    vip_uint32_t core_count = kdriver->core_count;
    vip_uint32_t axi_sram_size = 0;

    for (i = 0; i < kdriver->core_count; i++) {
        axi_sram_size += kdriver->axi_sram_size[i];
        if (0 == kdriver->vip_reg_phy[i]) {
            core_count = min(i, core_count);
            break;
        }
        if (0 == kdriver->vip_reg_size[i]) {
            core_count = min(i, core_count);
            PRINTK_D("register size break core count\n");
            break;
        }
        if (0 == kdriver->irq_line[i]) {
            core_count = min(i, core_count);
            PRINTK_D("irq line break core count\n");
            break;
        }
        #if defined (USE_LINUX_PCIE_DEVICE)
        if (0 == kdriver->pci_bars[i]) {
            core_count = min(i, core_count);
            PRINTK_D("pcie bar break core count\n");
            break;
        }
        #endif
    }

    if (core_count != kdriver->core_count) {
        PRINTK_E("warning update core count from %d to %d\n", kdriver->core_count, core_count);
        kdriver->core_count = core_count;
    }
    for (i = 0; i < gcdMAX_CORE; i++) {
        total_core += kdriver->device_core_number[i];
        if (total_core > kdriver->core_count) {
            kdriver->device_core_number[i] = 0;
        }
    }

    if (0 == kdriver->sys_heap_size) {
         /* default 2M bytes for system memory heap  */
        kdriver->sys_heap_size = 2 << 20;
    }

    if ((kdriver->vip_sram_size > 0) && (axi_sram_size > 0)) {
        if ((kdriver->vip_sram_base < (kdriver->axi_sram_base + axi_sram_size)) &&
            (kdriver->vip_sram_base >= kdriver->axi_sram_base)) {
            PRINTK_E("vipcore, axi-sram address=0x%08x overlap with vip sram.\n", kdriver->axi_sram_base);
            return -1;
        }
    }

    /* default full clock */
    kdriver->core_fscale_percent = 100;

    return ret;
}

/* used insmod command line to initialize parameters*/
static vip_status_e drv_init_params(
    gckvip_driver_t *kdriver
    )
{
    vip_uint32_t core = 0;
    kdriver->axi_sram_base = AXISramBaseAddress;
    kdriver->vip_sram_base = VIPSramBaseAddress;
    kdriver->vip_memsize = contiguousSize;
    kdriver->cpu_physical = (phy_address_t)contiguousBase;
#if vpmdENABLE_RESERVE_PHYSICAL
    kdriver->reserve_physical_base = reservePhysicalBase;
    kdriver->reserve_physical_size = reservePhysicalSize;
#endif

    for (core = 0; core < gcdMAX_CORE; core++) {
        kdriver->axi_sram_size[core] = AXISramSize[core];
        kdriver->vip_sram_size[core] = VIPSramSize[core];
        kdriver->irq_line[core] = irqLine[core];
        kdriver->vip_reg_phy[core] = registerMemBase[core];
        kdriver->vip_reg_size[core] = registerMemSize[core];
        #if defined (USE_LINUX_PCIE_DEVICE)
        kdriver->pci_bars[core] = pciBars[core];
        kdriver->reg_offset[core] = registerOffset[core];
        #endif
    }

    return VIP_SUCCESS;
}

#if vpmdENABLE_VIDEO_MEMORY_HEAP
static vip_int32_t drv_prepare_video_memory(
    gckvip_driver_t *kdriver
    )
{
    vip_int32_t ret = 0;
    vip_uint32_t prepare_flag = 1;

    /* Allocate the contiguous memory. */
    if (kdriver->vip_memsize >=  MIN_VIDEO_MEMORY_SIZE) {
#if defined (USE_LINUX_RESERVE_MEM)
        if ((kdriver->cpu_physical & (gcdVIP_MEMORY_ALIGN_BASE_ADDRESS - 1)) != 0) {
            PRINTK_E("failed to reserved memory contiguousBase=0x%"PRIx64"\n", kdriver->cpu_physical);
            return -1;
        }
        if (GCKVIP_RESERVED_MEM_BOOT_TIME == kdriver->reserved_mem_type) {
            /* map reserved memory in boot time */
            struct resource *region = VIP_NULL;
            region = request_mem_region(kdriver->cpu_physical, kdriver->vip_memsize, "vip_reserved");
            if (region == NULL) {
                PRINTK("vipcore, warning request mem region, address=0x%"PRIx64", size=0x%08x\n",
                        kdriver->cpu_physical, kdriver->vip_memsize);
            }
        }
        kdriver->cpu_virtual= VIP_NULL;
        kdriver->cpu_virtual_kernel = VIP_NULL;
        kdriver->vip_physical = gckvip_drv_get_vipphysical(kdriver->cpu_physical);
        PRINTK_D("vipcore, reserved memory type=%d, cpu_phy=0x%"PRIx64", vip_phy=0x%"PRIx64", size=0x%x\n",
                 kdriver->reserved_mem_type, kdriver->cpu_physical, kdriver->vip_physical,
                 kdriver->vip_memsize);
        if ((kdriver->vip_physical % (1 << 12)) != 0) {
           PRINTK("vipcore, vip physical base address must alignment with 4K\n");
           return -1;
        }
#else
        {
            /* 1. not reserved memory in boot time.
               2. not reserved memory in run time.
                  (May allocate contiguous physical from platform allocator)
               3. so alloc page for video memory heap.
            */
            u32 gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_NOWARN;
    #if !vpmdENABLE_MMU
            gfp &= ~__GFP_HIGHMEM;
        #if defined(CONFIG_ZONE_DMA32) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
            gfp |= __GFP_DMA32;
        #else
            gfp |= __GFP_DMA;
        #endif
    #endif
            PRINTK("vipcore, allocate page for video memory, size: 0x%xbytes\n", kdriver->vip_memsize);
            if (kdriver->vip_memsize > (4 << 20)) {
                PRINTK_E("vipcore, video memory heap size is more than 4Mbyte,"
                        "only can allocate 4M byte from page\n");
                kdriver->vip_memsize = (4 << 20);
            }

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
            {
            void *addr = alloc_pages_exact(kdriver->vip_memsize, gfp | __GFP_NORETRY);
            kdriver->pages = addr ? virt_to_page(addr) : VIP_NULL;
            }
    #endif
            if (VIP_NULL == kdriver->pages) {
                int order = get_order(kdriver->vip_memsize);
                kdriver->pages = alloc_pages(gfp, order);
                kdriver->is_exact_page = vip_false_e;
            }
            else {
                kdriver->is_exact_page = vip_true_e;
            }

            if (kdriver->pages == VIP_NULL) {
                PRINTK_E("vipcore, fail to alloc page, setting video memory heap size is 0\n");
                prepare_flag = 0;
            }
            else {
                kdriver->cpu_virtual = VIP_NULL;
                kdriver->cpu_virtual_kernel = VIP_NULL;
                kdriver->cpu_physical = page_to_phys(nth_page(kdriver->pages, 0));
                kdriver->vip_physical = gckvip_drv_get_vipphysical(kdriver->cpu_physical);
            }
            if (0 == kdriver->cpu_physical) {
                PRINTK_E("vipcore, fail to alloc page, get cpu physical is 0\n");
                prepare_flag = 0;
            }
            PRINTK("vipcore, cpu_physical=0x%"PRIx64", vip_physical=0x%"PRIx64", vip_memsize=0x%x\n",
                    kdriver->cpu_physical, kdriver->vip_physical, kdriver->vip_memsize);
        }
#endif
    }
    else {
        PRINTK_E("vipcore, reserved memory size=0x%x is small than mini=0x%x\n",
                  kdriver->vip_memsize, MIN_VIDEO_MEMORY_SIZE);

    }

    if ((0 == prepare_flag) || (0 == kdriver->cpu_physical)) {
        kdriver->vip_memsize = 0;
        kdriver->vip_physical = 0;
        kdriver->cpu_physical = 0;
        kdriver->vip_memsize = 0;
    }

#if !vpmdENABLE_MMU
    /* check vip-sram not overlap with video memory heap */
    if (kdriver->vip_sram_size > 0) {
        if ((kdriver->vip_sram_base < ((vip_uint32_t)kdriver->vip_physical + kdriver->vip_memsize)) &&
            (kdriver->vip_sram_base >= (vip_uint32_t)kdriver->vip_physical)) {
            PRINTK_E("vipcore, vip-sram address=0x%08x overlap with video memory heap\n", kdriver->vip_sram_base);
            return -1;
        }
    }
#endif

    if (kdriver->vip_memsize >= MIN_VIDEO_MEMORY_SIZE) {
        void *logical = VIP_NULL;
        int ret = map_kernel_logical(kdriver, kdriver->cpu_physical, kdriver->vip_memsize, &logical);
        if (ret < 0) {
            PRINTK("vipcore, fail to map physical address to kernel space\n");
            return -1;
        }
        kdriver->cpu_virtual_kernel = (void *)logical;
    }

    return ret;
}

static vip_int32_t drv_unprepare_video_memory(
    gckvip_driver_t *kdriver
    )
{
    if (kdriver->vip_memsize >  MIN_VIDEO_MEMORY_SIZE) {
        unmap_kernel_logical(kdriver);
#if defined(USE_LINUX_RESERVE_MEM)
        if (GCKVIP_RESERVED_MEM_BOOT_TIME == kdriver->reserved_mem_type) {
            release_mem_region(kdriver->cpu_physical, kdriver->vip_memsize);
        }
#else
        if (kdriver->pages != VIP_NULL) {
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
            if (kdriver->is_exact_page) {
                free_pages_exact(page_address(kdriver->pages), kdriver->vip_memsize);
            }
            else
        #endif
            {
                __free_pages(kdriver->pages, get_order(kdriver->vip_memsize));
            }
            PRINTK_I("vipcore, unmap kernel space and free video memory\n");
        }
#endif
    }
    kdriver->vip_memsize = 0;
    kdriver->vip_physical = 0;
    kdriver->cpu_physical = 0;
    kdriver->cpu_virtual_kernel = VIP_NULL;
    kdriver->pages = VIP_NULL;
    kdriver->cpu_virtual = VIP_NULL;

    return 0;
}

#endif

static vip_int32_t drv_prepare_device(
    gckvip_driver_t *kdriver
    )
{
    vip_int32_t ret = 0;

    if (1 == drvType) {
        /* Register as misc driver. */
        PRINTK("vipcore, register misc device\n");
        if (misc_register(&vipmisc_device) < 0) {
            PRINTK_E("vipcore, fail to register misc device\n");
            return -1;
        }
        kdriver->registered = 1;

        if (VIP_NULL == kdriver->device) {
            kdriver->device = vipmisc_device.this_device;
        }
    }
    else {
        struct device *device = VIP_NULL;
        vip_int32_t result = 0;
        /* Register device. */
        result = register_chrdev(major, DEVICE_NAME, &file_operations);
        if (result < 0) {
            PRINTK_E("vipcore, register_chrdev failed\n");
            return -1;
        }

        if (0 == major) {
            major = result;
        }

        kdriver->major = major;
        kdriver->registered = 1;

        /* Create the graphics class. */
        kdriver->class = class_create(THIS_MODULE, CLASS_NAME);
        if (IS_ERR(kdriver->class)) {
            PRINTK_E("vipcore, class_create failed\n");
            return -1;
        }

        /* Create the device. */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
        device = device_create(kdriver->class, NULL, MKDEV(kdriver->major, 0), NULL, DEVICE_NAME);
#else
        device = device_create(kdriver->class, NULL, MKDEV(kdriver->major, 0), DEVICE_NAME);
#endif
        if (device == NULL) {
            PRINTK_E("vipcore, device_create failed\n");
            return -1;
        }
        kdriver->created = 1;

        if (VIP_NULL == kdriver->device) {
            kdriver->device = device;
        }

        PRINTK_D("vipcore, created %s , device=0x%"PRPx", major=%d\n", DEVICE_NAME, device, kdriver->major);
    }

    return ret;
}

static vip_int32_t drv_unprepare_device(
    gckvip_driver_t *kdriver
    )
{
    if (kdriver->created) {
        /* Destroy the device. */
        device_destroy(kdriver->class, MKDEV(kdriver->major, 0));
        kdriver->created = 0;
    }

    if (kdriver->class != VIP_NULL) {
        /* Destroy the class. */
        class_destroy(kdriver->class);
    }

    if (kdriver->registered) {
        if (1 == drvType) {
            misc_deregister(&vipmisc_device);
        }
        else {
            /* Unregister the device. */
            unregister_chrdev(kdriver->major, "vipcore");
        }
        kdriver->registered = 0;
    }

    return 0;
}

static vip_int32_t drv_prepare_irq(
    gckvip_driver_t *kdriver
    )
{
#if !vpmdENABLE_POLLING
    vip_char_t *irq_name = VIP_NULL;
    vip_char_t *irq_name_default = "vipcore_def";
    vip_char_t *irq_name_core[8] = {
        "vipcore_0",
        "vipcore_1",
        "vipcore_2",
        "vipcore_3",
        "vipcore_4",
        "vipcore_5",
        "vipcore_6",
        "vipcore_7",
    };

    /* Install IRQ. */
    LOOP_CORE_START
    if (core < (sizeof(irq_name_core) / sizeof(vip_char_t *))) {
        irq_name = irq_name_core[core];
    }
    else {
        irq_name = irq_name_default;
    }
    PRINTK("core_%d, request irqline=%d, name=%s\n", core, kdriver->irq_line[core], irq_name);
    if (request_irq(kdriver->irq_line[core], drv_irq_handler, gcdVIP_IRQ_FLAG,
                    irq_name, (void *)(uintptr_t)(core + 1))) {
        PRINTK_E("vipcore, request_irq failed line=%d\n", kdriver->irq_line[core]);
        return -1;
    }
    kdriver->irq_registed[core] = 1;
    /* Initialize the wait queue. */
    init_waitqueue_head(&kdriver->irq_queue[core]);
    LOOP_CORE_END
#endif

    return 0;
}

static vip_int32_t drv_prepare_register(
    gckvip_driver_t *kdriver
    )
{
    vip_char_t *reg_name = VIP_NULL;
    vip_char_t *reg_name_default = "vipcore_reg_def";
    vip_char_t *reg_name_core[8] = {
        "vipcore_reg_0",
        "vipcore_reg_1",
        "vipcore_reg_2",
        "vipcore_reg_3",
        "vipcore_reg_4",
        "vipcore_reg_5",
        "vipcore_reg_6",
        "vipcore_reg_7",
    };

    LOOP_CORE_START
    if (kdriver->vip_reg[core] != VIP_NULL) {
        PRINTK_I("core%d regiseter has mapped in platform code\n", core);
        continue;
    }

    if (core < (sizeof(reg_name_core) / sizeof(vip_char_t *))) {
        reg_name = reg_name_core[core];
    }
    else {
        reg_name = reg_name_default;
    }
    if (request_mem_region(kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core], reg_name) == VIP_NULL) {
        PRINTK_E("vipcore, warning request register mem physical=0x%"PRIx64", size=0x%x name=%s\n",
            kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core], reg_name);
    }
    /* Map the VIP registers. */
#if defined (USE_LINUX_PCIE_DEVICE)
    kdriver->vip_reg[core] = (void *)pci_iomap(kdriver->pdev, kdriver->pci_bars[core],
                                               kdriver->pci_bar_size[kdriver->pci_bars[core]]);
    kdriver->pci_vip_reg[core] = kdriver->vip_reg[core];
    PRINTK_D("core_%d bar=%d, map logical base=0x%"PRIx64"\n", core, kdriver->pci_bars[core], kdriver->vip_reg[core]);
    kdriver->vip_reg[core] = (vip_uint8_t*)kdriver->vip_reg[core] + kdriver->reg_offset[core];
#else
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
    kdriver->vip_reg[core] = ioremap(kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core]);
    #else
    kdriver->vip_reg[core] = ioremap_nocache(kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core]);
    #endif
#endif
    if (kdriver->vip_reg[core] == VIP_NULL) {
        PRINTK_E("core_%d, ioremap failed for VIP memory.\n", core);
        return -1;
    }

    PRINTK_I("core_%d, register map address=0x%"PRPx"\n", core, kdriver->vip_reg[core]);

    kdriver->reg_drv_map[core] = 1;
    LOOP_CORE_END

    return 0;
}

#if DEFAULT_DISABLE_INTERNEL_CLOCK
static vip_int32_t drv_disable_internel_clock(
    gckvip_driver_t *kdriver
    )
{
    vip_uint32_t value = 0;

    value = SETBIT(value, 0, 1);
    value = SETBIT(value, 1, 1);
    value = SETBIT(value, 2, 1);
    value = SETBIT(value, 9, 1);

    LOOP_CORE_START
    if (VIP_NULL == kdriver->vip_reg[core]) {
        PRINTK_E("register is NULL\n");
        return -1;
    }
    writel(value, (uint8_t *)kdriver->vip_reg[core] + 0x00000);
    LOOP_CORE_END

    value = SETBIT(value, 9, 0);

    LOOP_CORE_START
    writel(value, (uint8_t *)kdriver->vip_reg[core] + 0x00000);
    LOOP_CORE_END
    PRINTK("hardware internel clock is disabled by default\n");

    return 0;
}
#endif

static void drv_exit(void)
{
    /* Check for valid device. */
    if (VIP_NULL == kdriver) {
        PRINTK("vipcore, kdriver is NULL, return\n");
        return;
    }

#if vpmdENABLE_DEBUGFS
    gckvip_debug_destroy_fs();
#endif

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    drv_unprepare_video_memory(kdriver);
#endif

    LOOP_CORE_START
    if (kdriver->vip_reg[core] != VIP_NULL) {
        if (kdriver->reg_drv_map[core]) {
            /* Unmap the vip registers. */
        #if defined (USE_LINUX_PCIE_DEVICE)
            iounmap((void*)kdriver->pci_vip_reg[core]);
        #else
            iounmap((void*)kdriver->vip_reg[core]);
        #endif
            release_mem_region(kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core]);
            kdriver->vip_reg[core] = VIP_NULL;
            kdriver->reg_drv_map[core] = 0;
        }
    }

#if !vpmdENABLE_POLLING
    free_irq(kdriver->irq_line[core],(void*)(uintptr_t)(core+1));
    if (kdriver->irq_registed[core]) {
        kdriver->irq_registed[core] = 0;
    }
#endif
    LOOP_CORE_END

    drv_unprepare_device(kdriver);

    gckvip_os_zero_memory(kdriver, sizeof(kdriver));
}

static vip_int32_t drv_init(void)
{
    vip_int32_t ret = 0;

    if (kdriver == VIP_NULL) {
        PRINTK("vipcore, failed vip device object is null\n");
        return -1;
    }

    if (drv_check_params(kdriver) < 0) {
        PRINTK("vipcore, failed to check parameters\n");
        return -1;
    }

    ret = drv_prepare_device(kdriver);
    if (ret < 0) {
        PRINTK_E("failed to register device node");
        goto error;
    }

    ret = drv_prepare_irq(kdriver);
    if (ret < 0) {
        PRINTK_E("failed to register irq");
        goto error;
    }

    ret = drv_prepare_register(kdriver);
    if (ret < 0) {
        PRINTK_E("failed to prepare register");
        goto error;
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    ret = drv_prepare_video_memory(kdriver);
    if (ret < 0) {
        PRINTK_E("failed to prepare video memory\n");
        goto error;
    }
#endif

#if (vpmdENABLE_DEBUG_LOG > 1)
    drv_show_params(kdriver);
#endif

#if vpmdENABLE_DEBUGFS
    ret = gckvip_debug_create_fs();
    if (ret < 0) {
       PRINTK("create viplite fs failed, error = %d.\n", ret);
    }
#endif

#if DEFAULT_DISABLE_INTERNEL_CLOCK
    /* disable internel clock by default */
    drv_disable_internel_clock(kdriver);
#endif

    PRINTK("VIPLite driver version %d.%d.%d.%s\n",
            VERSION_MAJOR, VERSION_MINOR, VERSION_SUB_MINOR, VERSION_PATCH);

    return 0;
error:
    drv_exit();
    return -1;
}

/*
@brief Get all hardware basic information.
*/
vip_status_e gckvip_drv_get_hardware_info(
    gckvip_hardware_info_t *info
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t iter = 0;

    info->max_core_count = kdriver->max_core_count;
    info->core_count = kdriver->core_count;
    info->core_fscale_percent = kdriver->core_fscale_percent;

    info->axi_sram_base = kdriver->axi_sram_base;
    info->vip_sram_base = kdriver->vip_sram_base;

    LOOP_CORE_START
    info->vip_reg[core] = kdriver->vip_reg[core];
    info->irq_queue[core] = (vip_uint32_t*)&kdriver->irq_queue[core];
    info->irq_value[core] = (vip_uint32_t*)&kdriver->irq_value[core];
    LOOP_CORE_END

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    info->cpu_virtual = kdriver->cpu_virtual;
    info->cpu_virtual_kernel = kdriver->cpu_virtual_kernel;
    info->vip_physical = kdriver->vip_physical;
    info->vip_memsize = kdriver->vip_memsize;

    if ((info->vip_physical % gcdVIP_MEMORY_ALIGN_BASE_ADDRESS) != 0) {
        PRINTK_E("vipcore, vip physical base address must alignment with 0x%xbytes\n",
                gcdVIP_MEMORY_ALIGN_BASE_ADDRESS);
        status = VIP_ERROR_IO;
    }

    if (kdriver->vip_memsize > 0) {
        if ((info->cpu_virtual == VIP_NULL) || (info->cpu_virtual_kernel == VIP_NULL) ||
            (info->vip_physical == 0)) {
            PRINTK_E("vipcore, fail user_logi=0x%"PRPx", ker_logi=0x%"PRPx", physical=0x%"PRIx64", size=0x%x\n",
                    kdriver->cpu_virtual, info->cpu_virtual_kernel, kdriver->vip_physical, kdriver->vip_memsize);
            status = VIP_ERROR_IO;
        }
    }
#endif
#if vpmdENABLE_SYS_MEMORY_HEAP
    info->sys_heap_size = kdriver->sys_heap_size;
#endif
#if vpmdENABLE_RESERVE_PHYSICAL
    info->reserve_phy_base = kdriver->reserve_physical_base;
    info->reserve_phy_size = kdriver->reserve_physical_size;
#endif

    for (iter = 0; iter < gcdMAX_CORE; iter++) {
        info->device_core_number[iter] = kdriver->device_core_number[iter];
        info->axi_sram_size[iter] = kdriver->axi_sram_size[iter];
        info->vip_sram_size[iter] = kdriver->vip_sram_size[iter];
    }

    return status;
}

/*
@brief Set power on/off and clock on/off
@param state, power status. refer to gckvip_power_status_e.
*/
vip_status_e gckvip_drv_set_power_clk(
    vip_uint32_t core,
    vip_uint32_t state
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t ret = 0;

    mutex_lock(&set_power_mutex);

    if (state == GCKVIP_POWER_ON) {
        if (state != kdriver->power_status[core]) {
            PRINTK_D("power on vip hardware%d.\n", core);
            ret = gckvip_drv_device_init(kdriver, core);
            if (ret < 0) {
                PRINTK_E("vipcore, failed to init device, ret=%d\n", ret);
                status = VIP_ERROR_FAILURE;
            }
            kdriver->power_status[core] = GCKVIP_POWER_ON;
        }
    }
    else if (state == GCKVIP_POWER_OFF) {
        if (state != kdriver->power_status[core]) {
            PRINTK_D("power off vip hardware%d.\n", core);
            ret = gckvip_drv_device_uninit(kdriver, core);
            if (ret < 0) {
                PRINTK_E("vipcore, failed to uninit device, ret=%d\n", ret);
                status = VIP_ERROR_FAILURE;
            }
            kdriver->power_status[core] = GCKVIP_POWER_OFF;
        }
    }
    else {
        PRINTK("vipcore, no this state=%d in set power clk\n", state);
        status = VIP_ERROR_FAILURE;
    }

    mutex_unlock(&set_power_mutex);

    return status;
}

/*
@brief do some initialize in this function.
@param, vip_memsizem, the size of video memory heap.
*/
vip_status_e gckvip_drv_init(
    vip_uint32_t video_memsize
    )
{
    vip_status_e status = VIP_SUCCESS;
    PRINTK_D("linux kernel version=0x%x\n", LINUX_VERSION_CODE);

    /* power on VIP */
    LOOP_CORE_START
    status = gckvip_drv_set_power_clk(core, GCKVIP_POWER_ON);
    if (status != VIP_SUCCESS) {
        PRINTK("vipcore, failed to power on\n");
        goto exit;
    }
    LOOP_CORE_END

#if vpmdENABLE_DEBUGFS
    gckvip_os_zero_memory(&kdriver->profile_data, sizeof(kdriver->profile_data));
#endif

exit:
    return status;
}

/*
@brief do some un-initialize in this function.
*/
vip_status_e gckvip_drv_exit(void)
{
    vip_status_e status = VIP_SUCCESS;

    LOOP_CORE_START
    status = gckvip_drv_set_power_clk(core, GCKVIP_POWER_OFF);
    if (status != VIP_SUCCESS) {
        PRINTK("vipcore, failed to power off\n");
    }
    LOOP_CORE_END

    return status;
}

#if defined (USE_LINUX_PCIE_DEVICE)
int gckvip_pcie_probe(
    struct pci_dev *pdev,
    const struct pci_device_id *ent
    )
{
    vip_int32_t ret = -ENODEV;
    vip_int32_t i = 0;
    vip_uint32_t total_core = 0;

    kdriver->pdev = pdev;
    kdriver->device = &pdev->dev;

    PRINTK("vipcore, pcie driver device=0x%"PRPx"\n", kdriver->device);

    if (pci_enable_device(pdev)) {
        PRINTK(KERN_ERR "vipcore, pci_enable_device() failed.\n");
    }

    pci_set_master(pdev);

    if (pci_request_regions(pdev, "vipcore")) {
        PRINTK(KERN_ERR "vipcore, Failed to get ownership of BAR region.\n");
    }
    /* adjust parameters from platform code */
    ret = gckvip_drv_adjust_param(kdriver);
    if (ret < 0) {
        PRINTK("vipcore, failed to adjust parameters\n");
        return -1;
    }

    if (kdriver->core_count > gcdMAX_CORE) {
        PRINTK("vipcore, failed init, the max core=%d core count=%d\n",
                 gcdMAX_CORE, kdriver->core_count);
        return -1;
    }

    if (0 == kdriver->core_count) {
        kdriver->core_count = 1;
    }

    for (i = 0; i < kdriver->core_count; i++) {
        total_core += kdriver->device_core_number[i];
    }
    if (0 == total_core) {
        kdriver->device_core_number[0] = 1;
    }

    /* power off VIP default */
    LOOP_CORE_START
    kdriver->power_status[core] = GCKVIP_POWER_OFF;
    LOOP_CORE_END

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,18,0)
    {
    #if vpmdENABLE_MMU
        u64 dma_mask = DMA_BIT_MASK(40);
    #else
        u64 dma_mask = DMA_BIT_MASK(32);
    #endif
        if (pci_set_dma_mask(pdev, dma_mask)) {
            PRINTK(KERN_ERR "vipcore, Failed to set DMA mask.\n");
        }
    }
#endif

    ret = drv_init();
    if (ret < 0) {
        PRINTK_E("vipcore, failed to drv init\n");
        return -1;
    }

    pci_set_drvdata(pdev, kdriver);

    return ret;
}

void gckvip_pcie_remove(
    struct pci_dev *pdev
    )
{
    drv_exit();

    gckvip_drv_unadjust_param(kdriver);

    pci_set_drvdata(pdev, NULL);
    pci_clear_master(pdev);
    pci_release_regions(pdev);
    pci_disable_device(pdev);

    return;
}

static struct pci_driver gcvip_pcie_driver = {
    .name = DEVICE_NAME,
    .probe = gckvip_pcie_probe,
    .remove = gckvip_pcie_remove
};

static int __init module_drv_init(void)
{
    int ret = 0;

    /* Create device structure. */
    kdriver = kmalloc(sizeof(gckvip_driver_t), GFP_KERNEL);
    if (kdriver == NULL) {
        PRINTK("vipcore, kmalloc failed for driver device.\n");
        return -1;
    }

    /* Zero structure. */
    memset(kdriver, 0, sizeof(gckvip_driver_t));
    kdriver->pdrv = &gcvip_pcie_driver;
    kdriver->log_level = DEFAULT_ENABLE_LOG;
    kdriver->func_config.enable_capture = DEFAULT_ENABLE_CAPTURE;
    kdriver->func_config.enable_cnn_profile = DEFAULT_ENABLE_CNN_PROFILE;
    kdriver->func_config.enable_dump_nbg = DEFAULT_ENABLE_DUMP_NBG;

    /* initialize parameters */
    drv_init_params(kdriver);

    gckvip_drv_platform_init(kdriver);

    ret = pci_register_driver(kdriver->pdrv);
    if (ret < 0) {
        PRINTK(KERN_ERR "vipcore, vip drv init() failed to register driver!\n");
        return ret;
    }

    return 0;
}

static void __exit module_drv_exit(void)
{
    pci_unregister_driver(&gcvip_pcie_driver);

    gckvip_drv_platform_uninit(kdriver);

    /* Free up the device structure. */
    kfree(kdriver);
    kdriver = VIP_NULL;
}

#elif defined (USE_LINUX_PLATFORM_DEVICE)
/* Linux platform device driver */
static vip_int32_t gckvip_platform_probe(
    struct platform_device *pdev
    )
{
    vip_int32_t ret = 0;
    vip_uint32_t i = 0;
    vip_uint32_t total_core = 0;
#if vpmdENABLE_MMU
    u64 dma_mask = DMA_BIT_MASK(40);
#else
    u64 dma_mask = DMA_BIT_MASK(32);
#endif

    kdriver->pdev = pdev;
    kdriver->device = &pdev->dev;

    PRINTK("vipcore, platform driver device=0x%"PRPx"\n", kdriver->device);
    /* adjust parameters from platform code */
    ret = gckvip_drv_adjust_param(kdriver);
    if (ret < 0) {
        PRINTK_E("vipcore, failed to adjust parameters\n");
        return -1;
    }

    if (kdriver->core_count > gcdMAX_CORE) {
        PRINTK_E("vipcore, failed init, the max core=%d core count=%d\n",
                 gcdMAX_CORE, kdriver->core_count);
        return -1;
    }

    if (0 == kdriver->core_count) {
        kdriver->core_count = 1;
    }

    for (i = 0; i < kdriver->core_count; i++) {
        total_core += kdriver->device_core_number[i];
    }
    if (0 == total_core) {
        kdriver->device_core_number[0] = 1;
    }

    PRINTK_D("vipcore, core count=%d\n", kdriver->core_count);
    /* power on VIP */
    LOOP_CORE_START
    kdriver->power_status[core] = GCKVIP_POWER_OFF;
    LOOP_CORE_END

    ret = dma_set_mask(kdriver->device, dma_mask);
    if (ret < 0) {
        kdriver->device->dma_mask = &dma_mask;
    }
    ret = drv_init();
    if (ret < 0) {
        PRINTK_E("vipcore, failed to drv init\n");
        return -1;
    }

    return ret;
}

static vip_int32_t gckvip_platform_remove(
    struct platform_device *pdev
    )
{
    drv_exit();

    gckvip_drv_unadjust_param(kdriver);

    return 0;
}

static vip_int32_t gckvip_platform_suspend(
    struct platform_device *pdev,
    pm_message_t state
    )
{
#if vpmdENABLE_SUSPEND_RESUME
    PRINTK_I("system suspend...\n");
    gckvip_pm_suspend();
#endif

    return 0;
}

static vip_int32_t gckvip_platform_resume(
    struct platform_device *pdev
    )
{
    vip_int32_t ret = 0;
#if vpmdENABLE_SUSPEND_RESUME
    vip_status_e status = VIP_SUCCESS;
    PRINTK_I("system resume...\n");
    status = gckvip_pm_resume();
    if (status != VIP_SUCCESS) {
        PRINTK("vipcore, failed to do pm init hardware status=%d\n", status);
        ret = -1;
        goto end;
    }
end:
#endif
    return ret;
}

static struct platform_driver gcvip_platform_driver = {
    .probe      = gckvip_platform_probe,
    .remove     = gckvip_platform_remove,
    .suspend    = gckvip_platform_suspend,
    .resume     = gckvip_platform_resume,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = DEVICE_NAME,
    }
};

static vip_int32_t __init module_drv_init(void)
{
    vip_int32_t ret = 0;

    /* Create device structure. */
    kdriver = kmalloc(sizeof(gckvip_driver_t), GFP_KERNEL);
    if (kdriver == NULL) {
        PRINTK("vipcore, kmalloc failed for driver device.\n");
        return -ENODEV;
    }

    /* Zero structure. */
    memset(kdriver, 0, sizeof(gckvip_driver_t));
    kdriver->pdrv = &gcvip_platform_driver;
    kdriver->log_level = DEFAULT_ENABLE_LOG;
    kdriver->func_config.enable_capture = DEFAULT_ENABLE_CAPTURE;
    kdriver->func_config.enable_cnn_profile = DEFAULT_ENABLE_CNN_PROFILE;
    kdriver->func_config.enable_dump_nbg = DEFAULT_ENABLE_DUMP_NBG;

    /* initialize parameters */
    drv_init_params(kdriver);

    /* get the pdrv->driver.of_match_table for match with platform device in DTS */
    ret = gckvip_drv_platform_init(kdriver);
    if (ret) {
        PRINTK(KERN_ERR "vipcore, Soc platform init failed.\n");
        return -ENODEV;
    }

    if (VIP_NULL == kdriver->pdrv->driver.of_match_table) {
        kdriver->pdev = platform_device_alloc(DEVICE_NAME, -1);
        if (!kdriver->pdev) {
            PRINTK(KERN_ERR "vipcore, fail to alloc platform device\n");
            return -ENOMEM;
        }
        ret = platform_device_add(kdriver->pdev);
        if (ret) {
            platform_device_unregister(kdriver->pdev);
            PRINTK(KERN_ERR "vipcore, fail to add platform device\n");
            return -ENOMEM;
        }
        PRINTK("vipcore, use vip driver alloc platform device name=%s\n", DEVICE_NAME);
    }
    else {
        /* used the DTS platform device */
        PRINTK("vipcore, platform device compatible=%s\n", kdriver->pdrv->driver.of_match_table->compatible);
    }

    ret = platform_driver_register(kdriver->pdrv);
    if (ret < 0) {
        PRINTK(KERN_ERR "vipcore, platform driver register failed.\n");
        return -ENODEV;
    }

    return ret;
}

static void __exit module_drv_exit(void)
{
    vip_int32_t ret = 0;

    platform_driver_unregister(kdriver->pdrv);

    if ((VIP_NULL == kdriver->pdrv->driver.of_match_table) && (kdriver->pdev != VIP_NULL)) {
        platform_device_unregister(kdriver->pdev);
        kdriver->pdev = VIP_NULL;
    }

    ret = gckvip_drv_platform_uninit(kdriver);
    if (ret < 0) {
        PRINTK(KERN_ERR "vipcore, platform driver uninit failed.\n");
    }

    /* Free up the device structure. */
    kfree(kdriver);
    kdriver = VIP_NULL;
}

#else

static vip_int32_t __init module_drv_init(void)
{
    vip_int32_t ret = 0;

    /* Create device structure. */
    kdriver = kmalloc(sizeof(gckvip_driver_t), GFP_KERNEL);
    if (kdriver == NULL) {
        PRINTK("vipcore, kmalloc failed for driver device.\n");
        return -ENODEV;
    }

    PRINTK("vipcore, not on any bus to initialize driver\n");
    /* Zero structure. */
    memset(kdriver, 0, sizeof(gckvip_driver_t));
    kdriver->log_level = DEFAULT_ENABLE_LOG;
    kdriver->func_config.enable_capture = DEFAULT_ENABLE_CAPTURE;
    kdriver->func_config.enable_cnn_profile = DEFAULT_ENABLE_CNN_PROFILE;
    kdriver->func_config.enable_dump_nbg = DEFAULT_ENABLE_DUMP_NBG;

    /* initialize parameters */
    drv_init_params(kdriver);

    /* adjust parameters from platform code */
    ret = gckvip_drv_adjust_param(kdriver);
    if (ret < 0) {
        PRINTK("vipcore, failed to adjust parameters\n");
        goto exit;
    }

    if (0 == kdriver->core_count) {
        kdriver->core_count = 1;
    }
    if (0 == kdriver->device_core_number[0]) {
        kdriver->device_core_number[0] = 1;
    }

    LOOP_CORE_START
    kdriver->power_status[core] = GCKVIP_POWER_OFF;
    LOOP_CORE_END

    ret = drv_init();
    if (ret < 0) {
        PRINTK("vipcore, failed to drv init\n");
        goto exit;
    }

    return 0;

exit:
    return -1;
}

static void __exit module_drv_exit(void)
{
    drv_exit();

    gckvip_drv_unadjust_param(kdriver);

    /* Free up the device structure. */
    kfree(kdriver);
    kdriver = VIP_NULL;
}
#endif

module_init(module_drv_init)
module_exit(module_drv_exit)

MODULE_AUTHOR("VeriSilicon Corporation");
MODULE_DESCRIPTION("NPU driver");
MODULE_LICENSE("Dual MIT/GPL");

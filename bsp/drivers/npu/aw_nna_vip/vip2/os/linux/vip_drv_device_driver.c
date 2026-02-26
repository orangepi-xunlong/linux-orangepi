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
#include <vip_drv_pm.h>
#include <vip_drv_debug.h>
#include <vip_drv_device_driver.h>
#include <vip_drv_os.h>
#include <vip_lite_version.h>
#include <vip_drv_video_memory.h>
#include <vip_drv_interface.h>
#include <vip_drv_context.h>

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

static vip_uint32_t pciBars[vipdMAX_CORE] = {[0 ... vipdMAX_CORE -1] = 0};
module_param_array(pciBars, uint, NULL, 0644);
MODULE_PARM_DESC(pciBars, " the index of each pcie bars for vip multi-core");

static ulong registerMemBase[vipdMAX_CORE] = {[0 ... vipdMAX_CORE -1] = 0};
module_param_array(registerMemBase, ulong, NULL, 0644);
MODULE_PARM_DESC(registerMemBase, "AHB register base physical address of vip multi-core");

static vip_uint32_t registerMemSize[vipdMAX_CORE] = {[0 ... vipdMAX_CORE -1] = 0};
module_param_array(registerMemSize, uint, NULL, 0644);
MODULE_PARM_DESC(registerMemSize, "AHB register memory size of vip multi-core");

static vip_uint32_t registerOffset[vipdMAX_CORE] = {[0 ... vipdMAX_CORE -1] = 0};
module_param_array(registerOffset, uint, NULL, 0644);
MODULE_PARM_DESC(registerOffset, "AHB register offsets in corresponding BAR space, only for PCIE device");

static vip_uint32_t irqLine[vipdMAX_CORE] = {[0 ... vipdMAX_CORE -1] = 0};
module_param_array(irqLine, uint, NULL, 0644);
MODULE_PARM_DESC(irqLine, "irq line number of vip multi-core");

static ulong contiguousSize[VIDEO_MEMORY_HEAP_MAX_CNT] = {[0 ... VIDEO_MEMORY_HEAP_MAX_CNT -1] = 0};
module_param_array(contiguousSize, ulong, NULL, 0644);
MODULE_PARM_DESC(contiguousSize, "defalut video memory heap size");

static ulong contiguousBase[VIDEO_MEMORY_HEAP_MAX_CNT] = {[0 ... VIDEO_MEMORY_HEAP_MAX_CNT -1] = 0};
module_param_array(contiguousBase, ulong, NULL, 0644);
MODULE_PARM_DESC(contiguousBase, "the base pyhsical address of video memory heap");

static ulong allocatorIndex[VIDEO_MEMORY_HEAP_MAX_CNT] = {[0 ... VIDEO_MEMORY_HEAP_MAX_CNT -1] = (ulong)-1};
module_param_array(allocatorIndex, ulong, NULL, 0644);
MODULE_PARM_DESC(allocatorIndex, "the allocator index of each video memory heap. 0:heap reserved; 1:heap exclusive");

static ulong deviceVis[VIDEO_MEMORY_HEAP_MAX_CNT] = {[0 ... VIDEO_MEMORY_HEAP_MAX_CNT -1] = (ulong)-1};
module_param_array(deviceVis, ulong, NULL, 0644);
MODULE_PARM_DESC(deviceVis, "the visibility bitmap of heap for each device");
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

static vip_uint32_t AXISramSize[vipdMAX_CORE] = {[0 ... vipdMAX_CORE -1] = 0};
module_param_array(AXISramSize, uint, NULL, 0644);
MODULE_PARM_DESC(AXISramSize, "the size of AXI SRAM");

static ulong AXISramBaseAddress = 0;
module_param(AXISramBaseAddress, ulong, 0644);
MODULE_PARM_DESC(AXISramBaseAddress, "the npu physical base address of AXI SRAM");

static vip_uint32_t VIPSramBaseAddress = 0;
module_param(VIPSramBaseAddress, uint, 0644);
MODULE_PARM_DESC(VIPSramBaseAddress, "the base address of VIP SRAM");


#define MIN_VIDEO_MEMORY_SIZE       (4 * 1024)
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


#ifndef vpmdREGISTER_PCIE_DRIVER
#define vpmdREGISTER_PCIE_DRIVER            1
#endif


/*
@brief define loop fun for each vip core
*/
#define LOOP_CORE_START                                  \
{  vip_uint32_t core = 0;                                \
   vipdrv_driver_t *kdriver = vipdrv_get_kdriver();      \
   for (core = 0 ; core < kdriver->core_count; core++) {

#define LOOP_CORE_END }}

/*
@brief define loop fun for each heap
*/
#define LOOP_HEAP_START                                                     \
{  vip_uint32_t heap_index = 0;                                             \
   vipdrv_driver_t *kdriver = vipdrv_get_kdriver();                         \
   for (heap_index = 0 ; heap_index < kdriver->vip_mem_cnt; heap_index++) {

#define LOOP_HEAP_END }}

static DEFINE_MUTEX(set_power_mutex);
static DEFINE_MUTEX(open_mutex);

/* Global variable for vip device */
vipdrv_driver_t *kdriver = VIP_NULL;

vipdrv_driver_t* vipdrv_get_kdriver(void)
{
    return kdriver;
}

static vip_int32_t drv_open(struct inode * inode, struct file * file)
{
    vip_int32_t ret = 0;

    mutex_lock(&open_mutex);
    kdriver->open_device_count++;

    file->private_data = (void*)kdriver;
    PRINTK_D("drv open process pid=%x, tid=%x\n", vipdrv_os_get_pid(), vipdrv_os_get_tid());

#if !vpmdTASK_QUEUE_USED
    if (kdriver->open_device_count > kdriver->device_count) {
        ret = -1;
        kdriver->open_device_count--;
        PRINTK_E("fail to open device, not used task queue open cnt=%d, device_cnt=%d\n",
                kdriver->open_device_count, kdriver->device_count);
    }
#endif
    mutex_unlock(&open_mutex);

    return ret;
}

static vip_int32_t drv_release(struct inode * inode, struct file * file)
{
    mutex_lock(&open_mutex);
    kdriver->open_device_count--;

    PRINTK_D("drv release process pid=%x, tid=%x\n", vipdrv_os_get_pid(), vipdrv_os_get_tid());

    vipdrv_destroy();
    mutex_unlock(&open_mutex);

    return 0;
}

static long drv_ioctl(struct file *file, unsigned int ioctl_code, unsigned long arg)
{
    struct ioctl_data arguments;
    void __user *uarg = (void __user *)arg;
    void *data = VIP_NULL;
    void *heap_data = VIP_NULL;
    vipdrv_command_data stack_data;

    if (ioctl_code != VIPDRV_IOCTL) {
        PRINTK("vipcore, error ioctl code is %d\n", ioctl_code);
        return -1;
    }
    if (uarg == VIP_NULL) {
        PRINTK("vipcore, error arg is NULL\n");
        return -1;
    }
    if (copy_from_user(&arguments, uarg, sizeof(arguments)) != 0) {
        PRINTK("vipcore, fail to copy arguments\n");
        return -1;
    }
    data = &stack_data;
    if (arguments.bytes > 0) {
        if (arguments.bytes >= sizeof(vipdrv_command_data)) {
            vipdrv_os_allocate_memory(arguments.bytes, (void **)&heap_data);
            if (VIP_NULL == heap_data) {
                PRINTK("vipcore, fail to malloc memory for ioctl data");
                return -1;
            }
            data = heap_data;
        }

        if (copy_from_user(data, (void*)(vipdrv_uintptr_t)(arguments.buffer), arguments.bytes) != 0) {
            PRINTK("vipcore, %s fail to copy arguments buffer, size: %d\n", __func__, arguments.bytes);
            goto error;
        }
    }

    /* Execute vip-drv command. */
    arguments.error = vipdrv_interface_call(arguments.command, data);
    if (arguments.error != VIP_SUCCESS) {
        if (copy_to_user(uarg, &arguments, sizeof(arguments)) != 0) {
            PRINTK("vipcore, fail to copy data to user arguments\n");
        }
        goto error;
    }

    if (arguments.bytes > 0) {
        if (copy_to_user((void*)(vipdrv_uintptr_t)arguments.buffer, data, arguments.bytes) != 0) {
            PRINTK("vipcore, %s fail to copy data to user arguments buffer, size: %d\n", __func__,
                    arguments.bytes);
            goto error;
        }
    }

    if (copy_to_user(uarg, &arguments, sizeof(arguments)) != 0) {
        PRINTK("vipcore, fail to copy data to user arguments\n");
        goto error;
    }

    if (heap_data != VIP_NULL) {
        vipdrv_os_free_memory(heap_data);
        heap_data = VIP_NULL;
    }
    return 0;

error:
    if ((arguments.error != VIP_ERROR_POWER_OFF) && (arguments.error != VIP_ERROR_CANCELED)) {
        vipdrv_intrface_e index = arguments.command;
        if (index > VIPDRV_INTRFACE_MAX) {
            index = VIPDRV_INTRFACE_MAX;
        }
        PRINTK("vipcore, fail to ioctl, command[%d]: %s, status=%d\n",
                (vip_int32_t)arguments.command, ioctl_cmd_string[index], arguments.error);
    }
    if (heap_data != VIP_NULL) {
        vipdrv_os_free_memory(heap_data);
    }
    return -1;
}

static long drv_compat_ioctl(struct file *file, unsigned int ioctl_code, unsigned long arg)
{
#ifdef CONFIG_COMPAT
    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,4)
    return compat_ptr_ioctl(file, ioctl_code, arg);
    #else
    return drv_ioctl(file, ioctl_code, (unsigned long)compat_ptr(arg));
    #endif
#else
    return drv_ioctl(file, ioctl_code, arg);
#endif
}

static struct file_operations file_operations =
{
    .owner          = THIS_MODULE,
    .open           = drv_open,
    .release        = drv_release,
    .unlocked_ioctl = drv_ioctl,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(5,8,18)
#ifdef HAVE_COMPAT_IOCTL
    .compat_ioctl   = drv_compat_ioctl,
#endif
#else
    .compat_ioctl   = drv_compat_ioctl,
#endif
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
    uint8_t handled_flag = 0;
    vip_uint32_t core = VIPDRV_PTR_TO_UINT32(data) - 1;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

    if (VIP_NULL == kdriver->vip_reg[core]) {
        return IRQ_NONE;
    }
    /* Read interrupt status. */
    value = readl((uint8_t *)kdriver->vip_reg[core] + 0x00010);
    if (value != 0x00) {
        /* Combine with current interrupt flags. */
        kdriver->irq_value[core] = value;
        while (0 != value) {
            value &= (value - 1);
            vipdrv_os_inc_atomic(kdriver->irq_count[core]);
        }
        kdriver->irq_value[kdriver->irq_map[core]] |= IRQ_VALUE_FLAG;
        /* Wake up any waiters. */
        vipdrv_os_wakeup_interrupt(&kdriver->irq_queue[kdriver->irq_map[core]]);
        handled_flag = 1;
    }

#if vpmdENABLE_FUSA
    {
        uint32_t error_value_ext = 0, error_value= 0;
        error_value = readl((uint8_t *)kdriver->vip_reg[core] + 0x00340);
        error_value_ext = readl((uint8_t *)kdriver->vip_reg[core] + 0x00350);
        if ((error_value != 0) || (error_value_ext != 0)) {
            PRINTK("irq error_value_ext=0x%x error_value=0x%x\n", error_value_ext, error_value);
            kdriver->irq_value[kdriver->irq_map[core]] |= IRQ_VALUE_ERROR_FUSA;
            vipdrv_os_wakeup_interrupt(&kdriver->irq_queue[kdriver->irq_map[core]]);
            handled_flag = 1;
        }
    }
#endif

    if (1 == handled_flag) {
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
    vipdrv_driver_t *kdriver
    )
{
    #define NAME_HEADER_SIZE  20
    vip_char_t info[256];
    vip_char_t *tmp = info;
    if (verbose) {
        PRINTK("=======vipcore parameter=====\n");
        vipdrv_os_copy_memory(tmp, "registerMemBase     ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        vipdrv_os_snprint(tmp, 20, "0x%012x, ", kdriver->vip_reg_phy[core]);
        tmp += 16;
        LOOP_CORE_END
        PRINTK("%s\n", info);

        tmp = info;
        vipdrv_os_copy_memory(tmp, "registerMemSize     ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        vipdrv_os_snprint(tmp, 20, "0x%08x, ", kdriver->vip_reg_size[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);

        tmp = info;
        vipdrv_os_copy_memory(tmp, "irqLine             ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        vipdrv_os_snprint(tmp, 20, "0x%08x, ", kdriver->irq_line[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);

        #if defined (USE_LINUX_PCIE_DEVICE)
        tmp = info;
        vipdrv_os_copy_memory(tmp, "pciBars             ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        vipdrv_os_snprint(tmp, 20, "0x%08x, ", kdriver->pci_bars[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);
        tmp = info;
        vipdrv_os_copy_memory(tmp, "registerOffset      ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        vipdrv_os_snprint(tmp, 20, "0x%08x, ", kdriver->reg_offset[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);
        #endif
        #if vpmdENABLE_VIDEO_MEMORY_HEAP
        tmp = info;
        vipdrv_os_copy_memory(tmp, "contiguousSize      ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_HEAP_START
        vipdrv_os_snprint(tmp, 20, "0x%08x, ", kdriver->heap_infos[heap_index].vip_memsize);
        tmp += 12;
        LOOP_HEAP_END
        PRINTK("%s\n", info);
        tmp = info;
        vipdrv_os_copy_memory(tmp, "cpuContiguousBase   ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_HEAP_START
        vipdrv_os_snprint(tmp, 20, "0x%"PRIx64", ", kdriver->cpu_physical[heap_index]);
        if (kdriver->cpu_physical[heap_index] & 0xFF00000000) {
            tmp += 14;
        }
        else {
            tmp += 12;
        }
        LOOP_HEAP_END
        PRINTK("%s\n", info);
        tmp = info;
        vipdrv_os_copy_memory(tmp, "vipContiguousBase   ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_HEAP_START
        vipdrv_os_snprint(tmp, 20, "0x%"PRIx64", ", kdriver->heap_infos[heap_index].vip_physical);
        if (kdriver->heap_infos[heap_index].vip_physical & 0xFF00000000) {
            tmp += 14;
        }
        else {
            tmp += 12;
        }
        LOOP_HEAP_END
        PRINTK("%s\n", info);
        tmp = info;
        vipdrv_os_copy_memory(tmp, "allocator name      ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_HEAP_START
        vipdrv_os_snprint(tmp, 20, "%s, ", kdriver->heap_infos[heap_index].name);
        tmp += 15;
        LOOP_HEAP_END
        PRINTK("%s\n", info);
        tmp = info;
        vipdrv_os_copy_memory(tmp, "allocatorIndex      ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_HEAP_START
        vipdrv_os_snprint(tmp, 20, "0x%08x, ", allocatorIndex[heap_index]);
        tmp += 12;
        LOOP_HEAP_END
        PRINTK("%s\n", info);
        tmp = info;
        vipdrv_os_copy_memory(tmp, "device visibility   ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_HEAP_START
        vipdrv_os_snprint(tmp, 20, "0x%08x, ", deviceVis[heap_index]);
        tmp += 12;
        LOOP_HEAP_END
        PRINTK("%s\n", info);
        #endif
        PRINTK("drvType             0x%08x\n", drvType);
        PRINTK("AXISramBaseAddress  0x%"PRIx64"\n", kdriver->axi_sram_base);
        tmp = info;
        vipdrv_os_copy_memory(tmp, "AXISramSize         ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        vipdrv_os_snprint(tmp, 20, "0x%08x, ", kdriver->axi_sram_size[core]);
        tmp += 12;
        LOOP_CORE_END
        PRINTK("%s\n", info);
        PRINTK("VIPSramBaseAddress  0x%08x\n", kdriver->vip_sram_base);
        #if vpmdENABLE_SYS_MEMORY_HEAP
        PRINTK("sysHeapSize         0x%08x\n", kdriver->sys_heap_size);
        #endif
        #if vpmdENABLE_RESERVE_PHYSICAL
        PRINTK("reservePhysicalBase 0x%"PRIx64"\n", kdriver->reserve_physical_base);
        PRINTK("reservePhysicalSize 0x%08x\n", kdriver->reserve_physical_size);
        #endif
        tmp = info;
        vipdrv_os_copy_memory(tmp, "core cnt per-device ", NAME_HEADER_SIZE);
        tmp += NAME_HEADER_SIZE;
        LOOP_CORE_START
        if (kdriver->device_core_number[core] != 0) {
        vipdrv_os_snprint(tmp, 20, "%02d, ", kdriver->device_core_number[core]);
        tmp += 4;
        }
        LOOP_CORE_END
        PRINTK("%s\n", info);
        PRINTK("===============================\n");
    }

    return 0;
}
#endif

static vip_int32_t drv_check_params(
    vipdrv_driver_t *kdriver
    )
{
    vip_int32_t ret = 0;
    vip_uint32_t i = 0;
    vip_uint32_t total_core = 0;
    vip_uint32_t core_count = kdriver->core_count;
    vip_uint32_t axi_sram_size = 0;

    for (i = 0; i < kdriver->core_count; i++) {
        axi_sram_size += kdriver->axi_sram_size[i];
        #ifndef USE_LINUX_PCIE_DEVICE
        if (0 == kdriver->vip_reg_phy[i]) {
            core_count = min(i, core_count);
            break;
        }
        #endif
        if (0 == kdriver->vip_reg_size[i]) {
            core_count = min(i, core_count);
            PRINTK_E("register size break core count\n");
            break;
        }
        if (0 == kdriver->irq_line[i]) {
            core_count = min(i, core_count);
            PRINTK_E("irq line break core count\n");
            break;
        }
    }

    if (core_count != kdriver->core_count) {
        PRINTK_E("warning update core count from %d to %d\n", kdriver->core_count, core_count);
        kdriver->core_count = core_count;
    }
    for (i = 0; i < vipdMAX_CORE; i++) {
        if (kdriver->device_core_number[i] > 0) {
            total_core += kdriver->device_core_number[i];
            if (total_core > kdriver->core_count) {
                kdriver->device_core_number[i] = kdriver->core_count;
            }
        }
        else {
            break;
        }
        kdriver->device_count++;
    }

    if (0 == kdriver->sys_heap_size) {
         /* default 2M bytes for system memory heap  */
        kdriver->sys_heap_size = 2 << 20;
    }

    /* default full clock */
    kdriver->core_fscale_percent = 100;
    PRINTK("vipcore, device_cnt=%d, core_cnt=%d\n", kdriver->device_count, kdriver->core_count);

    return ret;
}

/* used insmod command line to initialize parameters*/
static vip_status_e drv_init_params(
    vipdrv_driver_t *kdriver
    )
{
    vip_uint32_t index = 0;
    kdriver->axi_sram_base = AXISramBaseAddress;
    kdriver->vip_sram_base = VIPSramBaseAddress;
#if vpmdENABLE_VIDEO_MEMORY_HEAP
    for (index = 0; index < VIDEO_MEMORY_HEAP_MAX_CNT; index++) {
        kdriver->heap_infos[index].vip_memsize = (vip_size_t)contiguousSize[index];
        kdriver->cpu_physical[index] = (vip_physical_t)contiguousBase[index];
        PRINTK("vipcore, insmod param cpu_physical=0x%"PRIx64"\n", contiguousBase[index]);
        if (0 == allocatorIndex[index]) {
            kdriver->heap_infos[index].name = ALLOCATOR_NAME_HEAP_RESERVED;
        }
        else if (1 == allocatorIndex[index]) {
            kdriver->heap_infos[index].name = ALLOCATOR_NAME_HEAP_EXCLUSIVE;
        }
        else {
            kdriver->heap_infos[index].name = "";
        }
        kdriver->heap_infos[index].device_vis = deviceVis[index];
    }
#endif
#if vpmdENABLE_RESERVE_PHYSICAL
    kdriver->reserve_physical_base = reservePhysicalBase;
    kdriver->reserve_physical_size = reservePhysicalSize;
#endif

    for (index = 0; index < vipdMAX_CORE; index++) {
        kdriver->axi_sram_size[index] = AXISramSize[index];
        kdriver->irq_line[index] = irqLine[index];
        kdriver->vip_reg_phy[index] = registerMemBase[index];
        kdriver->vip_reg_size[index] = registerMemSize[index];
        #if defined (USE_LINUX_PCIE_DEVICE)
        kdriver->pci_bars[index] = pciBars[index];
        kdriver->reg_offset[index] = registerOffset[index];
        #endif
    }

    return VIP_SUCCESS;
}

#if vpmdENABLE_VIDEO_MEMORY_HEAP
static vip_int32_t drv_prepare_video_memory(
    vipdrv_driver_t *kdriver
    )
{
    vip_int32_t ret = 0;
    vip_uint32_t prepare_flag = 1;
    vip_uint32_t heap_cnt = 0;
    vip_uint32_t i = 0;
    u32 gfp = 0;
    for (i = 0; i < VIDEO_MEMORY_HEAP_MAX_CNT; i++) {
        /* Allocate the contiguous memory. */
        if (kdriver->heap_infos[i].vip_memsize >=  MIN_VIDEO_MEMORY_SIZE &&
            kdriver->cpu_physical[i] > 0) {
            if ((kdriver->cpu_physical[i] & (gcdVIP_MEMORY_ALIGN_BASE_ADDRESS - 1)) != 0) {
                PRINTK_E("fail to reserved memory contiguousBase=0x%"PRIx64"\n", kdriver->cpu_physical[i]);
                return -1;
            }
            if (0 == vipdrv_os_strcmp(ALLOCATOR_NAME_HEAP_EXCLUSIVE, kdriver->heap_infos[i].name)) {
                /* map reserved memory in boot time */
                struct resource *region = VIP_NULL;
                region = request_mem_region(kdriver->cpu_physical[i],
                            kdriver->heap_infos[i].vip_memsize, "vip_reserved");
                if (region == NULL) {
                    PRINTK("vipcore, warning request mem region, address=0x%"PRIx64", size=0x%"PRIx64"\n",
                            kdriver->cpu_physical[i], kdriver->heap_infos[i].vip_memsize);
                }
                allocatorIndex[heap_cnt] = 1;
            }
            else if (0 == vipdrv_os_strcmp(ALLOCATOR_NAME_HEAP_RESERVED, kdriver->heap_infos[i].name)) {
                allocatorIndex[heap_cnt] = 0;
            }

            kdriver->heap_infos[i].vip_physical = vipdrv_drv_get_vipphysical(kdriver->cpu_physical[i]);
            kdriver->heap_infos[i].ops.phy_cpu2vip = vipdrv_drv_get_vipphysical;
            kdriver->heap_infos[i].ops.phy_vip2cpu = vipdrv_drv_get_cpuphysical;
            kdriver->heap_infos[i].device_vis = deviceVis[i];
            if ((kdriver->heap_infos[i].vip_physical % (1 << 12)) != 0) {
                PRINTK("vipcore, vip physical base address must alignment with 4K\n");
                return -1;
            }
            PRINTK("vipcore, heap_cnt=%d, name=%s video memory cpu_phy=0x%"PRIx64", vip_phy=0x%"PRIx64", size=0x%x\n",
                heap_cnt, kdriver->heap_infos[i].name, kdriver->cpu_physical[i],
                kdriver->heap_infos[i].vip_physical, kdriver->heap_infos[heap_cnt].vip_memsize);
        }

        if ((0 == prepare_flag) || (0 == kdriver->cpu_physical[i])) {
            continue;
        }
        heap_cnt++;
    }

    /* not heap-exclusive memory, alloc 4M for default video memory heap */
    if (0 == heap_cnt) {
        gfp = GFP_KERNEL | __GFP_HIGHMEM | __GFP_NOWARN;
		#if !vpmdENABLE_MMU || LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
        gfp &= ~__GFP_HIGHMEM;
        #if defined(CONFIG_ZONE_DMA32) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
        gfp |= __GFP_DMA32;
        #else
        gfp |= __GFP_DMA;
        #endif
        #endif
        kdriver->heap_infos[heap_cnt].vip_memsize = (4 << 20);
        PRINTK("vipcore, allocate page for video memory, size: 0x%x bytes\n",
                kdriver->heap_infos[heap_cnt].vip_memsize);
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
        {
        void *addr = alloc_pages_exact(kdriver->heap_infos[heap_cnt].vip_memsize, gfp | __GFP_NORETRY);
        kdriver->pages[heap_cnt] = addr ? virt_to_page(addr) : VIP_NULL;
        }
        #else
        {
        int order = get_order(kdriver->heap_infos[heap_cnt].vip_memsize);
        kdriver->pages[heap_cnt] = alloc_pages(gfp, order);
        }
        #endif

        if (kdriver->pages[heap_cnt] == VIP_NULL) {
            PRINTK_E("vipcore, fail to alloc page, setting video memory heap size is 0\n");
            prepare_flag = 0;
        }
        else {
            kdriver->cpu_physical[heap_cnt] = page_to_phys(nth_page(kdriver->pages[heap_cnt], 0));
            kdriver->heap_infos[heap_cnt].vip_physical =
                                        vipdrv_drv_get_vipphysical(kdriver->cpu_physical[heap_cnt]);
        }
        if (0 == kdriver->cpu_physical[heap_cnt]) {
            PRINTK_E("vipcore, fail to alloc page, get cpu physical is 0\n");
            prepare_flag = 0;
            kdriver->heap_infos[heap_cnt].vip_memsize = 0;
        }
        kdriver->heap_infos[heap_cnt].name = ALLOCATOR_NAME_HEAP_RESERVED;
        kdriver->heap_infos[heap_cnt].ops.phy_cpu2vip = vipdrv_drv_get_vipphysical;
        kdriver->heap_infos[heap_cnt].ops.phy_vip2cpu = vipdrv_drv_get_cpuphysical;
        kdriver->heap_infos[heap_cnt].device_vis = ~(vip_uint64_t)0;
        PRINTK("vipcore, heap_cnt=%d, reserved video memory cpu_phy=0x%"PRIx64", "
                "vip_phy=0x%"PRIx64", size=0x%x\n",
                heap_cnt, kdriver->cpu_physical[heap_cnt], kdriver->heap_infos[heap_cnt].vip_physical,
                kdriver->heap_infos[heap_cnt].vip_memsize);
        allocatorIndex[heap_cnt] = 0;
        heap_cnt++;
    }

    kdriver->vip_mem_cnt = heap_cnt;

    return ret;
}

static vip_int32_t drv_unprepare_video_memory(
    vipdrv_driver_t *kdriver
    )
{
    LOOP_HEAP_START
    if (heap_index == kdriver->vip_mem_cnt - 1) {
        if (kdriver->pages[heap_index] != VIP_NULL) {
            #if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
            free_pages_exact(page_address(kdriver->pages[heap_index]), kdriver->heap_infos[heap_index].vip_memsize);
            #else
            __free_pages(kdriver->pages[heap_index], get_order(kdriver->heap_infos[heap_index].vip_memsize));
            #endif
            PRINTK_I("vipcore, unmap kernel space and free video memory\n");
        }
    }
    else {
        if (0 == vipdrv_os_strcmp(ALLOCATOR_NAME_HEAP_EXCLUSIVE, kdriver->heap_infos[heap_index].name)) {
            release_mem_region(kdriver->cpu_physical[heap_index], kdriver->heap_infos[heap_index].vip_memsize);
        }
    }

    kdriver->heap_infos[heap_index].vip_memsize = 0;
    kdriver->heap_infos[heap_index].vip_physical = 0;
    kdriver->cpu_physical[heap_index] = 0;
    kdriver->pages[heap_index] = VIP_NULL;
    LOOP_HEAP_END

    return 0;
}

#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 2, 0)
static char *vip_devnode(const struct device *dev, umode_t *mode)
#else
static char *vip_devnode(struct device *dev, umode_t *mode)
#endif
{
    if (mode)
        *mode = 0666;
    return NULL;
}

static vip_int32_t drv_prepare_device(
    vipdrv_driver_t *kdriver
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
            PRINTK_E("vipcore, register_chrdev failed ret=%d\n", result);
            return -1;
        }

        if (0 == major) {
            major = result;
        }

        kdriver->major = major;
        kdriver->registered = 1;

        /* Create the graphics class. */
        #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
        kdriver->class = class_create(CLASS_NAME);
        #else
        kdriver->class = class_create(THIS_MODULE, CLASS_NAME);
        #endif
        if (IS_ERR(kdriver->class)) {
            PRINTK_E("vipcore, class_create failed\n");
            return -1;
        }

        kdriver->class->devnode = vip_devnode;

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
    vipdrv_driver_t *kdriver
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
    vipdrv_driver_t *kdriver
    )
{
    vip_uint32_t i = 0, j = 0, core_cnt = 0, irq_index = 0;

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
    vipdrv_os_create_atomic(&kdriver->irq_count[core]);
    /* Initialize the wait queue. */
    init_waitqueue_head(&kdriver->irq_queue[core]);
    LOOP_CORE_END
#endif

    for (i = 0; i < vipdMAX_CORE; i++) {
        if (0 == kdriver->device_core_number[i]) {
            break;
        }

        for (j = 0; j < kdriver->device_core_number[i]; j++) {
            kdriver->irq_map[core_cnt++] = irq_index;
        #if !vpmdTASK_PARALLEL_ASYNC
            irq_index++;
        #endif
        }
    #if vpmdTASK_PARALLEL_ASYNC
        irq_index += kdriver->device_core_number[i];
    #endif
    }

    return 0;
}

static vip_int32_t drv_prepare_register(
    vipdrv_driver_t *kdriver
    )
{
    LOOP_CORE_START
    if (core >= vipdMAX_CORE) {
        break;
    }
    if (kdriver->vip_reg[core] != VIP_NULL) {
        PRINTK_I("core%d regiseter has mapped in platform code\n", core);
        continue;
    }

    /* Map the VIP registers. */
#if defined (USE_LINUX_PCIE_DEVICE)
    kdriver->vip_reg[core] = (void *)pci_iomap(kdriver->pdev, kdriver->pci_bars[core],
                                               kdriver->pci_bar_size[kdriver->pci_bars[core]]);
    kdriver->pci_vip_reg[core] = kdriver->vip_reg[core];
    PRINTK_D("core_%d bar=%d, map logical base=0x%"PRIx64"\n", core, kdriver->pci_bars[core], kdriver->vip_reg[core]);
    kdriver->vip_reg[core] = (vip_uint8_t*)kdriver->vip_reg[core] + kdriver->reg_offset[core];
#else
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

    #if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)
    kdriver->vip_reg[core] = ioremap(kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core]);
    #else
    kdriver->vip_reg[core] = ioremap_nocache(kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core]);
    #endif
}
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
    vipdrv_driver_t *kdriver
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
    vipdrv_debug_destroy_fs();
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
            release_mem_region(kdriver->vip_reg_phy[core], kdriver->vip_reg_size[core]);
        #endif
            kdriver->vip_reg[core] = VIP_NULL;
            kdriver->reg_drv_map[core] = 0;
        }
    }

#if !vpmdENABLE_POLLING
    if (kdriver->irq_registed[core]) {
        free_irq(kdriver->irq_line[core], (void*)(uintptr_t)(core+1));
        kdriver->irq_registed[core] = 0;
    }
    if (kdriver->irq_count[core] != VIP_NULL) {
        vipdrv_os_destroy_atomic(kdriver->irq_count[core]);
    }
#endif
    LOOP_CORE_END

    drv_unprepare_device(kdriver);

    vipdrv_os_zero_memory(kdriver, sizeof(kdriver));
}

static vip_int32_t drv_init(void)
{
    vip_int32_t ret = 0;

    if (kdriver == VIP_NULL) {
        PRINTK("vipcore, failed vip device object is null\n");
        return -1;
    }

    if (drv_check_params(kdriver) < 0) {
        PRINTK("vipcore, fail to check parameters\n");
        return -1;
    }

    ret = drv_prepare_device(kdriver);
    if (ret < 0) {
        PRINTK_E("fail to register device node");
        goto error;
    }

    ret = drv_prepare_irq(kdriver);
    if (ret < 0) {
        PRINTK_E("fail to register irq");
        goto error;
    }

    ret = drv_prepare_register(kdriver);
    if (ret < 0) {
        PRINTK_E("fail to prepare register");
        goto error;
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    ret = drv_prepare_video_memory(kdriver);
    if (ret < 0) {
        PRINTK_E("fail to prepare video memory\n");
        goto error;
    }
#else
    PRINTK("video memory heap is disabled\n");
#endif

#if (vpmdENABLE_DEBUG_LOG > 1)
    drv_show_params(kdriver);
#endif

#if vpmdENABLE_DEBUGFS
    ret = vipdrv_debug_create_fs();
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
 Get the video memory config information
 1. the size of system memory used by vip_hal.
 2. reserve_physical is a video memory, used by vip_create_network_from_physical() api in nbg_linker.
 3. heap info is used for video memory heap basic information.
*/
vip_status_e vipdrv_drv_get_memory_config(
    vipdrv_platform_memory_config_t *memory_info
    )
{
    if (VIP_NULL == memory_info) {
        PRINTK_E("fail to get video memory info param is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

#if vpmdENABLE_VIDEO_MEMORY_HEAP
    memory_info->heap_count = kdriver->vip_mem_cnt;
    memory_info->heap_infos = kdriver->heap_infos;
#endif

#if vpmdENABLE_RESERVE_PHYSICAL
    memory_info->reserve_phy_base = kdriver->reserve_physical_base;
    memory_info->reserve_phy_size = kdriver->reserve_physical_size;
#endif

#if vpmdENABLE_SYS_MEMORY_HEAP
    memory_info->sys_heap_size = kdriver->sys_heap_size;
#endif

    return VIP_SUCCESS;
}

/*
@brief Get all hardware basic information.
*/
vip_status_e vipdrv_drv_get_hardware_info(
    vipdrv_hardware_info_t *info
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
    info->irq_queue[core] = (void*)&kdriver->irq_queue[core];
    info->irq_count[core] = &kdriver->irq_count[core];
    info->irq_value[core] = (vip_uint32_t*)&kdriver->irq_value[core];
    LOOP_CORE_END

    for (iter = 0; iter < vipdMAX_CORE; iter++) {
        info->device_core_number[iter] = kdriver->device_core_number[iter];
        info->axi_sram_size[iter] = kdriver->axi_sram_size[iter];
    }

    return status;
}

/*
@brief Set power on/off and clock on/off
@param state, power status. refer to vipdrv_power_status_e.
*/
vip_status_e vipdrv_drv_set_power_clk(
    vip_uint32_t core,
    vip_uint32_t state
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_int32_t ret = 0;

    mutex_lock(&set_power_mutex);

    if (state == VIPDRV_POWER_ON) {
        if (state != kdriver->power_status[core]) {
            PRINTK_D("power on vip hardware%d.\n", core);
            ret = vipdrv_drv_device_init(kdriver, core);
            if (ret < 0) {
                PRINTK_E("vipcore, fail to init device, ret=%d\n", ret);
                status = VIP_ERROR_FAILURE;
            }
            kdriver->power_status[core] = VIPDRV_POWER_ON;
        }
    }
    else if (state == VIPDRV_POWER_OFF) {
        if (state != kdriver->power_status[core]) {
            PRINTK_D("power off vip hardware%d.\n", core);
            ret = vipdrv_drv_device_uninit(kdriver, core);
            if (ret < 0) {
                PRINTK_E("vipcore, fail to uninit device, ret=%d\n", ret);
                status = VIP_ERROR_FAILURE;
            }
            kdriver->power_status[core] = VIPDRV_POWER_OFF;
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
vip_status_e vipdrv_drv_init(
    vip_uint32_t video_memsize
    )
{
    vip_status_e status = VIP_SUCCESS;
    PRINTK_D("linux kernel version=0x%x\n", LINUX_VERSION_CODE);

    /* power on VIP */
    LOOP_CORE_START
    status = vipdrv_drv_set_power_clk(core, VIPDRV_POWER_ON);
    if (status != VIP_SUCCESS) {
        PRINTK("vipcore, fail to power on\n");
        goto exit;
    }
    LOOP_CORE_END

exit:
    return status;
}

/*
@brief do some un-initialize in this function.
*/
vip_status_e vipdrv_drv_exit(void)
{
    vip_status_e status = VIP_SUCCESS;

    LOOP_CORE_START
    status = vipdrv_drv_set_power_clk(core, VIPDRV_POWER_OFF);
    if (status != VIP_SUCCESS) {
        PRINTK("vipcore, fail to power off\n");
    }
    LOOP_CORE_END

    return status;
}

#if defined (USE_LINUX_PCIE_DEVICE)
int vipdrv_pcie_probe(
    struct pci_dev *pdev,
    const struct pci_device_id *ent
    )
{
    vip_int32_t ret = -ENODEV;
    vip_int32_t i = 0;
    vip_uint32_t total_core = 0;

    kdriver->device_cnt++;
    if (kdriver->device_cnt > 1) {
        PRINTK("only support one pcie device, detected cnt=%d\n", kdriver->device_cnt);
        return ret;
    }

    if (VIP_NULL == pdev) {
        PRINTK_E("fail pcie dev is NULL\n");
        return ret;
    }

    kdriver->pdev = pdev;
    kdriver->device = &pdev->dev;

    PRINTK("vipcore, pcie driver device=0x%"PRPx", pdev=%p\n", kdriver->device, kdriver->pdev);

    if (pci_enable_device(pdev)) {
        PRINTK(KERN_ERR "vipcore, pci_enable_device() failed.\n");
    }
    pci_set_master(pdev);

    if (pci_request_regions(pdev, "vipcore")) {
        PRINTK("vipcore, fail to get ownership of BAR region.\n");
    }
    /* adjust parameters from platform code */
    ret = vipdrv_drv_adjust_param(kdriver);
    if (ret < 0) {
        PRINTK("vipcore, fail to adjust parameters\n");
        return -1;
    }

    if (kdriver->core_count > vipdMAX_CORE) {
        PRINTK("vipcore, failed init, the max core=%d core count=%d\n",
                 vipdMAX_CORE, kdriver->core_count);
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
    kdriver->power_status[core] = VIPDRV_POWER_OFF;
    LOOP_CORE_END

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0)
    if (dma_set_mask(&pdev->dev, DMA_BIT_MASK(48))) {
        PRINTK(KERN_ERR "vipcore, fail to set DMA mask.\n");
    }
#else
    if (pci_set_dma_mask(pdev, DMA_BIT_MASK(48))) {
        PRINTK(KERN_ERR "vipcore, fail to set DMA mask.\n");
    }
#endif

    ret = drv_init();
    if (ret < 0) {
        PRINTK_E("vipcore, fail to drv init\n");
        return -1;
    }

#if vpmdREGISTER_PCIE_DRIVER
    pci_set_drvdata(pdev, kdriver);
#endif

    return ret;
}

void vipdrv_pcie_remove(
    struct pci_dev *pdev
    )
{
    drv_exit();

    vipdrv_drv_unadjust_param(kdriver);

    pci_set_drvdata(pdev, NULL);
    pci_clear_master(pdev);
    pci_release_regions(pdev);
    pci_disable_device(pdev);

    return;
}

static struct pci_driver vipdrv_pcie_driver = {
    .name = DEVICE_NAME,
    .probe = vipdrv_pcie_probe,
    .remove = vipdrv_pcie_remove
};

static int __init module_drv_init(void)
{
    int ret = 0;

    /* Create device structure. */
    kdriver = kmalloc(sizeof(vipdrv_driver_t), GFP_KERNEL);
    if (kdriver == NULL) {
        PRINTK("vipcore, kmalloc failed for driver device.\n");
        return -1;
    }
    PRINTK("vipcore, pcie driver init\n");

    /* Zero structure. */
    memset(kdriver, 0, sizeof(vipdrv_driver_t));
    kdriver->pdrv = &vipdrv_pcie_driver;
    kdriver->log_level = DEFAULT_ENABLE_LOG;
    kdriver->func_config.enable_capture = DEFAULT_ENABLE_CAPTURE;
    kdriver->func_config.enable_cnn_profile = DEFAULT_ENABLE_CNN_PROFILE;
    kdriver->func_config.enable_dump_nbg = DEFAULT_ENABLE_DUMP_NBG;

    /* initialize parameters */
    drv_init_params(kdriver);

    vipdrv_drv_platform_init(kdriver);

#if vpmdREGISTER_PCIE_DRIVER
    ret = pci_register_driver(kdriver->pdrv);
#else
    PRINTK("not register pcie driver, used probed device\n");
    ret = vipdrv_pcie_probe(kdriver->pdev, NULL);
#endif
    if (ret < 0) {
        PRINTK(KERN_ERR "vipcore, vip drv init() fail to register driver!\n");
        return ret;
    }

    return 0;
}

static void __exit module_drv_exit(void)
{
#if vpmdREGISTER_PCIE_DRIVER
    pci_unregister_driver(&vipdrv_pcie_driver);
#else
    vipdrv_pcie_remove(kdriver->pdev);
#endif

    vipdrv_drv_platform_uninit(kdriver);

    /* Free up the device structure. */
    kfree(kdriver);
    kdriver = VIP_NULL;
    PRINTK("vipcore, pcie driver exit\n");
}

#elif defined (USE_LINUX_PLATFORM_DEVICE)
/* Linux platform device driver */
static vip_int32_t vipdrv_platform_probe(
    struct platform_device *pdev
    )
{
    vip_int32_t ret = 0;
    vip_uint32_t i = 0;
    vip_uint32_t total_core = 0;

    kdriver->pdev = pdev;
    kdriver->device = &pdev->dev;

    PRINTK("vipcore, platform driver device=0x%"PRPx"\n", kdriver->device);
    /* adjust parameters from platform code */
    ret = vipdrv_drv_adjust_param(kdriver);
    if (ret < 0) {
        PRINTK_E("vipcore, fail to adjust parameters\n");
        return -1;
    }

    if (kdriver->core_count > vipdMAX_CORE) {
        PRINTK_E("vipcore, failed init, the max core=%d core count=%d\n",
                 vipdMAX_CORE, kdriver->core_count);
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
    kdriver->power_status[core] = VIPDRV_POWER_OFF;
    LOOP_CORE_END

    ret = dma_set_mask(kdriver->device, DMA_BIT_MASK(40));
    if (ret < 0) {
        PRINTK_E("vipcore, fail to set dma mask\n");
    }
    ret = drv_init();
    if (ret < 0) {
        PRINTK_E("vipcore, fail to drv init\n");
        return -1;
    }

    return ret;
}

static vip_int32_t vipdrv_platform_remove(
    struct platform_device *pdev
    )
{
    drv_exit();

    vipdrv_drv_unadjust_param(kdriver);

    return 0;
}

static void vipdrv_platform_shutdown(struct platform_device *pdev)
{
	vipdrv_platform_remove(pdev);
}

static vip_int32_t vipdrv_platform_suspend(
    struct platform_device *pdev,
    pm_message_t state
    )
{
#if vpmdENABLE_SUSPEND_RESUME
    PRINTK_I("system suspend...\n");
    vipdrv_pm_suspend();
#endif

    return 0;
}

static vip_int32_t vipdrv_platform_resume(
    struct platform_device *pdev
    )
{
    vip_int32_t ret = 0;
#if vpmdENABLE_SUSPEND_RESUME
    vip_status_e status = VIP_SUCCESS;
    PRINTK_I("system resume...\n");
    status = vipdrv_pm_resume();
    if (status != VIP_SUCCESS) {
        PRINTK("vipcore, fail to do pm init hardware status=%d\n", status);
        ret = -1;
        goto end;
    }
end:
#endif
    return ret;
}

#if defined(CONFIG_PM) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
#ifdef CONFIG_PM_SLEEP
static int vipdrv_system_suspend(
	struct device *dev
	)
{
	pm_message_t state = { 0 };
	return vipdrv_platform_suspend(to_platform_device(dev), state);
}

static int vipdrv_system_resume(
	struct device *dev
	)
{
	return vipdrv_platform_resume(to_platform_device(dev));
}

static const struct dev_pm_ops vipdrv_dev_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(vipdrv_system_suspend, vipdrv_system_resume)
};
#endif
#endif

static struct platform_driver vipdrv_platform_driver = {
    .probe      = vipdrv_platform_probe,
    .remove     = vipdrv_platform_remove,
    .suspend    = vipdrv_platform_suspend,
    .resume     = vipdrv_platform_resume,
	.shutdown   = vipdrv_platform_shutdown,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = DEVICE_NAME,
#if defined(CONFIG_PM) && LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 30)
	#ifdef CONFIG_PM_SLEEP
		.pm = &vipdrv_dev_pm_ops,
	#endif
#endif
    }
};

static vip_int32_t __init module_drv_init(void)
{
    vip_int32_t ret = 0;

    /* Create device structure. */
    kdriver = kmalloc(sizeof(vipdrv_driver_t), GFP_KERNEL);
    if (kdriver == NULL) {
        PRINTK("vipcore, kmalloc failed for driver device.\n");
        return -ENODEV;
    }
    PRINTK("vipcore, platform driver init\n");

    /* Zero structure. */
    memset(kdriver, 0, sizeof(vipdrv_driver_t));
    kdriver->pdrv = &vipdrv_platform_driver;
    kdriver->log_level = DEFAULT_ENABLE_LOG;
    kdriver->func_config.enable_capture = DEFAULT_ENABLE_CAPTURE;
    kdriver->func_config.enable_cnn_profile = DEFAULT_ENABLE_CNN_PROFILE;
    kdriver->func_config.enable_dump_nbg = DEFAULT_ENABLE_DUMP_NBG;

    /* initialize parameters */
    drv_init_params(kdriver);

    /* get the pdrv->driver.of_match_table for match with platform device in DTS */
    ret = vipdrv_drv_platform_init(kdriver);
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

    ret = vipdrv_drv_platform_uninit(kdriver);
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
    kdriver = kmalloc(sizeof(vipdrv_driver_t), GFP_KERNEL);
    if (kdriver == NULL) {
        PRINTK("vipcore, kmalloc failed for driver device.\n");
        return -ENODEV;
    }

    PRINTK("vipcore, not on any bus to initialize driver\n");
    /* Zero structure. */
    memset(kdriver, 0, sizeof(vipdrv_driver_t));
    kdriver->log_level = DEFAULT_ENABLE_LOG;
    kdriver->func_config.enable_capture = DEFAULT_ENABLE_CAPTURE;
    kdriver->func_config.enable_cnn_profile = DEFAULT_ENABLE_CNN_PROFILE;
    kdriver->func_config.enable_dump_nbg = DEFAULT_ENABLE_DUMP_NBG;

    /* initialize parameters */
    drv_init_params(kdriver);

    /* adjust parameters from platform code */
    ret = vipdrv_drv_adjust_param(kdriver);
    if (ret < 0) {
        PRINTK("vipcore, fail to adjust parameters\n");
        goto exit;
    }

    if (0 == kdriver->core_count) {
        kdriver->core_count = 1;
    }
    if (0 == kdriver->device_core_number[0]) {
        kdriver->device_core_number[0] = 1;
    }

    LOOP_CORE_START
    kdriver->power_status[core] = VIPDRV_POWER_OFF;
    LOOP_CORE_END

    ret = drv_init();
    if (ret < 0) {
        PRINTK("vipcore, fail to drv init\n");
        goto exit;
    }

    return 0;

exit:
    return -1;
}

static void __exit module_drv_exit(void)
{
    drv_exit();

    vipdrv_drv_unadjust_param(kdriver);

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

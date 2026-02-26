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

#include "gc_vip_kernel_os_debug.h"
#include "gc_vip_kernel_port.h"
#include "gc_vip_kernel.h"
#include "gc_vip_kernel_drv.h"
#include "gc_vip_kernel_heap.h"
#include "gc_vip_hardware.h"
#include "gc_vip_kernel_debug.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,16,0)
#include <linux/stdarg.h>
#else
#include <stdarg.h>
#endif
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/mm.h>
#include <linux/spinlock.h>
#include <asm/atomic.h>
#include <linux/vmalloc.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0)
#include <linux/sched/task.h>
#else
#include <linux/sched.h>
#endif

#if (LINUX_VERSION_CODE > KERNEL_VERSION (4,20,17) && !defined(CONFIG_ARCH_NO_SG_CHAIN)) ||   \
    (LINUX_VERSION_CODE >= KERNEL_VERSION (3,6,0)       \
    && (defined(ARCH_HAS_SG_CHAIN) || defined(CONFIG_ARCH_HAS_SG_CHAIN)))
#define gcdUSE_Linux_SG_TABLE_API   1
#else
#define gcdUSE_Linux_SG_TABLE_API   0
#endif

#if (vpmdENABLE_VIDEO_MEMORY_CACHE) && (!gcdUSE_Linux_SG_TABLE_API)
#ifndef USE_LINUX_RESERVE_MEM
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,23)
struct sg_table
{
    struct scatterlist *sgl;
    unsigned int nents;
    unsigned int orig_nents;
};

static inline void sg_set_page(
    struct scatterlist *sg,
    struct page *page,
    unsigned int len,
    unsigned int offset
    )
{
    sg->page = page;
    sg->offset = offset;
    sg->length = len;
}

static inline void sg_mark_end(
    struct scatterlist *sg
    )
{
    (void)sg;
}
#endif

static vip_int32_t gckvip_sg_alloc_table_from_pages(
    struct scatterlist **sgl,
    struct page **pages,
    unsigned int  n_pages,
    unsigned long offset,
    unsigned long size,
    unsigned int  *nents
    )
{
    unsigned int chunks = 0;
    unsigned int i = 0;
    unsigned int cur_page = 0;
    struct scatterlist *s = VIP_NULL;

    chunks = 1;
    PRINTK_D("start do vsi sg alloc table from pages.\n");

    for (i = 1; i < n_pages; ++i) {
        if (page_to_pfn(pages[i]) != page_to_pfn(pages[i - 1]) + 1) {
            ++chunks;
        }
    }

    gckvip_os_allocate_memory(sizeof(struct scatterlist) * chunks, (void**)&s);
    if (unlikely(!s)) {
        PRINTK_E("failed to allocate memory in table_from_pages\n");
        return -ENOMEM;
    }

    *sgl = s;
    *nents = chunks;
    cur_page = 0;

    for (i = 0; i < chunks; i++, s++) {
        unsigned long chunk_size = 0;
        unsigned int j = 0;

        for (j = cur_page + 1; j < n_pages; j++) {
            if (page_to_pfn(pages[j]) != page_to_pfn(pages[j - 1]) + 1) {
                break;
            }
        }

        chunk_size = ((j - cur_page) << PAGE_SHIFT) - offset;
        sg_set_page(s, pages[cur_page], min(size, chunk_size), offset);
        size -= chunk_size;
        offset = 0;
        cur_page = j;
    }

    sg_mark_end(s - 1);

    return 0;
}
#endif
#endif

typedef struct _gckvip_signal_data
{
    wait_queue_head_t wait_queue;
    volatile vip_bool_e wait_cond;
    volatile vip_bool_e value;
    spinlock_t lock;
} gckvip_signal_data_t;


#if vpmdENABLE_MEMORY_PROFILING
#define INIT_CAP_NUM     128
typedef struct _gckvip_mem_profile_table_t
{
    vip_ptr *memory;
    vip_uint32_t size;
} gckvip_mem_profile_table_t;

typedef struct _gckvip_mem_profile_db_t
{
    gckvip_mem_profile_table_t *table;
    vip_uint32_t total_count;
    vip_uint32_t used_count;
    vip_uint32_t current_index;
    vip_uint32_t kmem_alloc_cnt;
    vip_uint32_t kmem_free_cnt;
    vip_uint32_t kmem_peak;
    vip_uint32_t kmem_current;
} gckvip_mem_profile_db_t;

gckvip_mem_profile_db_t database = {0};
#endif

/*
@brief Do some initialization works in KERNEL_OS layer.
@param video_mem_size, the size of video memory heap.
*/
vip_status_e gckvip_os_init(
    vip_uint32_t video_mem_size
    )
{
    vip_status_e status = VIP_SUCCESS;
    /* workround solution for dyn enable and disable capture and cnn profile*/
    gckvip_context_t *context = gckvip_get_context();
    gckvip_driver_t  *kdriver = gckvip_get_kdriver();
    context->func_config.enable_capture = kdriver->func_config.enable_capture;
    context->func_config.enable_cnn_profile = kdriver->func_config.enable_cnn_profile;
    context->func_config.enable_dump_nbg = kdriver->func_config.enable_dump_nbg;

    status = gckvip_drv_init(video_mem_size);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to drv init\n");
    }

#if vpmdENABLE_DEBUGFS
	status = gckvip_debug_profile_start();
	if (status != VIP_SUCCESS) {
	    PRINTK_E("fail debug profile start\n");
	}
#endif

    return status;
}

/*
@brief Do some un-initialization works in KERNEL_OS layer.
*/
vip_status_e gckvip_os_close(void)
{
    vip_status_e status = VIP_SUCCESS;

#if vpmdENABLE_DEBUGFS
	gckvip_debug_profile_end();
#endif

    status = gckvip_drv_exit();
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to drv exit\n");
    }

#if vpmdENABLE_MEMORY_PROFILING
#if vpmdENABLE_MULTIPLE_TASK
    /* workaround, free context->initialize_mutex after calling gckvip_os_close */
    if (database.kmem_current >= 40) {
        database.kmem_current -= 40;
    }
    if (database.kmem_free_cnt >= 1) {
        database.kmem_free_cnt += 1;
    }
#endif
    PRINTK("========== Kernel Memory Statistics ========\n");
    PRINTK("Kernel Memory Alloc Count   : %d times.\n", database.kmem_alloc_cnt);
    PRINTK("Kernel Memory Free Count    : %d times.\n", database.kmem_free_cnt);
    PRINTK("Kernel Memory Current Value : %d bytes.\n", database.kmem_current);
    PRINTK("Kernel Memory Peak Value    : %d bytes.\n", database.kmem_peak);
    PRINTK("================   End  ====================\n");

    if (database.kmem_current != 0) {
        vip_uint32_t i = 0;
        PRINTK("Kernel Memory Not Free Ptr List:\n");
        for (i = 0; i < database.total_count; i++) {
            if (VIP_NULL != database.table[i].memory) {
                PRINTK("Ptr=0x%"PRPx", Size=%dbytes.\n", database.table[i].memory, database.table[i].size);
            }
        }
    }

    if (database.table != VIP_NULL) {
        kfree(database.table);
        database.table = VIP_NULL;
    }
    gckvip_os_zero_memory(&database, sizeof(gckvip_mem_profile_db_t));
#endif

    return status;
}


/************************************************************************/
/************************************************************************/
/* *********************need to be ported functions**********************/
/************************************************************************/
/************************************************************************/
/************************************************************************/
/*
@brief Allocate system memory in kernel space.
@param video_mem_size, the size of video memory heap.
@param size, Memory size to be allocated.
@param memory, Pointer to a variable that will hold the pointer to the memory.
*/
vip_status_e gckvip_os_allocate_memory(
    IN  vip_uint32_t size,
    OUT void **memory
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (size > 0) {
        *memory = kmalloc(size, GFP_KERNEL);
        if (*memory == VIP_NULL) {
            *memory = vmalloc(size);
            if (*memory == VIP_NULL) {
                status = VIP_ERROR_OUT_OF_MEMORY;
            }
        }
    }
    else {
        status = VIP_ERROR_INVALID_ARGUMENTS;
    }

#if vpmdENABLE_MEMORY_PROFILING
    if (database.used_count >= database.total_count) {
        /* expansion */
        gckvip_mem_profile_table_t *table = VIP_NULL;
        table = (gckvip_mem_profile_table_t*)kmalloc(\
            sizeof(gckvip_mem_profile_table_t) * (database.total_count + INIT_CAP_NUM), GFP_KERNEL);
        gckvip_os_zero_memory(table, sizeof(gckvip_mem_profile_table_t) * (database.total_count + INIT_CAP_NUM));
        if (VIP_NULL != database.table) {
            gckvip_os_memcopy(table, database.table, sizeof(gckvip_mem_profile_table_t) * database.total_count);
            kfree(database.table);
        }

        database.total_count += INIT_CAP_NUM;
        database.table = table;
    }

    while (1) {
        database.current_index %= database.total_count;
        if (VIP_NULL == database.table[database.current_index].memory) {
            database.table[database.current_index].memory = *memory;
            database.table[database.current_index].size = size;
            break;
        }
        database.current_index++;
    }

    database.kmem_alloc_cnt++;
    database.kmem_current += size;
    database.used_count++;
    if (database.kmem_current > database.kmem_peak) {
        database.kmem_peak = database.kmem_current;
    }
#endif

    return status;
}

/*
@brief Free system memory in kernel space.
@param memory, Memory to be freed.
*/
vip_status_e gckvip_os_free_memory(
    IN void *memory
    )
{
    unsigned long address = GCKVIPPTR_TO_UINT64(memory);
#if vpmdENABLE_MEMORY_PROFILING
    {
    vip_uint32_t i = 0;
    vip_uint32_t size = 0;
    for (i = 0; i < database.total_count; i++) {
        if (memory == database.table[i].memory) {
            database.table[i].memory = VIP_NULL;
            size = database.table[i].size;
            database.table[i].size = 0;
            break;
        }
    }
    if (i >= database.total_count) {
        PRINTK_D("fail to get memory size in free memory\n");
    }
    database.kmem_free_cnt++;
    database.used_count--;
    database.kmem_current -= size;
    }
#endif

    if (memory != VIP_NULL) {
        if (unlikely(address >= VMALLOC_START && address <= VMALLOC_END)) {
            vfree(memory);
        }
        else {
            kfree(memory);
        }
    }
    else {
        PRINTK_E("failed to free memory is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    return VIP_SUCCESS;
}

/*
@brief Set memory to zero.
@param memory, Memory to be set to zero.
@param size, the size of memory.
*/
vip_status_e gckvip_os_zero_memory(
    IN void *memory,
    IN vip_uint32_t size
    )
{
    vip_uint8_t *data = (vip_uint8_t*)memory;

    if (!memory) {
        PRINTK_E("failed to memset memory, size: %d bytes\n", size);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    #if 0
    {
    vip_uint32_t i = 0;
    for (i = 0; i < size; i++) {
        data[i] = 0;
    }
    }
    #else
    memset(data, 0, size);
    #endif

    return VIP_SUCCESS;
}

/*
@brief copy src memory to dst memory.
@param dst, Destination memory.
@param src. Source memory
@param size, the size of memory should be copy.
*/
vip_status_e gckvip_os_memcopy(
    IN void *dst,
    IN const void *src,
    IN vip_uint32_t size
    )
{
    if ((VIP_NULL == src) || (VIP_NULL == dst))  {
        PRINTK_E("failed to copy memory, parameter is NULL, size: %d bytes\n", size);
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    memcpy(dst, src, size);

    return VIP_SUCCESS;
}

/*
@brief Read a hardware register.
@param reg_base, the base address of hardware register.
@param address, read the register address.
@param data, the data of address be read.
*/
vip_status_e gckvip_os_read_reg(
    IN volatile void *reg_base,
    IN vip_uint32_t address,
    OUT vip_uint32_t *data
    )
{
    *data = readl((vip_uint8_t *)reg_base + address);

#if vpmdENABLE_CAPTURE_IN_KERNEL
    {
    gckvip_hardware_info_t info = {0};
    vip_uint32_t i = 0;
    gckvip_drv_get_hardware_info(&info);
    for (i = 0; i < info.core_count; i++) {
        if (reg_base == info.vip_reg[i]) {
            break;
        }
    }
    PRINTK("@[register.read %u 0x%05X 0x%08X]\n", i, address, *data);
    }
#endif

    return VIP_SUCCESS;
}

/*
@brief Write a hardware register.
@param reg_base, the base address of hardware register.
@param address, write the register address.
@param data, the data of address be wrote.
*/
vip_status_e  gckvip_os_write_reg(
    IN volatile void *reg_base,
    IN vip_uint32_t address,
    IN vip_uint32_t data
    )
{
    writel(data, (vip_uint8_t *)reg_base + address);

#if vpmdENABLE_CAPTURE_IN_KERNEL
    {
    gckvip_hardware_info_t info = {0};
    vip_uint32_t i = 0;
    gckvip_drv_get_hardware_info(&info);
    for (i = 0; i < info.core_count; i++) {
        if (reg_base == info.vip_reg[i]) {
            break;
        }
    }
    PRINTK("@[register.write %u 0x%05X 0x%08X]\n", i, address, data);
    }
#endif

    return VIP_SUCCESS;
}

/*
@brief Waiting for the hardware interrupt
@param irq_queue, the wait queue of interrupt handle.
@param irq_value, a flag for interrupt.
@param time_out, the time out of this wait. xxxms.
@param mask, mask data for interrupt.
*/
vip_status_e gckvip_os_wait_interrupt(
    IN void *irq_queue,
    IN volatile vip_uint32_t* volatile irq_value,
    IN vip_uint32_t time_out,
    IN vip_uint32_t mask
    )
{
    vip_status_e status = VIP_SUCCESS;
    wait_queue_head_t *wait_queue = (wait_queue_head_t *)irq_queue;
    gcIsNULL(irq_value);

    if (wait_queue != VIP_NULL) {
        unsigned long jiffies = 0;
        int result = 0;

        if (vpmdINFINITE == time_out) {
            jiffies = MAX_SCHEDULE_TIMEOUT;
        }
        else {
            jiffies = msecs_to_jiffies(time_out);
        }

    wait_irq:
        result = wait_event_interruptible_timeout(*wait_queue, *irq_value & mask, jiffies);
        if (0 == result) {
            dump_stack();
            PRINTK_E("failed to wait interrupt, timeout....\n");
            gcGoOnError(VIP_ERROR_TIMEOUT);
        }
        else if (result < 0) {
            PRINTK_E("wait irq event wake up by signal, result=%d\n", result);
            goto wait_irq;
        }
    }
    else {
        status = VIP_ERROR_INVALID_ARGUMENTS;
        PRINTK_E("irq_queue is NULL in os_wait_interrupt\n");
    }

onError:
    return status;
}

vip_status_e gckvip_os_wakeup_interrupt(
    IN void *irq_queue
    )
{
    vip_status_e status = VIP_SUCCESS;

    wait_queue_head_t *wait_queue = (wait_queue_head_t *)irq_queue;

    wake_up_interruptible(wait_queue);

    return status;
}

/*
@brief Delay execution of the current thread for a number of milliseconds.
@param ms, delay unit ms. Delay to sleep, specified in milliseconds.
*/
void gckvip_os_delay(
    IN vip_uint32_t ms
    )
{
    if (ms > 0)
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
        ktime_t delay = ktime_set((ms / MSEC_PER_SEC), (ms % MSEC_PER_SEC) * NSEC_PER_MSEC);
        __set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_hrtimeout(&delay, HRTIMER_MODE_REL);
#else
        msleep(ms);
#endif
    }
}

/*
@brief Delay execution of the current thread for a number of microseconds.
@param ms, delay unit us. Delay to sleep, specified in microseconds.
*/
void gckvip_os_udelay(
    IN vip_uint32_t us
    )
{
    if (us > 0) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
        #if 0
        ktime_t delay = ktime_set((us / USEC_PER_SEC), (us % USEC_PER_SEC) * NSEC_PER_USEC);
        __set_current_state(TASK_UNINTERRUPTIBLE);
        schedule_hrtimeout(&delay, HRTIMER_MODE_REL);
        #endif
        udelay(us);
#else
        usleep_range((unsigned long)us, (unsigned long)us + 1);
#endif
    }
}

/*
@brief Print string on console.
@param msg, which message to be print.
*/
vip_status_e gckvip_os_print(
    gckvip_log_level level,
    IN const char *msg,
    ...
    )
{
    vip_status_e status = VIP_SUCCESS;
    char strbuf[256] = {'\0'};
    va_list varg;
#if (vpmdENABLE_DEBUG_LOG > 2)
    gckvip_driver_t *driver = VIP_NULL;
    driver = gckvip_get_kdriver();
    if (((level == GCKVIP_DEBUG) || (level == GCKVIP_INFO)) && (0 == driver->log_level)) {
        return VIP_SUCCESS;
    }
#endif

    va_start (varg, msg);
    vsnprintf(strbuf, 256, msg, varg);
    va_end(varg);

#if (vpmdENABLE_DEBUG_LOG >= 4)
    printk(KERN_EMERG "npu[%x][%x] %s", gckvip_os_get_pid(), gckvip_os_get_tid(), strbuf);
#else
    printk("npu[%x][%x] %s", gckvip_os_get_pid(), gckvip_os_get_tid(), strbuf);
#endif

    return status;
}

/*
@brief Print string to buffer.
@param, size, the size of msg.
@param msg, which message to be print.
*/
vip_status_e gckvip_os_snprint(
    IN vip_char_t *buffer,
    IN vip_uint32_t size,
    IN const vip_char_t *msg,
    ...
    )
{
    vip_status_e status = VIP_SUCCESS;
    va_list varg;
    va_start (varg, msg);
    vsnprintf(buffer, size, msg, varg);
    va_end(varg);

    return status;
}

/*
@brief get the current system time. return value unit us, microsecond
*/
vip_uint64_t  gckvip_os_get_time(void)
{
    vip_uint64_t time = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
    struct timespec64 tv;
    /* Return the time of day in microseconds. */
    ktime_get_ts64(&tv);
    time = (vip_uint64_t)((tv.tv_sec * 1000000ULL) + (tv.tv_nsec / 1000));
#else
    struct timeval tv;
    do_gettimeofday(&tv);
    time = (vip_uint64_t)((tv.tv_sec * 1000000ULL) + tv.tv_usec);
#endif

    return time;
}

/*
@brief Memory barrier function for memory consistency.
*/
vip_status_e gckvip_os_memorybarrier(void)
{
    mb();
    return VIP_SUCCESS;
}

/*
@brief Get kernel space process id
*/
vip_uint32_t gckvip_os_get_pid(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
    return (vip_uint32_t)task_tgid_vnr(current);
#else
    return (vip_uint32_t)current->tgid;
#endif
}

/*
@brief Get kernel space thread id
*/
vip_uint32_t gckvip_os_get_tid(void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
        return task_pid_vnr(current);
#else
        return current->pid;
#endif
}

#if vpmdENABLE_FLUSH_CPU_CACHE
/*
@brief 1.Flush CPU cache for video memory which allocated from heap.
       2.Flush external cache device if support.
@param physical, VIP physical address.
        gckvip_drv_get_cpuphysical to convert the physical address should be flush CPU cache.
@param logical, the logical address should be flush.
@param size, the size of the memory should be flush.
@param type The type of operate cache. see gckvip_cache_type_e.
*/
/*
@param physical, VIP physical address. May need to covert to CPU physcial address.
        gckvip_drv_get_cpuphysical to convert the physical address should be flush CPU cache.
@param logical, the logical address should be flush.
@param size, the size of the memory should be flush.
@param type The type of operate cache.
*/
vip_status_e gckvip_os_flush_cache(
    IN phy_address_t physical,
    IN void *logical,
    IN vip_uint32_t size,
    IN vip_uint8_t type
    )
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdENABLE_VIDEO_MEMORY_CACHE
    #if defined (USE_LINUX_RESERVE_MEM)
    struct page *page;
    dma_addr_t *dma_addr = VIP_NULL;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    phy_address_t cpu_physical = gckvip_drv_get_cpuphysical(physical);
    vip_uint32_t offset = cpu_physical - (cpu_physical & PAGE_MASK);
    vip_uint32_t num_pages = GetPageCount(PAGE_ALIGN(size));
    unsigned long pfn = __phys_to_pfn(cpu_physical);
    page = pfn_to_page(pfn);

    gckvip_os_allocate_memory(sizeof(dma_addr_t), (void**)&dma_addr);
    if (VIP_NULL == dma_addr) {
        PRINTK_E("fail to flush cache for video memory.\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }
    *dma_addr = dma_map_page(kdriver->device, page, offset,
            num_pages * PAGE_SIZE, DMA_BIDIRECTIONAL);

    if (dma_mapping_error(kdriver->device, *dma_addr)) {
        PRINTK_E("dma map page failed.\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }
    switch (type) {
        case GCKVIP_CACHE_CLEAN:
            {
                dma_sync_single_for_device(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_TO_DEVICE);
            }
            break;
        case GCKVIP_CACHE_FLUSH:
            {
                dma_sync_single_for_device(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_TO_DEVICE);
                dma_sync_single_for_cpu(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
            }
            break;
        case GCKVIP_CACHE_INVALID:
            {
                dma_sync_single_for_cpu(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
            }
            break;
        default:
            PRINTK_E("not support this flush cache type=%d\n", type);
            break;
    }
    dma_unmap_page(kdriver->device, *dma_addr, num_pages << PAGE_SHIFT, DMA_FROM_DEVICE);
    gckvip_os_free_memory(dma_addr);
    #else
    struct sg_table *sgt = VIP_NULL;
    struct page *page;
    struct page **pages;
    unsigned long long memory = (unsigned long long)(gckvip_uintptr_t)logical;
    vip_uint32_t offset = memory - (memory & PAGE_MASK);
    vip_int32_t result = 0, flush_size = 0, i = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    vip_uint32_t num_pages = GetPageCount(PAGE_ALIGN(size));
    phy_address_t cpu_physical = gckvip_drv_get_cpuphysical(physical);
    unsigned long pfn = __phys_to_pfn(cpu_physical);

    if (!pfn_valid(pfn)) {
        PRINTK("vipcore, unable to convert %pa to pfn\n", &cpu_physical);
        goto onError;
    }
    page = pfn_to_page(pfn);
    gckvip_os_allocate_memory(sizeof(struct page *) * num_pages, (void**)&pages);
    if (!pages) {
        PRINTK("vipcore, fail to malloc for pages\n");
        return -1;
    }
    for (i = 0; i < num_pages; i++) {
        pages[i] = nth_page(page, i);
    }

    flush_size = GCVIP_ALIGN(size, cache_line_size());

    if (size & (cache_line_size() - 1)) {
        PRINTK_W("flush cache warning memory size 0x%x not align to cache line size 0x%x\n",
                   size, cache_line_size());
    }
    if (VIP_NULL == kdriver->device) {
        PRINTK_E("failed to flush cache init, device is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    gckvip_os_allocate_memory(sizeof(struct sg_table), (void**)&sgt);
    if (!sgt) {
        PRINTK_E("%s[%d], failed to alloca memory for flush cache handle\n",
                 __FUNCTION__, __LINE__);
        gcGoOnError(VIP_ERROR_IO);
    }

#if gcdUSE_Linux_SG_TABLE_API
    result = sg_alloc_table_from_pages(sgt, pages, num_pages, offset,
                                       flush_size, GFP_KERNEL | __GFP_NOWARN);
#else
    result = gckvip_sg_alloc_table_from_pages(&sgt->sgl, pages, num_pages, offset,
                                             flush_size, &sgt->nents);
    sgt->orig_nents = sgt->nents;
#endif
    if (unlikely(result < 0)) {
        PRINTK_E("%s[%d], sg alloc table from pages failed\n", __FUNCTION__, __LINE__);
        gcGoOnError(VIP_ERROR_IO);
    }

    result = dma_map_sg(kdriver->device, sgt->sgl, sgt->nents, DMA_BIDIRECTIONAL);
    if (unlikely(result != sgt->nents)) {
        PRINTK_E("%s[%d], dma_map_sg failed, result=%d, exp=%d\n",
                  __FUNCTION__, __LINE__, result, sgt->nents);
#if gcdUSE_Linux_SG_TABLE_API
        sg_free_table(sgt);
#else
        gckvip_os_free_memory(sgt->sgl);
#endif
        sgt = VIP_NULL;
        gcGoOnError(VIP_ERROR_IO);
    }
    switch (type) {
        case GCKVIP_CACHE_CLEAN:
            {
                dma_sync_sg_for_device(kdriver->device, sgt->sgl, sgt->nents, DMA_TO_DEVICE);
            }
            break;
        case GCKVIP_CACHE_FLUSH:
            {
                dma_sync_sg_for_device(kdriver->device, sgt->sgl, sgt->nents, DMA_TO_DEVICE);
                dma_sync_sg_for_cpu(kdriver->device, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);
            }
            break;
        case GCKVIP_CACHE_INVALID:
            {
                dma_sync_sg_for_cpu(kdriver->device, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);
            }
            break;
        default:
            PRINTK_E("not support this flush cache type=%d\n", type);
            break;
    }

    dma_unmap_sg(kdriver->device, sgt->sgl, sgt->nents, DMA_FROM_DEVICE);

    #if gcdUSE_Linux_SG_TABLE_API
    sg_free_table(sgt);
    #else
    gckvip_os_free_memory(sgt->sgl);
    #endif
    gckvip_os_free_memory(sgt);

    if (pages != NULL) {
        gckvip_os_free_memory(pages);
    }
    #endif

onError:
#endif
    return status;
}
#endif

/*
@brief Create a mutex for multiple thread.
@param mutex, create mutex pointer.
*/
vip_status_e gckvip_os_create_mutex(
    OUT gckvip_mutex *mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct mutex *c_mutex = VIP_NULL;

    status = gckvip_os_allocate_memory(sizeof(struct mutex), (void**)&c_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate memory for mutex\n");
        return VIP_ERROR_IO;
    }

    mutex_init(c_mutex);
    if (VIP_NULL == mutex) {
        PRINTK_E("fail to create mutex, parameter is NULL\n");
        return VIP_ERROR_INVALID_ARGUMENTS;
    }
    else {
        *mutex = (gckvip_mutex)c_mutex;
    }

    return status;
}

vip_status_e gckvip_os_lock_mutex(
    IN gckvip_mutex mutex
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (mutex != VIP_NULL) {
        mutex_lock((struct mutex *)mutex);
    }
    else {
        PRINTK_E("fail to lock mutex, parameter is NULL\n");
        status = VIP_ERROR_INVALID_ARGUMENTS;
        GCKVIP_DUMP_STACK();
    }

    return status;
}

vip_status_e gckvip_os_unlock_mutex(
    IN gckvip_mutex mutex
    )
{
    vip_status_e status = VIP_SUCCESS;

    if (mutex != VIP_NULL) {
        mutex_unlock((struct mutex *)mutex);
    }
    else {
        PRINTK_E("fail to unlock mutex, parameter is NULL\n");
        status = VIP_ERROR_INVALID_ARGUMENTS;
        GCKVIP_DUMP_STACK();
    }

    return status;
}

/*
@brief Destroy a mutex which create in gckvip_os_create_mutex..
@param mutex, create mutex pointer.
*/
vip_status_e gckvip_os_destroy_mutex(
    IN gckvip_mutex mutex
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (mutex != VIP_NULL) {
        mutex_destroy((struct mutex *)mutex);
        gckvip_os_free_memory(mutex);
    }
    else {
        PRINTK_E("fail to unlock mutex, parameter is NULL\n");
        status = VIP_ERROR_INVALID_ARGUMENTS;
        GCKVIP_DUMP_STACK();
    }

    return status;
}

#if vpmdENABLE_MULTIPLE_TASK || vpmdPOWER_OFF_TIMEOUT
/*
@brief Create a thread.
@param func, the thread handle function.
@param parm, parameter for this thread.
@param name, the thread name.
@param handle, the handle for thread.
*/
vip_status_e gckvip_os_create_thread(
    IN gckvip_thread_func func,
    IN vip_ptr parm,
    IN vip_char_t *name,
    OUT gckvip_thread *handle
    )
{
    if ((handle != VIP_NULL) && (func != VIP_NULL)) {
        struct task_struct *task;
        task = kthread_create(func, parm,  "vip_%s", name);
        if (IS_ERR(task)) {
            PRINTK_E("failed to create thread.\n");
            return VIP_ERROR_IO;
        }
        get_task_struct(task);
        wake_up_process(task);
        *handle = (gckvip_thread)task;
    }
    else {
        PRINTK_E("failed to create thread. parameter is NULL\n");
        GCKVIP_DUMP_STACK();
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    return VIP_SUCCESS;
}

/*
@brief Destroy a thread.
@param handle, the handle for thread.
*/
vip_status_e gckvip_os_destroy_thread(
    IN gckvip_thread handle
    )
{
    if (handle != VIP_NULL) {
        kthread_stop(handle);
        put_task_struct(handle);
    }
    else {
        PRINTK_E("failed to destroy thread");
        GCKVIP_DUMP_STACK();
        return VIP_ERROR_IO;
    }

    return VIP_SUCCESS;
}
#endif

/*
@brief Create a atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_create_atomic(
    OUT gckvip_atomic *atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    struct atomic_t *c_atomic = VIP_NULL;
    gcIsNULL(atomic);

    status = gckvip_os_allocate_memory(sizeof(atomic_t), (void**)&c_atomic);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to allocate memory for atomic\n");
        return VIP_ERROR_IO;
    }

    /* Initialize the atomic */
    atomic_set((atomic_t *)c_atomic, 0);
    *atomic = (gckvip_atomic)c_atomic;

onError:
    return status;
}

/*
@brief Destroy a atomic which create in gckvip_os_create_atomic
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_destroy_atomic(
    IN gckvip_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    gcIsNULL(atomic);

    gckvip_os_free_memory(atomic);

onError:
    return status;
}

/*
@brief Set the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_set_atomic(
    IN gckvip_atomic atomic,
    IN vip_uint32_t value
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == atomic) {
        PRINTK_E("failed to set atomic is NULL\n");
        GCKVIP_DUMP_STACK();
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    atomic_set((atomic_t *)atomic, value);
    return status;
}

/*
@brief Get the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_uint32_t gckvip_os_get_atomic(
    IN gckvip_atomic atomic
    )
{
    if (VIP_NULL == atomic) {
        PRINTK_E("failed to get atomic is NULL\n");
        GCKVIP_DUMP_STACK();
        return 0xFFFFFFFE;
    }
    return atomic_read((atomic_t *)atomic);
}

/*
@brief Increase the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_inc_atomic(
    IN gckvip_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == atomic) {
        PRINTK_E("failed to inc atomic is NULL\n");
        GCKVIP_DUMP_STACK();
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    atomic_inc((atomic_t *)atomic);
    return status;
}

/*
@brief Decrease the 32-bit value protected by an atomic.
@param atomic, create atomic pointer.
*/
vip_status_e gckvip_os_dec_atomic(
    IN gckvip_atomic atomic
    )
{
    vip_status_e status = VIP_SUCCESS;
    if (VIP_NULL == atomic) {
        PRINTK_E("failed to dec atomic is NULL\n");
        GCKVIP_DUMP_STACK();
        return VIP_ERROR_INVALID_ARGUMENTS;
    }

    atomic_dec((atomic_t *)atomic);
    return status;
}

#if vpmdENABLE_MULTIPLE_TASK || vpmdENABLE_SUSPEND_RESUME || vpmdPOWER_OFF_TIMEOUT
/*
@brief Create a signal.
@param handle, the handle for signal.
*/
vip_status_e gckvip_os_create_signal(
    OUT gckvip_signal *handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_signal_data_t *data = VIP_NULL;

    if (handle == VIP_NULL) {
        PRINTK_E("failed to create signal, parameter is NULL\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_signal_data_t), (vip_ptr*)&data));

    init_waitqueue_head(&data->wait_queue);
    data->wait_cond = vip_false_e;
    data->value = vip_false_e;

    *handle = (gckvip_signal)data;

    spin_lock_init(&data->lock);

    return status;

onError:
    if (data != VIP_NULL) {
        gckvip_os_free_memory(data);
    }

    return status;
}

vip_status_e gckvip_os_destroy_signal(
    IN gckvip_signal handle
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_signal_data_t *data = (gckvip_signal_data_t*)handle;

    if (data == VIP_NULL) {
        PRINTK_E("failed to destory signal, parameter is NULL\n");
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    data->wait_cond = vip_false_e;
    data->value = vip_false_e;
    gckvip_os_free_memory(handle);

onError:
    return status;
}

/*
@brief Waiting for a signal.
@param handle, the handle for signal.
@param timeout, the timeout of waiting signal. unit ms. timeout is 0 or vpmdINFINITE, infinite wait signal.
*/
vip_status_e gckvip_os_wait_signal(
    IN gckvip_signal handle,
    IN vip_uint32_t timeout
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_signal_data_t *data = (gckvip_signal_data_t*)handle;
    unsigned long jiffies;
    vip_int32_t ret = 0;

    if (data == VIP_NULL) {
        PRINTK_E("failed to wait signal, parameter is NULL\n");
        GCKVIP_DUMP_STACK();
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    if (vpmdINFINITE == timeout) {
        jiffies = msecs_to_jiffies(WAIT_SIGNAL_TIMEOUT);
    }
    else {
        jiffies = msecs_to_jiffies(timeout);
    }
    if (0 == jiffies) {
        jiffies = 1;
    }

    spin_lock(&data->lock);
    if (!data->value) {
        /* Wait for the condition. */
        data->wait_cond = vip_false_e;
        spin_unlock(&data->lock);

        if (vpmdINFINITE == timeout) {
            while (1) {
                ret = wait_event_timeout(data->wait_queue, data->wait_cond, jiffies);
                if (0 == ret) {
                    continue;
                }
                break;
            }
        }
        else {
            ret = wait_event_timeout(data->wait_queue, data->wait_cond, jiffies);
            if (0 == ret) {
                PRINTK_I("wait signal handle=0x%"PRPx", timeout=%dms\n", handle, timeout);
                status = VIP_ERROR_TIMEOUT;
                #if (vpmdENABLE_DEBUG_LOG>3)
                dump_stack();
                #endif
            }
        }
    }
    else {
        spin_unlock(&data->lock);
    }

onError:
    return status;
}

/*
@brief wake up wait signal.
     state is true, wake up all waiting signal and chane the signal state to SET.
     state is false, change the signal to RESET so the gckvip_os_wait_signal will block until
     the signal state changed to SET. block waiting signal function.
@param handle, the handle for signal.
*/
vip_status_e gckvip_os_set_signal(
    IN gckvip_signal handle,
    IN vip_bool_e state
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_signal_data_t *data = (gckvip_signal_data_t*)handle;

    if (data == VIP_NULL) {
        PRINTK_E("failed to set signal, parameter is NULL\n");
        GCKVIP_DUMP_STACK();
        gcGoOnError(VIP_ERROR_FAILURE);
    }

    spin_lock(&data->lock);
    if (vip_true_e == state) {
        data->value = vip_true_e;
        data->wait_cond = vip_true_e;
        /* wake up all signal in wait queue */
        wake_up_all(&data->wait_queue);
    }
    else {
        data->value = vip_false_e;
    }
    spin_unlock(&data->lock);

onError:
    return status;
}
#endif

#if vpmdPOWER_OFF_TIMEOUT
typedef struct _gckvip_timer
{
    struct delayed_work      work;
    gckvip_timer_func        function;
    vip_ptr                  data;
} gckvip_timer_t;

void _timer_function_handle(
    struct work_struct *work
    )
{
    gckvip_timer_t *timer = (gckvip_timer_t*)work;
    gckvip_timer_func function = timer->function;

    function(timer->data);
}

/*
@brief create a system timer callback.
@param func, the callback function of this timer.
@param param, the paramete of the timer.
*/
vip_status_e gckvip_os_create_timer(
    IN gckvip_timer_func func,
    IN vip_ptr param,
    OUT gckvip_timer *timer
    )
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_timer_t *pointer = VIP_NULL;

    gcOnError(gckvip_os_allocate_memory(sizeof(gckvip_timer_t), (vip_ptr*)&pointer));

    pointer->function = func;
    pointer->data = param;

    INIT_DELAYED_WORK(&pointer->work, _timer_function_handle);

    if (timer != VIP_NULL) {
        *timer = (gckvip_timer)pointer;
    }
    else {
        status = VIP_ERROR_FAILURE;
    }

onError:
    return status;
}

vip_status_e gckvip_os_start_timer(
    IN gckvip_timer timer,
    IN vip_uint32_t time_out
    )
{
    gckvip_timer_t *pointer = (gckvip_timer_t*)timer;

    if (VIP_NULL == pointer) {
        PRINTK_E("failed to start timer, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    schedule_delayed_work(&pointer->work, msecs_to_jiffies(time_out));

    return VIP_SUCCESS;
}

vip_status_e gckvip_os_stop_timer(
    IN gckvip_timer timer
    )
{
    gckvip_timer_t *pointer = (gckvip_timer_t*)timer;

    if (VIP_NULL == pointer) {
        PRINTK_E("failed to stop timer, parameter is NULL\n");
        return VIP_ERROR_FAILURE;
    }

    cancel_delayed_work_sync(&pointer->work);

    return VIP_SUCCESS;
}

/*
@brief destroy a system timer
*/
vip_status_e gckvip_os_destroy_timer(
    IN gckvip_timer timer
    )
{
    if (timer != VIP_NULL) {
        gckvip_timer_t *pointer = (gckvip_timer_t*)timer;
        cancel_delayed_work_sync(&pointer->work);
        gckvip_os_free_memory(timer);
    }

    return VIP_SUCCESS;
}
#endif


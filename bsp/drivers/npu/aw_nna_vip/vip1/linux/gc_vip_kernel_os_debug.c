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
#if vpmdENABLE_DEBUGFS
#include "gc_vip_kernel_os_debug.h"
#include "gc_vip_kernel_drv.h"
#include <gc_vip_kernel.h>
#include <gc_vip_hardware.h>
#include <gc_vip_kernel_port.h>
#include <gc_vip_video_memory.h>
#include <gc_vip_kernel_debug.h>
#include <linux/string.h>
#include <asm/div64.h>
#include <gc_vip_kernel_pm.h>


#ifndef vpmdUSE_DEBUG_FS
#ifdef CONFIG_DEBUG_FS
/*
   debugFS is enabled when vpmdUSE_DEBUG_FS set to 1
   debugFS folder is in: /sys/kernel/debug/viplite/
*/
#define vpmdUSE_DEBUG_FS            1
#else
/*
   sysFS is enabled when vpmdUSE_DEBUG_FS set to 0
   sysFS folder is in: /sys/devices/platform/xxx.vipcore/
   use bin_attribute for sysfs to send msg larger than 1 page size
*/
#define vpmdUSE_DEBUG_FS            0
#endif
#endif

#if vpmdENABLE_DEBUGFS
vip_uint32_t debugfs_core_cnt = 1;
gckvip_core_loading_profile_t *core_loading = VIP_NULL;
#endif

vip_uint32_t register_address = 0x0, register_value = 0x0;

static DEFINE_MUTEX(debugfs_mutex);

#define CHECK_CONTEXT_INITIALIZE()  \
{   \
    if (0 == context->initialize) { \
        len += fs_printf(p + len, "vip not working...\n"); \
        return len;   \
    }   \
}

#define CHECK_POWER(item)  \
GCKVIP_POWER_ON == (item) || \
GCKVIP_POWER_IDLE == (item)

#if vpmdUSE_DEBUG_FS
#define FS_PRINTF(p, len, off, ...)                                                       \
    len += fs_printf(p + len - off, __VA_ARGS__);
#if defined(CONFIG_CPU_CSKYV2) && LINUX_VERSION_CODE <= KERNEL_VERSION(3,0,8)
void seq_vprintf(
    struct seq_file *m,
    const char *f,
    va_list args
    )
{
    int len;
    if (m->count < m->size)
    {
        len = vsnprintf(m->buf + m->count, m->size - m->count, f, args);
        if (m->count + len < m->size)
        {
            m->count += len;
            return;
        }
    }
    m->count = m->size;
}
#endif
static int fs_printf(
    IN void* obj,
    IN const char* fmt,
    ...
    )
{
    va_list args;
    va_start(args, fmt);
    seq_vprintf((struct seq_file*)obj, fmt, args);
    va_end(args);

    return 0;
}
#else
typedef struct _sys_param {
    char* buf;
    loff_t offset;
} sys_param;
/* write 1024 bytes at most each time */
#define FS_PRINTF(p, len, off, ...)                                            \
    if (len - off > 1024) goto flush_buffer;                                   \
    len += fs_printf(p + (len >= off ? len - off : 0), __VA_ARGS__);
static int fs_printf(
    IN void* obj,
    IN const char* fmt,
    ...
    )
{
    int len = 0;
    va_list args;
    va_start(args, fmt);
    len = vsprintf((char*)obj, fmt, args);
    va_end(args);

    return len;
}
#endif

/*
@brief show the viplite info profile result
*/
static loff_t gckvip_vipinfo_show(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vip_bool_e need_init = vip_true_e;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t chip_ver1, chip_ver2, chip_cid, chip_date;
    gckvip_context_t *context = gckvip_get_context();
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    mutex_lock(&debugfs_mutex);

    if (0 < context->initialize) {
        need_init = vip_false_e;
    }

    if (vip_true_e == need_init) {
        need_init = vip_false_e;
        status = gckvip_init();
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to init hardware\n");
            mutex_unlock(&debugfs_mutex);
            return 0;
        }
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_lock_mutex(context->initialize_mutex);
    }
#endif

    GCKVIP_LOOP_DEVICE_START
#if vpmdONE_POWER_DOMAIN
    if (CHECK_POWER(gckvip_os_get_atomic(context->sp_management.status))) {
#else
    if (CHECK_POWER(gckvip_os_get_atomic(device->dp_management.status))) {
#endif
        vip_uint32_t hw_index = 0;
        gckvip_hardware_t *hardware = VIP_NULL;

        #if vpmdENABLE_CLOCK_SUSPEND
        gckvip_os_inc_atomic(device->disable_clock_suspend);
        gckvip_pm_enable_clock(device);
        #endif

        for (hw_index = 0; hw_index < device->hardware_count; hw_index++) {
            hardware = &device->hardware[hw_index];
            status = gckvip_os_read_reg(hardware->reg_base, 0x00020, &chip_ver1);
            if (status != VIP_SUCCESS) {
                PRINTK_E("viplite read chip ver1 faild, error = %d.\n", status);
            }

            status = gckvip_os_read_reg(hardware->reg_base, 0x00024, &chip_ver2);
            if (status != VIP_SUCCESS) {
                PRINTK_E("viplite read chip ver2 faild, error = %d.\n", status);
            }

            status = gckvip_os_read_reg(hardware->reg_base, 0x00030, &chip_cid);
            if (status != VIP_SUCCESS) {
                PRINTK_E("viplite read chip cid faild, error = %d.\n", status);
            }

            status = gckvip_os_read_reg(hardware->reg_base, 0x00028, &chip_date);
            if (status != VIP_SUCCESS) {
                PRINTK_E("viplite read chip date faild, error = %d.\n", status);
            }

            printk("vipcore_%d info: pid=0x%x, date=0x%x, ver1=0x%x, ver2=0x%x.\n",
                    hardware->core_id, chip_cid, chip_date, chip_ver1, chip_ver2);

            FS_PRINTF(ptr, len, offset, "vipcore_%d info: pid=0x%x, date=0x%x, ver1=0x%x, ver2=0x%x.\n",
                    hardware->core_id, chip_cid, chip_date, chip_ver1, chip_ver2);
        }

        #if vpmdENABLE_CLOCK_SUSPEND
        gckvip_os_dec_atomic(device->disable_clock_suspend);
        #endif
    }
    else {
        FS_PRINTF(ptr, len, offset, "vip %d not working, may vip has power off, can't read register\n", \
               device->device_id);
    }
    GCKVIP_LOOP_DEVICE_END

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        gckvip_os_unlock_mutex(context->initialize_mutex);
    }
#endif
    if (vip_true_e == need_init) {
        gckvip_destroy();
    }

    mutex_unlock(&debugfs_mutex);

    return len;
}

static loff_t gckvip_vipfreq_show(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    gckvip_context_t *context = gckvip_get_context();
    vip_uint64_t mc_freq = 0, sh_freq = 0;
    vip_uint32_t hw_index = 0;
    gckvip_hardware_t *hardware = VIP_NULL;
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    mutex_lock(&debugfs_mutex);

    if (context->initialize > 0) {
        GCKVIP_LOOP_DEVICE_START
        FS_PRINTF(ptr, len, offset, "vip frequency percent = %d\n", \
                  gckvip_os_get_atomic(device->core_fscale_percent));
#if vpmdONE_POWER_DOMAIN
        if (CHECK_POWER(gckvip_os_get_atomic(context->sp_management.status))) {
#else
        if (CHECK_POWER(gckvip_os_get_atomic(device->dp_management.status))) {
#endif
#if vpmdENABLE_MULTIPLE_TASK
            if (context->initialize_mutex != VIP_NULL) {
                gckvip_os_lock_mutex(context->initialize_mutex);
            }
#endif

            #if vpmdENABLE_CLOCK_SUSPEND
            gckvip_os_inc_atomic(device->disable_clock_suspend);
            gckvip_pm_enable_clock(device);
            #endif

            gckvip_pm_set_frequency(device, gckvip_os_get_atomic(device->core_fscale_percent));

            for (hw_index = 0; hw_index < device->hardware_count; hw_index++) {
                hardware = &device->hardware[hw_index];
                if (VIP_SUCCESS == gckvip_get_clock(hardware, &mc_freq, &sh_freq)) {
                    FS_PRINTF(ptr, len, offset, "  Core Frequency=%" PRId64" HZ\n", mc_freq);
                    FS_PRINTF(ptr, len, offset, "  PPU  Frequency=%" PRId64" HZ\n", sh_freq);
                }
            }

            #if vpmdENABLE_CLOCK_SUSPEND
            gckvip_os_dec_atomic(device->disable_clock_suspend);
            #endif

#if vpmdENABLE_MULTIPLE_TASK
            if (context->initialize_mutex != VIP_NULL) {
                gckvip_os_unlock_mutex(context->initialize_mutex);
            }
#endif
        }
        else {
            FS_PRINTF(ptr, len, offset, "vip %d not working, may vip has power off, can't read register\n", \
                   device->device_id);
        }
        GCKVIP_LOOP_DEVICE_END
    }
    else {
        vip_status_e status = VIP_SUCCESS;
        status = gckvip_init();
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to init hardware\n");
            mutex_unlock(&debugfs_mutex);
            return 0;
        }
        GCKVIP_LOOP_DEVICE_START
#if vpmdONE_POWER_DOMAIN
        if (CHECK_POWER(gckvip_os_get_atomic(context->sp_management.status))) {
#else
        if (CHECK_POWER(gckvip_os_get_atomic(device->dp_management.status))) {
#endif
#if vpmdENABLE_MULTIPLE_TASK
            if (context->initialize_mutex != VIP_NULL) {
                gckvip_os_lock_mutex(context->initialize_mutex);
            }
#endif

            #if vpmdENABLE_CLOCK_SUSPEND
            gckvip_os_inc_atomic(device->disable_clock_suspend);
            gckvip_pm_enable_clock(device);
            #endif

            gckvip_pm_set_frequency(device, gckvip_os_get_atomic(device->core_fscale_percent));
            for (hw_index = 0; hw_index < device->hardware_count; hw_index++) {
                hardware = &device->hardware[hw_index];
                if (VIP_SUCCESS == gckvip_get_clock(hardware, &mc_freq, &sh_freq)) {
                    FS_PRINTF(ptr, len, offset, "  Core Frequency=%" PRId64" HZ\n", mc_freq);
                    FS_PRINTF(ptr, len, offset, "  PPU  Frequency=%" PRId64" HZ\n", sh_freq);
                }
            }

            #if vpmdENABLE_CLOCK_SUSPEND
            gckvip_os_dec_atomic(device->disable_clock_suspend);
            #endif

#if vpmdENABLE_MULTIPLE_TASK
            if (context->initialize_mutex != VIP_NULL) {
                gckvip_os_unlock_mutex(context->initialize_mutex);
            }
#endif
        }
        else {
            FS_PRINTF(ptr, len, offset, "vip %d not working, may vip has power off, can't read register\n", \
                   device->device_id);
        }
        GCKVIP_LOOP_DEVICE_END

        gckvip_destroy();
    }

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    mutex_unlock(&debugfs_mutex);

    return len;

}

static ssize_t gckvip_vipfreq_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;

    gckvip_context_t *context = gckvip_get_context();
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    mutex_lock(&debugfs_mutex);

    if (count > 128) {
        PRINTK_E("please echo bytes no more than 128, not reset time profile data.\n");
        return count;
    }
    ret  = simple_strtol(buf, NULL, 0);
    if ((ret > 0) && (ret <= 100)) {
        kdriver->core_fscale_percent = ret;
        PRINTK("set vip frequency percent=%d\n", ret);
        if (context->initialize != 0) {
            GCKVIP_LOOP_DEVICE_START
            gckvip_os_set_atomic(device->core_fscale_percent, ret);
        #if vpmdONE_POWER_DOMAIN
            if (CHECK_POWER(gckvip_os_get_atomic(context->sp_management.status))) {
        #else
            if (CHECK_POWER(gckvip_os_get_atomic(device->dp_management.status))) {
        #endif
                #if vpmdENABLE_CLOCK_SUSPEND
                gckvip_os_inc_atomic(device->disable_clock_suspend);
                gckvip_pm_enable_clock(device);
                #endif

                gckvip_pm_set_frequency(device, ret);

                #if vpmdENABLE_CLOCK_SUSPEND
                gckvip_os_dec_atomic(device->disable_clock_suspend);
                #endif
            }
            else {
                PRINTK("vip %d not working, may vip has power off, can't read register\n", device->device_id);
            }
            GCKVIP_LOOP_DEVICE_END
        }
    }
    else {
        PRINTK("please set 1~~100 value for vip frequency percent\n");
    }
    mutex_unlock(&debugfs_mutex);

    return count;
}

static loff_t gckvip_core_loading_get(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vip_uint32_t i = 0;
    vip_uint64_t total_time = 0, cur_time = 0, total_time_ms = 0;

#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

	if (VIP_NULL == core_loading)
		return 0;

	cur_time = gckvip_os_get_time();
	if (0 == core_loading[0].destory_time) {
		total_time = cur_time - core_loading[0].init_time;

	} else {
		core_loading[0].infer_time = 0;
		core_loading[0].last_infer_time = 0;
		core_loading[0].last_record_time = 0;
		core_loading[0].latest_sbumit_time = 0;
	}
	total_time_ms = total_time;
	do_div(total_time_ms, 1000);

	for (i = 0; i < debugfs_core_cnt; i++) {
		/*
		vip_uint64_t infer_ms = core_loading[i].infer_time;
		vip_uint64_t ratio = core_loading[i].infer_time * 100;
		do_div(infer_ms, 1000);
		do_div(ratio, total_time);
		if (0 == i) {
			FS_PRINTF(ptr, len, offset, "NPU Loading -----> Core%d: Inference Time=%"PRId64" ms (%2d%%)\n",
					i, infer_ms, ratio);
		} else {
			FS_PRINTF(ptr, len, offset, "                       Core%d: Inference Time=%"PRId64" ms (%2d%%)\n",
					i, infer_ms, ratio);
		}*/
		vip_uint64_t step_infer_us = core_loading[i].infer_time - core_loading[0].last_infer_time;
		vip_uint64_t step_total_us = 0, ratio = 0;
		cur_time = gckvip_os_get_time();
		step_total_us = cur_time - core_loading[0].last_record_time;
		ratio = step_infer_us * 100;
		do_div(ratio, step_total_us);
		if (0 == i) {
			FS_PRINTF(ptr, len, offset, "NPU Loading -----> Core%d: %2d%% \n",
					i, ratio);
		} else {
			FS_PRINTF(ptr, len, offset, "                   Core%d: %2d%%\n",
					i, ratio);
		}
	}

	core_loading[0].last_record_time = core_loading[0].latest_sbumit_time;/*cur_time;*/
	core_loading[0].last_infer_time = core_loading[0].infer_time;


#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return len;
}

static ssize_t gckvip_core_loading_set(
    const char *buf,
    size_t count
    )
{
    vip_uint32_t ret;
    gckvip_driver_t *kdriver;
    vip_uint32_t i = 0;
    kdriver = gckvip_get_kdriver();

    ret  = strncmp(buf, "reset", 5);

    if (!ret) {
	    for (i = 0; i < debugfs_core_cnt; i++) {
		    core_loading[i].init_time = gckvip_os_get_time();
		    core_loading[i].submit_time = 0;
		    core_loading[i].infer_time = 0;
		    core_loading[i].destory_time = 0;
		    core_loading[i].last_record_time = core_loading[i].init_time;
		    core_loading[i].last_infer_time = 0;
		    core_loading[i].latest_sbumit_time = 0;
	    }
    } else {
	    PRINTK_E("invalid param, please set reset string to reset core loading data.\n");
    }

    return count;
}

static loff_t gckvip_mem_profile(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    gckvip_context_t *context = gckvip_get_context();
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    if (context->initialize > 0) {
        gckvip_os_lock_mutex(context->memory_mutex);
    }

    FS_PRINTF(ptr, len, offset, "video memory profile\n");
    FS_PRINTF(ptr, len, offset, " ------------------------------------------------------------------------------\n");
    FS_PRINTF(ptr, len, offset, "|       current(byte)    |      peak(byte)    |   alloc count   |   free count \n");
    FS_PRINTF(ptr, len, offset, "|------------------------------------------------------------------------------\n");
    FS_PRINTF(ptr, len, offset, "|      0x%08x        |     0x%08x     |      %d         |    %d     \n",
                kdriver->profile_data.video_memory,
                kdriver->profile_data.video_peak,
                kdriver->profile_data.video_allocs,
                kdriver->profile_data.video_frees);

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    if (context->initialize > 0) {
        gckvip_os_unlock_mutex(context->memory_mutex);
    }

    return len;
}

static ssize_t gckvip_mem_profile_set(
    const char *buf,
    size_t count
    )
{
    vip_uint32_t ret;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    ret = strncmp(buf, "reset", 5);
    if (!ret) {
        kdriver->profile_data.video_memory = 0;
        kdriver->profile_data.video_peak = 0;
        kdriver->profile_data.video_allocs = 0;
        kdriver->profile_data.video_frees = 0;
    }
    else {
        PRINTK_E("invalid param, please set reset to reset memory profile data.\n");
    }

  return count;
}

vip_status_e show_memory_mapping(
    gckvip_video_mem_handle_t *ptr,
    void *m,
    vip_uint32_t index,
    vip_bool_e is_heap_alloc,
    loff_t *len,
    loff_t offset
    )
{
    vip_uint32_t iter = 0;
    vip_char_t *alloc_type = VIP_NULL;
    gckvip_context_t *context = VIP_NULL;
    gckvip_mem_mapping_profile_t *memory_mapping = VIP_NULL;
    vip_uint8_t *kernel_logical = VIP_NULL;
    vip_uint8_t *user_logical = VIP_NULL;
#if vpmdUSE_DEBUG_FS
    struct seq_file *p = (struct seq_file*)m;
#else
    char *p = (char*)m;
#endif
    vip_status_e status = VIP_SUCCESS;
    context = gckvip_get_context();

    memory_mapping = kmalloc(sizeof(gckvip_mem_mapping_profile_t), GFP_KERNEL);
    if (memory_mapping == NULL) {
        PRINTK("memory mapping kmalloc memory failed.\n");
        return VIP_ERROR_FAILURE;
    }
    memset(memory_mapping, 0, sizeof(gckvip_mem_mapping_profile_t));

    if (ptr->memory_type & GCVIP_VIDEO_MEMORY_TYPE_WRAP_DMA_BUF) {
        alloc_type = "  dma_buf ";
    }
    else if (ptr->memory_type & GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC) {
        alloc_type = "dyna_alloc";
    }
    else if (ptr->memory_type & GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_LOGICAL) {
        alloc_type = "wrap_logic";
    }
    else if (ptr->memory_type & GCVIP_VIDEO_MEMORY_TYPE_WRAP_USER_PHYSICAL) {
        alloc_type = "wrap_physic";
    }

    if (!is_heap_alloc) {
        memory_mapping->alloc_type = alloc_type;
        memory_mapping->size_table = ptr->node.dyn_node.size_table;
        memory_mapping->virtual_addr = ptr->node.dyn_node.vip_virtual_address;
        memory_mapping->physical_table = ptr->node.dyn_node.physical_table;
        memory_mapping->kernel_logical = ptr->node.dyn_node.kerl_logical;
        memory_mapping->user_logical = ptr->node.dyn_node.user_logical;
        memory_mapping->physical_num = ptr->node.dyn_node.physical_num;
        for (iter = 0; iter < memory_mapping->physical_num; iter++) {
            if (memory_mapping->kernel_logical != VIP_NULL) {
                kernel_logical = memory_mapping->kernel_logical + iter * memory_mapping->size_table[iter];
            }
            else {
                kernel_logical = VIP_NULL;
            }
            if (memory_mapping->user_logical != VIP_NULL) {
                user_logical = memory_mapping->user_logical + iter * memory_mapping->size_table[iter];
            }
            else {
                user_logical = VIP_NULL;
            }

            if (0 == iter) {
                FS_PRINTF(p, *(len), offset, "|   %08d   |  %s  |  0x%08x |    0x%" PRIx64"   "\
                                               "|   0x%08x  |    0x%"PRPx"  |     0x%"PRPx" |\n",
                index, memory_mapping->alloc_type, memory_mapping->size_table[iter],
                memory_mapping->physical_table[iter],
                memory_mapping->virtual_addr + iter * memory_mapping->size_table[iter], kernel_logical, user_logical);
            }
            else {
                FS_PRINTF(p, *(len), offset, "            %08d          |  0x%08x |    0x%" PRIx64"   "\
                                               "|   0x%08x  |    0x%"PRPx"  |     0x%"PRPx" |\n",
                 iter, memory_mapping->size_table[iter], memory_mapping->physical_table[iter],
                 memory_mapping->virtual_addr + iter * memory_mapping->size_table[iter], kernel_logical, user_logical);
            }

            FS_PRINTF(p, *(len), offset, "|-----------------------------------------------"\
                "----------------------------------------------------------------------------|\n");
        }
    }
    else {
        memory_mapping->alloc_type = "heap_alloc";
#if vpmdENABLE_VIDEO_MEMORY_HEAP
        memory_mapping->size = context->heap_size;
        memory_mapping->virtual_addr = context->heap_virtual;
        memory_mapping->physical = context->heap_physical;
        memory_mapping->kernel_logical = context->heap_kernel;
        memory_mapping->user_logical = context->heap_user;
#else
        memory_mapping->size = 0;
        memory_mapping->virtual_addr = 0;
        memory_mapping->physical = 0;
        memory_mapping->kernel_logical = NULL;
        memory_mapping->user_logical = NULL;
#endif
        FS_PRINTF(p, *(len), offset, "|   %08d   |  %s  |  0x%08x |    0x%" PRIx64"   |   0x%08x  " \
                                       "|     0x%"PRPx"  |      0x%"PRPx" |\n",
        index, memory_mapping->alloc_type, memory_mapping->size, memory_mapping->physical,
        memory_mapping->virtual_addr, memory_mapping->kernel_logical, memory_mapping->user_logical);
        FS_PRINTF(p, *(len), offset, "|----------------------------------------------------"\
            "-----------------------------------------------------------------------|\n");
    }
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    kfree(memory_mapping);

    return status;
}

/*
@brief show the viplite memory mapping profile result
*/
static loff_t gckvip_mem_mapping(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vip_uint32_t iter = 0;
    vip_uint32_t left_mem_count = 0;
    gckvip_video_mem_handle_t *pointer = VIP_NULL;
    vip_bool_e is_heap_alloc = vip_false_e, heap_not_profile = vip_true_e;
    vip_status_e status = VIP_SUCCESS;
    gckvip_context_t *context = gckvip_get_context();
    vip_uint32_t first_free_pos = 0;
    gckvip_database_table_t *table = VIP_NULL;
#if vpmdUSE_DEBUG_FS
    struct seq_file *p = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* p = param->buf;
    offset = param->offset;
#endif

    CHECK_CONTEXT_INITIALIZE();

    status = gckvip_os_lock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("lock memory mutex faild, status = %d.\n", status);
        return status;
    }
    FS_PRINTF(p, len, offset, " start profiling...\n");
    FS_PRINTF(p, len, offset, " ------------------------------------------------------------------------------"  \
                    "--------------------------------------------- \n");
    FS_PRINTF(p, len, offset, "|    index    |  alloc_type  |     size    |     physical    |      virtual   |    "  \
                    "   kerl_logical     |        user_logical    |\n");
    FS_PRINTF(p, len, offset, "|------------------------------------------------------------------------------"  \
                    "---------------------------------------------|\n");

    first_free_pos = gckvip_database_get_freepos(&context->videomem_database);
    for (iter = 0; iter < first_free_pos; iter++) {
        gckvip_database_get_table(&context->videomem_database, iter + DATABASE_MAGIC_DATA, &table);
        if (table->handle != VIP_NULL) {
            pointer = (gckvip_video_mem_handle_t*)table->handle;
            is_heap_alloc = (pointer->memory_type == GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP) ? vip_true_e : vip_false_e;
            if (is_heap_alloc) {
                if (heap_not_profile) {
                    show_memory_mapping(pointer, (void*)p, iter, vip_true_e, &len, offset);
                    heap_not_profile = vip_false_e;
                    left_mem_count++;
                }
                else {
                  continue;
                }
            }
            else {
                show_memory_mapping(pointer, (void*)p, iter, vip_false_e, &len, offset);
                left_mem_count++;
            }
        }
    }
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    status = gckvip_os_unlock_mutex(context->memory_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("unlock memory mutex faild, status = %d.\n", status);
        return status;
    }

    if (left_mem_count < 1) {
        len += fs_printf(p + len, "all the video memory have freed, not need profile.\n");
        return len;
    }

    return len;
}

#if vpmdENABLE_MMU
static vip_status_e gckvip_debug_dump_data(
    void *m,
    loff_t  *len,
    loff_t offset,
    vip_uint32_t *data,
    vip_uint32_t physical,
    vip_uint32_t size
    )
{
    vip_uint32_t i = 0;
    vip_uint32_t line = size / 32;
    vip_uint32_t left = size % 32;
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    char *ptr = (char*)m;
#endif

    for (i = 0; i < line; i++) {
        FS_PRINTF(ptr, *(len), offset, "%08X : %08X %08X %08X %08X %08X %08X %08X %08X\n",
                  physical, data[0], data[1], data[2], data[3],
                  data[4], data[5], data[6], data[7]);
        data += 8;
        physical += 8 * 4;
    }

    switch(left) {
    case 28:
      FS_PRINTF(ptr, *(len), offset, "%08X : %08X %08X %08X %08X %08X %08X %08X\n",
                physical, data[0], data[1], data[2], data[3],
                data[4], data[5], data[6]);
      break;
    case 24:
      FS_PRINTF(ptr, *(len), offset, "%08X : %08X %08X %08X %08X %08X %08X\n",
                physical, data[0], data[1], data[2], data[3],
                data[4], data[5]);
      break;
    case 20:
      FS_PRINTF(ptr, *(len), offset, "%08X : %08X %08X %08X %08X %08X\n",
                physical, data[0], data[1], data[2], data[3],
                data[4]);
      break;
    case 16:
      FS_PRINTF(ptr, *(len), offset, "%08X : %08X %08X %08X %08X\n", physical, data[0],
                data[1], data[2], data[3]);
      break;
    case 12:
      FS_PRINTF(ptr, *(len), offset, "%08X : %08X %08X %08X\n", physical, data[0],
                data[1], data[2]);
      break;
    case 8:
      FS_PRINTF(ptr, *(len), offset, "%08X : %08X %08X\n", physical, data[0], data[1]);
      break;
    case 4:
      FS_PRINTF(ptr, *(len), offset, "%08X : %08X\n", physical, data[0]);
      break;
    default:
      break;
    }
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return VIP_SUCCESS;
}

static vip_status_e gckvip_debug_dump_mtlb(
    void *m,
    loff_t  *len,
    loff_t offset,
    gckvip_context_t *context
    )
{
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    char *ptr = (char*)m;
#endif

    FS_PRINTF(ptr, *(len), offset, "\n***DUMP MMU entry****\n");
    gckvip_debug_dump_data((void*)ptr,
                     len,
                     offset,
                     context->MMU_entry.logical,
                     context->MMU_entry.physical,
                     (context->MMU_entry.size > 64) ? 64 : context->MMU_entry.size
                     );

    FS_PRINTF(ptr, *(len), offset, "*****DUMP MTLB******\n");
    gckvip_debug_dump_data((void*)ptr,
                     len,
                     offset,
                     context->MTLB.logical,
                     context->MTLB.physical,
                     context->MTLB.size);
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return VIP_SUCCESS;
}

static vip_status_e gckvip_debug_dump_stlb(
    void *m,
    loff_t *len,
    loff_t offset,
    gckvip_context_t *context
    )
{
    vip_int32_t i = 0;
    vip_uint32_t *logical = VIP_NULL;
    vip_int32_t unit = 0;
    vip_uint32_t dump_size = 0;
    vip_uint32_t *tmp = VIP_NULL;
    vip_int32_t total_num = 0;
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    char *ptr = (char*)m;
#endif

    FS_PRINTF(ptr, *(len), offset, "\n*****DUMP STLB Start******\n");
    FS_PRINTF(ptr, *(len), offset, "dump 1Mbytes STLB:\n");
    FS_PRINTF(ptr, *(len), offset, "1Mbytes STLB physical base=0x%"PRIx64", size=0x%x\n",
                    context->ptr_STLB_1M->physical,
                    context->ptr_STLB_1M->size);

    logical = (vip_uint32_t*)context->ptr_STLB_1M->logical;
    unit = gcdMMU_1M_MTLB_COUNT * gcdMMU_STLB_1M_ENTRY_NUM;
    if (VIP_NULL != logical) {
        for (i = unit -1; i >= 0; i--) {
            /* 0 bit indicate whether present or not. */
            if (logical[i] & (1 << 0)) {
                break;
            }
        }

        total_num = i + 1;
        for (i = 0; i < total_num; i += 8) {
            tmp = logical + i;
            if ((total_num - i) >= 8) {
                dump_size = 8 * sizeof(vip_uint32_t);
            }
            else {
                dump_size = (total_num - i) * sizeof(vip_uint32_t);
            }
            gckvip_debug_dump_data((void*)ptr,
                                 len,
                                 offset,
                                 tmp,
                                 context->ptr_STLB_1M->physical + sizeof(vip_uint32_t) * i,
                                 dump_size);
        }
    }

    FS_PRINTF(ptr, *(len), offset, "dump 4Kbytes STLB:\n");
    FS_PRINTF(ptr, *(len), offset, "4Kbytes STLB physical base=0x%"PRIx64", size=0x%x\n",
                    context->ptr_STLB_4K->physical,
                    context->ptr_STLB_4K->size);

    logical = (vip_uint32_t*)context->ptr_STLB_4K->logical;
    unit = gcdMMU_4K_MTLB_COUNT * gcdMMU_STLB_4K_ENTRY_NUM;
    if (VIP_NULL != logical) {
        for (i = unit -1; i >= 0; i--) {
            /* 0 bit indicate whether present or not. */
            if (logical[i] & (1 << 0)) {
                break;
            }
        }

        total_num = i + 1;
        for (i = 0; i < total_num; i += 8) {
            tmp = logical + i;
            if ((total_num - i) >= 8) {
                dump_size = 8 * sizeof(vip_uint32_t);
            }
            else {
                dump_size = (total_num - i) * sizeof(vip_uint32_t);
            }
            gckvip_debug_dump_data((void*)ptr,
                                 len,
                                 offset,
                                 tmp,
                                 context->ptr_STLB_4K->physical + sizeof(vip_uint32_t) * i,
                                 dump_size);
        }
    }

    FS_PRINTF(ptr, *(len), offset, "*****DUMP STLB Done******\n");
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return VIP_SUCCESS;
}
/*
@brief show the viplite page table profile result
*/
static loff_t gckvip_dump_mmu_table(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    gckvip_context_t *context = VIP_NULL;
#if vpmdENABLE_MULTIPLE_TASK
    vip_status_e status = VIP_SUCCESS;
#endif
#if vpmdUSE_DEBUG_FS
    struct seq_file *p = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* p = param->buf;
    offset = param->offset;
#endif

    context = gckvip_get_context();
    CHECK_CONTEXT_INITIALIZE();

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_lock_mutex(context->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to lock mmu mutex\n");
        return VIP_ERROR_FAILURE;
    }
#endif
#if vpmdENABLE_CLOCK_SUSPEND
    GCKVIP_LOOP_DEVICE_START
    gckvip_os_inc_atomic(device->disable_clock_suspend);
    gckvip_pm_enable_clock(device);
    GCKVIP_LOOP_DEVICE_END
#endif

    gckvip_debug_dump_mtlb((void*)p, &len, offset, context);
    gckvip_debug_dump_stlb((void*)p, &len, offset, context);

#if vpmdENABLE_CLOCK_SUSPEND
    GCKVIP_LOOP_DEVICE_START
    gckvip_os_dec_atomic(device->disable_clock_suspend);
    GCKVIP_LOOP_DEVICE_END
#endif

#if vpmdENABLE_MULTIPLE_TASK
    status = gckvip_os_unlock_mutex(context->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("failed to unlock mmu mutex\n");
    }
#endif

    return len;
}
#endif

#if vpmdENABLE_HANG_DUMP
static loff_t gckvip_get_hang_dump(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    gckvip_context_t *context = gckvip_get_context();
#if vpmdUSE_DEBUG_FS
    struct seq_file *p = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* p = param->buf;
    offset = param->offset;
#endif

    CHECK_CONTEXT_INITIALIZE();
    PRINTK("debugFS hang dump start ...\n");

    GCKVIP_LOOP_DEVICE_START
    #if vpmdONE_POWER_DOMAIN
    if (CHECK_POWER(gckvip_os_get_atomic(context->sp_management.status))) {
    #else
    if (CHECK_POWER(gckvip_os_get_atomic(device->dp_management.status))) {
    #endif
        vip_uint32_t i = 0;
        #if vpmdENABLE_CLOCK_SUSPEND
        gckvip_os_inc_atomic(device->disable_clock_suspend);
        gckvip_pm_enable_clock(device);
        #endif

        for (i = 0; i < device->hardware_count; i++) {
            gckvip_dump_states(context, device);
            gckvip_dump_command(context, device);
            #if vpmdENABLE_WAIT_LINK_LOOP
            gckvip_dump_waitlink(device);
            #endif
        }

        #if vpmdENABLE_CLOCK_SUSPEND
        gckvip_os_dec_atomic(device->disable_clock_suspend);
        #endif
    }
    else {
        FS_PRINTF(p, len, offset, "vip not working, may vip has power off, can't read register\n");
    }
    GCKVIP_LOOP_DEVICE_END
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return len;
}
#endif

#if vpmdENABLE_SUSPEND_RESUME
static ssize_t gckvip_suspend_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    vip_status_e status = VIP_SUCCESS;
    mutex_lock(&debugfs_mutex);

    ret  = simple_strtol(buf, NULL, 0);
    if (ret) {
        /* suspend */
        PRINTK("debugfs suspend start ....\n");
        gckvip_pm_suspend();
        PRINTK("debugfs suspend done ....\n");
    }
    else if (0 == ret) {
        /* resume */
        PRINTK("debugfs resume start ...\n");
        status = gckvip_pm_resume();
        if (status != VIP_SUCCESS) {
            PRINTK("vipcore, failed to do pm resume\n");
        }
        PRINTK("debugfs resume done ...\n");
    }
    else {
        PRINTK_E("suspend not support this value=%d\n", ret);
    }

    mutex_unlock(&debugfs_mutex);

    return count;
}
#endif
#if vpmdENABLE_DEBUG_LOG > 2
static ssize_t gckvip_rt_log_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    if (count > 128) {
        PRINTK_E("please echo bytes no more than 128, not modify rt_log status.\n");
        return count;
    }
    ret  = simple_strtol(buf, NULL, 0);
    if ((ret == 0) || (ret == 1)) {
        kdriver->log_level = ret;
    }
    else {
        PRINTK("please set 0 or 1 value for disable or enable log.\n");
    }

    return count;
}
#endif
#if vpmdENABLE_CAPTURE
static ssize_t gckvip_rt_capture_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    if (count > 128) {
        PRINTK_E("please echo bytes no more than 128, not modify capture status.\n");
        return count;
    }
    ret  = simple_strtol(buf, NULL, 0);
    if (ret == 1) {
        kdriver->func_config.enable_capture = vip_true_e;
    }
    else if (ret == 0){
        kdriver->func_config.enable_capture = vip_false_e;
    }
    else {
        PRINTK("please set 0 or 1 value for disable or enable capture.\n");
    }

    return count;
}
#endif

#if vpmdENABLE_CNN_PROFILING
static ssize_t gckvip_cnn_profile_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    if (count > 128) {
        PRINTK_E("please echo bytes no more than 128, not modify cnn profile status.\n");
        return count;
    }
    ret  = simple_strtol(buf, NULL, 0);
    if (ret == 1) {
        kdriver->func_config.enable_cnn_profile = vip_true_e;
    }
    else if (ret == 0){
        kdriver->func_config.enable_cnn_profile = vip_false_e;
    }
    else {
        PRINTK("please set 0 or 1 value for disable or enable cnn profile.\n");
    }

    return count;
}
#endif

/*
@brief Set debugfs profile data for network run
*/
vip_status_e gckvip_debug_set_profile_data(
    vip_uint64_t time,
    gckvip_profile_dtype_t dtype
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t cur_total = 0;
    vip_uint64_t cur_infer = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    if (VIP_NULL == core_loading)
	    return status;

    switch (dtype) {
    case PROFILE_INIT:
    {
        kdriver->profile_data.init_time = time;
        kdriver->profile_data.infer_time = 0;
        kdriver->profile_data.total_time = 0;
    }
    break;

    case PROFILE_SUBMIT:
    {
        kdriver->profile_data.submit_time = time;
	core_loading[0].latest_sbumit_time = time;
    }
    break;

    case PROFILE_WAIT:
    {
        kdriver->profile_data.wait_time = time;
        cur_total = kdriver->profile_data.wait_time - kdriver->profile_data.init_time;
        cur_infer = kdriver->profile_data.wait_time - kdriver->profile_data.submit_time;
        kdriver->profile_data.total_time += cur_total;
        kdriver->profile_data.infer_time += cur_infer;
	core_loading[0].infer_time += cur_infer;
    }
    break;

    default:
    break;
    }

    return status;
}

/*
@brief return the profile result.
*/
gckvip_profile_t gckvip_debug_get_profile_data(void)
{
    gckvip_profile_t profile_result;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    profile_result.total_time = kdriver->profile_data.total_time;
    profile_result.infer_time = kdriver->profile_data.infer_time;
    return profile_result;
}

vip_status_e gckvip_debug_videomemory_profile_alloc(
    vip_uint32_t size
    )
{
    gckvip_driver_t *kdriver;

    vip_status_e status = VIP_SUCCESS;
    kdriver = gckvip_get_kdriver();

    kdriver->profile_data.video_memory += size;

    if (kdriver->profile_data.video_memory > kdriver->profile_data.video_peak) {
        kdriver->profile_data.video_peak = kdriver->profile_data.video_memory;
    }
    kdriver->profile_data.video_allocs++;

    return status;
}

vip_status_e gckvip_debug_videomemory_profile_free(
    void *handle
    )
{
    gckvip_driver_t *kdriver;
    gckvip_video_mem_handle_t *ptr = (gckvip_video_mem_handle_t*)handle;
    vip_status_e status = VIP_SUCCESS;
    kdriver = gckvip_get_kdriver();

    if (handle != VIP_NULL) {
    #if vpmdENABLE_MMU
        if (GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP & ptr->memory_type) {
            #if vpmdENABLE_VIDEO_MEMORY_HEAP
            gckvip_context_t *context = gckvip_get_context();
            gckvip_database_table_t *table = VIP_NULL;
            if (kdriver->profile_data.video_memory > ptr->node.heap_node.alloc_handle->size) {
                kdriver->profile_data.video_memory -= ptr->node.heap_node.alloc_handle->size;
            }
            else {
                vip_uint32_t i = 0;
                vip_uint32_t first_free_pos = 0;
                first_free_pos = gckvip_database_get_freepos(&context->videomem_database);
                for (i = 0; i < first_free_pos; i++) {
                    gckvip_database_get_table(&context->videomem_database, i + DATABASE_MAGIC_DATA, &table);
                    if (table->handle != VIP_NULL) {
                        break;
                    }
                }
                if (i  >= first_free_pos) {
                    /* all video memory free */
                    kdriver->profile_data.video_memory = 0;
                }
            }
            kdriver->profile_data.video_frees++;
            #else
            PRINTK_E("fail to profile free memory, video memory heap has been disable,"
                     "vpmdENABLE_VIDEO_MEMORY_HEAP=0\n");
            #endif
        }
        else {
            kdriver->profile_data.video_memory -= ptr->node.dyn_node.size;
            kdriver->profile_data.video_frees++;
        }
    #else
        #if vpmdENABLE_VIDEO_MEMORY_HEAP
        if (GCVIP_VIDEO_MEMORY_TYPE_VIDO_HEAP == ptr->memory_type) {
            kdriver->profile_data.video_memory -= ptr->node.heap_node.alloc_handle->size;
            kdriver->profile_data.video_frees++;
        }
        else if (GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC == ptr->memory_type) {
            kdriver->profile_data.video_memory -= ptr->node.dyn_node.size;
            kdriver->profile_data.video_frees++;
        }
        #else
        if (GCVIP_VIDEO_MEMORY_TYPE_DYN_ALLOC == ptr->memory_type) {
            kdriver->profile_data.video_memory -= ptr->node.dyn_node.size;
            kdriver->profile_data.video_frees++;
        }
        #endif
    #endif
    }
    else {
        /* profile failed, but not return failed status*/
        PRINTK_E("profile failed, mem free video memory parameter is NULL\n");
    }

    return status;
}

#if vpmdDUMP_NBG_RESOURCE
static ssize_t gckvip_rt_dump_nbg_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    if (count > 2) {
        PRINTK_E("please echo bytes no more than 2, echo 1 enable dump nbg, echo 0 disable dump nbg\n");
        return count;
    }
    ret  = simple_strtol(buf, NULL, 0);
    if (ret == 1) {
        kdriver->func_config.enable_dump_nbg = vip_true_e;
    }
    else if (ret == 0){
        kdriver->func_config.enable_dump_nbg = vip_false_e;
    }
    else {
        PRINTK("please set 0 or 1 value for disable or enable dump nbg.\n");
    }

    return count;
}
#endif

#if vpmdENABLE_MULTIPLE_VIP
static ssize_t gckvip_multi_vip_set(
    const char *buf,
    size_t count
    )
{
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    int device_core_number[gcdMAX_CORE] = {0};
    int index_buf = 0, core_num = 0, index_core = 0, sum_core = 0;
    vip_bool_e record = vip_false_e;
    gckvip_context_t *context = gckvip_get_context();

    if (0 < context->initialize) {
        PRINTK_E("vip is still working...\n");
        return count;
    }
    if (kdriver->core_count > gcdMAX_CORE) {
        PRINTK_E("fail core count=%d\n", kdriver->core_count);
        return count;
    }

    while(buf[index_buf]) {
        if ('0' <= buf[index_buf] && '9' >= buf[index_buf]) {
            core_num = 10 * core_num + buf[index_buf] - '0';
            record = vip_true_e;
        }
        else {
            if (record) {
                /* core number after 0 is invalid */
                /* {2,1,0,4,0,0,0,0}. indicate that driver supports 2 logical device */
                if (0 == core_num) break;

                if ((kdriver->core_count > index_core) || (index_core >= gcdMAX_CORE)) {
                    device_core_number[index_core++] = core_num;
                    sum_core += core_num;
                }
                else {
                    /* too many device */
                    PRINTK_E("device number should be less than %d\n", kdriver->core_count);
                    return count;
                }
            }
            record = vip_false_e;
            core_num = 0;
        }
        index_buf++;
    }

    if (0 >= sum_core) {
        /* 1 core at least */
        PRINTK_E("set 1 core at least\n");
    }
    else if (kdriver->core_count < sum_core) {
        /* too many core */
        PRINTK_E("total core number should be less than %d\n", kdriver->core_count);
    }
    else {
        for (index_core = 0; index_core < kdriver->core_count; index_core++) {
            kdriver->device_core_number[index_core] = device_core_number[index_core];
        }
    }

    return count;
}
#endif

#if vpmdREGISTER_NETWORK
static loff_t gckvip_rt_net_profile(
    void *m,
    void *data
    )
{
    gckvip_context_t *context = gckvip_get_context();
    loff_t len = 0, offset = 0;
    vip_uint32_t i = 0;
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    if (0 >= context->initialize) {
        FS_PRINTF(ptr, len, offset, "vip not working, fail to show network profile\n");
        return len;
    }

    FS_PRINTF(ptr, len, offset, "------------------------------------------------------------------"\
                                "---------------------------------------------------------------\n");
    FS_PRINTF(ptr, len, offset, "|  network handle          |  enable  |  network name             "\
                                "         |  infer count  |  avg infer time  |  avg cycle      |\n");
    for (i = 1; i < gckvip_hashmap_free_pos(&context->network_info); i++) {
        gckvip_network_info_t *net_info = VIP_NULL;
        gckvip_segment_info_t *seg_info = VIP_NULL;
        vip_uint32_t j = 0;
        vip_uint64_t time = 0;
        vip_uint32_t cycle = 0;
        #if !vpmdENABLE_CNN_PROFILING
        vip_uint32_t k = 0;
        #endif
        gckvip_hashmap_get_by_index(&context->network_info, i, (void**)&net_info);
        if (VIP_NULL == net_info) {
            PRINTK_E("fail to get network info for index=%d\n", i);
            continue;
        }
        if (VIP_NULL != net_info->segment_handle) {
            #if !vpmdENABLE_CNN_PROFILING
            for (j = 0; j < net_info->segment_count; j++) {
                gckvip_hashmap_get_by_handle(&context->segment_info, net_info->segment_handle[j],
                                             &k, (void**)&seg_info);
                if (0 < k) {
                    time += seg_info->avg_data.time;
                    cycle += (seg_info->avg_data.total_cycle - seg_info->avg_data.total_idle_cycle);
                    gckvip_hashmap_unuse(&context->segment_info, k, vip_false_e);
                }
            }
            #endif
            gckvip_hashmap_get_by_handle(&context->segment_info, net_info->segment_handle[0], &j, (void**)&seg_info);
            if (0 < j) {
                FS_PRINTF(ptr, len, offset, "------------------------------------------------------------------"\
                                            "---------------------------------------------------------------\n");
                FS_PRINTF(ptr, len, offset, "|  0x%"PRPx"      |  %d       |  %-32s  |  %-11d  |  %-12dus  |"
                          "  %-13d  |\n", seg_info->network_handle, !seg_info->skip, net_info->network_name,
                          seg_info->submit_count, time, cycle);
                gckvip_hashmap_unuse(&context->segment_info, j, vip_false_e);
            }
        }
        gckvip_hashmap_unuse(&context->network_info, i, vip_false_e);
    }
    FS_PRINTF(ptr, len, offset, "------------------------------------------------------------------"\
                                "---------------------------------------------------------------\n");

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return len;
}

static ssize_t gckvip_rt_net_profile_set(
    const char *buf,
    size_t count
    )
{
    gckvip_context_t *context = gckvip_get_context();
    void *handle = VIP_NULL;
    vip_uint64_t tmp = 0;
    gckvip_network_info_t* net_info = VIP_NULL;
    gckvip_segment_info_t* seg_info = VIP_NULL;
    vip_uint32_t i = 0;
    vip_char_t *usage = "echo [network handle] > rt_net_profile. (toggle the specifed network state)\n";

    if (0 >= context->initialize) {
        PRINTK_E("vip not working, fail to manage network\n");
        return count;
    }

    sscanf(buf, "0x%"PRIx64"", &tmp);
    handle = GCKVIPUINT64_TO_PTR(tmp);
    if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->network_info, handle, &i, (void**)&net_info)) {
        if (VIP_NULL != net_info->segment_handle) {
            vip_uint32_t j = 0, k = 0;
            for (j = 0; j < net_info->segment_count; j++) {
                if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->segment_info,
                    net_info->segment_handle[j], &k, (void**)&seg_info)) {
                    seg_info->skip = !seg_info->skip;
                    gckvip_hashmap_unuse(&context->segment_info, k, vip_false_e);
                }
            }
        }
        gckvip_hashmap_unuse(&context->network_info, i, vip_false_e);
    }
    else if (VIP_SUCCESS == gckvip_hashmap_get_by_handle(&context->segment_info, handle, &i, (void**)&seg_info)) {
        seg_info->skip = !seg_info->skip;
        gckvip_hashmap_unuse(&context->segment_info, i, vip_false_e);
    }
    else {
        PRINTK("did not find this network 0x%x\n", handle);
        PRINTK("usage: %s", usage);
    }

    return count;
}
#endif

#if vpmdUSE_DEBUG_FS
/*
@brief linux seq file operations.
*/
static void *gcvip_seq_start(
    struct seq_file *s,
    loff_t *pos
    )
{
    if(*pos > 0)
        return NULL;
    return pos;
}

static void *gcvip_seq_next(
    struct seq_file *s,
    void *v,
    loff_t *pos
    )
{
    return NULL;
}

static void gcvip_seq_stop(
    struct seq_file *s,
    void *v
    )
{
}

#define gcvip_debugfs_write(a)                                   \
static ssize_t gcvip_debugfs_##a##_write(struct file *file,      \
                                           const char __user *ubuf,\
                                           size_t count,           \
                                           loff_t *ppos)           \
{                                                                  \
    return count;                                                  \
}

gcvip_debugfs_write(vip_info)
gcvip_debugfs_write(mem_mapping)
#if vpmdENABLE_MMU
gcvip_debugfs_write(dump_mmu_table)
#endif
#if vpmdENABLE_HANG_DUMP
gcvip_debugfs_write(hang_dump)
#endif

static ssize_t gcvip_debugfs_core_loading_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (count > 128) {
        PRINTK_E("please echo bytes no more than 128, not reset time profile data.\n");
        return count;
    }

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail,not reset time profile data.\n");
    }

    gckvip_core_loading_set((const char *)&buf, count);

    return count;
}

/*
@brief show the time profile result
*/
static int gcvip_seq_core_loading_show(
    struct seq_file *s,
    void *v
    )
{
    return gckvip_core_loading_get((void *)s, VIP_NULL);
}

/*
@brief show the viplite info profile result
*/
static int gcvip_seq_vip_info_show(
    struct seq_file *s,
    void *v
    )
{
    return gckvip_vipinfo_show((void *)s, VIP_NULL);
}

static ssize_t gcvip_debugfs_mem_profile_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (count > 128) {
        PRINTK_E("please echo bytes no more than 128, not reset memory profile data.\n");
        return count;
    }

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail,not reset memory profile data.\n");
    }

    gckvip_mem_profile_set((const char*)&buf, VIP_NULL);

    return count;
}

static int gcvip_seq_mem_profile_show(
    struct seq_file *s,
    void *v
    )
{
    return gckvip_mem_profile((void*)s, VIP_NULL);
}

static ssize_t gcvip_debugfs_vip_freq_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail,not reset time profile data.\n");
    }

    gckvip_vipfreq_set((const char*)&buf, count);

    return count;
}

static int gcvip_seq_vip_freq_show(
    struct seq_file *s,
    void *v
    )
{
    return gckvip_vipfreq_show((void*)s, VIP_NULL);
}

static int gcvip_seq_mem_mapping_show(
    struct seq_file *s,
    void *v
    )
{
    ssize_t ret = 0;
    mutex_lock(&debugfs_mutex);
    ret = (ssize_t)gckvip_mem_mapping((void*)s, VIP_NULL);
    mutex_unlock(&debugfs_mutex);
    return ret;
}

#if vpmdENABLE_MMU
static int gcvip_seq_dump_mmu_table_show(
    struct seq_file *s,
    void *v
    )
{
    ssize_t ret = 0;
    mutex_lock(&debugfs_mutex);
    ret = (ssize_t)gckvip_dump_mmu_table((void*)s, VIP_NULL);
    mutex_unlock(&debugfs_mutex);
    return ret;
}
#endif
#if vpmdENABLE_HANG_DUMP
static int gcvip_seq_hang_dump_show(
    struct seq_file *s,
    void *v
    )
{
    ssize_t ret = 0;
    mutex_lock(&debugfs_mutex);
    ret = (ssize_t)gckvip_get_hang_dump((void*)s, VIP_NULL);
    mutex_unlock(&debugfs_mutex);
    return ret;
}
#endif
#if vpmdENABLE_SUSPEND_RESUME
static ssize_t gcvip_debugfs_suspend_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};
    gckvip_context_t *context = gckvip_get_context();
    if (0 == context->initialize) {
        PRINTK_E("vip not working, fail to set suspend\n");
        return count;
    }

    if (count > 2) {
        PRINTK_E("please echo bytes no more than 2, echo 1 suspend vip, echo 0 resume vip\n");
        return count;
    }

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail, not suspend/resume vip\n");
    }

    gckvip_suspend_set((const char*)&buf, count);

    return count;
}

static int gcvip_seq_suspend_show(
    struct seq_file *s,
    void *v
    )
{
    seq_printf(s, "suspend vip command: echo 1 > /sys/kernel/debug/viplite/suspend\n");
    seq_printf(s, "resume vip command: echo 0 > /sys/kernel/debug/viplite/suspend\n");
    return 0;
}
#endif
#if vpmdENABLE_DEBUG_LOG > 2
static ssize_t gcvip_debugfs_rt_log_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail,not modify rt_log status.\n");
    }

    gckvip_rt_log_set((const char*)&buf, count);

    return count;
}

static int gcvip_seq_rt_log_show(
    struct seq_file *s,
    void *v
    )
{
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    seq_printf(s, "rt_log=%d\n", kdriver->log_level);
    seq_printf(s, "  when rt_log=1, enable kernel info/debug log\n");
    seq_printf(s, "  when rt_log=0, disable kernel info/debug log\n");
    return 0;
}
#endif
#if vpmdENABLE_CAPTURE
static ssize_t gcvip_debugfs_rt_capture_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail,not modify capture status.\n");
    }

    gckvip_rt_capture_set((const char*)&buf, count);

    return count;
}

static int gcvip_seq_rt_capture_show(
    struct seq_file *s,
    void *v
    )
{
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    seq_printf(s, "  capture = %d\n", kdriver->func_config.enable_capture);
    seq_printf(s, "  when capture =1, enable user space capture function.\n");
    seq_printf(s, "  when capture =0, disable user space capture function.\n");

    return 0;
}
#endif

#if vpmdENABLE_CNN_PROFILING
static ssize_t gcvip_debugfs_rt_cnn_profile_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail,not modify cnn profile status.\n");
    }

    gckvip_cnn_profile_set((const char*)&buf, count);

    return count;
}

static int gcvip_seq_rt_cnn_profile_show(
    struct seq_file *s,
    void *v
    )
{
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    seq_printf(s, "  cnn_profile  = %d\n", kdriver->func_config.enable_cnn_profile);
    seq_printf(s, "  when cnn_profile =1, enable user space cnn profile function.\n");
    seq_printf(s, "  when cnn_profile =0, disable user space cnn profile function.\n");

    return 0;
}
#endif

#if vpmdDUMP_NBG_RESOURCE
static ssize_t gcvip_debugfs_rt_dump_nbg_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (count > 2) {
        PRINTK_E("please echo bytes no more than 2, echo 1 enable dump nbg, echo 0 disable dump nbg\n");
        return count;
    }

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail, not enable/disable dump nbg.\n");
    }

    gckvip_rt_dump_nbg_set((const char*)&buf, count);

    return count;
}

static int gcvip_seq_rt_dump_nbg_show(
    struct seq_file *s,
    void *v
    )
{
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    seq_printf(s, "dump_nbg = %d\n", kdriver->func_config.enable_dump_nbg);
    seq_printf(s, "enable dump nbg command: echo 1 > /sys/kernel/debug/viplite/dump_nbg\n");
    seq_printf(s, "disable dump nbg command: echo 0 > /sys/kernel/debug/viplite/dump_nbg\n");
    return 0;
}
#endif

#if vpmdENABLE_MULTIPLE_VIP
static ssize_t gcvip_debugfs_multi_vip_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail, fail to config multi-vip.\n");
    }

    gckvip_multi_vip_set((const char*)&buf, count);

    return count;
}

static int gcvip_seq_multi_vip_show(
    struct seq_file *s,
    void *v
    )
{
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    int i = 0;

    seq_printf(s, "current config: {");
    for (i = 0; i < kdriver->core_count; i++) {
        if (0 < i) {
            seq_printf(s, ", ");
        }
        seq_printf(s, "%d", kdriver->device_core_number[i]);
    }
    seq_printf(s, "}\n");
    seq_printf(s, "config multi-vip: echo {");
    for (i = 0; i < kdriver->core_count; i++) {
        if (0 < i) {
            seq_printf(s, ", ");
        }
        seq_printf(s, "1");
    }
    seq_printf(s, "} > /sys/kernel/debug/viplite/multi_vip\n");

    return 0;
}
#endif

#if vpmdREGISTER_NETWORK
static ssize_t gcvip_debugfs_rt_net_profile_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail, fail to manage network.\n");
    }

    gckvip_rt_net_profile_set((const char*)&buf, count);

    return count;
}

static int gcvip_seq_rt_net_profile_show(
    struct seq_file *s,
    void *v
    )
{
    ssize_t ret = 0;

    ret = (ssize_t)gckvip_rt_net_profile((void*)s, VIP_NULL);

    return ret;
}
#endif

static ssize_t gcvip_debugfs_register_rw_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {}, rw = 0;
    vip_uint32_t val = 0, address = 0, core_id = 0;
    gckvip_context_t *context = gckvip_get_context();
    gckvip_hardware_t *hardware;
    vip_status_e status = VIP_SUCCESS;
    vip_char_t *usage =
        "echo > [w --write] [core id] [register address] [write val] > /sys/kernel/debug/viplite/register_rw \n\
         echo > [r  --read] [core id] [register address] > /sys/kernel/debug/viplite/register_rw\n\
         eg: echo > r 0 0x30\n\
             echo > w 0 0x00 0x00070900";

    if (context->initialize == 0) {
        PRINTK("vip not working... .\n");
        return count;
    }

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail.\n");
    }

    if(sscanf(buf, "%c", &rw) == 1) {
        if (rw != 'W' && rw == 'w' && rw == 'R' && rw == 'r') {
            PRINTK_E("%s", usage);
            return count;
        }
    }

    hardware = gckvip_get_hardware(core_id);
    if (VIP_NULL == hardware) {
        PRINTK_E("fail get hardware object is NULL core-id=%d\n", core_id);
        return count;
    }

    if (rw == 'w' || rw == 'W') {
        if (sscanf(buf, "%c %ud %x %x", &rw, &core_id, &address, &val) == 4) {

            status = gckvip_os_write_reg(hardware->reg_base, address, val);
            if (status != VIP_SUCCESS) {
                PRINTK_E("write core%d address[0x%x] val[0x%x] failed.\n", core_id, address, val);
                return count;
            }
            PRINTK_E("write core[%d] address[0x%x] val[0x%x] success.\n", core_id, address, val);
        } else {
            PRINTK_E("read param error, usage:\n %s.\n", usage);
        }
    } else if (rw == 'r' || rw == 'R') {
        if(sscanf(buf, "%c %d %x", &rw, &core_id, &address) == 3) {
            status = gckvip_os_read_reg(hardware->reg_base, address, &register_value);
            if (status != VIP_SUCCESS) {
                PRINTK_E("read core%d address[0x%x] failed.\n", core_id, address);
                return count;
            }
            register_address = address;
            PRINTK_E("read core[%d] address[0x%x] = [0x%x] success.\n", core_id, address, register_value);
        } else {
            PRINTK_E("read param error, usage:\n %s.\n", usage);
        }
    } else {
        PRINTK_E("param error, usage:\n %s.\n", usage);
    }

    return count;
}

static int gcvip_seq_register_rw_show(
    struct seq_file *s,
    void *v
    )
{
    seq_printf(s, "register[0x%x] = 0x%x.\n", register_address, register_value);

    return 0;
}

#define seq_ops(a)                                             \
    static const struct seq_operations gcvip_seq_##a##_ops = { \
        .start = gcvip_seq_start,                              \
        .next = gcvip_seq_next,                                \
        .stop = gcvip_seq_stop,                                \
        .show = gcvip_seq_##a##_show,                          \
    };

#define seq_open(a)                                                             \
    static int gcvip_debugfs_##a##_open(struct inode *inode, struct file *file) \
    {                                                                           \
        return seq_open(file, &gcvip_seq_##a##_ops);                            \
    }

#define debugfs_ops(a)                                               \
    static const struct file_operations gcvip_debugfs_##a##_ops = {  \
        .owner = THIS_MODULE,                                        \
        .open = gcvip_debugfs_##a##_open,                            \
        .read = seq_read,                                            \
        .write = gcvip_debugfs_##a##_write,                          \
        .llseek = seq_lseek,                                         \
        .release = seq_release,                                      \
    };

#define unroll_debugfs_file(a) \
    seq_ops(a)                 \
    seq_open(a)                \
    debugfs_ops(a)

unroll_debugfs_file(core_loading)
unroll_debugfs_file(vip_info)
unroll_debugfs_file(mem_profile)
unroll_debugfs_file(mem_mapping)
#if vpmdENABLE_MMU
unroll_debugfs_file(dump_mmu_table)
#endif
#if vpmdENABLE_HANG_DUMP
unroll_debugfs_file(hang_dump)
#endif
unroll_debugfs_file(vip_freq)
#if vpmdENABLE_SUSPEND_RESUME
unroll_debugfs_file(suspend)
#endif
#if vpmdENABLE_DEBUG_LOG > 2
unroll_debugfs_file(rt_log)
#endif
#if vpmdENABLE_CAPTURE
unroll_debugfs_file(rt_capture)
#endif
#if vpmdENABLE_CNN_PROFILING
unroll_debugfs_file(rt_cnn_profile)
#endif
#if vpmdDUMP_NBG_RESOURCE
unroll_debugfs_file(rt_dump_nbg)
#endif
#if vpmdENABLE_MULTIPLE_VIP
unroll_debugfs_file(multi_vip)
#endif
unroll_debugfs_file(register_rw)
#if vpmdREGISTER_NETWORK
unroll_debugfs_file(rt_net_profile)
#endif
/*
@brief Creat debugfs profile dir and file
*/
vip_status_e gckvip_debug_create_debugfs(void)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    kdriver->debugfs_parent = debugfs_create_dir("viplite", NULL);
    if (!kdriver->debugfs_parent) {
        PRINTK("create viplite debugfs dir failed.\n");
        status = VIP_ERROR_FAILURE;
        return status;
    }

    debugfs_create_file("vip_info", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_vip_info_ops);
    debugfs_create_file("vip_freq", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_vip_freq_ops);
    debugfs_create_file("core_loading", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_core_loading_ops);
    debugfs_create_file("mem_profile", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_mem_profile_ops);
    debugfs_create_file("mem_mapping", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_mem_mapping_ops);
    debugfs_create_file("register_rw", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_register_rw_ops);
#if vpmdENABLE_DEBUG_LOG > 2
    debugfs_create_file("rt_log", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_rt_log_ops);
#endif
#if vpmdENABLE_CAPTURE
    debugfs_create_file("rt_capture", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_rt_capture_ops);
#endif
#if vpmdENABLE_CNN_PROFILING
    debugfs_create_file("rt_cnn_profile", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_rt_cnn_profile_ops);
#endif
#if vpmdDUMP_NBG_RESOURCE
    debugfs_create_file("rt_dump_nbg", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_rt_dump_nbg_ops);
#endif
#if vpmdENABLE_MULTIPLE_VIP
    debugfs_create_file("multi_vip", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_multi_vip_ops);
#endif
#if vpmdENABLE_MMU
    debugfs_create_file("dump_mmu_table", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_dump_mmu_table_ops);
#endif
#if vpmdENABLE_HANG_DUMP
    debugfs_create_file("hang_dump", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_hang_dump_ops);
#endif
#if vpmdENABLE_SUSPEND_RESUME
    debugfs_create_file("suspend", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_suspend_ops);
#endif
#if vpmdREGISTER_NETWORK
    debugfs_create_file("rt_net_profile", 0644, kdriver->debugfs_parent, NULL, &gcvip_debugfs_rt_net_profile_ops);
#endif
    return status;
}

/*
@brief Destroy debugfs profile dir and file
*/
vip_status_e gckvip_debug_destroy_debugfs(void)
{
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    if (kdriver->debugfs_parent != VIP_NULL) {
        debugfs_remove_recursive(kdriver->debugfs_parent);
        kdriver->debugfs_parent = VIP_NULL;
    }
    return VIP_SUCCESS;
}


#undef gcvip_debugfs_write
#undef seq_ops
#undef seq_open
#undef debugfs_ops
#undef undef_debugfs_file

#else
static ssize_t vip_info_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    sys_param param;
    param.buf = buf;
    param.offset = off;
    len = (loff_t)gckvip_vipinfo_show((void*)(&param), VIP_NULL);

    return (len >= off ? len - off : 0);
}

static ssize_t vip_freq_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    sys_param param;
    param.buf = buf;
    param.offset = off;
    len = (loff_t)gckvip_vipfreq_show((void*)(&param), VIP_NULL);

    return (len >= off ? len - off : 0);
}

static ssize_t vip_freq_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return gckvip_vipfreq_set(buf, count);
}

static ssize_t core_loading_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    sys_param param;
    param.buf = buf;
    param.offset = off;
    len = (loff_t)gckvip_core_loading_get((void *)(&param), VIP_NULL);

    return (len >= off ? len - off : 0);
}
static ssize_t core_loading_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return gckvip_core_loading_set(buf, count);
}

static ssize_t mem_profile_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    sys_param param;
    param.buf = buf;
    param.offset = off;
    len = (loff_t)gckvip_mem_profile((void *)(&param), VIP_NULL);

    return (len >= off ? len - off : 0);
}
static ssize_t mem_profile_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    ssize_t ret = 0;
    mutex_lock(&debugfs_mutex);
    ret = (ssize_t)gckvip_mem_profile_set(buf, count);
    mutex_unlock(&debugfs_mutex);
    return ret;
}

static ssize_t mem_mapping_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    sys_param param;
    param.buf = buf;
    param.offset = off;
    if (0 == off) mutex_lock(&debugfs_mutex);
    len = (loff_t)gckvip_mem_mapping((void*)(&param), VIP_NULL);
    if (off >= len) mutex_unlock(&debugfs_mutex);

    return (len >= off ? len - off : 0);
}

#if vpmdENABLE_MMU
static ssize_t dump_mmu_table_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    sys_param param;
    param.buf = buf;
    param.offset = off;
    if (0 == off) mutex_lock(&debugfs_mutex);
    len = (loff_t)gckvip_dump_mmu_table((void*)(&param), VIP_NULL);
    if (off >= len) mutex_unlock(&debugfs_mutex);

    return (len >= off ? len - off : 0);
}
#endif

#if vpmdENABLE_HANG_DUMP
static ssize_t hang_dump_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    sys_param param;
    param.buf = buf;
    param.offset = off;
    if (0 == off) mutex_lock(&debugfs_mutex);
    len = (loff_t)gckvip_get_hang_dump((void*)(&param), VIP_NULL);
    if (off >= len) mutex_unlock(&debugfs_mutex);

    return (len >= off ? len - off : 0);
}
#endif

#if vpmdENABLE_SUSPEND_RESUME
static ssize_t suspend_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    len += fs_printf(buf, "suspend vip command: echo 1 > /sys/devices/platform/xxx/suspend\n");
    len += fs_printf(buf + len, "resume vip command: echo 0 > /sys/devices/platform/xxx/suspend\n");
    return (len >= off ? len - off : 0);
}
static ssize_t suspend_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    gckvip_context_t *context = gckvip_get_context();
    if (0 == context->initialize) {
        PRINTK_E("vip not working, fail to set suspend\n");
        return count;
    }
    return gckvip_suspend_set(buf, count);
}
#endif
#if vpmdENABLE_DEBUG_LOG > 2
static ssize_t rt_log_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    len += fs_printf(buf, "rt_log = %d.\n", kdriver->log_level);
    return (len >= off ? len - off : 0);
}
static ssize_t rt_log_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return gckvip_rt_log_set(buf, count);
}
#endif
#if vpmdENABLE_CAPTURE
static ssize_t rt_capture_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    len += fs_printf(buf, "capture = %d.\n", kdriver->func_config.enable_capture);
    return (len >= off ? len - off : 0);
}
static ssize_t rt_capture_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return gckvip_capture_set(buf, count);
}
#endif

#if vpmdENABLE_CNN_PROFILING
static ssize_t rt_cnn_profile_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    len += fs_printf(buf, "rt_cnn_profile = %d.\n", kdriver->func_config.enable_cnn_profile);
    return (len >= off ? len - off : 0);
}
static ssize_t rt_cnn_profile_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return gckvip_cnn_profile_set(buf, count);
}
#endif

#if vpmdDUMP_NBG_RESOURCE
static ssize_t rt_dump_nbg_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    len += fs_printf(buf, "dump_nbg = %d.\n", kdriver->func_config.enable_dump_nbg);
    len += fs_printf(buf + len, "enable dump nbg command: echo 1 > /sys/kernel/debug/viplite/dump_nbg\n");
    len += fs_printf(buf + len, "disable dump nbg command: echo 0 > /sys/kernel/debug/viplite/dump_nbg\n");
    return (len >= off ? len - off : 0);
}
static ssize_t rt_dump_nbg_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return gckvip_rt_dump_nbg_set(buf, count);
}
#endif

#if vpmdENABLE_MULTIPLE_VIP
static ssize_t multi_vip_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0, i = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();
    len += fs_printf(buf, "current config: {");
    for (i = 0; i < kdriver->core_count; i++) {
        if (0 < i) {
            len += fs_printf(buf + len, ", ");
        }
        len += fs_printf(buf + len, "%d", kdriver->device_core_number[i]);
    }
    len += fs_printf(buf + len, "}\n");
    len += fs_printf(buf + len, "config multi-vip: echo {");
    for (i = 0; i < kdriver->core_count; i++) {
        if (0 < i) {
            len += fs_printf(buf + len, ", ");
        }
        len += fs_printf(buf + len, "1");
    }
    len += fs_printf(buf + len, "} > /sys/devices/platform/xxx/multi_vip\n");
    return (len >= off ? len - off : 0);
}
static ssize_t multi_vip_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return gckvip_multi_vip_set(buf, count);
}
#endif

#if vpmdREGISTER_NETWORK
static ssize_t rt_net_profile_show(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    loff_t len = 0;
    sys_param param;
    param.buf = buf;
    param.offset = off;
    len = (loff_t)gckvip_rt_net_profile((void*)(&param), VIP_NULL);

    return (len >= off ? len - off : 0);
}
static ssize_t rt_net_profile_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return gckvip_rt_net_profile_set(buf, count);
}
#endif

#define sysfs_RW(a)                                        \
    static const struct bin_attribute sysfs_##a##_attr = { \
        .attr = {                                          \
        .name = #a,                                        \
            .mode = S_IRUGO | S_IWUSR,                     \
        },                                                 \
        .read = a##_show,                                  \
        .write = a##_store                                 \
    };

#define sysfs_RO(a)                                        \
    static const struct bin_attribute sysfs_##a##_attr = { \
        .attr = {                                          \
        .name = #a,                                        \
            .mode = S_IRUGO,                               \
        },                                                 \
        .read = a##_show                                   \
    };

sysfs_RO(vip_info);
sysfs_RW(vip_freq);
sysfs_RW(core_loading);
sysfs_RW(mem_profile);
sysfs_RO(mem_mapping);
#if vpmdENABLE_MMU
sysfs_RO(dump_mmu_table);
#endif
#if vpmdENABLE_HANG_DUMP
sysfs_RO(hang_dump);
#endif
#if vpmdENABLE_SUSPEND_RESUME
sysfs_RW(suspend);
#endif
#if vpmdENABLE_DEBUG_LOG > 2
sysfs_RW(rt_log);
#endif
#if vpmdENABLE_CAPTURE
sysfs_RW(rt_capture);
#endif
#if vpmdENABLE_CNN_PROFILING
sysfs_RW(rt_cnn_profile);
#endif
#if vpmdDUMP_NBG_RESOURCE
sysfs_RW(rt_dump_nbg);
#endif
#if vpmdENABLE_MULTIPLE_VIP
sysfs_RW(multi_vip);
#endif
#if vpmdREGISTER_NETWORK
sysfs_RW(rt_net_profile);
#endif

vip_status_e gckvip_debug_create_sysfs(void)
{
    vip_status_e status = VIP_SUCCESS;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_vip_info_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_vip_freq_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_core_loading_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_mem_profile_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_mem_mapping_attr);
#if vpmdENABLE_MMU
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_dump_mmu_table_attr);
#endif
#if vpmdENABLE_HANG_DUMP
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_hang_dump_attr);
#endif
#if vpmdENABLE_SUSPEND_RESUME
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_suspend_attr);
#endif
#if vpmdENABLE_DEBUG_LOG > 2
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_rt_log_attr);
#endif
#if vpmdENABLE_CAPTURE
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_rt_capture_attr);
#endif
#if vpmdENABLE_CNN_PROFILING
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_rt_cnn_profile_attr);
#endif
#if vpmdDUMP_NBG_RESOURCE
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_rt_dump_nbg_attr);
#endif
#if vpmdENABLE_MULTIPLE_VIP
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_multi_vip_attr);
#endif
#if vpmdREGISTER_NETWORK
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_rt_net_profile_attr);
#endif

    return status;
}

vip_status_e gckvip_debug_destroy_sysfs(void)
{
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_vip_info_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_vip_freq_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_core_loading_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_mem_profile_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_mem_mapping_attr);
#if vpmdENABLE_MMU
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_dump_mmu_table_attr);
#endif
#if vpmdENABLE_HANG_DUMP
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_hang_dump_attr);
#endif
#if vpmdENABLE_SUSPEND_RESUME
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_suspend_attr);
#endif
#if vpmdENABLE_DEBUG_LOG > 2
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_rt_log_attr);
#endif
#if vpmdENABLE_CAPTURE
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_rt_capture_attr);
#endif
#if vpmdENABLE_CNN_PROFILING
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_rt_cnn_profile_attr);
#endif
#if vpmdDUMP_NBG_RESOURCE
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_rt_dump_nbg_attr);
#endif
#if vpmdENABLE_MULTIPLE_VIP
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_multi_vip_attr);
#endif
#if vpmdREGISTER_NETWORK
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_rt_net_profile_attr);
#endif

    return VIP_SUCCESS;
}
#endif

vip_status_e  gckvip_debug_create_fs(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t size = 0;
    gckvip_driver_t *kdriver = gckvip_get_kdriver();

    if (kdriver->core_count > 0) {
	    debugfs_core_cnt = kdriver->core_count;
    } else {
	    debugfs_core_cnt = gcdMAX_CORE;
    }
    size = debugfs_core_cnt * sizeof(gckvip_core_loading_profile_t);
    gckvip_os_allocate_memory(size, (void **)&core_loading);
    gckvip_os_zero_memory((void *)core_loading, size);
    PRINTK_D("debugfs core cnt=%d\n", debugfs_core_cnt);

#if vpmdUSE_DEBUG_FS
    status = gckvip_debug_create_debugfs();
#else
    status = gckvip_debug_create_sysfs();
#endif

    return status;
}

vip_status_e gckvip_debug_profile_start(void)
{
    vip_uint32_t i = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t time = gckvip_os_get_time();
    if (VIP_NULL == core_loading)
	    return status;

    for (i = 0; i < debugfs_core_cnt; i++) {
	    core_loading[i].init_time = time;
	    core_loading[i].destory_time = 0;
	    core_loading[i].submit_time = 0;
	    core_loading[i].infer_time = 0;
	    core_loading[i].last_record_time = time;
	    core_loading[i].last_infer_time = 0;
	    core_loading[i].latest_sbumit_time = 0;
    }

    return status;
}

vip_status_e gckvip_debug_profile_end(void)
{
    vip_uint32_t i = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t time = gckvip_os_get_time();
    if (VIP_NULL == core_loading)
	    return status;

    for (i = 0; i < debugfs_core_cnt; i++) {
	    core_loading[i].destory_time = time;
    }

	return status;
 }

vip_status_e  gckvip_debug_destroy_fs(void)
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdUSE_DEBUG_FS
    status = gckvip_debug_destroy_debugfs();
#else
    status = gckvip_debug_destroy_sysfs();
#endif

    if (core_loading != VIP_NULL) {
	    gckvip_os_free_memory((void *)core_loading);
	    core_loading = VIP_NULL;
    }

    return status;
}

#endif

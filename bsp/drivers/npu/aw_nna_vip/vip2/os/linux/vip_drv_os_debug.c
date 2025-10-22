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
#include <vip_lite_config.h>
#if vpmdENABLE_DEBUGFS
#include <linux/string.h>
#include "vip_drv_os_debug.h"
#include "vip_drv_device_driver.h"
#include <vip_drv_interface.h>
#include <vip_drv_hardware.h>
#include <vip_drv_os_port.h>
#include <vip_drv_video_memory.h>
#include <vip_drv_mem_allocator.h>
#include <vip_drv_debug.h>
#include <vip_drv_mmu.h>
#include <vip_drv_context.h>
#include <vip_drv_task_common.h>
#include <vip_drv_task_descriptor.h>
#include <vip_lite_version.h>

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
struct dentry    *debugfs_parent;
vipdrv_video_mem_profile_t video_mem_data;
vip_uint32_t debugfs_core_cnt = 0;
vipdrv_core_loading_profile_t *core_loading = VIP_NULL;
#endif


#if vpmdUSE_DEBUG_FS
static vip_uint32_t register_address = 0x0, register_value = 0x0;
#endif

static DEFINE_MUTEX(debugfs_mutex);

#define CHECK_CONTEXT_INITIALIZE()  \
{   \
    if (0 == *(vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED)) { \
        len += fs_printf(p + len, "vip not working...\n"); \
        return len;   \
    }   \
}

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
static loff_t vipdrv_vipinfo_show(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vip_bool_e need_init = vip_true_e;
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t chip_ver1, chip_ver2, chip_cid, chip_date;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
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
        vipdrv_initialize_t init_data;
        init_data.version = ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_SUB_MINOR));
        status = vipdrv_init(&init_data);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to init hardware\n");
            mutex_unlock(&debugfs_mutex);
            return 0;
        }
    }

#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_lock_mutex(context->initialize_mutex);
    }
#endif

    VIPDRV_LOOP_HARDWARE_START
    #if vpmdENABLE_POWER_MANAGEMENT
    status = vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
    if (VIPDRV_POWER_CLOCK & vipdrv_pm_query_state(hw)) {
    #endif
        status = vipdrv_os_read_reg(hw->reg_base, 0x00020, &chip_ver1);
        if (status != VIP_SUCCESS) {
            PRINTK_E("viplite read chip ver1 faild, error = %d.\n", status);
        }

        status = vipdrv_os_read_reg(hw->reg_base, 0x00024, &chip_ver2);
        if (status != VIP_SUCCESS) {
            PRINTK_E("viplite read chip ver2 faild, error = %d.\n", status);
        }

        status = vipdrv_os_read_reg(hw->reg_base, 0x00030, &chip_cid);
        if (status != VIP_SUCCESS) {
            PRINTK_E("viplite read chip cid faild, error = %d.\n", status);
        }

        status = vipdrv_os_read_reg(hw->reg_base, 0x00028, &chip_date);
        if (status != VIP_SUCCESS) {
            PRINTK_E("viplite read chip date faild, error = %d.\n", status);
        }

        printk("dev%d hw%d info: pid=0x%x, date=0x%x, ver1=0x%x, ver2=0x%x.\n",
                dev_index, hw->core_index, chip_cid, chip_date, chip_ver1, chip_ver2);

        FS_PRINTF(ptr, len, offset, "dev%d hw%d info: pid=0x%x, date=0x%x, ver1=0x%x, ver2=0x%x.\n",
                dev_index, hw->core_index, chip_cid, chip_date, chip_ver1, chip_ver2);
    #if vpmdENABLE_POWER_MANAGEMENT
    }
    else {
        FS_PRINTF(ptr, len, offset, "dev%d hw%d not working, may vip has power off, can't read register\n", \
               dev_index, hw->core_index);
    }
    status = vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
    #endif
    VIPDRV_LOOP_HARDWARE_END

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
#if vpmdENABLE_MULTIPLE_TASK
    if (context->initialize_mutex != VIP_NULL) {
        vipdrv_os_unlock_mutex(context->initialize_mutex);
    }
#endif
    if (vip_true_e == need_init) {
        vipdrv_destroy();
    }

    mutex_unlock(&debugfs_mutex);

    return len;
}

#if IS_ENABLED(CONFIG_NPU_SET_CLK_VOL)
static loff_t vipdrv_test_volregul_show(
	void *m,
	void *data)
{
	loff_t len = 0, offset = 0;
    u32 vol;
#if vpmdUSE_DEBUG_FS
	struct seq_file *ptr = (struct seq_file *)m;
#else
	sys_param *param = (sys_param *)m;
	char *ptr = param->buf;
	offset = param->offset;
#endif

	mutex_lock(&debugfs_mutex);
	vol = vipdrv_drv_get_vol_regul();

	FS_PRINTF(ptr, len, offset, "supported voltage:\n");
	FS_PRINTF(ptr, len, offset, "%u  <---- current vol\n", vol);

	mutex_unlock(&debugfs_mutex);
	return len;
}

static loff_t vipdrv_test_clkfreq_show(
	void *m,
	void *data)
{
	loff_t len = 0, offset = 0;
	u64 cur_freq = 0;

#if vpmdUSE_DEBUG_FS
	struct seq_file *ptr = (struct seq_file *)m;
#else
	sys_param *param = (sys_param *)m;
	char *ptr = param->buf;
	offset = param->offset;
#endif

	mutex_lock(&debugfs_mutex);
	cur_freq = vipdrv_drv_get_clk_freq();

	FS_PRINTF(ptr, len, offset, "supported frequencies:\n");
	FS_PRINTF(ptr, len, offset, "%16u (Hz) <---- current freq\n", cur_freq);

	mutex_unlock(&debugfs_mutex);
	return len;
}

#else
static loff_t vipdrv_clkfreq_show(
	void *m,
	void *data)
{
	loff_t len = 0, offset = 0;
	int count = 0;
	u64 freqs[MAX_FREQ_POINTS] = {0};
	int i;
	u64 cur_freq = 0;

#if vpmdUSE_DEBUG_FS
	struct seq_file *ptr = (struct seq_file *)m;
#else
	sys_param *param = (sys_param *)m;
	char *ptr = param->buf;
	offset = param->offset;
#endif

	mutex_lock(&debugfs_mutex);
	count = vipdrv_drv_get_supported_frequency(NULL, NULL, NULL, freqs, MAX_FREQ_POINTS);
	if (count <= 0) {
		printk("failed to get frequency list\n");
		goto out;
	}
	cur_freq = vipdrv_drv_get_clk_freq();

	FS_PRINTF(ptr, len, offset, "supported frequencies:\n");
	for (i = 0; (i < count && i < MAX_FREQ_POINTS); i++) {
		if (cur_freq == freqs[i]) {
			FS_PRINTF(ptr, len, offset, "%16u (Hz) <---- current freq\n", freqs[i]);
		} else {
			FS_PRINTF(ptr, len, offset, "%16u (Hz)\n", freqs[i]);
		}
	}
out:
	mutex_unlock(&debugfs_mutex);
	return len;
}
#endif

#if IS_ENABLED(CONFIG_NPU_SET_CLK_VOL)
static ssize_t vipdrv_test_clkfreq_set(
	const char *buf,
	size_t count
	)
{
	vip_uint64_t freq = 0;
	vip_uint32_t core = 0;

	vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
	freq = simple_strtoull(buf, NULL, 10);

	mutex_lock(&debugfs_mutex);

	// check is npu running here?
	for (core = 0; core < kdriver->core_count; core++) {
		if (kdriver->power_status[core] == VIPDRV_POWER_ON) {
			printk("NPU is running, unable to change freq\n");
			goto out;
		}
	}

	if (vipdrv_drv_update_clk_freq(freq))
		PRINTK_E("failed to update freq\n");
	else
		PRINTK("success to update to frequency %llu\n", freq);
out:
	mutex_unlock(&debugfs_mutex);
	return count;
}

static ssize_t vipdrv_test_volregul_set(
	const char *buf,
	size_t count,
    vipdrv_driver_t *kdriver,
    struct device *dev
	)
{
    long num = simple_strtol(buf, NULL, 10);
    int vol = (int)num;

    vipdrv_drv_update_vol_regul(vol);

    return count;
}
#else
static ssize_t vipdrv_clkfreq_set(
	const char *buf,
	size_t count
	)
{
	vip_uint64_t freq = 0;
	u64 freqs[MAX_FREQ_POINTS] = {0};
	int i, num = 0;
	bool find = false;
	vip_uint32_t core = 0;

	vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
	freq = simple_strtoull(buf, NULL, 10);

	mutex_lock(&debugfs_mutex);

	// check is npu running here?
	for (core = 0; core < kdriver->core_count; core++) {
		if (kdriver->power_status[core] == VIPDRV_POWER_ON) {
			printk("NPU is running, unable to change freq\n");
			goto out;
		}
	}

	num = vipdrv_drv_get_supported_frequency(NULL, NULL, NULL, freqs, MAX_FREQ_POINTS);
	if (num <= 0) {
		printk("failed to get frequency list\n");
		goto out;
	}

	for (i = 0; (i < num && i < MAX_FREQ_POINTS); i++) {
		if (freq == freqs[i]) {
			find = true;
			break;
		} else {
			continue;
		}
	}
	if (!find) {
		PRINTK_E("failed, please echo with supported frequency\n");
		goto out;
	}

	if (vipdrv_drv_update_clk_freq(freq))
		PRINTK_E("failed to update freq\n");
	else
		PRINTK("success to update to frequency %llu\n", freq);
out:
	mutex_unlock(&debugfs_mutex);
	return count;
}
#endif

static loff_t vipdrv_vipfreq_show(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vip_uint64_t mc_freq = 0, sh_freq = 0;
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    mutex_lock(&debugfs_mutex);

    if (context->initialize > 0) {
        #if vpmdENABLE_MULTIPLE_TASK
        if (context->initialize_mutex != VIP_NULL) {
            vipdrv_os_lock_mutex(context->initialize_mutex);
        }
        #endif

        VIPDRV_LOOP_HARDWARE_START
        #if vpmdENABLE_POWER_MANAGEMENT
        FS_PRINTF(ptr, len, offset, "vip frequency percent = %d\n", hw->core_fscale_percent);
        vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
        if (VIPDRV_POWER_CLOCK & vipdrv_pm_query_state(hw)) {
        #endif
            if (VIP_SUCCESS == vipdrv_get_clock(hw, &mc_freq, &sh_freq)) {
                FS_PRINTF(ptr, len, offset, "dev%d hw%d Core Frequency=%" PRId64" HZ\n",
                          dev_index, hw->core_index, mc_freq);
                FS_PRINTF(ptr, len, offset, "dev%d hw%d PPU  Frequency=%" PRId64" HZ\n",
                          dev_index, hw->core_index,sh_freq);
            }
        #if vpmdENABLE_POWER_MANAGEMENT
        }
        else {
            FS_PRINTF(ptr, len, offset, "vip %d not working, may vip has power off, can't read register\n", \
                   hw->core_id);
        }
        vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
        #endif
        VIPDRV_LOOP_HARDWARE_END

        #if vpmdENABLE_MULTIPLE_TASK
        if (context->initialize_mutex != VIP_NULL) {
            vipdrv_os_unlock_mutex(context->initialize_mutex);
        }
        #endif
    }
    else {
        vip_status_e status = VIP_SUCCESS;
        vipdrv_initialize_t init_data;
        init_data.version = ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_SUB_MINOR));
        status = vipdrv_init(&init_data);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to init hardware\n");
            mutex_unlock(&debugfs_mutex);
            return 0;
        }
        #if vpmdENABLE_MULTIPLE_TASK
        if (context->initialize_mutex != VIP_NULL) {
            vipdrv_os_lock_mutex(context->initialize_mutex);
        }
        #endif

        VIPDRV_LOOP_HARDWARE_START
        #if vpmdENABLE_POWER_MANAGEMENT
        status = vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
        if (VIPDRV_POWER_CLOCK & vipdrv_pm_query_state(hw)) {
        #endif
            if (VIP_SUCCESS == vipdrv_get_clock(hw, &mc_freq, &sh_freq)) {
                FS_PRINTF(ptr, len, offset, "dev%d hw%d Core Frequency=%" PRId64" HZ\n",
                          dev_index, hw->core_index, mc_freq);
                FS_PRINTF(ptr, len, offset, "dev%d hw%d PPU  Frequency=%" PRId64" HZ\n",
                          dev_index, hw->core_index,sh_freq);
            }
        #if vpmdENABLE_POWER_MANAGEMENT
        }
        else {
            FS_PRINTF(ptr, len, offset, "vip %d not working, may vip has power off, can't read register\n", \
                   hw->core_id);
        }
        status = vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
        #endif
        VIPDRV_LOOP_HARDWARE_END

        #if vpmdENABLE_MULTIPLE_TASK
        if (context->initialize_mutex != VIP_NULL) {
            vipdrv_os_unlock_mutex(context->initialize_mutex);
        }
        #endif
        vipdrv_destroy();
    }

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    mutex_unlock(&debugfs_mutex);

    return len;

}

static ssize_t vipdrv_vipfreq_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;

    vip_uint32_t* initialize = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED);
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

    mutex_lock(&debugfs_mutex);

    if (count > 128) {
        PRINTK_E("please echo bytes no more than 128, not reset time profile data.\n");
        mutex_unlock(&debugfs_mutex);
        return count;
    }
    ret  = simple_strtol(buf, NULL, 0);
    if ((ret > 0) && (ret <= 100)) {
        kdriver->core_fscale_percent = ret;
        PRINTK("set vip frequency percent=%d\n", ret);
        if (*initialize != 0) {
            #if vpmdENABLE_POWER_MANAGEMENT
            VIPDRV_LOOP_HARDWARE_START
            vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_FREQ, &ret);
            VIPDRV_LOOP_HARDWARE_END
            #endif
        }
    }
    else {
        PRINTK("please set 1~~100 value for vip frequency percent\n");
    }
    mutex_unlock(&debugfs_mutex);

    return count;
}

static loff_t vipdrv_pc_value_get(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vip_uint32_t i = 0;
    vipdrv_hardware_t* hardware = VIP_NULL;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    if (0 == context->initialize) {
        FS_PRINTF(ptr, len, offset, "vip not working, fail to get pc value\n");
        return 0;
    }

    VIPDRV_LOOP_DEVICE_START
    for (i = 0; i < device->hardware_count; i++) {
        hardware = vipdrv_get_hardware(device, i);
        VIPDRV_CHECK_NULL(hardware);
        if (vip_false_e == vipdrv_os_get_atomic(hardware->idle)) {
            vip_uint32_t dmaAddr = 0xFFFFF, dmaLo = 0, dmaHi = 0;

            #if vpmdENABLE_POWER_MANAGEMENT
            vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
            #endif

            vipdrv_read_register(hardware, 0x00668, &dmaLo);
            vipdrv_read_register(hardware, 0x0066C, &dmaHi);
            vipdrv_read_register(hardware, 0x00664, &dmaAddr);

            #if vpmdENABLE_POWER_MANAGEMENT
            vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
            #endif

            FS_PRINTF(ptr, len, offset, "dev%d hw%d DMA Address = 0x%08X\n", device->device_index,
                      hardware->core_index, dmaAddr);
            FS_PRINTF(ptr, len, offset, "   dmaLow   = 0x%08X\n", dmaLo);
            FS_PRINTF(ptr, len, offset, "   dmaHigh  = 0x%08X\n", dmaHi);
        }
    }
    VIPDRV_LOOP_DEVICE_END

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif

    return len;
}

typedef struct _vipdrv_core_load
{
    /* the number of task on this core */
    vip_uint32_t task_count;
    /* The time required to complete all tasks on this core */
    vip_uint64_t estimated_time;
    /* this core idle or busy */
    vip_bool_e   idle;
} vipdrv_core_load_t;

static loff_t vipdrv_core_loading_get(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vip_uint32_t ratio = 0, i = 0;
    vip_uint64_t total_time = 0;
#if vpmdTASK_QUEUE_USED
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
#endif
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    if (VIP_NULL == core_loading) {
        return 0;
    }

    if (0 == core_loading[0].destory_time) {
        total_time = vipdrv_os_get_time() - core_loading[0].init_time;
    }
    else {
        total_time = core_loading[0].destory_time - core_loading[0].init_time;
    }

    FS_PRINTF(ptr, len, offset, "VIP Total Time=%" PRId64" ms\n", total_time / 1000);
    for (i = 0; i < debugfs_core_cnt; i++) {
        ratio = (vip_uint32_t)((core_loading[i].infer_time * 100) / total_time);
        if (0 == i) {
            FS_PRINTF(ptr, len, offset, "History Loading -----> Core%d: Inference Time=%"PRId64" ms (%2d%%)\n",
                i, core_loading[i].infer_time / 1000, ratio);
        }
        else {
            FS_PRINTF(ptr, len, offset, "                       Core%d: Inference Time=%"PRId64" ms (%2d%%)\n",
                i, core_loading[i].infer_time / 1000, ratio);
        }
    }

#if vpmdTASK_QUEUE_USED
    if (context->initialize) {
        vip_uint32_t i = 0;
        vipdrv_core_load_t loads[vipdMAX_CORE];
        VIPDRV_LOOP_DEVICE_START
        vipdrv_queue_property_t *property = VIP_NULL;
        vip_uint32_t core_count = 0;
        vip_uint32_t start_core = 0;
        vip_uint32_t index = 0;
        vipdrv_hardware_t* hardware = VIP_NULL;
        vipdrv_os_zero_memory((void*)loads, sizeof(vipdrv_core_load_t) * vipdMAX_CORE);

        for (core_count = 1; core_count <= device->hardware_count; core_count++) {
            for (start_core = 0; start_core <= device->hardware_count - core_count; start_core++) {
                if (VIP_SUCCESS == vipdrv_queue_query_property(&device->tskTCB_queue, (void*)(&index), &property)) {
                    for (i = start_core; i < start_core + core_count; i++) {
                        loads[i].estimated_time += property->estimated_time;
                        loads[i].task_count += property->task_count;
                    }
                }
                index++;
            }
        }

        for (i = 0; i < device->hardware_count; i++) {
            hardware = vipdrv_get_hardware(device, i);
            VIPDRV_CHECK_NULL(hardware);
            loads[i].idle = vipdrv_os_get_atomic(hardware->idle);
        }
        VIPDRV_LOOP_DEVICE_END
        for (i = 0; i < debugfs_core_cnt; i++) {
            if (0 == i) {
                FS_PRINTF(ptr, len, offset,
                "Current Loading -----> Core%d: %s, Task Count=%d, Estimated Time=%"PRId64" us\n",
                i, vip_true_e == loads[i].idle ? "Idle" : "Busy",
                loads[i].task_count, loads[i].estimated_time);
            }
            else {
                FS_PRINTF(ptr, len, offset,
                  "                       Core%d: %s, Task Count=%d, Estimated Time=%"PRId64" us\n",
                 i, vip_true_e == loads[i].idle ? "Idle" : "Busy",
                 loads[i].task_count, loads[i].estimated_time);
            }
        }
    }
#endif

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return len;
}

static ssize_t vipdrv_core_loading_set(
    const char *buf,
    size_t count
    )
{
    vip_uint32_t ret;
    vip_uint32_t i = 0;
    if (VIP_NULL == core_loading) {
        return 0;
    }

    ret  = strncmp(buf, "reset", 5);

    if (!ret) {
        for (i = 0; i < debugfs_core_cnt; i++) {
            core_loading[i].init_time = vipdrv_os_get_time();
            core_loading[i].submit_time = 0;
            core_loading[i].infer_time = 0;
            core_loading[i].destory_time = 0;
        }
    }
    else {
        PRINTK_E("invalid param, please set 'reset' string to reset core loading data.\n");
    }

    return count;
}

static loff_t vipdrv_mem_profile(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    FS_PRINTF(ptr, len, offset, "video memory profile\n");
    FS_PRINTF(ptr, len, offset,
        " ------------------------------------------------------------------------------\n");
    FS_PRINTF(ptr, len, offset,
        "|       current(byte)    |      peak(byte)    |   alloc count   |   free count \n");
    FS_PRINTF(ptr, len, offset,
        "|------------------------------------------------------------------------------\n");
    FS_PRINTF(ptr, len, offset,
        "|      0x%08x        |     0x%08x     |      %d         |    %d     \n",
                video_mem_data.video_memory,
                video_mem_data.video_peak,
                video_mem_data.video_allocs,
                video_mem_data.video_frees);

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif

    return len;
}

static ssize_t vipdrv_mem_profile_set(
    const char *buf,
    size_t count
    )
{
    vip_uint32_t ret;

    ret = strncmp(buf, "reset", 5);
    if (!ret) {
        video_mem_data.video_memory = 0;
        video_mem_data.video_peak = 0;
        video_mem_data.video_allocs = 0;
        video_mem_data.video_frees = 0;
    }
    else {
        PRINTK_E("invalid param, please set reset to reset memory profile data.\n");
    }

  return count;
}

vip_status_e show_memory_mapping(
    vipdrv_video_mem_handle_t *ptr,
    void *m,
    vip_uint32_t index,
    vip_bool_e is_heap_alloc,
    loff_t *len,
    loff_t offset
    )
{
    vip_uint32_t iter = 0;
    vip_char_t *alloc_type = VIP_NULL;
    vipdrv_mem_mapping_profile_t *memory_mapping = VIP_NULL;
    vip_uint8_t *kernel_logical = VIP_NULL;
    vip_uint8_t *user_logical = VIP_NULL;
#if vpmdUSE_DEBUG_FS
    struct seq_file *p = (struct seq_file*)m;
#else
    char *p = (char*)m;
#endif
    vip_status_e status = VIP_SUCCESS;

    memory_mapping = kmalloc(sizeof(vipdrv_mem_mapping_profile_t), GFP_KERNEL);
    if (memory_mapping == NULL) {
        PRINTK("memory mapping kmalloc memory failed.\n");
        return VIP_ERROR_FAILURE;
    }
    memset(memory_mapping, 0, sizeof(vipdrv_mem_mapping_profile_t));

    if (ptr->allocator_type & VIPDRV_ALLOCATOR_TYPE_WRAP_DMA_BUF) {
        alloc_type = "  dma_buf ";
    }
    else if (ptr->allocator_type & VIPDRV_ALLOCATOR_TYPE_DYN_ALLOC) {
        alloc_type = "dyna_alloc";
    }
    else if (ptr->allocator_type & VIPDRV_ALLOCATOR_TYPE_WRAP_USER_LOGICAL) {
        alloc_type = "wrap_logic";
    }
    else if (ptr->allocator_type & VIPDRV_ALLOCATOR_TYPE_WRAP_USER_PHYSICAL) {
        alloc_type = "wrap_physic";
    }

    if (!is_heap_alloc) {
        memory_mapping->alloc_type = alloc_type;
        memory_mapping->size_table = ptr->size_table;
        memory_mapping->virtual_addr = ptr->vip_address;
        memory_mapping->physical_table = ptr->physical_table;
        memory_mapping->kernel_logical = ptr->kerl_logical;
        memory_mapping->user_logical = ptr->user_logical;
        memory_mapping->physical_num = ptr->physical_num;
        for (iter = 0; iter < memory_mapping->physical_num; iter++) {
            if (memory_mapping->kernel_logical != VIP_NULL) {
                kernel_logical = memory_mapping->kernel_logical +
                    iter * memory_mapping->size_table[iter];
            }
            else {
                kernel_logical = VIP_NULL;
            }
            if (memory_mapping->user_logical != VIP_NULL) {
                user_logical = memory_mapping->user_logical +
                    iter * memory_mapping->size_table[iter];
            }
            else {
                user_logical = VIP_NULL;
            }

            if (0 == iter) {
                FS_PRINTF(p, *(len), offset, "|   %08d   |  %s  |  0x%08x |    0x%" PRIx64"   "\
                                               "|   0x%08x  |    0x%"PRPx"  |     0x%"PRPx" |\n",
                index, memory_mapping->alloc_type, memory_mapping->size_table[iter],
                memory_mapping->physical_table[iter],
                memory_mapping->virtual_addr + iter * memory_mapping->size_table[iter],
                kernel_logical, user_logical);
            }
            else {
                FS_PRINTF(p, *(len), offset, "            %08d          |  0x%08x |    0x%" PRIx64"   "\
                                               "|   0x%08x  |    0x%"PRPx"  |     0x%"PRPx" |\n",
                 iter, memory_mapping->size_table[iter], memory_mapping->physical_table[iter],
                 memory_mapping->virtual_addr + iter * memory_mapping->size_table[iter],
                 kernel_logical, user_logical);
            }

            FS_PRINTF(p, *(len), offset, "|-----------------------------------------------"\
                "----------------------------------------------------------------------------|\n");
        }
    }
    else {
        memory_mapping->alloc_type = "heap_alloc";
#if vpmdENABLE_VIDEO_MEMORY_HEAP
        memory_mapping->size = ptr->size;
        memory_mapping->virtual_addr = ptr->vip_address;
        memory_mapping->physical = ptr->physical_table[0];
        memory_mapping->kernel_logical = ptr->kerl_logical;
        memory_mapping->user_logical = ptr->user_logical;
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
static loff_t vipdrv_mem_mapping(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vip_uint32_t left_mem_count = 0;
    vipdrv_video_mem_handle_t *pointer = VIP_NULL;
    vip_bool_e is_heap_alloc = vip_false_e, heap_not_profile = vip_true_e;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vip_uint32_t i = 0;
    vipdrv_mem_control_block_t *mcb = VIP_NULL;
#if vpmdUSE_DEBUG_FS
    struct seq_file *p = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* p = param->buf;
    offset = param->offset;
#endif

    CHECK_CONTEXT_INITIALIZE();
    FS_PRINTF(p, len, offset, " start profiling...\n");
    FS_PRINTF(p, len, offset,
        " ------------------------------------------------------------------------------"  \
                    "--------------------------------------------- \n");
    FS_PRINTF(p, len, offset,
        "|    index    |  alloc_type  |     size    |     physical    |      virtual   |    "  \
                    "   kerl_logical     |        user_logical    |\n");
    FS_PRINTF(p, len, offset,
        "|------------------------------------------------------------------------------"  \
                    "---------------------------------------------|\n");

    for (i = 1; i < vipdrv_database_free_pos(&context->mem_database); i++) {
        vipdrv_database_use(&context->mem_database, i, (void**)&mcb);
        if (VIP_NULL != mcb) {
            if (VIP_NULL != mcb->handle) {
                pointer = mcb->handle;
                is_heap_alloc =
                (pointer->allocator_type == VIPDRV_ALLOCATOR_TYPE_VIDO_HEAP) ? vip_true_e : vip_false_e;
                if (is_heap_alloc) {
                    if (heap_not_profile) {
                        show_memory_mapping(pointer, (void*)p, i, vip_true_e, &len, offset);
                        heap_not_profile = vip_false_e;
                        left_mem_count++;
                    }
                    else {
                        vipdrv_database_unuse(&context->mem_database, i);
                        continue;
                    }
                }
                else {
                    show_memory_mapping(pointer, (void*)p, i, vip_false_e, &len, offset);
                    left_mem_count++;
                }
            }
            vipdrv_database_unuse(&context->mem_database, i);
        }
    }
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    if (left_mem_count < 1) {
        len += fs_printf(p + len, "all the video memory have freed, not need profile.\n");
        return len;
    }

    return len;
}

#if vpmdENABLE_MMU
static vip_status_e vipdrv_debug_dump_data(
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

static vip_status_e vipdrv_debug_dump_mtlb(
    void *m,
    loff_t  *len,
    loff_t offset,
    vipdrv_mmu_info_t* mmu_info
    )
{
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    char *ptr = (char*)m;
#endif

    if (vip_false_e == vipdrv_context_support(VIP_HW_FEATURE_MMU_PDMODE)) {
        vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
        FS_PRINTF(ptr, *(len), offset, "\n***DUMP MMU entry****\n");
        vipdrv_debug_dump_data((void*)ptr,
                        len,
                        offset,
                        context->MMU_entry.logical,
                        context->MMU_entry.physical,
                        (context->MMU_entry.size > 64) ? 64 : context->MMU_entry.size
                        );
    }

    FS_PRINTF(ptr, *(len), offset, "*****DUMP MTLB******\n");
    vipdrv_debug_dump_data((void*)ptr,
                     len,
                     offset,
                     mmu_info->MTLB.logical,
                     mmu_info->MTLB.physical,
                     mmu_info->MTLB.size);
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return VIP_SUCCESS;
}

static vip_status_e vipdrv_debug_dump_stlb(
    void *m,
    loff_t *len,
    loff_t offset,
    vipdrv_mmu_info_t* mmu_info
    )
{
    vip_int32_t i = 0, stlb_index = 0;
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
    FS_PRINTF(ptr, *(len), offset, "*****DUMP STLB Start******\n");
    for (stlb_index = 0; stlb_index < mmu_info->MTLB_cnt; stlb_index++) {
        vipdrv_MTLB_info_t *MTLB_info = &mmu_info->MTLB_info[stlb_index];
        vip_uint32_t size = 0;
        if (VIP_SUCCESS != vipdrv_mmu_page_macro(MTLB_info->type, VIP_NULL, &unit,
            VIP_NULL, VIP_NULL, VIP_NULL, VIP_NULL)) {
            continue;
        }
        size = unit * sizeof(vip_uint32_t);

        FS_PRINTF(ptr, *(len), offset, "dump STLB (%s page), physical base=0x%"PRIx64", size=0x%x\n",
            MTLB_info->type == VIPDRV_MMU_4K_PAGE ? "4K" :
            MTLB_info->type == VIPDRV_MMU_64K_PAGE ? "64K" : "1M",
            MTLB_info->STLB->physical, size);
        logical = (vip_uint32_t*)MTLB_info->STLB->logical;
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
                vipdrv_debug_dump_data((void*)ptr,
                                    len,
                                    offset,
                                    tmp,
                                    MTLB_info->STLB->physical + sizeof(vip_uint32_t) * i,
                                    dump_size);
            }
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
static loff_t vipdrv_dump_mmu_table(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
    vipdrv_mmu_info_t* mmu_info = VIP_NULL;
    vip_bool_e clock_off = vip_false_e;
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

    CHECK_CONTEXT_INITIALIZE();

    vipdrv_mmu_page_get(0, &mmu_info);

#if vpmdENABLE_MULTIPLE_TASK
    status = vipdrv_os_lock_mutex(mmu_info->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to lock mmu mutex\n");
        vipdrv_mmu_page_put(0);
        return VIP_ERROR_FAILURE;
    }
#endif

    #if vpmdENABLE_POWER_MANAGEMENT
    VIPDRV_LOOP_HARDWARE_START
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
    if (!(VIPDRV_POWER_CLOCK & vipdrv_pm_query_state(hw))) {
        clock_off = vip_true_e;
    }
    VIPDRV_LOOP_HARDWARE_END
    #endif

    if (vip_false_e == clock_off) {
        vipdrv_debug_dump_mtlb((void*)p, &len, offset, mmu_info);
        vipdrv_debug_dump_stlb((void*)p, &len, offset, mmu_info);
    }

    #if vpmdENABLE_POWER_MANAGEMENT
    VIPDRV_LOOP_HARDWARE_START
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
    VIPDRV_LOOP_HARDWARE_END
    #endif

#if vpmdENABLE_MULTIPLE_TASK
    status = vipdrv_os_unlock_mutex(mmu_info->mmu_mutex);
    if (status != VIP_SUCCESS) {
        PRINTK_E("fail to unlock mmu mutex\n");
    }
#endif
    vipdrv_mmu_page_put(0);
    return len;
}
#endif

#if vpmdENABLE_HANG_DUMP
static loff_t vipdrv_get_hang_dump(
    void *m,
    void *data
    )
{
    loff_t len = 0, offset = 0;
#if vpmdUSE_DEBUG_FS
    struct seq_file *p = (struct seq_file*)m;
    offset = 0;
#else
    sys_param *param = (sys_param*)m;
    char* p = param->buf;
    offset = param->offset;
#endif

    CHECK_CONTEXT_INITIALIZE();
    PRINTK("debugFS hang dump start ...\n");

    VIPDRV_LOOP_DEVICE_START
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_START(device)
    vipdrv_task_t task = { 0 };
    task.core_cnt = 1;
    task.core_index = hw->core_id;
    task.device = device;
    #if vpmdENABLE_POWER_MANAGEMENT
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
    if (VIPDRV_POWER_CLOCK & vipdrv_pm_query_state(hw)) {
    #endif
        vipdrv_dump_states(&task);
        vipdrv_task_tcb_dump(&task);
        #if vpmdTASK_PARALLEL_ASYNC
        vipdrv_dump_waitlink_table(&task);
        #endif
    #if vpmdENABLE_POWER_MANAGEMENT
    }
    else {
        FS_PRINTF(p, len, offset, "vip not working, may vip has power off, can't read register\n");
    }
    vipdrv_pm_send_evnt(hw, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
    #endif
    VIPDRV_LOOP_HARDWARE_IN_DEVICE_END
    VIPDRV_LOOP_DEVICE_END
#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    PRINTK("debugFS hang dump end ...\n");

    return len;
}
#endif

#if vpmdENABLE_SUSPEND_RESUME
static ssize_t vipdrv_suspend_set(
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
        VIPDRV_LOOP_HARDWARE_START
        while (1) {
            if (VIPDRV_POWER_CLOCK & vipdrv_pm_query_state(hw)) {
                vipdrv_os_udelay(100);
            }
            else {
                vipdrv_pm_lock_hardware(hw);
                if (VIPDRV_POWER_CLOCK & vipdrv_pm_query_state(hw)) {
                    vipdrv_pm_unlock_hardware(hw);
                }
                else {
                    break;
                }
            }
        }
        VIPDRV_LOOP_HARDWARE_END

        PRINTK("debugfs suspend start ....\n");
        vipdrv_pm_suspend();
        PRINTK("debugfs suspend done ....\n");

        VIPDRV_LOOP_HARDWARE_START
        vipdrv_pm_unlock_hardware(hw);
        VIPDRV_LOOP_HARDWARE_END
    }
    else if (0 == ret) {
        /* resume */
        VIPDRV_LOOP_HARDWARE_START
        vipdrv_pm_lock_hardware(hw);
        VIPDRV_LOOP_HARDWARE_END
        PRINTK("debugfs resume start ...\n");

        status = vipdrv_pm_resume();
        if (status != VIP_SUCCESS) {
            PRINTK("vipcore, fail to do pm resume\n");
        }
        PRINTK("debugfs resume done ...\n");

        VIPDRV_LOOP_HARDWARE_START
        vipdrv_pm_unlock_hardware(hw);
        VIPDRV_LOOP_HARDWARE_END
    }
    else {
        PRINTK_E("suspend not support this value=%d\n", ret);
    }

    mutex_unlock(&debugfs_mutex);

    return count;
}
#endif
#if vpmdENABLE_DEBUG_LOG > 2
static ssize_t vipdrv_rt_log_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

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
static ssize_t vipdrv_rt_capture_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

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
static ssize_t vipdrv_cnn_profile_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

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
vip_status_e vipdrv_debug_core_loading_profile_set(
    vipdrv_task_t *task,
    vipdrv_profile_dtype_t dtype
    )
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t cur_infer = 0;
    vip_uint64_t time = vipdrv_os_get_time();

    if (VIP_NULL == core_loading) {
        return status;
    }

    switch (dtype) {
    case PROFILE_START:
    {
        VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
        core_loading[hw->core_id].submit_time = time;
        VIPDRV_LOOP_HARDWARE_IN_TASK_END
    }
    break;

    case PROFILE_END:
    {
        VIPDRV_LOOP_HARDWARE_IN_TASK_START(task)
        cur_infer = time - core_loading[hw->core_id].submit_time;
        core_loading[hw->core_id].infer_time += cur_infer;
        VIPDRV_LOOP_HARDWARE_IN_TASK_END
    }
    break;

    default:
    break;
    }

    return status;
}

vip_status_e vipdrv_debug_videomemory_profile_alloc(
    vip_size_t size
    )
{
    vip_status_e status = VIP_SUCCESS;

    video_mem_data.video_memory += size;

    if (video_mem_data.video_memory > video_mem_data.video_peak) {
        video_mem_data.video_peak = video_mem_data.video_memory;
    }
    video_mem_data.video_allocs++;

    return status;
}

vip_status_e vipdrv_debug_videomemory_profile_free(
    vip_size_t size
    )
{
    vip_status_e status = VIP_SUCCESS;

    video_mem_data.video_memory -= size;
    video_mem_data.video_frees++;

    return status;
}

#if vpmdDUMP_NBG_RESOURCE
static ssize_t vipdrv_rt_dump_nbg_set(
    const char *buf,
    size_t count
    )
{
    vip_int32_t ret = 0;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

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
static ssize_t vipdrv_multi_vip_set(
    const char *buf,
    size_t count
    )
{
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    vip_uint8_t device_core_number[256] = {0};
    int index_buf = 0, core_num = 0, index_core = 0, sum_core = 0;
    vip_bool_e record = vip_false_e;
    vip_uint32_t* initialize = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED);
    vip_uint32_t device_count = 0;

    if (0 < *initialize) {
        PRINTK_E("vip is still working...\n");
        return count;
    }
    if (kdriver->core_count > vipdMAX_CORE) {
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

                if ((kdriver->core_count > index_core) || (index_core >= vipdMAX_CORE)) {
                    device_core_number[index_core++] = (vip_uint8_t)core_num;
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
        if (index_buf > vipdMAX_CORE) {
            break;
        }
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

    /* update device count */
    for (index_core = 0; index_core < kdriver->core_count; index_core++) {
        if (kdriver->device_core_number[index_core] > 0) {
            device_count++;
        }
        else {
            kdriver->device_count = device_count;
            PRINTK("debugfs update device count=%d\n", device_count);
            break;
        }
    }

    return count;
}
#endif

static loff_t vipdrv_rt_net_profile(
    void *m,
    void *data
    )
{
    vip_uint32_t* initialize = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED);
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    loff_t len = 0, offset = 0;
    vip_uint32_t i = 0;
#if vpmdUSE_DEBUG_FS
    struct seq_file *ptr = (struct seq_file*)m;
#else
    sys_param *param = (sys_param*)m;
    char* ptr = param->buf;
    offset = param->offset;
#endif

    if (0 >= *initialize) {
        FS_PRINTF(ptr, len, offset, "vip not working, fail to show network profile\n");
        return len;
    }

    FS_PRINTF(ptr, len, offset, "------------------------------------------------------------------"\
                                "-----------------------------------------------------------------\n");
    FS_PRINTF(ptr, len, offset, "|  network handle      |  enable  | index |  network name             "\
                                "         |  infer cnt  |  avg infer time  |  avg cycle      |\n");
    for (i = 1; i < vipdrv_database_free_pos(tskDesc_database); i++) {
        vipdrv_task_descriptor_t *tsk_desc = VIP_NULL;
        vipdrv_subtask_info_t *subtask_info = VIP_NULL;
        vip_uint32_t j = 0;
        vip_uint64_t time = 0;
        vip_uint32_t cycle = 0;
        vipdrv_database_use(tskDesc_database, i, (void**)&tsk_desc);
        if (VIP_NULL == tsk_desc) {
            PRINTK_E("fail to get network info for index=%d\n", i);
            continue;
        }
        if (VIP_NULL != tsk_desc->subtasks) {
            for (j = 0; j < tsk_desc->subtask_count; j++) {
                subtask_info = &tsk_desc->subtasks[j];
                #if vpmdENABLE_APP_PROFILING
                time = subtask_info->avg_time;
                cycle = (subtask_info->avg_cycle[0].total_cycle - subtask_info->avg_cycle[0].total_idle_cycle);
                #endif
                FS_PRINTF(ptr, len, offset,
                   "------------------------------------------------------------------"\
                   "-----------------------------------------------------------------\n");
                if (0 == j) {
                    FS_PRINTF(ptr, len, offset,
                     "|  0x%-16x  |  %d       | %-5d |  %-32s  |  %-9d  |  %-12dus  |"\
                     "  %-13d  |\n", vipdrv_task_desc_index2id(i), !tsk_desc->skip, j, tsk_desc->task_name,
                     subtask_info->submit_count, time, cycle);
                }
                else {
                    FS_PRINTF(ptr, len, offset,
                      "|                      |          | %-5d |  %-32s  |  %-9d  |"\
                      "  %-12dus  |  %-13d  |\n",
                      j, tsk_desc->task_name, subtask_info->submit_count, time, cycle);
                }
            }
        }
        vipdrv_database_unuse(tskDesc_database, i);
    }
    FS_PRINTF(ptr, len, offset, "------------------------------------------------------------------"\
                                "-----------------------------------------------------------------\n");

#if !vpmdUSE_DEBUG_FS
flush_buffer:
#endif
    return len;
}

static ssize_t vipdrv_rt_net_profile_set(
    const char *buf,
    size_t count
    )
{
    vip_uint32_t* initialize = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED);
    vip_uint32_t index = 0;
    vip_uint64_t tmp = 0;
    vipdrv_database_t* tskDesc_database = (vipdrv_database_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_TASK_DESC);
    vipdrv_task_descriptor_t* tsk_desc = VIP_NULL;
    vip_char_t *usage = "echo [network handle] > rt_net_profile. (toggle the specifed network state)\n";

    if (0 >= *initialize) {
        PRINTK_E("vip not working, fail to manage network\n");
        return count;
    }

    sscanf(buf, "0x%"PRIx64"", &tmp);
    index = vipdrv_task_desc_id2index(tmp);
    if (VIP_SUCCESS == vipdrv_database_use(tskDesc_database, index, (void**)&tsk_desc)) {
        if (VIP_NULL != tsk_desc->subtasks) {
            tsk_desc->skip = !tsk_desc->skip;
        }
        else {
            PRINTK("did not find this network 0x%x\n", tmp);
            PRINTK("usage: %s", usage);
        }
        vipdrv_database_unuse(tskDesc_database, index);
    }
    else {
        PRINTK("did not find this network 0x%x\n", tmp);
        PRINTK("usage: %s", usage);
    }

    return count;
}

#if vpmdTASK_DISPATCH_DEBUG
static loff_t vipdrv_task_dispatch_get(
    void *m,
    void *data
    )
{
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    if (0 == context->initialize) {
        return 0;
    }

    VIPDRV_LOOP_DEVICE_START
    vipdrv_task_dispatch_profile(device, VIPDRV_TASK_DIS_PROFILE_SHOW);
    VIPDRV_LOOP_DEVICE_END

    return 0;
}

static ssize_t vipdrv_task_dispatch_set(
    const char *buf,
    size_t count
    )
{
    vip_uint32_t ret;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    if (0 == context->initialize) {
        return 0;
    }

    ret = strncmp(buf, "reset", 5);

    if (!ret) {
        VIPDRV_LOOP_DEVICE_START
        vipdrv_task_dispatch_profile(device, VIPDRV_TASK_DIS_PROFILE_RESET);
        VIPDRV_LOOP_DEVICE_END
    }
    else {
        PRINTK_E("invalid param, please set 'reset' string to reset task dispatch data.\n");
    }

    return count;
}
#endif

#if vpmdUSE_DEBUG_FS
/*
@brief linux seq file operations.
*/
static void *vipdrv_seq_start(
    struct seq_file *s,
    loff_t *pos
    )
{
    if(*pos > 0)
        return NULL;
    return pos;
}

static void *vipdrv_seq_next(
    struct seq_file *s,
    void *v,
    loff_t *pos
    )
{
    return NULL;
}

static void vipdrv_seq_stop(
    struct seq_file *s,
    void *v
    )
{
}

#define vip_debugfs_write(a)                                   \
static ssize_t vip_debugfs_##a##_write(struct file *file,      \
                                           const char __user *ubuf,\
                                           size_t count,           \
                                           loff_t *ppos)           \
{                                                                  \
    return count;                                                  \
}

vip_debugfs_write(vip_info)
vip_debugfs_write(mem_mapping)
#if vpmdENABLE_MMU
vip_debugfs_write(dump_mmu_table)
#endif
#if vpmdENABLE_HANG_DUMP
vip_debugfs_write(hang_dump)
#endif
vip_debugfs_write(pc_value)


static ssize_t vip_debugfs_core_loading_write(
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

    vipdrv_core_loading_set((const char*)&buf, count);

    return count;
}

/*
@brief show the time profile result
*/
static int vipdrv_seq_core_loading_show(
    struct seq_file *s,
    void *v
    )
{
    return vipdrv_core_loading_get((void*)s, VIP_NULL);
}

/*
@brief show the viplite info profile result
*/
static int vipdrv_seq_vip_info_show(
    struct seq_file *s,
    void *v
    )
{
    return vipdrv_vipinfo_show((void*)s, VIP_NULL);
}

static int vipdrv_seq_pc_value_show(
    struct seq_file *s,
    void *v
    )
{
    return vipdrv_pc_value_get((void*)s, VIP_NULL);
}

#if IS_ENABLED(CONFIG_NPU_SET_CLK_VOL)
static ssize_t vip_debugfs_test_clk_freq_write(
	struct file *file,
	const char __user *ubuf,
	size_t count,
	loff_t *opps
	)
{
	vip_char_t buf[16] = {};
	if (count > 16) {
		PRINTK_E("please echo a valid frequency number\n");
		return count;
	}

	if (copy_from_user(&buf, ubuf, count)) {
		PRINTK_E("copy user data to kernel space fail, not set clk freq\n");
		return count;
	}

    vipdrv_test_clkfreq_set((const char *)&buf, count);

	return count;
}

static ssize_t vip_debugfs_test_vol_regul_write(
	struct file *file,
	const char __user *ubuf,
	size_t count,
	loff_t *opps
	)
{
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    struct device *dev = &(kdriver->pdev->dev);
	vip_char_t buf[16] = {};
	if (count > 16) {
		PRINTK_E("please echo a vol regul number\n");
		return count;
	}

	if (copy_from_user(&buf, ubuf, count)) {
		PRINTK_E("copy user data to kernel space fail, not set vol regul\n");
		return count;
	}

	vipdrv_test_volregul_set((const char *)&buf, count, kdriver, dev);
	return count;
}
#else
static ssize_t vip_debugfs_clk_freq_write(
	struct file *file,
	const char __user *ubuf,
	size_t count,
	loff_t *opps
	)
{
	vip_char_t buf[16] = {};
	if (count > 16) {
		PRINTK_E("please echo a valid frequency number\n");
		return count;
	}

	if (copy_from_user(&buf, ubuf, count)) {
		PRINTK_E("copy user data to kernel space fail, not set clk freq\n");
		return count;
	}

    vipdrv_clkfreq_set((const char *)&buf, count);

	return count;
}
#endif

#if IS_ENABLED(CONFIG_NPU_SET_CLK_VOL)
static int vipdrv_seq_test_clk_freq_show(
	struct seq_file *s,
	void *v
	)
{
	return vipdrv_test_clkfreq_show((void *)s, VIP_NULL);
}

static int vipdrv_seq_test_vol_regul_show(
	struct seq_file *s,
	void *v
	)
{
	return vipdrv_test_volregul_show((void *)s, VIP_NULL);
}
#else
static int vipdrv_seq_clk_freq_show(
	struct seq_file *s,
	void *v
	)
{
	return vipdrv_clkfreq_show((void *)s, VIP_NULL);
}
#endif

static ssize_t vip_debugfs_mem_profile_write(
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

    vipdrv_mem_profile_set((const char*)&buf, VIP_NULL);

    return count;
}

static int vipdrv_seq_mem_profile_show(
    struct seq_file *s,
    void *v
    )
{
    return vipdrv_mem_profile((void*)s, VIP_NULL);
}

static ssize_t vip_debugfs_vip_freq_write(
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

    vipdrv_vipfreq_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_vip_freq_show(
    struct seq_file *s,
    void *v
    )
{
    return vipdrv_vipfreq_show((void*)s, VIP_NULL);
}

static int vipdrv_seq_mem_mapping_show(
    struct seq_file *s,
    void *v
    )
{
    ssize_t ret = 0;
    mutex_lock(&debugfs_mutex);
    ret = (ssize_t)vipdrv_mem_mapping((void*)s, VIP_NULL);
    mutex_unlock(&debugfs_mutex);
    return ret;
}

#if vpmdENABLE_MMU
static int vipdrv_seq_dump_mmu_table_show(
    struct seq_file *s,
    void *v
    )
{
    ssize_t ret = 0;
    mutex_lock(&debugfs_mutex);
    ret = (ssize_t)vipdrv_dump_mmu_table((void*)s, VIP_NULL);
    mutex_unlock(&debugfs_mutex);
    return ret;
}
#endif
#if vpmdENABLE_HANG_DUMP
static int vipdrv_seq_hang_dump_show(
    struct seq_file *s,
    void *v
    )
{
    ssize_t ret = 0;
    mutex_lock(&debugfs_mutex);
    ret = (ssize_t)vipdrv_get_hang_dump((void*)s, VIP_NULL);
    mutex_unlock(&debugfs_mutex);
    return ret;
}
#endif
#if vpmdENABLE_SUSPEND_RESUME
static ssize_t vip_debugfs_suspend_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};
    vip_uint32_t* initialize = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED);
    if (0 == *initialize) {
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

    vipdrv_suspend_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_suspend_show(
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
static ssize_t vip_debugfs_rt_log_write(
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

    vipdrv_rt_log_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_rt_log_show(
    struct seq_file *s,
    void *v
    )
{
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    seq_printf(s, "rt_log=%d\n", kdriver->log_level);
    seq_printf(s, "  when rt_log=1, enable kernel info/debug log\n");
    seq_printf(s, "  when rt_log=0, disable kernel info/debug log\n");
    return 0;
}
#endif
#if vpmdENABLE_CAPTURE
static ssize_t vip_debugfs_rt_capture_write(
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

    vipdrv_rt_capture_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_rt_capture_show(
    struct seq_file *s,
    void *v
    )
{
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    seq_printf(s, "  capture = %d\n", kdriver->func_config.enable_capture);
    seq_printf(s, "  when capture =1, enable user space capture function.\n");
    seq_printf(s, "  when capture =0, disable user space capture function.\n");

    return 0;
}
#endif

#if vpmdENABLE_CNN_PROFILING
static ssize_t vip_debugfs_rt_cnn_profile_write(
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

    vipdrv_cnn_profile_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_rt_cnn_profile_show(
    struct seq_file *s,
    void *v
    )
{
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    seq_printf(s, "  cnn_profile  = %d\n", kdriver->func_config.enable_cnn_profile);
    seq_printf(s, "  when cnn_profile =1, enable user space cnn profile function.\n");
    seq_printf(s, "  when cnn_profile =0, disable user space cnn profile function.\n");

    return 0;
}
#endif

#if vpmdDUMP_NBG_RESOURCE
static ssize_t vip_debugfs_rt_dump_nbg_write(
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

    vipdrv_rt_dump_nbg_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_rt_dump_nbg_show(
    struct seq_file *s,
    void *v
    )
{
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
    seq_printf(s, "dump_nbg = %d\n", kdriver->func_config.enable_dump_nbg);
    seq_printf(s, "enable dump nbg command: echo 1 > /sys/kernel/debug/viplite/dump_nbg\n");
    seq_printf(s, "disable dump nbg command: echo 0 > /sys/kernel/debug/viplite/dump_nbg\n");
    return 0;
}
#endif

#if vpmdENABLE_MULTIPLE_VIP
static ssize_t vip_debugfs_multi_vip_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[256] = {};

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail, fail to config multi-vip.\n");
    }

    vipdrv_multi_vip_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_multi_vip_show(
    struct seq_file *s,
    void *v
    )
{
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
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

static ssize_t vip_debugfs_rt_net_profile_write(
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

    vipdrv_rt_net_profile_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_rt_net_profile_show(
    struct seq_file *s,
    void *v
    )
{
    ssize_t ret = 0;

    ret = (ssize_t)vipdrv_rt_net_profile((void*)s, VIP_NULL);

    return ret;
}

static vip_status_e vipdrv_register_rw(
    const char __user *ubuf,
    size_t count
    )
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vip_char_t buf[128] = {}, rw = 0;
    vip_uint32_t val = 0, address = 0;
    vip_uint32_t dev_index = 0, core_index = 0;
    int size = 0;
    vipdrv_device_t* device = VIP_NULL;
    vipdrv_hardware_t* hardware = VIP_NULL;
    vip_char_t *usage = "echo > [option] [dev-index ][core-index] [address] (value) > register_rw\n"
          "  echo > [w --write] [dev-index] [core-index] [register address] [write val] > register_rw\n"
          "  echo > [r  --read] [dev-index] [core-index] [register address] > register_rw\n"
          "  eg: echo r 0 0 0x30 > register_rw\n"
          "      echo w 0 0 0x00 0x00070900 > register_rw\n";

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail.\n");
    }

    size = sscanf(buf, "%c %d %d", &rw, &dev_index, &core_index);
    if (rw == 'h') {
        PRINTK_E("Usage:\n %s", usage);
        return status;
    }
    else if (size != 3) {
        PRINTK_E("fail to get used command\n");
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    /* sanity check */
    if (rw != 'W' && rw != 'w' && rw != 'R' && rw != 'r') {
        PRINTK_E("fail input is %c\n", rw);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    if (dev_index >= context->device_count) {
        PRINTK_E("fail dev-index=%d, MAX=%d\n", dev_index, context->device_count);
        vipGoOnError(VIP_ERROR_FAILURE);
    }
    device = vipdrv_get_device(dev_index);
    vipIsNULL(device);

    if (core_index >= device->hardware_count) {
        PRINTK_E("fail dev-index=%d, core-index, MAX=%d\n", dev_index, core_index, device->hardware_count);
        vipGoOnError(VIP_ERROR_FAILURE);
    }

    hardware = vipdrv_get_hardware(device, core_index);
    vipIsNULL(hardware);

    #if vpmdENABLE_POWER_MANAGEMENT
    vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_ON, VIP_NULL);
    if (VIPDRV_POWER_CLOCK & vipdrv_pm_query_state(hardware)) {
    #endif
        if (rw == 'w' || rw == 'W') {
            size = sscanf(buf, "%c %d %d %x %x", &rw, &dev_index, &core_index, &address, &val);
            if (size == 5) {
                status = vipdrv_os_write_reg(hardware->reg_base, address, val);
                if (status != VIP_SUCCESS) {
                    PRINTK_E("write dev%d hw%d address[0x%x] val[0x%x] failed.\n",
                            dev_index, core_index, address, val);
                    return count;
                }
                PRINTK_E("write dev%d hw%d address[0x%x] val[0x%x] success.\n",
                            dev_index, core_index, address, val);
            } else {
                PRINTK_E("write param error size=%d, usage:\n %s.\n", size, usage);
            }
        }
        else if (rw == 'r' || rw == 'R') {
            size = sscanf(buf, "%c %d %d %x", &rw, &dev_index, &core_index, &address);
            if (size == 4) {
                status = vipdrv_os_read_reg(hardware->reg_base, address, &register_value);
                if (status != VIP_SUCCESS) {
                    PRINTK_E("read dev%d hw%d address[0x%x] failed.\n", dev_index, core_index, address);
                    return count;
                }
                register_address = address;
                PRINTK_E("read dev%d hw%d address[0x%x] = [0x%x] success.\n",
                          dev_index, core_index, address, register_value);
            } else {
                PRINTK_E("read param error size=%d, usage:\n %s.\n", size, usage);
            }
        }
    #if vpmdENABLE_POWER_MANAGEMENT
    }
    else {
        PRINTK_E("dev%d hw%d not working, may vip has power off, can't read register\n", \
               dev_index, hardware->core_index);
    }
    status = vipdrv_pm_send_evnt(hardware, VIPDRV_PWR_EVNT_CLK_OFF, VIP_NULL);
    #endif
    PRINTK_D("end power off....\n");

    return status;

onError:
    PRINTK_E("Usage: %s", usage);
    return status;
}

static ssize_t vip_debugfs_register_rw_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vipdrv_context_t* context = (vipdrv_context_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_ALL);
    vip_status_e status = VIP_SUCCESS;
    vip_bool_e need_init = vip_true_e;

    mutex_lock(&debugfs_mutex);

    if (0 < context->initialize) {
        need_init = vip_false_e;
    }

    if (vip_true_e == need_init) {
        vipdrv_initialize_t init_data;
        init_data.version = ((VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | (VERSION_SUB_MINOR));
        status = vipdrv_init(&init_data);
        if (status != VIP_SUCCESS) {
            PRINTK_E("fail to init vipdrv\n");
            mutex_unlock(&debugfs_mutex);
            return count;
        }
    }

    vipdrv_register_rw(ubuf, count);

    if (vip_true_e == need_init) {
        PRINTK_D("vip drv destroy start\n");
        vipdrv_destroy();
    }

    mutex_unlock(&debugfs_mutex);

    return count;
}

static int vipdrv_seq_register_rw_show(
    struct seq_file *s,
    void *v
    )
{
    if ((0 == register_address) && (0 == register_value)) {
        vip_char_t *usage = "echo > [option] [dev-index ][core-index] [address] (value) > register_rw\n"
              "  echo > [w --write] [dev-index] [core-index] [register address] [write val] > register_rw\n"
              "  echo > [r  --read] [dev-index] [core-index] [register address] > register_rw\n"
              "  eg: echo r 0 0 0x30 > register_rw\n"
              "      echo w 0 0 0x00 0x00070900 > register_rw\n";
        seq_printf(s, "usage: %s\n", usage);
    }
    else {
        mutex_lock(&debugfs_mutex);
        seq_printf(s, "register[0x%x] = 0x%x.\n", register_address, register_value);
        mutex_unlock(&debugfs_mutex);
    }

    return 0;
}

#if vpmdTASK_DISPATCH_DEBUG
static ssize_t vip_debugfs_task_dispatch_write(
    struct file *file,
    const char __user *ubuf,
    size_t count,
    loff_t *ppos
    )
{
    vip_char_t buf[128] = {};

    if (count > 128) {
        PRINTK_E("please echo bytes no more than 128, not reset task profile.\n");
        return count;
    }

    if (copy_from_user(&buf, ubuf, count)) {
        PRINTK_E("copy user data to kernel space fail,not reset task profile.\n");
    }

    vipdrv_task_dispatch_set((const char*)&buf, count);

    return count;
}

static int vipdrv_seq_task_dispatch_show(
    struct seq_file *s,
    void *v
    )
{
    return vipdrv_task_dispatch_get((void*)s, VIP_NULL);
}
#endif

#define seq_ops(a)                                             \
    static const struct seq_operations vipdrv_seq_##a##_ops = { \
        .start = vipdrv_seq_start,                              \
        .next = vipdrv_seq_next,                                \
        .stop = vipdrv_seq_stop,                                \
        .show = vipdrv_seq_##a##_show,                          \
    };

#define seq_open(a)                                                             \
    static int vip_debugfs_##a##_open(struct inode *inode, struct file *file) \
    {                                                                           \
        return seq_open(file, &vipdrv_seq_##a##_ops);                            \
    }

#define debugfs_ops(a)                                               \
    static const struct file_operations vip_debugfs_##a##_ops = {  \
        .owner = THIS_MODULE,                                        \
        .open = vip_debugfs_##a##_open,                            \
        .read = seq_read,                                            \
        .write = vip_debugfs_##a##_write,                          \
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
#if IS_ENABLED(CONFIG_NPU_SET_CLK_VOL)
unroll_debugfs_file(test_vol_regul)
unroll_debugfs_file(test_clk_freq)
#else
unroll_debugfs_file(clk_freq)
#endif
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
unroll_debugfs_file(pc_value)
unroll_debugfs_file(rt_net_profile)
#if vpmdTASK_DISPATCH_DEBUG
unroll_debugfs_file(task_dispatch)
#endif
/*
@brief Creat debugfs profile dir and file
*/
vip_status_e vipdrv_debug_create_debugfs(void)
{
    vip_status_e status = VIP_SUCCESS;
    debugfs_parent = debugfs_create_dir("viplite", NULL);
    if (!debugfs_parent) {
        PRINTK("create viplite debugfs dir failed.\n");
        status = VIP_ERROR_FAILURE;
        return status;
    }

    debugfs_create_file("vip_info", 0644, debugfs_parent, NULL, &vip_debugfs_vip_info_ops);
    debugfs_create_file("vip_freq", 0644, debugfs_parent, NULL, &vip_debugfs_vip_freq_ops);
    debugfs_create_file("core_loading", 0644, debugfs_parent, NULL, &vip_debugfs_core_loading_ops);
    debugfs_create_file("mem_profile", 0644, debugfs_parent, NULL, &vip_debugfs_mem_profile_ops);
    debugfs_create_file("mem_mapping", 0644, debugfs_parent, NULL, &vip_debugfs_mem_mapping_ops);
    debugfs_create_file("register_rw", 0644, debugfs_parent, NULL, &vip_debugfs_register_rw_ops);
    debugfs_create_file("pc_value", 0644, debugfs_parent, NULL, &vip_debugfs_pc_value_ops);
#if IS_ENABLED(CONFIG_NPU_SET_CLK_VOL)
    debugfs_create_file("test_vol_regul", 0644, debugfs_parent, NULL, &vip_debugfs_test_vol_regul_ops);
    debugfs_create_file("test_clk_freq", 0644, debugfs_parent, NULL, &vip_debugfs_test_clk_freq_ops);
#else
    debugfs_create_file("clk_freq", 0644, debugfs_parent, NULL, &vip_debugfs_clk_freq_ops);
#endif
#if vpmdENABLE_DEBUG_LOG > 2
    debugfs_create_file("rt_log", 0644, debugfs_parent, NULL, &vip_debugfs_rt_log_ops);
#endif
#if vpmdENABLE_CAPTURE
    debugfs_create_file("rt_capture", 0644, debugfs_parent, NULL, &vip_debugfs_rt_capture_ops);
#endif
#if vpmdENABLE_CNN_PROFILING
    debugfs_create_file("rt_cnn_profile", 0644, debugfs_parent, NULL, &vip_debugfs_rt_cnn_profile_ops);
#endif
#if vpmdDUMP_NBG_RESOURCE
    debugfs_create_file("rt_dump_nbg", 0644, debugfs_parent, NULL, &vip_debugfs_rt_dump_nbg_ops);
#endif
#if vpmdENABLE_MULTIPLE_VIP
    debugfs_create_file("multi_vip", 0644, debugfs_parent, NULL, &vip_debugfs_multi_vip_ops);
#endif
#if vpmdENABLE_MMU
    debugfs_create_file("dump_mmu_table", 0644, debugfs_parent, NULL, &vip_debugfs_dump_mmu_table_ops);
#endif
#if vpmdENABLE_HANG_DUMP
    debugfs_create_file("hang_dump", 0644, debugfs_parent, NULL, &vip_debugfs_hang_dump_ops);
#endif
#if vpmdENABLE_SUSPEND_RESUME
    debugfs_create_file("suspend", 0644, debugfs_parent, NULL, &vip_debugfs_suspend_ops);
#endif
    debugfs_create_file("rt_net_profile", 0644, debugfs_parent, NULL, &vip_debugfs_rt_net_profile_ops);
#if vpmdTASK_DISPATCH_DEBUG
    debugfs_create_file("task_dispatch", 0644, debugfs_parent, NULL, &vip_debugfs_task_dispatch_ops);
#endif
    return status;
}

/*
@brief Destroy debugfs profile dir and file
*/
vip_status_e vipdrv_debug_destroy_debugfs(void)
{
    if (debugfs_parent != VIP_NULL) {
        debugfs_remove_recursive(debugfs_parent);
        debugfs_parent = VIP_NULL;
    }
    return VIP_SUCCESS;
}


#undef vip_debugfs_write
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
    len = (loff_t)vipdrv_vipinfo_show((void*)(&param), VIP_NULL);

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
    len = (loff_t)vipdrv_vipfreq_show((void*)(&param), VIP_NULL);

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
    return vipdrv_vipfreq_set(buf, count);
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
    len = (loff_t)vipdrv_core_loading_get((void*)(&param), VIP_NULL);

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
    return vipdrv_core_loading_set(buf, count);
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
    len = (loff_t)vipdrv_mem_profile((void*)(&param), VIP_NULL);

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
    ret = (ssize_t)vipdrv_mem_profile_set(buf, count);
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
    len = (loff_t)vipdrv_mem_mapping((void*)(&param), VIP_NULL);
    if (off >= len) mutex_unlock(&debugfs_mutex);

    return (len >= off ? len - off : 0);
}

static ssize_t pc_value_show(
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
    len = (loff_t)vipdrv_pc_value_get((void*)(&param), VIP_NULL);
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
    len = (loff_t)vipdrv_dump_mmu_table((void*)(&param), VIP_NULL);
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
    len = (loff_t)vipdrv_get_hang_dump((void*)(&param), VIP_NULL);
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
    vip_uint32_t* initialize = (vip_uint32_t*)vipdrv_get_context(VIPDRV_CONTEXT_PROP_INITED);
    if (0 == *initialize) {
        PRINTK_E("vip not working, fail to set suspend\n");
        return count;
    }
    return vipdrv_suspend_set(buf, count);
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
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
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
    return vipdrv_rt_log_set(buf, count);
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
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
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
    return vipdrv_capture_set(buf, count);
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
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
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
    return vipdrv_cnn_profile_set(buf, count);
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
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
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
    return vipdrv_rt_dump_nbg_set(buf, count);
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
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();
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
    return vipdrv_multi_vip_set(buf, count);
}
#endif

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
    len = (loff_t)vipdrv_rt_net_profile((void*)(&param), VIP_NULL);

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
    return vipdrv_rt_net_profile_set(buf, count);
}

#if vpmdTASK_DISPATCH_DEBUG
static ssize_t task_dispatch_show(
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
    len = (loff_t)vipdrv_task_dispatch_get((void*)(&param), VIP_NULL);

    return (len >= off ? len - off : 0);
}

static ssize_t task_dispatch_store(
    struct file *filp,
    struct kobject *kobj,
    struct bin_attribute *attr,
    char *buf,
    loff_t off,
    size_t count)
{
    return vipdrv_task_dispatch_set(buf, count);
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
sysfs_RO(pc_value);
#if IS_ENABLED(CONFIG_NPU_SET_CLK_VOL)
sysfs_RW(test_vol_regul)
sysfs_RW(test_clk_freq);
#else
sysfs_RW(clk_freq);
#endif
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
sysfs_RW(rt_net_profile);
#if vpmdTASK_DISPATCH_DEBUG
sysfs_RW(task_dispatch);
#endif

vip_status_e vipdrv_debug_create_sysfs(void)
{
    vip_status_e status = VIP_SUCCESS;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_vip_info_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_vip_freq_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_core_loading_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_mem_profile_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_mem_mapping_attr);
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_pc_value_attr);
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
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_rt_net_profile_attr);
#if vpmdTASK_DISPATCH_DEBUG
    status |= sysfs_create_bin_file(&kdriver->device->kobj, &sysfs_task_dispatch_attr);
#endif
    return status;
}

vip_status_e vipdrv_debug_destroy_sysfs(void)
{
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_vip_info_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_vip_freq_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_core_loading_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_mem_profile_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_mem_mapping_attr);
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_pc_value_attr);
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
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_rt_net_profile_attr);
#if vpmdTASK_DISPATCH_DEBUG
    sysfs_remove_bin_file(&kdriver->device->kobj, &sysfs_task_dispatch_attr);
#endif

    return VIP_SUCCESS;
}
#endif

vip_status_e  vipdrv_debug_create_fs(void)
{
    vip_status_e status = VIP_SUCCESS;
    vip_uint32_t size = 0;
    vipdrv_driver_t *kdriver = vipdrv_get_kdriver();

    if (kdriver->core_count > 0) {
        debugfs_core_cnt = kdriver->core_count;
    }
    else {
        debugfs_core_cnt = vipdMAX_CORE;
    }
    size = debugfs_core_cnt * sizeof(vipdrv_core_loading_profile_t);
    vipdrv_os_allocate_memory(size, (void**)&core_loading);
    vipdrv_os_zero_memory((void*)core_loading, size);
    PRINTK_D("debugfs core cnt=%d\n", debugfs_core_cnt);

#if vpmdUSE_DEBUG_FS
    status = vipdrv_debug_create_debugfs();
#else
    status = vipdrv_debug_create_sysfs();
#endif
    return status;
}

vip_status_e  vipdrv_debug_destroy_fs(void)
{
    vip_status_e status = VIP_SUCCESS;
#if vpmdUSE_DEBUG_FS
    status = vipdrv_debug_destroy_debugfs();
#else
    status = vipdrv_debug_destroy_sysfs();
#endif

    if (core_loading != VIP_NULL) {
        vipdrv_os_free_memory((void*)core_loading);
        core_loading = VIP_NULL;
    }

    return status;
}

vip_status_e vipdrv_debug_profile_start(void)
{
    vip_uint32_t i = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t time = vipdrv_os_get_time();
    if (VIP_NULL == core_loading) {
        return status;
    }

    for (i = 0; i < debugfs_core_cnt; i++) {
        core_loading[i].init_time = time;
        core_loading[i].destory_time = 0;
        core_loading[i].submit_time = 0;
        core_loading[i].infer_time = 0;
    }

    video_mem_data.video_memory = 0;
    video_mem_data.video_peak = 0;
    video_mem_data.video_allocs = 0;
    video_mem_data.video_frees = 0;

    return status;
}

vip_status_e vipdrv_debug_profile_end(void)
{
    vip_uint32_t i = 0;
    vip_status_e status = VIP_SUCCESS;
    vip_uint64_t time = vipdrv_os_get_time();
    if (VIP_NULL == core_loading) {
        return status;
    }

    for (i = 0; i < debugfs_core_cnt; i++) {
        core_loading[i].destory_time = time;
    }

    return status;
}

#endif

/*
 * rdr_hisi_ap_adapter.c
 *
 * Based on the RDR framework, adapt to the AP side to implement resource
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */
#include "rdr_platform_ap_adapter.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/thread_info.h>
#include <linux/hardirq.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_fdt.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/kthread.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/preempt.h>
#include <asm/cacheflush.h>
#include <linux/kmsg_dump.h>
#include <linux/slab.h>
#include <linux/kdebug.h>
#include <linux/io.h>
#include <linux/stacktrace.h>
#include <linux/kallsyms.h>
#include <linux/blkdev.h>
#include <linux/soc/cix/util.h>
#include <linux/soc/cix/rdr_pub.h>
#include "../rdr_inner.h"
#include <linux/soc/cix/mntn_dump.h>
#include <linux/version.h>
#include <linux/watchdog.h>
#include <mntn_subtype_exception.h>
#include "../rdr_print.h"
#include "rdr_platform_ap_exception_logsave.h"
#include "../rdr_utils.h"
#include <linux/soc/cix/dst_reboot_reason.h>
#include <linux/panic_notifier.h>

static struct ap_eh_root *g_rdr_ap_root = NULL;
static AP_RECORD_PC *g_bbox_ap_record_pc = NULL;
u64 g_hisiap_addr;
static char g_log_path[LOG_PATH_LEN];
static int g_rdr_ap_init;
static struct rdr_register_module_result g_current_info;
static struct mutex g_dump_mem_mutex;
int g_bbox_fpga_flag = -1;

static unsigned int g_dump_buffer_size_tbl[HK_MAX] = {0};
static unsigned int g_last_task_struct_size;
static unsigned int g_last_task_stack_size;

u64 get_hisiap_addr(void)
{
	return g_hisiap_addr;
}

/*
 * struct rdr_exception_info_s {
 *  struct list_head e_list;
 *  u32 e_modid;
 *  u32 e_modid_end;
 *  u32 e_process_priority;
 *  u32 e_reboot_priority;
 *  u64 e_notify_core_mask;
 *  u64 e_reset_core_mask;
 *  u64 e_from_core;
 *  u32 e_reentrant;
 *  u32 e_exce_type;
 *  u32 e_upload_flag;
 *  u8  e_from_module[MODULE_NAME_LEN];
 *  u8  e_desc[STR_EXCEPTIONDESC_MAXLEN];
 *  u32 e_reserve_u32;
 *  void*   e_reserve_p;
 *  rdr_e_callback e_callback;
 * };
 */
struct rdr_exception_info_s g_einfo[] = {
	{ { 0, 0 }, MODID_AP_S_TEST, MODID_AP_S_TEST, RDR_ERR,
	 RDR_REBOOT_NO, RDR_AP, RDR_CSUSE, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_PANIC, HI_APPANIC_RESERVED, (u32)RDR_UPLOAD_YES, "ap", "ap",
	 RDR_SAVE_LOGBUF, 0, 0, 0},
	{ { 0, 0 }, MODID_AP_S_PANIC, MODID_AP_S_PANIC, RDR_ERR,
	 RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
	 (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_PANIC, HI_APPANIC_RESERVED, (u32)RDR_UPLOAD_YES, "ap", "ap",
	 0, 0, 0, 0},
};

/*
 * The following table lists the dump memory used by other maintenance
 * and test modules and IP addresses of the AP.
 */
static unsigned int g_dump_modu_mem_size_tbl[MODU_MAX] = {0};
static char g_dump_modu_compatible[MODU_MAX][AMNTN_MODULE_COMP_LEN] = {
#ifdef CONFIG_PLAT_BBOX_TEST
	[MODU_TEST] = { "ap_dump_mem_modu_test_size" },
#endif
	[MODU_NOC] = { "" },
	[MODU_DDR] = { "" },
	[MODU_TZC400] = {"ap_dump_mem_modu_tzc400_size"},
	[MODU_IDM] = {"ap_dump_mem_modu_idm_size"},
	[MODU_SMMU] = {"ap_dump_mem_modu_smmu_size"},
	[MODU_TFA] = {"ap_dump_mem_modu_tfa_size"},
	[MODU_GAP] = { "ap_dump_mem_modu_gap_size" },
};

static struct notifier_block acpu_panic_loop_block = {
	.notifier_call = acpu_panic_loop_notify,
	.priority = INT_MAX,
};

static struct notifier_block rdr_hisiap_panic_block = {
	.notifier_call = rdr_hisiap_panic_notify,
	.priority = INT_MIN,
};

static struct notifier_block rdr_hisiap_die_block = {
	.notifier_call = rdr_hisiap_die_notify,
	.priority = INT_MIN,
};

/*
 * Description:   Read the memory size of each track on the AP side from dts.
 * Return:        0:read success, non 0:fail
 */
static int get_ap_trace_mem_size_from_dts(struct device *dev)
{
	int ret;
	u32 i = 0;

	ret = device_property_read_u32(dev, "ap_trace_irq_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_irq_size in dts!\n", __func__);
		return ret;
	}

	ret = device_property_read_u32(dev, "ap_trace_task_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_task_size in dts!\n", __func__);
		return ret;
	}

	ret = device_property_read_u32(dev, "ap_trace_cpu_idle_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_cpu_idle_size in dts!\n", __func__);
		return ret;
	}

	ret = device_property_read_u32(dev, "ap_trace_worker_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_worker_size in dts!\n", __func__);
		return ret;
	}
/* delete ap_trace_mem_alloc_size and ap_trace_ion_alloc_size,You need to delete this parameter */
	g_dump_buffer_size_tbl[i++] = 0;
	g_dump_buffer_size_tbl[i++] = 0;

	ret = device_property_read_u32(dev, "ap_trace_time_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_time_size in dts!\n", __func__);
		return ret;
	}

	ret = device_property_read_u32(dev, "ap_trace_cpu_on_off_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_cpu_on_off_size in dts!\n", __func__);
		return ret;
	}

	ret = device_property_read_u32(dev, "ap_trace_syscall_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_syscall_size in dts!\n", __func__);
		return ret;
	}

	ret = device_property_read_u32(dev, "ap_trace_hung_task_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_hung_task_size in dts!\n", __func__);
		return ret;
	}

	ret = device_property_read_u32(dev, "ap_trace_tasklet_size", &g_dump_buffer_size_tbl[i++]);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_trace_tasklet_size in dts!\n", __func__);
		return ret;
	}

	return ret;
}

/*
 * Description:  Read the dump memory size of each module on the AP side from the dts file.
 * Return:       0:read success, non 0:fail
 */
static int get_ap_dump_mem_modu_size_from_dts(struct device *dev)
{
	int ret = 0;
	u32 i = 0;

	for (i = 0; i < MODU_MAX; i++) {
		/* empty string, set size zero */
		if (g_dump_modu_compatible[i][0] == 0) {
			g_dump_modu_mem_size_tbl[i] = 0;
			continue;
		}
		ret = device_property_read_u32(dev, g_dump_modu_compatible[i], &g_dump_modu_mem_size_tbl[i]);
		if (ret) {
			BB_PRINT_ERR("[%s], cannot find %s in dts!\n", __func__, g_dump_modu_compatible[i]);
			return ret;
		}
		BB_PRINT_DBG("[%s], module %s dump size = 0x%x \n", __func__, g_dump_modu_compatible[i], g_dump_modu_mem_size_tbl[i]);
	}

	return ret;
}

void print_debug_info(void)
{
	unsigned int i;
	struct regs_info *regs_info = g_rdr_ap_root->dump_regs_info;

	BB_PRINT_PN("=================struct ap_eh_root================");
	BB_PRINT_PN("[%s], dump_magic [0x%x]\n", __func__, g_rdr_ap_root->dump_magic);
	BB_PRINT_PN("[%s], version [%s]\n", __func__, g_rdr_ap_root->version);
	BB_PRINT_PN("[%s], modid [0x%x]\n", __func__, g_rdr_ap_root->modid);
	BB_PRINT_PN("[%s], e_exce_type [0x%x],\n", __func__, g_rdr_ap_root->e_exce_type);
	BB_PRINT_PN("[%s], e_exce_subtype [0x%x],\n", __func__, g_rdr_ap_root->e_exce_subtype);
	BB_PRINT_PN("[%s], coreid [0x%llx]\n", __func__, g_rdr_ap_root->coreid);
	BB_PRINT_PN("[%s], slice [%llu]\n", __func__, g_rdr_ap_root->slice);
	BB_PRINT_PN("[%s], enter_times [0x%x]\n", __func__, g_rdr_ap_root->enter_times);
	BB_PRINT_PN("[%s], num_reg_regions [0x%x]\n", __func__, g_rdr_ap_root->num_reg_regions);

	for (i = 0; i < g_rdr_ap_root->num_reg_regions; i++)
		BB_PRINT_PN(
			"[%s], reg_name [%s], reg_base [0x%px], reg_size [0x%x], reg_dump_addr [0x%px]\n",
			__func__, regs_info[i].reg_name,
			(void *)(uintptr_t)regs_info[i].reg_base,
			regs_info[i].reg_size,
			regs_info[i].reg_dump_addr);
}

static int check_addr_overflow(const unsigned char *addr)
{
	unsigned char *max_addr = NULL;

	if (!addr) {
		BB_PRINT_ERR("[%s], invalid addr!\n", __func__);
		return -1;
	}

	max_addr = g_rdr_ap_root->rdr_ap_area_map_addr +
		g_rdr_ap_root->ap_rdr_info.log_len;
	if ((addr < g_rdr_ap_root->rdr_ap_area_map_addr) || (addr >= max_addr))
		return -1;

	return 0;
}

/* The 1K space occupied by the struct ap_eh_root is not included */
static unsigned char *get_rdr_hisiap_dump_start_addr(void)
{
	unsigned char *addr = NULL;
	unsigned int times = sizeof(*g_rdr_ap_root) / SIZE_1K + 1;

	addr = g_rdr_ap_root->rdr_ap_area_map_addr + ALIGN(sizeof(*g_rdr_ap_root), times * SIZE_1K);
	BB_PRINT_DBG("[%s], aligned by %u, dump_start_addr [0x%px]\n", __func__, times, addr);
	if (check_addr_overflow(addr)) {
		BB_PRINT_ERR("[%s], there is no space left for ap to dump!\n", __func__);
		return NULL;
	}
	return addr;
}

static int io_resources_init(struct device *dev)
{
	int ret;
	unsigned int i;
	struct resource res;
	struct regs_info *regs = NULL;
	unsigned char *tmp = NULL;

	regs = g_rdr_ap_root->dump_regs_info;
	memset((void *)regs, 0, REGS_DUMP_MAX_NUM * sizeof(*regs));

	ret = device_property_read_u32(dev, "reg-dump-regions", &g_rdr_ap_root->num_reg_regions);
	if (ret) {
		BB_PRINT_PN("[%s], cannot find reg-dump-regions in dts!\n", __func__);
		goto ioinit_fail;
	}

	if (g_rdr_ap_root->num_reg_regions == 0) {
		BB_PRINT_ERR("[%s], reg-dump-regions in zero, so no reg resource to init\n", __func__);
		goto ioinit_fail;
	}

	for (i = 0; i < g_rdr_ap_root->num_reg_regions; i++) {
		if (of_address_to_resource(dev->of_node, i, &res)) {
			BB_PRINT_ERR("[%s], of_addr_to_resource [%u] fail!\n", __func__, i);
			goto ioinit_fail;
		}

		strncpy(regs[i].reg_name, res.name, REG_NAME_LEN - 1);
		regs[i].reg_name[REG_NAME_LEN - 1] = '\0';
		regs[i].reg_base = res.start;
		regs[i].reg_size = resource_size(&res);

		if (regs[i].reg_size == 0) {
			BB_PRINT_ERR("[%s], [%s] registers size is 0, skip map!\n", __func__, (regs[i].reg_name));
			goto reg_dump_addr_init;
		}
		regs[i].reg_map_addr = of_iomap(dev->of_node, i);
		BB_PRINT_DBG("[%s], regs_info[%u].reg_name[%s], reg_base[0x%px], reg_size[0x%x], map_addr[0x%px]\n",
			__func__, i, regs[i].reg_name, (void *)(uintptr_t)regs[i].reg_base, regs[i].reg_size,
			regs[i].reg_map_addr);
		if (!regs[i].reg_map_addr) {
			BB_PRINT_ERR("[%s], unable to map [%s] registers\n", __func__, (regs[i].reg_name));
			goto ioinit_fail;
		}
		BB_PRINT_DBG("[%s], map [%s] registers ok\n", __func__, (regs[i].reg_name));
reg_dump_addr_init:

		if (i == 0) {
			regs[i].reg_dump_addr = get_rdr_hisiap_dump_start_addr();
			if (IS_ERR_OR_NULL(regs[i].reg_dump_addr)) {
				BB_PRINT_ERR("[%s], reg_dump_addr is invalid!\n", __func__);
				goto ioinit_fail;
			}
		} else {
			regs[i].reg_dump_addr = regs[i - 1].reg_dump_addr + regs[i - 1].reg_size;
		}
	}

	tmp = regs[i - 1].reg_dump_addr + regs[i - 1].reg_size;
	if (check_addr_overflow(tmp))
		BB_PRINT_ERR("[%s], there is no space left for ap to dump regs!\n", __func__);

ioinit_fail:
	return 0;
}

static unsigned int get_total_regdump_region_size(const struct regs_info *regs_info, u32 regs_info_len)
{
	unsigned int i;
	u32 total = 0;

	if (!regs_info) {
		BB_PRINT_ERR("[%s],regs_info is null\n", __func__);
		return 0;
	}

	if (regs_info_len < REGS_DUMP_MAX_NUM)
		BB_PRINT_ERR("[%s],regs_info_len is too small\n", __func__);

	for (i = 0; i < g_rdr_ap_root->num_reg_regions; i++)
		total += regs_info[i].reg_size;

	BB_PRINT_DBG("[%s], num_reg_regions [%u], regdump size [0x%x]\n",
		__func__, g_rdr_ap_root->num_reg_regions, total);
	return total;
}

/*
 * The interrupt, task, and cpuidle are divided based on the CPU frequency,
 * nitialization of other track areas that do not differentiate CPUs
 */
int hook_buffer_alloc(void)
{
	int ret;

	BB_PRINT_DBG("[%s], irq_buffer_init start!\n", __func__);
	ret = irq_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_IRQ],
		g_rdr_ap_root->hook_buffer_addr[HK_IRQ], g_dump_buffer_size_tbl[HK_IRQ]);
	if (ret) {
		BB_PRINT_ERR("[%s], irq_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], task_buffer_init start!\n", __func__);
	ret = task_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_TASK],
		g_rdr_ap_root->hook_buffer_addr[HK_TASK], g_dump_buffer_size_tbl[HK_TASK]);
	if (ret) {
		BB_PRINT_ERR("[%s], task_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], cpuidle_buffer_init start!\n", __func__);
	ret = cpuidle_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_CPUIDLE],
		g_rdr_ap_root->hook_buffer_addr[HK_CPUIDLE], g_dump_buffer_size_tbl[HK_CPUIDLE]);
	if (ret) {
		BB_PRINT_ERR("[%s], cpuidle_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], worker_buffer_init start!\n", __func__);
	ret = worker_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_WORKER],
		g_rdr_ap_root->hook_buffer_addr[HK_WORKER], g_dump_buffer_size_tbl[HK_WORKER]);
	if (ret) {
		BB_PRINT_ERR("[%s], worker_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], mem_alloc_buffer_init start!\n", __func__);
	ret = mem_alloc_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_MEM_ALLOCATOR],
		g_rdr_ap_root->hook_buffer_addr[HK_MEM_ALLOCATOR], g_dump_buffer_size_tbl[HK_MEM_ALLOCATOR]);
	if (ret) {
		BB_PRINT_ERR("[%s], mem_alloc_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], ion_alloc_buffer_init start!\n", __func__);
	ret = ion_alloc_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_ION_ALLOCATOR],
		g_rdr_ap_root->hook_buffer_addr[HK_ION_ALLOCATOR], g_dump_buffer_size_tbl[HK_ION_ALLOCATOR]);
	if (ret) {
		BB_PRINT_ERR("[%s], ion_alloc_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], time_buffer_init start!\n", __func__);
	ret = time_buffer_init(&g_rdr_ap_root->hook_percpu_buffer[HK_TIME],
			g_rdr_ap_root->hook_buffer_addr[HK_TIME], g_dump_buffer_size_tbl[HK_TIME]);
	if (ret) {
		BB_PRINT_ERR("[%s], time_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], cpu_onoff_buffer_init start!\n", __func__);
	ret = cpu_onoff_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_CPU_ONOFF],
			g_dump_buffer_size_tbl[HK_CPU_ONOFF]);
	if (ret) {
		BB_PRINT_ERR("[%s], cpu_onoff_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], syscall_buffer_init start!\n", __func__);
	ret = syscall_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_SYSCALL], g_dump_buffer_size_tbl[HK_SYSCALL]);
	if (ret) {
		BB_PRINT_ERR("[%s], syscall_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], hung_task_buffer_init start!\n", __func__);
	ret = hung_task_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_HUNGTASK], g_dump_buffer_size_tbl[HK_HUNGTASK]);
	if (ret) {
		BB_PRINT_ERR("[%s], hung_task_buffer_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], tasklet_buffer_init start!\n", __func__);
	ret = tasklet_buffer_init(&g_rdr_ap_root->hook_buffer_addr[HK_TASKLET], g_dump_buffer_size_tbl[HK_TASKLET]);
	if (ret) {
		BB_PRINT_ERR("[%s], tasklet_buffer_init failed!\n", __func__);
		return ret;
	}

	return 0;
}

static void rdr_hisiap_print_all_dump_addrs(void)
{
	unsigned int i;

	if (IS_ERR_OR_NULL(g_rdr_ap_root)) {
		BB_PRINT_ERR("[%s], g_rdr_ap_root [0x%px] is invalid\n", __func__, g_rdr_ap_root);
		return;
	}

	for (i = 0; i < g_rdr_ap_root->num_reg_regions; i++)
		BB_PRINT_DBG("[%s], reg_name [%s], reg_dump_addr [0x%px]\n",
			__func__,
			g_rdr_ap_root->dump_regs_info[i].reg_name,
			g_rdr_ap_root->dump_regs_info[i].reg_dump_addr);

	BB_PRINT_DBG("[%s], rdr_ap_area_map_addr [0x%px]\n",
			__func__, g_rdr_ap_root->rdr_ap_area_map_addr);
	BB_PRINT_DBG("[%s], kirq_switch_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_IRQ]);
	BB_PRINT_DBG("[%s], ktask_switch_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_TASK]);
	BB_PRINT_DBG("[%s], cpu_on_off_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_CPU_ONOFF]);
	BB_PRINT_DBG("[%s], cpu_idle_stat_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_CPUIDLE]);
	BB_PRINT_DBG("[%s], worker_trace_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_WORKER]);
	BB_PRINT_DBG("[%s], mem_allocator_trace_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_MEM_ALLOCATOR]);
	BB_PRINT_DBG("[%s], ion_allocator_trace_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_ION_ALLOCATOR]);
	BB_PRINT_DBG("[%s], time_trace_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_TIME]);
	BB_PRINT_DBG("[%s], syscall_trace_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_SYSCALL]);
	BB_PRINT_DBG("[%s], hung_task_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_HUNGTASK]);
	BB_PRINT_DBG("[%s], tasklet_trace_addr [0x%px]\n", __func__,
			g_rdr_ap_root->hook_buffer_addr[HK_TASKLET]);

	for (i = 0; i < PLAT_AP_HOOK_CPU_NUMBERS; i++)
		BB_PRINT_DBG("[%s], last_task_stack_dump_addr[%u] [0x%px]\n",
			__func__, i, g_rdr_ap_root->last_task_stack_dump_addr[i]);

	for (i = 0; i < PLAT_AP_HOOK_CPU_NUMBERS; i++)
		BB_PRINT_DBG("[%s], last_task_struct_dump_addr[%u] [0x%px]\n",
			__func__, i, g_rdr_ap_root->last_task_struct_dump_addr[i]);

	for (i = 0; i < MODU_MAX; i++) {
		if (g_rdr_ap_root->module_dump_info[i].dump_size != 0)
			BB_PRINT_DBG("[%s], module_dump_info[%u].dump_addr [0x%px]\n",
				__func__, i, g_rdr_ap_root->module_dump_info[i].dump_addr);
	}
}

static void module_dump_mem_init(struct device *dev)
{
	int i;

	for (i = 0; i < MODU_MAX; i++) {
		if (i == 0) {
			g_rdr_ap_root->module_dump_info[0].dump_addr =
				g_rdr_ap_root->last_task_stack_dump_addr[PLAT_AP_HOOK_CPU_NUMBERS - 1] +
				g_last_task_stack_size;
		} else {
			g_rdr_ap_root->module_dump_info[i].dump_addr =
				g_rdr_ap_root->module_dump_info[i - 1].dump_addr +
				g_dump_modu_mem_size_tbl[i - 1];
		}

		if (check_addr_overflow(g_rdr_ap_root->module_dump_info[i].dump_addr +
				g_dump_modu_mem_size_tbl[i])) {
			BB_PRINT_ERR("[%s], there is no enough space for modu [%d] to dump mem!\n", __func__, i);
			break;
		}
		g_rdr_ap_root->module_dump_info[i].dump_size = g_dump_modu_mem_size_tbl[i];

		BB_PRINT_DBG("[%s], dump_addr [0x%px] dump_size [0x%x]!\n",
				__func__,
				g_rdr_ap_root->module_dump_info[i].dump_addr,
				g_rdr_ap_root->module_dump_info[i].dump_size);
	}
	/*
	 * When ap_last_task_switch is disabled,
	 * the start address of the dump memory of the last_task struct and stack is set to 0
	 */
	if (!get_ap_last_task_switch_from_dts(dev)) {
		for (i = 0; i < PLAT_AP_HOOK_CPU_NUMBERS; i++) {
			g_rdr_ap_root->last_task_struct_dump_addr[i] = 0;
			g_rdr_ap_root->last_task_stack_dump_addr[i] = 0;
		}
	}
}

static int __init ap_dump_buffer_init(struct device *dev)
{
	int i;

	/* Track record area */
	g_rdr_ap_root->hook_buffer_addr[0] = get_rdr_hisiap_dump_start_addr() +
		get_total_regdump_region_size(g_rdr_ap_root->dump_regs_info, REGS_DUMP_MAX_NUM);
	BB_PRINT_DBG("[%s], hook_buf0_addr=0x%px, size=0x%x \n",
		__func__, g_rdr_ap_root->hook_buffer_addr[0], g_dump_buffer_size_tbl[0]);

	for (i = 1; i < HK_MAX; i++) {
		g_rdr_ap_root->hook_buffer_addr[i] =
			g_rdr_ap_root->hook_buffer_addr[i - 1] + g_dump_buffer_size_tbl[i - 1];
		BB_PRINT_DBG("[%s], hook_buf%d_addr=0x%px, size=0x%x \n",
			__func__, i, g_rdr_ap_root->hook_buffer_addr[i], g_dump_buffer_size_tbl[i]);
	}

	/* Task record area */
	if (get_ap_last_task_switch_from_dts(dev)) {
		g_last_task_struct_size = sizeof(struct task_struct);
		g_last_task_stack_size = THREAD_SIZE;
	}

	g_rdr_ap_root->last_task_struct_dump_addr[0] =
		g_rdr_ap_root->hook_buffer_addr[HK_MAX - 1] + g_dump_buffer_size_tbl[HK_MAX - 1];
	BB_PRINT_DBG("[%s], last_task0_addr=0x%px, size=0x%x \n",
		__func__, g_rdr_ap_root->last_task_struct_dump_addr[0], g_last_task_struct_size);


	for (i = 1; i < PLAT_AP_HOOK_CPU_NUMBERS; i++) {
		g_rdr_ap_root->last_task_struct_dump_addr[i] =
			g_rdr_ap_root->last_task_struct_dump_addr[i - 1] + g_last_task_struct_size;

		BB_PRINT_DBG("[%s], last_task%d_addr=0x%px, size=0x%x \n",
			__func__, i, g_rdr_ap_root->last_task_struct_dump_addr[i], g_last_task_struct_size);
	}

	g_rdr_ap_root->last_task_stack_dump_addr[0] = (unsigned char *)(uintptr_t)
		ALIGN(((uintptr_t)g_rdr_ap_root->last_task_struct_dump_addr[PLAT_AP_HOOK_CPU_NUMBERS - 1] +
				g_last_task_struct_size), SIZE_1K); /* Aligned by 1K */
	BB_PRINT_DBG("[%s], last_stack0_addr=0x%px, size=0x%x \n",
		__func__, g_rdr_ap_root->last_task_stack_dump_addr[0], g_last_task_stack_size);

	for (i = 1; i < PLAT_AP_HOOK_CPU_NUMBERS; i++) {
		g_rdr_ap_root->last_task_stack_dump_addr[i] =
			g_rdr_ap_root->last_task_stack_dump_addr[i - 1] +
			g_last_task_stack_size;
		BB_PRINT_DBG("[%s], last_stack%d_addr=0x%px, size=0x%x \n", __func__,
			i, g_rdr_ap_root->last_task_stack_dump_addr[i], g_last_task_stack_size);
	}

	if (check_addr_overflow(g_rdr_ap_root->last_task_stack_dump_addr[PLAT_AP_HOOK_CPU_NUMBERS - 1] +
		g_last_task_stack_size)) {
		BB_PRINT_ERR("[%s], there is no enough space for ap to dump!\n", __func__);
		return -ENOSPC;
	}
	BB_PRINT_PN("[%s], module_dump_mem_init start\n", __func__);
	module_dump_mem_init(dev);

	rdr_hisiap_print_all_dump_addrs();
	return hook_buffer_alloc();
}

// need implement according to platform (vincent).
u64 get_32k_abs_timer_value(void)
{
	return 0;
}

void regs_dump(void)
{
	unsigned int i;
	struct regs_info *regs_info = NULL;

	regs_info = g_rdr_ap_root->dump_regs_info;

	/*
	 * NOTE:sctrl in the power-on area, pctrl, pctrl, pericrg in the peripheral area,
	 * Do not check the domain when accessing the A core
	 */
	for (i = 0; i < g_rdr_ap_root->num_reg_regions; i++) {
		if (IS_ERR_OR_NULL(regs_info[i].reg_map_addr) ||
			IS_ERR_OR_NULL(regs_info[i].reg_dump_addr)) {
			regs_info[i].reg_dump_addr = 0;
			BB_PRINT_ERR(
				"[%s], regs_info[%u].reg_map_addr [%px] reg_dump_addr [%px] invalid!\n",
				__func__, i, regs_info[i].reg_map_addr,
				regs_info[i].reg_dump_addr);
			continue;
		}
		BB_PRINT_PN(
			"[%s], memcpy [0x%x] size from regs_info[%u].reg_map_addr [%px] to reg_dump_addr [%px]\n",
			__func__, regs_info[i].reg_size, i,
			regs_info[i].reg_map_addr,
			regs_info[i].reg_dump_addr);
		rdr_regs_dump(regs_info[i].reg_dump_addr,
			regs_info[i].reg_map_addr,
			regs_info[i].reg_size);
	}
}



/*
 * Description:    Memory dump registration interface provided for the AP maintenance
 *                 and test module and IP address
 * Input:          func:Registered dump function, module_name, mod
 * Output:         NA
 * Return:         0:Registration succeeded, <0:fail
 */
int register_module_dump_mem_func(rdr_hisiap_dump_func_ptr func,
				const char *module_name,
				dump_mem_module modu)
{
	int ret = -1;

	if (modu >= MODU_MAX) {
		BB_PRINT_ERR("[%s], modu [%u] is invalid!\n", __func__, modu);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(func)) {
		BB_PRINT_ERR("[%s], func [0x%px] is invalid!\n", __func__, func);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(module_name)) {
		BB_PRINT_ERR("[%s], module_name is invalid!\n", __func__);
		return -EINVAL;
	}

	if (!g_rdr_ap_root) {
		BB_PRINT_ERR("[%s], g_rdr_ap_root is null!\n", __func__);
		return -1;
	}

	BB_PRINT_PN("[%s], module_name [%s]\n", __func__, module_name);
	mutex_lock(&g_dump_mem_mutex);
	if (g_rdr_ap_root->module_dump_info[modu].dump_size != 0) {
		g_rdr_ap_root->module_dump_info[modu].dump_funcptr = func;
		strncpy(g_rdr_ap_root->module_dump_info[modu].module_name,
			module_name, AMNTN_MODULE_NAME_LEN - 1);
		g_rdr_ap_root->module_dump_info[modu].module_name[AMNTN_MODULE_NAME_LEN - 1] = '\0';
		ret = 0;
	}
	mutex_unlock(&g_dump_mem_mutex);

	if (ret)
		BB_PRINT_ERR(
			"[%s], func[0x%px], size[%u], [%s] register failed!\n",
			__func__, func,
			g_rdr_ap_root->module_dump_info[modu].dump_size,
			module_name);
	return ret;
}

/*
 * Description:  Obtains the dump start address of the dump module
 * Input:        modu:Module ID,This is a unified allocation;
 * Output:       dump_addr:Start address of the dump memory allocated to the module MODU
 * Return:       0:The data is successfully obtained ,Smaller than 0:Obtaining failed
 */
int get_module_dump_mem_size(dump_mem_module modu, u32 *size_addr)
{
	if (!rdr_get_ap_init_done()) {
		BB_PRINT_ERR("[%s]rdr not init\n", __func__);
		return -EPERM;
	}

	if (modu >= MODU_MAX) {
		BB_PRINT_ERR("[%s]modu [%u] is invalid\n", __func__, modu);
		return -EINVAL;
	}

	if (!size_addr) {
		BB_PRINT_ERR("[%s]size_addr is invalid\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_dump_mem_mutex);
	if (g_rdr_ap_root->module_dump_info[modu].dump_size == 0) {
		BB_PRINT_ERR("[%s]modu[%u] dump_size is zero\n", __func__, modu);
		mutex_unlock(&g_dump_mem_mutex);
		return -EPERM;
	}
	*size_addr = g_rdr_ap_root->module_dump_info[modu].dump_size;

	if (!(*size_addr)) {
		BB_PRINT_ERR("[%s] *size_addr is invalid\n", __func__);
		mutex_unlock(&g_dump_mem_mutex);
		return -EINVAL;
	}

	mutex_unlock(&g_dump_mem_mutex);

	return 0;
}

/*
 * Description:  Obtains the dump start address of the dump module
 * Input:        modu:Module ID,This is a unified allocation;
 * Output:       dump_addr:Start address of the dump memory allocated to the module MODU
 * Return:       0:The data is successfully obtained ,Smaller than 0:Obtaining failed
 */
int get_module_dump_mem_addr(dump_mem_module modu, unsigned char **dump_addr)
{
	if (!rdr_get_ap_init_done()) {
		BB_PRINT_ERR("[%s]rdr not init\n", __func__);
		return -EPERM;
	}

	if (modu >= MODU_MAX) {
		BB_PRINT_ERR("[%s]modu [%u] is invalid\n", __func__, modu);
		return -EINVAL;
	}

	if (!dump_addr) {
		BB_PRINT_ERR("[%s]dump_addr is invalid\n", __func__);
		return -EINVAL;
	}

	mutex_lock(&g_dump_mem_mutex);
	if (g_rdr_ap_root->module_dump_info[modu].dump_size == 0) {
		BB_PRINT_ERR("[%s]modu[%u] dump_size is zero\n", __func__, modu);
		mutex_unlock(&g_dump_mem_mutex);
		return -EPERM;
	}
	*dump_addr = g_rdr_ap_root->module_dump_info[modu].dump_addr;

	if (!(*dump_addr)) {
		BB_PRINT_ERR("[%s] *dump_addr is invalid\n", __func__);
		mutex_unlock(&g_dump_mem_mutex);
		return -EINVAL;
	}

	mutex_unlock(&g_dump_mem_mutex);

	return 0;
}

uint32_t get_hook_buffer_size(enum hook_type type)
{
	if (type >= HK_MAX)
		return -EINVAL;

	return g_dump_buffer_size_tbl[type];
}

/*
 * Description:   Before the abnormal reset, the AP maintenance and test
 *                module and the memory dump registration function provided
 *                by the IP are invoked
 */
void save_module_dump_mem(void)
{
	int i;
	void *addr = NULL;
	unsigned int size = 0;

	BB_PRINT_PN("[%s], enter\n", __func__);
	for (i = 0; i < MODU_MAX; i++) {
		if (g_rdr_ap_root->module_dump_info[i].dump_funcptr != NULL) {
			addr = (void *)g_rdr_ap_root->module_dump_info[i].dump_addr;
			size = g_rdr_ap_root->module_dump_info[i].dump_size;
			if (g_rdr_ap_root->module_dump_info[i].dump_funcptr(addr, size)) {
				BB_PRINT_ERR("[%s], [%s] dump failed!\n", __func__,
					g_rdr_ap_root->module_dump_info[i].module_name);
				continue;
			}
		}
	}
	BB_PRINT_PN("[%s], exit\n", __func__);
}

int rdr_hisiap_dump_init(const struct rdr_register_module_result *retinfo, struct device *dev)
{
	int ret = 0;

	BB_PRINT_DBG("[%s], begin\n", __func__);

	if (!retinfo) {
		BB_PRINT_ERR("%s():%d:retinfo is NULL!\n", __func__, __LINE__);
		return -1;
	}

	g_rdr_ap_root = (struct ap_eh_root *)(uintptr_t)g_hisiap_addr;
	strncpy(g_log_path, g_rdr_ap_root->log_path, LOG_PATH_LEN - 1);
	g_log_path[LOG_PATH_LEN - 1] = '\0';

	memset((void *)(uintptr_t)g_hisiap_addr, 0, retinfo->log_len);
	g_rdr_ap_root = (struct ap_eh_root *)(uintptr_t) g_hisiap_addr;
	g_rdr_ap_root->ap_rdr_info.log_addr = retinfo->log_addr;
	g_rdr_ap_root->ap_rdr_info.log_len = retinfo->log_len;
	g_rdr_ap_root->ap_rdr_info.nve = retinfo->nve;
	g_rdr_ap_root->rdr_ap_area_map_addr = (void *)(uintptr_t)g_hisiap_addr;
//	get_device_platform(g_rdr_ap_root->device_id, PRODUCT_DEVICE_LEN);
	g_rdr_ap_root->bbox_version = BBOX_VERSION;
	g_rdr_ap_root->dump_magic = AP_DUMP_MAGIC;
	g_rdr_ap_root->end_magic = AP_DUMP_END_MAGIC;

	BB_PRINT_DBG("[%s], io_resources_init start\n", __func__);
	ret = io_resources_init(dev);
	if (ret) {
		BB_PRINT_ERR("[%s], io_resources_init failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], get_ap_trace_mem_size_from_dts start\n", __func__);
	ret = get_ap_trace_mem_size_from_dts(dev);
	if (ret) {
		BB_PRINT_ERR("[%s], get_ap_trace_mem_size_from_dts failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], get_ap_dump_mem_modu_size_from_dts start\n", __func__);
	ret = get_ap_dump_mem_modu_size_from_dts(dev);
	if (ret) {
		BB_PRINT_ERR("[%s], get_ap_dump_mem_modu_size_from_dts failed!\n", __func__);
		return ret;
	}

	BB_PRINT_DBG("[%s], ap_dump_buffer_init start\n", __func__);
	ret = ap_dump_buffer_init(dev);
	if (ret) {
		BB_PRINT_ERR("[%s], ap_dump_buffer_init failed!\n", __func__);
		return ret;
	}

	/* Whether the track is generated is consistent with the kernel dump */
	if (check_himntn(HIMNTN_KERNEL_DUMP_ENABLE)) {
		BB_PRINT_DBG("[%s], hisi_trace_hook_install start\n", __func__);
		ret = hisi_trace_hook_install();
		if (ret) {
			BB_PRINT_ERR("[%s], hisi_trace_hook_install failed!\n", __func__);
			return ret;
		}
	} else {
		BB_PRINT_DBG("[%s], hisi_ap_defopen_hook_install start\n", __func__);
		hisi_ap_defopen_hook_install();
	}
	BB_PRINT_DBG("[%s], end\n", __func__);
	return ret;
}

void rdr_hisiap_dump_root_head(u32 modid, u32 etype, u64 coreid)
{
	if (!g_rdr_ap_root) {
		BB_PRINT_ERR("[%s], exit!\n", __func__);
		return;
	}

	g_rdr_ap_root->modid = modid;
	g_rdr_ap_root->e_exce_type = etype;
	g_rdr_ap_root->coreid = coreid;
	g_rdr_ap_root->slice = 0;
	g_rdr_ap_root->enter_times++;
}

void rdr_hisiap_dump(u32 modid, u32 etype,
				u64 coreid, char *log_path,
				pfn_cb_dump_done pfn_cb)
{
	uintptr_t exception_info = 0;
	unsigned long exception_info_len = 0;

	BB_PRINT_PN("[%s], begin\n", __func__);
	BB_PRINT_PN("modid is 0x%x, etype is 0x%x\n", modid, etype);

	if (!g_rdr_ap_init) {
		BB_PRINT_ERR("rdr_hisi_ap_adapter is not ready\n");
		if (pfn_cb)
			pfn_cb(modid, coreid);
		return;
	}

	if (modid == RDR_MODID_AP_ABNORMAL_REBOOT ||
		modid == BBOX_MODID_LAST_SAVE_NOT_DONE) {
		BB_PRINT_PN("RDR_MODID_AP_ABNORMAL_REBOOT\n");
		if (log_path && check_himntn(HIMNTN_GOBAL_RESETLOG)) {
			strncpy(g_log_path, log_path, LOG_PATH_LEN - 1);
			g_log_path[LOG_PATH_LEN - 1] = '\0';

			save_hisiap_log(log_path, modid);
		}

		if (pfn_cb)
			pfn_cb(modid, coreid);
		return;
	}

	console_loglevel = RDR_CONSOLE_LOGLEVEL_DEFAULT;

	/* If etype is panic, record the pc pointer and transfer it to the fastboot */
	if ((etype == AP_S_PANIC || etype == AP_S_VENDOR_PANIC) && g_bbox_ap_record_pc) {
		get_exception_info(&exception_info, &exception_info_len);
		memset(g_bbox_ap_record_pc->exception_info, 0,
			RECORD_PC_STR_MAX_LENGTH);
		memcpy(g_bbox_ap_record_pc->exception_info, (char *)exception_info, exception_info_len);
		BB_PRINT_PN("exception_info is [%s],len is [%lu]\n",
			(char *)exception_info, exception_info_len);
		g_bbox_ap_record_pc->exception_info_len = exception_info_len;
	}

	BB_PRINT_PN("%s modid[%x],etype[%x],coreid[%llx], log_path[%s]\n", __func__,
		modid, etype, coreid, log_path);
	BB_PRINT_PN("[%s], hisi_trace_hook_uninstall start!\n", __func__);
	hisi_trace_hook_uninstall();

	if (!g_rdr_ap_root)
		goto out;

	g_rdr_ap_root->modid = modid;
	g_rdr_ap_root->e_exce_type = etype;
	g_rdr_ap_root->coreid = coreid;
	BB_PRINT_PN("%s log_path [%s]\n", __func__, g_rdr_ap_root->log_path);
	if (log_path) {
		strncpy(g_rdr_ap_root->log_path, log_path, LOG_PATH_LEN - 1);
		g_rdr_ap_root->log_path[LOG_PATH_LEN - 1] = '\0';
	}

	g_rdr_ap_root->slice = get_32k_abs_timer_value();

	BB_PRINT_PN("[%s], regs_dump start!\n", __func__);
	regs_dump();

	BB_PRINT_PN("[%s], last_task_stack_dump start!\n", __func__);
	last_task_stack_dump();

	g_rdr_ap_root->enter_times++;

	BB_PRINT_PN("[%s], save_module_dump_mem start!\n", __func__);
	save_module_dump_mem();

	print_debug_info();

	show_irq_register();
out:
	BB_PRINT_PN("[%s], exit!\n", __func__);
	if (pfn_cb)
		pfn_cb(modid, coreid);
}

/*
 * Description : register exceptionwith to rdr
 */
static void rdr_hisiap_register_exception(void)
{
	unsigned int i;
	u32 ret;

	BB_PRINT_START();
	for (i = 0; i < sizeof(g_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		BB_PRINT_DBG("register exception:%u", g_einfo[i].e_exce_type);
		g_einfo[i].e_callback = hisiap_callback;
		if (i == 0)
			/*
			 * Register the common callback function of the AP core.
			 * If other cores notify the ap core dump, invoke the callback function.
			 * RDR_COMMON_CALLBACK is a public callback tag,
			 * and ap core is a private callback function that is not marked
			 */
			g_einfo[i].e_callback = (rdr_e_callback)(uintptr_t)(
				(uintptr_t)(g_einfo[i].e_callback) | BBOX_COMMON_CALLBACK);
		ret = rdr_register_exception(&g_einfo[i]);
		if (ret == 0)
			BB_PRINT_ERR("rdr_register_exception fail, ret = [%u]\n", ret);
	}
	BB_PRINT_END();
}

/*
 * Description : Register the dump and reset functions to the rdr
 */
static int rdr_hisiap_register_core(void)
{
	struct rdr_module_ops_pub s_soc_ops;
	struct rdr_register_module_result retinfo;
	int ret;
	u64 coreid = RDR_AP;

	s_soc_ops.ops_dump = rdr_hisiap_dump;
	s_soc_ops.ops_reset = rdr_hisiap_reset;

	ret = rdr_register_module_ops(coreid, &s_soc_ops, &retinfo);
	if (ret < 0) {
		BB_PRINT_ERR("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	ret = rdr_register_cleartext_ops(coreid, rdr_hisiap_cleartext_print);
	if (ret < 0) {
		BB_PRINT_ERR("rdr_register_cleartext_ops fail, ret = [%d]\n", ret);
		return ret;
	}

	g_current_info.log_addr = retinfo.log_addr;
	g_current_info.log_len = retinfo.log_len;
	g_current_info.nve = retinfo.nve;
	BB_PRINT_DBG("%s,%d: addr=0x%llx, len=0x%x\n",
		__func__, __LINE__, g_current_info.log_addr, g_current_info.log_len);

	return ret;
}

/*
 * Description : Obtains the initialization status
 */
bool rdr_get_ap_init_done(void)
{
	return g_rdr_ap_init == 1;
}

/*
 * Description:    Saving exception information
 * Input:          void *arg Not Used now
 * Return:         -1 : error, 0 : ok
 */
int save_exception_info(void *arg)
{
	int ret;
	char exce_dir[LOG_PATH_LEN];

	BB_PRINT_ERR("[%s], start\n", __func__);

	/* Obtain the path of the abnormal log directory from the global variable */
	memset(exce_dir, 0, LOG_PATH_LEN);

	if (LOG_PATH_LEN - 1 < strlen(g_log_path)) {
		BB_PRINT_ERR("g_log_path's len too large\n");
		return -1;
	}
	memcpy(exce_dir, g_log_path, strlen(g_log_path));
	BB_PRINT_ERR("exce_dir is [%s]\n", exce_dir);

	/*
	 * If the input parameter of the function is null,
	 * it indicates that the function is invoked in rdr_hisiap_dump
	 */
	if (!arg)
		return ret;

	/* Create the DONE file in the abnormal directory, indicating that the exception log is saved done */
	bbox_save_done(g_log_path, BBOX_SAVE_STEP_DONE);

	/* The file system sync ensures that the read and write tasks are complete */
	if (!in_atomic() && !irqs_disabled() && !in_irq())
		ksys_sync();

	/*
	 * According to the permission requirements, the group of the hisi_logs directory
	 * and its subdirectories is changed to root-system
	 */
	ret = (int)rdr_chown((const char __user *)g_log_path, ROOT_UID, SYSTEM_GID, true);
	if (ret)
		BB_PRINT_ERR("[%s], chown %s uid [%u] gid [%u] failed err [%d]!\n",
			__func__, PATH_ROOT, ROOT_UID, SYSTEM_GID, ret);
	return ret;
}

static const struct of_device_id rdr_ap_of_match[] = {
	{ .compatible = "rdr,rdr_ap_adapter" },
	{}
};

static int rdr_hisiap_probe(struct platform_device *pdev)
{
	int ret;

	BB_PRINT_DBG("%s init start\n", __func__);

	mutex_init(&g_dump_mem_mutex);

	rdr_hisiap_register_exception();
	ret = rdr_hisiap_register_core();
	if (ret) {
		BB_PRINT_ERR("%s rdr_hisiap_register_core fail, ret = [%d]\n", __func__, ret);
		return ret;
	}

	ret = register_mntn_dump(MNTN_DUMP_PANIC, sizeof(AP_RECORD_PC), (void **)&g_bbox_ap_record_pc);
	if (ret) {
		BB_PRINT_ERR("%s register g_bbox_ap_record_pc fail\n", __func__);
		return ret;
	}

	if (!g_bbox_ap_record_pc)
		BB_PRINT_ERR("%s g_bbox_ap_record_pc is NULL\n", __func__);

	g_hisiap_addr =
		(uintptr_t)rdr_bbox_map(g_current_info.log_addr, g_current_info.log_len);
	if (!g_hisiap_addr) {
		BB_PRINT_ERR("hisi_bbox_map g_hisiap_addr fail\n");
		return -1;
	}
	BB_PRINT_DBG("[%s], retinfo: log_addr [0x%llx][0x%llx]",
		__func__, g_current_info.log_addr, g_hisiap_addr);

	ret = rdr_hisiap_dump_init(&g_current_info, &pdev->dev);
	if (ret) {
		BB_PRINT_ERR("%s rdr_hisiap_dump_init fail, ret = [%d]\n", __func__, ret);
		return -1;
	}

	atomic_notifier_chain_register(&panic_notifier_list, &acpu_panic_loop_block);
	atomic_notifier_chain_register(&panic_notifier_list, &rdr_hisiap_panic_block);

	panic_on_oops = 1;
	register_die_notifier(&rdr_hisiap_die_block);
	g_rdr_ap_init = 1;
	BB_PRINT_DBG("%s init end\n", __func__);

	return 0;
}

static int rdr_hisiap_remove(struct platform_device *pdev)
{
	BB_PRINT_PN("%s\n", __func__);
	return 0;
}

static struct platform_driver rdr_ap_driver = {
	.driver		= {
		.name			= "rdr ap driver",
		.of_match_table		= rdr_ap_of_match,
	},
	.probe		= rdr_hisiap_probe,
	.remove		= rdr_hisiap_remove,
};

/*
 * Description : Initialization Function
 */
int __init rdr_hisiap_init(void)
{
	platform_driver_register(&rdr_ap_driver);

	return 0;
}

static void __exit rdr_hisiap_exit(void)
{
	platform_driver_unregister(&rdr_ap_driver);
}

module_init(rdr_hisiap_init);
module_exit(rdr_hisiap_exit);

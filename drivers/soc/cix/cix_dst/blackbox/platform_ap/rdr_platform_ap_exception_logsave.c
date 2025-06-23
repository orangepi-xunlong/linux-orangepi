/*
 * rdr_hisi_ap_exception_logsave.c
 *
 * Based on the RDR framework, adapt to the AP side to implement resource
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
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
#include "rdr_platform_ap_exception_logsave.h"
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
#include <linux/module.h>
#include <linux/stacktrace.h>
#include <linux/kallsyms.h>
#include <linux/blkdev.h>
#include <linux/reboot.h>
#include <linux/soc/cix/util.h>
#include <linux/soc/cix/rdr_pub.h>
#include "../rdr_inner.h"
#include <linux/soc/cix/mntn_dump.h>
#include <linux/watchdog.h>
#include <mntn_subtype_exception.h>
#include "../rdr_print.h"
#include "../rdr_utils.h"
#include <linux/soc/cix/dst_reboot_reason.h>
#include <linux/kexec.h>
#include <linux/console.h>

static char exception_buf[KSYM_SYMBOL_LEN] __attribute__((__section__(".data")));
static unsigned long exception_buf_len __attribute__((__section__(".data")));

/*
 * Description:    Sets the abnormal pc that invokes the __show_regs.
 * Input:          buf:Address of the abnormal pc that invokes the __show_regs.
 */
void set_exception_info(unsigned long address)
{
	memset(exception_buf, 0, sizeof(exception_buf));

	exception_buf_len = sprint_symbol(exception_buf, address);
}

/*
 * Description:    Obtains the abnormal pc that invokes the __show_regs
 * Input:          NA
 * Output:         buf:Address of the abnormal pc that invokes the __show_regs.
 *                 buf_len:Obtains buf long
 * Return:         NA
 */
void get_exception_info(unsigned long *buf, unsigned long *buf_len)
{
	if (unlikely(!buf || !buf_len))
		return;

	*buf = (uintptr_t)exception_buf;
	*buf_len = exception_buf_len;
}

/*
 * Replacement for kernel memcpy function to solve KASAN problem in rdr stack dump flow.
 * @dest: Where to copy to
 * @src: Where to copy from
 * @count: The size of the area.
 *
 * You should not use this function to access IO space, use memcpy_toio()
 * or memcpy_fromio() instead.
 */

/*
 * Description:  Read the ap_last_task memory allocation switch from the dts.
 * Return:       0:The read fails or the switch is turned off, non 0:The switch is turned on.
 */
unsigned int get_ap_last_task_switch_from_dts(struct device *dev)
{
	int ret;
	unsigned int ap_last_task_switch;

	ret = device_property_read_u32(dev, "ap_last_task_switch", &ap_last_task_switch);
	if (ret) {
		BB_PRINT_ERR("[%s], cannot find ap_last_task_switch in dts!\n", __func__);
		return 0;
	}

	return ap_last_task_switch;
}

void rdr_regs_dump(void *dest, const void *src, size_t len)
{
	size_t remain, mult, i;
	u64 *u64_dst = NULL;
	const u64 *u64_src = NULL;

	remain  = len % sizeof(*u64_dst);
	mult    = len / sizeof(*u64_src);
	u64_dst = dest;
	u64_src = src;

	for (i = 0; i < mult; i++)
		*(u64_dst++) = *(u64_src++);

	for (i = 0; i < remain; i++)
		*((u8 *)u64_dst + i) = *((u8 *)u64_src + i);
}

int hisi_trace_hook_install(void)
{
	int ret = 0;
	enum hook_type hk;

	for (hk = HK_IRQ; hk < HK_MAX; hk++) {
		ret = hisi_ap_hook_install(hk);
		if (ret) {
			BB_PRINT_ERR("[%s], hook_type [%u] install failed!\n", __func__, hk);
			return ret;
		}
	}
	return ret;
}

void hisi_trace_hook_uninstall(void)
{
	enum hook_type hk;

	for (hk = HK_IRQ; hk < HK_MAX; hk++)
		hisi_ap_hook_uninstall(hk);
}

/*
 * Description:    save the log from file_node
 * Input:          arg:if arg is null, the func is called hisiap_dump
 * Return:         0:success;
 */
int save_mntndump_log(void *arg)
{
	int ret;

	if (!check_himntn(HIMNTN_GOBAL_RESETLOG))
		return 0;

	while (rdr_wait_partition(PATH_MNTN_PARTITION, RDR_WAIT_PARTITION_TIME) != 0);

	ret = save_exception_info(arg);
	if (ret)
		BB_PRINT_ERR("save_exception_info fail, ret=%d\n", ret);

	return ret;
}

/*
 * Description:   After phone reboots, save hisiap_log
 * Input:         log_path:path;modid:excep_id
 */
void save_hisiap_log(char *log_path, u32 modid)
{
	struct rdr_exception_info_s temp;
	int ret, path_root_len;
	bool is_save_done = false;

	if (!log_path) {
		BB_PRINT_ERR("%s():%d:log_path is NULL!\n", __func__, __LINE__);
		return;
	}

	temp.e_notify_core_mask = RDR_AP;
	temp.e_reset_core_mask = RDR_AP;
	temp.e_from_core = RDR_AP;
	temp.e_exce_type = rdr_get_reboot_type();
	temp.e_exce_subtype = rdr_get_exec_subtype_value();

	path_root_len = strlen(PATH_ROOT);

	/* if last save not done, need to add "last_save_not_done" in history.log */
	if (modid == BBOX_MODID_LAST_SAVE_NOT_DONE)
		is_save_done = false;
	else
		is_save_done = true;

	rdr_save_history_log(&temp, &log_path[path_root_len], DATATIME_MAXLEN,
		is_save_done, 0);

	ret = save_mntndump_log(NULL);
	if (ret)
		BB_PRINT_ERR("save_mntndump_log fail, ret=%d", ret);
}

/*
 * Description : Reset function of the AP when an exception occurs
 */
void rdr_hisiap_reset(u32 modid, u32 etype, u64 coreid)
{
	BB_PRINT_PN("%s start\n", __func__);

	if (!in_atomic() && !irqs_disabled() && !in_irq())
		ksys_sync();

	if (etype != AP_S_PANIC) {
		BB_PRINT_PN("etype is not panic\n");
		dump_stack();
		preempt_disable();
		smp_send_stop();
	}

	console_flush_on_panic(CONSOLE_FLUSH_PENDING);
	/* HIMNTN_PANIC_INTO_LOOP will disbale ap reset */
	if (check_himntn(HIMNTN_PANIC_INTO_LOOP) == 1 &&
	    !kexec_crash_loaded()) {
		do {
		} while (1);
	}
	kmsg_dump(KMSG_DUMP_PANIC);
	flush_cache_all();

	BB_PRINT_PN("%s end\n", __func__);

	if (!kexec_crash_loaded())
	{
		machine_restart(NULL);
	}
}

/*
 * Description : Used by the rdr to record exceptions to the pmu
 */
void record_exce_type(const struct rdr_exception_info_s *e_info)
{
	if (!e_info) {
		BB_PRINT_ERR("einfo is null\n");
		return;
	}
	set_reboot_reason(e_info->e_exce_type);
	set_subtype_exception(e_info->e_exce_subtype, false);
}

/*
 * Description : Parameters for registration exceptions.
 * This parameter is invoked between dump and reset
 */
void hisiap_callback(u32 argc, void *argv)
{
	int ret;

	if (check_himntn(HIMNTN_GOBAL_RESETLOG)) {
		ret = hisi_trace_hook_install();

		if (ret)
			BB_PRINT_ERR("[%s]\n", __func__);
	}
}

/*
 * Description : Hook function, which is used for the dead loop
 *               before the system panic processing, so that
 *               the field is reserved.
 */
int acpu_panic_loop_notify(struct notifier_block *nb,
				unsigned long event, void *buf)
{
	console_flush_on_panic(CONSOLE_FLUSH_PENDING);
	if (check_himntn(HIMNTN_PANIC_INTO_LOOP) && !kexec_crash_loaded()) {
		do {} while (1);
	}

	return 0;
}

/*
 * Description : panic reset hook function
 */
int rdr_hisiap_panic_notify(struct notifier_block *nb,
				unsigned long event, void *buf)
{
	BB_PRINT_PN("[%s], ===> enter panic notify!\n", __func__);

	rdr_syserr_process_for_ap(MODID_AP_S_PANIC, 0, 0);

	return 0;
}

/*
 * Description : die reset hook function
 */
int rdr_hisiap_die_notify(struct notifier_block *nb,
				unsigned long event, void *p_reg)
{
	return 0;
}

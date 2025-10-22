// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Allwinner sunxi crash dump debug
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kmemleak.h>
#include <asm/cacheflush.h>
#include <linux/version.h>
#include <linux/cpufreq.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/pm_opp.h>
#include <linux/math64.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/ioport.h>
#include <linux/kernel.h>
#include <linux/libfdt.h>
#include <linux/memblock.h>
#include <linux/of_fdt.h>
#include <linux/types.h>
#include <linux/vmalloc.h>
#include <linux/kdebug.h>
#include <linux/crash_core.h>
#include <asm/kexec.h>


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0)
#include <linux/panic_notifier.h>
#endif
#include "sunxi-sip.h"
#include "sunxi-smc.h"
#include "sunxi-crashdump.h"
#include "sunxi-crashnote.h"
#include "sunxi-minidump.h"

#define SUNXI_DUMP_COMPATIBLE "sunxi-dump"
static LIST_HEAD(dump_group_list);
static int sunxi_dump = 1;
struct ctl_table_header *cth;

/*
 * Sometimes panic can't stop several CPUs unexpectedly, we'd better wait
 * for them offline in a period of time rather than forever.
 */
#define SUNXI_DUMP_CPU_ONLINE_TIMEOUT (1000 * 30)
#define POLICY_NUM                       4

struct sunxi_dump_group {
	char name[10];
	u32 *reg_buf;
	void __iomem *vir_base;
	phys_addr_t phy_base;
	u32 len;
	struct list_head list;
};

struct current_freq {
	unsigned char policy_name[32];
	unsigned int used;
	u64 cpu_freq_changed_start_time;
	u64 cpu_freq_changed_end_time;
	u64 last_freq_changed_time;
	unsigned long target_freq;
	unsigned long last_freq;
	unsigned long cur_clk;
	unsigned long last_volt;
	unsigned long cur_volt;
} current_freq_info[POLICY_NUM + 1];

int sunxi_store_freqinfo(struct device *dev, unsigned int arry_id, u64 start_time,
							u64 end_time, unsigned long target_freq, unsigned long last_freq)
{
	struct clk *cur_clock = NULL;
	struct dev_pm_opp *opp = NULL;
	struct dev_pm_opp *cur_opp = NULL;

	if (!dev) {
		pr_err("input dev error!\n");
		return 0;
	}

	current_freq_info[arry_id].cpu_freq_changed_start_time = start_time;
	current_freq_info[arry_id].cpu_freq_changed_end_time = end_time;

	if (end_time - start_time)
		current_freq_info[arry_id].last_freq_changed_time = end_time - start_time;

	if (target_freq)
		current_freq_info[arry_id].target_freq = target_freq;

	if (last_freq)
		current_freq_info[arry_id].last_freq = last_freq;

	cur_clock = clk_get(dev, NULL);
	if (!cur_clock) {
		pr_err("cpu %d clock get error!", smp_processor_id());
		return 0;
	}
	current_freq_info[arry_id].cur_clk = clk_get_rate(cur_clock);
	clk_put(cur_clock);

	opp = dev_pm_opp_find_freq_ceil(dev, &last_freq);
	if (IS_ERR(opp)) {
		pr_err("%s: unable to find cpu OPP for %lu\n", __func__, last_freq);
		return 0;
	}
	current_freq_info[arry_id].last_volt = dev_pm_opp_get_voltage(opp);

	cur_opp = dev_pm_opp_find_freq_ceil(dev, &current_freq_info[arry_id].cur_clk);
	if (IS_ERR(cur_opp)) {
		pr_err("%s: unable to find cpu cur_opp for %lu\n", __func__,
				current_freq_info[arry_id].cur_clk);
		return 0;
	}
	current_freq_info[arry_id].cur_volt = dev_pm_opp_get_voltage(cur_opp);

	current_freq_info[arry_id].used = 1;

	return 0;
}

int sunxi_get_freq_info(struct device *dev, struct cpufreq_policy *policy, u64 start_time,
							u64 end_time, unsigned long target_freq, unsigned long last_freq)
{
	int i, policy_exist = 0;

	/* dram freq data */
	if (policy == NULL) {
		sunxi_store_freqinfo(dev, POLICY_NUM, start_time, end_time, target_freq, last_freq);
		return 0;
	}

	for (i = 0; i < POLICY_NUM; i++) {
		/* update current policy action */
		if (!strcmp(current_freq_info[i].policy_name, policy->kobj.name)) {
			sunxi_store_freqinfo(dev, i, start_time, end_time, target_freq, last_freq);
			policy_exist = 1;
			break;
		}
	}

	/* add new policy action */
	if (!policy_exist) {
		for (i = 0; i < POLICY_NUM; i++) {
			if (current_freq_info[i].used == 0) {
				strcpy(current_freq_info[i].policy_name, policy->kobj.name);
				sunxi_store_freqinfo(dev, i, start_time, end_time, target_freq, last_freq);
				break;
			}
		}
	}

	return 0;
}
EXPORT_SYMBOL(sunxi_get_freq_info);

int sunxi_dump_freq_info(void)
{
	int i;
	pr_err("start dump cpuinfo:\n");
	for (i = 0; i < POLICY_NUM; i++) {
		if (current_freq_info[i].used == 1) {
			pr_err("policy_name = %s \n", current_freq_info[i].policy_name);
			pr_err("cpu_freq_changed_start_time = %llu(us) cpu_freq_changed_end_time = %llu(us) last_freq_changed_time = %llu(us) \n",
				div_u64(current_freq_info[i].cpu_freq_changed_start_time, 1000),
				div_u64(current_freq_info[i].cpu_freq_changed_end_time, 1000),
				div_u64(current_freq_info[i].last_freq_changed_time, 1000));
			pr_err("last_freq = %lu(hz) target_freq = %lu(hz) current_clk = %lu(hz) last_volt = %lu(uv) cur_volt = %lu(uv) \n\n",
				current_freq_info[i].last_freq,
				current_freq_info[i].target_freq,
				current_freq_info[i].cur_clk,
				current_freq_info[i].last_volt,
				current_freq_info[i].cur_volt);
		}
	}

	if (current_freq_info[POLICY_NUM].used == 1) {
		pr_err("dram_freq_changed_start_time = %llu(us) dram_freq_changed_end_time = %llu(us) last_freq_changed_time = %llu(us) \n",
				div_u64(current_freq_info[POLICY_NUM].cpu_freq_changed_start_time, 1000),
				div_u64(current_freq_info[POLICY_NUM].cpu_freq_changed_end_time, 1000),
				div_u64(current_freq_info[POLICY_NUM].last_freq_changed_time, 1000));
		pr_err("last_freq = %lu(hz) target_freq = %lu(hz) current_clk = %lu(hz)\n\n",
				current_freq_info[POLICY_NUM].last_freq,
				current_freq_info[POLICY_NUM].target_freq,
				current_freq_info[POLICY_NUM].cur_clk);
	}

	return 0;
}

static int sunxi_dump_group_reg(struct sunxi_dump_group *group)
{
	u32 *buf = group->reg_buf;
	void __iomem *membase = group->vir_base;
	u32 len = ALIGN(group->len, 4);
	int i;

	for (i = 0; i < len; i += 4)
		*(buf++) = readl(membase + i);

	return 0;
}

int sunxi_dump_group_dump(void)
{
	struct sunxi_dump_group *dump_group = NULL;

	list_for_each_entry(dump_group, &dump_group_list, list) {
		sunxi_dump_group_reg(dump_group);
	}
	return 0;
}

static int sunxi_dump_group_register(const char *name, phys_addr_t start, u32 len)
{
	struct sunxi_dump_group *dump_group = NULL;

	dump_group = kmalloc(sizeof(*dump_group), GFP_KERNEL);
	if (!dump_group)
		return -ENOMEM;

	memcpy(dump_group->name, name, sizeof(dump_group->name));
	dump_group->phy_base = start;
	dump_group->len = len;
	dump_group->vir_base = ioremap(dump_group->phy_base, dump_group->len);
	if (!dump_group->vir_base) {
		pr_err("%s can't iomap\n", dump_group->name);
		return -EINVAL;
	}
	dump_group->reg_buf = kmalloc(dump_group->len, GFP_KERNEL);
	if (!dump_group->reg_buf)
		return -ENOMEM;

	list_add_tail(&dump_group->list, &dump_group_list);
	return 0;
}

void sunxi_dump_group_unregister(void)
{

	struct sunxi_dump_group *dump_group;

	list_for_each_entry(dump_group, &dump_group_list, list) {
		if (dump_group->vir_base)
			iounmap(dump_group->vir_base);

		if (dump_group->reg_buf)
			kfree(dump_group->reg_buf);

		list_del(&dump_group->list);

		kfree(dump_group);
	}

}

int sunxi_set_crashdump_mode(void)
{
#if IS_ENABLED(CONFIG_ARM64)
	invoke_scp_fn_smc(ARM_SVC_SUNXI_CRASHDUMP_START, sunxi_md_get_elf_pa(), 0, 0);
#endif
#if IS_ENABLED(CONFIG_ARM)
	sunxi_optee_call_crashdump();
#endif

	return 0;
}

#if IS_ENABLED(CONFIG_ARM)
static inline void crash_setup_regs32(struct pt_regs *newregs,
				    struct pt_regs *oldregs)
{
	if (oldregs) {
		memcpy(newregs, oldregs, sizeof(*newregs));
	} else {
		__asm__ __volatile__ (
			"stmia	%[regs_base], {r0-r12}\n\t"
			"mov	%[_ARM_sp], sp\n\t"
			"str	lr, %[_ARM_lr]\n\t"
			"adr	%[_ARM_pc], 1f\n\t"
			"mrs	%[_ARM_cpsr], cpsr\n\t"
		"1:"
			: [_ARM_pc] "=r" (newregs->ARM_pc),
			  [_ARM_cpsr] "=r" (newregs->ARM_cpsr),
			  [_ARM_sp] "=r" (newregs->ARM_sp),
			  [_ARM_lr] "=o" (newregs->ARM_lr)
			: [regs_base] "r" (&newregs->ARM_r0)
			: "memory"
		);
	}
}
#define crash_setup_regs crash_setup_regs32
#endif

struct pt_regs *die_regs;
static int sunxi_crashdump_die_callback(struct notifier_block *nb,
					unsigned long reason, void *arg)
{
	struct die_args *d_args = arg;
	die_regs = d_args->regs;

	return NOTIFY_DONE;
}
static struct notifier_block sunxi_dump_die_nb = {
	.notifier_call = sunxi_crashdump_die_callback,
};

void sunxi_crashdump_exec(struct pt_regs *recorded_regs)
{
	struct pt_regs fixed_regs;

	if ((system_state == SYSTEM_RESTART) || (system_state == SYSTEM_POWER_OFF))
		return;

#if IS_ENABLED(CONFIG_ARM64)
	crash_setup_regs(&fixed_regs, recorded_regs);
#endif
#if IS_ENABLED(CONFIG_ARM)
	crash_setup_regs32(&fixed_regs, recorded_regs);
#endif
#if IS_ENABLED(CONFIG_KEXEC) && IS_ENABLED(CONFIG_CRASH_DUMP) && IS_BUILTIN(CONFIG_AW_CRASHDUMP)
	sunxi_crash_save_cpu(&fixed_regs, smp_processor_id());
	crash_setup_elfheader();
	pr_info("cpu %d trigger save.\n", smp_processor_id());
#else
#if IS_ENABLED(CONFIG_ARM64)
	{
		/*
	 * unwind first frame for gdb, or gdb bt will fail
	 * because frame from x30 and frame from x29 are identical
	 */
		unsigned long fp = fixed_regs.regs[29];
		vmcoreinfo_append_str(
			"reg_fix:cpu%d:reg29:0x%016llx->0x%016lx\n",
			smp_processor_id(), fixed_regs.regs[29],
			READ_ONCE(*(unsigned long *)(fp)));
		fixed_regs.regs[29] = READ_ONCE(*(unsigned long *)(fp));
		vmcoreinfo_append_str(
			"reg_fix:cpu%d:reg30:0x%016llx->0x%016lx\n",
			smp_processor_id(), fixed_regs.regs[30],
			READ_ONCE(*(unsigned long *)(fp + 8)));
		fixed_regs.regs[30] = READ_ONCE(*(unsigned long *)(fp + 8));
	}
#endif
	sunxi_crash_save_cpu(&fixed_regs, smp_processor_id());
#endif
	sunxi_dump_status(&fixed_regs);
}
EXPORT_SYMBOL(sunxi_crashdump_exec);

static int sunxi_dump_panic_event(struct notifier_block *self, unsigned long val, void *reason)
{
	unsigned int i, online_time;
	unsigned long __maybe_unused offset;
	struct pt_regs fixed_regs;

	if (!sunxi_dump) {
		pr_emerg("crashdump disabled\n");
		return NOTIFY_DONE;
	}

#if IS_REACHABLE(CONFIG_AW_DISP2) || IS_REACHABLE(CONFIG_AW_DRM)
	sunxi_kernel_panic_printf("CRASHDUMP NOW,\n PLEASE CONNECT TO TIGERDUMP TOOL\n AND KEEP POWER ON");
#endif

	flush_cache_all();
	mdelay(1000);
	crash_setup_regs(&fixed_regs, die_regs);
	sunxi_crashdump_exec(&fixed_regs);
	sunxi_md_exec(&fixed_regs);
	sunxi_dump_group_dump();

	for (i = 0; i < num_possible_cpus(); i++) {
		if (i == smp_processor_id())
			continue;

		/*
		 * Notice: record the online time of per cpu,
		 * ignore those more than 30 seconds.
		 */
		online_time = 0;
		while (1) {
			if (!cpu_online(i))
				break;
			mdelay(10);
			online_time += 10;
			if (online_time >= SUNXI_DUMP_CPU_ONLINE_TIMEOUT)
				break;
		}
	}

	/* Notice: make sure to print the full stack trace */
	mdelay(5000);

	prepare_kdump_header();
	pr_emerg("\033[31mEnter crashdump, Please connect PC tool TigerDump\033[0m\n");
	/* Support to provide debug information for arm64 */
#if IS_ENABLED(CONFIG_ARM64)
	offset = kaslr_offset();
	pr_emerg("vabits_actual: %llu, kimage_voffset: 0x%llx, kaslr: 0x%lx\n",
		 vabits_actual, kimage_voffset, offset);
#endif
	sunxi_dump_freq_info();

	/*
	 * Due to the lengthy process of crashdump, we need to first decide whether it is worthwhile to crashdump
	 * based on the panic information.
	 * However, in some cases, we are unable to obtain the panic information in a timely manner:
	 * For example:
	 * 1. If another CPU is currently printing, the panic CPU will be unable to acquire the console_sem.
	 * 2. Meanwhile, since the CPU will immediately enter ATF, it cannot return from ATF before dump operation,
	 * Therefore, the flush_on_panic cannot be executed.
	 * In this scenario, the panic information may not be printed to the UART in a timely manner. Therefore,
	 * we forced flush here. Although there is no symmetrical operation for the semaphore here,
	 * since we are panic, NO ONE care about it!
	 */
	console_unlock();

	sunxi_set_crashdump_mode();
	pr_emerg("crashdump exit\n");

#if IS_ENABLED(CONFIG_PANIC_DEBUG)
	if (reason != NULL && strstr(reason, "Watchdog detected hard LOCKUP"))
		while (1)
			cpu_relax();
#endif

	return NOTIFY_DONE;
}

static struct notifier_block sunxi_dump_panic_event_nb = {
	.notifier_call = sunxi_dump_panic_event,
	/* .priority = INT_MAX, */
};

static struct ctl_table sunxi_dump_sysctl_table[] = {
	{
		.procname = "sunxi_dump",
		.data = &sunxi_dump,
		.maxlen = sizeof(sunxi_dump),
		.mode = 0644,
		.proc_handler = proc_dointvec,
	},
	{ }
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
static struct ctl_table sunxi_dump_sysctl_root[] = {
	{
		.procname = "kernel",
		.mode = 0555,
		.child = sunxi_dump_sysctl_table,
	},
	{ }
};
#endif

int sunxi_crash_dump2pc_init(void)
{
	struct device_node *node;
	int i = 0;
	const char *name = NULL;
	struct resource res;
	unsigned long __maybe_unused offset;

	node = of_find_compatible_node(NULL, NULL, SUNXI_DUMP_COMPATIBLE);

	for (i = 0; ; i++) {
		if (of_address_to_resource(node, i, &res))
			break;
		if (of_property_read_string_index(node, "group-names", i, &name))
			break;
		sunxi_dump_group_register(name, res.start, resource_size(&res));
	}

	/* register sunxi dump sysctl */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 5, 0)
	cth = register_sysctl("kernel", sunxi_dump_sysctl_table);
#else
	cth = register_sysctl_table(sunxi_dump_sysctl_root);
	kmemleak_not_leak(cth);
#endif
	/* register sunxi dump panic notifier */
	atomic_notifier_chain_register(&panic_notifier_list, &sunxi_dump_panic_event_nb);
	register_die_notifier(&sunxi_dump_die_nb);

	/* Support to provide debug information for arm64 in RT system*/
#if IS_ENABLED(CONFIG_ARM64) && IS_ENABLED(CONFIG_PREEMPT_RT)
	offset = kaslr_offset();
	pr_emerg("vabits_actual: %llu, kimage_voffset: 0x%llx, kaslr: 0x%lx\n",
		 vabits_actual, kimage_voffset, offset);
#endif

	return 0;
}

void sunxi_crash_dump2pc_exit(void)
{
	unregister_die_notifier(&sunxi_dump_die_nb);
	atomic_notifier_chain_unregister(&panic_notifier_list, &sunxi_dump_panic_event_nb);
	unregister_sysctl_table(cth);
	sunxi_dump_group_unregister();
}

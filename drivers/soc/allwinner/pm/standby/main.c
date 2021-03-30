/*
 * standby driver for allwinnertech
 *
 * Copyright (C) 2015 allwinnertech Ltd.
 * Author: Ming Li <liming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include "standby.h"
#include "../pm.h"
#include "main.h"
#include "dram/mctl_standby.h"

#define SUPER_FALG      (0x10000)

static void cache_count_init(void);
static void cache_count_get(void);
static void cache_count_output(void);
static __u32 sp_backup;
static struct aw_pm_info pm_info;
static extended_standby_t extended_standby_para_info;
static struct pll_factor_t orig_pll;
static struct pll_factor_t local_pll;
static struct standby_clk_div_t clk_div;
static struct standby_clk_div_t tmp_clk_div;

static int dram_enter_selfresh(extended_standby_t *para);
static int dram_exit_selfresh(extended_standby_t *para);
static int cpu_enter_lowfreq(void);
static int cpu_freq_resume(void);
static int bus_enter_lowfreq(extended_standby_t *para);
static int bus_freq_resume(extended_standby_t *para);
static void query_wakeup_source(struct aw_pm_info *arg);
losc_enter_ss_func losc_enter_ss;

static u32 before_crc;
static u32 after_crc;
/* static unsigned int backup_actlr;*/
/* #define CHECK_CACHE_TLB_MISS */
#ifdef CHECK_CACHE_TLB_MISS
/* #define STATS_CACHE_TLB_MISS */
/* #define STATS_BUS_ACCESS */
#define STATS_CACHE_ACCESS
int d_cache_miss_start;
int d_tlb_miss_start;
int i_tlb_miss_start;
int i_cache_miss_start;
int d_cache_miss_end;
int d_tlb_miss_end;
int i_tlb_miss_end;
int i_cache_miss_end;
#endif

int standby_main(struct aw_pm_info *arg)
{
	char *tmp_ptr = (char *)&__bss_start;
	save_mem_status(STANDBY_START | 0X01);
	if (!arg) {
		/* standby parameter is invalid */
		return -1;
	}

	/* clear bss segment */
	do {
		*tmp_ptr++ = 0;
	} while (tmp_ptr <= (char *)&__bss_end);
	/* save stack pointer registger, switch stack to sram */
	sp_backup = save_sp();

	/* flush data and instruction tlb, there is 32 items of data tlb and
	 * 32 items of instruction tlb, The TLB is normally allocated on a
	 * rotating basis. The oldest entry is always the next allocated
	 */
	mem_flush_tlb();
	/* backup_actlr = disable_prefetch();
	 * disable_cache();
	 */
	save_mem_status(STANDBY_START | 0X02);

	/* copy standby parameter from dram */
	standby_memcpy(&pm_info, arg, sizeof(pm_info));

	/* copy extended standby info */
	if (0 != pm_info.standby_para.pextended_standby) {
		standby_memcpy(&extended_standby_para_info,
			       (void *)(pm_info.standby_para.pextended_standby),
			       sizeof(extended_standby_para_info));
	}

	/* preload tlb for standby */
	mem_preload_tlb();
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	/* init module before dram enter selfrefresh */
	/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
	/*init perf counter for timing: need double check */
	init_perfcounters(1, 0);
	standby_clk_init();
	mem_clk_init(1);
	save_mem_status(STANDBY_START | 0X03);
	/* init uart for print */
	if (unlikely
	    (pm_info.standby_para.debug_mask & PM_STANDBY_PRINT_STANDBY))
		serial_init_manager();

	/* calc dram checksum */
	if (standby_dram_crc_enable(&(extended_standby_para_info.soc_dram_state))) {
		before_crc = standby_dram_crc(&(extended_standby_para_info.soc_dram_state));
		mem_preload_tlb();
	}

	save_mem_status(STANDBY_START | 0X04);
	/* cpu reduce frequency */
	cpu_enter_lowfreq();

	standby_twi_init(pm_info.pmu_arg.twi_port);

	/* process standby */
	if (unlikely
	    (pm_info.standby_para.
	     debug_mask & PM_STANDBY_PRINT_CACHE_TLB_MISS)) {
		cache_count_init();
	}
	/* enable dram enter into self-refresh */
	dram_enter_selfresh(&extended_standby_para_info);

	save_mem_status(STANDBY_START | 0X05);
#ifdef CONFIG_AW_AXP
	if (SUPER_STANDBY_FLAG
			== extended_standby_para_info.id) {
		save_super_flags(SUPER_FALG);
		save_super_addr(pm_info.resume_addr);
		save_mem_status(before_crc);
		power_enter_super_calc(&pm_info, &extended_standby_para_info,
				       &losc_enter_ss);
		save_mem_status(STANDBY_START | 0X0a);
		if (NULL != losc_enter_ss) {
			save_mem_status(before_crc);
			losc_enter_ss();
			asm("b .");
		}
		/* when apb2 clk src is losc, twi clk divide ratio need reconfig */
		standby_twi_init_losc(pm_info.pmu_arg.twi_port);
	}
	/* power domain suspend */
	dm_suspend(&pm_info, &extended_standby_para_info);
	save_mem_status(STANDBY_START | 0X0b);
#endif

	save_mem_status(STANDBY_START | 0X07);
	/* bus reduce frequency:
	 * dram need enter selfresh state
	 * the reason is: the hosc, ldo will be close.
	 */
	bus_enter_lowfreq(&extended_standby_para_info);

	/* cpu enter sleep, wait wakeup by interrupt */
	/*
	 */
	asm("WFI");
	/* bus freq resume */
	bus_freq_resume(&extended_standby_para_info);

#ifdef CONFIG_AW_AXP
	save_mem_status(RESUME0_START | 0X02);
	/* power domain resume */
	dm_resume(&extended_standby_para_info);
#endif

	save_mem_status(RESUME0_START | 0X03);
	/* dram out self-refresh */
	dram_exit_selfresh(&extended_standby_para_info);

	if (unlikely
	    (pm_info.standby_para.
	     debug_mask & PM_STANDBY_PRINT_CACHE_TLB_MISS)) {
		cache_count_get();
		cache_count_output();
	}

	standby_twi_exit();
	/* cpu freq resume */
	cpu_freq_resume();
	save_mem_status(RESUME0_START | 0X04);


	if (unlikely
	    (pm_info.standby_para.debug_mask & PM_STANDBY_PRINT_STANDBY))
		serial_exit_manager();

	/* restore stack pointer register, switch stack back to dram */
	restore_sp(sp_backup);
	/* FIXME: seems the dram para have some err.
	 * 1. the dram crc, in normal standby case, may have crc err.
	 *      such as the region: 0x40000000 -> 0x40010000
	 * 2. need delay, such as: 5ms, before return to kernel.
	 * 3. the bug is inexplicable, the summary as follow:
	 *      3.1 rtc err code: may be 5004, or 5005, or 8000.
	 *              mean, the reason for cpu die is not sure.
	 *      3.2 add delay here, bring good effect for cpu running.
	 *              but if we have an condition expresstion before delay,
	 *              it may have no effect.
	 *              so, memory attribute or bus behavior may have
	 *              contribute to this bug. to locate the real reason,
	 *              not use compile optiomize is better option.
	 *      3.3 dram crc, in normal standby case, may have crc err.
	 * 4. the right flow to correct this bug is:
	 *      4.1 make sure dram crc is right. (right now, crc err occur.)
	 *      4.2 make sure the sramA1 memory attribute is correct.
	 *              (right now, strongly-order is in use? conflict with trm.)
	 */

	if (likely(pm_info.standby_para.debug_mask & PM_STANDBY_TEST)) {
		/* need double check..*/
		init_perfcounters(1, 0);
		change_runtime_env();
		delay_ms(5);
	}

	/* enable_cache();
	 * restore_prefetch(backup_actlr);
	 */

	return 0;
}

static int dram_enter_selfresh(extended_standby_t *para)
{
	s32 ret = -1;
	dram_disable_all_master();

	ret = dram_power_save_process();

	return ret;
}

static int dram_exit_selfresh(extended_standby_t *para)
{
	s32 ret = -1;

	ret = dram_power_up_process(&(pm_info.dram_para));

	if (standby_dram_crc_enable(&(para->soc_dram_state))) {
		after_crc = standby_dram_crc(&(para->soc_dram_state));
		printk("before_crc = 0x%x, after_crc = 0x%x.\n", before_crc,
		       after_crc);
		if (after_crc != before_crc) {
			save_mem_status(RESUME0_START | 0xe);
			printk("dram crc error...\n");
			asm("b .");
		}
	}
	dram_enable_all_master();
	return ret;
}

static int cpu_enter_lowfreq(void)
{
	/* backup cpu freq */
	standby_clk_get_pll_factor(&orig_pll);
	/* backup bus src */
	standby_clk_bus_src_backup();

	/*lower freq from 1008M to 408M */
	local_pll.FactorN = 16;
	local_pll.FactorK = 0;
	local_pll.FactorM = 0;
	local_pll.FactorP = 0;
	standby_clk_set_pll_factor(&local_pll);

	delay_ms(10);

	/* switch cpu clock to HOSC:
	 * after switch to HOSC, the axi, ahb freq will change also
	 * which will lead the jtag exception. */
	standby_clk_core2hosc();
	delay_us(1);

	return 0;
}

static int cpu_freq_resume(void)
{
	/* switch cpu clock to core pll */
	standby_clk_core2pll();
	change_runtime_env();
	delay_ms(5);

	/*restore freq from 408M to 1008M */
	standby_clk_set_pll_factor(&orig_pll);
	change_runtime_env();
	delay_ms(10);

	return 0;
}

static int bus_enter_lowfreq(extended_standby_t *para)
{
	/* change ahb src to axi */
	standby_clk_bus_src_set();

	standby_clk_getdiv(&clk_div);
	/* set clock division cpu:axi:ahb:apb = 2:2:2:1 */
	tmp_clk_div.axi_div = 0;
	tmp_clk_div.ahb_div = 0;
	tmp_clk_div.ahb_pre_div = 0;
	tmp_clk_div.apb_div = 0;
	tmp_clk_div.apb2_div = 0;
	tmp_clk_div.apb2_pre_div = 0;
	standby_clk_setdiv(&tmp_clk_div);

	/* swtich apb2 to losc from 24M */
	standby_clk_apb2losc();
	change_runtime_env();
	/* delay_ms(1); */
	standby_clk_plldisable();

	/* switch cpu to 32k */
	save_mem_status(STANDBY_START | 0x0d);
	standby_clk_core2losc();
	if (NULL != losc_enter_ss) {
		save_mem_status((__u32) losc_enter_ss);
		losc_enter_ss();
		asm("b .");
	}
	save_mem_status(STANDBY_START | 0x0f);
	if (1 == para->soc_dram_state.selfresh_flag) {
		/* printk("disable HOSC, and disable LDO\n"); */
		/* disable HOSC, and disable LDO */
		if (0 == para->cpux_clk_state.osc_en) {
			standby_clk_hoscdisable();
			standby_clk_ldodisable();
		}
	}
	return 0;
}

static int bus_freq_resume(extended_standby_t *para)
{
	if (1 == para->soc_dram_state.selfresh_flag) {
		if (0 == para->cpux_clk_state.osc_en) {
			/* enable LDO, enable HOSC */
			standby_clk_ldoenable();
			/* delay 1ms for power be stable */
			/* 3ms */
			standby_delay_cycle(1);
			standby_clk_hoscenable();
			/* 3ms */
			standby_delay_cycle(1);
		}
	}
	/* switch clock to hosc */
	standby_clk_core2hosc();

	/* swtich apb2 to hosc */
	standby_clk_apb2hosc();

	/* enable pll */
	standby_clk_pllenable();
	change_runtime_env();
	delay_ms(10);

	/* restore clock division */
	standby_clk_setdiv(&clk_div);

	standby_clk_bus_src_restore();

	return 0;
}

#ifdef CHECK_CACHE_TLB_MISS

static void cache_count_init(void)
{
#if defined(STATS_CACHE_TLB_MISS)
	set_event_counter(D_CACHE_MISS);
	set_event_counter(D_TLB_MISS);
	set_event_counter(I_CACHE_MISS);
	set_event_counter(I_TLB_MISS);
	init_event_counter(1, 0);
	d_cache_miss_start = get_event_counter(D_CACHE_MISS);
	d_tlb_miss_start = get_event_counter(D_TLB_MISS);
	i_tlb_miss_start = get_event_counter(I_TLB_MISS);
	i_cache_miss_start = get_event_counter(I_CACHE_MISS);
#elif defined(STATS_BUS_ACCESS)
	set_event_counter(BR_PRED);
	set_event_counter(MEM_ACCESS);
	set_event_counter(BUS_ACCESS);
	set_event_counter(INST_SPEC);
	init_event_counter(1, 0);
	d_cache_miss_start = get_event_counter(BR_PRED);
	d_tlb_miss_start = get_event_counter(MEM_ACCESS);
	i_tlb_miss_start = get_event_counter(BUS_ACCESS);
	i_cache_miss_start = get_event_counter(INST_SPEC);
#elif defined(STATS_CACHE_ACCESS)
	set_event_counter(D_CACHE_ACCESS);
	set_event_counter(D_CACHE_EVICT);
	set_event_counter(L2_CACHE_REFILL);
	set_event_counter(PREFETCH_LINEFILL);
	init_event_counter(1, 0);
	d_cache_miss_start = get_event_counter(D_CACHE_ACCESS);
	d_tlb_miss_start = get_event_counter(D_CACHE_EVICT);
	i_tlb_miss_start = get_event_counter(L2_CACHE_REFILL);
	i_cache_miss_start = get_event_counter(PREFETCH_LINEFILL);
#endif
	return;
}

static void cache_count_get(void)
{
#if defined(STATS_CACHE_TLB_MISS)
	d_cache_miss_end = get_event_counter(BR_PRED);
	d_tlb_miss_end = get_event_counter(MEM_ACCESS);
	i_tlb_miss_end = get_event_counter(BUS_ACCESS);
	i_cache_miss_end = get_event_counter(INST_SPEC);
#elif defined(STATS_BUS_ACCESS)
	d_cache_miss_end = get_event_counter(D_CACHE_MISS);
	d_tlb_miss_end = get_event_counter(D_TLB_MISS);
	i_tlb_miss_end = get_event_counter(I_TLB_MISS);
	i_cache_miss_end = get_event_counter(I_CACHE_MISS);
#elif defined(STATS_CACHE_ACCESS)
	d_cache_miss_end = get_event_counter(D_CACHE_ACCESS);
	d_tlb_miss_end = get_event_counter(D_CACHE_EVICT);
	i_tlb_miss_end = get_event_counter(L2_CACHE_REFILL);
	i_cache_miss_end = get_event_counter(PREFETCH_LINEFILL);
#endif

	return;
}

static void cache_count_output(void)
{
	if (d_cache_miss_end || d_tlb_miss_end || i_tlb_miss_end
	    || i_cache_miss_end) {
		printk
		    ("=============================NOTICE====================================. \n");
		printk("d_cache_miss_start = %d, d_cache_miss_end= %d. \n",
		       d_cache_miss_start, d_cache_miss_end);
		printk("d_tlb_miss_start = %d, d_tlb_miss_end= %d. \n",
		       d_tlb_miss_start, d_tlb_miss_end);
		printk("i_cache_miss_start = %d, i_cache_miss_end= %d. \n",
		       i_cache_miss_start, i_cache_miss_end);
		printk("i_tlb_miss_start = %d, i_tlb_miss_end= %d. \n",
		       i_tlb_miss_start, i_tlb_miss_end);
		asm("b .");
	} else {
		printk("no miss. \n");
	}

	return;
}

#else
static void cache_count_init(void)
{
	return;
}

static void cache_count_get(void)
{
	return;
}

static void cache_count_output(void)
{
	return;
}
#endif

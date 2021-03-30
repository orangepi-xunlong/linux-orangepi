/*
 * Copyright (c) 2011-2020 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "pm_o.h"
#include <linux/clk.h>
#include <linux/clk-private.h>
#include <linux/clk/sunxi.h>
#include <asm/cpuidle.h>
static struct kobject *aw_pm_kobj;
unsigned long cpu_resume_addr;
/*
*********************************************************************************************************
*                           aw_pm_valid
*
*Description: determine if given system sleep state is supported by the platform;
*
*Arguments  : state     suspend state;
*
*Return     : if the state is valid, return 1, else return 0;
*
*Notes      : this is a call-back function, registered into PM core;
*
*********************************************************************************************************
*/
static int aw_pm_valid(suspend_state_t state)
{
#ifdef CHECK_IC_VERSION
	enum sw_ic_ver version = MAGIC_VER_NULL;
#endif

	PM_DBG("valid\n");

	if (!((state > PM_SUSPEND_ON) && (state < PM_SUSPEND_MAX))) {
		PM_DBG("state (%d) invalid!\n", state);
		return 0;
	}
#ifdef CHECK_IC_VERSION
	if (1 == standby_mode) {
		version = sw_get_ic_ver();
		if (!
		    (MAGIC_VER_A13B == version || MAGIC_VER_A12B == version
		     || MAGIC_VER_A10SB == version)) {
			pr_info
			    ("ic version: %d not support super standby. \n",
			     version);
			standby_mode = 0;
		}
	}
#endif

	/*if 1 == standby_mode, actually, mean mem corresponding with super standby */
	if (PM_SUSPEND_STANDBY == state) {
		if (1 == standby_mode) {
			standby_type = NORMAL_STANDBY;
		} else {
			standby_type = SUPER_STANDBY;
		}
	} else if (PM_SUSPEND_MEM == state || PM_SUSPEND_BOOTFAST == state) {
		if (1 == standby_mode) {
			standby_type = SUPER_STANDBY;
		} else {
			standby_type = NORMAL_STANDBY;
		}
	}
#if defined(CONFIG_ARCH_SUN9IW1P1) || \
	defined(CONFIG_ARCH_SUN8IW5P1) || \
	defined(CONFIG_ARCH_SUN8IW6P1) || \
	defined(CONFIG_ARCH_SUN50IW1P1) || \
	defined(CONFIG_ARCH_SUN50IW2P1)
	if (NORMAL_STANDBY == standby_type) {
		printk
		    ("Notice: sun9i & sun8iw5 & sun50i not need support normal standby, \
				change to super standby.\n");

		standby_type = SUPER_STANDBY;
	}
#elif defined(CONFIG_ARCH_SUN8IW8P1)
	if (SUPER_STANDBY == standby_type) {
		printk("Notice: sun8iw8p1 not need support super standby, \
				change to normal standby.\n");
		standby_type = NORMAL_STANDBY;
	}
	/* default enter to extended standby's normal standby */
	{
		static struct scene_lock normal_lock;
		if (unlikely(check_scene_locked(SCENE_NORMAL_STANDBY))) {
			scene_lock_init(&normal_lock, SCENE_NORMAL_STANDBY,
					"normal_standby");
			scene_lock(&normal_lock);
		}
	}

#endif

	return 1;

}

/*
*********************************************************************************************************
*                           aw_pm_begin
*
*Description: Initialise a transition to given system sleep state;
*
*Arguments  : state     suspend state;
*
*Return     : return 0 for process successed;
*
*Notes      : this is a call-back function, registered into PM core, and this function
*             will be called before devices suspened;
*********************************************************************************************************
*/
static int aw_pm_begin(suspend_state_t state)
{
	static __u32 suspend_status;
	static int suspend_result;	/*record last suspend status, 0/-1 */
	static bool backup_console_suspend_enabled;
	static bool backup_initcall_debug;
	static int backup_console_loglevel;
	static __u32 backup_debug_mask;

	PM_DBG("%d state begin\n", state);
	/*set freq max */
#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
	/*cpufreq_user_event_notify(); */
#endif

	/*must init perfcounter, because delay_us and delay_ms is depandant perf counter */

	/*check rtc status, if err happened, do sth to fix it. */
	suspend_status = get_pm_secure_mem_status();
	if ((error_gen(MOD_FIRST_BOOT_FLAG, 0) != suspend_status) &&
	    (BOOT_UPGRADE_FLAG != suspend_status) &&
	    (error_gen(MOD_RESUME_COMPLETE_FLAG, 0) != suspend_status)) {
		suspend_result = -1;
		printk("suspend_err, rtc gpr as follow: \n");
		show_mem_status();
		/*adjust: loglevel, console_suspend, initcall_debug, debug_mask config. */
		/*disable console suspend. */
		backup_console_suspend_enabled = console_suspend_enabled;
		console_suspend_enabled = 0;
		/*enable initcall_debug */
		backup_initcall_debug = initcall_debug;
		initcall_debug = 1;
		/*change loglevel to 8 */
		backup_console_loglevel = console_loglevel;
		console_loglevel = 8;
		/*change debug_mask to 0xff */
		backup_debug_mask = debug_mask;
		debug_mask |= 0x07;
	} else {
		if (-1 == suspend_result) {
			/*restore console suspend. */
			console_suspend_enabled =
			    backup_console_suspend_enabled;
			/*restore initcall_debug */
			initcall_debug = backup_initcall_debug;
			/*restore console_loglevel */
			console_loglevel = backup_console_loglevel;
			/*restore debug_mask */
			debug_mask = backup_debug_mask;
		}
		suspend_result = 0;
	}
	save_pm_secure_mem_status(error_gen(MOD_FREEZER_THREAD, 0));

	return 0;

}

/*
*********************************************************************************************************
*                           aw_pm_prepare
*
*Description: Prepare the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core, this function
*             will be called after devices suspended, and before device late suspend
*             call-back functions;
*********************************************************************************************************
*/
static int aw_pm_prepare(void)
{
	PM_DBG("prepare\n");
	save_pm_secure_mem_status(error_gen
				  (MOD_SUSPEND_DEVICES,
				   ERR_SUSPEND_DEVICES_SUSPEND_DEVICES_DONE));

	return 0;
}

/*
*********************************************************************************************************
*                           aw_pm_prepare_late
*
*Description: Finish preparing the platform for entering the system sleep state.
*
*Arguments  : none;
*
*Return     : return 0 for process successed, and negative code for error;
*
*Notes      : this is a call-back function, registered into PM core.
*             prepare_late is called before disabling nonboot CPUs and after
*              device drivers' late suspend callbacks have been executed;
*********************************************************************************************************
*/
static int aw_pm_prepare_late(void)
{
#ifdef CONFIG_CPU_FREQ
	struct cpufreq_policy policy;
#endif
	PM_DBG("prepare_late\n");

#ifdef CONFIG_CPU_FREQ
	if (cpufreq_get_policy(&policy, 0))
		goto out;

	cpufreq_driver_target(&policy, suspend_freq, CPUFREQ_RELATION_L);
#endif

#if (defined(CONFIG_ARCH_SUN8IW10P1) || defined(CONFIG_ARCH_SUN8IW11P1)) && defined(CONFIG_AW_AXP)
	if (unlikely(debug_mask & PM_STANDBY_PRINT_PWR_STATUS)) {
		printk(KERN_INFO "power status as follow:");
		/* notice:
		 * with no cpus, the axp will use twi0 which
		 * will use schedule interface to read axp status.
		 * considering schedule is prohibited  when timekeeping is in suspended state.
		 * so call axp_regulator_dump() before calling timekeeping_suspend().
		 */
		axp_regulator_dump();
	}

	/* check pwr usage info: change sys_id according module usage info. */
	check_pwr_status();
	/* config sys_pwr_dm according to the sysconfig */
	config_sys_pwr();
	/* save axp regulator state if need  */
	axp_mem_save();
#endif
	save_pm_secure_mem_status(error_gen
				  (MOD_SUSPEND_DEVICES,
				   ERR_SUSPEND_DEVICES_LATE_SUSPEND_DEVICES_DONE));
	return 0;

#ifdef CONFIG_CPU_FREQ
      out:
	return -1;
#endif
}

/*
*********************************************************************************************************
*                           aw_early_suspend
*
*Description: prepare necessary info for suspend & resume;
*
*Return     : return 0 is process successed;
*
*Notes      : -1: data is ok;
*			-2: data has been destory.
*********************************************************************************************************
*/
static int aw_early_suspend(void)
{

#if defined(CONFIG_ARCH_SUN8IW10P1)
	static u32 sram_ctrl_reg_backup;
#endif

#define SYS_CTRL (0xf1c00000)
#define SRAM_CTRL_REG (0x04)
#define CCU_V_BASE (0xf1c20000)
#define DE_PLL_OFFSET (0x48)
#define AHB1_APB1_CFG_OFFSET (0x54)
#define EE_GATING_OFFSET (0x64)
#define DE_CLK_OFFSET (0x104)
#define EE_CLK_OFFSET (0x108)
#define EE_RST_OFFSET (0x2c4)

#ifdef ENTER_SUPER_STANDBY
	/*print_call_info(); */
	if (check_scene_locked(SCENE_BOOT_FAST))
		super_standby_para_info.event = mem_para_info.axp_event;
	else
		super_standby_para_info.event = CPUS_BOOTFAST_WAKEUP;

	/* the wakeup src is independent of the scene_lock. */
	/* the developer only need to care about: the scene support the wakeup src; */
	if (NULL != extended_standby_manager_id) {
		super_standby_para_info.event |=
		    extended_standby_manager_id->event;
		super_standby_para_info.gpio_enable_bitmap =
		    extended_standby_manager_id->wakeup_gpio_map;
		super_standby_para_info.cpux_gpiog_bitmap =
		    extended_standby_manager_id->wakeup_gpio_group;
	}
	if ((super_standby_para_info.event &
	     (CPUS_WAKEUP_DESCEND | CPUS_WAKEUP_ASCEND)) == 0) {
#ifdef	CONFIG_AW_AXP
		axp_powerkey_set(1);
#endif
	}
#ifdef RESUME_FROM_RESUME1
	super_standby_para_info.resume_entry = SRAM_FUNC_START_PA;
	if ((NULL != extended_standby_manager_id)
	    && (NULL != extended_standby_manager_id->pextended_standby))
		super_standby_para_info.pextended_standby =
		    (unsigned int)(standby_space.extended_standby_mem_base);
	else
		super_standby_para_info.pextended_standby = 0;
#endif

	super_standby_para_info.timeout = time_to_wakeup;
	if (0 < super_standby_para_info.timeout) {
		super_standby_para_info.event |= CPUS_WAKEUP_TIMEOUT;
	}

	if (unlikely(debug_mask & PM_STANDBY_PRINT_STANDBY)) {
		pr_info
		    ("total dynamic config standby wakeup src config: 0x%x.\n",
		     super_standby_para_info.event);
		parse_wakeup_event(NULL, 0, super_standby_para_info.event);
	}

	if (standby_space.mem_size <
	    sizeof(super_standby_para_info) +
	    sizeof(*(extended_standby_manager_id->pextended_standby))) {
		/*judge the reserved space for mem para is enough or not. */
		printk("ERR: reserved space is not enough for mem_para. \n");
		printk("need size: %lx. \n",
		       (unsigned long)(sizeof(super_standby_para_info) +
				       sizeof(*
					      (extended_standby_manager_id->
					       pextended_standby))));
		printk("reserved size: %lx. \n",
		       (unsigned long)(standby_space.mem_size));
		return -1;
	}

	/*clean all the data into dram */
	memcpy((void *)phys_to_virt(standby_space.standby_mem_base),
	       (void *)&super_standby_para_info,
	       sizeof(super_standby_para_info));
	if ((NULL != extended_standby_manager_id)
	    && (NULL != extended_standby_manager_id->pextended_standby))
		memcpy((void *)(phys_to_virt
				(standby_space.extended_standby_mem_base)),
		       (void *)(extended_standby_manager_id->pextended_standby),
		       sizeof(*
			      (extended_standby_manager_id->
			       pextended_standby)));
#if defined CONFIG_ARM
	dmac_flush_range((void *)
			 phys_to_virt(standby_space.standby_mem_base),
			 (void
			  *)(phys_to_virt(standby_space.standby_mem_base +
					  standby_space.mem_size)));
#elif defined CONFIG_ARM64
	__dma_flush_range((void *)
			  phys_to_virt(standby_space.standby_mem_base),
			  (void
			   *)(phys_to_virt(standby_space.standby_mem_base +
					   standby_space.mem_size)));
#endif

#if defined(CONFIG_ARCH_SUN8IW10P1) || defined(CONFIG_ARCH_SUN8IW11P1)
	mem_device_save();
#if defined(CONFIG_ARCH_SUN8IW10P1)
	writel(readl((void *)(CCU_V_BASE + DE_PLL_OFFSET)) | 0x80000000,
	       (void *)(CCU_V_BASE + DE_PLL_OFFSET));
	writel(readl((void *)(CCU_V_BASE + DE_CLK_OFFSET)) | 0x80000000,
	       (void *)(CCU_V_BASE + DE_CLK_OFFSET));
	writel(readl((void *)(CCU_V_BASE + EE_CLK_OFFSET)) | 0x80000000,
	       (void *)(CCU_V_BASE + EE_CLK_OFFSET));
	writel(readl((void *)(CCU_V_BASE + EE_GATING_OFFSET)) | 0x00003000,
	       (void *)(CCU_V_BASE + EE_GATING_OFFSET));
	writel(readl((void *)(CCU_V_BASE + EE_RST_OFFSET)) | 0x00003000,
	       (void *)(CCU_V_BASE + EE_RST_OFFSET));
	writel(readl((void *)(CCU_V_BASE + AHB1_APB1_CFG_OFFSET)) | 0x00000010,
	       (void *)(CCU_V_BASE + AHB1_APB1_CFG_OFFSET));
	sram_ctrl_reg_backup = readl((void *)(SYS_CTRL + SRAM_CTRL_REG));
	writel(sram_ctrl_reg_backup | ((u32) 0x1 << (u32) 24),
	       (void *)(SYS_CTRL + SRAM_CTRL_REG));
#endif
	/* copy brom to dram address */
	/*memcpy(); */
	standby_info.standby_para.event = CPU0_MEM_WAKEUP;
	standby_info.standby_para.event |= super_standby_para_info.event;
	standby_info.standby_para.gpio_enable_bitmap =
	    super_standby_para_info.gpio_enable_bitmap;
	standby_info.standby_para.cpux_gpiog_bitmap =
	    super_standby_para_info.cpux_gpiog_bitmap;
	standby_info.standby_para.pextended_standby =
	    (unsigned int)(0 ==
			   super_standby_para_info.pextended_standby) ? 0 :
	    (unsigned
	     int)(phys_to_virt(super_standby_para_info.pextended_standby));
	standby_info.standby_para.timeout = super_standby_para_info.timeout;
	standby_info.standby_para.debug_mask = debug_mask;
#if defined(CONFIG_AW_AXP)
	get_pwr_regu_tree((unsigned int *)(standby_info.
					   pmu_arg.soc_power_tree));
#endif
#endif
	save_pm_secure_mem_status(error_gen
				  (MOD_SUSPEND_CPUXSYS,
				   ERR_SUSPEND_CPUXSYS_CONFIG_SUPER_PARA_DONE));
	init_wakeup_src(standby_info.standby_para.event,
				standby_info.standby_para.gpio_enable_bitmap,
				standby_info.standby_para.cpux_gpiog_bitmap);
	save_pm_secure_mem_status(error_gen
				  (MOD_SUSPEND_CPUXSYS,
				   ERR_SUSPEND_CPUXSYS_CONFIG_WAKEUP_SRC_DONE));
#ifdef CONFIG_CPU_OPS_SUNXI
	asm("wfi");
#elif defined(CONFIG_ARCH_SUN8IW10P1) || defined(CONFIG_ARCH_SUN8IW11P1)
	standby_info.resume_addr = virt_to_phys((void *)&cpu_brom_start);
	cpu_resume_addr = virt_to_phys(cpu_resume);
	create_mapping();
	/*cpu_cluster_pm_enter(); */
	cpu_suspend((unsigned long)(&standby_info), aw_standby_enter);
	/*cpu_cluster_pm_enter(); */
	restore_mapping();
	/* report which wake source wakeup system */
	query_wakeup_source(&standby_info);
#elif defined(CONFIG_ARCH_SUN50I)
#if defined(CONFIG_ARM)
	arm_cpuidle_suspend(3);
#elif defined(CONFIG_ARM64)
	cpu_suspend(3);
#endif
#endif

	exit_wakeup_src(standby_info.standby_para.event,
				standby_info.standby_para.gpio_enable_bitmap,
				standby_info.standby_para.cpux_gpiog_bitmap);
	save_pm_secure_mem_status(error_gen
				  (MOD_RESUME_CPUXSYS,
				   ERR_RESUME_CPUXSYS_CONFIG_WAKEUP_SRC_DONE));
#endif
#if defined(CONFIG_ARCH_SUN8IW10P1) || defined(CONFIG_ARCH_SUN8IW11P1)
#if defined(CONFIG_ARCH_SUN8IW10P1)
	writel(sram_ctrl_reg_backup, (void *)(SYS_CTRL + SRAM_CTRL_REG));
#endif
	mem_device_restore();
#endif
	return 0;

}

/*
*********************************************************************************************************
*                           verify_restore
*
*Description: verify src and dest region is the same;
*
*Return     : 0: same;
*                -1: different;
*
*Notes      :
*********************************************************************************************************
*/
#ifdef VERIFY_RESTORE_STATUS
static int verify_restore(void *src, void *dest, int count)
{
	volatile char *s = (volatile char *)src;
	volatile char *d = (volatile char *)dest;

	while (count--) {
		if (*(s + (count)) != *(d + (count))) {
			/*busy_waiting(); */
			return -1;
		}
	}

	return 0;
}
#endif

/*
*********************************************************************************************************
*                           aw_super_standby
*
*Description: enter super standby;
*
*Return     : return 0 is process successed;
*
*Notes      :
*********************************************************************************************************
*/
static int aw_super_standby(suspend_state_t state)
{
	int result = 0;

	standby_level = STANDBY_WITH_POWER_OFF;
	mem_para_info.debug_mask = debug_mask;
	mem_para_info.suspend_delay_ms = suspend_delay_ms;

	/* config cpus wakeup event type */
#ifdef CONFIG_SUNXI_ARISC
	if (PM_SUSPEND_MEM == state || PM_SUSPEND_STANDBY == state) {
		mem_para_info.axp_event = CPUS_MEM_WAKEUP;
	} else if (PM_SUSPEND_BOOTFAST == state) {
		mem_para_info.axp_event = CPUS_BOOTFAST_WAKEUP;
	}
#else
	mem_para_info.axp_event = 0;

#endif
	save_pm_secure_mem_status(error_gen
				  (MOD_SUSPEND_CPUXSYS,
				   ERR_SUSPEND_CPUXSYS_CONFIG_MEM_PARA_DONE));
	result = aw_early_suspend();

	save_pm_secure_mem_status(error_gen
				  (MOD_RESUME_CPUXSYS,
				   ERR_RESUME_CPUXSYS_RESUME_DEVICES_DONE));

	return 0;

}

/*
 *********************************************************************************************************
 *                           aw_pm_enter
 *
 *Description: Enter the system sleep state;
 *
 *Arguments  : state     system sleep state;
 *
 *Return     : return 0 is process successed;
 *
 *Notes      : this function is the core function for platform sleep.
 *********************************************************************************************************
 */
static int aw_pm_enter(suspend_state_t state)
{
#ifdef CONFIG_ARCH_SUN8IW3P1
	int val = 0;
	int sram_backup = 0;
#endif

	PM_DBG("enter state %d\n", state);

	save_pm_secure_mem_status(error_gen(MOD_SUSPEND_CORE, 0));
	if (unlikely(0 == console_suspend_enabled)) {
		debug_mask |=
		    (PM_STANDBY_PRINT_RESUME | PM_STANDBY_PRINT_STANDBY);
	} else {
		debug_mask &=
		    ~(PM_STANDBY_PRINT_RESUME | PM_STANDBY_PRINT_STANDBY);
	}

#ifdef CONFIG_ARCH_SUN8IW3P1
	/*#if 0 */
	val = readl((const volatile void __iomem *)SRAM_CTRL_REG1_ADDR_VA);
	sram_backup = val;
	val |= 0x1000000;
	writel(val, (volatile void __iomem *)SRAM_CTRL_REG1_ADDR_VA);
#endif

	/* show device: cpux_io, cpus_io, ccu status */
	aw_pm_show_dev_status();

#if (defined(CONFIG_ARCH_SUN8IW8P1) || \
	defined(CONFIG_ARCH_SUN8IW6P1) || \
	defined(CONFIG_ARCH_SUN50IW1P1) || \
	defined(CONFIG_ARCH_SUN50IW2P1)) && \
	defined(CONFIG_AW_AXP)
	if (unlikely(debug_mask & PM_STANDBY_PRINT_PWR_STATUS)) {
		printk(KERN_INFO "power status as follow:");
		axp_regulator_dump();
	}

	/* check pwr usage info: change sys_id according module usage info. */
	check_pwr_status();
	/* config sys_pwr_dm according to the sysconfig */
	config_sys_pwr();
#endif

	extended_standby_manager_id = get_extended_standby_manager();
	extended_standby_show_state();
	save_pm_secure_mem_status(error_gen
				  (MOD_SUSPEND_CPUXSYS,
				   ERR_SUSPEND_CPUXSYS_SHOW_DEVICES_STATE_DONE));

	aw_super_standby(state);

#ifdef CONFIG_SUNXI_ARISC
	arisc_query_wakeup_source(&mem_para_info.axp_event);
	PM_DBG("platform wakeup, standby wakesource is:0x%x\n",
	       mem_para_info.axp_event);
	parse_wakeup_event(NULL, 0, mem_para_info.axp_event);
#else
	mem_para_info.axp_event = standby_info.standby_para.event;
	PM_DBG("platform wakeup, standby wakesource is:0x%x\n",
	       mem_para_info.axp_event);
	parse_wakeup_event(NULL, 0, mem_para_info.axp_event);
#endif

	if (mem_para_info.axp_event & (CPUS_WAKEUP_LONG_KEY)) {
#ifdef	CONFIG_AW_AXP
		axp_powerkey_set(0);
#endif
	}

	/* show device: cpux_io, cpus_io, ccu status */
	aw_pm_show_dev_status();

#if (defined(CONFIG_ARCH_SUN8IW8P1) || \
		defined(CONFIG_ARCH_SUN8IW6P1) || \
		defined(CONFIG_ARCH_SUN50IW1P1) || \
		defined(CONFIG_ARCH_SUN50IW2P1)) && \
	defined(CONFIG_AW_AXP)
	resume_sys_pwr_state();
#endif


#ifdef CONFIG_ARCH_SUN8IW3P1
	/*#if 0 */
	writel(sram_backup, (volatile void __iomem *)SRAM_CTRL_REG1_ADDR_VA);
#endif
	save_pm_secure_mem_status(error_gen
				  (MOD_RESUME_CPUXSYS,
				   ERR_RESUME_CPUXSYS_RESUME_CPUXSYS_DONE));
	return 0;
}

/*
 *********************************************************************************************************
 *                           aw_pm_wake
 *
 *Description: platform wakeup;
 *
 *Arguments  : none;
 *
 *Return     : none;
 *
 *Notes      : This function called when the system has just left a sleep state, right after
 *             the nonboot CPUs have been enabled and before device drivers' early resume
 *             callbacks are executed. This function is opposited to the aw_pm_prepare_late;
 *********************************************************************************************************
 */
static void aw_pm_wake(void)
{
	save_pm_secure_mem_status(error_gen(MOD_RESUME_PROCESSORS, 0));
	PM_DBG("%s \n", __func__);

#if (defined(CONFIG_ARCH_SUN8IW10P1) || defined(CONFIG_ARCH_SUN8IW11P1)) && defined(CONFIG_AW_AXP)
	/* restore axp regulator state if need  */
	axp_mem_restore();
	resume_sys_pwr_state();
	if (unlikely(debug_mask & PM_STANDBY_PRINT_PWR_STATUS)) {
		printk(KERN_INFO "power status as follow:");
		/* notice:
		 * with no cpus, the axp will use twi0 which
		 * will use schedule interface to read axp status.
		 * considering schedule is prohibited  when timekeeping is in suspended state.
		 * so call axp_regulator_dump() before calling timekeeping_suspend().
		 */
		axp_regulator_dump();
	}
#endif
	return;
}

/*
*********************************************************************************************************
*                           aw_pm_finish
*
*Description: Finish wake-up of the platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called right prior to calling device drivers' regular suspend
*              callbacks. This function is opposited to the aw_pm_prepare function.
*********************************************************************************************************
*/
static void aw_pm_finish(void)
{
#ifdef CONFIG_CPU_FREQ
	struct cpufreq_policy policy;
#endif
	save_pm_secure_mem_status(error_gen
				  (MOD_RESUME_DEVICES,
				   ERR_RESUME_DEVICES_EARLY_RESUME_DEVICES_DONE));
	PM_DBG("platform wakeup finish\n");

#ifdef CONFIG_CPU_FREQ
	if (cpufreq_get_policy(&policy, 0))
		goto out;

	cpufreq_driver_target(&policy, policy.max, CPUFREQ_RELATION_L);
#endif
	return;

#ifdef CONFIG_CPU_FREQ
      out:
	return;
#endif
}

/*
*********************************************************************************************************
*                           aw_pm_end
*
*Description: Notify the platform that system is in work mode now.
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function is called by the PM core right after resuming devices, to indicate to
*             the platform that the system has returned to the working state or
*             the transition to the sleep state has been aborted. This function is opposited to
*             aw_pm_begin function.
*********************************************************************************************************
*/
static void aw_pm_end(void)
{
	save_pm_secure_mem_status(error_gen
				  (MOD_RESUME_DEVICES,
				   ERR_RESUME_DEVICES_RESUME_DEVICES_DONE));
	PM_DBG("aw_pm_end!\n");

	save_pm_secure_mem_status(error_gen(MOD_RESUME_COMPLETE_FLAG, 0));
	return;
}

/*
*********************************************************************************************************
*                           aw_pm_recover
*
*Description: Recover platform from a suspend failure;
*
*Arguments  : none
*
*Return     : none
*
*Notes      : This function alled by the PM core if the suspending of devices fails.
*             This callback is optional and should only be implemented by platforms
*             which require special recovery actions in that situation.
*********************************************************************************************************
*/
static void aw_pm_recover(void)
{
	save_pm_secure_mem_status(error_gen
				  (MOD_SUSPEND_DEVICES,
				   ERR_SUSPEND_DEVICES_SUSPEND_DEVICES_FAILED));
	PM_DBG("aw_pm_recover\n");
}

/*
    define platform_suspend_ops which is registered into PM core.
*/
static struct platform_suspend_ops aw_pm_ops = {
	.valid = aw_pm_valid,
	.begin = aw_pm_begin,
	.prepare = aw_pm_prepare,
	.prepare_late = aw_pm_prepare_late,
	.enter = aw_pm_enter,
	.wake = aw_pm_wake,
	.finish = aw_pm_finish,
	.end = aw_pm_end,
	.recover = aw_pm_recover,
};

static DEVICE_ATTR(debug_mask, S_IRUGO | S_IWUSR | S_IWGRP,
		   debug_mask_show, debug_mask_store);

static DEVICE_ATTR(parse_status_code, S_IRUGO | S_IWUSR | S_IWGRP,
		   parse_status_code_show, parse_status_code_store);

static struct attribute *g[] = {
	&dev_attr_debug_mask.attr,
	&dev_attr_parse_status_code.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};

/*
*********************************************************************************************************
*                           aw_pm_init
*
*Description: initial pm sub-system for platform;
*
*Arguments  : none;
*
*Return     : result;
*
*Notes      :
*
*********************************************************************************************************
*/
static int __init aw_pm_init(void)
{
	int ret = 0;
	u32 value[3] = { 0, 0, 0 };
#if 0
	struct device_node *sram_a1_np;
	void __iomem *sram_a1_vbase;
	struct resource res;
	phys_addr_t pbase;
	size_t size;
#endif

	PM_DBG("aw_pm_init!\n");

	/*init debug state */
	pm_secure_mem_status_init("rtc");
	/*for auto test reason. */
	/**(volatile __u32 *)(STANDBY_STATUS_REG  + 0x08) = BOOT_UPGRADE_FLAG;*/

#if (defined(CONFIG_ARCH_SUN8IW8P1) || \
	defined(CONFIG_ARCH_SUN8IW6P1) || \
	defined(CONFIG_ARCH_SUN8IW10P1) || \
	defined(CONFIG_ARCH_SUN8IW11P1) || \
	defined(CONFIG_ARCH_SUN50IW1P1) || \
	defined(CONFIG_ARCH_SUN50IW2P1)) && \
	defined(CONFIG_AW_AXP)
	config_pmu_para();
	/*init sys_pwr_dm */
	init_sys_pwr_dm();
	config_dynamic_standby();
#endif

	standby_space.np =
	    of_find_compatible_node(NULL, NULL, "allwinner,standby_space");
	if (IS_ERR(standby_space.np)) {
		printk(KERN_ERR
		       "get [allwinner,standby_space] device node error\n");
		return -EINVAL;
	}

	ret =
	    of_property_read_u32_array(standby_space.np, "space1", value,
				       ARRAY_SIZE(value));
	if (ret) {
		printk(KERN_ERR "get standby_space1 err. \n");
		return -EINVAL;
	}
#if defined(CONFIG_ARCH_SUN8IW10P1) || defined(CONFIG_ARCH_SUN8IW11P1)
	ret = fetch_and_save_dram_para(&(standby_info.dram_para));
	if (ret) {
		printk(KERN_ERR "get standby dram para err. \n");
		return -EINVAL;
	}
	mem_device_init();
	/*sram_a1_np = of_find_compatible_node(NULL, NULL, "allwinner,sram_a1");
	   if (IS_ERR(sram_a1_np)) {
	   printk(KERN_ERR "get [allwinner,sram_a1] device node error\n");
	   return -EINVAL;
	   }

	   ret = of_address_to_resource(sram_a1_np, 0, &res);
	   if (ret || !res.start) {
	   printk(KERN_ERR "get sram_a1 pbase error\n");
	   return -EINVAL;
	   }
	   pbase = res.start;
	   size = resource_size(&res);
	   printk("%s: sram_a1_pbase = 0x%x , size=0x%x\n", __func__, (unsigned int)pbase, size);
	   sram_a1_vbase = of_iomap(sram_a1_np, 0);
	   if (!sram_a1_vbase)
	   panic("Can't map sram_a1 registers");
	   printk("%s: sram_a1_vbase = 0x%x\n", __func__, (unsigned int)sram_a1_vbase); */
#endif
	standby_space.standby_mem_base = (phys_addr_t) value[0];
	standby_space.extended_standby_mem_base = (phys_addr_t) value[0] + 0x400;	/*1K bytes offset */
	standby_space.mem_offset = (phys_addr_t) value[1];
	standby_space.mem_size = (size_t) value[2];

	suspend_set_ops(&aw_pm_ops);
	aw_pm_kobj = kobject_create_and_add("aw_pm", power_kobj);
	if (!aw_pm_kobj)
		return -ENOMEM;
	ret = sysfs_create_group(aw_pm_kobj, &attr_group);

	return ret ? ret : 0;
}

/*
*********************************************************************************************************
*                           aw_pm_exit
*
*Description: exit pm sub-system on platform;
*
*Arguments  : none
*
*Return     : none
*
*Notes      :
*
*********************************************************************************************************
*/
static void __exit aw_pm_exit(void)
{
	PM_DBG("aw_pm_exit!\n");
	suspend_set_ops(NULL);
}

module_param_named(suspend_freq, suspend_freq, int, S_IRUGO | S_IWUSR);
module_param_named(suspend_delay_ms, suspend_delay_ms, int, S_IRUGO | S_IWUSR);
module_param_named(standby_mode, standby_mode, int, S_IRUGO | S_IWUSR);
module_param_named(time_to_wakeup, time_to_wakeup, ulong, S_IRUGO | S_IWUSR);
subsys_initcall_sync(aw_pm_init);
module_exit(aw_pm_exit);

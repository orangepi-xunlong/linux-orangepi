/*
*********************************************************************************************************
*                                                    LINUX-KERNEL
*                                        AllWinner Linux Platform Develop Kits
*                                                   Kernel Module
*
*                                    (c) Copyright 2006-2011, kevin.z China
*                                             All Rights Reserved
*
* File    : pm.c
* By      : kevin.z
* Version : v1.0
* Date    : 2011-5-27 14:08
* Descript: power manager for allwinners chips platform.
* Update  : date                auther      ver     notes
*********************************************************************************************************
*/

#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/cpufreq.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/console.h>
#include <asm/uaccess.h>
#include <asm/delay.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/tlbflush.h>
#include <linux/power/aw_pm.h>
#include <asm/mach/map.h>
#include <asm/cacheflush.h>
#include "pm_o.h"
#include <linux/arisc/arisc.h>

#include <mach/sys_config.h>
#include <linux/power/scenelock.h>
#include <linux/kobject.h>
#include <linux/ctype.h>
#include <linux/regulator/consumer.h>
#include <linux/power/axp_depend.h>
#include <linux/power/scenelock.h>
#include "../../../../kernel/power/power.h"
#include <asm/firmware.h>
#include <mach/sunxi-smc.h>

static struct kobject *aw_pm_kobj;

//#define CROSS_MAPPING_STANDBY

#define AW_PM_DBG   1
#undef PM_DBG
#if(AW_PM_DBG)
    #define PM_DBG(format,args...)   printk("[pm]"format,##args)
#else
    #define PM_DBG(format,args...)   do{}while(0)
#endif

#ifdef RETURN_FROM_RESUME0_WITH_NOMMU
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

#ifdef ENTER_SUPER_STANDBY
#undef PRE_DISABLE_MMU
#endif

#ifdef ENTER_SUPER_STANDBY_WITH_NOMMU
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

#ifdef RETURN_FROM_RESUME0_WITH_MMU
#undef PRE_DISABLE_MMU
#endif

#ifdef WATCH_DOG_RESET
#define PRE_DISABLE_MMU    //actually, mean ,prepare condition to disable mmu
#endif

//#define VERIFY_RESTORE_STATUS

/* define major number for power manager */
#define AW_PMU_MAJOR    267

__u32 debug_mask = PM_STANDBY_TEST; //PM_STANDBY_PRINT_STANDBY | PM_STANDBY_PRINT_RESUME| PM_STANDBY_ENABLE_JTAG;
static int suspend_freq = SUSPEND_FREQ;
static int suspend_delay_ms = SUSPEND_DELAY_MS;
static unsigned long time_to_wakeup = 0;

extern char *standby_bin_start;
extern char *standby_bin_end;
extern char *suspend_bin_start;
extern char *suspend_bin_end;

#ifdef RESUME_FROM_RESUME1
extern char *resume1_bin_start;
extern char *resume1_bin_end;
#endif

/*mem_cpu_asm.S*/
extern int mem_arch_suspend(void);
extern int mem_arch_resume(void);
extern int disable_prefetch(void);
extern asmlinkage int mem_clear_runtime_context(void);
extern void save_runtime_context(__u32 *addr);
extern void clear_reg_context(void);

#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
extern void cpufreq_user_event_notify(void);
#endif
static struct aw_pm_info standby_info = {
    .standby_para = {
		.event = CPU0_WAKEUP_MSGBOX,
		.axp_event = CPUS_MEM_WAKEUP,
		.timeout = 0,
    },
    .pmu_arg = {
        .twi_port = 0,
        .dev_addr = 10,
    },
};

static struct clk_state saved_clk_state;
static __mem_tmr_reg_t saved_tmr_state;
static struct twi_state saved_twi_state;
static struct gpio_state saved_gpio_state;
static struct ccm_state  saved_ccm_state;

#ifdef CONFIG_ARCH_SUN9IW1P1
static struct cci400_state saved_cci400_state;
static struct gtbus_state saved_gtbus_state;
#endif

#if defined(CONFIG_ARCH_SUN9IW1P1) || defined(CONFIG_ARCH_SUN8IW6P1)
#ifndef CONFIG_SUNXI_TRUSTZONE
static __mem_tmstmp_reg_t saved_tmstmp_state;
#endif
#endif

#ifndef CONFIG_SUNXI_TRUSTZONE
static struct sram_state saved_sram_state;
#endif

#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1)) && defined(CONFIG_AW_AXP)
static int config_sys_pwr(void);
#endif

#ifdef GET_CYCLE_CNT
static int start = 0;
static int resume0_period = 0;
static int resume1_period = 0;

static int pm_start = 0;
static int invalidate_data_time = 0;
static int invalidate_instruct_time = 0;
static int before_restore_processor = 0;
static int after_restore_process = 0;
//static int restore_runtime_peroid = 0;

//late_resume timing
static int late_resume_start = 0;
static int backup_area_start = 0;
static int backup_area1_start = 0;
static int backup_area2_start = 0;
static int clk_restore_start = 0;
static int gpio_restore_start = 0;
static int twi_restore_start = 0;
static int int_restore_start = 0;
static int tmr_restore_start = 0;
static int sram_restore_start = 0;
static int late_resume_end = 0;
#endif

struct aw_mem_para mem_para_info;
struct super_standby_para super_standby_para_info;
static const extended_standby_manager_t *extended_standby_manager_id = NULL;

standby_type_e standby_type = NON_STANDBY;
EXPORT_SYMBOL(standby_type);
standby_level_e standby_level = STANDBY_INITIAL;
EXPORT_SYMBOL(standby_level);

//static volatile int enter_flag = 0;
static int standby_mode = 0;
static int suspend_status_flag = 0;

#ifdef	CONFIG_AW_AXP
extern void axp_powerkey_set(int value);
#endif

#define pm_printk(mask, format, args... )   do	{   \
    if(unlikely(debug_mask&mask)){		    \
	printk(KERN_INFO format, ##args);   \
    }					    \
}while(0)

static char *parse_debug_mask(unsigned int bitmap, char *s, char *end)
{
    int i = 0;
    int counted = 0;
    int count = 0;
    unsigned int bit_event = 0;
    
    for(i=0; i<32; i++){
	bit_event = (1<<i & bitmap);
	switch(bit_event){
	case 0				  : break;
	case PM_STANDBY_PRINT_STANDBY	  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_STANDBY         ", PM_STANDBY_PRINT_STANDBY	  ); count++; break;
	case PM_STANDBY_PRINT_RESUME	  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_RESUME          ", PM_STANDBY_PRINT_RESUME	  ); count++; break;
	case PM_STANDBY_ENABLE_JTAG		  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_ENABLE_JTAG           ", PM_STANDBY_ENABLE_JTAG	  ); count++; break;
	case PM_STANDBY_PRINT_PORT		  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_PORT            ", PM_STANDBY_PRINT_PORT	  ); count++; break;
	case PM_STANDBY_PRINT_IO_STATUS 	  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_IO_STATUS       ", PM_STANDBY_PRINT_IO_STATUS  ); count++; break;
	case PM_STANDBY_PRINT_CACHE_TLB_MISS  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_CACHE_TLB_MISS  ", PM_STANDBY_PRINT_CACHE_TLB_MISS); count++; break;
	case PM_STANDBY_PRINT_CCU_STATUS 	  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_CCU_STATUS      ", PM_STANDBY_PRINT_CCU_STATUS  ); count++; break;
	case PM_STANDBY_PRINT_PWR_STATUS 	  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_PWR_STATUS      ", PM_STANDBY_PRINT_PWR_STATUS  ); count++; break;
	case PM_STANDBY_PRINT_CPUS_IO_STATUS  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_CPUS_IO_STATUS  ", PM_STANDBY_PRINT_CPUS_IO_STATUS); count++; break;
	case PM_STANDBY_PRINT_CCI400_REG	  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_CCI400_REG      ", PM_STANDBY_PRINT_CCI400_REG   ); count++; break;
	case PM_STANDBY_PRINT_GTBUS_REG	  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_GTBUS_REG       ", PM_STANDBY_PRINT_GTBUS_REG  ); count++; break;
	case PM_STANDBY_TEST		  : s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_TEST                  ", PM_STANDBY_TEST		  ); count++; break;
	case PM_STANDBY_PRINT_RESUME_IO_STATUS: s += scnprintf(s, end-s, "%-34s bit 0x%x\t", "PM_STANDBY_PRINT_RESUME_IO_STATUS", PM_STANDBY_PRINT_RESUME_IO_STATUS); count++; break;
	default				  : break;
	
	}
	if(counted != count && 0 == count%2){
	    counted = count;
	    s += scnprintf(s, end-s,"\n");
	}
    }
    
    s += scnprintf(s, end-s,"\n");

    return s;
}
static ssize_t debug_mask_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	char *s = buf;
	char *end = buf + PAGE_SIZE;
	
	s += sprintf(buf, "0x%x\n", debug_mask);
	s = parse_debug_mask(debug_mask, s, end);

	s += sprintf(s, "%s\n", "debug_mask usage help info:");
	s += sprintf(s, "%s\n", "target: for enable checking the io, ccu... suspended status.");
	s += sprintf(s, "%s\n", "bitmap: each bit corresponding one module, as follow:");
	s = parse_debug_mask(0xffff, s, end);

	return (s-buf);
}

#define TMPBUFLEN 22
static int get_long(const char **buf, size_t *size, unsigned long *val, bool *neg)
{
	int len;
	char *p, tmp[TMPBUFLEN];

	if (!*size)
		return -EINVAL;

	len = *size;
	if (len > TMPBUFLEN - 1)
		len = TMPBUFLEN - 1;

	memcpy(tmp, *buf, len);

	tmp[len] = 0;
	p = tmp;
	if (*p == '-' && *size > 1) {
		*neg = true;
		p++;
	} else
		*neg = false;
	if (!isdigit(*p))
		return -EINVAL;

	*val = simple_strtoul(p, &p, 0);

	len = p - tmp;

	/* We don't know if the next char is whitespace thus we may accept
	 * invalid integers (e.g. 1234...a) or two integers instead of one
	 * (e.g. 123...1). So lets not allow such large numbers. */
	if (len == TMPBUFLEN - 1)
		return -EINVAL;

	return 0;
}


static ssize_t debug_mask_store(struct device *dev,
	struct device_attribute *attr,
	const char *buf, size_t count)
{
    unsigned long data = 0;
    bool neg = false;

    if(!get_long(&buf, &count, &data, &neg)){
	if(true != neg){
	    debug_mask = (unsigned int)data;
	}else{
	    printk("%s\n", "minus is Illegal. ");
	    return -EINVAL;
	}
    }else{
	printk("%s\n", "non-digital is Illegal. ");
	return -EINVAL;
    }

    return count;
}

#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1)) && defined(CONFIG_AW_AXP)
static unsigned int pwr_dm_mask_saved = 0;
static int save_sys_pwr_state(const char *id)
{
	int bitmap = 0;

	bitmap = is_sys_pwr_dm_id(id);
	if ((-1) != bitmap) {
		pwr_dm_mask_saved |= (0x1 << bitmap);
	} else {
		pm_printk(PM_STANDBY_PRINT_PWR_STATUS, KERN_INFO "%s: is not sys \n", id);
	}
	return 0;
}

static int resume_sys_pwr_state(void)
{
	int i = 0, ret = -1;
	char *sys_name = NULL;

	for (i=0; i<32; i++) {
		if (pwr_dm_mask_saved & (0x1 << i)) {
			sys_name = get_sys_pwr_dm_id(i);
			if (NULL != sys_name) {
				ret = add_sys_pwr_dm(sys_name);
				if (ret < 0) {
					pm_printk(PM_STANDBY_PRINT_PWR_STATUS, KERN_INFO "%s: resume failed \n", sys_name);
				}
			}
		}
	}
	pwr_dm_mask_saved = 0;
	return 0;
}

static int check_sys_pwr_dm_status(char *pwr_dm)
{
    char  ldo_name[20] = "\0";
    char  enable_id[20] = "\0";
    int ret  = 0;
    int i = 0;

    ret = get_ldo_name(pwr_dm, ldo_name);
    if(ret < 0){
	printk(KERN_ERR "%s call %s failed. ret = %d \n", pwr_dm, __func__, ret);
	return -1;
    }
    ret = get_enable_id_count(ldo_name);
    if(0 == ret){
	pm_printk(PM_STANDBY_PRINT_PWR_STATUS, KERN_INFO "%s: no child, use by %s, property: sys.\n", ldo_name, pwr_dm);
    }else{
	for(i = 0; i < ret; i++){
	    get_enable_id(ldo_name, i, (char *)enable_id);
	    printk(KERN_INFO "%s: active child id %d is: %s. ",  ldo_name,  i, enable_id);
	    //need to check all enabled id is belong to sys_pwr_dm.
	    if((-1) != is_sys_pwr_dm_id(enable_id)){
		pm_printk(PM_STANDBY_PRINT_PWR_STATUS, KERN_INFO "property: sys \n");
	    }else{
		pm_printk(PM_STANDBY_PRINT_PWR_STATUS, KERN_INFO "property: module \n");
		del_sys_pwr_dm(pwr_dm);
		save_sys_pwr_state(pwr_dm);
		break;
	    }
	}
    }
    return 0;

}

static int check_pwr_status(void)
{
    check_sys_pwr_dm_status("vdd-cpua");
    check_sys_pwr_dm_status("vdd-cpub");
    check_sys_pwr_dm_status("vcc-dram");
    check_sys_pwr_dm_status("vdd-gpu");
    check_sys_pwr_dm_status("vdd-sys");
    check_sys_pwr_dm_status("vdd-vpu");
    check_sys_pwr_dm_status("vdd-cpus");
    check_sys_pwr_dm_status("vdd-drampll");
    check_sys_pwr_dm_status("vcc-adc");
    check_sys_pwr_dm_status("vcc-pl");
    check_sys_pwr_dm_status("vcc-pm");
    check_sys_pwr_dm_status("vcc-io");
    check_sys_pwr_dm_status("vcc-cpvdd");
    check_sys_pwr_dm_status("vcc-ldoin");
    check_sys_pwr_dm_status("vcc-pll");

    return 0;
}

static int init_sys_pwr_dm(void)
{
    unsigned int sys_mask = 0;

    add_sys_pwr_dm("vdd-cpua");
    //add_sys_pwr_dm("vdd-cpub");
    add_sys_pwr_dm("vcc-dram");
    //add_sys_pwr_dm("vdd-gpu");
    add_sys_pwr_dm("vdd-sys");
    //add_sys_pwr_dm("vdd-vpu");
    add_sys_pwr_dm("vdd-cpus");
    add_sys_pwr_dm("vdd-drampll");
    add_sys_pwr_dm("vcc-adc");
    add_sys_pwr_dm("vcc-pl");
    //add_sys_pwr_dm("vcc-pm");
    add_sys_pwr_dm("vcc-io");
    //add_sys_pwr_dm("vcc-cpvdd");
    add_sys_pwr_dm("vcc-ldoin");
    add_sys_pwr_dm("vcc-pll");
    
    sys_mask = get_sys_pwr_dm_mask();
    printk(KERN_INFO "after inited: sys_mask config = 0x%x. \n", sys_mask);

    return 0;
}
#endif 

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

    if(!((state > PM_SUSPEND_ON) && (state < PM_SUSPEND_MAX))){
        PM_DBG("state (%d) invalid!\n", state);
        return 0;
    }

#ifdef CHECK_IC_VERSION
	if(1 == standby_mode){
			version = sw_get_ic_ver();
			if(!(MAGIC_VER_A13B == version || MAGIC_VER_A12B == version || MAGIC_VER_A10SB == version)){
				pr_info("ic version: %d not support super standby. \n", version);
				standby_mode = 0;
			}
	}
#endif

	//if 1 == standby_mode, actually, mean mem corresponding with super standby
	if(PM_SUSPEND_STANDBY == state){
		if(1 == standby_mode){
			standby_type = NORMAL_STANDBY;
		}else{
			standby_type = SUPER_STANDBY;
		}
	}else if(PM_SUSPEND_MEM == state || PM_SUSPEND_BOOTFAST == state){
		if(1 == standby_mode){
			standby_type = SUPER_STANDBY;
		}else{
			standby_type = NORMAL_STANDBY;
		}
	}

#if defined(CONFIG_ARCH_SUN9IW1P1) || defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW6P1)
	if(NORMAL_STANDBY == standby_type){
		printk("Notice: sun9i&sun8iw5 not need support normal standby, \
				change to super standby.\n");

		standby_type = SUPER_STANDBY;
	}
#elif defined(CONFIG_ARCH_SUN8IW8P1)
	if(SUPER_STANDBY == standby_type){
		printk("Notice: sun8iw8p1 not need support super standby, \
				change to normal standby.\n");
		standby_type = NORMAL_STANDBY;
	}
	/* default enter to extended standby's normal standby*/
	{
		static struct scene_lock  normal_lock;
		if (unlikely(check_scene_locked(SCENE_NORMAL_STANDBY))) {
			scene_lock_init(&normal_lock, SCENE_NORMAL_STANDBY,  "normal_standby");
			scene_lock(&normal_lock);
		}
	}

#endif

#ifdef GET_CYCLE_CNT
		// init counters:
		init_perfcounters (1, 0);
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
	static __u32 suspend_status = 0;
	static int  suspend_result = 0;    //record last suspend status, 0/-1
	static bool backup_console_suspend_enabled = 0;
	static bool backup_initcall_debug = 0; 
	static int backup_console_loglevel = 0;
	static __u32 backup_debug_mask = 0;

	PM_DBG("version 2014年09月25日 星期四 09时14分04秒_2b7dd9f33b4db4f767f10ac3e26343b3ed04ce04 %d state begin\n", state);
	//set freq max
#ifdef CONFIG_CPU_FREQ_USR_EVNT_NOTIFY
	//cpufreq_user_event_notify();
#endif

	/*must init perfcounter, because delay_us and delay_ms is depandant perf counter*/
#ifndef GET_CYCLE_CNT
	backup_perfcounter();
	init_perfcounters (1, 0);
#endif

	//check rtc status, if err happened, do sth to fix it.
	suspend_status = get_pm_secure_mem_status(); 
	suspend_status = FIRST_BOOT_FLAG; 
	if( (FIRST_BOOT_FLAG)!= suspend_status && (RESUME_COMPLETE_FLAG) != suspend_status){
	    suspend_result = -1;
	    printk("suspend_err, rtc gpr as follow: \n");
	    show_mem_status();
	    //adjust: loglevel, console_suspend, initcall_debug, debug_mask config.
	    //disable console suspend.
	    backup_console_suspend_enabled = console_suspend_enabled;
	    console_suspend_enabled = 0;
	    //enable initcall_debug
	    backup_initcall_debug = initcall_debug; 
	    initcall_debug = 1;
	    //change loglevel to 8
	    backup_console_loglevel = console_loglevel;
	    console_loglevel = 8;
	    //change debug_mask to 0xff
	    backup_debug_mask = debug_mask;
	    debug_mask |= 0x07;
	}else{
	    if(-1 == suspend_result){
		//restore console suspend.
		console_suspend_enabled = backup_console_suspend_enabled;
		//restore initcall_debug
		initcall_debug = backup_initcall_debug; 
		//restore console_loglevel
		console_loglevel = backup_console_loglevel;
		//restore debug_mask
		debug_mask = backup_debug_mask;
	    }
	    suspend_result = 0; 
	}
	save_pm_secure_mem_status(BEFORE_EARLY_SUSPEND |0x1);
	
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
	save_pm_secure_mem_status(BEFORE_EARLY_SUSPEND |0x3);

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
    save_pm_secure_mem_status(BEFORE_EARLY_SUSPEND |0x5);
    return 0;

#ifdef CONFIG_CPU_FREQ	
out:
    return -1;
#endif
}

static int aw_suspend_cpu_die(void)
{
	unsigned long actlr;
	
    disable_prefetch();
    
    /* step1: disable cache */
    asm("mrc    p15, 0, %0, c1, c0, 0" : "=r" (actlr) );
    actlr &= ~(1<<2);
    asm("mcr    p15, 0, %0, c1, c0, 0\n" : : "r" (actlr));

    /* step2: clean and ivalidate L1 cache */
    flush_cache_all();

    /* step3: execute a CLREX instruction */
    asm("clrex" : : : "memory", "cc");

    /* step4: switch cpu from SMP mode to AMP mode, aim is to disable cache coherency */
    asm("mrc    p15, 0, %0, c1, c0, 1" : "=r" (actlr) );
    actlr &= ~(1<<6);
    asm("mcr    p15, 0, %0, c1, c0, 1\n" : : "r" (actlr));

    /* step5: execute an ISB instruction */
    isb();
    /* step6: execute a DSB instruction  */
    dsb();

    /*
     * step7: if define trustzone, switch to secure os
     * and then enter to wfi mode
     */
#ifdef CONFIG_SUNXI_TRUSTZONE
    call_firmware_op(suspend);
#else
    /* step7: execute a WFI instruction */
    asm("wfi" : : : "memory", "cc");
#endif
    
    return 0;
}

/*
*********************************************************************************************************
*                           aw_early_suspend
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      : -1: data is ok;
*			-2: data has been destory.
*********************************************************************************************************
*/
static int aw_early_suspend(void)
{
	save_pm_secure_mem_status(BEFORE_EARLY_SUSPEND |0x9);
#define MAX_RETRY_TIMES (5)
	//backup device state
	mem_ccu_save(&(saved_ccm_state));
#ifdef CONFIG_ARCH_SUN9IW1P1	
	mem_gtbus_save(&(saved_gtbus_state));
	mem_cci400_save(&(saved_cci400_state));
#endif
#if defined(CONFIG_ARCH_SUN9IW1P1) || defined(CONFIG_ARCH_SUN8IW6P1)
#ifndef CONFIG_SUNXI_TRUSTZONE
	mem_tmstmp_save(&(saved_tmstmp_state));
#endif
#endif
	mem_clk_save(&(saved_clk_state));
	mem_gpio_save(&(saved_gpio_state));
	mem_tmr_save(&(saved_tmr_state));
	mem_twi_save(&(saved_twi_state));
#ifndef CONFIG_SUNXI_TRUSTZONE
	mem_sram_save(&(saved_sram_state));
#endif

#if 0
	//backup volt and freq state, after backup device state
	mem_twi_init(AXP_IICBUS);
	/* backup voltages */
	while(-1 == (mem_para_info.suspend_dcdc2 = mem_get_voltage(POWER_VOL_DCDC2)) && --retry){
		;
	}
	if(0 == retry){
		return -1;
	}else{
		retry = MAX_RETRY_TIMES;
	}

	while(-1 == (mem_para_info.suspend_dcdc3 = mem_get_voltage(POWER_VOL_DCDC3)) && --retry){
		;
	}
	if(0 == retry){
		return -1;
	}else{
		retry = MAX_RETRY_TIMES;
	}
#endif

#if 1
	mem_clk_init(1);
	/*backup bus ratio*/
	mem_clk_getdiv(&mem_para_info.clk_div);
	/*backup pll ratio*/
	mem_clk_get_pll_factor(&mem_para_info.pll_factor);
	/*backup ccu misc*/
	mem_clk_get_misc(&mem_para_info.clk_misc);
#endif

	//backup mmu
	save_mmu_state(&(mem_para_info.saved_mmu_state));

	//backup 0x0000,0000 page entry, size?
	save_mapping(MEM_SW_VA_SRAM_BASE);

	if(DRAM_MEM_PARA_INFO_SIZE < sizeof(mem_para_info)){
		//judge the reserved space for mem para is enough or not.
		return -1;

	}

	//clean all the data into dram
#ifdef CONFIG_SUNXI_TRUSTZONE
        /* note: switch to secureos and save monitor vector to mem_para_info. */
	mem_para_info.monitor_vector= call_firmware_op(suspend_prepare);
	call_firmware_op(get_cp15_status, virt_to_phys((void *)&(mem_para_info.saved_secure_mmu_state)));
#endif
	printk("hsr: monitor_vector %x\n", mem_para_info.monitor_vector);
	memcpy((void *)phys_to_virt(DRAM_MEM_PARA_INFO_PA), (void *)&mem_para_info, sizeof(mem_para_info));
	if ((NULL != extended_standby_manager_id) && (NULL != extended_standby_manager_id->pextended_standby))
		memcpy((void *)(phys_to_virt(DRAM_EXTENDED_STANDBY_INFO_PA)),
				(void *)(extended_standby_manager_id->pextended_standby),
				sizeof(*(extended_standby_manager_id->pextended_standby)));
	dmac_flush_range((void *)phys_to_virt(DRAM_MEM_PARA_INFO_PA), (void *)(phys_to_virt(DRAM_EXTENDED_STANDBY_INFO_PA) + DRAM_EXTENDED_STANDBY_INFO_SIZE));

	mem_arch_suspend();
	save_processor_state();

	//create 0x0000,0000 mapping table: 0x0000,0000 -> 0x0000,0000
	create_mapping();
	
#ifdef ENTER_SUPER_STANDBY
	//print_call_info();
	if (check_scene_locked(SCENE_BOOT_FAST))
		super_standby_para_info.event = mem_para_info.axp_event;
	else
		super_standby_para_info.event = CPUS_BOOTFAST_WAKEUP;
	
	// the wakeup src is independent of the scene_lock.
	// the developer only need to care about: the scene support the wakeup src;
	if (NULL != extended_standby_manager_id) {
		super_standby_para_info.event |= extended_standby_manager_id->event;
		super_standby_para_info.gpio_enable_bitmap = extended_standby_manager_id->wakeup_gpio_map;
		super_standby_para_info.cpux_gpiog_bitmap = extended_standby_manager_id->wakeup_gpio_group;
	}
	if ((super_standby_para_info.event & (CPUS_WAKEUP_DESCEND | CPUS_WAKEUP_ASCEND)) == 0) {
#ifdef	CONFIG_AW_AXP
		axp_powerkey_set(1);
#endif
	}
#ifdef RESUME_FROM_RESUME1
	super_standby_para_info.resume_code_src = (unsigned long)(virt_to_phys((void *)&resume1_bin_start));
	super_standby_para_info.resume_code_length = ((int)&resume1_bin_end - (int)&resume1_bin_start);
	super_standby_para_info.resume_entry = SRAM_FUNC_START_PA;
	if ((NULL != extended_standby_manager_id) && (NULL != extended_standby_manager_id->pextended_standby))
		super_standby_para_info.pextended_standby = (extended_standby_t *)(DRAM_EXTENDED_STANDBY_INFO_PA);
	else
		super_standby_para_info.pextended_standby = NULL;
#endif

	super_standby_para_info.timeout = time_to_wakeup;
	if(0 < super_standby_para_info.timeout){
		super_standby_para_info.event |= CPUS_WAKEUP_TIMEOUT;
	}
	
	save_pm_secure_mem_status(BEFORE_EARLY_SUSPEND |0xb);
	if(unlikely(debug_mask&PM_STANDBY_PRINT_STANDBY)){
		pr_info("standby info: \n");
		pr_info("resume1_bin_start = 0x%x, resume1_bin_end = 0x%x. \n", (int)&resume1_bin_start, (int)&resume1_bin_end);
		pr_info("resume_code_src = 0x%lx, resume_code_length = %ld. resume_code_length = %lx \n", \
			super_standby_para_info.resume_code_src, \
			super_standby_para_info.resume_code_length, \
			super_standby_para_info.resume_code_length);

		pr_info("total(def & dynamic config) standby wakeup src config: 0x%lx.\n", super_standby_para_info.event);
		parse_wakeup_event(NULL, 0, super_standby_para_info.event, CPUS_ID);
	}

#if 1
	//disable int to make sure the cpu0 into wfi state.
	mem_int_init();
	save_pm_secure_mem_status(BEFORE_EARLY_SUSPEND |0xd);
#ifdef CONFIG_SUNXI_ARISC
	arisc_standby_super((struct super_standby_para *)(&super_standby_para_info), NULL, NULL);
#endif
	save_pm_secure_mem_status(BEFORE_EARLY_SUSPEND |0xf);
	aw_suspend_cpu_die();
#endif
	pr_info("standby suspend failed\n");
	//busy_waiting();
#endif

	return -2;

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

	while(count--){
		if(*(s+(count)) != *(d+(count))){
			//busy_waiting();
			return -1;
		}
	}

	return 0;
}
#endif

/*
*********************************************************************************************************
*                           aw_late_resume
*
*Description: prepare necessary info for suspend&resume;
*
*Return     : return 0 is process successed;
*
*Notes      :
*********************************************************************************************************
*/
static void aw_late_resume(void)
{
	int i = 0;
	memcpy((void *)&mem_para_info, (void *)phys_to_virt(DRAM_MEM_PARA_INFO_PA), sizeof(mem_para_info));
	mem_para_info.mem_flag = 0;
	
#if defined(CONFIG_ARCH_SUN9IW1P1) || defined(CONFIG_ARCH_SUN8IW6P1)
#ifndef CONFIG_SUNXI_TRUSTZONE
	mem_tmstmp_restore(&(saved_tmstmp_state));
#endif
#endif
	save_pm_secure_mem_status(LATE_RESUME_START |0x0);
	//restore device state
	mem_clk_restore(&(saved_clk_state));
	save_pm_secure_mem_status(LATE_RESUME_START |0x1);
	mem_ccu_restore(&(saved_ccm_state));
	save_pm_secure_mem_status(LATE_RESUME_START |0x2);
	
	if(unlikely(debug_mask&PM_STANDBY_PRINT_RESUME_IO_STATUS)){
		printk("before io_resume. \n");
		printk(KERN_INFO "IO status as follow:");
		for(i=0; i<(GPIO_REG_LENGTH); i++){
			printk(KERN_INFO "ADDR = %x, value = %x .\n", \
				(volatile __u32)(IO_ADDRESS(AW_PIO_BASE) + i*0x04), *(volatile __u32 *)(IO_ADDRESS(AW_PIO_BASE) + i*0x04));
		}
	}
	
	mem_gpio_restore(&(saved_gpio_state));
	
	save_pm_secure_mem_status(LATE_RESUME_START |0x3);
	mem_twi_restore(&(saved_twi_state));
	mem_tmr_restore(&(saved_tmr_state));
	save_pm_secure_mem_status(LATE_RESUME_START |0x4);
	//mem_int_restore(&(saved_gic_state));
#ifndef CONFIG_SUNXI_TRUSTZONE
	mem_sram_restore(&(saved_sram_state));
#endif
	save_pm_secure_mem_status(LATE_RESUME_START |0x5);
#ifdef CONFIG_ARCH_SUN9IW1P1	
	mem_gtbus_restore(&(saved_gtbus_state));
	save_pm_secure_mem_status(LATE_RESUME_START |0x6);
//	mem_cci400_restore(&(saved_cci400_state));
#endif
	
	return;
}

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
	suspend_status_flag = 0;

mem_enter:
	if( 1 == mem_para_info.mem_flag){
		save_pm_secure_mem_status(BEFORE_LATE_RESUME |0x1);
		//print_call_info();
		invalidate_branch_predictor();
		//print_call_info();
		//must be called to invalidate I-cache inner shareable?
		// I+BTB cache invalidate
		__cpuc_flush_icache_all();
		save_pm_secure_mem_status(BEFORE_LATE_RESUME |0x2);
		//print_call_info();
		//disable 0x0000 <---> 0x0000 mapping
		save_pm_secure_mem_status(BEFORE_LATE_RESUME |0x3);
		restore_processor_state();
		//destroy 0x0000 <---> 0x0000 mapping
		save_pm_secure_mem_status(BEFORE_LATE_RESUME |0x4);
		restore_mapping(MEM_SW_VA_SRAM_BASE);
		save_pm_secure_mem_status(BEFORE_LATE_RESUME |0x5);
		mem_arch_resume();
		save_pm_secure_mem_status(BEFORE_LATE_RESUME |0x6);
		goto resume;
	}

	save_runtime_context(mem_para_info.saved_runtime_context_svc);
	mem_para_info.mem_flag = 1;
	standby_level = STANDBY_WITH_POWER_OFF;
	mem_para_info.resume_pointer = (void *)&&mem_enter;
	mem_para_info.debug_mask = debug_mask;
	mem_para_info.suspend_delay_ms = suspend_delay_ms;
	if(unlikely(debug_mask&PM_STANDBY_PRINT_STANDBY)){
		pr_info("resume_pointer = 0x%x. \n", (unsigned int)(mem_para_info.resume_pointer));
	}


	/* config cpus wakeup event type */
	if(PM_SUSPEND_MEM == state || PM_SUSPEND_STANDBY == state){
		mem_para_info.axp_event = CPUS_MEM_WAKEUP;
	}else if(PM_SUSPEND_BOOTFAST == state){
		mem_para_info.axp_event = CPUS_BOOTFAST_WAKEUP;
	}
	
	
	result = aw_early_suspend();
	if(-2 == result){
		//mem_para_info.mem_flag = 1;
		suspend_status_flag = 2;
		goto mem_enter;
	}else if(-1 == result){
		suspend_status_flag = 1;
		goto suspend_err;
	}

resume:
	aw_late_resume();
	save_pm_secure_mem_status(LATE_RESUME_START |0x7);
	//have been disable dcache in resume1
	//enable_cache();
	if(unlikely(debug_mask&PM_STANDBY_PRINT_RESUME)){
		print_call_info();
	}
	save_pm_secure_mem_status(LATE_RESUME_START |0x8);

suspend_err:
	if(unlikely(debug_mask&PM_STANDBY_PRINT_RESUME)){
		pr_info("suspend_status_flag = %d. \n", suspend_status_flag);
	}
	save_pm_secure_mem_status(LATE_RESUME_START |0x9);

	return 0;

}

static void init_wakeup_src(unsigned int event)
{
	//config int src.
	mem_int_init();
	mem_clk_init(1);
	/* initialise standby modules */
	if(unlikely(debug_mask&PM_STANDBY_PRINT_STANDBY)){
		//don't need init serial ,depend kernel?
		serial_init(0);
		printk("normal standby wakeup src config = 0x%x. \n", event);
	}
	
	mem_tmr_init();
	
	/* init some system wake source */
	if(event & CPU0_WAKEUP_MSGBOX){
	    if(unlikely(standby_info.standby_para.debug_mask&PM_STANDBY_PRINT_STANDBY)){
		printk("enable CPU0_WAKEUP_MSGBOX. \n");
	    }
	    mem_enable_int(INT_SOURCE_MSG_BOX);
	}
	
	if(event & CPU0_WAKEUP_EXINT){
	    printk("enable CPU0_WAKEUP_EXINT. \n");
	    mem_enable_int(INT_SOURCE_EXTNMI);
	}

	if(event & CPU0_WAKEUP_TIMEOUT){
	    printk("enable CPU0_WAKEUP_TIMEOUT. \n");
	    /* set timer for power off */
	    if(standby_info.standby_para.timeout) {
		mem_tmr_set(standby_info.standby_para.timeout);
		mem_enable_int(INT_SOURCE_TIMER0);
	    }
	}

	if(event & CPU0_WAKEUP_ALARM){
	    mem_enable_int(INT_SOURCE_ALARM);
	}

	if(event & CPU0_WAKEUP_KEY){
	    mem_key_init();
	    mem_enable_int(INT_SOURCE_LRADC);
	}
	if(event & CPU0_WAKEUP_IR){
	    mem_ir_init();
	    mem_enable_int(INT_SOURCE_IR0);
	    mem_enable_int(INT_SOURCE_IR1);
	}
	
	if(event & CPU0_WAKEUP_USB){
	    mem_usb_init();
	    mem_enable_int(INT_SOURCE_USBOTG);
	    mem_enable_int(INT_SOURCE_USBEHCI0);
	    mem_enable_int(INT_SOURCE_USBEHCI1);
	    mem_enable_int(INT_SOURCE_USBEHCI2);
	    mem_enable_int(INT_SOURCE_USBOHCI0);
	    mem_enable_int(INT_SOURCE_USBOHCI1);
	    mem_enable_int(INT_SOURCE_USBOHCI2);
	}
	
	if(event & CPU0_WAKEUP_PIO){
	    mem_enable_int(INT_SOURCE_GPIOA);
	    mem_enable_int(INT_SOURCE_GPIOB);
	    mem_enable_int(INT_SOURCE_GPIOC);
	    mem_enable_int(INT_SOURCE_GPIOD);
	    mem_enable_int(INT_SOURCE_GPIOE);
	    mem_enable_int(INT_SOURCE_GPIOF);
	    mem_enable_int(INT_SOURCE_GPIOG);
	    mem_enable_int(INT_SOURCE_GPIOH);
	    mem_enable_int(INT_SOURCE_GPIOI);
	    mem_enable_int(INT_SOURCE_GPIOJ);
	    mem_pio_clk_src_init();
	}

	return ;
}

static void exit_wakeup_src(unsigned int event)
{
	/* exit standby module */
	if(event & CPU0_WAKEUP_PIO){
	    mem_pio_clk_src_exit();
	}
	
	if(event & CPU0_WAKEUP_USB){
	    mem_usb_exit();
	}
	
	if(event & CPU0_WAKEUP_IR){
	    mem_ir_exit();
	}
	
	if(event & CPU0_WAKEUP_ALARM){
	}

	if(event & CPU0_WAKEUP_KEY){
	    mem_key_exit();
	}

	/* exit standby module */
	mem_tmr_exit();

	if(unlikely(debug_mask&PM_STANDBY_PRINT_STANDBY)){
		//restore serial clk & gpio config.
		serial_exit();
	}
	
	save_pm_secure_mem_status(BEFORE_LATE_RESUME);
	mem_int_exit();
	
	return ;
}

static void aw_pm_show_dev_status(void)
{
    int i = 0;
    if(unlikely(debug_mask&PM_STANDBY_PRINT_IO_STATUS)){
	printk(KERN_INFO "IO status as follow:");
	for(i=0; i<(GPIO_REG_LENGTH); i++){
	    printk(KERN_INFO "ADDR = %x, value = %x .\n", \
		    (volatile __u32)(IO_ADDRESS(AW_PIO_BASE) + i*0x04), *(volatile __u32 *)(IO_ADDRESS(AW_PIO_BASE) + i*0x04));
	}
    }

    if(unlikely(debug_mask&PM_STANDBY_PRINT_CPUS_IO_STATUS)){
	printk(KERN_INFO "CPUS IO status as follow:");
	for(i=0; i<(CPUS_GPIO_REG_LENGTH); i++){
	    printk(KERN_INFO "ADDR = %x, value = %x .\n", \
		    (volatile __u32)(IO_ADDRESS(AW_R_PIO_BASE) + i*0x04), *(volatile __u32 *)(IO_ADDRESS(AW_R_PIO_BASE) + i*0x04));
	}
    }

    if(unlikely(debug_mask&PM_STANDBY_PRINT_CCU_STATUS)){
	printk(KERN_INFO "CCU status as follow:");
	for(i=0; i<(CCU_REG_LENGTH); i++){
	    printk(KERN_INFO "ADDR = %x, value = %x .\n", \
		    (volatile __u32)(IO_ADDRESS(AW_CCM_BASE) + i*0x04), *(volatile __u32 *)(IO_ADDRESS(AW_CCM_BASE) + i*0x04));
	}
    }

    return ;
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
    int (*standby)(struct aw_pm_info *arg);
    unsigned int backuped_event = 0;
#ifdef CONFIG_ARCH_SUN8IW3P1
    int val = 0;
    int sram_backup = 0;
#endif
    
    PM_DBG("enter state %d\n", state);
    
    save_pm_secure_mem_status(BEFORE_EARLY_SUSPEND |0x7);
    if(unlikely(0 == console_suspend_enabled)){
	debug_mask |= (PM_STANDBY_PRINT_RESUME | PM_STANDBY_PRINT_STANDBY);
    }else{
	debug_mask &= ~(PM_STANDBY_PRINT_RESUME | PM_STANDBY_PRINT_STANDBY);
    }

#ifdef CONFIG_ARCH_SUN8IW3P1
    //#if 0
    val = readl((const volatile void __iomem *)SRAM_CTRL_REG1_ADDR_VA);
    sram_backup = val;
    val |= 0x1000000;
    writel(val, (volatile void __iomem *)SRAM_CTRL_REG1_ADDR_VA);
#endif

    /* show device: cpux_io, cpus_io, ccu status */
    aw_pm_show_dev_status();

#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1)) && defined(CONFIG_AW_AXP)
    if(unlikely(debug_mask&PM_STANDBY_PRINT_PWR_STATUS)){
	printk(KERN_INFO "power status as follow:");
	axp_regulator_dump();	
    }

    /* check pwr usage info: change sys_id according module usage info.*/
    check_pwr_status();
    /* config sys_pwr_dm according to the sysconfig */
    config_sys_pwr();
#endif

    extended_standby_manager_id = get_extended_standby_manager();
    extended_standby_show_state();

    if(NORMAL_STANDBY== standby_type){

	standby = (int (*)(struct aw_pm_info *arg))SRAM_FUNC_START;
	
	//move standby code to sram
	memcpy((void *)SRAM_FUNC_START, (void *)&standby_bin_start, (int)&standby_bin_end - (int)&standby_bin_start);
	
	/* prepare system wakeup evetn type */
	if(PM_SUSPEND_MEM == state || PM_SUSPEND_STANDBY == state){
	    standby_info.standby_para.axp_event = CPUS_MEM_WAKEUP;
	    standby_info.standby_para.event = CPU0_MEM_WAKEUP;
	}else if(PM_SUSPEND_BOOTFAST == state){
	    standby_info.standby_para.axp_event = CPUS_BOOTFAST_WAKEUP;
	    standby_info.standby_para.event = CPU0_BOOTFAST_WAKEUP;
	}

	//config extended_standby info.
	if (NULL != extended_standby_manager_id) {
	    standby_info.standby_para.axp_event |= extended_standby_manager_id->event;
	    standby_info.standby_para.gpio_enable_bitmap = extended_standby_manager_id->wakeup_gpio_map;
	    standby_info.standby_para.cpux_gpiog_bitmap = extended_standby_manager_id->wakeup_gpio_group;
	}
	
	if ((NULL != extended_standby_manager_id) && (NULL != extended_standby_manager_id->pextended_standby))
	    standby_info.standby_para.pextended_standby = (extended_standby_t *)(DRAM_EXTENDED_STANDBY_INFO_PA);
	else
	    standby_info.standby_para.pextended_standby = NULL;

	standby_info.standby_para.debug_mask = debug_mask;
	//ignore less than 1000ms
	standby_info.standby_para.timeout = (time_to_wakeup/1000);
	if(0 < standby_info.standby_para.timeout){
	    standby_info.standby_para.axp_event |= CPUS_WAKEUP_TIMEOUT;
	    standby_info.standby_para.event |= CPU0_WAKEUP_TIMEOUT;
	}
#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1)) && defined(CONFIG_AW_AXP)
	get_pwr_regu_tree(standby_info.pmu_arg.soc_power_tree);
#endif
	//clean all the data into dram
	memcpy((void *)phys_to_virt(DRAM_MEM_PARA_INFO_PA), (void *)&mem_para_info, sizeof(mem_para_info));
	if ((NULL != extended_standby_manager_id) && (NULL != extended_standby_manager_id->pextended_standby))
	    memcpy((void *)(phys_to_virt(DRAM_EXTENDED_STANDBY_INFO_PA)),
		    (void *)(extended_standby_manager_id->pextended_standby),
		    sizeof(*(extended_standby_manager_id->pextended_standby)));
	dmac_flush_range((void *)phys_to_virt(DRAM_MEM_PARA_INFO_PA), (void *)(phys_to_virt(DRAM_EXTENDED_STANDBY_INFO_PA) + DRAM_EXTENDED_STANDBY_INFO_SIZE));

	// build the coherent between cache and memory
	//clean and flush: the data is already in cache, so can clean the data into sram;
	//while, in dma transfer condition, the data is not in cache, so can not use this api in dma ops.
	//at current situation, no need to clean & invalidate cache;
	//__cpuc_flush_kern_all();

	/* init some system wake source: including necessary init ops. */
	init_wakeup_src(standby_info.standby_para.event);

	save_pm_secure_mem_status(RESUME0_START);
	backuped_event = standby_info.standby_para.event;

	/* goto sram and run */
	standby(&standby_info);

	exit_wakeup_src(backuped_event);
	save_pm_secure_mem_status(BEFORE_LATE_RESUME | 0x1);

	PM_DBG("platform wakeup, normal standby cpu0 wakesource is:0x%x\n, normal standby cpus wakesource is:0x%x. \n", \
		standby_info.standby_para.event, standby_info.standby_para.axp_event);
		
	parse_wakeup_event(NULL, 0, standby_info.standby_para.event, CPU0_ID);
	parse_wakeup_event(NULL, 0, standby_info.standby_para.axp_event, CPUS_ID);

		save_pm_secure_mem_status(BEFORE_LATE_RESUME | 0x2);

    }else if(SUPER_STANDBY == standby_type){
	aw_super_standby(state);

#ifdef CONFIG_SUNXI_ARISC
	arisc_cpux_ready_notify();
	arisc_query_wakeup_source((unsigned int *)(&(mem_para_info.axp_event)));
#endif
	if (mem_para_info.axp_event & (CPUS_WAKEUP_LONG_KEY)) {
#ifdef	CONFIG_AW_AXP
	    axp_powerkey_set(0);
#endif
	}
	PM_DBG("platform wakeup, super standby wakesource is:0x%x\n", mem_para_info.axp_event);	
	parse_wakeup_event(NULL, 0, mem_para_info.axp_event, CPUS_ID);

	/* show device: cpux_io, cpus_io, ccu status */
        aw_pm_show_dev_status();

    }
	
#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1)) && defined(CONFIG_AW_AXP)
    resume_sys_pwr_state();
#endif

#ifdef CONFIG_ARCH_SUN8IW3P1	
    //#if 0
    writel(sram_backup,(volatile void __iomem *)SRAM_CTRL_REG1_ADDR_VA);
#endif
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
	save_pm_secure_mem_status(AFTER_LATE_RESUME |0x1);
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
    save_pm_secure_mem_status(AFTER_LATE_RESUME |0x2);
    PM_DBG("platform wakeup finish\n");

#ifdef CONFIG_CPU_FREQ	
    if (cpufreq_get_policy(&policy, 0))
	goto out;

    cpufreq_driver_target(&policy, policy.max, CPUFREQ_RELATION_L);
#endif
    return ;

#ifdef CONFIG_CPU_FREQ	
out:
    return ;
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
#ifndef GET_CYCLE_CNT
	#ifndef IO_MEASURE
			restore_perfcounter();
	#endif
#endif

	save_pm_secure_mem_status(RESUME_COMPLETE_FLAG);
	PM_DBG("aw_pm_end!\n");
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
	save_pm_secure_mem_status(AFTER_LATE_RESUME |0x3);
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

static DEVICE_ATTR(debug_mask, S_IRUGO|S_IWUSR|S_IWGRP,
		debug_mask_show, debug_mask_store);

static struct attribute * g[] = {
	&dev_attr_debug_mask.attr,
	NULL,
};
static struct attribute_group attr_group = {
	.attrs = g,
};

#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1)) && defined(CONFIG_AW_AXP)
static int config_pmux_para(unsigned num)
{
    script_item_u item;
#define PM_NAME_LEN (25)
    char name[PM_NAME_LEN] = "\0";
    int pmux_enable = 0;
    int pmux_id = 0;
    
    sprintf(name, "pmu%d_para", num);
    printk("pmu name: %s .\n", name);

    //get customer customized dynamic_standby config
    if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item(name, "pmu_used", &item)){
	pr_err("%s: script_parser_fetch err. \n", __func__);
	pmux_enable = 0;
    }else{
	pmux_enable = item.val;
    }
    printk(KERN_INFO "pmu%d_enable = 0x%x. \n", num, pmux_enable);
    
    if(1 == pmux_enable){
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item(name, "pmu_id", &item)){
	    pr_err("%s: script_parser_fetch err. \n", __func__);
	    pmux_id = 0;
	}else{
	    pmux_id = item.val;
	}
	printk(KERN_INFO "pmux_id = 0x%x. \n", pmux_id);
	extended_standby_set_pmu_id(num, pmux_id);

	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item(name, "pmu_twi_id", &item)){
	    pr_err("%s: script_parser_fetch err. \n", __func__);
	    standby_info.pmu_arg.twi_port = 0;
	}else{
	    standby_info.pmu_arg.twi_port = item.val;
	}

	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item(name, "pmu_twi_addr", &item)){
	    pr_err("%s: script_parser_fetch err. \n", __func__);
	    standby_info.pmu_arg.dev_addr = 0x34;
	}else{
	    standby_info.pmu_arg.dev_addr = item.val;
	}
    }
    
    return 0;
}

static int config_pmu_para(void)
{
    config_pmux_para(1);
    config_pmux_para(2);
    return 0;
}

static int init_dynamic_standby_volt(void)
{
	script_item_u item;
	int ret = 0;
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("dynamic_standby_para", "vdd_cpua_vol", &item)){
		pr_err("%s: vdd_cpua_vol script_parser_fetch err. \n", __func__);
	}else{
		ret = scene_set_volt(SCENE_DYNAMIC_STANDBY, VDD_CPUA_BIT, item.val);
		if (ret < 0)
			printk(KERN_ERR "%s: set vdd_cpua volt failed\n", __func__);
	}
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("dynamic_standby_para", "vdd_sys_vol", &item)){
		pr_err("%s: vdd_sys_vol script_parser_fetch err. \n", __func__);
	}else{
		ret = scene_set_volt(SCENE_DYNAMIC_STANDBY, VDD_SYS_BIT, item.val);
		if (ret < 0)
			printk(KERN_ERR "%s: set vdd_sys volt failed\n", __func__);
	}
	return 0;
}

static int config_dynamic_standby(void)
{
    script_item_u item;
    aw_power_scene_e type = SCENE_DYNAMIC_STANDBY;
    scene_extended_standby_t *local_standby;
    int enable = 0;
    int dram_selfresh_flag = 1;
    int i = 0;

    //get customer customized dynamic_standby config
    if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("dynamic_standby_para", "enable", &item)){
	pr_err("%s: script_parser_fetch err. \n", __func__);
	enable = 0;
    }else{
	enable = item.val;
    }
    printk(KERN_INFO "dynamic_standby enalbe = 0x%x. \n", enable);
    if(1 == enable){
	for (i = 0; i < extended_standby_cnt; i++) {
	    if (type == extended_standby[i].scene_type) {
		//config dram_selfresh flag;
		local_standby = &(extended_standby[i]);
		if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("dynamic_standby_para", "dram_selfresh_flag", &item)){
		    pr_err("%s: script_parser_fetch err. \n", __func__);
		    dram_selfresh_flag = 1;
		}else{
		    dram_selfresh_flag = item.val;
		}
		printk(KERN_INFO "dynamic_standby dram selfresh flag = 0x%x. \n", dram_selfresh_flag);
		if(0 == dram_selfresh_flag){
		    local_standby->soc_pwr_dep.soc_dram_state.selfresh_flag     = dram_selfresh_flag;
		    local_standby->soc_pwr_dep.soc_pwr_dm_state.state |= BITMAP(VDD_SYS_BIT);
		    local_standby->soc_pwr_dep.cpux_clk_state.osc_en         |= 0xf;	// mean all osc is on.
		    //mean pll5 is shutdowned & open by dram driver. 
		    //hsic can't closed.  
		    //periph is needed.
		    local_standby->soc_pwr_dep.cpux_clk_state.init_pll_dis   |= (BITMAP(PM_PLL_HSIC) | BITMAP(PM_PLL_PERIPH) | BITMAP(PM_PLL_DRAM));
		}

		//config other flag?
	    }

	    //config other extended_standby?
	}

	init_dynamic_standby_volt();

	printk(KERN_INFO "enable dynamic_standby by customer.\n");
	scene_lock_store(NULL, NULL, "dynamic_standby", 0);
    }

    return 0;
}

static int config_sys_pwr_dm(char *pwr_dm)
{
    script_item_u item;
    if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("sys_pwr_dm_para", pwr_dm , &item)){
	//pr_info("%s: script_parser_fetch err. not change. \n", __func__);
    }else{
	printk("%s: value: %d. \n", pwr_dm, item.val);
	if(0 == item.val) {
	    del_sys_pwr_dm(pwr_dm);	
	    save_sys_pwr_state(pwr_dm);
	}else{
	    add_sys_pwr_dm(pwr_dm);	
	}

    }

    return 0;
}

static int config_sys_pwr(void)
{
    unsigned int sys_mask = 0;
    
    config_sys_pwr_dm("vdd-cpua");
    config_sys_pwr_dm("vdd-cpub");
    config_sys_pwr_dm("vcc-dram");
    config_sys_pwr_dm("vdd-gpu");
    config_sys_pwr_dm("vdd-sys");
    config_sys_pwr_dm("vdd-vpu");
    config_sys_pwr_dm("vdd-cpus");
    config_sys_pwr_dm("vdd-drampll");
    config_sys_pwr_dm("vcc-adc");
    config_sys_pwr_dm("vcc-pl");
    config_sys_pwr_dm("vcc-pm");
    config_sys_pwr_dm("vcc-io");
    config_sys_pwr_dm("vcc-cpvdd");
    config_sys_pwr_dm("vcc-ldoin");
    config_sys_pwr_dm("vcc-pll");

    sys_mask = get_sys_pwr_dm_mask();
    printk(KERN_INFO "after customized: sys_mask config = 0x%x. \n", sys_mask);

    return 0;
}
#endif

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
	script_item_u item;
	script_item_u   *list = NULL;
	int wakeup_src_cnt = 0;
	unsigned gpio = 0;
	int i = 0;
	int error = 0;
	
	PM_DBG("aw_pm_init!\n");

	//get standby_mode.
	if(SCIRPT_ITEM_VALUE_TYPE_INT != script_get_item("pm_para", "standby_mode", &item)){
		pr_err("%s: script_parser_fetch err. \n", __func__);
		standby_mode = 0;
		//standby_mode = 1;
		pr_err("notice: standby_mode = %d.\n", standby_mode);
	}else{
		standby_mode = item.val;
		pr_info("standby_mode = %d. \n", standby_mode);
		if(1 != standby_mode){
			pr_err("%s: not support super standby. \n",  __func__);
		}
	}

	//get wakeup_src_cnt
	wakeup_src_cnt = script_get_pio_list("wakeup_src_para",&list);
	pr_info("wakeup src cnt is : %d. \n", wakeup_src_cnt);

	//script_dump_mainkey("wakeup_src_para");
	if(0 != wakeup_src_cnt){
		while(wakeup_src_cnt--){
			gpio = (list + (i++) )->gpio.gpio;
			extended_standby_enable_wakeup_src(CPUS_GPIO_SRC, gpio);
		}
	}


	/*init debug state*/
	pm_secure_mem_status_init(); 
	
	/*for auto test reason.*/
	//*(volatile __u32 *)(STANDBY_STATUS_REG  + 0x08) = BOOT_UPGRAGE_FLAG;
	suspend_set_ops(&aw_pm_ops);

#if (defined(CONFIG_ARCH_SUN8IW8P1) || defined(CONFIG_ARCH_SUN8IW6P1)) && defined(CONFIG_AW_AXP)
	config_pmu_para();
	/*init sys_pwr_dm*/
	init_sys_pwr_dm();
	config_dynamic_standby();
#endif


	aw_pm_kobj = kobject_create_and_add("aw_pm", power_kobj);
	if (!aw_pm_kobj)
		return -ENOMEM;
	error = sysfs_create_group(aw_pm_kobj, &attr_group);
	
	return error ? error : 0;
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
module_init(aw_pm_init);
module_exit(aw_pm_exit);


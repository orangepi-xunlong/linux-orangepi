/*
 * drivers/cpufreq/sunxi-cpufreq.c
 *
 * Copyright (c) 2012 softwinner.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <mach/sys_config.h>
#include <linux/cpu.h>
#include <asm/cpu.h>
#include <linux/pm.h>
#include <linux/arisc/arisc.h>
#include <linux/clk/sunxi_name.h>
#include <linux/regulator/consumer.h>
#include <linux/io.h>
#include <mach/platform.h>

#include "sunxi-cpufreq.h"

#ifdef CONFIG_DEBUG_FS
static unsigned long long cpufreq_set_time_usecs = 0;
static unsigned long long cpufreq_get_time_usecs = 0;
#endif

static struct sunxi_cpu_freq_t  cpu_cur;    /* current cpu frequency configuration  */
static unsigned int last_target = ~0;       /* backup last target frequency         */

#if !defined(CONFIG_ARCH_SUN8IW7P1) && !defined(CONFIG_ARCH_SUN8IW8P1)
struct regulator *cpu_vdd;  /* cpu vdd handler   */
#endif
static struct clk *clk_pll; /* pll clock handler */
static struct clk *clk_cpu; /* cpu clock handler */
static struct clk *clk_axi; /* axi clock handler */
static DEFINE_MUTEX(sunxi_cpu_lock);

static unsigned int cpu_freq_max   = SUNXI_CPUFREQ_MAX / 1000;
static unsigned int cpu_freq_min   = SUNXI_CPUFREQ_MIN / 1000;
static unsigned int cpu_freq_ext   = SUNXI_CPUFREQ_MAX / 1000;  // extremity cpu freq
static unsigned int cpu_freq_boot  = SUNXI_CPUFREQ_MAX / 1000;

int sunxi_dvfs_debug = 0;
int sunxi_boot_freq_lock = 0;

#if defined(CONFIG_ARCH_SUN8IW3P1) && defined(CONFIG_DEVFREQ_DRAM_FREQ)
extern int dramfreq_need_suspend;
extern int __ahb_set_rate(unsigned long ahb_freq);
#endif

int table_length_syscfg = 0;
struct cpufreq_dvfs dvfs_table_syscfg[16];

struct cpufreq_frequency_table sunxi_freq_tbl[] = {
    { .frequency = 60000  , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 120000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 240000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 312000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 408000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 480000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 504000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 528000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 576000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 600000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 624000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 648000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 672000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 720000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 768000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 816000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 864000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 912000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 960000 , .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1008000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1056000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1104000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1152000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1200000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1248000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1296000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1344000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1440000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },
    { .frequency = 1536000, .index = SUNXI_CLK_DIV(0, 0, 0, 0), },

	/* table end */
	{ .frequency = CPUFREQ_TABLE_END,  .index = 0,              },
};

/*
 *check if the cpu frequency policy is valid;
 */
static int sunxi_cpufreq_verify(struct cpufreq_policy *policy)
{
	return 0;
}


#if !defined(CONFIG_ARCH_SUN8IW7P1) && !defined(CONFIG_ARCH_SUN8IW8P1)
/*
 * get the current cpu vdd;
 * return: cpu vdd, based on mv;
 */
int sunxi_cpufreq_getvolt(void)
{
	return regulator_get_voltage(cpu_vdd) / 1000;
}
#endif

/*
 * get the frequency that cpu currently is running;
 * cpu:    cpu number, all cpus use the same clock;
 * return: cpu frequency, based on khz;
 */
static unsigned int sunxi_cpufreq_get(unsigned int cpu)
{
	unsigned int current_freq = 0;
#ifdef CONFIG_DEBUG_FS
	ktime_t calltime = ktime_set(0, 0), delta, rettime;

	calltime = ktime_get();
#endif

	clk_get_rate(clk_pll);
	current_freq = clk_get_rate(clk_cpu) / 1000;

#ifdef CONFIG_DEBUG_FS
	rettime = ktime_get();
	delta = ktime_sub(rettime, calltime);
	cpufreq_get_time_usecs = ktime_to_ns(delta) >> 10;
#endif

	return current_freq;
}


/*
 *show cpu frequency information;
 */
static void sunxi_cpufreq_show(const char *pfx, struct sunxi_cpu_freq_t *cfg)
{
	pr_debug("%s: pll=%u, cpudiv=%u, axidiv=%u\n", pfx, cfg->pll, cfg->div.cpu_div, cfg->div.axi_div);
}


/*
 * adjust the frequency that cpu is currently running;
 * policy:  cpu frequency policy;
 * freq:    target frequency to be set, based on khz;
 * relation:    method for selecting the target requency;
 * return:  result, return 0 if set target frequency successed, else, return -EINVAL;
 * notes:   this function is called by the cpufreq core;
 */
static int sunxi_cpufreq_target(struct cpufreq_policy *policy, __u32 freq, __u32 relation)
{
	int ret = 0;
	unsigned int            index;
	struct sunxi_cpu_freq_t freq_cfg;
	struct cpufreq_freqs    freqs;
#ifdef CONFIG_DEBUG_FS
	ktime_t calltime = ktime_set(0, 0), delta, rettime;
#endif
#ifdef CONFIG_SMP
	int i;
#endif

	mutex_lock(&sunxi_cpu_lock);

	/* avoid repeated calls which cause a needless amout of duplicated
	 * logging output (and CPU time as the calculation process is
	 * done) */
	if (freq == last_target)
		goto out;

	if (unlikely(sunxi_dvfs_debug))
		printk("[cpufreq] request frequency is %u\n", freq);

	if (unlikely(sunxi_boot_freq_lock))
		freq = freq > cpu_freq_boot ? cpu_freq_boot : freq;

	/* try to look for a valid frequency value from cpu frequency table */
	if (cpufreq_frequency_table_target(policy, sunxi_freq_tbl, freq, relation, &index)) {
		CPUFREQ_ERR("try to look for a valid frequency for %u failed!\n", freq);
		ret = -EINVAL;
		goto out;
	}

	/* frequency is same as the value last set, need not adjust */
	if (sunxi_freq_tbl[index].frequency == last_target)
		goto out;

	freq = sunxi_freq_tbl[index].frequency;

	/* update the target frequency */
	freq_cfg.pll = sunxi_freq_tbl[index].frequency * 1000;
	freq_cfg.div = *(struct sunxi_clk_div_t *)&sunxi_freq_tbl[index].index;

	if (unlikely(sunxi_dvfs_debug))
		printk("[cpufreq] target frequency find is %u, entry %u\n", freq_cfg.pll, index);

	/* notify that cpu clock will be adjust if needed */
	if (policy) {
		freqs.cpu = policy->cpu;
		freqs.old = last_target;
		freqs.new = freq;

#ifdef CONFIG_SMP
		/* notifiers */
		for_each_cpu(i, policy->cpus) {
			freqs.cpu = i;
			cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);
		}
#else
		cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);
#endif
	}

#ifdef CONFIG_DEBUG_FS
	calltime = ktime_get();
#endif

#if defined(CONFIG_ARCH_SUN8IW8P1)
	if (clk_set_rate(clk_pll, freq_cfg.pll)) {
#else
	/* try to set cpu frequency */
	if (arisc_dvfs_set_cpufreq(freq, ARISC_DVFS_PLL1, ARISC_DVFS_SYN, NULL, NULL)) {
#endif
		CPUFREQ_ERR("set cpu frequency to %uMHz failed!\n", freq / 1000);
		/* set cpu frequency failed */
		if (policy) {
			freqs.cpu = policy->cpu;
			freqs.old = freqs.new;
			freqs.new = last_target;

#ifdef CONFIG_SMP
			/* notifiers */
			for_each_cpu(i, policy->cpus) {
				freqs.cpu = i;
				cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
			}
#else
			cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
#endif
		}

		ret = -EINVAL;
		goto out;
	}

#ifdef CONFIG_DEBUG_FS
	rettime = ktime_get();
	delta = ktime_sub(rettime, calltime);
	cpufreq_set_time_usecs = ktime_to_ns(delta) >> 10;
#endif

	/* notify that cpu clock will be adjust if needed */
	if (policy) {
#ifdef CONFIG_SMP
		/*
		 * Note that loops_per_jiffy is not updated on SMP systems in
		 * cpufreq driver. So, update the per-CPU loops_per_jiffy value
		 * on frequency transition. We need to update all dependent cpus
		 */
		for_each_cpu(i, policy->cpus) {
			per_cpu(cpu_data, i).loops_per_jiffy =
				 cpufreq_scale(per_cpu(cpu_data, i).loops_per_jiffy, freqs.old, freqs.new);
			freqs.cpu = i;
			cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
		}
#else
		cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);
#endif
	}

	last_target = freq;

#if defined(CONFIG_ARCH_SUN8IW3P1) && defined(CONFIG_DEVFREQ_DRAM_FREQ)
	if (!dramfreq_need_suspend) {
		if (freq < 100000) {
			if (__ahb_set_rate(100000000)) {
				CPUFREQ_ERR("set ahb to 100MHz failed!\n");
			}
		} else {
			if (__ahb_set_rate(200000000)) {
				CPUFREQ_ERR("set ahb to 200MHz failed!\n");
			}
		}
	}
#endif

	if (unlikely(sunxi_dvfs_debug)) {
#if !defined(CONFIG_ARCH_SUN8IW7P1) && !defined(CONFIG_ARCH_SUN8IW8P1)
		printk("[cpufreq] DVFS done! Freq[%uMHz] Volt[%umv] ok\n\n", \
			sunxi_cpufreq_get(0) / 1000, sunxi_cpufreq_getvolt());
#else
		printk("[cpufreq] DFS done! Freq[%uMHz] ok\n\n", \
			sunxi_cpufreq_get(0) / 1000);
#endif
	}


out:
	mutex_unlock(&sunxi_cpu_lock);

	return ret;
}


/*
 * get the frequency that cpu average is running;
 * cpu:    cpu number, all cpus use the same clock;
 * return: cpu frequency, based on khz;
 */
static unsigned int sunxi_cpufreq_getavg(struct cpufreq_policy *policy, unsigned int cpu)
{
	return clk_get_rate(clk_cpu) / 1000;
}


/*
 * get a valid frequency from cpu frequency table;
 * target_freq: target frequency to be judge, based on KHz;
 * return: cpu frequency, based on khz;
 */
static unsigned int __get_valid_freq(unsigned int target_freq)
{
	struct cpufreq_frequency_table *tmp = &sunxi_freq_tbl[0];

	while(tmp->frequency != CPUFREQ_TABLE_END){
		if((tmp+1)->frequency <= target_freq)
			tmp++;
		else
			break;
	}

	return tmp->frequency;
}

static int __init_vftable_syscfg(char *tbl_name, int value)
{
	script_item_u val;
	script_item_value_type_e type;
	int i ,ret = -1;
	char name[16] = {0};

	type = script_get_item(tbl_name, "LV_count", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		CPUFREQ_ERR("fetch LV_count from sysconfig failed\n");
		goto fail;
	}

	table_length_syscfg = val.val;
	if(table_length_syscfg >= 16){
		CPUFREQ_ERR("LV_count from sysconfig is out of bounder\n");
		goto fail;
	}

	for (i = 1; i <= table_length_syscfg; i++){
		sprintf(name, "LV%d_freq", i);
		type = script_get_item(tbl_name, name, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			CPUFREQ_ERR("get LV%d_freq from sysconfig failed\n", i);
			goto fail;
		}
		dvfs_table_syscfg[i-1].freq = val.val / 1000;

		sprintf(name, "LV%d_volt", i);
		type = script_get_item(tbl_name, name, &val);
		if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
			CPUFREQ_ERR("get LV%d_volt from sysconfig failed\n", i);
			goto fail;
		}

#ifdef CONFIG_ARCH_SUN8IW3P1
		if (value) {
			dvfs_table_syscfg[i-1].volt = val.val >= SUNXI_CPUFREQ_VOLT_LIMIT ? val.val : SUNXI_CPUFREQ_VOLT_LIMIT;
		} else {
			dvfs_table_syscfg[i-1].volt = val.val;
		}
#else
		dvfs_table_syscfg[i-1].volt = val.val;
#endif
	}

	return 0;

fail:
	return ret;
}

static void __vftable_show(void)
{
	int i;

	pr_debug("----------------CPU V-F Table-------------\n");
	for(i = 0; i < table_length_syscfg; i++){
		pr_debug("\tfrequency = %4dKHz \tvoltage = %4dmv\n", \
				dvfs_table_syscfg[i].freq, dvfs_table_syscfg[i].volt);
	}
	pr_debug("------------------------------------------\n");
}

extern int sunxi_get_soc_chipid(uint8_t *chip_id);
static int calibrate_max_cpufreq(void *maxfreq)
{
    int ret = 0;
#ifdef CONFIG_ARCH_SUN8IW7P1
    unsigned char chipid[16];
    memset(chipid, 0, sizeof(chipid));

    sunxi_get_soc_chipid((uint8_t *)chipid);
    switch(chipid[0]&0xff) {
    case 0x42:      /* H2+ */
    case 0x83:
        *((unsigned int *)maxfreq) = 1008000000;
        ret = 1;
        break;
    case 0x00:      /* H3 */
    case 0x81:
        break;
    default:        /* H3 */
        break;
    }
#endif
    printk("calibrat: max_cpufreq %uMhz Type %d!\n", *((unsigned int *)maxfreq)/1000000, ret);
    return ret;
}

/*
 * init cpu max/min frequency from sysconfig;
 * return: 0 - init cpu max/min successed, !0 - init cpu max/min failed;
 */
static int __init_freq_syscfg(char *tbl_name)
{
	int ret = 0;
	script_item_u max, min, boot;
	script_item_value_type_e type;

	type = script_get_item(tbl_name, "max_freq", &max);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		CPUFREQ_ERR("get cpu max frequency from sysconfig failed\n");
		ret = -1;
		goto fail;
	}
	cpu_freq_max = max.val;
	calibrate_max_cpufreq((void *)&cpu_freq_max);

	type = script_get_item(tbl_name, "min_freq", &min);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		CPUFREQ_ERR("get cpu min frequency from sysconfig failed\n");
		ret = -1;
		goto fail;
	}
	cpu_freq_min = min.val;

	type = script_get_item(tbl_name, "extremity_freq", &max);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		CPUFREQ_ERR("get cpu extremity frequency from sysconfig failed, use max_freq\n");
		max.val = cpu_freq_max;
	}
	cpu_freq_ext = max.val;

	type = script_get_item(tbl_name, "boot_freq", &boot);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		boot.val = cpu_freq_max;
	} else {
		sunxi_boot_freq_lock = 1;
	}
	cpu_freq_boot = boot.val;

	if(cpu_freq_max > SUNXI_CPUFREQ_MAX || cpu_freq_max < SUNXI_CPUFREQ_MIN
		|| cpu_freq_min < SUNXI_CPUFREQ_MIN || cpu_freq_min > SUNXI_CPUFREQ_MAX){
		CPUFREQ_ERR("cpu max or min frequency from sysconfig is more than range\n");
		ret = -1;
		goto fail;
	}

	if(cpu_freq_min > cpu_freq_max){
		CPUFREQ_ERR("cpu min frequency can not be more than cpu max frequency\n");
		ret = -1;
		goto fail;
	}

	if (cpu_freq_ext < cpu_freq_max) {
		CPUFREQ_ERR("cpu ext frequency can not be less than cpu max frequency\n");
		ret = -1;
		goto fail;
	}

	if(cpu_freq_boot > cpu_freq_max || cpu_freq_boot < cpu_freq_min){
		CPUFREQ_ERR("cpu boot frequency can not be more than cpu max frequency\n");
		ret = -1;
		goto fail;
	}

	/* get valid max/min frequency from cpu frequency table */
	cpu_freq_max = __get_valid_freq(cpu_freq_max / 1000);
	cpu_freq_min = __get_valid_freq(cpu_freq_min / 1000);
	cpu_freq_ext = __get_valid_freq(cpu_freq_ext / 1000);
	cpu_freq_boot = __get_valid_freq(cpu_freq_boot / 1000);

	return 0;

fail:
	/* use default cpu max/min frequency */
	cpu_freq_max = SUNXI_CPUFREQ_MAX / 1000;
	cpu_freq_min = SUNXI_CPUFREQ_MIN / 1000;
	cpu_freq_ext = SUNXI_CPUFREQ_MAX / 1000;
	cpu_freq_boot = cpu_freq_max;

	return ret;
}


/*
 * cpu frequency initialise a policy;
 * policy:  cpu frequency policy;
 * result:  return 0 if init ok, else, return -EINVAL;
 */
static int sunxi_cpufreq_init(struct cpufreq_policy *policy)
{
	policy->cur = sunxi_cpufreq_get(0);
	policy->min = cpu_freq_min;
	policy->max = cpu_freq_ext;
	policy->cpuinfo.min_freq = cpu_freq_min;
	policy->cpuinfo.max_freq = cpu_freq_ext;
	policy->cpuinfo.boot_freq = cpu_freq_boot;
	policy->governor = CPUFREQ_DEFAULT_GOVERNOR;

	/* feed the latency information from the cpu driver */
	policy->cpuinfo.transition_latency = SUNXI_FREQTRANS_LATENCY;
	cpufreq_frequency_table_get_attr(sunxi_freq_tbl, policy->cpu);

#ifdef CONFIG_SMP
	/*
	 * both processors share the same voltage and the same clock,
	 * but have dedicated power domains. So both cores needs to be
	 * scaled together and hence needs software co-ordination.
	 * Use cpufreq affected_cpus interface to handle this scenario.
	 */
	policy->shared_type = CPUFREQ_SHARED_TYPE_ANY;
	cpumask_copy(policy->cpus, topology_core_cpumask(policy->cpu));
#endif

	return 0;
}


/*
 * get current cpu frequency configuration;
 * cfg:     cpu frequency cofniguration;
 * return:  result;
 */
static int sunxi_cpufreq_getcur(struct sunxi_cpu_freq_t *cfg)
{
	unsigned int    freq, freq0;

	if(!cfg) {
		return -EINVAL;
	}

	cfg->pll = clk_get_rate(clk_pll);
	freq = clk_get_rate(clk_cpu);
	cfg->div.cpu_div = cfg->pll / freq;
	freq0 = clk_get_rate(clk_axi);
	cfg->div.axi_div = freq / freq0;

	return 0;
}


#if defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1)
static int __set_pll_cpu_lock_time(void)
{
	unsigned int value;

	/* modify pll cpu lock time */
	value = readl(SUNXI_CCM_VBASE + 0x204);
	value &= ~(0xffff);
	value |= 0x400;
	writel(value, SUNXI_CCM_VBASE + 0x204);

	return 0;
}
#endif


#ifdef CONFIG_PM

/*
 * cpu frequency configuration suspend;
 */
static int sunxi_cpufreq_suspend(struct cpufreq_policy *policy)
{
	return 0;
}

/*
 * cpu frequency configuration resume;
 */
static int sunxi_cpufreq_resume(struct cpufreq_policy *policy)
{
#if defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1)
	__set_pll_cpu_lock_time();
#endif

	/* invalidate last_target setting */
	last_target = ~0;
	return 0;
}


#else   /* #ifdef CONFIG_PM */

#define sunxi_cpufreq_suspend   NULL
#define sunxi_cpufreq_resume    NULL

#endif  /* #ifdef CONFIG_PM */

/* Export freq_table to sysfs */
static struct freq_attr *sunxi_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver sunxi_cpufreq_driver = {
	.name       = "cpufreq-sunxi",
	.flags      = CPUFREQ_STICKY,
	.init       = sunxi_cpufreq_init,
	.verify     = sunxi_cpufreq_verify,
	.target     = sunxi_cpufreq_target,
	.get        = sunxi_cpufreq_get,
	.getavg     = sunxi_cpufreq_getavg,
	.suspend    = sunxi_cpufreq_suspend,
	.resume     = sunxi_cpufreq_resume,
	.attr	    = sunxi_cpufreq_attr,
};


/*
 * cpu frequency driver init
 */
static int __init sunxi_cpufreq_initcall(void)
{
	int ret = 0;
	char vftbl_name[16] = {0};
	int flag = 0;

#if defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1)
	__set_pll_cpu_lock_time();
#endif

#if defined(CONFIG_ARCH_SUN8IW1P1) || defined(CONFIG_ARCH_SUN8IW3P1)
	clk_pll = clk_get(NULL, PLL1_CLK);
#elif defined(CONFIG_ARCH_SUN8IW5P1) || defined(CONFIG_ARCH_SUN8IW7P1) || defined(CONFIG_ARCH_SUN8IW8P1)
	clk_pll = clk_get(NULL, PLL_CPU_CLK);
#endif

	clk_cpu = clk_get(NULL, CPU_CLK);
	clk_axi = clk_get(NULL, AXI_CLK);

	if (IS_ERR(clk_pll) || IS_ERR(clk_cpu) || IS_ERR(clk_axi)) {
		CPUFREQ_ERR("%s: could not get clock(s)\n", __func__);
		return -ENOENT;
	}

	pr_debug("%s: clocks pll=%lu,cpu=%lu,axi=%lu\n", __func__,
		   clk_get_rate(clk_pll), clk_get_rate(clk_cpu), clk_get_rate(clk_axi));

	/* initialise current frequency configuration */
	sunxi_cpufreq_getcur(&cpu_cur);
	sunxi_cpufreq_show("cur", &cpu_cur);

#if !defined(CONFIG_ARCH_SUN8IW7P1) && !defined(CONFIG_ARCH_SUN8IW8P1)
	cpu_vdd = regulator_get(NULL, SUNXI_CPUFREQ_CPUVDD);
	if (IS_ERR(cpu_vdd)) {
		CPUFREQ_ERR("%s: could not get cpu vdd\n", __func__);
		return -ENOENT;
	}
#endif

#ifdef CONFIG_ARCH_SUN8IW3P1
	writel(BIT(15), IO_ADDRESS(0x01c00024));
	if ((readl(IO_ADDRESS(0x01c00024)) >> 16) == MAGIC0) {
		sprintf(vftbl_name, "dvfs_table");
	} else {
		if (script_is_main_key_exist("dvfs_table_bak")) {
			sprintf(vftbl_name, "dvfs_table_bak");
		} else {
			sprintf(vftbl_name, "dvfs_table");
			flag = 1;
		}
	}
#else
	sprintf(vftbl_name, "dvfs_table");
#endif

	/* init cpu frequency from sysconfig */
	if(__init_freq_syscfg(vftbl_name)) {
		CPUFREQ_ERR("%s, use default cpu max/min frequency, max freq: %uMHz, min freq: %uMHz\n",
					__func__, cpu_freq_max/1000, cpu_freq_min/1000);
	}else{
		pr_debug("%s, get cpu frequency from sysconfig, max freq: %uMHz, min freq: %uMHz\n",
					__func__, cpu_freq_max/1000, cpu_freq_min/1000);
	}

	ret = __init_vftable_syscfg(vftbl_name, flag);
	if (ret) {
		CPUFREQ_ERR("%s get V-F table failed\n", __func__);
		return ret;
	} else {
		__vftable_show();
	}

	/* register cpu frequency driver */
	ret = cpufreq_register_driver(&sunxi_cpufreq_driver);

	return ret;
}
module_init(sunxi_cpufreq_initcall);

/*
 * cpu frequency driver exit
 */
static void __exit sunxi_cpufreq_exitcall(void)
{
#if !defined(CONFIG_ARCH_SUN8IW7P1) && !defined(CONFIG_ARCH_SUN8IW8P1)
	regulator_put(cpu_vdd);
#endif
	clk_put(clk_pll);
	clk_put(clk_cpu);
	clk_put(clk_axi);
	cpufreq_unregister_driver(&sunxi_cpufreq_driver);
}
module_exit(sunxi_cpufreq_exitcall);

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_cpufreq_root;

static int cpufreq_get_time_show(struct seq_file *s, void *data)
{
	seq_printf(s, "%Ld\n", cpufreq_get_time_usecs);
	return 0;
}

static int cpufreq_get_time_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpufreq_get_time_show, inode->i_private);
}

static const struct file_operations cpufreq_get_time_fops = {
	.open = cpufreq_get_time_open,
	.read = seq_read,
};

static int cpufreq_set_time_show(struct seq_file *s, void *data)
{
	seq_printf(s, "%Ld\n", cpufreq_set_time_usecs);
	return 0;
}

static int cpufreq_set_time_open(struct inode *inode, struct file *file)
{
	return single_open(file, cpufreq_set_time_show, inode->i_private);
}

static const struct file_operations cpufreq_set_time_fops = {
	.open = cpufreq_set_time_open,
	.read = seq_read,
};

static int __init cpufreq_debug_init(void)
{
	int err = 0;

	debugfs_cpufreq_root = debugfs_create_dir("cpufreq", 0);
	if (!debugfs_cpufreq_root)
		return -ENOMEM;

	if (!debugfs_create_file("get_time", 0444, debugfs_cpufreq_root, NULL, &cpufreq_get_time_fops)) {
		err = -ENOMEM;
		goto out;
	}

	if (!debugfs_create_file("set_time", 0444, debugfs_cpufreq_root, NULL, &cpufreq_set_time_fops)) {
		err = -ENOMEM;
		goto out;
	}

	return 0;

out:
	debugfs_remove_recursive(debugfs_cpufreq_root);
	return err;
}

static void __exit cpufreq_debug_exit(void)
{
	debugfs_remove_recursive(debugfs_cpufreq_root);
}

late_initcall(cpufreq_debug_init);
module_exit(cpufreq_debug_exit);
#endif /* CONFIG_DEBUG_FS */

MODULE_DESCRIPTION("cpufreq driver for sunxi SOCs");
MODULE_LICENSE("GPL");

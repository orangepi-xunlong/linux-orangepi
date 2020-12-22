/*
 * linux/drivers/cpufreq/sunxi-iks-cpufreq.c
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <sunny@allwinnertech.com>
 *
 * allwinner sunxi iks cpufreq driver.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/clk.h>
#include <linux/cpu.h>
#include <linux/cpufreq.h>
#include <linux/cpumask.h>
#include <linux/export.h>
#include <linux/mutex.h>
#include <linux/opp.h>
#include <linux/slab.h>
#include <linux/topology.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/clk/sunxi_name.h>
#include <asm/bL_switcher.h>
#include <linux/arisc/arisc.h>
#include <linux/regulator/consumer.h>
#include <mach/sys_config.h>
#include <mach/sunxi-chip.h>
#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#include <linux/hrtimer.h>
#endif

#include "sunxi-iks-cpufreq.h"

#ifdef CONFIG_DEBUG_FS
static unsigned long long c0_set_time_usecs = 0;
static unsigned long long c0_get_time_usecs = 0;
static unsigned long long c1_set_time_usecs = 0;
static unsigned long long c1_get_time_usecs = 0;
#endif


#if defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN8IW9P1)
#define PLL1_CLK PLL_CPU0_CLK
#define PLL2_CLK PLL_CPU1_CLK
static struct cpufreq_frequency_table sunxi_freq_table_ca7[] = {
	{0,  2016 * 1000},
	{0,  1800 * 1000},
	{0,  1608 * 1000},
	{0,  1200 * 1000},
	{0,  1128 * 1000},
	{0,  1008 * 1000},
	{0,  912  * 1000},
	{0,  864  * 1000},
	{0,  720  * 1000},
	{0,  600  * 1000},
	{0,  480  * 1000},
	{0,  CPUFREQ_TABLE_END},
};
#define sunxi_freq_table_ca15 sunxi_freq_table_ca7

/*
 * Notice:
 * The the definition of the minimum frequnecy should be a valid value
 * in the frequnecy table, otherwise, there may be some power efficiency
 * lost in the interactive governor, when the cpufreq_interactive_idle_start
 * try to check the frequency status:
 * if (pcpu->target_freq != pcpu->policy->min) {}
 * the target_freq will never equal to the policy->min !!! Then, the timer
 * will wakeup the cpu frequently
*/
#define SUNXI_CPUFREQ_L_MAX           (2016000000)            /* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_L_MIN           (480000000)             /* config the minimum frequency of sunxi core */
#define SUNXI_CPUFREQ_B_MAX           SUNXI_CPUFREQ_L_MAX     /* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_B_MIN           SUNXI_CPUFREQ_L_MIN     /* config the minimum frequency of sunxi core */

#else

static struct cpufreq_frequency_table sunxi_freq_table_ca7[] = {
	{0,  1200 * 1000},
	{0,  1104 * 1000},
	{0,  1008 * 1000},
	{0,  912  * 1000},
	{0,  816  * 1000},
	{0,  720  * 1000},
	{0,  600  * 1000},
	{0,  480  * 1000},
	{0,  CPUFREQ_TABLE_END},
};

static struct cpufreq_frequency_table sunxi_freq_table_ca15[] = {
	{0,  1800 * 1000},
	{0,  1704 * 1000},
	{0,  1608 * 1000},
	{0,  1512 * 1000},
	{0,  1416 * 1000},
	{0,  1320 * 1000},
	{0,  1200 * 1000},
	{0,  1104 * 1000},
	{0,  1008 * 1000},
	{0,  912  * 1000},
	{0,  816  * 1000},
	{0,  720  * 1000},
	{0,  600  * 1000},
	{0,  CPUFREQ_TABLE_END},
};

/*
 * Notice:
 * The the definition of the minimum frequnecy should be a valid value
 * in the frequnecy table, otherwise, there may be some power efficiency
 * lost in the interactive governor, when the cpufreq_interactive_idle_start
 * try to check the frequency status:
 * if (pcpu->target_freq != pcpu->policy->min) {}
 * the target_freq will never equal to the policy->min !!! Then, the timer
 * will wakeup the cpu frequently
*/
#define SUNXI_CPUFREQ_L_MAX           (1200000000)    /* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_L_MIN           (480000000)     /* config the minimum frequency of sunxi core */
#define SUNXI_CPUFREQ_B_MAX           (1800000000)    /* config the maximum frequency of sunxi core */
#define SUNXI_CPUFREQ_B_MIN           (600000000)     /* config the minimum frequency of sunxi core */

#endif

static unsigned int l_freq_max   = SUNXI_CPUFREQ_L_MAX / 1000;
static unsigned int l_freq_boot  = SUNXI_CPUFREQ_L_MAX / 1000;
static unsigned int l_freq_ext   = SUNXI_CPUFREQ_L_MAX / 1000;
static unsigned int l_freq_min   = SUNXI_CPUFREQ_L_MIN / 1000;
static unsigned int b_freq_max   = SUNXI_CPUFREQ_B_MAX / 1000;
static unsigned int b_freq_boot  = SUNXI_CPUFREQ_B_MAX / 1000;
static unsigned int b_freq_ext   = SUNXI_CPUFREQ_B_MAX / 1000;
static unsigned int b_freq_min   = SUNXI_CPUFREQ_B_MIN / 1000;

#ifdef CONFIG_BL_SWITCHER
bool bL_switching_enabled;
#endif

int sunxi_dvfs_debug = 0;
int sunxi_boot_freq_lock = 0;

static struct clk *clk_pll1; /* pll1 clock handler */
static struct clk *clk_pll2; /* pll2 clock handler */
static struct clk  *cluster_clk[MAX_CLUSTERS];
static unsigned int cluster_pll[MAX_CLUSTERS];
static struct cpufreq_frequency_table *freq_table[MAX_CLUSTERS + 1];
static struct regulator *cpu_vdd[MAX_CLUSTERS]; /* cpu vdd handler   */
static char cpufreq_c0_cpuvdd[32];
static char cpufreq_c1_cpuvdd[32];

static DEFINE_PER_CPU(unsigned int, physical_cluster);
static DEFINE_PER_CPU(unsigned int, cpu_last_req_freq);

static struct mutex cluster_lock[MAX_CLUSTERS];

static unsigned int find_cluster_maxfreq(int cluster)
{
    int j;
    u32 max_freq = 0, cpu_freq;

    for_each_online_cpu(j) {
        cpu_freq = per_cpu(cpu_last_req_freq, j);
        if ((cluster == per_cpu(physical_cluster, j)) && (max_freq < cpu_freq)) {
            max_freq = cpu_freq;
        }
    }

    if (unlikely(sunxi_dvfs_debug))
        CPUFREQ_DBG("%s: cluster:%d, max freq:%d\n", __func__, cluster, max_freq);

    return max_freq;
}

/*
 * get the current cpu vdd;
 * return: cpu vdd, based on mv;
 */
static int sunxi_cpufreq_getvolt(unsigned int cpu)
{
    u32 cur_cluster = per_cpu(physical_cluster, cpu);
    return regulator_get_voltage(cpu_vdd[cur_cluster]) / 1000;
}


static unsigned int sunxi_clk_get_cpu_rate(unsigned int cpu)
{
    u32 cur_cluster = per_cpu(physical_cluster, cpu), rate;
#ifdef CONFIG_DEBUG_FS
    ktime_t calltime = ktime_set(0, 0), delta, rettime;
#endif

    mutex_lock(&cluster_lock[cur_cluster]);

#ifdef CONFIG_DEBUG_FS
    calltime = ktime_get();
#endif

    if (cur_cluster == A7_CLUSTER)
        clk_get_rate(clk_pll1);
    else if (cur_cluster == A15_CLUSTER)
        clk_get_rate(clk_pll2);

    rate = clk_get_rate(cluster_clk[cur_cluster]) / 1000;

    /* For switcher we use virtual A15 clock rates */
    if (is_bL_switching_enabled()) {
        rate = VIRT_FREQ(cur_cluster, rate);
    }

#ifdef CONFIG_DEBUG_FS
    rettime = ktime_get();
    delta = ktime_sub(rettime, calltime);
    if (cur_cluster == A7_CLUSTER)
        c0_get_time_usecs = ktime_to_ns(delta) >> 10;
    else if (cur_cluster == A15_CLUSTER)
        c1_get_time_usecs = ktime_to_ns(delta) >> 10;
#endif

    if (unlikely(sunxi_dvfs_debug))
        CPUFREQ_DBG("cpu:%d, cur_cluster:%d,  cur_freq:%d\n", cpu, cur_cluster, rate);

    mutex_unlock(&cluster_lock[cur_cluster]);

    return rate;
}

#ifdef CONFIG_SUNXI_ARISC
static int clk_set_fail_notify(void *arg)
{
    CPUFREQ_ERR("%s: cluster: %d\n", __func__, (u32)arg);

    /* maybe should do others */

    return 0;
}
#endif

static int sunxi_clk_set_cpu_rate(unsigned int cluster, unsigned int cpu, unsigned int rate)
{
    int ret;
#ifdef CONFIG_DEBUG_FS
    ktime_t calltime = ktime_set(0, 0), delta, rettime;
#endif

    if (unlikely(sunxi_dvfs_debug))
        CPUFREQ_DBG("cpu:%d, cluster:%d, set freq:%u\n", cpu, cluster, rate);

#ifdef CONFIG_DEBUG_FS
    calltime = ktime_get();
#endif

#ifndef CONFIG_SUNXI_ARISC
    ret = 0;
#else
    /* the rate is base on khz */
    ret = arisc_dvfs_set_cpufreq(rate, cluster_pll[cluster],
            ARISC_MESSAGE_ATTR_SOFTSYN, clk_set_fail_notify, (void *)cluster);
#endif

#ifdef CONFIG_DEBUG_FS
    rettime = ktime_get();
    delta = ktime_sub(rettime, calltime);
    if (cluster == A7_CLUSTER)
        c0_set_time_usecs = ktime_to_ns(delta) >> 10;
    else if (cluster == A15_CLUSTER)
        c1_set_time_usecs = ktime_to_ns(delta) >> 10;
#endif

    return ret;
}

static unsigned int sunxi_cpufreq_set_rate(u32 cpu, u32 old_cluster,
                                           u32 new_cluster, u32 rate)
{
    u32 new_rate, prev_rate;
    int ret;

    mutex_lock(&cluster_lock[new_cluster]);

    prev_rate = per_cpu(cpu_last_req_freq, cpu);
    per_cpu(cpu_last_req_freq, cpu) = rate;
    per_cpu(physical_cluster, cpu) = new_cluster;

    if (is_bL_switching_enabled()) {
        new_rate = find_cluster_maxfreq(new_cluster);
        new_rate = ACTUAL_FREQ(new_cluster, new_rate);
    } else {
        new_rate = rate;
    }

    if (unlikely(sunxi_dvfs_debug))
        CPUFREQ_DBG("cpu:%d, old cluster:%d, new cluster:%d, target freq:%d\n",
                cpu, old_cluster, new_cluster, new_rate);

    ret = sunxi_clk_set_cpu_rate(new_cluster, cpu, new_rate);
    if (WARN_ON(ret)) {
        CPUFREQ_ERR("clk_set_rate failed:%d, new cluster:%d\n", ret, new_cluster);
        per_cpu(cpu_last_req_freq, cpu) = prev_rate;
        per_cpu(physical_cluster, cpu) = old_cluster;

        mutex_unlock(&cluster_lock[new_cluster]);

        return ret;
    }
    mutex_unlock(&cluster_lock[new_cluster]);

    if (is_bL_switching_enabled()) {
        /* Recalc freq for old cluster when switching clusters */
        if (old_cluster != new_cluster) {
            if (unlikely(sunxi_dvfs_debug))
                CPUFREQ_DBG("cpu:%d, switch from cluster-%d to cluster-%d\n", cpu, old_cluster, new_cluster);

            bL_switch_request(cpu, new_cluster);

            mutex_lock(&cluster_lock[old_cluster]);
            /* Set freq of old cluster if there are cpus left on it */
            new_rate = find_cluster_maxfreq(old_cluster);
            new_rate = ACTUAL_FREQ(old_cluster, new_rate);
            if (new_rate) {
                if (unlikely(sunxi_dvfs_debug))
                    CPUFREQ_DBG("Updating rate of old cluster:%d, to freq:%d\n", old_cluster, new_rate);

                if (sunxi_clk_set_cpu_rate(old_cluster, cpu, new_rate))
                    CPUFREQ_ERR("clk_set_rate failed: %d, old cluster:%d\n", ret, old_cluster);
            }
            mutex_unlock(&cluster_lock[old_cluster]);
        }
    }

    return 0;
}

/* Validate policy frequency range */
static int sunxi_cpufreq_verify_policy(struct cpufreq_policy *policy)
{
	u32 cur_cluster = cpu_to_cluster(policy->cpu);

	return cpufreq_frequency_table_verify(policy, freq_table[cur_cluster]);
}

/* Set clock frequency */
static int sunxi_cpufreq_set_target(struct cpufreq_policy *policy,
                                    unsigned int target_freq, unsigned int relation)
{
    struct cpufreq_freqs freqs;
    u32 cpu = policy->cpu, freq_tab_idx;
    u32 cur_cluster, new_cluster, actual_cluster;
    int ret = 0;
    int boot_freq = 0;
#ifdef CONFIG_SCHED_SMP_DCMP
    u32 i,other_cluster;
#endif

    freqs.old = sunxi_clk_get_cpu_rate(cpu);
    if (freqs.old == target_freq) {
        return 0;
    }

    cur_cluster = cpu_to_cluster(cpu);
    new_cluster = actual_cluster = per_cpu(physical_cluster, cpu);

    if (unlikely(sunxi_dvfs_debug))
        CPUFREQ_DBG("request frequency is %u\n", target_freq);

    if (unlikely(sunxi_boot_freq_lock)) {
        boot_freq = cur_cluster == A7_CLUSTER ? l_freq_boot : b_freq_boot;
        target_freq = target_freq > boot_freq ? boot_freq : target_freq;
    }

    /* Determine valid target frequency using freq_table */
    ret = cpufreq_frequency_table_target(policy, freq_table[cur_cluster],
        target_freq, relation, &freq_tab_idx);
    if (ret) {
        CPUFREQ_ERR("try to look for a valid frequency for %u failed!\n", target_freq);
        return ret;
    }

    freqs.new = freq_table[cur_cluster][freq_tab_idx].frequency;
    if (freqs.old == freqs.new) {
        return 0;
    }

    if (unlikely(sunxi_dvfs_debug))
        CPUFREQ_DBG("target frequency find is %u, entry %u\n", freqs.new, freq_tab_idx);

    if (is_bL_switching_enabled()) {
        if ((actual_cluster == A15_CLUSTER) &&
            (freqs.new < SUNXI_BL_SWITCH_THRESHOLD)) {
            new_cluster = A7_CLUSTER;
        } else if ((actual_cluster == A7_CLUSTER) &&
            (freqs.new > SUNXI_BL_SWITCH_THRESHOLD)) {
            new_cluster = A15_CLUSTER;
        }
    }

    cpufreq_notify_transition(policy, &freqs, CPUFREQ_PRECHANGE);

    ret = sunxi_cpufreq_set_rate(cpu, actual_cluster, new_cluster, freqs.new);
#ifdef CONFIG_SCHED_SMP_DCMP
    for_each_online_cpu(i)
    {
        other_cluster = cpu_to_cluster(i);
        if(other_cluster != actual_cluster)
        {
            ret = sunxi_cpufreq_set_rate(i, other_cluster, other_cluster, freqs.new);
            break;
        }
    }
#endif

    if (ret) {
        return ret;
    }

    policy->cur = freqs.new;

    cpufreq_notify_transition(policy, &freqs, CPUFREQ_POSTCHANGE);

    if (unlikely(sunxi_dvfs_debug))
        CPUFREQ_DBG("DVFS done! Freq[%uMHz] Volt[%umv] ok\n\n",
            sunxi_clk_get_cpu_rate(cpu) / 1000, sunxi_cpufreq_getvolt(cpu));

    return ret;
}

static inline u32 get_table_count(struct cpufreq_frequency_table *table)
{
	int count;

	for (count = 0; table[count].frequency != CPUFREQ_TABLE_END; count++) {
		;
	}

	return count;
}


static int merge_cluster_tables(void)
{
    int i, j, k = 0, count = 1;
    struct cpufreq_frequency_table *table;

    for (i = 0; i < MAX_CLUSTERS; i++) {
        count += get_table_count(freq_table[i]);
    }

    table = kzalloc(sizeof(*table) * count, GFP_KERNEL);
    if (!table) {
        return -ENOMEM;
    }
    freq_table[MAX_CLUSTERS] = table;

    /* Add in reverse order to get freqs in increasing order */
    for (i = MAX_CLUSTERS - 1; i >= 0; i--) {
        for (j = 0; freq_table[i][j].frequency != CPUFREQ_TABLE_END; j++) {
            table[k].frequency = VIRT_FREQ(i, freq_table[i][j].frequency);
            CPUFREQ_DBG("%s: index: %d, freq: %d\n", __func__, k, table[k].frequency);
            k++;
        }
    }

    table[k].index = k;
    table[k].frequency = CPUFREQ_TABLE_END;

    CPUFREQ_DBG("%s: End, table: %p, count: %d\n", __func__, table, k);

    return 0;
}

/*
 * init cpu max/min frequency from sysconfig;
 * return: 0 - init cpu max/min successed, !0 - init cpu max/min failed;
 */
static int __init_freq_syscfg(char *tbl_name)
{
    int ret = 0;
    script_item_u l_max, l_min, b_max, b_min, l_boot, l_ext, b_boot, b_ext;
    script_item_value_type_e type;

    type = script_get_item(tbl_name, "L_max_freq", &l_max);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        CPUFREQ_ERR("get cpu l_max frequency from sysconfig failed\n");
        ret = -1;
        goto fail;
    }
    l_freq_max = l_max.val;

    type = script_get_item(tbl_name, "L_min_freq", &l_min);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        CPUFREQ_ERR("get cpu l_min frequency from sysconfig failed\n");
        ret = -1;
        goto fail;
    }
    l_freq_min = l_min.val;

    type = script_get_item(tbl_name, "L_extremity_freq", &l_ext);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        l_ext.val = l_freq_max;
    }
    l_freq_ext = l_ext.val;

    type = script_get_item(tbl_name, "L_boot_freq", &l_boot);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        l_boot.val = l_freq_max;
    } else {
        sunxi_boot_freq_lock = 1;
    }
    l_freq_boot = l_boot.val;

    type = script_get_item(tbl_name, "B_max_freq", &b_max);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        CPUFREQ_ERR("get cpu b_max frequency from sysconfig failed\n");
        ret = -1;
        goto fail;
    }
    b_freq_max = b_max.val;

    type = script_get_item(tbl_name, "B_min_freq", &b_min);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        CPUFREQ_ERR("get cpu b_min frequency from sysconfig failed\n");
        ret = -1;
        goto fail;
    }
    b_freq_min = b_min.val;

    type = script_get_item(tbl_name, "B_extremity_freq", &b_ext);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        b_ext.val = b_freq_max;
    }
    b_freq_ext = b_ext.val;

    type = script_get_item(tbl_name, "B_boot_freq", &b_boot);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        b_boot.val = b_freq_max;
    } else {
        sunxi_boot_freq_lock = 1;
    }
    b_freq_boot = b_boot.val;

    if (l_freq_max > SUNXI_CPUFREQ_L_MAX || l_freq_max < SUNXI_CPUFREQ_L_MIN
        || l_freq_min < SUNXI_CPUFREQ_L_MIN || l_freq_min > SUNXI_CPUFREQ_L_MAX) {
        CPUFREQ_ERR("l_cpu max or min frequency from sysconfig is more than range\n");
        ret = -1;
        goto fail;
    }

    if (l_freq_min > l_freq_max) {
        CPUFREQ_ERR("l_cpu min frequency can not be more than l_cpu max frequency\n");
        ret = -1;
        goto fail;
    }

    if (l_freq_ext < l_freq_max) {
        CPUFREQ_ERR("l_cpu ext frequency can not be less than l_cpu max frequency\n");
        ret = -1;
        goto fail;
    }

    if (l_freq_boot > l_freq_max || l_freq_boot < l_freq_min) {
        CPUFREQ_ERR("l_cpu boot frequency invalid\n");
        ret = -1;
        goto fail;
    }

    if (b_freq_max > SUNXI_CPUFREQ_B_MAX || b_freq_max < SUNXI_CPUFREQ_B_MIN
        || b_freq_min < SUNXI_CPUFREQ_B_MIN || b_freq_min > SUNXI_CPUFREQ_B_MAX) {
        CPUFREQ_ERR("b_cpu max or min frequency from sysconfig is more than range\n");
        ret = -1;
        goto fail;
    }

    if (b_freq_min > b_freq_max) {
        CPUFREQ_ERR("b_cpu min frequency can not be more than b_cpu max frequency\n");
        ret = -1;
        goto fail;
    }

    if (b_freq_ext < b_freq_max) {
        CPUFREQ_ERR("b_cpu ext frequency can not be less than b_cpu max frequency\n");
        ret = -1;
        goto fail;
    }

    if (b_freq_boot > b_freq_max || b_freq_boot < b_freq_min) {
        CPUFREQ_ERR("b_cpu boot frequency invalid\n");
        ret = -1;
        goto fail;
    }

    l_freq_max   /= 1000;
    l_freq_min   /= 1000;
    l_freq_ext   /= 1000;
    l_freq_boot  /= 1000;
    b_freq_max   /= 1000;
    b_freq_min   /= 1000;
    b_freq_ext   /= 1000;
    b_freq_boot  /= 1000;

    return 0;

fail:
    /* use default cpu max/min frequency */
    l_freq_max   = SUNXI_CPUFREQ_L_MAX / 1000;
    l_freq_min   = SUNXI_CPUFREQ_L_MIN / 1000;
    l_freq_ext   = l_freq_max;
    l_freq_boot  = l_freq_max;
    b_freq_max   = SUNXI_CPUFREQ_B_MAX / 1000;
    b_freq_min   = SUNXI_CPUFREQ_B_MIN / 1000;
    b_freq_ext   = b_freq_max;
    b_freq_boot  = b_freq_max;

    return ret;
}

static int sunxi_cpufreq_cpu_init(struct cpufreq_policy *policy)
{
    u32 cur_cluster = cpu_to_cluster(policy->cpu);
    u32 cpu;

    /* set cpu freq-table */
    if (is_bL_switching_enabled()) {
        /* bL switcher use the merged freq table */
        cpufreq_frequency_table_get_attr(freq_table[MAX_CLUSTERS], policy->cpu);
    } else {
        /* HMP use the per-cluster freq table */
        cpufreq_frequency_table_get_attr(freq_table[cur_cluster], policy->cpu);
    }

    /* set the target cluster of cpu */
    if (cur_cluster < MAX_CLUSTERS) {
        /* set cpu masks */
#ifdef CONFIG_SCHED_SMP_DCMP
        cpumask_copy(policy->cpus, cpu_possible_mask);
#else
        cpumask_copy(policy->cpus, topology_core_cpumask(policy->cpu));
#endif
        /* set core sibling */
        for_each_cpu(cpu, topology_core_cpumask(policy->cpu)) {
            per_cpu(physical_cluster, cpu) = cur_cluster;
        }
    } else {
        /* Assumption: during init, we are always running on A7 */
        per_cpu(physical_cluster, policy->cpu) = A7_CLUSTER;
    }
    /* initialize current cpu freq */
    policy->cur = sunxi_clk_get_cpu_rate(policy->cpu);
    per_cpu(cpu_last_req_freq, policy->cpu) = policy->cur;

    if (unlikely(sunxi_dvfs_debug))
        CPUFREQ_DBG("cpu:%d, cluster:%d, init freq:%d\n", policy->cpu, cur_cluster, policy->cur);

    /* set the policy min and max freq */
    if (is_bL_switching_enabled()) {
        /* bL switcher use the merged freq table */
        cpufreq_frequency_table_cpuinfo(policy, freq_table[MAX_CLUSTERS]);
    } else {
        /* HMP use the per-cluster freq table */
        if (cur_cluster == A7_CLUSTER) {
            policy->min = policy->cpuinfo.min_freq = l_freq_min;
            policy->cpuinfo.max_freq = l_freq_ext;
            policy->max = l_freq_max;
            policy->cpuinfo.boot_freq  = l_freq_boot;
        } else if (cur_cluster == A15_CLUSTER) {
            policy->min = policy->cpuinfo.min_freq = b_freq_min;
            policy->cpuinfo.max_freq = b_freq_ext;
            policy->max = b_freq_max;
            policy->cpuinfo.boot_freq  = b_freq_boot;
        }
    }

    /* set the transition latency value */
    policy->cpuinfo.transition_latency = SUNXI_FREQTRANS_LATENCY;

    return 0;
}

/* Export freq_table to sysfs */
static struct freq_attr *sunxi_cpufreq_attr[] = {
	&cpufreq_freq_attr_scaling_available_freqs,
	NULL,
};

static struct cpufreq_driver sunxi_cpufreq_driver = {
	.name			= "sunxi-iks",
	.flags			= CPUFREQ_STICKY,
	.verify			= sunxi_cpufreq_verify_policy,
	.target			= sunxi_cpufreq_set_target,
	.get			= sunxi_clk_get_cpu_rate,
	.init			= sunxi_cpufreq_cpu_init,
    .have_governor_per_policy = true,
	.attr			= sunxi_cpufreq_attr,
};

static int sunxi_cpufreq_switcher_notifier(struct notifier_block *nfb,
                                           unsigned long action, void *_arg)
{
	pr_info("%s: action:%lu\n", __func__, action);

	switch (action) {
	case BL_NOTIFY_PRE_ENABLE:
	case BL_NOTIFY_PRE_DISABLE:
		cpufreq_unregister_driver(&sunxi_cpufreq_driver);
		break;

	case BL_NOTIFY_POST_ENABLE:
		set_switching_enabled(true);
		cpufreq_register_driver(&sunxi_cpufreq_driver);
		break;

	case BL_NOTIFY_POST_DISABLE:
		set_switching_enabled(false);
		cpufreq_register_driver(&sunxi_cpufreq_driver);
		break;

	default:
		return NOTIFY_DONE;
	}

	return NOTIFY_OK;
}

static struct notifier_block sunxi_switcher_notifier = {
	.notifier_call = sunxi_cpufreq_switcher_notifier,
};

static void sunxi_get_cluster_power(void)
{
    script_item_value_type_e type;
    script_item_u item_temp;

	memset(cpufreq_c0_cpuvdd, 0, 32);
	memset(cpufreq_c1_cpuvdd, 0, 32);

	printk("%s:%d: \n", __func__, __LINE__);

    type = script_get_item("cluster_para", "cluster0_power", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        strcpy(cpufreq_c0_cpuvdd, item_temp.str);
        CPUFREQ_DBG("cpufreq_c0_cpuvdd: %s\n", cpufreq_c0_cpuvdd);
    }else{
        strcpy(cpufreq_c0_cpuvdd, SUNXI_CPUFREQ_C0_CPUVDD);
		CPUFREQ_ERR("Can not get cluster0_power from sys_config, use default[%s] for cluster0\n", cpufreq_c0_cpuvdd);
	}

    type = script_get_item("cluster_para", "cluster1_power", &item_temp);
    if(type == SCIRPT_ITEM_VALUE_TYPE_STR){
        strcpy(cpufreq_c1_cpuvdd, item_temp.str);
        CPUFREQ_DBG("cpufreq_c1_cpuvdd: %s\n", cpufreq_c1_cpuvdd);
    }else{
        strcpy(cpufreq_c1_cpuvdd, SUNXI_CPUFREQ_C1_CPUVDD);
		CPUFREQ_ERR("Can not get cluster0_power from sys_config, use default[%s] for cluster1\n", cpufreq_c1_cpuvdd);
	}

	return;
}

static int  __init sunxi_cpufreq_init(void)
{
    int ret, i;
    char vftbl_name[16] = {0};
    script_item_u vf_table_count;
    script_item_value_type_e type;
    unsigned int vf_table_type = 0;

    ret = bL_switcher_get_enabled();
    set_switching_enabled(ret);

    /* get pll1 pll2 clock handle */
    clk_pll1 = clk_get(NULL, PLL1_CLK);
    clk_pll2 = clk_get(NULL, PLL2_CLK);

    /* get cluster clock handle */
    cluster_clk[A7_CLUSTER]  = clk_get(NULL, CLUSTER0_CLK);
    cluster_clk[A15_CLUSTER] = clk_get(NULL, CLUSTER1_CLK);

    /* initialize cpufreq table and merge ca7/ca15 table */
    freq_table[A7_CLUSTER] = sunxi_freq_table_ca7;
    freq_table[A15_CLUSTER] = sunxi_freq_table_ca15;
    merge_cluster_tables();

    /* initialize ca7 and ca15 cluster pll number */
    cluster_pll[A7_CLUSTER] = ARISC_DVFS_PLL1;
    cluster_pll[A15_CLUSTER] = ARISC_DVFS_PLL2;

	sunxi_get_cluster_power();

    cpu_vdd[A7_CLUSTER] = regulator_get(NULL, cpufreq_c0_cpuvdd);
    if (IS_ERR(cpu_vdd[A7_CLUSTER])) {
        CPUFREQ_ERR("%s: could not get Cluster0 cpu vdd\n", __func__);
        return -ENOENT;
    }

    cpu_vdd[A15_CLUSTER] = regulator_get(NULL, cpufreq_c1_cpuvdd);
    if (IS_ERR(cpu_vdd[A15_CLUSTER])) {
        regulator_put(cpu_vdd[A7_CLUSTER]);
        CPUFREQ_ERR("%s: could not get Cluster1 cpu vdd\n", __func__);
        return -ENOENT;
    }

    type = script_get_item("dvfs_table", "vf_table_count", &vf_table_count);
    if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
        CPUFREQ_DBG("%s: support only one vf_table\n", __func__);
        sprintf(vftbl_name, "%s", "dvfs_table");
    } else {
        vf_table_type = sunxi_get_soc_bin();
        sprintf(vftbl_name, "%s%d", "vf_table", vf_table_type);
    }

    /* init cpu frequency from sysconfig */
    if(__init_freq_syscfg(vftbl_name)) {
        CPUFREQ_ERR("%s, use default cpu max/min frequency, l_max: %uMHz, l_min: %uMHz, b_max: %uMHz, b_min: %uMHz\n",
                __func__, l_freq_max/1000, l_freq_min/1000, b_freq_max/1000, b_freq_min/1000);
    }else{
        CPUFREQ_DBG("%s, get cpu frequency from sysconfig, l_max: %uMHz, l_min: %uMHz, b_max: %uMHz, b_min: %uMHz\n",
                __func__, l_freq_max/1000, l_freq_min/1000, b_freq_max/1000, b_freq_min/1000);
    }

    for (i = 0; i < MAX_CLUSTERS; i++) {
        mutex_init(&cluster_lock[i]);
    }

    ret = cpufreq_register_driver(&sunxi_cpufreq_driver);
    if (ret) {
        CPUFREQ_ERR("sunxi register cpufreq driver fail, err: %d\n", ret);
    } else {
        CPUFREQ_DBG("sunxi register cpufreq driver succeed\n");
        ret = bL_switcher_register_notifier(&sunxi_switcher_notifier);
        if (ret) {
            CPUFREQ_ERR("sunxi register bL notifier fail, err: %d\n", ret);
            cpufreq_unregister_driver(&sunxi_cpufreq_driver);
        } else {
            CPUFREQ_DBG("sunxi register bL notifier succeed\n");
        }
    }

    bL_switcher_put_enabled();

    pr_debug("%s: done!\n", __func__);
    return ret;
}
module_init(sunxi_cpufreq_init);

static void __exit sunxi_cpufreq_exit(void)
{
    int i;

    for (i = 0; i < MAX_CLUSTERS; i++)
        mutex_destroy(&cluster_lock[i]);

    clk_put(clk_pll1);
    clk_put(clk_pll2);
    clk_put(cluster_clk[A7_CLUSTER]);
    clk_put(cluster_clk[A15_CLUSTER]);
    regulator_put(cpu_vdd[A7_CLUSTER]);
    regulator_put(cpu_vdd[A15_CLUSTER]);
    bL_switcher_get_enabled();
    bL_switcher_unregister_notifier(&sunxi_switcher_notifier);
    cpufreq_unregister_driver(&sunxi_cpufreq_driver);
    bL_switcher_put_enabled();
    CPUFREQ_DBG("%s: done!\n", __func__);
}
module_exit(sunxi_cpufreq_exit);

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_cpufreq_root;

static int get_c0_time_show(struct seq_file *s, void *data)
{
    seq_printf(s, "%Ld\n", c0_get_time_usecs);
    return 0;
}

static int get_c0_time_open(struct inode *inode, struct file *file)
{
    return single_open(file, get_c0_time_show, inode->i_private);
}

static const struct file_operations get_c0_time_fops = {
    .open = get_c0_time_open,
    .read = seq_read,
};

static int set_c0_time_show(struct seq_file *s, void *data)
{
    seq_printf(s, "%Ld\n", c0_set_time_usecs);
    return 0;
}

static int set_c0_time_open(struct inode *inode, struct file *file)
{
    return single_open(file, set_c0_time_show, inode->i_private);
}

static const struct file_operations set_c0_time_fops = {
    .open = set_c0_time_open,
    .read = seq_read,
};

static int get_c1_time_show(struct seq_file *s, void *data)
{
    seq_printf(s, "%Ld\n", c1_get_time_usecs);
    return 0;
}

static int get_c1_time_open(struct inode *inode, struct file *file)
{
    return single_open(file, get_c1_time_show, inode->i_private);
}

static const struct file_operations get_c1_time_fops = {
    .open = get_c1_time_open,
    .read = seq_read,
};

static int set_c1_time_show(struct seq_file *s, void *data)
{
    seq_printf(s, "%Ld\n", c1_set_time_usecs);
    return 0;
}

static int set_c1_time_open(struct inode *inode, struct file *file)
{
    return single_open(file, set_c1_time_show, inode->i_private);
}

static const struct file_operations set_c1_time_fops = {
    .open = set_c1_time_open,
    .read = seq_read,
};

static int __init debug_init(void)
{
    int err = 0;

    debugfs_cpufreq_root = debugfs_create_dir("cpufreq", 0);
    if (!debugfs_cpufreq_root)
        return -ENOMEM;

    if (!debugfs_create_file("c0_get_time", 0444, debugfs_cpufreq_root, NULL, &get_c0_time_fops)) {
        err = -ENOMEM;
        goto out;
    }

    if (!debugfs_create_file("c0_set_time", 0444, debugfs_cpufreq_root, NULL, &set_c0_time_fops)) {
        err = -ENOMEM;
        goto out;
    }

    if (!debugfs_create_file("c1_get_time", 0444, debugfs_cpufreq_root, NULL, &get_c1_time_fops)) {
        err = -ENOMEM;
        goto out;
    }

    if (!debugfs_create_file("c1_set_time", 0444, debugfs_cpufreq_root, NULL, &set_c1_time_fops)) {
        err = -ENOMEM;
        goto out;
    }

    return 0;

out:
    debugfs_remove_recursive(debugfs_cpufreq_root);
    return err;
}

static void __exit debug_exit(void)
{
    debugfs_remove_recursive(debugfs_cpufreq_root);
}

late_initcall(debug_init);
module_exit(debug_exit);
#endif /* CONFIG_DEBUG_FS */

MODULE_AUTHOR("sunny <sunny@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi iks cpufreq driver");
MODULE_LICENSE("GPL");

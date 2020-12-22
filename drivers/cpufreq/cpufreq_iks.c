/*
 * drivers/cpufreq/cpufreq_iks.c
 *
 * Copyright (C) 2013 Allwinnertech, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author: Kevin.z.m (kevin@allwinnertech.com)
 *
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/rwsem.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/kernel_stat.h>
#include <asm/cputime.h>
#include <linux/input.h>

struct cpu_dbs_info_s {
    int cpu;                    /* current cpu number           */
    u64 prev_cpu_idle;          /* previous idle time statistic */
    u64 prev_cpu_wall;          /* previous total time stat     */
    struct cpufreq_policy *cur_policy;
    struct cpufreq_frequency_table *freq_table;
	struct delayed_work work;   /* work queue for check loading */
    struct mutex timer_mutex;   /* semaphore for protection     */
    unsigned int load;          /* load of this cpu (%)         */
    unsigned int load_freq;     /* frequency for system needed  */
    unsigned int target_freq;   /* frequency for setting to hw  */
    unsigned int cur_freq;      /* frequency of current cpu     */
    unsigned int max_freq;      /* max frequency of current cpu */
    bool is_little;             /* flag to mark if current cpu is little or big */
};


#define DEF_SAMPLING_RATE               (50000) /* check cpu frequency sample rate is 50ms */
#define DEF_IOWAIT_IS_BUSY              (1)     /* iowait is think as cpu busy in default  */
#define DEF_DVFS_DEBUG                  (1)
#define DEF_LITTLE_MAX_FREQ             (500000)
#define DEF_BIG_MIN_FREQ                (600000)
#define DEF_LITTLE_TRY_UP_THRESHOLD     (80)
#define DEF_BIG_2_LITTLE_THRESHOLD      (60)
#define DEF_LITTLE_2_BIG_THRESHOLD      (95)
#define DEF_CORE_UP_THRESHOLD           (80)    /* increase cpu frequency, if the loading is higher */
#define DEF_CORE_DOWN_THRESHOLD         (30)
#define DEF_LITTLE_2_BIG_TARGET         (800000)
#define DEF_BIG_2_LITTLE_TARGET         (300000)
#define DEF_INPUT_PULSE                 (80)    /* pulse cpu upper 80% than max frequency of little */

static struct dbs_tuners {
    unsigned int sampling_rate;     /* dvfs sample rate                             */
    unsigned int io_is_busy;        /* flag to mark if iowait calculate as cpu busy */
    unsigned int dvfs_debug;        /* dvfs debug flag, print dbs information       */

    unsigned int little_max_freq;   /* max frequency of the little cpu  */
    unsigned int big_min_freq;      /* min frequency of the max cpu     */
    unsigned int little_try_up_threshold;
                                    /* if loading is higher, need check if core up or switch to big */
    unsigned int little_2_big_threshold;
                                    /* if loading is higher, switch to big directly */
    unsigned int big_2_little_threshold;
                                    /* if loading is lower, switch to little directly */
    unsigned int core_up_threshold; /* if all cpus's loading is higer, try to up core */
    unsigned int core_down_threshold;
                                    /* if more than 2 cores' loading is lower, try to down core */
    unsigned int little_2_big_target;
                                    /* taget frequency of a core, when it switch from little to big */
    unsigned int big_2_little_target;
                                    /* taget frequency of a core, when it switch from little to big */
    unsigned int boost;             /* boost mode, cpu lock to the max frequency, but not lock core */
    unsigned int pulse;             /* boost cpu to the pulse_ratio * maxfreq a pulse_duration period */
    unsigned int pulse_duration;    /* pulse period */
    unsigned int pulse_ratio;       /* pulse frequency ratio */
    unsigned int max_power;         /* max power mode, lock all cores to the max frequency  */
} dbs_tuners_ins = {
    .sampling_rate = DEF_SAMPLING_RATE,
    .io_is_busy = DEF_IOWAIT_IS_BUSY,
    .dvfs_debug = DEF_DVFS_DEBUG,
    .little_max_freq = DEF_LITTLE_MAX_FREQ,
    .big_min_freq = DEF_BIG_MIN_FREQ,
    .little_try_up_threshold = DEF_LITTLE_TRY_UP_THRESHOLD,
    .little_2_big_threshold = DEF_LITTLE_2_BIG_THRESHOLD,
    .big_2_little_threshold = DEF_BIG_2_LITTLE_THRESHOLD,
    .core_up_threshold = DEF_CORE_UP_THRESHOLD,
    .core_down_threshold = DEF_CORE_DOWN_THRESHOLD,
    .little_2_big_target = DEF_LITTLE_2_BIG_TARGET,
    .big_2_little_target = DEF_BIG_2_LITTLE_TARGET,
    .boost = 0,
    .pulse = 0,
    .pulse_ratio = 80,
    .pulse_duration = 50,
    .max_power = 0,
};


static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);
static struct task_struct *power_change_task;
static DEFINE_MUTEX(dbs_mutex);
static unsigned int dbs_enable;	/* number of CPUs using this policy */
static struct cpumask core_down_mask;
static struct cpumask core_up_mask;
static struct cpumask speed_change_mask;
static spinlock_t cpumask_lock;
static unsigned long long  pulse_start;

#define EVENT_INPUT_DEVICE      (1<<0)
#define EVENT_USER_BOOST        (1<<1)
#define EVENT_USER_PULSE        (1<<2)
#define EVENT_MAX_POWER         (1<<3)
static unsigned long cpufreq_event_mask;


/* cpufreq_pegasusq Governor Tunables */
#define show_one(file_name, object)                    \
static ssize_t show_##file_name                        \
(struct kobject *kobj, struct attribute *attr, char *buf)        \
{                                    \
    return sprintf(buf, "%u\n", dbs_tuners_ins.object);        \
}

show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(dvfs_debug, dvfs_debug);
show_one(little_max_freq, little_max_freq);
show_one(big_min_freq, big_min_freq);
show_one(little_try_up_threshold, little_try_up_threshold);
show_one(little_2_big_threshold, little_2_big_threshold);
show_one(big_2_little_threshold, big_2_little_threshold);
show_one(core_up_threshold, core_up_threshold);
show_one(core_down_threshold, core_down_threshold);
show_one(little_2_big_target, little_2_big_target);
show_one(big_2_little_target, big_2_little_target);
show_one(boost, boost);
show_one(pulse, pulse);
show_one(pulse_ratio, pulse_ratio);
show_one(pulse_duration, pulse_duration);
show_one(max_power, max_power);

static ssize_t store_sampling_rate(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.sampling_rate = input;
    return count;
}
static ssize_t store_io_is_busy(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.io_is_busy = !!input;
    return count;
}
static ssize_t store_dvfs_debug(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.dvfs_debug = !!input;
    return count;
}
static ssize_t store_little_max_freq(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input > dbs_tuners_ins.big_min_freq))
        return -EINVAL;
    dbs_tuners_ins.little_max_freq = input;
    return count;
}
static ssize_t store_big_min_freq(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input < dbs_tuners_ins.little_max_freq))
        return -EINVAL;
    dbs_tuners_ins.big_min_freq = input;
    return count;
}
static ssize_t store_little_try_up_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input < 30) || (input > 100))
        return -EINVAL;
    dbs_tuners_ins.little_try_up_threshold = input;
    return count;
}
static ssize_t store_little_2_big_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input < 50) || (input > 100))
        return -EINVAL;
    dbs_tuners_ins.little_2_big_threshold = input;
    return count;
}
static ssize_t store_big_2_little_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input > 100))
        return -EINVAL;
    dbs_tuners_ins.big_2_little_threshold = input;
    return count;
}
static ssize_t store_core_up_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input < 30) || (input > 100))
        return -EINVAL;
    dbs_tuners_ins.core_up_threshold = input;
    return count;
}
static ssize_t store_core_down_threshold(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input < 50))
        return -EINVAL;
    dbs_tuners_ins.core_down_threshold = input;
    return count;
}
static ssize_t store_little_2_big_target(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input < dbs_tuners_ins.big_min_freq))
        return -EINVAL;
    dbs_tuners_ins.little_2_big_target = input;
    return count;
}
static ssize_t store_big_2_little_target(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input > dbs_tuners_ins.little_max_freq))
        return -EINVAL;
    dbs_tuners_ins.big_2_little_target = input;
    return count;
}

static ssize_t store_boost(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    unsigned long flags;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.boost = !!input;

    spin_lock_irqsave(&cpumask_lock, flags);
    cpufreq_event_mask |= EVENT_USER_BOOST;
    spin_unlock_irqrestore(&cpumask_lock, flags);

    wake_up_process(power_change_task);
    return count;
}
static ssize_t store_pulse(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    unsigned long flags;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    if(input) {
        dbs_tuners_ins.pulse = 0;
        spin_lock_irqsave(&cpumask_lock, flags);
        cpufreq_event_mask |= EVENT_USER_PULSE;
        spin_unlock_irqrestore(&cpumask_lock, flags);
        wake_up_process(power_change_task);
    }
    return count;
}
static ssize_t store_pulse_ratio(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input < 10) || (input > 100))
        return -EINVAL;
    dbs_tuners_ins.pulse_ratio = input;
    return count;
}
static ssize_t store_pulse_duration(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if ((ret != 1) || (input < 50) || (input > 5000))
        return -EINVAL;
    dbs_tuners_ins.pulse_duration = input;
    return count;
}
static ssize_t store_max_power(struct kobject *a, struct attribute *b, const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    unsigned long flags;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.max_power = !!input;
    spin_lock_irqsave(&cpumask_lock, flags);
    cpufreq_event_mask |= EVENT_MAX_POWER;
    spin_unlock_irqrestore(&cpumask_lock, flags);

    wake_up_process(power_change_task);
    return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(dvfs_debug);
define_one_global_rw(little_max_freq);
define_one_global_rw(big_min_freq);
define_one_global_rw(little_try_up_threshold);
define_one_global_rw(little_2_big_threshold);
define_one_global_rw(big_2_little_threshold);
define_one_global_rw(core_up_threshold);
define_one_global_rw(core_down_threshold);
define_one_global_rw(little_2_big_target);
define_one_global_rw(big_2_little_target);
define_one_global_rw(boost);
define_one_global_rw(pulse);
define_one_global_rw(pulse_ratio);
define_one_global_rw(pulse_duration);
define_one_global_rw(max_power);

static struct attribute *dbs_attributes[] = {
    &sampling_rate.attr,
    &io_is_busy.attr,
    &dvfs_debug.attr,
    &little_max_freq.attr,
    &big_min_freq.attr,
    &little_try_up_threshold.attr,
    &little_2_big_threshold.attr,
    &big_2_little_threshold.attr,
    &core_up_threshold.attr,
    &core_down_threshold.attr,
    &little_2_big_target.attr,
    &big_2_little_target.attr,
    &boost.attr,
    &pulse.attr,
    &pulse_ratio.attr,
    &pulse_duration.attr,
    &max_power.attr,
    NULL
};

static struct attribute_group dbs_attr_group = {
    .attrs = dbs_attributes,
    .name = "iks",
};

unsigned int __cal_little_freq(int curfreq, int load, int maxfreq)
{
    unsigned int target;

    /* if current loading is higher than 90%, up frequency */
    if(load > 90) {
        if(curfreq < 100000)
            target = min(200000, maxfreq);
        else if(curfreq < 200000)
            target = min(300000, maxfreq);
        else if(curfreq < 300000)
            target = min(400000, maxfreq);
        else if(curfreq < 400000)
            target = min(500000, maxfreq);
        else
            target = maxfreq;
    } else if(load > 60){
        /* if current loading is 60%~90%, keep it */
        target = curfreq;
    } else {
        /* if current loading is lower than 60%, scale down */
        target = min(curfreq * load / 70, maxfreq);
    }

    if(dbs_tuners_ins.dvfs_debug) {
        printk("%s :cur:%d, load:%d, target:%d\n", __func__, curfreq, load, target);
    }

    return target;
}

unsigned int __cal_big_freq(int curfreq, int load, int maxfreq)
{
    unsigned int target;

    if(load > 90) {
        if(curfreq < 800000)
            target = min(1000000, maxfreq);
        else if(curfreq < 1000000)
            target = min(1200000, maxfreq);
        else if(curfreq < 1200000)
            target = min(1600000, maxfreq);
        else
            target = maxfreq;
    } else if(load > 60){
        target = curfreq;
    } else {
        target = min(curfreq * load / 70, maxfreq);
    }

    if(dbs_tuners_ins.dvfs_debug) {
        printk("%s :cur:%d, load:%d, target:%d\n", __func__, curfreq, load, target);
    }

    return target;
}

static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
	struct cpufreq_policy *policy;
	unsigned int cpu, loading, min_loading = 100;
    struct cpu_dbs_info_s *j_dbs_info;
    int little_2_big = -1, big_2_little = -1, request_up = 0, request_down = 0;
    int heavy_load_core_cnt = 0, light_load_core_cnt = 0, light_load_core = -1;
    unsigned long flags;

	policy = this_dbs_info->cur_policy;

    /* step 1: calculate per cpu loading */
    for_each_cpu(cpu, policy->cpus) {
        u64 cur_wall_time, cur_idle_time;
        u64 prev_wall_time, prev_idle_time;
        unsigned int idle_time, wall_time;

        j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

        prev_wall_time = j_dbs_info->prev_cpu_wall;
        prev_idle_time = j_dbs_info->prev_cpu_idle;

        cur_idle_time = get_cpu_idle_time(cpu, &cur_wall_time, dbs_tuners_ins.io_is_busy);

        wall_time = cur_wall_time - prev_wall_time;
        j_dbs_info->prev_cpu_wall = cur_wall_time;
        idle_time = cur_idle_time - prev_idle_time;
        j_dbs_info->prev_cpu_idle = cur_idle_time;

        if (wall_time && (wall_time > idle_time))
            j_dbs_info->load = 100 * (wall_time - idle_time) / wall_time;
        else
            j_dbs_info->load = 0;

        j_dbs_info->load_freq = j_dbs_info->cur_freq * j_dbs_info->load;

        loading = j_dbs_info->load_freq / j_dbs_info->max_freq;
        if(loading > dbs_tuners_ins.core_up_threshold)
            heavy_load_core_cnt ++;
        else if(j_dbs_info->is_little && loading < dbs_tuners_ins.core_down_threshold) {
            light_load_core_cnt ++;
            if((loading < min_loading) && (cpu != 0)) {
                min_loading = loading;
                light_load_core = cpu;
            }
        }
        if(dbs_tuners_ins.dvfs_debug) {
            printk("%s step1: cpu(%d:%s) load:%d curfreq:%d maxfreq:%d loading:%d "
                   "(wall:%u,idle:%u)\n", __func__, cpu, j_dbs_info->is_little?
                   "little":"big", j_dbs_info->load, j_dbs_info->cur_freq,
                    j_dbs_info->max_freq, loading, wall_time, idle_time);
        }
    }

    /* skip frequency and cores check if current is max power mode */
    if(dbs_tuners_ins.max_power) {
        goto __skip_core_check;
    }

    if(dbs_tuners_ins.boost) {
        for_each_cpu(cpu, policy->cpus) {
            j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
            j_dbs_info->target_freq = cpufreq_quick_get_max(0);
        }
        goto __skip_target_cal;
    }

    /* step 2: calculate per cpu target frequency */
    for_each_cpu(cpu, policy->cpus) {
        j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

        if(this_dbs_info->is_little) {
            /* check if little cpu need plug in or switch to big */
            if(j_dbs_info->load_freq >= dbs_tuners_ins.little_max_freq *
                dbs_tuners_ins.little_2_big_threshold) {
                /* loading of little cpu is too high, switch to big directly */
                little_2_big = cpu;
            } else if(j_dbs_info->load_freq >= dbs_tuners_ins.little_max_freq *
                dbs_tuners_ins.little_try_up_threshold) {
                /* loading of little cpu is very high,
                 * check if need core up or switch to big
                 */
                int     i, switch_flag = 0;
                struct cpu_dbs_info_s *i_dbs_info;

                /* check if another little cpu's loading is too low,
                 * if so, we think the load can't be balance, try to
                 * swtich to a big cpu
                 */
                for_each_cpu(i, policy->cpus) {
                    if(i == cpu)
                        continue;
                    i_dbs_info = &per_cpu(od_cpu_dbs_info, i);
                    if(i_dbs_info->is_little && (i_dbs_info->load_freq <
                        dbs_tuners_ins.little_max_freq * dbs_tuners_ins.core_down_threshold)) {
                        little_2_big = (little_2_big < 0)? cpu : little_2_big;
                        switch_flag = 1;
                        break;
                    }
                }

                /* if all load of every little cpu is not very high, we think the load is balanced,
                 * try to check if we can enable another little cpu, if all cpu online, try to switch
                 * to a big
                 */
                if(!switch_flag) {
                    if(num_online_cpus() < num_possible_cpus()) {
                        request_up++;
                    } else {
                        if(little_2_big < 0)
                            little_2_big = cpu;
                        else
                            j_dbs_info->target_freq = dbs_tuners_ins.little_max_freq;
                    }
                }
            } else {
                /* adjust the frquency to a proper value */
                j_dbs_info->target_freq = __cal_little_freq(j_dbs_info->cur_freq,
                        j_dbs_info->load, dbs_tuners_ins.little_max_freq);
            }
        }
        else {
            /* current cpu is big, check if need switch to little */
            if(j_dbs_info->load_freq > dbs_tuners_ins.little_max_freq *
                dbs_tuners_ins.big_2_little_threshold) {
                struct cpufreq_policy *policy = cpufreq_cpu_get(cpu);
                j_dbs_info->target_freq = max(dbs_tuners_ins.big_min_freq,
                    __cal_big_freq(j_dbs_info->cur_freq, j_dbs_info->load, policy->max));
            } else {
                if(big_2_little < 0) {
                    big_2_little = cpu;
                    j_dbs_info->target_freq = dbs_tuners_ins.big_2_little_target;
                }
                else
                    j_dbs_info->target_freq = max(dbs_tuners_ins.big_min_freq,
                        __cal_big_freq(j_dbs_info->cur_freq, j_dbs_info->load, policy->max));
            }
        }

        if(dbs_tuners_ins.dvfs_debug) {
            printk("%s step2: cpu(%d) target:%d\n", __func__, cpu, j_dbs_info->target_freq);
        }
    }
    j_dbs_info = &per_cpu(od_cpu_dbs_info, little_2_big);
    j_dbs_info->target_freq = dbs_tuners_ins.little_2_big_target;

    __skip_target_cal:
    /* step 3: check if need plug-in or plug-out core */
    if(heavy_load_core_cnt >= num_online_cpus())
        request_up++;
    else if(light_load_core_cnt > 1)
        request_down++;
    if(request_up > request_down) {
        for_each_cpu_not(cpu, cpu_online_mask) {
            spin_lock_irqsave(&cpumask_lock, flags);
            cpumask_or(&core_up_mask, cpumask_of(cpu), &core_up_mask);
            spin_unlock_irqrestore(&cpumask_lock, flags);

            if(dbs_tuners_ins.dvfs_debug) {
                printk("%s step3: core up:%d\n", __func__, cpu);
            }

            break;
        }
    } else if(request_up < request_down) {
        spin_lock_irqsave(&cpumask_lock, flags);
        cpumask_or(&core_down_mask, cpumask_of(light_load_core), &core_down_mask);
        spin_unlock_irqrestore(&cpumask_lock, flags);

        if(dbs_tuners_ins.dvfs_debug) {
            printk("%s step3: core down:%d\n", __func__, cpu);
        }
    }

    __skip_core_check:
    /* step 4: try to wake up power change task to process the change */
    spin_lock_irqsave(&cpumask_lock, flags);
    cpumask_or(&speed_change_mask, policy->cpus, &speed_change_mask);
    spin_unlock_irqrestore(&cpumask_lock, flags);
    wake_up_process(power_change_task);
}


static int cpufreq_iks_power_change_task(void *data)
{
    unsigned int    little_cpu_freq, big_cpu_freq, cpu;
    struct cpu_dbs_info_s *j_dbs_info;
    struct cpufreq_policy *policy;
    struct cpumask tmp_core_up_mask, tmp_core_down_mask, tmp_speed_change_mask;
    struct cpumask tmp_little_cores, tmp_big_cores;
    unsigned long flags, tmp_event;

    while (1) {
        set_current_state(TASK_INTERRUPTIBLE);
        spin_lock_irqsave(&cpumask_lock, flags);
        if(cpumask_empty(&speed_change_mask) && cpumask_empty(&core_up_mask)
                    && cpumask_empty(&core_down_mask) && !cpufreq_event_mask) {
            spin_unlock_irqrestore(&cpumask_lock, flags);
            schedule_timeout(10*HZ);
            if (kthread_should_stop())
                break;
            continue;
        }

        set_current_state(TASK_RUNNING);
        tmp_core_up_mask = core_up_mask;
        cpumask_clear(&core_up_mask);
        tmp_core_down_mask = core_down_mask;
        cpumask_clear(&core_down_mask);
        tmp_speed_change_mask = speed_change_mask;
        cpumask_clear(&speed_change_mask);
        tmp_event = cpufreq_event_mask;
        cpufreq_event_mask = 0;
        spin_unlock_irqrestore(&cpumask_lock, flags);

        cpumask_clear(&tmp_little_cores);
        cpumask_clear(&tmp_big_cores);

        /* check if current is under max power mode */
        if(dbs_tuners_ins.max_power) {
            /* up all cores */
            cpumask_clear(&tmp_core_down_mask);
            cpumask_copy(&tmp_core_up_mask, cpu_possible_mask);
            /* set every core to the max frequency */
            cpumask_copy(&tmp_speed_change_mask, cpu_possible_mask);
            little_cpu_freq = cpufreq_quick_get_max(0);
            for_each_cpu(cpu, &tmp_speed_change_mask) {
                j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
                j_dbs_info->target_freq = little_cpu_freq;
            }
            goto __process_power_change;
        }
        if(dbs_tuners_ins.boost) {
            cpumask_copy(&tmp_speed_change_mask, cpu_online_mask);
            little_cpu_freq = cpufreq_quick_get_max(0);
            for_each_cpu(cpu, &tmp_speed_change_mask) {
                j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
                j_dbs_info->target_freq = little_cpu_freq;
            }
            goto __process_power_change;
        }

        if(tmp_event & EVENT_INPUT_DEVICE) {
            /* scaling cpu frequency upper than 80% of little's max frequency */
            cpumask_copy(&tmp_speed_change_mask, cpu_online_mask);
            little_cpu_freq = (dbs_tuners_ins.little_max_freq * DEF_INPUT_PULSE)/100;
            for_each_cpu(cpu, &tmp_speed_change_mask) {
                j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
                j_dbs_info->target_freq = max(little_cpu_freq, j_dbs_info->target_freq);
            }
        }
        if(tmp_event & EVENT_USER_PULSE) {
            cpumask_copy(&tmp_speed_change_mask, cpu_online_mask);
            little_cpu_freq = (cpufreq_quick_get_max(0) * dbs_tuners_ins.pulse_ratio) / 100;
            for_each_cpu(cpu, &tmp_speed_change_mask) {
                j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
                j_dbs_info->target_freq = max(little_cpu_freq, j_dbs_info->target_freq);
            }
            pulse_start = sched_clock();
        } else if(pulse_start) {
            unsigned long long now = sched_clock();
            if((now>pulse_start) && (((now - pulse_start) >> 20) > dbs_tuners_ins.pulse_duration)) {
                /* pulse time end */
                pulse_start = 0;
            }
            cpumask_copy(&tmp_speed_change_mask, cpu_online_mask);
            little_cpu_freq = (cpufreq_quick_get_max(0) * dbs_tuners_ins.pulse_ratio) / 100;
            for_each_cpu(cpu, &tmp_speed_change_mask) {
                j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
                j_dbs_info->target_freq = max(little_cpu_freq, j_dbs_info->target_freq);
            }
        }

        __process_power_change:
        /* step 1: core up or core down */
        if(!cpumask_empty(&tmp_core_up_mask)) {
            for_each_cpu(cpu, &tmp_core_up_mask) {
                if(dbs_tuners_ins.dvfs_debug) {
                    printk("%s step1: core up:%u\n", __func__, cpu);
                }
                if(!cpu_online(cpu))
                    cpu_up(cpu);
            }
        } else if(!cpumask_empty(&tmp_core_down_mask)) {
            cpumask_andnot(&tmp_speed_change_mask, &tmp_speed_change_mask, &tmp_core_down_mask);
            for_each_cpu(cpu, &tmp_core_down_mask) {
                if(dbs_tuners_ins.dvfs_debug) {
                    printk("%s step1: core down:%u\n", __func__, cpu);
                }
                if(cpu_online(cpu))
                    cpu_down(cpu);
            }
        }

        /* step 2: look up big and little frequency */
        little_cpu_freq = big_cpu_freq = 0;
        for_each_cpu(cpu, &tmp_speed_change_mask) {
            j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
            if(j_dbs_info->target_freq <= dbs_tuners_ins.little_max_freq) {
                cpumask_or(&tmp_little_cores, cpumask_of(cpu), &tmp_little_cores);
                j_dbs_info->is_little = true;
                little_cpu_freq = max(little_cpu_freq, j_dbs_info->target_freq);
            } else {
                j_dbs_info->is_little = false;
                cpumask_or(&tmp_big_cores, cpumask_of(cpu), &tmp_big_cores);
                big_cpu_freq = max(big_cpu_freq,  j_dbs_info->target_freq);
            }
        }
        if(dbs_tuners_ins.dvfs_debug) {
            printk("%s step2: little freq:%d, big freq:%d\n", __func__,
                    little_cpu_freq, big_cpu_freq);
        }

        /* step 3: set big and little frequency */
        if(!cpumask_empty(&tmp_little_cores)) {
            for_each_cpu(cpu, &tmp_little_cores) {
                if(dbs_tuners_ins.dvfs_debug) {
                    printk("%s step3: scaling little(%d) to %d Khz\n", __func__, cpu, little_cpu_freq);
                }
                policy = cpufreq_cpu_get(cpu);
                cpufreq_driver_target(policy, little_cpu_freq, CPUFREQ_RELATION_L);
                j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
                j_dbs_info->cur_freq = cpufreq_get(cpu);
                j_dbs_info->max_freq = dbs_tuners_ins.little_max_freq;
            }
        }
        if(!cpumask_empty(&tmp_big_cores)) {
            for_each_cpu(cpu, &tmp_big_cores) {
                if(dbs_tuners_ins.dvfs_debug) {
                    printk("%s step3: scaling big(%d) to %d Khz\n", __func__, cpu, big_cpu_freq);
                }
                policy = cpufreq_cpu_get(cpu);
                cpufreq_driver_target(policy, big_cpu_freq, CPUFREQ_RELATION_L);
                j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
                j_dbs_info->cur_freq = cpufreq_get(cpu);
                j_dbs_info->max_freq = policy->max;
            }
        }
    }

    return 0;
}

static void do_dbs_timer(struct work_struct *work)
{
	struct cpu_dbs_info_s *dbs_info = container_of(work, struct cpu_dbs_info_s, work.work);
	unsigned int cpu = dbs_info->cpu;
	int delay;

	mutex_lock(&dbs_info->timer_mutex);
    dbs_check_cpu(dbs_info);
    delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
	schedule_delayed_work_on(cpu, &dbs_info->work, delay);
	mutex_unlock(&dbs_info->timer_mutex);
}

static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
    /* We want all CPUs to do sampling nearly on same jiffy */
    int delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);
    if (num_online_cpus() > 1)
        delay -= jiffies % delay;

    INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
	schedule_delayed_work_on(dbs_info->cpu, &dbs_info->work, delay);
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
    cancel_delayed_work_sync(&dbs_info->work);
}

static int __cpuinit cpu_hotplug_callback(struct notifier_block *nfb, unsigned long action, void *hcpu)
{
	unsigned int cpu = (unsigned long)hcpu;
    struct cpu_dbs_info_s *j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
    struct cpufreq_policy *policy = cpufreq_cpu_get(cpu);

    switch (action) {
    case CPU_ONLINE:
    case CPU_ONLINE_FROZEN:
        j_dbs_info->prev_cpu_idle = get_cpu_idle_time(cpu, &j_dbs_info->prev_cpu_wall, dbs_tuners_ins.io_is_busy);
        j_dbs_info->cur_freq = cpufreq_get(cpu);
        if(j_dbs_info->cur_freq <= dbs_tuners_ins.little_max_freq) {
            j_dbs_info->max_freq = dbs_tuners_ins.little_max_freq;
            j_dbs_info->is_little = true;
        }
        else {
            j_dbs_info->max_freq = policy->max;
            j_dbs_info->is_little = false;
        }
    	break;

    }
	return NOTIFY_OK;
}

static struct notifier_block __refdata cpu_hotplug_notifier = {
    .notifier_call = cpu_hotplug_callback,
};


#ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY
/*
 * trigger cpu frequency to a high speed some when input event coming.
 * such as key, ir, touchpannel for ex. , but skip gsensor.
 */
static void cpufreq_input_event(struct input_handle *handle,
                        unsigned int type,
                        unsigned int code, int value)
{
    unsigned long flags;

    if (type == EV_SYN && code == SYN_REPORT) {
        spin_lock_irqsave(&cpumask_lock, flags);
        cpufreq_event_mask |= EVENT_INPUT_DEVICE;
        spin_unlock_irqrestore(&cpumask_lock, flags);
        wake_up_process(power_change_task);
    }
}


static int cpufreq_input_connect(struct input_handler *handler,
                         struct input_dev *dev,
                         const struct input_device_id *id)
{
    struct input_handle *handle;
    int error;

    handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
    if (!handle)
        return -ENOMEM;

    handle->dev = dev;
    handle->handler = handler;
    handle->name = "cpufreq_input";

    error = input_register_handle(handle);
    if (error)
        goto err;

    error = input_open_device(handle);
    if (error)
        goto err_open;

    return 0;

err_open:
    input_unregister_handle(handle);
err:
    kfree(handle);
    return error;
}

static void cpufreq_input_disconnect(struct input_handle *handle)
{
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
}

static const struct input_device_id cpufreq_input_ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
             INPUT_DEVICE_ID_MATCH_ABSBIT,
        .evbit = { BIT_MASK(EV_ABS) },
        .absbit = { [BIT_WORD(ABS_MT_POSITION_X)] =
                BIT_MASK(ABS_MT_POSITION_X) |
                BIT_MASK(ABS_MT_POSITION_Y) },
    }, /* multi-touch touchscreen */
    {
        .flags = INPUT_DEVICE_ID_MATCH_KEYBIT |
             INPUT_DEVICE_ID_MATCH_ABSBIT,
        .keybit = { [BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH) },
        .absbit = { [BIT_WORD(ABS_X)] =
                BIT_MASK(ABS_X) | BIT_MASK(ABS_Y) },
    }, /* touchpad */
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
             INPUT_DEVICE_ID_MATCH_BUS   |
             INPUT_DEVICE_ID_MATCH_VENDOR |
             INPUT_DEVICE_ID_MATCH_PRODUCT |
             INPUT_DEVICE_ID_MATCH_VERSION,
        .bustype = BUS_HOST,
        .vendor = 0x0001,
        .product = 0x0001,
        .version = 0x0100,
        .evbit = { BIT_MASK(EV_KEY) },
    }, /* keyboard/ir */
    { },
};

static struct input_handler cpufreq_input_handler = {
    .event          = cpufreq_input_event,
    .connect        = cpufreq_input_connect,
    .disconnect     = cpufreq_input_disconnect,
    .name           = "cpufreq_input",
    .id_table       = cpufreq_input_ids,
};
#endif  /* #ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY */


static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event)
{
    unsigned int cpu = policy->cpu;
    struct cpu_dbs_info_s *this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
    unsigned int j;
    int ret;
    struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };

	switch (event) {
	case CPUFREQ_GOV_START:
        if ((!cpu_online(cpu)) || (!policy->cur))
            return -EINVAL;

        mutex_lock(&dbs_mutex);
        /* init idle/wall time for per-cpu */
        for_each_cpu(j, policy->cpus) {
            struct cpu_dbs_info_s *j_dbs_info;
            j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
            j_dbs_info->cur_policy = policy;
            j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j, &j_dbs_info->prev_cpu_wall, dbs_tuners_ins.io_is_busy);
            j_dbs_info->cur_freq = policy->cur;
            if(j_dbs_info->cur_freq <= dbs_tuners_ins.little_max_freq) {
                j_dbs_info->max_freq = dbs_tuners_ins.little_max_freq;
                j_dbs_info->is_little = true;
            }
            else {
                j_dbs_info->max_freq = policy->max;
                j_dbs_info->is_little = false;
            }

            if(dbs_tuners_ins.dvfs_debug) {
                printk("%s cpu:%u(%s) wall:%llu, idle:%llu\n", __func__, j,
                    j_dbs_info->is_little? "little":"big", j_dbs_info->prev_cpu_wall,
                    j_dbs_info->prev_cpu_idle);
            }
        }
        this_dbs_info->cpu = cpu;
        this_dbs_info->freq_table = cpufreq_frequency_get_table(cpu);
        mutex_init(&this_dbs_info->timer_mutex);

        if (!(dbs_enable++)) {
            /* initialize some variable to default value */
            dbs_tuners_ins.sampling_rate = DEF_SAMPLING_RATE;
            dbs_tuners_ins.io_is_busy = DEF_IOWAIT_IS_BUSY;

            /* crate sys_fs node for the governor attrs, the path is
             * /sys/devices/system/cpu/cpufreq
             */
            ret = sysfs_create_group(cpufreq_global_kobject, &dbs_attr_group);
            if (ret) {
                mutex_destroy(&this_dbs_info->timer_mutex);
                mutex_unlock(&dbs_mutex);
                return ret;
            }

            /* create a high prio task to processing the power change, adjust
             * frequency, process cpu hotplug for ex.
             */
            power_change_task = kthread_create(cpufreq_iks_power_change_task, NULL,
                         "iks-pwr-change");
            if (IS_ERR(power_change_task)) {
                ret = PTR_ERR(power_change_task);
                sysfs_remove_group(cpufreq_global_kobject, &dbs_attr_group);
                mutex_destroy(&this_dbs_info->timer_mutex);
                mutex_unlock(&dbs_mutex);
                return ret;
            }
            sched_setscheduler_nocheck(power_change_task, SCHED_FIFO, &param);
        	get_task_struct(power_change_task);
            register_hotcpu_notifier(&cpu_hotplug_notifier);

            #ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY
            ret = input_register_handler(&cpufreq_input_handler);
            if (ret)
                pr_warn("%s: failed to register input handler\n", __func__);
            #endif
        }

        mutex_unlock(&dbs_mutex);
        dbs_timer_init(this_dbs_info);
		break;

	case CPUFREQ_GOV_STOP:
        dbs_timer_exit(this_dbs_info);
        mutex_lock(&dbs_mutex);
        mutex_destroy(&this_dbs_info->timer_mutex);
        if(!(--dbs_enable)) {
            #ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY
            input_unregister_handler(&cpufreq_input_handler);
            #endif

            unregister_hotcpu_notifier(&cpu_hotplug_notifier);
            kthread_stop(power_change_task);
            put_task_struct(power_change_task);
            /* delete the sys_fs node */
            sysfs_remove_group(cpufreq_global_kobject, &dbs_attr_group);
        }
        mutex_unlock(&dbs_mutex);
		break;

	case CPUFREQ_GOV_LIMITS:
		mutex_lock(&this_dbs_info->timer_mutex);
		if (policy->max < this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->max, CPUFREQ_RELATION_H);
		else if (policy->min > this_dbs_info->cur_policy->cur)
			__cpufreq_driver_target(this_dbs_info->cur_policy,
				policy->min, CPUFREQ_RELATION_L);
		mutex_unlock(&this_dbs_info->timer_mutex);
		break;
	}

	return 0;
}

struct cpufreq_governor cpufreq_gov_iks = {
    .name                   = "iks",
    .governor               = cpufreq_governor_dbs,
    .owner                  = THIS_MODULE,
};


static int __init cpufreq_dbs_init(void)
{
    int ret;

    /* register cpu freq governor */
    ret = cpufreq_register_governor(&cpufreq_gov_iks);
    if (ret)
        goto err_reg;
    spin_lock_init(&cpumask_lock);

    return ret;

err_reg:
    return ret;
}


static void __exit cpufreq_dbs_exit(void)
{
    cpufreq_unregister_governor(&cpufreq_gov_iks);
}


#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_IKS
fs_initcall(cpufreq_dbs_init);
#else
module_init(cpufreq_dbs_init);
#endif
module_exit(cpufreq_dbs_exit);


MODULE_AUTHOR("Kevin.z.m <kevin@allwinnertech.com>");
MODULE_DESCRIPTION("'cpufreq_iks' - A cpufreq governor for big-Little IKS");
MODULE_LICENSE("GPL");

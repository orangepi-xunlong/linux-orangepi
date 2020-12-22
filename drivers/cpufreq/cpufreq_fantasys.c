/*
 * drivers/cpufreq/cpufreq_fantasys.c
 *
 * copyright (c) 2012-2013 allwinnertech
 *
 * based on ondemand policy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/jiffies.h>
#include <linux/kernel_stat.h>
#include <linux/mutex.h>
#include <linux/hrtimer.h>
#include <linux/tick.h>
#include <linux/ktime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/threads.h>
#include <linux/suspend.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/kthread.h>
#ifdef CONFIG_SW_POWERNOW
#include <mach/powernow.h>
#endif

#include "sunxi-cpufreq.h"

/*
 * dbs is used in this file as a shortform for demandbased switching
 * It helps to keep variable names smaller, simpler
 */
#define FANTASY_DEBUG_LEVEL     (1)

#if (FANTASY_DEBUG_LEVEL == 0 )
    #define FANTASY_DBG(format,args...)
    #define FANTASY_WRN(format,args...)
    #define FANTASY_ERR(format,args...)
#elif(FANTASY_DEBUG_LEVEL == 1)
    #define FANTASY_DBG(format,args...)
    #define FANTASY_WRN(format,args...)
    #define FANTASY_ERR(format,args...)     printk(KERN_ERR "[fantasy] err "format,##args)
#elif(FANTASY_DEBUG_LEVEL == 2)
    #define FANTASY_DBG(format,args...)
    #define FANTASY_WRN(format,args...)     printk(KERN_WARNING "[fantasy] wrn "format,##args)
    #define FANTASY_ERR(format,args...)     printk(KERN_ERR "[fantasy] err "format,##args)
#elif(FANTASY_DEBUG_LEVEL == 3)
    #define FANTASY_DBG(format,args...)     printk(KERN_DEBUG "[fantasy] dbg "format,##args)
    #define FANTASY_WRN(format,args...)     printk(KERN_WARNING "[fantasy] wrn "format,##args)
    #define FANTASY_ERR(format,args...)     printk(KERN_ERR "[fantasy] err "format,##args)
#endif

#define DEF_CPU_UP_CYCLE_REF            (10)    /* check cpu up period is n*sample_rate=0.5 second */
#define DEF_CPU_DOWN_RATE               (10)    /* check cpu down period is n*sample_rate=0.5 second */
#define MAX_HOTPLUG_RATE                (10)    /* array size for save history sample data          */

#define DEF_MAX_CPU_LOCK                (0)
#define DEF_START_DELAY                 (0)

#define DEF_SAMPLING_RATE               (50000) /* check cpu frequency sample rate is 50ms */

#define CPU_STATE_DEBUG_TOTAL_MASK      (0x1)
#define CPU_STATE_DEBUG_CORES_MASK      (0x2)

#define CPU_UP_DOWN_FREQ_REF            (400000)


#define POWERNOW_PERFORM_MAX       (1200000)    /* config the maximum frequency of performance mode */

extern int sunxi_cpufreq_getvolt(void);
extern struct cpufreq_dvfs dvfs_table_syscfg[16];

/*
 * static data of cpu usage
 */
struct cpu_usage {
    unsigned int freq;              /* cpu frequency value              */
    unsigned int volt;              /* cpu voltage value                */
    unsigned int loading[NR_CPUS];  /* cpu frequency loading            */
    unsigned int running[NR_CPUS];  /* cpu running list loading         */
    unsigned int iowait[NR_CPUS];   /* cpu waiting                      */
    unsigned int loading_avg;       /* system average freq loading      */
    unsigned int running_avg;       /* system average thread loading    */
    unsigned int iowait_avg;        /* system average waiting           */
};

/* record cpu history loading information */
struct cpu_usage_history {
    struct cpu_usage usage[MAX_HOTPLUG_RATE];
    unsigned int num_hist;          /* current number of history data   */
};
static struct cpu_usage_history *hotplug_history;

/*
 * get average loading of all cpu's run list
 */
static unsigned int get_rq_avg_sys(void)
{
    return nr_running_avg();
}

/*
 * get average loading of spec cpu's run list
 */
static unsigned int get_rq_avg_cpu(int cpu)
{
    return nr_running_avg_cpu(cpu);
}

/*
 * define thread loading policy table for cpu hotplug
 * if(rq > hotplug_rq[0][1]) cpu need plug in, run 2 cores;
 * if(rq < hotplug_rq[1][0]) cpu can plug out, run 1 cores;
 */
#if defined(CONFIG_ARCH_SUN8IW3P1)
static int hotplug_rq_def[NR_CPUS][2] = {
    {0   , 1500},
    {700 , 0   },
};
#elif defined(CONFIG_ARCH_SUN8IW1P1) || defined(CONFIG_ARCH_SUN8IW5P1)
static int hotplug_rq_def[NR_CPUS][2] = {
    {0    , 1500},
    {1000 , 2500},
    {2000 , 3500},
    {3000 , 0   },
};
#endif

#ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY
/*
 * define frequency policy table for user event triger
 * switch cpu frequency to policy->max * 100% if single core currently
 * switch cpu frequency to policy->max * 80% if dual core currently
 */
#if defined(CONFIG_ARCH_SUN8IW3P1)
static int usrevent_freq_def[NR_CPUS] = {
    80,
    60,
};
#elif defined(CONFIG_ARCH_SUN8IW1P1) || defined(CONFIG_ARCH_SUN8IW5P1)
static int usrevent_freq_def[NR_CPUS] = {
    80,
    80,
    60,
    60,
};
#endif
#endif

static int cpu_up_weight_tbl[10] = {
    25,
    20,
    15,
    10,
     5,
     5,
     5,
     5,
     5,
     5,
};

static int hotplug_rq[NR_CPUS][2];

#if defined(CONFIG_ARCH_SUN8IW3P1)
static int pulse_freq[NR_CPUS] =
{
    80,
    80,
};
#elif defined(CONFIG_ARCH_SUN8IW1P1) || defined(CONFIG_ARCH_SUN8IW5P1)
static int pulse_freq[NR_CPUS] =
{
    80,
    80,
    80,
    80,
};
#endif

static int cpu_up_rq_hist[DEF_CPU_UP_CYCLE_REF];
static int cpu_up_iow_hist[DEF_CPU_UP_CYCLE_REF];

/*
 * define data structure for dbs
 */
struct cpu_dbs_info_s {
    u64 prev_cpu_idle;          /* previous idle time statistic */
    u64 prev_cpu_iowait;        /* previous iowait time stat    */
    u64 prev_cpu_wall;          /* previous total time stat     */
    struct cpufreq_policy *cur_policy;
    struct delayed_work work;
    struct work_struct up_work;     /* cpu plug-in processor    */
    struct work_struct down_work;   /* cpu plug-out processer   */
    struct cpufreq_frequency_table *freq_table;
    int cpu;                    /* current cpu number           */
    /*
     * percpu mutex that serializes governor limit change with
     * do_dbs_timer invocation. We do not want do_dbs_timer to run
     * when user is changing the governor or limits.
     */
    struct mutex timer_mutex;   /* semaphore for protection     */
};

/*
 * define percpu variable for dbs information
 */
static DEFINE_PER_CPU(struct cpu_dbs_info_s, od_cpu_dbs_info);
struct workqueue_struct *dvfs_workqueue;
static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event);

/* number of CPUs using this policy */
static unsigned int dbs_enable;

/* thread and flag for pulse cpu frequency */
static struct task_struct *pulse_task;
atomic_t g_pulse_flag = ATOMIC_INIT(0);

/* suspend flag */
static int is_suspended = false;

/* cpu up/down work flag */
int fantasys_cpu_work_initialized = false;
/*
 * dbs_mutex protects dbs_enable in governor start/stop.
 */
static DEFINE_MUTEX(dbs_mutex);

static struct dbs_tuners {
    unsigned int sampling_rate;     /* dvfs sample rate                             */
    unsigned int io_is_busy;        /* flag to mark if iowait calculate as cpu busy */
    unsigned int cpu_up_cycle_ref;  /* history sample rate for cpu up               */
    unsigned int cpu_down_rate;     /* history sample rate for cpu down             */
    unsigned int freq_down_delay;   /* delay sample rate for cpu frequency down     */
    unsigned int max_cpu_lock;      /* max count of online cpu, user limit          */
    atomic_t hotplug_lock;          /* lock cpu online number, disable plug-in/out  */
    unsigned int dvfs_debug;        /* dvfs debug flag, print dbs information       */
    unsigned int cpu_state_debug;   /* cpu status debug flag, print cpu detail info */
#ifdef CONFIG_SW_POWERNOW
    unsigned int powernow;          /* power now mode                               */
#endif
    unsigned int pn_perform_freq;   /* performance mode max freq                    */
} dbs_tuners_ins = {
    .cpu_up_cycle_ref = DEF_CPU_UP_CYCLE_REF,
    .cpu_down_rate = DEF_CPU_DOWN_RATE,
    .freq_down_delay = 0,
    .max_cpu_lock = DEF_MAX_CPU_LOCK,
    .hotplug_lock = ATOMIC_INIT(0),
    .dvfs_debug = 0,
    .cpu_state_debug = 0,
#ifdef CONFIG_SW_POWERNOW
    .powernow = SW_POWERNOW_PERFORMANCE,
#endif
    .pn_perform_freq = POWERNOW_PERFORM_MAX,
};

struct cpufreq_governor cpufreq_gov_fantasys = {
    .name                   = "fantasys",
    .governor               = cpufreq_governor_dbs,
    .owner                  = THIS_MODULE,
};

/*
 * CPU hotplug lock interface
 */
atomic_t g_hotplug_lock = ATOMIC_INIT(0);

/*
 * CPU max power flag
 */
atomic_t g_max_power_flag = ATOMIC_INIT(0);

/*
 * apply cpu hotplug lock, up or down cpu
 */
static void apply_hotplug_lock(void)
{
    int online, possible, lock, flag;
    struct work_struct *work;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    online = num_online_cpus();
    possible = num_possible_cpus();
    lock = atomic_read(&g_hotplug_lock);
    flag = lock - online;

    if (flag == 0)
        return;

    work = flag > 0 ? &dbs_info->up_work : &dbs_info->down_work;

    FANTASY_DBG("%s online:%d possible:%d lock:%d flag:%d %d\n",
         __func__, online, possible, lock, flag, (int)abs(flag));

    queue_work_on(dbs_info->cpu, dvfs_workqueue, work);
}

/*
 * lock cpu number, the number of onlie cpu should less then num_core
 */
int cpufreq_fantasys_cpu_lock(int num_core)
{
    int prev_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    if (num_core < 1 || num_core > num_possible_cpus()) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    prev_lock = atomic_read(&g_hotplug_lock);
    if (prev_lock != 0 && prev_lock < num_core) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    atomic_set(&g_hotplug_lock, num_core);
    apply_hotplug_lock();
    mutex_unlock(&dbs_info->timer_mutex);

    return 0;
}

/*
 * unlock cpu hotplug number
 */
int cpufreq_fantasys_cpu_unlock(int num_core)
{
    int prev_lock = atomic_read(&g_hotplug_lock);
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    if (prev_lock != num_core) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    atomic_set(&g_hotplug_lock, 0);
    mutex_unlock(&dbs_info->timer_mutex);

    return 0;
}


/* cpufreq_pegasusq Governor Tunables */
#define show_one(file_name, object)                    \
static ssize_t show_##file_name                        \
(struct kobject *kobj, struct attribute *attr, char *buf)        \
{                                    \
    return sprintf(buf, "%u\n", dbs_tuners_ins.object);        \
}
show_one(sampling_rate, sampling_rate);
show_one(io_is_busy, io_is_busy);
show_one(cpu_up_cycle_ref, cpu_up_cycle_ref);
show_one(cpu_down_rate, cpu_down_rate);
show_one(max_cpu_lock, max_cpu_lock);
show_one(dvfs_debug, dvfs_debug);
show_one(cpu_state_debug, cpu_state_debug);
static ssize_t show_hotplug_lock(struct kobject *kobj, struct attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&g_hotplug_lock));
}

static ssize_t show_max_power(struct kobject *kobj, struct attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", atomic_read(&g_max_power_flag));
}

static ssize_t show_pulse(struct kobject *kobj, struct attribute *attr, char *buf)
{
    int nr_up, cpu, hotplug_lock;
    struct cpu_dbs_info_s *dbs_info;

    mutex_lock(&dbs_mutex);
    if (!dbs_enable) {
        mutex_unlock(&dbs_mutex);
        return -EINVAL;
    }
    mutex_unlock(&dbs_mutex);

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    hotplug_lock = atomic_read(&g_hotplug_lock);
    if (hotplug_lock)
        goto unlock;

    __cpufreq_driver_target(dbs_info->cur_policy, dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
    nr_up = nr_cpu_ids - num_online_cpus();
    for_each_cpu_not(cpu, cpu_online_mask) {
        if (cpu == 0)
            continue;

        if (nr_up-- == 0)
            break;

        cpu_up(cpu);
    }

    /* avoid plug out cpu in the current cycle, reset the stat cycle */
    hotplug_history->num_hist = 0;
    /* delay 40 sampling cycle for frequency down */
    dbs_tuners_ins.freq_down_delay = 40;

unlock:
    mutex_unlock(&dbs_info->timer_mutex);

    return sprintf(buf, "pulse\n");
}

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

static ssize_t store_cpu_up_cycle_ref(struct kobject *a, struct attribute *b,
                 const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.cpu_up_cycle_ref = (unsigned int)min((int)input, (int)MAX_HOTPLUG_RATE);
    return count;
}

static ssize_t store_cpu_down_rate(struct kobject *a, struct attribute *b,
                   const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.cpu_down_rate = (unsigned int)min((int)input, (int)MAX_HOTPLUG_RATE);
    return count;
}

static ssize_t store_max_cpu_lock(struct kobject *a, struct attribute *b,
                  const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    input = min(input, num_possible_cpus());
    dbs_tuners_ins.max_cpu_lock = input;
    return count;
}

static ssize_t store_hotplug_lock(struct kobject *a, struct attribute *b,
                  const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    int prev_lock;

    mutex_lock(&dbs_mutex);
    if (!dbs_enable) {
        mutex_unlock(&dbs_mutex);
        return -EINVAL;
    }
    mutex_unlock(&dbs_mutex);

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    input = min(input, num_possible_cpus());
    prev_lock = atomic_read(&dbs_tuners_ins.hotplug_lock);

    if (prev_lock)
        cpufreq_fantasys_cpu_unlock(prev_lock);

    if (input == 0) {
        atomic_set(&dbs_tuners_ins.hotplug_lock, 0);
        return count;
    }

    ret = cpufreq_fantasys_cpu_lock(input);
    if (ret) {
        printk(KERN_ERR "[HOTPLUG] already locked with smaller value %d < %d\n",
            atomic_read(&g_hotplug_lock), input);
        return ret;
    }

    atomic_set(&dbs_tuners_ins.hotplug_lock, input);

    return count;
}

static ssize_t store_dvfs_debug(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret;
    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;
    dbs_tuners_ins.dvfs_debug = input > 0;
    return count;
}

static ssize_t store_cpu_state_debug(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret;

    ret = sscanf(buf, "%u", &input);
    if (ret != 1)
        return -EINVAL;

    if (input > 3) {
        return -EINVAL;
    } else {
        dbs_tuners_ins.cpu_state_debug = input;
    }

    return count;
}

static ssize_t store_max_power(struct kobject *a, struct attribute *b,
                const char *buf, size_t count)
{
    unsigned int input;
    int ret, nr_up, cpu, max_power_flag;
    struct cpu_dbs_info_s *dbs_info;

    mutex_lock(&dbs_mutex);
    if (!dbs_enable) {
        mutex_unlock(&dbs_mutex);
        return -EINVAL;
    }
    mutex_unlock(&dbs_mutex);

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);
    ret = sscanf(buf, "%u", &input);
    if (ret != 1) {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }

    atomic_set(&g_max_power_flag, !!input);
    max_power_flag = atomic_read(&g_max_power_flag);
    if (max_power_flag == 1) {
        __cpufreq_driver_target(dbs_info->cur_policy, dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
        nr_up = nr_cpu_ids - num_online_cpus();
        for_each_cpu_not(cpu, cpu_online_mask) {
            if (cpu == 0)
                continue;

            if (nr_up-- == 0)
                break;

            cpu_up(cpu);
        }
    } else if (max_power_flag == 0) {
        hotplug_history->num_hist = 0;
        queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->work, 0);
    } else {
        mutex_unlock(&dbs_info->timer_mutex);
        return -EINVAL;
    }
    mutex_unlock(&dbs_info->timer_mutex);

    return count;
}

define_one_global_rw(sampling_rate);
define_one_global_rw(io_is_busy);
define_one_global_rw(cpu_up_cycle_ref);
define_one_global_rw(cpu_down_rate);
define_one_global_rw(max_cpu_lock);
define_one_global_rw(hotplug_lock);
define_one_global_rw(dvfs_debug);
define_one_global_rw(cpu_state_debug);
define_one_global_rw(max_power);
define_one_global_ro(pulse);

static struct attribute *dbs_attributes[] = {
    &sampling_rate.attr,
    &io_is_busy.attr,
    &cpu_up_cycle_ref.attr,
    &cpu_down_rate.attr,
    &max_cpu_lock.attr,
    &hotplug_lock.attr,
    &dvfs_debug.attr,
    &cpu_state_debug.attr,
    &max_power.attr,
    &pulse.attr,
    NULL
};

static struct attribute_group dbs_attr_group = {
    .attrs = dbs_attributes,
    .name = "fantasys",
};


/*
 * cpu hotplug, just plug in 2 cpu
 */
static void cpu_up_work(struct work_struct *work)
{
    int cpu, nr_up, online, hotplug_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);
    online = num_online_cpus();
    hotplug_lock = atomic_read(&g_hotplug_lock);

    if (hotplug_lock) {
        nr_up = (hotplug_lock - online) > 0 ? (hotplug_lock - online) : 0;
    } else {
        nr_up = 1;
    }

    for_each_cpu_not(cpu, cpu_online_mask) {
        if (cpu == 0)
            continue;

        if (nr_up-- == 0)
            break;

        FANTASY_WRN("cpu up:%d\n", cpu);
        cpu_up(cpu);
    }
    mutex_unlock(&dbs_info->timer_mutex);
}

/*
 * cpu hotplug, cpu plugout
 */
static void cpu_down_work(struct work_struct *work)
{
    int cpu, nr_down, online, hotplug_lock;
    struct cpu_dbs_info_s *dbs_info;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);
    online = num_online_cpus();
    hotplug_lock = atomic_read(&g_hotplug_lock);

    if (hotplug_lock) {
        nr_down = (online - hotplug_lock) > 0 ? (online - hotplug_lock) : 0;
    } else {
        nr_down = 1;
    }

    for_each_online_cpu(cpu) {
        if (cpu == 0)
            continue;

        if (nr_down-- == 0)
            break;

        FANTASY_DBG("cpu down:%d\n", cpu);
        cpu_down(cpu);
    }

    mutex_unlock(&dbs_info->timer_mutex);
}

/* check cpu if need to run max power according to one core's loading/running/iowait */
static int check_up(int num_hist, struct cpufreq_policy *policy)
{
    unsigned int cpu_rq_avg = 0, iow_avg = 0;
    int i, online, index_mod, index_cur, up_rq;
    int hotplug_lock = atomic_read(&g_hotplug_lock);

    /* hotplug has been locked, do nothing */
    if (hotplug_lock > 0)
        return 0;

    online = num_online_cpus();
    if (online == num_possible_cpus())
        return 0;

    if (dbs_tuners_ins.max_cpu_lock != 0 && online >= dbs_tuners_ins.max_cpu_lock)
        return 0;

    if (policy->cur * hotplug_history->usage[num_hist].loading_avg / 100 <= CPU_UP_DOWN_FREQ_REF)
        return 0;

    up_rq = hotplug_rq[online-1][1];
    index_mod = num_hist%dbs_tuners_ins.cpu_up_cycle_ref;
    cpu_up_rq_hist[index_mod] = hotplug_history->usage[num_hist].running_avg;
    cpu_up_iow_hist[index_mod] = hotplug_history->usage[num_hist].iowait_avg;

    for (i=0; i<dbs_tuners_ins.cpu_up_cycle_ref ;i++) {
        index_cur = index_mod - i;
        index_cur = index_cur < 0 ? index_cur + dbs_tuners_ins.cpu_up_cycle_ref : index_cur;
        cpu_rq_avg += cpu_up_rq_hist[index_cur] * cpu_up_weight_tbl[i];
        iow_avg += cpu_up_iow_hist[index_cur] * cpu_up_weight_tbl[i];
    }

    cpu_rq_avg /= 100;
    iow_avg /= 100;
    if (dbs_tuners_ins.dvfs_debug) {
        printk("%s: cpu_rq_avg:%u, up_rq:%u\n", __func__, cpu_rq_avg, up_rq);
    }

    if (cpu_rq_avg > up_rq) {
        FANTASY_WRN("cpu need plugin, cpu_rq_avg: %d>%d\n", cpu_rq_avg, up_rq);
        hotplug_history->num_hist = 0;
        memset(cpu_up_rq_hist, 0, sizeof(cpu_up_rq_hist));
        return 1;
    }

    if (iow_avg > 20) {
        FANTASY_WRN("cpu need plugin, iow_avg: %u>%u\n", iow_avg, 20);
        hotplug_history->num_hist = 0;
        memset(cpu_up_iow_hist, 0, sizeof(cpu_up_iow_hist));
        return 1;
    }

    return 0;
}

/*
 * check if need plug out one cpu core
 */
static int check_down(struct cpufreq_policy *policy)
{
    struct cpu_usage *usage;
    int i, online, down_rq;
    int avg_rq = 0,  max_rq_avg = 0, avg_freq_fact = 0;
    int down_rate = dbs_tuners_ins.cpu_down_rate;
    int num_hist = hotplug_history->num_hist;
    int hotplug_lock = atomic_read(&g_hotplug_lock);

    /* hotplug has been locked, do nothing */
    if (hotplug_lock > 0)
        return 0;

    online = num_online_cpus();
    if (online == 1)
        return 0;

    /* if count of online cpu is larger than the max value, plug out cpu */
    if (dbs_tuners_ins.max_cpu_lock != 0 && online > dbs_tuners_ins.max_cpu_lock)
        return 1;

    /* check if reached the switch point */
    if (num_hist == 0 || num_hist % down_rate)
        return 0;

    down_rq = hotplug_rq[online-1][0];

    /* check system average loading */
    for (i = num_hist - 1; i >= num_hist - down_rate; --i) {
        usage = &hotplug_history->usage[i];
        avg_rq = usage->running_avg;
        avg_freq_fact += usage->freq * usage->loading_avg;
        max_rq_avg = max(max_rq_avg, avg_rq);
    }

    avg_freq_fact /= down_rate;
    avg_freq_fact /= 100;

    if (dbs_tuners_ins.dvfs_debug) {
        printk("%s: avg_freq_fact:%d, down_freq_ref:%d, max_rq_avg:%u, down_rq:%u\n", \
                __func__, avg_freq_fact, CPU_UP_DOWN_FREQ_REF, max_rq_avg, down_rq);
    }

    if ((avg_freq_fact <= CPU_UP_DOWN_FREQ_REF) && (max_rq_avg <= down_rq)) {
        FANTASY_WRN("cpu need plugout, avg_freq_fact: %d<%d, max_rq_avg: %d<%d\n", \
                    avg_freq_fact, CPU_UP_DOWN_FREQ_REF, max_rq_avg, down_rq);
        hotplug_history->num_hist = 0;
        /* need plug out cpu, reset the stat cycle */
        return 1;
    }

    return 0;
}

/*
 * define frequency limit for min londing, cpu frequency should be limit to lower if loading lower
 */
#define FANTASY_CPUFREQ_LOAD_MIN_RATE(freq)         \
    (freq<200000? 30 : (freq<400000? 40 : (freq<600000? 60 : (freq<900000? 70 : 80))))

/*
 * define frequency limit for max londing, cpu frequency should be limit to higher if loading higher
 */
#define FANTASY_CPUFREQ_LOAD_MAX_RATE(freq)         \
    (freq<200000? 60 : (freq<400000? 70 : (freq<600000? 80 : (freq<900000? 90 : 95))))

/*
 * define frequency limit for io-wait rate, cpu frequency should be limit to higher if io-wait higher
 */
#define FANTASY_CPUFREQ_IOW_LIMIT_RATE(iow)         \
    (iow<10? (iow>0? 20 : 0) : (iow<20? 40 : (iow<40? 60 : (iow<60? 80 : 100))))

static inline unsigned int __get_target_freq(unsigned int freq)
{
    struct cpufreq_dvfs *dvfs_inf = NULL;
    dvfs_inf = &dvfs_table_syscfg[0];

    while((dvfs_inf+1)->freq > freq) dvfs_inf++;

    return dvfs_inf->freq;
}

static int check_freq_up_down(struct cpu_dbs_info_s *this_dbs_info, unsigned int *target, int num_hist)
{
    unsigned int freq_load, freq_iow, index;
    struct cpufreq_policy *policy = this_dbs_info->cur_policy;
    unsigned int max_load = 0, max_iow = 0;
    int cpu;

    if (dbs_tuners_ins.freq_down_delay) {
        dbs_tuners_ins.freq_down_delay--;
        *target = policy->cur;
        return 0;
    }

    for_each_online_cpu(cpu) {
        max_load = max(max_load, hotplug_history->usage[num_hist].loading[cpu]);
        max_iow = max(max_iow, hotplug_history->usage[num_hist].iowait[cpu]);
    }

    if (max_load > FANTASY_CPUFREQ_LOAD_MAX_RATE(policy->cur)) {
        if (dbs_tuners_ins.dvfs_debug)
            printk("%s(%d): max_load=%d, cur_freq=%d, load_rate=%d\n",  \
                __func__, __LINE__, max_load, policy->cur, FANTASY_CPUFREQ_LOAD_MAX_RATE(policy->cur));
        *target = __get_target_freq(policy->cur);
        return 1;
    } else if (max_load >= FANTASY_CPUFREQ_LOAD_MIN_RATE(policy->cur)) {
        *target = policy->cur;
        return 0;
    }

    /*
     * calculate freq load
     * freq_load = (cur_freq * max_load) / ((min_rate + max_rate)/2)
     */
    freq_load = (policy->cur * max_load) / 100;
    freq_load = (policy->cur * max_load) / FANTASY_CPUFREQ_LOAD_MAX_RATE(freq_load);

    /* check cpu io-waiting */
    freq_iow = (policy->max*FANTASY_CPUFREQ_IOW_LIMIT_RATE(max_iow))/100;

    if (dbs_tuners_ins.dvfs_debug) {
        printk("%s(%d): max_load=%d, cur_freq=%d, load_rate=%d, freq_load=%d\n", \
            __func__, __LINE__, max_load, policy->cur, FANTASY_CPUFREQ_LOAD_MIN_RATE(policy->cur), freq_load);
        printk("%s(%d): max_iow=%d, limit_rate=%d, freq_iow=%d\n", \
            __func__, __LINE__, max_iow, FANTASY_CPUFREQ_IOW_LIMIT_RATE(max_iow), freq_iow);
    }

    /* select target frequency */
    *target = max(freq_load, freq_iow);

    if (cpufreq_frequency_table_target(policy, this_dbs_info->freq_table, *target, CPUFREQ_RELATION_L, &index)) {
        FANTASY_ERR("%s: failed to get next lowest frequency\n", __func__);
        *target = policy->max;
        return 1;
    }
    *target = this_dbs_info->freq_table[index].frequency;

    return 1;
}

static inline cputime64_t get_cpu_iowait_time(unsigned int cpu, cputime64_t *wall)
{
    u64 iowait_time = get_cpu_iowait_time_us(cpu, wall);

    if (iowait_time == -1ULL)
        return 0;

    return iowait_time;
}

/*
 * check if need plug in/out cpu, if need increase/decrease cpu frequency
 */
static void dbs_check_cpu(struct cpu_dbs_info_s *this_dbs_info)
{
    unsigned int cpu, freq_target = 0;
    struct cpufreq_policy *policy;
    int num_hist = hotplug_history->num_hist;
    int max_hotplug_rate = max((int)dbs_tuners_ins.cpu_up_cycle_ref, (int)dbs_tuners_ins.cpu_down_rate);

    policy = this_dbs_info->cur_policy;
    /* static cpu loading */
    hotplug_history->usage[num_hist].freq = policy->cur;
    hotplug_history->usage[num_hist].volt = sunxi_cpufreq_getvolt();
    hotplug_history->usage[num_hist].running_avg = get_rq_avg_sys();
    hotplug_history->usage[num_hist].loading_avg = 0;
    hotplug_history->usage[num_hist].iowait_avg = 0;
    ++hotplug_history->num_hist;

    for_each_online_cpu(cpu) {
        struct cpu_dbs_info_s *j_dbs_info;
        u64 cur_wall_time, cur_idle_time, cur_iowait_time;
        u64 prev_wall_time, prev_idle_time, prev_iowait_time;
        unsigned int idle_time, wall_time, iowait_time;
        unsigned int load = 0, iowait = 0;

        j_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);
        prev_wall_time = j_dbs_info->prev_cpu_wall;
        prev_idle_time = j_dbs_info->prev_cpu_idle;
        prev_iowait_time = j_dbs_info->prev_cpu_iowait;

        cur_idle_time = get_cpu_idle_time(cpu, &cur_wall_time, dbs_tuners_ins.io_is_busy);
        cur_iowait_time = get_cpu_iowait_time(cpu, &cur_wall_time);

        wall_time = cur_wall_time - prev_wall_time;
        j_dbs_info->prev_cpu_wall = cur_wall_time;

        idle_time = cur_idle_time - prev_idle_time;
        j_dbs_info->prev_cpu_idle = cur_idle_time;

        iowait_time = cur_iowait_time - prev_iowait_time;
        j_dbs_info->prev_cpu_iowait = cur_iowait_time;

        if (dbs_tuners_ins.io_is_busy && idle_time >= iowait_time)
            idle_time -= iowait_time;

        if (wall_time && (wall_time > idle_time))
            load = 100 * (wall_time - idle_time) / wall_time;
        else
            load = 0;
        hotplug_history->usage[num_hist].loading[cpu] = load;

        if (wall_time && (iowait_time < wall_time))
            iowait = 100 * iowait_time / wall_time;
        else
            iowait = 0;
        hotplug_history->usage[num_hist].iowait[cpu] = iowait;

        /* calculate system average loading */
        hotplug_history->usage[num_hist].running[cpu] = get_rq_avg_cpu(cpu);
        hotplug_history->usage[num_hist].loading_avg += load;
        hotplug_history->usage[num_hist].iowait_avg  += iowait;

        if (dbs_tuners_ins.cpu_state_debug & CPU_STATE_DEBUG_CORES_MASK) {
            printk("[cpu%u] %u,%u,%u,%u,%u\n", cpu, \
                    hotplug_history->usage[num_hist].freq, \
                    hotplug_history->usage[num_hist].volt, \
                    hotplug_history->usage[num_hist].loading[cpu], \
                    hotplug_history->usage[num_hist].running[cpu], \
                    hotplug_history->usage[num_hist].iowait[cpu]);
        }
    }
    hotplug_history->usage[num_hist].loading_avg /= num_online_cpus();
    hotplug_history->usage[num_hist].iowait_avg  /= num_online_cpus();

    if (dbs_tuners_ins.cpu_state_debug & CPU_STATE_DEBUG_TOTAL_MASK) {
        printk("%u,%u,%u,%u,%u,%u\n", hotplug_history->usage[num_hist].freq, \
                hotplug_history->usage[num_hist].volt, \
                num_online_cpus(), \
                hotplug_history->usage[num_hist].loading_avg, \
                hotplug_history->usage[num_hist].running_avg, \
                hotplug_history->usage[num_hist].iowait_avg);
    }

    /* Check for every core is in high loading/running/iowait */
    if (check_up(num_hist, policy)) {
        __cpufreq_driver_target(this_dbs_info->cur_policy, this_dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
        queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->up_work);
        return;
    } else if (check_down(policy)) {
        queue_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->down_work);
    }

    if (check_freq_up_down(this_dbs_info, &freq_target, num_hist)) {
        FANTASY_WRN("%s, %d : try to switch cpu freq to %d \n", __func__, __LINE__, freq_target);
        __cpufreq_driver_target(policy, freq_target, CPUFREQ_RELATION_L);
    }

    /* check if history array is out of range */
    if (hotplug_history->num_hist == max_hotplug_rate)
        hotplug_history->num_hist = 0;
}


static void do_dbs_timer(struct work_struct *work)
{
    struct cpu_dbs_info_s *dbs_info = container_of(work, struct cpu_dbs_info_s, work.work);
    unsigned int cpu = dbs_info->cpu;
    int delay;

    mutex_lock(&dbs_info->timer_mutex);

    if (is_suspended)
        goto unlock;

    if (atomic_read(&g_max_power_flag))
        goto unlock;

    dbs_check_cpu(dbs_info);

    /* We want all CPUs to do sampling nearly on
     * same jiffy
     */
    delay = usecs_to_jiffies(dbs_tuners_ins.sampling_rate);

    queue_delayed_work_on(cpu, dvfs_workqueue, &dbs_info->work, delay);

unlock:
    mutex_unlock(&dbs_info->timer_mutex);
}


static inline void dbs_timer_init(struct cpu_dbs_info_s *dbs_info)
{
    /* We want all CPUs to do sampling nearly on same jiffy */
    int delay = usecs_to_jiffies(DEF_START_DELAY * 1000 * 1000 + dbs_tuners_ins.sampling_rate);
    if (num_online_cpus() > 1)
        delay -= jiffies % delay;

    INIT_DELAYED_WORK_DEFERRABLE(&dbs_info->work, do_dbs_timer);
    INIT_WORK(&dbs_info->up_work, cpu_up_work);
    INIT_WORK(&dbs_info->down_work, cpu_down_work);
    queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->work, delay);
    fantasys_cpu_work_initialized = true;
}

static inline void dbs_timer_exit(struct cpu_dbs_info_s *dbs_info)
{
    cancel_delayed_work_sync(&dbs_info->work);
    cancel_work_sync(&dbs_info->up_work);
    cancel_work_sync(&dbs_info->down_work);
    fantasys_cpu_work_initialized = false;
}


static int fantasys_pm_notify(struct notifier_block *nb, unsigned long event, void *dummy)
{
    struct cpu_dbs_info_s *dbs_info;
    struct cpufreq_policy policy;

    if (cpufreq_get_policy(&policy, 0)) {
        printk(KERN_ERR "can not get cpu policy\n");
        return NOTIFY_BAD;
    }

    if (strcmp(policy.governor->name, "fantasys"))
        return NOTIFY_OK;

    dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    mutex_lock(&dbs_info->timer_mutex);

    if (event == PM_SUSPEND_PREPARE) {
        is_suspended = true;
        printk("sunxi cpufreq suspend ok\n");
    } else if (event == PM_POST_SUSPEND) {
        is_suspended = false;
        if (atomic_read(&g_max_power_flag))
            __cpufreq_driver_target(dbs_info->cur_policy, dbs_info->cur_policy->max, CPUFREQ_RELATION_H);
        queue_delayed_work_on(dbs_info->cpu, dvfs_workqueue, &dbs_info->work, 0);
        printk("sunxi cpufreq resume ok\n");
    }

    mutex_unlock(&dbs_info->timer_mutex);
    return NOTIFY_OK;
}

static struct notifier_block fantasys_pm_notifier = {
    .notifier_call = fantasys_pm_notify,
};

static void pulse_cpufreq(void)
{

    unsigned int freq_trig, index;
    struct cpu_dbs_info_s *this_dbs_info = &per_cpu(od_cpu_dbs_info, 0);

    /* cpu frequency limitation has changed, adjust current frequency */
    if (!mutex_trylock(&this_dbs_info->timer_mutex)) {
        FANTASY_WRN("try to lock mutex failed!\n");
        return;
    }

    freq_trig = (this_dbs_info->cur_policy->max * pulse_freq[num_online_cpus() - 1])/100;
    if(!cpufreq_frequency_table_target(this_dbs_info->cur_policy, this_dbs_info->freq_table,
        freq_trig, CPUFREQ_RELATION_L, &index)) {
        freq_trig = this_dbs_info->freq_table[index].frequency;
        if(this_dbs_info->cur_policy->cur < freq_trig) {
            /* set cpu frequenc to the max value, and reset state machine */
            FANTASY_DBG("CPUFREQ_GOV_USREVENT cpu to %u\n", freq_trig);
            __cpufreq_driver_target(this_dbs_info->cur_policy, freq_trig, CPUFREQ_RELATION_L);
        }
    }
    dbs_tuners_ins.freq_down_delay = 6;
    mutex_unlock(&this_dbs_info->timer_mutex);
    return;
}

static int cpufreq_fantasys_pulse_task(void *data)
{
    while (1) {
        set_current_state(TASK_INTERRUPTIBLE);

        if (!atomic_read(&g_pulse_flag)) {
            schedule_timeout(10*HZ);
            if (kthread_should_stop())
                break;

            continue;
        }

        set_current_state(TASK_RUNNING);
        pulse_cpufreq();
        atomic_set(&g_pulse_flag, 0);
    }

    return 0;
}

#ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY
/*
 * trigger cpu frequency to a high speed some when input event coming.
 * such as key, ir, touchpannel for ex. , but skip gsensor.
 */
static void cpufreq_fantasys_input_event(struct input_handle *handle,
                        unsigned int type,
                        unsigned int code, int value)
{
    if (type == EV_SYN && code == SYN_REPORT) {
        atomic_set(&g_pulse_flag, 1);
        wake_up_process(pulse_task);
    }
}


static int cpufreq_fantasys_input_connect(struct input_handler *handler,
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
    handle->name = "cpufreq_fantasys";

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

static void cpufreq_fantasys_input_disconnect(struct input_handle *handle)
{
    input_close_device(handle);
    input_unregister_handle(handle);
    kfree(handle);
}

static const struct input_device_id cpufreq_fantasys_ids[] = {
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

static struct input_handler cpufreq_fantasys_input_handler = {
    .event          = cpufreq_fantasys_input_event,
    .connect        = cpufreq_fantasys_input_connect,
    .disconnect     = cpufreq_fantasys_input_disconnect,
    .name           = "cpufreq_fantasys",
    .id_table       = cpufreq_fantasys_ids,
};
#endif  /* #ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY */


/*
 * cpufreq dbs governor
 */
static int cpufreq_governor_dbs(struct cpufreq_policy *policy, unsigned int event)
{
    unsigned int cpu = policy->cpu;
    struct cpu_dbs_info_s *this_dbs_info;
    unsigned int j;
    int ret;
    struct sched_param param = { .sched_priority = MAX_RT_PRIO-1 };

    this_dbs_info = &per_cpu(od_cpu_dbs_info, cpu);

    switch (event) {
        case CPUFREQ_GOV_POLICY_INIT:
        {
            if ((!cpu_online(cpu)) || (!policy->cur))
                return -EINVAL;

            hotplug_history->num_hist = 0;

            mutex_lock(&dbs_mutex);

            for_each_possible_cpu(j) {
                struct cpu_dbs_info_s *j_dbs_info;
                j_dbs_info = &per_cpu(od_cpu_dbs_info, j);
                j_dbs_info->cur_policy = policy;
                j_dbs_info->prev_cpu_idle = get_cpu_idle_time(j, &j_dbs_info->prev_cpu_wall, dbs_tuners_ins.io_is_busy);
            }
            this_dbs_info->cpu = cpu;
            this_dbs_info->freq_table = cpufreq_frequency_get_table(cpu);

            mutex_init(&this_dbs_info->timer_mutex);

            /*
             * Start the timerschedule work, when this governor
             * is used for first time
             */
            if (!(dbs_enable++)) {
                dbs_tuners_ins.sampling_rate = DEF_SAMPLING_RATE;
                dbs_tuners_ins.io_is_busy = 1;
                dbs_tuners_ins.pn_perform_freq = policy->max;

                pulse_task = kthread_create(cpufreq_fantasys_pulse_task, NULL,
                             "kfantasys-pulse");
                if (IS_ERR(pulse_task)) {
                    ret = PTR_ERR(pulse_task);
                    mutex_destroy(&this_dbs_info->timer_mutex);
                    mutex_unlock(&dbs_mutex);
                    return ret;
                }
                kthread_bind(pulse_task, 0);
                sched_setscheduler_nocheck(pulse_task, SCHED_FIFO, &param);
                get_task_struct(pulse_task);

                #ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY
                ret = input_register_handler(&cpufreq_fantasys_input_handler);
                if (ret)
                    pr_warn("%s: failed to register input handler\n", __func__);
                #endif
            }
            mutex_unlock(&dbs_mutex);

            dbs_timer_init(this_dbs_info);

            break;
        }

        case CPUFREQ_GOV_POLICY_EXIT:
        {
            dbs_timer_exit(this_dbs_info);

            mutex_lock(&dbs_mutex);
            mutex_destroy(&this_dbs_info->timer_mutex);
            if(!(--dbs_enable)) {
                #ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY
                input_unregister_handler(&cpufreq_fantasys_input_handler);
                #endif
                kthread_stop(pulse_task);
                put_task_struct(pulse_task);
            }
            mutex_unlock(&dbs_mutex);

            break;
        }

        case CPUFREQ_GOV_LIMITS:
        {
            if (policy->max < this_dbs_info->cur_policy->cur)
                __cpufreq_driver_target(this_dbs_info->cur_policy, policy->max, CPUFREQ_RELATION_H);
            else if (policy->min > this_dbs_info->cur_policy->cur)
                __cpufreq_driver_target(this_dbs_info->cur_policy, policy->min, CPUFREQ_RELATION_L);

            break;
        }
    }
    return 0;
}

#ifdef CONFIG_SW_POWERNOW
static int start_powernow(unsigned long mode)
{
    int retval = 0, nr_up = 0, cpu = 0;
    struct cpu_dbs_info_s *this_dbs_info;
    struct cpufreq_policy *policy;

    this_dbs_info = &per_cpu(od_cpu_dbs_info, 0);
    policy = this_dbs_info->cur_policy;

    if (policy == NULL) {
	printk("[cpufreq_fantasys] policy is null %s, %d, mode=%lu, powernow=%d\n",
		__func__, __LINE__, mode, dbs_tuners_ins.powernow);
        if (mode != SW_POWERNOW_MAXPOWER && mode != SW_POWERNOW_USEREVENT){
            dbs_tuners_ins.powernow = mode;
        }
        return -EINVAL;
    }
    if (policy->governor != &cpufreq_gov_fantasys) {
        printk(KERN_DEBUG "scaling cur governor is not fantasys!\n");
        if (mode != SW_POWERNOW_MAXPOWER && mode != SW_POWERNOW_USEREVENT) {
            dbs_tuners_ins.powernow = mode;
        }
        return -EINVAL;
    }

    printk(KERN_DEBUG "start_powernow :%d!\n", (int)mode);

    // WARNING: may sleep
    // cancel_delayed_work_sync(&this_dbs_info->work);
    // dbs_timer_exit(this_dbs_info);

    mutex_lock(&this_dbs_info->timer_mutex);
    switch (mode) {
    case SW_POWERNOW_EXTREMITY:
        atomic_set(&g_max_power_flag, 1);
        policy->max = policy->cpuinfo.max_freq;
        policy->user_policy.max = policy->cpuinfo.max_freq;
        printk("%s, %d : max=%d, policy_add=%p\n",
		__func__, __LINE__, policy->max, policy);

        __cpufreq_driver_target(policy, policy->max, CPUFREQ_RELATION_H);

        nr_up = num_possible_cpus() - num_online_cpus();
        for_each_cpu_not(cpu, cpu_online_mask) {
            if (cpu == 0)
                continue;

            if (nr_up-- == 0)
                break;

            cpu_up(cpu);
        }

        break;

    case SW_POWERNOW_PERFORMANCE:
        policy->max = dbs_tuners_ins.pn_perform_freq;
        policy->user_policy.max = dbs_tuners_ins.pn_perform_freq;
        if (atomic_read(&g_max_power_flag) == 1) {
            atomic_set(&g_max_power_flag, 0);
            hotplug_history->num_hist = 0;
            queue_delayed_work_on(this_dbs_info->cpu, dvfs_workqueue, &this_dbs_info->work, 0);
        }
        break;

    default:
        printk(KERN_ERR "start_powernow uncare mode:%d!\n", (int)mode);
        atomic_set(&g_max_power_flag, 0);
        mode = dbs_tuners_ins.powernow;
        retval = -EINVAL;
        break;
    }

#if 0
    if (mode != SW_POWERNOW_EXTREMITY && dbs_enable > 0){
#else
    if (dbs_enable > 0) {
#endif
        // dbs_timer_init(this_dbs_info);
    }
    dbs_tuners_ins.powernow = mode;
    mutex_unlock(&this_dbs_info->timer_mutex);
    return retval;
}

static int powernow_notifier_call(struct notifier_block *nfb,
                    unsigned long mode, void *cmd)
{
    int retval = NOTIFY_DONE;
    printk("[cpufreq_fantasys] enter %s %d, mode=%lu, %s\n",
	    __func__, __LINE__, mode, (char *)cmd);
    mutex_lock(&dbs_mutex);
    retval = start_powernow(mode);
    mutex_unlock(&dbs_mutex);
    retval = retval ? NOTIFY_DONE : NOTIFY_OK;
    return retval;
}

static struct notifier_block fantasys_powernow_notifier = {
    .notifier_call = powernow_notifier_call,
};
#endif /* CONFIG_SW_POWERNOW*/

/*
 * cpufreq governor dbs initiate
 */
static int __init cpufreq_gov_dbs_init(void)
{
    int i, ret;

    /* init policy table */
    for(i=0; i<NR_CPUS; i++) {
        hotplug_rq[i][0] = hotplug_rq_def[i][0];
        hotplug_rq[i][1] = hotplug_rq_def[i][1];

        #ifdef CONFIG_CPU_FREQ_INPUT_EVNT_NOTIFY
        pulse_freq[i] = usrevent_freq_def[i];
        #endif
    }
    hotplug_rq[NR_CPUS-1][1] = INT_MAX;

    hotplug_history = kzalloc(sizeof(struct cpu_usage_history), GFP_KERNEL);
    if (!hotplug_history) {
        FANTASY_ERR("%s cannot create hotplug history array\n", __func__);
        ret = -ENOMEM;
        goto err_hist;
    }

    /* create dvfs daemon */
    dvfs_workqueue = create_singlethread_workqueue("fantasys");
    if (!dvfs_workqueue) {
        pr_err("%s cannot create workqueue\n", __func__);
        ret = -ENOMEM;
        goto err_queue;
    }

    /* register cpu freq governor */
    ret = cpufreq_register_governor(&cpufreq_gov_fantasys);
    if (ret)
        goto err_reg;

    ret = sysfs_create_group(cpufreq_global_kobject, &dbs_attr_group);
    if (ret) {
        goto err_governor;
    }

#ifdef CONFIG_SW_POWERNOW
    ret = register_sw_powernow_notifier(&fantasys_powernow_notifier);
#endif

    register_pm_notifier(&fantasys_pm_notifier);
    return ret;

err_governor:
    cpufreq_unregister_governor(&cpufreq_gov_fantasys);
err_reg:
    destroy_workqueue(dvfs_workqueue);
err_queue:
    kfree(hotplug_history);
err_hist:
    return ret;
}

/*
 * cpufreq governor dbs exit
 */
static void __exit cpufreq_gov_dbs_exit(void)
{
#ifdef CONFIG_SW_POWERNOW
    unregister_sw_powernow_notifier(&fantasys_powernow_notifier);
#endif
    unregister_pm_notifier(&fantasys_pm_notifier);
    sysfs_remove_group(cpufreq_global_kobject, &dbs_attr_group);
    cpufreq_unregister_governor(&cpufreq_gov_fantasys);
    destroy_workqueue(dvfs_workqueue);
    kfree(hotplug_history);
}

MODULE_AUTHOR("kevin.z.m <kevin@allwinnertech.com>");
MODULE_DESCRIPTION("'cpufreq_fantasys' - A dynamic cpufreq/cpuhotplug governor");
MODULE_LICENSE("GPL");

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_FANTASYS
fs_initcall(cpufreq_gov_dbs_init);
#else
module_init(cpufreq_gov_dbs_init);
#endif
module_exit(cpufreq_gov_dbs_exit);

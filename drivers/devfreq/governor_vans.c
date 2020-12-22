/*
 *  linux/drivers/devfreq/governor_vans.c
 *
 *  Copyright (C) 2012 Reuuimlla Technology
 *  pannan<pannan@reuuimllatech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/cpu.h>
#include <linux/device.h>
#include <linux/devfreq.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/pm.h>
#include <linux/mutex.h>
#include <linux/random.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <mach/hardware.h>
#include "governor.h"
#include "dramfreq/sunxi-dramfreq.h"

#ifdef CONFIG_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#if (1)
    #define VANS_DBG(format,args...)   printk("[vans] "format,##args)
#else
    #define VANS_DBG(format,args...)   do{}while(0)
#endif

#define VANS_ERR(format,args...)   printk(KERN_ERR "[vans] ERR:"format,##args)

#ifdef CONFIG_EARLYSUSPEND
#ifdef CONFIG_CPU_FREQ_GOV_FANTASYS
extern int cpufreq_fantasys_cpu_lock(int num_core);
extern int cpufreq_fantasys_cpu_unlock(int num_core);
static int online_backup_fantasys = 0;
extern atomic_t g_hotplug_lock;
#endif
static int online_backup = 0;
#endif

static struct cpumask online_backup_cpumask;
static struct mutex timer_mutex;
struct timer_list dramfreq_timer;
unsigned long dramfreq_expired_time;
int dramfreq_need_suspend = 0;
static struct devfreq *this_devfreq;
static struct work_struct dramfreq_work;
static unsigned int dramfreq_freq_lock = 0;

DECLARE_COMPLETION(dramfreq_resume_done);

static int devfreq_vans_func(struct devfreq *df, unsigned long *freq)
{
    *freq = (dramfreq_need_suspend > 0) ? df->scaling_min_freq : df->scaling_max_freq;

    return 0;
}

static void dramfreq_do_work(struct work_struct *work)
{
    mutex_lock(&timer_mutex);

    /* update dram frequency */
    mutex_lock(&this_devfreq->lock);
    update_devfreq(this_devfreq);
    mutex_unlock(&this_devfreq->lock);

    if (!dramfreq_need_suspend)
        complete(&dramfreq_resume_done);

    mutex_unlock(&timer_mutex);
}

static void dramfreq_vans_timer(unsigned long data)
{
    if (!timer_pending(&dramfreq_timer)) {
        schedule_work(&dramfreq_work);
    }
}

#ifdef CONFIG_EARLYSUSPEND
static int dramfreq_lock_cpu0(void)
{
    struct cpufreq_policy policy;
    int cpu, nr_down;

    if (cpufreq_get_policy(&policy, 0)) {
        VANS_ERR("can not get cpu policy\n");
        return -1;
    }

#ifdef CONFIG_CPU_FREQ_GOV_FANTASYS
    if (!strcmp(policy.governor->name, "fantasys")) {
        online_backup_fantasys = atomic_read(&g_hotplug_lock);
        if ((online_backup_fantasys >= 0) && (online_backup_fantasys <= nr_cpu_ids)) {
            cpufreq_fantasys_cpu_lock(1);
            while (num_online_cpus() != 1) {
                msleep(50);
            }
        }
        goto out;
    }
#endif

    online_backup = num_online_cpus();
    cpumask_clear(&online_backup_cpumask);
    cpumask_copy(&online_backup_cpumask, cpu_online_mask);
    if ((online_backup > 1) && (online_backup <= nr_cpu_ids)) {
        nr_down = online_backup - 1;
        for_each_cpu(cpu, &online_backup_cpumask) {
            if (cpu == 0)
                continue;

            if (nr_down-- == 0)
                break;

            printk("cpu down:%d\n", cpu);
            cpu_down(cpu);
        }
    } else if (online_backup == 1) {
        printk("only cpu0 online, need not down\n");
    } else {
        VANS_ERR("ERROR online cpu sum is %d\n", online_backup);
    }

#ifdef CONFIG_CPU_FREQ_GOV_FANTASYS
out:
#endif
    return 0;
}

static int dramfreq_unlock_cpu0(void)
{
    struct cpufreq_policy policy;
    int cpu, nr_up;

    if (cpufreq_get_policy(&policy, 0)) {
        VANS_ERR("can not get cpu policy\n");
        return -1;
    }

#ifdef CONFIG_CPU_FREQ_GOV_FANTASYS
    if (!strcmp(policy.governor->name, "fantasys")) {
        if (online_backup_fantasys >= 0 && online_backup_fantasys <= nr_cpu_ids) {
            printk("online_backup_fantasys is %d\n", online_backup_fantasys);
            cpufreq_fantasys_cpu_unlock(1);
            if (online_backup_fantasys) {
                cpufreq_fantasys_cpu_lock(online_backup_fantasys);
                while (num_online_cpus() != online_backup_fantasys) {
                    msleep(50);
                }
            }
        }
        goto out;
    }
#endif

    if (online_backup > 1 && online_backup <= nr_cpu_ids) {
        nr_up = online_backup - 1;
        for_each_cpu(cpu, &online_backup_cpumask) {
            if (cpu == 0)
                continue;

            if (nr_up-- == 0)
                break;

            printk("cpu up:%d\n", cpu);
            cpu_up(cpu);
        }
    } else if (online_backup == 1) {
        printk("only cpu0 online, need not up\n");
    } else {
        VANS_ERR("ERROR online backup cpu sum is %d\n", online_backup);
    }

#ifdef CONFIG_CPU_FREQ_GOV_FANTASYS
out:
#endif
    return 0;
}

static void dramfreq_early_suspend(struct early_suspend *h)
{
    if (dramfreq_freq_lock) {
        printk("usb already in, do not need early suspend\n");
        return;
    }

    if (dramfreq_lock_cpu0()) {
        VANS_ERR("%s: can not lock cpu0\n", __func__);
        return;
    }

    mutex_lock(&timer_mutex);
    dramfreq_need_suspend = 1;
    dramfreq_expired_time = jiffies + msecs_to_jiffies(2000);
    mod_timer(&dramfreq_timer, dramfreq_expired_time);
    mutex_unlock(&timer_mutex);

    printk("%s:%s done\n", __FILE__, __func__);
}

static void dramfreq_late_resume(struct early_suspend *h)
{
    int timer_pending = 0;

    if (dramfreq_freq_lock) {
        printk("usb already in, do not need late resume\n");
        return;
    }

    mutex_lock(&timer_mutex);
    dramfreq_need_suspend = 0;
    if (time_before(jiffies, dramfreq_expired_time)) {
        VANS_DBG("timer pending!!!!\n");
        timer_pending = 1;
        del_timer_sync(&dramfreq_timer);
    } else {
        schedule_work(&dramfreq_work);
    }
    mutex_unlock(&timer_mutex);

    if (!timer_pending)
        wait_for_completion(&dramfreq_resume_done);

    if (dramfreq_unlock_cpu0()) {
        VANS_ERR("%s: can not unlock cpu0\n", __func__);
        return;
    }

    printk("%s:%s done\n", __FILE__, __func__);
}

static struct early_suspend dramfreq_earlysuspend =
{
    .level   = EARLY_SUSPEND_LEVEL_DISABLE_FB + 300,
    .suspend = dramfreq_early_suspend,
    .resume  = dramfreq_late_resume,
};
#endif

static ssize_t show_max_freq_lock(struct device *dev, struct device_attribute *attr,
            char *buf)
{
    return sprintf(buf, "%u\n", dramfreq_freq_lock);
}

static ssize_t store_max_freq_lock(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t count)
{
    struct devfreq *df = to_devfreq(dev);
    unsigned int value;
    int ret;

    ret = sscanf(buf, "%u", &value);
    if (ret != 1)
        goto out;

    mutex_lock(&df->lock);
    dramfreq_freq_lock = value;
    if (dramfreq_freq_lock == 1) {
        del_timer_sync(&dramfreq_timer);
        df->scaling_min_freq = df->max_freq;
    } else if(dramfreq_freq_lock == 0) {
        df->scaling_min_freq = df->min_freq;
    } else {
        VANS_ERR("unsupported paras: %u\n", dramfreq_freq_lock);
        mutex_unlock(&df->lock);
        goto out;
    }

    update_devfreq(df);
    ret = count;
    mutex_unlock(&df->lock);

out:
    return ret;
}

static DEVICE_ATTR(max_freq_lock, 0600, show_max_freq_lock, store_max_freq_lock);

static struct attribute *dev_entries[] = {
    &dev_attr_max_freq_lock.attr,
    NULL,
};

static struct attribute_group dev_attr_group = {
    .name   = "vans",
    .attrs  = dev_entries,
};

static int vans_init(struct devfreq *devfreq)
{
    cpumask_clear(&online_backup_cpumask);
    mutex_init(&timer_mutex);
    init_timer(&dramfreq_timer);
    dramfreq_timer.function = dramfreq_vans_timer;
    this_devfreq = devfreq;
    INIT_WORK(&dramfreq_work, dramfreq_do_work);
#ifdef  CONFIG_EARLYSUSPEND
    register_early_suspend(&dramfreq_earlysuspend);
#endif

    return sysfs_create_group(&devfreq->dev.kobj, &dev_attr_group);
}

static void vans_exit(struct devfreq *devfreq)
{
    sysfs_remove_group(&devfreq->dev.kobj, &dev_attr_group);
#ifdef CONFIG_EARLYSUSPEND
    unregister_early_suspend(&dramfreq_earlysuspend);
#endif
    cancel_work_sync(&dramfreq_work);
    del_timer_sync(&dramfreq_timer);
    mutex_destroy(&timer_mutex);
}

const struct devfreq_governor devfreq_vans = {
    .name = "vans",
    .init = vans_init,
    .exit = vans_exit,
    .get_target_freq = devfreq_vans_func,
    .no_central_polling = true,
};

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("'governor_vans' - a simple dram frequency policy");
MODULE_AUTHOR("pannan");

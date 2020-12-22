/* kernel/power/earlysuspend.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
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
 */

#include <linux/earlysuspend.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/rtc.h>
#include <linux/syscalls.h> /* sys_sync */
#include <linux/wakelock.h>
#include <linux/workqueue.h>

#include "power.h"
#include <linux/delay.h>

static struct kobject *earlysuspend_kobj;

enum {
	DEBUG_USER_STATE = 1U << 0,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_VERBOSE = 1U << 3,
};
static int debug_mask = DEBUG_USER_STATE;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);
static int debug_mask_delay_ms = 0;
module_param_named(debug_mask_delay_ms, debug_mask_delay_ms, int, S_IRUGO | S_IWUSR | S_IWGRP);

extern struct wake_lock sync_wake_lock;
extern struct workqueue_struct *sync_work_queue;

static void sync_system(struct work_struct *work);
static DEFINE_MUTEX(early_suspend_lock);
static LIST_HEAD(early_suspend_handlers);
static void early_suspend(struct work_struct *work);
static void late_resume(struct work_struct *work);
static DECLARE_WORK(sync_system_work, sync_system);
static DECLARE_WORK(early_suspend_work, early_suspend);
static DECLARE_WORK(late_resume_work, late_resume);
static DEFINE_SPINLOCK(state_lock);
enum {
	SUSPEND_REQUESTED = 0x1,
	SUSPENDED = 0x2,
	SUSPEND_REQUESTED_AND_SUSPENDED = SUSPEND_REQUESTED | SUSPENDED,
};
static int state;

#ifdef CONFIG_EARLYSUSPEND_DELAY
extern struct wake_lock ealysuspend_delay_work;
#endif

static void sync_system(struct work_struct *work)
{
        wake_lock(&sync_wake_lock);
        sys_sync();
        wake_unlock(&sync_wake_lock);
}

static struct hrtimer earlysuspend_timer;
static void (*earlysuspend_func)(struct early_suspend *h);
static void (*stallfunc)(struct early_suspend *h);
static int earlysuspend_timeout_ms = 7000; //7s

void register_early_suspend(struct early_suspend *handler)
{
	struct list_head *pos;

	mutex_lock(&early_suspend_lock);
	list_for_each(pos, &early_suspend_handlers) {
		struct early_suspend *e;
		e = list_entry(pos, struct early_suspend, link);
		if (e->level > handler->level)
			break;
	}
	list_add_tail(&handler->link, pos);
	if ((state & SUSPENDED) && handler->suspend)
		handler->suspend(handler);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(register_early_suspend);

void unregister_early_suspend(struct early_suspend *handler)
{
	mutex_lock(&early_suspend_lock);
	list_del(&handler->link);
	mutex_unlock(&early_suspend_lock);
}
EXPORT_SYMBOL(unregister_early_suspend);

static void early_suspend(struct work_struct *work)
{
	struct early_suspend *pos;
	unsigned long irqflags;
	int abort = 0;
	ktime_t calltime;
	u64 usecs64;
	int usecs;
	ktime_t starttime;

	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED)
		state |= SUSPENDED;
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("early_suspend: abort, state %d\n", state);
		mutex_unlock(&early_suspend_lock);
		goto abort;
	}

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("early_suspend: call handlers\n");

	/*in case some device blocking in the early suspend process, especially the devices after tp.*/
	hrtimer_cancel(&earlysuspend_timer);
	hrtimer_start(&earlysuspend_timer,
			ktime_set(earlysuspend_timeout_ms / 1000, (earlysuspend_timeout_ms % 1000) * 1000000),
			HRTIMER_MODE_REL);

	list_for_each_entry(pos, &early_suspend_handlers, link) {
		if (pos->suspend != NULL) {
			if (debug_mask & DEBUG_VERBOSE){
				pr_info("early_suspend: calling %pf\n", pos->suspend);
				starttime = ktime_get();
				
			}
			//backup suspend addr.
			earlysuspend_func = pos->suspend;
			pos->suspend(pos);

			if (debug_mask & DEBUG_VERBOSE){
				calltime = ktime_get();
				usecs64 = ktime_to_ns(ktime_sub(calltime, starttime));
				do_div(usecs64, NSEC_PER_USEC);
				usecs = usecs64;
				if (usecs == 0)
					usecs = 1;
				pr_info("early_suspend: %pf complete after %ld.%03ld msecs\n",
					pos->suspend, usecs / USEC_PER_MSEC, usecs % USEC_PER_MSEC);
				if(debug_mask_delay_ms > 0){
				    printk("sleep %d ms for debug. \n", debug_mask_delay_ms);
				    msleep(debug_mask_delay_ms);
				}
			}
		}
	}
	
	hrtimer_cancel(&earlysuspend_timer);
	standby_level = STANDBY_WITH_POWER;
	mutex_unlock(&early_suspend_lock);

	if (debug_mask & DEBUG_SUSPEND)
		pr_info("early_suspend: sync\n");

       queue_work(sync_work_queue, &sync_system_work);
abort:
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPEND_REQUESTED_AND_SUSPENDED)
		wake_unlock(&main_wake_lock);
	spin_unlock_irqrestore(&state_lock, irqflags);
}

static void late_resume(struct work_struct *work)
{
	struct early_suspend *pos;
	unsigned long irqflags;
	int abort = 0;
	ktime_t calltime;
	u64 usecs64;
	int usecs;
	ktime_t starttime;

	mutex_lock(&early_suspend_lock);
	spin_lock_irqsave(&state_lock, irqflags);
	if (state == SUSPENDED)
		state &= ~SUSPENDED;
	else
		abort = 1;
	spin_unlock_irqrestore(&state_lock, irqflags);

	if (abort) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("late_resume: abort, state %d\n", state);
		goto abort;
	}
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: call handlers\n");

	/*in case some device blocking in the late_resume process, especially the devices after tp.*/
	hrtimer_cancel(&earlysuspend_timer);
	hrtimer_start(&earlysuspend_timer,
			ktime_set(earlysuspend_timeout_ms / 1000, (earlysuspend_timeout_ms % 1000) * 1000000),
			HRTIMER_MODE_REL);
	
	list_for_each_entry_reverse(pos, &early_suspend_handlers, link) {
		if (pos->resume != NULL) {
			if (debug_mask & DEBUG_VERBOSE){
				pr_info("late_resume: calling %pf\n", pos->resume);
				starttime = ktime_get();
			}
			//backup resume addr.
			earlysuspend_func = pos->resume;
			pos->resume(pos);

			if (debug_mask & DEBUG_VERBOSE){
				calltime = ktime_get();
				usecs64 = ktime_to_ns(ktime_sub(calltime, starttime));
				do_div(usecs64, NSEC_PER_USEC);
				usecs = usecs64;
				if (usecs == 0)
					usecs = 1;
				pr_info("late_resume: %pf complete after %ld.%03ld msecs\n",
					pos->resume, usecs / USEC_PER_MSEC, usecs % USEC_PER_MSEC);
				if(debug_mask_delay_ms > 0){
				    printk("sleep %d ms for debug. \n", debug_mask_delay_ms);
				    msleep(debug_mask_delay_ms);
				}
			}
	
		}
	}
	
	
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("late_resume: done\n");

	hrtimer_cancel(&earlysuspend_timer);

	standby_level = STANDBY_INITIAL;
abort:
	mutex_unlock(&early_suspend_lock);
}

void request_suspend_state(suspend_state_t new_state)
{
	unsigned long irqflags;
	int old_sleep;

	spin_lock_irqsave(&state_lock, irqflags);
	old_sleep = state & SUSPEND_REQUESTED;
	if (debug_mask & DEBUG_USER_STATE) {
		struct timespec ts;
		struct rtc_time tm;
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		pr_info("request_suspend_state: %s (%d->%d) at %lld "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n",
			new_state != PM_SUSPEND_ON ? "sleep" : "wakeup",
			requested_suspend_state, new_state,
			ktime_to_ns(ktime_get()),
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}
	if (!old_sleep && new_state != PM_SUSPEND_ON) {
		state |= SUSPEND_REQUESTED;

        #ifdef CONFIG_EARLYSUSPEND_DELAY
        /* delay 5 seconds to enter suspend */
        wake_unlock(&ealysuspend_delay_work);
        wake_lock_timeout(&ealysuspend_delay_work, HZ * 5);
        #endif

		queue_work(suspend_work_queue, &early_suspend_work);
	} else if (old_sleep && new_state == PM_SUSPEND_ON) {
		state &= ~SUSPEND_REQUESTED;
		wake_lock(&main_wake_lock);
		queue_work(suspend_work_queue, &late_resume_work);
	}
	requested_suspend_state = new_state;
	spin_unlock_irqrestore(&state_lock, irqflags);
}

suspend_state_t get_suspend_state(void)
{
	return requested_suspend_state;
}

static enum hrtimer_restart earlysuspend_timer_func(struct hrtimer *timer)
{
	//record the lastest time stall function point.
	stallfunc = earlysuspend_func;
	printk("NOTICE: called earlysuspend_func or lateresume func = %pf. \
		stalled. \n", stallfunc);
	return HRTIMER_NORESTART;
}

/*it is used for clear the stallfunc's state*/
static ssize_t earlysuspend_stallfunc_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 16, &data);
	if (error)
		return error;

	//data represent the func's addr
	stallfunc = (void (*)(struct early_suspend *h))(data);
	
	return count;
}


static ssize_t earlysuspend_stallfunc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	if(NULL == stallfunc){		
		return sprintf(buf, "0\n"); 
	}else{
		return sprintf(buf, "%pf\n", stallfunc);
	}
}


static DEVICE_ATTR(debug_stallfunc, S_IRUGO|S_IWUSR|S_IWGRP,
		earlysuspend_stallfunc_show, earlysuspend_stallfunc_store);
		
static struct attribute *earlysuspend_attributes[] = {
	&dev_attr_debug_stallfunc.attr,
	NULL
};

static struct attribute_group earlysuspend_attribute_group = {
	.attrs = earlysuspend_attributes
};


static int __init earlysuspend_init(void)
{

	stallfunc = NULL;
	hrtimer_init(&earlysuspend_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	earlysuspend_timer.function = earlysuspend_timer_func;

	earlysuspend_kobj = kobject_create_and_add("earlysuspend", power_kobj);
	if (!earlysuspend_kobj)
		return -ENOMEM;
	return sysfs_create_group(earlysuspend_kobj, &earlysuspend_attribute_group);

	return 0;
}

core_initcall(earlysuspend_init);


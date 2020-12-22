#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/semaphore.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/vmalloc.h>
#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/kernel_stat.h>
#include <linux/reboot.h>
#include <asm/uaccess.h>
#define CREATE_TRACE_POINTS
#include <trace/events/cpu_autohotplug.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0))
#include <linux/sched/rt.h>
#endif
#include <linux/freezer.h>
#include "autohotplug.h"

struct auto_hotplug_data_struct auto_hotplug_data;
static DEFINE_PER_CPU(struct auto_cpu_hotplug_cpuinfo, cpuinfo);
static struct auto_cpu_hotplug_governor governor;
static struct cpumask hmp_fast_cpu_mask;
static struct cpumask hmp_slow_cpu_mask;
static struct task_struct *auto_hotplug_task;
struct timer_list hotplug_task_timer;
struct timer_list hotplug_sample_timer;
static struct mutex hotplug_enable_mutex;
static spinlock_t hotplug_load_lock;
static spinlock_t cpumask_lock;
struct autohotplug_governor *cur_governor;

static atomic_t hotplug_lock   = ATOMIC_INIT(0);
static atomic_t g_hotplug_lock = ATOMIC_INIT(0);

static unsigned int hotplug_enable    = 0;
static unsigned int boost_all_online  = 0;
static unsigned int hotplug_period_us = 500000;
static unsigned int hotplug_sample_us = 20000;
static unsigned int cpu_up_lastcpu    = INVALID_CPU;

static unsigned int hotplug_up_attempt_hold_us = 500000;
static unsigned int hotplug_up_cpu_hold_us     = 1500000;
static unsigned int hotplug_boost_hold_us      = 3000000;
static unsigned int total_nr_cpus              = CONFIG_NR_CPUS;

static unsigned long cpu_sleep_lasttime[CONFIG_NR_CPUS];

unsigned int load_try_down          = 30;
unsigned int load_save_up           = 75;
unsigned int load_try_up            = 80;
unsigned int load_try_boost         = 95;

unsigned int load_last_big_min_freq = 300000;
unsigned int load_up_stable_us      = 100000;
unsigned int load_down_stable_us    = 500000;
unsigned int load_boost_stable_us   = 200000;

unsigned long cpu_boost_lasttime;
unsigned long cpu_up_lasttime;
unsigned long cpu_in_sleep[CONFIG_NR_CPUS];

static int c0_min = 0;
static int c0_max = 0;
static int c1_min = 0;
static int c1_max = 0;

static char *cpu_state_sysfs[]=
{
	"cpu0_on",
	"cpu1_off",
	"cpu2_off",
	"cpu3_off",
	"cpu4_off",
	"cpu5_off",
	"cpu6_off",
	"cpu7_off"
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0)) || \
defined(CONFIG_ARCH_SUN9IW1) || \
defined(CONFIG_ARCH_SUN8IW5) || \
defined(CONFIG_ARCH_SUN8IW6) || \
defined(CONFIG_ARCH_SUN8IW7)
extern u64 get_cpu_idle_time(unsigned int cpu, u64 *wall, int io_busy);
#else
static inline cputime64_t get_cpu_idle_time_jiffy(unsigned int cpu,
												  cputime64_t *wall)
{
	u64 idle_time;
	u64 cur_wall_time;
	u64 busy_time;

	cur_wall_time = jiffies64_to_cputime64(get_jiffies_64());

	busy_time  = kcpustat_cpu(cpu).cpustat[CPUTIME_USER];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SYSTEM];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_IRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_SOFTIRQ];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_STEAL];
	busy_time += kcpustat_cpu(cpu).cpustat[CPUTIME_NICE];

	idle_time = cur_wall_time - busy_time;
	if (wall)
			*wall = jiffies_to_usecs(cur_wall_time);

	return jiffies_to_usecs(idle_time);
}

static inline cputime64_t get_cpu_idle_time(unsigned int cpu,
											cputime64_t *wall,int io_busy)
{
		u64 idle_time = get_cpu_idle_time_us(cpu, wall);

		if (idle_time == -1ULL)
				idle_time = get_cpu_idle_time_jiffy(cpu, wall);
		else if (!io_busy)
				idle_time += get_cpu_iowait_time_us(cpu, wall);

		return idle_time;
}
#endif

/* Check if cpu is in fastest hmp_domain */
unsigned int is_cpu_big(int cpu)
{
	return cpumask_test_cpu(cpu, &hmp_fast_cpu_mask);
}

/* Check if cpu is in slowest hmp_domain */
unsigned int is_cpu_little(int cpu)
{
	return cpumask_test_cpu(cpu, &hmp_slow_cpu_mask);
}

static int get_cpu_load_level(unsigned int load)
{
	if (load == INVALID_LOAD)
		return LOAD_LEVEL_INVALID;
	else if(load < load_try_down)
		return LOAD_LEVEL_DOWN;
	else if(load < load_save_up)
		return LOAD_LEVEL_NORMAL;
	else if(load < load_try_up)
		return LOAD_LEVEL_MIDDLE;
	else if(load < load_try_boost)
		return LOAD_LEVEL_UP;
	else
		return LOAD_LEVEL_BOOST;
}

static void set_cpu_load(unsigned int cpu, unsigned int load)
{
	if (cpu >= total_nr_cpus)
		return;

	if (get_cpu_load_level(governor.load.cpu_load[cpu])
								!= get_cpu_load_level(load))
		governor.load.cpu_load_lasttime[cpu] = jiffies;

	governor.load.cpu_load[cpu] = load;
}

int do_cpu_down(unsigned int cpu)
{
	int i, c0_online=0, c1_online=0;
    struct cpumask* c0_mask;
    struct cpumask* c1_mask;

	if (cpu == 0 || cpu >= total_nr_cpus)
		return 0;
    if(cpumask_test_cpu(0,&hmp_slow_cpu_mask))
    {
        c0_mask=&hmp_slow_cpu_mask;
        c1_mask=&hmp_fast_cpu_mask;
    }
    else
    {
        c0_mask=&hmp_fast_cpu_mask;
        c1_mask=&hmp_slow_cpu_mask;
    }
	for_each_online_cpu(i) {
		if (cpumask_test_cpu(i, c0_mask))
			c0_online++;
		else if (cpumask_test_cpu(i, c1_mask))
			c1_online++;
	}

	if (cpumask_test_cpu(cpu,c0_mask) && c0_online <= c0_min)
		return 0;

	if (cpumask_test_cpu(cpu,c1_mask) && c1_online <= c1_min)
		return 0;

	if (cpu == cpu_up_lastcpu && time_before(jiffies,
				cpu_up_lasttime + usecs_to_jiffies(hotplug_up_cpu_hold_us)))
		return 0;

	if (time_before(jiffies,
				cpu_boost_lasttime + usecs_to_jiffies(hotplug_boost_hold_us)))
		return 0;

	if (cpu_down(cpu))
		return 0;

	trace_cpu_autohotplug_operate(cpu, 0);
	return 1;

}

int do_cpu_up(unsigned int cpu)
{
	int i, c0_online = 0, c1_online = 0;
    struct cpumask* c0_mask;
    struct cpumask* c1_mask;

	if (cpu == 0 || cpu >= total_nr_cpus)
		return 0;
    if(cpumask_test_cpu(0,&hmp_slow_cpu_mask))
    {
        c0_mask=&hmp_slow_cpu_mask;
        c1_mask=&hmp_fast_cpu_mask;
    }
    else
    {
        c0_mask=&hmp_fast_cpu_mask;
        c1_mask=&hmp_slow_cpu_mask;
    }
	for_each_online_cpu(i) {
		if (cpumask_test_cpu(i, c0_mask))
			c0_online++;
		else if (cpumask_test_cpu(i, c1_mask))
			c1_online++;
	}

	if (cpumask_test_cpu(cpu,c0_mask) && c0_online >= c0_max)
		return 0;

	if (cpumask_test_cpu(cpu,c1_mask) && c1_online >= c1_max)
		return 0;

	if (cpu_up(cpu))
		return 0;

	cpu_up_lastcpu = cpu;
	cpu_up_lasttime = jiffies;

	trace_cpu_autohotplug_operate(cpu, 1);
	return 1;
}

int get_cpus_stable_under(struct auto_cpu_hotplug_loadinfo *load,
						unsigned char level, unsigned int *first, int is_up)
{
	int i, found = 0, count = 0;

	for (i = total_nr_cpus - 1; i >= 0; i--) {
		if ((load->cpu_load[i] != INVALID_LOAD)
				&& load->cpu_load[i] < level
				&& time_after_eq(jiffies, load->cpu_load_lasttime[i]
					+ usecs_to_jiffies(is_up ? load_up_stable_us
					: load_down_stable_us))
			)
		{
			if (first && (!found)) {
				*first = i;
				found = 1;
			}
			count++;
		}
	}

	return count;
}

int get_bigs_under(struct auto_cpu_hotplug_loadinfo *load,
						unsigned char level, unsigned int *first)
{
	int i, found = 0, count = 0;

	for (i = total_nr_cpus - 1; i >= 0; i--) {
		if ((load->cpu_load[i] != INVALID_LOAD)
				&& (load->cpu_load[i] < level)
				&& is_cpu_big(i))
		{
			if (first && (!found)) {
				*first = i;
				found = 1;
			}
			count++;
		}
	}

	return count;
}

int get_bigs_above(struct auto_cpu_hotplug_loadinfo *load,
						unsigned char level, unsigned int *first)
{
	int i, found = 0, count = 0;

	for (i = total_nr_cpus - 1; i >= 0; i--) {
		if ((load->cpu_load[i] != INVALID_LOAD)
				&& (load->cpu_load[i] >= level)
				&& is_cpu_big(i))
		{
			if (first && (!found)) {
				*first = i;
				found = 1;
			}
			count++;
		}
	}

	return count;
}

int get_cpus_under(struct auto_cpu_hotplug_loadinfo *load,
						unsigned char level, unsigned int *first)
{
	int i, found = 0, count = 0;

	for (i = total_nr_cpus - 1; i >= 0; i--) {
		if ((load->cpu_load[i] != INVALID_LOAD) && load->cpu_load[i] < level) {
			if (first && (!found)) {
				*first = i;
				found = 1;
			}
			count++;
		}
	}

	return count;
}

int get_littles_under(struct auto_cpu_hotplug_loadinfo* load,
						unsigned char level, unsigned int* first)
{
	int i, found = 0, count = 0;

	for (i = total_nr_cpus - 1; i >= 0; i--) {
		if ((load->cpu_load[i] != INVALID_LOAD)
				&& (load->cpu_load[i] < level)
				&& is_cpu_little(i))
		{
			if (first && (!found)) {
				*first = i;
				found = 1;
			}
			count++;
		}
	}

	return count;
}

int get_cpus_online(struct auto_cpu_hotplug_loadinfo *load,
						int *little, int *big)
{
	int i, big_count = 0, little_count = 0;

	for (i = total_nr_cpus - 1; i >= 0; i--) {
		if ((load->cpu_load[i] != INVALID_LOAD)) {
			if (is_cpu_little(i))
				little_count++;
			else
				big_count++;
		}
	}

	*little = little_count;
	*big = big_count;

	return 1;
}

int try_up_big(void)
{
	unsigned int cpu = 0;
	unsigned int found = total_nr_cpus;

	while (cpu < total_nr_cpus && cpu_possible(cpu)) {
		cpu = cpumask_next_zero(cpu, cpu_online_mask);
		if (is_cpu_big(cpu)) {
			found = cpu;
			break;
		}
	}

	if (found < total_nr_cpus && cpu_possible(found))
		return do_cpu_up(found);
	else
		return 0;
}

int try_up_little(void)
{
	unsigned int cpu = 0;
	unsigned int found = total_nr_cpus;

	while (cpu < total_nr_cpus && cpu_possible(cpu)) {
		cpu = cpumask_next_zero(cpu, cpu_online_mask);
		if (is_cpu_little(cpu)) {
			found = cpu;
			break;
		}
	}

	if (found < total_nr_cpus && cpu_possible(found))
		return do_cpu_up(found);
	else
		return 0;
}

static int try_any_up(void)
{
	unsigned int cpu = 0;

	while (cpu < total_nr_cpus && cpu_possible(cpu)) {
		cpu = cpumask_next_zero(cpu, cpu_online_mask);
		if (cpu < total_nr_cpus && cpu_possible(cpu))
			return do_cpu_up(cpu);
	}

	return 0;
}

static int try_lock_up(int hotplug_lock)
{
	struct cpumask tmp_core_up_mask, tmp_core_down_mask;
	int cpu, lock_flag = hotplug_lock - num_online_cpus();
	unsigned long flags;

	spin_lock_irqsave(&cpumask_lock, flags);
	cpumask_clear(&tmp_core_up_mask);
	cpumask_clear(&tmp_core_down_mask);
	spin_unlock_irqrestore(&cpumask_lock, flags);

	if (lock_flag > 0) {
		for_each_cpu_not(cpu, cpu_online_mask) {
			if (lock_flag-- == 0)
				break;
			spin_lock_irqsave(&cpumask_lock, flags);
			cpumask_or(&tmp_core_up_mask, cpumask_of(cpu), &tmp_core_up_mask);
			spin_unlock_irqrestore(&cpumask_lock, flags);
		}
	} else if (lock_flag < 0) {
		lock_flag = -lock_flag;
		for (cpu = 7; cpu > 0; cpu--) {
			if (cpumask_test_cpu(cpu, cpu_online_mask)) {
				if (lock_flag-- == 0)
					break;
				spin_lock_irqsave(&cpumask_lock, flags);
				cpumask_or(&tmp_core_down_mask, cpumask_of(cpu),
					&tmp_core_down_mask);
				spin_unlock_irqrestore(&cpumask_lock, flags);
			}
		}
	}

	if (!cpumask_empty(&tmp_core_up_mask)) {
		for_each_cpu(cpu, &tmp_core_up_mask) {
			if (!cpu_online(cpu)) {
				return do_cpu_up(cpu);
			}
		}
	} else if (!cpumask_empty(&tmp_core_down_mask)) {
		for_each_cpu(cpu, &tmp_core_down_mask) {
			if (cpu_online(cpu)) {
				cpu_down(cpu);
			}
		}
	}

	return 0;
}

#ifdef CONFIG_CPU_FREQ_GOV_AUTO_HOTPLUG_ROOMAGE
static int get_any_offline_cpu(const cpumask_t *mask)
{
	int cpu, lastcpu = 0xffff;

	for_each_cpu(cpu, mask) {
		if ((cpu != 0) && !cpu_online(cpu))
			return cpu;
	}

	return lastcpu;
}
static int get_any_online_cpu(const cpumask_t *mask)
{
	int cpu, lastcpu = 0xffff;

	for_each_cpu(cpu, mask) {
		if ((cpu != 0) && cpu_online(cpu)) {
			if (lastcpu == 0xffff)
				lastcpu = cpu;
			else if (cpu >lastcpu)
				lastcpu = cpu;
		}
	}

	return lastcpu;
}

static int autohotplug_tryroom(void)
{
	unsigned int to_down, to_up;
	int i, c0_online = 0, c1_online = 0;
    struct cpumask* c0_mask;
    struct cpumask* c1_mask;

    if(cpumask_test_cpu(0,&hmp_slow_cpu_mask))
    {
        c0_mask=&hmp_slow_cpu_mask;
        c1_mask=&hmp_fast_cpu_mask;
    }
    else
    {
        c0_mask=&hmp_fast_cpu_mask;
        c1_mask=&hmp_slow_cpu_mask;
    }
	for_each_online_cpu(i) {
		if (cpumask_test_cpu(i, c0_mask))
			c0_online++;
		else if (cpumask_test_cpu(i, c1_mask))
			c1_online++;
	}

	while (c1_online > c1_max) {
		to_down = get_any_online_cpu(c1_mask);
		do_cpu_down(to_down);
		c1_online--;
	}

	while (c0_online > c0_max) {
		to_down = get_any_online_cpu(c0_mask);
		do_cpu_down(to_down);
		c0_online--;
	}

	while (c1_online < c1_min) {
		to_up = get_any_offline_cpu(c1_mask);
		do_cpu_up(to_up);
		c1_online++;
	}

	while (c0_online < c0_min) {
		to_up = get_any_offline_cpu(c0_mask);
		do_cpu_up(to_up);
		c0_online++;
	}

	return 1;
}

int autohotplug_update_room(unsigned int c0min, unsigned int c1min,
							unsigned int c0max, unsigned int c1max)
{
	mutex_lock(&hotplug_enable_mutex);
	c0_min = c0min;
	c1_min = c1min;
	c0_max = c0max;
	c1_max = c1max;
	mutex_unlock(&hotplug_enable_mutex);

	return 0;
}
EXPORT_SYMBOL(autohotplug_update_room);
#endif

static int autohotplug_task(void *data)
{
	int i, hotplug_lock, try_attemp = 0; // 0: no success 1: up success 2: down success
	unsigned long flags;
	struct auto_cpu_hotplug_loadinfo load;

	set_freezable();
	while (1) {
		if (freezing(current)) {
			if (try_to_freeze())
				continue;
		}

		if (hotplug_enable) {
			try_attemp = 0;
			spin_lock_irqsave(&hotplug_load_lock, flags);
			memcpy(&load, &governor.load, sizeof(load));
			spin_unlock_irqrestore(&hotplug_load_lock, flags);

			mutex_lock(&hotplug_enable_mutex);
			load.max_load = 0;
			load.min_load = 255;
			load.max_cpu = INVALID_CPU;
			load.min_cpu = INVALID_CPU;

			for (i = total_nr_cpus - 1; i>= 0; i--) {
				if (!cpu_online(i))
					load.cpu_load[i] = INVALID_LOAD;

				if ((load.cpu_load[i] != INVALID_LOAD)
						&& (load.cpu_load[i] >= load.max_load))
				{
					load.max_load =load.cpu_load[i];
					load.max_cpu  = i;
				}

				if ((load.cpu_load[i] != INVALID_LOAD)
						&& (load.cpu_load[i] < load.min_load))
				{
					load.min_load =load.cpu_load[i];
					load.min_cpu  = i;
				}
			}

			if (boost_all_online) {
				try_any_up();
			} else {
				hotplug_lock = atomic_read(&g_hotplug_lock);
				/* check if current is in hotplug lock */
				if (hotplug_lock) {
					try_lock_up(hotplug_lock);
				} else {
					if (cur_governor->try_up(&load)) {
						try_attemp = 1;
						if (cur_governor->update_limits)
							cur_governor->update_limits();
					} else {
						if (time_after_eq(jiffies, cpu_up_lasttime
								+ usecs_to_jiffies(hotplug_up_attempt_hold_us)))
						{
							if (cur_governor->try_down(&load)) {
								try_attemp = 2;
								if (cur_governor->update_limits)
									cur_governor->update_limits();
							}
						}
					}
				}
			}

#ifdef CONFIG_CPU_FREQ_GOV_AUTO_HOTPLUG_ROOMAGE
			if (!try_attemp)
			   autohotplug_tryroom();
#endif
			mutex_unlock(&hotplug_enable_mutex);
		}

		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		if (kthread_should_stop())
			break;
		set_current_state(TASK_RUNNING);
	}

	return 0;
}

static unsigned int autohotplug_updateload(int cpu)
{
	struct auto_cpu_hotplug_cpuinfo *pcpu = &per_cpu(cpuinfo, cpu);
	u64 now, now_idle, delta_idle, delta_time, active_time, load;

	now_idle = get_cpu_idle_time(cpu, &now, 1);
	delta_idle = now_idle - pcpu->time_in_idle;
	delta_time = now - pcpu->time_in_idle_timestamp;

	if (delta_time <= delta_idle)
		active_time = 0;
	else
		active_time = delta_time - delta_idle;

	pcpu->time_in_idle = now_idle;
	pcpu->time_in_idle_timestamp = now;

	load = active_time * 100;
	do_div(load, delta_time);

	return ((unsigned int)load);
}

static void autohotplug_governor_updateload(int cpu, unsigned int load,
							unsigned int target, struct cpufreq_policy *policy)
{

	unsigned long flags;
	unsigned int cur = target;

	if (cur) {
		spin_lock_irqsave(&hotplug_load_lock, flags);
		set_cpu_load(cpu, (load * cur) / policy->max);
		if (is_cpu_big(cpu)) {
			governor.load.big_min_load = load_last_big_min_freq * 100 / policy->max;
		}
		spin_unlock_irqrestore(&hotplug_load_lock, flags);
	}

}
static void autohotplug_sample_timer(unsigned long data)
{
	unsigned int i, load;
	struct cpufreq_policy policy;
	struct auto_cpu_hotplug_cpuinfo *pcpu;
	unsigned long flags, expires;
	static const char performance_governor[] = "performance";
	static const char powersave_governor[] = "powersave";

	for_each_possible_cpu(i) {
		if ((cpufreq_get_policy(&policy, i) == 0)
				&& policy.governor->name
				&& !(!strncmp(policy.governor->name, performance_governor,
						strlen(performance_governor))
					|| !strncmp(policy.governor->name, powersave_governor,
								strlen(powersave_governor))
					)
			)
		{
			pcpu = &per_cpu(cpuinfo, i);
			spin_lock_irqsave(&pcpu->load_lock, flags);
			load = autohotplug_updateload(i);
			autohotplug_governor_updateload(i, load,
							(policy.cur ? policy.cur : policy.min), &policy);
			spin_unlock_irqrestore(&pcpu->load_lock, flags);
		}
		else
#ifdef CONFIG_CPU_GOV_AUTO_HOTPLUG_WITHOUT_POLICY
		{
			policy.cur=1000000;
			policy.max=1000000;
			pcpu = &per_cpu(cpuinfo, i);
			spin_lock_irqsave(&pcpu->load_lock, flags);
			load = autohotplug_updateload(i);
			autohotplug_governor_updateload(i, load,
							(policy.cur ? policy.cur : policy.min), &policy);
			spin_unlock_irqrestore(&pcpu->load_lock, flags);
		}
#else
		set_cpu_load(i, INVALID_LOAD);
#endif
	}

	if (!timer_pending(&hotplug_sample_timer)) {
		expires = jiffies + usecs_to_jiffies(hotplug_sample_us);
		mod_timer_pinned(&hotplug_sample_timer, expires);
	}
	return;
}

static void autohotplug_task_timer(unsigned long data)
{
	unsigned long expires;

	if (hotplug_enable)
		wake_up_process(auto_hotplug_task);

	if (!timer_pending(&hotplug_task_timer)) {
		expires = jiffies + usecs_to_jiffies(hotplug_period_us);
		mod_timer_pinned(&hotplug_task_timer, expires);
	}
}

static int autohotplug_timer_start(void)
{
	int i;

	mutex_lock(&hotplug_enable_mutex);

	/* init sample timer */
	init_timer_deferrable(&hotplug_sample_timer);
	hotplug_sample_timer.function  = autohotplug_sample_timer;
	hotplug_sample_timer.data = (unsigned long)&governor.load;
	hotplug_sample_timer.expires = jiffies + usecs_to_jiffies(hotplug_sample_us);
	add_timer_on(&hotplug_sample_timer, 0);

	/* init task timer */
	for (i = total_nr_cpus - 1; i >= 0; i--) {
		set_cpu_load(i, INVALID_LOAD);
		cpu_in_sleep[i] = 0;
		cpu_sleep_lasttime[i] = i ? 0 : jiffies;
	}

	cpu_up_lasttime = jiffies;
	cpu_boost_lasttime = jiffies;

	/* init hotplug timer */
	init_timer(&hotplug_task_timer);
	hotplug_task_timer.function  = autohotplug_task_timer;
	hotplug_task_timer.data = (unsigned long)&governor.load;
	hotplug_task_timer.expires = jiffies + usecs_to_jiffies(hotplug_period_us);
	add_timer_on(&hotplug_task_timer, 0);

	mutex_unlock(&hotplug_enable_mutex);
	return 0;
}

static int autohotplug_timer_stop(void)
{
	mutex_lock(&hotplug_enable_mutex);
	del_timer_sync(&hotplug_task_timer);
	del_timer_sync(&hotplug_sample_timer);
	mutex_unlock(&hotplug_enable_mutex);

	return 0;
}

int hotplug_cpu_lock(int num_core)
{
	int prev_lock;

	mutex_lock(&hotplug_enable_mutex);
	if (num_core < 1 || num_core > num_possible_cpus()) {
		mutex_unlock(&hotplug_enable_mutex);
		return -EINVAL;
	}

	prev_lock = atomic_read(&g_hotplug_lock);
	if (prev_lock != 0 && prev_lock < num_core) {
		mutex_unlock(&hotplug_enable_mutex);
		return -EINVAL;
	}

	atomic_set(&g_hotplug_lock, num_core);
	wake_up_process(auto_hotplug_task);
	mutex_unlock(&hotplug_enable_mutex);

	return 0;
}

int hotplug_cpu_unlock(int num_core)
{
	int prev_lock = atomic_read(&g_hotplug_lock);

	mutex_lock(&hotplug_enable_mutex);

	if (prev_lock != num_core) {
		mutex_unlock(&hotplug_enable_mutex);
		return -EINVAL;
	}

	atomic_set(&g_hotplug_lock, 0);
	mutex_unlock(&hotplug_enable_mutex);

	return 0;
}

unsigned int hotplug_enable_from_sysfs(unsigned int temp, unsigned int *value)
{
	int prev_lock;

	if (temp  && (!hotplug_enable)) {
		autohotplug_timer_start();
	} else if (!temp && hotplug_enable) {
		prev_lock = atomic_read(&hotplug_lock);
		if (prev_lock)
			hotplug_cpu_unlock(prev_lock);

		atomic_set(&hotplug_lock, 0);
		autohotplug_timer_stop();
	}

	return temp;
}

unsigned int cpu_sleep_to_sysfs(unsigned int temp, unsigned int *value)
{
	unsigned int index = ((unsigned int)value
							- (unsigned int)cpu_in_sleep) / sizeof(unsigned int);

	if (!index)
		return (jiffies - cpu_sleep_lasttime[index]);

	if (cpu_online(index))
		return cpu_in_sleep[index];
	else
		return (cpu_in_sleep[index] + (jiffies - cpu_sleep_lasttime[index]));
}

unsigned int hotplug_lock_from_sysfs(unsigned int temp, unsigned int *value)
{
	int ret, prev_lock;

	if ((!hotplug_enable) || temp > NR_CPUS) {
		return atomic_read(&g_hotplug_lock);
	}

	prev_lock = atomic_read(&hotplug_lock);
	if (prev_lock)
		hotplug_cpu_unlock(prev_lock);

	if (temp == 0) {
		atomic_set(&hotplug_lock, 0);
		return 0;
	}

	ret = hotplug_cpu_lock(temp);
	if (ret) {
		printk(KERN_ERR "[HOTPLUG] already locked with smaller value %d < %d\n",
			atomic_read(&g_hotplug_lock), temp);
		return ret;
	}

	atomic_set(&hotplug_lock, temp);

	return temp;
}

unsigned int hotplug_lock_to_sysfs(unsigned int temp, unsigned int *value)
{
	return atomic_read(&hotplug_lock);
}

static ssize_t autohotplug_show(struct kobject *kobj,
		struct attribute *attr, char *buf)
{
	ssize_t ret = 0;
	struct auto_hotplug_global_attr *auto_attr =
		container_of(attr, struct auto_hotplug_global_attr, attr);
	unsigned int temp = *(auto_attr->value);

	if (auto_attr->to_sysfs != NULL)
		temp = auto_attr->to_sysfs(temp, auto_attr->value);

	ret = sprintf(buf, "%u\n", temp);
	return ret;
}

static ssize_t autohotplug_store(struct kobject *a, struct attribute *attr,
		const char *buf, size_t count)
{
	unsigned int temp;
	ssize_t ret = count;
	struct auto_hotplug_global_attr *auto_attr =
		container_of(attr, struct auto_hotplug_global_attr, attr);
	char *str = vmalloc(count + 1);

	if (str == NULL)
		return -ENOMEM;

	memcpy(str, buf, count);
	str[count] = 0;
	if (sscanf(str, "%u", &temp) < 1) {
		ret = -EINVAL;
	} else {
		if (auto_attr->from_sysfs != NULL)
			temp = auto_attr->from_sysfs(temp, auto_attr->value);
		if (temp < 0)
			ret = -EINVAL;
		else
			*(auto_attr->value) = temp;
	}

	vfree(str);
	return ret;
}

void autohotplug_attr_add(const char *name, unsigned int *value, umode_t mode,
		unsigned int (*to_sysfs)(unsigned int, unsigned int *),
		unsigned int (*from_sysfs)(unsigned int ,unsigned int*))
{
	int i = 0;

	while (auto_hotplug_data.attributes[i] != NULL) {
		i++;
		if (i >= HOTPLUG_DATA_SYSFS_MAX)
			return;
	}

	auto_hotplug_data.attr[i].attr.mode = mode;
	auto_hotplug_data.attr[i].show = autohotplug_show;
	auto_hotplug_data.attr[i].store = autohotplug_store;
	auto_hotplug_data.attr[i].attr.name = name;
	auto_hotplug_data.attr[i].value = value;
	auto_hotplug_data.attr[i].to_sysfs = to_sysfs;
	auto_hotplug_data.attr[i].from_sysfs = from_sysfs;
	auto_hotplug_data.attributes[i] = &auto_hotplug_data.attr[i].attr;
	auto_hotplug_data.attributes[i + 1] = NULL;
}

static int __cpuinit autohotplug_cpu_callback(struct notifier_block *nfb,
		unsigned long action, void *hcpu)
{
	unsigned long flags;
	unsigned int cpu = (unsigned long)hcpu;
	struct device *dev;

	dev = get_cpu_device(cpu);
	if (dev) {
		switch (action) {
			case CPU_ONLINE:
				spin_lock_irqsave(&hotplug_load_lock, flags);
				set_cpu_load(cpu, INVALID_LOAD);
				if (cpu_sleep_lasttime[cpu]) {
					cpu_in_sleep[cpu] += jiffies - cpu_sleep_lasttime[cpu];
					cpu_sleep_lasttime[cpu] = 0;
				}
				spin_unlock_irqrestore(&hotplug_load_lock, flags);
				break;
			case CPU_DOWN_PREPARE:
			case CPU_UP_CANCELED_FROZEN:
			case CPU_DOWN_FAILED:
				spin_lock_irqsave(&hotplug_load_lock, flags);
				set_cpu_load(cpu, INVALID_LOAD);
				spin_unlock_irqrestore(&hotplug_load_lock, flags);
				break;
			case CPU_DEAD:
				spin_lock_irqsave(&hotplug_load_lock, flags);
				cpu_sleep_lasttime[cpu] = jiffies;
				spin_unlock_irqrestore(&hotplug_load_lock, flags);
				break;
			default:
				break;
		}
	}
	return NOTIFY_OK;
}

static struct notifier_block __refdata hotplug_cpu_notifier = {
	.notifier_call = autohotplug_cpu_callback,
};

#ifdef CONFIG_DEBUG_FS
static struct dentry *autohotplug_loaddbg_root;
static char autohotplug_load_info[256];

static int autohotplug_loaddbg_open(struct inode *inode,
							struct file *file)
{
	return 0;
}
static int autohotplug_loaddbg_release(struct inode *inode,
							struct file *file)
{
	return 0;
}
static ssize_t autohotplug_loaddbg_write(struct file *file,
							const char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}
static ssize_t autohotplug_loaddbg_read(struct file *file,
							char __user *buf, size_t count, loff_t *ppos)
{
	int i, len;

	for_each_possible_cpu(i) {
		sprintf(autohotplug_load_info + i * 5,"%4d ", governor.load.cpu_load[i]);
	}

	len = strlen(autohotplug_load_info);
	autohotplug_load_info[len] = 0x0A;
	autohotplug_load_info[len + 1] = 0x0;

	len = strlen(autohotplug_load_info);
	if (len) {
		if (*ppos >=len)
			return 0;
		if (count >=len)
			count = len;
		if (count > (len - *ppos))
			count = (len - *ppos);
		if (copy_to_user((void __user *)buf,
				(const void *)autohotplug_load_info, (unsigned long)len))
			return 0;
		*ppos += count;
	} else {
		count = 0;
	}

	return count;
}

static const struct file_operations loaddbg_ops = {
	.write      = autohotplug_loaddbg_write,
	.read       = autohotplug_loaddbg_read,
	.open       = autohotplug_loaddbg_open,
	.release    = autohotplug_loaddbg_release,
};

int autohotplug_debug_init(void)
{
	autohotplug_loaddbg_root = debugfs_create_dir("hotplug", NULL);

	if (debugfs_create_file("load", 0444, autohotplug_loaddbg_root,
			NULL, &loaddbg_ops))
		return 0;

	debugfs_remove_recursive(autohotplug_loaddbg_root);
	autohotplug_loaddbg_root = NULL;

	return -ENOENT;
}
#endif /* CONFIG_DEBUG_FS */

static int autohotplug_attr_init(void)
{
	int i;

	memset(&auto_hotplug_data, sizeof(auto_hotplug_data), 0);

	for_each_possible_cpu(i)
		autohotplug_attr_add(cpu_state_sysfs[i], (unsigned int*)&cpu_in_sleep[i],
							0444, cpu_sleep_to_sysfs, NULL);

	autohotplug_attr_add("enable",          &hotplug_enable,             0644,
							NULL, hotplug_enable_from_sysfs);
	autohotplug_attr_add("boost_all",       &boost_all_online,           0644,
							NULL, NULL);
	autohotplug_attr_add("timer_task_us",   &hotplug_period_us,          0644,
							NULL, NULL);
	autohotplug_attr_add("try_cpuup_level", &load_try_up,                0644,
							NULL, NULL);
	autohotplug_attr_add("try_boost_level", &load_try_boost,             0644,
							NULL, NULL);
	autohotplug_attr_add("try_cpudn_level", &load_try_down,              0644,
							NULL, NULL);
	autohotplug_attr_add("save_all_up",     &load_save_up,               0644,
							NULL, NULL);
	autohotplug_attr_add("hold_atept_us",   &hotplug_up_attempt_hold_us, 0644,
							NULL, NULL);
	autohotplug_attr_add("hold_cpuup_us",   &hotplug_up_cpu_hold_us,     0644,
							NULL, NULL);
	autohotplug_attr_add("hold_boost_us",   &hotplug_boost_hold_us,      0644,
							NULL, NULL);
	autohotplug_attr_add("stable_tryup_us", &load_up_stable_us,          0644,
							NULL, NULL);
	autohotplug_attr_add("stable_boost_us", &load_boost_stable_us,       0644,
							NULL, NULL);
	autohotplug_attr_add("stable_tdown_us", &load_down_stable_us,        0644,
							NULL, NULL);
	autohotplug_attr_add("lock", (unsigned int *)&hotplug_lock,          0644,
							hotplug_lock_to_sysfs, hotplug_lock_from_sysfs);

	/* init governor attr */
	if (cur_governor->init_attr)
		cur_governor->init_attr();

	auto_hotplug_data.attr_group.name = "autohotplug";
	auto_hotplug_data.attr_group.attrs = auto_hotplug_data.attributes;

	return sysfs_create_group(kernel_kobj, &auto_hotplug_data.attr_group);
}

static int reboot_notifier_call(struct notifier_block *this,
						unsigned long code, void *_cmd)
{
    printk("%s:%s: stop autoplug begin\n", __FILE__, __func__);

    // disable auto hotplug
    hotplug_enable_from_sysfs(0, NULL);

    printk("%s:%s: stop autoplug done\n", __FILE__, __func__);
    return NOTIFY_DONE;
}

static struct notifier_block reboot_notifier = {
	.notifier_call	= reboot_notifier_call,
	// autohotplug notifier must be invoked before cpufreq notifier
	.priority	= 1,
};

int autohotplug_init(void)
{
	int cpu;
	struct auto_cpu_hotplug_cpuinfo *pcpu;

	cur_governor = &autohotplug_smart;
	if (cur_governor == NULL) {
		pr_err("autohotplug governor is NULL, failed\n");
		return -EINVAL;
	}

	if (cur_governor->get_fast_and_slow_cpus == NULL) {
		pr_err("get_fast_and_slow_cpus is NULL, failed\n");
		return -EINVAL;
	}

	cur_governor->get_fast_and_slow_cpus(&hmp_fast_cpu_mask, &hmp_slow_cpu_mask);

	/* init per_cpu load_lock */
	for_each_possible_cpu(cpu) {
		pcpu = &per_cpu(cpuinfo, cpu);
		spin_lock_init(&pcpu->load_lock);
		if (cpumask_test_cpu(cpu, &hmp_fast_cpu_mask))
			c1_max++;
		else
			c0_max++;
	}

	mutex_init(&hotplug_enable_mutex);
	spin_lock_init(&hotplug_load_lock);
	spin_lock_init(&cpumask_lock);

	if (hotplug_enable)
		autohotplug_timer_start();

	/* start task */
	auto_hotplug_task = kthread_create(autohotplug_task,
										NULL, "auto_cpu_hotplug");
	if (IS_ERR(auto_hotplug_task))
		return PTR_ERR(auto_hotplug_task);
	get_task_struct(auto_hotplug_task);

	/* attr init */
	autohotplug_attr_init();
	/* register hotcpu notifier */
	register_hotcpu_notifier(&hotplug_cpu_notifier);
#ifdef CONFIG_DEBUG_FS
	autohotplug_debug_init();
#endif

	/* register reboot notifier for process cpus when reboot */
	register_reboot_notifier(&reboot_notifier);

	/* turn hotplug task on*/
	wake_up_process(auto_hotplug_task);

	pr_debug("%s init ok\n", __func__);
	return 0;
}

device_initcall(autohotplug_init);

MODULE_DESCRIPTION("CPU Auto Hotplug");
MODULE_LICENSE("GPL");

/*
 * drivers/cpufreq/autohotplug.h
 *
 * Copyright (c) 2012 Softwinner.
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

#ifndef __AUTOHOTPLUG__
#define __AUTOHOTPLUG__

#define INVALID_LOAD    0xffffffff
#define INVALID_CPU     0xffffffff

#define STABLE_DOWN           0
#define STABLE_UP             1
#define STABLE_BOOST          2
#define STABLE_LAST_BIG       3

#define LOAD_LEVEL_INVALID   -1
#define LOAD_LEVEL_DOWN       0
#define LOAD_LEVEL_NORMAL     1
#define LOAD_LEVEL_MIDDLE     2
#define LOAD_LEVEL_UP         3
#define LOAD_LEVEL_BOOST      4

#if defined(CONFIG_SCHED_HMP)
	#define AUTOHOTPLUG_SUNXI_SYSFS_MAX (3)
#elif defined(CONFIG_SCHED_SMP_DCMP)
	#define AUTOHOTPLUG_SUNXI_SYSFS_MAX (1)
#else
	#define AUTOHOTPLUG_SUNXI_SYSFS_MAX (0)
#endif

#define HOTPLUG_DATA_SYSFS_MAX (14 + AUTOHOTPLUG_SUNXI_SYSFS_MAX + CONFIG_NR_CPUS)

struct auto_cpu_hotplug_loadinfo
{
	unsigned int cpu_load[CONFIG_NR_CPUS];
	unsigned long cpu_load_lasttime[CONFIG_NR_CPUS];
	unsigned int max_load;
	unsigned int min_load;
	unsigned int max_cpu;
	unsigned int min_cpu;
	unsigned int big_min_load;
};

struct auto_cpu_hotplug_governor
{
	struct timer_list cpu_timer;
	struct auto_cpu_hotplug_loadinfo load;
};

struct auto_cpu_hotplug_cpuinfo {
	spinlock_t load_lock; /* protects the next 4 fields */
	u64 time_in_idle;
	u64 time_in_idle_timestamp;
};

struct auto_hotplug_global_attr {
	struct attribute attr;
	ssize_t (*show)(struct kobject *kobj,
			struct attribute *attr, char *buf);
	ssize_t (*store)(struct kobject *a, struct attribute *b,
			const char *c, size_t count);
	unsigned int *value;
	unsigned int (*to_sysfs)(unsigned int, unsigned int *);
	unsigned int  (*from_sysfs)(unsigned int, unsigned int *);
};

struct auto_hotplug_data_struct {
	struct attribute_group attr_group;
	struct attribute *attributes[HOTPLUG_DATA_SYSFS_MAX + 1];
	struct auto_hotplug_global_attr attr[HOTPLUG_DATA_SYSFS_MAX];
};

struct autohotplug_governor {
	void (*init_attr)(void);
	int (*get_fast_and_slow_cpus)(struct cpumask *hmp_fast_cpu_mask,
			struct cpumask *hmp_slow_cpu_mask);
	int (*try_up)(struct auto_cpu_hotplug_loadinfo *load);
	int (*try_down)(struct auto_cpu_hotplug_loadinfo *load);
	void (*update_limits)(void);
};

extern unsigned int is_cpu_big(int cpu);
extern unsigned int is_cpu_little(int cpu);
extern int do_cpu_down(unsigned int cpu);
extern int try_up_little(void);
extern int try_up_big(void);

extern void autohotplug_attr_add(const char *name,
					unsigned int *value, umode_t mode,
					unsigned int (*to_sysfs)(unsigned int, unsigned int *),
					unsigned int (*from_sysfs)(unsigned int ,unsigned int*));
extern int get_cpus_under(struct auto_cpu_hotplug_loadinfo *load,
					unsigned char level, unsigned int *first);
extern int get_bigs_above(struct auto_cpu_hotplug_loadinfo *load,
					unsigned char level, unsigned int *first);
extern int get_littles_under(struct auto_cpu_hotplug_loadinfo* load,
					unsigned char level, unsigned int* first);
extern int get_bigs_under(struct auto_cpu_hotplug_loadinfo *load,
					unsigned char level, unsigned int *first);
extern int get_cpus_stable_under(struct auto_cpu_hotplug_loadinfo *load,
					unsigned char level, unsigned int *first, int is_up);
extern int get_cpus_online(struct auto_cpu_hotplug_loadinfo *load,
					int *little, int *big);


extern struct autohotplug_governor autohotplug_smart;

#endif  /* #ifndef __AUTOHOTPLUG__ */

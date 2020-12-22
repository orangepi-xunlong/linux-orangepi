/*
 *  linux/include/linux/cpu_budget_cooling.h
 *
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef __CPU_BUDGET_COOLING_H__
#define __CPU_BUDGET_COOLING_H__

#include <linux/thermal.h>
#include <linux/cpufreq.h>

#define CPUFREQ_COOLING_START		0
#define CPUFREQ_COOLING_STOP		1


#define BUDGET_GPU_UNTHROTTLE       0
#define BUDGET_GPU_THROTTLE         1
#define NOTIFY_INVALID NULL
#define INVALID_FREQ    (-1)
struct cpu_budget_table
{
    unsigned int online;
    unsigned int cluster0_freq;
    unsigned int cluster0_cpunr;
    unsigned int cluster1_freq;
    unsigned int cluster1_cpunr;
    unsigned int gpu_throttle;
};

struct cpu_budget_cooling_device {
	int id;
	struct thermal_cooling_device *cool_dev;
	unsigned int cpu_budget_state;
	unsigned int cluster0_freq_limit;
	unsigned int cluster0_num_limit;
	unsigned int cluster1_freq_limit;
	unsigned int cluster1_num_limit;
	unsigned int cluster0_freq_roof;
	unsigned int cluster0_num_roof;
	unsigned int cluster1_freq_roof;
	unsigned int cluster1_num_roof;
	unsigned int cluster0_freq_floor;
	unsigned int cluster0_num_floor;
	unsigned int cluster1_freq_floor;
	unsigned int cluster1_num_floor;
	unsigned int gpu_powernow;
    unsigned int gpu_throttle;
	struct cpumask cluster0_cpus;
	struct cpumask cluster1_cpus;
    struct cpu_budget_table * tbl;
    unsigned int tbl_num;
    unsigned int need_takedown;
    struct notifier_block cpufreq_notifer;
    struct notifier_block hotplug_notifer;
	struct list_head node;
    spinlock_t lock;
};
#if defined(CONFIG_CPU_BUDGET_THERMAL)
/**
 * cpu_budget_cooling_register - function to create cpufreq cooling device.
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen
 */
struct thermal_cooling_device *cpu_budget_cooling_register(
    struct cpu_budget_table* tbl,   unsigned int tbl_num,
    const struct cpumask *cluster0_cpus,const struct cpumask *cluster1_cpus);

/**
 * cpu_budget_cooling_unregister - function to remove cpufreq cooling device.
 * @cdev: thermal cooling device pointer.
 */
void cpu_budget_cooling_unregister(struct thermal_cooling_device *cdev);
int register_budget_cooling_notifier(struct notifier_block *nb);
int cpu_budget_update_state(struct cpu_budget_cooling_device *cpu_budget_device);
#else /* !CONFIG_CPUFREQ_HOTPLUG_THERMAL */
static inline struct thermal_cooling_device *cpu_budget_cooling_register(
    struct cpu_budget_table* tbl,   unsigned int tbl_num,
    const struct cpumask *cluster0_cpus,const struct cpumask *cluster1_cpus)

{
	return NULL;
}
static inline void cpu_budget_cooling_unregister(struct thermal_cooling_device *cdev)
{
	return;
}
static inline int register_budget_cooling_notifier(struct notifier_block *nb)
{
    return 0;
}
static inline int cpu_budget_update_state(struct cpu_budget_cooling_device *cpu_budget_device)
{
    return 0;
}
#endif	/* CONFIG_CPU_BUDGET_THERMAL */

#endif /* __CPU_BUDGET_COOLING_H__ */

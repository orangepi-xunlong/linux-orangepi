#ifndef __CPU_COOLING_H__
#define __CPU_COOLING_H__

#include <linux/thermal.h>
#include <linux/cpumask.h>

#ifdef CONFIG_CPU_THERMAL
/**
 * cpufreq_cooling_register - function to create cpufreq cooling device.
 * @clip_cpus: cpumask of cpus where the frequency constraints will happen
 */
struct thermal_cooling_device *
cpufreq_cooling_register(const struct cpumask *clip_cpus);

/**
 * cpufreq_cooling_unregister - function to remove cpufreq cooling device.
 * @cdev: thermal cooling device pointer.
 */
void cpufreq_cooling_unregister(struct thermal_cooling_device *cdev);

#else /* !CONFIG_CPU_THERMAL */
static inline struct thermal_cooling_device *
cpufreq_cooling_register(const struct cpumask *clip_cpus)
{
	return NULL;
}
static inline
void cpufreq_cooling_unregister(struct thermal_cooling_device *cdev)
{
	return;
}

#endif	/* CONFIG_CPU_THERMAL */

#endif /* __CPU_COOLING_H__ */


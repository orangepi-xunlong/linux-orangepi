#include <linux/cpu.h>
#include <linux/cpu_cooling.h>
#include <linux/cpufreq.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/pm_opp.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/thermal.h>
#include <linux/units.h>

struct hotplug_cooling_device {
	unsigned int max_level;
	unsigned int hotplug_state;
	unsigned int cpu;
	struct thermal_cooling_device_ops cooling_ops;
};

static int hotplug_get_max_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct hotplug_cooling_device *hotplug_cdev = cdev->devdata;

	*state = hotplug_cdev->max_level;
	return 0;
}

static int hotplug_get_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long *state)
{
	struct hotplug_cooling_device *hotplug_cdev = cdev->devdata;

	*state = hotplug_cdev->hotplug_state;
	return 0;
}

static int hotplug_set_cur_state(struct thermal_cooling_device *cdev,
				 unsigned long state)
{
	struct hotplug_cooling_device *hotplug_cdev = cdev->devdata;

	/* Request state should be less than max_level */
	if (state > hotplug_cdev->max_level)
		return -EINVAL;

	/* Check if the old cooling action is same as new cooling action */
	if (hotplug_cdev->hotplug_state == state)
		return 0;

	hotplug_cdev->hotplug_state = state;

	/* do some governors ? */
	if (state == 1)
		cpu_device_down(get_cpu_device(hotplug_cdev->cpu));
	else
		cpu_device_up(get_cpu_device(hotplug_cdev->cpu));

	return 0;
}

static struct thermal_cooling_device *
__hotplug_cooling_register(struct device_node *np, unsigned int cpu)
{
	char *name;
	struct device *dev;
	struct thermal_cooling_device *cdev;
	struct hotplug_cooling_device *hotplug_cdev;
	struct thermal_cooling_device_ops *cooling_ops;

	dev = get_cpu_device(cpu);
	if (unlikely(!dev)) {
		pr_warn("No cpu device for cpu %d\n", cpu);
		return ERR_PTR(-ENODEV);
	}

	hotplug_cdev = kzalloc(sizeof(*hotplug_cdev), GFP_KERNEL);
	if (!hotplug_cdev)
		return ERR_PTR(-ENOMEM);

	hotplug_cdev->max_level = 1;
	hotplug_cdev->cpu = cpu;
	cooling_ops = &hotplug_cdev->cooling_ops;
	cooling_ops->get_max_state = hotplug_get_max_state;
	cooling_ops->get_cur_state = hotplug_get_cur_state;
	cooling_ops->set_cur_state = hotplug_set_cur_state;

	cdev = ERR_PTR(-ENOMEM);
	name = kasprintf(GFP_KERNEL, "hotplug-%s", dev_name(dev));

	cdev = thermal_of_cooling_device_register(np, name, hotplug_cdev,
						  cooling_ops);
	kfree(name);

	if (IS_ERR(cdev)) {
		pr_err("%s: Failed to register hotplug cooling device (%p)\n", __func__, PTR_ERR(cdev));
		return PTR_ERR(cdev);
	}

	return cdev;
}

struct thermal_cooling_device **
of_hotplug_cooling_register(struct cpufreq_policy *policy)
{
	unsigned int cpu, num_cpus = 0, cpus = 0;
	struct device_node *np = NULL;
	struct device_node *cooling_node;
	struct thermal_cooling_device **cdev = NULL;

	for_each_cpu(cpu, policy->related_cpus)
		++ num_cpus;

	cdev = kzalloc(sizeof(struct thermal_cooling_device *) * num_cpus, GFP_KERNEL);
	if (!cdev) {
		pr_err("hotplug_cooling: alloc cooling_device failed\n");
		return NULL;
	}

	for_each_cpu(cpu, policy->related_cpus) {
		np = of_get_cpu_node(cpu, NULL);
		if (!np) {
			pr_err("hotplug_cooling: OF node not available for cpu%d\n", cpu);
			return NULL;
		}

		cooling_node = of_get_child_by_name(np, "thermal-hotplug");
		if (of_find_property(cooling_node, "#cooling-cells", NULL)) {
			cdev[cpus] = __hotplug_cooling_register(cooling_node, cpu);
			if (IS_ERR(cdev)) {
				pr_err("hotplug_cooling: cpu%d failed to register as cooling device: %ld\n",
						cpu, PTR_ERR(cdev[cpus]));
				cdev[cpus] = NULL;
			}

			++cpus;
		}

		of_node_put(np);
	}

	return cdev;
}
EXPORT_SYMBOL_GPL(of_hotplug_cooling_register);

void hotplug_cooling_unregister(struct cpufreq_policy *policy, struct thermal_cooling_device **cdev)
{
	unsigned int cpu;
	struct hotplug_cooling_device *hotplug_cdev;

	if (cdev)
		return;

	for_each_cpu(cpu, policy->related_cpus) {
		hotplug_cdev = cdev[cpu]->devdata;
		thermal_cooling_device_unregister(cdev[cpu]);
		kfree(hotplug_cdev);
	}
}
EXPORT_SYMBOL_GPL(hotplug_cooling_unregister);

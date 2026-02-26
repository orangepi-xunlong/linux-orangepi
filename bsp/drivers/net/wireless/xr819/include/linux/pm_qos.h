#ifndef _COMPAT_LINUX_PM_QOS_H
#define _COMPAT_LINUX_PM_QOS_H 1

#include <generated/uapi/linux/version.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0))
#include_next <linux/pm_qos.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))

#ifndef PM_QOS_NETWORK_LATENCY
#define PM_QOS_NETWORK_LATENCY 2
#endif

#define PM_QOS_NETWORK_LAT_DEFAULT_VALUE	(2000 * USEC_PER_SEC)
#define PM_QOS_NETWORK_THROUGHPUT_DEFAULT_VALUE	0
#define PM_QOS_MEMORY_BANDWIDTH_DEFAULT_VALUE	0
//#define PM_QOS_RESUME_LATENCY_DEFAULT_VALUE	0
//#define PM_QOS_LATENCY_ANY			((s32)(~(__u32)0 >> 1))
#define PM_QOS_FLAG_REMOTE_WAKEUP	(1 << 1)

struct xr_miscdevice  {
	int minor;
	const char *name;
	const struct file_operations *fops;
	struct list_head list;
	struct device *parent;
	struct device *this_device;
	const struct attribute_group **groups;
	const char *nodename;
	umode_t mode;
};

struct xr_pm_qos_object {
	struct pm_qos_constraints *constraints;
	struct xr_miscdevice pm_qos_power_miscdev;
	char *name;
};

int xr_pm_qos_request(int pm_qos_class);

int xr_pm_qos_add_notifier(int pm_qos_class,
			struct notifier_block *notifier);

int xr_pm_qos_remove_notifier(int pm_qos_class,
				struct notifier_block *notifier);

#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0)) */

#else
#include <linux/pm_qos_params.h>
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 2, 0)) */

#endif	/* _COMPAT_LINUX_PM_QOS_H */

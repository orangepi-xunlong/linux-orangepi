#ifndef _KY_PLATFORM_PM_OPS_H
#define _KY_PLATFORM_PM_OPS_H

#include <linux/list.h>

struct platfrom_pm_ops {
	struct list_head node;
	int (*prepare_late)(void);
	void (*wake)(void);
#ifdef CONFIG_HIBERNATION
	int (*pre_snapshot)(void);
	void (*finish)(void);
#endif
};

extern void register_platform_pm_ops(struct platfrom_pm_ops *ops);
extern void unregister_platform_pm_ops(struct platfrom_pm_ops *ops);
#ifdef CONFIG_PM_SLEEP
extern int platform_pm_prepare_late_suspend(void);
extern void platform_pm_resume_wake(void);
#endif

#ifdef CONFIG_HIBERNATION
extern void register_platform_hibernate_pm_ops(struct platfrom_pm_ops *ops);
extern void unregister_platform_hibernate_pm_ops(struct platfrom_pm_ops *ops);
extern int platform_hibernate_pm_pre_snapshot(void);
extern void platform_hibernate_pm_finish(void);
#endif

#endif

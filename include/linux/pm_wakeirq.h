/* SPDX-License-Identifier: GPL-2.0-only */
/* pm_wakeirq.h - Device wakeirq helper functions */

#ifndef _LINUX_PM_WAKEIRQ_H
#define _LINUX_PM_WAKEIRQ_H

#ifdef CONFIG_PM

extern int dev_pm_set_wake_irq(struct device *dev, int irq);
extern int dev_pm_set_dedicated_wake_irq(struct device *dev, int irq);
#ifdef CONFIG_SOC_KY_X1
extern int dev_pm_set_dedicated_wake_irq_ky(struct device *dev, int irq, int trigger_tyep);
#endif
extern int dev_pm_set_dedicated_wake_irq_reverse(struct device *dev, int irq);
extern void dev_pm_clear_wake_irq(struct device *dev);

#else	/* !CONFIG_PM */

static inline int dev_pm_set_wake_irq(struct device *dev, int irq)
{
	return 0;
}

static inline int dev_pm_set_dedicated_wake_irq(struct device *dev, int irq)
{
	return 0;
}

#ifdef CONFIG_SOC_KY_X1
static inline int dev_pm_set_dedicated_wake_irq_ky(struct device *dev, int irq)
{
	return 0;
}
#endif

static inline int dev_pm_set_dedicated_wake_irq_reverse(struct device *dev, int irq)
{
	return 0;
}

static inline void dev_pm_clear_wake_irq(struct device *dev)
{
}

#endif	/* CONFIG_PM */
#endif	/* _LINUX_PM_WAKEIRQ_H */

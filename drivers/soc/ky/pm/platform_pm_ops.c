#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/ky/platform_pm_ops.h>

static LIST_HEAD(platform_pm_ops_list);
static DEFINE_MUTEX(platform_pm_ops_lock);

void register_platform_pm_ops(struct platfrom_pm_ops *ops)
{
	mutex_lock(&platform_pm_ops_lock);
	list_add_tail(&ops->node, &platform_pm_ops_list);
	mutex_unlock(&platform_pm_ops_lock);
}
EXPORT_SYMBOL_GPL(register_platform_pm_ops);

void unregister_platform_pm_ops(struct platfrom_pm_ops *ops)
{
	mutex_lock(&platform_pm_ops_lock);
	list_del(&ops->node);
	mutex_unlock(&platform_pm_ops_lock);
}
EXPORT_SYMBOL_GPL(unregister_platform_pm_ops);

#ifdef CONFIG_PM_SLEEP
int platform_pm_prepare_late_suspend(void)
{
	int ret;
	struct platfrom_pm_ops *ops;

	list_for_each_entry_reverse(ops, &platform_pm_ops_list, node)
		if (ops->prepare_late) {
			pm_pr_dbg("Calling %pS\n", ops->prepare_late);
			ret = ops->prepare_late();
			if (ret)
				goto err_out;
		}

	return 0;

 err_out:
	pr_err("PM: Platform late suspend callback %pS failed.\n", ops->prepare_late);

	/* we just stall here now !!!!! */
	while (1);

	return ret;

}
EXPORT_SYMBOL_GPL(platform_pm_prepare_late_suspend);

void platform_pm_resume_wake(void)
{
	struct platfrom_pm_ops *ops;

	list_for_each_entry(ops, &platform_pm_ops_list, node)
		if (ops->wake) {
			pm_pr_dbg("Calling %pS\n", ops->wake);
			ops->wake();
		}
}
EXPORT_SYMBOL_GPL(platform_pm_resume_wake);
#endif /* CONFIG_PM_SLEEP */

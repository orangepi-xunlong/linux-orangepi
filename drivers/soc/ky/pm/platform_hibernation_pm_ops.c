#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <linux/ky/platform_pm_ops.h>

static LIST_HEAD(platform_hibernate_pm_ops_list);
static DEFINE_MUTEX(platform_hibernate_pm_ops_lock);

void register_platform_hibernate_pm_ops(struct platfrom_pm_ops *ops)
{
	mutex_lock(&platform_hibernate_pm_ops_lock);
	list_add_tail(&ops->node, &platform_hibernate_pm_ops_list);
	mutex_unlock(&platform_hibernate_pm_ops_lock);
}
EXPORT_SYMBOL_GPL(register_platform_hibernate_pm_ops);

void unregister_platform_hibernate_pm_ops(struct platfrom_pm_ops *ops)
{
	mutex_lock(&platform_hibernate_pm_ops_lock);
	list_del(&ops->node);
	mutex_unlock(&platform_hibernate_pm_ops_lock);
}
EXPORT_SYMBOL_GPL(unregister_platform_hibernate_pm_ops);

#ifdef CONFIG_PM_SLEEP
int platform_hibernate_pm_pre_snapshot(void)
{
	int ret;
	struct platfrom_pm_ops *ops;

	mutex_lock(&platform_hibernate_pm_ops_lock);

	list_for_each_entry_reverse(ops, &platform_hibernate_pm_ops_list, node)
		if (ops->pre_snapshot) {
			pm_pr_dbg("Calling %pS\n", ops->pre_snapshot);
			ret = ops->pre_snapshot();
			if (ret)
				goto err_out;
		}

	mutex_unlock(&platform_hibernate_pm_ops_lock);

	return 0;

 err_out:
	mutex_unlock(&platform_hibernate_pm_ops_lock);
	pr_err("PM: Platform late suspend callback %pS failed.\n", ops->pre_snapshot);

	/* we just stall here now !!!!! */
	while (1);

	return ret;

}
EXPORT_SYMBOL_GPL(platform_hibernate_pm_pre_snapshot);

void platform_hibernate_pm_finish(void)
{
	struct platfrom_pm_ops *ops;

	mutex_lock(&platform_hibernate_pm_ops_lock);

	list_for_each_entry(ops, &platform_hibernate_pm_ops_list, node)
		if (ops->finish) {
			pm_pr_dbg("Calling %pS\n", ops->finish);
			ops->finish();
		}

	mutex_unlock(&platform_hibernate_pm_ops_lock);
}
EXPORT_SYMBOL_GPL(platform_hibernate_pm_finish);
#endif /* CONFIG_PM_SLEEP */

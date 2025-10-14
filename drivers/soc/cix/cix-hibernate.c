// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */

#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/suspend.h>
#include <trace/events/power.h>
#include <linux/soc/cix/cix_hibernate.h>

static LIST_HEAD(reserve_mem_ops_list);
static DEFINE_MUTEX(reserve_mem_ops_lock);

/**
 * register_reserve_mem_ops - Register a set of Reserved Mem operations.
 * @ops: Reserved Mem operations to register.
 */
void register_reserve_mem_ops(struct hibernate_rmem_ops *ops)
{
	mutex_lock(&reserve_mem_ops_lock);
	ops->priv = NULL;
	if (!ops->name) {
		ops->name = "unknown";
	}
	list_add_tail(&ops->node, &reserve_mem_ops_list);
	mutex_unlock(&reserve_mem_ops_lock);
}
EXPORT_SYMBOL_GPL(register_reserve_mem_ops);

/**
 * unregister_reserve_mem_ops - Unregister a set of Reserved Mem operations.
 * @ops: Reserved Mem operations to unregister.
 */
void unregister_reserve_mem_ops(struct hibernate_rmem_ops *ops)
{
	mutex_lock(&reserve_mem_ops_lock);
	list_del(&ops->node);
	mutex_unlock(&reserve_mem_ops_lock);
}
EXPORT_SYMBOL_GPL(unregister_reserve_mem_ops);

#ifdef CONFIG_PM_SLEEP
/**
 * reserve_mem_suspend - Execute all the registered Reserved Mem suspend callbacks.
 *
 * This function is executed with one CPU on-line and disabled interrupts.
 */
int reserve_mem_suspend(void)
{
	struct hibernate_rmem_ops *ops;
	int ret = 0;
	void *vaddr = NULL;

	trace_suspend_resume(TPS("reserve_mem_suspend"), 0, true);
	pm_pr_dbg("Checking wakeup interrupts\n");

	WARN_ONCE(!irqs_disabled(),
		  "Interrupts enabled before Reserved Mem suspend.\n");

	list_for_each_entry_reverse(ops, &reserve_mem_ops_list, node) {
		/*Backup Reserved Mem*/
		ops->priv = kzalloc(ops->size, GFP_KERNEL);
		if (IS_ERR_OR_NULL(ops->priv)) {
			pr_warn("%s: 0x%llx[0 : 0x%llx] backup fail\n",
				ops->name, ops->paddr, ops->size);
			continue;
		}

		/*callback*/
		if (ops->suspend) {
			pm_pr_dbg("Calling %pS\n", ops->suspend);
			ret = ops->suspend(ops->paddr, ops->size);
			if (ret)
				goto err_out;
			WARN_ONCE(!irqs_disabled(),
				  "Interrupts enabled after %pS\n",
				  ops->suspend);
		}

		pm_pr_dbg("%s reserved mem[0x%llx] save start\n",ops->name, ops->paddr);
		vaddr = ioremap_wc(ops->paddr, ops->size);
		if (IS_ERR_OR_NULL(vaddr)) {
			pm_pr_dbg("%s reserved mem[0x%llx] save fail\n",ops->name, ops->paddr);
			ret = -1;
			goto err_out;
		}
		memcpy(ops->priv, vaddr, ops->size);
		iounmap(vaddr);
	}

	trace_suspend_resume(TPS("reserve_mem_suspend"), 0, false);
	return 0;

err_out:
	pr_err("PM: Reserved Mem suspend callback %pS failed.\n", ops->suspend);

	list_for_each_entry_continue(ops, &reserve_mem_ops_list, node) {
		if (!IS_ERR_OR_NULL(ops->priv)) {
			kfree(ops->priv);
			ops->priv = NULL;
		}
		if (ops->resume)
			ops->resume(ops->paddr, ops->size);
	}

	return ret;
}
EXPORT_SYMBOL_GPL(reserve_mem_suspend);

/**
 * syscore_resume - Execute all the registered system core resume callbacks.
 *
 * This function is executed with one CPU on-line and disabled interrupts.
 */
void reserve_mem_resume(void)
{
	struct hibernate_rmem_ops *ops;
	void *vaddr = NULL;

	trace_suspend_resume(TPS("reserve_mem_resume"), 0, true);
	WARN_ONCE(!irqs_disabled(),
		  "Interrupts enabled before Reserved Mem resume.\n");

	list_for_each_entry(ops, &reserve_mem_ops_list, node) {
		/*restore reserved mem*/
		if (IS_ERR_OR_NULL(ops->priv))
			continue;

		pm_pr_dbg("%s reserved mem[0x%llx] restore start\n",ops->name, ops->paddr);
		vaddr = ioremap_wc(ops->paddr, ops->size);
		if (IS_ERR_OR_NULL(ops->priv)) {
			pm_pr_dbg("%s reserved mem[0x%llx] restore fail\n",ops->name, ops->paddr);
			continue;
		}

		memcpy((void *)vaddr, ops->priv, ops->size);
		kfree(ops->priv);
		iounmap(vaddr);
		ops->priv = NULL;

		/*callback*/
		if (ops->resume) {
			pm_pr_dbg("Calling %pS\n", ops->resume);
			ops->resume(ops->paddr, ops->size);
			WARN_ONCE(!irqs_disabled(),
				  "Interrupts enabled after %pS\n",
				  ops->resume);
		}
	}
	trace_suspend_resume(TPS("reserve_mem_resume"), 0, false);
}
EXPORT_SYMBOL_GPL(reserve_mem_resume);
#endif /* CONFIG_PM_SLEEP */

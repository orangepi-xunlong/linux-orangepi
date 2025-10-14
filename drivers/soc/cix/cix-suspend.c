/*
 * SPDX-License-Identifier: GPL-2.0+
 *
 * Add CIX SKY1 SoC Version driver
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/soc/cix/cix_suspend.h>
#include <dfx/hiview_hisysevent.h>

static struct suspend_info suspend_warn;
static spinlock_t suspend_warn_lock;
unsigned long suspend_warn_flags;

void suspend_warning_set(const char *name, suspend_warn_type_t type)
{
	if (type >= SUSPEND_WARN_TYPE_MAX) {
		pr_err("Invalid suspend warn type: %d\n", type);
		return;
	}

	if (suspend_warn.name) {
		kfree(suspend_warn.name);
		suspend_warn.name = NULL;
	}
	suspend_warn.name = kzalloc(SUSPEND_MAX_MODULE_NAME_LEN, GFP_KERNEL);
	if (!suspend_warn.name) {
		pr_err("Failed to allocate memory for suspend_warn.name\n");
		return;
	}

	spin_lock_irqsave(&suspend_warn_lock, suspend_warn_flags);
	if (!suspend_warn.is_set) {
		suspend_warn.type = type;
		suspend_warn.is_set = true;
		strncpy(suspend_warn.name, name, SUSPEND_MAX_MODULE_NAME_LEN - 1);
	} else {
		pr_warn("Suspend warn already set: type=%d, new type=%d\n", suspend_warn.type, type);
	}
	spin_unlock_irqrestore(&suspend_warn_lock, suspend_warn_flags);
}
EXPORT_SYMBOL_GPL(suspend_warning_set);

void suspend_warning_clear(void)
{
	spin_lock_irqsave(&suspend_warn_lock, suspend_warn_flags);
	if (suspend_warn.is_set) {
		suspend_warn.is_set = false;
		suspend_warn.type = SUSPEND_WARN_TYPE_NONE;
		pr_warn("Suspend warn cleared\n");
	}
	spin_unlock_irqrestore(&suspend_warn_lock, suspend_warn_flags);
	if (suspend_warn.name) {
		kfree(suspend_warn.name);
		suspend_warn.name = NULL;
	}
}
EXPORT_SYMBOL_GPL(suspend_warning_clear);

bool suspend_warning_check(void)
{
	struct hiview_hisysevent *event = NULL;
	int ret = 0;
	bool was_set = false;

	spin_lock_irqsave(&suspend_warn_lock, suspend_warn_flags);
	if (suspend_warn.is_set) {
		was_set = true;

		if(!IS_ERR_OR_NULL(suspend_warn.name)) {
			event = hisysevent_create("KERNEL_VENDOR", "SUSPEND_ERROR", FAULT);
			if (event == NULL) {
				pr_err("create hisysevent failed: %s \n", suspend_warn.name);
				return was_set;
			}
			hisysevent_put_string(event, "MODULE_NAME", suspend_warn.name);
			ret = hisysevent_write(event);
			if (ret < 0)
				pr_err("report hievent failed: error_name = %s \n", suspend_warn.name);
			hisysevent_destroy(&event);
		}

	}
	spin_unlock_irqrestore(&suspend_warn_lock, suspend_warn_flags);

	return was_set;
}
EXPORT_SYMBOL_GPL(suspend_warning_check);

static int __init suspend_debug_init(void)
{
	memset(&suspend_warn, 0, sizeof(suspend_warn));
	pr_info("Suspend debug module loaded\n");

	return 0;
}

static void __exit suspend_debug_exit(void)
{
	pr_info("Suspend debug module unloaded\n");
}

module_init(suspend_debug_init);
module_exit(suspend_debug_exit);

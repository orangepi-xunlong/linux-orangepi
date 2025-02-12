// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>

#include "sysfs_display.h"

struct class *display_class;
EXPORT_SYMBOL_GPL(display_class);

#ifndef MODULE
static int __init display_class_init(void)
#else
int display_class_init(void)
#endif
{
	pr_info("display class register\n");

	display_class = class_create("display");
	if (IS_ERR(display_class)) {
		pr_err("Unable to create display class\n");
		return PTR_ERR(display_class);
	}

	return 0;
}

#ifndef MODULE
postcore_initcall(display_class_init);
#endif

MODULE_DESCRIPTION("Provide display class for hardware driver");
MODULE_LICENSE("GPL v2");


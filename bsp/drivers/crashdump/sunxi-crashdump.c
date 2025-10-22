// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2019-2020 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Allwinner sunxi crash dump debug
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/kmemleak.h>
#include <asm/cacheflush.h>
#include <linux/version.h>
#include <linux/input.h>
#include "sunxi-crashdump.h"
#include "sunxi-crashnote.h"
#include "sunxi-minidump.h"

static int __init sunxi_crashdump_init(void)
{
	if (sunxi_crash_dump2pc_init())
		pr_err("register sunxi crashdump2pc failed.\n");

	if (sunxi_crashdump_key_register())
		pr_err("register sunxi crashdump key failed.\n");

	if (sunxi_md_init())
		pr_err("register sunxi minidump failed.\n");

	crash_info_init();
	return 0;
}

static void __exit sunxi_crashdump_exit(void)
{

	sunxi_crashdump_key_unregister();

	sunxi_crash_dump2pc_exit();

	return;
}
late_initcall(sunxi_crashdump_init);
module_exit(sunxi_crashdump_exit);

MODULE_AUTHOR("cuizhikui<cuizhikui@allwinnertech.com>");
MODULE_AUTHOR("kanghoupeng<kanghoupeng@allwinnertech.com>");
MODULE_DESCRIPTION("sunxi crash dump debug");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.8");
MODULE_IMPORT_NS(MINIDUMP);

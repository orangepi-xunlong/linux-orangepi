// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021 Western Digital Corporation or its affiliates.
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 */

#define pr_fmt(fmt) "suspend: " fmt

#include <linux/ftrace.h>
#include <linux/suspend.h>
#include <asm/csr.h>
#include <asm/sbi.h>
#include <asm/suspend.h>
#include <linux/ky/platform_pm_ops.h>

static int system_hibernate_begin(pm_message_t stage)
{
	return 0;
}

static void system_hibernate_end(void)
{
}

static int system_hibernate_pre_snapshot(void)
{
	platform_hibernate_pm_pre_snapshot();

	return 0;
}

static void system_hibernate_finish(void)
{
	platform_hibernate_pm_finish();
}

static int system_hibernate_prepare(void)
{
	return 0;
}

static int system_hibernate_enter(void)
{
	return 0;
}

static void system_hibernate_leave(void)
{
}

static int system_hibernate_pre_restore(void)
{
	platform_hibernate_pm_pre_snapshot();

	return 0;
}

static void system_hibernate_restore_cleanup(void)
{
}

static void system_hibernate_recover(void)
{
}

static const struct platform_hibernation_ops ky_system_hibernation_ops ={
	.begin = system_hibernate_begin,
	.end = system_hibernate_end,
	.pre_snapshot = system_hibernate_pre_snapshot,
	.finish = system_hibernate_finish,
	.prepare = system_hibernate_prepare,
	.enter = system_hibernate_enter,
	.leave = system_hibernate_leave,
	.pre_restore = system_hibernate_pre_restore,
	.restore_cleanup = system_hibernate_restore_cleanup,
	.recover = system_hibernate_recover,
};

static int __init ky_system_hibernation_init(void)
{
	hibernation_set_ops(&ky_system_hibernation_ops);

	return 0;
}

late_initcall(ky_system_hibernation_init);

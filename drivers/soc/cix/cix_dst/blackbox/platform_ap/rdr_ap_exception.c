
// SPDX-License-Identifier: GPL-2.0-only
/*
 * Stack tracing support
 *
 * Copyright (C) 2025 CIX Ltd.
 */
#include <linux/console.h>
#include <linux/kexec.h>
#include <linux/panic_notifier.h>
#include <linux/kdebug.h>
#include <linux/soc/cix/util.h>
#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/rdr_platform.h>
#include <mntn_public_interface.h>
#include <mntn_subtype_exception.h>
#include "include/rdr_ap_adapter.h"
#include "../rdr_print.h"
#include "../rdr_inner.h"

static struct rdr_exception_info_s g_einfo[] = {
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
			       AP_PANIC, AP_PANIC_RES, "ap", 0, NULL),
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NO, RDR_AP, 0, RDR_AP,
			       AP_PANIC, AP_PANIC_TEST, "ap", RDR_SAVE_DMESG,
			       NULL),
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
			       AP_PANIC, AP_PANIC_SOFTLOCKUP, "softlockup", 0,
			       NULL),
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
			       AP_PANIC, AP_PANIC_STORAGE, "storage", 0, NULL),
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
			       AP_RESUME, AP_RESUME_RES, "resume too slow", 0,
			       NULL),
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
			       AP_PANIC, AP_PANIC_RCUSTALL, "rcu stall", 0,
			       NULL),
	DEF_EXCE_STRUCT_SINGLE(RDR_WARN, RDR_REBOOT_NO, RDR_AP, 0, RDR_AP,
			       AP_SUSPEND, AP_SUSPEND_DEVICE_FAIL, "suspend",
			       RDR_SAVE_DMESG, NULL)
};

/*
 * Description : panic reset hook function
 */
static int ap_panic_notify(struct notifier_block *nb, unsigned long event,
			   void *buf)
{
	BB_PN("===> enter panic notify!\n");

	if (strnstr(buf, "softlockup", strlen(buf)))
		rdr_system_error(MODID_AP_PANIC_SOFTLOCKUP, 0, 0);
	else if (strnstr(buf, "RCU Stall", strlen(buf)))
		rdr_system_error(MODID_AP_PANIC_RCUSTALL, 0, 0);
	else
		rdr_system_error(MODID_AP_PANIC_RES, 0, 0);

	return 0;
}

/*
 * Description : die reset hook function
 */
static int ap_die_notify(struct notifier_block *nb, unsigned long event,
			 void *p_reg)
{
	return 0;
}

static struct notifier_block ap_panic_block = {
	.notifier_call = ap_panic_notify,
	.priority = INT_MIN,
};

static struct notifier_block ap_die_block = {
	.notifier_call = ap_die_notify,
	.priority = INT_MIN,
};

int rdr_exception_init(void)
{
	int ret = 0, i;

	for (i = 0; i < ARRAY_SIZE(g_einfo); i++) {
		BB_DBG("register exception:%u", g_einfo[i].e_exce_type);
		ret = (int)rdr_register_exception(&g_einfo[i]);
		if (ret == 0)
			BB_ERR("rdr_register_exception fail, ret = [%u]\n",
			       ret);
	}

	ret = atomic_notifier_chain_register(&panic_notifier_list,
					     &ap_panic_block);
	panic_on_oops = 1;
	sysctl_panic_on_rcu_stall = 1;
	ret = register_die_notifier(&ap_die_block);
	return ret;
}
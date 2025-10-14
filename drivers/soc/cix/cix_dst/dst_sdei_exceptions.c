// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Cix Technology Group Co., Ltd.*/

#include <linux/arm_sdei.h>
#include <linux/cacheflush.h>
#include <mntn_subtype_exception.h>
#include <linux/soc/cix/rdr_platform.h>
#include <linux/soc/cix/dst_reboot_reason.h>
#include <linux/nmi.h>
#include <linux/sched/debug.h>
#include <linux/console.h>
#include "blackbox/rdr_inner.h"
#include "dst_print.h"

#define CIX_SDEI_WDT_EVENT (100)
#define CIX_TFA_EXCEPTION_EVENT (0xFF)
#define ABNORMAL_RST_FLAG (0xFF)

struct EventArgs {
	u32 event_num;
	void *cb;
	void *arg;
	struct rdr_exception_info_s einfo;
};

#ifdef CONFIG_PLAT_KERNELDUMP
extern void plat_set_cpu_regs(int coreid, struct pt_regs *reg);
#endif
static int plat_sdei_event_callback(u32 event, struct pt_regs *regs, void *arg);

/*
 * struct rdr_exception_info_s {
 *  struct list_head e_list;
 *  u32 e_modid;
 *  u32 e_modid_end;
 *  u32 e_process_priority;
 *  u32 e_reboot_priority;
 *  u64 e_notify_core_mask;
 *  u64 e_reset_core_mask;
 *  u64 e_from_core;
 *  u32 e_reentrant;
 *  u32 e_exce_type;
 *  u32 e_upload_flag;
 *  u8  e_from_module[MODULE_NAME_LEN];
 *  u8  e_desc[STR_EXCEPTIONDESC_MAXLEN];
 *  u32 e_reserve_u32;
 *  void*   e_reserve_p;
 *  rdr_e_callback e_callback;
 * };
 */
static struct EventArgs g_sdei_events[] = {
	{ CIX_SDEI_WDT_EVENT, plat_sdei_event_callback, NULL,
	  DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP,
				 RDR_AP, AP_AWDT, AP_AWDT_BL31, "ap wdt", 0,
				 NULL) },
	{ CIX_TFA_EXCEPTION_EVENT, plat_sdei_event_callback, NULL,
	  DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP,
				 RDR_AP, BL31_PANIC, BL31_PANIC_RES, "ap bl31",
				 0, NULL) },
};

static int init_plat_sdei_event(u32 event_num, void *cb, void *arg)
{
	int err;

	err = sdei_event_register(event_num, cb, arg);
	if (!err) {
		err = sdei_event_enable(event_num);
		if (!err) {
			DST_PN("event%d registered & enabled ...\n", event_num);
			return -1;
		}
	} else {
		DST_ERR("event%d register failed ...\n", event_num);
		return -1;
	}

	return 0;
}

static void init_rdr_sdei_events(void)
{
	unsigned int i;
	u32 ret;

	DST_PR_START();
	for (i = 0; i < ARRAY_SIZE(g_sdei_events); i++) {
		DST_DBG("register exception:%u",
			g_sdei_events[i].einfo.e_exce_type);
		ret = rdr_register_exception(&g_sdei_events[i].einfo);
		if (ret == 0) {
			DST_ERR("rdr_register_exception fail, ret = [%u]\n",
				ret);
			continue;
		}
		init_plat_sdei_event(g_sdei_events[i].event_num,
				     g_sdei_events[i].cb, g_sdei_events[i].arg);
	}
	DST_PR_END();
}

int plat_sdei_event_callback(u32 event, struct pt_regs *regs, void *arg)
{
	unsigned int i;
	struct EventArgs *evtargs = NULL;

	DST_PR_START();
	bust_spinlocks(1); /* bust_spinlocks is open */

	for (i = 0; i < sizeof(g_sdei_events) / sizeof(struct EventArgs); i++) {
		if (event == g_sdei_events[i].event_num) {
			evtargs = &g_sdei_events[i];
		}
	}

	if (!evtargs) {
		DST_ERR("event%d not found ...\n", event);
		bust_spinlocks(0);
		return 0;
	}

#ifdef CONFIG_PLAT_KERNELDUMP
	plat_set_cpu_regs(raw_smp_processor_id(), regs);
#endif
	trigger_allbutcpu_cpu_backtrace(smp_processor_id());
	smp_send_stop();

	rdr_system_error(evtargs->einfo.e_modid, 0, 0);

	/*If the rdr is not initialized and the AP is not reset, then restart the AP.*/
	machine_restart(
		rdr_get_exception_type_name(evtargs->einfo.e_exce_type));
	return 0;
}

static int dst_wdt_sdei_init(void)
{
	init_rdr_sdei_events();
	return 0;
}

late_initcall(dst_wdt_sdei_init);

static int __init plat_sdei_init(void)
{
	sdei_init();
	return 0;
}

subsys_initcall_sync(plat_sdei_init);

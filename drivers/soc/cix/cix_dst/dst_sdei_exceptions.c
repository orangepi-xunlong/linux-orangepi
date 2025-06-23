#include <linux/arm_sdei.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/smp.h>
#include <linux/kmsg_dump.h>
#include <linux/blkdev.h>
#include <linux/io.h>
#include <asm/stacktrace.h>
#include <asm/exception.h>
#include <asm/system_misc.h>
#include <asm/cacheflush.h>
#include <mntn_subtype_exception.h>
#include <mntn_public_interface.h>
#include "blackbox/rdr_inner.h"
#include "blackbox/rdr_field.h"
#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/rdr_platform.h>
#include <linux/version.h>
#include <linux/soc/cix/dst_reboot_reason.h>
#include <linux/nmi.h>
#include <linux/sched/debug.h>
#include <linux/console.h>
#include "dst_print.h"

#define CIX_SDEI_WDT_EVENT			(100)
#define CIX_TFA_EXCEPTION_EVENT		(0xFF)
#define ABNORMAL_RST_FLAG			(0xFF)

struct EventArgs
{
	u32 event_num;
	void *cb;
	void *arg;
	struct rdr_exception_info_s einfo;
};

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
		{ { 0, 0 }, MODID_AP_S_WDT, MODID_AP_S_WDT, RDR_ERR, RDR_REBOOT_NOW,
			RDR_AP, RDR_AP, RDR_AP, (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_AWDT,
			HI_APPANIC_RESERVED, (u32)RDR_UPLOAD_YES, "ap wdt", "ap wdt", 0, 0, 0 }
	},
	{ CIX_TFA_EXCEPTION_EVENT, plat_sdei_event_callback, NULL,
		{ { 0, 0 }, MODID_AP_S_BL31_PANIC, MODID_AP_S_BL31_PANIC, RDR_ERR, RDR_REBOOT_NOW,
			RDR_AP, RDR_AP, RDR_AP, (u32)RDR_REENTRANT_DISALLOW, (u32)AP_S_BL31_PANIC,
			HI_APPANIC_RESERVED, (u32)RDR_UPLOAD_YES, "ap bl31", "ap bl31", 0, 0, 0 },
	},
};

static int init_plat_sdei_event(u32 event_num, void *cb, void *arg)
{
	int err;

	err = sdei_event_register(event_num, cb, arg);
	if (!err) {
		err = sdei_event_enable(event_num);
		if(!err) {
			pr_info("%s,%d: event%d registered & enabled ...\n",
				__func__, __LINE__, event_num);
			return -1;
		}
	} else {
		pr_err("%s,%d: event%d register failed ...\n",
			__func__, __LINE__, event_num);
		return -1;
	}

	return 0;
}

static void init_rdr_sdei_events(void)
{
	unsigned int i;
	u32 ret;

	DST_PRINT_START();
	for (i = 0; i < sizeof(g_sdei_events) / sizeof(struct EventArgs); i++) {
		DST_PRINT_DBG("register exception:%u", g_sdei_events[i].einfo.e_exce_type);
		ret = rdr_register_exception(&g_sdei_events[i].einfo);
		if (ret == 0) {
			DST_PRINT_ERR("rdr_register_exception fail, ret = [%u]\n", ret);
			continue;
		} else {
			init_plat_sdei_event(g_sdei_events[i].event_num, g_sdei_events[i].cb, g_sdei_events[i].arg);
		}
	}
	DST_PRINT_END();
}

int plat_sdei_event_callback(u32 event, struct pt_regs *regs, void *arg)
{
	struct rdr_exception_info_s *p_exce_info = NULL;
	char date[DATATIME_MAXLEN];
	int ret = 0;
	unsigned int reset_reason;
	unsigned int i;
	struct EventArgs* evtargs = NULL;

	DST_PRINT_START();
	bust_spinlocks(1); /* bust_spinlocks is open */

	for (i = 0; i < sizeof(g_sdei_events) / sizeof(struct EventArgs); i++) {
		if (event == g_sdei_events[i].event_num) {
			evtargs = &g_sdei_events[i];
		}
	}

	/*
	* If the system is reset abnormally, the reset may fail. In this case, only the watchdog can be reset.
	* In this case, the previous abnormality records will be overwritten.
	*/
	reset_reason = get_reboot_reason();
	if ((reset_reason == ABNORMAL_RST_FLAG) || (reset_reason == AP_S_AWDT)) {
		set_subtype_exception(HI_APWDT_AP, true);
	}

#ifdef CONFIG_PLAT_PRINTK_EXT
	printk_level_setup(LOGLEVEL_DEBUG);
#endif
	DST_PRINT_PN("%s", linux_banner);
	console_verbose();
	show_regs(regs);
	dump_stack();
	smp_send_stop();

	if (!rdr_init_done()) {
		DST_PRINT_ERR("rdr init faild!\n");
		return -1;
	}

	last_task_stack_dump();
	regs_dump(); /* "sctrl", "pctrl", "peri_crg", "gic" */
	save_module_dump_mem();

	rdr_field_baseinfo_reinit();

	if (!evtargs) {
		DST_PRINT_ERR("trigger unknown event %d\n", event);
	} else {
		rdr_save_args(evtargs->einfo.e_modid, 0, 0);
		p_exce_info = rdr_get_exception_info(evtargs->einfo.e_modid);
		if (p_exce_info != NULL) {
			memset(date, 0, DATATIME_MAXLEN);
			ret = snprintf(date, DATATIME_MAXLEN - 1, "%s-%08lld",
			rdr_get_timestamp(), rdr_get_tick());
			if (unlikely(ret < 0)) {
				DST_PRINT_ERR("snprintf_s ret %d!\n", ret);
				return -1;
			}

			rdr_fill_edata(p_exce_info, date);

			(void)rdr_exception_trace_record(p_exce_info->e_reset_core_mask,
			p_exce_info->e_from_core, p_exce_info->e_exce_type, p_exce_info->e_exce_subtype);
		}

		rdr_hisiap_dump_root_head(evtargs->einfo.e_modid, evtargs->einfo.e_exce_type, evtargs->einfo.e_modid);
	}

	DST_PRINT_PN("%s end\n", __func__);
	trigger_allbutself_cpu_backtrace();

	kmsg_dump(KMSG_DUMP_PANIC);
	console_flush_on_panic(CONSOLE_FLUSH_PENDING);
	machine_restart(NULL);
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

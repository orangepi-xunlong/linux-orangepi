/*
 * drivers/soc/cix/cix_dst/dst_sdei_tee_exceptions.c
 *
 * tee exception
 *
 * Copyright (c) 2012-2020 Cix Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

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

#define CIX_TEE_EXCEPTION_EVENT		(0xFE)
#define ABNORMAL_RST_FLAG			(0xFF)

struct EventArgs
{
	u32 event_num;
	void *cb;
	void *arg;
	struct rdr_exception_info_s einfo;
};

static int plat_sdei_tee_event_callback(u32 event, struct pt_regs *regs, void *arg);

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
static struct EventArgs g_sdei_tee_events[] = {
	{ CIX_TEE_EXCEPTION_EVENT, plat_sdei_tee_event_callback, NULL,
		{ { 0, 0 }, MODID_TEE_ERROR, MODID_TEE_ERROR, RDR_ERR, RDR_REBOOT_NO,
			RDR_AP, RDR_AP, RDR_AP, (u32)RDR_REENTRANT_DISALLOW, (u32)TEE_S_EXCEPTION,
			HI_APPANIC_RESERVED, (u32)RDR_UPLOAD_YES, "ap tee", "ap tee", 0, 0, 0 },
	},
};

static int init_plat_sdei_tee_event(u32 event_num, void *cb, void *arg)
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

static void rdr_tee_dump(u32 modid, u32 etype,
				u64 coreid, char *log_path,
				pfn_cb_dump_done pfn_cb)
{
	pr_err("rdr_tee_dump\n");
}

static u64 g_phy_addr = 0;


static void init_rdr_sdei_tee_events(void)
{
	unsigned int i;
	u32 ret;
	struct rdr_register_module_result retinfo;
	struct rdr_module_ops_pub s_dsp_ops;

	DST_PRINT_START();

	/** 1) Get phy address */
	s_dsp_ops.ops_dump = rdr_tee_dump;
	s_dsp_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(RDR_TEEOS, &s_dsp_ops, &retinfo);
	if (ret < 0) {
		pr_err("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return;
	}

	/** 2) Register phy address */
	g_phy_addr = retinfo.log_addr;
	sdei_api_event_set_info(g_sdei_tee_events[0].event_num, SDEI_EVENT_SET_TEE_MEMORY, retinfo.log_addr);
	sdei_api_event_set_info(g_sdei_tee_events[0].event_num, SDEI_EVENT_SET_TEE_ENABLE, 1);
	pr_err("init_rdr_sdei_tee_events , phy_addr = [0x%llx]\n", retinfo.log_addr);
	for (i = 0; i < sizeof(g_sdei_tee_events) / sizeof(struct EventArgs); i++) {
		DST_PRINT_DBG("register exception:%u", g_sdei_tee_events[i].einfo.e_exce_type);
		ret = rdr_register_exception(&g_sdei_tee_events[i].einfo);
		if (ret == 0) {
			DST_PRINT_ERR("rdr_register_exception fail, ret = [%u]\n", ret);
			continue;
		} else {
			init_plat_sdei_tee_event(g_sdei_tee_events[i].event_num, g_sdei_tee_events[i].cb, g_sdei_tee_events[i].arg);
		}
	}
	DST_PRINT_END();
}


int plat_sdei_tee_event_callback(u32 event, struct pt_regs *regs, void *arg)
{
	struct rdr_exception_info_s *p_exce_info = NULL;
	char date[DATATIME_MAXLEN];
	int ret = 0;
	unsigned int i;
	struct EventArgs* evtargs = NULL;

	DST_PRINT_START();
	bust_spinlocks(1); /* bust_spinlocks is open */

	for (i = 0; i < sizeof(g_sdei_tee_events) / sizeof(struct EventArgs); i++) {
		if (event == g_sdei_tee_events[i].event_num) {
			evtargs = &g_sdei_tee_events[i];
		}
	}

#ifdef CONFIG_PRINTK_EXTENSION
	printk_level_setup(LOGLEVEL_DEBUG);
#endif
	DST_PRINT_PN("%s", linux_banner);
	console_verbose();

	if (!rdr_init_done()) {
		DST_PRINT_ERR("rdr init faild!\n");
		return -1;
	}

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

static int dst_tee_sdei_init(void)
{
	init_rdr_sdei_tee_events();
	return 0;
}

late_initcall(dst_tee_sdei_init);

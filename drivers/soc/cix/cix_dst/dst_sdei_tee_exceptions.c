// SPDX-License-Identifier: GPL-2.0
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
#include <linux/cacheflush.h>
#include <mntn_subtype_exception.h>
#include <linux/soc/cix/rdr_platform.h>
#include <linux/nmi.h>
#include <linux/console.h>
#include "blackbox/rdr_inner.h"
#include "dst_print.h"

#define CIX_TEE_EXCEPTION_EVENT (0xFE)
#define ABNORMAL_RST_FLAG (0xFF)
#define DST_SET_TEE_MEMORY 3

struct EventArgs {
	u32 event_num;
	void *cb;
	void *arg;
	struct rdr_exception_info_s einfo;
};

#ifdef CONFIG_PLAT_KERNELDUMP
extern void plat_set_cpu_regs(int coreid, struct pt_regs *reg);
#endif
static int plat_sdei_tee_event_callback(u32 event, struct pt_regs *regs,
					void *arg);

static struct EventArgs g_sdei_tee_events[] = {
	{ CIX_TEE_EXCEPTION_EVENT, plat_sdei_tee_event_callback, NULL,
	  DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP,
				 RDR_AP, TEE_EXCEPTION, TEE_EXCEPTION_RES,
				 "ap tee", 0, NULL) },
};

static int init_plat_sdei_tee_event(u32 event_num, void *cb, void *arg)
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

static void rdr_tee_dump(u32 modid, u32 etype, u64 coreid, char *log_path)
{
}

static u64 g_phy_addr;

static void init_rdr_sdei_tee_events(void)
{
	unsigned int i;
	u32 ret;
	struct rdr_register_module_result retinfo;
	struct rdr_module_ops_pub s_dsp_ops;

	DST_PR_START();

	/** 1) Get phy address */
	s_dsp_ops.ops_dump = rdr_tee_dump;
	s_dsp_ops.ops_reset = NULL;

	ret = rdr_register_module_ops(RDR_TEEOS, &s_dsp_ops, &retinfo);
	if ((int)ret < 0) {
		DST_ERR("rdr_register_module_ops fail, ret = [%d]\n", ret);
		return;
	}

	/** 2) Register phy address */
	g_phy_addr = retinfo.log_addr;
	dst_sec_call(DST_SET_TEE_MEMORY, g_sdei_tee_events[0].event_num,
		     retinfo.log_addr, 0);
	DST_ERR("phy_addr = [0x%llx]\n", retinfo.log_addr);
	for (i = 0; i < ARRAY_SIZE(g_sdei_tee_events); i++) {
		DST_DBG("register exception:%u",
			g_sdei_tee_events[i].einfo.e_exce_type);
		ret = rdr_register_exception(&g_sdei_tee_events[i].einfo);
		if (ret == 0) {
			DST_ERR("rdr_register_exception fail, ret = [%u]\n",
				ret);
			continue;
		}
		init_plat_sdei_tee_event(g_sdei_tee_events[i].event_num,
					 g_sdei_tee_events[i].cb,
					 g_sdei_tee_events[i].arg);
	}
	DST_PR_END();
}

int plat_sdei_tee_event_callback(u32 event, struct pt_regs *regs, void *arg)
{
	unsigned int i;
	struct EventArgs *evtargs = NULL;

	DST_PR_START();

	for (i = 0; i < ARRAY_SIZE(g_sdei_tee_events); i++) {
		if (event == g_sdei_tee_events[i].event_num) {
			evtargs = &g_sdei_tee_events[i];
			break;
		}
	}

	if (!evtargs) {
		DST_ERR("event%d not found ...\n", event);
		return 0;
	}

	bust_spinlocks(1); /* bust_spinlocks is open */
#ifdef CONFIG_PLAT_KERNELDUMP
	plat_set_cpu_regs(raw_smp_processor_id(), regs);
#endif
	trigger_allbutcpu_cpu_backtrace(raw_smp_processor_id());

	rdr_system_error(evtargs->einfo.e_modid, 0, 0);
	/*If the rdr is not initialized and the AP is not reset, then restart the AP.*/
	machine_restart(
		rdr_get_exception_type_name(evtargs->einfo.e_exce_type));
	return 0;
}

static int dst_tee_sdei_init(void)
{
	init_rdr_sdei_tee_events();
	return 0;
}

late_initcall(dst_tee_sdei_init);

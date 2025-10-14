// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Cix Technology Group Co., Ltd.*/

#include <linux/arm_sdei.h>
#include <linux/cacheflush.h>
#include <mntn_subtype_exception.h>
#include <linux/soc/cix/rdr_platform.h>
#include <linux/nmi.h>
#include "dst_print.h"

/* define for SDEI tzc400 Intr */
#define SDEI_SKY1_TZC400_EVENT0 (210)
#define SDEI_SKY1_TZC400_EVENT1 (211)
#define SDEI_SKY1_TZC400_EVENT2 (212)
#define SDEI_SKY1_TZC400_EVENT3 (213)
#define SDEI_SKY1_TZC400_EVENT_BASE SDEI_SKY1_TZC400_EVENT0
#define SDEI_SKY1_TZC400_EVENT_END SDEI_SKY1_TZC400_EVENT3

#define DST_SET_TZC400_MEMORY (0x11)

static int sdei_tzc400_event_callback(u32 event, struct pt_regs *regs,
				      void *arg);

#define MAX_TZC400_NAME (8)
#define MAX_TZC400_FILTERS (4)

struct TZC400_ERROR_INFO {
	uintptr_t fail_address;
	u32 fail_ctrl;
	u32 fail_id;
	u32 intr_status; // store intr status
};

struct EventArgs {
	u32 event_num;
	void *cb;
	struct TZC400_ERROR_INFO *args;
};

static struct EventArgs g_sdei_events[] = {
	{
		SDEI_SKY1_TZC400_EVENT0,
		sdei_tzc400_event_callback,
		NULL,
	},
	{
		SDEI_SKY1_TZC400_EVENT1,
		sdei_tzc400_event_callback,
		NULL,
	},
	{
		SDEI_SKY1_TZC400_EVENT2,
		sdei_tzc400_event_callback,
		NULL,
	},
	{
		SDEI_SKY1_TZC400_EVENT3,
		sdei_tzc400_event_callback,
		NULL,
	},
};

struct rdr_exception_info_s g_tzc400_einfo[] = {
	DEF_EXCE_STRUCT_SINGLE(RDR_WARN, RDR_REBOOT_NO, RDR_AP, 0, RDR_AP,
			       TZC400_EXCEPTION, TZC400_EXCEPTION_RES,
			       "tzc400-error", RDR_SAVE_DMESG | RDR_SAVE_BL31,
			       NULL),
};

static void init_rdr_tzc400_exception(void)
{
	unsigned int i;
	u32 ret;

	DST_PR_START();
	for (i = 0; i < ARRAY_SIZE(g_tzc400_einfo); i++) {
		ret = rdr_register_exception(&g_tzc400_einfo[i]);
		if (!ret) {
			DST_ERR("rdr_register_exception fail, ret = [%u]\n",
				ret);
			continue;
		}
	}
	DST_PR_END();
}

static int init_sdei_tzc400_event(u32 event_num, void *cb, void *arg)
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

static void init_sdei_tzc400_events(void)
{
	unsigned int i;
	unsigned char *virt_addr;
	uintptr_t phy_addr;
	u32 size, per_size;

	DST_PR_START();

	if (get_module_dump_mem_addr(MODU_TZC400, &virt_addr, &size)) {
		DST_ERR("get module memory failed.\n");
		return;
	}
	per_size = MAX_TZC400_FILTERS * sizeof(struct TZC400_ERROR_INFO);
	if (size < per_size * ARRAY_SIZE(g_sdei_events)) {
		DST_ERR("memory is too small.\n");
		return;
	}

	phy_addr = (vmalloc_to_pfn(virt_addr) << PAGE_SHIFT) +
		   ((u64)virt_addr & ((1 << PAGE_SHIFT) - 1));
	DST_DBG("phys memory address=0x%lx\n", phy_addr);

	for (i = 0; i < ARRAY_SIZE(g_sdei_events); i++) {
		DST_DBG("register exception:%u", g_sdei_events[i].event_num);
		g_sdei_events[i].args =
			(struct TZC400_ERROR_INFO *)((size_t)virt_addr +
						     ((size_t)per_size * i));
		// set share memory address to tf-a
		dst_sec_call(DST_SET_TZC400_MEMORY,
			     (u64)g_sdei_events[i].event_num,
			     (u64)phy_addr + ((size_t)per_size * i), 0);
		init_sdei_tzc400_event(g_sdei_events[i].event_num,
				       g_sdei_events[i].cb, NULL);
		DST_DBG("set address for event%d", g_sdei_events[i].event_num);
	}
	// init tzc400 rdr exception
	init_rdr_tzc400_exception();

	DST_PR_END();
}

void tzc400_print_info(int index, struct TZC400_ERROR_INFO *info)
{
	uint secure, direction;

	secure = info->fail_ctrl & BIT(21);
	direction = info->fail_ctrl & BIT(24);

	DST_ERR("ERR MSG:\n");
	DST_ERR("\tindex: %d\n", index);
	DST_ERR("\taddress: 0x%lx\n", info->fail_address);
	DST_ERR("\tnasid: %d\n", info->fail_id);
	DST_ERR("\tsecure: %s\n",
		secure ? "Non-secure access" : "Secure access");
	DST_ERR("\tdirection: %s\n",
		direction ? "Write access" : "Read access");
}

#ifdef CONFIG_PLAT_KERNELDUMP
extern void plat_set_cpu_regs(int coreid, struct pt_regs *reg);
#endif

int sdei_tzc400_event_callback(u32 event, struct pt_regs *regs, void *arg)
{
	int index;
	int has_error = -1;
	struct TZC400_ERROR_INFO *info;

	if (event > SDEI_SKY1_TZC400_EVENT_END ||
	    event < SDEI_SKY1_TZC400_EVENT_BASE) {
		DST_ERR("Error Event%d\n", event);
		return 0;
	}

	index = (int)(event - SDEI_SKY1_TZC400_EVENT_BASE);
	info = (struct TZC400_ERROR_INFO *)g_sdei_events[index].args;
	dcache_inval_poc((unsigned long)info,
			 (unsigned long)((char *)info +
					 sizeof(*info) * MAX_TZC400_FILTERS));

	for (index = 0; index < MAX_TZC400_FILTERS; index++) {
		if (info[index].fail_address != 0) {
			tzc400_print_info(index, &info[index]);
			has_error = index;
		}
	}

	if (has_error != -1) {
		DST_PN("rdr tzc400 exception triggered...\n");
#ifdef CONFIG_PLAT_KERNELDUMP
		plat_set_cpu_regs(raw_smp_processor_id(), regs);
#endif
		trigger_allbutcpu_cpu_backtrace(raw_smp_processor_id());
		rdr_system_error(MODID_TZC400_EXCEPTION_RES,
				 info[has_error].fail_address >> 32,
				 info[has_error].fail_address & 0xffffffff);
	}
	return 0;
}

static int __init dst_tzc400_sdei_init(void)
{
	init_sdei_tzc400_events();
	return 0;
}

late_initcall(dst_tzc400_sdei_init);

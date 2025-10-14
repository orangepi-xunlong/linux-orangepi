// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/

#include <linux/arm_sdei.h>
#include <linux/cacheflush.h>
#include <mntn_subtype_exception.h>
#include <linux/soc/cix/rdr_platform.h>
#include <linux/nmi.h>
#include "dst_print.h"

/* define for SDEI idm Intr */
#define SDEI_SKY1_IDM_MMHUB_EVENT0 (200)
#define SDEI_SKY1_IDM_MMHUB_EVENT1 (201)
#define SDEI_SKY1_IDM_PCIE_EVENT0 (202)
#define SDEI_SKY1_IDM_PCIE_EVENT1 (203)
#define SDEI_SKY1_IDM_SYSHUB_EVENT0 (204)
#define SDEI_SKY1_IDM_SYSHUB_EVENT1 (205)
#define SDEI_SKY1_IDM_SMN_EVENT0 (206)
#define SDEI_SKY1_IDM_SMN_EVENT1 (207)
#define SDEI_SKY1_IDM_EVENT_BASE SDEI_SKY1_IDM_MMHUB_EVENT0
#define SDEI_SKY1_IDM_EVENT_END SDEI_SKY1_IDM_SMN_EVENT1

#define DST_SET_IDM_MEMORY 1

static int idm_sdei_event_callback(u32 event, struct pt_regs *regs, void *arg);

#define MAX_IDM_NAME (12)

struct IDM_INFO {
	uint8_t is_secure;
	int8_t error_idm_index;
	uintptr_t error_address;
	unsigned int error_type; // store intr status
	unsigned int error_status; // store error status
	uint32_t error_intr_num;
	char name[MAX_IDM_NAME];
};

struct EventArgs {
	u32 event_num;
	void *cb;
	struct IDM_INFO *args;
};

#define IDM_EVENT_DEF(name, callback) \
	{ SDEI_SKY1_IDM_##name, idm_##callback##_callback, NULL }

static struct EventArgs g_idm_events[] = {
	IDM_EVENT_DEF(MMHUB_EVENT0, sdei_event),
	IDM_EVENT_DEF(MMHUB_EVENT1, sdei_event),
	IDM_EVENT_DEF(PCIE_EVENT0, sdei_event),
	IDM_EVENT_DEF(PCIE_EVENT1, sdei_event),
	IDM_EVENT_DEF(SYSHUB_EVENT0, sdei_event),
	IDM_EVENT_DEF(SYSHUB_EVENT1, sdei_event),
	IDM_EVENT_DEF(SMN_EVENT0, sdei_event),
	IDM_EVENT_DEF(SMN_EVENT1, sdei_event),
};

struct rdr_exception_info_s g_idm_einfo[] = {

#ifdef CONFIG_DST_IDM_RECOVERY
	DEF_EXCE_STRUCT(MODID_NI700_IDM_TIMEOUT, MODID_NI700_IDM_TIMEOUT,
			RDR_ERR, RDR_REBOOT_NO, RDR_AP, 0, RDR_AP,
			NI700_EXCEPTION, NI700_EXCEPTION_IDM, "idm timeout", 0,
			NULL),
#else
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, 0, RDR_AP,
			       NI700_EXCEPTION, NI700_EXCEPTION_IDM_TIMEOUT,
			       "idm timeout", 0, NULL),
#endif
};

static void init_rdr_idm_exception(void)
{
	unsigned int i;
	u32 ret;

	DST_PR_START();
	for (i = 0; i < ARRAY_SIZE(g_idm_einfo); i++) {
		ret = rdr_register_exception(&g_idm_einfo[i]);
		if (!ret) {
			DST_ERR("rdr_register_exception fail, ret = [%u]\n",
				ret);
			continue;
		}
	}
	DST_PR_END();
}

static void idm_exception_work(struct IDM_INFO *info)
{
	trigger_allbutcpu_cpu_backtrace(raw_smp_processor_id());
	rdr_system_error(MODID_NI700_EXCEPTION_IDM_TIMEOUT,
			 info->error_address >> 32,
			 info->error_address & 0xffffffff);
}

static int init_idm_sdei_event(u32 event_num, void *cb, void *arg)
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

static void init_idm_sdei_events(void)
{
	unsigned int i;
	unsigned char *virt_addr;
	uintptr_t phy_addr;
	u32 size;

	DST_PR_START();

	if (get_module_dump_mem_addr(MODU_IDM, &virt_addr, &size)) {
		DST_ERR("get module memory failed.\n");
		return;
	}

	phy_addr = (vmalloc_to_pfn(virt_addr) << PAGE_SHIFT) +
		   ((u64)virt_addr & ((1 << PAGE_SHIFT) - 1));
	DST_DBG("phys memory address=0x%lx\n", phy_addr);

	for (i = 0; i < ARRAY_SIZE(g_idm_events); i++) {
		DST_DBG("register exception:%u", g_idm_events[i].event_num);
		// set share memory address to tf-a
		dst_sec_call(DST_SET_IDM_MEMORY, (u64)phy_addr, 0, 0);
		g_idm_events[i].args = (struct IDM_INFO *)virt_addr;
		init_idm_sdei_event(g_idm_events[i].event_num,
				    g_idm_events[i].cb, NULL);
		DST_DBG("set address for event%d", g_idm_events[i].event_num);
	}
	// init idm rdr exception
	init_rdr_idm_exception();

	DST_PR_END();
}

#ifdef CONFIG_PLAT_KERNELDUMP
extern void plat_set_cpu_regs(int coreid, struct pt_regs *reg);
#endif

int idm_sdei_event_callback(u32 event, struct pt_regs *regs, void *arg)
{
	int index;
	struct IDM_INFO *info;

	if (event > SDEI_SKY1_IDM_EVENT_END ||
	    event < SDEI_SKY1_IDM_EVENT_BASE) {
		DST_ERR("Error Event%d\n", event);
		return 0;
	}

	index = (int)(event - SDEI_SKY1_IDM_EVENT_BASE);

	info = (struct IDM_INFO *)g_idm_events[index].args;
	dcache_inval_poc((unsigned long)info,
			 (unsigned long)((char *)info + sizeof(*info)));
	DST_ERR("%s: Error address: 0x%lx\n", info->name, info->error_address);

#ifdef CONFIG_PLAT_KERNELDUMP
	plat_set_cpu_regs(raw_smp_processor_id(), regs);
#endif

	idm_exception_work(info);

	return 0;
}

static int __init dst_idm_sdei_init(void)
{
	init_idm_sdei_events();
	return 0;
}

late_initcall(dst_idm_sdei_init);

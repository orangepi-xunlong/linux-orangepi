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
#include <linux/module.h>
#include "dst_print.h"
#include <linux/soc/cix/util.h>
#include <linux/pgtable.h>

/* define for SDEI idm Intr */
#define SDEI_SKY1_IDM_MMHUB_EVENT0	(200)
#define SDEI_SKY1_IDM_MMHUB_EVENT1	(201)
#define SDEI_SKY1_IDM_PCIE_EVENT0	(202)
#define SDEI_SKY1_IDM_PCIE_EVENT1	(203)
#define SDEI_SKY1_IDM_SYSHUB_EVENT0	(204)
#define SDEI_SKY1_IDM_SYSHUB_EVENT1	(205)
#define SDEI_SKY1_IDM_SMN_EVENT0	(206)
#define SDEI_SKY1_IDM_SMN_EVENT1	(207)
#define SDEI_SKY1_IDM_EVENT_BASE	SDEI_SKY1_IDM_MMHUB_EVENT0
#define SDEI_SKY1_IDM_EVENT_END		SDEI_SKY1_IDM_SMN_EVENT1

static int idm_sdei_event_callback(u32 event, struct pt_regs *regs, void *arg);

#define MAX_IDM_NAME (12)

struct IDM_INFO {
	uint8_t is_secure;
	int8_t error_idm_index;
	uintptr_t error_address;
	unsigned int error_type;	// store intr status
	unsigned int error_status;	// store error status
	uint32_t error_intr_num;
	char name[MAX_IDM_NAME];
};

struct EventArgs
{
	u32 event_num;
	void *cb;
	struct IDM_INFO *args;
};

static struct delayed_work g_idm_wq;

static struct EventArgs g_sdei_events[] = {
	{ SDEI_SKY1_IDM_MMHUB_EVENT0, idm_sdei_event_callback, NULL, },
	{ SDEI_SKY1_IDM_MMHUB_EVENT1, idm_sdei_event_callback, NULL, },
	{ SDEI_SKY1_IDM_PCIE_EVENT0, idm_sdei_event_callback, NULL, },
	{ SDEI_SKY1_IDM_PCIE_EVENT1, idm_sdei_event_callback, NULL, },
	{ SDEI_SKY1_IDM_SYSHUB_EVENT0, idm_sdei_event_callback, NULL, },
	{ SDEI_SKY1_IDM_SYSHUB_EVENT1, idm_sdei_event_callback, NULL, },
	{ SDEI_SKY1_IDM_SMN_EVENT0, idm_sdei_event_callback, NULL, },
	{ SDEI_SKY1_IDM_SMN_EVENT1, idm_sdei_event_callback, NULL, },
};

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
struct rdr_exception_info_s g_idm_einfo[] = {
#ifdef CONFIG_DST_IDM_RECOVERY
	{ { 0, 0 }, MODID_NI700_IDM_TIMEOUT, MODID_NI700_IDM_TIMEOUT, RDR_ERR, RDR_REBOOT_NO,
#else
	{ { 0, 0 }, MODID_NI700_IDM_TIMEOUT, MODID_NI700_IDM_TIMEOUT, RDR_ERR, RDR_REBOOT_NOW,
#endif
			RDR_AP, 0, RDR_AP, (u32)RDR_REENTRANT_DISALLOW, (u32)IDM_S_EXCEPTION,
			HI_APPANIC_RESERVED, (u32)RDR_UPLOAD_YES, "idm timeout", "idm timeout", 0, 0, 0, 0},
};

static void init_rdr_idm_exception(void)
{
	unsigned int i;
	u32 ret;

	DST_PRINT_START();
	for (i = 0; i < sizeof(g_idm_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		ret = rdr_register_exception(&g_idm_einfo[i]);
		if (!ret) {
			DST_PRINT_ERR("rdr_register_exception fail, ret = [%u]\n", ret);
			continue;
		}
	}
	DST_PRINT_END();
}

static void idm_exception_work(struct work_struct *work)
{
	pr_info("rdr test exception triggered... \n");
	trigger_allbutself_cpu_backtrace();
	rdr_syserr_process_for_ap(MODID_NI700_IDM_TIMEOUT, 0, 0);
}

static int init_idm_sdei_event(u32 event_num, void *cb, void *arg)
{
	int err;

	err = sdei_event_register(event_num, cb, arg);
	if (!err) {
		err = sdei_event_enable(event_num);
		if(!err) {
			DST_PRINT_PN("%s,%d: event%d registered & enabled ...\n",
				__func__, __LINE__, event_num);
			return -1;
		}
	} else {
		DST_PRINT_ERR("%s,%d: event%d register failed ...\n",
			__func__, __LINE__, event_num);
		return -1;
	}

	return 0;
}

static void init_idm_sdei_events(void)
{
	unsigned int i;
	unsigned char* virt_addr;
	uintptr_t phy_addr;

	DST_PRINT_START();
	INIT_DELAYED_WORK(&g_idm_wq, idm_exception_work);

	if (get_module_dump_mem_addr(MODU_IDM, &virt_addr)) {
		DST_PRINT_ERR("%s, get module memory failed. \n", __func__);
		return;
	}

	phy_addr = (vmalloc_to_pfn(virt_addr) << PAGE_SHIFT) + ((u64)virt_addr & ((1 << PAGE_SHIFT) - 1));
	DST_PRINT_DBG("%s, phys memory address=0x%lx \n", __func__, phy_addr);

	for (i = 0; i < sizeof(g_sdei_events) / sizeof(struct EventArgs); i++) {
		DST_PRINT_DBG("register exception:%u", g_sdei_events[i].event_num);
		// set share memory address to tf-a
		sdei_api_event_set_info(g_sdei_events[i].event_num, SDEI_EVENT_SET_IDM_MEMORY, (u64)phy_addr);
		g_sdei_events[i].args = (struct IDM_INFO *)virt_addr;
		init_idm_sdei_event(g_sdei_events[i].event_num, g_sdei_events[i].cb, NULL);
		DST_PRINT_DBG("%s: set address for event%d", __func__, g_sdei_events[i].event_num);
	}
	// init idm rdr exception
	init_rdr_idm_exception();

	DST_PRINT_END();
}

#ifdef CONFIG_PLAT_KERNELDUMP
extern void plat_set_cpu_regs(int coreid, struct pt_regs* reg);
#endif

int idm_sdei_event_callback(u32 event, struct pt_regs *regs, void *arg)
{
	int index;
	struct IDM_INFO* info;

	if (event > SDEI_SKY1_IDM_EVENT_END || event < SDEI_SKY1_IDM_EVENT_BASE) {
		DST_PRINT_ERR("%s, Error Event%d \n", __func__, event);
		return 0;
	}

	index = (int)(event - SDEI_SKY1_IDM_EVENT_BASE);

	info = (struct IDM_INFO*)g_sdei_events[index].args;
	dcache_inval_poc((unsigned long)info, (unsigned long)((char*)info + sizeof(*info)));
	DST_PRINT_ERR("%s, %s: Error address: 0x%lx \n", __func__, info->name, info->error_address);

#ifdef CONFIG_PLAT_KERNELDUMP
	plat_set_cpu_regs(raw_smp_processor_id(), regs);
#endif

#ifdef CONFIG_DST_IDM_RECOVERY
	schedule_delayed_work(&g_idm_wq, 0);
#else
	idm_exception_work(NULL);
#endif

	return 0;
}

static int __init dst_idm_sdei_init(void)
{
	init_idm_sdei_events();
	return 0;
}

late_initcall(dst_idm_sdei_init);

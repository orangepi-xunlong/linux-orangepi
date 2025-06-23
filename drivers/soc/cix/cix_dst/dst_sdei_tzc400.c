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

/* define for SDEI tzc400 Intr */
#define SDEI_SKY1_TZC400_EVENT0		(210)
#define SDEI_SKY1_TZC400_EVENT1		(211)
#define SDEI_SKY1_TZC400_EVENT2		(212)
#define SDEI_SKY1_TZC400_EVENT3		(213)
#define SDEI_SKY1_TZC400_EVENT_BASE		SDEI_SKY1_TZC400_EVENT0
#define SDEI_SKY1_TZC400_EVENT_END		SDEI_SKY1_TZC400_EVENT3

static int sdei_tzc400_event_callback(u32 event, struct pt_regs *regs, void *arg);

#define MAX_TZC400_NAME (8)
#define MAX_TZC400_FILTERS (4)

struct TZC400_ERROR_INFO {
	uintptr_t fail_address;
	unsigned fail_ctrl;
	unsigned fail_id;
	unsigned intr_status;	// store intr status
};

struct EventArgs
{
	u32 event_num;
	void *cb;
	struct TZC400_ERROR_INFO *args;
};

static struct EventArgs g_sdei_events[] = {
	{ SDEI_SKY1_TZC400_EVENT0, sdei_tzc400_event_callback, NULL, },
	{ SDEI_SKY1_TZC400_EVENT1, sdei_tzc400_event_callback, NULL, },
	{ SDEI_SKY1_TZC400_EVENT2, sdei_tzc400_event_callback, NULL, },
	{ SDEI_SKY1_TZC400_EVENT3, sdei_tzc400_event_callback, NULL, },
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
struct rdr_exception_info_s g_tzc400_einfo[] = {
	{ { 0, 0 }, MODID_TZC400_ERROR,	MODID_TZC400_ERROR, RDR_ERR, RDR_REBOOT_NO,
		RDR_AP, RDR_AP, RDR_AP,	(u32)RDR_REENTRANT_DISALLOW, (u32)TZC400_S_EXCEPTION,
		HI_APPANIC_RESERVED, (u32)RDR_UPLOAD_YES, "tzc400-error", "tzc400-error", 0, 0, 0, 0 },
};

static void init_rdr_tzc400_exception(void)
{
	unsigned int i;
	u32 ret;

	DST_PRINT_START();
	for (i = 0; i < sizeof(g_tzc400_einfo) / sizeof(struct rdr_exception_info_s); i++) {
		ret = rdr_register_exception(&g_tzc400_einfo[i]);
		if (!ret) {
			DST_PRINT_ERR("rdr_register_exception fail, ret = [%u]\n", ret);
			continue;
		}
	}
	DST_PRINT_END();
}

static int init_sdei_tzc400_event(u32 event_num, void *cb, void *arg)
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

static void init_sdei_tzc400_events(void)
{
	unsigned int i;
	unsigned char* virt_addr;
	uintptr_t phy_addr;

	DST_PRINT_START();

	if (get_module_dump_mem_addr(MODU_TZC400, &virt_addr)) {
		DST_PRINT_ERR("%s, get module memory failed. \n", __func__);
		return;
	}

	phy_addr = (vmalloc_to_pfn(virt_addr) << PAGE_SHIFT) + ((u64)virt_addr & ((1 << PAGE_SHIFT) - 1));
	DST_PRINT_DBG("%s, phys memory address=0x%lx \n", __func__, phy_addr);

	for (i = 0; i < sizeof(g_sdei_events) / sizeof(struct EventArgs); i++) {
		DST_PRINT_DBG("register exception:%u", g_sdei_events[i].event_num);
		g_sdei_events[i].args = (struct TZC400_ERROR_INFO *)virt_addr;
		// set share memory address to tf-a
		sdei_api_event_set_info(g_sdei_events[i].event_num, SDEI_EVENT_SET_TZC400_MEMORY, (u64)phy_addr);
		// set tf-a tzc400 function enabled
		sdei_api_event_set_info(g_sdei_events[i].event_num, SDEI_EVENT_SET_TZC400_ENABLE, 1);
		init_sdei_tzc400_event(g_sdei_events[i].event_num, g_sdei_events[i].cb, NULL);
		DST_PRINT_DBG("%s: set address for event%d", __func__, g_sdei_events[i].event_num);

	}
	// init tzc400 rdr exception
	init_rdr_tzc400_exception();

	DST_PRINT_END();
}

void tzc400_print_info(int index, struct TZC400_ERROR_INFO *info)
{
	uint secure, direction;

	secure = info->fail_ctrl & BIT(21);
	direction = info->fail_ctrl & BIT(24);

	DST_PRINT_ERR("ERR MSG:\n");
	DST_PRINT_ERR("\tindex: %d\n", index);
	DST_PRINT_ERR("\taddress: 0x%lx\n", info->fail_address);
	DST_PRINT_ERR("\tnasid: %d\n", info->fail_id);
	DST_PRINT_ERR("\tsecure: %s\n",
		      secure ? "Non-secure access" : "Secure access");
	DST_PRINT_ERR("\tdirection: %s\n",
		      direction ? "Write access" : "Read access");
}

#ifdef CONFIG_PLAT_KERNELDUMP
extern void plat_set_cpu_regs(int coreid, struct pt_regs* reg);
#endif

int sdei_tzc400_event_callback(u32 event, struct pt_regs *regs, void *arg)
{
	int index;
	bool has_error = 0;
	struct TZC400_ERROR_INFO* info;

	if (event > SDEI_SKY1_TZC400_EVENT_END || event < SDEI_SKY1_TZC400_EVENT_BASE) {
		DST_PRINT_ERR("%s, Error Event%d \n", __func__, event);
		return 0;
	}

	index = (int)(event - SDEI_SKY1_TZC400_EVENT_BASE);
	info = (struct TZC400_ERROR_INFO*)g_sdei_events[index].args;
	dcache_inval_poc((unsigned long)info, (unsigned long)((char*)info + sizeof(*info) * MAX_TZC400_FILTERS));

	for (index = 0; index < MAX_TZC400_FILTERS; index++) {
		info += index;
		if (info->fail_address != 0) {
			tzc400_print_info(index, info);
			has_error = TRUE;
		}
	}

	if (has_error) {
		pr_info("rdr tzc400 exception triggered... \n");
#ifdef CONFIG_PLAT_KERNELDUMP
		plat_set_cpu_regs(raw_smp_processor_id(), regs);
#endif
		trigger_allbutself_cpu_backtrace();
		rdr_syserr_process_for_ap(MODID_TZC400_ERROR, 0, 0);
	}
	return 0;
}

static int __init dst_tzc400_sdei_init(void)
{
	init_sdei_tzc400_events();
	return 0;
}

late_initcall(dst_tzc400_sdei_init);
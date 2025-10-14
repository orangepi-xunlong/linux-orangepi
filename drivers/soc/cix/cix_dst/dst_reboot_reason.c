// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */

#include <linux/arm_sdei.h>
#include <linux/soc/cix/dst_reboot_reason.h>
#include <linux/io.h>
#include "dst_print.h"
#if IS_ENABLED(CONFIG_PLAT_BBOX)
#include "blackbox/rdr_inner.h"
#endif

#define REBOOT_REASON_SHIFT 0
#define SUB_REBOOT_REASON_SHIFT 8
#define REBOOT_REASON_MASK 0xFFFFFF00
#define SUB_REBOOT_REASON_MASK 0xFFFF00FF

static u64 g_reboot_reason_addr;
static uint32_t g_last_reboot_reason;

#ifdef CONFIG_SKY1_REBOOT_REASON
#define CIX_SIP_SVC_SET_REBOOT_REASON (0xc2000002)
static void set_reboot_reason_sky1(unsigned int reboot_reason)
{
	struct arm_smccc_res res;

	arm_smccc_smc(CIX_SIP_SVC_SET_REBOOT_REASON, reboot_reason, 0, 0, 0, 0,
		      0, 0, &res);
}
#endif

void set_reboot_reason(unsigned int reboot_reason, unsigned int sub_reason)
{
	unsigned int value;
	static u32 prev_set;

	if (prev_set != 0 && prev_set != COLDBOOT) {
		DST_ERR("reboot reason is already set\n");
		return;
	}

	if (g_reboot_reason_addr) {
		value = readl((void *)(uintptr_t)g_reboot_reason_addr);
		value &= REBOOT_REASON_MASK;
		value |= reboot_reason;
		value |= ((sub_reason << SUB_REBOOT_REASON_SHIFT) &
			  ~SUB_REBOOT_REASON_MASK);
#ifdef CONFIG_SKY1_REBOOT_REASON
		DST_ERR("set reboot reason 0x%x\n", value);
		set_reboot_reason_sky1(value);
#else
		DST_PN("set reboot reason 0x%x\n", value);
		writel(value, (void *)(uintptr_t)g_reboot_reason_addr);
#endif
		prev_set = reboot_reason;
	} else {
		DST_ERR("set failed 0x%x\n", reboot_reason);
	}
}

unsigned int get_reboot_reason(bool is_last)
{
	unsigned int value = 0xFF;

	if (is_last)
		return g_last_reboot_reason & 0xFF;

	if (g_reboot_reason_addr) {
		value = readl((void *)(uintptr_t)g_reboot_reason_addr);
		value &= 0xFF;
		DST_PN("get:0x%x\n", value);
	}

	return value;
}

/*
 * Description : Obtaining the reboot reason
 */
unsigned int get_sub_reboot_reason(bool is_last)
{
	unsigned int value = 0xFF;

	if (is_last)
		return (g_last_reboot_reason >> SUB_REBOOT_REASON_SHIFT) & 0xFF;

	if (g_reboot_reason_addr) {
		value = readl((void *)(uintptr_t)g_reboot_reason_addr);
		value = (g_last_reboot_reason >> SUB_REBOOT_REASON_SHIFT) &
			0xFF;
		DST_PN("get:0x%x\n", value);
	}
	return value;
}

void plat_pm_system_reset_comm(const char *cmd)
{
	unsigned int curr_reboot_type = UNKNOWN;

	if (cmd == NULL || *cmd == '\0') {
		DST_PN("cmd is null\n");
	} else {
		DST_PN("cmd is %s\n", cmd);
#if IS_ENABLED(CONFIG_PLAT_BBOX)
		curr_reboot_type = rdr_get_exception_type((char *)cmd);
#endif
	}

	if (curr_reboot_type == UNKNOWN) {
		curr_reboot_type = COLDBOOT;
		console_verbose();
		dump_stack();
	}
	set_reboot_reason(curr_reboot_type, 0);
}

/*
 * base address:0x1600_0000
 * ID  offset  Desc                    comments
 * 0	0x218	RSMRST_                    HW
 * 1	0x218	STR                        HW
 * 2	0x218	SD                         HW
 * 3	0x218	WDT 2nd TIME-OUT           HW
 * 4	0x218	WARM RESET                 HW
 * 5	0x218	EXTERNAL RESET             HW
 * 6	0x218	POWER BUTTON OVERRID       HW
 * 7	0x218	CSU_SE WDT 2ND TIME-OUT    HW
 */
void print_hw_reboot_reason(u32 value)
{
	char *Desc = NULL;

	switch (value) {
	case 0:
		Desc = "RSMRST_";
		break;
	case 1:
		Desc = "STR";
		break;
	case 2:
		Desc = "SD";
		break;
	case 3:
		Desc = "WDT 2nd TIME-OUT";
		break;
	case 4:
		Desc = "WARM RESET";
		break;
	case 5:
		Desc = "EXTERNAL RESET";
		break;
	case 6:
		Desc = "POWER BUTTON OVERRID";
		break;
	case 7:
		Desc = "CSU_SE WDT 2ND TIME-OUT";
		break;
	default:
		Desc = "Unknown";
		break;
	}
	DST_PN("last hw reboot reason:0x%x (%s)\n", value, Desc);
}

/*
 * Description:    init bootup_keypoint_addr
 * Input:          NA
 * Output:         NA
 * Return:         NA
 */
static void reboot_reason_addr_init(void)
{
	static u64 hw_reboot_reason_addr;

	g_reboot_reason_addr =
		(uintptr_t)ioremap_wc(REBOOT_REASON_ADDR, sizeof(int));

	if (g_reboot_reason_addr) {
		g_last_reboot_reason =
			readl((void *)(uintptr_t)g_reboot_reason_addr);
		writel(0, (void *)(uintptr_t)g_reboot_reason_addr);
		DST_PN("last sw reboot reason:0x%x\n", g_last_reboot_reason);
		set_reboot_reason_sky1(AP_S_COLDBOOT);
	}

	hw_reboot_reason_addr =
		(uintptr_t)ioremap_wc(SKY1_HW_REBOOT_REASON_ADDR, sizeof(int));

	if (hw_reboot_reason_addr) {
		print_hw_reboot_reason(
			readl((void *)(uintptr_t)hw_reboot_reason_addr));
	}
}

static int reboot_reason_handle(struct notifier_block *nb, unsigned long action,
				void *cmd)
{
	switch (action) {
	case SYS_RESTART:
		plat_pm_system_reset_comm(cmd);
		break;
	case SYS_HALT:
	case SYS_POWER_OFF:
	default:
		set_reboot_reason(COLDBOOT, 0);
		break;
	}
	return NOTIFY_DONE;
}

static struct notifier_block reboot_reason_notifier = {
	.notifier_call = reboot_reason_handle,
};

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init reboot_reason_init(void)
{
	reboot_reason_addr_init();
	if (register_reboot_notifier(&reboot_reason_notifier))
		DST_ERR("register reboot notifier failed\n");
	return 0;
}
early_initcall(reboot_reason_init);

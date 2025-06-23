// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */

#include <linux/arm_sdei.h>
#include <linux/soc/cix/dst_reboot_reason.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <mntn_public_interface.h>
#include "dst_print.h"
#include "blackbox/rdr_inner.h"

#define REBOOT_REASON_SHIFT 0
#define SUB_REBOOT_REASON_SHIFT 8
#define REBOOT_REASON_MASK 0xFFFFFF00
#define SUB_REBOOT_REASON_MASK 0xFFFF00FF

static u64 g_reboot_reason_addr = 0;

#ifdef CONFIG_SKY1_REBOOT_REASON
#define CIX_SIP_SVC_SET_REBOOT_REASON (0xc2000002)
static void set_reboot_reason_sky1(unsigned int reboot_reason)
{
	struct arm_smccc_res res;

	arm_smccc_smc(CIX_SIP_SVC_SET_REBOOT_REASON, reboot_reason, 0, 0, 0, 0, 0, 0, &res);
}
#endif

void set_reboot_reason(unsigned int reboot_reason)
{
	unsigned int value;
	static bool is_set = false;

	if (is_set == true) {
		DST_PRINT_PN("[%s]: reboot reason is already set\n", __func__);
		return;
	}

	if (g_reboot_reason_addr) {
		DST_PRINT_PN("[%s]:set 0x%x\n", __func__, reboot_reason);
		value = readl((void *)(uintptr_t)g_reboot_reason_addr);
		value &= REBOOT_REASON_MASK;
		value |= reboot_reason;
#ifdef CONFIG_SKY1_REBOOT_REASON
		DST_PRINT_PN("[%s]:set reboot reason 0x%x\n", __func__,
			     value);
		set_reboot_reason_sky1(value);
#else
		DST_PRINT_PN("[%s]:set reboot reason 0x%x\n", __func__,
			     value);
		writel(value, (void *)(uintptr_t)g_reboot_reason_addr);
#endif
		is_set = true;
	} else {
		DST_PRINT_ERR("[%s]:set failed 0x%x\n", __func__,
			      reboot_reason);
	}
}

/*
 * Description : Obtaining the reboot reason
 */
unsigned int get_reboot_reason(void)
{
	unsigned int value = 0xFF;

	if (g_reboot_reason_addr) {
		value = readl((void *)(uintptr_t)g_reboot_reason_addr);
		value &= 0xFF;
		DST_PRINT_PN("[%s]: get:0x%x\n", __func__, value);
	}

	return value;
}

void plat_pm_system_reset_comm(const char *cmd)
{
	unsigned int i;
	unsigned int curr_reboot_type = UNKNOWN;
	const struct cmdword *reboot_reason_map = NULL;

	if (cmd == NULL || *cmd == '\0') {
		pr_info("%s cmd is null\n", __func__);
		cmd = "COLDBOOT";
	} else {
		pr_info("%s cmd is %s\n", __func__, cmd);
	}

	reboot_reason_map = get_reboot_reason_map();
	if (reboot_reason_map == NULL) {
		pr_err("reboot_reason_map is NULL\n");
		return;
	}
	for (i = 0; i < get_reboot_reason_map_size(); i++) {
		if (!strncmp((char *)reboot_reason_map[i].name, cmd, sizeof(reboot_reason_map[i].name))) {
			curr_reboot_type = reboot_reason_map[i].num;
			break;
		}
	}
	if (curr_reboot_type == UNKNOWN) {
		curr_reboot_type = COLDBOOT;
		console_verbose();
		dump_stack();
	}
	set_reboot_reason(curr_reboot_type);
}

void set_subtype_exception(unsigned int subtype, bool save_value)
{
	unsigned int value;

	if (g_reboot_reason_addr) {
		DST_PRINT_PN("[%s]:set 0x%x\n", __func__, subtype);
		value = readl((void *)(uintptr_t)g_reboot_reason_addr);
		value &= SUB_REBOOT_REASON_MASK;
		subtype &= 0xFF;
		value |= subtype << SUB_REBOOT_REASON_SHIFT;
		writel(value, (void *)(uintptr_t)g_reboot_reason_addr);
	} else {
		DST_PRINT_ERR("[%s]:set failed 0x%x\n", __func__, subtype);
	}

	return;
}

unsigned int get_subtype_exception(void)
{
	unsigned int value = 0xFF;

	if (g_reboot_reason_addr) {
		value = readl((void *)(uintptr_t)g_reboot_reason_addr);
		value = value >> SUB_REBOOT_REASON_SHIFT;
		value &= 0xFF;
		DST_PRINT_PN("[%s]: get:0x%x\n", __func__, value);
	}

	return value;
}

/*
base address:0x1600_0000
ID  offset  Desc                    comments
0	0x218	RSMRST_                    HW
1	0x218	STR                        HW
2	0x218	SD                         HW
3	0x218	WDT 2nd TIME-OUT           HW
4	0x218	WARM RESET                 HW
5	0x218	EXTERNAL RESET             HW
6	0x218	POWER BUTTON OVERRID       HW
7	0x218	CSU_SE WDT 2ND TIME-OUT    HW
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
	DST_PRINT_PN("[%s]: last hw reboot reason:0x%x (%s)\n", __func__, value, Desc);
}

/*
 * Description:    init bootup_keypoint_addr
 * Input:          NA
 * Output:         NA
 * Return:         NA
 */
static void reboot_reason_addr_init(void)
{
	static u64 hw_reboot_reason_addr = 0;

	g_reboot_reason_addr =
		(uintptr_t)ioremap_wc(REBOOT_REASON_ADDR, sizeof(int));

	if (g_reboot_reason_addr) {
		pr_info("last sw reboot reason:0x%x\n", readl((void *)(uintptr_t)g_reboot_reason_addr));
		set_reboot_reason_sky1(AP_S_COLDBOOT);
	}

	hw_reboot_reason_addr =
		(uintptr_t)ioremap_wc(SKY1_HW_REBOOT_REASON_ADDR, sizeof(int));

	if (hw_reboot_reason_addr) {
		print_hw_reboot_reason(readl((void *)(uintptr_t)hw_reboot_reason_addr));
	}
}

/*
 * Description:    set bootup_keypoint
 * Input:          NA
 * Output:         NA
 * Return:         OK:success
 */
static int __init reboot_reason_init(void)
{
	reboot_reason_addr_init();
	return 0;
}
early_initcall(reboot_reason_init);

// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Cix Technology Group Co., Ltd.*/

#include <linux/soc/cix/rdr_pub.h>
#include <linux/soc/cix/rdr_platform.h>
#include <mntn_subtype_exception.h>
#include <linux/soc/cix/dst_audio_timeout_error.h>
#include <linux/soc/cix/util.h>
#include "dst_print.h"

#define SKY1_AUDIO_ADDR_BASE (0x07000000)
#define SKY1_AUDIO_ADDR_END (0x07FFFFFF)

struct rdr_exception_info_s g_audio_einfo[] = {
	DEF_EXCE_STRUCT_SINGLE(RDR_ERR, RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP,
			       AP_PANIC, AP_PANIC_AUDIO, "audio timeout", 0,
			       NULL),
};

void sky1_check_audio_timeout_error(unsigned long far, struct pt_regs *regs)
{
	unsigned long phy_addr;
	bool user;

	user = user_mode(regs);
	phy_addr = dst_get_phy_addr(far);
	DST_DBG("far=0x%lx phy=0x%lx\n", far, phy_addr);

	if (phy_addr < SKY1_AUDIO_ADDR_BASE || phy_addr > SKY1_AUDIO_ADDR_END)
		return;
	DST_ERR("audio timeout, far=0x%lx phy=0x%lx\n", far, phy_addr);
	if (!user)
		rdr_system_error(MODID_AP_PANIC_AUDIO, phy_addr >> 32,
				 phy_addr & 0xffffffff);
}

static __init int dst_audio_timeout_error_init(void)
{
	for (int i = 0; i < ARRAY_SIZE(g_audio_einfo); i++) {
		if (!rdr_register_exception(&g_audio_einfo[i]))
			DST_ERR("register audio timeout error fail\n");
	}

	return 0;
}

late_initcall(dst_audio_timeout_error_init);

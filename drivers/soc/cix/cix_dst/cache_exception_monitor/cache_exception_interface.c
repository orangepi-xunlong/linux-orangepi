// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include "cache_exception_interface.h"
#include "../dst_print.h"
#include <asm/sysreg.h>
#include <linux/smp.h>

static void cache_set_enable_disable(int enable, int type)
{
	if ((read_sysreg_s(SYS_ERXFR_EL1) & 0x3) == 0x2) {
		write_sysreg_s(type, SYS_ERRSELR_EL1);
		isb();

		write_sysreg_s(enable, SYS_ERXCTLR_EL1);
		isb();
	}
}

static void cache_excep_init_regs(void *info, int enable)
{
	struct cache_excep_drvdata *data = (struct cache_excep_drvdata *)info;

	DST_PRINT_DBG("%s, init cache related registers\n", __func__);

	if (data->core_type == CORE_HAYES || data->core_type == CORE_HUNTER) {
		if (data->cpu != smp_processor_id()) {
			DST_PRINT_ERR("%s, core id doesn't match...[%d,%d]\n",
				      __func__, data->cpu, smp_processor_id());
			return;
		}

		if (data->core_type == CORE_HAYES) {
			cache_set_enable_disable(enable, CACHE_EXCEP_L1);
			cache_set_enable_disable(enable, CACHE_EXCEP_L2);
		} else if (data->core_type == CORE_HUNTER) {
			cache_set_enable_disable(enable, CACHE_EXCEP_CORE);
		}
	}

	if (data->core_type == CORE_DSU) {
		cache_set_enable_disable(enable, CACHE_EXCEP_DSU);
	}
}

void cache_excep_init(void *info)
{
	cache_excep_init_regs(info, 1);
}

void cache_excep_uninit(void *info)
{
	cache_excep_init_regs(info, 0);
}

void cache_excep_record(enum cache_excep_type type)
{
	DST_PRINT_PN("%s, dump type%d registers\n", __func__, type);

	write_sysreg_s(type, SYS_ERRSELR_EL1);
	isb();

	// dump registers
	DST_PRINT_PN("ERRIDR_EL1=0x%llx\n", read_sysreg_s(SYS_ERRIDR_EL1));
	DST_PRINT_PN("ERXFR_EL1=0x%llx\n", read_sysreg_s(SYS_ERXFR_EL1));
	DST_PRINT_PN("ERXCTLR_EL1=0x%llx\n", read_sysreg_s(SYS_ERXCTLR_EL1));
	DST_PRINT_PN("ERXSTATUS_EL1=0x%llx\n", read_sysreg_s(SYS_ERXSTATUS_EL1));
	DST_PRINT_PN("ERXADDR_EL1=0x%llx\n", read_sysreg_s(SYS_ERXADDR_EL1));
	DST_PRINT_PN("ERXMISC0_EL1=0x%llx\n", read_sysreg_s(SYS_ERXMISC0_EL1));
	DST_PRINT_PN("ERXMISC1_EL1=0x%llx\n", read_sysreg_s(SYS_ERXMISC1_EL1));

	// clear status
	write_sysreg_s(0xFFF80000, SYS_ERXSTATUS_EL1);
	isb();
}

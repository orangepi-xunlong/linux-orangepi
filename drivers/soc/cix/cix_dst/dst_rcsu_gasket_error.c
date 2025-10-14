// SPDX-License-Identifier: GPL-2.0
/* Copyright 2025 Cix Technology Group Co., Ltd.*/

#include <linux/io.h>
#include <linux/soc/cix/rdr_platform.h>
#include <linux/soc/cix/rdr_pub.h>
#include <mntn_subtype_exception.h>
#include "dst_print.h"

#define RCSU_SEC_FAIL_ADDR (0x180)
#define RCSU_SEC_FAIL_STATUS (0x184)

#define RCSU_SEC_INTR_CLEAR_MASK (1)
#define RCSU_FAIL_RW_MASK (1 << 1)
#define RCSU_FAIL_SEC_ACCESS_MASK (1 << 2)
#define RCSU_FAIL_ID_MASK (0x3F << 3)

#define RCSU_SEC_INTR_CLEAR_SHIFT (0)
#define RCSU_FAIL_RW_SHIFT (1)
#define RCSU_FAIL_SEC_ACCESS_SHIFT (2)
#define RCSU_FAIL_ID_SHIFT (3)

#define MAX_RCSU_NAME (20)

struct rcsu_dev {
	unsigned short is_monitored;
	char *name;
	unsigned long phys_addr;
	unsigned long mem_size;
	void *virt_addr;
};

#define RCSU_DEV_DEFINE(monitor, dname, addr) \
	{ .is_monitored = (monitor),          \
	  .name = #dname,                     \
	  .phys_addr = (addr),                \
	  .mem_size = 0x10000,                \
	  .virt_addr = NULL }

static struct rcsu_dev g_sky1_rcsu_devs[] = {
	RCSU_DEV_DEFINE(0, FCH_LOCAL, 0x0),
	RCSU_DEV_DEFINE(1, CSUSE_LOCAL, 0x05000000),
	RCSU_DEV_DEFINE(1, CSUPM_LOCAL, 0x06000000),
	RCSU_DEV_DEFINE(0, AUDIO_LOCAL, 0x0700000),
	RCSU_DEV_DEFINE(0, SF_LOCAL, 0x08000000),
	RCSU_DEV_DEFINE(0, USBC0_LOCAL, 0x09000000),
	RCSU_DEV_DEFINE(0, USBC1_LOCAL, 0x09070000),
	RCSU_DEV_DEFINE(0, USBC2_LOCAL, 0x090E0000),
	RCSU_DEV_DEFINE(0, USBC3_LOCAL, 0x09150000),
	RCSU_DEV_DEFINE(0, USB3_LOCAL, 0x091C0000),
	RCSU_DEV_DEFINE(0, USB20_LOCAL, 0x09250000),
	RCSU_DEV_DEFINE(0, USB21_LOCAL, 0x09280000),
	RCSU_DEV_DEFINE(0, USB22_LOCAL, 0x092B0000),
	RCSU_DEV_DEFINE(0, USB23_LOCAL, 0x092E0000),
	RCSU_DEV_DEFINE(0, ETH_LOCAL, 0x09310000),
	RCSU_DEV_DEFINE(0, PCIe_X8_LOCAL, 0x0A000000),
	RCSU_DEV_DEFINE(0, PCIe_X4211, 0x0A030000),
	RCSU_DEV_DEFINE(0, SMMU0_LOCAL, 0x0B000000),
	RCSU_DEV_DEFINE(0, SMMU1_LOCAL, 0x0B0D0000),
	RCSU_DEV_DEFINE(0, SMMU2_LOCAL, 0x0B1A0000),
	RCSU_DEV_DEFINE(0, DDR0_LOCAL, 0x0C000000),
	RCSU_DEV_DEFINE(0, DDR1_LOCAL, 0x0C020000),
	RCSU_DEV_DEFINE(0, DDR2_LOCAL, 0x0C040000),
	RCSU_DEV_DEFINE(0, DDR3_LOCAL, 0x0C060000),
	RCSU_DEV_DEFINE(0, BRCAST_LOCAL, 0x0C080000),
	RCSU_DEV_DEFINE(0, TZC0_LOCAL, 0x0C0A0000),
	RCSU_DEV_DEFINE(0, TZC1_LOCAL, 0x0C0C0000),
	RCSU_DEV_DEFINE(0, TZC2_LOCAL, 0x0C0E0000),
	RCSU_DEV_DEFINE(0, TZC3_LOCAL, 0x0C100000),
	RCSU_DEV_DEFINE(0, PCIe_HUB_LOCAL, 0x0D000000),
	RCSU_DEV_DEFINE(0, MMHUB_LOCAL, 0x0D030000),
	RCSU_DEV_DEFINE(0, SYS_HUB_LOCAL, 0x0D080000),
	RCSU_DEV_DEFINE(0, SMN_HUB_LOCAL, 0x0D150000),
	RCSU_DEV_DEFINE(0, GICD_LOCAL, 0x0E000000),
	RCSU_DEV_DEFINE(0, CORE_PCSM_LOCAL, 0x0FC50000),
	RCSU_DEV_DEFINE(0, CI_700_LOCAL, 0x10000000),
	RCSU_DEV_DEFINE(0, DPU0_LOCAL, 0x14000000),
	RCSU_DEV_DEFINE(1, DP0_LOCAL, 0x14050000),
	RCSU_DEV_DEFINE(0, DPU1_LOCAL, 0x14070000),
	RCSU_DEV_DEFINE(1, DP1_LOCAL, 0x140C0000),
	RCSU_DEV_DEFINE(0, DPU2_LOCAL, 0x140E0000),
	RCSU_DEV_DEFINE(1, DP2_LOCAL, 0x14130000),
	RCSU_DEV_DEFINE(0, DPU3_LOCAL, 0x14150000),
	RCSU_DEV_DEFINE(1, DP3_LOCAL, 0x141A0000),
	RCSU_DEV_DEFINE(0, DPU4_LOCAL, 0x141C0000),
	RCSU_DEV_DEFINE(1, DP4_LOCAL, 0x14210000),
	RCSU_DEV_DEFINE(0, VPU_LOCAL, 0x14230000),
	RCSU_DEV_DEFINE(1, NPU_LOCAL, 0x14250000),
	RCSU_DEV_DEFINE(0, MIPI0_LOCAL, 0x14270000),
	RCSU_DEV_DEFINE(0, MIPI1_LOCAL, 0x142D0000),
	RCSU_DEV_DEFINE(0, ISP0_LOCAL, 0x14330000),
	RCSU_DEV_DEFINE(0, ISP1_LOCAL, 0x14350000),
	RCSU_DEV_DEFINE(1, GPU_LOCAL, 0x15000000),
};

static struct rdr_exception_info_s g_rcsu_einfo[] = { DEF_EXCE_STRUCT_RANGE(
	MODID_RCSU_EXCEPTION_START, MODID_RCSU_EXCEPTION_END, RDR_ERR,
	RDR_REBOOT_NOW, RDR_AP, RDR_AP, RDR_AP, RCSU_EXCEPTION,
	RCSU_EXCEPTION_RES, "rcsu", 0, NULL) };

static void init_rcsu_dev(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(g_sky1_rcsu_devs); i++) {
		if (!g_sky1_rcsu_devs[i].is_monitored)
			continue;
		g_sky1_rcsu_devs[i].virt_addr =
			ioremap(g_sky1_rcsu_devs[i].phys_addr,
				g_sky1_rcsu_devs[i].mem_size);
		if (!g_sky1_rcsu_devs[i].virt_addr) {
			DST_ERR("ioremap for rcsu-%s failed, base=0x%08lx, size=0x%08lx\n",
				g_sky1_rcsu_devs[i].name,
				g_sky1_rcsu_devs[i].phys_addr,
				g_sky1_rcsu_devs[i].mem_size);
		}

		DST_DBG("phy=0x%08lx, virt=0x%px\n",
			g_sky1_rcsu_devs[i].phys_addr,
			g_sky1_rcsu_devs[i].virt_addr);
	}
}

void sky1_check_rcsu_gasket_error(void)
{
	int i;
	u32 fail_addr;
	u32 status;

	for (i = 0; i < ARRAY_SIZE(g_sky1_rcsu_devs); i++) {
		if (!g_sky1_rcsu_devs[i].is_monitored ||
		    !g_sky1_rcsu_devs[i].virt_addr)
			continue;

		status = readl_relaxed(g_sky1_rcsu_devs[i].virt_addr +
				       RCSU_SEC_FAIL_STATUS);
		if (!status)
			continue;

		fail_addr = readl_relaxed(g_sky1_rcsu_devs[i].virt_addr +
					  RCSU_SEC_FAIL_ADDR);
		DST_ERR("RCSU-0x%x:%s generate error, err_addr=0x%x status=0x%x\n",
			i, g_sky1_rcsu_devs[i].name, fail_addr, status);
		rdr_system_error(MODID_RCSU_EXCEPTION_START + i + 1, fail_addr,
				 status);
	}
}

static int __init dst_rcsu_init(void)
{
	init_rcsu_dev();
	return 0;
}

core_initcall(dst_rcsu_init);

static __init int dst_rcsu_error_init(void)
{
	for (int i = 0; i < ARRAY_SIZE(g_rcsu_einfo); i++) {
		if (!rdr_register_exception(&g_rcsu_einfo[i]))
			DST_ERR("register rcsu error fail\n");
	}

	return 0;
}

late_initcall(dst_rcsu_error_init);

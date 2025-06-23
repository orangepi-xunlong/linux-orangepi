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
#include "blackbox/rdr_utils.h"

#define RCSU_SEC_FAIL_ADDR		(0x180)
#define RCSU_SEC_FAIL_STATUS	(0x184)

#define RCSU_SEC_INTR_CLEAR_MASK	(1)
#define RCSU_FAIL_RW_MASK		(1 << 1)
#define RCSU_FAIL_SEC_ACCESS_MASK	(1 << 2)
#define RCSU_FAIL_ID_MASK		(0x3F << 3)

#define RCSU_SEC_INTR_CLEAR_SHIFT	(0)
#define RCSU_FAIL_RW_SHIFT		(1)
#define RCSU_FAIL_SEC_ACCESS_SHIFT	(2)
#define RCSU_FAIL_ID_SHIFT		(3)

#define MAX_RCSU_NAME (20)

struct rcsu_dev
{
	unsigned short id_num;
	unsigned short is_monitored;
	char name[MAX_RCSU_NAME];
	unsigned long phys_addr;
	unsigned long mem_size;
	void *virt_addr;
};

static struct rcsu_dev g_sky1_rcsu_devs[] = {
	{ 0x00, 0, "FCH_LOCAL",			0x00000000, 0x10000, NULL },
	{ 0x01, 1, "CSU_SE_LOCAL",		0x05000000, 0x10000, NULL },
	{ 0x02, 1, "CSU_PM_LOCAL",		0x06000000, 0x10000, NULL },
	{ 0x03, 0, "AUDIO_LOCAL",		0x07000000, 0x10000, NULL },
	{ 0x04, 0, "SF_LOCAL",			0x08000000, 0x10000, NULL },
	{ 0x05, 0, "USBC0_LOCAL",		0x09000000, 0x10000, NULL },
	{ 0x06, 0, "USBC1_LOCAL",		0x09070000, 0x10000, NULL },
	{ 0x07, 0, "USBC2_LOCAL",		0x090E0000, 0x10000, NULL },
	{ 0x08, 0, "USBC3_LOCAL",		0x09150000, 0x10000, NULL },
	{ 0x09, 0, "USB3_LOCAL",		0x091C0000, 0x10000, NULL },
	{ 0x0A, 0, "USB20_LOCAL",		0x09250000, 0x10000, NULL },
	{ 0x0B, 0, "USB21_LOCAL",		0x09280000, 0x10000, NULL },
	{ 0x0C, 0, "USB22_LOCAL",		0x092B0000, 0x10000, NULL },
	{ 0x0D, 0, "USB23_LOCAL",		0x092E0000, 0x10000, NULL },
	{ 0x0E, 0, "ETH_LOCAL",			0x09310000, 0x10000, NULL },
	{ 0x0F, 0, "PCIe_X8_LOCAL",		0x0A000000, 0x10000, NULL },
	{ 0x10, 0, "PCIe_X4211",		0x0A030000, 0x10000, NULL },
	{ 0x11, 0, "SMMU0_LOCAL",		0x0B000000, 0x10000, NULL },
	{ 0x12, 0, "SMMU1_LOCAL",		0x0B0D0000, 0x10000, NULL },
	{ 0x13, 0, "SMMU2_LOCAL",		0x0B1A0000, 0x10000, NULL },
	{ 0x14, 0, "DDR0_LOCAL",		0x0C000000, 0x10000, NULL },
	{ 0x15, 0, "DDR1_LOCAL",		0x0C020000, 0x10000, NULL },
	{ 0x16, 0, "DDR2_LOCAL",		0x0C040000, 0x10000, NULL },
	{ 0x17, 0, "DDR3_LOCAL",		0x0C060000, 0x10000, NULL },
	{ 0x18, 0, "BRCAST_LOCAL",		0x0C080000, 0x10000, NULL },
	{ 0x19, 0, "TZC0_LOCAL",		0x0C0A0000, 0x10000, NULL },
	{ 0x1A, 0, "TZC1_LOCAL",		0x0C0C0000, 0x10000, NULL },
	{ 0x1B, 0, "TZC2_LOCAL",		0x0C0E0000, 0x10000, NULL },
	{ 0x1C, 0, "TZC3_LOCAL",		0x0C100000, 0x10000, NULL },
	{ 0x1D, 0, "PCIe_HUB_LOCAL",	0x0D000000, 0x10000, NULL },
	{ 0x1E, 0, "MMHUB_LOCAL",		0x0D030000, 0x10000, NULL },
	{ 0x1F, 0, "SYS_HUB_LOCAL",		0x0D080000, 0x10000, NULL },
	{ 0x20, 0, "SMN_HUB_LOCAL",		0x0D150000, 0x10000, NULL },
	{ 0x21, 0, "GICD_LOCAL",		0x0E000000, 0x10000, NULL },
	{ 0x22, 0, "CORE_PCSM_LOCAL",	0x0FC50000, 0x10000, NULL },
	{ 0x23, 0, "CI_700_LOCAL",		0x10000000, 0x10000, NULL },
	{ 0x24, 0, "DPU0_LOCAL",		0x14000000, 0x10000, NULL },
	{ 0x25, 1, "DP0_LOCAL",			0x14050000, 0x10000, NULL },
	{ 0x26, 0, "DPU1_LOCAL",		0x14070000, 0x10000, NULL },
	{ 0x27, 1, "DP1_LOCAL",			0x140C0000, 0x10000, NULL },
	{ 0x28, 0, "DPU2_LOCAL",		0x140E0000, 0x10000, NULL },
	{ 0x29, 1, "DP2_LOCAL",			0x14130000, 0x10000, NULL },
	{ 0x2A, 0, "DPU3_LOCAL",		0x14150000, 0x10000, NULL },
	{ 0x2B, 1, "DP3_LOCAL",			0x141A0000, 0x10000, NULL },
	{ 0x2C, 0, "DPU4_LOCAL",		0x141C0000, 0x10000, NULL },
	{ 0x2D, 1, "DP4_LOCAL",			0x14210000, 0x10000, NULL },
	{ 0x2E, 0, "VPU_LOCAL",			0x14230000, 0x10000, NULL },
	{ 0x2F, 1, "NPU_LOCAL",			0x14250000, 0x10000, NULL },
	{ 0x30, 0, "MIPI0_LOCAL",		0x14270000, 0x10000, NULL },
	{ 0x31, 0, "MIPI1_LOCAL",		0x142D0000, 0x10000, NULL },
	{ 0x32, 0, "ISP0_LOCAL",		0x14330000, 0x10000, NULL },
	{ 0x33, 0, "ISP1_LOCAL",		0x14350000, 0x10000, NULL },
	{ 0x34, 1, "GPU_LOCAL",			0x15000000, 0x10000, NULL },
};

static void init_rcsu_dev(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(g_sky1_rcsu_devs); i++) {
		if (!g_sky1_rcsu_devs[i].is_monitored)
			continue;
		g_sky1_rcsu_devs[i].virt_addr = ioremap(g_sky1_rcsu_devs[i].phys_addr, g_sky1_rcsu_devs[i].mem_size);
		if (!g_sky1_rcsu_devs[i].virt_addr) {
			DST_PRINT_ERR("%s,%d: ioremap for rcsu-0x%02x failed, base=0x%08lx, size=0x%08lx \n", __func__,
			__LINE__, g_sky1_rcsu_devs[i].id_num, g_sky1_rcsu_devs[i].phys_addr, g_sky1_rcsu_devs[i].mem_size);
		}

		DST_PRINT_DBG("%s,%d: phy=0x%08lx, virt=0x%px \n",
				__func__, __LINE__, g_sky1_rcsu_devs[i].phys_addr, g_sky1_rcsu_devs[i].virt_addr);
	}
}

int sky1_check_rcsu_gasket_error(void)
{
	int i;
	int has_error = 0;
	u32 fail_addr;
	u32 status;

	for (i = 0; i < ARRAY_SIZE(g_sky1_rcsu_devs); i++) {
		if (!g_sky1_rcsu_devs[i].is_monitored || !g_sky1_rcsu_devs[i].virt_addr)
			continue;

		status = readl_relaxed((void*)((char*)g_sky1_rcsu_devs[i].virt_addr + RCSU_SEC_FAIL_STATUS));
		if (status) {
			fail_addr = readl_relaxed((void*)((char*)g_sky1_rcsu_devs[i].virt_addr + RCSU_SEC_FAIL_ADDR));
			DST_PRINT_ERR("%s,%d: RCSU-0x%x:%s generate error, err_addr=0x%x status=0x%x \n",
				__func__, __LINE__, i, g_sky1_rcsu_devs[i].name, fail_addr, status);
			has_error = 1;
			break;
		}
	}

	if (!has_error)
		DST_PRINT_DBG("%s,%d: no gasket error... \n", __func__, __LINE__);

	return has_error;
}

static int __init dst_rcsu_init(void)
{
	init_rcsu_dev();
	return 0;
}

core_initcall(dst_rcsu_init);

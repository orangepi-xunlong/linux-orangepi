#include <linux/types.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/defines.h>
#include <linux/sys_uapi.h>

#include "vo_sys.h"
#include "reg.h"
#include "sys_debug.h"

/* register bank */
#define TOP_RESET_BASE 0x28103000
#define TOP_RESET_SIZE 0x4

#define TOP_BASE 0x28100000
#define TOP_REG_BANK_SIZE 0x100
#define GP_REG3_OFFSET 0x8C

#define OTP_SHADOW_SYS 0x27102000
#define OTP_SHADOW_SYS_SIZE 0x100
#define BONDING_OFFSET 0x14

#define RTC_BASE 0x05026000
#define RTC_REG_BANK_SIZE 0x140
#define RTC_ST_ON_REASON 0xF8

static void __iomem *top_base;
static void __iomem *rtc_base;
static void __iomem *otp_shadow_sys_base;

/* top_rst */
static uintptr_t top_rst_reg_base;

unsigned int sys_comm_read_chip_id(void)
{
	unsigned int cpu_bonding = ioread32(otp_shadow_sys_base + BONDING_OFFSET) & 0x7;

	TRACE_SYS(DBG_DEBUG, "cpu_bonding=0x%x\n", cpu_bonding);

	switch (cpu_bonding) {
	case 0x0:
	case 0x7:
		return E_CHIPID_BM1688;
	case 0x1:
		return E_CHIPID_CV186AH;
	default:
		return E_CHIPID_UNKOWN;
	};

	return E_CHIPID_UNKOWN;
}
EXPORT_SYMBOL_GPL(sys_comm_read_chip_id);

unsigned int sys_comm_read_chip_version(void)
{
	unsigned int chip_version = 0;

	chip_version = ioread32(top_base);

	TRACE_SYS(DBG_DEBUG, "chip_version=0x%x\n", chip_version);

	switch (chip_version) {
	case 0x18802000:
	case 0x18220000:
	case 0x18100000:
	case 0x1686A200: // A2
		return E_CHIPVERSION_U01;
	case 0x18802001:
	case 0x18220001:
	case 0x18100001:
		return E_CHIPVERSION_U02;
	default:
		return E_CHIPVERSION_U01;
	}
}
EXPORT_SYMBOL_GPL(sys_comm_read_chip_version);

unsigned int sys_comm_read_chip_pwr_on_reason(void)
{
	unsigned int reason = 0;

	reason = ioread32(rtc_base + RTC_ST_ON_REASON);

	TRACE_SYS(DBG_DEBUG, "pwr on reason = 0x%x\n", reason);

	switch (reason) {
	case 0x800d0000:
	case 0x800f0000:
		return E_CHIP_PWR_ON_COLDBOOT;
	case 0x880d0003:
	case 0x880f0003:
		return E_CHIP_PWR_ON_WDT;
	case 0x80050009:
	case 0x800f0009:
		return E_CHIP_PWR_ON_SUSPEND;
	case 0x840d0003:
	case 0x840f0003:
		return E_CHIP_PWR_ON_WARM_RST;
	default:
		return E_CHIP_PWR_ON_COLDBOOT;
	}
}
EXPORT_SYMBOL_GPL(sys_comm_read_chip_pwr_on_reason);

// top_rst
union top_reset top_sys_get_reset(void)
{
	union top_reset reset;

	reset.raw = _reg_read(top_rst_reg_base);
	return reset;
}
EXPORT_SYMBOL_GPL(top_sys_get_reset);

void top_sys_set_reset(union top_reset reset)
{
	_reg_write(top_rst_reg_base, reset.raw);
}
EXPORT_SYMBOL_GPL(top_sys_set_reset);

int sys_comm_init(void)
{
	top_base = ioremap(TOP_BASE, TOP_REG_BANK_SIZE);
	rtc_base = ioremap(RTC_BASE, RTC_REG_BANK_SIZE);
	otp_shadow_sys_base = ioremap(OTP_SHADOW_SYS, OTP_SHADOW_SYS_SIZE);
	top_rst_reg_base = (uintptr_t)ioremap(TOP_RESET_BASE, TOP_RESET_SIZE);

	TRACE_SYS(DBG_NOTICE, "CVITEK CHIP ID = %d\n", sys_comm_read_chip_id());

	return 0;
}

void sys_comm_deinit(void)
{
	iounmap(top_base);
	iounmap(rtc_base);
	iounmap(otp_shadow_sys_base);
	iounmap((void *)top_rst_reg_base);
}
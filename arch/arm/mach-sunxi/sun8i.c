/*
 * Device Tree support for Allwinner A1X SoCs
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/clocksource.h>
#include <linux/clk-provider.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/sunxi-sid.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include "sunxi.h"

#ifdef CONFIG_SMP
extern struct smp_operations sunxi_smp_ops;
#endif

static struct map_desc sunxi_io_desc[] __initdata = {
	{
		.virtual	= (unsigned long) IO_ADDRESS(SUNXI_IO_PBASE),
		.pfn		= __phys_to_pfn(SUNXI_IO_PBASE),
		.length		= SUNXI_IO_SIZE,
		.type		= MT_DEVICE,
	},
#ifdef CONFIG_ARCH_SUN8IW10P1
	{
		.virtual	= (unsigned long)IO_ADDRESS(SUNXI_SRAM_A1_PBASE),
		.pfn		= __phys_to_pfn(SUNXI_SRAM_A1_PBASE),
		.length		= SUNXI_SRAM_A1_SIZE,
		.type		= MT_MEMORY_ITCM,
	},
	{
		.virtual        = (unsigned long)IO_ADDRESS(SUNXI_SRAM_C_PBASE),
		.pfn            = __phys_to_pfn(SUNXI_SRAM_C_PBASE),
		.length         = SUNXI_SRAM_C_SIZE,
		.type           = MT_MEMORY_ITCM,
	},

#endif
#if defined(CONFIG_ARCH_SUN8IW11P1)
	{
		.virtual    = (unsigned long)IO_ADDRESS(SUNXI_SRAM_A_PBASE),
		.pfn        = __phys_to_pfn(SUNXI_SRAM_A_PBASE),
		.length     = SUNXI_SRAM_A_SIZE,
		.type       = MT_MEMORY_ITCM,
	},
#endif
};

void __init sunxi_map_io(void)
{
	iotable_init(sunxi_io_desc, ARRAY_SIZE(sunxi_io_desc));
}

static void __init sunxi_timer_init(void)
{
	of_clk_init(NULL);
#ifdef CONFIG_COMMON_CLK_ENABLE_SYNCBOOT_EARLY
	clk_syncboot();
#endif
	clocksource_of_init();
}

static const char * const sunxi_board_dt_compat[] = {
	"allwinner,sun4i-a10",
	"allwinner,sun5i-a13",
	"arm,sun8iw10p1",
	"arm,sun8iw11p1",
	NULL,
};

DT_MACHINE_START(SUNXI_DT, CONFIG_SUNXI_SOC_NAME)
	.smp            = smp_ops(sunxi_smp_ops),
	.map_io		= sunxi_map_io,
	.init_time	= sunxi_timer_init,
	.dt_compat	= sunxi_board_dt_compat,
MACHINE_END

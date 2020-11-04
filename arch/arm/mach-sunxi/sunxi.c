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

#include <linux/clk-provider.h>
#include <linux/clocksource.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mcpm.h>

#include "sunxi.h"

void __iomem *sunxi_cpucfg_base;
void __iomem *sunxi_cpuscfg_base;
void __iomem *sunxi_sysctl_base;
void __iomem *sunxi_rtc_base;
void __iomem *sunxi_soft_entry_base;

static void __init sunxi_dt_cpufreq_init(void)
{
	platform_device_register_simple("cpufreq-dt", -1, NULL, 0);
}

static const char * const sunxi_board_dt_compat[] = {
	"allwinner,sun4i-a10",
	"allwinner,sun5i-a10s",
	"allwinner,sun5i-a13",
	"allwinner,sun5i-r8",
	NULL,
};

DT_MACHINE_START(SUNXI_DT, "Allwinner sun4i/sun5i Families")
	.dt_compat	= sunxi_board_dt_compat,
	.init_late	= sunxi_dt_cpufreq_init,
MACHINE_END

static const char * const sun6i_board_dt_compat[] = {
	"allwinner,sun6i-a31",
	"allwinner,sun6i-a31s",
	NULL,
};

extern void __init sun6i_reset_init(void);
static void __init sun6i_timer_init(void)
{
	of_clk_init(NULL);
	if (IS_ENABLED(CONFIG_RESET_CONTROLLER))
		sun6i_reset_init();
	clocksource_probe();
}

DT_MACHINE_START(SUN6I_DT, "Allwinner sun6i (A31) Family")
	.init_time	= sun6i_timer_init,
	.dt_compat	= sun6i_board_dt_compat,
	.init_late	= sunxi_dt_cpufreq_init,
MACHINE_END

static const char * const sun7i_board_dt_compat[] = {
	"allwinner,sun7i-a20",
	NULL,
};

DT_MACHINE_START(SUN7I_DT, "Allwinner sun7i (A20) Family")
	.dt_compat	= sun7i_board_dt_compat,
	.init_late	= sunxi_dt_cpufreq_init,
MACHINE_END

static struct map_desc sunxi_io_desc[] __initdata = {
	{
		.virtual = (unsigned long) UARTIO_ADDRESS(SUNXI_UART_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_UART_PBASE),
		.length  = SUNXI_UART_SIZE,
		.type    = MT_DEVICE,
	},
#if defined(CONFIG_ARCH_SUN8IW12P1)
	{
		.virtual = (unsigned long) IO_ADDRESS(ARISC_MESSAGE_POOL_PBASE),
		.pfn     = __phys_to_pfn(ARISC_MESSAGE_POOL_PBASE),
		.length  = ARISC_MESSAGE_POOL_RANGE,
		.type    = MT_MEMORY_RWX_ITCM,
	},
#endif
#if defined(CONFIG_ARCH_SUN8IW10P1)
	{
		.virtual = (unsigned long)IO_ADDRESS(SUNXI_SRAM_A1_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_SRAM_A1_PBASE),
		.length  = SUNXI_SRAM_A1_SIZE,
		.type    = MT_MEMORY_RWX_ITCM,
	},
	{
		.virtual = (unsigned long)IO_ADDRESS(SUNXI_SRAM_C_PBASE),
		.pfn     = __phys_to_pfn(SUNXI_SRAM_C_PBASE),
		.length  = SUNXI_SRAM_C_SIZE,
		.type    = MT_MEMORY_RWX_ITCM,
	},
#endif
#if defined(CONFIG_ARCH_SUN8IW6P1)
	{
		.virtual	= (unsigned long)IO_ADDRESS(SUNXI_SRAM_A1_PBASE),
		.pfn		= __phys_to_pfn(SUNXI_SRAM_A1_PBASE),
		.length		= SUNXI_SRAM_A1_SIZE,
		.type		= MT_MEMORY_RWX_ITCM,
	},

	{
		.virtual        = (unsigned long)IO_ADDRESS(SUNXI_SRAM_A2_PBASE),
		.pfn            = __phys_to_pfn(SUNXI_SRAM_A2_PBASE),
		.length         = SUNXI_SRAM_A2_SIZE,
		.type           = MT_DEVICE_NONSHARED,
	},

#endif
};

void __init sunxi_map_io(void)
{
	iotable_init(sunxi_io_desc, ARRAY_SIZE(sunxi_io_desc));
}

#ifdef CONFIG_ARCH_SUN8IW15P1
/* sun8iw15p1 has't map io addr space,it will ioremap failed,so ignore it */
static void __init sunxi_cpucfg_init(void)
{
}
static void __init sunxi_sysctl_init(void)
{
}
static void __init sunxi_cpuscfg_init(void)
{
}
static void __init sunxi_rtc_init(void)
{
}
#else
static void __init sunxi_cpucfg_init(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL,
						"allwinner,sunxi-cpucfg");
	if (!np) {
		pr_err("Can not find sunxi_cpucfg device tree\n");
		return;
	}

	sunxi_cpucfg_base = of_iomap(np, 0);
	if (!sunxi_cpucfg_base)
		pr_err("sunxi_cpucfg_base iomap Failed\n");

	of_node_put(np);
}

static void __init sunxi_sysctl_init(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "allwinner,sunxi-sysctl");
	if (!np) {
		pr_err("Can not find sunxi_sysctl device tree\n");
		return;
	}

	sunxi_sysctl_base = of_iomap(np, 0);
	if (!sunxi_sysctl_base) {
		pr_err("sunxi_sysctl iomap Failed\n");
		goto node_put;
	}

	if (of_property_read_bool(np, "cpu-soft-entry"))
		sunxi_soft_entry_base = sunxi_sysctl_base;

node_put:
	of_node_put(np);
}

static void __init sunxi_rtc_init(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "allwinner,sunxi-rtc");
	if (!np) {
		pr_err("Can not find sunxi_rtc device tree\n");
		return;
	}

	sunxi_rtc_base = of_iomap(np, 0);
	if (!sunxi_rtc_base) {
		pr_err("sunxi_rtc iomap Failed\n");
		goto node_put;
	}

	sunxi_soft_entry_base = sunxi_rtc_base;

node_put:
	of_node_put(np);
}

static void __init sunxi_cpuscfg_init(void)
{
	struct device_node *np;

	np = of_find_compatible_node(NULL, NULL, "allwinner,sunxi-cpuscfg");
	if (!np) {
		pr_warn("Can not find sunxi_cpuscfg device tree\n");
		return;
	}

	sunxi_cpuscfg_base = of_iomap(np, 0);
	if (!sunxi_cpuscfg_base)
		pr_err("sunxi_cpuscfg iomap Failed\n");

	if (of_property_read_bool(np, "cpu-soft-entry"))
		sunxi_soft_entry_base = sunxi_cpuscfg_base;

	of_node_put(np);
}
#endif

static __attribute__((unused)) void __init sunxi_init_early(void)
{
	sunxi_cpucfg_init();
	sunxi_sysctl_init();
	sunxi_cpuscfg_init();

	/*
	 * If we could not find cpu-soft-entry in sunxi-sysctl node,
	 * the sunxi_soft_entry is in sunxi-rtc region.
	 */
	if (!sunxi_soft_entry_base)
		sunxi_rtc_init();

	if (sunxi_cpucfg_base != NULL)
		pr_debug("sunxi_cpucfg_base: %p\n", sunxi_cpucfg_base);

	if (sunxi_cpuscfg_base != NULL)
		pr_debug("sunxi_cpuscfg_base: %p\n", sunxi_cpuscfg_base);

	if (sunxi_sysctl_base != NULL)
		pr_debug("sunxi_sysctl_base: %p\n", sunxi_sysctl_base);

	if (sunxi_soft_entry_base != NULL)
		pr_debug("sunxi_soft_entry_base: %p\n", sunxi_soft_entry_base);
}

static struct platform_device sunxi_cpuidle = {
	.name = "sunxi_cpuidle",
};

static void __init sunxi_init_late(void)
{
	if (of_machine_is_compatible("allwinner,sun8iw11p1") ||
		of_machine_is_compatible("allwinner,sun8iw12p1") ||
		of_machine_is_compatible("allwinner,sun8iw15p1") ||
		of_machine_is_compatible("allwinner,sun8iw7p1") ||
		of_machine_is_compatible("allwinner,sun8iw6p1"))
		platform_device_register(&sunxi_cpuidle);
}

static const char * const sun8i_board_dt_compat[] = {
	"allwinner,sun8i-a23",
	"allwinner,sun8i-a33",
	"allwinner,sun8i-h3",
	"allwinner,sun8iw11p1",
	"allwinner,sun8iw12p1",
	"allwinner,sun8iw15p1",
	"allwinner,sun8iw7p1",
	"allwinner,sun8iw6p1",
	NULL,
};

DT_MACHINE_START(SUN8I_DT, CONFIG_SUNXI_SOC_NAME)
	.init_time	= sun6i_timer_init,
	.map_io		= sunxi_map_io,
#ifdef CONFIG_ARCH_SUN8IW6P1
	.smp_init       = smp_init_ops(mcpm_smp_set_ops),
#endif
	.init_early	= NULL,
	.init_late	= sunxi_init_late,
	.dt_compat	= sun8i_board_dt_compat,
MACHINE_END

static const char * const sun9i_board_dt_compat[] = {
	"allwinner,sun9i-a80",
	NULL,
};

DT_MACHINE_START(SUN9I_DT, "Allwinner sun9i Family")
	.dt_compat	= sun9i_board_dt_compat,
MACHINE_END

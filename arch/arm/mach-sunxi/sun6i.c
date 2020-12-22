/*
 * Support for Allwinner A3X SoCs
 *
 * Copyright (c) Allwinner.  All rights reserved.
 *
 * Sugar (shuge@allwinnertech.com)
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/io.h>
#include <linux/sunxi_timer.h>
#include <linux/platform_device.h>
#include <linux/serial_8250.h>
#include <linux/param.h>

#include <asm/hardware/gic.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <mach/hardware.h>

#include "sunxi.h"

#define GIC_DIST_BASE	0xf1c81000
#define GIC_CPU_BASE	0xf1c82000

#define WATCHDOG_BASE	0xf1f01000
#define WATCHDOG_INT_REG	0x00
#define WATCHDOG_CTRL_REG	0x14
#define WATCHDOG_MODE_REG	0x18

#define AW_UART0_V_BASE	0xf1c28000
#define AW_UART0_P_BASE	0x01c28000

/* uart */
static struct plat_serial8250_port serial_platform_data[] = {
	{
		.membase        = (void __iomem *)(AW_UART0_V_BASE),
		.mapbase        = (resource_size_t)AW_UART0_P_BASE,
		.irq            = AW_IRQ_UART0,
		.flags          = UPF_BOOT_AUTOCONF|UPF_IOREMAP,
		.iotype         = UPIO_MEM32,
		.regshift       = 2,
		.uartclk        = 24000000,
	}, {
		.flags          = 0,
	}
 };

static struct platform_device serial_dev = {
	.name = "serial8250",
	.id = PLAT8250_DEV_PLATFORM,
	.dev = {
		.platform_data = &serial_platform_data[0],
	}
};

static struct platform_device *sunxi_dev[] __initdata = {
	&serial_dev,
};

static void sun6i_restart(char mode, const char *cmd)
{
	writel(0, (void __iomem *)(WATCHDOG_BASE + WATCHDOG_INT_REG));
	writel(0x01, (void __iomem *)(WATCHDOG_BASE + WATCHDOG_CTRL_REG));
	writel((0x1 << 5) | 0x01, (void __iomem *)(WATCHDOG_BASE + WATCHDOG_CTRL_REG));
	while(1);
}

static struct map_desc sunxi_io_desc[] __initdata = {
	{
		.virtual	= (unsigned long) IO_ADDRESS(SUNXI_SRAM1_BASE),
		.pfn		= __phys_to_pfn(SUNXI_SRAM1_BASE),
		.length		= SUNXI_SRAM1_SIZE,
		.type		= MT_MEMORY_ITCM,
	},
	{
		.virtual	= (unsigned long) IO_ADDRESS(SUNXI_SRAM2_BASE),
		.pfn		= __phys_to_pfn(SUNXI_SRAM2_BASE),
		.length		= SUNXI_SRAM2_SIZE,
		.type		= MT_DEVICE_NONSHARED,
	},
	{
		.virtual	= (unsigned long) SUNXI_REGS_VIRT_BASE,
		.pfn		= __phys_to_pfn(SUNXI_REGS_PHYS_BASE),
		.length		= SUNXI_REGS_SIZE,
		.type		= MT_DEVICE,
	},
};

static void __init sun6i_fixup(struct tag *tags, char **from,
			       struct meminfo *meminfo)
{
	struct tag *t;

	for (t = tags; t->hdr.size; t = tag_next(t)) {
		if (t->hdr.tag == ATAG_MEM && t->u.mem.size) {
			pr_debug("[%s]: From boot, get meminfo:\n"
					"\tStart:\t0x%08x\n"
					"\tSize:\t%dMB\n",
					__func__,
					t->u.mem.start,
					t->u.mem.size >> 20);
			return;
		}
	}
	early_printk("[%s] enter\n", __func__);

	meminfo->bank[0].start = PLAT_PHYS_OFFSET;
	meminfo->bank[0].size = PLAT_MEM_SIZE;
	meminfo->nr_banks = 1;

	early_printk("nr_banks: %d, bank.start: 0x%08x, bank.size: 0x%08x\n",
			meminfo->nr_banks, meminfo->bank[0].start,
			meminfo->bank[0].size);
}

void __init sun6i_reserve(void)
{
	/* Add any reserve memory to here */
}

#ifndef CONFIG_OF
static void __init sun6i_gic_init(void)
{
	gic_init(0, 29, (void __iomem *)GIC_DIST_BASE, (void __iomem *)GIC_CPU_BASE);
}
#endif

void __init sunxi_map_io(void)
{
	iotable_init(sunxi_io_desc, ARRAY_SIZE(sunxi_io_desc));
}

static void __init sunxi_dev_init(void)
{
#ifdef CONFIG_OF
	of_platform_populate(NULL, of_default_bus_match_table, NULL, NULL);
#else
	platform_add_devices(sunxi_dev, ARRAY_SIZE(sunxi_dev));
#endif
}

MACHINE_START(SUN6I, "Allwinner A3X")
	.atag_offset	= 0x100,
	.init_machine	= sunxi_dev_init,
	.map_io		= sunxi_map_io,
#ifndef CONFIG_OF
	.init_irq	= sun6i_gic_init,
#endif
	.handle_irq	= gic_handle_irq,
	.restart	= sun6i_restart,
	.timer		= &sunxi_timer,
	.dt_compat	= NULL,
	.reserve	= NULL,
	.fixup		= sun6i_fixup,
	.nr_irqs	= (AW_IRQ_GIC_START + 128),
#ifdef CONFIG_SMP
	.smp		= smp_ops(sunxi_smp_ops),
#endif
MACHINE_END

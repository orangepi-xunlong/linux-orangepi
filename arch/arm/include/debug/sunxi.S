/*
 * Early serial output macro for Allwinner A1X SoCs
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <mach/platform.h>

#if defined(CONFIG_DEBUG_SUNXI_UART0)
#define SUNXI_UART_DEBUG_PHYS_BASE SUNXI_UART0_PBASE
#define SUNXI_UART_DEBUG_VIRT_BASE SUNXI_UART0_VBASE
#elif defined(CONFIG_DEBUG_SUNXI_UART1)
#define SUNXI_UART_DEBUG_PHYS_BASE SUNXI_UART1_PBASE
#define SUNXI_UART_DEBUG_VIRT_BASE SUNXI_UART1_VBASE
#elif defined(CONFIG_DEBUG_SUNXI_UART2)
#define SUNXI_UART_DEBUG_PHYS_BASE SUNXI_UART2_PBASE
#define SUNXI_UART_DEBUG_VIRT_BASE SUNXI_UART2_VBASE
#endif

	.macro	addruart, rp, rv, tmp
	ldr	\rp, =SUNXI_UART_DEBUG_PHYS_BASE
	ldr	\rv, =SUNXI_UART_DEBUG_VIRT_BASE
	.endm

#define UART_SHIFT	2
#include <asm/hardware/debug-8250.S>

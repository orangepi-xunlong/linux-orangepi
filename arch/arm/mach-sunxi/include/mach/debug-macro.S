/* arch/arm/mach-sun6i/include/mach/debug-macro.S
 *
 * Debugging macro include header
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * Benn Huang (benn@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <mach/platform.h>
#include <asm/hardware/debug-8250.S>

#ifdef CONFIG_DEBUG_SUNXI_UART0
#define UART_PADDR	SUNXI_UART0_PBASE
#define UART_VADDR	(0xf0000000 + SUNXI_UART0_PBASE)
	
#elif defined (CONFIG_DEBUG_SUNXI_UART1)
#define UART_PADDR	SUNXI_UART1_PBASE
#define UART_VADDR	(0xf0000000 + SUNXI_UART1_PBASE)

#elif defined (CONFIG_DEBUG_SUNXI_UART2)
#define UART_PADDR	SUNXI_UART2_PBASE
#define UART_VADDR	(0xf0000000 + SUNXI_UART2_PBASE)

#else
#error "Please select a uart port"
#endif

	.macro	addruart, rp, rv, tmp
	ldr	\rv, =UART_VADDR
	ldr	\rp, =UART_PADDR
	.endm

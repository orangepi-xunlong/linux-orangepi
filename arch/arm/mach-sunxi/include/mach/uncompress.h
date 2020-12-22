/*
 *  arch/arm/mach-sun6i/include/mach/uncompress.h
 *
 * Copyright (c) Allwinner.  All rights reserved.
 * Benn Huang (benn@allwinnertech.com) 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __MACH_UNCOMPRESS_H
#define __MACH_UNCOMPRESS_H

#include <asm/mach-types.h>
#include <asm/processor.h>
#include <asm/barrier.h>
#include <mach/platform.h>
#include <linux/bitops.h>

#define UART_TF		(*(volatile unsigned long*)(SUNXI_UART0_PBASE + 0x00))
#define UART_SR		(*(volatile unsigned long*)(SUNXI_UART0_PBASE + 0x7C))
#define TX_BUSY		BIT(1)
/*
 * put the character through uart
 */
static inline void putc(int c)
{
#if 0
	while(!(UART_SR & TX_BUSY))
		cpu_relax();
	UART_TF = c;
#endif
}
static inline void flush(void) {}
static inline void arch_decomp_setup(void) {}

#define arch_decomp_setup()
#define arch_decomp_wdog()

#endif /* __MACH_UNCOMPRESS_H */

/*
 * Generic definitions for Allwinner SunXi SoCs
 *
 * Copyright (C) 2021 huafenghuang
 *
 * huafenghuang  <huafenghuang@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __MACH_SUNXI_H
#define __MACH_SUNXI_H

#if !defined(SUNXI_UART_PBASE)
#define SUNXI_UART_PBASE		CONFIG_DEBUG_UART_PHYS
#define SUNXI_UART_SIZE			UL(0x2000)
#endif

#if !defined(UARTIO_ADDRESS)
#define UARTIO_ADDRESS(x)  IOMEM((x) + 0xf0000000)
#endif

#endif /* __MACH_SUNXI_H */

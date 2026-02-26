/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's errcode for audio
 *
 * Copyright (c) 2023, emma<liujuan1@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef __SUNXI_ERR_UART_NG_H__
#define __SUNXI_ERR_UART_NG_H__

enum sunxi_err_uart_ng_func {
	TTY_FLIP = 0x0,
	DMA_CHAN,
	DMA_CONFIG,
	DMA_MAP_SG,
	DMA_PREP_SLAVE_SG,
	DMA_PREP_CYCLIC,
	DMA_ALLOC_COHERENT,
	IRQ_DTS,
	IRQ_GET,
	MEM,
	PORT,
	TYPE,
	FIFO_SIZE,
	DMA_INIT,
};

enum sunxi_err_uart_ng {
	/* E_UART_NG_HW_XXX	= E_UART_NG_HW_ERR0	| E_USER(XXX), */

	E_UART_NG_SW_DEP_DMA_CHAN		= E_UART_NG_SW_DEP_ERR0	| E_USER(DMA_CHAN),
	E_UART_NG_SW_DEP_DMA_CONFIG		= E_UART_NG_SW_DEP_ERR0	| E_USER(DMA_CONFIG),
	E_UART_NG_SW_DEP_DMA_MAP_SG		= E_UART_NG_SW_DEP_ERR0	| E_USER(DMA_MAP_SG),
	E_UART_NG_SW_DEP_DMA_PREP_SLAVE_SG	= E_UART_NG_SW_DEP_ERR0	| E_USER(DMA_PREP_SLAVE_SG),
	E_UART_NG_SW_DEP_DMA_PREP_CYCLIC	= E_UART_NG_SW_DEP_ERR0	| E_USER(DMA_PREP_CYCLIC),
	E_UART_NG_SW_DEP_DMA_ALLOC_COHERENT	= E_UART_NG_SW_DEP_ERR0	| E_USER(DMA_ALLOC_COHERENT),
	E_UART_NG_SW_DEP_MEM			= E_UART_NG_SW_DEP_ERR0	| E_USER(MEM),
	E_UART_NG_SW_DEP_DMA_INIT		= E_UART_NG_SW_DEP_ERR0	| E_USER(DMA_INIT),

	E_UART_NG_SYS_TTY_FLIP			= E_UART_NG_SW_SYS_ERR0	| E_USER(TTY_FLIP),
	E_UART_NG_SYS_IRQ_GET			= E_UART_NG_SW_SYS_ERR0	| E_USER(IRQ_GET),

	E_UART_NG_ARG_IRQ_DTS			= E_UART_NG_SW_ARG_ERR0	| E_USER(IRQ_DTS),
	E_UART_NG_ARG_PORT			= E_UART_NG_SW_ARG_ERR0	| E_USER(PORT),
	E_UART_NG_ARG_TYPE			= E_UART_NG_SW_ARG_ERR0	| E_USER(TYPE),
	E_UART_NG_ARG_FIFO_SIZE			= E_UART_NG_SW_ARG_ERR0	| E_USER(FIFO_SIZE),
};

#endif

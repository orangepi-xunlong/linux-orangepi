// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef __CIX_CLK_H__
#define __CIX_CLK_H__

#include <linux/clk-provider.h>

#ifndef MODULE
void cix_uart_clocks_register(void);
#else
static inline void cix_uart_clocks_register(void)
{
}
#endif
#endif

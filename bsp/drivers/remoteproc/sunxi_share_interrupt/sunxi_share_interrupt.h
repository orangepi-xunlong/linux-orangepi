/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's Remote Processor Share Interrupt Platform Head File
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SUNXI_SHARE_INTERRUPT_H
#define _SUNXI_SHARE_INTERRUPT_H

struct share_irq_res_table {
	uint32_t major;
	uint32_t hwirq;
	uint32_t arch_irq;
	uint32_t softirq;
};

int sunxi_arch_interrupt_save(const char *name);
int sunxi_arch_interrupt_restore(const char *name);
uint32_t sunxi_rproc_get_gpio_mask(int softirq);
uint32_t sunxi_rproc_get_gpio_mask_by_hwirq(int hwirq);

#endif

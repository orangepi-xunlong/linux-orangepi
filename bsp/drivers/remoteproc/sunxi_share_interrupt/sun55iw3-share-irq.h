/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's Remote Processor Share Interrupt Head File
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#define ARM_IRQn(_x)		(_x - 32)

static struct share_irq_res_table share_irq_table[] = {
		/* major  host coreq hwirq  remore core irq */
		/* table for e906 */
		{1,       ARM_IRQn(101)}, /* GPIOB */
		{2,       ARM_IRQn(103)}, /* GPIOC */
		{3,       ARM_IRQn(105)}, /* GPIOD */
		{4,       ARM_IRQn(107)}, /* GPIOE */
		{5,       ARM_IRQn(109)}, /* GPIOF */
		{6,       ARM_IRQn(111)}, /* GPIOG */
		{7,       ARM_IRQn(113)}, /* GPIOH */
		{8,       ARM_IRQn(115)}, /* GPIOI */
		{9,       ARM_IRQn(117)}, /* GPIOJ */
		{10,      ARM_IRQn(172)}, /* GPIOK */
		{11,      ARM_IRQn(191)}, /* GPIOL */
		{12,      ARM_IRQn(193)}, /* GPIOM */
		/* table for hifi4 */
		{13,      ARM_IRQn(101)}, /* GPIOB */
		{14,      ARM_IRQn(103)}, /* GPIOC */
		{15,      ARM_IRQn(105)}, /* GPIOD */
		{16,      ARM_IRQn(107)}, /* GPIOE */
		{17,      ARM_IRQn(109)}, /* GPIOF */
		{18,      ARM_IRQn(111)}, /* GPIOG */
		{19,      ARM_IRQn(113)}, /* GPIOH */
		{20,      ARM_IRQn(115)}, /* GPIOI */
		{21,      ARM_IRQn(117)}, /* GPIOJ */
		{22,      ARM_IRQn(172)}, /* GPIOK */
		{23,      ARM_IRQn(191)}, /* GPIOL */
		{24,      ARM_IRQn(193)}, /* GPIOM */
};



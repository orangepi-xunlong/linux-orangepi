/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
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

#ifndef _DT_BINDINGS_SHATE_INTERRUPT_H
#define _DT_BINDINGS_SHATE_INTERRUPT_H

/* sunxi share-interrupt format
 * major type flags
 *
 * Generic Type Define
 * type = 0x0 --- normal irq
 *
 * GPIO Type Define
 * type = 0x1 --- GPIOA
 * type = 0x2 --- GPIOB
 * type = 0x3 --- GPIOC
 * type = 0x4 --- GPIOD
 * type = 0x5 --- GPIOE
 * type = 0x6 --- GPIOF
 * type = 0x7 --- GPIOG
 * type = 0x8 --- GPIOH
 * type = 0x9 --- GPIOI
 * ...
 */
#define SH_GPIOA_IRQ  1 /* major */
#define SH_GPIOC_IRQ  2 /* major */
#define SH_GPIOD_IRQ  3 /* major */
#define SH_GPIOE_IRQ  4 /* major */
#define SH_GPIOF_IRQ  5 /* major */
#define SH_GPIOG_IRQ  6 /* major */
#define SH_GPIOH_IRQ  7 /* major */
#define SH_GPIOI_IRQ  8 /* major */

#define SH_GPIOA      0x1 /* type */
#define SH_GPIOC      0x3 /* type */
#define SH_GPIOD      0x4 /* type */
#define SH_GPIOE      0x5 /* type */
#define SH_GPIOF      0x6 /* type */
#define SH_GPIOG      0x7 /* type */
#define SH_GPIOH      0x8 /* type */
#define SH_GPIOI      0x9 /* type */

#endif

/* SPDX-License-Identifier: GPL-2.0 */
/*
 * sunxi's Remote Processor Share Interrupt Platform Head File
 *
 * Copyright (C) 2023 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _SUN55IW3_SHARE_INTERRUPT_DT_H
#define _SUN55IW3_SHARE_INTERRUPT_DT_H

#define RINTC_IRQ_MASK			0xffff0000
#define RISCV_IRQn(major, sub)		((major) * 100 + (sub))
#define DSP_IRQn(major, sub)		(((sub) * 200 + (major)) | RINTC_IRQ_MASK)
#define ARM_IRQn(_x)			((_x) - 32)

/* sunxi share-interrupt format
 * major type remote_core_irq flags
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
 * type = 0xa --- GPIOJ
 * type = 0xb --- GPIOK
 * type = 0xc --- GPIOL
 * type = 0xd --- GPIOM
 * type = 0xe --- GPION
 */

#define RISCV_GPIOB	(1	0x2	RISCV_IRQn(98, 6)	ARM_IRQn(101))
#define RISCV_GPIOC	(2	0x3	RISCV_IRQn(98, 8)	ARM_IRQn(103))
#define RISCV_GPIOD	(3	0x4	RISCV_IRQn(99, 2)	ARM_IRQn(105))
#define RISCV_GPIOE	(4	0x5	RISCV_IRQn(99, 4)	ARM_IRQn(107))
#define RISCV_GPIOF	(5	0x6	RISCV_IRQn(99, 6)	ARM_IRQn(109))
#define RISCV_GPIOG	(6	0x7	RISCV_IRQn(99, 8)	ARM_IRQn(111))
#define RISCV_GPIOH	(7	0x8	RISCV_IRQn(100, 2)	ARM_IRQn(113))
#define RISCV_GPIOI	(8	0x9	RISCV_IRQn(100, 4)	ARM_IRQn(115))
#define RISCV_GPIOJ	(9	0xa	RISCV_IRQn(100, 6)	ARM_IRQn(117))
#define RISCV_GPIOK	(10	0xb	RISCV_IRQn(107, 5)	ARM_IRQn(172))
#define RISCV_GPIOL	(11	0xc	RISCV_IRQn(63, 0)	ARM_IRQn(191))
#define RISCV_GPIOM	(12	0xd	RISCV_IRQn(65, 0)	ARM_IRQn(193))

#define DSP_GPIOB	(13	0x2	DSP_IRQn(46, 6)		ARM_IRQn(101))
#define DSP_GPIOC	(14	0x3	DSP_IRQn(46, 8)		ARM_IRQn(103))
#define DSP_GPIOD	(15	0x4	DSP_IRQn(47, 2)		ARM_IRQn(105))
#define DSP_GPIOE	(16	0x5	DSP_IRQn(47, 4)		ARM_IRQn(107))
#define DSP_GPIOF	(17	0x6	DSP_IRQn(47, 6)		ARM_IRQn(109))
#define DSP_GPIOG	(18	0x7	DSP_IRQn(47, 8)		ARM_IRQn(111))
#define DSP_GPIOH	(19	0x8	DSP_IRQn(48, 2)		ARM_IRQn(113))
#define DSP_GPIOI	(20	0x9	DSP_IRQn(48, 4)		ARM_IRQn(115))
#define DSP_GPIOJ	(21	0xa	DSP_IRQn(48, 6)		ARM_IRQn(117))
#define DSP_GPIOK	(22	0xb	DSP_IRQn(55, 5)		ARM_IRQn(172))
#define DSP_GPIOL	(23	0xc	DSP_IRQn(2, 0)		ARM_IRQn(191))
#define DSP_GPIOM	(24	0xd	DSP_IRQn(4, 0)		ARM_IRQn(193))

#endif

/*
 * arch/arm/mach-sunxi/include/mach/irqs.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi irq defination header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_MACH_IRQS_H
#define __SUNXI_MACH_IRQS_H

#define SUNXI_GIC_START          32

#if defined CONFIG_ARCH_SUN8IW1P1
#include "sun8i/irqs-sun8iw1p1.h"
#elif defined CONFIG_ARCH_SUN8IW3P1
#include "sun8i/irqs-sun8iw3p1.h"
#elif defined CONFIG_ARCH_SUN8IW5P1
#include "sun8i/irqs-sun8iw5p1.h"
#elif defined CONFIG_ARCH_SUN8IW6P1
#include "sun8i/irqs-sun8iw6p1.h"
#elif defined CONFIG_ARCH_SUN8IW7P1
#include "sun8i/irqs-sun8iw7p1.h"
#elif defined CONFIG_ARCH_SUN8IW8P1
#include "sun8i/irqs-sun8iw8p1.h"
#elif defined CONFIG_ARCH_SUN8IW9P1
#include "sun8i/irqs-sun8iw9p1.h"
#elif defined CONFIG_ARCH_SUN9IW1P1
#include "sun9i/irqs-sun9iw1p1.h"
#else
#error "please select a platform\n"
#endif

#define MAX_GIC_NR              1

/* irq total number = gic irq max + gpio irq max(reserve 256) */
#define NR_IRQS                 (SUNXI_IRQ_MAX + 256)

#ifndef NR_IRQS
#error "NR_IRQS not defined by the board-specific files"
#endif

#endif

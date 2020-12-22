/*
 * arch/arm/mach-sunxi/include/mach/platform.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi platform header file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_PLATFORM_H
#define __SUNXI_PLATFORM_H

#include "hardware.h"

#if defined CONFIG_ARCH_SUN8IW1P1
#include "sun8i/platform-sun8iw1p1.h"
#elif defined CONFIG_ARCH_SUN8IW3P1
#include "sun8i/platform-sun8iw3p1.h"
#elif defined CONFIG_ARCH_SUN8IW5P1
#include "sun8i/platform-sun8iw5p1.h"
#elif defined CONFIG_ARCH_SUN8IW6P1
#include "sun8i/platform-sun8iw6p1.h"
#elif defined CONFIG_ARCH_SUN8IW7P1
#include "sun8i/platform-sun8iw7p1.h"
#elif defined CONFIG_ARCH_SUN8IW8P1
#include "sun8i/platform-sun8iw8p1.h"
#elif defined CONFIG_ARCH_SUN8IW9P1
#include "sun8i/platform-sun8iw9p1.h"
#elif defined(CONFIG_ARCH_SUN9IW1P1)
#include "sun9i/platform-sun9iw1p1.h"
#else
#error "please select a platform\n"
#endif

#define MAGIC0                (0x00001650)

#endif

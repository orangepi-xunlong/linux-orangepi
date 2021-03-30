/**
 * Copyright (C) 2015-2016 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: Albert Yu <yuxyun@allwinnertech.com>
 */

#ifndef _CONFIG_H_
#define _CONFIG_H_

#if defined CONFIG_ARCH_SUN8IW3P1
#include "sun8i/sun8iw3p1.h"
#elif defined CONFIG_ARCH_SUN8IW5P1
#include "sun8i/sun8iw5p1.h"
#elif defined CONFIG_ARCH_SUN8IW7P1
#include "sun8i/sun8iw7p1.h"
#elif defined CONFIG_ARCH_SUN8IW11P1
#include "sun8i/sun8iw11p1.h"
#elif defined CONFIG_ARCH_SUN50IW1P1
#include "sun50i/sun50iw1p1.h"
#else
#error "please select a platform\n"
#endif

#endif /* _CONFIG_H_ */

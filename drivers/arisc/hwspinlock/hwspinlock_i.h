/*
 *  arch/arm/mach-sunxi/arisc/hwspinlock/hwspinlock-i.h
 *
 * Copyright (c) 2012 Allwinner.
 * 2012-05-01 Written by sunny (sunny@allwinnertech.com).
 * 2012-10-01 Written by superm (superm@allwinnertech.com).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __HW_SPINLOCK_I_H
#define __HW_SPINLOCK_I_H

#include "../include/arisc_includes.h"

/* the used state of spinlock */
#define SPINLOCK_FREE       (0)
#define SPINLOCK_USED       (1)

#endif  /* __HW_SPINLOCK_I_H */

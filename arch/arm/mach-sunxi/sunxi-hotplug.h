/*
 * arch/arm/mach-sunxi/sunxi-hotplug.h
 *
 * Copyright(c) 2013-2015 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: liugang <liugang@allwinnertech.com>
 *
 * sunxi hotplug head file
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_HOTPLUG_H
#define __SUNXI_HOTPLUG_H

int  sunxi_cpu_kill(unsigned int cpu);
void sunxi_cpu_die(unsigned int cpu);
int  sunxi_cpu_disable(unsigned int cpu);

#endif /* __SUNXI_HOTPLUG_H */

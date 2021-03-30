#ifndef _STANDBY_H
#define _STANDBY_H

/*
 * Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/power/aw_pm.h>
#include <linux/power/axp_depend.h>
#include "common.h"
#include "../pm.h"
#include "standby_dram.h"

#ifndef IO_ADDRESS
#define IO_ADDRESS(x)  ((x) + 0xf0000000)
#endif

#define readb(addr)		(*((volatile unsigned char  *)(addr)))
#define readw(addr)		(*((volatile unsigned short *)(addr)))
#define readl(addr)		(*((volatile unsigned long  *)(addr)))
#define writeb(v, addr)	(*((volatile unsigned char  *)(addr)) = (unsigned char)(v))
#define writew(v, addr)	(*((volatile unsigned short *)(addr)) = (unsigned short)(v))
#define writel(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))

extern char *__bss_start;
extern char *__bss_end;
typedef void (*losc_enter_ss_func)(void);

#endif /*_PM_H*/

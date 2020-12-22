/*
 *  arch/arm/mach-sunxi/arisc/include/arisc_dbgs.h
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

#ifndef __ARISC_DBGS_H
#define __ARISC_DBGS_H

/*
 * debug level define,
 * level 0 : dump debug information--none;
 * level 1 : dump debug information--information;
 * level 2 : dump debug information--information+warning;
 * level 3 : dump debug information--information+warning+error;
 * extern void printk(const char *, ...);
 */

#ifdef ARISC_DEBUG_ON
/* debug levels */
#define DEBUG_LEVEL_INF    ((u32)1 << 0)
#define DEBUG_LEVEL_LOG    ((u32)1 << 1)
#define DEBUG_LEVEL_WRN    ((u32)1 << 2)
#define DEBUG_LEVEL_ERR    ((u32)1 << 3)

#define ARISC_INF(format, args...)                          \
	do {if(DEBUG_LEVEL_INF & (0xf0 >> (arisc_debug_level +1)))  \
		pr_debug("[ARISC] :"format, ##args);}while (0)

#define ARISC_LOG(format, args...)                                      \
	do {if(DEBUG_LEVEL_LOG & (0xf0 >> (arisc_debug_level +1)))	\
		printk(KERN_NOTICE "[ARISC] :"format, ##args);}while (0)

#define ARISC_WRN(format, args...)                          \
	do {if(DEBUG_LEVEL_WRN & (0xf0 >> (arisc_debug_level +1)))  \
		printk(KERN_WARNING "[ARISC WARING] :"format, ##args);}while (0)

#define ARISC_ERR(format, args...)                          \
	do {if(DEBUG_LEVEL_ERR & (0xf0 >> (arisc_debug_level +1)))  \
		printk(KERN_ERR "[ARISC ERROR] :"format, ##args);}while (0)

#else /* ARISC_DEBUG_ON */
#define ARISC_INF(...)
#define ARISC_WRN(...)
#define ARISC_ERR(...)
#define ARISC_LOG(...)

#endif /* ARISC_DEBUG_ON */

/* report error information id */
#define ERR_NMI_INT_TIMEOUT    (0x1)

#endif /* __ARISC_DBGS_H */

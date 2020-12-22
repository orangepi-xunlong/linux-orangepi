/*
 *  arch/arm/mach-sun6i/arisc/include/arisc_bgs.h
 *
 * Copyright (c) 2012 Allwinner.
 * sunny (sunny@allwinnertech.com)
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

#ifndef	__ARISC_DBGS_H__
#define	__ARISC_DBGS_H__

//debug level define,
//level 0 : dump debug information--none;
//level 1 : dump debug information--error;
//level 2 : dump debug information--error+warning;
//level 3 : dump debug information--error+warning+information;
#define DEBUG_ENABLE

//extern void printk(const char *, ...);
#if		(ARISC_DEBUG_LEVEL == 0)
#define	ARISC_INF(...)
#define	ARISC_WRN(...)
#define	ARISC_ERR(...)
#define	ARISC_LOG(...)
#elif 	(ARISC_DEBUG_LEVEL == 1)
#define	ARISC_INF(...)
#define	ARISC_WRN(...)
#define	ARISC_ERR(...)		printk(__VA_ARGS__)
#define	ARISC_LOG(...)		printk(__VA_ARGS__)
#elif 	(ARISC_DEBUG_LEVEL == 2)
#define	ARISC_INF(...)
#define	ARISC_WRN(...)		printk(__VA_ARGS__)
#define	ARISC_ERR(...)		printk(__VA_ARGS__)
#define	ARISC_LOG(...)		printk(__VA_ARGS__)
#elif 	(ARISC_DEBUG_LEVEL == 3)
#define	ARISC_INF(...)		printk(__VA_ARGS__)
#define	ARISC_WRN(...)		printk(__VA_ARGS__)
#define	ARISC_ERR(...)		printk(__VA_ARGS__)
#define	ARISC_LOG(...)		printk(__VA_ARGS__)
#endif	//ARISC_DEBUG_LEVEL

#endif	//__ARISC_DBGS_H__

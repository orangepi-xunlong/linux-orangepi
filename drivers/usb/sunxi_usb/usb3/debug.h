/*
 * drivers/usb/sunxi_usb/udc/usb3/debugfs.h
 * (C) Copyright 2013-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangjx, 2014-3-14, create this file
 *
 * usb3.0 contoller debugfs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "core.h"

//#define  DMSG_PRINT(stuff...)		printk(stuff)
#define  DMSG_PRINT(fmt,stuff...)		pr_debug(fmt,##stuff)

//#define  DMSG_PRINT_EX(stuff...)		printk(stuff)
#define  DMSG_PRINT_EX(fmt,stuff...)		pr_err(fmt,##stuff)

//#define  DMSG_ERR(...)        		(DMSG_PRINT_EX("WRN:L%d(%s):", __LINE__, __FILE__), DMSG_PRINT_EX(__VA_ARGS__))
#define  DMSG_ERR(fmt,...)        		DMSG_PRINT_EX("WRN:L%d(%s):"fmt, __LINE__, __FILE__,##__VA_ARGS__)

#if  0
    #define DMSG_TEST         			DMSG_PRINT
#else
    #define DMSG_TEST(...)
#endif

#if  0
    #define DMSG_MANAGER_DEBUG          DMSG_PRINT
#else
    #define DMSG_MANAGER_DEBUG(...)
#endif

#if  0
    #define DMSG_DEBUG        			DMSG_PRINT
#else
    #define DMSG_DEBUG(...)
#endif

#if  1
    #define DMSG_INFO         			DMSG_PRINT
#else
    #define DMSG_INFO(...)
#endif

#if	1
    #define DMSG_PANIC        			DMSG_ERR
#else
    #define DMSG_PANIC(...)
#endif

#if	0
    #define DMSG_WRN        			DMSG_ERR
#else
    #define DMSG_WRN(...)
#endif



#ifdef CONFIG_DEBUG_FS
extern int sunxi_debugfs_init(struct sunxi_otgc *);
extern void sunxi_debugfs_exit(struct sunxi_otgc *);
#else
static inline int sunxi_debugfs_init(struct sunxi_otgc *d)
{  return 0;  }
static inline void sunxi_debugfs_exit(struct sunxi_otgc *d)
{  }
#endif


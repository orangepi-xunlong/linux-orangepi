#ifndef _PM_ERR_CODE_H
#define _PM_ERR_CODE_H

/*
 * Copyright (c) 2011-2015 yanggq.young@allwinnertech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

 #define BEFORE_EARLY_SUSPEND		(0x00)
 #define RESUME0_START			(0x20)	    
 #define RESUME1_START			(0x40)
 #define BEFORE_LATE_RESUME		(0x60)
 #define LATE_RESUME_START		(0x80)
 #define CLK_RESUME_START		(0xA0)
 #define AFTER_LATE_RESUME		(0xC0)
 #define RESUME_COMPLETE_FLAG		(AFTER_LATE_RESUME | 0xf)
 #define FIRST_BOOT_FLAG		(0x0) 
 
 #define SHUTDOWN_FLOW			(0x10)
 #define OTA_FLOW			(0x30)

#if defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN9IW1P1)
 #define BOOT_UPGRAGE_FLAG		(0x5a)		     //Notice: 0x5a is occupied by boot for upgrade.
#else 
 #define BOOT_UPGRAGE_FLAG		(0x5AA5A55A)	    //Notice: 0x5a is occupied by boot for upgrade.
#endif

 #define BOOT_SECOND_START		(0x70)
 #define UP_CPU_START			(0x90)
#endif /*_PM_ERR_CODE_H*/

/*
 * arch/arm/mach-sun6i/pm/standby/standby_arisc.h
 *
 * Copyright 2012 (c) Allwinner.
 * sunny (sunny@allwinnertech.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#ifndef	__ASM_ARCH_STANDBY_A100_H
#define	__ASM_ARCH_STANDBY_A100_H

//the sync mode between arisc and ac327
#define STANDBY_ARISC_SYNC (1<<1)
#define STANDBY_ARISC_ASYNC (1<<2)
 
 int standby_arisc_init(void);
 int standby_arisc_exit(void);
 
/*
 * notify arisc to wakeup: restore cpus freq, volt, and init_dram.
 * para:  mode.
 * STANDBY_ARISC_SYNC:
 * STANDBY_ARISC_ASYNC:
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int standby_arisc_notify_restore(unsigned long mode);
/*
 * check arisc restore status.
 * para:  none.
 * return: result, 0 - restore completion successed, !0 - notify failed;
 */
int standby_arisc_check_restore_status(void);
/*
 * query standby wakeup source.
 * para:  point of buffer to store wakeup event informations.
 * return: result, 0 - query successed, !0 - query failed;
 */
int standby_arisc_query_wakeup_src(unsigned long *event);
/*
 * enter normal standby.
 * para:  parameter for enter normal standby.
 * return: result, 0 - normal standby successed, !0 - normal standby failed;
 */
int standby_arisc_standby_normal(struct normal_standby_para *para);


#endif	//__ASM_ARCH_STANDBY_A100_H

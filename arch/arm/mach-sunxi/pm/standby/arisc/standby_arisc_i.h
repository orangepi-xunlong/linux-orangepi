/*
 *  arch/arm/mach-sun6i/pm/standby/arisc/standby_arisc_i.h
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

#ifndef	__STANDBY_ARISC_I_H__
#define	__STANDBY_ARISC_I_H__

#include "./../standby_i.h"
#include <linux/power/aw_pm.h>
#include <linux/arisc/hwmsgbox.h>
#include "arisc_cfgs.h"
#include "arisc_messages.h"
#include "arisc_dbgs.h"
#include "./../standby_arisc.h"
#include "asm-generic/errno-base.h"

extern unsigned long arisc_sram_a2_vbase;

//hwspinlock interfaces
int arisc_hwspinlock_init(void);
int arisc_hwspinlock_exit(void);
int arisc_hwspin_lock_timeout(int hwid, unsigned int timeout);
int arisc_hwspin_unlock(int hwid);

//hwmsgbox interfaces
int arisc_hwmsgbox_init(void);
int arisc_hwmsgbox_exit(void);
int arisc_hwmsgbox_send_message(struct arisc_message *pmessage, unsigned int timeout);
int arisc_hwmsgbox_feedback_message(struct arisc_message *pmessage, unsigned int timeout);
struct arisc_message *arisc_hwmsgbox_query_message(void);

//message manager interfaces
int arisc_message_manager_init(void);
int arisc_message_manager_exit(void);
struct arisc_message *arisc_message_allocate(unsigned int msg_attr);
void arisc_message_free(struct arisc_message *pmessage);

#endif	//__STANDBY_ARISC_I_H__

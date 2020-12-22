/*
 *  arch/arm/mach-sun6i/arisc/hwspinlock/hwspinlock.c
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

#include "standby_arisc_i.h"
#include "./../../mem_hwspinlock.h"

/*
*********************************************************************************************************
*                                       INITIALIZE HWSPINLOCK
*
* Description: 	initialize hwspinlock.
*
* Arguments  : 	none.
*
* Returns    : 	0 if initialize hwspinlock succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_hwspinlock_init(void)
{
	return 0;
}

/*
*********************************************************************************************************
*                                       	EXIT HWSPINLOCK
*
* Description: 	exit hwspinlock.
*
* Arguments  : 	none.
*
* Returns    : 	0 if exit hwspinlock succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_hwspinlock_exit(void)
{
	return 0;
}

/*
*********************************************************************************************************
*                                       	LOCK HWSPINLOCK WITH TIMEOUT
*
* Description:	lock an hwspinlock with timeout limit.
*
* Arguments  : 	hwid : an hwspinlock id which we want to lock.
*
* Returns    : 	0 if lock hwspinlock succeeded, other if failed.
*********************************************************************************************************
*/
int arisc_hwspin_lock_timeout(int hwid, unsigned int timeout)
{
	//try to take spinlock
	while (readl(IO_ADDRESS(MEM_SPINLOCK_LOCK_REG(hwid))) == MEM_SPINLOCK_TAKEN) {
		/*
		 * The lock is already taken, let's check if the user wants
		 * us to try again
		 */
		 ;
	}
	
	return 0;
}

/*
*********************************************************************************************************
*                                       	UNLOCK HWSPINLOCK
*
* Description:	unlock a specific hwspinlock.
*
* Arguments  : 	hwid : an hwspinlock id which we want to unlock.
*
* Returns    : 	0 if unlock hwspinlock succeeded, other if failed.
*********************************************************************************************************
*/
int arisc_hwspin_unlock(int hwid)
{
	//untaken the spinlock
	writel(0x0, IO_ADDRESS(MEM_SPINLOCK_LOCK_REG(hwid)));
		
	return 0;
}

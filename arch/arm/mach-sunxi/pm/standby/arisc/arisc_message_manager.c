/*
 *  arch/arm/mach-sun6i/arisc/message_manager/message_manager.c
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

//the start and end of message pool
static struct arisc_message *message_start;
static struct arisc_message *message_end;

/*
*********************************************************************************************************
*                                       INIT MESSAGE MANAGER
*
* Description: 	initialize message manager.
*
* Arguments  : 	none.
*
* Returns    : 	0 if initialize succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_message_manager_init(void)
{
	//initialize message pool start and end
	message_start = (struct arisc_message *)(arisc_sram_a2_vbase + ARISC_MESSAGE_POOL_START);
	message_end   = (struct arisc_message *)(arisc_sram_a2_vbase + ARISC_MESSAGE_POOL_END);
	
	return 0;
}

/*
*********************************************************************************************************
*                                       EXIT MESSAGE MANAGER
*
* Description: 	exit message manager.
*
* Arguments  : 	none.
*
* Returns    : 	0 if exit succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_message_manager_exit(void)
{
	return 0;
}

/*
*********************************************************************************************************
*                                       ALLOCATE MESSAGE FRAME
*
* Description: 	allocate one message frame. mainly use for send message by message-box,
*			   	the message frame allocate form messages pool shared memory area.
*
* Arguments  : 	none.
*
* Returns    : 	the pointer of allocated message frame, NULL if failed;
*********************************************************************************************************
*/
struct arisc_message *arisc_message_allocate(unsigned int msg_attr)
{
	struct arisc_message *pmessage = (struct arisc_message *)(NULL);
	struct arisc_message *palloc   = (struct arisc_message *)(NULL);
	
	//use spinlock 0 to exclusive with arisc.
	arisc_hwspin_lock_timeout(0, ARISC_SPINLOCK_TIMEOUT);
	
	//seach from the start of message pool every time.
	//maybe have other more good choice.
	//by sunny at 2012-5-13 10:36:50.
	pmessage = message_start;
	while (pmessage < message_end) {
		if (pmessage->state == ARISC_MESSAGE_FREED) {
			//find free message in message pool, allocate it.
			palloc = pmessage;
			palloc->state  = ARISC_MESSAGE_ALLOCATED;
			ARISC_INF("message allocate from message pool\n");
			break;
		}
		//next message frame
		pmessage++;
	}
	
	//unlock hwspinlock 0
	arisc_hwspin_unlock(0);
	
	if (palloc == (struct arisc_message *)(NULL)) {
		ARISC_ERR("allocate message frame fail\n");
		return palloc;
	}

	/* initialize messgae frame */
	palloc->next = (struct arisc_message *)(NULL);
	palloc->attr = msg_attr;
	
	return palloc;
}

/*
*********************************************************************************************************
*                                       FREE MESSAGE FRAME
*
* Description: 	free one message frame. mainly use for process message finished, 
*			   	free it to messages pool or add to free message queue.
*
* Arguments  : 	pmessage : the pointer of free message frame.
*
* Returns    : 	none.
*********************************************************************************************************
*/
void arisc_message_free(struct arisc_message *pmessage)
{
	//free to message pool directly.
	//set message state as FREED.
	arisc_hwspin_lock_timeout(0, ARISC_SPINLOCK_TIMEOUT);
	pmessage->state = ARISC_MESSAGE_FREED;
	pmessage->next  = (struct arisc_message *)(NULL);
	arisc_hwspin_unlock(0);
}

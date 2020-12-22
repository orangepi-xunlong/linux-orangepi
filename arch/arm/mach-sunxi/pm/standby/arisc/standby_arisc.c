/*
 * arch/arm/mach-sun6i/include/mach/arisc.h
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

#include "standby_arisc_i.h"

//sram a2 virtual base address
unsigned long arisc_sram_a2_vbase = (unsigned long)IO_ADDRESS(AW_SRAM_A2_BASE);

//record wakeup event information
static unsigned long wakeup_event;

//record restore status information
static unsigned long restore_completed;

int standby_arisc_init(void)
{
	//initialize wakeup_event as invalid
	wakeup_event = 0;
	
	//initialize restore as uncompleted
	restore_completed = 0;
	
	//initialize hwspinlock
	arisc_hwspinlock_init();
	
	//initialize hwmsgbox
	arisc_hwmsgbox_init();
	
	//initialize message manager
	arisc_message_manager_init();
	
	return 0;
}

int standby_arisc_exit(void)
{
	//exit message manager
	arisc_message_manager_exit();
	
	//exit hwmsgbox
	arisc_hwmsgbox_exit();
	
	//exit hwspinlock
	arisc_hwspinlock_exit();
	
	restore_completed = 0;
	
	wakeup_event = 0;
	
	return 0;
}

/*
 * standby_arisc_notify_restore: 
 * function: notify arisc to wakeup: restore cpus freq, volt, and init_dram.
 * para:  mode.
 * STANDBY_ARISC_SYNC:
 * STANDBY_ARISC_ASYNC:
 * return: result, 0 - notify successed, !0 - notify failed;
 */
int standby_arisc_notify_restore(unsigned long mode)
{
	struct arisc_message *pmessage;
	unsigned char         attr;
	
	//allocate a message frame
	pmessage = arisc_message_allocate(0);
	if (pmessage == (struct arisc_message *)(NULL)) {
		ARISC_ERR("allocate message for normal-standby notify restore failed\n");
		return -ENOMEM;
	}
	//initialize message attributes
	attr = 0;
	if (mode & STANDBY_ARISC_SYNC) {
		attr |= ARISC_MESSAGE_ATTR_HARDSYN;
	}
	//initialize message
	pmessage->type     = ARISC_NSTANDBY_RESTORE_REQ;
	pmessage->attr     = attr;
	pmessage->state    = ARISC_MESSAGE_INITIALIZED;
	
	//send enter normal-standby restore request to arisc
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
	
	//syn message must free message frame by self.
	if (mode & STANDBY_ARISC_SYNC) {
		arisc_message_free(pmessage);
	}
	return 0;
}

/* 
 * standby_arisc_check_restore_status
 * function: check arisc restore status.
 * para:  none.
 * return: result, 0 - restore completion successed, !0 - notify failed;
 */
int standby_arisc_check_restore_status(void)
{
	struct arisc_message *pmessage;
	
	//check restore completion flag first
	if (restore_completed) {
		//arisc restore completion already.
		return 0;
	}
	//try ot query message for hwmsgbox
	pmessage = arisc_hwmsgbox_query_message();
	if (pmessage == (struct arisc_message *)(NULL)) {
		//no valid message
		return -EINVAL;
	}
	if (pmessage->type == ARISC_NSTANDBY_RESTORE_COMPLETE) {
		//restore complete message received
		pmessage->state = ARISC_MESSAGE_PROCESSED;
		if ((pmessage->attr & ARISC_MESSAGE_ATTR_SOFTSYN) || 
			(pmessage->attr & ARISC_MESSAGE_ATTR_HARDSYN)) {
			//synchronous message, should feedback process result.
			arisc_hwmsgbox_feedback_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
		} else {
			//asyn message, no need feedback message result,
			//free message directly.
			arisc_message_free(pmessage);	
		}
		//arisc restore completion
		restore_completed = 1;
		return 0;
	} else {
		ARISC_ERR("invalid message received when check restore status\n");
		return -EINVAL;
	}
	
	//not received restore completion
	return -EINVAL;
}

/* 
 * standby_arisc_query_wakeup_src
 * function: query standby wakeup source.
 * para:  point of buffer to store wakeup event informations.
 * return: result, 0 - query successed, !0 - query failed;
 */
int standby_arisc_query_wakeup_src(unsigned long *event)
{
	struct arisc_message *pmessage;
	
	//check parameter valid or not
	if (event == (unsigned long *)(NULL)) {
		ARISC_WRN("invalid buffer to query wakeup source\n");
		return -EINVAL;
	}
	
	//check wakeup event received first
	if (wakeup_event) {
		//arisc wakeup event received already.
		*event = wakeup_event;
		return 0;
	}
	//try ot query message for hwmsgbox
	pmessage = arisc_hwmsgbox_query_message();
	if (pmessage == (struct arisc_message *)(NULL)) {
		//no valid message
		return -EINVAL;
	}
	if (pmessage->type == ARISC_NSTANDBY_WAKEUP_NOTIFY) {
		//wakeup notify message received
		wakeup_event = pmessage->paras[0];
		pmessage->state = ARISC_MESSAGE_PROCESSED;
		if ((pmessage->attr & ARISC_MESSAGE_ATTR_SOFTSYN) || 
			(pmessage->attr & ARISC_MESSAGE_ATTR_HARDSYN)) {
			//synchronous message, should feedback process result.
			arisc_hwmsgbox_feedback_message(pmessage, ARISC_SEND_MSG_TIMEOUT);
		} else {
			//asyn message, no need feedback message result,
			//free message directly.
			arisc_message_free(pmessage);	
		}
		//arisc wakeup_event valid
		ARISC_INF("standby wakeup source : 0x%x\n", (unsigned int)wakeup_event);
		*event = wakeup_event;
		return 0;
	} else {
		ARISC_ERR("invalid message received when check restore status\n");
		return -EINVAL;
	}
	//not received wakup event
	return -EINVAL;
}

/*
 * standby_arisc_standby_normal
 * function: enter normal standby.
 * para:  parameter for enter normal standby.
 * return: result, 0 - normal standby successed, !0 - normal standby failed;
 */
int standby_arisc_standby_normal(struct normal_standby_para *para)
{
	struct arisc_message *pmessage;
	int result = 0;
	
	//allocate a message frame
	pmessage = arisc_message_allocate(0);
	if (pmessage == (struct arisc_message *)(NULL)) {
		ARISC_ERR("allocate message for normal-standby request failed\n");
		return -ENOMEM;
	}
	
	//check normal_standby_para size valid or not
	if (sizeof(struct normal_standby_para) > sizeof(pmessage->paras)) {
		ARISC_ERR("normal-standby parameters number too long\n");
		return -EINVAL;
	}
	//initialize message
	pmessage->type     = ARISC_NSTANDBY_ENTER_REQ;
	standby_memcpy((void *)(pmessage->paras), para, sizeof(struct normal_standby_para));
	pmessage->state    = ARISC_MESSAGE_INITIALIZED;
	
	//send enter normal-standby request to arisc
	arisc_hwmsgbox_send_message(pmessage, ARISC_SEND_MSG_TIMEOUT);

	//syn message, free it after feedbacked.

	if (pmessage->attr != 0) {
		result = pmessage->result;
		arisc_message_free(pmessage);
	}
	
	return result;
}

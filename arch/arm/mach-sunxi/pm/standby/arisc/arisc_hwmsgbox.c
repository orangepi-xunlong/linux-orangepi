/*
 *  arch/arm/mach-sun6i/arisc/hwmsgbox/hwmsgbox.c
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

//local functions
int arisc_hwmsgbox_clear_receiver_pending(int queue, int user);
int arisc_hwmsgbox_query_receiver_pending(int queue, int user);
int arisc_hwmsgbox_enable_receiver_int(int queue, int user);
int arisc_hwmsgbox_set_receiver(int queue, int user);
int arisc_hwmsgbox_set_transmitter(int queue, int user);
int arisc_hwmsgbox_wait_message_feedback(struct arisc_message *pmessage);
int arisc_hwmsgbox_message_feedback(struct arisc_message *pmessage);
int arisc_message_valid(struct arisc_message *pmessage);

/*
*********************************************************************************************************
*                                       	INITIALIZE HWMSGBOX
*
* Description: 	initialize hwmsgbox.
*
* Arguments  : 	none.
*
* Returns    : 	0 if initialize hwmsgbox succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_hwmsgbox_init(void)
{
	return 0;
}

/*
*********************************************************************************************************
*                                       	EXIT HWMSGBOX
*
* Description: 	exit hwmsgbox.
*
* Arguments  : 	none.
*
* Returns    : 	0 if exit hwmsgbox succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_hwmsgbox_exit(void)
{
	return 0;
}

/*
*********************************************************************************************************
*                                       SEND MESSAGE BY HWMSGBOX
*
* Description: 	send one message to another processor by hwmsgbox.
*
* Arguments  : 	pmessage 	: the pointer of sended message frame.
*				timeout		: the wait time limit when message fifo is full,
*							  it is valid only when parameter mode = HWMSG_SEND_WAIT_TIMEOUT.
*
* Returns    : 	0 if send message succeeded, other if failed.
*********************************************************************************************************
*/
int arisc_hwmsgbox_send_message(struct arisc_message *pmessage, unsigned int timeout)
{
	volatile unsigned long value;
	
	if (pmessage == (struct arisc_message *)(NULL)) {
		return -EINVAL;
	}
	if (pmessage->attr & ARISC_MESSAGE_ATTR_HARDSYN) {
		//use ac327 hwsyn transmit channel.
		while (readl(IO_ADDRESS(AW_MSGBOX_FIFO_STATUS_REG(ARISC_HWMSGBOX_AC327_SYN_TX_CH))) == 1) {
			//message-queue fifo is full, waiting always
			;
		}
		value = ((volatile unsigned long)pmessage) - arisc_sram_a2_vbase; 
		ARISC_INF("ac327 send hard syn message : %x\n", (unsigned int)value);
		writel(value, IO_ADDRESS(AW_MSGBOX_MSG_REG(ARISC_HWMSGBOX_AC327_SYN_TX_CH)));
		
		//hwsyn messsage must feedback use syn rx channel
		while (readl(IO_ADDRESS(AW_MSGBOX_MSG_STATUS_REG(ARISC_HWMSGBOX_AC327_SYN_RX_CH))) == 0) {
			//message not valid
			;
		}
		//check message valid
		if (value != (readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(ARISC_HWMSGBOX_AC327_SYN_RX_CH))))) {
			ARISC_ERR("hard syn message error\n");
			return -EINVAL;
		}
		ARISC_INF("ac327 hard syn message [%x, %x] feedback\n", (unsigned int)value, (unsigned int)pmessage->type);
		return 0;
	}
	
	//use ac327 asyn transmit channel.
	while (readl(IO_ADDRESS(AW_MSGBOX_FIFO_STATUS_REG(ARISC_HWMSGBOX_ARISC_ASYN_RX_CH))) == 1) {
		//message-queue fifo is full, waiting always
		;
	}
	//write message to message-queue fifo.
	value = ((volatile unsigned long)pmessage) - arisc_sram_a2_vbase; 
	ARISC_LOG("ac327 send soft syn or asyn message : %x\n", (unsigned int)value);
	writel(value, IO_ADDRESS(AW_MSGBOX_MSG_REG(ARISC_HWMSGBOX_ARISC_ASYN_RX_CH)));
	
	//syn messsage must wait message feedback
	if (pmessage->attr & ARISC_MESSAGE_ATTR_SOFTSYN) {
		ARISC_ERR("standby arisc driver not support soft syn message transfer\n");
		return -EINVAL;
	}
	
	return 0;
}

int arisc_hwmsgbox_feedback_message(struct arisc_message *pmessage, unsigned int timeout)
{
	volatile unsigned long value;
	
	if (pmessage->attr & ARISC_MESSAGE_ATTR_HARDSYN) {
		//use ac327 hard syn receiver channel.
		while (readl(IO_ADDRESS(AW_MSGBOX_FIFO_STATUS_REG(ARISC_HWMSGBOX_ARISC_SYN_RX_CH))) == 1) {
			//message-queue fifo is full, waiting.
			;
		}
		value = ((volatile unsigned long)pmessage) - arisc_sram_a2_vbase;
		ARISC_INF("arisc feedback hard syn message : %x\n", (unsigned int)value);
		writel(value, IO_ADDRESS(AW_MSGBOX_MSG_REG(ARISC_HWMSGBOX_ARISC_SYN_RX_CH)));
		return 0;
	}
	//soft syn use asyn tx channel
	if (pmessage->attr & ARISC_MESSAGE_ATTR_SOFTSYN) {
		while (readl(IO_ADDRESS(AW_MSGBOX_FIFO_STATUS_REG(ARISC_HWMSGBOX_ARISC_ASYN_RX_CH))) == 1) {
			//fifo is full, wait
			;
		}
		//write message to message-queue fifo.
		value = ((volatile unsigned long)pmessage) - arisc_sram_a2_vbase;
		ARISC_INF("arisc send asyn or soft syn message : %x\n", (unsigned int)value);
		writel(value, IO_ADDRESS(AW_MSGBOX_MSG_REG(ARISC_HWMSGBOX_ARISC_ASYN_RX_CH)));
		return 0;
	}
	
	//invalid syn message
	return -EINVAL;
}

/*
*********************************************************************************************************
*                                       	ENABLE RECEIVER INT
*
* Description: 	enbale the receiver interrupt of message-queue.
*
* Arguments  : 	queue 	: the number of message-queue which we want to enable interrupt.
*				user	: the user which we want to enable interrupt.
*
* Returns    : 	0 if enable interrupt succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_hwmsgbox_enable_receiver_int(int queue, int user)
{
	volatile unsigned int value;
	
	value  =  readl(IO_ADDRESS(AW_MSGBOX_IRQ_EN_REG(user)));
	value &= ~(0x1 << (queue * 2));
	value |=  (0x1 << (queue * 2));
	writel(value, IO_ADDRESS(AW_MSGBOX_IRQ_EN_REG(user)));
	
	return 0;
}

/*
*********************************************************************************************************
*                                       	QUERY PENDING
*
* Description: 	query the receiver interrupt pending of message-queue.
*
* Arguments  : 	queue 	: the number of message-queue which we want to query.
*				user	: the user which we want to query.
*
* Returns    : 	0 if query pending succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_hwmsgbox_query_receiver_pending(int queue, int user)
{
	volatile unsigned long value;
	
	value  =  readl(IO_ADDRESS((AW_MSGBOX_IRQ_STATUS_REG(user))));
	
	return value & (0x1 << (queue * 2));
}

/*
*********************************************************************************************************
*                                       	CLEAR PENDING
*
* Description: 	clear the receiver interrupt pending of message-queue.
*
* Arguments  : 	queue 	: the number of message-queue which we want to clear.
*				user	: the user which we want to clear.
*
* Returns    : 	0 if clear pending succeeded, others if failed.
*********************************************************************************************************
*/
int arisc_hwmsgbox_clear_receiver_pending(int queue, int user)
{
	writel((0x1 << (queue * 2)), IO_ADDRESS(AW_MSGBOX_IRQ_STATUS_REG(user)));
	
	return 0;
}

/*
*********************************************************************************************************
*                                        QUERY MESSAGE
*
* Description: 	query message of hwmsgbox by hand, mainly for.
*
* Arguments  : 	none.
*
* Returns    : 	the point of message, NULL if timeout.
*********************************************************************************************************
*/
struct arisc_message *arisc_hwmsgbox_query_message(void)
{
	struct arisc_message *pmessage = (struct arisc_message *)(NULL);
	
	//query ac327 asyn received channel
	if (readl(IO_ADDRESS(AW_MSGBOX_MSG_STATUS_REG(ARISC_HWMSGBOX_ARISC_ASYN_TX_CH)))) {
		volatile unsigned long value;
		value = readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(ARISC_HWMSGBOX_ARISC_ASYN_TX_CH)));
		pmessage = (struct arisc_message *)(value + arisc_sram_a2_vbase);
		if (arisc_message_valid(pmessage)) {
			//message state switch
			if (pmessage->state == ARISC_MESSAGE_PROCESSED) {
				//ARISC_MESSAGE_PROCESSED->ARISC_MESSAGE_FEEDBACKED
				pmessage->state = ARISC_MESSAGE_FEEDBACKED;
			} else {
				//ARISC_MESSAGE_INITIALIZED->ARISC_MESSAGE_RECEIVED
				pmessage->state = ARISC_MESSAGE_RECEIVED;
			}
		} else {
			//print_call_info();
			ARISC_ERR("invalid message received: 1 pmessage = 0x%x. \n", (__u32)pmessage);
			return (struct arisc_message *)(NULL);
		}
		//clear pending
		arisc_hwmsgbox_clear_receiver_pending(ARISC_HWMSGBOX_ARISC_ASYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);
		return pmessage;
	}
	//query ac327 syn received channel
	if (readl(IO_ADDRESS(AW_MSGBOX_MSG_STATUS_REG(ARISC_HWMSGBOX_ARISC_SYN_TX_CH)))) {
		volatile unsigned long value;
		value = readl(IO_ADDRESS(AW_MSGBOX_MSG_REG(ARISC_HWMSGBOX_ARISC_SYN_TX_CH)));
		pmessage = (struct arisc_message *)(value + arisc_sram_a2_vbase);
		if (arisc_message_valid(pmessage)) {
			//message state switch
			if (pmessage->state == ARISC_MESSAGE_PROCESSED) {
				//ARISC_MESSAGE_PROCESSED->ARISC_MESSAGE_FEEDBACKED
				pmessage->state = ARISC_MESSAGE_FEEDBACKED;
			} else {
				//ARISC_MESSAGE_INITIALIZED->ARISC_MESSAGE_RECEIVED
				pmessage->state = ARISC_MESSAGE_RECEIVED;
			}
		} else {
			//print_call_info();
			ARISC_ERR("invalid message received: 2 pmessage = 0x%x. \n", (__u32)pmessage);
			return (struct arisc_message *)(NULL);
		}
		arisc_hwmsgbox_clear_receiver_pending(ARISC_HWMSGBOX_ARISC_SYN_TX_CH, AW_HWMSG_QUEUE_USER_AC327);
		return pmessage;
	}
	//no valid message
	return (struct arisc_message *)(NULL);
}

int arisc_message_valid(struct arisc_message *pmessage)
{
	if ((((__u32)pmessage) >= (ARISC_MESSAGE_POOL_START + arisc_sram_a2_vbase)) && 
		(((__u32)pmessage) <  (ARISC_MESSAGE_POOL_END   + arisc_sram_a2_vbase))) {
		//valid message
		return 1;
	}
	return 0;
}

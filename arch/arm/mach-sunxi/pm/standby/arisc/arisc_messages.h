/*
 *  arch/arm/mach-sun6i/arisc/include/arisc_messages.h
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


#ifndef	__ARISC_MESSAGES_H__
#define	__ARISC_MESSAGES_H__

//message attributes(only use 8bit)
#define	ARISC_MESSAGE_ATTR_SOFTSYN		(1<<0)	//need soft syn with another cpu
#define	ARISC_MESSAGE_ATTR_HARDSYN		(1<<1)	//need hard syn with another cpu

//message states
#define	ARISC_MESSAGE_FREED			(0x0)	//freed state
#define	ARISC_MESSAGE_ALLOCATED		(0x1)	//allocated state
#define ARISC_MESSAGE_INITIALIZED	(0x2)	//initialized state
#define	ARISC_MESSAGE_RECEIVED		(0x3)	//received state
#define	ARISC_MESSAGE_PROCESSING	(0x4)	//processing state
#define	ARISC_MESSAGE_PROCESSED		(0x5)	//processed state
#define	ARISC_MESSAGE_FEEDBACKED	(0x6)	//feedback state

typedef int (*arisc_cb_t)(void *arg);

/* call back struct */
typedef struct arisc_msg_cb
{
	arisc_cb_t   handler;
	void        *arg;
} arisc_msg_cb_t;

//the structure of message frame,
//this structure will transfer between arisc and ac327.
//sizeof(struct message) : 32Byte.
typedef struct arisc_message
{
	volatile unsigned char   		 state;		/* identify the used status of message frame */
	volatile unsigned char   		 attr;		/* message attribute : SYN OR ASYN           */
	volatile unsigned char   		 type;		/* message type : DVFS_REQ                   */
	volatile unsigned char   		 result;	/* message process result                    */
	volatile struct arisc_message	*next;		/* pointer of next message frame             */
	volatile struct arisc_msg_cb		 cb;		/* the callback function and arg of message  */
	volatile void    	   			*private;	/* message private data                      */
	volatile unsigned int   			 paras[11];	/* the parameters of message                 */
} arisc_message_t;

//the base of messages
#define	ARISC_MESSAGE_BASE	(0x10)

//standby commands
#define	ARISC_SSTANDBY_ENTER_REQ	 	(ARISC_MESSAGE_BASE + 0x00)  //request to enter(ac327 to arisc)
#define	ARISC_SSTANDBY_RESTORE_NOTIFY   (ARISC_MESSAGE_BASE + 0x01)  //restore finished(ac327 to arisc)
#define	ARISC_NSTANDBY_ENTER_REQ	 	(ARISC_MESSAGE_BASE + 0x02)  //request to enter(ac327 to arisc)
#define	ARISC_NSTANDBY_WAKEUP_NOTIFY    (ARISC_MESSAGE_BASE + 0x03)  //wakeup notify   (arisc to ac327)
#define	ARISC_NSTANDBY_RESTORE_REQ      (ARISC_MESSAGE_BASE + 0x04)  //request to restore    (ac327 to arisc)
#define	ARISC_NSTANDBY_RESTORE_COMPLETE (ARISC_MESSAGE_BASE + 0x05)  //arisc restore complete(arisc to ac327)
#define	ARISC_ESSTANDBY_ENTER_REQ        (ARISC_MESSAGE_BASE + 0x06)  /* request to enter       (ac327 to arisc) */
#define	ARISC_TSTANDBY_ENTER_REQ	 	 (ARISC_MESSAGE_BASE + 0x07)  /* request to enter(ac327 to arisc)        */
#define	ARISC_TSTANDBY_RESTORE_NOTIFY    (ARISC_MESSAGE_BASE + 0x08)  /* restore finished(ac327 to arisc)		 */
#define	ARISC_FAKE_POWER_OFF_REQ         (ARISC_MESSAGE_BASE + 0x09)  /* request to enter(ac327 to arisc)        */

//dvfs commands
#define	ARISC_CPUX_DVFS_REQ		 	(ARISC_MESSAGE_BASE + 0x20)  //request dvfs    (ac327 to arisc)

//pmu commands                                      
#define	ARISC_AXP_POWEROFF_REQ	 	(ARISC_MESSAGE_BASE + 0x40)  //request power-off(ac327 to arisc)
#define	ARISC_AXP_READ_REGS		 	(ARISC_MESSAGE_BASE + 0x41)  //read registers	(ac327 to arisc)
#define	ARISC_AXP_WRITE_REGS		(ARISC_MESSAGE_BASE + 0x42)  //write registers  (ac327 to arisc)
#define ARISC_AXP_SET_BATTERY		(ARISC_MESSAGE_BASE + 0x43)  //set battery 		(ac327 to arisc)
#define ARISC_AXP_GET_BATTERY		(ARISC_MESSAGE_BASE + 0x44)  //get battery 		(ac327 to arisc)
#define ARISC_AXP_INT_COMING_NOTIFY (ARISC_MESSAGE_BASE + 0x45)  //interrupt coming notify(arisc to ac327)

//arisc initialize state notify commands
#define ARISC_STARTUP_NOTIFY	 	(ARISC_MESSAGE_BASE + 0x80)  //arisc init state notify(arisc to ac327)

#endif	//__ARISC_MESSAGES_H__

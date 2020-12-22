/*
 *  include/linux/arisc/hwmsgbox.h
 *
 * Copyright 2012 (c) Allwinner.
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

#ifndef	__ASM_ARCH_HWMSGBOX_H
#define	__ASM_ARCH_HWMSGBOX_H

//the number of hardware message queue.
#define AW_HWMSG_QUEUE_NUMBER	(8)

//the user of hardware message queue.
typedef enum aw_hwmsg_queue_user
{
	AW_HWMSG_QUEUE_USER_ARISC,	//arisc
	AW_HWMSG_QUEUE_USER_AC327,	//cpu0
} aw_hwmsg_queue_user_e;

//hardware message-box register list
#define	AW_MSGBOX_CTRL_REG(m)			(SUNXI_MSGBOX_PBASE + 0x0000 + (0x4 * (m>>2)))
#define AW_MSGBOX_IRQ_EN_REG(u)			(SUNXI_MSGBOX_PBASE + 0x0040 + (0x20* u))
#define	AW_MSGBOX_IRQ_STATUS_REG(u)		(SUNXI_MSGBOX_PBASE + 0x0050 + (0x20* u))
#define AW_MSGBOX_FIFO_STATUS_REG(m)	(SUNXI_MSGBOX_PBASE + 0x0100 + (0x4 * m))
#define AW_MSGBOX_MSG_STATUS_REG(m)		(SUNXI_MSGBOX_PBASE + 0x0140 + (0x4 * m))
#define AW_MSGBOX_MSG_REG(m)			(SUNXI_MSGBOX_PBASE + 0x0180 + (0x4 * m))
#define AW_MSGBOX_DEBUG_REG				(SUNXI_MSGBOX_PBASE + 0x01c0)

#endif	//__ASM_ARCH_HWMSGBOX_H

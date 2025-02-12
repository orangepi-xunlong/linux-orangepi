/*****************************************************************************
 *
 *  Copyright (C) 2006-2008  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT Master.
 *
 *  The IgH EtherCAT Master is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License version 2, as
 *  published by the Free Software Foundation.
 *
 *  The IgH EtherCAT Master is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 *  Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the IgH EtherCAT Master; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 ****************************************************************************/

/** \file
 * FoE defines.
 */

#ifndef __FOE_H__
#define __FOE_H__

/****************************************************************************/

/** FoE error enumeration type.
 */
typedef enum {
    FOE_BUSY               = 0, /**< Busy. */
    FOE_READY              = 1, /**< Ready. */
    FOE_IDLE               = 2, /**< Idle. */
    FOE_WC_ERROR           = 3, /**< Working counter error. */
    FOE_RECEIVE_ERROR      = 4, /**< Receive error. */
    FOE_PROT_ERROR         = 5, /**< Protocol error. */
    FOE_NODATA_ERROR       = 6, /**< No data error. */
    FOE_PACKETNO_ERROR     = 7, /**< Packet number error. */
    FOE_OPCODE_ERROR       = 8, /**< OpCode error. */
    FOE_TIMEOUT_ERROR      = 9, /**< Timeout error. */
    FOE_SEND_RX_DATA_ERROR = 10, /**< Error sending received data. */
    FOE_RX_DATA_ACK_ERROR  = 11, /**< Error acknowledging received data. */
    FOE_ACK_ERROR          = 12, /**< Acknowledge error. */
    FOE_MBOX_FETCH_ERROR   = 13, /**< Error fetching data from mailbox. */
    FOE_READ_NODATA_ERROR  = 14, /**< No data while reading. */
    FOE_MBOX_PROT_ERROR    = 15, /**< Mailbox protocol error. */
} ec_foe_error_t;

/****************************************************************************/

#endif

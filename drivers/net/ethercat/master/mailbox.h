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

/**
   \file
   Mailbox functionality.
*/

/****************************************************************************/

#ifndef __EC_MAILBOX_H__
#define __EC_MAILBOX_H__

#include "slave.h"

/****************************************************************************/

/** Size of the mailbox header.
 */
#define EC_MBOX_HEADER_SIZE 6

/** Mailbox types.
 *
 * These are used in the 'Type' field of the mailbox header.
 */
enum {
    EC_MBOX_TYPE_EOE = 0x02,
    EC_MBOX_TYPE_COE = 0x03,
    EC_MBOX_TYPE_FOE = 0x04,
    EC_MBOX_TYPE_SOE = 0x05,
    EC_MBOX_TYPE_VOE = 0x0f,
};

/****************************************************************************/

uint8_t *ec_slave_mbox_prepare_send(const ec_slave_t *, ec_datagram_t *,
                                    uint8_t, size_t);
int      ec_slave_mbox_prepare_check(const ec_slave_t *, ec_datagram_t *);
int      ec_slave_mbox_check(const ec_datagram_t *);
int      ec_slave_mbox_prepare_fetch(const ec_slave_t *, ec_datagram_t *);
uint8_t *ec_slave_mbox_fetch(const ec_slave_t *, const ec_datagram_t *,
                             uint8_t *, size_t *);

/****************************************************************************/

#endif

/*****************************************************************************
 *
 *  Copyright (C) 2006-2012  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT datagram pair structure.
*/

/****************************************************************************/

#ifndef __EC_DATAGRAM_PAIR_H__
#define __EC_DATAGRAM_PAIR_H__

#include <linux/list.h>

#include "globals.h"
#include "datagram.h"

/****************************************************************************/

/** Domain datagram pair.
 */
typedef struct {
    struct list_head list; /**< List header. */
    ec_domain_t *domain; /**< Parent domain. */
    ec_datagram_t datagrams[EC_MAX_NUM_DEVICES]; /**< Datagrams.  */
#if EC_MAX_NUM_DEVICES > 1
    uint8_t *send_buffer;
#endif
    unsigned int expected_working_counter; /**< Expectord working conter. */
} ec_datagram_pair_t;

/****************************************************************************/

int ec_datagram_pair_init(ec_datagram_pair_t *, ec_domain_t *, uint32_t,
        uint8_t *, size_t, const unsigned int []);
void ec_datagram_pair_clear(ec_datagram_pair_t *);

uint16_t ec_datagram_pair_process(ec_datagram_pair_t *, uint16_t[]);

/****************************************************************************/

#endif

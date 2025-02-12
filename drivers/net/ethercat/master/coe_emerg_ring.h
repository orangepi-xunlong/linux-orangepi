/*****************************************************************************
 *
 *  Copyright (C) 2012  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT CoE emergency ring buffer structure.
*/

/****************************************************************************/

#ifndef __EC_COE_EMERG_RING_H__
#define __EC_COE_EMERG_RING_H__

#include "globals.h"

/****************************************************************************/

/** EtherCAT CoE emergency message record.
 */
typedef struct {
    u8 data[EC_COE_EMERGENCY_MSG_SIZE]; /**< Message data. */
} ec_coe_emerg_msg_t;

/****************************************************************************/

/** EtherCAT CoE emergency ring buffer.
 */
typedef struct {
    ec_slave_config_t *sc; /**< Slave configuration  owning the ring. */

    ec_coe_emerg_msg_t *msgs; /**< Message ring. */
    size_t size; /**< Ring size. */

    unsigned int read_index; /**< Read index. */
    unsigned int write_index; /**< Write index. */
    unsigned int overruns; /**< Number of overruns since last reset. */
} ec_coe_emerg_ring_t;

/****************************************************************************/

void ec_coe_emerg_ring_init(ec_coe_emerg_ring_t *, ec_slave_config_t *);
void ec_coe_emerg_ring_clear(ec_coe_emerg_ring_t *);

int ec_coe_emerg_ring_size(ec_coe_emerg_ring_t *, size_t);
void ec_coe_emerg_ring_push(ec_coe_emerg_ring_t *, const u8 *);
int ec_coe_emerg_ring_pop(ec_coe_emerg_ring_t *, u8 *);
int ec_coe_emerg_ring_clear_ring(ec_coe_emerg_ring_t *);
int ec_coe_emerg_ring_overruns(const ec_coe_emerg_ring_t *);

/****************************************************************************/

#endif

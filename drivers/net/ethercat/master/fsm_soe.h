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
   EtherCAT CoE state machines.
*/

/****************************************************************************/

#ifndef __EC_FSM_SOE_H__
#define __EC_FSM_SOE_H__

#include "globals.h"
#include "datagram.h"
#include "slave.h"
#include "soe_request.h"

/****************************************************************************/

typedef struct ec_fsm_soe ec_fsm_soe_t; /**< \see ec_fsm_soe */

/** Finite state machines for the Sercos over EtherCAT protocol.
 */
struct ec_fsm_soe {
    ec_slave_t *slave; /**< slave the FSM runs on */
    unsigned int retries; /**< retries upon datagram timeout */

    void (*state)(ec_fsm_soe_t *, ec_datagram_t *); /**< CoE state function */
    ec_datagram_t *datagram; /**< Datagram used in the previous step. */
    unsigned long jiffies_start; /**< Timestamp. */
    ec_soe_request_t *request; /**< SoE request */
    off_t offset; /**< IDN data offset during fragmented write. */
    size_t fragment_size; /**< Size of the current fragment. */
};

/****************************************************************************/

void ec_fsm_soe_init(ec_fsm_soe_t *);
void ec_fsm_soe_clear(ec_fsm_soe_t *);

void ec_fsm_soe_transfer(ec_fsm_soe_t *, ec_slave_t *, ec_soe_request_t *);

int ec_fsm_soe_exec(ec_fsm_soe_t *, ec_datagram_t *);
int ec_fsm_soe_success(const ec_fsm_soe_t *);

/****************************************************************************/

#endif

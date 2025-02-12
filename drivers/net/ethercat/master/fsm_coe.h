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

#ifndef __EC_FSM_COE_H__
#define __EC_FSM_COE_H__

#include "globals.h"
#include "datagram.h"
#include "slave.h"
#include "sdo.h"
#include "sdo_request.h"

/****************************************************************************/

typedef struct ec_fsm_coe ec_fsm_coe_t; /**< \see ec_fsm_coe */

/** Finite state machines for the CANopen over EtherCAT protocol.
 */
struct ec_fsm_coe {
    ec_slave_t *slave; /**< slave the FSM runs on */
    unsigned int retries; /**< retries upon datagram timeout */

    void (*state)(ec_fsm_coe_t *, ec_datagram_t *); /**< CoE state function */
    ec_datagram_t *datagram; /**< Datagram used in last step. */
    unsigned long jiffies_start; /**< CoE timestamp. */
    ec_sdo_t *sdo; /**< current SDO */
    uint8_t subindex; /**< current subindex */
    ec_sdo_request_t *request; /**< SDO request */
    uint32_t complete_size; /**< Used when segmenting. */
    uint8_t toggle; /**< toggle bit for segment commands */
    uint32_t offset; /**< Data offset during segmented download. */
    uint32_t remaining; /**< Remaining bytes during segmented download. */
    size_t segment_size; /**< Current segment size. */
};

/****************************************************************************/

void ec_fsm_coe_init(ec_fsm_coe_t *);
void ec_fsm_coe_clear(ec_fsm_coe_t *);

void ec_fsm_coe_dictionary(ec_fsm_coe_t *, ec_slave_t *);
void ec_fsm_coe_transfer(ec_fsm_coe_t *, ec_slave_t *, ec_sdo_request_t *);

int ec_fsm_coe_exec(ec_fsm_coe_t *, ec_datagram_t *);
int ec_fsm_coe_success(const ec_fsm_coe_t *);

/****************************************************************************/

#endif

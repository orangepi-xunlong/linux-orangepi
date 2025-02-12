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
 * EtherCAT PDO entry configuration state machine structures.
 */

/****************************************************************************/

#ifndef __EC_FSM_PDO_ENTRY_H__
#define __EC_FSM_PDO_ENTRY_H__

#include "globals.h"
#include "datagram.h"
#include "fsm_coe.h"

/****************************************************************************/

/**
 * \see ec_fsm_pdo_entry
 */
typedef struct ec_fsm_pdo_entry ec_fsm_pdo_entry_t;

/** PDO configuration state machine.
 */
struct ec_fsm_pdo_entry
{
    void (*state)(ec_fsm_pdo_entry_t *, ec_datagram_t *); /**< state function
                                                           */
    ec_fsm_coe_t *fsm_coe; /**< CoE state machine to use */
    ec_sdo_request_t request; /**< SDO request. */

    ec_slave_t *slave; /**< Slave the FSM runs on. */
    ec_pdo_t *target_pdo; /**< PDO to read the mapping for. */
    const ec_pdo_t *source_pdo; /**< PDO with desired mapping. */
    const ec_pdo_t *cur_pdo; /**< PDO with current mapping (display only). */
    const ec_pdo_entry_t *entry; /**< Current entry. */
    unsigned int entry_count; /**< Number of entries. */
    unsigned int entry_pos; /**< Position in PDO mapping. */
};

/****************************************************************************/

void ec_fsm_pdo_entry_init(ec_fsm_pdo_entry_t *, ec_fsm_coe_t *);
void ec_fsm_pdo_entry_clear(ec_fsm_pdo_entry_t *);

void ec_fsm_pdo_entry_start_reading(ec_fsm_pdo_entry_t *, ec_slave_t *,
        ec_pdo_t *);
void ec_fsm_pdo_entry_start_configuration(ec_fsm_pdo_entry_t *, ec_slave_t *,
        const ec_pdo_t *, const ec_pdo_t *);

int ec_fsm_pdo_entry_exec(ec_fsm_pdo_entry_t *, ec_datagram_t *);
int ec_fsm_pdo_entry_success(const ec_fsm_pdo_entry_t *);

/****************************************************************************/

#endif

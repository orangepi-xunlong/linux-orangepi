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
   EtherCAT PDO configuration state machine structures.
*/

/****************************************************************************/

#ifndef __EC_FSM_PDO_H__
#define __EC_FSM_PDO_H__

#include "globals.h"
#include "datagram.h"
#include "fsm_coe.h"
#include "fsm_pdo_entry.h"

/****************************************************************************/

/**
 * \see ec_fsm_pdo
 */
typedef struct ec_fsm_pdo ec_fsm_pdo_t;

/** PDO configuration state machine.
 */
struct ec_fsm_pdo
{
    void (*state)(ec_fsm_pdo_t *, ec_datagram_t *); /**< State function. */
    ec_fsm_coe_t *fsm_coe; /**< CoE state machine to use. */
    ec_fsm_pdo_entry_t fsm_pdo_entry; /**< PDO entry state machine. */
    ec_pdo_list_t pdos; /**< PDO configuration. */
    ec_sdo_request_t request; /**< SDO request. */
    ec_pdo_t slave_pdo; /**< PDO actually appearing in a slave. */

    ec_slave_t *slave; /**< Slave the FSM runs on. */
    uint8_t sync_index; /**< Current sync manager index. */
    ec_sync_t *sync; /**< Current sync manager. */
    ec_pdo_t *pdo; /**< Current PDO. */
    unsigned int pdo_pos; /**< Assignment position of current PDOs. */
    unsigned int pdo_count; /**< Number of assigned PDOs. */
};

/****************************************************************************/

void ec_fsm_pdo_init(ec_fsm_pdo_t *, ec_fsm_coe_t *);
void ec_fsm_pdo_clear(ec_fsm_pdo_t *);

void ec_fsm_pdo_start_reading(ec_fsm_pdo_t *, ec_slave_t *);
void ec_fsm_pdo_start_configuration(ec_fsm_pdo_t *, ec_slave_t *);

int ec_fsm_pdo_exec(ec_fsm_pdo_t *, ec_datagram_t *);
int ec_fsm_pdo_success(const ec_fsm_pdo_t *);

/****************************************************************************/

#endif

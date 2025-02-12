/*****************************************************************************
 *
 *  Copyright (C) 2006-2024  Florian Pose, Ingenieurgemeinschaft IgH
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
 */

/**
   \file
   EtherCAT slave configuration state machine.
*/

/****************************************************************************/

#ifndef __EC_FSM_SLAVE_CONFIG_H__
#define __EC_FSM_SLAVE_CONFIG_H__

#include "globals.h"
#include "slave.h"
#include "datagram.h"
#include "fsm_change.h"
#include "fsm_coe.h"
#include "fsm_pdo.h"
#include "fsm_eoe.h"

/****************************************************************************/

/** \see ec_fsm_slave_config */
typedef struct ec_fsm_slave_config ec_fsm_slave_config_t;

/** Finite state machine to configure an EtherCAT slave.
 */
struct ec_fsm_slave_config
{
    ec_datagram_t *datagram; /**< Datagram used in the state machine. */
    ec_fsm_change_t *fsm_change; /**< State change state machine. */
    ec_fsm_coe_t *fsm_coe; /**< CoE state machine. */
    ec_fsm_soe_t *fsm_soe; /**< SoE state machine. */
    ec_fsm_pdo_t *fsm_pdo; /**< PDO configuration state machine. */
    ec_fsm_eoe_t *fsm_eoe; /**< EoE state machine. */

    ec_slave_t *slave; /**< Slave the FSM runs on. */
    void (*state)(ec_fsm_slave_config_t *); /**< State function. */
    unsigned int retries; /**< Retries on datagram timeout. */
    ec_sdo_request_t *request; /**< SDO request for SDO configuration. */
    ec_sdo_request_t request_copy; /**< Copied SDO request. */
    ec_soe_request_t *soe_request; /**< SDO request for SDO configuration. */
    ec_soe_request_t soe_request_copy; /**< Copied SDO request. */
    unsigned long jiffies_start; /**< For timeout calculations. */
    unsigned int take_time; /**< Store jiffies after datagram reception. */
    unsigned long wait_ms; /**< Wait time (used to wait before SAFEOP). */
};

/****************************************************************************/

void ec_fsm_slave_config_init(ec_fsm_slave_config_t *, ec_datagram_t *,
        ec_fsm_change_t *, ec_fsm_coe_t *, ec_fsm_soe_t *, ec_fsm_pdo_t *,
        ec_fsm_eoe_t *);
void ec_fsm_slave_config_clear(ec_fsm_slave_config_t *);

void ec_fsm_slave_config_start(ec_fsm_slave_config_t *, ec_slave_t *);

int ec_fsm_slave_config_exec(ec_fsm_slave_config_t *);
int ec_fsm_slave_config_success(const ec_fsm_slave_config_t *);

/****************************************************************************/

#endif

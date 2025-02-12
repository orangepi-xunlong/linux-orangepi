/*****************************************************************************
 *
 *  Copyright (C) 2006-2023  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT master state machine.
*/

/****************************************************************************/

#ifndef __EC_FSM_MASTER_H__
#define __EC_FSM_MASTER_H__

#include "globals.h"
#include "datagram.h"
#include "foe_request.h"
#include "sdo_request.h"
#include "soe_request.h"
#include "fsm_slave_config.h"
#include "fsm_slave_scan.h"
#include "fsm_pdo.h"

/****************************************************************************/

/** SII write request.
 */
typedef struct {
    struct list_head list; /**< List head. */
    ec_slave_t *slave; /**< EtherCAT slave. */
    uint16_t offset; /**< SII word offset. */
    size_t nwords; /**< Number of words. */
    const uint16_t *words; /**< Pointer to the data words. */
    ec_internal_request_state_t state; /**< State of the request. */
} ec_sii_write_request_t;

/****************************************************************************/

typedef struct ec_fsm_master ec_fsm_master_t; /**< \see ec_fsm_master */

/** Finite state machine of an EtherCAT master.
 */
struct ec_fsm_master {
    ec_master_t *master; /**< master the FSM runs on */
    ec_datagram_t *datagram; /**< datagram used in the state machine */
    unsigned int retries; /**< retries on datagram timeout. */

    void (*state)(ec_fsm_master_t *); /**< master state function */
    ec_device_index_t dev_idx; /**< Current device index (for scanning etc.).
                                */
    int idle; /**< state machine is in idle phase */
    unsigned long scan_jiffies; /**< beginning of slave scanning */
    uint8_t link_state[EC_MAX_NUM_DEVICES]; /**< Last link state for every
                                              device. */
    unsigned int slaves_responding[EC_MAX_NUM_DEVICES]; /**< Number of
                                                          responding slaves
                                                          for every device. */
    unsigned int rescan_required; /**< A bus rescan is required. */
    ec_slave_state_t slave_states[EC_MAX_NUM_DEVICES]; /**< AL states of
                                                         responding slaves for
                                                         every device. */
    ec_slave_t *slave; /**< current slave */
    ec_sii_write_request_t *sii_request; /**< SII write request */
    off_t sii_index; /**< index to SII write request data */
    ec_sdo_request_t *sdo_request; /**< SDO request to process. */
    ec_soe_request_t *soe_request; /**< SoE request to process. */

    ec_fsm_coe_t fsm_coe; /**< CoE state machine */
    ec_fsm_soe_t fsm_soe; /**< SoE state machine */
    ec_fsm_pdo_t fsm_pdo; /**< PDO configuration state machine. */
    ec_fsm_eoe_t fsm_eoe; /**< EoE state machine */
    ec_fsm_change_t fsm_change; /**< State change state machine */
    ec_fsm_slave_config_t fsm_slave_config; /**< slave state machine */
    ec_fsm_slave_scan_t fsm_slave_scan; /**< slave state machine */
    ec_fsm_sii_t fsm_sii; /**< SII state machine */
};

/****************************************************************************/

void ec_fsm_master_init(ec_fsm_master_t *, ec_master_t *, ec_datagram_t *);
void ec_fsm_master_clear(ec_fsm_master_t *);

void ec_fsm_master_reset(ec_fsm_master_t *);

int ec_fsm_master_exec(ec_fsm_master_t *);
int ec_fsm_master_idle(const ec_fsm_master_t *);

/****************************************************************************/

#endif

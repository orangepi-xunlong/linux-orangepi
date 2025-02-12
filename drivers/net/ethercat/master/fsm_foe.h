/*****************************************************************************
 *
 *  Copyright (C) 2008       Olav Zarges, imc Messsysteme GmbH
 *                2009-2012  Florian Pose <fp@igh.de>
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
   EtherCAT FoE state machines.
*/

/****************************************************************************/

#ifndef __EC_FSM_FOE_H__
#define __EC_FSM_FOE_H__

#include "globals.h"
#include "../include/ecrt.h"
#include "datagram.h"
#include "slave.h"
#include "foe_request.h"

/****************************************************************************/

typedef struct ec_fsm_foe ec_fsm_foe_t; /**< \see ec_fsm_foe */

/** Finite state machines for the CANopen-over-EtherCAT protocol.
 */
struct ec_fsm_foe {
    ec_slave_t *slave; /**< Slave the FSM runs on. */
    unsigned int retries; /**< Retries upon datagram timeout */

    void (*state)(ec_fsm_foe_t *, ec_datagram_t *); /**< FoE state function.
                                                     */
    ec_datagram_t *datagram; /**< Datagram used in previous step. */
    unsigned long jiffies_start; /**< FoE timestamp. */
    uint8_t subindex; /**< Current subindex. */
    ec_foe_request_t *request; /**< FoE request. */
    uint8_t toggle; /**< Toggle bit for segment commands. */

    uint8_t *tx_buffer; /**< Buffer with data to transmit. */
    uint32_t tx_buffer_size; /**< Size of data to transmit. */
    uint32_t tx_buffer_offset; /**< Offset of data to tranmit next. */
    uint32_t tx_last_packet; /**< Current packet is last one to send. */
    uint32_t tx_packet_no; /**< FoE packet number. */
    uint32_t tx_current_size; /**< Size of current packet to send. */
    uint8_t *tx_filename; /**< Name of file to transmit. */
    uint32_t tx_filename_len; /**< Lenth of transmit file name. */

    uint8_t *rx_buffer; /**< Buffer for received data. */
    uint32_t rx_buffer_size; /**< Size of receive buffer. */
    uint32_t rx_buffer_offset; /**< Offset in receive buffer. */
    uint32_t rx_expected_packet_no; /**< Expected receive packet number. */
    uint32_t rx_last_packet; /**< Current packet is the last to receive. */
    uint8_t *rx_filename; /**< Name of the file to receive. */
    uint32_t rx_filename_len; /**< Length of the receive file name. */
};

/****************************************************************************/

void ec_fsm_foe_init(ec_fsm_foe_t *);
void ec_fsm_foe_clear(ec_fsm_foe_t *);

int ec_fsm_foe_exec(ec_fsm_foe_t *, ec_datagram_t *);
int ec_fsm_foe_success(const ec_fsm_foe_t *);

void ec_fsm_foe_transfer(ec_fsm_foe_t *, ec_slave_t *, ec_foe_request_t *);

/****************************************************************************/

#endif

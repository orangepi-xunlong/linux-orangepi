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
   EtherCAT slave information interface FSM structure.
*/

/****************************************************************************/

#ifndef __EC_FSM_SII_H__
#define __EC_FSM_SII_H__

#include "globals.h"
#include "datagram.h"
#include "slave.h"

/****************************************************************************/

/** SII access addressing mode.
 */
typedef enum {
    EC_FSM_SII_USE_INCREMENT_ADDRESS, /**< Use auto-increment addressing. */
    EC_FSM_SII_USE_CONFIGURED_ADDRESS /**< Use configured addresses. */
} ec_fsm_sii_addressing_t;

/****************************************************************************/

typedef struct ec_fsm_sii ec_fsm_sii_t; /**< \see ec_fsm_sii */

/**
   Slave information interface FSM.
*/

struct ec_fsm_sii
{
    ec_slave_t *slave; /**< slave the FSM runs on */
    ec_datagram_t *datagram; /**< datagram used in the state machine */
    unsigned int retries; /**< retries upon datagram timeout */

    void (*state)(ec_fsm_sii_t *); /**< SII state function */
    uint16_t word_offset; /**< input: word offset in SII */
    ec_fsm_sii_addressing_t mode; /**< reading via APRD or NPRD */
    uint8_t value[4]; /**< raw SII value (32bit) */
    unsigned long jiffies_start; /**< Start timestamp. */
    uint8_t check_once_more; /**< one more try after timeout */
};

/****************************************************************************/

void ec_fsm_sii_init(ec_fsm_sii_t *, ec_datagram_t *);
void ec_fsm_sii_clear(ec_fsm_sii_t *);

void ec_fsm_sii_read(ec_fsm_sii_t *, ec_slave_t *,
                     uint16_t, ec_fsm_sii_addressing_t);
void ec_fsm_sii_write(ec_fsm_sii_t *, ec_slave_t *, uint16_t,
        const uint16_t *, ec_fsm_sii_addressing_t);

int ec_fsm_sii_exec(ec_fsm_sii_t *);
int ec_fsm_sii_success(ec_fsm_sii_t *);

/****************************************************************************/

#endif

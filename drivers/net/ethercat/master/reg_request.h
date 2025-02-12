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
   EtherCAT register request structure.
*/

/****************************************************************************/

#ifndef __EC_REG_REQUEST_H__
#define __EC_REG_REQUEST_H__

#include <linux/list.h>

#include "globals.h"

/****************************************************************************/

/** Register request.
 */
struct ec_reg_request {
    struct list_head list; /**< List item. */
    size_t mem_size; /**< Size of data memory. */
    uint8_t *data; /**< Pointer to data memory. */
    ec_direction_t dir; /**< Direction. EC_DIR_OUTPUT means writing to the
                          slave, EC_DIR_INPUT means reading from the slave. */
    uint16_t address; /**< Register address. */
    size_t transfer_size; /**< Size of the data to transfer. */
    ec_internal_request_state_t state; /**< Request state. */
    uint16_t ring_position; /**< Ring position for emergency requests. */
};

/****************************************************************************/

int ec_reg_request_init(ec_reg_request_t *, size_t);
void ec_reg_request_clear(ec_reg_request_t *);

/****************************************************************************/

#endif

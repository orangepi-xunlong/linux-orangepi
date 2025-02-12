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
   EtherCAT CANopen SDO request structure.
*/

/****************************************************************************/

#ifndef __EC_SDO_REQUEST_H__
#define __EC_SDO_REQUEST_H__

#include <linux/list.h>

#include "globals.h"

/****************************************************************************/

/** CANopen SDO request.
 */
struct ec_sdo_request {
    struct list_head list; /**< List item. */
    uint16_t index; /**< SDO index. */
    uint8_t subindex; /**< SDO subindex. */
    uint8_t *data; /**< Pointer to SDO data. */
    size_t mem_size; /**< Size of SDO data memory. */
    size_t data_size; /**< Size of SDO data. */
    uint8_t complete_access; /**< SDO shall be transferred completely. */
    uint32_t issue_timeout; /**< Maximum time in ms, the processing of the
                              request may take. */
    uint32_t response_timeout; /**< Maximum time in ms, the transfer is
                                 retried, if the slave does not respond. */
    ec_direction_t dir; /**< Direction. EC_DIR_OUTPUT means downloading to
                          the slave, EC_DIR_INPUT means uploading from the
                          slave. */
    ec_internal_request_state_t state; /**< SDO request state. */
    unsigned long jiffies_start; /**< Jiffies, when the request was issued. */
    unsigned long jiffies_sent; /**< Jiffies, when the upload/download
                                     request was sent. */
    int errno; /**< Error number. */
    uint32_t abort_code; /**< SDO request abort code. Zero on success. */
};

/****************************************************************************/

void ec_sdo_request_init(ec_sdo_request_t *);
void ec_sdo_request_clear(ec_sdo_request_t *);

int ec_sdo_request_copy(ec_sdo_request_t *, const ec_sdo_request_t *);
int ec_sdo_request_alloc(ec_sdo_request_t *, size_t);
int ec_sdo_request_copy_data(ec_sdo_request_t *, const uint8_t *, size_t);
int ec_sdo_request_timed_out(const ec_sdo_request_t *);

/****************************************************************************/

#endif

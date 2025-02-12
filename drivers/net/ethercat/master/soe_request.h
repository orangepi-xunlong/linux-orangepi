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
   EtherCAT SoE request structure.
*/

/****************************************************************************/

#ifndef __EC_SOE_REQUEST_H__
#define __EC_SOE_REQUEST_H__

#include <linux/list.h>

#include "globals.h"

/****************************************************************************/

/** Sercos-over-EtherCAT request.
 */
struct ec_soe_request {
    struct list_head list; /**< List item. */
    uint8_t drive_no; /**< Drive number. */
    uint16_t idn; /**< Sercos ID-Number. */
    ec_al_state_t al_state; /**< AL state (only valid for IDN config). */
    uint8_t *data; /**< Pointer to SDO data. */
    size_t mem_size; /**< Size of SDO data memory. */
    size_t data_size; /**< Size of SDO data. */
    uint32_t issue_timeout; /**< Maximum time in ms, the processing of the
                              request may take. */
    ec_direction_t dir; /**< Direction. EC_DIR_OUTPUT means writing to the
                          slave, EC_DIR_INPUT means reading from the slave. */
    ec_internal_request_state_t state; /**< Request state. */
    unsigned long jiffies_start; /**< Jiffies, when the request was issued. */
    unsigned long jiffies_sent; /**< Jiffies, when the upload/download
                                     request was sent. */
    uint16_t error_code; /**< SoE error code. */
};

/****************************************************************************/

void ec_soe_request_init(ec_soe_request_t *);
void ec_soe_request_clear(ec_soe_request_t *);

int ec_soe_request_copy(ec_soe_request_t *, const ec_soe_request_t *);
void ec_soe_request_set_drive_no(ec_soe_request_t *, uint8_t);
void ec_soe_request_set_idn(ec_soe_request_t *, uint16_t);
int ec_soe_request_alloc(ec_soe_request_t *, size_t);
int ec_soe_request_copy_data(ec_soe_request_t *, const uint8_t *, size_t);
int ec_soe_request_append_data(ec_soe_request_t *, const uint8_t *, size_t);
int ec_soe_request_read(ec_soe_request_t *);
int ec_soe_request_write(ec_soe_request_t *);
int ec_soe_request_timed_out(const ec_soe_request_t *);

/****************************************************************************/

#endif

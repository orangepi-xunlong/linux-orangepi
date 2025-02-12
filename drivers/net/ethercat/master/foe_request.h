/*****************************************************************************
 *
 *  Copyright (C) 2008  Olav Zarges, imc Messsysteme GmbH
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
   EtherCAT FoE request structure.
*/

/****************************************************************************/

#ifndef __EC_FOE_REQUEST_H__
#define __EC_FOE_REQUEST_H__

#include <linux/list.h>

#include "../include/ecrt.h"

#include "globals.h"

/****************************************************************************/

/** FoE request.
 */
typedef struct {
    struct list_head list; /**< List item. */
    uint8_t *buffer; /**< Pointer to FoE data. */
    size_t buffer_size; /**< Size of FoE data memory. */
    size_t data_size; /**< Size of FoE data. */

    uint32_t issue_timeout; /**< Maximum time in ms, the processing of the
                              request may take. */
    uint32_t response_timeout; /**< Maximum time in ms, the transfer is
                                 retried, if the slave does not respond. */
    ec_direction_t dir; /**< Direction. EC_DIR_OUTPUT means downloading to
                          the slave, EC_DIR_INPUT means uploading from the
                          slave. */
    ec_internal_request_state_t state; /**< FoE request state. */
    unsigned long jiffies_start; /**< Jiffies, when the request was issued. */
    unsigned long jiffies_sent; /**< Jiffies, when the upload/download
                                     request was sent. */
    uint8_t *file_name; /**< Pointer to the filename. */
    uint32_t result; /**< FoE request abort code. Zero on success. */
    uint32_t error_code; /**< Error code from an FoE Error Request. */
} ec_foe_request_t;

/****************************************************************************/

void ec_foe_request_init(ec_foe_request_t *, uint8_t *file_name);
void ec_foe_request_clear(ec_foe_request_t *);

int ec_foe_request_alloc(ec_foe_request_t *, size_t);
int ec_foe_request_copy_data(ec_foe_request_t *, const uint8_t *, size_t);
int ec_foe_request_timed_out(const ec_foe_request_t *);

void ec_foe_request_write(ec_foe_request_t *);
void ec_foe_request_read(ec_foe_request_t *);

/****************************************************************************/

#endif

/*****************************************************************************
 *
 *  Copyright (C) 2008  Olav Zarges, imc Messsysteme GmbH
 *  Copyright (C) 2020  Florian Pose, IgH
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
 * File-over-EtherCAT request functions.
 */

/****************************************************************************/

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#include "foe_request.h"
#include "foe.h"

/****************************************************************************/

/** Default timeout in ms to wait for FoE transfer responses.
 */
#define EC_FOE_REQUEST_RESPONSE_TIMEOUT 3000

/****************************************************************************/

// prototypes for private methods
void ec_foe_request_clear_data(ec_foe_request_t *);

/****************************************************************************/

/** FoE request constructor.
 */
void ec_foe_request_init(
        ec_foe_request_t *req, /**< FoE request. */
        uint8_t* file_name /** filename */)
{
    INIT_LIST_HEAD(&req->list);
    req->buffer = NULL;
    req->file_name = file_name;
    req->buffer_size = 0;
    req->data_size = 0;
    req->dir = EC_DIR_INVALID;
    req->issue_timeout = 0; // no timeout
    req->response_timeout = EC_FOE_REQUEST_RESPONSE_TIMEOUT;
    req->state = EC_INT_REQUEST_INIT;
    req->result = FOE_BUSY;
    req->error_code = 0x00000000;
}

/****************************************************************************/

/** FoE request destructor.
 */
void ec_foe_request_clear(
        ec_foe_request_t *req /**< FoE request. */
        )
{
    ec_foe_request_clear_data(req);
}

/****************************************************************************/

/** FoE request destructor.
 */
void ec_foe_request_clear_data(
        ec_foe_request_t *req /**< FoE request. */
        )
{
    if (req->buffer) {
        vfree(req->buffer);
        req->buffer = NULL;
    }

    req->buffer_size = 0;
    req->data_size = 0;
}

/****************************************************************************/

/** Pre-allocates the data memory.
 *
 * If the internal \a buffer_size is already bigger than \a size, nothing is
 * done.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_foe_request_alloc(
        ec_foe_request_t *req, /**< FoE request. */
        size_t size /**< Data size to allocate. */
        )
{
    if (size <= req->buffer_size) {
        return 0;
    }

    ec_foe_request_clear_data(req);

    if (!(req->buffer = (uint8_t *) vmalloc(size))) {
        EC_ERR("Failed to allocate %zu bytes of FoE memory.\n", size);
        return -ENOMEM;
    }

    req->buffer_size = size;
    req->data_size = 0;
    return 0;
}

/****************************************************************************/

/** Copies FoE data from an external source.
 *
 * If the \a buffer_size is to small, new memory is allocated.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_foe_request_copy_data(
        ec_foe_request_t *req, /**< FoE request. */
        const uint8_t *source, /**< Source data. */
        size_t size /**< Number of bytes in \a source. */
        )
{
    int ret;

    ret = ec_foe_request_alloc(req, size);
    if (ret) {
        return ret;
    }

    memcpy(req->buffer, source, size);
    req->data_size = size;
    return 0;
}

/****************************************************************************/

/** Checks, if the timeout was exceeded.
 *
 * \return non-zero if the timeout was exceeded, else zero.
 */
int ec_foe_request_timed_out(
        const ec_foe_request_t *req /**< FoE request. */
        )
{
    return req->issue_timeout
        && jiffies - req->jiffies_start > HZ * req->issue_timeout / 1000;
}

/****************************************************************************/

/** Prepares a read request (slave to master).
 */
void ec_foe_request_read(
        ec_foe_request_t *req /**< FoE request. */
        )
{
    req->dir = EC_DIR_INPUT;
    req->state = EC_INT_REQUEST_QUEUED;
    req->result = FOE_BUSY;
    req->jiffies_start = jiffies;
}

/****************************************************************************/

/** Prepares a write request (master to slave).
 */
void ec_foe_request_write(
        ec_foe_request_t *req /**< FoE request. */
        )
{
    req->dir = EC_DIR_OUTPUT;
    req->state = EC_INT_REQUEST_QUEUED;
    req->result = FOE_BUSY;
    req->jiffies_start = jiffies;
}

/****************************************************************************/

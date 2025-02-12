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

/** \file
 * Register request functions.
 */

/****************************************************************************/

#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/slab.h>

#include "reg_request.h"

/****************************************************************************/

/** Register request constructor.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_reg_request_init(
        ec_reg_request_t *reg, /**< Register request. */
        size_t size /**< Memory size. */
        )
{
    if (!(reg->data = (uint8_t *) kmalloc(size, GFP_KERNEL))) {
        EC_ERR("Failed to allocate %zu bytes of register memory.\n", size);
        return -ENOMEM;
    }

    INIT_LIST_HEAD(&reg->list);
    reg->mem_size = size;
    memset(reg->data, 0x00, size);
    reg->dir = EC_DIR_INVALID;
    reg->address = 0;
    reg->transfer_size = 0;
    reg->state = EC_INT_REQUEST_INIT;
    reg->ring_position = 0;
    return 0;
}

/****************************************************************************/

/** Register request destructor.
 */
void ec_reg_request_clear(
        ec_reg_request_t *reg /**< Register request. */
        )
{
    if (reg->data) {
        kfree(reg->data);
    }
}

/*****************************************************************************
 * Application interface.
 ****************************************************************************/

uint8_t *ecrt_reg_request_data(const ec_reg_request_t *reg)
{
    return reg->data;
}

/****************************************************************************/

ec_request_state_t ecrt_reg_request_state(const ec_reg_request_t *reg)
{
   return ec_request_state_translation_table[reg->state];
}

/****************************************************************************/

int ecrt_reg_request_write(ec_reg_request_t *reg, uint16_t address,
        size_t size)
{
    reg->dir = EC_DIR_OUTPUT;
    reg->address = address;
    reg->transfer_size = min(size, reg->mem_size);
    reg->state = EC_INT_REQUEST_QUEUED;
    return 0;
}

/****************************************************************************/

int ecrt_reg_request_read(ec_reg_request_t *reg, uint16_t address,
        size_t size)
{
    reg->dir = EC_DIR_INPUT;
    reg->address = address;
    reg->transfer_size = min(size, reg->mem_size);
    reg->state = EC_INT_REQUEST_QUEUED;
    return 0;
}

/****************************************************************************/

/** \cond */

EXPORT_SYMBOL(ecrt_reg_request_data);
EXPORT_SYMBOL(ecrt_reg_request_state);
EXPORT_SYMBOL(ecrt_reg_request_write);
EXPORT_SYMBOL(ecrt_reg_request_read);

/** \endcond */

/****************************************************************************/

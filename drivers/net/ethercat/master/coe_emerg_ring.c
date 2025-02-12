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
 *  vim: expandtab
 *
 ****************************************************************************/

/** \file
 * EtherCAT CoE emergency ring buffer methods.
 */

/****************************************************************************/

#include <linux/slab.h>

#include "coe_emerg_ring.h"

/****************************************************************************/

/** Emergency ring buffer constructor.
 */
void ec_coe_emerg_ring_init(
        ec_coe_emerg_ring_t *ring, /**< Emergency ring. */
        ec_slave_config_t *sc /**< Slave configuration. */
        )
{
    ring->sc = sc;
    ring->msgs = NULL;
    ring->size = 0;
    ring->read_index = 0;
    ring->write_index = 0;
    ring->overruns = 0;
}

/****************************************************************************/

/** Emergency ring buffer destructor.
 */
void ec_coe_emerg_ring_clear(
        ec_coe_emerg_ring_t *ring /**< Emergency ring. */
        )
{
    if (ring->msgs) {
        kfree(ring->msgs);
    }
}

/****************************************************************************/

/** Set the ring size.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_coe_emerg_ring_size(
        ec_coe_emerg_ring_t *ring, /**< Emergency ring. */
        size_t size /**< Maximum number of messages in the ring. */
        )
{
    ring->size = 0;

    if (size < 0) {
        size = 0;
    }

    ring->read_index = ring->write_index = 0;

    if (ring->msgs) {
        kfree(ring->msgs);
    }
    ring->msgs = NULL;

    if (size == 0) {
        return 0;
    }

    ring->msgs = kmalloc(sizeof(ec_coe_emerg_msg_t) * (size + 1), GFP_KERNEL);
    if (!ring->msgs) {
        return -ENOMEM;
    }

    ring->size = size;
    return 0;
}

/****************************************************************************/

/** Add a new emergency message.
 */
void ec_coe_emerg_ring_push(
        ec_coe_emerg_ring_t *ring, /**< Emergency ring. */
        const u8 *msg /**< Emergency message. */
        )
{
    if (!ring->size ||
            (ring->write_index + 1) % (ring->size + 1) == ring->read_index) {
        ring->overruns++;
        return;
    }

    memcpy(ring->msgs[ring->write_index].data, msg,
            EC_COE_EMERGENCY_MSG_SIZE);
    ring->write_index = (ring->write_index + 1) % (ring->size + 1);
}

/****************************************************************************/

/** Remove an emergency message from the ring.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_coe_emerg_ring_pop(
        ec_coe_emerg_ring_t *ring, /**< Emergency ring. */
        u8 *msg /**< Memory to store the emergency message. */
        )
{
    if (ring->read_index == ring->write_index) {
        return -ENOENT;
    }

    memcpy(msg, ring->msgs[ring->read_index].data, EC_COE_EMERGENCY_MSG_SIZE);
    ring->read_index = (ring->read_index + 1) % (ring->size + 1);
    return 0;
}

/****************************************************************************/

/** Clear the ring.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_coe_emerg_ring_clear_ring(
        ec_coe_emerg_ring_t *ring /**< Emergency ring. */
        )
{
    ring->read_index = ring->write_index;
    ring->overruns = 0;
    return 0;
}

/****************************************************************************/

/** Read the number of overruns.
 *
 * \return Number of overruns.
 */
int ec_coe_emerg_ring_overruns(
        const ec_coe_emerg_ring_t *ring /**< Emergency ring. */
        )
{
    return ring->overruns;
}

/****************************************************************************/

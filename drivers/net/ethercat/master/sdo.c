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
   CANopen SDO functions.
*/

/****************************************************************************/

#include <linux/slab.h>

#include "master.h"

#include "sdo.h"

/****************************************************************************/

/** Constructor.
 */
void ec_sdo_init(
        ec_sdo_t *sdo, /**< SDO. */
        ec_slave_t *slave, /**< Parent slave. */
        uint16_t index /**< SDO index. */
        )
{
    sdo->slave = slave;
    sdo->index = index;
    sdo->object_code = 0x00;
    sdo->name = NULL;
    sdo->max_subindex = 0;
    INIT_LIST_HEAD(&sdo->entries);
}

/****************************************************************************/

/** SDO destructor.
 *
 * Clears and frees an SDO object.
 */
void ec_sdo_clear(
        ec_sdo_t *sdo /**< SDO. */
        )
{
    ec_sdo_entry_t *entry, *next;

    // free all entries
    list_for_each_entry_safe(entry, next, &sdo->entries, list) {
        list_del(&entry->list);
        ec_sdo_entry_clear(entry);
        kfree(entry);
    }

    if (sdo->name)
        kfree(sdo->name);
}

/****************************************************************************/

/** Get an SDO entry from an SDO via its subindex.
 *
 * \retval >0 Pointer to the requested SDO entry.
 * \retval NULL SDO entry not found.
 */
ec_sdo_entry_t *ec_sdo_get_entry(
        ec_sdo_t *sdo, /**< SDO. */
        uint8_t subindex /**< Entry subindex. */
        )
{
    ec_sdo_entry_t *entry;

    list_for_each_entry(entry, &sdo->entries, list) {
        if (entry->subindex != subindex)
            continue;
        return entry;
    }

    return NULL;
}

/****************************************************************************/

/** Get an SDO entry from an SDO via its subindex.
 *
 * const version.
 *
 * \retval >0 Pointer to the requested SDO entry.
 * \retval NULL SDO entry not found.
 */
const ec_sdo_entry_t *ec_sdo_get_entry_const(
        const ec_sdo_t *sdo, /**< SDO. */
        uint8_t subindex /**< Entry subindex. */
        )
{
    const ec_sdo_entry_t *entry;

    list_for_each_entry(entry, &sdo->entries, list) {
        if (entry->subindex != subindex)
            continue;
        return entry;
    }

    return NULL;
}

/****************************************************************************/

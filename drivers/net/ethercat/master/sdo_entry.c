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
   CANopen over EtherCAT SDO entry functions.
*/

/****************************************************************************/

#include <linux/slab.h>

#include "sdo_entry.h"

/****************************************************************************/

/** Constructor.
 */
void ec_sdo_entry_init(
        ec_sdo_entry_t *entry, /**< SDO entry. */
        ec_sdo_t *sdo, /**< Parent SDO. */
        uint8_t subindex /**< Subindex. */
        )
{
    entry->sdo = sdo;
    entry->subindex = subindex;
    entry->data_type = 0x0000;
    entry->bit_length = 0;
    entry->read_access[EC_SDO_ENTRY_ACCESS_PREOP] = 0;
    entry->read_access[EC_SDO_ENTRY_ACCESS_SAFEOP] = 0;
    entry->read_access[EC_SDO_ENTRY_ACCESS_OP] = 0;
    entry->write_access[EC_SDO_ENTRY_ACCESS_PREOP] = 0;
    entry->write_access[EC_SDO_ENTRY_ACCESS_SAFEOP] = 0;
    entry->write_access[EC_SDO_ENTRY_ACCESS_OP] = 0;
    entry->description = NULL;
}

/****************************************************************************/

/** Destructor.
 */
void ec_sdo_entry_clear(
        ec_sdo_entry_t *entry /**< SDO entry. */
        )
{

    if (entry->description)
        kfree(entry->description);
}

/****************************************************************************/

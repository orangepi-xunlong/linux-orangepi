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

/** \file
 * EtherCAT sync manager methods.
 */

/****************************************************************************/

#include "globals.h"
#include "slave.h"
#include "master.h"
#include "pdo.h"
#include "sync.h"

/****************************************************************************/

/** Constructor.
 */
void ec_sync_init(
        ec_sync_t *sync, /**< EtherCAT sync manager. */
        ec_slave_t *slave /**< EtherCAT slave. */
        )
{
    sync->slave = slave;
    sync->physical_start_address = 0x0000;
    sync->default_length = 0x0000;
    sync->control_register = 0x00;
    sync->enable = 0x00;
    ec_pdo_list_init(&sync->pdos);
}

/****************************************************************************/

/** Copy constructor.
 */
void ec_sync_init_copy(
        ec_sync_t *sync, /**< EtherCAT sync manager. */
        const ec_sync_t *other /**< Sync manager to copy from. */
        )
{
   sync->slave = other->slave;
   sync->physical_start_address = other->physical_start_address;
   sync->default_length = other->default_length;
   sync->control_register = other->control_register;
   sync->enable = other->enable;
   ec_pdo_list_init(&sync->pdos);
   ec_pdo_list_copy(&sync->pdos, &other->pdos);
}

/****************************************************************************/

/** Destructor.
 */
void ec_sync_clear(
        ec_sync_t *sync /**< EtherCAT sync manager. */
        )
{
    ec_pdo_list_clear(&sync->pdos);
}

/****************************************************************************/

/** Initializes a sync manager configuration page.
 *
 * The referenced memory (\a data) must be at least \a EC_SYNC_SIZE bytes.
 */
void ec_sync_page(
        const ec_sync_t *sync, /**< Sync manager. */
        uint8_t sync_index, /**< Index of the sync manager. */
        uint16_t data_size, /**< Data size. */
        const ec_sync_config_t *sync_config, /**< Configuration. */
        uint8_t pdo_xfer, /**< Non-zero, if PDOs will be transferred via this
                            sync manager. */
        uint8_t *data /**> Configuration memory. */
        )
{
    // enable only if (SII enable is set or PDO xfer)
    // and size is > 0 and SM is not virtual
    uint16_t enable = ((sync->enable & 0x01) || pdo_xfer)
                        && data_size
                        && ((sync->enable & 0x04) == 0);
    uint8_t control = sync->control_register;

    if (sync_config) {

        switch (sync_config->dir) {
            case EC_DIR_OUTPUT:
            case EC_DIR_INPUT:
                EC_WRITE_BIT(&control, 2,
                        sync_config->dir == EC_DIR_OUTPUT ? 1 : 0);
                EC_WRITE_BIT(&control, 3, 0);
                break;
            default:
                break;
        }

        switch (sync_config->watchdog_mode) {
            case EC_WD_ENABLE:
            case EC_WD_DISABLE:
                EC_WRITE_BIT(&control, 6,
                        sync_config->watchdog_mode == EC_WD_ENABLE);
                break;
            default:
                break;
        }
    }

    EC_SLAVE_DBG(sync->slave, 1, "SM%u: Addr 0x%04X, Size %3u,"
            " Ctrl 0x%02X, En %u\n",
            sync_index, sync->physical_start_address,
            data_size, control, enable);

    EC_WRITE_U16(data,     sync->physical_start_address);
    EC_WRITE_U16(data + 2, data_size);
    EC_WRITE_U8 (data + 4, control);
    EC_WRITE_U8 (data + 5, 0x00); // status byte (read only)
    EC_WRITE_U16(data + 6, enable);
}

/****************************************************************************/

/** Adds a PDO to the list of known mapped PDOs.
 *
 * \return 0 on success, else < 0
 */
int ec_sync_add_pdo(
        ec_sync_t *sync, /**< EtherCAT sync manager. */
        const ec_pdo_t *pdo /**< PDO to map. */
        )
{
    return ec_pdo_list_add_pdo_copy(&sync->pdos, pdo);
}

/****************************************************************************/

/** Determines the default direction from the control register.
 *
 * \return Direction.
 */
ec_direction_t ec_sync_default_direction(
        const ec_sync_t *sync /**< EtherCAT sync manager. */
        )
{
    switch ((sync->control_register & 0x0C) >> 2) {
        case 0x0: return EC_DIR_INPUT;
        case 0x1: return EC_DIR_OUTPUT;
        default: return EC_DIR_INVALID;
    }
}

/****************************************************************************/

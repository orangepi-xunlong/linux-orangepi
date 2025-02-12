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
 * EtherCAT sync manager configuration methods.
 */

/****************************************************************************/

#include "globals.h"
#include "sync_config.h"

/****************************************************************************/

/** Constructor.
 */
void ec_sync_config_init(
        ec_sync_config_t *sync_config /**< Sync manager configuration. */
        )
{
    sync_config->dir = EC_DIR_INVALID;
    sync_config->watchdog_mode = EC_WD_DEFAULT;
    ec_pdo_list_init(&sync_config->pdos);
}

/****************************************************************************/

/** Destructor.
 */
void ec_sync_config_clear(
        ec_sync_config_t *sync_config /**< Sync manager configuration. */
        )
{
    ec_pdo_list_clear(&sync_config->pdos);
}

/****************************************************************************/

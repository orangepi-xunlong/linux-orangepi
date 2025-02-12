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
 * EtherCAT sync manager.
 */

/****************************************************************************/

#ifndef __EC_SYNC_CONFIG_H__
#define __EC_SYNC_CONFIG_H__

#include "globals.h"
#include "pdo_list.h"

/****************************************************************************/

/** Sync manager configuration.
 */
typedef struct {
    ec_direction_t dir; /**< Sync manager direction. */
    ec_watchdog_mode_t watchdog_mode; /**< Watchdog mode. */
    ec_pdo_list_t pdos; /**< Current PDO assignment. */
} ec_sync_config_t;

/****************************************************************************/

void ec_sync_config_init(ec_sync_config_t *);
void ec_sync_config_clear(ec_sync_config_t *);

/****************************************************************************/

#endif

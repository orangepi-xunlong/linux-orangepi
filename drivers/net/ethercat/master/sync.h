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

#ifndef __EC_SYNC_H__
#define __EC_SYNC_H__

#include "globals.h"
#include "pdo_list.h"
#include "sync_config.h"

/****************************************************************************/

/** Sync manager.
 */
typedef struct {
    ec_slave_t *slave; /**< Slave, the sync manager belongs to. */
    uint16_t physical_start_address; /**< Physical start address. */
    uint16_t default_length; /**< Data length in bytes. */
    uint8_t control_register; /**< Control register value. */
    uint8_t enable; /**< Enable bit. */
    ec_pdo_list_t pdos; /**< Current PDO assignment. */
} ec_sync_t;

/****************************************************************************/

void ec_sync_init(ec_sync_t *, ec_slave_t *);
void ec_sync_init_copy(ec_sync_t *, const ec_sync_t *);
void ec_sync_clear(ec_sync_t *);
void ec_sync_page(const ec_sync_t *, uint8_t, uint16_t,
        const ec_sync_config_t *, uint8_t, uint8_t *);
int ec_sync_add_pdo(ec_sync_t *, const ec_pdo_t *);
ec_direction_t ec_sync_default_direction(const ec_sync_t *);

/****************************************************************************/

#endif

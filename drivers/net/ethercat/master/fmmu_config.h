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
 * EtherCAT FMMU configuration structure.
 */

/****************************************************************************/

#ifndef __EC_FMMU_CONFIG_H__
#define __EC_FMMU_CONFIG_H__

#include "globals.h"
#include "sync.h"

/****************************************************************************/

/** FMMU configuration.
 */
typedef struct {
    struct list_head list; /**< List node used by domain. */
    const ec_slave_config_t *sc; /**< EtherCAT slave config. */
    const ec_domain_t *domain; /**< Domain. */
    uint8_t sync_index; /**< Index of sync manager to use. */
    ec_direction_t dir; /**< FMMU direction. */
    uint32_t logical_start_address; /**< Logical start address. */
    unsigned int data_size; /**< Covered PDO size. */
} ec_fmmu_config_t;

/****************************************************************************/

void ec_fmmu_config_init(ec_fmmu_config_t *, ec_slave_config_t *,
        ec_domain_t *, uint8_t, ec_direction_t);

void ec_fmmu_config_page(const ec_fmmu_config_t *, const ec_sync_t *,
        uint8_t *);

/****************************************************************************/

#endif

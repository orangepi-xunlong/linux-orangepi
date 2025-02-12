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
   EtherCAT CANopen SDO entry structure.
*/

/****************************************************************************/

#ifndef __EC_SDO_ENTRY_H__
#define __EC_SDO_ENTRY_H__

#include <linux/list.h>
#include <linux/kobject.h>

#include "globals.h"

/****************************************************************************/

struct ec_sdo;
typedef struct ec_sdo ec_sdo_t; /**< \see ec_sdo. */

/****************************************************************************/

/** CANopen SDO entry.
 */
typedef struct {
    struct list_head list; /**< List item. */
    ec_sdo_t *sdo; /**< Parent SDO. */
    uint8_t subindex; /**< Subindex. */
    uint16_t data_type; /**< Data type. */
    uint16_t bit_length; /**< Data size in bit. */
    uint8_t read_access[EC_SDO_ENTRY_ACCESS_COUNT]; /**< Read access. */
    uint8_t write_access[EC_SDO_ENTRY_ACCESS_COUNT]; /**< Write access. */
    char *description; /**< Description. */
} ec_sdo_entry_t;

/****************************************************************************/

void ec_sdo_entry_init(ec_sdo_entry_t *, ec_sdo_t *, uint8_t);
void ec_sdo_entry_clear(ec_sdo_entry_t *);

/****************************************************************************/

#endif

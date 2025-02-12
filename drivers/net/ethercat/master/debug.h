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
   Network interface for debugging purposes.
*/

/****************************************************************************/

#ifndef __EC_DEBUG_H__
#define __EC_DEBUG_H__

#include "../devices/ecdev.h"

/****************************************************************************/

/** Debugging network interface.
 */
typedef struct
{
    ec_device_t *device; /**< Parent device. */
    struct net_device *dev; /**< net_device for virtual ethernet device */
    struct net_device_stats stats; /**< device statistics */
    uint8_t registered; /**< net_device is opened */
    uint8_t opened; /**< net_device is opened */
}
ec_debug_t;

/****************************************************************************/

int ec_debug_init(ec_debug_t *, ec_device_t *, const char *);
void ec_debug_clear(ec_debug_t *);
void ec_debug_register(ec_debug_t *, const struct net_device *);
void ec_debug_unregister(ec_debug_t *);
void ec_debug_send(ec_debug_t *, const uint8_t *, size_t);

#endif

/****************************************************************************/

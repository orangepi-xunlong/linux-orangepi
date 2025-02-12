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
 *
 * EtherCAT interface for EtherCAT device drivers.
 *
 * \defgroup DeviceInterface EtherCAT Device Interface
 *
 * Master interface for EtherCAT-capable network device drivers. Through the
 * EtherCAT device interface, EtherCAT-capable network device drivers are able
 * to connect their device(s) to the master, pass received frames and notify
 * the master about status changes. The master on his part, can send his
 * frames through connected devices.
 */

/****************************************************************************/

#ifndef __ECDEV_H__
#define __ECDEV_H__

#include <linux/netdevice.h>

/****************************************************************************/

struct ec_device;
typedef struct ec_device ec_device_t; /**< \see ec_device */

/** Device poll function type.
 */
typedef void (*ec_pollfunc_t)(struct net_device *);

/*****************************************************************************
 * Offering/withdrawal functions
 ****************************************************************************/

ec_device_t *ecdev_offer(struct net_device *net_dev, ec_pollfunc_t poll,
        struct module *module);
void ecdev_withdraw(ec_device_t *device);

/*****************************************************************************
 * Device methods
 ****************************************************************************/

int ecdev_open(ec_device_t *device);
void ecdev_close(ec_device_t *device);
void ecdev_receive(ec_device_t *device, const void *data, size_t size);
void ecdev_set_link(ec_device_t *device, uint8_t state);
uint8_t ecdev_get_link(const ec_device_t *device);

/****************************************************************************/

#endif

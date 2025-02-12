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
   EtherCAT master character device.
*/

/****************************************************************************/

#ifndef __EC_CDEV_H__
#define __EC_CDEV_H__

#include <linux/fs.h>
#include <linux/cdev.h>

#include "globals.h"

/****************************************************************************/

/** EtherCAT master character device.
*/
typedef struct {
    ec_master_t *master; /**< Master owning the device. */
    struct cdev cdev; /**< Character device. */
} ec_cdev_t;

/****************************************************************************/

int ec_cdev_init(ec_cdev_t *, ec_master_t *, dev_t);
void ec_cdev_clear(ec_cdev_t *);

/****************************************************************************/

#endif

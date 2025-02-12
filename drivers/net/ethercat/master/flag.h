/*****************************************************************************
 *
 *  Copyright (C) 2021  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT Slave Configuration Feature Flag.
*/

/****************************************************************************/

#ifndef __EC_FLAG_H__
#define __EC_FLAG_H__

#include <linux/list.h>

/****************************************************************************/

/** Slave configutation feature flag.
 */
typedef struct {
    struct list_head list; /**< List item. */
    char *key; /**< Flag key (null-terminated ASCII string. */
    int32_t value; /**< Flag value (meaning depends on key). */
} ec_flag_t;

/****************************************************************************/

int ec_flag_init(ec_flag_t *, const char *, int32_t);
void ec_flag_clear(ec_flag_t *);

/****************************************************************************/

#endif

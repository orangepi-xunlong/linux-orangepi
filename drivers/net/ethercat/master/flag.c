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

/** \file
 * Slave Configuration Feature Flag.
 */

/****************************************************************************/

#include <linux/slab.h>

#include "flag.h"

/****************************************************************************/

/** SDO request constructor.
 */
int ec_flag_init(
        ec_flag_t *flag, /**< Feature flag. */
        const char *key, /**< Feature key. */
        int32_t value /**< Feature value. */
        )
{
    if (!key || strlen(key) == 0) {
        return -EINVAL;
    }

    if (!(flag->key = (uint8_t *) kmalloc(strlen(key) + 1, GFP_KERNEL))) {
        return -ENOMEM;
    }

    strcpy(flag->key, key); // no strncpy, buffer is alloc'ed with strlen
    flag->value = value;
    return 0;
}

/****************************************************************************/

/** SDO request destructor.
 */
void ec_flag_clear(
        ec_flag_t *flag /**< Feature flag. */
        )
{
    if (flag->key) {
        kfree(flag->key);
        flag->key = NULL;
    }
}

/****************************************************************************/

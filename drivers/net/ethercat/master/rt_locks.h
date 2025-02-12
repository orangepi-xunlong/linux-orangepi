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
   Abstract locks for realtime part of interface.
*/

/****************************************************************************/

#ifndef __EC_LOCKS_H__
#define __EC_LOCKS_H__

#include "globals.h"
#include <linux/version.h>

#include <linux/semaphore.h>

/****************************************************************************/

#ifdef EC_USE_RTMUTEX

#include <linux/rtmutex.h>

typedef struct rt_mutex ec_lock_t;

static inline void ec_lock_init(ec_lock_t *sem) { rt_mutex_init(sem); }
static inline void ec_lock_down(ec_lock_t *sem) { rt_mutex_lock(sem); }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 14, 34)
static inline int ec_lock_down_interruptible(ec_lock_t *sem) {
    return rt_mutex_lock_interruptible(sem);
}
#else
static inline int ec_lock_down_interruptible(ec_lock_t *sem) {
    return rt_mutex_lock_interruptible(sem, 1);
}
#endif
static inline void ec_lock_up(ec_lock_t *sem) { rt_mutex_unlock(sem); }

#else

typedef struct semaphore ec_lock_t;

static inline void ec_lock_init(ec_lock_t *sem) { sema_init(sem, 1); }
static inline void ec_lock_down(ec_lock_t *sem) { down(sem); }
static inline int ec_lock_down_interruptible(ec_lock_t *sem) {
    return down_interruptible(sem);
}
static inline void ec_lock_up(ec_lock_t *sem) { up(sem); }

#endif

/****************************************************************************/

#endif

/****************************************************************************/

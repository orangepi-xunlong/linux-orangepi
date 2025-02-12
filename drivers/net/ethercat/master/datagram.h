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
   EtherCAT datagram structure.
*/

/****************************************************************************/

#ifndef __EC_DATAGRAM_H__
#define __EC_DATAGRAM_H__

#include <linux/list.h>
#include <linux/time.h>
#include <linux/timex.h>

#include "globals.h"

/****************************************************************************/

/** EtherCAT datagram type.
 */
typedef enum {
    EC_DATAGRAM_NONE = 0x00, /**< Dummy. */
    EC_DATAGRAM_APRD = 0x01, /**< Auto Increment Physical Read. */
    EC_DATAGRAM_APWR = 0x02, /**< Auto Increment Physical Write. */
    EC_DATAGRAM_APRW = 0x03, /**< Auto Increment Physical ReadWrite. */
    EC_DATAGRAM_FPRD = 0x04, /**< Configured Address Physical Read. */
    EC_DATAGRAM_FPWR = 0x05, /**< Configured Address Physical Write. */
    EC_DATAGRAM_FPRW = 0x06, /**< Configured Address Physical ReadWrite. */
    EC_DATAGRAM_BRD  = 0x07, /**< Broadcast Read. */
    EC_DATAGRAM_BWR  = 0x08, /**< Broadcast Write. */
    EC_DATAGRAM_BRW  = 0x09, /**< Broadcast ReadWrite. */
    EC_DATAGRAM_LRD  = 0x0A, /**< Logical Read. */
    EC_DATAGRAM_LWR  = 0x0B, /**< Logical Write. */
    EC_DATAGRAM_LRW  = 0x0C, /**< Logical ReadWrite. */
    EC_DATAGRAM_ARMW = 0x0D, /**< Auto Increment Physical Read Multiple
                               Write.  */
    EC_DATAGRAM_FRMW = 0x0E, /**< Configured Address Physical Read Multiple
                               Write. */
} ec_datagram_type_t;

/****************************************************************************/

/** EtherCAT datagram state.
 */
typedef enum {
    EC_DATAGRAM_INIT,      /**< Initial state of a new datagram. */
    EC_DATAGRAM_QUEUED,    /**< Queued for sending. */
    EC_DATAGRAM_SENT,      /**< Sent (still in the queue). */
    EC_DATAGRAM_RECEIVED,  /**< Received (dequeued). */
    EC_DATAGRAM_TIMED_OUT, /**< Timed out (dequeued). */
    EC_DATAGRAM_ERROR      /**< Error while sending/receiving (dequeued). */
} ec_datagram_state_t;

/****************************************************************************/

/** EtherCAT datagram.
 */
typedef struct {
    struct list_head queue; /**< Master datagram queue item,
        protected by user-supplied mutex. */
    struct list_head ext_queue; /**< External datagram queue item, protected by ext_queue_sem. */
    struct list_head sent; /**< Master list item for sent datagrams. */
    ec_device_index_t device_index; /**< Device via which the datagram shall
                                      be / was sent. */
    ec_datagram_type_t type; /**< Datagram type (APRD, BWR, etc.). */
    uint8_t address[EC_ADDR_LEN]; /**< Recipient address. */
    uint8_t *data; /**< Datagram payload. */
    ec_origin_t data_origin; /**< Origin of the \a data memory. */
    size_t mem_size; /**< Datagram \a data memory size. */
    size_t data_size; /**< Size of the data in \a data. */
    uint8_t index; /**< Index (set by master). */
    uint16_t working_counter; /**< Working counter. */
    ec_datagram_state_t state; /**< State. */
#ifdef EC_HAVE_CYCLES
    cycles_t cycles_sent; /**< Time, when the datagram was sent. */
#endif
    unsigned long jiffies_sent; /**< Jiffies, when the datagram was sent. */
#ifdef EC_HAVE_CYCLES
    cycles_t cycles_received; /**< Time, when the datagram was received. */
#endif
    unsigned long jiffies_received; /**< Jiffies, when the datagram was
                                      received. */
    unsigned int skip_count; /**< Number of requeues when not yet received. */
    unsigned long stats_output_jiffies; /**< Last statistics output. */
    char name[EC_DATAGRAM_NAME_SIZE]; /**< Description of the datagram. */
} ec_datagram_t;

/****************************************************************************/

void ec_datagram_init(ec_datagram_t *);
void ec_datagram_clear(ec_datagram_t *);
void ec_datagram_unqueue(ec_datagram_t *);
int ec_datagram_prealloc(ec_datagram_t *, size_t);
void ec_datagram_zero(ec_datagram_t *);

int ec_datagram_aprd(ec_datagram_t *, uint16_t, uint16_t, size_t);
int ec_datagram_apwr(ec_datagram_t *, uint16_t, uint16_t, size_t);
int ec_datagram_aprw(ec_datagram_t *, uint16_t, uint16_t, size_t);
int ec_datagram_armw(ec_datagram_t *, uint16_t, uint16_t, size_t);
int ec_datagram_fprd(ec_datagram_t *, uint16_t, uint16_t, size_t);
int ec_datagram_fpwr(ec_datagram_t *, uint16_t, uint16_t, size_t);
int ec_datagram_fprw(ec_datagram_t *, uint16_t, uint16_t, size_t);
int ec_datagram_frmw(ec_datagram_t *, uint16_t, uint16_t, size_t);
int ec_datagram_brd(ec_datagram_t *, uint16_t, size_t);
int ec_datagram_bwr(ec_datagram_t *, uint16_t, size_t);
int ec_datagram_brw(ec_datagram_t *, uint16_t, size_t);
int ec_datagram_lrd(ec_datagram_t *, uint32_t, size_t);
int ec_datagram_lwr(ec_datagram_t *, uint32_t, size_t);
int ec_datagram_lrw(ec_datagram_t *, uint32_t, size_t);
int ec_datagram_lrd_ext(ec_datagram_t *, uint32_t, size_t, uint8_t *);
int ec_datagram_lwr_ext(ec_datagram_t *, uint32_t, size_t, uint8_t *);
int ec_datagram_lrw_ext(ec_datagram_t *, uint32_t, size_t, uint8_t *);

void ec_datagram_print_state(const ec_datagram_t *);
void ec_datagram_print_wc_error(const ec_datagram_t *);
void ec_datagram_output_stats(ec_datagram_t *);
const char *ec_datagram_type_string(const ec_datagram_t *);

/****************************************************************************/

#endif

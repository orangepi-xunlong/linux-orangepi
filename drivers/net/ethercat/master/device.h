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
   EtherCAT device structure.
*/

/****************************************************************************/

#ifndef __EC_DEVICE_H__
#define __EC_DEVICE_H__

#include <linux/interrupt.h>

#include "../device/ecdev.h"
#include "globals.h"

/**
 * Size of the transmit ring.
 * This memory ring is used to transmit frames. It is necessary to use
 * different memory regions, because otherwise the network device DMA could
 * send the same data twice, if it is called twice.
 */
#define EC_TX_RING_SIZE 2

#ifdef EC_DEBUG_IF
#include "debug.h"
#endif

#ifdef EC_DEBUG_RING
#define EC_DEBUG_RING_SIZE 10

typedef enum {
    TX, RX
} ec_debug_frame_dir_t;

typedef struct {
    ec_debug_frame_dir_t dir;
    struct timeval t;
    uint8_t data[EC_MAX_DATA_SIZE];
    unsigned int data_size;
} ec_debug_frame_t;

#endif

/****************************************************************************/

/**
   EtherCAT device.
   An EtherCAT device is a network interface card, that is owned by an
   EtherCAT master to send and receive EtherCAT frames with.
*/

struct ec_device
{
    ec_master_t *master; /**< EtherCAT master */
    struct net_device *dev; /**< pointer to the assigned net_device */
    ec_pollfunc_t poll; /**< pointer to the device's poll function */
    struct module *module; /**< pointer to the device's owning module */
    uint8_t open; /**< true, if the net_device has been opened */
    uint8_t link_state; /**< device link state */
    struct sk_buff *tx_skb[EC_TX_RING_SIZE]; /**< transmit skb ring */
    unsigned int tx_ring_index; /**< last ring entry used to transmit */
#ifdef EC_HAVE_CYCLES
    cycles_t cycles_poll; /**< cycles of last poll */
#endif
#ifdef EC_DEBUG_RING
    struct timeval timeval_poll;
#endif
    unsigned long jiffies_poll; /**< jiffies of last poll */

    // Frame statistics
    u64 tx_count; /**< Number of frames sent. */
    u64 last_tx_count; /**< Number of frames sent of last statistics cycle. */
    u64 rx_count; /**< Number of frames received. */
    u64 last_rx_count; /**< Number of frames received of last statistics
                         cycle. */
    u64 tx_bytes; /**< Number of bytes sent. */
    u64 last_tx_bytes; /**< Number of bytes sent of last statistics cycle. */
    u64 rx_bytes; /**< Number of bytes received. */
    u64 last_rx_bytes; /**< Number of bytes received of last statistics cycle.
                        */
    u64 tx_errors; /**< Number of transmit errors. */
    s32 tx_frame_rates[EC_RATE_COUNT]; /**< Transmit rates in frames/s for
                                         different statistics cycle periods.
                                        */
    s32 rx_frame_rates[EC_RATE_COUNT]; /**< Receive rates in frames/s for
                                         different statistics cycle periods.
                                        */
    s32 tx_byte_rates[EC_RATE_COUNT]; /**< Transmit rates in byte/s for
                                        different statistics cycle periods. */
    s32 rx_byte_rates[EC_RATE_COUNT]; /**< Receive rates in byte/s for
                                        different statistics cycle periods. */

#ifdef EC_DEBUG_IF
    ec_debug_t dbg; /**< debug device */
#endif
#ifdef EC_DEBUG_RING
    ec_debug_frame_t debug_frames[EC_DEBUG_RING_SIZE];
    unsigned int debug_frame_index;
    unsigned int debug_frame_count;
#endif
};

/****************************************************************************/

int ec_device_init(ec_device_t *, ec_master_t *);
void ec_device_clear(ec_device_t *);

void ec_device_attach(ec_device_t *, struct net_device *, ec_pollfunc_t,
        struct module *);
void ec_device_detach(ec_device_t *);

int ec_device_open(ec_device_t *);
int ec_device_close(ec_device_t *);

void ec_device_poll(ec_device_t *);
uint8_t *ec_device_tx_data(ec_device_t *);
void ec_device_send(ec_device_t *, size_t);
void ec_device_clear_stats(ec_device_t *);
void ec_device_update_stats(ec_device_t *);

#ifdef EC_DEBUG_RING
void ec_device_debug_ring_append(ec_device_t *, ec_debug_frame_dir_t,
        const void *, size_t);
void ec_device_debug_ring_print(const ec_device_t *);
#endif

/****************************************************************************/

#endif

/*****************************************************************************
 *
 *  Copyright (C) 2006-2012  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT datagram pair methods.
*/

/****************************************************************************/

#include <linux/slab.h>

#include "master.h"
#include "datagram_pair.h"

/****************************************************************************/

/** Datagram pair constructor.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_datagram_pair_init(
        ec_datagram_pair_t *pair, /**< Datagram pair. */
        ec_domain_t *domain, /**< Parent domain. */
        uint32_t logical_offset, /**< Logical offset. */
        uint8_t *data, /**< Data pointer. */
        size_t data_size, /**< Data size. */
        const unsigned int used[] /**< input/output use count. */
        )
{
    ec_device_index_t dev_idx;
    int ret;

    INIT_LIST_HEAD(&pair->list);
    pair->domain = domain;

    for (dev_idx = EC_DEVICE_MAIN;
            dev_idx < ec_master_num_devices(domain->master); dev_idx++) {
        ec_datagram_init(&pair->datagrams[dev_idx]);
        snprintf(pair->datagrams[dev_idx].name,
                EC_DATAGRAM_NAME_SIZE, "domain%u-%u-%s", domain->index,
                logical_offset, ec_device_names[dev_idx != 0]);
        pair->datagrams[dev_idx].device_index = dev_idx;
    }

    pair->expected_working_counter = 0U;

    for (dev_idx = EC_DEVICE_BACKUP;
            dev_idx < ec_master_num_devices(domain->master); dev_idx++) {
        /* backup datagrams have their own memory */
        ret = ec_datagram_prealloc(&pair->datagrams[dev_idx], data_size);
        if (ret) {
            goto out_datagrams;
        }
    }

#if EC_MAX_NUM_DEVICES > 1
    if (!(pair->send_buffer = kmalloc(data_size, GFP_KERNEL))) {
        EC_MASTER_ERR(domain->master,
                "Failed to allocate domain send buffer!\n");
        ret = -ENOMEM;
        goto out_datagrams;
    }
#endif

    /* The ec_datagram_lxx() calls below can not fail, because either the
     * datagram has external memory or it is preallocated. */

    if (used[EC_DIR_OUTPUT] && used[EC_DIR_INPUT]) { // inputs and outputs
        ec_datagram_lrw_ext(&pair->datagrams[EC_DEVICE_MAIN],
                logical_offset, data_size, data);

        for (dev_idx = EC_DEVICE_BACKUP;
                dev_idx < ec_master_num_devices(domain->master); dev_idx++) {
            ec_datagram_lrw(&pair->datagrams[dev_idx],
                    logical_offset, data_size);
        }

        // If LRW is used, output FMMUs increment the working counter by 2,
        // while input FMMUs increment it by 1.
        pair->expected_working_counter =
            used[EC_DIR_OUTPUT] * 2 + used[EC_DIR_INPUT];
    } else if (used[EC_DIR_OUTPUT]) { // outputs only
        ec_datagram_lwr_ext(&pair->datagrams[EC_DEVICE_MAIN],
                logical_offset, data_size, data);
        for (dev_idx = EC_DEVICE_BACKUP;
                dev_idx < ec_master_num_devices(domain->master); dev_idx++) {
            ec_datagram_lwr(&pair->datagrams[dev_idx],
                    logical_offset, data_size);
        }

        pair->expected_working_counter = used[EC_DIR_OUTPUT];
    } else { // inputs only (or nothing)
        ec_datagram_lrd_ext(&pair->datagrams[EC_DEVICE_MAIN],
                logical_offset, data_size, data);
        for (dev_idx = EC_DEVICE_BACKUP;
                dev_idx < ec_master_num_devices(domain->master); dev_idx++) {
            ec_datagram_lrd(&pair->datagrams[dev_idx], logical_offset,
                    data_size);
        }

        pair->expected_working_counter = used[EC_DIR_INPUT];
    }

    for (dev_idx = EC_DEVICE_MAIN;
            dev_idx < ec_master_num_devices(domain->master); dev_idx++) {
        ec_datagram_zero(&pair->datagrams[dev_idx]);
    }

    return 0;

out_datagrams:
    for (dev_idx = EC_DEVICE_MAIN;
            dev_idx < ec_master_num_devices(domain->master); dev_idx++) {
        ec_datagram_clear(&pair->datagrams[dev_idx]);
    }

    return ret;
}

/****************************************************************************/

/** Datagram pair destructor.
 */
void ec_datagram_pair_clear(
        ec_datagram_pair_t *pair /**< Datagram pair. */
        )
{
    unsigned int dev_idx;

    for (dev_idx = EC_DEVICE_MAIN;
            dev_idx < ec_master_num_devices(pair->domain->master);
            dev_idx++) {
        ec_datagram_clear(&pair->datagrams[dev_idx]);
    }

#if EC_MAX_NUM_DEVICES > 1
    if (pair->send_buffer) {
        kfree(pair->send_buffer);
    }
#endif
}

/****************************************************************************/

/** Process received data.
 *
 * \return Working counter sum over all devices.
 */
uint16_t ec_datagram_pair_process(
        ec_datagram_pair_t *pair, /**< Datagram pair. */
        uint16_t wc_sum[] /**< Working counter sums. */
        )
{
    unsigned int dev_idx;
    uint16_t pair_wc = 0;

    for (dev_idx = 0; dev_idx < ec_master_num_devices(pair->domain->master);
            dev_idx++) {
        ec_datagram_t *datagram = &pair->datagrams[dev_idx];

#ifdef EC_RT_SYSLOG
        ec_datagram_output_stats(datagram);
#endif

        if (datagram->state == EC_DATAGRAM_RECEIVED) {
            pair_wc += datagram->working_counter;
            wc_sum[dev_idx] += datagram->working_counter;
        }
    }

    return pair_wc;
}

/****************************************************************************/

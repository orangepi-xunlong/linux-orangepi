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
 * Vendor specific over EtherCAT protocol handler functions.
 */

/****************************************************************************/

#include <linux/module.h>

#include "master.h"
#include "slave_config.h"
#include "mailbox.h"
#include "voe_handler.h"

/** VoE header size.
 */
#define EC_VOE_HEADER_SIZE 6

/** VoE response timeout in [ms].
 */
#define EC_VOE_RESPONSE_TIMEOUT 500

/****************************************************************************/

void ec_voe_handler_state_write_start(ec_voe_handler_t *);
void ec_voe_handler_state_write_response(ec_voe_handler_t *);

void ec_voe_handler_state_read_start(ec_voe_handler_t *);
void ec_voe_handler_state_read_check(ec_voe_handler_t *);
void ec_voe_handler_state_read_response(ec_voe_handler_t *);

void ec_voe_handler_state_read_nosync_start(ec_voe_handler_t *);
void ec_voe_handler_state_read_nosync_response(ec_voe_handler_t *);

void ec_voe_handler_state_end(ec_voe_handler_t *);
void ec_voe_handler_state_error(ec_voe_handler_t *);

/****************************************************************************/

/** VoE handler constructor.
 *
 * \return Return value of ec_datagram_prealloc().
 */
int ec_voe_handler_init(
        ec_voe_handler_t *voe, /**< VoE handler. */
        ec_slave_config_t *sc, /**< Parent slave configuration. */
        size_t size /**< Size of memory to reserve. */
        )
{
    voe->config = sc;
    voe->vendor_id = 0x00000000;
    voe->vendor_type = 0x0000;
    voe->data_size = 0;
    voe->dir = EC_DIR_INVALID;
    voe->state = ec_voe_handler_state_error;
    voe->request_state = EC_INT_REQUEST_INIT;

    ec_datagram_init(&voe->datagram);
    return ec_datagram_prealloc(&voe->datagram,
            size + EC_MBOX_HEADER_SIZE + EC_VOE_HEADER_SIZE);
}

/****************************************************************************/

/** VoE handler destructor.
 */
void ec_voe_handler_clear(
        ec_voe_handler_t *voe /**< VoE handler. */
        )
{
    ec_datagram_clear(&voe->datagram);
}

/****************************************************************************/

/** Get usable memory size.
 *
 * \return Memory size.
 */
size_t ec_voe_handler_mem_size(
        const ec_voe_handler_t *voe /**< VoE handler. */
        )
{
    if (voe->datagram.mem_size >= EC_MBOX_HEADER_SIZE + EC_VOE_HEADER_SIZE)
        return voe->datagram.mem_size -
            (EC_MBOX_HEADER_SIZE + EC_VOE_HEADER_SIZE);
    else
        return 0;
}

/*****************************************************************************
 * Application interface.
 ****************************************************************************/

int ecrt_voe_handler_send_header(ec_voe_handler_t *voe, uint32_t vendor_id,
        uint16_t vendor_type)
{
    voe->vendor_id = vendor_id;
    voe->vendor_type = vendor_type;
    return 0;
}

/****************************************************************************/

int ecrt_voe_handler_received_header(const ec_voe_handler_t *voe,
        uint32_t *vendor_id, uint16_t *vendor_type)
{
    uint8_t *header = voe->datagram.data + EC_MBOX_HEADER_SIZE;

    if (vendor_id)
        *vendor_id = EC_READ_U32(header);
    if (vendor_type)
        *vendor_type = EC_READ_U16(header + 4);
    return 0;
}

/****************************************************************************/

uint8_t *ecrt_voe_handler_data(const ec_voe_handler_t *voe)
{
    return voe->datagram.data + EC_MBOX_HEADER_SIZE + EC_VOE_HEADER_SIZE;
}

/****************************************************************************/

size_t ecrt_voe_handler_data_size(const ec_voe_handler_t *voe)
{
    return voe->data_size;
}

/****************************************************************************/

int ecrt_voe_handler_read(ec_voe_handler_t *voe)
{
    voe->dir = EC_DIR_INPUT;
    voe->state = ec_voe_handler_state_read_start;
    voe->request_state = EC_INT_REQUEST_BUSY;
    return 0;
}

/****************************************************************************/

int ecrt_voe_handler_read_nosync(ec_voe_handler_t *voe)
{
    voe->dir = EC_DIR_INPUT;
    voe->state = ec_voe_handler_state_read_nosync_start;
    voe->request_state = EC_INT_REQUEST_BUSY;
    return 0;
}

/****************************************************************************/

int ecrt_voe_handler_write(ec_voe_handler_t *voe, size_t size)
{
    voe->dir = EC_DIR_OUTPUT;
    voe->data_size = size;
    voe->state = ec_voe_handler_state_write_start;
    voe->request_state = EC_INT_REQUEST_BUSY;
    return 0;
}

/****************************************************************************/

ec_request_state_t ecrt_voe_handler_execute(ec_voe_handler_t *voe)
{
    if (voe->config->slave) { // FIXME locking?
        voe->state(voe);
        if (voe->request_state == EC_INT_REQUEST_BUSY) {
            ec_master_queue_datagram(voe->config->master, &voe->datagram);
        }
    } else {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
    }

    return ec_request_state_translation_table[voe->request_state];
}

/*****************************************************************************
 * State functions.
 ****************************************************************************/

/** Start writing VoE data.
 */
void ec_voe_handler_state_write_start(ec_voe_handler_t *voe)
{
    ec_slave_t *slave = voe->config->slave;
    uint8_t *data;

    if (slave->master->debug_level) {
        EC_SLAVE_DBG(slave, 0, "Writing %zu bytes of VoE data.\n",
               voe->data_size);
        ec_print_data(ecrt_voe_handler_data(voe), voe->data_size);
    }

    if (!(slave->sii.mailbox_protocols & EC_MBOX_VOE)) {
        EC_SLAVE_ERR(slave, "Slave does not support VoE!\n");
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        return;
    }

    data = ec_slave_mbox_prepare_send(slave, &voe->datagram,
            EC_MBOX_TYPE_VOE, EC_VOE_HEADER_SIZE + voe->data_size);
    if (IS_ERR(data)) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        return;
    }

    EC_WRITE_U32(data,     voe->vendor_id);
    EC_WRITE_U16(data + 4, voe->vendor_type);
    /* data already in datagram */

    voe->retries = EC_FSM_RETRIES;
    voe->jiffies_start = jiffies;
    voe->state = ec_voe_handler_state_write_response;
}

/****************************************************************************/

/** Wait for the mailbox response.
 */
void ec_voe_handler_state_write_response(ec_voe_handler_t *voe)
{
    ec_datagram_t *datagram = &voe->datagram;
    ec_slave_t *slave = voe->config->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && voe->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Failed to receive VoE write request datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        if (!datagram->working_counter) {
            unsigned long diff_ms =
                (jiffies - voe->jiffies_start) * 1000 / HZ;
            if (diff_ms < EC_VOE_RESPONSE_TIMEOUT) {
                EC_SLAVE_DBG(slave, 1, "Slave did not respond to"
                        " VoE write request. Retrying after %lu ms...\n",
                        diff_ms);
                // no response; send request datagram again
                return;
            }
        }
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Reception of VoE write request failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    EC_CONFIG_DBG(voe->config, 1, "VoE write request successful.\n");

    voe->request_state = EC_INT_REQUEST_SUCCESS;
    voe->state = ec_voe_handler_state_end;
}

/****************************************************************************/

/** Start reading VoE data.
 */
void ec_voe_handler_state_read_start(ec_voe_handler_t *voe)
{
    ec_datagram_t *datagram = &voe->datagram;
    ec_slave_t *slave = voe->config->slave;

    EC_SLAVE_DBG(slave, 1, "Reading VoE data.\n");

    if (!(slave->sii.mailbox_protocols & EC_MBOX_VOE)) {
        EC_SLAVE_ERR(slave, "Slave does not support VoE!\n");
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        return;
    }

    ec_slave_mbox_prepare_check(slave, datagram); // can not fail.

    voe->jiffies_start = jiffies;
    voe->retries = EC_FSM_RETRIES;
    voe->state = ec_voe_handler_state_read_check;
}

/****************************************************************************/

/** Check for new data in the mailbox.
 */
void ec_voe_handler_state_read_check(ec_voe_handler_t *voe)
{
    ec_datagram_t *datagram = &voe->datagram;
    ec_slave_t *slave = voe->config->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && voe->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Failed to receive VoE mailbox check datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Reception of VoE mailbox check"
                " datagram failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    if (!ec_slave_mbox_check(datagram)) {
        unsigned long diff_ms =
            (datagram->jiffies_received - voe->jiffies_start) * 1000 / HZ;
        if (diff_ms >= EC_VOE_RESPONSE_TIMEOUT) {
            voe->state = ec_voe_handler_state_error;
            voe->request_state = EC_INT_REQUEST_FAILURE;
            EC_SLAVE_ERR(slave, "Timeout while waiting for VoE data.\n");
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        voe->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    voe->retries = EC_FSM_RETRIES;
    voe->state = ec_voe_handler_state_read_response;
}

/****************************************************************************/

/** Read the pending mailbox data.
 */
void ec_voe_handler_state_read_response(ec_voe_handler_t *voe)
{
    ec_datagram_t *datagram = &voe->datagram;
    ec_slave_t *slave = voe->config->slave;
    ec_master_t *master = voe->config->master;
    uint8_t *data, mbox_prot;
    size_t rec_size;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && voe->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Failed to receive VoE read datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Reception of VoE read response failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        return;
    }

    if (mbox_prot != EC_MBOX_TYPE_VOE) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_WARN(slave, "Received mailbox protocol 0x%02X"
                " as response.\n", mbox_prot);
        ec_print_data(data, rec_size);
        return;
    }

    if (rec_size < EC_VOE_HEADER_SIZE) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Received VoE header is"
                " incomplete (%zu bytes)!\n", rec_size);
        return;
    }

    if (master->debug_level) {
        EC_CONFIG_DBG(voe->config, 0, "VoE data:\n");
        ec_print_data(data, rec_size);
    }

    voe->data_size = rec_size - EC_VOE_HEADER_SIZE;
    voe->request_state = EC_INT_REQUEST_SUCCESS;
    voe->state = ec_voe_handler_state_end; // success
}

/****************************************************************************/

/** Start reading VoE data without sending a sync message before.
 */
void ec_voe_handler_state_read_nosync_start(ec_voe_handler_t *voe)
{
    ec_datagram_t *datagram = &voe->datagram;
    ec_slave_t *slave = voe->config->slave;

    EC_SLAVE_DBG(slave, 1, "Reading VoE data.\n");

    if (!(slave->sii.mailbox_protocols & EC_MBOX_VOE)) {
        EC_SLAVE_ERR(slave, "Slave does not support VoE!\n");
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        return;
    }

    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.

    voe->jiffies_start = jiffies;
    voe->retries = EC_FSM_RETRIES;
    voe->state = ec_voe_handler_state_read_nosync_response;
}

/****************************************************************************/

/** Read the pending mailbox data without sending a sync message before. This
 *  might lead to an empty reponse from the client.
 */
void ec_voe_handler_state_read_nosync_response(ec_voe_handler_t *voe)
{
    ec_datagram_t *datagram = &voe->datagram;
    ec_slave_t *slave = voe->config->slave;
    ec_master_t *master = voe->config->master;
    uint8_t *data, mbox_prot;
    size_t rec_size;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && voe->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Failed to receive VoE read datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter == 0) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_DBG(slave, 1, "Slave did not send VoE data.\n");
        return;
    }

    if (datagram->working_counter != 1) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_WARN(slave, "Reception of VoE read response failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    if (!(data = ec_slave_mbox_fetch(slave, datagram,
                    &mbox_prot, &rec_size))) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        return;
    }

    if (mbox_prot != EC_MBOX_TYPE_VOE) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_WARN(slave, "Received mailbox protocol 0x%02X"
                " as response.\n", mbox_prot);
        ec_print_data(data, rec_size);
        return;
    }

    if (rec_size < EC_VOE_HEADER_SIZE) {
        voe->state = ec_voe_handler_state_error;
        voe->request_state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Received VoE header is"
                " incomplete (%zu bytes)!\n", rec_size);
        return;
    }

    if (master->debug_level) {
        EC_CONFIG_DBG(voe->config, 1, "VoE data:\n");
        ec_print_data(data, rec_size);
    }

    voe->data_size = rec_size - EC_VOE_HEADER_SIZE;
    voe->request_state = EC_INT_REQUEST_SUCCESS;
    voe->state = ec_voe_handler_state_end; // success
}

/****************************************************************************/

/** Successful termination state function.
 */
void ec_voe_handler_state_end(ec_voe_handler_t *voe)
{
}

/****************************************************************************/

/** Failure termination state function.
 */
void ec_voe_handler_state_error(ec_voe_handler_t *voe)
{
}

/****************************************************************************/

/** \cond */

EXPORT_SYMBOL(ecrt_voe_handler_send_header);
EXPORT_SYMBOL(ecrt_voe_handler_received_header);
EXPORT_SYMBOL(ecrt_voe_handler_data);
EXPORT_SYMBOL(ecrt_voe_handler_data_size);
EXPORT_SYMBOL(ecrt_voe_handler_read);
EXPORT_SYMBOL(ecrt_voe_handler_write);
EXPORT_SYMBOL(ecrt_voe_handler_execute);

/** \endcond */

/****************************************************************************/

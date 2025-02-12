/*****************************************************************************
 *
 *  Copyright (C) 2006-2020  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT SoE state machines.
*/

/****************************************************************************/

#include "globals.h"
#include "master.h"
#include "mailbox.h"
#include "fsm_soe.h"

/****************************************************************************/

/** SoE operations
 */
enum {
    OPCODE_READ_REQUEST   = 0x01, /**< Read request. */
    OPCODE_READ_RESPONSE  = 0x02, /**< Read response. */
    OPCODE_WRITE_REQUEST  = 0x03, /**< Write request. */
    OPCODE_WRITE_RESPONSE = 0x04  /**< Write response. */
};

/** Size of all SoE headers.
 */
#define EC_SOE_SIZE 0x04

/** SoE header size.
 */
#define EC_SOE_HEADER_SIZE (EC_MBOX_HEADER_SIZE + EC_SOE_SIZE)

/** SoE response timeout [ms].
 */
#define EC_SOE_RESPONSE_TIMEOUT 1000

/****************************************************************************/

// prototypes for private methods
void ec_print_soe_error(const ec_slave_t *, uint16_t);
void ec_fsm_soe_print_error(ec_fsm_soe_t *);
int ec_fsm_soe_prepare_read(ec_fsm_soe_t *, ec_datagram_t *);
void ec_fsm_soe_write_next_fragment(ec_fsm_soe_t *, ec_datagram_t *);

/****************************************************************************/

void ec_fsm_soe_read_start(ec_fsm_soe_t *, ec_datagram_t *);
void ec_fsm_soe_read_request(ec_fsm_soe_t *, ec_datagram_t *);
void ec_fsm_soe_read_check(ec_fsm_soe_t *, ec_datagram_t *);
void ec_fsm_soe_read_response(ec_fsm_soe_t *, ec_datagram_t *);

void ec_fsm_soe_write_start(ec_fsm_soe_t *, ec_datagram_t *);
void ec_fsm_soe_write_request(ec_fsm_soe_t *, ec_datagram_t *);
void ec_fsm_soe_write_check(ec_fsm_soe_t *, ec_datagram_t *);
void ec_fsm_soe_write_response(ec_fsm_soe_t *, ec_datagram_t *);

void ec_fsm_soe_end(ec_fsm_soe_t *, ec_datagram_t *);
void ec_fsm_soe_error(ec_fsm_soe_t *, ec_datagram_t *);

/****************************************************************************/

extern const ec_code_msg_t soe_error_codes[];

/****************************************************************************/

/** Outputs an SoE error code.
*/
void ec_print_soe_error(const ec_slave_t *slave, uint16_t error_code)
{
    const ec_code_msg_t *error_msg;

    for (error_msg = soe_error_codes; error_msg->code; error_msg++) {
        if (error_msg->code == error_code) {
            EC_SLAVE_ERR(slave, "SoE error 0x%04X: \"%s\".\n",
                   error_msg->code, error_msg->message);
            return;
        }
    }

    EC_SLAVE_ERR(slave, "Unknown SoE error 0x%04X.\n", error_code);
}

/****************************************************************************/

/** Constructor.
 */
void ec_fsm_soe_init(
        ec_fsm_soe_t *fsm /**< finite state machine */
        )
{
    fsm->state = NULL;
    fsm->datagram = NULL;
    fsm->fragment_size = 0;
}

/****************************************************************************/

/** Destructor.
 */
void ec_fsm_soe_clear(
        ec_fsm_soe_t *fsm /**< finite state machine */
        )
{
}

/****************************************************************************/

/** Starts to transfer an IDN to/from a slave.
 */
void ec_fsm_soe_transfer(
        ec_fsm_soe_t *fsm, /**< State machine. */
        ec_slave_t *slave, /**< EtherCAT slave. */
        ec_soe_request_t *request /**< SoE request. */
        )
{
    fsm->slave = slave;
    fsm->request = request;

    if (request->dir == EC_DIR_OUTPUT) {
        fsm->state = ec_fsm_soe_write_start;
    } else {
        fsm->state = ec_fsm_soe_read_start;
    }
}

/****************************************************************************/

/** Executes the current state of the state machine.
 *
 * \return 1 if the datagram was used, else 0.
 */
int ec_fsm_soe_exec(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    int datagram_used = 0;

    if (fsm->datagram &&
            (fsm->datagram->state == EC_DATAGRAM_INIT ||
             fsm->datagram->state == EC_DATAGRAM_QUEUED ||
             fsm->datagram->state == EC_DATAGRAM_SENT)) {
        // datagram not received yet
        return datagram_used;
    }

    fsm->state(fsm, datagram);

    datagram_used =
        fsm->state != ec_fsm_soe_end && fsm->state != ec_fsm_soe_error;

    if (datagram_used) {
        fsm->datagram = datagram;
    } else {
        fsm->datagram = NULL;
    }

    return datagram_used;
}

/****************************************************************************/

/** Returns, if the state machine terminated with success.
 *
 * \return non-zero if successful.
 */
int ec_fsm_soe_success(const ec_fsm_soe_t *fsm /**< Finite state machine */)
{
    return fsm->state == ec_fsm_soe_end;
}

/****************************************************************************/

/** Output information about a failed SoE transfer.
 */
void ec_fsm_soe_print_error(ec_fsm_soe_t *fsm /**< Finite state machine */)
{
    ec_soe_request_t *request = fsm->request;

    EC_SLAVE_ERR(fsm->slave, "");

    if (request->dir == EC_DIR_OUTPUT) {
        printk(KERN_CONT "Writing");
    } else {
        printk(KERN_CONT "Reading");
    }

    printk(KERN_CONT " IDN 0x%04X failed.\n", request->idn);
}

/*****************************************************************************
 * SoE read state machine
 ****************************************************************************/

/** Prepare a read operation.
 *
 * \return 0 on success, otherwise a negative error code.
 */
int ec_fsm_soe_prepare_read(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    uint8_t *data;
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    ec_soe_request_t *request = fsm->request;

    data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_SOE,
            EC_SOE_SIZE);
    if (IS_ERR(data)) {
        return PTR_ERR(data);
    }

    EC_WRITE_U8(data, OPCODE_READ_REQUEST | (request->drive_no & 0x07) << 5);
    EC_WRITE_U8(data + 1, 1 << 6); // request value
    EC_WRITE_U16(data + 2, request->idn);

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 0, "SSC read request:\n");
        ec_print_data(data, EC_SOE_SIZE);
    }

    fsm->request->jiffies_sent = jiffies;
    fsm->state = ec_fsm_soe_read_request;

    return 0;
}

/****************************************************************************/

/** SoE state: READ START.
 */
void ec_fsm_soe_read_start(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_soe_request_t *request = fsm->request;

    EC_SLAVE_DBG(slave, 1, "Reading IDN 0x%04X of drive %u.\n", request->idn,
            request->drive_no);

    if (!(slave->sii.mailbox_protocols & EC_MBOX_SOE)) {
        EC_SLAVE_ERR(slave, "Slave does not support SoE!\n");
        fsm->state = ec_fsm_soe_error;
        ec_fsm_soe_print_error(fsm);
        return;
    }

    request->data_size = 0;
    fsm->retries = EC_FSM_RETRIES;

    if (ec_fsm_soe_prepare_read(fsm, datagram)) {
        fsm->state = ec_fsm_soe_error;
        ec_fsm_soe_print_error(fsm);
    }
}

/****************************************************************************/

/** SoE state: READ REQUEST.
 */
void ec_fsm_soe_read_request(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    unsigned long diff_ms;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        if (ec_fsm_soe_prepare_read(fsm, datagram)) {
            fsm->state = ec_fsm_soe_error;
            ec_fsm_soe_print_error(fsm);
        }
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Failed to receive SoE read request: ");
        ec_datagram_print_state(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    diff_ms = (jiffies - fsm->request->jiffies_sent) * 1000 / HZ;

    if (fsm->datagram->working_counter != 1) {
        if (!fsm->datagram->working_counter) {
            if (diff_ms < EC_SOE_RESPONSE_TIMEOUT) {
                // no response; send request datagram again
                if (ec_fsm_soe_prepare_read(fsm, datagram)) {
                    fsm->state = ec_fsm_soe_error;
                    ec_fsm_soe_print_error(fsm);
                }
                return;
            }
        }
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Reception of SoE read request"
                " failed after %lu ms: ", diff_ms);
        ec_datagram_print_wc_error(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    fsm->jiffies_start = fsm->datagram->jiffies_sent;
    ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_soe_read_check;
}

/****************************************************************************/

/** CoE state: READ CHECK.
 */
void ec_fsm_soe_read_check(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Failed to receive SoE mailbox check datagram: ");
        ec_datagram_print_state(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Reception of SoE mailbox check"
                " datagram failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= EC_SOE_RESPONSE_TIMEOUT) {
            fsm->state = ec_fsm_soe_error;
            EC_SLAVE_ERR(slave, "Timeout after %lu ms while waiting for"
                    " read response.\n", diff_ms);
            ec_fsm_soe_print_error(fsm);
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_soe_read_response;
}

/****************************************************************************/

/** SoE state: READ RESPONSE.
 */
void ec_fsm_soe_read_response(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    uint8_t *data, mbox_prot, header, opcode, incomplete, error_flag,
            value_included;
    size_t rec_size, data_size;
    ec_soe_request_t *req = fsm->request;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Failed to receive SoE read response datagram: ");
        ec_datagram_print_state(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Reception of SoE read response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        fsm->state = ec_fsm_soe_error;
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 0, "SSC read response:\n");
        ec_print_data(data, rec_size);
    }

    if (mbox_prot != EC_MBOX_TYPE_SOE) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Received mailbox protocol 0x%02X as response.\n",
                mbox_prot);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (rec_size < EC_SOE_SIZE) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Received currupted SoE read response"
                " (%zu bytes)!\n", rec_size);
        ec_print_data(data, rec_size);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    header = EC_READ_U8(data);
    opcode = header & 0x7;
    incomplete = (header >> 3) & 1;
    error_flag = (header >> 4) & 1;

    if (opcode != OPCODE_READ_RESPONSE) {
        EC_SLAVE_ERR(slave, "Received no read response (opcode %x).\n",
                opcode);
        ec_print_data(data, rec_size);
        ec_fsm_soe_print_error(fsm);
        fsm->state = ec_fsm_soe_error;
        return;
    }

    if (error_flag) {
        req->error_code = EC_READ_U16(data + rec_size - 2);
        EC_SLAVE_ERR(slave, "Received error response:\n");
        ec_print_soe_error(slave, req->error_code);
        ec_fsm_soe_print_error(fsm);
        fsm->state = ec_fsm_soe_error;
        return;
    } else {
        req->error_code = 0x0000;
    }

    value_included = (EC_READ_U8(data + 1) >> 6) & 1;
    if (!value_included) {
        EC_SLAVE_ERR(slave, "No value included!\n");
        ec_fsm_soe_print_error(fsm);
        fsm->state = ec_fsm_soe_error;
        return;
    }

    data_size = rec_size - EC_SOE_SIZE;
    if (ec_soe_request_append_data(req,
                data + EC_SOE_SIZE, data_size)) {
        fsm->state = ec_fsm_soe_error;
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (incomplete) {
        EC_SLAVE_DBG(slave, 1, "SoE data incomplete. Waiting for fragment"
                " at offset %zu.\n", req->data_size);
        fsm->jiffies_start = fsm->datagram->jiffies_sent;
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_soe_read_check;
    } else {
        if (master->debug_level) {
            EC_SLAVE_DBG(slave, 0, "IDN data:\n");
            ec_print_data(req->data, req->data_size);
        }

        fsm->state = ec_fsm_soe_end; // success
    }
}

/*****************************************************************************
 * SoE write state machine
 ****************************************************************************/

/** Write next fragment.
 */
void ec_fsm_soe_write_next_fragment(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    ec_soe_request_t *req = fsm->request;
    uint8_t incomplete, *data;
    size_t max_fragment_size, remaining_size;
    uint16_t fragments_left;

    remaining_size = req->data_size - fsm->offset;
    max_fragment_size = slave->configured_rx_mailbox_size - EC_SOE_HEADER_SIZE;
    incomplete = remaining_size > max_fragment_size;
    fsm->fragment_size = incomplete ? max_fragment_size : remaining_size;
    fragments_left = remaining_size / fsm->fragment_size - 1;
    if (remaining_size % fsm->fragment_size) {
        fragments_left++;
    }

    data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_SOE,
            EC_SOE_SIZE + fsm->fragment_size);
    if (IS_ERR(data)) {
        fsm->state = ec_fsm_soe_error;
        ec_fsm_soe_print_error(fsm);
        return;
    }

    EC_WRITE_U8(data, OPCODE_WRITE_REQUEST | incomplete << 3 |
            (req->drive_no & 0x07) << 5);
    EC_WRITE_U8(data + 1, 1 << 6); // only value included
    EC_WRITE_U16(data + 2, incomplete ? fragments_left : req->idn);
    memcpy(data + EC_SOE_SIZE, req->data + fsm->offset, fsm->fragment_size);

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 0, "SSC write request:\n");
        ec_print_data(data, EC_SOE_SIZE + fsm->fragment_size);
    }

    fsm->state = ec_fsm_soe_write_request;
}

/****************************************************************************/

/** SoE state: WRITE START.
 */
void ec_fsm_soe_write_start(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_soe_request_t *req = fsm->request;

    EC_SLAVE_DBG(slave, 1, "Writing IDN 0x%04X of drive %u (%zu byte).\n",
            req->idn, req->drive_no, req->data_size);

    if (!(slave->sii.mailbox_protocols & EC_MBOX_SOE)) {
        EC_SLAVE_ERR(slave, "Slave does not support SoE!\n");
        fsm->state = ec_fsm_soe_error;
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (slave->configured_rx_mailbox_size <= EC_SOE_HEADER_SIZE) {
        EC_SLAVE_ERR(slave, "Mailbox size (%u) too small for SoE write.\n",
                slave->configured_rx_mailbox_size);
        fsm->state = ec_fsm_soe_error;
        ec_fsm_soe_print_error(fsm);
        return;
    }

    fsm->offset = 0;
    fsm->retries = EC_FSM_RETRIES;
    ec_fsm_soe_write_next_fragment(fsm, datagram);
    req->jiffies_sent = jiffies;
}

/****************************************************************************/

/** SoE state: WRITE REQUEST.
 */
void ec_fsm_soe_write_request(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    unsigned long diff_ms;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_fsm_soe_write_next_fragment(fsm, datagram);
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Failed to receive SoE write request: ");
        ec_datagram_print_state(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    diff_ms = (jiffies - fsm->request->jiffies_sent) * 1000 / HZ;

    if (fsm->datagram->working_counter != 1) {
        if (!fsm->datagram->working_counter) {
            if (diff_ms < EC_SOE_RESPONSE_TIMEOUT) {
                // no response; send request datagram again
                ec_fsm_soe_write_next_fragment(fsm, datagram);
                return;
            }
        }
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Reception of SoE write request"
                " failed after %lu ms: ", diff_ms);
        ec_datagram_print_wc_error(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    // fragment successfully sent
    fsm->offset += fsm->fragment_size;

    if (fsm->offset < fsm->request->data_size) {
        // next fragment
        fsm->retries = EC_FSM_RETRIES;
        ec_fsm_soe_write_next_fragment(fsm, datagram);
        fsm->request->jiffies_sent = jiffies;
    } else {
        // all fragments sent; query response
        fsm->jiffies_start = fsm->datagram->jiffies_sent;
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_soe_write_check;
    }
}

/****************************************************************************/

/** CoE state: WRITE CHECK.
 */
void ec_fsm_soe_write_check(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Failed to receive SoE write request datagram: ");
        ec_datagram_print_state(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Reception of SoE write request datagram: ");
        ec_datagram_print_wc_error(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (datagram->jiffies_received - fsm->jiffies_start) * 1000 / HZ;
        if (diff_ms >= EC_SOE_RESPONSE_TIMEOUT) {
            fsm->state = ec_fsm_soe_error;
            EC_SLAVE_ERR(slave, "Timeout after %lu ms while waiting"
                    " for write response.\n", diff_ms);
            ec_fsm_soe_print_error(fsm);
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_soe_write_response;
}

/****************************************************************************/

/** SoE state: WRITE RESPONSE.
 */
void ec_fsm_soe_write_response(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    ec_soe_request_t *req = fsm->request;
    uint8_t *data, mbox_prot, opcode, error_flag;
    uint16_t idn;
    size_t rec_size;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return; // FIXME: request again?
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Failed to receive SoE write"
                " response datagram: ");
        ec_datagram_print_state(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Reception of SoE write response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        fsm->state = ec_fsm_soe_error;
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 0, "SSC write response:\n");
        ec_print_data(data, rec_size);
    }

    if (mbox_prot != EC_MBOX_TYPE_SOE) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Received mailbox protocol 0x%02X as response.\n",
                mbox_prot);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    if (rec_size < EC_SOE_SIZE) {
        fsm->state = ec_fsm_soe_error;
        EC_SLAVE_ERR(slave, "Received corrupted SoE write response"
                " (%zu bytes)!\n", rec_size);
        ec_print_data(data, rec_size);
        ec_fsm_soe_print_error(fsm);
        return;
    }

    opcode = EC_READ_U8(data) & 0x7;
    if (opcode != OPCODE_WRITE_RESPONSE) {
        EC_SLAVE_ERR(slave, "Received no write response"
                " (opcode %x).\n", opcode);
        ec_print_data(data, rec_size);
        ec_fsm_soe_print_error(fsm);
        fsm->state = ec_fsm_soe_error;
        return;
    }

    idn = EC_READ_U16(data + 2);
    if (idn != req->idn) {
        EC_SLAVE_ERR(slave, "Received response for"
                " wrong IDN 0x%04x.\n", idn);
        ec_print_data(data, rec_size);
        ec_fsm_soe_print_error(fsm);
        fsm->state = ec_fsm_soe_error;
        return;
    }

    error_flag = (EC_READ_U8(data) >> 4) & 1;
    if (error_flag) {
        if (rec_size < EC_SOE_SIZE + 2) {
            EC_SLAVE_ERR(slave, "Received corrupted error response"
                    " - error flag set, but received size is %zu.\n",
                    rec_size);
        } else {
            req->error_code = EC_READ_U16(data + EC_SOE_SIZE);
            EC_SLAVE_ERR(slave, "Received error response:\n");
            ec_print_soe_error(slave, req->error_code);
        }
        ec_print_data(data, rec_size);
        ec_fsm_soe_print_error(fsm);
        fsm->state = ec_fsm_soe_error;
    } else {
        req->error_code = 0x0000;
        fsm->state = ec_fsm_soe_end; // success
    }
}

/****************************************************************************/

/** State: ERROR.
 */
void ec_fsm_soe_error(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

/** State: END.
 */
void ec_fsm_soe_end(
        ec_fsm_soe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

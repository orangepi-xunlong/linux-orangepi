/*****************************************************************************
 *
 *  Copyright (C) 2006-2014  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT EoE state machines.
*/

/****************************************************************************/

#include "globals.h"
#include "master.h"
#include "mailbox.h"
#include "fsm_eoe.h"

/****************************************************************************/

/** Maximum time to wait for a set IP parameter response.
 */
#define EC_EOE_RESPONSE_TIMEOUT 3000 // [ms]

/****************************************************************************/

// prototypes for private functions
void memcpy_swap32(void *, const void *);

int ec_fsm_eoe_prepare_set(ec_fsm_eoe_t *, ec_datagram_t *);

void ec_fsm_eoe_set_ip_start(ec_fsm_eoe_t *, ec_datagram_t *);
void ec_fsm_eoe_set_ip_request(ec_fsm_eoe_t *, ec_datagram_t *);
void ec_fsm_eoe_set_ip_check(ec_fsm_eoe_t *, ec_datagram_t *);
void ec_fsm_eoe_set_ip_response(ec_fsm_eoe_t *, ec_datagram_t *);

void ec_fsm_eoe_end(ec_fsm_eoe_t *, ec_datagram_t *);
void ec_fsm_eoe_error(ec_fsm_eoe_t *, ec_datagram_t *);

/****************************************************************************/

/** Host-architecture-independent 32-bit swap function.
 *
 * The internal storage of struct in_addr is always big-endian.
 * The mailbox protocol format to supply IPv4 adresses is little-endian
 * (Yuck!). So we need a swap function, that is independent of the CPU
 * architecture. ntohl()/htonl() can not be used, because they evaluate to
 * NOPs if the host architecture matches the target architecture!
 */
void memcpy_swap32(void *dst, const void *src)
{
    int i;
    for (i = 0; i < 4; i++) {
        ((u8 *) dst)[i] = ((const u8 *) src)[3 - i];
    }
}

/****************************************************************************/

/** Constructor.
 */
void ec_fsm_eoe_init(
        ec_fsm_eoe_t *fsm /**< finite state machine */
        )
{
    fsm->slave = NULL;
    fsm->retries = 0;
    fsm->state = NULL;
    fsm->datagram = NULL;
    fsm->jiffies_start = 0;
    fsm->request = NULL;
    fsm->frame_type_retries = 0;
}

/****************************************************************************/

/** Destructor.
 */
void ec_fsm_eoe_clear(
        ec_fsm_eoe_t *fsm /**< finite state machine */
        )
{
}

/****************************************************************************/

/** Starts to set the EoE IP partameters of a slave.
 */
void ec_fsm_eoe_set_ip_param(
        ec_fsm_eoe_t *fsm, /**< State machine. */
        ec_slave_t *slave, /**< EtherCAT slave. */
        ec_eoe_request_t *request /**< EoE request. */
        )
{
    fsm->slave = slave;
    fsm->request = request;
    fsm->state = ec_fsm_eoe_set_ip_start;
}

/****************************************************************************/

/** Executes the current state of the state machine.
 *
 * \return 1 if the datagram was used, else 0.
 */
int ec_fsm_eoe_exec(
        ec_fsm_eoe_t *fsm, /**< finite state machine */
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
        fsm->state != ec_fsm_eoe_end && fsm->state != ec_fsm_eoe_error;

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
int ec_fsm_eoe_success(const ec_fsm_eoe_t *fsm /**< Finite state machine */)
{
    return fsm->state == ec_fsm_eoe_end;
}

/*****************************************************************************
 * EoE set IP parameter state machine
 ****************************************************************************/

/** Prepare a set IP parameters operation.
 *
 * \return 0 on success, otherwise a negative error code.
 */
int ec_fsm_eoe_prepare_set(
        ec_fsm_eoe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    uint8_t *data, *cur;
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    ec_eoe_request_t *req = fsm->request;

    // Note: based on wireshark packet filter it suggests that the EOE_INIT
    //   information is a fixed size with fixed information positions.
    //   see: packet-ecatmb.h and packet-ecatmb.c
    //   However, TwinCAT 2.1 testing also indicates that if a piece of
    //   information is missing then all subsequent items are ignored
    //   Also, if you want DHCP, then only set the mac address.
    size_t size = 8 +                       // header + flags
                  ETH_ALEN +                // mac address
                  4 +                       // ip address
                  4 +                       // subnet mask
                  4 +                       // gateway
                  4 +                       // dns server
                  EC_MAX_HOSTNAME_SIZE;     // dns name

    data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_EOE,
            size);
    if (IS_ERR(data)) {
        return PTR_ERR(data);
    }

    // zero data
    memset(data, 0, size);

    // header
    EC_WRITE_U8(data, EC_EOE_FRAMETYPE_SET_IP_REQ); // Set IP parameter req.
    EC_WRITE_U8(data + 1, 0x00);                    // not used
    EC_WRITE_U16(data + 2, 0x0000);                 // not used

    EC_WRITE_U32(data + 4,
            ((req->mac_address_included != 0) << 0) |
            ((req->ip_address_included != 0) << 1) |
            ((req->subnet_mask_included != 0) << 2) |
            ((req->gateway_included != 0) << 3) |
            ((req->dns_included != 0) << 4) |
            ((req->name_included != 0) << 5)
            );

    cur = data + 8;

    if (req->mac_address_included) {
        memcpy(cur, req->mac_address, ETH_ALEN);
    }
    cur += ETH_ALEN;

    if (req->ip_address_included) {
        memcpy_swap32(cur, &req->ip_address);
    }
    cur += 4;

    if (req->subnet_mask_included) {
        memcpy_swap32(cur, &req->subnet_mask);
    }
    cur += 4;

    if (req->gateway_included) {
        memcpy_swap32(cur, &req->gateway);
    }
    cur += 4;

    if (req->dns_included) {
        memcpy_swap32(cur, &req->dns);
    }
    cur += 4;

    if (req->name_included) {
        memcpy(cur, req->name, EC_MAX_HOSTNAME_SIZE);
    }
    cur += EC_MAX_HOSTNAME_SIZE;

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 0, "Set IP parameter request:\n");
        ec_print_data(data, cur - data);
    }

    fsm->request->jiffies_sent = jiffies;

    return 0;
}

/****************************************************************************/

/** EoE state: SET IP START.
 */
void ec_fsm_eoe_set_ip_start(
        ec_fsm_eoe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    EC_SLAVE_DBG(slave, 1, "Setting IP parameters.\n");

    if (!(slave->sii.mailbox_protocols & EC_MBOX_EOE)) {
        EC_SLAVE_ERR(slave, "Slave does not support EoE!\n");
        fsm->state = ec_fsm_eoe_error;
        return;
    }

    if (ec_fsm_eoe_prepare_set(fsm, datagram)) {
        fsm->state = ec_fsm_eoe_error;
        return;
    }

    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_eoe_set_ip_request;
}

/****************************************************************************/

/** EoE state: SET IP REQUEST.
 */
void ec_fsm_eoe_set_ip_request(
        ec_fsm_eoe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        if (ec_fsm_eoe_prepare_set(fsm, datagram)) {
            fsm->state = ec_fsm_eoe_error;
        }
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_ERR(slave, "Failed to receive EoE set IP parameter"
                " request: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        unsigned long diff_ms =
            (jiffies - fsm->request->jiffies_sent) * 1000 / HZ;

        if (!fsm->datagram->working_counter) {
            if (diff_ms < EC_EOE_RESPONSE_TIMEOUT) {
                // no response; send request datagram again
                if (ec_fsm_eoe_prepare_set(fsm, datagram)) {
                    fsm->state = ec_fsm_eoe_error;
                }
                return;
            }
        }
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_ERR(slave, "Reception of EoE set IP parameter request"
                " failed after %lu ms: ", diff_ms);
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    fsm->jiffies_start = fsm->datagram->jiffies_sent;
    ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_eoe_set_ip_check;
    fsm->frame_type_retries = 10;
}

/****************************************************************************/

/** EoE state: SET IP CHECK.
 */
void ec_fsm_eoe_set_ip_check(
        ec_fsm_eoe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_ERR(slave, "Failed to receive EoE mailbox check datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_ERR(slave, "Reception of EoE mailbox check"
                " datagram failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= EC_EOE_RESPONSE_TIMEOUT) {
            fsm->state = ec_fsm_eoe_error;
            EC_SLAVE_ERR(slave, "Timeout after %lu ms while waiting for"
                    " set IP parameter response.\n", diff_ms);
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_eoe_set_ip_response;
}

/****************************************************************************/

/** EoE state: SET IP RESPONSE.
 */
void ec_fsm_eoe_set_ip_response(
        ec_fsm_eoe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    uint8_t *data, mbox_prot, frame_type;
    size_t rec_size;
    ec_eoe_request_t *req = fsm->request;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_ERR(slave, "Failed to receive EoE read response datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_ERR(slave, "Reception of EoE read response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        fsm->state = ec_fsm_eoe_error;
        return;
    }

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 0, "Set IP parameter response:\n");
        ec_print_data(data, rec_size);
    }

    if (mbox_prot != EC_MBOX_TYPE_EOE) {
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_ERR(slave, "Received mailbox protocol 0x%02X as response.\n",
                mbox_prot);
        return;
    }

    if (rec_size < 4) {
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_ERR(slave, "Received currupted EoE set IP parameter response"
                " (%zu bytes)!\n", rec_size);
        ec_print_data(data, rec_size);
        return;
    }

    frame_type = EC_READ_U8(data) & 0x0f;

    if (frame_type != EC_EOE_FRAMETYPE_SET_IP_RES) {
        if (master->debug_level) {
            EC_SLAVE_DBG(slave, 0, "Received no set IP parameter response"
                    " (frame type %x).\n", frame_type);
            ec_print_data(data, rec_size);
        }
        if (fsm->frame_type_retries--) {
            // there may be an EoE segment left in the mailbox.
            // discard it and receive again.
            fsm->jiffies_start = fsm->datagram->jiffies_sent;
            ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
            fsm->retries = EC_FSM_RETRIES;
            fsm->state = ec_fsm_eoe_set_ip_check;
            return;
        }
        else {
            EC_SLAVE_ERR(slave, "Received no set IP parameter response.\n");
            fsm->state = ec_fsm_eoe_error;
            return;
        }
    }

    req->result = EC_READ_U16(data + 2); // result code 0x0000 means success

    if (req->result) {
        fsm->state = ec_fsm_eoe_error;
        EC_SLAVE_DBG(slave, 1, "EoE set IP parameters failed with result code"
                " 0x%04X.\n", req->result);
    } else {
        fsm->state = ec_fsm_eoe_end; // success
    }
}

/****************************************************************************/

/** State: ERROR.
 */
void ec_fsm_eoe_error(
        ec_fsm_eoe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

/** State: END.
 */
void ec_fsm_eoe_end(
        ec_fsm_eoe_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

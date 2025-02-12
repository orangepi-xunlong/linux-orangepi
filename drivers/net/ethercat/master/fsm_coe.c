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
 * EtherCAT CoE state machines.
 */

/****************************************************************************/

#include "globals.h"
#include "master.h"
#include "mailbox.h"
#include "fsm_coe.h"
#include "slave_config.h"

/****************************************************************************/

/** Maximum time in ms to wait for responses when reading out the dictionary.
 */
#define EC_FSM_COE_DICT_TIMEOUT 1000

/** CoE download request header size.
 */
#define EC_COE_DOWN_REQ_HEADER_SIZE 10

/** CoE download segment request header size.
 */
#define EC_COE_DOWN_SEG_REQ_HEADER_SIZE 3

/** Minimum size of download segment.
 */
#define EC_COE_DOWN_SEG_MIN_DATA_SIZE 7

/** Enable debug output for CoE retries.
 */
#define DEBUG_RETRIES 0

/** Enable warning output if transfers take too long.
 */
#define DEBUG_LONG 0

/****************************************************************************/

// prototypes for private methods
void ec_canopen_abort_msg(const ec_slave_t *, uint32_t);
int ec_fsm_coe_check_emergency(const ec_fsm_coe_t *, const uint8_t *, size_t);
int ec_fsm_coe_prepare_dict(ec_fsm_coe_t *, ec_datagram_t *);
int ec_fsm_coe_dict_prepare_desc(ec_fsm_coe_t *, ec_datagram_t *);
int ec_fsm_coe_dict_prepare_entry(ec_fsm_coe_t *, ec_datagram_t *);
int ec_fsm_coe_prepare_down_start(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_down_prepare_segment_request(ec_fsm_coe_t *, ec_datagram_t *);
int ec_fsm_coe_prepare_up(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_up_prepare_segment_request(ec_fsm_coe_t *, ec_datagram_t *);

/****************************************************************************/

void ec_fsm_coe_dict_start(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_request(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_check(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_response(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_desc_request(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_desc_check(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_desc_response(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_entry_request(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_entry_check(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_dict_entry_response(ec_fsm_coe_t *, ec_datagram_t *);

void ec_fsm_coe_down_start(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_down_request(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_down_check(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_down_response(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_down_seg_check(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_down_seg_response(ec_fsm_coe_t *, ec_datagram_t *);

void ec_fsm_coe_up_start(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_up_request(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_up_check(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_up_response(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_up_seg_request(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_up_seg_check(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_up_seg_response(ec_fsm_coe_t *, ec_datagram_t *);

void ec_fsm_coe_end(ec_fsm_coe_t *, ec_datagram_t *);
void ec_fsm_coe_error(ec_fsm_coe_t *, ec_datagram_t *);

/****************************************************************************/

/** SDO abort messages.
 *
 * The "abort SDO transfer request" supplies an abort code, which can be
 * translated to clear text. This table does the mapping of the codes and
 * messages.
 */
const ec_code_msg_t sdo_abort_messages[] = {
    {0x05030000, "Toggle bit not changed"},
    {0x05040000, "SDO protocol timeout"},
    {0x05040001, "Client/Server command specifier not valid or unknown"},
    {0x05040005, "Out of memory"},
    {0x06010000, "Unsupported access to an object"},
    {0x06010001, "Attempt to read a write-only object"},
    {0x06010002, "Attempt to write a read-only object"},
    {0x06020000, "This object does not exist in the object directory"},
    {0x06040041, "The object cannot be mapped into the PDO"},
    {0x06040042, "The number and length of the objects to be mapped would"
     " exceed the PDO length"},
    {0x06040043, "General parameter incompatibility reason"},
    {0x06040047, "Gerneral internal incompatibility in device"},
    {0x06060000, "Access failure due to a hardware error"},
    {0x06070010, "Data type does not match, length of service parameter does"
     " not match"},
    {0x06070012, "Data type does not match, length of service parameter too"
     " high"},
    {0x06070013, "Data type does not match, length of service parameter too"
     " low"},
    {0x06090011, "Subindex does not exist"},
    {0x06090030, "Value range of parameter exceeded"},
    {0x06090031, "Value of parameter written too high"},
    {0x06090032, "Value of parameter written too low"},
    {0x06090036, "Maximum value is less than minimum value"},
    {0x08000000, "General error"},
    {0x08000020, "Data cannot be transferred or stored to the application"},
    {0x08000021, "Data cannot be transferred or stored to the application"
     " because of local control"},
    {0x08000022, "Data cannot be transferred or stored to the application"
     " because of the present device state"},
    {0x08000023, "Object dictionary dynamic generation fails or no object"
     " dictionary is present"},
    {}
};

/****************************************************************************/

/** Outputs an SDO abort message.
 */
void ec_canopen_abort_msg(
        const ec_slave_t *slave, /**< Slave. */
        uint32_t abort_code /**< Abort code to search for. */
        )
{
    const ec_code_msg_t *abort_msg;

    for (abort_msg = sdo_abort_messages; abort_msg->code; abort_msg++) {
        if (abort_msg->code == abort_code) {
            EC_SLAVE_ERR(slave, "SDO abort message 0x%08X: \"%s\".\n",
                   abort_msg->code, abort_msg->message);
            return;
        }
    }

    EC_SLAVE_ERR(slave, "Unknown SDO abort code 0x%08X.\n", abort_code);
}

/****************************************************************************/

/** Constructor.
 */
void ec_fsm_coe_init(
        ec_fsm_coe_t *fsm /**< Finite state machine */
        )
{
    fsm->state = NULL;
    fsm->datagram = NULL;
}

/****************************************************************************/

/** Destructor.
 */
void ec_fsm_coe_clear(
        ec_fsm_coe_t *fsm /**< Finite state machine */
        )
{
}

/****************************************************************************/

/** Starts reading a slaves' SDO dictionary.
 */
void ec_fsm_coe_dictionary(
        ec_fsm_coe_t *fsm, /**< Finite state machine */
        ec_slave_t *slave /**< EtherCAT slave */
        )
{
    fsm->slave = slave;
    fsm->state = ec_fsm_coe_dict_start;
}

/****************************************************************************/

/** Starts to transfer an SDO to/from a slave.
 */
void ec_fsm_coe_transfer(
        ec_fsm_coe_t *fsm, /**< State machine. */
        ec_slave_t *slave, /**< EtherCAT slave. */
        ec_sdo_request_t *request /**< SDO request. */
        )
{
    fsm->slave = slave;
    fsm->request = request;

    if (request->dir == EC_DIR_OUTPUT) {
        fsm->state = ec_fsm_coe_down_start;
    }
    else {
        fsm->state = ec_fsm_coe_up_start;
    }
}

/****************************************************************************/

/** Executes the current state of the state machine.
 *
 * \return 1 if the datagram was used, else 0.
 */
int ec_fsm_coe_exec(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
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
        fsm->state != ec_fsm_coe_end && fsm->state != ec_fsm_coe_error;

    if (datagram_used) {
        fsm->datagram = datagram;
    } else {
        fsm->datagram = NULL;
    }

    return datagram_used;
}

/****************************************************************************/

/** Returns, if the state machine terminated with success.
 * \return non-zero if successful.
 */
int ec_fsm_coe_success(
        const ec_fsm_coe_t *fsm /**< Finite state machine */
        )
{
    return fsm->state == ec_fsm_coe_end;
}

/****************************************************************************/

/** Check if the received data are a CoE emergency request.
 *
 * If the check is positive, the emergency request is output.
 *
 * \return The data were an emergency request.
 */
int ec_fsm_coe_check_emergency(
        const ec_fsm_coe_t *fsm, /**< Finite state machine */
        const uint8_t *data, /**< CoE mailbox data. */
        size_t size /**< CoE mailbox data size. */
        )
{
    if (size < 2 || ((EC_READ_U16(data) >> 12) & 0x0F) != 0x01)
        return 0;

    if (size < 10) {
        EC_SLAVE_WARN(fsm->slave, "Received incomplete CoE Emergency"
                " request:\n");
        ec_print_data(data, size);
        return 1;
    }

    {
        ec_slave_config_t *sc = fsm->slave->config;
        if (sc) {
            ec_coe_emerg_ring_push(&sc->emerg_ring, data + 2);
        }
    }

    EC_SLAVE_WARN(fsm->slave, "CoE Emergency Request received:\n"
            "Error code 0x%04X, Error register 0x%02X, data:\n",
            EC_READ_U16(data + 2), EC_READ_U8(data + 4));
    ec_print_data(data + 5, 5);
    return 1;
}

/*****************************************************************************
 *  CoE dictionary state machine
 ****************************************************************************/

/** Prepare a dictionary request.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_fsm_coe_prepare_dict(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    uint8_t *data = ec_slave_mbox_prepare_send(slave, datagram,
			EC_MBOX_TYPE_COE, 8);
    if (IS_ERR(data)) {
        return PTR_ERR(data);
    }

    EC_WRITE_U16(data, 0x8 << 12); // SDO information
    EC_WRITE_U8 (data + 2, 0x01); // Get OD List Request
    EC_WRITE_U8 (data + 3, 0x00);
    EC_WRITE_U16(data + 4, 0x0000);
    EC_WRITE_U16(data + 6, 0x0001); // deliver all SDOs!

    fsm->state = ec_fsm_coe_dict_request;
    return 0;
}

/****************************************************************************/

/** CoE state: DICT START.
 */
void ec_fsm_coe_dict_start(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (!(slave->sii.mailbox_protocols & EC_MBOX_COE)) {
        EC_SLAVE_ERR(slave, "Slave does not support CoE!\n");
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (slave->sii.has_general && !slave->sii.coe_details.enable_sdo_info) {
        EC_SLAVE_ERR(slave, "Slave does not support"
                " SDO information service!\n");
        fsm->state = ec_fsm_coe_error;
        return;
    }

    fsm->retries = EC_FSM_RETRIES;

    if (ec_fsm_coe_prepare_dict(fsm, datagram)) {
        fsm->state = ec_fsm_coe_error;
    }
}

/****************************************************************************/

/** CoE state: DICT REQUEST.
 * \todo Timeout behavior
 */
void ec_fsm_coe_dict_request(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        if (ec_fsm_coe_prepare_dict(fsm, datagram)) {
            fsm->state = ec_fsm_coe_error;
        }
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE dictionary"
                " request datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE dictionary request failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    fsm->jiffies_start = fsm->datagram->jiffies_sent;

    ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_dict_check;
}

/****************************************************************************/

/** CoE state: DICT CHECK.
 */
void ec_fsm_coe_dict_check(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE mailbox check datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave,"Reception of CoE mailbox check"
                " datagram failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= EC_FSM_COE_DICT_TIMEOUT) {
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Timeout while waiting for"
                    " SDO dictionary list response.\n");
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_dict_response;
}

/****************************************************************************/

/** Prepare an object description request.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_fsm_coe_dict_prepare_desc(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    u8 *data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_COE,
			8);
    if (IS_ERR(data)) {
        return PTR_ERR(data);
    }

    EC_WRITE_U16(data, 0x8 << 12); // SDO information
    EC_WRITE_U8 (data + 2, 0x03); // Get object description request
    EC_WRITE_U8 (data + 3, 0x00);
    EC_WRITE_U16(data + 4, 0x0000);
    EC_WRITE_U16(data + 6, fsm->sdo->index); // SDO index

    fsm->state = ec_fsm_coe_dict_desc_request;
    return 0;
}

/****************************************************************************/

/**
   CoE state: DICT RESPONSE.
   \todo Timeout behavior
*/

void ec_fsm_coe_dict_response(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    uint8_t *data, mbox_prot;
    size_t rec_size;
    unsigned int sdo_count, i;
    uint16_t sdo_index, fragments_left;
    ec_sdo_t *sdo;
    bool first_segment;
    size_t index_list_offset;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE dictionary"
                " response datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE dictionary response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (mbox_prot != EC_MBOX_TYPE_COE) {
        EC_SLAVE_ERR(slave, "Received mailbox protocol 0x%02X as response.\n",
                mbox_prot);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (ec_fsm_coe_check_emergency(fsm, data, rec_size)) {
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_dict_check;
        return;
    }

    if (rec_size < 3) {
        EC_SLAVE_ERR(slave, "Received corrupted SDO dictionary response"
                " (size %zu).\n", rec_size);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (EC_READ_U16(data) >> 12 == 0x8 && // SDO information
        (EC_READ_U8(data + 2) & 0x7F) == 0x07) { // error response
        EC_SLAVE_ERR(slave, "SDO information error response!\n");
        if (rec_size < 10) {
            EC_SLAVE_ERR(slave, "Incomplete SDO information"
                    " error response:\n");
            ec_print_data(data, rec_size);
        } else {
            ec_canopen_abort_msg(slave, EC_READ_U32(data + 6));
        }
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (EC_READ_U16(data) >> 12 != 0x8 || // SDO information
        (EC_READ_U8 (data + 2) & 0x7F) != 0x02) { // Get OD List response
        if (fsm->slave->master->debug_level) {
            EC_SLAVE_DBG(slave, 1, "Invalid SDO list response!"
                    " Retrying...\n");
            ec_print_data(data, rec_size);
        }
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_dict_check;
        return;
    }

    first_segment = list_empty(&slave->sdo_dictionary) ? true : false;
    index_list_offset = first_segment ? 8 : 6;

    if (rec_size < index_list_offset || rec_size % 2) {
        EC_SLAVE_ERR(slave, "Invalid data size %zu!\n", rec_size);
        ec_print_data(data, rec_size);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    sdo_count = (rec_size - index_list_offset) / 2;

    for (i = 0; i < sdo_count; i++) {
        sdo_index = EC_READ_U16(data + index_list_offset + i * 2);
        if (!sdo_index) {
            EC_SLAVE_DBG(slave, 1, "SDO dictionary contains index 0x0000.\n");
            continue;
        }

        if (!(sdo = (ec_sdo_t *) kmalloc(sizeof(ec_sdo_t), GFP_KERNEL))) {
            EC_SLAVE_ERR(slave, "Failed to allocate memory for SDO!\n");
            fsm->state = ec_fsm_coe_error;
            return;
        }

        ec_sdo_init(sdo, slave, sdo_index);
        list_add_tail(&sdo->list, &slave->sdo_dictionary);
    }

    fragments_left = EC_READ_U16(data + 4);
    if (fragments_left) {
        EC_SLAVE_DBG(slave, 1, "SDO list fragments left: %u\n",
                fragments_left);
    }

    if (EC_READ_U8(data + 2) & 0x80 || fragments_left) {
        // more messages waiting. check again.
        fsm->jiffies_start = fsm->datagram->jiffies_sent;
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_dict_check;
        return;
    }

    if (list_empty(&slave->sdo_dictionary)) {
        // no SDOs in dictionary. finished.
        fsm->state = ec_fsm_coe_end; // success
        return;
    }

    // fetch SDO descriptions
    fsm->sdo = list_entry(slave->sdo_dictionary.next, ec_sdo_t, list);

    fsm->retries = EC_FSM_RETRIES;
    if (ec_fsm_coe_dict_prepare_desc(fsm, datagram)) {
        fsm->state = ec_fsm_coe_error;
    }
}

/****************************************************************************/

/**
   CoE state: DICT DESC REQUEST.
   \todo Timeout behavior
*/

void ec_fsm_coe_dict_desc_request(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        if (ec_fsm_coe_dict_prepare_desc(fsm, datagram)) {
            fsm->state = ec_fsm_coe_error;
        }
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE SDO"
                " description request datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE SDO description"
                " request failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    fsm->jiffies_start = fsm->datagram->jiffies_sent;

    ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_dict_desc_check;
}

/****************************************************************************/

/**
   CoE state: DICT DESC CHECK.
*/

void ec_fsm_coe_dict_desc_check(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE mailbox check datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE mailbox check"
                " datagram failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= EC_FSM_COE_DICT_TIMEOUT) {
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Timeout while waiting for"
                    " SDO 0x%04x object description response.\n",
                    fsm->sdo->index);
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_dict_desc_response;
}

/****************************************************************************/

/** Prepare an entry description request.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_fsm_coe_dict_prepare_entry(
        ec_fsm_coe_t *fsm, /**< Finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    u8 *data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_COE,
			10);
    if (IS_ERR(data)) {
        return PTR_ERR(data);
    }

    EC_WRITE_U16(data, 0x8 << 12); // SDO information
    EC_WRITE_U8 (data + 2, 0x05); // Get entry description request
    EC_WRITE_U8 (data + 3, 0x00);
    EC_WRITE_U16(data + 4, 0x0000);
    EC_WRITE_U16(data + 6, fsm->sdo->index); // SDO index
    EC_WRITE_U8 (data + 8, fsm->subindex); // SDO subindex
    EC_WRITE_U8 (data + 9, 0x01); // value info (access rights only)

    fsm->state = ec_fsm_coe_dict_entry_request;
    return 0;
}

/****************************************************************************/

/**
   CoE state: DICT DESC RESPONSE.
   \todo Timeout behavior
*/

void ec_fsm_coe_dict_desc_response(
        ec_fsm_coe_t *fsm, /**< Finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_sdo_t *sdo = fsm->sdo;
    uint8_t *data, mbox_prot;
    size_t rec_size, name_size;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE SDO description"
                " response datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE SDO description"
                " response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (mbox_prot != EC_MBOX_TYPE_COE) {
        EC_SLAVE_ERR(slave, "Received mailbox protocol 0x%02X as response.\n",
                mbox_prot);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (ec_fsm_coe_check_emergency(fsm, data, rec_size)) {
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_dict_desc_check;
        return;
    }

    if (rec_size < 3) {
        EC_SLAVE_ERR(slave, "Received corrupted SDO description response"
                " (size %zu).\n", rec_size);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (EC_READ_U16(data) >> 12 == 0x8 && // SDO information
        (EC_READ_U8 (data + 2) & 0x7F) == 0x07) { // error response
        EC_SLAVE_ERR(slave, "SDO information error response while"
                " fetching SDO 0x%04X!\n", sdo->index);
        ec_canopen_abort_msg(slave, EC_READ_U32(data + 6));
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (rec_size < 8) {
        EC_SLAVE_ERR(slave, "Received corrupted SDO"
                " description response (size %zu).\n", rec_size);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (EC_READ_U16(data) >> 12 != 0x8 || // SDO information
        (EC_READ_U8 (data + 2) & 0x7F) != 0x04 || // Object desc. response
        EC_READ_U16(data + 6) != sdo->index) { // SDO index
        if (fsm->slave->master->debug_level) {
            EC_SLAVE_DBG(slave, 1, "Invalid object description response while"
                    " fetching SDO 0x%04X!\n", sdo->index);
            ec_print_data(data, rec_size);
        }
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_dict_desc_check;
        return;
    }

    if (rec_size < 12) {
        EC_SLAVE_ERR(slave, "Invalid data size!\n");
        ec_print_data(data, rec_size);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    sdo->max_subindex = EC_READ_U8(data + 10);
    sdo->object_code = EC_READ_U8(data + 11);

    name_size = rec_size - 12;
    if (name_size) {
        if (!(sdo->name = kmalloc(name_size + 1, GFP_KERNEL))) {
            EC_SLAVE_ERR(slave, "Failed to allocate SDO name!\n");
            fsm->state = ec_fsm_coe_error;
            return;
        }

        memcpy(sdo->name, data + 12, name_size);
        sdo->name[name_size] = 0;
    }

    if (EC_READ_U8(data + 2) & 0x80) {
        EC_SLAVE_ERR(slave, "Fragment follows (not implemented)!\n");
        fsm->state = ec_fsm_coe_error;
        return;
    }

    // start fetching entries

    fsm->subindex = 0;
    fsm->retries = EC_FSM_RETRIES;

    if (ec_fsm_coe_dict_prepare_entry(fsm, datagram)) {
        fsm->state = ec_fsm_coe_error;
    }
}

/****************************************************************************/

/**
   CoE state: DICT ENTRY REQUEST.
   \todo Timeout behavior
*/

void ec_fsm_coe_dict_entry_request(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        if (ec_fsm_coe_dict_prepare_entry(fsm, datagram)) {
            fsm->state = ec_fsm_coe_error;
        }
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE SDO entry"
                " request datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE SDO entry request failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    fsm->jiffies_start = fsm->datagram->jiffies_sent;

    ec_slave_mbox_prepare_check(slave, datagram); // can not fail
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_dict_entry_check;
}

/****************************************************************************/

/**
   CoE state: DICT ENTRY CHECK.
*/

void ec_fsm_coe_dict_entry_check(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE mailbox check datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE mailbox check"
                " datagram failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= EC_FSM_COE_DICT_TIMEOUT) {
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Timeout while waiting for"
                    " SDO entry 0x%04x:%x description response.\n",
                    fsm->sdo->index, fsm->subindex);
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_dict_entry_response;
}

/****************************************************************************/

/**
   CoE state: DICT ENTRY RESPONSE.
   \todo Timeout behavior
*/

void ec_fsm_coe_dict_entry_response(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_sdo_t *sdo = fsm->sdo;
    uint8_t *data, mbox_prot;
    size_t rec_size, data_size;
    ec_sdo_entry_t *entry;
    u16 word;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE SDO"
                " description response datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE SDO description"
                " response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (mbox_prot != EC_MBOX_TYPE_COE) {
        EC_SLAVE_ERR(slave, "Received mailbox protocol"
                " 0x%02X as response.\n", mbox_prot);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (ec_fsm_coe_check_emergency(fsm, data, rec_size)) {
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_dict_entry_check;
        return;
    }

    if (rec_size < 3) {
        EC_SLAVE_ERR(slave, "Received corrupted SDO entry"
                " description response (size %zu).\n", rec_size);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (EC_READ_U16(data) >> 12 == 0x8 && // SDO information
        (EC_READ_U8 (data + 2) & 0x7F) == 0x07) { // error response
        EC_SLAVE_WARN(slave, "SDO information error response while"
               " fetching SDO entry 0x%04X:%02X!\n",
               sdo->index, fsm->subindex);
        ec_canopen_abort_msg(slave, EC_READ_U32(data + 6));

        /* There may be gaps in the subindices, so try to continue with next
         * subindex. */

    } else {

        if (rec_size < 9) {
            EC_SLAVE_ERR(slave, "Received corrupted SDO entry"
                    " description response (size %zu).\n", rec_size);
            fsm->state = ec_fsm_coe_error;
            return;
        }

        if (EC_READ_U16(data) >> 12 != 0x8 || // SDO information
            (EC_READ_U8(data + 2) & 0x7F) != 0x06 || // Entry desc. response
            EC_READ_U16(data + 6) != sdo->index || // SDO index
            EC_READ_U8(data + 8) != fsm->subindex) { // SDO subindex
            if (fsm->slave->master->debug_level) {
                EC_SLAVE_DBG(slave, 1, "Invalid entry description response"
                        " while fetching SDO entry 0x%04X:%02X!\n",
                        sdo->index, fsm->subindex);
                ec_print_data(data, rec_size);
            }
            // check for CoE response again
            ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
            fsm->retries = EC_FSM_RETRIES;
            fsm->state = ec_fsm_coe_dict_entry_check;
            return;
        }

        if (rec_size < 16) {
            EC_SLAVE_ERR(slave, "Invalid data size %zu!\n", rec_size);
            ec_print_data(data, rec_size);
            fsm->state = ec_fsm_coe_error;
            return;
        }

        data_size = rec_size - 16;

        if (!(entry = (ec_sdo_entry_t *)
              kmalloc(sizeof(ec_sdo_entry_t), GFP_KERNEL))) {
            EC_SLAVE_ERR(slave, "Failed to allocate entry!\n");
            fsm->state = ec_fsm_coe_error;
            return;
        }

        ec_sdo_entry_init(entry, sdo, fsm->subindex);
        entry->data_type = EC_READ_U16(data + 10);
        entry->bit_length = EC_READ_U16(data + 12);

        // read access rights
        word = EC_READ_U16(data + 14);
        entry->read_access[EC_SDO_ENTRY_ACCESS_PREOP] = word & 0x0001;
        entry->read_access[EC_SDO_ENTRY_ACCESS_SAFEOP] =
            (word >> 1)  & 0x0001;
        entry->read_access[EC_SDO_ENTRY_ACCESS_OP] = (word >> 2)  & 0x0001;
        entry->write_access[EC_SDO_ENTRY_ACCESS_PREOP] = (word >> 3) & 0x0001;
        entry->write_access[EC_SDO_ENTRY_ACCESS_SAFEOP] =
            (word >> 4)  & 0x0001;
        entry->write_access[EC_SDO_ENTRY_ACCESS_OP] = (word >> 5)  & 0x0001;

        if (data_size) {
            uint8_t *desc;
            if (!(desc = kmalloc(data_size + 1, GFP_KERNEL))) {
                EC_SLAVE_ERR(slave, "Failed to allocate SDO entry name!\n");
                fsm->state = ec_fsm_coe_error;
                return;
            }
            memcpy(desc, data + 16, data_size);
            desc[data_size] = 0;
            entry->description = desc;
        }

        list_add_tail(&entry->list, &sdo->entries);
    }

    if (fsm->subindex < sdo->max_subindex) {

        fsm->subindex++;
        fsm->retries = EC_FSM_RETRIES;

        if (ec_fsm_coe_dict_prepare_entry(fsm, datagram)) {
            fsm->state = ec_fsm_coe_error;
        }

        return;
    }

    // another SDO description to fetch?
    if (fsm->sdo->list.next != &slave->sdo_dictionary) {

        fsm->sdo = list_entry(fsm->sdo->list.next, ec_sdo_t, list);
        fsm->retries = EC_FSM_RETRIES;

        if (ec_fsm_coe_dict_prepare_desc(fsm, datagram)) {
            fsm->state = ec_fsm_coe_error;
        }

        return;
    }

    fsm->state = ec_fsm_coe_end;
}

/*****************************************************************************
 *  CoE state machine
 ****************************************************************************/

/** Prepare a donwnload request.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_fsm_coe_prepare_down_start(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    u8 *data;
    ec_slave_t *slave = fsm->slave;
    ec_sdo_request_t *request = fsm->request;
    uint8_t data_set_size;

    if (request->data_size > 0 && request->data_size <= 4) {
        // use expedited transfer mode for lengths between 1 and 4 bytes
        data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_COE,
                EC_COE_DOWN_REQ_HEADER_SIZE);
        if (IS_ERR(data)) {
            request->errno = PTR_ERR(data);
            return PTR_ERR(data);
        }

        fsm->remaining = 0;

        data_set_size = 4 - request->data_size;

        EC_WRITE_U16(data, 0x2 << 12); // SDO request
        EC_WRITE_U8 (data + 2, (0x3 // size specified, expedited
                    | data_set_size << 2
                    | ((request->complete_access ? 1 : 0) << 4)
                    | 0x1 << 5)); // Download request
        EC_WRITE_U16(data + 3, request->index);
        EC_WRITE_U8 (data + 5,
                request->complete_access ? 0x00 : request->subindex);
        memcpy(data + 6, request->data, request->data_size);
        memset(data + 6 + request->data_size, 0x00, 4 - request->data_size);

        if (slave->master->debug_level) {
            EC_SLAVE_DBG(slave, 1, "Expedited download request:\n");
            ec_print_data(data, EC_COE_DOWN_REQ_HEADER_SIZE);
        }
    }
    else { // data_size < 1 or data_size > 4, use normal transfer type
        size_t data_size,
               max_data_size =
                   slave->configured_rx_mailbox_size - EC_MBOX_HEADER_SIZE,
               required_data_size =
                   EC_COE_DOWN_REQ_HEADER_SIZE + request->data_size;

        if (max_data_size < required_data_size) {
            // segmenting needed
            data_size = max_data_size;
        } else {
            data_size = required_data_size;
        }

        data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_COE,
                data_size);
        if (IS_ERR(data)) {
            request->errno = PTR_ERR(data);
            return PTR_ERR(data);
        }

        fsm->offset = 0;
        fsm->remaining = request->data_size;

        EC_WRITE_U16(data, 0x2 << 12); // SDO request
        EC_WRITE_U8(data + 2,
                0x1 // size indicator, normal
                | ((request->complete_access ? 1 : 0) << 4)
                | 0x1 << 5); // Download request
        EC_WRITE_U16(data + 3, request->index);
        EC_WRITE_U8 (data + 5,
                request->complete_access ? 0x00 : request->subindex);
        EC_WRITE_U32(data + 6, request->data_size);

        if (data_size > EC_COE_DOWN_REQ_HEADER_SIZE) {
            size_t segment_size = data_size - EC_COE_DOWN_REQ_HEADER_SIZE;
            memcpy(data + EC_COE_DOWN_REQ_HEADER_SIZE,
                    request->data, segment_size);
            fsm->offset += segment_size;
            fsm->remaining -= segment_size;
        }

        if (slave->master->debug_level) {
            EC_SLAVE_DBG(slave, 1, "Normal download request:\n");
            ec_print_data(data, data_size);
        }
    }

    fsm->state = ec_fsm_coe_down_request;
    return 0;
}

/****************************************************************************/

/** CoE state: DOWN START.
 */
void ec_fsm_coe_down_start(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_sdo_request_t *request = fsm->request;

    if (fsm->slave->master->debug_level) {
        char subidxstr[10];
        if (request->complete_access) {
            subidxstr[0] = 0x00;
        } else {
            sprintf(subidxstr, ":%02X", request->subindex);
        }
        EC_SLAVE_DBG(slave, 1, "Downloading SDO 0x%04X%s.\n",
                request->index, subidxstr);
        ec_print_data(request->data, request->data_size);
    }

    if (!(slave->sii.mailbox_protocols & EC_MBOX_COE)) {
        EC_SLAVE_ERR(slave, "Slave does not support CoE!\n");
        request->errno = EPROTONOSUPPORT;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (slave->configured_rx_mailbox_size <
            EC_MBOX_HEADER_SIZE + EC_COE_DOWN_REQ_HEADER_SIZE) {
        EC_SLAVE_ERR(slave, "Mailbox too small!\n");
        request->errno = ENOBUFS;
        fsm->state = ec_fsm_coe_error;
        return;
    }


    fsm->request->jiffies_sent = jiffies;
    fsm->retries = EC_FSM_RETRIES;

    if (ec_fsm_coe_prepare_down_start(fsm, datagram)) {
        fsm->state = ec_fsm_coe_error;
    }
}

/****************************************************************************/

/**
   CoE state: DOWN REQUEST.
   \todo Timeout behavior
*/

void ec_fsm_coe_down_request(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    unsigned long diff_ms;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        if (ec_fsm_coe_prepare_down_start(fsm, datagram)) {
            fsm->state = ec_fsm_coe_error;
        }
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE download"
                " request datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    diff_ms = (jiffies - fsm->request->jiffies_sent) * 1000 / HZ;

    if (fsm->datagram->working_counter != 1) {
        if (!fsm->datagram->working_counter) {
            if (diff_ms < fsm->request->response_timeout) {
#if DEBUG_RETRIES
                EC_SLAVE_DBG(slave, 1, "Slave did not respond to SDO"
                        " download request. Retrying after %lu ms...\n",
                        diff_ms);
#endif
                // no response; send request datagram again
                if (ec_fsm_coe_prepare_down_start(fsm, datagram)) {
                    fsm->state = ec_fsm_coe_error;
                }
                return;
            }
        }
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE download request"
                " for SDO 0x%04x:%x failed with timeout after %lu ms: ",
                fsm->request->index, fsm->request->subindex, diff_ms);
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

#if DEBUG_LONG
    if (diff_ms > 200) {
        EC_SLAVE_WARN(slave, "SDO 0x%04x:%x download took %lu ms.\n",
                fsm->request->index, fsm->request->subindex, diff_ms);
    }
#endif

    fsm->jiffies_start = fsm->datagram->jiffies_sent;

    ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_down_check;
}

/****************************************************************************/

/** CoE state: DOWN CHECK.
 */
void ec_fsm_coe_down_check(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE mailbox check"
                " datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE mailbox check"
                " datagram failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= fsm->request->response_timeout) {
            fsm->request->errno = EIO;
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Timeout after %lu ms while waiting"
                    " for SDO 0x%04x:%x download response.\n", diff_ms,
                    fsm->request->index, fsm->request->subindex);
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_down_response;
}

/****************************************************************************/

/** Prepare a download segment request.
 */
void ec_fsm_coe_down_prepare_segment_request(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_sdo_request_t *request = fsm->request;
    size_t max_segment_size =
        slave->configured_rx_mailbox_size
        - EC_MBOX_HEADER_SIZE
        - EC_COE_DOWN_SEG_REQ_HEADER_SIZE;
    size_t data_size;
    uint8_t last_segment, seg_data_size, *data;

    if (fsm->remaining > max_segment_size) {
        fsm->segment_size = max_segment_size;
        last_segment = 0;
    } else {
        fsm->segment_size = fsm->remaining;
        last_segment = 1;
    }

    if (fsm->segment_size > EC_COE_DOWN_SEG_MIN_DATA_SIZE) {
        seg_data_size = 0x00;
        data_size = EC_COE_DOWN_SEG_REQ_HEADER_SIZE + fsm->segment_size;
    } else {
        seg_data_size = EC_COE_DOWN_SEG_MIN_DATA_SIZE - fsm->segment_size;
        data_size = EC_COE_DOWN_SEG_REQ_HEADER_SIZE
            + EC_COE_DOWN_SEG_MIN_DATA_SIZE;
    }

    data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_COE,
            data_size);
    if (IS_ERR(data)) {
        request->errno = PTR_ERR(data);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    EC_WRITE_U16(data, 0x2 << 12); // SDO request
    EC_WRITE_U8(data + 2, (last_segment ? 1 : 0)
            | (seg_data_size << 1)
            | (fsm->toggle << 4)
            | (0x00 << 5)); // Download segment request
    memcpy(data + EC_COE_DOWN_SEG_REQ_HEADER_SIZE,
            request->data + fsm->offset, fsm->segment_size);
    if (fsm->segment_size < EC_COE_DOWN_SEG_MIN_DATA_SIZE) {
        memset(data + EC_COE_DOWN_SEG_REQ_HEADER_SIZE + fsm->segment_size,
                0x00, EC_COE_DOWN_SEG_MIN_DATA_SIZE - fsm->segment_size);
    }

    if (slave->master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Download segment request:\n");
        ec_print_data(data, data_size);
    }

    fsm->state = ec_fsm_coe_down_seg_check;
}

/****************************************************************************/

/**
   CoE state: DOWN RESPONSE.
   \todo Timeout behavior
*/

void ec_fsm_coe_down_response(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    uint8_t *data, mbox_prot;
    size_t rec_size;
    ec_sdo_request_t *request = fsm->request;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE download"
                " response datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE download response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        request->errno = PTR_ERR(data);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (mbox_prot != EC_MBOX_TYPE_COE) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Received mailbox protocol 0x%02X as response.\n",
                mbox_prot);
        return;
    }

    if (ec_fsm_coe_check_emergency(fsm, data, rec_size)) {
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_down_check;
        return;
    }

    if (slave->master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Download response:\n");
        ec_print_data(data, rec_size);
    }

    if (rec_size < 6) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Received data are too small (%zu bytes):\n",
                rec_size);
        ec_print_data(data, rec_size);
        return;
    }

    if (EC_READ_U16(data) >> 12 == 0x2 && // SDO request
        EC_READ_U8 (data + 2) >> 5 == 0x4) { // abort SDO transfer request
        char subidxstr[10];
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        if (request->complete_access) {
            subidxstr[0] = 0x00;
        } else {
            sprintf(subidxstr, ":%02X", request->subindex);
        }
        EC_SLAVE_ERR(slave, "SDO download 0x%04X%s (%zu bytes) aborted.\n",
                request->index, subidxstr, request->data_size);
        if (rec_size < 10) {
            EC_SLAVE_ERR(slave, "Incomplete abort command:\n");
            ec_print_data(data, rec_size);
        } else {
            fsm->request->abort_code = EC_READ_U32(data + 6);
            ec_canopen_abort_msg(slave, fsm->request->abort_code);
        }
        return;
    }

    if (EC_READ_U16(data) >> 12 != 0x3 || // SDO response
        EC_READ_U8 (data + 2) >> 5 != 0x3 || // Download response
        EC_READ_U16(data + 3) != request->index || // index
        EC_READ_U8 (data + 5) != request->subindex) { // subindex
        if (slave->master->debug_level) {
            EC_SLAVE_DBG(slave, 1, "Invalid SDO download response!"
                    " Retrying...\n");
            ec_print_data(data, rec_size);
        }
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_down_check;
        return;
    }

    if (fsm->remaining) { // more segments to download
        fsm->toggle = 0;
        ec_fsm_coe_down_prepare_segment_request(fsm, datagram);
    } else {
        fsm->state = ec_fsm_coe_end; // success
    }
}

/****************************************************************************/

/**
   CoE state: DOWN SEG CHECK.
*/

void ec_fsm_coe_down_seg_check(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE mailbox check datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE mailbox segment check"
                " datagram failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= fsm->request->response_timeout) {
            fsm->request->errno = EIO;
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Timeout while waiting for SDO download"
                    " segment response.\n");
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_down_seg_response;
}

/****************************************************************************/

/**
   CoE state: DOWN SEG RESPONSE.
   \todo Timeout behavior
*/

void ec_fsm_coe_down_seg_response(
        ec_fsm_coe_t *fsm, /**< Finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    uint8_t *data, mbox_prot;
    size_t rec_size;
    ec_sdo_request_t *request = fsm->request;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE download response"
                " datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE download response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        request->errno = PTR_ERR(data);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (mbox_prot != EC_MBOX_TYPE_COE) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Received mailbox protocol 0x%02X as response.\n",
                mbox_prot);
        return;
    }

    if (ec_fsm_coe_check_emergency(fsm, data, rec_size)) {
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_down_check;
        return;
    }

    if (slave->master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Download response:\n");
        ec_print_data(data, rec_size);
    }

    if (rec_size < 6) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Received data are too small (%zu bytes):\n",
                rec_size);
        ec_print_data(data, rec_size);
        return;
    }

    if (EC_READ_U16(data) >> 12 == 0x2 && // SDO request
        EC_READ_U8 (data + 2) >> 5 == 0x4) { // abort SDO transfer request
        char subidxstr[10];
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        if (request->complete_access) {
            subidxstr[0] = 0x00;
        } else {
            sprintf(subidxstr, ":%02X", request->subindex);
        }
        EC_SLAVE_ERR(slave, "SDO download 0x%04X%s (%zu bytes) aborted.\n",
                request->index, subidxstr, request->data_size);
        if (rec_size < 10) {
            EC_SLAVE_ERR(slave, "Incomplete abort command:\n");
            ec_print_data(data, rec_size);
        } else {
            fsm->request->abort_code = EC_READ_U32(data + 6);
            ec_canopen_abort_msg(slave, fsm->request->abort_code);
        }
        return;
    }

    if (EC_READ_U16(data) >> 12 != 0x3 ||
            ((EC_READ_U8(data + 2) >> 5) != 0x01)) { // segment response
        if (slave->master->debug_level) {
            EC_SLAVE_DBG(slave, 1, "Invalid SDO download response!"
                    " Retrying...\n");
            ec_print_data(data, rec_size);
        }
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_down_seg_check;
        return;
    }

    if (((EC_READ_U8(data + 2) >> 4) & 0x01) != fsm->toggle) {
        EC_SLAVE_ERR(slave, "Invalid toggle received during"
                " segmented download:\n");
        ec_print_data(data, rec_size);
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    fsm->offset += fsm->segment_size;
    fsm->remaining -= fsm->segment_size;

    if (fsm->remaining) { // more segments to download
        fsm->toggle = !fsm->toggle;
        ec_fsm_coe_down_prepare_segment_request(fsm, datagram);
    } else {
        fsm->state = ec_fsm_coe_end; // success
    }
}

/****************************************************************************/

/** Prepare an upload request.
 *
 * \return Zero on success, otherwise a negative error code.
 */
int ec_fsm_coe_prepare_up(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_sdo_request_t *request = fsm->request;
    ec_master_t *master = slave->master;

    u8 *data = ec_slave_mbox_prepare_send(slave, datagram, EC_MBOX_TYPE_COE,
			10);
    if (IS_ERR(data)) {
        request->errno = PTR_ERR(data);
        return PTR_ERR(data);
    }

    EC_WRITE_U16(data, 0x2 << 12); // SDO request
    EC_WRITE_U8 (data + 2, 0x2 << 5); // initiate upload request
    EC_WRITE_U16(data + 3, request->index);
    EC_WRITE_U8 (data + 5, request->subindex);
    memset(data + 6, 0x00, 4);

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Upload request:\n");
        ec_print_data(data, 10);
    }

    fsm->state = ec_fsm_coe_up_request;
    return 0;
}

/****************************************************************************/

/**
   CoE state: UP START.
*/

void ec_fsm_coe_up_start(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_sdo_request_t *request = fsm->request;

    EC_SLAVE_DBG(slave, 1, "Uploading SDO 0x%04X:%02X.\n",
            request->index, request->subindex);

    if (!(slave->sii.mailbox_protocols & EC_MBOX_COE)) {
        EC_SLAVE_ERR(slave, "Slave does not support CoE!\n");
        request->errno = EPROTONOSUPPORT;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    fsm->retries = EC_FSM_RETRIES;
    fsm->request->jiffies_sent = jiffies;

    if (ec_fsm_coe_prepare_up(fsm, datagram)) {
        fsm->state = ec_fsm_coe_error;
    }
}

/****************************************************************************/
/**
   CoE state: UP REQUEST.
   \todo Timeout behavior
*/

void ec_fsm_coe_up_request(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    unsigned long diff_ms;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        if (ec_fsm_coe_prepare_up(fsm, datagram)) {
            fsm->state = ec_fsm_coe_error;
        }
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE upload request: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    diff_ms = (jiffies - fsm->request->jiffies_sent) * 1000 / HZ;

    if (fsm->datagram->working_counter != 1) {
        if (!fsm->datagram->working_counter) {
            if (diff_ms < fsm->request->response_timeout) {
#if DEBUG_RETRIES
                EC_SLAVE_DBG(slave, 1, "Slave did not respond to"
                        " SDO upload request. Retrying after %lu ms...\n",
                        diff_ms);
#endif
                // no response; send request datagram again
                if (ec_fsm_coe_prepare_up(fsm, datagram)) {
                    fsm->state = ec_fsm_coe_error;
                }
                return;
            }
        }
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE upload request for"
                " SDO 0x%04x:%x failed with timeout after %lu ms: ",
                fsm->request->index, fsm->request->subindex, diff_ms);
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

#if DEBUG_LONG
    if (diff_ms > 200) {
        EC_SLAVE_WARN(slave, "SDO 0x%04x:%x upload took %lu ms.\n",
                fsm->request->index, fsm->request->subindex, diff_ms);
    }
#endif

    fsm->jiffies_start = fsm->datagram->jiffies_sent;

    ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_up_check;
}

/****************************************************************************/

/**
   CoE state: UP CHECK.
*/

void ec_fsm_coe_up_check(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE mailbox check datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE mailbox check"
                " datagram failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= fsm->request->response_timeout) {
            fsm->request->errno = EIO;
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Timeout after %lu ms while waiting for"
                    " SDO 0x%04x:%x upload response.\n", diff_ms,
                    fsm->request->index, fsm->request->subindex);
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_up_response;
}

/****************************************************************************/

/** Prepare an SDO upload segment request.
 */
void ec_fsm_coe_up_prepare_segment_request(
        ec_fsm_coe_t *fsm, /**< Finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    uint8_t *data =
        ec_slave_mbox_prepare_send(fsm->slave, datagram, EC_MBOX_TYPE_COE,
				10);
    if (IS_ERR(data)) {
        fsm->request->errno = PTR_ERR(data);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    EC_WRITE_U16(data, 0x2 << 12); // SDO request
    EC_WRITE_U8 (data + 2, (fsm->toggle << 4 // toggle
                | 0x3 << 5)); // upload segment request
    memset(data + 3, 0x00, 7);

    if (fsm->slave->master->debug_level) {
        EC_SLAVE_DBG(fsm->slave, 1, "Upload segment request:\n");
        ec_print_data(data, 10);
    }
}

/****************************************************************************/

/**
   CoE state: UP RESPONSE.
   \todo Timeout behavior
*/

void ec_fsm_coe_up_response(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    uint16_t rec_index;
    uint8_t *data, mbox_prot, rec_subindex;
    size_t rec_size, data_size;
    ec_sdo_request_t *request = fsm->request;
    unsigned int expedited, size_specified;
    int ret;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE upload response"
                " datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE upload response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        request->errno = PTR_ERR(data);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Upload response:\n");
        ec_print_data(data, rec_size);
    }

    if (mbox_prot != EC_MBOX_TYPE_COE) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_WARN(slave, "Received mailbox protocol 0x%02X"
                " as response.\n", mbox_prot);
        return;
    }

    if (ec_fsm_coe_check_emergency(fsm, data, rec_size)) {
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_up_check;
        return;
    }

    if (rec_size < 6) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Received currupted SDO upload response"
                " (%zu bytes)!\n", rec_size);
        ec_print_data(data, rec_size);
        return;
    }

    if (EC_READ_U16(data) >> 12 == 0x2 && // SDO request
            EC_READ_U8(data + 2) >> 5 == 0x4) { // abort SDO transfer request
        EC_SLAVE_ERR(slave, "SDO upload 0x%04X:%02X aborted.\n",
               request->index, request->subindex);
        if (rec_size >= 10) {
            request->abort_code = EC_READ_U32(data + 6);
            ec_canopen_abort_msg(slave, request->abort_code);
        } else {
            EC_SLAVE_ERR(slave, "No abort message.\n");
        }
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (EC_READ_U16(data) >> 12 != 0x3 || // SDO response
            EC_READ_U8(data + 2) >> 5 != 0x2) { // upload response
        EC_SLAVE_ERR(slave, "Received unknown response while"
                " uploading SDO 0x%04X:%02X.\n",
                request->index, request->subindex);
        ec_print_data(data, rec_size);
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    rec_index = EC_READ_U16(data + 3);
    rec_subindex = EC_READ_U8(data + 5);

    if (rec_index != request->index || rec_subindex != request->subindex) {
        EC_SLAVE_ERR(slave, "Received upload response for wrong SDO"
                " (0x%04X:%02X, requested: 0x%04X:%02X).\n",
                rec_index, rec_subindex, request->index, request->subindex);
        ec_print_data(data, rec_size);

        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_up_check;
        return;
    }

    // normal or expedited?
    expedited = EC_READ_U8(data + 2) & 0x02;

    if (expedited) {
        size_specified = EC_READ_U8(data + 2) & 0x01;
        if (size_specified) {
            fsm->complete_size = 4 - ((EC_READ_U8(data + 2) & 0x0C) >> 2);
        } else {
            fsm->complete_size = 4;
        }

        if (rec_size < 6 + fsm->complete_size) {
            request->errno = EIO;
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Received corrupted SDO expedited upload"
                    " response (only %zu bytes)!\n", rec_size);
            ec_print_data(data, rec_size);
            return;
        }

        ret = ec_sdo_request_copy_data(request, data + 6, fsm->complete_size);
        if (ret) {
            request->errno = -ret;
            fsm->state = ec_fsm_coe_error;
            return;
        }
    } else { // normal
        if (rec_size < 10) {
            request->errno = EIO;
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Received currupted SDO normal upload"
                    " response (only %zu bytes)!\n", rec_size);
            ec_print_data(data, rec_size);
            return;
        }

        data_size = rec_size - 10;
        fsm->complete_size = EC_READ_U32(data + 6);

        ret = ec_sdo_request_alloc(request, fsm->complete_size);
        if (ret) {
            request->errno = -ret;
            fsm->state = ec_fsm_coe_error;
            return;
        }

        ret = ec_sdo_request_copy_data(request, data + 10, data_size);
        if (ret) {
            request->errno = -ret;
            fsm->state = ec_fsm_coe_error;
            return;
        }

        fsm->toggle = 0;

        if (data_size < fsm->complete_size) {
            EC_SLAVE_DBG(slave, 1, "SDO data incomplete (%zu / %u)."
                    " Segmenting...\n", data_size, fsm->complete_size);
            ec_fsm_coe_up_prepare_segment_request(fsm, datagram);
            fsm->retries = EC_FSM_RETRIES;
            fsm->state = ec_fsm_coe_up_seg_request;
            return;
        }
    }

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Uploaded data:\n");
        ec_print_data(request->data, request->data_size);
    }

    fsm->state = ec_fsm_coe_end; // success
}

/****************************************************************************/

/**
   CoE state: UP REQUEST.
   \todo Timeout behavior
*/

void ec_fsm_coe_up_seg_request(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_fsm_coe_up_prepare_segment_request(fsm, datagram);
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE upload segment"
                " request datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE upload segment"
                " request failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    fsm->jiffies_start = fsm->datagram->jiffies_sent;

    ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_up_seg_check;
}

/****************************************************************************/

/**
   CoE state: UP CHECK.
*/

void ec_fsm_coe_up_seg_check(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE mailbox check"
                " datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        fsm->request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE mailbox check datagram"
                " failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    if (!ec_slave_mbox_check(fsm->datagram)) {
        unsigned long diff_ms =
            (fsm->datagram->jiffies_received - fsm->jiffies_start) *
            1000 / HZ;
        if (diff_ms >= fsm->request->response_timeout) {
            fsm->request->errno = EIO;
            fsm->state = ec_fsm_coe_error;
            EC_SLAVE_ERR(slave, "Timeout while waiting for SDO upload"
                    " segment response.\n");
            return;
        }

        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // Fetch response
    ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_coe_up_seg_response;
}

/****************************************************************************/

/**
   CoE state: UP RESPONSE.
   \todo Timeout behavior
*/

void ec_fsm_coe_up_seg_response(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    uint8_t *data, mbox_prot;
    size_t rec_size, data_size;
    ec_sdo_request_t *request = fsm->request;
    unsigned int last_segment;

    if (fsm->datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        ec_slave_mbox_prepare_fetch(slave, datagram); // can not fail.
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Failed to receive CoE upload segment"
                " response datagram: ");
        ec_datagram_print_state(fsm->datagram);
        return;
    }

    if (fsm->datagram->working_counter != 1) {
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        EC_SLAVE_ERR(slave, "Reception of CoE upload segment"
                " response failed: ");
        ec_datagram_print_wc_error(fsm->datagram);
        return;
    }

    data = ec_slave_mbox_fetch(slave, fsm->datagram, &mbox_prot, &rec_size);
    if (IS_ERR(data)) {
        request->errno = PTR_ERR(data);
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Upload segment response:\n");
        ec_print_data(data, rec_size);
    }

    if (mbox_prot != EC_MBOX_TYPE_COE) {
        EC_SLAVE_ERR(slave, "Received mailbox protocol 0x%02X as response.\n",
                mbox_prot);
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (ec_fsm_coe_check_emergency(fsm, data, rec_size)) {
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_up_seg_check;
        return;
    }

    if (rec_size < 10) {
        EC_SLAVE_ERR(slave, "Received currupted SDO upload"
                " segment response!\n");
        ec_print_data(data, rec_size);
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (EC_READ_U16(data) >> 12 == 0x2 && // SDO request
            EC_READ_U8 (data + 2) >> 5 == 0x4) { // abort SDO transfer request
        EC_SLAVE_ERR(slave, "SDO upload 0x%04X:%02X aborted.\n",
               request->index, request->subindex);
        request->abort_code = EC_READ_U32(data + 6);
        ec_canopen_abort_msg(slave, request->abort_code);
        request->errno = EIO;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    if (EC_READ_U16(data) >> 12 != 0x3 || // SDO response
        EC_READ_U8 (data + 2) >> 5 != 0x0) { // upload segment response
        if (fsm->slave->master->debug_level) {
            EC_SLAVE_DBG(slave, 1, "Invalid SDO upload segment response!\n");
            ec_print_data(data, rec_size);
        }
        // check for CoE response again
        ec_slave_mbox_prepare_check(slave, datagram); // can not fail.
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_up_seg_check;
        return;
    }

    data_size = rec_size - 3; /* Header of segment upload is smaller than
                                 normal upload */
    if (rec_size == 10) {
        uint8_t seg_size = (EC_READ_U8(data + 2) & 0xE) >> 1;
        data_size -= seg_size;
    }

    if (request->data_size + data_size > fsm->complete_size) {
        EC_SLAVE_ERR(slave, "SDO upload 0x%04X:%02X failed: Fragment"
                " exceeding complete size!\n",
                request->index, request->subindex);
        request->errno = ENOBUFS;
        fsm->state = ec_fsm_coe_error;
        return;
    }

    memcpy(request->data + request->data_size, data + 3, data_size);
    request->data_size += data_size;

    last_segment = EC_READ_U8(data + 2) & 0x01;
    if (!last_segment) {
        fsm->toggle = !fsm->toggle;
        ec_fsm_coe_up_prepare_segment_request(fsm, datagram);
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_coe_up_seg_request;
        return;
    }

    if (request->data_size != fsm->complete_size) {
        EC_SLAVE_WARN(slave, "SDO upload 0x%04X:%02X: Assembled data"
                " size (%zu) does not match complete size (%u)!\n",
                request->index, request->subindex,
                request->data_size, fsm->complete_size);
    }

    if (master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Uploaded data:\n");
        ec_print_data(request->data, request->data_size);
    }

    fsm->state = ec_fsm_coe_end; // success
}

/****************************************************************************/

/**
   State: ERROR.
*/

void ec_fsm_coe_error(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

/**
   State: END.
*/

void ec_fsm_coe_end(
        ec_fsm_coe_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

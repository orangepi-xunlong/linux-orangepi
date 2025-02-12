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
   EtherCAT slave information interface FSM.
*/

/****************************************************************************/

#include "globals.h"
#include "mailbox.h"
#include "master.h"
#include "fsm_sii.h"

/** Read/write timeout [ms].
 *
 * Used to calculate timeouts bsed on the jiffies counter.
 *
 * \attention Must be more than 10 to avoid problems on kernels that run with
 * a timer interupt frequency of 100 Hz.
 */
#define SII_TIMEOUT 20

/** Time before evaluating answer at writing [ms].
 */
#define SII_INHIBIT 5

//#define SII_DEBUG

/****************************************************************************/

void ec_fsm_sii_state_start_reading(ec_fsm_sii_t *);
void ec_fsm_sii_state_read_check(ec_fsm_sii_t *);
void ec_fsm_sii_state_read_fetch(ec_fsm_sii_t *);
void ec_fsm_sii_state_start_writing(ec_fsm_sii_t *);
void ec_fsm_sii_state_write_check(ec_fsm_sii_t *);
void ec_fsm_sii_state_write_check2(ec_fsm_sii_t *);
void ec_fsm_sii_state_end(ec_fsm_sii_t *);
void ec_fsm_sii_state_error(ec_fsm_sii_t *);

/****************************************************************************/

/**
   Constructor.
*/

void ec_fsm_sii_init(ec_fsm_sii_t *fsm, /**< finite state machine */
                     ec_datagram_t *datagram /**< datagram structure to use */
                     )
{
    fsm->state = NULL;
    fsm->datagram = datagram;
}

/****************************************************************************/

/**
   Destructor.
*/

void ec_fsm_sii_clear(ec_fsm_sii_t *fsm /**< finite state machine */)
{
}

/****************************************************************************/

/**
   Initializes the SII read state machine.
*/

void ec_fsm_sii_read(ec_fsm_sii_t *fsm, /**< finite state machine */
                     ec_slave_t *slave, /**< slave to read from */
                     uint16_t word_offset, /**< offset to read from */
                     ec_fsm_sii_addressing_t mode /**< addressing scheme */
                     )
{
    fsm->state = ec_fsm_sii_state_start_reading;
    fsm->slave = slave;
    fsm->word_offset = word_offset;
    fsm->mode = mode;
}

/****************************************************************************/

/**
   Initializes the SII write state machine.
*/

void ec_fsm_sii_write(ec_fsm_sii_t *fsm, /**< finite state machine */
                      ec_slave_t *slave, /**< slave to read from */
                      uint16_t word_offset, /**< offset to read from */
                      const uint16_t *value, /**< pointer to 2 bytes of data */
                      ec_fsm_sii_addressing_t mode /**< addressing scheme */
                      )
{
    fsm->state = ec_fsm_sii_state_start_writing;
    fsm->slave = slave;
    fsm->word_offset = word_offset;
    fsm->mode = mode;
    memcpy(fsm->value, value, 2);
}

/****************************************************************************/

/**
   Executes the SII state machine.
   \return false, if the state machine has terminated
*/

int ec_fsm_sii_exec(ec_fsm_sii_t *fsm /**< finite state machine */)
{
    fsm->state(fsm);

    return fsm->state != ec_fsm_sii_state_end
        && fsm->state != ec_fsm_sii_state_error;
}

/****************************************************************************/

/**
   Returns, if the master startup state machine terminated with success.
   \return non-zero if successful.
*/

int ec_fsm_sii_success(ec_fsm_sii_t *fsm /**< Finite state machine */)
{
    return fsm->state == ec_fsm_sii_state_end;
}

/*****************************************************************************
 * state functions
 ****************************************************************************/

/**
   SII state: START READING.
   Starts reading the slave information interface.
*/

void ec_fsm_sii_state_start_reading(
        ec_fsm_sii_t *fsm /**< finite state machine */
        )
{
    ec_datagram_t *datagram = fsm->datagram;

    // initiate read operation
    switch (fsm->mode) {
        case EC_FSM_SII_USE_INCREMENT_ADDRESS:
            ec_datagram_apwr(datagram, fsm->slave->ring_position, 0x502, 4);
            break;
        case EC_FSM_SII_USE_CONFIGURED_ADDRESS:
            ec_datagram_fpwr(datagram, fsm->slave->station_address, 0x502, 4);
            break;
    }

    EC_WRITE_U8 (datagram->data,     0x80); // two address octets
    EC_WRITE_U8 (datagram->data + 1, 0x01); // request read operation
    EC_WRITE_U16(datagram->data + 2, fsm->word_offset);

#ifdef SII_DEBUG
    EC_SLAVE_DBG(fsm->slave, 0, "reading SII data, word %u:\n",
            fsm->word_offset);
    ec_print_data(datagram->data, 4);
#endif

    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_sii_state_read_check;
}

/****************************************************************************/

/**
   SII state: READ CHECK.
   Checks, if the SII-read-datagram has been sent and issues a fetch datagram.
*/

void ec_fsm_sii_state_read_check(
        ec_fsm_sii_t *fsm /**< finite state machine */
        )
{
    ec_datagram_t *datagram = fsm->datagram;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_sii_state_error;
        EC_SLAVE_ERR(fsm->slave, "Failed to receive SII read datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        fsm->state = ec_fsm_sii_state_error;
        EC_SLAVE_ERR(fsm->slave, "Reception of SII read datagram failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    fsm->jiffies_start = datagram->jiffies_sent;
    fsm->check_once_more = 1;

    // issue check/fetch datagram
    switch (fsm->mode) {
        case EC_FSM_SII_USE_INCREMENT_ADDRESS:
            ec_datagram_aprd(datagram, fsm->slave->ring_position, 0x502, 10);
            break;
        case EC_FSM_SII_USE_CONFIGURED_ADDRESS:
            ec_datagram_fprd(datagram, fsm->slave->station_address, 0x502, 10);
            break;
    }

    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_sii_state_read_fetch;
}

/****************************************************************************/

/**
   SII state: READ FETCH.
   Fetches the result of an SII-read datagram.
*/
void ec_fsm_sii_state_read_fetch(
        ec_fsm_sii_t *fsm /**< finite state machine */
        )
{
    ec_datagram_t *datagram = fsm->datagram;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_sii_state_error;
        EC_SLAVE_ERR(fsm->slave,
                "Failed to receive SII check/fetch datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        fsm->state = ec_fsm_sii_state_error;
        EC_SLAVE_ERR(fsm->slave,
                "Reception of SII check/fetch datagram failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

#ifdef SII_DEBUG
    EC_SLAVE_DBG(fsm->slave, 0, "checking SII read state:\n");
    ec_print_data(datagram->data, 10);
#endif

    if (EC_READ_U8(datagram->data + 1) & 0x20) {
        EC_SLAVE_ERR(fsm->slave, "Error on last command while"
                " reading from SII word 0x%04x.\n", fsm->word_offset);
        fsm->state = ec_fsm_sii_state_error;
        return;
    }

    // check "busy bit"
    if (EC_READ_U8(datagram->data + 1) & 0x81) { /* busy bit or
                                                    read operation busy */
        // still busy... timeout?
        unsigned long diff_ms =
            (datagram->jiffies_received - fsm->jiffies_start) * 1000 / HZ;
        if (diff_ms >= SII_TIMEOUT) {
            if (fsm->check_once_more) {
                fsm->check_once_more = 0;
            } else {
                EC_SLAVE_ERR(fsm->slave, "SII: Read timeout.\n");
                fsm->state = ec_fsm_sii_state_error;
                return;
            }
        }

        // issue check/fetch datagram again
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    // SII value received.
    memcpy(fsm->value, datagram->data + 6, 4);
    fsm->state = ec_fsm_sii_state_end;
}

/****************************************************************************/

/**
   SII state: START WRITING.
   Starts writing a word through the slave information interface.
*/

void ec_fsm_sii_state_start_writing(
        ec_fsm_sii_t *fsm /**< finite state machine */
        )
{
    ec_datagram_t *datagram = fsm->datagram;

    // initiate write operation
    ec_datagram_fpwr(datagram, fsm->slave->station_address, 0x502, 8);
    EC_WRITE_U8 (datagram->data,     0x81); /* two address octets
                                               + enable write access */
    EC_WRITE_U8 (datagram->data + 1, 0x02); // request write operation
    EC_WRITE_U16(datagram->data + 2, fsm->word_offset);
    memset(datagram->data + 4, 0x00, 2);
    memcpy(datagram->data + 6, fsm->value, 2);

#ifdef SII_DEBUG
    EC_SLAVE_DBG(fsm->slave, 0, "writing SII data:\n");
    ec_print_data(datagram->data, 8);
#endif

    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_sii_state_write_check;
}

/****************************************************************************/

/**
   SII state: WRITE CHECK.
*/

void ec_fsm_sii_state_write_check(
        ec_fsm_sii_t *fsm /**< finite state machine */
        )
{
    ec_datagram_t *datagram = fsm->datagram;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_sii_state_error;
        EC_SLAVE_ERR(fsm->slave, "Failed to receive SII write datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        fsm->state = ec_fsm_sii_state_error;
        EC_SLAVE_ERR(fsm->slave, "Reception of SII write datagram failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    fsm->jiffies_start = datagram->jiffies_sent;
    fsm->check_once_more = 1;

    // issue check datagram
    ec_datagram_fprd(datagram, fsm->slave->station_address, 0x502, 2);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_sii_state_write_check2;
}

/****************************************************************************/

/**
   SII state: WRITE CHECK 2.
*/

void ec_fsm_sii_state_write_check2(
        ec_fsm_sii_t *fsm /**< finite state machine */
        )
{
    ec_datagram_t *datagram = fsm->datagram;
    unsigned long diff_ms;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_sii_state_error;
        EC_SLAVE_ERR(fsm->slave,
                "Failed to receive SII write check datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        fsm->state = ec_fsm_sii_state_error;
        EC_SLAVE_ERR(fsm->slave,
                "Reception of SII write check datagram failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

#ifdef SII_DEBUG
    EC_SLAVE_DBG(fsm->slave, 0, "checking SII write state:\n");
    ec_print_data(datagram->data, 2);
#endif

    if (EC_READ_U8(datagram->data + 1) & 0x20) {
        EC_SLAVE_ERR(fsm->slave, "SII: Error on last SII command!\n");
        fsm->state = ec_fsm_sii_state_error;
        return;
    }

    /* FIXME: some slaves never answer with the busy flag set...
     * wait a few ms for the write operation to complete. */
    diff_ms = (datagram->jiffies_received - fsm->jiffies_start) * 1000 / HZ;
    if (diff_ms < SII_INHIBIT) {
#ifdef SII_DEBUG
        EC_SLAVE_DBG(fsm->slave, 0, "too early.\n");
#endif
        // issue check datagram again
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    if (EC_READ_U8(datagram->data + 1) & 0x82) { /* busy bit or
                                                    write operation busy bit */
        // still busy... timeout?
        if (diff_ms >= SII_TIMEOUT) {
            if (fsm->check_once_more) {
                fsm->check_once_more = 0;
            } else {
                EC_SLAVE_ERR(fsm->slave, "SII: Write timeout.\n");
                fsm->state = ec_fsm_sii_state_error;
                return;
            }
        }

        // issue check datagram again
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    if (EC_READ_U8(datagram->data + 1) & 0x40) {
        EC_SLAVE_ERR(fsm->slave, "SII: Write operation failed!\n");
        fsm->state = ec_fsm_sii_state_error;
        return;
    }

    // success
    fsm->state = ec_fsm_sii_state_end;
}

/****************************************************************************/

/**
   State: ERROR.
*/

void ec_fsm_sii_state_error(
        ec_fsm_sii_t *fsm /**< finite state machine */
        )
{
}

/****************************************************************************/

/**
   State: END.
*/

void ec_fsm_sii_state_end(
        ec_fsm_sii_t *fsm /**< finite state machine */
        )
{
}

/****************************************************************************/

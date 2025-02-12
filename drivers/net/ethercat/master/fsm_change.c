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
   EtherCAT state change FSM.
*/

/****************************************************************************/

#include "globals.h"
#include "master.h"
#include "fsm_change.h"
#include "slave_config.h"

/****************************************************************************/

unsigned int ec_fsm_change_timeout_ms(const ec_fsm_change_t *);

void ec_fsm_change_state_start(ec_fsm_change_t *);
void ec_fsm_change_state_check(ec_fsm_change_t *);
void ec_fsm_change_state_status(ec_fsm_change_t *);
void ec_fsm_change_state_start_code(ec_fsm_change_t *);
void ec_fsm_change_state_code(ec_fsm_change_t *);
void ec_fsm_change_state_ack(ec_fsm_change_t *);
void ec_fsm_change_state_check_ack(ec_fsm_change_t *);
void ec_fsm_change_state_end(ec_fsm_change_t *);
void ec_fsm_change_state_error(ec_fsm_change_t *);

/****************************************************************************/

/**
   Constructor.
*/

void ec_fsm_change_init(ec_fsm_change_t *fsm, /**< finite state machine */
                        ec_datagram_t *datagram /**< datagram */
                        )
{
    fsm->state = NULL;
    fsm->datagram = datagram;
    fsm->spontaneous_change = 0;
}

/****************************************************************************/

/**
   Destructor.
*/

void ec_fsm_change_clear(ec_fsm_change_t *fsm /**< finite state machine */)
{
}

/****************************************************************************/

/** Get timeout in ms.
 *
 * For defaults see ETG2000_S_R_V1i0i15 section 5.3.6.2.
 */
unsigned int ec_fsm_change_timeout_ms(
        const ec_fsm_change_t *fsm /**< finite state machine */
        )
{
    ec_slave_state_t from = fsm->old_state;
    ec_slave_state_t to = fsm->requested_state;

    /* Search for specified timeout in slave configuration */
    if (fsm->slave->config) {
        unsigned int timeout_ms =
            ec_slave_config_al_timeout(fsm->slave->config, from, to);
        if (timeout_ms) {
            return timeout_ms;
        }
    }

    /* No specific timeout found. Use defaults from spec. */

    if (from == EC_SLAVE_STATE_INIT &&
            (to == EC_SLAVE_STATE_PREOP || to == EC_SLAVE_STATE_BOOT)) {
        return 3000; // PreopTimeout
    }
    if ((from == EC_SLAVE_STATE_PREOP && to == EC_SLAVE_STATE_SAFEOP) ||
            (from == EC_SLAVE_STATE_SAFEOP && to == EC_SLAVE_STATE_OP)) {
        return 10000; // SafeopOpTimeout
    }
    if (to == EC_SLAVE_STATE_INIT ||
            ((from == EC_SLAVE_STATE_OP || from == EC_SLAVE_STATE_SAFEOP)
             && to == EC_SLAVE_STATE_PREOP)) {
        return 5000; // BackToInitTimeout
    }
    if (from == EC_SLAVE_STATE_OP && to == EC_SLAVE_STATE_SAFEOP) {
        return 200; // BackToSafeopTimeout
    }

    return 10000; // default [ms]
}

/****************************************************************************/

/**
   Starts the change state machine.
*/

void ec_fsm_change_start(ec_fsm_change_t *fsm, /**< finite state machine */
                         ec_slave_t *slave, /**< EtherCAT slave */
                         ec_slave_state_t state /**< requested state */
                         )
{
    fsm->mode = EC_FSM_CHANGE_MODE_FULL;
    fsm->slave = slave;
    fsm->requested_state = state;
    fsm->state = ec_fsm_change_state_start;
}

/****************************************************************************/

/**
   Starts the change state machine to only acknowlegde a slave's state.
*/

void ec_fsm_change_ack(ec_fsm_change_t *fsm, /**< finite state machine */
                       ec_slave_t *slave /**< EtherCAT slave */
                       )
{
    fsm->mode = EC_FSM_CHANGE_MODE_ACK_ONLY;
    fsm->slave = slave;
    fsm->requested_state = EC_SLAVE_STATE_UNKNOWN;
    fsm->state = ec_fsm_change_state_start_code;
}

/****************************************************************************/

/**
   Executes the current state of the state machine.
   \return false, if the state machine has terminated
*/

int ec_fsm_change_exec(ec_fsm_change_t *fsm /**< finite state machine */)
{
    fsm->state(fsm);

    return fsm->state != ec_fsm_change_state_end
        && fsm->state != ec_fsm_change_state_error;
}

/****************************************************************************/

/**
   Returns, if the state machine terminated with success.
   \return non-zero if successful.
*/

int ec_fsm_change_success(ec_fsm_change_t *fsm /**< Finite state machine */)
{
    return fsm->state == ec_fsm_change_state_end;
}

/*****************************************************************************
 *  state change state machine
 ****************************************************************************/

/**
   Change state: START.
*/

void ec_fsm_change_state_start(ec_fsm_change_t *fsm
                               /**< finite state machine */)
{
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    fsm->take_time = 1;
    fsm->old_state = fsm->slave->current_state;

    // write new state to slave
    ec_datagram_fpwr(datagram, slave->station_address, 0x0120, 2);
    EC_WRITE_U16(datagram->data, fsm->requested_state);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_change_state_check;
}

/****************************************************************************/

/**
   Change state: CHECK.
*/

void ec_fsm_change_state_check(ec_fsm_change_t *fsm
                               /**< finite state machine */)
{
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive state datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (fsm->take_time) {
        fsm->take_time = 0;
        fsm->jiffies_start = datagram->jiffies_sent;
    }

    if (datagram->working_counter == 0) {
        if (datagram->jiffies_received - fsm->jiffies_start >= 3 * HZ) {
            char state_str[EC_STATE_STRING_SIZE];
            ec_state_string(fsm->requested_state, state_str, 0);
            fsm->state = ec_fsm_change_state_error;
            EC_SLAVE_ERR(slave, "Failed to set state %s: ", state_str);
            ec_datagram_print_wc_error(datagram);
            return;
        }

        // repeat writing new state to slave
        ec_datagram_fpwr(datagram, slave->station_address, 0x0120, 2);
        EC_WRITE_U16(datagram->data, fsm->requested_state);
        fsm->retries = EC_FSM_RETRIES;
        return;
    }

    if (unlikely(datagram->working_counter > 1)) {
        char state_str[EC_STATE_STRING_SIZE];
        ec_state_string(fsm->requested_state, state_str, 0);
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Failed to set state %s: ", state_str);
        ec_datagram_print_wc_error(datagram);
        return;
    }

    fsm->take_time = 1;

    // read AL status from slave
    ec_datagram_fprd(datagram, slave->station_address, 0x0130, 2);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
    fsm->spontaneous_change = 0;
    fsm->state = ec_fsm_change_state_status;
}

/****************************************************************************/

/**
   Change state: STATUS.
*/

void ec_fsm_change_state_status(ec_fsm_change_t *fsm
                                /**< finite state machine */)
{
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;
    unsigned int timeout_ms;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive state checking datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        char req_state[EC_STATE_STRING_SIZE];
        ec_state_string(fsm->requested_state, req_state, 0);
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Failed to check state %s: ", req_state);
        ec_datagram_print_wc_error(datagram);
        return;
    }

    if (fsm->take_time) {
        fsm->take_time = 0;
        fsm->jiffies_start = datagram->jiffies_sent;
    }

    slave->current_state = EC_READ_U8(datagram->data);

    if (slave->current_state == fsm->requested_state) {
        // state has been set successfully
        fsm->state = ec_fsm_change_state_end;
        return;
    }

    if (slave->current_state != fsm->old_state) { // state changed
        char req_state[EC_STATE_STRING_SIZE], cur_state[EC_STATE_STRING_SIZE];

        ec_state_string(slave->current_state, cur_state, 0);

        if ((slave->current_state & 0x0F) != (fsm->old_state & 0x0F)) {
            // Slave spontaneously changed its state just before the new state
            // was written. Accept current state as old state and wait for
            // state change
            fsm->spontaneous_change = 1;
            fsm->old_state = slave->current_state;
            EC_SLAVE_WARN(slave, "Changed to %s in the meantime.\n",
                    cur_state);
            goto check_again;
        }

        // state change error

        slave->error_flag = 1;
        ec_state_string(fsm->requested_state, req_state, 0);

        EC_SLAVE_ERR(slave, "Failed to set %s state, slave refused state"
                " change (%s).\n", req_state, cur_state);

        ec_fsm_change_state_start_code(fsm);
        return;
    }

    // still old state

    timeout_ms = ec_fsm_change_timeout_ms(fsm);
    if (datagram->jiffies_received - fsm->jiffies_start >=
            timeout_ms * HZ / 1000) {
        // timeout while checking
        char state_str[EC_STATE_STRING_SIZE];
        ec_state_string(fsm->requested_state, state_str, 0);
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Timeout after %u ms while setting state %s.\n",
                timeout_ms, state_str);
        return;
    }

 check_again:
    // no timeout yet. check again
    ec_datagram_fprd(datagram, slave->station_address, 0x0130, 2);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
}

/****************************************************************************/

/** Enter reading AL status code.
 */
void ec_fsm_change_state_start_code(
        ec_fsm_change_t *fsm /**< finite state machine */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_datagram_t *datagram = fsm->datagram;

    // fetch AL status error code
    ec_datagram_fprd(datagram, slave->station_address, 0x0134, 2);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_change_state_code;
}

/****************************************************************************/

/**
   Application layer status messages.
*/

const ec_code_msg_t al_status_messages[] = {
    {0x0000, "No error"},
    {0x0001, "Unspecified error"},
    {0x0002, "No Memory"},
    {0x0011, "Invalid requested state change"},
    {0x0012, "Unknown requested state"},
    {0x0013, "Bootstrap not supported"},
    {0x0014, "No valid firmware"},
    {0x0015, "Invalid mailbox configuration"},
    {0x0016, "Invalid mailbox configuration"},
    {0x0017, "Invalid sync manager configuration"},
    {0x0018, "No valid inputs available"},
    {0x0019, "No valid outputs"},
    {0x001A, "Synchronization error"},
    {0x001B, "Sync manager watchdog"},
    {0x001C, "Invalid sync manager types"},
    {0x001D, "Invalid output configuration"},
    {0x001E, "Invalid input configuration"},
    {0x001F, "Invalid watchdog configuration"},
    {0x0020, "Slave needs cold start"},
    {0x0021, "Slave needs INIT"},
    {0x0022, "Slave needs PREOP"},
    {0x0023, "Slave needs SAFEOP"},
    {0x0024, "Invalid Input Mapping"},
    {0x0025, "Invalid Output Mapping"},
    {0x0026, "Inconsistent Settings"},
    {0x0027, "Freerun not supported"},
    {0x0028, "Synchronization not supported"},
    {0x0029, "Freerun needs 3 Buffer Mode"},
    {0x002A, "Background Watchdog"},
    {0x002B, "No Valid Inputs and Outputs"},
    {0x002C, "Fatal Sync Error"},
    {0x002D, "No Sync Error"},
    {0x0030, "Invalid DC SYNCH configuration"},
    {0x0031, "Invalid DC latch configuration"},
    {0x0032, "PLL error"},
    {0x0033, "DC Sync IO Error"},
    {0x0034, "DC Sync Timeout Error"},
    {0x0035, "DC Invalid Sync Cycle Time"},
    {0x0036, "DC Sync0 Cycle Time"},
    {0x0037, "DC Sync1 Cycle Time"},
    {0x0041, "MBX_AOE"},
    {0x0042, "MBX_EOE"},
    {0x0043, "MBX_COE"},
    {0x0044, "MBX_FOE"},
    {0x0045, "MBX_SOE"},
    {0x004F, "MBX_VOE"},
    {0x0050, "EEPROM No Access"},
    {0x0051, "EEPROM Error"},
    {0x0060, "Slave Restarted Locally"},
    {0xffff}
};


/****************************************************************************/

/**
   Change state: CODE.
*/

void ec_fsm_change_state_code(ec_fsm_change_t *fsm
                              /**< finite state machine */)
{
    ec_datagram_t *datagram = fsm->datagram;
    uint32_t code;
    const ec_code_msg_t *al_msg;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(fsm->slave, "Failed to receive"
                " AL status code datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        EC_SLAVE_WARN(fsm->slave, "Reception of AL status code"
                " datagram failed: ");
        ec_datagram_print_wc_error(datagram);
    } else {
        code = EC_READ_U16(datagram->data);
        for (al_msg = al_status_messages; al_msg->code != 0xffff; al_msg++) {
            if (al_msg->code != code) {
                continue;
            }

            EC_SLAVE_ERR(fsm->slave, "AL status message 0x%04X: \"%s\".\n",
                    al_msg->code, al_msg->message);
            break;
        }
        if (al_msg->code == 0xffff) { /* not found in our list. */
            EC_SLAVE_ERR(fsm->slave, "Unknown AL status code 0x%04X.\n",
                    code);
        }
    }

    // acknowledge "old" slave state
    ec_datagram_fpwr(datagram, fsm->slave->station_address, 0x0120, 2);
    EC_WRITE_U16(datagram->data, fsm->slave->current_state);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_change_state_ack;
}

/****************************************************************************/

/**
   Change state: ACK.
*/

void ec_fsm_change_state_ack(ec_fsm_change_t *fsm /**< finite state machine */)
{
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive state ack datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Reception of state ack datagram failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    fsm->take_time = 1;

    // read new AL status
    ec_datagram_fprd(datagram, slave->station_address, 0x0130, 2);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_change_state_check_ack;
}

/****************************************************************************/

/**
   Change state: CHECK ACK.
*/

void ec_fsm_change_state_check_ack(ec_fsm_change_t *fsm
                                   /**< finite state machine */)
{
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;
    unsigned int timeout_ms;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive state ack check datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Reception of state ack check datagram failed: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    if (fsm->take_time) {
        fsm->take_time = 0;
        fsm->jiffies_start = datagram->jiffies_sent;
    }

    slave->current_state = EC_READ_U8(datagram->data);

    if (!(slave->current_state & EC_SLAVE_STATE_ACK_ERR)) {
        char state_str[EC_STATE_STRING_SIZE];
        ec_state_string(slave->current_state, state_str, 0);
        if (fsm->mode == EC_FSM_CHANGE_MODE_FULL) {
            fsm->state = ec_fsm_change_state_error;
        }
        else { // EC_FSM_CHANGE_MODE_ACK_ONLY
            fsm->state = ec_fsm_change_state_end;
        }
        EC_SLAVE_INFO(slave, "Acknowledged state %s.\n", state_str);
        return;
    }

    timeout_ms = ec_fsm_change_timeout_ms(fsm);
    if (datagram->jiffies_received - fsm->jiffies_start >=
            timeout_ms * HZ / 1000) {
        // timeout while checking
        char state_str[EC_STATE_STRING_SIZE];
        ec_state_string(slave->current_state, state_str, 0);
        fsm->state = ec_fsm_change_state_error;
        EC_SLAVE_ERR(slave, "Timeout after %u ms while acknowledging"
                " state %s.\n", timeout_ms, state_str);
        return;
    }

    // reread new AL status
    ec_datagram_fprd(datagram, slave->station_address, 0x0130, 2);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
}

/****************************************************************************/

/**
   State: ERROR.
*/

void ec_fsm_change_state_error(ec_fsm_change_t *fsm
                               /**< finite state machine */)
{
}

/****************************************************************************/

/**
   State: END.
*/

void ec_fsm_change_state_end(ec_fsm_change_t *fsm
                             /**< finite state machine */)
{
}

/****************************************************************************/

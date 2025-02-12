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
 * EtherCAT PDO mapping state machine.
 */

/****************************************************************************/

#include "globals.h"
#include "master.h"
#include "mailbox.h"
#include "slave_config.h"

#include "fsm_pdo_entry.h"

/****************************************************************************/

// prototypes for private methods
void ec_fsm_pdo_entry_print(const ec_fsm_pdo_entry_t *);
int ec_fsm_pdo_entry_running(const ec_fsm_pdo_entry_t *);
ec_pdo_entry_t *ec_fsm_pdo_entry_conf_next_entry(const ec_fsm_pdo_entry_t *,
        const struct list_head *);

/****************************************************************************/

void ec_fsm_pdo_entry_read_state_start(ec_fsm_pdo_entry_t *, ec_datagram_t *);
void ec_fsm_pdo_entry_read_state_count(ec_fsm_pdo_entry_t *, ec_datagram_t *);
void ec_fsm_pdo_entry_read_state_entry(ec_fsm_pdo_entry_t *, ec_datagram_t *);

void ec_fsm_pdo_entry_read_action_next(ec_fsm_pdo_entry_t *, ec_datagram_t *);

void ec_fsm_pdo_entry_conf_state_start(ec_fsm_pdo_entry_t *, ec_datagram_t *);
void ec_fsm_pdo_entry_conf_state_zero_entry_count(ec_fsm_pdo_entry_t *,
        ec_datagram_t *);
void ec_fsm_pdo_entry_conf_state_map_entry(ec_fsm_pdo_entry_t *,
        ec_datagram_t *);
void ec_fsm_pdo_entry_conf_state_set_entry_count(ec_fsm_pdo_entry_t *,
        ec_datagram_t *);

void ec_fsm_pdo_entry_conf_action_map(ec_fsm_pdo_entry_t *, ec_datagram_t *);

void ec_fsm_pdo_entry_state_end(ec_fsm_pdo_entry_t *, ec_datagram_t *);
void ec_fsm_pdo_entry_state_error(ec_fsm_pdo_entry_t *, ec_datagram_t *);

/****************************************************************************/

/** Constructor.
 */
void ec_fsm_pdo_entry_init(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_fsm_coe_t *fsm_coe /**< CoE state machine to use. */
        )
{
    fsm->fsm_coe = fsm_coe;
    ec_sdo_request_init(&fsm->request);
}

/****************************************************************************/

/** Destructor.
 */
void ec_fsm_pdo_entry_clear(
        ec_fsm_pdo_entry_t *fsm /**< PDO mapping state machine. */
        )
{
    ec_sdo_request_clear(&fsm->request);
}

/****************************************************************************/

/** Print the current and desired PDO mapping.
 */
void ec_fsm_pdo_entry_print(
        const ec_fsm_pdo_entry_t *fsm /**< PDO mapping state machine. */
        )
{
    printk(KERN_CONT "Currently mapped PDO entries: ");
    ec_pdo_print_entries(fsm->cur_pdo);
    printk(KERN_CONT ". Entries to map: ");
    ec_pdo_print_entries(fsm->source_pdo);
    printk(KERN_CONT "\n");
}

/****************************************************************************/

/** Start reading a PDO's entries.
 */
void ec_fsm_pdo_entry_start_reading(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_slave_t *slave, /**< Slave to configure. */
        ec_pdo_t *pdo /**< PDO to read entries for. */
        )
{
    fsm->slave = slave;
    fsm->target_pdo = pdo;

    ec_pdo_clear_entries(fsm->target_pdo);

    fsm->state = ec_fsm_pdo_entry_read_state_start;
}

/****************************************************************************/

/** Start PDO mapping state machine.
 */
void ec_fsm_pdo_entry_start_configuration(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_slave_t *slave, /**< Slave to configure. */
        const ec_pdo_t *pdo, /**< PDO with the desired entries. */
        const ec_pdo_t *cur_pdo /**< Current PDO mapping. */
        )
{
    fsm->slave = slave;
    fsm->source_pdo = pdo;
    fsm->cur_pdo = cur_pdo;

    if (fsm->slave->master->debug_level) {
        EC_SLAVE_DBG(slave, 1, "Changing mapping of PDO 0x%04X.\n",
                pdo->index);
        EC_SLAVE_DBG(slave, 1, ""); ec_fsm_pdo_entry_print(fsm);
    }

    fsm->state = ec_fsm_pdo_entry_conf_state_start;
}

/****************************************************************************/

/** Get running state.
 *
 * \return false, if state machine has terminated
 */
int ec_fsm_pdo_entry_running(
        const ec_fsm_pdo_entry_t *fsm /**< PDO mapping state machine. */
        )
{
    return fsm->state != ec_fsm_pdo_entry_state_end
        && fsm->state != ec_fsm_pdo_entry_state_error;
}

/****************************************************************************/

/** Executes the current state.
 *
 * \return false, if state machine has terminated
 */
int ec_fsm_pdo_entry_exec(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    fsm->state(fsm, datagram);

    return ec_fsm_pdo_entry_running(fsm);
}

/****************************************************************************/

/** Get execution result.
 *
 * \return true, if the state machine terminated gracefully
 */
int ec_fsm_pdo_entry_success(
        const ec_fsm_pdo_entry_t *fsm /**< PDO mapping state machine. */
        )
{
    return fsm->state == ec_fsm_pdo_entry_state_end;
}

/*****************************************************************************
 * Reading state functions.
 ****************************************************************************/

/** Request reading the number of mapped PDO entries.
 */
void ec_fsm_pdo_entry_read_state_start(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ecrt_sdo_request_index(&fsm->request, fsm->target_pdo->index, 0);
    ecrt_sdo_request_read(&fsm->request);

    fsm->state = ec_fsm_pdo_entry_read_state_count;
    ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
    ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
}

/****************************************************************************/

/** Read number of mapped PDO entries.
 */
void ec_fsm_pdo_entry_read_state_count(
        ec_fsm_pdo_entry_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_ERR(fsm->slave,
                "Failed to read number of mapped PDO entries.\n");
        fsm->state = ec_fsm_pdo_entry_state_error;
        return;
    }

    if (fsm->request.data_size != sizeof(uint8_t)) {
        EC_SLAVE_ERR(fsm->slave, "Invalid data size %zu at uploading"
                " SDO 0x%04X:%02X.\n",
                fsm->request.data_size, fsm->request.index,
                fsm->request.subindex);
        fsm->state = ec_fsm_pdo_entry_state_error;
        return;
    }

    fsm->entry_count = EC_READ_U8(fsm->request.data);

    EC_SLAVE_DBG(fsm->slave, 1, "%u PDO entries mapped.\n", fsm->entry_count);

    // read first PDO entry
    fsm->entry_pos = 1;
    ec_fsm_pdo_entry_read_action_next(fsm, datagram);
}

/****************************************************************************/

/** Read next PDO entry.
 */
void ec_fsm_pdo_entry_read_action_next(
        ec_fsm_pdo_entry_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (fsm->entry_pos <= fsm->entry_count) {
        ecrt_sdo_request_index(&fsm->request, fsm->target_pdo->index,
                fsm->entry_pos);
        ecrt_sdo_request_read(&fsm->request);
        fsm->state = ec_fsm_pdo_entry_read_state_entry;
        ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
        ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
        return;
    }

    // finished reading entries.
    fsm->state = ec_fsm_pdo_entry_state_end;
}

/****************************************************************************/

/** Read PDO entry information.
 */
void ec_fsm_pdo_entry_read_state_entry(
        ec_fsm_pdo_entry_t *fsm, /**< finite state machine */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_ERR(fsm->slave, "Failed to read mapped PDO entry.\n");
        fsm->state = ec_fsm_pdo_entry_state_error;
        return;
    }

    if (fsm->request.data_size != sizeof(uint32_t)) {
        EC_SLAVE_ERR(fsm->slave, "Invalid data size %zu at"
                " uploading SDO 0x%04X:%02X.\n",
                fsm->request.data_size, fsm->request.index,
                fsm->request.subindex);
        fsm->state = ec_fsm_pdo_entry_state_error;
    } else {
        uint32_t pdo_entry_info;
        ec_pdo_entry_t *pdo_entry;

        pdo_entry_info = EC_READ_U32(fsm->request.data);

        if (!(pdo_entry = (ec_pdo_entry_t *)
                    kmalloc(sizeof(ec_pdo_entry_t), GFP_KERNEL))) {
            EC_SLAVE_ERR(fsm->slave, "Failed to allocate PDO entry.\n");
            fsm->state = ec_fsm_pdo_entry_state_error;
            return;
        }

        ec_pdo_entry_init(pdo_entry);
        pdo_entry->index = pdo_entry_info >> 16;
        pdo_entry->subindex = (pdo_entry_info >> 8) & 0xFF;
        pdo_entry->bit_length = pdo_entry_info & 0xFF;

        if (!pdo_entry->index && !pdo_entry->subindex) {
            if (ec_pdo_entry_set_name(pdo_entry, "Gap")) {
                ec_pdo_entry_clear(pdo_entry);
                kfree(pdo_entry);
                fsm->state = ec_fsm_pdo_entry_state_error;
                return;
            }
        }

        EC_SLAVE_DBG(fsm->slave, 1,
                "PDO entry 0x%04X:%02X, %u bit, \"%s\".\n",
                pdo_entry->index, pdo_entry->subindex,
                pdo_entry->bit_length,
                pdo_entry->name ? pdo_entry->name : "???");

        list_add_tail(&pdo_entry->list, &fsm->target_pdo->entries);

        // next PDO entry
        fsm->entry_pos++;
        ec_fsm_pdo_entry_read_action_next(fsm, datagram);
    }
}

/*****************************************************************************
 * Configuration state functions.
 ****************************************************************************/

/** Start PDO mapping.
 */
void ec_fsm_pdo_entry_conf_state_start(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_sdo_request_alloc(&fsm->request, 4)) {
        fsm->state = ec_fsm_pdo_entry_state_error;
        return;
    }

    // set mapped PDO entry count to zero
    EC_WRITE_U8(fsm->request.data, 0);
    fsm->request.data_size = 1;
    ecrt_sdo_request_index(&fsm->request, fsm->source_pdo->index, 0);
    ecrt_sdo_request_write(&fsm->request);

    EC_SLAVE_DBG(fsm->slave, 1, "Setting entry count to zero.\n");

    fsm->state = ec_fsm_pdo_entry_conf_state_zero_entry_count;
    ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
    ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
}

/****************************************************************************/

/** Process next PDO entry.
 *
 * \return Next PDO entry, or NULL.
 */
ec_pdo_entry_t *ec_fsm_pdo_entry_conf_next_entry(
        const ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        const struct list_head *list /**< current entry list item */
        )
{
    list = list->next;
    if (list == &fsm->source_pdo->entries)
        return NULL; // no next entry
    return list_entry(list, ec_pdo_entry_t, list);
}

/****************************************************************************/

/** Set the number of mapped entries to zero.
 */
void ec_fsm_pdo_entry_conf_state_zero_entry_count(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_WARN(fsm->slave, "Failed to clear PDO mapping.\n");
        EC_SLAVE_WARN(fsm->slave, ""); ec_fsm_pdo_entry_print(fsm);
        fsm->state = ec_fsm_pdo_entry_state_error;
        return;
    }

    // find first entry
    if (!(fsm->entry = ec_fsm_pdo_entry_conf_next_entry(
                    fsm, &fsm->source_pdo->entries))) {

        EC_SLAVE_DBG(fsm->slave, 1, "No entries to map.\n");

        fsm->state = ec_fsm_pdo_entry_state_end; // finished
        return;
    }

    // add first entry
    fsm->entry_pos = 1;
    ec_fsm_pdo_entry_conf_action_map(fsm, datagram);
}

/****************************************************************************/

/** Starts to add a PDO entry.
 */
void ec_fsm_pdo_entry_conf_action_map(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    uint32_t value;

    EC_SLAVE_DBG(fsm->slave, 1, "Mapping PDO entry 0x%04X:%02X (%u bit)"
            " at position %u.\n",
            fsm->entry->index, fsm->entry->subindex,
            fsm->entry->bit_length, fsm->entry_pos);

    value = fsm->entry->index << 16
        | fsm->entry->subindex << 8 | fsm->entry->bit_length;
    EC_WRITE_U32(fsm->request.data, value);
    fsm->request.data_size = 4;
    ecrt_sdo_request_index(&fsm->request, fsm->source_pdo->index,
            fsm->entry_pos);
    ecrt_sdo_request_write(&fsm->request);

    fsm->state = ec_fsm_pdo_entry_conf_state_map_entry;
    ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
    ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
}

/****************************************************************************/

/** Add a PDO entry.
 */
void ec_fsm_pdo_entry_conf_state_map_entry(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_WARN(fsm->slave, "Failed to map PDO entry"
                " 0x%04X:%02X (%u bit) to position %u.\n",
                fsm->entry->index, fsm->entry->subindex,
                fsm->entry->bit_length, fsm->entry_pos);
        EC_SLAVE_WARN(fsm->slave, ""); ec_fsm_pdo_entry_print(fsm);
        fsm->state = ec_fsm_pdo_entry_state_error;
        return;
    }

    // find next entry
    if (!(fsm->entry = ec_fsm_pdo_entry_conf_next_entry(
                    fsm, &fsm->entry->list))) {

        // No more entries to add. Write entry count.
        EC_WRITE_U8(fsm->request.data, fsm->entry_pos);
        fsm->request.data_size = 1;
        ecrt_sdo_request_index(&fsm->request, fsm->source_pdo->index, 0);
        ecrt_sdo_request_write(&fsm->request);

        EC_SLAVE_DBG(fsm->slave, 1, "Setting number of PDO entries to %u.\n",
                fsm->entry_pos);

        fsm->state = ec_fsm_pdo_entry_conf_state_set_entry_count;
        ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
        ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
        return;
    }

    // add next entry
    fsm->entry_pos++;
    ec_fsm_pdo_entry_conf_action_map(fsm, datagram);
}

/****************************************************************************/

/** Set the number of entries.
 */
void ec_fsm_pdo_entry_conf_state_set_entry_count(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_WARN(fsm->slave, "Failed to set number of entries.\n");
        EC_SLAVE_WARN(fsm->slave, ""); ec_fsm_pdo_entry_print(fsm);
        fsm->state = ec_fsm_pdo_entry_state_error;
        return;
    }

    EC_SLAVE_DBG(fsm->slave, 1, "Successfully configured"
            " mapping for PDO 0x%04X.\n", fsm->source_pdo->index);

    fsm->state = ec_fsm_pdo_entry_state_end; // finished
}

/*****************************************************************************
 * Common state functions
 ****************************************************************************/

/** State: ERROR.
 */
void ec_fsm_pdo_entry_state_error(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

/** State: END.
 */
void ec_fsm_pdo_entry_state_end(
        ec_fsm_pdo_entry_t *fsm, /**< PDO mapping state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

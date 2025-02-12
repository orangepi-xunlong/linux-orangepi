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
 * EtherCAT PDO configuration state machine.
 */

/****************************************************************************/

#include "globals.h"
#include "master.h"
#include "mailbox.h"
#include "slave_config.h"

#include "fsm_pdo.h"

/****************************************************************************/

// prototypes for private methods
void ec_fsm_pdo_print(const ec_fsm_pdo_t *);
int ec_fsm_pdo_running(const ec_fsm_pdo_t *);
ec_pdo_t *ec_fsm_pdo_conf_action_next_pdo(const ec_fsm_pdo_t *,
        const struct list_head *);

/****************************************************************************/

void ec_fsm_pdo_read_state_start(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_read_state_pdo_count(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_read_state_pdo(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_read_state_pdo_entries(ec_fsm_pdo_t *, ec_datagram_t *);

void ec_fsm_pdo_read_action_next_sync(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_read_action_next_pdo(ec_fsm_pdo_t *, ec_datagram_t *);

void ec_fsm_pdo_conf_state_start(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_state_read_mapping(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_state_mapping(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_state_zero_pdo_count(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_state_assign_pdo(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_state_set_pdo_count(ec_fsm_pdo_t *, ec_datagram_t *);

void ec_fsm_pdo_conf_action_next_sync(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_action_pdo_mapping(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_action_check_mapping(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_action_next_pdo_mapping(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_action_check_assignment(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_conf_action_assign_pdo(ec_fsm_pdo_t *, ec_datagram_t *);

void ec_fsm_pdo_state_end(ec_fsm_pdo_t *, ec_datagram_t *);
void ec_fsm_pdo_state_error(ec_fsm_pdo_t *, ec_datagram_t *);

/****************************************************************************/

/** Constructor.
 */
void ec_fsm_pdo_init(
        ec_fsm_pdo_t *fsm, /**< PDO configuration state machine. */
        ec_fsm_coe_t *fsm_coe /**< CoE state machine to use */
        )
{
    fsm->fsm_coe = fsm_coe;
    ec_fsm_pdo_entry_init(&fsm->fsm_pdo_entry, fsm_coe);
    ec_pdo_list_init(&fsm->pdos);
    ec_sdo_request_init(&fsm->request);
    ec_pdo_init(&fsm->slave_pdo);
}

/****************************************************************************/

/** Destructor.
 */
void ec_fsm_pdo_clear(
        ec_fsm_pdo_t *fsm /**< PDO configuration state machine. */
        )
{
    ec_fsm_pdo_entry_clear(&fsm->fsm_pdo_entry);
    ec_pdo_list_clear(&fsm->pdos);
    ec_sdo_request_clear(&fsm->request);
    ec_pdo_clear(&fsm->slave_pdo);
}

/****************************************************************************/

/** Print the current and desired PDO assignment.
 */
void ec_fsm_pdo_print(
        const ec_fsm_pdo_t *fsm /**< PDO configuration state machine. */
        )
{
    printk(KERN_CONT "Currently assigned PDOs: ");
    ec_pdo_list_print(&fsm->sync->pdos);
    printk(KERN_CONT ". PDOs to assign: ");
    ec_pdo_list_print(&fsm->pdos);
    printk(KERN_CONT "\n");
}

/****************************************************************************/

/** Start reading the PDO configuration.
 */
void ec_fsm_pdo_start_reading(
        ec_fsm_pdo_t *fsm, /**< PDO configuration state machine. */
        ec_slave_t *slave /**< slave to configure */
        )
{
    fsm->slave = slave;
    fsm->state = ec_fsm_pdo_read_state_start;
}

/****************************************************************************/

/** Start writing the PDO configuration.
 */
void ec_fsm_pdo_start_configuration(
        ec_fsm_pdo_t *fsm, /**< PDO configuration state machine. */
        ec_slave_t *slave /**< slave to configure */
        )
{
    fsm->slave = slave;
    fsm->state = ec_fsm_pdo_conf_state_start;
}

/****************************************************************************/

/** Get running state.
 *
 * \return false, if state machine has terminated
 */
int ec_fsm_pdo_running(
        const ec_fsm_pdo_t *fsm /**< PDO configuration state machine. */
        )
{
    return fsm->state != ec_fsm_pdo_state_end
        && fsm->state != ec_fsm_pdo_state_error;
}

/****************************************************************************/

/** Executes the current state of the state machine.
 *
 * If the state machine's datagram is not sent or received yet, the execution
 * of the state machine is delayed to the next cycle.
 *
 * \return false, if state machine has terminated
 */
int ec_fsm_pdo_exec(
        ec_fsm_pdo_t *fsm, /**< PDO configuration state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    fsm->state(fsm, datagram);

    return ec_fsm_pdo_running(fsm);
}

/****************************************************************************/

/** Get execution result.
 *
 * \return true, if the state machine terminated gracefully
 */
int ec_fsm_pdo_success(
        const ec_fsm_pdo_t *fsm /**< PDO configuration state machine. */
        )
{
    return fsm->state == ec_fsm_pdo_state_end;
}

/*****************************************************************************
 * Reading state funtions.
 ****************************************************************************/

/** Start reading PDO assignment.
 */
void ec_fsm_pdo_read_state_start(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    // read PDO assignment for first sync manager not reserved for mailbox
    fsm->sync_index = 1; // next is 2
    ec_fsm_pdo_read_action_next_sync(fsm, datagram);
}

/****************************************************************************/

/** Read PDO assignment of next sync manager.
 */
void ec_fsm_pdo_read_action_next_sync(
        ec_fsm_pdo_t *fsm, /**< finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;

    fsm->sync_index++;

    for (; fsm->sync_index < EC_MAX_SYNC_MANAGERS; fsm->sync_index++) {
        if (!(fsm->sync = ec_slave_get_sync(slave, fsm->sync_index)))
            continue;

        EC_SLAVE_DBG(slave, 1, "Reading PDO assignment of SM%u.\n",
                fsm->sync_index);

        ec_pdo_list_clear_pdos(&fsm->pdos);

        ecrt_sdo_request_index(&fsm->request, 0x1C10 + fsm->sync_index, 0);
        ecrt_sdo_request_read(&fsm->request);
        fsm->state = ec_fsm_pdo_read_state_pdo_count;
        ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
        ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Reading of PDO configuration finished.\n");

    ec_pdo_list_clear_pdos(&fsm->pdos);
    fsm->state = ec_fsm_pdo_state_end;
}

/****************************************************************************/

/** Count assigned PDOs.
 */
void ec_fsm_pdo_read_state_pdo_count(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_ERR(fsm->slave, "Failed to read number of assigned PDOs"
                " for SM%u.\n", fsm->sync_index);
        ec_fsm_pdo_read_action_next_sync(fsm, datagram);
        return;
    }

    if (fsm->request.data_size != sizeof(uint8_t)) {
        EC_SLAVE_ERR(fsm->slave, "Invalid data size %zu returned"
                " when uploading SDO 0x%04X:%02X.\n", fsm->request.data_size,
                fsm->request.index, fsm->request.subindex);
        ec_fsm_pdo_read_action_next_sync(fsm, datagram);
        return;
    }
    fsm->pdo_count = EC_READ_U8(fsm->request.data);

    EC_SLAVE_DBG(fsm->slave, 1, "%u PDOs assigned.\n", fsm->pdo_count);

    // read first PDO
    fsm->pdo_pos = 1;
    ec_fsm_pdo_read_action_next_pdo(fsm, datagram);
}

/****************************************************************************/

/** Read next PDO.
 */
void ec_fsm_pdo_read_action_next_pdo(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (fsm->pdo_pos <= fsm->pdo_count) {
        ecrt_sdo_request_index(&fsm->request, 0x1C10 + fsm->sync_index,
                fsm->pdo_pos);
        ecrt_sdo_request_read(&fsm->request);
        fsm->state = ec_fsm_pdo_read_state_pdo;
        ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
        ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
        return;
    }

    // finished reading PDO configuration

    ec_pdo_list_copy(&fsm->sync->pdos, &fsm->pdos);
    ec_pdo_list_clear_pdos(&fsm->pdos);

    // next sync manager
    ec_fsm_pdo_read_action_next_sync(fsm, datagram);
}

/****************************************************************************/

/** Fetch PDO information.
 */
void ec_fsm_pdo_read_state_pdo(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_ERR(fsm->slave, "Failed to read index of"
                " assigned PDO %u from SM%u.\n",
                fsm->pdo_pos, fsm->sync_index);
        ec_fsm_pdo_read_action_next_sync(fsm, datagram);
        return;
    }

    if (fsm->request.data_size != sizeof(uint16_t)) {
        EC_SLAVE_ERR(fsm->slave, "Invalid data size %zu returned"
                " when uploading SDO 0x%04X:%02X.\n", fsm->request.data_size,
                fsm->request.index, fsm->request.subindex);
        ec_fsm_pdo_read_action_next_sync(fsm, datagram);
        return;
    }

    if (!(fsm->pdo = (ec_pdo_t *)
                kmalloc(sizeof(ec_pdo_t), GFP_KERNEL))) {
        EC_SLAVE_ERR(fsm->slave, "Failed to allocate PDO.\n");
        ec_fsm_pdo_read_action_next_sync(fsm, datagram);
        return;
    }

    ec_pdo_init(fsm->pdo);
    fsm->pdo->index = EC_READ_U16(fsm->request.data);
    fsm->pdo->sync_index = fsm->sync_index;

    EC_SLAVE_DBG(fsm->slave, 1, "PDO 0x%04X.\n", fsm->pdo->index);

    list_add_tail(&fsm->pdo->list, &fsm->pdos.list);

    fsm->state = ec_fsm_pdo_read_state_pdo_entries;
    ec_fsm_pdo_entry_start_reading(&fsm->fsm_pdo_entry, fsm->slave, fsm->pdo);
    fsm->state(fsm, datagram); // execute immediately
}

/****************************************************************************/

/** Fetch PDO information.
 */
void ec_fsm_pdo_read_state_pdo_entries(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_pdo_entry_exec(&fsm->fsm_pdo_entry, datagram)) {
        return;
    }

    if (!ec_fsm_pdo_entry_success(&fsm->fsm_pdo_entry)) {
        EC_SLAVE_ERR(fsm->slave, "Failed to read mapped PDO entries"
                " for PDO 0x%04X.\n", fsm->pdo->index);
        ec_fsm_pdo_read_action_next_sync(fsm, datagram);
        return;
    }

    // next PDO
    fsm->pdo_pos++;
    ec_fsm_pdo_read_action_next_pdo(fsm, datagram);
}

/*****************************************************************************
 * Writing state functions.
 ****************************************************************************/

/** Start PDO configuration.
 */
void ec_fsm_pdo_conf_state_start(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (!fsm->slave->config) {
        fsm->state = ec_fsm_pdo_state_end;
        return;
    }

    fsm->sync_index = 1; // next is 2
    ec_fsm_pdo_conf_action_next_sync(fsm, datagram);
}

/****************************************************************************/

/** Assign next PDO.
 *
 * \return Next PDO, or NULL.
 */
ec_pdo_t *ec_fsm_pdo_conf_action_next_pdo(
        const ec_fsm_pdo_t *fsm, /**< PDO configuration state machine. */
        const struct list_head *list /**< current PDO list item */
        )
{
    list = list->next;
    if (list == &fsm->pdos.list)
        return NULL; // no next PDO
    return list_entry(list, ec_pdo_t, list);
}

/****************************************************************************/

/** Get the next sync manager for a pdo configuration.
 */
void ec_fsm_pdo_conf_action_next_sync(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    fsm->sync_index++;

    for (; fsm->sync_index < EC_MAX_SYNC_MANAGERS; fsm->sync_index++) {
        if (!fsm->slave->config) {
            // slave configuration removed in the meantime
            fsm->state = ec_fsm_pdo_state_error;
            return;
        }

        if (ec_pdo_list_copy(&fsm->pdos,
                    &fsm->slave->config->sync_configs[fsm->sync_index].pdos))
        {
            fsm->state = ec_fsm_pdo_state_error;
            return;
        }

        if (!(fsm->sync = ec_slave_get_sync(fsm->slave, fsm->sync_index))) {
            if (!list_empty(&fsm->pdos.list))
                EC_SLAVE_WARN(fsm->slave, "PDOs configured for SM%u,"
                        " but slave does not provide the"
                        " sync manager information!\n",
                        fsm->sync_index);
            continue;
        }

        // get first configured PDO
        if (!(fsm->pdo =
                    ec_fsm_pdo_conf_action_next_pdo(fsm, &fsm->pdos.list))) {
            // no pdos configured
            ec_fsm_pdo_conf_action_check_assignment(fsm, datagram);
            return;
        }

        ec_fsm_pdo_conf_action_pdo_mapping(fsm, datagram);
        return;
    }

    fsm->state = ec_fsm_pdo_state_end;
}

/****************************************************************************/

/** Check if the mapping has to be read, otherwise start to configure it.
 */
void ec_fsm_pdo_conf_action_pdo_mapping(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    const ec_pdo_t *assigned_pdo;

    fsm->slave_pdo.index = fsm->pdo->index;

    if ((assigned_pdo = ec_slave_find_pdo(fsm->slave, fsm->pdo->index))) {
        ec_pdo_copy_entries(&fsm->slave_pdo, assigned_pdo);
    } else { // configured PDO is not assigned and thus unknown
        ec_pdo_clear_entries(&fsm->slave_pdo);
    }

    if (list_empty(&fsm->slave_pdo.entries)) {
        EC_SLAVE_DBG(fsm->slave, 1, "Reading mapping of PDO 0x%04X.\n",
                fsm->pdo->index);

        // pdo mapping is unknown; start loading it
        ec_fsm_pdo_entry_start_reading(&fsm->fsm_pdo_entry, fsm->slave,
                &fsm->slave_pdo);
        fsm->state = ec_fsm_pdo_conf_state_read_mapping;
        fsm->state(fsm, datagram); // execute immediately
        return;
    }

    // pdo mapping is known, check if it most be re-configured
    ec_fsm_pdo_conf_action_check_mapping(fsm, datagram);
}

/****************************************************************************/

/** Execute the PDO entry state machine to read the current PDO's mapping.
 */
void ec_fsm_pdo_conf_state_read_mapping(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_pdo_entry_exec(&fsm->fsm_pdo_entry, datagram)) {
        return;
    }

    if (!ec_fsm_pdo_entry_success(&fsm->fsm_pdo_entry))
        EC_SLAVE_WARN(fsm->slave,
                "Failed to read PDO entries for PDO 0x%04X.\n",
                fsm->pdo->index);

    // check if the mapping must be re-configured
    ec_fsm_pdo_conf_action_check_mapping(fsm, datagram);
}

/****************************************************************************/

/** Check if the mapping has to be re-configured.
 *
 * \todo Display mapping differences.
 */
void ec_fsm_pdo_conf_action_check_mapping(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    // check, if slave supports PDO configuration
    if ((fsm->slave->sii.mailbox_protocols & EC_MBOX_COE)
            && fsm->slave->sii.has_general
            && fsm->slave->sii.coe_details.enable_pdo_configuration) {

        // always write PDO mapping
        ec_fsm_pdo_entry_start_configuration(&fsm->fsm_pdo_entry, fsm->slave,
                fsm->pdo, &fsm->slave_pdo);
        fsm->state = ec_fsm_pdo_conf_state_mapping;
        fsm->state(fsm, datagram); // execure immediately
        return;
    }
    else if (!ec_pdo_equal_entries(fsm->pdo, &fsm->slave_pdo)) {
        EC_SLAVE_WARN(fsm->slave, "Slave does not support"
                " changing the PDO mapping!\n");
        EC_SLAVE_WARN(fsm->slave, "");
        printk(KERN_CONT "Currently mapped PDO entries: ");
        ec_pdo_print_entries(&fsm->slave_pdo);
        printk(KERN_CONT ". Entries to map: ");
        ec_pdo_print_entries(fsm->pdo);
        printk(KERN_CONT "\n");
    }

    ec_fsm_pdo_conf_action_next_pdo_mapping(fsm, datagram);
}

/****************************************************************************/

/** Let the PDO entry state machine configure the current PDO's mapping.
 */
void ec_fsm_pdo_conf_state_mapping(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_pdo_entry_exec(&fsm->fsm_pdo_entry, datagram)) {
        return;
    }

    if (!ec_fsm_pdo_entry_success(&fsm->fsm_pdo_entry))
        EC_SLAVE_WARN(fsm->slave,
                "Failed to configure mapping of PDO 0x%04X.\n",
                fsm->pdo->index);

    ec_fsm_pdo_conf_action_next_pdo_mapping(fsm, datagram);
}

/****************************************************************************/

/** Check mapping of next PDO, otherwise configure assignment.
 */
void ec_fsm_pdo_conf_action_next_pdo_mapping(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    // get next configured PDO
    if (!(fsm->pdo = ec_fsm_pdo_conf_action_next_pdo(fsm, &fsm->pdo->list))) {
        // no more configured pdos
        ec_fsm_pdo_conf_action_check_assignment(fsm, datagram);
        return;
    }

    ec_fsm_pdo_conf_action_pdo_mapping(fsm, datagram);
}

/****************************************************************************/

/** Check if the PDO assignment of the current SM has to be re-configured.
 */
void ec_fsm_pdo_conf_action_check_assignment(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if ((fsm->slave->sii.mailbox_protocols & EC_MBOX_COE)
            && fsm->slave->sii.has_general
            && fsm->slave->sii.coe_details.enable_pdo_assign) {

        // always write PDO assignment
        if (fsm->slave->master->debug_level) {
            EC_SLAVE_DBG(fsm->slave, 1, "Setting PDO assignment of SM%u:\n",
                    fsm->sync_index);
            EC_SLAVE_DBG(fsm->slave, 1, ""); ec_fsm_pdo_print(fsm);
        }

        if (ec_sdo_request_alloc(&fsm->request, 2)) {
            fsm->state = ec_fsm_pdo_state_error;
            return;
        }

        // set mapped PDO count to zero
        EC_WRITE_U8(fsm->request.data, 0); // zero PDOs mapped
        fsm->request.data_size = 1;
        ecrt_sdo_request_index(&fsm->request, 0x1C10 + fsm->sync_index, 0);
        ecrt_sdo_request_write(&fsm->request);

        EC_SLAVE_DBG(fsm->slave, 1, "Setting number of assigned"
                " PDOs to zero.\n");

        fsm->state = ec_fsm_pdo_conf_state_zero_pdo_count;
        ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
        ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
        return;
    }
    else if (!ec_pdo_list_equal(&fsm->sync->pdos, &fsm->pdos)) {
        EC_SLAVE_WARN(fsm->slave, "Slave does not support assigning PDOs!\n");
        EC_SLAVE_WARN(fsm->slave, ""); ec_fsm_pdo_print(fsm);
    }

    ec_fsm_pdo_conf_action_next_sync(fsm, datagram);
}

/****************************************************************************/

/** Set the number of assigned PDOs to zero.
 */
void ec_fsm_pdo_conf_state_zero_pdo_count(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_WARN(fsm->slave, "Failed to clear PDO assignment of SM%u.\n",
                fsm->sync_index);
        EC_SLAVE_WARN(fsm->slave, "");
        ec_fsm_pdo_print(fsm);
        ec_fsm_pdo_conf_action_next_sync(fsm, datagram);
        return;
    }

    // the sync manager's assigned PDOs have been cleared
    ec_pdo_list_clear_pdos(&fsm->sync->pdos);

    // assign all PDOs belonging to the current sync manager

    // find first PDO
    if (!(fsm->pdo = ec_fsm_pdo_conf_action_next_pdo(fsm, &fsm->pdos.list))) {
        // check for mapping to be altered
        ec_fsm_pdo_conf_action_next_sync(fsm, datagram);
        return;
    }

    // assign first PDO
    fsm->pdo_pos = 1;
    ec_fsm_pdo_conf_action_assign_pdo(fsm, datagram);
}

/****************************************************************************/

/** Assign a PDO.
 */
void ec_fsm_pdo_conf_action_assign_pdo(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    EC_WRITE_U16(fsm->request.data, fsm->pdo->index);
    fsm->request.data_size = 2;
    ecrt_sdo_request_index(&fsm->request,
            0x1C10 + fsm->sync_index, fsm->pdo_pos);
    ecrt_sdo_request_write(&fsm->request);

    EC_SLAVE_DBG(fsm->slave, 1, "Assigning PDO 0x%04X at position %u.\n",
            fsm->pdo->index, fsm->pdo_pos);

    fsm->state = ec_fsm_pdo_conf_state_assign_pdo;
    ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
    ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
}

/****************************************************************************/

/** Add a PDO to the sync managers PDO assignment.
 */
void ec_fsm_pdo_conf_state_assign_pdo(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_WARN(fsm->slave, "Failed to assign PDO 0x%04X at position %u"
                " of SM%u.\n",
                fsm->pdo->index, fsm->pdo_pos, fsm->sync_index);
        EC_SLAVE_WARN(fsm->slave, ""); ec_fsm_pdo_print(fsm);
        fsm->state = ec_fsm_pdo_state_error;
        return;
    }

    // find next PDO
    if (!(fsm->pdo = ec_fsm_pdo_conf_action_next_pdo(fsm, &fsm->pdo->list))) {
        // no more PDOs to assign, set PDO count
        EC_WRITE_U8(fsm->request.data, fsm->pdo_pos);
        fsm->request.data_size = 1;
        ecrt_sdo_request_index(&fsm->request, 0x1C10 + fsm->sync_index, 0);
        ecrt_sdo_request_write(&fsm->request);

        EC_SLAVE_DBG(fsm->slave, 1,
                "Setting number of assigned PDOs to %u.\n",
                fsm->pdo_pos);

        fsm->state = ec_fsm_pdo_conf_state_set_pdo_count;
        ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request);
        ec_fsm_coe_exec(fsm->fsm_coe, datagram); // execute immediately
        return;
    }

    // add next PDO to assignment
    fsm->pdo_pos++;
    ec_fsm_pdo_conf_action_assign_pdo(fsm, datagram);
}

/****************************************************************************/

/** Set the number of assigned PDOs.
 */
void ec_fsm_pdo_conf_state_set_pdo_count(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    if (ec_fsm_coe_exec(fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_WARN(fsm->slave, "Failed to set number of"
                " assigned PDOs of SM%u.\n", fsm->sync_index);
        EC_SLAVE_WARN(fsm->slave, ""); ec_fsm_pdo_print(fsm);
        fsm->state = ec_fsm_pdo_state_error;
        return;
    }

    // PDOs have been configured
    ec_pdo_list_copy(&fsm->sync->pdos, &fsm->pdos);

    EC_SLAVE_DBG(fsm->slave, 1, "Successfully configured"
            " PDO assignment of SM%u.\n", fsm->sync_index);

    // check if PDO mapping has to be altered
    ec_fsm_pdo_conf_action_next_sync(fsm, datagram);
}

/*****************************************************************************
 * Common state functions
 ****************************************************************************/

/** State: ERROR.
 */
void ec_fsm_pdo_state_error(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

/** State: END.
 */
void ec_fsm_pdo_state_end(
        ec_fsm_pdo_t *fsm, /**< Finite state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
}

/****************************************************************************/

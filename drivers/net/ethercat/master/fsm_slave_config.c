/*****************************************************************************
 *
 *  Copyright (C) 2006-2024  Florian Pose, Ingenieurgemeinschaft IgH
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
 */

/** \file
 *
 * EtherCAT slave configuration state machine.
 */

/****************************************************************************/

#include <asm/div64.h>

#include "globals.h"
#include "master.h"
#include "mailbox.h"
#include "slave_config.h"
#include "fsm_slave_config.h"

/****************************************************************************/

/** Maximum clock difference (in ns) before going to SAFEOP.
 *
 * Wait for DC time difference to drop under this absolute value before
 * requesting SAFEOP.
 */
#define EC_DC_MAX_SYNC_DIFF_NS 10000

/** Maximum time (in ms) to wait for clock discipline.
 */
#define EC_DC_SYNC_WAIT_MS 5000

/** Time offset (in ns), that is added to cyclic start time.
 */
#define EC_DC_START_OFFSET 100000000ULL

/****************************************************************************/

// prototypes for private methods
int ec_fsm_slave_config_running(const ec_fsm_slave_config_t *);

/****************************************************************************/

void ec_fsm_slave_config_state_start(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_init(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_clear_fmmus(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_clear_sync(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_dc_clear_assign(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_mbox_sync(ec_fsm_slave_config_t *);
#ifdef EC_SII_ASSIGN
void ec_fsm_slave_config_state_assign_pdi(ec_fsm_slave_config_t *);
#endif
void ec_fsm_slave_config_state_boot_preop(ec_fsm_slave_config_t *);
#ifdef EC_SII_ASSIGN
void ec_fsm_slave_config_state_assign_ethercat(ec_fsm_slave_config_t *);
#endif
void ec_fsm_slave_config_state_sdo_conf(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_soe_conf_preop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_eoe_ip_param(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_watchdog_divider(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_watchdog(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_pdo_sync(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_pdo_conf(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_fmmu(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_dc_cycle(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_dc_sync_check(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_dc_start(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_dc_assign(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_wait_safeop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_safeop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_soe_conf_safeop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_op(ec_fsm_slave_config_t *);

void ec_fsm_slave_config_enter_init(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_clear_sync(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_dc_clear_assign(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_mbox_sync(ec_fsm_slave_config_t *);
#ifdef EC_SII_ASSIGN
void ec_fsm_slave_config_enter_assign_pdi(ec_fsm_slave_config_t *);
#endif
void ec_fsm_slave_config_enter_boot_preop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_sdo_conf(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_soe_conf_preop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_eoe_ip_param(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_pdo_conf(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_watchdog_divider(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_watchdog(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_pdo_sync(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_fmmu(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_dc_cycle(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_wait_safeop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_safeop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_soe_conf_safeop(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_enter_op(ec_fsm_slave_config_t *);

void ec_fsm_slave_config_state_end(ec_fsm_slave_config_t *);
void ec_fsm_slave_config_state_error(ec_fsm_slave_config_t *);

void ec_fsm_slave_config_reconfigure(ec_fsm_slave_config_t *);

/****************************************************************************/

/** Constructor.
 */
void ec_fsm_slave_config_init(
        ec_fsm_slave_config_t *fsm, /**< slave state machine */
        ec_datagram_t *datagram, /**< datagram structure to use */
        ec_fsm_change_t *fsm_change, /**< State change state machine to use. */
        ec_fsm_coe_t *fsm_coe, /**< CoE state machine to use. */
        ec_fsm_soe_t *fsm_soe, /**< SoE state machine to use. */
        ec_fsm_pdo_t *fsm_pdo, /**< PDO configuration state machine to use. */
        ec_fsm_eoe_t *fsm_eoe /**< EoE state machine to use. */
        )
{
    ec_sdo_request_init(&fsm->request_copy);
    ec_soe_request_init(&fsm->soe_request_copy);

    fsm->datagram = datagram;
    fsm->fsm_change = fsm_change;
    fsm->fsm_coe = fsm_coe;
    fsm->fsm_soe = fsm_soe;
    fsm->fsm_pdo = fsm_pdo;
    fsm->fsm_eoe = fsm_eoe;

    fsm->wait_ms = 0;
}

/****************************************************************************/

/** Destructor.
 */
void ec_fsm_slave_config_clear(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
    ec_sdo_request_clear(&fsm->request_copy);
    ec_soe_request_clear(&fsm->soe_request_copy);
}

/****************************************************************************/

/** Start slave configuration state machine.
 */
void ec_fsm_slave_config_start(
        ec_fsm_slave_config_t *fsm, /**< slave state machine */
        ec_slave_t *slave /**< slave to configure */
        )
{
    fsm->slave = slave;
    fsm->state = ec_fsm_slave_config_state_start;
}

/****************************************************************************/

/**
 * \return false, if state machine has terminated
 */
int ec_fsm_slave_config_running(
        const ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
    return fsm->state != ec_fsm_slave_config_state_end
        && fsm->state != ec_fsm_slave_config_state_error;
}

/****************************************************************************/

/** Executes the current state of the state machine.
 *
 * If the state machine's datagram is not sent or received yet, the execution
 * of the state machine is delayed to the next cycle.
 *
 * \return false, if state machine has terminated
 */
int ec_fsm_slave_config_exec(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
    if (fsm->datagram->state == EC_DATAGRAM_SENT
        || fsm->datagram->state == EC_DATAGRAM_QUEUED) {
        // datagram was not sent or received yet.
        return ec_fsm_slave_config_running(fsm);
    }

    fsm->state(fsm);
    return ec_fsm_slave_config_running(fsm);
}

/****************************************************************************/

/**
 * \return true, if the state machine terminated gracefully
 */
int ec_fsm_slave_config_success(
        const ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
    return fsm->state == ec_fsm_slave_config_state_end;
}

/*****************************************************************************
 * Slave configuration state machine
 ****************************************************************************/

/** Slave configuration state: START.
 */
void ec_fsm_slave_config_state_start(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_start\n");
    EC_SLAVE_DBG(fsm->slave, 1, "Configuring...\n");
    ec_fsm_slave_config_enter_init(fsm);
}

/****************************************************************************/

/** Start state change to INIT.
 */
void ec_fsm_slave_config_enter_init(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_init\n");
    ec_fsm_change_start(fsm->fsm_change, fsm->slave, EC_SLAVE_STATE_INIT);
    ec_fsm_change_exec(fsm->fsm_change);
    fsm->state = ec_fsm_slave_config_state_init;
}

/****************************************************************************/

/** Slave configuration state: INIT.
 */
void ec_fsm_slave_config_state_init(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_init\n");
    ec_slave_t *slave = fsm->slave;
    ec_datagram_t *datagram = fsm->datagram;

    if (ec_fsm_change_exec(fsm->fsm_change)) return;

    if (!ec_fsm_change_success(fsm->fsm_change)) {
        if (!fsm->fsm_change->spontaneous_change)
            slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Now in INIT.\n");

    if (!slave->base_fmmu_count) { // skip FMMU configuration
        ec_fsm_slave_config_enter_clear_sync(fsm);
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Clearing FMMU configurations...\n");

    // clear FMMU configurations
    ec_datagram_fpwr(datagram, slave->station_address,
            0x0600, EC_FMMU_PAGE_SIZE * slave->base_fmmu_count);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_clear_fmmus;
}

/****************************************************************************/

/** Slave configuration state: CLEAR FMMU.
 */
void ec_fsm_slave_config_state_clear_fmmus(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_clear_fmmus\n");
    ec_datagram_t *datagram = fsm->datagram;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(fsm->slave, "Failed receive FMMU clearing datagram.\n");
        return;
    }

    if (datagram->working_counter != 1) {
        fsm->slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(fsm->slave, "Failed to clear FMMUs: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    ec_fsm_slave_config_enter_clear_sync(fsm);
}

/****************************************************************************/

/** Clear the sync manager configurations.
 */
void ec_fsm_slave_config_enter_clear_sync(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_clear_fmmus\n");
    ec_slave_t *slave = fsm->slave;
    ec_datagram_t *datagram = fsm->datagram;
    size_t sync_size;

    if (!slave->base_sync_count) {
        // no sync managers
        ec_fsm_slave_config_enter_dc_clear_assign(fsm);
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Clearing sync manager configurations...\n");

    sync_size = EC_SYNC_PAGE_SIZE * slave->base_sync_count;

    // clear sync manager configurations
    ec_datagram_fpwr(datagram, slave->station_address, 0x0800, sync_size);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_clear_sync;
}

/****************************************************************************/

/** Slave configuration state: CLEAR SYNC.
 */
void ec_fsm_slave_config_state_clear_sync(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_clear_sync\n");
    ec_datagram_t *datagram = fsm->datagram;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(fsm->slave, "Failed receive sync manager"
                " clearing datagram.\n");
        return;
    }

    if (datagram->working_counter != 1) {
        fsm->slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(fsm->slave,
                "Failed to clear sync manager configurations: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    ec_fsm_slave_config_enter_dc_clear_assign(fsm);
}

/****************************************************************************/

/** Clear the DC assignment.
 */
void ec_fsm_slave_config_enter_dc_clear_assign(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_clear_sync\n");
    ec_slave_t *slave = fsm->slave;
    ec_datagram_t *datagram = fsm->datagram;

    if (!slave->base_dc_supported || !slave->has_dc_system_time) {
        ec_fsm_slave_config_enter_mbox_sync(fsm);
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Clearing DC assignment...\n");

    ec_datagram_fpwr(datagram, slave->station_address, 0x0980, 2);
    ec_datagram_zero(datagram);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_dc_clear_assign;
}

/****************************************************************************/

/** Slave configuration state: CLEAR DC ASSIGN.
 */
void ec_fsm_slave_config_state_dc_clear_assign(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_dc_clear_assign\n");
    ec_datagram_t *datagram = fsm->datagram;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(fsm->slave, "Failed receive DC assignment"
                " clearing datagram.\n");
        return;
    }

    if (datagram->working_counter != 1) {
        // clearing the DC assignment does not succeed on simple slaves
        EC_SLAVE_DBG(fsm->slave, 1, "Failed to clear DC assignment: ");
        ec_datagram_print_wc_error(datagram);
    }

    ec_fsm_slave_config_enter_mbox_sync(fsm);
}

/****************************************************************************/

/** Check for mailbox sync managers to be configured.
 */
void ec_fsm_slave_config_enter_mbox_sync(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_dc_clear_assign\n");
    ec_slave_t *slave = fsm->slave;
    ec_datagram_t *datagram = fsm->datagram;
    unsigned int i;

    // slave is now in INIT
    if (slave->current_state == slave->requested_state) {
        fsm->state = ec_fsm_slave_config_state_end; // successful
        EC_SLAVE_DBG(slave, 1, "Finished configuration.\n");
        return;
    }

    if (!slave->sii.mailbox_protocols) {
        // no mailbox protocols supported
        EC_SLAVE_DBG(slave, 1, "Slave does not support"
                " mailbox communication.\n");
#ifdef EC_SII_ASSIGN
        ec_fsm_slave_config_enter_assign_pdi(fsm);
#else
        ec_fsm_slave_config_enter_boot_preop(fsm);
#endif
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Configuring mailbox sync managers...\n");

    if (slave->requested_state == EC_SLAVE_STATE_BOOT) {
        ec_sync_t sync;

        ec_datagram_fpwr(datagram, slave->station_address, 0x0800,
                EC_SYNC_PAGE_SIZE * 2);
        ec_datagram_zero(datagram);

        ec_sync_init(&sync, slave);
        sync.physical_start_address = slave->sii.boot_rx_mailbox_offset;
        sync.control_register = 0x26;
        sync.enable = 1;
        ec_sync_page(&sync, 0, slave->sii.boot_rx_mailbox_size,
                EC_DIR_INVALID, // use default direction
                0, // no PDO xfer
                datagram->data);
        slave->configured_rx_mailbox_offset =
            slave->sii.boot_rx_mailbox_offset;
        slave->configured_rx_mailbox_size =
            slave->sii.boot_rx_mailbox_size;

        ec_sync_init(&sync, slave);
        sync.physical_start_address = slave->sii.boot_tx_mailbox_offset;
        sync.control_register = 0x22;
        sync.enable = 1;
        ec_sync_page(&sync, 1, slave->sii.boot_tx_mailbox_size,
                EC_DIR_INVALID, // use default direction
                0, // no PDO xfer
                datagram->data + EC_SYNC_PAGE_SIZE);
        slave->configured_tx_mailbox_offset =
            slave->sii.boot_tx_mailbox_offset;
        slave->configured_tx_mailbox_size =
            slave->sii.boot_tx_mailbox_size;

    } else if (slave->sii.sync_count >= 2) { // mailbox configuration provided
        ec_datagram_fpwr(datagram, slave->station_address, 0x0800,
                EC_SYNC_PAGE_SIZE * slave->sii.sync_count);
        ec_datagram_zero(datagram);

        for (i = 0; i < 2; i++) {
            ec_sync_page(&slave->sii.syncs[i], i,
                    slave->sii.syncs[i].default_length,
                    NULL, // use default sync manager configuration
                    0, // no PDO xfer
                    datagram->data + EC_SYNC_PAGE_SIZE * i);
        }

        slave->configured_rx_mailbox_offset =
            slave->sii.syncs[0].physical_start_address;
        slave->configured_rx_mailbox_size =
            slave->sii.syncs[0].default_length;
        slave->configured_tx_mailbox_offset =
            slave->sii.syncs[1].physical_start_address;
        slave->configured_tx_mailbox_size =
            slave->sii.syncs[1].default_length;
    } else { // no mailbox sync manager configurations provided
        ec_sync_t sync;

        EC_SLAVE_DBG(slave, 1, "Slave does not provide"
                " mailbox sync manager configurations.\n");

        ec_datagram_fpwr(datagram, slave->station_address, 0x0800,
                EC_SYNC_PAGE_SIZE * 2);
        ec_datagram_zero(datagram);

        ec_sync_init(&sync, slave);
        sync.physical_start_address = slave->sii.std_rx_mailbox_offset;
        sync.control_register = 0x26;
        sync.enable = 1;
        ec_sync_page(&sync, 0, slave->sii.std_rx_mailbox_size,
                NULL, // use default sync manager configuration
                0, // no PDO xfer
                datagram->data);
        slave->configured_rx_mailbox_offset =
            slave->sii.std_rx_mailbox_offset;
        slave->configured_rx_mailbox_size =
            slave->sii.std_rx_mailbox_size;

        ec_sync_init(&sync, slave);
        sync.physical_start_address = slave->sii.std_tx_mailbox_offset;
        sync.control_register = 0x22;
        sync.enable = 1;
        ec_sync_page(&sync, 1, slave->sii.std_tx_mailbox_size,
                NULL, // use default sync manager configuration
                0, // no PDO xfer
                datagram->data + EC_SYNC_PAGE_SIZE);
        slave->configured_tx_mailbox_offset =
            slave->sii.std_tx_mailbox_offset;
        slave->configured_tx_mailbox_size =
            slave->sii.std_tx_mailbox_size;
    }

    fsm->take_time = 1;

    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_mbox_sync;
}

/****************************************************************************/

/** Slave configuration state: SYNC.
 *
 * \todo Timeout for response.
 */
void ec_fsm_slave_config_state_mbox_sync(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_mbox_sync\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive sync manager"
                " configuration datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (fsm->take_time) {
        fsm->take_time = 0;
        fsm->jiffies_start = datagram->jiffies_sent;
    }

    /* Because the sync manager configurations are cleared during the last
     * cycle, some slaves do not immediately respond to the mailbox sync
     * manager configuration datagram. Therefore, resend the datagram for
     * a certain time, if the slave does not respond.
     */
    if (datagram->working_counter == 0) {
        unsigned long diff = datagram->jiffies_received - fsm->jiffies_start;

        if (diff >= HZ) {
            slave->error_flag = 1;
            fsm->state = ec_fsm_slave_config_state_error;
            EC_SLAVE_ERR(slave, "Timeout while configuring"
                    " mailbox sync managers.\n");
            return;
        } else {
            EC_SLAVE_DBG(slave, 1, "Resending after %u ms...\n",
                    (unsigned int) diff * 1000 / HZ);
        }

        // send configuration datagram again
        fsm->retries = EC_FSM_RETRIES;
        return;
    }
    else if (datagram->working_counter != 1) {
        slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to set sync managers: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

#ifdef EC_SII_ASSIGN
    ec_fsm_slave_config_enter_assign_pdi(fsm);
#else
    ec_fsm_slave_config_enter_boot_preop(fsm);
#endif
}

/****************************************************************************/

#ifdef EC_SII_ASSIGN

/** Assign SII to PDI.
 */
void ec_fsm_slave_config_enter_assign_pdi(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_mbox_sync\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (fsm->slave->requested_state != EC_SLAVE_STATE_BOOT) {
        EC_SLAVE_DBG(slave, 1, "Assigning SII access to PDI.\n");

        ec_datagram_fpwr(datagram, slave->station_address, 0x0500, 0x01);
        EC_WRITE_U8(datagram->data, 0x01); // PDI
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_slave_config_state_assign_pdi;
    }
    else {
        ec_fsm_slave_config_enter_boot_preop(fsm);
    }
}

/****************************************************************************/

/** Slave configuration state: ASSIGN_PDI.
 */
void ec_fsm_slave_config_state_assign_pdi(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_assign_pdi\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        return;
    }

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        EC_SLAVE_WARN(slave, "Failed receive SII assignment datagram: ");
        ec_datagram_print_state(datagram);
        goto cont_preop;
    }

    if (datagram->working_counter != 1) {
        EC_SLAVE_WARN(slave, "Failed to assign SII to PDI: ");
        ec_datagram_print_wc_error(datagram);
    }

cont_preop:
    ec_fsm_slave_config_enter_boot_preop(fsm);
}

#endif

/****************************************************************************/

/** Request PREOP state.
 */
void ec_fsm_slave_config_enter_boot_preop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_assign_pdi\n");
    fsm->state = ec_fsm_slave_config_state_boot_preop;

    if (fsm->slave->requested_state != EC_SLAVE_STATE_BOOT) {
        ec_fsm_change_start(fsm->fsm_change,
                fsm->slave, EC_SLAVE_STATE_PREOP);
    } else { // BOOT
        ec_fsm_change_start(fsm->fsm_change,
                fsm->slave, EC_SLAVE_STATE_BOOT);
    }

    ec_fsm_change_exec(fsm->fsm_change); // execute immediately
}

/****************************************************************************/

/** Slave configuration state: BOOT/PREOP.
 */
void ec_fsm_slave_config_state_boot_preop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_boot_preop\n");
    ec_slave_t *slave = fsm->slave;
#ifdef EC_SII_ASSIGN
    int assign_to_pdi;
    ec_slave_config_t *config;
    ec_flag_t *flag;
#endif

    if (ec_fsm_change_exec(fsm->fsm_change)) {
        return;
    }

    if (!ec_fsm_change_success(fsm->fsm_change)) {
        if (!fsm->fsm_change->spontaneous_change)
            slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        return;
    }

    // slave is now in BOOT or PREOP
    slave->jiffies_preop = fsm->datagram->jiffies_received;

    EC_SLAVE_DBG(slave, 1, "Now in %s.\n",
            slave->requested_state != EC_SLAVE_STATE_BOOT ? "PREOP" : "BOOT");

#ifdef EC_SII_ASSIGN
    assign_to_pdi = 0;
    config = fsm->slave->config;
    if (config) {
        flag = ec_slave_config_find_flag(config, "AssignToPdi");
        if (flag) {
            assign_to_pdi = flag->value;
        }
    }

    if (assign_to_pdi) {
        EC_SLAVE_DBG(slave, 1, "Skipping SII assignment back to EtherCAT.\n");
        if (slave->current_state == slave->requested_state) {
            fsm->state = ec_fsm_slave_config_state_end; // successful
            EC_SLAVE_DBG(slave, 1, "Finished configuration.\n");
            return;
        }

        ec_fsm_slave_config_enter_sdo_conf(fsm);
    }
    else {
        EC_SLAVE_DBG(slave, 1, "Assigning SII access back to EtherCAT.\n");

        ec_datagram_fpwr(fsm->datagram, slave->station_address, 0x0500, 0x01);
        EC_WRITE_U8(fsm->datagram->data, 0x00); // EtherCAT
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_slave_config_state_assign_ethercat;
    }
#else
    if (slave->current_state == slave->requested_state) {
        fsm->state = ec_fsm_slave_config_state_end; // successful
        EC_SLAVE_DBG(slave, 1, "Finished configuration.\n");
        return;
    }

    ec_fsm_slave_config_enter_sdo_conf(fsm);
#endif
}

/****************************************************************************/

#ifdef EC_SII_ASSIGN

/** Slave configuration state: ASSIGN_ETHERCAT.
 */
void ec_fsm_slave_config_state_assign_ethercat(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_assign_ethercat\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--) {
        return;
    }

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        EC_SLAVE_WARN(slave, "Failed receive SII assignment datagram: ");
        ec_datagram_print_state(datagram);
        goto cont_sdo_conf;
    }

    if (datagram->working_counter != 1) {
        EC_SLAVE_WARN(slave, "Failed to assign SII back to EtherCAT: ");
        ec_datagram_print_wc_error(datagram);
    }

cont_sdo_conf:
    if (slave->current_state == slave->requested_state) {
        fsm->state = ec_fsm_slave_config_state_end; // successful
        EC_SLAVE_DBG(slave, 1, "Finished configuration.\n");
        return;
    }

    ec_fsm_slave_config_enter_sdo_conf(fsm);
}

#endif

/****************************************************************************/

/** Check for SDO configurations to be applied.
 */
void ec_fsm_slave_config_enter_sdo_conf(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_sdo_conf\n");
    ec_slave_t *slave = fsm->slave;

    if (!slave->config) {                          //当我用命令行进入op时，slave->config在这里被清空了？
        ec_fsm_slave_config_enter_pdo_sync(fsm);
        return;
    }

    // No CoE configuration to be applied?
    if (list_empty(&slave->config->sdo_configs)) { // skip SDO configuration
        ec_fsm_slave_config_enter_soe_conf_preop(fsm);
        return;
    }

    // start SDO configuration
    fsm->state = ec_fsm_slave_config_state_sdo_conf;
    fsm->request = list_entry(fsm->slave->config->sdo_configs.next,
            ec_sdo_request_t, list);
    ec_sdo_request_copy(&fsm->request_copy, fsm->request);
    ecrt_sdo_request_write(&fsm->request_copy);
    ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request_copy);
    ec_fsm_coe_exec(fsm->fsm_coe, fsm->datagram); // execute immediately
}

/****************************************************************************/

/** Slave configuration state: SDO_CONF.
 */
void ec_fsm_slave_config_state_sdo_conf(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_sdo_conf\n");
    if (ec_fsm_coe_exec(fsm->fsm_coe, fsm->datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(fsm->fsm_coe)) {
        EC_SLAVE_ERR(fsm->slave, "SDO configuration failed.\n");
        fsm->slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        return;
    }

    if (!fsm->slave->config) { // config removed in the meantime
        ec_fsm_slave_config_reconfigure(fsm);
        return;
    }

    // Another SDO to configure?
    if (fsm->request->list.next != &fsm->slave->config->sdo_configs) {
        fsm->request = list_entry(fsm->request->list.next,
                ec_sdo_request_t, list);
        ec_sdo_request_copy(&fsm->request_copy, fsm->request);
        ecrt_sdo_request_write(&fsm->request_copy);
        ec_fsm_coe_transfer(fsm->fsm_coe, fsm->slave, &fsm->request_copy);
        ec_fsm_coe_exec(fsm->fsm_coe, fsm->datagram); // execute immediately
        return;
    }

    // All SDOs are now configured.
    ec_fsm_slave_config_enter_soe_conf_preop(fsm);
}

/****************************************************************************/

/** Check for SoE configurations to be applied.
 */
void ec_fsm_slave_config_enter_soe_conf_preop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_soe_conf_preop\n");
    ec_slave_t *slave = fsm->slave;
    ec_soe_request_t *req;

    if (!slave->config) {
        ec_fsm_slave_config_enter_pdo_sync(fsm);
        return;
    }

    list_for_each_entry(req, &slave->config->soe_configs, list) {
        if (req->al_state == EC_AL_STATE_PREOP) {
            // start SoE configuration
            fsm->state = ec_fsm_slave_config_state_soe_conf_preop;
            fsm->soe_request = req;
            ec_soe_request_copy(&fsm->soe_request_copy, fsm->soe_request);
            ec_soe_request_write(&fsm->soe_request_copy);
            ec_fsm_soe_transfer(fsm->fsm_soe, fsm->slave,
                    &fsm->soe_request_copy);
            ec_fsm_soe_exec(fsm->fsm_soe, fsm->datagram);
            return;
        }
    }

    // No SoE configuration to be applied in PREOP
    ec_fsm_slave_config_enter_eoe_ip_param(fsm);
}

/****************************************************************************/

/** Slave configuration state: SOE_CONF.
 */
void ec_fsm_slave_config_state_soe_conf_preop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_soe_conf_preop\n");
    ec_slave_t *slave = fsm->slave;

    if (ec_fsm_soe_exec(fsm->fsm_soe, fsm->datagram)) {
        return;
    }

    if (!ec_fsm_soe_success(fsm->fsm_soe)) {
        EC_SLAVE_ERR(slave, "SoE configuration failed.\n");
        fsm->slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        return;
    }

    if (!fsm->slave->config) { // config removed in the meantime
        ec_fsm_slave_config_reconfigure(fsm);
        return;
    }

    // Another IDN to configure in PREOP?
    while (fsm->soe_request->list.next != &fsm->slave->config->soe_configs) {
        fsm->soe_request = list_entry(fsm->soe_request->list.next,
                ec_soe_request_t, list);
        if (fsm->soe_request->al_state == EC_AL_STATE_PREOP) {
            ec_soe_request_copy(&fsm->soe_request_copy, fsm->soe_request);
            ec_soe_request_write(&fsm->soe_request_copy);
            ec_fsm_soe_transfer(fsm->fsm_soe, fsm->slave,
                    &fsm->soe_request_copy);
            ec_fsm_soe_exec(fsm->fsm_soe, fsm->datagram);
            return;
        }
    }

    // All PREOP IDNs are now configured.
    ec_fsm_slave_config_enter_eoe_ip_param(fsm);
}

/****************************************************************************/

/** EOE_IP_PARAM entry function.
 */
void ec_fsm_slave_config_enter_eoe_ip_param(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_eoe_ip_param\n");
#ifdef EC_EOE
    ec_slave_t *slave = fsm->slave;
    ec_eoe_request_t *request = &slave->config->eoe_ip_param_request;

    if (ec_eoe_request_valid(request)) {
        EC_SLAVE_DBG(slave, 1, "Setting EoE IP parameters...\n");

        // Start EoE command
        fsm->state = ec_fsm_slave_config_state_eoe_ip_param;
        ec_fsm_eoe_set_ip_param(fsm->fsm_eoe, slave, request);
        ec_fsm_eoe_exec(fsm->fsm_eoe, fsm->datagram); // execute immediately
        return;
    }
#endif

    ec_fsm_slave_config_enter_pdo_conf(fsm);
}

/****************************************************************************/

/** Slave configuration state: EOE_IP_PARAM.
 */
void ec_fsm_slave_config_state_eoe_ip_param(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_eoe_ip_param\n");
#ifdef EC_EOE
    ec_slave_t *slave = fsm->slave;

    if (ec_fsm_eoe_exec(fsm->fsm_eoe, fsm->datagram)) {
        return;
    }

    if (ec_fsm_eoe_success(fsm->fsm_eoe)) {
        EC_SLAVE_DBG(slave, 1, "Finished setting EoE IP parameters.\n");
    }
    else {
        EC_SLAVE_ERR(slave, "Failed to set EoE IP parameters.\n");
    }
#endif
    ec_fsm_slave_config_enter_pdo_conf(fsm);
}

/****************************************************************************/

/** PDO_CONF entry function.
 */
void ec_fsm_slave_config_enter_pdo_conf(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_pdo_conf\n");
    // Start configuring PDOs
    ec_fsm_pdo_start_configuration(fsm->fsm_pdo, fsm->slave);
    fsm->state = ec_fsm_slave_config_state_pdo_conf;
    fsm->state(fsm); // execute immediately
}

/****************************************************************************/

/** Slave configuration state: PDO_CONF.
 */
void ec_fsm_slave_config_state_pdo_conf(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_pdo_conf\n");
    // TODO check for config here

    if (ec_fsm_pdo_exec(fsm->fsm_pdo, fsm->datagram)) {
        return;
    }

    if (!fsm->slave->config) { // config removed in the meantime
        ec_fsm_slave_config_reconfigure(fsm);
        return;
    }

    if (!ec_fsm_pdo_success(fsm->fsm_pdo)) {
        EC_SLAVE_WARN(fsm->slave, "PDO configuration failed.\n");
    }

    ec_fsm_slave_config_enter_watchdog_divider(fsm);
}

/****************************************************************************/

/** WATCHDOG_DIVIDER entry function.
 */
void ec_fsm_slave_config_enter_watchdog_divider(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
    printk("config->watchdog_divider = 1000\n");

    ec_slave_t *slave = fsm->slave;
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_config_t *config = slave->config;

//    config->watchdog_divider = 1000;

    if (config && config->watchdog_divider) {
        EC_SLAVE_DBG(slave, 1, "Setting watchdog divider to %u.\n",
                config->watchdog_divider);
        printk("config->watchdog_divider = 1000\n");
        ec_datagram_fpwr(datagram, slave->station_address, 0x0400, 2);
        EC_WRITE_U16(datagram->data, config->watchdog_divider);
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_slave_config_state_watchdog_divider;
    } else {
        ec_fsm_slave_config_enter_watchdog(fsm);
    }
}

/****************************************************************************/

/** Slave configuration state: WATCHDOG_DIVIDER.
 */
void ec_fsm_slave_config_state_watchdog_divider(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive watchdog divider"
                " configuration datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        slave->error_flag = 1;
        EC_SLAVE_WARN(slave, "Failed to set watchdog divider: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    ec_fsm_slave_config_enter_watchdog(fsm);
}

/****************************************************************************/

/** WATCHDOG entry function
 */
void ec_fsm_slave_config_enter_watchdog(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
    printk("config->watchdog_intervals = 250\n");

    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;
    ec_slave_config_t *config = slave->config;

//    config->watchdog_intervals = 250;

    if (config && config->watchdog_intervals) {
        EC_SLAVE_DBG(slave, 1, "Setting process data"
                " watchdog intervals to %u.\n", config->watchdog_intervals);
        printk("config->watchdog_intervals = 250\n");
        ec_datagram_fpwr(datagram, slave->station_address, 0x0420, 2);
        EC_WRITE_U16(datagram->data, config->watchdog_intervals);

        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_slave_config_state_watchdog;
    } else {
        ec_fsm_slave_config_enter_pdo_sync(fsm);
    }
}

/****************************************************************************/

/** Slave configuration state: WATCHDOG.
 */

void ec_fsm_slave_config_state_watchdog(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive sync manager"
                " watchdog configuration datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        EC_SLAVE_WARN(slave, "Failed to set process data"
                " watchdog intervals: ");
        ec_datagram_print_wc_error(datagram);
    }

    ec_fsm_slave_config_enter_pdo_sync(fsm);
}

/****************************************************************************/

/** Check for PDO sync managers to be configured.
 */
void ec_fsm_slave_config_enter_pdo_sync(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_pdo_sync\n");
    ec_slave_t *slave = fsm->slave;
    ec_datagram_t *datagram = fsm->datagram;
    unsigned int i, j, offset, num_pdo_syncs;
    uint8_t sync_index;
    const ec_sync_t *sync;
    uint16_t size;

    if (slave->sii.mailbox_protocols) {
        offset = 2; // slave has mailboxes
    } else {
        offset = 0;
    }

    if (slave->sii.sync_count <= offset) {
        // no PDO sync managers to configure
        ec_fsm_slave_config_enter_fmmu(fsm);
        return;
    }

    num_pdo_syncs = slave->sii.sync_count - offset;

    // configure sync managers for process data
    ec_datagram_fpwr(datagram, slave->station_address,
            0x0800 + EC_SYNC_PAGE_SIZE * offset,
            EC_SYNC_PAGE_SIZE * num_pdo_syncs);
    ec_datagram_zero(datagram);

    for (i = 0; i < num_pdo_syncs; i++) {
        const ec_sync_config_t *sync_config;
        uint8_t pdo_xfer = 0;
        sync_index = i + offset;
        sync = &slave->sii.syncs[sync_index];

        if (slave->config) {
            const ec_slave_config_t *sc = slave->config;
            sync_config = &sc->sync_configs[sync_index];
            size = ec_pdo_list_total_size(&sync_config->pdos);

            // determine, if PDOs shall be transferred via this SM
            // inthat case, enable sync manager in every case
            for (j = 0; j < sc->used_fmmus; j++) {
                if (sc->fmmu_configs[j].sync_index == sync_index) {
                    pdo_xfer = 1;
                    break;
                }
            }

        } else {
            sync_config = NULL;
            size = sync->default_length;
        }

        ec_sync_page(sync, sync_index, size, sync_config, pdo_xfer,
                datagram->data + EC_SYNC_PAGE_SIZE * i);
    }

    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_pdo_sync;
}

/****************************************************************************/

/** Configure PDO sync managers.
 */
void ec_fsm_slave_config_state_pdo_sync(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_pdo_sync\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive process data sync"
                " manager configuration datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to set process data sync managers: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    ec_fsm_slave_config_enter_fmmu(fsm);
}

/****************************************************************************/

/** Check for FMMUs to be configured.
 */
void ec_fsm_slave_config_enter_fmmu(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_fmmu\n");
    ec_slave_t *slave = fsm->slave;
    ec_datagram_t *datagram = fsm->datagram;
    unsigned int i;
    const ec_fmmu_config_t *fmmu;
    const ec_sync_t *sync;

    if (!slave->config) {
        ec_fsm_slave_config_enter_wait_safeop(fsm);
        return;
    }

    if (slave->base_fmmu_count < slave->config->used_fmmus) {
        slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Slave has less FMMUs (%u)"
                " than requested (%u).\n", slave->base_fmmu_count,
                slave->config->used_fmmus);
        return;
    }

    if (!slave->base_fmmu_count) { // skip FMMU configuration
        ec_fsm_slave_config_enter_dc_cycle(fsm);
        return;
    }

    // configure FMMUs
    ec_datagram_fpwr(datagram, slave->station_address,
                     0x0600, EC_FMMU_PAGE_SIZE * slave->base_fmmu_count);
    ec_datagram_zero(datagram);
    for (i = 0; i < slave->config->used_fmmus; i++) {
        fmmu = &slave->config->fmmu_configs[i];
        if (!(sync = ec_slave_get_sync(slave, fmmu->sync_index))) {
            slave->error_flag = 1;
            fsm->state = ec_fsm_slave_config_state_error;
            EC_SLAVE_ERR(slave, "Failed to determine PDO sync manager"
                    " for FMMU!\n");
            return;
        }
        ec_fmmu_config_page(fmmu, sync,
                datagram->data + EC_FMMU_PAGE_SIZE * i);
    }

    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_fmmu;
}

/****************************************************************************/

/** Slave configuration state: FMMU.
 */
void ec_fsm_slave_config_state_fmmu(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_fmmu\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive FMMUs datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to set FMMUs: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    ec_fsm_slave_config_enter_dc_cycle(fsm);
}

/****************************************************************************/

/** Check for DC to be configured.
 */
void ec_fsm_slave_config_enter_dc_cycle(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_dc_cycle\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;
    ec_slave_config_t *config = slave->config;

    if (!config) { // config removed in the meantime
        ec_fsm_slave_config_reconfigure(fsm);
        return;
    }

    if (config->dc_assign_activate) {
        if (!slave->base_dc_supported || !slave->has_dc_system_time) {
            EC_SLAVE_WARN(slave, "Slave seems not to support"
                    " distributed clocks!\n");
        }

        EC_SLAVE_DBG(slave, 1, "Setting DC cycle times to %u / %u.\n",
                config->dc_sync[0].cycle_time, config->dc_sync[1].cycle_time);

        // set DC cycle times
        ec_datagram_fpwr(datagram, slave->station_address, 0x09A0, 8);
        EC_WRITE_U32(datagram->data, config->dc_sync[0].cycle_time);
        EC_WRITE_U32(datagram->data + 4, config->dc_sync[1].cycle_time);
        fsm->retries = EC_FSM_RETRIES;
        fsm->state = ec_fsm_slave_config_state_dc_cycle;
    } else {
        // DC are unused
        ec_fsm_slave_config_enter_wait_safeop(fsm);
    }
}

/****************************************************************************/

/** Slave configuration state: DC CYCLE.
 */
void ec_fsm_slave_config_state_dc_cycle(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_dc_cycle\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;
    ec_slave_config_t *config = slave->config;

    if (!config) { // config removed in the meantime
        ec_fsm_slave_config_reconfigure(fsm);
        return;
    }

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive DC cycle times datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to set DC cycle times: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Checking for synchrony.\n");

    fsm->jiffies_start = jiffies;
    ec_datagram_fprd(datagram, slave->station_address, 0x092c, 4);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_dc_sync_check;
}

/****************************************************************************/

/** Slave configuration state: DC SYNC CHECK.
 */
void ec_fsm_slave_config_state_dc_sync_check(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_dc_sync_check\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;
    ec_master_t *master = slave->master;
    ec_slave_config_t *config = slave->config;
    uint32_t abs_sync_diff;
    unsigned long diff_ms;
    ec_sync_signal_t *sync0 = &config->dc_sync[0];
    ec_sync_signal_t *sync1 = &config->dc_sync[1];
    u64 start_time;

    if (!config) { // config removed in the meantime
        ec_fsm_slave_config_reconfigure(fsm);
        return;
    }

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive DC sync check datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to check DC synchrony: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    abs_sync_diff = EC_READ_U32(datagram->data) & 0x7fffffff;
    diff_ms = (datagram->jiffies_received - fsm->jiffies_start) * 1000 / HZ;

    if (abs_sync_diff > EC_DC_MAX_SYNC_DIFF_NS) {

        if (diff_ms >= EC_DC_SYNC_WAIT_MS) {
            EC_SLAVE_WARN(slave, "Slave did not sync after %lu ms.\n",
                    diff_ms);
        } else {
            EC_SLAVE_DBG(slave, 1, "Sync after %4lu ms: %10u ns\n",
                    diff_ms, abs_sync_diff);

            // check synchrony again
            ec_datagram_fprd(datagram, slave->station_address, 0x092c, 4);
            fsm->retries = EC_FSM_RETRIES;
            return;
        }
    } else {
        EC_SLAVE_DBG(slave, 1, "%u ns difference after %lu ms.\n",
                abs_sync_diff, diff_ms);
    }

    // set DC start time (roughly in the future, not in-phase)
    start_time = master->app_time + EC_DC_START_OFFSET; // now + X ns

    if (sync0->cycle_time) {
        // find correct phase
        if (master->dc_ref_time) {
            u64 diff, start;
            u32 remainder, cycle;

            diff = start_time - master->dc_ref_time;
            cycle = sync0->cycle_time + sync1->cycle_time;
            remainder = do_div(diff, cycle);

            start = start_time + cycle - remainder + sync0->shift_time;

            EC_SLAVE_DBG(slave, 1, "   ref_time=%llu\n", master->dc_ref_time);
            EC_SLAVE_DBG(slave, 1, "   app_time=%llu\n", master->app_time);
            EC_SLAVE_DBG(slave, 1, " start_time=%llu\n", start_time);
            EC_SLAVE_DBG(slave, 1, "      cycle=%u\n", cycle);
            EC_SLAVE_DBG(slave, 1, " shift_time=%i\n", sync0->shift_time);
            EC_SLAVE_DBG(slave, 1, "  remainder=%u\n", remainder);
            EC_SLAVE_DBG(slave, 1, "       start=%llu\n", start);
            start_time = start;
        } else {
            EC_SLAVE_WARN(slave, "No application time supplied."
                    " Cyclic start time will not be in phase.\n");
        }
    }

    EC_SLAVE_DBG(slave, 1, "Setting DC cyclic operation"
            " start time to %llu.\n", start_time);

    ec_datagram_fpwr(datagram, slave->station_address, 0x0990, 8);
    EC_WRITE_U64(datagram->data, start_time);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_dc_start;
}

/****************************************************************************/

/** Slave configuration state: DC START.
 */
void ec_fsm_slave_config_state_dc_start(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_dc_start\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;
    ec_slave_config_t *config = slave->config;

    if (!config) { // config removed in the meantime
        ec_fsm_slave_config_reconfigure(fsm);
        return;
    }

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive DC start time datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to set DC start time: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Setting DC AssignActivate to 0x%04x.\n",
            config->dc_assign_activate);

    // assign sync unit to EtherCAT or PDI
    ec_datagram_fpwr(datagram, slave->station_address, 0x0980, 2);
    EC_WRITE_U16(datagram->data, config->dc_assign_activate);
    fsm->retries = EC_FSM_RETRIES;
    fsm->state = ec_fsm_slave_config_state_dc_assign;
}

/****************************************************************************/

/** Slave configuration state: DC ASSIGN.
 */
void ec_fsm_slave_config_state_dc_assign(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_dc_assign\n");
    ec_datagram_t *datagram = fsm->datagram;
    ec_slave_t *slave = fsm->slave;

    if (datagram->state == EC_DATAGRAM_TIMED_OUT && fsm->retries--)
        return;

    if (datagram->state != EC_DATAGRAM_RECEIVED) {
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to receive DC activation datagram: ");
        ec_datagram_print_state(datagram);
        return;
    }

    if (datagram->working_counter != 1) {
        slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        EC_SLAVE_ERR(slave, "Failed to activate DC: ");
        ec_datagram_print_wc_error(datagram);
        return;
    }

    ec_fsm_slave_config_enter_wait_safeop(fsm);
}

/****************************************************************************/

/** Wait before SAFEOP transition.
 *
 * The feature flag WaitBeforeSAFEOPms can be used to add a wait time before
 * going to SAFEOP. This can be used as a workaround for slaves that need some
 * extra time for initialisation.
 */
void ec_fsm_slave_config_enter_wait_safeop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_wait_safeop\n");
    ec_slave_config_t *config = fsm->slave->config;
    fsm->wait_ms = 0UL;
    if (config) {
        ec_flag_t *flag = ec_slave_config_find_flag(config,
                "WaitBeforeSAFEOPms");
        if (flag && flag->value > 0) {
            fsm->wait_ms = (unsigned long) flag->value;
        }
    }

    if (fsm->wait_ms > 0) {
        fsm->state = ec_fsm_slave_config_state_wait_safeop;

        /* dummy read */
        ec_datagram_fprd(fsm->datagram, fsm->slave->station_address,
                0x0600, 1);

        fsm->jiffies_start = jiffies;
    }
    else {
        ec_fsm_slave_config_enter_safeop(fsm);
    }
}

/****************************************************************************/

/** Slave configuration state: WAIT SAFEOP.
 */
void ec_fsm_slave_config_state_wait_safeop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_wait_safeop\n");
    unsigned long diff = jiffies - fsm->jiffies_start;

    if (diff * 1000 / HZ < fsm->wait_ms) {
        return;
    }

    ec_fsm_slave_config_enter_safeop(fsm);
}

/****************************************************************************/

/** Request SAFEOP state.
 */
void ec_fsm_slave_config_enter_safeop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_safeop\n");
    fsm->state = ec_fsm_slave_config_state_safeop;
    ec_fsm_change_start(fsm->fsm_change, fsm->slave, EC_SLAVE_STATE_SAFEOP);
    ec_fsm_change_exec(fsm->fsm_change); // execute immediately
}

/****************************************************************************/

/** Slave configuration state: SAFEOP.
 */
void ec_fsm_slave_config_state_safeop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_safeop\n");
    ec_slave_t *slave = fsm->slave;

    if (ec_fsm_change_exec(fsm->fsm_change)) return;

    if (!ec_fsm_change_success(fsm->fsm_change)) {
        if (!fsm->fsm_change->spontaneous_change)
            fsm->slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        return;
    }

    // slave is now in SAFEOP

    EC_SLAVE_DBG(slave, 1, "Now in SAFEOP.\n");

    if (fsm->slave->current_state == fsm->slave->requested_state) {
        fsm->state = ec_fsm_slave_config_state_end; // successful
        EC_SLAVE_DBG(slave, 1, "Finished configuration.\n");
        return;
    }

    ec_fsm_slave_config_enter_soe_conf_safeop(fsm);
}

/****************************************************************************/

/** Check for SoE configurations to be applied in SAFEOP.
 */
void ec_fsm_slave_config_enter_soe_conf_safeop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_soe_conf_safeop\n");
    ec_slave_t *slave = fsm->slave;
    ec_soe_request_t *req;

    if (!slave->config) {
        ec_fsm_slave_config_enter_op(fsm);
        return;
    }

    list_for_each_entry(req, &slave->config->soe_configs, list) {
        if (req->al_state == EC_AL_STATE_SAFEOP) {
            // start SoE configuration
            fsm->state = ec_fsm_slave_config_state_soe_conf_safeop;
            fsm->soe_request = req;
            ec_soe_request_copy(&fsm->soe_request_copy, fsm->soe_request);
            ec_soe_request_write(&fsm->soe_request_copy);
            ec_fsm_soe_transfer(fsm->fsm_soe, fsm->slave,
                    &fsm->soe_request_copy);
            ec_fsm_soe_exec(fsm->fsm_soe, fsm->datagram);
            return;
        }
    }

    // No SoE configuration to be applied in SAFEOP
    ec_fsm_slave_config_enter_op(fsm);
}

/****************************************************************************/

/** Slave configuration state: SOE_CONF.
 */
void ec_fsm_slave_config_state_soe_conf_safeop(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_soe_conf_safeop\n");
    ec_slave_t *slave = fsm->slave;

    if (ec_fsm_soe_exec(fsm->fsm_soe, fsm->datagram)) {
        return;
    }

    if (!ec_fsm_soe_success(fsm->fsm_soe)) {
        EC_SLAVE_ERR(slave, "SoE configuration failed.\n");
        fsm->slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        return;
    }

    if (!fsm->slave->config) { // config removed in the meantime
        ec_fsm_slave_config_reconfigure(fsm);
        return;
    }

    // Another IDN to configure in SAFEOP?
    while (fsm->soe_request->list.next != &fsm->slave->config->soe_configs) {
        fsm->soe_request = list_entry(fsm->soe_request->list.next,
                ec_soe_request_t, list);
        if (fsm->soe_request->al_state == EC_AL_STATE_SAFEOP) {
            ec_soe_request_copy(&fsm->soe_request_copy, fsm->soe_request);
            ec_soe_request_write(&fsm->soe_request_copy);
            ec_fsm_soe_transfer(fsm->fsm_soe, fsm->slave,
                    &fsm->soe_request_copy);
            ec_fsm_soe_exec(fsm->fsm_soe, fsm->datagram);
            return;
        }
    }

    // All SAFEOP IDNs are now configured.
    ec_fsm_slave_config_enter_op(fsm);
}

/****************************************************************************/

/** Bring slave to OP.
 */
void ec_fsm_slave_config_enter_op(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_enter_op\n");
    // set state to OP
    fsm->state = ec_fsm_slave_config_state_op;
    ec_fsm_change_start(fsm->fsm_change, fsm->slave, EC_SLAVE_STATE_OP);
    ec_fsm_change_exec(fsm->fsm_change); // execute immediately
}

/****************************************************************************/

/** Slave configuration state: OP
 */
void ec_fsm_slave_config_state_op(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_op\n");
    ec_slave_t *slave = fsm->slave;

    if (ec_fsm_change_exec(fsm->fsm_change)) return;

    if (!ec_fsm_change_success(fsm->fsm_change)) {
        if (!fsm->fsm_change->spontaneous_change)
            slave->error_flag = 1;
        fsm->state = ec_fsm_slave_config_state_error;
        return;
    }

    // slave is now in OP

    EC_SLAVE_DBG(slave, 1, "Now in OP. Finished configuration.\n");

    fsm->state = ec_fsm_slave_config_state_end; // successful
}

/****************************************************************************/

/** Reconfigure the slave starting at INIT.
 */
void ec_fsm_slave_config_reconfigure(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_reconfigure\n");
    EC_SLAVE_DBG(fsm->slave, 1, "Slave configuration detached during "
            "configuration. Reconfiguring.");

    ec_fsm_slave_config_enter_init(fsm); // reconfigure
}

/*****************************************************************************
 *  Common state functions
 ****************************************************************************/

/** State: ERROR.
 */
void ec_fsm_slave_config_state_error(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_error\n");
}

/****************************************************************************/

/** State: END.
 */
void ec_fsm_slave_config_state_end(
        ec_fsm_slave_config_t *fsm /**< slave state machine */
        )
{
//    printk("ec_fsm_slave_config_state_end\n");
}

/****************************************************************************/

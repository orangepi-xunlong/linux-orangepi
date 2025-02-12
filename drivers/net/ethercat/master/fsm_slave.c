/*****************************************************************************
 *
 *  Copyright (C) 2006-2023  Florian Pose, Ingenieurgemeinschaft IgH
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
 * EtherCAT slave (SDO) state machine.
 */

/****************************************************************************/

#include "globals.h"
#include "master.h"
#include "mailbox.h"
#include "slave_config.h"

#include "fsm_slave.h"

/****************************************************************************/

void ec_fsm_slave_state_idle(ec_fsm_slave_t *, ec_datagram_t *);
void ec_fsm_slave_state_ready(ec_fsm_slave_t *, ec_datagram_t *);
int ec_fsm_slave_action_process_sdo(ec_fsm_slave_t *, ec_datagram_t *);
void ec_fsm_slave_state_sdo_request(ec_fsm_slave_t *, ec_datagram_t *);
int ec_fsm_slave_action_process_reg(ec_fsm_slave_t *, ec_datagram_t *);
void ec_fsm_slave_state_reg_request(ec_fsm_slave_t *, ec_datagram_t *);
int ec_fsm_slave_action_process_foe(ec_fsm_slave_t *, ec_datagram_t *);
void ec_fsm_slave_state_foe_request(ec_fsm_slave_t *, ec_datagram_t *);
int ec_fsm_slave_action_process_soe(ec_fsm_slave_t *, ec_datagram_t *);
void ec_fsm_slave_state_soe_request(ec_fsm_slave_t *, ec_datagram_t *);
#ifdef EC_EOE
int ec_fsm_slave_action_process_eoe(ec_fsm_slave_t *, ec_datagram_t *);
void ec_fsm_slave_state_eoe_request(ec_fsm_slave_t *, ec_datagram_t *);
#endif

/****************************************************************************/

/** Constructor.
 */
void ec_fsm_slave_init(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_slave_t *slave /**< EtherCAT slave. */
        )
{
    fsm->slave = slave;
    INIT_LIST_HEAD(&fsm->list); // mark as unlisted

    fsm->state = ec_fsm_slave_state_idle;
    fsm->datagram = NULL;
    fsm->sdo_request = NULL;
    fsm->reg_request = NULL;
    fsm->foe_request = NULL;
    fsm->soe_request = NULL;
#ifdef EC_EOE
    fsm->eoe_request = NULL;
#endif

    // Init sub-state-machines
    ec_fsm_coe_init(&fsm->fsm_coe);
    ec_fsm_foe_init(&fsm->fsm_foe);
    ec_fsm_soe_init(&fsm->fsm_soe);
#ifdef EC_EOE
    ec_fsm_eoe_init(&fsm->fsm_eoe);
#endif
}

/****************************************************************************/

/** Destructor.
 */
void ec_fsm_slave_clear(
        ec_fsm_slave_t *fsm /**< Master state machine. */
        )
{
    // signal requests that are currently in operation

    if (fsm->sdo_request) {
        fsm->sdo_request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&fsm->slave->master->request_queue);
    }

    if (fsm->reg_request) {
        fsm->reg_request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&fsm->slave->master->request_queue);
    }

    if (fsm->foe_request) {
        fsm->foe_request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&fsm->slave->master->request_queue);
    }

    if (fsm->soe_request) {
        fsm->soe_request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&fsm->slave->master->request_queue);
    }

#ifdef EC_EOE
    if (fsm->eoe_request) {
        fsm->soe_request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&fsm->slave->master->request_queue);
    }
#endif

    // clear sub-state machines
    ec_fsm_coe_clear(&fsm->fsm_coe);
    ec_fsm_foe_clear(&fsm->fsm_foe);
    ec_fsm_soe_clear(&fsm->fsm_soe);
#ifdef EC_EOE
    ec_fsm_eoe_clear(&fsm->fsm_eoe);
#endif
}

/****************************************************************************/

/** Executes the current state of the state machine.
 *
 * \return 1 if \a datagram was used, else 0.
 */
int ec_fsm_slave_exec(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< New datagram to use. */
        )
{
    int datagram_used;

    fsm->state(fsm, datagram);

    datagram_used = fsm->state != ec_fsm_slave_state_idle &&
        fsm->state != ec_fsm_slave_state_ready;

    if (datagram_used) {
        fsm->datagram = datagram;
    } else {
        fsm->datagram = NULL;
    }

    return datagram_used;
}

/****************************************************************************/

/** Sets the current state of the state machine to READY
 */
void ec_fsm_slave_set_ready(
        ec_fsm_slave_t *fsm /**< Slave state machine. */
        )
{
    if (fsm->state == ec_fsm_slave_state_idle) {
        EC_SLAVE_DBG(fsm->slave, 1, "Ready for requests.\n");
        fsm->state = ec_fsm_slave_state_ready;
    }
}

/****************************************************************************/

/** Returns, if the FSM is currently not busy and ready to execute.
 *
 * \return Non-zero if ready.
 */
int ec_fsm_slave_is_ready(
        const ec_fsm_slave_t *fsm /**< Slave state machine. */
        )
{
    return fsm->state == ec_fsm_slave_state_ready;
}

/*****************************************************************************
 * Slave state machine
 ****************************************************************************/

/** Slave state: IDLE.
 */
void ec_fsm_slave_state_idle(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    // do nothing
}

/****************************************************************************/

/** Slave state: READY.
 */
void ec_fsm_slave_state_ready(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    // Check for pending external SDO requests
    if (ec_fsm_slave_action_process_sdo(fsm, datagram)) {
        return;
    }

    // Check for pending external register requests
    if (ec_fsm_slave_action_process_reg(fsm, datagram)) {
        return;
    }

    // Check for pending FoE requests
    if (ec_fsm_slave_action_process_foe(fsm, datagram)) {
        return;
    }

    // Check for pending SoE requests
    if (ec_fsm_slave_action_process_soe(fsm, datagram)) {
        return;
    }

#ifdef EC_EOE
    // Check for pending EoE IP parameter requests
    if (ec_fsm_slave_action_process_eoe(fsm, datagram)) {
        return;
    }
#endif
}

/****************************************************************************/

/** Check for pending SDO requests and process one.
 *
 * \return non-zero, if an SDO request is processed.
 */
int ec_fsm_slave_action_process_sdo(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_sdo_request_t *request;

    if (list_empty(&slave->sdo_requests)) {
        return 0;
    }

    // take the first request to be processed
    request = list_entry(slave->sdo_requests.next, ec_sdo_request_t, list);
    list_del_init(&request->list); // dequeue

    if (slave->current_state & EC_SLAVE_STATE_ACK_ERR) {
        EC_SLAVE_WARN(slave, "Aborting SDO request,"
                " slave has error flag set.\n");
        request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->state = ec_fsm_slave_state_idle;
        return 0;
    }

    if (slave->current_state == EC_SLAVE_STATE_INIT) {
        EC_SLAVE_WARN(slave, "Aborting SDO request, slave is in INIT.\n");
        request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->state = ec_fsm_slave_state_idle;
        return 0;
    }

    fsm->sdo_request = request;
    request->state = EC_INT_REQUEST_BUSY;

    // Found pending SDO request. Execute it!
    EC_SLAVE_DBG(slave, 1, "Processing SDO request...\n");

    // Start SDO transfer
    fsm->state = ec_fsm_slave_state_sdo_request;
    ec_fsm_coe_transfer(&fsm->fsm_coe, slave, request);
    ec_fsm_coe_exec(&fsm->fsm_coe, datagram); // execute immediately
    return 1;
}

/****************************************************************************/

/** Slave state: SDO_REQUEST.
 */
void ec_fsm_slave_state_sdo_request(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_sdo_request_t *request = fsm->sdo_request;

    if (ec_fsm_coe_exec(&fsm->fsm_coe, datagram)) {
        return;
    }

    if (!ec_fsm_coe_success(&fsm->fsm_coe)) {
        EC_SLAVE_ERR(slave, "Failed to process SDO request.\n");
        request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->sdo_request = NULL;
        fsm->state = ec_fsm_slave_state_ready;
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Finished SDO request.\n");

    // SDO request finished
    request->state = EC_INT_REQUEST_SUCCESS;
    wake_up_all(&slave->master->request_queue);
    fsm->sdo_request = NULL;
    fsm->state = ec_fsm_slave_state_ready;
}

/****************************************************************************/

/** Check for pending register requests and process one.
 *
 * \return non-zero, if a register request is processed.
 */
int ec_fsm_slave_action_process_reg(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_reg_request_t *reg;

    fsm->reg_request = NULL;

    if (slave->config) {
        // search the first internal register request to be processed
        list_for_each_entry(reg, &slave->config->reg_requests, list) {
            if (reg->state == EC_INT_REQUEST_QUEUED) {
                fsm->reg_request = reg;
                break;
            }
        }
    }

    if (!fsm->reg_request && !list_empty(&slave->reg_requests)) {
        // take the first external request to be processed
        fsm->reg_request =
            list_entry(slave->reg_requests.next, ec_reg_request_t, list);
        list_del_init(&fsm->reg_request->list); // dequeue
    }

    if (!fsm->reg_request) { // no register request to process
        return 0;
    }

    if (slave->current_state & EC_SLAVE_STATE_ACK_ERR) {
        EC_SLAVE_WARN(slave, "Aborting register request,"
                " slave has error flag set.\n");
        fsm->reg_request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->reg_request = NULL;
        fsm->state = ec_fsm_slave_state_idle;
        return 0;
    }

    // Found pending register request. Execute it!
    EC_SLAVE_DBG(slave, 1, "Processing register request...\n");

    fsm->reg_request->state = EC_INT_REQUEST_BUSY;

    // Start register access
    if (fsm->reg_request->dir == EC_DIR_INPUT) {
        ec_datagram_fprd(datagram, slave->station_address,
                fsm->reg_request->address, fsm->reg_request->transfer_size);
        ec_datagram_zero(datagram);
    } else {
        ec_datagram_fpwr(datagram, slave->station_address,
                fsm->reg_request->address, fsm->reg_request->transfer_size);
        memcpy(datagram->data, fsm->reg_request->data,
                fsm->reg_request->transfer_size);
    }
    datagram->device_index = slave->device_index;
    fsm->state = ec_fsm_slave_state_reg_request;
    return 1;
}

/****************************************************************************/

/** Slave state: Register request.
 */
void ec_fsm_slave_state_reg_request(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_reg_request_t *reg = fsm->reg_request;

    if (!reg) {
        // configuration was cleared in the meantime
        fsm->state = ec_fsm_slave_state_ready;
        fsm->reg_request = NULL;
        return;
    }

    if (fsm->datagram->state != EC_DATAGRAM_RECEIVED) {
        EC_SLAVE_ERR(slave, "Failed to receive register"
                " request datagram: ");
        ec_datagram_print_state(fsm->datagram);
        reg->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->reg_request = NULL;
        fsm->state = ec_fsm_slave_state_ready;
        return;
    }

    if (fsm->datagram->working_counter == 1) {
        if (reg->dir == EC_DIR_INPUT) { // read request
            memcpy(reg->data, fsm->datagram->data, reg->transfer_size);
        }

        reg->state = EC_INT_REQUEST_SUCCESS;
        EC_SLAVE_DBG(slave, 1, "Register request successful.\n");
    } else {
        reg->state = EC_INT_REQUEST_FAILURE;
        ec_datagram_print_state(fsm->datagram);
        EC_SLAVE_ERR(slave, "Register request failed"
                " (working counter is %u).\n",
                fsm->datagram->working_counter);
    }

    wake_up_all(&slave->master->request_queue);
    fsm->reg_request = NULL;
    fsm->state = ec_fsm_slave_state_ready;
}

/****************************************************************************/

/** Check for pending FoE requests and process one.
 *
 * \return non-zero, if an FoE request is processed.
 */
int ec_fsm_slave_action_process_foe(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_foe_request_t *request;

    if (list_empty(&slave->foe_requests)) {
        return 0;
    }

    // take the first request to be processed
    request = list_entry(slave->foe_requests.next, ec_foe_request_t, list);
    list_del_init(&request->list); // dequeue

    if (slave->current_state & EC_SLAVE_STATE_ACK_ERR) {
        EC_SLAVE_WARN(slave, "Aborting FoE request,"
                " slave has error flag set.\n");
        request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->state = ec_fsm_slave_state_idle;
        return 0;
    }

    request->state = EC_INT_REQUEST_BUSY;
    fsm->foe_request = request;

    EC_SLAVE_DBG(slave, 1, "Processing FoE request.\n");

    fsm->state = ec_fsm_slave_state_foe_request;
    ec_fsm_foe_transfer(&fsm->fsm_foe, slave, request);
    ec_fsm_foe_exec(&fsm->fsm_foe, datagram);
    return 1;
}

/****************************************************************************/

/** Slave state: FOE REQUEST.
 */
void ec_fsm_slave_state_foe_request(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_foe_request_t *request = fsm->foe_request;

    if (ec_fsm_foe_exec(&fsm->fsm_foe, datagram)) {
        return;
    }

    if (!ec_fsm_foe_success(&fsm->fsm_foe)) {
        EC_SLAVE_ERR(slave, "Failed to handle FoE request.\n");
        request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->foe_request = NULL;
        fsm->state = ec_fsm_slave_state_ready;
        return;
    }

    // finished transferring FoE
    EC_SLAVE_DBG(slave, 1, "Successfully transferred %zu bytes of FoE"
            " data.\n", request->data_size);

    request->state = EC_INT_REQUEST_SUCCESS;
    wake_up_all(&slave->master->request_queue);
    fsm->foe_request = NULL;
    fsm->state = ec_fsm_slave_state_ready;
}

/****************************************************************************/

/** Check for pending SoE requests and process one.
 *
 * \return non-zero, if a request is processed.
 */
int ec_fsm_slave_action_process_soe(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_soe_request_t *req;

    if (list_empty(&slave->soe_requests)) {
        return 0;
    }

    // take the first request to be processed
    req = list_entry(slave->soe_requests.next, ec_soe_request_t, list);
    list_del_init(&req->list); // dequeue

    if (slave->current_state & EC_SLAVE_STATE_ACK_ERR) {
        EC_SLAVE_WARN(slave, "Aborting SoE request,"
                " slave has error flag set.\n");
        req->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->state = ec_fsm_slave_state_idle;
        return 0;
    }

    if (slave->current_state == EC_SLAVE_STATE_INIT) {
        EC_SLAVE_WARN(slave, "Aborting SoE request, slave is in INIT.\n");
        req->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->state = ec_fsm_slave_state_idle;
        return 0;
    }

    fsm->soe_request = req;
    req->state = EC_INT_REQUEST_BUSY;

    // Found pending request. Execute it!
    EC_SLAVE_DBG(slave, 1, "Processing SoE request...\n");

    // Start SoE transfer
    fsm->state = ec_fsm_slave_state_soe_request;
    ec_fsm_soe_transfer(&fsm->fsm_soe, slave, req);
    ec_fsm_soe_exec(&fsm->fsm_soe, datagram); // execute immediately
    return 1;
}

/****************************************************************************/

/** Slave state: SOE_REQUEST.
 */
void ec_fsm_slave_state_soe_request(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_soe_request_t *request = fsm->soe_request;

    if (ec_fsm_soe_exec(&fsm->fsm_soe, datagram)) {
        return;
    }

    if (!ec_fsm_soe_success(&fsm->fsm_soe)) {
        EC_SLAVE_ERR(slave, "Failed to process SoE request.\n");
        request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->soe_request = NULL;
        fsm->state = ec_fsm_slave_state_ready;
        return;
    }

    EC_SLAVE_DBG(slave, 1, "Finished SoE request.\n");

    // SoE request finished
    request->state = EC_INT_REQUEST_SUCCESS;
    wake_up_all(&slave->master->request_queue);
    fsm->soe_request = NULL;
    fsm->state = ec_fsm_slave_state_ready;
}

/****************************************************************************/

#ifdef EC_EOE
/** Check for pending EoE IP parameter requests and process one.
 *
 * \return non-zero, if a request is processed.
 */
int ec_fsm_slave_action_process_eoe(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_eoe_request_t *request;

    if (list_empty(&slave->eoe_requests)) {
        return 0;
    }

    // take the first request to be processed
    request = list_entry(slave->eoe_requests.next, ec_eoe_request_t, list);
    list_del_init(&request->list); // dequeue

    if (slave->current_state & EC_SLAVE_STATE_ACK_ERR) {
        EC_SLAVE_WARN(slave, "Aborting EoE request,"
                " slave has error flag set.\n");
        request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->state = ec_fsm_slave_state_idle;
        return 0;
    }

    if (slave->current_state == EC_SLAVE_STATE_INIT) {
        EC_SLAVE_WARN(slave, "Aborting EoE request, slave is in INIT.\n");
        request->state = EC_INT_REQUEST_FAILURE;
        wake_up_all(&slave->master->request_queue);
        fsm->state = ec_fsm_slave_state_idle;
        return 0;
    }

    fsm->eoe_request = request;
    request->state = EC_INT_REQUEST_BUSY;

    // Found pending request. Execute it!
    EC_SLAVE_DBG(slave, 1, "Processing EoE request...\n");

    // Start EoE command
    fsm->state = ec_fsm_slave_state_eoe_request;
    ec_fsm_eoe_set_ip_param(&fsm->fsm_eoe, slave, request);
    ec_fsm_eoe_exec(&fsm->fsm_eoe, datagram); // execute immediately
    return 1;
}

/****************************************************************************/

/** Slave state: EOE_REQUEST.
 */
void ec_fsm_slave_state_eoe_request(
        ec_fsm_slave_t *fsm, /**< Slave state machine. */
        ec_datagram_t *datagram /**< Datagram to use. */
        )
{
    ec_slave_t *slave = fsm->slave;
    ec_eoe_request_t *req = fsm->eoe_request;

    if (ec_fsm_eoe_exec(&fsm->fsm_eoe, datagram)) {
        return;
    }

    if (ec_fsm_eoe_success(&fsm->fsm_eoe)) {
		req->state = EC_INT_REQUEST_SUCCESS;
		EC_SLAVE_DBG(slave, 1, "Finished EoE request.\n");
    }
	else {
        req->state = EC_INT_REQUEST_FAILURE;
        EC_SLAVE_ERR(slave, "Failed to process EoE request.\n");
	}

    wake_up_all(&slave->master->request_queue);
    fsm->eoe_request = NULL;
    fsm->state = ec_fsm_slave_state_ready;
}

/****************************************************************************/
#endif

/*****************************************************************************
 *
 *  Copyright (C) 2006-2012  Florian Pose, Ingenieurgemeinschaft IgH
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
   EtherCAT slave methods.
*/

/****************************************************************************/

#include <linux/module.h>
#include <linux/delay.h>

#include "globals.h"
#include "datagram.h"
#include "master.h"
#include "slave_config.h"

#include "slave.h"

/****************************************************************************/

extern const ec_code_msg_t al_status_messages[];

/****************************************************************************/

// prototypes for private methods
char *ec_slave_sii_string(const ec_slave_t *, unsigned int);
void ec_slave_find_names_for_pdo(ec_slave_t *, ec_pdo_t *);
unsigned int ec_slave_get_previous_port(const ec_slave_t *, unsigned int);
unsigned int ec_slave_get_next_port(const ec_slave_t *, unsigned int);
uint32_t ec_slave_calc_rtt_sum(const ec_slave_t *);
ec_slave_t *ec_slave_find_next_dc_slave(ec_slave_t *);

/****************************************************************************/

/**
   Slave constructor.
   \return 0 in case of success, else < 0
*/

void ec_slave_init(
        ec_slave_t *slave, /**< EtherCAT slave */
        ec_master_t *master, /**< EtherCAT master */
        ec_device_index_t dev_idx, /**< Device index. */
        uint16_t ring_position, /**< ring position */
        uint16_t station_address /**< station address to configure */
        )
{
    unsigned int i;

    slave->master = master;
    slave->device_index = dev_idx;
    slave->ring_position = ring_position;
    slave->station_address = station_address;
    slave->effective_alias = 0x0000;

    slave->config = NULL;
    slave->requested_state = EC_SLAVE_STATE_PREOP;
    slave->current_state = EC_SLAVE_STATE_UNKNOWN;
    slave->error_flag = 0;
    slave->force_config = 0;
    slave->configured_rx_mailbox_offset = 0x0000;
    slave->configured_rx_mailbox_size = 0x0000;
    slave->configured_tx_mailbox_offset = 0x0000;
    slave->configured_tx_mailbox_size = 0x0000;

    slave->base_type = 0;
    slave->base_revision = 0;
    slave->base_build = 0;
    slave->base_fmmu_count = 0;
    slave->base_sync_count = 0;

    for (i = 0; i < EC_MAX_PORTS; i++) {
        slave->ports[i].desc = EC_PORT_NOT_IMPLEMENTED;

        slave->ports[i].link.link_up = 0;
        slave->ports[i].link.loop_closed = 0;
        slave->ports[i].link.signal_detected = 0;
        slave->sii.physical_layer[i] = 0xFF;

        slave->ports[i].receive_time = 0U;

        slave->ports[i].next_slave = NULL;
        slave->ports[i].delay_to_next_dc = 0U;
    }

    slave->base_fmmu_bit_operation = 0;
    slave->base_dc_supported = 0;
    slave->base_dc_range = EC_DC_32;
    slave->has_dc_system_time = 0;
    slave->transmission_delay = 0U;

    slave->sii_words = NULL;
    slave->sii_nwords = 0;

    slave->sii.alias = 0x0000;
    slave->sii.vendor_id = 0x00000000;
    slave->sii.product_code = 0x00000000;
    slave->sii.revision_number = 0x00000000;
    slave->sii.serial_number = 0x00000000;
    slave->sii.boot_rx_mailbox_offset = 0x0000;
    slave->sii.boot_rx_mailbox_size = 0x0000;
    slave->sii.boot_tx_mailbox_offset = 0x0000;
    slave->sii.boot_tx_mailbox_size = 0x0000;
    slave->sii.std_rx_mailbox_offset = 0x0000;
    slave->sii.std_rx_mailbox_size = 0x0000;
    slave->sii.std_tx_mailbox_offset = 0x0000;
    slave->sii.std_tx_mailbox_size = 0x0000;
    slave->sii.mailbox_protocols = 0;

    slave->sii.strings = NULL;
    slave->sii.string_count = 0;

    slave->sii.has_general = 0;
    slave->sii.group = NULL;
    slave->sii.image = NULL;
    slave->sii.order = NULL;
    slave->sii.name = NULL;
    memset(&slave->sii.coe_details, 0x00, sizeof(ec_sii_coe_details_t));
    memset(&slave->sii.general_flags, 0x00, sizeof(ec_sii_general_flags_t));
    slave->sii.current_on_ebus = 0;

    slave->sii.syncs = NULL;
    slave->sii.sync_count = 0;

    INIT_LIST_HEAD(&slave->sii.pdos);

    INIT_LIST_HEAD(&slave->sdo_dictionary);

    slave->sdo_dictionary_fetched = 0;
    slave->jiffies_preop = 0;

    INIT_LIST_HEAD(&slave->sdo_requests);
    INIT_LIST_HEAD(&slave->reg_requests);
    INIT_LIST_HEAD(&slave->foe_requests);
    INIT_LIST_HEAD(&slave->soe_requests);
    INIT_LIST_HEAD(&slave->eoe_requests);

    // create state machine object
    ec_fsm_slave_init(&slave->fsm, slave);
}

/****************************************************************************/

/**
   Slave destructor.
   Clears and frees a slave object.
*/

void ec_slave_clear(ec_slave_t *slave /**< EtherCAT slave */)
{
    ec_sdo_t *sdo, *next_sdo;
    unsigned int i;
    ec_pdo_t *pdo, *next_pdo;

    // abort all pending requests

    while (!list_empty(&slave->sdo_requests)) {
        ec_sdo_request_t *request =
            list_entry(slave->sdo_requests.next, ec_sdo_request_t, list);
        list_del_init(&request->list); // dequeue
        EC_SLAVE_WARN(slave, "Discarding SDO request,"
                " slave about to be deleted.\n");
        request->state = EC_INT_REQUEST_FAILURE;
    }

    while (!list_empty(&slave->reg_requests)) {
        ec_reg_request_t *reg =
            list_entry(slave->reg_requests.next, ec_reg_request_t, list);
        list_del_init(&reg->list); // dequeue
        EC_SLAVE_WARN(slave, "Discarding register request,"
                " slave about to be deleted.\n");
        reg->state = EC_INT_REQUEST_FAILURE;
    }

    while (!list_empty(&slave->foe_requests)) {
        ec_foe_request_t *request =
            list_entry(slave->foe_requests.next, ec_foe_request_t, list);
        list_del_init(&request->list); // dequeue
        EC_SLAVE_WARN(slave, "Discarding FoE request,"
                " slave about to be deleted.\n");
        request->state = EC_INT_REQUEST_FAILURE;
    }

    while (!list_empty(&slave->soe_requests)) {
        ec_soe_request_t *request =
            list_entry(slave->soe_requests.next, ec_soe_request_t, list);
        list_del_init(&request->list); // dequeue
        EC_SLAVE_WARN(slave, "Discarding SoE request,"
                " slave about to be deleted.\n");
        request->state = EC_INT_REQUEST_FAILURE;
    }

#ifdef EC_EOE
    while (!list_empty(&slave->eoe_requests)) {
        ec_eoe_request_t *request =
            list_entry(slave->eoe_requests.next, ec_eoe_request_t, list);
        list_del_init(&request->list); // dequeue
        EC_SLAVE_WARN(slave, "Discarding EoE request,"
                " slave about to be deleted.\n");
        request->state = EC_INT_REQUEST_FAILURE;
    }
#endif

    wake_up_all(&slave->master->request_queue);

    if (slave->config) {
        ec_slave_config_detach(slave->config);
    }

    // free all SDOs
    list_for_each_entry_safe(sdo, next_sdo, &slave->sdo_dictionary, list) {
        list_del(&sdo->list);
        ec_sdo_clear(sdo);
        kfree(sdo);
    }

    // free all strings
    if (slave->sii.strings) {
        for (i = 0; i < slave->sii.string_count; i++)
            kfree(slave->sii.strings[i]);
        kfree(slave->sii.strings);
    }

    // free all sync managers
    ec_slave_clear_sync_managers(slave);

    // free all SII PDOs
    list_for_each_entry_safe(pdo, next_pdo, &slave->sii.pdos, list) {
        list_del(&pdo->list);
        ec_pdo_clear(pdo);
        kfree(pdo);
    }

    if (slave->sii_words) {
        kfree(slave->sii_words);
    }

    ec_fsm_slave_clear(&slave->fsm);
}

/****************************************************************************/

/** Clear the sync manager array.
 */
void ec_slave_clear_sync_managers(ec_slave_t *slave /**< EtherCAT slave. */)
{
    unsigned int i;

    if (slave->sii.syncs) {
        for (i = 0; i < slave->sii.sync_count; i++) {
            ec_sync_clear(&slave->sii.syncs[i]);
        }
        kfree(slave->sii.syncs);
        slave->sii.syncs = NULL;
    }
}

/****************************************************************************/

/**
 * Sets the application state of a slave.
 */

void ec_slave_set_state(ec_slave_t *slave, /**< EtherCAT slave */
        ec_slave_state_t new_state /**< new application state */
        )
{
    if (new_state != slave->current_state) {
        if (slave->master->debug_level) {
            char old_state[EC_STATE_STRING_SIZE],
                cur_state[EC_STATE_STRING_SIZE];
            ec_state_string(slave->current_state, old_state, 0);
            ec_state_string(new_state, cur_state, 0);
            EC_SLAVE_DBG(slave, 0, "%s -> %s.\n", old_state, cur_state);
        }
        slave->current_state = new_state;
    }
}

/****************************************************************************/

/**
 * Request a slave state and resets the error flag.
 */

void ec_slave_request_state(ec_slave_t *slave, /**< EtherCAT slave */
                            ec_slave_state_t state /**< new state */
                            )
{
    slave->requested_state = state;
    slave->error_flag = 0;
}

/****************************************************************************/

/**
   Fetches data from a STRING category.
   \todo range checking
   \return 0 in case of success, else < 0
*/

int ec_slave_fetch_sii_strings(
        ec_slave_t *slave, /**< EtherCAT slave */
        const uint8_t *data, /**< category data */
        size_t data_size /**< number of bytes */
        )
{
    int i, err;
    size_t size;
    off_t offset;

    slave->sii.string_count = data[0];

    if (slave->sii.string_count) {
        if (!(slave->sii.strings =
                    kmalloc(sizeof(char *) * slave->sii.string_count,
                        GFP_KERNEL))) {
            EC_SLAVE_ERR(slave, "Failed to allocate string array memory.\n");
            err = -ENOMEM;
            goto out_zero;
        }

        offset = 1;
        for (i = 0; i < slave->sii.string_count; i++) {
            size = data[offset];
            // allocate memory for string structure and data at a single blow
            if (!(slave->sii.strings[i] =
                        kmalloc(sizeof(char) * size + 1, GFP_KERNEL))) {
                EC_SLAVE_ERR(slave, "Failed to allocate string memory.\n");
                err = -ENOMEM;
                goto out_free;
            }
            memcpy(slave->sii.strings[i], data + offset + 1, size);
            slave->sii.strings[i][size] = 0x00; // append binary zero
            offset += 1 + size;
        }
    }

    return 0;

out_free:
    for (i--; i >= 0; i--)
        kfree(slave->sii.strings[i]);
    kfree(slave->sii.strings);
    slave->sii.strings = NULL;
out_zero:
    slave->sii.string_count = 0;
    return err;
}

/****************************************************************************/

/**
   Fetches data from a GENERAL category.
   \return 0 in case of success, else < 0
*/

int ec_slave_fetch_sii_general(
        ec_slave_t *slave, /**< EtherCAT slave */
        const uint8_t *data, /**< category data */
        size_t data_size /**< size in bytes */
        )
{
    unsigned int i;
    uint8_t flags;

    if (data_size != 32) {
        EC_SLAVE_ERR(slave, "Wrong size of general category (%zu/32).\n",
                data_size);
        return -EINVAL;
    }

    slave->sii.group = ec_slave_sii_string(slave, data[0]);
    slave->sii.image = ec_slave_sii_string(slave, data[1]);
    slave->sii.order = ec_slave_sii_string(slave, data[2]);
    slave->sii.name = ec_slave_sii_string(slave, data[3]);

    for (i = 0; i < 4; i++)
        slave->sii.physical_layer[i] =
            (data[4] & (0x03 << (i * 2))) >> (i * 2);

    // read CoE details
    flags = EC_READ_U8(data + 5);
    slave->sii.coe_details.enable_sdo =                 (flags >> 0) & 0x01;
    slave->sii.coe_details.enable_sdo_info =            (flags >> 1) & 0x01;
    slave->sii.coe_details.enable_pdo_assign =          (flags >> 2) & 0x01;
    slave->sii.coe_details.enable_pdo_configuration =   (flags >> 3) & 0x01;
    slave->sii.coe_details.enable_upload_at_startup =   (flags >> 4) & 0x01;
    slave->sii.coe_details.enable_sdo_complete_access = (flags >> 5) & 0x01;

    // read general flags
    flags = EC_READ_U8(data + 0x000B);
    slave->sii.general_flags.enable_safeop =  (flags >> 0) & 0x01;
    slave->sii.general_flags.enable_not_lrw = (flags >> 1) & 0x01;

    slave->sii.current_on_ebus = EC_READ_S16(data + 0x0C);
    slave->sii.has_general = 1;
    return 0;
}

/****************************************************************************/

/** Fetches data from a SYNC MANAGER category.
 *
 * Appends the sync managers described in the category to the existing ones.
 *
 * \return 0 in case of success, else < 0
 */
int ec_slave_fetch_sii_syncs(
        ec_slave_t *slave, /**< EtherCAT slave. */
        const uint8_t *data, /**< Category data. */
        size_t data_size /**< Number of bytes. */
        )
{
    unsigned int i, count, total_count;
    ec_sync_t *sync;
    size_t memsize;
    ec_sync_t *syncs;
    uint8_t index;

    // one sync manager struct is 4 words long
    if (data_size % 8) {
        EC_SLAVE_ERR(slave, "Invalid SII sync manager category size %zu.\n",
                data_size);
        return -EINVAL;
    }

    count = data_size / 8;

    if (count) {
        total_count = count + slave->sii.sync_count;
        if (total_count > EC_MAX_SYNC_MANAGERS) {
            EC_SLAVE_ERR(slave, "Exceeded maximum number of"
                    " sync managers!\n");
            return -EOVERFLOW;
        }
        memsize = sizeof(ec_sync_t) * total_count;
        if (!(syncs = kmalloc(memsize, GFP_KERNEL))) {
            EC_SLAVE_ERR(slave, "Failed to allocate %zu bytes"
                    " for sync managers.\n", memsize);
            return -ENOMEM;
        }

        for (i = 0; i < slave->sii.sync_count; i++)
            ec_sync_init_copy(syncs + i, slave->sii.syncs + i);

        // initialize new sync managers
        for (i = 0; i < count; i++, data += 8) {
            index = i + slave->sii.sync_count;
            sync = &syncs[index];

            ec_sync_init(sync, slave);
            sync->physical_start_address = EC_READ_U16(data);
            sync->default_length = EC_READ_U16(data + 2);
            sync->control_register = EC_READ_U8(data + 4);
            sync->enable = EC_READ_U8(data + 6);
        }

        if (slave->sii.syncs)
            kfree(slave->sii.syncs);
        slave->sii.syncs = syncs;
        slave->sii.sync_count = total_count;
    }

    return 0;
}

/****************************************************************************/

/**
   Fetches data from a [RT]xPDO category.
   \return 0 in case of success, else < 0
*/

int ec_slave_fetch_sii_pdos(
        ec_slave_t *slave, /**< EtherCAT slave */
        const uint8_t *data, /**< category data */
        size_t data_size, /**< number of bytes */
        ec_direction_t dir /**< PDO direction. */
        )
{
    int ret;
    ec_pdo_t *pdo;
    ec_pdo_entry_t *entry;
    unsigned int entry_count, i;

    while (data_size >= 8) {
        if (!(pdo = kmalloc(sizeof(ec_pdo_t), GFP_KERNEL))) {
            EC_SLAVE_ERR(slave, "Failed to allocate PDO memory.\n");
            return -ENOMEM;
        }

        ec_pdo_init(pdo);
        pdo->index = EC_READ_U16(data);
        entry_count = EC_READ_U8(data + 2);
        pdo->sync_index = EC_READ_U8(data + 3);
        ret = ec_pdo_set_name(pdo,
                ec_slave_sii_string(slave, EC_READ_U8(data + 5)));
        if (ret) {
            ec_pdo_clear(pdo);
            kfree(pdo);
            return ret;
        }
        list_add_tail(&pdo->list, &slave->sii.pdos);

        data_size -= 8;
        data += 8;

        for (i = 0; i < entry_count; i++) {
            if (!(entry = kmalloc(sizeof(ec_pdo_entry_t), GFP_KERNEL))) {
                EC_SLAVE_ERR(slave, "Failed to allocate PDO entry memory.\n");
                return -ENOMEM;
            }

            ec_pdo_entry_init(entry);
            entry->index = EC_READ_U16(data);
            entry->subindex = EC_READ_U8(data + 2);
            ret = ec_pdo_entry_set_name(entry,
                    ec_slave_sii_string(slave, EC_READ_U8(data + 3)));
            if (ret) {
                ec_pdo_entry_clear(entry);
                kfree(entry);
                return ret;
            }
            entry->bit_length = EC_READ_U8(data + 5);
            list_add_tail(&entry->list, &pdo->entries);

            data_size -= 8;
            data += 8;
        }

        // if sync manager index is positive, the PDO is mapped by default
        if (pdo->sync_index >= 0) {
            ec_sync_t *sync;

            if (!(sync = ec_slave_get_sync(slave, pdo->sync_index))) {
                EC_SLAVE_ERR(slave, "Invalid SM index %i for PDO 0x%04X.",
                        pdo->sync_index, pdo->index);
                return -ENOENT;
            }

            ret = ec_pdo_list_add_pdo_copy(&sync->pdos, pdo);
            if (ret)
                return ret;
        }
    }

    return 0;
}

/****************************************************************************/

/**
   Searches the string list for an index.
   \return 0 in case of success, else < 0
*/

char *ec_slave_sii_string(
        const ec_slave_t *slave, /**< EtherCAT slave */
        unsigned int index /**< string index */
        )
{
    if (!index--)
        return NULL;

    if (index >= slave->sii.string_count) {
        EC_SLAVE_DBG(slave, 1, "String %u not found.\n", index);
        return NULL;
    }

    return slave->sii.strings[index];
}

/****************************************************************************/

/** Get the sync manager given an index.
 *
 * \return pointer to sync manager, or NULL.
 */
ec_sync_t *ec_slave_get_sync(
        ec_slave_t *slave, /**< EtherCAT slave. */
        uint8_t sync_index /**< Sync manager index. */
        )
{
    if (sync_index < slave->sii.sync_count) {
        return &slave->sii.syncs[sync_index];
    } else {
        return NULL;
    }
}

/****************************************************************************/

/**
   Counts the total number of SDOs and entries in the dictionary.
*/

void ec_slave_sdo_dict_info(const ec_slave_t *slave, /**< EtherCAT slave */
                            unsigned int *sdo_count, /**< number of SDOs */
                            unsigned int *entry_count /**< total number of
                                                         entries */
                            )
{
    unsigned int sdos = 0, entries = 0;
    ec_sdo_t *sdo;
    ec_sdo_entry_t *entry;

    list_for_each_entry(sdo, &slave->sdo_dictionary, list) {
        sdos++;
        list_for_each_entry(entry, &sdo->entries, list) {
            entries++;
        }
    }

    *sdo_count = sdos;
    *entry_count = entries;
}

/****************************************************************************/

/**
 * Get an SDO from the dictionary.
 * \returns The desired SDO, or NULL.
 */

ec_sdo_t *ec_slave_get_sdo(
        ec_slave_t *slave, /**< EtherCAT slave */
        uint16_t index /**< SDO index */
        )
{
    ec_sdo_t *sdo;

    list_for_each_entry(sdo, &slave->sdo_dictionary, list) {
        if (sdo->index != index)
            continue;
        return sdo;
    }

    return NULL;
}

/****************************************************************************/

/**
 * Get an SDO from the dictionary.
 *
 * const version.
 *
 * \returns The desired SDO, or NULL.
 */

const ec_sdo_t *ec_slave_get_sdo_const(
        const ec_slave_t *slave, /**< EtherCAT slave */
        uint16_t index /**< SDO index */
        )
{
    const ec_sdo_t *sdo;

    list_for_each_entry(sdo, &slave->sdo_dictionary, list) {
        if (sdo->index != index)
            continue;
        return sdo;
    }

    return NULL;
}

/****************************************************************************/

/** Get an SDO from the dictionary, given its position in the list.
 * \returns The desired SDO, or NULL.
 */

const ec_sdo_t *ec_slave_get_sdo_by_pos_const(
        const ec_slave_t *slave, /**< EtherCAT slave. */
        uint16_t sdo_position /**< SDO list position. */
        )
{
    const ec_sdo_t *sdo;

    list_for_each_entry(sdo, &slave->sdo_dictionary, list) {
        if (sdo_position--)
            continue;
        return sdo;
    }

    return NULL;
}

/****************************************************************************/

/** Get the number of SDOs in the dictionary.
 * \returns SDO count.
 */

uint16_t ec_slave_sdo_count(
        const ec_slave_t *slave /**< EtherCAT slave. */
        )
{
    const ec_sdo_t *sdo;
    uint16_t count = 0;

    list_for_each_entry(sdo, &slave->sdo_dictionary, list) {
        count++;
    }

    return count;
}

/****************************************************************************/

/** Finds a mapped PDO.
 * \returns The desired PDO object, or NULL.
 */
const ec_pdo_t *ec_slave_find_pdo(
        const ec_slave_t *slave, /**< Slave. */
        uint16_t index /**< PDO index to find. */
        )
{
    unsigned int i;
    const ec_sync_t *sync;
    const ec_pdo_t *pdo;

    for (i = 0; i < slave->sii.sync_count; i++) {
        sync = &slave->sii.syncs[i];

        if (!(pdo = ec_pdo_list_find_pdo_const(&sync->pdos, index)))
            continue;

        return pdo;
    }

    return NULL;
}

/****************************************************************************/

/** Find name for a PDO and its entries.
 */
void ec_slave_find_names_for_pdo(
        ec_slave_t *slave,
        ec_pdo_t *pdo
        )
{
    const ec_sdo_t *sdo;
    ec_pdo_entry_t *pdo_entry;
    const ec_sdo_entry_t *sdo_entry;

    list_for_each_entry(sdo, &slave->sdo_dictionary, list) {
        if (sdo->index == pdo->index) {
            ec_pdo_set_name(pdo, sdo->name);
        } else {
            list_for_each_entry(pdo_entry, &pdo->entries, list) {
                if (sdo->index == pdo_entry->index) {
                    sdo_entry = ec_sdo_get_entry_const(
                            sdo, pdo_entry->subindex);
                    if (sdo_entry) {
                        ec_pdo_entry_set_name(pdo_entry,
                                sdo_entry->description);
                    }
                }
            }
        }
    }
}

/****************************************************************************/

/** Attach PDO names.
 */
void ec_slave_attach_pdo_names(
        ec_slave_t *slave
        )
{
    unsigned int i;
    ec_sync_t *sync;
    ec_pdo_t *pdo;

    for (i = 0; i < slave->sii.sync_count; i++) {
        sync = slave->sii.syncs + i;
        list_for_each_entry(pdo, &sync->pdos.list, list) {
            ec_slave_find_names_for_pdo(slave, pdo);
        }
    }
}

/****************************************************************************/

/** Returns the previous connected port of a given port.
 *
 * \return Port index.
 */
unsigned int ec_slave_get_previous_port(
        const ec_slave_t *slave, /**< EtherCAT slave. */
        unsigned int port_index /**< Port index. */
        )
{
    static const unsigned int prev_table[EC_MAX_PORTS] = {
        2, 3, 1, 0
    };

    if (port_index >= EC_MAX_PORTS) {
        EC_SLAVE_WARN(slave, "%s(port_index=%u): Invalid port index!\n",
                __func__, port_index);
    }

    do {
        port_index = prev_table[port_index];
        if (slave->ports[port_index].next_slave) {
            return port_index;
        }
    } while (port_index);

    return 0;
}

/****************************************************************************/

/** Returns the next connected port of a given port.
 *
 * \return Port index.
 */
unsigned int ec_slave_get_next_port(
        const ec_slave_t *slave, /**< EtherCAT slave. */
        unsigned int port_index /**< Port index. */
        )
{
    static const unsigned int next_table[EC_MAX_PORTS] = {
        3, 2, 0, 1
    };

    if (port_index >= EC_MAX_PORTS) {
        EC_SLAVE_WARN(slave, "%s(port_index=%u): Invalid port index!\n",
                __func__, port_index);
    }

    do {
        port_index = next_table[port_index];
        if (slave->ports[port_index].next_slave) {
            return port_index;
        }
    } while (port_index);

    return 0;
}

/****************************************************************************/

/** Calculates the sum of round-trip-times of connected ports 1-3.
 *
 * \return Round-trip-time in ns.
 */
uint32_t ec_slave_calc_rtt_sum(
        const ec_slave_t *slave /**< EtherCAT slave. */
        )
{
    uint32_t rtt_sum = 0, rtt;
    unsigned int port_index = ec_slave_get_next_port(slave, 0);

    while (port_index != 0) {
        unsigned int prev_index =
            ec_slave_get_previous_port(slave, port_index);

        rtt = slave->ports[port_index].receive_time -
            slave->ports[prev_index].receive_time;
        rtt_sum += rtt;
        port_index = ec_slave_get_next_port(slave, port_index);
    }

    return rtt_sum;
}

/****************************************************************************/

/** Finds the next slave supporting DC delay measurement.
 *
 * \return Next DC slave, or NULL.
 */
ec_slave_t *ec_slave_find_next_dc_slave(
        ec_slave_t *slave /**< EtherCAT slave. */
        )
{
    unsigned int port_index;
    ec_slave_t *dc_slave = NULL;

    if (slave->base_dc_supported) {
        dc_slave = slave;
    } else {
        port_index = ec_slave_get_next_port(slave, 0);

        while (port_index != 0) {
            ec_slave_t *next = slave->ports[port_index].next_slave;

            if (next) {
                dc_slave = ec_slave_find_next_dc_slave(next);

                if (dc_slave) {
                    break;
                }
            }
            port_index = ec_slave_get_next_port(slave, port_index);
        }
    }

    return dc_slave;
}

/****************************************************************************/

/** Calculates the port transmission delays.
 */
void ec_slave_calc_port_delays(
        ec_slave_t *slave /**< EtherCAT slave. */
        )
{
    unsigned int port_index;
    ec_slave_t *next_slave, *next_dc;
    uint32_t rtt, next_rtt_sum;

    if (!slave->base_dc_supported)
        return;

    port_index = ec_slave_get_next_port(slave, 0);

    while (port_index != 0) {
        next_slave = slave->ports[port_index].next_slave;
        next_dc = ec_slave_find_next_dc_slave(next_slave);

        if (next_dc) {
            unsigned int prev_port =
                ec_slave_get_previous_port(slave, port_index);

            rtt = slave->ports[port_index].receive_time -
                slave->ports[prev_port].receive_time;
            next_rtt_sum = ec_slave_calc_rtt_sum(next_dc);

            slave->ports[port_index].delay_to_next_dc =
                (rtt - next_rtt_sum) / 2; // FIXME
            next_dc->ports[0].delay_to_next_dc =
                (rtt - next_rtt_sum) / 2;

#if 0
            EC_SLAVE_DBG(slave, 1, "delay %u:%u rtt=%u"
                    " next_rtt_sum=%u delay=%u\n",
                    slave->ring_position, port_index, rtt, next_rtt_sum,
                    slave->ports[port_index].delay_to_next_dc);
#endif
        }

        port_index = ec_slave_get_next_port(slave, port_index);
    }
}

/****************************************************************************/

/** Recursively calculates transmission delays.
 */
void ec_slave_calc_transmission_delays_rec(
        ec_slave_t *slave, /**< Current slave. */
        uint32_t *delay /**< Sum of delays. */
        )
{
    unsigned int i;
    ec_slave_t *next_dc;

    EC_SLAVE_DBG(slave, 1, "%s(delay = %u ns)\n", __func__, *delay);

    slave->transmission_delay = *delay;

    i = ec_slave_get_next_port(slave, 0);

    while (i != 0) {
        ec_slave_port_t *port = &slave->ports[i];
        next_dc = ec_slave_find_next_dc_slave(port->next_slave);
        if (next_dc) {
            *delay = *delay + port->delay_to_next_dc;
#if 0
            EC_SLAVE_DBG(slave, 1, "%u:%u %u\n",
                    slave->ring_position, i, *delay);
#endif
            ec_slave_calc_transmission_delays_rec(next_dc, delay);
        }

        i = ec_slave_get_next_port(slave, i);
    }

    *delay = *delay + slave->ports[0].delay_to_next_dc;
}

/****************************************************************************/

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
   EtherCAT slave structure.
*/

/****************************************************************************/

#ifndef __EC_SLAVE_H__
#define __EC_SLAVE_H__

#include <linux/list.h>
#include <linux/kobject.h>

#include "globals.h"
#include "datagram.h"
#include "pdo.h"
#include "sync.h"
#include "sdo.h"
#include "fsm_slave.h"

/****************************************************************************/

/** Convenience macro for printing slave-specific information to syslog.
 *
 * This will print the message in \a fmt with a prefixed
 * "EtherCAT <INDEX>-<POSITION>: ", where INDEX is the master index and
 * POSITION is the slave's ring position.
 *
 * \param slave EtherCAT slave
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_SLAVE_INFO(slave, fmt, args...) \
    printk(KERN_INFO "EtherCAT %u-%u: " fmt, slave->master->index, \
            slave->ring_position, ##args)

/** Convenience macro for printing slave-specific errors to syslog.
 *
 * This will print the message in \a fmt with a prefixed
 * "EtherCAT <INDEX>-<POSITION>: ", where INDEX is the master index and
 * POSITION is the slave's ring position.
 *
 * \param slave EtherCAT slave
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_SLAVE_ERR(slave, fmt, args...) \
    printk(KERN_ERR "EtherCAT ERROR %u-%u: " fmt, slave->master->index, \
            slave->ring_position, ##args)

/** Convenience macro for printing slave-specific warnings to syslog.
 *
 * This will print the message in \a fmt with a prefixed
 * "EtherCAT <INDEX>-<POSITION>: ", where INDEX is the master index and
 * POSITION is the slave's ring position.
 *
 * \param slave EtherCAT slave
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_SLAVE_WARN(slave, fmt, args...) \
    printk(KERN_WARNING "EtherCAT WARNING %u-%u: " fmt, \
            slave->master->index, slave->ring_position, ##args)

/** Convenience macro for printing slave-specific debug messages to syslog.
 *
 * This will print the message in \a fmt with a prefixed
 * "EtherCAT <INDEX>-<POSITION>: ", where INDEX is the master index and
 * POSITION is the slave's ring position.
 *
 * \param slave EtherCAT slave
 * \param level Debug level. Master's debug level must be >= \a level for
 * output.
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_SLAVE_DBG(slave, level, fmt, args...) \
    do { \
        if (slave->master->debug_level >= level) { \
            printk(KERN_DEBUG "EtherCAT DEBUG %u-%u: " fmt, \
                    slave->master->index, slave->ring_position, ##args); \
        } \
    } while (0)

/****************************************************************************/

/** Slave port.
 */
typedef struct {
    ec_slave_port_desc_t desc; /**< Port descriptors. */
    ec_slave_port_link_t link; /**< Port link status. */
    ec_slave_t *next_slave; /**< Connected slaves. */
    uint32_t receive_time; /**< Port receive times for delay
                                            measurement. */
    uint32_t delay_to_next_dc; /**< Delay to next slave with DC support behind
                                 this port [ns]. */
} ec_slave_port_t;

/****************************************************************************/

/** Slave information interface data.
 */
typedef struct {
    // Non-category data
    uint16_t alias; /**< Configured station alias. */
    uint32_t vendor_id; /**< Vendor ID. */
    uint32_t product_code; /**< Vendor-specific product code. */
    uint32_t revision_number; /**< Revision number. */
    uint32_t serial_number; /**< Serial number. */
    uint16_t boot_rx_mailbox_offset; /**< Bootstrap receive mailbox address. */
    uint16_t boot_rx_mailbox_size; /**< Bootstrap receive mailbox size. */
    uint16_t boot_tx_mailbox_offset; /**< Bootstrap transmit mailbox address. */
    uint16_t boot_tx_mailbox_size; /**< Bootstrap transmit mailbox size. */
    uint16_t std_rx_mailbox_offset; /**< Standard receive mailbox address. */
    uint16_t std_rx_mailbox_size; /**< Standard receive mailbox size. */
    uint16_t std_tx_mailbox_offset; /**< Standard transmit mailbox address. */
    uint16_t std_tx_mailbox_size; /**< Standard transmit mailbox size. */
    uint16_t mailbox_protocols; /**< Supported mailbox protocols. */

    // Strings
    char **strings; /**< Strings in SII categories. */
    unsigned int string_count; /**< Number of SII strings. */

    // General
    unsigned int has_general; /**< General category present. */
    char *group; /**< Group name. */
    char *image; /**< Image name. */
    char *order; /**< Order number. */
    char *name; /**< Slave name. */
    uint8_t physical_layer[EC_MAX_PORTS]; /**< Port media. */
    ec_sii_coe_details_t coe_details; /**< CoE detail flags. */
    ec_sii_general_flags_t general_flags; /**< General flags. */
    int16_t current_on_ebus; /**< Power consumption in mA. */

    // SyncM
    ec_sync_t *syncs; /**< SYNC MANAGER categories. */
    unsigned int sync_count; /**< Number of sync managers. */

    // [RT]XPDO
    struct list_head pdos; /**< SII [RT]XPDO categories. */
} ec_sii_t;

/****************************************************************************/

/** EtherCAT slave.
 */
struct ec_slave
{
    ec_master_t *master; /**< Master owning the slave. */
    ec_device_index_t device_index; /**< Index of device the slave responds
                                      on. */

    // addresses
    uint16_t ring_position; /**< Ring position. */
    uint16_t station_address; /**< Configured station address. */
    uint16_t effective_alias; /**< Effective alias address. */

    ec_slave_port_t ports[EC_MAX_PORTS]; /**< Ports. */

    // configuration
    ec_slave_config_t *config; /**< Current configuration. */
    ec_slave_state_t requested_state; /**< Requested application state. */
    ec_slave_state_t current_state; /**< Current application state. */
    unsigned int error_flag; /**< Stop processing after an error. */
    unsigned int force_config; /**< Force (re-)configuration. */
    uint16_t configured_rx_mailbox_offset; /**< Configured receive mailbox
                                             offset. */
    uint16_t configured_rx_mailbox_size; /**< Configured receive mailbox size.
                                          */
    uint16_t configured_tx_mailbox_offset; /**< Configured send mailbox
                                             offset. */
    uint16_t configured_tx_mailbox_size; /**< Configured send mailbox size. */

    // base data
    uint8_t base_type; /**< Slave type. */
    uint8_t base_revision; /**< Revision. */
    uint16_t base_build; /**< Build number. */
    uint8_t base_fmmu_count; /**< Number of supported FMMUs. */
    uint8_t base_sync_count; /**< Number of supported sync managers. */
    uint8_t base_fmmu_bit_operation; /**< FMMU bit operation is supported. */
    uint8_t base_dc_supported; /**< Distributed clocks are supported. */
    ec_slave_dc_range_t base_dc_range; /**< DC range. */
    uint8_t has_dc_system_time; /**< The slave supports the DC system time
                                  register. Otherwise it can only be used for
                                  delay measurement. */
    uint32_t transmission_delay; /**< DC system time transmission delay
                                   (offset from reference clock). */

    // SII
    uint16_t *sii_words; /**< Complete SII image. */
    size_t sii_nwords; /**< Size of the SII contents in words. */

    // Slave information interface
    ec_sii_t sii; /**< Extracted SII data. */

    struct list_head sdo_dictionary; /**< SDO dictionary list */
    uint8_t sdo_dictionary_fetched; /**< Dictionary has been fetched. */
    unsigned long jiffies_preop; /**< Time, the slave went to PREOP. */

    struct list_head sdo_requests; /**< SDO access requests. */
    struct list_head reg_requests; /**< Register access requests. */
    struct list_head foe_requests; /**< FoE requests. */
    struct list_head soe_requests; /**< SoE requests. */
    struct list_head eoe_requests; /**< EoE set IP parameter requests. */

    ec_fsm_slave_t fsm; /**< Slave state machine. */
};

/****************************************************************************/

// slave construction/destruction
void ec_slave_init(ec_slave_t *, ec_master_t *, ec_device_index_t,
        uint16_t, uint16_t);
void ec_slave_clear(ec_slave_t *);

void ec_slave_clear_sync_managers(ec_slave_t *);

void ec_slave_request_state(ec_slave_t *, ec_slave_state_t);
void ec_slave_set_state(ec_slave_t *, ec_slave_state_t);

// SII categories
int ec_slave_fetch_sii_strings(ec_slave_t *, const uint8_t *, size_t);
int ec_slave_fetch_sii_general(ec_slave_t *, const uint8_t *, size_t);
int ec_slave_fetch_sii_syncs(ec_slave_t *, const uint8_t *, size_t);
int ec_slave_fetch_sii_pdos(ec_slave_t *, const uint8_t *, size_t,
        ec_direction_t);

// misc.
ec_sync_t *ec_slave_get_sync(ec_slave_t *, uint8_t);

void ec_slave_sdo_dict_info(const ec_slave_t *,
        unsigned int *, unsigned int *);
ec_sdo_t *ec_slave_get_sdo(ec_slave_t *, uint16_t);
const ec_sdo_t *ec_slave_get_sdo_const(const ec_slave_t *, uint16_t);
const ec_sdo_t *ec_slave_get_sdo_by_pos_const(const ec_slave_t *, uint16_t);
uint16_t ec_slave_sdo_count(const ec_slave_t *);
const ec_pdo_t *ec_slave_find_pdo(const ec_slave_t *, uint16_t);
void ec_slave_attach_pdo_names(ec_slave_t *);

void ec_slave_calc_port_delays(ec_slave_t *);
void ec_slave_calc_transmission_delays_rec(ec_slave_t *, uint32_t *);

/****************************************************************************/

#endif

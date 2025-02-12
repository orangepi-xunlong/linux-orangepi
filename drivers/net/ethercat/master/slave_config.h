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
 *
 ****************************************************************************/

/**
   \file
   EtherCAT slave configuration structure.
*/

/****************************************************************************/

#ifndef __EC_SLAVE_CONFIG_H__
#define __EC_SLAVE_CONFIG_H__

#include <linux/list.h>

#include "globals.h"
#include "slave.h"
#include "sync_config.h"
#include "fmmu_config.h"
#include "coe_emerg_ring.h"
#include "flag.h"

/****************************************************************************/

/** Convenience macro for printing configuration-specific information to
 * syslog.
 *
 * This will print the message in \a fmt with a prefixed
 * "EtherCAT <INDEX> <ALIAS>:<POSITION>: ", where INDEX is the master index
 * and ALIAS and POSITION identify the configuration.
 *
 * \param sc EtherCAT slave configuration
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_CONFIG_INFO(sc, fmt, args...) \
    printk(KERN_INFO "EtherCAT %u %u:%u: " fmt, sc->master->index, \
            sc->alias, sc->position, ##args)

/** Convenience macro for printing configuration-specific errors to syslog.
 *
 * This will print the message in \a fmt with a prefixed
 * "EtherCAT <INDEX> <ALIAS>:<POSITION>: ", where INDEX is the master index
 * and ALIAS and POSITION identify the configuration.
 *
 * \param sc EtherCAT slave configuration
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_CONFIG_ERR(sc, fmt, args...) \
    printk(KERN_ERR "EtherCAT ERROR %u %u:%u: " fmt, sc->master->index, \
            sc->alias, sc->position, ##args)

/** Convenience macro for printing configuration-specific warnings to syslog.
 *
 * This will print the message in \a fmt with a prefixed
 * "EtherCAT <INDEX> <ALIAS>:<POSITION>: ", where INDEX is the master index
 * and ALIAS and POSITION identify the configuration.
 *
 * \param sc EtherCAT slave configuration
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_CONFIG_WARN(sc, fmt, args...) \
    printk(KERN_WARNING "EtherCAT WARNING %u %u:%u: " fmt, \
            sc->master->index, sc->alias, sc->position, ##args)

/** Convenience macro for printing configuration-specific debug messages to
 * syslog.
 *
 * This will print the message in \a fmt with a prefixed
 * "EtherCAT <INDEX> <ALIAS>:<POSITION>: ", where INDEX is the master index
 * and ALIAS and POSITION identify the configuration.
 *
 * \param sc EtherCAT slave configuration
 * \param level Debug level. Master's debug level must be >= \a level for
 * output.
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_CONFIG_DBG(sc, level, fmt, args...) \
    do { \
        if (sc->master->debug_level >= level) { \
            printk(KERN_DEBUG "EtherCAT DEBUG %u %u:%u: " fmt, \
                    sc->master->index, sc->alias, sc->position, ##args); \
        } \
    } while (0)

/****************************************************************************/

/** EtherCAT slave configuration.
 */
struct ec_slave_config {
    struct list_head list; /**< List item. */
    ec_master_t *master; /**< Master owning the slave configuration. */

    uint16_t alias; /**< Slave alias. */
    uint16_t position; /**< Index after alias. If alias is zero, this is the
                         ring position. */
    uint32_t vendor_id; /**< Slave vendor ID. */
    uint32_t product_code; /**< Slave product code. */

    uint16_t watchdog_divider; /**< Watchdog divider as a number of 40ns
                                 intervals (see spec. reg. 0x0400). */
    uint16_t watchdog_intervals; /**< Process data watchdog intervals (see
                                   spec. reg. 0x0420). */

    ec_slave_t *slave; /**< Slave pointer. This is \a NULL, if the slave is
                         offline. */

    ec_sync_config_t sync_configs[EC_MAX_SYNC_MANAGERS]; /**< Sync manager
                                                   configurations. */
    ec_fmmu_config_t fmmu_configs[EC_MAX_FMMUS]; /**< FMMU configurations. */
    uint8_t used_fmmus; /**< Number of FMMUs used. */
    uint16_t dc_assign_activate; /**< Vendor-specific AssignActivate word. */
    ec_sync_signal_t dc_sync[EC_SYNC_SIGNAL_COUNT]; /**< DC sync signals. */

    struct list_head sdo_configs; /**< List of SDO configurations. */
    struct list_head sdo_requests; /**< List of SDO requests. */
    struct list_head soe_requests; /**< List of SoE requests. */
    struct list_head voe_handlers; /**< List of VoE handlers. */
    struct list_head reg_requests; /**< List of register requests. */
    struct list_head soe_configs; /**< List of SoE configurations. */
    struct list_head flags; /**< List of feature flags. */
    struct list_head al_timeouts; /**< List of specific AL state timeouts. */

#ifdef EC_EOE
    ec_eoe_request_t eoe_ip_param_request; /**< EoE IP parameters. */
#endif

    ec_coe_emerg_ring_t emerg_ring; /**< CoE emergency ring buffer. */
};

/****************************************************************************/

void ec_slave_config_init(ec_slave_config_t *, ec_master_t *, uint16_t,
        uint16_t, uint32_t, uint32_t);
void ec_slave_config_clear(ec_slave_config_t *);

int ec_slave_config_attach(ec_slave_config_t *);
void ec_slave_config_detach(ec_slave_config_t *);

void ec_slave_config_load_default_sync_config(ec_slave_config_t *);

unsigned int ec_slave_config_sdo_count(const ec_slave_config_t *);
const ec_sdo_request_t *ec_slave_config_get_sdo_by_pos_const(
        const ec_slave_config_t *, unsigned int);
unsigned int ec_slave_config_idn_count(const ec_slave_config_t *);
const ec_soe_request_t *ec_slave_config_get_idn_by_pos_const(
        const ec_slave_config_t *, unsigned int);
unsigned int ec_slave_config_flag_count(const ec_slave_config_t *);
const ec_flag_t *ec_slave_config_get_flag_by_pos_const(
        const ec_slave_config_t *, unsigned int);
ec_sdo_request_t *ec_slave_config_find_sdo_request(ec_slave_config_t *,
        unsigned int);
ec_soe_request_t *ec_slave_config_find_soe_request(ec_slave_config_t *,
        unsigned int);
ec_reg_request_t *ec_slave_config_find_reg_request(ec_slave_config_t *,
        unsigned int);
ec_voe_handler_t *ec_slave_config_find_voe_handler(ec_slave_config_t *,
        unsigned int);
ec_flag_t *ec_slave_config_find_flag(ec_slave_config_t *, const char *);

ec_sdo_request_t *ecrt_slave_config_create_sdo_request_err(
        ec_slave_config_t *, uint16_t, uint8_t, size_t);
ec_soe_request_t *ecrt_slave_config_create_soe_request_err(
        ec_slave_config_t *, uint8_t, uint16_t, size_t);
ec_voe_handler_t *ecrt_slave_config_create_voe_handler_err(
        ec_slave_config_t *, size_t);
ec_reg_request_t *ecrt_slave_config_create_reg_request_err(
        ec_slave_config_t *, size_t);

unsigned int ec_slave_config_al_timeout(const ec_slave_config_t *,
        ec_slave_state_t, ec_slave_state_t);

/****************************************************************************/

#endif

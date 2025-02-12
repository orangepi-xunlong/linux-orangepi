/*****************************************************************************
 *
 *  Copyright (C) 2006-2021  Florian Pose, Ingenieurgemeinschaft IgH
 *
 *  This file is part of the IgH EtherCAT master.
 *
 *  The file is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU Lesser General Public License as published by the
 *  Free Software Foundation; version 2.1 of the License.
 *
 *  This file is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 *  License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this file. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

/** \file
 * Global definitions and macros.
 */

/****************************************************************************/

#ifndef __EC_MASTER_GLOBALS_H__
#define __EC_MASTER_GLOBALS_H__

#include "../include/globals.h"
#include "../include/ecrt.h"

/*****************************************************************************
 * EtherCAT master
 ****************************************************************************/

/** Datagram timeout in microseconds. */
#define EC_IO_TIMEOUT 500

/** Time to send a byte in nanoseconds.
 *
 * t_ns = 1 / (100 MBit/s / 8 bit/byte) = 80 ns/byte
 */
#define EC_BYTE_TRANSMISSION_TIME_NS 80

/** Number of state machine retries on datagram timeout. */
#define EC_FSM_RETRIES 3

/** Seconds to wait before fetching SDO dictionary
    after slave entered PREOP state. */
#define EC_WAIT_SDO_DICT 3

/** Minimum size of a buffer used with ec_state_string(). */
#define EC_STATE_STRING_SIZE 32

/** Maximum SII size in words, to avoid infinite reading. */
#define EC_MAX_SII_SIZE 4096

/** Number of statistic rate intervals to maintain. */
#define EC_RATE_COUNT 3

/*****************************************************************************
 * EtherCAT protocol
 ****************************************************************************/

/** Size of an EtherCAT frame header. */
#define EC_FRAME_HEADER_SIZE 2

/** Size of an EtherCAT datagram header. */
#define EC_DATAGRAM_HEADER_SIZE 10

/** Size of an EtherCAT datagram footer. */
#define EC_DATAGRAM_FOOTER_SIZE 2

/** Size of the EtherCAT address field. */
#define EC_ADDR_LEN 4

/** Resulting maximum data size of a single datagram in a frame. */
#define EC_MAX_DATA_SIZE (ETH_DATA_LEN - EC_FRAME_HEADER_SIZE \
                          - EC_DATAGRAM_HEADER_SIZE - EC_DATAGRAM_FOOTER_SIZE)

/** Mailbox header size.  */
#define EC_MBOX_HEADER_SIZE 6

/** Word offset of first SII category. */
#define EC_FIRST_SII_CATEGORY_OFFSET 0x40

/** Size of a sync manager configuration page. */
#define EC_SYNC_PAGE_SIZE 8

/** Maximum number of FMMUs per slave. */
#define EC_MAX_FMMUS 16

/** Size of an FMMU configuration page. */
#define EC_FMMU_PAGE_SIZE 16

/** Number of DC sync signals. */
#define EC_SYNC_SIGNAL_COUNT 2

/** Size of the datagram description string.
 *
 * This is also used as the maximum lenth of EoE device names.
 **/
#define EC_DATAGRAM_NAME_SIZE 20

/** Maximum hostname size.
 *
 * Used inside the EoE set IP parameter request.
 */
#define EC_MAX_HOSTNAME_SIZE 32

/** Slave state mask.
 *
 * Apply this mask to a slave state byte to get the slave state without
 * the error flag.
 */
#define EC_SLAVE_STATE_MASK 0x0F

/** State of an EtherCAT slave.
 */
typedef enum {
    EC_SLAVE_STATE_UNKNOWN = 0x00,
    /**< unknown state */
    EC_SLAVE_STATE_INIT = 0x01,
    /**< INIT state (no mailbox communication, no IO) */
    EC_SLAVE_STATE_PREOP = 0x02,
    /**< PREOP state (mailbox communication, no IO) */
    EC_SLAVE_STATE_BOOT = 0x03,
    /**< Bootstrap state (mailbox communication, firmware update) */
    EC_SLAVE_STATE_SAFEOP = 0x04,
    /**< SAFEOP (mailbox communication and input update) */
    EC_SLAVE_STATE_OP = 0x08,
    /**< OP (mailbox communication and input/output update) */
    EC_SLAVE_STATE_ACK_ERR = 0x10
    /**< Acknowledge/Error bit (no actual state) */
} ec_slave_state_t;

/** Supported mailbox protocols.
 *
 * Not to mix up with the mailbox type field in the mailbox header defined in
 * master/mailbox.h.
 */
enum {
    EC_MBOX_AOE = 0x01, /**< ADS over EtherCAT */
    EC_MBOX_EOE = 0x02, /**< Ethernet over EtherCAT */
    EC_MBOX_COE = 0x04, /**< CANopen over EtherCAT */
    EC_MBOX_FOE = 0x08, /**< File-Access over EtherCAT */
    EC_MBOX_SOE = 0x10, /**< Servo-Profile over EtherCAT */
    EC_MBOX_VOE = 0x20  /**< Vendor specific */
};

/** Slave information interface CANopen over EtherCAT details flags.
 */
typedef struct {
    uint8_t enable_sdo : 1; /**< Enable SDO access. */
    uint8_t enable_sdo_info : 1; /**< SDO information service available. */
    uint8_t enable_pdo_assign : 1; /**< PDO mapping configurable. */
    uint8_t enable_pdo_configuration : 1; /**< PDO configuration possible. */
    uint8_t enable_upload_at_startup : 1; /**< ?. */
    uint8_t enable_sdo_complete_access : 1; /**< Complete access possible. */
} ec_sii_coe_details_t;

/** Slave information interface general flags.
 */
typedef struct {
    uint8_t enable_safeop : 1; /**< ?. */
    uint8_t enable_not_lrw : 1; /**< Slave does not support LRW. */
} ec_sii_general_flags_t;

/** EtherCAT slave distributed clocks range.
 */
typedef enum {
    EC_DC_32, /**< 32 bit. */
    EC_DC_64 /*< 64 bit for system time, system time offset and
               port 0 receive time. */
} ec_slave_dc_range_t;

/** EtherCAT slave sync signal configuration.
 */
typedef struct {
    uint32_t cycle_time; /**< Cycle time [ns]. */
    int32_t shift_time; /**< Shift time [ns]. */
} ec_sync_signal_t;

/** Access states for SDO entries.
 *
 * The access rights are managed per AL state.
 */
enum {
    EC_SDO_ENTRY_ACCESS_PREOP, /**< Access rights in PREOP. */
    EC_SDO_ENTRY_ACCESS_SAFEOP, /**< Access rights in SAFEOP. */
    EC_SDO_ENTRY_ACCESS_OP, /**< Access rights in OP. */
    EC_SDO_ENTRY_ACCESS_COUNT /**< Number of states. */
};

/** Master devices.
 */
typedef enum {
    EC_DEVICE_MAIN, /**< Main device. */
    EC_DEVICE_BACKUP /**< Backup device */
} ec_device_index_t;

extern const char *ec_device_names[2]; // only main and backup!

/****************************************************************************/

/** Convenience macro for printing EtherCAT-specific information to syslog.
 *
 * This will print the message in \a fmt with a prefixed "EtherCAT: ".
 *
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_INFO(fmt, args...) \
    printk(KERN_INFO "EtherCAT: " fmt, ##args)

/** Convenience macro for printing EtherCAT-specific errors to syslog.
 *
 * This will print the message in \a fmt with a prefixed "EtherCAT ERROR: ".
 *
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_ERR(fmt, args...) \
    printk(KERN_ERR "EtherCAT ERROR: " fmt, ##args)

/** Convenience macro for printing EtherCAT-specific warnings to syslog.
 *
 * This will print the message in \a fmt with a prefixed "EtherCAT WARNING: ".
 *
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_WARN(fmt, args...) \
    printk(KERN_WARNING "EtherCAT WARNING: " fmt, ##args)

/** Convenience macro for printing EtherCAT debug messages to syslog.
 *
 * This will print the message in \a fmt with a prefixed "EtherCAT DEBUG: ".
 *
 * \param fmt format string (like in printf())
 * \param args arguments (optional)
 */
#define EC_DBG(fmt, args...) \
    printk(KERN_DEBUG "EtherCAT DEBUG: " fmt, ##args)

/****************************************************************************/

/** Absolute value.
 */
#define EC_ABS(X) ((X) >= 0 ? (X) : -(X))

/****************************************************************************/

extern char *ec_master_version_str;

/****************************************************************************/

unsigned int ec_master_count(void);
void ec_print_data(const uint8_t *, size_t);
void ec_print_data_diff(const uint8_t *, const uint8_t *, size_t);
size_t ec_state_string(uint8_t, char *, uint8_t);
ssize_t ec_mac_print(const uint8_t *, char *);
int ec_mac_is_zero(const uint8_t *);

ec_master_t *ecrt_request_master_err(unsigned int);

/****************************************************************************/

/** Code/Message pair.
 *
 * Some EtherCAT datagrams support reading a status code to display a certain
 * message. This type allows to map a code to a message string.
 */
typedef struct {
    uint32_t code; /**< Code. */
    const char *message; /**< Message belonging to \a code. */
} ec_code_msg_t;

/****************************************************************************/

/** Generic request state.
 *
 * \attention If ever changing this, please be sure to adjust the \a
 * state_table in master/sdo_request.c.
 */
typedef enum {
    EC_INT_REQUEST_INIT,
    EC_INT_REQUEST_QUEUED,
    EC_INT_REQUEST_BUSY,
    EC_INT_REQUEST_SUCCESS,
    EC_INT_REQUEST_FAILURE
} ec_internal_request_state_t;

/****************************************************************************/

extern const ec_request_state_t ec_request_state_translation_table[];

/****************************************************************************/

/** Origin type.
 */
typedef enum {
    EC_ORIG_INTERNAL, /**< Internal. */
    EC_ORIG_EXTERNAL /**< External. */
} ec_origin_t;

/****************************************************************************/

typedef struct ec_slave ec_slave_t; /**< \see ec_slave. */

/****************************************************************************/

#endif

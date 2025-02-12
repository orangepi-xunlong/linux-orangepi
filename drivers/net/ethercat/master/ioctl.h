/*****************************************************************************
 *
 *  Copyright (C) 2006-2024  Florian Pose, Ingenieurgemeinschaft IgH
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

/**
   \file
   EtherCAT master character device IOCTL commands.
*/

/****************************************************************************/

#ifndef __EC_IOCTL_H__
#define __EC_IOCTL_H__

#include <linux/ioctl.h>

#include "globals.h"

/****************************************************************************/

/** \cond */

#define EC_IOCTL_TYPE 0xa4

#define EC_IO(nr)           _IO(EC_IOCTL_TYPE, nr)
#define EC_IOR(nr, type)   _IOR(EC_IOCTL_TYPE, nr, type)
#define EC_IOW(nr, type)   _IOW(EC_IOCTL_TYPE, nr, type)
#define EC_IOWR(nr, type) _IOWR(EC_IOCTL_TYPE, nr, type)

/** EtherCAT master ioctl() version magic.
 *
 * Increment this when changing the ioctl interface!
 */
#define EC_IOCTL_VERSION_MAGIC 37

// Command-line tool
#define EC_IOCTL_MODULE                EC_IOR(0x00, ec_ioctl_module_t)
#define EC_IOCTL_MASTER                EC_IOR(0x01, ec_ioctl_master_t)
#define EC_IOCTL_SLAVE                EC_IOWR(0x02, ec_ioctl_slave_t)
#define EC_IOCTL_SLAVE_SYNC           EC_IOWR(0x03, ec_ioctl_slave_sync_t)
#define EC_IOCTL_SLAVE_SYNC_PDO       EC_IOWR(0x04, ec_ioctl_slave_sync_pdo_t)
#define EC_IOCTL_SLAVE_SYNC_PDO_ENTRY EC_IOWR(0x05, ec_ioctl_slave_sync_pdo_entry_t)
#define EC_IOCTL_DOMAIN               EC_IOWR(0x06, ec_ioctl_domain_t)
#define EC_IOCTL_DOMAIN_FMMU          EC_IOWR(0x07, ec_ioctl_domain_fmmu_t)
#define EC_IOCTL_DOMAIN_DATA          EC_IOWR(0x08, ec_ioctl_domain_data_t)
#define EC_IOCTL_MASTER_DEBUG           EC_IO(0x09)
#define EC_IOCTL_MASTER_RESCAN          EC_IO(0x0a)
#define EC_IOCTL_SLAVE_STATE           EC_IOW(0x0b, ec_ioctl_slave_state_t)
#define EC_IOCTL_SLAVE_SDO            EC_IOWR(0x0c, ec_ioctl_slave_sdo_t)
#define EC_IOCTL_SLAVE_SDO_ENTRY      EC_IOWR(0x0d, ec_ioctl_slave_sdo_entry_t)
#define EC_IOCTL_SLAVE_SDO_UPLOAD     EC_IOWR(0x0e, ec_ioctl_slave_sdo_upload_t)
#define EC_IOCTL_SLAVE_SDO_DOWNLOAD   EC_IOWR(0x0f, ec_ioctl_slave_sdo_download_t)
#define EC_IOCTL_SLAVE_SII_READ       EC_IOWR(0x10, ec_ioctl_slave_sii_t)
#define EC_IOCTL_SLAVE_SII_WRITE       EC_IOW(0x11, ec_ioctl_slave_sii_t)
#define EC_IOCTL_SLAVE_REG_READ       EC_IOWR(0x12, ec_ioctl_slave_reg_t)
#define EC_IOCTL_SLAVE_REG_WRITE       EC_IOW(0x13, ec_ioctl_slave_reg_t)
#define EC_IOCTL_SLAVE_FOE_READ       EC_IOWR(0x14, ec_ioctl_slave_foe_t)
#define EC_IOCTL_SLAVE_FOE_WRITE       EC_IOW(0x15, ec_ioctl_slave_foe_t)
#define EC_IOCTL_SLAVE_SOE_READ       EC_IOWR(0x16, ec_ioctl_slave_soe_read_t)
#define EC_IOCTL_SLAVE_SOE_WRITE      EC_IOWR(0x17, ec_ioctl_slave_soe_write_t)
#ifdef EC_EOE
#define EC_IOCTL_SLAVE_EOE_IP_PARAM    EC_IOW(0x18, ec_ioctl_eoe_ip_t)
#endif
#define EC_IOCTL_CONFIG               EC_IOWR(0x19, ec_ioctl_config_t)
#define EC_IOCTL_CONFIG_PDO           EC_IOWR(0x1a, ec_ioctl_config_pdo_t)
#define EC_IOCTL_CONFIG_PDO_ENTRY     EC_IOWR(0x1b, ec_ioctl_config_pdo_entry_t)
#define EC_IOCTL_CONFIG_SDO           EC_IOWR(0x1c, ec_ioctl_config_sdo_t)
#define EC_IOCTL_CONFIG_IDN           EC_IOWR(0x1d, ec_ioctl_config_idn_t)
#define EC_IOCTL_CONFIG_FLAG          EC_IOWR(0x1e, ec_ioctl_config_flag_t)
#ifdef EC_EOE
#define EC_IOCTL_CONFIG_EOE_IP_PARAM  EC_IOWR(0x1f, ec_ioctl_eoe_ip_t)
#define EC_IOCTL_EOE_HANDLER          EC_IOWR(0x20, ec_ioctl_eoe_handler_t)
#endif

// Application interface
#define EC_IOCTL_REQUEST                EC_IO(0x21)
#define EC_IOCTL_CREATE_DOMAIN          EC_IO(0x22)
#define EC_IOCTL_CREATE_SLAVE_CONFIG  EC_IOWR(0x23, ec_ioctl_config_t)
#define EC_IOCTL_SELECT_REF_CLOCK      EC_IOW(0x24, uint32_t)
#define EC_IOCTL_ACTIVATE              EC_IOR(0x25, ec_ioctl_master_activate_t)
#define EC_IOCTL_DEACTIVATE             EC_IO(0x26)
#define EC_IOCTL_SEND                   EC_IO(0x27)
#define EC_IOCTL_RECEIVE                EC_IO(0x28)
#define EC_IOCTL_MASTER_STATE          EC_IOR(0x29, ec_master_state_t)
#define EC_IOCTL_MASTER_LINK_STATE    EC_IOWR(0x2a, ec_ioctl_link_state_t)
#define EC_IOCTL_APP_TIME              EC_IOW(0x2b, uint64_t)
#define EC_IOCTL_SYNC_REF               EC_IO(0x2c)
#define EC_IOCTL_SYNC_REF_TO           EC_IOW(0x2d, uint64_t)
#define EC_IOCTL_SYNC_SLAVES            EC_IO(0x2e)
#define EC_IOCTL_REF_CLOCK_TIME        EC_IOR(0x2f, uint32_t)
#define EC_IOCTL_SYNC_MON_QUEUE         EC_IO(0x30)
#define EC_IOCTL_SYNC_MON_PROCESS      EC_IOR(0x31, uint32_t)
#define EC_IOCTL_RESET                  EC_IO(0x32)
#define EC_IOCTL_SC_SYNC               EC_IOW(0x33, ec_ioctl_config_t)
#define EC_IOCTL_SC_WATCHDOG           EC_IOW(0x34, ec_ioctl_config_t)
#define EC_IOCTL_SC_ADD_PDO            EC_IOW(0x35, ec_ioctl_config_pdo_t)
#define EC_IOCTL_SC_CLEAR_PDOS         EC_IOW(0x36, ec_ioctl_config_pdo_t)
#define EC_IOCTL_SC_ADD_ENTRY          EC_IOW(0x37, ec_ioctl_add_pdo_entry_t)
#define EC_IOCTL_SC_CLEAR_ENTRIES      EC_IOW(0x38, ec_ioctl_config_pdo_t)
#define EC_IOCTL_SC_REG_PDO_ENTRY     EC_IOWR(0x39, ec_ioctl_reg_pdo_entry_t)
#define EC_IOCTL_SC_REG_PDO_POS       EC_IOWR(0x3a, ec_ioctl_reg_pdo_pos_t)
#define EC_IOCTL_SC_DC                 EC_IOW(0x3b, ec_ioctl_config_t)
#define EC_IOCTL_SC_SDO                EC_IOW(0x3c, ec_ioctl_sc_sdo_t)
#define EC_IOCTL_SC_EMERG_SIZE         EC_IOW(0x3d, ec_ioctl_sc_emerg_t)
#define EC_IOCTL_SC_EMERG_POP         EC_IOWR(0x3e, ec_ioctl_sc_emerg_t)
#define EC_IOCTL_SC_EMERG_CLEAR        EC_IOW(0x3f, ec_ioctl_sc_emerg_t)
#define EC_IOCTL_SC_EMERG_OVERRUNS    EC_IOWR(0x40, ec_ioctl_sc_emerg_t)
#define EC_IOCTL_SC_SDO_REQUEST       EC_IOWR(0x41, ec_ioctl_sdo_request_t)
#define EC_IOCTL_SC_SOE_REQUEST       EC_IOWR(0x42, ec_ioctl_soe_request_t)
#define EC_IOCTL_SC_REG_REQUEST       EC_IOWR(0x43, ec_ioctl_reg_request_t)
#define EC_IOCTL_SC_VOE               EC_IOWR(0x44, ec_ioctl_voe_t)
#define EC_IOCTL_SC_STATE             EC_IOWR(0x45, ec_ioctl_sc_state_t)
#define EC_IOCTL_SC_IDN                EC_IOW(0x46, ec_ioctl_sc_idn_t)
#define EC_IOCTL_SC_FLAG               EC_IOW(0x47, ec_ioctl_sc_flag_t)
#ifdef EC_EOE
#define EC_IOCTL_SC_EOE_IP_PARAM       EC_IOW(0x48, ec_ioctl_eoe_ip_t)
#endif
#define EC_IOCTL_SC_STATE_TIMEOUT      EC_IOW(0x49, ec_ioctl_sc_state_timeout_t)
#define EC_IOCTL_DOMAIN_SIZE            EC_IO(0x4a)
#define EC_IOCTL_DOMAIN_OFFSET          EC_IO(0x4b)
#define EC_IOCTL_DOMAIN_PROCESS         EC_IO(0x4c)
#define EC_IOCTL_DOMAIN_QUEUE           EC_IO(0x4d)
#define EC_IOCTL_DOMAIN_STATE         EC_IOWR(0x4e, ec_ioctl_domain_state_t)
#define EC_IOCTL_SDO_REQUEST_INDEX    EC_IOWR(0x4f, ec_ioctl_sdo_request_t)
#define EC_IOCTL_SDO_REQUEST_TIMEOUT  EC_IOWR(0x50, ec_ioctl_sdo_request_t)
#define EC_IOCTL_SDO_REQUEST_STATE    EC_IOWR(0x51, ec_ioctl_sdo_request_t)
#define EC_IOCTL_SDO_REQUEST_READ     EC_IOWR(0x52, ec_ioctl_sdo_request_t)
#define EC_IOCTL_SDO_REQUEST_WRITE    EC_IOWR(0x53, ec_ioctl_sdo_request_t)
#define EC_IOCTL_SDO_REQUEST_DATA     EC_IOWR(0x54, ec_ioctl_sdo_request_t)
#define EC_IOCTL_SOE_REQUEST_IDN      EC_IOWR(0x55, ec_ioctl_soe_request_t)
#define EC_IOCTL_SOE_REQUEST_TIMEOUT  EC_IOWR(0x56, ec_ioctl_soe_request_t)
#define EC_IOCTL_SOE_REQUEST_STATE    EC_IOWR(0x57, ec_ioctl_soe_request_t)
#define EC_IOCTL_SOE_REQUEST_READ     EC_IOWR(0x58, ec_ioctl_soe_request_t)
#define EC_IOCTL_SOE_REQUEST_WRITE    EC_IOWR(0x59, ec_ioctl_soe_request_t)
#define EC_IOCTL_SOE_REQUEST_DATA     EC_IOWR(0x5a, ec_ioctl_soe_request_t)
#define EC_IOCTL_REG_REQUEST_DATA     EC_IOWR(0x5b, ec_ioctl_reg_request_t)
#define EC_IOCTL_REG_REQUEST_STATE    EC_IOWR(0x5c, ec_ioctl_reg_request_t)
#define EC_IOCTL_REG_REQUEST_WRITE    EC_IOWR(0x5d, ec_ioctl_reg_request_t)
#define EC_IOCTL_REG_REQUEST_READ     EC_IOWR(0x5e, ec_ioctl_reg_request_t)
#define EC_IOCTL_VOE_SEND_HEADER       EC_IOW(0x5f, ec_ioctl_voe_t)
#define EC_IOCTL_VOE_REC_HEADER       EC_IOWR(0x60, ec_ioctl_voe_t)
#define EC_IOCTL_VOE_READ              EC_IOW(0x61, ec_ioctl_voe_t)
#define EC_IOCTL_VOE_READ_NOSYNC       EC_IOW(0x62, ec_ioctl_voe_t)
#define EC_IOCTL_VOE_WRITE            EC_IOWR(0x63, ec_ioctl_voe_t)
#define EC_IOCTL_VOE_EXEC             EC_IOWR(0x64, ec_ioctl_voe_t)
#define EC_IOCTL_VOE_DATA             EC_IOWR(0x65, ec_ioctl_voe_t)
#define EC_IOCTL_SET_SEND_INTERVAL     EC_IOW(0x66, size_t)

/****************************************************************************/

#define EC_IOCTL_STRING_SIZE 64

/****************************************************************************/

typedef struct {
    uint32_t ioctl_version_magic;
    uint32_t master_count;
} ec_ioctl_module_t;

/****************************************************************************/

typedef struct {
    uint32_t slave_count;
    uint32_t scan_index;
    uint32_t config_count;
    uint32_t domain_count;
    uint32_t eoe_handler_count;
    uint8_t phase;
    uint8_t active;
    uint8_t scan_busy;
    struct ec_ioctl_device {
        uint8_t address[6];
        uint8_t attached;
        uint8_t link_state;
        uint64_t tx_count;
        uint64_t rx_count;
        uint64_t tx_bytes;
        uint64_t rx_bytes;
        uint64_t tx_errors;
        int32_t tx_frame_rates[EC_RATE_COUNT];
        int32_t rx_frame_rates[EC_RATE_COUNT];
        int32_t tx_byte_rates[EC_RATE_COUNT];
        int32_t rx_byte_rates[EC_RATE_COUNT];
    } devices[EC_MAX_NUM_DEVICES];
    uint32_t num_devices;
    uint64_t tx_count;
    uint64_t rx_count;
    uint64_t tx_bytes;
    uint64_t rx_bytes;
    int32_t tx_frame_rates[EC_RATE_COUNT];
    int32_t rx_frame_rates[EC_RATE_COUNT];
    int32_t tx_byte_rates[EC_RATE_COUNT];
    int32_t rx_byte_rates[EC_RATE_COUNT];
    int32_t loss_rates[EC_RATE_COUNT];
    uint64_t app_time;
    uint64_t dc_ref_time;
    uint16_t ref_clock;
} ec_ioctl_master_t;

/****************************************************************************/

typedef struct {
    // input
    uint16_t position;

    // outputs
    unsigned int device_index;
    uint32_t vendor_id;
    uint32_t product_code;
    uint32_t revision_number;
    uint32_t serial_number;
    uint16_t alias;
    uint16_t boot_rx_mailbox_offset;
    uint16_t boot_rx_mailbox_size;
    uint16_t boot_tx_mailbox_offset;
    uint16_t boot_tx_mailbox_size;
    uint16_t std_rx_mailbox_offset;
    uint16_t std_rx_mailbox_size;
    uint16_t std_tx_mailbox_offset;
    uint16_t std_tx_mailbox_size;
    uint16_t mailbox_protocols;
    uint8_t has_general_category;
    ec_sii_coe_details_t coe_details;
    ec_sii_general_flags_t general_flags;
    int16_t current_on_ebus;
    struct {
        ec_slave_port_desc_t desc;
        ec_slave_port_link_t link;
        uint32_t receive_time;
        uint16_t next_slave;
        uint32_t delay_to_next_dc;
    } ports[EC_MAX_PORTS];
    uint8_t fmmu_bit;
    uint8_t dc_supported;
    ec_slave_dc_range_t dc_range;
    uint8_t has_dc_system_time;
    uint32_t transmission_delay;
    uint8_t al_state;
    uint8_t error_flag;
    uint8_t sync_count;
    uint16_t sdo_count;
    uint32_t sii_nwords;
    char group[EC_IOCTL_STRING_SIZE];
    char image[EC_IOCTL_STRING_SIZE];
    char order[EC_IOCTL_STRING_SIZE];
    char name[EC_IOCTL_STRING_SIZE];
} ec_ioctl_slave_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint32_t sync_index;

    // outputs
    uint16_t physical_start_address;
    uint16_t default_size;
    uint8_t control_register;
    uint8_t enable;
    uint8_t pdo_count;
} ec_ioctl_slave_sync_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint32_t sync_index;
    uint32_t pdo_pos;

    // outputs
    uint16_t index;
    uint8_t entry_count;
    int8_t name[EC_IOCTL_STRING_SIZE];
} ec_ioctl_slave_sync_pdo_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint32_t sync_index;
    uint32_t pdo_pos;
    uint32_t entry_pos;

    // outputs
    uint16_t index;
    uint8_t subindex;
    uint8_t bit_length;
    int8_t name[EC_IOCTL_STRING_SIZE];
} ec_ioctl_slave_sync_pdo_entry_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t index;

    // outputs
    uint32_t data_size;
    uint32_t logical_base_address;
    uint16_t working_counter[EC_MAX_NUM_DEVICES];
    uint16_t expected_working_counter;
    uint32_t fmmu_count;
} ec_ioctl_domain_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t domain_index;
    uint32_t fmmu_index;

    // outputs
    uint16_t slave_config_alias;
    uint16_t slave_config_position;
    uint8_t sync_index;
    ec_direction_t dir;
    uint32_t logical_address;
    uint32_t data_size;
} ec_ioctl_domain_fmmu_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t domain_index;
    uint32_t data_size;
    uint8_t *target;
} ec_ioctl_domain_data_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint8_t al_state;
} ec_ioctl_slave_state_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint16_t sdo_position;

    // outputs
    uint16_t sdo_index;
    uint8_t max_subindex;
    int8_t name[EC_IOCTL_STRING_SIZE];
} ec_ioctl_slave_sdo_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    int sdo_spec; // positive: index, negative: list position
    uint8_t sdo_entry_subindex;

    // outputs
    uint16_t data_type;
    uint16_t bit_length;
    uint8_t read_access[EC_SDO_ENTRY_ACCESS_COUNT];
    uint8_t write_access[EC_SDO_ENTRY_ACCESS_COUNT];
    int8_t description[EC_IOCTL_STRING_SIZE];
} ec_ioctl_slave_sdo_entry_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint16_t sdo_index;
    uint8_t sdo_entry_subindex;
    size_t target_size;
    uint8_t *target;

    // outputs
    size_t data_size;
    uint32_t abort_code;
} ec_ioctl_slave_sdo_upload_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint16_t sdo_index;
    uint8_t sdo_entry_subindex;
    uint8_t complete_access;
    size_t data_size;
    uint8_t *data;

    // outputs
    uint32_t abort_code;
} ec_ioctl_slave_sdo_download_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint16_t offset;
    uint32_t nwords;
    uint16_t *words;
} ec_ioctl_slave_sii_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint8_t emergency;
    uint16_t address;
    size_t size;
    uint8_t *data;
} ec_ioctl_slave_reg_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint16_t offset;
    size_t buffer_size;
    uint8_t *buffer;

    // outputs
    size_t data_size;
    uint32_t result;
    uint32_t error_code;
    char file_name[32];
} ec_ioctl_slave_foe_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint8_t drive_no;
    uint16_t idn;
    size_t mem_size;
    uint8_t *data;

    // outputs
    size_t data_size;
    uint16_t error_code;
} ec_ioctl_slave_soe_read_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint16_t slave_position;
    uint8_t drive_no;
    uint16_t idn;
    size_t data_size;
    uint8_t *data;

    // outputs
    uint16_t error_code;
} ec_ioctl_slave_soe_write_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;

    // outputs
    uint16_t alias;
    uint16_t position;
    uint32_t vendor_id;
    uint32_t product_code;
    struct {
        ec_direction_t dir;
        ec_watchdog_mode_t watchdog_mode;
        uint32_t pdo_count;
        uint8_t config_this;
    } syncs[EC_MAX_SYNC_MANAGERS];
    uint16_t watchdog_divider;
    uint16_t watchdog_intervals;
    uint32_t sdo_count;
    uint32_t idn_count;
    uint32_t flag_count;
    int32_t slave_position;
    uint16_t dc_assign_activate;
    ec_sync_signal_t dc_sync[EC_SYNC_SIGNAL_COUNT];
} ec_ioctl_config_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    uint8_t sync_index;
    uint16_t pdo_pos;

    // outputs
    uint16_t index;
    uint8_t entry_count;
    int8_t name[EC_IOCTL_STRING_SIZE];
} ec_ioctl_config_pdo_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    uint8_t sync_index;
    uint16_t pdo_pos;
    uint8_t entry_pos;

    // outputs
    uint16_t index;
    uint8_t subindex;
    uint8_t bit_length;
    int8_t name[EC_IOCTL_STRING_SIZE];
} ec_ioctl_config_pdo_entry_t;

/****************************************************************************/

/** Maximum size for displayed SDO data.
 * \todo Make this dynamic.
 */
#define EC_MAX_SDO_DATA_SIZE 1024

typedef struct {
    // inputs
    uint32_t config_index;
    uint32_t sdo_pos;

    // outputs
    uint16_t index;
    uint8_t subindex;
    size_t size;
    uint8_t data[EC_MAX_SDO_DATA_SIZE];
    uint8_t complete_access;
} ec_ioctl_config_sdo_t;

/****************************************************************************/

/** Maximum size for displayed IDN data.
 * \todo Make this dynamic.
 */
#define EC_MAX_IDN_DATA_SIZE 1024

typedef struct {
    // inputs
    uint32_t config_index;
    uint32_t idn_pos;

    // outputs
    uint8_t drive_no;
    uint16_t idn;
    ec_al_state_t state;
    size_t size;
    uint8_t data[EC_MAX_IDN_DATA_SIZE];
} ec_ioctl_config_idn_t;

/****************************************************************************/

/** Maximum size for key.
 */
#define EC_MAX_FLAG_KEY_SIZE 128

typedef struct {
    // inputs
    uint32_t config_index;
    uint32_t flag_pos;

    // outputs
    char key[EC_MAX_FLAG_KEY_SIZE];
    int32_t value;
} ec_ioctl_config_flag_t;

/****************************************************************************/

#ifdef EC_EOE

typedef struct {
    // input
    uint16_t eoe_index;

    // outputs
    char name[EC_DATAGRAM_NAME_SIZE];
    uint16_t slave_position;
    uint8_t open;
    uint32_t rx_bytes;
    uint32_t rx_rate;
    uint32_t tx_bytes;
    uint32_t tx_rate;
    uint32_t tx_queued_frames;
    uint32_t tx_queue_size;
} ec_ioctl_eoe_handler_t;

#endif

/****************************************************************************/

#define EC_ETH_ALEN 6
#ifdef ETH_ALEN
#if ETH_ALEN != EC_ETH_ALEN
#error Ethernet address length mismatch
#endif
#endif

typedef struct {
    // input
    uint16_t slave_position;
    uint16_t config_index; // alternatively

    uint8_t mac_address_included;
    uint8_t ip_address_included;
    uint8_t subnet_mask_included;
    uint8_t gateway_included;
    uint8_t dns_included;
    uint8_t name_included;

    unsigned char mac_address[EC_ETH_ALEN];
    struct in_addr ip_address;
    struct in_addr subnet_mask;
    struct in_addr gateway;
    struct in_addr dns;
    char name[EC_MAX_HOSTNAME_SIZE];

	// output
	uint16_t result;
} ec_ioctl_eoe_ip_t;

/*****************************************************************************/

typedef struct {
    // outputs
    void *process_data;
    size_t process_data_size;
} ec_ioctl_master_activate_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    uint16_t pdo_index;
    uint16_t entry_index;
    uint8_t entry_subindex;
    uint8_t entry_bit_length;
} ec_ioctl_add_pdo_entry_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    uint16_t entry_index;
    uint8_t entry_subindex;
    uint32_t domain_index;

    // outputs
    unsigned int bit_position;
} ec_ioctl_reg_pdo_entry_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    uint32_t sync_index;
    uint32_t pdo_pos;
    uint32_t entry_pos;
    uint32_t domain_index;

    // outputs
    unsigned int bit_position;
} ec_ioctl_reg_pdo_pos_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    uint16_t index;
    uint8_t subindex;
    const uint8_t *data;
    size_t size;
    uint8_t complete_access;
} ec_ioctl_sc_sdo_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    size_t size;
    uint8_t *target;

    // outputs
    int32_t overruns;
} ec_ioctl_sc_emerg_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;

    // outputs
    ec_slave_config_state_t *state;
} ec_ioctl_sc_state_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    uint8_t drive_no;
    uint16_t idn;
    ec_al_state_t al_state;
    const uint8_t *data;
    size_t size;
} ec_ioctl_sc_idn_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    size_t key_size;
    char *key;
    int32_t value;
} ec_ioctl_sc_flag_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    ec_al_state_t from_state;
    ec_al_state_t to_state;
    uint32_t timeout_ms;
} ec_ioctl_sc_state_timeout_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t domain_index;

    // outputs
    ec_domain_state_t *state;
} ec_ioctl_domain_state_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;

    // inputs/outputs
    uint32_t request_index;
    uint16_t sdo_index;
    uint8_t sdo_subindex;
    size_t size;
    uint8_t *data;
    uint32_t timeout;
    ec_request_state_t state;
} ec_ioctl_sdo_request_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;

    // inputs/outputs
    uint32_t request_index;
    uint8_t drive_no;
    uint16_t idn;
    size_t size;
    uint8_t *data;
    uint32_t timeout;
    ec_request_state_t state;
} ec_ioctl_soe_request_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;
    size_t mem_size;

    // inputs/outputs
    uint32_t request_index;
    uint8_t *data;
    ec_request_state_t state;
    uint8_t new_data;
    uint16_t address;
    size_t transfer_size;
} ec_ioctl_reg_request_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t config_index;

    // inputs/outputs
    uint32_t voe_index;
    uint32_t *vendor_id;
    uint16_t *vendor_type;
    size_t size;
    uint8_t *data;
    ec_request_state_t state;
} ec_ioctl_voe_t;

/****************************************************************************/

typedef struct {
    // inputs
    uint32_t dev_idx;

    // outputs
    ec_master_link_state_t *state;
} ec_ioctl_link_state_t;

/****************************************************************************/

#ifdef __KERNEL__

/** Context data structure for file handles.
 */
typedef struct {
    unsigned int writable; /**< Device was opened with write permission. */
    unsigned int requested; /**< Master was requested via this file handle. */
    uint8_t *process_data; /**< Total process data area. */
    size_t process_data_size; /**< Size of the \a process_data. */
} ec_ioctl_context_t;

long ec_ioctl(ec_master_t *, ec_ioctl_context_t *, unsigned int,
        void __user *);

#ifdef EC_RTDM

long ec_ioctl_rtdm_rt(ec_master_t *, ec_ioctl_context_t *, unsigned int,
        void __user *);
long ec_ioctl_rtdm_nrt(ec_master_t *, ec_ioctl_context_t *, unsigned int,
        void __user *);

#ifndef EC_RTDM_XENOMAI_V3
int ec_rtdm_mmap(ec_ioctl_context_t *, void **);
#endif

#endif

#endif

/****************************************************************************/

/** \endcond */

#endif

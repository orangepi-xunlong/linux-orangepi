#ifndef _BT_HCI_CMDRH_H_
#define _BT_HCI_CMDRH_H_
#include <stdio.h>
#include "types.h"
#include "lib/ssv_lib.h"
#include "bt_hci_lib.h"

int bt_hci_cmdrh_ssv_reg_read(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_ssv_reg_write(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_ssv_ble_init(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_ssv_slave_subrate(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ssv_set_advertising_channel_priority(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ssv_acl_evt_to_external_host(
                        const int parameter_tbl_size,     bt_hci_parameter_entry*  const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry*  const ret_parameter_tbl);

int bt_hci_cmdrh_ssv_le_transmitter_test(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_dut_clean_buff(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_dut_check_payload(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_dut_query_adv_cnt(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_dut_query_num_of_packets_cnt(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_dut_query_data_buffer_overflow_cnt(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_dut_query_rx_acl_packets_cnt(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_transmitter_test(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_receiver_test(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_reset(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_le_drpb_reset(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_test_end (
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_get_system_status (
			const int parameter_tbl_size,	  bt_hci_parameter_entry* const parameter_tbl,
			const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_sleep_mode_configurations (
			const int parameter_tbl_size,	  bt_hci_parameter_entry* const parameter_tbl,
			const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_update_uart_hci_baudrate  (
			const int parameter_tbl_size,	  bt_hci_parameter_entry* const parameter_tbl,
			const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_set_le_test_mode_parameters(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_read_rssi(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_le_output_power(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_cont_tx_mode(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_write_bd_addr(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_pkt_tx_rx(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_ber_meter_start(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_ti_read_ber_meter_result(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_read_local_supported_features(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_read_local_supported_commands(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_read_local_version_information(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_read_local_supported_features(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_read_local_supported_states(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_read_remote_used_features(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_read_buffer_size(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_read_buffer_size(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_read_channel_map(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_set_random_address(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_read_bd_addr(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_set_event_mask(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_set_advertising_parameters(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_set_advertising_data(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_set_advertise_enable(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_set_scan_response_data(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_set_scan_parameters(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_set_scan_enable(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_set_event_mask(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_create_connection(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_create_connection_cancel(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_connection_update(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_set_host_channel_classification(
						const int parameter_tbl_size,	  bt_hci_parameter_entry* const parameter_tbl,
						const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_read_white_list_size(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_clear_white_list(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);
int bt_hci_cmdrh_le_add_device_to_white_list(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_remove_device_from_white_list(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_disconnect(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_read_remote_version_information(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_rand(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_encrypt(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_start_encryption(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_long_term_key_request_reply(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

int bt_hci_cmdrh_le_long_term_key_request_negative_reply(
                        const int parameter_tbl_size,     bt_hci_parameter_entry* const parameter_tbl,
                        const int ret_parameter_tbl_size, bt_hci_parameter_entry* const ret_parameter_tbl);

#endif

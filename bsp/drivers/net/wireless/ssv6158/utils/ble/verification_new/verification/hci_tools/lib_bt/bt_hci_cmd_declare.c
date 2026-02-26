#include <stdio.h>
#include "types.h"
#include "lib/ssv_lib.h"
#include "bt_hci_lib.h"
#include "bt_hci_cmdrh.h"

#define BT_HCI_CMD_DECLARE(CMD, OGF, OCF, NAME) \
bt_hci_cmd bt_hci_cmd_##CMD = {                                                                      \
    .opcode_group           = OGF,                                                                      \
    .opcode                 = CMD_OPCODE(OGF, OCF),                                                     \
    .parameter_tbl          = parameter_tbl_cmd_##CMD,                                                  \
    .parameter_tbl_size     = (sizeof(parameter_tbl_cmd_##CMD)/sizeof(bt_hci_parameter_entry)),      \
    .ret_parameter_tbl      = ret_parameter_tbl_cmd_##CMD,                                              \
    .ret_parameter_tbl_size = (sizeof(ret_parameter_tbl_cmd_##CMD)/sizeof(bt_hci_parameter_entry)),  \
    .ret_handle             = bt_hci_cmdrh_##CMD,                                                     \
    .name                   = NAME,                                                                     \
}

// ########################################
// # NOP parameter tables
// ########################################
bt_hci_parameter_entry  parameter_tbl_cmd_nop[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_nop[] = {
};

#define BT_HCI_CMD_NOP_DECLARE(CMD, OCF, NAME) BT_HCI_CMD_DECLARE(CMD, _BT_HCI_CMD_OGF_NOP_, _BT_HCI_CMD_OCF##OCF, NAME)

// BT_HCI_CMD_NOP_DECLARE(nop, _NOP_, "Nop");

// ########################################
// # link control commands define
// ########################################
bt_hci_parameter_entry  parameter_tbl_cmd_disconnect[] = {
{		2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL},		// Connection_Handle
{		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL},		// Reason
};

bt_hci_parameter_entry  parameter_tbl_cmd_read_remote_version_information[] = {
{		2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL},		// Connection_Handle
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_disconnect[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_remote_version_information[] = {
};

#define BT_HCI_CMD_LINK_CONTROL_DECLARE(CMD, OCF, NAME) BT_HCI_CMD_DECLARE(CMD, _BT_HCI_CMD_OGF_LINK_CONTROL_, _BT_HCI_CMD_OCF##OCF, NAME)

BT_HCI_CMD_LINK_CONTROL_DECLARE(disconnect                          ,_DISCONNECT_                       ,\
                                "Disconnect");
BT_HCI_CMD_LINK_CONTROL_DECLARE(read_remote_version_information     ,_READ_REMOTE_VERSION_INFORMATION_  ,\
                                "Read Remote Version Information");

// ########################################
// # control baseband command parameter tables
// ########################################
bt_hci_parameter_entry  parameter_tbl_cmd_set_event_mask[] = {
{		8,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL},		// LE_Event_Mask
};
bt_hci_parameter_entry  parameter_tbl_cmd_reset[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_ti_le_drpb_reset[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_read_transmit_power_level[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_set_controller_to_host_flow_control[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_host_buffer_size[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_host_number_of_completed_packets[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_read_le_host_support[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_write_le_host_support[] = {
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_set_event_mask[] = {
{		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL},		// Status
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_reset[] = {
{		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL},		// Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_ti_le_drpb_reset[] = {
{		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL},		// Status
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_transmit_power_level[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_set_controller_to_host_flow_control[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_host_buffer_size[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_host_number_of_completed_packets[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_le_host_support[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_write_le_host_support[] = {
};

#define BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(CMD, OCF, NAME) BT_HCI_CMD_DECLARE(CMD, _BT_HCI_CMD_OGF_CONTROL_BASEBAND_, _BT_HCI_CMD_OCF##OCF, NAME)

BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(set_event_mask                      ,_SET_EVENT_MASK_                     ,\
                                    "Set Event Mask"                     );
BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(reset                               ,_RESET_                              ,\
                                    "Reset"                              );
// BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(read_transmit_power_level           ,_READ_TRANSMIT_POWER_LEVEL_          ,\
//                                    "Read Transmit Power Level"          );
// BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(set_controller_to_host_flow_control ,_SET_CONTROLLER_TO_HOST_FLOW_CONTROL_,\
//                                    "Set Controller to Host Flow Control");
// BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(host_buffer_size                    ,_HOST_BUFFER_SIZE_                   ,\
//                                    "Host Buffer Size"                   );
// BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(host_number_of_completed_packets    ,_HOST_NUMBER_OF_COMPLETED_PACKETS_   ,\
//                                    "Host Number of Completed Packets"   );
// BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(read_le_host_support                ,_READ_LE_HOST_SUPPORT_               ,\
//                                    "Read LE Host Support"               );
// BT_HCI_CMD_CONTROL_BASEBAND_DECLARE(write_le_host_support               ,_WRITE_LE_HOST_SUPPORT_              ,\
//                                    "Write LE Host Support"              );

// ########################################
// # information parameters command parameter tables
// ########################################
bt_hci_parameter_entry  parameter_tbl_cmd_read_local_version_information[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_read_local_supported_features[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_read_local_supported_commands[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_read_buffer_size[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_read_bd_addr[] = {
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_local_version_information[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // HCI_Version
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// HCI_Revision
	{	1,  _BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// LMP/PAL_Version
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Manufacturer_Name
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// LMP/PAL_Subversion
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_local_supported_features[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
	{       8,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LMP_Features
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_local_supported_commands[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
	{      64,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // local supported commands
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_buffer_size[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
	{       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // HC_ACL_Data_Packet_Length:
	{	1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// HC_Synchronous_Data_Packet_Length
	{	2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// HC_Total_Num_ACL_Data_Packets:
	{	2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// HC_Total_Num_Synchronous_Data_Packets
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_bd_addr[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
	{       6,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // BD_ADDR
};

#define BT_HCI_CMD_INFORMATION_PARAMETERS_DECLARE(CMD, OCF, NAME) BT_HCI_CMD_DECLARE(CMD, _BT_HCI_CMD_OGF_INFORMATION_PARAMETERS_, _BT_HCI_CMD_OCF##OCF, NAME)

BT_HCI_CMD_INFORMATION_PARAMETERS_DECLARE(read_local_version_information     ,_LOCAL_VERSION_INFORMATION_    ,\
                                          "Read Local Version Information"    );
BT_HCI_CMD_INFORMATION_PARAMETERS_DECLARE(read_local_supported_features,_READ_LOCAL_SUPPORTED_FEATURES_,\
                                          "Read Local Supported Features");
BT_HCI_CMD_INFORMATION_PARAMETERS_DECLARE(read_local_supported_commands,_READ_LOCAL_SUPPORTED_COMMANDS_,\
                                          "Read Local Supported Commands");
BT_HCI_CMD_INFORMATION_PARAMETERS_DECLARE(read_buffer_size             ,_READ_BUFFER_SIZE_             ,\
                                          "Read Buffer Size"             );
BT_HCI_CMD_INFORMATION_PARAMETERS_DECLARE(read_bd_addr                 ,_READ_BD_ADDR_                 ,
                                          "Read BD_ADDR"                 );

// ########################################
// # status parameters command parameter tables
// ########################################
bt_hci_parameter_entry  parameter_tbl_cmd_read_rssi[] = {
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_read_rssi[] = {
};

#define BT_HCI_CMD_STATUS_PARAMETERS_DECLARE(CMD, OCF, NAME) BT_HCI_CMD_DECLARE(CMD, _BT_HCI_CMD_OGF_STATUS_PARAMETERS_, _BT_HCI_CMD_OCF##OCF, NAME)

// BT_HCI_CMD_STATUS_PARAMETERS_DECLARE(read_rssi ,_READ_RSSI_     ,"Read RSSI");

// ########################################
// # LE controller commands parameter tables
// ########################################
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_event_mask[] = {
    {       8,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LE_Event_Mask
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_read_buffer_size[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_read_local_supported_states[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_read_local_supported_features[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_random_address[] = {
	{       6,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Random_Addess
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_advertising_parameters[] = {
	{       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Advertising_Interval_Min
	{       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Advertising_Interval_Max
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Advertising_Type
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Own_Address_Type
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Direct_Address_Type
	{       6,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Direct_Address
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Advertising_Channel_Map
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Advertising_Filter_Policy
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_read_advertising_channel_tx_power[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_advertising_data[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Advertising_Data_Length
	{    31,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Advertising_Data:

};
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_scan_response_data[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Scan_Response_Data_Length
	{    31,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Scan_Response_Data
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_advertise_enable[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Advertising_Enable
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_scan_parameters[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LE_Scan_Type
	{       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LE_Scan_Interval
	{       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LE_Scan_Window
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Own_Address_Type
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Scanning_Filter_Policy
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_scan_enable[] = {
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LE_Scan_Enable
	{       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Filter_Duplicates
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_create_connection[] = {
	{       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },		// LE_Scan_Interval
	{       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },		// LE_Scan_Window:
	{	1,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Initiator_Filter_Policy
	{	1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },	// Peer_Address_Type
	{	6,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Peer_Address
	{	1,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Own_Address_Type
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Conn_Interval_Min:
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Conn_Interval_Max:
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Conn_Latency
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Supervision_Timeout
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Minimum_CE_Length
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Maximum_CE_Length
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_create_connection_cancel[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_read_white_list_size[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_clear_white_list[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_add_device_to_white_list[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Address_Type
    {	    6,  _BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Address
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_remove_device_from_white_list[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Address_Type
    {       6,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Address
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_connection_update[] = {
	{   2,  _BT_HCI_PARAMETER_PROP_DEF_,		NULL },     // Connection_Handle
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,		NULL },		// Conn_Interval_Min
	{	2  ,_BT_HCI_PARAMETER_PROP_DEF_,		NULL },		// Conn_Interval_Max
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,		NULL },		// Conn_Latency
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,		NULL },		// Supervision_Timeout
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,		NULL },		// Minimum_CE_Length
	{	2,  _BT_HCI_PARAMETER_PROP_DEF_,		NULL },		// Maximum_CE_Length
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_set_host_channel_classification[] = {
	{	5,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// LE_Channel_Map
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_read_channel_map[] = {
	{	2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Connection_Handle
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_read_remote_used_features[] = {
	{   2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Connection_Handle
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_encrypt[] = {
	{   16,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Key
	{   16,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Plaintext_Data
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_rand[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_start_encryption[] = {
	{	2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Connection_Handle
	{	8,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Random_Number
	{	2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Encrypted_Diversifier
	{	16,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Long_Term_Key
};

bt_hci_parameter_entry  parameter_tbl_cmd_le_long_term_key_request_reply[] = {
	{	2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Connection_Handle
	{	16,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Long Term Key
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_long_term_key_request_negative_reply[] = {
	{	2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Connection_Handle
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_read_supported_states[] = {
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_receiver_test[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // RX_Frequency
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_transmitter_test[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // TX_Frequency
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // Length_Of_Test_Data
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // Packet_Payload
};
bt_hci_parameter_entry  parameter_tbl_cmd_le_test_end[] = {
    /* length,  property,                       *value*/
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_event_mask[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_read_buffer_size[] = {
    {	    1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Status
    {	    2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// HC_LE_Data_Packet_Length:
    {	    1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// HC_Total_Num_LE_Data_Packets
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_read_local_supported_states[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
    {       8,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LE_States
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_read_local_supported_features[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
    {       8,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LE_Features
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_random_address[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_advertising_parameters[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_read_advertising_channel_tx_power[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_advertising_data[] = {
    {	    1,  _BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_scan_response_data[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_advertise_enable[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_scan_parameters[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_scan_enable[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_create_connection[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_create_connection_cancel[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_read_white_list_size[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
    {       1 , _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // size:
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_clear_white_list[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_add_device_to_white_list[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_remove_device_from_white_list[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_connection_update[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_set_host_channel_classification[] = {
    {	    1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },			// Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_read_channel_map[] = {
    {	    1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },     // Status
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Connection_Handle
    {       5,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // LE_Channel_Map
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_read_remote_used_features[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_encrypt[] = {
    {	    1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Status
    {	    16,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Encrypted_Data:
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_rand[] = {
    {	    1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Status
    {	    8,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Random_Number
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_start_encryption[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_long_term_key_request_reply[] = {
    {	    1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Status
    {	   2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Connection_Handle
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_long_term_key_request_negative_reply[] = {
    {	    1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Status
    {	   2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Connection_Handle

};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_read_supported_states[] = {
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_receiver_test[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_transmitter_test[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_le_test_end[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // Status
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // Number_of_Packets
};

#define BT_HCI_CMD_LE_CONTROLLER_DECLARE(CMD, OCF, NAME) BT_HCI_CMD_DECLARE(CMD, _BT_HCI_CMD_OGF_LE_CONTROLLER_, _BT_HCI_CMD_OCF##OCF, NAME)

BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_event_mask                          ,_LE_SET_EVENT_MASK_                        ,\
                                 "LE set event mask"                      );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_read_buffer_size                        ,_LE_READ_BUFFER_SIZE_                      ,\
                                 "LE read buffer size"                    );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_read_local_supported_features           ,_LE_READ_LOCAL_SUPPORTED_FEATURES_         ,\
                                 "LE read local supported features"       );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_read_local_supported_states           ,_LE_READ_SUPPORTED_STATES_         ,\
                                 "LE read local supported states"       );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_random_address                      ,_LE_SET_RANDOM_ADDRESS_                    ,\
                                 "LE set random address"                  );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_advertising_parameters              ,_LE_SET_ADVERTISING_PARAMETERS_            ,\
                                 "LE set advertising parameters"          );
// BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_read_advertising_channel_tx_power       ,_LE_READ_ADVERTISING_CHANNEL_TX_POWER_     ,\
//                                 "LE read advertising channel tx power"   );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_advertising_data                    ,_LE_SET_ADVERTISING_DATA_                  ,\
                                 "LE set advertising data"                );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_scan_response_data                  ,_LE_SET_SCAN_RESPONSE_DATA_                ,\
                                 "LE set scan response data"              );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_advertise_enable                    ,_LE_SET_ADVERTISE_ENABLE_                  ,\
                                 "LE set advertise enable"                );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_scan_parameters                     ,_LE_SET_SCAN_PARAMETERS_                   ,\
                                 "LE set scan parameters"                 );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_scan_enable                         ,_LE_SET_SCAN_ENABLE_                       ,\
                                 "LE set scan enable"                     );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_create_connection                       ,_LE_CREATE_CONNECTION_                     ,\
                                 "LE create connection"                   );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_create_connection_cancel                ,_LE_CREATE_CONNECTION_CANCEL_              ,\
                                 "LE create connection cancel"            );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_read_white_list_size                    ,_LE_READ_WHITE_LIST_SIZE_                  ,\
                                 "LE read white list size"                );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_clear_white_list                        ,_LE_CLEAR_WHITE_LIST_                      ,\
                                 "LE clear white list"                    );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_add_device_to_white_list                ,_LE_ADD_DEVICE_TO_WHITE_LIST_              ,\
                                 "LE add device to white list"            );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_remove_device_from_white_list           ,_LE_REMOVE_DEVICE_FROM_WHITE_LIST_         ,\
                                 "LE remove device from white list"       );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_connection_update                       ,_LE_CONNECTION_UPDATE_                     ,\
                                 "LE connection update"                   );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_set_host_channel_classification         ,_LE_SET_HOST_CHANNEL_CLASSIFICATION_       ,\
                                 "LE set host channel classification"     );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_read_channel_map                        ,_LE_READ_CHANNEL_MAP_                      ,\
                                 "LE read channel map"                    );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_read_remote_used_features               ,_LE_READ_REMOTE_USED_FEATURES_             ,\
                                 "LE read remote used features"           );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_encrypt                                 ,_LE_ENCRYPT_                               ,\
                                 "LE encrypt"                             );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_rand                                    ,_LE_RAND_                                  ,\
                                 "LE rand"                                );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_start_encryption                        ,_LE_START_ENCRYPTION_                      ,\
                                 "LE start encryption"                    );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_long_term_key_request_reply             ,_LE_LONG_TERM_KEY_REQUEST_REPLY_           ,\
                                 "LE long term key request reply"         );
BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_long_term_key_request_negative_reply    ,_LE_LONG_TERM_KEY_REQUEST_NEGATIVE_REPLY_  ,\
                                 "LE long term key request negative reply");
// BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_read_supported_states                   ,_LE_READ_SUPPORTED_STATES_                 ,\
//                                 "LE read supported states"               );
 BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_receiver_test                           ,_LE_RECEIVER_TEST_                         ,\
                                 "LE receiver test"                       );
 BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_transmitter_test                        ,_LE_TRANSMITTER_TEST_                      ,\
                                 "LE transmitter test"                    );
 BT_HCI_CMD_LE_CONTROLLER_DECLARE(le_test_end                                ,_LE_TEST_END_                              ,\
                                 "LE test end"                            );

// ########################################
// # TI-VS command parameter tables
// ########################################
bt_hci_parameter_entry parameter_tbl_cmd_ti_set_le_test_mode_parameters[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // TX_Power_Level
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // RX_Mode
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Packets_to_transmit
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Access_code
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // PER_BER_Test_Enable
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // BER_Test_Pattern
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // BER_Test_Packet_Length
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // BER_FA_Threshold
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Trace_Enable
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Reference_CRC
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_set_le_test_mode_parameters[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};

bt_hci_parameter_entry parameter_tbl_cmd_ti_read_rssi[] = {
    /* length,  property,                       *value*/
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Connection Handle
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_read_rssi[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Connection Handle
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // RSSI
};


bt_hci_parameter_entry parameter_tbl_cmd_ti_sleep_mode_configurations[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Reserved
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Deep sleep enable
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Deep sleep mode
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Output I/O to select
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Output pull enable
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Input pull enable
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Input I/O select
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Reserved
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_sleep_mode_configurations[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};


bt_hci_parameter_entry parameter_tbl_cmd_ti_get_system_status[] = {
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_get_system_status[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Software version Major
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Software version Internal
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Chip revision
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		4,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },
};

bt_hci_parameter_entry parameter_tbl_cmd_ti_update_uart_hci_baudrate[] = {
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Baudrate
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_update_uart_hci_baudrate[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};

bt_hci_parameter_entry parameter_tbl_cmd_ti_le_output_power[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Power Level Index
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_le_output_power[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};

bt_hci_parameter_entry parameter_tbl_cmd_ti_cont_tx_mode[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Modulation
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Test Pattern
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Frequency
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Power Level
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Reserved
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Reserved
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_cont_tx_mode[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};

bt_hci_parameter_entry parameter_tbl_cmd_ti_write_bd_addr[] = {
    /* length,  property,                       *value*/
    {       6,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // bd address
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_write_bd_addr[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};

bt_hci_parameter_entry parameter_tbl_cmd_ti_pkt_tx_rx[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Frequency
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Tx single-channel
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Rx single-channel
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // ACL packet type
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // ACL packet pattern
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Reserved
    {		2,	_BT_HCI_PARAMETER_PROP_DEF_,	    NULL },     // ACL packet length
	{		1,	_BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Power Level
	{		1,	_BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Enable/Disable whitening
	{		2,	_BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// PRBS9 initial value
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_pkt_tx_rx[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};

bt_hci_parameter_entry parameter_tbl_cmd_ti_ber_meter_start[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Frequency
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Reserved
    {       6,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // bd address
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // lt address
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // ACL packet type
    {		2,	_BT_HCI_PARAMETER_PROP_DEF_,	    NULL },     // ACL packet length
	{		2,	_BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// Number of packets
	{		2,	_BT_HCI_PARAMETER_PROP_DEF_,	    NULL },	// PRBS9 initial value
	{		1,	_BT_HCI_PARAMETER_PROP_DEF_,		NULL },		// Poll period

};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_ber_meter_start[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
};

bt_hci_parameter_entry parameter_tbl_cmd_ti_read_ber_meter_result[] = {
};

bt_hci_parameter_entry ret_parameter_tbl_cmd_ti_read_ber_meter_result[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Status
    {		1,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Finished at least one test
    {		2,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Number of packets received
    {		4,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Total bits counted
    {		4,	_BT_HCI_PARAMETER_PROP_DEF_,	NULL },		// Number of errors found
};


#define BT_HCI_CMD_TI_SPECIFIC_DECLARE(CMD, OCF, NAME) BT_HCI_CMD_DECLARE(CMD, _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_, _BT_HCI_CMD_OCF##OCF, NAME)
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_set_le_test_mode_parameters,  _TI_SET_LE_TEST_MODE_PARAMETERS_,   "TI LE Test Mode Parameters");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_read_rssi,                    _TI_READ_RSSI_,                     "TI Read RSSI");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_get_system_status,            _TI_GET_SYSTEM_STATUS_,             "TI Get System Status");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_sleep_mode_configurations,    _TI_SLEEP_MODE_CONFIGURATIONS_,     "TI Sleep Mode Configurations");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_update_uart_hci_baudrate,     _TI_UPDATE_UART_HCI_BAUDRATE_,      "TI Update Uart Hci Baudrate");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_le_drpb_reset,                _TI_DRPB_RESET_,                    "TI LE DRPb Reset");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_le_output_power,              _TI_LE_OUTPUT_POWER_,               "TI LE Output Power");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_cont_tx_mode,                 _TI_CONT_TX_MODE_,                  "TI Cont Tx Mode");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_write_bd_addr,                _TI_WRITE_BD_ADDR_,                 "TI Write BD Addr");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_pkt_tx_rx,                    _TI_PKT_TX_RX_,                     "TI Packet Tx Rx");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_ber_meter_start,              _TI_BER_METER_START_,               "TI BER Meter Start");
BT_HCI_CMD_TI_SPECIFIC_DECLARE(ti_read_ber_meter_result,        _TI_READ_BER_METER_RESULT_,         "TI Read BER Meter Result");



// ########################################
// # SSV-VS command parameter tables
// ########################################
bt_hci_parameter_entry  parameter_tbl_cmd_ssv_reg_read[] = {
    /* length,  property,                       *value*/
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  Address
};
bt_hci_parameter_entry  parameter_tbl_cmd_ssv_reg_write[] = {
    /* length,  property,                       *value*/
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  Address
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  Write_Value
};
bt_hci_parameter_entry  parameter_tbl_cmd_ssv_ble_init[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },
};

bt_hci_parameter_entry  parameter_tbl_cmd_ssv_slave_subrate[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     //  Enable
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     //  Minimum_Tx_Latency

};

bt_hci_parameter_entry  parameter_tbl_cmd_ssv_set_advertising_channel_priority[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     //
};

bt_hci_parameter_entry  parameter_tbl_cmd_ssv_acl_evt_to_external_host[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },
};

bt_hci_parameter_entry  parameter_tbl_cmd_dut_clean_buff[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL }
};

bt_hci_parameter_entry  parameter_tbl_cmd_dut_check_payload[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL }
};

bt_hci_parameter_entry  parameter_tbl_cmd_dut_query_adv_cnt[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL }
};

bt_hci_parameter_entry  parameter_tbl_cmd_dut_query_num_of_packets_cnt[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL }
};

bt_hci_parameter_entry  parameter_tbl_cmd_dut_query_data_buffer_overflow_cnt[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL }
};

bt_hci_parameter_entry  parameter_tbl_cmd_dut_query_rx_acl_packets_cnt[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL }
};

bt_hci_parameter_entry  parameter_tbl_cmd_ssv_le_transmitter_test[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // TX_Frequency
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // Packet_Num
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // Length_Of_Test_Data
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL},      // Packet_Payload
};


bt_hci_parameter_entry  ret_parameter_tbl_cmd_ssv_reg_read[] = {
    /* length,  property,                       *value*/
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  Read_Value
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_ssv_reg_write[] = {
    /* length,  property,                       *value*/
    {       4,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  Read_Value
};
bt_hci_parameter_entry  ret_parameter_tbl_cmd_ssv_ble_init[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_ssv_slave_subrate[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  status
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_ssv_acl_evt_to_external_host[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  status
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_ssv_set_advertising_channel_priority[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  status
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_ssv_le_transmitter_test[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   //  return value
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_dut_clean_buff[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_dut_check_payload[] = {
    /* length,  property,                       *value*/
    {       0,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_dut_query_adv_cnt[] = {
    /* length,  property,                       *value*/
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   // Adv  Ind
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },   // Scan Rsp

};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_dut_query_num_of_packets_cnt[] = {
    /* length,  property,                       *value*/
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_dut_query_data_buffer_overflow_cnt[] = {
    /* length,  property,                       *value*/
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },
};

bt_hci_parameter_entry  ret_parameter_tbl_cmd_dut_query_rx_acl_packets_cnt[] = {
    /* length,  property,                       *value*/
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },
};

#define BT_HCI_CMD_SSV_SPECIFIC_DECLARE(CMD, OCF, NAME) BT_HCI_CMD_DECLARE(CMD, _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_, _BT_HCI_CMD_OCF##OCF, NAME)

BT_HCI_CMD_SSV_SPECIFIC_DECLARE(ssv_reg_read    ,_SSV_REG_READ_     ,"SSV reg read"     );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(ssv_reg_write   ,_SSV_REG_WRITE_    ,"SSV reg write"    );

BT_HCI_CMD_SSV_SPECIFIC_DECLARE(ssv_ble_init        ,_SSV_BLE_INIT_     ,"SSV ble init"    );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(ssv_slave_subrate   ,_SSV_SLAVE_SUBRATE_     ,"SSV slave subrate"    );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(ssv_set_advertising_channel_priority   ,_SSV_SET_ADVERTISING_CHANNEL_PRIORITY_     ,"SSV set adv channel priority"    );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(ssv_acl_evt_to_external_host   ,_SSV_ACL_EVT_TO_EXTERNAL_HOST_     ,"ssv_acl_evt_to_external_host"    );

BT_HCI_CMD_SSV_SPECIFIC_DECLARE(ssv_le_transmitter_test ,_SSV_LE_TRANSMITTER_TEST_    ,"SSV Le transmitter test"    );

BT_HCI_CMD_SSV_SPECIFIC_DECLARE(dut_clean_buff      ,_DUT_CLEAN_BUFFER_  ,"DUT clean buffer"    );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(dut_check_payload   ,_DUT_CHECK_PAYLOAD_ ,"DUT check payload"    );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(dut_query_adv_cnt   ,_DUT_QUERY_ADV_CNT_ ,"DUT query adv_cnt"    );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(dut_query_num_of_packets_cnt   ,_DUT_QUERY_NUM_OF_PACKETS_CNT_ ,"DUT query num of packets cnt"    );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(dut_query_data_buffer_overflow_cnt   ,_DUT_QUERY_DATA_BUFFER_OVERFLOW_CNT_ ,"DUT query data buffer overflow cnt"    );
BT_HCI_CMD_SSV_SPECIFIC_DECLARE(dut_query_rx_acl_packets_cnt   ,_DUT_QUERY_RX_ACL_PACKETS_CNT_ ,"DUT query rx acl packets cnt"    );

// ########################################
// # command tables
// ########################################
bt_hci_cmd* bt_hci_cmd_tbl[] = {
    &bt_hci_cmd_le_transmitter_test,
    &bt_hci_cmd_le_receiver_test,
    &bt_hci_cmd_le_test_end,
    &bt_hci_cmd_ti_get_system_status,
    &bt_hci_cmd_ti_sleep_mode_configurations,
    &bt_hci_cmd_ti_update_uart_hci_baudrate,
    &bt_hci_cmd_ti_read_rssi,
    &bt_hci_cmd_ti_le_output_power,
    &bt_hci_cmd_ti_cont_tx_mode,
    &bt_hci_cmd_ti_write_bd_addr,
    &bt_hci_cmd_ti_pkt_tx_rx,
    &bt_hci_cmd_ti_ber_meter_start,
    &bt_hci_cmd_ti_read_ber_meter_result,

    &bt_hci_cmd_reset,
    &bt_hci_cmd_read_local_version_information,
    &bt_hci_cmd_read_local_supported_features,
    &bt_hci_cmd_read_local_supported_commands,

    &bt_hci_cmd_le_read_local_supported_features,
    &bt_hci_cmd_le_read_local_supported_states,
    &bt_hci_cmd_le_read_remote_used_features,

    &bt_hci_cmd_le_read_buffer_size,
    &bt_hci_cmd_le_set_random_address,
    &bt_hci_cmd_read_buffer_size,
    &bt_hci_cmd_read_bd_addr,
    &bt_hci_cmd_set_event_mask,
    &bt_hci_cmd_le_set_advertising_parameters,
    &bt_hci_cmd_le_set_advertising_data,
    &bt_hci_cmd_le_set_advertise_enable,
    &bt_hci_cmd_le_set_scan_response_data,
    &bt_hci_cmd_le_set_scan_parameters,
    &bt_hci_cmd_le_set_scan_enable,
    &bt_hci_cmd_le_set_event_mask,
    &bt_hci_cmd_le_create_connection,
    &bt_hci_cmd_le_read_white_list_size,
    &bt_hci_cmd_le_clear_white_list,
    &bt_hci_cmd_le_add_device_to_white_list,
    &bt_hci_cmd_le_remove_device_from_white_list,
    &bt_hci_cmd_le_connection_update,
    &bt_hci_cmd_disconnect,
    &bt_hci_cmd_read_remote_version_information,
    &bt_hci_cmd_le_rand,
    &bt_hci_cmd_le_encrypt,
    &bt_hci_cmd_le_start_encryption,
    &bt_hci_cmd_le_long_term_key_request_reply,
    &bt_hci_cmd_le_long_term_key_request_negative_reply,
    &bt_hci_cmd_ssv_reg_read,
    &bt_hci_cmd_ssv_reg_write,
    &bt_hci_cmd_ssv_ble_init,
    &bt_hci_cmd_ssv_slave_subrate,
    &bt_hci_cmd_ssv_set_advertising_channel_priority,
    &bt_hci_cmd_ssv_acl_evt_to_external_host,

    &bt_hci_cmd_ssv_le_transmitter_test,
    &bt_hci_cmd_le_set_host_channel_classification,
    &bt_hci_cmd_le_read_channel_map,

    &bt_hci_cmd_dut_clean_buff,
    &bt_hci_cmd_dut_check_payload,
    &bt_hci_cmd_dut_query_adv_cnt,
    &bt_hci_cmd_dut_query_num_of_packets_cnt,
    &bt_hci_cmd_dut_query_data_buffer_overflow_cnt,
    &bt_hci_cmd_dut_query_rx_acl_packets_cnt,
};
const int bt_hci_cmd_tbl_size = (sizeof(bt_hci_cmd_tbl)/sizeof(bt_hci_cmd*));

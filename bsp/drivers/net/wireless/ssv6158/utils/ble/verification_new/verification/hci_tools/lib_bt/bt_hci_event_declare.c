#include <stdio.h>
#include <types.h>

#include <lib/ssv_lib.h>
#include "bt_hci_lib.h"
#include "bt_hci_eventh.h"

#define BT_HCI_EVENT_DECLARE(EVENT, OPCODE, NAME)  \
bt_hci_event bt_hci_event_##EVENT = {              \
    .opcode             = _BT_HCI_EVENT_OP##OPCODE,                                                               \
    .parameter_tbl      = parameter_tbl_event_##EVENT,                                          \
    .parameter_tbl_size = (sizeof(parameter_tbl_event_##EVENT)/sizeof(bt_hci_parameter_entry)), \
    .handle             = bt_hci_eventh_##EVENT,                                            \
    .name               = NAME                                                                  \
}

#define BT_HCI_LE_EVENT_DECLARE(EVENT, OPCODE, NAME)  \
bt_hci_le_event bt_hci_le_event_##EVENT = {\
    .sub_opcode         = _BT_HCI_LE_EVENT_SUB_OP##OPCODE,                                                               \
    .parameter_tbl      = parameter_tbl_le_event_##EVENT,                                          \
    .parameter_tbl_size = (sizeof(parameter_tbl_le_event_##EVENT)/sizeof(bt_hci_parameter_entry)), \
    .handle             = bt_hci_le_eventh_##EVENT,                                         \
    .name               = NAME                                                                  \
}

bt_hci_parameter_entry  parameter_tbl_event_disconnection_complete[] = {
};
bt_hci_parameter_entry  parameter_tbl_event_encryption_change[] = {
};
bt_hci_parameter_entry  parameter_tbl_event_read_remote_version_information_complete[] = {
};
bt_hci_parameter_entry  parameter_tbl_event_command_complete[] = {
    /* length,  property,                       *value*/
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Num_HCI_Command_Packets
    {       2,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Command_Opcode
    {       0,  _BT_HCI_PARAMETER_PROP_VLEN_,   NULL },     // Return_Parameters
};
bt_hci_parameter_entry  parameter_tbl_event_command_status[] = {
};
bt_hci_parameter_entry  parameter_tbl_event_hardware_error[] = {
};
bt_hci_parameter_entry  parameter_tbl_event_number_of_completed_packets[] = {
};
bt_hci_parameter_entry  parameter_tbl_event_data_buffer_overflow[] = {
};
bt_hci_parameter_entry  parameter_tbl_event_encryption_key_refresh_complete[] = {
};
bt_hci_parameter_entry  parameter_tbl_event_le_event[] = {
};

bt_hci_parameter_entry  parameter_tbl_le_event_connection_complete[] = {
};
bt_hci_parameter_entry  parameter_tbl_le_event_advertising_report[] = {
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Subevent_Code
    {       1,  _BT_HCI_PARAMETER_PROP_DEF_,    NULL },     // Num_Reports
    {       0,  _BT_HCI_PARAMETER_PROP_VLEN_,   NULL },     //Event_Type[i]
    {       0,  _BT_HCI_PARAMETER_PROP_VLEN_,    NULL },     // Address_Type[i]:
    {       0,  _BT_HCI_PARAMETER_PROP_VLEN_,    NULL },     // Address[i]
    {       0,  _BT_HCI_PARAMETER_PROP_VLEN_,   NULL },     //Length_Data[i]:
    {       0,  _BT_HCI_PARAMETER_PROP_VLEN_,    NULL },     //Data[i]
    {       0,  _BT_HCI_PARAMETER_PROP_VLEN_,    NULL },     //RSSI[i]:
};
bt_hci_parameter_entry  parameter_tbl_le_event_connection_update_complete[] = {
};
bt_hci_parameter_entry  parameter_tbl_le_event_read_remote_used_features_complete[] = {
};
bt_hci_parameter_entry  parameter_tbl_le_event_long_term_key_request[] = {
};

// BT_HCI_EVENT_DECLARE(disconnection_complete                    ,_DISCONNECTION_COMPLETE_                   ,\
//                     "Disconnection Complete"                  );
// BT_HCI_EVENT_DECLARE(encryption_change                         ,_ENCRYPTION_CHANGE_                        ,\
//                     "Encryption Change"                       );
// BT_HCI_EVENT_DECLARE(read_remote_version_information_complete  ,_READ_REMOTE_VERSION_INFORMATION_COMPLETE_ ,\
//                     "Read Remote Version Information Complete");
BT_HCI_EVENT_DECLARE(command_complete                          ,_COMMAND_COMPLETE_                         ,\
                    "Command Complete"                        );
// BT_HCI_EVENT_DECLARE(command_status                            ,_COMMAND_STATUS_                           ,\
//                     "Command Status"                          );
// BT_HCI_EVENT_DECLARE(hardware_error                            ,_HARDWARE_ERROR_                           ,\
//                     "Hardware Error"                          );
// BT_HCI_EVENT_DECLARE(number_of_completed_packets               ,_NUMBER_OF_COMPLETED_PACKETS_              ,\
//                     "Number of Completed Packets"             );
// BT_HCI_EVENT_DECLARE(data_buffer_overflow                      ,_DATA_BUFFER_OVERFLOW_                     ,\
//                     "Data Buffer Overflow"                    );
// BT_HCI_EVENT_DECLARE(encryption_key_refresh_complete           ,_ENCRYPTION_KEY_REFRESH_COMPLETE_          ,\
//                     "Encryption Key Refresh Complete"         );
// BT_HCI_EVENT_DECLARE(le_event                                  ,_LE_EVENT_                                 ,\
//                     "LE Event"                                );

// BT_HCI_LE_EVENT_DECLARE(connection_complete               ,_CONNECTION_COMPLETE_                   ,\
//                     "LE connection complete"               );
BT_HCI_LE_EVENT_DECLARE(advertising_report                ,_ADVERTISING_REPORT_                    ,\
                     "LE advertising report"                );
// BT_HCI_LE_EVENT_DECLARE(connection_update_complete        ,_CONNECTION_UPDATE_COMPLETE_            ,\
//                     "LE connection update complete"        );
// BT_HCI_LE_EVENT_DECLARE(read_remote_used_features_complete,_READ_REMOTE_USED_FEATURES_COMPLETE_    ,\
//                     "LE read remote used features complete");
// BT_HCI_LE_EVENT_DECLARE(long_term_key_request             ,_LONG_TERM_KEY_REQUEST_                 ,\
//                     "LE long term key request"             );

bt_hci_event* bt_hci_event_tbl[1] = {
    &bt_hci_event_command_complete,
};

bt_hci_le_event* bt_hci_le_event_tbl[1] = {
    &bt_hci_le_event_advertising_report,
};

const int bt_hci_event_tbl_size = (sizeof(bt_hci_event_tbl)/sizeof(bt_hci_event*));
const int bt_hci_le_event_tbl_size = (sizeof(bt_hci_le_event_tbl)/sizeof(bt_hci_le_event*));

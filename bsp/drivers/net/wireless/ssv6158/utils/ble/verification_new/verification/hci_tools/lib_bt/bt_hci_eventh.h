#ifndef _BT_HCI_EVENTH_H_
#define _BT_HCI_EVENTH_H_
#include "lib/ssv_lib.h"
#include "bt_hci_lib.h"

int bt_hci_eventh_command_complete (const int parameter_tbl_size, bt_hci_parameter_entry* const parameter_tbl);
int bt_hci_le_eventh_advertising_report (const int parameter_tbl_size, bt_hci_parameter_entry* const parameter_tbl);

#endif

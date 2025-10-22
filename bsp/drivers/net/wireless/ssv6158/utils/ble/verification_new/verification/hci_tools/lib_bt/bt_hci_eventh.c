#include <stdio.h>
#include "types.h"
#include "lib/ssv_lib.h"
#include "bt_hci_lib.h"

extern bt_hci_cmd* bt_hci_cmd_tbl[];
extern const int bt_hci_cmd_tbl_size;

int bt_hci_eventh_command_complete (const int parameter_tbl_size, bt_hci_parameter_entry* const parameter_tbl) {

    int         idx_cmd_tbl;
    bt_hci_cmd* cmd;

    PRINTF_FX("## Num_HCI_Command_Packets: %2x\n", *(u8*) parameter_tbl[0].value);
    PRINTF_FX("## Command_Opcode: %04x\n",         *(u16*)parameter_tbl[1].value);
    PRINTF_FX("## Return_Parameters:\n");
    print_charray(parameter_tbl[2].length, (u8*)parameter_tbl[2].value);

    //PRINTF_FX("## Call Command-Return Handle:\n");

    for(idx_cmd_tbl=0; idx_cmd_tbl < bt_hci_cmd_tbl_size; idx_cmd_tbl++) {

        cmd = bt_hci_cmd_tbl[idx_cmd_tbl];

        if(cmd->opcode == (u32)*(u16*)parameter_tbl[1].value) {

            PRINTF_FX("### going to call %s return-handle\n",  cmd->name);
            //PRINTF_FX("### parsing parameters for return-handle\n");
            bt_hci_parameter_parsing(cmd->ret_parameter_tbl_size, cmd->ret_parameter_tbl,
                                     parameter_tbl[2].length, parameter_tbl[2].value);

            //PRINTF_FX("### call return-handle\n");
            cmd->ret_handle(cmd->parameter_tbl_size,     cmd->parameter_tbl,
                            cmd->ret_parameter_tbl_size, cmd->ret_parameter_tbl);

            break;
        }
    }

    /*
    if(idx_cmd_tbl < bt_hci_cmd_tbl_size) {
    }
    else {
    }
    */

    return FX_SUCCESS;
}

int bt_hci_le_eventh_advertising_report (const int parameter_tbl_size, bt_hci_parameter_entry* const parameter_tbl) {

    int         idx_cmd_tbl;
    bt_hci_cmd* cmd;

    PRINTF_FX("## Subevent_Code: %2x\n",			*(u8*) parameter_tbl[0].value);
    PRINTF_FX("## Num_Reports:    %2x\n",				*(u8*)parameter_tbl[1].value);
//PRINTF_FX("## Num_Reports:    %2x\n",					*(u8*)parameter_tbl[1].value);

//    PRINTF_FX("## Return_Parameters:\n");
//    print_charray(parameter_tbl[2].length, (u8*)parameter_tbl[2].value);

    //PRINTF_FX("## Call Command-Return Handle:\n");

#if 0
    for(idx_cmd_tbl=0; idx_cmd_tbl < bt_hci_cmd_tbl_size; idx_cmd_tbl++) {

        cmd = bt_hci_cmd_tbl[idx_cmd_tbl];

        if(cmd->opcode == (u32)*(u16*)parameter_tbl[1].value) {

            PRINTF_FX("### going to call %s return-handle\n",  cmd->name);
            PRINTF_FX("### parsing parameters for return-handle\n");
            bt_hci_parameter_parsing(cmd->ret_parameter_tbl_size, cmd->ret_parameter_tbl,
                                     parameter_tbl[2].length, parameter_tbl[2].value);

            PRINTF_FX("### call return-handle\n");
            cmd->ret_handle(cmd->parameter_tbl_size,     cmd->parameter_tbl,
                            cmd->ret_parameter_tbl_size, cmd->ret_parameter_tbl);

            break;
        }
    }
#endif
    /*
    if(idx_cmd_tbl < bt_hci_cmd_tbl_size) {
    }
    else {
    }
    */

    return FX_SUCCESS;
}

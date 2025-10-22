#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "task.h"
#include "types.h"

#include "../bench/cli/cli.h"
#include "../hci_tools/lib/ssv_lib.h"
#include "../hci_tools/lib_bt/bt_hci_cmd_declare.h"

#include "ctrl_bench.h"
#include "cli_cmd_bcmd.h"

void BCmd_Event_Log_Query(s32 argc, s8 *argv[]) {

    int dut;
    s32 dut_fd=0;

    // parameter
    u8  bcmd_buf[_BT_HCI_W_BCMD_HEADER_ +_BT_HCI_W_PARAMETER_BCMD_EVENT_LOG_QUERY_] = {
        _BT_HCI_INDICATOR_BCMD_,
        BT_HCI_BCMD_OPCODE_EVENT_LOG_QUERY,
        _BT_HCI_W_PARAMETER_BCMD_EVENT_LOG_QUERY_,
    };

    bcmd_event_log_query_param_st*  param = (bcmd_event_log_query_param_st*)(bcmd_buf +_BT_HCI_IDX_BCMD_PARAM_);
    bevent_event_log_query_end_st*  ret_param;

    param->event_code       = 0x33;
    param->le_subevent_code = 0x22;
    param->latest           = false;

    // for socket
    u16 rcv_socket_len;
    u8  rcv_socket_buf[1024];

    if(argc != 2) {

        printf("{dut-x}\n");
        return;
    }
    dut     = atoi(argv[1]);
    dut_fd  = dut_socket_init(dut);

    // # trigger cmd
    socket_msg_send(dut_fd, sizeof(bcmd_buf), bcmd_buf);

    // # wait cmd_end
    while(1) {
        socket_msg_get(dut_fd, &rcv_socket_len, rcv_socket_buf);

        if(RCV_BEVENT_EVENT_LOG_QUERY_END(rcv_socket_buf)) {
            ret_param = (bevent_event_log_query_end_st*)(rcv_socket_buf +_BT_HCI_IDX_BEVENT_PARAM_);
            printf("# event_log_query end, pending events = %d\n", ret_param->pending_events);
            return;
        }
        else {
            print_charray(rcv_socket_len, rcv_socket_buf);
        }
    }
}

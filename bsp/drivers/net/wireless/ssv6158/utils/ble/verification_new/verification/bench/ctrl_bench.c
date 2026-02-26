#include <stdio.h>
#include <string.h>
#include "task.h"
#include "types.h"
#include "../log/log.h"
#include "ctrl_bench.h"

#include "./hci_tools/lib_bt/bt_hci_lib.h"
#include "./hci_tools/lib_bt/bt_hci_cmd_declare.h"
#include "./hci_tools/lib/ssv_lib.h"

#include "sys/time.h"

extern sock_fd_path_t dut_sock_fd_path[];
//extern void cli_task(void);
#define MAX_DUT_SOCKET 10
int DUT_SOCKETFD_MAP[MAX_DUT_SOCKET];

char* dut_socket_fd_path[MAX_DUTS+1] = {
    "/tmp/ctrl_bench_dut0",//unused
    "/tmp/ctrl_bench_dut1",
    "/tmp/ctrl_bench_dut2",
    "/tmp/ctrl_bench_dut3",
    "/tmp/ctrl_bench_dut4",
};

int dut_socket_fd[MAX_DUTS+1] = {
    -1,//unused
    -1,
    -1,
    -1,
    -1,
};

//static event_log_ring_st sg_event_log_ring = {0};

void event_query_timer_stop(struct itimerval *timer)
{
	timer->it_value.tv_sec = 0;
	timer->it_value.tv_usec = 0;
	timer->it_interval.tv_sec = 0;
	timer->it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, timer, NULL);
}

void event_query_timer_config(u16 init_sec ,u32 init_usec ,u16 interval_sec,u16 interval_usec,struct itimerval *timer)
{
	timer->it_value.tv_sec = init_sec;
	timer->it_value.tv_usec = init_usec;
	timer->it_interval.tv_sec = interval_sec;
	timer->it_interval.tv_usec = interval_usec;
}

int event_query_timer_start(struct itimerval value , __sighandler_t handler )
{
//	struct itimerval ovalue;
	signal(SIGALRM, handler);
//	setitimer(ITIMER_REAL, &value, &ovalue);
	if (setitimer(ITIMER_REAL, &value, NULL) < 0 ){
        printf("set query timer error \n");
        while(1){};
	};
    return 0;
}

int bt_hci_write_data2socket(int fd, bt_hci_acl_data* p_data) {

    u32 buf_length;

    u8  buf[_BT_HCI_W_ACL_DATA_HEADER_ + _BT_HCI_W_ACL_DATA_MAX_] = {0};

    // # prepare indicator
    buf[_BT_HCI_IDX_INDICATOR_] = _BT_HCI_INDICATOR_ACL_DATA_;
    // # prepare connection handle and PB/BC ,input conn handles is MSB->LSB

#if 1 //(BC | PB | CONN_HANDLE(1) ; CONN_HANDLE(2))
    buf[_BT_HCI_IDX_ACL_DATA_CONN_HANDLE_ + 0] =
	(unsigned char)(p_data->conn_handle & 0x0ff);
    buf[_BT_HCI_IDX_ACL_DATA_CONN_HANDLE_ + 1] =
	(unsigned char)((p_data->conn_handle  & 0xf00 ) >> 8 | ((p_data->pb)<<4) | ((p_data->bc)<<6)) ;
#else // below can't working
    buf[_BT_HCI_IDX_ACL_DATA_CONN_HANDLE_ + 0] =
		(unsigned char)(((p_data->conn_handle & 0x00f) <<4) |((p_data->pb)<<2) | p_data->bc);
    buf[_BT_HCI_IDX_ACL_DATA_CONN_HANDLE_ + 1] =
		(unsigned char)((p_data->conn_handle  & 0xff0 ) >> 4) ;
#endif

    // # prepare length
    buf[_BT_HCI_IDX_ACL_DATA_LENGTH_ + 0] = (p_data->length &0x00ff) >> 0;
    buf[_BT_HCI_IDX_ACL_DATA_LENGTH_ + 1] = (p_data->length &0xff00) >> 4;

    buf_length = p_data->length + _BT_HCI_W_ACL_DATA_HEADER_;

    // # copy data
    memcpy (buf +_BT_HCI_IDX_ACL_DATA_PAYLOAD_ , p_data->data ,  p_data->length) ;

    printf(COLOR);
    printf("# Write ACL Data [len=%d](dut:%d)\n" , buf_length , fd);
    printf(NONE);
    print_charray(buf_length , buf);

    return socket_msg_send(fd ,buf_length  ,buf);
}

int bt_hci_write_cmd2socket(int fd, bt_hci_cmd* p_cmd) {

    u8  parameter_length;
    u32 buf_length;

    u8  buf[_BT_HCI_W_CMD_HEADER_ + _BT_HCI_W_PARAMTER_MAX_] = {0};

    // # prepare indicator
    buf[_BT_HCI_IDX_INDICATOR_] = _BT_HCI_INDICATOR_COMMAND_;

    // # prepare opcode
    buf[_BT_HCI_IDX_CMD_OPCODE_ + 0] = (unsigned char)((p_cmd->opcode & 0x00ff) >> 0);
    buf[_BT_HCI_IDX_CMD_OPCODE_ + 1] = (unsigned char)((p_cmd->opcode & 0xff00) >> 8);

    // # prepare paramter
    parameter_length = bt_hci_parameter_building(p_cmd->parameter_tbl_size, p_cmd->parameter_tbl, \
                       _BT_HCI_W_PARAMTER_MAX_, (buf + _BT_HCI_IDX_CMD_PARAMETER_));

    // # prepare length
    buf[_BT_HCI_IDX_CMD_LENGTH_] = parameter_length;

    // # write command
    buf_length = _BT_HCI_W_CMD_HEADER_ + parameter_length;

    printf(COLOR);
    printf("# Write Command [%s](%d)\n", p_cmd->name , fd);
    printf(NONE);

    print_charray(buf_length, buf);

    return socket_msg_send(fd ,buf_length,buf);
}

/**
 * bcmd handler: event log query
 *
 */
void bcmd_hdl_event_log_query(s32 fd, u8 cmd_len, u8* cmd) {

    u8  bevent_buf[_BT_HCI_W_BEVENT_HEADER_ +_BT_HCI_W_PARAM_BEVENT_EVENT_LOG_QUERY_END_] = {
        _BT_HCI_INDICATOR_BEVENT_,
         BT_HCI_BEVENT_OPCODE_EVENT_LOG_QUERY_END,
        _BT_HCI_W_PARAM_BEVENT_EVENT_LOG_QUERY_END_,
    };

    bevent_event_log_query_end_st*  bevent_param = (bevent_event_log_query_end_st*)(bevent_buf +_BT_HCI_IDX_BEVENT_PARAM_);

    PRINTF_FX("called ...\n");

    // # action
    bevent_param->pending_events = 99;

    // # end
    socket_msg_send(fd, sizeof(bevent_buf), bevent_buf);

    return;
}

/**
 * bcmd handler: event log dump
 *
 */
void bcmd_hdl_event_log_dump(s32 fd, u8 cmd_len, u8* cmd) {
    PRINTF_FX("called ...\n");
    return;
}

/**
 * bcmd handler: event log reset
 *
 */
void bcmd_hdl_event_log_reset(s32 fd, u8 cmd_len, u8* cmd) {
    PRINTF_FX("called ...\n");
    return;
}

typedef void (*BCMD_HDL)(s32 fd, u8 cmd_len, u8* cmd);

typedef struct bcmd_decription {
    u8          opcode;
    BCMD_HDL    hdl;
} bcmd_decription_st;

bcmd_decription_st sg_bcmd_decription_tbl[] = {
    {BT_HCI_BCMD_OPCODE_EVENT_LOG_QUERY,    bcmd_hdl_event_log_query},
    {BT_HCI_BCMD_OPCODE_EVENT_LOG_DUMP,     bcmd_hdl_event_log_dump},
    {BT_HCI_BCMD_OPCODE_EVENT_LOG_RESET,    bcmd_hdl_event_log_reset},
};


/**
 * bcmd handler
 *
 */
int bcmd_hdl(s32 fd, u8 cmd_len, u8* cmd_buf) {

    int loop_i;

    if(cmd_buf[_BT_HCI_IDX_INDICATOR_] == _BT_HCI_INDICATOR_BCMD_) {
        for(loop_i=0; loop_i < sizeof(sg_bcmd_decription_tbl)/sizeof(bcmd_decription_st); loop_i++) {
            if(cmd_buf[_BT_HCI_W_BCMD_OPCODE_] == sg_bcmd_decription_tbl[loop_i].opcode) {
                sg_bcmd_decription_tbl[loop_i].hdl(fd, cmd_len, cmd_buf);
                return true;
            }
        }
    }
    return false;
}

int dut_socket_get(int dut)
{
    return dut_socket_fd[dut];
}

// TODO: this function return is ambiguous. fd or error?
int dut_socket_init(int dut)
{
	s32 dut_fd=0;

	if(-1 == dut_socket_fd[dut]){
		printf("\nDUT socket init with dut %d | " , dut);

		if(client_socket_init(dut_socket_fd_path[dut], &dut_fd) != SOCKET_SUCCESS) {
            printf(ERR);
			printf("socket(%d) init failed!\n\n" ,dut_fd);
            printf(NONE);
			return ERROR;
		}
		printf("socket(%d) init success!\n",dut_fd);
		dut_socket_fd[dut]=dut_fd;
    }

    return dut_socket_fd[dut];
}

void dut_socket_map_init(int dev_num)
{
    int cnt = 0;
    for( ; cnt < MAX_DUT_SOCKET ; cnt++){
        DUT_SOCKETFD_MAP[cnt] = -1;
    }
}

int bench_init(int dev_num)
{
	dut_socket_map_init(dev_num);

	return 0;
}

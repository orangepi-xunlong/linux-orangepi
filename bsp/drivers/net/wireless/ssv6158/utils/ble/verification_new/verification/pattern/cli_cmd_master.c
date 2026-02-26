#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "task.h"
#include "types.h"

#include "../log/log.h"

#include "../bench/cli/cli.h"
#include "../hci_tools/lib/ssv_lib.h"
#include "../hci_tools/lib_bt/bt_hci_cmd_declare.h"
#include "../hci_tools/lib_bt/bt_hci_lib.h"
#include "../hci_tools/lib_bluez/bluetooth.h"

#include "ctrl_bench.h"
#include "cli_cmd_master.h"
#include "pattern2dut.h"

static int query_le_event(int dut_fd, u8 sub_event, u8 *buf)
{
	struct itimerval timer;
	int		ret;

	event_query_timer_config(5 , 0 , 0 , 0, &timer);
	event_query_timer_start(timer ,read_le_event_enable );

	while (1) //waiting adv report event , timer or counter
	{
		if ( 1 == read_le_event_get() )
		{
            read_le_event_disable();

			ret = QUERY_LE_Event(dut_fd, buf, sub_event, 0);
			event_query_timer_stop(&timer);
			break;
		}
	}
	return ret;
}

void Cmd_lecc(s32 argc, s8 *argv[]) {

    int dut;
    s32 dut_fd=0;

    // parameter
    u8  le_scan_intv        [2] = {0x80, 0x0c}; // 0x0c80: 2s
    u8  le_scan_window      [2] = {0x80, 0x0c};
    u8  init_filter_policy  [1] = {0x00};
    u8  peer_addr_type      [1] = {0x00};
    u8  peer_addr           [6] = // {0x10, 0x32, 0x54, 0x76, 0x98, 0xba};
		                          {0x13,0x71,0xda,0x7d,0x1a,0x00};
    u8  own_addr_type       [1] = {0x00};
    u8  conn_intv_min       [2] = {0x28, 0x00};
    u8  conn_intv_max       [2] = {0x28, 0x00};
    u8  conn_latency        [2] = {0x01, 0x00};
    u8  supervision_tout    [2] = {0x32, 0x00};
    u8  min_ce_length       [2] = {0x28, 0x00};
    u8  max_ce_length       [2] = {0x28, 0x00};

    // for socket
    u16 len;
    u8  buf[1024];
	char	msg[256];
	bdaddr_t _ba;
	u8	conn_handle[2];
	u8	evt_status;
	u16	_u16;
	int ret;

    if (argc == 2)	goto lecc;
	if (argc == 3)	goto def_ba;

//usage:
	printf("lecc <dut> : use def ba(%02X:%02X:%02X:%02X:%02X:%02X)\n",
			peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
	printf("lecc <dut> <ba>\n");
	return;

def_ba:
	if (str2ba(argv[2], &_ba) != 0)	{
		printf(COLOR_ERR "invalid param!\n" COLOR_NONE);
		return;
	}
	memcpy(peer_addr, &_ba, 6);
	// go thru lecc
lecc:
	dut     = atoi(argv[1]);
    dut_fd  = dut_socket_init(dut);

	printf(COLOR_INFO);
	printf("create LE conn_req\n");
	printf("dut = %d, dut_fd = %d\n", dut, dut_fd);
	printf("%-20s: %02X:%02X:%02X:%02X:%02X:%02X\n", "peer_addr",
			peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
	printf(COLOR_NONE);
#if 1
	printf("%-20s: 0x%04X\n", "le_scan_intv",		((u16)le_scan_intv[0] << 8 | le_scan_intv[1]));
	printf("%-20s: 0x%04X\n", "le_scan_window",		((u16)le_scan_window[0] << 8 | le_scan_window[1]));
	printf("%-20s: 0x%04X\n", "conn_intv_min",		((u16)conn_intv_min[0] << 8 | conn_intv_min[1]));
	printf("%-20s: 0x%04X\n", "conn_intv_max",		((u16)conn_intv_max[0] << 8 | conn_intv_max[1]));
	printf("%-20s: 0x%04X\n", "conn_latency",		((u16)conn_latency[0] << 8 | conn_latency[1]));
	printf("%-20s: 0x%04X\n", "supervision_tout",	((u16)supervision_tout[0] << 8 | supervision_tout[1]));
	printf("%-20s: 0x%04X\n", "min_ce_length",		((u16)min_ce_length[0] << 8 | min_ce_length[1]));
	printf("%-20s: 0x%04X\n", "max_ce_length",		((u16)max_ce_length[0] << 8 | max_ce_length[1]));
#endif

    bt_hci_cmd_le_create_connection.parameter_tbl[0].value  = le_scan_intv;
    bt_hci_cmd_le_create_connection.parameter_tbl[1].value  = le_scan_window;
    bt_hci_cmd_le_create_connection.parameter_tbl[2].value  = init_filter_policy;
    bt_hci_cmd_le_create_connection.parameter_tbl[3].value  = peer_addr_type;
    bt_hci_cmd_le_create_connection.parameter_tbl[4].value  = peer_addr;
    bt_hci_cmd_le_create_connection.parameter_tbl[5].value  = own_addr_type;
    bt_hci_cmd_le_create_connection.parameter_tbl[6].value  = conn_intv_min;
    bt_hci_cmd_le_create_connection.parameter_tbl[7].value  = conn_intv_max;
    bt_hci_cmd_le_create_connection.parameter_tbl[8].value  = conn_latency;
    bt_hci_cmd_le_create_connection.parameter_tbl[9].value  = supervision_tout;
    bt_hci_cmd_le_create_connection.parameter_tbl[10].value = min_ce_length;
    bt_hci_cmd_le_create_connection.parameter_tbl[11].value = max_ce_length;

    bt_hci_write_cmd2socket (dut_fd, &bt_hci_cmd_le_create_connection);
    socket_msg_get          (dut_fd, &len, buf);

    printf("Get Event(%d):\n", dut_fd);
    print_charray(len, buf);
	//hci_event_socket_dump(buf, len);

	ret = query_le_event(dut_fd, _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_COMPLETE_, buf);
	if (SUCCESS != ret) {
		sprintf(msg, "query_le_event() = %d != SUCCESS!\n", ret);
		goto FUN_FAIL;
	}
	hci_event_socket_dump(buf, len);

	// parse recv buf[]
	evt_status		= buf[_BT_HCI_EVT_CON_COMP_STATUS];
	conn_handle[0]	= buf[_BT_HCI_EVT_COMP_HANDLE + 0];
	conn_handle[1]	= buf[_BT_HCI_EVT_COMP_HANDLE + 1];
	if( (conn_handle[0] == 0) &&  (conn_handle[1] == 0)) {
		sprintf(msg, "conn_handle = 0\n");
		goto FUN_FAIL;
	}
	if (SUCCESS != evt_status) {
		sprintf(msg, "evt_status(%d) != SUCCESS\n", evt_status);
		goto FUN_FAIL;
	}
	_u16 = conn_handle[1] << 8 | conn_handle[0];
	printf(COLOR_INFO "conn_handle = %d (0x%04x)\n" COLOR_NONE, _u16, _u16);
	goto FUN_SUCCESS;

FUN_SUCCESS:
	printf("success!\n");
	return;
FUN_FAIL:
	printf(COLOR_ERR "%s\n" COLOR_NONE, msg);
	printf("fail!\n");
	return;
}

void Cmd_ledc(s32 argc, s8 *argv[]) {
    int dut;
    s32 dut_fd=0;

    u8  conn_hdl[2] = {0};
    u8  reason  [1] = {0x16};
    // for socket
    u16 len;
    u8  buf[1024];
	u16	_u16;

    u8  prm_len;
    u32 buf_len;
	u16 opcode;
	int ret;

    if (argc == 3)	goto ledc;
	if (argc == 4)	goto with_reason;

//usage:
	printf("ledc <dut> <conn_hdl> [reason](def:%d)\n", reason[0]);
	return;

with_reason:
	reason[0] = atoi(argv[3]);

ledc:
    dut     = atoi(argv[1]);
    dut_fd  = dut_socket_init(dut);

	_u16	= atoi(argv[2]);
	conn_hdl[0] = (_u16 & 0xff00) >> 8;
	conn_hdl[1] = (_u16 & 0x00ff);

	printf(COLOR_INFO);
	printf("conn_hdl = %d\n", ((u16)conn_hdl[0] << 8)|conn_hdl[1]);
	printf("reason   = 0x%02x (%s)\n", reason[0], hci_error_code_str(reason[0]));
	printf(COLOR_NONE);

	memset(buf, 0, 1024);
    // # prepare indicator
    buf[0] = _BT_HCI_INDICATOR_COMMAND_;

    // # prepare opcode
	opcode = (u16)((0x0006 & 0x03ff)|(0x0001 << 10));
	buf[1] = (unsigned char)((opcode & 0x00ff) >> 0);
    buf[2] = (unsigned char)((opcode & 0xff00) >> 8);

	// # prepare paramter
	prm_len = 3;
    buf[3] = prm_len;

	buf[4] = conn_hdl[1];
	buf[5] = conn_hdl[0];
	buf[6] = reason[0];
    // # write command
    buf_len = _BT_HCI_W_CMD_HEADER_ + prm_len;

    printf(COLOR);
    printf("# Write Command [%s](%d)\n", "Disconnect", dut_fd);
    printf(NONE);

    print_charray(buf_len, buf);
    socket_msg_send(dut_fd, buf_len, buf);

	// -------------------------------------------------------------------
	memset(buf, 0, 1024);
    // expect: command status
    socket_msg_get(dut_fd, &len, buf);

    printf("Get Event(%d):\n", dut_fd);
    print_charray(len, buf);

	// expect: le event
	ret = query_le_event(dut_fd, _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_COMPLETE_, buf);
	if (SUCCESS != ret) {
		printf(COLOR_ERR "query_le_event() = %d != SUCCESS!\n" COLOR_NONE, ret);
		return;
	}
	hci_event_socket_dump(buf, len);
	return;
}

void Cmd_lecan(s32 argc, s8 *argv[]) {
    int dut;
    s32 dut_fd=0;

    // for socket
    u16 len;
    u8  buf[1024];
	u16	_u16;

    u8  prm_len;
    u32 buf_len;
	u16 opcode;
	int ret;

    if (argc == 2)	goto lecan;

//usage:
	printf("lecan <dut>\n");
	return;

lecan:
    dut     = atoi(argv[1]);
    dut_fd  = dut_socket_init(dut);

	memset(buf, 0, 1024);
    // # prepare indicator
    buf[0] = _BT_HCI_INDICATOR_COMMAND_;

    // # prepare opcode
	opcode = (u16)((0x000E & 0x03ff)|(0x0008 << 10));
	buf[1] = (unsigned char)((opcode & 0x00ff) >> 0);
    buf[2] = (unsigned char)((opcode & 0xff00) >> 8);

	// # prepare paramter
	prm_len = 0;

    // # write command
    buf_len = _BT_HCI_W_CMD_HEADER_ + prm_len;

    printf(COLOR);
    printf("# Write Command [%s](%d)\n", "create connection cancel", dut_fd);
    printf(NONE);

    print_charray(buf_len, buf);
    socket_msg_send(dut_fd, buf_len, buf);

	// -------------------------------------------------------------------
	memset(buf, 0, 1024);
    // expect: command status
    socket_msg_get(dut_fd, &len, buf);

    printf("Get Event(%d):\n", dut_fd);
    print_charray(len, buf);

	// expect: le event
	ret = query_le_event(dut_fd, _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_COMPLETE_, buf);
	if (SUCCESS != ret) {
		printf(COLOR_ERR "query_le_event() = %d != SUCCESS!\n" COLOR_NONE, ret);
		return;
	}
	hci_event_socket_dump(buf, len);
	return;
}
void Cmd_acl(s32 argc, s8 *argv[]) {

	u8		data[37], conn_hdl[2];
	u16		data_len, _u16;
	int		i, dut, dut_fd;
	u8		pb, bc;
	int		verdict = FAIL;

	if (argc == 1)		goto usage;
	if (argc == 4)		goto _len;

	printf(COLOR_ERR "invalid param!\n" COLOR_NONE);
usage:
	printf("acl <dut> <conn_hdl> <len>: send len(1-37) bytes\n");
	return;
_len:
	i = atoi(argv[3]);
	if ((i <= 0) || (i > 37)) {
		printf(COLOR_ERR "invalid param <len> !\n" COLOR_NONE);
		goto usage;
	}
	data_len = i;
	for (i=0; i<data_len; i++)	data[i] = i;
	goto _send;
_send:
	dut = atoi(argv[1]);
	dut_fd = dut_socket_init(dut);

	_u16 = atoi(argv[2]);
	// printf("# conn_hdl = %d\n", _u16);
	conn_hdl[0] = (_u16 & 0x00ff);
	conn_hdl[1] = (_u16 & 0xff00) >> 8;
	pb = bc = 0;
	verdict = COMM_Send_ACL_Data(dut_fd, conn_hdl, pb, bc, data_len, data);
	if (PASS != verdict) {
		//printf(COLOR_ERR "fail!\n" COLOR_NONE);
	}
	return;
}

void Cmd_acl_fragment(s32 argc, s8 *argv[]) {

	u8		data[37], conn_hdl[2];
	u16		data_len, _u16;
	int		i, dut, dut_fd;
	u8		pb, bc;
	int		verdict = FAIL;
	int		loop_idx;
	int	first_bytes = 0;
	int		left_bytes = 0;
	int		more_bytes = 0;

	if (argc == 1)		goto usage;
	if (argc == 5)		goto _len;

	printf(COLOR_ERR "invalid param!\n" COLOR_NONE);
usage:
	printf("acl <dut> <conn_hdl> <len>: send len(1-37) bytes\n");
	return;
_len:
	i = atoi(argv[3]);
	if ((i <= 0) || (i > 37)) {
		printf(COLOR_ERR "invalid param <len> !\n" COLOR_NONE);
		goto usage;
	}
	data_len = i;
	for (i=0; i<data_len; i++)	data[i] = i;
	goto _send;
_send:
	dut = atoi(argv[1]);
	dut_fd = dut_socket_init(dut);

	_u16 = atoi(argv[2]);
	// printf("# conn_hdl = %d\n", _u16);

	first_bytes = atoi(argv[4]);

	conn_hdl[0] = (_u16 & 0x00ff);
	conn_hdl[1] = (_u16 & 0xff00) >> 8;
	pb = bc = 0;
	verdict = COMM_Send_ACL_Data_Without_event(dut_fd, conn_hdl, pb, bc, first_bytes, data);
	if (PASS != verdict) {
		//printf(COLOR_ERR "fail!\n" COLOR_NONE);
	}
	usleep(1000);
	conn_hdl[0] = (_u16 & 0x00ff);
	conn_hdl[1] = (_u16 & 0xff00) >> 8;
	pb = 1;
	bc = 0;
	left_bytes = data_len-first_bytes;
	if (left_bytes > 27) {
		more_bytes = left_bytes - 27;
		left_bytes = 27;
	} else {
        more_bytes = 0;
	}
	verdict = COMM_Send_ACL_Data_Without_event(dut_fd, conn_hdl, pb, bc, left_bytes, data+first_bytes);
	if (PASS != verdict) {
		//printf(COLOR_ERR "fail!\n" COLOR_NONE);
	}
	if (more_bytes) {
		usleep(1000);
		conn_hdl[0] = (_u16 & 0x00ff);
		conn_hdl[1] = (_u16 & 0xff00) >> 8;
		pb = 1;
		bc = 0;
		verdict = COMM_Send_ACL_Data_Without_event(dut_fd, conn_hdl, pb, bc, more_bytes, data+first_bytes+left_bytes);
		if (PASS != verdict) {
			//printf(COLOR_ERR "fail!\n" COLOR_NONE);
		}
	}

	return;
}
void Cmd_hcicmd_reset(s32 argc, s8 *argv[]) {
	int		dut, dut_fd;

	if (argc == 1)	goto usage;
	if (argc == 2)	goto reset;
usage:
	printf("hcicmd-reset <dut>\n");
	return;

reset:
	dut = atoi(argv[1]);
    dut_fd = dut_socket_init(dut);
    if (ERROR == dut_fd) {
		printf(COLOR_ERR "dut(%d): dut_socket_init() fail!!\n" COLOR_NONE, dut);
        return;
    }
	printf("dut_fd = %d\n", dut_fd);

	COMM_Reset(dut_fd);
	return;
}


void Cmd_dut_reset(s32 argc, s8 *argv[]) {
	int		dut, dut_fd;

	if (argc == 1)	goto usage;
	if (argc == 2)	goto reset;
usage:
	printf("hcicmd-reset <dut>\n");
	return;

reset:
	dut = atoi(argv[1]);
    dut_fd = dut_socket_init(dut);
    if (ERROR == dut_fd) {
		printf(COLOR_ERR "dut(%d): dut_socket_init() fail!!\n" COLOR_NONE, dut);
        return;
    }
	printf("dut_fd = %d\n", dut_fd);

	dut_reset_with_mask_en(dut_fd);
	return;
}

void Cmd_Master_Conn(s32 argc, s8 *argv[]) {

    int dut;
    s32 dut_fd=0;

    // parameter
    u8  le_scan_intv        [2] = {0x80, 0x0c}; // 0x0c80: 2s
    u8  le_scan_window      [2] = {0x80, 0x0c};
    u8  init_filter_policy  [1] = {0x00};
    u8  peer_addr_type      [1] = {0x00};
    u8  peer_addr           [6] = // {0x10, 0x32, 0x54, 0x76, 0x98, 0xba};
		                          {0x13,0x71,0xda,0x7d,0x1a,0x00};
    u8  own_addr_type       [1] = {0x00};
    u8  conn_intv_min       [2] = {0x28, 0x00};
    u8  conn_intv_max       [2] = {0x28, 0x00};
    u8  conn_latency        [2] = {0x01, 0x00};
    u8  supervision_tout    [2] = {0x32, 0x00};
    u8  min_ce_length       [2] = {0x28, 0x00};
    u8  max_ce_length       [2] = {0x28, 0x00};

    // for socket
    u16 rcv_socket_len;
    u8  rcv_socket_buf[1024];

    if(argc != 2) {

        printf("{dut-x}\n");
        return;
    }
    dut     = atoi(argv[1]);
    dut_fd  = dut_socket_init(dut);

    printf("%s: dut-%d as master\n", __FUNCTION__, dut);

    bt_hci_cmd_le_create_connection.parameter_tbl[0].value  = le_scan_intv;
    bt_hci_cmd_le_create_connection.parameter_tbl[1].value  = le_scan_window;
    bt_hci_cmd_le_create_connection.parameter_tbl[2].value  = init_filter_policy;
    bt_hci_cmd_le_create_connection.parameter_tbl[3].value  = peer_addr_type;
    bt_hci_cmd_le_create_connection.parameter_tbl[4].value  = peer_addr;
    bt_hci_cmd_le_create_connection.parameter_tbl[5].value  = own_addr_type;
    bt_hci_cmd_le_create_connection.parameter_tbl[6].value  = conn_intv_min;
    bt_hci_cmd_le_create_connection.parameter_tbl[7].value  = conn_intv_max;
    bt_hci_cmd_le_create_connection.parameter_tbl[8].value  = conn_latency;
    bt_hci_cmd_le_create_connection.parameter_tbl[9].value  = supervision_tout;
    bt_hci_cmd_le_create_connection.parameter_tbl[10].value = min_ce_length;
    bt_hci_cmd_le_create_connection.parameter_tbl[11].value = max_ce_length;

    bt_hci_write_cmd2socket (dut_fd, &bt_hci_cmd_le_create_connection);
    socket_msg_get          (dut_fd, &rcv_socket_len, rcv_socket_buf);

    printf("Get Event(%d):\n", dut_fd);
    print_charray(rcv_socket_len, rcv_socket_buf);
}

/**
 * # disconnect
 *  ## specify conn_hdl and reason
 *
 */
void Cmd_Master_Disconn(s32 argc, s8 *argv[]) {

    int dut;
    s32 dut_fd=0;

    u8  conn_hdl[2] = {0x01, 0x04};
    u8  reason  [1] = {0x16};       // 0x16:

    // for socket
    u16 rcv_socket_len;
    u8  rcv_socket_buf[1024];

    if(argc < 2) {
        printf("{dut-x}\n");
        return;
    }
    dut     = atoi(argv[1]);
    dut_fd  = dut_socket_init(dut);

    if(argc >= 3) {
        u32_to_u8(ssv_atoi(argv[2]), conn_hdl, 2);
    }
    PRINTF_FX("conn_hdl: 0x%02x%02x\n", conn_hdl[1], conn_hdl[0]);

    if(argc >= 4) {
        u32_to_u8(ssv_atoi(argv[3]), reason, 1);
    }
    PRINTF_FX("reason: 0x%02x\n", reason[0]);

    bt_hci_cmd_disconnect.parameter_tbl[0].value = conn_hdl;
    bt_hci_cmd_disconnect.parameter_tbl[1].value = reason;

    bt_hci_write_cmd2socket(dut_fd, &bt_hci_cmd_disconnect);

    // expect: command status
    socket_msg_get(dut_fd, &rcv_socket_len, rcv_socket_buf);

    printf("Get Event(%d):\n", dut_fd);
    print_charray(rcv_socket_len, rcv_socket_buf);
}

/**
 * # update parameter
 *  ## specify conn_hdl/intv
 *      ### latency would be 0, if not specified
 *      ### supervision_tout would be 100ms, if not specified
 *
 */
void Cmd_Master_Conn_Update(s32 argc, s8 *argv[]) {

    int dut;
    s32 dut_fd=0;

    u8  conn_hdl            [2] = {0x01, 0x04};
    u8  conn_intv           [2] = {0x28, 0x00};
    u8  conn_latency        [2] = {0x00, 0x00};
    u8  supervision_tout    [2] = {0x64, 0x00};
    u8  ce_length           [2] = {0x28, 0x00};

    // for socket
    u16 rcv_socket_len;
    u8  rcv_socket_buf[1024];

    if(argc < 2) {
        printf("argv[1]: {dut-x}\n");
        return;
    }
    dut     = atoi(argv[1]);
    dut_fd  = dut_socket_init(dut);

    if(argc >= 3) {
        u32_to_u8(ssv_atoi(argv[2]), conn_hdl, 2);
    }
    else {
        printf("argv[2]: {conn_hdl}\n");
        return;
    }
    PRINTF_FX("conn_hdl:  0x%02x%02x\n", conn_hdl[1], conn_hdl[0]);

    if(argc >= 4) {
        u32_to_u8(ssv_atoi(argv[3]), conn_intv, 2);
        u32_to_u8(ssv_atoi(argv[3]), ce_length, 2);
    }
    else {
        printf("argv[3]: {conn_intv}\n");
        return;
    }
    PRINTF_FX("conn_intv: 0x%02x%02x\n", conn_intv[1], conn_intv[0]);

    if(argc >= 5) {
        u32_to_u8(ssv_atoi(argv[4]), conn_latency, 2);
    }
    PRINTF_FX("conn_latency: 0x%02x%02x\n", conn_latency[1], conn_latency[0]);

    if(argc >= 6) {
        u32_to_u8(ssv_atoi(argv[5]), supervision_tout, 2);
    }
    PRINTF_FX("supervision_tout: 0x%02x%02x\n", supervision_tout[1], supervision_tout[0]);

    bt_hci_cmd_le_connection_update.parameter_tbl[0].value  = conn_hdl;
    bt_hci_cmd_le_connection_update.parameter_tbl[1].value  = conn_intv;
    bt_hci_cmd_le_connection_update.parameter_tbl[2].value  = conn_intv;
    bt_hci_cmd_le_connection_update.parameter_tbl[3].value  = conn_latency;
    bt_hci_cmd_le_connection_update.parameter_tbl[4].value  = supervision_tout;
    bt_hci_cmd_le_connection_update.parameter_tbl[5].value  = conn_intv;
    bt_hci_cmd_le_connection_update.parameter_tbl[6].value  = conn_intv;

    bt_hci_write_cmd2socket (dut_fd, &bt_hci_cmd_le_connection_update);
    socket_msg_get          (dut_fd, &rcv_socket_len, rcv_socket_buf);

    printf("Get Event(%d):\n", dut_fd);
    print_charray(rcv_socket_len, rcv_socket_buf);
}

void Cmd_Master_Feature_Exchange(s32 argc, s8 *argv[]) {

    int dut;
    s32 dut_fd=0;

    // parameter
    u8  conn_hdl[2] = {0x01, 0x04};

    // for socket
    u16 rcv_socket_len;
    u8  rcv_socket_buf[1024];

    if(argc >= 2) {
        dut     = atoi(argv[1]);
        dut_fd  = dut_socket_init(dut);
    }
    else {
        printf("argv[1]: {dut-x}\n");
        return;
    }

    if(argc >= 3) {
        u32_to_u8(ssv_atoi(argv[2]), conn_hdl, 2);
    }
    PRINTF_FX("conn_hdl:  0x%02x%02x\n", conn_hdl[1], conn_hdl[0]);

    bt_hci_cmd_le_read_remote_used_features.parameter_tbl[0].value = conn_hdl;

    bt_hci_write_cmd2socket (dut_fd, &bt_hci_cmd_le_read_remote_used_features);
    socket_msg_get          (dut_fd, &rcv_socket_len, rcv_socket_buf);

    printf("Get Event(%d):\n", dut_fd);
    print_charray(rcv_socket_len, rcv_socket_buf);
}

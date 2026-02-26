#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>
#include <poll.h>

#include "types.h"
#include "task.h"

#include "../hci_tools/ssv_hci_if.h"
#include "../hci_tools/host_cli.h"
#include "../hci_tools/lib_bt/bt_hci_lib.h"
#include "../hci_tools/lib_bt/bt_hci_cmd_declare.h"
#include "../hci_tools/lib/ssv_lib.h"
#include "../log/log.h"

#include "../hci_tools/lib_bluez/bluetooth.h"
#include "../hci_tools/lib_bluez/hci.h"
#include "../hci_tools/lib_bluez/hci_lib.h"

#define BUFF_SIZE_MAX 255

typedef struct Async_buff_s{
	u8 buf[BUFF_SIZE_MAX];
}Async_buff;

#define ACL_BUFF_NUM 254
#define ASYNC_BUFF_NUM_INVALID 0

static Async_buff ASYNC_BUFF[BUFF_SIZE_MAX];

static u32 g_data_buffer_overflow_num  = 0;
static u32 g_num_pkts               = 0;
static u8  g_pld_check_enable       = 0;
static u32 g_adv_rpt_num            = 0;
static u32 g_scn_rsp_num            = 0;
static u32 g_seq_num                = 0;
static u32 g_rx_acl_packets_cnt     = 0;

const char * log_file = NULL ;

// SamYu
//
// the function name 'single' means taking the following 2 items as a whole action.
//	1: send "only one" ACL data packet
//	2: wait for HCI_Number_Of_Completed_Packets event within 'tout'.
//
// int hci_acl_cmd_proc_v2_4csr(int dd, int conn_handle, uint8_t pb, uint8_t bc, uint16_t data_len, void *data, int tout)
int hci_acl_cmd_proc_v2_4csr(s32 bench_fd, int dd, u16 len, u8 *buf)
{
	u8		type = HCI_ACLDATA_PKT;
	void	*data;
	u16	data_len;
	hci_acl_hdr acl_hdr;
	struct iovec iv[3];
	int ivn;

	// ----------------- write HCI ACL ------------------
	data		= buf + 5;
	data_len	= buf[4] << 8 | buf[3];

	acl_hdr.handle	= buf[2] << 8 | buf[1];
	acl_hdr.dlen	= data_len;

	iv[0].iov_base = &type;
	iv[0].iov_len  = 1;
	iv[1].iov_base = &acl_hdr;
	iv[1].iov_len  = HCI_ACL_HDR_SIZE;
	ivn = 2;

	if (data_len > 0) {
		iv[2].iov_base = data;
		iv[2].iov_len  = data_len;
		ivn = 3;
	}

	while (writev(dd, iv, ivn) < 0) {
		if (errno == EAGAIN || errno == EINTR)
			continue;

		return -1;
	}
	return 0;

}


void hci_acl_cmd_proc_v2(int uart_fd , u16 len , u8 *buf){

    u8 acl_cmd_len_idx    = _BT_HCI_IDX_ACL_DATA_LENGTH_;
    u8 acl_cmd_data_idx   = _BT_HCI_IDX_ACL_DATA_PAYLOAD_;
    u8 acl_cmd_header_len = _BT_HCI_W_ACL_DATA_HEADER_;

    u16 bench_msg_len;

    bench_msg_len = len ;
    if(bench_msg_len > 0){

        bt_hci_write(uart_fd
                    , buf[acl_cmd_len_idx]   + acl_cmd_header_len
                    , buf + acl_cmd_data_idx - acl_cmd_header_len);
#if 0
        printf("PROC : check len %d: len idx %d : data idx %d\n"
            , bench_msg_len
            , acl_cmd_len_idx
            , acl_cmd_data_idx
            );
#endif

#if 0 //def READ_EVT_DEBUG
        print_charray(       buf[acl_cmd_len_idx]  + acl_cmd_header_len
                           , buf + acl_cmd_data_idx- acl_cmd_header_len);
#endif
    }
}



bool hci_buff_reset_check(u8 *rxbuf){

    if(_BT_HCI_INDICATOR_COMMAND_ == rxbuf[_BT_HCI_IDX_INDICATOR_]){

        switch(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OGF] >> 2) {

            case _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_ :
                if(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OCF] == _BT_HCI_CMD_OCF_DUT_CLEAN_BUFFER_){
                    return FX_SUCCESS;
                }
                break;

            default:
                break;
        }
    }

    return FX_FAIL;

}


bool hci_dut_payload_check(u8 *rxbuf , u8 *enable ){

    if(_BT_HCI_INDICATOR_COMMAND_ == rxbuf[_BT_HCI_IDX_INDICATOR_]){

        switch(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OGF] >> 2) {

            case _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_ :
                if(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OCF] == _BT_HCI_CMD_OCF_DUT_CHECK_PAYLOAD_){
                    *enable = rxbuf[_BT_HCI_IDX_CMD_PARAMETER_];

                    if (*enable == 0){
                        g_seq_num = 0;
                    }

                    return FX_SUCCESS;
                }
                break;

            default:
                break;
        }
    }

    return FX_FAIL;

}


bool hci_dut_adv_cnt_check(u8 *rxbuf){

    if(_BT_HCI_INDICATOR_COMMAND_ == rxbuf[_BT_HCI_IDX_INDICATOR_]){

        switch(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OGF] >> 2) {

            case _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_ :
                if(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OCF] == _BT_HCI_CMD_OCF_DUT_QUERY_ADV_CNT_){
                    return FX_SUCCESS;
                }
                break;

            default:
                break;
        }
    }

    return FX_FAIL;

}

bool hci_dut_num_of_packets_cnt_check(u8 *rxbuf){

    if(_BT_HCI_INDICATOR_COMMAND_ == rxbuf[_BT_HCI_IDX_INDICATOR_]){

        switch(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OGF] >> 2) {

            case _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_ :
                if(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OCF] == _BT_HCI_CMD_OCF_DUT_QUERY_NUM_OF_PACKETS_CNT_){
                    return FX_SUCCESS;
                }
                break;

            default:
                break;
        }
    }

    return FX_FAIL;

}

bool hci_dut_data_buffer_overflow_cnt_check(u8 *rxbuf){

    if(_BT_HCI_INDICATOR_COMMAND_ == rxbuf[_BT_HCI_IDX_INDICATOR_]){

        switch(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OGF] >> 2) {

            case _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_ :
                if(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OCF] == _BT_HCI_CMD_OCF_DUT_QUERY_DATA_BUFFER_OVERFLOW_CNT_){
                    return FX_SUCCESS;
                }
                break;

            default:
                break;
        }
    }

    return FX_FAIL;

}

bool hci_dut_rx_acl_packets_cnt_check(u8 *rxbuf){

    if(_BT_HCI_INDICATOR_COMMAND_ == rxbuf[_BT_HCI_IDX_INDICATOR_]){

        switch(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OGF] >> 2) {

            case _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_ :
                if(rxbuf[_BT_HCI_IDX_CMD_OPCODE_OCF] == _BT_HCI_CMD_OCF_DUT_QUERY_RX_ACL_PACKETS_CNT_){
                    return FX_SUCCESS;
                }
                break;

            default:
                break;
        }
    }

    return FX_FAIL;

}

#define ADV_IND_BYTE_WIDTH 2
#define SCN_RSP_BYTE_WIDTH 2

s8 hci_cmd_dut_func(u32 bench_fd, u8 *rxbuf){

	u8 cnt = 0;

	if(hci_buff_reset_check(rxbuf) == FX_SUCCESS){
		printf("\n## Clean async buffer ##\n\n");
		for ( ; cnt < BUFF_SIZE_MAX ; cnt ++){

			memset(ASYNC_BUFF[cnt].buf, 0 , BUFF_SIZE_MAX);
		}

        g_data_buffer_overflow_num = 0;
        g_num_pkts               = 0;
        g_pld_check_enable       = 0;
        g_adv_rpt_num            = 0;
        g_scn_rsp_num            = 0;
        g_seq_num                = 0;
        g_rx_acl_packets_cnt     = 0;

        return FX_SUCCESS;
	}

	if(hci_dut_payload_check(rxbuf , &g_pld_check_enable) == FX_SUCCESS){
		printf("\n## Payload check %s ##\n\n" , g_pld_check_enable ? "enable" : "disable");
        return FX_SUCCESS;
	}

	if(hci_dut_adv_cnt_check(rxbuf) == FX_SUCCESS){

		printf("\n## adv cnt query [%d] [%d] ##\n\n" , g_adv_rpt_num , g_scn_rsp_num);

        u8 adv_rpt[ADV_IND_BYTE_WIDTH + SCN_RSP_BYTE_WIDTH];

        adv_rpt[0] =  g_adv_rpt_num & 0x00FF;
        adv_rpt[1] = (g_adv_rpt_num & 0xFF00) >> 8;

        adv_rpt[2] =  g_scn_rsp_num & 0x00FF;
        adv_rpt[3] = (g_scn_rsp_num & 0xFF00) >> 8;

        bt_hci_write(bench_fd , ADV_IND_BYTE_WIDTH + SCN_RSP_BYTE_WIDTH , adv_rpt);

        return FX_SUCCESS;
    }

    if(hci_dut_num_of_packets_cnt_check(rxbuf) == FX_SUCCESS) {
        printf("\n## num of packets cnt query [%d] ##\n\n" , g_num_pkts);

        u8 buf[2];

        buf[0] =  g_num_pkts & 0x00FF;
        buf[1] = (g_num_pkts & 0xFF00) >> 8;

        bt_hci_write(bench_fd , 2 , buf);

        return FX_SUCCESS;
    }

    if(hci_dut_data_buffer_overflow_cnt_check(rxbuf) == FX_SUCCESS) {
        printf("\n## data buffer overflow cnt query [%d] ##\n\n" , g_data_buffer_overflow_num);

        u8 buf[2];

        buf[0] =  g_data_buffer_overflow_num & 0x00FF;
        buf[1] = (g_data_buffer_overflow_num & 0xFF00) >> 8;

        bt_hci_write(bench_fd , 2 , buf);

        return FX_SUCCESS;
    }

    if(hci_dut_rx_acl_packets_cnt_check(rxbuf) == FX_SUCCESS) {
        printf("\n## rx acl packets cnt query [%d] ##\n\n" , g_rx_acl_packets_cnt);

        u8 buf[2];

        buf[0] =  g_rx_acl_packets_cnt & 0x00FF;
        buf[1] = (g_rx_acl_packets_cnt & 0xFF00) >> 8;

        bt_hci_write(bench_fd , 2 , buf);

        return FX_SUCCESS;
    }

	return FX_FAIL;
}

u8 hci_event_query_check(u8 *rxbuf){

	u8 buf_num = ASYNC_BUFF_NUM_INVALID;

	if( (_BT_HCI_INDICATOR_QUERY_EVENT_   == rxbuf[_BT_HCI_IDX_INDICATOR_])&&
	    (_BT_HCI_EVENT_OP_LE_EVENT_       == rxbuf[_BT_HCI_IDX_EVENT_CODE_]))
	{
		switch (rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_])
		{
			case  _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_COMPLETE_:
				printf("query connection_complete\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_]+10;
			break;

			case  _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_UPDATE_COMPLETE_:
				printf("query connection_update_complete\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_]+10;
			break;

			case  _BT_HCI_LE_EVENT_SUB_OP_READ_REMOTE_USED_FEATURES_COMPLETE_:
				printf("query read_remote_used_features_complete\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_]+10;
			break;

			case  _BT_HCI_LE_EVENT_SUB_OP_LONG_TERM_KEY_REQUEST_:
				printf("query LTK_requeset\n");
			break;
			case  _BT_HCI_LE_EVENT_SUB_OP_ADVERTISING_REPORT_://shift subcode +20 + event_type
			    printf("query advertising report\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_] +20 +rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_+2] ;
			break;
		}
	}
	else if( _BT_HCI_INDICATOR_QUERY_EVENT_ == rxbuf[_BT_HCI_IDX_INDICATOR_])
	{
		switch (rxbuf[_BT_HCI_IDX_EVENT_CODE_])
		{
		case _BT_HCI_EVENT_OP_DISCONNECTION_COMPLETE_:
				printf("query disconnection_complete\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
		    break;

		case _BT_HCI_EVENT_OP_ENCRYPTION_CHANGE_:
				printf("query encryption_change\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
		    break;

		case _BT_HCI_EVENT_OP_ENCRYPTION_KEY_REFRESH_COMPLETE_:
				printf("query encryption_key_refresh_complete\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
		    break;

			case _BT_HCI_EVENT_OP_READ_REMOTE_VERSION_INFORMATION_COMPLETE_:
				printf("query read_remote_version_info_complete\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
			break;

            case _BT_HCI_EVENT_OP_DATA_BUFFER_OVERFLOW_:
				printf("query data_buffer_overflow\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
            break;

            case _BT_HCI_EVENT_OP_NUMBER_OF_COMPLETED_PACKETS_:
				printf("query number of completed packets\n");
				buf_num =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
            break;

		}
	}
	else if((_BT_HCI_INDICATOR_QUERY_ACL_DATA_ == rxbuf[_BT_HCI_IDX_INDICATOR_])&&
	                               (0xFF == rxbuf[_BT_HCI_IDX_INDICATOR_ + 1]))
	{
			printf("# query acl_data\n");
			buf_num = ACL_BUFF_NUM;
	}

	return buf_num;


}

void _acl_data_check(u8 *rxbuf){

    int        rx_seq     = 0;
    int        cmp_len    = 0;

    rx_seq  =   (rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_]   << 16) +
                (rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_+1] << 8)  +
                (rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_+2] << 0)  ;

    if(rx_seq != g_seq_num){
        LOG_DUT("rx_seq[%x] != sg_seq_num[%x]\n" , rx_seq ,g_seq_num);
        while(1){};
    }

    for( ; cmp_len < rxbuf[_BT_HCI_IDX_ACL_DATA_LENGTH_]-4 ; cmp_len ++){

        if( rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_+3+cmp_len] != rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_+2]){
            LOG_DUT("rx_payload[%x] != [%x]\n" ,
                    rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_+3+cmp_len] ,
                    rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_+3]);
            while(1){};
        };
    }

    g_seq_num ++ ;


}

// return: (note: ASYNC_BUFF_NUM_INVALID = 0)
// > 0: queued.
//   0: bypass to bench.
//  -1: DROP.
//
s32 hci_event_check(u8 *rxbuf) {

	s32 result = ASYNC_BUFF_NUM_INVALID;

	if( (_BT_HCI_INDICATOR_EVENT_   == rxbuf[_BT_HCI_IDX_INDICATOR_])&&
	    (_BT_HCI_EVENT_OP_LE_EVENT_ == rxbuf[_BT_HCI_IDX_EVENT_CODE_]))
	{
        printf(COLOR);

		switch (rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_])
		{
			case  _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_COMPLETE_:
				LOG_DUT("# recv connection_complete [0x%x]\n" ,rxbuf[_BT_HCI_EVT_CON_COMP_STATUS]);
				result =rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_]+10;
			break;

			case  _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_UPDATE_COMPLETE_:
				LOG_DUT("# recv connection_update_complete \n");
				result =rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_]+10;
			break;

			case  _BT_HCI_LE_EVENT_SUB_OP_READ_REMOTE_USED_FEATURES_COMPLETE_:
				LOG_DUT("# recv read_remote_used_features_complete\n");
				result =rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_]+10;
			break;

			case  _BT_HCI_LE_EVENT_SUB_OP_LONG_TERM_KEY_REQUEST_:
				LOG_DUT("# recv LTK_request\n");
			break;

			case  _BT_HCI_LE_EVENT_SUB_OP_ADVERTISING_REPORT_://shift subcode +20 + event_type

                if(rxbuf[_BT_HCI_EVT_ADV_RPT_EVT_TYPE] == HCI_SUBEVENT_ADV_RPT_EVENT_SCAN_RSP){
                    g_scn_rsp_num ++ ;
                }else{
                    g_adv_rpt_num ++ ;
                }
			    LOG_DUT("# recv advertising report ; total adv_rpt[%d] scn_rsp[%d]\n" , g_adv_rpt_num , g_scn_rsp_num);
				result =rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_] +20 +
					    rxbuf[_BT_HCI_IDX_EVENT_PARAMETER_+2] ;

			break;
		}

        printf(NONE);
	}
	else if( _BT_HCI_INDICATOR_EVENT_ == rxbuf[_BT_HCI_IDX_INDICATOR_])
	{
        printf(COLOR);

		switch (rxbuf[_BT_HCI_IDX_EVENT_CODE_])
		{
		case _BT_HCI_EVENT_OP_DISCONNECTION_COMPLETE_:
				LOG_DUT("# recv disconnction_complete [0x%x]\n" , rxbuf[_BT_HCI_EVT_DISCONN_COMP_REASON]);
				result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
			break;

		case _BT_HCI_EVENT_OP_ENCRYPTION_CHANGE_:
				LOG_DUT("# recv encryption_change\n");
				result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
			break;

		case _BT_HCI_EVENT_OP_COMMAND_COMPLETE_:
                if((0x00 == rxbuf[_BT_HCI_IDX_EVENT_COMMAND_OP_])&&
                   (0x00 == rxbuf[_BT_HCI_IDX_EVENT_COMMAND_OP_+1])){
				LOG_DUT("# recv command complete NOP\n");
				result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
                }
			break;

		case _BT_HCI_EVENT_OP_COMMAND_STATUS_:
                if((0x00 == rxbuf[_BT_HCI_IDX_EVENT_STATUS_COMMAND_OP_])&&
                   (0x00 == rxbuf[_BT_HCI_IDX_EVENT_STATUS_COMMAND_OP_+1])){
				LOG_DUT("# recv command status NOP\n");
				result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
                }
			break;

			case _BT_HCI_EVENT_OP_HARDWARE_ERROR_:
				LOG_DUT("# recv hardware error\n");
				result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
			break;

		case _BT_HCI_EVENT_OP_ENCRYPTION_KEY_REFRESH_COMPLETE_:
				LOG_DUT("# recv encryption_key_refresh_complete\n");
				result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
			break;

			case _BT_HCI_EVENT_OP_READ_REMOTE_VERSION_INFORMATION_COMPLETE_:
				LOG_DUT("# recv read_remote_version_info_complete\n");
				result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
			break;

            case _BT_HCI_EVENT_OP_DATA_BUFFER_OVERFLOW_:
				result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
                g_data_buffer_overflow_num ++;
                LOG_DUT("# recv data buffer overflow ; total [%d]\n" , g_data_buffer_overflow_num);
            break;

            case _BT_HCI_EVENT_OP_NUMBER_OF_COMPLETED_PACKETS_:
                if(rxbuf[_BT_HCI_IDX_EVENT_NUM_OF_PACKETS] !=0 ){
                    result =rxbuf[_BT_HCI_IDX_EVENT_CODE_];
                    g_num_pkts += rxbuf[_BT_HCI_IDX_EVENT_NUM_OF_PACKETS] ;
				LOG_DUT("# recv number of completed packets [%d] ; total [%d]\n" ,rxbuf[_BT_HCI_IDX_EVENT_NUM_OF_PACKETS] , g_num_pkts);
                }
            break;

			case 0xFF:
				LOG_DUT("# recv unknown event (0xFF)\n");
				result = -1;
			break;

		}
        printf(NONE);

	}
	else if(_BT_HCI_INDICATOR_ACL_DATA_ == rxbuf[_BT_HCI_IDX_INDICATOR_])
	{
	    g_rx_acl_packets_cnt ++;

        printf(COLOR);

		LOG_DUT("# recv acl_data [%x][%x][%x], total [%d]\n" ,
            rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_] ,
            rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_ + 1] ,
            rxbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_ + 2] ,
            g_rx_acl_packets_cnt
            );

        printf(NONE);

        if (g_pld_check_enable == 1){
            _acl_data_check(rxbuf);
        }

		result = ACL_BUFF_NUM ;
	}
	return result;
}

const char vender_ti[]  = {"ti"};
const char vender_ssv[] = {"ssv"};
const char vender_ssv_icmd[] = {"ssvicmd"};
/*use condor as dut, switch form icmd to hci cmd*/
const char vender_ssv_condor[] = {"ssvcondor"};
const char vender_csr[] = {"csr"};

s16 dut_condor_bridge_hci(char* port_name){

    int dev_fd      = 0 ;
    s16 rxbytes;
    u8  rxbuf[SOCKET_BUF_SIZE] = {0};

    printf("# Condor ICMD bridge HCI setting : set dut baudrate to 9600 ==>\n");

    //#0720 , default 9600
    if (FX_FAIL == ttyInit(&dev_fd, port_name ,B9600)) {
        return -1;
    }

    printf("# disable dut RTS/CTS\n");
    ttyEnableHwFlowCtrl(dev_fd, FALSE);

    printf("# Wait device ready\n");
    u8 dev_ready_buf[2] = {0x0d, 0x0c};
    bt_hci_write(dev_fd, 2, dev_ready_buf);
    do {
        sleep(1);
        rxbytes = bt_hci_read_icmd(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);

    printf("# Set full duplex\n");
    u8 full_duplex_buf[2] = {0x0d, 0xff};
    bt_hci_write(dev_fd, 2, full_duplex_buf);

    //printf("# enable dut RTS/CTS\n");
    //ttyEnableHwFlowCtrl(dev_fd, TRUE);

    printf("# Set acl evt to external host\n");
    u8 acl_evt_to_external_host[4] = {0x01, 0x06, 0xfc, 0x00};
    bt_hci_write(dev_fd, 4, acl_evt_to_external_host);
    do {
        sleep(1);
        rxbytes = bt_hci_read(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);

    /*printf("# Set device baudrate to 115200\n");
    u8 baud_rate[7] = {0x0a, 0x31, 0x04, 0x00 ,0xC2, 0x01 ,0x00};
    bt_hci_write(dev_fd, 7, baud_rate);
    do {
        sleep(1);
        rxbytes = bt_hci_read_icmd(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);*/


    return dev_fd;

}


s16 dut_icmd_bridge_hci(char* port_name){

    int dev_fd      = 0 ;
    s16 rxbytes;
    u8  rxbuf[SOCKET_BUF_SIZE] = {0};

    printf("# ICMD bridge HCI setting : set dut baudrate to 9600 ==>\n");

    //#0720 , default 9600
    if (FX_FAIL == ttyInit(&dev_fd, port_name ,B9600)) {
        return -1;
    }

    printf("# disable dut RTS/CTS\n");
    ttyEnableHwFlowCtrl(dev_fd, FALSE);

    printf("# Wait device ready\n");
    u8 dev_ready_buf[2] = {0x0d, 0x0c};
    bt_hci_write(dev_fd, 2, dev_ready_buf);
    do {
        sleep(1);
        rxbytes = bt_hci_read_icmd(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);

    printf("# Set full duplex\n");
    u8 full_duplex_buf[2] = {0x0d, 0xff};
    bt_hci_write(dev_fd, 2, full_duplex_buf);

    printf("# enable dut RTS/CTS\n");
    ttyEnableHwFlowCtrl(dev_fd, TRUE);

    printf("# Set acl evt to external host\n");
    u8 acl_evt_to_external_host[4] = {0x01, 0x06, 0xfc, 0x00};
    bt_hci_write(dev_fd, 4, acl_evt_to_external_host);
    do {
        sleep(1);
        rxbytes = bt_hci_read(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);

    printf("# Set device baudrate to 115200\n");
    u8 baud_rate[7] = {0x0a, 0x31, 0x04, 0x00 ,0xC2, 0x01 ,0x00};
    bt_hci_write(dev_fd, 7, baud_rate);
    do {
        sleep(1);
        rxbytes = bt_hci_read_icmd(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);
#if 0
    //# write efuse
    printf("# efuse write : Set MAC Hi to 84:84:84\n");
    u8 mac_hi[10] = {0x0a, 0x27, 0x07, 0x01 ,0x00, 0x84 ,0x84 ,0x84 ,0x00 ,0x0};
    bt_hci_write(dev_fd, 10, mac_hi);
    do {
        sleep(1);
        rxbytes = bt_hci_read_icmd(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);

    //# write debounce
    printf("# reg write : Set debounce enable\n");
    u8 reg_debounce[15] = {0x0a, 0x2f, 12, 0x2c, 0x02, 0xc0 ,0x00 ,0xFF ,0xFF ,0xFF , 0xFF ,0x00 ,0x00 ,0xFF , 0x01};
    bt_hci_write(dev_fd, 15, reg_debounce);
    do {
        sleep(1);
        rxbytes = bt_hci_read_icmd(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);
#endif
    //# write device config
    printf("# device config \n");
    u8 dev_config[22] = {0x0a, 0x01, 0x13,
                         0x84, 0x84, 0x84 ,0x84 ,0x84 ,0x84 ,
                         0x01, 0x01 ,0x01 ,0x01 ,0x01 ,0x01 ,
                         0x01, 0x00 ,0x00 ,0x00 ,0x00 ,0x00 , 0x00
                         };
    bt_hci_write(dev_fd, 22, dev_config);
    do {
        sleep(1);
        rxbytes = bt_hci_read_icmd(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);

    printf("# Set Reset\n");
    u8 reset[3] = {0x0a, 0x21, 0x00};
    bt_hci_write(dev_fd, 3, reset);
    do {
        sleep(1);
        rxbytes = bt_hci_read_icmd(dev_fd, rxbuf);
    } while (rxbytes == 0);
    print_charray(rxbytes,rxbuf);

    printf("# set dut Baudrate to 115200\n");
    if (FX_FAIL == ttyChangeBaudRate(dev_fd,B115200)) {
        return -1;
    }

    return dev_fd;

}

// return
//	> 0: success, handle to csr hci_dev
//	= 0: fail
//
static int csr_init(const char *dev_str, const char *vendor_str)
{
	int	handle;
	char	cmd[64];

	printf(COLOR_INFO "csr init ...\n" COLOR_NONE);
/*
	sprintf(cmd, "hciconfig %s down", dev_str);
	printf("%s\n", cmd);
	system(cmd);
	sprintf(cmd, "hciconfig %s up", dev_str);
	printf("%s\n", cmd);
	system(cmd);
*/
	sprintf(cmd, "hciconfig %s noscan", dev_str);
	printf("%s\n", cmd);
	system(cmd);
	sprintf(cmd, "hciconfig");
	printf("%s\n", cmd);
	system(cmd);

	if ((handle = ssv_hci_open(dev_str, "csr")) == 0)
		return 0;
	if (ssv_hci_init_filter(handle) != 0)
		return 0;

	ssv_hci_dump(handle);
	printf("\n");

	return handle;
}

int main(s32 argc, char *argv[])
{
    u16     len;
    u8      buf[SOCKET_BUF_SIZE]   = {0};
    u8      rxbuf[SOCKET_BUF_SIZE] = {0};
    int     dev_fd      = 0 ;
    u8      ret         = 0;
	s32		_s32;
    s16     rxbytes;

    u16     cmd_len     = 0;
    u16     pld_len     = 0;
    u8      *pbuf          ;

    int     dev         = 0;
	int	h_csr		= 0;

    // # bench interface
    int dut;
    s32 bench_fd;

    char fd_path[64];
	char cmd[64];

    if(argc != 4) {
        printf("uasge:[DUT_NUM] [DEV_PORT] [VENDOR]\n");
        return ERROR;
    }

    // # argument dispatch
    // ## argv-1: dut
    dut = atoi(argv[1]);
    printf("dut %d init\n", dut);

    // ## argv-2: ttyX / hciX

    char dut_log_file[64] ;
    log_file = dut_log_file ;

    char now[256];
    log_time(now);

    sprintf( dut_log_file , "%s_%s_%s" , argv[2] , argv[3] , now);

    LOG_DUT("# start dut log @ %s\n" , now);

    if( 0 == strncmp(argv[2], "ttyUSB", 6)){
        // ## argv-3: vender
        if (FX_SUCCESS == strcmp(vender_ti , argv[3])) {
            if (FX_FAIL == ttyInit(&dev_fd, argv[2] ,B115200)) {
                return ERROR;
            }

            TI_LoadPatch(dev_fd);
            dev = DEV_TI;
        }

        if (FX_SUCCESS == strcmp(vender_ssv , argv[3])) {

            if (FX_FAIL == ttyInit(&dev_fd, argv[2] ,B115200)) {
                return ERROR;
            }

            dev = DEV_SSV;
        }

        if (FX_SUCCESS == strcmp(vender_ssv_icmd , argv[3])) {
            dev_fd = dut_icmd_bridge_hci(argv[2]);
            if(dev_fd <= 0){
                return ERROR;
            }

            dev = DEV_SSV;
        }

        if (FX_SUCCESS == strcmp(vender_ssv_condor , argv[3])) {
            dev_fd = dut_condor_bridge_hci(argv[2]);
            if(dev_fd <= 0){
                return ERROR;
            }

            dev = DEV_SSV;
        }
    }
    else if( 0 == strncmp(argv[2], "hci", 3)) {
		if (strcmp(argv[3], vender_csr) != 0) {
			printf(COLOR_ERR "invalid param [VENDOR]\n" COLOR_NONE);
			return ERROR;
		}
		if ((h_csr = csr_init(argv[2], argv[3])) == 0)
			return ERROR;
		// init internal data for csr in bt_hci_lib.
		bt_hci_init_csr();

		dev_fd	= ssv_hci_get_dd(h_csr);
        dev	= DEV_CSR;
    }
	else {
		printf(COLOR_ERR "invalid param\n" COLOR_NONE);
		return ERROR;
	}

    // # socket with bench
    printf("# socket init with dut %d\n", dut);
    sprintf(fd_path, "/tmp/ctrl_bench_dut%d", dut);

    if(server_socket_init(fd_path, &bench_fd) == SOCKET_SUCCESS) {
        printf("# socket init success \n");
    }
    else {
        printf("# socket init failed  \n");
        return ERROR ;
    }
	sprintf(cmd, "chmod 777 %s", fd_path);
	system(cmd);

    // # select with bench and uart
    while(1) {
        usleep(100);
        // #check socket that from bench
        ret = socket_msg_get_nonblocking(bench_fd, &len, buf);

        cmd_len = len ;
        pbuf    = buf ;

        while((ret == SOCKET_SUCCESS)&&(cmd_len !=0)) {

            LOG_DUT("# bench  msg -> len = %2d\n",cmd_len);

#ifdef READ_EVT_DEBUG
            print_charray(cmd_len,pbuf);
#endif

			//event contents query from Utester
            ret = hci_event_query_check(pbuf);

            if ( ASYNC_BUFF_NUM_INVALID != ret){  //send async buff to bench
                printf("# async buffer[%d] -> bench\n\n" , ret);

                if(ret != ACL_BUFF_NUM){
                    socket_msg_send(bench_fd,
                                    ASYNC_BUFF[ret].buf[_BT_HCI_IDX_EVENT_LENGTH_] + _BT_HCI_W_EVENT_HEADER_,
                                    &(ASYNC_BUFF[ret].buf));//total len = payloadlen+header

#if 0
                    print_charray(  ASYNC_BUFF[ret].buf[_BT_HCI_IDX_EVENT_LENGTH_] + _BT_HCI_W_EVENT_HEADER_,
                                    ((u8*)(ASYNC_BUFF[ret].buf)));
#endif
                }
                else{
                    socket_msg_send(bench_fd,
                                    ASYNC_BUFF[ret].buf[_BT_HCI_IDX_ACL_DATA_LENGTH_] + _BT_HCI_W_ACL_DATA_HEADER_,
                                    &(ASYNC_BUFF[ret].buf));//total len = payloadlen+header
                }

                // clean the buff after move out
                memset(&(ASYNC_BUFF[ret].buf) , 0 , BUFF_SIZE_MAX);

                pld_len = (pbuf[_BT_HCI_IDX_EVENT_LENGTH_] + _BT_HCI_W_EVENT_HEADER_) ;

            }
            else{
                if(_BT_HCI_INDICATOR_ACL_DATA_ == pbuf[_BT_HCI_IDX_INDICATOR_]){
                    printf(COLOR);
                    LOG_DUT("# Write acl_data[%x][%x][%x]\n" ,
                        pbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_] ,
                        pbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_ + 1] ,
                        pbuf[_BT_HCI_IDX_ACL_DATA_PAYLOAD_ + 2]);
                    printf(NONE);

					if (dev == DEV_CSR) {
						hci_acl_cmd_proc_v2_4csr(bench_fd, dev_fd, cmd_len, pbuf);
					}
					else {
						hci_acl_cmd_proc_v2(dev_fd , cmd_len ,pbuf);
					}

                    //# update the cmd_len
                    pld_len = (pbuf[_BT_HCI_IDX_ACL_DATA_LENGTH_] + _BT_HCI_W_ACL_DATA_HEADER_) ;
                }
                else{
                    //send to lowtester
                    pld_len = (pbuf[_BT_HCI_IDX_CMD_LENGTH_] + _BT_HCI_W_CMD_HEADER_) ;
                    //printf("cmd_len %d ,pld_len %d \n" , cmd_len ,pld_len);

                    ret = hci_cmd_dut_func( bench_fd, pbuf);

					// transparently bypass HCI raw format packet to device.
                    if (ret != FX_SUCCESS) {
						if (dev == DEV_CSR)
							bt_hci_write_4csr(dev_fd, pld_len, pbuf);
						else
							bt_hci_write(dev_fd, pld_len, pbuf);
                    }
                }
            }

            //printf("cmd_len %d ,pld_len %d \n" , cmd_len ,pld_len);

            cmd_len -= pld_len ;
            pbuf += pld_len ;

        }

        // #check socket that from dev
        if (dev == DEV_CSR){
			rxbytes = bt_hci_read_csr(dev_fd, rxbuf, sizeof(rxbuf));
        }
        else{
            rxbytes = bt_hci_read(dev_fd,rxbuf);

            if (rxbytes == FX_FAIL){
                exit(1);
            }
        }

        if (rxbytes > 0){
            LOG_DUT("# device msg <- len = %2d\n",rxbytes);

#ifdef READ_EVT_DEBUG
            print_charray(rxbytes,rxbuf);
#endif
            if (rxbytes > 255){
                goto FUN_FAIL;
            }

            _s32 = hci_event_check(rxbuf);

            //if uart data is event / le event , ret != 0
            //and return event /subevent  , will be store to buffer[subcode]
			if (ASYNC_BUFF_NUM_INVALID == _s32) {
                //#pass to bench
                socket_msg_send(bench_fd,rxbytes,rxbuf);
			}
			else if (-1 == _s32) {
				printf("-> event drop\n\n");
            }
			else {
                memcpy(&(ASYNC_BUFF[_s32].buf) , rxbuf , rxbytes);
                printf("-> async buffer[%d]\n\n", _s32);
            }
        }

    }
FUN_FAIL:
    close(dev_fd);
    close(bench_fd);
    printf("socket closed\n");

    return ERROR;
}

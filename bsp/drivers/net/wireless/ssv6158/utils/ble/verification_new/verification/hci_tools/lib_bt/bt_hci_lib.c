#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <types.h>
#include <stdint.h>					// for types in hci.h
#include "../lib_bluez/bluetooth.h"		// for bdaddr_t in hci.h

#include <lib/ssv_lib.h>
#include "bt_hci_lib.h"
#include "bt_hci_event_declare.h"
#include "../log/log.h"

#include "../lib_bluez/hci.h"

// include vs library

// external
//extern bt_hci_event* bt_hci_event_tbl[];
//extern bt_hci_le_event* bt_hci_le_event_tbl[];

// NOTE: plz sync SOCKET_BUF_SIZE to task.h
#ifndef SOCKET_BUF_SIZE
#define SOCKET_BUF_SIZE		1024
#endif

extern const int bt_hci_event_tbl_size;
extern const int bt_hci_le_event_tbl_size;

char*	hci_error_code_str(u8 code) {
	switch (code) {
	case 0x00:	return "Success";
	case 0x01:	return "Unknown HCI Command";
	case 0x02:	return "Unknown Connection Identifier";
	case 0x03:	return "Hardware Failure";
	case 0x04:	return "Page Timeout";
	case 0x05:	return "Authentication Failure";
	case 0x06:	return "PIN or Key Missing";
	case 0x07:	return "Memory Capacity Exceeded";
	case 0x08:	return "Connection Timeout";
	case 0x09:	return "Connection Limit Exceeded";
	case 0x0A:	return "Synchronous Connection Limit To A Device Exceeded";
	case 0x0B:	return "ACL Connection Already Exists";
	case 0x0C:	return "Command Disallowed";
	case 0x0D:	return "Connection Rejected due to Limited Resources";
	case 0x0E:	return "Connection Rejected due to Security Reasons";
	case 0x0F:	return "Connection Rejected due to Unacceptable BD_ADDR";
	case 0x10:	return "Connection Accept Timeout Exceed";
	case 0x11:	return "Unsupported Feature or Parameter Value";
	case 0x12:	return "Invalid HCI Command Parameters";
	case 0x13:	return "Remote User Terminated Connection";
	case 0x14:	return "Remote Device Terminated Connection due to Low Resources";
	case 0x15:	return "Remote Device Terminated Connection due to Power Off";
	case 0x16:	return "Connection Terminated by Local Host";
	case 0x17:	return "Repeated Attempts";
	case 0x18:	return "Pairing Not Allowed";
	case 0x19:	return "Unknown LMP PDU";
	case 0x1A:	return "Unsupported Remote/LMP Feature";
	case 0x1B:	return "SCO Offset Rejected";
	}
	return "";
}

char*	hci_event_code_str(u8 code) {
	switch (code)
	{
	case 0x03:	return "Connection Complete";
	case 0x04:	return "Connection Request";
	case 0x05:	return "Disconnection Complete";
	case 0x0B:	return "Read Remote Supported Features Complete";
	case 0x0C:	return "Read Remote Version Information Complete";
	case 0x0E:	return "Command Complete";
	case 0x0F:	return "Command Status";
	case 0x10:	return "Hardware Error";
	case 0x13:	return "Number of Completed Packets";
	case 0x38:	return "Link Supervision Timeout Changed";
	case 0x3D:	return "Remote Host Supported Features Notification";
	case 0x3E:	return "LE Meta Event";
	}
	return "Other";
}

void hci_event_socket_dump(u8 *buf, int len) {
	u32		_u32;
	u16		_u16;
	u8		*prm;	// event parameter
	int		i;
	char	str[32];

	printf("< hci event > len = %d\n", len);
	printf("%-20s: 0x%02x (%s)\n", "Event Code", buf[1], hci_event_code_str(buf[1]));
	printf("%-20s: %d\n", "Param Length", buf[2]);
	prm = buf + 3;
	if (buf[1] == 0x10)		goto Hardware_Error;
	if (buf[1] == 0x0E)	goto Command_Complete;
	if (buf[1] == 0x0F)	goto Command_Status;
	// LE_Meta_Event
	if (buf[1] == 0x3E)	{
		if (prm[0] == 0x01)		goto LE_Connection_Complete;
		if (prm[0] == 0x02)		goto LE_Advertising_Report;
		if (prm[0] == 0x03)		goto LE_Connection_Update_Complete;
		if (prm[0] == 0x04)		goto LE_Read_Remote_Used_Features_Complete;
		if (prm[0] == 0x05)		goto LE_Long_Term_Key_Request;
	}
	return;

Hardware_Error:
	printf("%-20s: 0x%02X\n", "Hardware_Code", prm[0]);
	return;
Command_Complete:
	printf("%-20s: %d\n", "Num_HCI_Command_Packets", prm[0]);
	_u16 = prm[2] << 8 | prm[1];
	printf("%-20s: 0x%04x\n", "Command_Opcode", _u16);
	return;
Command_Status:
	printf("%-20s: %d (%s)\n", "Status", prm[0], hci_error_code_str(prm[0]));
	printf("%-20s: %d\n", "Num_HCI_Command_Packets", prm[1]);
	_u16 = prm[3] << 8 | prm[2];
	printf("%-20s: 0x%04x\n", "Command_Opcode", _u16);
	return;
LE_Connection_Complete:
	printf("%-20s: 0x01 (LE_Connection_Complete)\n", "Subevent_Code");
	printf("%-20s: 0x%02x (%s)\n",	"Status", prm[1], hci_error_code_str(prm[1]));
	_u16 = prm[3] << 8 | prm[2];
	printf("%-20s: 0x%04x (%d)\n",	"Connection_Handle", _u16, _u16);
	// printf("%-20s: %d\n", "Role", prm[4]);
	// printf("%-20s: %d\n", "Peer_Address_Type", prm[5]);
	printf("%-20s: %02X:%02X:%02X:%02X:%02X:%02X\n", "Peer_Address", prm[11],prm[10],prm[9],prm[8],prm[7],prm[6]);
	_u16 = prm[13] << 8 | prm[12];
	printf("%-20s: 0x%04X\n", "Conn_Interval", _u16);
	_u16 = prm[15] << 8 | prm[14];
	printf("%-20s: 0x%04X\n", "Conn_Latency", _u16);
	_u16 = prm[17] << 8 | prm[16];
	printf("%-20s: 0x%04X\n", "Supervision_Timeout", _u16);
	printf("%-20s: 0x%02X\n", "Master_Clock_Accuracy", prm[18]);
	return;
LE_Advertising_Report:
	printf("%-20s: 0x02 (LE_Advertising_Report)\n", "Subevent_Code");
	printf("%-20s: %d\n", "Num_Reports", prm[1]);
	// TBD
	return;
LE_Connection_Update_Complete:
	printf("%-20s: 0x03 (LE_Connection_Update_Complete)\n", "Subevent_Code");
	printf("%-20s: 0x%02x (%s)\n",	"Status", prm[1], hci_error_code_str(prm[1]));
	_u16 = prm[3] << 8 | prm[2];
	printf("%-20s: 0x%04x (%d)\n",	"Connection_Handle", _u16, _u16);
	_u16 = prm[5] << 8 | prm[4];
	printf("%-20s: 0x%04X\n", "Conn_Interval", _u16);
	_u16 = prm[7] << 8 | prm[6];
	printf("%-20s: 0x%04X\n", "Conn_Latency", _u16);
	_u16 = prm[9] << 8 | prm[8];
	printf("%-20s: 0x%04X\n", "Supervision_Timeout", _u16);
	return;
LE_Read_Remote_Used_Features_Complete:
	printf("%-20s: 0x04 (LE_Read_Remote_Used_Features_Complete)\n", "Subevent_Code");
	printf("%-20s: 0x%02x (%s)\n",	"Status", prm[1], hci_error_code_str(prm[1]));
	_u16 = prm[3] << 8 | prm[2];
	printf("%-20s: 0x%04x (%d)\n",	"Connection_Handle", _u16, _u16);
	_u32 = ((prm[ 7]<<24)|(prm[ 6]<<16)|(prm[ 5]<< 8)|(prm[ 4]));
	printf("%-20s: 0x%08X",		"LE_Features", _u32);
	_u32 = ((prm[11]<<24)|(prm[10]<<16)|(prm[ 9]<< 8)|(prm[ 8]));
	printf("%08X\n", _u32);
	return;
LE_Long_Term_Key_Request:
	printf("%-20s: 0x05 (LE_Long_Term_Key_Request)\n", "Subevent_Code");
	_u16 = prm[2] << 8 | prm[1];
	printf("%-20s: 0x%04x (%d)\n",	"Connection_Handle", _u16, _u16);
	_u32 = ((prm[ 6]<<24)|(prm[ 5]<<16)|(prm[ 4]<< 8)|(prm[ 3]));
	printf("%-20s: 0x%08X",		"Random_Number", _u32);
	_u32 = ((prm[10]<<24)|(prm[ 9]<<16)|(prm[ 8]<< 8)|(prm[ 7]));
	printf("%08X\n", _u32);
	_u16 = prm[12] << 8 | prm[11];
	printf("%-20s: 0x%04x\n",	"Encrypted_Diversifier", _u16);
	return;
}


void bt_hci_dump_error_ind_bytes(int fd){

    u8 garbage[255] ;
    u8 loop_cnt = 0;
    u8 read_cnt = 0;

    read_cnt = read(fd, &garbage, 255) ;

    printf("dump garbage bytes = %d\n" , read_cnt);

    for( ; loop_cnt < read_cnt ; loop_cnt ++){
         printf("byte[%d] = 0x%02x\n" , loop_cnt , garbage[loop_cnt]);
    }

}

// -------------------------------------------------------
//					ring buffer for CSR
// -------------------------------------------------------
#define	RINGBUF_INVALID_PARAM	-1
#define	RINGBUF_OVERFLOW		-2
#define	RINGBUF_FULL			-3
#define	RINGBUF_EMPTY			-4
#define	RINGBUF_EXCEED_READ		-5

#define RINGBUF_CAPACITY	SOCKET_BUF_SIZE

typedef struct {
	u8		buf[RINGBUF_CAPACITY];
	u32		ptr;		// access ptr, from 0 to (SOCKET_BUF_SIZE-1)
	u32		size;		// # of occupied bytes.
} ringbuf_st;

static ringbuf_st	s_ringbuf = {0};

static void ringbuf_dump(ringbuf_st *p)
{
	int		n;

	printf(COLOR_DBG);
	printf("< ring buf > ptr = %d, size = %d\n", p->ptr, p->size);
	if (p->ptr + p->size < RINGBUF_CAPACITY)
	{
		printf("[%d] to [%d]\n", p->ptr, p->ptr + p->size);
		print_charray(p->size, p->buf + p->ptr);
	}
	else
	{
		printf("[%d] to [%d]\n", p->ptr, RINGBUF_CAPACITY-1);
		print_charray(RINGBUF_CAPACITY - p->ptr, p->buf + p->ptr);

		n = p->size - (RINGBUF_CAPACITY - p->ptr);
		printf("[%d] to [%d]\n", 0, n-1);
		print_charray(n, p->buf);
	}
	printf(COLOR_NONE);
}

// 'end_ptr' means 'ending buf[] entry that is NOT occupied'.
//
// return NULL: error (buf maybe already overflowed)
static u8*	ringbuf_end_ptr(ringbuf_st *p)
{
	assert(p->size < RINGBUF_CAPACITY);

	if (p->size == 0) {
		return (p->buf + p->ptr);
	}
	// NO wrap
	if (p->ptr + p->size < RINGBUF_CAPACITY)
		return (p->buf + (p->ptr + p->size));
	// wrap
	return (p->buf + (p->ptr + p->size - RINGBUF_CAPACITY));
}

// return
//	>= 0: success, # of bytes written.
//	< 0: error
//
static int ringbuf_write(INOUT ringbuf_st *p, u8 *buf, int len)
{
	u8	*p_end;
	u32	len_1st, len_2nd;

	if (p->size >= RINGBUF_CAPACITY) {
		//printf(COLOR_ERR "%s(): RINGBUF_FULL!\n" COLOR_NONE, __FUNCTION__);
		return RINGBUF_FULL;
	}
	if (p->size + len > RINGBUF_CAPACITY) {
		//printf(COLOR_ERR "%s(): RINGBUF_OVERFLOW!\n" COLOR_NONE, __FUNCTION__);
		return RINGBUF_OVERFLOW;
	}
	if (buf == NULL) {
		//printf(COLOR_ERR "%s(): RINGBUF_INVALID_PARAM!\n" COLOR_NONE, __FUNCTION__);
		return RINGBUF_INVALID_PARAM;
	}
	if (len == 0) {
		//printf(COLOR_WARN "%s(): len = 0\n" COLOR_NONE, __FUNCTION__);
		return 0;
	}
	// locate end_ptr
	if ((p_end = ringbuf_end_ptr(p)) == NULL) {
		NEVER_RUN_HERE();
	}

	if (p->ptr + p->size + len < RINGBUF_CAPACITY) {	// NO wrap
		memcpy(p_end, buf, len);
	}
	else {	// wrap
		len_1st = RINGBUF_CAPACITY - (p->ptr + p->size);
		len_2nd = len - len_1st;
		// copy 1st half
		memcpy(p_end, buf, len_1st);
		// copy 2nd half
		memcpy(p->buf, buf+len_1st, len_2nd);
	}
	p->size += len;
	return len;
}

// copy # of bytes to 'buf', but do NOT change the ringbuf internal status.
//
// look_pos: the offset to the (p->ptr) to look.
//			 for example, if (p->ptr = 2, look_pos = 2) then
//           we will looking at buf[4] from the start.
//			 note that 'look_pos' value is starting from 0.
//
// look_num: # of bytes to be copied.
//
// return:
// > 0: success, # of bytes copied.
// < 0: error
static int ringbuf_look(IN ringbuf_st *p, OUT u8 *buf, IN int look_pos, IN int look_num)
{
	u32		ptr, len_1st, len_2nd;

	if (p->size == 0) {
		//printf(COLOR_ERR "%s(): RINGBUF_EMPTY\n" COLOR_NONE, __FUNCTION__);
		return RINGBUF_EMPTY;
	}
	if (p->size < look_num)	{
		//printf(COLOR_ERR "%s(): RINGBUF_EXCEED_READ\n" COLOR_NONE, __FUNCTION__);
		return RINGBUF_EXCEED_READ;
	}
	if ((ptr = p->ptr + look_pos) >= RINGBUF_CAPACITY) {
		ptr -= RINGBUF_CAPACITY;
	}

	if (ptr + look_num <= RINGBUF_CAPACITY) {	// NO wrap
		memcpy(buf, p->buf + ptr, look_num);
	}
	else {	// wrap
		len_1st = RINGBUF_CAPACITY - ptr;
		len_2nd = look_num - len_1st;
		// copy 1st half
		memcpy(buf, p->buf + ptr, len_1st);
		// copy 2nd half
		memcpy(buf+len_1st, p->buf, len_2nd);
	}
	return look_num;
}

// consume bytes, this function will change ringbuf status.
// but while FAIL, the ringbuf status remains unchanged.
//
// return
//	0: success
// -1: fail
static int ringbuf_consume(INOUT ringbuf_st *p, int num)
{
	u32		len_1st, len_2nd;

	if (p->size < num) {
		//printf(COLOR_ERR "%s(): p->size(%d) < num(%d\n" COLOR_NONE, __FUNCTION__, p->size, num);
		return -1;
	}

	p->ptr += num;
	if (p->ptr >= RINGBUF_CAPACITY)
		p->ptr -= RINGBUF_CAPACITY;

	if (p->ptr + num <= RINGBUF_CAPACITY) {		// NO wrap
		p->ptr	+= num;
	}
	else {	// wrap
		len_1st = RINGBUF_CAPACITY - p->ptr;
		len_2nd = num - len_1st;
		p->ptr = len_2nd - 1;
	}

	p->size -= num;
	return 0;
}

// read out an event from ringbuf
//
// return:
//	> 0: # of bytes read out.
//	< 0: other condition
static int ringbuf_read_event(INOUT ringbuf_st *p, OUT u8 *evt_buf, int evt_buf_len)
{
	int		idx, r;
	u8		_u8;
	u16		_u16;

	if (evt_buf_len < SOCKET_BUF_SIZE) {
		//printf(COLOR_ERR "%s(): evt_buf_len(= %d) < %d\n" COLOR_NONE, __FUNCTION__, evt_buf_len, SOCKET_BUF_SIZE);
		return RINGBUF_INVALID_PARAM;
	}
	if (p->size == 0) {
		//printf(COLOR_ERR "%s(): ringbuf empty!\n" COLOR_NONE, __FUNCTION__);
		return RINGBUF_EMPTY;
	}
	idx = 0;
	// INDICATOR (1 byte)
	if ((r = ringbuf_look(p, evt_buf+idx, idx, 1)) != 1) {
		//printf("INDICATOR ringbuf_look() = %d\n", r);
		return r;
	}
	//printf("INDICATOR = %02x\n", evt_buf[idx]);
	idx++;
	if (evt_buf[0] == _BT_HCI_INDICATOR_EVENT_)		goto EVENT;
	if (evt_buf[0] == _BT_HCI_INDICATOR_ACL_DATA_)	goto ACL_DATA;

	printf(COLOR_ERR);
	printf("INDICATOR invalid value = 0x%02X\n", evt_buf[0]);
	printf(COLOR_NONE);
	ringbuf_dump(p);
	exit(1);

EVENT:
	// EVENT_CODE (1 byte)
	if ((r = ringbuf_look(p, evt_buf+idx, idx, 1)) != 1) {
		//printf("EVENT_CODE ringbuf_look() = %d\n", r);
		return r;
	}
	//printf("EVT_CODE = %02x\n", evt_buf[idx]);
	idx++;

	// PARAMETER_TOTAL_LENGTH (1 byte)
	if ((r = ringbuf_look(p, evt_buf+idx, idx, 1)) != 1) {
		//printf("PARAM_LEN ringbuf_look() = %d\n", r);
		return r;
	}
	//printf("PARAM_LEN = %02x\n", evt_buf[idx]);
	idx++;

	// EVENT_PARAMETER (n bytes)
	_u8 = evt_buf[2];
	if ((r = ringbuf_look(p, evt_buf+idx, idx, _u8)) != _u8) {
		//printf("EVT_PARAM  ringbuf_look() = %d\n", r);
		return r;
	}
	//printf("EVT_PARAM = %d bytes\n", r);
	idx += _u8;

	// now, all bytes look successfully, consume it!
	r = idx;
	ringbuf_consume(p, r);
	return r;

ACL_DATA:
	// HANDLE_PB_BC (2 bytes)
	if ((r = ringbuf_look(p, evt_buf+idx, idx, 2)) != 2) {
		//printf("HANDLE_PB_BC ringbuf_look() = %d\n", r);
		return r;
	}
	idx += 2;
	// DATA_TOTAL_LENGTH (2 bytes)
	if ((r = ringbuf_look(p, evt_buf+idx, idx, 2)) != 2) {
		//printf("DATA_LEN ringbuf_look() = %d\n", r);
		return r;
	}
	_u16 = evt_buf[3];
	_u16 += (evt_buf[4] << 8);
	//printf("DATA_LEN = %d\n", _u16);
	idx += 2;

	// ACL_DATA
	if ((r = ringbuf_look(p, evt_buf+idx, idx, _u16)) != _u16) {
		//printf("ACL_DATA ringbuf_look() = %d\n", r);
		return r;
	}
	idx += _u16;

	// now, all bytes look successfully, consume it!
	r = idx;
	ringbuf_consume(p, r);
	return r;
}

void bt_hci_init_csr(void)
{
	memset(&s_ringbuf, 0, sizeof(ringbuf_st));
}

int bt_hci_read_csr(int fd, u8 *rxbuf, int buf_len)
{
	char	tmp[SOCKET_BUF_SIZE];
	int		nr, nw;

	if ((nr = read(fd, tmp, sizeof(tmp))) < 0) {
		return nr;
	}
#if 0
	printf(COLOR_DBG);
	printf("read()\n");
	print_charray(nr, tmp);
	printf(COLOR_NONE);
#endif
	if ((nw = ringbuf_write(&s_ringbuf, tmp, nr)) < 0) {
		printf(COLOR_ERR "%s(): ringbuf_write() = %d\n" COLOR_NONE, __FUNCTION__, nw);
		exit(1);
	}
	if (nw != nr) {
		printf(COLOR_ERR "%s(): nw(%d) != nr (%d)\n" COLOR_NONE, __FUNCTION__, nw, nr);
		exit(1);
	}
#if 0
	printf(COLOR_DBG);
	printf("ringbuf_write()\n");
	ringbuf_dump(&s_ringbuf);
	printf(COLOR_NONE);
#endif
	//printf("rb w: %d, size = %d, ptr = %d\n", nw, s_ringbuf.size, s_ringbuf.ptr);
	nr = ringbuf_read_event(&s_ringbuf, rxbuf, buf_len);
	//printf("rb r: %d, size = %d, ptr = %d\n", nr, s_ringbuf.size, s_ringbuf.ptr);
#if 0
	printf(COLOR_DBG);
	printf("ringbuf_read_event()\n");
	print_charray(nr, rxbuf);
	printf(COLOR_NONE);
#endif
	return nr;
}

int bt_hci_read_icmd(int fd, u8 *rxbuf)
{
    fd_set fds;
    int i;
    int byte_idx          = 0;
    int data_coming = 0;
    int read_cnt          = 0;

    unsigned char buf;
    struct timeval time_out,time_start,time_end;

    while(1)
    {
	//printf("byte_idx %d , byte_cnt_in_event %d" , byte_idx ,byte_cnt_in_event);
        // # wait uart
        //ret = -1;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

	if (data_coming){
		time_out.tv_sec = 1;	//blocking
		time_out.tv_usec = 0;
	}
	else{
		time_out.tv_sec = 0;	//non-blocking
		time_out.tv_usec = 0;
	}

        i = select(fd + 1, &fds, NULL, NULL, &time_out);
        // # check uart
        switch(i)
	{
		case -1:
			printf("select failed!\n");
			//ret = -1;
			break;

		case 0:
			//printf("no socket_msg_get\n");
			data_coming = 0 ;
			//ret = 0 ;
			break;

		default:
                //# data coming , should become blocking to waiting more uart data
			data_coming = 1;

			if(FD_ISSET(fd,&fds))
			{
				// ## read 1-byte from uart
				read_cnt = read(fd, &buf, 1) ;
				if(read_cnt != 1) {
					printf("read error on byte %d, errno = %d ! read_cnt = %d\n", byte_idx, errno ,read_cnt);
					return FX_FAIL;
				}
				else{	//copy uart data to buf
					//printf("%d %02x\n" , byte_idx , buf);
					rxbuf[byte_idx] = buf;
				}

				// ## next byte
				byte_idx++;
			}
			break;
	}

	if ( 0 == data_coming)
		return byte_idx;
    }

    return byte_idx;
}


int bt_hci_read(int fd, u8 *rxbuf)
{
    fd_set fds;
    int i;
    int byte_cnt_in_event = _BT_HCI_W_EVENT_HEADER_;
    int byte_idx          = 0;
    int data_coming = 0;
    int acl_data_coming = 0;
    int read_cnt          = 0;

    unsigned char buf;
    struct timeval time_out,time_start,time_end;

    while(byte_idx != byte_cnt_in_event)
    {
        usleep(100);
	//printf("byte_idx %d , byte_cnt_in_event %d \n" , byte_idx ,byte_cnt_in_event);
        // # wait uart
        //ret = -1;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

	if (data_coming){
		time_out.tv_sec  = 1;	//blocking
		time_out.tv_usec = 0;
	}
	else{
		time_out.tv_sec  = 0;	//non-blocking
		time_out.tv_usec = 0;
	}

        i = select(fd + 1, &fds, NULL, NULL, &time_out);
        // # check uart
        switch(i)
	{
		case -1:
			printf("select failed!\n");
			//ret = -1;
			break;

		case 0:
			//printf("no socket_msg_get\n");
			data_coming = 0 ;
			//ret = 0 ;
			break;

		default:
                //# data coming , should become blocking to waiting more uart data
			data_coming = 1;

			if(FD_ISSET(fd,&fds))
			{
				// ## read 1-byte from uart
				read_cnt = read(fd, &buf, 1) ;
				if(read_cnt != 1) {
					printf("read error on byte %d, errno = %d ! read_cnt = %d\n", byte_idx, errno ,read_cnt);
					return FX_FAIL;
				}
				else{	//copy uart data to buf
					//printf("[%d] = %02x\n" , byte_idx , buf);
					rxbuf[byte_idx]=buf;
				}

				if(acl_data_coming){
					switch (byte_idx)
					{
						case _BT_HCI_IDX_ACL_DATA_LENGTH_:

							 byte_cnt_in_event += buf ;
							//printf("buf_len[0] =%d\n\n" , buf);
						break;

						case _BT_HCI_IDX_ACL_DATA_LENGTH_ + 1:

							 byte_cnt_in_event += buf << 8 ;
							//printf("buf_len[1] =%d\n\n" , buf);
						break;


						default: // _BT_HCI_IDX_EVENT_PARAMETER_
						break;
					}
				}
				else {
						switch (byte_idx)
						{
						   case _BT_HCI_IDX_INDICATOR_:

                               if(_BT_HCI_INDICATOR_COMMAND_HCILL_GO_TO_SLEEP_IND == buf){
								   printf("recv ind : ctrl going to sleep mode \n", buf);

                                   u8 ack[1] ;
                                   ack[0] = _BT_HCI_INDICATOR_COMMAND_HCILL_GO_TO_SLEEP_ACK ;

                                   if (write(fd, ack, 1) == 1)  {
                                        printf("send ack to ctrl\n\n");
                                   }
                                   break;
                               }

                               if(_BT_HCI_INDICATOR_COMMAND_HCILL_WAKE_UP_ACK == buf){
								   printf("recv ack : ctrl going to wake up mode \n", buf);

                                   u8 sleep[13] ={0x01 ,0x0c ,0xfd ,0x09 ,0x01 ,0x01 ,0x00 ,0xff ,0xff ,0xff ,0xff ,0x64 ,0x00};
                                   write(fd,sleep,13);
                                   printf("auto send sleep to ctrl\n\n");
                                   break;
                               }


                               if(_BT_HCI_INDICATOR_COMMAND_HCILL_WAKE_UP_IND == buf){
								   printf("recv ind : ctrl going to wakp up mode \n", buf);

                                   u8 ack[1] ;
                                   ack[0] = _BT_HCI_INDICATOR_COMMAND_HCILL_WAKE_UP_ACK ;

                                   if (write(fd, ack, 1) == 1)  {
                                        printf("send ack to ctrl\n\n");
                                   }
                                   break;
                               }

							   if(_BT_HCI_INDICATOR_ACL_DATA_ == buf){
								   acl_data_coming = 1;
								   byte_cnt_in_event = _BT_HCI_W_ACL_DATA_HEADER_ ;
								   //printf("Get acl data\n");
							   }
							   else if (buf != _BT_HCI_INDICATOR_EVENT_) {
								   printf("error: not a event, buf indicator as 0x%02x \n", buf);
                                   bt_hci_dump_error_ind_bytes(fd);

								   return FX_FAIL;
							   }
							   break;

						   case _BT_HCI_IDX_EVENT_LENGTH_:
							   byte_cnt_in_event += buf;
							   break;

						   default: // _BT_HCI_IDX_EVENT_PARAMETER_
							   break;
						}
				}
				// ## next byte
				byte_idx++;
			}
			break;
	}

	if ( 0 == data_coming)
		return FX_SUCCESS;
    }

    return byte_idx;
}

int bt_hci_read_event(int fd, unsigned char *code, unsigned char *length, unsigned char *parameter) {

    fd_set fds;

    int byte_cnt_in_event = _BT_HCI_W_EVENT_HEADER_;
    int byte_idx          = 0;

    unsigned char buf;

    while(byte_idx != byte_cnt_in_event) {

        // # wait uart
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        select(fd + 1, &fds, NULL, NULL, NULL);

        // # check uart
        if (FD_ISSET(fd, &fds)) {
            // ## read 1-byte from uart
            if(read(fd, &buf, 1) != 1) {
                printf("read error on byte %d, errno = %d !\n", byte_idx, errno);
                return FX_FAIL;
            }
            switch (byte_idx) {

                case _BT_HCI_IDX_INDICATOR_:
                    if (buf != _BT_HCI_INDICATOR_EVENT_) {
                        printf("error: not a event, buf indicator as 0x%02x \n", buf);
                        bt_hci_dump_error_ind_bytes(fd);

                        return FX_FAIL;
                    }
                    break;

                case _BT_HCI_IDX_EVENT_CODE_:
                    *code = buf;
                    break;

                case _BT_HCI_IDX_EVENT_LENGTH_:
                    *length = buf;
                    byte_cnt_in_event += buf;
                    break;

                default: // _BT_HCI_IDX_EVENT_PARAMETER_
                    parameter[byte_idx - _BT_HCI_W_EVENT_HEADER_] = buf;
                    break;
            }
            // ## next byte
            byte_idx++;
        }
    }
    // PRINT_DATA(EVENT_TYPE, event, result);
    PRINTF_FX("## code: %02x, ## length %02x\n", *code, *length);
    PRINTF_FX("## parameter \n");
    print_charray(*length, parameter);
    //bt_hci_event_parsing(*code, *length, parameter);

    return FX_SUCCESS;
}

int bt_hci_parameter_parsing(unsigned char parameter_tbl_size, bt_hci_parameter_entry *parameter_tbl,\
                             unsigned char length, unsigned char *parameter) {

    int idx_parameter_tbl;
    int idx_parameter;
    bt_hci_parameter_entry *loop_parameter_entry;

    idx_parameter = 0;
    for(idx_parameter_tbl=0; ((idx_parameter < length) && (idx_parameter_tbl < parameter_tbl_size)); idx_parameter_tbl++) {

        loop_parameter_entry = parameter_tbl+idx_parameter_tbl;

        loop_parameter_entry->value = (parameter + idx_parameter);

        if ((loop_parameter_entry->property & _BT_HCI_PARAMETER_PROP_VARIABLE_LENGTH_)) {
            loop_parameter_entry->length = (length - idx_parameter);
            idx_parameter_tbl++;
            break;
        }
        else {
            idx_parameter += loop_parameter_entry->length;
        }

    }
    return idx_parameter_tbl;
}

int bt_hci_parameter_building (unsigned char parameter_tbl_size, bt_hci_parameter_entry *parameter_tbl,\
                               int length_max, unsigned char *parameter) {

    int idx_parameter_tbl;
    int idx_parameter;
    int idx_parameter_nxt;
    int parameter_entry_length;

    idx_parameter = 0;
    for(idx_parameter_tbl=0; idx_parameter_tbl < parameter_tbl_size; idx_parameter_tbl++) {

        parameter_entry_length = parameter_tbl[idx_parameter_tbl].length;
        idx_parameter_nxt      = idx_parameter + parameter_entry_length;


        if((parameter_entry_length != 0) && (idx_parameter_nxt <= length_max)) {
            memcpy(parameter+idx_parameter, parameter_tbl[idx_parameter_tbl].value, parameter_entry_length);

            idx_parameter = idx_parameter_nxt;
        }
        else {
            break;
        }
    }


    //
    return idx_parameter;
}


int bt_hci_le_event_parsing(unsigned char code, unsigned char length, unsigned char *parameter) {

    int idx_event_tbl;

    PRINTF_FX("## code: %02x, ## length %02x\n", code, length);
    PRINTF_FX("## parameter... \n");
    print_charray(length, parameter);

    for(idx_event_tbl=0; idx_event_tbl < bt_hci_le_event_tbl_size; idx_event_tbl++) {
        PRINTF_FX("### %d %d  ... \n", code ,bt_hci_le_event_tbl[idx_event_tbl]->sub_opcode);

        if(code == bt_hci_le_event_tbl[idx_event_tbl]->sub_opcode) {
            PRINTF_FX("#### got %s event ... \n", bt_hci_le_event_tbl[idx_event_tbl]->name);
            //
            bt_hci_parameter_parsing(bt_hci_le_event_tbl[idx_event_tbl]->parameter_tbl_size,
                                     bt_hci_le_event_tbl[idx_event_tbl]->parameter_tbl,
                                     length, parameter);
	        //printf("######after param parsing####\n");
            bt_hci_le_event_tbl[idx_event_tbl]->handle(bt_hci_le_event_tbl[idx_event_tbl]->parameter_tbl_size,
                                                    bt_hci_le_event_tbl[idx_event_tbl]->parameter_tbl);
            break;
        }
    }

    if (idx_event_tbl < bt_hci_le_event_tbl_size) {
        return FX_SUCCESS;
    }
    else {
        return FX_FAIL;
    }
}


int bt_hci_event_parsing(unsigned char code, unsigned char length, unsigned char *parameter) {

    int idx_event_tbl;

    PRINTF_FX("## code: %02x, ## length %02x\n", code, length);
    PRINTF_FX("## parameter... \n");
    print_charray(length, parameter);

    for(idx_event_tbl=0; idx_event_tbl < bt_hci_event_tbl_size; idx_event_tbl++) {
        // PRINTF_FX("### checking %d event ... \n", idx_event_tbl);

        if(code == bt_hci_event_tbl[idx_event_tbl]->opcode) {
            PRINTF_FX("#### got %s event ... \n", bt_hci_event_tbl[idx_event_tbl]->name);
            //
            bt_hci_parameter_parsing(bt_hci_event_tbl[idx_event_tbl]->parameter_tbl_size,
                                     bt_hci_event_tbl[idx_event_tbl]->parameter_tbl,
                                     length, parameter);
	        //printf("######after param parsing####\n");
            bt_hci_event_tbl[idx_event_tbl]->handle(bt_hci_event_tbl[idx_event_tbl]->parameter_tbl_size,
                                                    bt_hci_event_tbl[idx_event_tbl]->parameter_tbl);
            break;
        }
    }

    if (idx_event_tbl < bt_hci_event_tbl_size) {
        return FX_SUCCESS;
    }
    else {
        return FX_FAIL;
    }

}

int bt_hci_write_4csr(int fd, int buf_length, unsigned char *buf) {

	uint8_t		plen;
	uint16_t	ogf, ocf;
	void		*param = 0;

	ogf	= buf[_BT_HCI_IDX_CMD_OPCODE_OGF] >> 2;
	ocf	= buf[_BT_HCI_IDX_CMD_OPCODE_OCF];
	plen	= buf[_BT_HCI_IDX_CMD_LENGTH_];
	if (plen > 0)
		param	= buf + _BT_HCI_IDX_CMD_PARAMETER_;

#if 0
	printf(COLOR_TRACE);
	printf("fd = %d\n", fd);
	printf("ogf = 0x%02x, ocf = 0x%04x, plen = %d\n", ogf, ocf, plen);
	for (i=0; i<plen; i++)
		printf("%02x ", *((unsigned char *)param+i));
	printf("\n");
	printf(COLOR_NONE);
#endif

	if (hci_send_cmd(fd, ogf, ocf, plen, param) == 0)
		return FX_SUCCESS;
	return FX_FAIL;
}

int bt_hci_write(int fd, int buf_length, unsigned char *buf) {

    if (write(fd, buf, buf_length) == buf_length)  {

        //PRINTF_FX("SUCCESS\n");
        return FX_SUCCESS;

    }
    else {

        //PRINTF_FX("FAIL\n");
        return FX_FAIL;
    }

}

int bt_hci_write_cmd(int fd, bt_hci_cmd* p_cmd) {

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
    PRINTF_FX("# Write Command %s\n", p_cmd->name);
    printf(NONE);
    print_charray(buf_length, buf);

    return bt_hci_write(fd, buf_length, buf);
}

int bt_hci_vs_patch (int fd, int patch_size, unsigned char *patch) {

    unsigned char buf[_BT_HCI_W_CMD_HEADER_ + _BT_HCI_W_PARAMTER_MAX_] = {0};
    int           buf_length;
    int           patch_idx = 0;

    unsigned char cmd_opcode[_BT_HCI_W_CMD_OPCODE_];
    unsigned char cmd_legth;
    unsigned char cmd_parameter[_BT_HCI_W_PARAMTER_MAX_] = { 0 };

    unsigned char event_code;
    unsigned char event_length;
    unsigned char event_parameter[_BT_HCI_W_PARAMTER_MAX_] = { 0 };

    if ((patch_size == 0) || (patch == NULL)) {
        printf("Error: invalid patch_size of patch pointer\n");
        return FX_FAIL;
    }

    patch_idx = 0;
    while(patch_idx < patch_size) {
        printf("# processing on patch_idx %d\n", patch_idx);

        // clear buf and temp space
        memset(buf,             0x55, sizeof(buf)            );
        memset(cmd_parameter,   0x55, sizeof(cmd_parameter)  );
        memset(event_parameter, 0x55, sizeof(event_parameter));

        // parse cmd
        cmd_legth = patch[patch_idx + _BT_HCI_IDX_CMD_LENGTH_];

        memcpy(cmd_opcode,    (patch + patch_idx + _BT_HCI_IDX_CMD_OPCODE_),    _BT_HCI_W_CMD_OPCODE_);
        memcpy(cmd_parameter, (patch + patch_idx + _BT_HCI_IDX_CMD_PARAMETER_), cmd_legth            );

        // copy to buf, then write
        buf_length = cmd_legth + _BT_HCI_W_CMD_HEADER_;
        memcpy(buf, (patch + patch_idx), buf_length);

        if (write(fd, buf, buf_length) != buf_length)  {
            printf("write error on patch_idx %d\n", patch_idx);
            return FX_FAIL;
        }

        // print success cmd
        PRINT_DATA(CMD_TYPE, buf, buf_length);

        // check event
        if (bt_hci_read_event(fd, &event_code, &event_length, event_parameter) != FX_SUCCESS) {
            printf("read_event error on patch_idx %d\n", patch_idx);
            return FX_FAIL;
        }

        // proceed to next command
        patch_idx += buf_length;
    }
    return FX_SUCCESS;
}

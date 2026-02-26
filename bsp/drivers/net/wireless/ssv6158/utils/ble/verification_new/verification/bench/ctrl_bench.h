#ifndef _CTRL_BENCH_H_
#define _CTRL_BENCH_H_

#include <signal.h>
#include "sys/time.h"

#include "./hci_tools/lib_bt/bt_hci_lib.h"

typedef struct ret_param
{
	int ret;
	int pkt;
	int channel;
}Ret_param;

typedef void (*EventHandle)(int, u8*);

int bench_init(int dev_num);
int dut_socket_init(int dut);
int dut_socket_get(int dut);
int bt_hci_write_cmd2socket(int fd, bt_hci_cmd* p_cmd);
int bt_hci_write_data2socket(int fd, bt_hci_acl_data* p_data);
void event_query_timer_config(u16 init_sec ,u32 init_usec ,u16 interval_sec,u16 interval_usec,struct itimerval *timer);
int event_query_timer_start(struct itimerval value , __sighandler_t handler );
void event_query_timer_stop(struct itimerval *timer);

#define MAX_DUTS 4
extern char*    dut_socket_fd_path  [MAX_DUTS+1];
extern int      dut_socket_fd       [MAX_DUTS+1];

/**
 * following define "bench-cmd"
 *
 */
#define _BT_HCI_INDICATOR_BCMD_     0xf0    // cmd for bench

#define _BT_HCI_W_BCMD_OPCODE_  1
#define _BT_HCI_W_BCMD_LENGTH_  1
#define _BT_HCI_W_BCMD_HEADER_  (_BT_HCI_W_INDICATOR_ +_BT_HCI_W_BCMD_OPCODE_ +_BT_HCI_W_BCMD_LENGTH_)

#define _BT_HCI_IDX_BCMD_OPCODE_    (_BT_HCI_IDX_INDICATOR_     +_BT_HCI_W_INDICATOR_   )
#define _BT_HCI_IDX_BCMD_LENGTH_    (_BT_HCI_IDX_BCMD_OPCODE_   +_BT_HCI_W_BCMD_OPCODE_ )
#define _BT_HCI_IDX_BCMD_PARAM_     (_BT_HCI_IDX_BCMD_LENGTH_   +_BT_HCI_W_BCMD_LENGTH_ )

typedef enum {
    BT_HCI_BCMD_OPCODE_EVENT_LOG_QUERY = 0x01,
    BT_HCI_BCMD_OPCODE_EVENT_LOG_DUMP,
    BT_HCI_BCMD_OPCODE_EVENT_LOG_RESET,
    BT_HCI_BCMD_OPCODE_STATISTIC_QUERY,
    BT_HCI_BCMD_OPCODE_STATISTIC_RESET,

} BT_HCI_BCMD_OP;

typedef struct bcmd_event_log_query_param {
    u8  event_code;
    u8  le_subevent_code;
    u8  latest;

} bcmd_event_log_query_param_st;

typedef struct bcmd_event_log_dump_param {
    u8  max_dump_cnt;

} bcmd_event_log_dump_param_st;

#define _BT_HCI_W_PARAMETER_BCMD_EVENT_LOG_QUERY_   (sizeof(bcmd_event_log_query_param_st))
#define _BT_HCI_W_PARAMETER_BCMD_EVENT_LOG_DUMP_    (sizeof(bcmd_event_log_dump_param_st))
#define _BT_HCI_W_PARAMETER_BCMD_EVENT_LOG_RESET_   0

/**
 * following define "bench-cmd"
 *
 */
#define _BT_HCI_INDICATOR_BEVENT_   0xf1    // event for bench

#define _BT_HCI_W_BEVENT_OPCODE_  1
#define _BT_HCI_W_BEVENT_LENGTH_  1
#define _BT_HCI_W_BEVENT_HEADER_  (_BT_HCI_W_INDICATOR_ +_BT_HCI_W_BEVENT_OPCODE_ +_BT_HCI_W_BEVENT_LENGTH_)

#define _BT_HCI_IDX_BEVENT_OPCODE_    (_BT_HCI_IDX_INDICATOR_       +_BT_HCI_W_INDICATOR_       )
#define _BT_HCI_IDX_BEVENT_LENGTH_    (_BT_HCI_IDX_BEVENT_OPCODE_   +_BT_HCI_W_BEVENT_OPCODE_   )
#define _BT_HCI_IDX_BEVENT_PARAM_     (_BT_HCI_IDX_BEVENT_LENGTH_   +_BT_HCI_W_BEVENT_LENGTH_   )

typedef enum {
    BT_HCI_BEVENT_OPCODE_EVENT_LOG_QUERY_END        = 0x01,
    BT_HCI_BEVENT_OPCODE_EVENT_LOG_QUERY_DUMP_END,
    BT_HCI_BEVENT_OPCODE_EVENT_LOG_QUERY_RESET_END,

} BT_HCI_BEVENT_OPCODE;

#define RCV_BEVENT_EVENT_LOG_QUERY_END(_BUF_) ( \
    (_BUF_[_BT_HCI_IDX_INDICATOR_]      == _BT_HCI_INDICATOR_BEVENT_) &&   \
    (_BUF_[_BT_HCI_W_BEVENT_OPCODE_]    == BT_HCI_BEVENT_OPCODE_EVENT_LOG_QUERY_END))

#define RCV_BEVENT_EVENT_LOG_DUMP_END(_BUF_) ( \
    (_BUF_[_BT_HCI_IDX_INDICATOR_]      == _BT_HCI_INDICATOR_BEVENT_) &&   \
    (_BUF_[_BT_HCI_W_BEVENT_OPCODE_]    == BT_HCI_BEVENT_OPCODE_EVENT_LOG_QUERY_DUMP_END))

#define RCV_BEVENT_EVENT_LOG_RESET_END(_BUF_) ( \
    (_BUF_[_BT_HCI_IDX_INDICATOR_]      == _BT_HCI_INDICATOR_BEVENT_) &&   \
    (_BUF_[_BT_HCI_W_BEVENT_OPCODE_]    == BT_HCI_BEVENT_OPCODE_EVENT_LOG_QUERY_RESET_END))

typedef struct bevent_event_log_query_end {
    u8  pending_events;

} bevent_event_log_query_end_st;

typedef struct bevent_event_log_dump_end {
    u8  pending_events;

} bevent_event_log_dump_end_st;

#define _BT_HCI_W_PARAM_BEVENT_EVENT_LOG_QUERY_END_ (sizeof(bevent_event_log_query_end_st))
#define _BT_HCI_W_PARAM_BEVENT_EVENT_LOG_DUMP_END_  (sizeof(bevent_event_log_dump_end_st))
#define _BT_HCI_W_PARAM_BEVENT_EVENT_LOG_END_END_   (0)

/**
 * BCMD handler.
 *  - return true if command is accepted, and execute successfully.
 *
 */
int bcmd_hdl(s32 fd, u8 cmd_len, u8* cmd_buf);

/**
 * bench log
 *  - may add some time-stamp in it
 *
 */
typedef struct event_log {

    u8  event_code;
    u8  le_subevent_code;

    u8  event[_BT_HCI_W_CMD_HEADER_ +_BT_HCI_W_PARAMTER_MAX_];

} event_log_st;

#define BENCH_EVENT_LOG_DEPTH   255
typedef struct event_log_ring {

    u8  rd_ptr;
    u8  wr_ptr;

    event_log_st    elements[BENCH_EVENT_LOG_DEPTH];

} event_log_ring_st;

/**
 * event_log_inq:
 *  - return the number of events in the ring
 *
 */
int bench_event_log_inq(event_log_ring_st* ring, event_log_st* log);

/**
 * event_log_deq:
 *  - return NULL if nothing in the ring
 *
 */
event_log_st* bench_event_log_deq(event_log_ring_st* ring);


/**
 * bench statistic
 *
 */

#endif

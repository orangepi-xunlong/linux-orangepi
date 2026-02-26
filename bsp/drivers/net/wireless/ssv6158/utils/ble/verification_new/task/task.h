#ifndef _TASK_H_
#define _TASK_H_
//#include <config.h>


//process use
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <types.h>


#define SOCKET_BUF_SIZE         1024
#define SOCKET_SUCCESS          0
#define SOCKET_FAILED           1

#define U8_TO_STREAM(p,uint8) {*(p)++ = (u8)(uint8);}
#define U16_TO_STREAM(p,uint16) {*(p)++ = (u8)(uint16);*(p)++ = (u8)((uint16)>>8);}

#define U32_TO_STREAM(p,uint32) {*(p)++ = (u8)(uint32);*(p)++ = (u8)((uint32)>>8);*(p)++ = (u8)((uint32)>>16);*(p)++ = (u8)((uint32)>>24);}

#define STREAM_TO_U8(p,uint8) {(uint8) = (u8)(*(p));p += 1;}
#define STREAM_TO_U16(p,uint16) {(uint16) = (u16)(*(p)) + ((u16)(*((p)+1))<<8);p += 2;}
#define STREAM_TO_U32(p,uint32) {(uint32) = (u32)(*(p)) + ((u32)(*((p)+1))<<8)+ ((u32)(*((p)+2))<<16)+ ((u32)(*((p)+3))<<24);p += 4;}

//these 4 macro can be replaced, use a pointer point to the head of an array, then move the pointer
#define U16_TO_STREAM_ARR(p,uint16) {p[0] = (u8)(uint16); p[1] = (u8)((uint16)>>8);}
#define U32_TO_STREAM_ARR(p,uint32) {p[0] = (u8)(uint32); p[1] = (u8)(uint32 >> 8);\
    p[2] = (u8)(uint32>>16);p[3] = (u8)(uint32>>24);}

#define STREAM_ARR_TO_U16(p,uint16) {uint16 = *p + ((u16)*(p+1)<<8);}
#define STREAM_ARR_TO_U32(p,uint32) {uint32 = *p + ((u32)*(p+1)<<8) + ((u32)*(p+2)<<16)\
    +((u32)*(p+3)<<24);}

//fd path for socket communication
enum{
    DUT_TYPE_DUMMY_CENTRAL,
    DUT_TYPE_DUMMY_PERIPHERAL,
    DUT_TYPE_BDROID_CENT,
    DUT_TYPE_BDROID_PERI,
    DUT_TYPE_MAX
}DUT_TYPE_E;

enum{
    DUT_SOCK_TYPE_TI,
    DUT_SOCK_TYPE_SSV,
    DUT_SOCK_TYPE_SIM,
    DUT_SOCK_TYPE_MAX
}DUT_SOCKET_TYPE_E;

typedef struct {
    s8 path[60];
}sock_fd_path_t;


//this is the format of messages between dut and bench
typedef struct task_msg_st {
    u32 msg_cmd;
    u16 msg_len;
    u8  msg_data[SOCKET_BUF_SIZE];
//    struct task_msg_st  *next;
} task_msg;

#define TASK_TIMEOUT 120

sock_fd_path_t dut_sock_fd_path[DUT_SOCK_TYPE_MAX] ;


u8 client_socket_init(char *fd_path,s32 *fd);
u8 server_socket_init(char *fd_path,s32 *fd);
u8 socket_msg_send(s32 fd,u16 len,void *buf);
u8 socket_msg_get(s32 fd,u16 *len,void *buf);

#define _TASK_CMD_WD_CMD_       16
#define _TASK_CMD_WD_SENDER_    8
#define _TASK_CMD_WD_RSV_       8

#define _TASK_CMD_B0_CMD_    0
#define _TASK_CMD_B0_SENDER_ (_TASK_CMD_B0_CMD_      + _TASK_CMD_WD_CMD_   )
#define _TASK_CMD_B0_RSV_    (_TASK_CMD_B0_SENDER_   + _TASK_CMD_WD_SENDER_)

#define _TASK_CMD_MSK_CMD_      ((1 << _TASK_CMD_WD_CMD_   ) -1)
#define _TASK_CMD_MSK_SENDER_   ((1 << _TASK_CMD_WD_SENDER_) -1)

#define _TASK_CMD_SENDER_PATTERN_   0
#define _TASK_CMD_SENDER_BENCH_     64
#define _TASK_CMD_SENDER_DUT_       128

// commands from pattern
#define _TASK_CMD_PATTERN_NOP_      0

// commands from bench
#define _TASK_CMD_BENCH_NOP_        0

// commands from dut
#define _TASK_CMD_DUT_NOP_        0

#define TASK_CMD_SET_VALUE(_SENDER_, _CMD_) ((_SENDER_ << _TASK_CMD_B0_SENDER_) + (_CMD_ << _TASK_CMD_B0_CMD_))

#define TASK_CMD_GET_CMD(_VALUE_)           ((_VALUE_ >> _TASK_CMD_B0_CMD_)    & _TASK_CMD_MSK_CMD_   )
#define TASK_CMD_GET_SENDER(_VALUE_)        ((_VALUE_ >> _TASK_CMD_B0_SENDER_) & _TASK_CMD_MSK_SENDER_)

u8 socket_msg_get_nonblocking(s32 fd,u16 *len,void *buf);


#endif /* _TASK_H_ */

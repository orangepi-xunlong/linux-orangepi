#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>

#include "../log/log.h"
#include "task.h"


//for socket communication, add new fd path here.
sock_fd_path_t dut_sock_fd_path[DUT_SOCK_TYPE_MAX] = {
    {"/tmp/dut_ti_"},
    {"/tmp/dut_ssv_"},
    {"/tmp/duv_sim_"},
};

u8 socket_msg_send(s32 fd,u16 len,void *buf)
{
    u8 ret = SOCKET_SUCCESS;
    u16 send_len = 0;
    send_len = send(fd,buf,len,0);
    if(send_len != len)
    {
        printf(ERR);
        LOG_PRINTF("\n!socket msg send failed!\n");
        printf(NONE);
        ret = SOCKET_FAILED;
    }
    return ret;
}

u8 socket_msg_get_nonblocking(s32 fd, u16 *len, void *buf)
{
    struct timeval time_out = {
        .tv_sec     = 0,
        .tv_usec    = 0,
    };
#ifdef BLOCK_SIGALRM
    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);

    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        printf("sigprocmask failed\n");
    } else {
        //printf("SIGARLM blocked\n");
    }
#endif

    fd_set fds;

    // # start receive socket data
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    *len = 0;
    if(select(fd +1, &fds, NULL, NULL, &time_out) >= 0) {
        if(FD_ISSET(fd, &fds)) {
            *len = recv(fd, buf, SOCKET_BUF_SIZE, 0);
        }
#ifdef BLOCK_SIGALRM
        if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
            printf("sigprocmask failed\n");
        } else {
            //printf("SIGALRM unblocked\n");
        }
#endif

        return SOCKET_SUCCESS;
    }
    else {
#ifdef BLOCK_SIGALRM
        if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
            printf("sigprocmask failed\n");
        } else {
            //printf("SIGALRM unblocked\n");
        }
#endif
        printf(ERR);
        printf("\n!select failed!\n");
        printf(NONE);
        return SOCKET_FAILED;
    }
}

u8 socket_msg_get(s32 fd,u16 *len,void *buf)
{
    u8 ret = SOCKET_SUCCESS;

    s8 i = 0;
    s32 max_fd = 0;
#ifdef BLOCK_SIGALRM
    sigset_t newmask, oldmask;
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGALRM);
#endif
    struct timeval time_out,time_start,time_end ,time_recv;
    fd_set fds;
    *len = 0;
    memset(buf , 0 , SOCKET_BUF_SIZE);

    //0 = non-blocking , here we use blocking for waiting DUT send Uart data back
    u8 time_out_sec  = TASK_TIMEOUT;
    time_out.tv_sec  = time_out_sec ;
    time_out.tv_usec = 0;
    gettimeofday(&time_start,NULL);
    //LOG_PRINTF("task: start time = %d\n",time_start.tv_sec);
#ifdef BLOCK_SIGALRM
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        printf("sigprocmask failed\n");
    } else {
        //printf("SIGARLM blocked\n");
    }
#endif

    //start receive socket data
    FD_ZERO(&fds);
    FD_SET(fd,&fds);
    max_fd = fd + 1;

#if 0
    i = select(max_fd,&fds,NULL,NULL,NULL);
#else // # select with timeout
RESELECT:
    i = select(max_fd,&fds,NULL,NULL,&time_out);
#endif
    switch(i)
    {
        case -1:
            perror("select");
            perror("recv_len: ");

            ret = SOCKET_FAILED;
            goto RESELECT;
        break;

        case 0:
	        //printf("socket_msg_get\n");
        break;

        default:
            if(FD_ISSET(fd,&fds))
            {
                *len = recv(fd,buf,SOCKET_BUF_SIZE,0);
                gettimeofday(&time_recv,NULL);
                //LOG_PRINTF("socket : recv msg @ %ld : %ld\n" , time_recv.tv_sec , time_recv.tv_usec);

                //return ret ;
            }
        break;
    }

    gettimeofday(&time_end,NULL);

    if(time_end.tv_sec >= (time_start.tv_sec + time_out_sec))
    {
        printf(ERR);
        LOG_PRINTF("socket msg time out for %d sec [fd :%d]\n" , time_out_sec , fd);
        printf(NONE);
        ret = SOCKET_FAILED;

    }
#ifdef BLOCK_SIGALRM
    if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
        printf("sigprocmask failed\n");
    } else {
        //printf("SIGALRM unblocked\n");
    }
#endif

    return ret;
}


u8 client_socket_init(char* fd_path, s32 *fd)
{
    u8 ret = SOCKET_SUCCESS;

    struct sockaddr_un client_add;

    *fd = socket(AF_UNIX,SOCK_STREAM,0);

    if(*fd == -1)
    {
        LOG_PRINTF("create socket failed!\n");
        ret = SOCKET_FAILED;
        return ret;
    }
    memset(&client_add,0,sizeof(struct sockaddr_un));

    client_add.sun_family = AF_UNIX;
    strcpy(client_add.sun_path,(const char*)fd_path);

    ret = connect(*fd,(struct sockaddr *)&client_add,sizeof(struct sockaddr_un));

    if(ret != SOCKET_SUCCESS)
    {
        perror("!client connect server error!\n");
        ret = SOCKET_FAILED;
    }

    return ret;
}


u8 server_socket_init(char* fd_path , s32 *fd)
{
    u8 ret = SOCKET_SUCCESS;
    s32 server_fd;
    u8 len;
    struct sockaddr_un client_add,server_add;

    memset(&client_add,0,sizeof(struct sockaddr_un));
    memset(&server_add,0,sizeof(struct sockaddr_un));
    server_add.sun_family = AF_UNIX;

    unlink((const char*)fd_path);
   // strncpy(dut_add.sun_path,fd_path,sizeof(dut_add.sun_path));
    strcpy(server_add.sun_path,(const char*)fd_path);

    server_fd = socket(AF_UNIX,SOCK_STREAM,0);
    if(server_fd == -1)
    {
        printf("server create socket error!\n");
        ret = SOCKET_FAILED;
        return ret;
    }

    len = sizeof(struct sockaddr_un);
    ret = bind(server_fd,(struct sockaddr *)&server_add,(socklen_t)len);
    if(ret != SOCKET_SUCCESS)
    {
        printf("bind error %d\n",ret);
         ret = SOCKET_FAILED;
        return ret;
    }

    //ret = listen(server_fd,3);
    ret = listen(server_fd,1);
    if(ret != SOCKET_SUCCESS)
    {
        printf("listen error %d\n",ret);
        ret = SOCKET_FAILED;
        return ret;
    }

    len = sizeof(struct sockaddr_un);
    printf("# waiting accept\n");
    *fd = accept(server_fd,(struct sockaddr *)&client_add,(socklen_t *)&len);

    if(*fd < 0)
    {
        printf("accept error !%d \n",ret);
        ret = SOCKET_FAILED;
        return ret;
    }

    printf("# server connect to client!\n");

    return ret;
}

//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include "vpuconfig.h"
#include "vputypes.h"

#include "../vdi_osal.h"
// supporting simple file system for customer reference.
// this file system supports to open a file as many as MAX_NUM_OF_FILES.
// each file uses memory space to save and load some data from FILE_BUFFER_BASE to FILE_BUFFER_SIZE
#define MAX_NUM_OF_FILES 2
#define FILE_BUFFER_BASE 0x5000000
#define FILE_BUFFER_SIZE 1024*1024*10
#define FILE_BUFFER_END (FILE_BUFFER_BASE+(FILE_BUFFER_SIZE*MAX_NUM_OF_FILES))

typedef struct fileio_buf_t {
    char *_ptr;
    int   _cnt;
    char *_base;
    int   _flag;
    int   _file;
    int   _charbuf;
    int   _bufsiz;
    char *_tmpfname;
} fileio_buf_t;

static fileio_buf_t s_file[MAX_NUM_OF_FILES];


int InitLog()
{
    return 1;
}

void DeInitLog()
{

}

void SetMaxLogLevel(int level)
{
}
int GetMaxLogLevel(void)
{
    return -1;
}

void LogMsg(int level, const char *format, ...)
{
    va_list ptr;
    char logBuf[MAX_PRINT_LENGTH] = {0};

    va_start( ptr, format );

    vsnprintf( logBuf, MAX_PRINT_LENGTH, format, ptr );

    va_end(ptr);

    printf(logBuf);

}


void osal_init_keyboard()
{

}

void osal_close_keyboard()
{

}

void * osal_memcpy(void * dst, const void * src, int count)
{
    return memcpy(dst, src, (size_t)count);
}

int osal_memcmp(const void* src, const void* dst, int size)
{
    return memcmp(src, dst, size);
}

void * osal_memset(void *dst, int val, int count)
{
    return memset(dst, val, count);
}

void * osal_malloc(int size)
{
    return malloc(size);
}

void * osal_realloc(void* ptr, int size)
{
    return realloc(ptr, size);
}

void osal_free(void *p)
{
    free(p);//lint -e{424}
}

int osal_fflush(osal_file_t fp)
{
    return 1;
}

int osal_feof(osal_file_t fp)
{
    return 0;
}
static int find_not_opened_file_idx()
{
    int ret = 0;
    int i;

    for (i = 0; i < MAX_NUM_OF_FILES; i++)
    {
        if (s_file[i]._bufsiz == 0)
            break;
    }

    ret = i;

    return ret;
}
osal_file_t osal_fopen(const char * osal_file_tname, const char * mode)
{
    fileio_buf_t *file;
    int idx = find_not_opened_file_idx();
    if (idx >= MAX_NUM_OF_FILES)
    {
        return ((void *)0);
    }
    file = (fileio_buf_t *)&s_file[idx];

    file->_base = (char *)(FILE_BUFFER_BASE + FILE_BUFFER_SIZE*idx);
    if (file->_base > (char *)FILE_BUFFER_END)
    {
        return ((void *)0);
    }

    file->_bufsiz = FILE_BUFFER_SIZE;
    file->_ptr = (char *)0;
    file->_file = idx;

    return file;
}
size_t osal_fwrite(const void * p, int size, int count, osal_file_t fp)
{
    fileio_buf_t *file = (fileio_buf_t *)fp;
    long addr;

    if (!file) {
        return (size_t)-1;
    }
    addr = (long)((int)file->_base+(int)file->_ptr);
    osal_memcpy((void *)addr, (void *)p, count*size);
    file->_ptr += count*size;
    return count*size;
}
size_t osal_fread(void *p, int size, int count, osal_file_t fp)
{
    fileio_buf_t *file = (fileio_buf_t *)fp;
    long addr;

    if (!file) {
        return (size_t)-1;
    }

    addr = (long)((int)file->_base+(int)file->_ptr);
    osal_memcpy((void *)p, (void *)addr, count*size);
    file->_ptr += count*size;

    return count*size;
}

long osal_ftell(osal_file_t fp)
{
    fileio_buf_t *file = (fileio_buf_t *)fp;
    if (!file) {
        return (long)-1;
    }
    return file->_bufsiz;
}

int osal_fseek(osal_file_t fp, long offset, int origin)
{
    fileio_buf_t *file = (fileio_buf_t *)fp;
    if (!file) {
        return (int)-1;
    }

    file->_ptr = (char *)offset;
    return offset;
}
int osal_fclose(osal_file_t fp)
{
    fileio_buf_t *file = (fileio_buf_t *)fp;
    if (!file) {
        return (int)-1;
    }

    file->_base = (char *)0;
    file->_bufsiz = 0;
    file->_ptr = (char *)0;

    return 1;
}
int osal_fscanf(osal_file_t fp, const char * _Format, ...)
{
    return 1;
}

int osal_fprintf(osal_file_t fp, const char * _Format, ...)
{
    va_list ptr;
    char logBuf[MAX_PRINT_LENGTH] = {0};

    va_start( ptr, _Format);

    vsnprintf(logBuf, MAX_PRINT_LENGTH, _Format, ptr);

    va_end(ptr);

    printf(logBuf);

    return 1;

}

int osal_kbhit(void)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return 0;
}
int osal_getch(void)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return -1;
}

int osal_flush_ch(void)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return -1;
}

void osal_msleep(Uint32 millisecond)
{
    Uint32 countDown = millisecond;
    while (countDown > 0) countDown--;
    VLOG(WARN, "<%s:%d> Please implemenet osal_msleep()\n", __FUNCTION__, __LINE__);
}

osal_thread_t osal_thread_create(void(*start_routine)(void*), void*arg)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return NULL;
}

Int32 osal_thread_join(osal_thread_t thread, void** retval)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return -1;
}

Int32 osal_thread_timedjoin(osal_thread_t thread, void** retval, Uint32 millisecond)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return 2;   /* error */
}

osal_mutex_t osal_mutex_create(void)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return NULL;
}

void osal_mutex_destroy(osal_mutex_t mutex)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
}

BOOL osal_mutex_lock(osal_mutex_t mutex)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return FALSE;
}

BOOL osal_mutex_unlock(osal_mutex_t mutex)
{
    return FALSE;
}

osal_sem_t osal_sem_init(Uint32 count)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return NULL;
}

void osal_sem_destroy(osal_sem_t sem)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
}

void osal_sem_wait(osal_sem_t sem)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
}

void osal_sem_post(osal_sem_t sem)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
}

int _gettimeofday( struct timeval *tv, void *tzvp )
{
    Uint64 t = 0;//__your_system_time_function_here__();  // get uptime in nanoseconds
    tv->tv_sec = t / 1000000000;  // convert to seconds
    tv->tv_usec = ( t % 1000000000 ) / 1000;  // get remaining microseconds

    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return 0;
}

Uint64 osal_gettime(void)
{
    VLOG(WARN, "<%s:%d> NEED TO IMPLEMENT %s\n", __FUNCTION__, __LINE__, __FUNCTION__);
    return 0ULL;
}

int osal_snprintf(char* str, size_t buf_size, const char *format, ...)
{
    return snprintf(str, buf_size, format);
}

#ifdef LIB_C_STUB
/*
* newlib_stubs.c
* the bellow code is just to build ref-code. customers will removed the bellow code bacuase they need a library which is related to the system library such as newlibc
*/
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

#ifndef STDOUT_USART
#define STDOUT_USART 0
#endif

#ifndef STDERR_USART
#define STDERR_USART 0
#endif

#ifndef STDIN_USART
#define STDIN_USART 0
#endif

#undef errno
extern int errno;

/*
environ
A pointer to a list of environment variables and their values.
For a minimal environment, this empty list is adequate:
*/
char *__env[1] = { 0 };
char **environ = __env;

int _write(int file, char *ptr, int len);

void _exit(int status) {
    _write(1, "exit", 4);
    while (1) {
        ;
    }
}

int _close(int file) {
    return -1;
}
/*
execve
Transfer control to a new process. Minimal implementation (for a system without processes):
*/
int _execve(char *name, char **argv, char **env) {
    errno = ENOMEM;
    return -1;
}
/*
fork
Create a new process. Minimal implementation (for a system without processes):
*/

int _fork() {
    errno = EAGAIN;
    return -1;
}
/*
fstat
Status of an open file. For consistency with other minimal implementations in these examples,
all files are regarded as character special devices.
The `sys/stat.h' header file required is distributed in the `include' subdirectory for this C library.
*/
int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

/*
getpid
Process-ID; this is sometimes used to generate strings unlikely to conflict with other processes. Minimal implementation, for a system without processes:
*/

int _getpid() {
    return 1;
}

/*
isatty
Query whether output stream is a terminal. For consistency with the other minimal implementations,
*/
int _isatty(int file) {
    switch (file){
    case STDOUT_FILENO:
    case STDERR_FILENO:
    case STDIN_FILENO:
        return 1;
    default:
        //errno = ENOTTY;
        errno = EBADF;
        return 0;
    }
}

/*
kill
Send a signal. Minimal implementation:
*/
int _kill(int pid, int sig) {
    errno = EINVAL;
    return (-1);
}

/*
link
Establish a new name for an existing file. Minimal implementation:
*/

int _link(char *old, char *new) {
    errno = EMLINK;
    return -1;
}

/*
lseek
Set position in a file. Minimal implementation:
*/
int _lseek(int file, int ptr, int dir) {
    return 0;
}

/*
sbrk
Increase program data space.
Malloc and related functions depend on this
*/
caddr_t _sbrk(int incr) {

    // extern char _ebss; // Defined by the linker
    char _ebss;
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0) {
        heap_end = &_ebss;
    }
    prev_heap_end = heap_end;

    //char * stack = (char*) __get_MSP();
    char * stack = 0;
    if (heap_end + incr >  stack)//lint !e413
    {
        _write (STDERR_FILENO, "Heap and stack collision\n", 25);
        errno = ENOMEM;
        return  (caddr_t) -1;
        //abort ();
    }

    heap_end += incr;
    return (caddr_t) prev_heap_end;

}

/*
read
Read a character to a file. `libc' subroutines will use this system routine for input from all files, including stdin
Returns -1 on error or blocks until the number of characters have been read.
*/

int _read(int file, char *ptr, int len) {
    int n;
    int num = 0;
    switch (file) {
    case STDIN_FILENO:
        for (n = 0; n < len; n++) {
            char c =0;
#if   STDIN_USART == 1
            while ((USART1->SR & USART_FLAG_RXNE) == (Uint16)RESET) {}
            c = (char)(USART1->DR & (Uint16)0x01FF);
#elif STDIN_USART == 2
            while ((USART2->SR & USART_FLAG_RXNE) == (Uint16) RESET) {}
            c = (char) (USART2->DR & (Uint16) 0x01FF);
#elif STDIN_USART == 3
            while ((USART3->SR & USART_FLAG_RXNE) == (Uint16)RESET) {}
            c = (char)(USART3->DR & (Uint16)0x01FF);
#endif
            *ptr++ = c;
            num++;
        }
        break;
    default:
        errno = EBADF;
        return -1;
    }
    return num;
}

/*
stat
Status of a file (by name). Minimal implementation:
int    _EXFUN(stat,( const char *__path, struct stat *__sbuf ));
*/

int _stat(const char *filepath, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

/*
times
Timing information for current process. Minimal implementation:
*/

clock_t _times(struct tms *buf) {
    return -1;
}

/*
unlink
Remove a file's directory entry. Minimal implementation:
*/
int _unlink(char *name) {
    errno = ENOENT;
    return -1;
}

/*
wait
Wait for a child process. Minimal implementation:
*/
int _wait(int *status) {
    errno = ECHILD;
    return -1;
}

/*
write
Write a character to a file. `libc' subroutines will use this system routine for output to all files, including stdout
Returns -1 on error or number of bytes sent
*/
int _write(int file, char *ptr, int len) {
    int n;
    switch (file) {
    case STDOUT_FILENO: /*stdout*/
        for (n = 0; n < len; n++) {
#if STDOUT_USART == 1
            while ((USART1->SR & USART_FLAG_TC) == (Uint16)RESET) {}
            USART1->DR = (*ptr++ & (Uint16)0x01FF);
#elif  STDOUT_USART == 2
            while ((USART2->SR & USART_FLAG_TC) == (Uint16) RESET) {
            }
            USART2->DR = (*ptr++ & (Uint16) 0x01FF);
#elif  STDOUT_USART == 3
            while ((USART3->SR & USART_FLAG_TC) == (Uint16)RESET) {}
            USART3->DR = (*ptr++ & (Uint16)0x01FF);
#endif
        }
        break;
    case STDERR_FILENO: /* stderr */
        for (n = 0; n < len; n++) {
#if STDERR_USART == 1
            while ((USART1->SR & USART_FLAG_TC) == (Uint16)RESET) {}
            USART1->DR = (*ptr++ & (Uint16)0x01FF);
#elif  STDERR_USART == 2
            while ((USART2->SR & USART_FLAG_TC) == (Uint16) RESET) {
            }
            USART2->DR = (*ptr++ & (Uint16) 0x01FF);
#elif  STDERR_USART == 3
            while ((USART3->SR & USART_FLAG_TC) == (Uint16)RESET) {}
            USART3->DR = (*ptr++ & (Uint16)0x01FF);
#endif
        }
        break;
    default:
        errno = EBADF;
        return -1;
    }
    return len;
}

#endif
//#endif


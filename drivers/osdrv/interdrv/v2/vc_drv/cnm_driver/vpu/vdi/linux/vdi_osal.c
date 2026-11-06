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
#include <linux/time.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <uapi/linux/sched/types.h>
#include <linux/moduleparam.h>
#include "vpuconfig.h"
#include "../vdi_osal.h"

#define ANSI_COLOR_ERR      "\x1b[31m"       // RED
#define ANSI_COLOR_TRACE    "\x1b[32m"       // GREEN
#define ANSI_COLOR_WARN     "\x1b[33m"       // YELLOW
#define ANSI_COLOR_INFO     "\x1b[34m"       // BLUE
// For future
#define ANSI_COLOR_MAGENTA  "\x1b[35m"       // MAGENTA
#define ANSI_COLOR_CYAN     "\x1b[36m"       // CYAN
#define ANSI_COLOR_RESET    "\x1b[0m"        // RESET

static unsigned log_decor = LOG_HAS_TIME | LOG_HAS_FILE | LOG_HAS_MICRO_SEC |
                            LOG_HAS_NEWLINE |
                            LOG_HAS_SPACE | LOG_HAS_COLOR;
int VPU_LOG_LEVEL = ERR;
module_param(VPU_LOG_LEVEL, int, 0644);
static osal_file_t fpLog  = NULL;

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
static osal_mutex_t s_log_mutex;
#endif

struct vdi_osal_file {
    struct file *filep;
    mm_segment_t old_fs;
};

int InitLog()
{
    fpLog = osal_fopen("ErrorLog.txt", "wb");
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    s_log_mutex = osal_mutex_create();
#endif
    return 1;
}

void DeInitLog()
{
    if (fpLog)
    {
        osal_fclose(fpLog);
        fpLog = NULL;
    }
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    osal_mutex_destroy(s_log_mutex);
#endif
}

void SetMaxLogLevel(int level)
{
    VPU_LOG_LEVEL = level;
}

int GetMaxLogLevel()
{
    return VPU_LOG_LEVEL;
}

void LogMsg(int level, const char *format, ...)
{
    va_list ptr;
    char    logBuf[MAX_PRINT_LENGTH] = {0};
    char*   prefix = "";
    char*   postfix= "";
    char    logMsg[MAX_PRINT_LENGTH+32] = {0};

    if (level > VPU_LOG_LEVEL)
        return;
#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    osal_mutex_lock(s_log_mutex);
#endif

    if ((log_decor & LOG_HAS_COLOR)) {
        postfix = ANSI_COLOR_RESET;
        switch (level) {
        case INFO:  prefix = ANSI_COLOR_INFO "[INFO]";  break;
        case ERR:   prefix = ANSI_COLOR_ERR "[ERROR]";   break;
        case TRACE: prefix = ANSI_COLOR_TRACE "[TRACK]"; break;
        case WARN:  prefix = ANSI_COLOR_WARN"[WARN ]";  break;
        default:    prefix = "";               break;
        }
    }

    va_start( ptr, format );
    vsnprintf( logBuf, MAX_PRINT_LENGTH, format, ptr );
    va_end(ptr);

#ifdef ANDROID
    if (level == ERR) ALOGE("%s", logBuf);
    else              ALOGI("%s", logBuf);
    fputs(logBuf, stderr);
#else
    sprintf(logMsg, "%s%s%s", prefix, logBuf, postfix);
    if (level == ERR) {
        pr_err("%s\n", logMsg);
    } else {
        printk("%s\n", logMsg);
    }
#endif

    if ((log_decor & LOG_HAS_FILE) && fpLog)
    {
        osal_fwrite(logBuf, strlen(logBuf), 1,fpLog);
        osal_fflush(fpLog);
    }

#if defined(SUPPORT_SW_UART) || defined(SUPPORT_SW_UART_V2)
    osal_mutex_unlock(s_log_mutex);
#endif
}

void osal_init_keyboard()
{

}

void osal_close_keyboard()
{

}

int osal_kbhit()
{
    return 0;
}

int osal_getch()
{
    return 0;
}

int osal_flush_ch(void)
{
    return 1;
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
    return vmalloc(size);
}

void * osal_calloc(int nmemb, int size)
{
    return vzalloc(nmemb * size);
}

void * osal_realloc(void* ptr, int size)
{
    return NULL;
}

void osal_free(void *p)
{
    if(p) {
        vfree(p);
    }
}

void *osal_kmalloc(int size)
{
    return kmalloc(size, GFP_KERNEL);
}

void osal_kfree(void *p)
{
    if (p) {
        kfree(p);
    }
}

int osal_fflush(osal_file_t fp)
{
    return 0;
}

int osal_feof(osal_file_t fp)
{
    return 0;
}

osal_file_t osal_fopen(const char * file_name, const char * mode)
{
    struct vdi_osal_file *vdi_fp = (struct vdi_osal_file *)vmalloc(sizeof(struct vdi_osal_file));

    if (!vdi_fp)
        return NULL;

    if (!strncmp(mode, "rb", 2)) {
        vdi_fp->filep = filp_open(file_name, O_RDONLY/*|O_NONBLOCK*/, 0644);
    } else if (!strncmp(mode, "wb", 2)) {
        vdi_fp->filep = filp_open(file_name, O_RDWR | O_CREAT, 0644);
    }

    if (IS_ERR(vdi_fp->filep)) {
        vfree(vdi_fp);
        return NULL;
    }

    vdi_fp->old_fs = get_fs();
    return vdi_fp;
}
size_t osal_fwrite(const void * p, int size, int count, osal_file_t fp)
{
    struct vdi_osal_file *vdi_fp = (struct vdi_osal_file *)fp;
    struct file *filep = vdi_fp->filep;
    size_t write_size;

    set_fs(KERNEL_DS);
    write_size = kernel_write(filep, p, size * count, &filep->f_pos);
    set_fs(vdi_fp->old_fs);

    return write_size;
}
size_t osal_fread(void *p, int size, int count, osal_file_t fp)
{
    struct vdi_osal_file *vdi_fp = (struct vdi_osal_file *)fp;
    struct file *filep = vdi_fp->filep;
    size_t read_size;

    set_fs(KERNEL_DS);
    read_size = kernel_read(filep, p, size * count, &filep->f_pos);
    set_fs(vdi_fp->old_fs);

    return read_size;
}

long osal_ftell(osal_file_t fp)
{
    struct vdi_osal_file *vdi_fp = (struct vdi_osal_file *)fp;
    struct file *filep = vdi_fp->filep;

    return filep->f_pos;
}

int osal_fseek(osal_file_t fp, long offset, int origin)
{
    struct vdi_osal_file *vdi_fp = (struct vdi_osal_file *)fp;
    struct file *filep = vdi_fp->filep;

    return default_llseek(filep, offset, origin);
}
int osal_fclose(osal_file_t fp)
{
    struct vdi_osal_file *vdi_fp = (struct vdi_osal_file *)fp;
    struct file *filep = vdi_fp->filep;

    if (!fp)
        return -1;

    filp_close(filep, 0);
    set_fs(vdi_fp->old_fs);
    vfree(vdi_fp);
    return 0;
}

char osal_fgetc(osal_file_t fp)
{
    return 0;
}

void osal_srand(int seed)
{

}
int osal_rand(void)
{
    return 0;
}

int osal_toupper(int c)
{
    return 0;
}

size_t osal_fputs(const char *s, osal_file_t fp)
{
    return 0;
}
int osal_fscanf(osal_file_t fp, const char * _Format, ...)
{
    return -1;
}

int osal_fprintf(osal_file_t fp, const char * _Format, ...)
{
    return -1;
}

size_t osal_strlen(const char * str)
{
    return strlen(str);
}

int osal_strcmp(const char * str1, const char * str2)
{
    return strcmp(str1, str2);
}
char *osal_strdup(const char *s)
{
    char *str = NULL;
    if ((s == NULL) || (strlen(s) == 0))
        return str;

    str = osal_calloc(1, strlen(s) + 1);
    strcpy(str, s);
    return str;
}

int osal_sprintf (char * str, const char * format, ...)
{
    int ret;
    va_list arglist;
    va_start(arglist, format);

    ret = vsprintf(str, format, arglist);

    va_end(arglist);

    return ret;
}

long long strtoi(const char *str, int minus)
{
    long long num = 0;
    int flag = minus ? -1 : 1;

    while (*str != '\0') {
        if (*str >= '0' && *str <= '9') {
            num = num * 10 + flag * (*str - '0');
            ++str;
        } else {
            break;
        }
    }

    return num;
}

int osal_atoi(const char * str)
{
    int num = 0;
    int minus = false;

    if (str != NULL && *str != '\0') {
        while (*str == ' ') {
            ++str;
            continue;
        }

        if (*str == '+') {
            ++str;
        } else if (*str == '-') {
            ++str;
            minus = true;
        }

        if (*str != '\0') {
            num = strtoi(str, minus);
        }
    }

    return num;
}

char * osal_strtok(char * str, const char * delimiters)
{
    static char *rembmberLastString;
    const char *indexDelim = delimiters;
    int flag = 1, index = 0;
    char *temp = NULL;

    if (str == NULL) {
        str = rembmberLastString;
    }

    for (; *str; str++) {
        delimiters = indexDelim;

        for (; *delimiters; delimiters++) {
            if (*str == *delimiters) {
                *str = 0;
                index = 1;
                break;
            }
        }

        if (*str != 0 && flag == 1) {
            temp = str;
            flag = 0;
        }

        if (*str != 0 && flag == 0 && index == 1) {
            rembmberLastString = str;
            return temp;
        }
    }

    rembmberLastString = str;
    return temp;
}
#define TOLOWER(x) ((x) | 0x20)
#define IS_DIGIT(x) ('0' <= (x) && (x) <= '9')
unsigned long osal_strtoul(const char *cp, char **endp, unsigned int base)
{
    unsigned long result = 0, value;

    if (!base) {
        base  = 10;
        if (*cp == '0') {
            base = 8;
            cp ++;
            if ((TOLOWER(*cp) == 'x') && IS_DIGIT(cp[1])) {
                base = 16;
                cp++;
            }
        }
    } else {
        if ((base = 16) && (cp[0] == '0') && (TOLOWER(*cp) == 'x'))
            cp += 2;
    }

    while(1) {
        value = IS_DIGIT(*cp) ? (*cp - '0') : (TOLOWER(*cp) - 'a');
        if (value >= base)
            break;

        result = result * base + value;
        cp++;
    }

    return result;
}
char * osal_strcpy(char * destination, const char * source)
{
    return strcpy(destination, source);
}

long osal_atof(const char* str)
{
    return 0;
}

char * osal_fgets(char * str, int num, osal_file_t stream)
{
    size_t read_size;
    int i;

    read_size = osal_fread(str, 1, num, stream);
    if (read_size == 0)
        return 0;

    for(i=0; i<read_size; i++) {
        if ((str[i]=='\n') ||(str[i]=='\r'))
            break;
    }
    osal_memset(str+i, 0, read_size-i);
    osal_fseek(stream, -(read_size-i-1), 1);//need skip '\n'
    return str;
}

int osal_strcasecmp(const char *s1, const char *s2)
{
    return strcasecmp(s1, s2);
}

int osal_sscanf(const char * s, const char * format, ...)
{
    int ret;
    extern int vsscanf(const char * s, const char*, va_list);

    va_list arglist;
    va_start(arglist, format);

    ret = vsscanf(s, format, arglist);

    va_end(arglist);

    return ret;
}

int osal_strncmp(const char * str1, const char * str2, size_t num)
{
    return strncmp(str1, str2, num);
}

char * osal_strstr(char * str1, const char * str2)
{
    return strstr(str1, str2);
}

int osal_fputc(int c, osal_file_t fp)
{
    return 0;
}

void osal_msleep(Uint32 millisecond)
{
    msleep(millisecond);
}

osal_thread_t osal_thread_create(int(*start_routine)(void*), void*arg)
{
    osal_thread_t   handle = NULL;
    static int i = 0;
    struct sched_param param = {
        .sched_priority = 95,
    };

    handle = kthread_run(start_routine, arg, "vcodec_thread_%d", i++);
    if (IS_ERR(handle)) {
        VLOG(ERR, "<%s:%d> Failed to kthread_create\n", __FUNCTION__, __LINE__);
        return NULL;
    }
    sched_setscheduler(handle, SCHED_RR, &param);

    return handle;  //lint !e593
}

Int32 osal_thread_join(osal_thread_t thread, void** retval)
{
    if (thread == NULL) {
        VLOG(ERR, "%s:%d invalid thread handle\n", __FUNCTION__, __LINE__);
        return 2;
    }

    kthread_stop(thread);
    thread = NULL;

    return 0;
}

Int32 osal_thread_timedjoin(osal_thread_t thread, void** retval, Uint32 millisecond)
{
    if (thread == NULL) {
        VLOG(ERR, "%s:%d invalid thread handle\n", __FUNCTION__, __LINE__);
        return 2;
    }

    kthread_stop(thread);
    thread = NULL;
    return 0;

}

osal_mutex_t osal_mutex_create(void)
{
    struct mutex *mutex = (struct mutex *)osal_malloc(sizeof(struct mutex));

    if (mutex == NULL) {
        VLOG(ERR, "<%s:%d> failed to allocate memory\n", __FUNCTION__, __LINE__);
        return NULL;
    }

    mutex_init(mutex);
    return (osal_mutex_t)mutex;
}

void osal_mutex_destroy(osal_mutex_t mutex)
{
    if (mutex == NULL) {
        VLOG(ERR, "<%s:%d> Invalid mutex handle\n", __FUNCTION__, __LINE__);
        return;
    }

    mutex_destroy(mutex);
    osal_free(mutex);
}

BOOL osal_mutex_lock(osal_mutex_t mutex)
{
    if (mutex == NULL) {
        VLOG(ERR, "<%s:%d> Invalid mutex handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    mutex_lock((struct mutex *)mutex);
    return TRUE;    //lint !e454
}

BOOL osal_mutex_unlock(osal_mutex_t mutex)
{
    if (mutex == NULL) {
        VLOG(ERR, "<%s:%d> Invalid mutex handle\n", __FUNCTION__, __LINE__);
        return FALSE;
    }

    mutex_unlock((struct mutex *)mutex);
    return TRUE;
}

Uint64 osal_gettime(void)
{
    struct timespec64 tv;

    ktime_get_ts64(&tv);

    return tv.tv_sec * 1000 + tv.tv_nsec / 1000000; // in ms
}

Int32 osal_snprintf(char* str, size_t buf_size, const char *format, ...)
{
    return snprintf(str, buf_size, format);
}


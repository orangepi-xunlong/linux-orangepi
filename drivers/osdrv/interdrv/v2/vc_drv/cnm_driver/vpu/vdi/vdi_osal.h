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

#ifndef _VDI_OSAL_H_
#define _VDI_OSAL_H_
#include "vputypes.h"

enum {NONE=0, ERR, WARN, INFO, TRACE, MAX_LOG_LEVEL};
enum
{
    LOG_HAS_DAY_NAME   =    1, /**< Include day name [default: no]           */
    LOG_HAS_YEAR       =    2, /**< Include year digit [no]              */
    LOG_HAS_MONTH      =    4, /**< Include month [no]              */
    LOG_HAS_DAY_OF_MON =    8, /**< Include day of month [no]          */
    LOG_HAS_TIME      =   16, /**< Include time [yes]              */
    LOG_HAS_MICRO_SEC  =   32, /**< Include microseconds [yes]             */
    LOG_HAS_FILE      =   64, /**< Include sender in the log [yes]           */
    LOG_HAS_NEWLINE      =  128, /**< Terminate each call with newline [yes] */
    LOG_HAS_CR      =  256, /**< Include carriage return [no]           */
    LOG_HAS_SPACE      =  512, /**< Include two spaces before log [yes]    */
    LOG_HAS_COLOR      = 1024, /**< Colorize logs [yes on win32]          */
    LOG_HAS_LEVEL_TEXT = 2048 /**< Include level text string [no]          */
};
enum {
    TERM_COLOR_R    = 2,    /**< Red            */
    TERM_COLOR_G    = 4,    /**< Green          */
    TERM_COLOR_B    = 1,    /**< Blue.          */
    TERM_COLOR_BRIGHT = 8    /**< Bright mask.   */
};

#define MAX_PRINT_LENGTH 512

#ifdef ANDROID
#include <utils/Log.h>
#undef LOG_NDEBUG
#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "VPUAPI"
#endif

#define VLOG(dbg_lv, format, ...)                   LogMsg(dbg_lv, "[%s:%d]"format, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define VLOG_EX(dbg_lv, format, ...)        LogMsg(dbg_lv, "[%s:%d]"format, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define LOG_ENABLE_FILE	SetLogDecor(GetLogDecor()|LOG_HAS_FILE);

typedef void * osal_file_t;
# ifndef SEEK_SET
# define	SEEK_SET	0
# endif

# ifndef SEEK_CUR
# define	SEEK_CUR	1
# endif

# ifndef SEEK_END
# define	SEEK_END	2
# endif

#if defined(_WIN32) || defined(__WIN32__) || defined(_WIN64) || defined(WIN32) || defined(__MINGW32__)
#elif defined(linux) || defined(__linux) || defined(ANDROID)
#else

#ifndef stdout
# define	stdout	(void * )1
#endif
#ifndef stderr
# define	stderr	(void * )1
#endif

#endif

typedef void*   osal_thread_t;
typedef void*   osal_mutex_t;

#if defined (__cplusplus)
extern "C" {
#endif

int InitLog(void);
void DeInitLog(void);

void SetMaxLogLevel(int level);
int GetMaxLogLevel(void);

//log print
void LogMsg(int level, const char *format, ...);

//terminal
void osal_init_keyboard(void);
void osal_close_keyboard(void);

//memory
void * osal_memcpy (void *dst, const void * src, int count);
void * osal_memset (void *dst, int val,int count);
int    osal_memcmp(const void* src, const void* dst, int size);
void * osal_malloc(int size);
void * osal_calloc(int nmemb, int size);
void * osal_realloc(void* ptr, int size);
void osal_free(void *p);
void *osal_kmalloc(int size);
void osal_kfree(void *p);

osal_file_t osal_fopen(const char * osal_file_tname, const char * mode);
size_t osal_fwrite(const void * p, int size, int count, osal_file_t fp);
size_t osal_fread(void *p, int size, int count, osal_file_t fp);
long osal_ftell(osal_file_t fp);
int osal_fseek(osal_file_t fp, long offset, int origin);
int osal_fclose(osal_file_t fp);
int osal_fflush(osal_file_t fp);
int osal_fprintf(osal_file_t fp, const char * _Format, ...);
int osal_fscanf(osal_file_t fp, const char * _Format, ...);
int osal_kbhit(void);
int osal_getch(void);
int osal_flush_ch(void);
int osal_feof(osal_file_t fp);
char osal_fgetc(osal_file_t fp);
void osal_srand(int seed);
int osal_rand(void);
int osal_toupper(int c);
size_t osal_fputs(const char *s, osal_file_t fp);

size_t osal_strlen(const char * str);
int osal_strcmp(const char * str1, const char * str2);
int osal_sprintf(char * str, const char * format, ...);
int osal_atoi(const char * str);
char * osal_strtok(char * str, const char * delimiters);
unsigned long osal_strtoul(const char *cp, char **endp, unsigned int base);
char * osal_strcpy(char * destination, const char * source);
long osal_atof(const char* str);
char * osal_fgets(char * str, int num, osal_file_t stream);
int osal_strcasecmp(const char *s1, const char *s2);
int osal_sscanf(const char * s, const char * format, ...);
int osal_strncmp(const char * str1, const char * str2, size_t num);
char * osal_strstr(char * str1, const char * str2);
int osal_fputc(int c, osal_file_t fp);
char * osal_strdup(const char * str);
void osal_msleep(Uint32 millisecond);
int osal_snprintf(char* str, size_t buf_size, const char *format, ...);

/********************************************************************************
 * THREAD                                                                       *
 ********************************************************************************/
osal_thread_t osal_thread_create(int(*start_routine)(void*), void*arg);

/* @return  0 - success
            2 - failure
 */
Int32 osal_thread_join(osal_thread_t thread, void** retval);
/* @return  0 - success
            1 - timed out
            2 - failure
 */
Int32 osal_thread_timedjoin(osal_thread_t thread, void** retval, Uint32 millisecond);

/********************************************************************************
 * MUTEX                                                                        *
 ********************************************************************************/
osal_mutex_t osal_mutex_create(void);
void osal_mutex_destroy(osal_mutex_t mutex);
BOOL osal_mutex_lock(osal_mutex_t mutex);
BOOL osal_mutex_unlock(osal_mutex_t mutex);

/********************************************************************************
 * SEMAPHORE                                                                    *
 ********************************************************************************/
typedef void* osal_sem_t;

/* @param   count   The number of semaphores.
 */
osal_sem_t osal_sem_init(Uint32 count);
void osal_sem_wait(osal_sem_t sem);
void osal_sem_post(osal_sem_t sem);
void osal_sem_destroy(osal_sem_t sem);

/********************************************************************************
 * SYSTEM TIME                                                                  *
 ********************************************************************************/
/* @brief   It returns system time in millisecond.
 */
Uint64 osal_gettime(void);

osal_file_t osal_fopen(const char *file_name, const char *mode);
size_t osal_fwrite(const void *p, int size, int count, osal_file_t fp);
size_t osal_fread(void *p, int size, int count, osal_file_t fp);
long osal_ftell(osal_file_t fp);
int osal_fseek(osal_file_t fp, long offset, int origin);
int osal_fclose(osal_file_t fp);

#if defined (__cplusplus)
}
#endif

#endif //#ifndef _VDI_OSAL_H_


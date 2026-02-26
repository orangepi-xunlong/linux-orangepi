/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */

#ifndef USERKERNELADAPTER_h
#define USERKERNELADAPTER_h

#ifdef CONFIG_AW_VIDEO_KERNEL_ENC
//#include "ve_interface.h"
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/kthread.h>

// #include "sc_interface.h"
#include "cedar_ve.h"

int kernel_write_file(unsigned char *data0, int size0, unsigned char *data1, int size1, char *file_name);
int kernel_mutex_init(struct mutex *p_mutex);

#define PRINTF printk

#define MUTEX_STRUCT struct mutex
#define SEM_STRUCT struct semaphore
#define THREAD_STRUCT struct task_struct*
#define MALLOC(size) kmalloc(size, GFP_KERNEL | __GFP_ZERO)
#define MALLOC_BIG(size) vmalloc(size)
#define CALLOC(num, size)  kcalloc(num, size, GFP_KERNEL)
#if 1
#define FREE(poiter) do {\
	if (poiter) {\
		kfree(poiter);\
		poiter = NULL;\
	} \
} while (0)

#define FREE_BIG(poiter) do {\
	if (poiter) {\
		vfree(poiter);\
		poiter = NULL;\
	} \
} while (0)
#else
#define FREE(poiter) do {\
    if (poiter) {\
		logd("poiter 0x%px", poiter);\
		kfree(poiter);\
	poiter = NULL;\
    } \
} while (0)

#define FREE_BIG(poiter) do {\
    if (poiter) {\
		vfree(poiter);\
		poiter = NULL;\
    } \
} while (0)
#endif
#define THREAD_CREATE(p_thread, attr, func, arg, name) do { \
	p_thread = kthread_run(func, (void *)arg, name); \
} while (0)

#define THREAD_JOIN(thread, pp_ret) kthread_stop(thread)

#define DEF_STATIC_MUTEX(Mutex) DEFINE_MUTEX(Mutex)
#define MUTEX_INIT(p_mutex, p_mutex_attr)  kernel_mutex_init(p_mutex)
#define MUTEX_LOCK(p_mutex) mutex_lock(p_mutex)
#define MUTEX_UNLOCK(p_mutex) mutex_unlock(p_mutex)
#define MUTEX_DESTROY(p_mutex) mutex_destroy(p_mutex)

#define SEM_INIT(p_sem, shared, value) sema_init(p_sem, value)
#define SEM_POST(p_sem) up(p_sem)
#define SEM_WAIT(p_sem) down(p_sem)
#define SEM_TRYWAIT(p_sem) ((void)down_trylock(p_sem))
#define SEM_GETVALUME(p_sem, cnt) ((*cnt) = (p_sem)->count)
#define SEM_DESTROY(p_sem)

#define MEMOPS_STRUCT struct mem_interface
// #define MEMOPS_STRUCT struct ScMemOpsS
#define VEOPS_STRUCT struct ve_interface

#define FILE_STRUCT struct file
#define WRITE_FILE(data0, size0, data1, size1, file_name) kernel_write_file(data0, size0, data1, size1, file_name)
#define WRITE_FILE_FD(data0, size0, data1, size1, fd)

#define POPCOUNT(p_addr) popcount(p_addr)
#define USLEEP(us) udelay(us)

#define ATOI(str) simple_strtol(str, NULL, 10)

#ifndef VENC_SUPPORT_EXT_PARAM
#define VENC_SUPPORT_EXT_PARAM 0

typedef void *devfd_t;
typedef void *ionfd_t;

#define IOCTL(fd, cmd, arg) rt_cedardev_ioctl(fd, cmd, arg)
#endif

#else//not CONFIG_AW_VIDEO_KERNEL_ENC
#include <pthread.h>
#include <stdio.h>

#define PRINTF printf
#define MUTEX_STRUCT pthread_mutex_t
#define SEM_STRUCT sem_t
#define THREAD_STRUCT pthread_t
#define MALLOC(size) malloc(size)
#define MALLOC_BIG(size) malloc(size)
#define CALLOC(num, size)  calloc(num, size)
#define FREE(poiter) do {\
    if (poiter) {\
		free(poiter);\
		poiter = NULL;\
    } \
} while (0)

#define FREE_BIG(poiter) do {\
    if (poiter) {\
		free(poiter);\
		poiter = NULL;\
    } \
} while (0)

#define THREAD_CREATE(thread, attr, func, arg, name) pthread_create(&thread, attr, func, (void *)arg)
#define THREAD_JOIN(thread, pp_ret) pthread_join(thread, pp_ret)

#define DEF_STATIC_MUTEX(Mutex) static pthread_mutex_t Mutex = (PTHREAD_MUTEX_INITIALIZER)
#define MUTEX_INIT(p_mutex, p_mutex_attr)  pthread_mutex_init(p_mutex, p_mutex_attr)
#define MUTEX_LOCK(p_mutex) pthread_mutex_lock(p_mutex)
#define MUTEX_UNLOCK(p_mutex) pthread_mutex_unlock(p_mutex)

#define MUTEX_DESTROY(p_mutex) pthread_mutex_destroy(p_mutex)

#define SEM_INIT(p_sem, shared, value) sem_init(p_sem, shared, value)
#define SEM_POST(p_sem) sem_post(p_sem)
#define SEM_WAIT(p_sem) sem_wait(p_sem)
#define SEM_TRYWAIT(p_sem) sem_trywait(p_sem)
#define SEM_GETVALUME(p_sem, cnt) sem_getvalue(p_sem, cnt)
#define SEM_DESTROY(p_sem) sem_destroy(p_sem)

#define MEMOPS_STRUCT struct ScMemOpsS
#define VEOPS_STRUCT VeOpsS

#define FILE_STRUCT FILE
#define WRITE_FILE(data0, size0, data1, size1, file_name) user_write_file(data0, size0, data1, size1, file_name)
#define WRITE_FILE_FD(data0, size0, data1, size1, fd) user_write_file_fd(data0, size0, data1, size1, fd)

#define POPCOUNT(p_addr) __builtin_popcount(p_addr)
#define USLEEP(us) usleep(us)

#define ATOI(str) atoi(str)

int user_write_file(unsigned char *data0, int size0, unsigned char *data1, int size1, char *file_name);
int user_write_file_fd(unsigned char *data0, int size0, unsigned char *data1, int size1, FILE_STRUCT *fp);

typedef int devfd_t;
typedef int ionfd_t;

#define IOCTL(fd, cmd, arg) ioctl(fd, cmd, arg)
#endif//CONFIG_AW_VIDEO_KERNEL_ENC

/*
#define ALIGN_XXB(y, x) (((x) + ((y)-1)) & ~((y)-1))
typedef enum eVeLbcMode
{
    LBC_MODE_DISABLE  = 0,
    LBC_MODE_1_5X     = 1,
    LBC_MODE_2_0X     = 2,
    LBC_MODE_2_5X     = 3,
    LBC_MODE_NO_LOSSY = 4,
    LBC_MODE_1_0X     = 5,
} eVeLbcMode;

typedef struct ve_lbc_in_param
{
    eVeLbcMode   eLbcMode;
    unsigned int nWidht;
    unsigned int nHeight;
} ve_lbc_in_param;

typedef struct ve_lbc_out_param
{
    unsigned int header_size;
    unsigned int buffer_size;
    unsigned int seg_tar_bits;
    unsigned int segline_tar_bits;
    unsigned int is_lossy;
    unsigned int seg_rc_en;
} ve_lbc_out_param;

typedef enum MEMORY_TYPE {
    MEMORY_NORMAL,
    MEMORY_IOMMU,
}MEMORY_TYPE;


int ComputeLbcParameter(ve_lbc_in_param *in_param, ve_lbc_out_param *out_param);
*/
int get_random_number(void);

#endif//USERKERNELADAPTER_h

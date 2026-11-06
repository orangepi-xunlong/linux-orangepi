#ifndef __DRV_FILE_H__
#define __DRV_FILE_H__
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/vmalloc.h>
#include <linux/io.h>


typedef struct {
    struct file *filep;
    mm_segment_t old_fs;
}drv_file_t;

typedef void * driver_file_t;

driver_file_t drv_fopen(const char *osal_file_tname, const char *mode);
size_t drv_fwrite(const void *p, int size, int count, driver_file_t fp);
size_t drv_fread(void *p, int size, int count, driver_file_t fp);
long drv_ftell(driver_file_t fp);
int drv_fseek(driver_file_t fp, long offset, int origin);
int drv_fclose(driver_file_t fp);



#endif //__DRV_FILE_H__

/*
 * Sunxi Oops/Panic logger
 *
 * Copyright (C) 2014 Allwinnertech Ltd.. All rights reserved.
 *
 * Author: Heming <lvheming@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/kmsg_dump.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/kmsg_dump.h>

#define LOG_BUF_LEN	(1 << CONFIG_LOG_BUF_SHIFT)
#define BUF_LEN (LOG_BUF_LEN + 128)
#define OOPS_MAX_COUNT (20)
#define OOPS_INFO_ADDR (-20)    //the abs must lager than sizeof(struct oops_info)
#define OOPS_LOG_PARTITION "/dev/block/by-name/klog"

static void sunxi_oops_dump(void);

#define _USE_KMSG_DUMP_
#ifdef _USE_KMSG_DUMP_

struct kmsg_dumper sunxi_dumper;

void sunxi_do_dump(struct kmsg_dumper *dumper, enum kmsg_dump_reason reason,
		const char *s1, unsigned long l1,
		const char *s2, unsigned long l2)
{
	if (reason != KMSG_DUMP_OOPS &&
		reason != KMSG_DUMP_PANIC)
		return;

	sunxi_oops_dump();
}


void sunxi_dumper_register(void)
{
	int err;

	sunxi_dumper.dump = sunxi_do_dump;
	err = kmsg_dump_register(&sunxi_dumper);
	if (err) {
		printk(KERN_ERR "sunxi_oops: registering kmsg dumper failed, error %d\n", err);
	}
}

void sunxi_dumper_unregister(void)
{
	if (kmsg_dump_unregister(&sunxi_dumper) < 0)
		printk(KERN_ERR "sunxi_oops: could not unregister kmsg_dumper\n");
}


#endif

struct oops_info {
        int write_pos;
        int oops_count;
};

static bool haspanic = false;
static char buf[BUF_LEN];
static struct work_struct work_read;
static struct platform_device sunxi_oops = {
        .name	= "sunxi_oops",
        .id	= -1,
};
static struct oops_info sunxi_oops_info = {
        .write_pos = OOPS_MAX_COUNT,
        .oops_count = OOPS_MAX_COUNT,
};

extern int log_buf_copy(char *dest, int idx, int len);

static void sunxi_oops_init_oops_info(void)
{
        struct file* filp = NULL;
        mm_segment_t oldfs;

        if(sunxi_oops_info.write_pos == OOPS_MAX_COUNT){
                filp = filp_open(OOPS_LOG_PARTITION, O_RDONLY, 0);
                if(IS_ERR_OR_NULL(filp)){
                        pr_err("can not open the oops log partition!\n");
                        return;
                }
                oldfs = get_fs();
                set_fs(get_ds());
                vfs_llseek(filp,(loff_t)OOPS_INFO_ADDR,SEEK_END);
                vfs_read(filp, (char *)&sunxi_oops_info, sizeof(sunxi_oops_info), &filp->f_pos);
                set_fs(oldfs); 
                filp_close(filp, NULL);
                if(sunxi_oops_info.write_pos == 0xFFFFFFFF)   //default val,never oops
                        sunxi_oops_info.write_pos = 0;
                if(sunxi_oops_info.oops_count == 0xFFFFFFFF)    //default val,never oops
                        sunxi_oops_info.oops_count = 0;
        }
        //pr_info("write_pos= %d, oops_count =%d\n",sunxi_oops_info.write_pos,sunxi_oops_info.oops_count);
        return;
}

static void sunxi_oops_save_oops_info(struct oops_info *info)
{
        struct file* filp = NULL;
        mm_segment_t oldfs;
        
        filp = filp_open(OOPS_LOG_PARTITION, O_RDWR, 0);
        if(IS_ERR_OR_NULL(filp)){
                pr_err("can not open the oops log partition!\n");
                return;
        }
        oldfs = get_fs();
        set_fs(get_ds());
        vfs_llseek(filp,(loff_t)OOPS_INFO_ADDR,SEEK_END);
        vfs_write(filp, (const char*)info, sizeof(*info), &filp->f_pos);
        vfs_fsync(filp,0);
        set_fs(oldfs); 
        filp_close(filp, NULL);
        //pr_info("set write_pos= %d, oops_count =%d \n",info->write_pos,info->oops_count);
        return;
}


static void sunxi_oops_workfunc_read(struct work_struct *work)
{
        struct file* filp = NULL;
        char *last_oops_buf = NULL , *buf_pos = NULL ;
        char oops_log_file [96];
        ssize_t len;
        mm_segment_t oldfs;
        struct timeval timestamp;
        struct tm log_tm;

        sunxi_oops_init_oops_info();
        if(sunxi_oops_info.oops_count == 0){
                pr_info("It's healthy, never panic or get panic info error.\n");
                return;
        }
        last_oops_buf = vzalloc(BUF_LEN * OOPS_MAX_COUNT);
        if(IS_ERR_OR_NULL(last_oops_buf)){
                pr_err("can not get enough memory!\n");
                goto error;
        }
        buf_pos = last_oops_buf;
        //read oops log
        filp = filp_open(OOPS_LOG_PARTITION, O_RDONLY, 0);
        if(IS_ERR_OR_NULL(filp)){
                pr_err("can not open the oops log partition!\n");
                goto error;
        }
        oldfs = get_fs();
        set_fs(get_ds());
        if(sunxi_oops_info.oops_count <= OOPS_MAX_COUNT){
                vfs_llseek(filp,0,SEEK_SET);
                vfs_read(filp, buf_pos, BUF_LEN * sunxi_oops_info.oops_count, &filp->f_pos);
        }else if(sunxi_oops_info.write_pos == 0){
                vfs_llseek(filp,0,SEEK_SET);
                len = vfs_read(filp, buf_pos, BUF_LEN * OOPS_MAX_COUNT, &filp->f_pos);
        }
        else{
                vfs_llseek(filp,BUF_LEN * sunxi_oops_info.write_pos,SEEK_SET);
                len = vfs_read(filp, buf_pos, BUF_LEN * (OOPS_MAX_COUNT - sunxi_oops_info.write_pos), &filp->f_pos);
                buf_pos += len;
                vfs_llseek(filp,0,SEEK_SET);
                len = vfs_read(filp, buf_pos, BUF_LEN * sunxi_oops_info.write_pos, &filp->f_pos);
        }
        filp_close(filp, NULL);
        //write oops log to sdcard
        do_gettimeofday(&timestamp);
        time_to_tm(timestamp.tv_sec,0,&log_tm);
        sprintf(oops_log_file, "/mnt/sdcard/Logger/oops_log_%ld-%d-%d_%d-%d-%d.txt", log_tm.tm_year + 1900, log_tm.tm_mon + 1, 
                log_tm.tm_mday,log_tm.tm_hour, log_tm.tm_min, log_tm.tm_sec);
        filp = filp_open(oops_log_file, O_RDWR | O_CREAT, S_IRUGO);
        if(IS_ERR_OR_NULL(filp)){
                pr_err("can not creat the oops log file!path=%s\n",oops_log_file);
                goto error;
        }
        if(sunxi_oops_info.oops_count <= OOPS_MAX_COUNT)
                vfs_write(filp, last_oops_buf, BUF_LEN * sunxi_oops_info.oops_count , &filp->f_pos);
        else
                vfs_write(filp, last_oops_buf, BUF_LEN * OOPS_MAX_COUNT , &filp->f_pos);
        vfs_fsync(filp,0);
        filp_close(filp, NULL);
        //erase oops log partition and reset count
        pr_info("erase the oops log partition.\n");
        filp = filp_open(OOPS_LOG_PARTITION, O_RDWR, 0);
        if(IS_ERR_OR_NULL(filp)){
                pr_err("can not open the oops log partition!\n");
                goto error;
        }
        memset(last_oops_buf,0xff,BUF_LEN * OOPS_MAX_COUNT);
        vfs_llseek(filp,0,SEEK_SET);
        vfs_write(filp, last_oops_buf, BUF_LEN * OOPS_MAX_COUNT, &filp->f_pos);
        vfs_fsync(filp,0);
        set_fs(oldfs); 
        filp_close(filp, NULL);
        sunxi_oops_info.oops_count = 0;
        sunxi_oops_info.write_pos = 0;
        sunxi_oops_save_oops_info(&sunxi_oops_info);
error:
        if(!IS_ERR_OR_NULL(last_oops_buf)){
                vfree(last_oops_buf);
        }
}

static ssize_t oops_log_show(struct device *dev, struct device_attribute *attr,char *buf)
{
        if(!work_busy(&work_read))
                schedule_work(&work_read);
        return sprintf(buf,"0");
}

static DEVICE_ATTR(oops_log, S_IRUGO, oops_log_show, NULL);

static void sunxi_oops_dump(void)
{
        struct file* filp = NULL;
        int len = 0;
        char *buf_pos = buf;
        mm_segment_t oldfs;
        struct timeval timestamp;
        struct tm oops_tm;

        if(haspanic)    //do not save the other cpu core stack info except the first one
                return;
        haspanic = true;
        sunxi_oops_init_oops_info();
        memset(buf,0xff,BUF_LEN);
        do_gettimeofday(&timestamp);
        time_to_tm(timestamp.tv_sec,0,&oops_tm);
        //the head tag len should less than 64
        len = sprintf(buf_pos, "\n=== UTC %ld-%d-%d %d:%d:%d count=%u start ===\n", oops_tm.tm_year + 1900, oops_tm.tm_mon + 1, 
                oops_tm.tm_mday,oops_tm.tm_hour, oops_tm.tm_min, oops_tm.tm_sec,sunxi_oops_info.oops_count);
        buf_pos += len;
        len = log_buf_copy(buf_pos, 0, LOG_BUF_LEN);
        if(len != -1)
                buf_pos += len;
        else{
                pr_err("can not get the oops log!\n");
                return;
        }

        //the end tag len should less than 64
        len = sprintf(buf_pos, "\n=== UTC %ld-%d-%d %d:%d:%d count=%u end ===\n", oops_tm.tm_year + 1900, oops_tm.tm_mon + 1, 
                oops_tm.tm_mday,oops_tm.tm_hour, oops_tm.tm_min, oops_tm.tm_sec,sunxi_oops_info.oops_count);
        buf_pos += len;
        //write oops log
        filp = filp_open(OOPS_LOG_PARTITION, O_RDWR, 0);
        if(IS_ERR_OR_NULL(filp)){
                pr_err("can not open the oops log partition!\n");
                return;
        }
        oldfs = get_fs();
        set_fs(get_ds());
        vfs_llseek(filp,(loff_t)BUF_LEN*sunxi_oops_info.write_pos,SEEK_SET);
        len = vfs_write(filp, buf, BUF_LEN, &filp->f_pos);
        //pr_info("kmsg buf wirte length = %d \n",len);
        set_fs(oldfs); 
        filp_close(filp, NULL);
        if(++sunxi_oops_info.write_pos == OOPS_MAX_COUNT)
                sunxi_oops_info.write_pos = 0;
        sunxi_oops_info.oops_count++;
        sunxi_oops_save_oops_info(&sunxi_oops_info);
}

static int __init sunxi_oops_init(void)
{
        int ret;

        ret = platform_device_register(&sunxi_oops);
        if (ret)
                return ret;
        INIT_WORK(&work_read, sunxi_oops_workfunc_read);
        pr_err("heming add OOPS_INFO_ADDR = %d\n",OOPS_INFO_ADDR);
        ret = device_create_file(&sunxi_oops.dev, &dev_attr_oops_log);

		sunxi_dumper_register();

        return ret;
}

static void __exit sunxi_oops_exit(void)
{
		sunxi_dumper_unregister();
        cancel_work_sync(&work_read);
        device_remove_file(&sunxi_oops.dev,&dev_attr_oops_log);
        platform_device_unregister(&sunxi_oops);
}

module_init(sunxi_oops_init);
module_exit(sunxi_oops_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Heming <lvheming@allwinnertech.com>");
MODULE_DESCRIPTION("Sunxi Oops/Panic console logger/driver");


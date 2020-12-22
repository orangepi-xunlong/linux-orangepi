/* secure storage driver for sunxi platform 
 *
 * Copyright (C) 2014 Allwinner Ltd. 
 *
 * Author:
 *	Ryan Chen <ryanchen@allwinnertech.com>
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
 
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/kthread.h>
#include <linux/smp.h>
#include <linux/crc32.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

char *path="/dev/block/by-name/bootloader";

#define SECBLK_READ							_IO('V',20)
#define SECBLK_WRITE						_IO('V',21)
#define SECBLK_IOCTL						_IO('V',22)

struct secblc_op_t{
	int item;
	unsigned char *buf;
	unsigned int len ;
};

static struct secblc_op_t secblk_op ;
static int _sst_user_init(char *filename )
{
    struct file *fd;
    mm_segment_t old_fs ;
    int ret ;

	if(!filename){
		printk("- filename NULL\n");
		return (-EINVAL);
	}

	printk(": file %s\n" , filename);
    old_fs = get_fs();
    set_fs(KERNEL_DS);
    
    fd = filp_open(filename, O_RDONLY, 0);
    
    if(IS_ERR(fd)){
			printk(" -file open fail\n");
        return -1;
    }   
		if( (ret = fd->f_op->unlocked_ioctl(fd,SECBLK_IOCTL,NULL) )<0){
			printk(" -file ioctl access fail\n");
			return -1;
		}
	
	filp_close(fd, NULL);
    
  set_fs(old_fs);
	printk(": file %s init done \n",filename);
	return (ret) ;
}

static int _sst_user_read(
		char *filename, 
		char *buf, 
		ssize_t len,
		int offset
		)
{	
	struct file *fd;
	//ssize_t ret;
	int retLen = -1;
	mm_segment_t old_fs ;

	if(!filename || !buf){
		printk("- filename/buf NULL\n");
		return (-EINVAL);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = filp_open(filename, O_RDONLY, 0);

	if(IS_ERR(fd)) {
		printk(" -file open fail\n");
		return -1;
	}
	do{
		if ((fd->f_op == NULL) || (fd->f_op->unlocked_ioctl == NULL))
		{
			printk(" -file can't to open!!\n");
			break;
		} 	
		secblk_op.item = offset ;
		secblk_op.buf = buf;
		secblk_op.len = len ;
		
		retLen = fd->f_op->unlocked_ioctl(
				fd,
				SECBLK_READ,
				&secblk_op);			

	}while(false);

	filp_close(fd, NULL);

	set_fs(old_fs);

	return retLen;
}

static int _sst_user_write(char *filename, char *buf, ssize_t len, int offset)
{	
	struct file *fd;
	//ssize_t ret;
	int retLen = -1;
	mm_segment_t old_fs = get_fs();

	if(!filename || !buf){
		printk("- filename/buf NULL\n");
		return (-EINVAL);
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	fd = filp_open(filename, O_WRONLY|O_CREAT, 0666);

	if(IS_ERR(fd)) {
		printk(" -file open fail\n");
		return -1;
	}
	do{
		if ((fd->f_op == NULL) || (fd->f_op->unlocked_ioctl == NULL))
		{
			printk(" -file can't to write!!\n");
			break;
		} 

		secblk_op.item = offset ;
		secblk_op.buf = buf;
		secblk_op.len = len ;
		
		retLen = fd->f_op->unlocked_ioctl(
				fd,
				SECBLK_WRITE,
				&secblk_op);			

	}while(false);

	vfs_fsync(fd, 0);

	filp_close(fd, NULL);

	set_fs(old_fs);

	return retLen;
}
static int sst_daemon_kthread(void *data)
{
	msleep(16000);
	ssize_t len = 4096;
	u8 *src = kmalloc(len,GFP_KERNEL);
	u8 *dst = kmalloc(len,GFP_KERNEL);
	if(!src || !dst){
		printk("out of memroy\n");
		return - ENOMEM ;
	}
	
	int ret ;
	if( (ret =_sst_user_init(path)) <0){
		kfree(src);
		kfree(dst);
		printk("open fail\n");
		return -1 ;
	}
	memset(src,0x12,len);
	memset(dst,0x34,len);
	if( (ret = _sst_user_read(path, dst, len , 1)) <0){
		printk("read fail\n");
		kfree(src);
		kfree(dst);
		return -1 ;
	}else
		printk("read pass\n");
		
			print_hex_dump_bytes( "dst:", DUMP_PREFIX_OFFSET,
				src, len);
				
				
	memset(src,0x12,len);
	memset(dst,0x34,len);	
	if( (ret = _sst_user_write(path, src, len , 1)) <0){
		printk("write fail\n");
		kfree(src);
		kfree(dst);
		return -1 ;
	}else
		printk("write pass\n");	
		
	if( (ret = _sst_user_read(path, dst, len , 1)) <0){
		printk("read fail\n");
		kfree(src);
		kfree(dst);
		return -1 ;
	}else
		printk("read pass\n");
		
	if(memcmp(src,dst, len) !=0){
		printk("cmp fail\n");
		print_hex_dump_bytes( "src:", DUMP_PREFIX_OFFSET,
				src, len);
		print_hex_dump_bytes( "dst:", DUMP_PREFIX_OFFSET,
				dst, len);
		kfree(src);
		kfree(dst);
		return -1;
	}
	printk("cmp pass!!!\n");
	kfree(src);
	kfree(dst);
	
}

static int __init aw_sst_init(void)
{
	int ret = -1;
	struct task_struct *tsk;
#if 1
	tsk = kthread_create(sst_daemon_kthread, NULL, "sst_daemon"); 
	if(!IS_ERR(tsk)){
		//kthread_bind(tsk, 0);
		wake_up_process(tsk);
	}else{
		printk("Tsk request fail!\n");
		ret = (- EINVAL);
		return ret;
	}
#endif
	printk("aw_sst init test \n");
	return ret ;
}

static void __exit aw_sst_exit(void)
{
	
}

module_init(aw_sst_init);
module_exit(aw_sst_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ryan chen<ryanchen@allsoftwinnertech.com>");
MODULE_DESCRIPTION("secure storage daemon for normal world operation");



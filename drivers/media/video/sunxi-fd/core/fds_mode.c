#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
//#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <asm/io.h>

#include "fd_priv.h"
#include "fd_lib.h"
#include "sun_fd.h"
#include "fds_mode.h"

static struct fd_dev *fd_devp;
static short fds_exit_flag = 0;
//EXPORT_SYMBOL(fds_exit_flag);
static DECLARE_WAIT_QUEUE_HEAD(wait_fds);
static spinlock_t fds_spin_lock;

static struct workqueue_struct *p_fds_queue;
//EXPORT_SYMBOL(p_fds_queue);
static struct work_struct fds_work;
//EXPORT_SYMBOL(fds_work);

extern void reset_modules(volatile void __iomem* regs_modules);

long fds_get_rtl(struct fds_dev* ptr_fds_dev,void __user* ptr_user_rtl_buf,int frame_num)
{
	 spin_lock(&fds_spin_lock);
	 if(ptr_fds_dev->fds_dev_status == 0)
	 {
	    fd_rtl det_rtl;

	    fds_get_rtl_sl((&ptr_fds_dev->dev_info),(*ptr_fds_dev->ptr_iomap_addrs),(*ptr_fds_dev->ptr_mem_addr),(unsigned char*)(&det_rtl),frame_num);

	    if(copy_to_user(ptr_user_rtl_buf,&det_rtl,sizeof(fd_rtl)))
		  {
		  		spin_unlock(&fds_spin_lock);
	    		printk("base_get_rtl copy_to_user fail\n");
		   		return -EFAULT;
	    }
	 }
	 spin_unlock(&fds_spin_lock);
	 return 0;
}

int fds_run(struct fds_dev* ptr_fds_dev,void __user* arg)
{
	unsigned long flags;
	
	if(copy_from_user((void *)(&ptr_fds_dev->dev_info),(const void __user *)arg,sizeof(fds_dev_info)))
	{
		printk("warning:fds_run copy_from_user fault");
		return -EINVAL;
	}
	
	cancel_work_sync(&fds_work);
	spin_lock_irqsave(&fds_spin_lock, flags);
	fds_exit_flag = 0;
	fd_devp->phy_mem_addr.phy_mem_src_img_addr = (ptr_fds_dev->dev_info.image_buf_phyaddr - 0x20000000) >> 2;
	set_src_img_phy_addr_sl(fd_devp->iomap_addrs,fd_devp->phy_mem_addr);
	queue_work(p_fds_queue,&fds_work);
	spin_unlock_irqrestore(&fds_spin_lock, flags);

	return 0;
}    
   
int fds_get_status(struct fds_dev* ptr_fds_dev)
{
	return fds_get_status_sl((*ptr_fds_dev->ptr_iomap_addrs));
}

long fds_load_ftr(struct fds_dev* ptr_fds_dev,void __user* ptr_ld_file,int size)
{
	if(copy_from_user(ptr_fds_dev->ptr_mem_addr->mem_ld0_addr,ptr_ld_file,size)){
		printk("base_load_ftr copy_from_user fail\n");
		return -EFAULT;
	}
	if(copy_from_user(ptr_fds_dev->ptr_mem_addr->mem_ld1_addr,ptr_ld_file + FTR_FILE_SIZE,size)){
		printk("base_load_ftr copy_from_user fail\n");
		return -EFAULT;
	}
	if(copy_from_user(ptr_fds_dev->ptr_mem_addr->mem_ld2_addr,ptr_ld_file + 2*FTR_FILE_SIZE,size)){
		printk("base_load_ftr copy_from_user fail\n");
		return -EFAULT;
	}

	return 0;
}

long fds_load_ftr_cfg(struct fds_dev* ptr_fds_dev,void __user* ptr_cfg_file,int size)
{
	if(copy_from_user(ptr_fds_dev->ptr_mem_addr->mem_ftr_cfg_addr,ptr_cfg_file,size)){
		printk("base_load_ftr copy_from_user fail\n");
		return -EFAULT;
	}
	return 0;
}

static void fds_work_handle(struct work_struct *work)
{
	struct fds_dev* ptr_fds_dev = fd_devp->ptr_fds_dev;
	unsigned long flags;
//	printk("-----------fds_work_handle: 0\n");
	spin_lock_irqsave(&fds_spin_lock, flags);
	if(fds_exit_flag == 0)
	{  
	  if((fds_exit_flag == 0)&&(ptr_fds_dev->fds_dev_status == 0))
	  {
	  	 ptr_fds_dev->fds_dev_status = 1;
	  	 ptr_fds_dev->fds_dev_step = 1;
//	  	 printk("-----------fds_work_handle: 1 fds_run_sl\n");
		   fds_run_sl((*ptr_fds_dev->ptr_iomap_addrs),(*ptr_fds_dev->ptr_mem_addr),&ptr_fds_dev->dev_info);
		}
	}
	else
		msleep(1);
  spin_unlock_irqrestore(&fds_spin_lock, flags);
}	

int fds_interrupt_process(struct fds_dev* ptr_fds_dev)
{
	 //long flags;
	 if(fds_exit_flag == 1)
	 	return 1;
	 	
	 //spin_lock(&fds_spin_lock);
	 ptr_fds_dev->fds_dev_status = 0;
	 //spin_unlock(&fds_spin_lock);
	 return 1;
}

int fds_get_run_status(struct fds_dev* ptr_fds_dev)
{
	 int status;
	 unsigned long flags;
	 spin_lock_irqsave(&fds_spin_lock, flags);
	 status =  ptr_fds_dev->fds_dev_status;
	 spin_unlock_irqrestore(&fds_spin_lock, flags);
	 return status;
}

int fds_dev_release(struct fds_dev* ptr_fds_dev)
{
	//unsigned long flags;
	spin_lock(&fds_spin_lock);
	if(ptr_fds_dev->fds_dev_status)
	{	
		reset_modules(ptr_fds_dev->ptr_iomap_addrs->regs_fd);
		ptr_fds_dev->fds_dev_status = 0;
	}
	fds_exit_flag = 1;
	spin_unlock(&fds_spin_lock);
	cancel_work_sync(&fds_work);
	if(p_fds_queue)
	{
     destroy_workqueue(p_fds_queue);
     p_fds_queue = NULL;
  }
	return 0;
}

int fds_dev_exit(struct fds_dev* ptr_fds_dev)
{
	 
   return 0;
}

int fds_dev_init(struct fds_dev* ptr_fds_dev)
{
	fd_devp = (struct fd_dev *)get_fddevp();
	spin_lock_init(&fds_spin_lock);
	if(ptr_fds_dev != NULL)
	{
		ptr_fds_dev->fds_dev_status = 0;
		ptr_fds_dev->fds_dev_step = 0;
  	ptr_fds_dev->ptr_iomap_addrs = &fd_devp->iomap_addrs;
  	ptr_fds_dev->ptr_mem_addr = &fd_devp->mem_addr;
  }

  p_fds_queue = alloc_workqueue("fds_work_queue",0,0);
  if(p_fds_queue == NULL)
  	return -EINVAL;
  INIT_WORK(&fds_work,fds_work_handle);
  if(ptr_fds_dev != NULL)
 		reset_modules(ptr_fds_dev->ptr_iomap_addrs->regs_fd);
	return 0;
}

void fds_clear_angle_context(void)
{
	struct fds_dev* ptr_fds_dev = fd_devp->ptr_fds_dev;
	unsigned long flags;

	reset_modules(ptr_fds_dev->ptr_iomap_addrs->regs_fd);
	cancel_work_sync(&fds_work);
	spin_lock_irqsave(&fds_spin_lock, flags);
	//fds_run_sl((*ptr_fds_dev->ptr_iomap_addrs),(*ptr_fds_dev->ptr_mem_addr),&ptr_fds_dev->dev_info);
	
	fds_exit_flag = 1;
  spin_unlock_irqrestore(&fds_spin_lock, flags);
}




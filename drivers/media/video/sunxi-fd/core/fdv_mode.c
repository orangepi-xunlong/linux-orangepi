#include <linux/cdev.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
//#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <asm/io.h>

#include "fd_priv.h"
#include "fd_lib.h"
#include "sun_fd.h"

#include "fdv_mode.h"

static struct fd_dev *fd_devp;
static short fdv_exit_flag = 0;
//EXPORT_SYMBOL(fdv_exit_flag);
static DECLARE_WAIT_QUEUE_HEAD(wait_fdv);
static spinlock_t fdv_spin_lock;

static struct workqueue_struct *p_fdv_queue;
//EXPORT_SYMBOL(p_fdv_queue);
static struct work_struct fdv_work;
//EXPORT_SYMBOL(fdv_work);

extern void reset_modules(volatile void __iomem* regs_modules);

long fdv_get_rtl(struct fdv_dev* ptr_fdv_dev,void __user* ptr_user_rtl_buf,int frame_num)
{

	 spin_lock(&fdv_spin_lock);

	 if(ptr_fdv_dev->fdv_dev_status == 0)
	 {
	    fd_rtl det_rtl;
	    fdv_get_rtl_sl(ptr_fdv_dev,(unsigned char*)(&det_rtl),frame_num);
	    
	    if(copy_to_user(ptr_user_rtl_buf,&det_rtl,sizeof(fd_rtl)))
		  {
		  		spin_unlock(&fdv_spin_lock);
	    		printk("base_get_rtl copy_to_user fail\n");
		   		return -EFAULT;
	    }
	 }
	 spin_unlock(&fdv_spin_lock);

	 return 0;
}

int fdv_run(struct fdv_dev* ptr_fdv_dev,void __user* arg)
{

	if(ptr_fdv_dev->fdv_dev_status != 0)
	{
		printk("warning:fdv_dev busy\n");
		return 0;
	}
	if(copy_from_user((void *)(&ptr_fdv_dev->dev_info),(const void __user *)arg,sizeof(fdv_dev_info)))
	{
		printk("warning:fdv_run copy_from_user fault");
		return -EINVAL;
	}
	cancel_work_sync(&fdv_work);
	
	spin_lock(&fdv_spin_lock);

	ptr_fdv_dev->fdv_dev_step = 0;
	fd_devp->phy_mem_addr.phy_mem_src_img_addr = (ptr_fdv_dev->dev_info.image_buf_phyaddr - 0x20000000) >> 2;
	set_src_img_phy_addr_sl(fd_devp->iomap_addrs,fd_devp->phy_mem_addr);
	queue_work(p_fdv_queue,&fdv_work);
	fdv_exit_flag = 0;
	spin_unlock(&fdv_spin_lock);

	return 0;
}    
   
int fdv_get_status(struct fdv_dev* ptr_fdv_dev)
{
	return fdv_get_status_sl((*ptr_fdv_dev->ptr_iomap_addrs));
}

long fdv_load_ftr_cfg(struct fdv_dev* ptr_fdv_dev,void __user* ptr_cfg_file,int size)
{
	if(copy_from_user(ptr_fdv_dev->ptr_mem_addr->mem_ftr_cfg_addr,ptr_cfg_file,size)){
		printk("base_load_ftr copy_from_user fail\n");
		return -EFAULT;
	}

	return 0;
}

long fdv_load_ftr(struct fdv_dev* ptr_fdv_dev,void __user* ptr_ld_file,int size)
{
	 
	if(copy_from_user(ptr_fdv_dev->ptr_mem_addr->mem_ld0_addr,ptr_ld_file,size)){
		printk("base_load_ftr copy_from_user fail\n");
		return -EFAULT;
	}
	if(copy_from_user(ptr_fdv_dev->ptr_mem_addr->mem_ld1_addr,ptr_ld_file + FTR_FILE_SIZE,size)){
		printk("base_load_ftr copy_from_user fail\n");
		return -EFAULT;
	}
	if(copy_from_user(ptr_fdv_dev->ptr_mem_addr->mem_ld2_addr,ptr_ld_file + 2*FTR_FILE_SIZE,size)){
		printk("base_load_ftr copy_from_user fail\n");
		return -EFAULT;
	}
	return 0;
}

static void fdv_work_handle(struct work_struct *work)
{
	struct fdv_dev* ptr_fdv_dev = fd_devp->ptr_fdv_dev;
  
fdv_work_handle_step:
	spin_lock(&fdv_spin_lock);
	
	if(fdv_exit_flag == 0)
	{  
	  if((fdv_exit_flag == 0)&&(ptr_fdv_dev->fdv_dev_status == 0)&&(ptr_fdv_dev->fdv_dev_step == 0))
	  {
	  	 ptr_fdv_dev->fdv_dev_status = 1;
	  	 ptr_fdv_dev->fdv_dev_step = 1;
	  	 //trace detect
		   if(fdv_run_sl(ptr_fdv_dev,1) == -1)
		   {
		   		spin_unlock(&fdv_spin_lock);
		   		goto fdv_work_handle_step;
		   }
		}
		else if((fdv_exit_flag == 0)&&(ptr_fdv_dev->fdv_dev_status == 1)&&(ptr_fdv_dev->fdv_dev_step == 1))
		{
			  int flag = 0;
			  ptr_fdv_dev->fdv_dev_step = 2;
			  if(fdv_step_complete_notify(ptr_fdv_dev,11,ptr_fdv_dev->dev_info.frame_num) != (long)(-11))
			      flag = 1;
        //global detect
        fdv_run_sl(ptr_fdv_dev,0);
        //process trace rtl
        if(flag)
        {
        	fdv_step_complete_notify(ptr_fdv_dev,1,ptr_fdv_dev->dev_info.frame_num);
        }
		}
		else if((fdv_exit_flag == 0)&&(ptr_fdv_dev->fdv_dev_status == 1)&&(ptr_fdv_dev->fdv_dev_step == 2))
		{ 
			  fdv_step_complete_notify(ptr_fdv_dev,12,ptr_fdv_dev->dev_info.frame_num);
			  ptr_fdv_dev->fdv_dev_step = 3;
			  //process global rtl and schedule other task
			  fdv_step_complete_notify(ptr_fdv_dev,2,ptr_fdv_dev->dev_info.frame_num);
			  
			  //temp add,because external mode unsupport.
			  ptr_fdv_dev->fdv_dev_step = 4;
				ptr_fdv_dev->fdv_dev_status = 0;
			  wake_up_fdv();  
		}
		else if((fdv_exit_flag == 0)&&(ptr_fdv_dev->fdv_dev_status == 1)&&(ptr_fdv_dev->fdv_dev_step == 3))
		{
			  //wait all prcocess complete
			  //notify main pocess all process complete
			  ptr_fdv_dev->fdv_dev_step = 4;
				ptr_fdv_dev->fdv_dev_status = 0;
			  wake_up_fdv();  
				
		}
	}
	else
		msleep(1);
  spin_unlock(&fdv_spin_lock);
}	

int fdv_interrupt_process(struct fdv_dev* ptr_fdv_dev)
{
	 //long flags;
	 if(fdv_exit_flag == 1)
	 	return 1;
	 queue_work(p_fdv_queue,&fdv_work);
	 return 1;
}

int fdv_get_run_status(struct fdv_dev* ptr_fdv_dev)
{
	 int status;

	 spin_lock(&fdv_spin_lock);
	 status =  ptr_fdv_dev->fdv_dev_status;
	 spin_unlock(&fdv_spin_lock);
	 return status;
}

int fdv_dev_release(struct fdv_dev* ptr_fdv_dev)
{
	//unsigned long flags;
	spin_lock(&fdv_spin_lock);
	wake_up_fdv();  
	if(ptr_fdv_dev->fdv_dev_status)
	{	
		reset_modules(ptr_fdv_dev->ptr_iomap_addrs->regs_fd);
		ptr_fdv_dev->fdv_dev_status = 0;
	}
	fdv_exit_flag = 1;
	spin_unlock(&fdv_spin_lock);
	cancel_work_sync(&fdv_work);
	
	if(p_fdv_queue)
	{
     destroy_workqueue(p_fdv_queue);
     p_fdv_queue = NULL;
  }
  kfree(ptr_fdv_dev->ptr_fdv_det_buffer);
	return 0;
}

int fdv_dev_exit(struct fdv_dev* ptr_fdv_dev)
{
   return 0;
}

int fdv_dev_init(struct fdv_dev* ptr_fdv_dev)
{
	fd_devp = (struct fd_dev *)get_fddevp();
	spin_lock_init(&fdv_spin_lock);
	if(ptr_fdv_dev != NULL)
	{
		ptr_fdv_dev->fdv_dev_status = 0;
		ptr_fdv_dev->fdv_dev_step = 0;
  	ptr_fdv_dev->ptr_iomap_addrs = &fd_devp->iomap_addrs;
  	ptr_fdv_dev->ptr_mem_addr = &fd_devp->mem_addr;
  }

  p_fdv_queue = alloc_workqueue("fdv_work_queue",0,0);
  if(p_fdv_queue == NULL)
  	return -EINVAL;
  INIT_WORK(&fdv_work,fdv_work_handle);
 
  //alloc 
  if(ptr_fdv_dev != NULL)
	{
  	ptr_fdv_dev->ptr_fdv_det_buffer = kmalloc(get_sizeof_fdv_buffer(),GFP_KERNEL);
  	memset(ptr_fdv_dev->ptr_fdv_det_buffer,0,get_sizeof_fdv_buffer());
  }
	return 0;
}

extern int count;
int fdv_clear_angle_context(void)
{
	struct fdv_dev* ptr_fdv_dev = fd_devp->ptr_fdv_dev;

	int try_count = 0;
	count = 0;
	reset_modules(ptr_fdv_dev->ptr_iomap_addrs->regs_fd);
	cancel_work_sync(&fdv_work);
	spin_lock(&fdv_spin_lock);
	
	while(ptr_fdv_dev->fdv_dev_status == 1)
	{
		msleep(5);
		try_count++;
		if(try_count > 30)
			break;
	}
	if(ptr_fdv_dev->fdv_dev_status == 1)
	{
		spin_unlock(&fdv_spin_lock);
		return -1;
	}
	else
		memset(ptr_fdv_dev->ptr_fdv_det_buffer,0,get_sizeof_fdv_buffer());
	ptr_fdv_dev->fdv_dev_step = 0;
	ptr_fdv_dev->fdv_dev_status = 0;
	fdv_exit_flag = 1;
	spin_unlock(&fdv_spin_lock);
	return 0;
}

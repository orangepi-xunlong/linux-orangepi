#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/preempt.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/rmap.h>
#include <linux/wait.h>
#include <linux/semaphore.h>
#include <linux/poll.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/dma.h>
#include <mach/hardware.h>
#include <asm/system.h>
#include <asm/siginfo.h>
#include <asm/signal.h>
#include <linux/ion_sunxi.h>
#include <linux/clk/sunxi.h>
#include <linux/dma-mapping.h>
#include "fd_priv.h"
#include "fd_lib.h"
#include "sun_fd.h" 
#include "base_mode.h"
#include "fdv_mode.h"
#include "fds_mode.h"

#ifdef CONFIG_HAS_EARLYSUSPEND
#define CONFIG_ES
#endif
#ifdef CONFIG_ES
#include <linux/earlysuspend.h>
#endif

//#define AW_IRQ_FD (AW_IRQ_GIC_START + 126) //temp add
#define CLK_AHB_FD "ahb_fd" //temp add
#define CLK_MOD_FD "fd" //temp add
#define CLK_DRAM_FD "dram_fd" //temp add

#define CLK_SYS_PLL11 "pll11"
#define CLK_SYS_PLL4 "pll4"
#define CLK_SYS_HOSC ""
#define CLK_VPU_POWER_GATING "cpurvddve"

#ifndef FDDEV_MAJOR
#define FDDEV_MAJOR (151)
#endif
#ifndef FDDEV_MINOR
#define FDDEV_MINOR (0)
#endif

#define BASE_MODE 1
#define FDS_MODE 2
#define FDV_MODE 3

#define FD_STATUS_IOADDR 0x18
#define FD_SYC_FLAG_IOADDR 0x9C

static struct fd_dev *fd_devp = NULL;
//EXPORT_SYMBOL(fd_devp);
//extern short fds_exit_flag;
static int g_dev_major = FDDEV_MAJOR;
static int g_dev_minor = FDDEV_MINOR;
module_param(g_dev_major, int, S_IRUGO);//S_IRUGO represent that g_dev_major can be read,but canot be write
module_param(g_dev_minor, int, S_IRUGO);
static struct clk *fd_moduleclk = NULL;
static struct clk *fd_pll11clk = NULL;
//static struct clk *ahb_fdclk = NULL;
//static struct clk *dram_fdclk = NULL;
//static struct clk *hosc_clk = NULL;
static struct clk *vpu_power_gating = NULL;
static int clk_status = 0;
static int standby_clk_status = 0;
static int vpu_clk_status = 0;
static unsigned long pll11clk_rate = 300000000;
static DECLARE_WAIT_QUEUE_HEAD(wait_fd);
static spinlock_t fd_spin_lock;
static int file_open_flag = 0;
static int suspend_stop_for_file = 0;
//struct workqueue_struct *p_fd_queue;
static struct mutex sus_mutex;
static struct mutex close_sus_mutex;
static int stop = 1;
static int fd_run_flag = 0;
static int close_need_disable = 0;
static int suspend_stop = 0;
#ifdef CONFIG_ES
	static struct early_suspend fd_early_suspend;
#endif

static int __devexit sunxi_fd_remove(struct platform_device *pdev);
static int __devinit sunxi_fd_probe(struct platform_device *pdev);
void close_clk(void);
int open_clk(void);

#ifdef CONFIG_ES
static void early_suspend_fd(struct early_suspend *h);
static void late_resume_fd(struct early_suspend *h);
#endif



static irqreturn_t fdEngineInterupt(int irq, void *dev)
{
	 struct iomap_para iomap_addrs = fd_devp->iomap_addrs;

	 writel(0x01,(volatile void __iomem *)(iomap_addrs.regs_fd + FD_STATUS_IOADDR));

	 ///////////////////////////
	 switch(fd_devp->mode_idx)
	 {
	 	  case 1:
	 	  	 //--------------------------//
		     fd_devp->fd_irq_value = 1;
		     fd_devp->fd_irq_flag = 1;
	       wake_up_interruptible(&wait_fd);
	 	  	break;
	 	  case 2:  
	 	  	 //--------------------------//
	 	  	 if(fds_interrupt_process(fd_devp->ptr_fds_dev) == 1)
	 	  	 {    
         	  fd_devp->fd_irq_value = 1;        
            fd_devp->fd_irq_flag = 1;         
            wake_up_interruptible(&wait_fd);
         }  
	 	  	break;
	 	  case 3:
	 	  	//printk(KERN_ALERT"-----------fdv_interrupt_process\n");
	 	  	fdv_interrupt_process(fd_devp->ptr_fdv_dev);
	 	  	break;
	 	  default:
	 	  	break;
	 }
	 //--------------------------//
	 
	  //printk(KERN_ALERT"--------------------------------------------------fdEngineInterupt 1   regr_val = %x,%x\n",regr_val,regr_val2);
	  return IRQ_HANDLED;
}

#define ISP_PLL_REG0 0xf6000028
#define PLL0_REG0 0xf600000C
#define FD_CLK_REG 0xf60004CC
#define BUS_CLK_GATING_REG0 0xf6000580
#define BUS_SOFT_RST_REG0 0xf60005a0

static int enable_fd_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;
	
	spin_lock_irqsave(&fd_spin_lock, flags);		
	
	if (clk_status == 1)
	{
		spin_unlock_irqrestore(&fd_spin_lock, flags);
		return res;
	}
	else
	{
		spin_unlock_irqrestore(&fd_spin_lock, flags);
	}

	if (clk_prepare_enable(fd_moduleclk)) {
		printk("enable fd_moduleclk failed; \n");
		goto out;
	}else {
		res = 0;
	}
	
	spin_lock_irqsave(&fd_spin_lock, flags);		
	clk_status = 1;
	spin_unlock_irqrestore(&fd_spin_lock, flags);
	
	init_hw_para_sl(fd_devp->iomap_addrs,fd_devp->mem_addr,fd_devp->phy_mem_addr);
out:
	return res;
}

static int disable_fd_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;

	spin_lock_irqsave(&fd_spin_lock, flags);		
	
	if (clk_status == 0) {
		res = 0;
		spin_unlock_irqrestore(&fd_spin_lock, flags);
		return res;
	}
	else
	{
		spin_unlock_irqrestore(&fd_spin_lock, flags);
	}

	if ((NULL == fd_moduleclk)||(IS_ERR(fd_moduleclk))) {
		printk("fd_moduleclk is invalid, just return!\n");
	} else {
		clk_disable_unprepare(fd_moduleclk);
		res = 0;
	}
	spin_lock_irqsave(&fd_spin_lock, flags);	
	clk_status = 0;
	spin_unlock_irqrestore(&fd_spin_lock, flags);

	return res;
}



static int clear_angle_context(void)
{
	switch(fd_devp->mode_idx)
	{
	case BASE_MODE:
		base_clear_angle_context();
		break;
	case FDS_MODE:
		fds_clear_angle_context();
		break;
	case FDV_MODE:
		fdv_clear_angle_context();
		break;
	default:
		break;
	}
	return 0;
}

static void clear_mode_context(int mode_idx)
{
	clear_angle_context();
	
	switch(mode_idx)
	{
		case FDS_MODE:
			fds_dev_release(fd_devp->ptr_fds_dev);
			break;
		case FDV_MODE:
			fdv_dev_release(fd_devp->ptr_fdv_dev);
			break;
		default:
			break;
	}
	return;
}

static long fddev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long   ret = 0;
	struct fd_dev *devp;
	int fd_timeout;
	unsigned long flags;
	devp = filp->private_data;
	
	switch(cmd)
	{
	   case IOCTL_ENABLE_ENGINE   :
	   		sunxi_periph_reset_deassert(fd_moduleclk);
	   	  enable_fd_hw_clk();
	   		break;
	   case IOCTL_DISABLE_ENGINE  :
	   		sunxi_periph_reset_assert(fd_moduleclk);
	   	  disable_fd_hw_clk();
	   		break; 
	   case IOCTL_SET_FREQ        :
	   		break;
	   case IOCTL_RESET_ENGINE    :
	   		break;
	   case IOCTL_WRITE_REG    :
	   	  {
	   	  	 struct reg_paras reg_para;
	   	     if(copy_from_user(&reg_para, (void __user*)arg, sizeof(struct reg_paras)))
					{
	          	return -EFAULT;
					}
					//printk("addr = %x,%x\n",fd_devp->iomap_addrs.regs_fd,reg_para.addr);
	   	  	writel(reg_para.value,(volatile void __iomem *)(fd_devp->iomap_addrs.regs_fd + reg_para.addr));
	   		}
	   		break;
	   case IOCTL_READ_REG    :
	   	  {
	   	  	 struct reg_paras reg_para;
	   	     if(copy_from_user(&reg_para, (void __user*)arg, sizeof(struct reg_paras)))
					{
	          	return -EFAULT;
					}

	   	  	return readl((volatile void __iomem *)(fd_devp->iomap_addrs.regs_fd + reg_para.addr));
	   		}
	   		break;
	   case IOCTL_SET_MODE        :
	   {
	   		if(devp->mode_idx != -1)
	   		{
	   			clear_mode_context(devp->mode_idx);
	   		}
			  if((0<arg)&&(arg<4))
			  	devp->mode_idx = arg;
			  else
			  	break;
			  
			  switch(devp->mode_idx)
			  {
			  case BASE_MODE:
			  	devp->mode_idx = BASE_MODE;
			  	break;
			  case FDS_MODE:
			  	devp->mode_idx = FDS_MODE;
			  	fds_dev_init(fd_devp->ptr_fds_dev);
			  	break;
			  case FDV_MODE:
			  	devp->mode_idx = FDV_MODE;
			  	fdv_dev_init(fd_devp->ptr_fdv_dev);
			  	break;
			  default:
			  	break;
			  }
			  
			 // printk("trace:FD IOCTL_SET_MODE finish  %x\n",readl(fd_devp->iomap_addrs.regs_fd + 0x00));
	   	  break;
	    }
	   //Base Mode           
	   case IOCTL_BASE_RUN        :
	   		
	   	  if(copy_from_user(devp->ptr_od_dev,(void __user*)arg,base_get_dev_size_sl()))
	   	  	return -1;
	   	  
	   	  
	   	  mutex_lock(&sus_mutex);
	 	 		if(suspend_stop == 1)
				{
					mutex_unlock(&sus_mutex);
					
					break;
				}
	 	 		fd_run_flag = 1;
	 	 		mutex_unlock(&sus_mutex);
	   	  base_run(devp->iomap_addrs,devp->mem_addr,devp->ptr_od_dev);
	   		break;
	   case IOCTL_BASE_GET_RTL    :
	   	  base_get_rtl(devp->iomap_addrs,devp->mem_addr,(void __user*)arg,1);
	   		break;
	   case IOCTL_BASE_GET_STA    :
	   	  (*((int*)arg)) = base_get_status(devp->iomap_addrs);
	   		break;
	   case IOCTL_BASE_WAIT_ENGINE:
	   	  fd_timeout = (int)arg;
        devp->fd_irq_value = 0;
        
        spin_lock_irqsave(&fd_spin_lock, flags);
        if(devp->fd_irq_flag)
        	devp->fd_irq_value = 1;
        spin_unlock_irqrestore(&fd_spin_lock, flags);
        
        mutex_lock(&sus_mutex);
	 	 		if(suspend_stop == 1)
				{
					devp->fd_irq_value = 0;
					fd_run_flag = 0;
					mutex_unlock(&sus_mutex);
					return devp->fd_irq_value;
				}
	 	 		mutex_unlock(&sus_mutex);
        wait_event_interruptible_timeout(wait_fd, devp->fd_irq_flag, fd_timeout*HZ);            
	      devp->fd_irq_flag = 0;
	      
	      fd_timeout = 0;
	      while(devp->fd_irq_value)
	      {
	      	if(((readl((volatile void __iomem *)(devp->iomap_addrs.regs_fd + FD_SYC_FLAG_IOADDR))&0x0FF) == devp->mem_addr.mem_sys_flag_addr[15])
	      			&&((readl((volatile void __iomem *)(devp->iomap_addrs.regs_fd + FD_SYC_FLAG_IOADDR))&0x0FF) != 0x01))
	      	{
	      		break;
	      	}
	      	if(fd_timeout == 5)
	      	{
	      		devp->fd_irq_value = 0;
	      		break;
	      	}
	      	fd_timeout++;
	      	msleep(2);
	      }	
	     	mutex_lock(&sus_mutex);
	 	 		fd_run_flag = 0;
	 	 		mutex_unlock(&sus_mutex);
	      /*����1����ʾ�жϷ��أ�����0����ʾtimeout����*/
			  return devp->fd_irq_value;
	   		break;
	   case IOCTL_BASE_LOAD_FTR   :
	   	  base_load_ftr(devp->mem_addr,(void __user*)arg,FTR_FILE_SIZE,0);
	   	  base_load_ftr(devp->mem_addr,(void __user*)arg + FTR_FILE_SIZE,FTR_FILE_SIZE,1);
	   	  base_load_ftr(devp->mem_addr,(void __user*)arg + 2*FTR_FILE_SIZE,FTR_FILE_SIZE,2);
	   	  //base_load_ftr(devp->mem_addr,(void __user*)arg + 3*FTR_FILE_SIZE,FTR_FILE_SIZE,3);
	   	  //base_load_ftr(devp->mem_addr,(void __user*)arg + 4*FTR_FILE_SIZE,FTR_FILE_SIZE,4);
	   	  //base_load_ftr(devp->mem_addr,(void __user*)arg + 5*FTR_FILE_SIZE,FTR_FILE_SIZE,5);
	   	 // base_load_ftr(devp->mem_addr,(void __user*)arg + 6*FTR_FILE_SIZE,FTR_FILE_SIZE,6);
	   	  //base_load_ftr(devp->mem_addr,(void __user*)arg + 7*FTR_FILE_SIZE,FTR_FILE_SIZE,7);
	   		break;

	   //FD Video Mode       
     case IOCTL_FD_V_SET_ORI    :
	 		  {
	 	    	 fdv_dev_info* ptr_det_para;
	 	    	 
	 	    	 clear_angle_context();
	 	  	   if(devp->mode_idx != FDV_MODE)
	 	  	   {
	 	  	   	  printk("warning:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  	   	  return 0;
	 	  	   }
	 	  	   if(devp->ptr_fdv_dev->fdv_dev_status != 0)
	 	  	   {
	 	  	   	  printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	   	  return 0;
	 	  	   }
	 	  	   devp->fd_irq_flag = 0;
			     ptr_det_para = &(devp->ptr_fdv_dev->dev_info);
	 	   	   switch(arg)
	 	   	   {
	 	   	      case 0:
			      	   ptr_det_para->angle0 = 1;
				        ptr_det_para->angle90 = 0;
				        ptr_det_para->angle180 = 0;
				        ptr_det_para->angle270 = 0;
			      	   break;
			      case 1:
			      	   ptr_det_para->angle0 = 0;
				        ptr_det_para->angle90 = 1;
				        ptr_det_para->angle180 = 0;
				        ptr_det_para->angle270 = 0;
			      	   break;
			      case 2:
			      	   ptr_det_para->angle0 = 0;
				        ptr_det_para->angle90 = 0;
				        ptr_det_para->angle180 = 1;
				        ptr_det_para->angle270 = 0;
			      	   break;
			      case 3:
			      	   ptr_det_para->angle0 = 0;
				        ptr_det_para->angle90 = 0;
				        ptr_det_para->angle180 = 0;
				        ptr_det_para->angle270 = 1;
			      	break;
			      default:
			      	break;
	 	   	   }
     	  }
//     	  printk("trace:FD IOCTL_FD_V_SET_ORI finish\n");
     		break;
     case IOCTL_FD_V_ENABLE_SMILE  :
     	  {
	 	   	  fdv_dev_info* ptr_det_para;
	 	   	  if(devp->mode_idx != FDV_MODE)
	 	  	  {
	 	  	  	printk("warning:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  	  	return 0;
	 	  	  }
	 	  	  if(devp->ptr_fdv_dev->fdv_dev_status != 0)
	 	  	  {
	 	  	  	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	  	return 0;
	 	  	  }
	 	        ptr_det_para = (fdv_dev_info*)(&devp->ptr_fdv_dev->dev_info);
			      ptr_det_para->smile_flag = 1;
     	  }
     		break;
	 case IOCTL_FD_V_DISABLE_SMILE  :
	 	    {
	 	  	  fdv_dev_info* ptr_det_para;
	 	  	  if(devp->mode_idx != FDV_MODE)
	 	  	  {
	 	  	  	printk("warning:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  	  	return 0;
	 	  	  }
	 	  	  if(devp->ptr_fdv_dev->fdv_dev_status != 0)
	 	  	  {
	 	  	  	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	  	return 0;
	 	  	  }
	 	      ptr_det_para = (fdv_dev_info*)(&devp->ptr_fdv_dev->dev_info);
			    ptr_det_para->smile_flag = 0;
     	  }
     		break;
     case IOCTL_FD_V_ENABLE_BLINK  :
     	  {
	 	    	 fdv_dev_info* ptr_det_para;
	 	  	   if(devp->mode_idx != FDV_MODE)
	 	  	    {
	 	  	    	printk("warning:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  	    	return 0;
	 	  	    }
	 	  	    if(devp->ptr_fdv_dev->fdv_dev_status != 0)
	 	  	    {
	 	  	    	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	    	return 0;
	 	  	    }
	 	       ptr_det_para = (fdv_dev_info*)(&devp->ptr_fdv_dev->dev_info);
			     ptr_det_para->blink_flag = 1;
     	  }
     		break;
	 case IOCTL_FD_V_DISABLE_BLINK  :
	 	    {
	 	   	   fdv_dev_info* ptr_det_para;
	 	   	   if(devp->mode_idx != FDV_MODE)
	 	  	   {
	 	  	    	printk("warning:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  	    	return 0;
	 	  	   }
	 	  	   if(devp->ptr_fdv_dev->fdv_dev_status != 0)
	 	  	   {
	 	  	    	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	    	return 0;
	 	  	   }
	 	       ptr_det_para = (fdv_dev_info*)(&devp->ptr_fdv_dev->dev_info);
			     ptr_det_para->blink_flag = 0;
     	  }
     		break;
     case IOCTL_FD_V_RUN        :
     	  if(devp->mode_idx != FDV_MODE)
	 	  	{
	 	  	   printk("warning IOCTL_FD_V_RUN:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  	   return 0;
	 	  	}
	 	  	if(devp->ptr_fdv_dev->fdv_dev_status != 0)
	 	  	{
	 	  	   printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING  ::IOCTL_FD_V_RUN\n");
	 	  	   return 0;
	 	  	}
	 	 		
	 	  	mutex_lock(&sus_mutex);
	 	 		if(suspend_stop == 1)
				{
					mutex_unlock(&sus_mutex);
					break;
				}
	 	 		fd_run_flag = 1;
	 	 		mutex_unlock(&sus_mutex);
	 	 		
     	  fdv_run(devp->ptr_fdv_dev,(void __user*)arg);
     		break;
     case IOCTL_FD_V_GET_RTL    :
     	  if(devp->mode_idx != FDV_MODE)
	 	  	{
	 	  		printk("warning IOCTL_FD_V_GET_RTL:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  		return 0;
	 	  	}
	 	  	
	 	    fdv_get_rtl(devp->ptr_fdv_dev,(void __user*)arg,1);
     		break;
     case IOCTL_FD_V_GET_STA    :
     		break;
     case IOCTL_FD_V_WAIT_FD    :
     		break;
     case IOCTL_FD_V_WAIT_SMILE :
     		break;
     case IOCTL_FD_V_WAIT_BLINK :
     		break;
     case IOCTL_FD_V_WAIT_ENGINE :
     	  if(devp->mode_idx != FDV_MODE)
	 	  	  {
	 	  	      printk("warning IOCTL_FD_V_WAIT_ENGINE:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  	      return 0;
	 	  	  }
	 	      fd_timeout = (int)arg;
          devp->fd_irq_value = 0;
        
           spin_lock(&fd_spin_lock);
          if(devp->fd_irq_flag)
        	 devp->fd_irq_value = 1;
          spin_unlock(&fd_spin_lock);
        	
        	mutex_lock(&sus_mutex);
	 	 			if(suspend_stop == 1)
					{
						devp->fd_irq_value = 0;
						fd_run_flag = 0;
						mutex_unlock(&sus_mutex);
						return devp->fd_irq_value;
					}
	 	 			mutex_unlock(&sus_mutex);
	 	 		
          wait_event_interruptible_timeout(wait_fd, devp->fd_irq_flag, fd_timeout*HZ);            
	        devp->fd_irq_flag = 0;
	        fd_timeout = 0;
	        
	        while(devp->fd_irq_value)
	        {
	        	if((readl((volatile void __iomem *)(devp->iomap_addrs.regs_fd + FD_SYC_FLAG_IOADDR))&0x0FF) == devp->mem_addr.mem_sys_flag_addr[15])
	        	{
	        		devp->ptr_fdv_dev->fdv_dev_status = 0;
	        		break;
	        	}
	        	if(fd_timeout == 5)
	        	{
	        		devp->fd_irq_value = 0;
	        		break;
	        	}
	        	fd_timeout++;
	        	msleep(2);
	        }	
	        /*����1����ʾ�жϷ��أ�����0����ʾtimeout����*/
	        mutex_lock(&sus_mutex);
	 	 	fd_run_flag = 0;
	 	 	mutex_unlock(&sus_mutex);
	 	 			
		      return devp->fd_irq_value;
     		break;
     case IOCTL_FD_V_LOAD_FD_FTR:
//     	  printk("trace:FD IOCTL_FD_V_LOAD_FD_FTR start 0 \n"); 
     	  if(devp->mode_idx != FDV_MODE)
	 	  	{
	 	  		printk("warning:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  		return 0;
	 	  	}
//	 	  	printk("trace:FD IOCTL_FD_S_LOAD_FD_FTR start 1 %x\n",devp->ptr_fds_dev);
	 	  	if(devp->ptr_fdv_dev->fdv_dev_status != 0)
	 	  	{
	 	  		printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  		return 0;
	 	  	}
     	  fdv_load_ftr(devp->ptr_fdv_dev,(void __user*)arg,FTR_FILE_SIZE);
     		break;
     case IOCTL_FD_V_LOAD_FD_FTR_CFG:
     	  if(devp->mode_idx != FDV_MODE)
	 	  	{
	 	  		printk("warning:FD MODULES MODE IS NOT FDV_MODE\n");
	 	  		return 0;
	 	  	}
//	 	  	printk("trace:FD IOCTL_FD_S_LOAD_FD_FTR start 1 %x\n",devp->ptr_fds_dev);
	 	  	if(devp->ptr_fdv_dev->fdv_dev_status != 0)
	 	  	{
	 	  		printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  		return 0;
	 	  	}
     	  fdv_load_ftr_cfg(devp->ptr_fdv_dev,(void __user*)arg,8*1024);
     		break;
     case IOCTL_FD_V_LOAD_SE_FTR:
     		break;
     case IOCTL_FD_V_LOAD_BL_FTR: 
     		break;    
                          
     //FD Static Frame Mode
     case IOCTL_FD_S_SET_ORI    :
	 	    {
	 	    	 fds_dev_info* ptr_det_para;
	 	    	 devp->fd_irq_flag = 0;
	 	  	   if(devp->mode_idx != FDS_MODE)
	 	  	   {
	 	  	   	printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  	   	return 0;
	 	  	   }
	 	  	   if(devp->ptr_fds_dev->fds_dev_status != 0)
	 	  	   {
	 	  	   	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	   	return 0;
	 	  	   }
	 	  	   
			     ptr_det_para = &(devp->ptr_fds_dev->dev_info);
	 	   	   switch(arg)
	 	   	   {
	 	   	      case 0:
			      	   ptr_det_para->angle0 = 1;
				        ptr_det_para->angle90 = 0;
				        ptr_det_para->angle180 = 0;
				        ptr_det_para->angle270 = 0;
			      	   break;
			      case 1:
			      	   ptr_det_para->angle0 = 0;
				        ptr_det_para->angle90 = 1;
				        ptr_det_para->angle180 = 0;
				        ptr_det_para->angle270 = 0;
			      	   break;
			      case 2:
			      	   ptr_det_para->angle0 = 0;
				        ptr_det_para->angle90 = 0;
				        ptr_det_para->angle180 = 1;
				        ptr_det_para->angle270 = 0;
			      	   break;
			      case 3:
			      	   ptr_det_para->angle0 = 0;
				        ptr_det_para->angle90 = 0;
				        ptr_det_para->angle180 = 0;
				        ptr_det_para->angle270 = 1;
			      	break;
			      default:
			      	break;
	 	   	   }
     	  }
//     	  printk("trace:FD IOCTL_FD_S_SET_ORI finish\n");
     	  break;
     case IOCTL_FD_S_ENABLE_SMILE  :
	 	   {
	 	   	  fds_dev_info* ptr_det_para;
	 	   	  if(devp->mode_idx != FDS_MODE)
	 	  	  {
	 	  	  	printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  	  	return 0;
	 	  	  }
	 	  	  if(devp->ptr_fds_dev->fds_dev_status != 0)
	 	  	  {
	 	  	  	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	  	return 0;
	 	  	  }
	 	        ptr_det_para = (fds_dev_info*)(&devp->ptr_fds_dev->dev_info);
			      ptr_det_para->smile_flag = 1;
     	  }
     	  break;
	 case IOCTL_FD_S_DISABLE_SMILE  :
	 	  {
	 	  	  fds_dev_info* ptr_det_para;
	 	  	  if(devp->mode_idx != FDS_MODE)
	 	  	  {
	 	  	  	printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  	  	return 0;
	 	  	  }
	 	  	  if(devp->ptr_fds_dev->fds_dev_status != 0)
	 	  	  {
	 	  	  	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	  	return 0;
	 	  	  }
	 	      ptr_det_para = (fds_dev_info*)(&devp->ptr_fds_dev->dev_info);
			    ptr_det_para->smile_flag = 0;
     	  }
     	  break;
     case IOCTL_FD_S_ENABLE_BLINK  :
	 	    {
	 	    	 fds_dev_info* ptr_det_para;
	 	  	   if(devp->mode_idx != FDS_MODE)
	 	  	    {
	 	  	    	printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  	    	return 0;
	 	  	    }
	 	  	    if(devp->ptr_fds_dev->fds_dev_status != 0)
	 	  	    {
	 	  	    	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	    	return 0;
	 	  	    }
	 	       ptr_det_para = (fds_dev_info*)(&devp->ptr_fds_dev->dev_info);
			     ptr_det_para->blink_flag = 1;
     	  }
     	  break;
	 case IOCTL_FD_S_DISABLE_BLINK  :
	 	   {
	 	   	   fds_dev_info* ptr_det_para;
	 	   	   if(devp->mode_idx != FDS_MODE)
	 	  	   {
	 	  	    	printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  	    	return 0;
	 	  	   }
	 	  	   if(devp->ptr_fds_dev->fds_dev_status != 0)
	 	  	   {
	 	  	    	printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	    	return 0;
	 	  	   }
	 	       ptr_det_para = (fds_dev_info*)(&devp->ptr_fds_dev->dev_info);
			     ptr_det_para->blink_flag = 0;
     	  }
     	  break;
     case IOCTL_FD_S_RUN        :
     	  if(devp->mode_idx != FDS_MODE)
	 	  	{
	 	  	   printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  	   return 0;
	 	  	}
	 	  	if(devp->ptr_fds_dev->fds_dev_status != 0)
	 	  	{
	 	  	   printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  	   return 0;
	 	  	}
//	 	  	printk("----------fddev_ioctl:IOCTL_FD_S_RUN  2\n");

				mutex_lock(&sus_mutex);
	 	 		if(suspend_stop == 1)
				{
					mutex_unlock(&sus_mutex);
					break;
				}
	 	 		fd_run_flag = 1;
	 	 		mutex_unlock(&sus_mutex);
	 	 		
     	  fds_run(devp->ptr_fds_dev,(void __user*)arg);
     	  break;
     case IOCTL_FD_S_GET_RTL    :
     	  if(devp->mode_idx != FDS_MODE)
	 	  	{
	 	  		printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  		return 0;
	 	  	}
	 	  	
	 	    fds_get_rtl(devp->ptr_fds_dev,(void __user*)arg,1);
     	  break;
     case IOCTL_FD_S_GET_STA    :
     	  break;
     case IOCTL_FD_S_WAIT_FD    :
     	  break;
     case IOCTL_FD_S_WAIT_SMILE :
     	  break;
     case IOCTL_FD_S_WAIT_BLINK :
     	  break;
     case IOCTL_FD_S_WAIT_ENGINE :
     	    if(devp->mode_idx != FDS_MODE)
	 	  	  {
	 	  	      printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  	      return 0;
	 	  	  }
	 	      fd_timeout = (int)arg;
          devp->fd_irq_value = 0;
        
          spin_lock_irqsave(&fd_spin_lock, flags);
          if(devp->fd_irq_flag)
        	 devp->fd_irq_value = 1;
          spin_unlock_irqrestore(&fd_spin_lock, flags);
        
       		mutex_lock(&sus_mutex);
	 	 			if(suspend_stop == 1)
					{
						devp->fd_irq_value = 0;
						fd_run_flag = 0;
						mutex_unlock(&sus_mutex);
						return devp->fd_irq_value;
					}
	 	 			mutex_unlock(&sus_mutex); 	
       
          wait_event_interruptible_timeout(wait_fd, devp->fd_irq_flag, fd_timeout*HZ);            
	        devp->fd_irq_flag = 0;
	        fd_timeout = 0;
	        while(devp->fd_irq_value)
	        {
	        	if((readl((volatile void __iomem *)(devp->iomap_addrs.regs_fd + FD_SYC_FLAG_IOADDR))&0x0FF) == devp->mem_addr.mem_sys_flag_addr[15])
	        			//	&&((readl(devp->iomap_addrs.regs_fd + FD_SYC_FLAG_IOADDR)&0x0FF) != 0x01))
	        	{
	        		devp->ptr_fds_dev->fds_dev_status = 0;
	        		break;
	        	}
	        	if(fd_timeout == 500)
	        	{
	        		devp->fd_irq_value = 0;
	        		printk("warning:FD MODULES MODE IS fd_timeout\n");
	        		break;
	        	}
	        	fd_timeout++;
	        	msleep(20);
	        }	
	        /*����1����ʾ�жϷ��أ�����0����ʾtimeout����*/
	     		mutex_lock(&sus_mutex);
	 	 			fd_run_flag = 0;
	 	 			mutex_unlock(&sus_mutex);
		      return devp->fd_irq_value;
     	  break;
     case IOCTL_FD_S_LOAD_FD_FTR:
//     	  printk("trace:FD IOCTL_FD_S_LOAD_FD_FTR start 0 \n"); 
     	  if(devp->mode_idx != FDS_MODE)
	 	  	{
	 	  		printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  		return 0;
	 	  	}
//	 	  	printk("trace:FD IOCTL_FD_S_LOAD_FD_FTR start 1 %x\n",devp->ptr_fds_dev);
	 	  	if(devp->ptr_fds_dev->fds_dev_status != 0)
	 	  	{
	 	  		printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  		return 0;
	 	  	}
     	  fds_load_ftr(devp->ptr_fds_dev,(void __user*)arg,FTR_FILE_SIZE);
     	  break;
     case IOCTL_FD_S_LOAD_FD_FTR_CFG:
     	  if(devp->mode_idx != FDS_MODE)
	 	  	{
	 	  		printk("warning:FD MODULES MODE IS NOT FDS_MODE\n");
	 	  		return 0;
	 	  	}
//	 	  	printk("trace:FD IOCTL_FD_S_LOAD_FD_FTR start 1 %x\n",devp->ptr_fds_dev);
	 	  	if(devp->ptr_fds_dev->fds_dev_status != 0)
	 	  	{
	 	  		printk("warning:FD MODULES IS BUSY,CAN NOT SET ANYTHING\n");
	 	  		return 0;
	 	  	}
     	  fds_load_ftr_cfg(devp->ptr_fds_dev,(void __user*)arg,8*1024);
     		break;
     case IOCTL_FD_S_LOAD_SE_FTR:
     	  break;
     case IOCTL_FD_S_LOAD_BL_FTR:
     	  break;
     default:
     	  break;
	}

	return ret;
}

void wake_up_fdv(void)
{

	fd_devp->fd_irq_value = 1;        
  fd_devp->fd_irq_flag = 1;    
      
  wake_up_interruptible(&wait_fd);

}

static int fddev_open(struct inode *inode, struct file *filp)
{
	
	//unsigned long flags;
	struct fd_dev *devp;
	printk("=============fddev_open: 0\n");

	 
   mutex_lock(&close_sus_mutex); 	
	 if(suspend_stop_for_file == 1)
	 {
	 	  mutex_unlock(&close_sus_mutex);
	 	  printk("=============fddev_open: alearly sus\n");
	 	  return -EBUSY;
	 }
	 file_open_flag = 1;
	 mutex_unlock(&close_sus_mutex);
	 
	devp = container_of(inode->i_cdev, struct fd_dev, cdev);
	
	spin_lock(&fd_spin_lock);
	if(stop == 1)
	   stop = 0;
	else
	{
		 spin_unlock(&fd_spin_lock);
		 mutex_lock(&close_sus_mutex);
		 file_open_flag = 0;
		 mutex_unlock(&close_sus_mutex);
		 return -EBUSY;
  }

	spin_unlock(&fd_spin_lock);
	
	filp->private_data = devp;
	nonseekable_open(inode, filp);	 

  //setup_timer(&fd_devp->fd_engine_timer, fd_engine_for_events, (unsigned long)fd_devp);
//	printk(KERN_ALERT"-----------fddev_open 1\n"); 
	close_need_disable = 0;
	printk("=============fddev_open: 1\n");
	return 0;
}	

static int fddev_release(struct inode *inode, struct file *filp)
{
//	unsigned long flags;

	struct fd_dev *devp;
	int ret;
	printk("=============fddev_release:over 0\n");
	//del_timer_sync(&fd_devp->fd_engine_timer);
	mutex_lock(&close_sus_mutex);
  if(close_need_disable == 1)
  {	
	   sunxi_periph_reset_assert(fd_moduleclk);
	   
	   ret = disable_fd_hw_clk();
	   if (ret < 0) {
	   	printk("Warring: fd clk disable somewhere error!\n");
	   	goto __end;
	   }
	   if(vpu_clk_status == 1)
	   {
	   		clk_disable_unprepare(vpu_power_gating);
	   		vpu_clk_status = 0;
	   }
  }
__end:
  close_need_disable = 0;
	mutex_unlock(&close_sus_mutex);

	devp = container_of(inode->i_cdev, struct fd_dev, cdev);
	
	spin_lock(&fd_spin_lock);
	stop = 1; 
	//suspend_stop = 1;
	spin_unlock(&fd_spin_lock);
	
	if(devp->mode_idx == FDS_MODE)
	{
		//clear context��reset hardware
		fds_dev_release(devp->ptr_fds_dev);
	}
	
	if(devp->mode_idx == FDV_MODE)
	{
		//clear context��reset hardware
		fdv_dev_release(devp->ptr_fdv_dev);
	}
	
	devp->mode_idx = -1;
	mutex_lock(&close_sus_mutex);
	file_open_flag = 0;
	mutex_unlock(&close_sus_mutex);
	printk("=============fddev_release:over 1\n");
	return 0;
}

static int fddev_mmap(struct file *filp, struct vm_area_struct *vma)
{
	return 0;
}

#ifdef CONFIG_PM
static int snd_sw_fd_suspend(struct platform_device *pdev,pm_message_t state)
{	
	int ret = 0;
	int count = 0;
	//unsigned long flags;
	printk("[fd] standby suspend\n");
	do
	{
		mutex_lock(&close_sus_mutex);
		if(file_open_flag == 1)
		{
			mutex_unlock(&close_sus_mutex);
			count++;
			msleep(10);
		 	if(count > 150)
		 		break;
		}
		else
		{
			suspend_stop_for_file = 1;
			mutex_unlock(&close_sus_mutex);
			break;
		}
  }while(1);
	
	printk("[fd] standby suspend  = %d\n",count);
	if(count < 150)
	{
	   count = 0;
	   while(1)
	   { 
	   	 mutex_lock(&sus_mutex);
	   	
	   	 if(fd_run_flag != 1)
	   	 {
	   	 	  suspend_stop = 1;
	   	 	  mutex_unlock(&sus_mutex);
	   	 	  break;
	   	 }
	   	 mutex_unlock(&sus_mutex);
	   	 msleep(10);
	   	 count++;
	   	 if(count > 100)
	   	 	break;
	   }
	   
	   msleep(2);
	   sunxi_periph_reset_assert(fd_moduleclk);
	   msleep(2);
	   standby_clk_status = clk_status;
     
	   ret = disable_fd_hw_clk();
	   if (ret < 0) {
	   	printk("Warring: fd clk disable somewhere error!\n");
	   	return -EFAULT;
	   }
	   
	   if(vpu_clk_status == 1)
	   {
	   		clk_disable_unprepare(vpu_power_gating);
	   		vpu_clk_status = 0;
	   }
  }
  else
  {
  	standby_clk_status = clk_status;
  	mutex_lock(&close_sus_mutex);
  	close_need_disable = 1;
  	mutex_unlock(&close_sus_mutex);
  }
	printk("[fd] standby suspend  end %d\n",count);
	return ret;
}

static int snd_sw_fd_resume(struct platform_device *pdev)
{
	int ret = 0;
	
	printk("[fd] standby resume\n");
	close_need_disable = 0;
	if(vpu_clk_status == 0)
	{
		clk_prepare_enable(vpu_power_gating);
		vpu_clk_status = 1;
	}
	if(standby_clk_status == 1)
	{
		ret = enable_fd_hw_clk();
		if (ret < 0) {
			printk("Warring: fd clk enable somewhere error!\n");
			return -EFAULT;
		}
		sunxi_periph_reset_deassert(fd_moduleclk);
	}
	mutex_lock(&sus_mutex);
	suspend_stop = 0;
	mutex_unlock(&sus_mutex);
	
	mutex_lock(&close_sus_mutex);
	suspend_stop_for_file = 0;
	mutex_unlock(&close_sus_mutex);
	return 0;
}
#endif

static void fd_platform_release(struct device* dev)
{
	return;
}

static struct file_operations fddev_fops = {
    .owner   = THIS_MODULE,
    .mmap    = fddev_mmap,
    .open    = fddev_open,
    .release = fddev_release,
	.llseek  = no_llseek,
    .unlocked_ioctl   = fddev_ioctl,
};

/*data relating*/

static struct resource sunxi_fd_resource[] = {
	[0] = {
		.start = SUNXI_IRQ_FD,
		.end   = SUNXI_IRQ_FD,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device sw_device_fd = {
	.name = "sunxi-fd",   
	.id = -1,
	.num_resources	= ARRAY_SIZE(sunxi_fd_resource),
	.resource	= sunxi_fd_resource,	   
	.dev = {
		.release = fd_platform_release,
	}   
};

static struct platform_driver sw_fd_driver = {
	.probe		= sunxi_fd_probe,
	.remove		= __devexit_p(sunxi_fd_remove),
#ifdef CONFIG_PM
	.suspend	= snd_sw_fd_suspend,
	.resume		= snd_sw_fd_resume,
#endif
	.driver		= {
		.name	= "sunxi-fd",
	},
};

/*open  clock*/
/*
#define ISP_PLL_REG0 0xf6000028
#define PLL0_REG0 0xf600000C
#define FD_CLK_REG 0xf60004CC
#define BUS_CLK_GATING_REG0 0xf6000580
#define BUS_SOFT_RST_REG0 0xf60005a0
*/

#ifdef CONFIG_ES
static void early_suspend_fd(struct early_suspend *h)
{
	int ret = 0;
	int count = 0;
	//unsigned long flags;
	printk("[fd] early standby suspend\n");
	
	do
	{
		mutex_lock(&close_sus_mutex);
		if(file_open_flag == 1)
		{
			mutex_unlock(&close_sus_mutex);
			count++;
			msleep(10);
		 	if(count > 150)
		 		break;
		}
		else
		{
			suspend_stop_for_file = 1;
			mutex_unlock(&close_sus_mutex);
			break;
		}
  }while(1);

	printk("[fd] early standby suspend  = %d\n",count);
	if(count < 150)
	{
	   count = 0;
	   while(1)
	   { 
	   	 mutex_lock(&sus_mutex);
	   	
	   	 if(fd_run_flag != 1)
	   	 {
	   	 	  suspend_stop = 1;
	   	 	  mutex_unlock(&sus_mutex);
	   	 	  break;
	   	 }
	   	 mutex_unlock(&sus_mutex);
	   	 msleep(10);
	   	 count++;
	   	 if(count > 100)
	   	 	break;
	   }
	   
	   msleep(2);
	   sunxi_periph_reset_assert(fd_moduleclk);
	   msleep(2);
	   standby_clk_status = clk_status;
     
	   ret = disable_fd_hw_clk();
	   if (ret < 0) {
	   	printk("Warring: fd clk disable somewhere error!\n");
	   	return;
	   }
	   
	   if(vpu_clk_status == 1)
	   {
	   		clk_disable_unprepare(vpu_power_gating);
	   		vpu_clk_status = 0;
	   }
  }
  else
  {
  	standby_clk_status = clk_status;
  	
  	mutex_lock(&close_sus_mutex);
  	close_need_disable = 1;
  	mutex_unlock(&close_sus_mutex);
  }
	printk("[fd] early standby suspend  end %d\n",count);
	return;
}

static void late_resume_fd(struct early_suspend *h)
{
	int ret = 0;
//	unsigned long flags;
	printk("[fd] early standby resume\n");
	close_need_disable = 0;
	mutex_lock(&sus_mutex);
	suspend_stop = 0;
	mutex_unlock(&sus_mutex);
	
	mutex_lock(&close_sus_mutex);
	suspend_stop_for_file = 0;
	mutex_unlock(&close_sus_mutex);
	
	if(vpu_clk_status == 0)
	{
		clk_prepare_enable(vpu_power_gating);
		vpu_clk_status = 1;
	}
	if(standby_clk_status == 1)
	{
		ret = enable_fd_hw_clk();
		if (ret < 0) {
			printk("Warring: fd clk enable somewhere error!\n");
			return;
		}
		sunxi_periph_reset_deassert(fd_moduleclk);
	}
	return;
}
#endif
/*
static void set_wbit(unsigned int addr,unsigned int  mask)
{
	unsigned int tmp = readl(addr);
	tmp |= mask;
	writel(tmp,addr);
}

static void clr_wbit(unsigned int addr,unsigned int  mask)
{
	unsigned int tmp = readl(addr);
	mask = ~mask;
	tmp &= mask;
	writel(tmp,addr);
}
*/
/*
static int open_clk2(void)
{
	unsigned int tmp;
	unsigned int fd_clk_div = 3;
	unsigned int fd_clk_src = 0;
	
	
	set_wbit(PLL0_REG0,(unsigned int)(1)<<31);
	//set_wbit(ISP_PLL_REG0,(unsigned int)(1)<<31);
	set_wbit(BUS_SOFT_RST_REG0,(unsigned int)(1)<<0);
	set_wbit(BUS_CLK_GATING_REG0,(unsigned int)(1)<<0);
	
	tmp = readl(FD_CLK_REG);
	if(fd_clk_src == 0)
	{
		tmp &= 0xF0FFFFFF;
		tmp |= 0x01000000;
	}
	else
	{
		tmp &= 0xF0FFFFFF;
		tmp |= 0x0C000000;
	}
	tmp &= 0xFFFFFFF0;
	fd_clk_div += 1;
	tmp |= fd_clk_div;
	writel(tmp,FD_CLK_REG);
	set_wbit(FD_CLK_REG,((unsigned int)1)<<31);
	return 0;
}

static void close_clk2(void)
{
	set_wbit(BUS_SOFT_RST_REG0,(unsigned int)(1)<<0);
	clr_wbit(BUS_SOFT_RST_REG0,(unsigned int)(1)<<0);
	clr_wbit(BUS_CLK_GATING_REG0,(unsigned int)(1)<<0);
	clr_wbit(FD_CLK_REG,((unsigned int)1)<<31);
}
*/

int open_clk(void)
{
	fd_pll11clk = clk_get(NULL, CLK_SYS_PLL4);
	if ((!fd_pll11clk)||IS_ERR(fd_pll11clk)) {
		printk("try to get fd_pll4clk fail\n");
		return -EINVAL;
	}
	pll11clk_rate = clk_get_rate(fd_pll11clk);
	if (pll11clk_rate <= 0) {
		printk("can't get teh fd pll11 clk\n");
		return -EINVAL;
	}
	fd_moduleclk = clk_get(NULL, CLK_MOD_FD);
	if ((!fd_moduleclk)||IS_ERR(fd_moduleclk)) {
		printk("try to get fd_moduleclk fail\n");
		return -EINVAL;
	}
	
	if(clk_set_parent(fd_moduleclk, fd_pll11clk)){
		printk("set parent of ve_moduleclk to ve_pll4clk failed!\n");		
		return -EFAULT;
	}
	
	pll11clk_rate = 320*1000000;
	if (clk_set_rate(fd_moduleclk, pll11clk_rate)) {

		printk("try to set fd rate fail\n");
	}
	
	sunxi_periph_reset_deassert(fd_moduleclk);
	clk_prepare_enable(fd_moduleclk);
	clk_status = 1;
	pll11clk_rate = clk_get_rate(fd_moduleclk);
	
	///////
	printk("open_clk:fd_clk_rate = %ld\n",pll11clk_rate);
	return 0;
}


void close_clk(void)
{	
	if (NULL == fd_moduleclk || IS_ERR(fd_moduleclk)) {
		printk("fd_moduleclk handle is invalid, just return!\n");
	} else {
		disable_fd_hw_clk();
		clk_put(fd_moduleclk);
		fd_moduleclk = NULL;
	}
	
	if (NULL == fd_pll11clk || IS_ERR(fd_pll11clk)) {
		printk("fd_pll4clk handle is invalid, just return!\n");
	} else {	
		clk_put(fd_pll11clk);
	}
}

void free_fd_mem(void);
void free_dev_mem(void);

int alloc_fd_mem(void)
{
	fd_devp->mem_addr.mem_ld0_addr = sunxi_buf_alloc(FTR_FILE_SIZE, &fd_devp->phy_mem_addr.phy_mem_ld0_addr);
	fd_devp->mem_addr.mem_ld1_addr = sunxi_buf_alloc(FTR_FILE_SIZE, &fd_devp->phy_mem_addr.phy_mem_ld1_addr);
	fd_devp->mem_addr.mem_ld2_addr = sunxi_buf_alloc(FTR_FILE_SIZE, &fd_devp->phy_mem_addr.phy_mem_ld2_addr);
	fd_devp->mem_addr.mem_ld3_addr = sunxi_buf_alloc(FTR_FILE_SIZE, &fd_devp->phy_mem_addr.phy_mem_ld3_addr);
	fd_devp->mem_addr.mem_ld4_addr = sunxi_buf_alloc(FTR_FILE_SIZE, &fd_devp->phy_mem_addr.phy_mem_ld4_addr);
	fd_devp->mem_addr.mem_ld5_addr = sunxi_buf_alloc(FTR_FILE_SIZE, &fd_devp->phy_mem_addr.phy_mem_ld5_addr);
	fd_devp->mem_addr.mem_ld6_addr = sunxi_buf_alloc(FTR_FILE_SIZE, &fd_devp->phy_mem_addr.phy_mem_ld6_addr);
	fd_devp->mem_addr.mem_ld7_addr = sunxi_buf_alloc(FTR_FILE_SIZE, &fd_devp->phy_mem_addr.phy_mem_ld7_addr);
	if (!fd_devp->mem_addr.mem_ld0_addr || !fd_devp->mem_addr.mem_ld1_addr || !fd_devp->mem_addr.mem_ld2_addr || !fd_devp->mem_addr.mem_ld3_addr
		|| !fd_devp->mem_addr.mem_ld4_addr || !fd_devp->mem_addr.mem_ld5_addr || !fd_devp->mem_addr.mem_ld6_addr) {
		printk("%s %d: err, alloc fail\n", __func__, __LINE__);
		free_fd_mem();
		return -1;
	}
	return 0;
}

int alloc_dev_mem(void)
{
	fd_devp->mem_addr.mem_rtl_addr = sunxi_buf_alloc(RTL_BUFFER_SIZE, &fd_devp->phy_mem_addr.phy_mem_rtl_addr);
	fd_devp->phy_mem_addr.phy_mem_rtl_size = RTL_BUFFER_SIZE;
	fd_devp->mem_addr.mem_ftr_cfg_addr  = sunxi_buf_alloc(FTR_CFG__BUFFER_SIZE, &fd_devp->phy_mem_addr.phy_mem_ftr_cfg_addr);
	fd_devp->phy_mem_addr.phy_mem_ftr_cfg_size = FTR_CFG__BUFFER_SIZE;
	fd_devp->mem_addr.mem_scale_img_addr = sunxi_buf_alloc(SCALE_IMG_BUFFER_SIZE, &fd_devp->phy_mem_addr.phy_mem_scale_img_addr);
	fd_devp->phy_mem_addr.phy_mem_scale_img_size = SCALE_IMG_BUFFER_SIZE;
	fd_devp->mem_addr.mem_tmpbuf_addr = sunxi_buf_alloc(TMP_BUFFER_SIZE, &fd_devp->phy_mem_addr.phy_mem_tmpbuf_addr);
	if (!fd_devp->mem_addr.mem_tmpbuf_addr) {
		fd_devp->mem_addr.mem_tmpbuf_addr = sunxi_buf_alloc(HALF_TMP_BUFFER_SIZE, &fd_devp->phy_mem_addr.phy_mem_tmpbuf_addr);
		fd_devp->phy_mem_addr.phy_mem_tmpbuf_size = HALF_TMP_BUFFER_SIZE;
	} else {
		fd_devp->phy_mem_addr.phy_mem_tmpbuf_size = TMP_BUFFER_SIZE;
	}
	fd_devp->mem_addr.mem_roi_addr = sunxi_buf_alloc(ROI_CFG_BUFFER_SIZE, &fd_devp->phy_mem_addr.phy_mem_roi_addr);
	fd_devp->phy_mem_addr.phy_mem_roi_size = ROI_CFG_BUFFER_SIZE;
	fd_devp->mem_addr.mem_sys_flag_addr = sunxi_buf_alloc(SYS_FLAG_BUFFER_SIZE, &fd_devp->phy_mem_addr.phy_mem_sys_flag_addr);
	fd_devp->phy_mem_addr.phy_mem_sys_flag_size = SYS_FLAG_BUFFER_SIZE;

	if (!fd_devp->mem_addr.mem_rtl_addr || !fd_devp->mem_addr.mem_ftr_cfg_addr || !fd_devp->mem_addr.mem_scale_img_addr
		|| !fd_devp->mem_addr.mem_tmpbuf_addr || !fd_devp->mem_addr.mem_roi_addr || !fd_devp->mem_addr.mem_sys_flag_addr) {
		printk("%s %d: err, alloc fail\n", __func__, __LINE__);
		free_dev_mem();
		return -1;
	}
	return 0;
}

void free_fd_mem(void)
{
 	if (fd_devp->mem_addr.mem_ld0_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ld0_addr, fd_devp->phy_mem_addr.phy_mem_ld0_addr, FTR_FILE_SIZE);
 		fd_devp->mem_addr.mem_ld0_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_ld1_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ld1_addr, fd_devp->phy_mem_addr.phy_mem_ld1_addr, FTR_FILE_SIZE);
 		fd_devp->mem_addr.mem_ld1_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_ld2_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ld2_addr, fd_devp->phy_mem_addr.phy_mem_ld2_addr, FTR_FILE_SIZE);
 		fd_devp->mem_addr.mem_ld2_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_ld3_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ld3_addr, fd_devp->phy_mem_addr.phy_mem_ld3_addr, FTR_FILE_SIZE);
 		fd_devp->mem_addr.mem_ld3_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_ld4_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ld4_addr, fd_devp->phy_mem_addr.phy_mem_ld4_addr, FTR_FILE_SIZE);
 		fd_devp->mem_addr.mem_ld4_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_ld5_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ld5_addr, fd_devp->phy_mem_addr.phy_mem_ld5_addr, FTR_FILE_SIZE);
 		fd_devp->mem_addr.mem_ld5_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_ld6_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ld6_addr, fd_devp->phy_mem_addr.phy_mem_ld6_addr, FTR_FILE_SIZE);
 		fd_devp->mem_addr.mem_ld6_addr = NULL;
 	}
	if (fd_devp->mem_addr.mem_ld7_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ld7_addr, fd_devp->phy_mem_addr.phy_mem_ld7_addr, FTR_FILE_SIZE);
		fd_devp->mem_addr.mem_ld7_addr = NULL;
	}
 }

void free_dev_mem(void)
{
 	if (fd_devp->mem_addr.mem_rtl_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_rtl_addr, fd_devp->phy_mem_addr.phy_mem_rtl_addr, fd_devp->phy_mem_addr.phy_mem_rtl_size);
 		fd_devp->mem_addr.mem_rtl_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_ftr_cfg_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_ftr_cfg_addr, fd_devp->phy_mem_addr.phy_mem_ftr_cfg_addr, fd_devp->phy_mem_addr.phy_mem_ftr_cfg_size);
 		fd_devp->mem_addr.mem_ftr_cfg_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_scale_img_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_scale_img_addr, fd_devp->phy_mem_addr.phy_mem_scale_img_addr, fd_devp->phy_mem_addr.phy_mem_scale_img_size);
 		fd_devp->mem_addr.mem_scale_img_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_tmpbuf_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_tmpbuf_addr, fd_devp->phy_mem_addr.phy_mem_tmpbuf_addr, fd_devp->phy_mem_addr.phy_mem_tmpbuf_size);
 		fd_devp->mem_addr.mem_tmpbuf_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_roi_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_roi_addr, fd_devp->phy_mem_addr.phy_mem_roi_addr, fd_devp->phy_mem_addr.phy_mem_roi_size);
 		fd_devp->mem_addr.mem_roi_addr = NULL;
 	}
 	if (fd_devp->mem_addr.mem_sys_flag_addr) {
		sunxi_buf_free(fd_devp->mem_addr.mem_sys_flag_addr, fd_devp->phy_mem_addr.phy_mem_sys_flag_addr, fd_devp->phy_mem_addr.phy_mem_sys_flag_size);
 		fd_devp->mem_addr.mem_sys_flag_addr = NULL;
 	}

}


static int request_fd_resource(void)
{
	int ret = 0;
	dev_t dev = 0;

	/*register or alloc the device number.*/
  
	if (g_dev_major) {
		dev = MKDEV(g_dev_major, g_dev_minor);	
		ret = register_chrdev_region(dev, 1, "fd_dev");
	} else {
		ret = alloc_chrdev_region(&dev, g_dev_minor, 1, "fd_dev");
		g_dev_major = MAJOR(dev);
		g_dev_minor = MINOR(dev);
	}

	if (ret < 0) {
		printk(KERN_WARNING "fd_dev: can't get major %d\n", g_dev_major);
		return ret;
	}
	
	spin_lock_init(&fd_spin_lock);
	fd_devp = kmalloc(sizeof(struct fd_dev), GFP_KERNEL);
  
	if (fd_devp == NULL) {
		printk("malloc mem for fd device err\n");
		return -ENOMEM;
	}	
	memset(fd_devp, 0, sizeof(struct fd_dev));
	fd_devp->irq = SUNXI_IRQ_FD;
	
	sema_init(&fd_devp->sem, 1);
	init_waitqueue_head(&fd_devp->wq);	

	memset(&fd_devp->iomap_addrs, 0, sizeof(struct iomap_para));

    ret = request_irq(SUNXI_IRQ_FD, fdEngineInterupt, 0, "fd_dev", NULL);
    if (ret < 0) {
        printk("request irq err\n");
        return -EINVAL;
    }
	/* map for fd io space */
    fd_devp->iomap_addrs.regs_fd = (volatile void __iomem*)ioremap((phys_addr_t)FD_REGS_BASE, (unsigned long)FD_REGS_SIZE);
	
    if (!fd_devp->iomap_addrs.regs_fd){
        printk("cannot map region for macc");
    }

   fd_devp->iomap_addrs.regs_ccmu = (volatile void __iomem*)ioremap((phys_addr_t)CCMU_REGS_BASE, (unsigned long)1*1024);

   	/*--------������Ӧ��Դ-----------*/
	//�����ļ���Դ��16byte���룬ÿ��200KByte����Ҫ����ion
	if (alloc_fd_mem()) {
		printk("%s %d: err, alloc fail\n", __func__, __LINE__);
		return -1;
	}
	        
	fd_devp->phy_mem_addr.phy_mem_ld0_size = FTR_FILE_SIZE;
	fd_devp->phy_mem_addr.phy_mem_ld1_size = FTR_FILE_SIZE;
	fd_devp->phy_mem_addr.phy_mem_ld2_size = FTR_FILE_SIZE;
	fd_devp->phy_mem_addr.phy_mem_ld3_size = FTR_FILE_SIZE;
	fd_devp->phy_mem_addr.phy_mem_ld4_size = FTR_FILE_SIZE;
	fd_devp->phy_mem_addr.phy_mem_ld5_size = FTR_FILE_SIZE;
	fd_devp->phy_mem_addr.phy_mem_ld6_size = FTR_FILE_SIZE;
	fd_devp->phy_mem_addr.phy_mem_ld7_size = FTR_FILE_SIZE;
	
	if(fd_devp->phy_mem_addr.phy_mem_ld0_addr == 0)
	{
		printk("warning:alloc phy_mem_ld0 fail\n");
		return -1;
	}
	if(fd_devp->phy_mem_addr.phy_mem_ld1_addr == 0)
	{
		printk("warning:alloc phy_mem_ld1 fail\n");
		return -1;
	}
	if(fd_devp->phy_mem_addr.phy_mem_ld2_addr == 0)
	{
		printk("warning:alloc phy_mem_ld2 fail\n");
		return -1;
	}
	if(fd_devp->phy_mem_addr.phy_mem_ld3_addr == 0)
	{
		printk("warning:alloc phy_mem_ld3 fail\n");
		return -1;
	}
	if(fd_devp->phy_mem_addr.phy_mem_ld4_addr == 0)
	{
		printk("warning:alloc phy_mem_ld4 fail\n");
		return -1;
	}
	if(fd_devp->phy_mem_addr.phy_mem_ld5_addr == 0)
	{
		printk("warning:alloc phy_mem_ld5 fail\n");
		return -1;
	}
	if(fd_devp->phy_mem_addr.phy_mem_ld6_addr == 0)
	{
		printk("warning:alloc phy_mem_ld6 fail\n");
		return -1;
	}
	if(fd_devp->phy_mem_addr.phy_mem_ld7_addr == 0)
	{
		printk("warning:alloc phy_mem_ld7 fail\n");
		return -1;
	}
	
	
	if(fd_devp->mem_addr.mem_ld0_addr == NULL)
	{
		printk("warning:map mem_ld0_addr fail\n");
		return -1;
	}
	if(fd_devp->mem_addr.mem_ld1_addr == NULL)
	{
		printk("warning:map mem_ld1_addr fail\n");
		return -1;
	}
	if(fd_devp->mem_addr.mem_ld2_addr == NULL)
	{
		printk("warning:map mem_ld2_addr fail\n");
		return -1;
	}
	if(fd_devp->mem_addr.mem_ld3_addr == NULL)
	{
		printk("warning:map mem_ld3_addr fail\n");
		return -1;
	}
	if(fd_devp->mem_addr.mem_ld4_addr == NULL)
	{
		printk("warning:map mem_ld4_addr fail\n");
		return -1;
	}
	if(fd_devp->mem_addr.mem_ld5_addr == NULL)
	{
		printk("warning:map mem_ld5_addr fail\n");
		return -1;
	}
	if(fd_devp->mem_addr.mem_ld6_addr == NULL)
	{
		printk("warning:map mem_ld6_addr fail\n");
		return -1;
	}
	if(fd_devp->mem_addr.mem_ld7_addr == NULL)
	{
		printk("warning:map mem_ld7_addr fail\n");
		return -1;
	}
	
	if (alloc_dev_mem()) {
		printk("%s %d: err, alloc fail\n", __func__, __LINE__);
		free_fd_mem();        
		return -1;
	}
	
	//�������ģʽ���豸�Ľṹ��ռ䡣
	
	fd_devp->ptr_od_dev = kmalloc(base_get_dev_size_sl(), GFP_KERNEL);
	fd_devp->ptr_fds_dev = kmalloc(sizeof(struct fds_dev), GFP_KERNEL);
	fd_devp->ptr_fdv_dev = kmalloc(sizeof(struct fdv_dev), GFP_KERNEL);
	
	//------------------------------------------------------//
	fd_devp->mem_addr.mem_src_img_size  =   fd_devp->phy_mem_addr.phy_mem_src_img_size;    
	fd_devp->mem_addr.mem_scale_img_size= 	fd_devp->phy_mem_addr.phy_mem_scale_img_size;
	fd_devp->mem_addr.mem_tmpbuf_size   = 	fd_devp->phy_mem_addr.phy_mem_tmpbuf_size;   
	fd_devp->mem_addr.mem_roi_size      = 	fd_devp->phy_mem_addr.phy_mem_roi_size;      
	fd_devp->mem_addr.mem_sys_flag_size = 	fd_devp->phy_mem_addr.phy_mem_sys_flag_size; 
	fd_devp->mem_addr.mem_rtl_size      = 	fd_devp->phy_mem_addr.phy_mem_rtl_size;      
	fd_devp->mem_addr.mem_ftr_cfg_size  = 	fd_devp->phy_mem_addr.phy_mem_ftr_cfg_size;  
                       
	fd_devp->mem_addr.mem_ld0_size      = 	fd_devp->phy_mem_addr.phy_mem_ld0_size;      
	fd_devp->mem_addr.mem_ld1_size      = 	fd_devp->phy_mem_addr.phy_mem_ld1_size;      
	fd_devp->mem_addr.mem_ld2_size      = 	fd_devp->phy_mem_addr.phy_mem_ld2_size;      
	fd_devp->mem_addr.mem_ld3_size      = 	fd_devp->phy_mem_addr.phy_mem_ld3_size;      
	fd_devp->mem_addr.mem_ld4_size      = 	fd_devp->phy_mem_addr.phy_mem_ld4_size;      
	fd_devp->mem_addr.mem_ld5_size      = 	fd_devp->phy_mem_addr.phy_mem_ld5_size;      
	fd_devp->mem_addr.mem_ld6_size      = 	fd_devp->phy_mem_addr.phy_mem_ld6_size;      
	fd_devp->mem_addr.mem_ld7_size      = 	fd_devp->phy_mem_addr.phy_mem_ld7_size;      
	//------------------------------------------------------//
	mutex_init(&sus_mutex);
	mutex_init(&close_sus_mutex);
	
	init_hw_para_sl(fd_devp->iomap_addrs,fd_devp->mem_addr,fd_devp->phy_mem_addr);
	return 0;
}


static void release_fd_resource(void)
{
//	unsigned long flags; 
	
	dev_t dev;
	dev = MKDEV(g_dev_major, g_dev_minor);
	
   free_irq(SUNXI_IRQ_FD, NULL);
  if(fd_devp != NULL) 
	{
		iounmap((void __iomem *)(fd_devp->iomap_addrs.regs_fd));
		iounmap((void __iomem *)(fd_devp->iomap_addrs.regs_ccmu));
	
	  //�����ļ���Դ��16byte���룬ÿ��200KByte����Ҫ����ion
	free_fd_mem();
	free_dev_mem();
	}
	
	unregister_chrdev_region(dev, 1);	
	
	if(fd_devp != NULL) 
	{
	   if(fd_devp->ptr_od_dev) {
	   	kfree(fd_devp->ptr_od_dev);
	   }
	   if(fd_devp->ptr_fds_dev) {
	   	kfree(fd_devp->ptr_fds_dev);
	   }
	   if(fd_devp->ptr_fdv_dev) {
	   	kfree(fd_devp->ptr_fdv_dev);
	   }
	   	kfree(fd_devp);
	}
	mutex_destroy(&sus_mutex);
	mutex_destroy(&close_sus_mutex);
}


static int create_fd_cdev(void)
{
	int ret = 0;
//	int err = 0;
	int devno;
//	unsigned int val;
	//dev_t dev = 0;
	/* Create char device */
	devno = MKDEV(g_dev_major, g_dev_minor);	
	cdev_init(&fd_devp->cdev, &fddev_fops);
	fd_devp->cdev.owner = THIS_MODULE;
	fd_devp->cdev.ops = &fddev_fops;
	ret = cdev_add(&fd_devp->cdev, devno, 1);
	if (ret) {
		printk(KERN_NOTICE "Err:%d add fddev", ret);	
	}
  fd_devp->class = class_create(THIS_MODULE, "fd_dev");
  fd_devp->dev   = device_create(fd_devp->class, NULL, devno, NULL, "fd_dev");
  
  return 0;
}

static void destroy_fd_cdev(void)
{
	dev_t dev = 0;
	dev = MKDEV(g_dev_major, g_dev_minor);
	if (fd_devp) {
		cdev_del(&fd_devp->cdev);
		device_destroy(fd_devp->class, dev);
		class_destroy(fd_devp->class);
	}
}

static int fd_init(void)
{
	int ret = 0;
	
	printk(KERN_ALERT"[fd]: install start!!!\n");
		
	vpu_power_gating = clk_get(NULL, CLK_VPU_POWER_GATING);
	if ((!vpu_power_gating)||IS_ERR(vpu_power_gating)) {
		printk("try to get vpu_power_gating fail\n");
		return -EINVAL;
	}
	if(vpu_clk_status == 0)
	{
		clk_prepare_enable(vpu_power_gating);
		vpu_clk_status = 1;
	}
	msleep(10);
		
	ret = open_clk();
	if(ret != 0)
		return ret;
		
	ret = request_fd_resource();
	if(ret != 0)
		return ret;
			
  ret = create_fd_cdev();
  if(ret != 0)
		return ret; 
  
  
  sunxi_periph_reset_assert(fd_moduleclk);
  ret = disable_fd_hw_clk();
	if (ret < 0) {
		printk("Warring: fd clk disable somewhere error!\n");
		return -EFAULT;
	}
	
	
#ifdef CONFIG_ES
	fd_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1;
	fd_early_suspend.suspend = early_suspend_fd;
	fd_early_suspend.resume = late_resume_fd;
	register_early_suspend(&fd_early_suspend);
	printk("register fd_early_suspend @ probe handle!\n");
#endif
	
	printk(KERN_ALERT"[fd]: install end!!!\n");
	
	return 0;	
}


static void fd_exit(void)
{
	destroy_fd_cdev();
	sunxi_periph_reset_assert(fd_moduleclk);
	close_clk();
  fds_dev_exit(fd_devp->ptr_fds_dev);
  fdv_dev_exit(fd_devp->ptr_fdv_dev);
	release_fd_resource();
  
#ifdef CONFIG_ES
	unregister_early_suspend(&fd_early_suspend);
#endif
  
  if (NULL == vpu_power_gating || IS_ERR(vpu_power_gating)) {
		printk("vpu_power_gating handle is invalid, just return!\n");
	} else {
		if(vpu_clk_status == 1)
		{
			clk_disable_unprepare(vpu_power_gating);
			vpu_clk_status = 0;
		}
		clk_put(vpu_power_gating);
		vpu_power_gating = NULL;
	}
	msleep(10);
	
  printk(KERN_ALERT"exit fd\n");
}

static int __devexit sunxi_fd_remove(struct platform_device *pdev)
{
	fd_exit();
	return 0;
}

static int __devinit sunxi_fd_probe(struct platform_device *pdev)
{
		fd_init();
		return 0;
}

static int __init sun_fd_init(void)
{
	int err = 0;
	if((platform_device_register(&sw_device_fd))<0)
		return err;
		
	printk("sun fd version 1.0 10\n");
	if((err = platform_driver_register(&sw_fd_driver)) < 0)
		return err;

	return 0;
}

static void __exit sun_fd_exit(void)
{
	platform_driver_unregister(&sw_fd_driver);
	platform_device_unregister(&sw_device_fd);
}
/*
void reset_modules(volatile void __iomem* regs_modules)
{
	writel(0x80000000,regs_modules + 0x10);
}
*/

void* get_fddevp(void)
{
	return (void *)fd_devp;
}


module_init(sun_fd_init);
module_exit(sun_fd_exit);

MODULE_AUTHOR("calvinLin");
MODULE_DESCRIPTION("User mode fd device interface");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:sun_fd");


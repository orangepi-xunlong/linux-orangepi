/*
 * drivers\media\cedar_ve
 * (C) Copyright 2010-2016
 * Reuuimlla Technology Co., Ltd. <www.allwinnertech.com>
 * fangning<fangning@allwinnertech.com>
 *
 * some simple description for this code
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

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
#include <linux/clk/sunxi.h>
#include <mach/sunxi-smc.h>

//#include <linux/clk/clk-sun8iw3.h>
#include <mach/irqs.h>
#include "cedar_ve.h"
#include <linux/regulator/consumer.h>

struct regulator *regu;

#define DRV_VERSION "0.01alpha"


#undef USE_CEDAR_ENGINE

#ifndef CEDARDEV_MAJOR
#define CEDARDEV_MAJOR (150)
#endif
#ifndef CEDARDEV_MINOR
#define CEDARDEV_MINOR (0)
#endif


#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1)
#define MACC_REGS_BASE      (0x01C0E000)           // Media ACCelerate

#elif defined CONFIG_ARCH_SUN9IW1P1

#define MACC_REGS_BASE      (0x03A40000)           // Media ACCelerate

#elif defined CONFIG_ARCH_SUN8IW5P1
#define MACC_REGS_BASE      (0x01C0E000)           // Media ACCelerate

#elif defined CONFIG_ARCH_SUN8IW6P1
#define MACC_REGS_BASE      (0x01C0E000)           // Media ACCelerate

#elif defined CONFIG_ARCH_SUN8IW7P1
#define MACC_REGS_BASE      (0x01C0E000)           // Media ACCelerate

#elif defined CONFIG_ARCH_SUN8IW8P1
#define MACC_REGS_BASE      (0x01C0E000)           // Media ACCelerate

#elif defined CONFIG_ARCH_SUN8IW9P1
#define MACC_REGS_BASE      (0x01C0E000)           // Media ACCelerate

#else
	#error "Unknown chip type!"
#endif

//#define CEDAR_DEBUG

#define VE_CLK_HIGH_WATER  (700)//400MHz
#define VE_CLK_LOW_WATER   (100) //160MHz

int g_dev_major = CEDARDEV_MAJOR;
int g_dev_minor = CEDARDEV_MINOR;
module_param(g_dev_major, int, S_IRUGO);//S_IRUGO represent that g_dev_major can be read,but canot be write
module_param(g_dev_minor, int, S_IRUGO);

struct clk *ve_moduleclk = NULL;
struct clk *ve_parent_pll_clk = NULL;
struct clk *ve_power_gating = NULL;

static unsigned long ve_parent_clk_rate = 300000000;

extern unsigned long ve_start;
extern unsigned long ve_size;

struct iomap_para{
	volatile char* regs_macc;
	volatile char* regs_avs;
};

static DECLARE_WAIT_QUEUE_HEAD(wait_ve);
struct cedar_dev {
	struct cdev cdev;	             /* char device struct                 */
	struct device *dev;              /* ptr to class device struct         */
	struct class  *class;            /* class for auto create device node  */

	struct semaphore sem;            /* mutual exclusion semaphore         */
	
	wait_queue_head_t wq;            /* wait queue for poll ops            */

	struct iomap_para iomap_addrs;   /* io remap addrs                     */

	struct timer_list cedar_engine_timer;
	struct timer_list cedar_engine_timer_rel;
	
	u32 irq;                         /* cedar video engine irq number      */
	u32 de_irq_flag;                    /* flag of video decoder engine irq generated */
	u32 de_irq_value;                   /* value of video decoder engine irq          */
	u32 en_irq_flag;                    /* flag of video encoder engine irq generated */
	u32 en_irq_value;                   /* value of video encoder engine irq          */
	u32 irq_has_enable;
	u32 ref_count;

	u32 jpeg_irq_flag;                    /* flag of video jpeg dec irq generated */
	u32 jpeg_irq_value;                   /* value of video jpeg dec  irq */
};

struct ve_info {
	unsigned int set_vol_flag;
};

struct cedar_dev *cedar_devp;

u32 int_sta=0,int_value;

/*
 * Video engine interrupt service routine
 * To wake up ve wait queue
 */
static irqreturn_t VideoEngineInterupt(int irq, void *dev)
{
	unsigned int ve_int_status_reg;
    unsigned int ve_int_ctrl_reg;
    unsigned int status;
    volatile int val;
    int modual_sel;
	unsigned int interrupt_enable;
    struct iomap_para addrs = cedar_devp->iomap_addrs;
    
    modual_sel = readl(addrs.regs_macc + 0);
    if(modual_sel&(3<<6))
    {
    	if(modual_sel&(1<<7))//avc enc
    	{
    		ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0xb00 + 0x1c);
    		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0xb00 + 0x14);
			interrupt_enable = readl(ve_int_ctrl_reg) &(0x7);
    		status = readl(ve_int_status_reg);
    		status &= 0xf;
    	}
    	else//isp
    	{
    		ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0xa00 + 0x10);
    		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0xa00 + 0x08);
			interrupt_enable = readl(ve_int_ctrl_reg) &(0x1);
    		status = readl(ve_int_status_reg);
    		status &= 0x1;
    	}
    	
    	if(status && interrupt_enable) //modify by fangning 2013-05-22
    	{
    		//disable interrupt
    		if(modual_sel&(1<<7))//avc enc
	    	{
	    		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0xb00 + 0x14);
	    		val = readl(ve_int_ctrl_reg);
		        writel(val & (~0x7), ve_int_ctrl_reg);
	    	}
	    	else//isp
	    	{
	    		ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0xa00 + 0x08);
	    		val = readl(ve_int_ctrl_reg);
		        writel(val & (~0x1), ve_int_ctrl_reg);
	    	}
		
		    cedar_devp->en_irq_value = 1;	//hx modify 2011-8-1 16:08:47
		    cedar_devp->en_irq_flag = 1;
		    //any interrupt will wake up wait queue
		    wake_up_interruptible(&wait_ve);        //ioctl
		}
    }

#if defined CONFIG_ARCH_SUN8IW8P1
	if(modual_sel&(0x20))
	{
		ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0xe00 + 0x1c);    
	    ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0xe00 + 0x14);
		interrupt_enable = readl(ve_int_ctrl_reg) & (0x38);
		
		status = readl(ve_int_status_reg);

    	if((status&0x7) && interrupt_enable) 
    	{
		    //disable interrupt
		    val = readl(ve_int_ctrl_reg);
		    writel(val & (~0x38), ve_int_ctrl_reg);
	
		    cedar_devp->jpeg_irq_value = 1;
		    cedar_devp->jpeg_irq_flag = 1;
			
		    //any interrupt will wake up wait queue
		    wake_up_interruptible(&wait_ve);
		}
	}
#endif

    modual_sel &= 0xf;
 	if(modual_sel<=4)
 	{
		// estimate Which video format
	    switch (modual_sel)
	    {
	        case 0: //mpeg124  
	        	ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0x100 + 0x1c);    
	            ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x100 + 0x14);
				interrupt_enable = readl(ve_int_ctrl_reg) & (0x7c);
	            break;
	        case 1: //h264    
	        	ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0x200 + 0x28);          
	            ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x200 + 0x20);
				interrupt_enable = readl(ve_int_ctrl_reg) & (0xf);
	            break;
	        case 2: //vc1  
	        	ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0x300 + 0x2c);           
	            ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x300 + 0x24);
				interrupt_enable = readl(ve_int_ctrl_reg) & (0xf);
	            break;
	        case 3: //rmvb      
	        	ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0x400 + 0x1c);        
	            ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x400 + 0x14);
				interrupt_enable = readl(ve_int_ctrl_reg) & (0xf);
	            break;
				
			case 4: //hevc		
				ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0x500 + 0x38); 	   
				ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x500 + 0x30);
				interrupt_enable = readl(ve_int_ctrl_reg) & (0xf);
				break;

	        default:   
	        	ve_int_status_reg = (unsigned int)(addrs.regs_macc + 0x100 + 0x1c);           
	            ve_int_ctrl_reg = (unsigned int)(addrs.regs_macc + 0x100 + 0x14);
				interrupt_enable = readl(ve_int_ctrl_reg) & (0xf);
	            printk("macc modual sel not defined!modual_sel:%x\n", modual_sel);
	            break;
	    }
	
		status = readl(ve_int_status_reg);

    	if((status&0xf) && interrupt_enable) //modify by fangning 2013-05-22
    	{
		    //disable interrupt
		    if(modual_sel == 0) {
		        val = readl(ve_int_ctrl_reg);
		        writel(val & (~0x7c), ve_int_ctrl_reg);
		    } else {
		        val = readl(ve_int_ctrl_reg);
		        writel(val & (~0xf), ve_int_ctrl_reg);
		    }
		    cedar_devp->de_irq_value = 1;	//hx modify 2011-8-1 16:08:47
		    cedar_devp->de_irq_flag = 1;
		    //any interrupt will wake up wait queue
//		    wake_up_interruptible(&wait_ve);        //ioctl
		    wake_up(&wait_ve);        //ioctl
		}
	}

    return IRQ_HANDLED;
}

static int clk_status = 0;
static LIST_HEAD(run_task_list);
static LIST_HEAD(del_task_list);
static spinlock_t cedar_spin_lock;
#define CEDAR_RUN_LIST_NONULL	-1
#define CEDAR_NONBLOCK_TASK  0
#define CEDAR_BLOCK_TASK 1
#define CLK_REL_TIME 10000
#define TIMER_CIRCLE 50
#define TASK_INIT      0x00
#define TASK_TIMEOUT   0x55
#define TASK_RELEASE   0xaa
#define SIG_CEDAR		35

int enable_cedar_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;
	
	spin_lock_irqsave(&cedar_spin_lock, flags);		
	
	if (clk_status == 1)
		goto out;

	clk_status = 1;

	if (clk_enable(ve_moduleclk)) {
		printk("enable ve_moduleclk failed; \n");
		goto out;
	}else {
		res = 0;
	}
	
	#ifdef CEDAR_DEBUG
	printk("%s,%d\n",__func__,__LINE__);
	#endif

out:
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
	return res;
}

int disable_cedar_hw_clk(void)
{
	unsigned long flags;
	int res = -EFAULT;

	spin_lock_irqsave(&cedar_spin_lock, flags);		
	
	if (clk_status == 0) {
		res = 0;
		goto out;
	}
	clk_status = 0;

	if ((NULL == ve_moduleclk)||(IS_ERR(ve_moduleclk))) {
		printk("ve_moduleclk is invalid, just return!\n");
	} else {
		clk_disable(ve_moduleclk);
		res = 0;
	}

	#ifdef CEDAR_DEBUG
	printk("%s,%d\n",__func__,__LINE__);
	#endif

out:
	spin_unlock_irqrestore(&cedar_spin_lock, flags);
	return res;
}

void cedardev_insert_task(struct cedarv_engine_task* new_task)
{	
	struct cedarv_engine_task *task_entry;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);		
	
	if(list_empty(&run_task_list))
		new_task->is_first_task = 1;


	list_for_each_entry(task_entry, &run_task_list, list) {
		if ((task_entry->is_first_task == 0) && (task_entry->running == 0) && (task_entry->t.task_prio < new_task->t.task_prio)) {
			break;
		}
	}
	
	list_add(&new_task->list, task_entry->list.prev);	

	#ifdef CEDAR_DEBUG
	printk("%s,%d, TASK_ID:",__func__,__LINE__);
	list_for_each_entry(task_entry, &run_task_list, list) {
		printk("%d!", task_entry->t.ID);
	}
	printk("\n");
	#endif
	
	mod_timer(&cedar_devp->cedar_engine_timer, jiffies + 0);

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

int cedardev_del_task(int task_id)
{
	struct cedarv_engine_task *task_entry;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);		

	list_for_each_entry(task_entry, &run_task_list, list) {
		if (task_entry->t.ID == task_id && task_entry->status != TASK_RELEASE) {
			task_entry->status = TASK_RELEASE;

			spin_unlock_irqrestore(&cedar_spin_lock, flags);
			mod_timer(&cedar_devp->cedar_engine_timer, jiffies + 0);
			return 0;
		}
	}
	spin_unlock_irqrestore(&cedar_spin_lock, flags);

	return -1;
}

int cedardev_check_delay(int check_prio)
{
	struct cedarv_engine_task *task_entry;
	int timeout_total = 0;
	unsigned long flags;
	
	spin_lock_irqsave(&cedar_spin_lock, flags);
	list_for_each_entry(task_entry, &run_task_list, list) {
		if ((task_entry->t.task_prio >= check_prio) || (task_entry->running == 1) || (task_entry->is_first_task == 1))							
			timeout_total = timeout_total + task_entry->t.frametime;
	}

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
#ifdef CEDAR_DEBUG
	printk("%s,%d,%d\n", __func__, __LINE__, timeout_total);
#endif
	return timeout_total;
}

static void cedar_engine_for_timer_rel(unsigned long arg)
{
	unsigned long flags;
	int ret = 0;
	spin_lock_irqsave(&cedar_spin_lock, flags);		
	
	if(list_empty(&run_task_list)){
		ret = disable_cedar_hw_clk(); 
		if (ret < 0) {
			printk("Warring: cedar clk disable somewhere error!\n");
		}
	} else {
		printk("Warring: cedar engine timeout for clk disable, but task left, something wrong?\n");
		mod_timer( &cedar_devp->cedar_engine_timer, jiffies + msecs_to_jiffies(TIMER_CIRCLE));
	}

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

static void cedar_engine_for_events(unsigned long arg)
{
	struct cedarv_engine_task *task_entry, *task_entry_tmp;
	struct siginfo info;
	unsigned long flags;

	spin_lock_irqsave(&cedar_spin_lock, flags);		

	list_for_each_entry_safe(task_entry, task_entry_tmp, &run_task_list, list) {
		mod_timer(&cedar_devp->cedar_engine_timer_rel, jiffies + msecs_to_jiffies(CLK_REL_TIME));
		if (task_entry->status == TASK_RELEASE || 
				time_after(jiffies, task_entry->t.timeout)) {
			if (task_entry->status == TASK_INIT)
				task_entry->status = TASK_TIMEOUT;
			list_move(&task_entry->list, &del_task_list);	
		}
	}

	list_for_each_entry_safe(task_entry, task_entry_tmp, &del_task_list, list) {		
		info.si_signo = SIG_CEDAR;
		info.si_code = task_entry->t.ID;
		if (task_entry->status == TASK_TIMEOUT){
			info.si_errno = TASK_TIMEOUT;			
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		}else if(task_entry->status == TASK_RELEASE){
			info.si_errno = TASK_RELEASE;			
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		}
		list_del(&task_entry->list);
		kfree(task_entry);
	}

	if(!list_empty(&run_task_list)){
		task_entry = list_entry(run_task_list.next, struct cedarv_engine_task, list);
		if(task_entry->running == 0){
			task_entry->running = 1;
			info.si_signo = SIG_CEDAR;
			info.si_code = task_entry->t.ID;
			info.si_errno = TASK_INIT;
			send_sig_info(SIG_CEDAR, &info, task_entry->task_handle);
		}

		mod_timer( &cedar_devp->cedar_engine_timer, jiffies + msecs_to_jiffies(TIMER_CIRCLE));
	}

	spin_unlock_irqrestore(&cedar_spin_lock, flags);
}

/*
 * ioctl function
 * including : wait video engine done,
 *             AVS Counter control,
 *             Physical memory control,
 *             module clock/freq control.
 *				cedar engine
 */ 
long cedardev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long   ret = 0;	
	int ve_timeout = 0;
	int wait_ret = 0;
	//struct cedar_dev *devp;
#ifdef USE_CEDAR_ENGINE
	int rel_taskid = 0;
	struct __cedarv_task task_ret;
	struct cedarv_engine_task *task_ptr = NULL;
#endif
	unsigned long flags;
	struct ve_info *info;
	
	info = filp->private_data;

	//devp = filp->private_data;

	switch (cmd)
	{
   		case IOCTL_ENGINE_REQ:
   		#ifdef USE_CEDAR_ENGINE
			if(copy_from_user(&task_ret, (void __user*)arg, sizeof(struct __cedarv_task))){
				printk("IOCTL_ENGINE_REQ copy_from_user fail\n");
				return -EFAULT;
			}
			spin_lock_irqsave(&cedar_spin_lock, flags);

			if(!list_empty(&run_task_list) && ( task_ret.block_mode == CEDAR_NONBLOCK_TASK)){
				spin_unlock_irqrestore(&cedar_spin_lock, flags);
				return CEDAR_RUN_LIST_NONULL;
			}
			spin_unlock_irqrestore(&cedar_spin_lock, flags);
					
			task_ptr = kmalloc(sizeof(struct cedarv_engine_task), GFP_KERNEL);
			if(!task_ptr){
				printk("get mem for IOCTL_ENGINE_REQ\n");
				return PTR_ERR(task_ptr);
			}
			task_ptr->task_handle = current;
			task_ptr->t.ID = task_ret.ID;
			task_ptr->t.timeout = jiffies + msecs_to_jiffies(1000*task_ret.timeout);//ms to jiffies
			task_ptr->t.frametime = task_ret.frametime;
			task_ptr->t.task_prio = task_ret.task_prio;
			task_ptr->running = 0;
			task_ptr->is_first_task = 0;
			task_ptr->status = TASK_INIT;

			cedardev_insert_task(task_ptr);
		
			ret = enable_cedar_hw_clk();
			if (ret < 0) {
				printk("Warring: cedar clk enable somewhere error!\n");
				return -EFAULT;
			}
			return task_ptr->is_first_task;		
		#else
			enable_cedar_hw_clk();
			cedar_devp->ref_count++; 
			break;
		#endif	
    	case IOCTL_ENGINE_REL:
    	#ifdef USE_CEDAR_ENGINE 
			rel_taskid = (int)arg;		
			
			ret = cedardev_del_task(rel_taskid);					
		#else
			ret = disable_cedar_hw_clk();
			if (ret < 0) {
				printk("Warring: cedar clk disable somewhere error!\n");
				return -EFAULT;
			}
			cedar_devp->ref_count--;
		#endif
			return ret;
		case IOCTL_ENGINE_CHECK_DELAY:		
			{
	            struct cedarv_engine_task_info task_info;

	            if(copy_from_user(&task_info, (void __user*)arg, sizeof(struct cedarv_engine_task_info))){
					printk("IOCTL_ENGINE_CHECK_DELAY copy_from_user fail\n");
					return -EFAULT;
				}
				task_info.total_time = cedardev_check_delay(task_info.task_prio);
				#ifdef CEDAR_DEBUG
				printk("%s,%d,%d\n", __func__, __LINE__, task_info.total_time);
				#endif
				task_info.frametime = 0;
				spin_lock_irqsave(&cedar_spin_lock, flags);
				if(!list_empty(&run_task_list)){
					
					struct cedarv_engine_task *task_entry;
					#ifdef CEDAR_DEBUG
					printk("%s,%d\n",__func__,__LINE__);
					#endif
					task_entry = list_entry(run_task_list.next, struct cedarv_engine_task, list);
					if(task_entry->running == 1)
						task_info.frametime = task_entry->t.frametime;
					#ifdef CEDAR_DEBUG
					printk("%s,%d,%d\n",__func__,__LINE__,task_info.frametime);
					#endif
				}
				spin_unlock_irqrestore(&cedar_spin_lock, flags);

				if (copy_to_user((void *)arg, &task_info, sizeof(struct cedarv_engine_task_info))){
	            	printk("IOCTL_ENGINE_CHECK_DELAY copy_to_user fail\n");
	                return -EFAULT;
	            }
        	}
			break;
        case IOCTL_WAIT_VE_DE:            
            ve_timeout = (int)arg;
            cedar_devp->de_irq_value = 0;

            spin_lock_irqsave(&cedar_spin_lock, flags);
            if(cedar_devp->de_irq_flag)
            	cedar_devp->de_irq_value = 1;
            spin_unlock_irqrestore(&cedar_spin_lock, flags);
            
			//wait_event_interruptible_timeout(wait_ve, cedar_devp->de_irq_flag, ve_timeout*HZ);
			wait_ret = wait_event_timeout(wait_ve, cedar_devp->de_irq_flag, ve_timeout*HZ);
			cedar_devp->de_irq_flag = 0;
			return cedar_devp->de_irq_value;
			
		case IOCTL_WAIT_VE_EN:             
            ve_timeout = (int)arg;
            cedar_devp->en_irq_value = 0;
            
            spin_lock_irqsave(&cedar_spin_lock, flags);
            if(cedar_devp->en_irq_flag)
            	cedar_devp->en_irq_value = 1;
            spin_unlock_irqrestore(&cedar_spin_lock, flags);
            
            wait_event_interruptible_timeout(wait_ve, cedar_devp->en_irq_flag, ve_timeout*HZ);            
	        cedar_devp->en_irq_flag = 0;	

			return cedar_devp->en_irq_value;

#if defined CONFIG_ARCH_SUN8IW8P1

        case IOCTL_WAIT_JPEG_DEC:            
            ve_timeout = (int)arg;
            cedar_devp->jpeg_irq_value = 0;
            
            spin_lock_irqsave(&cedar_spin_lock, flags);
            if(cedar_devp->jpeg_irq_flag)
            	cedar_devp->jpeg_irq_value = 1;
            spin_unlock_irqrestore(&cedar_spin_lock, flags);
            
            wait_event_interruptible_timeout(wait_ve, cedar_devp->jpeg_irq_flag, ve_timeout*HZ);            
	        cedar_devp->jpeg_irq_flag = 0;	
			return cedar_devp->jpeg_irq_value;	
#endif
		case IOCTL_ENABLE_VE:
            if (clk_prepare_enable(ve_moduleclk)) {
            	printk("try to enable ve_moduleclk failed!\n");
            }
			break;

		case IOCTL_DISABLE_VE:
			if ((NULL == ve_moduleclk)||IS_ERR(ve_moduleclk)) {
				printk("ve_moduleclk is invalid, just return!\n");
				return -EFAULT;
			} else {
				clk_disable_unprepare(ve_moduleclk);
			}
			break;

		case IOCTL_RESET_VE:
            sunxi_periph_reset_assert(ve_moduleclk);
			sunxi_periph_reset_deassert(ve_moduleclk);
			break;
	
		case IOCTL_SET_VE_FREQ:	
			{
				int arg_rate = (int)arg;
				
#if 0
				int v_div = 0;
				v_div = (ve_parent_clk_rate/1000000 + (arg_rate-1))/arg_rate;
				if (v_div <= 8 && v_div >= 1) {
					if (clk_set_rate(ve_moduleclk, ve_parent_clk_rate/v_div)) {
						/*
						* while set the rate fail, don't return the fail value,
						* we can still set the other rate of ve module clk.
						*/
						printk("try to set ve_rate fail\n");
					}
				}
#else
				if(arg_rate >= VE_CLK_LOW_WATER && 
						arg_rate <= VE_CLK_HIGH_WATER &&
						clk_get_rate(ve_moduleclk)/1000000 != arg_rate) {
					if(!clk_set_rate(ve_parent_pll_clk, arg_rate*1000000)) {
						ve_parent_clk_rate = clk_get_rate(ve_parent_pll_clk);
						if(clk_set_rate(ve_moduleclk, ve_parent_clk_rate)) {
							printk("set ve clock failed\n");
						}

					} else {
						printk("set pll4 clock failed\n");
					}
				}
				ret = clk_get_rate(ve_moduleclk);
#endif
				break;
			}
        case IOCTL_GETVALUE_AVS2:
        case IOCTL_ADJUST_AVS2:	    
        case IOCTL_ADJUST_AVS2_ABS:
        case IOCTL_CONFIG_AVS2:
        case IOCTL_RESET_AVS2:
        case IOCTL_PAUSE_AVS2:
        case IOCTL_START_AVS2:
			// printk("do not supprot this ioctrl now\n");
			break;

        case IOCTL_GET_ENV_INFO:
        {
            struct cedarv_env_infomation env_info;
 
			env_info.phymem_start = 0; // do not use this interface ,ve get phy mem form ion now
            env_info.phymem_total_size = 0;//ve_size = 0x04000000 
	        env_info.address_macc = 0;
            if (copy_to_user((char *)arg, &env_info, sizeof(struct cedarv_env_infomation)))
                return -EFAULT;
        }
        break;
        case IOCTL_GET_IC_VER:
        {        	
			return 0;
        }
        case IOCTL_SET_REFCOUNT:
        	cedar_devp->ref_count = (int)arg;
        break;
		case IOCTL_SET_VOL:
		{

#if defined CONFIG_ARCH_SUN9IW1P1
			int ret;
			int vol = (int)arg;

			if (down_interruptible(&cedar_devp->sem)) {
				return -ERESTARTSYS;
			}
			info->set_vol_flag = 1;

			//set output voltage to arg mV
			ret = regulator_set_voltage(regu,vol*1000,3300000);
			if(IS_ERR(regu )) {
				printk("some error happen, fail to set axp15_dcdc4 regulator voltage!\n");
			}

			//printk("set voltage value vol: %d(mv)\n", vol);
			up(&cedar_devp->sem);
#endif
			break;
		}
        default:
			return -1;
    }
    return ret;
}

static int cedardev_open(struct inode *inode, struct file *filp)
{
	//struct cedar_dev *devp;
	struct ve_info *info;

	info = kmalloc(sizeof(struct ve_info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->set_vol_flag = 0;

	//devp = container_of(inode->i_cdev, struct cedar_dev, cdev);
	filp->private_data = info;
	if (down_interruptible(&cedar_devp->sem)) {
		return -ERESTARTSYS;
	}
	
	/* init other resource here */
    cedar_devp->de_irq_flag = 0;
    cedar_devp->en_irq_flag = 0;
	cedar_devp->jpeg_irq_flag = 0;
	up(&cedar_devp->sem);
	nonseekable_open(inode, filp);	
	return 0;
}

static int cedardev_release(struct inode *inode, struct file *filp)
{   
	//struct cedar_dev *devp;
	struct ve_info *info;
	//int ret = 0;
	
	info = filp->private_data;
	
	if (down_interruptible(&cedar_devp->sem)) {
		return -ERESTARTSYS;
	}

#if defined CONFIG_ARCH_SUN9IW1P1

	if(info->set_vol_flag ==1) {
		regulator_set_voltage(regu,900000,3300000);
		if(IS_ERR(regu )) {
			printk("some error happen, fail to set axp15_dcdc4 regulator voltage!\n");
			return -EINVAL;
		}
	}
#endif
	
	/* release other resource here */
    cedar_devp->de_irq_flag = 1;
    cedar_devp->en_irq_flag = 1;
	cedar_devp->jpeg_irq_flag = 1;
	up(&cedar_devp->sem);

	kfree(info);
	return 0;
}

void cedardev_vma_open(struct vm_area_struct *vma)
{	
} 

void cedardev_vma_close(struct vm_area_struct *vma)
{	
}

static struct vm_operations_struct cedardev_remap_vm_ops = {
    .open  = cedardev_vma_open,
    .close = cedardev_vma_close,
};

static int cedardev_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long temp_pfn;
    //unsigned int  VAddr;
	//struct iomap_para addrs;

    //VAddr = vma->vm_pgoff << 12;
	//addrs = cedar_devp->iomap_addrs;

    temp_pfn = MACC_REGS_BASE >> 12;

    /* Set reserved and I/O flag for the area. */
    vma->vm_flags |= VM_RESERVED | VM_IO;

    /* Select uncached access. */
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    if (io_remap_pfn_range(vma, vma->vm_start, temp_pfn,
                           vma->vm_end - vma->vm_start, vma->vm_page_prot)) {
        return -EAGAIN;
    }

    vma->vm_ops = &cedardev_remap_vm_ops;
    cedardev_vma_open(vma);
    
    return 0; 
}

#ifdef CONFIG_PM
static int snd_sw_cedar_suspend(struct platform_device *pdev,pm_message_t state)
{	
	int ret = 0;

	printk("[cedar] standby suspend\n");
	ret = disable_cedar_hw_clk();

#if defined CONFIG_ARCH_SUN9IW1P1
	clk_disable_unprepare(ve_power_gating);
#endif

	if (ret < 0) {
		printk("Warring: cedar clk disable somewhere error!\n");
		return -EFAULT;
	}

	return 0;
}

static int snd_sw_cedar_resume(struct platform_device *pdev)
{
	int ret = 0;
	
	printk("[cedar] standby resume\n");

#if defined CONFIG_ARCH_SUN9IW1P1
	clk_prepare_enable(ve_power_gating);
#endif

	if(cedar_devp->ref_count == 0){
		return 0;
	}

	ret = enable_cedar_hw_clk();
	if (ret < 0) {
		printk("Warring: cedar clk enable somewhere error!\n");
		return -EFAULT;
	}
	return 0;
}
#endif

static struct file_operations cedardev_fops = {
    .owner   = THIS_MODULE,
    .mmap    = cedardev_mmap,
    .open    = cedardev_open,
    .release = cedardev_release,
	.llseek  = no_llseek,
    .unlocked_ioctl   = cedardev_ioctl,
};

static int cedardev_init(void)
{
	int ret = 0;
	int devno;
	unsigned int val;
	dev_t dev = 0;
	printk("[cedar]: install start!!!\n");
	
	/*register or alloc the device number.*/
	if (g_dev_major) {
		dev = MKDEV(g_dev_major, g_dev_minor);	
		ret = register_chrdev_region(dev, 1, "cedar_dev");
	} else {
		ret = alloc_chrdev_region(&dev, g_dev_minor, 1, "cedar_dev");
		g_dev_major = MAJOR(dev);
		g_dev_minor = MINOR(dev);
	}

	if (ret < 0) {
		printk(KERN_WARNING "cedar_dev: can't get major %d\n", g_dev_major);
		return ret;
	}
	spin_lock_init(&cedar_spin_lock);
	cedar_devp = kmalloc(sizeof(struct cedar_dev), GFP_KERNEL);
	if (cedar_devp == NULL) {
		printk("malloc mem for cedar device err\n");
		return -ENOMEM;
	}		
	memset(cedar_devp, 0, sizeof(struct cedar_dev));
	cedar_devp->irq = SUNXI_IRQ_VE;

	sema_init(&cedar_devp->sem, 1);
	init_waitqueue_head(&cedar_devp->wq);	

	memset(&cedar_devp->iomap_addrs, 0, sizeof(struct iomap_para));

    ret = request_irq(SUNXI_IRQ_VE, VideoEngineInterupt, 0, "cedar_dev", NULL);
    if (ret < 0) {
        printk("request irq err\n");
        return -EINVAL;
    }
	
	/* map for macc io space */
    cedar_devp->iomap_addrs.regs_macc = ioremap(MACC_REGS_BASE, 4096);
    if (!cedar_devp->iomap_addrs.regs_macc){
        printk("cannot map region for macc");
    }

#if (defined CONFIG_ARCH_SUN8IW1P1) || (defined CONFIG_ARCH_SUN8IW3P1)

	/*VE_SRAM mapping to AC320*/
	val = readl(0xf1c00000);
	val &= 0x80000000;
	writel(val,0xf1c00000);	
	
	/*remapping SRAM to MACC for codec test*/
	val = readl(0xf1c00000);
	val |= 0x7fffffff;
	writel(val,0xf1c00000);

	//clear bootmode bit for give sram to sram from system on aw1650 only
	val = readl(0xf1c00004);
	val &= 0xfeffffff;
	writel(val,0xf1c00004);

	ve_parent_pll_clk = clk_get(NULL, "pll4");
	if ((!ve_parent_pll_clk)||IS_ERR(ve_parent_pll_clk)) {
		printk("try to get ve_parent_pll_clk fail\n");
		return -EINVAL;
	}
	
#elif defined CONFIG_ARCH_SUN9IW1P1

		ve_power_gating = clk_get(NULL, "cpurvddve");
		if ((!ve_power_gating)||IS_ERR(ve_power_gating)) {
			printk("try to get ve_power_gating fail\n");
			return -EINVAL;
		}

		clk_prepare_enable(ve_power_gating);
		
		regu = regulator_get(NULL,"axp15_dcdc4");
		if(IS_ERR(regu )) {
				 printk("some error happen, fail to get axp15_dcdc4 regulator!\n");
				return -EINVAL;
		}

		//set output voltage to 0.9V
		ret = regulator_set_voltage(regu,900000,3300000);
		if(IS_ERR(regu )) {
				 printk("some error happen, fail to set axp15_dcdc4 regulator voltage!\n");
				return -EINVAL;
		}
	
#if 0	
		//enable regulator
		ret = regulator_enable(regu);
		if(IS_ERR(regu )) {
				 printk("some error happen, fail to enable regulator !"\n);
				return -EINVAL;
		}
#endif
	//VE_SRAM mapping to AC320
	val = sunxi_smc_readl((void __iomem *)0xf0800000);
	val &= 0x80000000;
	sunxi_smc_writel(val,(void __iomem *)0xf0800000);
		
	//remapping SRAM to MACC for codec test
	val = sunxi_smc_readl((void __iomem *)0xf0800000);
	val |= 0x7fffffff;
	sunxi_smc_writel(val, (void __iomem *)0xf0800000);

	ve_parent_pll_clk = clk_get(NULL, "pll5");
	if ((!ve_parent_pll_clk)||IS_ERR(ve_parent_pll_clk)) {
		printk("try to get ve_parent_pll_clk fail\n");
		return -EINVAL;
	}

#elif defined CONFIG_ARCH_SUN8IW5P1

	/*VE_SRAM mapping to AC320*/
	val = readl(0xf1c00000);
	val &= 0x80000000;
	writel(val,0xf1c00000);	
	
	/*remapping SRAM to MACC for codec test*/
	val = readl(0xf1c00000);
	val |= 0x7fffffff;
	writel(val,0xf1c00000);

	ve_parent_pll_clk = clk_get(NULL, "pll_ve");
	if ((!ve_parent_pll_clk)||IS_ERR(ve_parent_pll_clk)) {
		printk("try to get ve_parent_pll_clk fail\n");
		return -EINVAL;
	}
	
#elif defined CONFIG_ARCH_SUN8IW6P1
	
	/*VE_SRAM mapping to AC320*/
	val = sunxi_smc_readl((void __iomem *)0xf1c00000);
	val &= 0x80000000;
	sunxi_smc_writel(val, (void __iomem *)0xf1c00000);	
	
	/*remapping SRAM to MACC for codec test*/
	val = sunxi_smc_readl((void __iomem *)0xf1c00000);
	val |= 0x7fffffff;
	sunxi_smc_writel(val, (void __iomem *)0xf1c00000);
	
	//clear bootmode bit for give sram to sram from system on aw1650 only
	val = sunxi_smc_readl((void __iomem *)0xf1c00004);
	val &= 0xfeffffff;
	sunxi_smc_writel(val, (void __iomem *)0xf1c00004);

	ve_parent_pll_clk = clk_get(NULL, "pll_ve");
	if ((!ve_parent_pll_clk)||IS_ERR(ve_parent_pll_clk)) {
		printk("try to get ve_parent_pll_clk fail\n");
		return -EINVAL;
	}

#elif ((defined CONFIG_ARCH_SUN8IW7P1) || (defined CONFIG_ARCH_SUN8IW8P1) || (defined CONFIG_ARCH_SUN8IW9P1))

	/*VE_SRAM mapping to AC320*/
	val = sunxi_smc_readl((void __iomem *)0xf1c00000);
	val &= 0x80000000;
	sunxi_smc_writel(val, (void __iomem *)0xf1c00000); 

	/*remapping SRAM to MACC for codec test*/
	val = sunxi_smc_readl((void __iomem *)0xf1c00000);
	val |= 0x7fffffff;
	sunxi_smc_writel(val, (void __iomem *)0xf1c00000);

	//clear bootmode bit for give sram to ve
	val = sunxi_smc_readl((void __iomem *)0xf1c00004);
	val &= 0xfeffffff;
	sunxi_smc_writel(val, (void __iomem *)0xf1c00004);

	ve_parent_pll_clk = clk_get(NULL, "pll_ve");
	if ((!ve_parent_pll_clk)||IS_ERR(ve_parent_pll_clk)) {
		printk("try to get ve_parent_pll_clk fail\n");
		return -EINVAL;
	}
#else

	#error "Unknown chip type!"
#endif

	ve_moduleclk=clk_get(NULL,"ve");
	if(!ve_moduleclk || IS_ERR(ve_moduleclk))
	{
		printk("get ve_moduleclk failed; \n");
	}

	// no reset ve module
	sunxi_periph_reset_deassert(ve_moduleclk);

	clk_prepare(ve_moduleclk);

	/* Create char device */
	devno = MKDEV(g_dev_major, g_dev_minor);	
	cdev_init(&cedar_devp->cdev, &cedardev_fops);
	cedar_devp->cdev.owner = THIS_MODULE;
	cedar_devp->cdev.ops = &cedardev_fops;
	ret = cdev_add(&cedar_devp->cdev, devno, 1);
	if (ret) {
		printk(KERN_NOTICE "Err:%d add cedardev", ret);	
	}
    cedar_devp->class = class_create(THIS_MODULE, "cedar_dev");
    cedar_devp->dev   = device_create(cedar_devp->class, NULL, devno, NULL, "cedar_dev");

    setup_timer(&cedar_devp->cedar_engine_timer, cedar_engine_for_events, (unsigned long)cedar_devp);
	setup_timer(&cedar_devp->cedar_engine_timer_rel, cedar_engine_for_timer_rel, (unsigned long)cedar_devp);
	printk("[cedar]: install end!!!\n");
	return 0;
}


static void cedardev_exit(void)
{
	dev_t dev;
	dev = MKDEV(g_dev_major, g_dev_minor);

    free_irq(SUNXI_IRQ_VE, NULL);
	iounmap(cedar_devp->iomap_addrs.regs_macc);
//	iounmap(cedar_devp->iomap_addrs.regs_avs);
	/* Destroy char device */
	if (cedar_devp) {
		cdev_del(&cedar_devp->cdev);
		device_destroy(cedar_devp->class, dev);
		class_destroy(cedar_devp->class);
	}
	
	if (NULL == ve_moduleclk || IS_ERR(ve_moduleclk)) {
		printk("ve_moduleclk handle is invalid, just return!\n");
	} else {
		clk_disable_unprepare(ve_moduleclk);
		clk_put(ve_moduleclk);
		ve_moduleclk = NULL;
	}


	if (NULL == ve_parent_pll_clk || IS_ERR(ve_parent_pll_clk)) {
		printk("ve_parent_pll_clk handle is invalid, just return!\n");
	} else {	
		clk_put(ve_parent_pll_clk);
	}
	

#if	0
	//diable regulator
	ret = regulator_disable(regu);
	if(IS_ERR(regu )) {
			 printk("some error happen, fail to disable regulator !\n");
	}
#endif
	

#if defined CONFIG_ARCH_SUN9IW1P1

	//put regulator when module exit
	regulator_put(regu);

	if (NULL == ve_power_gating || IS_ERR(ve_power_gating)) {
		printk("ve_power_gating handle is invalid, just return!\n");
	} else {
		clk_disable_unprepare(ve_power_gating);
		clk_put(ve_power_gating);
		ve_power_gating = NULL;
	}
#endif

	unregister_chrdev_region(dev, 1);	
//  platform_driver_unregister(&sw_cedar_driver);
	if (cedar_devp) {
		kfree(cedar_devp);
	}
}

//module_init(cedardev_init);
//module_exit(cedardev_exit);
static int __devexit sunxi_cedar_remove(struct platform_device *pdev)
{
	cedardev_exit();
	return 0;
}

static int __devinit sunxi_cedar_probe(struct platform_device *pdev)
{
		cedardev_init();
		return 0;
}

/*share the irq no. with timer2*/
static struct resource sunxi_cedar_resource[] = {
	[0] = {
		.start = SUNXI_IRQ_VE,
		.end   = SUNXI_IRQ_VE,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device sunxi_device_cedar = {
	.name		= "sunxi-cedar",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(sunxi_cedar_resource),
	.resource	= sunxi_cedar_resource,
};

static struct platform_driver sunxi_cedar_driver = {
	.probe		= sunxi_cedar_probe,
	.remove		= __devexit_p(sunxi_cedar_remove),
#ifdef CONFIG_PM
	.suspend	= snd_sw_cedar_suspend,
	.resume		= snd_sw_cedar_resume,
#endif
	.driver		= {
		.name	= "sunxi-cedar",
		.owner	= THIS_MODULE,
	},
};

static int __init sunxi_cedar_init(void)
{
	platform_device_register(&sunxi_device_cedar);
	printk("sunxi cedar version 0.1 \n");
	return platform_driver_register(&sunxi_cedar_driver);
}

static void __exit sunxi_cedar_exit(void)
{
	platform_driver_unregister(&sunxi_cedar_driver);
}

module_init(sunxi_cedar_init);
module_exit(sunxi_cedar_exit);


MODULE_AUTHOR("Soft-Reuuimlla");
MODULE_DESCRIPTION("User mode CEDAR device interface");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);
MODULE_ALIAS("platform:cedarx-sunxi");


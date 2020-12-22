#include <linux/poll.h>
#include <linux/slab.h>
#include <asm/io.h>
#include "fd_lib.h"
#include "base_mode.h"
#include "fd_priv.h"
int base_run(struct iomap_para reg_addr,struct drammap_para_addr mem_addr,void* ptr_det_info)
{
	return base_run_sl(reg_addr,mem_addr,ptr_det_info);
}    
   
int base_get_status(struct iomap_para reg_addr)
{
	return base_get_status_sl(reg_addr);
}

long base_load_ftr(struct drammap_para_addr mem_addr,void __user* ptr_ld_file,int size,int ld_idx)
{
	switch(ld_idx)
	{
		case 0:
			
			if(copy_from_user(mem_addr.mem_ld0_addr,ptr_ld_file,size)){
				printk("base_load_ftr copy_from_user fail\n");
				return -EFAULT;
			}

			break;
		case 1:
			if(copy_from_user(mem_addr.mem_ld1_addr,ptr_ld_file,size)){
				printk("base_load_ftr copy_from_user fail\n");
				return -EFAULT;
			}
			break;
		case 2:
			if(copy_from_user(mem_addr.mem_ld2_addr,ptr_ld_file,size)){
				printk("base_load_ftr copy_from_user fail\n");
				return -EFAULT;
			}
			break;
		case 3:
			if(copy_from_user(mem_addr.mem_ld3_addr,ptr_ld_file,size)){
				printk("base_load_ftr copy_from_user fail\n");
				return -EFAULT;
			}
			break;
		case 4:
			if(copy_from_user(mem_addr.mem_ld4_addr,ptr_ld_file,size)){
				printk("base_load_ftr copy_from_user fail\n");
				return -EFAULT;
			}
			break;
		case 5:
			if(copy_from_user(mem_addr.mem_ld5_addr,ptr_ld_file,size)){
				printk("base_load_ftr copy_from_user fail\n");
				return -EFAULT;
			}
			break;
		case 6:
			if(copy_from_user(mem_addr.mem_ld6_addr,ptr_ld_file,size)){
				printk("base_load_ftr copy_from_user fail\n");
				return -EFAULT;
			}
			break;
		case 7:
			if(copy_from_user(mem_addr.mem_ld7_addr,ptr_ld_file,size)){
				printk("base_load_ftr copy_from_user fail\n");
				return -EFAULT;
			}
			break;	
	}
	return 0;
} 
 

long base_get_rtl(struct iomap_para reg_addr,struct drammap_para_addr mem_addr,void __user* ptr_user_rtl_buf,int frame_num)
{
	 void* ptr_det_rtl = kmalloc(base_get_rtl_size_sl(), GFP_KERNEL);
	 base_get_rtl_sl(reg_addr,mem_addr,(unsigned char*)(ptr_det_rtl),frame_num);
	 
	 if(copy_to_user(ptr_user_rtl_buf,ptr_det_rtl,base_get_rtl_size_sl()))
		{
				kfree(ptr_det_rtl);
	 			printk("base_get_rtl copy_to_user fail\n");
				return -EFAULT;
	  }
	  kfree(ptr_det_rtl);
//	  printk(KERN_ALERT"-----------base_get_rtl 0\n");
	 return 0;
}

void base_clear_angle_context(void)
{
	
}

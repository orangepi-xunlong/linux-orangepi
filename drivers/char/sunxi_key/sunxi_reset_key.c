#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include<linux/mm.h>   
#include<linux/cdev.h>   
#include<asm/uaccess.h>
#include <mach/sys_config.h>
#include "key_script.h"

#define  KEY_MAJOR  0
 
static reset_key_gpio_set_t sunxi_reset_key_gpio[1];
static unsigned long key_major = KEY_MAJOR; 

struct key_dev   
{  
	unsigned int reset_key_gpio_handler[1];
	struct cdev cdev;  
};  

struct key_dev   *key_devp;

static int init_gpio(void)
{
	int err =  -1;    
	err = reset_key_sys_script_get_item("reset_key_para", "reset_key_ctl",						
		(int *)&sunxi_reset_key_gpio[0],sizeof(reset_key_gpio_set_t)/sizeof(__u32));	    
	key_devp->reset_key_gpio_handler[0] =reset_key_sys_gpio_request(&sunxi_reset_key_gpio[0], 1);
	
	return err ;
}

ssize_t reset_key_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
    	int result;  
	result = reset_key_gpio_read_one_pin_value(key_devp->reset_key_gpio_handler[0],NULL);
	//printk("[debug_jason]:result is %d\n",result);
    if(put_user(result,(int*)buf)) {  
		return -EFAULT;  
	} else {  
		return sizeof(int);  
	}  
}

static ssize_t reset_key_write(struct file *filp, const char __user *buf,
size_t size, loff_t *ppos)
{
	unsigned int count = size;
	int ret = 0;
	int buf_value = 0;
	count = size ;
	if (copy_from_user(&buf_value, buf, count)) {
		ret = - EFAULT;
	} else {
		ret = count;
    }
    return ret;
}

static int reset_key_open(struct inode *inode,struct file *filp)
{
	filp->private_data = key_devp;
	init_gpio();
	return 0;  
}

static int sunxi_reset_fetch_sysconfig_para(void)
{
	int ret = -1;
	int device_used = -1;

    if(SCIRPT_ITEM_VALUE_TYPE_INT != reset_key_sys_script_get_item("reset_key_para", "reset_key_used", &device_used, 1))
	{
	      printk("rest key: script_parser_fetch fail. \n");
	      goto script_parser_fetch_err;
	}
	
    return 0;

script_parser_fetch_err:
	pr_notice("sunxi_reset_fetch_sysconfig_para fail\n");
	return ret;
}

static int reset_key_release(struct inode *inode, struct file *filp) 
{
	return 0;	  
}

struct file_operations rest_key_fops = {
	.owner = THIS_MODULE,
	.read  = reset_key_read,
	.write = reset_key_write,
    .open  = reset_key_open,
	.release = reset_key_release,
};

static void key_setup_cdev(struct key_dev *dev, int index)  
{  
    int err,devno = MKDEV(key_major,index);   
    cdev_init(&dev->cdev,&rest_key_fops);  
    dev->cdev.owner = THIS_MODULE;  
    err = cdev_add(&dev->cdev,devno,1);  
    if(err) {  
        printk(KERN_NOTICE "Error %d adding %d\n",err,index);  
    }  
}  

struct class *key_class;
static int __init reset_key_init(void)
{
	int result;  
	
	dev_t devno = MKDEV(key_major,0);    

	if( ( result = sunxi_reset_fetch_sysconfig_para()) < 0)
	{		
		printk("warning: ir_cut_fetch_sysconfig_para fail \n");         
		return -1; 	
	}
	if(key_major) {
//		printk(" rest_key register \n");
		result = register_chrdev_region(devno,1,"rest_key");  
	}else {  
			result = alloc_chrdev_region(&devno,0,1,"rest_key");  
			key_major = MAJOR(devno);  
//			printk(" rest_key alloc_chrdev \n");
	}  
	if(result < 0) {  
		printk(" id_init register failed!");  
		return result;	
	}  
	key_devp =(struct key_dev*)kmalloc(sizeof(struct key_dev),GFP_KERNEL);	
	if(!key_devp) {  
			result = -ENOMEM;  
			printk(" chrdev no memory fail\n");
			unregister_chrdev_region(devno,1);	
			return result;
	}  
	memset(key_devp, 0 ,sizeof(struct key_dev));  
	key_setup_cdev(key_devp,0);  
	key_class = class_create(THIS_MODULE, "rest_key");
	if(IS_ERR(key_class)) {
        printk("Err: failed in creating class\n");
        return -1; 
	}
	device_create(key_class, NULL, MKDEV(key_major, 0),  NULL, "rest_key");
	
    return 0;  
}

static void __exit reset_key_exit(void)
{
	if(key_devp)
		cdev_del(&key_devp->cdev);
	device_destroy(key_class, MKDEV(key_major, 0));        
    class_destroy(key_class);                               

	if(key_devp)
		kfree(key_devp);
	unregister_chrdev_region(MKDEV(key_major,0),1);
}

module_init(reset_key_init);
module_exit(reset_key_exit);
MODULE_DESCRIPTION("reset key driver");
MODULE_AUTHOR("jason_yin");
MODULE_LICENSE("GPL");

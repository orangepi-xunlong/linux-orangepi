
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include<linux/mm.h>   
#include<linux/cdev.h>   
#include<asm/uaccess.h>   

#define  ID_MAJOR  0
#define ss_read_w(n)   		(*((volatile u32 *)(n)))          
#define SS_BASE				 (0x01c15000 + 0xf0000000)
static unsigned long id_major = ID_MAJOR; 
struct id_dev   
{  
	struct cdev cdev;  
};  

struct id_dev   *id_devp;  
unsigned int ss_id(void)
{
	unsigned int reg_val;
    reg_val = ss_read_w(SS_BASE);
	reg_val >>=16;
	reg_val &=0x7;
	return reg_val;	
}

ssize_t id_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
    int result;  
    result = ss_id();
    if(put_user(result,(int*)buf)) {  
		return -EFAULT;  
	} else {  
		return sizeof(int);  
	}  
}

static ssize_t id_write(struct file *filp, const char __user *buf,
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

static int id_open(struct inode *inode,struct file *filp)
{
	filp->private_data = id_devp;
	return 0;  
}

static int id_release(struct inode *inode, struct file *filp) 
{
	return 0;	  
}

struct file_operations id_fops = {
	.owner = THIS_MODULE,
	.read  = id_read,
	.write = id_write,
    .open  = id_open,
	.release = id_release,
};

static void id_setup_cdev(struct id_dev *dev, int index)  
{  
    int err,devno = MKDEV(id_major,index);  
    cdev_init(&dev->cdev,&id_fops);  
    dev->cdev.owner = THIS_MODULE;  
    err = cdev_add(&dev->cdev,devno,1);  
    if(err) {  
        printk(KERN_NOTICE "Error %d adding %d\n",err,index);  
    }  
}  

struct class *id_class;
static int __init id_init(void)
{
	int result;  
	
	dev_t devno = MKDEV(id_major,0);  
	if(id_major) {
		printk(" id_init register \n");
		result = register_chrdev_region(devno,1,"ifm");  
	}else {  
			result = alloc_chrdev_region(&devno,0,1,"ifm");  
			id_major = MAJOR(devno);  
//			printk(" idt_init alloc_chrdev \n");
	}  
	if(result < 0) {  
		printk(" id_init register failed!");  
		return result;	
	}  
	id_devp =(struct id_dev*)kmalloc(sizeof(struct id_dev),GFP_KERNEL);	
	if(!id_devp) {  
			result = -ENOMEM;  
			printk(" chrdev no memory fail\n");
			unregister_chrdev_region(devno,1);	
			return result;
	}  
	memset(id_devp, 0 ,sizeof(struct id_dev));  
	id_setup_cdev(id_devp,0);  
	id_class = class_create(THIS_MODULE, "ifm");
	if(IS_ERR(id_class)) {
        printk("Err: failed in creating class\n");
        return -1; 
	}
	device_create(id_class, NULL, MKDEV(id_major, 0),  NULL, "ifm");
	
    return 0;  
}

static void __exit id_exit(void)
{
	if(id_devp)
		cdev_del(&id_devp->cdev);
	device_destroy(id_class, MKDEV(id_major, 0));        
    class_destroy(id_class);                               

	if(id_devp)
		kfree(id_devp);
	unregister_chrdev_region(MKDEV(id_major,0),1);
}


module_init(id_init);
module_exit(id_exit);
MODULE_DESCRIPTION("id driver");
MODULE_AUTHOR("Richard");
MODULE_LICENSE("GPL");


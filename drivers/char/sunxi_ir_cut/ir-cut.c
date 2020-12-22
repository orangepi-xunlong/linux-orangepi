/* ir-lirc-codec.c - rc-core to classic lirc interface bridge
 *
 * Copyright (C) 2010 by richard.liu <liubaihao@sina.com>
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include "ir_cut_sys_inf.h"

#define  LED_CONTROL_NAME   "ircut"

#define KEY_BASSADDRESS_T         (0xf1c22800)
#define LRADC_BASE                KEY_BASSADDRESS_T
#define LRADC_CTRL_T              (0x00)
#define LRADC_INTC_T              (0x04)
#define LRADC_INT_STA_T           (0x08)
#define LRADC_DATA0_T             (0x0c)
#define LRADC_DATA1_T             (0x10)
#define  IR_CUT_MAJOR  0
#define read32(addr)       (*((volatile unsigned int  *)(addr)))  
#define write32(addr,value) do{*(volatile unsigned int *)(addr) = value;}while(0)

struct ir_cut_dev   
{  
	unsigned int day_night ;
	int lradc_vol_day_limit ;
    int lradc_vol_night_limit ;
	unsigned int ir_cut_gpio_handler[2];
    struct cdev cdev;  
	
};  

struct ir_cut_dev *ir_cut_devp;  
static ir_cut_gpio_set_t ir_cut_gpio[2];
static unsigned long ir_cut_major = IR_CUT_MAJOR; 
static unsigned int lradc_vol_day_limit = 15;
struct class *ir_cut_class;

void led_on(unsigned char on)
{
    if( on == 0) {
        ir_gpio_write_one_pin_value(ir_cut_devp->ir_cut_gpio_handler[0], 0, NULL);  
	}
	else {
        ir_gpio_write_one_pin_value(ir_cut_devp->ir_cut_gpio_handler[0], 1, NULL); 
	}
}

void ir_ctl_set_riseedge(void)
{
    ir_gpio_write_one_pin_value(ir_cut_devp->ir_cut_gpio_handler[1], 0, NULL); 
 //   printk("before0 data =0x%x\n",read32(PG_DATA_TEST));
	msleep(100);
    ir_gpio_write_one_pin_value(ir_cut_devp->ir_cut_gpio_handler[1], 1, NULL); 
 
}

void ir_ctl_set_fallegde(void)
{
     ir_gpio_write_one_pin_value(ir_cut_devp->ir_cut_gpio_handler[1], 1, NULL); 
//	 printk("before1 data =0x%x\n",read32(PG_DATA_TEST));
	 msleep(100);
	 ir_gpio_write_one_pin_value(ir_cut_devp->ir_cut_gpio_handler[1], 0, NULL); 
}

void ir_ctl_set_edge(int edge)
{
	if(edge == 0) {
		ir_ctl_set_fallegde();
	}
	else {
		ir_ctl_set_riseedge();
	}
}
static int ir_cut_fetch_sysconfig_para(void)
{
	int ret = -1;
	int device_used = -1;

    if(SCIRPT_ITEM_VALUE_TYPE_INT != ir_cut_sys_script_get_item("ir_cut_para", "ir_cut_used", &device_used, 1)){
	      printk("ircut: script_parser_fetch fail. \n");
	      goto script_parser_fetch_err;
	}
	
	if(1 == device_used){
		if(SCIRPT_ITEM_VALUE_TYPE_INT != ir_cut_sys_script_get_item("ir_cut_para", "lradc_vol_day_val", &lradc_vol_day_limit,  sizeof(int)/sizeof(int))){
			pr_err("%s: script_parser_fetch err. \n", __func__);
			goto script_parser_fetch_err;
		}
		
	}else{
		pr_err("%s: ir_cut_para unused. \n",  __func__);
		ret = -1;
		goto script_parser_fetch_err;
	}
	
    return 0;

script_parser_fetch_err:
	pr_notice("ir_cut_fetch_sysconfig_para fail\n");
	return ret;
}

ssize_t ir_cut_read(struct file *filp, char __user *buf, size_t count, loff_t *f_ops)
{
	int lradc_data1 = 0xff;
    lradc_data1 = read32((LRADC_DATA1_T+LRADC_BASE));
//	printk("ir_cut_read[%d]..day_night = %d, lradc_data1 = %d\n", __LINE__, ir_cut_devp->day_night, lradc_data1);
    if(1 == ir_cut_devp->day_night && lradc_data1 < 26) {
		__inf("step[19]lradc_data1 = [%d]\n", lradc_data1);
		ir_cut_devp->day_night = 0 ; /* ir cut vol <0.6 V  ,day_night = 0,°×Ìì;day_night = 1,ºÚÒ¹ */
	}
	else if(0 == ir_cut_devp->day_night && lradc_data1 > 52) {
		ir_cut_devp->day_night = 1 ; /* ir cut vol >0.8 V */
	}
	
	__inf("lradc_data1 = [%d]\n", lradc_data1);
	if(put_user(ir_cut_devp->day_night,(int*)buf)) {  
		return -EFAULT;  
	} else {  
		return sizeof(int);  
	}  
}

static ssize_t ir_cut_write(struct file *filp, const char __user *buf,
size_t size, loff_t *ppos)
{
	int ret = 0;
	int buf_value = 0;
	
//	struct ir_cut_dev *dev = filp->private_data; 
    if (copy_from_user(&buf_value, buf, size)) {
		 printk("=> ir_cut_write copy_from_user fail\n"); 
		ret = - EFAULT;
	} else {
		ret = size;
    }
	//printk("##########[debug_jason]: the buf_value = %d ######\n",buf_value);
	if(buf_value == 0) {
        ir_ctl_set_edge(1);
		led_on(0);
        __inf("=>day   riseedge \n");
	}
	else if (buf_value == 1) 
	{	
        ir_ctl_set_edge(0);
		led_on(1);
   	    __inf("=> night fallegde\n");
	}
	return ret;
}

void lradc_init(void)
{
	unsigned int reg_value = 0;
	reg_value = (1<<0)|(0<<2)|(0<<4)|(1<<6)|(1<<22) ;
	writel(reg_value, KEY_BASSADDRESS_T + LRADC_CTRL_T);	
}

void lradc_reinit(void)
{
//	writel(0, KEY_BASSADDRESS_T + LRADC_INTC_T); 	
//	writel(0 ,KEY_BASSADDRESS_T + LRADC_CTRL_T);
}

void ir_cut_gpio_init(unsigned int cfg,unsigned int pull,unsigned int drv,unsigned int data)
{  
    int err =  -1;
    err = ir_cut_sys_script_get_item("ir_cut_para", "ir_cut_led",
						(int *)&ir_cut_gpio[0],sizeof(ir_cut_gpio_set_t)/sizeof(__u32));
	
    ir_cut_devp->ir_cut_gpio_handler[0] = ir_cut_sys_gpio_request(&ir_cut_gpio[0], 1);
	if(!ir_cut_devp->ir_cut_gpio_handler[0]) {
		ir_gpio_write_one_pin_value(ir_cut_devp->ir_cut_gpio_handler[0], 0, NULL);
	}

    err = ir_cut_sys_script_get_item("ir_cut_para", "ir_cut_ctl",
						(int *)&ir_cut_gpio[1],sizeof(ir_cut_gpio_set_t)/sizeof(__u32));
	
	ir_cut_devp->ir_cut_gpio_handler[1] = ir_cut_sys_gpio_request(&ir_cut_gpio[1], 1);
	
	if(!ir_cut_devp->ir_cut_gpio_handler[1]) {
		ir_gpio_write_one_pin_value(ir_cut_devp->ir_cut_gpio_handler[1], 0, NULL);
	}
}

static int ir_cut_open(struct inode *inode,struct file *filp)
{
	static int open_flag = 0;
	printk("ir_cut_open\n");
	if(open_flag == 0) {
    	lradc_init();
	    ir_cut_gpio_init(1, 1, 2 ,1);
		open_flag = 1;
	}
    filp->private_data = ir_cut_devp;
	return 0;  
}

static int ir_cut_release(struct inode *inode, struct file *filp) 
{
    lradc_reinit();
    return 0;	  
}

struct file_operations ir_cut_fops = {
	.owner = THIS_MODULE,
	.read  = ir_cut_read,
	.write = ir_cut_write,
    .open  = ir_cut_open,
	.release = ir_cut_release,
};

static void ir_cut_setup_cdev(struct ir_cut_dev *dev, int index)  
{  
    int err,devno = MKDEV(ir_cut_major,index);  
    cdev_init(&dev->cdev,&ir_cut_fops);  
    dev->cdev.owner = THIS_MODULE;  
    err = cdev_add(&dev->cdev,devno,1);  
    if(err) {  
        printk(KERN_NOTICE "Error %d adding %d\n",err,index);  
    }  
}  

static int __init ir_cut_init(void)
{
	int result;  
	dev_t devno = MKDEV(ir_cut_major,0); 
	printk("ir_cut_init\n");
    if( ( result = ir_cut_fetch_sysconfig_para()) < 0) {
		printk("warning: ir_cut_fetch_sysconfig_para fail \n"); 
        return -1; 
	}

	if(ir_cut_major) {
		printk(" ir_cut_init register \n");
		result = register_chrdev_region(devno,1,"ircut"); 
		printk("[debug_jason]:The result is :%d",result);
	}else {  
			result = alloc_chrdev_region(&devno,0,1,"ircut");  
			ir_cut_major = MAJOR(devno);  
			printk(" ir_cut_init alloc_chrdev \n");
	}  
	if(result < 0) {  
		printk(" ir_cut_init register failed!");  
		return result;	
	}  
	ir_cut_devp =(struct ir_cut_dev*)kmalloc(sizeof(struct ir_cut_dev),GFP_KERNEL);	
	if(!ir_cut_devp) {  
			result = -ENOMEM;  
			printk(" chrdev no memory fail\n");
			unregister_chrdev_region(devno,1);	
			return result;
	}  
	memset(ir_cut_devp, 0 ,sizeof(struct ir_cut_dev));  
	ir_cut_setup_cdev(ir_cut_devp,0);  
    ir_cut_class = class_create(THIS_MODULE, "ircut");
	ir_cut_devp->lradc_vol_day_limit = lradc_vol_day_limit;
	if(IS_ERR(ir_cut_class)) {
        printk("Err: failed in creating class\n");
        return -1; 
	}
	device_create(ir_cut_class, NULL, MKDEV(ir_cut_major, 0),  NULL, "ircut");

    return 0;  
}

static void __exit ir_cut_exit(void)
{
	if(ir_cut_devp)
	cdev_del(&ir_cut_devp->cdev);
	if(ir_cut_devp)
	kfree(ir_cut_devp);
	unregister_chrdev_region(MKDEV(ir_cut_major,0),1);
}

module_init(ir_cut_init);
module_exit(ir_cut_exit);
MODULE_DESCRIPTION(" IR cut driver");
MODULE_AUTHOR("Richard");
MODULE_LICENSE("GPL");


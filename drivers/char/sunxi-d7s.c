#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/major.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/ioport.h>		/* request_region */
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/atomic.h>
#include <asm/uaccess.h>		/* put_/get_user			*/
#include <asm/io.h>
#include <mach/gpio.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <mach/hardware.h>
#include <mach/sys_config.h>

#include "sunxi-d7s.h"

#define LOCK_ADDR    
//#define INC_ADDR   

static atomic_t d7s_users = ATOMIC_INIT(0);
static DEFINE_MUTEX(d7s_mutex);
static u32 show_number = 0;
static u32 on_off = 0;
static u8 show_level = 0;
struct d7s_info info;

#define  DIN_H()  gpio_direction_output(info.din.gpio, 1)
#define  DIN_L()  gpio_direction_output(info.din.gpio, 0)
#define  CLK_H()  gpio_direction_output(info.clk.gpio, 1)
#define  CLK_L()  gpio_direction_output(info.clk.gpio, 0)
#define  STB_H()  gpio_direction_output(info.stb.gpio, 1)
#define  STB_L()  gpio_direction_output(info.stb.gpio, 0)

const u8 NUM[16]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,0x7c,0x39,0x5e,0x79,0x71};

static void GPIO_set(void)
{
	if(0 != gpio_request(info.din.gpio, NULL))
		pr_err("din gpio_request is failed,");
	else
		gpio_direction_output(info.din.gpio, 0);

	if(0 != gpio_request(info.clk.gpio, NULL))
		pr_err("clk gpio_request is failed,");
	else
		gpio_direction_output(info.clk.gpio, 0);

	if(0 != gpio_request(info.stb.gpio, NULL))
		pr_err("stb gpio_request is failed,");
	else
		gpio_direction_output(info.stb.gpio, 0);

}

static void init_TM1616(void)
{
	GPIO_set();
}

static void free_TM1616(void)
{
	gpio_free(info.din.gpio);
	gpio_free(info.clk.gpio);
	gpio_free(info.stb.gpio);
}

static void write_TM1616(u8 data)
{
	u8 data_temp = data;
	u8 i;
	for(i=0;i<8;i++)
	{
		if((data_temp&0x01)!=0)   //send from lower bit
		{
			DIN_H();
		}
		else
		{
			DIN_L();
		}
		CLK_L();        //TM1616 read data as clk positive edge 
		ndelay(1);
		CLK_H();
		ndelay(1);
		data_temp = data_temp>>1;
	}	
}

static void write_TM1616_command(u8 command)
{
	STB_H();
	ndelay(1);
	STB_L();
	ndelay(1);
	write_TM1616(command);
	ndelay(1);
	STB_H();
	ndelay(1);
}

#ifdef LOCK_ADDR
static void write_TM1616_data_mod0(u8 addr,u8 data)
{
	STB_H();
	ndelay(1);
	STB_L();
	ndelay(1);
	write_TM1616(addr);   //frist addr
	ndelay(1);         
	write_TM1616(data);   //then data
	ndelay(1);
	STB_H();
	ndelay(1);
}
#endif

#ifdef INC_ADDR
static void write_TM1616_data_mod1(u8 addr,u8 data1,u8 data2,u8 data3,u8 data4)
{
	u8 char_num;
	STB_H();
	ndelay(1);
	STB_L();
	ndelay(1);
	for(char_num=0;char_num<15;char_num++)
	{
		if(char_num == 0)
		{
			write_TM1616(addr);
		}
		else if(char_num == 1)
		{
			write_TM1616(data1);
		}
		else if(char_num == 3)
		{
			write_TM1616(data2);
		}
		else if(char_num == 5)
		{
			write_TM1616(data3);
		}
		else if(char_num == 7)
		{
			write_TM1616(data4);
		}
		else
		{
			write_TM1616(0);
		}
	}
	ndelay(1);
	STB_H();
	ndelay(1);
}
#endif

static void show_4_num(u8 data1,u8 data2,u8 data3,u8 data4)
{
	DIN_H();
	CLK_H();
	STB_H();
	ndelay(1);
	write_TM1616_command(0x00);
#ifdef LOCK_ADDR
	write_TM1616_command(0x44);
	write_TM1616_data_mod0(0xc0,NUM[data1]);
	write_TM1616_data_mod0(0xc2,NUM[data2]);
	write_TM1616_data_mod0(0xc4,NUM[data3]);
	write_TM1616_data_mod0(0xc6,NUM[data4]);
#endif

#ifdef INC_ADDR
	write_TM1616_command(0x40);
	write_TM1616_data_mod1(0xc0,NUM[data1],NUM[data2],NUM[data3],NUM[data4]);
#endif

	sunxi_d7s_display_on();
}

void sunxi_d7s_show_number(u32 number)
{
	u8 num_tab[4] = {0,0,0,0};

	num_tab[0] = number%10;
	num_tab[1] = number/10%10;
	num_tab[2] = number/100%10;
	num_tab[3] = number/1000;
	show_number = number%10000;
	show_4_num(num_tab[3],num_tab[2],num_tab[1],num_tab[0]);
}
EXPORT_SYMBOL_GPL(sunxi_d7s_show_number);

void sunxi_d7s_set_power_level(u8 level)
{
	write_TM1616_command(0x88|(level&0x07));
}
EXPORT_SYMBOL_GPL(sunxi_d7s_set_power_level);

void sunxi_d7s_display_on(void)
{
	write_TM1616_command(0x88);
	on_off = 1;
}
EXPORT_SYMBOL_GPL(sunxi_d7s_display_on);

void sunxi_d7s_display_off(void)
{
	write_TM1616_command(0x80);
	on_off = 0;
}
EXPORT_SYMBOL_GPL(sunxi_d7s_display_off);

static int d7s_open(struct inode *inode, struct file *f)
{
	atomic_inc(&d7s_users);
	return 0;
}

static int d7s_release(struct inode *inode, struct file *f)
{
	/* Reset flipped state to OBP default only if
	 * no other users have the device open and we
	 * are not operating in solaris-compat mode
	 */
	if (atomic_dec_and_test(&d7s_users)) {
		
	}

	return 0;
}

static long d7s_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	u32 number = 0;
	int error = 0;
	u8 temp = 0;

	if (D7S_MINOR != iminor(file->f_path.dentry->d_inode))
		return -ENODEV;

	mutex_lock(&d7s_mutex);
	switch (cmd) {
	case D7S_ENABLE:
		pr_info("[%s]s7d enabled\n",__func__);
		sunxi_d7s_display_on();
		break;

	case D7S_DISABLE:
		pr_info("[%s]s7d disabled\n",__func__);
		sunxi_d7s_display_off();
		break;

	case D7S_SET_POWER_LEVEL:
		if (get_user(temp, (u8 __user *) arg)) {
			error = -EFAULT;
			break;
		}
		if(temp > 7){
			pr_err("[%s]err power level %u\n",__func__, temp);
			error = -EFAULT;
			break;
		}
		sunxi_d7s_set_power_level(temp);
		break;
	case D7S_SHOW_NUMBER:
		if (get_user(number, (u32 __user *) arg)) {
			error = -EFAULT;
			break;
		}
		if(temp > 9999){
			pr_err("[%s]err show number %u\n",__func__, number);
			error = -EFAULT;
			break;
		}
		sunxi_d7s_show_number(number);
		break;
	};
	mutex_unlock(&d7s_mutex);

	return error;
}

static ssize_t number_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",show_number);
}

static ssize_t number_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	u32 number = simple_strtoul(buf, NULL, 10);
	mutex_lock(&d7s_mutex);
	if(number < 10000){
		sunxi_d7s_show_number(number);
	}
	else
		pr_err("[%s]err show number %u\n",__func__, number);
	mutex_unlock(&d7s_mutex);
	return count;
}

static ssize_t level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",show_level);
}

static ssize_t level_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	u8 level = simple_strtoul(buf, NULL, 10);
	mutex_lock(&d7s_mutex);
	if(level < 8){
		show_level = level;
		sunxi_d7s_set_power_level(show_level);
	}
	else
		pr_err("[%s]err show level %u\n",__func__, level);
	mutex_unlock(&d7s_mutex);
	return count;
}

static ssize_t on_off_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf,"%u\n",on_off);
}

static ssize_t on_off_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	u32 on = simple_strtoul(buf, NULL, 10);
	mutex_lock(&d7s_mutex);
	if(on){
		sunxi_d7s_display_on();
		on_off = 1;
	}else{
		sunxi_d7s_display_off();
		on_off = 0;
	}
	mutex_unlock(&d7s_mutex);
	return count;
}

static DEVICE_ATTR(number, S_IRUGO|S_IWUSR|S_IWGRP,
		number_show, number_store);

static DEVICE_ATTR(level, S_IRUGO|S_IWUSR|S_IWGRP,
		level_show, level_store);

static DEVICE_ATTR(on_off, S_IRUGO|S_IWUSR|S_IWGRP,
		on_off_show, on_off_store);

static struct attribute *sunxi_d7s_attributes[] = {
	&dev_attr_number.attr,
	&dev_attr_level.attr,
	&dev_attr_on_off.attr,
	NULL
};

static struct attribute_group sunxi_d7s_attribute_group = {
	.attrs = sunxi_d7s_attributes,
};


static const struct file_operations d7s_fops = {
	.owner =		THIS_MODULE,
	.unlocked_ioctl =	d7s_ioctl,
	.compat_ioctl =		d7s_ioctl,
	.open =			d7s_open,
	.release =		d7s_release,
	.llseek = noop_llseek,
};

static struct miscdevice d7s_miscdev = {
	.minor		= D7S_MINOR,
	.name		= "sunxi_d7s",
	.fops		= &d7s_fops
};

static int d7s_fetch_sysconfig(void)
{
	script_item_u   val;
	script_item_value_type_e  type;

	type = script_get_item("d7s_para", "d7s_used", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_INT != type) {
		pr_err("%s: din script_get_item err. \n",__func__ );
		goto script_get_item_err;
	}

	if(!val.val){
		pr_debug("%s: d7s not used.\n", __func__);
		return -1;
	}

	type = script_get_item("d7s_para", "din_gpio", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("%s: din script_get_item err. \n",__func__ );
		goto script_get_item_err;
	}
	info.din = val.gpio;

	type = script_get_item("d7s_para", "clk_gpio", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("%s: clk script_get_item err. \n",__func__ );
		goto script_get_item_err;
	}
	info.clk = val.gpio;

	type = script_get_item("d7s_para", "stb_gpio", &val);
	if (SCIRPT_ITEM_VALUE_TYPE_PIO != type) {
		pr_err("%s: stb script_get_item err. \n",__func__ );
		goto script_get_item_err;
	}
	info.stb = val.gpio;

	return 0;
script_get_item_err:
	pr_err("[%s]get sysconfig fail\n",__func__);
	return -1;
}

static int __init sunxi_d7s_init(void)
{
	int err = -EINVAL;
	if(d7s_fetch_sysconfig())
		goto fail;
	init_TM1616();
	err = misc_register(&d7s_miscdev);
	if (err) {
		pr_err("Unable to acquire miscdevice minor %i\n",D7S_MINOR);
		goto fail;
	}
	sysfs_create_group(&d7s_miscdev.this_device->kobj,
						 &sunxi_d7s_attribute_group);
	return 0;
fail:
	return -EINVAL;
}

static void __exit sunxi_d7s_exit(void)
{
	if(on_off)
		sunxi_d7s_display_off();
	free_TM1616();
	sysfs_remove_group(&d7s_miscdev.this_device->kobj,
						 &sunxi_d7s_attribute_group);
	misc_deregister(&d7s_miscdev);
}

module_init(sunxi_d7s_init);
module_exit(sunxi_d7s_exit);

MODULE_AUTHOR("Professor Qin");
MODULE_DESCRIPTION("Sunxi display 7 segment");
MODULE_LICENSE("GPL");


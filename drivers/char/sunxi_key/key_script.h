#ifndef _KEY_RESET_SYS_INTF_
#define _KEY_RESET_SYS_INTF_
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/module.h> 
#include <linux/slab.h> 
#include <linux/kobject.h> 
#include <mach/hardware.h>
#include <linux/gpio.h>
#include <mach/sys_config.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-sunxi.h>
#include <linux/regulator/consumer.h>
#include <asm/system.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <asm/io.h>
#include<linux/types.h>   
 
#include<linux/cdev.h>   
#include<asm/uaccess.h>   
#include <mach/hardware.h>
#include <mach/sys_config.h>

//#define RESET_KEY_DEBUG_TEST     1
#ifdef RESET_KEY_DEBUG_TEST
#define RESET_KEY_DEBUG(fmt,args...)	printk(fmt ,##args)
#define __wrn(msg...)                   do{printk(KERN_WARNING "[IR CUT WRN] file:%s,line:%d:    ",__FILE__,__LINE__);printk(msg);}while(0)
#define __inf(msg...)                   do{if(1){printk(KERN_WARNING "[IR CUT INO] ");printk(msg);}}while(0)
#else
#define __wrn(msg...)                   do {} while(0)
#define RESET_KEY_DEBUG(fmt,args...)    do {} while(0)
#define __inf(msg...)                   do {} while(0)
#endif

typedef struct
{
	char  gpio_name[32];
	int port;
	int port_num;
	int mul_sel;
	int pull;
	int drv_level;
	int data;
	int gpio;
} reset_key_gpio_set_t;

__s32  reset_key_gpio_read_one_pin_value(u32 p_handler, const char *gpio_name);
__s32  reset_key_gpio_write_one_pin_value(u32 p_handler, __u32 value_to_gpio, const char *gpio_name);

int reset_key_sys_script_get_item(char *main_name, char *sub_name, int value[], int count);
int reset_key_sys_gpio_request_simple(reset_key_gpio_set_t *gpio_list, u32 group_count_max);
int reset_key_sys_gpio_release(int p_handler, s32 if_release_to_default_status);
int reset_key_sys_gpio_request(reset_key_gpio_set_t *gpio_list, u32 group_count_max);

#endif
#ifndef _VIB_SYS_INTF_
#define _VIB_SYS_INTF_
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

#define SCRIPT_PARSER_OK (0)

//#define VIB_DEBUG_TEST     0
#ifdef VIB_DEBUG_TEST
#define VIB_DEBUG(fmt,args...)	printk(fmt ,##args)
#define __wrn(msg...)       do{printk(KERN_WARNING "[VIB WRN] file:%s,line:%d:    ",__FILE__,__LINE__);printk(msg);}while(0)
#define __inf(msg...)       do{if(1){printk(KERN_WARNING "[VIB INO] ");printk(msg);}}while(0)
#else
#define __wrn(msg...)             do {} while(0)
#define VIB_DEBUG(fmt,args...)    do {} while(0)
#define __inf(msg...)             do {} while(0)
#endif

typedef enum 
{
   VIB_M0_SELECT =0,
   VIB_M1_SELECT,
   VIB_ERR_SELECT
}vib_select;

typedef enum 
{
   VIB_ROUND_FORWARD =0,
   VIB_ROUND_BACK
}vib_direction;

typedef enum {
	SCIRPT_PARSER_VALUE_TYPE_INVALID = 0,
	SCIRPT_PARSER_VALUE_TYPE_SINGLE_WORD,
	SCIRPT_PARSER_VALUE_TYPE_STRING,
	SCIRPT_PARSER_VALUE_TYPE_MULTI_WORD,
	SCIRPT_PARSER_VALUE_TYPE_GPIO_WORD
} script_parser_value_type_t;

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
} script_gpio_set_t;

struct my_kobj  { 
	int val; 
	struct kobject kobj; 
}; 

struct my_attribute { 
	struct attribute attr; 
    ssize_t (*show)(struct my_kobj *obj, 
    struct my_attribute *attr, char *buf); 
    ssize_t (*store)(struct my_kobj *obj, 
    struct my_attribute *attr, const char *buf, size_t count); 
}; 

__s32  gpio_read_one_pin_value(u32 p_handler, const char *gpio_name);
__s32  gpio_write_one_pin_value(u32 p_handler, __u32 value_to_gpio, const char *gpio_name);
int vib_sys_script_get_item(char *main_name, char *sub_name, int value[], int count);
int vib_sys_gpio_request_simple(script_gpio_set_t *gpio_list, u32 group_count_max);
int vib_sys_gpio_release(int p_handler, s32 if_release_to_default_status);
void vib_round_times(vib_select vib_sel,vib_direction direction,int times);
void vib_round_always(vib_select vib_sel,vib_direction direction);
int vib_sys_gpio_request(script_gpio_set_t *gpio_list, u32 group_count_max);
#endif
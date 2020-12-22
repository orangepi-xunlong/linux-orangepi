#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/leds.h>
#include <linux/arisc/arisc.h>
#include "leds.h"

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_INT = 1U << 1,
	DEBUG_DATA_INFO = 1U << 2,
	DEBUG_SUSPEND = 1U << 3,
};
static u32 debug_mask = 0;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(fmt , ## arg)

module_param_named(debug_mask, debug_mask, int, 0644);

static void respiration_led_set(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	/* here */
	dprintk(DEBUG_DATA_INFO,"[%s]value:%d\n",__func__, value);
}

static int respiration_led_blink(struct led_classdev *led_cdev,
				unsigned long *delay_on,
				unsigned long *delay_off)
{
	/* here */	
	arisc_set_led_bln(0, (*delay_on)>>1, (*delay_on)>>1, *delay_off);
	dprintk(DEBUG_DATA_INFO,"[%s]on:%lu off:%lu\n",__func__, *delay_on , *delay_off);
	
	return 0;
}

static struct led_classdev respiration_lamp_dev = {
	.name			= "sunxi_respiration_lamp",
	.brightness_set		= respiration_led_set,
	.blink_set		= respiration_led_blink,
	.brightness		= LED_OFF,
	.default_trigger	= "sunxi_respiration_trigger",
};

static int __init respiration_init(void)
{
	if(led_classdev_register(NULL, &respiration_lamp_dev))
	{
		printk("[%s]led dev register fail.\n",__func__);
		return -1;
	}
	return 0;
}

static void __exit respiration_exit(void)
{
	led_classdev_unregister(&respiration_lamp_dev);
}

module_init(respiration_init);
module_exit(respiration_exit);

MODULE_AUTHOR("Qin ");
MODULE_DESCRIPTION("Sunxi respiration LED");
MODULE_LICENSE("GPL");


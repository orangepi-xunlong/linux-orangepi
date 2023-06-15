#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include "../init-input.h"
#include <linux/input.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pm_wakeirq.h>

#define SUSPEND_STATUS (1)
#define RESUME_STATUS (0)

#define DBG_EN 0
#if (DBG_EN == 1)
	#define __dbg(x, arg...) printk("[HAL_SWI_DBG]"x, ##arg)
	#define __inf(x, arg...) printk(KERN_INFO"[HAL_SWI_INF]"x, ##arg)
#else
	#define __dbg(x, arg...)
	#define __inf(x, arg...)
#endif

#define __err(x, arg...) printk(KERN_ERR"[HAL_SWI_ERR]"x, ##arg)

char phys[32];
u32 int_number;
struct gpio_config irq_gpio;
struct input_dev *input_dev;
struct work_struct  work;
static struct workqueue_struct *hal_wq;
/*static int trigger_mode;*/
static int sys_status;
static spinlock_t irq_lock;
static const char *device_name;
static int sNearKey;
static int sFarKey;
static bool isWakeSource;
/*static int resule_flag;*/
unsigned int old_level_status;

#if IS_ENABLED(CONFIG_PM)
static struct dev_pm_domain hal_pm_domain;
#endif

static unsigned int level_status; /*0:low level 1:high level*/
static unsigned int is_first = 1;

static void hall_work_func(struct work_struct *work)
{
	unsigned long irqflags = 0;
	unsigned int irq_num;
	unsigned int filter_status;

	level_status = gpio_get_value(int_number);

	__dbg("gpio value = %d\n", level_status);

	msleep(10);
	filter_status = gpio_get_value(int_number);
	if (level_status != filter_status) {
		__dbg("maybe this is noise, ignore it");
		goto end;
	}

	if ((level_status != old_level_status) || (is_first == 1)) {
		old_level_status = level_status;
		if (level_status == 1) { /*leave off*/
			__dbg("******leave off hall sensor, input keycode %d******\n", sFarKey);
			input_report_key(input_dev, sFarKey, 1);
			input_sync(input_dev);
			msleep(100);
			input_report_key(input_dev, sFarKey, 0);
			input_sync(input_dev);
		} else if (level_status == 0) {/*close*/
			__dbg("******close to hall sensor, input keycode %d******\n", sNearKey);
			input_report_key(input_dev, sNearKey, 1);
			input_sync(input_dev);
			msleep(100);
			input_report_key(input_dev, sNearKey, 0);
			input_sync(input_dev);
		}

		is_first = 0;
	 }

end:

	spin_lock_irqsave(&irq_lock, irqflags);
	irq_num = gpio_to_irq(int_number);
	enable_irq(irq_num);
	spin_unlock_irqrestore(&irq_lock, irqflags);
}

irqreturn_t switch_handle(int irq, void *dev_id)
{
	unsigned long irqflags = 0;
	unsigned int irq_num;

	__dbg("switch_handle enter\n");
	spin_lock_irqsave(&irq_lock, irqflags);
	irq_num = gpio_to_irq(int_number);
	disable_irq_nosync(irq_num);
	spin_unlock_irqrestore(&irq_lock, irqflags);

	queue_work(hal_wq, &work);

	return IRQ_HANDLED;
}

#if IS_ENABLED(CONFIG_PM)
static int sunxi_hal_suspend(struct device *dev)
{
	sys_status = SUSPEND_STATUS;
	/*old_level_status = 0;*/
	__dbg("%s,old_level_status = %d \n", __FUNCTION__, old_level_status);

	level_status = gpio_get_value(int_number);
	old_level_status = level_status;
	__dbg("%s sys_status = %d level_status = %d\n", __FUNCTION__, sys_status, level_status);

	flush_workqueue(hal_wq);
	return 0;
}

static int sunxi_hal_resume(struct device *dev)
{
	sys_status = RESUME_STATUS;

	__dbg("%s sys_status = %d \n", __FUNCTION__, sys_status);

	level_status = gpio_get_value(int_number);

	__dbg(" %s level_status = %d, old_level_status = %d , is_first = %d\n", __FUNCTION__, level_status, old_level_status, is_first);
	if (isWakeSource && (level_status == 1) && (old_level_status != level_status) && (is_first == 0)) {
		old_level_status  = level_status;
		__dbg("**********2***************\n");
		input_report_key(input_dev, sFarKey, 1);
		input_sync(input_dev);
		udelay(20);
		input_report_key(input_dev, sFarKey, 0);
		input_sync(input_dev);
	}
	if ((old_level_status != level_status) && level_status == 0) {
		old_level_status  = level_status;
		__dbg("%s 00000000000 old_level_status = %d,\n", __FUNCTION__, old_level_status);
	}

	return 0;
}
#endif

static int switch_request_irq(void)
{
	int irq_number = 0;
	int ret = 0;

	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		ret = -ENOMEM;
		__err("%s:Failed to allocate input device\n", __func__);
		return -1;
	}

	input_dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY);
	sprintf(phys, "input/switch");

	if (device_name != NULL)
		input_dev->name = device_name;
	else
		input_dev->name = "switch";
	input_dev->phys = phys;
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0002;
	input_dev->id.product = 0x0012;
	input_dev->id.version = 0x0102;
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_REL, input_dev->evbit);
	set_bit(sNearKey, input_dev->keybit);
	set_bit(sFarKey, input_dev->keybit);

#if IS_ENABLED(CONFIG_PM)
	hal_pm_domain.ops.suspend = sunxi_hal_suspend;
	hal_pm_domain.ops.resume = sunxi_hal_resume;
	input_dev->dev.pm_domain = &hal_pm_domain;
#endif
	ret = input_register_device(input_dev);
	if (ret) {
		__err("Unable to register %s input device\n", input_dev->name);
		return -1;
	}

    spin_lock_init(&irq_lock);
	/*×¢²áÖÐ¶Ï*/
	irq_number = gpio_to_irq(int_number);
    __dbg("%s,irq_number = %d\n", __FUNCTION__, irq_number);
	if (!gpio_is_valid(irq_number)) {
		__err("map gpio [%d] to virq failed, errno = %d\n",
				int_number, irq_number);
		ret = -EINVAL;
		goto switch_err;
	}

	ret = devm_request_irq(&(input_dev->dev), irq_number, switch_handle,
			       IRQF_TRIGGER_RISING|IRQF_TRIGGER_FALLING|IRQF_NO_SUSPEND, "HALL_EINT", NULL);
	if (ret) {
		__err("request virq %d failed, errno = %d\n", irq_number, ret);
		ret = -EINVAL;
		goto switch_err;
	}
	if (isWakeSource) {
		device_init_wakeup(&(input_dev->dev), 1);
		dev_pm_set_wake_irq(&input_dev->dev, irq_number);
	}
	return 0;

switch_err:
	input_unregister_device(input_dev);
	return ret;
}

static int __init switch_init(void)
{

	int ret = -1;
	struct device_node *np = NULL;

	np = of_find_node_by_name(NULL, "hall_para");
	if (!np) {
		pr_err("ERROR! get hall_para failed, func:%s, line:%d\n", __FUNCTION__, __LINE__);
		goto devicetree_get_item_err;
	}
	if (!of_device_is_available(np)) {
		pr_err("%s: hall is not used\n", __func__);
		goto devicetree_get_item_err;
	}
	int_number = of_get_named_gpio_flags(np, "hall_int_port", 0, (enum of_gpio_flags *)&irq_gpio);
	if (!gpio_is_valid(int_number)) {
		pr_err("%s: hall_int_port is invalid. \n", __func__);
		goto devicetree_get_item_err;
	}
	__dbg("%s int_number = %d \n", __FUNCTION__, int_number);
	of_property_read_string(np, "hall_name", &device_name);

	ret = of_property_read_u32(np, "near_keycode", &sNearKey);
	if (ret)
		sNearKey = KEY_SLEEP;

	ret = of_property_read_u32(np, "far_keycode", &sFarKey);
	if (ret)
		sFarKey = KEY_WAKEUP;

	isWakeSource = of_property_read_bool(np, "wakeup-source");

	__dbg("gpio int_number = %d\n", int_number);

	INIT_WORK(&work, hall_work_func);
	hal_wq = create_singlethread_workqueue("hall_wq");
	if (!hal_wq) {
		__err("%s:Creat hal_wq workqueue failed.\n", __func__);
		return 0;
	}
	flush_workqueue(hal_wq);
	ret = switch_request_irq();
	if (!ret) {
		__dbg("*********Register IRQ OK!*********\n");
	}
	return 0;

	/*add by clc*/
devicetree_get_item_err:
	__dbg("=========script_get_item_err============\n");
	return ret;
}

static void __exit switch_exit(void)
{
	unsigned int irq_num;
    if (hal_wq) {
		destroy_workqueue(hal_wq);
	}

	irq_num = gpio_to_irq(int_number);
	devm_free_irq(&(input_dev->dev), irq_num, NULL);
	input_unregister_device(input_dev);
	return;
}

late_initcall(switch_init);
module_exit(switch_exit);

MODULE_DESCRIPTION("hallswitch Driver");
MODULE_LICENSE("GPL v2");

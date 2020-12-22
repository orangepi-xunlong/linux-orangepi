#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/interrupt.h> 
#include <linux/sched.h>
#include <linux/of_device.h>

#include <asm/mach/irq.h>
#include <mach/platform.h>

static int has_init = 0;
static int sw_pin = -1;			// func: int 
static int sia_pin = -1;		// func: int
static int sib_pin = -1;		// func: input
static int sia_irq = -1;
static int sw_irq = -1;
static int counter = 0;
static int sw = 0;

static void rotary_sensor_hw_exit(void);
static int rotary_sensor_hw_init(void );


static ssize_t rotary_sensor_sw_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	if ( sw_pin != -1 && sia_pin != -1 && sib_pin != -1) {
		return sprintf(buf, "%d\n", sw);
    }
    return -1;
}

static ssize_t rotary_sensor_value_read(struct device *dev, struct device_attribute *attr, char *buf)
{
	if ( sw_pin != -1 && sia_pin != -1 && sib_pin != -1) {
	    return sprintf(buf, "%d\n", counter);
	}
	return -1;
}

static ssize_t rotary_sensor_gpio_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d,%d,%d\n", sw_pin, sia_pin, sib_pin);
}

static ssize_t rotary_sensor_gpio_store(struct device *device, struct device_attribute *attr, const char *buf, size_t count)
{
	if ( has_init ==0 ) {
		sscanf(buf,"%d,%d,%d ", &sw_pin, &sia_pin, &sib_pin);
		printk(KERN_ERR"buf=%s\n", buf);
		if (rotary_sensor_hw_init()) {
			has_init = 0;
			return -1;
		}	
		else {
			has_init = 1;
		}	
	} else {
		rotary_sensor_hw_exit();
		has_init = 0;
	}	
		
	return count;
}
static struct device_attribute value_attr = __ATTR(value, S_IRUGO, rotary_sensor_value_read, NULL);
static struct device_attribute sw_attr = __ATTR(sw,S_IRUGO, rotary_sensor_sw_read, NULL);
static struct device_attribute gpio_attr = __ATTR(gpio, S_IRUGO|S_IWUGO, rotary_sensor_gpio_show, rotary_sensor_gpio_store);

/* sys attribte group */
static struct attribute *attrs[] = {
	&value_attr.attr, &sw_attr.attr, &gpio_attr.attr, NULL,
};

static struct attribute_group attr_group = {
		.attrs = (struct attribute **)attrs,
};


static irqreturn_t sw_isr(int irq, void *data)
{	
	if (gpio_get_value(sw_pin) == 1) {
		sw = 0;
	} else {
		sw = 1;
	}
    // printk("%s sw=%d\n", __func__, sw);
    return IRQ_HANDLED;
}

static irqreturn_t rotate_isr(int irq, void *data)
{	
	if (gpio_get_value(sib_pin) == 1) {
		counter++;
	} else {
		counter--;
	}
    // printk("%s value=%d\n", __func__, counter);
    return IRQ_HANDLED;
}

static void rotary_sensor_hw_exit(void)
{
	if (sia_irq != -1)
	    free_irq(sia_irq, NULL);
	if (sw_irq != -1)    
		free_irq(sw_irq, NULL);
	if (sib_pin != -1)
		gpio_free(sib_pin);    
	if (sia_pin != -1)	
	    gpio_free(sia_pin);
	if (sw_pin != -1)    
	    gpio_free(sw_pin);	
	sw_pin = sia_pin = sib_pin = -1;    
}

static int rotary_sensor_hw_init(void )
{
	int ret = -1;

    ret = gpio_request(sw_pin, "sw");
    if (ret) {
        printk(KERN_ERR"gpio request sw_pin fail:%d\n", sw_pin);
        return -1;
    }
    ret = gpio_request(sia_pin, "sia_pin");
    if (ret) {
        printk(KERN_ERR"gpio request sia_pin fail:%d\n", sia_pin);
        goto fail_gpio_sw;
    }
    ret = gpio_request(sib_pin, "sib_pin");
    if (ret) {
        printk(KERN_ERR"gpio request sib_pin fail:%d\n", sia_pin);
        goto fail_gpio_sia;
    }

    sw_irq = gpio_to_irq(sw_pin);
    ret = request_irq(sw_irq, sw_isr, IRQ_TYPE_EDGE_BOTH, "rotary_sensor_sw", NULL);
    if (ret) {
        printk(KERN_ERR"%s fail to request_irq, gpio=%d irq=%d\n", __func__, sw_pin, sw_irq);
        goto fail_gpio_sib;
    }
    
    sia_irq = gpio_to_irq(sia_pin);
    ret = request_irq(sia_irq, rotate_isr, IRQ_TYPE_EDGE_FALLING , "rotary_sensor_sia", NULL);
    if (ret) {
        printk(KERN_ERR"%s fail to request_irq, gpio=%d irq=%d\n", __func__, sia_pin, sia_irq);
        goto fail_irq_sw;
    }

	gpio_direction_input(sib_pin);

	return ret;

fail_irq_sw:
	free_irq(sw_irq, NULL);
fail_gpio_sib:
    gpio_free(sib_pin);
fail_gpio_sia:
	gpio_free(sia_pin);
fail_gpio_sw:
	gpio_free(sw_pin);
    return ret;

}
static struct kobject *kobj = NULL;
static int rotary_sensor_init(void)
{		
	int ret = 0;
    printk(KERN_INFO "Matrix-rotary_encoder init.\n");
	
	kobj = kobject_create_and_add("rotary_encoder", &platform_bus.kobj);
	if (!kobj) {
		pr_err("%s: failed to kobject_create_and_add for rotary_encoder\n", __func__);
		return -EINVAL;
	}

	ret = sysfs_create_group(kobj, &attr_group);
	if (ret) {
		pr_err("%s: failed to sysfs_create_group for rotary_encoder\n", __func__);
		kobject_del(kobj);
		return -EINVAL;
	}
	return ret;
}

static void rotary_sensor_exit(void)
{
    printk(KERN_INFO "Matrix-rotary_encoder exit.\n");
    sysfs_remove_group(kobj, &attr_group);
    kobject_put(kobj);
	kobj = NULL;
}

module_init(rotary_sensor_init);
module_exit(rotary_sensor_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("FriendlyARM");
MODULE_DESCRIPTION("Driver for Matrix-Rotary_Sensor");

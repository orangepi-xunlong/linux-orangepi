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

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("DA BAI");
MODULE_DESCRIPTION("Driver for HC-SR04 ultrasonic sensor");

// #define HCSR04_IRQ_DEUBG
struct HCSR04_resource {
    int pin;
};

static int HCSR04_PIN = -1;
static int hasResource = 0;
static int irq_num = 0;
static int irq_time[3];
static int valid_value = 0;
static DECLARE_WAIT_QUEUE_HEAD(hcsr_waitq);

static int hcsr04_hwexit(void);
static int hcsr04_hwinit(void);
static ssize_t hcsr04_value_read(struct class *class, struct class_attribute *attr, char *buf);
static irqreturn_t gpio_isr(int irq, void *data);

static ssize_t hcsr04_value_write(struct class *class, struct class_attribute *attr, const char *buf, size_t len) 
{
    struct HCSR04_resource *res;
    if (len == 4) {
        res = (struct HCSR04_resource *)buf;

        if (res->pin != -1 && hasResource == 0) {
            HCSR04_PIN = res->pin;
            if (hcsr04_hwinit() == -1) {
                return -1;
            }
            hasResource = 1;
        } else if (res->pin == -1 && hasResource == 1) {
            if (hcsr04_hwexit() == -1) {
                return -1;
            }
            hasResource = 0;
        }
    }
    return len;
}

// This function is called when you read /sys/class/hcsr04/value
static ssize_t hcsr04_value_read(struct class *class, struct class_attribute *attr, char *buf)
{
    int ret = -1;
	int gpio_irq=-1;

    if (hasResource == 0) {
        return 0;
    }    
    irq_num = 0;
	valid_value = 0;
    gpio_direction_output(HCSR04_PIN, 0);
    udelay(50);
    gpio_set_value(HCSR04_PIN, 1);
    udelay(50);
    gpio_set_value(HCSR04_PIN, 0);
    udelay(50);

    ret = gpio_to_irq(HCSR04_PIN);
    if (ret < 0) {
        printk(KERN_ERR"%s fail to gpio_to_irq\n", __func__);
        goto fail;
    } else {
        gpio_irq = ret;
    }

    ret = request_irq(gpio_irq, gpio_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING , "hc-sr04", NULL);
    if (ret) {
        printk(KERN_ERR"%s fail to request_irq\n", __func__);
        goto fail;
    }
	
	if(wait_event_interruptible_timeout(hcsr_waitq, valid_value, HZ) == 0) {
		printk(KERN_ERR"%s timeout\n", __func__);
		free_irq(gpio_irq, NULL);
		goto fail;
	}
    free_irq(gpio_irq, NULL);
#ifdef HCSR04_IRQ_DEUBG	    
    printk("%s irq_time[1]=%d irq_time[0]=%d %d\n", __func__, irq_time[1], irq_time[0], irq_time[1]-irq_time[0]);
#endif
    return sprintf(buf, "%d\n", irq_time[1]-irq_time[0]);
 fail:
    return sprintf(buf, "%d\n", -1);;
}

// Sysfs definitions for hcsr04 class
static struct class_attribute hcsr04_class_attrs[] = {
        __ATTR(value,	S_IRUGO | S_IWUSR, hcsr04_value_read, hcsr04_value_write),
        __ATTR_NULL,
};

// Name of directory created in /sys/class
static struct class hcsr04_class = {
        .name =			"hcsr04",
        .owner =		THIS_MODULE, 
        .class_attrs =	hcsr04_class_attrs,
};

// Interrupt handler on ECHO signal
static irqreturn_t gpio_isr(int irq, void *data)
{	
	if (valid_value == 0) {	
		if((irq_num == 0 && gpio_get_value(HCSR04_PIN) != 1) || 
			(irq_num == 1 && gpio_get_value(HCSR04_PIN) != 0) ||
			(irq_num == 2 && gpio_get_value(HCSR04_PIN) != 1)
		) {
			return IRQ_HANDLED;
		}	
	    irq_time[irq_num] = ktime_to_us(ktime_get());
	    if(irq_num == 2) {
#ifdef HCSR04_IRQ_DEUBG	    			
			int i = 0;
	    	for(i=0; i<3; i++)
	    		printk("%d:%d\n", i, irq_time[i]);
#endif      		
			valid_value = 1;
			wake_up_interruptible(&hcsr_waitq);
	    	return IRQ_HANDLED;
	    }		
		irq_num++;
	}
    return IRQ_HANDLED;
}

static int hcsr04_hwinit(void)
{	
    int rtc;

    rtc=gpio_request(HCSR04_PIN, "HCSR04");
    if (rtc!=0) {
        printk(KERN_INFO "Error %d\n",__LINE__);
        goto fail;
    }
    return 0;

    fail:
    return -1;	
}

static int hcsr04_init(void)
{		
    printk(KERN_INFO "HC-SR04 driver v0.32 initializing.\n");

    if (class_register(&hcsr04_class)<0) 
        goto fail;
    return 0;

    fail:
    return -1;

}

static int hcsr04_hwexit(void)
{
    gpio_free(HCSR04_PIN);
    return 0;
}

static void hcsr04_exit(void)
{
    class_unregister(&hcsr04_class);
    printk(KERN_INFO "HC-SR04 disabled.\n");
}

module_init(hcsr04_init);
module_exit(hcsr04_exit);


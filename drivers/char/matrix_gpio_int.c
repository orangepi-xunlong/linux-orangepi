#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>

#define DEVICE_NAME		"sensor"
#define MAX_SENDEV_NUM      8

struct sensor {
    int gpio;
    int int_type;
};


struct sensor_desc {
    int ID;
	int gpio;	
    int int_type;
	char *name;	
	int is_used;
	struct timer_list timer;
};

static size_t sensor_num = -1;				// number of sensor has been registered
static int is_started = 0;
static struct semaphore lock;

static struct sensor_desc sensor_dev[MAX_SENDEV_NUM] = {
	{ 0, -1, -1, "gpio_int_sensor1"},
	{ 1, -1, -1, "gpio_int_sensor2"},
	{ 2, -1, -1, "gpio_int_sensor3"},
	{ 3, -1, -1, "gpio_int_sensor4"},
	{ 4, -1, -1, "gpio_int_sensor5"},
	{ 5, -1, -1, "gpio_int_sensor6"},
	{ 6, -1, -1, "gpio_int_sensor7"},
	{ 7, -1, -1, "gpio_int_sensor8"},
};

static volatile char sensor_dev_value[MAX_SENDEV_NUM] = {
};

static DECLARE_WAIT_QUEUE_HEAD(sensor_dev_waitq);

static volatile int ev_press = 0;

// static int timer_counter = 0;
static void gpio_int_sensors_timer(unsigned long _data)
{

	struct sensor_desc *bdata = (struct sensor_desc *)_data;
	int ID;
	unsigned tmp = gpio_get_value(bdata->gpio);
	ID = bdata->ID;

	// printk("%s %d value=%d int_type=%d\n", __FUNCTION__, timer_counter++, tmp, bdata->int_type);
	if (bdata->int_type == IRQ_TYPE_EDGE_RISING) {
		if(tmp == 1) {
			sensor_dev_value[ID] = 1;
			ev_press = 1;
			wake_up_interruptible(&sensor_dev_waitq);
		}
	} else if (bdata->int_type == IRQ_TYPE_EDGE_FALLING) {	
		if(tmp == 0) {
			sensor_dev_value[ID] = 1;
			ev_press = 1;
			wake_up_interruptible(&sensor_dev_waitq);
		}
	} else if (bdata->int_type == IRQ_TYPE_EDGE_BOTH) {
		sensor_dev_value[ID] = tmp;
		ev_press = 1;
		wake_up_interruptible(&sensor_dev_waitq);
	}
}

// static int int_counter=0;
static irqreturn_t gpio_int_sensor_interrupt(int irq, void *dev_id)
{
	// printk("%s %d\n", __FUNCTION__, int_counter++);
	struct sensor_desc *bdata = (struct sensor_desc *)dev_id;
	mod_timer(&bdata->timer, jiffies + msecs_to_jiffies(100));
	return IRQ_HANDLED;
}

static int start_all_sensor(void)
{
	int irq;
	int i;
	int err = 0;

	for (i = 0; i < sensor_num; i++) {
		if (!sensor_dev[i].gpio)
			continue;

		setup_timer(&sensor_dev[i].timer, gpio_int_sensors_timer,
				(unsigned long)&sensor_dev[i]);

		err = gpio_request(sensor_dev[i].gpio, "gpio_int_sensor");			
		if (err) {
			printk("%s gpio_request %d fail\n", __FUNCTION__, sensor_dev[i].gpio);
            break;
		}

		irq = gpio_to_irq(sensor_dev[i].gpio);
		err = request_irq(irq, gpio_int_sensor_interrupt, IRQ_TYPE_EDGE_FALLING, 
				sensor_dev[i].name, (void *)&sensor_dev[i]);
		if (err) {
		    printk("%s request_irq %d fail\n", __FUNCTION__, irq);
            break;
		}
	}

	if (err) {
		i--;
		for (; i >= 0; i--) {
			if (!sensor_dev[i].gpio)
				continue;

			gpio_free(sensor_dev[i].gpio);
			irq = gpio_to_irq(sensor_dev[i].gpio);
			disable_irq(irq);
			free_irq(irq, (void *)&sensor_dev[i]);

			del_timer_sync(&sensor_dev[i].timer);
		}

		return -EBUSY;
	}

	ev_press = 0;
	return 0;
}

static int gpio_int_sensors_open(struct inode *inode, struct file *file)
{
	if (down_trylock(&lock))
		return -EBUSY;
    sensor_num = 0;
    is_started = 0;
	return 0;
}

static int stop_all_sensor(void)
{
	int irq, i;

	for (i = 0; i < sensor_num; i++) {
		if (!sensor_dev[i].gpio)
			continue;

		gpio_free(sensor_dev[i].gpio);
		irq = gpio_to_irq(sensor_dev[i].gpio);
		free_irq(irq, (void *)&sensor_dev[i]);

		del_timer_sync(&sensor_dev[i].timer);
	}
	return 0;
}

static int gpio_int_sensors_close(struct inode *inode, struct file *file)
{
	// printk("%s sensor_num=%d is_started=%d\n",__FUNCTION__, sensor_num, is_started);
	if (is_started && sensor_num > 0) {
		stop_all_sensor();
	}
	sensor_num = -1;
	is_started = 0;
	ev_press = 0;
	up(&lock);
	return 0;
}

// static int read_counter = 0;
static int gpio_int_sensors_read(struct file *filp, char __user *buff,
		size_t count, loff_t *offp)
{
	unsigned long err;
	int i = 0;

	if (!ev_press) {
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		else
			wait_event_interruptible(sensor_dev_waitq, ev_press);
	}

	ev_press = 0;

	err = copy_to_user((void *)buff, (const void *)(&sensor_dev_value),
			min(sensor_num, count));
	for (i = 0; i<sensor_num; i++) {
		sensor_dev_value[i] = 0;
	}

	return err ? -EFAULT : min(sensor_num, count);
}

static unsigned int gpio_int_sensors_poll( struct file *file,
		struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &sensor_dev_waitq, wait);
	if (ev_press)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static long gpio_int_sensors_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
#define ADD_SENSOR                  (0)
#define DEL_SENSOR                  (1)
#define START_ALL_SENSOR            (4)
#define STOP_ALL_SENSOR             (8)

	struct sensor sen;
	void __user *argp;
	int del_dev;
	int ret;

	//printk("%s cmd=%d\n", __func__, cmd);
	switch(cmd) {
		case ADD_SENSOR:
			if (is_started != 0 || sensor_num >= MAX_SENDEV_NUM)
				return -EBUSY;
			argp = (void __user *)arg;
			if (copy_from_user(&sen, argp, sizeof sen))
				return -EINVAL;
			sensor_dev[sensor_num].gpio = sen.gpio;
			sensor_dev[sensor_num].int_type= sen.int_type;
			sensor_num++;
			break;
		case DEL_SENSOR:
			del_dev = (int)arg;
			// printk("%s del_dev=%d\n", __func__, del_dev);
			if (is_started != 0 || sensor_num <= 0 || del_dev != sensor_num) 
				return -EINVAL;			
			sensor_dev[sensor_num].gpio = -1;					
			sensor_dev[sensor_num].int_type= -1;			
			sensor_num--;
			break;
		case START_ALL_SENSOR:
			if (is_started != 0 || sensor_num <= 0)
				return -EBUSY;
			if((ret = start_all_sensor()) < 0)
				return ret;
			is_started = 1;					
			break;
		case STOP_ALL_SENSOR:
			if (is_started != 1 || sensor_num <= 0)
				return -EBUSY;
			stop_all_sensor();
			is_started = 0;
			break;
		default:
			return -EINVAL;
	}

	return 0;
}


static struct file_operations dev_fops = {
	.owner		= THIS_MODULE,
	.open		= gpio_int_sensors_open,
	.release	= gpio_int_sensors_close, 
	.read		= gpio_int_sensors_read,
	.poll		= gpio_int_sensors_poll,
	.unlocked_ioctl	= gpio_int_sensors_ioctl,
};

static struct miscdevice misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DEVICE_NAME,
	.fops		= &dev_fops,
};

static int __init sensor_dev_init(void)
{
	printk("Matirx GPIO Int sensor init\n");
	sema_init(&lock, 1);
	return misc_register(&misc);
}

static void __exit sensor_dev_exit(void)
{
	printk("Matirx GPIO Int sensor exit\n");
	misc_deregister(&misc);
}

module_init(sensor_dev_init);
module_exit(sensor_dev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc.");


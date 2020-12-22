/*
 * linux/drivers/char/matrix_pwm.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/gpio.h>

#include <mach/platform.h>
#include <asm/uaccess.h>

#define DEVICE_NAME				"pwm"
#define PWM_NUM					(2)
#define PWM_IOCTL_SET_FREQ		(0x1)
#define PWM_IOCTL_STOP			(0x0)
#define NS_IN_1HZ				(1000000000UL)

static int is_pwm_working[PWM_NUM] = {0, 0};
static struct pwm_device *matrix_pwm[PWM_NUM];
static struct semaphore lock;

static int pwm_set_freq(int index, int pin, unsigned long freq, int duty) 
{
	int ret=0;
	int period_ns = NS_IN_1HZ / freq;
	int duty_ns = period_ns  / 1000 * duty;

	printk("%s %d %d\n", __func__, period_ns, duty_ns);
	if ((ret = pwm_config(matrix_pwm[index], duty_ns, period_ns))) {
		printk("fail to pwm_config\n");
		return ret;
	}
	if ((ret = pwm_enable(matrix_pwm[index]))) {
		printk("fail to pwm_enable\n");
		return ret;
	}
	return ret;
}

static void pwm_stop(int index, int pin) 
{
	pwm_config(matrix_pwm[index], 0, NS_IN_1HZ / 100);
	pwm_disable(matrix_pwm[index]);
}

static int matrix_pwm_hw_init(int index, int pin)
{
	int ret=0;

#if 0
	ret = gpio_request(pin, DEVICE_NAME);
	if (ret) {
		printk("request gpio %d for pwm failed\n", pin);
		return ret;
	}
	gpio_direction_output(pin, 0);
#endif

	matrix_pwm[index] = pwm_request(index, DEVICE_NAME);
	if (IS_ERR(matrix_pwm[index])) {
		printk("request pwm%d failed\n", index);
#if 0
		gpio_free(pin);
#endif
		return -ENODEV;
	}

	pwm_stop(index, pin);
	return ret;
}

static void matrix_pwm_hw_exit(int index,int pin) 
{
	pwm_stop(index, pin);
	pwm_free(matrix_pwm[index]);
#if 0
	gpio_free(pin);
#endif
}

static int matrix_pwm_open(struct inode *inode, struct file *file) {
	if (!down_trylock(&lock))
		return 0;
	else
		return -EBUSY;
}

static int matrix_pwm_close(struct inode *inode, struct file *file) {
	up(&lock);
	return 0;
}

static long matrix_pwm_ioctl(struct file *filep, unsigned int cmd,unsigned long arg)
{
	int pwm_id;
	int param[3];
	int gpio;
	int freq;
	int duty;
	int ret;
	
	switch (cmd) {
		case PWM_IOCTL_SET_FREQ:
			if (copy_from_user(param, (void __user *)arg, sizeof(param)))
				return -EINVAL;
			gpio = param[0];
			freq = param[1];
			duty = param[2];
			if (gpio == (SUNXI_PA_BASE + 5)) {
				pwm_id = 0;
			} else if (gpio == (SUNXI_PA_BASE + 6)) {
				pwm_id = 1;
			} else {
				printk("Invalid pwm gpio %d\n", gpio);
				return -EINVAL;
			}

			if (duty < 0 || duty > 1000) {
				printk("Invalid pwm duty %d\n", duty);
				return -EINVAL;
			}
			
			if (is_pwm_working[pwm_id] == 1) {
				matrix_pwm_hw_exit(pwm_id, gpio);
			}
			
			if ((ret = matrix_pwm_hw_init(pwm_id, gpio))) {
				return ret;
			}
			is_pwm_working[pwm_id] = 1;
			if ((ret = pwm_set_freq(pwm_id, gpio, freq, duty))) {
				return ret;
			}
			break;
		case PWM_IOCTL_STOP:
			if (copy_from_user(&gpio, (void __user *)arg, sizeof(gpio)))
				return -EINVAL;
			if (gpio == (SUNXI_PA_BASE + 5)) {
				pwm_id = 0;
			} else if (gpio == (SUNXI_PA_BASE + 6)) {
				pwm_id = 1;
			} else {
				printk("Invalid pwm gpio %d\n", gpio);
				return -EINVAL;
			}
			if (is_pwm_working[pwm_id] == 1) {
				matrix_pwm_hw_exit(pwm_id, gpio);
				is_pwm_working[pwm_id] = 0;
			}
			break;
		default:
			printk("%s unsupported pwm ioctl %d", __FUNCTION__, cmd);
			break;
	}

	return 0;
}

static struct file_operations matrix_pwm_ops = {
	.owner			= THIS_MODULE,
	.open			= matrix_pwm_open,
	.release		= matrix_pwm_close, 
	.unlocked_ioctl	= matrix_pwm_ioctl,
};

static struct miscdevice matrix_misc_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &matrix_pwm_ops,
};

static int __init matrix_pwm_init(void) 
{
	int ret;
	ret = misc_register(&matrix_misc_dev);
	printk("Matirx PWM init\n");

	sema_init(&lock, 1);
	return ret;
}

static void __exit matrix_pwm_exit(void) {
	misc_deregister(&matrix_misc_dev);
	printk("Matirx PWM exit\n");
}

module_init(matrix_pwm_init);
module_exit(matrix_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("FriendlyARM Inc.");
MODULE_DESCRIPTION("FriendlyARM Matrix PWM Driver");


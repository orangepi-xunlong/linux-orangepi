#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/sys_config.h>

#include <linux/regulator/consumer.h>

#define DEV_NAME "orangepi_module"

/*
 * Mali device tree 
 * */
void Gpio_PowerKey_DT(void)
{
	struct device_node *node;
	const char *string;
	int ret;

	node = of_find_node_by_name(NULL, "gpio_power_key");
	if (!node) {
		printk("Can't get gpio_powerkey device node\n");
		goto out;
	}
	printk("SUCCEED!!!!!\n");

	return;

out:
	printk("Bad Operater\n");
}

/*
 * open operation
 */
static int buddy_open(struct inode *inode,struct file *filp)
{
	Gpio_PowerKey_DT();
	printk(KERN_INFO "Open device\n");
	return 0;
}
/*
 * write operation
 */
static ssize_t buddy_write(struct file *filp,const char __user *buf,size_t count,loff_t *offset)
{
	printk(KERN_INFO "Write device\n");
	return 0;
}
/*
 *release operation
 */
static int buddy_release(struct inode *inode,struct file *filp)
{
	printk(KERN_INFO "Release device\n");
	return 0;
}
/*
 * read operation
 */
static ssize_t buddy_read(struct file *filp,char __user *buf,size_t count,
		loff_t *offset)
{
	printk(KERN_INFO "Read device\n");
   
	return 0;
}
/*
 * file_operations
 */
static struct file_operations buddy_fops = {
	.owner     = THIS_MODULE,
	.open      = buddy_open,
	.release   = buddy_release,
	.write     = buddy_write,
	.read      = buddy_read,
};
/*
 * misc struct 
 */

static struct miscdevice buddy_misc = {
	.minor    = MISC_DYNAMIC_MINOR,
	.name     = DEV_NAME,
	.fops     = &buddy_fops,
};
/*
 * Init module
 */
static __init int buddy_init(void)
{
	misc_register(&buddy_misc);
	printk("buddy_test\n");
	return 0;
}
/*
 * Exit module
 */
static __exit void buddy_exit(void)
{
	printk(KERN_INFO "buddy_exit_module\n");
	misc_deregister(&buddy_misc);
}
/*
 * module information
 */
module_init(buddy_init);
module_exit(buddy_exit);

MODULE_LICENSE("GPL");

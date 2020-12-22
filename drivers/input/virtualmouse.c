#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>    
#include <linux/aio.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
 
int vmouse_major = 200;

MODULE_AUTHOR("yangfuyang <yangfuyang@allwinnertech.com>");
MODULE_LICENSE("GPL");
 
static struct input_dev* vmouse_idev = NULL;

int vmouse_open(struct inode* inode, struct file* filp)
{
    return 0;
}
 
int vmouse_release(struct inode* inode, struct file* filp)
{
    return 0;
}
 
ssize_t vmouse_read(struct file* filp, char __user *buf, size_t count, loff_t* f_pos)
{
    printk(KERN_INFO"%s\n", __func__);
    return count;
}
 
struct mouse_event
{
    int flag;
    int x;
    int y;
};
 
ssize_t vmouse_write(struct file* filp, const char __user * buf, size_t count, loff_t* f_pos)
{
    int ret = 0;
    struct mouse_event event;
 
    while(ret < count)
    {
        if(copy_from_user(&event, buf + ret, sizeof(event)))
        {
            return -EFAULT;
        }
        ret += sizeof(event);

		if(event.flag == 2)
		{
			input_report_rel(vmouse_idev, REL_X, event.x);
			input_report_rel(vmouse_idev, REL_Y, event.y);
			input_sync(vmouse_idev);
		} else if (event.flag == 1) {
			input_report_key(vmouse_idev, BTN_MOUSE, event.x);
			input_sync(vmouse_idev);
		}else if (event.flag == 0) {
			input_report_key(vmouse_idev, BTN_RIGHT, event.x);
			input_sync(vmouse_idev);
		} else if(event.flag == 3)
		{
			input_report_rel(vmouse_idev, REL_WHEEL, event.x);
			input_report_rel(vmouse_idev, REL_HWHEEL, event.y);
			input_sync(vmouse_idev);
		}
        //printk(KERN_INFO"%s p=%d x=%d y=%d\n", __func__, event.flag, event.x, event.y);
    }
 
    return ret;
}
 
static struct file_operations vmouse_fops = 
{
    .owner = THIS_MODULE,
    .open    = vmouse_open,
    .release = vmouse_release,
    .read    = vmouse_read,
    .write   = vmouse_write,
};

static int vmouse_input_dev_open(struct input_dev* idev)
{
    printk(KERN_INFO"%s \n", __func__);
 
    return 0;
}
 
static void vmouse_input_dev_close(struct input_dev* idev)
{
    printk(KERN_INFO"%s \n", __func__);
 
    return;
}
 
static int vmouse_input_dev_setup(void)
{
    int ret = 0;
    vmouse_idev = input_allocate_device();
 
    if(vmouse_idev == NULL)
    {
        return -ENOMEM;
    }
 
    vmouse_idev->name = "vmouse";
    vmouse_idev->phys = "vmouse/input0";
	vmouse_idev->id.bustype = BUS_HOST;
	vmouse_idev->id.product = 0x0001;
	vmouse_idev->id.vendor = 0x0002;
	vmouse_idev->id.version = 0x0100;
    vmouse_idev->open = vmouse_input_dev_open;
    vmouse_idev->close = vmouse_input_dev_close;

	vmouse_idev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_REL);
	vmouse_idev->keybit[BIT_WORD(BTN_LEFT)] = BIT_MASK(BTN_LEFT) |
		BIT_MASK(BTN_MIDDLE) | BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_TOUCH);
	vmouse_idev->relbit[0] = BIT_MASK(REL_X) | BIT_MASK(REL_Y) | BIT_MASK(REL_WHEEL) | BIT_MASK(REL_HWHEEL);
 
    ret = input_register_device(vmouse_idev);
 
    return ret;
}

static struct miscdevice mouse_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "vmouse",
	.fops = &vmouse_fops,
};

static int __init vmouse_init(void)
{
    int result = misc_register(&mouse_dev);
	printk(" uinput result %d , %s \n", result, __func__);
    vmouse_input_dev_setup();
 
    return result;
}
 
static void __exit vmouse_cleanup(void)
{
    input_unregister_device(vmouse_idev);
    misc_deregister(&mouse_dev);
 
    return;
}
 
module_init(vmouse_init);
module_exit(vmouse_cleanup);



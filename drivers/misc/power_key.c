#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/irq.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#define touch_gpio 362 //这个引脚为中断输入引脚
struct fasync_struct *fasync_queue; //异步通知队列

//中断处理函数
static irqreturn_t touch_interrupt(int irq, void *dev_id) {
    /* 在中断服务函数中向应用层发送SIGIO信号-异步通知 */
    kill_fasync(&fasync_queue, SIGIO, POLL_IN); 
    printk("touch_interrupt\n");
    return IRQ_RETVAL(IRQ_HANDLED);     
}

static int touch_open(struct inode *inode, struct file *file)
{
    int ret;
    int virq;
    printk("touch_open\n");
    if(atomic_read(&file->f_count) != 0)
    {
        /*gpio申请*/
        ret = gpio_request(touch_gpio,"touch_gpio");
        if(ret!=0){
            printk("gpio_request failed.\n");
        }
        /*将gpio端口映射到中断号*/
        virq = gpio_to_irq(touch_gpio);
        if (IS_ERR_VALUE(virq)) {
            printk("map gpio [%d] to virq [%d] failed\n", touch_gpio, virq);
            return -EINVAL;
        }
        /* 申请中断并设置为高电平触发 */
        ret = request_irq(virq, touch_interrupt, IRQF_TRIGGER_RISING, "PA0_EINT", NULL);
        if (IS_ERR_VALUE(ret)) {
            printk("request virq %d failed, errno = %d\n", virq, ret);
            return -EINVAL;
        }
    }
    return 0;
}
static int touch_release(struct inode *inode, struct file *file)
{
    printk("touch_release\n");
    if(atomic_read(&file->f_count) == 0)
    {
        int virq;
        virq = gpio_to_irq(touch_gpio);
        if (IS_ERR_VALUE(virq)) {
            printk("map gpio [%d] to virq [%d] failed\n", touch_gpio, virq);
            return -EINVAL;
        }
        free_irq(virq,NULL);
        /*释放GPIO*/
        gpio_free(touch_gpio);
    }
    return 0;
}
static int touch_fasync(int fd, struct file *file, int on)
{
    /*将该设备登记到fasync_queue队列中去*/
    int ret = fasync_helper(fd, file, on, &fasync_queue); 
    if(ret<0){
        printk("failed touch_fasync \n");
        return ret;
    }
    return 0;
}
static struct file_operations touch_ops = {
    .owner = THIS_MODULE,
    .open = touch_open,
    .release = touch_release,
    .fasync = touch_fasync,
};

static struct miscdevice touch_device = {
	.minor	= MISC_DYNAMIC_MINOR,
	.fops	= &touch_ops,
	.name	= "touch_driver", // 杂项设备名称为“touch_driver”
};

static int touch_init(void)
{
    int ret;
    printk("[leeboby] touch_init\n");
    ret = misc_register(&touch_device);
    return ret;
}

static void touch_exit(void)
{
    printk("touch_exit\n");
    misc_deregister(&touch_device);
}

MODULE_LICENSE("GPL v2");
module_init(touch_init);
module_exit(touch_exit);

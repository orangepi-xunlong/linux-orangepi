#include <linux/device.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <asm/uaccess.h>

#include "sunxi_tvd.h"
#include "virtual_source.h"
#include "car_reverse.h"

struct buffer_info {
	void *phy_address;
	void *vir_address;
	int size;
};

struct virtual_source_priv {
	int enable;
	int period;	/* update period in ms */

	int references;
	struct list_head frame;
	struct buffer_ops buffer;

	int status;
	struct hrtimer timer;
	ktime_t alarm;

	spinlock_t lock;
	int interrupt;

	/* buffer */
	struct buffer_info ibuffer;
	struct display_info display;

	spinlock_t buffer_lock;
	struct list_head active;

	void (*buffer_done)(int tvd_fd);
};

static int debug_mask;
static struct virtual_source_priv virtual_source;

static enum hrtimer_restart virtual_source_timer_callback(struct hrtimer *timer);

int tvd_open_special(int tvd_fd)
{
	virtual_source.references++;
	printk("%s: references = %d\n", __func__,
		virtual_source.references);

	return 0;
}
EXPORT_SYMBOL(tvd_open_special);

int tvd_close_special(int tvd_fd)
{
	virtual_source.references--;
	printk("%s: references = %d\n", __func__,
		virtual_source.references);

	return 0;
}
EXPORT_SYMBOL(tvd_close_special);

int dqbuffer_special(int tvd_fd, struct tvd_buffer **buf)
{
	struct tvd_buffer *buffer;

	*buf = NULL;
	spin_lock(&virtual_source.buffer_lock);
	if (!list_empty(&virtual_source.active)) {
		buffer = list_entry(virtual_source.active.next, struct tvd_buffer, list);
		list_del(&buffer->list);
		*buf = buffer;
		printk("[virtual tvd] dequeue buffer %p\n", buffer->paddr);
	}
	spin_unlock(&virtual_source.buffer_lock);
	return 0;
}
EXPORT_SYMBOL(dqbuffer_special);

int qbuffer_special(int tvd_fd, struct tvd_buffer *buf)
{
	spin_lock(&virtual_source.buffer_lock);
	list_add_tail(&buf->list, &virtual_source.frame);
	spin_unlock(&virtual_source.buffer_lock);
	return 0;
}
EXPORT_SYMBOL(qbuffer_special);

void virtual_tvd_register_buffer_done_callback(void *func)
{
	virtual_source.buffer_done = func;
}
EXPORT_SYMBOL(virtual_tvd_register_buffer_done_callback);

int vidioc_streamon_special(int tvd_fd, enum v4l2_buf_type flag)
{
	printk("++ %s\n", __func__);
	hrtimer_init(&virtual_source.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	virtual_source.timer.function = virtual_source_timer_callback;
	virtual_source.alarm = ktime_set(0, virtual_source.period*1000*1000);

	virtual_source.status = 1;
	hrtimer_start(&virtual_source.timer, virtual_source.alarm, HRTIMER_MODE_REL);

	return 0;
}
EXPORT_SYMBOL(vidioc_streamon_special);

int vidioc_streamoff_special(int tvd_fd, enum v4l2_buf_type flag)
{
	struct tvd_buffer *buffer;

	printk("++ %s\n", __func__);
	virtual_source.status = 0;
	hrtimer_cancel(&virtual_source.timer);

	spin_lock(&virtual_source.buffer_lock);
	while (!list_empty(&virtual_source.frame)) {
		buffer = list_entry(virtual_source.frame.next, struct tvd_buffer, list);
		list_del(&buffer->list);
		list_add_tail(&buffer->list, &virtual_source.active);
		printk("%s: remove buffer %p\n", __func__, buffer->paddr);
	}
	spin_unlock(&virtual_source.buffer_lock);

	return 0;
}
EXPORT_SYMBOL(vidioc_streamoff_special);

int virtual_source_get(int idx)
{
	virtual_source.references++;
	printk("%s: references = %d\n", __func__,
		virtual_source.references);

	return 0;
}
EXPORT_SYMBOL(virtual_source_get);

int virtual_source_put(int idx)
{
	virtual_source.references--;
	printk("%s: references = %d\n", __func__,
		virtual_source.references);

	return 0;
}
EXPORT_SYMBOL(virtual_source_put);

int virtual_source_register_buffer_ops(struct buffer_ops *ops)
{
	printk("++ %s, ops %p\n", __func__, ops);
	if (ops) {
		virtual_source.buffer.queue_buffer   = ops->queue_buffer;
		virtual_source.buffer.dequeue_buffer = ops->dequeue_buffer;
	}
	return 0;
}
EXPORT_SYMBOL(virtual_source_register_buffer_ops);

static void tvd_buffer_draw(struct tvd_buffer *buffer)
{
#if 0
	static int index;
	int i, j;
	int width  = buffer->fmt->width;
	int height = buffer->fmt->height;
	int stride = width * 4;
	int gap = width / 8;
	char *line_start = (char *)buffer->vaddr + (index % 8) * gap * 4;
	char *p;

	memset(buffer->vaddr, 0, width * height * 4);
	for (i = 0; i < height; i++) {
		p = line_start;
		line_start += stride;
		for (j = 0; j < gap; j++) {
			*p++ = 0xff;
			*p++ = 0xa0;
			*p++ = 0xa0;
			*p++ = 0xa0;
		}
	}
	index++;
#endif

	static int index;
	int width  = buffer->fmt->width;
	int height = buffer->fmt->height;
	char *pbuffer = (char *)buffer->vaddr;
	int yplan_size = width * height;
	int uplan_size = width * height / 4;
	int i;

	index++;

	switch (buffer->fmt->output_fmt) {
	case TVD_PL_YUV420:
		if (index & 0x01) {
			memset(pbuffer,  71, yplan_size/2);

			pbuffer += yplan_size;
			memset(pbuffer,  87, uplan_size);

			pbuffer += uplan_size;
			memset(pbuffer, 248, uplan_size);
		} else {
			memset(pbuffer, 0xf0, yplan_size/2);

			pbuffer += yplan_size;
			memset(pbuffer, 0, uplan_size);

			pbuffer += uplan_size;
			memset(pbuffer, 0, uplan_size);
		}
		break;
	case TVD_MB_YUV420:
		if (index & 0x01) {
			memset(pbuffer,  71, yplan_size/2);

			pbuffer += yplan_size;
			for (i = 0; i < uplan_size; i++) {
				*pbuffer++ = 87;
				*pbuffer++ = 248;
			}
		} else {
			memset(pbuffer, 0xf0, yplan_size/2);

			pbuffer += yplan_size;
			for (i = 0; i < uplan_size; i++) {
				*pbuffer++ = 87;
				*pbuffer++ = 248;
			}
		}
		break;
	default:
		break;
	}
}

static enum hrtimer_restart virtual_source_timer_callback(struct hrtimer *timer)
{
#if 0
	struct buffer_node *node;
	struct display_info info;

	if (!list_empty(&virtual_source.frame)) {
		node = list_entry(virtual_source.frame.next, struct buffer_node, list);
		list_del(&node->list);
		virtual_source.buffer.queue_buffer(node);
		printk("[virtual source] queue buffer %p\n", node->phy_address);
	}

	node = virtual_source.buffer.dequeue_buffer();
	if (node) {
		list_add(&node->list, &virtual_source.frame);
	}

	spin_lock(&virtual_source.lock);
	virtual_source.interrupt++;
	memcpy(&info, &virtual_source.display, sizeof(struct display_info));
	spin_unlock(&virtual_source.lock);

	hrtimer_forward_now(timer, virtual_source.alarm);
	return virtual_source.status ? HRTIMER_RESTART : HRTIMER_NORESTART;
#endif
	struct tvd_buffer *buffer;

	spin_lock(&virtual_source.buffer_lock);
	if (!list_empty(&virtual_source.frame)) {
		buffer = list_entry(virtual_source.frame.next, struct tvd_buffer, list);
		list_del(&buffer->list);
		list_add_tail(&buffer->list, &virtual_source.active);
		tvd_buffer_draw(buffer);
		if (virtual_source.buffer_done) {
			virtual_source.buffer_done(0);
		}
	} else {
		printk(KERN_ERR "%s: buffer maybe over write!\n", __func__);
	}
	spin_unlock(&virtual_source.buffer_lock);

	hrtimer_forward_now(timer, virtual_source.alarm);
	return virtual_source.status ? HRTIMER_RESTART : HRTIMER_NORESTART;
}

int virtual_source_streamon(void)
{
	struct buffer_node *node;

	printk("++ %s\n", __func__);
	hrtimer_init(&virtual_source.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	virtual_source.timer.function = virtual_source_timer_callback;
	virtual_source.alarm = ktime_set(0, virtual_source.period*1000*1000);

	virtual_source.status = 1;
	if (virtual_source.buffer.dequeue_buffer) {
		node = virtual_source.buffer.dequeue_buffer();
		if (node) {
			list_add(&node->list, &virtual_source.frame);
		}
		hrtimer_start(&virtual_source.timer, virtual_source.alarm, HRTIMER_MODE_REL);
	}

	return 0;
}
EXPORT_SYMBOL(virtual_source_streamon);

int virtual_source_streamoff(void)
{
	struct tvd_buffer *buffer;

	printk("++ %s\n", __func__);
	virtual_source.status = 0;
	hrtimer_cancel(&virtual_source.timer);

	spin_lock(&virtual_source.buffer_lock);
	while (!list_empty(&virtual_source.frame)) {
		buffer = list_entry(virtual_source.frame.next, struct tvd_buffer, list);
		list_del(&buffer->list);
		list_add_tail(&buffer->list, &virtual_source.active);
		printk("%s: remove buffer %p\n", __func__, buffer->paddr);
	}
	spin_unlock(&virtual_source.buffer_lock);

	return 0;
}
EXPORT_SYMBOL(virtual_source_streamoff);

static int alloc_image_buffer(struct device *dev, struct buffer_info *info, int size)
{
	unsigned long phyaddr;

	info->size = PAGE_ALIGN(size);
	info->vir_address = dma_alloc_coherent(dev, info->size,
		(dma_addr_t *)&phyaddr, GFP_KERNEL);

	if (!info->vir_address) {
		printk("dma memory alloc failed\n");
		return -1;
	}
	info->phy_address = (void *)phyaddr;
	printk("%s: at %p, size %d\n", __func__,
		info->phy_address, info->size);
	return 0;
}

static int free_image_buffer(struct device *dev, struct buffer_info *info)
{
	if (info && info->vir_address) {
		dma_free_coherent(dev, info->size,
			info->vir_address, (dma_addr_t)info->phy_address);
	}
	return 0;
}

static int virtual_source_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int virtual_source_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long virtual_source_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int retval = 0;
	void __user *argp = (void __user *)arg;

	switch (cmd) {
	case VS_IOC_GET_BUFFER_SIZE:
		if (copy_to_user(argp, &virtual_source.ibuffer.size, sizeof(int))) {
			printk("copy from user failed!\n");
			retval = -EIO;
		}
		break;
	case VS_IOC_UPDATE_DISPLAY:
		spin_lock(&virtual_source.lock);
		if (copy_from_user(&virtual_source.display, argp, sizeof(struct display_info))) {
			printk("copy from user failed!\n");
			retval = -EIO;
		}
		spin_unlock(&virtual_source.lock);
		break;
	default:
		printk("%s: unknow cmd %x\n", __func__, cmd);
		break;
	}
	return retval;
}

static int virtual_source_mmap(struct file *file, struct vm_area_struct *vma)
{
	unsigned long physics = (unsigned long)virtual_source.ibuffer.phy_address;
	unsigned long pfn = physics >> PAGE_SHIFT;
	unsigned long vmsize = vma->vm_end - vma->vm_start;

	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
    if (remap_pfn_range(vma, vma->vm_start, pfn, vmsize, vma->vm_page_prot))
		return -EAGAIN;

	printk("%s: mmap at %p, size %ld\n", __func__,
		virtual_source.ibuffer.phy_address, vmsize);

	return 0;
}

static ssize_t virtual_source_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", virtual_source.enable ? "enable" : "disable");
}

static ssize_t virtual_source_enable_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	if (!strncmp(buf, "enable", 6))
		virtual_source.enable = 1;
	else
		virtual_source.enable = 0;
	return count;
}

static ssize_t virtual_source_period_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d ms\n", virtual_source.period);
}

static ssize_t virtual_source_period_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	virtual_source.period = simple_strtoul(buf, NULL, 10);
	return count;
}

static ssize_t virtual_source_interrupt_show(struct device *class,
		struct device_attribute *attr, char *buf)
{
	int interrupt;

	spin_lock(&virtual_source.lock);
	interrupt = virtual_source.interrupt;
	spin_unlock(&virtual_source.lock);

	return sprintf(buf, "%d\n", interrupt);
}

DEVICE_ATTR(enable, 0644, virtual_source_enable_show, virtual_source_enable_store);
DEVICE_ATTR(period, 0644, virtual_source_period_show, virtual_source_period_store);
DEVICE_ATTR(interrupt, 0444, virtual_source_interrupt_show, NULL);

static struct file_operations virtual_source_ops = {
	.owner          = THIS_MODULE,
	.open           = virtual_source_open,
	.release        = virtual_source_release,
	.unlocked_ioctl = virtual_source_ioctl,
	.mmap           = virtual_source_mmap,
};

static struct cdev *vscdev;
static struct class *vscdev_class;
static dev_t devid;

static int virtual_source_probe(struct platform_device *pdev)
{
	memset(&virtual_source, 0, sizeof(virtual_source));
	virtual_source.enable = 0;
	virtual_source.period = 500;
	INIT_LIST_HEAD(&virtual_source.frame);
	INIT_LIST_HEAD(&virtual_source.active);

	spin_lock_init(&virtual_source.lock);
	spin_lock_init(&virtual_source.buffer_lock);
	virtual_source.interrupt = 0;

	alloc_chrdev_region(&devid, 0, 1, "vscdev");
	vscdev = cdev_alloc();
	cdev_init(vscdev, &virtual_source_ops);
	vscdev->owner = THIS_MODULE;
	if (cdev_add(vscdev, devid, 1)) {
		printk("add char device '%d' failed\n", MAJOR(devid));
	}

	vscdev_class = class_create(THIS_MODULE, "vscdev");
	device_create(vscdev_class, NULL, devid, NULL, "vscdev");

	device_create_file(&pdev->dev, &dev_attr_enable);
	device_create_file(&pdev->dev, &dev_attr_period);
	device_create_file(&pdev->dev, &dev_attr_interrupt);

	alloc_image_buffer(&pdev->dev, &virtual_source.ibuffer, 720*480*4);

	return 0;
}

static int virtual_source_remove(struct platform_device *pdev)
{
	free_image_buffer(&pdev->dev, &virtual_source.ibuffer);
	device_remove_file(&pdev->dev, &dev_attr_interrupt);
	device_remove_file(&pdev->dev, &dev_attr_period);
	device_remove_file(&pdev->dev, &dev_attr_enable);
	device_destroy(vscdev_class, devid);
	class_destroy(vscdev_class);
	cdev_del(vscdev);
	return 0;
}

static struct platform_driver virtual_source_driver = {
	.probe   = virtual_source_probe,
	.remove  = virtual_source_remove,
	.driver = {
		.name = "virtual_source",
		.owner = THIS_MODULE,
	},
};

static struct platform_device *pdev;
static int __init virtual_source_init(void)
{
	int retval = 0;

	retval = platform_driver_register(&virtual_source_driver);
	if (retval)
		return -EINVAL;

	pdev = platform_device_register_simple("virtual_source", -1, NULL, 0);
	if (IS_ERR(pdev)) {
		retval = PTR_ERR(pdev);
		printk("%s: failed, error = %d\n", __func__, retval);
		return retval;
	}
	return 0;
}

static void __exit virtual_source_exit(void)
{
	platform_device_unregister(pdev);
	platform_driver_unregister(&virtual_source_driver);
	printk(KERN_INFO "virtual_source: driver unloaded.\n");
}

/*module_init(virtual_source_init);*/
fs_initcall(virtual_source_init);
module_exit(virtual_source_exit);
module_param_named(debug_mask, debug_mask, int, 0644);

MODULE_DESCRIPTION("virtual source driver");
MODULE_LICENSE("GPL");

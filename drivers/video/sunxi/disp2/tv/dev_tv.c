#include "dev_tv.h"
#include <mach/platform.h>


static struct cdev *tv_cdev;
static dev_t devid ;
static struct class *tv_class;

struct tv_info_t g_tv_info;



static struct resource tv_resource[1] = 
{
	[0] = {
		.start = 0x01c16000,  //modify
		.end   = 0x01c165ff,
		.flags = IORESOURCE_MEM,
	},
};

struct platform_device tv_device = 
{
	.name           = "tv",
	.id		        = -1,
	.num_resources  = ARRAY_SIZE(tv_resource),
	.resource	    = tv_resource,
	.dev            = {}
};



static int __init tv_probe(struct platform_device *pdev)
{
	int ret;
	memset(&g_tv_info, 0, sizeof(struct tv_info_t));
	g_tv_info.screen[0].base_address= (void __iomem *)SUNXI_TVE_VBASE;
	ret = tv_init();
	if(ret!=0) {
		pr_debug("tv_probe is failed\n");
		return -1;
	}

	return 0;
}

static int tv_remove(struct platform_device *pdev)
{
	tv_exit();
	return 0;
}

/*int tv_suspend(struct platform_device *pdev, pm_message_t state)
{
    return 0;
}*/

/*int tv_resume(struct platform_device *pdev)
{
    return 0;
}*/
static struct platform_driver tv_driver =
{
	.probe      = tv_probe,
	.remove     = tv_remove,
 //   .suspend    = tv_suspend,
  //  .resume     = tv_resume,
	.driver     =
	{
		.name   = "tv",
		.owner  = THIS_MODULE,
	},
};

int tv_open(struct inode *inode, struct file *file)
{
	return 0;
}

int tv_release(struct inode *inode, struct file *file)
{
	return 0;
}


ssize_t tv_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

ssize_t tv_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
	return -EINVAL;
}

int tv_mmap(struct file *file, struct vm_area_struct * vma)
{
	return 0;
}

long tv_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return 0;
}


static const struct file_operations tv_fops = 
{
	.owner		= THIS_MODULE,
	.open		= tv_open,
	.release    = tv_release,
	.write      = tv_write,
	.read		= tv_read,
	.unlocked_ioctl	= tv_ioctl,
	.mmap       = tv_mmap,
};


int __init tv_module_init(void)
{
	int ret = 0, err;

	alloc_chrdev_region(&devid, 0, 1, "tv");
	tv_cdev = cdev_alloc();
	cdev_init(tv_cdev, &tv_fops);
	tv_cdev->owner = THIS_MODULE;
	err = cdev_add(tv_cdev, devid, 1);
	if (err) {
        pr_debug("cdev_add fail.\n");
        return -1;
	}

	tv_class = class_create(THIS_MODULE, "tv");
	if (IS_ERR(tv_class)) {
        pr_debug("class_create fail.\n");
		return -1;
	}

	g_tv_info.dev = device_create(tv_class, NULL, devid, NULL, "tv");
	ret = platform_device_register(&tv_device);

    if (ret == 0) {
        ret = platform_driver_register(&tv_driver);
	}
	return ret;
}

static void __exit tv_module_exit(void)
{

	platform_driver_unregister(&tv_driver);
	platform_device_unregister(&tv_device);
	class_destroy(tv_class);
	cdev_del(tv_cdev);
}


late_initcall(tv_module_init);
module_exit(tv_module_exit);

MODULE_AUTHOR("zengqi");
MODULE_DESCRIPTION("tv driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:tv");


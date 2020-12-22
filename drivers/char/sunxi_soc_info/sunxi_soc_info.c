#include <linux/atomic.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/printk.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/sysfs.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/io.h>

#include <mach/sunxi-chip.h>



#define CHECK_SOC_SECURE_ATTR 0x00
#define CHECK_SOC_VERSION     0x01
#define CHECK_SOC_BONDING     0x03
#define CHECK_SOC_BOOT_ATTR   0x04


static int soc_info_open(struct inode *inode, struct file *file)
{
	int ret = 0;
	return ret;
}
static int soc_info_release(struct inode *inode, struct file *file)
{
	return 0;
}

static long soc_info_ioctl(struct file *file, unsigned int ioctl_num,
		unsigned long ioctl_param)
{

	int err = 0;
	char id[8];

	switch(ioctl_num){
		case CHECK_SOC_SECURE_ATTR:
			err = sunxi_soc_is_secure();
			if(err){
				pr_debug("soc is secure. return value: %d\n", err);
			}else{
				pr_debug("soc is normal. return value: %d\n", err);
			}
			break;
		case CHECK_SOC_VERSION:
			err = sunxi_get_soc_ver();
			pr_debug("soc version:%x\n", err);
			break;
		case CHECK_SOC_BONDING:
			sunxi_get_soc_chipid_str(id);
			err = copy_to_user((void __user *)ioctl_param, id, 8);
			pr_debug("soc id:%s\n", id);
			break;
		case CHECK_SOC_BOOT_ATTR:
			err = sunxi_boot_is_secure() ; 
			if(err)
				pr_debug("secure boot %d\n",err);
			else
				pr_debug("normal boot %d\n",err);
			break;
		default:
			pr_err("un supported cmd:%d\n", ioctl_num);
			break;
	}
	return err;

}

static const struct file_operations soc_info_ops = {
	.owner 		= THIS_MODULE,
	.open 		= soc_info_open,
	.release 	= soc_info_release,
	.unlocked_ioctl = soc_info_ioctl,
};

struct miscdevice soc_info_device = {
	.minor 	= MISC_DYNAMIC_MINOR,
	.name 	= "sunxi_soc_info",
	.fops 	= &soc_info_ops,
};

#ifdef CONFIG_DEBUG_FS

static int soc_is_secure_show(struct seq_file *s, void *d)
{
	int ret;
	char *soc_attr;
	ret = sunxi_soc_is_secure();
	if(ret){
		soc_attr = "secure";
	}else{
		soc_attr = "normal";
	}
	seq_printf(s, "%s\n", soc_attr);
	return 0;
}

static int soc_version_show(struct seq_file *s, void *d)
{
	int ret;
	ret = sunxi_get_soc_ver();
	seq_printf(s, "%x\n",ret);
	return 0;
}

static int soc_is_secure_open(struct inode *inode, struct file *file)
{
	return single_open(file, soc_is_secure_show, inode->i_private);
}
static int soc_version_open(struct inode *inode, struct file *file)
{
	return single_open(file, soc_version_show, inode->i_private);
}


static const struct file_operations soc_is_secure_ops = {
	.open		= soc_is_secure_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};
static const struct file_operations soc_version_ops = {
	.open		= soc_version_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.owner		= THIS_MODULE,
};

static struct dentry *debugfs_root;
static int sunxi_soc_info_debugfs(void)
{
	int ret = 0;
	debugfs_root = debugfs_create_dir("sunxi_soc_info", NULL);
	if (IS_ERR(debugfs_root) || !debugfs_root) {
		debugfs_root = NULL;
		ret = -1;
		return ret;
	}
	debugfs_create_file("soc_is_secure",S_IRUGO,
				debugfs_root, NULL, &soc_is_secure_ops);
	debugfs_create_file("soc_version",  S_IRUGO,
			   	debugfs_root, NULL, &soc_version_ops);

	return ret;
}
#endif

static unsigned int sunxi_get_platform(char *buf)
{
#if (defined CONFIG_ARCH_SUN8IW1P1)
	return sprintf(buf, "%s", "Sun8iw1p1");
#elif (defined CONFIG_ARCH_SUN8IW3P1)
	return sprintf(buf, "%s", "Sun8iw3p1");
#elif (defined CONFIG_ARCH_SUN8IW5P1)
	return sprintf(buf, "%s", "Sun8iw5p1");
#elif (defined CONFIG_ARCH_SUN8IW6P1)
	return sprintf(buf, "%s", "Sun8iw6p1");
#elif (defined CONFIG_ARCH_SUN8IW7P1)
	return sprintf(buf, "%s", "Sun8iw7p1");
#elif (defined CONFIG_ARCH_SUN8IW8P1)
	return sprintf(buf, "%s", "Sun8iw8p1");
#elif (defined CONFIG_ARCH_SUN9IW1P1)
	return sprintf(buf, "%s", "Sun9iw1p1");
#endif
}

ssize_t sys_info_show(struct class *class, struct class_attribute *attr, char *buf)
{
	int i;
	int databuf[4];
	char tmpbuf[129];
	size_t size = 0;

	/* platform */
	sunxi_get_platform(tmpbuf);
	size += sprintf(buf + size, "sunxi_platform    : %s\n", tmpbuf);

	/* secure */
	size += sprintf(buf + size, "sunxi_secure      : ");
	if (sunxi_soc_is_secure())
		size += sprintf(buf + size, "%s\n", "secure");
	else
		size += sprintf(buf + size, "%s\n", "normal");

	/* chipid */
	sunxi_get_serial((u8 *)databuf);
	for (i = 0; i < 4; i++)
		sprintf(tmpbuf + i*8, "%08x", databuf[i]);
	tmpbuf[128] = 0;
	size += sprintf(buf + size, "sunxi_chipid      : %s\n", tmpbuf);

	/* chiptype */
	sunxi_get_soc_chipid_str(tmpbuf);
	size += sprintf(buf + size, "sunxi_chiptype    : %s\n", tmpbuf);

	/* socbatch number */
	size += sprintf(buf + size, "sunxi_batchno     : %x\n", \
	                sunxi_get_soc_ver()&0x0ffff);
	/* Board vendor id*/
	databuf[0] = sunxi_get_board_vendor_id();
	size += sprintf(buf + size, "sunxi_board_id    : %d(%d)\n", (databuf[0]<0)?(-1):(databuf[0]&~(0xe0)), (databuf[0]<0)?(-1):((databuf[0]>>5)&0x01));

	return size;
}

static struct class_attribute info_class_attrs[] = {
	__ATTR(sys_info, 0644, sys_info_show, NULL),
	__ATTR_NULL,
};

static struct class info_class = {
	.name           = "sunxi_info",
	.owner          = THIS_MODULE,
	.class_attrs    = info_class_attrs,
};

static int __init sunxi_info_init(void)
{
	int status;

	status = class_register(&info_class);
	if(status < 0)
		pr_err("%s err, status %d\n", __func__, status);
	else
		pr_debug("%s success\n", __func__);

	return status;
}

static int __init sunxi_get_soc_info_init(void)
{
	int ret;

	pr_debug("sunxi get soc info driver init\n");
	ret = sunxi_info_init();
	if(ret)
		return ret;
	ret = misc_register(&soc_info_device);
	if(ret) {
		pr_err("%s: cannot register miscdev.(return value-%d)\n", __func__, ret);
		return ret;
	}
#ifdef CONFIG_DEBUG_FS
	sunxi_soc_info_debugfs();
#endif

	return ret;
}

static void __exit sunxi_get_soc_info_exit(void)
{
	int ret;
	pr_debug("sunxi get soc info driver exit\n");
	ret = misc_deregister(&soc_info_device);
	if(ret) {
		pr_err("%s: cannot deregister miscdev.(return value-%d)\n", __func__, ret);
	}
#ifdef CONFIG_DEBUG_FS
	if (debugfs_root) {
		debugfs_remove_recursive(debugfs_root);
		debugfs_root = NULL;
	}
#endif
}

module_init(sunxi_get_soc_info_init);
module_exit(sunxi_get_soc_info_exit);
MODULE_LICENSE     ("GPL");
MODULE_AUTHOR      ("huangshr");
MODULE_DESCRIPTION ("get sunxi soc infomation");


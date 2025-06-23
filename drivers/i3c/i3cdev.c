// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 Synopsys, Inc. and/or its affiliates.
 *
 * Author: Vitor Soares <soares@synopsys.com>
 */

#include <linux/cdev.h>
#include <linux/compat.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/i3c/i3cdev.h>

#include "internals.h"

#define I3C_MINORS	MINORMASK
#define N_I3C_MINORS	16 /* For now */

extern const struct device_type i3c_masterdev_type;

static DECLARE_BITMAP(minors, N_I3C_MINORS);

struct i3cdev_data {
	struct list_head list;
	struct i3c_device *i3c;
	struct cdev cdev;
	struct device *dev;
	dev_t devt;
};

static dev_t i3cdev_number; /* Alloted device number */

static LIST_HEAD(i3cdev_list);
static DEFINE_SPINLOCK(i3cdev_list_lock);

static struct i3cdev_data *i3cdev_get_by_minor(unsigned int minor)
{
	struct i3cdev_data *i3cdev;

	spin_lock(&i3cdev_list_lock);
	list_for_each_entry(i3cdev, &i3cdev_list, list) {
		if (MINOR(i3cdev->devt) == minor)
			goto found;
	}

	i3cdev = NULL;

found:
	spin_unlock(&i3cdev_list_lock);
	return i3cdev;
}

static struct i3cdev_data *i3cdev_get_by_i3c(struct i3c_device *i3c)
{
	struct i3cdev_data *i3cdev;

	spin_lock(&i3cdev_list_lock);
	list_for_each_entry(i3cdev, &i3cdev_list, list) {
		if (i3cdev->i3c == i3c)
			goto found;
	}

	i3cdev = NULL;

found:
	spin_unlock(&i3cdev_list_lock);
	return i3cdev;
}

static struct i3cdev_data *get_free_i3cdev(struct i3c_device *i3c)
{
	struct i3cdev_data *i3cdev;
	unsigned long minor;

	minor = find_first_zero_bit(minors, N_I3C_MINORS);
	if (minor >= N_I3C_MINORS) {
		pr_err("i3cdev: no minor number available!\n");
		return ERR_PTR(-ENODEV);
	}

	i3cdev = kzalloc(sizeof(*i3cdev), GFP_KERNEL);
	if (!i3cdev)
		return ERR_PTR(-ENOMEM);

	i3cdev->i3c = i3c;
	i3cdev->devt = MKDEV(MAJOR(i3cdev_number), minor);
	set_bit(minor, minors);

	spin_lock(&i3cdev_list_lock);
	list_add_tail(&i3cdev->list, &i3cdev_list);
	spin_unlock(&i3cdev_list_lock);

	return i3cdev;
}

static void put_i3cdev(struct i3cdev_data *i3cdev)
{
	spin_lock(&i3cdev_list_lock);
	list_del(&i3cdev->list);
	spin_unlock(&i3cdev_list_lock);
	kfree(i3cdev);
}

static ssize_t
i3cdev_read(struct file *file, char __user *buf, size_t count, loff_t *f_pos)
{
	struct i3c_device *i3c = file->private_data;
	struct i3c_priv_xfer xfers = {
		.rnw = true,
		.len = count,
	};
	char *tmp;
	int ret;

	tmp = kzalloc(count, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;

	xfers.data.in = tmp;

	dev_info(&i3c->dev, "Reading %zu bytes.\n", count);

	ret = i3c_device_do_priv_xfers(i3c, &xfers, 1);
	if (!ret)
		ret = copy_to_user(buf, tmp, count) ? -EFAULT : ret;

	kfree(tmp);
	return ret;
}

static ssize_t
i3cdev_write(struct file *file, const char __user *buf, size_t count,
	     loff_t *f_pos)
{
	struct i3c_device *i3c = file->private_data;
	struct i3c_priv_xfer xfers = {
		.rnw = false,
		.len = count,
	};
	char *tmp;
	int ret;

	tmp = memdup_user(buf, count);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	xfers.data.out = tmp;

	dev_info(&i3c->dev, "Writing %zu bytes.\n", count);

	ret = i3c_device_do_priv_xfers(i3c, &xfers, 1);
	kfree(tmp);
	return (!ret) ? count : ret;
}

static int
i3cdev_do_priv_xfer_wrap(struct i3c_device *dev, struct i3c_priv_xfer *xfers,
		    unsigned int nxfers)
{
	void __user **data_ptrs;
	unsigned int i;
	int ret = 0;

	data_ptrs = kmalloc_array(nxfers, sizeof(u8 __user *), GFP_KERNEL);
	if (!data_ptrs)
		return -ENOMEM;
	for (i = 0; i < nxfers; i++) {
		if (xfers[i].rnw) {
			data_ptrs[i] = (void __user *)xfers[i].data.in;

			xfers[i].data.in = memdup_user(data_ptrs[i],
						       xfers[i].len);
			if (IS_ERR(xfers[i].data.in)) {
				ret = PTR_ERR(xfers[i].data.in);
				break;
			}
		} else {
			data_ptrs[i] = (void __user *)xfers[i].data.out;
			xfers[i].data.out = memdup_user(data_ptrs[i],
							xfers[i].len);
			if (IS_ERR(xfers[i].data.out)) {
				ret = PTR_ERR(xfers[i].data.out);
				break;
			}
		}
	}
	if (ret < 0) {
		unsigned int j;

		for (j = 0; j < i; ++j) {
			if (xfers[j].rnw)
				kfree(xfers[j].data.in);
			else
				kfree(xfers[j].data.out);
		}
		kfree(data_ptrs);
		return ret;
	}

	ret = i3c_device_do_priv_xfers(dev, xfers, nxfers);
	while (i-- > 0) {
		if (ret >= 0 && xfers[i].rnw) {
			if (copy_to_user(data_ptrs[i], xfers[i].data.in,
					 xfers[i].len))
				ret = -EFAULT;
		}

		if (xfers[i].rnw)
			kfree(xfers[i].data.in);
		else
			kfree(xfers[i].data.out);
	}

	kfree(data_ptrs);
	return ret;
}

static int
i3cdev_transfer_priv_xfer(struct i3c_device *i3c,
		     struct i3c_ioc_priv_xfer __user *u_ioc_xfers)
{
	struct i3c_ioc_priv_xfer k_ioc_xfer;
	struct i3c_priv_xfer *xfers;
	int ret;

	if (copy_from_user(&k_ioc_xfer, u_ioc_xfers, sizeof(k_ioc_xfer)))
		return -EFAULT;

	xfers = memdup_user(k_ioc_xfer.xfers,
			    k_ioc_xfer.nxfers * sizeof(struct i3c_priv_xfer));
	if (IS_ERR(xfers))
		return PTR_ERR(xfers);

	ret = i3cdev_do_priv_xfer_wrap(i3c, xfers, k_ioc_xfer.nxfers);

	kfree(xfers);

	return ret;
}

static long
i3cdev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct i3c_device *i3c = file->private_data;

	if (_IOC_TYPE(cmd) != I3C_DEV_IOC_MAGIC)
		return -ENOTTY;
	if (cmd == I3C_IOC_PRIV_XFER)
		return i3cdev_transfer_priv_xfer(i3c,
					(struct i3c_ioc_priv_xfer __user *)arg);

	return 0;
}

static int i3cdev_open(struct inode *inode, struct file *file)
{
	unsigned int minor = iminor(inode);
	struct i3cdev_data *i3cdev;

	i3cdev = i3cdev_get_by_minor(minor);
	if (!i3cdev)
		return -ENODEV;

	file->private_data = i3cdev->i3c;

	return 0;
}

static int i3cdev_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;

	return 0;
}

static const struct file_operations i3cdev_fops = {
	.owner		= THIS_MODULE,
	.read		= i3cdev_read,
	.write		= i3cdev_write,
	.unlocked_ioctl	= i3cdev_ioctl,
	.open		= i3cdev_open,
	.release	= i3cdev_release,
};

/* ------------------------------------------------------------------------- */

static struct class *i3cdev_class;

static int i3cdev_attach(struct device *dev, void *dummy)
{
	struct i3c_device *i3c;
	struct i3cdev_data *i3cdev;
	int res;

	if (dev->type == &i3c_masterdev_type || dev->driver)
		return 0;

	i3c = dev_to_i3cdev(dev);

	/* Get a device */
	i3cdev = get_free_i3cdev(i3c);
	if (IS_ERR(i3cdev))
		return PTR_ERR(i3cdev);

	cdev_init(&i3cdev->cdev, &i3cdev_fops);
	i3cdev->cdev.owner = THIS_MODULE;
	res = cdev_add(&i3cdev->cdev, i3cdev->devt, 1);
	if (res)
		goto error_cdev;

	/* register this i3c device with the driver core */
	i3cdev->dev = device_create(i3cdev_class, &i3c->dev,
				    i3cdev->devt, NULL,
				    "i3c-%s", dev_name(&i3c->dev));
	if (IS_ERR(i3cdev->dev)) {
		res = PTR_ERR(i3cdev->dev);
		goto error;
	}
	pr_info("i3c-cdev: I3C device [%s] registered as minor %d\n",
		 dev_name(&i3c->dev), MINOR(i3cdev->devt));
	return 0;

error:
	cdev_del(&i3cdev->cdev);
error_cdev:
	put_i3cdev(i3cdev);
	return res;
}

static int i3cdev_detach(struct device *dev, void *dummy)
{
	struct i3c_device *i3c;
	struct i3cdev_data *i3cdev;

	if (dev->type == &i3c_masterdev_type)
		return 0;

	i3c = dev_to_i3cdev(dev);

	i3cdev = i3cdev_get_by_i3c(i3c);
	if (!i3cdev)
		return 0;

	clear_bit(MINOR(i3cdev->devt), minors);
	cdev_del(&i3cdev->cdev);
	device_destroy(i3cdev_class, i3cdev->devt);
	put_i3cdev(i3cdev);
	pr_info("i3c-busdev: bus [%s] unregistered\n",
		 dev_name(&i3c->dev));

	return 0;
}

static int i3cdev_notifier_call(struct notifier_block *nb,
				unsigned long action,
				void *data)
{
	struct device *dev = data;

	switch (action) {
	case BUS_NOTIFY_ADD_DEVICE:
	case BUS_NOTIFY_BOUND_DRIVER:
		return i3cdev_attach(dev, NULL);
	case BUS_NOTIFY_DEL_DEVICE:
	case BUS_NOTIFY_UNBOUND_DRIVER:
		return i3cdev_detach(dev, NULL);
	}

	return 0;
}

static struct notifier_block i3c_notifier = {
	.notifier_call = i3cdev_notifier_call,
};

static int __init i3cdev_init(void)
{
	int res;

	/* Dynamically request unused major number */
	res = alloc_chrdev_region(&i3cdev_number, 0, N_I3C_MINORS, "i3c");
	if (res)
		goto out;

	/* Create a classe to populate sysfs entries*/
	i3cdev_class = class_create(THIS_MODULE, "i3c-dev");
	if (IS_ERR(i3cdev_class)) {
		res = PTR_ERR(i3cdev_class);
		goto out_unreg_chrdev;
	}

	/* Keep track of busses which have devices to add or remove later */
	res = bus_register_notifier(&i3c_bus_type, &i3c_notifier);
	if (res)
		goto out_unreg_class;

	/* Bind to already existing device without driver right away */
	i3c_for_each_dev(NULL, i3cdev_attach);

	return 0;
out_unreg_class:
	class_destroy(i3cdev_class);
out_unreg_chrdev:
	unregister_chrdev_region(i3cdev_number, I3C_MINORS);
out:
	pr_err("%s: Driver Initialisation failed\n", __FILE__);
	return res;
}

static void __exit i3cdev_exit(void)
{
	bus_unregister_notifier(&i3c_bus_type, &i3c_notifier);
	i3c_for_each_dev(NULL, i3cdev_detach);
	class_destroy(i3cdev_class);
	unregister_chrdev_region(i3cdev_number, I3C_MINORS);
}

MODULE_AUTHOR("Vitor Soares <Vitor.Soares@synopsys.com>");
MODULE_DESCRIPTION("I3C /dev entries driver");
MODULE_LICENSE("GPLv2");

module_init(i3cdev_init);
module_exit(i3cdev_exit);

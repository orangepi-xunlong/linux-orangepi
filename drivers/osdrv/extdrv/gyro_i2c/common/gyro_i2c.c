/*
 * Copyright (C) Cvitek Co., Ltd. 2022-2023. All rights reserved.
 *
 * File Name: gyro_i2c.c
 * Description: gyro kernel space driver entry related code

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include <linux/compat.h>
#include <linux/iommu.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/signal.h>
#endif

#include "mpu9250.h"
#include "linux/cvi_gyro_ioctl.h"
#include "gyro_i2c.h"
#include "pinctrl-cv181x.h"

#define CVI_GYRO_CDEV_NAME "cvi-gyro"
#define CVI_GYRO_CLASS_NAME "cvi-gyro"

struct cvi_gy_device ndev;

void cvi_pinmux_switch(void)
{
	PINMUX_CONFIG(IIC2_SCL, IIC2_SCL);
	PINMUX_CONFIG(IIC2_SDA, IIC2_SDA);
}

// proc_operations function
static int gyro_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, "[GYRO] gyro log have not implemented yet\n");
	return 0;
}

static ssize_t proc_gy_write(struct file *file, const char __user *user_buf,
				  size_t count, loff_t *ppos)
{
	int16_t tmp;
	struct sAxis a;
	uint32_t user_input_param = 0;

	if (kstrtouint_from_user(user_buf, count, 0, &user_input_param)) {
		pr_err("\n[GYRO] input parameter incorrect\n");
		return count;
	}

	pr_err("\n[GYRO] get sel %d\n", user_input_param);

	if (user_input_param == 0) {
		set_acc_scale(scale_2g);//set accelerometer scale
		set_gyro_scale(scale_250dps);//set gyroscope scale
		pr_err("\n[GYRO] set accelerometer and gyroscope scale\n");
	} else if (user_input_param == 1) {
		cvi_gy_adj_acc_offset(acc_5Hz, gyro_5Hz);
		pr_err("\n[GYRO] adjust acc offset\n");
	} else if (user_input_param == 2) {
		cvi_gy_adj_gyro_offset(acc_5Hz, gyro_5Hz);
		pr_err("\n[GYRO] adjust gyro offset\n");
	} else if (user_input_param == 3) {
		read_acc(&a);
		pr_err("\n[GYRO] acc x = %d, y = %d, z = %d\n", (int)a.x, (int)a.y, (int)a.z);
	} else if (user_input_param == 4) {
		read_gyro(&a);
		pr_err("\n[GYRO] gyro x = %d, y = %d, z = %d\n", a.x, a.y, a.z);
	} else if (user_input_param == 5) {
		read_mag(&a);
		pr_err("\n[GYRO] mag x = %d, y = %d, z = %d\n", a.x, a.y, a.z);
	} else if (user_input_param == 6) {
		tmp = read_temp();
		pr_err("\n[GYRO] temp = %d\n", tmp);
	} else if (user_input_param == 7) {
		cvi_gy_reset(ndev.client);
		pr_err("\n[GYRO] reset\n");
	}

	read_acc(&a);
	pr_err("\n[GYRO] acc x = %d, y = %d, z = %d\n", a.x, a.y, a.z);
	return count;
}

static int proc_gy_open(struct inode *inode, struct file *file)
{
	return single_open(file, gyro_proc_show, PDE_DATA(inode));
}

#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
static const struct proc_ops gyro_proc_ops = {
	.proc_open = proc_gy_open,
	.proc_read = seq_read,
	.proc_write = proc_gy_write,
	.proc_release = single_release,
};
#else
static const struct file_operations gyro_proc_ops = {
	.owner = THIS_MODULE,
	.open = proc_gy_open,
	.read = seq_read,
	.write = proc_gy_write,
	.release = single_release,
};
#endif

static long fp_gy_ioctl(struct file *filp, unsigned int cmd,
			  unsigned long arg)
{
	struct cvi_gy_device *ndev = filp->private_data;
	struct cvi_gy_regval reg;
	long ret;

	if (ndev == NULL)
		return -EBADF;

	switch (cmd) {

	case CVI_GYRO_IOC_READ: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0) {
			reg.val = mpu9250_read(ndev->client, reg.addr);
			ret = copy_to_user((void __user *)arg, &reg, sizeof(reg));
		}
	} break;
	case CVI_GYRO_IOC_READ_2BYTE: {
		uint8_t rawData[2];

		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0) {
			mpu9250_readArray(ndev->client, reg.addr, rawData, 2);
			reg.val = ((int16_t)rawData[0] << 8) | rawData[1];
			ret = copy_to_user((void __user *)arg, &reg, sizeof(reg));
		}
	} break;
	case CVI_GYRO_IOC_READ_6BYTE: {
		struct cvi_gy_regval_6byte reg_6byte;
		uint8_t rawData[6];

		if (copy_from_user(&reg_6byte, (void __user *)arg, sizeof(reg_6byte)) == 0) {
			mpu9250_readArray(ndev->client, reg_6byte.addr, rawData, 6);
			reg_6byte.x_val = ((int16_t)rawData[0] << 8) | rawData[1];
			reg_6byte.y_val = ((int16_t)rawData[2] << 8) | rawData[3];
			reg_6byte.z_val = ((int16_t)rawData[4] << 8) | rawData[5];
			ret = copy_to_user((void __user *)arg, &reg_6byte, sizeof(reg_6byte));
		}
	} break;
	case CVI_GYRO_IOC_WRITE: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0) {
			ret = mpu9250_write(ndev->client, reg.addr, reg.val);
		}
	} break;
	case CVI_GYRO_IOC_WRITE_OR: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0) {
			ret = mpu9250_write_OR(ndev->client, reg.addr, reg.val);
		}
	} break;
	case CVI_GYRO_IOC_WRITE_AND: {
		if (copy_from_user(&reg, (void __user *)arg, sizeof(reg)) == 0) {
			ret = mpu9250_write_AND(ndev->client, reg.addr, reg.val);
		}
	} break;
	case CVI_GYRO_IOC_CHECK: {
		reg.addr = 0x75;
		reg.val = mpu9250_read(ndev->client, WHO_AM_I);
		ret = copy_to_user((void __user *)arg, &reg, sizeof(reg));
	} break;
	case CVI_GYRO_IOC_ADJUST: {
		cvi_gy_adj_gyro_offset(acc_5Hz, gyro_5Hz);
	} break;
	case CVI_GYRO_IOC_ACC_ADJUST: {
		cvi_gy_adj_acc_offset(acc_5Hz, gyro_5Hz);
	} break;
	default:
		return -ENOTTY;
	}
	return ret;
}

static int fp_gy_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &ndev;
	return 0;
}

static int fp_gy_close(struct inode *inode, struct file *filp)
{
	filp->private_data = NULL;
	return 0;
}

static const struct file_operations gyro_fops = {
	.owner = THIS_MODULE,
	.open = fp_gy_open,
	.release = fp_gy_close,
	.unlocked_ioctl = fp_gy_ioctl, //2.6.36
#ifdef CONFIG_COMPAT
	.compat_ioctl = fp_gy_compat_ioctl, //2.6.36
#endif
};

static int __init cvi_gyro_probe(void)
{
	int ret = 0;

	spin_lock_init(&ndev.lock);

	/* 2. register dev number */
	if (ndev.major) {
		ndev.devid = MKDEV(ndev.major, 0);
		register_chrdev_region(ndev.devid, 1, CVI_GYRO_CDEV_NAME);
	} else {
		ret = alloc_chrdev_region(&ndev.devid, 0, 1, CVI_GYRO_CDEV_NAME);
		ndev.major = MAJOR(ndev.devid);
		ndev.minor = MINOR(ndev.devid);
	}

	if (ret < 0) {
		pr_err("[GYRO] chr_dev region err\n");
		return -1;
	}

	/* 3. register chardev */
	ndev.cdev.owner = THIS_MODULE;
	cdev_init(&ndev.cdev, &gyro_fops);
	ret = cdev_add(&ndev.cdev, ndev.devid, 1);
	if (ret < 0) {
		pr_err("[GYRO] cdev_add err\n");
		return -1;
	}

	/* 4. auto create device node */
	ndev.class = class_create(THIS_MODULE, CVI_GYRO_CLASS_NAME);
	if (IS_ERR(ndev.class)) {
		pr_err("[GYRO] create class failed\n");
		return PTR_ERR(ndev.class);
	}
	ndev.device = device_create(ndev.class, NULL, ndev.devid, NULL, CVI_GYRO_CDEV_NAME);
	if (IS_ERR(ndev.device)) {
		return PTR_ERR(ndev.device);
	}

	// Create gyro proc descript
	ndev.proc_dir = proc_mkdir("gyro", NULL);
	if (proc_create_data("hw_profiling", 0644, ndev.proc_dir,
				 &gyro_proc_ops, &ndev) == NULL)
		pr_err("[GYRO] gyro hw_profiling proc creation failed\n");

	cvi_pinmux_switch();

	ndev.client = cvi_gy_open("/dev/i2c-2");
	cvi_gy_reset(ndev.client);

	return 0;
}

static void __exit cvi_gyro_remove(void)
{
	/* 1. colse mpu9250 */
	cvi_gy_close();

	/* 2. unregister */
	cdev_del(&ndev.cdev);

	unregister_chrdev_region(ndev.devid, 1);

	/* 3. destroy device */
	device_destroy(ndev.class, ndev.devid);

	/* 4. dectroy class */
	class_destroy(ndev.class);

	// remove gyro proc
	proc_remove(ndev.proc_dir);
}

module_init(cvi_gyro_probe);
module_exit(cvi_gyro_remove);

MODULE_DESCRIPTION("Cvitek GYRO Driver");
MODULE_AUTHOR("Ken Lin<ken.lin@cvitek.com>");
MODULE_LICENSE("GPL");
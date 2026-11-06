#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>  /* for put_user */
#include <linux/compat.h>

#include "cviusb_misc.h"
#include "cviusb_drd.h"

/*-------------------------------------------------------------------------*/
/* Miscellaneous device for DRD */

/*
 * Weak declarations for DRD:
 * - should be defined in DRD driver and overwrite below
 * - shall not be used in gadget driver
 */
int __attribute__((weak)) cviusb_otg_standby_allowed(struct cviusb_dev *cviusb)
{
	return -EPERM;
}

int __attribute__((weak)) cviusb_otg_set_standby(struct cviusb_dev *cviusb)
{
	return -EPERM;
}

int __attribute__((weak)) cviusb_otg_clear_standby(struct cviusb_dev *cviusb)
{
	return -EPERM;
}

void __attribute__((weak)) cviusb_set_state(struct otg_fsm *fsm,
					enum usb_otg_state new_state)
{
}

void __attribute__((weak)) cviusb_otg_fsm_sync(struct cviusb_dev *cviusb)
{
}

static int cviusb_drd_open(struct inode *inode, struct file *file)
{
	int res;
	struct cviusb_drd_misc *cviusb_misc =
		container_of(file->private_data,
			     struct cviusb_drd_misc, miscdev);
	struct cviusb_dev *cviusb =
		container_of(cviusb_misc, struct cviusb_dev, cviusb_misc);

	res = cviusb_otg_standby_allowed(cviusb);
	if (res < 0) {
		cviusb_err(cviusb->dev, "Can't open because of lack of symbols!\n");
		return res;
	}
	return 0;
}


static ssize_t cviusb_drd_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct cviusb_drd_misc *cviusb_misc =
		container_of(file->private_data, struct cviusb_drd_misc,
			     miscdev);
	struct cviusb_dev *cviusb =
		container_of(cviusb_misc, struct cviusb_dev, cviusb_misc);
	long int cmd;
	int res;

	if (buf == NULL)
		return -ENOMEM;

	res = kstrtol(buf, 10, &cmd);
	if (res)
		return res;
	cviusb_set_state(cviusb->fsm, cmd);
	return 1;
}

static ssize_t cviusb_drd_read(struct file *file, char __user *buf,
						size_t count, loff_t *ppos)
{
	struct cviusb_drd_misc *cviusb_misc =
		container_of(file->private_data, struct cviusb_drd_misc,
			     miscdev);
	struct cviusb_dev *cviusb =
		container_of(cviusb_misc, struct cviusb_dev, cviusb_misc);
	int len, i;
	char kernel_buf[5], *kernel_ptr;

	/* EOF */
	if (*ppos > 0)
		return 0;

	sprintf(kernel_buf, "%d", cviusb->fsm->otg->state);
	len = strlen(kernel_buf);
	kernel_ptr = kernel_buf;
	for (i = 0; i < len; i++)
		put_user(*(kernel_ptr++), buf++);
	(*ppos)++;
	return len;
}

static long cviusb_drd_unlocked_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct cviusb_drd_misc *cviusb_misc =
		container_of(file->private_data,
			     struct cviusb_drd_misc, miscdev);
	struct cviusb_dev *cviusb =
		container_of(cviusb_misc, struct cviusb_dev, cviusb_misc);
	int status;

	switch (cmd) {
	case CVI_DRD_SET_A_IDLE:
		if (cviusb->fsm->otg->state != OTG_STATE_A_HOST)
			return -EPERM;
		cviusb->cviusb_misc.force_a_idle = 1;
		break;
	case CVI_DRD_SET_B_SRP_INIT:
		if (cviusb->fsm->otg->state != OTG_STATE_B_IDLE)
			return -EPERM;
		cviusb->cviusb_misc.force_b_srp_init = 3;
		break;
	case CVI_DRD_GET_STATE:
		if (copy_to_user(
				(int *)arg,
				&cviusb->fsm->otg->state,
				sizeof(cviusb->fsm->otg->state))
			) {
			return -EACCES;
		}
		return 0;
	case CVI_DRD_SET_HNP_REQ:
		if (cviusb->fsm->otg->state != OTG_STATE_B_PERIPHERAL)
			return -EPERM;
		if (!cviusb->fsm->otg->gadget)
			return -EPERM;
		cviusb->fsm->otg->gadget->host_request_flag = 1;
		break;
	case CVI_DRD_STB_ALLOWED:
		status = cviusb_otg_standby_allowed(cviusb);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	case CVI_DRD_SET_STB:
		status = cviusb_otg_set_standby(cviusb);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	case CVI_DRD_CLEAR_STB:
		status = cviusb_otg_clear_standby(cviusb);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	default:
		return -ENOTTY;
	}
	cviusb_otg_fsm_sync(cviusb);
	return 0;
}

#ifdef CONFIG_COMPAT
static long cviusb_misc_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations cviusb_drd_file_ops = {
		.owner = THIS_MODULE,
		.open = cviusb_drd_open,
		.read = cviusb_drd_read,
		.write = cviusb_drd_write,
		.unlocked_ioctl = cviusb_drd_unlocked_ioctl,
#ifdef CONFIG_COMPAT
		.compat_ioctl = cviusb_misc_compat_ptr_ioctl,
#endif
};

void cviusb_drd_misc_register(struct cviusb_dev *cviusb, int res_address)
{
	struct miscdevice *miscdev;

	miscdev = &cviusb->cviusb_misc.miscdev;
	miscdev->minor = MISC_DYNAMIC_MINOR;
	miscdev->name = kasprintf(GFP_KERNEL, "cviusb-otg-%x", res_address);
	miscdev->fops = &cviusb_drd_file_ops;
	misc_register(miscdev);
}

/*-------------------------------------------------------------------------*/
/* Miscellaneous device for gadget */

/*
 * Weak declarations for gadget:
 * - should be defined in gadget driver and overwrite below
 * - shall not be used in DRD driver
 */
int __attribute__((weak)) gadget_is_stb_allowed(struct usb_ss_dev *usb_ss)
{
	return -EPERM;
}

int __attribute__((weak)) gadget_enter_stb_request(struct usb_ss_dev *usb_ss)
{
	return -EPERM;
}

int __attribute__((weak)) gadget_exit_stb_request(struct usb_ss_dev *usb_ss)
{
	return -EPERM;
}
static int cviusb_dev_open(struct inode *inode, struct file *file)
{
	int res;
	struct usb_ss_dev *usb_ss =
		container_of(file->private_data, struct usb_ss_dev, miscdev);

	res = gadget_is_stb_allowed(usb_ss);
	if (res < 0) {
		cviusb_err(usb_ss->dev, "Can't open because of lack of symbols!\n");
		return res;
	}
	return 0;
}

static long cviusb_dev_unlocked_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct usb_ss_dev *usb_ss =
		container_of(file->private_data, struct usb_ss_dev, miscdev);
	int status = 0;

	switch (cmd) {
	case CVI_DEV_STB_ALLOWED:
		status = gadget_is_stb_allowed(usb_ss);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	case CVI_DEV_SET_STB:
		status = gadget_enter_stb_request(usb_ss);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	case CVI_DEV_CLEAR_STB:
		status = gadget_exit_stb_request(usb_ss);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	}

	return 0;
}

static const struct file_operations cviusb_dev_file_ops = {
		.owner = THIS_MODULE,
		.open = cviusb_dev_open,
		.unlocked_ioctl = cviusb_dev_unlocked_ioctl,
#ifdef CONFIG_COMPAT
		.compat_ioctl = cviusb_misc_compat_ptr_ioctl,
#endif
};

void cviusb_dev_misc_register(struct usb_ss_dev *usb_ss, int res_address)
{
	struct miscdevice *miscdev;

	miscdev = &usb_ss->miscdev;
	miscdev->minor = MISC_DYNAMIC_MINOR;
	miscdev->name = kasprintf(GFP_KERNEL, "cviusb-dev-%x", res_address);
	miscdev->fops = &cviusb_dev_file_ops;
	misc_register(miscdev);
}

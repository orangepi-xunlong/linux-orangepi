#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>  /* for put_user */
#include <linux/compat.h>

#include "bmusb_misc.h"
#include "bmusb_drd.h"

/*-------------------------------------------------------------------------*/
/* Miscellaneous device for DRD */

/*
 * Weak declarations for DRD:
 * - should be defined in DRD driver and overwrite below
 * - shall not be used in gadget driver
 */
int __attribute__((weak)) bmusb_otg_standby_allowed(struct bmusb_dev *bmusb)
{
	return -EPERM;
}

int __attribute__((weak)) bmusb_otg_set_standby(struct bmusb_dev *bmusb)
{
	return -EPERM;
}

int __attribute__((weak)) bmusb_otg_clear_standby(struct bmusb_dev *bmusb)
{
	return -EPERM;
}

void __attribute__((weak)) bmusb_set_state(struct otg_fsm *fsm,
					enum usb_otg_state new_state)
{
}

void __attribute__((weak)) bmusb_otg_fsm_sync(struct bmusb_dev *bmusb)
{
}

static int bmusb_drd_open(struct inode *inode, struct file *file)
{
	int res;
	struct bmusb_drd_misc *bmusb_misc =
		container_of(file->private_data,
			     struct bmusb_drd_misc, miscdev);
	struct bmusb_dev *bmusb =
		container_of(bmusb_misc, struct bmusb_dev, bmusb_misc);

	res = bmusb_otg_standby_allowed(bmusb);
	if (res < 0) {
		bmusb_err(bmusb->dev, "Can't open because of lack of symbols!\n");
		return res;
	}
	return 0;
}


static ssize_t bmusb_drd_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct bmusb_drd_misc *bmusb_misc =
		container_of(file->private_data, struct bmusb_drd_misc,
			     miscdev);
	struct bmusb_dev *bmusb =
		container_of(bmusb_misc, struct bmusb_dev, bmusb_misc);
	long int cmd;
	int res;

	if (buf == NULL)
		return -ENOMEM;

	res = kstrtol(buf, 10, &cmd);
	if (res)
		return res;
	bmusb_set_state(bmusb->fsm, cmd);
	return 1;
}

static ssize_t bmusb_drd_read(struct file *file, char __user *buf,
						size_t count, loff_t *ppos)
{
	struct bmusb_drd_misc *bmusb_misc =
		container_of(file->private_data, struct bmusb_drd_misc,
			     miscdev);
	struct bmusb_dev *bmusb =
		container_of(bmusb_misc, struct bmusb_dev, bmusb_misc);
	int len, i;
	char kernel_buf[5], *kernel_ptr;

	/* EOF */
	if (*ppos > 0)
		return 0;

	sprintf(kernel_buf, "%d", bmusb->fsm->otg->state);
	len = strlen(kernel_buf);
	kernel_ptr = kernel_buf;
	for (i = 0; i < len; i++)
		put_user(*(kernel_ptr++), buf++);
	(*ppos)++;
	return len;
}

static long bmusb_drd_unlocked_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct bmusb_drd_misc *bmusb_misc =
		container_of(file->private_data,
			     struct bmusb_drd_misc, miscdev);
	struct bmusb_dev *bmusb =
		container_of(bmusb_misc, struct bmusb_dev, bmusb_misc);
	int status;

	switch (cmd) {
	case BM_DRD_SET_A_IDLE:
		if (bmusb->fsm->otg->state != OTG_STATE_A_HOST)
			return -EPERM;
		bmusb->bmusb_misc.force_a_idle = 1;
		break;
	case BM_DRD_SET_B_SRP_INIT:
		if (bmusb->fsm->otg->state != OTG_STATE_B_IDLE)
			return -EPERM;
		bmusb->bmusb_misc.force_b_srp_init = 3;
		break;
	case BM_DRD_GET_STATE:
		if (copy_to_user(
				(int *)arg,
				&bmusb->fsm->otg->state,
				sizeof(bmusb->fsm->otg->state))
			) {
			return -EACCES;
		}
		return 0;
	case BM_DRD_SET_HNP_REQ:
		if (bmusb->fsm->otg->state != OTG_STATE_B_PERIPHERAL)
			return -EPERM;
		if (!bmusb->fsm->otg->gadget)
			return -EPERM;
		bmusb->fsm->otg->gadget->host_request_flag = 1;
		break;
	case BM_DRD_STB_ALLOWED:
		status = bmusb_otg_standby_allowed(bmusb);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	case BM_DRD_SET_STB:
		status = bmusb_otg_set_standby(bmusb);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	case BM_DRD_CLEAR_STB:
		status = bmusb_otg_clear_standby(bmusb);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	default:
		return -ENOTTY;
	}
	bmusb_otg_fsm_sync(bmusb);
	return 0;
}

#ifdef CONFIG_COMPAT
static long bmusb_misc_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations bmusb_drd_file_ops = {
		.owner = THIS_MODULE,
		.open = bmusb_drd_open,
		.read = bmusb_drd_read,
		.write = bmusb_drd_write,
		.unlocked_ioctl = bmusb_drd_unlocked_ioctl,
#ifdef CONFIG_COMPAT
		.compat_ioctl = bmusb_misc_compat_ptr_ioctl,
#endif
};

void bmusb_drd_misc_register(struct bmusb_dev *bmusb, int res_address)
{
	struct miscdevice *miscdev;

	miscdev = &bmusb->bmusb_misc.miscdev;
	miscdev->minor = MISC_DYNAMIC_MINOR;
	miscdev->name = kasprintf(GFP_KERNEL, "bmusb-otg-%x", res_address);
	miscdev->fops = &bmusb_drd_file_ops;
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
static int bmusb_dev_open(struct inode *inode, struct file *file)
{
	int res;
	struct usb_ss_dev *usb_ss =
		container_of(file->private_data, struct usb_ss_dev, miscdev);

	res = gadget_is_stb_allowed(usb_ss);
	if (res < 0) {
		bmusb_err(usb_ss->dev, "Can't open because of lack of symbols!\n");
		return res;
	}
	return 0;
}

static long bmusb_dev_unlocked_ioctl(struct file *file,
		unsigned int cmd, unsigned long arg)
{
	struct usb_ss_dev *usb_ss =
		container_of(file->private_data, struct usb_ss_dev, miscdev);
	int status = 0;

	switch (cmd) {
	case BM_DEV_STB_ALLOWED:
		status = gadget_is_stb_allowed(usb_ss);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	case BM_DEV_SET_STB:
		status = gadget_enter_stb_request(usb_ss);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	case BM_DEV_CLEAR_STB:
		status = gadget_exit_stb_request(usb_ss);
		if (copy_to_user((int *)arg, &status, sizeof(status)))
			return -EACCES;
		return 0;
	}

	return 0;
}

static const struct file_operations bmusb_dev_file_ops = {
		.owner = THIS_MODULE,
		.open = bmusb_dev_open,
		.unlocked_ioctl = bmusb_dev_unlocked_ioctl,
#ifdef CONFIG_COMPAT
		.compat_ioctl = bmusb_misc_compat_ptr_ioctl,
#endif
};

void bmusb_dev_misc_register(struct usb_ss_dev *usb_ss, int res_address)
{
	struct miscdevice *miscdev;

	miscdev = &usb_ss->miscdev;
	miscdev->minor = MISC_DYNAMIC_MINOR;
	miscdev->name = kasprintf(GFP_KERNEL, "bmusb-dev-%x", res_address);
	miscdev->fops = &bmusb_dev_file_ops;
	misc_register(miscdev);
}

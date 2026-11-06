#include <linux/types.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#include <linux/defines.h>
#include <linux/sys_uapi.h>
#include <linux/comm_sys.h>
#include <linux/compat.h>
#include "base_cb.h"
#include "vi_cb.h"
#include "vpss_cb.h"
#include "sys.h"
#include "sys_common.h"
#include "vi_sys.h"
#include "vo_sys.h"
#include "cdma.h"
#include "sys_debug.h"

#define SYS_DEV_NAME   "soph-sys"
#define SYS_CLASS_NAME "soph-sys"

u32 sys_log_lv = DBG_WARN;

struct sys_device {
	struct device *dev;
	struct miscdevice miscdev;
	struct mutex dev_lock;
	spinlock_t close_lock;
	atomic_t sys_inited;
	vi_vpss_mode_s vivpss_mode;
	int cdma_irq_num;
	int use_count;
};

module_param(sys_log_lv, int, 0644);

static irqreturn_t cdma_isr(int irq, void *data)
{
	cdma_irq_handler();

	return IRQ_HANDLED;
}

static int _sys_call_cb(u32 m_id, u32 cmd_id, void *data)
{
	struct base_exe_m_cb exe_cb;

	exe_cb.callee = m_id;
	exe_cb.caller = E_MODULE_SYS;
	exe_cb.cmd_id = cmd_id;
	exe_cb.data   = (void *)data;

	return base_exe_module_cb(&exe_cb);
}

static long sys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct sys_device *dev = filp->private_data;

	switch (cmd) {
	case SYS_IOC_SET_INIT: {
		atomic_set(&dev->sys_inited, 1);
		break;
	}
	case SYS_IOC_GET_INIT: {
		unsigned int value = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		value = atomic_read(&dev->sys_inited);
		if (copy_to_user((uint32_t *)arg, &value, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_SET_VIVPSSMODE: {
		CHECK_IOCTL_CMD(cmd, vi_vpss_mode_s);
		if (copy_from_user(&dev->vivpss_mode, (void __user *)arg, sizeof(vi_vpss_mode_s)) != 0) {
			TRACE_SYS(DBG_ERR, "SYS_IOCTL_SET_VIVPSSMODE, copy_from_user failed.\n");
			return -EFAULT;
		}

		if (_sys_call_cb(E_MODULE_VI, VI_CB_SET_VIVPSSMODE, &dev->vivpss_mode) != 0) {
			TRACE_SYS(DBG_ERR, "VI_CB_SET_VIVPSSMODE failed\n");
		}

		if (_sys_call_cb(E_MODULE_VPSS, VPSS_CB_SET_VIVPSSMODE, &dev->vivpss_mode) != 0) {
			TRACE_SYS(DBG_ERR, "VPSS_CB_SET_VIVPSSMODE failed\n");
		}
		break;
	}
	case SYS_IOC_GET_VIVPSSMODE: {
		CHECK_IOCTL_CMD(cmd, vi_vpss_mode_s);
		if (copy_to_user((void __user *)arg, &dev->vivpss_mode, sizeof(vi_vpss_mode_s)) != 0) {
			TRACE_SYS(DBG_ERR, "SYS_IOCTL_GET_VIVPSSMODE, copy_to_user failed.\n");
			return -EFAULT;
		}
		break;
	}
	case SYS_IOC_READ_CHIP_ID: {
		unsigned int chip_id = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		chip_id = sys_comm_read_chip_id();
		if (copy_to_user((void __user *)arg, &chip_id, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_READ_CHIP_VERSION: {
		unsigned int chip_version = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		chip_version = sys_comm_read_chip_version();
		if (copy_to_user((void __user *)arg, &chip_version, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_READ_CHIP_PWR_ON_REASON: {
		unsigned int reason = 0;

		CHECK_IOCTL_CMD(cmd, unsigned int);
		reason = sys_comm_read_chip_pwr_on_reason();
		if (copy_to_user((void __user *)arg, &reason, sizeof(unsigned int)))
			return -EFAULT;
		break;
	}
	case SYS_IOC_CDMA_COPY: {
		struct sys_cdma_copy cdma_cfg;
		struct cdma_1d_param param1d;

		CHECK_IOCTL_CMD(cmd, struct sys_cdma_copy);
		if (copy_from_user(&cdma_cfg, (void __user *)arg, sizeof(cdma_cfg)) != 0) {
			TRACE_SYS(DBG_ERR, "SYS_IOC_CDMA_COPY, copy_from_user failed.\n");
			return -EFAULT;
		}
		param1d.data_type = CDMA_DATA_TYPE_8BIT;
		param1d.src_addr = cdma_cfg.phy_addr_src;
		param1d.dst_addr = cdma_cfg.phy_addr_dst;
		param1d.len = cdma_cfg.len;

		ret = cdma_copy1d(&param1d);
		break;
	}
	case SYS_IOC_CDMA_COPY2D: {
		cdma_2d_s cdma_2d;
		struct cdma_2d_param param2d;

		CHECK_IOCTL_CMD(cmd, cdma_2d_s);
		if (copy_from_user(&cdma_2d, (void __user *)arg, sizeof(cdma_2d)) != 0) {
			TRACE_SYS(DBG_ERR, "SYS_IOC_CDMA_COPY, copy_from_user failed.\n");
			return -EFAULT;
		}
		param2d.data_type = CDMA_DATA_TYPE_8BIT;
		param2d.src_addr = cdma_2d.phy_addr_src;
		param2d.dst_addr = cdma_2d.phy_addr_dst;
		param2d.width = cdma_2d.width;
		param2d.height = cdma_2d.height;
		param2d.src_stride = cdma_2d.stride_src;
		param2d.dst_stride = cdma_2d.stride_dst;
		param2d.fixed_enable = cdma_2d.enable_fixed;
		param2d.fixed_value = cdma_2d.fixed_value;

		ret = cdma_copy2d(&param2d);
		break;
	}
	default:
		return -ENOTTY;
	}
	return ret;
}

static int sys_open(struct inode *inode, struct file *filp)
{
	struct sys_device *ndev = container_of(filp->private_data, struct sys_device, miscdev);

	spin_lock(&ndev->close_lock);
	ndev->use_count++;
	spin_unlock(&ndev->close_lock);
	filp->private_data = ndev;
	return 0;
}

static int sys_close(struct inode *inode, struct file *filp)
{
	struct sys_device *ndev = filp->private_data;
	int cnt = 0;

	spin_lock(&ndev->close_lock);
	cnt = --ndev->use_count;
	spin_unlock(&ndev->close_lock);

	if (cnt == 0) {
		atomic_set(&ndev->sys_inited, 0);
	}

	filp->private_data = NULL;
	return 0;
}

#ifdef CONFIG_COMPAT
static long sys_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static const struct file_operations sys_fops = {
	.owner = THIS_MODULE,
	.open = sys_open,
	.release = sys_close,
	.unlocked_ioctl = sys_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = sys_compat_ptr_ioctl,
#endif
};

int sys_register_misc(struct sys_device *ndev)
{
	int rc;

	ndev->miscdev.minor = MISC_DYNAMIC_MINOR;
	ndev->miscdev.name = SYS_DEV_NAME;
	ndev->miscdev.fops = &sys_fops;

	rc = misc_register(&ndev->miscdev);
	if (rc) {
		dev_err(ndev->dev, "sys: failed to register misc device.\n");
		return rc;
	}

	return 0;
}

static int sys_init_resources(struct platform_device *pdev, struct sys_device *ndev)
{
	int32_t ret;
	void __iomem *reg_base;
	struct resource *res;
	char *irq_name = "vi_cdma";
	int irq_num;

	/* Get vi_sys base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	TRACE_SYS(DBG_INFO, "vi_sys res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, reg_base);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}
	vi_sys_set_base_addr(reg_base);

	/* Get cdma base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	TRACE_SYS(DBG_INFO, "vi-cdma res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, reg_base);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}
	cdma_set_base_addr(reg_base);

	/* Get vo_sys base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 2);
	reg_base = devm_ioremap_resource(&pdev->dev, res);
	TRACE_SYS(DBG_INFO, "vo_sys res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, reg_base);
	if (IS_ERR(reg_base)) {
		ret = PTR_ERR(reg_base);
		return ret;
	}
	vo_sys_set_base_addr(reg_base);

	/* Get cdma irq num */
	irq_num = platform_get_irq_byname(pdev, irq_name);
	if (irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for %s\n", irq_name);
		return -ENODEV;
	}
	TRACE_SYS(DBG_INFO, "irq(%d) for %s get from platform driver.\n", irq_num, irq_name);
	ndev->cdma_irq_num = irq_num;

	return 0;
}

static int sys_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct sys_device *ndev;
	int32_t ret;

	ndev = devm_kzalloc(&pdev->dev, sizeof(*ndev), GFP_KERNEL);
	if (!ndev)
		return -ENOMEM;
	ndev->dev = dev;

	mutex_init(&ndev->dev_lock);
	spin_lock_init(&ndev->close_lock);
	ndev->use_count = 0;

	// get hw-resources
	ret = sys_init_resources(pdev, ndev);
	if (ret) {
		TRACE_SYS(DBG_ERR, "Failed to sys init resource\n");
		return ret;
	}

	ret = sys_register_misc(ndev);
	if (ret < 0) {
		TRACE_SYS(DBG_ERR, "register misc error\n");
		return ret;
	}

	if (devm_request_irq(&pdev->dev, ndev->cdma_irq_num, cdma_isr, IRQF_SHARED,
		"vi_cdma", ndev)) {
		dev_err(&pdev->dev, "Unable to request cdma IRQ(%d)\n", ndev->cdma_irq_num);
		misc_deregister(&ndev->miscdev);
		return -ENODEV;
	}

	platform_set_drvdata(pdev, ndev);
	sys_comm_init();

	TRACE_SYS(DBG_WARN, "sys probe done\n");
	return 0;
}

static int sys_remove(struct platform_device *pdev)
{
	struct sys_device *ndev = platform_get_drvdata(pdev);

	sys_comm_deinit();

	devm_free_irq(&pdev->dev, ndev->cdma_irq_num, ndev);
	misc_deregister(&ndev->miscdev);
	platform_set_drvdata(pdev, NULL);

	return 0;
}

static const struct of_device_id sys_match[] = {
	{ .compatible = "cvitek,sys" },
	{},
};
MODULE_DEVICE_TABLE(of, sys_match);

static struct platform_driver sys_driver = {
	.probe = sys_probe,
	.remove = sys_remove,
	.driver = {
			.owner = THIS_MODULE,
			.name = SYS_DEV_NAME,
			.of_match_table = sys_match,
		},
};
module_platform_driver(sys_driver);

MODULE_DESCRIPTION("Soph SoC SYS driver");
MODULE_LICENSE("GPL");
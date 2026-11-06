#include <vi_core.h>
#include <base_cb.h>
#include <linux/compat.h>

#define VI_IRQ_NAME            "isp"
#define VI_CLASS_NAME          "soph-vi"
#define VI_DEV_NAME            "soph-vi"

/* Runtime to enable vi write reg info
 * Ctrl:
 *	0: Disable to dump addr/val info of writing reg.
 *	1: Enable to dump addr/val info of writing reg.
 */
#ifdef PORTING_TEST
int vi_dump_reg;
module_param(vi_dump_reg, int, 0644);
#endif

static long vi_core_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	return vi_ioctl(filp, cmd, arg);
}

#ifdef CONFIG_COMPAT
static long vi_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int vi_core_open(struct inode *inode, struct file *filp)
{
	return vi_open(inode, filp);
}

static int vi_core_release(struct inode *inode, struct file *filp)
{
	return vi_release(inode, filp);
}

static int vi_core_mmap(struct file *filp, struct vm_area_struct *vm)
{
	return vi_mmap(filp, vm);
}

static unsigned int vi_core_poll(struct file *filp, struct poll_table_struct *wait)
{
	return vi_poll(filp, wait);
}

const struct file_operations vi_fops = {
	.owner = THIS_MODULE,
	.open = vi_core_open,
	.unlocked_ioctl = vi_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = vi_compat_ptr_ioctl,
#endif
	.release = vi_core_release,
	.mmap = vi_core_mmap,
	.poll = vi_core_poll,
};

int vi_core_cb(void *dev, enum enum_modules_id caller, u32 cmd, void *arg)
{
	return vi_cb(dev, caller, cmd, arg);
}

static int vi_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_VI);
}

static int vi_core_register_cb(struct sop_vi_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VI;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= vi_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static irqreturn_t vi_core_isr(int irq, void *priv)
{
	struct sop_vi_dev *vdev = priv;

	vi_irq_handler(vdev);

	return IRQ_HANDLED;
}

static int vi_core_register_cdev(struct sop_vi_dev *dev)
{
	struct device *dev_t;
	int err = 0;

	dev->vi_class = class_create(THIS_MODULE, VI_CLASS_NAME);
	if (IS_ERR(dev->vi_class)) {
		dev_err(dev->dev, "create class failed\n");
		return PTR_ERR(dev->vi_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&dev->cdev_id, 0, 1, VI_DEV_NAME)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "allocate chrdev failed\n");
		return err;
	}

	/* initialize the device structure and register the device with the kernel */
	dev->cdev.owner = THIS_MODULE;
	cdev_init(&dev->cdev, &vi_fops);

	if ((cdev_add(&dev->cdev, dev->cdev_id, 1)) < 0) {
		err = -EBUSY;
		dev_err(dev->dev, "add chrdev failed\n");
		return err;
	}

	dev_t = device_create(dev->vi_class, dev->dev, dev->cdev_id, NULL, "%s", VI_DEV_NAME);
	if (IS_ERR(dev_t)) {
		dev_err(dev->dev, "device create failed error code(%ld)\n", PTR_ERR(dev_t));
		err = PTR_ERR(dev_t);
		return err;
	}

	return err;
}

#ifndef FPGA_PORTING
static int vi_core_clk_init(struct platform_device *pdev)
{
	struct sop_vi_dev *dev;
	u8 i = 0;

	dev = dev_get_drvdata(&pdev->dev);
	if (!dev) {
		dev_err(&pdev->dev, "Can not get sop_vi drvdata\n");
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(clk_sys_name); ++i) {
		dev->clk_sys[i] = devm_clk_get(&pdev->dev, clk_sys_name[i]);
		if (IS_ERR(dev->clk_sys[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_sys_name[i]);
			return PTR_ERR(dev->clk_sys[i]);
		}
		clk_prepare_enable(dev->clk_sys[i]);
		clk_disable_unprepare(dev->clk_sys[i]);
	}

	for (i = 0; i < ARRAY_SIZE(clk_isp_name); ++i) {
		dev->clk_isp[i] = devm_clk_get(&pdev->dev, clk_isp_name[i]);
		if (IS_ERR(dev->clk_isp[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_isp_name[i]);
			return PTR_ERR(dev->clk_isp[i]);
		}
		clk_prepare_enable(dev->clk_isp[i]);
		clk_disable_unprepare(dev->clk_isp[i]);
	}

	for (i = 0; i < ARRAY_SIZE(clk_mac_name); ++i) {
		dev->clk_mac[i] = devm_clk_get(&pdev->dev, clk_mac_name[i]);
		if (IS_ERR(dev->clk_mac[i])) {
			dev_err(&pdev->dev, "Cannot get clk for %s\n", clk_mac_name[i]);
			return PTR_ERR(dev->clk_mac[i]);
		}
		clk_prepare_enable(dev->clk_mac[i]);
		clk_disable_unprepare(dev->clk_mac[i]);
	}

	return 0;
}
#endif

static int vi_core_probe(struct platform_device *pdev)
{
	struct sop_vi_dev *dev;
	struct resource *res;
	int ret = 0;

	dev = devm_kzalloc(&pdev->dev, sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->dev = &pdev->dev;
	dev_set_drvdata(&pdev->dev, dev);

	/* IP register base address */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	dev->reg_base = devm_ioremap_resource(&pdev->dev, res);
	vi_pr(VI_INFO, "res-reg: start: 0x%llx, end: 0x%llx, virt-addr(%px).\n",
			res->start, res->end, dev->reg_base);
	if (IS_ERR(dev->reg_base)) {
		ret = PTR_ERR(dev->reg_base);
		return ret;
	}

	/* Interrupt */
	dev->irq_num = platform_get_irq_byname(pdev, VI_IRQ_NAME);
	if (dev->irq_num < 0) {
		dev_err(&pdev->dev, "No IRQ resource for %s\n", VI_IRQ_NAME);
		return -ENODEV;
	}
	vi_pr(VI_INFO, "irq(%d) for %s get from platform driver.\n",
			dev->irq_num, VI_IRQ_NAME);
#ifndef FPGA_PORTING
	ret = vi_core_clk_init(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to init clk, err %d\n", ret);
		goto err_clk_init;
	}
#endif
	ret = vi_core_register_cdev(dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register dev, err %d\n", ret);
		goto err_dev_register;
	}

	ret = vi_create_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create instance, err %d\n", ret);
		goto err_create_instance;
	}

	ret = devm_request_irq(&pdev->dev, dev->irq_num, vi_core_isr, 0,
				pdev->name, dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to request irq_num(%d) ret(%d)\n",
				dev->irq_num, ret);
		ret = -EINVAL;
		goto err_req_irq;
	}

	ret = vi_core_register_cb(dev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to register vi cb, err %d\n", ret);
		goto err_create_instance;
	}

	vi_pr(VI_INFO, "isp registered as %s\n", VI_DEV_NAME);

err_create_instance:

err_dev_register:

#ifndef FPGA_PORTING
err_clk_init:
#endif

err_req_irq:
	return ret;
}

static int vi_core_remove(struct platform_device *pdev)
{
	int ret = 0;

	struct sop_vi_dev *dev = dev_get_drvdata(&pdev->dev);

	ret = vi_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	ret = vi_core_rm_cb();
	if (ret) {
		dev_err(&pdev->dev, "Failed to rm vi cb, err %d\n", ret);
	}

	device_destroy(dev->vi_class, dev->cdev_id);
	cdev_del(&dev->cdev);
	unregister_chrdev_region(dev->cdev_id, 1);
	class_destroy(dev->vi_class);

	dev_set_drvdata(&pdev->dev, NULL);

err_destroy_instance:
	vi_pr(VI_INFO, "%s -\n", __func__);

	return ret;
}

static int vi_core_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct sop_vi_dev *vdev = dev_get_drvdata(&pdev->dev);
	dev_info(&pdev->dev, "vi suspend start\n");
	vi_suspend(vdev);
	dev_info(&pdev->dev, "vi suspend end\n");

	return 0;
}

static int vi_core_resume(struct platform_device *pdev)
{
	struct sop_vi_dev *vdev = dev_get_drvdata(&pdev->dev);
	dev_info(&pdev->dev, "vi resume start\n");
	vi_resume(vdev);
	dev_info(&pdev->dev, "vi resume end\n");

	return 0;
}

static const struct of_device_id vi_core_match[] = {
	{
		.compatible = "cvitek,vi",
		.data       = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, vi_core_match);

static struct platform_driver vi_core_driver = {
	.probe = vi_core_probe,
	.remove = vi_core_remove,
	.suspend = vi_core_suspend,
	.resume = vi_core_resume,
	.driver = {
		.name = VI_DEV_NAME,
		.of_match_table = vi_core_match,
	},
};

module_platform_driver(vi_core_driver);
MODULE_AUTHOR("CVITEK Inc.");
MODULE_DESCRIPTION("Cvitek video input driver");
MODULE_LICENSE("GPL");

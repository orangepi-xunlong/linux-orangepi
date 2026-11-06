#include <rgn_core.h>
#include <base_cb.h>
#include <linux/compat.h>

#define CVI_RGN_DEV_NAME            "soph-rgn"

static long rgn_core_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	return rgn_ioctl(filp, cmd, arg);
}

#ifdef CONFIG_COMPAT
static long rgn_compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static int rgn_core_open(struct inode *inode, struct file *filp)
{
	return rgn_open(inode, filp);
}

static int rgn_core_release(struct inode *inode, struct file *filp)
{
	return rgn_release(inode, filp);
}

const struct file_operations rgn_fops = {
	.owner = THIS_MODULE,
	.open = rgn_core_open,
	.unlocked_ioctl = rgn_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = rgn_compat_ptr_ioctl,
#endif
	.release = rgn_core_release,
};

int rgn_core_cb(void *dev, enum enum_modules_id caller, unsigned int cmd, void *arg)
{
	return rgn_cb(dev, caller, cmd, arg);
}

static int rgn_core_rm_cb(void)
{
	return base_rm_module_cb(E_MODULE_RGN);
}

static int rgn_core_register_cb(struct rgn_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_RGN;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= rgn_core_cb;

	return base_reg_module_cb(&reg_cb);
}

static int register_rgn_dev(struct device *dev, struct rgn_dev *rdev)
{
	int ret;

	rdev->miscdev.minor = MISC_DYNAMIC_MINOR;
	rdev->miscdev.name = "soph-rgn";
	rdev->miscdev.fops = &rgn_fops;

	ret = misc_register(&rdev->miscdev);
	if (ret) {
		pr_err("rgn_dev: failed to register misc device.\n");
		return ret;
	}

	return ret;
}

static int rgn_probe(struct platform_device *pdev)
{
	struct rgn_dev *rdev;
	int ret = 0;

	rdev = devm_kzalloc(&pdev->dev, sizeof(*rdev), GFP_KERNEL);
	if (!rdev)
		return -ENOMEM;

	/* initialize locks */
	spin_lock_init(&rdev->lock);
	mutex_init(&rdev->mutex);

	dev_set_drvdata(&pdev->dev, rdev);

	ret = rgn_create_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to create instance, err %d\n", ret);
		goto err_create_instance;
	}

	if (register_rgn_dev(&pdev->dev, rdev)) {
		CVI_TRACE_RGN(RGN_ERR, "Failed to register rgn-dev\n");
		goto err_create_instance;
	}

	if (rgn_core_register_cb(rdev)) {
		dev_err(&pdev->dev, "Failed to register rgn cb, err %d\n", ret);
		goto err_create_instance;
	}

	CVI_TRACE_RGN(RGN_INFO, "done with ret(%d).\n", ret);
	return ret;

err_create_instance:
	misc_deregister(&rdev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

	CVI_TRACE_RGN(RGN_INFO, "failed with rc(%d).\n", ret);
	return ret;
}

static int rgn_remove(struct platform_device *pdev)
{
	struct rgn_dev *rdev = platform_get_drvdata(pdev);
	int ret = 0;

	ret = rgn_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}

	if (rgn_core_rm_cb()) {
		dev_err(&pdev->dev, "Failed to rm rgn cb, err %d\n", ret);
	}

	misc_deregister(&rdev->miscdev);
	dev_set_drvdata(&pdev->dev, NULL);

err_destroy_instance:
	CVI_TRACE_RGN(RGN_INFO, "%s -\n", __func__);

	return ret;
}

static const struct of_device_id rgn_dt_match[] = {
	{.compatible = "cvitek,rgn"},
	{},
};

MODULE_DEVICE_TABLE(of, rgn_dt_match);

static struct platform_driver rgn_core_driver = {
	.probe = rgn_probe,
	.remove = rgn_remove,
	.driver = {
		.name = CVI_RGN_DEV_NAME,
		.of_match_table = rgn_dt_match,
	},
};

module_platform_driver(rgn_core_driver);
MODULE_AUTHOR("CVITEK Inc.");
MODULE_DESCRIPTION("Cvitek rgn driver");
MODULE_LICENSE("GPL");

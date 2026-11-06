#include <audio_core.h>
#include <audio_interface.h>//internal api declaration
#include <audio_ioctl_cmd.h>//share struct & cmd between userspace and kernel space
#include <linux/compat.h>

//static function declaration -------------------start
static int audio_core_probe(struct platform_device *pdev);
static int audio_core_remove(struct platform_device *pdev);
//static function declaration --------------------end


int audio_dump_reg;
module_param(audio_dump_reg, int, 0644);

static const struct of_device_id audio_core_match_tbl[] = {
	{
		.compatible = "cvitek,audio",
		.data       = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, audio_core_match_tbl);

static struct platform_driver audio_core_driver = {
	.probe = audio_core_probe,
	.remove = audio_core_remove,
	.driver = {
		.name = AUDIO_DRV_PLATFORM_DEVICE_NAME,
		.owner = THIS_MODULE,
		.of_match_table = audio_core_match_tbl,
	},
};



static long audio_device_core_ioctl(struct file *filp, u_int cmd, u_long arg)
{
	return audio_device_ioctl(filp, cmd, arg);
}


#ifdef CONFIG_COMPAT
static long compat_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}

static long audio_compat_device_ptr_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (!file->f_op->unlocked_ioctl)
		return -ENOIOCTLCMD;

	return file->f_op->unlocked_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif


static int audio_device_core_open(struct inode *inode, struct file *filp)
{
	return audio_device_open(inode, filp);
}

static int audio_device_core_release(struct inode *inode, struct file *filp)
{
	return audio_device_release(inode, filp);
}


static int audio_device_core_mmap(struct file *filp, struct vm_area_struct *vm)
{
	return audio_device_mmap(filp, vm);
}

static unsigned int audio_device_core_poll(struct file *filp, struct poll_table_struct *wait)
{
	return audio_device_poll(filp, wait);
}


const struct file_operations audio_device_fops = {
	.owner = THIS_MODULE,
	.open = audio_device_core_open,
	.unlocked_ioctl = audio_device_core_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = audio_compat_device_ptr_ioctl,
#endif
	.release = audio_device_core_release,
	.mmap = audio_device_core_mmap,
	.poll = audio_device_core_poll,
};


int audio_core_cb(void *dev, enum enum_modules_id caller, u32 cmd, void *arg)
{
	return audio_device_cb(dev, caller, cmd, arg);
}

static int audio_core_register_cb(struct audio_dev *dev)
{
	struct base_m_cb_info reg_cb;

	reg_cb.module_id	= E_MODULE_VI;
	reg_cb.dev		= (void *)dev;
	reg_cb.cb		= audio_core_cb;
#if 0
	return base_reg_module_cb(&reg_cb);
#else
	return 0;
#endif
}


static int audio_core_register_cdev(struct audio_dev *adev)
{
	int err = 0;
	char devName[256];
	dev_t subDevice;

	adev->audio_class = class_create(THIS_MODULE, AUDIO_DRV_CLASS_NAME);
	if (IS_ERR(adev->audio_class)) {
		pr_err("create class failed\n");
		return PTR_ERR(adev->audio_class);
	}

	/* get the major number of the character device */
	if ((alloc_chrdev_region(&adev->cdev_id, 0, AUDIO_DRV_MAX_ALL_NUM, AUDIO_DRV_CLASS_NAME)) < 0) {
		err = -EBUSY;
		pr_err("could not allocate major number\n");
		return err;
	}
	adev->audio_major = MAJOR(adev->cdev_id);
	pr_err("SUCCESS alloc_chrdev_region major %d\n", adev->audio_major);

	subDevice = MKDEV(adev->audio_major, 1);
	cdev_init(&adev->cdev_ceo, &audio_device_fops);
	adev->cdev_ceo.owner = THIS_MODULE;
	if (cdev_add(&adev->cdev_ceo, subDevice, 1) < 0) {
		pr_err("cound not allocate chrdev for cdev_ceo\n");
		return -EBUSY;
	}
	sprintf(devName, "%s", AUDIO_CEO_DEV_NAME);
	device_create(adev->audio_class, NULL, subDevice, NULL, "%s", devName);

	return err;

}

static int audio_core_probe(struct platform_device *pdev)
{
	int ret = 0;
	const struct of_device_id *match;
	struct device *dev = &pdev->dev;
	struct audio_dev *adev;

	audio_pr(AUDIO_ERR, "Enter audio_core_probe!\n");
	audio_pr(AUDIO_DBG, "[%s][%d]!\n", __func__, __LINE__);
	audio_pr(AUDIO_INFO, "[%s][%d]!\n", __func__, __LINE__);
	adev = devm_kzalloc(&pdev->dev, sizeof(*adev), GFP_KERNEL);
	if (!adev) {
		pr_err("out of memory in audio_core_probe\n");
		return -ENOMEM;
	}
	memset(adev, 0, sizeof(*adev));
	adev->dev = dev;
	/*check match*/
	match = of_match_device(audio_core_match_tbl, &pdev->dev);
	if (!match) {
		pr_err("of match device in audio_core_probe\n");
		return -EINVAL;
	}
	adev->pdata = match->data;
	/* register device and create device nodes by channels*/
	ret = audio_core_register_cdev(adev);
	if (ret < 0) {
		pr_err("adev failed to register dev, err[%d]\n", ret);
		unregister_chrdev_region(adev->audio_major, AUDIO_DRV_MAX_ALL_NUM);
		return -EINVAL;
	}
	/* register callback function: None*/
	audio_core_register_cb(adev);
	platform_set_drvdata(pdev, adev);
	//ret = vi_create_instance(pdev);
	ret = audio_create_instance(pdev);
	if (ret)
		pr_err("adev failure to create instance[%d]\n", ret);
	else
		pr_err("audio_create_instance success!!\n");
	return ret;
}

static int audio_core_remove(struct platform_device *pdev)
{
	int ret = 0;
	int i = 0;
	struct audio_dev *adev = platform_get_drvdata(pdev);

	audio_pr(AUDIO_INFO, "[%s][%d]\n", __func__, __LINE__);
	//TODO: should destroy instance as probe create the instance
	//the destroy instance should written in the audio_interface.c
	#if 0
	ret = vi_destroy_instance(pdev);
	if (ret) {
		dev_err(&pdev->dev, "Failed to destroy instance, err %d\n", ret);
		goto err_destroy_instance;
	}
	#endif
	if (adev->audio_major <= 0) {
		pr_err("udio error audio_core_remove error in major\n");
		return -1;
	}
	//do the device destroy
	pr_err("[%s][%d]\n", __func__, __LINE__);
	for (i = 0; i < AUDIO_DRV_MAX_ALL_NUM; i++) {
		dev_t subDevice;

		subDevice = MKDEV(adev->audio_major, i);
		cdev_del(&adev->cdev_ceo);
		device_destroy(adev->audio_class, subDevice);
	}
	//class destroy and chrdev_region unregister
	class_destroy(adev->audio_class);
	unregister_chrdev_region(adev->audio_major, AUDIO_DRV_MAX_ALL_NUM);
	adev->audio_major = 0;
	pr_err("[%s][%d]\n", __func__, __LINE__);
	return ret;
}


module_platform_driver(audio_core_driver);
MODULE_AUTHOR("Sophon Audio Inc.");
MODULE_DESCRIPTION("Sophon Audio driver");
MODULE_LICENSE("GPL");

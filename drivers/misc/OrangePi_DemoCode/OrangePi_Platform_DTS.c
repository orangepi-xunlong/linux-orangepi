#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/sys_config.h>
#include <linux/platform_device.h>

static int OrangePi_DTS_probe(struct platform_device *pdev)
{
	printk("OrangePi_DTS_probe\n");
	return 0;
}

static int OrangePi_DTS_remove(struct platform_device *pdev)
{
	printk("OrangePi_DTS_remove\n");
	return 0;
}

static const struct of_device_id OrangePi_DTS_ids[] = {
	{ .compatible = "OrangePi,OrangePi_DTS" },
	{ /* Sentinel */ }
};

static struct platform_driver OrangePi_DTS_driver = {
	.probe	= OrangePi_DTS_probe,
	.remove = OrangePi_DTS_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name  = "OrangePi_DTS",
		.of_match_table = OrangePi_DTS_ids,
	},
};

static int __init OrangePi_DTS_init(void)
{
	printk("Register OrangePi_DTS\n");
	return platform_driver_register(&OrangePi_DTS_driver);
}

static void __exit OrangePi_DTS_exit(void)
{
	printk("Unegister OrangePi_DTS\n");
	platform_driver_unregister(&OrangePi_DTS_driver);
}

module_init(OrangePi_DTS_init);
module_exit(OrangePi_DTS_exit);

MODULE_AUTHOR("BuddyZhang1");
MODULE_DESCRIPTION("Platform driver with DTS");
MODULE_LICENSE("GPL");


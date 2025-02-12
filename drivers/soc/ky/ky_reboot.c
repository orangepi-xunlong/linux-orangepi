// SPDX-License-Identifier: GPL-2.0
/*
 * Ky x1 soc fastboot mode reboot
 */

#include <linux/device.h>
#include <linux/errno.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/printk.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/io.h>

#define RESET_REG_VALUE 0x55a
#define RESET_REG_VALUE1 0x55f
static char *rebootcmd = "fastboot";
static char *shellcmd = "uboot";

struct ky_reboot_ctrl {
	void __iomem *base;
	struct notifier_block reset_handler;
};

static int x1_reset_handler(struct notifier_block *this, unsigned long mode,
		void *cmd)
{
	struct ky_reboot_ctrl *info = container_of(this,struct ky_reboot_ctrl,
			reset_handler);

	if(cmd != NULL && !strcmp(cmd, rebootcmd))
		writel(RESET_REG_VALUE, info->base);

	if(cmd != NULL && !strcmp(cmd, shellcmd))
                writel(RESET_REG_VALUE1, info->base);

	return NOTIFY_DONE;
}

static const struct of_device_id ky_reboot_of_match[] = {
	{.compatible = "ky,x1-reboot"},
	{},
};
MODULE_DEVICE_TABLE(of, ky_reboot_of_match);

static int ky_reboot_probe(struct platform_device *pdev)
{
	struct ky_reboot_ctrl *info;
	int ret;

	info = devm_kzalloc(&pdev->dev, sizeof(struct ky_reboot_ctrl), GFP_KERNEL);
	if(info == NULL)
		return -ENOMEM;

	info->base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(info->base))
		return PTR_ERR(info->base);

	platform_set_drvdata(pdev, info);

	info->reset_handler.notifier_call = x1_reset_handler;
	info->reset_handler.priority = 128;
	ret = register_restart_handler(&info->reset_handler);
	if (ret) {
		dev_warn(&pdev->dev, "cannot register restart handler: %d\n",
			 ret);
	}

	return 0;
}

static int ky_reboot_remove(struct platform_device *pdev)
{
	struct ky_reboot_ctrl *info = platform_get_drvdata(pdev);

	unregister_restart_handler(&info->reset_handler);
	return 0;
}

static struct platform_driver ky_reboot_driver = {
	.driver = {
		.name = "ky-reboot",
		.of_match_table = of_match_ptr(ky_reboot_of_match),
	},
	.probe = ky_reboot_probe,
	.remove = ky_reboot_remove,
};

module_platform_driver(ky_reboot_driver);
MODULE_DESCRIPTION("X1 fastboot mode reboot");
MODULE_AUTHOR("Ky");
MODULE_LICENSE("GPL v2");

#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/module.h>

static struct platform_device virt[] = {
	{
			.name = "reg-15-cs-ldo0",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_ldo0",
			}
	},
	{
			.name = "reg-15-cs-aldo1",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_aldo1",
			}
	},
	{
			.name = "reg-15-cs-aldo2",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_aldo2",
			}
	},
	{
			.name = "reg-15-cs-dldo1",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_dldo1",
			}
	},
	{
			.name = "reg-15-cs-dldo2",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_dldo2",
			}
	},
	{
			.name = "reg-15-cs-dcdc1",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_dcdc1",
			}
	},
	{
			.name = "reg-15-cs-dcdc2",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_dcdc2",
			}
	},
	{
			.name = "reg-15-cs-dcdc3",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_dcdc3",
			}
	},
	{
			.name = "reg-15-cs-dcdc4",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_dcdc4",
			}
	},
	{
			.name = "reg-15-cs-ldoio0",
			.id = -1,
			.dev        = {
				.platform_data = "axp15_gpioldo",
			}
	},
};

static int __init virtual_init(void)
{
	int j, ret;

	for (j = 0; j < ARRAY_SIZE(virt); j++) {
		ret = platform_device_register(&virt[j]);
		if (ret)
				goto creat_devices_failed;
	}

	return ret;

creat_devices_failed:
	while (j--)
		platform_device_register(&virt[j]);
	return ret;

}

module_init(virtual_init);

static void __exit virtual_exit(void)
{
	int j;

	for (j = ARRAY_SIZE(virt) - 1; j >= 0; j--)
		platform_device_unregister(&virt[j]);
}
module_exit(virtual_exit);

MODULE_DESCRIPTION("Axp regulator test");
MODULE_AUTHOR("Kyle Cheung");
MODULE_LICENSE("GPL");

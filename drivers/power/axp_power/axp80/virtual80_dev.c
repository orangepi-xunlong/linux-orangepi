#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/module.h>

static struct platform_device virt[] = {
	{
			.name = "reg-80-cs-sw",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_sw",
			}
	},
	{
			.name = "reg-80-cs-aldo1",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_aldo1",
			}
	},
	{
			.name = "reg-80-cs-aldo2",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_aldo2",
			}
	},
	{
			.name = "reg-80-cs-aldo3",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_aldo3",
			}
	},
	{
			.name = "reg-80-cs-bldo1",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_bldo1",
			}
	},
	{
			.name = "reg-80-cs-bldo2",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_bldo2",
			}
	},
	{
			.name = "reg-80-cs-bldo3",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_bldo3",
			}
	},
	{
			.name = "reg-80-cs-bldo4",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_bldo4",
			}
	},
	{
			.name = "reg-80-cs-cldo1",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_cldo1",
			}
	},
	{
			.name = "reg-80-cs-cldo2",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_cldo2",
			}
	},
	{
			.name = "reg-80-cs-cldo3",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_cldo3",
			}
	},
	{
			.name = "reg-80-cs-dcdca",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_dcdca",
			}
	},
	{
			.name = "reg-80-cs-dcdcb",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_dcdcb",
			}
	},
	{
			.name = "reg-80-cs-dcdcc",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_dcdcc",
			}
	},
	{
			.name = "reg-80-cs-dcdcd",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_dcdcd",
			}
	},
	{
			.name = "reg-80-cs-dcdce",
			.id = -1,
			.dev        = {
				.platform_data = "axp80_dcdce",
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

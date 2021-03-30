#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include "axpdummy-regu.h"
#include "virtualdummy.h"

static struct platform_device virt[] = {
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo1",   "axpdummy_ldo1"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo2",   "axpdummy_ldo2"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo3",   "axpdummy_ldo3"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo4",   "axpdummy_ldo4"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo5",   "axpdummy_ldo5"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo6",   "axpdummy_ldo6"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo7",   "axpdummy_ldo7"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo8",   "axpdummy_ldo8"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo9",   "axpdummy_ldo9"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo10", "axpdummy_ldo10"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo11", "axpdummy_ldo11"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo12", "axpdummy_ldo12"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo13", "axpdummy_ldo13"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo14", "axpdummy_ldo14"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo15", "axpdummy_ldo15"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo16", "axpdummy_ldo16"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo17", "axpdummy_ldo17"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo18", "axpdummy_ldo18"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo19", "axpdummy_ldo19"),
	VIRTUAL_DUMMY_DEVICE_DATA("reg-dummy-cs-ldo20", "axpdummy_ldo20"),
};

static int __init virtual_init(void)
{
	int j, ret;

	for (j = 0; j < axpdummy_ldo_count; j++) {
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

	for (j = axpdummy_ldo_count - 1; j >= 0; j--)
		platform_device_unregister(&virt[j]);
}
module_exit(virtual_exit);

MODULE_DESCRIPTION("Axpdummy regulator test");
MODULE_AUTHOR("Kyle Cheung");
MODULE_LICENSE("GPL");

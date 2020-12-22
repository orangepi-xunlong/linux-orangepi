#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/mfd/axp-mfd.h>
#include <linux/module.h>

#include "axp-cfg.h"

static struct platform_device virt[]={
	{
			.name = "reg-15-cs-aldo1",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_aldo1",
			}
 	},{
			.name = "reg-15-cs-aldo2",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_aldo2",
			}
 	},{
			.name = "reg-15-cs-aldo3",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_aldo3",
			}
 	},{
			.name = "reg-15-cs-bldo1",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_bldo1",
			}
	},{
			.name = "reg-15-cs-bldo2",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_bldo2",
			}
 	},{
			.name = "reg-15-cs-bldo3",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_bldo3",
			}
 	},{
			.name = "reg-15-cs-bldo4",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_bldo4",
			}
	},{
			.name = "reg-15-cs-cldo1",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_cldo1",
			}
	},{
			.name = "reg-15-cs-cldo2",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_cldo2",
			}
	},{
			.name = "reg-15-cs-cldo3",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_cldo3",
			}
	},{
			.name = "reg-15-cs-dcdc1",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_dcdc1",
			}
	},{
			.name = "reg-15-cs-dcdc2",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_dcdc2",
			}
	},{
			.name = "reg-15-cs-dcdc3",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_dcdc3",
			}
	},{
			.name = "reg-15-cs-dcdc4",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_dcdc4",
			}
 	},{
			.name = "reg-15-cs-dcdc5",
			.id = -1,
			.dev		= {
				.platform_data = "axp15_dcdc5",
			}
	},
};


 static int __init virtual_init(void)
{
	int j,ret;
	
	for (j = 0; j < ARRAY_SIZE(virt); j++){
 		ret =  platform_device_register(&virt[j]);
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
	
	for (j = ARRAY_SIZE(virt) - 1; j >= 0; j--){
		platform_device_unregister(&virt[j]);
	}
}
module_exit(virtual_exit);

MODULE_DESCRIPTION("X-POWERS axp regulator test");
MODULE_AUTHOR("Weijin zhong X-POWERS");
MODULE_LICENSE("GPL");

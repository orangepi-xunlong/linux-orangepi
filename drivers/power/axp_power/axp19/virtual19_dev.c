#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <mach/irqs.h>
#include <linux/power_supply.h>
#include <linux/mfd/axp-mfd.h>

static struct platform_device virt[]={
	{
			.name = "reg-19-cs-dcdc1",
			.id = -1,
			.dev		= {
				.platform_data = "axp19_dcdc1",
			}
	},{
			.name = "reg-19-cs-dcdc2",
			.id = -1,
			.dev		= {
				.platform_data = "axp19_dcdc2",
			}
	},{
			.name = "reg-19-cs-dcdc3",
			.id = -1,
			.dev		= {
				.platform_data = "axp19_dcdc3",
			}
	},{
			.name = "reg-19-cs-ldo2",
			.id = -1,
			.dev		= {
				.platform_data = "axp19_ldo2",
			}
	},{
			.name = "reg-19-cs-ldo3",
			.id = -1,
			.dev		= {
				.platform_data = "axp19_ldo3",
			}
	},{
			.name = "reg-19-cs-ldo4",
			.id = -1,
			.dev		= {
				.platform_data = "axp19_ldo4",
			}
	},{
			.name = "reg-19-cs-ldoio0",
			.id = -1,
			.dev		= {
				.platform_data = "axp19_ldoio0",
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

MODULE_DESCRIPTION("Krosspower axp regulator test");
MODULE_AUTHOR("Donglu Zhang Krosspower");
MODULE_LICENSE("GPL");
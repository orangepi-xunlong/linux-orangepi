#include <linux/version.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>

#include <asm/io.h>

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 0, 0))
#include <linux/printk.h>
#include <linux/err.h>
#else
#include <config/printk.h>
#endif

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0))
#ifdef CONFIG_GPIO_WAKEUP
#include <linux/gpio.h>
#endif
#endif

#ifdef CONFIG_GPIO_WAKEUP
extern unsigned int oob_irq;
#endif

#include <linux/mmc/host.h>
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5, 4, 0))
#include <linux/sunxi-gpio.h>
#include <linux/power/aw_pm.h>
#endif

extern void sunxi_mmc_rescan_card(unsigned ids);
extern void sunxi_wlan_set_power(bool on);
extern int sunxi_wlan_get_bus_index(void);
extern int sunxi_wlan_get_oob_irq(void);
extern int sunxi_wlan_get_oob_irq_flags(void);

extern int ssvdevice_init(void);
extern void ssvdevice_exit(void);

int initWlan(void)
{
    int ret=0;
    printk(KERN_INFO "wlan.c initWlan\n");
    ret = ssvdevice_init();
    return ret;
}

void exitWlan(void)
{
    ssvdevice_exit();
    return;
}
int generic_wifi_init_module(void)
{
    int wlan_bus_index = 0;
#ifdef CONFIG_WIFI_LOAD_DRIVER_WHEN_KERNEL_BOOTUP
    int type = get_wifi_chip_type();
    if (type < WIFI_AP6XXX_SERIES || type == WIFI_ESP8089)
		return 0;
#endif
    printk("\n");
    printk("=======================================================\n");
    printk("==== Launching Wi-Fi driver! (Powered by Allwiner) ====\n");
    printk("=======================================================\n");
    printk("SSV6158 SDIO WiFi driver (Powered by Allwinner init.\n");
    sunxi_wlan_set_power(1);
    //msleep(100);

    wlan_bus_index = sunxi_wlan_get_bus_index();
	if (wlan_bus_index < 0) {
		printk("get wifi_sdc_id failed\n");
		return -1;
    } else {
		printk("----- %s sdc_id: %d\n", __FUNCTION__, wlan_bus_index);
		sunxi_mmc_rescan_card(wlan_bus_index);
    }
#ifdef CONFIG_GPIO_WAKEUP
    oob_irq = sunxi_wlan_get_oob_irq();
#endif

	return initWlan();
}

void generic_wifi_exit_module(void)
{
    int wlan_bus_index = 0;
#ifdef CONFIG_WIFI_LOAD_DRIVER_WHEN_KERNEL_BOOTUP
    int type = get_wifi_chip_type();
    if (type < WIFI_AP6XXX_SERIES || type == WIFI_ESP8089)
		return;
#endif
    printk("\n");
    printk("=======================================================\n");
    printk("==== Dislaunching Wi-Fi driver! (Powered by Allwinner) ====\n");
    printk("=======================================================\n");
    printk("SSV6158  SDIO WiFi driver (Powered by Allwinner init.\n");
	exitWlan();

    wlan_bus_index = sunxi_wlan_get_bus_index();
    sunxi_mmc_rescan_card(wlan_bus_index);

    //msleep(100);

    sunxi_wlan_set_power(0);
    printk("%s: remove card, power off.\n", __FUNCTION__);
}

EXPORT_SYMBOL(generic_wifi_init_module);
EXPORT_SYMBOL(generic_wifi_exit_module);

#ifdef CONFIG_SSV6X5X //CONFIG_SSV6XXX=y
late_initcall(generic_wifi_init_module);
#else //CONFIG_SSV6XXX=m or =n
module_init(generic_wifi_init_module);
#endif
module_exit(generic_wifi_exit_module);

MODULE_LICENSE("Dual BSD/GPL");

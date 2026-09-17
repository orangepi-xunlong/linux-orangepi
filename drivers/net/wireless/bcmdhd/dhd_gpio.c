
#include <osl.h>
#include <dhd_linux.h>
#include <linux/gpio.h>
#ifdef BCMDHD_DTS
#include <linux/of_gpio.h>
#endif /* BCMDHD_DTS */
#ifdef BCMDHD_PLATDEV
#include <linux/platform_device.h>
#endif /* BCMDHD_PLATDEV */

#ifdef BCMDHD_DTS
/* This is sample code in dts file.
bcmdhd_wlan {
	compatible = "android,bcmdhd_wlan";
	gpio_wl_reg_on = <&gpio GPIOH_4 GPIO_ACTIVE_HIGH>;
	gpio_wl_host_wake = <&gpio GPIOZ_15 GPIO_ACTIVE_HIGH>;
	gpio_wl_gpio_tsf = <&gpio GPIOZ_16 GPIO_ACTIVE_HIGH>;
};
*/
#define DHD_DT_COMPAT_ENTRY		"android,bcmdhd_wlan"
#define GPIO_WL_REG_ON_PROPNAME		"gpio_wl_reg_on" ADAPTER_IDX_STR
#define GPIO_WL_HOST_WAKE_PROPNAME	"gpio_wl_host_wake" ADAPTER_IDX_STR
#ifdef OOB_GPIO_TSF_INTR
#define GPIO_WL_GPIO_TSF_PROPNAME	"gpio_wl_gpio_tsf" ADAPTER_IDX_STR
#endif /* OOB_GPIO_TSF_INTR */
#endif /* BCMDHD_DTS */
#define GPIO_WL_REG_ON_NAME 	"WL_REG_ON" ADAPTER_IDX_STR
#define GPIO_WL_HOST_WAKE_NAME	"WL_HOST_WAKE" ADAPTER_IDX_STR
#ifdef OOB_GPIO_TSF_INTR
#define GPIO_WL_GPIO_TSF_NAME 	"WL_GPIO_TSF" ADAPTER_IDX_STR
#endif /* OOB_GPIO_TSF_INTR */

#ifdef CONFIG_DHD_USE_STATIC_BUF
#if defined(BCMDHD_MDRIVER) && !defined(DHD_STATIC_IN_DRIVER)
extern void *dhd_wlan_mem_prealloc(uint bus_type, int index,
	int section, unsigned long size);
#else
extern void *dhd_wlan_mem_prealloc(int section, unsigned long size);
#endif
#endif /* CONFIG_DHD_USE_STATIC_BUF */

#if defined(BCMPCIE) && defined(PCIE_ATU_FIXUP)
extern void pcie_power_on_atu_fixup(void);
#endif

static int
dhd_wlan_set_power(wifi_adapter_info_t *adapter, int on)
{
	int gpio_wl_reg_on = adapter->gpio_wl_reg_on;
	int err = 0;

	if (on) {
		printf("======== PULL WL_REG_ON(%d) HIGH! ========\n", gpio_wl_reg_on);
		if (gpio_wl_reg_on >= 0) {
			err = gpio_direction_output(gpio_wl_reg_on, 1);
			if (err) {
				printf("%s: WL_REG_ON didn't output high\n", __FUNCTION__);
				return -EIO;
			}
		}
#ifdef BCMPCIE
#ifdef PCIE_ATU_FIXUP
		OSL_SLEEP(WIFI_TURNON_DELAY);
		pcie_power_on_atu_fixup();
#endif /* PCIE_ATU_FIXUP */
#ifdef BUS_POWER_RESTORE
		if (adapter->pci_dev) {
			OSL_SLEEP(WIFI_TURNON_DELAY);
			printf("======== pci_set_power_state PCI_D0! ========\n");
			pci_set_power_state(adapter->pci_dev, PCI_D0);
			if (adapter->pci_saved_state)
				pci_load_and_free_saved_state(adapter->pci_dev, &adapter->pci_saved_state);
			pci_restore_state(adapter->pci_dev);
			err = pci_enable_device(adapter->pci_dev);
			if (err < 0)
				printf("%s: PCI enable device failed", __FUNCTION__);
			pci_set_master(adapter->pci_dev);
		}
#endif /* BUS_POWER_RESTORE */
#endif /* BCMPCIE */
	} else {
#ifdef BCMPCIE
#ifdef BUS_POWER_RESTORE
		if (adapter->pci_dev) {
			printf("======== pci_set_power_state PCI_D3hot! ========\n");
			pci_save_state(adapter->pci_dev);
			adapter->pci_saved_state = pci_store_saved_state(adapter->pci_dev);
			if (pci_is_enabled(adapter->pci_dev))
				pci_disable_device(adapter->pci_dev);
			pci_set_power_state(adapter->pci_dev, PCI_D3hot);
		}
#endif /* BUS_POWER_RESTORE */
#endif /* BCMPCIE */
		printf("======== PULL WL_REG_ON(%d) LOW! ========\n", gpio_wl_reg_on);
		if (gpio_wl_reg_on >= 0) {
			err = gpio_direction_output(gpio_wl_reg_on, 0);
			if (err) {
				printf("%s: WL_REG_ON didn't output low\n", __FUNCTION__);
				return -EIO;
			}
		}
	}

	return err;
}

static int
dhd_wlan_set_reset(int onoff)
{
	return 0;
}

static int
dhd_wlan_set_carddetect(wifi_adapter_info_t *adapter, int present)
{
#if defined(BCMPCIE) && defined(PCIE_DETECT_CHANGE)
	struct pci_bus *b = NULL;
	struct pci_dev *rc_dev;
#endif /* BCMPCIE && PCIE_DETECT_CHANGE */
	int err = 0;

	if (present) {
#if defined(BCMSDIO)
		printf("======== Card detection to detect SDIO card! ========\n");
#ifdef CUSTOMER_HW_PLATFORM
		err = sdhci_force_presence_change(&sdmmc_channel, 1);
#endif /* CUSTOMER_HW_PLATFORM */
#elif defined(BCMPCIE)
		printf("======== Card detection to detect PCIE card! ========\n");
#ifdef PCIE_DETECT_CHANGE
		pci_lock_rescan_remove();
		while ((b = pci_find_next_bus(b)) != NULL) {
			printf("rescan pcie device\n");
			pci_rescan_bus(b);
		}
		pci_unlock_rescan_remove();
#endif /* PCIE_DETECT_CHANGE */
#endif
	} else {
#if defined(BCMSDIO)
		printf("======== Card detection to remove SDIO card! ========\n");
#ifdef CUSTOMER_HW_PLATFORM
		err = sdhci_force_presence_change(&sdmmc_channel, 0);
#endif /* CUSTOMER_HW_PLATFORM */
#elif defined(BCMPCIE)
		printf("======== Card detection to remove PCIE card! ========\n");
#ifdef PCIE_DETECT_CHANGE
		if(adapter->pci_dev) {
			printf("remove device 0x%x (vendor 0x%x)\n",
				adapter->pci_dev->device, adapter->pci_dev->vendor);
			pci_stop_and_remove_bus_device(adapter->pci_dev);
			if (adapter->pci_dev->bus) {
				rc_dev = adapter->pci_dev->bus->self;
				printf("remove rc device 0x%x (vendor 0x%x)\n",
					rc_dev->device, rc_dev->vendor);
				pci_stop_and_remove_bus_device_locked(rc_dev);
			}
		}
#endif /* PCIE_DETECT_CHANGE */
#endif
	}

	return err;
}

static int
dhd_wlan_get_mac_addr(wifi_adapter_info_t *adapter,
	unsigned char *buf, int ifidx)
{
	int err = -1;

	if (ifidx == 0) {
		/* Here is for wlan0 MAC address and please enable CONFIG_BCMDHD_CUSTOM_MAC in Makefile */
#ifdef EXAMPLE_GET_MAC
		struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
#endif /* EXAMPLE_GET_MAC */
	}
	else if (ifidx == 1) {
		/* Here is for wlan1 MAC address and please enable CUSTOM_MULTI_MAC in Makefile */
#ifdef EXAMPLE_GET_MAC
		struct ether_addr ea_example = {{0x02, 0x11, 0x22, 0x33, 0x44, 0x55}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
#endif /* EXAMPLE_GET_MAC */
	}
	else {
		printf("%s: invalid ifidx=%d\n", __FUNCTION__, ifidx);
	}

	printf("======== %s err=%d ========\n", __FUNCTION__, err);

	return err;
}

struct wifi_platform_data dhd_wlan_control = {
	.set_power	= dhd_wlan_set_power,
	.set_reset	= dhd_wlan_set_reset,
	.set_carddetect	= dhd_wlan_set_carddetect,
	.get_mac_addr	= dhd_wlan_get_mac_addr,
#ifdef CONFIG_DHD_USE_STATIC_BUF
	.mem_prealloc	= dhd_wlan_mem_prealloc,
#endif /* CONFIG_DHD_USE_STATIC_BUF */
};

#if defined(BCMPCIE) && defined(PCIE_DETECT_CHANGE)
#include <bcmdevs.h>
#include <bcmdevs_legacy.h>
typedef struct module_devid_map_t {
	uint vendor;
	uint device;
} module_devid_map_t;

const module_devid_map_t module_devid_map[] = {
	{VENDOR_BROADCOM, BCM4345_D11AC_ID},
	{VENDOR_BROADCOM, BCM4356_D11AC_ID},
	{VENDOR_BROADCOM, BCM43751_D11AX_ID},
	{VENDOR_BROADCOM, BCM43752_D11AX_ID},
	{VENDOR_BROADCOM, BCM43756_D11AX_ID},
	{VENDOR_BROADCOM, BCM43756E_D11AX6E_ID},
	{VENDOR_BROADCOM, BCM4381_D11AX_ID},
	{VENDOR_BROADCOM, BCM4382_D11AX_ID},
	{VENDOR_BROADCOM, BCM4383_D11AX_ID},
	{VENDOR_SYNAPTICS, BCM43711_D11AX6E_ID},
};

static void
dhd_wlan_remove_pcie(void)
{
	struct pci_dev *pci_dev, *rc_dev;
	wifi_adapter_info_t *adapter;
	int i;

	for (i=0; i<sizeof(module_devid_map)/sizeof(module_devid_map[0]); i++) {
		const module_devid_map_t *row = &module_devid_map[i];
		pci_dev = pci_get_device(row->vendor, row->device, NULL);
		if (pci_dev) {
			adapter = dhd_wifi_platform_get_adapter(PCI_BUS, pci_dev->bus->number,
				PCI_SLOT(pci_dev->devfn));
			if (adapter) {
				dhd_wlan_set_power(adapter, FALSE);
				printf("remove device 0x%x (vendor 0x%x)\n", row->device, row->vendor);
				pci_stop_and_remove_bus_device(pci_dev);
				if (pci_dev->bus) {
					rc_dev = pci_dev->bus->self;
					printf("remove rc device 0x%x (vendor 0x%x)\n",
						rc_dev->device, rc_dev->vendor);
					pci_stop_and_remove_bus_device_locked(rc_dev);
				}
			} else {
				printf("No adapter found for bus 0x%x, slot 0x%x, device 0x%x (vendor 0x%x)\n",
					pci_dev->bus->number, PCI_SLOT(pci_dev->devfn),
					pci_dev->device, pci_dev->vendor);
			}
		}
	}
}
#endif /* BCMPCIE && PCIE_DETECT_CHANGE */

static int
dhd_wlan_request_gpio(int gpio, bool input, char *name)
{
	int gpio_irq = -1, err;

	if (gpio >= 0) {
		err = gpio_request(gpio, name);
		if (err < 0) {
			printf("%s: gpio_request(%d) for %s failed %d\n",
				__FUNCTION__, gpio, name, err);
			return -1;
		}
		if (input) {
			err = gpio_direction_input(gpio);
			if (err < 0) {
				printf("%s: gpio_direction_input(%d) for %s failed %d\n",
					__FUNCTION__, gpio, name, err);
				gpio_free(gpio);
				return -1;
			}
			gpio_irq = gpio_to_irq(gpio);
			if (gpio_irq < 0) {
				printf("%s: gpio_to_irq(%d) for %s failed %d\n",
					__FUNCTION__, gpio, name, gpio_irq);
				gpio_free(gpio);
				return -1;
			}
		} else {
			gpio_irq = gpio;
		}
	}

	return gpio_irq;
}

static void
dhd_wlan_free_gpio(int gpio, char *name)
{
	if (gpio >= 0) {
		printf("%s: gpio_free(%d) for %s\n", __FUNCTION__, gpio, name);
		gpio_free(gpio);
	}
}

static int
dhd_wlan_init_gpio(wifi_adapter_info_t *adapter)
{
#ifdef BCMDHD_DTS
	char wlan_node[32];
	struct device_node *root_node = NULL;
#endif /* BCMDHD_DTS */
	int gpio_wl_reg_on = -1;
#ifdef OOB_INTR
	int gpio_wl_host_wake = -1;
	uint irq_flags = 0;
#endif /* OOB_INTR */
#ifdef OOB_GPIO_TSF_INTR
	int gpio_wl_gpio_tsf = -1;
#endif /* OOB_GPIO_TSF_INTR */

	/* Please check your schematic and fill right SoC GPIO number which connected to
	* WL_REG_ON and WL_HOST_WAKE.
	*/
#ifdef BCMDHD_DTS
#ifdef BCMDHD_PLATDEV
	if (adapter->pdev) {
		root_node = adapter->pdev->dev.of_node;
		strcpy(wlan_node, root_node->name);
	} else {
		printf("%s: adapter->pdev is NULL\n", __FUNCTION__);
		return -1;
	}
#else
	strcpy(wlan_node, DHD_DT_COMPAT_ENTRY);
	root_node = of_find_compatible_node(NULL, NULL, wlan_node);
#endif /* BCMDHD_PLATDEV */
	printf("======== Get GPIO from DTS(%s) ========\n", wlan_node);
	if (root_node) {
		gpio_wl_reg_on = of_get_named_gpio(root_node, GPIO_WL_REG_ON_PROPNAME, 0);
#ifdef OOB_INTR
		gpio_wl_host_wake = of_get_named_gpio(root_node, GPIO_WL_HOST_WAKE_PROPNAME, 0);
#endif /* OOB_INTR */
#ifdef OOB_GPIO_TSF_INTR
		gpio_wl_gpio_tsf = of_get_named_gpio(root_node, GPIO_WL_GPIO_TSF_PROPNAME, 0);
#endif /* OOB_GPIO_TSF_INTR */
	} else
#endif /* BCMDHD_DTS */
	{
		gpio_wl_reg_on = -1;
#ifdef OOB_INTR
		gpio_wl_host_wake = -1;
#endif /* OOB_INTR */
#ifdef OOB_GPIO_TSF_INTR
		gpio_wl_gpio_tsf = -1;
#endif /* OOB_GPIO_TSF_INTR */
	}

	adapter->gpio_wl_reg_on = dhd_wlan_request_gpio(gpio_wl_reg_on, FALSE, GPIO_WL_REG_ON_NAME);
	printf("%s: gpio_wl_reg_on=%d\n", __FUNCTION__, adapter->gpio_wl_reg_on);

#if defined(BCMPCIE) && defined(PCIE_DETECT_CHANGE)
	if (adapter->gpio_wl_reg_on >= 0) {
		dhd_wlan_remove_pcie();
	}
#endif /* BCMPCIE && PCIE_DETECT_CHANGE */

#ifdef OOB_INTR
	adapter->irq_num = dhd_wlan_request_gpio(gpio_wl_host_wake, TRUE, GPIO_WL_HOST_WAKE_NAME);
	if (adapter->irq_num >= 0)
		adapter->gpio_wl_host_wake = gpio_wl_host_wake;
	else {
		adapter->gpio_wl_host_wake = -1;
		return -1;
	}
#ifdef HW_OOB
#if defined(HW_OOB_LOW_LEVEL) || defined(OOB_INTR_ACTIVE_LOW)
	irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL | IORESOURCE_IRQ_SHAREABLE;
#else
	irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
#endif /* HW_OOB_LOW_LEVEL || OOB_INTR_ACTIVE_LOW */
#else
#ifdef OOB_INTR_ACTIVE_LOW
	irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE | IORESOURCE_IRQ_SHAREABLE;
#else
	irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_SHAREABLE;
#endif /* OOB_INTR_ACTIVE_LOW */
#endif /* HW_OOB */
	irq_flags &= IRQF_TRIGGER_MASK;
	adapter->intr_flags = irq_flags;
	printf("%s: gpio_wl_host_wake=%d, oob_irq=%d, irq_flags=0x%x\n", __FUNCTION__,
		adapter->gpio_wl_host_wake, adapter->irq_num, adapter->intr_flags);
#endif /* OOB_INTR */

#ifdef OOB_GPIO_TSF_INTR
	adapter->tsf_irq_num = dhd_wlan_request_gpio(gpio_wl_gpio_tsf, TRUE, GPIO_WL_GPIO_TSF_NAME);
	if (adapter->tsf_irq_num >= 0)
		adapter->gpio_wl_gpio_tsf = gpio_wl_gpio_tsf;
	else {
		adapter->gpio_wl_gpio_tsf = -1;
		return -1;
	}
	adapter->tsf_intr_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_SHAREABLE;
	adapter->tsf_intr_flags &= IRQF_TRIGGER_MASK;
	printf("%s: gpio_wl_gpio_tsf=%d, tsf_irq=%d, irq_flags=0x%x\n", __FUNCTION__,
		adapter->gpio_wl_gpio_tsf, adapter->tsf_irq_num, adapter->tsf_intr_flags);
#endif /* OOB_GPIO_TSF_INTR */

	return 0;
}

static void
dhd_wlan_deinit_gpio(wifi_adapter_info_t *adapter)
{
	dhd_wlan_free_gpio(adapter->gpio_wl_reg_on, GPIO_WL_REG_ON_NAME);
	adapter->gpio_wl_reg_on = -1;
#ifdef OOB_INTR
	dhd_wlan_free_gpio(adapter->gpio_wl_host_wake, GPIO_WL_HOST_WAKE_NAME);
	adapter->gpio_wl_host_wake = -1;
#endif /* OOB_INTR */
#ifdef OOB_GPIO_TSF_INTR
	dhd_wlan_free_gpio(adapter->gpio_wl_gpio_tsf, GPIO_WL_GPIO_TSF_NAME);
	adapter->gpio_wl_gpio_tsf = -1;
#endif /* OOB_GPIO_TSF_INTR */
}

#if defined(BCMDHD_MDRIVER)
static void
dhd_wlan_init_adapter(wifi_adapter_info_t *adapter)
{
#ifdef ADAPTER_IDX
	adapter->index = ADAPTER_IDX;
	if (adapter->index == 0) {
		adapter->bus_num = 1;
		adapter->slot_num = 1;
	} else if (adapter->index == 1) {
		adapter->bus_num = 2;
		adapter->slot_num = 1;
	}
#ifdef BCMSDIO
	adapter->bus_type = SDIO_BUS;
#elif defined(BCMPCIE)
	adapter->bus_type = PCI_BUS;
#elif defined(BCMDBUS)
	adapter->bus_type = USB_BUS;
#endif
	printf("bus_type=%d, bus_num=0x%x, slot_num=0x%x\n",
		adapter->bus_type, adapter->bus_num, adapter->slot_num);
#endif /* ADAPTER_IDX */

#ifdef DHD_STATIC_IN_DRIVER
	adapter->index = 0;
#elif !defined(ADAPTER_IDX)
#ifdef BCMSDIO
	adapter->index = 0;
#elif defined(BCMPCIE)
	adapter->index = 1;
#elif defined(BCMDBUS)
	adapter->index = 2;
#endif
#endif /* DHD_STATIC_IN_DRIVER */
}
#endif /* BCMDHD_MDRIVER */

int
dhd_wlan_init_plat_data(wifi_adapter_info_t *adapter)
{
	int err = 0;

#ifdef BCMDHD_MDRIVER
	dhd_wlan_init_adapter(adapter);
#endif /* BCMDHD_MDRIVER */
	err = dhd_wlan_init_gpio(adapter);

	return err;
}

void
dhd_wlan_deinit_plat_data(wifi_adapter_info_t *adapter)
{
	dhd_wlan_deinit_gpio(adapter);
}

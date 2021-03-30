#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/rfkill.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/sys_config.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/pinctrl/pinconf-sunxi.h>

struct sunxi_wlan_platdata {
	int bus_index;
	int usb_index;
	struct regulator *wlan_power;
	struct regulator *io_regulator;
	struct clk 	*lpo;
	int gpio_wlan_regon;
	int gpio_wlan_hostwake;
	int gpio_wlan_en;

	char *wlan_power_name;
	char *io_regulator_name;

	int power_state;
	struct platform_device *pdev;
};
static struct sunxi_wlan_platdata *wlan_data = NULL;

static int sunxi_wlan_on(struct sunxi_wlan_platdata *data, bool on_off);
static DEFINE_MUTEX(sunxi_wlan_mutex);

static int bt_power_state;

void sunxi_wlan_set_power(bool on_off)
{
	struct platform_device *pdev;
	int ret = 0;
	if (!wlan_data)
		return;

	pdev = wlan_data->pdev;
	mutex_lock(&sunxi_wlan_mutex);
	if(on_off != wlan_data->power_state){
		ret = sunxi_wlan_on(wlan_data, on_off);
		if(ret)
			dev_err(&pdev->dev,"set power failed\n");
	}
	mutex_unlock(&sunxi_wlan_mutex);
}
EXPORT_SYMBOL_GPL(sunxi_wlan_set_power);

void sunxi_wlan_gpio_set_power(bool on_off)
{
	if (!wlan_data)
		return;

	if (on_off) {
		if ((wlan_data->power_state) == 0) {
			/* set wifi_en on */
			if (gpio_is_valid(wlan_data->gpio_wlan_en))
				gpio_direction_output(wlan_data->gpio_wlan_en, 1);
		}
	} else {
		if ((wlan_data->power_state) == 0) {
			/* set wifi_en off */
			if (gpio_is_valid(wlan_data->gpio_wlan_en))
				gpio_direction_output(wlan_data->gpio_wlan_en, 0);
		}
	}

	bt_power_state = on_off;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_gpio_set_power);

int sunxi_wlan_get_bus_index(void)
{
	struct platform_device *pdev;
	if (!wlan_data)
		return -EINVAL;

	pdev = wlan_data->pdev;
	dev_info(&pdev->dev,"bus_index: %d\n",wlan_data->bus_index);
	return wlan_data->bus_index;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_get_bus_index);

int sunxi_wlan_get_usb_index(void)
{
	struct platform_device *pdev;
	if (!wlan_data)
		return -EINVAL;

	pdev = wlan_data->pdev;
	dev_info(&pdev->dev, "usb_index: %d\n", wlan_data->usb_index);
	return wlan_data->usb_index;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_get_usb_index);

int sunxi_wlan_get_oob_irq(void)
{
	struct platform_device *pdev;
	int host_oob_irq = 0;
	if (!wlan_data || !gpio_is_valid(wlan_data->gpio_wlan_hostwake))
		return 0;

	pdev = wlan_data->pdev;
	host_oob_irq = gpio_to_irq(wlan_data->gpio_wlan_hostwake);
	if (IS_ERR_VALUE(host_oob_irq)) 
		dev_err(&pdev->dev,"map gpio [%d] to virq failed, errno = %d\n",
			wlan_data->gpio_wlan_hostwake,host_oob_irq);

	return host_oob_irq;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_get_oob_irq);

int sunxi_wlan_get_oob_irq_flags(void)
{
	int oob_irq_flags;
	if (!wlan_data)
		return 0;

	oob_irq_flags = (IRQF_TRIGGER_HIGH | IRQF_SHARED | IRQF_NO_SUSPEND);

	return oob_irq_flags;
}
EXPORT_SYMBOL_GPL(sunxi_wlan_get_oob_irq_flags);

static int sunxi_wlan_on(struct sunxi_wlan_platdata *data, bool on_off)
{
	struct platform_device *pdev = data->pdev;
	struct device *dev = &pdev->dev;
	int ret = 0;

	if (!on_off && gpio_is_valid(data->gpio_wlan_regon))
		gpio_direction_output(data->gpio_wlan_regon, 0);

	if (!on_off && gpio_is_valid(data->gpio_wlan_en)) {
		if (!bt_power_state)
			gpio_direction_output(data->gpio_wlan_en, 0);
	}

	if(data->wlan_power_name){
		data->wlan_power = regulator_get(dev, data->wlan_power_name);
		if (!IS_ERR(data->wlan_power)) {
			if(on_off){
				ret = regulator_enable(data->wlan_power);
				if (ret < 0){
					dev_err(dev, "regulator wlan_power enable failed\n");
					regulator_put(data->wlan_power);
					return ret;
				}

				ret = regulator_get_voltage(data->wlan_power);
				if (ret < 0){
					dev_err(dev, "regulator wlan_power get voltage failed\n");
					regulator_put(data->wlan_power);
					return ret;
				}
				dev_info(dev, "check wlan wlan_power voltage: %d\n",ret);
			}else{
				ret = regulator_disable(data->wlan_power);
				if (ret < 0){
					dev_err(dev, "regulator wlan_power disable failed\n");
					regulator_put(data->wlan_power);
					return ret;
				}
			}
			regulator_put(data->wlan_power);
		}
	}
	
	if(data->io_regulator_name){
		data->io_regulator = regulator_get(dev, data->io_regulator_name);
		if (!IS_ERR(data->io_regulator)) {
			if(on_off){
				ret = regulator_enable(data->io_regulator);
				if (ret < 0){
					dev_err(dev, "regulator io_regulator enable failed\n");
					regulator_put(data->io_regulator);
					return ret;
				}

				ret = regulator_get_voltage(data->io_regulator);
				if (ret < 0){
					dev_err(dev, "regulator io_regulator get voltage failed\n");
					regulator_put(data->io_regulator);
					return ret;
				}
				dev_info(dev, "check wlan io_regulator voltage: %d\n",ret);
			}else{
				ret = regulator_disable(data->io_regulator);
				if (ret < 0){
					dev_err(dev, "regulator io_regulator disable failed\n");
					regulator_put(data->io_regulator);
					return ret;
				}
			}
			regulator_put(data->io_regulator);
		}
	}

	if (on_off && gpio_is_valid(data->gpio_wlan_en)) {
		if (!bt_power_state)
			gpio_direction_output(data->gpio_wlan_en, 1);
	}

	if (on_off && gpio_is_valid(data->gpio_wlan_regon)) {
		mdelay(10);
		gpio_direction_output(data->gpio_wlan_regon, 1);
	}
	wlan_data->power_state = on_off;

	return 0;
}

static ssize_t power_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", wlan_data->power_state);
}

static ssize_t power_state_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long state;
	int err;

	if (!capable(CAP_NET_ADMIN))
		return -EPERM;

	err = kstrtoul(buf, 0, &state);
	if (err)
		return err;

	if (state > 1 )
		return -EINVAL;

	mutex_lock(&sunxi_wlan_mutex);
	if(state != wlan_data->power_state){
		err = sunxi_wlan_on(wlan_data, state);
		if(err)
			dev_err(dev,"set power failed\n");
	}
	mutex_unlock(&sunxi_wlan_mutex);

	return count;
}

extern void sunxi_mmc_rescan_card(unsigned ids);
extern int sunxi_usb_disable_hcd(__u32 usbc_no);
extern int sunxi_usb_enable_hcd(__u32 usbc_no);
static ssize_t scan_device_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int bus = wlan_data->bus_index;
	int usb = wlan_data->usb_index;

	dev_info(dev, "start scan device on bus_index: %d\n",
			wlan_data->bus_index);
	if (bus < 0) {
		dev_err(dev, "scan device fail!\n");
		return -1;
	}
	sunxi_mmc_rescan_card(bus);

	dev_info(dev, "start scan device on usb_index: %d\n",
			wlan_data->usb_index);
	if (usb < 0) {
		dev_err(dev, "scan device fail!\n");
		return -1;
	}
	sunxi_usb_enable_hcd(usb);

	return count;
}

/*
static DEVICE_ATTR(power_state, S_IWUSR | S_IWGRP | S_IRUGO,
		power_state_show, power_state_store);
static DEVICE_ATTR(scan_device, S_IWUSR | S_IWGRP,
		NULL, scan_device_store);
*/
static DEVICE_ATTR(power_state, 0666,
		power_state_show, power_state_store);
static DEVICE_ATTR(scan_device, 0666,
		NULL, scan_device_store);

static struct attribute *misc_attributes[] = {
	&dev_attr_power_state.attr,
	&dev_attr_scan_device.attr,
	NULL,
};

static struct attribute_group misc_attribute_group = {
	.name  = "rf-ctrl",
	.attrs = misc_attributes,
};

static struct miscdevice sunxi_wlan_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "sunxi-wlan",
};

static char wifi_mac_str[18] = {0};
extern int sunxi_get_soc_chipid(uint8_t *chipid);
static void sunxi_wlan_chipid_mac_address(u8 *mac)
{
#define MD5_SIZE	16
#define CHIP_SIZE	16

	struct crypto_hash *tfm;
	struct hash_desc desc;
	struct scatterlist sg;
	u8 result[MD5_SIZE];
	u8 chipid[CHIP_SIZE];
	int i = 0;
	int ret = -1;

	memset(chipid, 0, sizeof(chipid));
	memset(result, 0, sizeof(result));

	sunxi_get_soc_chipid((u8 *)chipid);

	tfm = crypto_alloc_hash("md5", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		pr_err("Failed to alloc md5\n");
		return;
	}
	desc.tfm = tfm;
	desc.flags = 0;

	ret = crypto_hash_init(&desc);
	if (ret < 0) {
		pr_err("crypto_hash_init() failed\n");
		goto out;
	}

	sg_init_one(&sg, chipid, sizeof(chipid) - 1);
	ret = crypto_hash_update(&desc, &sg, sizeof(chipid) - 1);
	if (ret < 0) {
		pr_err("crypto_hash_update() failed for id\n");
		goto out;
	}

	crypto_hash_final(&desc, result);
	if (ret < 0) {
		pr_err("crypto_hash_final() failed for result\n");
		goto out;
	}

	/* Choose md5 result's [0][2][4][6][8][10] byte as mac address */
	for (i = 0; i < 6; i++)
		mac[i] = result[2*i];
	mac[0] &= 0xfe;     /* clear multicast bit */
	mac[0] &= 0xfd;     /* clear local assignment bit (IEEE802) */

out:
	crypto_free_hash(tfm);
}
EXPORT_SYMBOL(sunxi_wlan_chipid_mac_address);

void sunxi_wlan_custom_mac_address(u8 *mac)
{
	int i;
	char *p = wifi_mac_str;
	u8 mac_addr[ETH_ALEN] = {0};

	if (0 == strlen(p))
		return;

	for (i = 0; i < ETH_ALEN; i++, p++)
		mac_addr[i] = simple_strtoul(p, &p, 16);

	memcpy(mac, mac_addr, sizeof(mac_addr));
}
EXPORT_SYMBOL(sunxi_wlan_custom_mac_address);

#ifndef MODULE
static int __init set_wlan_mac_addr(char *str)
{
	char *p = str;

	if (str != NULL && *str)
		strlcpy(wifi_mac_str, p, 18);

	return 0;
}
__setup("wifi_mac=", set_wlan_mac_addr);
#endif

static int sunxi_wlan_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct sunxi_wlan_platdata *data;
	struct gpio_config config;
	unsigned long pin_config;
	u32 val;
	const char *power,*io_regulator;
	int ret = 0;
	int clk_gpio;
	char pin_name[32];

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	data->pdev = pdev;
	wlan_data = data;

	data->bus_index = -1;
	if (!of_property_read_u32(np, "wlan_busnum", &val)) {
		switch (val) {
		case 0:
		case 1:
		case 2:
			data->bus_index = val;
			break;
		default:
			dev_err(dev, "unsupported wlan_busnum (%u)\n", val);
			return -EINVAL;
		}
	}
	dev_info(dev, "wlan_busnum (%u)\n", val);

	data->usb_index = -1;
	if (!of_property_read_u32(np, "wlan_usbnum", &val)) {
		switch (val) {
		case 0:
		case 1:
		case 2:
		case 3:
			data->usb_index = val;
			break;
		default:
			dev_err(dev, "unsupported wlan_usbnum (%u)\n", val);
			return -EINVAL;
		}
	}
	dev_info(dev, "wlan_usbnum (%u)\n", val);

	if (of_property_read_string(np, "wlan_power", &power)) {
		dev_warn(dev, "Missing wlan_power.\n");
	}else{
		data->wlan_power_name = devm_kzalloc(dev, 64, GFP_KERNEL);
		if(!data->wlan_power_name)
			return -ENOMEM;
		else
			strcpy(data->wlan_power_name,power);
	}
	dev_info(dev, "wlan_power_name (%s)\n", data->wlan_power_name);

	if (of_property_read_string(np, "wlan_io_regulator", &io_regulator)) {
		dev_warn(dev, "Missing wlan_io_regulator.\n");
	}else{
		data->io_regulator_name = devm_kzalloc(dev, 64, GFP_KERNEL);
		if(!data->io_regulator_name)
			return -ENOMEM;
		else
			strcpy(data->io_regulator_name,io_regulator);
	}
	dev_info(dev, "io_regulator_name (%s)\n", data->io_regulator_name);

	data->gpio_wlan_regon = of_get_named_gpio_flags(np, "wlan_regon", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_wlan_regon)) {
		dev_err(dev, "get gpio wlan_regon failed\n");
	} else {
		dev_info(dev, "wlan_regon gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_wlan_regon,
				"wlan_regon");
		if (ret < 0) {
			dev_err(dev, "can't request wlan_regon gpio %d\n",
				data->gpio_wlan_regon);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_wlan_regon, 0);
		if (ret < 0) {
			dev_err(dev, "can't request output direction wlan_regon gpio %d\n",
				data->gpio_wlan_regon);
			return ret;
		}
	}

	data->gpio_wlan_en = of_get_named_gpio_flags(np, "wlan_en", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_wlan_en)) {
		dev_err(dev, "get gpio wlan_en failed\n");
	} else {
		dev_info(dev, "wlan_en gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_wlan_en,
				"wlan_en");
		if (ret < 0) {
			dev_err(dev, "can't request wlan_en gpio %d\n",
				data->gpio_wlan_regon);
			return ret;
		}

		ret = gpio_direction_output(data->gpio_wlan_en, 0);
		if (ret < 0) {
			dev_err(dev, "can't request output direction wlan_en gpio %d\n",
				data->gpio_wlan_en);
			return ret;
		}
	}

	data->gpio_wlan_hostwake = of_get_named_gpio_flags(np, "wlan_hostwake", 0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(data->gpio_wlan_hostwake)) {
		dev_err(dev, "get gpio wlan_hostwake failed\n");
	} else {
		dev_info(dev, "wlan_hostwake gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, data->gpio_wlan_hostwake,
				"wlan_hostwake");
		if (ret < 0) {
			dev_err(dev, "can't request wlan_hostwake gpio %d\n",
				data->gpio_wlan_hostwake);
			return ret;
		}

		gpio_direction_input(data->gpio_wlan_hostwake);
		if (ret < 0) {
			dev_err(dev, "can't request input direction wlan_hostwake gpio %d\n",
				data->gpio_wlan_hostwake);
			return ret;
		}
	}

	clk_gpio = of_get_named_gpio_flags(np, "wlan_clk_gpio", 0,
						(enum of_gpio_flags *)&config);
	if (!gpio_is_valid(clk_gpio)) {
		dev_err(dev, "get gpio wlan_clk_gpio failed\n");
	} else {
		dev_info(dev, "wlan_clk_gpio gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);

		ret = devm_gpio_request(dev, clk_gpio, "wlan_clk_gpio");
		if (ret < 0) {
			dev_err(dev, "can't request wlan_clk_gpio gpio %d\n",
				clk_gpio);
			return ret;
		}

		sunxi_gpio_to_name(config.gpio, pin_name);
		pin_config = SUNXI_PINCFG_PACK(SUNXI_PINCFG_TYPE_FUNC,
					config.mul_sel);
		pin_config_set(SUNXI_PINCTRL, pin_name, pin_config);
	}

	data->lpo = of_clk_get(np, 0);
	if (IS_ERR_OR_NULL(data->lpo)){
		dev_warn(dev, "clk not config\n");
	}else{
		dev_warn(dev, "enable clk\n");
		ret = clk_prepare_enable(data->lpo);
		if (ret < 0) 
			dev_warn(dev, "can't enable clk\n");
	}

	ret = misc_register(&sunxi_wlan_dev);
	if (ret) {
		dev_err(dev, "sunxi-wlan register driver as misc device error!\n");
		return ret;
	}
	ret = sysfs_create_group(&sunxi_wlan_dev.this_device->kobj,
			&misc_attribute_group);
	if (ret) {
		dev_err(dev, "sunxi-wlan register sysfs create group failed!\n");
		return ret;
	}

	data->power_state = 0;

	return 0;
}

static int sunxi_wlan_remove(struct platform_device *pdev)
{
	WARN_ON(0 != misc_deregister(&sunxi_wlan_dev));
	sysfs_remove_group(&(sunxi_wlan_dev.this_device->kobj),
			&misc_attribute_group);

	if (!IS_ERR_OR_NULL(wlan_data->lpo)) {
		clk_disable_unprepare(wlan_data->lpo);
		clk_put(wlan_data->lpo);
	}

	return 0;
}

static const struct of_device_id sunxi_wlan_ids[] = {
	{ .compatible = "allwinner,sunxi-wlan" },
	{ /* Sentinel */ }
};

static struct platform_driver sunxi_wlan_driver = {
	.probe	= sunxi_wlan_probe,
	.remove	= sunxi_wlan_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "sunxi-wlan",
		.of_match_table	= sunxi_wlan_ids,
	},
};

module_platform_driver(sunxi_wlan_driver);

MODULE_DESCRIPTION("sunxi wlan driver");
MODULE_LICENSE(GPL);

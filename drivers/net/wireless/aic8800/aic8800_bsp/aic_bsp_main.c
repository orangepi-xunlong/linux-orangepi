#include <linux/module.h>
#include <linux/inetdevice.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>
#include "aic_bsp_driver.h"

#define DRV_DESCRIPTION       "AIC BSP"
#define DRV_COPYRIGHT         "Copyright(c) 2015-2020 AICSemi"
#define DRV_AUTHOR            "AICSemi"
#define DRV_VERS_MOD          "1.0"

#if defined(AICWF_SDIO_SUPPORT)
#define DRV_TYPE_NAME   "compatible(sdio)"
#elif defined(AICWF_USB_SUPPORT)
#define DRV_TYPE_NAME   "compatible(usb)"
#else
#define DRV_TYPE_NAME   "compatible(unknow)"
#endif

#define DRV_RELEASE_DATE "20240919"
#define DRV_PATCH_LEVEL  "001"
#define DRV_RELEASE_TAG  "aic-bsp-" DRV_TYPE_NAME "-" DRV_RELEASE_DATE "-" DRV_PATCH_LEVEL

static struct platform_device *aicbsp_pdev;

const struct aicbsp_firmware *aicbsp_firmware_list;

struct aicbsp_info_t aicbsp_info = {
	.hwinfo_r = AICBSP_HWINFO_DEFAULT,
	.hwinfo   = AICBSP_HWINFO_DEFAULT,
	.cpmode   = AICBSP_CPMODE_DEFAULT,
	.sdio_clock = -1,
	.sdio_phase = -1,
	.btmode = -1,
};

struct mutex aicbsp_power_lock;

static void aicbsp_shutdown(struct platform_device *pdev)
{
	(void)pdev;
}

static struct platform_driver aicbsp_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "aic-bsp",
	},
	.shutdown = aicbsp_shutdown,
	//.probe = aicbsp_probe,
	//.remove = aicbsp_remove,
};

static int test_enable = -1;
module_param(test_enable, int, S_IRUGO);
MODULE_PARM_DESC(test_enable, "Set driver to test mode as default");

static int adap_test = -1;
module_param(adap_test, int, 0660);
MODULE_PARM_DESC(adap_test, "Set driver to adap test");

static ssize_t cpmode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;
	uint8_t i = 0;

	if (aicbsp_firmware_list == NULL) {
		count += sprintf(&buf[count], "Wi-Fi not opened since system power on\n");
		return count;
	}

	count += sprintf(&buf[count], "Support mode value:\n");

	for (i = 0; i < 2; i++) {
		if (aicbsp_firmware_list[i].desc)
			count += sprintf(&buf[count], " %2d: %s\n", i, aicbsp_firmware_list[i].desc);
	}

	count += sprintf(&buf[count], "Current: %d, firmware info:\n", aicbsp_info.cpmode);
	count += sprintf(&buf[count], "  BT ADID : %s\n", aicbsp_firmware_list[aicbsp_info.cpmode].bt_adid);
	count += sprintf(&buf[count], "  BT PATCH: %s\n", aicbsp_firmware_list[aicbsp_info.cpmode].bt_patch);
	count += sprintf(&buf[count], "  BT TABLE: %s\n", aicbsp_firmware_list[aicbsp_info.cpmode].bt_table);
	count += sprintf(&buf[count], "  WIFI FW : %s\n", aicbsp_firmware_list[aicbsp_info.cpmode].wl_fw);
	return count;
}

static ssize_t cpmode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long val;
	int err = kstrtoul(buf, 0, &val);
	if (err)
		return err;

	if (val >= AICBSP_CPMODE_MAX) {
		pr_err("mode value must less than %d\n", AICBSP_CPMODE_MAX);
		return -EINVAL;
	}

	aicbsp_info.cpmode = val;
	printk("%s, set mode to: %lu[%s] done\n", __func__, val, aicbsp_firmware_list ? aicbsp_firmware_list[val].desc : "unknow");

	return count;
}

static ssize_t hwinfo_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	if (aicbsp_info.chipinfo == NULL)
		count += sprintf(&buf[count], "chip info not avalible)\n");
	else
		count += sprintf(&buf[count], "chip name: %s, id: 0x%02X, rev: 0x%02X, subrev: 0x%02X\n",
					aicbsp_info.chipinfo->name, aicbsp_info.chipinfo->chipid,
					aicbsp_info.chipinfo->rev, aicbsp_info.chipinfo->subrev);

	count += sprintf(&buf[count], "hwinfo read: ");
	if (aicbsp_info.hwinfo_r < 0)
		count += sprintf(&buf[count], "%d(not avalible), ", aicbsp_info.hwinfo_r);
	else
		count += sprintf(&buf[count], "0x%02X, ", aicbsp_info.hwinfo_r);

	if (aicbsp_info.hwinfo < 0)
		count += sprintf(&buf[count], "set: %d(not avalible)\n", aicbsp_info.hwinfo);
	else
		count += sprintf(&buf[count], "set: 0x%02X\n", aicbsp_info.hwinfo);

	return count;
}

static ssize_t hwinfo_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	long val;
	int err = kstrtol(buf, 0, &val);

	if (err) {
		pr_err("invalid input\n");
		return err;
	}

	if ((val == -1) || (val >= 0 && val <= 0xFF)) {
		aicbsp_info.hwinfo = val;
	} else {
		pr_err("invalid values\n");
		return -EINVAL;
	}
	return count;
}

static ssize_t fwdebug_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	count += sprintf(&buf[count], "fw log status: %s\n",
			aicbsp_info.fwlog_en ? "on" : "off");

	return count;
}

static ssize_t fwdebug_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	long val;
	int err = kstrtol(buf, 0, &val);

	if (err) {
		pr_err("invalid input\n");
		return err;
	}

	if (val > 1 || val < 0) {
		pr_err("must be 0 or 1\n");
		return err;
	}

	aicbsp_info.fwlog_en = val;
	return count;
}

static ssize_t sdio_clock_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	count += sprintf(&buf[count], "sdio clock: %d\n", aicbsp_info.sdio_clock);

	return count;
}

static ssize_t sdio_clock_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	long val;
	int err = kstrtol(buf, 0, &val);

	if (err) {
		pr_err("invalid input\n");
		return err;
	}
	aicbsp_info.sdio_clock = val;
	return count;
}

static ssize_t sdio_phase_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	count += sprintf(&buf[count], "sdio phase: 0x%02X\n", aicbsp_info.sdio_phase);

	return count;
}

static ssize_t sdio_phase_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	long val;
	int err = kstrtol(buf, 0, &val);

	if (err) {
		pr_err("invalid input\n");
		return err;
	}

	if (val < 0) {
		pr_err("must greater than 0\n");
		return err;
	}

	aicbsp_info.sdio_phase = val;
	return count;
}

static ssize_t btpcm_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	count += sprintf(&buf[count], "%d\n", aicbsp_info.btpcm);

	return count;
}

static ssize_t btmode_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	ssize_t count = 0;

	count += sprintf(&buf[count], "%d\n", aicbsp_info.btmode);

	return count;
}

static ssize_t btmode_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	long val;
	int err = kstrtol(buf, 0, &val);

	if (err) {
		pr_err("invalid input\n");
		return err;
	}

	if (val < 0) {
		pr_err("must greater than 0\n");
		return val;
	}

	if (val >= AICBT_BTMODE_BT_ONLY_SW && val <= AICBT_BTMODE_NULL)
		aicbsp_info.btmode = val;
	else
		aicbsp_info.btmode = -1;
	return count;
}

static DEVICE_ATTR(cpmode, S_IRUGO | S_IWUSR,
		cpmode_show, cpmode_store);

static DEVICE_ATTR(hwinfo, S_IRUGO | S_IWUSR,
		hwinfo_show, hwinfo_store);

static DEVICE_ATTR(fwdebug, S_IRUGO | S_IWUSR,
		fwdebug_show, fwdebug_store);

static DEVICE_ATTR(sdio_clock, S_IRUGO | S_IWUSR,
		sdio_clock_show, sdio_clock_store);

static DEVICE_ATTR(sdio_phase, S_IRUGO | S_IWUSR,
		sdio_phase_show, sdio_phase_store);

static DEVICE_ATTR(btpcm, S_IRUGO | S_IWUSR,
		btpcm_show, NULL);

static DEVICE_ATTR(btmode, S_IRUGO | S_IWUSR,
		btmode_show, btmode_store);

static struct attribute *aicbsp_attributes[] = {
	&dev_attr_cpmode.attr,
	&dev_attr_hwinfo.attr,
	&dev_attr_fwdebug.attr,
	&dev_attr_sdio_clock.attr,
	&dev_attr_sdio_phase.attr,
	&dev_attr_btpcm.attr,
	&dev_attr_btmode.attr,
	NULL,
};

static struct attribute_group aicbsp_attribute_group = {
	.name  = "aicbsp_info",
	.attrs = aicbsp_attributes,
};

static int aicbt_set_power(void *data, bool blocked)
{
	pr_info("%s: start_block=%d\n", __func__, blocked);
	if (!blocked) {
		aicbsp_set_subsys(AIC_BLUETOOTH, AIC_PWR_ON);
	} else {
		aicbsp_set_subsys(AIC_BLUETOOTH, AIC_PWR_OFF);
	}

	pr_info("%s: end_block=%d\n", __func__, blocked);
	return 0;
}

static struct rfkill_ops aicbt_rfkill_ops = {
	.set_block = aicbt_set_power,
};

static struct rfkill *aicbt_rfk;

static int aicbt_rfkill_init(struct platform_device *pdev)
{
	int rc = 0;

	pr_info("-->%s\n", __func__);
	aicbt_rfk = rfkill_alloc("bluetooth", &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			&aicbt_rfkill_ops, NULL);
	if (!aicbt_rfk) {
		rc = -ENOMEM;
		goto err_rfkill_alloc;
	}
	/* userspace cannot take exclusive control */
	rfkill_init_sw_state(aicbt_rfk, true);
	rc = rfkill_register(aicbt_rfk);
	if (rc)
		goto err_rfkill_reg;

	pr_info("<--%s\n", __func__);

	return 0;

err_rfkill_reg:
	rfkill_destroy(aicbt_rfk);
err_rfkill_alloc:
	return rc;
}

static int aicbt_rfkill_remove(struct platform_device *dev)
{
	pr_info("-->%s\n", __func__);
	rfkill_unregister(aicbt_rfk);
	rfkill_destroy(aicbt_rfk);
	pr_info("<--%s\n", __func__);
	return 0;
}

static int __init aicbsp_init(void)
{
	int ret;
	printk("%s\n", __func__);
	printk("%s, Driver Release Tag: %s\n", __func__, DRV_RELEASE_TAG);

	aicbsp_resv_mem_init();
	mutex_init(&aicbsp_power_lock);
	ret = platform_driver_register(&aicbsp_driver);
	if (ret) {
		pr_err("register platform driver failed: %d\n", ret);
		goto err0;
	}

	aicbsp_pdev = platform_device_alloc("aic-bsp", -1);
	ret = platform_device_add(aicbsp_pdev);
	if (ret) {
		pr_err("register platform device failed: %d\n", ret);
		goto err1;
	}

	ret = sysfs_create_group(&(aicbsp_pdev->dev.kobj), &aicbsp_attribute_group);
	if (ret) {
		pr_err("register sysfs create group failed!\n");
		goto err2;
	}

	if (test_enable == 1) {
		aicbsp_info.cpmode = AICBSP_CPMODE_TEST;
		printk("aicbsp: Test mode enable!!!\n");
	}

	if (adap_test == 1) {
		aicbsp_info.adap_test = 1;
		printk("aicbsp: adap_test enable!!!\n");
	}

	ret = aicbt_rfkill_init(aicbsp_pdev);
	if (ret) {
		pr_err("register rfkill fail\n");
		goto err3;
	}

	ret = aicbsp_device_init();
	if (ret) {
		pr_err("bsp subsys init fail!\n");
		goto err4;
	}

	return 0;

err4:
	aicbt_rfkill_remove(aicbsp_pdev);
err3:
	sysfs_remove_group(&(aicbsp_pdev->dev.kobj), &aicbsp_attribute_group);
err2:
	platform_device_del(aicbsp_pdev);
err1:
	platform_driver_unregister(&aicbsp_driver);
err0:
	mutex_destroy(&aicbsp_power_lock);
	aicbsp_resv_mem_deinit();
	return ret;
}

static void __exit aicbsp_exit(void)
{
	aicbsp_device_exit();
	aicbt_rfkill_remove(aicbsp_pdev);
	sysfs_remove_group(&(aicbsp_pdev->dev.kobj), &aicbsp_attribute_group);
	platform_device_del(aicbsp_pdev);
	platform_driver_unregister(&aicbsp_driver);
	mutex_destroy(&aicbsp_power_lock);
	aicbsp_resv_mem_deinit();
	printk("%s\n", __func__);
}

module_init(aicbsp_init);
module_exit(aicbsp_exit);

MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_VERSION(DRV_VERS_MOD);
MODULE_AUTHOR(DRV_COPYRIGHT " " DRV_AUTHOR);
MODULE_LICENSE("GPL");

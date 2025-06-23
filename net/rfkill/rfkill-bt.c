// SPDX-License-Identifier: GPL-2.0
/*
 * rfkill bt driver for the cix bt
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/rfkill.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <asm/irq.h>
#include <linux/suspend.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <dt-bindings/gpio/gpio.h>
#include <uapi/linux/rfkill.h>
#include <linux/input.h>
#include "rfkill-bt.h"
#include <linux/of.h>

static struct rfkill_cix_platform_data *g_rfkill = NULL;

static inline void rfkill_sleep_bt_internal(struct rfkill_cix_platform_data *rfkill, bool sleep)
{
	if (rfkill->gpiod_wake_bt == NULL) {
		DBG("*** Not support bt wakeup and sleep\n");
		return;
	}

	if (!sleep) {
		gpiod_set_value_cansleep(rfkill->gpiod_wake_bt, !GPIO_WAKE_BT);
		usleep_range(10, 20);
		gpiod_set_value_cansleep(rfkill->gpiod_wake_bt, GPIO_WAKE_BT);
	}
}

void rfkill_sleep_bt(bool sleep)
{
	struct rfkill_cix_platform_data *rfkill = g_rfkill;

	DBG("Enter %s\n", __func__);

	if (!rfkill) {
		LOG("*** RFKILL is empty???\n");
		return;
	}

	rfkill_sleep_bt_internal(rfkill, sleep);
}

static ssize_t bluesleep_read_proc_lpm(struct file *file, char __user *buffer,
				   size_t count, loff_t *data)
{
	return sprintf(buffer, "unsupported to read\n");
}

static ssize_t bluesleep_write_proc_lpm(struct file *file,
					const char __user *buffer, size_t count,
					loff_t *data)
{
	return count;
}

static ssize_t bluesleep_read_proc_btwrite(struct file *file,
					char __user *buffer, size_t count,
					loff_t *data)
{
	return sprintf(buffer, "unsupported to read\n");
}

static ssize_t bluesleep_write_proc_btwrite(struct file *file,
					const char __user *buffer,
					size_t count, loff_t *data)
{
	char b;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&b, buffer, 1))
		return -EFAULT;

	DBG("btwrite %c\n", b);
	/* HCI_DEV_WRITE */
	if (b != '0')
		rfkill_sleep_bt(true);
	else
		rfkill_sleep_bt(false);

	return count;
}

static const struct proc_ops bluesleep_lpm = {
	.proc_read = bluesleep_read_proc_lpm,
	.proc_write = bluesleep_write_proc_lpm,
};

static const struct proc_ops bluesleep_btwrite = {
	.proc_read = bluesleep_read_proc_btwrite,
	.proc_write = bluesleep_write_proc_btwrite,
};

static int rfkill_cix_set_power(void *data, bool blocked)
{
	//struct rfkill_cix_platform_data *pdata = data;

	DBG("Enter %s\n", __func__);

	if (!blocked) {
		LOG("bt turn on power\n");
		// if (gpiod_get_value_cansleep(pdata->gpiod_reset) != GPIO_ENABLE) {
		//	 gpiod_set_value_cansleep(pdata->gpiod_reset, 0);
		//	 msleep(20);
		//	 gpiod_set_value_cansleep(pdata->gpiod_reset, 1);
		// }
	} else {
		LOG("bt shut off power\n");
		// if (gpiod_get_value_cansleep(pdata->gpiod_reset) == GPIO_ENABLE) {
		//	 gpiod_set_value_cansleep(pdata->gpiod_reset, 0);
		// }
	}

	return 0;
}

static const struct rfkill_ops rfkill_cix_ops = {
	.set_block = rfkill_cix_set_power,
};

static int irq_init_input(struct rfkill_cix_platform_data *rfkill)
{
	int ret;
	rfkill->input = devm_input_allocate_device(&rfkill->pdev->dev);
	if (!rfkill->input) {
		dev_err(&rfkill->pdev->dev, "%s: fail to allocate input device\n", __func__);
		return -ENOMEM;
	}
	rfkill->input->name = dev_name(&rfkill->pdev->dev);
	rfkill->input->phys = dev_name(&rfkill->pdev->dev);
	/* virtual power key */
	input_set_capability(rfkill->input, EV_KEY, KEY_POWER);
	ret = input_register_device(rfkill->input);
	if (ret) {
		dev_err(&rfkill->pdev->dev, "fail to register input device\n");
		return ret;
	}
	return 0;
}


static irqreturn_t rfkill_wake_host_irq(int irq, void *dev)
{
	struct rfkill_cix_platform_data *rfkill = dev;

	LOG("BT_WAKE_HOST IRQ fired\n");

	/* wakeup system, screen on */
	if (!rfkill->input) return IRQ_HANDLED;
	input_report_key(rfkill->input, KEY_POWER, 1);
	input_sync(rfkill->input);
	input_report_key(rfkill->input, KEY_POWER, 0);
	input_sync(rfkill->input);

	return IRQ_HANDLED;
}

static int rfkill_setup_wake_irq(struct rfkill_cix_platform_data *rfkill)
{
	int ret = 0;
	if (rfkill->gpiod_wake_host == NULL)
		return 0;

	rfkill->rfkill_irq = gpiod_to_irq(rfkill->gpiod_wake_host);

	ret = devm_request_irq(&rfkill->pdev->dev, rfkill->rfkill_irq, rfkill_wake_host_irq,
				GPIO_WAKE_HOST > 0 ? IRQF_TRIGGER_RISING : IRQF_TRIGGER_FALLING,
				rfkill->name, rfkill);
	if (ret) {
		return ret;
	}
	LOG("** disable irq\n");
	disable_irq(rfkill->rfkill_irq);
	ret = enable_irq_wake(rfkill->rfkill_irq);
	if (ret) {
		free_irq(rfkill->rfkill_irq, rfkill);
	}
	return ret;
}

static inline void rfkill_free_wake_irq(struct rfkill_cix_platform_data *rfkill)
{
	if (rfkill->gpiod_wake_host == NULL)
		return;

	free_irq(rfkill->rfkill_irq, rfkill);
}

static inline void rfkill_remove_proc(struct rfkill_cix_platform_data *rfkill)
{
	if (rfkill->sleep_dir) {
		remove_proc_entry("btwrite", rfkill->sleep_dir);
		remove_proc_entry("lpm", rfkill->sleep_dir);
		proc_remove(rfkill->sleep_dir);
	}
	if (rfkill->bluetooth_dir) {
		proc_remove(rfkill->bluetooth_dir);
	}
}

static int rfkill_create_proc(struct rfkill_cix_platform_data *rfkill)
{
	int ret = 0;
	struct proc_dir_entry *ent;
	rfkill->bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (!rfkill->bluetooth_dir) {
		LOG("Unable to create /proc/bluetooth directory");
		ret = -ENOMEM;
		goto fail_create;
	}

	rfkill->sleep_dir = proc_mkdir("sleep", rfkill->bluetooth_dir);
	if (!rfkill->sleep_dir) {
		LOG("Unable to create /proc/%s directory", PROC_DIR);
		ret = -ENOMEM;
		goto fail_create;
	}

	/* read/write proc entries */
	ent = proc_create("lpm", 0444, rfkill->sleep_dir, &bluesleep_lpm);
	if (!ent) {
		LOG("Unable to create /proc/%s/lpm entry", PROC_DIR);
		ret = -ENOMEM;
		goto fail_create;
	}

	/* read/write proc entries */
	ent = proc_create("btwrite", 0444, rfkill->sleep_dir, &bluesleep_btwrite);
	if (!ent) {
		LOG("Unable to create /proc/%s/btwrite entry", PROC_DIR);
		ret = -ENOMEM;
		goto fail_create;
	}

	return ret;
fail_create:
	rfkill_remove_proc(rfkill);
	return ret;
}

static int bluetooth_platdata_parse_dt(struct device *dev,
				   struct rfkill_cix_platform_data *data)
{
	memset(data, 0, sizeof(*data));


	data->gpiod_reset = devm_gpiod_get_optional(dev, "BT,reset", GPIOD_OUT_HIGH);
	if (IS_ERR(data->gpiod_reset) || data->gpiod_reset == NULL) {
		LOG("Failed to get BT,reset-gpios gpio.\n");
		return PTR_ERR(data->gpiod_reset);
	}
	LOG("success to get BT,reset-gpios gpio.\n");
	data->gpiod_wake_bt = devm_gpiod_get_optional(dev, "BT,wake", GPIOD_OUT_HIGH);
	if (IS_ERR(data->gpiod_wake_bt) || data->gpiod_wake_bt == NULL) {
		LOG("BT,wake-gpios not used.\n");
		data->gpiod_wake_bt = NULL;
	}


	data->gpiod_wake_host = devm_gpiod_get_optional(dev, "BT,wake_host_irq", GPIOD_IN);
	if (IS_ERR(data->gpiod_wake_host) || data->gpiod_wake_host == NULL) {
		LOG("BT,wake_host_irq-gpios not used.\n");
		data->gpiod_wake_host = NULL;
	}

	return 0;
}

static int rfkill_cix_probe(struct platform_device *pdev)
{
	struct rfkill_cix_platform_data *pdata = pdev->dev.platform_data;
	int ret = 0;

	DBG("Enter %s\n", __func__);

	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(struct rfkill_cix_platform_data),
					GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;

		memset(pdata, 0, sizeof(struct rfkill_cix_platform_data));
		ret = bluetooth_platdata_parse_dt(&pdev->dev, pdata);
		if (ret < 0) {
			LOG("%s: No platform data specified\n", __func__);
			return ret;
		}
	}

	pdata->name = (char *)BT_NAME;
	pdata->type = RFKILL_TYPE_BLUETOOTH;
	pdata->pdev = pdev;

	ret = rfkill_setup_wake_irq(pdata);
	if (ret) {
		goto fail_setup_wake_irq;
	}

	rfkill_create_proc(pdata);

	DBG("setup rfkill\n");
	pdata->rfkill_dev = rfkill_alloc(pdata->name, &pdev->dev, pdata->type,
					&rfkill_cix_ops, pdata);
	if (!pdata->rfkill_dev)
		goto fail_alloc;

	rfkill_init_sw_state(pdata->rfkill_dev, true);
	rfkill_set_sw_state(pdata->rfkill_dev, true);
	rfkill_set_hw_state(pdata->rfkill_dev, false);
	ret = rfkill_register(pdata->rfkill_dev);
	if (ret < 0)
		goto fail_rfkill;

	platform_set_drvdata(pdev, pdata);
	LOG("%s device registered.\n", pdata->name);
	gpiod_set_value_cansleep(pdata->gpiod_reset, 0);
	msleep(20);
	gpiod_set_value_cansleep(pdata->gpiod_reset, 1);

	ret = irq_init_input(pdata);
	if(!ret)
		LOG("%s irq_init_input registered.\n", pdata->name);
	else
		LOG("%s irq_init_input unregistered.\n", pdata->name);

	g_rfkill = pdata;

	return 0;

fail_rfkill:
	rfkill_destroy(pdata->rfkill_dev);
fail_alloc:
	rfkill_remove_proc(pdata);
	rfkill_free_wake_irq(pdata);
fail_setup_wake_irq:
	devm_kfree(&pdev->dev, pdata);
	return ret;
}

static int rfkill_cix_remove(struct platform_device *pdev)
{
	struct rfkill_cix_platform_data *pdata = platform_get_drvdata(pdev);

	LOG("Enter %s\n", __func__);
	rfkill_unregister(pdata->rfkill_dev);
	rfkill_destroy(pdata->rfkill_dev);

	rfkill_remove_proc(pdata);
	rfkill_free_wake_irq(pdata);

	devm_kfree(&pdev->dev, pdata);

	return 0;
}

static struct of_device_id bt_platdata_of_match[] = {
	{ .compatible = "bluetooth-platdata" },
	{}
};
MODULE_DEVICE_TABLE(of, bt_platdata_of_match);


static struct platform_driver rfkill_cix_driver = {
	.probe = rfkill_cix_probe,
	.remove = rfkill_cix_remove,
	.driver = {
		.name = "rfkill_bt",
		.of_match_table = of_match_ptr(bt_platdata_of_match),
   },
};

static int __init rfkill_cix_init(void)
{
	LOG("Enter %s\n", __func__);

	return platform_driver_register(&rfkill_cix_driver);
}

static void __exit rfkill_cix_exit(void)
{
	LOG("Enter %s\n", __func__);
	platform_driver_unregister(&rfkill_cix_driver);
}

module_init(rfkill_cix_init);
module_exit(rfkill_cix_exit);

MODULE_DESCRIPTION("cix rfkill for Bluetooth v1");
MODULE_AUTHOR("Cixtech.inc");
MODULE_LICENSE("GPL");

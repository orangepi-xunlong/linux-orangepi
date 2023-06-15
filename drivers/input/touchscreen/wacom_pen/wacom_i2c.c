/*
 * Wacom Penabled Driver for I2C
 *
 * Copyright (c) 2011 - 2013 Tatsunosuke Tobita, Wacom.
 * <tobita.tatsunosuke@wacom.co.jp>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version of 2 of the License,
 * or (at your option) any later version.
 */

#define DEBUG

#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/pm_wakeirq.h>
#include <linux/pm_wakeup.h>
#include <asm/unaligned.h>
#include "../../init-input.h"

#define USE_IN_SUNXI	1

#define WACOM_CMD_QUERY0	0x04
#define WACOM_CMD_QUERY1	0x00
#define WACOM_CMD_QUERY2	0x33
#define WACOM_CMD_QUERY3	0x02
#define WACOM_CMD_THROW0	0x05
#define WACOM_CMD_THROW1	0x00
#define WACOM_QUERY_SIZE	19

/*
#ifdef pr_debug
#undef pr_debug
#define pr_debug printk
#endif
*/

struct wacom_features {
	int x_max;
	int y_max;
	int pressure_max;
	char fw_version;
};

struct wacom_i2c {
	struct i2c_client *client;
	struct input_dev *input;
	struct work_struct wa_work;
	u8 data[WACOM_QUERY_SIZE];
	bool prox;
	int tool;
#ifdef USE_IN_SUNXI
	int easy_wakeup_gesture;
	bool wait_until_wake;
	int sleep_state;
	struct mutex system_lock;
	wait_queue_head_t wait_q;
	struct device *dev;
#endif
};

#ifdef USE_IN_SUNXI
struct ctp_config_info config_info = {
	.input_type = CTP_TYPE,
	.name = NULL,
	.np_name = "emr",
	.int_number = 0,
};

enum wacom_sleep_state {
	SLEEP_OFF,
	SLEEP_ON,
};

static int screen_max_x;
static int screen_max_y;
static int revert_x_flag;
static int revert_y_flag;
static int exchange_x_y_flag;
static int twi_id = -1;
static int wakeup_report;
struct wacom_i2c *global_wac_i2c;
#endif

static struct wacom_features w_features;

static int wacom_query_device(struct i2c_client *client,
			      struct wacom_features *features)
{
	int ret;
	u8 cmd1[] = { WACOM_CMD_QUERY0, WACOM_CMD_QUERY1,
			WACOM_CMD_QUERY2, WACOM_CMD_QUERY3 };
	u8 cmd2[] = { WACOM_CMD_THROW0, WACOM_CMD_THROW1 };
	u8 data[WACOM_QUERY_SIZE];
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = sizeof(cmd1),
			.buf = cmd1,
		},
		{
			.addr = client->addr,
			.flags = 0,
			.len = sizeof(cmd2),
			.buf = cmd2,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = sizeof(data),
			.buf = data,
		},
	};

	ret = i2c_transfer(client->adapter, msgs, ARRAY_SIZE(msgs));
	if (ret < 0) {
		pr_debug("i2c_transfer is fail,ret = %d\n", ret);
		return ret;
    }
	if (ret != ARRAY_SIZE(msgs))
		return -EIO;

	features->x_max = get_unaligned_le16(&data[3]);
	features->y_max = get_unaligned_le16(&data[5]);
	features->pressure_max = get_unaligned_le16(&data[11]);
	features->fw_version = get_unaligned_le16(&data[13]);

	dev_dbg(&client->dev,
		"x_max:%d, y_max:%d, pressure:%d, fw:%d\n",
		features->x_max, features->y_max,
		features->pressure_max, features->fw_version);
	pr_debug("x_max:%d, y_max:%d, pressure:%d, fw:%d\n",
		features->x_max, features->y_max,
		features->pressure_max, features->fw_version);

	return 0;
}

static void wa_wake_func(struct work_struct *work)
{
	struct wacom_i2c *wac_i2c =
			container_of(work, struct wacom_i2c, wa_work);

	if (wakeup_report) {
		input_report_key(wac_i2c->input, KEY_WAKEUP, 1);
		input_sync(wac_i2c->input);

		msleep(100);

		input_report_key(wac_i2c->input, KEY_WAKEUP, 0);
		input_sync(wac_i2c->input);

		wakeup_report = 0;
	}
}

static irqreturn_t wacom_i2c_irq(int irq, void *dev_id)
{
	struct wacom_i2c *wac_i2c = dev_id;
	struct input_dev *input = wac_i2c->input;
	u8 *data = wac_i2c->data;
	unsigned int x, y, pressure;
	unsigned char tsw, f1, f2, ers;
	int error;
	int t;

#ifdef USE_IN_SUNXI
	if (wac_i2c->easy_wakeup_gesture) {
		mutex_lock(&wac_i2c->system_lock);
		if (wac_i2c->sleep_state == SLEEP_ON) {
			mutex_unlock(&wac_i2c->system_lock);
			pm_wakeup_hard_event(wac_i2c->dev);
			pr_debug("wait system resume\n");
			t = wait_event_timeout(wac_i2c->wait_q,
					wac_i2c->wait_until_wake,
					msecs_to_jiffies(2000));

			if (t == 0) {
				goto out;
			}
		} else {
			mutex_unlock(&wac_i2c->system_lock);
		}
	}
#endif

	error = i2c_master_recv(wac_i2c->client,
				wac_i2c->data, sizeof(wac_i2c->data));
	if (error < 0)
		goto out;

	tsw = data[3] & 0x01;
	ers = data[3] & 0x04;
	f1 = data[3] & 0x02;
	f2 = data[3] & 0x10;
	x = le16_to_cpup((__le16 *)&data[4]);
	y = le16_to_cpup((__le16 *)&data[6]);
	pressure = le16_to_cpup((__le16 *)&data[8]);

	if (!wac_i2c->prox)
		wac_i2c->tool = (data[3] & 0x0c) ?
			BTN_TOOL_RUBBER : BTN_TOOL_PEN;
#ifdef USE_IN_SUNXI
	wac_i2c->prox = data[3] & 0x20;
	x = x * screen_max_x / w_features.x_max;
	y = y * screen_max_y / w_features.y_max;
	if (revert_x_flag) {
		x = screen_max_x - x;
	}
	if (revert_y_flag) {
		y = screen_max_y - y;
	}
	if (exchange_x_y_flag) {
		int tmp = x;

		x = y;
		y = tmp;
	}
#endif
	mutex_lock(&wac_i2c->system_lock);
	if (wakeup_report) {
		pr_debug("wacom driver func = %s line = %d\n", __func__, __LINE__);
		input_report_key(input, KEY_WAKEUP, 1);
		input_sync(input);

		input_report_key(input, KEY_WAKEUP, 0);
		input_sync(input);

		wakeup_report = 0;
	}
	mutex_unlock(&wac_i2c->system_lock);

	//schedule_work(&wac_i2c->wa_work);
	input_report_key(input, BTN_TOUCH, tsw || ers);
	input_report_key(input, wac_i2c->tool, wac_i2c->prox);
	input_report_key(input, BTN_STYLUS, f1);
	input_report_key(input, BTN_STYLUS2, f2);
	input_report_abs(input, ABS_X, x);
	input_report_abs(input, ABS_Y, y);
	input_report_abs(input, ABS_PRESSURE, pressure);
	input_sync(input);
	//pr_debug("%s: %d, %d\n", __func__, x, y);

out:
	return IRQ_HANDLED;
}

static int wacom_i2c_open(struct input_dev *dev)
{
	struct wacom_i2c *wac_i2c = input_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	enable_irq(client->irq);

	return 0;
}

static void wacom_i2c_close(struct input_dev *dev)
{
	struct wacom_i2c *wac_i2c = input_get_drvdata(dev);
	struct i2c_client *client = wac_i2c->client;

	disable_irq(client->irq);
}

static int wacom_i2c_probe(struct i2c_client *client,
				     const struct i2c_device_id *id)
{
	struct wacom_i2c *wac_i2c;
	struct input_dev *input;
	struct wacom_features features = { 0 };
	int error;

	pr_debug("%s:enter \n", __func__);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_debug("i2c_check_functionality error\n");
		dev_err(&client->dev, "i2c_check_functionality error\n");
		return -EIO;
	}

	error = wacom_query_device(client, &features);
	if (error) {
		pr_debug("wacom_query_device is error, error = %d\n");
		return error;
    }
	wac_i2c = kzalloc(sizeof(*wac_i2c), GFP_KERNEL);
	input = input_allocate_device();
	if (!wac_i2c || !input) {
		pr_debug("%s:input_allocate_device fail\n", __func__);
		error = -ENOMEM;
		goto err_free_mem;
	}

	wac_i2c->client = client;
	wac_i2c->input = input;

	input->name = "Wacom I2C Digitizer";
	input->id.bustype = BUS_I2C;
	input->id.vendor = 0x56a;
	input->id.version = features.fw_version;
	input->dev.parent = &client->dev;
	input->open = wacom_i2c_open;
	input->close = wacom_i2c_close;

	input->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

	__set_bit(BTN_TOOL_PEN, input->keybit);
	__set_bit(BTN_TOOL_RUBBER, input->keybit);
	__set_bit(BTN_STYLUS, input->keybit);
	__set_bit(BTN_STYLUS2, input->keybit);
	__set_bit(BTN_TOUCH, input->keybit);
	__set_bit(KEY_WAKEUP, input->keybit);

	__set_bit(INPUT_PROP_DIRECT, input->propbit);

	input_set_abs_params(input, ABS_X, 0, config_info.screen_max_x, 0, 0); //features.x_max, 0, 0);
	input_set_abs_params(input, ABS_Y, 0, config_info.screen_max_y, 0, 0);
	input_set_abs_params(input, ABS_PRESSURE,
			     0, features.pressure_max, 0, 0);

	input_set_drvdata(input, wac_i2c);
#ifdef USE_IN_SUNXI
	client->irq = gpio_to_irq(config_info.int_number);
	device_init_wakeup(&client->dev, true);
	dev_pm_set_wake_irq(&client->dev, client->irq);

	wac_i2c->easy_wakeup_gesture = 1;
	wac_i2c->sleep_state = SLEEP_OFF;
	mutex_init(&wac_i2c->system_lock);
	init_waitqueue_head(&wac_i2c->wait_q);
	wac_i2c->dev = &client->dev;
	global_wac_i2c = wac_i2c;

	error = request_threaded_irq(client->irq, NULL, wacom_i2c_irq,
				     IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				     "wacom_i2c", wac_i2c);

#else
	error = request_threaded_irq(client->irq, NULL, wacom_i2c_irq,
				     IRQF_TRIGGER_LOW | IRQF_ONESHOT,
				     "wacom_i2c", wac_i2c);
#endif
	INIT_WORK(&wac_i2c->wa_work, wa_wake_func);

	if (error) {
		pr_debug("fail to enable IRQ,error :%d\n", error);
		dev_err(&client->dev,
			"Failed to enable IRQ, error: %d\n", error);
		goto err_free_mem;
	}

	/* Disable the IRQ, we'll enable it in wac_i2c_open() */
	disable_irq(client->irq);

	error = input_register_device(wac_i2c->input);
	if (error) {
		pr_debug("Failed to register input device, error: %d\n", error);
		dev_err(&client->dev,
			"Failed to register input device, error: %d\n", error);
		goto err_free_irq;
	}

	i2c_set_clientdata(client, wac_i2c);
	w_features = features;
	pr_debug("wacom_i2c_probe succeed!\n");
	return 0;

err_free_irq:
	free_irq(client->irq, wac_i2c);
err_free_mem:

	input_free_device(input);
	kfree(wac_i2c);

	pr_err("wacom_i2c_probe failed!\n");
	return error;
}

static int wacom_i2c_remove(struct i2c_client *client)
{
	struct wacom_i2c *wac_i2c = i2c_get_clientdata(client);

	free_irq(client->irq, wac_i2c);
	input_unregister_device(wac_i2c->input);
	kfree(wac_i2c);

	return 0;
}

static int wacom_i2c_suspend(struct device *c_dev)
{
#ifdef USE_IN_SUNXI
	//disable_irq(client->irq);
	if (global_wac_i2c->easy_wakeup_gesture) {
		mutex_lock(&global_wac_i2c->system_lock);
		pr_debug("wacom driver func = %s line = %d\n", __func__, __LINE__);
		global_wac_i2c->wait_until_wake = false;
		global_wac_i2c->sleep_state = SLEEP_ON;
		mutex_unlock(&global_wac_i2c->system_lock);
		wakeup_report = 1;
	}
#endif
	return 0;
}

static int wacom_i2c_resume(struct device *c_dev)
{
	//struct i2c_client *client = to_i2c_client(dev);
	//enable_irq(client->irq);
#ifdef USE_IN_SUNXI
	if (global_wac_i2c->easy_wakeup_gesture) {
		mutex_lock(&global_wac_i2c->system_lock);
		global_wac_i2c->wait_until_wake = true;
		global_wac_i2c->sleep_state = SLEEP_OFF;
		pr_debug("wacom driver func = %s line = %d\n", __func__, __LINE__);
		mutex_unlock(&global_wac_i2c->system_lock);
		wake_up(&global_wac_i2c->wait_q);
	}
#endif
	return 0;
}

#ifdef USE_IN_SUNXI
#define EMR_NAME	"wacom_i2c"

/*
static int wacom_detect(struct i2c_client *client, struct i2c_board_info *info)
{
	struct i2c_adapter *adapter = client->adapter;
	int ret = -ENODEV;

	pr_debug("wacom_detect entect.\n");
	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return ret;

	if (twi_id == adapter->nr) {
		strlcpy(info->type, EMR_NAME, I2C_NAME_SIZE);
		pr_info("%s: addr= %x\n", __func__, client->addr);
		ret = 0;
	}
	return ret;
}
*/
static const unsigned short normal_i2c[] = {0x09, I2C_CLIENT_END};
#endif

#ifdef USE_IN_SUNXI
static UNIVERSAL_DEV_PM_OPS(wacom_i2c_pm, wacom_i2c_suspend, wacom_i2c_resume, NULL);
#else
static SIMPLE_DEV_PM_OPS(wacom_i2c_pm, wacom_i2c_suspend, wacom_i2c_resume);
#endif
static const struct of_device_id wacom_of_match[] = {
	{.compatible = "allwinner,wacom-i2c"},
	{},
};

static const struct i2c_device_id wacom_i2c_id[] = {
	{ EMR_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, wacom_i2c_id);

static struct i2c_driver wacom_i2c_driver = {
	.class = I2C_CLASS_HWMON,
	.driver	= {
//		.of_match_table = of_match_ptr(wacom_of_match),
		.of_match_table = wacom_of_match,
		.owner = THIS_MODULE,
		.name	= EMR_NAME,
		.pm	= &wacom_i2c_pm,
	},

	.probe		= wacom_i2c_probe,
	.remove		= wacom_i2c_remove,
	.id_table	= wacom_i2c_id,
#ifdef USE_IN_SUNXI
//	.detect	= wacom_detect,
	.address_list	= normal_i2c,
#endif
};

#ifndef USE_IN_SUNXI
module_i2c_driver(wacom_i2c_driver);
#else

static void emr_print_info(struct ctp_config_info info)
{
	pr_debug("info.emr_used:%d\n", info.ctp_used);
	pr_debug("info.emr_name:%s\n", info.name);
	pr_debug("info.twi_id:%d\n", info.twi_id);
	pr_debug("info.screen_max_x:%d\n", info.screen_max_x);
	pr_debug("info.screen_max_y:%d\n", info.screen_max_y);
	pr_debug("info.revert_x_flag:%d\n", info.revert_x_flag);
	pr_debug("info.revert_y_flag:%d\n", info.revert_y_flag);
	pr_debug("info.exchange_x_y_flag:%d\n", info.exchange_x_y_flag);
	pr_debug("info.irq_gpio_number:%d\n", info.irq_gpio.gpio);
	pr_debug("info.wakeup_gpio_number:%d\n", info.wakeup_gpio.gpio);
}

static int emr_wakeup(int status, int ms)
{
	pr_debug("%s: status:%d, ms = %d\n", __func__, status, ms);

	if (status == 0) {

		if (ms == 0) {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		} else {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
			msleep(ms);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
		}
	}

	if (status == 1) {
		if (ms == 0) {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		} else {
			__gpio_set_value(config_info.wakeup_gpio.gpio, 1);
			msleep(ms);
			__gpio_set_value(config_info.wakeup_gpio.gpio, 0);
		}
	}
	usleep_range(5000, 6000);

	return 0;
}

static int emr_get_system_config(void)
{
	emr_print_info(config_info);

	twi_id = config_info.twi_id;
	screen_max_x = config_info.screen_max_x;
	screen_max_y = config_info.screen_max_y;
	revert_x_flag = config_info.revert_x_flag;
	revert_y_flag = config_info.revert_y_flag;
	exchange_x_y_flag = config_info.exchange_x_y_flag;
	if ((screen_max_x == 0) || (screen_max_y == 0)) {
		pr_debug("%s:read config error!\n", __func__);
		return 0;
	}

	return 1;
}

static int __init wacom_emr_init(void)
{
	int ret = -1;

	pr_debug("*******************************************\n");
	if (input_sensor_startup(&(config_info.input_type))) {
		pr_debug("%s: emr_startup err.\n", __func__);
		return 0;
	} else {
		ret = input_sensor_init(&(config_info.input_type));
		if (ret != 0)
			pr_debug("%s:emr_ops.init err.\n", __func__);

	}

	if (config_info.ctp_used == 0) {
		pr_debug("*** emr_used set to 0 !\n");
		pr_debug("please put the sys_config.fex emr_used set to 1.\n");
		return 0;
	}

	if (!emr_get_system_config()) {
		pr_debug("%s:read config fail!\n", __func__);
		return ret;
	}

	input_set_power_enable(&(config_info.input_type), 1);
	msleep(20);
	emr_wakeup(1, 0);
	msleep(50);

	ret = i2c_add_driver(&wacom_i2c_driver);
	pr_debug("*i2c_add_driver = %d**********************************************\n", ret);
	return ret;
}

static void __exit wacom_emr_exit(void)
{
	pr_debug("==wacom_pen_exit==\n");
	i2c_del_driver(&wacom_i2c_driver);
	input_sensor_free(&(config_info.input_type));
}

module_init(wacom_emr_init);
module_exit(wacom_emr_exit);
#endif

MODULE_AUTHOR("Tatsunosuke Tobita <tobita.tatsunosuke@wacom.co.jp>");
MODULE_DESCRIPTION("WACOM EMR I2C Driver");
MODULE_LICENSE("GPL");

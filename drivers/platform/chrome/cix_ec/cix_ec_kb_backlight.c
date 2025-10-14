// SPDX-License-Identifier: GPL-2.0
/*
 * light sensor driver for the cix ec
 *
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#include <linux/module.h>
#include <linux/platform_data/cros_ec_commands.h>
#include <linux/platform_data/cros_ec_proto.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/of.h>

struct cix_ec_kb_data {
	struct cros_ec_device *ec;
	struct led_classdev led_dev;
};

static void keyboard_led_set_brightness(struct led_classdev *led_dev,
					enum led_brightness brightness)
{
	struct cix_ec_kb_data *cix_ec_kb_data;
	struct cros_ec_device *ec;
	struct {
		struct cros_ec_command msg;
		struct ec_params_pwm_set_keyboard_backlight params;
	} __packed buf;
	struct ec_params_pwm_set_keyboard_backlight *params = &buf.params;
	struct cros_ec_command *msg = &buf.msg;

	cix_ec_kb_data = container_of(led_dev, struct cix_ec_kb_data, led_dev);
	ec = cix_ec_kb_data->ec;
	if (brightness > led_dev->max_brightness) {
		printk("overlimit max_brightness");
		return;
	}

	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_PWM_SET_KEYBOARD_BACKLIGHT;
	msg->insize = 0;
	msg->outsize = sizeof(*params);
	params->percent = brightness;
	cros_ec_cmd_xfer_status(ec, msg);

	return;
}

static enum led_brightness
keyboard_led_get_brightness(struct led_classdev *led_dev)
{
	struct cix_ec_kb_data *cix_ec_kb_data;
	struct cros_ec_device *ec;
	struct {
		struct cros_ec_command msg;
		struct ec_response_pwm_get_keyboard_backlight resp;
	} __packed buf;
	struct ec_response_pwm_get_keyboard_backlight *resp = &buf.resp;
	struct cros_ec_command *msg = &buf.msg;

	cix_ec_kb_data = container_of(led_dev, struct cix_ec_kb_data, led_dev);
	ec = cix_ec_kb_data->ec;
	memset(&buf, 0, sizeof(buf));
	msg->version = 0;
	msg->command = EC_CMD_PWM_GET_KEYBOARD_BACKLIGHT;
	msg->insize = sizeof(*resp);
	msg->outsize = 0;
	cros_ec_cmd_xfer_status(ec, msg);

	return resp->percent;
}

static int cix_ec_kb_backlight_probe(struct platform_device *pdev)
{
	struct cros_ec_device *ec;
	struct cix_ec_kb_data *cix_ec_kb_data;
	int rc;
	struct led_classdev *led_dev;

	cix_ec_kb_data =
		devm_kzalloc(&pdev->dev, sizeof(*cix_ec_kb_data), GFP_KERNEL);
	if (!cix_ec_kb_data)
		return -ENOMEM;
	ec = dev_get_drvdata(pdev->dev.parent);
	platform_set_drvdata(pdev, cix_ec_kb_data);
	cix_ec_kb_data->ec = ec;

	led_dev = &(cix_ec_kb_data->led_dev);
	led_dev->name = "cix-ec-keyboard-backlight";
	led_dev->max_brightness = 255;
	led_dev->flags |= LED_CORE_SUSPENDRESUME;
	led_dev->brightness_set = keyboard_led_set_brightness;
	led_dev->brightness_get = keyboard_led_get_brightness;
	rc = devm_led_classdev_register(&pdev->dev, led_dev);
	if (rc < 0)
		return rc;

	return rc;
}

static int cix_ec_kb_backlight_remove(struct platform_device *pdev)
{
	struct cix_ec_kb_data *cix_ec_kb_data = platform_get_drvdata(pdev);
	struct cros_ec_device *ec;

	ec = cix_ec_kb_data->ec;
	devm_led_classdev_unregister(&pdev->dev, &cix_ec_kb_data->led_dev);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id cix_ec_kb_backlight_of_match[] = {
	{ .compatible = "cix,cix-ec-keyboard-backlight" },
	{},
};
MODULE_DEVICE_TABLE(of, cix_ec_kb_backlight_of_match);
#endif

static struct platform_driver cix_ec_kb_backlight_driver = {
	.probe = cix_ec_kb_backlight_probe,
	.remove = cix_ec_kb_backlight_remove,
	.driver = {
		.name = "cix-ec-keyboard-backlight",
		.of_match_table = of_match_ptr(cix_ec_kb_backlight_of_match),
	},
};
module_platform_driver(cix_ec_kb_backlight_driver);

MODULE_ALIAS("platform:cix-keyboard-backlight");
MODULE_DESCRIPTION("CIX EC keyboard backlight");
MODULE_LICENSE("GPL v2");
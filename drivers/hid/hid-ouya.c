/*
 *  HID driver for OUYA Game Controller(s)
 *
 *  Copyright (c) 2013 OUYA
 *  Copyright (c) 2013 Gregorios Leach <optikflux@gmail.com>
 *  Copyright (c) 2018 Lukas Rusak <lorusak@gmail.com>
 */

#include <linux/device.h>
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/module.h>

#include "hid-ids.h"

static const unsigned int ouya_absmap[] = {
	[0x30] = ABS_X,		/* left stick X */
	[0x31] = ABS_Y,		/* left stick Y */
	[0x32] = ABS_Z,		/* L2 */
	[0x33] = ABS_RX,	/* right stick X */
	[0x34] = ABS_RY,	/* right stick Y */
	[0x35] = ABS_RZ,	/* R2 */
};

static const unsigned int ouya_keymap[] = {
	[0x1] = BTN_SOUTH,	/* O */
	[0x2] = BTN_WEST,	/* U */
	[0x3] = BTN_NORTH,	/* Y */
	[0x4] = BTN_EAST,	/* A */
	[0x5] = BTN_TL,		/* L1 */
	[0x6] = BTN_TR,		/* R1 */
	[0x7] = BTN_THUMBL,	/* L3 */
	[0x8] = BTN_THUMBR,	/* R3 */
	[0x9] = BTN_DPAD_UP,	/* Up */
	[0xa] = BTN_DPAD_DOWN,	/* Down */
	[0xb] = BTN_DPAD_LEFT,	/* Left */
	[0xc] = BTN_DPAD_RIGHT, /* Right */
	[0xd] = BTN_TL2,	/* L2 */
	[0xe] = BTN_TR2,	/* R2 */
	[0xf] = BTN_MODE,	/* Power */
};

static int ouya_input_mapping(struct hid_device *hdev, struct hid_input *hi,
			       struct hid_field *field, struct hid_usage *usage,
			       unsigned long **bit, int *max)
{
	if ((usage->hid & HID_USAGE_PAGE) == HID_UP_BUTTON) {
		unsigned int key = usage->hid & HID_USAGE;

		if (key >= ARRAY_SIZE(ouya_keymap))
			return -1;

		key = ouya_keymap[key];
		hid_map_usage_clear(hi, usage, bit, max, EV_KEY, key);

		return 1;

	} else if ((usage->hid & HID_USAGE_PAGE) == HID_UP_GENDESK) {
		unsigned int abs = usage->hid & HID_USAGE;

		if (abs >= ARRAY_SIZE(ouya_absmap))
			return -1;

		abs = ouya_absmap[abs];
		hid_map_usage_clear(hi, usage, bit, max, EV_ABS, abs);

		return 1;
	}

	return 0;
}

static int ouya_probe(struct hid_device *hdev, const struct hid_device_id *id)
{
	int ret;

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err_free;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT | HID_CONNECT_HIDDEV_FORCE);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err_free;
	}

	return 0;

err_free:
	return ret;
}

static void ouya_remove(struct hid_device *hdev)
{
	hid_hw_stop(hdev);
	kfree(hid_get_drvdata(hdev));
}

static const struct hid_device_id ouya_devices[] = {
	{ HID_BLUETOOTH_DEVICE(USB_VENDOR_ID_OUYA, USB_DEVICE_ID_OUYA_CONTROLLER) },
	{ }
};
MODULE_DEVICE_TABLE(hid, ouya_devices);

static struct hid_driver ouya_driver = {
	.name = "ouya",
	.id_table = ouya_devices,
	.input_mapping = ouya_input_mapping,
	.probe = ouya_probe,
	.remove = ouya_remove,
};

static int __init ouya_init(void)
{
	return hid_register_driver(&ouya_driver);
}

static void __exit ouya_exit(void)
{
	hid_unregister_driver(&ouya_driver);
}

module_init(ouya_init);
module_exit(ouya_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lukas Rusak <lorusak@gmail.com>");
MODULE_AUTHOR("Gregorios Leach <optikflux@gmail.com>");
MODULE_DESCRIPTION("Ouya Controller Driver");

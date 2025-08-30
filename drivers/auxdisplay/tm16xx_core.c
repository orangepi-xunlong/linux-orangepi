// SPDX-License-Identifier: GPL-2.0
/*
 * TM16xx and compatible LED display/keypad controller driver
 * Supports TM16xx, FD6xx, PT6964, HBS658, AIP16xx and related chips.
 *
 * Copyright (C) 2025 Jean-François Lessard
 */

#include <linux/bitfield.h>
#include <linux/bitmap.h>
#include <linux/cleanup.h>
#include <linux/container_of.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/map_to_7segment.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/sysfs.h>
#include <linux/workqueue.h>

#include "line-display.h"
#include "tm16xx.h"

#define TM16XX_DIGIT_SEGMENTS	7

#define linedisp_to_tm16xx(display) \
	container_of(display, struct tm16xx_display, linedisp)

/**
 * struct tm16xx_led - Individual LED icon mapping
 * @cdev: LED class device for sysfs interface.
 * @hwgrid: Controller grid index of the LED.
 * @hwseg: Controller segment index of the LED.
 */
struct tm16xx_led {
	struct led_classdev cdev;
	u8 hwgrid;
	u8 hwseg;
};

/**
 * struct tm16xx_digit - 7-segment digit mapping and value
 * @hwgrids: Array mapping each 7-segment position to a grid on the controller.
 * @hwsegs: Array mapping each 7-segment position to a segment on the controller.
 * @value: Current character value displayed on this digit.
 */
struct tm16xx_digit {
	u8 hwgrids[TM16XX_DIGIT_SEGMENTS];
	u8 hwsegs[TM16XX_DIGIT_SEGMENTS];
};

/* state bitmap helpers */
/**
 * tm16xx_led_nbits() - Number of bits used for the display state bitmap
 * @display: pointer to tm16xx_display
 *
 * Return: total bits in the display state bitmap (grids * segments)
 */
static inline unsigned int tm16xx_led_nbits(const struct tm16xx_display *display)
{
	return display->num_hwgrid * display->num_hwseg;
}

/**
 * tm16xx_set_seg() - Set the display state for a specific grid/segment
 * @display: pointer to tm16xx_display
 * @hwgrid: grid index
 * @hwseg: segment index
 * @on: true to turn on, false to turn off
 */
static inline void tm16xx_set_seg(const struct tm16xx_display *display,
				  const u8 hwgrid, const u8 hwseg, const bool on)
{
	assign_bit(hwgrid * display->num_hwseg + hwseg, display->state, on);
}

/**
 * tm16xx_get_grid() - Get the current segment pattern for a grid
 * @display: pointer to tm16xx_display
 * @index: grid index
 *
 * Return: bit pattern of all segments for the given grid
 */
static inline unsigned int tm16xx_get_grid(const struct tm16xx_display *display,
					   const unsigned int index)
{
	return bitmap_read(display->state, index * display->num_hwseg,
			   display->num_hwseg);
}

/* main display */
/**
 * tm16xx_display_flush_init() - Workqueue to configure controller and set brightness
 * @work: pointer to work_struct
 */
static void tm16xx_display_flush_init(struct work_struct *work)
{
	struct tm16xx_display *display = container_of(work,
						      struct tm16xx_display,
						      flush_init);
	int ret;

	if (display->controller->init) {
		scoped_guard(mutex, &display->lock) {
			ret = display->controller->init(display);
			display->flush_status = ret;
		}
		if (ret)
			dev_err(display->dev,
				"Failed to configure controller: %d\n", ret);
	}
}

/**
 * tm16xx_display_flush_data() - Workqueue to update display data to controller
 * @work: pointer to work_struct
 */
static void tm16xx_display_flush_data(struct work_struct *work)
{
	struct tm16xx_display *display = container_of(work,
						      struct tm16xx_display,
						      flush_display);
	unsigned int grid, i;
	int ret = 0;

	scoped_guard(mutex, &display->lock) {
		if (display->controller->data) {
			for (i = 0; i < display->num_hwgrid; i++) {
				grid = tm16xx_get_grid(display, i);
				ret = display->controller->data(display, i, grid);
				if (ret) {
					dev_err(display->dev,
						"Failed to write display data: %d\n",
						ret);
					break;
				}
			}
		}

		display->flush_status = ret;
	}
}

/**
 * tm16xx_brightness_set() - Set display main LED brightness
 * @led_cdev: pointer to led_classdev
 * @brightness: new brightness value
 */
static void tm16xx_brightness_set(struct led_classdev *led_cdev,
				  enum led_brightness brightness)
{
	struct tm16xx_display *display = dev_get_drvdata(led_cdev->dev->parent);

	led_cdev->brightness = brightness;
	schedule_work(&display->flush_init);
}

/**
 * tm16xx_led_set() - Set state of an individual LED icon
 * @led_cdev: pointer to led_classdev
 * @value: new brightness (0/1)
 */
static void tm16xx_led_set(struct led_classdev *led_cdev,
			   enum led_brightness value)
{
	struct tm16xx_led *led = container_of(led_cdev, struct tm16xx_led, cdev);
	struct tm16xx_display *display = dev_get_drvdata(led_cdev->dev->parent);

	tm16xx_set_seg(display, led->hwgrid, led->hwseg, value);
	schedule_work(&display->flush_display);
}

static int tm16xx_display_value(struct tm16xx_display *display, const char *buf, size_t count)
{
	struct linedisp *linedisp = &display->linedisp;
	struct linedisp_map *map = linedisp->map;
	struct tm16xx_digit *digit;
	unsigned int i, j;
	int seg_pattern;
	bool val;

	for (i = 0; i < display->num_digits && i < count; i++) {
		digit = &display->digits[i];
		seg_pattern = map_to_seg7(&map->map.seg7, buf[i]);

		for (j = 0; j < TM16XX_DIGIT_SEGMENTS; j++) {
			val = seg_pattern & BIT(j);
			tm16xx_set_seg(display, digit->hwgrids[j], digit->hwsegs[j], val);
		}
	}

	for (; i < display->num_digits; i++) {
		digit = &display->digits[i];
		for (j = 0; j < TM16XX_DIGIT_SEGMENTS; j++)
			tm16xx_set_seg(display, digit->hwgrids[j], digit->hwsegs[j], 0);
	}

	schedule_work(&display->flush_display);
	return 0;
}

static int tm16xx_linedisp_get_map_type(struct linedisp *linedisp)
{
	return LINEDISP_MAP_SEG7;
}

static void tm16xx_linedisp_update(struct linedisp *linedisp)
{
	struct tm16xx_display *display = linedisp_to_tm16xx(linedisp);

	tm16xx_display_value(display, linedisp->buf, linedisp->num_chars);
}

static const struct linedisp_ops tm16xx_linedisp_ops = {
	.get_map_type = tm16xx_linedisp_get_map_type,
	.update = tm16xx_linedisp_update,
};

static int tm16xx_display_init(struct tm16xx_display *display)
{
	schedule_work(&display->flush_init);
	flush_work(&display->flush_init);
	if (display->flush_status)
		return display->flush_status;

	return 0;
}

static int tm16xx_parse_fwnode(struct device *dev, struct tm16xx_display *display)
{
	struct tm16xx_led *led;
	struct tm16xx_digit *digit;
	unsigned int max_hwgrid = 0, max_hwseg = 0;
	unsigned int i, j;
	int ret;
	u32 segments[TM16XX_DIGIT_SEGMENTS * 2];
	u32 reg[2];

	struct fwnode_handle *digits_node __free(fwnode_handle) =
		device_get_named_child_node(dev, "digits");
	struct fwnode_handle *leds_node __free(fwnode_handle) =
		device_get_named_child_node(dev, "leds");

	/* parse digits */
	if (digits_node) {
		display->num_digits = fwnode_get_child_node_count(digits_node);

		if (display->num_digits) {
			display->digits = devm_kcalloc(dev, display->num_digits,
						       sizeof(*display->digits),
						       GFP_KERNEL);
			if (!display->digits)
				return -ENOMEM;

			i = 0;
			fwnode_for_each_available_child_node_scoped(digits_node, child) {
				digit = &display->digits[i];

				ret = fwnode_property_read_u32(child, "reg", reg);
				if (ret)
					return ret;

				ret = fwnode_property_read_u32_array(child,
								     "segments", segments,
								     TM16XX_DIGIT_SEGMENTS * 2);
				if (ret < 0)
					return ret;

				for (j = 0; j < TM16XX_DIGIT_SEGMENTS; ++j) {
					digit->hwgrids[j] = segments[2 * j];
					digit->hwsegs[j] = segments[2 * j + 1];
					max_hwgrid = umax(max_hwgrid, digit->hwgrids[j]);
					max_hwseg = umax(max_hwseg, digit->hwsegs[j]);
				}
				i++;
			}
		}
	}

	/* parse leds */
	if (leds_node) {
		display->num_leds = fwnode_get_child_node_count(leds_node);

		if (display->num_leds) {
			display->leds = devm_kcalloc(dev, display->num_leds,
						     sizeof(*display->leds),
						     GFP_KERNEL);
			if (!display->leds)
				return -ENOMEM;

			i = 0;
			fwnode_for_each_available_child_node_scoped(leds_node, child) {
				led = &display->leds[i];
				ret = fwnode_property_read_u32_array(child, "reg", reg, 2);
				if (ret < 0)
					return ret;

				led->hwgrid = reg[0];
				led->hwseg = reg[1];
				max_hwgrid = umax(max_hwgrid, led->hwgrid);
				max_hwseg = umax(max_hwseg, led->hwseg);
				i++;
			}
		}
	}

	if (max_hwgrid >= display->controller->max_grids) {
		dev_err(dev, "grid %u exceeds controller max_grids %u\n",
			max_hwgrid, display->controller->max_grids);
		return -EINVAL;
	}

	if (max_hwseg >= display->controller->max_segments) {
		dev_err(dev, "segment %u exceeds controller max_segments %u\n",
			max_hwseg, display->controller->max_segments);
		return -EINVAL;
	}

	display->num_hwgrid = max_hwgrid + 1;
	display->num_hwseg = max_hwseg + 1;

	return 0;
}

/**
 * tm16xx_probe() - Probe and initialize display device, register LEDs
 * @display: pointer to tm16xx_display
 *
 * Return: 0 on success, negative error code on failure
 */
int tm16xx_probe(struct tm16xx_display *display)
{
	struct device *dev = display->dev;
	struct led_classdev *main = &display->main_led;
	struct led_init_data led_init = {0};
	struct fwnode_handle *leds_node;
	struct tm16xx_led *led;
	unsigned int nbits, i;
	int ret;

	ret = tm16xx_parse_fwnode(dev, display);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to parse device tree\n");

	nbits = tm16xx_led_nbits(display);
	display->state = devm_bitmap_zalloc(dev, nbits, GFP_KERNEL);
	if (!display->state)
		return -ENOMEM;

	ret = devm_mutex_init(display->dev, &display->lock);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to initialize mutex\n");

	INIT_WORK(&display->flush_init, tm16xx_display_flush_init);
	INIT_WORK(&display->flush_display, tm16xx_display_flush_data);

	/* Initialize main LED properties */
	led_init.fwnode = dev_fwnode(dev); /* apply label property */
	main->max_brightness = display->controller->max_brightness;
	device_property_read_u32(dev, "max-brightness", &main->max_brightness);
	main->max_brightness = umin(main->max_brightness,
				    display->controller->max_brightness);

	main->brightness = main->max_brightness;
	device_property_read_u32(dev, "default-brightness", &main->brightness);
	main->brightness = umin(main->brightness, main->max_brightness);

	main->brightness_set = tm16xx_brightness_set;
	main->flags = LED_RETAIN_AT_SHUTDOWN | LED_CORE_SUSPENDRESUME;

	/* Register individual LEDs from device tree */
	ret = led_classdev_register_ext(dev, main, &led_init);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to register main LED\n");

	i = 0;
	led_init.devicename = dev_name(main->dev);
	led_init.devname_mandatory = true;
	led_init.default_label = "led";
	leds_node = device_get_named_child_node(dev, "leds");
	fwnode_for_each_available_child_node_scoped(leds_node, child) {
		led_init.fwnode = child;
		led = &display->leds[i];
		led->cdev.max_brightness = 1;
		led->cdev.brightness_set = tm16xx_led_set;
		led->cdev.flags = LED_RETAIN_AT_SHUTDOWN | LED_CORE_SUSPENDRESUME;

		ret = led_classdev_register_ext(dev, &led->cdev, &led_init);
		if (ret) {
			dev_err_probe(dev, ret, "Failed to register LED %s\n",
				      led->cdev.name);
			goto unregister_leds;
		}

		i++;
	}

	ret = tm16xx_display_init(display);
	if (ret) {
		dev_err_probe(dev, ret, "Failed to initialize display\n");
		goto unregister_leds;
	}

	ret = linedisp_attach(&display->linedisp, display->main_led.dev,
			      display->num_digits, &tm16xx_linedisp_ops);
	if (ret) {
		dev_err_probe(dev, ret, "Failed to initialize line-display\n");
		goto unregister_leds;
	}

	ret = tm16xx_keypad_probe(display);
	if (ret)
		dev_warn(dev, "Failed to initialize keypad: %d\n", ret);

	return 0;

unregister_leds:
	while (i--)
		led_classdev_unregister(&display->leds[i].cdev);

	led_classdev_unregister(main);
	return ret;
}
EXPORT_SYMBOL_NS(tm16xx_probe, "TM16XX");

/**
 * tm16xx_remove() - Remove display, unregister LEDs, blank output
 * @display: pointer to tm16xx_display
 */
void tm16xx_remove(struct tm16xx_display *display)
{
	unsigned int nbits = tm16xx_led_nbits(display);
	struct tm16xx_led *led;

	linedisp_detach(display->main_led.dev);

	/*
	 * Unregister LEDs first to immediately stop trigger activity.
	 * This prevents LED triggers from attempting to access hardware
	 * after it's been disconnected or driver unloaded.
	 */
	for (int i = 0; i < display->num_leds; i++) {
		led = &display->leds[i];
		led_classdev_unregister(&led->cdev);
	}
	led_classdev_unregister(&display->main_led);

	/* Clear display state */
	bitmap_zero(display->state, nbits);
	schedule_work(&display->flush_display);
	flush_work(&display->flush_display);

	/* Turn off display */
	display->main_led.brightness = LED_OFF;
	schedule_work(&display->flush_init);
	flush_work(&display->flush_init);
}
EXPORT_SYMBOL_NS(tm16xx_remove, "TM16XX");

MODULE_AUTHOR("Jean-François Lessard");
MODULE_DESCRIPTION("TM16xx LED Display Controllers");
MODULE_LICENSE("GPL");
MODULE_IMPORT_NS("LINEDISP");

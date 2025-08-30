// SPDX-License-Identifier: GPL-2.0
/*
 * TM16xx and compatible LED display/keypad controller driver
 * Supports TM16xx, FD6xx, PT6964, HBS658, AIP16xx and related chips.
 *
 * Copyright (C) 2025 Jean-François Lessard
 */

#include <linux/bitmap.h>
#include <linux/cleanup.h>
#include <linux/device.h>
#include <linux/input.h>
#include <linux/input/matrix_keypad.h>
#include <linux/property.h>

#include "tm16xx.h"

/**
 * struct tm16xx_keypad - Keypad matrix state and input device
 * @input: Input device for reporting key events.
 * @state: Current bitmap of key states.
 * @last_state: Previous bitmap of key states for change detection.
 * @changes: Bitmap of key state changes since last poll.
 * @row_shift: Row shift for keymap encoding.
 */
struct tm16xx_keypad {
	struct input_dev *input;
	unsigned long *state;
	unsigned long *last_state;
	unsigned long *changes;
	int row_shift;
};

/**
 * tm16xx_key_nbits() - Number of bits for the keypad state bitmap
 * @display: pointer to tm16xx_display
 *
 * Return: total bits in keypad state bitmap (max_key_rows * max_key_cols)
 */
static inline unsigned int tm16xx_key_nbits(const struct tm16xx_display *display)
{
	return display->controller->max_key_rows *
	       display->controller->max_key_cols;
}

/**
 * tm16xx_get_key_row() - Get row index from keypad bit index
 * @display: pointer to tm16xx_display
 * @bit: bit index in state bitmap
 *
 * Return: row index
 */
static inline int tm16xx_get_key_row(const struct tm16xx_display *display,
				     const unsigned int bit)
{
	return bit / display->controller->max_key_cols;
}

/**
 * tm16xx_get_key_col() - Get column index from keypad bit index
 * @display: pointer to tm16xx_display
 * @bit: bit index in state bitmap
 *
 * Return: column index
 */
static inline int tm16xx_get_key_col(const struct tm16xx_display *display,
				     const unsigned int bit)
{
	return bit % display->controller->max_key_cols;
}

/**
 * tm16xx_set_key() - Set the keypad state for a key
 * @display: pointer to tm16xx_display
 * @row: row index
 * @col: column index
 * @pressed: true if pressed, false otherwise
 */
void tm16xx_set_key(const struct tm16xx_display *display, const int row,
		    const int col, const bool pressed)
{
	__assign_bit(row * display->controller->max_key_cols + col,
		     display->keypad->state, pressed);
}
EXPORT_SYMBOL_NS(tm16xx_set_key, "TM16XX");

/**
 * tm16xx_keypad_poll() - Polls the keypad, reports events
 * @input: pointer to input_dev
 *
 * Reads the matrix keypad state, compares with previous state, and
 * reports key events to the input subsystem.
 */
static void tm16xx_keypad_poll(struct input_dev *input)
{
	struct tm16xx_display *display = input_get_drvdata(input);
	struct tm16xx_keypad *keypad = display->keypad;
	const unsigned short *keycodes = keypad->input->keycode;
	unsigned int nbits = tm16xx_key_nbits(display);
	unsigned int bit;
	int row, col, scancode;
	bool pressed;
	int ret;

	bitmap_zero(keypad->state, nbits);
	bitmap_zero(keypad->changes, nbits);

	scoped_guard(mutex, &display->lock) {
		ret = display->controller->keys(display);
	}

	if (ret) {
		dev_err(display->dev, "Reading failed: %d\n", ret);
		return;
	}

	bitmap_xor(keypad->changes, keypad->state, keypad->last_state, nbits);

	for_each_set_bit(bit, keypad->changes, nbits) {
		row = tm16xx_get_key_row(display, bit);
		col = tm16xx_get_key_col(display, bit);
		pressed = _test_bit(bit, keypad->state);
		scancode = MATRIX_SCAN_CODE(row, col, keypad->row_shift);

		input_event(keypad->input, EV_MSC, MSC_SCAN, scancode);
		input_report_key(keypad->input, keycodes[scancode], pressed);
	}
	input_sync(keypad->input);

	bitmap_copy(keypad->last_state, keypad->state, nbits);
}

/**
 * tm16xx_keypad_probe() - Initialize keypad/input device
 * @display: pointer to tm16xx_display
 *
 * Return: 0 on success, negative error code on failure
 */
int tm16xx_keypad_probe(struct tm16xx_display *display)
{
	const unsigned int rows = display->controller->max_key_rows;
	const unsigned int cols = display->controller->max_key_cols;
	struct tm16xx_keypad *keypad;
	struct input_dev *input;
	unsigned int poll_interval, nbits;
	int ret;

	if (!display->controller->keys || !rows || !cols)
		return 0; /* keypad not supported */

	if (!device_property_present(display->dev, "poll-interval") ||
	    !device_property_present(display->dev, "linux,keymap"))
		return 0; /* keypad disabled */

	ret = device_property_read_u32(display->dev, "poll-interval", &poll_interval);
	if (ret)
		return dev_err_probe(display->dev, ret,
				     "Failed to read poll-interval\n");

	keypad = devm_kzalloc(display->dev, sizeof(*keypad), GFP_KERNEL);
	if (!keypad)
		return -ENOMEM;
	display->keypad = keypad;

	nbits = tm16xx_key_nbits(display);
	keypad->state = devm_bitmap_zalloc(display->dev, nbits, GFP_KERNEL);
	keypad->last_state = devm_bitmap_zalloc(display->dev, nbits, GFP_KERNEL);
	keypad->changes = devm_bitmap_zalloc(display->dev, nbits, GFP_KERNEL);
	if (!keypad->state || !keypad->last_state || !keypad->changes)
		return -ENOMEM;

	input = devm_input_allocate_device(display->dev);
	if (!input)
		return -ENOMEM;
	input->name = "tm16xx-keypad";
	keypad->input = input;
	input_set_drvdata(input, display);

	keypad->row_shift = get_count_order(cols); /* !cols already checked */
	ret = matrix_keypad_build_keymap(NULL, "linux,keymap", rows, cols, NULL, input);
	if (ret)
		return dev_err_probe(display->dev, ret,
				     "Failed to build keymap\n");

	if (device_property_read_bool(display->dev, "autorepeat"))
		__set_bit(EV_REP, input->evbit);

	input_setup_polling(input, tm16xx_keypad_poll);
	input_set_poll_interval(input, poll_interval);
	ret = input_register_device(input);
	if (ret)
		return dev_err_probe(display->dev, ret,
				     "Failed to register input device\n");

	return 0;
}

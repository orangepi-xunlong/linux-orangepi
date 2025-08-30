/* SPDX-License-Identifier: GPL-2.0 */
/*
 * TM16xx and compatible LED display/keypad controller driver
 * Supports TM16xx, FD6xx, PT6964, HBS658, AIP16xx and related chips.
 *
 * Copyright (C) 2025 Jean-François Lessard
 */

#ifndef _TM16XX_H
#define _TM16XX_H

#include <linux/bitfield.h>
#include <linux/leds.h>
#include <linux/workqueue.h>

#include "line-display.h"

/* Common bit field definitions */

/* Command type bits (bits 7-6) */
#define TM16XX_CMD_MASK		GENMASK(7, 6)
#define TM16XX_CMD_MODE		(0 << 6)
#define TM16XX_CMD_DATA		(1 << 6)
#define TM16XX_CMD_CTRL		(2 << 6)
#define TM16XX_CMD_ADDR		(3 << 6)
#define TM16XX_CMD_WRITE	(TM16XX_CMD_DATA | TM16XX_DATA_MODE_WRITE)
#define TM16XX_CMD_READ		(TM16XX_CMD_DATA | TM16XX_DATA_MODE_READ)

/* Mode command grid settings (bits 1-0) */
#define TM16XX_MODE_GRID_MASK	GENMASK(1, 0)
#define TM16XX_MODE_4GRIDS	(0 << 0)
#define TM16XX_MODE_5GRIDS	(1 << 0)
#define TM16XX_MODE_6GRIDS	(2 << 0)
#define TM16XX_MODE_7GRIDS	(3 << 0)

/* Data command settings */
#define TM16XX_DATA_ADDR_MASK	BIT(2)
#define TM16XX_DATA_ADDR_AUTO	(0 << 2)
#define TM16XX_DATA_ADDR_FIXED	(1 << 2)
#define TM16XX_DATA_MODE_MASK	GENMASK(1, 0)
#define TM16XX_DATA_MODE_WRITE	(0 << 0)
#define TM16XX_DATA_MODE_READ	(2 << 0)

/* Control command settings */
#define TM16XX_CTRL_BR_MASK	GENMASK(2, 0)
#define TM16XX_CTRL_ON		(1 << 3)

/* TM1618 specific constants */
#define TM1618_BYTE1_MASK	GENMASK(4, 0)
#define TM1618_BYTE2_MASK	GENMASK(7, 5)
#define TM1618_BYTE2_SHIFT	3
#define TM1618_KEY_READ_LEN	3
#define TM1618_KEY_MASK		(BIT(4) | BIT(1))

/* TM1628 specific constants */
#define TM1628_BYTE1_MASK	GENMASK(7, 0)
#define TM1628_BYTE2_MASK	GENMASK(13, 8)
#define TM1628_KEY_READ_LEN	5
#define TM1628_KEY_MASK		(GENMASK(4, 3) | GENMASK(1, 0))

/* TM1638 specific constants */
#define TM1638_KEY_READ_LEN	4
#define TM1638_KEY_MASK		(GENMASK(6, 4) | GENMASK(2, 0))

/* FD620 specific constants */
#define FD620_BYTE1_MASK	GENMASK(6, 0)

#define FD620_BYTE2_MASK	BIT(7)
#define FD620_BYTE2_SHIFT	5
#define FD620_KEY_READ_LEN	4
#define FD620_KEY_MASK		(BIT(3) | BIT(0))

/* I2C controller addresses and control settings */
#define TM1650_CMD_CTRL		0x48
#define TM1650_CMD_READ		0x4F
#define TM1650_CMD_ADDR		0x68
#define TM1650_CTRL_BR_MASK	GENMASK(6, 4)
#define TM1650_CTRL_ON		(1 << 0)
#define TM1650_CTRL_SLEEP	(1 << 2)
#define TM1650_CTRL_SEG_MASK	BIT(3)
#define TM1650_CTRL_SEG8_MODE	(0 << 3)
#define TM1650_CTRL_SEG7_MODE	(1 << 3)
#define TM1650_KEY_ROW_MASK	GENMASK(1, 0)
#define TM1650_KEY_COL_MASK	GENMASK(5, 3)
#define TM1650_KEY_DOWN_MASK	BIT(6)
#define TM1650_KEY_COMBINED	GENMASK(5, 3)

#define FD655_CMD_CTRL		0x48
#define FD655_CMD_ADDR		0x66
#define FD655_CTRL_BR_MASK	GENMASK(6, 5)
#define FD655_CTRL_ON		(1 << 0)

#define FD6551_CMD_CTRL		0x48
#define FD6551_CTRL_BR_MASK	GENMASK(3, 1)
#define FD6551_CTRL_ON		(1 << 0)

#define HBS658_KEY_COL_MASK	GENMASK(7, 5)

#define TM16XX_CTRL_BRIGHTNESS(on, val, prefix) \
	((on) ? (FIELD_PREP(prefix##_CTRL_BR_MASK, (val)) | prefix##_CTRL_ON) : 0)

/* Forward declarations */
struct tm16xx_display;
struct tm16xx_digit;
struct tm16xx_led;
struct tm16xx_keypad;

/**
 * DOC: struct tm16xx_controller - Controller-specific operations and limits
 * @max_grids: Maximum number of grids supported by the controller.
 * @max_segments: Maximum number of segments supported by the controller.
 * @max_brightness: Maximum brightness level supported by the controller.
 * @max_key_rows: Maximum number of key input rows supported by the controller.
 * @max_key_cols: Maximum number of key input columns supported by the controller.
 * @init: Pointer to controller mode/brightness configuration function.
 * @data: Pointer to function writing display data to the controller.
 * @keys: Pointer to function reading controller key state into bitmap.
 *
 * Holds function pointers and limits for controller-specific operations.
 */
struct tm16xx_controller {
	const u8 max_grids;
	const u8 max_segments;
	const u8 max_brightness;
	const u8 max_key_rows;
	const u8 max_key_cols;
	int (*const init)(struct tm16xx_display *display);
	int (*const data)(struct tm16xx_display *display, u8 index, unsigned int grid);
	int (*const keys)(struct tm16xx_display *display);
};

/**
 * struct tm16xx_display - Main driver structure for the display
 * @dev: Pointer to device struct.
 * @controller: Controller-specific function table and limits.
 * @linedisp: character line display structure
 * @keypad: Opaque pointer to tm16xx_keypad struct.
 * @spi_buffer: DMA-safe buffer for SPI transactions, or NULL for I2C.
 * @num_hwgrid: Number of controller grids in use.
 * @num_hwseg: Number of controller segments in use.
 * @main_led: LED class device for the entire display.
 * @leds: Array of individual LED icon structures.
 * @num_leds: Number of individual LED icons.
 * @digits: Array of 7-segment digit structures.
 * @num_digits: Number of 7-segment digits.
 * @flush_init: Work struct for configuration update.
 * @flush_display: Work struct for display update.
 * @flush_status: Status/result of last flush work.
 * @lock: Mutex protecting concurrent access to work operations.
 * @state: Bitmap holding current raw display state.
 */
struct tm16xx_display {
	struct device *dev;
	const struct tm16xx_controller *controller;
	struct tm16xx_keypad *keypad;
	struct linedisp linedisp;
	u8 *spi_buffer;
	u8 num_hwgrid;
	u8 num_hwseg;
	struct led_classdev main_led;
	struct tm16xx_led *leds;
	u8 num_leds;
	struct tm16xx_digit *digits;
	u8 num_digits;
	struct work_struct flush_init;
	struct work_struct flush_display;
	int flush_status;
	struct mutex lock; /* prevents concurrent work operations */
	unsigned long *state;
};

int tm16xx_probe(struct tm16xx_display *display);
void tm16xx_remove(struct tm16xx_display *display);

/* keypad support */
#if IS_ENABLED(CONFIG_TM16XX_KEYPAD)
int tm16xx_keypad_probe(struct tm16xx_display *display);
void tm16xx_set_key(const struct tm16xx_display *display, const int row,
		    const int col, const bool pressed);
#else
static inline int tm16xx_keypad_probe(struct tm16xx_display *display)
{
	return 0;
}

static inline void tm16xx_set_key(const struct tm16xx_display *display,
				  const int row, const int col,
				  const bool pressed)
{
}
#endif

#define tm16xx_for_each_key(display, _r, _c) \
	for (int _r = 0; _r < (display)->controller->max_key_rows; _r++) \
		for (int _c = 0; _c < (display)->controller->max_key_cols; _c++)

#endif /* _TM16XX_H */

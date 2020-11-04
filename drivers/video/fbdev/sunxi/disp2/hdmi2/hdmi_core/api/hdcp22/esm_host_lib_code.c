/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include "include/ESMHost.h"

uint32_t elliptic_byte_get(uint8_t *image, uint32_t const value_size)
{
	uint32_t i;
	uint32_t value;

	value = 0;

	for (i = 0; i < value_size; i++) {
		value <<= 8;
		value += (0xff & image[i]);
	}

	return value;
}

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include "dw_access.h"

static struct device_access *device_bsp;

/* calculate valid bit account */
static u8 lowestBitSet(u8 x)
{
	u8 result = 0;

	/* x=0 is not properly handled by while-loop */
	if (x == 0)
		return 0;

	while ((x & 1) == 0) {
		x >>= 1;
		result++;
	}

	return result;
}

u16 concat_bits(u8 bHi, u8 oHi, u8 nHi, u8 bLo, u8 oLo, u8 nLo)
{
	return (bit_field(bHi, oHi, nHi) << nLo) | bit_field(bLo, oLo, nLo);
}

u16 byte_to_word(const u8 hi, const u8 lo)
{
	return concat_bits(hi, 0, 8, lo, 0, 8);
}

u8 bit_field(const u16 data, u8 shift, u8 width)
{
	return (data >> shift) & ((((u16) 1) << width) - 1);
}

u32 byte_to_dword(u8 b3, u8 b2, u8 b1, u8 b0)
{
	u32 retval = 0;

	retval |= b0 << (0 * 8);
	retval |= b1 << (1 * 8);
	retval |= b2 << (2 * 8);
	retval |= b3 << (3 * 8);
	return retval;
}

/**
 * Find first (least significant) bit set
 * @param[in] data word to search
 * @return bit position or 32 if none is set
 */
static unsigned first_bit_set(uint32_t data)
{
	unsigned n = 0;

	if (data != 0) {
		for (n = 0; (data & 1) == 0; n++)
			data >>= 1;
	}
	return n;
}

/**
 * Set bit field
 * @param[in] data raw data
 * @param[in] mask bit field mask
 * @param[in] value new value
 * @return new raw data
 */
uint32_t set(uint32_t data, uint32_t mask, uint32_t value)
{
	return ((value << first_bit_set(mask)) & mask) | (data & ~mask);
}

u32 dev_read(dw_hdmi_dev_t *dev, u32 addr)
{
	if (dev && device_bsp && device_bsp->read)
		return device_bsp->read((uintptr_t)addr);
	return 0;
}

void dev_write(dw_hdmi_dev_t *dev, u32 addr, u32 data)
{
	if (dev && device_bsp && device_bsp->write)
		return device_bsp->write((uintptr_t)addr, data);
}

u32 dev_read_mask(dw_hdmi_dev_t *dev, u32 addr, u8 mask)
{
	u8 shift = lowestBitSet(mask);

	return (dev_read(dev, addr) & mask) >> shift;
}

void dev_write_mask(dw_hdmi_dev_t *dev, u32 addr, u8 mask, u8 data)
{
	u8 temp = 0;
	u8 shift = lowestBitSet(mask);

	temp = dev_read(dev, addr);
	temp &= ~(mask);
	temp |= (mask & (data << shift));
	dev_write(dev, addr, temp);
}

void dw_access_init(struct device_access *device)
{
	LOG_TRACE();
	device_bsp = device;
}

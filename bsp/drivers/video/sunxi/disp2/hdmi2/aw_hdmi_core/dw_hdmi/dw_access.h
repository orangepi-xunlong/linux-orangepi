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

#ifndef _DW_ACCESS_H
#define _DW_ACCESS_H

#include "dw_dev.h"

struct device_access {
	char name[30];
	int (*initialize)(void);
	int (*disable)(void);
	void (*write)(uintptr_t addr, u32 data);
	u32  (*read)(uintptr_t addr);
};

/**
 * Concatenate two parts of two 8-bit bytes into a new 16-bit word
 * @param bHi first byte
 * @param oHi shift part of first byte (to be place as most significant
 * bits)
 * @param nHi width part of first byte (to be place as most significant
 * bits)
 * @param bLo second byte
 * @param oLo shift part of second byte (to be place as least
 * significant bits)
 * @param nLo width part of second byte (to be place as least
 * significant bits)
 * @returns 16-bit concatenated word as part of the first byte and part of
 * the second byte
 */
u16 concat_bits(u8 bHi, u8 oHi, u8 nHi, u8 bLo, u8 oLo, u8 nLo);

/** Concatenate two full bytes into a new 16-bit word
 * @param hi
 * @param lo
 * @returns hi as most significant bytes concatenated with lo as least
 * significant bits.
 */
u16 byte_to_word(const u8 hi, const u8 lo);

/** Extract the content of a certain part of a byte
 * @param data 8bit byte
 * @param shift shift from the start of the bit (0)
 * @param width width of the desired part starting from shift
 * @returns an 8bit byte holding only the desired part of the passed on
 * data byte
 */
u8 bit_field(const u16 data, u8 shift, u8 width);

/** Concatenate four 8-bit bytes into a new 32-bit word
 * @param b3 assumed as most significant 8-bit byte
 * @param b2
 * @param b1
 * @param b0 assumed as least significant 8bit byte
 * @returns a 2D word, 32bits, composed of the 4 passed on parameters
 */
u32 byte_to_dword(u8 b3, u8 b2, u8 b1, u8 b0);

/**
 * Set bit field
 * @param[in] data raw data
 * @param[in] mask bit field mask
 * @param[in] value new value
 * @return new raw data
 */
uint32_t set(uint32_t data, uint32_t mask, uint32_t value);

void dw_access_init(struct device_access *device);

/**
 *Read the contents of a register
 *@param addr of the register
 *@return 8bit byte containing the contents
 */
u32 dev_read(dw_hdmi_dev_t *dev, u32 addr);

/**
 *Read several bits from a register
 *@param addr of the register
 *@param shift of the bit from the beginning
 *@param width or number of bits to read
 *@return the contents of the specified bits
 */
u32 dev_read_mask(dw_hdmi_dev_t *dev, u32 addr, u8 mask);

/**
 *Write a byte to a register
 *@param data to be written to the register
 *@param addr of the register
 */
void dev_write(dw_hdmi_dev_t *dev, u32 addr, u32 data);

/**
 *Write to several bits in a register
 *
 *@param data to be written to the required part
 *@param addr of the register
 *@param shift of the bits from the beginning
 *@param width or number of bits to written to
 */
void dev_write_mask(dw_hdmi_dev_t *dev, u32 addr, u8 mask, u8 data);

#endif				/* _DW_ACCESS_H */

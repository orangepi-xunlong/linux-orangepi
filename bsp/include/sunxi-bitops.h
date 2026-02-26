/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner's Bit Operation Helper functions
 *
 * Copyright (c) 2023, Martin <wuyan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#ifndef __SUNXI_BITOPS_H_
#define __SUNXI_BITOPS_H_

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/iopoll.h>
#include <linux/bits.h>

/* See also BIT(x) in include/vdso/bits.h, and include/linux/bits.h */
/* See also include/asm-generic/bitops/instrumented-atomic.h */
#define BITS_WIDTH(bit_start, bit_width)	GENMASK((bit_start) + (bit_width) - 1, bit_start)
#define BITS(bit_start, bit_end)		GENMASK(bit_end, bit_start)

static __always_inline bool verify(u32 val, volatile void __iomem *addr)
{
#if IS_ENABLED(CONFIG_AW_REG_VERIFY)
	u32 valr = readl(addr);
	if (valr != val) {
		pr_err("verify(): Failed @0x%p: Expect 0x%x but got 0x%x\n",
			addr, val, valr);
		return false;
	}
	return true;
#endif
	return true;
}

static __always_inline bool verify_bits(u32 val, volatile void __iomem *addr, u32 bits)
{
#if IS_ENABLED(CONFIG_AW_REG_VERIFY)
	u32 valr = readl(addr);
	if ((valr & bits) != (val & bits)) {
		pr_err("verify_bits(): Failed @0x%p: Expect 0x%x but got 0x%x on bits 0x%x\n",
			addr, val & bits, valr & bits, bits);
		return false;
	}
	return true;
#endif
	return true;
}

static __always_inline void writel_verify(u32 val, volatile void __iomem *addr)
{
	writel(val, addr);
	verify(val, addr);
}

static __always_inline u32 get_bits(const void volatile __iomem *addr, u32 bits)
{
	return readl(addr) & bits;
}

static __always_inline void set_bits(volatile void  __iomem *addr, u32 bits)
{
	writel(readl(addr) | bits, addr);
	verify_bits(bits, addr, bits);
}

static __always_inline void clear_bits(volatile void  __iomem *addr, u32 bits)
{
	writel(readl(addr) & ~bits, addr);
	verify_bits(0, addr, bits);
}

static __always_inline void change_bits(volatile void  __iomem *addr, u32 bits)
{
	writel(readl(addr) ^ bits, addr);
	verify_bits(~bits, addr, bits);
}

static __always_inline bool test_bits_set(const void volatile __iomem *addr, u32 bits)
{
	return (get_bits(addr, bits) & bits) == bits;
}

static __always_inline bool test_bits_cleared(const void volatile __iomem *addr, u32 bits)
{
	return (get_bits(addr, bits) & bits) == 0;
}

static __always_inline bool test_bits_equ(const void volatile __iomem *addr, u32 bits, u32 val)
{
	return get_bits(addr, bits) == val;
}

static __always_inline int wait_bits_set(const volatile void  __iomem *addr, u32 bits, unsigned int timeout_us)
{
	u32 val;
	int err = readl_poll_timeout_atomic(addr, val, ((val & bits) == bits), 0, timeout_us);
	if (err)
		pr_err("wait_bits_set(): Timeout. addr=0x%p, bits=0x%x\n", addr, bits);
	return err;
}

/* Wait for @bits in @addr are cleared with @ms_timeout */
static __always_inline int wait_bits_cleared(const volatile void  __iomem *addr, u32 bits, unsigned int timeout_us)
{
	u32 val;
	int err = readl_poll_timeout_atomic(addr, val, ((val & bits) == 0), 0, timeout_us);
	if (err)
		pr_err("wait_bits_cleared(): Timeout. addr=0x%p, bits=0x%x\n", addr, bits);
	return err;
}

static __always_inline int wait_bits_equ(const volatile void  __iomem *addr, u32 bits, u32 val, unsigned int timeout_us)
{
	u32 read_val;
	int err = readl_poll_timeout_atomic(addr, read_val, ((read_val & bits) == val), 0, timeout_us);
	if (err)
		pr_err("wait_bits_equ): Timeout. addr=0x%p, bits=0x%x\n", addr, bits);
	return err;
}

static __always_inline int wait_bits_set_maysleep(const volatile void  __iomem *addr, u32 bits, unsigned long sleep_us, unsigned int timeout_us)
{
	u32 val;
	int err = readl_poll_timeout(addr, val, ((val & bits) == bits), sleep_us, timeout_us);
	if (err)
		pr_err("wait_bits_set_maysleep(): Timeout. addr=0x%p, bits=0x%x\n", addr, bits);
	return err;
}

static __always_inline int wait_bits_cleared_maysleep(const volatile void  __iomem *addr, u32 bits, unsigned long sleep_us, unsigned int timeout_us)
{
	u32 val;
	int err = readl_poll_timeout(addr, val, ((val & bits) == 0), sleep_us, timeout_us);
	if (err)
		pr_err("wait_bits_cleared_maysleep(): Timeout. addr=0x%p, bits=0x%x\n", addr, bits);
	return err;
}

/* From linux-6.1/sound/pci/ctxfi/cthardware.c */
/**
 * @bits: bit mask, could be discontiguous, such as 0xf0f0
 */
static __always_inline u32 get_data_field(u32 data, u32 bits)
{
	int i;

	if (WARN_ON(!bits))
		return 0;
	/* @bits should always be greater than 0 */
	for (i = 0; !(bits & (1 << i)); )
		i++;

	return (data & bits) >> i;
}

static __always_inline u32 get_field(const void volatile __iomem *addr, u32 bits)
{
	return get_data_field(readl(addr), bits);
}

/**
 * @bits: bit mask, could be discontiguous, such as 0xf0f0
 * @val_on_bits: value on the @bits. NOT the value on the whole @addr
 */
static __always_inline void set_field(void volatile __iomem *addr, u32 bits, u32 val_on_bits)
{
	int i;
	u32 tmp;

	if (WARN_ON(!bits))
		return;
	/* @bits should always be greater than 0 */
	for (i = 0; !(bits & (1 << i)); )
		i++;

	tmp = readl(addr) & ~bits;
	tmp |= (val_on_bits << i) & bits;
	writel(tmp, addr);
	verify_bits(tmp, addr, bits);
}

#define clear_field(addr, bits)		clear_bits(addr, bits)
#define change_field(addr, bits)	change_bits(addr, bits)
#define test_field_set(addr, bits)	test_bits_set(addr, bits)
#define test_field_cleared(addr, bits)	test_bits_cleared(addr, bits)

static __always_inline bool test_field_equ(const void volatile __iomem *addr, u32 bits, u32 val_on_bits)
{
	return get_field(addr, bits) == val_on_bits;
}

#define wait_field_set(addr, bits, timeout_us)		wait_bits_set(addr, bits, timeout_us)
#define wait_field_cleared(addr, bits, timeout_us)	wait_bits_cleared(addr, bits, timeout_us)

static __always_inline int wait_field_equ(const volatile void  __iomem *addr, u32 bits, u32 val_on_bits, unsigned int timeout_us)
{
	int i, err;
	u32 read_val;

	if (WARN_ON(!bits))
		return -EINVAL;

	/* @bits should always be greater than 0 */
	for (i = 0; !(bits & (1 << i)); )
		i++;

	err = readl_poll_timeout_atomic(addr, read_val, ((read_val & bits) == (val_on_bits << i)), 0, timeout_us);
	if (err)
		pr_err("wait_field_equ(): Timeout. addr=0x%p, bits=0x%x\n", addr, bits);
	return err;
}

#define wait_field_set_maysleep(addr, bits, sleep_us, timeout_us)	wait_bits_set_maysleep(addr, bits, sleep_us, timeout_us)
#define wait_field_cleared_maysleep(addr, bits, sleep_us, timeout_us)	wait_bits_cleared_maysleep(addr, bits, sleep_us, timeout_us)

#endif

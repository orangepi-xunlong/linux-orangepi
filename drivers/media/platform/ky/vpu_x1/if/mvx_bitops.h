/*
 * The confidential and proprietary information contained in this file may
 * only be used by a person authorised under and to the extent permitted
 * by a subsisting licensing agreement from Arm Technology (China) Co., Ltd.
 *
 *            (C) COPYRIGHT 2021-2021 Arm Technology (China) Co., Ltd.
 *                ALL RIGHTS RESERVED
 *
 * This entire notice must be reproduced on all copies of this file
 * and copies of this file may only be made by a person if such person is
 * permitted to do so under the terms of a subsisting license agreement
 * from Arm Technology (China) Co., Ltd.
 * 
 * SPDX-License-Identifier: GPL-2.0-only
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 */

#ifndef _MVX_BITOPS_H_
#define _MVX_BITOPS_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/bug.h>
#include <linux/kernel.h>

/****************************************************************************
 * Static functions
 ****************************************************************************/

/**
 * mvx_set_bit() - Set a bit in the bitmask.
 * @bit:		Bit to be set.
 * @addr:		Pointer to bitmask.
 *
 * Works similar to set_bit but uses no locks, is not atomic and protects
 * agains overflow.
 */
static inline void mvx_set_bit(unsigned int bit,
			       uint64_t *addr)
{
	BUG_ON(bit >= (sizeof(*addr) * 8));
	*addr |= 1ull << bit;
}

/**
 * mvx_clear_bit() - Clear a bit in the bitmask.
 * @bit:		Bit to be cleared.
 * @addr:		Pointer to bitmask.
 *
 * Works similar to clear_bit but uses no locks, is not atomic and protects
 * agains overflow.
 */
static inline void mvx_clear_bit(unsigned int bit,
				 uint64_t *addr)
{
	BUG_ON(bit >= (sizeof(*addr) * 8));
	*addr &= ~(1ull << bit);
}

/**
 * mvx_test_bit() - Test a bit in the bitmask.
 * @bit:		Bit to be tested.
 * @addr:		Pointer to bitmask.
 *
 * Works similar to test_bit but uses no locks, is not atomic and protects
 * agains overflow.
 */
static inline bool mvx_test_bit(unsigned int bit,
				uint64_t *addr)
{
	BUG_ON(bit >= (sizeof(*addr) * 8));
	return 0 != (*addr & (1ull << bit));
}

#endif /* _MVX_BITOPS_H_ */

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

#ifndef _MVX_SEQ_H_
#define _MVX_SEQ_H_

/****************************************************************************
 * Defines
 ****************************************************************************/

#define mvx_seq_printf(s, tag, ind, fmt, ...)				   \
	seq_printf(s, "%-*s%-*s: " fmt, (3 * (ind)), "", 30 - (3 * (ind)), \
		   tag, ## __VA_ARGS__)

/****************************************************************************
 * Types
 ****************************************************************************/

/**
 * struct mvx_seq_hash_it - Iterator over hash table.
 */
struct mvx_seq_hash_it {
	struct hlist_node *node;
	size_t i;
	struct device *dev;
};

/**
 * mvx_seq_hash_start() - Initialize iterator.
 * @dev:	Pointer to device.
 * @head:	Pointer to a head of a hash table.
 * @size:	Size of a hash table.
 * @pos:	Position to start.
 *
 * Iterator created by this function should be provided to
 * mvx_seq_hash_start and mvx_seq_hash_stop as the first parameter.
 *
 * Return: Pointer to an iterator on success or ERR_PTR().
 */
struct mvx_seq_hash_it *mvx_seq_hash_start(struct device *dev,
					   struct hlist_head *head,
					   size_t size,
					   loff_t pos);

/**
 * mvx_seq_hash_next() - Move iterator to the next element.
 * @v:		Pointer to an iterator.
 * @head:	Pointer to a head of a hash table.
 * @size:	Size of a hash table.
 * @pos:	Position.
 *
 * Return: Iterator which points to a new element or NULL when the table
 * is over.
 */
struct mvx_seq_hash_it *mvx_seq_hash_next(void *v,
					  struct hlist_head *head,
					  size_t size,
					  loff_t *pos);

/**
 * mvx_seq_hash_stop() - Close an iterator.
 * @v:		Pointer to an iterator.
 */
void mvx_seq_hash_stop(void *v);

#endif /* _MVX_SEQ_H_ */

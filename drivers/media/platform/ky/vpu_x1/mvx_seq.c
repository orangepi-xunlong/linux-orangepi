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

#include <linux/device.h>
#include <linux/seq_file.h>
#include "mvx_seq.h"

struct mvx_seq_hash_it *mvx_seq_hash_start(struct device *dev,
					   struct hlist_head *head,
					   size_t size,
					   loff_t pos)
{
	struct mvx_seq_hash_it *it;
	size_t i;

	it = devm_kzalloc(dev, sizeof(*it), GFP_KERNEL);
	if (it == NULL)
		return ERR_PTR(-ENOMEM);

	it->dev = dev;
	for (i = 0; i < size; ++i) {
		it->i = i;
		hlist_for_each(it->node, &head[i]) {
			if (pos-- == 0)
				return it;
		}
	}

	devm_kfree(dev, it);
	return NULL;
}

struct mvx_seq_hash_it *mvx_seq_hash_next(void *v,
					  struct hlist_head *head,
					  size_t size,
					  loff_t *pos)
{
	struct mvx_seq_hash_it *it = v;

	++*pos;
	it->node = it->node->next;

	if (it->node != NULL)
		return it;

	do {
		++it->i;
	} while ((it->i < size) && hlist_empty(&head[it->i]));

	if (it->i == size) {
		devm_kfree(it->dev, it);
		return NULL;
	}

	it->node = head[it->i].first;
	return it;
}

void mvx_seq_hash_stop(void *v)
{
	struct mvx_seq_hash_it *it = v;

	if (it == NULL)
		return;

	devm_kfree(it->dev, it);
}

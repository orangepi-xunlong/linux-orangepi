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

#ifndef _MVX_SECURE_H_
#define _MVX_SECURE_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/types.h>

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;
struct dma_buf;
struct kset;
struct mvx_secure_firmware;
struct workqueue_struct;

/**
 * struct mvx_secure - Secure video.
 * @dev:	Pointer to device.
 * @kset:	Kset that allows uevents to be sent.
 * @workqueue:	Work queue used to wait for firmware load.
 */
struct mvx_secure {
	struct device *dev;
	struct kset *kset;
	struct workqueue_struct *workqueue;
};

/**
 * typedef firmware_done - Firmware load callback.
 */
typedef void (*mvx_secure_firmware_done)(struct mvx_secure_firmware *,
					 void *arg);

/**
 * struct mvx_secure_firmware - Secure firmware.
 * @dmabuf:	Pointer to DMA buffer.
 * @l2pages:	Array of L2 pages. One per core.
 * @ncores:	Maximum number of cores.
 * @major:	Firmware protocol major version.
 * @minor:	Firmware protocol minor version.
 */
struct mvx_secure_firmware {
	struct dma_buf *dmabuf;
	phys_addr_t l2pages;
	unsigned int ncores;
	struct {
		unsigned int major;
		unsigned int minor;
	} protocol;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_secure_construct() - Construct the secure object.
 * @secure:	Pointer to secure object.
 * @dev:	Pointer to device.
 *
 * Return: 0 on success, else error code.
 */
int mvx_secure_construct(struct mvx_secure *secure,
			 struct device *dev);

/**
 * mvx_secure_destruct() - Destruct the secure object.
 * @secure:	Pointer to secure object.
 */
void mvx_secure_destruct(struct mvx_secure *secure);

/**
 * mvx_secure_request_firmware_nowait() - Request secure firmware.
 * @secure:     Pointer to secure object.
 * @name:       Name of firmware binary.
 * @ncores:	Number of cores to setup.
 * @arg:        Callback argument.
 * @done:       Done callback.
 *
 * Return: 0 on success, else error code.
 */
int mvx_secure_request_firmware_nowait(struct mvx_secure *secure,
				       const char *name,
				       unsigned int ncores,
				       void *arg,
				       mvx_secure_firmware_done done);

/**
 * mvx_secure_release_firmware() - Release secure firmware.
 * @securefw:     Pointer to secure firmware.
 */
void mvx_secure_release_firmware(struct mvx_secure_firmware *securefw);

/**
 * mvx_secure_mem_alloc() - Secure memory allocation.
 * @secure:	Pointer to secure object.
 * @size:	Size in bytes to allocate.
 *
 * Return: Valid pointer on success, else ERR_PTR.
 */
struct dma_buf *mvx_secure_mem_alloc(struct mvx_secure *secure,
				     size_t size);

#endif /* _MVX_SECURE_H_ */

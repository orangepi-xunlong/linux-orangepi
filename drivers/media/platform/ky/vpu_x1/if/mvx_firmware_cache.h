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

#ifndef _MVX_FIRMWARE_CACHE_H_
#define _MVX_FIRMWARE_CACHE_H_

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/atomic.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include "mvx_if.h"

/****************************************************************************
 * Types
 ****************************************************************************/

struct device;
struct firmware;
struct mvx_client_session;
struct mvx_secure;
struct mvx_secure_firmware;

/**
 * struct mvx_fw_cache - Firmware cache.
 *
 * There is exactly one firmware context per device. It keeps track of the
 * firmware binaries.
 */
struct mvx_fw_cache {
	struct device *dev;
	struct mvx_secure *secure;
	struct mutex mutex;
	struct list_head fw_bin_list;
	struct kobject kobj;
	struct kobject *kobj_parent;
	atomic_t flush_cnt;
	struct task_struct *cache_thread;
};

/**
 * struct mvx_fw_header - Firmware binary header.
 * @rasc_jmp:		Start address.
 * @protocol_minor:	Host internface protocol minor version.
 * @protocol_major:	Host internface protocol major version.
 * @reserved:		Reserved for future use. Always 0.
 * @info_string:	Human readable codec information.
 * @part_number:	Part number.
 * @svn_revision:	SVN revision.
 * @version_string:	Firmware version.
 * @text_length:	Length in bytes of the read-only part of the firmware.
 * @bss_start_address:	Start address for BSS segment. This is always
 *                      page-aligned.
 * @bss_bitmap_size:	The number of bits used in 'bss_bitmap'.
 * @bss_bitmap:		Bitmap which pages that shall be allocated and MMU
 *                      mapped. If bit N is set, then a page shall be allocated
 *                      and MMU mapped to VA address
 *                      FW_BASE + bss_start_address + N * MVE_PAGE_SIZE.
 * @master_rw_start_address: Defines a region of shared pages.
 * @master_rw_size:	Defines a region of shared pages.
 */
struct mvx_fw_header {
	uint32_t rasc_jmp;
	uint8_t protocol_minor;
	uint8_t protocol_major;
	uint8_t reserved[2];
	uint8_t info_string[56];
	uint8_t part_number[8];
	uint8_t svn_revision[8];
	uint8_t version_string[16];
	uint32_t text_length;
	uint32_t bss_start_address;
	uint32_t bss_bitmap_size;
	uint32_t bss_bitmap[16];
	uint32_t master_rw_start_address;
	uint32_t master_rw_size;
};

/**
 * struct mvx_fw_bin - Structure describing a loaded firmware binary.
 *
 * Multiple sessions may share the same firmware binary.
 */
struct mvx_fw_bin {
	struct device *dev;
	struct mvx_fw_cache *cache;
	struct mutex mutex;
	struct kobject kobj;
	struct list_head cache_head;
	struct list_head event_list;
	char filename[128];
	enum mvx_format format;
	enum mvx_direction dir;
	struct mvx_hw_ver hw_ver;
	atomic_t flush_cnt;
	bool securevideo;
	struct {
		const struct firmware *fw;
		const struct mvx_fw_header *header;
		unsigned int text_cnt;
		unsigned int bss_cnt;
		unsigned int sbss_cnt;
	} nonsecure;
	struct {
		struct mvx_secure *secure;
		struct mvx_secure_firmware *securefw;
	} secure;
};

/**
 * struct mvx_fw_event - Firmware load event notification.
 * @head:		Used by the firmware loader. Should not be used
 *                      by the client.
 * @fw_bin_ready:	Callback routine invoked after the firmware binary has
 *                      finished loading. Will be called both on success and
 *                      failure.
 * @arg:		Argument passed to fw_bin_ready. Client may set this
 *                      pointer to any value.
 *
 * Structure used to keep track of clients that have subscribed to event
 * notification after the firmware binary has been loaded.
 */
struct mvx_fw_event {
	struct list_head head;
	void (*fw_bin_ready)(struct mvx_fw_bin *fw_bin,
			     void *arg,
			     bool same_thread);
	void *arg;
};

/****************************************************************************
 * Exported functions
 ****************************************************************************/

/**
 * mvx_fw_cache_construct() - Construct the firmware object.
 * @cache:	Pointer to firmware cache.
 * @dev:	Pointer to device.
 * @secure:	Pointer to secure video.
 * @kobj:	Pointer to parent kobj.
 *
 * When FW cache is constructed, corresponding sysfs entry will be created
 * and attached as a child to kobj.
 *
 * Return: 0 on success, else error code.
 */
int mvx_fw_cache_construct(struct mvx_fw_cache *cache,
			   struct device *dev,
			   struct mvx_secure *secure,
			   struct kobject *kobj);

/**
 * mvx_fw_cache_destruct() - Destroy the firmware object.
 * @cache:	Pointer to firmware cache.
 */
void mvx_fw_cache_destruct(struct mvx_fw_cache *cache);

/**
 * mvx_fw_cache_get() - Get a reference to a firmware binary.
 * @cache:	Pointer for firmware cache.
 * @format:	Format used on the bitstream port.
 * @dir:	Which port that is configured as bitstream port.
 * @event:	Callback routine and argument that will be invoded after
 *              the firmware binary has been loaded.
 * @hw_ver:	MVE hardware version.
 * @securevideo:Secure video enabled.
 *
 * Loading a firmware binary is an asynchronous operation. The client will be
 * informed through a callback routine when the binary is ready.
 *
 * If the firmware binary is already in the cache, then the callback routine
 * will be called directly from mvx_fw_cache_get(). The client must take care
 * not to reaquire any mutexes already held.
 *
 * If the firmware binary was not found in the cache, then the callback routine
 * will be called from a separete thread context. The client must make sure
 * its data is protected by a mutex.
 *
 * Return: 0 on success, else error code.
 */
int mvx_fw_cache_get(struct mvx_fw_cache *cache,
		     enum mvx_format format,
		     enum mvx_direction dir,
		     struct mvx_fw_event *event,
		     struct mvx_hw_ver *hw_ver,
		     bool securevideo);

/**
 * mvx_fw_cache_put() - Return firmware binary to cache and decrement the
 *                      reference count.
 * @cache:	Pointer to firmware cache.
 * @fw:_bin	Pointer to firmware binary.
 */
void mvx_fw_cache_put(struct mvx_fw_cache *cache,
		      struct mvx_fw_bin *fw_bin);

/**
 * mvx_fw_cache_log() - Log firmware binary to ram log.
 * @fw_bin:	Pointer to firmware binary.
 * @csession:	Pointer to client session.
 */
void mvx_fw_cache_log(struct mvx_fw_bin *fw_bin,
		      struct mvx_client_session *csession);

/**
 * mvx_fw_cache_get_formats() - Get supported formats.
 * @cache:	Pointer to firmware cache.
 * @direction:	Input or output port.
 * @formats:	Pointer to bitmask listing supported formats.
 */
void mvx_fw_cache_get_formats(struct mvx_fw_cache *cache,
			      enum mvx_direction direction,
			      uint64_t *formats);

#endif /* _MVX_FIRMWARE_CACHE_H_ */

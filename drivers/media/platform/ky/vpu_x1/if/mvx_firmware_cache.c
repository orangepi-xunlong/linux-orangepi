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

/****************************************************************************
 * Includes
 ****************************************************************************/

#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/firmware.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include "mvx_log_group.h"
#include "mvx_firmware_cache.h"
#include "mvx_log_ram.h"
#include "mvx_mmu.h"
#include "mvx_secure.h"
#include "mvx_seq.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

#define CACHE_CLEANUP_INTERVAL_MS       5000

#define MVX_SECURE_NUMCORES             8

/****************************************************************************
 * Private functions
 ****************************************************************************/

/*
 * Backwards compliance with older kernels.
 */
#if (KERNEL_VERSION(4, 11, 0) > LINUX_VERSION_CODE)
static unsigned int kref_read(const struct kref *kref)
{
	return atomic_read(&kref->refcount);
}

#endif

/**
 * test_bit_32() - 32 bit version Linux test_bit.
 *
 * Test if bit is set in bitmap array.
 */
static bool test_bit_32(int bit,
			uint32_t *addr)
{
	return 0 != (addr[bit >> 5] & (1 << (bit & 0x1f)));
}

/**
 * hw_id_to_name() - Convert HW id to string
 */
static const char *hw_id_to_string(enum mvx_hw_id id)
{
	switch (id) {
	case MVE_v500:
		return "v500";
	case MVE_v550:
		return "v550";
	case MVE_v61:
		return "v61";
	case MVE_v52_v76:
		return "v52_v76";
	default:
		return "unknown";
	}
}

/**
 * get_fw_name() - Return the file name for the requested format and direction.
 *
 * This function will neither check if there is hardware support nor if the
 * firmware binary is available on the file system.
 */
static int get_fw_name(char *filename,
		       size_t size,
		       enum mvx_format format,
		       enum mvx_direction dir,
		       struct mvx_hw_ver *hw_ver)
{
	const char *codec = NULL;
	const char *enc_dec = (dir == MVX_DIR_INPUT) ? "dec" : "enc";
	size_t n;

	switch (format) {
	case MVX_FORMAT_H263:
		codec = "mpeg4";
		break;
	case MVX_FORMAT_H264:
		codec = "h264";
		break;
	case MVX_FORMAT_HEVC:
		codec = "hevc";
		break;
	case MVX_FORMAT_JPEG:
		codec = "jpeg";
		break;
	case MVX_FORMAT_MPEG2:
		codec = "mpeg2";
		break;
	case MVX_FORMAT_MPEG4:
		codec = "mpeg4";
		break;
	case MVX_FORMAT_RV:
		codec = "rv";
		break;
	case MVX_FORMAT_VC1:
		codec = "vc1";
		break;
	case MVX_FORMAT_VP8:
		codec = "vp8";
		break;
	case MVX_FORMAT_VP9:
		codec = "vp9";
		break;
	case MVX_FORMAT_AVS2:
		codec = "avs2";
		break;
	case MVX_FORMAT_AVS:
		codec = "avs";
		break;
	default:
		return -ENOENT;
	}

	n = snprintf(filename, size, "linlon-%s-%u-%u/%s%s.fwb",
		     hw_id_to_string(hw_ver->id),
		     hw_ver->revision,
		     hw_ver->patch,
		     codec, enc_dec);
	if (n >= size)
		return -ENOENT;

	return 0;
}

static struct mvx_fw_bin *kobj_to_fw_bin(struct kobject *kobj)
{
	return container_of(kobj, struct mvx_fw_bin, kobj);
}

/**
 * fw_bin_destroy() - Destroy instance of firmware binary.
 */
static void fw_bin_destroy(struct kobject *kobj)
{
	struct mvx_fw_bin *fw_bin = kobj_to_fw_bin(kobj);

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "Releasing firmware binary. bin=0x%p.", fw_bin);

	if (fw_bin->securevideo == false &&
	    IS_ERR_OR_NULL(fw_bin->nonsecure.fw) == false)
		release_firmware(fw_bin->nonsecure.fw);

	if (fw_bin->securevideo != false &&
	    IS_ERR_OR_NULL(fw_bin->secure.securefw) == false)
		mvx_secure_release_firmware(fw_bin->secure.securefw);

	list_del(&fw_bin->cache_head);
	devm_kfree(fw_bin->dev, fw_bin);
}

/**
 * fw_bin_validate() - Verify that the loaded firmware is a valid binary.
 */
static int fw_bin_validate(const struct firmware *fw,
			   struct device *dev)
{
	struct mvx_fw_header *header = (struct mvx_fw_header *)fw->data;

	if (fw->size < sizeof(*header)) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Firmware binary size smaller than firmware header. size=%zu.",
			      fw->size);
		return -EFAULT;
	}

	if (header->text_length > fw->size) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Firmware text length larger than firmware binary size. text_length=%u, size=%zu.",
			      header->text_length,
			      fw->size);
		return -EFAULT;
	}

	return 0;
}

/**
 * fw_bin_callback() - Call firmware ready callback.
 */
static void fw_bin_callback(struct mvx_fw_bin *fw_bin)
{
	struct mvx_fw_event *event;
	struct mvx_fw_event *tmp;
	int ret;

	/*
	 * Continue even if lock fails, or else any waiting session will
	 * be blocked forever.
	 */
	ret = mutex_lock_interruptible(&fw_bin->mutex);

	/*
	 * Inform all clients that the firmware has been loaded. This must be
	 * done even if the firmware load fails, or else the clients will hung
	 * waiting for a firmware load the will never happen.
	 */
	list_for_each_entry_safe(event, tmp, &fw_bin->event_list, head) {
		list_del(&event->head);
		event->fw_bin_ready(fw_bin, event->arg, false);
	}

	if (ret == 0)
		mutex_unlock(&fw_bin->mutex);
}

/**
 * secure_request_firmware_done() - Firmware load callback routine.
 */
static void secure_request_firmware_done(struct mvx_secure_firmware *securefw,
					 void *arg)
{
	struct mvx_fw_bin *fw_bin = arg;

	if (securefw == NULL) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to load secure firmware binary. filename=%s.",
			      fw_bin->filename);
		securefw = ERR_PTR(-EINVAL);
		goto fw_bin_callback;
	}

fw_bin_callback:
	fw_bin->secure.securefw = securefw;

	fw_bin_callback(fw_bin);
}

/**
 * request_firmware_done() - Callback routine after firmware has been loaded.
 */
static void request_firmware_done(const struct firmware *fw,
				  void *arg)
{
	struct mvx_fw_bin *fw_bin = arg;
	struct mvx_fw_header *header;
	mvx_mmu_va va;
	int ret;
	uint32_t i;

	BUG_ON(!arg);

	if (fw == NULL) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Failed to load firmware binary. filename=%s.",
			      fw_bin->filename);
		fw = ERR_PTR(-EINVAL);
		goto fw_ready_callback;
	}

	ret = fw_bin_validate(fw, fw_bin->dev);
	if (ret != 0) {
		release_firmware(fw);
		fw = ERR_PTR(ret);
		goto fw_ready_callback;
	}

	header = (struct mvx_fw_header *)fw->data;
	fw_bin->nonsecure.header = header;

	/* Calculate number of pages needed for the text segment. */
	fw_bin->nonsecure.text_cnt =
		(header->text_length + MVE_PAGE_SIZE - 1) >> MVE_PAGE_SHIFT;

	/* Calculate number of pages needed for the BSS segments. */
	va = header->bss_start_address;
	for (i = 0; i < header->bss_bitmap_size; i++) {
		if (va >= header->master_rw_start_address &&
		    va < (header->master_rw_start_address +
			  header->master_rw_size))
			fw_bin->nonsecure.sbss_cnt++;
		else if (test_bit_32(i, header->bss_bitmap))
			fw_bin->nonsecure.bss_cnt++;

		va += MVE_PAGE_SIZE;
	}

	MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_INFO,
		      "Loaded firmware binary. bin=0x%p, major=%u, minor=%u, info=\"%s\", jump=0x%x, pages={text=%u, bss=%u, shared=%u}, text_length=%u, bss=0x%x.",
		      fw_bin,
		      header->protocol_major,
		      header->protocol_minor,
		      header->info_string,
		      header->rasc_jmp,
		      fw_bin->nonsecure.text_cnt,
		      fw_bin->nonsecure.bss_cnt,
		      fw_bin->nonsecure.sbss_cnt,
		      header->text_length,
		      header->bss_start_address);

fw_ready_callback:
	fw_bin->nonsecure.fw = fw;

	fw_bin_callback(fw_bin);
}

/**
 * hwvercmp() - Compare two hardware versions.
 *
 * Semantic of this function equivalent to strcmp().
 */
static int hwvercmp(struct mvx_hw_ver *v1,
		    struct mvx_hw_ver *v2)
{
	if (v1->id != v2->id)
		return v1->id - v2->id;

	if (v1->revision != v2->revision)
		return v1->revision - v2->revision;

	if (v1->patch != v2->patch)
		return v1->patch - v2->patch;

	return 0;
}

static ssize_t path_show(struct kobject *kobj,
			 struct kobj_attribute *attr,
			 char *buf)
{
	struct mvx_fw_bin *fw_bin = kobj_to_fw_bin(kobj);

	return scnprintf(buf, PAGE_SIZE, "%s\n", fw_bin->filename);
}

static ssize_t hw_ver_show(struct kobject *kobj,
			   struct kobj_attribute *attr,
			   char *buf)
{
	struct mvx_fw_bin *fw_bin = kobj_to_fw_bin(kobj);
	struct mvx_hw_ver *hw_ver = &fw_bin->hw_ver;

	return scnprintf(buf, PAGE_SIZE, "%s-%u-%u\n",
			 hw_id_to_string(hw_ver->id),
			 hw_ver->revision,
			 hw_ver->patch);
}

static ssize_t count_show(struct kobject *kobj,
			  struct kobj_attribute *attr,
			  char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n",
			 kref_read(&kobj->kref) - 1);
}

static ssize_t dirty_show(struct kobject *kobj,
			  struct kobj_attribute *attr,
			  char *buf)
{
	struct mvx_fw_bin *fw_bin = kobj_to_fw_bin(kobj);
	int dirty = 0;

	if (atomic_read(&fw_bin->flush_cnt) !=
	    atomic_read(&fw_bin->cache->flush_cnt))
		dirty = 1;

	return scnprintf(buf, PAGE_SIZE, "%d\n", dirty);
}

static struct kobj_attribute path_attr = __ATTR_RO(path);
static struct kobj_attribute count_attr = __ATTR_RO(count);
static struct kobj_attribute hw_ver_attr = __ATTR_RO(hw_ver);
static struct kobj_attribute dirty_attr = __ATTR_RO(dirty);

static struct attribute *fw_bin_attrs[] = {
	&path_attr.attr,
	&count_attr.attr,
	&hw_ver_attr.attr,
	&dirty_attr.attr,
	NULL
};

static const struct attribute_group fw_bin_group = {
    .name = "",
    .attrs = fw_bin_attrs
};

static const struct attribute_group *fw_bin_groups[] = {
    &fw_bin_group,
    NULL
};

static struct kobj_type fw_bin_ktype = {
	.release       = fw_bin_destroy,
	.sysfs_ops     = &kobj_sysfs_ops,
	.default_groups = fw_bin_groups
};

/**
 * fw_bin_create() - Create a new firmware binary instance.
 */
static struct mvx_fw_bin *fw_bin_create(struct mvx_fw_cache *cache,
					enum mvx_format format,
					enum mvx_direction dir,
					struct mvx_hw_ver *hw_ver,
					bool securevideo)
{
	struct mvx_fw_bin *fw_bin;
	int ret;

	/* Allocate object and initialize members. */
	fw_bin = devm_kzalloc(cache->dev, sizeof(*fw_bin), GFP_KERNEL);
	if (fw_bin == NULL)
		return ERR_PTR(-ENOMEM);

	fw_bin->dev = cache->dev;
	fw_bin->cache = cache;
	fw_bin->format = format;
	fw_bin->dir = dir;
	fw_bin->hw_ver = *hw_ver;
	atomic_set(&fw_bin->flush_cnt, atomic_read(&cache->flush_cnt));
	mutex_init(&fw_bin->mutex);
	INIT_LIST_HEAD(&fw_bin->cache_head);
	INIT_LIST_HEAD(&fw_bin->event_list);

	fw_bin->securevideo = securevideo;
	if (securevideo != false)
		fw_bin->secure.secure = cache->secure;

	ret = kobject_init_and_add(&fw_bin->kobj, &fw_bin_ktype, &cache->kobj,
				   "%lx", (unsigned long)fw_bin);
	if (ret != 0)
		goto free_fw_bin;

	ret = get_fw_name(fw_bin->filename, sizeof(fw_bin->filename), format,
			  dir, &fw_bin->hw_ver);
	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
			      "No firmware available. format=%d, direction=%d.",
			      format, dir);
		goto free_fw_bin;
	}

	kobject_get(&fw_bin->kobj);

	if (securevideo != false)
		ret = mvx_secure_request_firmware_nowait(
			cache->secure, fw_bin->filename, MVX_SECURE_NUMCORES,
			fw_bin,
			secure_request_firmware_done);
	else
		ret = request_firmware_nowait(THIS_MODULE, true,
					      fw_bin->filename,
					      fw_bin->dev, GFP_KERNEL, fw_bin,
					      request_firmware_done);

	if (ret != 0) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_ERROR,
			      "Failed to request firmware. filename=%s, securevideo=%d.",
			      fw_bin->filename, securevideo);
		kobject_put(&fw_bin->kobj);
		goto free_fw_bin;
	}

	return fw_bin;

free_fw_bin:
	kobject_put(&fw_bin->kobj);

	return ERR_PTR(ret);
}

/**
 * fw_bin_get() - Get reference to firmware binary.
 *
 * If firmware binary has already been loaded the reference count is increased,
 * else the function tries to create a new descriptor and load the firmware
 * into memory.
 */
static struct mvx_fw_bin *fw_bin_get(struct mvx_fw_cache *cache,
				     enum mvx_format format,
				     enum mvx_direction dir,
				     struct mvx_hw_ver *hw_ver,
				     bool securevideo)
{
	struct mvx_fw_bin *fw_bin = NULL;
	struct mvx_fw_bin *tmp;
	int ret;

	ret = mutex_lock_interruptible(&cache->mutex);
	if (ret != 0)
		return ERR_PTR(ret);

	/* Search if firmware binary has already been loaded. */
	list_for_each_entry(tmp, &cache->fw_bin_list, cache_head) {
		if (tmp->format == format && tmp->dir == dir &&
		    hwvercmp(&tmp->hw_ver, hw_ver) == 0 &&
		    tmp->securevideo == securevideo &&
		    atomic_read(&tmp->flush_cnt) ==
		    atomic_read(&cache->flush_cnt)) {
			fw_bin = tmp;
			break;
		}
	}

	/* If firmware was not found, then try to request firmware. */
	if (fw_bin == NULL) {
		fw_bin = fw_bin_create(cache, format, dir, hw_ver, securevideo);
		if (!IS_ERR(fw_bin))
			list_add(&fw_bin->cache_head, &cache->fw_bin_list);
	} else {
		kobject_get(&fw_bin->kobj);
	}

	mutex_unlock(&cache->mutex);

	return fw_bin;
}

/****************************************************************************
 * Private functions
 ****************************************************************************/

static struct mvx_fw_cache *kobj_to_fw_cache(struct kobject *kobj)
{
	return container_of(kobj, struct mvx_fw_cache, kobj);
}

/**
 * cache_flush_show() - FW cache flush status is always 0.
 */
static ssize_t cache_flush_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "0\n");
}

/**
 * cache_flush_store() - Trigger FW cache flush.
 */
static ssize_t cache_flush_store(struct kobject *kobj,
				 struct kobj_attribute *attr,
				 const char *buf,
				 size_t size)
{
	struct mvx_fw_cache *cache = kobj_to_fw_cache(kobj);

	atomic_inc(&cache->flush_cnt);
	return size;
}

/**
 * Sysfs attribute which triggers FW cache flush.
 */
static struct kobj_attribute cache_flush =
	__ATTR(flush, 0600, cache_flush_show, cache_flush_store);


static struct attribute *cache_attrs[] = {
	&cache_flush.attr,
	NULL
};

static const struct attribute_group cache_group = {
    .name = "",
    .attrs = cache_attrs
};

static const struct attribute_group *cache_groups[] = {
    &cache_group,
    NULL
};

static void cache_release(struct kobject *kobj)
{
	struct mvx_fw_cache *cache = kobj_to_fw_cache(kobj);

	kthread_stop(cache->cache_thread);
	kobject_put(cache->kobj_parent);
}

static struct kobj_type cache_ktype = {
	.release       = cache_release,
	.sysfs_ops     = &kobj_sysfs_ops,
	.default_groups = cache_groups
};

static void cache_update(struct mvx_fw_cache *cache)
{
	struct mvx_fw_bin *fw_bin;
	struct mvx_fw_bin *tmp;
	int ret;

	ret = mutex_lock_interruptible(&cache->mutex);
	if (ret != 0)
		return;

	list_for_each_entry_safe(fw_bin, tmp, &cache->fw_bin_list, cache_head) {
		int ref;

		ref = kref_read(&fw_bin->kobj.kref);
		if (ref == 1)
			kobject_put(&fw_bin->kobj);
	}

	mutex_unlock(&cache->mutex);
}

static int cache_thread(void *v)
{
	struct mvx_fw_cache *cache = (struct mvx_fw_cache *)v;

	while (!kthread_should_stop()) {
		cache_update(cache);
		msleep_interruptible(CACHE_CLEANUP_INTERVAL_MS);
	}

	return 0;
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_fw_cache_construct(struct mvx_fw_cache *cache,
			   struct device *dev,
			   struct mvx_secure *secure,
			   struct kobject *kobj_parent)
{
	int ret;

	cache->dev = dev;
	cache->secure = secure;
	cache->kobj_parent = kobject_get(kobj_parent);
	atomic_set(&cache->flush_cnt, 0);
	mutex_init(&cache->mutex);
	INIT_LIST_HEAD(&cache->fw_bin_list);

	ret = kobject_init_and_add(&cache->kobj, &cache_ktype,
				   kobj_parent, "fw_cache");
	if (ret != 0)
		goto kobj_put;

	cache->cache_thread = kthread_run(cache_thread, cache, "fw_cache");
	if (IS_ERR(cache->cache_thread))
		goto kobj_put;

	return 0;

kobj_put:
	kobject_put(&cache->kobj);
	kobject_put(cache->kobj_parent);
	return -EFAULT;
}

void mvx_fw_cache_destruct(struct mvx_fw_cache *cache)
{
	kobject_put(&cache->kobj);
}

int mvx_fw_cache_get(struct mvx_fw_cache *cache,
		     enum mvx_format format,
		     enum mvx_direction dir,
		     struct mvx_fw_event *event,
		     struct mvx_hw_ver *hw_ver,
		     bool securevideo)
{
	int ret;
	struct mvx_fw_bin *fw_bin;

	/* Allocate a new firmware binary or get handle to existing object. */
	fw_bin = fw_bin_get(cache, format, dir, hw_ver, securevideo);
	if (IS_ERR(fw_bin))
		return PTR_ERR(fw_bin);

	ret = mutex_lock_interruptible(&fw_bin->mutex);
	if (ret != 0) {
		mvx_fw_cache_put(cache, fw_bin);
		return ret;
	}

	/*
	 * If the firmware binary has already been loaded, then the callback
	 * routine can be called right away.
	 * Else the callback and argument is enqueued to the firmware
	 * notification list.
	 */
	if ((fw_bin->securevideo != false &&
	     IS_ERR_OR_NULL(fw_bin->secure.securefw) == false)) {
		mutex_unlock(&fw_bin->mutex);
		event->fw_bin_ready(fw_bin, event->arg, true);
	} else if (fw_bin->securevideo == false &&
		   IS_ERR_OR_NULL(fw_bin->nonsecure.fw) == false) {
		mutex_unlock(&fw_bin->mutex);
		event->fw_bin_ready(fw_bin, event->arg, true);
	} else {
		list_add(&event->head, &fw_bin->event_list);
		mutex_unlock(&fw_bin->mutex);
	}

	return 0;
}

void mvx_fw_cache_put(struct mvx_fw_cache *cache,
		      struct mvx_fw_bin *fw_bin)
{
	int ret;

	ret = mutex_lock_interruptible(&cache->mutex);

	kobject_put(&fw_bin->kobj);

	if (ret == 0)
		mutex_unlock(&cache->mutex);
}

void mvx_fw_cache_log(struct mvx_fw_bin *fw_bin,
		      struct mvx_client_session *csession)
{
	struct mvx_log_header header;
	struct mvx_log_fw_binary fw_binary;
	struct timespec64 timespec;
	struct iovec vec[3];

	if (fw_bin->securevideo != false)
		return;

	ktime_get_real_ts64(&timespec);

	header.magic = MVX_LOG_MAGIC;
	header.length = sizeof(fw_binary) + sizeof(*fw_bin->nonsecure.header);
	header.type = MVX_LOG_TYPE_FW_BINARY;
	header.severity = MVX_LOG_INFO;
	header.timestamp.sec = timespec.tv_sec;
	header.timestamp.nsec = timespec.tv_nsec;

	fw_binary.session = (uintptr_t)csession;

	vec[0].iov_base = &header;
	vec[0].iov_len = sizeof(header);

	vec[1].iov_base = &fw_binary;
	vec[1].iov_len = sizeof(fw_binary);

	vec[2].iov_base = (void *)fw_bin->nonsecure.header;
	vec[2].iov_len = sizeof(*fw_bin->nonsecure.header);

	MVX_LOG_DATA(&mvx_log_fwif_if, MVX_LOG_INFO, vec, 3);
}

void mvx_fw_cache_get_formats(struct mvx_fw_cache *cache,
			      enum mvx_direction direction,
			      uint64_t *formats)
{
	/* Support all formats by default. */
	*formats = (1ull << MVX_FORMAT_MAX) - 1ull;

	/* TODO remove formats we can't find any firmware for. */
}

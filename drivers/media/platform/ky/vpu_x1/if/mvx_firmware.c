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

#include <linux/atomic.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-buf.h>
#include <linux/errno.h>
#include <linux/gfp.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/firmware.h>
#include <linux/kobject.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/version.h>
#include "mvx_if.h"
#include "mvx_log_group.h"
#include "mvx_firmware_cache.h"
#include "mvx_firmware_priv.h"
#include "mvx_mmu.h"
#include "mvx_secure.h"
#include "mvx_seq.h"

/****************************************************************************
 * Defines
 ****************************************************************************/

#define FW_TEXT_BASE_ADDR       0x1000u

/****************************************************************************
 * Private functions
 ****************************************************************************/

/**
 * test_bit_32() - 32 bit version Linux test_bit.
 *
 * Test if bit is set in bitmap array.
 */
static bool test_bit_32(int bit,
			const uint32_t *addr)
{
	return 0 != (addr[bit >> 5] & (1 << (bit & 0x1f)));
}

/**
 * get_major_version() - Get firmware major version.
 *
 * Return: Major version.
 */
static unsigned int get_major_version(const struct mvx_fw_bin *fw_bin)
{
	if (fw_bin->securevideo != false)
		return fw_bin->secure.securefw->protocol.major;
	else
		return fw_bin->nonsecure.header->protocol_major;
}

/**
 * get_minor_version() - Get firmware minor version.
 *
 * Return: Minor version.
 */
static unsigned int get_minor_version(const struct mvx_fw_bin *fw_bin)
{
	if (fw_bin->securevideo != false)
		return fw_bin->secure.securefw->protocol.minor;
	else
		return fw_bin->nonsecure.header->protocol_minor;
}

/**
 * fw_unmap() - Remove MMU mappings and release allocated memory.
 */
static void fw_unmap(struct mvx_fw *fw)
{
	unsigned int i;
	uint32_t begin;
	uint32_t end;
	int ret;

	if (fw->fw_bin->securevideo == false) {
		/* Unmap a region of 4 MB for each core. */
		for (i = 0; i < fw->ncores; i++) {
			ret = fw->ops.get_region(MVX_FW_REGION_CORE_0 + i,
						 &begin, &end);
			if (ret == 0)
				mvx_mmu_unmap_va(fw->mmu, begin,
						 4 * 1024 * 1024);
		}

		if (!IS_ERR_OR_NULL(fw->text))
			mvx_mmu_free_pages(fw->text);

		if (!IS_ERR_OR_NULL(fw->bss))
			mvx_mmu_free_pages(fw->bss);

		if (!IS_ERR_OR_NULL(fw->bss_shared))
			mvx_mmu_free_pages(fw->bss_shared);
	}

	fw->ops.unmap_protocol(fw);
}

/**
 * fw_map_core() - Map pages for the text and BSS segments for one core.
 *
 * This function assumes that the fw instance has been correctly allocated
 * and instansiated and will therefor not make any NULL pointer checks. It
 * assumes that all pointers - for example to the mmu or firmware binary - have
 * been correctly set up.
 */
static int fw_map_core(struct mvx_fw *fw,
		       unsigned int core)
{
	int ret;
	const struct mvx_fw_header *header = fw->fw_bin->nonsecure.header;
	mvx_mmu_va fw_base;
	mvx_mmu_va end;
	mvx_mmu_va va;
	unsigned int i;
	unsigned int bss_cnt = core * fw->fw_bin->nonsecure.bss_cnt;
	unsigned int bss_scnt = 0;

	/*
	 * Get the base address where the pages for this cores should be
	 * mapped.
	 */
	ret = fw->ops.get_region(MVX_FW_REGION_CORE_0 + core, &fw_base, &end);
	if (ret != 0)
		return ret;

	/* Map text segment. */
	ret = mvx_mmu_map_pages(fw->mmu,
				fw_base + FW_TEXT_BASE_ADDR,
				fw->text,
				MVX_ATTR_PRIVATE,
				MVX_ACCESS_EXECUTABLE);
	if (ret != 0)
		return ret;

	/* Map bss shared and private pages. */
	va = header->bss_start_address;
	for (i = 0; i < header->bss_bitmap_size; i++) {
		if (va >= header->master_rw_start_address &&
		    va < (header->master_rw_start_address +
			  header->master_rw_size))
			ret = mvx_mmu_map_pa(
				fw->mmu,
				fw_base + va,
				fw->bss_shared->pages[bss_scnt++],
				MVE_PAGE_SIZE,
				MVX_ATTR_PRIVATE,
				MVX_ACCESS_READ_WRITE);
		else if (test_bit_32(i, header->bss_bitmap))
			ret = mvx_mmu_map_pa(fw->mmu,
					     fw_base + va,
					     fw->bss->pages[bss_cnt++],
					     MVE_PAGE_SIZE,
					     MVX_ATTR_PRIVATE,
					     MVX_ACCESS_READ_WRITE);

		if (ret != 0)
			return ret;

		va += MVE_PAGE_SIZE;
	}

	return 0;
}

/**
 * fw_map() - Map up MMU tables.
 */
static int fw_map(struct mvx_fw *fw)
{
	int ret;
	unsigned int i;

	if (fw->fw_bin->securevideo != false) {
		/* Map MMU tables for each core. */
		for (i = 0; i < fw->ncores; i++) {
			mvx_mmu_va fw_base;
			mvx_mmu_va end;
			phys_addr_t l2 = fw->fw_bin->secure.securefw->l2pages +
					 i * MVE_PAGE_SIZE;

			ret = fw->ops.get_region(MVX_FW_REGION_CORE_0 + i,
						 &fw_base, &end);
			if (ret != 0)
				return ret;

			ret = mvx_mmu_map_l2(fw->mmu, fw_base, l2);
			if (ret != 0)
				goto unmap_fw;
		}
	} else {
		const struct mvx_fw_bin *fw_bin = fw->fw_bin;
		const struct mvx_fw_header *header = fw_bin->nonsecure.header;

		/* Allocate memory for text segment. */
		fw->text = mvx_mmu_alloc_pages(fw->dev,
					       fw_bin->nonsecure.text_cnt,
					       0);
		if (IS_ERR(fw->text))
			return PTR_ERR(fw->text);

		/* Allocate memory for BSS segment. */
		fw->bss = mvx_mmu_alloc_pages(
			fw->dev, fw_bin->nonsecure.bss_cnt * fw->ncores, 0);
		if (IS_ERR(fw->bss)) {
			ret = PTR_ERR(fw->bss);
			goto unmap_fw;
		}

		/* Allocate memory for BSS shared segment. */
		fw->bss_shared = mvx_mmu_alloc_pages(
			fw->dev, fw_bin->nonsecure.sbss_cnt, 0);
		if (IS_ERR(fw->bss_shared)) {
			ret = PTR_ERR(fw->bss_shared);
			goto unmap_fw;
		}

		/* Map MMU tables for each core. */
		for (i = 0; i < fw->ncores; i++) {
			ret = fw_map_core(fw, i);
			if (ret != 0)
				goto unmap_fw;
		}

		/* Copy firmware binary. */
		ret = mvx_mmu_write(fw->mmu, FW_TEXT_BASE_ADDR,
				    fw_bin->nonsecure.fw->data,
				    header->text_length);
		if (ret != 0)
			goto unmap_fw;
	}

	/* Map MMU tables for the message queues. */
	ret = fw->ops.map_protocol(fw);
	if (ret != 0)
		goto unmap_fw;

	return 0;

unmap_fw:
	fw_unmap(fw);

	return ret;
}

/**
 * Callbacks and handlers for FW stats.
 */
static int fw_stat_show(struct seq_file *s,
			void *v)
{
	struct mvx_fw *fw = (struct mvx_fw *)s->private;
	const struct mvx_fw_bin *fw_bin = fw->fw_bin;

	mvx_seq_printf(s, "mvx_fw", 0, "%p\n", fw);
	seq_puts(s, "\n");

	mvx_seq_printf(s, "mmu", 0, "%p\n", fw->mmu);

	if (fw_bin->securevideo == false) {
		mvx_seq_printf(s, "text", 0, "%p\n", fw->text);
		mvx_seq_printf(s, "bss", 0, "%p\n", fw->bss);
		mvx_seq_printf(s, "bss_shared", 0, "%p\n", fw->bss_shared);
	}

	seq_puts(s, "\n");

	mvx_seq_printf(s, "msg_host", 0, "%p\n", fw->msg_host);
	mvx_seq_printf(s, "msg_mve", 0, "%p\n", fw->msg_mve);
	mvx_seq_printf(s, "buf_in_host", 0, "%p\n", fw->buf_in_host);
	mvx_seq_printf(s, "buf_in_mve", 0, "%p\n", fw->buf_in_mve);
	mvx_seq_printf(s, "buf_out_host", 0, "%p\n", fw->buf_out_host);
	mvx_seq_printf(s, "buf_out_mve", 0, "%p\n", fw->buf_out_mve);
	seq_puts(s, "\n");

	fw->ops.print_stat(fw, 0, s);
	seq_puts(s, "\n");

	mvx_seq_printf(s, "rpc", 0, "%p\n", fw->rpc);
	mvx_seq_printf(s, "ncores", 0, "%u\n", fw->ncores);
	mvx_seq_printf(s, "msg_pending", 0, "%u\n", fw->msg_pending);
	seq_puts(s, "\n");

	mvx_seq_printf(s, "ops.map_protocol", 0, "%ps\n",
		       fw->ops.map_protocol);
	mvx_seq_printf(s, "ops.unmap_protocol", 0, "%ps\n",
		       fw->ops.unmap_protocol);
	mvx_seq_printf(s, "ops.get_region", 0, "%ps\n",
		       fw->ops.get_region);
	mvx_seq_printf(s, "ops.get_message", 0, "%ps\n",
		       fw->ops.get_message);
	mvx_seq_printf(s, "ops.put_message", 0, "%ps\n",
		       fw->ops.put_message);
	mvx_seq_printf(s, "ops.handle_rpc", 0, "%ps\n",
		       fw->ops.handle_rpc);
	seq_puts(s, "\n");

	mvx_seq_printf(s, "fw_bin", 0, "%p\n", fw_bin);
	mvx_seq_printf(s, "fw_bin.cache", 0, "%p\n", fw_bin->cache);
	mvx_seq_printf(s, "fw_bin.filename", 0, "%s\n", fw_bin->filename);
	mvx_seq_printf(s, "fw_bin.format", 0, "%u\n", fw_bin->format);
	mvx_seq_printf(s, "fw_bin.dir", 0, "%s\n",
		       (fw_bin->dir == MVX_DIR_INPUT) ? "in" :
		       (fw_bin->dir == MVX_DIR_OUTPUT) ? "out" :
		       "invalid");

	if (fw_bin->securevideo == false) {
		mvx_seq_printf(s, "fw_bin.text_cnt", 0, "%u\n",
			       fw_bin->nonsecure.text_cnt);
		mvx_seq_printf(s, "fw_bin.bss_cnt", 0, "%u\n",
			       fw_bin->nonsecure.bss_cnt);
		mvx_seq_printf(s, "fw_bin.sbss_cnt", 0, "%u\n",
			       fw_bin->nonsecure.sbss_cnt);
	}

	return 0;
}

static int fw_stat_open(struct inode *inode,
			struct file *file)
{
	return single_open(file, fw_stat_show, inode->i_private);
}

static const struct file_operations fw_stat_fops = {
	.open    = fw_stat_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static void *rpcmem_seq_start(struct seq_file *s,
			      loff_t *pos)
{
	struct mvx_fw *fw = s->private;
	int ret;

	ret = mutex_lock_interruptible(&fw->rpcmem_mutex);
	if (ret != 0)
		return ERR_PTR(-EINVAL);

	return mvx_seq_hash_start(fw->dev, fw->rpc_mem, HASH_SIZE(
					  fw->rpc_mem), *pos);
}

static void *rpcmem_seq_next(struct seq_file *s,
			     void *v,
			     loff_t *pos)
{
	struct mvx_fw *fw = s->private;

	return mvx_seq_hash_next(v, fw->rpc_mem, HASH_SIZE(fw->rpc_mem), pos);
}

static void rpcmem_seq_stop(struct seq_file *s,
			    void *v)
{
	struct mvx_fw *fw = s->private;

	mutex_unlock(&fw->rpcmem_mutex);
	mvx_seq_hash_stop(v);
}

static int rpcmem_seq_show(struct seq_file *s,
			   void *v)
{
	struct mvx_seq_hash_it *it = v;
	struct mvx_mmu_pages *pages = hlist_entry(it->node,
						  struct mvx_mmu_pages, node);

	if (pages == NULL)
		return 0;

	seq_printf(s, "va = %08x, cap = %08zu, count = %08zu\n",
		   pages->va, pages->capacity, pages->count);

	return 0;
}

static const struct seq_operations rpcmem_seq_ops = {
	.start = rpcmem_seq_start,
	.next  = rpcmem_seq_next,
	.stop  = rpcmem_seq_stop,
	.show  = rpcmem_seq_show
};

static int rpcmem_open(struct inode *inode,
		       struct file *file)
{
	int ret;
	struct seq_file *s;
	struct mvx_fw *fw = inode->i_private;

	ret = seq_open(file, &rpcmem_seq_ops);
	if (ret != 0)
		return ret;

	s = file->private_data;
	s->private = fw;

	return 0;
}

static const struct file_operations rpcmem_fops = {
	.open    = rpcmem_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

/**
 * fw_debugfs_init() - Create debugfs entries for mvx_fw.
 */
static int fw_debugfs_init(struct mvx_fw *fw,
			   struct dentry *parent)
{
	int ret;
	struct dentry *dentry;

	fw->dentry = debugfs_create_dir("fw", parent);
	if (IS_ERR_OR_NULL(fw->dentry))
		return -ENOMEM;

	dentry = debugfs_create_file("stat", 0400, fw->dentry, fw,
				     &fw_stat_fops);
	if (IS_ERR_OR_NULL(dentry)) {
		ret = -ENOMEM;
		goto remove_dentry;
	}

	if (fw->fw_bin->securevideo == false) {
		ret = mvx_mmu_pages_debugfs_init(fw->text, "text", fw->dentry);
		if (ret != 0)
			goto remove_dentry;

		ret = mvx_mmu_pages_debugfs_init(fw->bss, "bss", fw->dentry);
		if (ret != 0)
			goto remove_dentry;

		ret = mvx_mmu_pages_debugfs_init(fw->bss_shared, "bss_shared",
						 fw->dentry);
		if (ret != 0)
			goto remove_dentry;

		dentry = debugfs_create_file("rpc_mem", 0400, fw->dentry, fw,
					     &rpcmem_fops);
		if (IS_ERR_OR_NULL(dentry)) {
			ret = -ENOMEM;
			goto remove_dentry;
		}
	}

	return 0;

remove_dentry:
	debugfs_remove_recursive(fw->dentry);
	return ret;
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_fw_factory(struct mvx_fw *fw,
		   struct mvx_fw_bin *fw_bin,
		   struct mvx_mmu *mmu,
		   struct mvx_session *session,
		   struct mvx_client_ops *client_ops,
		   struct mvx_client_session *csession,
		   unsigned int ncores,
		   struct dentry *parent)
{
	unsigned int major;
	unsigned int minor;
	int ret;

	/* Verifty that firmware loading was successful. */
	if ((fw_bin->securevideo == false &&
	     IS_ERR_OR_NULL(fw_bin->nonsecure.fw)) ||
	    (fw_bin->securevideo != false &&
	     IS_ERR_OR_NULL(fw_bin->secure.securefw))) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Firmware binary was loaded with error.");
		return -EINVAL;
	}

	if (fw_bin->securevideo != false &&
	    ncores > fw_bin->secure.securefw->ncores) {
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Too few secure cores setup. max_ncores=%u, ncores=%u.",
			      fw_bin->secure.securefw->ncores, ncores);
		return -EINVAL;
	}

	major = get_major_version(fw_bin);
	minor = get_minor_version(fw_bin);

	/* Call constructor for derived class based on protocol version. */
	switch (major) {
	case 2:
		ret = mvx_fw_construct_v2(fw, fw_bin, mmu, session, client_ops,
					  csession, ncores, major, minor);
		if (ret != 0)
			return ret;

		break;
	case 3:
		ret = mvx_fw_construct_v3(fw, fw_bin, mmu, session, client_ops,
					  csession, ncores, major, minor);
		if (ret != 0)
			return ret;

		break;
	default:
		MVX_LOG_PRINT(&mvx_log_if, MVX_LOG_WARNING,
			      "Unsupported firmware interface revision. major=%u, minor=%u.",
			      major, minor);
		return -EINVAL;
	}

	/* Map up the MMU tables. */
	ret = fw_map(fw);
	if (ret != 0)
		return ret;

	if (IS_ENABLED(CONFIG_DEBUG_FS))
		ret = fw_debugfs_init(fw, parent);

	return ret;
}

int mvx_fw_construct(struct mvx_fw *fw,
		     struct mvx_fw_bin *fw_bin,
		     struct mvx_mmu *mmu,
		     struct mvx_session *session,
		     struct mvx_client_ops *client_ops,
		     struct mvx_client_session *csession,
		     unsigned int ncores)
{
	memset(fw, 0, sizeof(*fw));

	fw->dev = fw_bin->dev;
	fw->mmu = mmu;
	fw->session = session;
	fw->client_ops = client_ops;
	fw->csession = csession;
	fw->ncores = ncores;
	fw->fw_bin = fw_bin;
	mutex_init(&fw->rpcmem_mutex);

	return 0;
}

void mvx_fw_destruct(struct mvx_fw *fw)
{
	if (IS_ENABLED(CONFIG_DEBUG_FS))
		debugfs_remove_recursive(fw->dentry);

	/* Release and unmap allocates pages. */
	fw_unmap(fw);
}

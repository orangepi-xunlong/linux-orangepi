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

#include <linux/device.h>
#include <linux/of_address.h>
#include <linux/clk.h>
#include "mvx_log_group.h"
#include "mvx_hwreg.h"
#include "mvx_hwreg_v500.h"
#include "mvx_hwreg_v550.h"
#include "mvx_hwreg_v61.h"
#include "mvx_hwreg_v52_v76.h"
#include "mvx_pm_runtime.h"
#include "mvx_dev.h"

/****************************************************************************
 * Static functions
 ****************************************************************************/

static unsigned int get_offset(enum mvx_hwreg_what what)
{
	switch (what) {
	case MVX_HWREG_HARDWARE_ID:
		return 0x0;
	case MVX_HWREG_ENABLE:
		return 0x4;
	case MVX_HWREG_NCORES:
		return 0x8;
	case MVX_HWREG_NLSID:
		return 0xc;
	case MVX_HWREG_CORELSID:
		return 0x10;
	case MVX_HWREG_JOBQUEUE:
		return 0x14;
	case MVX_HWREG_IRQVE:
		return 0x18;
	case MVX_HWREG_CLKFORCE:
		return 0x24;
	case MVX_HWREG_FUSE:
		return 0x34;
	case MVX_HWREG_CONFIG:
		return 0x38;
	case MVX_HWREG_PROTCTRL:
		return 0x40;
	case MVX_HWREG_RESET:
		return 0x50;
	default:
		return 0;
	}
}

static unsigned int get_lsid_offset(unsigned int lsid,
				    enum mvx_hwreg_lsid what)
{
	unsigned int offset = 0x0200 + 0x40 * lsid;

	switch (what) {
	case MVX_HWREG_CTRL:
		offset += 0x0;
		break;
	case MVX_HWREG_MMU_CTRL:
		offset += 0x4;
		break;
	case MVX_HWREG_NPROT:
		offset += 0x8;
		break;
	case MVX_HWREG_ALLOC:
		offset += 0xc;
		break;
	case MVX_HWREG_FLUSH_ALL:
		offset += 0x10;
		break;
	case MVX_HWREG_SCHED:
		offset += 0x14;
		break;
	case MVX_HWREG_TERMINATE:
		offset += 0x18;
		break;
	case MVX_HWREG_LIRQVE:
		offset += 0x1c;
		break;
	case MVX_HWREG_IRQHOST:
		offset += 0x20;
		break;
	case MVX_HWREG_INTSIG:
		offset += 0x24;
		break;
	case MVX_HWREG_STREAMID:
		offset += 0x2c;
		break;
	case MVX_HWREG_BUSATTR_0:
		offset += 0x30;
		break;
	case MVX_HWREG_BUSATTR_1:
		offset += 0x34;
		break;
	case MVX_HWREG_BUSATTR_2:
		offset += 0x38;
		break;
	case MVX_HWREG_BUSATTR_3:
		offset += 0x3c;
		break;
	default:
		return 0;
	}

	return offset;
}

static enum mvx_hw_id get_hw_id(void *registers,
				uint32_t *revision,
				uint32_t *patch)
{
	uint32_t value;

	value = readl(registers);

	if (revision != NULL)
		*revision = (value >> 8) & 0xff;

	if (patch != NULL)
		*patch = value & 0xff;

	switch (value >> 16) {
	case 0x5650:
		return MVE_v500;
	case 0x5655:
		return MVE_v550;
	case 0x5660:
	case 0x5661:
		return MVE_v61;
	case 0x5662:
    case 0x5663:
		return MVE_v52_v76;
	default:
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Unknown hardware version. version=0x%08x.",
			      value);
		break;
	}

	return MVE_Unknown;
}

static int regs_show(struct seq_file *s,
		     void *v)
{
	struct mvx_hwreg *hwreg = (struct mvx_hwreg *)s->private;
	int ret;

	ret = mvx_pm_runtime_get_sync(hwreg->dev);
	if (ret < 0)
		return 0;

	seq_printf(s, "HARDWARE_ID = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_HARDWARE_ID));
	seq_printf(s, "ENABLE = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_ENABLE));
	seq_printf(s, "NCORES = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_NCORES));
	seq_printf(s, "NLSID = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_NLSID));
	seq_printf(s, "CORELSID = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_CORELSID));
	seq_printf(s, "JOBQUEUE = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_JOBQUEUE));
	seq_printf(s, "IRQVE = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_IRQVE));
	seq_printf(s, "CLKFORCE = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_CLKFORCE));
	seq_printf(s, "FUSE = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_FUSE));
	seq_printf(s, "CONFIG = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_CONFIG));
	seq_printf(s, "PROTCTRL = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_PROTCTRL));
	seq_printf(s, "RESET = 0x%08x\n",
		   mvx_hwreg_read(hwreg, MVX_HWREG_RESET));
	seq_puts(s, "\n");

	mvx_pm_runtime_put_sync(hwreg->dev);

	return 0;
}

static int regs_open(struct inode *inode,
		     struct file *file)
{
	return single_open(file, regs_show, inode->i_private);
}

static const struct file_operations regs_fops = {
	.open    = regs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int regs_debugfs_init(struct mvx_hwreg *hwreg,
			     struct dentry *parent)
{
	struct dentry *dentry;

	dentry = debugfs_create_file("regs", 0400, parent, hwreg,
				     &regs_fops);
	if (IS_ERR_OR_NULL(dentry))
		return -ENOMEM;

	return 0;
}

static int lsid_regs_show(struct seq_file *s,
			  void *v)
{
	struct mvx_lsid_hwreg *lsid_hwreg = (struct mvx_lsid_hwreg *)s->private;
	struct mvx_hwreg *hwreg = lsid_hwreg->hwreg;
	int lsid = lsid_hwreg->lsid;
	int ret;

	ret = mvx_pm_runtime_get_sync(hwreg->dev);
	if (ret < 0)
		return 0;

	seq_printf(s, "CTRL = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_CTRL));
	seq_printf(s, "MMU_CTRL = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_MMU_CTRL));
	seq_printf(s, "NPROT = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_NPROT));
	seq_printf(s, "ALLOC = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_ALLOC));
	seq_printf(s, "FLUSH_ALL = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_FLUSH_ALL));
	seq_printf(s, "SCHED = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_SCHED));
	seq_printf(s, "TERMINATE = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_TERMINATE));
	seq_printf(s, "LIRQVE = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_LIRQVE));
	seq_printf(s, "IRQHOST = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_IRQHOST));
	seq_printf(s, "INTSIG = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_INTSIG));
	seq_printf(s, "STREAMID = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_STREAMID));
	seq_printf(s, "BUSATTR_0 = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_BUSATTR_0));
	seq_printf(s, "BUSATTR_1 = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_BUSATTR_1));
	seq_printf(s, "BUSATTR_2 = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_BUSATTR_2));
	seq_printf(s, "BUSATTR_3 = 0x%08x\n",
		   mvx_hwreg_read_lsid(hwreg, lsid, MVX_HWREG_BUSATTR_3));
	seq_puts(s, "\n");

	mvx_pm_runtime_put_sync(hwreg->dev);

	return 0;
}

static int lsid_regs_open(struct inode *inode,
			  struct file *file)
{
	return single_open(file, lsid_regs_show, inode->i_private);
}

static const struct file_operations lsid_regs_fops = {
	.open    = lsid_regs_open,
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = single_release
};

static int lsid_regs_debugfs_init(struct mvx_lsid_hwreg *lsid_hwreg,
				  struct dentry *parent)
{
	struct dentry *dentry;
	char name[20];

	scnprintf(name, sizeof(name), "lsid%u_regs", lsid_hwreg->lsid);

	dentry = debugfs_create_file(name, 0400, parent, lsid_hwreg,
				     &lsid_regs_fops);
	if (IS_ERR_OR_NULL(dentry))
		return -ENOMEM;

	return 0;
}

int debugfs_init(struct mvx_hwreg *hwreg,
		 struct dentry *parent)
{
	int ret;

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		int lsid;

		ret = regs_debugfs_init(hwreg, parent);
		if (ret != 0)
			return ret;

		for (lsid = 0; lsid < MVX_LSID_MAX; ++lsid) {
			ret = lsid_regs_debugfs_init(&hwreg->lsid_hwreg[lsid],
						     parent);
			if (ret != 0)
				return ret;
		}
	}

	return 0;
}

/****************************************************************************
 * Exported functions
 ****************************************************************************/

int mvx_hwreg_construct(struct mvx_hwreg *hwreg,
			struct device *dev,
			struct resource *res,
			struct dentry *parent)
{
	char const *name = dev_name(dev);
	enum mvx_hw_id hw_id;
	int ret;
	int lsid;
	struct mvx_dev_ctx *ctx = dev_get_drvdata(dev);

	hwreg->dev = dev;

	hwreg->res = request_mem_region(res->start, resource_size(res), name);
	if (hwreg->res == NULL) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to request mem region. start=0x%llx, size=0x%llx.",
			      res->start, resource_size(res));
		return -EINVAL;
	}

	hwreg->registers = ioremap(res->start, resource_size(res));
	if (hwreg->registers == NULL) {
		MVX_LOG_PRINT(&mvx_log_dev, MVX_LOG_ERROR,
			      "Failed to iomap region. start=0x%llx, size=0x%llx.",
			      res->start, resource_size(res));
		ret = -ENOMEM;
		goto release_mem;
	}

	hw_id = get_hw_id(hwreg->registers, &ctx->hw_revision, &ctx->hw_patch);
	switch (hw_id) {
	case MVE_v500:
		hwreg->ops.get_formats = mvx_hwreg_get_formats_v500;
		break;
	case MVE_v550:
		hwreg->ops.get_formats = mvx_hwreg_get_formats_v550;
		break;
	case MVE_v61:
		hwreg->ops.get_formats = mvx_hwreg_get_formats_v61;
		break;
	case MVE_v52_v76:
		hwreg->ops.get_formats = mvx_hwreg_get_formats_v52_v76;
		break;
	default:
		ret = -EINVAL;
		goto unmap_io;
	}

	ctx->hw_id = hw_id;

	for (lsid = 0; lsid < MVX_LSID_MAX; ++lsid) {
		hwreg->lsid_hwreg[lsid].hwreg = hwreg;
		hwreg->lsid_hwreg[lsid].lsid = lsid;
	}

	if (IS_ENABLED(CONFIG_DEBUG_FS)) {
		ret = debugfs_init(hwreg, parent);
		if (ret != 0)
			goto unmap_io;
	}

	return 0;

unmap_io:
	iounmap(hwreg->registers);

release_mem:
	release_mem_region(res->start, resource_size(res));

	return ret;
}

void mvx_hwreg_destruct(struct mvx_hwreg *hwreg)
{
	iounmap(hwreg->registers);
	release_mem_region(hwreg->res->start, resource_size(hwreg->res));
}

uint32_t mvx_hwreg_read(struct mvx_hwreg *hwreg,
			enum mvx_hwreg_what what)
{
	unsigned int offset = get_offset(what);

	return readl(hwreg->registers + offset);
}

void mvx_hwreg_write(struct mvx_hwreg *hwreg,
		     enum mvx_hwreg_what what,
		     uint32_t value)
{
	unsigned int offset = get_offset(what);

	writel(value, hwreg->registers + offset);
}

uint32_t mvx_hwreg_read_lsid(struct mvx_hwreg *hwreg,
			     unsigned int lsid,
			     enum mvx_hwreg_lsid what)
{
	unsigned int offset = get_lsid_offset(lsid, what);

	return readl(hwreg->registers + offset);
}

void mvx_hwreg_write_lsid(struct mvx_hwreg *hwreg,
			  unsigned int lsid,
			  enum mvx_hwreg_lsid what,
			  uint32_t value)
{
	unsigned int offset = get_lsid_offset(lsid, what);

	writel(value, hwreg->registers + offset);
}

enum mvx_hw_id mvx_hwreg_get_hw_id(struct mvx_hwreg *hwreg,
				   uint32_t *revision,
				   uint32_t *patch)
{
	return get_hw_id(hwreg->registers, revision, patch);
}

// SPDX-License-Identifier: GPL-2.0
/*
 * Description on this file
 *
 * Copyright (C) 2023 KY Micro Limited
 */

#include "x1_isp_reg.h"
#include "x1_isp_drv.h"

#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/i2c.h>

ulong __iomem g_base_addr = 0, g_end_addr = 0;

void x1isp_reg_set_base_addr(ulong __iomem base_reg_addr, ulong __iomem end_reg_addr)
{
	g_base_addr = base_reg_addr;
	g_end_addr = end_reg_addr;
}

/*
 * This function must make sure the addr is subtraction ISP_REG_BASE_OFFSET(0x30000)
 */
ulong x1isp_reg_readl(ulong __iomem addr)
{
	ulong value = 0, temp_addr = 0;
	ulong __iomem reg_addr = 0;

	temp_addr = (addr & ISP_REG_MASK) - ISP_REG_BASE_OFFSET;
	reg_addr = g_base_addr + temp_addr;

	if (reg_addr > g_end_addr) {
		isp_log_err
		    ("read the reg_addr(0x%lx) is beyond the range[0x%lx-0x%lx]!",
		     reg_addr, g_base_addr, g_end_addr);
		return 0;
	}

	value = readl((void __iomem *)reg_addr);
//      isp_log_dbg("reg[addr:0x%lx] read, reg_addr=0x%lx!", addr, reg_addr);

	return value;
}

/*
* This function must make sure the addr is subtraction ISP_REG_BASE_OFFSET(0x30000)
*/
int x1isp_reg_writel(ulong __iomem addr, ulong value, ulong mask)
{
	ulong temp_value = 0, temp_addr = 0;
	ulong __iomem reg_addr = 0;
	static DEFINE_RATELIMIT_STATE(rs, 5 * HZ, 20);

	if (0 == mask && __ratelimit(&rs)) {
		isp_log_err("reg[addr:0x%lx] mask is zeor!", addr);
		return -1;
	}

	temp_addr = (addr & ISP_REG_MASK) - ISP_REG_BASE_OFFSET;
	reg_addr = g_base_addr + temp_addr;

	if (reg_addr < g_base_addr || reg_addr > g_end_addr) {
		isp_log_err
		    ("write the reg_addr(0x%lx) is beyond the range[0x%lx-0x%lx]!",
		     reg_addr, g_base_addr, g_end_addr);
		return -1;
	}

	if (mask != 0xffffffff) {
		temp_value = readl((void __iomem *)reg_addr);
		temp_value = (temp_value & ~mask) | (value & mask);
	} else
		temp_value = value;

	writel(temp_value, (void __iomem *)reg_addr);
//      isp_log_dbg("reg[addr:0x%lx] write=0x%lx, mask=0x%lx, temp_addr=0x%lx, reg_addr=0x%lx!", addr, value, mask, temp_addr, reg_addr);

#ifdef ISP_REG_DEBUG
	ulong after = 0;
	after = readl(reg_addr);
	if (after != temp_value) {
		isp_log_info
		    ("reg[addr:0x%llx] write may be failed, write=0x%x, after=0x%x!",
		     addr, value, after);
	}
#endif

	return 0;
}

int x1isp_reg_write_single(struct isp_reg_unit *reg_unit)
{
	int ret = 0;

	if (!reg_unit)
		return -EINVAL;

	ret = x1isp_reg_writel(reg_unit->reg_addr, reg_unit->reg_value, reg_unit->reg_mask);

	return ret;
}

int x1isp_reg_write_brust(void *reg_data, u32 reg_size, bool user_space,
			   void *kvir_addr)
{
	int i = 0, ret = 0;
	struct isp_reg_unit *reg_list = NULL;

	if (!reg_data)
		return -EINVAL;

	if (user_space) {
		reg_list = (struct isp_reg_unit *)kvir_addr;
		if (!reg_list) {
			isp_log_err("the reg mem hasn't been kmap!");
			return -EPERM;
		}
	} else {
		reg_list = (struct isp_reg_unit *)reg_data;
	}

	for (i = 0; i < reg_size; i++) {
		//	isp_log_info("the reg_addr(0x%x),value=0x%x, mask=0x%x!", reg_list->reg_addr, reg_list->reg_value, reg_list->reg_mask);
		x1isp_reg_writel(reg_list->reg_addr, reg_list->reg_value, reg_list->reg_mask);
		reg_list++;
	}

	return ret;
}

int x1isp_reg_read_brust(struct isp_regs_info *regs_info)
{
	int i = 0, ret = 0;
	struct isp_reg_unit *reg_list = NULL;
	void *temp_buf = NULL;
	ulong buf_size = 0;

	if (!regs_info)
		return -EINVAL;

	buf_size = regs_info->size * sizeof(struct isp_reg_unit);
	temp_buf = kmalloc(buf_size, GFP_KERNEL);
	if (NULL == temp_buf)
		return -ENOMEM;

	reg_list = (struct isp_reg_unit *)temp_buf;
	if (copy_from_user((void *)reg_list, regs_info->data, buf_size)) {
		isp_log_err("copy isp reg from user failed!");
		kfree(temp_buf);
		temp_buf = NULL;
		return -EPERM;
	}

	for (i = 0; i < regs_info->size; i++) {
		reg_list->reg_value = x1isp_reg_readl(reg_list->reg_addr);
		//      isp_log_info("the reg_addr(0x%x),value=0x%x, mask=0x%x!", reg_list->reg_addr, reg_list->reg_value, reg_list->reg_mask);
		reg_list++;
	}

	if (copy_to_user(regs_info->data, temp_buf, buf_size)) {
		isp_log_err("copy isp reg to user failed!");
		ret = -EPERM;
	}

	kfree(temp_buf);
	temp_buf = NULL;
	return ret;
}


/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * sunxi's firmware loader driver
 * it implement search elf file from boot_package.
 *
 * Copyright (C) 2022 Allwinnertech - All Rights Reserved
 *
 * Author: lijiajian <lijiajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/string.h>
#include <linux/memblock.h>
#include <linux/elf.h>
#include <linux/io.h>
#include <linux/decompress/unlzma.h>
#include <linux/decompress/unlz4.h>
#include <linux/decompress/inflate.h>
#include "remoteproc_elf_helpers.h"

#include "sunxi_rproc_firmware.h"
#include "sunxi_rproc_load_partition.h"

struct fw_mem_info {
	const char *name;
	void *addr;
	phys_addr_t pa;
	uint32_t len;
	struct list_head list;
};
static LIST_HEAD(g_memory_fw_list);
static DEFINE_MUTEX(g_list_lock);

#ifdef DEBUG
static int sunxi_firmware_dump_data(void *buf, int len)
{
	int i, j, line;
	u8 *p = buf;

	pr_info("mem firmware:\n");

	for (i = 0; i < len; i += 16) {
		j = 0;
		if (len > (i + 16))
			line = 16;
		else
			line = len - i;
		for (j = 0; j < line; j++)
			pr_cont("%02x ", p[i + j]);

		pr_cont("\t\t|");
		for (j = 0; j < line; j++) {
			if (p[i + j] > 32 && p[i + j] < 126)
				pr_cont("%c", p[i + j]);
			else
				pr_cont(".");
		}
		pr_cont("|\n");
	}
	pr_cont("\n");

	return 0;
}
#endif

static int sunxi_check_elf_fw_len(const char *fw_name, const u8 *elf_data, uint32_t reserved_mem_len)
{
	int ret;
	struct elf32_hdr *ehdr;
	struct elf32_shdr *shdr;
	u32 i, shdr_table_end_offset, elf_size = 0;

	ehdr = (struct elf32_hdr *)elf_data;

	if (memcmp(ehdr->e_ident, ELFMAG, SELFMAG)) {
		pr_err("Image is corrupted (bad magic)\n");
		ret = -EINVAL;
		goto error_exit;
	}

	shdr_table_end_offset = ehdr->e_shoff + ehdr->e_shentsize * ehdr->e_shnum;
	if (reserved_mem_len < shdr_table_end_offset) {
		pr_err("error! the length(%u) of reserved mem is little than the end offset(%u) of section header table!\n", reserved_mem_len, shdr_table_end_offset);
		ret = -EINVAL;
		goto error_exit;
	}

	shdr = (struct elf32_shdr *)(elf_data + ehdr->e_shoff);
	for (i = 0; i < ehdr->e_shnum; i++, shdr++) {
		if (shdr->sh_type == SHT_NOBITS)
			continue;

		elf_size += shdr->sh_size;
	}

	if (reserved_mem_len < elf_size) {
		pr_err("error, the length(%u) of reserved mem is little than the elf load size(%u)!\n", reserved_mem_len, elf_size);
		ret = -EINVAL;
		goto error_exit;
	}

	pr_debug("shdr_table_end_offset: %d, elf load size: %d\n", shdr_table_end_offset, elf_size);
	return 0;

error_exit:
	pr_err("please confirm the length of reserved mem in dts is greater or equal than the size of ELF firmware file('%s')\n", fw_name);
	return ret;
}

static int __sunxi_register_memory_fw(const char *name, phys_addr_t addr, uint32_t len, bool is_elf_fw)
{
	int ret;
	struct fw_mem_info *info;

	pr_info("register mem fw('%s'), addr=%pap, len=%u\n", name, &addr, len);

	if (is_elf_fw) {
		ret = sunxi_check_elf_fw_len(name, phys_to_virt(addr), len);
		if (ret) {
			return -EINVAL;
		}
	}

	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_err("Can't kzalloc fw_mem_info.\n");
		return -ENOMEM;
	}

	info->name = name;
	info->pa = addr;
	info->len = len;
	info->addr = phys_to_virt(addr);

	mutex_lock(&g_list_lock);
	list_add(&info->list, &g_memory_fw_list);
	mutex_unlock(&g_list_lock);

	pr_debug("register mem fw('%s') success: va=0x%px\n", name, info->addr);
	return 0;
}

int sunxi_register_memory_bin_fw(const char *name, phys_addr_t addr, uint32_t len)
{
	__sunxi_register_memory_fw(name, addr, len, false);
	return 0;
}
EXPORT_SYMBOL(sunxi_register_memory_bin_fw);


int sunxi_register_memory_fw(const char *name, phys_addr_t addr, uint32_t len)
{
	__sunxi_register_memory_fw(name, addr, len, true);
	return 0;
}
EXPORT_SYMBOL(sunxi_register_memory_fw);

void sunxi_unregister_memory_fw(const char *name)
{
	int ret;
	struct fw_mem_info *pos, *tmp;

	mutex_lock(&g_list_lock);
	list_for_each_entry_safe(pos, tmp, &g_memory_fw_list, list) {
		if (!strcmp(pos->name, name)) {
			pr_info("unregister mem fw('%s'), addr=%pad,len=%u\n",
					name, &pos->pa, pos->len);

			list_del(&pos->list);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)
			ret = memblock_free(pos->pa, pos->len);
#else
			ret = memblock_phys_free(pos->pa, pos->len);
#endif
			if (ret)
				pr_err("memblock_free failed, addr: %pap, len: %d, ret: %d\n", &pos->pa, pos->len, ret);
			free_reserved_area(__va(pos->pa), __va(pos->pa + pos->len), -1, NULL);
			kfree(pos);
			mutex_unlock(&g_list_lock);
			return;
		}
	}
	mutex_unlock(&g_list_lock);
}
EXPORT_SYMBOL(sunxi_unregister_memory_fw);

static int sunxi_remove_memory_fw(struct fw_mem_info *info)
{
	int ret;

	mutex_lock(&g_list_lock);
	list_del(&info->list);
	mutex_unlock(&g_list_lock);

	pr_info("remove mem fw('%s'), addr=%pap, len=%u\n",
			info->name, &info->pa, info->len);
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)
	ret = memblock_free(info->pa, info->len);
#else
	ret = memblock_phys_free(info->pa, info->len);
#endif
	if (ret)
		pr_err("memblock_free failed, addr: %pap, len: %d, ret: %d\n", &info->pa, info->len, ret);
	free_reserved_area(__va(info->pa), __va(info->pa + info->len), -1, NULL);
	kfree(info);

	return 0;
}

static struct fw_mem_info *sunxi_find_memory_fw(const char *name)
{
	struct fw_mem_info *pos, *tmp;

	mutex_lock(&g_list_lock);
	list_for_each_entry_safe(pos, tmp, &g_memory_fw_list, list) {
		if (!strcmp(pos->name, name)) {
			mutex_unlock(&g_list_lock);
			return pos;
		}
	}
	mutex_unlock(&g_list_lock);

	return NULL;
}

static int sunxi_request_fw_from_mem(const struct firmware **fw,
			   const char *name, struct device *dev)
{
	int ret;
	u8 *fw_buffer;
	struct firmware *fw_p = NULL;
	struct fw_mem_info *info;

	info = sunxi_find_memory_fw(name);
	if (!info) {
		dev_err(dev, "Can't find firmware('%s') in memory.\n", name);
		return -ENODEV;
	}

#ifdef AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
	fw_buffer = info->addr;
#else
	fw_buffer = vmalloc(info->len);
	if (!fw_buffer) {
		dev_err(dev, "failed to alloc firmware buffer\n");
		ret = -ENOMEM;
		goto exit_with_unregister_mem_fw;
	}
#endif

	fw_p = kmalloc(sizeof(*fw_p), GFP_KERNEL);
	if (!fw_p) {
		dev_err(dev, "failed to alloc firmware struct\n");
		ret = -ENOMEM;
		goto exit_with_free_fw_buffer;
	}

#ifndef AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
	/* read data from memory */
	memcpy(fw_buffer, info->addr, info->len);
#endif

	fw_p->size = info->len;
	fw_p->priv = NULL;
	fw_p->data = fw_buffer;
	*fw = fw_p;
	ret = 0;

#ifdef DEBUG
	sunxi_firmware_dump_data(fw_buffer, 128);
#endif

#ifndef AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
	sunxi_remove_memory_fw(info);
#endif
	return 0;

exit_with_free_fw_buffer:
#ifndef AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
	vfree(fw_buffer);

exit_with_unregister_mem_fw:
#endif
	sunxi_remove_memory_fw(info);

	return ret;
}

int sunxi_request_firmware_from_memory(const struct firmware **fw, const char *name,
				       struct device *dev)
{
	int ret;

	ret = sunxi_request_fw_from_mem(fw, name, dev);
	if (ret) {
		dev_err(dev, "request firmware('%s') in memory failed, ret: %d\n", name, ret);
	}

	return ret;
}
EXPORT_SYMBOL(sunxi_request_firmware_from_memory);

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_DECOMPRESS_FW)
static int rproc_elf_find_section(const u8 *input,
					 const char *section_name, const void **piggydata_shdr)
{
	const void *shdr, *name_table_shdr;
	int i;
	const char *name_table;
	struct elf32_hdr *ehdr = (struct elf32_hdr *)input;
	u8 class = ehdr->e_ident[EI_CLASS];
	u16 shnum = elf_hdr_get_e_shnum(class, ehdr);
	u32 elf_shdr_get_size = elf_size_of_shdr(class);
	u16 shstrndx = elf_hdr_get_e_shstrndx(class, ehdr);

	shdr = input + elf_hdr_get_e_shoff(class, ehdr);
	name_table_shdr = shdr + (shstrndx * elf_shdr_get_size);
	name_table = input + elf_shdr_get_sh_offset(class, name_table_shdr);

	for (i = 0; i < shnum; i++, shdr += elf_shdr_get_size) {
		u32 name = elf_shdr_get_sh_name(class, shdr);

		if (strcmp(name_table + name, section_name))
			continue;
		*piggydata_shdr = shdr;

		return 0;
	}

	return -EINVAL;

}

static u8 *uncompress_fw(const u8 *input, struct device *dev)
{
	long pos = 0;
	u8 *compress_data;
	u8 *out;
	int ret;
	const void *shdr;
	const Elf_Shdr *piggydata_shdr;

	ret = rproc_elf_find_section(input, COMPRESSED_DATA_SECTION, &shdr);
	if (ret) {
		dev_warn(dev, "krproc_elf_find_section fail\n");
		return NULL;
	}
	piggydata_shdr = shdr;

	compress_data = (u8 *)(input + piggydata_shdr->sh_offset);

	out = (u8 *)vmalloc(CONFIG_AW_REMOTEPROC_DECOMPRESS_FW_SIZE);
	if (!out) {
		dev_warn(dev, "kmalloc outbuffer fail\n");
		return NULL;
	}

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_USE_UNLZMA)
	ret = unlzma((unsigned char *)(compress_data),
			piggydata_shdr->sh_size, NULL, NULL, out, &pos, NULL);
	if (ret) {
		dev_warn(dev, "unlzma fail:%d\n", ret);
		return NULL;
	}
#elif IS_ENABLED(CONFIG_AW_REMOTEPROC_USE_UNGZIP)
	ret = gunzip((unsigned char *)(compress_data),
			piggydata_shdr->sh_size, NULL, NULL, out, &pos, NULL);
	if (ret) {
		dev_warn(dev, "ungzip fail:%d\n", ret);
		return NULL;
	}
#elif IS_ENABLED(CONFIG_AW_REMOTEPROC_USE_UNLZ4)
	ret = unlz4((unsigned char *)(compress_data),
			piggydata_shdr->sh_size - 4, NULL, NULL, out, &pos, NULL);
	if (ret) {
		dev_warn(dev, "unlzma fail:%d\n", ret);
		return NULL;
	}
#endif

	return out;
}
#endif

int sunxi_request_firmware_from_partition(const struct firmware **fw, const char *name,
					  size_t fw_size, struct device *dev)
{
	int ret;
	u8 *fw_buffer;
#if IS_ENABLED(CONFIG_AW_REMOTEPROC_DECOMPRESS_FW)
	u8 *decompress_buffer;
#endif

	sunxi_image_header_t *ih;
	struct firmware *fw_p = NULL;

	fw_buffer = vmalloc(fw_size);
	if (!fw_buffer) {
		dev_err(dev, "failed to alloc firmware buffer\n");
		return -ENOMEM;
	}

	ret = load_from_partition(name, fw_buffer, fw_size);
	if (ret < 0) {
		dev_err(dev, "failed to load fw from partition %s\n", name);
		goto exit_with_free_fw_buffer;
	}
	fw_size = ret;

	/*
	 * When safe booting, a header is added to the rv partition,
	 * which is no longer needed when the kernel is booted again
	 */
	ih = (sunxi_image_header_t *) fw_buffer;
	if (ih->ih_magic == SUNXI_IH_MAGIC) {
		dev_info(dev,
				"rv is verified in uboot when safe booting, there is no need to check it again here\n");
		memcpy(fw_buffer, fw_buffer + ih->ih_hsize, ih->ih_psize);
	}

	ret = sunxi_check_elf_fw_len(name, fw_buffer, fw_size);
	if (ret < 0) {
		dev_err(dev, "failed to check fw from partition %s\n", name);
		goto exit_with_free_fw_buffer;
	}

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_DECOMPRESS_FW)
	decompress_buffer = uncompress_fw(fw_buffer, dev);
#endif

	fw_p = kmalloc(sizeof(*fw_p), GFP_KERNEL);
	if (!fw_p) {
		dev_err(dev, "failed to alloc firmware struct\n");
		ret = -ENOMEM;
		goto exit_with_free_fw_buffer;
	}

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_DECOMPRESS_FW)
	fw_p->size = CONFIG_AW_REMOTEPROC_DECOMPRESS_FW_SIZE;
	fw_p->data = decompress_buffer;
	vfree(fw_buffer);
#else
	fw_p->size = fw_size;
	fw_p->data = fw_buffer;
#endif
	fw_p->priv = NULL;
	*fw = fw_p;

#ifdef DEBUG
	sunxi_firmware_dump_data(fw_buffer, 128);
#endif

	return 0;

exit_with_free_fw_buffer:
	vfree(fw_buffer);
	return ret;
}
EXPORT_SYMBOL(sunxi_request_firmware_from_partition);

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Sunxi Remote processor framework
 *
 * Copyright (C) 2022 Allwinner, Inc.
 * Base on remoteproc_internal.h
 *
 * lijiajian <lijianjian@allwinner.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef SUNXI_RPROC_FIRMWARE_H
#define SUNXI_RPROC_FIRMWARE_H

#include <linux/firmware.h>
#include <linux/version.h>

#if IS_ENABLED(CONFIG_RISCV) && (LINUX_VERSION_CODE < KERNEL_VERSION(5, 10, 0))
#define AW_RPROC_SUPPORT_ZERO_COPY_MEM_FW
#endif

#if IS_ENABLED(CONFIG_AW_REMOTEPROC_DECOMPRESS_FW)
struct lzma_header {
	char signature[4];
	u32 file_size;
	u32 original_file_size;
};

#define COMPRESSED_DATA_SECTION ".piggydata"
#endif

#define SUNXI_IH_MAGIC  (0x48495741)

/* Image Header(128B) */
typedef struct sunxi_image_header {
    uint32_t ih_magic;             /* Image Header Magic Number */
    uint16_t ih_hversion;          /* Image Header Version:     */
    uint16_t ih_pversion;          /* Image Payload Version:    */
    uint16_t ih_hchksum;           /* Image Header Checksum     */
    uint16_t ih_dchksum;           /* Image Data Checksum       */
    uint32_t ih_hsize;             /* Image Header Size         */
    uint32_t ih_psize;             /* Image Payload Size        */
    uint32_t ih_tsize;             /* Image TLV Size            */
    uint32_t ih_load;              /* Image Load Address        */
    uint32_t ih_ep;                /* Image Entry Point         */
    uint32_t ih_imgattr;           /* Image Attribute           */
    uint32_t ih_nxtsecaddr;        /* Next Section Address      */
    uint8_t  ih_name[16];          /* Image Name                */
    uint32_t ih_priv[18];          /* Image Private Data        */
} sunxi_image_header_t;

int sunxi_request_firmware_from_memory(const struct firmware **fw, const char *name,
				       struct device *dev);
int sunxi_register_memory_fw(const char *name, phys_addr_t addr, uint32_t len);
int sunxi_register_memory_bin_fw(const char *name, phys_addr_t addr, uint32_t len);
void sunxi_unregister_memory_fw(const char *name);

int sunxi_request_firmware_from_partition(const struct firmware **fw, const char *name,
					  size_t fw_size, struct device *dev);

#endif /* SUNXI_RPROC_FIRMWARE_H */

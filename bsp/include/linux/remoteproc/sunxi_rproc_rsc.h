
/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * remoteproc/sunxi_remoteproc.h
 *
 * Copyright(c) 2023 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * allwinner sunxi remoteproc manager.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_REMOTEPROC_RSC_H__
#define __SUNXI_REMOTEPROC_RSC_H__

#define EXCEPTION_CAUSE_STR_LEN  64
enum SUNXI_RSC_TYPE {
	RSC_TYPE_AW_TRACE = RSC_VENDOR_START + 1,
	RSC_TYPE_AW_USER_RESOURCE,
	RSC_TYPE_AW_MISC_RSC,
};

struct fw_rsc_aw_trace {
	u32 da;
	u32 len;
	u32 reserved;
	u8 name[32];
} __packed;

struct fw_rsc_user_resource {
	//u32 type; // useless for helper
	u32 da;
	u32 len;
	u32 reserved;
	u32 src_type;
	u8 src_name[32];
} __packed;

struct fw_rsc_misc_rsc_data {
	int is_exception;
	char exception_cause[EXCEPTION_CAUSE_STR_LEN];
};

struct fw_rsc_misc_resource {
	u32 da;
	u32 len;
	u32 reserved;
	u8 name[32];
} __packed;

#endif

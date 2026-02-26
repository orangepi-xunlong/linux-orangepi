/* SPDX-License-Identifier: GPL-2.0-or-later */

/* Copyright(c) 2024 - 2027 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Driver for sunxi SD/MMC host controllers
* (C) Copyright 2024-2026 lixiang <lixiang@allwinnertech>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef _UFSHCD_DWC_H
#define _UFSHCD_DWC_H

struct ufshcd_dme_attr_val {
	u32 attr_sel;
	u32 mib_val;
	u8 peer;
};

struct pair_addr {
	u16 addr;
	u16 value;
};

int ufshcd_sunxi_link_startup_notify(struct ufs_hba *hba,
				   enum ufs_notify_change_status status);
int ufshcd_sunxi_dme_set_attrs(struct ufs_hba *hba,
			     const struct ufshcd_dme_attr_val *v, int n);
int ufshcd_sunxi_dme_get_attrs(struct ufs_hba *hba,
			     const struct ufshcd_dme_attr_val *v, int n);

#endif /* End of Header */

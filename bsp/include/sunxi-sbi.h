// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2024 Allwinner Technology Co.,Ltd. All rights reserved. */
#ifndef __SUNXI_SBI_H
#define __SUNXI_SBI_H

#if IS_ENABLED(CONFIG_AW_SBI)

#define SBI_EXT_SUNXI				0x54535251

#define SBI_EXT_SUNXI_EFUSE_WRITE		0

int sbi_efuse_write(phys_addr_t key_buf);

#endif  /* CONFIG_AW_SBI */

#endif  /* __SUNXI_SBI_H */

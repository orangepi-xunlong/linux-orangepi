/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright(c) 2015-2016 Allwinnertech Co., Ltd.
 *         http://www.allwinnertech.com
 *
 * Author: sunny <superm@allwinnertech.com>
 *
 * allwinner sunxi soc chip version and chip id manager.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __SUNXI_SMC_H
#define __SUNXI_SMC_H

#define SUNXI_OPTEE_SMC_OFFSET (0x200)

enum secure_key_type {
	CE_KEY_SELECT_SSK = 1,
	CE_KEY_SELECT_HUK,
	CE_KEY_SELECT_RSSK,
};

#if IS_ENABLED(CONFIG_AW_SBI)
extern int sbi_efuse_write(phys_addr_t key_buf);
#endif

extern int invoke_smc_fn(u32 function_id, u64 arg0, u64 arg1, u64 arg2);
extern u32 sunxi_smc_readl(phys_addr_t addr);
extern int sunxi_smc_writel(u32 value, phys_addr_t addr);
extern int arm_svc_efuse_read(phys_addr_t key_buf, phys_addr_t read_buf);
extern int arm_svc_efuse_write(phys_addr_t key_buf);
extern int sunxi_smc_probe_secure(void);
extern int sunxi_soc_is_secure(void);
extern int sunxi_smc_copy_arisc_paras(phys_addr_t dest, phys_addr_t src, u32 len);
extern int sunxi_smc_call_offset(void);
extern int  optee_probe_drm_configure(unsigned long *drm_base,
	size_t *drm_size, unsigned long  *tee_base);
extern int sunxi_optee_call_crashdump(void);
extern int smc_tee_secure_key_encrypt(char *out_buf, char *in_buf, int in_len, enum secure_key_type key);
extern int smc_tee_secure_key_decrypt(char *out_buf, char *in_buf, int len, enum secure_key_type key);

#endif  /* __SUNXI_SMC_H */

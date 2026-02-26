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

#ifndef __SUNXI_REMOTEPROC_H__
#define __SUNXI_REMOTEPROC_H__

#if IS_ENABLED(CONFIG_SUNXI_RPROC_SHARE_IRQ)
int sunxi_arch_interrupt_save(const char *name);
int sunxi_arch_interrupt_restore(const char *name);
int sunxi_arch_create_debug_dir(struct dentry *dir, const char *name);
uint32_t sunxi_rproc_get_gpio_mask_by_hwirq(int hwirq);
#endif

#define RSC_AW_TRACE 129
struct fw_rsc_aw_trace {
    u32 da;
    u32 len;
    u32 reserved;
    u8 name[32];
} __packed;

#endif

// SPDX-License-Identifier: GPL-2.0
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _LINLON_AEU_LOG_H_
#define _LINLON_AEU_LOG_H_

#include <linux/device.h>
#include <linux/debugfs.h>

void linlon_aeu_log_init(struct dentry *root);
void linlon_aeu_log_exit(void);
int linlon_aeu_log(bool read, u32 reg, u32 val);
#endif

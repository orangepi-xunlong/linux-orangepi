// SPDX-License-Identifier: GPL-2.0
/* Copyright 2024 Cix Technology Group Co., Ltd.*/
/**
 * SoC: CIX SKY1 platform
 */

#ifndef __DST_REBOOT_REASON_H__
#define __DST_REBOOT_REASON_H__

#include <linux/types.h>

void set_reboot_reason(unsigned int reboot_reason);
unsigned int get_reboot_reason(void);
void set_subtype_exception(unsigned int subtype, bool save_value);
unsigned int get_subtype_exception(void);
void plat_pm_system_reset_comm(const char *cmd);

#endif

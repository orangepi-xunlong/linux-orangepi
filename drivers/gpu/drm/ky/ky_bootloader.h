// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 Ky Co., Ltd.
 *
 */

#ifndef _KY_BOOTLOADER_H_
#define _KY_BOOTLOADER_H_

#include <linux/of_reserved_mem.h>

int ky_dpu_free_bootloader_mem(struct reserved_mem *rmem);

#endif

/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * rdr_print.h
 *
 * blackbox header file (blackbox: kernel run data recorder.)
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 * Copyright 2024 Cix Technology Group Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __BB_PRINTER_H__
#define __BB_PRINTER_H__

#include <linux/types.h>
#include "../dst_print.h"

#define BB_PN DST_PN
#define BB_ERR DST_ERR
#define BB_DBG DST_DBG

#define BB_PR_START DST_PR_START
#define BB_PR_END DST_PR_END

void rdr_print_all_ops(void);
void rdr_print_all_exc(void);

#endif /* End #define __BB_PRINTER_H__ */

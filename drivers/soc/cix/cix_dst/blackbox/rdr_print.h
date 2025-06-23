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

#define BB_PRINT_PN(args...)    pr_info("<rdr>"args)
#define BB_PRINT_ERR(args...)   pr_err("<rdr error>"args)
#define BB_PRINT_DBG(args...)   pr_debug("<rdr>"args)

#define BB_PRINT_START(args...) \
	pr_debug(">>>>>enter rdr %s: %.4d\n", __func__, __LINE__)
#define BB_PRINT_END(args...)   \
	pr_debug("<<<<<exit  rdr %s: %.4d\n", __func__, __LINE__)

void rdr_print_all_ops(void);
void rdr_print_all_exc(void);

#endif /* End #define __BB_PRINTER_H__ */

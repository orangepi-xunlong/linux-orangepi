/*
 * SPDX-License-Identifier: GPL-2.0
 * nand_test_dev.h for  SUNXI NAND .
 *
 * Copyright (C) 2016 Allwinner.
 *
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __BSP_NAND_TEST_H__
#define __BSP_NAND_TEST_H__

extern int nand_driver_test_init(void);
extern int nand_driver_test_exit(void);
extern unsigned int get_nftl_num(void);
extern unsigned int get_nftl_cap(void);
extern unsigned int get_first_nftl_cap(void);
extern unsigned int nftl_test_read(unsigned int start_sector, unsigned int len,
				   unsigned char *buf);
extern unsigned int nftl_test_write(unsigned int start_sector, unsigned int len,
				    unsigned char *buf);
extern unsigned int nftl_test_flush_write_cache(void);

#endif

/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/**
 * SPDX-License-Identifier: GPL-2.0+
 * aw_rawnand_securestorage.c
 *
 * (C) Copyright 2020 - 2021
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * cuizhikui <cuizhikui@allwinnertech.com>
 *
 */

#include <linux/mtd/aw-rawnand.h>


struct aw_nand_sec_sto rawnand_sec_sto;

int aw_rawnand_secure_storage_read(struct aw_nand_sec_sto *sec_sto,
		int item, char *buf, unsigned int len)
{
	int ret = 0;

	return ret;
}

int aw_rawnand_secure_storage_write(struct aw_nand_sec_sto *sec_sto,
		int item, char *buf, unsigned int len)
{
	int ret = 0;

	return ret;
}

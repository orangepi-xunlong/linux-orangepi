
/*
 * Copyright (c) 2007-2017 Allwinnertech Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define _NSCF_C_

#include "../nand_physic_inc.h"

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_reset_super_chip(struct _nand_super_chip_info *nsci,
			  unsigned int super_chip_no)
{
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_read_super_chip_status(struct _nand_super_chip_info *nsci,
				unsigned int super_chip_no)
{
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int is_super_chip_rb_ready(struct _nand_super_chip_info *nsci,
			   unsigned int super_chip_no)
{
	return 0;
}

/*****************************************************************************
*Name         :
*Description  :
*Parameter    :
*Return       : 0:ok  -1:fail
*Note         :
*****************************************************************************/
int nand_wait_all_rb_ready(void)
{
	int ret = 0;
	struct _nand_controller_info *nctri = g_nctri;

	while (nctri != NULL) {
		ret |= ndfc_wait_all_rb_ready(nctri);
		nctri = nctri->next;
	}
	return ret;
}

/******************************************************************************
 *
 * Copyright(c) 2016 - 2017 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 ******************************************************************************/

#ifndef __HALMAC_PCIE_REG_H__
#define __HALMAC_PCIE_REG_H__

/* PCIE PHY register */
#define CLKCAL_CTRL_PHYPARA		0x00
#define CLKCAL_SET_PHYPARA		0x20
#define CLKCAL_TRG_VAL_PHYPARA	0x21
#define PCIE_L1_BACKDOOR		0x719

/* PCIE MAC register */
#define LINK_CTRL2_REG_OFFSET	0xA0
#define GEN2_CTRL_OFFSET		0x80C
#define LINK_STATUS_REG_OFFSET	0x82

#endif/* __HALMAC_PCIE_REG_H__ */

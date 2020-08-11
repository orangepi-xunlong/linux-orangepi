/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Functions and registers to access AC200 IC.
 *
 * Copyright (C) 2019 Jernej Skrabec <jernej.skrabec@siol.net>
 */

#ifndef __LINUX_MFD_AC200_H
#define __LINUX_MFD_AC200_H

#include <linux/types.h>

struct ac200_dev;

/* interface registers (can be accessed from any page) */
#define AC200_TWI_CHANGE_TO_RSB		0x3E
#define AC200_TWI_PAD_DELAY		0xC4
#define AC200_TWI_REG_ADDR_H		0xFE

/* General registers */
#define AC200_SYS_VERSION		0x0000
#define AC200_SYS_CONTROL		0x0002
#define AC200_SYS_IRQ_ENABLE		0x0004
#define AC200_SYS_IRQ_STATUS		0x0006
#define AC200_SYS_CLK_CTL		0x0008
#define AC200_SYS_DLDO_OSC_CTL		0x000A
#define AC200_SYS_PLL_CTL0		0x000C
#define AC200_SYS_PLL_CTL1		0x000E
#define AC200_SYS_AUDIO_CTL0		0x0010
#define AC200_SYS_AUDIO_CTL1		0x0012
#define AC200_SYS_EPHY_CTL0		0x0014
#define AC200_SYS_EPHY_CTL1		0x0016
#define AC200_SYS_TVE_CTL0		0x0018
#define AC200_SYS_TVE_CTL1		0x001A

/* EPHY registers */
#define AC200_EPHY_CTL			0x6000
#define AC200_EPHY_BIST			0x6002

int ac200_reg_read(struct ac200_dev *ac200, u16 reg, u16 *value);
int ac200_reg_write(struct ac200_dev *ac200, u16 reg, u16 value);
int ac200_reg_mod(struct ac200_dev *ac200, u16 reg, u16 mask, u16 value);

#endif /* __LINUX_MFD_AC200_H */

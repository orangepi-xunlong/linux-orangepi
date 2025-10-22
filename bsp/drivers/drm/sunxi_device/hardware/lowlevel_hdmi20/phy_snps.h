/* SPDX-License-Identifier: GPL-2.0-or-later */
/*******************************************************************************
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 ******************************************************************************/
#ifndef _PHY_SNPS_H
#define _PHY_SNPS_H

int snps_phy_disconfig(void);

int snps_phy_config(void);

int snps_phy_init(void);

int snps_phy_write(u8 addr, void *data);

int snps_phy_read(u8 addr, void *data);

ssize_t snps_phy_dump(char *buf);

#endif	/* _PHY_SNPS_H */

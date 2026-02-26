/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Allwinner SoCs hdmi2.0 driver.
 *
 * Copyright (C) 2016 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */
#include <sunxi-log.h>
#include "dw_access.h"
#include "dw_mc.h"
#include "dw_fc.h"
#include "dw_i2cm.h"
#include "dw_scdc.h"

#ifndef SUPPORT_ONLY_HDMI14

int dw_scdc_read(dw_hdmi_dev_t *dev, u8 addr, u8 size, u8 *data)
{
	if (dw_i2cm_ddc_read(dev, SCDC_SLAVE_ADDRESS, 0, 0, addr, size, data)) {
		hdmi_err("%s: scdc addr 0x%x read failed!!!\n", __func__, addr);
		return -1;
	}
	return 0;
}

int dw_scdc_write(dw_hdmi_dev_t *dev, u8 addr, u8 size, u8 *data)
{
	if (dw_i2cm_ddc_write(dev, SCDC_SLAVE_ADDRESS, addr, size, data)) {
		hdmi_err("%s: scdc addr 0x%x write failed!!!\n", __func__, addr);
		return -1;
	}
	return 0;
}

int dw_scdc_set_scramble(dw_hdmi_dev_t *dev, u8 enable)
{
	u8 read_value = 0;

	if (dw_scdc_read(dev, SCDC_TMDS_CONFIG, 1, &read_value)) {
		hdmi_err("%s: scdc read address 0x%x failed!!!\n", __func__, SCDC_TMDS_CONFIG);
		return -1;
	}

	read_value = set(read_value, 0x1, enable ? 0x1 : 0x0);
	if (dw_scdc_write(dev, SCDC_TMDS_CONFIG, 1, &read_value)) {
		hdmi_err("%s: scdc write address 0x%x = 0x%x failed!!!\n",
				__func__, SCDC_TMDS_CONFIG, read_value);
		return -1;
	}
	return 0;
}

int dw_scdc_set_tmds_clk_ratio(dw_hdmi_dev_t *dev, u8 enable)
{
	u8 read_value = 0;

	if (dw_scdc_read(dev, SCDC_TMDS_CONFIG, 1, &read_value)) {
		hdmi_err("%s: scdc read address 0x%x failed!!!\n", __func__, SCDC_TMDS_CONFIG);
		return -1;
	}

	if (enable) {
		read_value |= 0x02;
		if (dw_scdc_write(dev, SCDC_TMDS_CONFIG, 1, &read_value)) {
			hdmi_err("%s: scdc write address 0x%x = 0x%x failed!!!\n",
					__func__, SCDC_TMDS_CONFIG, read_value);
			return -1;
		}

		read_value |= 0x01;
		if (dw_scdc_write(dev, SCDC_TMDS_CONFIG, 1, &read_value)) {
			hdmi_err("%s: scdc write address 0x%x = 0x%x failed!!!\n",
					__func__, SCDC_TMDS_CONFIG, read_value);
			return -1;
		}
	} else {
		read_value = 0x0;
		if (dw_scdc_write(dev, SCDC_TMDS_CONFIG, 1, &read_value)) {
			hdmi_err("%s: scdc write address 0x%x = 0x%x failed!!!\n",
					__func__, SCDC_TMDS_CONFIG, read_value);
			return -1;
		}
	}
	return 0;
}

#endif

/******************************************************************************
 *
 * Copyright(c) 2021 Realtek Corporation.
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
 *****************************************************************************/
#define _HAL_TEST_FPGA_C_
#include "../../hal_headers.h"
#include "../../../test/fpga/phl_test_fpga_def.h"
#include "hal_test_fpga_api.h"


#ifdef CONFIG_HAL_TEST_FPGA

enum rtw_hal_status
rtw_hal_fpga_rx_mac_crc_ok(struct fpga_context *fpga_ctx,
                           enum phl_band_idx band,
                           u32 *crc_ok)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	PHL_INFO("%s\n", __func__);

	hal_status = rtw_hal_mac_get_rx_cnt(fpga_ctx->hal, band, MAC_AX_RX_CRC_OK, crc_ok);

	PHL_INFO("%s: status = %d\n", __func__, hal_status);
	PHL_INFO("%s: mac crc OK count = %d\n", __func__, *crc_ok);

	return hal_status;
}

enum rtw_hal_status
rtw_hal_fpga_rx_mac_crc_err(struct fpga_context *fpga_ctx,
                            enum phl_band_idx band,
                            u32 *crc_err)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	PHL_INFO("%s\n", __func__);

	hal_status = rtw_hal_mac_get_rx_cnt(fpga_ctx->hal, band, MAC_AX_RX_CRC_FAIL, crc_err);

	PHL_INFO("%s: status = %d\n", __func__, hal_status);
	PHL_INFO("%s: mac crc error count = %d\n", __func__, *crc_err);

	return hal_status;
}

enum rtw_hal_status
rtw_hal_fpga_reset_mac_cnt(struct fpga_context *fpga_ctx,
                           enum phl_band_idx band)
{
	enum rtw_hal_status hal_status = RTW_HAL_STATUS_FAILURE;

	PHL_INFO("%s !\n", __FUNCTION__);

	hal_status = rtw_hal_mac_set_reset_rx_cnt(fpga_ctx->hal, band);
	PHL_INFO("%s: status = %d\n", __FUNCTION__, hal_status);

	return hal_status;
}

#endif /* CONFIG_HAL_TEST_FPGA */
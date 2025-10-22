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
#ifndef _HAL_TEST_FPGA_H_
#define _HAL_TEST_FPGA_H_

#ifdef CONFIG_HAL_TEST_FPGA

enum rtw_hal_status
rtw_hal_fpga_rx_mac_crc_ok(struct fpga_context *fpga_ctx,
                           enum phl_band_idx band,
                           u32 *crc_ok);

enum rtw_hal_status
rtw_hal_fpga_rx_mac_crc_err(struct fpga_context *fpga_ctx,
                            enum phl_band_idx band,
                            u32 *crc_err);

enum rtw_hal_status
rtw_hal_fpga_reset_mac_cnt(struct fpga_context *fpga_ctx,
                           enum phl_band_idx band);

#endif /* CONFIG_HAL_TEST_FPGA */

#endif /* _HAL_TEST_FPGA_H_ */

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

#ifndef _HALMAC_SDIO_88XX_H_
#define _HALMAC_SDIO_88XX_H_

#include "../halmac_api.h"

#if HALMAC_88XX_SUPPORT

HALMAC_RET_STATUS
halmac_init_sdio_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_deinit_sdio_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_cfg_rx_aggregation_88xx_sdio(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_RXAGG_CFG phalmac_rxagg_cfg
);

HALMAC_RET_STATUS
halmac_cfg_tx_agg_align_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable,
	IN u16 align_size
);

u32
halmac_reg_read_indirect_32_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset
);

HALMAC_RET_STATUS
halmac_reg_read_nbyte_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u32 halmac_offset,
	IN u32 halmac_size,
	OUT u8 *halmac_data
);

HALMAC_RET_STATUS
halmac_set_bulkout_num_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 bulkout_num
);

HALMAC_RET_STATUS
halmac_get_usb_bulkout_id_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 *halmac_buf,
	IN u32 halmac_size,
	OUT u8 *bulkout_id
);

HALMAC_RET_STATUS
halmac_sdio_cmd53_4byte_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_SDIO_CMD53_4BYTE_MODE cmd53_4byte_mode
);

HALMAC_RET_STATUS
halmac_sdio_hw_info_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_SDIO_HW_INFO pSdio_hw_info
);

VOID
halmac_config_sdio_tx_page_threshold_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_TX_PAGE_THRESHOLD_INFO pThreshold_info
);

HALMAC_RET_STATUS
halmac_convert_to_sdio_bus_offset_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	INOUT u32 *halmac_offset
);

u32
halmac_read_indirect_sdio_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u16 halmac_offset,
	IN HALMAC_IO_SIZE size
);

#endif /* HALMAC_88XX_SUPPORT */

#endif/* _HALMAC_SDIO_88XX_H_ */

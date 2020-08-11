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

#ifndef _HALMAC_INIT_88XX_H_
#define _HALMAC_INIT_88XX_H_

#include "../halmac_api.h"

#if HALMAC_88XX_SUPPORT

HALMAC_RET_STATUS
halmac_register_api_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN PHALMAC_API_REGISTRY pApi_registry
);

VOID
halmac_init_adapter_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

VOID
halmac_init_adapter_dynamic_para_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_mount_api_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_pre_init_system_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_init_system_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_init_edca_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_init_wmac_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

HALMAC_RET_STATUS
halmac_init_mac_cfg_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE mode
);

HALMAC_RET_STATUS
halmac_reset_feature_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_FEATURE_ID feature_id
);

HALMAC_RET_STATUS
halmac_verify_platform_api_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

VOID
halmac_tx_desc_checksum_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN u8 enable
);

HALMAC_RET_STATUS
halmac_pg_num_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode,
	IN PHALMAC_PG_NUM pPg_num_table
);

HALMAC_RET_STATUS
halmac_rqpn_parser_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter,
	IN HALMAC_TRX_MODE halmac_trx_mode,
	IN PHALMAC_RQPN pPwr_seq_cfg
);

VOID
halmac_init_offload_feature_state_machine_88xx(
	IN PHALMAC_ADAPTER pHalmac_adapter
);

#endif /* HALMAC_88XX_SUPPORT */

#endif/* _HALMAC_INIT_88XX_H_ */

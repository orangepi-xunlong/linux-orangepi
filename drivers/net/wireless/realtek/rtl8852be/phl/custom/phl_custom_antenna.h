/******************************************************************************
 *
 * Copyright(c) 2023 Realtek Corporation.
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
#ifndef _PHL_CUSTOM_ANTENNA_H_
#define _PHL_CUSTOM_ANTENNA_H_

#ifdef CONFIG_PHL_CUSTOM_FEATURE_ANTENNA

enum rtw_phl_custom_ant_op {
	RTW_PHL_CUSTOM_ANT_OP_EXAMPLE = 0,
	RTW_PHL_CUSTOM_ANT_OP_FRN_CNT_RST = 1,
	RTW_PHL_CUSTOM_ANT_OP_FIX_DIV_ANT = 2,
	RTW_PHL_CUSTOM_ANT_OP_MAX,
};

void rtw_phl_custom_antenna_frn_rst(struct phl_info_t *phl_info, enum phl_band_idx band_idx);
void rtw_phl_custom_antenna_example_cmd(struct phl_info_t *phl_info, enum phl_band_idx band_idx, u8 *para, u8 para_len);
void rtw_phl_custom_antenna_fix_div_ant(struct phl_info_t *phl_info, enum phl_band_idx band_idx, u8 *ant_idx, u8 para_len);

enum phl_mdl_ret_code
phl_custom_antenna_cmd_hdlr(void* dispr,
			    void* custom_ctx,
			    struct phl_msg* msg);
#else

#define rtw_phl_custom_antenna_frn_rst(_phl_info, _band_idx)
#define rtw_phl_custom_antenna_example_cmd(_phl_info, _band_idx, _para, _para_len)
#define rtw_phl_custom_antenna_fix_div_ant(_phl_info, _band_idx, _ant_idx, _para_len)
#define phl_custom_antenna_cmd_hdlr(_dispr, _custom_ctx, _msg) MDL_RET_SUCCESS


#endif
#endif
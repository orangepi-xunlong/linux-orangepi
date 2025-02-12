/******************************************************************************
 *
 * Copyright(c) 2020 Realtek Corporation.
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
#ifndef _PHL_REGULATION_H_
#define _PHL_REGULATION_H_

#define MAX_CH_NUM_GROUP 24

enum rtw_regulation_freq_group {
	FREQ_GROUP_2GHZ = 0x0,
	FREQ_GROUP_5GHZ_BAND1,
	FREQ_GROUP_5GHZ_BAND2,
	FREQ_GROUP_5GHZ_BAND3,
	FREQ_GROUP_5GHZ_BAND4,
	FREQ_GROUP_6GHZ_UNII5,
	FREQ_GROUP_6GHZ_UNII6,
	FREQ_GROUP_6GHZ_UNII7,
	FREQ_GROUP_6GHZ_UNII8,
	FREQ_GROUP_MAX
};

enum rtw_regulation_status {
	REGULATION_SUCCESS = 0x0,
	REGULATION_FAILURE,
	REGULATION_DOMAIN_MISMATCH,
	REGULATION_INVALID_2GHZ_RD,
	REGULATION_INVALID_5GHZ_RD,
	REGULATION_INVALID_DOMAIN
};

struct rtw_regulation_chplan_group {
	u32 cnt;
	struct rtw_regulation_channel ch[MAX_CH_NUM_GROUP];
};

struct rtw_domain {
	u8 code;
	u8 reason;
};

bool rtw_phl_set_regulation_info(void* phl,
	struct rtw_regulation_info *regu_info);

bool rtw_phl_regu_interface_init(void *phl);

bool rtw_phl_regu_interface_deinit(void *phl);

enum rtw_regulation_freq_group
rtw_phl_get_regu_freq_group(enum band_type band, u8 ch);

u8 rtw_phl_get_regu_country_ver_ex(
	void *phl, u8 tbl_idx);

u8 rtw_phl_get_regu_chplan_ver_ex(
	void *phl, u8 tbl_idx);

void rtw_phl_get_6g_regulatory_info(void *phl,
	u8 domain, u8 *dm_code, u8 *regulation, u8 *ch_idx,
	u8 tbl_idx);

void rtw_phl_get_chdef_6g(void *phl,
	u8 ch_idx, struct chdef_6ghz *chdef, u8 tbl_idx);

u8 rtw_phl_get_cat6g_by_country_ex(void *phl,
	char *country, u8 tbl_idx);

/* legacy api, will be removed */
u8 rtw_phl_get_cat6g_by_country(char *cntry);
u8 rtw_phl_get_regu_country_ver(void);
u8 rtw_phl_get_regu_chplan_ver(void);

#endif /* _PHL_REGULATION_H_ */

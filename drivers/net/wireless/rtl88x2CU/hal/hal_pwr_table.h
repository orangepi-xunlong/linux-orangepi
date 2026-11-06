/******************************************************************************
 *
 * Copyright(c) 2007 - 2022 Realtek Corporation.
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
#ifndef _HAL_PWR_TABLE_H_
#define _HAL_PWR_TABLE_H_

#if CONFIG_TXPWR_LIMIT
#define TXPWR_LMT_RS_CCK	0
#define TXPWR_LMT_RS_OFDM	1
#define TXPWR_LMT_RS_HT		2
#define TXPWR_LMT_RS_VHT	3
#define TXPWR_LMT_RS_NUM	4

#define TXPWR_LMT_RS_NUM_2G	4 /* CCK, OFDM, HT, VHT */
#define TXPWR_LMT_RS_NUM_5G	3 /* OFDM, HT, VHT */

#define MAX_2_4G_BANDWIDTH_NUM	2
#define MAX_5G_BANDWIDTH_NUM	4

extern const char *const _txpwr_lmt_rs_str[];
#define txpwr_lmt_rs_str(rs) (((rs) >= TXPWR_LMT_RS_NUM) ? _txpwr_lmt_rs_str[TXPWR_LMT_RS_NUM] : _txpwr_lmt_rs_str[(rs)])

struct lmt_reg_exc {
	_list list;
	char country[2];
	u8 domain;
	char reg_name[0];
};

struct lmt_2g_t {
	s8 v[MAX_2_4G_BANDWIDTH_NUM]
		[TXPWR_LMT_RS_NUM_2G]
		[CENTER_CH_2G_NUM]
		[MAX_TX_COUNT];
};

#if CONFIG_IEEE80211_BAND_5GHZ
struct lmt_5g_t {
	s8 v[MAX_5G_BANDWIDTH_NUM]
		[TXPWR_LMT_RS_NUM_5G]
		[CENTER_CH_5G_ALL_NUM]
		[MAX_TX_COUNT];
};
#endif

struct lmt_reg {
	_list list;

	struct lmt_2g_t *lmt_2g;

	#if CONFIG_IEEE80211_BAND_5GHZ
	struct lmt_5g_t *lmt_5g;
	#endif

	char name[];
};

struct txpwr_lmt_tb_t {
	_mutex lock;

	_list reg_exc_list;
	u8 reg_exc_num;

	_list reg_list;
	u8 reg_num;

	u8 lmt_2g_cck_ofdm_state;
	#if CONFIG_IEEE80211_BAND_5GHZ
	u8 lmt_5g_cck_ofdm_state;
	u8 lmt_5g_20_40_ref;
	#endif

	char *cur_reg_names[BAND_MAX];
	int cur_reg_names_len[BAND_MAX];
};

struct hal_com_data;

void hal_txpwr_lmt_reg_exc_add_with_nlen(struct hal_com_data *hal_data, const char *country, u8 domain, const char *reg_name, u32 nlen);
void hal_txpwr_lmt_reg_exc_add(struct hal_com_data *hal_data, const char *country, u8 domain, const char *reg_name);
struct lmt_reg_exc *hal_txpwr_lmt_reg_exc_search(struct hal_com_data *hal_data, const char *country, u8 domain);
void hal_txpwr_lmt_reg_exc_list_free(struct hal_com_data *hal_data);
void dump_txpwr_lmt_reg_exc_list(void *sel, struct hal_com_data *hal_data);

void hal_txpwr_lmt_reg_add_with_nlen(struct hal_com_data *hal_data, const char *name, u32 nlen
	, u8 band, u8 bw, u8 tlrs, u8 ntx_idx, u8 ch_idx, s8 lmt);
void hal_txpwr_lmt_reg_add(struct hal_com_data *hal_data, const char *name
	, u8 band, u8 bw, u8 tlrs, u8 ntx_idx, u8 ch_idx, s8 lmt);
struct lmt_reg *_hal_txpwr_lmt_reg_get_by_name(struct hal_com_data *hal_data, const char *name);
struct lmt_reg *hal_txpwr_lmt_reg_get_by_name(struct hal_com_data *hal_data, const char *name);
void hal_txpwr_set_current_lmt_regs(struct hal_com_data *hal_data, enum band_type band, char *names, int names_len);
void hal_txpwr_get_current_lmt_regs(struct hal_com_data *hal_data, enum band_type band, char **names, int *names_len);
bool hal_txpwr_is_current_lmt_reg(struct hal_com_data *hal_data, enum band_type band, const char *name);
void hal_txpwr_lmt_reg_list_free(struct hal_com_data *hal_data);

void hal_txpwr_lmt_tb_init(struct hal_com_data *hal_data);
void hal_txpwr_lmt_tb_deinit(struct hal_com_data *hal_data);

void dump_txpwr_lmt(void *sel, _adapter *adapter);
#endif /* CONFIG_TXPWR_LIMIT */

#endif /* _HAL_PWR_TABLE_H_ */

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
#define _HAL_PWR_TABLE_C_

#include <drv_types.h>
#include <hal_data.h>

#if CONFIG_TXPWR_LIMIT
const char *const _txpwr_lmt_rs_str[] = {
	[TXPWR_LMT_RS_CCK]	= "CCK",
	[TXPWR_LMT_RS_OFDM]	= "OFDM",
	[TXPWR_LMT_RS_HT]	= "HT",
	[TXPWR_LMT_RS_VHT]	= "VHT",
	[TXPWR_LMT_RS_NUM]	= "UNKNOWN",
};

void hal_txpwr_lmt_reg_exc_add_with_nlen(struct hal_com_data *hal_data, const char *country, u8 domain, const char *reg_name, u32 nlen)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg_exc *ent;

	if (!reg_name || !nlen) {
		rtw_warn_on(1);
		goto exit;
	}

	ent = rtw_zmalloc(sizeof(struct lmt_reg_exc) + nlen + 1);
	if (!ent)
		goto exit;

	_rtw_init_listhead(&ent->list);
	if (country)
		_rtw_memcpy(ent->country, country, 2);
	ent->domain = domain;
	_rtw_memcpy(ent->reg_name, reg_name, nlen);

	_rtw_mutex_lock_interruptible(&tb->lock);

	rtw_list_insert_tail(&ent->list, &tb->reg_exc_list);
	tb->reg_exc_num++;

	_rtw_mutex_unlock(&tb->lock);

exit:
	return;
}

void hal_txpwr_lmt_reg_exc_add(struct hal_com_data *hal_data, const char *country, u8 domain, const char *reg_name)
{
	hal_txpwr_lmt_reg_exc_add_with_nlen(hal_data, country, domain, reg_name, strlen(reg_name));
}

static struct lmt_reg_exc *_hal_txpwr_lmt_reg_exc_search(struct hal_com_data *hal_data, const char *country, u8 domain)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg_exc *ent;
	_list *cur, *head;
	u8 match = 0;

	head = &tb->reg_exc_list;
	cur = get_next(head);

	while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
		u8 has_country;

		ent = LIST_CONTAINOR(cur, struct lmt_reg_exc, list);
		cur = get_next(cur);
		has_country = (ent->country[0] == '\0' && ent->country[1] == '\0') ? 0 : 1;

		/* entry has country condition to match */
		if (has_country) {
			if (!country)
				continue;
			if (ent->country[0] != country[0]
				|| ent->country[1] != country[1])
				continue;
		}

		/* entry has domain condition to match */
		if (ent->domain != 0xFF) {
			if (domain == 0xFF)
				continue;
			if (ent->domain != domain)
				continue;
		}

		match = 1;
		break;
	}

	if (match)
		return ent;
	else
		return NULL;
}

struct lmt_reg_exc *hal_txpwr_lmt_reg_exc_search(struct hal_com_data *hal_data, const char *country, u8 domain)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg_exc *ent;

	_rtw_mutex_lock_interruptible(&tb->lock);
	ent = _hal_txpwr_lmt_reg_exc_search(hal_data, country, domain);
	_rtw_mutex_unlock(&tb->lock);

	return ent;
}

void hal_txpwr_lmt_reg_exc_list_free(struct hal_com_data *hal_data)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg_exc *ent;
	_list *cur, *head;

	_rtw_mutex_lock_interruptible(&tb->lock);

	head = &tb->reg_exc_list;
	cur = get_next(head);

	while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
		ent = LIST_CONTAINOR(cur, struct lmt_reg_exc, list);
		cur = get_next(cur);
		rtw_list_delete(&ent->list);
		rtw_mfree((u8 *)ent, sizeof(struct lmt_reg_exc) + strlen(ent->reg_name) + 1);
	}
	tb->reg_exc_num = 0;

	_rtw_mutex_unlock(&tb->lock);
}

static void _dump_txpwr_lmt_reg_exc_list(void *sel, struct hal_com_data *hal_data)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg_exc *ent;
	_list *cur, *head;

	RTW_PRINT_SEL(sel, "reg_exc_num:%u\n", tb->reg_exc_num);

	if (!tb->reg_exc_num)
		goto exit;

	RTW_PRINT_SEL(sel, "%-7s %-6s %-8s\n", "country", "domain", "reg_name");

	head = &tb->reg_exc_list;
	cur = get_next(head);

	while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
		u8 has_country;

		ent = LIST_CONTAINOR(cur, struct lmt_reg_exc, list);
		cur = get_next(cur);
		has_country = (ent->country[0] == '\0' && ent->country[1] == '\0') ? 0 : 1;

		RTW_PRINT_SEL(sel, "     %c%c   0x%02x %s\n"
			, has_country ? ent->country[0] : '-'
			, has_country ? ent->country[1] : '-'
			, ent->domain
			, ent->reg_name
		);
	}

exit:
	return;
}

void dump_txpwr_lmt_reg_exc_list(void *sel, struct hal_com_data *hal_data)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;

	_rtw_mutex_lock_interruptible(&tb->lock);
	_dump_txpwr_lmt_reg_exc_list(sel, hal_data);
	_rtw_mutex_unlock(&tb->lock);
}

/* search matcing first, if not found, alloc one */
void hal_txpwr_lmt_reg_add_with_nlen(struct hal_com_data *hal_data, const char *name, u32 nlen
	, u8 band, u8 bw, u8 tlrs, u8 ntx_idx, u8 ch_idx, s8 lmt)
{
	struct hal_spec_t *hal_spec = &hal_data->hal_spec;
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg *ent;
	_list *cur, *head;
	s8 pre_lmt;
	u8 ch;

	if (!name || !nlen) {
		rtw_warn_on(1);
		goto exit;
	}

	_rtw_mutex_lock_interruptible(&tb->lock);

	/* search for existed entry */
	head = &tb->reg_list;
	cur = get_next(head);
	while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
		ent = LIST_CONTAINOR(cur, struct lmt_reg, list);
		cur = get_next(cur);

		if (strlen(ent->name) == nlen
			&& _rtw_memcmp(ent->name, name, nlen) == _TRUE)
			goto chk_lmt_band;
	}

	/* alloc new one */
	ent = rtw_zvmalloc(sizeof(struct lmt_reg) + nlen + 1);
	if (!ent)
		goto release_lock;

	_rtw_init_listhead(&ent->list);
	_rtw_memcpy(ent->name, name, nlen);
	rtw_list_insert_tail(&ent->list, &tb->reg_list);
	tb->reg_num++;

chk_lmt_band:
	if (band == BAND_ON_24G && !ent->lmt_2g) {
		ent->lmt_2g = rtw_vmalloc(sizeof(*ent->lmt_2g));
		if (ent->lmt_2g) {
			u8 j, k, l, m;

			for (j = 0; j < MAX_2_4G_BANDWIDTH_NUM; ++j)
				for (k = 0; k < TXPWR_LMT_RS_NUM_2G; ++k)
					for (m = 0; m < CENTER_CH_2G_NUM; ++m)
						for (l = 0; l < MAX_TX_COUNT; ++l)
							ent->lmt_2g->v[j][k][m][l] = hal_spec->txgi_max;
		} else
			goto release_lock;
	}
	#if CONFIG_IEEE80211_BAND_5GHZ
	else if (band == BAND_ON_5G && !ent->lmt_5g) {
		ent->lmt_5g = rtw_vmalloc(sizeof(*ent->lmt_5g));
		if (ent->lmt_5g) {
			u8 j, k, l, m;

			for (j = 0; j < MAX_5G_BANDWIDTH_NUM; ++j)
				for (k = 0; k < TXPWR_LMT_RS_NUM_5G; ++k)
					for (m = 0; m < CENTER_CH_5G_ALL_NUM; ++m)
						for (l = 0; l < MAX_TX_COUNT; ++l)
							ent->lmt_5g->v[j][k][m][l] = hal_spec->txgi_max;
		} else
			goto release_lock;
	}
	#endif

	if (band == BAND_ON_2_4G) {
		pre_lmt = ent->lmt_2g->v[bw][tlrs][ch_idx][ntx_idx];
		ch = ch_idx + 1;
	}
	#if CONFIG_IEEE80211_BAND_5GHZ
	else if (band == BAND_ON_5G) {
		pre_lmt = ent->lmt_5g->v[bw][tlrs - 1][ch_idx][ntx_idx];
		ch = center_ch_5g_all[ch_idx];
	}
	#endif
	else
		goto release_lock;

	if (pre_lmt != hal_spec->txgi_max)
		RTW_PRINT("duplicate txpwr_lmt for [%s][%s][%s][%s][%uT][%d]\n"
			, name, band_str(band), ch_width_str(bw), txpwr_lmt_rs_str(tlrs), ntx_idx + 1, ch);

	lmt = rtw_min(pre_lmt, lmt);
	if (band == BAND_ON_2_4G)
		ent->lmt_2g->v[bw][tlrs][ch_idx][ntx_idx] = lmt;
	#if CONFIG_IEEE80211_BAND_5GHZ
	else if (band == BAND_ON_5G)
		ent->lmt_5g->v[bw][tlrs - 1][ch_idx][ntx_idx] = lmt;
	#endif

	if (0)
		RTW_PRINT("%s, %4s, %6s, %7s, %uT, ch%3d = %d\n"
			, name, band_str(band), ch_width_str(bw), txpwr_lmt_rs_str(tlrs), ntx_idx + 1
			, ch, lmt);

release_lock:
	_rtw_mutex_unlock(&tb->lock);

exit:
	return;
}

void hal_txpwr_lmt_reg_add(struct hal_com_data *hal_data, const char *name
	, u8 band, u8 bw, u8 tlrs, u8 ntx_idx, u8 ch_idx, s8 lmt)
{
	hal_txpwr_lmt_reg_add_with_nlen(hal_data, name, strlen(name)
		, band, bw, tlrs, ntx_idx, ch_idx, lmt);
}

struct lmt_reg *_hal_txpwr_lmt_reg_get_by_name(struct hal_com_data *hal_data, const char *name)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg *ent;
	_list *cur, *head;
	u8 found = 0;

	head = &tb->reg_list;
	cur = get_next(head);

	while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
		ent = LIST_CONTAINOR(cur, struct lmt_reg, list);
		cur = get_next(cur);

		if (strcmp(ent->name, name) == 0) {
			found = 1;
			break;
		}
	}

	if (found)
		return ent;
	return NULL;
}

struct lmt_reg *hal_txpwr_lmt_reg_get_by_name(struct hal_com_data *hal_data, const char *name)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg *ent;

	_rtw_mutex_lock_interruptible(&tb->lock);
	ent = _hal_txpwr_lmt_reg_get_by_name(hal_data, name);
	_rtw_mutex_unlock(&tb->lock);

	return ent;
}

static void hal_txpwr_clear_current_lmt_reg_names(struct hal_com_data *hal_data, enum band_type band)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;

	if (band >= BAND_MAX)
		return;

	if (tb->cur_reg_names[band]) {
		rtw_mfree(tb->cur_reg_names[band], tb->cur_reg_names_len[band]);
		tb->cur_reg_names[band] = NULL;
	}
	tb->cur_reg_names_len[band] = 0;
}

static void hal_txpwr_clear_all_current_lmt_reg_names(struct hal_com_data *hal_data)
{
	u8 band;

	for (band = 0; band < BAND_MAX; band++)
		hal_txpwr_clear_current_lmt_reg_names(hal_data, band);
}

void hal_txpwr_set_current_lmt_regs(struct hal_com_data *hal_data, enum band_type band, char *names, int names_len)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;

	if (band >= BAND_MAX)
		return;

	_rtw_mutex_lock_interruptible(&tb->lock);

	hal_txpwr_clear_current_lmt_reg_names(hal_data, band);

	if (names && names_len) {
		tb->cur_reg_names[band] = rtw_malloc(names_len);
		if (tb->cur_reg_names[band]) {
			_rtw_memcpy(tb->cur_reg_names[band], names, names_len);
			tb->cur_reg_names_len[band] = names_len;
		}
	}

	_rtw_mutex_unlock(&tb->lock);
}

void hal_txpwr_get_current_lmt_regs(struct hal_com_data *hal_data, enum band_type band, char **names, int *names_len)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;

	if (!names || !names_len)
		return;

	*names = NULL;
	*names_len = 0;

	if (band >= BAND_MAX)
		return;

	_rtw_mutex_lock_interruptible(&tb->lock);

	if (tb->cur_reg_names[band] && tb->cur_reg_names_len[band]) {
		*names = rtw_malloc(tb->cur_reg_names_len[band]);
		if (*names) {
			_rtw_memcpy(*names, tb->cur_reg_names[band], tb->cur_reg_names_len[band]);
			*names_len = tb->cur_reg_names_len[band];
		}
	}

	_rtw_mutex_unlock(&tb->lock);
}

bool hal_txpwr_is_current_lmt_reg(struct hal_com_data *hal_data, enum band_type band, const char *name)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	const char *reg_names, *pos;
	int reg_names_len;

	if (band < BAND_MAX) {
		reg_names = tb->cur_reg_names[band];
		reg_names_len = tb->cur_reg_names_len[band];
		if (reg_names) {
			ustrs_for_each_str(reg_names, reg_names_len, pos) {
				if (strcmp(name, pos) == 0)
					return true;
			}
		}
	}

	return false;
}

void hal_txpwr_lmt_reg_list_free(struct hal_com_data *hal_data)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	struct lmt_reg *ent;
	_list *cur, *head;

	_rtw_mutex_lock_interruptible(&tb->lock);

	head = &tb->reg_list;
	cur = get_next(head);

	while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
		ent = LIST_CONTAINOR(cur, struct lmt_reg, list);
		cur = get_next(cur);
		rtw_list_delete(&ent->list);
		if (ent->lmt_2g)
			rtw_vmfree(ent->lmt_2g, sizeof(*ent->lmt_2g));
		#if CONFIG_IEEE80211_BAND_5GHZ
		if (ent->lmt_5g)
			rtw_vmfree(ent->lmt_5g, sizeof(*ent->lmt_5g));
		#endif
		rtw_vmfree(ent, sizeof(struct lmt_reg) + strlen(ent->name) + 1);
	}
	tb->reg_num = 0;

	hal_txpwr_clear_all_current_lmt_reg_names(hal_data);

	_rtw_mutex_unlock(&tb->lock);
}

void hal_txpwr_lmt_tb_init(struct hal_com_data *hal_data)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;

	_rtw_mutex_init(&tb->lock);
	_rtw_init_listhead(&tb->reg_exc_list);
	_rtw_init_listhead(&tb->reg_list);
}

void hal_txpwr_lmt_tb_deinit(struct hal_com_data *hal_data)
{
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;

	hal_txpwr_lmt_reg_exc_list_free(hal_data);
	hal_txpwr_lmt_reg_list_free(hal_data);
	_rtw_mutex_free(&tb->lock);
}

void dump_txpwr_lmt(void *sel, _adapter *adapter)
{
#define TMP_STR_LEN 16
	struct hal_com_data *hal_data = GET_HAL_DATA(adapter);
	struct hal_spec_t *hal_spec = GET_HAL_SPEC(adapter);
	struct txpwr_lmt_tb_t *tb = &hal_data->txpwr_lmt_tb;
	char fmt[16];
	char tmp_str[TMP_STR_LEN];
	s8 *lmt_idx = NULL;
	int bw, band, ch_num, tlrs, ntx_idx, rs, i, path;
	u8 ch, n, rfpath_num;

	_rtw_mutex_lock_interruptible(&tb->lock);

	_dump_txpwr_lmt_reg_exc_list(sel, hal_data);
	RTW_PRINT_SEL(sel, "\n");

	if (!tb->reg_num)
		goto release_lock;

	lmt_idx = rtw_malloc(sizeof(s8) * RF_PATH_MAX * tb->reg_num);
	if (!lmt_idx) {
		RTW_ERR("%s alloc fail\n", __func__);
		goto release_lock;
	}

	RTW_PRINT_SEL(sel, "lmt_2g_cck_ofdm_state:0x%02x\n", tb->lmt_2g_cck_ofdm_state);
	#if CONFIG_IEEE80211_BAND_5GHZ
	if (IS_HARDWARE_TYPE_JAGUAR_ALL(adapter)) {
		RTW_PRINT_SEL(sel, "lmt_5g_cck_ofdm_state:0x%02x\n", tb->lmt_5g_cck_ofdm_state);
		RTW_PRINT_SEL(sel, "lmt_5g_20_40_ref:0x%02x\n", tb->lmt_5g_20_40_ref);
	}
	#endif
	RTW_PRINT_SEL(sel, "\n");

	for (band = BAND_ON_2_4G; band <= BAND_ON_5G; band++) {
		if (!hal_is_band_support(adapter, band))
			continue;

		rfpath_num = (band == BAND_ON_2_4G ? hal_spec->rfpath_num_2g : hal_spec->rfpath_num_5g);

		for (bw = 0; bw < MAX_5G_BANDWIDTH_NUM; bw++) {

			if (bw >= CHANNEL_WIDTH_160)
				break;
			if (band == BAND_ON_2_4G && bw >= CHANNEL_WIDTH_80)
				break;

			if (band == BAND_ON_2_4G)
				ch_num = CENTER_CH_2G_NUM;
			#if CONFIG_IEEE80211_BAND_5GHZ
			else if (band == BAND_ON_5G)
				ch_num = center_chs_5g_num(bw);
			#endif
			else
				ch_num = 0;

			if (ch_num == 0) {
				rtw_warn_on(1);
				break;
			}

			for (tlrs = TXPWR_LMT_RS_CCK; tlrs < TXPWR_LMT_RS_NUM; tlrs++) {

				if (band == BAND_ON_2_4G && tlrs == TXPWR_LMT_RS_VHT)
					continue;
				if (band == BAND_ON_5G && tlrs == TXPWR_LMT_RS_CCK)
					continue;
				if (bw > CHANNEL_WIDTH_20 && (tlrs == TXPWR_LMT_RS_CCK || tlrs == TXPWR_LMT_RS_OFDM))
					continue;
				if (bw > CHANNEL_WIDTH_40 && tlrs == TXPWR_LMT_RS_HT)
					continue;
				if (tlrs == TXPWR_LMT_RS_VHT && !IS_HARDWARE_TYPE_JAGUAR_ALL(adapter))
					continue;

				for (ntx_idx = RF_1TX; ntx_idx < MAX_TX_COUNT; ntx_idx++) {
					struct lmt_reg *ent;
					_list *cur, *head;

					if (ntx_idx + 1 > hal_data->max_tx_cnt)
						continue;

					/* bypass CCK multi-TX is not defined */
					if (tlrs == TXPWR_LMT_RS_CCK && ntx_idx > RF_1TX) {
						if (band == BAND_ON_2_4G
							&& !(tb->lmt_2g_cck_ofdm_state & (TXPWR_LMT_HAS_CCK_1T << ntx_idx)))
							continue;
					}

					/* bypass OFDM multi-TX is not defined */
					if (tlrs == TXPWR_LMT_RS_OFDM && ntx_idx > RF_1TX) {
						if (band == BAND_ON_2_4G
							&& !(tb->lmt_2g_cck_ofdm_state & (TXPWR_LMT_HAS_OFDM_1T << ntx_idx)))
							continue;
						#if CONFIG_IEEE80211_BAND_5GHZ
						if (band == BAND_ON_5G
							&& !(tb->lmt_5g_cck_ofdm_state & (TXPWR_LMT_HAS_OFDM_1T << ntx_idx)))
							continue;
						#endif
					}

					/* bypass 5G 20M, 40M pure reference */
					#if CONFIG_IEEE80211_BAND_5GHZ
					if (band == BAND_ON_5G && (bw == CHANNEL_WIDTH_20 || bw == CHANNEL_WIDTH_40)) {
						if (tb->lmt_5g_20_40_ref == TXPWR_LMT_REF_HT_FROM_VHT) {
							if (tlrs == TXPWR_LMT_RS_HT)
								continue;
						} else if (tb->lmt_5g_20_40_ref == TXPWR_LMT_REF_VHT_FROM_HT) {
							if (tlrs == TXPWR_LMT_RS_VHT && bw <= CHANNEL_WIDTH_40)
								continue;
						}
					}
					#endif

					/* choose n-SS mapping rate section to get lmt diff value */
					if (tlrs == TXPWR_LMT_RS_CCK)
						rs = CCK;
					else if (tlrs == TXPWR_LMT_RS_OFDM)
						rs = OFDM;
					else if (tlrs == TXPWR_LMT_RS_HT)
						rs = HT_1SS + ntx_idx;
					else if (tlrs == TXPWR_LMT_RS_VHT)
						rs = VHT_1SS + ntx_idx;
					else {
						RTW_ERR("%s invalid tlrs %u\n", __func__, tlrs);
						continue;
					}

					RTW_PRINT_SEL(sel, "[%s][%s][%s][%uT]\n"
						, band_str(band)
						, ch_width_str(bw)
						, txpwr_lmt_rs_str(tlrs)
						, ntx_idx + 1
					);

					/* header for limit in db */
					RTW_PRINT_SEL(sel, "%3s ", "ch");

					head = &tb->reg_list;
					cur = get_next(head);
					while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
						ent = LIST_CONTAINOR(cur, struct lmt_reg, list);
						cur = get_next(cur);
						if ((band == BAND_ON_24G && !ent->lmt_2g)
							#if CONFIG_IEEE80211_BAND_5GHZ
							|| (band == BAND_ON_5G && !ent->lmt_5g)
							#endif
						)
							continue;

						sprintf(fmt, "%%%zus%%s ", strlen(ent->name) >= 6 ? 1 : 6 - strlen(ent->name));
						snprintf(tmp_str, TMP_STR_LEN, fmt
							, hal_txpwr_is_current_lmt_reg(hal_data, band, ent->name) ? "*" : ""
							, ent->name);
						_RTW_PRINT_SEL(sel, "%s", tmp_str);
					}
					sprintf(fmt, "%%%zus%%s ", strlen(txpwr_lmt_str(TXPWR_LMT_WW)) >= 6 ? 1 : 6 - strlen(txpwr_lmt_str(TXPWR_LMT_WW)));
					snprintf(tmp_str, TMP_STR_LEN, fmt
						, hal_txpwr_is_current_lmt_reg(hal_data, band, txpwr_lmt_str(TXPWR_LMT_WW)) ? "*" : ""
						, txpwr_lmt_str(TXPWR_LMT_WW));
					_RTW_PRINT_SEL(sel, "%s", tmp_str);

					/* header for limit offset */
					for (path = 0; path < RF_PATH_MAX; path++) {
						if (path >= rfpath_num)
							break;
						_RTW_PRINT_SEL(sel, "|");
						head = &tb->reg_list;
						cur = get_next(head);
						while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
							ent = LIST_CONTAINOR(cur, struct lmt_reg, list);
							cur = get_next(cur);
							if ((band == BAND_ON_24G && !ent->lmt_2g)
								#if CONFIG_IEEE80211_BAND_5GHZ
								|| (band == BAND_ON_5G && !ent->lmt_5g)
								#endif
							)
								continue;

							_RTW_PRINT_SEL(sel, "%3c "
								, hal_txpwr_is_current_lmt_reg(hal_data, band, ent->name) ? rf_path_char(path) : ' ');
						}
						_RTW_PRINT_SEL(sel, "%3c "
								, hal_txpwr_is_current_lmt_reg(hal_data, band, txpwr_lmt_str(TXPWR_LMT_WW)) ? rf_path_char(path) : ' ');
					}
					_RTW_PRINT_SEL(sel, "\n");

					for (n = 0; n < ch_num; n++) {
						s8 lmt;
						s8 lmt_offset;
						u8 base;

						if (band == BAND_ON_2_4G)
							ch = n + 1;
						#if CONFIG_IEEE80211_BAND_5GHZ
						else if (band == BAND_ON_5G)
							ch = center_chs_5g(bw, n);
						#endif
						else
							ch = 0;

						if (ch == 0) {
							rtw_warn_on(1);
							break;
						}

						/* dump limit in dBm */
						RTW_PRINT_SEL(sel, "%3u ", ch);
						head = &tb->reg_list;
						cur = get_next(head);
						while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
							ent = LIST_CONTAINOR(cur, struct lmt_reg, list);
							cur = get_next(cur);
							if ((band == BAND_ON_24G && !ent->lmt_2g)
								#if CONFIG_IEEE80211_BAND_5GHZ
								|| (band == BAND_ON_5G && !ent->lmt_5g)
								#endif
							)
								continue;

							lmt = phy_get_txpwr_lmt(adapter, ent->name, band, bw, tlrs, ntx_idx, ch, 0);
							txpwr_idx_get_dbm_str(lmt, hal_spec->txgi_max, hal_spec->txgi_pdbm, strlen(ent->name), tmp_str, TMP_STR_LEN);
							_RTW_PRINT_SEL(sel, "%s ", tmp_str);
						}
						lmt = phy_get_txpwr_lmt(adapter, txpwr_lmt_str(TXPWR_LMT_WW), band, bw, tlrs, ntx_idx, ch, 0);
						txpwr_idx_get_dbm_str(lmt, hal_spec->txgi_max, hal_spec->txgi_pdbm, strlen(txpwr_lmt_str(TXPWR_LMT_WW)), tmp_str, TMP_STR_LEN);
						_RTW_PRINT_SEL(sel, "%s ", tmp_str);

						/* dump limit offset of each path */
						for (path = RF_PATH_A; path < RF_PATH_MAX; path++) {
							if (path >= rfpath_num)
								break;

							base = phy_get_target_txpwr(adapter, band, path, rs);

							_RTW_PRINT_SEL(sel, "|");
							head = &tb->reg_list;
							cur = get_next(head);
							i = 0;
							while ((rtw_end_of_queue_search(head, cur)) == _FALSE) {
								ent = LIST_CONTAINOR(cur, struct lmt_reg, list);
								cur = get_next(cur);
								if ((band == BAND_ON_24G && !ent->lmt_2g)
									#if CONFIG_IEEE80211_BAND_5GHZ
									|| (band == BAND_ON_5G && !ent->lmt_5g)
									#endif
								)
									continue;

								lmt_offset = phy_get_txpwr_lmt_diff(adapter, ent->name, band, bw, path, rs, tlrs, ntx_idx, ch, 0);
								if (lmt_offset == hal_spec->txgi_max) {
									*(lmt_idx + i * RF_PATH_MAX + path) = hal_spec->txgi_max;
									_RTW_PRINT_SEL(sel, "%3s ", "NA");
								} else {
									*(lmt_idx + i * RF_PATH_MAX + path) = lmt_offset + base;
									_RTW_PRINT_SEL(sel, "%3d ", lmt_offset);
								}
								i++;
							}
							lmt_offset = phy_get_txpwr_lmt_diff(adapter, txpwr_lmt_str(TXPWR_LMT_WW), band, bw, path, rs, tlrs, ntx_idx, ch, 0);
							if (lmt_offset == hal_spec->txgi_max)
								_RTW_PRINT_SEL(sel, "%3s ", "NA");
							else
								_RTW_PRINT_SEL(sel, "%3d ", lmt_offset);

						}

						/* compare limit_idx of each path, print 'x' when mismatch */
						if (rfpath_num > 1) {
							for (i = 0; i < tb->reg_num; i++) {
								for (path = 0; path < RF_PATH_MAX; path++) {
									if (path >= rfpath_num)
										break;
									if (*(lmt_idx + i * RF_PATH_MAX + path) != *(lmt_idx + i * RF_PATH_MAX + ((path + 1) % rfpath_num)))
										break;
								}
								if (path >= rfpath_num)
									_RTW_PRINT_SEL(sel, " ");
								else
									_RTW_PRINT_SEL(sel, "x");
							}
						}
						_RTW_PRINT_SEL(sel, "\n");

					}
					RTW_PRINT_SEL(sel, "\n");
				}
			} /* loop for rate sections */
		} /* loop for bandwidths */
	} /* loop for bands */

	if (lmt_idx)
		rtw_mfree(lmt_idx, sizeof(s8) * RF_PATH_MAX * tb->reg_num);

release_lock:
	_rtw_mutex_unlock(&tb->lock);
}
#endif /* CONFIG_TXPWR_LIMIT */
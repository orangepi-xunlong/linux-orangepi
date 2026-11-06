/******************************************************************************
 *
 * Copyright(c) 2007 - 2017  Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in the
 * file called LICENSE.
 *
 * Contact Information:
 * wlanfae <wlanfae@realtek.com>
 * Realtek Corporation, No. 2, Innovation Road II, Hsinchu Science Park,
 * Hsinchu 300, Taiwan.
 *
 * Larry Finger <Larry.Finger@lwfinger.net>
 *
 *****************************************************************************/

#include "mp_precomp.h"
#if (DM_ODM_SUPPORT_TYPE == ODM_WIN)
#if RT_PLATFORM == PLATFORM_MACOSX
#include "phydm_precomp.h"
#else
#include "../phydm_precomp.h"
#endif
#else
#include "../../phydm_precomp.h"
#endif

#if (RTL8822C_SUPPORT == 1)

void _backup_bb_registers_8822c(
	void *dm_void,
	u32 *reg,
	u32 *reg_backup,
	u32 reg_num)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i;

	for (i = 0; i < reg_num; i++) {
		reg_backup[i] = odm_get_bb_reg(dm, reg[i], MASKDWORD);

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] Backup BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
	}
}

void _reload_bb_registers_8822c(
	void *dm_void,
	u32 *reg,
	u32 *reg_backup,
	u32 reg_num)

{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i;

	for (i = 0; i < reg_num; i++) {
		odm_set_bb_reg(dm, reg[i], MASKDWORD, reg_backup[i]);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] Reload BB 0x%x = 0x%x\n",
		       reg[i], reg_backup[i]);
	}
}



u8 _halrf_driver_rate_to_tssi_rate_8822c(
	void *dm_void, u8 rate)
{
	u8 tssi_rate = 0;

	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (rate == MGN_1M)
		tssi_rate = 0;
	else if (rate == MGN_2M)
		tssi_rate = 1;
	else if (rate == MGN_5_5M)
		tssi_rate = 2;
	else if (rate == MGN_11M)
		tssi_rate = 3;
	else if (rate == MGN_6M)
		tssi_rate = 4;
	else if (rate == MGN_9M)
		tssi_rate = 5;
	else if (rate == MGN_12M)
		tssi_rate = 6;
	else if (rate == MGN_18M)
		tssi_rate = 7;
	else if (rate == MGN_24M)
		tssi_rate = 8;
	else if (rate == MGN_36M)
		tssi_rate = 9;
	else if (rate == MGN_48M)
		tssi_rate = 10;
	else if (rate == MGN_54M)
		tssi_rate = 11;
	else if (rate >= MGN_MCS0 && rate <= MGN_VHT4SS_MCS9)
		tssi_rate = rate - MGN_MCS0 + 12;
	else
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s not exit tx rate\n", __func__);

	return tssi_rate;
}

u8 _halrf_tssi_rate_to_driver_rate_8822c(
	void *dm_void, u8 rate)
{
	u8 driver_rate = 0;

	struct dm_struct *dm = (struct dm_struct *)dm_void;

	if (rate == 0)
		driver_rate = MGN_1M;
	else if (rate == 1)
		driver_rate = MGN_2M;
	else if (rate == 2)
		driver_rate = MGN_5_5M;
	else if (rate == 3)
		driver_rate = MGN_11M;
	else if (rate == 4)
		driver_rate = MGN_6M;
	else if (rate == 5)
		driver_rate = MGN_9M;
	else if (rate == 6)
		driver_rate = MGN_12M;
	else if (rate == 7)
		driver_rate = MGN_18M;
	else if (rate == 8)
		driver_rate = MGN_24M;
	else if (rate == 9)
		driver_rate = MGN_36M;
	else if (rate == 10)
		driver_rate = MGN_48M;
	else if (rate == 11)
		driver_rate = MGN_54M;
	else if (rate >= 12 && rate <= 83)
		driver_rate = rate + MGN_MCS0 - 12;
	else
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s not exit tx rate\n", __func__);

	return driver_rate;
}

void _halrf_calculate_txagc_codeword_8822c(
	void *dm_void, u16 *tssi_value,  s16 *txagc_value)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u32 i, mcs7 = 19;

	for (i = 0; i < TSSI_CODE_NUM; i++) {
		txagc_value[i] =
			((tssi_value[i] - tssi->tssi_codeword[mcs7]) / TSSI_TXAGC_DIFF) & 0x7f;

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s txagc_value[%d](0x%x) = ((tssi_value[%d](%d) - tssi_value[mcs7](%d)) / %d) & 0x7f\n",
		       __func__, i, txagc_value[i], i, tssi_value[i], tssi_value[mcs7], TSSI_TXAGC_DIFF);
	}
#endif
}

u32 _halrf_get_efuse_tssi_offset_8822c(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel;
	u32 offset = 0;
	u32 offset_index = 0;
	/*u8 bandwidth = *dm->band_width;*/

	if (channel >= 1 && channel <= 2)
		offset_index = 6;
	else if (channel >= 3 && channel <= 5)
		offset_index = 7;
	else if (channel >= 6 && channel <= 8)
		offset_index = 8;
	else if (channel >= 9 && channel <= 11)
		offset_index = 9;
	else if (channel >= 12 && channel <= 14)
		offset_index = 10;
	else if (channel >= 36 && channel <= 40)
		offset_index = 11;
	else if (channel >= 42 && channel <= 48)
		offset_index = 12;
	else if (channel >= 50 && channel <= 58)
		offset_index = 13;
	else if (channel >= 60 && channel <= 64)
		offset_index = 14;
	else if (channel >= 100 && channel <= 104)
		offset_index = 15;
	else if (channel >= 106 && channel <= 112)
		offset_index = 16;
	else if (channel >= 114 && channel <= 120)
		offset_index = 17;
	else if (channel >= 122 && channel <= 128)
		offset_index = 18;
	else if (channel >= 130 && channel <= 136)
		offset_index = 19;
	else if (channel >= 138 && channel <= 144)
		offset_index = 20;
	else if (channel >= 149 && channel <= 153)
		offset_index = 21;
	else if (channel >= 155 && channel <= 161)
		offset_index = 22;
	else if (channel >= 163 && channel <= 169)
		offset_index = 23;
	else if (channel >= 171 && channel <= 177)
		offset_index = 24;

	offset = (u32)tssi->tssi_efuse[path][offset_index];

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"=====>%s channel=%d offset_index(Chn Group)=%d offset=%d\n",
		__func__, channel, offset_index, offset);

	return offset;
}

s8 _halrf_get_kfree_tssi_offset_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel;
	s8 offset = 0;
	u32 offset_index = 0;

	if (channel >= 1 && channel <= 14)
		offset_index = 0;
	else if (channel >= 36 && channel <= 64)
		offset_index = 1;
	else if (channel >= 100 && channel <= 144)
		offset_index = 2;
	else if (channel >= 149 && channel <= 177)
		offset_index = 3;

	return offset = tssi->tssi_kfree_efuse[0][offset_index];
}

void _halrf_calculate_set_thermal_codeword_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u8 channel = *dm->channel, i, thermal;
	s8 j;
	u8 rate = phydm_get_tx_rate(dm);
	u32 thermal_offset_tmp = 0, thermal_offset_index = 0x10, thermal_tmp = 0xff, tmp;
	s8 thermal_offset[64] = {0};
	u8 thermal_up_a[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_a[DELTA_SWINGIDX_SIZE] = {0};
	u8 thermal_up_b[DELTA_SWINGIDX_SIZE] = {0}, thermal_down_b[DELTA_SWINGIDX_SIZE] = {0};

	tssi->thermal[RF_PATH_A] = 0xff;
	tssi->thermal[RF_PATH_B] = 0xff;

	if (cali_info->txpowertrack_control == 4) {
		tmp = thermal_offset_index;
		i = 0;
		while (i < 64) {
			thermal_offset_tmp = 0;
			for (j = 0; j < 23; j = j + 6)
				thermal_offset_tmp = thermal_offset_tmp | ((thermal_offset[i++] & 0x3f) << j);

			thermal_offset_tmp = thermal_offset_tmp | ((tmp++ & 0xff) << 24);

			odm_set_bb_reg(dm, 0x18f4, MASKDWORD, thermal_offset_tmp);
			odm_set_bb_reg(dm, 0x41f4, MASKDWORD, thermal_offset_tmp);

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "===>%s write addr:0x%x value=0x%08x\n",
			       __func__, 0x18f4, thermal_offset_tmp);

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "===>%s write addr:0x%x value=0x%08x\n",
			       __func__, 0x41f4, thermal_offset_tmp);
		}

		odm_set_bb_reg(dm, R_0x1c20, 0x000fc000, 0x0);
		odm_set_bb_reg(dm, R_0x1c20, 0x03f00000, 0x0);

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "======>%s TSSI Calibration Mode Set Thermal Table == 0 return!!!\n", __func__);
		return;
	}

	if (rate == ODM_MGN_1M || rate == ODM_MGN_2M || rate == ODM_MGN_5_5M || rate == ODM_MGN_11M) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_2g_cck_a_p, sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_2g_cck_a_n, sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_2g_cck_b_p, sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_2g_cck_b_n, sizeof(thermal_down_b));
	} else if (channel >= 1 && channel <= 14) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_2ga_p, sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_2ga_n, sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_2gb_p, sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_2gb_n, sizeof(thermal_down_b));
	} else if (channel >= 36 && channel <= 64) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[0], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[0], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[0], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[0], sizeof(thermal_down_b));
	} else if (channel >= 100 && channel <= 144) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[1], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[1], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[1], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[1], sizeof(thermal_down_b));
	} else if (channel >= 149 && channel <= 177) {
		odm_move_memory(dm, thermal_up_a, cali_info->delta_swing_table_idx_5ga_p[2], sizeof(thermal_up_a));
		odm_move_memory(dm, thermal_down_a, cali_info->delta_swing_table_idx_5ga_n[2], sizeof(thermal_down_a));
		odm_move_memory(dm, thermal_up_b, cali_info->delta_swing_table_idx_5gb_p[2], sizeof(thermal_up_b));
		odm_move_memory(dm, thermal_down_b, cali_info->delta_swing_table_idx_5gb_n[2], sizeof(thermal_down_b));
	}

	/*path s0*/
	odm_efuse_logical_map_read(dm, 1, 0xd0, &thermal_tmp);

	thermal = (u8)thermal_tmp;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "======>%s channel=%d thermal_pahtA=0x%x cali_info->txpowertrack_control=%d\n",
	       __func__, channel, thermal, cali_info->txpowertrack_control);

	if (thermal == 0xff) {
		tmp = thermal_offset_index;
		i = 0;
		while (i < 64) {
			thermal_offset_tmp = 0;
			for (j = 0; j < 23; j = j + 6)
				thermal_offset_tmp = thermal_offset_tmp | ((thermal_offset[i++] & 0x3f) << j);

			thermal_offset_tmp = thermal_offset_tmp | ((tmp++ & 0xff) << 24);

			odm_set_bb_reg(dm, 0x18f4, MASKDWORD, thermal_offset_tmp);

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "===>%s write addr:0x%x value=0x%08x\n",
			       __func__, 0x18f4, thermal_offset_tmp);
		}

		odm_set_bb_reg(dm, R_0x1c20, 0x000fc000, 0x0);

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "======>%s thermal A=0x%x!!!\n", __func__, thermal);
	}

	tssi->thermal[RF_PATH_A] = thermal;

	if (thermal != 0xff) {
		odm_set_bb_reg(dm, R_0x1c20, 0x000fc000, (thermal & 0x3f));

		i = 0;
		for (j = thermal; j >= 0; j--) {
			if (i < DELTA_SWINGIDX_SIZE)
				thermal_offset[j] = thermal_down_a[i++];
			else
				thermal_offset[j] = thermal_down_a[DELTA_SWINGIDX_SIZE - 1];
		}

		i = 0;
		for (j = thermal; j < 64; j++) {
			if (i < DELTA_SWINGIDX_SIZE)
				thermal_offset[j] = -1 * thermal_up_a[i++];
			else
				thermal_offset[j] = -1 * thermal_up_a[DELTA_SWINGIDX_SIZE - 1];
		}

		for (i = 0; i < 64; i = i + 4) {
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "======>%s thermal_offset[%.2d]=%.2x %.2x %.2x %.2x\n",
			       __func__, i, thermal_offset[i] & 0xff, thermal_offset[i + 1] & 0xff,
			       thermal_offset[i + 2] & 0xff, thermal_offset[i + 3] & 0xff);
		}

		tmp = thermal_offset_index;
		i = 0;
		while (i < 64) {
			thermal_offset_tmp = 0;
			for (j = 0; j < 23; j = j + 6)
				thermal_offset_tmp = thermal_offset_tmp | ((thermal_offset[i++] & 0x3f) << j);

			thermal_offset_tmp = thermal_offset_tmp | ((tmp++ & 0xff) << 24);

			odm_set_bb_reg(dm, 0x18f4, MASKDWORD, thermal_offset_tmp);

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "======>%s write addr:0x%x value=0x%08x\n",
			       __func__, 0x18f4, thermal_offset_tmp);
		}
	} else 
		odm_set_bb_reg(dm, R_0x1c20, 0x000fc000, 0x0);


	/*path s1*/
	odm_memory_set(dm, thermal_offset, 0, sizeof(thermal_offset));
	odm_efuse_logical_map_read(dm, 1, 0xd1, &thermal_tmp);

	thermal = (u8)thermal_tmp;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "======>%s channel=%d thermal pahtB=0x%x cali_info->txpowertrack_control=%d\n",
	       __func__, channel, thermal, cali_info->txpowertrack_control);

	if (thermal == 0xff) {
		tmp = thermal_offset_index;
		i = 0;
		while (i < 64) {
			thermal_offset_tmp = 0;
			for (j = 0; j < 23; j = j + 6)
				thermal_offset_tmp = thermal_offset_tmp | ((thermal_offset[i++] & 0x3f) << j);

			thermal_offset_tmp = thermal_offset_tmp | ((tmp++ & 0xff) << 24);

			odm_set_bb_reg(dm, 0x41f4, MASKDWORD, thermal_offset_tmp);

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "===>%s write addr:0x%x value=0x%08x\n",
			       __func__, 0x41f4, thermal_offset_tmp);
		}

		odm_set_bb_reg(dm, R_0x1c20, 0x03f00000, 0x0);

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "======>%s thermal=0x%x return!!!\n", __func__, thermal);
	}

	tssi->thermal[RF_PATH_B] = thermal;

	if (thermal != 0xff) {
		odm_set_bb_reg(dm, R_0x1c20, 0x03f00000, (thermal & 0x3f));

		i = 0;
		for (j = thermal; j >= 0; j--) {
			if (i < DELTA_SWINGIDX_SIZE)
				thermal_offset[j] = thermal_down_b[i++];
			else
				thermal_offset[j] = thermal_down_b[DELTA_SWINGIDX_SIZE - 1];
		}

		i = 0;
		for (j = thermal; j < 64; j++) {
			if (i < DELTA_SWINGIDX_SIZE)
				thermal_offset[j] = -1 * thermal_up_b[i++];
			else
				thermal_offset[j] = -1 * thermal_up_b[DELTA_SWINGIDX_SIZE - 1];
		}

		for (i = 0; i < 64; i = i + 4) {
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "======>%s thermal_offset[%.2d]=%.2x %.2x %.2x %.2x\n",
			       __func__, i, thermal_offset[i] & 0xff, thermal_offset[i + 1] & 0xff,
			       thermal_offset[i + 2] & 0xff, thermal_offset[i + 3] & 0xff);
		}

		tmp = thermal_offset_index;
		i = 0;
		while (i < 64) {
			thermal_offset_tmp = 0;
			for (j = 0; j < 23; j = j + 6)
				thermal_offset_tmp = thermal_offset_tmp | ((thermal_offset[i++] & 0x3f) << j);

			thermal_offset_tmp = thermal_offset_tmp | ((tmp++ & 0xff) << 24);

			odm_set_bb_reg(dm, 0x41f4, MASKDWORD, thermal_offset_tmp);

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "======>%s write addr:0x%x value=0x%08x\n",
			       __func__, 0x41f4, thermal_offset_tmp);
		}
	} else
		odm_set_bb_reg(dm, R_0x1c20, 0x03f00000, 0x0);
}

void _halrf_set_txagc_codeword_8822c(
	void *dm_void, s16 *tssi_value)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i, j, k = 0, tssi_value_tmp;

	/*power by rate table (tssi codeword)*/
	for (i = 0x3a00; i <= R_0x3a50; i = i + 4) {
		tssi_value_tmp = 0;

		for (j = 0; j < 31; j = j + 8)
			tssi_value_tmp = tssi_value_tmp | ((tssi_value[k++] & 0x7f) << j);

		odm_set_bb_reg(dm, i, MASKDWORD, tssi_value_tmp);

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s write addr:0x%x value=0x%08x\n",
		       __func__, i, tssi_value_tmp);
	}
#endif
}

void _halrf_set_efuse_kfree_offset_8822c(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	s32 offset = 0;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "===>%s\n", __func__);

	/*path s0*/
	/*2G CCK*/
	offset = (_halrf_get_efuse_tssi_offset_8822c(dm, 3) +
	 	_halrf_get_kfree_tssi_offset_8822c(dm)) & 0xff;
	odm_set_bb_reg(dm, R_0x18e8, 0x01fe0000, (u32)offset);
	
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s write addr:0x%x value=0x%08x\n",
		       __func__, R_0x18e8, offset);

	/*2G & 5G OFDM*/
	offset = (_halrf_get_efuse_tssi_offset_8822c(dm, 19) +
	 	_halrf_get_kfree_tssi_offset_8822c(dm)) & 0xff;
	odm_set_bb_reg(dm, R_0x18a8, 0xff000000, (u32)offset);

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s write addr:0x%x value=0x%08x\n",
		       __func__, R_0x18a8, offset);

	/*path s1*/
	/*2G CCK*/
	offset = (_halrf_get_efuse_tssi_offset_8822c(dm, 3) +
	 	_halrf_get_kfree_tssi_offset_8822c(dm)) & 0xff;
	odm_set_bb_reg(dm, R_0x1ef0, 0x0001fe00, (u32)offset);

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s write addr:0x%x value=0x%08x\n",
		       __func__, R_0x1ef0, offset);

	/*2G & 5G OFDM*/
	offset = (_halrf_get_efuse_tssi_offset_8822c(dm, 19) +
	 	_halrf_get_kfree_tssi_offset_8822c(dm)) & 0xff;
	odm_set_bb_reg(dm, R_0x1eec, 0x3fc00000, (u32)offset);

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s write addr:0x%x value=0x%08x\n",
		       __func__, R_0x1eec, offset);
#endif
}

void _halrf_tssi_init_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	_halrf_calculate_set_thermal_codeword_8822c(dm);
	halrf_calculate_tssi_codeword_8822c(dm);
	/*_halrf_calculate_txagc_codeword_8822c(dm, tssi->tssi_codeword, tssi->txagc_codeword);*/
	/*_halrf_set_txagc_codeword_8822c(dm, tssi->txagc_codeword);*/
	halrf_set_tssi_codeword_8822c(dm, tssi->tssi_codeword);
	/*_halrf_set_efuse_kfree_offset_8822c(dm);*/
}

void _halrf_tssi_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 channel = *dm->channel;

	/*path s0*/
	if (channel >= 1 && channel <= 14) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);
		/*odm_set_bb_reg(dm, R_0x1860, 0x00007000, 0x4);*/
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x706f0044);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c24, 0x07f80000, 0x20);
		odm_set_bb_reg(dm, R_0x18ec, 0x00c00000, 0x2);
		odm_set_bb_reg(dm, R_0x1834, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x18e8, 0x00000001, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a8, 0x00000003, 0x2);
		odm_set_bb_reg(dm, R_0x18ec, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x1880, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a8, 0x00007ffc, 0x1266);
		odm_set_bb_reg(dm, R_0x18a8, 0x00ff8000, 0x000);
		odm_set_bb_reg(dm, R_0x1880, 0x7fc00000, 0x000);
		/*odm_set_bb_reg(dm, R_0x18a8, 0xff000000, 0x00);*/
		/*odm_set_bb_reg(dm, R_0x18e8, 0x01fe0000, 0x00);*/
		odm_set_bb_reg(dm, R_0x18e8, 0x000003fe, 0x110);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x7f, 0x00002, 0x1);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x65, 0x03000, 0x3);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x67, 0x0000c, 0x3);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x67, 0x000c0, 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x6e, 0x001e0, 0x0);
		odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x180c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x1800, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x1804, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x1e7c, 0x00800000, 0x0);
		odm_set_bb_reg(dm, R_0x1800, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1804, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x18ec, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x18ec, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a0, 0x0000007f, 0x00);
	} else {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);
		/*odm_set_bb_reg(dm, R_0x1860, 0x00007000, 0x2);*/
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0042);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0042);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0042);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0042);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x706f0042);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c24, 0x07f80000, 0x20);
		odm_set_bb_reg(dm, R_0x18ec, 0x00c00000, 0x2);
		odm_set_bb_reg(dm, R_0x1834, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x18e8, 0x00000001, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a8, 0x00000003, 0x2);
		odm_set_bb_reg(dm, R_0x1880, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a8, 0x00007ffc, 0x1266);
		odm_set_bb_reg(dm, R_0x18a8, 0x00ff8000, 0x000);
		/*odm_set_bb_reg(dm, R_0x18a8, 0xff000000, 0x00);*/
		odm_set_bb_reg(dm, R_0x18e8, 0x000003fe, 0x110);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x7f, 0x00100, 0x1);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x65, 0x03000, 0x3);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x67, 0x00003, 0x3);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x67, 0x00030, 0x2);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x6f, 0x001e0, 0x0);
		odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x180c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x1800, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x1804, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x1e7c, 0x00800000, 0x0);
		odm_set_bb_reg(dm, R_0x1800, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1804, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x18ec, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x18ec, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a0, 0x0000007f, 0x00);
	}

	/*path s1*/
	if (channel >= 1 && channel <= 14) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);
		/*odm_set_bb_reg(dm, R_0x1860, 0x00007000, 0x4);*/
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x701f0044);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x702f0044);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x703f0044);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x704f0044);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x706f0044);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70fb8041);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1d04, 0x000ff000, 0x20);
		odm_set_bb_reg(dm, R_0x41ec, 0x00c00000, 0x2);
		odm_set_bb_reg(dm, R_0x4134, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x41e8, 0x00000001, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a8, 0x00000003, 0x2);
		odm_set_bb_reg(dm, R_0x41ec, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x4180, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x1eec, 0x00001fff, 0x1266);
		odm_set_bb_reg(dm, R_0x1eec, 0x003fe000, 0x000);
		odm_set_bb_reg(dm, R_0x4180, 0x7fc00000, 0x000);
		/*odm_set_bb_reg(dm, R_0x1eec, 0x3fc00000, 0x00);*/
		/*odm_set_bb_reg(dm, R_0x41e8, 0x01fe0000, 0x00);*/
		odm_set_bb_reg(dm, R_0x1ef0, 0x000001ff, 0x110);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x7f, 0x00002, 0x1);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x65, 0x03000, 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x67, 0x0000c, 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x67, 0x000c0, 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x6e, 0x001e0, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x4100, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x4104, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x4100, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x4104, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x41ec, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x41ec, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x00800000, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x1);
		/*odm_set_bb_reg(dm, R_0x1c20, 0x03f00000, 0x26);*/
	} else {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);
		/*odm_set_bb_reg(dm, R_0x1860, 0x00007000, 0x2);*/
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x701f0042);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x702f0042);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x703f0042);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x704f0042);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x706f0042);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70fb8041);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1d04, 0x000ff000, 0x20);
		odm_set_bb_reg(dm, R_0x41ec, 0x00c00000, 0x2);
		odm_set_bb_reg(dm, R_0x4134, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x41e8, 0x00000001, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a8, 0x00000003, 0x2);
		odm_set_bb_reg(dm, R_0x4180, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x1eec, 0x00001fff, 0x1266);
		odm_set_bb_reg(dm, R_0x1eec, 0x003fe000, 0x000);
		/*odm_set_bb_reg(dm, R_0x1eec, 0x3fc00000, 0x00);*/
		odm_set_bb_reg(dm, R_0x1ef0, 0x000001ff, 0x110);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x7f, 0x00100, 0x1);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x65, 0x03000, 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x67, 0x00003, 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x67, 0x00030, 0x2);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x6f, 0x001e0, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x4100, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x4104, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x4100, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x4104, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x41ec, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x41ec, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x00800000, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x1);
		/*odm_set_bb_reg(dm, R_0x1c20, 0x03f00000, 0x26);*/
	}
}

void _halrf_thermal_init_8822c(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	_halrf_calculate_set_thermal_codeword_8822c(dm);
	/*halrf_calculate_tssi_codeword_8822c(dm);*/
	/*_halrf_calculate_txagc_codeword_8822c(dm, tssi->tssi_codeword, tssi->txagc_codeword);*/
	/*_halrf_set_txagc_codeword_8822c(dm, tssi->txagc_codeword);*/
	/*halrf_set_tssi_codeword_8822c(dm, tssi->tssi_codeword);*/
	/*_halrf_set_efuse_kfree_offset_8822c(dm);*/
#endif
}

void halrf_calculate_tssi_codeword_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	
	u8 i, rate;
	u8 channel = *dm->channel, bandwidth = *dm->band_width;
	u32 big_a, small_a, slope, db_temp;
	s32 samll_b = 64;
	/*u32 big_a_reg[2] = {0x18a8, 0x1eec};*/
	/*u32 big_a_bit_mask[2] = {0x7ffc, 0x1fff};*/
	
	big_a = odm_get_bb_reg(dm, 0x18a8, 0x7ffc);
	
	if (big_a == 0) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "======>%s big_a = %d rf_path=%d return !!!\n",
		       __func__, big_a, RF_PATH_A);
		return;
	}
	
	big_a = (big_a * 100) / 128;		/* 100 * big_a */
	small_a = 434295 / big_a;		/* 1000 * small_a */
	slope = 1000000 / small_a;			/* 1000 * slope */
	
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "======>%s 0x18a8[14:2] = 0x%x(%d) 100*big_a(%d) = 0x18a8[14:2] / 128 path=%d\n",
	       __func__, odm_get_bb_reg(dm, 0x18a8, 0x7ffc),
	       odm_get_bb_reg(dm, 0x18a8, 0x7ffc), big_a, RF_PATH_A);
	
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "1000 * small_a(%d) = 434295 / big_a(%d)  1000*slope(%d) = 1000000/small_a path=%d\n",
	       small_a, big_a, slope, RF_PATH_A);
	
	for (i = 0; i < TSSI_CODE_NUM; i++) {
		rate = _halrf_tssi_rate_to_driver_rate_8822c(dm, i);
		db_temp = (u32)phydm_get_tx_power_dbm(dm, RF_PATH_A, rate, bandwidth, channel);

		db_temp = db_temp * slope;
		db_temp = db_temp / 1000 + samll_b;

		if (db_temp > 0xff)
			db_temp = 0xff;
		else if ((s32)db_temp < 0)
			db_temp = 0x0;

		tssi->tssi_codeword[i] = (u16)(db_temp);
	
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "phydm_get_tx_power_dbm = %d, rate=0x%x(%d) bandwidth=%d channel=%d rf_path=%d\n",
			phydm_get_tx_power_dbm(dm, RF_PATH_A, rate, bandwidth, channel),
			rate, rate, bandwidth, channel, RF_PATH_A);
	
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "tssi_codeword[%d] = 0x%x(%d)\n",
			i, tssi->tssi_codeword[i], tssi->tssi_codeword[i]);
	}

}

void halrf_set_tssi_codeword_8822c(
	void *dm_void, u16 *tssi_value)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u32 i, j, k = 0, tssi_value_tmp;

	/*power by rate table (tssi codeword)*/
	for (i = 0x3a54; i <= 0x3aa4; i = i + 4) {
		tssi_value_tmp = 0;

		for (j = 0; j < 31; j = j + 8)
			tssi_value_tmp = tssi_value_tmp | ((tssi_value[k++] & 0xff) << j);

		odm_set_bb_reg(dm, i, MASKDWORD, tssi_value_tmp);

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s write addr:0x%x value=0x%08x\n",
		       __func__, i, tssi_value_tmp);
	}
}

void halrf_tssi_dck_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	u8 channel = *dm->channel, i, j, k;

	u32 reg = 0, dck_check;
	s32 reg_tmp = 0;
	u32 bb_reg[8] = {R_0x1800, R_0x4100, R_0x820, R_0x1e2c, R_0x1d08, 
			R_0x1c3c, R_0x2dbc, R_0x1e70};
	u32 bb_reg_backup[8] = {0};
	u32 backup_num = 8;

	u32 tssi_setting[2] = {R_0x1830, R_0x4130};
	u32 dc_offset[2] = {R_0x189c, R_0x419c};
	u32 path_setting[2] = {R_0x1800, R_0x4100};
	u32 tssi_counter[2] = {R_0x18a4, R_0x41a4};
	u32 tssi_enalbe[2] = {R_0x180c, R_0x410c};
	u32 debug_port[2] = {0x930, 0xb30};

	u32 addr_d[2] = {0x18a8, 0x1eec};
	u32 addr_d_bitmask[2] = {0xff000000, 0x3fc00000};
	u32 addr_cck_d[2] = {R_0x18e8, R_0x1ef0};
	u32 addr_cck_d_bitmask[2] = {0x01fe0000, 0x0001fe00};

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] ======>%s channel=%d\n",
		__func__, channel);
	
	/*odm_acquire_spin_lock(dm, RT_IQK_SPINLOCK);*/
	rf->is_tssi_in_progress = 1;

	_backup_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);

	for (i = 0; i < MAX_PATH_NUM_8822C; i++) {
		odm_set_bb_reg(dm, addr_cck_d[i], addr_cck_d_bitmask[i], 0x0);
		odm_set_bb_reg(dm, addr_d[i], addr_d_bitmask[i], 0x0);
	}

	halrf_disable_tssi_8822c(dm);

	for (i = 0; i < 2 ; i++){
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"[TSSI] ============== Path - %d ==============\n", i);

		if (channel >= 1 && channel <= 14) {
			for (k = 0; k < 3; k++) {
				odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);

				odm_set_bb_reg(dm, R_0x1d58, 0x00000008, 0x1);
				odm_set_bb_reg(dm, R_0x1d58, 0x00000ff0, 0xff);
				odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x2);

				/*odm_set_bb_reg(dm, R_0x1860, 0x00007000, 0x4);*/
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x700b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x701f0044);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x702f0044);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x703f0044);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x704f0044);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x705b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x706f0044);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x707b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x708b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x709b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70ab8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70bb8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70cb8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70db8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70eb8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70fb8041);

				odm_set_bb_reg(dm, tssi_counter[i], 0xe0000000, 0x0);
				ODM_delay_us(200);
				
				odm_set_rf_reg(dm, i, RF_0x7f, 0x00002, 0x1);
				odm_set_rf_reg(dm, i, RF_0x65, 0x03000, 0x3);
				odm_set_rf_reg(dm, i, RF_0x67, 0x0000c, 0x3);
				odm_set_rf_reg(dm, i, RF_0x67, 0x000c0, 0x0);
				odm_set_rf_reg(dm, i, RF_0x6e, 0x001e0, 0x0);
				
				odm_set_bb_reg(dm, tssi_enalbe[i], 0x08000000, 0x1);
				odm_set_bb_reg(dm, tssi_enalbe[i], 0x40000000, 0x1);
				odm_set_bb_reg(dm, R_0x1d08, 0x00000001, 0x1);
				odm_set_bb_reg(dm, R_0x1ca4, 0x00000001, 0x1);
				//ODM_delay_us(100);
				odm_set_bb_reg(dm, R_0x1b00, 0x00000006, i);
				odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x3f);
				odm_set_rf_reg(dm, i, RF_0xde, 0x10000, 0x1);
				odm_set_rf_reg(dm, i, RF_0x56, 0x000ff, 0x0);

				btc_set_gnt_wl_bt_8822c(dm, true);

				odm_set_bb_reg(dm, dc_offset[i], 0x0003ff00, 0xc00);
				odm_set_bb_reg(dm, R_0x820, 0x00000003, i + 1);
				odm_set_bb_reg(dm, R_0x1e2c, MASKDWORD, 0xe4e40000);
				odm_set_bb_reg(dm, R_0x1e28, 0x0000000f, 0x3);
				odm_set_bb_reg(dm, path_setting[i], 0x000fffff, 0x33312);
				odm_set_bb_reg(dm, path_setting[i], 0x80000000, 0x1);

				odm_set_bb_reg(dm, R_0x1e70, 0x00000004, 0x1);
				//ODM_delay_us(100);
				odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x1);
				//ODM_delay_us(100);
				
				/*odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x930);*/
				phydm_set_bb_dbg_port(dm, DBGPORT_PRI_2, debug_port[i]);
				odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x0);
				odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x1);				
				/*reg = odm_get_bb_reg(dm, R_0x2dbc, 0x000003ff);*/
				reg = phydm_get_bb_dbg_port_val(dm) & 0x000003ff;
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x2dbc[9:0]=0x%x\n", reg);
				phydm_release_bb_dbg_port(dm);

				reg_tmp = reg;
				reg = 1024 - (((reg_tmp - 512) * 4) & 0x000003ff) + 0;
				odm_set_bb_reg(dm, dc_offset[i], 0x0003ff00, (reg & 0x03ff));
				
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x%x[17:8]=0x%x reg=0x%x\n",
					dc_offset[i], odm_get_bb_reg(dm, dc_offset[i], 0x0003ff00), reg);
				
				for (j = 0; j < 3; j++) {
					/*odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x930);*/
					phydm_set_bb_dbg_port(dm, DBGPORT_PRI_2, debug_port[i]);
					odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x0);
					odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x1);
					/*dck_check = odm_get_bb_reg(dm, R_0x2dbc, 0x000003ff);*/
					dck_check = phydm_get_bb_dbg_port_val(dm) & 0x000003ff;
					phydm_release_bb_dbg_port(dm);

					if (dck_check < 0x1ff) {
						if (reg >= 0x3fb)
							reg = 0x3ff;
						else
							reg = reg + 4;
						odm_set_bb_reg(dm, dc_offset[i], 0x0003ff00, reg);
						RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x2dbc[23:12]=0x%x < 0x1ff Set 0x%x retry=%d\n",
							dck_check, reg, j);
					} else if (dck_check > 0x202) {
						if (reg <= 4)
							reg = 0;
						else
							reg = reg - 4;
						odm_set_bb_reg(dm, dc_offset[i], 0x0003ff00, reg);
						RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x2dbc[23:12]=0x%x > 0x202 Set 0x%x retry=%d\n",
							dck_check, reg, j);
					} else {
						RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x2dbc[23:12]=0x%x OK!!! retry=%d\n",
							dck_check, j);
						k = 3;
						break;
					}
				}

				btc_set_gnt_wl_bt_8822c(dm, false);
			}
#if 1
			odm_set_bb_reg(dm, R_0x1e70, 0x0000000f, 0x2);
			odm_set_rf_reg(dm, i, RF_0xde, 0x10000, 0x0);
			odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x0);
			odm_set_bb_reg(dm, R_0x1b00, 0x00000006, i);
			odm_set_bb_reg(dm, R_0x1ca4, 0x00000001, 0x0);
			odm_set_bb_reg(dm, R_0x1d08, 0x00000001, 0x0);

			odm_set_rf_reg(dm, i, RF_0x7f, 0x00002, 0x0);
			ODM_delay_us(100);
			odm_set_bb_reg(dm, tssi_enalbe[i], 0x08000000, 0x0);
			odm_set_bb_reg(dm, tssi_enalbe[i], 0x40000000, 0x0);
			ODM_delay_us(100);
			odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x0);
			ODM_delay_us(100);

#else
			odm_set_bb_reg(dm, R_0x1b00, 0x00000006, i);
			odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x00);
			odm_set_bb_reg(dm, R_0x1ca4, 0x00000001, 0x0);
			odm_set_rf_reg(dm, i, RF_0x7f, 0x00002, 0x0);
			odm_set_bb_reg(dm, tssi_enalbe[i], 0x08000000, 0x0);
			odm_set_bb_reg(dm, tssi_enalbe[i], 0x40000000, 0x0);
			odm_set_bb_reg(dm, R_0x1e70, 0x00000004, 0x0);
			odm_set_rf_reg(dm, i, RF_0xde, 0x10000, 0x0);
			odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x0);
			odm_set_bb_reg(dm, R_0x1d08, 0x00000001, 0x0);
#endif
			odm_set_bb_reg(dm, R_0x1d58, 0x00000008, 0x0);
			odm_set_bb_reg(dm, R_0x1d58, 0x00000ff0, 0x0);
			odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x0);

		} else {
			for (k = 0; k < 3; k++) {
				odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);
				/*odm_set_bb_reg(dm, R_0x1860, 0x00007000, 0x2);*/

				odm_set_bb_reg(dm, R_0x1d58, 0x00000008, 0x1);
				odm_set_bb_reg(dm, R_0x1d58, 0x00000ff0, 0xff);
				odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x2);

				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x700b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x701f0042);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x702f0042);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x703f0042);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x704f0042);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x705b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x706f0042);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x707b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x708b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x709b8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70ab8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70bb8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70cb8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70db8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70eb8041);
				odm_set_bb_reg(dm, tssi_setting[i], MASKDWORD, 0x70fb8041);


				odm_set_bb_reg(dm, dc_offset[i], 0x0003ff00, 0x0);
				odm_set_bb_reg(dm, R_0x820, 0x00000003, i + 1);
				odm_set_bb_reg(dm, R_0x1e2c, MASKDWORD, 0xe4e40000);
				odm_set_bb_reg(dm, R_0x1e28, 0x0000000f, 0x3);
				odm_set_bb_reg(dm, path_setting[i], 0x000fffff, 0x33312);
				odm_set_bb_reg(dm, path_setting[i], 0x80000000, 0x1);
				odm_set_bb_reg(dm, tssi_counter[i], 0xe0000000, 0x0);
				ODM_delay_us(200);
				odm_set_rf_reg(dm, i, RF_0x7f, 0x100, 0x1);
				odm_set_rf_reg(dm, i, RF_0x65, 0x03000, 0x3);
				odm_set_rf_reg(dm, i, RF_0x67, 0x00003, 0x3);
				odm_set_rf_reg(dm, i, RF_0x67, 0x00030, 0x2);
				odm_set_rf_reg(dm, i, RF_0x6f, 0x001e0, 0x0);
		
				odm_set_bb_reg(dm, tssi_enalbe[i], 0x08000000, 0x1);
				odm_set_bb_reg(dm, tssi_enalbe[i], 0x40000000, 0x1);
				odm_set_bb_reg(dm, R_0x1d08, 0x00000001, 0x1);
				odm_set_bb_reg(dm, R_0x1ca4, 0x00000001, 0x1);
				odm_set_bb_reg(dm, R_0x1b00, 0x00000006, i);
				odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x3f);
				odm_set_rf_reg(dm, i, RF_0xde, 0x10000, 0x1);
				odm_set_rf_reg(dm, i, RF_0x56, 0x000ff, 0x0);

				/*Set OFDM Packet Type*/
				odm_set_bb_reg(dm, R_0x900, 0x00000004, 0x1);
				odm_set_bb_reg(dm, R_0x900, 0x30000000, 0x2);
				odm_set_bb_reg(dm, R_0x908, 0x00ffffff, 0x21b6b);
				odm_set_bb_reg(dm, R_0x90c, 0x00ffffff, 0x800006);
				odm_set_bb_reg(dm, R_0x910, 0x00ffffff, 0x13600);
				odm_set_bb_reg(dm, R_0x914, 0x1fffffff, 0x6000fa);
				odm_set_bb_reg(dm, R_0x938, 0x0000ffff, 0x4b0f);
				odm_set_bb_reg(dm, R_0x940, MASKDWORD, 0x4ee33e41);
				odm_set_bb_reg(dm, R_0xa58, 0x003f8000, 0x2c);

				odm_set_bb_reg(dm, R_0x1e70, 0x00000004, 0x1);
				//ODM_delay_us(100);
				odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x1);
				//ODM_delay_us(100);

				/*odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x930);*/
				phydm_set_bb_dbg_port(dm, DBGPORT_PRI_2, debug_port[i]);
				odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x0);
				odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x1);
				/*reg = odm_get_bb_reg(dm, R_0x2dbc, 0x000003ff);*/
				reg = phydm_get_bb_dbg_port_val(dm) & 0x000003ff;
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x2dbc[9:0]=0x%x\n", reg);
				phydm_release_bb_dbg_port(dm);

				reg_tmp = reg;
				reg = 1024 - (((reg_tmp - 512) * 4) & 0x000003ff) + 5;
				odm_set_bb_reg(dm, dc_offset[i], 0x0003ff00, (reg & 0x03ff));
				
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x%x[17:8]=0x%x reg=0x%x\n",
					dc_offset[i], odm_get_bb_reg(dm, dc_offset[i], 0x0003ff00), reg);

				for (j = 0; j < 3; j++) {
					/*odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x930);*/
					phydm_set_bb_dbg_port(dm, DBGPORT_PRI_2, debug_port[i]);
					odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x0);
					odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x1);
					/*dck_check = odm_get_bb_reg(dm, R_0x2dbc, 0x000003ff);*/
					dck_check = phydm_get_bb_dbg_port_val(dm) & 0x000003ff;
					phydm_release_bb_dbg_port(dm);

					if (dck_check < 0x1ff) {
						if (reg >= 0x3fb)
							reg = 0x3ff;
						else
							reg = reg + 4;
						odm_set_bb_reg(dm, dc_offset[i], 0x0003ff00, reg);
						RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x2dbc[23:12]=0x%x < 0x1ff Set 0x%x retry=%d\n",
							dck_check, reg, j);
					} else if (dck_check > 0x202) {
						if (reg <= 4)
							reg = 0;
						else
							reg = reg - 4;
						odm_set_bb_reg(dm, dc_offset[i], 0x0003ff00, reg);
						RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x2dbc[23:12]=0x%x > 0x202 Set 0x%x retry=%d\n",
							dck_check, reg, j);
					} else {
						RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] 0x2dbc[23:12]=0x%x OK!!! retry=%d\n",
							dck_check, j);
						k = 3;
						break;
					}
				}
			}

#if 1
			odm_set_bb_reg(dm, R_0x1e70, 0x0000000f, 0x2);
			odm_set_rf_reg(dm, i, RF_0xde, 0x10000, 0x0);
			odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x0);
			odm_set_bb_reg(dm, R_0x1b00, 0x00000006, i);
			odm_set_bb_reg(dm, R_0x1ca4, 0x00000001, 0x0);
			odm_set_bb_reg(dm, R_0x1d08, 0x00000001, 0x0);

			odm_set_rf_reg(dm, i, RF_0x7f, 0x100, 0x0);
			ODM_delay_us(100);
			odm_set_bb_reg(dm, tssi_enalbe[i], 0x08000000, 0x0);
			odm_set_bb_reg(dm, tssi_enalbe[i], 0x40000000, 0x0);
			ODM_delay_us(100);
			odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x0);
			ODM_delay_us(100);
#else
			odm_set_bb_reg(dm, R_0x1b00, 0x00000006, i);
			odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x00);
			odm_set_bb_reg(dm, R_0x1ca4, 0x00000001, 0x0);
			odm_set_rf_reg(dm, i, RF_0x7f, 0x100, 0x0);
			odm_set_bb_reg(dm, tssi_enalbe[i], 0x08000000, 0x0);
			odm_set_bb_reg(dm, tssi_enalbe[i], 0x40000000, 0x0);
			odm_set_bb_reg(dm, R_0x1e70, 0x00000004, 0x0);
			odm_set_rf_reg(dm, i, RF_0xde, 0x10000, 0x0);
			odm_set_bb_reg(dm, tssi_counter[i], 0x10000000, 0x0);
			odm_set_bb_reg(dm, R_0x1d08, 0x00000001, 0x0);
#endif
			odm_set_bb_reg(dm, R_0x1d58, 0x00000008, 0x0);
			odm_set_bb_reg(dm, R_0x1d58, 0x00000ff0, 0x0);
			odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x0);


		}
	}

	_reload_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);
	rf->is_tssi_in_progress = 0;
	/*odm_release_spin_lock(dm, RT_IQK_SPINLOCK);*/

}

void halrf_tssi_get_efuse_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u8 pg_tssi = 0xff, i, j;
	u32 pg_tssi_tmp = 0xff;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "===>%s\n", __func__);

	/*path s0*/
	j = 0;
	for (i = 0x10; i <= 0x1a; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[0][j] = (s8)pg_tssi_tmp;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"tssi->tssi_efuse[%d][%d]=%d\n", 0, j, tssi->tssi_efuse[0][j]);
		j++;
	}

	for (i = 0x22; i <= 0x2f; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[0][j] = (s8)pg_tssi_tmp;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"tssi->tssi_efuse[%d][%d]=%d\n", 0, j, tssi->tssi_efuse[0][j]);
		j++;
	}

	/*path s1*/
	j = 0;
	for (i = 0x3a; i <= 0x44; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[1][j] = (s8)pg_tssi_tmp;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"tssi->tssi_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_efuse[1][j]);
		j++;
	}

	for (i = 0x4c; i <= 0x59; i++) {
		odm_efuse_logical_map_read(dm, 1, i, &pg_tssi_tmp);
		tssi->tssi_efuse[1][j] = (s8)pg_tssi_tmp;
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"tssi->tssi_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_efuse[1][j]);
		j++;
	}
}

u32 halrf_tssi_get_de_8822c(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;

	u32 i;

	u32 tssi_offest_de = 0;
	/*halrf_do_tssi_8822c(dm);*/
	rf->is_tssi_in_progress = 1;

	_halrf_tssi_8822c(dm);

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x180c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x1c64, 0x00007f00, 0x00);
		odm_set_bb_reg(dm, R_0x1c64, 0x003f8000, 0x00);
		odm_set_bb_reg(dm, R_0x1c64, 0x1fc00000, 0x00);
		odm_set_bb_reg(dm, R_0x18ec, 0x00c00000, 0x2);
		odm_set_bb_reg(dm, R_0x1c24, 0x07f80000, 0x20);
		odm_set_bb_reg(dm, R_0x1c64, 0x00007f00, 0x00);
		odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
		odm_set_bb_reg(dm, R_0x186c, 0x0000ff00, 0xff);
		odm_set_bb_reg(dm, R_0x1834, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x1860, 0x00000800, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x1);

		for (i = 0; odm_get_bb_reg(dm, R_0x28a4, 0x10000) == 0; i++) {
			ODM_delay_ms(1);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				 "TSSI finish bit != 1 retry=%d s0 0x%x\n", i,
				 odm_get_bb_reg(dm, R_0x28a4, MASKDWORD));
			if (i >= 36) {
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				       "TSSI finish bit i > 36ms, return s0\n");
				rf->is_tssi_in_progress = 0;
				break;
			}
		}

		tssi_offest_de = odm_get_bb_reg(dm, R_0x28a4, 0x1ff);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"======>%s path=%d tssi_offest_de_org=0x%x\n",
			__func__, path, tssi_offest_de);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x1c64, 0x00007f00, 0x00);
		odm_set_bb_reg(dm, R_0x1c64, 0x003f8000, 0x00);
		odm_set_bb_reg(dm, R_0x1c64, 0x1fc00000, 0x00);
		odm_set_bb_reg(dm, R_0x1ef0, 0x01fe0000, 0xff);
		odm_set_bb_reg(dm, R_0x41ec, 0x00c00000, 0x2);
		odm_set_bb_reg(dm, R_0x1d04, 0x000ff000, 0x20);
		odm_set_bb_reg(dm, R_0x1c64, 0x00007f00, 0x00);
		odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
		odm_set_bb_reg(dm, R_0x4134, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x4160, 0x00000800, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x1);

		for (i = 0; odm_get_bb_reg(dm, R_0x45a4, 0x10000) == 0; i++) {
			ODM_delay_ms(1);
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				 "TSSI finish bit != 1 retry=%d s1 0x%x\n", i,
				 odm_get_bb_reg(dm, R_0x45a4, MASKDWORD));
			if (i >= 36) {
				RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				       "TSSI finish bit i > 36ms, return s1\n");
				rf->is_tssi_in_progress = 0;
				break;
			}
		}

		tssi_offest_de = odm_get_bb_reg(dm, R_0x45a4, 0x1ff);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"======>%s path=%d tssi_offest_de_org=0x%x\n",
			__func__, path, tssi_offest_de);
	} else {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"======>%s path=%d is not exist!!!\n", __func__, path);
	}

	if (tssi_offest_de & BIT(8))
		tssi_offest_de = (tssi_offest_de & 0xff) | BIT(7);

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"======>%s path=%d tssi_offest_de_change=0x%x\n", __func__, path, tssi_offest_de);
	rf->is_tssi_in_progress = 0;

	return tssi_offest_de;
}


void halrf_tssi_get_kfree_efuse_8822c(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u8 pg_tssi = 0xff, i, j;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "===>%s\n", __func__);

	/*path s0*/
	j = 0;
	odm_efuse_one_byte_read(dm, 0x1c0, &pg_tssi, false);
	if (((pg_tssi & BIT(7)) >> 7) == 0) {
		if ((pg_tssi & BIT(0)) == 0)
			tssi->tssi_kfree_efuse[0][j] = (-1 * (pg_tssi >> 1));
		else
			tssi->tssi_kfree_efuse[0][j] = (pg_tssi >> 1);
	}
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"tssi->tssi_kfree_efuse[%d][%d]=%d\n", 0, j, tssi->tssi_kfree_efuse[0][j]);
	j++;

	odm_efuse_one_byte_read(dm, 0x1bc, &pg_tssi, false);
	if (((pg_tssi & BIT(7)) >> 7) == 0) {
		if ((pg_tssi & BIT(0)) == 0)
			tssi->tssi_kfree_efuse[0][j] = (-1 * (pg_tssi >> 1));
		else
			tssi->tssi_kfree_efuse[0][j] = (pg_tssi >> 1);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"tssi->tssi_kfree_efuse[%d][%d]=%d\n", 0, j, tssi->tssi_kfree_efuse[0][j]);
	j++;

	odm_efuse_one_byte_read(dm, 0x1b8, &pg_tssi, false);
	if (((pg_tssi & BIT(7)) >> 7) == 0) {
		if ((pg_tssi & BIT(0)) == 0)
			tssi->tssi_kfree_efuse[0][j] = (-1 * (pg_tssi >> 1));
		else
			tssi->tssi_kfree_efuse[0][j] = (pg_tssi >> 1);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"tssi->tssi_kfree_efuse[%d][%d]=%d\n", 0, j, tssi->tssi_kfree_efuse[0][j]);
	j++;

	odm_efuse_one_byte_read(dm, 0x3b4, &pg_tssi, false);
	if (((pg_tssi & BIT(7)) >> 7) == 0) {
		if ((pg_tssi & BIT(0)) == 0)
			tssi->tssi_kfree_efuse[0][j] = (-1 * (pg_tssi >> 1));
		else
			tssi->tssi_kfree_efuse[0][j] = (pg_tssi >> 1);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"tssi->tssi_kfree_efuse[%d][%d]=%d\n", 0, j, tssi->tssi_kfree_efuse[0][j]);
	j++;

	/*path s0*/
	j = 0;
	odm_efuse_one_byte_read(dm, 0x1bf, &pg_tssi, false);
	if (((pg_tssi & BIT(7)) >> 7) == 0) {
		if ((pg_tssi & BIT(0)) == 0)
			tssi->tssi_kfree_efuse[0][j] = (-1 * (pg_tssi >> 1));
		else
			tssi->tssi_kfree_efuse[0][j] = (pg_tssi >> 1);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"tssi->tssi_kfree_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_kfree_efuse[1][j]);
	j++;

	odm_efuse_one_byte_read(dm, 0x1bb, &pg_tssi, false);
	if (((pg_tssi & BIT(7)) >> 7) == 0) {
		if ((pg_tssi & BIT(0)) == 0)
			tssi->tssi_kfree_efuse[0][j] = (-1 * (pg_tssi >> 1));
		else
			tssi->tssi_kfree_efuse[0][j] = (pg_tssi >> 1);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"tssi->tssi_kfree_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_kfree_efuse[1][j]);
	j++;

	odm_efuse_one_byte_read(dm, 0x3b7, &pg_tssi, false);
	if (((pg_tssi & BIT(7)) >> 7) == 0) {
		if ((pg_tssi & BIT(0)) == 0)
			tssi->tssi_kfree_efuse[0][j] = (-1 * (pg_tssi >> 1));
		else
			tssi->tssi_kfree_efuse[0][j] = (pg_tssi >> 1);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"tssi->tssi_kfree_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_kfree_efuse[1][j]);
	j++;

	odm_efuse_one_byte_read(dm, 0x3b3, &pg_tssi, false);
	if (((pg_tssi & BIT(7)) >> 7) == 0) {
		if ((pg_tssi & BIT(0)) == 0)
			tssi->tssi_kfree_efuse[0][j] = (-1 * (pg_tssi >> 1));
		else
			tssi->tssi_kfree_efuse[0][j] = (pg_tssi >> 1);
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"tssi->tssi_kfree_efuse[%d][%d]=%d\n", 1, j, tssi->tssi_kfree_efuse[1][j]);
	j++;

#endif

}

void halrf_tssi_set_de_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	
	u8 i;
	u32 addr_d[2] = {0x18a8, 0x1eec};
	u32 addr_d_bitmask[2] = {0xff000000, 0x3fc00000};
	u32 addr_cck_d[2] = {R_0x18e8, R_0x1ef0};
	u32 addr_cck_d_bitmask[2] = {0x01fe0000, 0x0001fe00};
	s8 tssi_offest_de;
	u32 offset_index = 0;
	u8 channel = *dm->channel;
	s32 tmp;

	if (dm->rf_calibrate_info.txpowertrack_control == 4) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"==>%s txpowertrack_control=%d return!!!\n", __func__,
			dm->rf_calibrate_info.txpowertrack_control);
#if 0
		for (i = 0; i < MAX_PATH_NUM_8822C; i++) {
			odm_set_bb_reg(dm, addr_cck_d[i], addr_cck_d_bitmask[i], 0x0);
			odm_set_bb_reg(dm, addr_d[i], addr_d_bitmask[i], 0x0);
		}
#else
		if (channel >= 1 && channel <= 2)
			offset_index = 0;
		else if (channel >= 3 && channel <= 5)
			offset_index = 1;
		else if (channel >= 6 && channel <= 8)
			offset_index = 2;
		else if (channel >= 9 && channel <= 11)
			offset_index = 3;
		else if (channel >= 12 && channel <= 13)
			offset_index = 4;
		else if (channel == 14)
			offset_index = 5;

		for (i = 0; i < MAX_PATH_NUM_8822C; i++) {
			tmp = phydm_get_tssi_trim_de(dm, i);

			if (tmp > 127)
				tmp = 127;
			else if (tmp < -128)
				tmp = -128;

			odm_set_bb_reg(dm, addr_cck_d[i], addr_cck_d_bitmask[i],
				(tmp & 0xff));

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				"==>%s CCK 0x%x[%x] tssi_trim(%d)\n",
				__func__,
				addr_cck_d[i], addr_cck_d_bitmask[i],
				(tmp & 0xff)
				);

			tmp = phydm_get_tssi_trim_de(dm, i);
			odm_set_bb_reg(dm, addr_d[i], addr_d_bitmask[i], (tmp & 0xff));

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				"==>%s 0x%x[%x] tssi_offset(%d)\n",
				__func__,
				addr_d[i], addr_d_bitmask[i],
				(tmp & 0xff)
				);
		}



#endif
		return;
	}

	if (channel >= 1 && channel <= 2)
		offset_index = 0;
	else if (channel >= 3 && channel <= 5)
		offset_index = 1;
	else if (channel >= 6 && channel <= 8)
		offset_index = 2;
	else if (channel >= 9 && channel <= 11)
		offset_index = 3;
	else if (channel >= 12 && channel <= 13)
		offset_index = 4;
	else if (channel == 14)
		offset_index = 5;

	for (i = 0; i < MAX_PATH_NUM_8822C; i++) {
		tmp = tssi->tssi_efuse[i][offset_index] + phydm_get_tssi_trim_de(dm, i);

		if (tmp > 127)
			tmp = 127;
		else if (tmp < -128)
			tmp = -128;

		odm_set_bb_reg(dm, addr_cck_d[i], addr_cck_d_bitmask[i],
			(tmp & 0xff));

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"==>%s CCK 0x%x[%x] tssi_offset(%d)=tssi_efuse(%d)+tssi_trim(%d)\n",
			__func__,
			addr_cck_d[i], addr_cck_d_bitmask[i],
			(tmp & 0xff),
			tssi->tssi_efuse[i][offset_index],
			phydm_get_tssi_trim_de(dm, i));

		tssi_offest_de = (s8)_halrf_get_efuse_tssi_offset_8822c(dm, i);
		tmp = tssi_offest_de + phydm_get_tssi_trim_de(dm, i);

		if (tmp > 127)
			tmp = 127;
		else if (tmp < -128)
			tmp = -128;

		odm_set_bb_reg(dm, addr_d[i], addr_d_bitmask[i], (tmp & 0xff));

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			"==>%s 0x%x[%x] tssi_offset(%d)=tssi_efuse(%d)+tssi_trim(%d)\n",
			__func__,
			addr_d[i], addr_d_bitmask[i],
			(tmp & 0xff),
			tssi_offest_de,
			phydm_get_tssi_trim_de(dm, i));
	}
}

void halrf_tssi_set_de_for_tx_verify_8822c(
	void *dm_void, u32 tssi_de, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	u32 addr_d[2] = {0x18a8, 0x1eec};
	u32 addr_d_bitmask[2] = {0xff000000, 0x3fc00000};
	u32 addr_cck_d[2] = {R_0x18e8, R_0x1ef0};
	u32 addr_cck_d_bitmask[2] = {0x01fe0000, 0x0001fe00};
	u32 tssi_offest_de, offset_index = 0;
	s32 tmp, tssi_de_tmp;

#if 0
	odm_set_bb_reg(dm, addr_cck_d[path], addr_cck_d_bitmask[path], 
				(tssi_de & 0xff));
	odm_set_bb_reg(dm, addr_d[path], addr_d_bitmask[path], (tssi_de & 0xff));

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"==>%s CCK 0x%x[%x] tssi_efuse_offset=%d path=%d\n", __func__,
		addr_cck_d[path], addr_cck_d_bitmask[path], (tssi_de & 0xff), path);

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"==>%s 0x%x[%x] tssi_efuse_offset=%d path=%d\n", __func__,
		addr_d[path], addr_d_bitmask[path], (tssi_de & 0xff), path);
#endif
	if (tssi_de & BIT(7))
		tssi_de_tmp = tssi_de | 0xffffff00;
	else
		tssi_de_tmp = tssi_de;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"==>%s tssi_de(%d) tssi_de_tmp(%d)\n",
		__func__,
		tssi_de, tssi_de_tmp);

	tmp = tssi_de_tmp + phydm_get_tssi_trim_de(dm, path);

	if (tmp > 127)
		tmp = 127;
	else if (tmp < -128)
		tmp = -128;

	odm_set_bb_reg(dm, addr_cck_d[path], addr_cck_d_bitmask[path],
		(tmp & 0xff));

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"==>%s CCK 0x%x[%x] tssi_offset(%d)=tssi_de_tmp(%d)+tssi_trim(%d)\n",
		__func__,
		addr_cck_d[path], addr_cck_d_bitmask[path],
		(tmp & 0xff),
		tssi_de_tmp,
		phydm_get_tssi_trim_de(dm, path));

	tmp = tssi_de_tmp + phydm_get_tssi_trim_de(dm, path);

	if (tmp > 127)
		tmp = 127;
	else if (tmp < -128)
		tmp = -128;

	odm_set_bb_reg(dm, addr_d[path], addr_d_bitmask[path], (tmp & 0xff));

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		"==>%s 0x%x[%x] tssi_offset(%d)=tssi_de_tmp(%d)+tssi_trim(%d)\n",
		__func__,
		addr_d[path], addr_d_bitmask[path],
		(tmp & 0xff),
		tssi_de_tmp,
		phydm_get_tssi_trim_de(dm, path));
}

void halrf_enable_tssi_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 channel = *dm->channel;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "===>%s   channel=%d\n", __func__, channel);

	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);

	/*path s0*/
	if (channel >= 1 && channel <= 14)
		odm_set_bb_reg(dm, R_0x18a4, 0x0003e000, 0xb);
	else
		odm_set_bb_reg(dm, R_0x18a4, 0x0003e000, 0xd);

	odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, R_0x180c, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x1c64, 0x00007f00, 0x00);
	odm_set_bb_reg(dm, R_0x1c64, 0x003f8000, 0x00);
	odm_set_bb_reg(dm, R_0x1c64, 0x1fc00000, 0x00);
	odm_set_bb_reg(dm, R_0x18ec, 0x00c00000, 0x2);
	odm_set_bb_reg(dm, R_0x1c24, 0x07f80000, 0x20);
	odm_set_bb_reg(dm, R_0x1c64, 0x00007f00, 0x00);
	odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
	odm_set_bb_reg(dm, R_0x186c, 0x0000ff00, 0xff);
	odm_set_bb_reg(dm, R_0x1834, 0x80000000, 0x0);
	odm_set_bb_reg(dm, R_0x1860, 0x00000800, 0x0);
	odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x1);

	/*path s1*/
	if (channel >= 1 && channel <= 14)
		odm_set_bb_reg(dm, R_0x41a4, 0x0003e000, 0xb);
	else
		odm_set_bb_reg(dm, R_0x41a4, 0x0003e000, 0xd);

	odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, R_0x410c, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x1c64, 0x00007f00, 0x00);
	odm_set_bb_reg(dm, R_0x1c64, 0x003f8000, 0x00);
	odm_set_bb_reg(dm, R_0x1c64, 0x1fc00000, 0x00);
	odm_set_bb_reg(dm, R_0x1ef0, 0x01fe0000, 0xff);
	odm_set_bb_reg(dm, R_0x41ec, 0x00c00000, 0x2);
	odm_set_bb_reg(dm, R_0x1d04, 0x000ff000, 0x20);
	odm_set_bb_reg(dm, R_0x1c64, 0x00007f00, 0x00);
	odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
	odm_set_bb_reg(dm, R_0x4134, 0x80000000, 0x0);
	odm_set_bb_reg(dm, R_0x4160, 0x00000800, 0x0);
	odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x1);
}

void halrf_disable_tssi_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "===>%s\n", __func__);

	/*path s0*/
	odm_set_bb_reg(dm, R_0x18a4, 0x0003e000, 0x0);
	odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, R_0x180c, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);

	odm_set_bb_reg(dm, R_0x18a0, 0x7f, 0x0);

	/*path s1*/
	odm_set_bb_reg(dm, R_0x41a4, 0x0003e000, 0x0);
	odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x0);
	odm_set_bb_reg(dm, R_0x410c, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);

	odm_set_bb_reg(dm, R_0x41a0, 0x7f, 0x0);

	/*ADDA Serring*/
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
}

void halrf_do_tssi_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	struct dm_dpk_info *dpk_info = &dm->dpk_info;

	u32 bb_reg[7] = {R_0x820, R_0x1e2c, R_0x1d08, R_0x1c3c, R_0x1e28,
			R_0x18a0, R_0x41a0};
	u32 bb_reg_backup[7] = {0};
	u32 backup_num = 7;
	
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] ======>%s\n", __func__);

	/*odm_acquire_spin_lock(dm, RT_IQK_SPINLOCK);*/
	rf->is_tssi_in_progress = 1;

	_backup_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);
	
#if 0
	if ((rf->rf_supportability & HAL_RF_TX_PWR_TRACK) && (dm->priv->pmib->dot11RFEntry.tssi_enable) == 1) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] ======>%s\n", __func__);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[TSSI] rf_supportability HAL_RF_TX_PWR_TRACK on\n");
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[TSSI] dm->priv->pmib->dot11RFEntry.tssi_enable=%d\n",
		       dm->priv->pmib->dot11RFEntry.tssi_enable);
	} else {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "[TSSI] ======>%s, return!\n", __func__);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[TSSI] rf_supportability HAL_RF_TX_PWR_TRACK off, return!!\n");

		halrf_disable_tssi_8812f(dm);
		return;
	}
#endif
	
	
	halrf_tssi_set_de_8822c(dm);
	halrf_disable_tssi_8822c(dm);
	_halrf_tssi_init_8822c(dm);
	/*halrf_tssi_dck_8822c(dm);*/
	_halrf_tssi_8822c(dm);

	if (!(rf->rf_supportability & HAL_RF_TX_PWR_TRACK)) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "[TSSI] rf_supportability HAL_RF_TX_PWR_TRACK=%d, return!!!\n",
		       (rf->rf_supportability & HAL_RF_TX_PWR_TRACK));
		_reload_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);
		rf->is_tssi_in_progress = 0;
		/*odm_release_spin_lock(dm, RT_IQK_SPINLOCK);*/
		return;
	}

	if (*dm->mp_mode == 1) {
		if (cali_info->txpowertrack_control == 3) {
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				"[TSSI] cali_info->txpowertrack_control=%d, TSSI Mode\n",
				cali_info->txpowertrack_control);
			halrf_enable_tssi_8822c(dm);
			_reload_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);
			rf->is_tssi_in_progress = 0;
			/*odm_release_spin_lock(dm, RT_IQK_SPINLOCK);*/
			return;
		}
	} else {
		if (rf->power_track_type >= 4 && rf->power_track_type <= 7) {
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
				"[TSSI] cali_info->txpowertrack_control=%d, TSSI Mode\n",
				cali_info->txpowertrack_control);
			/*halrf_disable_tssi_8822c(dm);*/
			halrf_enable_tssi_8822c(dm);
			_reload_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);
			rf->is_tssi_in_progress = 0;
			/*odm_release_spin_lock(dm, RT_IQK_SPINLOCK);*/
			return;
		}	
	}

	_reload_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);
	rf->is_tssi_in_progress = 0;
	/*odm_release_spin_lock(dm, RT_IQK_SPINLOCK);*/
}

void halrf_do_thermal_8822c(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);
	struct _hal_rf_ *rf = &(dm->rf_table);
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u8 channel = *dm->channel;
	u8 rate = phydm_get_tx_rate(dm);

	if (tssi->index[RF_PATH_A][channel - 1] != 0 || tssi->index[RF_PATH_B][channel - 1] != 0) {
		odm_set_bb_reg(dm, R_0x18e8, 0x0001fc00,
			       (tssi->index[RF_PATH_A][channel - 1] & 0x7f));
		odm_set_bb_reg(dm, R_0x41e8, 0x0001fc00,
			       (tssi->index[RF_PATH_B][channel - 1] & 0x7f));

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s Set coex power index PathA:%d PathB:%d\n",
		       __func__, tssi->index[RF_PATH_A][channel - 1],
		       tssi->index[RF_PATH_B][channel - 1]);
	}

	if ((rf->rf_supportability & HAL_RF_TX_PWR_TRACK) == 1) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s\n", __func__);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "rf_supportability HAL_RF_TX_PWR_TRACK on\n");
	} else {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s, return!\n", __func__);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "rf_supportability HAL_RF_TX_PWR_TRACK off, return!!\n");

		halrf_disable_tssi_8822c(dm);
		return;
	}

	if (tssi->thermal[0] == 0xff || tssi->thermal[1] == 0xff) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s thermal[0]=0x%x thermal[1]=0x%x return!!!\n",
		       __func__, tssi->thermal[RF_PATH_A], tssi->thermal[RF_PATH_B]);
		return;
	}

	halrf_disable_tssi_8822c(dm);
	_halrf_thermal_init_8822c(dm);
	
	/*halrf_tssi_dck_8822c(dm);*/

	if (rate == MGN_1M || rate == MGN_2M || rate == MGN_5_5M || rate == MGN_11M) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s, in CCK Rate return!!!\n", __func__);
		return;
	}

	/*path s0*/
	if (channel >= 1 && channel <= 14) {
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c24, 0x0001fe00, 0x20);
		odm_set_bb_reg(dm, R_0x18ec, 0x00c00000, 0x3);
		odm_set_bb_reg(dm, R_0x1834, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x18e8, 0x00000001, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a8, 0x00000003, 0x2);
		odm_set_bb_reg(dm, R_0x18a8, 0x00007ffc, 0x0000);
		odm_set_bb_reg(dm, R_0x18a8, 0x00ff8000, 0x000);
		odm_set_bb_reg(dm, R_0x18a8, 0xff000000, 0x00);
		odm_set_bb_reg(dm, R_0x18e8, 0x000003fe, 0x000);
		odm_set_bb_reg(dm, R_0x18ec, 0x20000000, 0x1);
		odm_set_bb_reg(dm, R_0x186c, 0x0000ff00, 0xff);
		odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x1fffff00, 0x000000);
		odm_set_bb_reg(dm, R_0x18a0, 0x0000007f, 0x00);
		odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
		odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x1);		
	} else {
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c24, 0x0001fe00, 0x20);
		odm_set_bb_reg(dm, R_0x18ec, 0x00c00000, 0x3);
		odm_set_bb_reg(dm, R_0x1834, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x18e8, 0x00000001, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a8, 0x00000003, 0x2);
		odm_set_bb_reg(dm, R_0x18a8, 0x00007ffc, 0x0000);
		odm_set_bb_reg(dm, R_0x18a8, 0x00ff8000, 0x000);
		odm_set_bb_reg(dm, R_0x18a8, 0xff000000, 0x00);
		odm_set_bb_reg(dm, R_0x18e8, 0x000003fe, 0x000);
		odm_set_bb_reg(dm, R_0x18ec, 0x20000000, 0x1);
		odm_set_bb_reg(dm, R_0x186c, 0x0000ff00, 0xff);
		odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x1fffff00, 0x000000);
		odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a0, 0x0000007f, 0x00);
		odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x1);
	}

#if 1
	/*path s1*/
	if (channel >= 1 && channel <= 14) {
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1d04, 0x000ff000, 0x20);
		odm_set_bb_reg(dm, R_0x41ec, 0x00c00000, 0x3);
		odm_set_bb_reg(dm, R_0x4134, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x41e8, 0x00000001, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a8, 0x00000003, 0x2);
		odm_set_bb_reg(dm, R_0x1eec, 0x00001fff, 0x0000);
		odm_set_bb_reg(dm, R_0x1eec, 0x003fe000, 0x000);
		odm_set_bb_reg(dm, R_0x1eec, 0x3fc00000, 0x00);
		odm_set_bb_reg(dm, R_0x1ef0, 0x000001ff, 0x000);
		odm_set_bb_reg(dm, R_0x41ec, 0x20000000, 0x1);
		odm_set_bb_reg(dm, R_0x1ef0, 0x01fe0000, 0xff);
		odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x1fffff00, 0x000000);
		odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x41a0, 0x0000007f, 0x00);
		odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x1);
	} else {
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
		odm_set_bb_reg(dm, R_0x1d04, 0x000ff000, 0x20);
		odm_set_bb_reg(dm, R_0x41ec, 0x00c00000, 0x3);
		odm_set_bb_reg(dm, R_0x4134, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x41e8, 0x00000001, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a8, 0x00000003, 0x2);
		odm_set_bb_reg(dm, R_0x1eec, 0x00001fff, 0x0000);
		odm_set_bb_reg(dm, R_0x1eec, 0x003fe000, 0x000);
		odm_set_bb_reg(dm, R_0x1eec, 0x3fc00000, 0x00);
		odm_set_bb_reg(dm, R_0x1ef0, 0x000001ff, 0x000);
		odm_set_bb_reg(dm, R_0x41ec, 0x20000000, 0x1);
		odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
		odm_set_bb_reg(dm, R_0x1ef0, 0x01fe0000, 0xff);
		odm_set_bb_reg(dm, R_0x1c64, 0x1fffff00, 0x000000);
		odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x41a0, 0x0000007f, 0x00);
		odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
		odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x1);
		odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x1);
	}
#endif
	halrf_enable_tssi_8822c(dm);
#endif

}


u32 halrf_set_tssi_value_8822c(
	void *dm_void,
	u32 tssi_value)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u16 tssi_codeword_tmp[TSSI_CODE_NUM] = {0};
	s16 txagc_codeword_tmp[TSSI_CODE_NUM] = {0};
	u8 tx_rate = phydm_get_tx_rate(dm);
	u8 tssi_rate = _halrf_driver_rate_to_tssi_rate_8822c(dm, tx_rate);
	u8 rate = phydm_get_tx_rate(dm);
	s8 efuse, kfree;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "Call:%s tx_rate=0x%X tssi_rate=%d\n"
	       , __func__, tx_rate, tssi_rate);

	odm_move_memory(dm, tssi_codeword_tmp, tssi->tssi_codeword,
			sizeof(tssi_codeword_tmp));

	tssi_codeword_tmp[tssi_rate] = (u8)tssi_value;

	_halrf_calculate_txagc_codeword_8822c(dm, tssi_codeword_tmp, txagc_codeword_tmp);
	_halrf_set_txagc_codeword_8822c(dm, txagc_codeword_tmp);
	halrf_set_tssi_codeword_8822c(dm, tssi_codeword_tmp);

	kfree = _halrf_get_kfree_tssi_offset_8822c(dm);

	tssi_value = tssi_value - tssi->tssi_codeword[tssi_rate] - kfree;

	/*path s0*/
	/*2G CCK*/
	odm_set_bb_reg(dm, R_0x18e8, 0x01fe0000, 0);

	/*2G & 5G OFDM*/
	odm_set_bb_reg(dm, R_0x18a8, 0xff000000, 0);

	/*path s1*/
	/*2G CCK*/
	odm_set_bb_reg(dm, R_0x1ef0, 0x0001fe00, 0);

	/*2G & 5G OFDM*/
	odm_set_bb_reg(dm, R_0x1eec, 0x3fc00000, 0);

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s Set DE = 0\n", __func__);

	return tssi_value;
#endif
	return 0;
}


void halrf_set_tssi_poewr_8822c(
	void *dm_void,
	s8 power)
{
	s32 tssi_codeword = 0;
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u16 tssi_codeword_tmp[TSSI_CODE_NUM] = {0};
	s16 txagc_codeword_tmp[TSSI_CODE_NUM] = {0};
	u8 tx_rate = phydm_get_tx_rate(dm);
	u8 tssi_rate = _halrf_driver_rate_to_tssi_rate_8822c(dm, tx_rate);
	u8 rate = phydm_get_tx_rate(dm), i;
	u8 channel = *dm->channel, bw = *dm->band_width;
	s8 efuse, kfree;
	s32 index_tmp_a = 0, index_tmp_b = 0;
	u8 indexa , indexb;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "Call:%s tx_rate=0x%X tssi_rate=%d channel=%d\n"
	       , __func__, tx_rate, tssi_rate, channel);

	if (channel > 14) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "Not in 2G channel=%d\n", channel);
		return;
	}

	if ((power >= -13 && power <= 13) || power == 0x7f)
	{
#if 0
		power = power * TSSI_SLOPE_2G;

		odm_move_memory(dm, tssi_codeword_tmp, tssi->tssi_codeword,
				sizeof(tssi_codeword_tmp));

		if (power != 0x7f) {
			for (i = 0; i < TSSI_CODE_NUM; i++) {
				tssi_codeword_tmp[i] = tssi_codeword_tmp[i] + power;

				if (tssi_codeword_tmp[i] > 255)
					tssi_codeword_tmp[i] = 255;
				else if ((s16)tssi_codeword_tmp[i] < 0)
					tssi_codeword_tmp[i] = 0;
			}
		}

		_halrf_calculate_txagc_codeword_8822c(dm, tssi_codeword_tmp, txagc_codeword_tmp);
		_halrf_set_txagc_codeword_8822c(dm, txagc_codeword_tmp);
		/*halrf_set_tssi_codeword_8822c(dm, tssi_codeword_tmp);*/
#endif
#if 0
		if (power != 0x7f) {
			for (i = 0; i < TSSI_CODE_NUM; i++) {
				txagc_codeword_tmp[i] = power * 4;

				if (txagc_codeword_tmp[i] > 63)
					tssi_codeword_tmp[i] = 63;
				else if (txagc_codeword_tmp[i] < -64)
					tssi_codeword_tmp[i] = -64;
			}
		}

		_halrf_set_txagc_codeword_8822c(dm, txagc_codeword_tmp);

#endif
			  
		if (power != 0x7f) {
			indexa = odm_get_tx_power_index(dm, RF_PATH_A, rate, bw, channel);
			indexb = odm_get_tx_power_index(dm, RF_PATH_B, rate, bw, channel);
			
			index_tmp_a = indexa + power * 4;

			if (index_tmp_a > 127)
				index_tmp_a = 127;
			else if (index_tmp_a < 0)
				index_tmp_a = 0;

			tssi->index[RF_PATH_A][channel - 1] = (u32)index_tmp_a;

			index_tmp_b = indexb + power * 4;

			if (index_tmp_b > 127)
				index_tmp_b = 127;
			else if (index_tmp_b < 0)
				index_tmp_b = 0;

			tssi->index[RF_PATH_B][channel - 1] = (u32)index_tmp_b;

			odm_set_bb_reg(dm, R_0x18e8, 0x0001fc00, (index_tmp_a & 0x7f));
			odm_set_bb_reg(dm, R_0x41e8, 0x0001fc00, (index_tmp_b & 0x7f));

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "===>%s Set coex Tx index PathA:%d PathB:%d\n",
			       __func__,
			       odm_get_bb_reg(dm, R_0x18e8, 0x0001fc00),
			       odm_get_bb_reg(dm, R_0x41e8, 0x0001fc00));
		} else {
			indexa = odm_get_tx_power_index(dm, RF_PATH_A, rate, bw, channel);
			indexb = odm_get_tx_power_index(dm, RF_PATH_B, rate, bw, channel);

			odm_set_bb_reg(dm, R_0x18e8, 0x0001fc00, (indexa & 0x7f));
			odm_set_bb_reg(dm, R_0x41e8, 0x0001fc00, (indexb & 0x7f));

			RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
			       "===>%s Set coex Tx default index PathA:%d PathB:%d\n",
			       __func__,
			       odm_get_bb_reg(dm, R_0x18e8, 0x0001fc00),
			       odm_get_bb_reg(dm, R_0x41e8, 0x0001fc00));

			for (i = 1; i <= 14; i++) {
				tssi->index[RF_PATH_A][i - 1] = 0;
				tssi->index[RF_PATH_B][i - 1] = 0;
			}
		}

	}
#endif

}

void halrf_get_efuse_thermal_pwrtype_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct dm_rf_calibration_struct *cali_info = &(dm->rf_calibrate_info);
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;

	u32 thermal_tmp, pg_tmp;

	tssi->thermal[RF_PATH_A] = 0xff;
	tssi->thermal[RF_PATH_B] = 0xff;

	/*path s0*/
	odm_efuse_logical_map_read(dm, 1, 0xd0, &thermal_tmp);
	tssi->thermal[RF_PATH_A] = (u8)thermal_tmp;

	/*path s1*/
	odm_efuse_logical_map_read(dm, 1, 0xd1, &thermal_tmp);
	tssi->thermal[RF_PATH_B] = (u8)thermal_tmp;

	/*power tracking type*/
	odm_efuse_logical_map_read(dm, 1, 0xc8, &pg_tmp);
	if (((pg_tmp >> 4) & 0xf) == 0xf)
		rf->power_track_type = 0x0;
	else
		rf->power_track_type = (u8)((pg_tmp >> 4) & 0xf);

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "===>%s thermal pahtA=0x%x pahtB=0x%x power_track_type=0x%x\n",
	       __func__, tssi->thermal[RF_PATH_A],  tssi->thermal[RF_PATH_B],
	       rf->power_track_type);
	
}

u32 halrf_query_tssi_value_8822c(
	void *dm_void)
{
	s32 tssi_codeword = 0;

	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 tssi_rate;
	u8 rate = phydm_get_tx_rate(dm);
	/*s8 efuse, kfree;*/

	tssi_rate = _halrf_driver_rate_to_tssi_rate_8822c(dm, rate);
	/*tssi_codeword = tssi->tssi_codeword[tssi_rate];*/

#if 0
	if (rate == MGN_1M || rate == MGN_2M || rate == MGN_5_5M || rate == MGN_11M) {
		efuse = _halrf_get_efuse_tssi_offset_8822c(dm, 3);
		kfree = _halrf_get_kfree_tssi_offset_8822c(dm);
	} else {
		efuse = _halrf_get_efuse_tssi_offset_8822c(dm, 19);
		kfree = _halrf_get_kfree_tssi_offset_8822c(dm);
	}

	tssi_codeword = tssi_codeword + efuse + kfree;

	if (tssi_codeword <= 0)
		tssi_codeword = 0;
	else if (tssi_codeword >= 255)
		tssi_codeword = 255;
#endif
	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "===>%s tx_rate=0x%x tssi_codeword=0x%x\n",
	       __func__, rate, tssi->tssi_codeword[tssi_rate]);

	return (u32)tssi->tssi_codeword[tssi_rate];
}

void halrf_tssi_cck_8822c(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 rate = phydm_get_tx_rate(dm);
	u32 alogk, regc, regde, regf;
	s32 sregde, sregf;

	if (!(rate == MGN_1M || rate == MGN_2M || rate == MGN_5_5M || rate == MGN_11M))
		return;

	/*path s0*/
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0044);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0044);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0044);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0044);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x706f0044);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
	odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c24, 0x0001fe00, 0x20);
	odm_set_bb_reg(dm, R_0x18ec, 0x00c00000, 0x2);
	odm_set_bb_reg(dm, R_0x1834, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x18e8, 0x00000001, 0x0);
	odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
	odm_set_bb_reg(dm, R_0x18a8, 0x00000003, 0x2);
	odm_set_bb_reg(dm, R_0x18a8, 0x00007ffc, 0x1266);
	odm_set_bb_reg(dm, R_0x18a8, 0x00ff8000, 0x000);
	odm_set_bb_reg(dm, R_0x18a8, 0xff000000, 0x00);
	odm_set_bb_reg(dm, R_0x18e8, 0x01fe0000, 0x00);
	odm_set_bb_reg(dm, R_0x18e8, 0x000003fe, 0x110);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x7f, 0x00002, 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x65, 0x03000, 0x3);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x67, 0x0000c, 0x3);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x67, 0x000c0, 0x3);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x6e, 0x001e0, 0x0);
	odm_set_bb_reg(dm, R_0x1e7c, 0x00800000, 0x0);
	odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, R_0x180c, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x1800, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x1804, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x1800, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x1804, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x18ec, 0x20000000, 0x0);
	odm_set_bb_reg(dm, R_0x18ec, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x1860, 0x00000800, 0x0);
	odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x1);
	odm_set_bb_reg(dm, R_0x186c, 0x0000ff00, 0xff);
	odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c64, 0x1fffff00, 0x000000);
	odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x18a0, 0x0000007f, 0x00);
	odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);

	/*read AlogK u9bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x936);
	odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x1);
	alogk = odm_get_bb_reg(dm, R_0x2dbc, 0xff800000);

	/*read c u8bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x933);
	regc = odm_get_bb_reg(dm, R_0x2dbc, 0x003fc000);
	
	/*read de s8bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x933);
	regde = odm_get_bb_reg(dm, R_0x2dbc, 0x3fc00000);

	if (regde & 0x80)
		sregde = regde - 256;
	else
		sregde = regde;
	
	/*read f s7bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x934);
	regf = odm_get_bb_reg(dm, R_0x2dbc, 0x0000007f);
	
	if (regf & 0x40)
		sregf = regf - 128;
	else
		sregf = regf;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "tssi->cck_offset_patha(%d)\n",
		tssi->cck_offset_patha);

	tssi->cck_offset_patha = tssi->cck_offset_patha + ((s32)(regc - sregde - alogk - sregf) / 2);

	if (tssi->cck_offset_patha >= 63)
		tssi->cck_offset_patha = 63;
	else if (tssi->cck_offset_patha <= -64)
		tssi->cck_offset_patha = -64;
	
	odm_set_bb_reg(dm, R_0x18a0, 0x0000007f, (tssi->cck_offset_patha & 0x7f));

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "tssi->cck_offset_patha(%d) = (regc(%d) - sregde(%d) - alogk(%d) - sregf(%d)) / 2\n",
		tssi->cck_offset_patha, regc, sregde, alogk, sregf);


	/*path s1*/
	odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xf7d5005e);
	odm_set_bb_reg(dm, R_0x1860, 0x00007000, 0x4);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x701f0044);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x702f0044);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x703f0044);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x704f0044);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x706f0044);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709b8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ab8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bb8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cb8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70db8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70eb8041);
	odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70fb8041);
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
	odm_set_bb_reg(dm, R_0x1d04, 0x000ff000, 0x20);
	odm_set_bb_reg(dm, R_0x41ec, 0x00c00000, 0x2);
	odm_set_bb_reg(dm, R_0x4134, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x41e8, 0x00000001, 0x0);
	odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
	odm_set_bb_reg(dm, R_0x41a8, 0x00000003, 0x2);
	odm_set_bb_reg(dm, R_0x1eec, 0x00001fff, 0x1266);
	odm_set_bb_reg(dm, R_0x1eec, 0x003fe000, 0x000);
	odm_set_bb_reg(dm, R_0x1eec, 0x3fc00000, 0x00);
	odm_set_bb_reg(dm, R_0x1ef0, 0x0001fe00, 0x00);
	odm_set_bb_reg(dm, R_0x1ef0, 0x000001ff, 0x110);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x7f, 0x00002, 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x65, 0x03000, 0x3);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x67, 0x0000c, 0x3);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x67, 0x000c0, 0x3);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x6e, 0x001e0, 0x0);
	odm_set_bb_reg(dm, R_0x1e7c, 0x00800000, 0x0);
	odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
	odm_set_bb_reg(dm, R_0x410c, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x4100, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x4104, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x4100, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x4104, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x41ec, 0x20000000, 0x0);
	odm_set_bb_reg(dm, R_0x41ec, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x4160, 0x00000800, 0x0);
	odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x1);
	odm_set_bb_reg(dm, R_0x1ef0, 0x01fe0000, 0xff);
	odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c64, 0x1fffff00, 0x000000);
	odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x41a0, 0x0000007f, 0x00);
	odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);

	/*read AlogK u9bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0xb36);
	odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x1);
	alogk = odm_get_bb_reg(dm, R_0x2dbc, 0xff800000);

	/*read c u8bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0xb33);
	regc = odm_get_bb_reg(dm, R_0x2dbc, 0x003fc000);
	
	/*read de s8bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0xb33);
	regde = odm_get_bb_reg(dm, R_0x2dbc, 0x3fc00000);

	if (regde & 0x80)
		sregde = regde - 256;
	else
		sregde = regde;
	
	/*read f s7bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0xb34);
	regf = odm_get_bb_reg(dm, R_0x2dbc, 0x0000007f);
	
	if (regf & 0x40)
		sregf = regf - 128;
	else
		sregf = regf;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "tssi->cck_offset_pathb(%d)\n",
		tssi->cck_offset_pathb);

	tssi->cck_offset_pathb = tssi->cck_offset_pathb + ((s32)(regc - sregde - alogk - sregf) / 2);

	if (tssi->cck_offset_pathb >= 63)
		tssi->cck_offset_pathb = 63;
	else if (tssi->cck_offset_pathb <= -64)
		tssi->cck_offset_pathb = -64;
	
	odm_set_bb_reg(dm, R_0x41a0, 0x0000007f, (tssi->cck_offset_pathb & 0x7f));

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "tssi->cck_offset_pathb(%d) = (regc(%d) - sregde(%d) - alogk(%d) - sregf(%d)) / 2\n",
		tssi->cck_offset_pathb, regc, sregde, alogk, sregf);
#endif

}

void halrf_thermal_cck_8822c(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_tssi_data *tssi = &rf->halrf_tssi_data;
	u8 rate = phydm_get_tx_rate(dm);
	u32 alogk, regc, regde, regf;
	s32 sregde, sregf;

	if ((rf->rf_supportability & HAL_RF_TX_PWR_TRACK) == 1) {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s\n", __func__);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "rf_supportability HAL_RF_TX_PWR_TRACK on\n");
	} else {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "===>%s, return!\n", __func__);
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
		       "rf_supportability HAL_RF_TX_PWR_TRACK off, return!!\n");

		/*halrf_disable_tssi_8822c(dm);*/
		return;
	}

	if (!tssi->get_thermal) {
		odm_set_rf_reg(dm, RF_PATH_A, R_0x42, BIT(19), 0x01);
		odm_set_rf_reg(dm, RF_PATH_A, R_0x42, BIT(19), 0x00);
		odm_set_rf_reg(dm, RF_PATH_A, R_0x42, BIT(19), 0x01);
		
		odm_set_rf_reg(dm, RF_PATH_B, R_0x42, BIT(19), 0x01);
		odm_set_rf_reg(dm, RF_PATH_B, R_0x42, BIT(19), 0x00);
		odm_set_rf_reg(dm, RF_PATH_B, R_0x42, BIT(19), 0x01);

		tssi->get_thermal = 1;

		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "Trigger current thmermal\n");
	} else {
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "s0 current thmermal=0x%x(%d)\n",
		       odm_get_rf_reg(dm, RF_PATH_A, R_0x42, 0xfc00),
		       odm_get_rf_reg(dm, RF_PATH_A, R_0x42, 0xfc00));
		
		RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "s1 current thmermal=0x%x(%d)\n",
		       odm_get_rf_reg(dm, RF_PATH_B, R_0x42, 0xfc00),
		       odm_get_rf_reg(dm, RF_PATH_B, R_0x42, 0xfc00));

		tssi->get_thermal = 0;
	}

	if ((DBG_RF_TX_PWR_TRACK & dm->rf_table.rf_dbg_comp) == 1) {
		/*set debug port to 0x944*/
		if (phydm_set_bb_dbg_port(dm, DBGPORT_PRI_1, 0x944)) {
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "s0 Power Tracking offset %d\n",
			       (phydm_get_bb_dbg_port_val(dm) & 0x7f00) >> 8);
			phydm_release_bb_dbg_port(dm);
		}

		/*set debug port to 0xb44*/
		if (phydm_set_bb_dbg_port(dm, DBGPORT_PRI_1, 0xb44)) {
			RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "s1 Power Tracking offset %d\n",
			       (phydm_get_bb_dbg_port_val(dm) & 0x7f00) >> 8);
			phydm_release_bb_dbg_port(dm);
		}
	}

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK, "===>%s \n", __func__);

	if (!(rate == MGN_1M || rate == MGN_2M || rate == MGN_5_5M || rate == MGN_11M))
		return;

	/*path s0*/
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c24, 0x0001fe00, 0x20);
	odm_set_bb_reg(dm, R_0x18ec, 0x00c00000, 0x3);
	odm_set_bb_reg(dm, R_0x1834, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x18a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x18e8, 0x00000001, 0x0);
	odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
	odm_set_bb_reg(dm, R_0x18a8, 0x00000003, 0x2);
	odm_set_bb_reg(dm, R_0x18a8, 0x00007ffc, 0x0000);
	odm_set_bb_reg(dm, R_0x18a8, 0x00ff8000, 0x000);
	odm_set_bb_reg(dm, R_0x18a8, 0xff000000, 0x00);
	odm_set_bb_reg(dm, R_0x18e8, 0x01fe0000, 0x00);
	odm_set_bb_reg(dm, R_0x18e8, 0x000003fe, 0x000);
	odm_set_bb_reg(dm, R_0x18ec, 0x20000000, 0x1);
	odm_set_bb_reg(dm, R_0x186c, 0x0000ff00, 0xff);
	odm_set_bb_reg(dm, R_0x18a4, 0xe0000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c64, 0x1fffff00, 0x000000);
	odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x18a0, 0x0000007f, 0x00);
	odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
	
	/*read f s7bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0x934);
	regf = odm_get_bb_reg(dm, R_0x2dbc, 0x0000007f);
	
	if (regf & 0x40)
		sregf = regf - 128;
	else
		sregf = regf;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "tssi->cck_offset_patha(%d)\n",
		tssi->cck_offset_patha);

	//tssi->cck_offset_patha = tssi->cck_offset_patha + sregf;
	tssi->cck_offset_patha = sregf;

	if (tssi->cck_offset_patha >= 63)
		tssi->cck_offset_patha = 63;
	else if (tssi->cck_offset_patha <= -64)
		tssi->cck_offset_patha = -64;
	
	odm_set_bb_reg(dm, R_0x18a0, 0x0000007f, (tssi->cck_offset_patha & 0x7f));

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "tssi->cck_offset_patha(%d)\n", tssi->cck_offset_patha);


	/*path s1*/
	odm_set_bb_reg(dm, R_0x1e7c, 0x40000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c64, 0x20000000, 0x0);
	odm_set_bb_reg(dm, R_0x1d04, 0x000ff000, 0x20);
	odm_set_bb_reg(dm, R_0x41ec, 0x00c00000, 0x3);
	odm_set_bb_reg(dm, R_0x4134, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x41a4, 0x10000000, 0x0);
	odm_set_bb_reg(dm, R_0x41e8, 0x00000001, 0x0);
	odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
	odm_set_bb_reg(dm, R_0x41a8, 0x00000003, 0x2);
	odm_set_bb_reg(dm, R_0x1eec, 0x00001fff, 0x0000);
	odm_set_bb_reg(dm, R_0x1eec, 0x003fe000, 0x000);
	odm_set_bb_reg(dm, R_0x1eec, 0x3fc00000, 0x00);
	odm_set_bb_reg(dm, R_0x1ef0, 0x0001fe00, 0x00);
	odm_set_bb_reg(dm, R_0x1ef0, 0x000001ff, 0x000);
	odm_set_bb_reg(dm, R_0x41ec, 0x20000000, 0x1);
	odm_set_bb_reg(dm, R_0x1ef0, 0x01fe0000, 0xff);
	odm_set_bb_reg(dm, R_0x41a4, 0xe0000000, 0x0);
	odm_set_bb_reg(dm, R_0x1c64, 0x1fffff00, 0x000000);
	odm_set_bb_reg(dm, R_0x1e7c, 0x80000000, 0x1);
	odm_set_bb_reg(dm, R_0x41a0, 0x0000007f, 0x00);
	odm_set_bb_reg(dm, R_0x1d04, 0x07f00000, 0x00);
	
	/*read f s7bit*/
	odm_set_bb_reg(dm, R_0x1c3c, 0x000fff00, 0xb34);
	regf = odm_get_bb_reg(dm, R_0x2dbc, 0x0000007f);
	
	if (regf & 0x40)
		sregf = regf - 128;
	else
		sregf = regf;

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "tssi->cck_offset_pathb(%d)\n",
		tssi->cck_offset_pathb);

	//tssi->cck_offset_pathb = tssi->cck_offset_pathb + sregf;
	tssi->cck_offset_pathb = sregf;

	if (tssi->cck_offset_pathb >= 63)
		tssi->cck_offset_pathb = 63;
	else if (tssi->cck_offset_pathb <= -64)
		tssi->cck_offset_pathb = -64;
	
	odm_set_bb_reg(dm, R_0x41a0, 0x0000007f, (tssi->cck_offset_pathb & 0x7f));

	RF_DBG(dm, DBG_RF_TX_PWR_TRACK,
	       "tssi->cck_offset_pathb(%d)\n", tssi->cck_offset_pathb);
#endif

}

#endif

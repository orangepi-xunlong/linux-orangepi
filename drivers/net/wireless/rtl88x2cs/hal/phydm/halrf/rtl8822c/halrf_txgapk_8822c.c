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

void _halrf_txgapk_backup_bb_registers_8822c(
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

void _halrf_txgapk_reload_bb_registers_8822c(
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

void _halrf_txgapk_bb_dpk_8822c(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);
	
	odm_set_bb_reg(dm, R_0x1e24, 0x00020000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x10000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x20000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x40000000, 0x1);
	odm_set_bb_reg(dm, R_0x1cd0, 0x80000000, 0x0);
	/*odm_set_bb_reg(dm, R_0x1c68, 0x0f000000, 0xf);*/
	odm_set_bb_reg(dm, R_0x1d58, 0x00000ff8, 0x1ff);

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1864, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x186c, 0x00000080, 0x1);
		odm_set_bb_reg(dm, R_0x180c, 0x00000003, 0x0);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x4164, 0x80000000, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x1);
		odm_set_bb_reg(dm, R_0x416c, 0x00000080, 0x1);
		odm_set_bb_reg(dm, R_0x410c, 0x00000003, 0x0);
	}
	
	odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x2);
}

void _halrf_txgapk_afe_dpk_8822c(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x701f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x702f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x703f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x704f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x706f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709f0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70af0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bf0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cf0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70df0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ef0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ff0001);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ff0001);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffffffff);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x701f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x702f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x703f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x704f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x706f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709f0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70af0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bf0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cf0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70df0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ef0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ff0001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ff0001);
	}
}

void _halrf_txgapk_afe_dpk_restore_8822c(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70144041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70244041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70344041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70444041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70644041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8041);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x1c38, MASKDWORD, 0xffa1005e);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70144041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70244041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70344041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70444041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x705b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70644041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x707b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x708b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x709b8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70ab8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70bb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70cb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70db8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70eb8041);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70fb8041);
	}
}

void _halrf_txgapk_bb_dpk_restore_8822c(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	odm_set_rf_reg(dm, path, RF_0xde, 0x10000, 0x0);
	odm_set_rf_reg(dm, path, RF_0x9e, 0x00020, 0x0);
	odm_set_rf_reg(dm, path, RF_0x9e, 0x00400, 0x0);
	odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x0);
	odm_set_bb_reg(dm, R_0x1b20, 0xc0000000, 0x0);
	odm_set_bb_reg(dm, R_0x1bb8, 0x00100000, 0x0);
	odm_set_bb_reg(dm, R_0x1bcc, 0xff, 0x00);
	odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x1);
	odm_set_bb_reg(dm, R_0x1b20, 0xc0000000, 0x0);
	odm_set_bb_reg(dm, R_0x1bb8, 0x00100000, 0x0);
	odm_set_bb_reg(dm, R_0x1bcc, 0xff, 0x00);
	odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x0);
#if 0
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x1);
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x0);
	odm_set_bb_reg(dm, R_0x1d0c, 0x00010000, 0x1);
#endif
	/*odm_set_bb_reg(dm, R_0x1c68, 0x0f000000, 0x0);*/
	odm_set_bb_reg(dm, R_0x1d58, 0x00000ff8, 0x0);

	if (path == RF_PATH_A) {
		odm_set_bb_reg(dm, R_0x1864, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x180c, 0x08000000, 0x0);
		odm_set_bb_reg(dm, R_0x186c, 0x00000080, 0x0);
		odm_set_bb_reg(dm, R_0x180c, 0x00000003, 0x3);
	} else if (path == RF_PATH_B) {
		odm_set_bb_reg(dm, R_0x4164, 0x80000000, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x08000000, 0x0);
		odm_set_bb_reg(dm, R_0x416c, 0x00000080, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x00000003, 0x3);
	}

	odm_set_bb_reg(dm, R_0x1a00, 0x00000003, 0x0);
	odm_set_bb_reg(dm, R_0x1b20, 0x07000000, 0x5);

}

void _halrf_txgapk_write_gain_bb_table_8822c(
	void *dm_void)
{
#if 0
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	u8 channel = *dm->channel, i;
	u32 tmp_3f;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s channel=%d\n",
		__func__, channel);
	
	odm_set_bb_reg(dm, R_0x1b00, 0x00000006, 0x0);

	if (channel >= 1 && channel <= 14)
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x0);
	else if (channel >= 36 && channel <= 64)
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x2);
	else if (channel >= 100 && channel <= 144)
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x3);
	else if (channel >= 149 && channel <= 177)
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x4);

	odm_set_bb_reg(dm, R_0x1b9c, 0x000000ff, 0x88);

	for (i = 0; i < 11; i++) {
		tmp_3f = txgapk->txgapk_rf3f_bp[0][i][RF_PATH_A] & 0xfff;
		odm_set_bb_reg(dm, R_0x1b98, 0x00000fff, tmp_3f);
		odm_set_bb_reg(dm, R_0x1b98, 0x000f0000, i);
		odm_set_bb_reg(dm, R_0x1b98, 0x00008000, 0x1);
		odm_set_bb_reg(dm, R_0x1b98, 0x00008000, 0x0);

		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Set 0x1b98[11:0]=0x%03X   0x%x\n",
			tmp_3f, i);
	}
#else
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	u8 channel = *dm->channel, i;
	u8 path_idx, gain_idx, band_idx, check_txgain;
	u32 tmp_3f = 0;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s channel=%d\n",
		__func__, channel);

	for (band_idx = 0; band_idx < 5; band_idx++) {
		for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822C; path_idx++) {
			odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path_idx);

			if (band_idx == 0 || band_idx == 1)	/*2G*/
				odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x0);
			else if (band_idx == 2)	/*5GL*/
				odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x2);
			else if (band_idx == 3)	/*5GM*/
				odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x3);
			else if (band_idx == 4)	/*5GH*/
				odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x4);

			odm_set_bb_reg(dm, R_0x1b9c, 0x000000ff, 0x88);

			check_txgain = 0;
			for (gain_idx = 0; gain_idx < 11; gain_idx++) {

				if (((txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xf00) >> 8) >= 0xc &&
					((txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xf0) >> 4) >= 0xe) {
					if (check_txgain == 0) {
						tmp_3f = txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx];
						check_txgain = 1;
					}
					RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] tx_gain=0x%03X >= 0xCEX\n",
						txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx]);
				} else
					tmp_3f = txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] & 0xfff;
				
				odm_set_bb_reg(dm, R_0x1b98, 0x00000fff, tmp_3f);
				odm_set_bb_reg(dm, R_0x1b98, 0x000f0000, gain_idx);
				odm_set_bb_reg(dm, R_0x1b98, 0x00008000, 0x1);
				odm_set_bb_reg(dm, R_0x1b98, 0x00008000, 0x0);

				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Write Gain 0x1b98 Band=%d 0x1b98[11:0]=0x%03X path=%d\n",
					band_idx, tmp_3f, path_idx);
			}
		}
	}
#endif
}

void _halrf_txgapk_calculate_offset_8822c(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;

	u8 i;
	u8 channel = *dm->channel;

	u32 set_pi[MAX_PATH_NUM_8822C] = {R_0x1c, R_0xec};
	u32 set_1b00_cfg1[MAX_PATH_NUM_8822C] = {0x00000d18, 0x00000d2a};
	u32 set_1b00_cfg2[MAX_PATH_NUM_8822C] = {0x00000d19, 0x00000d2b};
	u32 path_setting[2] = {R_0x1800, R_0x4100};

	u32 bb_reg[5] = {R_0x820, R_0x1e2c, R_0x1e28, R_0x1800, R_0x4100};
	u32 bb_reg_backup[5] = {0};
	u32 backup_num = 5;
	
	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s channel=%d\n",
		__func__, channel);

	_halrf_txgapk_backup_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);

	if (channel >= 1 && channel <= 14) {	/*2G*/
		odm_set_bb_reg(dm, R_0x1bb8, 0x00100000, 0x0);
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x3f);
		odm_set_bb_reg(dm, R_0x1b20, 0xc0000000, 0x0);
		odm_set_rf_reg(dm, path, RF_0xde, 0x10000, 0x1);
		odm_set_rf_reg(dm, path, RF_0x00, RFREGOFFSETMASK, 0x5000f);
		odm_set_rf_reg(dm, path, RF_0x55, 0x0001c, 0x0);
		odm_set_rf_reg(dm, path, RF_0x87, 0x40000, 0x1);
		odm_set_rf_reg(dm, path, RF_0x00, 0x003e0, 0x0f);
		odm_set_rf_reg(dm, path, RF_0xde, 0x00004, 0x1);
		odm_set_rf_reg(dm, path, RF_0x1a, 0x07000, 0x1);
		odm_set_rf_reg(dm, path, RF_0x1a, 0x00c00, 0x0);
		odm_set_rf_reg(dm, path, RF_0x8f, 0x00002, 0x1);

		odm_set_bb_reg(dm, R_0x1b10, 0xff, 0x00);
		odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x0);

		odm_set_bb_reg(dm, R_0x820, 0x00000003, path + 1);
		odm_set_bb_reg(dm, R_0x1e2c, MASKDWORD, 0xe4e40000);
		odm_set_bb_reg(dm, R_0x1e28, 0x0000000f, 0x3);
		odm_set_bb_reg(dm, path_setting[path], 0x000fffff, 0x33312);
		odm_set_bb_reg(dm, path_setting[path], 0x80000000, 0x1);

		odm_set_bb_reg(dm, set_pi[path], 0xc0000000, 0x0);
		odm_set_rf_reg(dm, path, RF_0xdf, 0x00010, 0x1);
		odm_set_rf_reg(dm, path, RF_0x58, 0xfff00, 0x820);
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);

		odm_set_bb_reg(dm, R_0x1b10, 0x000000ff, 0x0);

		odm_set_bb_reg(dm, R_0x1b2c, 0xff, 0x018);
		ODM_delay_us(1000);
		odm_set_bb_reg(dm, R_0x1bcc, 0xff, 0x2d);
		ODM_delay_us(1000);

		odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, set_1b00_cfg1[path]);
		odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, set_1b00_cfg2[path]);

		for (i = 0; i < 100; i++) {
			ODM_delay_us(1000);
			RF_DBG(dm, DBG_RF_TXGAPK, "================= delay %dms\n", i + 1);
			if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000ff) == 0x55)
				break;	
		}

		odm_set_bb_reg(dm, set_pi[path], 0xc0000000, 0x2);
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, R_0x1bd4, 0x00200000, 0x1);
		odm_set_bb_reg(dm, R_0x1bd4, 0x001f0000, 0x12);
		odm_set_bb_reg(dm, R_0x1b9c, 0x00000f00, 0x3);

		txgapk->offset[0][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x0000000f);
		txgapk->offset[1][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x000000f0);
		txgapk->offset[2][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x00000f00);
		txgapk->offset[3][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x0000f000);
		txgapk->offset[4][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x000f0000);
		txgapk->offset[5][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x00f00000);
		txgapk->offset[6][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x0f000000);
		txgapk->offset[7][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0xf0000000);
		odm_set_bb_reg(dm, R_0x1b9c, 0x00000f00, 0x4);
		txgapk->offset[8][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x0000000f);
		txgapk->offset[9][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x000000f0);

		for (i = 0; i < 10; i++) {
			if (txgapk->offset[i][path] & BIT(3))
				txgapk->offset[i][path] = txgapk->offset[i][path] | 0xf0;
		}

		for (i = 0; i < 10; i++)
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] offset %d   %d   path=%d\n",
				txgapk->offset[i][path], i, path);

		RF_DBG(dm, DBG_RF_TXGAPK, "========================================\n");
	} else {	/*5G*/
		odm_set_bb_reg(dm, R_0x1bb8, 0x00100000, 0x0);
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, R_0x1bcc, 0x0000003f, 0x3f);
		odm_set_bb_reg(dm, R_0x1b20, 0xc0000000, 0x0);
		odm_set_rf_reg(dm, path, RF_0xde, 0x10000, 0x1);
		odm_set_rf_reg(dm, path, RF_0x00, RFREGOFFSETMASK, 0x50011);
		odm_set_rf_reg(dm, path, RF_0x63, 0x0c000, 0x3);
		odm_set_rf_reg(dm, path, RF_0x63, 0x0001c, 0x3);
		odm_set_rf_reg(dm, path, RF_0x63, 0x03000, 0x1);
		odm_set_rf_reg(dm, path, RF_0x8a, 0x00018, 0x2);
		odm_set_rf_reg(dm, path, RF_0x00, 0x003e0, 0x12);
		odm_set_rf_reg(dm, path, RF_0xde, 0x00004, 0x1);
		odm_set_rf_reg(dm, path, RF_0x1a, 0x00c00, 0x0);
		odm_set_rf_reg(dm, path, RF_0x8f, 0x00002, 0x1);
		odm_set_rf_reg(dm, path, RF_0x0, 0xf0000, 0x5);

		odm_set_bb_reg(dm, R_0x1b10, 0xff, 0x0);

		if (channel >= 36 && channel <= 64)
			odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x2);
		else if (channel >= 100 && channel <= 144)
			odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x3);
		else if (channel >= 149 && channel <= 177) 
			odm_set_bb_reg(dm, R_0x1b98, 0x00007000, 0x4);

		odm_set_bb_reg(dm, R_0x820, 0x00000003, path + 1);
		odm_set_bb_reg(dm, R_0x1e2c, MASKDWORD, 0xe4e40000);
		odm_set_bb_reg(dm, R_0x1e28, 0x0000000f, 0x3);
		odm_set_bb_reg(dm, path_setting[path], 0x000fffff, 0x33312);
		odm_set_bb_reg(dm, path_setting[path], 0x80000000, 0x1);

		odm_set_bb_reg(dm, set_pi[path], 0xc0000000, 0x0);
		odm_set_rf_reg(dm, path, RF_0xdf, 0x00010, 0x1);
		odm_set_rf_reg(dm, path, RF_0x58, 0xfff00, 0x820);
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);

		odm_set_bb_reg(dm, R_0x1b10, 0x000000ff, 0x0);

		odm_set_bb_reg(dm, R_0x1b2c, 0xff, 0x018);
		ODM_delay_us(1000);
		odm_set_bb_reg(dm, R_0x1bcc, 0xff, 0x36);
		ODM_delay_us(1000);
		
		odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, set_1b00_cfg1[path]);
		odm_set_bb_reg(dm, R_0x1b00, MASKDWORD, set_1b00_cfg2[path]);

		for (i = 0; i < 100; i++) {
			ODM_delay_us(1000);
			RF_DBG(dm, DBG_RF_TXGAPK, "================= delay %dms\n", i + 1);
			if (odm_get_bb_reg(dm, R_0x2d9c, 0x000000ff) == 0x55)
				break;	
		}

		odm_set_bb_reg(dm, set_pi[path], 0xc0000000, 0x2);
		odm_set_bb_reg(dm, R_0x1b00, 0x00000006, path);
		odm_set_bb_reg(dm, R_0x1bd4, 0x00200000, 0x1);
		odm_set_bb_reg(dm, R_0x1bd4, 0x001f0000, 0x12);
		odm_set_bb_reg(dm, R_0x1b9c, 0x00000f00, 0x3);

		txgapk->offset[0][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x0000000f);
		txgapk->offset[1][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x000000f0);
		txgapk->offset[2][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x00000f00);
		txgapk->offset[3][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x0000f000);
		txgapk->offset[4][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x000f0000);
		txgapk->offset[5][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x00f00000);
		txgapk->offset[6][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x0f000000);
		txgapk->offset[7][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0xf0000000);
		odm_set_bb_reg(dm, R_0x1b9c, 0x00000f00, 0x4);
		txgapk->offset[8][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x0000000f);
		txgapk->offset[9][path] = (s8)odm_get_bb_reg(dm, R_0x1bfc, 0x000000f0);

		for (i = 0; i < 10; i++) {
			if (txgapk->offset[i][path] & BIT(3))
				txgapk->offset[i][path] = txgapk->offset[i][path] | 0xf0;
		}

		for (i = 0; i < 10; i++)
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] offset %d   %d   path=%d\n",
				txgapk->offset[i][path], i, path);

		RF_DBG(dm, DBG_RF_TXGAPK, "========================================\n");
	}
	_halrf_txgapk_reload_bb_registers_8822c(dm, bb_reg, bb_reg_backup, backup_num);
}

void _halrf_txgapk_rf_restore_8822c(
	void *dm_void, u8 path)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 i;
	u32 tmp[10] = {0};

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (path == RF_PATH_A) {
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x00, 0xf0000, 0x3);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0xde, 0x00004, 0x0);
		odm_set_rf_reg(dm, RF_PATH_A, RF_0x8f, 0x00002, 0x0);
	} else if (path == RF_PATH_B) {
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x00, 0xf0000, 0x3);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0xde, 0x00004, 0x0);
		odm_set_rf_reg(dm, RF_PATH_B, RF_0x8f, 0x00002, 0x0);
	}
}

u32 _halrf_txgapk_calculat_tx_gain_8822c(
	void *dm_void, u32 original_tx_gain, s8 offset)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	u32 modify_tx_gain = original_tx_gain;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (((original_tx_gain & 0xf00) >> 8) >= 0xc && ((original_tx_gain & 0xf0) >> 4) >= 0xe) {
		modify_tx_gain = original_tx_gain;
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] original_tx_gain=0x%03X(>=0xCEX) offset=%d modify_tx_gain=0x%03X\n",
			original_tx_gain, offset, modify_tx_gain);
		return modify_tx_gain;
	}

	if (offset < 0) {
		if ((offset % 2) == 0)
			modify_tx_gain = modify_tx_gain + (offset / 2);
		else {
			modify_tx_gain = modify_tx_gain + 0x1000 + (offset / 2) - 1;
		}
	} else {
		if ((offset % 2) == 0)
			modify_tx_gain = modify_tx_gain + (offset / 2);
		else {
			modify_tx_gain = modify_tx_gain + 0x1000 + (offset / 2);
		}
	}

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] original_tx_gain=0x%X offset=%d modify_tx_gain=0x%X\n",
		original_tx_gain, offset, modify_tx_gain);

	return modify_tx_gain;
}

void _halrf_txgapk_write_tx_gain_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;

	u32 i, j, tmp = 0x20, tmp1 = 0x60, tmp_3f;
	s8 offset_tmp[11] = {0};
	u8 channel = *dm->channel, path_idx, band_idx = 1;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (channel >= 1 && channel <= 14) {
		tmp = 0x20;	/*2G CCK*/
		tmp1 = 0x60;	/*2G OFDM*/
		band_idx = 1;
	} else if (channel >= 36 && channel <= 64) {
		tmp = 0x200;	/*5G L*/
		tmp1 = 0x0;
		band_idx = 2;
	} else if (channel >= 100 && channel <= 144) {
		tmp = 0x280;	/*5G M*/
		tmp1 = 0x0;
		band_idx = 3;
	} else if (channel >= 149 && channel <= 177) {
		tmp = 0x300;	/*5G H*/
		tmp1 = 0x0;
		band_idx = 4;
	}

	for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822C; path_idx++) {
		for (i = 0; i <= 10; i++) {
			offset_tmp[i] = 0;
			for (j = i; j <= 10; j++) {
				if ((((txgapk->txgapk_rf3f_bp[band_idx][j][path_idx] & 0xf00) >> 8) >= 0xc) &&
					(((txgapk->txgapk_rf3f_bp[band_idx][j][path_idx] & 0xf0) >> 4) >= 0xe))
					continue;

				offset_tmp[i] = offset_tmp[i] + txgapk->offset[j][path_idx];
				txgapk->fianl_offset[i][path_idx] = offset_tmp[i];
			}

			if ((((txgapk->txgapk_rf3f_bp[band_idx][i][path_idx] & 0xf00) >> 8) >= 0xc) &&
				(((txgapk->txgapk_rf3f_bp[band_idx][i][path_idx] & 0xf0) >> 4) >= 0xe))
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] tx_gain=0x%03X >= 0xCEX\n",
					txgapk->txgapk_rf3f_bp[band_idx][i][path_idx]);
			else
				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] offset %d   %d\n", offset_tmp[i], i);
		}

		odm_set_rf_reg(dm, path_idx, 0xee, 0xfffff, 0x10000);

		j = 0;
		for (i = tmp; i <= (tmp + 10); i++) {
			odm_set_rf_reg(dm, path_idx, RF_0x33, 0xfffff, i);

			tmp_3f = _halrf_txgapk_calculat_tx_gain_8822c(dm,
				txgapk->txgapk_rf3f_bp[band_idx][j][path_idx], offset_tmp[j]);
			tmp_3f = tmp_3f & 0x01fff;
			odm_set_rf_reg(dm, path_idx, RF_0x3f, 0x01fff, tmp_3f);

			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x33=0x%05X   0x3f=0x%04X\n",
				i, tmp_3f);
			j++;
		}

		if (tmp1 == 0x60) {
			j = 0;
			for (i = tmp1; i <= (tmp1 + 10); i++) {
				odm_set_rf_reg(dm, path_idx, RF_0x33, 0xfffff, i);

				tmp_3f = _halrf_txgapk_calculat_tx_gain_8822c(dm,
					txgapk->txgapk_rf3f_bp[band_idx][j][path_idx], offset_tmp[j]);
				tmp_3f = tmp_3f & 0x01fff;
				odm_set_rf_reg(dm, path_idx, RF_0x3f, 0x01fff, tmp_3f);

				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x33=0x%05X   0x3f=0x%04X\n",
					i, tmp_3f);
				j++;
			}
		}

		odm_set_rf_reg(dm, path_idx, 0xee, 0xfffff, 0x0);
	}
}

void _halrf_txgapk_disable_power_trim_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 path_idx;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822C; path_idx++) {
		odm_set_rf_reg(dm, path_idx, RF_0xde, BIT(9), 0x1);
		odm_set_rf_reg(dm, path_idx, RF_0x55, 0x000fc000, 0x0);
	}

}

void _halrf_txgapk_enable_power_trim_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	u8 path_idx;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822C; path_idx++)
		odm_set_rf_reg(dm, path_idx, RF_0xde, BIT(9), 0x0);
}

void halrf_txgapk_save_all_tx_gain_table_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	u32 three_wire[2] = {R_0x180c, R_0x410c}, rf18;
	u8 ch_num[5] = {1, 1, 36, 100, 149};
	u8 band_num[5] = {0x0, 0x0, 0x1, 0x3, 0x5};
	u8 path_idx, band_idx, gain_idx, rf0_idx;
	
	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (txgapk->read_txgain == 1) {
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Already Read txgapk->read_txgain return!!!\n");
		_halrf_txgapk_write_gain_bb_table_8822c(dm);
		return;
	}

	for (band_idx = 0; band_idx < 5; band_idx++) {
		for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822C; path_idx++) {
			rf18 = odm_get_rf_reg(dm, path_idx, RF_0x18, 0xfffff);

			odm_set_bb_reg(dm, three_wire[path_idx], 0x00000003, 0x0);

			odm_set_rf_reg(dm, path_idx, RF_0x18, 0x000ff, ch_num[band_idx]);
			odm_set_rf_reg(dm, path_idx, RF_0x18, 0x70000, band_num[band_idx]);

			gain_idx = 0;
			for (rf0_idx = 1; rf0_idx < 32; rf0_idx = rf0_idx + 3) {
				odm_set_rf_reg(dm, path_idx, 0x0, 0x000ff, rf0_idx);
				txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx] = odm_get_rf_reg(dm, path_idx, 0x5f, 0xfffff);

				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x5f=0x%03X band_idx=%d path=%d\n",
					txgapk->txgapk_rf3f_bp[band_idx][gain_idx][path_idx], band_idx, path_idx);
				gain_idx++;
			}

			odm_set_rf_reg(dm, path_idx, RF_0x18, 0xfffff, rf18);
			odm_set_bb_reg(dm, three_wire[path_idx], 0x00000003, 0x3);
		}
	}

	_halrf_txgapk_write_gain_bb_table_8822c(dm);

	txgapk->read_txgain = 1;
}

void halrf_txgapk_reload_tx_gain_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;

	u32 i, j, tmp, tmp1;
	u8 path_idx, band_idx;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	for (band_idx = 1; band_idx <= 4; band_idx++) {
		if (band_idx == 1) {
			tmp = 0x20;	/*2G CCK*/
			tmp1 = 0x60;	/*2G OFDM*/
		} else if (band_idx == 2) {
			tmp = 0x200;	/*5G L*/
			tmp1 = 0x0;
		} else if (band_idx == 3) {
			tmp = 0x280;	/*5G M*/
			tmp1 = 0x0;
		} else if (band_idx == 4) {
			tmp = 0x300;	/*5G H*/
			tmp1 = 0x0;
		}

		for (path_idx = RF_PATH_A; path_idx < MAX_PATH_NUM_8822C; path_idx++) {
			odm_set_rf_reg(dm, path_idx, 0xee, 0xfffff, 0x10000);

			j = 0;
			for (i = tmp; i <= (tmp + 10); i++) {
				odm_set_rf_reg(dm, path_idx, RF_0x33, 0xfffff, i);

				odm_set_rf_reg(dm, path_idx, RF_0x3f, 0xfff,
					txgapk->txgapk_rf3f_bp[band_idx][j][path_idx]);

				RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x33=0x%05X   0x3f=0x%03X\n",
					i, txgapk->txgapk_rf3f_bp[band_idx][j][path_idx]);
				j++;
			}

			if (tmp1 == 0x60) {
				j = 0;
				for (i = tmp1; i <= (tmp1 + 10); i++) {
					odm_set_rf_reg(dm, path_idx, RF_0x33, 0xfffff, i);

					odm_set_rf_reg(dm, path_idx, RF_0x3f, 0xfff,
						txgapk->txgapk_rf3f_bp[band_idx][j][path_idx]);

					RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] 0x33=0x%05X   0x3f=0x%04X\n",
						i, txgapk->txgapk_rf3f_bp[band_idx][j][path_idx]);
					j++;
				}
			}

			odm_set_rf_reg(dm, path_idx, 0xee, 0xfffff, 0x0);
		}
	}
}

void halrf_txgapk_8822c(
	void *dm_void)
{
	struct dm_struct *dm = (struct dm_struct *)dm_void;
	struct _hal_rf_ *rf = &dm->rf_table;
	struct _halrf_txgapk_info *txgapk = &rf->halrf_txgapk_info;
	struct dm_rf_calibration_struct *cali_info = &dm->rf_calibrate_info;
	u8 path_idx;

	RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] ======>%s\n", __func__);

	if (txgapk->read_txgain == 0) {
		RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] txgapk->read_txgain == 0 return!!!\n");
		return;
	}

	if (*dm->mp_mode == 1) {
		if (cali_info->txpowertrack_control == 2 ||
			cali_info->txpowertrack_control == 3 ||
			cali_info->txpowertrack_control == 4 ||
			cali_info->txpowertrack_control == 5) {
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] MP Mode in TSSI mode. return!!!\n");
			return;
		}
	} else {
		if (rf->power_track_type >= 4 && rf->power_track_type <= 7) {
			RF_DBG(dm, DBG_RF_TXGAPK, "[TXGAPK] Normal Mode in TSSI mode. return!!!\n");
			return;
		}
	}

	rf->is_tssi_in_progress = 1;

	/*_halrf_txgapk_disable_power_trim_8822c(dm);*/

	for (path_idx = 0; path_idx < MAX_PATH_NUM_8822C; path_idx++) {
		_halrf_txgapk_bb_dpk_8822c(dm, path_idx);
		_halrf_txgapk_afe_dpk_8822c(dm, path_idx);
		_halrf_txgapk_calculate_offset_8822c(dm, path_idx);
		_halrf_txgapk_rf_restore_8822c(dm, path_idx);
		_halrf_txgapk_afe_dpk_restore_8822c(dm, path_idx);
		_halrf_txgapk_bb_dpk_restore_8822c(dm, path_idx);
	}

	_halrf_txgapk_write_tx_gain_8822c(dm);

	/*_halrf_txgapk_enable_power_trim_8822c(dm);*/

	rf->is_tssi_in_progress = 0;
}


#endif

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
#include "../phydm_precomp.h"

#if (RTL8822C_SUPPORT)
#if (PHYDM_FW_API_ENABLE_8822C)
/* ======================================================================== */
/* These following functions can be used for PHY DM only*/
#ifdef CONFIG_TXAGC_DEBUG_8822C
__odm_func__
boolean phydm_set_pw_by_rate_8822c(struct dm_struct *dm, s8 *pw_idx,
				   u8 rate_idx)
{
	u32 pw_all = 0;
	u8 j = 0;

	if (rate_idx % 4 != 0) {
		pr_debug("[Warning] %s\n", __func__);
		return false;
	}

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "pow = {%d, %d, %d, %d}\n",
		  *pw_idx, *(pw_idx - 1), *(pw_idx - 2), *(pw_idx - 3));

	/* @bbrstb TX AGC report - default disable */
	/* @Enable for writing the TX AGC table when bb_reset=0 */
	odm_set_bb_reg(dm, R_0x1c90, BIT(15), 0x0);

	/* @According the rate to write in the ofdm or the cck */
	/* @driver need to construct a 4-byte power index */
	odm_set_bb_reg(dm, R_0x3a00 + rate_idx, MASKDWORD, pw_all);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "rate_idx=0x%x (REG0x%x) = 0x%x\n",
		  rate_idx, R_0x3a00 + rate_idx, pw_all);

	for (j = 0; j < 4; j++)
		config_phydm_read_txagc_diff_8822c(dm, rate_idx + j);

	return true;
}

__odm_func__
void phydm_txagc_tab_buff_init_8822c(struct dm_struct *dm)
{
	u8 i;

	for (i = 0; i < NUM_RATE_AC_2SS; i++) {
		dm->txagc_buff[RF_PATH_A][i] = i >> 2;
		dm->txagc_buff[RF_PATH_B][i] = i >> 2;
	}
}

__odm_func__
void phydm_txagc_tab_buff_show_8822c(struct dm_struct *dm)
{
	u8 i;

	pr_debug("path A\n");
	for (i = 0; i < NUM_RATE_AC_2SS; i++)
		pr_debug("[A][rate:%d] = %d\n", i,
			 dm->txagc_buff[RF_PATH_A][i]);
	pr_debug("path B\n");
	for (i = 0; i < NUM_RATE_AC_2SS; i++)
		pr_debug("[B][rate:%d] = %d\n", i,
			 dm->txagc_buff[RF_PATH_B][i]);
}
#endif

__odm_func__
void phydm_bb_reset_8822c(struct dm_struct *dm)
{
	if (*dm->mp_mode) 
		return;

	odm_set_mac_reg(dm, R_0x0, BIT(16), 1);
	odm_set_mac_reg(dm, R_0x0, BIT(16), 0);
	odm_set_mac_reg(dm, R_0x0, BIT(16), 1);
}

__odm_func__
void phydm_bb_reset_no_3wires_8822c(struct dm_struct *dm)
{
	/* Disable bbrstb 3-wires */
	odm_set_bb_reg(dm, R_0x1c90, BIT(8), 0x0);

	odm_set_mac_reg(dm, R_0x0, BIT(16), 1);
	odm_set_mac_reg(dm, R_0x0, BIT(16), 0);
	odm_set_mac_reg(dm, R_0x0, BIT(16), 1);

	/* Enable bbrstb 3-wires */
	odm_set_bb_reg(dm, R_0x1c90, BIT(8), 0x1);
}

__odm_func__
boolean phydm_chk_pkg_set_valid_8822c(struct dm_struct *dm,
				      u8 ver_bb, u8 ver_rf)
{
	boolean valid = true;

	if (ver_bb >= 41 && ver_rf >= 23)
		valid = true;
	else if (ver_bb < 41 && ver_rf < 23)
		valid = true;
	else
		valid = false;

	if (!valid) {
		odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x0);
		pr_debug("[Warning][%s] Pkg_ver{bb, rf}={%d, %d} disable all BB block\n",
			 __func__, ver_bb, ver_rf);
	}

	return valid;
}

__odm_func__
void phydm_igi_toggle_8822c(struct dm_struct *dm)
{
/*
 * @Toggle IGI to force BB HW send 3-wire-cmd and will let RF HW enter RX mode.
 * @Because BB HW does not send 3-wire command automacically when BB setting
 * @is changed including the configuration of path/channel/BW
 */
	u32 igi = 0x20;

	/* @Do not use PHYDM API to read/write because FW can not access */
	igi = odm_get_bb_reg(dm, R_0x1d70, 0x7f);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f, igi - 2);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f00, igi - 2);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f, igi);
	odm_set_bb_reg(dm, R_0x1d70, 0x7f00, igi);
}

__odm_func__
u32 phydm_check_bit_mask_8822c(u32 bit_mask, u32 data_original, u32 data)
{
	u8 bit_shift = 0;

	if (bit_mask != 0xfffff) {
		for (bit_shift = 0; bit_shift <= 19; bit_shift++) {
			if ((bit_mask >> bit_shift) & 0x1)
				break;
		}
		return (data_original & (~bit_mask)) | (data << bit_shift);
	}

	return data;
}

__odm_func__
u32 config_phydm_read_rf_reg_8822c(struct dm_struct *dm, enum rf_path path,
				   u32 reg_addr, u32 bit_mask)
{
	u32 readback_value = 0, direct_addr = 0;
	u32 offset_read_rf[2] = {R_0x3c00, R_0x4c00};

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Error handling.*/
	if (path > RF_PATH_B) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported path (%d)\n", path);
		return INVALID_RF_DATA;
	}

	/* @Calculate offset */
	reg_addr &= 0xff;
	direct_addr = offset_read_rf[path] + (reg_addr << 2);

	/* @RF register only has 20bits */
	bit_mask &= RFREG_MASK;

	/* @Read RF register directly */
	readback_value = odm_get_bb_reg(dm, direct_addr, bit_mask);
	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "RF-%d 0x%x = 0x%x, bit mask = 0x%x\n", path, reg_addr,
		  readback_value, bit_mask);
	return readback_value;
}

__odm_func__
boolean
config_phydm_direct_write_rf_reg_8822c(struct dm_struct *dm, enum rf_path path,
				       u32 reg_addr, u32 bit_mask, u32 data)
{
	u32 direct_addr = 0;
	u32 offset_write_rf[2] = {R_0x3c00, R_0x4c00};

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Calculate offset */
	reg_addr &= 0xff;
	direct_addr = offset_write_rf[path] + (reg_addr << 2);

	/* @RF register only has 20bits */
	bit_mask &= RFREG_MASK;

	/* @write RF register directly*/
	odm_set_bb_reg(dm, direct_addr, bit_mask, data);

	ODM_delay_us(1);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "RF-%d 0x%x = 0x%x , bit mask = 0x%x\n",
		  path, reg_addr, data, bit_mask);

	return true;
}

__odm_func__
boolean
config_phydm_write_rf_reg_8822c(struct dm_struct *dm, enum rf_path path,
				u32 reg_addr, u32 bit_mask, u32 data)
{
	u32 data_and_addr = 0, data_original = 0;
	u32 offset_write_rf[2] = {R_0x1808, R_0x4108};
	boolean result = false;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Error handling.*/
	if (path > RF_PATH_B) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Invalid path=%d\n", path);
		return false;
	}

	if (!(reg_addr == RF_0x0)) {
		result = config_phydm_direct_write_rf_reg_8822c(dm, path,
								reg_addr,
								bit_mask, data);
		return result;
	}

	/* @Read RF register content first */
	reg_addr &= 0xff;
	bit_mask &= RFREG_MASK;

	if (bit_mask != RFREG_MASK) {
		data_original = config_phydm_read_rf_reg_8822c(dm, path,
							       reg_addr,
							       RFREG_MASK);

		/* @Error handling. RF is disabled */
		if (!(data_original != INVALID_RF_DATA)) {
			PHYDM_DBG(dm, ODM_PHY_CONFIG,
				  "Write fail, RF is disable\n");
			return false;
		}

		/* @check bit mask */
		data = phydm_check_bit_mask_8822c(bit_mask, data_original,
						  data);
	}

	/* @Put write addr in [27:20] and write data in [19:00] */
	data_and_addr = ((reg_addr << 20) | (data & 0x000fffff)) & 0x0fffffff;

	/* @Write operation */
	odm_set_bb_reg(dm, offset_write_rf[path], MASKDWORD, data_and_addr);

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "RF-%d 0x%x = 0x%x (original: 0x%x), bit mask = 0x%x\n",
		  path, reg_addr, data, data_original, bit_mask);
#if (defined(CONFIG_RUN_IN_DRV))
	if (dm->support_interface == ODM_ITRF_PCIE)
		ODM_delay_us(13);
#elif (defined(CONFIG_RUN_IN_FW))
	ODM_delay_us(13);
#endif

	return true;
}

__odm_func__
boolean
phydm_write_txagc_1byte_8822c(struct dm_struct *dm, u32 pw_idx, u8 hw_rate)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8822C)

	u32 offset_txagc = R_0x3a00;
	u8 rate_idx = (hw_rate & 0xfc), i = 0;
	u8 rate_offset = (hw_rate & 0x3);
	u8 ret = 0;
	u32 txagc_idx = 0x0;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);
	/* @For debug command only!!!! */

	/* @bbrstb TX AGC report - default disable */
	/* @Enable for writing the TX AGC table when bb_reset=0 */
	odm_set_bb_reg(dm, R_0x1c90, BIT(15), 0x0);

	/* @Error handling */
	if (hw_rate > 0x53) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported rate\n");
		return false;
	}

	/* @For HW limitation, We can't write TXAGC once a byte. */
	for (i = 0; i < 4; i++) {
		if (i != rate_offset) {
			ret = config_phydm_read_txagc_diff_8822c(dm,
								 rate_idx + i);
			txagc_idx |= ret << (i << 3);
		} else {
			txagc_idx |= (pw_idx & 0x7f) << (i << 3);
		}
	}
	odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKDWORD, txagc_idx);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "rate_idx 0x%x (0x%x) = 0x%x\n",
		  hw_rate, (offset_txagc + hw_rate), txagc_idx);
	return true;
#else
	return false;
#endif
}

__odm_func__
boolean
config_phydm_write_txagc_ref_8822c(struct dm_struct *dm, u8 power_index,
				   enum rf_path path,
				   enum PDM_RATE_TYPE rate_type)
{
	/* @2-path power reference */
	u32 txagc_ofdm_ref[2] = {R_0x18e8, R_0x41e8};
	u32 txagc_cck_ref[2] = {R_0x18a0, R_0x41a0};

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Input need to be HW rate index, not driver rate index!!!! */
	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for debug\n");
		return true;
	}

	/* @Error handling */
	if (path > RF_PATH_B) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported path (%d)\n",
			  path);
		return false;
	}

	/* @bbrstb TX AGC report - default disable */
	/* @Enable for writing the TX AGC table when bb_reset=0 */
	odm_set_bb_reg(dm, R_0x1c90, BIT(15), 0x0);

	/* @According the rate to write in the ofdm or the cck */
	/* @CCK reference setting */
	if (rate_type == PDM_CCK) {
		odm_set_bb_reg(dm, txagc_cck_ref[path], 0x007f0000,
			       power_index);
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "path-%d rate type %d (0x%x) = 0x%x\n",
			  path, rate_type, txagc_cck_ref[path], power_index);

	/* @OFDM reference setting */
	} else {
		odm_set_bb_reg(dm, txagc_ofdm_ref[path], 0x0001fc00,
			       power_index);
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "path-%d rate type %d (0x%x) = 0x%x\n",
			  path, rate_type, txagc_ofdm_ref[path], power_index);
	}

	return true;
}

__odm_func__
boolean
config_phydm_write_txagc_diff_8822c(struct dm_struct *dm, s8 power_index1,
				    s8 power_index2, s8 power_index3,
				    s8 power_index4, u8 hw_rate)
{
	u32 offset_txagc = R_0x3a00;
	u8 rate_idx = hw_rate & 0xfc; /* @Extract the 0xfc */
	u8 power_idx1 = 0;
	u8 power_idx2 = 0;
	u8 power_idx3 = 0;
	u8 power_idx4 = 0;
	u32 pw_all = 0;

	power_idx1 = power_index1 & 0x7f;
	power_idx2 = power_index2 & 0x7f;
	power_idx3 = power_index3 & 0x7f;
	power_idx4 = power_index4 & 0x7f;
	pw_all = (power_idx4 << 24) | (power_idx3 << 16) | (power_idx2 << 8) |
		 power_idx1;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Input need to be HW rate index, not driver rate index!!!! */
	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for debug\n");
		return true;
	}

	/* @Error handling */
	if (hw_rate > ODM_RATEVHTSS2MCS9) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported rate\n");
		return false;
	}

	/* @bbrstb TX AGC report - default disable */
	/* @Enable for writing the TX AGC table when bb_reset=0 */
	odm_set_bb_reg(dm, R_0x1c90, BIT(15), 0x0);

	/* @According the rate to write in the ofdm or the cck */
	/* @driver need to construct a 4-byte power index */
	odm_set_bb_reg(dm, (offset_txagc + rate_idx), MASKDWORD, pw_all);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "rate index 0x%x (0x%x) = 0x%x\n",
		  hw_rate, (offset_txagc + hw_rate), pw_all);
	return true;
}

#if 1 /*Will remove when FW fill TXAGC funciton well verified*/
__odm_func__
void config_phydm_set_txagc_to_hw_8822c(struct dm_struct *dm)
{
#if (defined(CONFIG_RUN_IN_DRV))
	s8 diff_tab[2][NUM_RATE_AC_2SS]; /*power diff table of 2 paths*/
	s8 diff_tab_min[NUM_RATE_AC_2SS];
	u8 ref_pow_cck[2] = {dm->txagc_buff[RF_PATH_A][ODM_RATE11M],
			     dm->txagc_buff[RF_PATH_B][ODM_RATE11M]};
	u8 ref_pow_ofdm[2] = {dm->txagc_buff[RF_PATH_A][ODM_RATEMCS7],
			      dm->txagc_buff[RF_PATH_B][ODM_RATEMCS7]};
	u8 ref_pow_tmp = 0;
	enum rf_path path = 0;
	u8 i, j = 0;

	if (*dm->is_fcs_mode_enable)
		return;

	/* === [Reference base] =============================================*/
#ifdef CONFIG_TXAGC_DEBUG_8822C
	pr_debug("ref_pow_cck={%d, %d}, ref_pow_ofdm={%d, %d}\n",
		 ref_pow_cck[0], ref_pow_cck[1], ref_pow_ofdm[0],
		 ref_pow_ofdm[1]);
#endif
	/*Set OFDM/CCK Ref. power index*/
	config_phydm_write_txagc_ref_8822c(dm, ref_pow_cck[0], RF_PATH_A,
					   PDM_CCK);
	config_phydm_write_txagc_ref_8822c(dm, ref_pow_cck[1], RF_PATH_B,
					   PDM_CCK);
	config_phydm_write_txagc_ref_8822c(dm, ref_pow_ofdm[0], RF_PATH_A,
					   PDM_OFDM);
	config_phydm_write_txagc_ref_8822c(dm, ref_pow_ofdm[1], RF_PATH_B,
					   PDM_OFDM);

	/* === [Power By Rate] ==============================================*/
	for (path = RF_PATH_A; path <= RF_PATH_B; path++)
		odm_move_memory(dm, diff_tab[path], dm->txagc_buff[path],
				NUM_RATE_AC_2SS);

#ifdef CONFIG_TXAGC_DEBUG_8822C
	pr_debug("1. diff_tab path A\n");
	for (i = 0; i <= ODM_RATEVHTSS2MCS9; i++)
		pr_debug("[A][rate:%d] = %d\n", i, diff_tab[RF_PATH_A][i]);
	pr_debug("2. diff_tab path B\n");
	for (i = 0; i <= ODM_RATEVHTSS2MCS9; i++)
		pr_debug("[B][rate:%d] = %d\n", i, diff_tab[RF_PATH_B][i]);
#endif

	for (path = RF_PATH_A; path <= RF_PATH_B; path++) {
		/*CCK*/
		ref_pow_tmp = ref_pow_cck[path];
		for (j = ODM_RATE1M; j <= ODM_RATE11M; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
		/*OFDM*/
		ref_pow_tmp = ref_pow_ofdm[path];
		for (j = ODM_RATE6M; j <= ODM_RATEMCS15; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
		for (j = ODM_RATEVHTSS1MCS0; j <= ODM_RATEVHTSS2MCS9; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
	}

#ifdef CONFIG_TXAGC_DEBUG_8822C
	pr_debug("3. diff_tab path A\n");
	for (i = 0; i <= ODM_RATEVHTSS2MCS9; i++)
		pr_debug("[A][rate:%d] = %d\n", i, diff_tab[RF_PATH_A][i]);
	pr_debug("4. diff_tab path B\n");
	for (i = 0; i <= ODM_RATEVHTSS2MCS9; i++)
		pr_debug("[B][rate:%d] = %d\n", i, diff_tab[RF_PATH_B][i]);
#endif

	for (i = ODM_RATE1M; i <= ODM_RATEMCS15; i++) {
		diff_tab_min[i] = MIN_2(diff_tab[RF_PATH_A][i],
					diff_tab[RF_PATH_B][i]);
		#ifdef CONFIG_TXAGC_DEBUG_8822C
		pr_debug("diff_tab_min[rate:%d]= %d\n", i, diff_tab_min[i]);
		#endif
		if  (i % 4 == 3) {
			config_phydm_write_txagc_diff_8822c(dm,
							    diff_tab_min[i - 3],
							    diff_tab_min[i - 2],
							    diff_tab_min[i - 1],
							    diff_tab_min[i],
							    i - 3);
		}
	}

	for (i = ODM_RATEVHTSS1MCS0; i <= ODM_RATEVHTSS2MCS9; i++) {
		diff_tab_min[i] = MIN_2(diff_tab[RF_PATH_A][i],
					diff_tab[RF_PATH_B][i]);
		#ifdef CONFIG_TXAGC_DEBUG_8822C
		pr_debug("diff_tab_min[rate:%d]= %d\n", i, diff_tab_min[i]);
		#endif
		if  (i % 4 == 3) {
			config_phydm_write_txagc_diff_8822c(dm,
							    diff_tab_min[i - 3],
							    diff_tab_min[i - 2],
							    diff_tab_min[i - 1],
							    diff_tab_min[i],
							    i - 3);
		}
	}
#endif
}

__odm_func__
boolean config_phydm_write_txagc_8822c(struct dm_struct *dm, u32 pw_idx,
				       enum rf_path path, u8 hw_rate)
{
#if (defined(CONFIG_RUN_IN_DRV))
	u8 ref_rate = ODM_RATEMCS15;
	u8 rate;
	u8 fill_valid_cnt = 0;
	u8 i = 0;

	if (*dm->is_fcs_mode_enable)
		return false;

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for debug\n");
		return true;
	}

	if (path > RF_PATH_B) {
		pr_debug("[Warning 1] %s\n", __func__);
		return false;
	}

	if ((hw_rate > ODM_RATEMCS15 && hw_rate <= ODM_RATEMCS31) ||
	    hw_rate > ODM_RATEVHTSS2MCS9) {
		pr_debug("[Warning 2] %s\n", __func__);
		return false;
	}

	if (hw_rate <= ODM_RATEMCS15)
		ref_rate = ODM_RATEMCS15;
	else
		ref_rate = ODM_RATEVHTSS2MCS9;

	fill_valid_cnt = ref_rate - hw_rate + 1;
	if (fill_valid_cnt > 4)
		fill_valid_cnt = 4;

	for (i = 0; i < fill_valid_cnt; i++) {
		rate = hw_rate + i;
		if (rate > (PHY_NUM_RATE_IDX - 1)) /*Just for protection*/
			break;

		dm->txagc_buff[path][rate] = (u8)((pw_idx >> (8 * i)) & 0xff);
	}
#endif
	return true;
}
#endif

#if 1 /*API for FW fill txagc*/
__odm_func__
void phydm_set_txagc_by_table_8822c(struct dm_struct *dm,
				    struct txagc_table_8822c *tab)
{
#if (defined(CONFIG_RUN_IN_FW))
	u8 i = 0;

	/* === [Reference base] =============================================*/
	/*Set OFDM/CCK Ref. power index*/
	config_phydm_write_txagc_ref_8822c(dm, tab->ref_pow_cck[0], RF_PATH_A,
					   PDM_CCK);
	config_phydm_write_txagc_ref_8822c(dm, tab->ref_pow_cck[1], RF_PATH_B,
					   PDM_CCK);
	config_phydm_write_txagc_ref_8822c(dm, tab->ref_pow_ofdm[0], RF_PATH_A,
					   PDM_OFDM);
	config_phydm_write_txagc_ref_8822c(dm, tab->ref_pow_ofdm[1], RF_PATH_B,
					   PDM_OFDM);

	for (i = ODM_RATE1M; i <= ODM_RATEMCS15; i++) {
		if  (i % 4 == 3) {
			config_phydm_write_txagc_diff_8822c(dm,
							    tab->diff_t[i - 3],
							    tab->diff_t[i - 2],
							    tab->diff_t[i - 1],
							    tab->diff_t[i],
							    i - 3);
		}
	}

	for (i = ODM_RATEVHTSS1MCS0; i <= ODM_RATEVHTSS2MCS9; i++) {
		if  (i % 4 == 3) {
			config_phydm_write_txagc_diff_8822c(dm,
							    tab->diff_t[i - 3],
							    tab->diff_t[i - 2],
							    tab->diff_t[i - 1],
							    tab->diff_t[i],
							    i - 3);
		}
	}
#endif
}

__odm_func__
void phydm_get_txagc_ref_and_diff_8822c(struct dm_struct *dm,
					u8 txagc_buff[2][NUM_RATE_AC_2SS],
					u16 length,
					struct txagc_table_8822c *tab)
{
#if (defined(CONFIG_RUN_IN_DRV))
	s8 diff_tab[2][NUM_RATE_AC_2SS]; /*power diff table of 2 paths*/
	s8 diff_tab_min[NUM_RATE_AC_2SS];
	u8 ref_pow_cck[2];
	u8 ref_pow_ofdm[2];
	u8 ref_pow_tmp = 0;
	enum rf_path path = 0;
	u8 i, j = 0;

	if (*dm->mp_mode || !dm->is_download_fw)
		return;

	if (length != NUM_RATE_AC_2SS) {
		pr_debug("[warning] %s\n", __func__);
		return;
	}

	/* === [Reference base] =============================================*/
#ifdef CONFIG_TXAGC_DEBUG_8822C
	pr_debug("ref_pow_cck={%d, %d}, ref_pow_ofdm={%d, %d}\n",
		 ref_pow_cck[0], ref_pow_cck[1], ref_pow_ofdm[0],
		 ref_pow_ofdm[1]);
#endif

	/* === [Power By Rate] ==============================================*/
	for (path = RF_PATH_A; path <= RF_PATH_B; path++)
		odm_move_memory(dm, diff_tab[path], txagc_buff[path],
				NUM_RATE_AC_2SS);

	ref_pow_cck[0] = diff_tab[RF_PATH_A][ODM_RATE11M];
	ref_pow_cck[1] = diff_tab[RF_PATH_B][ODM_RATE11M];

	ref_pow_ofdm[0] = diff_tab[RF_PATH_A][ODM_RATEMCS7];
	ref_pow_ofdm[1] = diff_tab[RF_PATH_B][ODM_RATEMCS7];

#ifdef CONFIG_TXAGC_DEBUG_8822C
	pr_debug("1. diff_tab path A\n");
	for (i = 0; i <= ODM_RATEVHTSS2MCS9; i++)
		pr_debug("[A][rate:%d] = %d\n", i, diff_tab[RF_PATH_A][i]);
	pr_debug("2. diff_tab path B\n");
	for (i = 0; i <= ODM_RATEVHTSS2MCS9; i++)
		pr_debug("[B][rate:%d] = %d\n", i, diff_tab[RF_PATH_B][i]);
#endif

	for (path = RF_PATH_A; path <= RF_PATH_B; path++) {
		/*CCK*/
		ref_pow_tmp = ref_pow_cck[path];
		for (j = ODM_RATE1M; j <= ODM_RATE11M; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
		/*OFDM*/
		ref_pow_tmp = ref_pow_ofdm[path];
		for (j = ODM_RATE6M; j <= ODM_RATEMCS15; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
		for (j = ODM_RATEVHTSS1MCS0; j <= ODM_RATEVHTSS2MCS9; j++) {
			diff_tab[path][j] -= (s8)ref_pow_tmp;
			/**/
		}
	}

#ifdef CONFIG_TXAGC_DEBUG_8822C
	pr_debug("3. diff_tab path A\n");
	for (i = 0; i <= ODM_RATEVHTSS2MCS9; i++)
		pr_debug("[A][rate:%d] = %d\n", i, diff_tab[RF_PATH_A][i]);
	pr_debug("4. diff_tab path B\n");
	for (i = 0; i <= ODM_RATEVHTSS2MCS9; i++)
		pr_debug("[B][rate:%d] = %d\n", i, diff_tab[RF_PATH_B][i]);
#endif

	for (i = ODM_RATE1M; i <= ODM_RATEMCS15; i++) {
		diff_tab_min[i] = MIN_2(diff_tab[RF_PATH_A][i],
					diff_tab[RF_PATH_B][i]);
		#ifdef CONFIG_TXAGC_DEBUG_8822C
		pr_debug("diff_tab_min[rate:%d]= %d\n", i, diff_tab_min[i]);
		#endif
	}

	for (i = ODM_RATEVHTSS1MCS0; i <= ODM_RATEVHTSS2MCS9; i++) {
		diff_tab_min[i] = MIN_2(diff_tab[RF_PATH_A][i],
					diff_tab[RF_PATH_B][i]);
		#ifdef CONFIG_TXAGC_DEBUG_8822C
		pr_debug("diff_tab_min[rate:%d]= %d\n", i, diff_tab_min[i]);
		#endif
	}

	odm_move_memory(dm, tab->ref_pow_cck, ref_pow_cck, 2);
	odm_move_memory(dm, tab->ref_pow_ofdm, ref_pow_ofdm, 2);
	odm_move_memory(dm, tab->diff_t, diff_tab_min, NUM_RATE_AC_2SS);
#endif
}
#endif

__odm_func__
s8 config_phydm_read_txagc_diff_8822c(struct dm_struct *dm, u8 hw_rate)
{
#if (PHYDM_FW_API_FUNC_ENABLE_8822C)
	s8 read_back_data = 0;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Input need to be HW rate index, not driver rate index!!!! */

	/* @Error handling */
	if (hw_rate > 0x53) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported rate\n");
		return INVALID_TXAGC_DATA;
	}

	/* @Disable TX AGC report */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x0); /* need to check */

	/* @Set data rate index (bit30~24) */
	odm_set_bb_reg(dm, R_0x1c7c, 0x7F000000, hw_rate);

	/* @Enable TXAGC report */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x1);

	/* @Read TX AGC report */
	read_back_data = (s8)odm_get_bb_reg(dm, R_0x2de8, 0xff);
	if (read_back_data & BIT(6))
		read_back_data |= BIT(7);

	/* @Driver have to disable TXAGC report after reading TXAGC */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x0);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "rate index 0x%x = 0x%x\n", hw_rate,
		  read_back_data);
	return read_back_data;
#else
	return 0;
#endif
}

__odm_func__
u8 config_phydm_read_txagc_8822c(struct dm_struct *dm, enum rf_path path,
				 u8 hw_rate, enum PDM_RATE_TYPE rate_type)
{
	s8 read_back_data = 0;
	u8 ref_data = 0;
	u8 result_data = 0;
	/* @2-path power reference */
	u32 r_txagc_ofdm[2] = {R_0x18e8, R_0x41e8};
	u32 r_txagc_cck[2] = {R_0x18a0, R_0x41a0};

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	/* @Input need to be HW rate index, not driver rate index!!!! */

	/* @Error handling */
	if (path > RF_PATH_B || hw_rate > 0x53) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Unsupported path (%d)\n", path);
		return INVALID_TXAGC_DATA;
	}

	/* @Disable TX AGC report */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x0); /* need to check */

	/* @Set data rate index (bit30~24) */
	odm_set_bb_reg(dm, R_0x1c7c, 0x7F000000, hw_rate);

	/* @Enable TXAGC report */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x1);

	/* @Read power difference report */
	read_back_data = (s8)odm_get_bb_reg(dm, R_0x2de8, 0xff);
	if (read_back_data & BIT(6))
		read_back_data |= BIT(7);

	/* @Read power reference value report */
	if (rate_type == PDM_CCK) /* @Bit=22:16 */
		ref_data = (u8)odm_get_bb_reg(dm, r_txagc_cck[path], 0x7F0000);
	else if (rate_type == PDM_OFDM) /* @Bit=16:10 */
		ref_data = (u8)odm_get_bb_reg(dm, r_txagc_ofdm[path], 0x1FC00);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "diff=%d ref=%d\n", read_back_data,
		  ref_data);

	if (read_back_data + ref_data < 0)
		result_data = 0;
	else
		result_data = read_back_data + ref_data;

	/* @Driver have to disable TXAGC report after reading TXAGC */
	odm_set_bb_reg(dm, R_0x1c7c, BIT(23), 0x0);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "path-%d rate index 0x%x = 0x%x\n",
		  path, hw_rate, result_data);
	return result_data;
}

__odm_func__
void
phydm_get_tx_path_en_setting_8822c(struct dm_struct *dm,
				   struct tx_path_en_8822c *path)
{
	u32 val = 0;
#ifdef CONFIG_PATH_DIVERSITY
	struct _ODM_PATH_DIVERSITY_ *p_div = &dm->dm_path_div;
#endif

	/*OFDM*/
	val = odm_get_bb_reg(dm, R_0x820, MASKDWORD);
	path->tx_path_en_ofdm_2sts = (u8)((val & 0xf0) >> 4);
	path->tx_path_en_ofdm_1sts = (u8)(val & 0xf);

	/*CCK*/
	val = odm_get_bb_reg(dm, R_0x1a04, 0xf0000000);

	if (val == 0xc)
		path->tx_path_en_cck = 3; /*AB*/
	else if (val == 0x8)
		path->tx_path_en_cck = 1; /*A*/
	else if (val == 0x4)
		path->tx_path_en_cck = 2; /*B*/
	else if (val == 0x0)
		path->tx_path_en_cck = 0; /*disable cck tx in 5G*/

	/*Path CTRL source*/
	val = odm_get_bb_reg(dm, R_0x1e24, BIT(16));
	path->is_path_ctrl_by_bb_reg = (boolean)(~val);

	/*stop path div*/
#ifdef CONFIG_PATH_DIVERSITY
	path->stop_path_div = p_div->stop_path_div;
#else
	path->stop_path_div = true;
#endif
}

__odm_func__
void
phydm_get_rx_path_en_setting_8822c(struct dm_struct *dm,
				   struct rx_path_en_8822c *path)
{
	u32 val = 0;

	/*OFDM*/
	path->rx_path_en_ofdm = (u8)odm_get_bb_reg(dm, R_0x824, 0xf0000);

	/*CCK*/
	val = odm_get_bb_reg(dm, R_0x1a04, 0x0f000000);

	if (val == 0x1)
		path->rx_path_en_cck = 3; /*AB*/
	else if (val == 0x0)
		path->rx_path_en_cck = 1; /*A*/
	else if (val == 0x5)
		path->rx_path_en_cck = 2; /*B*/
}

__odm_func__
void
phydm_config_cck_tx_path_8822c(struct dm_struct *dm, enum bb_path tx_path)
{
	if (tx_path == BB_PATH_A)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x8);
	else if (tx_path == BB_PATH_B)
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0x4);
	else /* if (tx_path == BB_PATH_AB)*/
		odm_set_bb_reg(dm, R_0x1a04, 0xf0000000, 0xc);

	phydm_bb_reset_8822c(dm);
}

__odm_func__
boolean
phydm_config_cck_rx_path_8822c(struct dm_struct *dm, enum bb_path rx_path)
{
	boolean set_result = PHYDM_SET_FAIL;

	if (rx_path == BB_PATH_A) {
		/* Select ant_A to receive CCK_1 and CCK_2*/
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x0);
		/* Enable Rx clk gated */
		odm_set_bb_reg(dm, R_0x1a2c, BIT(5), 0x0);
		/* Disable MRC for CCK barker */
		odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x0);
		/* Disable MRC for CCK CCA */
		odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x0);
		/* 2R CS ratio setting*/
		odm_set_bb_reg(dm, R_0x1ad0, 0x3e0, 0xd);
	} else if (rx_path == BB_PATH_B) {
		/* Select ant_B to receive CCK_1 and CCK_2*/
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x5);
		/* Disable Rx clk gated */
		odm_set_bb_reg(dm, R_0x1a2c, BIT(5), 0x1);
		/* replace path-B with path-AB: [PHYDM-336]*/
		/* Disable MRC for CCK barker */
		odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x0);
		/* Eable MRC for CCK CCA */
		odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x1);
		/* 2R CS ratio setting*/
		odm_set_bb_reg(dm, R_0x1ad0, 0x3e0, 0xf);
	} else if (rx_path == BB_PATH_AB) {
		/* Select ant_A to receive CCK_1 and ant_B to receive CCK_2*/
		odm_set_bb_reg(dm, R_0x1a04, 0x0f000000, 0x1);
		/* Enable Rx clk gated */
		odm_set_bb_reg(dm, R_0x1a2c, BIT(5), 0x0);
		/* Enable MRC for CCK barker */
		odm_set_bb_reg(dm, R_0x1a2c, 0x00060000, 0x1);
		/* Eable MRC for CCK CCA */
		odm_set_bb_reg(dm, R_0x1a2c, 0x00600000, 0x1);
		/* 2R CS ratio setting*/
		odm_set_bb_reg(dm, R_0x1ad0, 0x3e0, 0xd);
	}

	set_result = PHYDM_SET_SUCCESS;

	phydm_bb_reset_8822c(dm);
	return set_result;
}

__odm_func__
void
phydm_config_ofdm_tx_path_8822c(struct dm_struct *dm, enum bb_path tx_path_2ss,
				enum bb_path tx_path_sel_1ss)
{
	u8 tx_path_2ss_en = false;

	if (tx_path_2ss == BB_PATH_AB)
		tx_path_2ss_en = true;

	if (!tx_path_2ss_en) {/* 1ss1T, do not config this with STBC*/
		if (tx_path_sel_1ss == BB_PATH_A) {
			odm_set_bb_reg(dm, R_0x820, 0xff, 0x1);
			odm_set_bb_reg(dm, R_0x1e2c, 0xffff, 0x0);
		} else { /*if (tx_path_sel_1ss == BB_PATH_B)*/
			odm_set_bb_reg(dm, R_0x820, 0xff, 0x2);
			odm_set_bb_reg(dm, R_0x1e2c, 0xffff, 0x0);
		}
	} else {
		if (tx_path_sel_1ss == BB_PATH_A) {
			odm_set_bb_reg(dm, R_0x820, 0xff, 0x31);
			odm_set_bb_reg(dm, R_0x1e2c, 0xffff, 0x0400);
		} else if (tx_path_sel_1ss == BB_PATH_B) {
			odm_set_bb_reg(dm, R_0x820, 0xff, 0x32);
			odm_set_bb_reg(dm, R_0x1e2c, 0xffff, 0x0400);
		} else { /*BB_PATH_AB*/
			odm_set_bb_reg(dm, R_0x820, 0xff, 0x33);
			odm_set_bb_reg(dm, R_0x1e2c, 0xffff, 0x0404);
		}
	}

	phydm_bb_reset_8822c(dm);
}

__odm_func__
void
phydm_config_ofdm_rx_path_8822c(struct dm_struct *dm, enum bb_path rx_path)
{
	u32 ofdm_rx = 0x0;

	ofdm_rx = (u32)rx_path;
	if (!(*dm->mp_mode)) {
		if (ofdm_rx == BB_PATH_B) {
			ofdm_rx = BB_PATH_AB;
			odm_set_bb_reg(dm, R_0xcc0, 0x7ff, 0x0);
			odm_set_bb_reg(dm, R_0xcc0, BIT(22), 0x1);
			odm_set_bb_reg(dm, R_0xcc8, 0x7ff, 0x0);
			odm_set_bb_reg(dm, R_0xcc8, BIT(22), 0x1);
		} else { /* ofdm_rx == BB_PATH_A || ofdm_rx == BB_PATH_AB*/
			odm_set_bb_reg(dm, R_0xcc0, 0x7ff, 0x400);
			odm_set_bb_reg(dm, R_0xcc0, BIT(22), 0x0);
			odm_set_bb_reg(dm, R_0xcc8, 0x7ff, 0x400);
			odm_set_bb_reg(dm, R_0xcc8, BIT(22), 0x0);
		}
	}

	if (ofdm_rx == BB_PATH_A || ofdm_rx == BB_PATH_B) {
		/*@ ht_mcs_limit*/
		odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x0);
		/*@ vht_nss_limit*/
		odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x0);
		/* @Disable Antenna weighting */
		odm_set_bb_reg(dm, R_0xc44, BIT(17), 0x0);
		/* @htstf ant-wgt enable = 0*/
		odm_set_bb_reg(dm, R_0xc54, BIT(20), 0x0);
		/* @MRC_mode = 'original ZF eqz'*/
		odm_set_bb_reg(dm, R_0xc38, BIT(24), 0x0);
		/* @Rx_ant */
		odm_set_bb_reg(dm, R_0x824, 0x000f0000, rx_path);
		/* @Rx_CCA*/
		odm_set_bb_reg(dm, R_0x824, 0x0f000000, rx_path);
	} else if (ofdm_rx == BB_PATH_AB) {
		/*@ ht_mcs_limit*/
		odm_set_bb_reg(dm, R_0x1d30, 0x300, 0x1);
		/*@ vht_nss_limit*/
		odm_set_bb_reg(dm, R_0x1d30, 0x600000, 0x1);
		/* @Enable Antenna weighting */
		odm_set_bb_reg(dm, R_0xc44, BIT(17), 0x1);
		/* @htstf ant-wgt enable = 1*/
		odm_set_bb_reg(dm, R_0xc54, BIT(20), 0x1);
		/* @MRC_mode = 'modified ZF eqz'*/
		odm_set_bb_reg(dm, R_0xc38, BIT(24), 0x1);
		/* @Rx_ant */
		odm_set_bb_reg(dm, R_0x824, 0x000f0000, BB_PATH_AB);
		/* @Rx_CCA*/
		odm_set_bb_reg(dm, R_0x824, 0x0f000000, BB_PATH_AB);
	}

	phydm_bb_reset_8822c(dm);
}

__odm_func__
void phydm_config_tx_path_8822c(struct dm_struct *dm, enum bb_path tx_path_2ss,
				enum bb_path tx_path_sel_1ss,
				enum bb_path tx_path_sel_cck)
{
	dm->tx_2ss_status = tx_path_2ss;
	dm->tx_1ss_status = tx_path_sel_1ss;

	dm->tx_ant_status = dm->tx_2ss_status | dm->tx_1ss_status;

	/* @CCK TX antenna mapping */
	phydm_config_cck_tx_path_8822c(dm, tx_path_sel_cck);

	/* @OFDM TX antenna mapping*/
	phydm_config_ofdm_tx_path_8822c(dm, tx_path_2ss, tx_path_sel_1ss);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "path_sel_2ss/1ss/cck={%d, %d, %d}\n",
		  tx_path_2ss, tx_path_sel_1ss, tx_path_sel_cck);

	phydm_bb_reset_8822c(dm);
}

__odm_func__
void phydm_config_rx_path_8822c(struct dm_struct *dm, enum bb_path rx_path)
{
	/* @CCK RX antenna mapping */
	phydm_config_cck_rx_path_8822c(dm, rx_path);

	/* @OFDM RX antenna mapping*/
	phydm_config_ofdm_rx_path_8822c(dm, rx_path);

	dm->rx_ant_status = rx_path;

	phydm_bb_reset_8822c(dm);
}

__odm_func__
void
phydm_set_rf_mode_table_8822c(struct dm_struct *dm,
			      enum bb_path tx_path_mode_table,
			      enum bb_path rx_path)
{
	 /* @Cannot shut down path-A, beacause synthesizer will shut down
	  * @when path-A is in shut down mode
	  */

	/* @[3-wire setting]  0: shutdown, 1: standby, 2: TX, 3: RX*/
	if (tx_path_mode_table == BB_PATH_A) {
		if (rx_path == BB_PATH_A) {
			odm_set_bb_reg(dm, R_0x1800, 0xfffff, 0x33312);
			odm_set_bb_reg(dm, R_0x4100, 0xfffff, 0x0);
		} else { /* @BB_PATH_AB*/
			odm_set_bb_reg(dm, R_0x1800, 0xfffff, 0x33312);
			odm_set_bb_reg(dm, R_0x4100, 0xfffff, 0x33311);
		}
	} else if (tx_path_mode_table == BB_PATH_B) {
		if (rx_path == BB_PATH_A) {
			odm_set_bb_reg(dm, R_0x1800, 0xfffff, 0x33311);
			odm_set_bb_reg(dm, R_0x4100, 0xfffff, 0x11112);
		} else {
			odm_set_bb_reg(dm, R_0x1800, 0xfffff, 0x33311);
			odm_set_bb_reg(dm, R_0x4100, 0xfffff, 0x33312);
		}
	} else if (tx_path_mode_table == BB_PATH_AB) {
		if (rx_path == BB_PATH_A) {
			odm_set_bb_reg(dm, R_0x1800, 0xfffff, 0x33312);
			odm_set_bb_reg(dm, R_0x4100, 0xfffff, 0x11112);
		} else {
			odm_set_bb_reg(dm, R_0x1800, 0xfffff, 0x33312);
			odm_set_bb_reg(dm, R_0x4100, 0xfffff, 0x33312);
		}
	}
}

__odm_func__
void
phydm_rfe_8822c(struct dm_struct *dm, enum bb_path path)
{
	u8 rfe_type = dm->rfe_type;
	u32 rf_reg18 = 0;
	u8 central_ch = 0;
	boolean is_2g_ch = false;

	rf_reg18 = config_phydm_read_rf_reg_8822c(dm, RF_PATH_A, RF_0x18,
						  RFREG_MASK);
	central_ch = (u8)(rf_reg18 & 0xff);
	is_2g_ch = (central_ch <= 14) ? true : false;

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "[8822C] Update RFE PINs: T/RX_path:{0x%x, 0x%x}, rfe_type:%d\n",
		  dm->tx_ant_status, dm->rx_ant_status, rfe_type);

	/*HW Setting for each RFE type */
	if (rfe_type == 21 || rfe_type == 22) {
		/*rfe sel*/
		/*0 : PAPE_2G_rfm*/
		/*2 : LNAON_2G*/
		/*3 : rfm_lnaon*/
		/*7 : 1'b0*/
		if (is_2g_ch)
			path = BB_PATH_NON;

		switch (path) {
		case BB_PATH_NON:
			odm_set_bb_reg(dm, R_0x1840, 0xffff, 0x7770);
			odm_set_bb_reg(dm, R_0x4144, 0xffff, 0x7077);
			break;
		case BB_PATH_A:
			odm_set_bb_reg(dm, R_0x1840, 0xffff, 0x2300);
			odm_set_bb_reg(dm, R_0x4144, 0xffff, 0x7077);
			break;
		case BB_PATH_B:
			odm_set_bb_reg(dm, R_0x1840, 0xffff, 0x7770);
			odm_set_bb_reg(dm, R_0x4144, 0xffff, 0x2030);
			break;
		case BB_PATH_AB:
			odm_set_bb_reg(dm, R_0x1840, 0xffff, 0x2300);
			odm_set_bb_reg(dm, R_0x4144, 0xffff, 0x2030);
			break;
		default:
			break;
		}
	}
}

__odm_func__
boolean
config_phydm_trx_mode_8822c(struct dm_struct *dm, enum bb_path tx_path_en,
			    enum bb_path rx_path, enum bb_path tx_path_sel_1ss)
{
#ifdef CONFIG_PATH_DIVERSITY
	struct _ODM_PATH_DIVERSITY_ *p_div = &dm->dm_path_div;
#endif
	boolean disable_2sts_div_mode = false;
	enum bb_path tx_path_mode_table = tx_path_en;
	enum bb_path tx_path_2ss = BB_PATH_AB;
	u8 rfe_type = dm->rfe_type;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (dm->is_disable_phy_api) {
		pr_debug("[%s] Disable PHY API\n", __func__);
		return true;
	}

	/*RX Check*/
	if (rx_path & ~BB_PATH_AB) {
		pr_debug("[Warning][%s] RX:0x%x\n", __func__, rx_path);
		return false;
	}

	/*TX Check*/
	if (tx_path_en == BB_PATH_AUTO && tx_path_sel_1ss == BB_PATH_AUTO) {
		/*@ Shutting down 2sts rate, but 1sts PathDiv is enabled*/
		disable_2sts_div_mode = true;
		tx_path_mode_table = BB_PATH_AB;
	} else if (tx_path_en & ~BB_PATH_AB) {
		pr_debug("[Warning][%s] TX:0x%x\n", __func__, tx_path_en);
		return false;
	}

	/* @==== [RF Mode Table] ========================================*/
	phydm_set_rf_mode_table_8822c(dm, tx_path_mode_table, rx_path);

	/* @==== [RX Path] ==============================================*/
	phydm_config_rx_path_8822c(dm, rx_path);

	/* @==== [TX Path] ==============================================*/
#ifdef CONFIG_PATH_DIVERSITY
	/*@ [PHYDM-312]*/
	if (p_div->default_tx_path != BB_PATH_A &&
	    p_div->default_tx_path != BB_PATH_B)
		p_div->default_tx_path = BB_PATH_A;

	if (tx_path_en == BB_PATH_A || tx_path_en == BB_PATH_B) {
		p_div->stop_path_div = true;
		tx_path_sel_1ss = tx_path_en;
		tx_path_2ss = BB_PATH_NON;
	} else if (tx_path_en == BB_PATH_AB) {
		if (tx_path_sel_1ss == BB_PATH_AUTO) {
			p_div->stop_path_div = false;
			tx_path_sel_1ss = p_div->default_tx_path;
		} else { /* @BB_PATH_AB, BB_PATH_A, BB_PATH_B*/
			p_div->stop_path_div = true;
		}
		tx_path_2ss = BB_PATH_AB;
	} else if (disable_2sts_div_mode) {
		p_div->stop_path_div = false;
		tx_path_sel_1ss = p_div->default_tx_path;
		tx_path_2ss = BB_PATH_NON;
	}
#else
	if (dm->tx_1ss_status == BB_PATH_NON)
		dm->tx_1ss_status = BB_PATH_A;

	if (tx_path_en == BB_PATH_A || tx_path_en == BB_PATH_B) {
		tx_path_2ss = BB_PATH_NON;
		tx_path_sel_1ss = tx_path_en;
	} else if (tx_path_en == BB_PATH_AB) {
		tx_path_2ss = BB_PATH_AB;
		if (tx_path_sel_1ss == BB_PATH_AUTO)
			tx_path_sel_1ss = dm->tx_1ss_status;
	} else if (disable_2sts_div_mode) {
		tx_path_2ss = BB_PATH_NON;
		tx_path_sel_1ss = dm->tx_1ss_status;
	}
#endif
	phydm_config_tx_path_8822c(dm, tx_path_2ss, tx_path_sel_1ss,
				   tx_path_sel_1ss);

	/*====== [RFE ctrl] =============================================*/
	if (rfe_type == 21 || rfe_type == 22) {
		if (dm->tx_ant_status == BB_PATH_A && rx_path == BB_PATH_A)
			phydm_rfe_8822c(dm, BB_PATH_A);
		else if (dm->tx_ant_status == BB_PATH_B && rx_path == BB_PATH_B)
			phydm_rfe_8822c(dm, BB_PATH_B);
		else
			phydm_rfe_8822c(dm, BB_PATH_AB);
	}

	phydm_igi_toggle_8822c(dm);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "RX_en=%x, tx_en/2ss/1ss={%x,%x,%x}\n",
		  rx_path, tx_path_en, tx_path_2ss, tx_path_sel_1ss);

	phydm_bb_reset_8822c(dm);

	return true;
}

void phydm_cck_rxiq_8822c(struct dm_struct *dm, u8 set_type)
{
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (set_type == PHYDM_SET) {
		/* @ CCK source 5*/
		odm_set_bb_reg(dm, R_0x1a9c, BIT(20), 0x1);
		/* @ CCK RxIQ weighting = [1,1] */
		odm_set_bb_reg(dm, R_0x1a14, 0x300, 0x0);
	} else if (set_type == PHYDM_REVERT) {
		/* @ CCK source 1*/
		odm_set_bb_reg(dm, R_0x1a9c, BIT(20), 0x0);
		/* @ CCK RxIQ weighting = [0,0] */
		odm_set_bb_reg(dm, R_0x1a14, 0x300, 0x3);
	}
}

__odm_func__
boolean
config_phydm_switch_band_8822c(struct dm_struct *dm, u8 central_ch)
{
	return true;
}

__odm_func__
void
phydm_cck_tx_shaping_filter_8822c(struct dm_struct *dm, u8 central_ch)
{
	/* @CCK TX filter parameters */
	if (central_ch == 14) {
		/* @TX Shaping Filter C0~1 */
		odm_set_bb_reg(dm, R_0x1a20, MASKHWORD, 0x3da0);
		/* @TX Shaping Filter C2~5 */
		odm_set_bb_reg(dm, R_0x1a24, MASKDWORD, 0x4962c931);
		/* @TX Shaping Filter C6~7 */
		odm_set_bb_reg(dm, R_0x1a28, MASKLWORD, 0x6aa3);
		/* @TX Shaping Filter C8~9 */
		odm_set_bb_reg(dm, R_0x1a98, MASKHWORD, 0xaa7b);
		/* @TX Shaping Filter C10~11 */
		odm_set_bb_reg(dm, R_0x1a9c, MASKLWORD, 0xf3d7);
		/* @TX Shaping Filter C12~15 */
		odm_set_bb_reg(dm, R_0x1aa0, MASKDWORD, 0x00000000);
		/* @TX Shaping Filter_MSB C0~7 */
		odm_set_bb_reg(dm, R_0x1aac, MASKDWORD, 0xff012455);
		/* @TX Shaping Filter_MSB C8~15 */
		odm_set_bb_reg(dm, R_0x1ab0, MASKDWORD, 0x0000ffff);
	} else {
		/* @TX Shaping Filter C0~1 */
		odm_set_bb_reg(dm, R_0x1a20, MASKHWORD, 0x5284);
		/* @TX Shaping Filter C2~5 */
		odm_set_bb_reg(dm, R_0x1a24, MASKDWORD, 0x3e18fec8);
		/* @TX Shaping Filter C6~7 */
		odm_set_bb_reg(dm, R_0x1a28, MASKLWORD, 0x0a88);
		/* @TX Shaping Filter C8~9 */
		odm_set_bb_reg(dm, R_0x1a98, MASKHWORD, 0xacc4);
		/* @TX Shaping Filter C10~11 */
		odm_set_bb_reg(dm, R_0x1a9c, MASKLWORD, 0xc8b2);
		/* @TX Shaping Filter C12~15 */
		odm_set_bb_reg(dm, R_0x1aa0, MASKDWORD, 0x00faf0de);
		/* @TX Shaping Filter_MSB C0~7 */
		odm_set_bb_reg(dm, R_0x1aac, MASKDWORD, 0x00122344);
		/* @TX Shaping Filter_MSB C8~15 */
		odm_set_bb_reg(dm, R_0x1ab0, MASKDWORD, 0x0fffffff);
	}
}

__odm_func__
void
phydm_cck_agc_tab_sel_8822c(struct dm_struct *dm, u8 table)
{
	odm_set_bb_reg(dm, R_0x18ac, 0xf000, table);
	odm_set_bb_reg(dm, R_0x41ac, 0xf000, table);
}

__odm_func__
void
phydm_ofdm_agc_tab_sel_8822c(struct dm_struct *dm, u8 table)
{
	struct phydm_dig_struct *dig_tab = &dm->dm_dig_table;
	u8 lower_bound = dm->ofdm_rxagc_l_bnd[table];

	odm_set_bb_reg(dm, R_0x18ac, 0x1f0, table);
	odm_set_bb_reg(dm, R_0x41ac, 0x1f0, table);
	dig_tab->agc_table_idx = table;

	if (!dm->l_bnd_detect[table])
		lower_bound = L_BND_DEFAULT_8822C;

	/*AGC lower bound, need to be updated with AGC table*/
	odm_set_bb_reg(dm, R_0x828, 0xf8, lower_bound);
}

__odm_func__
void
phydm_sco_trk_fc_setting_8822c(struct dm_struct *dm, u8 central_ch)
{
	if (central_ch == 13 || central_ch == 14) {
		/* @n:41, s:37 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x969);
	} else if (central_ch == 11 || central_ch == 12) {
		/* @n:42, s:37 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x96a);
	} else if (central_ch >= 1 && central_ch <= 10) {
		/* @n:42, s:38 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x9aa);
	} else if (central_ch >= 36 && central_ch <= 51) {
		/* @n:20, s:18 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x494);
	} else if (central_ch >= 52 && central_ch <= 55) {
		/* @n:19, s:18 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x493);
	} else if ((central_ch >= 56) && (central_ch <= 111)) {
		/* @n:19, s:17 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x453);
	} else if ((central_ch >= 112) && (central_ch <= 119)) {
		/* @n:18, s:17 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x452);
	} else if ((central_ch >= 120) && (central_ch <= 172)) {
		/* @n:18, s:16 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x412);
	} else { /* if ((central_ch >= 173) && (central_ch <= 177)) */
		/* n:17, s:16 */
		odm_set_bb_reg(dm, R_0xc30, 0xfff, 0x411);
	}
}

__odm_func__
void
phydm_tx_dfir_setting_8822c(struct dm_struct *dm, u8 central_ch)
{
	if (central_ch <= 14) {
		if (central_ch == 13)
			odm_set_bb_reg(dm, R_0x808, 0x70, 0x3);
		else
			odm_set_bb_reg(dm, R_0x808, 0x70, 0x1);
	} else {
		odm_set_bb_reg(dm, R_0x808, 0x70, 0x3);
	}
}

__odm_func__
void phydm_set_manual_nbi_8822c(struct dm_struct *dm, boolean en_manual_nbi,
				int tone_idx)
{
	if (en_manual_nbi) {
		/*set tone_idx*/
		odm_set_bb_reg(dm, R_0x1944, 0x001ff000, tone_idx);
		odm_set_bb_reg(dm, R_0x4044, 0x001ff000, tone_idx);
		/*enable manual NBI path_en*/
		odm_set_bb_reg(dm, R_0x1940, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x4040, BIT(31), 0x1);
		/*enable manual NBI*/
		odm_set_bb_reg(dm, R_0x818, BIT(11), 0x1);
		/*enable NBI block*/
		odm_set_bb_reg(dm, R_0x1d3c, 0x78000000, 0xf);
	} else {
		/*reset tone_idx*/
		odm_set_bb_reg(dm, R_0x1944, 0x001ff000, 0x0);
		odm_set_bb_reg(dm, R_0x4044, 0x001ff000, 0x0);
		/*disable manual NBI path_en*/
		odm_set_bb_reg(dm, R_0x1940, BIT(31), 0x0);
		odm_set_bb_reg(dm, R_0x4040, BIT(31), 0x0);
		/*disable manual NBI*/
		odm_set_bb_reg(dm, R_0x818, BIT(11), 0x0);
		/*disable NBI block*/
		odm_set_bb_reg(dm, R_0x1d3c, 0x78000000, 0x0);
	}
}

__odm_func__
void phydm_set_auto_nbi_8822c(struct dm_struct *dm, boolean en_auto_nbi)
{
	if (en_auto_nbi) {
		/*enable auto nbi detection*/
		odm_set_bb_reg(dm, R_0x818, BIT(3), 0x1);
		odm_set_bb_reg(dm, R_0x1d3c, 0x78000000, 0xf);
	} else {
		odm_set_bb_reg(dm, R_0x818, BIT(3), 0x0);
		odm_set_bb_reg(dm, R_0x1d3c, 0x78000000, 0x0);
	}
}

__odm_func__
void phydm_csi_mask_enable_8822c(struct dm_struct *dm, boolean enable)
{
	if (enable)
		odm_set_bb_reg(dm, R_0xc0c, BIT(3), 0x1);
	else
		odm_set_bb_reg(dm, R_0xc0c, BIT(3), 0x0);
}

__odm_func__
void phydm_set_csi_mask_8822c(struct dm_struct *dm, u32 tone_idx)
{
	u32 table_addr = tone_idx >> 1;

	/*enable clk*/
	odm_set_bb_reg(dm, R_0x1ee8, 0x3, 0x3);
	/*enable write table*/
	odm_set_bb_reg(dm, R_0x1d94, BIT(31) | BIT(30), 0x1);
	/*set table_addr*/
	odm_set_bb_reg(dm, R_0x1d94, MASKBYTE2, (table_addr & 0xff));

	if (tone_idx & 0x1)
		odm_set_bb_reg(dm, R_0x1d94, 0xf0, 0x8);
	else
		odm_set_bb_reg(dm, R_0x1d94, 0xf, 0x8);

	/*disable clk*/
	odm_set_bb_reg(dm, R_0x1ee8, 0x3, 0x0);
}

__odm_func__
void phydm_clean_all_csi_mask_8822c(struct dm_struct *dm)
{
	u8 i = 0;

	/*enable clk*/
	odm_set_bb_reg(dm, R_0x1ee8, 0x3, 0x3);
	/*enable write table*/
	odm_set_bb_reg(dm, R_0x1d94, BIT(31) | BIT(30), 0x1);

	for (i = 0; i < 128; i++) {
		odm_set_bb_reg(dm, R_0x1d94, MASKBYTE2, i);
		odm_set_bb_reg(dm, R_0x1d94, MASKBYTE0, 0x0);
	}

	/*disable clk*/
	odm_set_bb_reg(dm, R_0x1ee8, 0x3, 0x0);
}

__odm_func__
void phydm_spur_eliminate_8822c(struct dm_struct *dm, u8 central_ch)
{
	phydm_set_auto_nbi_8822c(dm, false);
	phydm_csi_mask_enable_8822c(dm, true);

	if (central_ch == 153 && (*dm->band_width == CHANNEL_WIDTH_20)) {
		phydm_set_manual_nbi_8822c(dm, true, 112); /*5760 MHz*/
		phydm_set_csi_mask_8822c(dm, 112);
	} else if (central_ch == 151 && (*dm->band_width == CHANNEL_WIDTH_40)) {
		phydm_set_manual_nbi_8822c(dm, true, 16); /*5760 MHz*/
		phydm_set_csi_mask_8822c(dm, 16);
	} else if (central_ch == 155 && (*dm->band_width == CHANNEL_WIDTH_80)) {
		phydm_set_manual_nbi_8822c(dm, true, 208); /*5760 MHz*/
		phydm_set_csi_mask_8822c(dm, 208);
	} else {
		phydm_set_manual_nbi_8822c(dm, false, 0);
		phydm_clean_all_csi_mask_8822c(dm);
		phydm_csi_mask_enable_8822c(dm, false);
	}
}

__odm_func__
void phydm_set_dis_dpd_by_rate_8822c(struct dm_struct *dm, u16 bitmask)
{
	/* bit(0) : ofdm 6m*/
	/* bit(1) : ofdm 9m*/
	/* bit(2) : ht mcs0*/
	/* bit(3) : ht mcs1*/
	/* bit(4) : ht mcs8*/
	/* bit(5) : ht mcs9*/
	/* bit(6) : vht 1ss mcs0*/
	/* bit(7) : vht 1ss mcs1*/
	/* bit(8) : vht 2ss mcs0*/
	/* bit(9) : vht 2ss mcs1*/

	odm_set_bb_reg(dm, R_0xa70, 0x3ff, bitmask);
	dm->dis_dpd_rate = bitmask;
}

__odm_func__
boolean
config_phydm_switch_channel_8822c(struct dm_struct *dm, u8 central_ch)
{
	u32 rf_reg18 = 0;
	boolean is_2g_ch = true;
	enum bb_path tx_1sts = BB_PATH_NON;
	enum bb_path tx_2sts = BB_PATH_NON;
	enum bb_path tx = BB_PATH_NON;
	enum bb_path rx = BB_PATH_NON;
	u8 rfe_type = dm->rfe_type;
	struct phydm_iot_center	*iot_table = &dm->iot_table;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API\n");
		return true;
	}

	if ((central_ch > 14 && central_ch < 36) ||
	    (central_ch > 64 && central_ch < 100) ||
	    (central_ch > 144 && central_ch < 149) ||
	    central_ch > 177) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Error CH:%d\n", central_ch);
		return false;
	}

	rf_reg18 = config_phydm_read_rf_reg_8822c(dm, RF_PATH_A, RF_0x18,
						  RFREG_MASK);
	if (rf_reg18 == INVALID_RF_DATA) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Invalid RF_0x18\n");
		return false;
	}

	is_2g_ch = (central_ch <= 14) ? true : false;

	/* [get trx info for cck tx and rfe ctrl] */
	tx_1sts = dm->tx_1ss_status;
	if (tx_1sts == BB_PATH_NON) {
		tx_1sts = (u8)odm_get_bb_reg(dm, R_0x820, 0xf);
		dm->tx_1ss_status = (u8)tx_1sts;
		pr_debug("[%s]tx_1ss is non!, update tx_1sts:%d\n",
			 __func__, tx_1sts);
	}
	tx = dm->tx_ant_status;
	if (tx == BB_PATH_NON) {
		tx_2sts = (u8)odm_get_bb_reg(dm, R_0x820, 0xf0);
		tx = (u8)(tx_1sts | tx_2sts);
		dm->tx_2ss_status = (u8)tx_2sts;
		dm->tx_ant_status = tx;
		pr_debug("[%s]tx_ant_status is non!, update tx_2sts/tx_path:%d/%d\n",
			 __func__, tx_2sts, tx);
	}
	rx = dm->rx_ant_status;
	if (rx == BB_PATH_NON) {
		rx = (u8)odm_get_bb_reg(dm, R_0x824, 0xf0000);
		dm->rx_ant_status = (u8)rx;
		pr_debug("[%s]rx_ant_status is non!, update rx_path:%d\n",
			 __func__, rx);
	}

	/* ==== [Set RF Reg 0x18] ===========================================*/
	rf_reg18 &= ~0x703ff; /*[18:17],[16],[9:8],[7:0]*/
	rf_reg18 |= central_ch; /* Channel*/

	if (!is_2g_ch) { /*5G*/
		rf_reg18 |= (BIT(16) | BIT(8));

		/* 5G Sub-Band, 01: 5400<f<=5720, 10: f>5720*/
		if (central_ch > 144)
			rf_reg18 |= BIT(18);
		else if (central_ch >= 80)
			rf_reg18 |= BIT(17);
	}

	/*reset HSSI*/
	odm_set_bb_reg(dm, R_0x1c90, BIT(8), 0x0);
	/*RxA enhance-Q setting*/
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xdf, BIT(18), is_2g_ch);
	/*write RF-0x18*/
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK, rf_reg18);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREG_MASK, rf_reg18);
	/*reset HSSI*/
	odm_set_bb_reg(dm, R_0x1c90, BIT(8), 0x1);
	/*force update anapar*/
	odm_set_bb_reg(dm, R_0x1830, BIT(29), 0x1);
	odm_set_bb_reg(dm, R_0x4130, BIT(29), 0x1);
	/* ==== [Set BB Reg] =================================================*/
	/* 1. AGC table selection */
	if (central_ch <= 14) {
		if (*dm->band_width == CHANNEL_WIDTH_20) {
			phydm_cck_agc_tab_sel_8822c(dm, CCK_BW20_8822C);
			phydm_ofdm_agc_tab_sel_8822c(dm, OFDM_2G_BW20_8822C);
		} else {
			phydm_cck_agc_tab_sel_8822c(dm, CCK_BW40_8822C);
			phydm_ofdm_agc_tab_sel_8822c(dm, OFDM_2G_BW40_8822C);
		}
	} else if (central_ch >= 36 && central_ch <= 64) {
		phydm_ofdm_agc_tab_sel_8822c(dm, OFDM_5G_LOW_BAND_8822C);
	} else if ((central_ch >= 100) && (central_ch <= 144)) {
		phydm_ofdm_agc_tab_sel_8822c(dm, OFDM_5G_MID_BAND_8822C);
	} else { /*if (central_ch >= 149)*/
		phydm_ofdm_agc_tab_sel_8822c(dm, OFDM_5G_HIGH_BAND_8822C);
	}
	/* 2. Set fc for clock offset tracking */
	phydm_sco_trk_fc_setting_8822c(dm, central_ch);
	/* 3. TX DFIR*/
	phydm_tx_dfir_setting_8822c(dm, central_ch);
	/* 4. Other BB Settings*/
	if (is_2g_ch) {
		phydm_cck_tx_shaping_filter_8822c(dm, central_ch);
		/* Enable CCK Rx IQ */
		phydm_cck_rxiq_8822c(dm, PHYDM_SET);
		/* Disable MAC CCK check */
		odm_set_mac_reg(dm, R_0x454, BIT(7), 0x0);
		/* Disable BB CCK check */
		odm_set_bb_reg(dm, R_0x1a80, BIT(18), 0x0);
		/* CCA Mask, default = 0xf */
		odm_set_bb_reg(dm, R_0x1c80, 0x3F000000, 0xF);
		/*RFE ctrl*/
		if (rfe_type == 21 || rfe_type == 22)
			phydm_rfe_8822c(dm, BB_PATH_NON);
	} else { /* 5G*/
		/* Enable BB CCK check */
		odm_set_bb_reg(dm, R_0x1a80, BIT(18), 0x1);
		/* Enable CCK check */
		odm_set_mac_reg(dm, R_0x454, BIT(7), 0x1);
		/* Disable CCK Rx IQ */
		phydm_cck_rxiq_8822c(dm, PHYDM_REVERT);
		/* CCA Mask */
		odm_set_bb_reg(dm, R_0x1c80, 0x3F000000, 0x22);
		/*RFE ctrl*/
		if (rfe_type == 21 || rfe_type == 22) {
			if (tx == BB_PATH_A && rx == BB_PATH_A)
				phydm_rfe_8822c(dm, BB_PATH_A);
			else if (tx == BB_PATH_B && rx == BB_PATH_B)
				phydm_rfe_8822c(dm, BB_PATH_B);
			else
				phydm_rfe_8822c(dm, BB_PATH_AB);
		}
	}

	if (iot_table->patch_id_011f0500) {
		if (central_ch != 1 && dm->en_dis_dpd)
			phydm_set_dis_dpd_by_rate_8822c(dm, 0x3ff);
		else
			phydm_set_dis_dpd_by_rate_8822c(dm, 0x0);
	}
	/*====================================================================*/
	if (*dm->mp_mode)
		phydm_spur_eliminate_8822c(dm, central_ch);

	phydm_bb_reset_8822c(dm);

	phydm_igi_toggle_8822c(dm);

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "Switch CH:%d success\n", central_ch);
	return true;
}

__odm_func__
boolean
config_phydm_switch_bandwidth_8822c(struct dm_struct *dm, u8 pri_ch,
				    enum channel_width bw)
{
	u32 rf_reg18 = 0;
	u32 rf_reg3f = 0;
	boolean rf_reg_status = true;

	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	if (dm->is_disable_phy_api) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Disable PHY API for debug!!\n");
		return true;
	}

	/*Error handling */
	if (bw >= CHANNEL_WIDTH_MAX || (bw == CHANNEL_WIDTH_40 && pri_ch > 2) ||
	    (bw == CHANNEL_WIDTH_80 && pri_ch > 4)) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Fail to switch bw(bw:%d, pri ch:%d)\n", bw, pri_ch);
		return false;
	}

	rf_reg18 = config_phydm_read_rf_reg_8822c(dm, RF_PATH_A, RF_0x18,
						  RFREG_MASK);
	if (rf_reg18 != INVALID_RF_DATA)
		rf_reg_status = true;
	else
		rf_reg_status = false;

	rf_reg18 &= ~(BIT(13) | BIT(12));

	/*Switch bandwidth */
	switch (bw) {
	case CHANNEL_WIDTH_5:
	case CHANNEL_WIDTH_10:
	case CHANNEL_WIDTH_20:
		if (bw == CHANNEL_WIDTH_5) {
			/*RX DFIR*/
			odm_set_bb_reg(dm, R_0x810, 0x3ff0, 0x2ab);

			/*small BW:[7:6]=0x1 */
			/*TX pri ch:[11:8]=0x0, RX pri ch:[15:12]=0x0 */
			odm_set_bb_reg(dm, R_0x9b0, 0xffc0, 0x1);

			/*DAC clock = 120M clock for BW5 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00000700, 0x4);

			/*ADC clock = 40M clock for BW5 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00700000, 0x4);
		} else if (bw == CHANNEL_WIDTH_10) {
			/*RX DFIR*/
			odm_set_bb_reg(dm, R_0x810, 0x3ff0, 0x2ab);

			/*small BW:[7:6]=0x2 */
			/*TX pri ch:[11:8]=0x0, RX pri ch:[15:12]=0x0 */
			odm_set_bb_reg(dm, R_0x9b0, 0xffc0, 0x2);

			/*DAC clock = 240M clock for BW10 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00000700, 0x6);

			/*ADC clock = 80M clock for BW10 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00700000, 0x5);
		} else if (bw == CHANNEL_WIDTH_20) {
			/*RX DFIR*/
			odm_set_bb_reg(dm, R_0x810, 0x3ff0, 0x19b);

			/*small BW:[7:6]=0x0 */
			/*TX pri ch:[11:8]=0x0, RX pri ch:[15:12]=0x0 */
			odm_set_bb_reg(dm, R_0x9b0, 0xffc0, 0x0);

			/*DAC clock = 480M clock for BW20 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00000700, 0x7);

			/*ADC clock = 160M clock for BW20 */
			odm_set_bb_reg(dm, R_0x9b4, 0x00700000, 0x6);
		}

		/*TX_RF_BW:[1:0]=0x0, RX_RF_BW:[3:2]=0x0 */
		odm_set_bb_reg(dm, R_0x9b0, 0xf, 0x0);

		/*RF bandwidth */
		rf_reg18 |= (BIT(13) | BIT(12));

		/*RF RXBB setting*/
		rf_reg3f = (BIT(4) | BIT(3));

		/*pilot smoothing on */
		odm_set_bb_reg(dm, R_0xcbc, BIT(21), 0x0);

		/*CCK source 4 */
		odm_set_bb_reg(dm, R_0x1abc, BIT(30), 0x0);

		/*dynamic CCK PD th*/
		odm_set_bb_reg(dm, R_0x1ae8, BIT(31), 0x1);
		odm_set_bb_reg(dm, R_0x1aec, 0xf, 0x6);

		/*subtune*/
		odm_set_bb_reg(dm, R_0x88c, 0xf000, 0x1);

		if (*dm->band_type == ODM_BAND_2_4G) {
			phydm_cck_agc_tab_sel_8822c(dm, CCK_BW20_8822C);
			phydm_ofdm_agc_tab_sel_8822c(dm, OFDM_2G_BW20_8822C);
		}
		break;
	case CHANNEL_WIDTH_40:
		/*CCK primary channel */
		if (pri_ch == 1)
			odm_set_bb_reg(dm, R_0x1a00, BIT(4), pri_ch);
		else
			odm_set_bb_reg(dm, R_0x1a00, BIT(4), 0);

		/*TX_RF_BW:[1:0]=0x1, RX_RF_BW:[3:2]=0x1 */
		odm_set_bb_reg(dm, R_0x9b0, 0xf, 0x5);

		/*small BW */
		odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0);

		/*TX pri ch:[11:8], RX pri ch:[15:12] */
		odm_set_bb_reg(dm, R_0x9b0, 0xff00, (pri_ch | (pri_ch << 4)));

		/*RF bandwidth */
		rf_reg18 |= BIT(13);

		/*RF RXBB setting*/
		rf_reg3f = BIT(4);

		/*pilot smoothing off */
		odm_set_bb_reg(dm, R_0xcbc, BIT(21), 0x1);

		/*CCK source 5 */
		odm_set_bb_reg(dm, R_0x1abc, BIT(30), 0x1);

		/*dynamic CCK PD th*/
		odm_set_bb_reg(dm, R_0x1ae8, BIT(31), 0x0);
		odm_set_bb_reg(dm, R_0x1aec, 0xf, 0x8);

		/*subtune*/
		odm_set_bb_reg(dm, R_0x88c, 0xf000, 0x1);

		if (*dm->band_type == ODM_BAND_2_4G) {
			/*CCK*/
			phydm_cck_agc_tab_sel_8822c(dm, CCK_BW40_8822C);
			phydm_ofdm_agc_tab_sel_8822c(dm, OFDM_2G_BW40_8822C);
		}
		break;
	case CHANNEL_WIDTH_80:
		/*TX_RF_BW:[1:0]=0x2, RX_RF_BW:[3:2]=0x2 */
		odm_set_bb_reg(dm, R_0x9b0, 0xf, 0xa);

		/*small BW */
		odm_set_bb_reg(dm, R_0x9b0, 0xc0, 0x0);

		/*TX pri ch:[11:8], RX pri ch:[15:12] */
		odm_set_bb_reg(dm, R_0x9b0, 0xff00, (pri_ch | (pri_ch << 4)));

		/*RF bandwidth */
		rf_reg18 |= BIT(12);

		/*RF RXBB setting*/
		rf_reg3f = BIT(3);

		/*pilot smoothing off */
		odm_set_bb_reg(dm, R_0xcbc, BIT(21), 0x1);

		/*subtune*/
		odm_set_bb_reg(dm, R_0x88c, 0xf000, 0x6);
		break;
	default:
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Fail to switch bw (bw:%d, pri ch:%d)\n", bw, pri_ch);
	}

	/*Write RF register */
	/*reset HSSI*/
	odm_set_bb_reg(dm, R_0x1c90, BIT(8), 0x0);
	/*RF RXBB setting, WLANBB-1081*/
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xee, 0x4, 0x1);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x33, 0x1F, 0x12);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x3f, RFREG_MASK, rf_reg3f);
	odm_set_rf_reg(dm, RF_PATH_A, RF_0xee, 0x4, 0x0);

	odm_set_rf_reg(dm, RF_PATH_B, RF_0xee, 0x4, 0x1);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x33, 0x1F, 0x12);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x3f, RFREG_MASK, rf_reg3f);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0xee, 0x4, 0x0);
	/*write RF-0x18*/
	odm_set_rf_reg(dm, RF_PATH_A, RF_0x18, RFREG_MASK, rf_reg18);
	odm_set_rf_reg(dm, RF_PATH_B, RF_0x18, RFREG_MASK, rf_reg18);
	/*reset HSSI*/
	odm_set_bb_reg(dm, R_0x1c90, BIT(8), 0x1);
	/*force update anapar*/
	odm_set_bb_reg(dm, R_0x1830, BIT(29), 0x1);
	odm_set_bb_reg(dm, R_0x4130, BIT(29), 0x1);

	if (!rf_reg_status) {
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Fail to switch bw (bw:%d, primary ch:%d), because writing RF register is fail\n",
			  bw, pri_ch);
		return false;
	}

	/*fix bw setting*/
	#ifdef CONFIG_BW_INDICATION
	if (!(*dm->mp_mode))
		phydm_bw_fixed_setting(dm);
	#endif

	/*Toggle IGI to let RF enter RX mode */
	phydm_igi_toggle_8822c(dm);

	PHYDM_DBG(dm, ODM_PHY_CONFIG,
		  "Success to switch bw (bw:%d, pri ch:%d)\n", bw, pri_ch);

	phydm_bb_reset_8822c(dm);
	return true;
}

__odm_func__
boolean
config_phydm_switch_channel_bw_8822c(struct dm_struct *dm, u8 central_ch,
				     u8 primary_ch_idx,
				     enum channel_width bandwidth)
{
	/* @Switch channel */
	if (!config_phydm_switch_channel_8822c(dm, central_ch))
		return false;

	/* @Switch bandwidth */
	if (!config_phydm_switch_bandwidth_8822c(dm, primary_ch_idx, bandwidth))
		return false;

	return true;
}

__odm_func__
void phydm_i_only_setting_8822c(struct dm_struct *dm, boolean en_i_only,
				boolean en_before_cca)
{
	if (en_i_only) { /*@ Set path-a*/
		if (en_before_cca) {
			odm_set_bb_reg(dm, R_0x1800, 0xfff00, 0x833);
			odm_set_bb_reg(dm, R_0x1c68, 0xc000, 0x2);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70038001);
		} else {
			if (!(*dm->band_width == CHANNEL_WIDTH_40))
				return;

			dm->bp_0x9b0 = odm_get_bb_reg(dm, R_0x9b0, MASKDWORD);
			odm_set_bb_reg(dm, R_0x1800, 0xfff00, 0x888);
			odm_set_bb_reg(dm, R_0x898, BIT(30), 0x1);
			odm_set_bb_reg(dm, R_0x1c68, 0xc000, 0x1);
			odm_set_bb_reg(dm, R_0x9b0, MASKDWORD, 0x2200);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70038001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70038001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70538001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70738001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70838001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70938001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70a38001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70b38001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70c38001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70d38001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70e38001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70f38001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70f38001);
		}
	} else {
		if (en_before_cca) {
			odm_set_bb_reg(dm, R_0x1800, 0xfff00, 0x333);
			odm_set_bb_reg(dm, R_0x1c68, 0xc000, 0x0);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8001);
		} else {
			if (!(*dm->band_width == CHANNEL_WIDTH_40))
				return;

			odm_set_bb_reg(dm, R_0x1800, 0xfff00, 0x333);
			odm_set_bb_reg(dm, R_0x898, BIT(30), 0x0);
			odm_set_bb_reg(dm, R_0x1c68, 0xc000, 0x0);
			odm_set_bb_reg(dm, R_0x9b0, MASKDWORD, dm->bp_0x9b0);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x700b8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x705b8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x707b8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x708b8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x709b8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70ab8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70bb8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70cb8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70db8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8001);
			odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70fb8001);
		}
	}
}

__odm_func__
void phydm_1rcca_setting_8822c(struct dm_struct *dm, boolean en_1rcca)
{
	if (en_1rcca) { /*@ Set path-a*/
		odm_set_bb_reg(dm, R_0x83c, 0x4, 0x1);
		odm_set_bb_reg(dm, R_0x824, 0x0f000000, 0x1);
		odm_set_bb_reg(dm, R_0x4100, 0xf0000, 0x1);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70008001);
		phydm_config_cck_rx_path_8822c(dm, BB_PATH_A);
	} else {
		odm_set_bb_reg(dm, R_0x83c, 0x4, 0x0);
		odm_set_bb_reg(dm, R_0x824, 0x0f000000, 0x3);
		odm_set_bb_reg(dm, R_0x4100, 0xf0000, 0x3);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x700b8001);
		phydm_config_cck_rx_path_8822c(dm, BB_PATH_AB);
	}
	phydm_bb_reset_8822c(dm);
}

__odm_func__
void phydm_invld_pkt_setting_8822c(struct dm_struct *dm, boolean en_invld_pkt)
{
	if (en_invld_pkt) {
		odm_set_bb_reg(dm, R_0x1c64, BIT(30), 0x1);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70e0c001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70e0c001);
	} else {
		odm_set_bb_reg(dm, R_0x1c64, BIT(30), 0x0);
		odm_set_bb_reg(dm, R_0x1830, MASKDWORD, 0x70eb8001);
		odm_set_bb_reg(dm, R_0x4130, MASKDWORD, 0x70eb8001);
	}
}

__odm_func__
void phydm_cck_gi_bound_8822c(struct dm_struct *dm)
{
	struct phydm_physts *physts_table = &dm->dm_physts_table;
	u8 cck_gi_u_bnd_msb = 0;
	u8 cck_gi_u_bnd_lsb = 0;
	u8 cck_gi_l_bnd_msb = 0;
	u8 cck_gi_l_bnd_lsb = 0;

	cck_gi_u_bnd_msb = (u8)odm_get_bb_reg(dm, R_0x1a98, 0xc000);
	cck_gi_u_bnd_lsb = (u8)odm_get_bb_reg(dm, R_0x1aa8, 0xf0000);
	cck_gi_l_bnd_msb = (u8)odm_get_bb_reg(dm, R_0x1a98, 0xc0);
	cck_gi_l_bnd_lsb = (u8)odm_get_bb_reg(dm, R_0x1a70, 0x0f000000);

	physts_table->cck_gi_u_bnd = (u8)((cck_gi_u_bnd_msb << 4) |
				     (cck_gi_u_bnd_lsb));
	physts_table->cck_gi_l_bnd = (u8)((cck_gi_l_bnd_msb << 4) |
				     (cck_gi_l_bnd_lsb));
}

__odm_func__
void phydm_ch_smooth_setting_8822c(struct dm_struct *dm, boolean en_ch_smooth)
{
	if (en_ch_smooth)
		/* @enable force channel smoothing*/
		odm_set_bb_reg(dm, R_0xc54, BIT(7), 0x1);
	else
		odm_set_bb_reg(dm, R_0xc54, BIT(7), 0x0);
}

__odm_func__
u16 phydm_get_dis_dpd_by_rate_8822c(struct dm_struct *dm)
{
	u16 dis_dpd_rate = 0;

	dis_dpd_rate = dm->dis_dpd_rate;

	return dis_dpd_rate;
}

__odm_func__
void phydm_cck_pd_init_8822c(struct dm_struct *dm)
{
	struct phydm_iot_center	*iot_table = &dm->iot_table;

	if (*dm->mp_mode && iot_table->patch_id_021f0800)
		/*CS ratio:BW20/1R*/
		odm_set_bb_reg(dm, R_0x1ad0, 0x1f, 0x12);
}

__odm_func__
boolean
config_phydm_parameter_init_8822c(struct dm_struct *dm,
				  enum odm_parameter_init type)
{
	PHYDM_DBG(dm, ODM_PHY_CONFIG, "%s ======>\n", __func__);

	phydm_cck_gi_bound_8822c(dm);
	phydm_cck_pd_init_8822c(dm);

	if (*dm->mp_mode)
		phydm_ch_smooth_setting_8822c(dm, true);

	/* Disable low rate DPD*/
	if (dm->en_dis_dpd)
		phydm_set_dis_dpd_by_rate_8822c(dm, 0x3ff);
	else
		phydm_set_dis_dpd_by_rate_8822c(dm, 0x0);

	/* @Do not use PHYDM API to read/write because FW can not access */
	/* @Turn on 3-wire*/
	odm_set_bb_reg(dm, R_0x180c, 0x3, 0x3);
	odm_set_bb_reg(dm, R_0x180c, BIT(28), 0x1);
	odm_set_bb_reg(dm, R_0x410c, 0x3, 0x3);
	odm_set_bb_reg(dm, R_0x410c, BIT(28), 0x1);

	if (type == ODM_PRE_SETTING) {
		odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x0);
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Pre setting: disable OFDM and CCK block\n");
	} else if (type == ODM_POST_SETTING) {
		odm_set_bb_reg(dm, R_0x1c3c, (BIT(0) | BIT(1)), 0x3);
		PHYDM_DBG(dm, ODM_PHY_CONFIG,
			  "Post setting: enable OFDM and CCK block\n");
	} else {
		PHYDM_DBG(dm, ODM_PHY_CONFIG, "Wrong type!!\n");
		return false;
	}

	phydm_bb_reset_8822c(dm);
	#ifdef CONFIG_TXAGC_DEBUG_8822C
	/*phydm_txagc_tab_buff_init_8822c(dm);*/
	#endif

	return true;
}

__odm_func__
boolean
phydm_chk_bb_state_idle_8822c(struct dm_struct *dm)
{
	u32 dbgport = 0;

	/* Do not check GNT_WL for LPS */
	odm_set_bb_reg(dm, R_0x1c3c, 0x00f00000, 0x0);
	dbgport = odm_get_bb_reg(dm, R_0x2db4, MASKDWORD);
	if ((dbgport & 0x1ffeff3f) == 0 &&
	    (dbgport & 0xc0000000) == 0xc0000000)
		return true;
	else
		return false;
}

#if CONFIG_POWERSAVING
__odm_func_aon__
boolean
phydm_8822c_lps(struct dm_struct *dm, boolean enable_lps)
{
	u16 poll_cnt = 0;
	u32 bbtemp = 0;

	if (enable_lps == _TRUE) {
		/* backup RF reg0x0 */
		SysMib.Wlan.PS.PSParm.RxGainPathA = config_phydm_read_rf_reg_8822c(dm, RF_PATH_A, RF_0x0, RFREG_MASK);
		SysMib.Wlan.PS.PSParm.RxGainPathB = config_phydm_read_rf_reg_8822c(dm, RF_PATH_B, RF_0x0, RFREG_MASK);

		/* turn off TRx HSSI*/
		odm_set_bb_reg(dm, R_0x180c, 0x3, 0x0);
		odm_set_bb_reg(dm, R_0x410c, 0x3, 0x0);

		/* Set RF enter shutdown mode*/
		bbtemp = odm_get_bb_reg(dm, R_0x824, MASKDWORD);
		odm_set_bb_reg(dm, R_0x824, 0xf0000, 0x3);
		config_phydm_write_rf_reg_8822c(dm, RF_PATH_A, RF_0x0, RFREG_MASK, 0x0);
		config_phydm_write_rf_reg_8822c(dm, RF_PATH_B, RF_0x0, RFREG_MASK, 0x0);
		odm_set_bb_reg(dm, R_0x824, MASKDWORD, bbtemp);

		/*bb reset w/o 3-wires */
		phydm_bb_reset_no_3wires_8822c(dm);

		while (1) {
			if (phydm_chk_bb_state_idle_8822c(dm))
				break;

			if (poll_cnt > WAIT_TXSM_STABLE_CNT) {
				WriteMACRegDWord(REG_DBG_DW_FW_ERR, ReadMACRegDWord(REG_DBG_DW_FW_ERR) | FES_BBSTATE_IDLE);
			/* SysMib.Wlan.DbgPort.DbgInfoParm.u4ErrFlag[0] |= FES_BBSTATE_IDLE; */
				return _FALSE;
			}

			DelayUS(WAIT_TXSM_STABLE_ONCE_TIME);
			poll_cnt++;
		}

		/* disable CCK and OFDM module */
		WriteMACRegByte(REG_SYS_FUNC_EN, ReadMACRegByte(REG_SYS_FUNC_EN)
				& ~BIT_FEN_BBRSTB);

		if (poll_cnt < WAIT_TXSM_STABLE_CNT) {
			/* Gated BBclk*/
			odm_set_bb_reg(dm, R_0x1c24, BIT(0), 0x1);
		}

		return _TRUE;
	} else {
		/* release BB clk*/
		odm_set_bb_reg(dm, R_0x1c24, BIT(0), 0x0);

		PwrGatedRestoreBB();

		/* Enable CCK and OFDM module, */
		/* should be a delay large than 200ns before RF access */
		WriteMACRegByte(REG_SYS_FUNC_EN, ReadMACRegByte(REG_SYS_FUNC_EN)
				| BIT_FEN_BBRSTB);
		DelayUS(1);

		/* Set RF enter active mode */
		bbtemp = odm_get_bb_reg(dm, R_0x824, MASKDWORD);
		odm_set_bb_reg(dm, R_0x824, 0xf0000, 0x3);
		config_phydm_write_rf_reg_8822c(dm, RF_PATH_A, RF_0x0, RFREG_MASK, SysMib.Wlan.PS.PSParm.RxGainPathA);
		config_phydm_write_rf_reg_8822c(dm, RF_PATH_B, RF_0x0, RFREG_MASK, SysMib.Wlan.PS.PSParm.RxGainPathB);
		odm_set_bb_reg(dm, R_0x824, MASKDWORD, bbtemp);

		/*bb reset w/o 3-wires */
		phydm_bb_reset_no_3wires_8822c(dm);

		/* turn on TRx HSSI*/
		odm_set_bb_reg(dm, R_0x180c, 0x3, 0x3);
		odm_set_bb_reg(dm, R_0x410c, 0x3, 0x3);

		return _TRUE;
	}
}
#endif /* #if CONFIG_POWERSAVING */

/* ======================================================================== */
#endif /* PHYDM_FW_API_ENABLE_8822C */
#endif /* RTL8822C_SUPPORT */

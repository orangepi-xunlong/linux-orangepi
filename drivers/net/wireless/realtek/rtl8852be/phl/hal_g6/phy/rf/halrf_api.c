/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation.
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

 
#include "halrf_precomp.h"

u8 halrf_kpath(struct rf_info *rf, enum phl_phy_idx phy_idx)
{
	struct rtw_hal_com_t *hal_com = rf->hal_com;

	RF_DBG(rf, DBG_RF_RFK, "[RFK]dbcc_en: %x,  PHY%d\n", rf->hal_com->dbcc_en, phy_idx);

#ifdef RF_8851B_SUPPORT
	if (hal_com->chip_id == CHIP_WIFI6_8851B)
		return RF_A;
	else
#endif
	{
		if (!rf->hal_com->dbcc_en) {
			return RF_AB;
		} else {
			if (phy_idx == HW_PHY_0)
				return RF_A;
			else
				return RF_B;
		}
	}
}

u8 halrf_max_path_num(struct rf_info *rf)
{
	struct rtw_hal_com_t *hal_com = rf->hal_com;

	switch (hal_com->chip_id) {
#ifdef RF_8851B_SUPPORT
		case CHIP_WIFI6_8851B:
			return 1;
		break;
#endif
		default:
			return 2;
		break;
	}
}

u32 phlrf_psd_log2base(struct rf_info *rf, u32 val)
{
	u32 j;
	u32 tmp, tmp2, val_integerd_b = 0, tindex, shiftcount = 0;
	u32 result, val_fractiond_b = 0;
	u32 table_fraction[21] = {
		0, 432, 332, 274, 232, 200, 174, 151, 132, 115,
		100, 86, 74, 62, 51, 42, 32, 23, 15, 7, 0};

	if (val == 0)
		return 0;

	tmp = val;

	while (1) {
		if (tmp == 1)
			break;

		tmp = (tmp >> 1);
		shiftcount++;
	}

	val_integerd_b = shiftcount + 1;

	tmp2 = 1;
	for (j = 1; j <= val_integerd_b; j++)
		tmp2 = tmp2 * 2;

	tmp = (val * 100) / tmp2;
	tindex = tmp / 5;

	if (tindex > 20)
		tindex = 20;

	val_fractiond_b = table_fraction[tindex];

	result = val_integerd_b * 100 - val_fractiond_b;

	return result;
}

void phlrf_rf_lna_setting(struct rf_info *rf, enum phlrf_lna_set type)
{
	struct rtw_hal_com_t *hal_i = rf->hal_com;

	switch (hal_i->chip_id) {
#ifdef RF_8852A_SUPPORT
			case CHIP_WIFI6_8852A:
				break;
#endif
			default:
				break;
		}

}

void halrf_bkp(struct rf_info *rf, u32 *bp_reg, u32 *bp, u32 reg_num)
{
	u32 i;

	for (i = 0; i < reg_num; i++)
		bp[i] = halrf_rreg(rf, bp_reg[i], MASKDWORD);
}

void halrf_bkprf(struct rf_info *rf, u32 *bp_reg, u32 bp[][4], u32 reg_num, u32 path_num)
{
	u32 i, j;

	for (i = 0; i < reg_num; i++) {
		for (j = 0; j < path_num; j++)
			bp[i][j] = halrf_rrf(rf, j, bp_reg[i], MASKRF);
	}
}

void halrf_reload_bkp(struct rf_info *rf, u32 *bp_reg, u32 *bp, u32 reg_num)
{
	u32 i;

	for (i = 0; i < reg_num; i++)
		halrf_wreg(rf, bp_reg[i], MASKDWORD, bp[i]);
}

void halrf_reload_bkprf(struct rf_info *rf,
		       u32 *bp_reg,
		       u32 bp[][4],
		       u32 reg_num,
		       u8 path_num)
{
	u32 i, path;

	for (i = 0; i < reg_num; i++) {
		for (path = 0; path < path_num; path++)
			halrf_wrf(rf, (enum rf_path)path, bp_reg[i],
				       MASKRF, bp[i][path]);
	}
}

void halrf_wait_rx_mode(struct rf_info *rf, u8 kpath)
{
	u8 path, rf_mode = 0;
	u16 count = 0;

	for (path = 0; path < halrf_max_path_num(rf); path++) {
		if (kpath & BIT(path)) {
			rf_mode = (u8)halrf_rrf(rf, path, 0x00, MASKRFMODE);

			while (rf_mode == 2 && count < 2500) {
				rf_mode = (u8)halrf_rrf(rf, path, 0x00, MASKRFMODE);
				halrf_delay_us(rf, 2);
				count++;
			}
			RF_DBG(rf, DBG_RF_RFK,
			       "[RFK] Wait S%d to Rx mode!! (count = %d)\n", path, count);
		}
	}
}

void halrf_tmac_tx_pause(struct rf_info *rf, enum phl_phy_idx band_idx, bool is_pause)
{
	halrf_tx_pause(rf, band_idx, is_pause, PAUSE_RSON_RFK);

	RF_DBG(rf, DBG_RF_RFK,"[RFK] Band%d Tx Pause %s!!\n",
	       band_idx, is_pause ? "on" : "off");

	if (is_pause)
		halrf_wait_rx_mode(rf, halrf_kpath(rf, band_idx));
}

void halrf_trigger_thermal(struct rf_info *rf)
{
	struct rtw_hal_com_t *hal_i = rf->hal_com;

	switch (hal_i->chip_id) {
#ifdef RF_8852A_SUPPORT
		case CHIP_WIFI6_8852A:
			halrf_trigger_thermal_8852a(rf, RF_PATH_A);
			halrf_trigger_thermal_8852a(rf, RF_PATH_B);
			break;
#endif
		default:
			break;
	}
}

u8 halrf_only_get_thermal(struct rf_info *rf, enum rf_path path)
{
	struct rtw_hal_com_t *hal_i = rf->hal_com;

	switch (hal_i->chip_id) {
#ifdef RF_8852A_SUPPORT
		case CHIP_WIFI6_8852A:
			return halrf_only_get_thermal_8852a(rf, path);
			break;
#endif
		default:
			break;
	}

	return 0;
}

void halrf_btc_rfk_ntfy(struct rf_info *rf, u8 phy_map, enum halrf_rfk_type type,
			enum halrf_rfk_process process)
{
	u32 cnt = 0;
	u8 band;
	/*idx : use BIT mask for RF path PATH A: 1, PATH B:2, PATH AB:3*/

	band = rf->hal_com->band[(phy_map & 0x30) >> 5].cur_chandef.band;

	phy_map = (band << 6) | phy_map;

	RF_DBG(rf, DBG_RF_RFK, "[RFK] RFK notify (%s / PHY%d / K_type = %d / path_idx = %d / process = %s)\n",
		band == 0 ? "2G" : (band == 1 ? "5G" : "6G"), (phy_map & 0x30) >> 5, type,
		phy_map & 0xf, process == 0 ? "RFK_STOP" : (process == 1 ? "RFK_START" :
		(process == 2 ? "ONE-SHOT_START" : "ONE-SHOT_STOP")));
#if 1
	if (process == RFK_START && rf->is_bt_iqk_timeout == false) {
		while (halrf_btc_ntfy(rf, phy_map, type, process) == 0 && cnt < 2500) {
			halrf_delay_us(rf, 40);
			cnt++;
		}
		if (cnt == 2500) {
			RF_DBG(rf, DBG_RF_RFK, "[RFK] Wait BT IQK timeout!!!!\n");
			rf->is_bt_iqk_timeout = true;
		}
	} else
		halrf_btc_ntfy(rf, phy_map, type, process);
#endif
}

void halrf_fcs_init(struct rf_info *rf)
{
	struct rtw_hal_com_t *hal_com = rf->hal_com;

#ifdef RF_8852A_SUPPORT
	if (hal_com->chip_id == CHIP_WIFI6_8852A)
		halrf_fcs_init_8852a(rf);
#endif
}

void halrf_fast_chl_sw_backup(struct rf_info *rf, u8 chl_index, u8 t_index)
{
	u32 t[2];

	t[0] = chl_index;
	t[1] = t_index;

	halrf_fill_h2c_cmd(rf, 8, FWCMD_H2C_BACKUP_RFK, 0xa, H2CB_TYPE_DATA, t);
	RF_DBG(rf, DBG_RF_RFK, "FWCMD_H2C_BACKUP_RFK chl=%d t=%d\n", chl_index, t_index);
}

void halrf_fast_chl_sw_reload(struct rf_info *rf, u8 chl_index, u8 t_index)
{
	u32 t[2];

	t[0] = chl_index;
	t[1] = t_index;

	halrf_fill_h2c_cmd(rf, 8, FWCMD_H2C_RELOAD_RFK, 0xa, H2CB_TYPE_DATA, t);
	RF_DBG(rf, DBG_RF_RFK, "FWCMD_H2C_RELOAD_RFK chl=%d t=%d\n", chl_index, t_index);
}

void  halrf_quick_check_rf(void *rf_void)
{
	struct rf_info *rf = (struct rf_info *)rf_void;
	struct rtw_hal_com_t *hal_com = rf->hal_com;

#ifdef RF_8852A_SUPPORT
	if (hal_com->chip_id == CHIP_WIFI6_8852A)
		halrf_quick_check_rfrx_8852a(rf);
#endif
#ifdef RF_8852B_SUPPORT
	if (hal_com->chip_id == CHIP_WIFI6_8852B)
		halrf_quick_checkrf_8852b(rf);
#endif
#ifdef RF_8852BT_SUPPORT
	if (hal_com->chip_id == CHIP_WIFI6_8852BT)
		halrf_quick_checkrf_8852bt(rf);
#endif
#ifdef RF_8852C_SUPPORT
	if (hal_com->chip_id == CHIP_WIFI6_8852C)
		halrf_quick_checkrf_8852c(rf);
#endif
#ifdef RF_8852D_SUPPORT
	if (hal_com->chip_id == CHIP_WIFI6_8852D)
		halrf_quick_checkrf_8852d(rf);
#endif
#ifdef RF_8851B_SUPPORT
	if (hal_com->chip_id == CHIP_WIFI6_8851B)
		halrf_quick_checkrf_8851b(rf);
#endif
}

bool halrf_check_if_dowatchdog(void *rf_void)
{
	struct rf_info *rf = (struct rf_info *)rf_void;

	return rf->is_watchdog_stop;
}

void  halrf_watchdog_stop(struct rf_info *rf, bool is_stop) {
	if(is_stop)
		rf->is_watchdog_stop = true;
	else
		rf->is_watchdog_stop = false;
	RF_DBG(rf, DBG_RF_RFK, "is_watchdog_stop=%d\n", rf->is_watchdog_stop);
}

void halrf_wifi_event_notify(void *rf_void,
			enum phl_msg_evt_id event, enum phl_phy_idx phy_idx)
{
	struct rf_info *rf = (struct rf_info *)rf_void;
	struct halrf_pwr_info *pwr = &rf->pwr_info;

	switch (event) {
		case MSG_EVT_SCAN_START:
			halrf_tssi_default_txagc(rf, phy_idx, true);
			halrf_tssi_set_avg(rf, phy_idx, true);
			halrf_dpk_track_onoff(rf, false);
			halrf_rx_dck_track_onoff(rf, false);
			halrf_set_scan_power_table_to_fw(rf);
		break;
		case MSG_EVT_SCAN_END:
			halrf_tssi_default_txagc(rf, phy_idx, false);
			halrf_tssi_set_avg(rf, phy_idx, false);
			halrf_dpk_track_onoff(rf, true);
			halrf_rx_dck_track_onoff(rf, true);
		break;
		case MSG_EVT_DBG_RX_DUMP:
			halrf_quick_check_rf(rf);
		break;
		case MSG_EVT_DBG_TX_DUMP:
			halrf_quick_check_rf(rf);
		break;
		case MSG_EVT_SWCH_START:
			halrf_tssi_backup_txagc(rf, phy_idx, true);
		break;
		case MSG_EVT_MCC_START:
			halrf_watchdog_stop(rf, true);
		break;
		case MSG_EVT_MCC_STOP:
			halrf_watchdog_stop(rf, false);
		break;
#if 0
		case MSG_EVT_HAL_INIT_OK:
			halrf_set_power(rf, HW_PHY_0, PWR_BY_RATE);
			halrf_tssi_trigger(rf, HW_PHY_0, false);
		break;
		case MSG_EVT_SET_PWR_LIMIT_LOW:
		case MSG_EVT_SET_PWR_LIMIT_STD:
		case MSG_EVT_SET_PWR_LIMIT_VLOW:
			halrf_config_power_limit_6g(rf, HW_PHY_0);
			halrf_config_power_limit_ru_6g(rf, HW_PHY_0);
			if (rf->hal_com->dbcc_en)
				halrf_set_power(rf, HW_PHY_1, (PWR_LIMIT & PWR_LIMIT_RU));
			halrf_set_power(rf, HW_PHY_0, (PWR_LIMIT & PWR_LIMIT_RU));
		break;
#endif
		default:
		break;
	}
}

void halrf_write_fwofld_start(struct rf_info *rf)
{
#if defined(HALRF_CONFIG_FW_DBCC_OFLD_SUPPORT) || defined(HALRF_CONFIG_FW_IO_OFLD_SUPPORT)
	bool fw_ofld = rf->phl_com->dev_cap.io_ofld;// rf->phl_com->dev_cap.fw_cap.offload_cap & BIT(0);

#ifdef HALRF_CONFIG_FW_DBCC_OFLD_SUPPORT
	fw_ofld = 1;
#endif

	rf->fw_ofld_enable = true;

	RF_DBG(rf, DBG_RF_FW, "======> %s   fw_ofld=%d   rf->fw_ofld_enable=%d\n",
		__func__, fw_ofld, rf->fw_ofld_enable);
#endif
}

void halrf_write_fwofld_trigger(struct rf_info *rf)
{
#if defined(HALRF_CONFIG_FW_DBCC_OFLD_SUPPORT) || defined(HALRF_CONFIG_FW_IO_OFLD_SUPPORT)
	struct rtw_mac_cmd cmd = {0};
	bool fw_ofld = rf->phl_com->dev_cap.io_ofld; //rf->phl_com->dev_cap.fw_cap.offload_cap & BIT(0);
	u32 rtn;
	u32 i;

#ifdef HALRF_CONFIG_FW_DBCC_OFLD_SUPPORT
	fw_ofld = 1;
#endif

	if (rf->fw_ofld_enable == false)
		return;

	RF_DBG(rf, DBG_RF_FW,
		"[FW]write count in h2c=%d\n",rf->fw_w_count - rf->pre_fw_w_count);

	if ((rf->fw_w_count - rf->pre_fw_w_count) == 0) {
		RF_DBG(rf, DBG_RF_FW,
		"[FW]SKIP h2c trigger, os delay %d us\n",rf->fw_delay_us_count);

		for (i = 0; i < rf->fw_delay_us_count; i++)
			halrf_os_delay_us(rf, 1);	
		
		rf->fw_delay_us_count = 0;
		return;
	}


	cmd.type = RTW_MAC_DELAY_OFLD;
	cmd.lc = 1;
	cmd.value = 1;

	if (fw_ofld) {
		rtn = halrf_mac_add_cmd_ofld(rf, &cmd);
		if (rtn) {
			RF_WARNING("======>%s return fail error code = %d !!!\n",
				__func__, rtn);
		}
	}
	rf->fw_ofld_start = false;
	rf->sw_trigger_count++;
	rf->pre_fw_w_count = rf->fw_w_count;

	rf->fw_delay_us_count = 0;
#endif
}

void halrf_write_fwofld_end(struct rf_info *rf)
{
#if defined(HALRF_CONFIG_FW_DBCC_OFLD_SUPPORT) || defined(HALRF_CONFIG_FW_IO_OFLD_SUPPORT)
	bool fw_ofld = rf->phl_com->dev_cap.io_ofld; //rf->phl_com->dev_cap.fw_cap.offload_cap & BIT(0);

#ifdef HALRF_CONFIG_FW_DBCC_OFLD_SUPPORT
	fw_ofld = 1;
#endif

	if (fw_ofld) {
		if (rf->fw_ofld_start == true)
			halrf_write_fwofld_trigger(rf);
		rf->fw_ofld_enable = false;
		
	}
#endif
}

void halrf_ctrl_bw_ch(void *rf_void, enum phl_phy_idx phy, u8 central_ch,
				enum band_type band, enum channel_width bw)
{
	struct rf_info *rf = (struct rf_info *)rf_void;
	u32 start_time, finish_time;
	bool lock = false;

	start_time = _os_get_cur_time_us();

	if (phl_is_mp_mode(rf->phl_com)) {
		halrf_mutex_lock(rf, &rf->rf_lock);
		lock = true;
	}
	halrf_set_gpio_by_ch(rf, phy, band);
	halrf_ctl_ch(rf, phy, central_ch, band);
	halrf_ctl_bw(rf, phy, bw);
	halrf_ctl_band_ch_bw(rf, phy, band, central_ch, bw);
	halrf_rxbb_bw(rf, phy, bw);
	halrf_rpt_rt_rfk_info(rf, phy, 1);

	if (lock)
		halrf_mutex_unlock(rf, &rf->rf_lock);

	finish_time = _os_get_cur_time_us();
	rf->set_ch_bw_time = HALRF_ABS(finish_time, start_time) / 1000;	
}

u32 halrf_test_event_trigger(void *rf_void,
		enum phl_phy_idx phy, enum halrf_event_idx idx, enum halrf_event_func func) {

	struct rf_info *rf = (struct rf_info *)rf_void;

	switch (idx) {
		case RF_EVENT_PWR_TRK:
			if (func == RF_EVENT_OFF)
				halrf_tssi_disable(rf, phy);
			else if (func == RF_EVENT_ON)
				halrf_tssi_enable(rf, phy);
			else if (func == RF_EVENT_TRIGGER)
				halrf_tssi_trigger(rf, phy, true);
		break;

		case RF_EVENT_IQK:
			if (func == RF_EVENT_OFF)
				halrf_iqk_onoff(rf, true);
			else if (func == RF_EVENT_ON)
				halrf_iqk_onoff(rf, false);
			else if (func == RF_EVENT_TRIGGER) {
				halrf_nbiqk_enable(rf, false); 		
				halrf_iqk_trigger(rf, phy, false);
			}
		break;

		case RF_EVENT_DPK:
			if (func == RF_EVENT_OFF)
				halrf_dpk_onoff(rf, false);
			else if (func == RF_EVENT_ON)
				halrf_dpk_onoff(rf, true);
			else if (func == RF_EVENT_TRIGGER)
				halrf_dpk_trigger(rf, phy, false);
		break;

		case RF_EVENT_TXGAPK:
			if (func == RF_EVENT_OFF)
				halrf_gapk_disable(rf, phy);
			else if (func == RF_EVENT_ON)
				halrf_gapk_enable(rf, phy);
			else if (func == RF_EVENT_TRIGGER)
				halrf_gapk_trigger(rf, phy, true);
		break;

		case RF_EVENT_DACK:
			if (func == RF_EVENT_OFF)
				halrf_dack_onoff(rf, false);
			else if (func == RF_EVENT_ON)
				halrf_dack_onoff(rf, true);
			else if (func == RF_EVENT_TRIGGER)
				halrf_dack_trigger(rf, true);
		break;

		default:
		break;
	}
	return 0;
}

void halrf_mcc_info_init(void *rf_void, enum phl_phy_idx phy)
{
	struct rf_info *rf = (struct rf_info *)rf_void;
	struct halrf_mcc_info *mcc_info = &rf->mcc_info;
	u8 idx;

	if(!mcc_info->is_init) {
		RF_DBG(rf, DBG_RF_RFK, "[MCC info]======> %s \n", __func__);

		mcc_info->is_init = true;
	
		for (idx = 0; idx < 2; idx++) { //channel
			mcc_info->ch[idx] = 0;
			mcc_info->band[idx] = 0;
		}
		mcc_info->table_idx = 0;		
	}
}

void halrf_mcc_get_ch_info(void *rf_void, enum phl_phy_idx phy) 
{
	struct rf_info *rf = (struct rf_info *)rf_void;
	struct halrf_mcc_info *mcc_info = &rf->mcc_info;
	u8 idx;
	u8 get_empty_table = false;
	struct rtw_hal_com_t *hal_i = rf->hal_com;

	halrf_mcc_info_init(rf, phy);
#if 0	
	//get channel info
	for  (idx = 0;  idx < 2; idx++) {
		if (mcc_info->ch[idx] == 0) {
			get_empty_table = true;
			break;
		}
	}

	if (false == get_empty_table) {
		idx = mcc_info->table_idx + 1;
		if (idx > 1) {
			idx = 0;
		}			
	}	

	mcc_info->table_idx = idx;
#endif
	idx = mcc_info->table_idx;
	mcc_info->ch[idx] = rf->hal_com->band[phy].cur_chandef.center_ch;
#ifdef RF_8852C_SUPPORT
	if (hal_i->chip_id == CHIP_WIFI6_8852C)
		mcc_info->band[idx] = rf->hal_com->band[phy].cur_chandef.band;
#endif	
}

void halrf_chlk_backup_dbcc(struct rf_info *rf, enum phl_phy_idx phy) 
{
	struct halrf_dbcc_info *dbcc_info = &rf->dbcc_info;
	u8 kpath, idx;

	RF_DBG(rf, DBG_RF_RFK, "[DBCC]======> %s \n", __func__);
	kpath = halrf_kpath(rf, phy);
	idx = dbcc_info->table_idx;

	if (kpath & BIT(0)) {
		rf->kip_table[idx][0] = halrf_rreg(rf, 0x8104, MASKDWORD);
		rf->kip_table[idx][1] = halrf_rreg(rf, 0x8154, MASKDWORD);
		RF_DBG(rf, DBG_RF_RFK, "[DBCC]S0 backup idx=%d\n", idx);
	}
	if (kpath & BIT(1)) {
		rf->kip_table[idx][2] = halrf_rreg(rf, 0x8204, MASKDWORD);
		rf->kip_table[idx][3] = halrf_rreg(rf, 0x8254, MASKDWORD);
		RF_DBG(rf, DBG_RF_RFK, "[DBCC]S1 backup idx=%d\n", idx);
	}
	RF_DBG(rf, DBG_RF_RFK, "[DBCC]kip_table00=0x%x,kip_table01=0x%x,kip_table02=0x%x,kip_table03=0x%x\n",
		rf->kip_table[0][0],rf->kip_table[0][1],
		rf->kip_table[0][2],rf->kip_table[0][3]);
	RF_DBG(rf, DBG_RF_RFK, "[DBCC]kip_table10=0x%x,kip_table11=0x%x,kip_table12=0x%x,kip_table13=0x%x\n",
		rf->kip_table[1][0],rf->kip_table[1][1],
		rf->kip_table[1][2],rf->kip_table[1][3]);
}

void halrf_chlk_reload_dbcc(struct rf_info *rf, enum phl_phy_idx phy, u8 idx) 
{
	struct halrf_dbcc_info *dbcc_info = &rf->dbcc_info;
	u8 kpath;

	RF_DBG(rf, DBG_RF_RFK, "[DBCC]======> %s \n", __func__);
	kpath = halrf_kpath(rf, phy);

	if (kpath & BIT(0)) {
		halrf_wreg(rf, 0x8104, MASKDWORD, rf->kip_table[idx][0]);
		halrf_wreg(rf, 0x8154, MASKDWORD, rf->kip_table[idx][1]);
		RF_DBG(rf, DBG_RF_RFK, "[DBCC]S0 reload idx=%x\n", idx);
	}

	if (kpath & BIT(1)) {
		halrf_wreg(rf, 0x8204, MASKDWORD, rf->kip_table[idx][2]);
		halrf_wreg(rf, 0x8254, MASKDWORD, rf->kip_table[idx][3]);
		RF_DBG(rf, DBG_RF_RFK, "[DBCC]S1 reload idx=%x\n", idx);
	}
}


bool halrf_chlk_reload_check_dbcc(struct rf_info *rf, enum phl_phy_idx phy) 
{
	struct halrf_dbcc_info *dbcc_info = &rf->dbcc_info;
	struct halrf_mcc_info *mcc_info = &rf->mcc_info;
	u8 path, i, j, idx, kpath, kch, kband;
	bool reload = false;

	RF_DBG(rf, DBG_RF_RFK, "[DBCC]======> %s \n", __func__);
	kpath = halrf_kpath(rf, phy);
	kch = rf->hal_com->band[phy].cur_chandef.center_ch;
	kband = rf->hal_com->band[phy].cur_chandef.band;
	idx = dbcc_info->table_idx;
	RF_DBG(rf, DBG_RF_RFK, "[DBCC]dbcc_en=%d  prek_is_dbcc=%d\n", rf->hal_com->dbcc_en, dbcc_info->prek_is_dbcc);
	if (rf->hal_com->dbcc_en || dbcc_info->prek_is_dbcc) {
		//try reload
		for(i = 0; i < 2; i++) {
			if (kpath == RF_AB) {
				if (kch == dbcc_info->ch[i][1] && kband == dbcc_info->band[i][1] &&
					kch == dbcc_info->ch[i][0] && kband == dbcc_info->band[i][0]) {
					idx = i;
					reload = true;
				}
			} else {
				if (kpath == RF_A)
					path = 0;
				else
					path = 1;
				if (kch == dbcc_info->ch[i][path] && kband == dbcc_info->band[i][path]) {
					idx = i;
					reload = true;
				}
			}
		}
		if (reload) {
			halrf_chlk_reload_dbcc(rf, phy, idx);
			rf->chlk_map = 0xffffffff & (~HAL_RF_IQK) & (~HAL_RF_DPK);
			RF_DBG(rf, DBG_RF_RFK, "[DBCC]reload kpath=%d, index=%d\n", kpath, idx);
			RF_DBG(rf, DBG_RF_RFK, "[DBCC]table0 S0 ch=%5d S1 ch=%5d\n", dbcc_info->ch[0][0], dbcc_info->ch[0][1]);
			RF_DBG(rf, DBG_RF_RFK, "[DBCC]table1 S0 ch=%5d S1 ch=%5d\n", dbcc_info->ch[1][0], dbcc_info->ch[1][1]);
			RF_DBG(rf, DBG_RF_RFK, "[DBCC]table0 S0 band =%5d S1 band =%5d\n", dbcc_info->band[0][0], dbcc_info->band[0][1]);
			RF_DBG(rf, DBG_RF_RFK, "[DBCC]table1 S0 band =%5d S1 band =%5d\n", dbcc_info->band[1][0], dbcc_info->band[1][1]);
			return reload;
		}
	}
	//force K
	for  (i = 0;  i < 2; i++)
		if (dbcc_info->ch[i][0] == 0 && dbcc_info->ch[i][1] == 0)
			break;
	if (i < 2) {
		idx = i;
	} else {
		for  (j = 0;  j < 2; j++)
			if (dbcc_info->ch[j][0] != dbcc_info->ch[j][1] ||
				dbcc_info->band[j][0] != dbcc_info->band[j][1])
				break;
		if (j == 2) {
			idx++;
			if (idx > 1)
				idx = 0;
		} else {
			idx = j;
		}
	}
	rf->chlk_map = 0xffffffff;
	dbcc_info->prek_is_dbcc = rf->hal_com->dbcc_en;
	dbcc_info->table_idx = idx;
	mcc_info->table_idx = idx;
	for (path = 0; path < 2; path++) {
		if (kpath & BIT(path)) {
			dbcc_info->ch[idx][path] = kch;
			dbcc_info->band[idx][path] = kband;
		}
	}
	RF_DBG(rf, DBG_RF_RFK, "[DBCC]foreK kpath=%d, index=%d\n", kpath, idx);
	RF_DBG(rf, DBG_RF_RFK, "[DBCC]ch00=%d ch01=%d ch10=%d ch11=%d\n",
		dbcc_info->ch[0][0], dbcc_info->ch[0][1], dbcc_info->ch[1][0], dbcc_info->ch[1][1]);
	RF_DBG(rf, DBG_RF_RFK, "[DBCC]band00=%d band01=%d band10=%d band10=%d\n",
		dbcc_info->band[0][0], dbcc_info->band[0][1], dbcc_info->band[1][0], dbcc_info->band[1][1]);

	return reload;
}

void halrf_reset_io_count(struct rf_info *rf)
{
	rf->w_count = 0;
	rf->r_count = 0;
	rf->fw_w_count = 0;
	rf->fw_r_count = 0;
	rf->sw_trigger_count = 0;
	rf->pre_fw_w_count = 0;
}

void halrf_common_setting_chl_rfk(struct rf_info *rf, enum phl_phy_idx phy, bool is_before_k)
{
	/*follow the original flow in PHL*/
	if (is_before_k) {
		#ifdef CONFIG_PHL_DFS
		if (halrf_is_radar_detect_enabled(rf, phy))
			halrf_bb_dfs_rpt_cfg(rf, phy, false);
		#endif

		halrf_mac_ctrl_ser(rf, HAL_SER_RSN_RFK, false);
	} else {
		halrf_mac_ctrl_ser(rf, HAL_SER_RSN_RFK, true);

		#ifdef CONFIG_PHL_DFS
		if (halrf_is_radar_detect_enabled(rf, phy))
			halrf_bb_dfs_rpt_cfg(rf, phy, true);
		#endif
	}
}

bool halrf_is_under_cac(struct rf_info *rf, enum phl_phy_idx phy)
{
	bool is_cac = false;

#ifdef CONFIG_PHL_DFS
	is_cac = rtw_hal_is_under_cac((rf)->hal_com, phy);
#endif
	return is_cac;
}

//
void halrf_ops_rx_dck(struct rf_info *rf, enum phl_phy_idx phy, bool is_afe)
{	
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_rx_dck) 
			rfk_ops->halrf_ops_rx_dck(rf, phy, is_afe);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_do_txgapk(struct rf_info *rf, enum phl_phy_idx phy)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_do_txgapk) 
			rfk_ops->halrf_ops_do_txgapk(rf, phy);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;

}

void halrf_ops_tssi_disable(struct rf_info *rf, enum phl_phy_idx phy)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_tssi_disable) 
			rfk_ops->halrf_ops_tssi_disable(rf, phy);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_do_tssi(struct rf_info *rf, enum phl_phy_idx phy, bool hwtx_en)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_do_tssi) 
			rfk_ops->halrf_ops_do_tssi(rf, phy, hwtx_en);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_dpk(struct rf_info *rf, enum phl_phy_idx phy, bool force)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_dpk) 
			rfk_ops->halrf_ops_dpk(rf, phy, force);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_dack(struct rf_info *rf, bool force)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_dack) 
			rfk_ops->halrf_ops_dack(rf, force);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_lck(struct rf_info *rf)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_lck) 
			rfk_ops->halrf_ops_lck(rf);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_lck_tracking(struct rf_info *rf)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_lck_tracking) 
			rfk_ops->halrf_ops_lck_tracking(rf);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_lo_test(struct rf_info *rf, bool is_on, enum rf_path path)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_lo_test) 
			rfk_ops->halrf_ops_lo_test(rf, is_on, path);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_config_radio_to_fw(struct rf_info *rf)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_config_radio_to_fw) 
			rfk_ops->halrf_config_radio_to_fw(rf);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_txgapk_w_table_default(struct rf_info *rf, enum phl_phy_idx phy)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;
	
	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_txgapk_w_table_default) 
			rfk_ops->halrf_ops_txgapk_w_table_default(rf, phy);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_txgapk_enable(struct rf_info *rf, enum phl_phy_idx phy)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;
	
	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_txgapk_enable) 
			rfk_ops->halrf_ops_txgapk_enable(rf, phy);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

void halrf_ops_txgapk_init(struct rf_info *rf)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;
	
	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_txgapk_init) 
			rfk_ops->halrf_ops_txgapk_init(rf);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;


}

void halrf_ops_adie_pow_ctrl(struct rf_info *rf, bool rf_off, bool others_off)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;
	
	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_adie_pow_ctrl) 
			rfk_ops->halrf_ops_adie_pow_ctrl(rf, rf_off, others_off);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;


}

void halrf_ops_afe_pow_ctrl(struct rf_info *rf, bool adda_off, bool pll_off)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;
	
	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_afe_pow_ctrl) 
			rfk_ops->halrf_ops_afe_pow_ctrl(rf, adda_off, pll_off);
		else 
			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;


}

void halrf_ops_set_gpio_by_ch(struct rf_info *rf, enum phl_phy_idx phy, enum band_type band)
{
	struct halrf_rfk_ops *rfk_ops = rf->rf_rfk_ops;

	if (rf->rf_rfk_ops) {
		if (rfk_ops->halrf_ops_set_gpio_by_ch) 
			rfk_ops->halrf_ops_set_gpio_by_ch(rf, phy, band);
//		else 
//			RF_WARNING("%s,function pointer is NULL, please check halrf_ops_rtlxxx.c .h\n", __func__);
	}
	else 
		RF_WARNING("%s,rf->rf_rfk_ops is NULL, please check rf_set_ops_xxx in halrf_init.c  \n", __func__);

	return;
}

u32 halrf_c2h_rfk_parsing(struct rf_info *rf, u8 cmdid, u16 len, u8 *c2h)
{
	u32 i;

	if (!c2h) {
		RF_WARNING("%s==>invalid c2h", __func__);		
		return 0;
	}
	if (rf->dbg_component & DBG_RF_FW) {
		for (i = 0; i < len; i++)
			RF_DBG(rf, DBG_RF_FW, "%x\n", c2h[i]);
	}

	switch(cmdid) {
	case RFK_LOG_DACK:
//		halrf_dack_fwrpt_8852c(rf, len, c2h);
		break;
	default:
		RF_WARNING("%s==>no cmdid is matching", __func__);
		break;
	}
	return 1;
}

u32 halrf_c2h_parsing(struct rf_info *rf, u8 classid, u8 cmdid, u16 len, u8 *c2h)
{
	u32 val = 0;

	RF_DBG(rf, DBG_RF_FW, "%s==>class=0x%x func=0x%x len=0x%x\n",
		__func__, classid, cmdid, len);

	switch(classid) {
	case HALRF_C2H_RFK_LOG:
		val = halrf_c2h_rfk_parsing(rf, cmdid, len, c2h);
		break;
	default:
		RF_WARNING("%s, no classid is matching\n", __func__);
		break;
	}
	return val;
}

void halrf_rpt_rt_rfk_info(struct rf_info *rf, enum phl_phy_idx phy, u32 type)
{
	struct halrf_rt_rpt *rpt = &rf->rf_rt_rpt;
	u8 i;	

	if (type == 1) {
		for (i = 0; i < 9; i++) {
			rpt->ch_info[9 - i][0][0] = rpt->ch_info[8 - i][0][0];
			rpt->ch_info[9 - i][0][1] = rpt->ch_info[8 - i][0][1];
			rpt->ch_info[9 - i][1][0] = rpt->ch_info[8 - i][1][0];
			rpt->ch_info[9 - i][1][1] = rpt->ch_info[8 - i][1][1];
		}

		for (i = 0; i < halrf_max_path_num(rf); i++) {
			rpt->ch_info[0][i][0] = halrf_rrf(rf, i, 0x18, 0x1ff);
			rpt->ch_info[0][i][1] = halrf_rrf(rf, i, 0xb2, 0x3ff);
		}
	}else if (type == 2) {
		for (i = 0; i < 9; i++) {
			rpt->tssi_code[9 - i][0] = rpt->tssi_code[8 - i][0];
			rpt->tssi_code[9 - i][1] = rpt->tssi_code[8 - i][1];
		}
		rpt->tssi_code[0][0] = (u8)halrf_rreg(rf, 0x1c60, 0xff000000);
		rpt->tssi_code[0][1]= (u8)halrf_rreg(rf, 0x3c60, 0xff000000);
	}
}

void halrf_ex_dack_info(struct rf_info *rf)
{
	struct rtw_hal_com_t *hal_i = rf->hal_com;
	struct halrf_dack_info *dack = &rf->dack;
	char *ic_name = NULL;
	u32 dack_ver = 0;
	u32 rf_para = 0;
	u32 rfk_init_ver = 0;
	u8 i;

	switch (hal_i->chip_id) {
#ifdef RF_8852A_SUPPORT
	case CHIP_WIFI6_8852A:
		ic_name = "8852A";
		dack_ver = DACK_VER_8852AB;
		rf_para = halrf_get_radio_reg_ver(rf);
		break;
#endif
		default:
		break;
	}

	RF_TRACE("\n===============[ DACK info %s ]===============\n", ic_name);
	RF_TRACE(" %-25s = 0x%x\n", "DACK Ver", dack_ver);
	RF_TRACE(" %-25s = %d ms\n", "RF Para Ver", rf_para);
	if (dack->dack_cnt == 0) {
		RF_TRACE("\n %-25s\n",
			 "No DACK had been done before!!!");
		return;
	}

	RF_TRACE(" %-25s = %d\n",
		 "DACK count", dack->dack_cnt);
	RF_TRACE(" %-25s = %d ms\n",
		 "DACK processing time", dack->dack_time);
	RF_TRACE(" %-60s = %d / %d / %d / %d / %d / %d\n",
		 "DACK timeout(ADDCK_0/ADDCK_1/DADCK_0/DADCK_1/MSBK_0/MSBK_1):",
		 dack->addck_timeout[0], dack->addck_timeout[1],
		 dack->dadck_timeout[0], dack->dadck_timeout[1],
		 dack->msbk_timeout[0], dack->msbk_timeout[1]);
	RF_TRACE(" %-25s = %s\n",
		 "DACK Fail(last)", (dack->dack_fail) ? "TRUE" : "FALSE");		
	RF_TRACE("===============[ ADDCK result ]===============\n");
	RF_TRACE(" %-25s = 0x%x / 0x%x \n",
		 "S0_I/ S0_Q", dack->addck_d[0][0], dack->addck_d[0][1]);
	RF_TRACE(" %-25s = 0x%x / 0x%x \n",
		 "S1_I/ S1_Q", dack->addck_d[1][0], dack->addck_d[1][1]);

	RF_TRACE("===============[ DADCK result ]===============\n");
	RF_TRACE(" %-25s = 0x%x / 0x%x \n",
		 "S0_I/ S0_Q", dack->dadck_d[0][0], dack->dadck_d[0][1]);
	RF_TRACE(" %-25s = 0x%x / 0x%x \n",
		 "S1_I/ S1_Q", dack->dadck_d[1][0], dack->dadck_d[1][1]);

	RF_TRACE("===============[ biask result ]===============\n");
	RF_TRACE(" %-25s = 0x%x / 0x%x \n",
		 "S0_I/ S0_Q", dack->biask_d[0][0], dack->biask_d[0][1]);
	RF_TRACE(" %-25s = 0x%x / 0x%x \n",
		 "S1_I/ S1_Q", dack->biask_d[1][0], dack->biask_d[1][1]);

	RF_TRACE("===============[ MSBK result ]===============\n");
	for (i = 0; i < 16; i++) {
		RF_TRACE(" %s [%2d] = 0x%x/ 0x%x/ 0x%x/ 0x%x\n",
			 "S0_I/S0_Q/S1_I/S1_Q",
			 i,
			 dack->msbk_d[0][0][i], dack->msbk_d[0][1][i],
			 dack->msbk_d[1][0][i], dack->msbk_d[1][1][i]);
	}
}

void halrf_ex_iqk_info(struct rf_info *rf)
{
	struct rtw_hal_com_t *hal_i = rf->hal_com;
	struct halrf_iqk_info *iqk_info = &rf->iqk;
	char *ic_name = NULL;
	u32 ver = 0;
	u32 rfk_init_ver = 0;
	u8 tmp = iqk_info->iqk_table_idx[0];

	switch (hal_i->chip_id) {
#ifdef RF_8852A_SUPPORT
	case CHIP_WIFI6_8852A:
		ic_name = "8852A";
		break;
#endif
#ifdef RF_8852B_SUPPORT
	case CHIP_WIFI6_8852B:
		ic_name = "8852B";
		break;
#endif
#ifdef RF_8852BT_SUPPORT
	case CHIP_WIFI6_8852BT:
		ic_name = "8852BT";
		break;
#endif
#ifdef RF_8852C_SUPPORT
	case CHIP_WIFI6_8852C:
		ic_name = "8852C";
		break;
#endif
#ifdef RF_8832BR_SUPPORT
	case CHIP_WIFI6_8832BR:
		ic_name = "8832BR";
		break;
#endif
#ifdef RF_8192XB_SUPPORT
	case CHIP_WIFI6_8192XB:
		ic_name = "8192XB";
		break;
#endif
	default:
		break;
	}
	
	ver = halrf_get_iqk_ver(rf);
	rfk_init_ver = halrf_get_nctl_reg_ver(rf);
	RF_TRACE(
		 "\n===============[ IQK info %s ]===============\n", ic_name);
	RF_TRACE(" %-25s = 0x%x\n",
		 "IQK Version", ver);
	RF_TRACE(" %-25s = 0x%x\n",
		 "RFK init ver", rfk_init_ver);	
	RF_TRACE(" %-25s = %d / %d / %d\n",
		 "IQK Cal / Fail / Reload", iqk_info->iqk_times, iqk_info->iqk_fail_cnt,
		 iqk_info->reload_cnt);
	RF_TRACE(" %-25s = %s / %d / %s\n",
		 "S0 Band / CH / BW",  iqk_info->iqk_band[0]== 0 ? "2G" : (iqk_info->iqk_band[0] == 1 ? "5G" : "6G"),
		 iqk_info->iqk_ch[0],
		 iqk_info->iqk_bw[0] == 0 ? "20M" : (iqk_info->iqk_bw[0] == 1 ? "40M" : "80M"));	
	RF_TRACE(" %-25s = %s\n",
		 "S0 NB/WB TXIQK", iqk_info->is_wb_txiqk[0]? "WBTXK" : "NBTXK");
	RF_TRACE(" %-25s = %s\n",
		 "S0 NB/WB RXIQK", iqk_info->is_wb_rxiqk[0]? "WBRXK" : "NBRXK");
	RF_TRACE(" %-25s = %s\n",
		 "S0 LOK status", (iqk_info->lok_cor_fail[0][0] | iqk_info->lok_fin_fail[0][0]) ? "Fail" : "Pass");
	RF_TRACE(" %-25s = %s\n",
		 "S0 TXK status", iqk_info->iqk_tx_fail[0][0]? "Fail" : "Pass");
	RF_TRACE(" %-25s = %s\n",
		 "S0 RXK status", iqk_info->iqk_rx_fail[0][0]? "Fail" : "Pass");
	RF_TRACE(" %-25s = %x/ %x\n",
		 "S0 LOK iDACK/VBUF", iqk_info->lok_idac[tmp][0], iqk_info->lok_vbuf[tmp][0]);
	RF_TRACE(" %-25s = %x\n",
		 "S0 TXK XYM", iqk_info->nb_txcfir[0]);
	RF_TRACE(" %-25s = %x\n",
		 "S0 RXK XYM", iqk_info->nb_rxcfir[0]);
	RF_TRACE(" %-25s = %s / %d / %s\n",
		 "S1 Band / CH / BW",  iqk_info->iqk_band[1]== 0 ? "2G" : (iqk_info->iqk_band[1] == 1 ? "5G" : "6G"),
		 iqk_info->iqk_ch[1],
		 iqk_info->iqk_bw[1] == 0 ? "20M" : (iqk_info->iqk_bw[1] == 1 ? "40M" : "80M"));
	RF_TRACE(" %-25s = %s\n",
		 "S1 NB/WB TXIQK", iqk_info->is_wb_txiqk[1]? "WBTXK" : "NBTXK");
	RF_TRACE(" %-25s = %s\n",
		 "S1 NB/WB RXIQK", iqk_info->is_wb_rxiqk[1]? "WBRXK" : "NBRXK");
	RF_TRACE(" %-25s = %s\n",
		 "S1 LOK status", (iqk_info->lok_cor_fail[0][1] | iqk_info->lok_fin_fail[0][1]) ? "Fail" : "Pass");
	RF_TRACE(" %-25s = %s\n",
		 "S1 TXK status", iqk_info->iqk_tx_fail[0][1]? "Fail" : "Pass");
	RF_TRACE(" %-25s = %s\n",
		 "S1 RXK status", iqk_info->iqk_rx_fail[0][1]? "Fail" : "Pass");
	RF_TRACE(" %-25s = %x/  %x\n",
		 "S1 LOK iDACK/VBUF", iqk_info->lok_idac[tmp][1], iqk_info->lok_vbuf[tmp][1]);
	RF_TRACE(" %-25s = %x\n",
		 "S1 TXK XYM", iqk_info->nb_txcfir[1]);
	RF_TRACE(" %-25s = %x\n",
		 "S1 RXK XYM", iqk_info->nb_rxcfir[1]);
}

void halrf_ex_dpk_info(struct rf_info *rf)
{
	struct rtw_hal_com_t *hal_i = rf->hal_com;
	struct halrf_dpk_info *dpk = &rf->dpk;

	char *ic_name = NULL;
	u32 dpk_ver = 0;
	u32 rf_para = 0;
	u32 rfk_init_ver = 0;
	u8 path, kidx;
	u32 rf_para_min = 0;

	switch (hal_i->chip_id) {
#ifdef RF_8852A_SUPPORT
	case CHIP_WIFI6_8852A:
		ic_name = "8852A";
		dpk_ver = DPK_VER_8852A;
		rf_para_min = 16;
		break;
#endif
#ifdef RF_8852B_SUPPORT
	case CHIP_WIFI6_8852B:
		ic_name = "8852B";
		dpk_ver = DPK_VER_8852B;
		break;
#endif
#ifdef RF_8852BT_SUPPORT
	case CHIP_WIFI6_8852BT:
		ic_name = "8852BT";
		dpk_ver = DPK_VER_8852BT;
		break;
#endif
#ifdef RF_8852C_SUPPORT
	case CHIP_WIFI6_8852C:
		ic_name = "8852C";
		dpk_ver = DPK_VER_8852C;
		break;
#endif
#ifdef RF_8851B_SUPPORT
	case CHIP_WIFI6_8851B:
		ic_name = "8851B";
		dpk_ver = DPK_VER_8851B;
		break;
#endif
#ifdef RF_8832BR_SUPPORT
	case CHIP_WIFI6_8832BR:
		ic_name = "8832BR";
		dpk_ver = DPK_VER_8832BR;
		break;
#endif
#ifdef RF_8192XB_SUPPORT
	case CHIP_WIFI6_8192XB:
		ic_name = "8192XB";
		dpk_ver = DPK_VER_8192XB;
		break;
#endif
#ifdef RF_8852BP_SUPPORT
	case CHIP_WIFI6_8852BP:
		ic_name = "8852BP";
		dpk_ver = DPK_VER_8852BP;
		break;
#endif
	default:
		break;
	}

	rf_para = halrf_get_radio_reg_ver(rf);
	rfk_init_ver = halrf_get_nctl_reg_ver(rf);

	RF_TRACE("\n===============[ DPK info %s ]===============\n", ic_name);
	RF_TRACE(" %-25s = 0x%x\n", "DPK Ver", dpk_ver);

	RF_TRACE(" %-25s = %d (%s)\n",
		 "RF Para Ver", rf_para, rf_para >= rf_para_min ? "match" : "mismatch");

	RF_TRACE(" %-25s = 0x%x\n", "RFK init ver", rfk_init_ver);

	RF_TRACE(" %-25s = %d / %d / %d (RFE type:%d)\n",
		 "Ext_PA 2G / 5G / 6G", rf->fem.epa_2g, rf->fem.epa_5g, rf->fem.epa_6g,
		 rf->phl_com->dev_cap.rfe_type);

	if (dpk->bp[0][0].ch == 0) {
		RF_TRACE("\n %-25s\n", "No DPK had been done before!!!");
		return;
	}

	RF_TRACE(" %-25s = %d / %d / %d\n",
		 "DPK Cal / OK / Reload", dpk->dpk_cal_cnt, dpk->dpk_ok_cnt,
		 dpk->dpk_reload_cnt);

	RF_TRACE(" %-25s = %s\n",
		 "BT IQK timeout", rf->is_bt_iqk_timeout ? "Yes" : "No");

	RF_TRACE(" %-25s = %d ms\n",
		 "DPK processing time", dpk->dpk_time);

	RF_TRACE(" %-25s = %s\n",
		 "DPD status", dpk->is_dpk_enable ? "Enable" : "Disable");

	RF_TRACE(" %-25s = %s\n",
		 "DPD track status", dpk->is_dpk_track_en ? "Enable" : "Disable");

	RF_TRACE(" %-25s = %s / %s\n",
		 "DBCC / TSSI", rf->hal_com->dbcc_en ? "On" : "Off",
		 rf->is_tssi_mode[0] ? "On" : "Off");

	for (path = 0; path < KPATH; path++) {
		for (kidx = 0; kidx < DPK_BKUP_NUM; kidx++) {
			if (dpk->bp[path][kidx].ch == 0)
				break;

			RF_TRACE("=============== S%d[%d] ===============\n", path, kidx);
			RF_TRACE(" %-25s = %s / %d / %s\n",
				 "Band / CH / BW", dpk->bp[path][kidx].band == 0 ? "2G" : (dpk->bp[path][kidx].band == 1 ? "5G" : "6G"),
				 dpk->bp[path][kidx].ch,
				 dpk->bp[path][kidx].bw == 0 ? "20M" : (dpk->bp[path][kidx].bw == 1 ? "40M" : 
				 (dpk->bp[path][kidx].bw == 2 ? "80M" : "160M")));

			RF_TRACE(" %-25s = %s\n",
				 "DPK result", dpk->bp[path][kidx].path_ok ? "OK" : "Fail");

			RF_TRACE( " %-25s = %d / %d\n",
				 "ReK_cnt[0] / ReK_cnt[1]", dpk->rek_cnt[path][0], dpk->rek_cnt[path][1]);

			RF_TRACE(" %-25s = 0x%x / 0x%x / 0x%x / 0x%x / 0x%x/\n",
				 "ReK[0] Check", dpk->rek_chk[path][0][0], dpk->rek_chk[path][0][1], dpk->rek_chk[path][0][2],
				 dpk->rek_chk[path][0][3], dpk->rek_chk[path][0][4]);

			RF_TRACE(" %-25s = 0x%x / 0x%x / 0x%x / 0x%x / 0x%x/\n",
				 "ReK[1] Check", dpk->rek_chk[path][1][0], dpk->rek_chk[path][1][1], dpk->rek_chk[path][1][2],
				 dpk->rek_chk[path][1][3], dpk->rek_chk[path][1][4]);

			RF_TRACE(" %-25s = 0x%x / 0x%x\n",
				 "DPK TxAGC / Gain Scaling", dpk->bp[path][kidx].txagc_dpk, dpk->bp[path][kidx].gs);

			RF_TRACE(" %-25s = %d / %d\n",
				 "Corr (idx/val)", dpk->corr_idx[path][kidx], dpk->corr_val[path][kidx]);

			RF_TRACE(" %-25s = 0x%x\n",
				 "DPK RXIQC", dpk->dpk_rxiqc[path]);

			RF_TRACE(" %-25s = %d / %d\n",
				 "DC (I/Q)", dpk->dc_i[path][kidx], dpk->dc_q[path][kidx]);

			RF_TRACE(" %-25s = 0x%x / 0x%x\n",
				 "IDL_Sync / DC", dpk->dpk_sync[path], dpk->dpk_dciq[path]);

			RF_TRACE(" %-25s = %d / %d\n",
				 "LDL_OV / RXBB_OV", dpk->ov_flag[path], dpk->rxbb_ov[path]);
		}
	}
}

void halrf_ex_rx_dck_info(struct rf_info *rf)
{
	struct rtw_hal_com_t *hal_i = rf->hal_com;
	struct halrf_rx_dck_info *rx_dck = &rf->rx_dck;

	char *ic_name = NULL;
	u32 rxdck_ver = 0;
	u8 path;
	u32 addr = 0;
	u32 reg_05[KPATH];

	switch (hal_i->chip_id) {
#ifdef RF_8852A_SUPPORT
	case CHIP_WIFI6_8852A:
		ic_name = "8852A";
		rxdck_ver = RXDCK_VER_8852A;
		break;
#endif
#ifdef RF_8852B_SUPPORT
	case CHIP_WIFI6_8852B:
		ic_name = "8852B";
		rxdck_ver = RXDCK_VER_8852B;
		break;
#endif
#ifdef RF_8852BT_SUPPORT
	case CHIP_WIFI6_8852BT:
		ic_name = "8852BT";
		rxdck_ver = RXDCK_VER_8852BT;
		break;
#endif
#ifdef RF_8852C_SUPPORT
	case CHIP_WIFI6_8852C:
		ic_name = "8852C";
		rxdck_ver = RXDCK_VER_8852C;
		break;
#endif
#ifdef RF_8851B_SUPPORT
	case CHIP_WIFI6_8851B:
		ic_name = "8851B";
		rxdck_ver = RXDCK_VER_8851B;
		break;
#endif
#ifdef RF_8832BR_SUPPORT
	case CHIP_WIFI6_8832BR:
		ic_name = "8832BR";
		rxdck_ver = RXDCK_VER_8832BR;
		break;
#endif
#ifdef RF_8192XB_SUPPORT
	case CHIP_WIFI6_8192XB:
		ic_name = "8192XB";
		rxdck_ver = RXDCK_VER_8192XB;
		break;
#endif
#ifdef RF_8852BP_SUPPORT
	case CHIP_WIFI6_8852BP:
		ic_name = "8192XB";
		rxdck_ver = RXDCK_VER_8852BP;
		break;
#endif
		default:
		break;
	}

	RF_TRACE( "\n===============[ RX_DCK info %s ]===============\n", ic_name);

	RF_TRACE(" %-25s = 0x%x\n",
		 "RX_DCK Ver", rxdck_ver);
	
	if (rx_dck->loc[0].cur_ch == 0) {
		RF_TRACE("\n %-25s\n",
			"No RX_DCK had been done before!!!");
		return;
	}

	RF_TRACE(" %-25s = %d ms\n",
		 "RX_DCK processing time", rx_dck->rxdck_time);

	for (path = 0; path < KPATH; path++) {
		if (rx_dck->loc[path].cur_ch == 0)
			break;

		RF_TRACE(" S%d:", path);
		RF_TRACE(" %-25s = %s/ %d/ %s/ %s/ 0x%x\n",
			 "Band/ CH/ BW/ Cal/ Ther", rx_dck->loc[path].cur_band == 0 ? "2G" :
			(rx_dck->loc[path].cur_band == 1 ? "5G" : "6G"),
			rx_dck->loc[path].cur_ch,
		        rx_dck->loc[path].cur_bw == 0 ? "20M" :
		        (rx_dck->loc[path].cur_bw == 1 ? "40M" : 
			(rx_dck->loc[path].cur_bw == 2 ? "80M" : "160M")),
		       	rx_dck->is_afe ? "AFE" : "RFC", rx_dck->ther_rxdck[path]);
	}

	for (path = 0; path < KPATH; path++) {
		if (rx_dck->loc[path].cur_ch == 0)
			break;

		RF_TRACE("\n---------------[ S%d DCK Value ]---------------\n", path);
		reg_05[path] = halrf_rrf(rf, path, 0x5, MASKRF);
		halrf_wrf(rf, path, 0x5, BIT(0), 0x0);
		halrf_wrf(rf, path, 0x00, MASKRFMODE, RF_RX);

		for (addr = 0; addr < 0x20; addr++) {
			halrf_wrf(rf, path, 0x00, 0x07c00, addr); /*[14:10]*/
			if (hal_i->chip_id == CHIP_WIFI6_8852C ||
			    hal_i->chip_id == CHIP_WIFI6_8852BP ||
			    hal_i->chip_id == CHIP_WIFI6_8851B
#ifdef RF_8192XB_SUPPORT
			    || hal_i->chip_id == CHIP_WIFI6_8192XB
#endif
			    )
				RF_TRACE("0x%02x | 0x%02x/ 0x%02x   0x%02x/ 0x%02x\n", addr,
				    halrf_rrf(rf, path, 0x92, 0xF0000),  /*[19:16]*/
				    halrf_rrf(rf, path, 0x92, 0x0FE00),  /*[15:9]*/
				    halrf_rrf(rf, path, 0x93, 0xF0000),  /*[19:16]*/
				    halrf_rrf(rf, path, 0x93, 0x0FE00)); /*[15:9]*/
			else
				RF_TRACE("0x%02x | 0x%02x/ 0x%02x   0x%02x/ 0x%02x\n", addr,
				    halrf_rrf(rf, path, 0x92, 0xF0000),  /*[19:16]*/
				    halrf_rrf(rf, path, 0x92, 0x0FC00),  /*[15:10]*/
				    halrf_rrf(rf, path, 0x93, 0xF0000),  /*[19:16]*/
				    halrf_rrf(rf, path, 0x93, 0x0FC00)); /*[15:10]*/
		}
		halrf_wrf(rf, path, 0x5, BIT(0), reg_05[path]);
	}
}

void halrf_ex_gapk_info(struct rf_info *rf)
{
	struct rtw_hal_com_t *hal_i = rf->hal_com;

	struct halrf_gapk_info *txgapk_info = &rf->gapk;
	struct halrf_iqk_info *iqk_info = &rf->iqk;
	struct halrf_mcc_info *mcc_info = &rf->mcc_info;
	u8 i;
	u8 channel = rf->hal_com->band[0].cur_chandef.center_ch;
	u32 bw = rf->hal_com->band[0].cur_chandef.bw;
	u32 band = rf->hal_com->band[0].cur_chandef.band;
	char *ic_name = NULL;
	u32 txgapk_ver = 0;
	u32 rf_para = 0;
	u32 rfk_init_ver = 0;

	switch (hal_i->chip_id) {
#ifdef RF_8852A_SUPPORT
	case CHIP_WIFI6_8852A:
		ic_name = "8852A";
		txgapk_ver = TXGAPK_VER_8852A;
		rf_para = halrf_get_radio_reg_ver(rf);
		rfk_init_ver = halrf_get_nctl_reg_ver(rf);
		break;
#endif

#ifdef RF_8852B_SUPPORT
	case CHIP_WIFI6_8852B:
		ic_name = "8852B";
		txgapk_ver = TXGAPK_VER_8852B;
		rf_para = halrf_get_radio_reg_ver(rf);
		rfk_init_ver = halrf_get_nctl_reg_ver(rf);
		break;
#endif

#ifdef RF_8852BT_SUPPORT
	case CHIP_WIFI6_8852BT:
		ic_name = "8852BT";
		txgapk_ver = TXGAPK_VER_8852BT;
		rf_para = halrf_get_radio_reg_ver(rf);
		rfk_init_ver = halrf_get_nctl_reg_ver(rf);
		break;
#endif

#ifdef RF_8852C_SUPPORT
	case CHIP_WIFI6_8852C:
		ic_name = "8852C";
		txgapk_ver = TXGAPK_VER_8852C;
		rf_para = halrf_get_radio_reg_ver(rf);
		rfk_init_ver = halrf_get_nctl_reg_ver(rf);
		break;
#endif

#ifdef RF_8832BR_SUPPORT
	case CHIP_WIFI6_8832BR:
		ic_name = "8832BR";
		txgapk_ver = TXGAPK_VER_8832BR;
		rf_para = halrf_get_radio_reg_ver(rf);
		rfk_init_ver = halrf_get_nctl_reg_ver(rf);
		break;
#endif

#ifdef RF_8192XB_SUPPORT
	case CHIP_WIFI6_8192XB:
		ic_name = "8192XB";
		txgapk_ver = TXGAPK_VER_8192XB;
		rf_para = halrf_get_radio_reg_ver(rf);
		rfk_init_ver = halrf_get_nctl_reg_ver(rf);
		break;
#endif

#ifdef RF_8852BP_SUPPORT
	case CHIP_WIFI6_8852BP:
		ic_name = "8852BP";
		txgapk_ver = TXGAPK_VER_8852BP;
		rf_para = halrf_get_radio_reg_ver(rf);
		rfk_init_ver = halrf_get_nctl_reg_ver(rf);
		break;
#endif

	default:
		break;
	}

	RF_TRACE(
		 "\n===============[ TxGapK info %s ]===============\n", ic_name);

	RF_TRACE(" %-25s = 0x%x\n",
		 "TxGapK Ver", txgapk_ver);

	RF_TRACE(" %-25s = %d\n",
		 "RF Para Ver", rf_para);

	RF_TRACE(" %-25s = 0x%x\n",
		 "RFK init ver", rfk_init_ver);

	RF_TRACE(" %-25s = %d ms\n",
		 "TxGapK processing time", txgapk_info->txgapk_time);

	RF_TRACE(" %-25s = %d / %d / %d (RFE type:%d)\n",
		 "Ext_PA 2G / 5G / 6G", rf->fem.epa_2g, rf->fem.epa_5g, rf->fem.epa_6g,
		 rf->phl_com->dev_cap.rfe_type);

	RF_TRACE(" %-25s = %s / %d / %s\n",
		 "Band / CH / BW", band == BAND_ON_24G ? "2G" : (band == BAND_ON_5G ? "5G" : "6G"),
		 channel,
		 bw == 0 ? "20M" : (bw == 1 ? "40M" : "80M"));

	RF_TRACE(
		 "=======================\n");
	/* table info */
	RF_TRACE(" %-25s = %d / %d\n",
		 "iqk_info->iqk_mcc_ch[0][0]/[0][1]", iqk_info->iqk_mcc_ch[0][0], iqk_info->iqk_mcc_ch[0][1]);	
	RF_TRACE(" %-25s = %d / %d\n",
		 "iqk_info->iqk_mcc_ch[1][0]/[1][1]", iqk_info->iqk_mcc_ch[1][0], iqk_info->iqk_mcc_ch[1][1]);		 
	RF_TRACE(" %-25s = %d / %d\n",
		 "iqk_info->iqk_table_idx[0]/[1]", iqk_info->iqk_table_idx[0], iqk_info->iqk_table_idx[1]);

	RF_TRACE(" %-25s = %d\n",
		 "txgapk_info->txgapk_mcc_ch[0]", txgapk_info->txgapk_mcc_ch[0]);
	RF_TRACE(" %-25s = %d\n",
		 "txgapk_info->txgapk_mcc_ch[1]", txgapk_info->txgapk_mcc_ch[1]);
	RF_TRACE(" %-25s = %d\n",
		 "txgapk_info->txgapk_table_idx", txgapk_info->txgapk_table_idx);
	RF_TRACE(" %-25s = %d\n",
		 "txgapk_info->ch", txgapk_info->ch[0]);

	RF_TRACE(" %-25s = %d\n",
		 "mcc_info->ch[0]", mcc_info->ch[0]);
	RF_TRACE(" %-25s = %d\n",
		 "mcc_info->ch[1]", mcc_info->ch[1]);
	RF_TRACE(" %-25s = %d\n",
		 "mcc_info->table_idx", mcc_info->table_idx);
	RF_TRACE(" %-25s = %d\n",
		 "mcc_info->band[0]", mcc_info->band[0]);
	RF_TRACE(" %-25s = %d\n",
		 "mcc_info->band[1]", mcc_info->band[1]);
		 

	RF_TRACE(
		 "===============[ TxGapK result ]===============\n");
	RF_TRACE(" %-25s = %s\n",
		 "TXGapK OK(last)", (txgapk_info->is_txgapk_ok) ? "TRUE" : "FALSE");

	#if defined(RF_8852B_SUPPORT) || defined(RF_8852BT_SUPPORT) || defined(RF_8852BP_SUPPORT)
	if ((hal_i->chip_id == CHIP_WIFI6_8852B) || (hal_i->chip_id == CHIP_WIFI6_8852BT) || (hal_i->chip_id == CHIP_WIFI6_8852BP)) {
		RF_TRACE(" %-25s = %s\n",
			"TXGapK d boundary check", (txgapk_info->d_bnd_ok) ? "PASS" : "FAILE");
	}
	#endif
	
	RF_TRACE(" %-25s = 0x%x / 0x%x\n",
		 "Read0x8010 Befr /Aftr GapK", txgapk_info->r0x8010[0], txgapk_info->r0x8010[1]);
	
	RF_TRACE(
		 "[ NCTL Done Check Times R_0xbff / R_0x80fc ]\n");

#ifdef RF_8852BP_SUPPORT
	RF_TRACE(" %-25s = 0x%x / 0x%x\n",
		"S[0]trk_gain_range_0x10011[6:4] / value_0x1005c[5:0]", halrf_rrf(rf, RF_PATH_A, 0x10011, 0x70), halrf_rrf(rf, RF_PATH_A, 0x1005c, 0x3f));

	RF_TRACE(" %-25s = 0x%x / 0x%x\n",
		"S[0]pwr_gain_range_0x10011[1:0] / value_0x1005E[5:0]", halrf_rrf(rf, RF_PATH_A, 0x10011, 0x03), halrf_rrf(rf, RF_PATH_A, 0x1005e, 0x3f));

	RF_TRACE(" %-25s = 0x%x / 0x%x\n",
		"S[1]trk_gain_range_0x10011[6:4] / value_0x1005c[5:0]", halrf_rrf(rf, RF_PATH_B, 0x10011, 0x70), halrf_rrf(rf, RF_PATH_B, 0x1005c, 0x3f));

	RF_TRACE(" %-25s = 0x%x / 0x%x\n",
		"S[1]pwr_gain_range_0x10011[1:0] / value_0x1005E[5:0]", halrf_rrf(rf, RF_PATH_B, 0x10011, 0x03), halrf_rrf(rf, RF_PATH_B, 0x1005e, 0x3f));
#endif

	/* txgapk_info->txgapk_chk_cnt[2][2][2]; */ /* path */ /* track pwr */ /* 0xbff8 0x80fc*/
	RF_TRACE(" %-25s = %d / %d\n",
		"Path_0 Track", txgapk_info->txgapk_chk_cnt[0][TXGAPK_TRACK][0], txgapk_info->txgapk_chk_cnt[0][TXGAPK_TRACK][1]);				
	RF_TRACE(" %-25s = %d / %d\n",
		"Path_0 PWR",txgapk_info->txgapk_chk_cnt[0][TXGAPK_PWR][0], txgapk_info->txgapk_chk_cnt[0][TXGAPK_PWR][1]);			
	RF_TRACE(" %-25s = %d / %d\n",
		"Path_0 IQKBK", txgapk_info->txgapk_chk_cnt[0][TXGAPK_IQKBK][0], txgapk_info->txgapk_chk_cnt[0][TXGAPK_IQKBK][1]);

	RF_TRACE(" %-25s = %d / %d\n",
		"Path_1 Track", txgapk_info->txgapk_chk_cnt[1][TXGAPK_TRACK][0], txgapk_info->txgapk_chk_cnt[1][TXGAPK_TRACK][1]);				
	RF_TRACE(" %-25s = %d / %d\n",
		"Path_1 PWR", txgapk_info->txgapk_chk_cnt[1][TXGAPK_PWR][0], txgapk_info->txgapk_chk_cnt[1][TXGAPK_PWR][1]);			
	RF_TRACE(" %-25s = %d / %d\n",
		"Path_1 IQKBK", txgapk_info->txgapk_chk_cnt[1][TXGAPK_IQKBK][0], txgapk_info->txgapk_chk_cnt[1][TXGAPK_IQKBK][1]);


	for (i = 0; i < 17; i++) {
		RF_TRACE(
			" %s [%2d] = 0x%02x/ 0x%02x/ 0x%02x/ 0x%02x\n",
			 "S0: Trk_d/Trk_ta/Pwr_d/Pwr_ta",
			 i,
			 txgapk_info->track_d[0][i]&0xff, txgapk_info->track_ta[0][i]&0xff,
			 txgapk_info->power_d[0][i]&0xff, txgapk_info->power_ta[0][i]&0xff);
	}
	for (i = 0; i < 17; i++) {
		RF_TRACE(
			" %s [%2d] = 0x%02x/ 0x%02x/ 0x%02x/ 0x%02x\n",
			 "S1: Trk_d/Trk_ta/Pwr_d/Pwr_ta",
			 i,
			 txgapk_info->track_d[1][i]&0xff, txgapk_info->track_ta[1][i]&0xff,
			 txgapk_info->power_d[1][i]&0xff, txgapk_info->power_ta[1][i]&0xff);
	}
}

void halrf_ex_rt_rfk_info(struct rf_info *rf)
{
	struct halrf_rt_rpt *rpt = &rf->rf_rt_rpt;

	RF_TRACE("s0 ch=[0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] \n",
		rpt->ch_info[0][0][0], rpt->ch_info[1][0][0], rpt->ch_info[2][0][0], rpt->ch_info[3][0][0],
		rpt->ch_info[4][0][0], rpt->ch_info[5][0][0], rpt->ch_info[6][0][0], rpt->ch_info[7][0][0],
		rpt->ch_info[8][0][0], rpt->ch_info[9][0][0]
		);
	RF_TRACE("s0 cv=[0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] \n",
		rpt->ch_info[0][0][1], rpt->ch_info[1][0][1], rpt->ch_info[2][0][1], rpt->ch_info[3][0][1],
		rpt->ch_info[4][0][1], rpt->ch_info[5][0][1], rpt->ch_info[6][0][1], rpt->ch_info[7][0][1],
		rpt->ch_info[8][0][1], rpt->ch_info[9][0][1]
		);
	RF_TRACE("s1 ch=[0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] \n",
		rpt->ch_info[0][1][0], rpt->ch_info[1][1][0], rpt->ch_info[2][1][0], rpt->ch_info[3][1][0],
		rpt->ch_info[4][1][0], rpt->ch_info[5][1][0], rpt->ch_info[6][1][0], rpt->ch_info[7][1][0],
		rpt->ch_info[8][1][0], rpt->ch_info[9][1][0]
		);
	RF_TRACE("s1 cv=[0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] [0x%03x] \n",
		rpt->ch_info[0][1][1], rpt->ch_info[1][1][1], rpt->ch_info[2][1][1], rpt->ch_info[3][1][1],
		rpt->ch_info[4][1][1], rpt->ch_info[5][1][1], rpt->ch_info[6][1][1], rpt->ch_info[7][1][1],
		rpt->ch_info[8][1][1], rpt->ch_info[9][1][1]
		);
	RF_TRACE("driver LCK fail count: %d, FW LCK fail count: %d\n", rpt->drv_lck_fail_count, rpt->fw_lck_fail_count);
	RF_TRACE("S0 TSSI=[0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] \n",
		rpt->tssi_code[0][0], rpt->tssi_code[1][0], rpt->tssi_code[2][0], rpt->tssi_code[3][0],
		rpt->tssi_code[4][0], rpt->tssi_code[5][0], rpt->tssi_code[6][0], rpt->tssi_code[7][0],
		rpt->tssi_code[8][0], rpt->tssi_code[9][0]
		);
	RF_TRACE("S1 TSSI=[0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] [0x%x] \n",
		rpt->tssi_code[0][1], rpt->tssi_code[1][1], rpt->tssi_code[2][1], rpt->tssi_code[3][1],
		rpt->tssi_code[4][1], rpt->tssi_code[5][1], rpt->tssi_code[6][1], rpt->tssi_code[7][1],
		rpt->tssi_code[8][1], rpt->tssi_code[9][1]
		);
}

void halrf_ex_rfk_info(struct rf_info *rf)
{
	halrf_ex_dack_info(rf);
	halrf_ex_iqk_info(rf);
	halrf_ex_dpk_info(rf);
	halrf_ex_rx_dck_info(rf);
	halrf_ex_gapk_info(rf);
}

struct halrf_fem_info halrf_ex_efem_info(struct rf_info *rf)
{
	struct halrf_fem_info fem_type;

	fem_type.elna_2g = rf->fem.elna_2g;
	fem_type.elna_5g = rf->fem.elna_5g;
	fem_type.elna_6g = rf->fem.elna_6g;
	fem_type.epa_2g = rf->fem.epa_2g;
	fem_type.epa_5g = rf->fem.epa_5g;
	fem_type.epa_6g = rf->fem.epa_6g;

	return fem_type;
}

bool halrf_get_dpk_by_rate(void *rf_void,
	enum phl_phy_idx phy, enum packet_format_t vector_index, u32 rate_index)
{
	struct rf_info *rf = (struct rf_info *)rf_void;
	u32 rate_addr = 0, rate_mask = 0, vector_value;
	u32 dpd_off_over, dpd_off_below, dpd_off_over_mask, dpd_off_below_mask;

	RF_DBG(rf, DBG_RF_POWER, " ======>%s vector_index=%d rate_index=%d, phy=%d\n",
		__func__, vector_index, rate_index, phy);

	switch (vector_index) {
		case B_MODE_FMT:
			rate_addr = 0xd22c;
			rate_mask = 0x001c0000;
			dpd_off_over_mask = BIT(21);
			dpd_off_below_mask = BIT(22);
			break;
		case LEGACY_FMT:
			rate_addr = 0xd22c;
			rate_mask = 0x07800000;
			dpd_off_over_mask = BIT(27);
			dpd_off_below_mask = BIT(28);
			break;
		case HT_MF_FMT:
		case HT_GF_FMT:
			rate_addr = 0xd230;
			rate_mask = 0x0000000f;
			dpd_off_over_mask = BIT(4);
			dpd_off_below_mask = BIT(5);
			break;
		case VHT_FMT:
			rate_addr = 0xd230;
			rate_mask = 0x000003c0;
			dpd_off_over_mask = BIT(10);
			dpd_off_below_mask = BIT(11);
			break;
		case HE_SU_FMT:
		case HE_ER_SU_FMT:
		case HE_MU_FMT:
		case HE_TB_FMT:
			rate_addr = 0xd230;
			rate_mask = 0x0000f000;
			dpd_off_over_mask = BIT(16);
			dpd_off_below_mask = BIT(17);
			break;
		default:
			break;
	}

	vector_value = halrf_mac_get_pwr_reg(rf, phy, rate_addr, rate_mask);
	dpd_off_over = halrf_mac_get_pwr_reg(rf, phy, rate_addr, dpd_off_over_mask);
	dpd_off_below = halrf_mac_get_pwr_reg(rf, phy, rate_addr, dpd_off_below_mask);

	RF_DBG(rf, DBG_RF_POWER, " ======>%s DPK Rate TH 0x%x[0x%x] = 0x%x   dpd_off_below=%d   dpd_off_below=%d\n",
		__func__, rate_addr, rate_mask, vector_value, dpd_off_over, dpd_off_below);

	if(rate_index < vector_value) {
		RF_DBG(rf, DBG_RF_POWER, " ======>%s Rate_index < TH  dpd_off_below=%d\n",
			__func__, dpd_off_below);

		return (bool) dpd_off_below;
	} else {
		RF_DBG(rf, DBG_RF_POWER, " ======>%s Rate_index >= TH  dpd_off_over=%d\n",
			__func__, dpd_off_below);

		return (bool) dpd_off_over;
	}
}

void halrf_set_dpk_by_rate(void *rf_void,
	enum phl_phy_idx phy, enum packet_format_t vector_index, u32 rate_index)
{
	struct rf_info *rf = (struct rf_info *)rf_void;

	RF_DBG(rf, DBG_RF_POWER, " ======>%s vector_index=%d rate_index=%d, phy=%d\n",
		__func__, vector_index, rate_index, phy);
	
	halrf_get_dpk_by_rate(rf, phy, vector_index, rate_index);

	halrf_wreg(rf, 0x45b8, BIT(16), halrf_get_dpk_by_rate(rf, phy, vector_index, rate_index));

	RF_DBG(rf, DBG_RF_POWER, " ======>%s Set 0x45b8[16]=0x%x\n",
		__func__, halrf_rreg(rf, 0x45b8, BIT(16)));
}


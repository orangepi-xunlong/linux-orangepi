/******************************************************************************
 *
 * Copyright(c) 2007 - 2019 Realtek Corporation.
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

#include <drv_types.h>
#ifdef CONFIG_RTW_80211K
#include "rtw_rm_fsm.h"
#include "rtw_rm_util.h"

u8 rm_get_ch_set(u8 op_class, struct rtw_ieee80211_channel *pch_set, u8 pch_num)
{
	int i, array_idx;
	const struct op_class_t *opc = get_global_op_class_by_id(op_class);

	if (opc < global_op_class
		|| (((u8 *)opc) - ((u8 *)global_op_class)) % sizeof(struct op_class_t)
	) {
		RTW_ERR("Invalid opc pointer:%p (global_op_class:%p, sizeof(struct op_class_t):%zu, %zu)\n"
			, opc, global_op_class, sizeof(struct op_class_t),
			(((u8 *)opc) - ((u8 *)global_op_class)) % sizeof(struct op_class_t));
		return 0;
	}

	array_idx = (((u8 *)opc) - ((u8 *)global_op_class)) / sizeof(struct op_class_t);
	if (pch_num < OPC_CH_LIST_LEN(global_op_class[array_idx])) {
		RTW_ERR("Invalid pch len %d < %d\n",pch_num,
			OPC_CH_LIST_LEN(global_op_class[array_idx]));
		return 0;
	}

	for (i = 0; i < OPC_CH_LIST_LEN(global_op_class[array_idx]); i++) {
		pch_set[i].hw_value = OPC_CH_LIST_CH(global_op_class[array_idx], i);
		pch_set[i].band = global_op_class[array_idx].band;
	}

	return i;
}


u8 rm_get_ch_set_from_bcn_req_opt(struct bcn_req_opt *opt,
	struct rtw_ieee80211_channel *pch_set, u8 pch_num)
{
	int i,j,k,sz;
	struct _RT_OPERATING_CLASS *ap_ch_rpt;
	enum band_type band;
	u8 ch_amount = 0;

	k = 0;
	for (i = 0; i < opt->ap_ch_rpt_num; i++) {
		if (opt->ap_ch_rpt[i] == NULL)
			break;

		ap_ch_rpt = opt->ap_ch_rpt[i];
		band = rtw_get_band_by_op_class(ap_ch_rpt->global_op_class);

		if ((k + ap_ch_rpt->Len) > pch_num) {
			RTW_ERR("RM: ch num exceed %d > %d\n", (k + ap_ch_rpt->Len), pch_num);
			return k;
		}

		for (j = 0; j < ap_ch_rpt->Len; j++) {
			pch_set[k].hw_value =
				ap_ch_rpt->Channel[j];
			pch_set[k].band = band;
			RTW_INFO("RM: meas_ch[%d].hw_value = %u\n",
				j, pch_set[k].hw_value);
			k++;
		}
	}
	return k;
}

int is_wildcard_bssid(u8 *bssid)
{
	int i;
	u8 val8 = 0xff;


	for (i=0;i<6;i++)
		val8 &= bssid[i];

	if (val8 == 0xff)
		return _SUCCESS;
	return _FALSE;
}

u8 translate_dbm_to_rcpi(s8 SignalPower)
{
	/* RCPI = Int{(Power in dBm + 110)*2} for 0dBm > Power > -110dBm
	 *    0	: power <= -110.0 dBm
	 *    1	: power =  -109.5 dBm
	 *    2	: power =  -109.0 dBm
	 */
	return (SignalPower + 110)*2;
}

u8 translate_percentage_to_rcpi(u32 SignalStrengthIndex)
{
	/* Translate to dBm (x=y-100) */
	return translate_dbm_to_rcpi(SignalStrengthIndex - 100);
}

u8 rm_get_bcn_rcpi(struct rm_obj *prm, struct wlan_network *pnetwork)
{
	return translate_percentage_to_rcpi(
		pnetwork->network.PhyInfo.SignalStrength);
}

u8 rm_get_frame_rsni(struct rm_obj *prm, union recv_frame *pframe)
{
	int i;
	u8 val8 = 0, snr;
	struct dvobj_priv *dvobj = adapter_to_dvobj(prm->psta->padapter);
	u8 rf_path = GET_HAL_RFPATH_NUM(dvobj);
#if 0
	if (IS_CCK_RATE((hw_rate_to_m_rate(pframe->u.hdr.attrib.data_rate))))
		val8 = 255;
	else {
		snr = 0;
		for (i = 0; i < rf_path; i++)
			snr += pframe->u.hdr.attrib.phy_info.rx_snr[i];
		snr = snr / rf_path;
		val8 = (u8)(snr + 10)*2;
	}
#endif
	return val8;
}

u8 rm_get_bcn_rsni(struct rm_obj *prm, struct wlan_network *pnetwork)
{
	int i;
	u8 val8, snr;
	struct dvobj_priv *dvobj = adapter_to_dvobj(prm->psta->padapter);
	u8 rf_path = GET_HAL_RFPATH_NUM(dvobj);


	if (pnetwork->network.PhyInfo.is_cck_rate) {
		/* current HW doesn't have CCK RSNI */
		/* 255 indicates RSNI is unavailable */
		val8 = 255;
	} else {
		snr = 0;
		for (i = 0; i < rf_path; i++) {
			snr += pnetwork->network.PhyInfo.rx_snr[i];
		}
		snr = snr / rf_path;
		val8 = (u8)(snr + 10)*2;
	}
	return val8;
}

/* output: pwr (unit dBm) */
int rm_get_tx_power(_adapter *adapter, enum band_type band, enum MGN_RATE rate, s8 *pwr)
{
	struct dvobj_priv *devob = adapter_to_dvobj(adapter);
	u8 rs = mgn_rate_to_rs(rate);

	*pwr = rtw_phl_get_power_by_rate_band(GET_PHL_INFO(devob), HW_BAND_0, _rate_mrate2phl(rate),
					       IS_DCM_RATE_SECTION(rs) ? 1 : 0, 0, band);

	return 0;
}

u8 rm_gen_dialog_token(_adapter *padapter)
{
	struct rm_priv *prmpriv = &(padapter->rmpriv);
	struct mlme_ext_priv *pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info *pmlmeinfo = &(pmlmeext->mlmext_info);

	do {
		pmlmeinfo->dialogToken++;
	} while (pmlmeinfo->dialogToken == 0);

	return pmlmeinfo->dialogToken;
}

u8 rm_gen_meas_token(_adapter *padapter)
{
	struct rm_priv *prmpriv = &(padapter->rmpriv);

	do {
		prmpriv->meas_token++;
	} while (prmpriv->meas_token == 0);

	return prmpriv->meas_token;
}

u32 rm_gen_rmid(_adapter *padapter, struct rm_obj *prm, u8 role)
{
	u32 rmid;

	if (prm->psta == NULL)
		goto err;

	if (prm->q.diag_token == 0)
		goto err;

	rmid = prm->psta->phl_sta->aid << 16
		| prm->q.diag_token << 8
		| role;

	return rmid;
err:
	RTW_ERR("RM: unable to gen rmid\n");
	return 0;
}

int rm_get_rx_sensitivity(_adapter *adapter, enum channel_width bw, enum MGN_RATE rate, s8 *pwr)
{
	s8 rx_sensitivity = -110;

	switch(rate) {
	case MGN_1M:
		rx_sensitivity= -101;
		break;
	case MGN_2M:
		rx_sensitivity= -98;
		break;
	case MGN_5_5M:
		rx_sensitivity= -92;
		break;
	case MGN_11M:
		rx_sensitivity= -89;
		break;
	case MGN_6M:
	case MGN_9M:
	case MGN_12M:
		rx_sensitivity = -92;
		break;
	case MGN_18M:
		rx_sensitivity = -90;
		break;
	case MGN_24M:
		rx_sensitivity = -88;
		break;
	case MGN_36M:
		rx_sensitivity = -84;
		break;
	case MGN_48M:
		rx_sensitivity = -79;
		break;
	case MGN_54M:
		rx_sensitivity = -78;
		break;

	case MGN_MCS0:
	case MGN_MCS8:
	case MGN_MCS16:
	case MGN_MCS24:
	case MGN_VHT1SS_MCS0:
	case MGN_VHT2SS_MCS0:
	case MGN_VHT3SS_MCS0:
	case MGN_VHT4SS_MCS0:
		/* BW20 BPSK 1/2 */
		rx_sensitivity = -82;
		break;

	case MGN_MCS1:
	case MGN_MCS9:
	case MGN_MCS17:
	case MGN_MCS25:
	case MGN_VHT1SS_MCS1:
	case MGN_VHT2SS_MCS1:
	case MGN_VHT3SS_MCS1:
	case MGN_VHT4SS_MCS1:
		/* BW20 QPSK 1/2 */
		rx_sensitivity = -79;
		break;

	case MGN_MCS2:
	case MGN_MCS10:
	case MGN_MCS18:
	case MGN_MCS26:
	case MGN_VHT1SS_MCS2:
	case MGN_VHT2SS_MCS2:
	case MGN_VHT3SS_MCS2:
	case MGN_VHT4SS_MCS2:
		/* BW20 QPSK 3/4 */
		rx_sensitivity = -77;
		break;

	case MGN_MCS3:
	case MGN_MCS11:
	case MGN_MCS19:
	case MGN_MCS27:
	case MGN_VHT1SS_MCS3:
	case MGN_VHT2SS_MCS3:
	case MGN_VHT3SS_MCS3:
	case MGN_VHT4SS_MCS3:
		/* BW20 16-QAM 1/2 */
		rx_sensitivity = -74;
		break;

	case MGN_MCS4:
	case MGN_MCS12:
	case MGN_MCS20:
	case MGN_MCS28:
	case MGN_VHT1SS_MCS4:
	case MGN_VHT2SS_MCS4:
	case MGN_VHT3SS_MCS4:
	case MGN_VHT4SS_MCS4:
		/* BW20 16-QAM 3/4 */
		rx_sensitivity = -70;
		break;

	case MGN_MCS5:
	case MGN_MCS13:
	case MGN_MCS21:
	case MGN_MCS29:
	case MGN_VHT1SS_MCS5:
	case MGN_VHT2SS_MCS5:
	case MGN_VHT3SS_MCS5:
	case MGN_VHT4SS_MCS5:
		/* BW20 64-QAM 2/3 */
		rx_sensitivity = -66;
		break;

	case MGN_MCS6:
	case MGN_MCS14:
	case MGN_MCS22:
	case MGN_MCS30:
	case MGN_VHT1SS_MCS6:
	case MGN_VHT2SS_MCS6:
	case MGN_VHT3SS_MCS6:
	case MGN_VHT4SS_MCS6:
		/* BW20 64-QAM 3/4 */
		rx_sensitivity = -65;
		break;

	case MGN_MCS7:
	case MGN_MCS15:
	case MGN_MCS23:
	case MGN_MCS31:
	case MGN_VHT1SS_MCS7:
	case MGN_VHT2SS_MCS7:
	case MGN_VHT3SS_MCS7:
	case MGN_VHT4SS_MCS7:
		/* BW20 64-QAM 5/6 */
		rx_sensitivity = -64;
		break;

	case MGN_VHT1SS_MCS8:
	case MGN_VHT2SS_MCS8:
	case MGN_VHT3SS_MCS8:
	case MGN_VHT4SS_MCS8:
		/* BW20 256-QAM 3/4 */
		rx_sensitivity = -59;
		break;

	case MGN_VHT1SS_MCS9:
	case MGN_VHT2SS_MCS9:
	case MGN_VHT3SS_MCS9:
	case MGN_VHT4SS_MCS9:
		/* BW20 256-QAM 5/6 */
		rx_sensitivity = -57;
		break;

	default:
		return -1;
		break;

	}

	switch(bw) {
	case CHANNEL_WIDTH_20:
		break;
	case CHANNEL_WIDTH_40:
		rx_sensitivity -= 3;
		break;
	case CHANNEL_WIDTH_80:
		rx_sensitivity -= 6;
		break;
	case CHANNEL_WIDTH_160:
		rx_sensitivity -= 9;
		break;
	case CHANNEL_WIDTH_5:
	case CHANNEL_WIDTH_10:
	case CHANNEL_WIDTH_80_80:
	default:
		return -1;
		break;
	}
	*pwr = rx_sensitivity;

	return 0;
}

/* output: path_a max tx power in dBm */
int rm_get_path_a_max_tx_power(_adapter *adapter, s8 *path_a)
{
#if 0 /*GEORGIA_TODO_FIXIT*/

	struct dvobj_priv *dvobj = adapter_to_dvobj(adapter);
	struct hal_spec_t *hal_spec = GET_HAL_SPEC(dvobj);
	HAL_DATA_TYPE *hal_data = GET_PHL_COM(dvobj);
	int path, tx_num, band, bw, ch, n, rs;
	u8 rate_num;
	s8 max_pwr[RF_PATH_MAX], pwr;


	band = hal_data->current_band_type;
	bw = hal_data->current_channel_bw;
	ch = hal_data->current_channel;

	for (path = 0; path < RF_PATH_MAX; path++) {
		if (!HAL_SPEC_CHK_RF_PATH(hal_spec, band, path))
			break;

		max_pwr[path] = -127; /* min value of s8 */
#if (RM_MORE_DBG_MSG)
		RTW_INFO("RM: [%s][%c]\n", band_str(band), rf_path_char(path));
#endif
		for (rs = 0; rs < RATE_SECTION_NUM; rs++) {
			tx_num = rate_section_to_tx_num(rs);

			if (tx_num >= hal_spec->tx_nss_num)
				continue;

			if (band == BAND_ON_5G && IS_CCK_RATE_SECTION(rs))
				continue;

			if (IS_VHT_RATE_SECTION(rs) && !IS_HARDWARE_TYPE_JAGUAR_ALL(adapter))
				continue;

			rate_num = rate_section_rate_num(rs);

			/* get power by rate in db */
			for (n = rate_num - 1; n >= 0; n--) {
				pwr = phy_get_tx_power_final_absolute_value(adapter, path, rates_by_sections[rs].rates[n], bw, ch);
				max_pwr[path] = MAX(max_pwr[path], pwr);
#if (RM_MORE_DBG_MSG)
				RTW_INFO("RM: %9s = %2d\n",
					MGN_RATE_STR(rates_by_sections[rs].rates[n]), pwr);
#endif
			}
		}
	}
#if (RM_MORE_DBG_MSG)
	RTW_INFO("RM: path_a max_pwr=%ddBm\n", max_pwr[0]);
#endif
	*path_a = max_pwr[0];
#endif
	return 0;
}

#endif /* CONFIG_RTW_80211K */

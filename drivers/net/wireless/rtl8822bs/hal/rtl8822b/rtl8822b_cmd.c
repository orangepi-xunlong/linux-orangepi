/******************************************************************************
 *
 * Copyright(c) 2015 - 2017 Realtek Corporation.
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
#define _RTL8822B_CMD_C_

#include <hal_data.h>		/* HAL_DATA_TYPE */
#include "../hal_halmac.h"	/* HRTW_HALMAC_H2C_MAX_SIZE, CMD_ID_RSVD_PAGE and etc. */
#include "rtl8822b.h"

/*
 * Below functions are for C2H
 */
/*****************************************
 * H2C Msg format :
 *| 31 - 8		|7-5	| 4 - 0	|
 *| h2c_msg		|Class	|CMD_ID	|
 *| 31-0				|
 *| Ext msg				|
 *
 ******************************************/
s32 rtl8822b_fillh2ccmd(PADAPTER adapter, u8 id, u32 buf_len, u8 *pbuf)
{
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};
#ifdef CONFIG_RTW_DEBUG
	u8 msg[(RTW_HALMAC_H2C_MAX_SIZE - 1) * 5 + 1] = {0};
	u8 *msg_p;
	u32 msg_size, i, n;
#endif /* CONFIG_RTW_DEBUG */
	int err;
	s32 ret = _FAIL;


	if (!pbuf)
		goto exit;

	if (buf_len > (RTW_HALMAC_H2C_MAX_SIZE - 1))
		goto exit;

	if (rtw_is_surprise_removed(adapter))
		goto exit;

#ifdef CONFIG_RTW_DEBUG
	msg_p = msg;
	msg_size = (RTW_HALMAC_H2C_MAX_SIZE - 1) * 5 + 1;
	for (i = 0; i < buf_len; i++) {
		n = rtw_sprintf(msg_p, msg_size, " 0x%02x", pbuf[i]);
		msg_p += n;
		msg_size -= n;
		if (msg_size == 0)
			break;
	}
	RTW_DBG(FUNC_ADPT_FMT ": id=0x%02x buf=%s\n",
		 FUNC_ADPT_ARG(adapter), id, msg);
#endif /* CONFIG_RTW_DEBUG */

	h2c[0] = id;
	_rtw_memcpy(h2c + 1, pbuf, buf_len);

	err = rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
	if (!err)
		ret = _SUCCESS;

exit:

	return ret;
}

static void rtl8822b_set_FwRsvdPage_cmd(PADAPTER adapter, PRSVDPAGE_LOC rsvdpageloc)
{
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};


	RTW_INFO(FUNC_ADPT_FMT ": ProbeRsp=%d PsPoll=%d Null=%d QoSNull=%d BTNull=%d\n",
		 FUNC_ADPT_ARG(adapter),
		 rsvdpageloc->LocProbeRsp, rsvdpageloc->LocPsPoll,
		 rsvdpageloc->LocNullData, rsvdpageloc->LocQosNull,
		 rsvdpageloc->LocBTQosNull);

	RSVD_PAGE_SET_CMD_ID(h2c, CMD_ID_RSVD_PAGE);
	RSVD_PAGE_SET_CLASS(h2c, CLASS_RSVD_PAGE);
	RSVD_PAGE_SET_LOC_PROBE_RSP(h2c, rsvdpageloc->LocProbeRsp);
	RSVD_PAGE_SET_LOC_PS_POLL(h2c, rsvdpageloc->LocPsPoll);
	RSVD_PAGE_SET_LOC_NULL_DATA(h2c, rsvdpageloc->LocNullData);
	RSVD_PAGE_SET_LOC_QOS_NULL(h2c, rsvdpageloc->LocQosNull);
	RSVD_PAGE_SET_LOC_BT_QOS_NULL(h2c, rsvdpageloc->LocBTQosNull);

	RTW_DBG_DUMP("H2C-RsvdPage Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
}

static void rtl8822b_set_FwAoacRsvdPage_cmd(PADAPTER adapter, PRSVDPAGE_LOC rsvdpageloc)
{
#ifdef CONFIG_WOWLAN
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(adapter);
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
	u8 res = 0, count = 0;
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};


	RTW_INFO(FUNC_ADPT_FMT ": RWC=%d ArpRsp=%d NbrAdv=%d GtkRsp=%d GtkInfo=%d ProbeReq=%d NetworkList=%d\n",
		 FUNC_ADPT_ARG(adapter),
		 rsvdpageloc->LocRemoteCtrlInfo, rsvdpageloc->LocArpRsp,
		 rsvdpageloc->LocNbrAdv, rsvdpageloc->LocGTKRsp,
		 rsvdpageloc->LocGTKInfo, rsvdpageloc->LocProbeReq,
		 rsvdpageloc->LocNetList);

	if (check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE) {
		AOAC_RSVD_PAGE_SET_CMD_ID(h2c, CMD_ID_AOAC_RSVD_PAGE);
		AOAC_RSVD_PAGE_SET_CLASS(h2c, CLASS_AOAC_RSVD_PAGE);
		AOAC_RSVD_PAGE_SET_LOC_REMOTE_CTRL_INFO(h2c, rsvdpageloc->LocRemoteCtrlInfo);
		AOAC_RSVD_PAGE_SET_LOC_ARP_RESPONSE(h2c, rsvdpageloc->LocArpRsp);
		AOAC_RSVD_PAGE_SET_LOC_GTK_RSP(h2c, rsvdpageloc->LocGTKRsp);
		AOAC_RSVD_PAGE_SET_LOC_GTK_INFO(h2c, rsvdpageloc->LocGTKInfo);
#ifdef CONFIG_GTK_OL
		AOAC_RSVD_PAGE_SET_LOC_GTK_EXT_MEM(h2c, rsvdpageloc->LocGTKEXTMEM);
#endif /* CONFIG_GTK_OL */
		RTW_DBG_DUMP("H2C-AoacRsvdPage Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
		rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
	} else {
#ifdef CONFIG_PNO_SUPPORT
		if (!pwrpriv->wowlan_in_resume) {
			RTW_INFO("%s: NLO_INFO=%d\n", __FUNCTION__, rsvdpageloc->LocPNOInfo);
			AOAC_RSVD_PAGE3_SET_CMD_ID(h2c, CMD_ID_AOAC_RSVD_PAGE3);
			AOAC_RSVD_PAGE3_SET_CLASS(h2c, CLASS_AOAC_RSVD_PAGE3);
			AOAC_RSVD_PAGE3_SET_LOC_NLO_INFO(h2c, rsvdpageloc->LocPNOInfo);
			RTW_DBG_DUMP("H2C-AoacRsvdPage3 Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
			rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
			rtw_msleep_os(10);
		}
#endif /* CONFIG_PNO_SUPPORT */
	}
#endif /* CONFIG_WOWLAN */
}

void rtl8822b_set_FwMediaStatusRpt_cmd(PADAPTER	adapter, u8 mstatus, u8 macid)
{
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};
	u8 macid_end = 0;

	RTW_INFO(FUNC_ADPT_FMT ": mstatus=%d macid=%d\n",
		 FUNC_ADPT_ARG(adapter), mstatus, macid);

	MEDIA_STATUS_RPT_SET_CMD_ID(h2c, CMD_ID_MEDIA_STATUS_RPT);
	MEDIA_STATUS_RPT_SET_CLASS(h2c, CLASS_MEDIA_STATUS_RPT);
	MEDIA_STATUS_RPT_SET_OP_MODE(h2c, mstatus);
	MEDIA_STATUS_RPT_SET_MACID_IN(h2c, 0);
	MEDIA_STATUS_RPT_SET_MACID(h2c, macid);
	MEDIA_STATUS_RPT_SET_MACID_END(h2c, macid_end);

	RTW_DBG_DUMP("H2C-MediaStatusRpt Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
}

static void rtl8822b_set_FwKeepAlive_cmd(PADAPTER adapter, u8 benable, u8 pkt_type)
{
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};
	u8 adopt = 1;
#ifdef CONFIG_PLATFORM_INTEL_BYT
	u8 check_period = 10;
#else
	u8 check_period = 5;
#endif


	RTW_INFO(FUNC_ADPT_FMT ": benable=%d\n", FUNC_ADPT_ARG(adapter), benable);

	KEEP_ALIVE_SET_CMD_ID(h2c, CMD_ID_KEEP_ALIVE);
	KEEP_ALIVE_SET_CLASS(h2c, CLASS_KEEP_ALIVE);
	KEEP_ALIVE_SET_ENABLE(h2c, benable);
	KEEP_ALIVE_SET_ADOPT_USER_SETTING(h2c, adopt);
	KEEP_ALIVE_SET_PKT_TYPE(h2c, pkt_type);
	KEEP_ALIVE_SET_KEEP_ALIVE_CHECK_PERIOD(h2c, check_period);

	RTW_DBG_DUMP("H2C-KeepAlive Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
}

static void rtl8822b_set_FwDisconDecision_cmd(PADAPTER adapter, u8 benable)
{
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};
	u8 adopt = 1, check_period = 10, trypkt_num = 0;


	RTW_INFO(FUNC_ADPT_FMT ": benable=%d\n",
		 FUNC_ADPT_ARG(adapter), benable);

	DISCONNECT_DECISION_SET_CMD_ID(h2c, CMD_ID_DISCONNECT_DECISION);
	DISCONNECT_DECISION_SET_CLASS(h2c, CLASS_DISCONNECT_DECISION);
	DISCONNECT_DECISION_SET_ENABLE(h2c, benable);
	DISCONNECT_DECISION_SET_ADOPT_USER_SETTING(h2c, adopt);
	DISCONNECT_DECISION_SET_DISCON_DECISION_CHECK_PERIOD(h2c, check_period);
	DISCONNECT_DECISION_SET_TRY_PKT_NUM(h2c, trypkt_num);

	RTW_DBG_DUMP("H2C-DisconDecision Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
}

static u8 get_ra_vht_en(u32 wirelessMode, u32 bitmap)
{
	u8 ret = 0;

	if (wirelessMode == WIRELESS_11_24AC) {
		if (bitmap & 0xfff00000) /* 2SS */
			ret = 3;
		else					/* 1SS */
			ret = 2;
	} else if (wirelessMode == WIRELESS_11_5AC)
		ret = 1;

	return ret;
}

void rtl8822b_set_FwRssiSetting_cmd(PADAPTER adapter, u8 *param)
{
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};
	u8 mac_id = *param;
	u8 rssi = *(param + 2);
	u8 ra_info = 0;


	RTW_INFO(FUNC_ADPT_FMT ": mac_id=%d rssi=%d param=%.2x-%.2x-%.2x\n",
		 FUNC_ADPT_ARG(adapter),
		 mac_id, rssi, *param, *(param + 1), *(param + 2));

	RSSI_SETTING_SET_CMD_ID(h2c, CMD_ID_RSSI_SETTING);
	RSSI_SETTING_SET_CLASS(h2c, CLASS_RSSI_SETTING);
	RSSI_SETTING_SET_MAC_ID(h2c, mac_id);
	RSSI_SETTING_SET_RSSI(h2c, rssi);
	RSSI_SETTING_SET_RA_INFO(h2c, ra_info);

	RTW_DBG_DUMP("H2C-RssiSetting Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
}

void rtl8822b_set_FwAPReqRPT_cmd(PADAPTER adapter, u32 need_ack)
{
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};
	u8 macid1 = 1, macid2 = 0;


	RTW_INFO(FUNC_ADPT_FMT ": need_ack = %d\n",
		 FUNC_ADPT_ARG(adapter), need_ack);

	AP_REQ_TXRPT_SET_CMD_ID(h2c, CMD_ID_AP_REQ_TXRPT);
	AP_REQ_TXRPT_SET_CLASS(h2c, CLASS_AP_REQ_TXRPT);
	AP_REQ_TXRPT_SET_STA1_MACID(h2c, macid1);
	AP_REQ_TXRPT_SET_STA2_MACID(h2c, macid2);

	RTW_DBG_DUMP("H2C-ApReqRpt Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
}

void rtl8822b_req_txrpt_cmd(PADAPTER adapter, u8 macid)
{
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};

	AP_REQ_TXRPT_SET_CMD_ID(h2c, CMD_ID_AP_REQ_TXRPT);
	AP_REQ_TXRPT_SET_CLASS(h2c, CLASS_AP_REQ_TXRPT);

	AP_REQ_TXRPT_SET_STA1_MACID(h2c, macid);
	AP_REQ_TXRPT_SET_STA2_MACID(h2c, 0xff);
	AP_REQ_TXRPT_SET_RTY_OK_TOTAL(h2c, 0x00);
	AP_REQ_TXRPT_SET_RTY_CNT_MACID(h2c, 0x00);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);

	AP_REQ_TXRPT_SET_RTY_CNT_MACID(h2c, 0x01);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
}

/*
 * lps_wait_bb_rf_ready() - Wait BB/RF ready after leaving LPS
 * @adapter	struct _ADAPTER*
 * @timeout	time to wait complete, unit is millisecond
 *
 * This function is used to wait BB and RF ready after leaving LPS. Besdies
 * checking registers, it will wait 1 ms to let everything has time to finish
 * their jobs, so this function will cost more than 1ms to return. Please call
 * this function carefully, or you will waste time to wait.
 *
 * Return 0 for BB/RF ready, otherwise NOT ready.
 * The error codes are as following:
 * -1	unclassified error
 * -2	RF ready check timeout
 * -3	BB ready check timeout
 */
static int lps_wait_bb_rf_ready(struct _ADAPTER *adapter, u32 timeout)
{
	systime s_time;	/* start time */
	u8 ready = 0;
#define RF_READY	BIT(0)	/* BB ready */
#define BB_READY	BIT(1)	/* RF ready */
	u8 awake = _FALSE;
	u8 sys_func_en;


	s_time = rtw_get_current_time();

	do {
		if (!(ready & RF_READY)) {
			rtw_hal_get_hwreg(adapter, HW_VAR_FWLPS_RF_ON, &awake);
			if (awake == _TRUE)
				ready |= RF_READY;
		}

		if ((ready & RF_READY) && (!(ready & BB_READY))) {
			sys_func_en = rtw_read8(adapter, REG_SYS_FUNC_EN_8822B);
			if (sys_func_en & BIT_FEN_BBRSTB_8822B)
				break;
		}

		if (rtw_is_surprise_removed(adapter))
			return -1;

		if (rtw_get_passing_time_ms(s_time) > timeout) {
			if (!(ready & RF_READY))
				return -2;
			return -3;
		}

		rtw_usleep_os(100); /* 100us interval between each check */
	} while (1);

	rtw_usleep_os(1000); /* Wait 1ms */

	return 0;
}

void rtl8822b_set_FwPwrMode_cmd(PADAPTER adapter, u8 psmode)
{
	int i;
	u8 smart_ps = 0, mode = 0;
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(adapter);
	struct mlme_ext_priv *pmlmeext = &adapter->mlmeextpriv;
#ifdef CONFIG_WMMPS_STA
	struct mlme_priv	*pmlmepriv = &(adapter->mlmepriv);
	struct qos_priv	*pqospriv = &pmlmepriv->qospriv;
#endif /* CONFIG_WMMPS_STA */	
	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};
	u8 PowerState = 0, awake_intvl = 1, byte5 = 0, rlbm = 0;
	u8 allQueueUAPSD = 0;
	char *fw_psmode_str = "";
#ifdef CONFIG_P2P
	struct wifidirect_info *wdinfo = &adapter->wdinfo;
#endif /* CONFIG_P2P */


	if (pwrpriv->dtim > 0)
		RTW_INFO(FUNC_ADPT_FMT ": dtim=%d, HW port id=%d\n", FUNC_ADPT_ARG(adapter),
			pwrpriv->dtim, psmode == PS_MODE_ACTIVE ? pwrpriv->current_lps_hw_port_id:get_hw_port(adapter));
	else
		RTW_INFO(FUNC_ADPT_FMT ": HW port id=%d\n", FUNC_ADPT_ARG(adapter),
			psmode == PS_MODE_ACTIVE ? pwrpriv->current_lps_hw_port_id:get_hw_port(adapter));

	if (psmode == PS_MODE_MIN || psmode == PS_MODE_MAX) {
#ifdef CONFIG_WMMPS_STA	
		if (rtw_is_wmmps_mode(adapter)) {
			mode = 2;

			smart_ps = pwrpriv->wmm_smart_ps;

			/* (WMMPS) allQueueUAPSD: 0: PSPoll, 1: QosNullData (if wmm_smart_ps=1) or do nothing (if wmm_smart_ps=2) */
			if ((pqospriv->uapsd_tid & BIT_MASK_TID_TC) == ALL_TID_TC_SUPPORTED_UAPSD)
				allQueueUAPSD = 1;
		} else
#endif /* CONFIG_WMMPS_STA */
		{
			mode = 1;
#ifdef CONFIG_WMMPS_STA	
			/* For WMMPS test case, the station must retain sleep mode to capture buffered data on LPS mechanism */ 
			if ((pqospriv->uapsd_tid & BIT_MASK_TID_TC)  != 0)
				smart_ps = 0;
			else
#endif /* CONFIG_WMMPS_STA */
			{
				smart_ps = pwrpriv->smart_ps;
			}
		}

		if (psmode == PS_MODE_MIN)
			rlbm = 0;
		else
			rlbm = 1;
	} else if (psmode == PS_MODE_DTIM) {
		mode = 1;
		/* For WOWLAN LPS, DTIM = (awake_intvl - 1) */
		if (pwrpriv->dtim > 0 && pwrpriv->dtim < 16)
			/* DTIM = (awake_intvl - 1) */
			awake_intvl = pwrpriv->dtim + 1;
		else
			/* DTIM = 3 */
			awake_intvl = 4;

		rlbm = 2;
		smart_ps = pwrpriv->smart_ps;
	} else if (psmode == PS_MODE_ACTIVE) {
		mode = 0;
	} else {
		rlbm = 2;
		awake_intvl = 4;
		smart_ps = pwrpriv->smart_ps;
	}

#ifdef CONFIG_P2P
	if (!rtw_p2p_chk_state(wdinfo, P2P_STATE_NONE)) {
		awake_intvl = 2;
		rlbm = 1;
	}
#endif /* CONFIG_P2P */

	if (adapter->registrypriv.wifi_spec == 1) {
		awake_intvl = 2;
		rlbm = 1;
	}

	if (psmode > 0) {
#ifdef CONFIG_BT_COEXIST
		if (rtw_btcoex_IsBtControlLps(adapter) == _TRUE) {
			PowerState = rtw_btcoex_RpwmVal(adapter);
			byte5 = rtw_btcoex_LpsVal(adapter);

			if ((rlbm == 2) && (byte5 & BIT(4))) {
				/*
				 * Keep awake interval to 1 to prevent from
				 * decreasing coex performance
				 */
				awake_intvl = 2;
				rlbm = 2;
			}
		} else
#endif /* CONFIG_BT_COEXIST */
		{
			PowerState = 0x00; /* AllON(0x0C), RFON(0x04), RFOFF(0x00) */
			byte5 = 0x40;
		}
	} else {
		PowerState = 0x0C; /* AllON(0x0C), RFON(0x04), RFOFF(0x00) */
		byte5 = 0x40;
	}

	if (mode == 0)
		fw_psmode_str = "ACTIVE";
	else if (mode == 1)
		fw_psmode_str = "LPS";
	else if (mode == 2)
		fw_psmode_str = "WMMPS";
	else
		fw_psmode_str = "UNSPECIFIED";

	RTW_INFO(FUNC_ADPT_FMT": fw ps mode = %s, drv ps mode = %d, rlbm = %d , smart_ps = %d, allQueueUAPSD = %d\n", 
				FUNC_ADPT_ARG(adapter), fw_psmode_str, psmode, rlbm, smart_ps, allQueueUAPSD);

	SET_PWR_MODE_SET_CMD_ID(h2c, CMD_ID_SET_PWR_MODE);
	SET_PWR_MODE_SET_CLASS(h2c, CLASS_SET_PWR_MODE);
	SET_PWR_MODE_SET_MODE(h2c, mode);
	SET_PWR_MODE_SET_SMART_PS(h2c, smart_ps);
	SET_PWR_MODE_SET_RLBM(h2c, rlbm);
	SET_PWR_MODE_SET_AWAKE_INTERVAL(h2c, awake_intvl);
	SET_PWR_MODE_SET_B_ALL_QUEUE_UAPSD(h2c, allQueueUAPSD);
	SET_PWR_MODE_SET_PWR_STATE(h2c, PowerState);
	if (psmode == PS_MODE_ACTIVE) {
		/* Leave LPS, set the same HW port ID */
		SET_PWR_MODE_SET_PORT_ID(h2c, pwrpriv->current_lps_hw_port_id);
	} else {
		/* Enter LPS, record HW port ID */
		SET_PWR_MODE_SET_PORT_ID(h2c, get_hw_port(adapter));
		pwrpriv->current_lps_hw_port_id = get_hw_port(adapter);
	}

	if (byte5 & BIT(0))
		SET_PWR_MODE_SET_LOW_POWER_RX_BCN(h2c, 1);
	if (byte5 & BIT(1))
		SET_PWR_MODE_SET_ANT_AUTO_SWITCH(h2c, 1);
	if (byte5 & BIT(2))
		SET_PWR_MODE_SET_PS_ALLOW_BT_HIGH_PRIORITY(h2c, 1);
	if (byte5 & BIT(3))
		SET_PWR_MODE_SET_PROTECT_BCN(h2c, 1);
	if (byte5 & BIT(4))
		SET_PWR_MODE_SET_SILENCE_PERIOD(h2c, 1);
	if (byte5 & BIT(5))
		SET_PWR_MODE_SET_FAST_BT_CONNECT(h2c, 1);
	if (byte5 & BIT(6))
		SET_PWR_MODE_SET_TWO_ANTENNA_EN(h2c, 1);

#ifdef CONFIG_LPS_LCLK
	if (psmode != PS_MODE_ACTIVE) {
		if ((pmlmeext->adaptive_tsf_done == _FALSE)
		    && (pmlmeext->bcn_cnt > 0)) {
			u8 ratio_20_delay, ratio_80_delay;

			/*
			 * byte 6 for adaptive_early_32k
			 * [0:3] = DrvBcnEarly (ms), [4:7] = DrvBcnTimeOut (ms)
			 * 20% for DrvBcnEarly, 80% for DrvBcnTimeOut
			 */
			ratio_20_delay = 0;
			ratio_80_delay = 0;
			pmlmeext->DrvBcnEarly = 0xff;
			pmlmeext->DrvBcnTimeOut = 0xff;

			for (i = 0; i < 9; i++) {
				pmlmeext->bcn_delay_ratio[i] = (pmlmeext->bcn_delay_cnt[i] * 100) / pmlmeext->bcn_cnt;

				ratio_20_delay += pmlmeext->bcn_delay_ratio[i];
				ratio_80_delay += pmlmeext->bcn_delay_ratio[i];

				if (ratio_20_delay > 20 && pmlmeext->DrvBcnEarly == 0xff)
					pmlmeext->DrvBcnEarly = i;

				if (ratio_80_delay > 80 && pmlmeext->DrvBcnTimeOut == 0xff)
					pmlmeext->DrvBcnTimeOut = i;

				/* reset adaptive_early_32k cnt */
				pmlmeext->bcn_delay_cnt[i] = 0;
				pmlmeext->bcn_delay_ratio[i] = 0;
			}

			pmlmeext->bcn_cnt = 0;
			pmlmeext->adaptive_tsf_done = _TRUE;
		}
	}
#endif /* CONFIG_LPS_LCLK */

#ifdef CONFIG_BT_COEXIST
	rtw_btcoex_RecordPwrMode(adapter, h2c + 1, RTW_HALMAC_H2C_MAX_SIZE - 1);
#endif /* CONFIG_BT_COEXIST */

	RTW_DBG_DUMP("H2C-PwrMode Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);

	if (psmode == PS_MODE_ACTIVE) {
		i = lps_wait_bb_rf_ready(adapter, 1000);
		if (i)
			RTW_WARN("%s: BB/RF status is unknown!(%d)\n",
				 __FUNCTION__, i);
	}
}

void rtl8822b_set_FwPwrModeInIPS_cmd(PADAPTER adapter, u8 cmd_param)
{

	u8 h2c[RTW_HALMAC_H2C_MAX_SIZE] = {0};

	INACTIVE_PS_SET_CMD_ID(h2c, CMD_ID_INACTIVE_PS);
	INACTIVE_PS_SET_CLASS(h2c, CLASS_INACTIVE_PS);

	if (cmd_param & BIT0)
		INACTIVE_PS_SET_ENABLE(h2c, 1);

	if (cmd_param & BIT1)
		INACTIVE_PS_SET_IGNORE_PS_CONDITION(h2c, 1);

	RTW_DBG_DUMP("H2C-FwPwrModeInIPS Parm:", h2c, RTW_HALMAC_H2C_MAX_SIZE);
	rtw_halmac_send_h2c(adapter_to_dvobj(adapter), h2c);
}

static s32 rtl8822b_set_FwLowPwrLps_cmd(PADAPTER adapter, u8 enable)
{
	return _FALSE;
}

#ifdef CONFIG_BT_COEXIST
static void ConstructBeacon(PADAPTER adapter, u8 *pframe, u32 *pLength)
{
	struct rtw_ieee80211_hdr *pwlanhdr;
	u16 *fctrl;
	u32 rate_len, pktlen;
	struct mlme_ext_priv *pmlmeext = &adapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;
	WLAN_BSSID_EX *cur_network = &pmlmeinfo->network;
	u8 bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;

	_rtw_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, adapter_mac_addr(adapter), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(cur_network), ETH_ALEN);

	SetSeqNum(pwlanhdr, 0);
	set_frame_sub_type(pframe, WIFI_BEACON);

	pframe += sizeof(struct rtw_ieee80211_hdr_3addr);
	pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);

	/* timestamp will be inserted by hardware */
	pframe += 8;
	pktlen += 8;

	/* beacon interval: 2 bytes */
	_rtw_memcpy(pframe, (u8 *)rtw_get_beacon_interval_from_ie(cur_network->IEs), 2);

	pframe += 2;
	pktlen += 2;

	/* capability info: 2 bytes */
	_rtw_memcpy(pframe, (u8 *)rtw_get_capability_from_ie(cur_network->IEs), 2);

	pframe += 2;
	pktlen += 2;

	if ((pmlmeinfo->state & 0x03) == WIFI_FW_AP_STATE) {
		pktlen += cur_network->IELength - sizeof(NDIS_802_11_FIXED_IEs);
		_rtw_memcpy(pframe, cur_network->IEs + sizeof(NDIS_802_11_FIXED_IEs), pktlen);

		goto _ConstructBeacon;
	}

	/* below for ad-hoc mode */

	/* SSID */
	pframe = rtw_set_ie(pframe, _SSID_IE_, cur_network->Ssid.SsidLength, cur_network->Ssid.Ssid, &pktlen);

	/* supported rates... */
	rate_len = rtw_get_rateset_len(cur_network->SupportedRates);
	pframe = rtw_set_ie(pframe, _SUPPORTEDRATES_IE_, ((rate_len > 8) ? 8 : rate_len), cur_network->SupportedRates, &pktlen);

	/* DS parameter set */
	pframe = rtw_set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&(cur_network->Configuration.DSConfig), &pktlen);

	if ((pmlmeinfo->state & 0x03) == WIFI_FW_ADHOC_STATE) {
		u32 ATIMWindow;
		/* IBSS Parameter Set... */
		ATIMWindow = 0;
		pframe = rtw_set_ie(pframe, _IBSS_PARA_IE_, 2, (unsigned char *)(&ATIMWindow), &pktlen);
	}

	/* todo: ERP IE */

	/* EXTERNDED SUPPORTED RATE */
	if (rate_len > 8)
		pframe = rtw_set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur_network->SupportedRates + 8), &pktlen);


	/* todo:HT for adhoc */

_ConstructBeacon:

	if ((pktlen + TXDESC_SIZE) > 512) {
		RTW_INFO("beacon frame too large\n");
		return;
	}

	*pLength = pktlen;
}

static void ConstructPSPoll(PADAPTER adapter, u8 *pframe, u32 *pLength)
{
	struct rtw_ieee80211_hdr *pwlanhdr;
	u16 *fctrl;
	u32 pktlen;
	struct mlme_ext_priv *pmlmeext = &adapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;


	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	/* Frame control. */
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	SetPwrMgt(fctrl);
	set_frame_sub_type(pframe, WIFI_PSPOLL);

	/* AID. */
	set_duration(pframe, (pmlmeinfo->aid | 0xc000));

	/* BSSID. */
	_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	/* TA. */
	_rtw_memcpy(pwlanhdr->addr2, adapter_mac_addr(adapter), ETH_ALEN);

	*pLength = 16;
}

static void ConstructNullFunctionData(
	PADAPTER adapter,
	u8		*pframe,
	u32		*pLength,
	u8		*StaAddr,
	u8		bQoS,
	u8		AC,
	u8		bEosp,
	u8		bForcePowerSave)
{
	struct rtw_ieee80211_hdr *pwlanhdr;
	u16 *fctrl;
	u32 pktlen;
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
	struct wlan_network *cur_network = &pmlmepriv->cur_network;
	struct mlme_ext_priv *pmlmeext = &adapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;


	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	fctrl = &pwlanhdr->frame_ctl;
	*(fctrl) = 0;
	if (bForcePowerSave)
		SetPwrMgt(fctrl);

	switch (cur_network->network.InfrastructureMode) {
	case Ndis802_11Infrastructure:
		SetToDs(fctrl);
		_rtw_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr2, adapter_mac_addr(adapter), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, StaAddr, ETH_ALEN);
		break;
	case Ndis802_11APMode:
		SetFrDs(fctrl);
		_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr2, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, adapter_mac_addr(adapter), ETH_ALEN);
		break;
	case Ndis802_11IBSS:
	default:
		_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr2, adapter_mac_addr(adapter), ETH_ALEN);
		_rtw_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
		break;
	}

	SetSeqNum(pwlanhdr, 0);

	if (bQoS == _TRUE) {
		struct rtw_ieee80211_hdr_3addr_qos *pwlanqoshdr;

		set_frame_sub_type(pframe, WIFI_QOS_DATA_NULL);

		pwlanqoshdr = (struct rtw_ieee80211_hdr_3addr_qos *)pframe;
		SetPriority(&pwlanqoshdr->qc, AC);
		SetEOSP(&pwlanqoshdr->qc, bEosp);

		pktlen = sizeof(struct rtw_ieee80211_hdr_3addr_qos);
	} else {
		set_frame_sub_type(pframe, WIFI_DATA_NULL);

		pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);
	}

	*pLength = pktlen;
}

static void ConstructProbeRsp(PADAPTER adapter, u8 *pframe, u32 *pLength, u8 *StaAddr, BOOLEAN bHideSSID)
{
	struct rtw_ieee80211_hdr *pwlanhdr;
	u16 *fctrl;
	u8 *mac, *bssid;
	u32 pktlen;
	struct mlme_ext_priv *pmlmeext = &adapter->mlmeextpriv;
	struct mlme_ext_info *pmlmeinfo = &pmlmeext->mlmext_info;
	WLAN_BSSID_EX *cur_network = &pmlmeinfo->network;
#if defined(CONFIG_AP_MODE) && defined(CONFIG_NATIVEAP_MLME)
	u8 *pwps_ie;
	uint wps_ielen;
	struct mlme_priv *pmlmepriv = &adapter->mlmepriv;
#endif /* CONFIG_AP_MODE && CONFIG_NATIVEAP_MLME */
#ifdef CONFIG_P2P
	struct wifidirect_info *pwdinfo = &adapter->wdinfo;
#ifdef CONFIG_WFD
	u32 wfdielen = 0;
#endif /* CONFIG_WFD */
#endif /* CONFIG_P2P */

	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;

	mac = adapter_mac_addr(adapter);
	bssid = cur_network->MacAddress;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, bssid, ETH_ALEN);

	RTW_INFO("%s FW Mac Addr:" MAC_FMT "\n", __FUNCTION__, MAC_ARG(mac));
	RTW_INFO("%s FW IP Addr" IP_FMT "\n", __FUNCTION__, IP_ARG(StaAddr));

	SetSeqNum(pwlanhdr, 0);
	set_frame_sub_type(fctrl, WIFI_PROBERSP);

	pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);
	pframe += pktlen;

	if (cur_network->IELength > MAX_IE_SZ)
		return;

	pwps_ie = rtw_get_wps_ie(cur_network->IEs + _FIXED_IE_LENGTH_,
		 cur_network->IELength - _FIXED_IE_LENGTH_, NULL, &wps_ielen);

	/* inerset & update wps_probe_resp_ie */
	if ((pmlmepriv->wps_probe_resp_ie != NULL) && pwps_ie && (wps_ielen > 0)) {
		uint wps_offset, remainder_ielen;
		u8 *premainder_ie;

		wps_offset = (uint)(pwps_ie - cur_network->IEs);

		premainder_ie = pwps_ie + wps_ielen;

		remainder_ielen = cur_network->IELength - wps_offset - wps_ielen;

		_rtw_memcpy(pframe, cur_network->IEs, wps_offset);
		pframe += wps_offset;
		pktlen += wps_offset;

		wps_ielen = (uint)pmlmepriv->wps_probe_resp_ie[1];/* to get ie data len */
		if ((wps_offset + wps_ielen + 2) <= MAX_IE_SZ) {
			_rtw_memcpy(pframe, pmlmepriv->wps_probe_resp_ie, wps_ielen + 2);
			pframe += wps_ielen + 2;
			pktlen += wps_ielen + 2;
		}

		if ((wps_offset + wps_ielen + 2 + remainder_ielen) <= MAX_IE_SZ) {
			_rtw_memcpy(pframe, premainder_ie, remainder_ielen);
			pframe += remainder_ielen;
			pktlen += remainder_ielen;
		}
	} else {
		_rtw_memcpy(pframe, cur_network->IEs, cur_network->IELength);
		pframe += cur_network->IELength;
		pktlen += cur_network->IELength;
	}

	/* retrieve SSID IE from cur_network->Ssid */
	{
		u8 *ssid_ie;
		sint ssid_ielen = 0;
		sint ssid_ielen_diff;
		u8 buf[MAX_IE_SZ];
		u8 *ies = pframe + sizeof(struct rtw_ieee80211_hdr_3addr);

		ssid_ie = rtw_get_ie(ies + _FIXED_IE_LENGTH_, _SSID_IE_, &ssid_ielen,
				     (pframe - ies) - _FIXED_IE_LENGTH_);

		ssid_ielen_diff = cur_network->Ssid.SsidLength - ssid_ielen;

		if (ssid_ie &&	cur_network->Ssid.SsidLength) {
			uint remainder_ielen;
			u8 *remainder_ie;

			remainder_ie = ssid_ie + 2;
			remainder_ielen = pframe - remainder_ie;

			if (remainder_ielen > MAX_IE_SZ) {
				RTW_WARN(FUNC_ADPT_FMT" remainder_ielen > MAX_IE_SZ\n", FUNC_ADPT_ARG(adapter));
				remainder_ielen = MAX_IE_SZ;
			}

			_rtw_memcpy(buf, remainder_ie, remainder_ielen);
			_rtw_memcpy(remainder_ie + ssid_ielen_diff, buf, remainder_ielen);
			*(ssid_ie + 1) = cur_network->Ssid.SsidLength;
			_rtw_memcpy(ssid_ie + 2, cur_network->Ssid.Ssid, cur_network->Ssid.SsidLength);
			pframe += ssid_ielen_diff;
			pktlen += ssid_ielen_diff;
		}
	}

#ifdef CONFIG_P2P
	if (rtw_p2p_chk_role(pwdinfo, P2P_ROLE_GO)) {
		u32 len;
#ifdef CONFIG_IOCTL_CFG80211
		if (adapter_wdev_data(adapter)->p2p_enabled && (pwdinfo->driver_interface == DRIVER_CFG80211)) {
			/* if pwdinfo->role == P2P_ROLE_DEVICE will call issue_probersp_p2p() */
			len = pmlmepriv->p2p_go_probe_resp_ie_len;
			if (pmlmepriv->p2p_go_probe_resp_ie && (len > 0))
				_rtw_memcpy(pframe, pmlmepriv->p2p_go_probe_resp_ie, len);
		} else
#endif /* CONFIG_IOCTL_CFG80211 */
		{
			len = build_probe_resp_p2p_ie(pwdinfo, pframe);
		}

		pframe += len;
		pktlen += len;

#ifdef CONFIG_WFD
#ifdef CONFIG_IOCTL_CFG80211
		if (_FALSE == pwdinfo->wfd_info->wfd_enable) {
			len = 0;
			if (pmlmepriv->wfd_probe_resp_ie && (pmlmepriv->wfd_probe_resp_ie_len > 0)) {
				len = pmlmepriv->wfd_probe_resp_ie_len;
				_rtw_memcpy(pframe, pmlmepriv->wfd_probe_resp_ie, len);
			}
		} else
#endif /* CONFIG_IOCTL_CFG80211 */
		{
			len = build_probe_resp_wfd_ie(pwdinfo, pframe, 0);
		}

		pframe += len;
		pktlen += len;
#endif /* CONFIG_WFD */
	}
#endif /* CONFIG_P2P */

	*pLength = pktlen;
}

static void ConstructBtNullFunctionData(
	PADAPTER adapter,
	u8 *pframe,
	u32 *pLength,
	u8 *StaAddr,
	u8 bQoS,
	u8 AC,
	u8 bEosp,
	u8 bForcePowerSave)
{
	struct rtw_ieee80211_hdr *pwlanhdr;
	u16 *fctrl;
	u32 pktlen;
	struct mlme_ext_priv *pmlmeext;
	struct mlme_ext_info *pmlmeinfo;
	u8 bssid[ETH_ALEN];


	RTW_INFO("+" FUNC_ADPT_FMT ": qos=%d eosp=%d ps=%d\n",
		 FUNC_ADPT_ARG(adapter), bQoS, bEosp, bForcePowerSave);

	pwlanhdr = (struct rtw_ieee80211_hdr *)pframe;
	pmlmeext = &adapter->mlmeextpriv;
	pmlmeinfo = &pmlmeext->mlmext_info;

	if (NULL == StaAddr) {
		_rtw_memcpy(bssid, adapter_mac_addr(adapter), ETH_ALEN);
		StaAddr = bssid;
	}

	fctrl = &pwlanhdr->frame_ctl;
	*fctrl = 0;
	if (bForcePowerSave)
		SetPwrMgt(fctrl);

	SetFrDs(fctrl);
	_rtw_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr2, adapter_mac_addr(adapter), ETH_ALEN);
	_rtw_memcpy(pwlanhdr->addr3, adapter_mac_addr(adapter), ETH_ALEN);

	set_duration(pwlanhdr, 0);
	SetSeqNum(pwlanhdr, 0);

	if (bQoS == _TRUE) {
		struct rtw_ieee80211_hdr_3addr_qos *pwlanqoshdr;

		set_frame_sub_type(pframe, WIFI_QOS_DATA_NULL);

		pwlanqoshdr = (struct rtw_ieee80211_hdr_3addr_qos *)pframe;
		SetPriority(&pwlanqoshdr->qc, AC);
		SetEOSP(&pwlanqoshdr->qc, bEosp);

		pktlen = sizeof(struct rtw_ieee80211_hdr_3addr_qos);
	} else {
		set_frame_sub_type(pframe, WIFI_DATA_NULL);

		pktlen = sizeof(struct rtw_ieee80211_hdr_3addr);
	}

	*pLength = pktlen;
}

static void SetFwRsvdPagePkt_BTCoex(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;
	struct xmit_frame *pcmdframe;
	struct pkt_attrib *pattrib;
	struct xmit_priv *pxmitpriv;
	struct mlme_ext_priv *pmlmeext;
	struct mlme_ext_info *pmlmeinfo;
	u32 BeaconLength = 0;
	u32 BTQosNullLength = 0;
	u8 *ReservedPagePacket;
	u32 page_size, desc_size;
	u8 TxDescOffset;
	u8 TotalPageNum = 0, CurtPktPageNum = 0, RsvdPageNum = 0;
	u16 BufIndex;
	u32 TotalPacketLen, MaxRsvdPageBufSize = 0;
	RSVDPAGE_LOC RsvdPageLoc;


	hal = GET_HAL_DATA(adapter);
	pxmitpriv = &adapter->xmitpriv;
	pmlmeext = &adapter->mlmeextpriv;
	pmlmeinfo = &pmlmeext->mlmext_info;
	rtw_hal_get_def_var(adapter, HAL_DEF_TX_PAGE_SIZE, &page_size);
	desc_size = rtl8822b_get_tx_desc_size(adapter);
	TxDescOffset = TXDESC_OFFSET;

	RsvdPageNum = rtw_hal_get_txbuff_rsvd_page_num(adapter, _FALSE);
	MaxRsvdPageBufSize = RsvdPageNum * page_size;

	pcmdframe = rtw_alloc_cmdxmitframe(pxmitpriv);
	if (pcmdframe == NULL) {
		RTW_INFO("%s: alloc ReservedPagePacket fail!\n", __FUNCTION__);
		return;
	}

	ReservedPagePacket = pcmdframe->buf_addr;
	_rtw_memset(&RsvdPageLoc, 0, sizeof(RSVDPAGE_LOC));

	/* (1) beacon */
	BufIndex = TxDescOffset;
	ConstructBeacon(adapter, &ReservedPagePacket[BufIndex], &BeaconLength);

	/*
	 * When we count the first page size, we need to reserve description size for the RSVD
	 * packet, it will be filled in front of the packet in TXPKTBUF.
	 */
	CurtPktPageNum = (u8)PageNum(desc_size + BeaconLength, page_size);
	/*
	 * If we don't add 1 more page, the WOWLAN function has a problem.
	 * Maybe it's a bug of firmware?
	 */
	if (CurtPktPageNum == 1)
		CurtPktPageNum += 1;
	TotalPageNum += CurtPktPageNum;

	BufIndex += (CurtPktPageNum * page_size);

	/* Jump to lastest page */
	if (BufIndex < (MaxRsvdPageBufSize - page_size)) {
		BufIndex = TxDescOffset + (MaxRsvdPageBufSize - page_size);
		TotalPageNum = RsvdPageNum - 1;
	}

	/* (6) BT Qos null data */
	RsvdPageLoc.LocBTQosNull = TotalPageNum;
	ConstructBtNullFunctionData(
		adapter,
		&ReservedPagePacket[BufIndex],
		&BTQosNullLength,
		NULL,
		_TRUE, 0, 0, _FALSE);
	rtw_hal_fill_fake_txdesc(adapter, &ReservedPagePacket[BufIndex - desc_size], BTQosNullLength, _FALSE, _TRUE, _FALSE);

	CurtPktPageNum = (u8)PageNum(desc_size + BTQosNullLength, page_size);

	TotalPageNum += CurtPktPageNum;

	TotalPacketLen = BufIndex + BTQosNullLength;
	if (TotalPacketLen > MaxRsvdPageBufSize) {
		RTW_INFO(FUNC_ADPT_FMT ": ERROR: The rsvd page size is not enough!!TotalPacketLen %d, MaxRsvdPageBufSize %d\n",
			FUNC_ADPT_ARG(adapter), TotalPacketLen, MaxRsvdPageBufSize);
		goto error;
	}

	/* update attribute */
	pattrib = &pcmdframe->attrib;
	update_mgntframe_attrib(adapter, pattrib);
	pattrib->qsel = QSLT_BEACON;
	pattrib->pktlen = pattrib->last_txcmdsz = TotalPacketLen - TxDescOffset;
#ifdef CONFIG_PCI_HCI
	dump_mgntframe(adapter, pcmdframe);
#else /* !CONFIG_PCI_HCI */
	dump_mgntframe_and_wait(adapter, pcmdframe, 100);
#endif /* !CONFIG_PCI_HCI */

	rtl8822b_set_FwRsvdPage_cmd(adapter, &RsvdPageLoc);
	rtl8822b_set_FwAoacRsvdPage_cmd(adapter, &RsvdPageLoc);

	return;

error:
	rtw_free_xmitframe(pxmitpriv, pcmdframe);
}

void rtl8822b_download_BTCoex_AP_mode_rsvd_page(PADAPTER adapter)
{
	PHAL_DATA_TYPE hal;
	struct mlme_ext_priv *pmlmeext;
	struct mlme_ext_info *pmlmeinfo;
	u8 bRecover = _FALSE;
	u8 bcn_valid = _FALSE;
	u8 DLBcnCount = 0;
	u32 poll = 0;
	u8 val8, RegFwHwTxQCtrl;
	u8 restore[2];


	RTW_INFO("+" FUNC_ADPT_FMT ": hw_port=%d fw_state=0x%08X\n",
		FUNC_ADPT_ARG(adapter), get_hw_port(adapter), get_fwstate(&adapter->mlmepriv));

#ifdef CONFIG_RTW_DEBUG
	if (check_fwstate(&adapter->mlmepriv, WIFI_AP_STATE) == _FALSE) {
		RTW_INFO(FUNC_ADPT_FMT ": [WARNING] not in AP mode!!\n",
			 FUNC_ADPT_ARG(adapter));
	}
#endif /* CONFIG_RTW_DEBUG */

	hal = GET_HAL_DATA(adapter);
	pmlmeext = &adapter->mlmeextpriv;
	pmlmeinfo = &pmlmeext->mlmext_info;

	/* We should set AID, correct TSF, HW seq enable before set JoinBssReport to Fw in 88/92C. */
	rtw_write16(adapter, REG_BCN_PSR_RPT_8822B, (0xC000 | pmlmeinfo->aid));

	/* set REG_CR bit 8 */
	val8 = rtw_read8(adapter, REG_CR_8822B + 1);
	restore[0] = val8;
	val8 |= BIT(0); /* ENSWBCN */
	rtw_write8(adapter,  REG_CR_8822B + 1, val8);

	/*
	 * Disable Hw protection for a time which revserd for Hw sending beacon.
	 * Fix download reserved page packet fail that access collision with the protection time.
	 */
	val8 = rtw_read8(adapter, REG_BCN_CTRL_8822B);
	restore[1] = val8;
	val8 &= ~BIT_EN_BCN_FUNCTION_8822B;
	val8 |= BIT_DIS_TSF_UDT_8822B;
	rtw_write8(adapter, REG_BCN_CTRL_8822B, val8);

	/* Set FWHW_TXQ_CTRL 0x422[6]=0 to tell Hw the packet is not a real beacon frame. */
	RegFwHwTxQCtrl = rtw_read8(adapter, REG_FWHW_TXQ_CTRL_8822B + 2);
	if (RegFwHwTxQCtrl & BIT(6))
		bRecover = _TRUE;

	/* To tell Hw the packet is not a real beacon frame. */
	RegFwHwTxQCtrl &= ~BIT(6);
	rtw_write8(adapter, REG_FWHW_TXQ_CTRL_8822B + 2, RegFwHwTxQCtrl);

	/* Clear beacon valid check bit. */
	rtw_hal_set_hwreg(adapter, HW_VAR_BCN_VALID, NULL);
	rtw_hal_set_hwreg(adapter, HW_VAR_DL_BCN_SEL, NULL);

	DLBcnCount = 0;
	poll = 0;
	do {
		SetFwRsvdPagePkt_BTCoex(adapter);
		DLBcnCount++;
		do {
			rtw_yield_os();

			/* check rsvd page download OK. */
			rtw_hal_get_hwreg(adapter, HW_VAR_BCN_VALID, &bcn_valid);
			poll++;
		} while (!bcn_valid && (poll % 10) != 0 && !RTW_CANNOT_RUN(adapter));
	} while (!bcn_valid && (DLBcnCount <= 100) && !RTW_CANNOT_RUN(adapter));

	if (_TRUE == bcn_valid) {
		struct pwrctrl_priv *pwrctl = adapter_to_pwrctl(adapter);

		pwrctl->fw_psmode_iface_id = adapter->iface_id;
		RTW_INFO(ADPT_FMT": DL RSVD page success! DLBcnCount:%d, poll:%d\n",
			 ADPT_ARG(adapter), DLBcnCount, poll);
	} else {
		RTW_INFO(ADPT_FMT": DL RSVD page fail! DLBcnCount:%d, poll:%d\n",
			 ADPT_ARG(adapter), DLBcnCount, poll);
		RTW_INFO(ADPT_FMT": DL RSVD page fail! bSurpriseRemoved=%s\n",
			ADPT_ARG(adapter), rtw_is_surprise_removed(adapter) ? "True" : "False");
		RTW_INFO(ADPT_FMT": DL RSVD page fail! bDriverStopped=%s\n",
			ADPT_ARG(adapter), rtw_is_drv_stopped(adapter) ? "True" : "False");
	}

	/*
	 * To make sure that if there exists an adapter which would like to send beacon.
	 * If exists, the origianl value of 0x422[6] will be 1, we should check this to
	 * prevent from setting 0x422[6] to 0 after download reserved page, or it will cause
	 * the beacon cannot be sent by HW.
	 */
	if (bRecover) {
		RegFwHwTxQCtrl |= BIT(6);
		rtw_write8(adapter, REG_FWHW_TXQ_CTRL_8822B + 2, RegFwHwTxQCtrl);
	}

	rtw_write8(adapter, REG_BCN_CTRL_8822B, restore[1]);
	rtw_write8(adapter,  REG_CR_8822B + 1, restore[0]);

	/* Clear CR[8] or beacon packet will not be send to TxBuf anymore. */
#ifndef CONFIG_PCI_HCI
	val8 = rtw_read8(adapter, REG_CR_8822B + 1);
	val8 &= ~BIT(0); /* ~ENSWBCN */
	rtw_write8(adapter, REG_CR_8822B + 1, val8);
#endif /* !CONFIG_PCI_HCI */
}
#endif /* CONFIG_BT_COEXIST */

void rtl8822b_fw_update_beacon_cmd(PADAPTER adapter)
{
}

/*
 * Below functions are for C2H
 */
static void c2h_ccx_rpt(PADAPTER adapter, u8 *pdata)
{
#ifdef CONFIG_XMIT_ACK
	u8 tx_state;


	tx_state = CCX_RPT_GET_TX_STATE(pdata);

	/* 0 means success, 1 means retry drop */
	if (tx_state == 0)
		rtw_ack_tx_done(&adapter->xmitpriv, RTW_SCTX_DONE_SUCCESS);
	else
		rtw_ack_tx_done(&adapter->xmitpriv, RTW_SCTX_DONE_CCX_PKT_FAIL);
#endif /* CONFIG_XMIT_ACK */
}

static VOID
C2HTxRPTHandler_8822b(
	IN	PADAPTER	Adapter,
	IN	u8			*CmdBuf,
	IN	u8			CmdLen
)
{
	_irqL	 irqL;
	u8 macid = 0, IniRate = 0;
	u16 TxOK = 0, TxFail = 0;
	u8 TxOK0 = 0, TxOK1 = 0;
	u8 TxFail0 = 0, TxFail1 = 0;
	struct sta_priv *pstapriv = &(Adapter->stapriv);
	struct sta_info *psta = NULL;

	if (!pstapriv->c2h_sta) {
		RTW_WARN("%s: No corresponding sta_info!\n", __FUNCTION__);
		return;
	}
	psta = pstapriv->c2h_sta;
	macid = C2H_AP_REQ_TXRPT_GET_STA1_MACID(CmdBuf);
	TxOK0 = C2H_AP_REQ_TXRPT_GET_TX_OK1_0(CmdBuf);
	TxOK1 = C2H_AP_REQ_TXRPT_GET_TX_OK1_1(CmdBuf);
	TxOK = (TxOK1 << 8) | TxOK0;
	TxFail0 = C2H_AP_REQ_TXRPT_GET_TX_FAIL1_0(CmdBuf);
	TxFail1 = C2H_AP_REQ_TXRPT_GET_TX_FAIL1_1(CmdBuf);
	TxFail = (TxFail1 << 8) | TxFail0;
	IniRate = C2H_AP_REQ_TXRPT_GET_INITIAL_RATE1(CmdBuf);

	psta->sta_stats.tx_ok_cnt = TxOK;
	psta->sta_stats.tx_fail_cnt = TxFail;

}

static VOID
C2HSPC_STAT_8822b(
	IN	PADAPTER	Adapter,
	IN	u8			*CmdBuf,
	IN	u8			CmdLen
)
{
	_irqL	 irqL;
	struct sta_priv *pstapriv = &(Adapter->stapriv);
	struct sta_info *psta = NULL;
	_list	*plist, *phead;
	u8 idx = C2H_SPECIAL_STATISTICS_GET_STATISTICS_IDX(CmdBuf);

	if (!pstapriv->c2h_sta) {
		RTW_WARN("%s: No corresponding sta_info!\n", __FUNCTION__);
		return;
	}
	psta = pstapriv->c2h_sta;
	psta->sta_stats.tx_retry_cnt = (C2H_SPECIAL_STATISTICS_GET_DATA3(CmdBuf) << 8) | C2H_SPECIAL_STATISTICS_GET_DATA2(CmdBuf);
	pstapriv->c2h_sta = NULL;
	rtw_sctx_done(&pstapriv->gotc2h);
}

/**
 * c2h = RXDESC + c2h packet
 * size = RXDESC_SIZE + c2h packet size
 * c2h payload = c2h packet revmoe id & seq
 */
static void process_c2h_event(PADAPTER adapter, u8 *c2h, u32 size)
{
	struct mlme_ext_priv *pmlmeext;
	struct mlme_ext_info *pmlmeinfo;
	u32 desc_size;
	u8 id, seq;
	u8 c2h_len, c2h_payload_len;
	u8 *pc2h_data, *pc2h_payload;


	if (!c2h) {
		RTW_INFO("%s: c2h buffer is NULL!!\n", __FUNCTION__);
		return;
	}

	desc_size = rtl8822b_get_rx_desc_size(adapter);

	if (size < desc_size) {
		RTW_INFO("%s: c2h length(%d) is smaller than RXDESC_SIZE(%d)!!\n",
			 __FUNCTION__, size, desc_size);
		return;
	}

	pmlmeext = &adapter->mlmeextpriv;
	pmlmeinfo = &pmlmeext->mlmext_info;

	/* shift rx desc len */
	pc2h_data = c2h + desc_size;
	c2h_len = size - desc_size;

	id = C2H_GET_CMD_ID(pc2h_data);
	seq = C2H_GET_SEQ(pc2h_data);

	/* shift 2 byte to remove cmd id & seq */
	pc2h_payload = pc2h_data + 2;
	c2h_payload_len = c2h_len - 2;

	switch (id) {
#ifdef CONFIG_BEAMFORMING
	case CMD_ID_C2H_SND_TXBF:
		RTW_INFO("%s: [CMD_ID_C2H_SND_TXBF] len=%d\n", __FUNCTION__, c2h_payload_len);
		rtw_bf_c2h_handler(adapter, id, pc2h_data, c2h_len);
		break;
#endif /* CONFIG_BEAMFORMING */

	case CMD_ID_C2H_AP_REQ_TXRPT:
		/*RTW_INFO("[C2H], C2H_AP_REQ_TXRPT!!\n");*/
		C2HTxRPTHandler_8822b(adapter, pc2h_data, c2h_len);
		break;

	case CMD_ID_C2H_SPECIAL_STATISTICS:
		/*RTW_INFO("[C2H], C2H_SPC_STAT!!\n");*/
		C2HSPC_STAT_8822b(adapter, pc2h_data, c2h_len);
		break;

	case C2H_EXTEND:
		if (C2H_HDR_GET_C2H_SUB_CMD_ID(pc2h_data) == C2H_SUB_CMD_ID_CCX_RPT) {
			/* Shift C2H HDR 4 bytes */
			c2h_ccx_rpt(adapter, pc2h_data);
			break;
		}

		/* indicate c2h pkt + rx desc to halmac */
		rtw_halmac_c2h_handle(adapter_to_dvobj(adapter), c2h, size);
		break;

	/* others for c2h common code */
	default:
		c2h_handler(adapter, id, seq, c2h_payload_len, pc2h_payload);
		break;
	}
}

void rtl8822b_c2h_handler(PADAPTER adapter, u8 *pbuf, u16 length)
{
#ifdef CONFIG_WOWLAN
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(adapter);


	if (pwrpriv->wowlan_mode == _TRUE) {
#ifdef CONFIG_RTW_DEBUG
		u32 desc_size;

		desc_size = rtl8822b_get_rx_desc_size(adapter);
		RTW_INFO("%s: return because wowolan_mode==TRUE! CMDID=%d\n",
			 __FUNCTION__, C2H_GET_CMD_ID(pbuf + desc_size));
#endif /* CONFIG_RTW_DEBUG */
		return;
	}
#endif /* CONFIG_WOWLAN*/

	process_c2h_event(adapter, pbuf, length);
}

/**
 * pbuf = RXDESC + c2h packet
 * length = RXDESC_SIZE + c2h packet size
 */
void rtl8822b_c2h_handler_no_io(PADAPTER adapter, u8 *pbuf, u16 length)
{
	u32 desc_size;
	u8 id, seq;
	u8 *pc2h_content;
	u8 res;


	if ((length == 0) || (!pbuf))
		return;

	desc_size = rtl8822b_get_rx_desc_size(adapter);

	/* shift rx desc len to get c2h packet content */
	pc2h_content = pbuf + desc_size;
	id = C2H_GET_CMD_ID(pc2h_content);
	seq = C2H_GET_SEQ(pc2h_content);

	RTW_INFO("%s: C2H, ID=%d seq=%d len=%d\n",
		 __FUNCTION__, id, seq, length);

	switch (id) {
	case CMD_ID_C2H_SND_TXBF:
	case CMD_ID_C2H_CCX_RPT:
	case C2H_BT_MP_INFO:
	case C2H_FW_CHNL_SWITCH_COMPLETE:
	case C2H_IQK_FINISH:
	case C2H_MCC:
	case C2H_BCN_EARLY_RPT:
	case C2H_EXTEND:
		/* no I/O, process directly */
		process_c2h_event(adapter, pbuf, length);
		break;

	default:
		/* Others may need I/O, run in command thread */
		res = rtw_c2h_packet_wk_cmd(adapter, pbuf, length);
		if (res == _FAIL)
			RTW_ERR("%s: C2H(%d) enqueue FAIL!\n", __FUNCTION__, id);
		break;
	}
}

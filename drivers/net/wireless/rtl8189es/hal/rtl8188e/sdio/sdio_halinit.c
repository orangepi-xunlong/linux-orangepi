/******************************************************************************
 *
 * Copyright(c) 2007 - 2017 Realtek Corporation.
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
#define _SDIO_HALINIT_C_

#include <drv_types.h>
#include <rtl8188e_hal.h>
#include "hal_com_h2c.h"
#ifndef CONFIG_SDIO_HCI
	#error "CONFIG_SDIO_HCI shall be on!\n"
#endif

#ifdef CONFIG_EXT_CLK
void EnableGpio5ClockReq(PADAPTER Adapter, u8 in_interrupt, u32 Enable)
{
	u32	value32;
	HAL_DATA_TYPE	*pHalData;

	pHalData = GET_HAL_DATA(Adapter);
	if (IS_D_CUT(pHalData->version_id))
		return;

	/* dbgdump("%s Enable:%x time:%d", __RTL_FUNC__, Enable, rtw_get_current_time()); */

	if (in_interrupt)
		value32 = _sdio_read32(Adapter, REG_GPIO_PIN_CTRL);
	else
		value32 = rtw_read32(Adapter, REG_GPIO_PIN_CTRL);

	/* open GPIO 5 */
	if (Enable)
		value32 |= BIT(13);/* 5+8 */
	else
		value32 &= ~BIT(13);

	/* GPIO 5 out put */
	value32 |= BIT(21);/* 5+16 */

	/* if (Enable) */
	/*	rtw_write8(Adapter, REG_GPIO_PIN_CTRL + 1, 0x20); */
	/* else */
	/*	rtw_write8(Adapter, REG_GPIO_PIN_CTRL + 1, 0x00); */

	if (in_interrupt)
		_sdio_write32(Adapter, REG_GPIO_PIN_CTRL, value32);
	else
		rtw_write32(Adapter, REG_GPIO_PIN_CTRL, value32);

} /* end of EnableGpio5ClockReq() */

void _InitClockTo26MHz(
		PADAPTER Adapter
)
{
	u8 u1temp = 0;
	HAL_DATA_TYPE	*pHalData;

	pHalData = GET_HAL_DATA(Adapter);

	if (IS_D_CUT(pHalData->version_id)) {
		/* FW special init */
		u1temp =  rtw_read8(Adapter, REG_XCK_OUT_CTRL);
		u1temp |= 0x18;
		rtw_write8(Adapter, REG_XCK_OUT_CTRL, u1temp);
		RTW_INFO("D cut version\n");
	}

	EnableGpio5ClockReq(Adapter, _FALSE, 1);

	/* 0x2c[3:0] = 5 will set clock to 26MHz */
	u1temp = rtw_read8(Adapter, REG_APE_PLL_CTRL_EXT);
	u1temp = (u1temp & 0xF0) | 0x05;
	rtw_write8(Adapter, REG_APE_PLL_CTRL_EXT, u1temp);
}
#endif


static void rtl8188es_interface_configure(PADAPTER padapter)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(padapter);
	struct dvobj_priv		*pdvobjpriv = adapter_to_dvobj(padapter);
	struct registry_priv	*pregistrypriv = &padapter->registrypriv;
	BOOLEAN		bWiFiConfig	= pregistrypriv->wifi_spec;


	pdvobjpriv->RtOutPipe[0] = WLAN_TX_HIQ_DEVICE_ID;
	pdvobjpriv->RtOutPipe[1] = WLAN_TX_MIQ_DEVICE_ID;
	pdvobjpriv->RtOutPipe[2] = WLAN_TX_LOQ_DEVICE_ID;

	if (bWiFiConfig)
		pHalData->OutEpNumber = 2;
	else
		pHalData->OutEpNumber = SDIO_MAX_TX_QUEUE;

	switch (pHalData->OutEpNumber) {
	case 3:
		pHalData->OutEpQueueSel = TX_SELE_HQ | TX_SELE_LQ | TX_SELE_NQ;
		break;
	case 2:
		pHalData->OutEpQueueSel = TX_SELE_HQ | TX_SELE_NQ;
		break;
	case 1:
		pHalData->OutEpQueueSel = TX_SELE_HQ;
		break;
	default:
		break;
	}

	Hal_MappingOutPipe(padapter, pHalData->OutEpNumber);
}

/*
 * Description:
 *	Call power on sequence to enable card
 *
 * Return:
 *	_SUCCESS	enable success
 *	_FAIL		enable fail
 */
static u8 _CardEnable(PADAPTER padapter)
{
	u8 bMacPwrCtrlOn;
	u8 ret;

	RTW_INFO("=>%s\n", __FUNCTION__);

	rtw_hal_get_hwreg(padapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);
	if (bMacPwrCtrlOn == _FALSE) {
#ifdef CONFIG_PLATFORM_SPRD
		u8 val8;
#endif /* CONFIG_PLATFORM_SPRD */

		/* RSV_CTRL 0x1C[7:0] = 0x00 */
		/* unlock ISO/CLK/Power control register */
		rtw_write8(padapter, REG_RSV_CTRL, 0x0);

#ifdef CONFIG_EXT_CLK
		_InitClockTo26MHz(padapter);
#endif /* CONFIG_EXT_CLK */

#ifdef CONFIG_PLATFORM_SPRD
		val8 =  rtw_read8(padapter, 0x4);
		val8 = val8 & ~BIT(5);
		rtw_write8(padapter, 0x4, val8);
#endif /* CONFIG_PLATFORM_SPRD */

		ret = HalPwrSeqCmdParsing(padapter, PWR_CUT_ALL_MSK, PWR_FAB_ALL_MSK, PWR_INTF_SDIO_MSK, Rtl8188E_NIC_ENABLE_FLOW);
		if (ret == _SUCCESS) {
			u8 bMacPwrCtrlOn = _TRUE;
			rtw_hal_set_hwreg(padapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);
		} else {
			RTW_ERR("%s: run power on flow fail\n", __func__);
			return _FAIL;
		}

	} else {

		RTW_INFO("=>%s bMacPwrCtrlOn == _TRUE do nothing !!\n", __FUNCTION__);
		ret = _SUCCESS;
	}

	RTW_INFO("<=%s\n", __FUNCTION__);

	return ret;

}

static u32 _InitPowerOn_8188ES(PADAPTER padapter)
{
	u8 value8;
	u16 value16;
	u32 value32;
	u8 ret;

	RTW_INFO("=>%s\n", __FUNCTION__);

	ret = _CardEnable(padapter);
	if (ret == _FAIL)
		return ret;

#if 0
	/*  Radio-Off Pin Trigger */
	value8 = rtw_read8(padapter, REG_GPIO_INTM + 1);
	value8 |= BIT(1); /*  Enable falling edge triggering interrupt */
	rtw_write8(padapter, REG_GPIO_INTM + 1, value8);
	value8 = rtw_read8(padapter, REG_GPIO_IO_SEL_2 + 1);
	value8 |= BIT(1);
	rtw_write8(padapter, REG_GPIO_IO_SEL_2 + 1, value8);
#endif

	/* Enable power down and GPIO interrupt */
	value16 = rtw_read16(padapter, REG_APS_FSMCO);
	value16 |= EnPDN; /* Enable HW power down and RF on */
	rtw_write16(padapter, REG_APS_FSMCO, value16);


	/* Enable MAC DMA/WMAC/SCHEDULE/SEC block */
	value16 = rtw_read16(padapter, REG_CR);
	value16 |= (HCI_TXDMA_EN | HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN
		    | PROTOCOL_EN | SCHEDULE_EN | ENSEC | CALTMR_EN);
	/* for SDIO - Set CR bit10 to enable 32k calibration. Suggested by SD1 Gimmy. Added by tynli. 2011.08.31. */

	rtw_write16(padapter, REG_CR, value16);

	/* Enable CMD53 R/W Operation
	*	bMacPwrCtrlOn = TRUE;
	*	rtw_hal_set_hwreg(padapter, HW_VAR_APFM_ON_MAC, (pu8)(&bMacPwrCtrlOn)); */

	RTW_INFO("<=%s\n", __FUNCTION__);

	return _SUCCESS;

}

static void hal_poweroff_8188es(PADAPTER padapter)
{
	u8		u1bTmp;
	u16		u2bTmp;
	u32		u4bTmp;
	u8		bMacPwrCtrlOn = _FALSE;
	u8		ret;

#ifdef CONFIG_PLATFORM_SPRD
	struct pwrctrl_priv *pwrpriv = adapter_to_pwrctl(padapter);
#endif /* CONFIG_PLATFORM_SPRD	 */

	rtw_hal_get_hwreg(padapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);
	if (bMacPwrCtrlOn == _FALSE) {
		RTW_INFO("=>%s bMacPwrCtrlOn == _FALSE return !!\n", __FUNCTION__);
		return;
	}
	RTW_INFO("=>%s\n", __FUNCTION__);


	/* Stop Tx Report Timer. 0x4EC[Bit1]=b'0 */
	u1bTmp = rtw_read8(padapter, REG_TX_RPT_CTRL);
	rtw_write8(padapter, REG_TX_RPT_CTRL, u1bTmp & (~BIT1));

	/* stop rx */
	rtw_write8(padapter, REG_CR, 0x0);


#ifdef CONFIG_EXT_CLK /* for sprd For Power Consumption. */
	EnableGpio5ClockReq(padapter, _FALSE, 0);
#endif /* CONFIG_EXT_CLK */

#if 1
	/* For Power Consumption. */
	u1bTmp = rtw_read8(padapter, GPIO_IN);
	rtw_write8(padapter, GPIO_OUT, u1bTmp);
	rtw_write8(padapter, GPIO_IO_SEL, 0xFF);/* Reg0x46 */

	u1bTmp = rtw_read8(padapter, REG_GPIO_IO_SEL);
	rtw_write8(padapter, REG_GPIO_IO_SEL, (u1bTmp << 4) | u1bTmp);
	u1bTmp = rtw_read8(padapter, REG_GPIO_IO_SEL + 1);
	rtw_write8(padapter, REG_GPIO_IO_SEL + 1, u1bTmp | 0x0F); /* Reg0x43 */
#endif


	/* Run LPS WL RFOFF flow	 */
	ret = HalPwrSeqCmdParsing(padapter, PWR_CUT_ALL_MSK, PWR_FAB_ALL_MSK, PWR_INTF_SDIO_MSK, Rtl8188E_NIC_LPS_ENTER_FLOW);
	if (ret == _FALSE)
		RTW_INFO("%s: run RF OFF flow fail!\n", __func__);

	/*	==== Reset digital sequence   ====== */

	u1bTmp = rtw_read8(padapter, REG_MCUFWDL);
	if ((u1bTmp & RAM_DL_SEL) && GET_HAL_DATA(padapter)->bFWReady) { /* 8051 RAM code */
		/* _8051Reset88E(padapter);		 */

		/* Reset MCU 0x2[10]=0. */
		u1bTmp = rtw_read8(padapter, REG_SYS_FUNC_EN + 1);
		u1bTmp &= ~BIT(2);	/* 0x2[10], FEN_CPUEN */
		rtw_write8(padapter, REG_SYS_FUNC_EN + 1, u1bTmp);
	}

	/* u1bTmp = rtw_read8(padapter, REG_SYS_FUNC_EN+1); */
	/* u1bTmp &= ~BIT(2);	 */ /* 0x2[10], FEN_CPUEN */
	/* rtw_write8(padapter, REG_SYS_FUNC_EN+1, u1bTmp); */

	/* MCUFWDL 0x80[1:0]=0 */
	/* reset MCU ready status */
	rtw_write8(padapter, REG_MCUFWDL, 0);

	/* ==== Reset digital sequence end ====== */


	bMacPwrCtrlOn = _FALSE;	/* Disable CMD53 R/W */
	rtw_hal_set_hwreg(padapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);


#if 0
	if ((pMgntInfo->RfOffReason & RF_CHANGE_BY_HW) && pHalData->pwrdown) {
		/*  Power Down */

		/*  Card disable power action flow */
		ret = HalPwrSeqCmdParsing(Adapter, PWR_CUT_ALL_MSK, PWR_FAB_ALL_MSK, PWR_INTF_SDIO_MSK, Rtl8188E_NIC_PDN_FLOW);
	} else
#endif

	{ /* Non-Power Down */

		/* Card disable power action flow */
		ret = HalPwrSeqCmdParsing(padapter, PWR_CUT_ALL_MSK, PWR_FAB_ALL_MSK, PWR_INTF_SDIO_MSK, Rtl8188E_NIC_DISABLE_FLOW);


		if (ret == _FALSE)
			RTW_INFO("%s: run CARD DISABLE flow fail!\n", __func__);
	}


#if 0
	/*  Reset MCU IO Wrapper, added by Roger, 2011.08.30 */
	u1bTmp = rtw_read8(padapter, REG_RSV_CTRL + 1);
	u1bTmp &= ~BIT(0);
	rtw_write8(padapter, REG_RSV_CTRL + 1, u1bTmp);
	u1bTmp = rtw_read8(padapter, REG_RSV_CTRL + 1);
	u1bTmp |= BIT(0);
	rtw_write8(padapter, REG_RSV_CTRL + 1, u1bTmp);
#endif


	/* RSV_CTRL 0x1C[7:0]=0x0E */
	/* lock ISO/CLK/Power control register */
	rtw_write8(padapter, REG_RSV_CTRL, 0x0E);

	GET_HAL_DATA(padapter)->bFWReady = _FALSE;
	bMacPwrCtrlOn = _FALSE;
	rtw_hal_set_hwreg(padapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);

	RTW_INFO("<=%s\n", __FUNCTION__);

}
#ifdef CONFIG_SDIO_TX_ENABLE_AVAL_INT
/* Tx Page FIFO threshold */
static void _init_available_page_threshold(PADAPTER padapter)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(padapter);

	hal_data->sdio_avail_int_en_q = 0xFF;
	rtw_write32(padapter, REG_TQPNT1, 0xFFFFFFFF);
	rtw_write32(padapter, REG_TQPNT2, 0xFFFFFFFF);
}
#endif
static void _InitQueueReservedPage(PADAPTER padapter)
{
#ifdef RTL8188ES_MAC_LOOPBACK

	/* #define MAC_LOOPBACK_PAGE_NUM_PUBQ		0x26 */
	/* #define MAC_LOOPBACK_PAGE_NUM_HPQ		0x0b */
	/* #define MAC_LOOPBACK_PAGE_NUM_LPQ		0x0b */
	/* #define MAC_LOOPBACK_PAGE_NUM_NPQ		0x0b */ /* 71 pages=>9088 bytes, 8.875k */

	rtw_write16(padapter, REG_RQPN_NPQ, 0x0b0b);
	rtw_write32(padapter, REG_RQPN, 0x80260b0b);

#else /* TX_PAGE_BOUNDARY_LOOPBACK_MODE */
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(padapter);
	struct registry_priv	*pregistrypriv = &padapter->registrypriv;
	u32			outEPNum	= (u32)pHalData->OutEpNumber;
	u32			numHQ		= 0;
	u32			numLQ		= 0;
	u32			numNQ		= 0;
	u32			numPubQ	= 0x00;
	u32			value32;
	u8			value8;
	BOOLEAN			bWiFiConfig	= pregistrypriv->wifi_spec;

	if (bWiFiConfig) {
		if (pHalData->OutEpQueueSel & TX_SELE_HQ)
			numHQ =  WMM_NORMAL_PAGE_NUM_HPQ_88E;

		if (pHalData->OutEpQueueSel & TX_SELE_LQ)
			numLQ = WMM_NORMAL_PAGE_NUM_LPQ_88E;

		/* NOTE: This step shall be proceed before writting REG_RQPN. */
		if (pHalData->OutEpQueueSel & TX_SELE_NQ)
			numNQ = WMM_NORMAL_PAGE_NUM_NPQ_88E;
	} else {
		if (pHalData->OutEpQueueSel & TX_SELE_HQ)
			numHQ = NORMAL_PAGE_NUM_HPQ_88E;

		if (pHalData->OutEpQueueSel & TX_SELE_LQ)
			numLQ = NORMAL_PAGE_NUM_LPQ_88E;

		/* NOTE: This step shall be proceed before writting REG_RQPN.		 */
		if (pHalData->OutEpQueueSel & TX_SELE_NQ)
			numNQ = NORMAL_PAGE_NUM_NPQ_88E;
	}

	value8 = (u8)_NPQ(numNQ);
	rtw_write8(padapter, REG_RQPN_NPQ, value8);

	numPubQ = TX_TOTAL_PAGE_NUMBER_88E(padapter) - numHQ - numLQ - numNQ;

	/* TX DMA */
	value32 = _HPQ(numHQ) | _LPQ(numLQ) | _PUBQ(numPubQ) | LD_RQPN;
	rtw_write32(padapter, REG_RQPN, value32);

	rtw_hal_set_sdio_tx_max_length(padapter, numHQ, numNQ, numLQ, numPubQ, SDIO_TX_DIV_NUM);

#ifdef CONFIG_SDIO_TX_ENABLE_AVAL_INT
	_init_available_page_threshold(padapter);
#endif
#endif
	return;
}

static void _InitTxBufferBoundary(PADAPTER padapter, u8 txpktbuf_bndy)
{
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	/* HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter); */

	/* u16	txdmactrl; */

	rtw_write8(padapter, REG_BCNQ_BDNY, txpktbuf_bndy);
	rtw_write8(padapter, REG_MGQ_BDNY, txpktbuf_bndy);
	rtw_write8(padapter, REG_WMAC_LBK_BF_HD, txpktbuf_bndy);
	rtw_write8(padapter, REG_TRXFF_BNDY, txpktbuf_bndy);
	rtw_write8(padapter, REG_TDECTRL + 1, txpktbuf_bndy);

}

static void
_InitNormalChipRegPriority(
		PADAPTER	Adapter,
		u16		beQ,
		u16		bkQ,
		u16		viQ,
		u16		voQ,
		u16		mgtQ,
		u16		hiQ
)
{
	u16 value16	= (rtw_read16(Adapter, REG_TRXDMA_CTRL) & 0x7);

	value16 |=	_TXDMA_BEQ_MAP(beQ)	| _TXDMA_BKQ_MAP(bkQ) |
			_TXDMA_VIQ_MAP(viQ)	| _TXDMA_VOQ_MAP(voQ) |
			_TXDMA_MGQ_MAP(mgtQ) | _TXDMA_HIQ_MAP(hiQ);

	rtw_write16(Adapter, REG_TRXDMA_CTRL, value16);
}

static void
_InitNormalChipOneOutEpPriority(
		PADAPTER Adapter
)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	u16	value = 0;
	switch (pHalData->OutEpQueueSel) {
	case TX_SELE_HQ:
		value = QUEUE_HIGH;
		break;
	case TX_SELE_LQ:
		value = QUEUE_LOW;
		break;
	case TX_SELE_NQ:
		value = QUEUE_NORMAL;
		break;
	default:
		/* RT_ASSERT(FALSE,("Shall not reach here!\n")); */
		break;
	}

	_InitNormalChipRegPriority(Adapter,
				   value,
				   value,
				   value,
				   value,
				   value,
				   value
				  );

}

static void
_InitNormalChipTwoOutEpPriority(
		PADAPTER Adapter
)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u16			beQ, bkQ, viQ, voQ, mgtQ, hiQ;


	u16	valueHi = 0;
	u16	valueLow = 0;

	switch (pHalData->OutEpQueueSel) {
	case (TX_SELE_HQ | TX_SELE_LQ):
		valueHi = QUEUE_HIGH;
		valueLow = QUEUE_LOW;
		break;
	case (TX_SELE_NQ | TX_SELE_LQ):
		valueHi = QUEUE_NORMAL;
		valueLow = QUEUE_LOW;
		break;
	case (TX_SELE_HQ | TX_SELE_NQ):
		valueHi = QUEUE_HIGH;
		valueLow = QUEUE_NORMAL;
		break;
	default:
		/* RT_ASSERT(FALSE,("Shall not reach here!\n")); */
		break;
	}

	if (!pregistrypriv->wifi_spec) {
		beQ		= valueLow;
		bkQ		= valueLow;
		viQ		= valueHi;
		voQ		= valueHi;
		mgtQ	= valueHi;
		hiQ		= valueHi;
	} else { /* for WMM ,CONFIG_OUT_EP_WIFI_MODE */
		beQ		= valueLow;
		bkQ		= valueHi;
		viQ		= valueHi;
		voQ		= valueLow;
		mgtQ	= valueHi;
		hiQ		= valueHi;
	}

	_InitNormalChipRegPriority(Adapter, beQ, bkQ, viQ, voQ, mgtQ, hiQ);

}

static void
_InitNormalChipThreeOutEpPriority(
		PADAPTER padapter
)
{
	struct registry_priv *pregistrypriv = &padapter->registrypriv;
	u16			beQ, bkQ, viQ, voQ, mgtQ, hiQ;

	if (!pregistrypriv->wifi_spec) { /* typical setting */
		beQ		= QUEUE_LOW;
		bkQ		= QUEUE_LOW;
		viQ		= QUEUE_NORMAL;
		voQ		= QUEUE_HIGH;
		mgtQ	= QUEUE_HIGH;
		hiQ		= QUEUE_HIGH;
	} else { /* for WMM */
		beQ		= QUEUE_LOW;
		bkQ		= QUEUE_NORMAL;
		viQ		= QUEUE_NORMAL;
		voQ		= QUEUE_HIGH;
		mgtQ	= QUEUE_HIGH;
		hiQ		= QUEUE_HIGH;
	}
	_InitNormalChipRegPriority(padapter, beQ, bkQ, viQ, voQ, mgtQ, hiQ);
}

static void
_InitNormalChipQueuePriority(
		PADAPTER Adapter
)
{
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);

	switch (pHalData->OutEpNumber) {
	case 1:
		_InitNormalChipOneOutEpPriority(Adapter);
		break;
	case 2:
		_InitNormalChipTwoOutEpPriority(Adapter);
		break;
	case 3:
		_InitNormalChipThreeOutEpPriority(Adapter);
		break;
	default:
		/* RT_ASSERT(FALSE,("Shall not reach here!\n")); */
		break;
	}


}


static void _InitQueuePriority(PADAPTER padapter)
{
	_InitNormalChipQueuePriority(padapter);
}

static void _InitPageBoundary(PADAPTER padapter)
{
	/* RX Page Boundary	 */
	u16 rxff_bndy = 0;

	rxff_bndy = MAX_RX_DMA_BUFFER_SIZE_88E(padapter) - 1;

	rtw_write16(padapter, (REG_TRXFF_BNDY + 2), rxff_bndy);

}

void _InitDriverInfoSize(PADAPTER padapter, u8 drvInfoSize)
{
	rtw_write8(padapter, REG_RX_DRVINFO_SZ, drvInfoSize);
}

void _InitNetworkType(PADAPTER padapter)
{
	u32 value32;

	value32 = rtw_read32(padapter, REG_CR);

	/* TODO: use the other function to set network type
	*	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AD_HOC); */
	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AP);

	rtw_write32(padapter, REG_CR, value32);
}

void _InitWMACSetting(PADAPTER padapter)
{
	u16 value16;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	u32 rcr;

	/* rcr = RCR_AAP | RCR_APM | RCR_AM | RCR_AB | RCR_CBSSID_DATA | RCR_CBSSID_BCN | RCR_AMF | RCR_HTC_LOC_CTRL | RCR_APP_PHYST_RXFF | RCR_APP_ICV | RCR_APP_MIC; */
	/* don't turn on AAP, it will allow all packets to driver */
	rcr = RCR_APM | RCR_AM | RCR_AB | RCR_CBSSID_DATA | RCR_CBSSID_BCN | RCR_AMF | RCR_HTC_LOC_CTRL | RCR_APP_PHYST_RXFF | RCR_APP_ICV | RCR_APP_MIC;
	rtw_hal_set_hwreg(padapter, HW_VAR_RCR, (u8 *)&rcr);

	/* Accept all data frames */
	value16 = 0xFFFF;
	rtw_write16(padapter, REG_RXFLTMAP2, value16);

	/* 2010.09.08 hpfan */
	/* Since ADF is removed from RCR, ps-poll will not be indicate to driver, */
	/* RxFilterMap should mask ps-poll to gurantee AP mode can rx ps-poll. */
	value16 = 0x400;
	rtw_write16(padapter, REG_RXFLTMAP1, value16);

	/* Accept all management frames */
	value16 = 0xFFFF;
	rtw_write16(padapter, REG_RXFLTMAP0, value16);

}

void _InitAdaptiveCtrl(PADAPTER padapter)
{
	u16	value16;
	u32	value32;

	/* Response Rate Set */
	value32 = rtw_read32(padapter, REG_RRSR);
	value32 &= ~RATE_BITMAP_ALL;
	value32 |= RATE_RRSR_CCK_ONLY_1M;

	rtw_phydm_set_rrsr(padapter, value32, TRUE);


	/* CF-END Threshold */
	/* m_spIoBase->rtw_write8(REG_CFEND_TH, 0x1); */

	/* SIFS (used in NAV) */
	value16 = _SPEC_SIFS_CCK(0x10) | _SPEC_SIFS_OFDM(0x10);
	rtw_write16(padapter, REG_SPEC_SIFS, value16);

	/* Retry Limit */
	value16 = BIT_LRL(RL_VAL_STA) | BIT_SRL(RL_VAL_STA);
	rtw_write16(padapter, REG_RETRY_LIMIT, value16);
}

void _InitEDCA(PADAPTER padapter)
{
	/* Set Spec SIFS (used in NAV) */
	rtw_write16(padapter, REG_SPEC_SIFS, 0x100a);
	rtw_write16(padapter, REG_MAC_SPEC_SIFS, 0x100a);

	/* Set SIFS for CCK */
	rtw_write16(padapter, REG_SIFS_CTX, 0x100a);

	/* Set SIFS for OFDM */
	rtw_write16(padapter, REG_SIFS_TRX, 0x100a);

	/* TXOP */
	rtw_write32(padapter, REG_EDCA_BE_PARAM, 0x005EA42B);
	rtw_write32(padapter, REG_EDCA_BK_PARAM, 0x0000A44F);
	rtw_write32(padapter, REG_EDCA_VI_PARAM, 0x005EA324);
	rtw_write32(padapter, REG_EDCA_VO_PARAM, 0x002FA226);
}

void _InitRetryFunction(PADAPTER padapter)
{
	u8	value8;

	value8 = rtw_read8(padapter, REG_FWHW_TXQ_CTRL);
	value8 |= EN_AMPDU_RTY_NEW;
	rtw_write8(padapter, REG_FWHW_TXQ_CTRL, value8);

	/* Set ACK timeout */
	rtw_write8(padapter, REG_ACKTO, 0x40);
}

static void HalRxAggr8188ESdio(PADAPTER padapter)
{
#if 1
	struct registry_priv *pregistrypriv;
	u8	valueDMATimeout;
	u8	valueDMAPageCount;


	pregistrypriv = &padapter->registrypriv;

	if (pregistrypriv->wifi_spec) {
		/* 2010.04.27 hpfan */
		/* Adjust RxAggrTimeout to close to zero disable RxAggr, suggested by designer */
		/* Timeout value is calculated by 34 / (2^n) */
		valueDMATimeout = 0x0f;
		valueDMAPageCount = 0x01;
	} else {
		valueDMATimeout = 0x06;
		/* valueDMAPageCount = 0x0F; */
		/* valueDMATimeout = 0x0a;  */
		valueDMAPageCount = 0x24;
	}

	rtw_write8(padapter, REG_RXDMA_AGG_PG_TH + 1, valueDMATimeout);
	rtw_write8(padapter, REG_RXDMA_AGG_PG_TH, valueDMAPageCount);
#endif
}

void sdio_AggSettingRxUpdate(PADAPTER padapter)
{
#if 1
	/* HAL_DATA_TYPE *pHalData; */
	u8 valueDMA;


	/* pHalData = GET_HAL_DATA(padapter); */

	valueDMA = rtw_read8(padapter, REG_TRXDMA_CTRL);
	valueDMA |= RXDMA_AGG_EN;
	rtw_write8(padapter, REG_TRXDMA_CTRL, valueDMA);

#if 0
	switch (RX_PAGE_SIZE_REG_VALUE) {
	case PBP_64:
		pHalData->HwRxPageSize = 64;
		break;
	case PBP_128:
		pHalData->HwRxPageSize = 128;
		break;
	case PBP_256:
		pHalData->HwRxPageSize = 256;
		break;
	case PBP_512:
		pHalData->HwRxPageSize = 512;
		break;
	case PBP_1024:
		pHalData->HwRxPageSize = 1024;
		break;
	default:
		break;
	}
#endif
#endif
}

void _initSdioAggregationSetting(PADAPTER padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	/* Tx aggregation setting */
	/* sdio_AggSettingTxUpdate(padapter); */

	/* Rx aggregation setting */
	HalRxAggr8188ESdio(padapter);
	sdio_AggSettingRxUpdate(padapter);

}

void _InitBeaconMaxError(PADAPTER padapter, BOOLEAN InfraMode)
{
#ifdef CONFIG_ADHOC_WORKAROUND_SETTING
	rtw_write8(padapter, REG_BCN_MAX_ERR, 0xFF);
#endif
}

void _InitInterrupt(PADAPTER padapter)
{

	/* HISR write one to clear */
	rtw_write32(padapter, REG_HISR_88E, 0xFFFFFFFF);

	/* HIMR - turn all off */
	rtw_write32(padapter, REG_HIMR_88E, 0);

	/*  */
	/* Initialize and enable SDIO Host Interrupt. */
	/*  */
	InitInterrupt8188ESdio(padapter);


	/*  */
	/* Initialize and enable system Host Interrupt. */
	/*  */
	/* InitSysInterrupt8188ESdio(Adapter); */ /* TODO: */

	/*  */
	/* Enable SDIO Host Interrupt. */
	/*  */
	/* EnableInterrupt8188ESdio(padapter); */ /* Move to sd_intf_start()/stop */

}

void _InitRDGSetting(PADAPTER padapter)
{
	rtw_write8(padapter, REG_RD_CTRL, 0xFF);
	rtw_write16(padapter, REG_RD_NAV_NXT, 0x200);
	rtw_write8(padapter, REG_RD_RESP_PKT_TH, 0x05);
}

static void _InitRFType(PADAPTER padapter)
{
	struct registry_priv *pregpriv = &padapter->registrypriv;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);

#if	DISABLE_BB_RF
	pHalData->rf_chip	= RF_PSEUDO_11N;
	return;
#endif
	pHalData->rf_chip	= RF_6052;

	RTW_INFO("Set RF Chip ID to RF_6052 and RF type to %d.\n", pHalData->rf_type);
}

/* Set CCK and OFDM Block "ON" */
static void _BBTurnOnBlock(PADAPTER padapter)
{
#if (DISABLE_BB_RF)
	return;
#endif

	phy_set_bb_reg(padapter, rFPGA0_RFMOD, bCCKEn, 0x1);
	phy_set_bb_reg(padapter, rFPGA0_RFMOD, bOFDMEn, 0x1);
}

static void _InitPABias(PADAPTER padapter)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(padapter);
	u8			pa_setting;

	/* FIXED PA current issue */
	/* efuse_one_byte_read(padapter, 0x1FA, &pa_setting); */
	efuse_OneByteRead(padapter, 0x1FA, &pa_setting, _FALSE);


	if (!(pa_setting & BIT0)) {
		phy_set_rf_reg(padapter, RF_PATH_A, 0x15, 0x0FFFFF, 0x0F406);
		phy_set_rf_reg(padapter, RF_PATH_A, 0x15, 0x0FFFFF, 0x4F406);
		phy_set_rf_reg(padapter, RF_PATH_A, 0x15, 0x0FFFFF, 0x8F406);
		phy_set_rf_reg(padapter, RF_PATH_A, 0x15, 0x0FFFFF, 0xCF406);
	}
	if (!(pa_setting & BIT4)) {
		pa_setting = rtw_read8(padapter, 0x16);
		pa_setting &= 0x0F;
		rtw_write8(padapter, 0x16, pa_setting | 0x80);
		rtw_write8(padapter, 0x16, pa_setting | 0x90);
	}
}

#if 0
void
_InitRDGSetting_8188E(
		PADAPTER Adapter
)
{
	PlatformEFIOWrite1Byte(Adapter, REG_RD_CTRL, 0xFF);
	PlatformEFIOWrite2Byte(Adapter, REG_RD_NAV_NXT, 0x200);
	PlatformEFIOWrite1Byte(Adapter, REG_RD_RESP_PKT_TH, 0x05);
}
#endif

static u32 rtl8188es_hal_init(PADAPTER padapter)
{
	s32 ret;
	u8	txpktbuf_bndy;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(padapter);
	struct dm_struct		*pDM_Odm = &pHalData->odmpriv;
	struct pwrctrl_priv		*pwrctrlpriv = adapter_to_pwrctl(padapter);
	struct registry_priv	*pregistrypriv = &padapter->registrypriv;
	rt_rf_power_state	eRfPowerStateToSet;
	u8 value8;
	u8 cpwm_orig, cpwm_now, rpwm;
	u16 value16;

	systime init_start_time = rtw_get_current_time();
	systime start_time;

#ifdef DBG_HAL_INIT_PROFILING
	enum HAL_INIT_STAGES {
		HAL_INIT_STAGES_BEGIN = 0,
		HAL_INIT_STAGES_INIT_PW_ON,
		HAL_INIT_STAGES_MISC01,
		HAL_INIT_STAGES_DOWNLOAD_FW,
		HAL_INIT_STAGES_MAC,
		HAL_INIT_STAGES_BB,
		HAL_INIT_STAGES_RF,
		HAL_INIT_STAGES_EFUSE_PATCH,
		HAL_INIT_STAGES_INIT_LLTT,

		HAL_INIT_STAGES_MISC02,
		HAL_INIT_STAGES_TURN_ON_BLOCK,
		HAL_INIT_STAGES_INIT_SECURITY,
		HAL_INIT_STAGES_MISC11,
		HAL_INIT_STAGES_INIT_HAL_DM,
		/* HAL_INIT_STAGES_RF_PS, */
		HAL_INIT_STAGES_IQK,
		HAL_INIT_STAGES_PW_TRACK,
		HAL_INIT_STAGES_LCK,
		/* HAL_INIT_STAGES_MISC21, */
		HAL_INIT_STAGES_INIT_PABIAS,
		/* HAL_INIT_STAGES_ANTENNA_SEL, */
		HAL_INIT_STAGES_MISC31,
		HAL_INIT_STAGES_END,
		HAL_INIT_STAGES_NUM
	};

	char *hal_init_stages_str[] = {
		"HAL_INIT_STAGES_BEGIN",
		"HAL_INIT_STAGES_INIT_PW_ON",
		"HAL_INIT_STAGES_MISC01",
		"HAL_INIT_STAGES_DOWNLOAD_FW",
		"HAL_INIT_STAGES_MAC",
		"HAL_INIT_STAGES_BB",
		"HAL_INIT_STAGES_RF",
		"HAL_INIT_STAGES_EFUSE_PATCH",
		"HAL_INIT_STAGES_INIT_LLTT",
		"HAL_INIT_STAGES_MISC02",
		"HAL_INIT_STAGES_TURN_ON_BLOCK",
		"HAL_INIT_STAGES_INIT_SECURITY",
		"HAL_INIT_STAGES_MISC11",
		"HAL_INIT_STAGES_INIT_HAL_DM",
		/* "HAL_INIT_STAGES_RF_PS", */
		"HAL_INIT_STAGES_IQK",
		"HAL_INIT_STAGES_PW_TRACK",
		"HAL_INIT_STAGES_LCK",
		/* "HAL_INIT_STAGES_MISC21", */
		"HAL_INIT_STAGES_INIT_PABIAS"
		/* "HAL_INIT_STAGES_ANTENNA_SEL", */
		"HAL_INIT_STAGES_MISC31",
		"HAL_INIT_STAGES_END",
	};


	int hal_init_profiling_i;
	systime hal_init_stages_timestamp[HAL_INIT_STAGES_NUM]; /* used to record the time of each stage's starting point */

	for (hal_init_profiling_i = 0; hal_init_profiling_i < HAL_INIT_STAGES_NUM; hal_init_profiling_i++)
		hal_init_stages_timestamp[hal_init_profiling_i] = 0;

#define HAL_INIT_PROFILE_TAG(stage) do { hal_init_stages_timestamp[(stage)] = rtw_get_current_time(); } while (0)
#else
#define HAL_INIT_PROFILE_TAG(stage) do {} while (0)
#endif /* DBG_HAL_INIT_PROFILING */

	RTW_INFO("+rtl8188es_hal_init\n");

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_BEGIN);
	/* Disable Interrupt first.
	*	rtw_hal_disable_interrupt(padapter);
	*	DisableInterrupt8188ESdio(padapter); */


	if (rtw_read8(padapter, REG_MCUFWDL) == 0xc6) {
#ifdef CONFIG_LPS_LCLK
		_enter_pwrlock(&pwrctrlpriv->lock);
		cpwm_orig = 0, rpwm = 0;
		rtw_hal_get_hwreg(padapter, HW_VAR_CPWM, &cpwm_orig);

		value8 = rtw_read8(padapter, SDIO_LOCAL_BASE | SDIO_REG_HRPWM1);
		value8 &= 0x80;
		value8 ^= BIT7;
		rpwm = PS_STATE_S4 | PS_ACK | value8;
		rtw_hal_set_hwreg(padapter, HW_VAR_SET_RPWM, (u8 *)(&rpwm));

		start_time = rtw_get_current_time();
		/* polling cpwm */
		do {
			rtw_mdelay_os(1);

			rtw_hal_get_hwreg(padapter, HW_VAR_CPWM, &cpwm_now);
			if ((cpwm_orig ^ cpwm_now) & 0x80) {
				RTW_INFO("%s:Leave LPS done before PowerOn!!\n",
					 __func__);
				break;
			}

			if (rtw_get_passing_time_ms(start_time) > LPS_RPWM_WAIT_MS) {
				if (rtw_read8(padapter, REG_CR) != 0xEA) {
					RTW_INFO("%s: polling cpwm timeout! but 0x100 != 0xEA!!\n",
						 __func__);
				} else {
					RTW_INFO("%s, polling cpwm timeout and 0x100 = 0xEA!!\n",
						 __func__);
				}
				break;
			}
		} while (1);
		_exit_pwrlock(&pwrctrlpriv->lock);

		rtw_hal_power_off(padapter);
#endif
	} else
		RTW_INFO("FW does not exit before power on!!\n");

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_INIT_PW_ON);
	ret = rtw_hal_power_on(padapter);
	if (_FAIL == ret) {
		goto exit;
	}

	ret = sdio_power_on_check(padapter);

	if (_FAIL == ret) {
		RTW_INFO("Power on Fail! do it again\n");
		ret = rtw_hal_power_on(padapter);
		if (_FAIL == ret) {
			RTW_INFO("Failed to init Power On!\n");
			goto exit;
		}
	}
	RTW_INFO("Power on ok!\n");

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_MISC01);
	if (!pregistrypriv->wifi_spec)
		txpktbuf_bndy = TX_PAGE_BOUNDARY_88E(padapter);
	else {
		/* for WMM */
		txpktbuf_bndy = WMM_NORMAL_TX_PAGE_BOUNDARY_88E(padapter);
	}
	_InitQueueReservedPage(padapter);
	_InitQueuePriority(padapter);
	_InitPageBoundary(padapter);
	_InitTransferPageSize(padapter);

#ifdef CONFIG_IOL_IOREG_CFG
	_InitTxBufferBoundary(padapter, 0);
#endif
	/*  */
	/* Configure SDIO TxRx Control to enable Rx DMA timer masking. */
	/* 2010.02.24. */
	/*  */
	value8 = SdioLocalCmd52Read1Byte(padapter, SDIO_REG_TX_CTRL);
	SdioLocalCmd52Write1Byte(padapter, SDIO_REG_TX_CTRL, 0x02);

	rtw_write8(padapter, SDIO_LOCAL_BASE | SDIO_REG_HRPWM1, 0);

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_DOWNLOAD_FW);
	if (padapter->registrypriv.mp_mode == 0) {
		ret = rtl8188e_FirmwareDownload(padapter, _FALSE);
		if (ret != _SUCCESS) {
			RTW_INFO("%s: Download Firmware failed!!\n", __func__);
			pHalData->bFWReady = _FALSE;
			pHalData->fw_ractrl = _FALSE;
			goto exit;
		} else {
			pHalData->bFWReady = _TRUE;
#ifdef CONFIG_SFW_SUPPORTED
			pHalData->fw_ractrl = IS_VENDOR_8188E_I_CUT_SERIES(padapter) ? _TRUE : _FALSE;
#else
			pHalData->fw_ractrl = _FALSE;
#endif
		}
	}

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_MAC);
#if (HAL_MAC_ENABLE == 1)
	ret = PHY_MACConfig8188E(padapter);
	if (ret != _SUCCESS) {
		goto exit;
	}
#endif

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_BB);
	/*  */
	/* d. Initialize BB related configurations. */
	/*  */
#if (HAL_BB_ENABLE == 1)
	ret = PHY_BBConfig8188E(padapter);
	if (ret != _SUCCESS) {
		goto exit;
	}
#endif


	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_RF);

#if (HAL_RF_ENABLE == 1)
	ret = PHY_RFConfig8188E(padapter);

	if (ret != _SUCCESS) {
		goto exit;
	}
	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_EFUSE_PATCH);
#if defined(CONFIG_IOL_EFUSE_PATCH)
	ret = rtl8188e_iol_efuse_patch(padapter);
	if (ret != _SUCCESS) {
		RTW_INFO("%s  rtl8188e_iol_efuse_patch failed\n", __FUNCTION__);
		goto exit;
	}
#endif
	_InitTxBufferBoundary(padapter, txpktbuf_bndy);
	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_INIT_LLTT);
	ret = InitLLTTable(padapter, txpktbuf_bndy);
	if (_SUCCESS != ret) {
		goto exit;
	}

	/* Enable TX Report & Tx Report Timer   */
	value8 = rtw_read8(padapter, REG_TX_RPT_CTRL);
	rtw_write8(padapter,  REG_TX_RPT_CTRL, (value8 | BIT1 | BIT0));

#if (RATE_ADAPTIVE_SUPPORT == 1)
	if (!pHalData->fw_ractrl) {
		/* Set MAX RPT MACID */
		rtw_write8(padapter,  REG_TX_RPT_CTRL + 1, 2); /* FOR sta mode ,0: bc/mc ,1:AP */
		/* Tx RPT Timer. Unit: 32us */
		rtw_write16(padapter, REG_TX_RPT_TIME, 0xCdf0);
	} else
#endif
	{
		/* disable tx rpt */
		rtw_write8(padapter,  REG_TX_RPT_CTRL + 1, 0); /* FOR sta mode ,0: bc/mc ,1:AP */
	}

#if 0
	if (pHTInfo->bRDGEnable)
		_InitRDGSetting_8188E(Adapter);
#endif

#ifdef CONFIG_TX_EARLY_MODE
	if (pHalData->bEarlyModeEnable) {

		value8 = rtw_read8(padapter, REG_EARLY_MODE_CONTROL);
#if RTL8188E_EARLY_MODE_PKT_NUM_10 == 1
		value8 = value8 | 0x1f;
#else
		value8 = value8 | 0xf;
#endif
		rtw_write8(padapter, REG_EARLY_MODE_CONTROL, value8);

		rtw_write8(padapter, REG_EARLY_MODE_CONTROL + 3, 0x80);

		value8 = rtw_read8(padapter, REG_TCR + 1);
		value8 = value8 | 0x40;
		rtw_write8(padapter, REG_TCR + 1, value8);
	} else
#endif
	{
		rtw_write8(padapter, REG_EARLY_MODE_CONTROL, 0);
	}


#if (SIC_ENABLE == 1)
	SIC_Init(padapter);
#endif


	if (pwrctrlpriv->reg_rfoff == _TRUE)
		pwrctrlpriv->rf_pwrstate = rf_off;

	/* 2010/08/09 MH We need to check if we need to turnon or off RF after detecting */
	/* HW GPIO pin. Before PHY_RFConfig8192C. */
	HalDetectPwrDownMode88E(padapter);


	/* Set RF type for BB/RF configuration */
	_InitRFType(padapter);

	/* Save target channel */
	/* <Roger_Notes> Current Channel will be updated again later. */
	pHalData->current_channel = 1;



	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_MISC02);
	/* Get Rx PHY status in order to report RSSI and others. */
	_InitDriverInfoSize(padapter, 4);
	_InitNetworkType(padapter);
	_InitWMACSetting(padapter);
	_InitAdaptiveCtrl(padapter);
	_InitEDCA(padapter);
	_InitRetryFunction(padapter);
	_initSdioAggregationSetting(padapter);
	InitBeaconParameters_8188e(padapter);
	_InitBeaconMaxError(padapter, _TRUE);
	_InitInterrupt(padapter);

	/* Enable MACTXEN/MACRXEN block */
	value16 = rtw_read16(padapter, REG_CR);
	value16 |= (MACTXEN | MACRXEN);
	rtw_write8(padapter, REG_CR, value16);

	rtw_write32(padapter, REG_MACID_NO_LINK_0, 0xFFFFFFFF);
	rtw_write32(padapter, REG_MACID_NO_LINK_1, 0xFFFFFFFF);

#endif /* HAL_RF_ENABLE == 1 */


	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_TURN_ON_BLOCK);
	_BBTurnOnBlock(padapter);

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_INIT_SECURITY);
#if 1
	invalidate_cam_all(padapter);
#else
	CamResetAllEntry(padapter);
	padapter->hal_func.EnableHWSecCfgHandler(padapter);
#endif

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_MISC11);
	/* 2010/12/17 MH We need to set TX power according to EFUSE content at first. */
	rtw_hal_set_tx_power_level(padapter, pHalData->current_channel);

	/*  */
	/* Disable BAR, suggested by Scott */
	/* 2010.04.09 add by hpfan */
	/*  */
	rtw_write32(padapter, REG_BAR_MODE_CTRL, 0x0201ffff);

	/* HW SEQ CTRL */
	/* set 0x0 to 0xFF by tynli. Default enable HW SEQ NUM. */
	rtw_write8(padapter, REG_HWSEQ_CTRL, 0xFF);


#ifdef RTL8188ES_MAC_LOOPBACK
	value8 = rtw_read8(padapter, REG_SYS_FUNC_EN);
	value8 &= ~(FEN_BBRSTB | FEN_BB_GLB_RSTn);
	rtw_write8(padapter, REG_SYS_FUNC_EN, value8);/* disable BB, CCK/OFDM */

	rtw_write8(padapter, REG_RD_CTRL, 0x0F);
	rtw_write8(padapter, REG_RD_CTRL + 1, 0xCF);
	/* rtw_write8(padapter, REG_TXPKTBUF_WMAC_LBK_BF_HD, 0x80); */ /* to check _InitPageBoundary() */
	rtw_write32(padapter, REG_CR, 0x0b0202ff);/* 0x100[28:24]=0x01011, enable mac loopback, no HW Security Eng. */
#endif


	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_INIT_HAL_DM);
	/*	InitHalDm(padapter); */
	rtl8188e_InitHalDm(padapter);


#if (MP_DRIVER == 1)
	if (padapter->registrypriv.mp_mode == 1) {
		padapter->mppriv.channel = pHalData->current_channel;
		MPT_InitializeAdapter(padapter, padapter->mppriv.channel);
	} else
#endif /* (MP_DRIVER == 1) */
	{
		/*  */
		/* 2010/08/11 MH Merge from 8192SE for Minicard init. We need to confirm current radio status */
		/* and then decide to enable RF or not.!!!??? For Selective suspend mode. We may not */
		/* call init_adapter. May cause some problem?? */
		/*  */
		/* Fix the bug that Hw/Sw radio off before S3/S4, the RF off action will not be executed */
		/* in MgntActSet_RF_State() after wake up, because the value of pHalData->eRFPowerState */
		/* is the same as eRfOff, we should change it to eRfOn after we config RF parameters. */
		/* Added by tynli. 2010.03.30. */
		pwrctrlpriv->rf_pwrstate = rf_on;

		/* 20100326 Joseph: Copy from GPIOChangeRFWorkItemCallBack() function to check HW radio on/off. */
		/* 20100329 Joseph: Revise and integrate the HW/SW radio off code in initialization.
		*	pHalData->bHwRadioOff = _FALSE; */
		pwrctrlpriv->b_hw_radio_off = _FALSE;
		eRfPowerStateToSet = rf_on;

		/* 2010/-8/09 MH For power down module, we need to enable register block contrl reg at 0x1c. */
		/* Then enable power down control bit of register 0x04 BIT4 and BIT15 as 1. */
		if (pHalData->pwrdown && eRfPowerStateToSet == rf_off) {
			/* Enable register area 0x0-0xc. */
			rtw_write8(padapter, REG_RSV_CTRL, 0x0);

			/*  */
			/* <Roger_Notes> We should configure HW PDn source for WiFi ONLY, and then */
			/* our HW will be set in power-down mode if PDn source from all  functions are configured. */
			/* 2010.10.06. */
			/*  */

			rtw_write16(padapter, REG_APS_FSMCO, 0x8812);

		}
		/* DrvIFIndicateCurrentPhyStatus(padapter); */ /* 2010/08/17 MH Disable to prevent BSOD. */

		/* 2010/08/26 MH Merge from 8192CE. */
		if (pwrctrlpriv->rf_pwrstate == rf_on) {

			HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_IQK);
			pHalData->neediqk_24g = _TRUE;
			/*		dm_check_txpowertracking(padapter);
			 *		phy_lc_calibrate(padapter); */

			HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_PW_TRACK);
			odm_txpowertracking_check(&pHalData->odmpriv);

			HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_LCK);
			/*phy_lc_calibrate_8188e(&pHalData->odmpriv);*/
			halrf_lck_trigger(&pHalData->odmpriv);

		}
	}

	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_INIT_PABIAS);
	/* if(pHalData->eRFPowerState == eRfOn) */
	{
		_InitPABias(padapter);
	}

	/* Init BT hw config.
	*	HALBT_InitHwConfig(padapter); */


	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_MISC31);
	/* 2010/05/20 MH We need to init timer after update setting. Otherwise, we can not get correct inf setting. */
	/* 2010/05/18 MH For SE series only now. Init GPIO detect time */
#if 0
	if (pDevice->RegUsbSS) {
		GpioDetectTimerStart(padapter);	/* Disable temporarily */
	}
#endif

	/* 2010/12/17 MH For TX power level OID modification from UI.
	*	padapter->hal_func.GetTxPowerLevelHandler( padapter, &pHalData->DefaultTxPwrDbm ); */
	/* dbg_print("pHalData->DefaultTxPwrDbm = %d\n", pHalData->DefaultTxPwrDbm); */

	/* if(pHalData->SwBeaconType < HAL92CSDIO_DEFAULT_BEACON_TYPE) */ /* The lowest Beacon Type that HW can support */
	/*		pHalData->SwBeaconType = HAL92CSDIO_DEFAULT_BEACON_TYPE; */

	/*  */
	/* Update current Tx FIFO page status. */
	/*  */
	HalQueryTxBufferStatus8189ESdio(padapter);
	HalQueryTxOQTBufferStatus8189ESdio(padapter);
	pHalData->SdioTxOQTMaxFreeSpace = pHalData->SdioTxOQTFreeSpace;
	PHY_SetRFEReg_8188E(padapter);

	if (pregistrypriv->wifi_spec) {
		rtw_write16(padapter, REG_FAST_EDCA_CTRL , 0);
		rtw_write8(padapter, REG_NAV_UPPER , 0x0);
	}

	if (IS_HARDWARE_TYPE_8188ES(padapter)) {
		value8 = rtw_read8(padapter, 0x4d3);
		rtw_write8(padapter, 0x4d3, (value8 | 0x1));
	}

	/* pHalData->PreRpwmVal = PlatformEFSdioLocalCmd52Read1Byte(Adapter, SDIO_REG_HRPWM1)&0x80; */

	if (!pHalData->fw_ractrl) {
		/* enable Tx report. */
		rtw_write8(padapter,  REG_FWHW_TXQ_CTRL + 1, 0x0F);
		/* tynli_test_tx_report. */
		rtw_write16(padapter, REG_TX_RPT_TIME, 0x3DF0);
	}
#if 0
	/*  Suggested by SD1 pisa. Added by tynli. 2011.10.21. */
	PlatformEFIOWrite1Byte(Adapter, REG_EARLY_MODE_CONTROL + 3, 0x01);

#endif



	/* enable tx DMA to drop the redundate data of packet */
	rtw_write16(padapter, REG_TXDMA_OFFSET_CHK, (rtw_read16(padapter, REG_TXDMA_OFFSET_CHK) | DROP_DATA_EN));

	/* #debug print for checking compile flags */
	/* RTW_INFO("RTL8188E_FPGA_TRUE_PHY_VERIFICATION=%d\n", RTL8188E_FPGA_TRUE_PHY_VERIFICATION); */
	RTW_INFO("DISABLE_BB_RF=%d\n", DISABLE_BB_RF);
	RTW_INFO("IS_HARDWARE_TYPE_8188ES=%d\n", IS_HARDWARE_TYPE_8188ES(padapter));
	/* # */

#ifdef CONFIG_PLATFORM_SPRD
	/* For Power Consumption, set all GPIO pin to ouput mode */
	/* 0x44~0x47 (GPIO 0~7), Note:GPIO5 is enabled for controlling external 26MHz request */
	rtw_write8(padapter, GPIO_IO_SEL, 0xFF);/* Reg0x46, set to o/p mode */

	/* 0x42~0x43 (GPIO 8~11) */
	value8 = rtw_read8(padapter, REG_GPIO_IO_SEL);
	rtw_write8(padapter, REG_GPIO_IO_SEL, (value8 << 4) | value8);
	value8 = rtw_read8(padapter, REG_GPIO_IO_SEL + 1);
	rtw_write8(padapter, REG_GPIO_IO_SEL + 1, value8 | 0x0F); /* Reg0x43 */
#endif /* CONFIG_PLATFORM_SPRD */


#ifdef CONFIG_XMIT_ACK
	/* ack for xmit mgmt frames. */
	rtw_write32(padapter, REG_FWHW_TXQ_CTRL, rtw_read32(padapter, REG_FWHW_TXQ_CTRL) | BIT(12));
#endif /* CONFIG_XMIT_ACK */

	if (padapter->registrypriv.wifi_spec == 1)
		odm_set_bb_reg(pDM_Odm,
			     rOFDM0_ECCAThreshold, bMaskDWord, 0x00fe0301);

	RTW_INFO("-rtl8188es_hal_init\n");

exit:
	HAL_INIT_PROFILE_TAG(HAL_INIT_STAGES_END);

	RTW_INFO("%s in %dms\n", __FUNCTION__, rtw_get_passing_time_ms(init_start_time));

#ifdef DBG_HAL_INIT_PROFILING
	hal_init_stages_timestamp[HAL_INIT_STAGES_END] = rtw_get_current_time();

	for (hal_init_profiling_i = 0; hal_init_profiling_i < HAL_INIT_STAGES_NUM - 1; hal_init_profiling_i++) {
		RTW_INFO("DBG_HAL_INIT_PROFILING: %35s, %u, %5u, %5u\n"
			 , hal_init_stages_str[hal_init_profiling_i]
			 , hal_init_stages_timestamp[hal_init_profiling_i]
			, (hal_init_stages_timestamp[hal_init_profiling_i + 1] - hal_init_stages_timestamp[hal_init_profiling_i])
			, rtw_get_time_interval_ms(hal_init_stages_timestamp[hal_init_profiling_i], hal_init_stages_timestamp[hal_init_profiling_i + 1])
			);
	}
#endif

	return ret;

}



static u32 rtl8188es_hal_deinit(PADAPTER padapter)
{
	RTW_INFO("=>%s\n", __FUNCTION__);

	if (rtw_is_hw_init_completed(padapter))
		rtw_hal_power_off(padapter);

	RTW_INFO("<=%s\n", __FUNCTION__);

	return _SUCCESS;
}

static void rtl8188es_init_default_value(PADAPTER padapter)
{
	PHAL_DATA_TYPE pHalData;
	struct pwrctrl_priv *pwrctrlpriv;
	u8 i;

	pHalData = GET_HAL_DATA(padapter);
	pwrctrlpriv = adapter_to_pwrctl(padapter);


	/* init default value */
	pHalData->fw_ractrl = _FALSE;
	if (!pwrctrlpriv->bkeepfwalive)
		pHalData->LastHMEBoxNum = 0;

	/* init dm default value */
	pHalData->bIQKInitialized = _FALSE;

	/* interface related variable */
	pHalData->SdioRxFIFOCnt = 0;
	pHalData->EfuseHal.fakeEfuseBank = 0;
	pHalData->EfuseHal.fakeEfuseUsedBytes = 0;
	_rtw_memset(pHalData->EfuseHal.fakeEfuseContent, 0xFF, EFUSE_MAX_HW_SIZE);
	_rtw_memset(pHalData->EfuseHal.fakeEfuseInitMap, 0xFF, EFUSE_MAX_MAP_LEN);
	_rtw_memset(pHalData->EfuseHal.fakeEfuseModifiedMap, 0xFF, EFUSE_MAX_MAP_LEN);

}

/*
 *	Description:
 *		We should set Efuse cell selection to WiFi cell in default.
 *
 *	Assumption:
 *		PASSIVE_LEVEL
 *
 *	Added by Roger, 2010.11.23.
 *   */
#if 0
static void _EfuseCellSel(
		PADAPTER	padapter
)
{
	/* HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter); */

	u32			value32;

	/* if(INCLUDE_MULTI_FUNC_BT(padapter)) */
	{
		value32 = rtw_read32(padapter, EFUSE_TEST);
		value32 = (value32 & ~EFUSE_SEL_MASK) | EFUSE_SEL(EFUSE_WIFI_SEL_0);
		rtw_write32(padapter, EFUSE_TEST, value32);
	}
}
#endif

static void
_ReadRFType(
		PADAPTER	Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

#if DISABLE_BB_RF
	pHalData->rf_chip = RF_PSEUDO_11N;
#else
	pHalData->rf_chip = RF_6052;
#endif
}

static void
Hal_EfuseParsePIDVID_8188ES(
		PADAPTER		pAdapter,
		u8			*hwinfo,
		BOOLEAN			AutoLoadFail
)
{
	/*	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter); */

	/*  */
	/* <Roger_Notes> The PID/VID info was parsed from CISTPL_MANFID Tuple in CIS area before. */
	/* VID is parsed from Manufacture code field and PID is parsed from Manufacture information field. */
	/* 2011.04.01. */
	/*  */

}

static void
readAdapterInfo_8188ES(
	PADAPTER			padapter
)
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);

	/* parse the eeprom/efuse content */
	Hal_EfuseParseIDCode88E(padapter, pHalData->efuse_eeprom_data);
	Hal_EfuseParsePIDVID_8188ES(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	hal_config_macaddr(padapter, pHalData->bautoload_fail_flag);
	Hal_ReadPowerSavingMode88E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	Hal_ReadTxPowerInfo88E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	Hal_EfuseParseEEPROMVer88E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	rtl8188e_EfuseParseChnlPlan(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	Hal_EfuseParseXtal_8188E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	Hal_EfuseParseCustomerID88E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	/* Hal_ReadAntennaDiversity88E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag); */
	Hal_ReadPAType_8188E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	Hal_ReadAmplifierType_8188E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	Hal_ReadRFEType_8188E(padapter, pHalData->efuse_eeprom_data,  pHalData->bautoload_fail_flag);
	Hal_EfuseParseBoardType88E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	Hal_ReadThermalMeter_88E(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
	/*  */
	/* The following part initialize some vars by PG info. */
	/*  */
#if defined(CONFIG_WOWLAN) || defined(CONFIG_AP_WOWLAN)
	Hal_DetectWoWMode(padapter);
#endif
#ifdef CONFIG_RF_POWER_TRIM
	Hal_ReadRFGainOffset(padapter, pHalData->efuse_eeprom_data, pHalData->bautoload_fail_flag);
#endif	/*CONFIG_RF_POWER_TRIM*/
}

static void _ReadPROMContent(
	PADAPTER		padapter
)
{
	PHAL_DATA_TYPE pHalData = GET_HAL_DATA(padapter);
	u8			eeValue;

	/* check system boot selection */
	eeValue = rtw_read8(padapter, REG_9346CR);
	pHalData->EepromOrEfuse = (eeValue & BOOT_FROM_EEPROM) ? _TRUE : _FALSE;
	pHalData->bautoload_fail_flag = (eeValue & EEPROM_EN) ? _FALSE : _TRUE;

	RTW_INFO("%s: 9346CR=0x%02X, Boot from %s, Autoload %s\n",
		 __FUNCTION__, eeValue,
		 (pHalData->EepromOrEfuse ? "EEPROM" : "EFUSE"),
		 (pHalData->bautoload_fail_flag ? "Fail" : "OK"));

	/*	pHalData->EEType = IS_BOOT_FROM_EEPROM(Adapter) ? EEPROM_93C46 : EEPROM_BOOT_EFUSE; */



	Hal_InitPGData88E(padapter);
	readAdapterInfo_8188ES(padapter);
}

/*
 *	Description:
 *		Read HW adapter information by E-Fuse or EEPROM according CR9346 reported.
 *
 *	Assumption:
 *		PASSIVE_LEVEL (SDIO interface)
 *
 *   */
static u8 ReadAdapterInfo8188ES(PADAPTER padapter)
{
	/* Read EEPROM size before call any EEPROM function */
	padapter->EepromAddressSize = GetEEPROMSize8188E(padapter);

	/*_EfuseCellSel(padapter); */

	_ReadRFType(padapter);/* rf_chip->_InitRFType() */
	_ReadPROMContent(padapter);

	return _SUCCESS;
}

static u8 SetHwReg8188ES(PADAPTER Adapter, u8 variable, u8 *val)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8 ret = _SUCCESS;


	switch (variable) {
	case HW_VAR_RXDMA_AGG_PG_TH:
		break;

	case HW_VAR_SET_RPWM:
#ifdef CONFIG_LPS_LCLK
		{
			u8	ps_state = *((u8 *)val);
			/* rpwm value only use BIT0(clock bit) ,BIT6(Ack bit), and BIT7(Toggle bit) for 88e. */
			/* BIT0 value - 1: 32k, 0:40MHz. */
			/* BIT6 value - 1: report cpwm value after success set, 0:do not report. */
			/* BIT7 value - Toggle bit change. */
			/* modify by Thomas. 2012/4/2. */
			ps_state = ps_state & 0xC1;

#ifdef CONFIG_EXT_CLK /* for sprd */
			if (ps_state & BIT(6)) { /* want to leave 32k */
				/* enable ext clock req before leave LPS-32K */
				/* RTW_INFO("enable ext clock req before leaving LPS-32K\n");					 */
				EnableGpio5ClockReq(Adapter, _FALSE, 1);
			}
#endif /* CONFIG_EXT_CLK */

			/* RTW_INFO("##### Change RPWM value to = %x for switch clk #####\n",ps_state); */
			rtw_write8(Adapter, SDIO_LOCAL_BASE | SDIO_REG_HRPWM1, ps_state);
		}
#endif
		break;
	default:
		ret = SetHwReg8188E(Adapter, variable, val);
		break;
	}

	return ret;
}

static void GetHwReg8188ES(PADAPTER padapter, u8 variable, u8 *val)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(padapter);

	switch (variable) {
	case HW_VAR_CPWM:
		*val = rtw_read8(padapter, SDIO_LOCAL_BASE | SDIO_REG_HCPWM1);
		break;
	default:
		GetHwReg8188E(padapter, variable, val);
		break;
	}

}

/*
 *	Description:
 *		Query setting of specified variable.
 *   */
u8
GetHalDefVar8188ESDIO(
		PADAPTER				Adapter,
		HAL_DEF_VARIABLE		eVariable,
		void						*pValue
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			bResult = _SUCCESS;

	switch (eVariable) {
	case HW_VAR_MAX_RX_AMPDU_FACTOR:
		*((u32 *)pValue) = MAX_AMPDU_FACTOR_16K;
		break;

	case HAL_DEF_TX_LDPC:
	case HAL_DEF_RX_LDPC:
		*((u8 *)pValue) = _FALSE;
		break;
	case HAL_DEF_RX_STBC:
		*((u8 *)pValue) = 1;
		break;
	default:
		bResult = GetHalDefVar8188E(Adapter, eVariable, pValue);
		break;
	}

	return bResult;
}




/*
 *	Description:
 *		Change default setting of specified variable.
 *   */
u8
SetHalDefVar8188ESDIO(
		PADAPTER				Adapter,
		HAL_DEF_VARIABLE		eVariable,
		void						*pValue
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			bResult = _TRUE;

	switch (eVariable) {
	default:
		bResult = SetHalDefVar(Adapter, eVariable, pValue);
		break;
	}

	return bResult;
}

void rtl8188es_set_hal_ops(PADAPTER padapter)
{
	struct hal_ops *pHalFunc = &padapter->hal_func;


	pHalFunc->hal_power_on = _InitPowerOn_8188ES;
	pHalFunc->hal_power_off = hal_poweroff_8188es;

	pHalFunc->hal_init = &rtl8188es_hal_init;
	pHalFunc->hal_deinit = &rtl8188es_hal_deinit;

	pHalFunc->init_xmit_priv = &rtl8188es_init_xmit_priv;
	pHalFunc->free_xmit_priv = &rtl8188es_free_xmit_priv;

	pHalFunc->init_recv_priv = &rtl8188es_init_recv_priv;
	pHalFunc->free_recv_priv = &rtl8188es_free_recv_priv;
#ifdef CONFIG_RTW_SW_LED
	pHalFunc->InitSwLeds = &rtl8188es_InitSwLeds;
	pHalFunc->DeInitSwLeds = &rtl8188es_DeInitSwLeds;
#endif
	pHalFunc->init_default_value = &rtl8188es_init_default_value;
	pHalFunc->intf_chip_configure = &rtl8188es_interface_configure;
	pHalFunc->read_adapter_info = &ReadAdapterInfo8188ES;

	pHalFunc->enable_interrupt = &EnableInterrupt8188ESdio;
	pHalFunc->disable_interrupt = &DisableInterrupt8188ESdio;
	pHalFunc->check_ips_status = &CheckIPSStatus;

#if defined(CONFIG_WOWLAN) || defined(CONFIG_AP_WOWLAN)
	pHalFunc->clear_interrupt = &ClearInterrupt8188ESdio;
#endif
	pHalFunc->set_hw_reg_handler = &SetHwReg8188ES;
	pHalFunc->GetHwRegHandler = &GetHwReg8188ES;

	pHalFunc->get_hal_def_var_handler = &GetHalDefVar8188ESDIO;
	pHalFunc->SetHalDefVarHandler = &SetHalDefVar8188ESDIO;

	pHalFunc->SetBeaconRelatedRegistersHandler = &SetBeaconRelatedRegisters8188E;

	pHalFunc->hal_xmit = &rtl8188es_hal_xmit;
	pHalFunc->mgnt_xmit = &rtl8188es_mgnt_xmit;
	pHalFunc->hal_xmitframe_enqueue = &rtl8188es_hal_xmitframe_enqueue;

#ifdef CONFIG_HOSTAPD_MLME
	pHalFunc->hostap_mgnt_xmit_entry = NULL;
#endif
#ifdef CONFIG_XMIT_THREAD_MODE
	pHalFunc->xmit_thread_handler = &rtl8188es_xmit_buf_handler;
#endif
	rtl8188e_set_hal_ops(pHalFunc);

}

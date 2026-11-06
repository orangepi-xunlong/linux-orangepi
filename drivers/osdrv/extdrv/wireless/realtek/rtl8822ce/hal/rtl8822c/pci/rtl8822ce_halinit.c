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

#define _RTL8822CE_HALINIT_C_
#include <drv_types.h>          /* PADAPTER, basic_types.h and etc. */
#include <hal_data.h>		/* HAL_DATA_TYPE */
#include "../../hal_halmac.h"	/* HALMAC API */
#include "../rtl8822c.h"
#include "rtl8822ce.h"

#ifdef CONFIG_FWLPS_IN_IPS
u8 rtl8822ce_fw_ips_init(_adapter *padapter)
{
	struct sreset_priv *psrtpriv = &GET_HAL_DATA(padapter)->srestpriv;
	struct debug_priv *pdbgpriv = &adapter_to_dvobj(padapter)->drv_dbg;
	struct pwrctrl_priv *pwrctl = adapter_to_pwrctl(padapter);
	systime start_time;
	u8 cpwm_orig, cpwm_now, rpwm;
	u8 bMacPwrCtrlOn = _TRUE;

	if ((pwrctl->bips_processing == _FALSE)
	    || (psrtpriv->silent_reset_inprogress == _TRUE)
	    || (GET_HAL_DATA(padapter)->bFWReady == _FALSE)
	    || (pwrctl->pre_ips_type != 0))
		return _FAIL;

	RTW_INFO("%s: Leaving FW_IPS\n", __func__);

#ifdef CONFIG_LPS_LCLK
	/* for polling cpwm */
	cpwm_orig = 0;
	rtw_hal_get_hwreg(padapter, HW_VAR_CPWM, &cpwm_orig);

	/* set rpwm */
	rtw_hal_get_hwreg(padapter, HW_VAR_RPWM_TOG, &rpwm);
	rpwm += 0x80;
	rpwm |= PS_ACK;
	rtw_hal_set_hwreg(padapter, HW_VAR_SET_RPWM, (u8 *)(&rpwm));


	RTW_INFO("%s: write rpwm=%02x\n", __func__, rpwm);

	pwrctl->tog = (rpwm + 0x80) & 0x80;

	/* do polling cpwm */
	start_time = rtw_get_current_time();
	do {
		rtw_mdelay_os(1);

		rtw_hal_get_hwreg(padapter, HW_VAR_CPWM, &cpwm_now);
		if ((cpwm_orig ^ cpwm_now) & 0x80) {
#ifdef DBG_CHECK_FW_PS_STATE
			RTW_INFO("%s: polling cpwm ok when leaving IPS in FWLPS state,"
				 " cost %d ms,"
				 " cpwm_orig=0x%02x, cpwm_now=0x%02x, 0x100=0x%x\n",
				 __FUNCTION__,
				 rtw_get_passing_time_ms(start_time),
				 cpwm_orig, cpwm_now, rtw_read8(padapter, REG_CR_8822C));
#endif /* DBG_CHECK_FW_PS_STATE */
			break;
		}

		if (rtw_get_passing_time_ms(start_time) > 100) {
			RTW_ERR("%s: polling cpwm timeout when leaving IPS in FWLPS state\n", __FUNCTION__);
			break;
		}
	} while (1);
#endif /* CONFIG_LPS_LCLK */

	rtl8822c_set_FwPwrModeInIPS_cmd(padapter, 0);

	rtw_hal_set_hwreg(padapter, HW_VAR_APFM_ON_MAC, &bMacPwrCtrlOn);

#ifdef CONFIG_LPS_LCLK
#ifdef DBG_CHECK_FW_PS_STATE
	if (rtw_fw_ps_state(padapter) == _FAIL) {
		RTW_INFO("after hal init, fw ps state in 32k\n");
		pdbgpriv->dbg_ips_drvopen_fail_cnt++;
	}
#endif /* DBG_CHECK_FW_PS_STATE */
#endif /* CONFIG_LPS_LCLK */

	return _SUCCESS;
}

u8 rtl8822ce_fw_ips_deinit(_adapter *padapter)
{
	struct sreset_priv *psrtpriv =  &GET_HAL_DATA(padapter)->srestpriv;
	struct debug_priv *pdbgpriv = &adapter_to_dvobj(padapter)->drv_dbg;
	struct pwrctrl_priv *pwrctl = adapter_to_pwrctl(padapter);
	systime start_time;
	int cnt = 0;
	u8 val8 = 0, rpwm;

	if ((pwrctl->bips_processing == _FALSE)
	    || (psrtpriv->silent_reset_inprogress == _TRUE)
	    || (GET_HAL_DATA(padapter)->bFWReady == _FALSE)
	    || (padapter->netif_up == _FALSE)) {
		pdbgpriv->dbg_carddisable_cnt++;
		pwrctl->pre_ips_type = 1;

		return _FAIL;
	}

	RTW_INFO("%s: issue H2C to FW when entering IPS\n", __func__);
	rtl8822c_set_FwPwrModeInIPS_cmd(padapter, 0x1);

#ifdef CONFIG_LPS_LCLK
	/*
	 * poll 0x1cc to make sure H2C command already finished by FW;
	 * MAC_0x1cc=0 means H2C done by FW.
	 */
	start_time = rtw_get_current_time();
	do {
		rtw_mdelay_os(10);
		val8 = rtw_read8(padapter, REG_HMETFR_8822C);
		cnt++;
		if (!val8)
			break;

		if (rtw_get_passing_time_ms(start_time) > 100) {
			RTW_ERR("%s: fail to wait H2C, REG_HMETFR=0x%x, cnt=%d\n",
				__FUNCTION__, val8, cnt);
#ifdef DBG_CHECK_FW_PS_STATE
			RTW_WARN("MAC_1C0=0x%08x, MAC_1C4=0x%08x, MAC_1C8=0x%08x, MAC_1CC=0x%08x\n",
				 rtw_read32(padapter, 0x1c0), rtw_read32(padapter, 0x1c4),
				 rtw_read32(padapter, 0x1c8), rtw_read32(padapter, REG_HMETFR_8822C));
#endif /* DBG_CHECK_FW_PS_STATE */
			goto exit;
		}
	} while (1);

	/* H2C done, enter 32k */
	/* set rpwm to enter 32k */
	rtw_hal_get_hwreg(padapter, HW_VAR_RPWM_TOG, &rpwm);
	rpwm += 0x80;
	rpwm |= PS_STATE_S0;
	rtw_hal_set_hwreg(padapter, HW_VAR_SET_RPWM, (u8 *)(&rpwm));
	RTW_INFO("%s: write rpwm=%02x\n", __func__, rpwm);
	pwrctl->tog = (val8 + 0x80) & 0x80;

	cnt = val8 = 0;
	start_time = rtw_get_current_time();
	do {
		val8 = rtw_read8(padapter, REG_CR_8822C);
		cnt++;
		RTW_INFO("%s: polling 0x100=0x%x, cnt=%d\n",
			 __FUNCTION__, val8, cnt);
		if (val8 == 0xEA) {
			RTW_INFO("%s: polling 0x100=0xEA, cnt=%d, cost %d ms\n",
				 __FUNCTION__, cnt,
				 rtw_get_passing_time_ms(start_time));
			break;
		}

		if (rtw_get_passing_time_ms(start_time) > 100) {
			RTW_ERR("%s: polling polling 0x100=0xEA timeout! cnt=%d\n",
				__FUNCTION__, cnt);
#ifdef DBG_CHECK_FW_PS_STATE
			RTW_WARN("MAC_1C0=0x%08x, MAC_1C4=0x%08x, MAC_1C8=0x%08x, MAC_1CC=0x%08x\n",
				 rtw_read32(padapter, 0x1c0), rtw_read32(padapter, 0x1c4),
				 rtw_read32(padapter, 0x1c8), rtw_read32(padapter, REG_HMETFR_8822C));
#endif /* DBG_CHECK_FW_PS_STATE */
			break;
		}

		rtw_mdelay_os(10);
	} while (1);

exit:
	RTW_INFO("polling done when entering IPS, check result: 0x100=0x%02x, cnt=%d, MAC_1cc=0x%02x\n",
		 rtw_read8(padapter, REG_CR_8822C), cnt, rtw_read8(padapter, REG_HMETFR_8822C));
#endif /* CONFIG_LPS_LCLK */

	pwrctl->pre_ips_type = 0;

	return _SUCCESS;

}

#endif /* CONFIG_FWLPS_IN_IPS */

u32 InitMAC_TRXBD_8822CE(PADAPTER Adapter)
{
	u8 tmpU1b;
	u16 tmpU2b;
	u32 tmpU4b;
	int q_idx;
	struct recv_priv *precvpriv = &Adapter->recvpriv;
	struct xmit_priv *pxmitpriv = &Adapter->xmitpriv;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	RTW_INFO("=======>InitMAC_TXBD_8822CE()\n");

	/*
	 * Set CMD TX BD (buffer descriptor) physical address(from OS API).
	 */
	rtw_write32(Adapter, REG_H2CQ_TXBD_DESA_8822C,
		    (u64)pxmitpriv->tx_ring[TXCMD_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_H2CQ_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE_CMD | ((RTL8822CE_SEG_NUM << 12) &
					 0x3000));

#ifdef CONFIG_64BIT_DMA
	rtw_write32(Adapter, REG_H2CQ_TXBD_DESA_8822C + 4,
		    ((u64)pxmitpriv->tx_ring[TXCMD_QUEUE_INX].dma) >> 32);
#endif
	/*
	 * Set TX/RX BD (buffer descriptor) physical address(from OS API).
	 */
	rtw_write32(Adapter, REG_BCNQ_TXBD_DESA_8822C,
		    (u64)pxmitpriv->tx_ring[BCN_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_MGQ_TXBD_DESA_8822C,
		    (u64)pxmitpriv->tx_ring[MGT_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_VOQ_TXBD_DESA_8822C,
		    (u64)pxmitpriv->tx_ring[VO_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_VIQ_TXBD_DESA_8822C,
		    (u64)pxmitpriv->tx_ring[VI_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_BEQ_TXBD_DESA_8822C,
		    (u64)pxmitpriv->tx_ring[BE_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));

	/* vincent sync windows */
	tmpU4b = rtw_read32(Adapter, REG_BEQ_TXBD_DESA_8822C);

	rtw_write32(Adapter, REG_BKQ_TXBD_DESA_8822C,
		    (u64)pxmitpriv->tx_ring[BK_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_HI0Q_TXBD_DESA_8822C,
		    (u64)pxmitpriv->tx_ring[HIGH_QUEUE_INX].dma &
		    DMA_BIT_MASK(32));
	rtw_write32(Adapter, REG_RXQ_RXBD_DESA_8822C,
		    (u64)precvpriv->rx_ring[RX_MPDU_QUEUE].dma &
		    DMA_BIT_MASK(32));

#ifdef CONFIG_64BIT_DMA
	/*
	 * 2009/10/28 MH For DMA 64 bits. We need to assign the high
	 * 32 bit address for NIC HW to transmit data to correct path.
	 */
	rtw_write32(Adapter, REG_BCNQ_TXBD_DESA_8822C + 4,
		    ((u64)pxmitpriv->tx_ring[BCN_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_MGQ_TXBD_DESA_8822C + 4,
		    ((u64)pxmitpriv->tx_ring[MGT_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_VOQ_TXBD_DESA_8822C + 4,
		    ((u64)pxmitpriv->tx_ring[VO_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_VIQ_TXBD_DESA_8822C + 4,
		    ((u64)pxmitpriv->tx_ring[VI_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_BEQ_TXBD_DESA_8822C + 4,
		    ((u64)pxmitpriv->tx_ring[BE_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_BKQ_TXBD_DESA_8822C + 4,
		    ((u64)pxmitpriv->tx_ring[BK_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_HI0Q_TXBD_DESA_8822C + 4,
		    ((u64)pxmitpriv->tx_ring[HIGH_QUEUE_INX].dma) >> 32);
	rtw_write32(Adapter, REG_RXQ_RXBD_DESA_8822C + 4,
		    ((u64)precvpriv->rx_ring[RX_MPDU_QUEUE].dma) >> 32);
#endif

	/* pci buffer descriptor mode: Reset the Read/Write point to 0 */
	PlatformEFIOWrite4Byte(Adapter, REG_TSFTIMER_HCI_8822C, 0x3fffffff);

	/* Reset the H2CQ R/W point index to 0 */
	tmpU4b = rtw_read32(Adapter, REG_H2CQ_CSR_8822C);
	rtw_write32(Adapter, REG_H2CQ_CSR_8822C, (tmpU4b | BIT8 | BIT16));

	tmpU1b = rtw_read8(Adapter, REG_PCIE_CTRL + 3);
	rtw_write8(Adapter, REG_PCIE_CTRL + 3, (tmpU1b | 0xF7));

	/* 20100318 Joseph: Reset interrupt migration setting
	 * when initialization. Suggested by SD1. */
	rtw_write32(Adapter, REG_INT_MIG, 0);
	pHalData->bInterruptMigration = _FALSE;

	/* 2009.10.19. Reset H2C protection register. by tynli. */
	rtw_write32(Adapter, REG_MCUTST_I_8822C, 0x0);

#if MP_DRIVER == 1
	if (Adapter->registrypriv.mp_mode == 1) {
		rtw_write32(Adapter, REG_MACID, 0x87654321);
		rtw_write32(Adapter, 0x0700, 0x87654321);
	}
#endif

	/* pic buffer descriptor mode: */
	/* ---- tx */
	rtw_write16(Adapter, REG_MGQ_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_VOQ_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_VIQ_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_BEQ_TXBD_NUM_8822C,
		    TX_BD_NUM_BEQ_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_BKQ_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI0Q_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI1Q_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI2Q_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI3Q_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI4Q_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI5Q_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI6Q_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));
	rtw_write16(Adapter, REG_HI7Q_TXBD_NUM_8822C,
		    TX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 12) & 0x3000));

#ifdef CONFIG_64BIT_DMA
	rtw_write16(Adapter, REG_RX_RXBD_NUM_8822C,
		    RX_BD_NUM_8822CE |((RTL8822CE_SEG_NUM<<13 ) & 0x6000) |
		    0x8000);
#else
	rtw_write16(Adapter, REG_RX_RXBD_NUM_8822C,
		    RX_BD_NUM_8822CE | ((RTL8822CE_SEG_NUM << 13) & 0x6000));
#endif
	/* reset read/write point */
	rtw_write32(Adapter, REG_TSFTIMER_HCI_8822C, 0XFFFFFFFF);

#if 1 /* vincent windows */
	/* Start debug mode */
	{
		u8 reg0x3f3 = 0;

		reg0x3f3 = rtw_read8(Adapter, 0x3f3);
		rtw_write8(Adapter, 0x3f3, reg0x3f3 | BIT2);
	}

	{
		/* Need to disable BT coex to let MP tool Tx, this would be done in FW
		 * in the future, suggest by ChunChu, 2015.05.19
		 */

		u8 tmp1Byte;
		u16 tmp2Byte;
		u32 tmp4Byte;

		tmp2Byte = rtw_read16(Adapter, REG_SYS_FUNC_EN_8822C);
		rtw_write16(Adapter, REG_SYS_FUNC_EN_8822C, tmp2Byte | BIT10);
		tmp1Byte = rtw_read8(Adapter, REG_DIS_TXREQ_CLR_8822C);
		rtw_write8(Adapter, REG_DIS_TXREQ_CLR_8822C, tmp1Byte | BIT7);
		tmp4Byte = rtw_read32(Adapter, 0x1080);
		rtw_write32(Adapter, 0x1080, tmp4Byte | BIT16);
	}
#endif

	RTW_INFO("InitMAC_TXBD_8822CE() <====\n");

	return _SUCCESS;
}

#ifdef CONFIG_RTW_LED
static void init_hwled(PADAPTER adapter, u8 enable)
{
	u8 mode = 0;
	struct led_priv *ledpriv = adapter_to_led(adapter);

	if (ledpriv->LedStrategy != HW_LED)
		return;

	rtw_halmac_led_cfg(adapter_to_dvobj(adapter), enable, mode);
}
#endif /* CONFIG_RTW_LED */

static void hal_init_misc(PADAPTER adapter)
{
#ifdef CONFIG_RTW_LED
	struct led_priv *ledpriv = adapter_to_led(adapter);
#ifdef CONFIG_SW_LED
	pledpriv->bRegUseLed = _TRUE;
	ledpriv->LedStrategy = SW_LED_MODE1;
#else /* HW LED */
	ledpriv->LedStrategy = HW_LED;
#endif /* CONFIG_SW_LED */
	init_hwled(adapter, 1);
#endif
}

u32 rtl8822ce_init(PADAPTER padapter)
{
	u8 ok = _TRUE;
	u8 val8;
	PHAL_DATA_TYPE hal;
	struct registry_priv  *registry_par = &padapter->registrypriv;

	hal = GET_HAL_DATA(padapter);

#if 0
	/*Only reset HW Ring*/
	InitMAC_TRXBD_8822CE(padapter);
#else
	/*Both reset HW and SW Ring.
	 *For FW IPS, only HW Ring is resetted. It cause Tx DMA errors.
	 *Both reset HW/SW Ring here can fix errors.
	 */
	rtl8822ce_reset_bd(padapter);
#endif

#ifdef CONFIG_FWLPS_IN_IPS
	if (_SUCCESS == rtl8822ce_fw_ips_init(padapter)) {
		RTW_INFO("%s(%d) ooo fw_ips_init init success\n",__func__,__LINE__);
		return _SUCCESS;
	}
#endif

	ok = rtl8822c_hal_init(padapter);
	if (_FALSE == ok)
		return _FAIL;

#if defined(USING_RX_TAG)
	/* have to init after halmac init */
	val8 = rtw_read8(padapter, REG_PCIE_CTRL_8822C + 2);
	rtw_write8(padapter, REG_PCIE_CTRL_8822C + 2, (val8 | BIT4));
	rtw_write16(padapter, REG_PCIE_CTRL_8822C, 0x8000);
#else
	rtw_write16(padapter, REG_PCIE_CTRL_8822C, 0x0000);
#endif

	rtw_write8(padapter, REG_RX_DRVINFO_SZ_8822C, 0x4);

	rtl8822c_phy_init_haldm(padapter);
#ifdef CONFIG_BEAMFORMING
	rtl8822c_phy_bf_init(padapter);
#endif

#ifdef CONFIG_FW_MULTI_PORT_SUPPORT
	/*HW /FW init*/
	rtw_hal_set_default_port_id_cmd(padapter, 0);
#endif

#ifdef CONFIG_BT_COEXIST
	/* Init BT hw config. */
	if (hal->EEPROMBluetoothCoexist == _TRUE) {
		rtw_btcoex_HAL_Initialize(padapter, _FALSE);
		#ifdef CONFIG_FW_MULTI_PORT_SUPPORT
		rtw_hal_set_wifi_btc_port_id_cmd(padapter);
		#endif
	} else
#endif /* CONFIG_BT_COEXIST */
		rtw_btcoex_wifionly_hw_config(padapter);

	hal->pci_backdoor_ctrl = registry_par->pci_aspm_config;

	rtw_pci_aspm_config(padapter);

	rtl8822c_init_misc(padapter);
	hal_init_misc(padapter);

#ifdef CONFIG_USB_CONFIG_OFFLOAD_8822C
	/* Check HCI Configuration if MIX mode, BT-USB, WiFi-PCIe */
	/* 3b'010 is WiFi-PCIe + BT-UART. 3b'011 is WiFi-PCIe + BT-USB */
	if (0x3 == ((rtw_read32(padapter, REG_SYS_STATUS1_8822C)>>12)&0x7))
		rtl8822c_set_usb_suspend_mode(padapter);
#endif

#ifdef CONFIG_8822CE_INT_MIGRATION 
	/* TX interrupt migration - 3pkts or 7*64=448us */
	rtw_write32(padapter, REG_INT_MIG_8822C, 0x03070000);
#endif
	return _SUCCESS;
}

void rtl8822ce_init_default_value(PADAPTER padapter)
{
	PHAL_DATA_TYPE pHalData;


	pHalData = GET_HAL_DATA(padapter);

	rtl8822c_init_default_value(padapter);

	/* interface related variable */
	pHalData->CurrentWirelessMode = WIRELESS_MODE_AUTO;
	pHalData->bDefaultAntenna = 1;
	pHalData->TransmitConfig = BIT_CFEND_FORMAT | BIT_WMAC_TCR_ERRSTEN_3;

	/* Set RCR-Receive Control Register .
	 * The value is set in InitializeAdapter8190Pci().
	 */
	pHalData->ReceiveConfig = (
#ifdef CONFIG_RX_PACKET_APPEND_FCS
					  BIT_APP_FCS		|
#endif
					  BIT_APP_MIC		|
					  BIT_APP_ICV		|
					  BIT_APP_PHYSTS		|
					  BIT_VHT_DACK		|
					  BIT_HTC_LOC_CTRL	|
					  /* BIT_AMF		| */
					  BIT_CBSSID_DATA		|
					  BIT_CBSSID_BCN		|
					  /* BIT_ACF		| */
					  /* BIT_ADF		| */ /* PS-Poll filter */
					  BIT_AB			|
					  BIT_AB			|
					  BIT_APM			|
					  0);

	/*
	 * Set default value of Interrupt Mask Register0
	 */
	pHalData->IntrMaskDefault[0] = (u32)(
					       BIT(29)			| /* BIT_PSTIMEOUT */
					       BIT(27)			| /* BIT_GTINT3 */
					       BIT_TXBCN0ERR_MSK	|
					       BIT_TXBCN0OK_MSK	|
					       BIT_BCNDMAINT0_MSK	|
					       BIT_HSISR_IND_ON_INT_MSK |
					       BIT_C2HCMD_MSK		|
			#ifdef CONFIG_LPS_LCLK
						BIT_CPWM_MSK		|
			#endif
			#ifndef CONFIG_PCI_TX_POLLING
					       BIT_HIGHDOK_MSK		|
					       BIT_MGTDOK_MSK		|
					       BIT_BKDOK_MSK		|
					       BIT_BEDOK_MSK		|
					       BIT_VIDOK_MSK		|
					       BIT_VODOK_MSK		|
			#endif
					       BIT_RDU_MSK		|
					       BIT_RXOK_MSK		|
					       0);

	/*
	 * Set default value of Interrupt Mask Register1
	 */
	pHalData->IntrMaskDefault[1] = (u32)(
					       BIT(9)		| /* TXFOVW */
					       BIT_FOVW_MSK	|
					       0);

	/*
	 * Set default value of Interrupt Mask Register3
	 */
	pHalData->IntrMaskDefault[3] = (u32)(
					       BIT(2)| /*PCIE TX DMA Stuck mask*/
					       BIT(3)| /*PCIE RX DMA Stuck mask*/
				       BIT_SETH2CDOK_MASK	| /* H2C_TX_OK */
					       0);

	/* 2012/03/27 hpfan Add for win8 DTM DPC ISR test */
	pHalData->IntrMaskReg[0] = (u32)(
					   BIT_RDU_MSK	|
					   BIT(29)		| /* BIT_PSTIMEOUT */
					   0);

	pHalData->IntrMaskReg[1] = (u32)(
					   BIT_C2HCMD_MSK	|
					   0);

	pHalData->IntrMask[0] = pHalData->IntrMaskDefault[0];
	pHalData->IntrMask[1] = pHalData->IntrMaskDefault[1];
	pHalData->IntrMask[3] = pHalData->IntrMaskDefault[3];

}

static void hal_deinit_misc(PADAPTER adapter)
{
#ifdef CONFIG_RTW_LED
	struct led_priv *ledpriv = adapter_to_led(adapter);

	init_hwled(adapter, 0);
#ifdef CONFIG_RTW_SW_LED
	if (ledpriv->bRegUseLed == _TRUE)
		rtw_halmac_led_cfg(adapter_to_dvobj(adapter), _FALSE, 3);
#endif
#endif /* CONFIG_RTW_LED */
}

u32 rtl8822ce_deinit(PADAPTER padapter)
{
	struct pwrctrl_priv *pwrctl = adapter_to_pwrctl(padapter);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct dvobj_priv *pobj_priv = adapter_to_dvobj(padapter);
	u8 status = _TRUE;

	RTW_INFO("==> %s\n", __func__);

#ifdef CONFIG_FWLPS_IN_IPS
	if (_SUCCESS == rtl8822ce_fw_ips_deinit(padapter))
		goto exit;
#endif

	hal_deinit_misc(padapter);
	status = rtl8822c_deinit(padapter);
	if (status == _FALSE) {
		RTW_INFO("%s: rtl8822c_hal_deinit fail\n", __func__);
		return _FAIL;
	}

#ifdef CONFIG_FWLPS_IN_IPS
exit:
#endif
	RTW_INFO("%s <==\n", __func__);
	return _SUCCESS;
}



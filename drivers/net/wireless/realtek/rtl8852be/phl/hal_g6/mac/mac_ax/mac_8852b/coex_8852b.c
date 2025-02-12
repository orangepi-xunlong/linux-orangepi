/******************************************************************************
 *
 * Copyright(c) 2019 Realtek Corporation. All rights reserved.
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
 ******************************************************************************/
#include "coex_8852b.h"
#include "../../mac_reg.h"
#include "../hw.h"

#if MAC_AX_8852B_SUPPORT

#define MAC_AX_RTK_RATE 5

#define MAC_AX_BT_MODE_0_3 0
#define MAC_AX_BT_MODE_2 2

#define MAC_AX_CSR_DELAY 0
#define MAC_AX_CSR_PRI_TO 5
#define MAC_AX_CSR_TRX_TO 4

#define MAC_AX_CSR_RATE 80

#define MAC_AX_SB_DRV_MSK 0xFFFFFF
#define MAC_AX_SB_DRV_SH 0
#define MAC_AX_SB_FW_MSK 0x7F
#define MAC_AX_SB_FW_SH 24

#define MAC_AX_BTGS1_NOTIFY BIT(0)

u32 coex_mac_init_8852b(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 ret = mac_write_lte_8852b(adapter, R_AX_LTECOEX_CTRL, 0);
	u8 val = MAC_REG_R8(R_AX_SYS_SDIO_CTRL + 3);

	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Write LTE REG fail\n");
		return ret;
	}

	ret = mac_write_lte_8852b(adapter, R_AX_LTECOEX_CTRL_2, 0);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Write LTE REG fail\n");
		return ret;
	}

	MAC_REG_W8(R_AX_SYS_SDIO_CTRL + 3, val | BIT(2));

	return MACSUCCESS;
}

u32 mac_write_lte_8852b(struct mac_ax_adapter *adapter,
			const u32 offset, u32 val)
{
	u32 cnt;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
#if MAC_USB_IO_ACC_ON
	struct mac_ax_ops *mac_ops = adapter_to_mac_ops(adapter);
	u32 ret, ofldcap = 0;

	if (adapter->sm.fwdl == MAC_AX_FWDL_INIT_RDY) {
		ret = mac_ops->get_hw_value(adapter, MAC_AX_HW_GET_FW_CAP, &ofldcap);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("Get MAC_AX_HW_GET_FW_CAP fail %d\n", ret);
			return ret;
		}
		if (ofldcap) {
			ret = MAC_REG_P_OFLD(R_AX_LTE_CTRL, B_AX_LTE_RDY, 1, 0);
			if (ret != MACSUCCESS)
				return ret;

			ret = MAC_REG_W32_OFLD(R_AX_LTE_WDATA, val, 0);
			if (ret != MACSUCCESS)
				return ret;

			ret = MAC_REG_W32_OFLD(R_AX_LTE_CTRL, 0xC00F0000 | offset, 1);
			if (ret != MACSUCCESS)
				return ret;

			return MACSUCCESS;
		} else {
			cnt = 1000;
			while ((MAC_REG_R8(R_AX_LTE_CTRL + 3) & BIT(5)) == 0) {
				if (cnt == 0) {
					PLTFM_MSG_ERR("[ERR]lte not ready(W)\n");
					return MACPOLLTO;
				}
				cnt--;
				PLTFM_DELAY_US(50);
			}

			PLTFM_MUTEX_LOCK(&adapter->hw_info->lte_rlock);

			MAC_REG_W32(R_AX_LTE_WDATA, val);
			MAC_REG_W32(R_AX_LTE_CTRL, 0xC00F0000 | offset);

			PLTFM_MUTEX_UNLOCK(&adapter->hw_info->lte_rlock);

			return MACSUCCESS;
		}
	}
#endif
	cnt = 1000;
	while ((MAC_REG_R8(R_AX_LTE_CTRL + 3) & BIT(5)) == 0) {
		if (cnt == 0) {
			PLTFM_MSG_ERR("[ERR]lte not ready(W)\n");
			return MACPOLLTO;
		}
		cnt--;
		PLTFM_DELAY_US(50);
	}

	PLTFM_MUTEX_LOCK(&adapter->hw_info->lte_rlock);

	MAC_REG_W32(R_AX_LTE_WDATA, val);
	MAC_REG_W32(R_AX_LTE_CTRL, 0xC00F0000 | offset);

	PLTFM_MUTEX_UNLOCK(&adapter->hw_info->lte_rlock);

	return MACSUCCESS;
}

u32 mac_read_lte_8852b(struct mac_ax_adapter *adapter,
		       const u32 offset, u32 *val)
{
	u32 cnt;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	cnt = 1000;
	while ((MAC_REG_R8(R_AX_LTE_CTRL + 3) & BIT(5)) == 0) {
		if (cnt == 0) {
			PLTFM_MSG_ERR("[ERR]lte not ready(W)\n");
			break;
		}
		cnt--;
		PLTFM_DELAY_US(50);
	}

	PLTFM_MUTEX_LOCK(&adapter->hw_info->lte_rlock);

	MAC_REG_W32(R_AX_LTE_CTRL, 0x800F0000 | offset);
	*val = MAC_REG_R32(R_AX_LTE_RDATA);

	PLTFM_MUTEX_UNLOCK(&adapter->hw_info->lte_rlock);

	return MACSUCCESS;
}

void _patch_hi_pri_resp_tx_8852b(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u8 val;

	val = MAC_REG_R8(R_AX_TRXPTCL_RESP_0 + 3);
	MAC_REG_W8(R_AX_TRXPTCL_RESP_0 + 3, val & ~BIT(1));
}

u32 mac_coex_init_8852b(struct mac_ax_adapter *adapter,
			struct mac_ax_coex *coex)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u8 val;
	u16 val16;
	u32 ret, val32;

	val = MAC_REG_R8(R_AX_GPIO_MUXCFG);
	MAC_REG_W8(R_AX_GPIO_MUXCFG, val | B_AX_ENBT);

	switch (coex->direction) {
	case MAC_AX_COEX_INNER:
		val = MAC_REG_R8(R_AX_GPIO_MUXCFG + 1);
		val = (val & ~BIT(2)) | BIT(1);
		MAC_REG_W8(R_AX_GPIO_MUXCFG + 1, val);
		break;
	case MAC_AX_COEX_OUTPUT:
		val = MAC_REG_R8(R_AX_GPIO_MUXCFG + 1);
		val = val | BIT(1) | BIT(0);
		MAC_REG_W8(R_AX_GPIO_MUXCFG + 1, val);
		break;
	case MAC_AX_COEX_INPUT:
		val = MAC_REG_R8(R_AX_GPIO_MUXCFG + 1);
		val = val & ~(BIT(2) | BIT(1));
		MAC_REG_W8(R_AX_GPIO_MUXCFG + 1, val);
		break;
	default:
		return MACNOITEM;
	}

	val = MAC_REG_R8(R_AX_BT_COEX_CFG_2 + 1);
	MAC_REG_W8(R_AX_BT_COEX_CFG_2 + 1, val | BIT(0));

	val = MAC_REG_R8(R_AX_CSR_MODE);
	MAC_REG_W8(R_AX_CSR_MODE, val | B_AX_STATIS_BT_EN | B_AX_WL_ACT_MSK);

	val = MAC_REG_R8(R_AX_CSR_MODE + 2);
	MAC_REG_W8(R_AX_CSR_MODE + 2, val | BIT(0));

	if (chk_patch_hi_pri_resp_tx(adapter))
		_patch_hi_pri_resp_tx_8852b(adapter);

	val16 = MAC_REG_R16(R_AX_CCA_CFG_0);
	val16 = (val16 | B_AX_BTCCA_EN) & ~B_AX_BTCCA_BRK_TXOP_EN;
	MAC_REG_W16(R_AX_CCA_CFG_0, val16);

	ret = mac_read_lte_8852b(adapter, R_AX_LTE_SW_CFG_2, &val32);
	if (ret) {
		PLTFM_MSG_ERR("%s: Read LTE fail!\n", __func__);
		return ret;
	}
	val32 = val32 & B_AX_WL_RX_CTRL;
	ret = mac_write_lte_8852b(adapter, R_AX_LTE_SW_CFG_2, val32);
	if (ret) {
		PLTFM_MSG_ERR("%s: Write LTE fail!\n", __func__);
		return ret;
	}

	switch (coex->pta_mode) {
	case MAC_AX_COEX_RTK_MODE:
		val = MAC_REG_R8(R_AX_GPIO_MUXCFG);
		val = SET_CLR_WORD(val, MAC_AX_BT_MODE_0_3,
				   B_AX_BTMODE);
		MAC_REG_W8(R_AX_GPIO_MUXCFG, val);

		val = MAC_REG_R8(R_AX_TDMA_MODE);
		MAC_REG_W8(R_AX_TDMA_MODE, val | B_AX_RTK_BT_ENABLE);

		val = MAC_REG_R8(R_AX_BT_COEX_CFG_5);
		val = SET_CLR_WORD(val, MAC_AX_RTK_RATE,
				   B_AX_BT_RPT_SAMPLE_RATE);
		MAC_REG_W8(R_AX_BT_COEX_CFG_5, val);
		break;
	case MAC_AX_COEX_CSR_MODE:
		val = MAC_REG_R8(R_AX_GPIO_MUXCFG);
		val = SET_CLR_WORD(val, MAC_AX_BT_MODE_2, B_AX_BTMODE);
		MAC_REG_W8(R_AX_GPIO_MUXCFG, val);

		val16 = MAC_REG_R16(R_AX_CSR_MODE);
		val16 = SET_CLR_WORD(val16, MAC_AX_CSR_PRI_TO,
				     B_AX_BT_PRI_DETECT_TO);
		val16 = SET_CLR_WORD(val16, MAC_AX_CSR_TRX_TO,
				     B_AX_BT_TRX_INIT_DETECT);
		val16 = SET_CLR_WORD(val16, MAC_AX_CSR_DELAY,
				     B_AX_BT_STAT_DELAY);
		val16 = val16 | B_AX_ENHANCED_BT;
		MAC_REG_W16(R_AX_CSR_MODE, val16);

		MAC_REG_W8(R_AX_BT_COEX_CFG_2, MAC_AX_CSR_RATE);
		break;
	default:
		return MACNOITEM;
	}

	return MACSUCCESS;
}

u32 mac_get_gnt_8852b(struct mac_ax_adapter *adapter,
		      struct mac_ax_coex_gnt *gnt_cfg)
{
	u32 val, ret, status;
	struct mac_ax_gnt *gnt;

	ret = mac_read_lte_8852b(adapter, R_AX_LTE_SW_CFG_1, &val);
	if (ret) {
		PLTFM_MSG_ERR("Read LTE fail!\n");
		return ret;
	}

	ret = mac_read_lte_8852b(adapter, R_AX_LTECOEX_STATUS, &status);
	if (ret) {
		PLTFM_MSG_ERR("Read LTE fail!\n");
		return ret;
	}

	gnt = &gnt_cfg->band0;
	gnt->gnt_bt_sw_en = !!(val & B_AX_GNT_BT_RFC_S0_SW_CTRL);
	gnt->gnt_bt = !!(status & B_AX_GNT_BT_RFC_S0_STA);
	gnt->gnt_wl_sw_en = !!(val & B_AX_GNT_WL_RFC_S0_SW_CTRL);
	gnt->gnt_wl = !!(status & B_AX_GNT_WL_RFC_S0_STA);

	gnt = &gnt_cfg->band1;
	gnt->gnt_bt_sw_en = !!(val & B_AX_GNT_BT_RFC_S1_SW_CTRL);
	gnt->gnt_bt = !!(status & B_AX_GNT_BT_RFC_S1_STA);
	gnt->gnt_wl_sw_en = !!(val & B_AX_GNT_WL_RFC_S1_SW_CTRL);
	gnt->gnt_wl = !!(status & B_AX_GNT_WL_RFC_S1_STA);

	return MACSUCCESS;
}

u32 mac_cfg_gnt_8852b(struct mac_ax_adapter *adapter,
		      struct mac_ax_coex_gnt *gnt_cfg)
{
	u32 val, ret;

	ret = mac_read_lte_8852b(adapter, R_AX_LTE_SW_CFG_1, &val);
	if (ret) {
		PLTFM_MSG_ERR("Read LTE fail!\n");
		return ret;
	}
	val = (gnt_cfg->band0.gnt_bt ? (B_AX_GNT_BT_RFC_S0_SW_VAL |
		 B_AX_GNT_BT_BB_S0_SW_VAL) : 0) |
		(gnt_cfg->band0.gnt_bt_sw_en ?
		 (B_AX_GNT_BT_RFC_S0_SW_CTRL |
		  B_AX_GNT_BT_BB_S0_SW_CTRL) : 0) |
		(gnt_cfg->band0.gnt_wl ? (B_AX_GNT_WL_RFC_S0_SW_VAL |
					  B_AX_GNT_WL_BB_S0_SW_VAL) : 0) |
		(gnt_cfg->band0.gnt_wl_sw_en ?
		 (B_AX_GNT_WL_RFC_S0_SW_CTRL |
		  B_AX_GNT_WL_BB_S0_SW_CTRL) : 0) |
		(gnt_cfg->band1.gnt_bt ? (B_AX_GNT_BT_RFC_S1_SW_VAL |
					  B_AX_GNT_BT_BB_S1_SW_VAL) : 0) |
		(gnt_cfg->band1.gnt_bt_sw_en ?
		 (B_AX_GNT_BT_RFC_S1_SW_CTRL |
		  B_AX_GNT_BT_BB_S1_SW_CTRL) : 0) |
		(gnt_cfg->band1.gnt_wl ? (B_AX_GNT_WL_RFC_S1_SW_VAL |
					  B_AX_GNT_WL_BB_S1_SW_VAL) : 0) |
		(gnt_cfg->band1.gnt_wl_sw_en ?
		 (B_AX_GNT_WL_RFC_S1_SW_CTRL |
		  B_AX_GNT_WL_BB_S1_SW_CTRL) : 0);
	ret = mac_write_lte_8852b(adapter, R_AX_LTE_SW_CFG_1, val);
	if (ret) {
		PLTFM_MSG_ERR("Write LTE fail!\n");
		return ret;
	}

	ret = mac_read_lte_8852b(adapter, R_AX_LTE_SW_CFG_2, &val);
	if (ret) {
		PLTFM_MSG_ERR("Read LTE fail!\n");
		return ret;
	}
	val = val & B_AX_WL_RX_CTRL ? B_AX_WL_RX_CTRL : 0 |
		((gnt_cfg->band0.gnt_bt_sw_en || gnt_cfg->band1.gnt_bt_sw_en) ?
		 (B_AX_GNT_BT_TX_SW_CTRL | B_AX_GNT_BT_RX_SW_CTRL) : 0) |
		((gnt_cfg->band0.gnt_bt ||  gnt_cfg->band1.gnt_bt) ?
		 (B_AX_GNT_BT_TX_SW_VAL | B_AX_GNT_BT_RX_SW_VAL) : 0) |
		((gnt_cfg->band0.gnt_wl_sw_en || gnt_cfg->band1.gnt_wl_sw_en) ?
		 (B_AX_GNT_WL_TX_SW_CTRL | B_AX_GNT_WL_RX_SW_CTRL) : 0) |
		((gnt_cfg->band0.gnt_wl ||  gnt_cfg->band1.gnt_wl) ?
		 (B_AX_GNT_WL_TX_SW_VAL | B_AX_GNT_WL_RX_SW_VAL) : 0);

	ret = mac_write_lte_8852b(adapter, R_AX_LTE_SW_CFG_2, val);
	if (ret) {
		PLTFM_MSG_ERR("Write LTE fail!\n");
		return ret;
	}

	return MACSUCCESS;
}

u32 mac_read_coex_reg_8852b(struct mac_ax_adapter *adapter,
			    const u32 offset, u32 *val)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (offset > 0xFF) {
		PLTFM_MSG_ERR("[ERR]offset exceed coex reg\n");
		return MACBADDR;
	}

	*val = MAC_REG_R32(R_AX_BTC_CFG + offset);

	return MACSUCCESS;
}

u32 mac_write_coex_reg_8852b(struct mac_ax_adapter *adapter,
			     const u32 offset, const u32 val)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (offset > 0xFF) {
		PLTFM_MSG_ERR("[ERR]offset exceed coex reg\n");
		return MACBADDR;
	}

	MAC_REG_W32(R_AX_BTC_CFG + offset, val);

	return MACSUCCESS;
}

u32 mac_cfg_ctrl_path_8852b(struct mac_ax_adapter *adapter, u32 wl)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u8 val = MAC_REG_R8(R_AX_SYS_SDIO_CTRL + 3);

	val = wl ? val | BIT(2) : val & ~BIT(2);
	MAC_REG_W8(R_AX_SYS_SDIO_CTRL + 3, val);

	return MACSUCCESS;
}

u32 mac_get_ctrl_path_8852b(struct mac_ax_adapter *adapter, u32 *wl)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u8 val = MAC_REG_R8(R_AX_SYS_SDIO_CTRL + 3);

	*wl = !!(val & BIT(2));

	return MACSUCCESS;
}

#endif /* #if MAC_AX_8852B_SUPPORT */

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
#include "_pcie_8852b.h"

#if MAC_AX_8852B_SUPPORT

#if MAC_AX_PCIE_SUPPORT

static struct mac_ax_intf_info intf_info_def_8852b = {
	MAC_AX_BD_TRUNC,
	MAC_AX_BD_TRUNC,
	MAC_AX_RXBD_PKT,
	MAC_AX_TAG_MULTI,
	MAC_AX_TX_BURST_2048B,
	MAC_AX_RX_BURST_128B,
	MAC_AX_WD_DMA_INTVL_256NS,
	MAC_AX_WD_DMA_INTVL_256NS,
	MAC_AX_TAG_NUM_8,
	0,
	NULL,
	NULL,
	0,
	NULL,
	MAC_AX_PCIE_ENABLE,
	MAC_AX_LBC_TMR_2MS,
	MAC_AX_PCIE_DISABLE,
	MAC_AX_PCIE_DISABLE,
	MAC_AX_IO_RCY_ANA_TMR_6MS
};

static struct mac_ax_pcie_ltr_param pcie_ltr_param_def_8852b = {
	0,
	0,
	MAC_AX_PCIE_ENABLE,
	MAC_AX_PCIE_ENABLE,
	MAC_AX_PCIE_LTR_SPC_500US,
	MAC_AX_PCIE_LTR_IDLE_TIMER_3_2MS,
	{MAC_AX_PCIE_ENABLE, 0x28},
	{MAC_AX_PCIE_ENABLE, 0x28},
	{MAC_AX_PCIE_ENABLE, 0x90039003},
	{MAC_AX_PCIE_ENABLE, 0x880b880b}
};

static struct mac_ax_pcie_cfgspc_param pcie_cfgspc_param_def_8852b = {
	0,
	0,
	MAC_AX_PCIE_DISABLE,
	MAC_AX_PCIE_ENABLE,
	MAC_AX_PCIE_ENABLE,
	MAC_AX_PCIE_ENABLE,
	MAC_AX_PCIE_ENABLE,
	MAC_AX_PCIE_CLKDLY_500US,
	MAC_AX_PCIE_L0SDLY_4US,
	MAC_AX_PCIE_L1DLY_16US
};

struct txbd_ram mac_bdram_tbl_8852b[] = {
		/* ACH0_QUEUE_IDX_8852BE */ {0, 5, 2},
		/* ACH1_QUEUE_IDX_8852BE */ {5, 5, 2},
		/* ACH2_QUEUE_IDX_8852BE */ {10, 5, 2},
		/* ACH3_QUEUE_IDX_8852BE */ {15, 5, 2},
		/* MGQ_B0_QUEUE_IDX_8852BE */ {20, 4, 1},
		/* HIQ_B0_QUEUE_IDX_8852BE */ {24, 4, 1},
		/* FWCMD_QUEUE_IDX_8852BE */ {28, 4, 1}
};

static u32 chk_stus_l1ss(struct mac_ax_adapter *adapter, u8 *val)
{
	u16 cap_val;
	u8 stus_val;
	u8 sup_val;
	u32 ret;
	u8 val8_1, val8_2;

	ret = dbi_r8_pcie(adapter, PCIE_L1SS_CAP + 1, &val8_1);
	if (ret != MACSUCCESS)
		return ret;

	ret = dbi_r8_pcie(adapter, PCIE_L1SS_CAP, &val8_2);
	if (ret != MACSUCCESS)
		return ret;

	cap_val = (u16)((val8_1 << 8) | val8_2);

	ret = dbi_r8_pcie(adapter, PCIE_L1SS_SUP, &sup_val);
	if (ret != MACSUCCESS)
		return ret;

	ret = dbi_r8_pcie(adapter, PCIE_L1SS_STS, &stus_val);
	if (ret != MACSUCCESS)
		return ret;

	if (cap_val == PCIE_L1SS_ID &&
	    (sup_val & PCIE_BIT_L1SSSUP) &&
	    (sup_val & PCIE_L1SS_MASK) != 0 &&
	    (stus_val & PCIE_L1SS_MASK) != 0)
		*val = 1;
	else
		*val = 0;

	return ret;
}

static u32 update_clkdly(struct mac_ax_adapter *adapter, u8 *val,
			 enum mac_ax_pcie_clkdly ctrl,
			 enum mac_ax_pcie_clkdly def_ctrl)
{
	u8 tmp;

	if (ctrl == MAC_AX_PCIE_CLKDLY_IGNORE ||
	    def_ctrl == MAC_AX_PCIE_CLKDLY_IGNORE)
		return MACSUCCESS;

	tmp = (u8)((ctrl == MAC_AX_PCIE_CLKDLY_DEF) ? def_ctrl : ctrl);
	switch (tmp) {
	case MAC_AX_PCIE_CLKDLY_0:
		*val = PCIE_CLKDLY_HW_0;
		break;

	case MAC_AX_PCIE_CLKDLY_30US:
		*val = PCIE_CLKDLY_HW_30US;
		break;

	case MAC_AX_PCIE_CLKDLY_50US:
		*val = PCIE_CLKDLY_HW_50US;
		break;

	case MAC_AX_PCIE_CLKDLY_100US:
		*val = PCIE_CLKDLY_HW_100US;
		break;

	case MAC_AX_PCIE_CLKDLY_150US:
		*val = PCIE_CLKDLY_HW_150US;
		break;

	case MAC_AX_PCIE_CLKDLY_200US:
		*val = PCIE_CLKDLY_HW_200US;
		break;
	case MAC_AX_PCIE_CLKDLY_300US:
		*val = PCIE_CLKDLY_HW_300US;
		break;
	case MAC_AX_PCIE_CLKDLY_400US:
		*val = PCIE_CLKDLY_HW_400US;
		break;
	case MAC_AX_PCIE_CLKDLY_500US:
		*val = PCIE_CLKDLY_HW_500US;
		break;
	case MAC_AX_PCIE_CLKDLY_1MS:
		*val = PCIE_CLKDLY_HW_1MS;
		break;
	case MAC_AX_PCIE_CLKDLY_3MS:
		*val = PCIE_CLKDLY_HW_3MS;
		break;
	default:
		PLTFM_MSG_ERR("[ERR]CLKDLY wt val illegal!\n");
		return MACSTCAL;
	}
	return MACSUCCESS;
}

static u32 update_aspmdly(struct mac_ax_adapter *adapter, u8 *val,
			  struct mac_ax_pcie_cfgspc_param *param,
			  struct mac_ax_pcie_cfgspc_param *param_def)
{
	u8 l1mask = PCIE_ASPMDLY_MASK << SHFT_L1DLY;
	u8 l0smask = PCIE_ASPMDLY_MASK << SHFT_L0SDLY;
	u8 l1updval = param->l1dly_ctrl;
	u8 l0supdval = param->l0sdly_ctrl;
	u8 l1defval = param_def->l1dly_ctrl;
	u8 l0sdefval = param_def->l0sdly_ctrl;
	u8 tmp;
	u8 hwval;

	if (l1updval != MAC_AX_PCIE_L1DLY_IGNORE) {
		tmp = (l1updval == MAC_AX_PCIE_L1DLY_DEF) ? l1defval : l1updval;
		switch (tmp) {
		case MAC_AX_PCIE_L1DLY_16US:
			hwval = PCIE_L1DLY_HW_16US;
			break;

		case MAC_AX_PCIE_L1DLY_32US:
			hwval = PCIE_L1DLY_HW_32US;
			break;

		case MAC_AX_PCIE_L1DLY_64US:
			hwval = PCIE_L1DLY_HW_64US;
			break;

		case MAC_AX_PCIE_L1DLY_INFI:
			hwval = PCIE_L1DLY_HW_INFI;
			break;

		default:
			PLTFM_MSG_ERR("[ERR]L1DLY wt val illegal!\n");
			return MACSTCAL;
		}

		tmp = (hwval << SHFT_L1DLY) & l1mask;
		*val = (*val & ~(l1mask)) | tmp;
	}

	if (l0supdval != MAC_AX_PCIE_L0SDLY_IGNORE) {
		tmp = (l0supdval == MAC_AX_PCIE_L0SDLY_DEF) ?
		       l0sdefval : l0supdval;
		switch (tmp) {
		case MAC_AX_PCIE_L0SDLY_1US:
			hwval = PCIE_L0SDLY_HW_1US;
			break;

		case MAC_AX_PCIE_L0SDLY_2US:
			hwval = PCIE_L0SDLY_HW_2US;
			break;

		case MAC_AX_PCIE_L0SDLY_3US:
			hwval = PCIE_L0SDLY_HW_3US;
			break;

		case MAC_AX_PCIE_L0SDLY_4US:
			hwval = PCIE_L0SDLY_HW_4US;
			break;

		case MAC_AX_PCIE_L0SDLY_5US:
			hwval = PCIE_L0SDLY_HW_5US;
			break;

		case MAC_AX_PCIE_L0SDLY_6US:
			hwval = PCIE_L0SDLY_HW_6US;
			break;

		case MAC_AX_PCIE_L0SDLY_7US:
			hwval = PCIE_L0SDLY_HW_7US;
			break;

		default:
			PLTFM_MSG_ERR("[ERR]L0SDLY wt val illegal!\n");
			return MACSTCAL;
		}
		tmp = (hwval << SHFT_L0SDLY) & l0smask;
		*val = (*val & ~(l0smask)) | tmp;
	}

	return MACSUCCESS;
}

struct mac_ax_intf_info *
get_pcie_info_def_8852b(struct mac_ax_adapter *adapter)
{
	return &intf_info_def_8852b;
}

struct txbd_ram *
get_bdram_tbl_pcie_8852b(struct mac_ax_adapter *adapter)
{
	return mac_bdram_tbl_8852b;
}

u32 mio_w32_pcie_8852b(struct mac_ax_adapter *adapter, u16 addr, u32 value)
{
	return MACNOTSUP;
}

u32 mio_r32_pcie_8852b(struct mac_ax_adapter *adapter, u16 addr, u32 *val)
{
	return MACNOTSUP;
}

u32 get_txbd_reg_pcie_8852b(struct mac_ax_adapter *adapter, u8 dma_ch, u32 *reg,
			    enum pcie_bd_ctrl_type type)
{
	*reg = MAC_AX_R32_FF;

	switch (dma_ch) {
	case MAC_AX_DMA_ACH0:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_ACH0_TXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_ACH0_TXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_ACH0_TXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_ACH0_TXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
			*reg = R_AX_ACH0_BDRAM_CTRL_V2;
			break;
		default:
			PLTFM_MSG_ERR("TXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	case MAC_AX_DMA_ACH1:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_ACH1_TXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_ACH1_TXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_ACH1_TXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_ACH1_TXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
			*reg = R_AX_ACH1_BDRAM_CTRL_V2;
			break;
		default:
			PLTFM_MSG_ERR("TXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	case MAC_AX_DMA_ACH2:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_ACH2_TXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_ACH2_TXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_ACH2_TXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_ACH2_TXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
			*reg = R_AX_ACH2_BDRAM_CTRL_V2;
			break;
		default:
			PLTFM_MSG_ERR("TXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	case MAC_AX_DMA_ACH3:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_ACH3_TXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_ACH3_TXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_ACH3_TXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_ACH3_TXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
			*reg = R_AX_ACH3_BDRAM_CTRL_V2;
			break;
		default:
			PLTFM_MSG_ERR("TXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	case MAC_AX_DMA_B0MG:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_CH8_TXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_CH8_TXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_CH8_TXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_CH8_TXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
			*reg = R_AX_CH8_BDRAM_CTRL_V2;
			break;
		default:
			PLTFM_MSG_ERR("TXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	case MAC_AX_DMA_B0HI:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_CH9_TXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_CH9_TXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_CH9_TXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_CH9_TXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
			*reg = R_AX_CH9_BDRAM_CTRL_V2;
			break;
		default:
			PLTFM_MSG_ERR("TXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	case MAC_AX_DMA_H2C:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_CH12_TXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_CH12_TXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_CH12_TXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_CH12_TXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
			*reg = R_AX_CH12_BDRAM_CTRL_V2;
			break;
		default:
			PLTFM_MSG_ERR("TXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	case MAC_AX_DMA_ACH4:
	case MAC_AX_DMA_ACH5:
	case MAC_AX_DMA_ACH6:
	case MAC_AX_DMA_ACH7:
	case MAC_AX_DMA_B1MG:
	case MAC_AX_DMA_B1HI:
		return MACNOTSUP;
	default:
		PLTFM_MSG_ERR("[ERR] TXBD num CH%d invalid\n", dma_ch);
		return MACFUNCINPUT;
	}

	return MACSUCCESS;
}

u32 set_txbd_reg_pcie_8852b(struct mac_ax_adapter *adapter, u8 dma_ch,
			    enum pcie_bd_ctrl_type type, u32 val0, u32 val1, u32 val2)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 ret, reg, val32;
	u16 val16;

	ret = p_ops->get_txbd_reg_pcie(adapter, dma_ch, &reg, type);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("get txbd%d reg type%d %d\n", dma_ch, type, ret);
		return ret;
	}

	switch (type) {
	case PCIE_BD_CTRL_DESC_L:
	case PCIE_BD_CTRL_DESC_H:
		MAC_REG_W32(reg, val0);
		break;
	case PCIE_BD_CTRL_NUM:
		val16 = MAC_REG_R16(reg);
		val16 = SET_CLR_WORD(val16, val0, B_AX_ACH0_DESC_NUM);
		MAC_REG_W16(reg, val16);
		break;
	case PCIE_BD_CTRL_IDX:
		val16 = MAC_REG_R16(reg);
		val16 = SET_CLR_WORD(val16, val0, B_AX_ACH0_HOST_IDX);
		MAC_REG_W16(reg, val16);
		break;
	case PCIE_BD_CTRL_BDRAM:
		val32 = MAC_REG_R32(reg);
		val32 = SET_CLR_WORD(val32, val0, B_AX_ACH0_BDRAM_SIDX_V2);
		val32 = SET_CLR_WORD(val32, val1, B_AX_ACH0_BDRAM_MAX_V2);
		val32 = SET_CLR_WORD(val32, val2, B_AX_ACH0_BDRAM_MIN_V2);
		MAC_REG_W32(reg, val32);
		break;
	default:
		PLTFM_MSG_ERR("TXBD reg type%d invalid\n", type);
		return MACFUNCINPUT;
	}

	return MACSUCCESS;
}

u32 get_rxbd_reg_pcie_8852b(struct mac_ax_adapter *adapter, u8 dma_ch, u32 *reg,
			    enum pcie_bd_ctrl_type type)
{
	*reg = MAC_AX_R32_FF;

	switch (dma_ch) {
	case MAC_AX_RX_CH_RXQ:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_RXQ_RXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_RXQ_RXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_RXQ_RXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_RXQ_RXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
		default:
			PLTFM_MSG_ERR("RXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	case MAC_AX_RX_CH_RPQ:
		switch (type) {
		case PCIE_BD_CTRL_DESC_L:
			*reg = R_AX_RPQ_RXBD_DESA_L;
			break;
		case PCIE_BD_CTRL_DESC_H:
			*reg = R_AX_RPQ_RXBD_DESA_H;
			break;
		case PCIE_BD_CTRL_NUM:
			*reg = R_AX_RPQ_RXBD_NUM;
			break;
		case PCIE_BD_CTRL_IDX:
			*reg = R_AX_RPQ_RXBD_IDX;
			break;
		case PCIE_BD_CTRL_BDRAM:
		default:
			PLTFM_MSG_ERR("RXBD reg type%d invalid\n", type);
			return MACFUNCINPUT;
		}
		break;
	default:
		PLTFM_MSG_ERR("[ERR] RXBD CH%d invalid\n", dma_ch);
		return MACFUNCINPUT;
	}

	return MACSUCCESS;
}

u32 set_rxbd_reg_pcie_8852b(struct mac_ax_adapter *adapter, u8 dma_ch,
			    enum pcie_bd_ctrl_type type, u32 val0, u32 val1, u32 val2)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 ret, reg;
	u16 val16;

	ret = p_ops->get_rxbd_reg_pcie(adapter, dma_ch, &reg, type);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("get rxbd%d reg type%d %d\n", dma_ch, type, ret);
		return ret;
	}

	switch (type) {
	case PCIE_BD_CTRL_DESC_L:
	case PCIE_BD_CTRL_DESC_H:
		MAC_REG_W32(reg, val0);
		break;
	case PCIE_BD_CTRL_NUM:
		val16 = MAC_REG_R16(reg);
		val16 = SET_CLR_WORD(val16, val0, B_AX_RXQ_DESC_NUM);
		MAC_REG_W16(reg, val16);
		break;
	case PCIE_BD_CTRL_IDX:
		val16 = MAC_REG_R16(reg);
		val16 = SET_CLR_WORD(val16, val0, B_AX_RXQ_HOST_IDX);
		MAC_REG_W16(reg, val16);
		break;
	case PCIE_BD_CTRL_BDRAM:
	default:
		PLTFM_MSG_ERR("RXBD reg type%d invalid\n", type);
		return MACFUNCINPUT;
	}

	return MACSUCCESS;
}

u32 pcie_cfgspc_write_8852b(struct mac_ax_adapter *adapter,
			    struct mac_ax_pcie_cfgspc_param *param)
{
#ifdef RTW_WKARD_GET_PROCESSOR_ID
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
#endif
	struct mac_ax_pcie_cfgspc_param *param_def = &pcie_cfgspc_param_def_8852b;
	u8 l1_val, aspm_val, l1ss_val, clk_val, tmp8, val8;
	u32 ret = MACSUCCESS;

	ret = dbi_r8_pcie(adapter, PCIE_L1_CTRL, &l1_val);
	if (ret != MACSUCCESS)
		return ret;
	ret = dbi_r8_pcie(adapter, PCIE_ASPM_CTRL, &aspm_val);
	if (ret != MACSUCCESS)
		return ret;
	ret = dbi_r8_pcie(adapter, PCIE_L1SS_CTRL, &l1ss_val);
	if (ret != MACSUCCESS)
		return ret;
	ret = dbi_r8_pcie(adapter, PCIE_CLK_CTRL, &clk_val);
	if (ret != MACSUCCESS)
		return ret;

	if (l1_val == 0xFF || aspm_val == 0xFF || l1ss_val == 0xFF ||
	    clk_val == 0xFF) {
		PLTFM_MSG_ERR("[ERR] PCIE CFG reg read 0xFF!\n");
		return MACCMP;
	}

	ret = dbi_r8_pcie(adapter, PCIE_L1_STS, &tmp8);
	if (ret != MACSUCCESS)
		return ret;
	if (((tmp8 & PCIE_BIT_STS_L0S) && param->l0s_ctrl ==
	    MAC_AX_PCIE_DEFAULT) || (param->l0s_ctrl != MAC_AX_PCIE_DEFAULT &&
	    param->l0s_ctrl != MAC_AX_PCIE_IGNORE))
		update_pcie_func_u8(&aspm_val, PCIE_BIT_L0S,
				    param->l0s_ctrl, param_def->l0s_ctrl);

	if (((tmp8 & PCIE_BIT_STS_L1) && param->l1_ctrl ==
	    MAC_AX_PCIE_DEFAULT) || (param->l1_ctrl != MAC_AX_PCIE_DEFAULT &&
	    param->l1_ctrl != MAC_AX_PCIE_IGNORE))
		update_pcie_func_u8(&l1_val, PCIE_BIT_L1,
				    param->l1_ctrl, param_def->l1_ctrl);
	update_pcie_func_u8(&l1_val, PCIE_BIT_WAKE,
			    param->wake_ctrl, param_def->wake_ctrl);
	update_pcie_func_u8(&l1_val, PCIE_BIT_CLK,
			    param->crq_ctrl, param_def->crq_ctrl);
	ret = chk_stus_l1ss(adapter, &val8);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR] PCIE chk_stus_l1ss\n");
		return ret;
	}

	if ((val8 && param->l1ss_ctrl == MAC_AX_PCIE_DEFAULT) ||
	    (param->l1ss_ctrl != MAC_AX_PCIE_DEFAULT &&
	     param->l1ss_ctrl != MAC_AX_PCIE_IGNORE))
		update_pcie_func_u8(&l1ss_val, PCIE_BIT_L1SS,
				    param->l1ss_ctrl, param_def->l1ss_ctrl);

	ret = update_clkdly(adapter, &clk_val, param->clkdly_ctrl,
			    param_def->clkdly_ctrl);
	if (ret != MACSUCCESS)
		return ret;

	ret = update_aspmdly(adapter, &aspm_val, param, param_def);
	if (ret != MACSUCCESS)
		return ret;

	if (param->clkdly_ctrl != MAC_AX_PCIE_CLKDLY_IGNORE) {
#ifdef RTW_WKARD_GET_PROCESSOR_ID
		ret = p_ops->chk_proc_long_ldy_pcie(adapter, &tmp8);
		if (ret != MACSUCCESS)
			return ret;
		if (tmp8 == PROC_SHORT_DLY)
			clk_val = PCIE_CLKDLY_HW_50US;
#endif
		ret = dbi_w8_pcie(adapter, PCIE_CLK_CTRL, clk_val);
		if (ret != MACSUCCESS)
			return ret;
	}

	if (param->l0s_ctrl != MAC_AX_PCIE_IGNORE ||
	    param->l1dly_ctrl != MAC_AX_PCIE_L1DLY_IGNORE ||
	    param->l0sdly_ctrl != MAC_AX_PCIE_L0SDLY_IGNORE) {
		ret = dbi_w8_pcie(adapter, PCIE_ASPM_CTRL, aspm_val);
		if (ret != MACSUCCESS)
			return ret;
	}
	if (param->l1_ctrl != MAC_AX_PCIE_IGNORE ||
	    param->wake_ctrl != MAC_AX_PCIE_IGNORE ||
	    param->crq_ctrl != MAC_AX_PCIE_IGNORE) {
		ret = dbi_w8_pcie(adapter, PCIE_L1_CTRL, l1_val);
		if (ret != MACSUCCESS)
			return ret;
	}
	if (param->l1ss_ctrl != MAC_AX_PCIE_IGNORE) {
		ret = dbi_w8_pcie(adapter, PCIE_L1SS_CTRL, l1ss_val);
		if (ret != MACSUCCESS)
			return ret;
	}

	return ret;
}

u32 pcie_cfgspc_read_8852b(struct mac_ax_adapter *adapter,
			   struct mac_ax_pcie_cfgspc_param *param)
{
	u8 l1_val;
	u8 aspm_val;
	u8 l1ss_val;
	u8 clk_val;
	u8 l0smask;
	u8 l1mask;
	u32 ret;

	ret = dbi_r8_pcie(adapter, PCIE_L1_CTRL, &l1_val);
	if (ret != MACSUCCESS)
		return ret;

	ret = dbi_r8_pcie(adapter, PCIE_ASPM_CTRL, &aspm_val);
	if (ret != MACSUCCESS)
		return ret;

	ret = dbi_r8_pcie(adapter, PCIE_L1SS_CTRL, &l1ss_val);
	if (ret != MACSUCCESS)
		return ret;

	ret = dbi_r8_pcie(adapter, PCIE_CLK_CTRL, &clk_val);
	if (ret != MACSUCCESS)
		return ret;

	if (l1_val == 0xFF || aspm_val == 0xFF ||
	    l1ss_val == 0xFF || clk_val == 0xFF) {
		PLTFM_MSG_ERR("[ERR] (2nd)PCIE CFG reg read 0xFF!\n");
		return MACCMP;
	}

	param->l0s_ctrl = GET_PCIE_FUNC_STUS(aspm_val, PCIE_BIT_L0S);
	param->l1_ctrl = GET_PCIE_FUNC_STUS(l1_val, PCIE_BIT_L1);
	param->l1ss_ctrl = GET_PCIE_FUNC_STUS(l1ss_val, PCIE_BIT_L1SS);
	param->wake_ctrl = GET_PCIE_FUNC_STUS(l1_val, PCIE_BIT_WAKE);
	param->crq_ctrl = GET_PCIE_FUNC_STUS(l1_val, PCIE_BIT_CLK);

	switch (clk_val) {
	case PCIE_CLKDLY_HW_0:
		param->clkdly_ctrl = MAC_AX_PCIE_CLKDLY_0;
		break;

	case PCIE_CLKDLY_HW_30US:
		param->clkdly_ctrl = MAC_AX_PCIE_CLKDLY_30US;
		break;

	case PCIE_CLKDLY_HW_50US:
		param->clkdly_ctrl = MAC_AX_PCIE_CLKDLY_50US;
		break;

	case PCIE_CLKDLY_HW_100US:
		param->clkdly_ctrl = MAC_AX_PCIE_CLKDLY_100US;
		break;

	case PCIE_CLKDLY_HW_150US:
		param->clkdly_ctrl = MAC_AX_PCIE_CLKDLY_150US;
		break;

	case PCIE_CLKDLY_HW_200US:
		param->clkdly_ctrl = MAC_AX_PCIE_CLKDLY_200US;
		break;

	default:
		param->clkdly_ctrl = MAC_AX_PCIE_CLKDLY_R_ERR;
		break;
	}

	l0smask = PCIE_ASPMDLY_MASK << SHFT_L0SDLY;
	l1mask = PCIE_ASPMDLY_MASK << SHFT_L1DLY;

	switch ((aspm_val & l0smask) >> SHFT_L0SDLY) {
	case PCIE_L0SDLY_HW_1US:
		param->l0sdly_ctrl = MAC_AX_PCIE_L0SDLY_1US;
		break;

	case PCIE_L0SDLY_HW_3US:
		param->l0sdly_ctrl = MAC_AX_PCIE_L0SDLY_3US;
		break;

	case PCIE_L0SDLY_HW_5US:
		param->l0sdly_ctrl = MAC_AX_PCIE_L0SDLY_5US;
		break;

	case PCIE_L0SDLY_HW_7US:
		param->l0sdly_ctrl = MAC_AX_PCIE_L0SDLY_7US;
		break;

	default:
		param->l0sdly_ctrl = MAC_AX_PCIE_L0SDLY_R_ERR;
		break;
	}

	switch ((aspm_val & l1mask) >> SHFT_L1DLY) {
	case PCIE_L1DLY_HW_16US:
		param->l1dly_ctrl = MAC_AX_PCIE_L1DLY_16US;
		break;

	case PCIE_L1DLY_HW_32US:
		param->l1dly_ctrl = MAC_AX_PCIE_L1DLY_32US;
		break;

	case PCIE_L1DLY_HW_64US:
		param->l1dly_ctrl = MAC_AX_PCIE_L1DLY_64US;
		break;

	case PCIE_L1DLY_HW_INFI:
		param->l1dly_ctrl = MAC_AX_PCIE_L1DLY_INFI;
		break;

	default:
		param->l1dly_ctrl = MAC_AX_PCIE_L1DLY_R_ERR;
		break;
	}

	return ret;
}

u32 pcie_ltr_write_8852b(struct mac_ax_adapter *adapter,
			 struct mac_ax_pcie_ltr_param *param)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 ltr_ctrl0, ltr_ctrl1, ltr_idle_ltcy, ltr_act_ltcy;
	u32 status = MACSUCCESS;
	struct mac_ax_pcie_ltr_param *param_def = &pcie_ltr_param_def_8852b;
	u32 val32;
	enum mac_ax_pcie_func_ctrl ctrl;

	ltr_ctrl0 = MAC_REG_R32(R_AX_LTR_CTRL_0);
	ltr_ctrl1 = MAC_REG_R32(R_AX_LTR_CTRL_1);
	ltr_idle_ltcy = MAC_REG_R32(R_AX_LTR_IDLE_LATENCY);
	ltr_act_ltcy = MAC_REG_R32(R_AX_LTR_ACTIVE_LATENCY);

	if (ltr_ctrl0 == MAC_AX_R32_FF || ltr_ctrl0 == MAC_AX_R32_EA ||
	    ltr_ctrl1 == MAC_AX_R32_FF || ltr_ctrl1 == MAC_AX_R32_EA ||
	    ltr_idle_ltcy == MAC_AX_R32_FF || ltr_idle_ltcy == MAC_AX_R32_EA ||
	    ltr_act_ltcy == MAC_AX_R32_FF || ltr_act_ltcy == MAC_AX_R32_EA) {
		PLTFM_MSG_ERR("[ERR] LTR reg read nothing!\n");
		return MACCMP;
	}

	if (!(param_def->ltr_ctrl == MAC_AX_PCIE_IGNORE ||
	      param->ltr_ctrl == MAC_AX_PCIE_IGNORE ||
	      param_def->ltr_ctrl == MAC_AX_PCIE_DEFAULT)) {
		ctrl = param->ltr_ctrl == MAC_AX_PCIE_DEFAULT ?
		       param_def->ltr_ctrl : param->ltr_ctrl;
		ltr_ctrl0 = ctrl == MAC_AX_PCIE_ENABLE ?
			    (ltr_ctrl0 | B_AX_LTR_EN) :
			    (ltr_ctrl0 & ~B_AX_LTR_EN);
	}

	if (!(param_def->ltr_hw_ctrl == MAC_AX_PCIE_IGNORE ||
	      param->ltr_hw_ctrl == MAC_AX_PCIE_IGNORE ||
	      param_def->ltr_hw_ctrl == MAC_AX_PCIE_DEFAULT)) {
		ctrl = param->ltr_hw_ctrl == MAC_AX_PCIE_DEFAULT ?
		       param_def->ltr_hw_ctrl : param->ltr_hw_ctrl;
		ltr_ctrl0 = ctrl == MAC_AX_PCIE_ENABLE ?
			    (ltr_ctrl0 | B_AX_LTR_HW_EN | B_AX_LTR_WD_NOEMP_CHK) :
			    (ltr_ctrl0 & ~(B_AX_LTR_HW_EN | B_AX_LTR_WD_NOEMP_CHK));
	}

	if (!(param_def->ltr_spc_ctrl == MAC_AX_PCIE_LTR_SPC_IGNORE ||
	      param->ltr_spc_ctrl == MAC_AX_PCIE_LTR_SPC_IGNORE ||
	      param_def->ltr_spc_ctrl == MAC_AX_PCIE_LTR_SPC_DEF)) {
		val32 = param->ltr_spc_ctrl == MAC_AX_PCIE_LTR_SPC_DEF ?
			param_def->ltr_spc_ctrl : param->ltr_spc_ctrl;
		ltr_ctrl0 = SET_CLR_WORD(ltr_ctrl0, val32, B_AX_LTR_SPACE_IDX);
	}

	if (!(param_def->ltr_idle_timer_ctrl ==
	      MAC_AX_PCIE_LTR_IDLE_TIMER_IGNORE ||
	      param->ltr_idle_timer_ctrl ==
	      MAC_AX_PCIE_LTR_IDLE_TIMER_IGNORE ||
	      param_def->ltr_idle_timer_ctrl ==
	      MAC_AX_PCIE_LTR_IDLE_TIMER_DEF)) {
		val32 = param->ltr_idle_timer_ctrl ==
			MAC_AX_PCIE_LTR_IDLE_TIMER_DEF ?
			param_def->ltr_idle_timer_ctrl :
			param->ltr_idle_timer_ctrl;
		ltr_ctrl0 = SET_CLR_WORD(ltr_ctrl0, val32,
					 B_AX_LTR_IDLE_TIMER_IDX);
	}

	if (!(param_def->ltr_rx0_th_ctrl.ctrl == MAC_AX_PCIE_IGNORE ||
	      param->ltr_rx0_th_ctrl.ctrl == MAC_AX_PCIE_IGNORE ||
	      param_def->ltr_rx0_th_ctrl.ctrl == MAC_AX_PCIE_DEFAULT)) {
		val32 = param->ltr_rx0_th_ctrl.ctrl == MAC_AX_PCIE_DEFAULT ?
			param_def->ltr_rx0_th_ctrl.val :
			param->ltr_rx0_th_ctrl.val;
		ltr_ctrl1 = SET_CLR_WORD(ltr_ctrl1, val32, B_AX_LTR_RX0_TH);
	}

	if (!(param_def->ltr_rx1_th_ctrl.ctrl == MAC_AX_PCIE_IGNORE ||
	      param->ltr_rx1_th_ctrl.ctrl == MAC_AX_PCIE_IGNORE ||
	      param_def->ltr_rx1_th_ctrl.ctrl == MAC_AX_PCIE_DEFAULT)) {
		val32 = param->ltr_rx1_th_ctrl.ctrl == MAC_AX_PCIE_DEFAULT ?
			param_def->ltr_rx1_th_ctrl.val :
			param->ltr_rx1_th_ctrl.val;
		ltr_ctrl1 = SET_CLR_WORD(ltr_ctrl1, val32, B_AX_LTR_RX1_TH);
	}

	if (!(param_def->ltr_idle_lat_ctrl.ctrl == MAC_AX_PCIE_IGNORE ||
	      param->ltr_idle_lat_ctrl.ctrl == MAC_AX_PCIE_IGNORE ||
	      param_def->ltr_idle_lat_ctrl.ctrl == MAC_AX_PCIE_DEFAULT)) {
		val32 = param->ltr_idle_lat_ctrl.ctrl == MAC_AX_PCIE_DEFAULT ?
			param_def->ltr_idle_lat_ctrl.val :
			param->ltr_idle_lat_ctrl.val;
		ltr_idle_ltcy =
			SET_CLR_WORD(ltr_idle_ltcy, val32, B_AX_LTR_IDLE_LTCY);
	}

	if (!(param_def->ltr_act_lat_ctrl.ctrl == MAC_AX_PCIE_IGNORE ||
	      param->ltr_act_lat_ctrl.ctrl == MAC_AX_PCIE_IGNORE ||
	      param_def->ltr_act_lat_ctrl.ctrl == MAC_AX_PCIE_DEFAULT)) {
		val32 = param->ltr_act_lat_ctrl.ctrl == MAC_AX_PCIE_DEFAULT ?
			param_def->ltr_act_lat_ctrl.val :
			param->ltr_act_lat_ctrl.val;
		ltr_act_ltcy =
			SET_CLR_WORD(ltr_act_ltcy, val32, B_AX_LTR_ACT_LTCY);
	}

	MAC_REG_W32(R_AX_LTR_CTRL_0, ltr_ctrl0);
	MAC_REG_W32(R_AX_LTR_CTRL_1, ltr_ctrl1);
	MAC_REG_W32(R_AX_LTR_IDLE_LATENCY, ltr_idle_ltcy);
	MAC_REG_W32(R_AX_LTR_ACTIVE_LATENCY, ltr_act_ltcy);

	return status;
}

u32 pcie_ltr_read_8852b(struct mac_ax_adapter *adapter,
			struct mac_ax_pcie_ltr_param *param)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 status = MACSUCCESS;
	u32 ltr_ctrl0, ltr_ctrl1, ltr_idle_ltcy, ltr_act_ltcy;

	ltr_ctrl0 = MAC_REG_R32(R_AX_LTR_CTRL_0);
	ltr_ctrl1 = MAC_REG_R32(R_AX_LTR_CTRL_1);
	ltr_idle_ltcy = MAC_REG_R32(R_AX_LTR_IDLE_LATENCY);
	ltr_act_ltcy = MAC_REG_R32(R_AX_LTR_ACTIVE_LATENCY);

	if (ltr_ctrl0 == MAC_AX_R32_FF || ltr_ctrl0 == MAC_AX_R32_EA ||
	    ltr_ctrl1 == MAC_AX_R32_FF || ltr_ctrl1 == MAC_AX_R32_EA ||
	    ltr_idle_ltcy == MAC_AX_R32_FF || ltr_idle_ltcy == MAC_AX_R32_EA ||
	    ltr_act_ltcy == MAC_AX_R32_FF || ltr_act_ltcy == MAC_AX_R32_EA) {
		PLTFM_MSG_ERR("[ERR] LTR reg read nothing!\n");
		return MACCMP;
	}

	param->ltr_ctrl = ltr_ctrl0 & B_AX_LTR_EN ?
			  MAC_AX_PCIE_ENABLE : MAC_AX_PCIE_DISABLE;

	param->ltr_hw_ctrl = ltr_ctrl0 & B_AX_LTR_HW_EN ?
			     MAC_AX_PCIE_ENABLE : MAC_AX_PCIE_DISABLE;

	param->ltr_spc_ctrl =
		(enum mac_ax_pcie_ltr_spc)
		GET_FIELD(ltr_ctrl0, B_AX_LTR_SPACE_IDX);
	param->ltr_idle_timer_ctrl =
		(enum mac_ax_pcie_ltr_idle_timer)
		GET_FIELD(ltr_ctrl0, B_AX_LTR_IDLE_TIMER_IDX);

	param->ltr_rx0_th_ctrl.val =
		(u16)GET_FIELD(ltr_ctrl1, B_AX_LTR_RX0_TH);
	param->ltr_rx1_th_ctrl.val =
		(u16)GET_FIELD(ltr_ctrl1, B_AX_LTR_RX1_TH);
	param->ltr_idle_lat_ctrl.val =
		GET_FIELD(ltr_idle_ltcy, B_AX_LTR_IDLE_LTCY);
	param->ltr_act_lat_ctrl.val =
		GET_FIELD(ltr_act_ltcy, B_AX_LTR_ACT_LTCY);

	return status;
}

u32 ltr_sw_trigger_8852b(struct mac_ax_adapter *adapter,
			 enum mac_ax_pcie_ltr_sw_ctrl ctrl)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 val32;

	val32 = MAC_REG_R32(R_AX_LTR_CTRL_0);

	if (val32 & B_AX_LTR_HW_EN) {
		PLTFM_MSG_ERR("[ERR]LTR HW mode running\n");
		return MACPROCERR;
	} else if (!(val32 & B_AX_LTR_EN)) {
		PLTFM_MSG_ERR("[ERR]LTR not enable\n");
		return MACHWNOTEN;
	}

	switch (ctrl) {
	case MAC_AX_PCIE_LTR_SW_ACT:
		MAC_REG_W32(R_AX_LTR_CTRL_0, val32 | B_AX_APP_LTR_ACT);
		break;
	case MAC_AX_PCIE_LTR_SW_IDLE:
		MAC_REG_W32(R_AX_LTR_CTRL_0, val32 | B_AX_APP_LTR_IDLE);
		break;
	default:
		PLTFM_MSG_ERR("Invalid input for %s\n", __func__);
		return MACFUNCINPUT;
	}

	return MACSUCCESS;
}

u32 get_avail_txbd_8852b(struct mac_ax_adapter *adapter, u8 ch_idx,
			 u16 *host_idx, u16 *hw_idx, u16 *avail_txbd)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 ret = MACSUCCESS, reg = 0, tmp32;
	u16 bndy = adapter->pcie_info.txbd_bndy;
	u8 tx_dma_ch = 0;

	tx_dma_ch = MAC_AX_DMA_ACH0 + ch_idx;

	ret = p_ops->get_txbd_reg_pcie(adapter, tx_dma_ch, &reg, PCIE_BD_CTRL_IDX);
	if (ret != MACSUCCESS) {
		*avail_txbd = 0;
		return ret;
	}

	tmp32 = MAC_REG_R32(reg);
	*host_idx = (u16)GET_FIELD(tmp32, B_AX_ACH0_HOST_IDX);
	*hw_idx = (u16)GET_FIELD(tmp32, B_AX_ACH0_HW_IDX);
	*avail_txbd = calc_avail_wptr(*hw_idx, *host_idx, bndy);
	PLTFM_MSG_TRACE("%s: ", __func__);
	PLTFM_MSG_TRACE("dma_ch %d, host_idx %d, hw_idx %d avail_txbd %d\n",
			ch_idx, *host_idx, *hw_idx, avail_txbd);

	return MACSUCCESS;
}

u32 get_avail_rxbd_8852b(struct mac_ax_adapter *adapter, u8 ch_idx,
			 u16 *host_idx, u16 *hw_idx, u16 *avail_rxbd)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 ret = MACSUCCESS, reg = 0, tmp32;
	u16 bndy;
	u8 rx_dma_ch = 0;

	rx_dma_ch = MAC_AX_RX_CH_RXQ + ch_idx;

	if (rx_dma_ch == MAC_AX_RX_CH_RXQ) {
		bndy = adapter->pcie_info.rxbd_bndy;
	} else if (rx_dma_ch == MAC_AX_RX_CH_RPQ) {
		bndy = adapter->pcie_info.rpbd_bndy;
	} else {
		PLTFM_MSG_ERR("Unkwown rx_dma_ch: %d\n", rx_dma_ch);
		return MACFUNCINPUT;
	}

	ret = p_ops->get_rxbd_reg_pcie(adapter, rx_dma_ch, &reg, PCIE_BD_CTRL_IDX);
	if (ret != MACSUCCESS) {
		*avail_rxbd = 0;
		return ret;
	}

	tmp32 = MAC_REG_R32(reg);
	*host_idx = (u16)GET_FIELD(tmp32, B_AX_RXQ_HOST_IDX);
	*hw_idx = (u16)GET_FIELD(tmp32, B_AX_RXQ_HW_IDX);
	*avail_rxbd = calc_avail_rptr(*host_idx, *hw_idx, bndy);

	return MACSUCCESS;
}

u32 get_io_stat_pcie_8852b(struct mac_ax_adapter *adapter,
			   struct mac_ax_io_stat *out_st)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 val32, to_addr;

	val32 = MAC_REG_R32(R_AX_LBC_WATCHDOG);
	if (val32 & B_AX_LBC_FLAG) {
		adapter->sm.io_st = MAC_AX_IO_ST_HANG;
		to_addr = GET_FIELD(val32, B_AX_LBC_ADDR);
		PLTFM_MSG_ERR("[ERR]pcie io timeout addr 0x%X\n", to_addr);
		if (out_st) {
			out_st->to_flag = 1;
			out_st->io_st = adapter->sm.io_st;
			out_st->addr = to_addr;
		}
		MAC_REG_W32(R_AX_LBC_WATCHDOG, val32);
	} else if (out_st) {
		out_st->to_flag = 0;
		out_st->io_st = adapter->sm.io_st;
		out_st->addr = 0;
	}

	return MACSUCCESS;
}

u32 ctrl_hci_dma_en_pcie_8852b(struct mac_ax_adapter *adapter,
			       enum mac_ax_pcie_func_ctrl txen,
			       enum mac_ax_pcie_func_ctrl rxen)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg = R_AX_HCI_FUNC_EN;
	u32 rval = MAC_REG_R32(reg);

	if (txen == MAC_AX_PCIE_ENABLE)
		rval |= B_AX_HCI_TXDMA_EN;
	else if (txen == MAC_AX_PCIE_DISABLE)
		rval &= ~B_AX_HCI_TXDMA_EN;

	if (rxen == MAC_AX_PCIE_ENABLE)
		rval |= B_AX_HCI_RXDMA_EN;
	else if (rxen == MAC_AX_PCIE_DISABLE)
		rval &= ~B_AX_HCI_RXDMA_EN;

	MAC_REG_W32(reg, rval);

	return MACSUCCESS;
}

u32 ctrl_trxdma_pcie_8852b(struct mac_ax_adapter *adapter,
			   enum mac_ax_pcie_func_ctrl txen,
			   enum mac_ax_pcie_func_ctrl rxen,
			   enum mac_ax_pcie_func_ctrl ioen)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_init = R_AX_PCIE_INIT_CFG1;
	u32 val_init = MAC_REG_R32(reg_init);
	u32 reg_stop1 = R_AX_PCIE_DMA_STOP1;
	u32 val_stop1 = MAC_REG_R32(reg_stop1);

	if (txen == MAC_AX_PCIE_ENABLE)
		val_init |= B_AX_TXHCI_EN;
	else if (txen == MAC_AX_PCIE_DISABLE)
		val_init &= ~B_AX_TXHCI_EN;

	if (rxen == MAC_AX_PCIE_ENABLE)
		val_init |= B_AX_RXHCI_EN;
	else if (rxen == MAC_AX_PCIE_DISABLE)
		val_init &= ~B_AX_RXHCI_EN;

	if (ioen == MAC_AX_PCIE_ENABLE)
		val_stop1 &= ~B_AX_STOP_PCIEIO;
	else if (ioen == MAC_AX_PCIE_DISABLE)
		val_stop1 |= B_AX_STOP_PCIEIO;

	MAC_REG_W32(reg_init, val_init);
	MAC_REG_W32(reg_stop1, val_stop1);

	return MACSUCCESS;
}

u32 ctrl_txdma_ch_pcie_8852b(struct mac_ax_adapter *adapter,
			     struct mac_ax_txdma_ch_map *ch_map)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_stop1 = R_AX_PCIE_DMA_STOP1;
	u32 val32;

	val32 = MAC_REG_R32(reg_stop1);
	if (ch_map->ch0 == MAC_AX_PCIE_ENABLE)
		val32 &= ~B_AX_STOP_ACH0;
	else if (ch_map->ch0 == MAC_AX_PCIE_DISABLE)
		val32 |= B_AX_STOP_ACH0;

	if (ch_map->ch1 == MAC_AX_PCIE_ENABLE)
		val32 &= ~B_AX_STOP_ACH1;
	else if (ch_map->ch1 == MAC_AX_PCIE_DISABLE)
		val32 |= B_AX_STOP_ACH1;

	if (ch_map->ch2 == MAC_AX_PCIE_ENABLE)
		val32 &= ~B_AX_STOP_ACH2;
	else if (ch_map->ch2 == MAC_AX_PCIE_DISABLE)
		val32 |= B_AX_STOP_ACH2;

	if (ch_map->ch3 == MAC_AX_PCIE_ENABLE)
		val32 &= ~B_AX_STOP_ACH3;
	else if (ch_map->ch3 == MAC_AX_PCIE_DISABLE)
		val32 |= B_AX_STOP_ACH3;

	if (ch_map->ch8 == MAC_AX_PCIE_ENABLE)
		val32 &= ~B_AX_STOP_CH8;
	else if (ch_map->ch8 == MAC_AX_PCIE_DISABLE)
		val32 |= B_AX_STOP_CH8;

	if (ch_map->ch9 == MAC_AX_PCIE_ENABLE)
		val32 &= ~B_AX_STOP_CH9;
	else if (ch_map->ch9 == MAC_AX_PCIE_DISABLE)
		val32 |= B_AX_STOP_CH9;

	if (ch_map->ch12 == MAC_AX_PCIE_ENABLE)
		val32 &= ~B_AX_STOP_CH12;
	else if (ch_map->ch12 == MAC_AX_PCIE_DISABLE)
		val32 |= B_AX_STOP_CH12;
	MAC_REG_W32(reg_stop1, val32);

	return MACSUCCESS;
}

u32 set_pcie_speed_8852b(struct mac_ax_adapter *adapter,
			 enum mac_ax_pcie_phy set_speed)
{
	u32 ret;
	u32 support_gen;
	u32 cnt;
	u32 poll_val32;
	u8 link_speed;

	if  (!(set_speed == MAC_AX_PCIE_PHY_GEN1 || set_speed == MAC_AX_PCIE_PHY_GEN2)) {
		PLTFM_MSG_ERR("[ERR]Do not support that speed!\n");
		return MACFUNCINPUT;
	}

	support_gen = get_pcie_sup_speed_8852b(adapter);

	if (support_gen == MAC_AX_PCIE_PHY_GEN1) {
		if (set_speed == MAC_AX_PCIE_PHY_GEN1) {
			ret = MACSUCCESS;
		} else {
			PLTFM_MSG_ERR("[ERR]Do not support that gen!\n");
			ret = MACFUNCINPUT;
		}
	} else if (support_gen == MAC_AX_PCIE_PHY_GEN2) {
		ret = get_pcie_speed_8852b(adapter, &link_speed);
		if (link_speed == set_speed) {
			ret = MACSUCCESS;
		} else {
			ret = dbi_w32_pcie(adapter, PCIE_LINK_CHANGE_SPEED, set_speed);
			if (ret != MACSUCCESS)
				return ret;
			ret = dbi_r32_pcie(adapter, PCIE_FTS, &poll_val32);
			if (ret != MACSUCCESS)
				return ret;
			ret = dbi_w32_pcie(adapter, PCIE_FTS, poll_val32 | PCIE_POLLING_BIT);
			if (ret != MACSUCCESS)
				return ret;

			cnt = PCIE_POLL_SPEED_CHANGE_CNT;
			while (cnt) {
				ret = dbi_r32_pcie(adapter, PCIE_FTS, &poll_val32);
				if (ret != MACSUCCESS)
					return ret;
				if (!(poll_val32 & PCIE_POLLING_BIT))
					break;
				cnt--;
				PLTFM_DELAY_US(PCIE_POLL_IO_IDLE_DLY_US);
			}
			if (!cnt) {
				PLTFM_MSG_ERR("[ERR]PCIE polling timeout\n");
				return MACPOLLTO;
			}
		}
	} else {
		PLTFM_MSG_ERR("[ERR]Do Not support that speed gen!\n");
		ret = MACFUNCINPUT;
	}

	return ret;
}

u32 get_pcie_speed_8852b(struct mac_ax_adapter *adapter,
			 u8 *speed)
{
	u32 ret;
	u32 val32;

	ret = dbi_r32_pcie(adapter, PCIE_LINK_SPEED_32BIT, &val32);
	if (ret != MACSUCCESS)
		return ret;
	val32 = GET_FIEL2(val32, PCIE_LINK_SPEED_SH, PCIE_LINK_SPEED_BITS_MSK);

	if (val32 == MAC_AX_PCIE_PHY_GEN1) {
		*speed = MAC_AX_PCIE_PHY_GEN1;
	} else if (val32 == MAC_AX_PCIE_PHY_GEN2) {
		*speed = MAC_AX_PCIE_PHY_GEN2;
	} else {
		ret = MACFUNCINPUT;
		PLTFM_MSG_ERR("[ERR]Do not support that speed!\n");
	}

	return ret;
}

u32 get_pcie_sup_speed_8852b(struct mac_ax_adapter *adapter)
{
	u32 ret;
	u32 support_gen;

	ret = dbi_r32_pcie(adapter, PCIE_CAPABILITY_SPEED, &support_gen);
	if (ret != MACSUCCESS)
		return ret;
	support_gen = GET_FIEL2(support_gen, PCIE_SUPPORT_GEN_SH, PCIE_LINK_SPEED_BITS_MSK);
	PLTFM_MSG_TRACE("pcie support highest link speed: %d\n", support_gen);

	return support_gen;
}

u32 ctrl_wpdma_pcie_8852b(struct mac_ax_adapter *adapter,
			  enum mac_ax_pcie_func_ctrl wpen)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_stop1 = R_AX_PCIE_DMA_STOP1;
	u32 val32;

	val32 = MAC_REG_R32(reg_stop1);
	if (wpen == MAC_AX_PCIE_ENABLE)
		val32 &= ~B_AX_STOP_WPDMA;
	else if (wpen == MAC_AX_PCIE_DISABLE)
		val32 |= B_AX_STOP_WPDMA;
	MAC_REG_W32(reg_stop1, val32);

	return MACSUCCESS;
}

u32 poll_io_idle_pcie_8852b(struct mac_ax_adapter *adapter)
{
#define B_IO_BUSY (B_AX_PCIEIO_BUSY | B_AX_PCIEIO_TX_BUSY | B_AX_PCIEIO_RX_BUSY)
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_busy1 = R_AX_PCIE_DMA_BUSY1;
	u32 val32;
	u32 cnt;

	cnt = PCIE_POLL_IO_IDLE_CNT;
	while (cnt) {
		val32 = MAC_REG_R32(reg_busy1);
		if (!(val32 & B_IO_BUSY))
			break;
		cnt--;
		PLTFM_DELAY_US(PCIE_POLL_IO_IDLE_DLY_US);
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]PCIE dmach busy1 0x%X\n", val32);
		return MACPOLLTO;
	}

	return MACSUCCESS;
}

u32 poll_txdma_ch_idle_pcie_8852b(struct mac_ax_adapter *adapter,
				  struct mac_ax_txdma_ch_map *ch_map)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_busy1 = R_AX_PCIE_DMA_BUSY1;
	u32 val32 = 0, rval32, cnt;

	if (ch_map->ch0 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_ACH0_BUSY;

	if (ch_map->ch1 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_ACH1_BUSY;

	if (ch_map->ch2 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_ACH2_BUSY;

	if (ch_map->ch3 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_ACH3_BUSY;

	if (ch_map->ch8 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CH8_BUSY;

	if (ch_map->ch9 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CH9_BUSY;

	if (ch_map->ch12 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CH12_BUSY;

	cnt = PCIE_POLL_DMACH_IDLE_CNT;
	while (cnt) {
		rval32 = MAC_REG_R32(reg_busy1);
		if (!(rval32 & val32))
			break;
		cnt--;
		PLTFM_DELAY_US(PCIE_POLL_DMACH_IDLE_DLY_US);
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]PCIE dmach busy1 0x%X\n", rval32);
		return MACPOLLTO;
	}

	return MACSUCCESS;
}

u32 poll_rxdma_ch_idle_pcie_8852b(struct mac_ax_adapter *adapter,
				  struct mac_ax_rxdma_ch_map *ch_map)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_busy1 = R_AX_PCIE_DMA_BUSY1;
	u32 val32 = 0, rval32, cnt;

	if (ch_map->rxq == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_RXQ_BUSY;
	if (ch_map->rpq == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_RPQ_BUSY;

	cnt = PCIE_POLL_DMACH_IDLE_CNT;
	while (cnt) {
		rval32 = MAC_REG_R32(reg_busy1);
		if (!(rval32 & val32))
			break;
		cnt--;
		PLTFM_DELAY_US(PCIE_POLL_DMACH_IDLE_DLY_US);
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]PCIE dmach busy1 0x%X\n", rval32);
		return MACPOLLTO;
	}

	return MACSUCCESS;
}

u32 poll_dma_all_idle_pcie_8852b(struct mac_ax_adapter *adapter)
{
	struct mac_ax_txdma_ch_map txch_map;
	struct mac_ax_rxdma_ch_map rxch_map;
	u32 ret;

	txch_map.ch0 = MAC_AX_PCIE_ENABLE;
	txch_map.ch1 = MAC_AX_PCIE_ENABLE;
	txch_map.ch2 = MAC_AX_PCIE_ENABLE;
	txch_map.ch3 = MAC_AX_PCIE_ENABLE;
	txch_map.ch8 = MAC_AX_PCIE_ENABLE;
	txch_map.ch9 = MAC_AX_PCIE_ENABLE;
	txch_map.ch12 = MAC_AX_PCIE_ENABLE;
	ret = poll_txdma_ch_idle_pcie_8852b(adapter, &txch_map);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]PCIE poll txdma all ch idle %d\n", ret);
		return ret;
	}

	rxch_map.rxq = MAC_AX_PCIE_ENABLE;
	rxch_map.rpq = MAC_AX_PCIE_ENABLE;
	ret = poll_rxdma_ch_idle_pcie_8852b(adapter, &rxch_map);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]PCIE poll rxdma all ch idle %d\n", ret);
		return ret;
	}

	return ret;
}

u32 clr_idx_ch_pcie_8852b(struct mac_ax_adapter *adapter,
			  struct mac_ax_txdma_ch_map *txch_map,
			  struct mac_ax_rxdma_ch_map *rxch_map)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_clr_tx1 = R_AX_TXBD_RWPTR_CLR1;
	u32 reg_clr_rx = R_AX_RXBD_RWPTR_CLR;
	u32 val32 = 0;

	if (txch_map->ch0 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_ACH0_IDX;

	if (txch_map->ch1 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_ACH1_IDX;

	if (txch_map->ch2 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_ACH2_IDX;

	if (txch_map->ch3 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_ACH3_IDX;

	if (txch_map->ch8 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_CH8_IDX;

	if (txch_map->ch9 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_CH9_IDX;

	if (txch_map->ch12 == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_CH12_IDX;

	MAC_REG_W32(reg_clr_tx1, val32);

	val32 = 0;
	if (rxch_map->rxq == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_RXQ_IDX;
	if (rxch_map->rpq == MAC_AX_PCIE_ENABLE)
		val32 |= B_AX_CLR_RPQ_IDX;

	MAC_REG_W32(reg_clr_rx, val32);

	return MACSUCCESS;
}

u32 rst_bdram_pcie_8852b(struct mac_ax_adapter *adapter, u8 val)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_init = R_AX_PCIE_INIT_CFG1;
	u32 reg_ps = R_AX_PCIE_PS_CTRL;
	u32 cnt, val32;

	MAC_REG_W32(reg_init, MAC_REG_R32(reg_init) | B_AX_RST_BDRAM);

	MAC_REG_W32(reg_ps, MAC_REG_R32(reg_ps) | B_AX_PCIE_FORCE_L0);

	cnt = PCIE_POLL_BDRAM_RST_CNT;
	while (cnt) {
		val32 = MAC_REG_R32(reg_init);
		if (!(val32 & B_AX_RST_BDRAM))
			break;
		cnt--;
		PLTFM_DELAY_US(PCIE_POLL_BDRAM_RST_DLY_US);
	}

	MAC_REG_W32(reg_ps, MAC_REG_R32(reg_ps) & ~B_AX_PCIE_FORCE_L0);

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]rst bdram timeout 0x%X\n", val32);
		return MACPOLLTO;
	}

	return MACSUCCESS;
}

u32 trx_mit_pcie_8852b(struct mac_ax_adapter *adapter,
		       struct mac_ax_pcie_trx_mitigation *mit_info)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u8 tmr_unit = 0;
	u32 value32 = 0;

	if (mit_info->txch_map->ch0 == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_TXMIT_ACH0_SEL;
	else if (mit_info->txch_map->ch0 == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_TXMIT_ACH0_SEL;

	if (mit_info->txch_map->ch1 == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_TXMIT_ACH1_SEL;
	else if (mit_info->txch_map->ch1 == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_TXMIT_ACH1_SEL;

	if (mit_info->txch_map->ch2 == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_TXMIT_ACH2_SEL;
	else if (mit_info->txch_map->ch2 == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_TXMIT_ACH2_SEL;

	if (mit_info->txch_map->ch3 == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_TXMIT_ACH3_SEL;
	else if (mit_info->txch_map->ch3 == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_TXMIT_ACH3_SEL;

	if (mit_info->txch_map->ch8 == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_TXMIT_CH8_SEL;
	else if (mit_info->txch_map->ch8 == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_TXMIT_CH8_SEL;

	if (mit_info->txch_map->ch9 == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_TXMIT_CH9_SEL;
	else if (mit_info->txch_map->ch9 == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_TXMIT_CH9_SEL;

	if (mit_info->txch_map->ch12 == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_TXMIT_CH12_SEL;
	else if (mit_info->txch_map->ch12 == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_TXMIT_CH12_SEL;

	switch (mit_info->tx_timer_unit) {
	case MAC_AX_MIT_64US:
		tmr_unit = 0;
		break;
	case MAC_AX_MIT_128US:
		tmr_unit = 1;
		break;
	case MAC_AX_MIT_256US:
		tmr_unit = 2;
		break;
	case MAC_AX_MIT_512US:
		tmr_unit = 3;
		break;
	default:
		PLTFM_MSG_WARN("[WARN]Set TX MIT timer unit fail\n");
		break;
	}

	value32 = SET_CLR_WORD(value32, tmr_unit, B_AX_TXTIMER_UNIT);
	value32 = SET_CLR_WORD(value32, mit_info->tx_counter, B_AX_TXCOUNTER_MATCH);
	value32 = SET_CLR_WORD(value32, mit_info->tx_timer, B_AX_TXTIMER_MATCH);
	MAC_REG_W32(R_AX_INT_MIT_TX, value32);

	value32 = 0;
	if (mit_info->rxch_map->rxq == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_RXMIT_RXP2_SEL;
	else if (mit_info->rxch_map->rxq == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_RXMIT_RXP2_SEL;

	if (mit_info->rxch_map->rpq == MAC_AX_PCIE_ENABLE)
		value32 |= B_AX_RXMIT_RXP1_SEL;
	else if (mit_info->rxch_map->rpq == MAC_AX_PCIE_DISABLE)
		value32 &= ~B_AX_RXMIT_RXP1_SEL;

	switch (mit_info->rx_timer_unit) {
	case MAC_AX_MIT_64US:
		tmr_unit = 0;
		break;
	case MAC_AX_MIT_128US:
		tmr_unit = 1;
		break;
	case MAC_AX_MIT_256US:
		tmr_unit = 2;
		break;
	case MAC_AX_MIT_512US:
		tmr_unit = 3;
		break;
	default:
		PLTFM_MSG_WARN("[WARN]Set RX MIT timer unit fail\n");
		break;
	}

	value32 = SET_CLR_WORD(value32, tmr_unit, B_AX_RXTIMER_UNIT);
	value32 = SET_CLR_WORD(value32, mit_info->rx_counter, B_AX_RXCOUNTER_MATCH);
	value32 = SET_CLR_WORD(value32, mit_info->rx_timer, B_AX_RXTIMER_MATCH);
	MAC_REG_W32(R_AX_INT_MIT_RX, value32);

	return MACSUCCESS;
}

u32 mode_op_pcie_8852b(struct mac_ax_adapter *adapter,
		       struct mac_ax_intf_info *intf_info)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	enum mac_ax_rxbd_mode *rxbd_mode = (&intf_info->rxbd_mode);
	enum mac_ax_tag_mode *tag_mode = (&intf_info->tag_mode);
	enum mac_ax_multi_tag_num *multi_tag_num = (&intf_info->multi_tag_num);
	enum mac_ax_wd_dma_intvl *wd_dma_idle_intvl =
		(&intf_info->wd_dma_idle_intvl);
	enum mac_ax_wd_dma_intvl *wd_dma_act_intvl =
		(&intf_info->wd_dma_act_intvl);
	enum mac_ax_tx_burst *tx_burst = &intf_info->tx_burst;
	enum mac_ax_rx_burst *rx_burst = &intf_info->rx_burst;
	struct mac_ax_intf_info *intf_info_def;
	u32 reg_init1 = R_AX_PCIE_INIT_CFG1;
	u32 reg_init2 = R_AX_PCIE_INIT_CFG2;
	u32 reg_exp = R_AX_PCIE_EXP_CTRL;
	u32 val32_init1, val32_init2, val32_exp;

	intf_info_def = p_ops->get_pcie_info_def(adapter);
	if (!intf_info_def) {
		PLTFM_MSG_ERR("%s: NULL intf_info\n", __func__);
		return MACNPTR;
	}

	if (intf_info->rxbd_mode == MAC_AX_RXBD_DEF)
		rxbd_mode = (&intf_info_def->rxbd_mode);
	if (intf_info->tx_burst == MAC_AX_TX_BURST_DEF)
		tx_burst = &intf_info_def->tx_burst;
	if (intf_info->rx_burst == MAC_AX_RX_BURST_DEF)
		rx_burst = &intf_info_def->rx_burst;
	if (intf_info->tag_mode == MAC_AX_TAG_DEF)
		tag_mode = (&intf_info_def->tag_mode);
	if (intf_info->multi_tag_num == MAC_AX_TAG_NUM_DEF)
		multi_tag_num = (&intf_info_def->multi_tag_num);
	if (intf_info->wd_dma_act_intvl == MAC_AX_WD_DMA_INTVL_DEF)
		wd_dma_act_intvl = (&intf_info_def->wd_dma_act_intvl);
	if (intf_info->wd_dma_idle_intvl == MAC_AX_WD_DMA_INTVL_DEF)
		wd_dma_idle_intvl = (&intf_info_def->wd_dma_idle_intvl);

	val32_init1 = MAC_REG_R32(reg_init1);
	val32_init2 = MAC_REG_R32(reg_init2);
	val32_exp = MAC_REG_R32(reg_exp);

	if ((*rxbd_mode) == MAC_AX_RXBD_PKT) {
		val32_init1 &= ~B_AX_RXBD_MODE;
	} else if ((*rxbd_mode) == MAC_AX_RXBD_SEP) {
		if (intf_info->rx_sep_append_len > B_AX_PCIE_RX_APPLEN_MSK) {
			PLTFM_MSG_ERR("rx sep app len %d\n",
				      intf_info->rx_sep_append_len);
			return MACFUNCINPUT;
		}

		val32_init1 |= B_AX_RXBD_MODE;
		val32_init2 = SET_CLR_WORD(val32_init2, intf_info->rx_sep_append_len,
					   B_AX_PCIE_RX_APPLEN);
	}

	val32_init1 = SET_CLR_WORD(val32_init1, *tx_burst, B_AX_PCIE_MAX_TXDMA);
	val32_init1 = SET_CLR_WORD(val32_init1, *rx_burst, B_AX_PCIE_MAX_RXDMA);

	if ((*tag_mode) == MAC_AX_TAG_SGL)
		val32_init1 &= ~B_AX_LATENCY_CONTROL;
	else if ((*tag_mode) == MAC_AX_TAG_MULTI)
		val32_init1 |= B_AX_LATENCY_CONTROL;

	val32_exp = SET_CLR_WORD(val32_exp, *multi_tag_num, B_AX_MAX_TAG_NUM);

	val32_init2 = SET_CLR_WORD(val32_init2, *wd_dma_idle_intvl, B_AX_WD_ITVL_IDLE);
	val32_init2 = SET_CLR_WORD(val32_init2, *wd_dma_act_intvl, B_AX_WD_ITVL_ACT);

	MAC_REG_W32(reg_init1, val32_init1);
	MAC_REG_W32(reg_init2, val32_init2);
	MAC_REG_W32(reg_exp, val32_exp);

	return MACSUCCESS;
}

u32 get_err_flag_pcie_8852b(struct mac_ax_adapter *adapter,
			    struct mac_ax_pcie_err_info *out_info)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 val32 = MAC_REG_R32(R_AX_DBG_ERR_FLAG);

	out_info->txbd_len_zero = (val32 & B_AX_PCIE_TXBD_LEN0) ? 1 : 0;
	out_info->tx_stuck = (val32 & B_AX_TX_STUCK) ? 1 : 0;
	out_info->rx_stuck = (val32 & B_AX_RX_STUCK) ? 1 : 0;

	return MACSUCCESS;
}

static u32 get_target_8852b(struct mac_ax_adapter *adapter, u16 *target,
			    enum mac_ax_pcie_phy phy_rate)
{
	u16 tmp_u16, count;
	u16 tar;
	u32 ret = MACSUCCESS;

	/* Enable counter */
	ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &tmp_u16);
	if (ret != MACSUCCESS)
		return ret;
	ret = mdio_w16_pcie(adapter, RAC_CTRL_PPR_V1, tmp_u16 & ~BAC_AUTOK_ONCE_EN,
			    phy_rate);
	if (ret != MACSUCCESS)
		return ret;
	ret = mdio_w16_pcie(adapter, RAC_CTRL_PPR_V1, tmp_u16 | BAC_AUTOK_ONCE_EN,
			    phy_rate);
	if (ret != MACSUCCESS)
		return ret;

	ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &tar);
	if (ret != MACSUCCESS)
		return ret;

	count = PCIE_POLL_AUTOK_CNT;
	while (count && (tar & BAC_AUTOK_ONCE_EN)) {
		ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &tar);
		if (ret != MACSUCCESS)
			return ret;
		count--;
		PLTFM_DELAY_US(PCIE_POLL_AUTOK_DLY_US);
	}

	if (!count) {
		PLTFM_MSG_ERR("[ERR]AutoK get target timeout: %X\n", tar);
		return MACPOLLTO;
	}

	tar = tar & BAC_AUTOK_HW_TAR_MSK;
	if (tar == 0 || tar == BAC_AUTOK_HW_TAR_MSK) {
		PLTFM_MSG_ERR("[ERR]Get target failed.\n");
		return MACHWERR;
	}

	*target = tar;
	return MACSUCCESS;
}

u32 mac_auto_refclk_cal_pcie_8852b(struct mac_ax_adapter *adapter,
				   enum mac_ax_pcie_func_ctrl en)
{
	u8 bdr_ori, val8;
	u16 tmp_u16;
	u16 mgn_set;
	u16 tar;
	u8 l1_flag = 0;
	u32 ret = MACSUCCESS;
	enum mac_ax_pcie_phy phy_rate = MAC_AX_PCIE_PHY_GEN1;

	if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
		return MACSUCCESS;

#if (INTF_INTGRA_HOSTREF_V1 <= INTF_INTGRA_MINREF_V1)
	PLTFM_MSG_ERR("[ERR]INTF_INTGRA_MINREF_V1 define fail\n");
	return MACCMP;
#endif
	ret = dbi_r8_pcie(adapter, PCIE_PHY_RATE, &val8);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]dbi_r8_pcie 0x%x\n", PCIE_PHY_RATE);
		return ret;
	}

	if ((val8 & (BIT1 | BIT0)) == 0x1) {
		phy_rate = MAC_AX_PCIE_PHY_GEN1;
	} else if ((val8 & (BIT1 | BIT0)) == 0x2) {
		phy_rate = MAC_AX_PCIE_PHY_GEN2;
	} else {
		PLTFM_MSG_ERR("[ERR]PCIe PHY rate not support\n");
		return MACHWNOSUP;
	}

	/* Disable L1BD */
	ret = dbi_r8_pcie(adapter, PCIE_L1_CTRL, &bdr_ori);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]dbi_r8_pcie 0x%X\n", PCIE_L1_CTRL);
		return ret;
	}

	if (bdr_ori & PCIE_BIT_L1) {
		ret = dbi_w8_pcie(adapter, PCIE_L1_CTRL,
				  bdr_ori & ~(PCIE_BIT_L1));
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]dbi_w8_pcie 0x%X\n", PCIE_L1_CTRL);
			return ret;
		}
		l1_flag = 1;
	}

	/* Disable function */
	ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &tmp_u16);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]mdio_r16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

	if (tmp_u16 & BAC_AUTOK_EN) {
		ret = mdio_w16_pcie(adapter, RAC_CTRL_PPR_V1,
				    tmp_u16 & ~(BAC_AUTOK_EN), phy_rate);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]mdio_w16_pcie 0x%X\n",
				      RAC_CTRL_PPR_V1);
			goto end;
		}
	}

	if (en != MAC_AX_PCIE_ENABLE)
		goto end;

	/* Set div */
	ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &tmp_u16);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]mdio_r16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

	tmp_u16 = SET_CLR_WOR2(tmp_u16, PCIE_AUTOK_DIV_2048, BAC_AUTOK_DIV_SH,
			       BAC_AUTOK_DIV_MSK);
	ret = mdio_w16_pcie(adapter, RAC_CTRL_PPR_V1, tmp_u16, phy_rate);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]mdio_w16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

	/*  Obtain div and margin */
	ret = get_target_8852b(adapter, &tar, phy_rate);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]1st get target fail %d\n", ret);
		goto end;
	}

	mgn_set = PCIE_AUTOK_MGN;

	ret = mdio_w16_pcie(adapter, RAC_SET_PPR_V1, (tar & BAC_AUTOK_TAR_MSK) |
			    (mgn_set << BAC_AUTOK_MGN_SH), phy_rate);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]mdio_w16_pcie 0x%X\n", RAC_SET_PPR_V1);
		goto end;
	}

	/* Enable function */
	ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &tmp_u16);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]mdio_r16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
		goto end;
	}
	ret = mdio_w16_pcie(adapter, RAC_CTRL_PPR_V1, tmp_u16 | BAC_AUTOK_EN,
			    phy_rate);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]mdio_w16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
		goto end;
	}

end:
	/* Set L1BD to ori */
	if (l1_flag == 1) {
		ret = dbi_w8_pcie(adapter, PCIE_L1_CTRL, bdr_ori);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]dbi_w8_pcie 0x%X\n", PCIE_L1_CTRL);
			return ret;
		}
	}
	PLTFM_MSG_TRACE("[TRACE]%s: <==\n", __func__);

	return ret;
}

#endif /* #if MAC_AX_PCIE_SUPPORT */
#endif /* #if MAC_AX_8852B_SUPPORT */

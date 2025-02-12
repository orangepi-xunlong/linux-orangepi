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
#include "_pcie.h"
#include "pwr.h"
#include "mac_priv.h"

#if MAC_AX_PCIE_SUPPORT

static struct mac_ax_processor_id proc_id_long_dly[PROC_ID_LIST_NUM] = {
	/* Cezanne & Barcelo */ {0x178BFBFF, 0x00A50F00},
	/* Rembrandt */ {0x178BFBFF, 0x00A40F40}
};

static u8 base_board_id_short_dly[BASE_BOARD_ID_SHORT_LIST_NUM][BASE_BOARD_ID_LEN] = {
	{0x38, 0x41, 0x43, 0x33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC3
	{0x38, 0x41, 0x43, 0x36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC6
	{0x38, 0x41, 0x43, 0x39, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC9
	{0x38, 0x41, 0x43, 0x42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8ACB
	{0x38, 0x41, 0x43, 0x43, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8ACC
	{0x38, 0x41, 0x43, 0x44, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8ACD
	{0x38, 0x41, 0x43, 0x45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8ACE
	{0x38, 0x41, 0x43, 0x30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC0
	{0x38, 0x41, 0x43, 0x31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC1
	{0x38, 0x41, 0x43, 0x32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC2
	{0x38, 0x42, 0x39, 0x39, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8B39
	{0x38, 0x41, 0x43, 0x34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC4
	{0x38, 0x41, 0x43, 0x35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC5
	{0x38, 0x41, 0x43, 0x37, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC7
	{0x38, 0x41, 0x43, 0x38, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8AC8
	{0x38, 0x39, 0x34, 0x46, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//894F
	{0x38, 0x39, 0x35, 0x32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8952
	{0x38, 0x39, 0x35, 0x35, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8955
	{0x38, 0x39, 0x35, 0x37, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8957
	{0x38, 0x39, 0x35, 0x46, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//895F
	{0x38, 0x39, 0x35, 0x38, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8958
	{0x38, 0x39, 0x35, 0x39, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8959
	{0x38, 0x39, 0x35, 0x31, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8951
	{0x38, 0x39, 0x35, 0x30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8950
	{0x38, 0x39, 0x35, 0x34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8954
	{0x38, 0x39, 0x35, 0x33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//8953
	{0x38, 0x39, 0x34, 0x44, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//894D
	{0x38, 0x39, 0x34, 0x45, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,\
		0, 0, 0, 0, 0, 0},	//894E
};

static u32 c_wow_ldo_id[C_WOW_LDO_ID_LIST_NUM] = {
	/* Fujitsu */ 0x709
};

static u32 tx_bd_reg_aon[HAXIDMA_SYNC_TX_CH_NUM] = {R_AX_DRV_FW_HSK_0, R_AX_DRV_FW_HSK_1,
						    R_AX_DRV_FW_HSK_2, R_AX_DRV_FW_HSK_3,
						    R_AX_DRV_FW_HSK_4, R_AX_DRV_FW_HSK_5};
static u32 tx_bd_reg_off[HAXIDMA_SYNC_TX_CH_NUM] = {R_AX_ACH0_TXBD_IDX, R_AX_ACH1_TXBD_IDX,
						    R_AX_ACH2_TXBD_IDX, R_AX_ACH3_TXBD_IDX,
						    R_AX_CH8_TXBD_IDX, R_AX_CH12_TXBD_IDX};

static u32 rx_bd_reg_aon[HAXIDMA_SYNC_RX_CH_NUM] = {R_AX_DRV_FW_HSK_6, R_AX_DRV_FW_HSK_7};
static u32 rx_bd_reg_off[HAXIDMA_SYNC_RX_CH_NUM] = {R_AX_RXQ_RXBD_IDX_V1, R_AX_RPQ_RXBD_IDX_V1};

static u8 patch_cmac_io_r8(struct mac_ax_adapter *adapter, u32 addr)
{
	u8 offset, count = 0, val8;
	u32 val, addr_shift;

	if (addr >= R_AX_CMAC_FUNC_EN && addr <= R_AX_CMAC_REG_END) {
		offset = addr & (MAC_REG_OFFSET - 1);
		addr_shift = addr - offset;
		val = PLTFM_REG_R32(addr_shift);

		while (count < MAC_REG_POOL_COUNT) {
			if (val != MAC_AX_R32_DEAD)
				break;

			PLTFM_MSG_ERR("[ERR]addr 0x%x = 0xdeadbeef\n", addr);
			PLTFM_REG_W32(R_AX_CK_EN, CMAC_CLK_ALLEN);
			val = PLTFM_REG_R32(addr_shift);
			count++;
		}

		val8 = (u8)(val >> (offset << MAC_REG_OFFSET_SH_2));
	} else {
		val8 = PLTFM_REG_R8(addr);
	}

	if (count == MAC_REG_POOL_COUNT && adapter->sm.l2_st == MAC_AX_L2_EN) {
		adapter->sm.l2_st = MAC_AX_L2_TRIG;
		PLTFM_L2_NOTIFY(void);
	}

	return val8;
}

static u16 patch_cmac_io_r16(struct mac_ax_adapter *adapter, u32 addr)
{
	u8 offset, count = 0;
	u16 val16;
	u32 val, addr_shift;

	if (addr >= R_AX_CMAC_FUNC_EN && addr <= R_AX_CMAC_REG_END) {
		offset = addr & (MAC_REG_OFFSET - 1);
		addr_shift = addr - offset;
		val = PLTFM_REG_R32(addr_shift);

		while (count < MAC_REG_POOL_COUNT) {
			if (val != MAC_AX_R32_DEAD)
				break;

			PLTFM_MSG_ERR("[ERR]addr 0x%x = 0xdeadbeef\n", addr);
			PLTFM_REG_W32(R_AX_CK_EN, CMAC_CLK_ALLEN);
			val = PLTFM_REG_R32(addr_shift);
			count++;
		}
		val16 = (u16)(val >> (offset << MAC_REG_OFFSET_SH_2));
	} else {
		val16 = PLTFM_REG_R16(addr);
	}

	if (count == MAC_REG_POOL_COUNT && adapter->sm.l2_st == MAC_AX_L2_EN) {
		adapter->sm.l2_st = MAC_AX_L2_TRIG;
		PLTFM_L2_NOTIFY(void);
	}

	return val16;
}

static u32 patch_cmac_io_r32(struct mac_ax_adapter *adapter, u32 addr)
{
	u8 count = 0;
	u32 val = PLTFM_REG_R32(addr);

	if (addr >= R_AX_CMAC_FUNC_EN && addr <= R_AX_CMAC_REG_END) {
		while (count < MAC_REG_POOL_COUNT) {
			if (val != MAC_AX_R32_DEAD)
				break;

			PLTFM_MSG_ERR("[ERR]addr 0x%x = 0xdeadbeef\n", addr);
			PLTFM_REG_W32(R_AX_CK_EN, CMAC_CLK_ALLEN);
			val = PLTFM_REG_R32(addr);
			count++;
		}
	}

	if (count == MAC_REG_POOL_COUNT && adapter->sm.l2_st == MAC_AX_L2_EN) {
		adapter->sm.l2_st = MAC_AX_L2_TRIG;
		PLTFM_L2_NOTIFY(void);
	}

	return val;
}

static u32 sync_tx_bd_idx_ax(struct mac_ax_adapter *adapter)
{
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	u8 ch;
	u16 val16_bd_off, val16_bd_aon;
	u32 val32_bd_off, val32_bd_aon;
	u32 bd_reg_off, bd_reg_aon;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	for (ch = 0; ch < HAXIDMA_SYNC_TX_CH_NUM; ch++) {
		bd_reg_off = tx_bd_reg_off[ch];
		bd_reg_aon = tx_bd_reg_aon[ch];
		val32_bd_off = MAC_REG_R32(bd_reg_off);
		val32_bd_aon = MAC_REG_R32(bd_reg_aon);

		val16_bd_off = GET_FIELD(val32_bd_aon, HOST_BD_IDX);
		MAC_REG_W16(bd_reg_off, val16_bd_off);

		val16_bd_aon = GET_FIELD(val32_bd_off, HW_BD_IDX);
		MAC_REG_W16(bd_reg_aon + 2, val16_bd_aon);
	}
#endif
	return MACSUCCESS;
}

static u32 sync_rx_bd_idx_ax(struct mac_ax_adapter *adapter)
{
	u8 ch;
	u16 val16_bd_off, val16_bd_aon;
	u32 val32_bd_off, val32_bd_aon;
	u32 bd_reg_off, bd_reg_aon;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	for (ch = 0; ch < HAXIDMA_SYNC_RX_CH_NUM; ch++) {
		bd_reg_off = rx_bd_reg_off[ch];
		bd_reg_aon = rx_bd_reg_aon[ch];
		val32_bd_off = MAC_REG_R32(bd_reg_off);
		val32_bd_aon = MAC_REG_R32(bd_reg_aon);

		val16_bd_off = GET_FIELD(val32_bd_aon, HOST_BD_IDX);
		MAC_REG_W16(bd_reg_off, val16_bd_off);

		val16_bd_aon = GET_FIELD(val32_bd_off, HW_BD_IDX);
		MAC_REG_W16(bd_reg_aon + 2, val16_bd_aon);
	}

	return MACSUCCESS;
}

u8 reg_read8_pcie(struct mac_ax_adapter *adapter, u32 addr)
{
	u8 val8;

	if (chk_patch_cmac_io_fail(adapter) == PATCH_DISABLE)
		val8 = PLTFM_REG_R8(addr);
	else
		val8 = patch_cmac_io_r8(adapter, addr);

	return val8;
}

void reg_write8_pcie(struct mac_ax_adapter *adapter, u32 addr, u8 val)
{
	PLTFM_REG_W8(addr, val);
}

u16 reg_read16_pcie(struct mac_ax_adapter *adapter, u32 addr)
{
	u16 val16;

	if ((addr & (MAC_REG_OFFSET16 - 1)) != 0) {
		PLTFM_MSG_ERR("[ERR]read16 failaddr 0x%x\n", addr);
		return MAC_AX_R16_DEAD;
	}

	if (chk_patch_cmac_io_fail(adapter) == PATCH_DISABLE)
		val16 = PLTFM_REG_R16(addr);
	else
		val16 = patch_cmac_io_r16(adapter, addr);

	return val16;
}

void reg_write16_pcie(struct mac_ax_adapter *adapter, u32 addr, u16 val)
{
	PLTFM_REG_W16(addr, val);
}

u32 reg_read32_pcie(struct mac_ax_adapter *adapter, u32 addr)
{
	u32 val;

	if (chk_patch_cmac_io_fail(adapter) == PATCH_DISABLE)
		val = PLTFM_REG_R32(addr);
	else
		val = patch_cmac_io_r32(adapter, addr);

	return val;
}

void reg_write32_pcie(struct mac_ax_adapter *adapter, u32 addr, u32 val)
{
	PLTFM_REG_W32(addr, val);
}

u32 dbi_r32_pcie(struct mac_ax_adapter *adapter, u16 addr, u32 *val)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 r_addr = addr & DBI_ADDR_MASK;
	u32 val32, cnt;
	u32 ret = MACSUCCESS;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))) {
		PLTFM_MSG_ERR("[ERR]DBI is not supported\n");
		return MACNOTSUP;
	}

	if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
		return MACSUCCESS;

	if (addr & DBI_ADDR_2LSB_MASK) {
		PLTFM_MSG_ERR("[ERR]DBI R32 addr 0x%X not 4B align\n", addr);
		return MACFUNCINPUT;
	}

	PLTFM_MUTEX_LOCK(&adapter->hw_info->dbi_lock);

	val32 = 0;
	val32 = SET_CLR_WORD(val32, r_addr, B_AX_DBI_ADDR);
	MAC_REG_W32(R_AX_DBI_FLAG, val32);

	val32 |= B_AX_DBI_RFLAG;
	MAC_REG_W32(R_AX_DBI_FLAG, val32);

	cnt = DBI_DLY_CNT;
	while (MAC_REG_R32(R_AX_DBI_FLAG) & B_AX_DBI_RFLAG && cnt) {
		PLTFM_DELAY_US(DBI_DLY_US);
		cnt--;
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]DBI R32 0x%X timeout\n", r_addr);
		ret = MACPOLLTO;
		goto end;
	}

	*val = MAC_REG_R32(R_AX_DBI_RDATA);
end:
	PLTFM_MUTEX_UNLOCK(&adapter->hw_info->dbi_lock);
	return ret;
#else
	return MACNOTSUP;
#endif
}

u32 dbi_w32_pcie(struct mac_ax_adapter *adapter, u16 addr, u32 data)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 w_addr = addr & DBI_ADDR_MASK;
	u32 val32, cnt;
	u32 ret = MACSUCCESS;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))) {
		PLTFM_MSG_ERR("[ERR]DBI is not supported\n");
		return MACNOTSUP;
	}

	if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
		return MACSUCCESS;

	PLTFM_MUTEX_LOCK(&adapter->hw_info->dbi_lock);

	MAC_REG_W32(R_AX_DBI_WDATA, data);

	val32 = 0;
	val32 = SET_CLR_WORD(val32, w_addr, B_AX_DBI_ADDR);
	val32 = SET_CLR_WORD(val32, DBI_WEN_DW, B_AX_DBI_WREN);
	MAC_REG_W32(R_AX_DBI_FLAG, val32);

	val32 |= B_AX_DBI_WFLAG;
	MAC_REG_W32(R_AX_DBI_FLAG, val32);

	cnt = DBI_DLY_CNT;
	while (MAC_REG_R32(R_AX_DBI_FLAG) & B_AX_DBI_WFLAG && cnt) {
		PLTFM_DELAY_US(DBI_DLY_US);
		cnt--;
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]DBI W32 0x%X = 0x%x timeout\n", w_addr, data);
		ret = MACPOLLTO;
		goto end;
	}
end:
	PLTFM_MUTEX_UNLOCK(&adapter->hw_info->dbi_lock);
	return ret;
#else
	return MACNOTSUP;
#endif
}

u32 dbi_r8_pcie(struct mac_ax_adapter *adapter, u16 addr, u8 *val)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 r_addr = addr & DBI_ADDR_MASK;
	u32 addr_2lsb = addr & DBI_ADDR_2LSB_MASK;
	u32 val32, cnt;
	u32 ret = MACSUCCESS;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))) {
		PLTFM_MSG_ERR("[ERR]DBI is not supported\n");
		return MACNOTSUP;
	}

	if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
		return MACSUCCESS;

	PLTFM_MUTEX_LOCK(&adapter->hw_info->dbi_lock);

	val32 = 0;
	val32 = SET_CLR_WORD(val32, r_addr, B_AX_DBI_ADDR);
	MAC_REG_W32(R_AX_DBI_FLAG, val32);

	val32 |= B_AX_DBI_RFLAG;
	MAC_REG_W32(R_AX_DBI_FLAG, val32);

	cnt = DBI_DLY_CNT;
	while (MAC_REG_R32(R_AX_DBI_FLAG) & B_AX_DBI_RFLAG && cnt) {
		PLTFM_DELAY_US(DBI_DLY_US);
		cnt--;
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]DBI R8 0x%X timeout\n", r_addr);
		ret = MACPOLLTO;
		goto end;
	}

	*val = MAC_REG_R8(R_AX_DBI_RDATA + addr_2lsb);
end:
	PLTFM_MUTEX_UNLOCK(&adapter->hw_info->dbi_lock);
	return ret;
#else
	return MACNOTSUP;
#endif
}

u32 dbi_w8_pcie(struct mac_ax_adapter *adapter, u16 addr, u8 data)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 w_addr = addr & DBI_ADDR_MASK;
	u32 addr_2lsb = addr & DBI_ADDR_2LSB_MASK;
	u32 val32, cnt;
	u32 ret = MACSUCCESS;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))) {
		PLTFM_MSG_ERR("[ERR]DBI is not supported\n");
		return MACNOTSUP;
	}

	if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
		return MACSUCCESS;

	PLTFM_MUTEX_LOCK(&adapter->hw_info->dbi_lock);

	MAC_REG_W8(R_AX_DBI_WDATA + addr_2lsb, data);

	val32 = 0;
	val32 = SET_CLR_WORD(val32, w_addr, B_AX_DBI_ADDR);
	val32 = SET_CLR_WORD(val32, DBI_WEN_B << addr_2lsb, B_AX_DBI_WREN);
	MAC_REG_W32(R_AX_DBI_FLAG, val32);

	val32 |= B_AX_DBI_WFLAG;
	MAC_REG_W32(R_AX_DBI_FLAG, val32);

	cnt = DBI_DLY_CNT;
	while (MAC_REG_R32(R_AX_DBI_FLAG) & B_AX_DBI_WFLAG && cnt) {
		PLTFM_DELAY_US(DBI_DLY_US);
		cnt--;
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]DBI W8 0x%X = 0x%x timeout\n", w_addr, data);
		ret = MACPOLLTO;
		goto end;
	}
end:
	PLTFM_MUTEX_UNLOCK(&adapter->hw_info->dbi_lock);
	return ret;
#else
	return MACNOTSUP;
#endif
}

u32 mdio_r16_pcie(struct mac_ax_adapter *adapter, u8 addr, u8 speed, u16 *val)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u16 val16;
	u32 cnt;
	u32 ret = MACSUCCESS;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))) {
		PLTFM_MSG_ERR("[ERR]MDIO is not supported\n");
		return MACNOTSUP;
	}

	if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
		return MACSUCCESS;

	PLTFM_MUTEX_LOCK(&adapter->hw_info->mdio_lock);

	MAC_REG_W8(R_AX_MDIO_CFG, addr & B_AX_MDIO_ADDR_MSK);

	val16 = MAC_REG_R16(R_AX_MDIO_CFG);
	if (speed == MAC_AX_PCIE_PHY_GEN1 && addr < MDIO_ADDR_PG1) {
		val16 = SET_CLR_WORD(val16, MDIO_PG0_G1, B_AX_MDIO_PHY_ADDR);
	} else if (speed == MAC_AX_PCIE_PHY_GEN1) {
		val16 = SET_CLR_WORD(val16, MDIO_PG1_G1, B_AX_MDIO_PHY_ADDR);
	} else if (speed == MAC_AX_PCIE_PHY_GEN2 && addr < MDIO_ADDR_PG1) {
		val16 = SET_CLR_WORD(val16, MDIO_PG0_G2, B_AX_MDIO_PHY_ADDR);
	} else if (speed == MAC_AX_PCIE_PHY_GEN2) {
		val16 = SET_CLR_WORD(val16, MDIO_PG1_G2, B_AX_MDIO_PHY_ADDR);
	} else {
		PLTFM_MSG_ERR("[ERR]Error MDIO PHY Speed %d!\n", speed);
		ret = MACFUNCINPUT;
		goto end;
	}
	MAC_REG_W16(R_AX_MDIO_CFG, val16);

	MAC_REG_W16(R_AX_MDIO_CFG,
		    MAC_REG_R16(R_AX_MDIO_CFG) | B_AX_MDIO_RFLAG);

	cnt = MDIO_DLY_CNT;
	while (MAC_REG_R16(R_AX_MDIO_CFG) & B_AX_MDIO_RFLAG && cnt) {
		PLTFM_DELAY_US(MDIO_DLY_US);
		cnt--;
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]MDIO R16 0x%X timeout\n", addr);
		ret = MACPOLLTO;
		goto end;
	}

	*val = MAC_REG_R16(R_AX_MDIO_RDATA);
end:
	PLTFM_MUTEX_UNLOCK(&adapter->hw_info->mdio_lock);
	return ret;
#else
	return MACNOTSUP;
#endif
}

u32 mdio_w16_pcie(struct mac_ax_adapter *adapter, u8 addr, u16 data, u8 speed)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u16 val16;
	u32 cnt;
	u32 ret = MACSUCCESS;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))) {
		PLTFM_MSG_ERR("[ERR]MDIO is not supported\n");
		return MACNOTSUP;
	}

	if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
		return MACSUCCESS;

	PLTFM_MUTEX_LOCK(&adapter->hw_info->mdio_lock);

	MAC_REG_W16(R_AX_MDIO_WDATA, data);
	MAC_REG_W8(R_AX_MDIO_CFG, addr & B_AX_MDIO_ADDR_MSK);

	val16 = MAC_REG_R16(R_AX_MDIO_CFG);
	if (speed == MAC_AX_PCIE_PHY_GEN1 && addr < MDIO_ADDR_PG1) {
		val16 = SET_CLR_WORD(val16, MDIO_PG0_G1, B_AX_MDIO_PHY_ADDR);
	} else if (speed == MAC_AX_PCIE_PHY_GEN1) {
		val16 = SET_CLR_WORD(val16, MDIO_PG1_G1, B_AX_MDIO_PHY_ADDR);
	} else if (speed == MAC_AX_PCIE_PHY_GEN2 && addr < MDIO_ADDR_PG1) {
		val16 = SET_CLR_WORD(val16, MDIO_PG0_G2, B_AX_MDIO_PHY_ADDR);
	} else if (speed == MAC_AX_PCIE_PHY_GEN2) {
		val16 = SET_CLR_WORD(val16, MDIO_PG1_G2, B_AX_MDIO_PHY_ADDR);
	} else {
		PLTFM_MSG_ERR("[ERR]Error MDIO PHY Speed %d!\n", speed);
		ret = MACFUNCINPUT;
		goto end;
	}
	MAC_REG_W16(R_AX_MDIO_CFG, val16);

	MAC_REG_W16(R_AX_MDIO_CFG,
		    MAC_REG_R16(R_AX_MDIO_CFG) | B_AX_MDIO_WFLAG);

	cnt = MDIO_DLY_CNT;
	while (MAC_REG_R16(R_AX_MDIO_CFG) & B_AX_MDIO_WFLAG && cnt) {
		PLTFM_DELAY_US(MDIO_DLY_US);
		cnt--;
	}

	if (!cnt) {
		PLTFM_MSG_ERR("[ERR]MDIO W16 0x%X = 0x%x timeout!\n", addr, data);
		ret = MACPOLLTO;
		goto end;
	}

end:
	PLTFM_MUTEX_UNLOCK(&adapter->hw_info->mdio_lock);
	return ret;
#else
	return MACNOTSUP;
#endif
}

void update_pcie_func_u32(u32 *val, u32 bitmask,
			  enum mac_ax_pcie_func_ctrl ctrl,
			  enum mac_ax_pcie_func_ctrl def_ctrl)
{
	if ((ctrl == MAC_AX_PCIE_DEFAULT &&
	     (def_ctrl == MAC_AX_PCIE_IGNORE ||
	      def_ctrl == MAC_AX_PCIE_DEFAULT)) || ctrl == MAC_AX_PCIE_IGNORE)
		return;

	if ((ctrl == MAC_AX_PCIE_DEFAULT && def_ctrl == MAC_AX_PCIE_DISABLE) ||
	    ctrl == MAC_AX_PCIE_DISABLE)
		*val &= ~(bitmask);
	else
		*val |= bitmask;
}

void update_pcie_func_u8(u8 *val, u8 bitmask,
			 enum mac_ax_pcie_func_ctrl ctrl,
			 enum mac_ax_pcie_func_ctrl def_ctrl)
{
	if ((ctrl == MAC_AX_PCIE_DEFAULT &&
	     (def_ctrl == MAC_AX_PCIE_IGNORE ||
	      def_ctrl == MAC_AX_PCIE_DEFAULT)) || ctrl == MAC_AX_PCIE_IGNORE)
		return;

	if ((ctrl == MAC_AX_PCIE_DEFAULT && def_ctrl == MAC_AX_PCIE_DISABLE) ||
	    ctrl == MAC_AX_PCIE_DISABLE)
		*val &= ~(bitmask);
	else
		*val |= bitmask;
}

u16 calc_avail_wptr(u16 rptr, u16 wptr, u16 bndy)
{
	u16 avail_wptr = 0;

	if (rptr > wptr)
		avail_wptr = rptr - wptr - 1;
	else
		avail_wptr = rptr + (bndy - wptr) - 1;

	return avail_wptr;
}

u16 calc_avail_rptr(u16 rptr, u16 wptr, u16 bndy)
{
	u16 avail_rptr = 0;

	if (wptr >= rptr)
		avail_rptr = wptr - rptr;
	else
		avail_rptr = wptr + (bndy - rptr);

	return avail_rptr;
}

u32 cfgspc_set_pcie(struct mac_ax_adapter *adapter,
		    struct mac_ax_pcie_cfgspc_param *param)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	u32 status = MACSUCCESS;

	if (param->write == 1)
		status = p_ops->pcie_cfgspc_write(adapter, param);

	if (param->read == 1)
		status = p_ops->pcie_cfgspc_read(adapter, param);

	return status;
}

u32 ltr_set_pcie(struct mac_ax_adapter *adapter,
		 struct mac_ax_pcie_ltr_param *param)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	u32 ret = MACSUCCESS;

	if (param->write) {
		ret = p_ops->pcie_ltr_write(adapter, param);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]pcie ltr write fail %d\n", ret);
			return ret;
		}
	}

	if (param->read) {
		ret = p_ops->pcie_ltr_read(adapter, param);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]pcie ltr read fail %d\n", ret);
			return ret;
		}
	}

	return ret;
}

static u32 pcie_set_sic(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_sic_clkreq(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PCIE_EXP_CTRL, MAC_REG_R32(R_AX_PCIE_EXP_CTRL) &
		    ~B_AX_SIC_EN_FORCE_CLKREQ);

	return MACSUCCESS;
}

static u32 pcie_set_lbc(struct mac_ax_adapter *adapter,
			enum mac_ax_pcie_func_ctrl ctrl,
			enum mac_ax_lbc_tmr tmr)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_intf_info *intf_info_def;
	u32 val32;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)))
		return MACSUCCESS;

	intf_info_def = p_ops->get_pcie_info_def(adapter);
	if (!intf_info_def) {
		PLTFM_MSG_ERR("%s: NULL intf_info\n", __func__);
		return MACNPTR;
	}

	val32 = MAC_REG_R32(R_AX_LBC_WATCHDOG);
	if (ctrl == MAC_AX_PCIE_ENABLE ||
	    (ctrl == MAC_AX_PCIE_DEFAULT &&
	     intf_info_def->lbc_en == MAC_AX_PCIE_ENABLE)) {
		val32 = SET_CLR_WORD(val32, tmr == MAC_AX_LBC_TMR_DEF ?
				     intf_info_def->lbc_tmr : tmr,
				     B_AX_LBC_TIMER);
		val32 |= B_AX_LBC_FLAG | B_AX_LBC_EN;
	} else {
		val32 &= ~B_AX_LBC_EN;
	}
	MAC_REG_W32(R_AX_LBC_WATCHDOG, val32);
#endif

	return MACSUCCESS;
}

static u32 pcie_set_dbg(struct mac_ax_adapter *adapter)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 val32;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)))
		return MACSUCCESS;

	val32 = MAC_REG_R32(R_AX_PCIE_DBG_CTRL) |
		B_AX_ASFF_FULL_NO_STK | B_AX_EN_STUCK_DBG;
	MAC_REG_W32(R_AX_PCIE_DBG_CTRL, val32);
	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A)) {
#if MAC_AX_8852A_SUPPORT
		val32 = MAC_REG_R32(R_AX_PCIE_EXP_CTRL) |
			B_AX_EN_CHKDSC_NO_RX_STUCK;
		MAC_REG_W32(R_AX_PCIE_EXP_CTRL, val32);
#endif
	}
#endif

	return MACSUCCESS;
}

static u32 pcie_set_keep_reg(struct mac_ax_adapter *adapter)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)))
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PCIE_INIT_CFG1, MAC_REG_R32(R_AX_PCIE_INIT_CFG1) |
					 B_AX_PCIE_TXRST_KEEP_REG |
					 B_AX_PCIE_RXRST_KEEP_REG);
#endif

	return MACSUCCESS;
}

static u32 pcie_set_io_rcy(struct mac_ax_adapter *adapter,
			   enum mac_ax_pcie_func_ctrl ctrl,
			   enum mac_ax_io_rcy_tmr tmr)
{
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_intf_info *intf_info_def;
	u32 val32;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)))
		return MACSUCCESS;

	intf_info_def = p_ops->get_pcie_info_def(adapter);
	if (!intf_info_def) {
		PLTFM_MSG_ERR("%s: NULL intf_info\n", __func__);
		return MACNPTR;
	}

	val32 = 0;
	if (ctrl == MAC_AX_PCIE_ENABLE ||
	    (ctrl == MAC_AX_PCIE_DEFAULT &&
	     intf_info_def->io_rcy_en == MAC_AX_PCIE_ENABLE)) {
		val32 = SET_CLR_WORD(val32,
				     tmr == MAC_AX_IO_RCY_ANA_TMR_DEF ?
				     intf_info_def->io_rcy_tmr : tmr,
				     B_AX_PCIE_WDT_TIMER_M1);
		MAC_REG_W32(R_AX_PCIE_WDT_TIMER_M1, val32);
		MAC_REG_W32(R_AX_PCIE_WDT_TIMER_M2, val32);
		MAC_REG_W32(R_AX_PCIE_WDT_TIMER_E0, val32);

		val32 = MAC_REG_R32(R_AX_PCIE_IO_RCY_M1);
		val32 |= B_AX_PCIE_IO_RCY_WDT_MODE_M1;
		MAC_REG_W32(R_AX_PCIE_IO_RCY_M1, val32);

		val32 = MAC_REG_R32(R_AX_PCIE_IO_RCY_M2);
		val32 |= B_AX_PCIE_IO_RCY_WDT_MODE_M2;
		MAC_REG_W32(R_AX_PCIE_IO_RCY_M2, val32);

		val32 = MAC_REG_R32(R_AX_PCIE_IO_RCY_E0);
		val32 |= B_AX_PCIE_IO_RCY_WDT_MODE_E0;
		MAC_REG_W32(R_AX_PCIE_IO_RCY_E0, val32);
	} else {
		val32 = MAC_REG_R32(R_AX_PCIE_IO_RCY_M1);
		val32 &= ~B_AX_PCIE_IO_RCY_WDT_MODE_M1;
		MAC_REG_W32(R_AX_PCIE_IO_RCY_M1, val32);

		val32 = MAC_REG_R32(R_AX_PCIE_IO_RCY_M2);
		val32 &= ~B_AX_PCIE_IO_RCY_WDT_MODE_M2;
		MAC_REG_W32(R_AX_PCIE_IO_RCY_M2, val32);

		val32 = MAC_REG_R32(R_AX_PCIE_IO_RCY_E0);
		val32 &= ~B_AX_PCIE_IO_RCY_WDT_MODE_E0;
		MAC_REG_W32(R_AX_PCIE_IO_RCY_E0, val32);
	}

	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
#if MAC_AX_8852C_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
		val32 = MAC_REG_R32(R_AX_PCIE_IO_RCY_S1);
		val32 &= ~B_AX_PCIE_IO_RCY_WDT_MODE_S1;
		MAC_REG_W32(R_AX_PCIE_IO_RCY_S1, val32);
#endif
	}
#endif

	return MACSUCCESS;
}

static u32 trx_init_bd(struct mac_ax_adapter *adapter,
		       struct mac_ax_intf_info *intf_info)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct tx_base_desc *txbd = (struct tx_base_desc *)intf_info->txbd_buf;
	struct rx_base_desc *rxbd = (struct rx_base_desc *)intf_info->rxbd_buf;
	enum mac_ax_rxbd_mode *rxbd_mode = (&intf_info->rxbd_mode);
	struct txbd_ram *bdram_tbl;
	struct mac_ax_intf_info *intf_info_def;
	enum pcie_bd_ctrl_type type;
	u32 reg, val32, bd_num;
	u8 ch, bdram_idx;
	u32 ret = MACSUCCESS;

	adapter->pcie_info.txbd_bndy =
		txbd[MAC_AX_DMA_ACH0].buf_len / BD_TRUNC_SIZE;

	bdram_tbl = p_ops->get_bdram_tbl_pcie(adapter);
	if (!bdram_tbl) {
		PLTFM_MSG_ERR("%s: NULL bdram_tbl\n", __func__);
		return MACNPTR;
	}

	intf_info_def = p_ops->get_pcie_info_def(adapter);
	if (!intf_info_def) {
		PLTFM_MSG_ERR("%s: NULL intf_info\n", __func__);
		return MACNPTR;
	}

	if (intf_info->rxbd_mode == MAC_AX_RXBD_DEF)
		rxbd_mode = (&intf_info_def->rxbd_mode);

#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) {
		reg = R_AX_TX_ADDRESS_INFO_MODE_SETTING;
		val32 = MAC_REG_R32(reg) | B_AX_HOST_ADDR_INFO_8B_SEL;
		MAC_REG_W32(reg, val32);

		reg = R_AX_PKTIN_SETTING;
		val32 = MAC_REG_R32(reg) & ~B_AX_WD_ADDR_INFO_LENGTH;
		MAC_REG_W32(reg, val32);
	}
#endif

	bdram_idx = 0;
	for (ch = MAC_AX_DMA_ACH0; ch < MAC_AX_DMA_CH_NUM; ch++) {
		if ((is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		     is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		     is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		     is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) &&
		    ((ch >= MAC_AX_DMA_ACH4 && ch <= MAC_AX_DMA_ACH7) ||
		     (ch >= MAC_AX_DMA_B1MG && ch <= MAC_AX_DMA_B1HI)))
			continue;
		if (txbd[ch].phy_addr_l % TXBD_BYTE_ALIGN) {
			PLTFM_MSG_ERR("[ERR]ch%d txbd phyaddr 0x%X not %dB align\n",
				      ch, txbd[ch].phy_addr_l, TXBD_BYTE_ALIGN);
			return MACBADDR;
		}

		type = PCIE_BD_CTRL_DESC_L;
		ret = p_ops->set_txbd_reg_pcie(adapter, ch, type, txbd[ch].phy_addr_l, 0, 0);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]set txbd%d reg type%d %d\n", ch, type, ret);
			return ret;
		}

		type = PCIE_BD_CTRL_DESC_H;
		ret = p_ops->set_txbd_reg_pcie(adapter, ch, type, txbd[ch].phy_addr_h, 0, 0);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]set txbd%d reg type%d %d\n", ch, type, ret);
			return ret;
		}

		bd_num = txbd[ch].buf_len / BD_TRUNC_SIZE;
		if (bd_num > BD_MAX_NUM) {
			PLTFM_MSG_ERR("ch%d txbd num %d\n", ch, bd_num);
			return MACFUNCINPUT;
		}

		type = PCIE_BD_CTRL_NUM;
		ret = p_ops->set_txbd_reg_pcie(adapter, ch, type, bd_num, 0, 0);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]set txbd%d reg type%d %d\n", ch, type, ret);
			return ret;
		}

		type = PCIE_BD_CTRL_BDRAM;
		ret = p_ops->set_txbd_reg_pcie(adapter, ch, type,
					  bdram_tbl[bdram_idx].sidx,
					  bdram_tbl[bdram_idx].max,
					  bdram_tbl[bdram_idx].min);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]set txbd%d reg type%d %d\n", ch, type, ret);
			return ret;
		}

		bdram_idx++;
	}

	for (ch = MAC_AX_RX_CH_RXQ; ch < MAC_AX_RX_CH_NUM; ch++) {
		if (rxbd[ch].phy_addr_l % RXBD_BYTE_ALIGN) {
			PLTFM_MSG_ERR("[ERR]ch%d rxbd phyaddr 0x%X not %dB align\n",
				      ch, rxbd[ch].phy_addr_l, RXBD_BYTE_ALIGN);
			return MACBADDR;
		}

		type = PCIE_BD_CTRL_DESC_L;
		ret = p_ops->set_rxbd_reg_pcie(adapter, ch, type, rxbd[ch].phy_addr_l, 0, 0);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]set rxbd%d reg type%d %d\n", ch, type, ret);
			return ret;
		}

		type = PCIE_BD_CTRL_DESC_H;
		ret = p_ops->set_rxbd_reg_pcie(adapter, ch, type, rxbd[ch].phy_addr_h, 0, 0);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]set rxbd%d reg type%d %d\n", ch, type, ret);
			return ret;
		}

		if (ch == MAC_AX_RX_CH_RXQ) {
			bd_num = (*rxbd_mode == MAC_AX_RXBD_PKT) ?
				 (rxbd[ch].buf_len / BD_TRUNC_SIZE) :
				 (rxbd[ch].buf_len / RXBD_SEP_TRUNC_NEW_SIZE);
			adapter->pcie_info.rxbd_bndy = (u16)bd_num;
		} else {
			bd_num = rxbd[ch].buf_len / BD_TRUNC_SIZE;
			adapter->pcie_info.rpbd_bndy = (u16)bd_num;
		}

		if (bd_num > BD_MAX_NUM) {
			PLTFM_MSG_ERR("ch%d rxbd num %d\n", ch, bd_num);
			return MACFUNCINPUT;
		}

		type = PCIE_BD_CTRL_NUM;
		ret = p_ops->set_rxbd_reg_pcie(adapter, ch, type, bd_num, 0, 0);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]set rxbd%d reg type%d %d\n", ch, type, ret);
			return ret;
		}
	}

	return MACSUCCESS;
}

static u32 ctrl_mode_op_pcie(struct mac_ax_adapter *adapter,
			     struct mac_ax_intf_info *intf_info)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	u32 ret = MACSUCCESS;

	if (intf_info->rxbd_mode == MAC_AX_RXBD_SEP &&
	    chk_patch_dis_separation(adapter)) {
		PLTFM_MSG_ERR("[ERR]RX separation mode is not supported\n");
		return MACNOTSUP;
	}

	ret = p_ops->mode_op_pcie(adapter, intf_info);

	return ret;
}

static u32 _patch_pcie_power_wake(struct mac_ax_adapter *adapter, u8 pwr_state)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_pcie_power_wake(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	if (pwr_state == PC_POWER_UP) {
		MAC_REG_W32(R_AX_HCI_OPT_CTRL, MAC_REG_R32(R_AX_HCI_OPT_CTRL)
			    | BIT_WAKE_CTRL);
	} else if (pwr_state == PC_POWER_DOWN) {
		MAC_REG_W32(R_AX_HCI_OPT_CTRL, MAC_REG_R32(R_AX_HCI_OPT_CTRL)
			    & ~BIT_WAKE_CTRL);
	} else {
		PLTFM_MSG_ERR("[ERR] patch power wake input: %d\n", pwr_state);
		return MACFUNCINPUT;
	}

	return MACSUCCESS;
}

static u32 _patch_pcie_clkreq_delay(struct mac_ax_adapter *adapter)
{
	u16 val16;
	u32 ret = MACSUCCESS;

	if (chk_patch_pcie_clkreq_delay(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	ret = mdio_r16_pcie(adapter, RAC_REG_REV2, MAC_AX_PCIE_PHY_GEN1,
			    &val16);
	if (ret != MACSUCCESS)
		return ret;

	val16 = SET_CLR_WOR2(val16, PCIE_DPHY_DLY_25US,
			     BAC_CMU_EN_DLY_SH, BAC_CMU_EN_DLY_MSK);

	ret = mdio_w16_pcie(adapter, RAC_REG_REV2, val16,
			    MAC_AX_PCIE_PHY_GEN1);
	if (ret != MACSUCCESS)
		return ret;

	return MACSUCCESS;
}

static u32 _patch_pcie_autok_x(struct mac_ax_adapter *adapter)
{
	u16 val16;
	u32 ret = MACSUCCESS;

	if (chk_patch_pcie_autok_x(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	ret = mdio_r16_pcie(adapter, RAC_REG_FLD_0, MAC_AX_PCIE_PHY_GEN1,
			    &val16);
	if (ret != MACSUCCESS)
		return ret;
	val16 = SET_CLR_WOR2(val16, PCIE_AUTOK_4, BAC_AUTOK_N_SH,
			     BAC_AUTOK_N_MSK);
	ret = mdio_w16_pcie(adapter, RAC_REG_FLD_0, val16,
			    MAC_AX_PCIE_PHY_GEN1);
	if (ret != MACSUCCESS)
		return ret;

	return MACSUCCESS;
}

static u32 set_pcie_refclk_autok(struct mac_ax_adapter *adapter,
				 struct mac_ax_intf_info *intf_info)
{
	u32 ret;
	struct mac_ax_intf_info *intf_info_def;
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)))
		return MACSUCCESS;

	intf_info_def = p_ops->get_pcie_info_def(adapter);

	if (intf_info->autok_en == MAC_AX_PCIE_DEFAULT)
		intf_info->autok_en = intf_info_def->autok_en;

	if (intf_info->autok_en != MAC_AX_PCIE_IGNORE) {
		ret = p_ops->mac_auto_refclk_cal_pcie(adapter,
						      intf_info->autok_en);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR] pcie autok fail %d\n", ret);
			return ret;
		}
	}

	return MACSUCCESS;
}

static u32 _patch_pcie_deglitch(struct mac_ax_adapter *adapter)
{
	u32 ret;
	u16 val16, bit_set;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_pcie_deglitch(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	bit_set = BIT11 | BIT10 | BIT9 | BIT8;

	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A)) {
#if MAC_AX_8852A_SUPPORT
		ret = mdio_r16_pcie(adapter, RAC_ANA24, MAC_AX_PCIE_PHY_GEN1,
				    &val16);
		if (ret != MACSUCCESS)
			return ret;
		val16 &= ~bit_set;
		ret = mdio_w16_pcie(adapter, RAC_ANA24, val16,
				    MAC_AX_PCIE_PHY_GEN1);
		if (ret != MACSUCCESS)
			return ret;

		ret = mdio_r16_pcie(adapter, RAC_ANA24, MAC_AX_PCIE_PHY_GEN2,
				    &val16);
		if (ret != MACSUCCESS)
			return ret;
		val16 &= ~bit_set;
		ret = mdio_w16_pcie(adapter, RAC_ANA24, val16,
				    MAC_AX_PCIE_PHY_GEN2);
		if (ret != MACSUCCESS)
			return ret;
#endif
	} else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
		   is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		   is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
		   is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
		if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
			return MACSUCCESS;

		val16 = MAC_REG_R16(RAC_DIRECT_OFFSET_G1 + RAC_ANA24 * 2);
		val16 &= ~bit_set;
		MAC_REG_W16(RAC_DIRECT_OFFSET_G1 + RAC_ANA24 * 2, val16);
		val16 = MAC_REG_R16(RAC_DIRECT_OFFSET_G2 + RAC_ANA24 * 2);
		val16 &= ~bit_set;
		MAC_REG_W16(RAC_DIRECT_OFFSET_G2 + RAC_ANA24 * 2, val16);
#endif
	} else {
		return MACSUCCESS;
	}

	return MACSUCCESS;
}

static u32 _patch_pcie_l2_rxen_lat(struct mac_ax_adapter *adapter)
{
	u32 ret;
	u16 val16, bit_set;

	if (chk_patch_pcie_l2_rxen_lat(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	bit_set = BIT15 | BIT14;

	ret = mdio_r16_pcie(adapter, RAC_ANA26, MAC_AX_PCIE_PHY_GEN1, &val16);
	if (ret != MACSUCCESS)
		return ret;
	val16 &= ~bit_set;
	ret = mdio_w16_pcie(adapter, RAC_ANA26, val16, MAC_AX_PCIE_PHY_GEN1);
	if (ret != MACSUCCESS)
		return ret;

	ret = mdio_r16_pcie(adapter, RAC_ANA26, MAC_AX_PCIE_PHY_GEN2, &val16);
	if (ret != MACSUCCESS)
		return ret;
	val16 &= ~bit_set;
	ret = mdio_w16_pcie(adapter, RAC_ANA26, val16, MAC_AX_PCIE_PHY_GEN2);
	if (ret != MACSUCCESS)
		return ret;

	return MACSUCCESS;
}

static u32 _patch_l12_reboot(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_l12_reboot(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PCIE_PS_CTRL,
		    MAC_REG_R32(R_AX_PCIE_PS_CTRL) & ~B_AX_L1OFF_PWR_OFF_EN);

	return MACSUCCESS;
}

static u32 _patch_aphy_pc(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_aphy_pc(adapter) == PATCH_DISABLE) {
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C)) {
#if MAC_AX_8852C_SUPPORT
			MAC_REG_W32(R_AX_SYS_PW_CTRL,
				    MAC_REG_R32(R_AX_SYS_PW_CTRL) | B_AX_PSUS_OFF_CAPC_EN);
#endif
		} else {
			MAC_REG_W32(R_AX_SYS_PW_CTRL,
				    MAC_REG_R32(R_AX_SYS_PW_CTRL) & ~B_AX_PSUS_OFF_CAPC_EN);
		}
	} else {
		MAC_REG_W32(R_AX_SYS_PW_CTRL,
			    MAC_REG_R32(R_AX_SYS_PW_CTRL) & ~B_AX_PSUS_OFF_CAPC_EN);
	}

	return MACSUCCESS;
}

static u32 _patch_pcie_hci_ldo(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 val32;

	if (chk_patch_pcie_hci_ldo(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) {
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
		val32 = MAC_REG_R32(R_AX_SYS_SDIO_CTRL);
		val32 |= B_AX_PCIE_DIS_L2_CTRL_LDO_HCI;
		val32 &= ~B_AX_PCIE_DIS_WLSUS_AFT_PDN;
		MAC_REG_W32(R_AX_SYS_SDIO_CTRL, val32);
#endif
	} else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
		   is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		   is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
		   is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
		val32 = MAC_REG_R32(R_AX_SYS_SDIO_CTRL);
		val32 &= ~B_AX_PCIE_DIS_L2_CTRL_LDO_HCI;
		MAC_REG_W32(R_AX_SYS_SDIO_CTRL, val32);
#endif
	} else {
		return MACSUCCESS;
	}

	return MACSUCCESS;
}

static u32 _patch_l2_ldo_power(struct mac_ax_adapter *adapter)
{
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	u32 ret;
	u16 bit_set;
	u8 val8;

	if (chk_patch_l2_ldo_power(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	bit_set = BIT0;

	ret = dbi_r8_pcie(adapter, CFG_RST_MSTATE, &val8);
	if (ret != MACSUCCESS)
		return ret;
	val8 |= bit_set;
	ret = dbi_w8_pcie(adapter, CFG_RST_MSTATE, val8);
	if (ret != MACSUCCESS)
		return ret;

	ret = dbi_r8_pcie(adapter, CFG_RST_MSTATE, &val8);
	if (ret != MACSUCCESS)
		return ret;
	val8 |= bit_set;
	ret = dbi_w8_pcie(adapter, CFG_RST_MSTATE, val8);
	if (ret != MACSUCCESS)
		return ret;
#endif
	return MACSUCCESS;
}

static u32 _patch_rx_prefetch(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_rx_prefetch(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PCIE_INIT_CFG1,
		    MAC_REG_R32(R_AX_PCIE_INIT_CFG1) | B_AX_DIS_RXDMA_PRE);

	return MACSUCCESS;
}

static u32 patch_pcie_sw_ltr_setparm(struct mac_ax_adapter *adapter,
				     struct mac_ax_pcie_ltr_param *param)
{
	if (!chk_patch_pcie_sw_ltr(adapter))
		return MACSUCCESS;

	param->ltr_hw_ctrl = MAC_AX_PCIE_DISABLE;

	return MACSUCCESS;
}

static u32 _patch_pcie_sw_ltr(struct mac_ax_adapter *adapter,
			      enum mac_ax_pcie_ltr_sw_ctrl ctrl)
{
	u32 ret;
	void *val;

	if (!chk_patch_pcie_sw_ltr(adapter))
		return MACSUCCESS;

	val = (void *)&ctrl;
	ret = adapter->ops->set_hw_value(adapter, MAX_AX_HW_PCIE_LTR_SW_TRIGGER, val);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie ltr sw trig %d %d\n", ctrl, ret);
		return ret;
	}

	return MACSUCCESS;
}

static u32 _patch_apb_hang(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_apb_hang(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PCIE_BG_CLR, MAC_REG_R32(R_AX_PCIE_BG_CLR) |
		    B_AX_BG_CLR_ASYNC_M3);

	MAC_REG_W32(R_AX_PCIE_BG_CLR, MAC_REG_R32(R_AX_PCIE_BG_CLR) &
		    ~B_AX_BG_CLR_ASYNC_M3);

	return MACSUCCESS;
}

static u32 _patch_pcie_vmain(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_pcie_vmain(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	MAC_REG_W32(R_AX_SYS_SDIO_CTRL, MAC_REG_R32(R_AX_SYS_SDIO_CTRL) |
		    B_AX_PCIE_FORCE_PWR_NGAT);

	return MACSUCCESS;
}

static u32 _patch_pcie_gen2_force_ib(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	if (chk_patch_pcie_gen2_force_ib(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PMC_DBG_CTRL2, MAC_REG_R32(R_AX_PMC_DBG_CTRL2) |
		    B_AX_SYSON_DIS_PMCR_AX_WRMSK);

	MAC_REG_W32(R_AX_HCI_BG_CTRL, MAC_REG_R32(R_AX_HCI_BG_CTRL) |
		    B_AX_FORCED_IB_EN);

	MAC_REG_W32(R_AX_PMC_DBG_CTRL2, MAC_REG_R32(R_AX_PMC_DBG_CTRL2) &
		    ~B_AX_SYSON_DIS_PMCR_AX_WRMSK);
#endif
	return MACSUCCESS;
}

static u32 set_pcie_l1_ent_lat(struct mac_ax_adapter *adapter)
{
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)))
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PCIE_PS_CTRL_V1, MAC_REG_R32(R_AX_PCIE_PS_CTRL_V1) &
		    ~B_AX_SEL_REQ_ENTR_L1);
#endif
	return MACSUCCESS;
}

u32 set_pcie_wd_exit_l1(struct mac_ax_adapter *adapter)
{
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)))
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PCIE_PS_CTRL_V1, MAC_REG_R32(R_AX_PCIE_PS_CTRL_V1) |
		    B_AX_DMAC0_EXIT_L1_EN);
#endif
	return MACSUCCESS;
}

static u32 _patch_pcie_err_ind(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_pcie_err_ind(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	MAC_REG_W32(R_AX_PCIE_IO_RCY_S1, MAC_REG_R32(R_AX_PCIE_IO_RCY_S1) |
		    B_AX_PCIE_IO_RCY_WDT_WP_S1 | B_AX_PCIE_IO_RCY_WDT_RP_S1);

	return MACSUCCESS;
}

static u32 _patch_pclk_nrdy(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (chk_patch_pclk_nrdy(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

	MAC_REG_W32(R_AX_SYS_SDIO_CTRL, MAC_REG_R32(R_AX_SYS_SDIO_CTRL) |
		    B_AX_PCIE_WAIT_TIME);

	return MACSUCCESS;
}

static u32 _patch_pcie_power_wake_efuse(struct mac_ax_adapter *adapter)
{
	u32 ret = MACSUCCESS;
#if MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	struct mac_ax_ops *mac_ops = adapter_to_mac_ops(adapter);
	u8 i = 0, *map = 0, *mask = 0;
	u32 efuse_size = 0;
	u32 size;
#endif
	if (chk_patch_pcie_power_wake_efuse(adapter) == PATCH_DISABLE)
		return MACSUCCESS;

#if MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) {
		ret = mac_ops->get_hw_value(adapter, MAC_AX_HW_GET_LOGICAL_EFUSE_SIZE,
					    &efuse_size);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]get logical efuse fail %d\n", ret);
			goto end;
		}

		ret = mac_ops->get_efuse_avl_size(adapter, &size);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]get avail efuse fail %d\n", ret);
			goto end;
		}
		if (size < EFUSE_AVAIL_ENOUGH) {
			PLTFM_MSG_WARN("[WARN]avail efuse not enough %d\n", size);
			goto end;
		}

		mask = (u8 *)PLTFM_MALLOC(efuse_size);
		if (!mask) {
			PLTFM_MSG_ERR("[ERR]malloc mask when patch pcie\n");
			ret = MACBUFALLOC;
			goto end;
		}
		ret = mac_ops->dump_log_efuse(adapter, MAC_AX_EFUSE_PARSER_MASK,
					      MAC_AX_EFUSE_R_DRV, mask, 0);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]dump efuse mask fail %d\n", ret);
			goto end;
		}
		if (!(*(mask + FIX_WAKE_EFUSE_OFFSET) != EFUSE_NOT_BURN_MASK)) {
			PLTFM_MSG_WARN("[WARN]efuse logical 0x74 not burn: %d\n",
				       *(mask + FIX_WAKE_EFUSE_OFFSET));
			goto end;
		}

		map = (u8 *)PLTFM_MALLOC(efuse_size);
		if (!map) {
			PLTFM_MSG_ERR("[ERR]malloc map when patch pcie\n");
			ret = MACBUFALLOC;
			goto end;
		}
		ret = mac_ops->dump_log_efuse(adapter, MAC_AX_EFUSE_PARSER_MAP,
					      MAC_AX_EFUSE_R_DRV, map, 0);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]dump efuse map fail %d\n", ret);
			goto end;
		}
		if (*(map + FIX_WAKE_EFUSE_OFFSET) & FIX_WAKE_EFUSE_BIT) {
			*(map + FIX_WAKE_EFUSE_OFFSET) = *(map + FIX_WAKE_EFUSE_OFFSET) &
				~FIX_WAKE_EFUSE_BIT;
			for (i = 0; i < EFUSE_2BYTES; i++) {
				ret = mac_ops->write_log_efuse(adapter, FIX_WAKE_EFUSE_OFFSET + i,
							       *(map + FIX_WAKE_EFUSE_OFFSET + i));
				if (ret != MACSUCCESS) {
					PLTFM_MSG_ERR("[ERR]write efuse 0x74 fail %d\n", ret);
					goto end;
				}
				PLTFM_MSG_TRACE("efuse 0x%X: 0x%X\n",
						FIX_WAKE_EFUSE_OFFSET + i,
						*(map + FIX_WAKE_EFUSE_OFFSET + i));
			}
		}
	}
end:
	if (map)
		PLTFM_FREE(map, efuse_size);
	if (mask)
		PLTFM_FREE(mask, efuse_size);
#endif

	return ret;
}

u32 clr_idx_all_pcie(struct mac_ax_adapter *adapter)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_txdma_ch_map txch_map;
	struct mac_ax_rxdma_ch_map rxch_map;
	u32 ret;

	txch_map.ch0 = MAC_AX_PCIE_ENABLE;
	txch_map.ch1 = MAC_AX_PCIE_ENABLE;
	txch_map.ch2 = MAC_AX_PCIE_ENABLE;
	txch_map.ch3 = MAC_AX_PCIE_ENABLE;
	txch_map.ch4 = MAC_AX_PCIE_ENABLE;
	txch_map.ch5 = MAC_AX_PCIE_ENABLE;
	txch_map.ch6 = MAC_AX_PCIE_ENABLE;
	txch_map.ch7 = MAC_AX_PCIE_ENABLE;
	txch_map.ch8 = MAC_AX_PCIE_ENABLE;
	txch_map.ch9 = MAC_AX_PCIE_ENABLE;
	txch_map.ch10 = MAC_AX_PCIE_ENABLE;
	txch_map.ch11 = MAC_AX_PCIE_ENABLE;
	txch_map.ch12 = MAC_AX_PCIE_ENABLE;
	rxch_map.rxq = MAC_AX_PCIE_ENABLE;
	rxch_map.rpq = MAC_AX_PCIE_ENABLE;
	ret = p_ops->clr_idx_ch_pcie(adapter, &txch_map, &rxch_map);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Clear all bd index %d\n", ret);
		return ret;
	}

	PLTFM_MSG_ALWAYS("Clear all bd index done.\n");

	return MACSUCCESS;
}

u32 ctrl_txhci_pcie(struct mac_ax_adapter *adapter, enum mac_ax_func_sw en)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	enum mac_ax_pcie_func_ctrl txen;
	enum mac_ax_pcie_func_ctrl rxen;
	enum mac_ax_pcie_func_ctrl ioen;
	u32 ret;

	if (en == MAC_AX_FUNC_EN) {
		txen = MAC_AX_PCIE_ENABLE;
		rxen = MAC_AX_PCIE_IGNORE;
		ioen = MAC_AX_PCIE_IGNORE;
	} else {
		txen = MAC_AX_PCIE_DISABLE;
		rxen = MAC_AX_PCIE_IGNORE;
		ioen = MAC_AX_PCIE_IGNORE;
	}

	ret = p_ops->ctrl_trxdma_pcie(adapter, txen, rxen, ioen);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Ctrl txhci pcie %d\n", ret);
		return ret;
	}

	return ret;
}

u32 ctrl_rxhci_pcie(struct mac_ax_adapter *adapter, enum mac_ax_func_sw en)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	enum mac_ax_pcie_func_ctrl txen;
	enum mac_ax_pcie_func_ctrl rxen;
	enum mac_ax_pcie_func_ctrl ioen;
	u32 ret;

	if (en == MAC_AX_FUNC_EN) {
		txen = MAC_AX_PCIE_IGNORE;
		rxen = MAC_AX_PCIE_ENABLE;
		ioen = MAC_AX_PCIE_IGNORE;
	} else {
		txen = MAC_AX_PCIE_IGNORE;
		rxen = MAC_AX_PCIE_DISABLE;
		ioen = MAC_AX_PCIE_IGNORE;
	}

	ret = p_ops->ctrl_trxdma_pcie(adapter, txen, rxen, ioen);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Ctrl rxhci pcie %d\n", ret);
		return ret;
	}

	return ret;
}

u32 ctrl_dma_io_pcie(struct mac_ax_adapter *adapter, enum mac_ax_func_sw en)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	enum mac_ax_pcie_func_ctrl txen;
	enum mac_ax_pcie_func_ctrl rxen;
	enum mac_ax_pcie_func_ctrl ioen;
	u32 ret;

	if (en == MAC_AX_FUNC_EN) {
		txen = MAC_AX_PCIE_IGNORE;
		rxen = MAC_AX_PCIE_IGNORE;
		ioen = MAC_AX_PCIE_ENABLE;
	} else {
		txen = MAC_AX_PCIE_IGNORE;
		rxen = MAC_AX_PCIE_IGNORE;
		ioen = MAC_AX_PCIE_DISABLE;
	}

	ret = p_ops->ctrl_trxdma_pcie(adapter, txen, rxen, ioen);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Ctrl rxhci pcie %d\n", ret);
		return ret;
	}

	return ret;
}

u32 pcie_pre_init(struct mac_ax_adapter *adapter, void *param)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_info *intf_info = (struct mac_ax_intf_info *)param;
	struct mac_ax_txdma_ch_map ch_map;
	u32 ret = MACSUCCESS;

	ret = _patch_rx_prefetch(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie dis rxdma prefth %d\n", ret);
		return ret;
	}

	ret = _patch_l12_reboot(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie dis l1off pwroff %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_deglitch(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie deglitch %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_l2_rxen_lat(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie l2 rxen latency %d\n", ret);
		return ret;
	}

	ret = _patch_aphy_pc(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie aphy pwrcut %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_hci_ldo(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie hci ldo %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_clkreq_delay(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie dphy delay %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_autok_x(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie autok_x %d\n", ret);
		return ret;
	}

	ret = set_pcie_refclk_autok(adapter, intf_info);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]set pcie refclk autok %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_power_wake(adapter, PC_POWER_UP);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie power wake %d\n", ret);
		return ret;
	}

	ret = _patch_apb_hang(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie autoload hang %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_vmain(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie l12 vmain %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_gen2_force_ib(adapter);
	if (ret) {
		PLTFM_MSG_ERR("[ERR]patch pcie gen2 force ib %d\n", ret);
		return ret;
	}

	ret = set_pcie_l1_ent_lat(adapter);
	if (ret) {
		PLTFM_MSG_ERR("[ERR]patch pcie l1 entrance latency %d\n", ret);
		return ret;
	}

	ret = set_pcie_wd_exit_l1(adapter);
	if (ret) {
		PLTFM_MSG_ERR("[ERR]patch pcie wd l1 exit %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_err_ind(adapter);
	if (ret) {
		PLTFM_MSG_ERR("[ERR]patch pcie error ind %d\n", ret);
		return ret;
	}

	ret = _patch_pclk_nrdy(adapter);
	if (ret) {
		PLTFM_MSG_ERR("[ERR]patch pclk nrdy %d\n", ret);
		return ret;
	}

	ret = pcie_set_sic(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie sic %d\n", ret);
		return ret;
	}

	ret = pcie_set_lbc(adapter, intf_info->lbc_en, intf_info->lbc_tmr);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie set lbc %d\n", ret);
		return ret;
	}

	ret = pcie_set_io_rcy(adapter, intf_info->io_rcy_en,
			      intf_info->io_rcy_tmr);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie set io rcy %d\n", ret);
		return ret;
	}

	ret = pcie_set_dbg(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie set dbg %d\n", ret);
		return ret;
	}

	ret = pcie_set_keep_reg(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie set keep reg %d\n", ret);
		return ret;
	}

	if (intf_info->skip_all)
		return ret;

	if (!intf_info->txbd_buf || !intf_info->rxbd_buf ||
	    !intf_info->txch_map) {
		PLTFM_MSG_ERR("[ERR]empty txbd_buf/rxbd_buf/txch_map\n");
		return MACNPTR;
	}

	ret = p_ops->ctrl_wpdma_pcie(adapter, MAC_AX_PCIE_DISABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]disable wpdma %d\n", ret);
		return ret;
	}

	ret = p_ops->ctrl_trxdma_pcie(adapter, MAC_AX_PCIE_DISABLE,
				      MAC_AX_PCIE_DISABLE, MAC_AX_PCIE_DISABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Disable pcie dma all %d\n", ret);
		return ret;
	}

	ret = ops->clr_idx_all(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]clear pcie idx all %d\n", ret);
		return ret;
	}

	ret = p_ops->poll_dma_all_idle_pcie(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]poll pcie dma all idle %d\n", ret);
		return ret;
	}

	ret = ctrl_mode_op_pcie(adapter, intf_info);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie mode op %d\n", ret);
		return ret;
	}

	ret = trx_init_bd(adapter, intf_info);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie trx init bd %d\n", ret);
		return ret;
	}

	ret = p_ops->rst_bdram_pcie(adapter, 0);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie rst bdram %d\n", ret);
		return ret;
	}

	ch_map.ch0 = MAC_AX_PCIE_DISABLE;
	ch_map.ch1 = MAC_AX_PCIE_DISABLE;
	ch_map.ch2 = MAC_AX_PCIE_DISABLE;
	ch_map.ch3 = MAC_AX_PCIE_DISABLE;
	ch_map.ch4 = MAC_AX_PCIE_DISABLE;
	ch_map.ch5 = MAC_AX_PCIE_DISABLE;
	ch_map.ch6 = MAC_AX_PCIE_DISABLE;
	ch_map.ch7 = MAC_AX_PCIE_DISABLE;
	ch_map.ch8 = MAC_AX_PCIE_DISABLE;
	ch_map.ch9 = MAC_AX_PCIE_DISABLE;
	ch_map.ch10 = MAC_AX_PCIE_DISABLE;
	ch_map.ch11 = MAC_AX_PCIE_DISABLE;
	ch_map.ch12 = MAC_AX_PCIE_ENABLE;
	ret = ops->ctrl_txdma_ch(adapter, &ch_map);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]enable pcie h2c ch only %d\n", ret);
		return ret;
	}

	ret = p_ops->ctrl_trxdma_pcie(adapter, MAC_AX_PCIE_ENABLE,
				      MAC_AX_PCIE_ENABLE, MAC_AX_PCIE_ENABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Enable pcie dma all %d\n", ret);
		return ret;
	}

	return MACSUCCESS;
}

u32 pcie_init(struct mac_ax_adapter *adapter, void *param)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_intf_info *intf_info = (struct mac_ax_intf_info *)param;
	struct mac_ax_pcie_ltr_param ltr_param = {
		1,
		0,
		MAC_AX_PCIE_DEFAULT,
		MAC_AX_PCIE_DEFAULT,
		MAC_AX_PCIE_LTR_SPC_DEF,
		MAC_AX_PCIE_LTR_IDLE_TIMER_DEF,
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0}
	};
	u32 ret;

	if (intf_info->skip_all)
		return MACSUCCESS;

	ret = _patch_pcie_power_wake_efuse(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie wake efuse %d\n", ret);
		return ret;
	}

	ret = patch_pcie_sw_ltr_setparm(adapter, &ltr_param);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie sw ltr set param %d\n", ret);
		return ret;
	}

	ret = ops->ltr_set_pcie(adapter, &ltr_param);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie ltr set fail %d\n", ret);
		return ret;
	}

	ret = _patch_pcie_sw_ltr(adapter, MAC_AX_PCIE_LTR_SW_ACT);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie sw ltr act %d\n", ret);
		return ret;
	}

	if (!intf_info->txch_map) {
		PLTFM_MSG_ERR("[ERR] pcie init no txch map\n");
		return MACNPTR;
	}

	ret = p_ops->ctrl_trxdma_pcie(adapter, MAC_AX_PCIE_IGNORE,
				      MAC_AX_PCIE_IGNORE, MAC_AX_PCIE_ENABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]enable pcie io %d\n", ret);
		return ret;
	}

	ret = p_ops->ctrl_wpdma_pcie(adapter, MAC_AX_PCIE_ENABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]enable wpdma %d\n", ret);
		return ret;
	}

	ret = ops->ctrl_txdma_ch(adapter, intf_info->txch_map);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]enable pcie txdma %d\n", ret);
		return ret;
	}

	return MACSUCCESS;
}

u32 pcie_deinit(struct mac_ax_adapter *adapter, void *param)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_intf_deinit_info *intf_deinit_info = (struct mac_ax_intf_deinit_info *)param;
	u32 val32, ret = MACSUCCESS;
	struct mac_ax_pcie_ltr_param ltr_param = {
		1,
		0,
		MAC_AX_PCIE_DISABLE,
		MAC_AX_PCIE_DISABLE,
		MAC_AX_PCIE_LTR_SPC_DEF,
		MAC_AX_PCIE_LTR_IDLE_TIMER_DEF,
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0}
	};

	ret = _patch_pcie_power_wake(adapter, PC_POWER_DOWN);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]patch pcie power wake %d\n", ret);
		return ret;
	}

	val32 = MAC_REG_R32(R_AX_IC_PWR_STATE);
	val32 = GET_FIELD(val32, B_AX_WLMAC_PWR_STE);
	if (val32 == MAC_AX_MAC_OFF) {
		PLTFM_MSG_WARN("PCIe deinit when MAC off\n");
		return MACSUCCESS;
	}

	if (!intf_deinit_info->fast_deinit_flag) {
		ret = ops->ltr_set_pcie(adapter, &ltr_param);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]pcie ltr set fail %d\n", ret);
			return ret;
		}
	}

	ret = p_ops->ctrl_trxdma_pcie(adapter, MAC_AX_PCIE_DISABLE,
				      MAC_AX_PCIE_DISABLE, MAC_AX_PCIE_DISABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Disable pcie dma all %d\n", ret);
		return ret;
	}

	ret = ops->clr_idx_all(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]clear pcie idx all %d\n", ret);
		return ret;
	}

	return ret;
}

u32 lv1rst_stop_dma_pcie(struct mac_ax_adapter *adapter, u8 val)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	enum mac_ax_pcie_func_ctrl txen = MAC_AX_PCIE_IGNORE;
	enum mac_ax_pcie_func_ctrl rxen = MAC_AX_PCIE_IGNORE;
	u32 ret;

	ret = p_ops->ctrl_trxdma_pcie(adapter, MAC_AX_PCIE_DISABLE,
				      MAC_AX_PCIE_DISABLE, MAC_AX_PCIE_DISABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Disable pcie dma all %d\n", ret);
		return ret;
	}

	ret = p_ops->poll_io_idle_pcie(adapter);
	if (ret != MACSUCCESS) {
		txen = MAC_AX_PCIE_DISABLE;
		rxen = MAC_AX_PCIE_DISABLE;

		ret = p_ops->ctrl_hci_dma_en_pcie(adapter, txen, rxen);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]hci dma pcie tx%d rx%d %d\n", txen, rxen, ret);
			return ret;
		}

		if (txen != MAC_AX_PCIE_IGNORE)
			txen = MAC_AX_PCIE_ENABLE;
		if (rxen != MAC_AX_PCIE_IGNORE)
			rxen = MAC_AX_PCIE_ENABLE;
		ret = p_ops->ctrl_hci_dma_en_pcie(adapter, txen, rxen);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]hci dma pcie tx%d rx%d %d\n", txen, rxen, ret);
			return ret;
		}

		ret = p_ops->poll_io_idle_pcie(adapter);
	}

	return ret;
}

u32 lv1rst_start_dma_pcie(struct mac_ax_adapter *adapter, u8 val)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	u32 ret;

	ret = p_ops->ctrl_hci_dma_en_pcie(adapter, MAC_AX_PCIE_DISABLE,
					  MAC_AX_PCIE_DISABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]hci dma disable pcie %d\n", ret);
		return ret;
	}

	ret = p_ops->ctrl_hci_dma_en_pcie(adapter, MAC_AX_PCIE_ENABLE,
					  MAC_AX_PCIE_ENABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]hci dma enable pcie %d\n", ret);
		return ret;
	}

	ret = ops->clr_idx_all(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]clear idx all %d\n", ret);
		return ret;
	}

	ret = p_ops->rst_bdram_pcie(adapter, 0);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]rst bdram %d\n", ret);
		return ret;
	}

	ret = p_ops->ctrl_trxdma_pcie(adapter, MAC_AX_PCIE_ENABLE,
				      MAC_AX_PCIE_ENABLE, MAC_AX_PCIE_ENABLE);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Enable pcie dma all %d\n", ret);
		return ret;
	}

	return ret;
}

u32 pcie_pwr_switch(void *vadapter, u8 pre_switch, u8 on)
{
	struct mac_ax_adapter *adapter = (struct mac_ax_adapter *)vadapter;

	if (pre_switch == PWR_PRE_SWITCH)
		adapter->mac_pwr_info.pwr_seq_proc = 1;
	else if (pre_switch == PWR_POST_SWITCH)
		adapter->mac_pwr_info.pwr_seq_proc = 0;

	return MACSUCCESS;
}

u32 set_pcie_wowlan(struct mac_ax_adapter *adapter, enum mac_ax_wow_ctrl w_c)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 ret;
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
#ifdef RTW_WKARD_GET_PROCESSOR_ID
	u8 i;
	u32 val32;
#endif
#endif
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	struct mac_ax_pcie_ltr_param ltr_param = {
		1,
		0,
		MAC_AX_PCIE_DEFAULT,
		MAC_AX_PCIE_DEFAULT,
		MAC_AX_PCIE_LTR_SPC_DEF,
		MAC_AX_PCIE_LTR_IDLE_TIMER_DEF,
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0},
		{MAC_AX_PCIE_DEFAULT, 0},
		MAC_AX_PCIE_IGNORE,
		MAC_AX_PCIE_IGNORE,
		MAC_AX_PCIE_IGNORE,
		PCIE_LTR_IDX_INVALID,
		PCIE_LTR_IDX_INVALID,
		PCIE_LTR_IDX_INVALID
	};
#endif
	if (w_c == MAC_AX_WOW_ENTER) {
		MAC_REG_W32(R_AX_RSV_CTRL, MAC_REG_R32(R_AX_RSV_CTRL) |
			    B_AX_WLOCK_1C_BIT6);
		MAC_REG_W32(R_AX_RSV_CTRL, MAC_REG_R32(R_AX_RSV_CTRL) |
			    B_AX_R_DIS_PRST);
		MAC_REG_W32(R_AX_RSV_CTRL, MAC_REG_R32(R_AX_RSV_CTRL) &
			    ~B_AX_WLOCK_1C_BIT6);
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) {
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
			if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A)) {
				MAC_REG_W32(R_AX_SYS_SDIO_CTRL,
					    MAC_REG_R32(R_AX_SYS_SDIO_CTRL) &
					    ~B_AX_PCIE_DIS_L2_CTRL_LDO_HCI);
#ifdef RTW_WKARD_GET_PROCESSOR_ID
				val32 = adapter->hw_info->adpt_info.cust_proc_id.customer_id;
				val32 &= C_WOW_LDO_ID_MSK;
				for (i = 0; i < C_WOW_LDO_ID_LIST_NUM; i++)
					if (val32 == c_wow_ldo_id[i]) {
						MAC_REG_W32(R_AX_SYS_SDIO_CTRL,
							    MAC_REG_R32(R_AX_SYS_SDIO_CTRL) |
							    B_AX_PCIE_DIS_L2_CTRL_LDO_HCI);
						break;
					}
#endif
			} else {
				MAC_REG_W32(R_AX_SYS_SDIO_CTRL,
					    MAC_REG_R32(R_AX_SYS_SDIO_CTRL) &
					    ~B_AX_PCIE_DIS_L2_CTRL_LDO_HCI);
			}

			MAC_REG_W32(R_AX_PCIE_INIT_CFG1,
				    MAC_REG_R32(R_AX_PCIE_INIT_CFG1) |
				    B_AX_PCIE_PERST_KEEP_REG |
				    B_AX_PCIE_TRAIN_KEEP_REG);
#endif
		} else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
			   is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
			   is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
			   is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
			MAC_REG_W32(R_AX_PCIE_PS_CTRL_V1,
				    MAC_REG_R32(R_AX_PCIE_PS_CTRL_V1) &
				    ~B_AX_CMAC_EXIT_L1_EN &
				    ~B_AX_DMAC0_EXIT_L1_EN);
			MAC_REG_W32(R_AX_PCIE_FRZ_CLK, MAC_REG_R32(R_AX_PCIE_FRZ_CLK) |
				    B_AX_PCIE_FRZ_REG_RST);
#endif
		} else {
			PLTFM_MSG_ERR("[ERR] Invalid wowlan chip id.\n");
			return MACCHIPID;
		}
	} else if (w_c == MAC_AX_WOW_LEAVE) {
		MAC_REG_W32(R_AX_RSV_CTRL, MAC_REG_R32(R_AX_RSV_CTRL) |
			    B_AX_WLOCK_1C_BIT6);
		MAC_REG_W32(R_AX_RSV_CTRL, MAC_REG_R32(R_AX_RSV_CTRL) &
			    ~B_AX_R_DIS_PRST);
		MAC_REG_W32(R_AX_RSV_CTRL, MAC_REG_R32(R_AX_RSV_CTRL) &
			    ~B_AX_WLOCK_1C_BIT6);
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) {
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
			MAC_REG_W32(R_AX_SYS_SDIO_CTRL,
				    MAC_REG_R32(R_AX_SYS_SDIO_CTRL) |
				    B_AX_PCIE_DIS_L2_CTRL_LDO_HCI);

			MAC_REG_W32(R_AX_PCIE_INIT_CFG1,
				    MAC_REG_R32(R_AX_PCIE_INIT_CFG1) &
				    ~(B_AX_PCIE_PERST_KEEP_REG |
				      B_AX_PCIE_TRAIN_KEEP_REG));
#endif
		} else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
			   is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
			   is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
			   is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
			MAC_REG_W32(R_AX_PCIE_PS_CTRL_V1,
				    (MAC_REG_R32(R_AX_PCIE_PS_CTRL_V1) |
				     B_AX_CMAC_EXIT_L1_EN |
				     B_AX_DMAC0_EXIT_L1_EN) &
				    ~B_AX_SEL_REQ_ENTR_L1);
			MAC_REG_W32(R_AX_PCIE_FRZ_CLK, MAC_REG_R32(R_AX_PCIE_FRZ_CLK) &
				    ~B_AX_PCIE_FRZ_REG_RST);
			ret = ops->ltr_set_pcie(adapter, &ltr_param);
			if (ret != MACSUCCESS) {
				PLTFM_MSG_ERR("[ERR]pcie ltr set fail %d\n", ret);
				return ret;
			}
#endif
		} else {
			PLTFM_MSG_ERR("[ERR] Invalid wowlan chip id.\n");
			return MACCHIPID;
		}

		ret = _patch_l2_ldo_power(adapter);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]patch pcie l2 hci ldo %d\n", ret);
			return ret;
		}
	} else {
		PLTFM_MSG_ERR("[ERR] Invalid WoWLAN input.\n");
		return MACFUNCINPUT;
	}

	return MACSUCCESS;
}

u32 set_pcie_l2_leave(struct mac_ax_adapter *adapter, u8 set)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (set) {
		/* fix WoWLAN Power Consumption */
		MAC_REG_W32(R_AX_SYS_SDIO_CTRL,
			    MAC_REG_R32(R_AX_SYS_SDIO_CTRL) &
			    ~B_AX_PCIE_CALIB_EN);
	}

	return MACSUCCESS;
}

u32 pcie_get_txagg_num(struct mac_ax_adapter *adapter, u8 band)
{
	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A))
		return PCIE_8852A_AGG_NUM;
	else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852B))
		return PCIE_8852B_AGG_NUM;
	else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C))
		return PCIE_8852C_AGG_NUM;
	else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB))
		return PCIE_8192XB_AGG_NUM;
	else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8851B))
		return PCIE_8851B_AGG_NUM;
	else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8851E))
		return PCIE_8851E_AGG_NUM;
	else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852D))
		return PCIE_8852D_AGG_NUM;
	else if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))
		return PCIE_8852BT_AGG_NUM;
	else
		return MACCHIPID;
}

u32 pcie_get_rx_state(struct mac_ax_adapter *adapter, u32 *val)
{
	return MACNOTSUP;
}

u32 trigger_txdma_pcie(struct mac_ax_adapter *adapter,
		       struct tx_base_desc *txbd_ring, u8 ch_idx)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	u8 tx_dma_ch;
	u32 ret;

	tx_dma_ch = MAC_AX_DMA_ACH0 + ch_idx;
	ret = p_ops->set_txbd_reg_pcie(adapter, tx_dma_ch, PCIE_BD_CTRL_IDX,
				       (u32)txbd_ring[ch_idx].host_idx, 0, 0);
	if (ret != MACSUCCESS)
		return ret;
	PLTFM_MSG_TRACE("%s => dma_ch %d, host_idx %d.\n", __func__, ch_idx,
			txbd_ring[ch_idx].host_idx);

	return MACSUCCESS;
}

u32 notify_rxdone_pcie(struct mac_ax_adapter *adapter,
		       struct rx_base_desc *rxbd, u8 ch)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	u32 ret;
	u8 rx_dma_ch;

	rx_dma_ch = MAC_AX_RX_CH_RXQ + ch;
	ret = p_ops->set_rxbd_reg_pcie(adapter, rx_dma_ch, PCIE_BD_CTRL_IDX,
				       rxbd->host_idx, 0, 0);
	if (ret != MACSUCCESS)
		return ret;

	return ret;
}

u32 dbcc_hci_ctrl_pcie(struct mac_ax_adapter *adapter,
		       struct mac_ax_dbcc_hci_ctrl *info)
{
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_dbcc_pcie_ctrl *ctrl;
	struct mac_ax_txdma_ch_map pause_txmap;
	struct mac_ax_rxdma_ch_map clr_rxch_map;
	enum mac_ax_band band;
	u32 ret;
	u16 host_idx, hw_idx, aval_txbd;
	u8 ch, pause;

	if (!info)
		return MACNPTR;

	band = info->band;
	pause = info->pause;
	ctrl = &info->u.pcie_ctrl;

	if (pause) {
		pause_txmap.ch0 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch1 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch2 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch3 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch4 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch5 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch6 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch7 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch8 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch9 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch10 = MAC_AX_PCIE_DISABLE;
		pause_txmap.ch11 = MAC_AX_PCIE_DISABLE;
	} else {
		pause_txmap.ch0 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch1 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch2 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch3 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch4 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch5 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch6 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch7 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch8 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch9 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch10 = MAC_AX_PCIE_ENABLE;
		pause_txmap.ch11 = MAC_AX_PCIE_ENABLE;
	}
	pause_txmap.ch12 = MAC_AX_PCIE_IGNORE;

	ret = ops->ctrl_txdma_ch(adapter, &pause_txmap);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("dbcc%d ctrl%d txdma ch pcie %d\n", band, pause, ret);
		return ret;
	}

	if (!pause)
		return MACSUCCESS;

	pause_txmap.ch0 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch1 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch2 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch3 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch4 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch5 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch6 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch7 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch8 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch9 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch10 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch11 = MAC_AX_PCIE_ENABLE;
	pause_txmap.ch12 = MAC_AX_PCIE_IGNORE;
	ret = ops->poll_txdma_ch_idle(adapter, &pause_txmap);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("dbcc%d poll txdma ch pcie %d\n", band, ret);
		return ret;
	}

	for (ch = MAC_AX_DMA_ACH0; ch < MAC_AX_DMA_CH_NUM; ch++) {
		if ((is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		     is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		     is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		     is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) &&
		    ((ch >= MAC_AX_DMA_ACH4 && ch <= MAC_AX_DMA_ACH7) ||
		     (ch >= MAC_AX_DMA_B1MG && ch <= MAC_AX_DMA_B1HI))) {
			ctrl->out_host_idx_l[ch] = BD_IDX_INVALID;
			ctrl->out_hw_idx_l[ch] = BD_IDX_INVALID;
			continue;
		}

		ret = ops->get_avail_txbd(adapter, ch, &host_idx, &hw_idx, &aval_txbd);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("get ch%d idx pcie %d\n", ch, ret);
			return ret;
		}

		ctrl->out_host_idx_l[ch] = host_idx;
		ctrl->out_hw_idx_l[ch] = hw_idx;
	}

	clr_rxch_map.rxq = MAC_AX_PCIE_IGNORE;
	clr_rxch_map.rpq = MAC_AX_PCIE_IGNORE;
	ret = p_ops->clr_idx_ch_pcie(adapter, &ctrl->clr_txch_map, &clr_rxch_map);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("dbcc%d clear ch idx pcie %d\n", band, ret);
		return ret;
	}

	return MACSUCCESS;
}

u32 pcie_autok_counter_avg(struct mac_ax_adapter *adapter)
{
	u8 bdr_ori, val8, l1_flag = 0;
	u16 tar16, hw_tar16, tmp16;
	u32 ret = MACSUCCESS;
	enum mac_ax_pcie_phy phy_rate = MAC_AX_PCIE_PHY_GEN1;

	if (adapter->env == DUT_ENV_FPGA || adapter->env == DUT_ENV_PXP)
		return MACSUCCESS;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)))
		return MACSUCCESS;

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

	ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &hw_tar16);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]mdio_r16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
		return ret;
	}
	PLTFM_MSG_TRACE("PCIe PHY %X: %X\n", RAC_CTRL_PPR_V1, hw_tar16);

	if (!(hw_tar16 & BAC_AUTOK_EN)) {
		PLTFM_MSG_ERR("[ERR]PCIe autok is not enabled\n: %X", hw_tar16);
		return MACPROCERR;
	}

	ret = mdio_r16_pcie(adapter, RAC_SET_PPR_V1, phy_rate, &tar16);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]mdio_r16_pcie 0x%X\n", RAC_SET_PPR_V1);
		return ret;
	}
	PLTFM_MSG_TRACE("PCIe PHY %X: %X\n", RAC_SET_PPR_V1, tar16);

	hw_tar16 = GET_FIELD(hw_tar16, BAC_AUTOK_HW_TAR);
	tar16 = GET_FIELD(tar16, BAC_AUTOK_TAR);

	if (tar16 > hw_tar16)
		tmp16 = tar16 - hw_tar16;
	else
		tmp16 = hw_tar16 - tar16;

	if (!(tmp16 < PCIE_AUTOK_MGN_2048)) {
		PLTFM_MSG_WARN("autok target is different from origin\n");
		return MACSUCCESS;
	}

	adapter->pcie_info.autok_total += hw_tar16;
	adapter->pcie_info.autok_2s_cnt++;

	if (adapter->pcie_info.autok_2s_cnt >= PCIE_AUTOK_UD_CNT) {
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

		ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &hw_tar16);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]mdio_r16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
			goto end;
		}

		hw_tar16 &= ~BAC_AUTOK_EN;

		ret = mdio_w16_pcie(adapter, RAC_CTRL_PPR_V1, hw_tar16, phy_rate);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]mdio_w16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
			goto end;
		}

		ret = mdio_r16_pcie(adapter, RAC_SET_PPR_V1, phy_rate, &tar16);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]mdio_r16_pcie 0x%X\n", RAC_SET_PPR_V1);
			goto end;
		}

		tmp16 = adapter->pcie_info.autok_total / adapter->pcie_info.autok_2s_cnt;
		PLTFM_MSG_TRACE("Autok 30 times avg tar: %X\n", tmp16);

		tar16 = SET_CLR_WOR2(tar16, tmp16, BAC_AUTOK_TAR_SH,
				     BAC_AUTOK_TAR_MSK);

		ret = mdio_w16_pcie(adapter, RAC_SET_PPR_V1, tar16, phy_rate);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]mdio_w16_pcie 0x%X\n", RAC_SET_PPR_V1);
			goto end;
		}

		ret = mdio_r16_pcie(adapter, RAC_CTRL_PPR_V1, phy_rate, &hw_tar16);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]mdio_r16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
			goto end;
		}

		hw_tar16 |= BAC_AUTOK_EN;

		ret = mdio_w16_pcie(adapter, RAC_CTRL_PPR_V1, hw_tar16, phy_rate);
		if (ret != MACSUCCESS) {
			PLTFM_MSG_ERR("[ERR]mdio_w16_pcie 0x%X\n", RAC_CTRL_PPR_V1);
			goto end;
		}

end:
		if (l1_flag == 1) {
			ret = dbi_w8_pcie(adapter, PCIE_L1_CTRL, bdr_ori);
			if (ret != MACSUCCESS) {
				PLTFM_MSG_ERR("[ERR]dbi_w8_pcie 0x%X\n", PCIE_L1_CTRL);
				return ret;
			}
		}

		adapter->pcie_info.autok_total = 0;
		adapter->pcie_info.autok_2s_cnt = 0;
	}

	return ret;
}

u32 pcie_tp_adjust(struct mac_ax_adapter *adapter,
		   struct mac_ax_tp_param tp)
{
	u32 ret = MACSUCCESS;
	struct mac_ax_pcie_cfgspc_param pcie_cfgspc_param = {
		1,
		0,
		MAC_AX_PCIE_IGNORE,
		MAC_AX_PCIE_IGNORE,
		MAC_AX_PCIE_IGNORE,
		MAC_AX_PCIE_IGNORE,
		MAC_AX_PCIE_IGNORE,
		MAC_AX_PCIE_CLKDLY_IGNORE,
		MAC_AX_PCIE_L0SDLY_IGNORE,
		MAC_AX_PCIE_L1DLY_DEF
	};

	if (tp.tx_tp > PCIE_TP_THOLD || tp.rx_tp > PCIE_TP_THOLD)
		pcie_cfgspc_param.l1dly_ctrl = MAC_AX_PCIE_L1DLY_INFI;
	else
		pcie_cfgspc_param.l1dly_ctrl = MAC_AX_PCIE_L1DLY_DEF;

	ret = cfgspc_set_pcie(adapter, &pcie_cfgspc_param);

	return ret;
}

#ifdef RTW_WKARD_GET_PROCESSOR_ID
u32 chk_proc_long_ldy(struct mac_ax_adapter *adapter, u8 *val)
{
	u8 proc;

	*val = PROC_LONG_DLY;
	for (proc = 0; proc < BASE_BOARD_ID_SHORT_LIST_NUM; proc++) {
		if (!memcmp(adapter->hw_info->adpt_info.cust_proc_id.base_board_id,
			    base_board_id_short_dly[proc], BASE_BOARD_ID_LEN)) {
			*val = PROC_SHORT_DLY;
			break;
		}
	}

	return MACSUCCESS;
}
#endif

u32 sync_trx_bd_idx_pcie(struct mac_ax_adapter *adapter)
{
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 ret = MACSUCCESS;

	if (!(is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	      is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)))
		return MACCHIPID;

	PLTFM_MSG_ALWAYS("Before Halmac Sync TRx BD Idx\n");
	PLTFM_MSG_ALWAYS("DRV_HSK_0 = %x, ACH0_TXBD_IDX = %x\n",
			 MAC_REG_R32(R_AX_DRV_FW_HSK_0),
			 MAC_REG_R32(R_AX_ACH0_TXBD_IDX));
	PLTFM_MSG_ALWAYS("DRV_HSK_6 = %x, RXQ_RXBD_IDX_V1 = %x\n",
			 MAC_REG_R32(R_AX_DRV_FW_HSK_6),
			 MAC_REG_R32(R_AX_RXQ_RXBD_IDX_V1));
	PLTFM_MSG_ALWAYS("DRV_HSK_7 = %x, RPQ_RXBD_IDX_V1 = %x\n",
			 MAC_REG_R32(R_AX_DRV_FW_HSK_7),
			 MAC_REG_R32(R_AX_RPQ_RXBD_IDX_V1));

	ret = sync_tx_bd_idx_ax(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR] sync txbd fail: %d\n", ret);
		return ret;
	}
	ret = sync_rx_bd_idx_ax(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR] sync rxbd fail: %d\n", ret);
		return ret;
	}

	PLTFM_MSG_ALWAYS("after Halmac Sync TRx BD Idx\n");
	PLTFM_MSG_ALWAYS("DRV_HSK_0 = %x, ACH0_TXBD_IDX = %x\n",
			 MAC_REG_R32(R_AX_DRV_FW_HSK_0),
			 MAC_REG_R32(R_AX_ACH0_TXBD_IDX));
	PLTFM_MSG_ALWAYS("DRV_HSK_6 = %x, RXQ_RXBD_IDX_V1 = %x\n",
			 MAC_REG_R32(R_AX_DRV_FW_HSK_6),
			 MAC_REG_R32(R_AX_RXQ_RXBD_IDX_V1));
	PLTFM_MSG_ALWAYS("DRV_HSK_7 = %x, RPQ_RXBD_IDX_V1 = %x\n",
			 MAC_REG_R32(R_AX_DRV_FW_HSK_7),
			 MAC_REG_R32(R_AX_RPQ_RXBD_IDX_V1));
#endif
	return MACSUCCESS;
}

u32 ctrl_txdma_pcie(struct mac_ax_adapter *adapter, u8 opt)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_txdma_ch_map txch_map;
	u32 ret;

	if (opt == RTW_MAC_CTRL_TXDMA_EN_ALL) {
		txch_map.ch0 = MAC_AX_PCIE_ENABLE;
		txch_map.ch1 = MAC_AX_PCIE_ENABLE;
		txch_map.ch2 = MAC_AX_PCIE_ENABLE;
		txch_map.ch3 = MAC_AX_PCIE_ENABLE;
		txch_map.ch4 = MAC_AX_PCIE_ENABLE;
		txch_map.ch5 = MAC_AX_PCIE_ENABLE;
		txch_map.ch6 = MAC_AX_PCIE_ENABLE;
		txch_map.ch7 = MAC_AX_PCIE_ENABLE;
		txch_map.ch8 = MAC_AX_PCIE_ENABLE;
		txch_map.ch9 = MAC_AX_PCIE_ENABLE;
		txch_map.ch10 = MAC_AX_PCIE_ENABLE;
		txch_map.ch11 = MAC_AX_PCIE_ENABLE;
		txch_map.ch12 = MAC_AX_PCIE_ENABLE;
	}
	else {
		txch_map.ch0 = MAC_AX_PCIE_DISABLE;
		txch_map.ch1 = MAC_AX_PCIE_DISABLE;
		txch_map.ch2 = MAC_AX_PCIE_DISABLE;
		txch_map.ch3 = MAC_AX_PCIE_DISABLE;
		txch_map.ch4 = MAC_AX_PCIE_DISABLE;
		txch_map.ch5 = MAC_AX_PCIE_DISABLE;
		txch_map.ch6 = MAC_AX_PCIE_DISABLE;
		txch_map.ch7 = MAC_AX_PCIE_DISABLE;
		txch_map.ch8 = MAC_AX_PCIE_DISABLE;
		txch_map.ch9 = MAC_AX_PCIE_DISABLE;
		txch_map.ch10 = MAC_AX_PCIE_DISABLE;
		txch_map.ch11 = MAC_AX_PCIE_DISABLE;
		if (opt == RTW_MAC_CTRL_TXDMA_H2C2H_ONLY)
			txch_map.ch12 = MAC_AX_PCIE_ENABLE;
		else
			txch_map.ch12 = MAC_AX_PCIE_DISABLE;
	}
	ret = ops->ctrl_txdma_ch(adapter, &txch_map);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("ctrl txdma pcie fail %d\n", ret);
		return ret;
	}

	return MACSUCCESS;
}

u32 poll_txdma_idle_pcie(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_txdma_ch_map txch_map;
	u32 ret;

	txch_map.ch0 = MAC_AX_PCIE_ENABLE;
	txch_map.ch1 = MAC_AX_PCIE_ENABLE;
	txch_map.ch2 = MAC_AX_PCIE_ENABLE;
	txch_map.ch3 = MAC_AX_PCIE_ENABLE;
	txch_map.ch4 = MAC_AX_PCIE_ENABLE;
	txch_map.ch5 = MAC_AX_PCIE_ENABLE;
	txch_map.ch6 = MAC_AX_PCIE_ENABLE;
	txch_map.ch7 = MAC_AX_PCIE_ENABLE;
	txch_map.ch8 = MAC_AX_PCIE_ENABLE;
	txch_map.ch9 = MAC_AX_PCIE_ENABLE;
	txch_map.ch10 = MAC_AX_PCIE_ENABLE;
	txch_map.ch11 = MAC_AX_PCIE_ENABLE;
	txch_map.ch12 = MAC_AX_PCIE_ENABLE;
	ret = ops->poll_txdma_ch_idle(adapter, &txch_map);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("poll txdma idle pcie fail %d\n", ret);
		return ret;
	}

	return MACSUCCESS;
}

u32 clr_hci_trx_pcie(struct mac_ax_adapter *adapter)
{
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	u32 ret;

	ret = ops->clr_idx_all(adapter);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("Clear hci trx pcie fail %d\n", ret);
		return ret;
	}

	ret = p_ops->rst_bdram_pcie(adapter, 0);
	if (ret != MACSUCCESS) {
		PLTFM_MSG_ERR("[ERR]pcie rst bdram %d\n", ret);
		return ret;
	}

	return MACSUCCESS;
}

u32 mac_read_pcie_cfg_spc(struct mac_ax_adapter *adapter, u16 addr, u32 *val)
{
	u32 ret = MACSUCCESS;
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);

#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT
	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) {
		ret = dbi_r32_pcie(adapter, (u16)addr, val);
		if (ret != MACSUCCESS)
			PLTFM_MSG_ERR("DBI r32 fail address: %X\n", addr);
	}
#endif
#if MAC_AX_8852C_SUPPORT || MAC_AX_8852D_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8192XB_SUPPORT
	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E)) {
		ret = p_ops->mio_r32_pcie(adapter, (u16)addr, val);
		if (ret != MACSUCCESS)
			PLTFM_MSG_ERR("MIO r32 fail address: %X\n", addr);
	}
#endif

	return ret;
}

#endif /* #if MAC_AX_PCIE_SUPPORT */


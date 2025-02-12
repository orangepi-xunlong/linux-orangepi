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

#include "power_saving.h"
#include "coex.h"
#include "mac_priv.h"

#define RPWM_SEQ_NUM_MAX                3
#define CPWM_SEQ_NUM_MAX                3

//RPWM bit definition
#define PS_RPWM_TOGGLE			BIT(15)
#define PS_RPWM_ACK             BIT(14)
#define PS_RPWM_SEQ_NUM_SH      12
#define PS_RPWM_SEQ_NUM_MSK     0x3
#define PS_RPWM_NOTIFY_WAKE     BIT(8)
#define PS_RPWM_STATE_SH        0
#define PS_RPWM_STATE_MSK       0x7

//CPWM bit definition
#define PS_CPWM_TOGGLE			BIT(15)
#define PS_CPWM_ACK             BIT(14)
#define PS_CPWM_SEQ_NUM_SH      12
#define PS_CPWM_SEQ_NUM_MSK     0x3
#define PS_CPWM_RSP_SEQ_NUM_SH  8
#define PS_CPWM_RSP_SEQ_NUM_MSK 0x3
#define PS_CPWM_STATE_SH        0
#define PS_CPWM_STATE_MSK       0x7

//(workaround) CPWM register is in OFF area
//LPS debug message bit definition
#define B_PS_LDM_32K_EN         BIT(31)
#define B_PS_LDM_32K_EN_SH      31

// Bcn rx rate
#define R_RXBCNHIT_RATE R_AX_USER_DEFINED_0
#define B_AX_BCN_RATE_SH 0
#define B_AX_BCN_RATE_MSK 0xff
#define B_AX_BCN_HIT_RATE_SH 8
#define B_AX_BCN_HIT_RATE_MSK 0xff
#define B_AX_BCN_NO_HIT_RATE_SH 16
#define B_AX_BCN_NO_HIT_RATE_MSK 0xff
#define B_AX_ROLE_IDX_SH 24
#define B_AX_ROLE_IDX_MSK 0xff

static u32 lps_status[4] = {0};
static u32 ips_status[4] = {0};
static u8 rpwm_seq_num = RPWM_SEQ_NUM_MAX;
static u8 cpwm_seq_num = CPWM_SEQ_NUM_MAX;

static u32 send_h2c_lps_parm(struct mac_ax_adapter *adapter,
			     struct lps_parm *parm)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_lps_parm *fwcmd_lps;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_lps_parm);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_PS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_LPS_PARM;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 0;

	fwcmd_lps = (struct fwcmd_lps_parm *)PLTFM_MALLOC(h2c_info.content_len);
	if (!fwcmd_lps)
		return MACNPTR;

	fwcmd_lps->dword0 =
	cpu_to_le32(SET_WORD(parm->macid, FWCMD_H2C_LPS_PARM_MACID) |
		SET_WORD(parm->psmode, FWCMD_H2C_LPS_PARM_PSMODE) |
		SET_WORD(parm->rlbm, FWCMD_H2C_LPS_PARM_RLBM) |
		SET_WORD(parm->smartps, FWCMD_H2C_LPS_PARM_SMARTPS) |
		SET_WORD(parm->awakeinterval,
			 FWCMD_H2C_LPS_PARM_AWAKEINTERVAL));

	fwcmd_lps->dword1 =
	cpu_to_le32((parm->vouapsd ? FWCMD_H2C_LPS_PARM_VOUAPSD : 0) |
		(parm->viuapsd ? FWCMD_H2C_LPS_PARM_VIUAPSD : 0) |
		(parm->beuapsd ? FWCMD_H2C_LPS_PARM_BEUAPSD : 0) |
		(parm->bkuapsd ? FWCMD_H2C_LPS_PARM_BKUAPSD : 0) |
		SET_WORD(parm->lastrpwm, FWCMD_H2C_LPS_PARM_LASTRPWM) |
		(parm->bcnnohit_en ? FWCMD_H2C_LPS_PARM_BCNNOHIT_EN : 0) |
		(parm->nulltype ? FWCMD_H2C_LPS_PARM_NULLTYPE : 0) |
		(parm->dyntxantnum_en ? FWCMD_H2C_LPS_PARM_DYNTXANTNUM_EN : 0) |
		(parm->maxtxantnum ? FWCMD_H2C_LPS_PARM_MAXTXANTNUM : 0) |
		(parm->lpstxantnum ? FWCMD_H2C_LPS_PARM_LPSTXANTNUM : 0));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)fwcmd_lps);

	PLTFM_FREE(fwcmd_lps, h2c_info.content_len);

	return ret;
}

static void send_rpwm(struct mac_ax_adapter *adapter,
		      struct ps_rpwm_parm *parm)
{
	u16 rpwm_value = 0;
	u8 toggle = 0;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	rpwm_value = MAC_REG_R16(R_AX_RPWM);
	if (0 == (rpwm_value & PS_RPWM_TOGGLE))
		toggle = 1;

	if (parm->notify_wake) {
		rpwm_value |= PS_RPWM_NOTIFY_WAKE;
	} else {
		if (rpwm_seq_num == RPWM_SEQ_NUM_MAX)
			rpwm_seq_num = 0;
		else
			rpwm_seq_num += 1;

		rpwm_value = (SET_WORD(parm->req_pwr_state, PS_RPWM_STATE) |
			SET_WORD(rpwm_seq_num, PS_RPWM_SEQ_NUM));

		if (parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_ACTIVE ||
		    parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_BAND0_RFON ||
		    parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_BAND1_RFON ||
		    parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_BAND0_RFOFF ||
		    parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_BAND1_RFOFF)
			rpwm_value |= PS_RPWM_ACK;

		if (parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_CLK_GATED ||
		    parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_PWR_GATED ||
		    parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_HIOE_PWR_GATED) {
			adapter->mac_pwr_info.pwr_in_lps = 1;
		}
	}

	if (toggle == 1)
		rpwm_value |= PS_RPWM_TOGGLE;
	else
		rpwm_value &= ~PS_RPWM_TOGGLE;

	switch (adapter->hw_info->intf) {
#if MAC_AX_USB_SUPPORT
	case MAC_AX_INTF_USB:
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))
			MAC_REG_W16(R_AX_USB_D2F_F2D_INFO + 2, rpwm_value);
#endif

#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D))
			MAC_REG_W16(R_AX_USB_D2F_F2D_INFO_V1 + 2, rpwm_value);
#endif
		break;
#endif //MAC_AX_USB_SUPPORT

#if MAC_AX_SDIO_SUPPORT
	case MAC_AX_INTF_SDIO:
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))
			MAC_REG_W16(R_AX_SDIO_HRPWM1 + 2, rpwm_value);
#endif

#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D))
			MAC_REG_W16(R_AX_SDIO_HRPWM1_V1 + 2, rpwm_value);
#endif

		if (parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_CLK_GATED ||
		    parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_PWR_GATED ||
		    parm->req_pwr_state == MAC_AX_RPWM_REQ_PWR_STATE_HIOE_PWR_GATED)
			adapter->sdio_info.tx_seq = 1;
		break;
#endif //MAC_AX_SDIO_SUPPORT

#if MAC_AX_PCIE_SUPPORT
	case MAC_AX_INTF_PCIE:
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))
			MAC_REG_W16(R_AX_PCIE_HRPWM, rpwm_value);
#endif

#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D))
			MAC_REG_W16(R_AX_PCIE_HRPWM_V1, rpwm_value);
#endif

		break;
#endif //MAC_AX_PCIE_SUPPORT
	default:
		PLTFM_MSG_ERR("%s: invalid interface = %d!!\n",
			      __func__, adapter->hw_info->intf);

		break;
	}

	PLTFM_DELAY_US(RPWM_DELAY_FOR_32K_TICK);

	PLTFM_MSG_TRACE("Send RPWM. rpwm_val=0x%x\n", rpwm_value);
}

static u32 leave_lps(struct mac_ax_adapter *adapter, u8 macid)
{
	struct lps_parm h2c_lps_parm;
	u32 ret;

	PLTFM_MEMSET(&h2c_lps_parm, 0, sizeof(struct lps_parm));

	h2c_lps_parm.macid = macid;
	h2c_lps_parm.psmode = MAC_AX_PS_MODE_ACTIVE;
	h2c_lps_parm.lastrpwm = LAST_RPWM_ACTIVE;

	ret = send_h2c_lps_parm(adapter, &h2c_lps_parm);

	if (ret)
		return ret;

	lps_status[(macid >> 5)] &= ~BIT(macid & 0x1F);

	return MACSUCCESS;
}

static u32 enter_lps(struct mac_ax_adapter *adapter,
		     u8 macid,
		     struct mac_ax_lps_info *lps_info)
{
	struct lps_parm h2c_lps_parm;
	u32 ret = 0;

	if (!lps_info) {
		PLTFM_MSG_ERR("[ERR]:LPS info is null\n");
		return MACNOITEM;
	}

	if (lps_status[(macid >> 5)] & BIT(macid & 0x1F))
		ret = leave_lps(adapter, macid);

	if (ret)
		return ret;

	PLTFM_MEMSET(&h2c_lps_parm, 0, sizeof(struct lps_parm));

	h2c_lps_parm.macid = macid;
	h2c_lps_parm.psmode = MAC_AX_PS_MODE_LEGACY;

	if (lps_info->listen_bcn_mode > MAC_AX_RLBM_USERDEFINE)
		lps_info->listen_bcn_mode = MAC_AX_RLBM_MIN;

	if (lps_info->listen_bcn_mode == MAC_AX_RLBM_USERDEFINE) {
		h2c_lps_parm.rlbm = MAC_AX_RLBM_USERDEFINE;
		h2c_lps_parm.awakeinterval = lps_info->awake_interval;
		if (h2c_lps_parm.awakeinterval == 0)
			h2c_lps_parm.awakeinterval = 1;
	} else if (lps_info->listen_bcn_mode == MAC_AX_RLBM_MAX) {
		h2c_lps_parm.rlbm = MAC_AX_RLBM_MAX;
		h2c_lps_parm.awakeinterval = 1;
	} else {
		h2c_lps_parm.rlbm = MAC_AX_RLBM_MIN;
		h2c_lps_parm.awakeinterval = 1;
	}

	h2c_lps_parm.smartps = lps_info->smart_ps_mode;
	h2c_lps_parm.lastrpwm = LAST_RPWM_PS;
	h2c_lps_parm.bcnnohit_en = lps_info->bcnnohit_en;
	h2c_lps_parm.nulltype = lps_info->nulltype;

	h2c_lps_parm.dyntxantnum_en = lps_info->dyn_tx_ant_num_en;

	if (h2c_lps_parm.dyntxantnum_en) {
		h2c_lps_parm.maxtxantnum = lps_info->max_tx_ant_num;
		h2c_lps_parm.lpstxantnum = lps_info->lps_tx_ant_num;
	}

	ret = send_h2c_lps_parm(adapter, &h2c_lps_parm);

	if (ret)
		return ret;

	lps_status[(macid >> 5)] |= BIT(macid & 0x1F);

	return MACSUCCESS;
}

static u32 enter_wmmlps(struct mac_ax_adapter *adapter,
			u8 macid,
			struct mac_ax_lps_info *lps_info)
{
	struct lps_parm h2c_lps_parm;
	u32 ret = 0;

	if (!lps_info) {
		PLTFM_MSG_ERR("[ERR]:LPS info is null\n");
		return MACNOITEM;
	}

	if (lps_status[(macid >> 5)] & BIT(macid & 0x1F))
		ret = leave_lps(adapter, macid);

	if (ret)
		return ret;

	PLTFM_MEMSET(&h2c_lps_parm, 0, sizeof(struct lps_parm));

	h2c_lps_parm.macid = macid;
	h2c_lps_parm.psmode = MAC_AX_PS_MODE_WMMPS;

	if (lps_info->listen_bcn_mode > MAC_AX_RLBM_USERDEFINE)
		lps_info->listen_bcn_mode = MAC_AX_RLBM_MIN;

	if (lps_info->listen_bcn_mode == MAC_AX_RLBM_USERDEFINE) {
		h2c_lps_parm.rlbm = MAC_AX_RLBM_USERDEFINE;
		h2c_lps_parm.awakeinterval = lps_info->awake_interval;
		if (h2c_lps_parm.awakeinterval == 0)
			h2c_lps_parm.awakeinterval = 1;
	} else if (lps_info->listen_bcn_mode == MAC_AX_RLBM_MAX) {
		h2c_lps_parm.rlbm = MAC_AX_RLBM_MAX;
		h2c_lps_parm.awakeinterval = 1;
	} else {
		h2c_lps_parm.rlbm = MAC_AX_RLBM_MIN;
		h2c_lps_parm.awakeinterval = 1;
	}

	h2c_lps_parm.lastrpwm = LAST_RPWM_PS;
	h2c_lps_parm.bcnnohit_en = lps_info->bcnnohit_en;
	h2c_lps_parm.vouapsd = lps_info->vouapsd_en;
	h2c_lps_parm.viuapsd = lps_info->viuapsd_en;
	h2c_lps_parm.beuapsd = lps_info->beuapsd_en;
	h2c_lps_parm.bkuapsd = lps_info->bkuapsd_en;

	ret = send_h2c_lps_parm(adapter, &h2c_lps_parm);

	if (ret)
		return ret;

	lps_status[(macid >> 5)] |= BIT(macid & 0x1F);

	return MACSUCCESS;
}

static u32 set_req_pwr_state(struct mac_ax_adapter *adapter,
			     enum mac_ax_rpwm_req_pwr_state req_pwr_state)
{
	struct ps_rpwm_parm parm;

	PLTFM_MEMSET(&parm, 0, sizeof(struct ps_rpwm_parm));

	if (req_pwr_state >= MAC_AX_RPWM_REQ_PWR_STATE_MAX) {
		PLTFM_MSG_ERR("%s: invalid pwr state:%d\n",
			      __func__, req_pwr_state);
		return MACNOITEM;
	}

	parm.req_pwr_state = req_pwr_state;
	parm.notify_wake = 0;
	send_rpwm(adapter, &parm);

	return MACSUCCESS;
}

static u32 _chk_cpwm_seq_num(u8 seq_num)
{
	u32 ret;

	if (cpwm_seq_num == CPWM_SEQ_NUM_MAX) {
		if (seq_num == 0) {
			cpwm_seq_num = seq_num;
			ret = MACSUCCESS;
		} else {
			ret = MACCPWMSEQERR;
		}
	} else {
		if (seq_num == (cpwm_seq_num + 1)) {
			cpwm_seq_num = seq_num;
			ret = MACSUCCESS;
		} else {
			ret = MACCPWMSEQERR;
		}
	}

	return ret;
}

static u32 chk_cur_pwr_state(struct mac_ax_adapter *adapter,
			     enum mac_ax_rpwm_req_pwr_state req_pwr_state)
{
	u16 cpwm = 0;
	u32 rpwm_32k;
	u32 req_32k;
#if MAC_AX_PCIE_SUPPORT
	struct mac_ax_priv_ops *p_ops = adapter_to_priv_ops(adapter);
	u32 ret = MACSUCCESS;
#endif
#if MAC_AX_USB_SUPPORT
#define USB_ON_IOH_SW_RST_RETRY_CNT 100
#define USB_ON_IOH_SW_RST_WAIT_US 50
	u32 val32, cnt;
#endif
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (req_pwr_state >= MAC_AX_RPWM_REQ_PWR_STATE_CLK_GATED)
		req_32k = 1;
	else
		req_32k = 0;

	//(workaround) CPWM register is in OFF area
	//Use LDM to check if FW receives RPWM
	rpwm_32k = (MAC_REG_R32(R_AX_LDM) & B_PS_LDM_32K_EN) >> B_PS_LDM_32K_EN_SH;
	if (req_32k != rpwm_32k)
		return MACCPWMPWRSTATERR;

	//There is no CPWM if 32K state
	if (req_32k)
		return MACSUCCESS;

#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT))
		cpwm = MAC_REG_R16(R_AX_CPWM);
#endif

#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
		switch (adapter->hw_info->intf) {
#if MAC_AX_USB_SUPPORT
		case MAC_AX_INTF_USB:
			if (chk_patch_usb_on_ioh_sw_rst(adapter) == PATCH_ENABLE) {
				val32 = MAC_REG_R32(R_AX_USB_IO_OFFREG_WDT_V1) |
						    B_AX_ON_IOH_SW_RST_V1;
				MAC_REG_W32(R_AX_USB_IO_OFFREG_WDT_V1, val32);
				cnt = USB_ON_IOH_SW_RST_RETRY_CNT;
				while (--cnt) {
					val32 = MAC_REG_R32(R_AX_USB_IO_OFFREG_WDT_V1);
					if (!(val32 & B_AX_OFF_IOH_RST_STS_V1))
						break;
					PLTFM_DELAY_US(USB_ON_IOH_SW_RST_WAIT_US);
				}
				if (!cnt)
					PLTFM_MSG_ERR("%s: reset off wrapper polling timeout!!\n",
						      __func__);
			}
			cpwm = MAC_REG_R16(R_AX_USB_D2F_F2D_INFO_V1);
			break;
#endif // MAC_AX_USB_SUPPORT

#if MAC_AX_SDIO_SUPPORT
		case MAC_AX_INTF_SDIO:
			return MACCPWMINTFERR;
			break;
#endif // MAC_AX_SDIO_SUPPORT

#if MAC_AX_PCIE_SUPPORT
		case MAC_AX_INTF_PCIE:
			cpwm = MAC_REG_R16(R_AX_PCIE_CRPWM);
			break;
#endif // MAC_AX_PCIE_SUPPORT
		default:
			PLTFM_MSG_ERR("%s: invalid interface = %d!!\n",
				      __func__, adapter->hw_info->intf);
			return MACCPWMINTFERR;
			break;
		}
	}
#endif

	PLTFM_MSG_TRACE("Read CPWM=0x%x\n", cpwm);
	if (rpwm_seq_num != GET_FIELD(cpwm, PS_CPWM_RSP_SEQ_NUM)) {
		PLTFM_MSG_TRACE("RPWM seq mismatch!!: expect val:%d, Rx val:%d\n",
				rpwm_seq_num, GET_FIELD(cpwm, PS_CPWM_RSP_SEQ_NUM));
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) {
			return MACCPWMSEQERR;
		}
#endif
	}

	if (_chk_cpwm_seq_num(GET_FIELD(cpwm, PS_CPWM_SEQ_NUM)) == MACCPWMSEQERR) {
		PLTFM_MSG_TRACE("CPWM seq mismatch!!: expect val:%d, Rx val:%d\n",
				cpwm_seq_num, GET_FIELD(cpwm, PS_CPWM_SEQ_NUM));
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT || MAC_AX_8852BT_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT)) {
			return MACCPWMSEQERR;
		}
#endif
	}

	if (req_pwr_state != GET_FIELD(cpwm, PS_CPWM_STATE))
		return MACCPWMSTATERR;

	if (adapter->mac_pwr_info.pwr_in_lps) {
		adapter->mac_pwr_info.pwr_in_lps = 0;
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
#if MAC_AX_PCIE_SUPPORT
			if (adapter->hw_info->intf == MAC_AX_INTF_PCIE) {
				ret = p_ops->sync_trx_bd_idx(adapter);
				if (ret != MACSUCCESS) {
					PLTFM_MSG_ERR("sync trx bd fail: %d\n",
						      ret);
					return ret;
				}
			}
#endif
		}
#endif
	}

	return MACSUCCESS;
}

u32 mac_cfg_lps(struct mac_ax_adapter *adapter, u8 macid,
		enum mac_ax_ps_mode ps_mode, struct mac_ax_lps_info *lps_info)
{
	u32 ret = 0;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	switch (ps_mode) {
	case MAC_AX_PS_MODE_ACTIVE:
		ret = leave_lps(adapter, macid);
		/* patch form BT BG/LDO issue */
		MAC_REG_W8(R_AX_SCOREBOARD + 3, MAC_AX_NOTIFY_TP_MAJOR);
		break;
	case MAC_AX_PS_MODE_LEGACY:
		ret = enter_lps(adapter, macid, lps_info);
		/* patch form BT BG/LDO issue */
		MAC_REG_W8(R_AX_SCOREBOARD + 3, MAC_AX_NOTIFY_PWR_MAJOR);
		break;
	case MAC_AX_PS_MODE_WMMPS:
		ret = enter_wmmlps(adapter, macid, lps_info);
		/* patch form BT BG/LDO issue */
		MAC_REG_W8(R_AX_SCOREBOARD + 3, MAC_AX_NOTIFY_PWR_MAJOR);
		break;
	default:
		return MACNOITEM;
	}

	if (ret)
		return ret;

	return MACSUCCESS;
}

u32 mac_ps_pwr_state(struct mac_ax_adapter *adapter,
		     enum mac_ax_pwr_state_action action,
		     enum mac_ax_rpwm_req_pwr_state req_pwr_state)
{
	u32 ret = MACSUCCESS;

	switch (action) {
	case MAC_AX_PWR_STATE_ACT_REQ:
		ret = set_req_pwr_state(adapter, req_pwr_state);
		break;
	case MAC_AX_PWR_STATE_ACT_CHK:
		ret = chk_cur_pwr_state(adapter, req_pwr_state);
		break;
	default:
		ret = MACNOITEM;
	}

	return ret;
}

void show_rx_bcn_info(struct mac_ax_adapter *adapter)
{
#if MAC_AX_8852C_SUPPORT || MAC_AX_8192XB_SUPPORT || MAC_AX_8851E_SUPPORT || MAC_AX_8852D_SUPPORT
	u8 role_idx, bcn_rate, hit_rate, no_hit_rate;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
	    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
		role_idx = (u8)GET_FIELD(MAC_REG_R32(R_RXBCNHIT_RATE), B_AX_ROLE_IDX);
		bcn_rate = (u8)GET_FIELD(MAC_REG_R32(R_RXBCNHIT_RATE), B_AX_BCN_RATE);
		hit_rate = (u8)GET_FIELD(MAC_REG_R32(R_RXBCNHIT_RATE), B_AX_BCN_HIT_RATE);
		no_hit_rate = (u8)GET_FIELD(MAC_REG_R32(R_RXBCNHIT_RATE), B_AX_BCN_NO_HIT_RATE);
		PLTFM_MSG_ALWAYS("role_idx: %d, bcn_rx_rate = %d%%, hit_rate = %d%%, no_hit_rate = %d%%\n",
				 role_idx, bcn_rate, hit_rate, no_hit_rate);
	}
#endif
}

u32 mac_chk_leave_lps(struct mac_ax_adapter *adapter, u8 macid)
{
	u8 band = 0;
	u8 port = 0;
	u32 chk_msk = 0;
	struct mac_role_tbl *role;
	u16 pwrbit_set_reg[2] = {R_AX_PPWRBIT_SETTING, R_AX_PPWRBIT_SETTING_C1};
	u32 pwr_mgt_en_bit = 0xE;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_pause = 0;
	u32 reg_sleep = 0;
	u8 macid_grp = macid >> MACID_GRP_SH;
	u8 macid_sh = macid & MACID_GRP_MASK;

	role = mac_role_srch(adapter, macid);

	if (!role) {
		PLTFM_MSG_ERR("[ERR]cannot find macid: %d\n", macid);
		return MACNOITEM;
	}

	band = role->info.a_info.bb_sel;
	port = role->info.a_info.port_int;

	chk_msk = pwr_mgt_en_bit << (PORT_SH * port);
	switch (macid_grp) {
	case MACID_GRP_0:
		reg_sleep = R_AX_MACID_SLEEP_0;
		reg_pause = R_AX_SS_MACID_PAUSE_0;
		break;
	case MACID_GRP_1:
		reg_sleep = R_AX_MACID_SLEEP_1;
		reg_pause = R_AX_SS_MACID_PAUSE_1;
		break;
	case MACID_GRP_2:
		reg_sleep = R_AX_MACID_SLEEP_2;
		reg_pause = R_AX_SS_MACID_PAUSE_2;
		break;
	case MACID_GRP_3:
		reg_sleep = R_AX_MACID_SLEEP_3;
		reg_pause = R_AX_SS_MACID_PAUSE_3;
		break;
	default:
		return MACPSSTATFAIL;
	}

	// Bypass Tx pause check during STOP SER period
	if (adapter->sm.ser_ctrl_st != MAC_AX_SER_CTRL_STOP)
		if (MAC_REG_R32(reg_pause) & BIT(macid_sh))
			return MACPSSTATFAIL;

	if ((MAC_REG_R32(reg_sleep) & BIT(macid_sh)))
		return MACPSSTATFAIL;

	if ((MAC_REG_R32(pwrbit_set_reg[band]) & chk_msk))
		return MACPSSTATPWRBITFAIL;

	show_rx_bcn_info(adapter);

#if MAC_AX_PCIE_SUPPORT
	if (adapter->hw_info->intf == MAC_AX_INTF_PCIE) {
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
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
			u32 ret;

			ret = ops->ltr_set_pcie(adapter, &ltr_param);
			if (ret != MACSUCCESS) {
				PLTFM_MSG_ERR("[ERR]pcie ltr set fail %d\n", ret);
				return ret;
			}
#endif
		}
	}
#endif

	return MACSUCCESS;
}

u8 _is_in_lps(struct mac_ax_adapter *adapter)
{
	u8 i;

	for (i = 0; i < 4; i++) {
		if (lps_status[i] != 0)
			return 1;
	}

	return 0;
}

void reset_lps_seq_num(struct mac_ax_adapter *adapter)
{
	rpwm_seq_num = RPWM_SEQ_NUM_MAX;
	cpwm_seq_num = CPWM_SEQ_NUM_MAX;
}

static u32 send_h2c_ips_cfg(struct mac_ax_adapter *adapter,
			    struct ips_cfg *cfg)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_ips_cfg *fwcmd_ips;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_ips_cfg);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_PS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_IPS_CFG;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;

	fwcmd_ips = (struct fwcmd_ips_cfg *)PLTFM_MALLOC(h2c_info.content_len);
	if (!fwcmd_ips)
		return MACNPTR;

	fwcmd_ips->dword0 =
	cpu_to_le32(SET_WORD(cfg->macid, FWCMD_H2C_IPS_CFG_MACID) |
		(cfg->enable ? FWCMD_H2C_IPS_CFG_ENABLE : 0));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)fwcmd_ips);

	PLTFM_FREE(fwcmd_ips, h2c_info.content_len);

	return ret;
}

static u32 leave_ips(struct mac_ax_adapter *adapter, u8 macid)
{
	struct ips_cfg h2c_ips_cfg;
	u32 ret;

	PLTFM_MEMSET(&h2c_ips_cfg, 0, sizeof(struct ips_cfg));

	h2c_ips_cfg.macid = macid;
	h2c_ips_cfg.enable = 0;

	ret = send_h2c_ips_cfg(adapter, &h2c_ips_cfg);
	if (ret != MACSUCCESS)
		return ret;

	ips_status[(macid >> 5)] &= ~BIT(macid & 0x1F);

	return MACSUCCESS;
}

static u32 enter_ips(struct mac_ax_adapter *adapter,
		     u8 macid)
{
	struct ips_cfg h2c_ips_cfg;
	u32 ret;

	if (ips_status[(macid >> 5)] & BIT(macid & 0x1F)) {
		PLTFM_MSG_ERR("[ERR]:IPS info is null\n");
		ret = leave_ips(adapter, macid);
		if (ret != MACSUCCESS)
			return ret;
	}

	PLTFM_MEMSET(&h2c_ips_cfg, 0, sizeof(struct ips_cfg));

	h2c_ips_cfg.macid = macid;
	h2c_ips_cfg.enable = 1;

	ret = send_h2c_ips_cfg(adapter, &h2c_ips_cfg);
	if (ret != MACSUCCESS)
		return ret;

	ips_status[(macid >> 5)] |= BIT(macid & 0x1F);

	return MACSUCCESS;
}

u32 mac_cfg_ips(struct mac_ax_adapter *adapter, u8 macid,
		u8 enable)
{
	u32 ret;

	if (enable == 1)
		ret = enter_ips(adapter, macid);
	else
		ret = leave_ips(adapter, macid);

	return ret;
}

u32 mac_chk_leave_ips(struct mac_ax_adapter *adapter, u8 macid)
{
	u8 band = 0;
	u8 port = 0;
	u32 chk_msk = 0;
	struct mac_role_tbl *role;
	u32 pwr_mgt_en_bit = 0xE;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);
	u32 reg_pause = 0;
	u32 reg_sleep = 0;
	u8 macid_grp = macid >> MACID_GRP_SH;
	u8 macid_sh = macid & MACID_GRP_MASK;

	role = mac_role_srch(adapter, macid);

	if (!role) {
		PLTFM_MSG_ERR("[ERR]cannot find macid: %d\n", macid);
		return MACNOITEM;
	}

	band = role->info.a_info.bb_sel;
	port = role->info.a_info.port_int;

	chk_msk = pwr_mgt_en_bit << (PORT_SH * port);
	switch (macid_grp) {
	case MACID_GRP_0:
		reg_sleep = R_AX_MACID_SLEEP_0;
		reg_pause = R_AX_SS_MACID_PAUSE_0;
		break;
	case MACID_GRP_1:
		reg_sleep = R_AX_MACID_SLEEP_1;
		reg_pause = R_AX_SS_MACID_PAUSE_1;
		break;
	case MACID_GRP_2:
		reg_sleep = R_AX_MACID_SLEEP_2;
		reg_pause = R_AX_SS_MACID_PAUSE_2;
		break;
	case MACID_GRP_3:
		reg_sleep = R_AX_MACID_SLEEP_3;
		reg_pause = R_AX_SS_MACID_PAUSE_3;
		break;
	default:
		return MACPSSTATFAIL;
	}

	// Bypass Tx pause check during STOP SER period
	if (adapter->sm.ser_ctrl_st != MAC_AX_SER_CTRL_STOP)
		if (MAC_REG_R32(reg_pause) & BIT(macid_sh))
			return MACPSSTATFAIL;

	if (MAC_REG_R32(reg_sleep) & BIT(macid_sh))
		return MACPSSTATFAIL;

#if MAC_AX_PCIE_SUPPORT
	if (adapter->hw_info->intf == MAC_AX_INTF_PCIE) {
		if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) ||
		    is_chip_id(adapter, MAC_AX_CHIP_ID_8852D)) {
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
			u32 ret;

			ret = ops->ltr_set_pcie(adapter, &ltr_param);
			if (ret != MACSUCCESS) {
				PLTFM_MSG_ERR("[ERR]pcie ltr set fail %d\n", ret);
				return ret;
			}
#endif
		}
	}
#endif

	return MACSUCCESS;
}

u8 _is_in_ips(struct mac_ax_adapter *adapter)
{
	u8 i;

	for (i = 0; i < 4; i++) {
		if (ips_status[i] != 0)
			return 1;
	}

	return 0;
}

u32 mac_ps_notify_wake(struct mac_ax_adapter *adapter)
{
	struct ps_rpwm_parm parm;

	PLTFM_MEMSET(&parm, 0, sizeof(struct ps_rpwm_parm));

	if (adapter->mac_pwr_info.pwr_in_lps == 0) {
		PLTFM_MSG_ERR("%s: Not in power saving!\n", __func__);
		return MACPWRSTAT;
	}

	parm.notify_wake = 1;
	send_rpwm(adapter, &parm);

	return MACSUCCESS;
}

u32 mac_cfg_ps_advance_parm(struct mac_ax_adapter *adapter,
			    struct mac_ax_ps_adv_parm *parm)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_ps_advance_parm *fwcmd_parm;

	if (!parm)
		return MACBADDR;

	PLTFM_MSG_ALWAYS("%s: MACID(%d), TRXTimeOutTimeSet(%d), TRXTimeOutTimeVal(%d)!\n",
			 __func__, parm->macid, parm->trxtimeouttimeset, parm->trxtimeouttimeval);
	PLTFM_MSG_ALWAYS("%s: EnSmartPsDtimRx(%d)!\n", __func__, parm->ensmartpsdtimrx);

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_ps_advance_parm);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_PS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_PS_ADVANCE_PARM;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;

	fwcmd_parm = (struct fwcmd_ps_advance_parm *)PLTFM_MALLOC(h2c_info.content_len);
	if (!fwcmd_parm) {
		PLTFM_MSG_ERR("%s: fwcmd_parm malloc fail!\n", __func__);
		return MACNPTR;
	}

	fwcmd_parm->dword0 =
	cpu_to_le32(SET_WORD(parm->macid, FWCMD_H2C_PS_ADVANCE_PARM_MACID) |
		    SET_WORD(parm->trxtimeouttimeset, FWCMD_H2C_PS_ADVANCE_PARM_TRXTIMEOUTTIMESET) |
		    (parm->ensmartpsdtimrx ? FWCMD_H2C_PS_ADVANCE_PARM_ENSMARTPSDTIMRX : 0) |
		    (parm->entrxextmode ? FWCMD_H2C_PS_ADVANCE_PARM_TRXEXTMODE : 0));
	fwcmd_parm->dword1 =
	cpu_to_le32(SET_WORD(parm->trxtimeouttimeval, FWCMD_H2C_PS_ADVANCE_PARM_TRXTIMEOUTTIMEVAL) |
		    SET_WORD(parm->extnum, FWCMD_H2C_PS_ADVANCE_PARM_EXTNUM));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)fwcmd_parm);
	if (ret != MACSUCCESS)
		PLTFM_MSG_ERR("%s: H2C sent fail (%d)!\n", __func__, ret);

	PLTFM_FREE(fwcmd_parm, h2c_info.content_len);

	return ret;
}

static u32 send_h2c_pw_cfg(struct mac_ax_adapter *adapter,
			   struct periodic_wake_cfg *parm)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_periodic_wake *fwcmd_pw;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_periodic_wake);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_PS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_PERIODIC_WAKE;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 1;

	fwcmd_pw = (struct fwcmd_periodic_wake *)PLTFM_MALLOC(h2c_info.content_len);
	if (!fwcmd_pw)
		return MACNPTR;

	fwcmd_pw->dword0 =
	cpu_to_le32(SET_WORD(parm->macid, FWCMD_H2C_PERIODIC_WAKE_MACID) |
		(parm->enable ? FWCMD_H2C_PERIODIC_WAKE_ENABLE : 0) |
		(parm->band ? FWCMD_H2C_PERIODIC_WAKE_BAND : 0) |
		SET_WORD(parm->port, FWCMD_H2C_PERIODIC_WAKE_PORT));
	fwcmd_pw->dword1 = cpu_to_le32(parm->wake_period);
	fwcmd_pw->dword2 = cpu_to_le32(parm->wake_duration);

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)fwcmd_pw);

	PLTFM_FREE(fwcmd_pw, h2c_info.content_len);

	return ret;
}

u32 mac_periodic_wake_cfg(struct mac_ax_adapter *adapter,
			  struct mac_ax_periodic_wake_info pw_info)
{
	u32 ret = MACSUCCESS;
	struct periodic_wake_cfg parm;

	if (adapter->sm.fwdl != MAC_AX_FWDL_INIT_RDY)
		return MACNOFW;

	PLTFM_MEMSET(&parm, 0, sizeof(struct periodic_wake_cfg));
	parm.macid = pw_info.macid;
	parm.enable = pw_info.enable;
	parm.band = pw_info.band;
	parm.port = pw_info.port;
	parm.wake_period = pw_info.wake_period;
	parm.wake_duration = pw_info.wake_duration;

	PLTFM_MSG_ALWAYS("%s: macid(%d), enable(%d), band(%d), port(%d)\n",
			 __func__, parm.macid, parm.enable, parm.band, parm.port);
	PLTFM_MSG_ALWAYS("%s: wake_period(%d), wake_duration(%d)\n",
			 __func__, parm.wake_period, parm.wake_duration);

	ret = send_h2c_pw_cfg(adapter, &parm);
	if (ret)
		return ret;

	return ret;
}

u32 send_h2c_req_pwr_state(struct mac_ax_adapter *adapter,
			   struct req_pwr_state_cfg *parm)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_bb_rf_pwr_st *fwcmd_req_pwr_st;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_bb_rf_pwr_st);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_PS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_BB_RF_PWR_ST;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 0;

	fwcmd_req_pwr_st = (struct fwcmd_bb_rf_pwr_st *)PLTFM_MALLOC(h2c_info.content_len);
	if (!fwcmd_req_pwr_st)
		return MACNPTR;

	fwcmd_req_pwr_st->dword0 =
	cpu_to_le32(SET_WORD(parm->req_pwr_state, FWCMD_H2C_BB_RF_PWR_ST_BB_RF_PWR_REQ_ST));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)fwcmd_req_pwr_st);

	PLTFM_FREE(fwcmd_req_pwr_st, h2c_info.content_len);

	return ret;
}

u32 set_pwr_st_cfg(struct mac_ax_adapter *adapter,
		   struct req_pwr_state_cfg *parm)
{
	u32 ret = MACSUCCESS;
	struct mac_ax_intf_ops *ops = adapter_to_intf_ops(adapter);

	switch (parm->req_pwr_state) {
	case REQ_PWR_ST_ADC_OFF:
		MAC_REG_W8(R_AX_ANAPAR_POW_MAC, (MAC_REG_R8(R_AX_ANAPAR_POW_MAC) &
			   ~(B_AX_POW_PC_LDO_PORT0 | B_AX_POW_PC_LDO_PORT1)));
		break;
	case REQ_PWR_ST_BB_OFF:
		MAC_REG_W8(R_AX_SYS_FUNC_EN, (MAC_REG_R8(R_AX_SYS_FUNC_EN) &
			   ~(B_AX_FEN_BBRSTB | B_AX_FEN_BB_GLB_RSTN)));
		MAC_REG_W8(R_AX_SYS_ISO_CTRL_EXTEND, (MAC_REG_R8(R_AX_SYS_ISO_CTRL_EXTEND) |
			   B_AX_R_SYM_ISO_BB2PP));
		MAC_REG_W16(R_AX_AFE_LDO_CTRL, (MAC_REG_R16(R_AX_AFE_LDO_CTRL) &
			    ~(B_AX_R_SYM_WLBBOFF_PC_EN | B_AX_R_SYM_WLBBOFF_P1_PC_EN |
			    B_AX_R_SYM_WLBBOFF_P2_PC_EN | B_AX_R_SYM_WLBBOFF_P3_PC_EN |
			    B_AX_R_SYM_WLBBOFF_P4_PC_EN | B_AX_R_SYM_WLBBOFF1_P1_PC_EN |
			    B_AX_R_SYM_WLBBOFF1_P2_PC_EN | B_AX_R_SYM_WLBBOFF1_P3_PC_EN |
			    B_AX_R_SYM_WLBBOFF1_P4_PC_EN)));
		break;
	case REQ_PWR_ST_CPU_OFF:
		MAC_REG_W8(R_AX_PLATFORM_ENABLE, (MAC_REG_R8(R_AX_PLATFORM_ENABLE) &
			   ~B_AX_WCPU_EN));
		MAC_REG_W16(R_AX_SYS_CLK_CTRL, (MAC_REG_R16(R_AX_SYS_CLK_CTRL) &
			    ~(B_AX_CPU_CLK_EN | B_AX_CPU_IDMEM_CLK_EN)));
		MAC_REG_W32(R_AX_SYS_ISO_CTRL_EXTEND, (MAC_REG_R32(R_AX_SYS_ISO_CTRL_EXTEND) |
			    B_AX_R_SYM_ISO_IMEM02PP | B_AX_R_SYM_ISO_IMEM12PP |
			    B_AX_R_SYM_ISO_IMEM22PP | B_AX_R_SYM_ISO_IMEM32PP |
			    B_AX_R_SYM_ISO_IMEM42PP | B_AX_R_SYM_ISO_DMEM12PP_V1 |
			    B_AX_R_SYM_ISO_DMEM22PP_V1 | B_AX_R_SYM_ISO_DMEM32PP_V1 |
			    B_AX_R_SYM_ISO_DMEM42PP | B_AX_R_SYM_ISO_DMEM52PP |
			    B_AX_R_SYM_ISO_DMEM62PP));
		MAC_REG_W32(R_AX_AFE_CTRL1, (MAC_REG_R32(R_AX_AFE_CTRL1) &
			    ~(B_AX_IMEM0_PC_EN | B_AX_IMEM1_PC_EN | B_AX_IMEM2_PC_EN |
			    B_AX_IMEM3_PC_EN | B_AX_IMEM4_PC_EN | B_AX_DMEM1_PC_EN |
			    B_AX_DMEM2_PC_EN | B_AX_DMEM3_PC_EN | B_AX_DMEM4_PC_EN |
			    B_AX_DMEM5_PC_EN | B_AX_DMEM6_PC_EN)));
		break;
	case REQ_PWR_ST_MAC_OFF:
		MAC_REG_W32(R_AX_SYS_ISO_CTRL_EXTEND, (MAC_REG_R32(R_AX_SYS_ISO_CTRL_EXTEND) &
			    ~B_AX_R_SYM_FEN_WLMACOFF));
		MAC_REG_W16(R_AX_SYS_CLK_CTRL, (MAC_REG_R16(R_AX_SYS_CLK_CTRL) & ~B_AX_MAC_CLK_EN));
		MAC_REG_W8(R_AX_PMC_DBG_CTRL2, (MAC_REG_R8(R_AX_PMC_DBG_CTRL2) |
			   B_AX_SYSON_DIS_PMCR_AX_WRMSK));
		MAC_REG_W8(R_AX_SYS_ISO_CTRL, (MAC_REG_R8(R_AX_SYS_ISO_CTRL) | B_AX_ISO_WLPON2PP));
		MAC_REG_W8(R_AX_SYS_ISO_CTRL, (MAC_REG_R8(R_AX_SYS_ISO_CTRL) | B_AX_ISO_WD2PP));
		MAC_REG_W32(R_AX_AFE_LDO_CTRL, (MAC_REG_R32(R_AX_AFE_LDO_CTRL) &
			    ~(B_AX_R_SYM_WLPOFF_PC_EN | B_AX_R_SYM_WLPOFF_P1_PC_EN |
			    B_AX_R_SYM_WLPOFF_P2_PC_EN | B_AX_R_SYM_WLPOFF_P3_PC_EN |
			    B_AX_R_SYM_WLPOFF_P4_PC_EN)));
		MAC_REG_W32(R_AX_AFE_LDO_CTRL, (MAC_REG_R32(R_AX_AFE_LDO_CTRL) &
			    ~(B_AX_R_SYM_WLPON_PC_EN | B_AX_R_SYM_WLPON_P1_PC_EN |
			    B_AX_R_SYM_WLPON_P2_PC_EN | B_AX_R_SYM_WLPON_P3_PC_EN)));
		break;
	case REQ_PWR_ST_PLL_OFF:
		MAC_REG_W8(R_AX_ANAPAR_POW_MAC, (MAC_REG_R8(R_AX_ANAPAR_POW_MAC) &
			   ~(B_AX_POW_POWER_CUT_POW_LDO | B_AX_POW_PLL_V1)));
		break;
	case REQ_PWR_ST_SWRD_OFF:
		MAC_REG_W8(R_AX_ANAPARLDO_POW_MAC, (MAC_REG_R8(R_AX_ANAPARLDO_POW_MAC) &
			   ~B_AX_POW_SW_SPSDIG));
		break;
	case REQ_PWR_ST_XTAL_OFF:
		MAC_REG_W32(R_AX_WLAN_XTAL_SI_CTRL, REQ_PWR_ST_XTAL_OFF_VAL);
		break;
	case REQ_PWR_ST_ADIE_OFF:
		MAC_REG_W32(R_AX_SYM_ANAPAR_XTAL_MODE_DECODER,
			    (MAC_REG_R32(R_AX_SYM_ANAPAR_XTAL_MODE_DECODER) |
			    B_AX_ADIE_CTRL_BY_SW));
		MAC_REG_W32(R_AX_SYM_ANAPAR_XTAL_MODE_DECODER,
			    (MAC_REG_R32(R_AX_SYM_ANAPAR_XTAL_MODE_DECODER) & ~B_AX_ADIE_EN));
		break;
	case REQ_PWR_ST_BYPASS_DATA_ON:
		MAC_REG_W32(R_AX_RCR, (MAC_REG_R32(R_AX_RCR) | B_AX_STOP_RX_IN));
		if ((MAC_REG_R32(R_AX_SYS_ISO_CTRL_EXTEND) & B_AX_CMAC1_FEN) == B_AX_CMAC1_FEN)
			MAC_REG_W32(R_AX_RCR_C1, (MAC_REG_R32(R_AX_RCR_C1) | B_AX_STOP_RX_IN));
		break;
	case REQ_PWR_ST_MP_INTER_BCN:
		// set LPS option
#if MAC_AX_PCIE_SUPPORT
		if (adapter->hw_info->intf == MAC_AX_INTF_PCIE)
			MAC_REG_W32(R_AX_WLLPS_CTRL, MP_INTER_BCN_LPS_OP_PCIE);
#endif
#if MAC_AX_USB_SUPPORT
		if (adapter->hw_info->intf == MAC_AX_INTF_USB)
			MAC_REG_W32(R_AX_WLLPS_CTRL, MP_INTER_BCN_LPS_OP_USB);
#endif
#if MAC_AX_SDIO_SUPPORT
		if (adapter->hw_info->intf == MAC_AX_INTF_USB)
			MAC_REG_W32(R_AX_WLLPS_CTRL, MP_INTER_BCN_LPS_OP_SDIO);
#endif
		// enter LPS
		MAC_REG_W32(R_AX_WLLPS_CTRL, (MAC_REG_R32(R_AX_WLLPS_CTRL) | B_AX_WL_LPS_EN));
		break;
	default:
		return MACNOITEM;
	}

	return ret;
}

u32 mac_req_pwr_state_cfg(struct mac_ax_adapter *adapter,
			  enum mac_req_pwr_st req_pwr_st)
{
	u32 ret = MACSUCCESS;
	struct req_pwr_state_cfg parm;

	PLTFM_MEMSET(&parm, 0, sizeof(struct req_pwr_state_cfg));
	parm.req_pwr_state = req_pwr_st;

	switch (req_pwr_st) {
	case REQ_PWR_ST_OPEN_RF:
		ret = send_h2c_req_pwr_state(adapter, &parm);
		break;
	case REQ_PWR_ST_CLOSE_RF:
		ret = send_h2c_req_pwr_state(adapter, &parm);
		break;
	case REQ_PWR_ST_ADC_OFF:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_BB_OFF:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_CPU_OFF:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_MAC_OFF:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_PLL_OFF:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_SWRD_OFF:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_XTAL_OFF:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_ADIE_OFF:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_BYPASS_DATA_ON:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	case REQ_PWR_ST_MP_INTER_BCN:
		ret = set_pwr_st_cfg(adapter, &parm);
		break;
	default:
		return MACNOITEM;
	}

	return ret;
}

u32 send_h2c_req_pwr_lvl(struct mac_ax_adapter *adapter,
			 struct req_pwr_lvl_cfg *parm)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_ps_power_level *fwcmd_req_pwr_lvl;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_ps_power_level);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_PS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_PS_POWER_LEVEL;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 0;

	fwcmd_req_pwr_lvl = (struct fwcmd_ps_power_level *)PLTFM_MALLOC(h2c_info.content_len);
	if (!fwcmd_req_pwr_lvl)
		return MACNPTR;

	fwcmd_req_pwr_lvl->dword0 =
	cpu_to_le32(SET_WORD(parm->macid, FWCMD_H2C_PS_POWER_LEVEL_MACID) |
		    SET_WORD(parm->bcn_to_val, FWCMD_H2C_PS_POWER_LEVEL_BCN_TO_VAL) |
		    SET_WORD(parm->ps_lvl, FWCMD_H2C_PS_POWER_LEVEL_PS_LVL) |
		    SET_WORD(parm->trx_lvl, FWCMD_H2C_PS_POWER_LEVEL_TRX_LVL));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)fwcmd_req_pwr_lvl);

	PLTFM_FREE(fwcmd_req_pwr_lvl, h2c_info.content_len);

	return ret;
}

u32 mac_req_pwr_lvl_cfg(struct mac_ax_adapter *adapter,
			struct mac_ax_req_pwr_lvl_info *pwr_lvl_info)
{
	u32 ret = MACSUCCESS;
	struct req_pwr_lvl_cfg parm;

	PLTFM_MEMSET(&parm, 0, sizeof(struct req_pwr_lvl_cfg));

	parm.macid = pwr_lvl_info->macid;

	if (pwr_lvl_info->ps_lvl >= REQ_PS_LVL_MAX ||
	    pwr_lvl_info->trx_lvl >= REQ_TRX_LVL_MAX ||
	    pwr_lvl_info->bcn_to_lvl >= REQ_BCN_TO_LVL_MAX)
		return MACNOITEM;

	if (pwr_lvl_info->bcn_to_val == REQ_BCN_TO_VAL_NONVALID)
		pwr_lvl_info->bcn_to_val = REQ_BCN_TO_VAL_MIN << pwr_lvl_info->bcn_to_lvl;
	else if (pwr_lvl_info->bcn_to_val < REQ_BCN_TO_VAL_MIN)
		pwr_lvl_info->bcn_to_val = REQ_BCN_TO_VAL_MIN;
	else if (pwr_lvl_info->bcn_to_val > REQ_BCN_TO_VAL_MAX)
		pwr_lvl_info->bcn_to_val = REQ_BCN_TO_VAL_MAX;

	parm.bcn_to_val = pwr_lvl_info->bcn_to_val;
	parm.ps_lvl = pwr_lvl_info->ps_lvl;
	parm.trx_lvl = pwr_lvl_info->trx_lvl;

	ret = send_h2c_req_pwr_lvl(adapter, &parm);

	return ret;
}

u32 send_h2c_lps_option_cfg(struct mac_ax_adapter *adapter,
			    struct lps_option_cfg *parm)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_lps_option_cfg *fwcmd_lps_option;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_lps_option_cfg);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_PS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_LPS_OPTION_CFG;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 0;

	fwcmd_lps_option = (struct fwcmd_lps_option_cfg *)PLTFM_MALLOC(h2c_info.content_len);
	if (!fwcmd_lps_option)
		return MACNPTR;

	fwcmd_lps_option->dword0 =
	cpu_to_le32(parm->req_lps_option ? FWCMD_H2C_LPS_OPTION_CFG_REQ_XTAL_OPTION : 0);

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)fwcmd_lps_option);

	PLTFM_FREE(fwcmd_lps_option, h2c_info.content_len);

	return ret;
}

u32 mac_lps_option_cfg(struct mac_ax_adapter *adapter,
		       struct rtw_mac_lps_option *lps_opt)
{
	u32 ret = MACSUCCESS;
	struct lps_option_cfg parm;

	PLTFM_MEMSET(&parm, 0, sizeof(struct lps_option_cfg));

#if MAC_AX_USB_SUPPORT
	if (adapter->hw_info->intf == MAC_AX_INTF_USB) {
		parm.req_lps_option = lps_opt->req_xtal_option;
		ret = send_h2c_lps_option_cfg(adapter, &parm);
	}
#endif //MAC_AX_USB_SUPPORT
	return ret;
}

u32 send_h2c_tbtt_tuning(struct mac_ax_adapter *adapter,
			 struct tbtt_tuning_cfg *parm)
{
	u32 ret = MACSUCCESS;
	struct h2c_info h2c_info = {0};
	struct fwcmd_tbtt_tuning *fwcmd_tbtt_tuning_info;

	h2c_info.agg_en = 0;
	h2c_info.content_len = sizeof(struct fwcmd_tbtt_tuning);
	h2c_info.h2c_cat = FWCMD_H2C_CAT_MAC;
	h2c_info.h2c_class = FWCMD_H2C_CL_PS;
	h2c_info.h2c_func = FWCMD_H2C_FUNC_TBTT_TUNING;
	h2c_info.rec_ack = 0;
	h2c_info.done_ack = 0;

	fwcmd_tbtt_tuning_info = (struct fwcmd_tbtt_tuning *)PLTFM_MALLOC(h2c_info.content_len);
	if (!fwcmd_tbtt_tuning_info)
		return MACNPTR;

	fwcmd_tbtt_tuning_info->dword0 =
	cpu_to_le32(SET_WORD(parm->band, FWCMD_H2C_TBTT_TUNING_BAND) |
		    SET_WORD(parm->port, FWCMD_H2C_TBTT_TUNING_PORT));

	fwcmd_tbtt_tuning_info->dword1 =
	cpu_to_le32(SET_WORD(parm->shift_val, FWCMD_H2C_TBTT_TUNING_SHIFT_VAL));

	ret = mac_h2c_common(adapter, &h2c_info, (u32 *)fwcmd_tbtt_tuning_info);

	PLTFM_FREE(fwcmd_tbtt_tuning_info, h2c_info.content_len);

	return ret;
}

u32 mac_tbtt_tuning_cfg(struct mac_ax_adapter *adapter,
			struct mac_ax_tbtt_tuning_info *tbtt_tuning_info)
{
	u32 ret = MACSUCCESS;
	struct tbtt_tuning_cfg parm;
#if MAC_AX_8852A_SUPPORT || MAC_AX_8852B_SUPPORT || MAC_AX_8851B_SUPPORT
	struct mac_ax_port_cfg_para cfg_para;

	if (is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) ||
		is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) ||
		is_chip_id(adapter, MAC_AX_CHIP_ID_8851B)) {

		cfg_para.mbssid_idx = 0;
		cfg_para.val = tbtt_tuning_info->shift_val/TU_UNIT;
		cfg_para.port = (u8)tbtt_tuning_info->port;
		cfg_para.band = (u8)tbtt_tuning_info->band;
		ret = mac_port_cfg(adapter, MAC_AX_PCFG_TBTT_SHIFT, &cfg_para);

		return ret;
	}
#endif
	PLTFM_MEMSET(&parm, 0, sizeof(struct tbtt_tuning_cfg));

	if (tbtt_tuning_info->band < MAC_AX_BAND_NUM)
		parm.band = tbtt_tuning_info->band;
	else
		return MACNOITEM;

	if (tbtt_tuning_info->port < MAC_AX_PORT_NUM)
		parm.port = tbtt_tuning_info->port;
	else
		return MACNOITEM;

	parm.shift_val = tbtt_tuning_info->shift_val;

	ret = send_h2c_tbtt_tuning(adapter, &parm);

	return ret;
}

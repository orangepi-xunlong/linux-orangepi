/******************************************************************************
 *
 * Copyright(c) 2017 - 2019 Realtek Corporation. All rights reserved.
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

#include "halmac_pcie_8822c.h"
#include "halmac_pwr_seq_8822c.h"
#include "../halmac_init_88xx.h"
#include "../halmac_common_88xx.h"
#include "../halmac_pcie_88xx.h"
#include "../halmac_88xx_cfg.h"

#if (HALMAC_8822C_SUPPORT && HALMAC_PCIE_SUPPORT)

/**
 * mac_pwr_switch_pcie_8822c() - switch mac power
 * @adapter : the adapter of halmac
 * @pwr : power state
 * Author : KaiYuan Chang
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */

#define INTF_INTGRA_MINREF_V1	90
#define INTF_INTGRA_HOSTREF_V1	100

static struct halmac_pcie_cfgspc_param pcie_cfgspc_param_def = {
	0,
	0,
	HALMAC_DISABLE,
	HALMAC_ENABLE,
	HALMAC_ENABLE,
	HALMAC_ENABLE,
	HALMAC_IGNORE,
	HALMAC_CLKDLY_0,
	HALMAC_L0SDLY_7US,
	HALMAC_L1DLY_16US,
};

enum pcie_clkdly_hw {
	PCIE_CLKDLY_HW_0 = 0,
	PCIE_CLKDLY_HW_30US = 0x1,
	PCIE_CLKDLY_HW_50US = 0x2,
	PCIE_CLKDLY_HW_100US = 0x3,
	PCIE_CLKDLY_HW_150US = 0x4,
	PCIE_CLKDLY_HW_200US = 0x5
};

enum pcie_l1dly_hw {
	PCIE_L1DLY_HW_16US = 4,
	PCIE_L1DLY_HW_32US = 5,
	PCIE_L1DLY_HW_64US = 6,
	PCIE_L1DLY_HW_INFI = 7
};

enum pcie_l0sdly_hw {
	PCIE_L0SDLY_HW_1US = 0,
	PCIE_L0SDLY_HW_3US = 2,
	PCIE_L0SDLY_HW_5US = 4,
	PCIE_L0SDLY_HW_7US = 6
};

#define GET_PCIE_FUNC_STUS(val, mask)                                          \
			  ((val & mask) ? HALMAC_ENABLE : HALMAC_DISABLE)

static u16
get_target(struct halmac_adapter *adapter);

static enum halmac_ret_status
pcie_cfgspc_write_8822c(struct halmac_adapter *adapter,
			struct halmac_pcie_cfgspc_param *param);

static enum halmac_ret_status
pcie_cfgspc_read_8822c(struct halmac_adapter *adapter,
		       struct halmac_pcie_cfgspc_param *param);

static void
update_pcie_func_8822c(u8 *val, u8 bitmask, enum halmac_func_ctrl ctrl,
		       enum halmac_func_ctrl def_ctrl);

static u8
chk_stus_l1ss_8822c(struct halmac_adapter *adapter);

static enum halmac_ret_status
update_clkdly_8822c(struct halmac_adapter *adapter, u8 *val,
		    enum halmac_pcie_clkdly ctrl,
		    enum halmac_pcie_clkdly def_ctrl);

static enum halmac_ret_status
update_pcie_clk_8822c(struct halmac_adapter *adapter, u8 *val);

static enum halmac_ret_status
update_aspmdly_8822c(struct halmac_adapter *adapter, u8 *val,
		     struct halmac_pcie_cfgspc_param *param,
		     struct halmac_pcie_cfgspc_param *param_def);

enum halmac_ret_status
mac_pwr_switch_pcie_8822c(struct halmac_adapter *adapter,
			  enum halmac_mac_power pwr)
{
	u8 value8;
	u8 rpwm;
	struct halmac_api *api = (struct halmac_api *)adapter->halmac_api;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);
	PLTFM_MSG_TRACE("[TRACE]pwr = %x\n", pwr);
	PLTFM_MSG_TRACE("[TRACE]8822C pwr seq ver = %s\n",
			HALMAC_8822C_PWR_SEQ_VER);

	adapter->rpwm = HALMAC_REG_R8(REG_PCIE_HRPWM1_V1);

	/* Check FW still exist or not */
	if (HALMAC_REG_R16(REG_MCUFW_CTRL) == 0xC078) {
		/* Leave 32K */
		rpwm = (u8)((adapter->rpwm ^ BIT(7)) & 0x80);
		HALMAC_REG_W8(REG_PCIE_HRPWM1_V1, rpwm);
	}

	value8 = HALMAC_REG_R8(REG_CR);
	if (value8 == 0xEA)
		adapter->halmac_state.mac_pwr = HALMAC_MAC_POWER_OFF;
	else
		adapter->halmac_state.mac_pwr = HALMAC_MAC_POWER_ON;

	/* Check if power switch is needed */
	if (pwr == HALMAC_MAC_POWER_ON &&
	    adapter->halmac_state.mac_pwr == HALMAC_MAC_POWER_ON) {
		PLTFM_MSG_WARN("[WARN]power state unchange!!\n");
		return HALMAC_RET_PWR_UNCHANGE;
	}

	if (pwr == HALMAC_MAC_POWER_OFF) {
		if (pwr_seq_parser_88xx(adapter, card_dis_flow_8822c) !=
		    HALMAC_RET_SUCCESS) {
			PLTFM_MSG_ERR("[ERR]Handle power off cmd error\n");
			return HALMAC_RET_POWER_OFF_FAIL;
		}

		adapter->halmac_state.mac_pwr = HALMAC_MAC_POWER_OFF;
		adapter->halmac_state.dlfw_state = HALMAC_DLFW_NONE;
		init_adapter_dynamic_param_88xx(adapter);
	} else {
		if (pwr_seq_parser_88xx(adapter, card_en_flow_8822c) !=
		    HALMAC_RET_SUCCESS) {
			PLTFM_MSG_ERR("[ERR]Handle power on cmd error\n");
			return HALMAC_RET_POWER_ON_FAIL;
		}

		adapter->halmac_state.mac_pwr = HALMAC_MAC_POWER_ON;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

/**
 * halmac_pcie_switch_8822c() - pcie gen1/gen2 switch
 * @adapter : the adapter of halmac
 * @cfg : gen1/gen2 selection
 * Author : KaiYuan Chang
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
pcie_switch_8822c(struct halmac_adapter *adapter, enum halmac_pcie_cfg cfg)
{
	u8 value8;
	u32 value32;
	u8 speed = 0;
	u32 cnt = 0;
	enum halmac_ret_status status;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	if (cfg == HALMAC_PCIE_GEN1) {
		value8 = dbi_r8_88xx(adapter, LINK_CTRL2_REG_OFFSET) & 0xF0;
		status = dbi_w8_88xx(adapter, LINK_CTRL2_REG_OFFSET,
				     value8 | BIT(0));
		if (status != HALMAC_RET_SUCCESS) {
			PLTFM_MSG_ERR("[ERR]DBI write 8 bits fail!\n");
			return status;
		}

		value32 = dbi_r32_88xx(adapter, GEN2_CTRL_OFFSET);
		status = dbi_w32_88xx(adapter, GEN2_CTRL_OFFSET,
				      value32 | BIT(17));
		if (status != HALMAC_RET_SUCCESS) {
			PLTFM_MSG_ERR("[ERR]DBI write 32 bits fail!\n");
			return status;
		}

		speed = dbi_r8_88xx(adapter, LINK_STATUS_REG_OFFSET) & 0x0F;
		cnt = 2000;

		while ((speed != PCIE_GEN1_SPEED) && (cnt != 0)) {
			PLTFM_DELAY_US(50);
			speed = dbi_r8_88xx(adapter, LINK_STATUS_REG_OFFSET);
			speed &= 0x0F;
			cnt--;
		}

		if (speed != PCIE_GEN1_SPEED) {
			PLTFM_MSG_ERR("[ERR]Speed change to GEN1 fail !\n");
			return HALMAC_RET_FAIL;
		}

	} else if (cfg == HALMAC_PCIE_GEN2) {
		value8 = dbi_r8_88xx(adapter, LINK_CTRL2_REG_OFFSET) & 0xF0;
		status = dbi_w8_88xx(adapter, LINK_CTRL2_REG_OFFSET,
				     value8 | BIT(1));
		if (status != HALMAC_RET_SUCCESS) {
			PLTFM_MSG_ERR("[ERR]DBI write 8 bits fail!\n");
			return status;
		}

		value32 = dbi_r32_88xx(adapter, GEN2_CTRL_OFFSET);
		status = dbi_w32_88xx(adapter, GEN2_CTRL_OFFSET,
				      value32 | BIT(17));
		if (status != HALMAC_RET_SUCCESS) {
			PLTFM_MSG_ERR("[ERR]DBI write 32 bits fail!\n");
			return status;
		}

		speed = dbi_r8_88xx(adapter, LINK_STATUS_REG_OFFSET) & 0x0F;
		cnt = 2000;

		while ((speed != PCIE_GEN2_SPEED) && (cnt != 0)) {
			PLTFM_DELAY_US(50);
			speed = dbi_r8_88xx(adapter, LINK_STATUS_REG_OFFSET);
			speed &= 0x0F;
			cnt--;
		}

		if (speed != PCIE_GEN2_SPEED) {
			PLTFM_MSG_ERR("[ERR]Speed change to GEN1 fail !\n");
			return HALMAC_RET_FAIL;
		}

	} else {
		PLTFM_MSG_ERR("[ERR]Error Speed !\n");
		return HALMAC_RET_FAIL;
	}

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

/**
 * phy_cfg_pcie_8822c() - phy config
 * @adapter : the adapter of halmac
 * Author : KaiYuan Chang
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
phy_cfg_pcie_8822c(struct halmac_adapter *adapter,
		   enum halmac_intf_phy_platform pltfm)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	PLTFM_MSG_TRACE("[TRACE]%s ===>\n", __func__);

	status = parse_intf_phy_88xx(adapter, pcie_gen1_phy_param_8822c, pltfm,
				     HAL_INTF_PHY_PCIE_GEN1);

	if (status != HALMAC_RET_SUCCESS)
		return status;

	status = parse_intf_phy_88xx(adapter, pcie_gen2_phy_param_8822c, pltfm,
				     HAL_INTF_PHY_PCIE_GEN2);

	if (status != HALMAC_RET_SUCCESS)
		return status;

	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	return HALMAC_RET_SUCCESS;
}

/**
 * intf_tun_pcie_8822c() - pcie interface fine tuning
 * @adapter : the adapter of halmac
 * Author : Rick Liu
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
intf_tun_pcie_8822c(struct halmac_adapter *adapter)
{
	return HALMAC_RET_SUCCESS;
}

/**
 * cfgspc_set_pcie_8822c() - pcie configuration space setting
 * @adapter : the adapter of halmac
 * Author : Rick Liu
 * Return : enum halmac_ret_status
 * More details of status code can be found in prototype document
 */
enum halmac_ret_status
cfgspc_set_pcie_8822c(struct halmac_adapter *adapter,
		      struct halmac_pcie_cfgspc_param *param)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	if (param->write == 1)
		status = pcie_cfgspc_write_8822c(adapter, param);

	if (param->read == 1)
		status = pcie_cfgspc_read_8822c(adapter, param);

	return status;
}

static enum halmac_ret_status
pcie_cfgspc_write_8822c(struct halmac_adapter *adapter,
			struct halmac_pcie_cfgspc_param *param)
{
	u8 l1_val;
	u8 aspm_val;
	u8 l1ss_val;
	u8 clk_val;
	struct halmac_pcie_cfgspc_param *param_def = &pcie_cfgspc_param_def;
	enum halmac_ret_status status;

	l1_val = dbi_r8_88xx(adapter, PCIE_L1_CTRL);
	aspm_val = dbi_r8_88xx(adapter, PCIE_ASPM_CTRL);
	l1ss_val = dbi_r8_88xx(adapter, PCIE_L1SS_CTRL);
	clk_val = dbi_r8_88xx(adapter, PCIE_CLK_CTRL);
	if (l1_val == 0xFF || aspm_val == 0xFF || l1ss_val == 0xFF ||
	    clk_val == 0xFF) {
		PLTFM_MSG_ERR("[ERR] PCIE CFG reg read 0xFF!\n");
		return HALMAC_RET_FAIL;
	}

	update_pcie_func_8822c(&aspm_val, PCIE_BIT_L0S,
			       param->l0s_ctrl, param_def->l0s_ctrl);

	status = update_pcie_clk_8822c(adapter, &l1_val);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	update_pcie_func_8822c(&l1_val, PCIE_BIT_L1,
			       param->l1_ctrl, param_def->l1_ctrl);
	update_pcie_func_8822c(&l1_val, PCIE_BIT_WAKE,
			       param->wake_ctrl, param_def->wake_ctrl);
	if (chk_stus_l1ss_8822c(adapter) == 1)
		update_pcie_func_8822c(&l1ss_val, PCIE_BIT_L1SS,
				       param->l1ss_ctrl, param_def->l1ss_ctrl);
	status = update_clkdly_8822c(adapter, &clk_val,
				     param->clkdly_ctrl,
				     param_def->clkdly_ctrl);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	status = update_aspmdly_8822c(adapter, &aspm_val, param, param_def);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	if (param->l0s_ctrl != HALMAC_IGNORE ||
	    param->l1dly_ctrl != HALMAC_L1DLY_IGNORE ||
	    param->l0sdly_ctrl != HALMAC_L0SDLY_IGNORE) {
		status = dbi_w8_88xx(adapter, PCIE_ASPM_CTRL, aspm_val);
		if (status != HALMAC_RET_SUCCESS)
			return status;
	}
	if (param->l1_ctrl != HALMAC_IGNORE ||
	    param->wake_ctrl != HALMAC_IGNORE) {
		status = dbi_w8_88xx(adapter, PCIE_L1_CTRL, l1_val);
		if (status != HALMAC_RET_SUCCESS)
			return status;
	}
	if (param->l1ss_ctrl != HALMAC_IGNORE) {
		status = dbi_w8_88xx(adapter, PCIE_L1SS_CTRL, l1ss_val);
		if (status != HALMAC_RET_SUCCESS)
			return status;
	}
	if (param->clkdly_ctrl != HALMAC_CLKDLY_IGNORE) {
		status = dbi_w8_88xx(adapter, PCIE_CLK_CTRL, clk_val);
		if (status != HALMAC_RET_SUCCESS)
			return status;
	}

	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
pcie_cfgspc_read_8822c(struct halmac_adapter *adapter,
		       struct halmac_pcie_cfgspc_param *param)
{
	u8 l1_val;
	u8 aspm_val;
	u8 l1ss_val;
	u8 clk_val;
	u8 l0smask;
	u8 l1mask;

	l1_val = dbi_r8_88xx(adapter, PCIE_L1_CTRL);
	aspm_val = dbi_r8_88xx(adapter, PCIE_ASPM_CTRL);
	l1ss_val = dbi_r8_88xx(adapter, PCIE_L1SS_CTRL);
	clk_val = dbi_r8_88xx(adapter, PCIE_CLK_CTRL);
	if (l1_val == 0xFF || aspm_val == 0xFF ||
	    l1ss_val == 0xFF || clk_val == 0xFF) {
		PLTFM_MSG_ERR("[ERR] (2nd)PCIE CFG reg read 0xFF!\n");
		return HALMAC_RET_FAIL;
	}

	param->l0s_ctrl = GET_PCIE_FUNC_STUS(aspm_val, PCIE_BIT_L0S);
	param->l1_ctrl = GET_PCIE_FUNC_STUS(l1_val, PCIE_BIT_L1);
	param->l1ss_ctrl = GET_PCIE_FUNC_STUS(l1ss_val, PCIE_BIT_L1SS);
	param->wake_ctrl = GET_PCIE_FUNC_STUS(l1_val, PCIE_BIT_WAKE);
	param->crq_ctrl = GET_PCIE_FUNC_STUS(l1_val, PCIE_BIT_CLK);

	switch (clk_val) {
	case PCIE_CLKDLY_HW_0:
		param->clkdly_ctrl = HALMAC_CLKDLY_0;
		break;

	case PCIE_CLKDLY_HW_30US:
		param->clkdly_ctrl = HALMAC_CLKDLY_30US;
		break;

	case PCIE_CLKDLY_HW_50US:
		param->clkdly_ctrl = HALMAC_CLKDLY_50US;
		break;

	case PCIE_CLKDLY_HW_100US:
		param->clkdly_ctrl = HALMAC_CLKDLY_100US;
		break;

	case PCIE_CLKDLY_HW_150US:
		param->clkdly_ctrl = HALMAC_CLKDLY_150US;
		break;

	case PCIE_CLKDLY_HW_200US:
		param->clkdly_ctrl = HALMAC_CLKDLY_200US;
		break;

	default:
		param->clkdly_ctrl = HALMAC_CLKDLY_R_ERR;
		break;
	}

	l0smask = PCIE_ASPMDLY_MASK << SHFT_L0SDLY;
	l1mask = PCIE_ASPMDLY_MASK << SHFT_L1DLY;

	switch ((aspm_val & l0smask) >> SHFT_L0SDLY) {
	case PCIE_L0SDLY_HW_1US:
		param->l0sdly_ctrl = HALMAC_L0SDLY_1US;
		break;

	case PCIE_L0SDLY_HW_3US:
		param->l0sdly_ctrl = HALMAC_L0SDLY_3US;
		break;

	case PCIE_L0SDLY_HW_5US:
		param->l0sdly_ctrl = HALMAC_L0SDLY_5US;
		break;

	case PCIE_L0SDLY_HW_7US:
		param->l0sdly_ctrl = HALMAC_L0SDLY_7US;
		break;

	default:
		param->l0sdly_ctrl = HALMAC_L0SDLY_R_ERR;
		break;
	}

	switch ((aspm_val & l1mask) >> SHFT_L1DLY) {
	case PCIE_L1DLY_HW_16US:
		param->l1dly_ctrl = HALMAC_L1DLY_16US;
		break;

	case PCIE_L1DLY_HW_32US:
		param->l1dly_ctrl = HALMAC_L1DLY_32US;
		break;

	case PCIE_L1DLY_HW_64US:
		param->l1dly_ctrl = HALMAC_L1DLY_64US;
		break;

	case PCIE_L1DLY_HW_INFI:
		param->l1dly_ctrl = HALMAC_L1DLY_INFI;
		break;

	default:
		param->l1dly_ctrl = HALMAC_L1DLY_R_ERR;
		break;
	}

	return HALMAC_RET_SUCCESS;
}

static void
update_pcie_func_8822c(u8 *val, u8 bitmask, enum halmac_func_ctrl ctrl,
		       enum halmac_func_ctrl def_ctrl)
{
	if ((ctrl == HALMAC_DEFAULT &&
	     (def_ctrl == HALMAC_IGNORE || def_ctrl == HALMAC_DEFAULT)) ||
	    ctrl == HALMAC_IGNORE)
		return;

	if ((ctrl == HALMAC_DEFAULT && def_ctrl == HALMAC_DISABLE) ||
	    ctrl == HALMAC_DISABLE)
		*val &= ~(bitmask);
	else
		*val |= bitmask;
}

static u8
chk_stus_l1ss_8822c(struct halmac_adapter *adapter)
{
	u16 cap_val;
	u8 stus_val;
	u8 sup_val;

	cap_val = (u16)((dbi_r8_88xx(adapter, PCIE_L1SS_CAP + 1) << 8) |
			 dbi_r8_88xx(adapter, PCIE_L1SS_CAP));
	sup_val = dbi_r8_88xx(adapter, PCIE_L1SS_SUP);
	stus_val = dbi_r8_88xx(adapter, PCIE_L1SS_STS);

	if (cap_val == PCIE_L1SS_ID &&
	    (sup_val & PCIE_BIT_L1SSSUP) &&
	    (sup_val & PCIE_L1SS_MASK) != 0 &&
	    (stus_val & PCIE_L1SS_MASK) != 0)
		return 1;

	return 0;
}

static enum halmac_ret_status
update_clkdly_8822c(struct halmac_adapter *adapter, u8 *val,
		    enum halmac_pcie_clkdly ctrl,
		    enum halmac_pcie_clkdly def_ctrl)
{
	u8 tmp;

	if (ctrl == HALMAC_CLKDLY_IGNORE)
		return HALMAC_RET_SUCCESS;

	tmp = (ctrl == HALMAC_CLKDLY_DEF) ? def_ctrl : ctrl;
	switch (tmp) {
	case HALMAC_CLKDLY_0:
		*val = PCIE_CLKDLY_HW_0;
		break;

	case HALMAC_CLKDLY_30US:
		*val = PCIE_CLKDLY_HW_30US;
		break;

	case HALMAC_CLKDLY_50US:
		*val = PCIE_CLKDLY_HW_50US;
		break;

	case HALMAC_CLKDLY_100US:
		*val = PCIE_CLKDLY_HW_100US;
		break;

	case HALMAC_CLKDLY_150US:
		*val = PCIE_CLKDLY_HW_150US;
		break;

	case HALMAC_CLKDLY_200US:
		*val = PCIE_CLKDLY_HW_200US;
		break;

	default:
		PLTFM_MSG_ERR("[ERR]CLKDLY wt val illegal!\n");
		return HALMAC_RET_FAIL;
	}
	return HALMAC_RET_SUCCESS;
}

static enum halmac_ret_status
update_pcie_clk_8822c(struct halmac_adapter *adapter, u8 *val)
{
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	if (*val & PCIE_BIT_CLK)
		return HALMAC_RET_SUCCESS;

	if (*val & PCIE_BIT_L1) {
		*val &= ~(PCIE_BIT_L1);
		status = dbi_w8_88xx(adapter, PCIE_L1_CTRL, *val);
		if (status != HALMAC_RET_SUCCESS)
			return status;
		*val |= PCIE_BIT_CLK;
		status = dbi_w8_88xx(adapter, PCIE_L1_CTRL, *val);
		if (status != HALMAC_RET_SUCCESS)
			return status;
		*val |= PCIE_BIT_L1;
		status = dbi_w8_88xx(adapter, PCIE_L1_CTRL, *val);
		PLTFM_MSG_WARN("[WARN] L1 enable & CLKREQ disable!\n");
	} else {
		*val |= PCIE_BIT_CLK;
		status = dbi_w8_88xx(adapter, PCIE_L1_CTRL, *val);
	}

	return status;
}

static enum halmac_ret_status
update_aspmdly_8822c(struct halmac_adapter *adapter, u8 *val,
		     struct halmac_pcie_cfgspc_param *param,
		     struct halmac_pcie_cfgspc_param *param_def)
{
	u8 l1mask = PCIE_ASPMDLY_MASK << SHFT_L1DLY;
	u8 l0smask = PCIE_ASPMDLY_MASK << SHFT_L0SDLY;
	u8 l1updval = param->l1dly_ctrl;
	u8 l0supdval = param->l0sdly_ctrl;
	u8 l1defval = param_def->l1dly_ctrl;
	u8 l0sdefval = param_def->l0sdly_ctrl;
	u8 tmp;
	u8 hwval;

	if (l1updval != HALMAC_L1DLY_IGNORE) {
		tmp = (l1updval == HALMAC_L1DLY_DEF) ? l1defval : l1updval;
		switch (tmp) {
		case HALMAC_L1DLY_16US:
			hwval = PCIE_L1DLY_HW_16US;
			break;

		case HALMAC_L1DLY_32US:
			hwval = PCIE_L1DLY_HW_32US;
			break;

		case HALMAC_L1DLY_64US:
			hwval = PCIE_L1DLY_HW_64US;
			break;

		case HALMAC_L1DLY_INFI:
			hwval = PCIE_L1DLY_HW_INFI;
			break;

		default:
			PLTFM_MSG_ERR("[ERR]L1DLY wt val illegal!\n");
			return HALMAC_RET_FAIL;
		}

		tmp = (hwval << SHFT_L1DLY) & l1mask;
		*val = (*val & ~(l1mask)) | tmp;
	}

	if (l0supdval != HALMAC_L0SDLY_IGNORE) {
		tmp = (l0supdval == HALMAC_L0SDLY_DEF) ? l0sdefval : l0supdval;
		switch (tmp) {
		case HALMAC_L0SDLY_1US:
			hwval = PCIE_L0SDLY_HW_1US;
			break;

		case HALMAC_L0SDLY_3US:
			hwval = PCIE_L0SDLY_HW_3US;
			break;

		case HALMAC_L0SDLY_5US:
			hwval = PCIE_L0SDLY_HW_5US;
			break;

		case HALMAC_L0SDLY_7US:
			hwval = PCIE_L0SDLY_HW_7US;
			break;

		default:
			PLTFM_MSG_ERR("[ERR]L0SDLY wt val illegal!\n");
			return HALMAC_RET_FAIL;
		}
		tmp = (hwval << SHFT_L0SDLY) & l0smask;
		*val = (*val & ~(l0smask)) | tmp;
	}

	return HALMAC_RET_SUCCESS;
}

enum halmac_ret_status
auto_refclk_cal_8822c_pcie(struct halmac_adapter *adapter)
{
	u8 bdr_ori;
	u16 tmp_u16;
	u16 div_set;
	u16 mgn_tmp;
	u16 mgn_set;
	u16 tar;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;
	u8 l1_flag = 0;

#if (INTF_INTGRA_HOSTREF_V1 <= INTF_INTGRA_MINREF_V1)
	return status;
#endif
	/* Disable L1BD */
	bdr_ori = dbi_r8_88xx(adapter, PCIE_L1_CTRL);
	if (bdr_ori & PCIE_BIT_L1) {
		status = dbi_w8_88xx(adapter, PCIE_L1_CTRL,
				     bdr_ori & ~(PCIE_BIT_L1));
		if (status != HALMAC_RET_SUCCESS)
			return status;
		l1_flag = 1;
	}

	/* Disable function */
	tmp_u16 = mdio_read_88xx(adapter, RAC_CTRL_PPR_V1,
				 HAL_INTF_PHY_PCIE_GEN1);
	if (tmp_u16 & BIT(13)) {
		status = mdio_write_88xx(adapter, RAC_CTRL_PPR_V1,
					 tmp_u16 & ~(BIT(13)),
					 HAL_INTF_PHY_PCIE_GEN1);
		if (status != HALMAC_RET_SUCCESS)
			return status;
	}
	if (adapter->pcie_refautok_en == 0) {
		if (l1_flag == 1)
			status = dbi_w8_88xx(adapter, PCIE_L1_CTRL, bdr_ori);
		return status;
	}

	/* Set div */
	tmp_u16 = mdio_read_88xx(adapter, RAC_CTRL_PPR_V1
				 , HAL_INTF_PHY_PCIE_GEN1);
	status = mdio_write_88xx(adapter, RAC_CTRL_PPR_V1,
				 tmp_u16 & ~(BIT(15) | BIT(14)),
				 HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	/*  Obtain div and margin */
	tar = get_target(adapter);
	if (tar == 0xFFFF)
		return HALMAC_RET_FAIL;
	mgn_tmp = tar * INTF_INTGRA_HOSTREF_V1 / INTF_INTGRA_MINREF_V1 - tar;

	if (mgn_tmp >= 128) {
		div_set = 0x0003;
		mgn_set = 0x000F;
	} else if (mgn_tmp >= 64) {
		div_set = 0x0003;
		mgn_set = mgn_tmp >> 3;
	} else if (mgn_tmp >= 32) {
		div_set = 0x0002;
		mgn_set = mgn_tmp >> 2;
	} else if (mgn_tmp >= 16) {
		div_set = 0x0001;
		mgn_set = mgn_tmp >> 1;
	} else if (mgn_tmp == 0) {
		div_set = 0x0000;
		mgn_set = 0x0001;
	} else {
		div_set = 0x0000;
		mgn_set = mgn_tmp;
	}

	/* Set div, margin, target*/
	tmp_u16 = mdio_read_88xx(adapter, RAC_CTRL_PPR_V1,
				 HAL_INTF_PHY_PCIE_GEN1);
	tmp_u16 = (tmp_u16 & ~(BIT(15) | BIT(14))) | (div_set << 14);
	status = mdio_write_88xx(adapter, RAC_CTRL_PPR_V1,
				 tmp_u16, HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;
	tar = get_target(adapter);
	if (tar == 0xFFFF)
		return HALMAC_RET_FAIL;
	PLTFM_MSG_TRACE("[TRACE]target = 0x%X, div = 0x%X, margin = 0x%X\n",
			tar, div_set, mgn_set);
	status = mdio_write_88xx(adapter, RAC_SET_PPR_V1,
				 (tar & 0x0FFF) | (mgn_set << 12),
				 HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;

	/* Enable function */
	tmp_u16 = mdio_read_88xx(adapter, RAC_CTRL_PPR_V1,
				 HAL_INTF_PHY_PCIE_GEN1);
	status = mdio_write_88xx(adapter, RAC_CTRL_PPR_V1, tmp_u16 | BIT(13),
				 HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return status;
	PLTFM_MSG_TRACE("[TRACE]%s <===\n", __func__);

	/* Set L1BD to ori */
	if (l1_flag == 1)
		status = dbi_w8_88xx(adapter, PCIE_L1_CTRL, bdr_ori);

	return status;
}

static u16
get_target(struct halmac_adapter *adapter)
{
	u16 tmp_u16;
	u16 tar;
	u16 count;
	enum halmac_ret_status status = HALMAC_RET_SUCCESS;

	/* Enable counter */
	tmp_u16 = mdio_read_88xx(adapter, RAC_CTRL_PPR_V1,
				 HAL_INTF_PHY_PCIE_GEN1);
	status = mdio_write_88xx(adapter, RAC_CTRL_PPR_V1,
				 tmp_u16 | BIT(12), HAL_INTF_PHY_PCIE_GEN1);
	if (status != HALMAC_RET_SUCCESS)
		return 0xFFFF;

	/* Obtain target */
	count = 0;
	do {
		PLTFM_DELAY_US(10);
		tmp_u16 = mdio_read_88xx(adapter, RAC_CTRL_PPR_V1,
					 HAL_INTF_PHY_PCIE_GEN1);
		count++;
		if (count > 100)
			break;
	} while ((tmp_u16 & BIT(12)) == BIT(12));
	if (count > 100) {
		PLTFM_MSG_ERR("[ERR]Get target timeout.\n");
		return 0xFFFF;
	}
	tar = mdio_read_88xx(adapter, RAC_CTRL_PPR_V1, HAL_INTF_PHY_PCIE_GEN1);
	tar = tar & 0x0FFF;
	if (tar == 0) {
		PLTFM_MSG_ERR("[ERR]Get target failed.\n");
		return 0xFFFF;
	}
	return tar;
}

#endif /* HALMAC_8822C_SUPPORT*/

/** @file */
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

#ifndef _MAC_AX_FWDL_H_
#define _MAC_AX_FWDL_H_

#include "../type.h"
#include "fwcmd.h"
#include "trx_desc.h"
#include "trxcfg.h"
#include "dle.h"
#include "hci_fc.h"
#include "power_saving.h"
#include "otpkeysinfo.h"

/****************************************************
 * Chip Register Offset, Bitfiled Mask and Shift
 * This Block Should be modify when new chip added
 ****************************************************/
//Build Time Marco
#define FWDL_SUPPORT_8852A MAC_AX_8852A_SUPPORT
#define FWDL_SUPPORT_8852B MAC_AX_8852B_SUPPORT
#define FWDL_SUPPORT_8852C MAC_AX_8852C_SUPPORT
#define FWDL_SUPPORT_8851B MAC_AX_8851B_SUPPORT
#define FWDL_SUPPORT_8851E MAC_AX_8851E_SUPPORT
#define FWDL_SUPPORT_8852D MAC_AX_8852D_SUPPORT
#define FWDL_SUPPORT_8192XB MAC_AX_8192XB_SUPPORT
#define FWDL_SUPPORT_8852BT 0

#define AX_SUPPORT \
(FWDL_SUPPORT_8852A || \
FWDL_SUPPORT_8852B || \
FWDL_SUPPORT_8852C || \
FWDL_SUPPORT_8851B || \
FWDL_SUPPORT_8851E || \
FWDL_SUPPORT_8852D || \
FWDL_SUPPORT_8192XB || \
FWDL_SUPPORT_8852BT)

#define AX_MIPS_SUPPORT \
(FWDL_SUPPORT_8852A || \
FWDL_SUPPORT_8852B || \
FWDL_SUPPORT_8851B || \
FWDL_SUPPORT_8852BT)

#define AX_RISCV_SUPPORT \
(FWDL_SUPPORT_8852C || \
FWDL_SUPPORT_8192XB || \
FWDL_SUPPORT_8851E || \
FWDL_SUPPORT_8852D)

#define AX_NOT_8851E_SUPPORT \
(FWDL_SUPPORT_8852A || \
FWDL_SUPPORT_8852B || \
FWDL_SUPPORT_8851B || \
FWDL_SUPPORT_8852BT || \
FWDL_SUPPORT_8852C || \
FWDL_SUPPORT_8192XB || \
FWDL_SUPPORT_8852D)

#define RESET_APB_WRAP_SUPPORT \
(FWDL_SUPPORT_8852B || \
FWDL_SUPPORT_8851B || \
FWDL_SUPPORT_8852BT)

/* Run Time Marco
 * Reference to mac_def.h::mac_ax_chip_id
 * When New Chip Added, Check the following API/Macro
 * fwdl.h::FWDL_PLE_BASE_ADDR
 * fwdl.c::fwdl_fw_debug_status_log
 * fwdl.c::
 */
#define IS_8852A is_chip_id(adapter, MAC_AX_CHIP_ID_8852A) // 0
#define IS_8852B is_chip_id(adapter, MAC_AX_CHIP_ID_8852B) // 1
#define IS_8852C is_chip_id(adapter, MAC_AX_CHIP_ID_8852C) // 2
#define IS_8192XB is_chip_id(adapter, MAC_AX_CHIP_ID_8192XB) // 3
#define IS_8851B is_chip_id(adapter, MAC_AX_CHIP_ID_8851B) // 4
#define IS_8851E is_chip_id(adapter, MAC_AX_CHIP_ID_8851E) // 5
#define IS_8852D is_chip_id(adapter, MAC_AX_CHIP_ID_8852D) // 6
#define IS_8852BT is_chip_id(adapter, MAC_AX_CHIP_ID_8852BT) // 7
#define IS_INVALID is_chip_id(adapter, MAC_AX_CHIP_ID_INVALID)

#define FWDL_REGISTERED_CHIP \
	(IS_8852A || \
	 IS_8852B || \
	 IS_8852C || \
	 IS_8192XB || \
	 IS_8851B || \
	 IS_8851E || \
	 IS_8852D || \
	 IS_8852BT)

#define IS_SUPPORT_REDL \
	(IS_8852B || \
	 IS_8851B || \
	 IS_8852BT)

#define IS_RESET_APB_WRAP \
	(IS_8852B || \
	 IS_8851B || \
	 IS_8852BT)

#define IS_AX \
	(IS_8852A || \
	 IS_8852B || \
	 IS_8852C || \
	 IS_8192XB || \
	 IS_8851B || \
	 IS_8851E || \
	 IS_8852D || \
	 IS_8852BT)

#define IS_AX_MIPS \
	(IS_8852A || \
	 IS_8852B || \
	 IS_8851B || \
	 IS_8852BT)

#define IS_AX_RISCV \
	(IS_8852C || \
	 IS_8192XB || \
	 IS_8851E || \
	 IS_8852D)

#if MAC_AX_PCIE_SUPPORT
#include "_pcie.h"
#endif

#ifdef MAC_8852A_SUPPORT
#include "../fw_ax/rtl8852a/hal8852a_fw.h"
#endif
#ifdef MAC_8852B_SUPPORT
#include "../fw_ax/rtl8852b/hal8852b_fw.h"
#endif
#ifdef MAC_8852C_SUPPORT
#include "../fw_ax/rtl8852c/hal8852c_fw.h"
#endif
#ifdef MAC_8192XB_SUPPORT
#include "../fw_ax/rtl8192xb/hal8192xb_fw.h"
#endif
#ifdef MAC_8851B_SUPPORT
#include "../fw_ax/rtl8851b/hal8851b_fw.h"
#endif
#ifdef MAC_8851E_SUPPORT
#include "../fw_ax/rtl8851e/hal8851e_fw.h"
#endif
#ifdef MAC_8852D_SUPPORT
#include "../fw_ax/rtl8852d/hal8852d_fw.h"
#endif
#ifdef MAC_8852BT_SUPPORT
#include "../fw_ax/rtl8852bt/hal8852bt_fw.h"
#endif

/**************************** PLE BASE ADDRESS ****************************/
#define FWDL_PLE_BASE_ADDR \
((IS_8852A || IS_8852C || IS_8192XB || IS_8851E || IS_8852D) ? 0xB8760000 :\
((IS_8852B || IS_8851B || IS_8852BT) ? 0xB8718000 : 0x0))

#define FWHDR_HDR_LEN (sizeof(struct fwhdr_hdr_t))
#define FWHDR_SECTION_LEN (sizeof(struct fwhdr_section_t))

#define ROMDL_SEG_LEN 0x40000

#define AX_BOOT_REASON_PWR_ON 0
#define AX_BOOT_REASON_WDT 1
#define AX_BOOT_REASON_LPS 2

#define RTL8852A_ROM_ADDR 0x18900000
#define RTL8852B_ROM_ADDR 0x18900000
#define RTL8852C_ROM_ADDR 0x20000000
#define RTL8192XB_ROM_ADDR 0x20000000
#define RTL8851B_ROM_ADDR 0x18900000
#define RTL8851E_ROM_ADDR 0x20000000
#define RTL8852D_ROM_ADDR 0x20000000
#define RTL8852BT_ROM_ADDR 0x18900000
#define FWDL_WAIT_CNT 400000
#define FWDL_SECTION_MAX_NUM 6
#define FWDL_SECURITY_SECTION_CONSTANT (64 + (FWDL_SECTION_MAX_NUM * 32 * 2))
#define FWDL_SECURITY_SECTION_TYPE 9
#define FWDL_SECURITY_SIGLEN 512
#define FWDL_SECTION_CHKSUM_LEN	8
#define FWDL_SECTION_PER_PKT_LEN 2020
#define FWDL_TRY_CNT 3

#define WDT_CTRL_ALL_DIS 0

/************************************************************************
 * INTERNAL FW MACRO DEFINE
 ************************************************************************/
#define FWDL_NO_INTERNAL_FW 0

#if defined(MAC_8852A_SUPPORT) && \
defined(MAC_FW_8852A_U2) && \
defined(PHL_FEATURE_AP) && \
defined(MAC_FW_CATEGORY_AP)
#define INTERNAL_FW_CONTENT_8852A_CBV_AP array_8852a_u2_ap
#define INTERNAL_FW_LEN_8852A_CBV_AP array_length_8852a_u2_ap
#else
#define INTERNAL_FW_CONTENT_8852A_CBV_AP FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852A_CBV_AP FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852A_SUPPORT) && \
defined(MAC_FW_8852A_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8852A_CBV_NIC array_8852a_u2_nic
#define INTERNAL_FW_LEN_8852A_CBV_NIC array_length_8852a_u2_nic
#else
#define INTERNAL_FW_CONTENT_8852A_CBV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852A_CBV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852A_SUPPORT) && \
defined(MAC_FW_8852A_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8852A_CBV_WOWLAN array_8852a_u2_wowlan
#define INTERNAL_FW_LEN_8852A_CBV_WOWLAN array_length_8852a_u2_wowlan
#else
#define INTERNAL_FW_CONTENT_8852A_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852A_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852A_SUPPORT) && \
defined(MAC_FW_8852A_U3) && \
defined(PHL_FEATURE_AP) && \
defined(MAC_FW_CATEGORY_AP)
#define INTERNAL_FW_CONTENT_8852A_CCV_AP array_8852a_u3_ap
#define INTERNAL_FW_LEN_8852A_CCV_AP array_length_8852a_u3_ap
#else
#define INTERNAL_FW_CONTENT_8852A_CCV_AP FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852A_CCV_AP FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852A_SUPPORT) && \
defined(MAC_FW_8852A_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8852A_CCV_NIC array_8852a_u3_nic
#define INTERNAL_FW_LEN_8852A_CCV_NIC array_length_8852a_u3_nic
#else
#define INTERNAL_FW_CONTENT_8852A_CCV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852A_CCV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852A_SUPPORT) && \
defined(MAC_FW_8852A_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8852A_CCV_WOWLAN array_8852a_u3_wowlan
#define INTERNAL_FW_LEN_8852A_CCV_WOWLAN array_length_8852a_u3_wowlan
#else
#define INTERNAL_FW_CONTENT_8852A_CCV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852A_CCV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NICCE)
#define INTERNAL_FW_CONTENT_8852B_CBV_NICCE array_8852b_u2_nicce
#define INTERNAL_FW_LEN_8852B_CBV_NICCE array_length_8852b_u2_nicce
#else
#define INTERNAL_FW_CONTENT_8852B_CBV_NICCE FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CBV_NICCE FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NICCE_BPLUS)
#define INTERNAL_FW_CONTENT_8852B_CBV_NICCE_BPLUS array_8852b_u2_nicce_bplus
#define INTERNAL_FW_LEN_8852B_CBV_NICCE_BPLUS array_length_8852b_u2_nicce_bplus
#else
#define INTERNAL_FW_CONTENT_8852B_CBV_NICCE_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CBV_NICCE_BPLUS FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8852B_CBV_NIC array_8852b_u2_nic
#define INTERNAL_FW_LEN_8852B_CBV_NIC array_length_8852b_u2_nic
#else
#define INTERNAL_FW_CONTENT_8852B_CBV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CBV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC_BPLUS)
#define INTERNAL_FW_CONTENT_8852B_CBV_NIC_BPLUS array_8852b_u2_nic_bplus
#define INTERNAL_FW_LEN_8852B_CBV_NIC_BPLUS array_length_8852b_u2_nic_bplus
#else
#define INTERNAL_FW_CONTENT_8852B_CBV_NIC_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CBV_NIC_BPLUS FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC_PLE)
#define INTERNAL_FW_CONTENT_8852B_CBV_NIC_PLE array_8852b_u2_nic_ple
#define INTERNAL_FW_LEN_8852B_CBV_NIC_PLE array_length_8852b_u2_nic_ple
#else
#define INTERNAL_FW_CONTENT_8852B_CBV_NIC_PLE FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CBV_NIC_PLE FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8852B_CBV_WOWLAN array_8852b_u2_wowlan
#define INTERNAL_FW_LEN_8852B_CBV_WOWLAN array_length_8852b_u2_wowlan
#else
#define INTERNAL_FW_CONTENT_8852B_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN_BPLUS)
#define INTERNAL_FW_CONTENT_8852B_CBV_WOWLAN_BPLUS array_8852b_u2_wowlan_bplus
#define INTERNAL_FW_LEN_8852B_CBV_WOWLAN_BPLUS array_length_8852b_u2_wowlan_bplus
#else
#define INTERNAL_FW_CONTENT_8852B_CBV_WOWLAN_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CBV_WOWLAN_BPLUS FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NICCE)
#define INTERNAL_FW_CONTENT_8852B_CCV_NICCE array_8852b_u3_nicce
#define INTERNAL_FW_LEN_8852B_CCV_NICCE array_length_8852b_u3_nicce
#else
#define INTERNAL_FW_CONTENT_8852B_CCV_NICCE FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CCV_NICCE FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NICCE_BPLUS)
#define INTERNAL_FW_CONTENT_8852B_CCV_NICCE_BPLUS array_8852b_u3_nicce_bplus
#define INTERNAL_FW_LEN_8852B_CCV_NICCE_BPLUS array_length_8852b_u3_nicce_bplus
#else
#define INTERNAL_FW_CONTENT_8852B_CCV_NICCE_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CCV_NICCE_BPLUS FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8852B_CCV_NIC array_8852b_u3_nic
#define INTERNAL_FW_LEN_8852B_CCV_NIC array_length_8852b_u3_nic
#else
#define INTERNAL_FW_CONTENT_8852B_CCV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CCV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC_BPLUS)
#define INTERNAL_FW_CONTENT_8852B_CCV_NIC_BPLUS array_8852b_u3_nic_bplus
#define INTERNAL_FW_LEN_8852B_CCV_NIC_BPLUS array_length_8852b_u3_nic_bplus
#else
#define INTERNAL_FW_CONTENT_8852B_CCV_NIC_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CCV_NIC_BPLUS FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC_PLE)
#define INTERNAL_FW_CONTENT_8852B_CCV_NIC_PLE array_8852b_u3_nic_ple
#define INTERNAL_FW_LEN_8852B_CCV_NIC_PLE array_length_8852b_u3_nic_ple
#else
#define INTERNAL_FW_CONTENT_8852B_CCV_NIC_PLE FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CCV_NIC_PLE FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8852B_CCV_WOWLAN array_8852b_u3_wowlan
#define INTERNAL_FW_LEN_8852B_CCV_WOWLAN array_length_8852b_u3_wowlan
#else
#define INTERNAL_FW_CONTENT_8852B_CCV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CCV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852B_SUPPORT) && \
defined(MAC_FW_8852B_U3) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN_BPLUS)
#define INTERNAL_FW_CONTENT_8852B_CCV_WOWLAN_BPLUS array_8852b_u3_wowlan_bplus
#define INTERNAL_FW_LEN_8852B_CCV_WOWLAN_BPLUS array_length_8852b_u3_wowlan_bplus
#else
#define INTERNAL_FW_CONTENT_8852B_CCV_WOWLAN_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852B_CCV_WOWLAN_BPLUS FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852C_SUPPORT) && \
defined(MAC_FW_8852C_U1) && \
defined(PHL_FEATURE_AP) && \
defined(MAC_FW_CATEGORY_AP)
#define INTERNAL_FW_CONTENT_8852C_CAV_AP array_8852c_u1_ap
#define INTERNAL_FW_LEN_8852C_CAV_AP array_length_8852c_u1_ap
#else
#define INTERNAL_FW_CONTENT_8852C_CAV_AP FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852C_CAV_AP FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852C_SUPPORT) && \
defined(MAC_FW_8852C_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8852C_CAV_NIC array_8852c_u1_nic
#define INTERNAL_FW_LEN_8852C_CAV_NIC array_length_8852c_u1_nic
#else
#define INTERNAL_FW_CONTENT_8852C_CAV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852C_CAV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852C_SUPPORT) && \
defined(MAC_FW_8852C_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8852C_CAV_WOWLAN array_8852c_u1_wowlan
#define INTERNAL_FW_LEN_8852C_CAV_WOWLAN array_length_8852c_u1_wowlan
#else
#define INTERNAL_FW_CONTENT_8852C_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852C_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852C_SUPPORT) && \
defined(MAC_FW_8852C_U2) && \
defined(PHL_FEATURE_AP) && \
defined(MAC_FW_CATEGORY_AP)
#define INTERNAL_FW_CONTENT_8852C_CBV_AP array_8852c_u2_ap
#define INTERNAL_FW_LEN_8852C_CBV_AP array_length_8852c_u2_ap
#else
#define INTERNAL_FW_CONTENT_8852C_CBV_AP FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852C_CBV_AP FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852C_SUPPORT) && \
defined(MAC_FW_8852C_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8852C_CBV_NIC array_8852c_u2_nic
#define INTERNAL_FW_LEN_8852C_CBV_NIC array_length_8852c_u2_nic
#else
#define INTERNAL_FW_CONTENT_8852C_CBV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852C_CBV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852C_SUPPORT) && \
defined(MAC_FW_8852C_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8852C_CBV_WOWLAN array_8852c_u2_wowlan
#define INTERNAL_FW_LEN_8852C_CBV_WOWLAN array_length_8852c_u2_wowlan
#else
#define INTERNAL_FW_CONTENT_8852C_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852C_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8192XB_SUPPORT) && \
defined(MAC_FW_8192XB_U1) && \
defined(PHL_FEATURE_AP) && \
defined(MAC_FW_CATEGORY_AP)
#define INTERNAL_FW_CONTENT_8192XB_CAV_AP array_8192xb_u1_ap
#define INTERNAL_FW_LEN_8192XB_CAV_AP array_length_8192xb_u1_ap
#else
#define INTERNAL_FW_CONTENT_8192XB_CAV_AP FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8192XB_CAV_AP FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8192XB_SUPPORT) && \
defined(MAC_FW_8192XB_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8192XB_CAV_NIC array_8192xb_u1_nic
#define INTERNAL_FW_LEN_8192XB_CAV_NIC array_length_8192xb_u1_nic
#else
#define INTERNAL_FW_CONTENT_8192XB_CAV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8192XB_CAV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8192XB_SUPPORT) && \
defined(MAC_FW_8192XB_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8192XB_CAV_WOWLAN array_8192xb_u1_wowlan
#define INTERNAL_FW_LEN_8192XB_CAV_WOWLAN array_length_8192xb_u1_wowlan
#else
#define INTERNAL_FW_CONTENT_8192XB_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8192XB_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8192XB_SUPPORT) && \
defined(MAC_FW_8192XB_U2) && \
defined(PHL_FEATURE_AP) && \
defined(MAC_FW_CATEGORY_AP)
#define INTERNAL_FW_CONTENT_8192XB_CBV_AP array_8192xb_u2_ap
#define INTERNAL_FW_LEN_8192XB_CBV_AP array_length_8192xb_u2_ap
#else
#define INTERNAL_FW_CONTENT_8192XB_CBV_AP FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8192XB_CBV_AP FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8192XB_SUPPORT) && \
defined(MAC_FW_8192XB_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8192XB_CBV_NIC array_8192xb_u2_nic
#define INTERNAL_FW_LEN_8192XB_CBV_NIC array_length_8192xb_u2_nic
#else
#define INTERNAL_FW_CONTENT_8192XB_CBV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8192XB_CBV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8192XB_SUPPORT) && \
defined(MAC_FW_8192XB_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8192XB_CBV_WOWLAN array_8192xb_u2_wowlan
#define INTERNAL_FW_LEN_8192XB_CBV_WOWLAN array_length_8192xb_u2_wowlan
#else
#define INTERNAL_FW_CONTENT_8192XB_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8192XB_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8851B_SUPPORT) && \
defined(MAC_FW_8851B_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8851B_CAV_NIC array_8851b_u1_nic
#define INTERNAL_FW_LEN_8851B_CAV_NIC array_length_8851b_u1_nic
#else
#define INTERNAL_FW_CONTENT_8851B_CAV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8851B_CAV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8851B_SUPPORT) && \
defined(MAC_FW_8851B_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8851B_CAV_WOWLAN array_8851b_u1_wowlan
#define INTERNAL_FW_LEN_8851B_CAV_WOWLAN array_length_8851b_u1_wowlan
#else
#define INTERNAL_FW_CONTENT_8851B_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8851B_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8851B_SUPPORT) && \
defined(MAC_FW_8851B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8851B_CBV_NIC array_8851b_u2_nic
#define INTERNAL_FW_LEN_8851B_CBV_NIC array_length_8851b_u2_nic
#else
#define INTERNAL_FW_CONTENT_8851B_CBV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8851B_CBV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8851B_SUPPORT) && \
defined(MAC_FW_8851B_U2) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8851B_CBV_WOWLAN array_8851b_u2_wowlan
#define INTERNAL_FW_LEN_8851B_CBV_WOWLAN array_length_8851b_u2_wowlan
#else
#define INTERNAL_FW_CONTENT_8851B_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8851B_CBV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8851E_SUPPORT) && \
defined(MAC_FW_8851E_U1) && \
defined(PHL_FEATURE_AP) && \
defined(MAC_FW_CATEGORY_AP)
#define INTERNAL_FW_CONTENT_8851E_CAV_AP array_8851e_u1_ap
#define INTERNAL_FW_LEN_8851E_CAV_AP array_length_8851e_u1_ap
#else
#define INTERNAL_FW_CONTENT_8851E_CAV_AP FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8851E_CAV_AP FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8851E_SUPPORT) && \
defined(MAC_FW_8851E_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8851E_CAV_NIC array_8851e_u1_nic
#define INTERNAL_FW_LEN_8851E_CAV_NIC array_length_8851e_u1_nic
#else
#define INTERNAL_FW_CONTENT_8851E_CAV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8851E_CAV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8851E_SUPPORT) && \
defined(MAC_FW_8851E_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8851E_CAV_WOWLAN array_8851e_u1_wowlan
#define INTERNAL_FW_LEN_8851E_CAV_WOWLAN array_length_8851e_u1_wowlan
#else
#define INTERNAL_FW_CONTENT_8851E_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8851E_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852D_SUPPORT) && \
defined(MAC_FW_8852D_U1) && \
defined(PHL_FEATURE_AP) && \
defined(MAC_FW_CATEGORY_AP)
#define INTERNAL_FW_CONTENT_8852D_CAV_AP array_8852d_u1_ap
#define INTERNAL_FW_LEN_8852D_CAV_AP array_length_8852d_u1_ap
#else
#define INTERNAL_FW_CONTENT_8852D_CAV_AP FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852D_CAV_AP FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852D_SUPPORT) && \
defined(MAC_FW_8852D_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8852D_CAV_NIC array_8852d_u1_nic
#define INTERNAL_FW_LEN_8852D_CAV_NIC array_length_8852d_u1_nic
#else
#define INTERNAL_FW_CONTENT_8852D_CAV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852D_CAV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852D_SUPPORT) && \
defined(MAC_FW_8852D_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8852D_CAV_WOWLAN array_8852d_u1_wowlan
#define INTERNAL_FW_LEN_8852D_CAV_WOWLAN array_length_8852d_u1_wowlan
#else
#define INTERNAL_FW_CONTENT_8852D_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852D_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852BT_SUPPORT) && \
defined(MAC_FW_8852BT_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NICCE)
#define INTERNAL_FW_CONTENT_8852BT_CAV_NICCE array_8852bt_u1_nicce
#define INTERNAL_FW_LEN_8852BT_CAV_NICCE array_length_8852bt_u1_nicce
#else
#define INTERNAL_FW_CONTENT_8852BT_CAV_NICCE FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852BT_CAV_NICCE FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852BT_SUPPORT) && \
defined(MAC_FW_8852BT_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NICCE_BPLUS)
#define INTERNAL_FW_CONTENT_8852BT_CAV_NICCE_BPLUS array_8852bt_u1_nicce_bplus
#define INTERNAL_FW_LEN_8852BT_CAV_NICCE_BPLUS array_length_8852bt_u1_nicce_bplus
#else
#define INTERNAL_FW_CONTENT_8852BT_CAV_NICCE_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852BT_CAV_NICCE_BPLUS FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852BT_SUPPORT) && \
defined(MAC_FW_8852BT_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC)
#define INTERNAL_FW_CONTENT_8852BT_CAV_NIC array_8852bt_u1_nic
#define INTERNAL_FW_LEN_8852BT_CAV_NIC array_length_8852bt_u1_nic
#else
#define INTERNAL_FW_CONTENT_8852BT_CAV_NIC FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852BT_CAV_NIC FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852BT_SUPPORT) && \
defined(MAC_FW_8852BT_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC_BPLUS)
#define INTERNAL_FW_CONTENT_8852BT_CAV_NIC_BPLUS array_8852bt_u1_nic_bplus
#define INTERNAL_FW_LEN_8852BT_CAV_NIC_BPLUS array_length_8852bt_u1_nic_bplus
#else
#define INTERNAL_FW_CONTENT_8852BT_CAV_NIC_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852BT_CAV_NIC_BPLUS FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852BT_SUPPORT) && \
defined(MAC_FW_8852BT_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(MAC_FW_CATEGORY_NIC_PLE)
#define INTERNAL_FW_CONTENT_8852BT_CAV_NIC_PLE array_8852bt_u1_nic_ple
#define INTERNAL_FW_LEN_8852BT_CAV_NIC_PLE array_length_8852bt_u1_nic_ple
#else
#define INTERNAL_FW_CONTENT_8852BT_CAV_NIC_PLE FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852BT_CAV_NIC_PLE FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852BT_SUPPORT) && \
defined(MAC_FW_8852BT_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN)
#define INTERNAL_FW_CONTENT_8852BT_CAV_WOWLAN array_8852bt_u1_wowlan
#define INTERNAL_FW_LEN_8852BT_CAV_WOWLAN array_length_8852bt_u1_wowlan
#else
#define INTERNAL_FW_CONTENT_8852BT_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852BT_CAV_WOWLAN FWDL_NO_INTERNAL_FW
#endif

#if defined(MAC_8852BT_SUPPORT) && \
defined(MAC_FW_8852BT_U1) && \
defined(PHL_FEATURE_NIC) && \
defined(CONFIG_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN) && \
defined(MAC_FW_CATEGORY_WOWLAN_BPLUS)
#define INTERNAL_FW_CONTENT_8852BT_CAV_WOWLAN_BPLUS array_8852bt_u1_wowlan_bplus
#define INTERNAL_FW_LEN_8852BT_CAV_WOWLAN_BPLUS array_length_8852bt_u1_wowlan_bplus
#else
#define INTERNAL_FW_CONTENT_8852BT_CAV_WOWLAN_BPLUS FWDL_NO_INTERNAL_FW
#define INTERNAL_FW_LEN_8852BT_CAV_WOWLAN_BPLUS FWDL_NO_INTERNAL_FW
#endif

struct fwhdr_section_info {
	u8 redl;
	u8 *addr;
	u32 len;
	u32 dladdr;
	u32 mssc;
	u8 type;
};

struct fw_bin_info {
	u8 section_num;
	u32 hdr_len;
	u32 git_idx;
	u32 is_fw_use_ple;
	u8 dynamic_hdr_en;
	u32 dynamic_hdr_len;
	struct fwhdr_section_info section_info[FWDL_SECTION_MAX_NUM];
};

struct hw_info {
	u8 chip;
	u8 cut;
	u8 category;
};

struct fwld_info {
	u32 len;
	u8 *fw;
};

/**
 * @struct fwhdr_hdr_t
 * @brief fwhdr_hdr_t
 *
 * @var fwhdr_hdr_t::dword0
 * Please Place Description here.
 * @var fwhdr_hdr_t::dword1
 * Please Place Description here.
 * @var fwhdr_hdr_t::dword2
 * Please Place Description here.
 * @var fwhdr_hdr_t::dword3
 * Please Place Description here.
 * @var fwhdr_hdr_t::dword4
 * Please Place Description here.
 * @var fwhdr_hdr_t::dword5
 * Please Place Description here.
 * @var fwhdr_hdr_t::dword6
 * Please Place Description here.
 * @var fwhdr_hdr_t::dword7
 * Please Place Description here.
 */
struct fwhdr_hdr_t {
	u32 dword0;
	u32 dword1;
	u32 dword2;
	u32 dword3;
	u32 dword4;
	u32 dword5;
	u32 dword6;
	u32 dword7;
};

/**
 * @struct fwhdr_section_t
 * @brief fwhdr_section_t
 *
 * @var fwhdr_section_t::dword0
 * Please Place Description here.
 * @var fwhdr_section_t::dword1
 * Please Place Description here.
 * @var fwhdr_section_t::dword2
 * Please Place Description here.
 * @var fwhdr_section_t::dword3
 * Please Place Description here.
 */
struct fwhdr_section_t {
	u32 dword0;
	u32 dword1;
	u32 dword2;
	u32 dword3;
};

/**
 * @enum fw_dl_status
 *
 * @brief fw_dl_status
 *
 * @var fw_dl_status::FWDL_INITIAL_STATE
 * Please Place Description here.
 * @var fw_dl_status::FWDL_FWDL_ONGOING
 * Please Place Description here.
 * @var fw_dl_status::FWDL_CHECKSUM_FAIL
 * Please Place Description here.
 * @var fw_dl_status::FWDL_SECURITY_FAIL
 * Please Place Description here.
 * @var fw_dl_status::FWDL_CUT_NOT_MATCH
 * Please Place Description here.
 * @var fw_dl_status::FWDL_RSVD0
 * Please Place Description here.
 * @var fw_dl_status::FWDL_WCPU_FWDL_RDY
 * Please Place Description here.
 * @var fw_dl_status::FWDL_WCPU_FW_INIT_RDY
 * Please Place Description here.
 */
enum fw_dl_status {
	FWDL_INITIAL_STATE = 0,
	FWDL_FWDL_ONGOING = 1,
	FWDL_CHECKSUM_FAIL = 2,
	FWDL_SECURITY_FAIL = 3,
	FWDL_CUT_NOT_MATCH = 4,
	FWDL_RSVD0 = 5,
	FWDL_WCPU_FWDL_RDY = 6,
	FWDL_WCPU_FW_INIT_RDY = 7
};

/**
 * @enum fw_dl_cv
 *
 * @brief fw_dl_cv
 *
 * @var fw_dl_chip_cut::FWDL_CAV
 * Please Place Description here.
 * @var fw_dl_chip_cut::FWDL_CBV
 * Please Place Description here.
 * @var fw_dl_chip_cut::FWDL_CCV
 * Please Place Description here.
 * @var fw_dl_chip_cut::FWDL_CDV
 * Please Place Description here.
 * @var fw_dl_chip_cut::FWDL_CEV
 * Please Place Description here.
 * @var fw_dl_chip_cut::FWDL_CFV
 * Please Place Description here.
 * @var fw_dl_chip_cut::FWDL_CGV
 * Please Place Description here.
 * @var fw_dl_chip_cut::FWDL_CHV
 * Please Place Description here.
 * @var fw_dl_chip_cut::FWDL_CIV
 * Please Place Description here.
 */
enum fw_dl_cv {
	FWDL_CAV = 0,
	FWDL_CBV = 1,
	FWDL_CCV,
	FWDL_CDV,
	FWDL_CEV,
	FWDL_CFV,
	FWDL_CGV,
	FWDL_CHV,
	FWDL_CIV,
};

/**
 * @enum fwdl_dynamic_hdr_type
 *
 * @brief fwdl_dynamic_hdr_type
 *
 * @var fwdl_dynamic_hdr_type::FWDL_DYNAMIC_HDR_NOUSE
 * Please Place Description here.
 * @var fwdl_dynamic_hdr_type::FWDL_DYNAMIC_HDR_FWCAP
 * Please Place Description here.
 * @var fwdl_dynamic_hdr_type::FWDL_DYNAMIC_HDR_MAX
 * Please Place Description here.
 */
enum fwdl_dynamic_hdr_type {
	FWDL_DYNAMIC_HDR_NOUSE = 0,
	FWDL_DYNAMIC_HDR_FWCAP = 1,
	FWDL_DYNAMIC_HDR_OUTSRC_GIT_INFO = 2,
	FWDL_DYNAMIC_HDR_MAX
};

/* === FW header === */
/* dword0 */
#define FWHDR_CUTID_SH 0
#define FWHDR_CUTID_MSK 0xff
#define FWHDR_CHIPID_SH 8
#define FWHDR_CHIPID_MSK 0xffffff

/* dword1 */
#define FWHDR_MAJORVER_SH 0
#define FWHDR_MAJORVER_MSK 0xff
#define FWHDR_MINORVER_SH 8
#define FWHDR_MINORVER_MSK 0xff
#define FWHDR_SUBVERSION_SH 16
#define FWHDR_SUBVERSION_MSK 0xff
#define FWHDR_SUBINDEX_SH 24
#define FWHDR_SUBINDEX_MSK 0xff

/* dword2 */
#define FWHDR_COMMITID_SH 0
#define FWHDR_COMMITID_MSK 0xffffffff

/* dword3 */
#define FWHDR_SEC_HDR_OFFSET_SH 0
#define FWHDR_SEC_HDR_OFFSET_MSK 0xff
#define FWHDR_SEC_HDR_SZ_SH 8
#define FWHDR_SEC_HDR_SZ_MSK 0xff
#define FWHDR_FWHDR_SZ_SH 16
#define FWHDR_FWHDR_SZ_MSK 0xff
#define FWHDR_FWHDR_VER_SH 24
#define FWHDR_FWHDR_VER_MSK 0xff

/* dword4 */
#define FWHDR_MONTH_SH 0
#define FWHDR_MONTH_MSK 0xff
#define FWHDR_DATE_SH 8
#define FWHDR_DATE_MSK 0xff
#define FWHDR_HOUR_SH 16
#define FWHDR_HOUR_MSK 0xff
#define FWHDR_MIN_SH 24
#define FWHDR_MIN_MSK 0xff

/* dword5 */
#define FWHDR_YEAR_SH 0
#define FWHDR_YEAR_MSK 0xffff

/* dword6 */
#define FWHDR_IMAGEFROM_SH 0
#define FWHDR_IMAGEFROM_MSK 0x3
#define FWHDR_BOOTFROM_SH 4
#define FWHDR_BOOTFROM_MSK 0x3
#define FWHDR_ROM_ONLY BIT(6)
#define FWHDR_FW_TYPE BIT(7)
#define FWHDR_SEC_NUM_SH 8
#define FWHDR_SEC_NUM_MSK 0xff
#define FWHDR_HCI_TYPE_SH 16
#define FWHDR_HCI_TYPE_MSK 0xf
#define FWHDR_NET_TYPE_SH 20
#define FWHDR_NET_TYPE_MSK 0xf

/* dword7 */
#define FWHDR_FW_PART_SZ_SH 0
#define FWHDR_FW_PART_SZ_MSK 0xffff
#define FWHDR_FW_DYN_HDR_SH 16
#define FWHDR_FW_DYN_HDR_MSK 0x1
#define FWHDR_CMD_VER_SH 24
#define FWHDR_CMD_VER_MSK 0xff

/* === Section header === */
/* dword0 */
#define SECTION_INFO_SEC_DL_ADDR_SH 0
#define SECTION_INFO_SEC_DL_ADDR_MSK 0xffffffff

/* dword1 */
#define SECTION_INFO_SEC_SIZE_SH 0
#define SECTION_INFO_SEC_SIZE_MSK 0xffffff
#define SECTION_INFO_SECTIONTYPE_SH 24
#define SECTION_INFO_SECTIONTYPE_MSK 0xf
#define SECTION_INFO_CHECKSUM BIT(28)
#define SECTION_INFO_REDL BIT(29)

/**
 * @addtogroup Firmware
 * @{
 * @addtogroup FW_Download
 * @{
 */
/**
 * @brief mac_fwredl
 *
 * @param *adapter
 * @param *fw
 * @param len
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_fwredl(struct mac_ax_adapter *adapter, u8 *fw, u32 len);
/**
 * @}
 * @}
 */

/**
 * @addtogroup Firmware
 * @{
 * @addtogroup FW_Download
 * @{
 */

/**
 * @brief mac_fwdl
 *
 * @param *adapter
 * @param *fw
 * @param len
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_fwdl(struct mac_ax_adapter *adapter, u8 *fw, u32 len);
/**
 * @}
 * @}
 */

/**
 * @addtogroup Firmware
 * @{
 * @addtogroup FW_Download
 * @{
 */

/**
 * @brief mac_enable_cpu
 *
 * @param *adapter
 * @param boot_reason
 * @param dlfw
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_enable_cpu(struct mac_ax_adapter *adapter, u8 boot_reason, u8 dlfw);
/**
 * @}
 * @}
 */

/**
 * @addtogroup Firmware
 * @{
 * @addtogroup FW_Download
 * @{
 */

/**
 * @brief mac_disable_cpu
 *
 * @param *adapter
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_disable_cpu(struct mac_ax_adapter *adapter);
/**
 * @}
 * @}
 */

/**
 * @addtogroup Firmware
 * @{
 * @addtogroup FW_Download
 * @{
 */

/**
 * @brief mac_romdl
 *
 * @param *adapter
 * @param *rom
 * @param romaddr
 * @param len
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_romdl(struct mac_ax_adapter *adapter, u8 *rom, u32 romaddr, u32 len);
/**
 * @}
 * @}
 */

/**
 * @addtogroup Firmware
 * @{
 * @addtogroup FW_Download
 * @{
 */

/**
 * @brief mac_ram_boot
 *
 * @param *adapter
 * @param *fw
 * @param len
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_ram_boot(struct mac_ax_adapter *adapter, u8 *fw, u32 len);
/**
 * @}
 * @}
 */

/**
 * @addtogroup Firmware
 * @{
 * @addtogroup FW_Download
 * @{
 */

/**
 * @brief mac_enable_fw
 *
 * @param *adapter
 * @param cat
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_enable_fw(struct mac_ax_adapter *adapter, enum rtw_fw_type cat);
/**
 * @}
 * @}
 */

/**
 * @brief mac_query_fw_buff
 *
 * @param *adapter
 * @param cat
 * @param **fw
 * @param *fw_len
 * @return Please Place Description here.
 * @retval u32
 */
u32 mac_query_fw_buff(struct mac_ax_adapter *adapter, enum rtw_fw_type cat, u8 **fw, u32 *fw_len);
/**
 * @}
 * @}
 */

/**
 * @brief mac_get_dynamic_hdr_ax
 *
 * @param *adapter
 * @param *fw
 * @param fw_len
 * @retval u32
 */
u32 mac_get_dynamic_hdr_ax(struct mac_ax_adapter *adapter, u8 *fw, u32 fw_len);
/**
 * @}
 * @}
 */
#endif
